#!/usr/bin/env python3
"""Verify an immutable Chapter09 archive, then exercise its normal Linux launcher.

Example:
  python3 Automation/verify-chapter09-package.py --archive ARCHIVE.tar.gz \
    --extract-dir /tmp/signal47-new-package-check --out Artifacts/Chapter09/PackageStartup

Extraction and output directories must be absent or empty. No observation,
smoke-test, telemetry, or direct gameplay APIs are used. Native input and a
zero exit code are machine-verifiable; the three original window captures
require a separate visual review of menu, playing, and pause content.
"""
import argparse
import ctypes as C
import fcntl
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import signal
import stat
import subprocess
import sys
import tarfile
import time

ROOT = Path(__file__).resolve().parents[1]
CHUNK = 1024 * 1024
MANIFEST_NAME = "package-manifest.json"
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
MAX_MEMBERS = 100000
MAX_UNPACKED_BYTES = 32 * 1024 ** 3
MAX_MANIFEST_BYTES = 16 * 1024 ** 2
WIDTH, HEIGHT = 1280, 800


def require(condition, message):
    if not condition:
        raise ValueError(message)


def utc():
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def digest_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(CHUNK), b""):
            digest.update(chunk)
    return digest.hexdigest()


def regular_file(path):
    require(path.exists() and not path.is_symlink() and stat.S_ISREG(path.stat().st_mode),
            f"Expected a regular file without a symbolic link: {path}")


def new_directory(path):
    """Never remove evidence, follow output symlinks, or reuse a populated tree."""
    path = path.expanduser().absolute()
    for parent in [path, *path.parents]:
        require(not parent.is_symlink(), f"Output path contains a symbolic link: {parent}")
        if parent.exists():
            require(parent.is_dir(), f"Output parent is not a directory: {parent}")
    if path.exists():
        require(not any(path.iterdir()), f"Output directory must be NEW and empty: {path}")
        require(path.stat().st_uid == os.getuid(), f"Output directory belongs to another user: {path}")
    else:
        path.mkdir(parents=True, mode=0o700)
    # A private extraction container prevents another account from inserting
    # entries while the checked regular-file archive is being written.
    path.chmod(0o700)
    return path.resolve()


def relative_path(value, allow_root=False):
    require(isinstance(value, str) and value and "\0" not in value and "\\" not in value,
            f"Invalid package path: {value!r}")
    if allow_root and value == ".":
        return value
    require(not value.startswith("/"), f"Absolute package path refused: {value}")
    pieces = value.split("/")
    require(all(piece not in ("", ".", "..") for piece in pieces), f"Non-canonical package path refused: {value}")
    require(PurePosixPath(value).as_posix() == value, f"Non-canonical package path refused: {value}")
    return value


def file_mode(value):
    require(isinstance(value, str) and re.fullmatch(r"0[0-7]{3}", value), f"Invalid permission mode: {value!r}")
    return int(value, 8)


