"""Local CLI for explicit production jobs, retrieval and evidence review."""
from __future__ import annotations

import argparse
from collections import Counter
from contextlib import closing
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import sqlite3
import subprocess
import sys
import time

from . import __version__
from .context import ContextIndex
from .models import ProductionError, Task
from .runner import Runner, write_json
from .store import Store
from .tools import TOOLS, validate_check


def default_state(repo: Path) -> Path:
    result = subprocess.run(["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
                            cwd=repo, capture_output=True, text=True, timeout=10, check=True)
    key = hashlib.sha256(result.stdout.strip().encode()).hexdigest()[:16]
    state_home = Path(os.environ.get("XDG_STATE_HOME", str(Path.home() / ".local/state")))
    return state_home / "signal47-production" / "projects" / key


def context_index(state: Path, repo: Path) -> ContextIndex:
    key = hashlib.sha256(str(repo.resolve()).encode()).hexdigest()[:16]
    return ContextIndex(state / f"context-{key}.sqlite3", repo)


def backlog(repo: Path) -> list[dict]:
    path = repo / "Docs/Active/TODO.md"
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    results = []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        match = re.search(r"(?:\|\s*|\*\*)([A-Z]+-\d+[A-Z]?)(?:\s*\||:?\*\*)", line)
        if match and "[ ]" in line:
            results.append({"id": match.group(1), "description": line.strip(),
                            "source": "Docs/Active/TODO.md", "line": number,
                            "source_sha256": digest, "execution_status": "not_enqueued"})
    return results


def status_data(store: Store) -> dict:
    rows = store.list()
    return {"schema_version": 1, "version": __version__, "counts": dict(Counter(row["status"] for row in rows)),
            "budget_consumed_usd": round(sum(row["cost"] for row in rows), 6),
            "tasks": [{"id": row["id"], "title": row["spec"]["title"], "status": row["status"],
                       "workspace": row["workspace"], "attempts": row["attempts"],
                       "budget_consumed_usd": row["cost"], "error": row["error"],
                       "receipt": row["receipt"]} for row in rows]}


def doctor(repo: Path) -> dict:
    executable = shutil.which("claude")
    authentication = {"loggedIn": False}
    if executable:
        result = subprocess.run([executable, "auth", "status"], capture_output=True,
                                text=True, timeout=15, check=False)
        try:
            data = json.loads(result.stdout)
            authentication = {key: data.get(key) for key in ("loggedIn", "authMethod", "subscriptionType")}
        except (ValueError, TypeError):
            authentication["error"] = "Claude authentication status was unavailable"
    with closing(sqlite3.connect(":memory:")) as db:
        try:
            db.execute("CREATE VIRTUAL TABLE probe USING fts5(text)")
            fts = True
        except sqlite3.OperationalError:
            fts = False
    return {"python": sys.version.split()[0], "sqlite": sqlite3.sqlite_version, "fts5": fts,
            "claude_executable": executable, "claude_auth": authentication,
            "claude_quota": "UNKNOWN_UNTIL_REQUEST",
            "production_scripts": {name: (repo / "unreal/Signal47/Scripts" / name).is_file()
                                   for name in ("build-editor.sh", "package-linux.sh", "unreal_guard.py",
                                                "production_lease.py", "graphics-review.py", "anomaly-pilot.py")},
            "ready_for_claude_jobs": bool(executable and authentication.get("loggedIn")),
            "engine_execution": "NOT_RUN", "gameplay_acceptance": "NOT_CLAIMED"}


def parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description="Signal 47: varig produksjonskø med Claude, kildesøk og testbevis")
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    ap.add_argument("--state-dir", type=Path, help="Lokale kødata/bevis; standard deles mellom repoets worktrees")
    ap.add_argument("--json", action="store_true", help="Maskinlesbar status")
    ap.add_argument("--version", action="version", version=__version__)
    sub = ap.add_subparsers(dest="command", required=True)
    sub.add_parser("init", help="Opprett lokal kø og indeks")
    sub.add_parser("doctor", help="Kontroller verktøy og innlogging uten modell-/motorjobb")
    sub.add_parser("index", help="Bygg kildesøket på nytt")
    search = sub.add_parser("search", help="Finn gjeldende kilder med fil/linje/hash")
    search.add_argument("query")
    search.add_argument("--limit", type=int, default=6)
    sub.add_parser("backlog", help="Les åpne kanoniske oppgaver uten å kjøre dem")
    sub.add_parser("tools", help="Vis navngitte verktøy og krav om tungt vindu")
    enqueue = sub.add_parser("enqueue", help="Legg validerte JSON-kontrakter i kø")
    enqueue.add_argument("files", type=Path, nargs="+")
    run = sub.add_parser("run", help="Kjør klare oppgaver og gjenoppta køen etter avbrudd")
    run.add_argument("--job")
    run.add_argument("--max-jobs", type=int, default=1, help="Maks forsøk i denne prosessen")
    run.add_argument("--allow-heavy", action="store_true", help="Tillat reserverte Blender-/Unreal-jobber")
    run.add_argument("--watch", action="store_true", help="Vent på klare oppgaver frem til idle-grensen")
    run.add_argument("--idle-seconds", type=float, default=60)
    sub.add_parser("status", help="Vis kø, venting og siste kvitteringer")
    show = sub.add_parser("show", help="Vis én kontrakt og siste kjørebevis")
    show.add_argument("id")
    sub.add_parser("recover", help="Frigi bare døde eiere; avbrutte forsøk sendes til review")
    retry = sub.add_parser("retry", help="Prøv en feilet oppgave igjen innen samme budsjett")
    retry.add_argument("id")
    review = sub.add_parser("review", help="Registrer menneskelig vurdering etter beståtte automatiske kontroller")
    review.add_argument("id")
    choice = review.add_mutually_exclusive_group(required=True)
    choice.add_argument("--accept", action="store_true")
    choice.add_argument("--reject", action="store_true")
    review.add_argument("--note", required=True)
    return ap


