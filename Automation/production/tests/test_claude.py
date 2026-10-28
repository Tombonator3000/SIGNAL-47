"""Tests for the Claude Code provider using a fake executable. Never runs real Claude."""

from __future__ import annotations

import gc
import hashlib
import json
import os
import signal
import subprocess
import sys
import tempfile
import textwrap
import time
import unittest
import warnings
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from production.claude import (  # noqa: E402
    MAX_PROPOSAL_BYTES,
    PROPOSAL_SCHEMA,
    ClaudeClient,
    ClaudeError,
    validate_proposal,
)

SHA = hashlib.sha256(b"old\n").hexdigest()

FAKE_CLAUDE = textwrap.dedent("""\
    #!{python}
    import json, os, signal, subprocess, sys, time
    cfg = json.load(open(os.environ["FAKE_CLAUDE_CONFIG"]))
    record = {{
        "argv": sys.argv[1:], "stdin": sys.stdin.read(), "cwd": os.getcwd(),
        "pid": os.getpid(), "pgid": os.getpgid(0), "sid": os.getsid(0),
    }}
    child = cfg.get("child")
    if child:
        code = "import time\\n"
        if child == "stubborn":
            code += "import signal\\nsignal.signal(signal.SIGTERM, signal.SIG_IGN)\\n"
        code += "time.sleep(120)\\n"
        proc = subprocess.Popen([sys.executable, "-c", code], stdin=subprocess.DEVNULL,
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        record["child_pid"] = proc.pid
        time.sleep(0.2)
    with open(cfg["record"], "w") as fh:
        json.dump(record, fh)
    if cfg.get("ignore_term"):
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
    time.sleep(cfg.get("sleep", 0))
    if cfg.get("flood"):
        sys.stdout.write("x" * cfg["flood"])
        sys.stdout.flush()
        time.sleep(120)
    sys.stdout.write(cfg.get("stdout", ""))
    sys.stderr.write(cfg.get("stderr", ""))
    sys.exit(cfg.get("exit", 0))
    """)


def good_proposal(**overrides):
    proposal = {
        "summary": "Add the lamp test",
        "notes": "",
        "changes": [
            {"path": "unreal/Source/Lamp.cpp", "content": "// new\n", "base_sha256": None},
            {"path": "Docs/Active/TODO.md", "content": "todo\n", "base_sha256": SHA},
        ],
    }
    proposal.update(overrides)
    return proposal


def result_obj(**overrides):
    obj = {
        "type": "result",
        "subtype": "success",
        "is_error": False,
        "duration_ms": 1200,
        "num_turns": 2,
        "result": "",
        "session_id": "11111111-2222-3333-4444-555555555555",
        "total_cost_usd": 0.0421,
        "usage": {"input_tokens": 10, "output_tokens": 20},
        "permission_denials": [],
        "structured_output": good_proposal(),
    }
    obj.update(overrides)
    return obj


def pid_in_session(pid: int, sid: int) -> bool:
    """True if ``pid`` is a live, non-zombie process in session/group ``sid``."""
    try:
        raw = Path(f"/proc/{pid}/stat").read_bytes()
    except OSError:
        return False
    fields = raw[raw.rfind(b")") + 2:].split()
    return fields[0] != b"Z" and sid in (int(fields[2]), int(fields[3]))


def wait_gone(pid: int, sid: int, limit: float = 5.0) -> bool:
    end = time.monotonic() + limit
    while time.monotonic() < end:
        if not pid_in_session(pid, sid):
            return True
        time.sleep(0.02)
    return False


class FakeClaudeCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.exe = self.tmp / "fake-claude"
        self.exe.write_text(FAKE_CLAUDE.format(python=sys.executable))
        self.exe.chmod(0o755)
        self.cwd = self.tmp / "repo"
        self.cwd.mkdir()
        self.out = self.tmp / "out"
        self.config_path = self.tmp / "config.json"
        self.record_path = self.tmp / "record.json"
        env = mock.patch.dict(os.environ, {"FAKE_CLAUDE_CONFIG": str(self.config_path)})
        env.start()
        self.addCleanup(env.stop)
        self.client = ClaudeClient(str(self.exe))
        self.client.kill_grace = 0.5

    def configure(self, stdout=None, **cfg):
        if stdout is None:
            stdout = result_obj()
        if not isinstance(stdout, str):
            stdout = json.dumps(stdout)
        cfg.setdefault("stdout", stdout)
        cfg["record"] = str(self.record_path)
        self.config_path.write_text(json.dumps(cfg))

    def record(self):
        return json.loads(self.record_path.read_text())

    def run_generate(self, prompt="Plan the SC01 lamp test.", **kwargs):
        kwargs.setdefault("budget_usd", 1.5)
        kwargs.setdefault("timeout", 20)
        return self.client.generate(prompt, cwd=self.cwd, output_dir=self.out, **kwargs)

    def assert_fails(self, pattern, **kwargs):
        with self.assertRaisesRegex(ClaudeError, pattern) as ctx:
            self.run_generate(**kwargs)
        meta = json.loads((self.out / "request.json").read_text())
        self.assertEqual(meta["status"], "error")
        return ctx.exception


