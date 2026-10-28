"""Claude Code provider for the production coordinator.

Runs the locally installed ``claude`` CLI once in print mode with a read-only
tool surface and returns a structurally validated change proposal. The CLI
never writes files or launches commands here; the parent runner applies the
proposal after checking allowed paths and prior hashes.

Process handling: the CLI runs in its own session. The session leader is not
reaped until every remaining member of its process group and session has been
stopped, so its pid (and therefore the group and session ids) cannot be reused
by an unrelated process while signals are sent. Processes outside that session
are never signalled. A child that deliberately starts a new session escapes
this sweep; the CLI has no tools that could do so during ``generate``.
"""

from __future__ import annotations

import datetime as _dt
import hashlib
import json
import math
import os
import re
import shutil
import signal
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Any, Callable

__all__ = [
    "ClaudeClient",
    "ClaudeError",
    "MAX_PROPOSAL_BYTES",
    "MAX_PROPOSAL_CHANGES",
    "PROPOSAL_SCHEMA",
    "READONLY_TOOLS",
    "validate_proposal",
]

MAX_PROPOSAL_BYTES = 2 * 1024 * 1024
MAX_PROPOSAL_CHANGES = 100
READONLY_TOOLS = ("Read", "Glob", "Grep")
EMPTY_MCP_CONFIG = '{"mcpServers": {}}'

PROPOSAL_SCHEMA: dict[str, Any] = {
    "type": "object",
    "additionalProperties": False,
    "required": ["summary", "notes", "changes"],
    "properties": {
        "summary": {"type": "string", "minLength": 1},
        "notes": {"type": "string"},
        "changes": {
            "type": "array",
            "maxItems": MAX_PROPOSAL_CHANGES,
            "items": {
                "type": "object",
                "additionalProperties": False,
                "required": ["path", "content", "base_sha256"],
                "properties": {
                    "path": {"type": "string", "minLength": 1},
                    "content": {"type": "string"},
                    "base_sha256": {
                        "anyOf": [
                            {"type": "string", "pattern": "^[0-9a-f]{64}$"},
                            {"type": "null"},
                        ]
                    },
                },
            },
        },
    },
}

_PROPOSAL_KEYS = frozenset(("summary", "notes", "changes"))
_CHANGE_KEYS = frozenset(("path", "content", "base_sha256"))
_SHA256_RE = re.compile(r"[0-9a-f]{64}")
_DRIVE_RE = re.compile(r"[A-Za-z]:")
_FENCE_RE = re.compile(r"\A```[A-Za-z0-9_-]*\s*\n(.*)\n\s*```\Z", re.DOTALL)


class ClaudeError(RuntimeError):
    """Claude Code did not produce a usable, verified proposal.

    ``cost_usd`` and ``session_id`` are set when the CLI reported them, so the
    caller can account for spend even on failure.
    """

    def __init__(self, message: str, *, cost_usd: float | None = None,
                 session_id: str | None = None, retry_after_seconds: float | None = None) -> None:
        super().__init__(message)
        self.cost_usd = cost_usd
        self.session_id = session_id
        self.retry_after_seconds = retry_after_seconds


# Proposal validation

def _fail(message: str) -> None:
    raise ClaudeError(f"invalid proposal: {message}")


def _utf8_len(value: str, what: str) -> int:
    try:
        return len(value.encode("utf-8"))
    except UnicodeEncodeError:
        _fail(f"{what} is not valid UTF-8 text")
    return 0  # unreachable


def _check_path(path: Any, index: int) -> str:
    where = f"changes[{index}].path"
    if not isinstance(path, str) or not path:
        _fail(f"{where} must be a nonempty string")
    if len(path) > 4096:
        _fail(f"{where} is too long")
    if any(ord(ch) < 32 or ord(ch) == 127 for ch in path):
        _fail(f"{where} contains control characters")
    if "\\" in path:
        _fail(f"{where} must use POSIX separators")
    if path.startswith("/") or _DRIVE_RE.match(path):
        _fail(f"{where} must be relative")
    if any(part in ("", ".", "..") for part in path.split("/")):
        _fail(f"{where} must be a normalized relative path without '.', '..' or empty parts")
    return path


