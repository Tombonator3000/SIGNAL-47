#!/usr/bin/env python3
"""Copy and verify the pinned VotV release for local, read-only research.

Only private/ below this research directory is written. The source installation
is never changed; an existing destination is verified, never overwritten.
No executable, asset payload, or decompiler is run by this script.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
import stat
import struct
import sys
import uuid

from pak_inspect import inspect_pak, sha256_file

ROOT = Path(__file__).resolve().parent.parent
SHIPPING = "VotV/Binaries/Win64/VotV-Win64-Shipping.exe"
PAK = "VotV/Content/Paks/VotV-WindowsNoEditor.pak"


def inventory(directory: Path) -> dict[str, dict]:
    result = {}
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError(f"Symlink in game input: {path}")
        if path.is_file():
            result[path.relative_to(directory).as_posix()] = {
                "size_bytes": path.stat().st_size, "sha256": sha256_file(path)
            }
    return result


def inspect_pe(path: Path) -> dict:
    data = path.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError("Shipping executable has no DOS header")
    nt = struct.unpack_from("<I", data, 0x3c)[0]
    if data[nt:nt + 4] != b"PE\0\0":
        raise ValueError("Shipping executable has no PE signature")
    machine, count = struct.unpack_from("<HH", data, nt + 4)
    optional_size = struct.unpack_from("<H", data, nt + 20)[0]
    opt = nt + 24
    magic = struct.unpack_from("<H", data, opt)[0]
    if machine != 0x8664 or magic != 0x20b or optional_size < 240:
        raise ValueError("Expected a Windows AMD64 PE32+ executable")
    if struct.unpack_from("<I", data, opt + 108)[0] < 15:
        raise ValueError("Missing PE data directories")
    clr = struct.unpack_from("<II", data, opt + 112 + 14 * 8)
    if clr != (0, 0):
        raise ValueError("This preparation expects native code, not a CLR assembly")
    sections = opt + optional_size

    def rva_to_offset(rva: int) -> int:
        for index in range(count):
            section = sections + 40 * index
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, section + 8)
            if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
                offset = raw_offset + rva - virtual_address
                if not 0 <= offset < len(data):
                    break
                return offset
        raise ValueError("PE directory does not map to file data")

    debug_rva, debug_size = struct.unpack_from("<II", data, opt + 112 + 6 * 8)
    symbols = []
    if debug_rva:
        debug = rva_to_offset(debug_rva)
        for item in range(debug_size // 28):
            kind, size, _, pointer = struct.unpack_from("<IIII", data, debug + item * 28 + 12)
            if kind == 2 and size >= 25 and data[pointer:pointer + 4] == b"RSDS":
                symbols.append({
                    "pdb_name": data[pointer + 24:pointer + size].split(b"\0", 1)[0].decode("utf-8"),
                    "guid": str(uuid.UUID(bytes_le=data[pointer + 4:pointer + 20])),
                    "age": struct.unpack_from("<I", data, pointer + 20)[0],
                })
    # This offset belongs to the SHA-256-pinned release, not to arbitrary PE files.
    version_offset = 83421720
    if struct.unpack_from("<I", data, version_offset)[0] != 0xfeef04bd:
        raise ValueError("Pinned VS_FIXEDFILEINFO record is missing")
    version_ms, version_ls = struct.unpack_from("<II", data, version_offset + 8)
    version = f"{version_ms >> 16}.{version_ms & 65535}.{version_ls >> 16}.{version_ls & 65535}"
    product = "++UE4+Release-4.27-CL-18319896"
    if product.encode("utf-16-le") not in data:
        raise ValueError("Pinned Unreal ProductVersion is missing")
    return {
        "machine": "AMD64", "format": "PE32+", "native_code": True,
        "clr_directory": list(clr), "file_version": version,
        "product_version": product, "fixed_file_info_offset": version_offset,
        "codeview_references": symbols,
    }


def prepare(source: Path, archive: Path | None = None) -> dict:
    release = json.loads((ROOT / "release.json").read_text())
    source = source.expanduser().resolve(strict=True)
    if not source.is_dir():
        raise ValueError("Source must be the extracted WindowsNoEditor directory")
    archive_check = None
    if archive:
        archive = archive.expanduser().resolve(strict=True)
        archive_check = {"size_bytes": archive.stat().st_size, "sha256": sha256_file(archive)}
        if any(archive_check[key] != release["archive"][key] for key in archive_check):
            raise ValueError("Archive does not match the official pinned release")
    source_files = inventory(source)
    manifest_digest = hashlib.sha256(json.dumps(source_files, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    if manifest_digest != release["source_inventory_sha256"]:
        raise ValueError("Complete source-file hash inventory differs from the pinned release")
    if len(source_files) != release["loose_file_count"] or sum(f["size_bytes"] for f in source_files.values()) != release["loose_total_bytes"]:
        raise ValueError("Loose game inventory does not match this release")
    for name, expected in release["files"].items():
        if source_files.get(name) != expected:
            raise ValueError(f"Pinned file is missing or different: {name}")

    private = ROOT / "private"
    destination = private / "game" / release["version"] / "WindowsNoEditor"
    if destination.resolve() == source:
        raise ValueError("Source and research destination must be different")
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, destination)
    copy_files = inventory(destination)
    if copy_files != source_files:
        raise ValueError("Existing research copy differs; it has not been overwritten")
    for name in source_files:
        if (destination / name).samefile(source / name):
            raise ValueError("Research copy shares a file with the original; no permissions have been changed")
        if (destination / name).stat().st_nlink != 1:
            raise ValueError("Research copy contains shared hardlinks; no permissions have been changed")
    for path in destination.rglob("*"):
        if path.is_file():
            path.chmod(stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
    pak_summary, names = inspect_pak(destination / PAK)
    pe_summary = inspect_pe(destination / SHIPPING)
    source_extensions = {".cpp", ".c", ".h", ".hpp", ".cs", ".sln", ".map", ".pdb"}
    loose_source = [name for name in source_files if Path(name).suffix.lower() in source_extensions]
    reports = private / "reports"
    reports.mkdir(parents=True, exist_ok=True)
    (reports / "source-files.json").write_text(json.dumps(source_files, indent=2) + "\n")
    (reports / "pak-files.txt").write_text("\n".join(names) + "\n")
    result = {
        "checked_at_utc": datetime.now(timezone.utc).isoformat(),
        "release": release["version"], "source_directory": str(source),
        "research_copy": str(destination.resolve()),
        "source_file_count": len(source_files),
        "source_total_bytes": sum(f["size_bytes"] for f in source_files.values()),
        "source_inventory_sha256": manifest_digest,
        "all_copy_hashes_match_source": True, "copy_files_read_only": True,
        "archive_check": archive_check, "pe": pe_summary, "pak": pak_summary,
        "loose_source_or_symbol_candidates": loose_source,
        "license_status": "No license for reusing the game's own code has been established",
        "decompilation_done": False, "asset_payload_extraction_done": False,
    }
    (reports / "preparation.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True, help="Installed/extracted WindowsNoEditor directory")
    parser.add_argument("--archive", type=Path, help="Optional official 090n.7z checksum check")
    args = parser.parse_args()
    try:
        result = prepare(args.source, args.archive)
        print(json.dumps({key: result[key] for key in (
            "research_copy", "source_file_count", "source_total_bytes",
            "all_copy_hashes_match_source", "copy_files_read_only",
        )}, indent=2))
        return 0
    except (OSError, ValueError, UnicodeError, struct.error) as error:
        print(f"Preparation stopped: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
