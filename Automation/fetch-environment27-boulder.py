#!/usr/bin/env python3
"""Reproduce the pinned Poly Haven Boulder 01 authoring source.

The five payloads are pinned to the official Poly Haven API response. Existing
files are hashed and reused; the helper never replaces a present file. The
GLTF texture aliases are local links/copies only, so a fresh checkout can be
opened by ``Unity/Blender/Source/environment27_boulder.py`` without another
network lookup.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import sys
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "Artifacts/Environment27/StagedBoulder/Source/Boulder01"

# URLs and hashes are copied from https://api.polyhaven.com/files/boulder_01.
FILES = (
    (
        "boulder_01_2k.gltf",
        "https://dl.polyhaven.org/file/ph-assets/Models/gltf/2k/boulder_01/boulder_01_2k.gltf",
        "52d118b56748a18509e4ec0075d07b254d72a0401793f22363c0bb3a682e7361",
    ),
    (
        "boulder_01.bin",
        "https://dl.polyhaven.org/file/ph-assets/Models/gltf/8k/boulder_01/boulder_01.bin",
        "7f5b06503d62bd95bfe9c6b8a354a5cc8ff04a4d271a8cf87a3e3125db4232d7",
    ),
    (
        "boulder_01_diff_2k.jpg",
        "https://dl.polyhaven.org/file/ph-assets/Models/jpg/2k/boulder_01/boulder_01_diff_2k.jpg",
        "90bbaa17c1fe0254d2b0b6e5148264fa834953a72724d566f14af029eebe3ca3",
    ),
    (
        "boulder_01_nor_gl_2k.jpg",
        "https://dl.polyhaven.org/file/ph-assets/Models/jpg/2k/boulder_01/boulder_01_nor_gl_2k.jpg",
        "5174b318712ac7725cbcb55d422607f9c66d7f8d035c5412ab897b5939e2db6b",
    ),
    (
        "boulder_01_arm_2k.jpg",
        "https://dl.polyhaven.org/file/ph-assets/Models/jpg/2k/boulder_01/boulder_01_arm_2k.jpg",
        "ab52859dde519b4822111aa60916b6d21f8a6966c67b6cec44d1cd0775b8a86a",
    ),
)

TEXTURE_ALIASES = tuple(name for name, _, _ in FILES if name.endswith(".jpg"))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download(url: str, destination: Path) -> None:
    request = urllib.request.Request(
        url, headers={"User-Agent": "SIGNAL47-Environment27-Reproducer/1.0"}
    )
    temporary = destination.with_name(destination.name + ".part")
    try:
        with urllib.request.urlopen(request, timeout=90) as response, temporary.open("wb") as output:
            shutil.copyfileobj(response, output, length=1024 * 1024)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def ensure_texture_aliases(check_only: bool) -> bool:
    """Make the paths referenced by the downloaded GLTF resolve locally."""
    alias_dir = SOURCE_DIR / "textures"
    alias_dir.mkdir(parents=True, exist_ok=True) if not check_only else None
    okay = True
    for name in TEXTURE_ALIASES:
        source = SOURCE_DIR / name
        alias = alias_dir / name
        if alias.exists():
            if alias.is_symlink() and alias.resolve() == source.resolve():
                print(f"LINK {name}")
            elif alias.is_file() and sha256(alias) == sha256(source):
                print(f"LINK_COPY {name}")
            else:
                print(f"ERROR texture alias mismatch: {alias}", file=sys.stderr)
                okay = False
            continue
        if check_only:
            print(f"MISSING texture alias {alias.relative_to(ROOT)}")
            okay = False
            continue
        # A symlink avoids a second copy of the pinned texture. Fall back to a
        # copy for filesystems where symlinks are unavailable.
        try:
            alias.symlink_to(Path("..") / name)
        except OSError:
            shutil.copy2(source, alias)
        print(f"LINKED {name}")
    return okay


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="verify the five pinned files and local GLTF aliases without downloading",
    )
    args = parser.parse_args()
    if not args.check_only:
        SOURCE_DIR.mkdir(parents=True, exist_ok=True)

    okay = True
    for name, url, expected in FILES:
        path = SOURCE_DIR / name
        if path.exists():
            actual = sha256(path)
            if actual != expected:
                print(
                    f"ERROR hash mismatch {path.relative_to(ROOT)}: {actual} != {expected}",
                    file=sys.stderr,
                )
                okay = False
            else:
                print(f"REUSE {name} {actual}")
            continue
        if args.check_only:
            print(f"MISSING {path.relative_to(ROOT)}")
            okay = False
            continue
        print(f"FETCH {name}")
        try:
            download(url, path)
        except Exception as exc:  # keep the message useful on offline machines
            print(f"ERROR download {name}: {exc}", file=sys.stderr)
            okay = False
            continue
        actual = sha256(path)
        if actual != expected:
            print(f"ERROR downloaded hash mismatch {name}: {actual} != {expected}", file=sys.stderr)
            path.unlink(missing_ok=True)
            okay = False
        else:
            print(f"FETCHED {name} {actual}")

    if all((SOURCE_DIR / name).is_file() for name, _, _ in FILES):
        okay = ensure_texture_aliases(args.check_only) and okay
    else:
        print("SKIP texture aliases until all five pinned files exist")

    if okay:
        print(f"ENVIRONMENT27_BOULDER_SOURCE_PASS 5 files at {SOURCE_DIR.relative_to(ROOT)}")
        return 0
    print("ENVIRONMENT27_BOULDER_SOURCE_FAIL", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
