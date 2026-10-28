"""Exercise the whole coordinator pipeline in tiny disposable Git repositories."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from Automation.production.claude import ClaudeError
from Automation.production.models import ProductionError, Task
from Automation.production.runner import Runner, apply_proposal, source_snapshot
from Automation.production.store import Store


def digest(value):
    return hashlib.sha256(value.encode("utf-8") if isinstance(value, str) else value).hexdigest()


def proposal(changes=None, summary="Production work prepared", notes="See the cited current source."):
    return {"summary": summary, "notes": notes, "changes": changes or []}


class FakeClaude:
    def __init__(self, *outputs, callback=None):
        self.outputs = list(outputs)
        self.prompts = []
        self.arguments = []
        self.callback = callback

    def generate(self, prompt, **arguments):
        self.prompts.append(prompt)
        self.arguments.append(arguments)
        if self.callback:
            self.callback()
        if arguments.get("heartbeat"):
            arguments["heartbeat"]()
        output = self.outputs.pop(0)
        if isinstance(output, BaseException):
            raise output
        return {
            "proposal": output, "cost_usd": 0.125, "session_id": "fake-session",
            "usage": {"input_tokens": 7, "output_tokens": 11}, "command": ["fake-claude"],
        }


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.repo = self.directory / "repo"
        self.repo.mkdir()
        self.state = self.directory / "state"
        self.state.mkdir()
        self.git("init", "-q")
        self.put(".gitignore", "__pycache__/\n")
        self.put("AGENTS.md", "Keep the accepted source and preserve player saves.\n")
        self.put("Docs/Active/STATUS.md", "Pilot production source is ready for an independent check.\n")
        self.put("Automation/pilot.py", "value = 1\n")
        self.git("add", ".")
        self.git("-c", "user.name=Production Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "fixture")
        self.head = self.git("rev-parse", "HEAD").strip()
        self.store = Store(self.state / "queue.sqlite3")

    def git(self, *arguments):
        result = subprocess.run(["git", *arguments], cwd=self.repo, text=True, capture_output=True, check=True)
        return result.stdout

    def put(self, name, contents):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(contents, encoding="utf-8")
        return path

    def task(self, identifier="PILOT-01", **changes):
        spec = {
            "id": identifier, "title": "Pilot production source", "description": "Inspect or improve the source and run its actual checks.",
            "kind": "report", "checks": [{"tool": "files", "params": {"paths": ["Docs/Active/STATUS.md"]}}],
            "context_query": "pilot production source", "requires_review": False,
            "max_attempts": 1, "budget_usd": 1,
        }
        spec.update(changes)
        return Task.from_dict(spec)

    def run_task(self, task, client):
        self.store.enqueue(task, self.repo)
        return Runner(self.store, self.state, client=client).run_next(task_id=task.id)

    @staticmethod
    def receipt(result):
        return json.loads(Path(result["receipt"]).read_text())

    def test_report_pipeline_records_context_checks_source_and_model_usage(self):
        client = FakeClaude(proposal())
        result = self.run_task(self.task(artifacts=["Docs/Active/STATUS.md"]), client)
        self.assertEqual(result["status"], "passed")
        receipt = self.receipt(result)
        self.assertEqual(receipt["source_before"]["head"], self.head)
        self.assertEqual(receipt["source_after"]["head"], self.head)
        self.assertEqual(receipt["actual_cost_usd"], 0.125)
        self.assertEqual(self.store.get("PILOT-01")["cost"], 0.125)
        self.assertEqual(receipt["gameplay_acceptance"], "NOT_CLAIMED")
        self.assertTrue(receipt["checks"][0]["passed"])
        expected = digest((self.repo / "Docs/Active/STATUS.md").read_bytes())
        self.assertEqual(receipt["checks"][0]["files"][0]["sha256"], expected)
        self.assertEqual(receipt["artifacts"][0]["sha256"], expected)
        self.assertEqual(receipt["changes"], [])
        self.assertIn("PROJECT INSTRUCTIONS (AGENTS.md)", client.prompts[0])
        self.assertIn("Docs/Active/STATUS.md", client.prompts[0])
        self.assertTrue((Path(result["receipt"]).parent / "context.txt").is_file())

    def test_command_pipeline_does_not_invoke_a_model(self):
        client = FakeClaude()
        result = self.run_task(self.task(kind="command"), client)
        self.assertEqual(result["status"], "passed")
        self.assertEqual(client.prompts, [])
        self.assertEqual(self.receipt(result)["actual_cost_usd"], 0)

    def test_edit_pipeline_applies_validated_content_and_records_actual_hashes(self):
        before, after = "value = 1\n", "value = 2\n"
        path = self.repo / "Automation/pilot.py"
        path.chmod(0o755)
        client = FakeClaude(proposal([{"path": "Automation/pilot.py", "content": after, "base_sha256": digest(before)}]))
        task = self.task(kind="edit", allowed_paths=["Automation/pilot.py"], checks=[
            {"tool": "python_compile", "params": {"paths": ["Automation/pilot.py"]}},
            {"tool": "files", "params": {"paths": ["Automation/pilot.py"]}},
        ])
        result = self.run_task(task, client)
        self.assertEqual(result["status"], "passed")
        self.assertEqual(path.read_text(), after)
        self.assertEqual(path.stat().st_mode & 0o777, 0o755)
        receipt = self.receipt(result)
        self.assertEqual(receipt["changes"][0]["before_sha256"], digest(before))
        self.assertEqual(receipt["changes"][0]["after_sha256"], digest(after))
        self.assertEqual(receipt["source_after"]["changed_files"]["Automation/pilot.py"]["sha256"], digest(after))
        self.assertIn(digest(before), client.prompts[0])

    def test_edit_can_create_a_new_file_inside_a_directory_scope(self):
        name = "Automation/generated/pilot.py"
        client = FakeClaude(proposal([{"path": name, "content": "value = 3\n", "base_sha256": None}]))
        task = self.task(kind="edit", allowed_paths=["Automation/generated/"], checks=[
            {"tool": "python_compile", "params": {"paths": [name]}},
        ])
        result = self.run_task(task, client)
        self.assertEqual(result["status"], "passed")
        self.assertTrue((self.repo / name).is_file())
        self.assertIsNone(self.receipt(result)["changes"][0]["before_sha256"])

    def test_report_cannot_smuggle_a_file_change(self):
        original = (self.repo / "Automation/pilot.py").read_text()
        client = FakeClaude(proposal([{"path": "Automation/pilot.py", "content": "value = 9\n", "base_sha256": digest(original)}]))
        result = self.run_task(self.task(), client)
        self.assertEqual(result["status"], "failed")
        self.assertIn("cannot change files", result["error"])
        self.assertEqual((self.repo / "Automation/pilot.py").read_text(), original)

    def test_allowed_path_boundary_cannot_be_bypassed(self):
        client = FakeClaude(proposal([{"path": "Automation/generated-other/pilot.py", "content": "value = 9\n", "base_sha256": None}]))
        result = self.run_task(self.task(kind="edit", allowed_paths=["Automation/generated/"]), client)
        self.assertEqual(result["status"], "failed")
        self.assertIn("outside task ownership", result["error"])
        self.assertFalse((self.repo / "Automation/generated-other/pilot.py").exists())

    def test_preflight_rejects_stale_hash_before_any_file_is_applied(self):
        second = self.put("Automation/second.py", "second = 1\n")
        original = (self.repo / "Automation/pilot.py").read_text()
        task = self.task(kind="edit", allowed_paths=["Automation/"])
        changes = [
            {"path": "Automation/pilot.py", "content": "value = 2\n", "base_sha256": digest(original)},
            {"path": "Automation/second.py", "content": "second = 2\n", "base_sha256": "0" * 64},
        ]
        with self.assertRaisesRegex(ProductionError, "Stale file hash"):
            apply_proposal(task, proposal(changes), self.repo)
        self.assertEqual((self.repo / "Automation/pilot.py").read_text(), original)
        self.assertEqual(second.read_text(), "second = 1\n")

    def test_preflight_rejects_unauthorized_second_change_before_first_apply(self):
        original = (self.repo / "Automation/pilot.py").read_text()
        changes = [
            {"path": "Automation/pilot.py", "content": "value = 2\n", "base_sha256": digest(original)},
            {"path": "Docs/Active/other.md", "content": "unauthorized", "base_sha256": None},
        ]
        with self.assertRaisesRegex(ProductionError, "ownership"):
            apply_proposal(self.task(kind="edit", allowed_paths=["Automation/pilot.py"]), proposal(changes), self.repo)
        self.assertEqual((self.repo / "Automation/pilot.py").read_text(), original)
        self.assertFalse((self.repo / "Docs/Active/other.md").exists())

    def test_symlink_ancestor_cannot_redirect_an_owned_change(self):
        outside = self.directory / "outside"
        outside.mkdir()
        (self.repo / "Automation/linked").symlink_to(outside, target_is_directory=True)
        change = {"path": "Automation/linked/file.py", "content": "changed", "base_sha256": None}
        with self.assertRaisesRegex(ProductionError, "Symlink"):
            apply_proposal(self.task(kind="edit", allowed_paths=["Automation/"]), proposal([change]), self.repo)
        self.assertEqual(list(outside.iterdir()), [])

    def test_noop_and_empty_edit_proposals_are_not_implementations(self):
        task = self.task(kind="edit", allowed_paths=["Automation/pilot.py"])
        with self.assertRaises(ProductionError):
            apply_proposal(task, proposal(), self.repo)
        with self.assertRaisesRegex(ProductionError, "No-op"):
            apply_proposal(task, proposal([{
                "path": "Automation/pilot.py", "content": "value = 1\n", "base_sha256": digest("value = 1\n"),
            }]), self.repo)

    def test_failed_real_check_prevents_success_and_is_in_the_receipt(self):
        client = FakeClaude(proposal([{"path": "Automation/pilot.py", "content": "if not valid python", "base_sha256": digest("value = 1\n")}]))
        result = self.run_task(self.task(kind="edit", allowed_paths=["Automation/pilot.py"], checks=[
            {"tool": "python_compile", "params": {"paths": ["Automation/pilot.py"]}},
        ]), client)
        self.assertEqual(result["status"], "failed")
        self.assertFalse(self.receipt(result)["checks"][0]["passed"])
        self.assertEqual(self.store.get("PILOT-01")["status"], "failed")

    def test_failed_check_receipt_cannot_be_promoted_to_a_pass(self):
        self.store.enqueue(self.task(kind="command"), self.repo)
        with patch("Automation.production.runner.execute_check", return_value={"tool": "files", "passed": False, "error": "Independent verifier rejected output"}):
            result = Runner(self.store, self.state, client=FakeClaude()).run_next()
        self.assertEqual(result["status"], "failed")

    def test_retry_prompt_contains_the_actual_previous_failure(self):
        bad = {"path": "Automation/pilot.py", "content": "value = ", "base_sha256": digest("value = 1\n")}
        good = {"path": "Automation/pilot.py", "content": "value = 3\n", "base_sha256": digest("value = ")}
        client = FakeClaude(proposal([bad]), proposal([good]))
        task = self.task(kind="edit", allowed_paths=["Automation/pilot.py"], max_attempts=2, checks=[
            {"tool": "python_compile", "params": {"paths": ["Automation/pilot.py"]}},
        ])
        first = self.run_task(task, client)
        self.assertEqual(first["status"], "queued")
        with self.store.connection() as connection:
            connection.execute("UPDATE tasks SET next_run=0 WHERE id=?", (task.id,))
        second = Runner(self.store, self.state, client=client).run_next(task_id=task.id)
        self.assertEqual(second["status"], "passed")
        self.assertIn(first["error"], client.prompts[1])
        self.assertIn(digest("value = "), client.prompts[1])
        self.assertEqual(self.store.get(task.id)["attempts"], 2)
        self.assertEqual(client.arguments[1]["budget_usd"], 0.875)

    def test_human_review_is_a_distinct_state_with_no_automatic_gameplay_acceptance(self):
        result = self.run_task(self.task(requires_review=True), FakeClaude(proposal()))
        self.assertEqual(result["status"], "review")
        self.assertEqual(self.store.get("PILOT-01")["status"], "review")
        self.assertEqual(self.receipt(result)["gameplay_acceptance"], "HUMAN_REVIEW_REQUIRED")
        self.store.review("PILOT-01", accept=True, note="Source and recorded result inspected.")
        self.assertEqual(self.store.get("PILOT-01")["status"], "passed")

    def test_heavy_job_is_deferred_before_any_engine_or_model_invocation(self):
        task = self.task(checks=[{"tool": "build_editor", "params": {}}])
        client = FakeClaude()
        self.store.enqueue(task, self.repo)
        with patch("Automation.production.runner.execute_check") as execute, patch("Automation.production.runner.load_script") as load:
            result = Runner(self.store, self.state, client=client).run_next()
        self.assertEqual(result["status"], "blocked")
        self.assertEqual(self.store.get(task.id)["attempts"], 0)
        self.assertEqual(client.prompts, [])
        execute.assert_not_called()
        load.assert_not_called()

    def test_model_failure_preserves_reported_cost_and_never_retries_blindly(self):
        failure = ClaudeError("Model permission or schema failure", cost_usd=0.2)
        result = self.run_task(self.task(max_attempts=2), FakeClaude(failure))
        self.assertEqual(result["status"], "failed")
        self.assertEqual(self.store.get("PILOT-01")["cost"], 0.2)
        self.assertEqual(self.receipt(result)["actual_cost_usd"], 0.2)

    def test_unknown_model_cost_reserves_budget_without_claiming_a_measured_bill(self):
        result = self.run_task(self.task(), FakeClaude(ClaudeError("No usage response")))
        receipt = self.receipt(result)
        self.assertEqual(result["status"], "failed")
        self.assertIsNone(receipt["actual_cost_usd"])
        self.assertEqual(receipt["budget_consumed_usd"], 1)
        self.assertIn("not a measured bill", receipt["budget_note"])

    def test_interrupted_attempt_cannot_be_accepted_as_tested(self):
        result = self.run_task(self.task(), FakeClaude(InterruptedError("Fixture interrupt")))
        self.assertEqual(result["status"], "review")
        with self.assertRaises(ProductionError):
            self.store.review("PILOT-01", accept=True, note="No actual checks completed.")

    def test_head_change_during_run_invalidates_the_evidence(self):
        def external_change():
            self.put("Automation/external.txt", "another author changed HEAD\n")
            self.git("add", "Automation/external.txt")
            self.git("-c", "user.name=Production Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "concurrent source change")
        result = self.run_task(self.task(), FakeClaude(proposal(), callback=external_change))
        self.assertEqual(result["status"], "failed")
        self.assertIn("HEAD changed", result["error"])

    def test_snapshot_hashes_actual_uncommitted_source_bytes(self):
        self.put("Automation/pilot.py", "value = 4\n")
        self.put("Automation/new.py", "value = 5\n")
        snapshot = source_snapshot(self.repo)
        self.assertEqual(snapshot["head"], self.head)
        self.assertEqual(snapshot["changed_files"]["Automation/pilot.py"]["sha256"], digest("value = 4\n"))
        self.assertEqual(snapshot["changed_files"]["Automation/new.py"]["sha256"], digest("value = 5\n"))


if __name__ == "__main__":
    unittest.main()
