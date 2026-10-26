#!/usr/bin/env python3
"""Prepare or verify the 48-item environment graphics pack.

Prepare: --inputs /absolute/private-inputs.json [--partial]
Verify:  --verify [--partial] [--review-file manual-review.json] [--no-report]

Input JSON: {"assets":[{"id":"...","prompt":"full prompt","source_path":"/absolute/output.png"}]}.
The input JSON stays outside the repository. Public metadata retains only
source basenames, dimensions, bytes, hashes and complete prompts.

Manual review uses the Documents review schema, including reviewed_jpg_sha256,
method="visual inspection", and one exact catalog key_checks string per row.
Image conversion and verification reuse the existing Documents tools. No image
editing, text correction, crop, drawing or upscaling is performed.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
DOC_TOOLS = Path(__file__).resolve().parents[2] / "Documents" / "Tools"
sys.path.insert(0, str(DOC_TOOLS))
from prepare_assets import atomic_write, convert_source, json_bytes, output_path, REPO
from build_gallery import gallery_bytes, overview_bytes
import verify_assets as document_verifier


ASSET_ROOT = Path(__file__).resolve().parent.parent
EXPECTED_IDS = [
    "CINEMA_DESERT_SKY", "CINEMA_FRIDAY_MATINEE", "CINEMA_DUST_COUNTY", "CINEMA_NIGHT_COACH", "CINEMA_BLUE_LAKE", "CINEMA_VISITORS", "CINEMA_CLOSED", "CINEMA_TICKET",
    "DINER_MENU", "DINER_COFFEE", "DINER_PIE", "GAS_AD", "OIL_AD", "FEED_AD", "LAUNDRY_RATES", "LAUNDRY_ETIQUETTE", "MOTEL_AD", "SOUVENIR_TOURS", "RADIO_AD", "CHURCH_NOTICE",
    "FEED_SACK", "COFFEE_TIN", "COLA_LABEL", "DETERGENT_BOX", "CRACKER_BOX", "BATTERY_CARD", "MATCHBOOK", "TAKEAWAY_BAG", "FOOD_WRAPPER", "CASSETTE_CARD",
    "MOTEL_KEY", "MOTEL_REGISTRATION", "DINER_RECEIPT", "GAS_RECEIPT", "POSTCARD", "MAP_COVER", "SARO_RADIO_QUIET", "SARO_AUTHORIZED", "DARKROOM_SIGN", "MAINTENANCE_TAG",
    "PARTS_LABEL", "SERVICE_FORM", "VISITOR_BADGE", "SUBSTATION_SIGN", "PAYPHONE_CARD", "LOST_DOG", "YARD_SALE", "SHOPPING_LIST",
]


def load_catalog(asset_root: Path) -> dict:
    catalog = json.loads((asset_root / "catalog.json").read_text(encoding="utf-8"))
    if [asset["id"] for asset in catalog["assets"]] != EXPECTED_IDS:
        raise ValueError("Environment catalog must contain the 48 expected IDs in order")
    if catalog.get("language") != "en" or catalog.get("asset_count") != 48:
        raise ValueError("Expected an English 48-asset catalog")
    filenames = set()
    for number, asset in enumerate(catalog["assets"], 1):
        if not asset["filename"].startswith(f"jpg/{number:02d}-"):
            raise ValueError(f"Incorrect numbered filename for {asset['id']}")
        destination = output_path(asset_root, asset["filename"])
        if destination in filenames:
            raise ValueError("Duplicate output filename")
        filenames.add(destination)
        if asset.get("status") != "original_background_prop" or not asset.get("text", "").strip():
            raise ValueError(f"Invalid background prop text/status for {asset['id']}")
        if not 1 <= len(asset.get("key_checks", [])) <= 3:
            raise ValueError(f"Expected one to three manual checks for {asset['id']}")
        if asset["group"] not in catalog["group_names"]:
            raise ValueError(f"Unknown gallery group for {asset['id']}")
        size = asset.get("physical_size_m", [])
        if len(size) != 2 or any(not isinstance(value, (int, float)) or value <= 0 for value in size):
            raise ValueError(f"Invalid physical-size hint for {asset['id']}")
        for source in asset["source_refs"]:
            path = Path(source["path"])
            if path.is_absolute() or ".." in path.parts:
                raise ValueError("Source references must be repository-relative")
    return catalog


def prepare(asset_root: Path, inputs_path: Path, partial: bool) -> dict:
    catalog = load_catalog(asset_root)
    if not inputs_path.is_absolute() or not inputs_path.is_file() or inputs_path.resolve().is_relative_to(REPO):
        raise ValueError("--inputs must be an existing absolute JSON path outside the repository")
    entries = json.loads(inputs_path.read_text(encoding="utf-8")).get("assets")
    if not isinstance(entries, list) or not entries:
        raise ValueError("Input JSON needs a non-empty assets list")
    known = {asset["id"]: asset for asset in catalog["assets"]}
    supplied = set()
    for entry in entries:
        id_ = entry.get("id")
        if id_ not in known or id_ in supplied:
            raise ValueError(f"Unknown or duplicate input ID: {id_}")
        supplied.add(id_)
        if not isinstance(entry.get("prompt"), str) or not entry["prompt"].strip():
            raise ValueError(f"Missing full prompt for {id_}")
        source = Path(entry.get("source_path", ""))
        if not source.is_absolute() or not source.is_file():
            raise ValueError(f"Missing absolute source file for {id_}")
    if not partial and supplied != set(EXPECTED_IDS):
        raise ValueError(f"Missing {48 - len(supplied)} inputs; --partial is for production checkpoints only")
    records = {}
    generation_path = asset_root / "generation.json"
    if partial and generation_path.is_file():
        previous = json.loads(generation_path.read_text(encoding="utf-8"))["assets"]
        records = {item["id"]: item for item in previous}
        if not set(records).issubset(EXPECTED_IDS) or len(records) != len(previous):
            raise ValueError("Existing generation manifest has duplicate or unknown IDs")
    pending = {}
    for entry in entries:
        id_ = entry["id"]
        data, record = convert_source(entry, known[id_]["filename"])
        pending[id_] = data
        records[id_] = record
        known[id_]["image"] = record["jpg"]
        known[id_]["generation"] = {"tool": record["tool"], "prompt": record["prompt"], "source": record["source"]}
    generation = {
        "schema_version": 1, "pack_id": catalog["pack_id"], "tool": "built-in imagegen",
        "prepared_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "partial": len(records) != 48,
        "notes_nb": ["Flat trykkgrafikk til senere bruk; ingen ny 3D-modell eller runtimeintegrasjon.",
                     "Felles Documents-konvertering: sRGB JPEG, maks 2048 piksler langkant og 650 KiB, kvalitet 86 ned til 78 ved behov.",
                     "Ingen beskjæring, opptegning, tekstendring eller oppskalering. Absolutte lokale kildestier publiseres ikke."],
        "assets": [records[id_] for id_ in EXPECTED_IDS if id_ in records],
    }
    # Validate every source and encode every requested JPG/gallery before writing.
    # A missing/corrupt input or failed size target leaves all existing files alone.
    catalog_data = json_bytes(catalog)
    generation_data = json_bytes(generation)
    gallery = gallery_bytes(catalog)
    overview = overview_bytes(asset_root, catalog, pending)
    for id_, data in pending.items():
        atomic_write(output_path(asset_root, known[id_]["filename"]), data)
    atomic_write(asset_root / "catalog.json", catalog_data)
    atomic_write(generation_path, generation_data)
    atomic_write(asset_root / "GALLERI.html", gallery)
    atomic_write(asset_root / "OVERSIKT.jpg", overview)
    return generation


def verify(asset_root: Path, partial: bool, review_file: Path | None) -> dict:
    catalog = load_catalog(asset_root)
    reviews = json.loads(review_file.read_text(encoding="utf-8")) if review_file else None
    # Reuse the existing hash-bound manual-review and JPEG verifier. Inventory is
    # local to this short-lived CLI process and restored before returning.
    previous_ids = document_verifier.EXPECTED_IDS
    try:
        document_verifier.EXPECTED_IDS = EXPECTED_IDS
        report = document_verifier.verify(asset_root, partial, reviews)
    finally:
        document_verifier.EXPECTED_IDS = previous_ids
    report["pack_id"] = catalog["pack_id"]
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--inputs", type=Path)
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--partial", action="store_true")
    parser.add_argument("--review-file", type=Path)
    parser.add_argument("--no-report", action="store_true")
    parser.add_argument("--asset-root", type=Path, default=ASSET_ROOT)
    args = parser.parse_args()
    try:
        if args.verify:
            if args.inputs:
                parser.error("--inputs and --verify are separate modes")
            report = verify(args.asset_root.resolve(), args.partial, args.review_file)
            if not args.no_report:
                atomic_write(args.asset_root / "verification.json", json_bytes(report))
            print(f"Technical {report['technical_status']}: {report['prepared_assets']}/48 JPGs; manual {report['manual_review_status']}; OCR NOT_RUN.")
            for error in report["errors"]:
                print(error)
            return 0 if report["technical_status"] == "PASS" and report["manual_review_status"] != "FAIL" else 1
        if args.inputs is None:
            parser.error("--inputs is required when preparing images")
        result = prepare(args.asset_root.resolve(), args.inputs, args.partial)
        print(f"Prepared {len(result['assets'])}/48 JPGs; generation.json, offline gallery and overview saved.")
        return 0
    except (OSError, ValueError, KeyError, TypeError, AttributeError) as error:
        parser.exit(1, f"Environment pack failed: {error}\n")


if __name__ == "__main__":
    raise SystemExit(main())
