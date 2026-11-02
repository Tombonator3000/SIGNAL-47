#!/usr/bin/env python3
"""Hold an explicitly requested, session-scoped KDE screensaver inhibitor.

The D-Bus connection stays alive until timeout or SIGTERM/SIGINT/SIGHUP. No permanent
desktop setting is changed, no unlock is attempted and no input is generated.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import threading
import time


class ScreenSaverInhibitor:
    def __init__(self, reason="Authorized SIGNAL47 gameplay work"):
        self.reason = reason
        self.bus = self.interface = self.cookie = None

    def __enter__(self):
        import dbus
        self.bus = dbus.SessionBus(private=True)
        try:
            self.interface = dbus.Interface(self.bus.get_object("org.freedesktop.ScreenSaver", "/ScreenSaver"), "org.freedesktop.ScreenSaver")
            if bool(self.interface.GetActive()):
                raise RuntimeError("Desktop is locked; inhibitor does not unlock it")
            self.cookie = self.interface.Inhibit("SIGNAL47 authorized gameplay work", self.reason)
            return self
        except BaseException:
            self.bus.close(); self.bus = None
            raise

    def __exit__(self, *_):
        try:
            if self.cookie is not None:
                self.interface.UnInhibit(self.cookie)
                self.cookie = None
        finally:
            if self.bus is not None:
                self.bus.close(); self.bus = None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seconds", type=int, default=10800)
    parser.add_argument("--state", type=Path, required=True, help="Fresh private task state file containing PID and release status")
    args = parser.parse_args()
    if not 30 <= args.seconds <= 14400:
        parser.error("--seconds must be 30..14400")
    path = args.state.absolute()
    if path.exists() or any(p.is_symlink() for p in (path, *path.parents)):
        parser.error("--state must be a fresh file without symlink parents")
    stopped = threading.Event()
    signal.signal(signal.SIGTERM, lambda *_: stopped.set())
    signal.signal(signal.SIGINT, lambda *_: stopped.set())
    signal.signal(signal.SIGHUP, lambda *_: stopped.set())
    began = time.monotonic()
    state = {"pid": os.getpid(), "scope": "session D-Bus connection only", "seconds_limit": args.seconds,
             "input": "NOT_SENT", "game": "NOT_LAUNCHED", "status": "STARTING"}
    def save():
        path.write_text(json.dumps(state, indent=2) + "\n")
    # Exclusive creation also prevents replacing a task's prior control record.
    with path.open("x") as stream:
        os.chmod(path, 0o600)
        stream.write(json.dumps(state, indent=2) + "\n")
    try:
        with ScreenSaverInhibitor() as inhibitor:
            state.update(status="INHIBITING", cookie=int(inhibitor.cookie), dbus_connection=str(inhibitor.bus.get_unique_name()))
            save(); print(json.dumps(state), flush=True)
            while not stopped.is_set() and time.monotonic() - began < args.seconds:
                stopped.wait(min(30, max(0, args.seconds - (time.monotonic() - began))))
        state.update(status="RELEASED", elapsed_seconds=round(time.monotonic() - began, 2))
        save(); print(json.dumps(state), flush=True)
    except BaseException as exc:
        state.update(status="STOPPED", error=str(exc), elapsed_seconds=round(time.monotonic() - began, 2))
        save()
        raise


if __name__ == "__main__":
    main()