class SuccessTests(FakeClaudeCase):
    def test_structured_output_success(self):
        self.configure(stderr="some warning\n")
        prompt = "Plan the SC01 lamp test.\nLine two: æøå\n"
        result = self.run_generate(prompt=prompt)
        self.assertEqual(set(result), {"proposal", "cost_usd", "session_id", "usage", "command"})
        self.assertEqual(result["proposal"], good_proposal())
        self.assertEqual(result["cost_usd"], 0.0421)
        self.assertEqual(result["session_id"], "11111111-2222-3333-4444-555555555555")
        self.assertEqual(result["usage"], {"input_tokens": 10, "output_tokens": 20})
        self.assertEqual(result["command"][0], str(self.exe))

        rec = self.record()
        self.assertEqual(rec["stdin"], prompt)
        self.assertEqual(rec["argv"], result["command"][1:])
        self.assertNotIn(prompt.strip(), " ".join(rec["argv"]))
        self.assertEqual(Path(rec["cwd"]).resolve(), self.cwd.resolve())
        # Private session: the fake is its own session and group leader.
        self.assertEqual(rec["sid"], rec["pid"])
        self.assertEqual(rec["pgid"], rec["pid"])
        self.assertNotEqual(rec["sid"], os.getsid(0))

    def test_artifacts_saved_without_environment(self):
        stdout = json.dumps(result_obj())
        self.configure(stdout=stdout, stderr="warn\n")
        with mock.patch.dict(os.environ, {"ANTHROPIC_API_KEY": "sk-secret-value"}):
            self.run_generate()
        self.assertEqual((self.out / "stdout.json").read_text(), stdout)
        self.assertEqual((self.out / "stderr.log").read_text(), "warn\n")
        meta_text = (self.out / "request.json").read_text()
        self.assertNotIn("sk-secret-value", meta_text)
        self.assertNotIn("Plan the SC01", meta_text)
        meta = json.loads(meta_text)
        self.assertEqual(meta["status"], "ok")
        self.assertEqual(meta["returncode"], 0)
        self.assertEqual(meta["cost_usd"], 0.0421)
        self.assertEqual(meta["prompt_sha256"],
                         hashlib.sha256(b"Plan the SC01 lamp test.").hexdigest())
        self.assertFalse(meta["timed_out"])

    def test_result_string_json(self):
        self.configure(result_obj(structured_output=None,
                                  result=json.dumps(good_proposal())))
        self.assertEqual(self.run_generate()["proposal"], good_proposal())

    def test_result_string_fenced_json(self):
        text = "```json\n" + json.dumps(good_proposal(changes=[])) + "\n```"
        del_obj = result_obj(result=text)
        del del_obj["structured_output"]
        self.configure(del_obj)
        self.assertEqual(self.run_generate()["proposal"]["changes"], [])

    def test_verbose_message_list(self):
        self.configure([{"type": "system", "subtype": "init"}, result_obj()])
        self.assertEqual(self.run_generate()["cost_usd"], 0.0421)


