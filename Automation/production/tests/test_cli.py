"""CLI workflows in disposable repositories, with Claude authentication mocked."""

from contextlib import redirect_stderr, redirect_stdout
import hashlib
import io
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from Automation.production import cli


class FakeClaude:
    def __init__(self):
        self.calls = []

    def generate(self, prompt, **arguments):
        self.calls.append((prompt, arguments))
        arguments["heartbeat"]()
        return {
            "proposal": {"summary": "Synthetic source report", "notes": "Docs/Active/STATUS.md:1 was inspected by the fixture.", "changes": []},
            "cost_usd": 0.1, "session_id": "fixture-session", "usage": {}, "command": ["fake-claude"],
        }


class CliTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.repo = self.directory / "repo"
        self.repo.mkdir()
        self.state = self.directory / "state"
        self.put("Docs/Active/STATUS.md", "Pilot source and current production status.\n")
        self.put("Docs/Active/TODO.md", "# Active tasks\n| FIX-01 | [ ] Investigate a crash. | evidence |\n- [ ] **GFX-01:** Check the light fixture.\n- [ ] **STORY-02:** Complete the next act.\n- [x] **STORY-04A, kilde:** Already delivered source.\n")
        self.git("init", "-q")
        self.git("add", ".")
        self.git("-c", "user.name=CLI Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "fixture")

    def git(self, *arguments):
        return subprocess.run(["git", *arguments], cwd=self.repo, text=True, capture_output=True, check=True).stdout

    def put(self, name, content):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def invoke(self, *arguments):
        output, error = io.StringIO(), io.StringIO()
        with redirect_stdout(output), redirect_stderr(error):
            code = cli.main(["--repo", str(self.repo), "--state-dir", str(self.state), "--json", *arguments])
        documents = []
        content = output.getvalue().strip()
        decoder = json.JSONDecoder()
        while content:
            document, offset = decoder.raw_decode(content)
            documents.append(document)
            content = content[offset:].lstrip()
        return code, documents[-1] if documents else None, error.getvalue()

    def contract(self, identifier="CLI-01", **changes):
        value = {
            "id": identifier, "title": "Pilot source", "description": "Validate the current source fixture.",
            "kind": "command", "checks": [{"tool": "files", "params": {"paths": ["Docs/Active/STATUS.md"]}}],
            "requires_review": False, "max_attempts": 1,
        }
        value.update(changes)
        path = self.directory / (identifier + ".json")
        path.write_text(json.dumps(value))
        return str(path)

    def test_init_builds_local_queue_and_current_reference_index(self):
        code, result, error = self.invoke("init")
        self.assertEqual((code, error), (0, ""))
        self.assertTrue((self.state / "queue.sqlite3").is_file())
        self.assertGreaterEqual(result["index"]["files"], 2)
        code, found, _ = self.invoke("search", "pilot source")
        self.assertEqual(code, 0)
        self.assertEqual(found[0]["path"], "Docs/Active/STATUS.md")

    def test_enqueue_run_status_and_show_form_a_complete_command_workflow(self):
        contract = self.contract()
        code, added, _ = self.invoke("enqueue", contract)
        self.assertEqual(code, 0)
        self.assertEqual(added[0]["status"], "queued")
        self.assertEqual(self.invoke("enqueue", contract)[0], 0)
        code, run, error = self.invoke("run", "--job", "CLI-01")
        self.assertEqual((code, error), (0, ""))
        self.assertEqual(run["runs"][0]["status"], "passed")
        code, status, _ = self.invoke("status")
        self.assertEqual(code, 0)
        self.assertEqual(status["counts"], {"passed": 1})
        self.assertEqual(status["budget_consumed_usd"], 0)
        code, shown, _ = self.invoke("show", "CLI-01")
        self.assertEqual(code, 0)
        self.assertEqual(shown["evidence"]["task_id"], "CLI-01")
        self.assertEqual(json.loads((self.state / "status.json").read_text())["counts"], {"passed": 1})

    def test_report_workflow_uses_only_the_injected_fake_client(self):
        client = FakeClaude()
        self.invoke("enqueue", self.contract(kind="report"))
        with patch("Automation.production.runner.ClaudeClient", return_value=client):
            code, result, error = self.invoke("run", "--job", "CLI-01")
        self.assertEqual((code, error), (0, ""))
        self.assertEqual(result["runs"][0]["status"], "passed")
        self.assertEqual(len(client.calls), 1)
        self.assertAlmostEqual(self.invoke("status")[1]["budget_consumed_usd"], 0.1)

    def test_review_is_explicit_and_does_not_run_more_model_or_engine_work(self):
        self.invoke("enqueue", self.contract(requires_review=True))
        self.assertEqual(self.invoke("run")[1]["runs"][0]["status"], "review")
        code, reviewed, error = self.invoke("review", "CLI-01", "--accept", "--note", "The synthetic source and actual checks were reviewed.")
        self.assertEqual((code, error), (0, ""))
        self.assertEqual(reviewed["status"], "passed")
        self.assertEqual(self.invoke("status")[1]["counts"], {"passed": 1})

    def test_backlog_includes_table_and_actual_bold_colon_task_formats(self):
        code, results, error = self.invoke("backlog")
        self.assertEqual((code, error), (0, ""))
        self.assertEqual([result["id"] for result in results], ["FIX-01", "GFX-01", "STORY-02"])
        source = self.repo / "Docs/Active/TODO.md"
        expected = hashlib.sha256(source.read_bytes()).hexdigest()
        self.assertTrue(all(result["source_sha256"] == expected and result["execution_status"] == "not_enqueued" for result in results))
        self.assertEqual(self.invoke("status")[1]["tasks"], [])

    def test_doctor_checks_mock_authentication_and_omits_private_account_fields(self):
        response = subprocess.CompletedProcess([], 0, json.dumps({
            "loggedIn": True, "authMethod": "claude.ai", "subscriptionType": "max",
            "email": "private@example.invalid", "orgId": "private-organization",
        }), "")
        with patch("Automation.production.cli.shutil.which", return_value="/fake/claude"), patch("Automation.production.cli.subprocess.run", return_value=response) as run:
            code, result, error = self.invoke("doctor")
        self.assertEqual((code, error), (0, ""))
        self.assertTrue(result["ready_for_claude_jobs"])
        self.assertEqual(result["claude_auth"], {"loggedIn": True, "authMethod": "claude.ai", "subscriptionType": "max"})
        self.assertNotIn("private@example.invalid", json.dumps(result))
        self.assertEqual(result["engine_execution"], "NOT_RUN")
        self.assertEqual(result["gameplay_acceptance"], "NOT_CLAIMED")
        self.assertEqual(run.call_args.args[0], ["/fake/claude", "auth", "status"])

    def test_doctor_without_claude_is_read_only_and_not_ready(self):
        with patch("Automation.production.cli.shutil.which", return_value=None), patch("Automation.production.cli.subprocess.run") as run:
            code, result, error = self.invoke("doctor")
        self.assertEqual((code, error), (0, ""))
        self.assertFalse(result["ready_for_claude_jobs"])
        run.assert_not_called()

    def test_doctor_closes_its_sqlite_probe(self):
        class Connection(sqlite3.Connection):
            closed = False
            def close(self):
                self.closed = True
                return super().close()
        connection = sqlite3.connect(":memory:", factory=Connection)
        try:
            with patch("Automation.production.cli.sqlite3.connect", return_value=connection), patch("Automation.production.cli.shutil.which", return_value=None):
                cli.doctor(self.repo)
            self.assertTrue(connection.closed, "SQLite context-manager exit commits but does not close")
        finally:
            connection.close()

    def test_invalid_contract_or_tool_never_enters_queue(self):
        path = self.contract(checks=[{"tool": "arbitrary_shell", "params": {}}])
        code, result, error = self.invoke("enqueue", path)
        self.assertEqual(code, 1)
        self.assertIsNone(result)
        self.assertIn("Unknown tool", error)
        self.assertEqual(self.invoke("status")[1]["tasks"], [])

    def test_failed_command_returns_nonzero_and_retains_failure_receipt(self):
        self.invoke("enqueue", self.contract(checks=[{"tool": "files", "params": {"paths": ["missing.txt"]}}]))
        code, result, error = self.invoke("run")
        self.assertEqual((code, error), (2, ""))
        self.assertEqual(result["runs"][0]["status"], "failed")
        self.assertEqual(self.invoke("status")[1]["counts"], {"failed": 1})

    def test_heavy_job_returns_nonzero_without_executing_a_tool(self):
        self.invoke("enqueue", self.contract(checks=[{"tool": "build_editor", "params": {}}]))
        with patch("Automation.production.runner.load_script") as load:
            code, result, error = self.invoke("run")
        self.assertEqual((code, error), (2, ""))
        self.assertEqual(result["runs"][0]["status"], "blocked")
        load.assert_not_called()

    def test_run_and_search_parameter_bounds_are_controlled_errors(self):
        for arguments in (("run", "--max-jobs", "0"), ("run", "--idle-seconds", "nan"), ("search", "pilot", "--limit", "0")):
            with self.subTest(arguments=arguments):
                code, result, error = self.invoke(*arguments)
                self.assertEqual(code, 1)
                self.assertIsNone(result)
                self.assertIn("Production error", error)


if __name__ == "__main__":
    unittest.main()