def validate_proposal(proposal: Any) -> dict[str, Any]:
    """Return a clean copy of ``proposal`` or raise ClaudeError."""
    if not isinstance(proposal, dict):
        _fail("top level must be an object")
    keys = set(proposal)
    if keys != _PROPOSAL_KEYS:
        extra = sorted(map(str, keys - _PROPOSAL_KEYS))
        missing = sorted(_PROPOSAL_KEYS - keys)
        _fail(f"fields must be exactly summary, notes, changes (extra={extra}, missing={missing})")
    summary, notes, changes = proposal["summary"], proposal["notes"], proposal["changes"]
    if not isinstance(summary, str) or not summary.strip():
        _fail("summary must be a nonempty string")
    if not isinstance(notes, str):
        _fail("notes must be a string")
    if not isinstance(changes, list):
        _fail("changes must be a list")
    if len(changes) > MAX_PROPOSAL_CHANGES:
        _fail(f"at most {MAX_PROPOSAL_CHANGES} changes are allowed, got {len(changes)}")

    total = _utf8_len(summary, "summary") + _utf8_len(notes, "notes")
    if total > MAX_PROPOSAL_BYTES:
        _fail(f"aggregate output exceeds {MAX_PROPOSAL_BYTES} bytes")
    seen: set[str] = set()
    clean_changes = []
    for index, change in enumerate(changes):
        if not isinstance(change, dict):
            _fail(f"changes[{index}] must be an object")
        ckeys = set(change)
        if ckeys != _CHANGE_KEYS:
            extra = sorted(map(str, ckeys - _CHANGE_KEYS))
            missing = sorted(_CHANGE_KEYS - ckeys)
            _fail(f"changes[{index}] fields must be exactly path, content, base_sha256 "
                  f"(extra={extra}, missing={missing})")
        path = _check_path(change["path"], index)
        if path in seen:
            _fail(f"duplicate path {path!r}")
        seen.add(path)
        content = change["content"]
        if not isinstance(content, str):
            _fail(f"changes[{index}].content must be a string")
        base = change["base_sha256"]
        if base is not None and (not isinstance(base, str) or not _SHA256_RE.fullmatch(base)):
            _fail(f"changes[{index}].base_sha256 must be 64 lowercase hex characters or null")
        total += _utf8_len(path, f"changes[{index}].path")
        total += _utf8_len(content, f"changes[{index}].content")
        if total > MAX_PROPOSAL_BYTES:
            _fail(f"aggregate output exceeds {MAX_PROPOSAL_BYTES} bytes")
        clean_changes.append({"path": path, "content": content, "base_sha256": base})
    return {"summary": summary, "notes": notes, "changes": clean_changes}


# JSON helpers

