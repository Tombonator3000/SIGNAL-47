#!/usr/bin/env python3
"""Explicit, isolated Unreal beta sessions controlled with native Linux input.

The Desktop device/focus pattern is adapted from Automation/gauntlet-user-journey.py.
No Unity telemetry, game callbacks, world coordinates or hidden state are used.
Default serve is a plan only. --execute is required to launch or send native input.
"""
import argparse
import contextlib
import ctypes as C
import ctypes.util
import fcntl
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import select
import shutil
import signal
import socket
import stat
import struct
import subprocess
import sys
import time

KEYS = {"F1": 59, "F2": 60, "F3": 61, "F4": 62, "F5": 63, "F6": 64, "F7": 65, "F8": 66, "F9": 67, "F10": 68, "F11": 87, "F12": 88, "Escape": 1, "Tab": 15, "Enter": 28, "Space": 57,
        "Shift": 42, "Control": 29, "Alt": 56, "Backspace": 14, ",": 51, "comma": 51,
        "Up": 103, "Left": 105, "Right": 106, "Down": 108,
        "PageUp": 104, "PageDown": 109, "Home": 102, "End": 107,
        **dict(zip("qwertyuiop", range(16, 26))),
        **dict(zip("asdfghjkl", range(30, 39))),
        **dict(zip("zxcvbnm", range(44, 51))),
        **dict(zip("1234567890", range(2, 12)))}
BUTTONS = {"left": 272, "right": 273, "middle": 274}
SOCKET_NAME = "control.sock"


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def unlocked():
    result = subprocess.run(["qdbus6", "org.freedesktop.ScreenSaver", "/ScreenSaver",
                             "org.freedesktop.ScreenSaver.GetActive"],
                            capture_output=True, text=True, timeout=5)
    if result.returncode or result.stdout.strip() != "false":
        raise RuntimeError("Desktop lock is active or unknown; no native input permitted")


def number(value, low, high, name):
    if type(value) not in (int, float) or not math.isfinite(value) or not low <= value <= high:
        raise ValueError(f"{name} must be a finite number in [{low}, {high}]")
    return value


def validate_action(value):
    if not isinstance(value, dict):
        raise ValueError("Action must be an object")
    kind = value.get("action")
    allowed = {"tap", "hold", "look", "click", "drag", "scroll", "screenshot", "observe", "activate", "stop"}
    if not isinstance(kind, str) or kind not in allowed:
        raise ValueError("Unknown action")
    result = {"action": kind}
    if kind in ("tap", "hold"):
        if ("key" in value) == ("keys" in value):
            raise ValueError("Specify exactly one of key or keys")
        keys = value.get("keys") if "keys" in value else [value["key"]]
        if not isinstance(keys, list) or not 1 <= len(keys) <= 3:
            raise ValueError("keys must contain 1..3 distinct supported keys")
        if any(not isinstance(key, str) or key not in KEYS for key in keys):
            raise ValueError("Unsupported key")
        if len({KEYS[key] for key in keys}) != len(keys):
            raise ValueError("Chord must use distinct physical keys")
        result["keys" if "keys" in value else "key"] = keys if "keys" in value else keys[0]
        result["seconds"] = number(value.get("seconds", .08 if kind == "tap" else .25), .02, .5 if kind == "tap" else 5, "seconds")
    if kind == "look":
        for k in ("dx", "dy"):
            result[k] = round(number(value.get(k, 0), -500, 500, k))
    if kind in ("click", "drag"):
        for k in (("x", "y") if kind == "click" else ("x", "y", "to_x", "to_y")):
            result[k] = round(number(value.get(k), 0, 8192, k))
        result["button"] = value.get("button", "left")
        if not isinstance(result["button"], str) or result["button"] not in BUTTONS:
            raise ValueError("Unsupported mouse button")
    if kind == "scroll":
        result["steps"] = round(number(value.get("steps"), -10, 10, "steps"))
    if kind == "screenshot":
        name = value.get("name", "view")
        if not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,64}", name):
            raise ValueError("Screenshot name must be a short filename stem")
        result["name"] = name
    if kind in ("observe", "stop"):
        note = value.get("note", "")
        if not isinstance(note, str) or len(note) > 4000:
            raise ValueError("Note must be text of at most 4000 characters")
        result["note"] = note
    return result


