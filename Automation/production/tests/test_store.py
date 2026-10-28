"""Transactional queue, ownership and recovery tests; no engine or network."""

import math
import multiprocessing
import os
from pathlib import Path
import tempfile
import time
import unittest
from unittest.mock import patch

from Automation.production.models import ProductionError, Task
from Automation.production.store import Store, owner_alive, process_identity


def task(task_id="TASK-A", **updates):
    data = {
        "id": task_id,
        "title": "Production source check",
        "description": "Validate one source and retain an actual execution receipt.",
        "kind": "command",
        "checks": [{"tool": "source_check", "params": {}}],
        "requires_review": False,
    }
    data.update(updates)
    return Task.from_dict(data)


def claim_in_process(database, task_id, barrier, output):
    """A genuinely independent SQLite claimant, then a simulated dead worker."""
    try:
        store = Store(Path(database))
        if barrier is not None:
            barrier.wait(timeout=8)
        result = store.claim(task_id=task_id)
        output.put({"id": result["id"], "run_id": result["run_id"]} if result else None)
    except BaseException as error:
        output.put({"error": type(error).__name__ + ": " + str(error)})


class StoreTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.workspace = self.root / "workspace"
        self.workspace.mkdir()
        self.database = self.root / "production.sqlite3"
        self.store = Store(self.database)

    def enqueue(self, task_id="TASK-A", workspace=None, **updates):
        return self.store.enqueue(task(task_id, **updates), workspace or self.workspace)

    def claim(self, task_id="TASK-A"):
        value = self.store.claim(task_id=task_id)
        self.assertIsNotNone(value)
        return value

    def parallel_claims(self, identifiers):
        context = multiprocessing.get_context("spawn")
        barrier = context.Barrier(len(identifiers))
        output = context.Queue()
        children = [context.Process(target=claim_in_process, args=(str(self.database), identifier, barrier, output))
                    for identifier in identifiers]
        for child in children:
            child.start()
        try:
            results = [output.get(timeout=12) for _ in children]
            for child in children:
                child.join(timeout=8)
                self.assertFalse(child.is_alive(), "Claimant failed to exit")
                self.assertEqual(child.exitcode, 0)
            self.assertFalse([value for value in results if value and "error" in value], results)
            return results
        finally:
            for child in children:
                if child.is_alive():
                    child.terminate()
                    child.join(timeout=3)
            output.close()
            output.join_thread()

    def test_enqueue_is_idempotent_and_returns_the_same_decoded_shape(self):
        first = self.enqueue()
        second = self.enqueue()
        self.assertEqual(first, second)
        self.assertIsInstance(second["spec"], dict)
        self.assertEqual(len(self.store.list()), 1)
        with self.store.connection() as connection:
            self.assertEqual(connection.execute("SELECT count(*) FROM events WHERE kind='enqueued'").fetchone()[0], 1)

    def test_same_identifier_cannot_replace_contract_or_workspace(self):
        self.enqueue()
        with self.assertRaises(ProductionError):
            self.enqueue(description="Changed scope")
        elsewhere = self.root / "elsewhere"
        elsewhere.mkdir()
        with self.assertRaises(ProductionError):
            self.enqueue(workspace=elsewhere)
        self.assertEqual(self.store.get("TASK-A")["spec"]["description"], task().description)

    def test_enqueue_revalidates_a_direct_task_constructor(self):
        invalid = Task(id="invalid/id", title="", description="")
        with self.assertRaises(ProductionError):
            self.store.enqueue(invalid, self.workspace)
        self.assertEqual(self.store.list(), [])

    def test_queue_survives_reopening_and_unknown_task_is_explicit(self):
        self.enqueue()
        reopened = Store(self.database)
        self.assertEqual(reopened.get("TASK-A")["status"], "queued")
        with self.assertRaises(ProductionError):
            reopened.get("MISSING")

    def test_parallel_claim_of_one_task_is_atomic(self):
        self.enqueue()
        results = self.parallel_claims(["TASK-A", "TASK-A"])
        self.assertEqual(sum(value is not None for value in results), 1)
        self.assertEqual(self.store.get("TASK-A")["attempts"], 1)
        with self.store.connection() as connection:
            self.assertEqual(connection.execute("SELECT count(*) FROM attempts").fetchone()[0], 1)

    def test_workspace_is_serialized_across_processes(self):
        self.enqueue("TASK-A")
        self.enqueue("TASK-B")
        results = self.parallel_claims(["TASK-A", "TASK-B"])
        self.assertEqual(sum(value is not None for value in results), 1)
        self.assertEqual(sorted(value["status"] for value in self.store.list()), ["blocked", "running"])
        with self.store.connection() as connection:
            self.assertEqual(connection.execute("SELECT count(*) FROM workspace_locks").fetchone()[0], 1)

    def test_independent_workspaces_can_be_claimed_in_parallel(self):
        other = self.root / "second-workspace"
        other.mkdir()
        self.enqueue("TASK-A")
        self.enqueue("TASK-B", workspace=other)
        self.assertEqual(sum(value is not None for value in self.parallel_claims(["TASK-A", "TASK-B"])), 2)

    def test_workspace_lock_is_released_after_finish(self):
        self.enqueue("TASK-A")
        self.enqueue("TASK-B")
        first = self.claim("TASK-A")
        self.assertIsNone(self.store.claim(task_id="TASK-B"))
        self.assertEqual(self.store.get("TASK-B")["attempts"], 0)
        self.store.finish("TASK-A", first["run_id"], "passed")
        self.assertEqual(self.claim("TASK-B")["status"], "running")

    def test_missing_dependency_blocks_without_spending_an_attempt(self):
        self.enqueue("TASK-B", depends_on=["TASK-A"])
        self.assertIsNone(self.store.claim(task_id="TASK-B"))
        blocked = self.store.get("TASK-B")
        self.assertEqual(blocked["status"], "blocked")
        self.assertEqual(blocked["attempts"], 0)
        self.assertIn("TASK-A", blocked["error"])
        self.enqueue("TASK-A")
        first = self.claim("TASK-A")
        self.store.finish("TASK-A", first["run_id"], "review")
        self.assertIsNone(self.store.claim(task_id="TASK-B"))
        self.store.review("TASK-A", accept=True, note="Actual source and check evidence inspected.")
        self.assertEqual(self.claim("TASK-B")["attempts"], 1)

    def test_cycle_is_rejected_transactionally_even_with_forward_dependencies(self):
        self.enqueue("TASK-A", depends_on=["TASK-B"])
        self.enqueue("TASK-B", depends_on=["TASK-C"])
        with self.assertRaisesRegex(ProductionError, "cycle"):
            self.enqueue("TASK-C", depends_on=["TASK-A"])
        self.assertEqual([row["id"] for row in self.store.list()], ["TASK-A", "TASK-B"])
        self.enqueue("TASK-C")
        self.assertIsNotNone(self.store.claim(task_id="TASK-C"))

    def test_excluded_tasks_are_not_claimed(self):
        self.enqueue("TASK-A")
        self.enqueue("TASK-B")
        value = self.store.claim(excluded={"TASK-A"})
        self.assertEqual(value["id"], "TASK-B")
        self.assertEqual(self.store.get("TASK-A")["status"], "queued")

    def test_stale_attempt_cannot_heartbeat_or_finish_or_release_new_lock(self):
        self.enqueue(max_attempts=3)
        first = self.claim()
        self.store.finish("TASK-A", first["run_id"], "failed", error="Independent check failed")
        self.store.retry("TASK-A")
        second = self.claim()
        self.assertNotEqual(first["run_id"], second["run_id"])
        with self.assertRaises(ProductionError):
            self.store.heartbeat("TASK-A", first["run_id"])
        with self.assertRaises(ProductionError):
            self.store.finish("TASK-A", first["run_id"], "passed")
        self.assertEqual(self.store.get("TASK-A")["run_id"], second["run_id"])
        self.store.heartbeat("TASK-A", second["run_id"])
        with self.store.connection() as connection:
            self.assertEqual(connection.execute("SELECT run_id FROM workspace_locks").fetchone()[0], second["run_id"])

    def test_recover_never_reclaims_a_live_owner_even_when_heartbeat_is_old(self):
        self.enqueue()
        value = self.claim()
        with self.store.connection() as connection:
            connection.execute("UPDATE tasks SET updated=0 WHERE id='TASK-A'")
        self.assertEqual(self.store.recover(), [])
        self.assertEqual(self.store.get("TASK-A")["status"], "running")
        self.assertTrue(owner_alive(os.getpid(), process_identity()))
        self.assertFalse(owner_alive(os.getpid(), "another-boot:another-start"))
        self.store.finish("TASK-A", value["run_id"], "passed")

    def test_dead_owner_is_recovered_to_review_never_blindly_rerun(self):
        self.enqueue()
        result = self.parallel_claims(["TASK-A"])[0]
        self.assertEqual(self.store.get("TASK-A")["status"], "running")
        self.assertEqual(self.store.recover(), ["TASK-A"])
        row = self.store.get("TASK-A")
        self.assertEqual(row["status"], "review")
        self.assertIn("Interrupted", row["error"])
        self.assertIsNone(self.store.claim(task_id="TASK-A"))
        with self.assertRaises(ProductionError):
            self.store.review("TASK-A", accept=True, note="Cannot substitute this note for lost checks.")
        with self.assertRaises(ProductionError):
            self.store.finish("TASK-A", result["run_id"], "passed")
        with self.store.connection() as connection:
            self.assertEqual(connection.execute("SELECT status FROM attempts").fetchone()[0], "interrupted")
            self.assertEqual(connection.execute("SELECT count(*) FROM workspace_locks").fetchone()[0], 0)
        self.store.retry("TASK-A")
        self.assertEqual(self.claim()["attempts"], 2)

    def test_pid_reuse_identity_mismatch_is_not_a_live_owner(self):
        self.enqueue()
        self.claim()
        with self.store.connection() as connection:
            connection.execute("UPDATE tasks SET owner_identity='old-boot:old-start'")
        self.assertEqual(self.store.recover(), ["TASK-A"])

    @unittest.skipUnless(hasattr(os, "fork") and Path("/proc").is_dir(), "Linux process identity test")
    def test_zombie_process_is_not_a_live_owner(self):
        reader, writer = os.pipe()
        pid = os.fork()
        if pid == 0:
            os.close(writer)
            os.read(reader, 1)
            os.close(reader)
            os._exit(0)
        os.close(reader)
        try:
            identity = process_identity(pid)
            self.assertTrue(identity)
            os.write(writer, b"x")
            os.close(writer)
            writer = None
            deadline = time.monotonic() + 2
            while time.monotonic() < deadline:
                fields = Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()
                if fields[0] == "Z":
                    break
                time.sleep(0.005)
            else:
                self.fail("Owned test child did not become a zombie")
            self.assertFalse(owner_alive(pid, identity))
        finally:
            if writer is not None:
                os.close(writer)
            os.waitpid(pid, 0)

    def test_review_requires_a_note_and_an_actual_review_state(self):
        self.enqueue(requires_review=True)
        value = self.claim()
        with self.assertRaises(ProductionError):
            self.store.review("TASK-A", accept=True, note="Premature review")
        self.store.finish("TASK-A", value["run_id"], "review", receipt=self.root / "receipt.json")
        for note in ("", "   ", None):
            with self.subTest(note=note), self.assertRaises(ProductionError):
                self.store.review("TASK-A", accept=True, note=note)
        self.store.review("TASK-A", accept=False, note="The visual result needs another pass.")
        self.assertEqual(self.store.get("TASK-A")["status"], "failed")

    def test_retry_respects_total_attempt_limit(self):
        self.enqueue(max_attempts=1)
        value = self.claim()
        self.store.finish("TASK-A", value["run_id"], "failed")
        with self.assertRaisesRegex(ProductionError, "exhausted"):
            self.store.retry("TASK-A")
        with self.store.connection() as connection:
            connection.execute("UPDATE tasks SET status='queued'")
        self.assertIsNone(self.store.claim(task_id="TASK-A"))
        self.assertEqual(self.store.get("TASK-A")["status"], "failed")

    def test_retry_respects_cumulative_model_budget(self):
        self.enqueue(budget_usd=1, max_attempts=5)
        value = self.claim()
        self.store.finish("TASK-A", value["run_id"], "failed", cost=0.4)
        self.store.retry("TASK-A")
        value = self.claim()
        self.store.finish("TASK-A", value["run_id"], "failed", cost=0.6)
        self.assertAlmostEqual(self.store.get("TASK-A")["cost"], 1)
        with self.assertRaisesRegex(ProductionError, "exhausted"):
            self.store.retry("TASK-A")

    def test_invalid_cost_cannot_reduce_or_corrupt_budget(self):
        for number, bad in enumerate((-1, math.nan, math.inf, "0.5", True)):
            identifier = f"COST-{number}"
            self.enqueue(identifier)
            value = self.claim(identifier)
            with self.subTest(cost=bad), self.assertRaises(ProductionError):
                self.store.finish(identifier, value["run_id"], "failed", cost=bad)
            row = self.store.get(identifier)
            self.assertEqual(row["status"], "running")
            self.assertEqual(row["cost"], 0)
            self.store.finish(identifier, value["run_id"], "failed", cost=0)

    def test_deferred_contention_does_not_consume_attempt_and_has_backoff(self):
        self.enqueue(max_attempts=1)
        with patch("Automation.production.store.time.time", return_value=1000):
            value = self.claim()
            self.store.finish("TASK-A", value["run_id"], "blocked", defer=True, error="Heavy resource busy")
        row = self.store.get("TASK-A")
        self.assertEqual(row["attempts"], 0)
        self.assertEqual(row["next_run"], 1003)
        with patch("Automation.production.store.time.time", return_value=1002):
            self.assertIsNone(self.store.claim())
        with patch("Automation.production.store.time.time", return_value=1003):
            self.assertEqual(self.claim()["attempts"], 1)

    def test_deferral_cannot_hide_a_completed_or_failed_attempt(self):
        for number, status in enumerate(("passed", "review", "failed")):
            identifier = f"DEFER-{number}"
            self.enqueue(identifier)
            value = self.claim(identifier)
            with self.subTest(status=status), self.assertRaises(ProductionError):
                self.store.finish(identifier, value["run_id"], status, defer=True)
            self.store.finish(identifier, value["run_id"], "failed")


if __name__ == "__main__":
    unittest.main()
