"""Bounded execution through a private Linux command-family subreaper."""
from __future__ import annotations

from contextlib import contextmanager
import hashlib
import json
import math
import os
from pathlib import Path
import signal
import subprocess
import sys
import threading
import time
import uuid

from .models import ProductionError
from .store import process_identity


MAX_LOG_BYTES = 64 * 1024 * 1024
SUPERVISOR_CLEANUP_SECONDS = 7


def _children(root_pid: int, known: dict[int, str]) -> dict[int, str]:
    records = {}
    for item in Path("/proc").iterdir():
        if not item.name.isdigit():
            continue
        try:
            fields = (item / "stat").read_text().rsplit(")", 1)[1].split()
            records[int(item.name)] = (int(fields[1]), int(fields[3]), fields[0])
        except (OSError, ValueError, IndexError):
            continue
    # A reaped/reused leader PID must not become a new, unrelated ownership root.
    roots = {pid for pid, identity in known.items() if identity and process_identity(pid) == identity}
    changed = True
    while changed:
        changed = False
        for pid, (parent, session, state) in records.items():
            if pid not in roots and (parent in roots or (root_pid in roots and session == root_pid)):
                roots.add(pid)
                changed = True
    return {pid: process_identity(pid) for pid in roots if pid in records and records[pid][2] != "Z"}


def _belongs_to(pid: int, root_pid: int) -> bool:
    seen = set()
    while pid > 0 and pid not in seen:
        if pid == root_pid:
            return True
        seen.add(pid)
        try:
            fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
            if int(fields[3]) == root_pid:
                return True
            pid = int(fields[1])
        except (OSError, ValueError, IndexError):
            return False
    return False


def _stop_owned(process: subprocess.Popen, known: dict[int, str]):
    """Ask our still-live supervisor to drain and reap its private family."""
    if process.poll() is not None:
        return
    identity = known.get(process.pid)
    if not identity or process_identity(process.pid) != identity:
        raise ProductionError("Cannot prove supervisor ownership for cleanup")
    try:
        os.kill(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        process.wait(timeout=SUPERVISOR_CLEANUP_SECONDS)
    except subprocess.TimeoutExpired as error:
        # A malfunctioning helper must not make the caller wait forever. This
        # signal targets only our original Popen child, never a broad PID list.
        if process_identity(process.pid) == identity:
            os.kill(process.pid, signal.SIGKILL)
        process.wait(timeout=2)
        raise ProductionError("Owned supervisor did not finish command-family cleanup") from error


@contextmanager
def _termination_as_interrupt():
    if threading.current_thread() is not threading.main_thread():
        yield
        return
    previous = signal.getsignal(signal.SIGTERM)

    def interrupted(signum, frame):
        # InterruptedError is an OSError and can be swallowed by /proc readers'
        # ordinary race handling. KeyboardInterrupt always reaches our finally.
        raise KeyboardInterrupt("Coordinator received SIGTERM; stopping owned command family")

    signal.signal(signal.SIGTERM, interrupted)
    try:
        yield
    finally:
        signal.signal(signal.SIGTERM, previous)


def _hash(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run_command(command: list[str], *, cwd: Path, log_path: Path, timeout: float,
                env: dict | None = None, heartbeat=None, guard=None) -> dict:
    if (not isinstance(command, list) or not command or
            not all(isinstance(value, str) and "\x00" not in value for value in command)):
        raise ProductionError("Command must be a nonempty argument list")
    try:
        if isinstance(timeout, bool) or not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("invalid timeout")
    except (TypeError, ValueError, OverflowError) as error:
        raise ProductionError("Command timeout must be positive and finite") from error
    cwd, log_path = Path(cwd), Path(log_path).absolute()
    log_path.parent.mkdir(parents=True, exist_ok=True)
    result_path = log_path.with_name(log_path.name + ".supervisor-" + uuid.uuid4().hex + ".json")
    started = time.monotonic()
    launch = [sys.executable, str(Path(__file__).with_name("supervise.py").resolve()),
              "--log", str(log_path), "--result", str(result_path),
              "--deadline", str(started + timeout), "--max-log-bytes", str(MAX_LOG_BYTES),
              "--parent-pid", str(os.getpid()), "--", *command]
    known = {}
    process = None
    last_heartbeat = 0.0
    with _termination_as_interrupt():
        try:
            with log_path.open("wb") as output:
                process = subprocess.Popen(launch, cwd=cwd, env=env, stdin=subprocess.DEVNULL,
                                           stdout=output, stderr=subprocess.STDOUT, start_new_session=True)
                known[process.pid] = process_identity(process.pid)
                while process.poll() is None:
                    known.update(_children(process.pid, known))
                    now = time.monotonic()
                    if heartbeat and now - last_heartbeat >= 1:
                        heartbeat()
                        last_heartbeat = now
                    if now - started > timeout + SUPERVISOR_CLEANUP_SECONDS:
                        raise ProductionError("Command supervisor exceeded its timeout and cleanup bound")
                    if log_path.stat().st_size > MAX_LOG_BYTES:
                        raise ProductionError("Command output exceeded 64 MiB log size limit")
                    if guard:
                        others = []
                        for job in guard.heavy_jobs():
                            pid = job["pid"]
                            identity = process_identity(pid)
                            if not identity:  # It may have exited since heavy_jobs' snapshot.
                                continue
                            own = pid in known and bool(known[pid]) and identity == known[pid]
                            if not own:
                                try:
                                    # The process may have appeared after our
                                    # snapshot. A proven parent/session chain
                                    # prevents a false contention decision.
                                    prover = getattr(guard, "owned_process", _belongs_to)
                                    own = prover(pid, process.pid)
                                except ProcessLookupError:
                                    continue
                            if not own:
                                others.append(job)
                        if others:
                            raise ProductionError("Another heavy process started; stopping only this job")
                        if guard.available_gib() < 2:
                            raise ProductionError("Available memory below 2 GiB; stopping only this job")
                    time.sleep(.05)
                process.wait()
                if log_path.stat().st_size > MAX_LOG_BYTES:
                    raise ProductionError("Command output exceeded 64 MiB log size limit")
                try:
                    supervision = json.loads(result_path.read_text())
                except (OSError, ValueError) as error:
                    raise ProductionError("Command supervisor exited without a valid cleanup receipt") from error
                expected_hash = hashlib.sha256(json.dumps(command).encode()).hexdigest()
                if (supervision.get("schema_version") != 1 or
                        supervision.get("supervisor_pid") != process.pid or
                        supervision.get("command_sha256") != expected_hash):
                    raise ProductionError("Command supervisor receipt does not match this launch")
                if supervision.get("stop_reason") or supervision.get("remaining"):
                    raise ProductionError(supervision.get("stop_reason") or "Owned command descendants remain")
                if process.returncode or type(supervision.get("returncode")) is not int:
                    raise ProductionError("Command supervisor failed or omitted the actual exit code")
        finally:
            if process is not None and process.poll() is None:
                _stop_owned(process, known)
    return {"command": command, "returncode": supervision["returncode"],
            "wall_seconds": round(time.monotonic() - started, 3),
            "log": str(log_path), "log_sha256": _hash(log_path),
            "supervision": {**supervision, "receipt": str(result_path)}}
