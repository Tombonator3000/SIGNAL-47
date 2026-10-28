"""Regressions from the actual Claude Max quota response, without API calls."""
import json
from pathlib import Path
import tempfile
import time
import unittest

from Automation.production.claude import ClaudeClient, ClaudeError, MAX_PROPOSAL_BYTES, validate_proposal
from Automation.production.models import Task
from Automation.production.runner import Runner
from Automation.production.store import Store


class AvailabilityTests(unittest.TestCase):
    def test_report_without_changes_still_obeys_proposal_byte_limit(self):
        with self.assertRaises(ClaudeError):
            validate_proposal({"summary": "Report", "notes": "x" * MAX_PROPOSAL_BYTES, "changes": []})

    def test_huge_reported_cost_is_a_controlled_provider_error(self):
        with tempfile.TemporaryDirectory() as directory:
            stdout, stderr = Path(directory) / "out", Path(directory) / "err"
            stdout.write_text(json.dumps({"type": "result", "total_cost_usd": 10 ** 400}))
            stderr.write_text("")
            with self.assertRaises(ClaudeError):
                ClaudeClient()._parse(stdout, stderr, 0, 1)

    def test_final_stdout_size_is_checked_even_when_command_exited_quickly(self):
        with tempfile.TemporaryDirectory() as directory:
            stdout, stderr = Path(directory) / "out", Path(directory) / "err"
            stdout.write_text("x" * 300)
            stderr.write_text("")
            client = ClaudeClient()
            client.max_stdout_bytes = 256
            with self.assertRaisesRegex(ClaudeError, "stdout exceeded"):
                client._parse(stdout, stderr, 0, 1)

    def test_nonzero_exit_and_success_subtype_still_identifies_real_quota_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            stdout = Path(directory) / "stdout.json"
            stderr = Path(directory) / "stderr.log"
            stdout.write_text(json.dumps({"type": "result", "subtype": "success", "is_error": True,
                                          "result": "You've hit your 5-hour limit · resets 1:10am (Europe/Oslo)",
                                          "total_cost_usd": 0, "session_id": "fixture", "usage": {},
                                          "permission_denials": []}))
            stderr.write_text("")
            with self.assertRaises(ClaudeError) as caught:
                ClaudeClient()._parse(stdout, stderr, 1, 1)
            self.assertEqual(caught.exception.retry_after_seconds, 900)
            self.assertEqual(caught.exception.cost_usd, 0)
            self.assertIn("quota unavailable", str(caught.exception))

    def test_known_zero_cost_quota_failure_waits_without_consuming_attempts(self):
        class Unavailable:
            def generate(self, *args, **kwargs):
                raise ClaudeError("Claude quota unavailable", cost_usd=0, retry_after_seconds=900)
        with tempfile.TemporaryDirectory() as directory:
            import subprocess
            root = Path(directory) / "repo"
            root.mkdir()
            (root / "proof.txt").write_text("source")
            for command in (["git", "init", "-q"], ["git", "add", "."],
                            ["git", "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "fixture"]):
                subprocess.run(command, cwd=root, capture_output=True, check=True)
            state = Path(directory) / "state"
            store = Store(state / "queue.sqlite3")
            task = Task.from_dict({"id": "QUOTA-01", "title": "Quota fixture", "description": "Exercise availability handling",
                                   "kind": "report", "max_attempts": 1, "checks": [{"tool": "files", "params": {"paths": ["proof.txt"]}}]})
            store.enqueue(task, root)
            result = Runner(store, state, client=Unavailable()).run_next()
            self.assertEqual(result["status"], "blocked")
            stored = store.get(task.id)
            self.assertEqual(stored["attempts"], 0)
            self.assertEqual(stored["cost"], 0)
            self.assertGreater(stored["next_run"], time.time() + 850)
            self.assertIsNone(store.claim())
            receipt = json.loads(Path(result["receipt"]).read_text())
            self.assertEqual(receipt["availability"], "CLAUDE_QUOTA_UNAVAILABLE")
            self.assertNotEqual(receipt["status"], "passed")


if __name__ == "__main__":
    unittest.main()
