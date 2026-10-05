#!/usr/bin/env python3
"""Inspect UE4.27 PAK v11 metadata; never extract/interpret asset payloads.

Supported layout: 221-byte FPakInfo footer, v11 path-hash/full-directory indexes.
Encrypted indexes and unrecognized versions are rejected without key handling.
This independent parser was checked against the local VotV 0.9.0n release.
Field names and version labels: Epic's FPakInfo / FPakFile API documentation.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import sys


PAK_MAGIC = 0x5A6F12E1
FOOTER_SIZE = 221
SUPPORTED_VERSION = 11
MAX_INDEX_BYTES = 128 * 1024 * 1024
MAX_ENTRIES = 2_000_000


class InspectError(ValueError):
    """Invalid, unsupported, or protected metadata."""


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def read(self, count: int) -> bytes:
        if count < 0 or self.offset + count > len(self.data):
            raise InspectError("Index is truncated or its length is invalid")
        value = self.data[self.offset:self.offset + count]
        self.offset += count
        return value

    def number(self, fmt: str) -> int:
        return struct.unpack(fmt, self.read(struct.calcsize(fmt)))[0]

    def fstring(self) -> str:
        count = self.number("<i")
        if not count:
            return ""
        if abs(count) > 1_000_000:
            raise InspectError("FString length is outside the supported bounds")
        if count > 0:
            data = self.read(count)
            if data[-1:] != b"\0":
                raise InspectError("FString has no NUL terminator")
            return data[:-1].decode("utf-8")
        data = self.read(-2 * count)
        if data[-2:] != b"\0\0":
            raise InspectError("UTF-16 FString has no NUL terminator")
        return data[:-2].decode("utf-16-le")

    def require_end(self) -> None:
        if self.offset != len(self.data):
            raise InspectError("Unexpected trailing bytes in index")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(8 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def inspect_pak(path: Path, include_sha256: bool = False) -> tuple[dict, list[str]]:
    """Validate metadata and file names; optional SHA-256 hashes the entire file."""
    size = path.stat().st_size
    footer_offset = size - FOOTER_SIZE
    if footer_offset < 0:
        raise InspectError("File is shorter than the supported PAK footer")

    with path.open("rb") as source:
        source.seek(footer_offset)
        footer = source.read(FOOTER_SIZE)
        magic, version, index_offset, index_size = struct.unpack_from("<Iiqq", footer, 17)
        if magic != PAK_MAGIC:
            raise InspectError("Not a recognized PAK v11 footer")
        if version != SUPPORTED_VERSION:
            raise InspectError(f"Unsupported PAK version {version}; expected 11")
        encrypted = footer[16]
        if encrypted not in (0, 1):
            raise InspectError("Invalid encrypted-index flag")
        if encrypted:
            raise InspectError("Encrypted index: inspection stops; no key or bypass supported")

        def index_blob(offset: int, length: int, expected_hash: bytes) -> tuple[dict, bytes]:
            if length < 0 or length > MAX_INDEX_BYTES or offset < 0 or offset + length > footer_offset:
                raise InspectError("Index range is outside the file or supported bounds")
            source.seek(offset)
            data = source.read(length)
            if len(data) != length:
                raise InspectError("Index is truncated")
            actual_hash = hashlib.sha1(data).digest()
            if actual_hash != expected_hash:
                raise InspectError("Index SHA-1 does not match the PAK metadata")
            return {
                "offset": offset,
                "size_bytes": length,
                "sha1": actual_hash.hex(),
                "hash_valid": True,
            }, data

        main, index = index_blob(index_offset, index_size, footer[41:61])
        reader = Reader(index)
        mount_point = reader.fstring()
        entry_count = reader.number("<i")
        if not 0 <= entry_count <= MAX_ENTRIES:
            raise InspectError("Entry count is outside supported bounds")
        path_hash_seed = reader.number("<Q")
        indexes = {"main": main}
        secondary_data = {}
        for name in ("path_hash", "full_directory"):
            present = reader.number("<i")
            if present not in (0, 1):
                raise InspectError("Invalid secondary-index presence flag")
            if present:
                offset = reader.number("<q")
                length = reader.number("<q")
                expected_hash = reader.read(20)
                indexes[name], secondary_data[name] = index_blob(offset, length, expected_hash)
        encoded_length = reader.number("<i")
        if not 0 <= encoded_length <= MAX_INDEX_BYTES:
            raise InspectError("Encoded-entry metadata length is outside supported bounds")
        reader.read(encoded_length)
        non_encoded_count = reader.number("<i")
        if non_encoded_count:
            raise InspectError("Non-encoded entry layout is unsupported by this metadata-only parser")
        reader.require_end()
        if "full_directory" not in secondary_data:
            raise InspectError("No full directory index; a complete file-name list is unavailable")

        reader = Reader(secondary_data["full_directory"])
        directory_count = reader.number("<i")
        if not 0 <= directory_count <= MAX_ENTRIES:
            raise InspectError("Directory count is outside supported bounds")
        names = []
        seen = set()
        for _ in range(directory_count):
            directory = reader.fstring()
            count = reader.number("<i")
            if not 0 <= count <= entry_count or len(names) + count > entry_count:
                raise InspectError("Directory file count exceeds the index entry count")
            for _ in range(count):
                name = directory + reader.fstring()
                location = reader.number("<i")
                if not 0 <= location <= encoded_length - 4:
                    raise InspectError("Directory entry location is outside encoded metadata")
                if name in seen:
                    raise InspectError("Directory index contains duplicate paths")
                if name.startswith("/") or ".." in PurePosixPath(name).parts:
                    raise InspectError("Directory index contains an unsafe path")
                seen.add(name)
                names.append(name)
        reader.require_end()
        if len(names) != entry_count:
            raise InspectError("Directory list does not match main-index entry count")

    source_extensions = {".pdb", ".cpp", ".c", ".h", ".hpp", ".cs", ".sln", ".map"}
    summary = {
        "size_bytes": size,
        "footer_offset": footer_offset,
        "footer_size_bytes": FOOTER_SIZE,
        "pak_magic": f"0x{magic:08x}",
        "pak_version": version,
        "pak_version_label": "PakFile_Version_Fnv64BugFix",
        "encrypted_index": False,
        "encryption_key_guid_hex": footer[:16].hex(),
        "compression_methods": [
            footer[61 + n * 32:93 + n * 32].split(b"\0", 1)[0].decode("ascii")
            for n in range(5)
        ],
        "indexes": indexes,
        "mount_point": mount_point,
        "path_hash_seed": path_hash_seed,
        "encoded_entry_bytes": encoded_length,
        "non_encoded_entry_count": non_encoded_count,
        "file_count": entry_count,
        "directory_count": directory_count,
        "extensions": dict(sorted(Counter(PurePosixPath(n).suffix.lower() for n in names).items())),
        "source_or_symbol_candidates": [n for n in names if PurePosixPath(n).suffix.lower() in source_extensions],
        "project_descriptors": [n for n in names if n.lower().endswith(".uproject")],
        "license_filename_candidates": [n for n in names if re.search(r"license|copyright|readme|eula|build\.version", n, re.I)],
        "scope": "footer and indexes only; asset payloads, Blueprints and executable code are not inspected",
    }
    if include_sha256:
        summary["sha256"] = sha256_file(path)
    return summary, sorted(names)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pak", type=Path)
    parser.add_argument("--sha256", action="store_true", help="Also hash the complete input file")
    parser.add_argument("--files", action="store_true", help="Include the validated file-name list in JSON")
    parser.add_argument("--list", action="store_true", help="Print validated file names, one per line")
    args = parser.parse_args()
    try:
        summary, names = inspect_pak(args.pak, args.sha256)
        if args.list:
            print("\n".join(names))
        else:
            if args.files:
                summary["files"] = names
            print(json.dumps(summary, ensure_ascii=False, indent=2))
    except (OSError, UnicodeError, struct.error, InspectError) as error:
        print(f"PAK inspection stopped: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