def describe_manifest(manifest):
    require(manifest.get("schema") == "signal47-chapter09-package-v1", "Unknown package-manifest schema.")
    package_id = manifest.get("package_id")
    require(isinstance(package_id, str) and re.fullmatch(r"(?:Chapter09|Visual10|NightSky12|Menu14|Recovery15|Archive16|Workstation18|Resources19)-[0-9a-f]{12}", package_id), "Invalid package id.")
    require(re.fullmatch(r"[0-9a-f]{40}", manifest.get("base_revision", "")), "Package needs an exact base revision.")
    for name in ("unity_source_sha256", "build_payload_sha256", "player_sha256", "package_payload_sha256"):
        require(isinstance(manifest.get(name), str) and SHA256.fullmatch(manifest[name]), f"Invalid manifest hash: {name}")
    require(package_id.split("-", 1)[1] == manifest["unity_source_sha256"][:12], "Package id does not identify its source hash.")
    files, directories = {}, {}
    for kind, result in (("files", files), ("directories", directories)):
        require(isinstance(manifest.get(kind), list), f"Missing manifest {kind} list.")
        for item in manifest[kind]:
            require(isinstance(item, dict), f"Invalid {kind} record.")
            path = relative_path(item.get("path"), allow_root=kind == "directories")
            require(path not in result, f"Duplicate manifest path: {path}")
            mode = file_mode(item.get("mode"))
            if kind == "files":
                require(path != MANIFEST_NAME, "The package manifest must not claim its own payload hash.")
                require(type(item.get("bytes")) is int and 0 <= item["bytes"] <= MAX_UNPACKED_BYTES,
                        f"Invalid byte count: {path}")
                require(isinstance(item.get("sha256"), str) and SHA256.fullmatch(item["sha256"]), f"Invalid file hash: {path}")
            result[path] = {**item, "numeric_mode": mode}
    require("." in directories, "Root package directory missing from manifest.")
    require(not set(files).intersection(directories), "Manifest reuses a file path as a directory.")
    for path in [*files, *directories]:
        if path == ".":
            continue
        parent = PurePosixPath(path).parent.as_posix()
        require(parent in directories, f"Manifest omits the parent directory of {path}")
    return package_id, files, directories


def inspect_archive(tar):
    entries, total = {}, 0
    for member in tar:
        require(len(entries) < MAX_MEMBERS, "Archive exceeds the member-count limit.")
        name = member.name.rstrip("/") if member.isdir() else member.name
        name = relative_path(name)
        require(name not in entries, f"Duplicate archive path: {name}")
        require(member.isdir() or member.isreg(), f"Links, devices and special archive entries are refused: {name}")
        require(not member.linkname and not member.issparse(), f"Link/sparse entry refused: {name}")
        require(not member.mode & ~0o777, f"Special permission bits refused: {name}")
        require(type(member.size) is int and member.size >= 0, f"Invalid archive size: {name}")
        total += member.size
        require(total <= MAX_UNPACKED_BYTES, "Archive exceeds the uncompressed-size limit.")
        entries[name] = member
    roots = {name.split("/", 1)[0] for name in entries}
    require(len(roots) == 1, "Archive must contain exactly one package root.")
    package_root = next(iter(roots))
    require(package_root in entries and entries[package_root].isdir(), "Package root is not an explicit directory.")
    manifest_path = package_root + "/" + MANIFEST_NAME
    require(manifest_path in entries and entries[manifest_path].isreg(), "Package manifest missing from archive.")
    manifest_info = entries[manifest_path]
    require(manifest_info.size <= MAX_MANIFEST_BYTES and manifest_info.mode == 0o644, "Invalid package-manifest size/mode.")
    with tar.extractfile(manifest_info) as stream:
        raw = stream.read(MAX_MANIFEST_BYTES + 1)
    require(len(raw) == manifest_info.size, "Truncated package manifest.")
    manifest = json.loads(raw.decode("utf-8"))
    require(isinstance(manifest, dict), "Package manifest must be an object.")
    package_id, files, directories = describe_manifest(manifest)
    require(package_root == package_id, "Archive root and package-manifest id differ.")
    expected = {package_id if path == "." else package_id + "/" + path for path in directories}
    expected.update(package_id + "/" + path for path in files)
    expected.add(manifest_path)
    require(set(entries) == expected, "Archive contains missing or unlisted entries relative to its package manifest.")
    for path, record in directories.items():
        member = entries[package_id if path == "." else package_id + "/" + path]
        require(member.isdir() and member.mode == record["numeric_mode"], f"Archive directory mode/type differs: {path}")
    for path, record in files.items():
        member = entries[package_id + "/" + path]
        require(member.isreg() and member.mode == record["numeric_mode"] and member.size == record["bytes"],
                f"Archive file mode/type/size differs: {path}")
    return entries, manifest, raw, files, directories, total


