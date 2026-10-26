#!/usr/bin/env python3
"""Verify document JPG delivery and record pending manual content checks.

This tool verifies format/metadata only. It never reports an OCR pass.
--partial permits an in-progress subset. --no-report leaves the filesystem unchanged.
Optional --review-file supplies per-asset, explicitly checked manual review rows:
{"assets":[{"id":"...","reviewed_jpg_sha256":"...","method":"visual inspection","checks":[
{"check":"exact catalog key_checks string","status":"PASS","notes_nb":"..."}]}]}.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path

from PIL import Image, ImageCms, JpegImagePlugin

from prepare_assets import atomic_write, json_bytes
from build_catalog import EXPECTED_IDS


ASSET_ROOT = Path(__file__).resolve().parent.parent


def verify(asset_root: Path, partial: bool = False, reviews: dict | None = None) -> dict:
    catalog = json.loads((asset_root / "catalog.json").read_text(encoding="utf-8"))
    assets = catalog["assets"]
    generation = json.loads((asset_root / "generation.json").read_text(encoding="utf-8"))
    records = {row["id"]: row for row in generation["assets"]}
    errors: list[str] = []
    if [asset["id"] for asset in assets] != EXPECTED_IDS:
        errors.append(f"Catalog is not the ordered inventory of {len(EXPECTED_IDS)} expected IDs")
    if generation.get("tool") != "built-in imagegen":
        errors.append("Generation tool is not built-in imagegen")
    if len(records) != len(generation["assets"]):
        errors.append("Duplicate generation IDs")
    if not set(records).issubset(EXPECTED_IDS):
        errors.append("Unknown generation IDs")
    expected = {asset["filename"] for asset in assets}
    present = {str(path.relative_to(asset_root)) for path in (asset_root / "jpg").rglob("*") if path.is_file()}
    if len(expected) != len(EXPECTED_IDS):
        errors.append(f"Expected exactly {len(EXPECTED_IDS)} distinct catalog JPG filenames")
    extras = present - expected
    missing = expected - present
    if extras:
        errors.append("Unexpected files in jpg/: " + ", ".join(sorted(extras)))
    if missing and not partial:
        errors.append(f"Missing {len(missing)} JPG files: " + ", ".join(sorted(missing)))
    pngs = [str(path.relative_to(asset_root)) for path in asset_root.rglob("*") if path.is_file() and path.suffix.lower() == ".png"]
    if pngs:
        errors.append("PNG files must not be delivered: " + ", ".join(pngs))
    if not partial and set(records) != set(EXPECTED_IDS):
        errors.append(f"Generation manifest must contain all {len(EXPECTED_IDS)} expected assets")
    review_by_id = {row["id"]: row for row in (reviews or {}).get("assets", [])}
    if len(review_by_id) != len((reviews or {}).get("assets", [])) or not set(review_by_id).issubset(EXPECTED_IDS):
        errors.append("Manual review contains duplicate or unknown IDs")
    rows = []
    for asset in assets:
        id_ = asset["id"]
        row_errors: list[str] = []
        path = asset_root / asset["filename"]
        image_meta = asset.get("image", {})
        record = records.get(id_)
        if path.is_file():
            content = path.read_bytes()
            digest = hashlib.sha256(content).hexdigest()
            if len(content) > 650 * 1024:
                row_errors.append("JPG exceeds 650 KiB")
            try:
                with Image.open(io.BytesIO(content)) as image:
                    image.load()
                    if image.format != "JPEG" or image.mode != "RGB":
                        row_errors.append("Image must be RGB JPEG")
                    if max(image.size) > 2048:
                        row_errors.append("Image exceeds 2048-pixel long edge")
                    if not (image.info.get("progressive") or image.info.get("progression")):
                        row_errors.append("JPEG is not progressive")
                    if JpegImagePlugin.get_sampling(image) != 0:
                        row_errors.append("JPEG does not use 4:4:4 sampling")
                    profile = image.info.get("icc_profile")
                    if not profile or "srgb" not in ImageCms.getProfileDescription(ImageCms.ImageCmsProfile(io.BytesIO(profile))).lower():
                        row_errors.append("JPEG lacks an embedded sRGB profile")
                    if image_meta.get("width") != image.width or image_meta.get("height") != image.height:
                        row_errors.append("Catalog dimensions do not match image")
            except (OSError, ValueError) as error:
                row_errors.append("JPEG cannot be decoded: " + str(error))
            if image_meta.get("sha256") != digest or image_meta.get("bytes") != len(content):
                row_errors.append("Catalog hash/bytes do not match image")
            if image_meta.get("filename") != asset["filename"] or image_meta.get("quality") not in (86, 84, 82, 80, 78):
                row_errors.append("Catalog filename/quality metadata is invalid")
            required_metadata = {"format": "JPEG", "mode": "RGB", "color_space": "sRGB", "progressive": True,
                                 "optimize": True, "subsampling": "4:4:4"}
            if any(image_meta.get(key) != value for key, value in required_metadata.items()):
                row_errors.append("Catalog JPEG encoding declarations are inconsistent")
            if record is None or record.get("jpg") != image_meta:
                row_errors.append("Generation JPG metadata does not match catalog")
            else:
                if record.get("tool") != "built-in imagegen" or not record.get("prompt", "").strip():
                    row_errors.append("Generation tool/full prompt is missing")
                source = record.get("source", {})
                if not source.get("basename") or Path(source["basename"]).name != source["basename"]:
                    row_errors.append("Source must retain a basename, not a local path")
                if len(source.get("sha256", "")) != 64 or source.get("format") != "PNG" or source.get("bytes", 0) <= 0:
                    row_errors.append("Source hash/format/bytes metadata is invalid")
                if image_meta.get("width", 0) > source.get("width", 0) or image_meta.get("height", 0) > source.get("height", 0):
                    row_errors.append("Output dimensions imply upscaling")
                for field in ("cropping", "drawing", "text_editing", "upscaling"):
                    if record.get("processing", {}).get(field) is not False:
                        row_errors.append("Unexpected or missing processing declaration: " + field)
        elif not partial:
            row_errors.append("Image file is missing")
        review = review_by_id.get(id_, {})
        supplied = {check.get("check"): check for check in review.get("checks", [])}
        if len(supplied) != len(review.get("checks", [])) or not set(supplied).issubset(asset["key_checks"]):
            row_errors.append("Manual review has duplicate or unknown catalog checks")
        if supplied and review.get("method") != "visual inspection":
            row_errors.append("Manual review method must be visual inspection")
        if supplied and (not path.is_file() or review.get("reviewed_jpg_sha256") != hashlib.sha256(path.read_bytes()).hexdigest()):
            row_errors.append("Manual review does not match the current JPG hash")
        manual_checks = []
        for check in asset["key_checks"]:
            evidence = supplied.get(check)
            status = evidence.get("status") if evidence else "PENDING_MANUAL_REVIEW"
            if status not in ("PASS", "FAIL", "PENDING_MANUAL_REVIEW"):
                row_errors.append("Invalid manual review status")
                status = "PENDING_MANUAL_REVIEW"
            manual_checks.append({"check": check, "status": status,
                                  "notes_nb": evidence.get("notes_nb", "") if evidence else "Ikke visuelt vurdert av dette verktøyet."})
        manual_status = "FAIL" if any(check["status"] == "FAIL" for check in manual_checks) else "PASS" if all(check["status"] == "PASS" for check in manual_checks) else "PENDING_MANUAL_REVIEW"
        rows.append({"id": id_, "filename": asset["filename"],
                     "reviewed_jpg_sha256": review.get("reviewed_jpg_sha256"),
                     "technical_status": "FAIL" if row_errors else "PASS" if path.is_file() else "NOT_PREPARED",
                     "errors": row_errors, "manual_review_status": manual_status, "key_checks": manual_checks})
        errors.extend(f"{id_}: {message}" for message in row_errors)
    manual_total = "FAIL" if any(row["manual_review_status"] == "FAIL" for row in rows) else "PASS" if all(row["manual_review_status"] == "PASS" for row in rows) else "PENDING_MANUAL_REVIEW"
    return {
        "schema_version": 1, "checked_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "technical_status": "FAIL" if errors else "PASS", "partial": partial,
        "expected_assets": len(EXPECTED_IDS), "prepared_assets": len(present & expected),
        "ocr": "NOT_RUN", "manual_review_status": manual_total,
        "notes_nb": ["PASS for teknisk format er ikke en vurdering av tekst, kanon eller visuelt innhold.",
                     "Manuelle kontrollpunkter står åpne med mindre en separat, eksplisitt visuell vurdering er oppgitt.",
                     "Dette verktøyet kjører ikke OCR. Separat best-effort-støtte er beskrevet i den manuelle vurderingen."],
        "errors": errors, "assets": rows,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset-root", type=Path, default=ASSET_ROOT)
    parser.add_argument("--partial", action="store_true")
    parser.add_argument("--no-report", action="store_true")
    parser.add_argument("--review-file", type=Path)
    args = parser.parse_args()
    try:
        review = json.loads(args.review_file.read_text(encoding="utf-8")) if args.review_file else None
        report = verify(args.asset_root.resolve(), args.partial, review)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"Verification failed: {error}\n")
    if not args.no_report:
        atomic_write(args.asset_root / "verification.json", json_bytes(report))
    print(f"Technical {report['technical_status']}: {report['prepared_assets']}/{report['expected_assets']} JPG files; manual review {report['manual_review_status']}; OCR NOT_RUN.")
    for error in report["errors"]:
        print(error)
    return 0 if report["technical_status"] == "PASS" and report["manual_review_status"] != "FAIL" else 1


if __name__ == "__main__":
    raise SystemExit(main())
