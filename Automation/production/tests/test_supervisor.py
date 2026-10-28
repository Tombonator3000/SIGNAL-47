"""Regression tests for adopted processes which escape polling-only ownership."""

import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

from Automation.production.models import ProductionError
from Automation.production import process as execution
from Automation.production.store import process_identity


REPO = Path(__file__).resolve().parents[3]


class SupervisorTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.log = self.root / "command.log"

    def execute(self, code, **options):
        return execution.run_command([sys.executable, "-c", code],
                                     cwd=self.root, log_path=self.log,
                                     timeout=options.pop("timeout", 5), **options)

    def wait_for(self, path):
        deadline = time.monotonic() + 5
        while not path.exists() and time.monotonic() < deadline:
            time.sleep(.01)
        self.assertTrue(path.exists(), f"Fixture did not start: {path}")

    def assert_reaped(self, pid):
        deadline = time.monotonic() + 3
        while Path(f"/proc/{pid}").exists() and time.monotonic() < deadline:
            time.sleep(.01)
        self.assertFalse(Path(f"/proc/{pid}").exists(), f"Owned process {pid} was not reaped")

    def cleanup_fixture(self, path):
        # Emergency cleanup targets only PIDs written by our synthetic fixture,
        # and keeps their Linux start identity fenced before signalling.
        if not path.exists():
            return
        for value in path.read_text().split():
            pid = int(value)
            identity = process_identity(pid)
            if identity and process_identity(pid) == identity:
                try:
                    os.kill(pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass

    def test_immediate_wrapper_exit_reaps_setsid_child(self):
        pidfile = self.root / "detached.pid"
        self.addCleanup(self.cleanup_fixture, pidfile)
        code = (
            "import subprocess,sys; from pathlib import Path; "
            "p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'], "
            "stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,start_new_session=True); "
            "Path('detached.pid').write_text(str(p.pid))"
        )
        with self.assertRaisesRegex(ProductionError, "background descendants"):
            self.execute(code)
        self.assert_reaped(int(pidfile.read_text()))

    def test_double_fork_and_new_session_are_adopted_and_reaped(self):
        pidfile = self.root / "double.pid"
        self.addCleanup(self.cleanup_fixture, pidfile)
        code = """import os,time
from pathlib import Path
reader,writer=os.pipe()
first=os.fork()
if first==0:
    if os.fork():
        os._exit(0)
    os.setsid()
    Path('double.pid').write_text(str(os.getpid()))
    os.write(writer,b'1')
    time.sleep(30)
    os._exit(0)
os.close(writer)
os.read(reader,1)
os._exit(0)
"""
        with self.assertRaisesRegex(ProductionError, "background descendants"):
            self.execute(code)
        self.assert_reaped(int(pidfile.read_text()))
        meta = json.loads(next(self.root.glob("command.log.supervisor-*.json")).read_text())
        self.assertGreaterEqual(meta["reaped"], 3)
        self.assertEqual(meta["remaining"], [])

    def test_sigterm_handler_cannot_leave_a_new_detached_child(self):
        pidfile = self.root / "late.pid"
        self.addCleanup(self.cleanup_fixture, pidfile)
        code = """import signal,subprocess,sys,time
from pathlib import Path
def terminate(*args):
    p=subprocess.Popen([sys.executable,'-c','import signal,time; signal.signal(signal.SIGTERM,signal.SIG_IGN); time.sleep(30)'],
                       stdin=subprocess.DEVNULL,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,start_new_session=True)
    Path('late.pid').write_text(str(p.pid))
    sys.exit(0)
signal.signal(signal.SIGTERM,terminate)
time.sleep(30)
"""
        with self.assertRaisesRegex(ProductionError, "timeout"):
            self.execute(code, timeout=.25)
        self.assertTrue(pidfile.exists(), "Fixture SIGTERM handler did not run")
        self.assert_reaped(int(pidfile.read_text()))

    def test_stubborn_descendant_is_killed_then_reaped(self):
        pidfile = self.root / "stubborn.pid"
        self.addCleanup(self.cleanup_fixture, pidfile)
        code = """import os,signal,time
from pathlib import Path
signal.signal(signal.SIGTERM,signal.SIG_IGN)
Path('stubborn.pid').write_text(str(os.getpid()))
time.sleep(30)
"""
        started = time.monotonic()
        with self.assertRaisesRegex(ProductionError, "timeout"):
            self.execute(code, timeout=.2)
        self.assertLess(time.monotonic() - started, 3)
        self.assert_reaped(int(pidfile.read_text()))

    def test_fast_exit_still_enforces_log_size_bound(self):
        with mock.patch.object(execution, "MAX_LOG_BYTES", 1024):
            with self.assertRaisesRegex(ProductionError, "log size"):
                self.execute("import os; os.ftruncate(1,2048)")
        self.assertEqual(self.log.stat().st_size, 2048)

    def test_tiny_deadline_cannot_pass_during_supervisor_startup(self):
        with self.assertRaisesRegex(ProductionError, "timeout"):
            self.execute("pass", timeout=.000001)

    def test_actual_signal_exit_is_preserved(self):
        result = self.execute("import os,signal; os.kill(os.getpid(),signal.SIGUSR1)")
        self.assertEqual(result["returncode"], -signal.SIGUSR1)
        self.assertIsNone(result["supervision"]["stop_reason"])

    def test_guard_ownership_proof_handles_child_appearing_after_snapshot(self):
        pidfile = self.root / "actual-leader.pid"
        calls = []

        class Guard:
            @staticmethod
            def heavy_jobs():
                return [{"pid": int(pidfile.read_text())}] if pidfile.exists() else []

            @staticmethod
            def owned_process(pid, owner):
                calls.append((pid, owner))
                return execution._belongs_to(pid, owner)

            @staticmethod
            def available_gib():
                return 32

        with mock.patch.object(execution, "_children", return_value={}):
            result = self.execute(
                "import os,time; from pathlib import Path; Path('actual-leader.pid').write_text(str(os.getpid())); time.sleep(.2)",
                guard=Guard())
        self.assertEqual(result["returncode"], 0)
        self.assertTrue(calls, "Guard's current ownership proof was not consulted")

    def coordinator(self):
        child = self.root / "family.pid"
        self.addCleanup(self.cleanup_fixture, child)
        command_code = (
            "import os,subprocess,sys,time; from pathlib import Path; "
            "p=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'],start_new_session=True); "
            "Path('family.pid').write_text(str(os.getpid())+' '+str(p.pid)); time.sleep(30)"
        )
        runner = (
            "import sys; from pathlib import Path; "
            f"sys.path.insert(0,{str(REPO)!r}); "
            "from Automation.production.process import run_command; "
            f"run_command([sys.executable,'-c',{command_code!r}],cwd=Path({str(self.root)!r}),"
            f"log_path=Path({str(self.log)!r}),timeout=20)"
        )
        process = subprocess.Popen([sys.executable, "-c", runner],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   start_new_session=True)
        self.addCleanup(lambda: process.wait(timeout=3) if process.poll() is not None else self.stop_fixture_coordinator(process))
        self.wait_for(child)
        return process, child

    @staticmethod
    def stop_fixture_coordinator(process):
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()  # Emergency stop of this test's own coordinator only.
            process.wait(timeout=3)

    def test_coordinator_sigterm_cleans_owned_family_and_preserves_foreign_process(self):
        unrelated = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"], start_new_session=True)
        try:
            process, path = self.coordinator()
            process.terminate()
            process.wait(timeout=5)
            for pid in map(int, path.read_text().split()):
                self.assert_reaped(pid)
            self.assertIsNone(unrelated.poll())
        finally:
            unrelated.terminate()
            unrelated.wait(timeout=3)

    def test_coordinator_sigkill_triggers_supervisor_parent_death_cleanup(self):
        process, path = self.coordinator()
        process.kill()
        process.wait(timeout=3)
        for pid in map(int, path.read_text().split()):
            self.assert_reaped(pid)
        deadline = time.monotonic() + 3
        while not list(self.root.glob("command.log.supervisor-*.json")) and time.monotonic() < deadline:
            time.sleep(.01)
        meta = json.loads(next(self.root.glob("command.log.supervisor-*.json")).read_text())
        self.assertEqual(meta["remaining"], [])
        self.assertIn("signal", meta["stop_reason"])


if __name__ == "__main__":
    unittest.main()