def unpack_checked(archive, extraction, out, result):
    regular_file(archive)
    sidecar = archive.with_name(archive.name + ".sha256")
    regular_file(sidecar)
    require(sidecar.stat().st_size <= 2048, "Unexpectedly large archive checksum sidecar.")
    sidecar_raw = sidecar.read_bytes()
    checksum = re.fullmatch(r"([0-9a-f]{64})  ([^\n\r]+)\n?", sidecar_raw.decode("ascii"))
    require(checksum and checksum.group(2) == archive.name, "Archive sidecar does not name this exact archive.")
    expected_digest = checksum.group(1)
    before = archive.stat()
    actual_digest = digest_file(archive)
    require(actual_digest == expected_digest, "Archive SHA-256 does not match its sidecar.")
    result["archive"] = {"path": str(archive), "bytes": before.st_size, "sha256": actual_digest,
                         "sidecar": str(sidecar), "sidecar_sha256": hashlib.sha256(sidecar_raw).hexdigest()}
    result["checks"]["archive_sidecar"] = "PASS"
    (out / "archive.sha256").write_bytes(sidecar_raw)
    with tarfile.open(archive, "r:gz") as tar:
        entries, manifest, manifest_raw, files, directories, total = inspect_archive(tar)
        result["checks"]["tar_paths_links_types"] = "PASS"
        package = extraction / manifest["package_id"]
        for path in sorted(directories, key=lambda value: (value.count("/"), value)):
            target = package if path == "." else package / path
            target.mkdir(mode=0o700, parents=False, exist_ok=False)
        records = {**files, MANIFEST_NAME: {"sha256": hashlib.sha256(manifest_raw).hexdigest(),
                                           "bytes": len(manifest_raw), "numeric_mode": 0o644}}
        written = []
        for path in sorted(records):
            record = records[path]
            target = package / path
            # All ancestors were explicitly created from checked directory
            # records. The archive cannot create a symlink, hardlink or device.
            fd = os.open(target, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
            digest, count = hashlib.sha256(), 0
            with os.fdopen(fd, "wb") as output, tar.extractfile(entries[manifest["package_id"] + "/" + path]) as source:
                for chunk in iter(lambda: source.read(CHUNK), b""):
                    count += len(chunk)
                    require(count <= record["bytes"], f"File exceeds its recorded size: {path}")
                    digest.update(chunk)
                    output.write(chunk)
                output.flush()
                os.fchmod(output.fileno(), record["numeric_mode"])
            require(count == record["bytes"] and digest.hexdigest() == record["sha256"], f"Extracted file differs from manifest: {path}")
            written.append({"path": path, "bytes": count, "sha256": digest.hexdigest(), "mode": format(record["numeric_mode"], "04o")})
        for path in sorted(directories, key=lambda value: (value.count("/"), value), reverse=True):
            (package if path == "." else package / path).chmod(directories[path]["numeric_mode"])
    after = archive.stat()
    require((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) ==
            (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns) and digest_file(archive) == expected_digest,
            "Archive changed during verification/extraction.")
    actual_files, actual_dirs = set(), {"."}
    for path in package.rglob("*"):
        require(not path.is_symlink(), f"Unexpected link in extracted package: {path}")
        relative = path.relative_to(package).as_posix()
        if path.is_dir():
            actual_dirs.add(relative)
        elif path.is_file():
            actual_files.add(relative)
        else:
            raise ValueError(f"Unexpected extracted file type: {path}")
    require(actual_files == set(records) and actual_dirs == set(directories), "Extracted tree differs from the manifest.")
    for path, record in records.items():
        actual = (package / path).stat()
        require(actual.st_size == record["bytes"] and stat.S_IMODE(actual.st_mode) == record["numeric_mode"]
                and digest_file(package / path) == record["sha256"], f"Post-write file verification failed: {path}")
    for path, record in directories.items():
        require(stat.S_IMODE((package if path == "." else package / path).stat().st_mode) == record["numeric_mode"],
                f"Extracted directory permission differs: {path}")
    payload = hashlib.sha256()
    build_payload = hashlib.sha256()
    # The packager sorts pathlib paths, which compare path components rather
    # than the raw slash-containing strings when a directory shares a prefix.
    for path in sorted(files, key=lambda value: PurePosixPath(value).parts):
        streams = [payload]
        if PurePosixPath(path).name not in {"build-manifest.json", "build-id.txt"} and path not in {"Start-SIGNAL47.sh", "START_HER.txt"}:
            streams.append(build_payload)
        for digest in streams:
            digest.update(path.encode("utf-8") + b"\0")
        with (package / path).open("rb") as stream:
            for chunk in iter(lambda: stream.read(CHUNK), b""):
                for digest in streams:
                    digest.update(chunk)
    require(payload.hexdigest() == manifest["package_payload_sha256"], "Package payload aggregate hash differs.")
    require(build_payload.hexdigest() == manifest["build_payload_sha256"], "Original build payload aggregate hash differs.")
    build = json.loads((package / "build-manifest.json").read_text(encoding="utf-8"))
    for name in ("base_revision", "unity_source_sha256", "build_payload_sha256", "player_sha256"):
        require(build.get(name) == manifest[name], f"Build/package identity mismatch: {name}")
    require(digest_file(package / "Signal47.x86_64") == manifest["player_sha256"], "Player hash differs from build identity.")
    require(type(build.get("working_tree_changes")) is bool, "Build manifest has no working-tree status.")
    tree_state = "working-tree" if build["working_tree_changes"] else "clean"
    require((package / "build-id.txt").read_text(encoding="utf-8").strip() ==
            f"{build['base_revision']} {tree_state} source-sha256:{build['unity_source_sha256']}", "Incorrect build-id.txt.")
    for name in ("Start-SIGNAL47.sh", "Signal47.x86_64"):
        require(files[name]["numeric_mode"] & 0o111 and os.access(package / name, os.X_OK), f"Packaged executable cannot be run: {name}")
    require(files["Start-SIGNAL47.sh"]["numeric_mode"] == 0o755, "Packaged launcher has unexpected permissions.")
    for name in ("package-manifest.json", "build-manifest.json", "build-id.txt"):
        (out / name).write_bytes((package / name).read_bytes())
    (out / "verified-files.json").write_text(json.dumps({"files": written, "directories": manifest["directories"]}, indent=2) + "\n")
    result["package"] = {"directory": str(package), "manifest_sha256": hashlib.sha256(manifest_raw).hexdigest(),
                         "package_id": manifest["package_id"], "file_count": len(written), "directory_count": len(directories),
                         "uncompressed_bytes": total, "package_payload_sha256": payload.hexdigest(),
                         "unity_source_sha256": manifest["unity_source_sha256"], "base_revision": manifest["base_revision"]}
    result["checks"]["all_files_hashes_sizes_modes"] = "PASS"
    result["checks"]["build_package_identity"] = "PASS"
    return package


def player_pids():
    query = subprocess.run(["pgrep", "-x", "Signal47.x86_64"], text=True, capture_output=True)
    require(query.returncode in (0, 1), "Could not inspect existing SIGNAL 47 players.")
    return {int(value) for value in query.stdout.split()} if query.returncode == 0 else set()


def desktop_environment(runtime):
    env = os.environ.copy()
    env["XDG_RUNTIME_DIR"] = str(runtime)
    env["PULSE_SERVER"] = "unix:" + str(runtime / "pulse/native")
    # Normal startup must not inherit an observer location from an earlier QA run.
    env.pop("SIGNAL47_OBSERVER_DIR", None)
    candidates = []
    if env.get("DISPLAY") and env.get("XAUTHORITY"):
        candidates.append((env["DISPLAY"], Path(env["XAUTHORITY"])))
    auths = [Path.home() / ".Xauthority", *sorted(runtime.glob("*xauth*"))]
    for socket in sorted(Path("/tmp/.X11-unix").glob("X*")):
        candidates.extend((":" + socket.name[1:], auth) for auth in auths)
    for display, auth in candidates:
        if not auth.is_file():
            continue
        trial = {**env, "DISPLAY": display, "XAUTHORITY": str(auth)}
        if subprocess.run(["xdpyinfo"], env=trial, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=5).returncode == 0:
            return trial
    raise RuntimeError("Authenticated X11/XWayland desktop unavailable; no input sent.")


def desktop_class():
    specification = importlib.util.spec_from_file_location("signal47_existing_native_journey", ROOT / "Automation/gauntlet-user-journey.py")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module.Desktop


def close_desktop(desktop):
    if desktop is None:
        return
    try:
        desktop.close()
    finally:
        if getattr(desktop, "d", None):
            desktop.x.XCloseDisplay.argtypes = [C.c_void_p]
            desktop.x.XCloseDisplay.restype = C.c_int
            desktop.x.XCloseDisplay(desktop.d)
            desktop.d = None


def wait_for_desktop(game, timeout):
    cls = desktop_class()
    deadline = time.monotonic() + timeout
    last_error = "No matching window yet."
    while time.monotonic() < deadline:
        require(game.poll() is None, "Player exited before its window appeared.")
        require(not player_pids().difference({game.pid}), "Another SIGNAL 47 player appeared; no input sent.")
        candidate = cls.__new__(cls)
        try:
            cls.__init__(candidate)
            return candidate
        except AssertionError as error:
            last_error = str(error)
            close_desktop(candidate)
        time.sleep(.25)
    raise RuntimeError("Normal player window did not appear: " + last_error)


def window_pid(desktop):
    x, connection = desktop.x, desktop.d
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XGetWindowProperty.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.c_long, C.c_long, C.c_int, C.c_ulong,
                                    C.POINTER(C.c_ulong), C.POINTER(C.c_int), C.POINTER(C.c_ulong),
                                    C.POINTER(C.c_ulong), C.POINTER(C.POINTER(C.c_ubyte))]
    x.XGetWindowProperty.restype = C.c_int
    atom = x.XInternAtom(connection, b"_NET_WM_PID", 1)
    require(atom, "The game window exposes no process-id property; no input sent.")
    actual, count, remaining = C.c_ulong(), C.c_ulong(), C.c_ulong()
    format_bits, value = C.c_int(), C.POINTER(C.c_ubyte)()
    status = x.XGetWindowProperty(connection, desktop.w, atom, 0, 1, 0, 0,
                                  C.byref(actual), C.byref(format_bits), C.byref(count), C.byref(remaining), C.byref(value))
    try:
        require(status == 0 and format_bits.value == 32 and count.value == 1 and bool(value), "Could not identify the game window's process.")
        return C.cast(value, C.POINTER(C.c_ulong))[0]
    finally:
        if value:
            x.XFree(value)


