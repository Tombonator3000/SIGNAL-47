"""Mock-only connection-lifetime checks; no real desktop inhibitor is created."""
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
from inhibit_lock import ScreenSaverInhibitor


class InhibitorTests(unittest.TestCase):
    def fixture(self, locked=False):
        interface, bus = Mock(), Mock()
        interface.GetActive.return_value = locked
        interface.Inhibit.return_value = 42
        module = SimpleNamespace(SessionBus=Mock(return_value=bus), Interface=Mock(return_value=interface))
        return module, interface, bus

    def test_connection_lives_until_context_exit(self):
        module, interface, bus = self.fixture()
        with patch.dict(sys.modules, {"dbus": module}):
            with ScreenSaverInhibitor():
                module.SessionBus.assert_called_once_with(private=True)
                bus.close.assert_not_called()
                interface.UnInhibit.assert_not_called()
            interface.UnInhibit.assert_called_once_with(42)
            bus.close.assert_called_once()

    def test_locked_desktop_is_not_unlocked_or_inhibited(self):
        module, interface, bus = self.fixture(locked=True)
        with patch.dict(sys.modules, {"dbus": module}):
            with self.assertRaisesRegex(RuntimeError, "locked"):
                with ScreenSaverInhibitor(): pass
        interface.Inhibit.assert_not_called()
        interface.SetActive.assert_not_called()
        bus.close.assert_called_once()

    def test_exception_still_releases_inhibitor(self):
        module, interface, bus = self.fixture()
        with patch.dict(sys.modules, {"dbus": module}):
            with self.assertRaises(ValueError):
                with ScreenSaverInhibitor(): raise ValueError("test interruption")
        interface.UnInhibit.assert_called_once_with(42)
        bus.close.assert_called_once()


if __name__ == "__main__":
    unittest.main()
