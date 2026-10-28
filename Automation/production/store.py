"""Durable, transactional job queue with fenced attempts and workspace ownership."""
from __future__ import annotations

from contextlib import contextmanager
import json
import math
import os
from pathlib import Path
import sqlite3
import time
import uuid

from .models import ProductionError, Task


def process_identity(pid: int | None = None) -> str:
    pid = pid or os.getpid()
    try:
        fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
        if fields[0] == "Z":
            return ""
        return Path("/proc/sys/kernel/random/boot_id").read_text().strip() + ":" + fields[19]
    except (OSError, IndexError):
        return ""


def owner_alive(pid: int | None, identity: str | None) -> bool:
    return bool(pid and identity and process_identity(pid) == identity)


class Store:
    def __init__(self, path: Path):
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.connection() as db:
            db.execute("PRAGMA journal_mode=WAL")
            db.executescript("""
                CREATE TABLE IF NOT EXISTS tasks (
                    id TEXT PRIMARY KEY, workspace TEXT NOT NULL, spec TEXT NOT NULL,
                    status TEXT NOT NULL, attempts INTEGER NOT NULL DEFAULT 0,
                    cost REAL NOT NULL DEFAULT 0, next_run REAL NOT NULL DEFAULT 0,
                    owner_pid INTEGER, owner_identity TEXT, run_id TEXT,
                    created REAL NOT NULL, updated REAL NOT NULL, error TEXT,
                    receipt TEXT
                );
                CREATE TABLE IF NOT EXISTS attempts (
                    run_id TEXT PRIMARY KEY, task_id TEXT NOT NULL,
                    number INTEGER NOT NULL, started REAL NOT NULL, ended REAL,
                    status TEXT NOT NULL, receipt TEXT, error TEXT
                );
                CREATE TABLE IF NOT EXISTS events (
                    sequence INTEGER PRIMARY KEY AUTOINCREMENT, at REAL NOT NULL,
                    task_id TEXT, kind TEXT NOT NULL, details TEXT NOT NULL
                );
                CREATE TABLE IF NOT EXISTS workspace_locks (
                    workspace TEXT PRIMARY KEY, task_id TEXT NOT NULL,
                    run_id TEXT NOT NULL
                );
            """)

    @contextmanager
    def connection(self):
        db = sqlite3.connect(self.path, timeout=10, isolation_level=None)
        db.row_factory = sqlite3.Row
        try:
            yield db
        finally:
            db.close()

    @contextmanager
    def transaction(self):
        with self.connection() as db:
            db.execute("BEGIN IMMEDIATE")
            try:
                yield db
                db.commit()
            except BaseException:
                db.rollback()
                raise

    @staticmethod
    def _event(db, task_id, kind, details):
        db.execute("INSERT INTO events(at,task_id,kind,details) VALUES(?,?,?,?)",
                   (time.time(), task_id, kind, json.dumps(details, sort_keys=True)))

    def enqueue(self, task: Task, workspace: Path) -> dict:
        task = Task.from_dict(task.to_dict())
        workspace = str(Path(workspace).resolve())
        spec = json.dumps(task.to_dict(), sort_keys=True)
        with self.transaction() as db:
            previous = db.execute("SELECT * FROM tasks WHERE id=?", (task.id,)).fetchone()
            if previous:
                if previous["spec"] == spec and previous["workspace"] == workspace:
                    result = dict(previous)
                    result["spec"] = json.loads(result["spec"])
                    return result
                raise ProductionError(f"Task {task.id} already exists with another contract or workspace")
            graph = {r["id"]: json.loads(r["spec"])["depends_on"]
                     for r in db.execute("SELECT id,spec FROM tasks")}
            graph[task.id] = task.depends_on
            def visit(node, active, seen):
                if node in active:
                    raise ProductionError("Dependency cycle at " + node)
                if node in seen:
                    return
                active.add(node)
                for dependency in graph.get(node, []):
                    visit(dependency, active, seen)
                active.remove(node)
                seen.add(node)
            seen = set()
            for node in graph:
                visit(node, set(), seen)
            now = time.time()
            db.execute("INSERT INTO tasks(id,workspace,spec,status,created,updated) VALUES(?,?,?,'queued',?,?)",
                       (task.id, workspace, spec, now, now))
            self._event(db, task.id, "enqueued", {"fingerprint": task.fingerprint()})
        return self.get(task.id)

    def get(self, task_id: str) -> dict:
        with self.connection() as db:
            row = db.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
            if row is None:
                raise ProductionError("Unknown task " + task_id)
            value = dict(row)
            value["spec"] = json.loads(value["spec"])
            return value

    def list(self) -> list[dict]:
        with self.connection() as db:
            rows = [dict(r) for r in db.execute("SELECT * FROM tasks ORDER BY created,id")]
        for row in rows:
            row["spec"] = json.loads(row["spec"])
        return rows

    def claim(self, *, task_id: str | None = None, excluded=()) -> dict | None:
        with self.transaction() as db:
            rows = db.execute("SELECT * FROM tasks WHERE status IN ('queued','blocked') AND next_run<=? ORDER BY created,id",
                              (time.time(),)).fetchall()
            statuses = {r["id"]: r["status"] for r in db.execute("SELECT id,status FROM tasks")}
            for row in rows:
                if row["id"] in excluded or task_id and row["id"] != task_id:
                    continue
                task = Task.from_dict(json.loads(row["spec"]))
                pending = [x for x in task.depends_on if statuses.get(x) != "passed"]
                reason = "Waiting for dependencies: " + ", ".join(pending) if pending else None
                if row["attempts"] >= task.max_attempts or row["cost"] >= task.budget_usd:
                    db.execute("UPDATE tasks SET status='failed',error=?,updated=? WHERE id=?",
                               ("Attempt or model budget exhausted", time.time(), task.id))
                    continue
                if not reason and db.execute("SELECT 1 FROM workspace_locks WHERE workspace=?", (row["workspace"],)).fetchone():
                    reason = "Workspace is reserved by another running task"
                if reason:
                    db.execute("UPDATE tasks SET status='blocked',error=?,updated=? WHERE id=?",
                               (reason, time.time(), task.id))
                    continue
                run_id = uuid.uuid4().hex
                now = time.time()
                db.execute("UPDATE tasks SET status='running',attempts=attempts+1,owner_pid=?,owner_identity=?,run_id=?,updated=?,error=NULL WHERE id=?",
                           (os.getpid(), process_identity(), run_id, now, task.id))
                db.execute("INSERT INTO workspace_locks(workspace,task_id,run_id) VALUES(?,?,?)",
                           (row["workspace"], task.id, run_id))
                db.execute("INSERT INTO attempts(run_id,task_id,number,started,status) VALUES(?,?,?,?,'running')",
                           (run_id, task.id, row["attempts"] + 1, now))
                self._event(db, task.id, "claimed", {"run_id": run_id, "attempt": row["attempts"] + 1})
                result = dict(row)
                result.update(status="running", attempts=row["attempts"] + 1, run_id=run_id,
                              spec=task.to_dict())
                return result
        return None

    def heartbeat(self, task_id: str, run_id: str):
        with self.transaction() as db:
            result = db.execute("UPDATE tasks SET updated=? WHERE id=? AND run_id=? AND status='running'",
                                (time.time(), task_id, run_id))
            if result.rowcount != 1:
                raise ProductionError("Lost task ownership; stopping this attempt")

    def bind_receipt(self, task_id: str, run_id: str, receipt: Path):
        """Make the in-progress receipt discoverable even after a hard crash."""
        with self.transaction() as db:
            changed = db.execute("UPDATE tasks SET receipt=? WHERE id=? AND run_id=? AND status='running'",
                                 (str(receipt), task_id, run_id))
            if changed.rowcount != 1:
                raise ProductionError("Stale attempt cannot attach a receipt")
            db.execute("UPDATE attempts SET receipt=? WHERE run_id=?", (str(receipt), run_id))

    def finish(self, task_id: str, run_id: str, status: str, *, cost=0.0, receipt=None, error=None, defer=False,
               retry_after_seconds=3.0):
        if status not in ("passed", "review", "failed", "queued", "blocked"):
            raise ProductionError("Invalid final attempt status")
        if isinstance(cost, bool) or not isinstance(cost, (int, float)) or not 0 <= cost <= 1_000_000 or not math.isfinite(cost):
            raise ProductionError("Attempt cost must be finite and nonnegative")
        if type(defer) is not bool or defer and (status != "blocked" or cost != 0):
            raise ProductionError("Only an unstarted blocked attempt can be deferred")
        if isinstance(retry_after_seconds, bool) or not isinstance(retry_after_seconds, (int, float)) or not 0 <= retry_after_seconds <= 86400:
            raise ProductionError("Invalid retry delay")
        with self.transaction() as db:
            row = db.execute("SELECT * FROM tasks WHERE id=? AND run_id=? AND status='running'", (task_id, run_id)).fetchone()
            if row is None:
                raise ProductionError("Stale attempt cannot finalize a task")
            now = time.time()
            db.execute("UPDATE tasks SET status=?,cost=cost+?,receipt=?,error=?,owner_pid=NULL,owner_identity=NULL,updated=?,next_run=?,attempts=attempts-? WHERE id=?",
                       (status, cost, str(receipt) if receipt else None, error, now,
                        now + retry_after_seconds if status in ("queued", "blocked") else 0, int(defer), task_id))
            db.execute("DELETE FROM workspace_locks WHERE run_id=?", (run_id,))
            db.execute("UPDATE attempts SET status=?,ended=?,receipt=?,error=? WHERE run_id=?",
                       (status, now, str(receipt) if receipt else None, error, run_id))
            self._event(db, task_id, status, {"run_id": run_id, "cost_usd": cost, "error": error})

    def recover(self) -> list[str]:
        recovered = []
        with self.transaction() as db:
            for row in db.execute("SELECT * FROM tasks WHERE status='running'").fetchall():
                if owner_alive(row["owner_pid"], row["owner_identity"]):
                    continue
                now = time.time()
                # No blind rerun after a crash: files/checks may already have changed.
                db.execute("UPDATE tasks SET status='review',owner_pid=NULL,owner_identity=NULL,updated=?,error=? WHERE id=?",
                           (now, "Interrupted attempt: inspect receipt/files before retry", row["id"]))
                db.execute("DELETE FROM workspace_locks WHERE run_id=?", (row["run_id"],))
                db.execute("UPDATE attempts SET status='interrupted',ended=? WHERE run_id=?", (now, row["run_id"]))
                self._event(db, row["id"], "recovered", {"run_id": row["run_id"]})
                recovered.append(row["id"])
        return recovered

    def review(self, task_id: str, *, accept: bool, note: str):
        if not isinstance(note, str) or not note.strip():
            raise ProductionError("Review needs a concrete human note")
        with self.transaction() as db:
            row = db.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
            if row is None or row["status"] != "review":
                raise ProductionError("Only tasks awaiting review can be reviewed")
            if accept and row["error"]:
                raise ProductionError("An interrupted/failed attempt cannot be accepted as tested")
            status = "passed" if accept else "failed"
            db.execute("UPDATE tasks SET status=?,updated=? WHERE id=?", (status, time.time(), task_id))
            self._event(db, task_id, "human_review", {"accepted": accept, "note": note})

    def retry(self, task_id: str):
        with self.transaction() as db:
            row = db.execute("SELECT * FROM tasks WHERE id=?", (task_id,)).fetchone()
            if row is None or row["status"] not in ("failed", "review"):
                raise ProductionError("Only a failed/recovered task can be retried")
            task = Task.from_dict(json.loads(row["spec"]))
            if row["attempts"] >= task.max_attempts or row["cost"] >= task.budget_usd:
                raise ProductionError("Task attempt/model budget is exhausted; use a new reviewed contract")
            db.execute("UPDATE tasks SET status='queued',next_run=0,error=NULL,updated=? WHERE id=?", (time.time(), task_id))
            self._event(db, task_id, "retry_requested", {})