def validate_profile_source(source):
    source = Path(source).absolute()
    if source.name != "profile" or source.parent.parent != Path("/tmp") or not re.fullmatch(r"s47-beta-[A-Za-z0-9_-]+", source.parent.name):
        raise ValueError("Profile source must be a prior /tmp/s47-beta-*/profile")
    if any(p.is_symlink() for p in (source, *source.parents)) or not source.is_dir():
        raise ValueError("Profile source must be an existing directory without symlinks")
    metadata = source.parent / "session.json"
    if metadata.is_symlink() or not metadata.is_file():
        raise ValueError("Prior native adapter session metadata required")
    prior = json.loads(metadata.read_text())
    if not isinstance(prior, dict) or prior.get("runtime") != "EXECUTED" or prior.get("output") != str(source.parent) or "-UserDir=" + str(source) not in prior.get("command", []):
        raise ValueError("Source provenance does not identify this isolated profile")
    validate_profile_tree(source)
    return source


def validate_profile_tree(source):
    for path in (source, *source.rglob("*")):
        mode = path.lstat().st_mode
        if not (stat.S_ISDIR(mode) or stat.S_ISREG(mode)) or path.stat().st_uid != os.getuid():
            raise ValueError("Profile tree must contain only owned regular files/directories, no symlinks or devices")


def prepare_profile(out, source=None):
    destination = out / "profile"
    if source is None:
        destination.mkdir(mode=0o700)
    else:
        source = validate_profile_source(source)
        # Preserve any concurrently introduced links rather than following them;
        # post-copy validation rejects them before a game can be launched.
        shutil.copytree(source, destination, symlinks=True)
        validate_profile_tree(destination)
        destination.chmod(0o700)


def load_xtest():
    library = ctypes.util.find_library("Xtst")
    if not library:
        raise RuntimeError("--mouse-backend xtest requires existing libXtst; no installation attempted")
    xt = C.CDLL(library)
    xt.XTestFakeRelativeMotionEvent.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_ulong]
    xt.XTestFakeRelativeMotionEvent.restype = C.c_int
    xt.XTestFakeButtonEvent.argtypes = [C.c_void_p, C.c_uint, C.c_int, C.c_ulong]
    xt.XTestFakeButtonEvent.restype = C.c_int
    return xt


def session_plan(repo, build, out, seconds, width, height, profile_source=None, mouse_backend="uinput"):
    if mouse_backend not in ("uinput", "xtest"):
        raise ValueError("Unknown mouse backend")
    repo, build, out = repo.resolve(), build.resolve(), out.absolute()
    if out.exists() or any(p.is_symlink() for p in (out, *out.parents)):
        raise ValueError("A fresh session directory without symlink parents is required")
    profile_source = validate_profile_source(profile_source) if profile_source is not None else None
    executable = build / "Signal47/Binaries/Linux/Signal47"
    if not executable.is_file():
        raise ValueError("--build must be the Linux directory containing Signal47/Binaries/Linux/Signal47")
    if len(os.fsencode(out / SOCKET_NAME)) >= 104:
        raise ValueError("Session directory is too long for a Unix socket; choose a short /tmp path")
    return {"repo": str(repo), "build": str(build), "output": str(out),
            "executable": str(executable), "runtime": "NOT_RUN", "native_input": "NOT_RUN", "human_playtest": "NOT_RUN",
            "duration_limit_seconds": seconds, "mouse_backend": mouse_backend, "keyboard_backend": "uinput",
            "profile_mode": "resumed" if profile_source else "fresh",
            "profile_source": str(profile_source) if profile_source else None,
            "profile_copy": "NOT_RUN; copied before launch only with --execute",
            "command": [str(executable), "Signal47", "-UserDir=" + str(out / "profile"),
                        "-windowed", "-ResX=" + str(width), "-ResY=" + str(height), "-sdlvideodriver=x11", "-log", "-NoSplash"],
            "method": "Visible-screen guided native uinput keyboard and " + mouse_backend + " mouse; no game-state setters, teleport, scenario fixtures or hidden telemetry."}


