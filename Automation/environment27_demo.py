#!/usr/bin/env python3
"""Stage and initialize the small, disposable STATION 01 demo profile.

The packaged seed deliberately contains relative photo names only.  The
initializer verifies those bytes, maps them into a fresh user profile, and
leaves the normal SIGNAL / 47 checkpoint untouched.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import sys
import uuid


CHUNK = 1024 * 1024
DEMO_ROOT = Path("Signal47") / "Environment27Demo"
REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_SEED = REPO_ROOT / "Docs/Environment27/DemoSource/case.json"
DEMO_METADATA = (json.dumps({
    "schema": "signal47-environment27-demo-seed-v1",
    "source": "Station26 completed WorldCase seed; historical first two frames retained",
    "cameraFramesKept": ["s03-field-photograph", "b12-control-photograph"],
    "stationState": "version 1 with inStation=true; P06-P09 fields omitted/reset",
    "notebookEntriesRemoved": ["FRAME 03", "FRAME 04", "PHOTO 03", "PHOTO 04"],
    "playerPose": {"x": 120.0, "y": 0.12, "z": 94.0, "yaw": 0.0, "pitch": 0.0},
    "photoPaths": "relative basenames in package; initializer remaps to the dedicated profile"
}, ensure_ascii=False, indent=2) + "\n").encode("utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest_bytes(data: bytes) -> str:
    digest = hashlib.sha256()
    for offset in range(0, len(data), CHUNK):
        digest.update(data[offset:offset + CHUNK])
    return digest.hexdigest()


def file_bytes(path: Path) -> bytes:
    require(path.is_file() and not path.is_symlink(), f"Expected a regular file: {path}")
    return path.read_bytes()


def reject_symlink_tree(path: Path) -> None:
    require(not path.is_symlink(), f"Refusing symbolic link: {path}")
    if not path.exists():
        return
    for child in path.rglob("*"):
        require(not child.is_symlink(), f"Refusing symbolic link: {child}")


def install_exact(path: Path, data: bytes, mode: int) -> None:
    """Create one immutable file, or accept the exact existing copy."""
    reject_symlink_tree(path)
    if path.exists():
        require(path.is_file() and path.read_bytes() == data, f"Existing immutable output differs: {path}")
        require(stat.S_IMODE(path.stat().st_mode) == mode, f"Existing immutable permissions differ: {path}")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    reject_symlink_tree(path.parent)
    with path.open("xb") as stream:
        stream.write(data)
    path.chmod(mode)


def source_photo(source_seed: Path, frame: dict) -> tuple[str, bytes]:
    raw_name = frame.get("path", "")
    name = Path(raw_name).name
    require(name and name == raw_name if not Path(raw_name).is_absolute() else bool(name),
            f"Invalid source frame path: {raw_name}")
    # Source frames historically use absolute paths; only the basename crosses
    # into the package, after the actual bytes and digest have been checked.
    candidate = Path(raw_name) if Path(raw_name).is_absolute() else source_seed.parent / "FieldPhotos" / name
    if not candidate.is_file():
        candidate = source_seed.parent / "FieldPhotos" / name
    data = file_bytes(candidate)
    require(frame.get("sha256", "").lower() == digest_bytes(data), f"Source photo hash mismatch: {candidate}")
    return name, data


def make_demo_seed(source_seed: Path) -> tuple[bytes, dict[str, bytes]]:
    source_seed = Path(source_seed)
    reject_symlink_tree(source_seed)
    source_seed = source_seed.resolve()
    reject_symlink_tree(source_seed)
    envelope = json.loads(file_bytes(source_seed).decode("utf-8"))
    payload_text = envelope["payload"]
    require(envelope.get("sha256", "").lower() == digest_bytes(payload_text.encode("utf-8")), "Source case envelope checksum mismatch")
    snapshot = json.loads(payload_text)
    chapter = json.loads(snapshot["chapter"])
    world_case = json.loads(snapshot["worldCase"])
    station = json.loads(snapshot["station"])
    camera = json.loads(snapshot["camera"])
    require(chapter.get("complete") is True, "Source seed must contain a completed chapter")
    require(world_case.get("p05Complete") is True and world_case.get("destination") and world_case.get("surveyId"),
            "Source seed must contain a completed WorldCase destination")
    require(station.get("version") == 1 and len(camera.get("frames", [])) >= 2,
            "Source seed must contain Station26 v1 state and at least two frames")
    historical = camera["frames"][:2]
    require([frame.get("id") for frame in historical] == ["s03-field-photograph", "b12-control-photograph"],
            "The first two source frames must be the historical pair")

    photos: dict[str, bytes] = {}
    normalized_frames = []
    for frame in historical:
        name, data = source_photo(source_seed, frame)
        require(name not in photos, f"Duplicate historical photo basename: {name}")
        photos[name] = data
        normalized = dict(frame)
        normalized["path"] = name
        normalized["sha256"] = digest_bytes(data)
        normalized_frames.append(normalized)

    demo_camera = dict(camera)
    demo_camera["frames"] = normalized_frames
    demo_station = {"version": 1, "inStation": True}
    demo_snapshot = dict(snapshot)
    demo_snapshot["camera"] = json.dumps(demo_camera, separators=(",", ":"), ensure_ascii=False)
    demo_snapshot["station"] = json.dumps(demo_station, separators=(",", ":"), ensure_ascii=False)
    demo_snapshot["playerPosition"] = {"x": 120.0, "y": 0.12, "z": 94.0}
    demo_snapshot["playerYaw"] = 0.0
    demo_snapshot["playerPitch"] = 0.0
    notebook = json.loads(snapshot["notebook"])
    notebook["entries"] = [entry for entry in notebook.get("entries", [])
                            if not any(token in entry for token in ("FRAME 03", "FRAME 04", "PHOTO 03", "PHOTO 04"))]
    demo_snapshot["notebook"] = json.dumps(notebook, separators=(",", ":"), ensure_ascii=False)
    demo_payload = json.dumps(demo_snapshot, separators=(",", ":"), ensure_ascii=False)
    demo_envelope = dict(envelope)
    demo_envelope["payload"] = demo_payload
    demo_envelope["sha256"] = digest_bytes(demo_payload.encode("utf-8")).upper()
    case_data = (json.dumps(demo_envelope, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    return case_data, photos


INITIALIZER = r'''#!/usr/bin/env python3
"""Create a fresh persistent STATION 01 demo profile without touching Chapter09."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import sys
import uuid

