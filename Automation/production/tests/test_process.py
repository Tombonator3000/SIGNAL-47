"""Bounded subprocess execution and cleanup of only the owned process tree."""

import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

from Automation.production.models import ProductionError
from Automation.production.process import run_command
from Automation.production.store import process_identity


class ProcessTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.log = self.root / "command.log"

    def run_code(self, code, **options):
        arguments = {"cwd": self.root, "log_path": self.log, "timeout": 5}
        arguments.update(options)
        return run_command([sys.executable, "-c", code], **arguments)

    def assert_not_live(self, pid):
        deadline = time.monotonic() + 2
        while process_identity(pid) and time.monotonic() < deadline:
            time.sleep(0.01)
        self.assertFalse(process_identity(pid), f"Owned child {pid} is still alive")

    def test_success_records_real_log_hash_and_heartbeat(self):
        heartbeats = []
        result = self.run_code(
            "import os,sys,time; print(os.environ['FIXTURE_VALUE']); print('stderr marker', file=sys.stderr); time.sleep(.15)",
            env={**os.environ, "FIXTURE_VALUE": "stdout marker"}, heartbeat=lambda: heartbeats.append(time.monotonic()),
        )
        self.assertEqual(result["returncode"], 0)
        content = self.log.read_bytes()
        self.assertIn(b"stdout marker", content)
        self.assertIn(b"stderr marker", content)
        self.assertEqual(result["log_sha256"], hashlib.sha256(content).hexdigest())
        self.assertEqual(result["log"], str(self.log))
        self.assertTrue(heartbeats)

    def test_nonzero_return_is_preserved_for_the_adapter_to_fail(self):
        result = self.run_code("import sys; print('controlled failure'); sys.exit(7)")
        self.assertEqual(result["returncode"], 7)
        self.assertIn("controlled failure", self.log.read_text())

    def test_arguments_are_not_interpreted_by_a_shell(self):
        marker = self.root / "should-not-exist"
        text = "$(touch " + str(marker) + ") ; echo injection"
        result = run_command([sys.executable, "-c", "import sys; print(sys.argv[1])", text],
                             cwd=self.root, log_path=self.log, timeout=5)
        self.assertEqual(result["returncode"], 0)
        self.assertIn(text, self.log.read_text())
        self.assertFalse(marker.exists())

    def test_timeout_stops_owned_parent_and_child_but_preserves_unrelated_process(self):
        unrelated = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"], start_new_session=True)
        try:
            code = (
                "import os,subprocess,sys,time; from pathlib import Path; "
                "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)'], start_new_session=True); "
                "Path('owned-pids.txt').write_text(str(os.getpid())+' '+str(child.pid)); time.sleep(30)"
            )
            with self.assertRaisesRegex(ProductionError, "timeout"):
                self.run_code(code, timeout=0.5)
            parents = [int(value) for value in (self.root / "owned-pids.txt").read_text().split()]
            for pid in parents:
                self.assert_not_live(pid)
            self.assertIsNone(unrelated.poll(), "Unrelated process was signalled")
        finally:
            unrelated.terminate()
            unrelated.wait(timeout=3)

    def test_background_descendant_after_parent_exit_cannot_pass(self):
        code = (
            "import subprocess,sys,time; from pathlib import Path; "
            "child=subprocess.Popen([sys.executable,'-c','import time; time.sleep(30)']); "
            "Path('background-pid.txt').write_text(str(child.pid)); time.sleep(.2)"
        )
        with self.assertRaisesRegex(ProductionError, "background descendants"):
            self.run_code(code)
        self.assert_not_live(int((self.root / "background-pid.txt").read_text()))

    def test_guard_contention_stops_only_owned_command(self):
        unrelated = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(30)"], start_new_session=True)
        class Guard:
            @staticmethod
            def heavy_jobs():
                return [{"pid": unrelated.pid}]

            @staticmethod
            def available_gib():
                return 32
        try:
            with self.assertRaisesRegex(ProductionError, "Another heavy process"):
                self.run_code("import time; time.sleep(30)", guard=Guard())
            self.assertIsNone(unrelated.poll(), "Guard cleanup signalled an unrelated process")
        finally:
            unrelated.terminate()
            unrelated.wait(timeout=3)

    def test_heartbeat_error_aborts_owned_command_and_propagates(self):
        def heartbeat():
            raise InterruptedError("Lost fixture ownership")
        with self.assertRaisesRegex(InterruptedError, "Lost fixture ownership"):
            self.run_code("import time; time.sleep(30)", heartbeat=heartbeat)

    def test_guard_low_memory_stops_the_owned_command(self):
        class Guard:
            @staticmethod
            def heavy_jobs():
                return []

            @staticmethod
            def available_gib():
                return 1
        with self.assertRaisesRegex(ProductionError, "memory below"):
            self.run_code("import time; time.sleep(30)", guard=Guard())


if __name__ == "__main__":
    unittest.main()