class Desktop:
    """PID-bound XWayland discovery plus native uinput keyboard and relative mouse."""
    def __init__(self, process, executable, health, mouse_backend="uinput"):
        self.process, self.executable, self.health = process, executable.resolve(), health
        self.mouse_backend = mouse_backend
        self.xtest = load_xtest() if mouse_backend == "xtest" else None
        self.devices, self.held, self.pressed = [], set(), set()
        self.emitted_events = 0
        self.window = None
        self.x = C.CDLL("libX11.so.6")
        self.display = None
        self.start_ticks = self._identity()
        D, U, I = C.c_void_p, C.c_ulong, C.c_int
        def bind(name, args, result=I):
            f = getattr(self.x, name); f.argtypes = args; f.restype = result
        bind("XOpenDisplay", [C.c_char_p], D)
        bind("XCloseDisplay", [D])
        bind("XDefaultRootWindow", [D], U)
        bind("XQueryTree", [D, U, C.POINTER(U), C.POINTER(U), C.POINTER(C.POINTER(U)), C.POINTER(C.c_uint)])
        bind("XGetWindowProperty", [D, U, U, C.c_long, C.c_long, I, U, C.POINTER(U), C.POINTER(I), C.POINTER(U), C.POINTER(U), C.POINTER(C.POINTER(C.c_ubyte))])
        bind("XInternAtom", [D, C.c_char_p, I], U)
        bind("XFree", [D])
        bind("XFlush", [D])
        bind("XSync", [D, I])
        bind("XWarpPointer", [D, U, U, I, I, C.c_uint, C.c_uint, I, I])
        bind("XGetInputFocus", [D, C.POINTER(U), C.POINTER(I)])
        bind("XQueryPointer", [D, U, C.POINTER(U), C.POINTER(U), C.POINTER(I), C.POINTER(I), C.POINTER(I), C.POINTER(I), C.POINTER(C.c_uint)])
        bind("XGetGeometry", [D, U, C.POINTER(U), C.POINTER(I), C.POINTER(I), C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint)])
        self._error_handler = C.CFUNCTYPE(I, D, D)(lambda *_: 0)
        self.x.XSetErrorHandler.argtypes = [C.CFUNCTYPE(I, D, D)]
        self.x.XSetErrorHandler(self._error_handler)
        unlocked()
        self.display = self.x.XOpenDisplay(None)
        if not self.display:
            raise RuntimeError("Cannot open XWayland display")
        self.root = self.x.XDefaultRootWindow(self.display)

    def _identity(self):
        if self.process.poll() is not None:
            raise RuntimeError("Owned game process exited")
        proc = Path("/proc") / str(self.process.pid)
        if Path(os.readlink(proc / "exe")).resolve() != self.executable:
            raise RuntimeError("Owned PID executable identity changed")
        return (proc / "stat").read_text().rsplit(")", 1)[1].split()[19]

    def window_pid(self, window):
        atom = self.x.XInternAtom(self.display, b"_NET_WM_PID", 1)
        actual, fmt, count, left = C.c_ulong(), C.c_int(), C.c_ulong(), C.c_ulong()
        data = C.POINTER(C.c_ubyte)()
        result = self.x.XGetWindowProperty(self.display, window, atom, 0, 1, 0, 0,
                                          C.byref(actual), C.byref(fmt), C.byref(count), C.byref(left), C.byref(data))
        try:
            return C.cast(data, C.POINTER(C.c_ulong))[0] if result == 0 and fmt.value == 32 and count.value == 1 and data else None
        finally:
            if data: self.x.XFree(data)

    def find_window(self):
        found = []
        def visit(window, depth):
            if self.window_pid(window) == self.process.pid:
                found.append(window)
            if not depth: return
            root, parent, children, count = C.c_ulong(), C.c_ulong(), C.POINTER(C.c_ulong)(), C.c_uint()
            if self.x.XQueryTree(self.display, window, C.byref(root), C.byref(parent), C.byref(children), C.byref(count)):
                try:
                    for i in range(count.value): visit(children[i], depth - 1)
                finally:
                    if children: self.x.XFree(children)
        visit(self.root, 4)
        if len(found) != 1:
            return False
        self.window = found[0]
        return True

    def guard(self, focus=True):
        self.health()
        unlocked()
        if self._identity() != self.start_ticks:
            raise RuntimeError("PID was reused")
        if self.window is None or self.window_pid(self.window) != self.process.pid:
            raise RuntimeError("Owned game window unavailable")
        if focus:
            window, revert = C.c_ulong(), C.c_int()
            self.x.XGetInputFocus(self.display, C.byref(window), C.byref(revert))
            if window.value != self.window:
                raise RuntimeError("Game lost focus; no native input sent")

    def activate(self):
        self.guard(focus=False)
        class Message(C.Structure):
            _fields_ = [("type", C.c_int), ("serial", C.c_ulong), ("send", C.c_int), ("display", C.c_void_p), ("window", C.c_ulong), ("message_type", C.c_ulong), ("format", C.c_int), ("data", C.c_long * 5)]
        class Event(C.Union):
            _fields_ = [("message", Message), ("pad", C.c_long * 24)]
        self.x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.POINTER(Event)]
        event = Event(); event.message.type = 33; event.message.display = self.display
        event.message.window = self.window
        event.message.message_type = self.x.XInternAtom(self.display, b"_NET_ACTIVE_WINDOW", 0)
        event.message.format = 32; event.message.data[0] = 2
        self.x.XSendEvent(self.display, self.root, 0, (1 << 20) | (1 << 19), C.byref(event))
        self.x.XFlush(self.display)
        time.sleep(.5)
        self.guard()

    def setup_devices(self):
        self.guard()
        for name, codes, relative in [("S47 beta keyboard", sorted(set(KEYS.values())), False), ("S47 beta mouse", list(BUTTONS.values()), True)]:
            if relative and self.mouse_backend == "xtest":
                continue
            fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
            self.devices.append(fd)
            fcntl.ioctl(fd, 0x40045564, 1)
            for key in codes: fcntl.ioctl(fd, 0x40045565, key)
            if relative:
                fcntl.ioctl(fd, 0x40045564, 2)
                for axis in (0, 1, 8): fcntl.ioctl(fd, 0x40045566, axis)
            fcntl.ioctl(fd, 0x405c5503, struct.pack("HHHH80sI", 3, 0x1209, 0x4747, 1, name.encode(), 0))
            fcntl.ioctl(fd, 0x5501)
        time.sleep(.7)

    def emit(self, device, events):
        if device == 1 and getattr(self, "mouse_backend", "uinput") == "xtest":
            dx = sum(value for kind, code, value in events if (kind, code) == (2, 0))
            dy = sum(value for kind, code, value in events if (kind, code) == (2, 1))
            if (dx or dy) and not self.xtest.XTestFakeRelativeMotionEvent(self.display, dx, dy, 0):
                raise RuntimeError("XTest relative motion failed")
            for kind, code, value in events:
                if kind == 1:
                    button = {272: 1, 273: 3, 274: 2}[code]
                    if not self.xtest.XTestFakeButtonEvent(self.display, button, int(value), 0):
                        raise RuntimeError("XTest button event failed")
                elif (kind, code) == (2, 8):
                    button = 4 if value > 0 else 5
                    for _ in range(abs(value)):
                        try:
                            if not self.xtest.XTestFakeButtonEvent(self.display, button, 1, 0):
                                raise RuntimeError("XTest wheel event failed")
                        finally:
                            self.xtest.XTestFakeButtonEvent(self.display, button, 0, 0)
            self.x.XSync(self.display, 0)
            self.emitted_events += len(events)
            return
        for kind, code, value in [*events, (0, 0, 0)]:
            os.write(self.devices[device], struct.pack("llHHi", 0, 0, kind, code, int(value)))
            if kind: self.emitted_events += 1

    def release(self):
        for key in tuple(self.held):
            with contextlib.suppress(OSError, RuntimeError): self.emit(0, [(1, KEYS[key], 0)])
            self.held.discard(key)
        for button in tuple(self.pressed):
            with contextlib.suppress(OSError, RuntimeError): self.emit(1, [(1, button, 0)])
            self.pressed.discard(button)

    def close(self):
        self.release()
        for fd in self.devices:
            with contextlib.suppress(OSError): fcntl.ioctl(fd, 0x5502)
            os.close(fd)
        self.devices = []
        if self.display:
            self.x.XCloseDisplay(self.display); self.display = None

    def key(self, key, seconds):
        self.chord([key], seconds)

    def chord(self, keys, seconds):
        self.guard()
        try:
            # Register all keys before writing: even a partial device write is released.
            self.held.update(keys)
            self.emit(0, [(1, KEYS[key], 1) for key in keys])
            end = time.monotonic() + seconds
            while time.monotonic() < end:
                time.sleep(min(.1, max(0, end - time.monotonic())))
                self.guard()
        finally:
            self.release()

    def pointer(self, x, y):
        root, gx, gy, width, height, border, depth = C.c_ulong(), C.c_int(), C.c_int(), C.c_uint(), C.c_uint(), C.c_uint(), C.c_uint()
        if not self.x.XGetGeometry(self.display, self.window, C.byref(root), C.byref(gx), C.byref(gy), C.byref(width), C.byref(height), C.byref(border), C.byref(depth)):
            raise RuntimeError("Cannot read game window geometry")
        if not 0 <= x < width.value or not 0 <= y < height.value:
            raise ValueError("Pointer target lies outside owned window")
        if getattr(self, "mouse_backend", "uinput") == "xtest":
            self.guard()
            self.x.XWarpPointer(self.display, 0, self.window, 0, 0, 0, 0, x, y)
            self.x.XSync(self.display, 0)
        for _ in range(50):
            self.guard()
            rr, child, rx, ry, px, py, mask = C.c_ulong(), C.c_ulong(), C.c_int(), C.c_int(), C.c_int(), C.c_int(), C.c_uint()
            if not self.x.XQueryPointer(self.display, self.window, C.byref(rr), C.byref(child), C.byref(rx), C.byref(ry), C.byref(px), C.byref(py), C.byref(mask)):
                raise RuntimeError("Cannot read pointer")
            dx, dy = x - px.value, y - py.value
            if abs(dx) <= 2 and abs(dy) <= 2: return
            if getattr(self, "mouse_backend", "uinput") == "xtest":
                raise RuntimeError("X11 pointer did not reach client target; game may capture pointer")
            def delta(value): return max(-100, min(100, round(value * .5))) or (1 if value > 0 else -1) if value else 0
            self.emit(1, [(2, 0, delta(dx)), (2, 1, delta(dy))]); time.sleep(.04)
        raise RuntimeError("Pointer target unreachable; mouse may be captured by gameplay")

    def perform(self, action):
        kind = action["action"]
        if kind == "activate": self.activate(); return
        self.guard()
        if kind in ("tap", "hold"): self.chord(action["keys"] if "keys" in action else [action["key"]], action["seconds"])
        elif kind == "look": self.emit(1, [(2, 0, action["dx"]), (2, 1, action["dy"])])
        elif kind == "scroll": self.emit(1, [(2, 8, action["steps"])])
        elif kind in ("click", "drag"):
            self.pointer(action["x"], action["y"])
            self.guard(); button = BUTTONS[action["button"]]; self.pressed.add(button)
            try:
                self.emit(1, [(1, button, 1)])
                time.sleep(.08)
                self.guard()
                if kind == "drag": self.pointer(action["to_x"], action["to_y"])
            finally: self.release()


