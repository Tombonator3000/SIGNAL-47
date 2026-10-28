"""Real process tests for the whole-series reservation, without engine jobs."""

from contextlib import contextmanager
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[3]
SCRIPTS = ROOT / "unreal/Signal47/Scripts"


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


lease_module = load_module("s47_test_lease", SCRIPTS / "production_lease.py")
guard_module = load_module("s47_test_guard", SCRIPTS / "unreal_guard.py")
HeavyReservation = lease_module.HeavyReservation


class LeaseTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.lock = Path(self.temporary.name) / "heavy.lock"
        self.clean_environment = os.environ.copy()
        self.clean_environment[lease_module.LOCK_PATH_ENV] = str(self.lock)
        self.clean_environment.pop(lease_module.TOKEN_ENV, None)
        self.prefix = f"import sys; sys.path.insert(0, {str(SCRIPTS)!r}); import production_lease as p; "

    def child(self, code, *, environment=None):
        return subprocess.run([sys.executable, "-c", self.prefix + code],
                              env=environment or self.clean_environment,
                              text=True, capture_output=True, timeout=5)

    @contextmanager
    def holder(self):
        code = (self.prefix +
                "r=p.HeavyReservation('independent job').acquire(); "
                "import json, os; print(json.dumps(r.environment()), flush=True); "
                "answer=input(); os._exit(0) if answer=='crash' else r.release()")
        process = subprocess.Popen([sys.executable, "-c", code],
                                   env=self.clean_environment, text=True,
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        try:
            environment = json.loads(process.stdout.readline())
            yield process, self.clean_environment | environment
        finally:
            if process.poll() is None:
                try:
                    process.communicate("stop\n", timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()  # Only this test's own fake holder.
                    process.communicate(timeout=5)
            for stream in (process.stdin, process.stdout, process.stderr):
                if stream and not stream.closed:
                    stream.close()

    def test_independent_worker_cannot_acquire_held_lock(self):
        with self.holder():
            with self.assertRaises(lease_module.ReservationBusy):
                HeavyReservation("second job", self.lock).acquire()
            result = self.child("p.HeavyReservation('another process').acquire()")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Another heavy production job", result.stderr)
        with HeavyReservation("after release", self.lock):
            pass

    def test_descendant_with_inherited_token_is_allowed_in_new_session(self):
        with HeavyReservation("parent", self.lock) as reservation:
            environment = self.clean_environment | reservation.environment()
            result = subprocess.run(
                [sys.executable, "-c", self.prefix +
                 "import json; print(json.dumps(p.assert_reservation_access()))"],
                env=environment, text=True, capture_output=True, timeout=5,
                start_new_session=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(result.stdout)
            self.assertEqual(data["pid"], os.getpid())
            self.assertNotIn("token", data)
            self.assertEqual(self.lock.stat().st_mode & 0o777, 0o600)

    def test_descendant_without_token_is_blocked(self):
        with HeavyReservation("parent", self.lock):
            result = self.child("p.assert_reservation_access()")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("no process was stopped", result.stderr)

    def test_leaked_token_does_not_allow_unrelated_sibling(self):
        with self.holder() as (_, environment):
            result = self.child("p.assert_reservation_access()", environment=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("reservation held", result.stderr)

    def test_process_gap_keeps_reservation_until_whole_series_finishes(self):
        with HeavyReservation("two-tool series", self.lock) as reservation:
            environment = self.clean_environment | reservation.environment()
            for _ in range(2):
                result = self.child("p.assert_reservation_access()", environment=environment)
                self.assertEqual(result.returncode, 0, result.stderr)
                # No child process remains in this gap. A competing scheduler
                # still must not take the build/test window.
                result = self.child("p.HeavyReservation('gap intruder').acquire()")
                self.assertNotEqual(result.returncode, 0)
        result = self.child("p.HeavyReservation('next series').acquire()")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_crash_releases_os_lock_without_trusting_stale_metadata(self):
        with self.holder() as (process, _):
            process.communicate("crash\n", timeout=5)
            self.assertEqual(process.returncode, 0)
        self.assertTrue(self.lock.exists())
        with mock.patch.dict(os.environ, self.clean_environment, clear=True):
            self.assertIsNone(lease_module.assert_reservation_access())
        with HeavyReservation("recovered after crash", self.lock):
            pass

    def test_exception_releases_and_explicit_release_is_idempotent(self):
        reservation = HeavyReservation("failure", self.lock)
        with self.assertRaisesRegex(ValueError, "job failed"):
            with reservation:
                raise ValueError("job failed")
        reservation.release()
        with HeavyReservation("retry", self.lock):
            pass
        with self.assertRaises(RuntimeError):
            reservation.environment()

    def test_invalid_metadata_is_fail_closed_only_while_lock_is_held(self):
        with HeavyReservation("bad metadata", self.lock) as reservation:
            self.lock.write_text("not JSON")
            environment = self.clean_environment | reservation.environment()
            with mock.patch.dict(os.environ, environment, clear=True):
                with self.assertRaises(lease_module.ReservationBusy):
                    lease_module.assert_reservation_access()
        with mock.patch.dict(os.environ, self.clean_environment, clear=True):
            self.assertIsNone(lease_module.assert_reservation_access())

    def test_pid_reuse_or_wrong_boot_metadata_cannot_authorize(self):
        for key, value in (("start_ticks", "0"), ("boot_id", "another boot")):
            with self.subTest(key=key):
                with HeavyReservation("wrong identity", self.lock) as reservation:
                    data = json.loads(self.lock.read_text())
                    data[key] = value
                    self.lock.write_text(json.dumps(data))
                    with mock.patch.dict(os.environ, self.clean_environment | reservation.environment(), clear=True):
                        with self.assertRaises(lease_module.ReservationBusy):
                            lease_module.assert_reservation_access()

    def test_oversized_metadata_and_non_ascii_token_fail_closed(self):
        with HeavyReservation("metadata limits", self.lock) as reservation:
            environment = self.clean_environment | reservation.environment()
            environment[lease_module.TOKEN_ENV] = "ø" * 64
            with mock.patch.dict(os.environ, environment, clear=True):
                with self.assertRaises(lease_module.ReservationBusy):
                    lease_module.assert_reservation_access()
            self.lock.write_text("x" * (lease_module.MAX_METADATA_BYTES + 1))
            with mock.patch.dict(os.environ, environment, clear=True):
                with self.assertRaises(lease_module.ReservationBusy):
                    lease_module.assert_reservation_access()

    def test_symlink_and_hardlink_lock_files_are_rejected(self):
        target = self.lock.parent / "do-not-touch"
        target.write_text("preserve")
        self.lock.symlink_to(target)
        with self.assertRaises(RuntimeError):
            HeavyReservation("symlink", self.lock).acquire()
        self.assertEqual(target.read_text(), "preserve")
        self.lock.unlink()
        os.link(target, self.lock)
        with self.assertRaises(RuntimeError):
            HeavyReservation("hardlink", self.lock).acquire()
        self.assertEqual(target.read_text(), "preserve")

    def test_symlink_parent_is_rejected(self):
        original = self.lock.parent / "real"
        original.mkdir()
        link = self.lock.parent / "linked"
        link.symlink_to(original, target_is_directory=True)
        with self.assertRaises(RuntimeError):
            HeavyReservation("parent symlink", link / "heavy.lock").acquire()

    def test_guard_checks_reservation_before_launching_any_probe(self):
        with self.holder():
            with mock.patch.dict(os.environ, self.clean_environment, clear=True):
                with mock.patch.object(guard_module.subprocess, "run", side_effect=AssertionError("probe was launched")):
                    with self.assertRaises(RuntimeError) as caught:
                        guard_module.require_idle()
            self.assertIn("reservation held", str(caught.exception))

    def test_legacy_copied_guard_keeps_process_checks_without_new_sibling(self):
        copied = self.lock.parent / "unreal_guard.py"
        copied.write_bytes((SCRIPTS / "unreal_guard.py").read_bytes())
        legacy = load_module("s47_legacy_guard_test", copied)
        with self.holder():
            with mock.patch.dict(os.environ, self.clean_environment, clear=True):
                self.assertIsNone(legacy.reservation_access())

    def test_blender_classification_uses_executable_not_prompt_text(self):
        self.assertTrue(guard_module.heavy_command("/opt/blender-5.2/blender", [b"blender", b"-b"]))
        self.assertTrue(guard_module.heavy_command("C:/Blender/blender.exe", [b"blender.exe"]))
        self.assertFalse(guard_module.heavy_command("/usr/bin/python3", [b"python3", b"script.py", b"blender"]))
        self.assertFalse(guard_module.heavy_command("/bin/bash", [b"bash", b"-c", b"echo blender"]))
        self.assertTrue(guard_module.heavy_command("/bin/bash", [b"bash", b"/Engine/RunUAT.sh"]))


if __name__ == "__main__":
    unittest.main()