CHUNK = 1024 * 1024
DEMO_ROOT = Path("Signal47") / "Environment27Demo"

def require(condition, message):
    if not condition:
        raise ValueError(message)

def digest(data):
    h = hashlib.sha256()
    for offset in range(0, len(data), CHUNK):
        h.update(data[offset:offset + CHUNK])
    return h.hexdigest()

def reject_tree(path):
    require(not path.is_symlink(), f"Refusing symbolic link: {path}")
    if path.exists():
        for child in path.rglob("*"):
            require(not child.is_symlink(), f"Refusing symbolic link: {child}")

def exact_create(path, data, mode=0o644):
    reject_tree(path)
    if path.exists():
        require(path.is_file() and path.read_bytes() == data, f"Existing immutable file differs: {path}")
        require(stat.S_IMODE(path.stat().st_mode) == mode, f"Existing immutable mode differs: {path}")
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    reject_tree(path.parent)
    with path.open("xb") as stream:
        stream.write(data)
    path.chmod(mode)

def data_root():
    configured = os.environ.get("XDG_DATA_HOME", "").strip()
    return (Path(configured) if configured else Path.home() / ".local" / "share") / DEMO_ROOT

def read_seed(package_root):
    seed = package_root / "DemoSeed"
    case_path = seed / "case.json"
    reject_tree(seed)
    case_bytes = case_path.read_bytes()
    envelope = json.loads(case_bytes.decode("utf-8"))
    payload = envelope["payload"]
    require(envelope.get("sha256", "").lower() == digest(payload.encode("utf-8")), "Packaged seed checksum mismatch")
    snapshot = json.loads(payload)
    chapter = json.loads(snapshot["chapter"])
    world = json.loads(snapshot["worldCase"])
    station = json.loads(snapshot["station"])
    camera = json.loads(snapshot["camera"])
    require(chapter.get("complete") is True and world.get("p05Complete") is True, "Packaged seed is not a completed WorldCase")
    require(station == {"version": 1, "inStation": True}, "Packaged seed contains unexpected Station26 progress")
    require(snapshot.get("playerPosition") == {"x": 120.0, "y": 0.12, "z": 94.0} and snapshot.get("playerYaw") == 0 and snapshot.get("playerPitch") == 0,
            "Packaged seed has unexpected Station01 starting pose")
    frames = camera.get("frames", [])
    require(len(frames) == 2, "Packaged demo must contain exactly two historical frames")
    photos = {}
    for frame in frames:
        name = frame.get("path", "")
        require(name and Path(name).name == name and not Path(name).is_absolute(), "Packaged seed contains an absolute photo path")
        data = (seed / "FieldPhotos" / name).read_bytes()
        require(frame.get("sha256", "").lower() == digest(data), f"Packaged photo checksum mismatch: {name}")
        photos[name] = data
    return snapshot, envelope, photos

