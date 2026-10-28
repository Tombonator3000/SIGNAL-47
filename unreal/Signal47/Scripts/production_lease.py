#!/usr/bin/env python3
"""Cooperative Linux reservation for a whole heavy production job series.

The lock is global across worktrees and databases. Keep a HeavyReservation open
across build/test process gaps, and pass environment() to its child tools. Flock
releases on process exit; stale JSON is informational and never a live lock.
Older worktrees without the unreal_guard hook cannot observe this reservation.
This module never signals or reclaims another process.
"""

from datetime import datetime, timezone
import errno
import fcntl
import hmac
import json
import os
from pathlib import Path
import secrets
import stat


LOCK_PATH_ENV = "S47_PRODUCTION_LOCK_PATH"
TOKEN_ENV = "S47_PRODUCTION_TOKEN"
MAX_METADATA_BYTES = 16_384


class ReservationBusy(RuntimeError):
    """A live owner holds the cooperative reservation."""


def _path(value=None):
    value = value if value is not None else os.environ.get(LOCK_PATH_ENV)
    path = Path(value).expanduser() if value else (
        Path.home() / ".local/state/signal47-production/heavy.lock")
    # Do not resolve symlinks before checking them. Keep one pathname/inode for
    # every participant instead of silently locking a symlink target.
    path = Path(os.path.abspath(path))
    for parent in reversed((path, *path.parents)):
        if parent.is_symlink():
            raise RuntimeError(f"Unsafe symlink in reservation path: {parent}")
    return path


def _open(path):
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    flags = os.O_RDWR | os.O_CREAT | os.O_CLOEXEC | os.O_NOFOLLOW
    try:
        fd = os.open(path, flags, 0o600)
    except OSError as error:
        raise RuntimeError(f"Cannot open reservation file: {path}: {error}") from error
    info = os.fstat(fd)
    if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_uid != os.getuid():
        os.close(fd)
        raise RuntimeError(f"Reservation must be an owned, unlinked regular file: {path}")
    return fd


def _process(pid):
    """Return parent PID and Linux start ticks, checking live process identity."""
    value = (Path("/proc") / str(pid) / "stat").read_text()
    fields = value.rsplit(")", 1)[1].split()
    if fields[0] == "Z":
        raise ProcessLookupError(pid)
    return int(fields[1]), fields[19]  # original fields 4 and 22


def _boot_id():
    return Path("/proc/sys/kernel/random/boot_id").read_text().strip()


def _is_descendant(pid, owner):
    seen = set()
    while pid > 0 and pid not in seen:
        if pid == owner:
            return True
        seen.add(pid)
        try:
            pid, _ = _process(pid)
        except (OSError, ValueError, IndexError):
            return False
    return False


def _read(fd):
    try:
        raw = os.pread(fd, MAX_METADATA_BYTES + 1, 0)
        if len(raw) > MAX_METADATA_BYTES:
            raise ValueError("metadata exceeds size limit")
        data = json.loads(raw)
        if (not isinstance(data, dict) or type(data.get("version")) is not int or data["version"] != 1 or
                type(data.get("pid")) is not int or data["pid"] <= 0 or
                not isinstance(data.get("start_ticks"), str) or
                not data["start_ticks"].isdigit() or
                not isinstance(data.get("boot_id"), str) or
                not isinstance(data.get("token"), str) or
                len(data["token"]) != 64 or any(c not in "0123456789abcdef" for c in data["token"]) or
                not isinstance(data.get("label"), str)):
            raise ValueError("invalid reservation fields")
        return data
    except (ValueError, UnicodeError, OSError, TypeError) as error:
        raise ReservationBusy("Heavy production reservation is held with invalid owner metadata") from error


def _public(data, path):
    return {key: value for key, value in data.items() if key != "token"} | {"lock_path": str(path)}


class HeavyReservation:
    """An exclusive reservation whose lifetime spans all tools in one job."""

    def __init__(self, label: str, lock_path: Path | None = None):
        if not isinstance(label, str) or not label.strip() or len(label) > 512:
            raise ValueError("Reservation label must contain 1 to 512 characters")
        self.label = label
        self.path = _path(lock_path)
        self._fd = None
        self._metadata = None

    def acquire(self):
        if self._fd is not None:
            raise RuntimeError("Reservation already acquired by this instance")
        fd = _open(self.path)
        try:
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except OSError as error:
                if error.errno not in (errno.EACCES, errno.EAGAIN):
                    raise
                raise ReservationBusy("Another heavy production job holds the reservation") from error
            _, start_ticks = _process(os.getpid())
            data = {"version": 1, "pid": os.getpid(), "start_ticks": start_ticks,
                    "boot_id": _boot_id(), "token": secrets.token_hex(32),
                    "label": self.label,
                    "acquired_at": datetime.now(timezone.utc).isoformat()}
            payload = (json.dumps(data, ensure_ascii=False) + "\n").encode("utf-8")
            os.fchmod(fd, 0o600)
            os.ftruncate(fd, 0)
            os.lseek(fd, 0, os.SEEK_SET)
            while payload:
                count = os.write(fd, payload)
                if count <= 0:
                    raise OSError("Could not write reservation metadata")
                payload = payload[count:]
            os.fsync(fd)
            self._fd, self._metadata = fd, data
        except BaseException:
            os.close(fd)  # Also releases a lock acquired before a write error.
            raise
        return self

    def environment(self):
        if self._fd is None:
            raise RuntimeError("Acquire reservation before requesting its child environment")
        return {LOCK_PATH_ENV: str(self.path), TOKEN_ENV: self._metadata["token"]}

    def release(self):
        if self._fd is None:
            return
        fd, self._fd = self._fd, None
        self._metadata = None
        try:
            fcntl.flock(fd, fcntl.LOCK_UN)
        finally:
            os.close(fd)
        # Keep last-owner information. Never truncate after unlock: a new owner
        # may already have acquired the inode and written its metadata.

    def __enter__(self):
        return self.acquire()

    def __exit__(self, exc_type, exc_value, traceback):
        self.release()


def assert_reservation_access():
    """Allow an idle lock or its proven owner's child, fail closed otherwise."""
    path = _path()
    fd = _open(path)
    try:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError as error:
            if error.errno not in (errno.EACCES, errno.EAGAIN):
                raise
        else:
            fcntl.flock(fd, fcntl.LOCK_UN)
            return None
        data = _read(fd)
        supplied = os.environ.get(TOKEN_ENV, "")
        try:
            _, start_ticks = _process(data["pid"])
            valid_owner = start_ticks == data["start_ticks"] and _boot_id() == data["boot_id"]
        except (OSError, ValueError, IndexError):
            valid_owner = False
        if (not valid_owner or not supplied or
                not hmac.compare_digest(supplied.encode("utf-8"), data["token"].encode("ascii")) or
                not _is_descendant(os.getpid(), data["pid"])):
            raise ReservationBusy(
                f"Heavy production reservation held by PID {data['pid']} ({data['label']!r}); no process was stopped")
        return _public(data, path)
    finally:
        os.close(fd)
