#!/usr/bin/env python3
"""Convert built-in ImageGen PNG outputs to compact, source-bound JPG assets.

Input JSON must be outside the repository and contain
{"assets": [{"id": "...", "prompt": "full prompt", "source_path": "/absolute/image.png"}]}.
No missing/corrupt source or failed size target can produce a partial write.
Only format conversion, color management and downscaling are performed.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile

from PIL import Image, ImageCms

from build_gallery import gallery_bytes, overview_bytes
from build_catalog import EXPECTED_IDS


ASSET_ROOT = Path(__file__).resolve().parent.parent
REPO = Path(__file__).resolve().parents[4]
MAX_EDGE = 2048
MAX_BYTES = 650 * 1024
QUALITIES = (86, 84, 82, 80, 78)
SRGB_PROFILE = ImageCms.ImageCmsProfile(ImageCms.createProfile("sRGB"))
SRGB_ICC = SRGB_PROFILE.tobytes()


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def json_bytes(value: object) -> bytes:
    return (json.dumps(value, ensure_ascii=False, indent=2) + "\n").encode("utf-8")


def atomic_write(path: Path, content: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary: str | None = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix="." + path.name + ".", suffix=".tmp", delete=False) as stream:
            temporary = stream.name
            stream.write(content)
        os.replace(temporary, path)
    finally:
        if temporary is not None and os.path.exists(temporary):
            os.unlink(temporary)


def output_path(asset_root: Path, filename: str) -> Path:
    relative = Path(filename)
    if relative.is_absolute() or len(relative.parts) != 2 or relative.parts[0] != "jpg" or relative.suffix != ".jpg":
        raise ValueError(f"Unsafe or unexpected catalog filename: {filename!r}")
    destination = (asset_root / relative).resolve()
    if not destination.is_relative_to(asset_root.resolve()):
        raise ValueError("Image destination escapes asset directory")
    return destination


def convert_source(entry: dict, filename: str) -> tuple[bytes, dict]:
    path = Path(entry["source_path"])
    if not path.is_absolute() or not path.is_file():
        raise ValueError(f"Missing absolute source file for {entry['id']}")
    original = path.read_bytes()
    with Image.open(io.BytesIO(original)) as decoded:
        if decoded.format != "PNG":
            raise ValueError(f"Source for {entry['id']} must be a PNG, got {decoded.format}")
        decoded.load()
        source_size = decoded.size
        source_mode = decoded.mode
        if "A" in decoded.getbands() or "transparency" in decoded.info:
            rgba = decoded.convert("RGBA")
            if rgba.getchannel("A").getextrema() != (255, 255):
                raise ValueError(f"Non-opaque source for {entry['id']}; background changes are not permitted")
        image = decoded.convert("RGB")
        color_action = "retained sRGB" if decoded.info.get("icc_profile") else "assumed sRGB; embedded sRGB profile"
        icc = decoded.info.get("icc_profile")
        if icc:
            profile = ImageCms.ImageCmsProfile(io.BytesIO(icc))
            description = ImageCms.getProfileDescription(profile).strip()
            if "srgb" not in description.lower():
                image = ImageCms.profileToProfile(image, profile, SRGB_PROFILE, outputMode="RGB")
                color_action = "converted embedded color profile to sRGB"
        image.thumbnail((MAX_EDGE, MAX_EDGE), resample=Image.Resampling.LANCZOS, reducing_gap=3.0)
        for quality in QUALITIES:
            buffer = io.BytesIO()
            image.save(buffer, "JPEG", quality=quality, subsampling=0, progressive=True,
                       optimize=True, icc_profile=SRGB_ICC)
            final = buffer.getvalue()
            if len(final) <= MAX_BYTES:
                break
        else:
            raise ValueError(f"{entry['id']} exceeds 650 KiB even at JPEG quality 78; no output written")
        jpg = {
            "filename": filename, "sha256": sha256(final), "bytes": len(final),
            "width": image.width, "height": image.height, "format": "JPEG", "mode": "RGB",
            "color_space": "sRGB", "quality": quality, "progressive": True,
            "optimize": True, "subsampling": "4:4:4",
        }
    generation = {
        "id": entry["id"], "tool": "built-in imagegen", "prompt": entry["prompt"],
        "source": {
            "basename": path.name, "sha256": sha256(original), "bytes": len(original),
            "format": "PNG", "mode": source_mode, "width": source_size[0], "height": source_size[1],
        },
        "jpg": jpg, "processing": {
            "operations": ["PNG decoding", "sRGB color management", "downscale only if longest edge exceeds 2048", "JPEG encoding"],
            "color_management": color_action, "cropping": False, "drawing": False,
            "text_editing": False, "upscaling": False,
        },
    }
    return final, generation


def prepare(asset_root: Path, inputs_path: Path, partial: bool) -> dict:
    if not inputs_path.is_absolute() or not inputs_path.is_file():
        raise ValueError("--inputs must name an existing absolute JSON path outside the repository")
    if inputs_path.resolve().is_relative_to(REPO):
        raise ValueError("Keep source-path input JSON outside the repository")
    catalog_path = asset_root / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    assets = catalog["assets"]
    if [asset["id"] for asset in assets] != EXPECTED_IDS:
        raise ValueError(f"Catalog must contain the {len(EXPECTED_IDS)} expected assets in order; run build_catalog.py")
    known = {asset["id"]: asset for asset in assets}
    payload = json.loads(inputs_path.read_text(encoding="utf-8"))
    entries = payload.get("assets")
    if not isinstance(entries, list) or not entries:
        raise ValueError("Input JSON must contain a non-empty assets array")
    ids: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict) or not isinstance(entry.get("id"), str):
            raise ValueError("Every input needs a string id")
        id_ = entry["id"]
        if id_ in ids or id_ not in known:
            raise ValueError(f"Duplicate or unknown input asset: {id_}")
        ids.add(id_)
        if not isinstance(entry.get("prompt"), str) or not entry["prompt"].strip():
            raise ValueError(f"Missing full prompt for {id_}")
        if not isinstance(entry.get("source_path"), str):
            raise ValueError(f"Missing source_path for {id_}")
        path = Path(entry["source_path"])
        if not path.is_absolute() or not path.is_file():
            raise ValueError(f"Missing absolute source file for {id_}")
        output_path(asset_root, known[id_]["filename"])
    if not partial and ids != set(known):
        missing = ", ".join(asset["id"] for asset in assets if asset["id"] not in ids)
        raise ValueError(f"Missing {len(known) - len(ids)} inputs: {missing}; use --partial only during production")

    # Pre-encode every requested file before making directories or touching outputs.
    pending: dict[str, bytes] = {}
    records: dict[str, dict] = {}
    generation_path = asset_root / "generation.json"
    if partial and generation_path.is_file():
        previous = json.loads(generation_path.read_text(encoding="utf-8"))
        records.update({item["id"]: item for item in previous.get("assets", [])})
    for entry in entries:
        id_ = entry["id"]
        final, record = convert_source(entry, known[id_]["filename"])
        pending[id_] = final
        records[id_] = record
        known[id_]["image"] = record["jpg"]
        known[id_]["generation"] = {
            "tool": record["tool"], "prompt": record["prompt"], "source": record["source"],
        }
    generation = {
        "schema_version": 1, "tool": "built-in imagegen",
        "prepared_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "partial": len(records) != len(EXPECTED_IDS),
        "notes_nb": ["Originalene ble formatkonvertert og eventuelt nedskalert. Ingen beskjæring, opptegning eller tekstendring.",
                     "Kildebasenavn og hasher beholdes. Absolutte lokale kildestier lagres ikke.",
                     "Kanon, lesbarhet og visuelt uttrykk vurderes manuelt; dette er ikke en OCR-kontroll."],
        "assets": [records[asset["id"]] for asset in assets if asset["id"] in records],
    }
    serialized_catalog = json_bytes(catalog)
    serialized_generation = json_bytes(generation)
    gallery = gallery_bytes(catalog)
    overview = overview_bytes(asset_root, catalog, pending)
    # Input, decoding, file-size and gallery failures above leave all outputs unchanged.
    for id_, data in pending.items():
        atomic_write(output_path(asset_root, known[id_]["filename"]), data)
    atomic_write(catalog_path, serialized_catalog)
    atomic_write(generation_path, serialized_generation)
    atomic_write(asset_root / "GALLERI.html", gallery)
    atomic_write(asset_root / "OVERSIKT.jpg", overview)
    return generation


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True, type=Path)
    parser.add_argument("--partial", action="store_true", help="prepare a subset while preserving previous assets")
    parser.add_argument("--asset-root", type=Path, default=ASSET_ROOT)
    args = parser.parse_args()
    try:
        generation = prepare(args.asset_root.resolve(), args.inputs, args.partial)
    except (ValueError, OSError, KeyError, TypeError) as error:
        parser.exit(1, f"Preparation failed: {error}\n")
    rows = [f"{item['id']}: {item['jpg']['width']}x{item['jpg']['height']}, q{item['jpg']['quality']}, {item['jpg']['bytes']} bytes" for item in generation["assets"]]
    print("\n".join(rows))
    print(f"Prepared {len(generation['assets'])}/{len(EXPECTED_IDS)} assets; offline gallery and overview saved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