class CommandTests(FakeClaudeCase):
    def test_readonly_tool_surface_and_flags(self):
        self.configure()
        argv = self.run_generate(budget_usd=2.25)["command"][1:]

        def value(flag):
            self.assertEqual(argv.count(flag), 1, flag)
            return argv[argv.index(flag) + 1]

        self.assertIn("-p", argv)
        self.assertEqual(value("--output-format"), "json")
        self.assertEqual(value("--permission-mode"), "dontAsk")
        self.assertEqual(value("--permission-prompts"), "none")
        self.assertEqual(value("--tools"), "Read,Glob,Grep")
        self.assertEqual(value("--allowedTools"), "Read,Glob,Grep")
        self.assertEqual(json.loads(value("--mcp-config")), {"mcpServers": {}})
        self.assertEqual(json.loads(value("--json-schema")), PROPOSAL_SCHEMA)
        self.assertEqual(float(value("--max-budget-usd")), 2.25)
        for flag in ("--strict-mcp-config", "--safe-mode", "--disable-slash-commands"):
            self.assertIn(flag, argv)
        for flag in ("--dangerously-skip-permissions", "--bare", "--model",
                     "--allow-dangerously-skip-permissions", "--add-dir"):
            self.assertNotIn(flag, argv)
        joined = " ".join(argv)
        for tool in ("Bash", "Edit", "Write", "NotebookEdit", "WebFetch"):
            self.assertNotIn(tool, joined.replace("additionalProperties", ""))

    def test_popen_uses_no_shell_and_new_session(self):
        self.configure()
        real = subprocess.Popen
        calls = []

        def spy(*args, **kwargs):
            calls.append(kwargs)
            return real(*args, **kwargs)

        with mock.patch("production.claude.subprocess.Popen", side_effect=spy):
            self.run_generate()
        self.assertEqual(len(calls), 1)
        self.assertIs(calls[0]["shell"], False)
        self.assertIs(calls[0]["start_new_session"], True)
        self.assertIs(calls[0]["close_fds"], True)

    def test_missing_executable(self):
        client = ClaudeClient(str(self.tmp / "nope"))
        with self.assertRaisesRegex(ClaudeError, "not found"):
            client.generate("x", cwd=self.cwd, output_dir=self.out, budget_usd=1, timeout=5)

    def test_invalid_arguments(self):
        self.configure()
        for kwargs in ({"budget_usd": 0}, {"budget_usd": -1}, {"budget_usd": float("nan")},
                       {"budget_usd": float("inf")}, {"budget_usd": True},
                       {"budget_usd": 1e-9}, {"timeout": 0}, {"timeout": float("inf")},
                       {"prompt": ""}, {"prompt": "  \n"}, {"heartbeat": 5}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ClaudeError):
                self.run_generate(**kwargs)
        with self.assertRaisesRegex(ClaudeError, "cwd"):
            self.client.generate("x", cwd=self.tmp / "missing", output_dir=self.out,
                                 budget_usd=1, timeout=5)
        self.assertFalse(self.record_path.exists())