def main(argv=None) -> int:
    args = parser().parse_args(argv)
    try:
        repo = args.repo.expanduser().resolve()
        state = args.state_dir.expanduser().resolve() if args.state_dir else default_state(repo)
        state.mkdir(parents=True, mode=0o700, exist_ok=True)
        store = Store(state / "queue.sqlite3")
        command = args.command
        if command == "doctor":
            output = doctor(repo)
        elif command in ("init", "index"):
            output = {"state_dir": str(state), "index": context_index(state, repo).rebuild()}
        elif command == "search":
            if not 1 <= args.limit <= 20:
                raise ProductionError("Search limit must be 1..20")
            output = context_index(state, repo).search(args.query, args.limit)
        elif command == "backlog":
            output = backlog(repo)
        elif command == "tools":
            output = [{"tool": tool, "heavy": heavy} for tool, heavy in TOOLS.items()]
        elif command == "enqueue":
            output = []
            for path in args.files:
                data = json.loads(path.read_text())
                task = Task.from_dict(data)
                for check in task.checks:
                    validate_check(check)
                row = store.enqueue(task, repo)
                output.append({"id": row["id"], "status": row["status"], "workspace": row["workspace"]})
        elif command == "status":
            output = status_data(store)
        elif command == "show":
            output = store.get(args.id)
            if output["receipt"] and Path(output["receipt"]).is_file():
                output["evidence"] = json.loads(Path(output["receipt"]).read_text())
        elif command == "recover":
            output = {"recovered_to_review": store.recover()}
        elif command == "retry":
            store.retry(args.id)
            output = {"id": args.id, "status": "queued"}
        elif command == "review":
            store.review(args.id, accept=args.accept, note=args.note)
            output = {"id": args.id, "status": store.get(args.id)["status"]}
        elif command == "run":
            if not 1 <= args.max_jobs <= 100 or not 0 <= args.idle_seconds <= 3600:
                raise ProductionError("max-jobs must be 1..100; idle-seconds must be 0..3600")
            recovered = store.recover()
            runner = Runner(store, state, allow_heavy=args.allow_heavy)
            output = {"recovered_to_review": recovered, "runs": []}
            idle_at = time.monotonic()
            deferred = set()
            for _ in range(args.max_jobs):
                while True:
                    result = runner.run_next(task_id=args.job, excluded=deferred)
                    if result:
                        break
                    if not args.watch or time.monotonic() - idle_at >= args.idle_seconds:
                        break
                    time.sleep(min(3, max(.05, args.idle_seconds)))
                if not result:
                    break
                output["runs"].append(result)
                print(json.dumps(result, ensure_ascii=False), flush=True)
                if result["status"] in ("blocked", "review", "failed"):
                    deferred.add(result["task_id"])
                idle_at = time.monotonic()
                write_json(state / "status.json", status_data(store))
            write_json(state / "status.json", status_data(store))
            output["state_dir"] = str(state)
        else:
            raise ProductionError("Unknown command")
        if command == "status" and not args.json:
            print(f"Kø: {state}")
            for task in output["tasks"]:
                print(f"{task['id']:24} {task['status']:9} forsøk={task['attempts']} {task['title']}")
                if task["error"]:
                    print("  " + task["error"])
            print("Budsjett brukt/reservert: " + str(output["budget_consumed_usd"]) + " USD (abonnementsbruk er ikke en API-faktura)")
        else:
            print(json.dumps(output, ensure_ascii=False, indent=2, allow_nan=False))
        if command == "run" and any(run["status"] in ("failed", "queued", "blocked") for run in output["runs"]):
            return 2
        return 0
    except (ProductionError, OSError, ValueError, sqlite3.Error, subprocess.SubprocessError) as exc:
        print(f"Production error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Avbrutt. Inspiser kvittering og bruk status/recover før videre kjøring.", file=sys.stderr)
        return 130


def entrypoint():
    def interrupted(signum, frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupted)
    raise SystemExit(main())
