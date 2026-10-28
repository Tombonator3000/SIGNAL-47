#!/usr/bin/env python3
"""Private Linux subreaper for one command family; no project imports required.

The supervisor stays alive until all descendants are reaped. This covers short
wrappers, double-forks and setsid children that disappear between /proc polls.
Signals are sent only to descendants of this still-live supervisor, with Linux
start-time fencing. A task cannot pass if its leader leaves background children.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time


TERM_GRACE = 0.35
CLEANUP_LIMIT = 5.0
POLL_SECONDS = 0.02


def process_records():
    records = {}
    for directory in Path("/proc").iterdir():
        if not directory.name.isdigit():
            continue
        try:
            fields = (directory / "stat").read_text().rsplit(")", 1)[1].split()
            records[int(directory.name)] = {
                "parent": int(fields[1]), "session": int(fields[3]),
                "state": fields[0], "start": fields[19],
            }
        except (OSError, ValueError, IndexError):
            continue
    return records


def descendants(records):
    roots = {os.getpid()}
    changed = True
    while changed:
        changed = False
        for pid, record in records.items():
            if pid not in roots and record["parent"] in roots:
                roots.add(pid)
                changed = True
    return {pid: records[pid] for pid in roots if pid != os.getpid() and pid in records}


def signal_descendants(records, signum):
    """Fence every PID immediately before signalling; never use a broad kill."""
    for pid, record in records.items():
        if record["state"] == "Z":
            continue
        try:
            current = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
            if current[19] == record["start"]:
                os.kill(pid, signum)
        except (ProcessLookupError, FileNotFoundError):
            pass


def setup_subreaper(parent_pid):
    if not sys.platform.startswith("linux"):
        raise RuntimeError("Production command supervision requires Linux")
    libc = ctypes.CDLL(None, use_errno=True)
    # These policies affect this private helper only, never the caller or its
    # other children. Request parent-death cleanup before any tool is started.
    for option, value in ((36, 1), (1, signal.SIGTERM)):  # CHILD_SUBREAPER, PDEATHSIG
        if libc.prctl(option, value, 0, 0, 0) != 0:
            raise OSError(ctypes.get_errno(), "Cannot enable Linux command supervision")
    if os.getppid() != parent_pid:
        raise RuntimeError("Coordinator exited before supervisor setup; command not started")
    if os.getsid(0) != os.getpid():
        raise RuntimeError("Supervisor requires its own private process session")


def reap(leader):
    """Return whether children remain and preserve the real leader exit code."""
    reaped = 0
    while True:
        try:
            pid, status = os.waitpid(-1, os.WNOHANG)
        except ChildProcessError:
            return False, reaped
        if not pid:
            return True, reaped
        if pid == leader.pid:
            leader.returncode = os.waitstatus_to_exitcode(status)
        reaped += 1


def write_result(path, result):
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("x", encoding="utf-8") as stream:
        json.dump(result, stream, sort_keys=True, allow_nan=False)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def supervise(command, *, log_path, result_path, deadline, max_log_bytes, parent_pid):
    started = time.monotonic()
    requested = []

    def interrupted(signum, frame):
        requested.append(signum)

    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    result = {"schema_version": 1, "supervisor_pid": os.getpid(),
              "command_sha256": hashlib.sha256(json.dumps(command).encode()).hexdigest(),
              "returncode": None, "stop_reason": None, "reaped": 0,
              "descendants_seen": 0, "remaining": []}
    leader = None
    seen = set()
    stop_reason = None
    cleanup_started = None
    try:
        setup_subreaper(parent_pid)
        if requested or time.monotonic() >= deadline:
            raise RuntimeError("Command timeout or interrupt before launch")
        leader = subprocess.Popen(command, stdin=subprocess.DEVNULL)
        while True:
            children_remain, reaped_count = reap(leader)
            result["reaped"] += reaped_count
            owned = descendants(process_records())
            seen.update((pid, value["start"]) for pid, value in owned.items())
            now = time.monotonic()
            if requested and stop_reason is None:
                stop_reason = f"Command interrupted by signal {requested[0]}"
            if now >= deadline and stop_reason is None:
                stop_reason = "Command timeout exceeded"
            if log_path.stat().st_size > max_log_bytes and stop_reason is None:
                stop_reason = "Command output exceeded log size limit"
            if leader.returncode is not None and children_remain and stop_reason is None:
                stop_reason = "Command left background descendants; task cannot pass"
            if not children_remain and leader.returncode is not None:
                break
            if stop_reason:
                if cleanup_started is None:
                    cleanup_started = now
                elapsed = now - cleanup_started
                # Re-discover every pass, including children created inside a
                # SIGTERM handler and children adopted after a double-fork.
                signal_descendants(owned, signal.SIGTERM if elapsed < TERM_GRACE else signal.SIGKILL)
                if elapsed >= CLEANUP_LIMIT:
                    result["remaining"] = sorted(owned)
                    stop_reason += "; owned processes did not reap within cleanup limit"
                    break
            time.sleep(POLL_SECONDS if stop_reason else min(POLL_SECONDS, max(0.0, deadline - now)))
    except BaseException as error:
        stop_reason = f"{type(error).__name__}: {error}"
        if leader is not None:
            cleanup_started = time.monotonic()
            while True:
                children_remain, count = reap(leader)
                result["reaped"] += count
                if not children_remain:
                    break
                owned = descendants(process_records())
                seen.update((pid, value["start"]) for pid, value in owned.items())
                elapsed = time.monotonic() - cleanup_started
                signal_descendants(owned, signal.SIGTERM if elapsed < TERM_GRACE else signal.SIGKILL)
                if elapsed >= CLEANUP_LIMIT:
                    result["remaining"] = sorted(owned)
                    break
                time.sleep(POLL_SECONDS)
    # Final checks also cover a child which fills a sparse log and exits before
    # any polling interval. Exit 0 alone never establishes valid evidence.
    try:
        if log_path.stat().st_size > max_log_bytes:
            stop_reason = stop_reason or "Command output exceeded log size limit"
    except OSError as error:
        stop_reason = stop_reason or f"Cannot inspect command log: {error}"
    if time.monotonic() >= deadline:
        stop_reason = stop_reason or "Command timeout exceeded"
    result.update(returncode=leader.returncode if leader is not None else None,
                  stop_reason=stop_reason, descendants_seen=len(seen),
                  wall_seconds=round(time.monotonic() - started, 6))
    write_result(result_path, result)
    return 0 if stop_reason is None else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--result", type=Path, required=True)
    parser.add_argument("--deadline", type=float, required=True)
    parser.add_argument("--max-log-bytes", type=int, required=True)
    parser.add_argument("--parent-pid", type=int, required=True)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or args.max_log_bytes <= 0 or args.parent_pid <= 0:
        parser.error("Command, positive log bound and valid parent PID are required")
    return supervise(command, log_path=args.log, result_path=args.result,
                     deadline=args.deadline, max_log_bytes=args.max_log_bytes,
                     parent_pid=args.parent_pid)


if __name__ == "__main__":
    raise SystemExit(main())