class FailureTests(FakeClaudeCase):
    def test_nonzero_exit_with_success_json(self):
        self.configure(exit=1, stderr="boom\n")
        exc = self.assert_fails("exited with status 1.*boom")
        self.assertEqual(exc.cost_usd, 0.0421)
        self.assertEqual(exc.session_id, "11111111-2222-3333-4444-555555555555")

    def test_nonzero_exit_without_output(self):
        self.configure(stdout="", exit=2, stderr="auth failed\n")
        self.assert_fails("without a result.*auth failed")

    def test_is_error(self):
        self.configure(result_obj(is_error=True, result="API Error"))
        self.assert_fails("reported an error")

    def test_is_error_must_be_false(self):
        for value in (None, 0, "false"):
            with self.subTest(value=value):
                self.configure(result_obj(is_error=value))
                self.assert_fails("reported an error")

    def test_budget_subtype(self):
        self.configure(result_obj(subtype="error_max_budget_usd", structured_output=None))
        self.assert_fails("error_max_budget_usd")

    def test_cost_over_budget(self):
        self.configure(result_obj(total_cost_usd=2.0))
        self.assert_fails("exceeds budget")

    def test_permission_denials(self):
        denial = {"tool_name": "Write", "tool_use_id": "t1", "tool_input": {}}
        self.configure(result_obj(permission_denials=[denial]))
        self.assert_fails("permission denial.*Write")

    def test_permission_denials_missing(self):
        obj = result_obj()
        del obj["permission_denials"]
        self.configure(obj)
        self.assert_fails("permission_denials")

    def test_invalid_costs(self):
        base = json.dumps(result_obj(total_cost_usd=0.5))
        bad_stdout = [
            base.replace('"total_cost_usd": 0.5', '"total_cost_usd": NaN'),
            base.replace('"total_cost_usd": 0.5', '"total_cost_usd": Infinity'),
            base.replace('"total_cost_usd": 0.5', '"total_cost_usd": 1e999'),
        ]
        for index, stdout in enumerate(bad_stdout):
            with self.subTest(case=index):
                self.configure(stdout=stdout)
                self.assert_fails("no JSON result|total_cost_usd")
        for cost in (-0.01, "0.1", None, True, [1]):
            with self.subTest(cost=cost):
                self.configure(result_obj(total_cost_usd=cost))
                self.assert_fails("total_cost_usd")
        obj = result_obj()
        del obj["total_cost_usd"]
        self.configure(obj)
        self.assert_fails("total_cost_usd")

    def test_missing_usage_and_session(self):
        self.configure(result_obj(usage=None))
        self.assert_fails("usage")
        self.configure(result_obj(session_id=""))
        self.assert_fails("session_id")

    def test_non_json_and_empty_stdout(self):
        for stdout in ("", "hello", "[]", '{"type": "assistant"}', "{} {}"):
            with self.subTest(stdout=stdout):
                self.configure(stdout=stdout)
                self.assert_fails("no JSON result")

    def test_duplicate_keys_in_output(self):
        text = json.dumps(result_obj())[:-1] + ', "is_error": true}'
        self.configure(stdout=text)
        self.assert_fails("no JSON result.*duplicate")

    def test_bad_proposal_rejected_end_to_end(self):
        self.configure(result_obj(structured_output=good_proposal(extra=1)))
        exc = self.assert_fails("invalid proposal")
        self.assertEqual(exc.cost_usd, 0.0421)
        self.configure(result_obj(structured_output=None, result="I changed the files."))
        self.assert_fails("not a JSON proposal")
        self.configure(result_obj(structured_output=None, result=""))
        self.assert_fails("neither structured_output")
        self.configure(result_obj(structured_output=["x"]))
        self.assert_fails("structured_output is not an object")

    def test_stdout_flood_is_bounded(self):
        self.client.max_stdout_bytes = 4096
        self.configure(flood=100_000)
        start = time.monotonic()
        self.assert_fails("stdout exceeded")
        self.assertLess(time.monotonic() - start, 10)
        rec = self.record()
        self.assertTrue(wait_gone(rec["pid"], rec["pid"]))


