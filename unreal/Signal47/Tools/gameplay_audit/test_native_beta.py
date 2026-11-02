"""No engine, X display connection, native input or device creation in these tests."""
import json
from pathlib import Path
import socket
import tempfile
import unittest
from unittest.mock import patch, Mock
import native_beta as beta


class NativeBetaTests(unittest.TestCase):
    def test_key_hold_is_bounded_and_names_known(self):
        self.assertEqual(beta.validate_action({"action": "tap", "key": "Tab"})["seconds"], .08)
        for value in (0, 6, float("nan"), True):
            with self.assertRaises(ValueError): beta.validate_action({"action": "hold", "key": "w", "seconds": value})
        with self.assertRaises(ValueError): beta.validate_action({"action": "tap", "key": "not-a-key"})

    def test_comma_aliases_validate_and_emit_linux_key_comma(self):
        for alias in (",", "comma"):
            with self.subTest(alias=alias):
                action = beta.validate_action({"action": "tap", "key": alias})
                desktop = object.__new__(beta.Desktop)
                desktop.held, desktop.pressed = set(), set()
                desktop.guard = lambda: None
                events = []
                desktop.emit = lambda device, values: events.extend((device, *event) for event in values)
                with patch.object(beta.time, "monotonic", side_effect=[0, 1]):
                    desktop.perform(action)
                self.assertEqual(events, [(0, 1, 51, 1), (0, 1, 51, 0)])
                self.assertFalse(desktop.held)

    def test_chord_validation_rejects_ambiguous_or_duplicate_keys(self):
        valid = beta.validate_action({"action": "hold", "keys": ["Shift", "w"], "seconds": 5})
        self.assertEqual(valid["keys"], ["Shift", "w"])
        for keys in ([], ["w"] * 2, [",", "comma"], ["w", "a", "Shift", "Space"], "wa", ["unknown"], [3]):
            with self.subTest(keys=keys), self.assertRaises(ValueError):
                beta.validate_action({"action": "hold", "keys": keys})
        with self.assertRaises(ValueError):
            beta.validate_action({"action": "tap", "key": "w", "keys": ["a"]})
        with self.assertRaises(ValueError):
            beta.validate_action({"action": "hold", "keys": ["w", "a"], "seconds": 5.1})

    def test_chord_releases_all_on_success_focus_loss_and_partial_write(self):
        for failure in (None, "focus", "write"):
            with self.subTest(failure=failure):
                desktop = object.__new__(beta.Desktop)
                desktop.held, desktop.pressed = set(), set()
                events, guards = [], []
                def guard():
                    guards.append(True)
                    if failure == "focus" and len(guards) > 1:
                        raise RuntimeError("focus lost")
                def emit(device, values):
                    for event in values:
                        events.append(event)
                        if failure == "write" and event[2] == 1:
                            raise OSError("partial device write")
                desktop.guard, desktop.emit = guard, emit
                ticks = [0, 1] if failure is None else [0, 0, 0]
                with patch.object(beta.time, "monotonic", side_effect=ticks), patch.object(beta.time, "sleep"):
                    if failure:
                        with self.assertRaises((RuntimeError, OSError)):
                            desktop.chord(["Shift", "w", "a"], .1)
                    else:
                        desktop.chord(["Shift", "w", "a"], .1)
                self.assertEqual({e[1] for e in events if e[2] == 0}, {42, 17, 30})
                if failure != "write": self.assertEqual(events[:3], [(1, 42, 1), (1, 17, 1), (1, 30, 1)])
                self.assertFalse(desktop.held)

    def test_resume_profile_is_bounded_copied_and_source_unchanged(self):
        with tempfile.TemporaryDirectory(prefix="s47-beta-fixture-", dir="/tmp") as tmp:
            prior = Path(tmp); source = prior / "profile"; source.mkdir()
            (source / "case.json").write_text('{"fixture":true}')
            (prior / "session.json").write_text(json.dumps({"runtime": "EXECUTED", "output": str(prior), "command": ["-UserDir=" + str(source)]}))
            self.assertEqual(beta.validate_profile_source(source), source)
            binary = prior / "Linux/Signal47/Binaries/Linux/Signal47"
            binary.parent.mkdir(parents=True); binary.write_bytes(b"fixture")
            destination = prior / "next-session"
            plan = beta.session_plan(prior, prior / "Linux", destination, 60, 1280, 720, source)
            self.assertEqual(plan["profile_mode"], "resumed")
            self.assertEqual(plan["profile_source"], str(source))
            self.assertEqual(plan["runtime"], "NOT_RUN")
            self.assertFalse(destination.exists())
            destination.mkdir()
            with self.assertRaises(ValueError):
                beta.session_plan(prior, prior / "Linux", destination, 60, 1280, 720, source)
            with tempfile.TemporaryDirectory() as outtmp:
                out = Path(outtmp)
                beta.prepare_profile(out, source)
                self.assertEqual((out / "profile/case.json").read_text(), '{"fixture":true}')
                (out / "profile/case.json").write_text("changed copy")
                self.assertEqual((source / "case.json").read_text(), '{"fixture":true}')
                with self.assertRaises(FileExistsError): beta.prepare_profile(out, source)
                (source / "linked").symlink_to(source / "case.json")
                with self.assertRaises(ValueError): beta.validate_profile_source(source)
                (source / "linked").unlink()
                (prior / "session.json").write_text('{}')
                with self.assertRaises(ValueError): beta.validate_profile_source(source)
        with self.assertRaises(ValueError): beta.validate_profile_source(Path.home() / "Saved")

    def xtest_desktop(self):
        desktop = object.__new__(beta.Desktop)
        desktop.mouse_backend = "xtest"
        desktop.xtest = Mock()
        desktop.x = Mock()
        desktop.display, desktop.window = 1, 2
        desktop.emitted_events = 0
        desktop.held, desktop.pressed = set(), set()
        desktop.guard = Mock()
        return desktop

    def test_xtest_motion_buttons_wheel_never_use_uinput_mouse(self):
        desktop = self.xtest_desktop()
        with patch.object(beta.os, "write") as write:
            desktop.emit(1, [(2, 0, 12), (2, 1, -5)])
            desktop.emit(1, [(1, 273, 1), (1, 273, 0)])
            desktop.emit(1, [(2, 8, -2)])
            write.assert_not_called()
        desktop.xtest.XTestFakeRelativeMotionEvent.assert_called_once_with(1, 12, -5, 0)
        self.assertEqual([c.args[1:3] for c in desktop.xtest.XTestFakeButtonEvent.call_args_list],
                         [(3, 1), (3, 0), (5, 1), (5, 0), (5, 1), (5, 0)])

    def test_xtest_click_focus_failure_releases_held_button(self):
        desktop = self.xtest_desktop()
        desktop.pointer = Mock()
        desktop.guard.side_effect = [None, None, RuntimeError("focus lost")]
        with patch.object(beta.time, "sleep"):
            with self.assertRaisesRegex(RuntimeError, "focus lost"):
                desktop.perform({"action": "click", "x": 4, "y": 6, "button": "left"})
        self.assertEqual([c.args[1:3] for c in desktop.xtest.XTestFakeButtonEvent.call_args_list], [(1, 1), (1, 0)])
        self.assertFalse(desktop.pressed)

    def test_xtest_pointer_is_client_bounded_and_verified(self):
        desktop = self.xtest_desktop()
        def geometry(display, window, root, x, y, width, height, border, depth):
            width._obj.value, height._obj.value = 1280, 720
            return 1
        def pointer(display, window, root, child, rx, ry, x, y, mask):
            x._obj.value, y._obj.value = 640, 360
            return 1
        desktop.x.XGetGeometry.side_effect = geometry
        desktop.x.XQueryPointer.side_effect = pointer
        desktop.pointer(640, 360)
        desktop.x.XWarpPointer.assert_called_once_with(1, 0, 2, 0, 0, 0, 0, 640, 360)
        desktop.xtest.XTestFakeRelativeMotionEvent.assert_not_called()
        with self.assertRaises(ValueError): desktop.pointer(1280, 360)
        self.assertEqual(desktop.x.XWarpPointer.call_count, 1)
        with self.assertRaisesRegex(RuntimeError, "did not reach"):
            desktop.pointer(100, 100)

    def test_missing_xtest_is_explicit_without_install(self):
        with patch.object(beta.ctypes.util, "find_library", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "existing libXtst"):
                beta.load_xtest()

    def test_actions_cannot_address_state_or_arbitrary_files(self):
        with self.assertRaises(ValueError): beta.validate_action({"action": "teleport", "x": 0})
        with self.assertRaises(ValueError): beta.validate_action({"action": "screenshot", "name": "../../save"})
        with self.assertRaises(ValueError): beta.validate_action({"action": "click", "x": 0, "y": 0, "button": []})
        result = beta.validate_action({"action": "observe", "note": "visible prompt", "hidden_state": {"stage": 7}})
        self.assertNotIn("hidden_state", result)

    def test_plan_is_not_run_and_forces_isolated_profile(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); binary = root / "Linux/Signal47/Binaries/Linux/Signal47"
            binary.parent.mkdir(parents=True); binary.write_bytes(b"fixture")
            out = root / "session"
            plan = beta.session_plan(root, root / "Linux", out, 60, 1280, 720)
            self.assertEqual(plan["runtime"], "NOT_RUN")
            self.assertEqual(plan["native_input"], "NOT_RUN")
            self.assertIn("-UserDir=" + str(out / "profile"), plan["command"])
            self.assertFalse(out.exists())
            self.assertFalse(any(s.startswith(("-S47Jump", "-S47Face", "-S47FreeformTest")) for s in plan["command"]))

    def test_lock_fail_closed(self):
        for output in ("true", "unexpected", ""):
            with patch.object(beta.subprocess, "run") as run:
                run.return_value.returncode = 0; run.return_value.stdout = output
                with self.assertRaises(RuntimeError): beta.unlocked()

    def test_release_sends_only_keyups_and_buttonups(self):
        desktop = object.__new__(beta.Desktop)
        desktop.held = {"w", "Tab"}; desktop.pressed = {272}
        events = []
        desktop.emit = lambda device, values: events.extend((device, *event) for event in values)
        desktop.release()
        self.assertEqual(len(events), 3)
        self.assertTrue(all(value[-1] == 0 for value in events))
        self.assertFalse(desktop.held or desktop.pressed)

    def test_focus_loss_during_hold_releases_the_pressed_key(self):
        desktop = object.__new__(beta.Desktop)
        desktop.held, desktop.pressed = set(), set()
        events = []
        desktop.emit = lambda device, values: events.extend((device, *event) for event in values)
        calls = []
        def guard():
            calls.append(True)
            if len(calls) > 1: raise RuntimeError("lost focus")
        desktop.guard = guard
        with patch.object(beta.time, "sleep"), patch.object(beta.time, "monotonic", return_value=0):
            with self.assertRaisesRegex(RuntimeError, "lost focus"):
                desktop.key("w", .2)
        self.assertEqual(events, [(0, 1, beta.KEYS["w"], 1), (0, 1, beta.KEYS["w"], 0)])
        self.assertFalse(desktop.held)

    def test_socket_request_is_validated_without_input(self):
        sender, receiver = socket.socketpair()
        try:
            sender.sendall((json.dumps({"action": "tap", "key": "Tab"}) + "\n").encode())
            self.assertEqual(beta.read_request(receiver)["key"], "Tab")
        finally:
            sender.close(); receiver.close()


if __name__ == "__main__":
    unittest.main()