def read_request(connection):
    raw = bytearray()
    while b"\n" not in raw:
        chunk = connection.recv(4096)
        if not chunk: raise ValueError("Incomplete command")
        raw.extend(chunk)
        if len(raw) > 8192: raise ValueError("Command too large")
    return validate_action(json.loads(raw.split(b"\n", 1)[0]))


def serve(args):
    plan = session_plan(args.repo, args.build, args.out, args.seconds, args.width, args.height, args.profile_source, args.mouse_backend)
    if not args.execute:
        print(json.dumps(plan, indent=2)); return
    out = args.out.absolute()
    if out.exists() or any(p.is_symlink() for p in (out, *out.parents)):
        raise ValueError("Execute requires a fresh session directory with no symlink parents")
    scripts = args.repo.resolve() / "unreal/Signal47/Scripts"
    lease = load_module("beta_lease", scripts / "production_lease.py")
    guard = load_module("beta_guard", scripts / "unreal_guard.py")
    if os.environ.get(guard.GRACE_ENV):
        raise RuntimeError("Native beta refuses installed-game grace exceptions")
    unlocked()
    if args.mouse_backend == "xtest": load_xtest()  # Verify library/symbols before any game launch.
    if not os.access("/dev/uinput", os.W_OK): raise RuntimeError("Existing uinput permission required; no permissions will be changed")
    if not shutil.which("spectacle"): raise RuntimeError("Spectacle is required for visible observations")
    # A root-owned build/test lease may be inherited; otherwise reserve this one session.
    reservation = contextlib.nullcontext() if os.environ.get(lease.TOKEN_ENV) else lease.HeavyReservation("native gameplay beta")
    result = dict(plan); result.update(gameplay_acceptance="UNASSESSED", native_actions=0, observations=0)
    process = desktop = server = None
    with reservation as held:
        if held: os.environ.update(held.environment())
        guard.require_idle()
        out.mkdir(mode=0o700, parents=True)
        prepare_profile(out, args.profile_source)
        result["profile_copy"] = "COPIED_FROM_PRIOR_ISOLATED_PROFILE" if args.profile_source else "FRESH_EMPTY_PROFILE"
        (out / "shots").mkdir(mode=0o700)
        env = os.environ.copy(); env.update(SDL_VIDEO_DRIVER="x11", SDL_VIDEODRIVER="x11")
        executable = Path(plan["executable"])
        with executable.open("rb") as stream:
            result["binary_sha256"] = hashlib.file_digest(stream, "sha256").hexdigest()
        result["source_revision"] = subprocess.check_output(["git", "-C", str(args.repo), "rev-parse", "HEAD"], text=True).strip()
        result["source_matches_binary"] = "UNVERIFIED; binary fingerprint is authoritative for this session"
        began = time.monotonic()
        def record(event):
            event = {"elapsed": round(time.monotonic() - began, 3), **event}
            with (out / "events.jsonl").open("a") as stream: stream.write(json.dumps(event, ensure_ascii=False) + "\n")
        def health():
            if time.monotonic() - began > args.seconds: raise RuntimeError("Session time limit reached")
            if process is not None and process.poll() is not None: raise RuntimeError("Owned game exited")
            for job in guard.heavy_jobs():
                if process is None or not guard.owned_process(job["pid"], process.pid):
                    raise RuntimeError("Another heavy job started; beta input stopped")
        def interrupted(signum, frame): raise InterruptedError("Native beta interrupted")
        old_term = signal.signal(signal.SIGTERM, interrupted)
        try:
            with (out / "game.log").open("w") as log:
                process = subprocess.Popen(plan["command"], cwd=args.build.resolve(), env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
            result.update(runtime="EXECUTED", pid=process.pid)
            # Wait for executable/window only; no gameplay state is inspected.
            time.sleep(.5)
            desktop = Desktop(process, executable, health, args.mouse_backend)
            end = time.monotonic() + 90
            while not desktop.find_window():
                health()
                if time.monotonic() > end: raise RuntimeError("No unique PID-bound XWayland game window")
                time.sleep(.25)
            desktop.activate(); desktop.setup_devices()
            server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            server.bind(str(out / SOCKET_NAME)); os.chmod(out / SOCKET_NAME, 0o600); server.listen(1)
            record({"event": "ready", "pid": process.pid, "window": desktop.window})
            (out / "session.json").write_text(json.dumps(result, indent=2) + "\n")
            print(json.dumps({"status": "READY", "session": str(out), "pid": process.pid}), flush=True)
            stopped = False
            while not stopped:
                health()
                if not select.select([server], [], [], .25)[0]: continue
                connection, _ = server.accept()
                with connection:
                    connection.settimeout(10)
                    action = None
                    try:
                        action = read_request(connection)
                        record({"event": "request", "action": action})
                        kind = action["action"]
                        response = {"status": "RECORDED", "action": kind}
                        if kind == "stop":
                            desktop.release(); stopped = True; result["end_note"] = action["note"]
                        elif kind == "observe":
                            result["observations"] += 1
                        elif kind == "screenshot":
                            desktop.guard()
                            path = out / "shots" / f"{time.monotonic_ns()}-{action['name']}.png"
                            subprocess.run(["spectacle", "--activewindow", "--background", "--nonotify", "--output", str(path)], check=True, timeout=15, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                            desktop.guard()
                            if not path.is_file() or path.read_bytes()[:8] != b"\x89PNG\r\n\x1a\n": raise RuntimeError("Screenshot was not saved as PNG")
                            response["image"] = str(path)
                            response["status"] = "CAPTURED_REQUIRES_VISUAL_REVIEW"
                        else:
                            desktop.perform(action)
                            response["status"] = "INPUT_SENT_REQUIRES_VISUAL_REVIEW"
                            result["native_actions"] += int(kind != "activate")
                            if result["native_actions"]: result["native_input"] = "EXECUTED_NOT_GAMEPLAY_ACCEPTANCE"
                        record({"event": "response", **response})
                        connection.sendall((json.dumps(response) + "\n").encode())
                    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as exc:
                        desktop.release()
                        response = {"status": "BLOCKED", "error": str(exc)}
                        record({"event": "response", **response})
                        with contextlib.suppress(OSError): connection.sendall((json.dumps(response) + "\n").encode())
                        # Focus/lock failures pause input; explicit activation is required to resume.
            result["status"] = "ENDED_BY_OPERATOR"
        except BaseException as exc:
            result.update(status="INTERRUPTED", error=str(exc))
            raise
        finally:
            if desktop:
                desktop.close()
                result["native_events_sent"] = desktop.emitted_events
                if desktop.emitted_events: result["native_input"] = "EXECUTED_NOT_GAMEPLAY_ACCEPTANCE"
            if server: server.close()
            with contextlib.suppress(FileNotFoundError): (out / SOCKET_NAME).unlink()
            if process is not None and process.poll() is None:
                with contextlib.suppress(ProcessLookupError): os.killpg(process.pid, signal.SIGTERM)
                try: process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    with contextlib.suppress(ProcessLookupError): os.killpg(process.pid, signal.SIGKILL)
                    process.wait(timeout=5)
            result["elapsed"] = round(time.monotonic() - began, 3)
            (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
            signal.signal(signal.SIGTERM, old_term)


def client(args):
    action = validate_action(json.loads(args.json))
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(30)
        connection.connect(str(args.session.absolute() / SOCKET_NAME))
        connection.sendall((json.dumps(action) + "\n").encode())
        data = bytearray()
        while b"\n" not in data:
            chunk = connection.recv(4096)
            if not chunk: raise RuntimeError("Session closed before response")
            data.extend(chunk)
            if len(data) > 65536: raise RuntimeError("Response too large")
        print(data.decode().strip())


def probe():
    """Inventory only: no display activation, device creation, files or game launch."""
    try:
        unlocked(); lock = "unlocked"
    except (RuntimeError, OSError, subprocess.SubprocessError):
        lock = "locked_or_unknown"
    print(json.dumps({"runtime": "NOT_RUN", "native_input": "NOT_RUN",
                      "session_type": os.environ.get("XDG_SESSION_TYPE"),
                      "x_display_present": bool(os.environ.get("DISPLAY")),
                      "libX11": bool(ctypes.util.find_library("X11")),
                      "uinput_writable": os.access("/dev/uinput", os.W_OK),
                      "spectacle": bool(shutil.which("spectacle")), "qdbus6": bool(shutil.which("qdbus6")),
                      "desktop_lock": lock,
                      "input_compatibility": "UNVERIFIED until native TAB/mouse and capture smoke test"}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("probe", help="Read-only input/capture prerequisites; no launch or input")
    start = commands.add_parser("serve", help="Print a NOT_RUN plan; --execute explicitly launches a guarded session")
    start.add_argument("--repo", type=Path, required=True)
    start.add_argument("--build", type=Path, required=True, help="Packaged Linux directory")
    start.add_argument("--out", type=Path, required=True, help="Fresh private session directory")
    start.add_argument("--mouse-backend", choices=("uinput", "xtest"), default="uinput", help="Explicit consistent mouse backend; xtest requires existing libXtst and forced X11 game")
    start.add_argument("--profile-source", type=Path, help="Prior isolated /tmp/s47-beta-*/profile to copy before launch; use normal Continue")
    start.add_argument("--seconds", type=int, default=10800, help="Wall-time limit; active gameplay time must be measured separately")
    start.add_argument("--width", type=int, default=1280)
    start.add_argument("--height", type=int, default=720)
    start.add_argument("--execute", action="store_true")
    action = commands.add_parser("action")
    action.add_argument("--session", type=Path, required=True)
    action.add_argument("--json", required=True)
    args = parser.parse_args()
    try:
        if args.command == "serve":
            number(args.seconds, 30, 14400, "seconds"); number(args.width, 640, 3840, "width"); number(args.height, 480, 2160, "height")
            serve(args)
        elif args.command == "action": client(args)
        else: probe()
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as exc:
        parser.exit(2, "native beta: " + str(exc) + "\n")


if __name__ == "__main__":
    main()