class ProcessTests(FakeClaudeCase):
    def setUp(self):
        super().setUp()
        # An unrelated process the provider must never signal.
        self.bystander = subprocess.Popen(
            [sys.executable, "-c", "import time; time.sleep(120)"],
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        self.addCleanup(self._stop_bystander)

    def _stop_bystander(self):
        if self.bystander.poll() is None:
            self.bystander.kill()
        self.bystander.wait()

    def assert_bystander_alive(self):
        self.assertIsNone(self.bystander.poll())

    def test_timeout_kills_leader_and_stubborn_child(self):
        self.configure(sleep=60, ignore_term=True, child="stubborn")
        start = time.monotonic()
        self.assert_fails("timed out", timeout=2.0)
        elapsed = time.monotonic() - start
        self.assertLess(elapsed, 10)
        rec = self.record()
        self.assertTrue(wait_gone(rec["pid"], rec["sid"]))
        self.assertTrue(wait_gone(rec["child_pid"], rec["sid"]))
        meta = json.loads((self.out / "request.json").read_text())
        self.assertTrue(meta["timed_out"])
        self.assertEqual(meta["returncode"], -signal.SIGKILL)
        self.assert_bystander_alive()

    def test_orphan_child_reaped_after_success(self):
        self.configure(child="stubborn")
        result = self.run_generate()
        self.assertEqual(result["cost_usd"], 0.0421)
        rec = self.record()
        self.assertTrue(wait_gone(rec["child_pid"], rec["sid"]))
        meta = json.loads((self.out / "request.json").read_text())
        self.assertEqual(meta["session_members_signalled"], 1)
        self.assert_bystander_alive()

    def test_heartbeat_called_during_wait(self):
        self.client.heartbeat_interval = 0.05
        beats = []
        self.configure(sleep=0.6)
        self.run_generate(heartbeat=lambda: beats.append(time.monotonic()))
        self.assertGreaterEqual(len(beats), 3)

    def test_interrupt_from_heartbeat_stops_session(self):
        self.client.heartbeat_interval = 0.05

        def interrupt():
            if self.record_path.exists():
                raise KeyboardInterrupt

        self.configure(sleep=60, child="plain")
        with self.assertRaises(KeyboardInterrupt):
            self.run_generate(heartbeat=interrupt)
        rec = self.record()
        self.assertTrue(wait_gone(rec["pid"], rec["sid"]))
        self.assertTrue(wait_gone(rec["child_pid"], rec["sid"]))
        meta = json.loads((self.out / "request.json").read_text())
        self.assertEqual(meta["status"], "interrupted")
        self.assert_bystander_alive()

    def test_no_descriptor_or_handle_leaks(self):
        self.configure()
        self.run_generate()  # warm up lazy imports
        before = sorted(os.listdir("/proc/self/fd"))
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            self.run_generate()
            self.configure(exit=3)
            with self.assertRaises(ClaudeError):
                self.run_generate()
            self.configure(sleep=60)
            with self.assertRaises(ClaudeError):
                self.run_generate(timeout=0.3)
            gc.collect()
        self.assertEqual(sorted(os.listdir("/proc/self/fd")), before)
        self.assertEqual([w for w in caught if issubclass(w.category, ResourceWarning)], [])


class ValidateProposalTests(unittest.TestCase):
    def test_valid(self):
        self.assertEqual(validate_proposal(good_proposal()), good_proposal())
        self.assertEqual(validate_proposal(good_proposal(changes=[]))["changes"], [])

    def assert_invalid(self, proposal, pattern="invalid proposal"):
        with self.assertRaisesRegex(ClaudeError, pattern):
            validate_proposal(proposal)

    def change(self, **overrides):
        change = {"path": "a/b.txt", "content": "x", "base_sha256": None}
        change.update(overrides)
        return change

    def test_top_level_shape(self):
        self.assert_invalid([], "object")
        self.assert_invalid(good_proposal(extra="x"), "extra")
        proposal = good_proposal()
        del proposal["notes"]
        self.assert_invalid(proposal, "missing")
        for summary in ("", "   ", None, 3):
            self.assert_invalid(good_proposal(summary=summary), "summary")
        self.assert_invalid(good_proposal(notes=None), "notes")
        self.assert_invalid(good_proposal(changes={}), "changes")

    def test_change_shape(self):
        self.assert_invalid(good_proposal(changes=["a"]), r"changes\[0\]")
        self.assert_invalid(good_proposal(changes=[self.change(mode="755")]), "extra")
        change = self.change()
        del change["base_sha256"]
        self.assert_invalid(good_proposal(changes=[change]), "missing")
        self.assert_invalid(good_proposal(changes=[self.change(content=b"x")]), "content")
        self.assert_invalid(good_proposal(changes=[self.change(content=None)]), "content")
        self.assert_invalid(good_proposal(changes=[self.change(content="\ud800")]), "UTF-8")

    def test_paths(self):
        for path in ("", "/etc/passwd", "../x", "a/../b", "./a", "a/./b", "a//b", "a/",
                     "a\\b", "C:/x", "c:x", "a\nb", "a\x00b", None, 5):
            with self.subTest(path=path):
                self.assert_invalid(good_proposal(changes=[self.change(path=path)]), "path")
        ok = validate_proposal(good_proposal(changes=[self.change(path=".github/x y.yml")]))
        self.assertEqual(ok["changes"][0]["path"], ".github/x y.yml")

    def test_duplicate_paths(self):
        changes = [self.change(), self.change(content="y")]
        self.assert_invalid(good_proposal(changes=changes), "duplicate")

    def test_base_sha256(self):
        for base in (SHA.upper(), SHA[:-1], SHA + "0", "g" * 64, 0, "", False):
            with self.subTest(base=base):
                self.assert_invalid(good_proposal(changes=[self.change(base_sha256=base)]),
                                    "base_sha256")
        ok = validate_proposal(good_proposal(changes=[self.change(base_sha256=SHA)]))
        self.assertEqual(ok["changes"][0]["base_sha256"], SHA)

    def test_aggregate_size(self):
        half = "a" * (MAX_PROPOSAL_BYTES // 2)
        changes = [self.change(path="a", content=half), self.change(path="b", content=half)]
        self.assert_invalid(good_proposal(changes=changes), "aggregate")
        # Multibyte characters count as UTF-8 bytes, not code points.
        wide = "\u00e6" * (MAX_PROPOSAL_BYTES // 2 + 1)
        self.assert_invalid(good_proposal(changes=[self.change(content=wide)]), "aggregate")
        fits = "a" * (MAX_PROPOSAL_BYTES - 1000)
        validate_proposal(good_proposal(changes=[self.change(content=fits)]))


if __name__ == "__main__":
    unittest.main()