def window_rectangle(desktop):
    x, connection = desktop.x, desktop.d
    x.XGetGeometry.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.c_ulong), C.POINTER(C.c_int), C.POINTER(C.c_int),
                              C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint), C.POINTER(C.c_uint)]
    x.XGetGeometry.restype = C.c_int
    root, child = C.c_ulong(), C.c_ulong()
    px, py, screen_x, screen_y = C.c_int(), C.c_int(), C.c_int(), C.c_int()
    width, height, border, depth = C.c_uint(), C.c_uint(), C.c_uint(), C.c_uint()
    require(x.XGetGeometry(connection, desktop.w, C.byref(root), C.byref(px), C.byref(py), C.byref(width), C.byref(height),
                           C.byref(border), C.byref(depth)), "Could not read the actual game client dimensions.")
    require((width.value, height.value) == (WIDTH, HEIGHT), f"Actual game window is {width.value}x{height.value}; expected unscaled 1280x800.")
    require(x.XTranslateCoordinates(connection, desktop.w, desktop.root, 0, 0, C.byref(screen_x), C.byref(screen_y), C.byref(child)),
            "Could not translate actual game-client origin to desktop coordinates.")
    root_width, root_height = C.c_uint(), C.c_uint()
    require(x.XGetGeometry(connection, desktop.root, C.byref(root), C.byref(px), C.byref(py), C.byref(root_width), C.byref(root_height),
                           C.byref(border), C.byref(depth)), "Could not read desktop bounds.")
    rectangle = (screen_x.value, screen_y.value, screen_x.value + width.value, screen_y.value + height.value)
    require(rectangle[0] >= 0 and rectangle[1] >= 0 and rectangle[2] <= root_width.value and rectangle[3] <= root_height.value,
            "The complete game client is not on-screen; no partial desktop capture will be accepted.")
    return rectangle