def _strict_loads(text: str) -> Any:
    def pairs(items: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in items:
            if key in result:
                raise ValueError(f"duplicate key {key!r}")
            result[key] = value
        return result

    def constant(name: str) -> Any:
        raise ValueError(f"non-finite number {name}")

    return json.loads(text, object_pairs_hook=pairs, parse_constant=constant)


def _finite_float(value: Any) -> float | None:
    """Return ``value`` as a finite float, or None for bools, non-numbers,
    NaN, infinities and integers too large for a float."""
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    try:
        number = float(value)
    except OverflowError:
        return None
    return number if math.isfinite(number) else None


def _now() -> str:
    return _dt.datetime.now(_dt.timezone.utc).isoformat(timespec="milliseconds")


def _write_json(path: Path, data: Any) -> None:
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(tmp, path)


# Process session handling (Linux /proc and pidfd)

def _proc_stat(pid: int) -> tuple[str, int, int] | None:
    """Return (state, pgrp, session) for ``pid`` or None if it is gone."""
    try:
        with open(f"/proc/{pid}/stat", "rb") as fh:
            raw = fh.read()
    except OSError:
        return None
    fields = raw[raw.rfind(b")") + 2:].split()
    try:
        return fields[0].decode("ascii", "replace"), int(fields[2]), int(fields[3])
    except (IndexError, ValueError):
        return None


def _session_members(leader: int) -> list[int]:
    """Live processes, other than the leader, in the leader's group or session."""
    members = []
    me = os.getpid()
    try:
        names = os.listdir("/proc")
    except OSError:
        return members
    for name in names:
        if not name.isdigit():
            continue
        pid = int(name)
        if pid in (leader, me):
            continue
        stat = _proc_stat(pid)
        if stat and stat[0] != "Z" and leader in (stat[1], stat[2]):
            members.append(pid)
    return members


def _signal_member(pid: int, leader: int, sig: int) -> bool:
    """Signal ``pid`` through a pidfd after rechecking that it belongs to ``leader``."""
    pidfd_open = getattr(os, "pidfd_open", None)
    send = getattr(signal, "pidfd_send_signal", None)
    if pidfd_open is None or send is None:
        return False
    try:
        fd = pidfd_open(pid)
    except OSError:
        return False
    try:
        stat = _proc_stat(pid)
        if not stat or leader not in (stat[1], stat[2]):
            return False
        send(fd, sig)
        return True
    except OSError:
        return False
    finally:
        os.close(fd)


def _leader_exited(pid: int) -> bool:
    """True once the leader has exited. Does not reap it (WNOWAIT)."""
    try:
        info = os.waitid(os.P_PID, pid, os.WEXITED | os.WNOHANG | os.WNOWAIT)
    except ChildProcessError:
        return True
    return info is not None and info.si_pid != 0


class ClaudeClient:
    """Run Claude Code in read-only print mode and return a validated proposal."""

    heartbeat_interval = 10.0
    poll_interval = 0.05
    kill_grace = 3.0
    reap_timeout = 10.0
    max_stdout_bytes = 16 * 1024 * 1024
    max_stderr_bytes = 4 * 1024 * 1024
    stderr_tail_bytes = 2000

    def __init__(self, executable: str = "claude") -> None:
        self.executable = executable

    def build_command(self, executable: str, budget_usd: float) -> list[str]:
        budget = f"{budget_usd:.6f}".rstrip("0").rstrip(".")
        tools = ",".join(READONLY_TOOLS)
        return [
            executable,
            "-p",
            "--output-format", "json",
            "--json-schema", json.dumps(PROPOSAL_SCHEMA, separators=(",", ":"), sort_keys=True),
            "--max-budget-usd", budget,
            "--tools", tools,
            "--allowedTools", tools,
            "--permission-mode", "dontAsk",
            "--mcp-config", EMPTY_MCP_CONFIG,
            "--permission-prompts", "none",
            "--strict-mcp-config",
            "--safe-mode",
            "--disable-slash-commands",
        ]

    def generate(self, prompt: str, *, cwd: Path, output_dir: Path,
                 budget_usd: float, timeout: float,
                 heartbeat: Callable[[], Any] | None = None) -> dict[str, Any]:
        """Run one read-only Claude session and return the validated proposal.

        ``heartbeat`` is called without arguments every ``heartbeat_interval``
        seconds while the process runs. Any exit stops the session first.
        KeyboardInterrupt, other BaseExceptions and InterruptedError propagate
        unchanged; every other failure is raised as ClaudeError.
        """
        if not isinstance(prompt, str) or not prompt.strip():
            raise ClaudeError("prompt must be a nonempty string")
        budget = _finite_float(budget_usd)
        if budget is None or budget <= 0:
            raise ClaudeError("budget_usd must be a finite positive number")
        if f"{budget:.6f}".rstrip("0").rstrip(".") in ("0", ""):
            raise ClaudeError("budget_usd is too small")
        seconds = _finite_float(timeout)
        if seconds is None or seconds <= 0:
            raise ClaudeError("timeout must be a finite positive number")
        if heartbeat is not None and not callable(heartbeat):
            raise ClaudeError("heartbeat must be callable")
        cwd = Path(cwd)
        if not cwd.is_dir():
            raise ClaudeError(f"cwd is not a directory: {cwd}")
        resolved = shutil.which(self.executable)
        if resolved is None:
            raise ClaudeError(f"Claude executable not found: {self.executable}")
        resolved = os.path.abspath(resolved)
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        try:
            prompt_bytes = prompt.encode("utf-8")
        except UnicodeEncodeError as exc:
            raise ClaudeError("prompt is not valid UTF-8 text") from exc
        command = self.build_command(resolved, budget)
        meta_path = output_dir / "request.json"
        meta: dict[str, Any] = {
            "provider": "claude-code",
            "command": command,
            "cwd": str(cwd.resolve()),
            "budget_usd": budget,
            "timeout_s": seconds,
            "prompt_sha256": hashlib.sha256(prompt_bytes).hexdigest(),
            "prompt_bytes": len(prompt_bytes),
            "started_at": _now(),
            "status": "running",
        }
        _write_json(meta_path, meta)

        started = time.monotonic()
        outcome: dict[str, Any] = {}
        try:
            try:
                self._run(command, prompt_bytes, cwd, output_dir, seconds,
                          heartbeat, outcome)
                result = self._parse(output_dir / "stdout.json", output_dir / "stderr.log",
                                     outcome["returncode"], budget)
            except (ClaudeError, InterruptedError):
                raise
            except Exception as exc:
                # A failing heartbeat or I/O error after launch is still a
                # provider failure with unknown spend, never a free retry.
                raise ClaudeError(f"Claude provider failed: {type(exc).__name__}: {exc}") from exc
        except BaseException as exc:
            meta["status"] = "error" if isinstance(exc, ClaudeError) else "interrupted"
            meta["error"] = f"{type(exc).__name__}: {exc}"
            if isinstance(exc, ClaudeError):
                meta["cost_usd"] = exc.cost_usd
                meta["session_id"] = exc.session_id
            raise
        else:
            meta["status"] = "ok"
            meta["cost_usd"] = result["cost_usd"]
            meta["session_id"] = result["session_id"]
            result["command"] = list(command)
            return result
        finally:
            meta.update(outcome)
            meta["finished_at"] = _now()
            meta["duration_s"] = round(time.monotonic() - started, 3)
            try:
                _write_json(meta_path, meta)
            except OSError:
                pass

    # Subprocess

    def _run(self, command: list[str], prompt_bytes: bytes, cwd: Path,
             output_dir: Path, timeout: float, heartbeat: Callable[[], Any] | None,
             outcome: dict[str, Any]) -> None:
        outcome.update(returncode=None, timed_out=False, session_members_signalled=0)
        with tempfile.TemporaryFile() as stdin_fh, \
                open(output_dir / "stdout.json", "wb") as stdout_fh, \
                open(output_dir / "stderr.log", "wb") as stderr_fh:
            stdin_fh.write(prompt_bytes)
            stdin_fh.flush()
            stdin_fh.seek(0)
            try:
                proc = subprocess.Popen(
                    command, stdin=stdin_fh, stdout=stdout_fh, stderr=stderr_fh,
                    cwd=str(cwd), shell=False, close_fds=True, start_new_session=True,
                )
            except OSError as exc:
                raise ClaudeError(f"could not start Claude: {exc}") from exc
            outcome["pid"] = proc.pid
            try:
                deadline = time.monotonic() + timeout
                next_beat = time.monotonic() + self.heartbeat_interval
                while not _leader_exited(proc.pid):
                    now = time.monotonic()
                    if now >= deadline:
                        outcome["timed_out"] = True
                        raise ClaudeError(f"Claude timed out after {timeout:g} s")
                    self._check_sizes(os.fstat(stdout_fh.fileno()).st_size,
                                      os.fstat(stderr_fh.fileno()).st_size)
                    if heartbeat is not None and now >= next_beat:
                        heartbeat()
                        next_beat = time.monotonic() + self.heartbeat_interval
                    time.sleep(max(0.0, min(self.poll_interval, deadline - now)))
            finally:
                outcome["session_members_signalled"] = self._stop_session(proc)
                outcome["returncode"] = proc.returncode

    def _stop_session(self, proc: Any) -> int:
        """Stop everything left in the session, then reap the leader."""
        leader = proc.pid
        signalled: set[int] = set()
        for sig in (signal.SIGTERM, signal.SIGKILL):
            leader_done = _leader_exited(leader)
            members = _session_members(leader)
            if leader_done and not members:
                break
            # The unreaped leader reserves its pid as group id: killpg is safe.
            try:
                os.killpg(leader, sig)
            except (ProcessLookupError, PermissionError):
                pass
            for pid in members:
                if _signal_member(pid, leader, sig):
                    signalled.add(pid)
            end = time.monotonic() + self.kill_grace
            while time.monotonic() < end:
                if _leader_exited(leader) and not _session_members(leader):
                    break
                time.sleep(0.02)
        try:
            proc.wait(timeout=self.reap_timeout)
        except subprocess.TimeoutExpired as exc:
            raise ClaudeError(f"Claude process {leader} could not be reaped") from exc
        return len(signalled)

    def _check_sizes(self, stdout_size: int, stderr_size: int) -> None:
        if stdout_size > self.max_stdout_bytes:
            raise ClaudeError(f"Claude stdout exceeded {self.max_stdout_bytes} bytes")
        if stderr_size > self.max_stderr_bytes:
            raise ClaudeError(f"Claude stderr exceeded {self.max_stderr_bytes} bytes")

    # Output

    def _parse(self, stdout_path: Path, stderr_path: Path, returncode: int | None,
               budget_usd: float) -> dict[str, Any]:
        # Final bounds: output can grow between poll ticks or after the leader
        # exits, so never trust the wait loop alone.
        with open(stdout_path, "rb") as fh:
            if os.fstat(fh.fileno()).st_size > self.max_stdout_bytes:
                raise ClaudeError(f"Claude stdout exceeded {self.max_stdout_bytes} bytes")
            raw = fh.read(self.max_stdout_bytes + 1)
        if len(raw) > self.max_stdout_bytes:
            raise ClaudeError(f"Claude stdout exceeded {self.max_stdout_bytes} bytes")
        with open(stderr_path, "rb") as fh:
            stderr_size = os.fstat(fh.fileno()).st_size
            fh.seek(max(0, stderr_size - self.stderr_tail_bytes))
            tail = fh.read(self.stderr_tail_bytes)
        stderr_tail = tail.decode("utf-8", "replace").strip()
        detail = f" (exit {returncode}; stderr: {stderr_tail or 'empty'})"
        stderr_oversize = stderr_size > self.max_stderr_bytes
        oversize_message = f"Claude stderr exceeded {self.max_stderr_bytes} bytes"

        data: Any = None
        parse_error = None
        try:
            data = _strict_loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, ValueError) as exc:
            parse_error = exc
        if isinstance(data, list):
            results = [m for m in data if isinstance(m, dict) and m.get("type") == "result"]
            data = results[-1] if results else None
        if not isinstance(data, dict) or data.get("type") != "result":
            if stderr_oversize:
                raise ClaudeError(oversize_message + detail)
            if returncode != 0:
                raise ClaudeError(f"Claude failed without a result{detail}")
            why = f": {parse_error}" if parse_error else ""
            raise ClaudeError(f"Claude produced no JSON result object{why}{detail}")

        session_id = data.get("session_id")
        session_id = session_id if isinstance(session_id, str) and session_id else None
        reported = data.get("total_cost_usd")
        cost = _finite_float(reported)
        if cost is None or cost < 0:
            shown = type(reported).__name__ if isinstance(reported, int) else repr(reported)[:100]
            raise ClaudeError(f"Claude result has no valid total_cost_usd: {shown}{detail}",
                              session_id=session_id)

        def fail(message: str) -> None:
            raise ClaudeError(message + detail, cost_usd=cost, session_id=session_id)

        if stderr_oversize:
            fail(oversize_message)
        subtype = data.get("subtype")
        # The real CLI reports quota errors with exit 1 AND subtype success.
        # Classify availability before the generic exit-code error.
        text = str(data.get("result", ""))
        lowered = text.lower()
        if data.get("is_error") is not False and ("rate_limit" in lowered or "hit your" in lowered and "limit" in lowered):
            raise ClaudeError("Claude quota unavailable: " + text[:500], cost_usd=cost,
                              session_id=session_id, retry_after_seconds=900)
        if returncode != 0:
            fail(f"Claude exited with status {returncode} (subtype {subtype!r})")
        if data.get("is_error") is not False:
            fail(f"Claude reported an error (is_error={data.get('is_error')!r}, "
                 f"subtype {subtype!r}): {str(data.get('result', ''))[:500]}")
        if subtype != "success":
            fail(f"Claude result subtype is {subtype!r}, not 'success'")
        denials = data.get("permission_denials")
        if not isinstance(denials, list):
            fail("Claude result has no permission_denials list")
        if denials:
            names = sorted({str(d.get("tool_name")) for d in denials if isinstance(d, dict)})
            fail(f"Claude hit {len(denials)} permission denial(s): {', '.join(names)}")
        if cost > budget_usd + 1e-9:
            fail(f"Claude cost {cost} exceeds budget {budget_usd}")
        if session_id is None:
            fail("Claude result has no session_id")
        usage = data.get("usage")
        if not isinstance(usage, dict):
            fail("Claude result has no usage object")

        structured = data.get("structured_output")
        if structured is not None:
            if not isinstance(structured, dict):
                fail("Claude structured_output is not an object")
            candidate = structured
        else:
            text = data.get("result")
            if not isinstance(text, str) or not text.strip():
                fail("Claude result has neither structured_output nor result text")
            text = text.strip()
            fenced = _FENCE_RE.match(text)
            if fenced:
                text = fenced.group(1).strip()
            try:
                candidate = _strict_loads(text)
            except ValueError as exc:
                fail(f"Claude result text is not a JSON proposal: {exc}")
        try:
            proposal = validate_proposal(candidate)
        except ClaudeError as exc:
            fail(str(exc))
        return {"proposal": proposal, "cost_usd": cost, "session_id": session_id,
                "usage": usage}