def select_profile(root, requested):
    reject_tree(root)
    root.mkdir(parents=True, exist_ok=True)
    reject_tree(root)
    if requested:
        requested_path = Path(requested).expanduser()
        require(not requested_path.is_symlink(), f"Refusing symbolic link: {requested_path}")
        profile = requested_path.resolve()
        require(profile != root and (profile == root or root in profile.parents), "Profile must be inside Environment27Demo")
        if profile.exists():
            reject_tree(profile)
            return profile
        profile.parent.mkdir(parents=True, exist_ok=True)
        reject_tree(profile.parent)
        profile.mkdir()
        return profile
    profile = root / "current"
    require(not profile.is_symlink(), f"Refusing symbolic link: {profile}")
    if profile.exists():
        reject_tree(profile)
        return profile
    profile.mkdir()
    return profile

def initialize(package_root, requested=None):
    snapshot, envelope, photos = read_seed(package_root)
    root = data_root()
    profile = select_profile(root, requested)
    case_path = profile / "case.json"
    if case_path.exists():
        # Idempotent and immutable: an existing run is never rewritten or
        # silently changed to a different case.
        reject_tree(profile)
        return profile
    children = list(profile.iterdir())
    require(not children, "Existing profile has no case but already contains files; refusing to overwrite it")
    photo_dir = profile / "FieldPhotos"
    reject_tree(photo_dir)
    remapped = json.loads(json.dumps(snapshot))
    camera = json.loads(remapped["camera"])
    for frame in camera["frames"]:
        name = frame["path"]
        frame["path"] = str(photo_dir / name)
    remapped["camera"] = json.dumps(camera, separators=(",", ":"), ensure_ascii=False)
    payload = json.dumps(remapped, separators=(",", ":"), ensure_ascii=False)
    runtime_envelope = dict(envelope)
    runtime_envelope["payload"] = payload
    runtime_envelope["sha256"] = digest(payload.encode("utf-8")).upper()
    runtime_case = (json.dumps(runtime_envelope, ensure_ascii=False, indent=2) + "\n").encode("utf-8")
    # All bytes and hashes were checked above, before any profile file is made.
    for name, data in photos.items():
        exact_create(photo_dir / name, data)
    exact_create(case_path, runtime_case)
    return profile

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", type=Path)
    parser.add_argument("--print-profile", action="store_true")
    parser.add_argument("--check-only", action="store_true")
    args = parser.parse_args()
    package_root = Path(__file__).resolve().parent
    if args.check_only:
        read_seed(package_root)
        print("verified")
        return
    profile = initialize(package_root, args.profile)
    if args.print_profile:
        print(profile)

if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Environment27 demo initialization failed: {error}", file=sys.stderr)
        raise SystemExit(2)
'''


STARTER = r'''#!/usr/bin/env bash
set -euo pipefail
signal47_station_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
signal47_demo_profile="$(python3 "$signal47_station_dir/initialize-station01.py" --print-profile)"
# Leave the player at its real menu. The user chooses CONTINUE CHECKPOINT.
exec "$signal47_station_dir/Start-SIGNAL47.sh" --signal47-save-dir "$signal47_demo_profile" "$@"
'''


def prepare(stage: Path, source_seed: Path) -> Path:
    """Add the immutable demo seed and launch helpers to a package stage."""
    stage = Path(stage).resolve()
    require(not stage.is_symlink() and stage.is_dir(), f"Package stage must be a directory: {stage}")
    case_data, photos = make_demo_seed(Path(source_seed))
    demo_seed = stage / "DemoSeed"
    install_exact(demo_seed / "case.json", case_data, 0o644)
    for name, data in photos.items():
        install_exact(demo_seed / "FieldPhotos" / name, data, 0o644)
    install_exact(demo_seed / "fixture-metadata.json", DEMO_METADATA, 0o644)
    install_exact(stage / "initialize-station01.py", INITIALIZER.encode("utf-8"), 0o755)
    install_exact(stage / "Start-STATION01.sh", STARTER.encode("utf-8"), 0o755)
    return stage


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--source-seed", type=Path, default=DEFAULT_SOURCE_SEED)
    parser.add_argument("--check-only", action="store_true", help="Validate the source without writing the stage.")
    args = parser.parse_args(argv)
    case_data, photos = make_demo_seed(args.source_seed)
    if not args.check_only:
        prepare(args.stage, args.source_seed)
    print(json.dumps({"stage": str(args.stage.resolve()), "case_bytes": len(case_data), "photos": sorted(photos)}, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"Environment27 demo staging failed: {error}", file=sys.stderr)
        raise SystemExit(2)