def exercise_package(package, out, env, result, startup_timeout):
    from PIL import Image

    desktop_class().require_unlocked()
    profile = out / "qa-profile"
    profile.mkdir(mode=0o700)
    launcher, player = package / "Start-SIGNAL47.sh", package / "Signal47.x86_64"
    arguments = [str(launcher), "--signal47-save-dir", str(profile), "-screen-width", str(WIDTH), "-screen-height", str(HEIGHT),
                 "-screen-fullscreen", "0", "-logFile", str(out / "player.log")]
    require(not any("gauntlet" in argument or "smoke" in argument for argument in arguments[1:] if argument.startswith("--")),
            "Observation/smoke mode is not allowed in normal package-start verification.")
    result["run"] = {"arguments": arguments, "cwd": str(package), "profile": str(profile), "display": env["DISPLAY"],
                     "pulse_server": env["PULSE_SERVER"], "observer_mode": False, "input_backend": "existing Desktop + Linux uinput",
                     "pid": None, "exit_code": None, "forced_cleanup": False}
    game, desktop = None, None
    saved_environment = {name: os.environ.get(name) for name in ("DISPLAY", "XAUTHORITY", "XDG_RUNTIME_DIR", "PULSE_SERVER")}
    try:
        for name in saved_environment:
            os.environ[name] = env[name]
        require(not player_pids(), "Another SIGNAL 47 player is running; no launch or input performed.")
        with (out / "startup.log").open("w", encoding="utf-8") as output:
            game = subprocess.Popen(arguments, cwd=package, env=env, stdout=output, stderr=subprocess.STDOUT)
            result["run"]["pid"] = game.pid
            desktop = wait_for_desktop(game, startup_timeout)
            require(window_pid(desktop) == game.pid, "SIGNAL 47 window belongs to another process; no input sent.")
            require(Path(f"/proc/{game.pid}/exe").resolve() == player.resolve(), "Launcher did not exec this extracted native player.")
            result["checks"]["normal_extracted_launcher"] = "PASS"
            desktop.inject_setup(extra_keys=[56])
            desktop.focus()

            def guard():
                require(game.poll() is None, "The package player exited before the requested sequence finished.")
                require(not player_pids().difference({game.pid}), "Another SIGNAL 47 player appeared; input stopped.")
                require(window_pid(desktop) == game.pid, "The game window changed ownership; input stopped.")
                desktop.guard()
                return window_rectangle(desktop)

            def settle(seconds):
                deadline = time.monotonic() + seconds
                while time.monotonic() < deadline:
                    guard()
                    time.sleep(min(.20, max(0, deadline - time.monotonic())))

            def mark(action, **details):
                result["input_events"].append({"action": action, "utc": utc(), "monotonic": time.monotonic(), **details})

            def capture(name, intended_state):
                rectangle = guard()
                path = out / name
                # XWayland exposes window geometry but cannot provide a root
                # GetImage capture on this KDE Wayland desktop. Use KDE's own
                # installed active-window capture, excluding frame and shadow.
                shot = subprocess.run(["spectacle", "--background", "--nonotify", "--activewindow",
                    "--no-decoration", "--no-shadow", "--output", str(path)],
                    env=env, capture_output=True, text=True, timeout=20)
                require(shot.returncode == 0 and path.is_file(), "KDE active-window capture failed: " + shot.stderr)
                with Image.open(path) as image:
                    width, height = image.size
                require((width, height) == (WIDTH, HEIGHT), "KDE capture is not the unscaled1280x800 game client.")
                desktop.guard()
                result["screenshots"].append({"file": name, "utc": utc(), "window_id": desktop.w, "window_pid": game.pid,
                    "actual_client_bbox": list(rectangle), "width": width, "height": height,
                    "sha256": digest_file(path), "intended_state": intended_state, "visual_content": "UNVERIFIED",
                    "capture_method": "Installed KDE Spectacle active-window capture, no decorations/shadows/notification; exact native client size checked against XGetGeometry. No resizing, retouching or full-desktop capture."})

            def focus_round_trip():
                guard()
                record = {"status": "UNVERIFIED", "focus_before": desktop.w, "requested_target": "next application through native Alt+Tab",
                          "focus_after_loss_request": None, "loss_observed": False,
                          "focus_after_reactivation": None, "return_observed": False,
                          "method": "Native Alt+Tab with both keys released, then Desktop.focus; no subsequent keyboard or mouse input to the other application."}
                result["focus_check"] = record
                result["checks"]["x11_focus_loss_and_return"] = "UNVERIFIED"
                require(not desktop.held, "Focus test requires every injected key to be released.")
                # Root-window focus requests are ignored by this Wayland
                # compositor. Use its normal application-switch shortcut.
                desktop.key("Alt", True)
                try:
                    desktop.key("Tab", True)
                    time.sleep(.12)
                    desktop.key("Tab", False)
                finally:
                    desktop.key("Alt", False)
                time.sleep(.5)
                focused, revert = C.c_ulong(), C.c_int()
                desktop.x.XGetInputFocus(desktop.d, C.byref(focused), C.byref(revert))
                record["focus_after_loss_request"] = focused.value
                record["loss_observed"] = focused.value != desktop.w
                mark("focus_loss_request", target="native Alt+Tab", observed_focus_window=focused.value,
                     loss_observed=record["loss_observed"])
                desktop.focus()
                desktop.x.XGetInputFocus(desktop.d, C.byref(focused), C.byref(revert))
                record["focus_after_reactivation"] = focused.value
                record["return_observed"] = focused.value == desktop.w
                mark("focus_reactivation", observed_focus_window=focused.value, return_observed=record["return_observed"])
                if record["loss_observed"] and record["return_observed"]:
                    record["status"] = "PASS"
                else:
                    record["limitation"] = "The compositor did not confirm both requested focus changes; the round trip remains UNVERIFIED."
                result["checks"]["x11_focus_loss_and_return"] = record["status"]
                # A rejected focus-loss request is recorded honestly. A failed
                # return additionally stops the run before any further input.
                guard()

            settle(4.0)
            capture("01-menu.png", "normal start menu")
            guard()
            desktop.click(WIDTH / 2, HEIGHT / 2 + 81)
            mark("mouse_click", target="START NIGHT SHIFT", client_x=WIDTH / 2, client_y=HEIGHT / 2 + 81,
                 effect="Input issued; actual menu transition requires screenshot review.")
            settle(3.0)
            focus_round_trip()
            settle(.4)
            for key, duration in (("w", .35), ("a", .18), ("s", .15), ("d", .12)):
                guard()
                desktop.key(key, True)
                mark("key_down", key=key)
                try:
                    settle(duration)
                finally:
                    desktop.key(key, False)
                    mark("key_up", key=key)
                settle(.15)
            settle(.7)
            capture("02-playing.png", "first-person play after short WASD inputs")
            guard()
            desktop.tap("Escape")
            mark("key_tap", key="Escape", intended_effect="open pause menu")
            settle(.8)
            capture("03-pause.png", "pause menu")
            guard()
            desktop.click(WIDTH / 2 + 62, HEIGHT / 2 + 168)
            mark("mouse_click", target="QUIT in pause menu", client_x=WIDTH / 2 + 62, client_y=HEIGHT / 2 + 168,
                 effect="Input issued; subsequent exit code is checked separately.")
            game.wait(timeout=20)
            result["run"]["exit_code"] = game.returncode
            require(game.returncode == 0, f"Normal QUIT ended with process exit code {game.returncode}.")
            result["checks"]["native_input_sequence_issued"] = "PASS"
            result["checks"]["zero_exit_after_quit_click"] = "PASS"
    finally:
        try:
            close_desktop(desktop)
        finally:
            # Popen owns exactly this PID. Never use pkill, killall, process-group
            # termination, or cleanup of another player's process/profile.
            if game is not None and game.poll() is None:
                result["run"]["forced_cleanup"] = True
                game.terminate()
                try:
                    game.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    game.kill()
                    game.wait(timeout=8)
            if game is not None:
                result["run"]["exit_code"] = game.returncode
            for name, old in saved_environment.items():
                if old is None:
                    os.environ.pop(name, None)
                else:
                    os.environ[name] = old


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--archive", required=True, type=Path)
    parser.add_argument("--extract-dir", required=True, type=Path, help="A NEW empty extraction container; never reuse a populated directory.")
    parser.add_argument("--out", type=Path, default=ROOT / "Artifacts/Chapter09/PackageStartup", help="New empty evidence directory.")
    parser.add_argument("--startup-timeout", type=float, default=45, help="Seconds to wait for the actual player window.")
    args = parser.parse_args()
    require(5 <= args.startup_timeout <= 180, "Startup timeout must be between 5 and 180 seconds.")
    archive = args.archive.expanduser().absolute()
    extraction_path, output_path = args.extract_dir.expanduser().absolute(), args.out.expanduser().absolute()
    require(extraction_path != output_path and extraction_path not in output_path.parents and output_path not in extraction_path.parents,
            "Extraction and evidence directories must be separate non-nested locations.")
    out = new_directory(output_path)
    result = {"schema": "signal47-chapter09-package-startup-v1", "started_utc": utc(), "ended_utc": None,
              "outcome": "FAIL", "error": None, "checks": {}, "input_events": [], "screenshots": [],
              "verification_script_sha256": digest_file(Path(__file__)),
              "desktop_source_sha256": digest_file(ROOT / "Automation/gauntlet-user-journey.py"),
              "visual_review": {"menu": "UNVERIFIED", "playing": "UNVERIFIED", "pause": "UNVERIFIED",
                                "requirement": "A reviewer must inspect the original captures for actual GUI state and any overlap. Issued input is not proof of the displayed state."}}
    code, lock_fd = 1, None

    def interrupt_for_cleanup(signum, frame):
        raise KeyboardInterrupt(f"Received signal {signum}")

    previous_sigterm = signal.signal(signal.SIGTERM, interrupt_for_cleanup)
    try:
        runtime = Path(f"/run/user/{os.getuid()}")
        require(runtime.is_dir(), "The current user's desktop runtime directory is unavailable.")
        lock_fd = os.open(runtime / "signal47-gauntlet.lock", os.O_WRONLY | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        result["exclusive_lock"] = str(runtime / "signal47-gauntlet.lock")
        require(not player_pids(), "Another SIGNAL 47 player is running; package startup refused before input.")
        extraction = new_directory(extraction_path)
        package = unpack_checked(archive, extraction, out, result)
        env = desktop_environment(runtime)
        exercise_package(package, out, env, result, args.startup_timeout)
        result["outcome"] = "AUTOMATION_PASS_VISUAL_UNVERIFIED"
        code = 0
    except KeyboardInterrupt:
        result["error"] = "Interrupted; only this run's process/input devices were cleaned up."
        code = 130
    except Exception as error:
        result["error"] = f"{type(error).__name__}: {error}"
    finally:
        signal.signal(signal.SIGTERM, previous_sigterm)
        result["ended_utc"] = utc()
        (out / "package-startup-result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        if lock_fd is not None:
            os.close(lock_fd)
    print(json.dumps({"outcome": result["outcome"], "result": str(out / "package-startup-result.json"), "error": result["error"],
                      "visual_review": "UNVERIFIED — inspect 01-menu.png, 02-playing.png and 03-pause.png"}, ensure_ascii=False, indent=2))
    return code


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as error:
        print(f"PACKAGE STARTUP REFUSED: {error}", file=sys.stderr)
        sys.exit(1)
