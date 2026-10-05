#!/usr/bin/env python3
"""Round 12 JPEG export and separate, read-only asset QA.

Export changes only RGB representation, dimensions, JPEG encoding and sRGB ICC.
It never recolors, blends seams, flips tiles, draws motifs or edits content.
Manual visual conclusions live in visual_review.json; runtime is UNVERIFIED.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
from io import BytesIO
import json
from pathlib import Path
import sys

from PIL import Image, ImageChops, ImageCms, ImageStat

ASSET = "road/tex_oldroad_macro.jpg"
SIZE = (2048, 2048)
MAX_BYTES = 900_000
TARGET_MEAN = (164, 145, 123)
MEAN_TOLERANCE = 10
REFERENCE_RANGE = (60, 225)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check(value: bool) -> str:
    return "PASS" if value else "FAIL"


def icc_info(icc: bytes | None) -> dict:
    result = {"present": bool(icc), "status": "FAIL"}
    if not icc:
        return result
    result.update({"sha256": digest(icc), "size_bytes": len(icc)})
    try:
        profile = ImageCms.ImageCmsProfile(BytesIO(icc))
        description = ImageCms.getProfileDescription(profile).strip()
        color_space = profile.profile.xcolor_space.strip()
        result.update({"description": description, "color_space": color_space,
                       "status": check("srgb" in description.casefold().replace(" ", "")
                                       and color_space == "RGB")})
    except (OSError, ValueError, TypeError) as error:
        result["error"] = str(error)
    return result


def private_output(path: Path, directory: Path) -> None:
    if path.parent.resolve() != directory.resolve() or path.is_symlink():
        raise ValueError(f"Unsafe output path: {path}")
    if path.exists() and (not path.is_file() or path.stat().st_nlink != 1):
        raise ValueError(f"Output is not a private regular file: {path}")


def export_jpeg(source: Path, destination: Path, quality: int, replace: bool) -> dict:
    source = source.expanduser().resolve(strict=True)
    private_output(destination, destination.parent)
    if destination.exists() and not replace:
        raise ValueError("Final asset already exists; explicit --replace is required")
    if destination.exists() and destination.samefile(source):
        raise ValueError("Export source and final JPEG must be separate files")
    raw = source.read_bytes()
    with Image.open(BytesIO(raw)) as opened:
        opened.load()
        original_size, original_mode = opened.size, opened.mode
        if opened.getexif().get(274, 1) != 1:
            raise ValueError("Source has EXIF orientation; export does not rotate content")
        if "A" in opened.getbands() or "transparency" in opened.info:
            if opened.convert("RGBA").getchannel("A").getextrema() != (255, 255):
                raise ValueError("Source contains transparency; export does not invent a background")
        source_icc = icc_info(opened.info.get("icc_profile"))
        if source_icc["present"] and source_icc["status"] != "PASS":
            raise ValueError("Source ICC is not identified as sRGB; no implicit recoloring is permitted")
        image = opened.convert("RGB")
    if image.size != SIZE:
        image = image.resize(SIZE, Image.Resampling.LANCZOS)
    destination.parent.mkdir(parents=True, exist_ok=True)
    srgb = ImageCms.ImageCmsProfile(ImageCms.createProfile("sRGB")).tobytes()
    # There are deliberately no ImageEnhance, channel shifts or seam operations.
    image.save(destination, "JPEG", quality=quality, optimize=True,
               progressive=True, subsampling=1, icc_profile=srgb)
    return {"source_sha256": digest(raw), "source_width": original_size[0],
            "source_height": original_size[1], "source_mode": original_mode,
            "source_icc": source_icc, "jpeg_quality": quality,
            "operations": ["convert RGB", "Lanczos resize to 2048x2048 if needed",
                           "JPEG encode", "embed sRGB ICC"],
            "source_color_assumption": "Existing sRGB ICC retained semantically" if source_icc["present"]
                                       else "Unprofiled generator RGB is assumed sRGB; no channel correction"}


def inspect_asset(path: Path) -> tuple[dict, Image.Image]:
    raw = path.read_bytes()
    with Image.open(BytesIO(raw)) as opened:
        opened.load()
        image = opened.copy()
        image.info = opened.info.copy()
        actual_format = opened.format
    rgb = image.convert("RGB")
    means = ImageStat.Stat(rgb).mean
    extrema = rgb.getextrema()
    pixels = rgb.width * rgb.height
    histograms = [channel.histogram() for channel in rgb.split()]
    reference_pass = all(low >= REFERENCE_RANGE[0] and high <= REFERENCE_RANGE[1]
                         for low, high in extrema)
    profile = icc_info(image.info.get("icc_profile"))
    checks = {
        "dimensions_2048x2048": check(image.size == SIZE),
        "format_JPEG": check(actual_format == "JPEG"),
        "mode_RGB": check(image.mode == "RGB"),
        "no_alpha": check("A" not in image.getbands() and "transparency" not in image.info),
        "srgb_ICC": profile["status"],
        "bytes_at_most_900000": check(len(raw) <= MAX_BYTES),
        "mean_each_channel_within_10": check(all(abs(value - target) <= MEAN_TOLERANCE
                                                 for value, target in zip(means, TARGET_MEAN))),
    }
    left_right = ImageChops.difference(rgb.crop((0, 0, 1, rgb.height)),
                                      rgb.crop((rgb.width - 1, 0, rgb.width, rgb.height)))
    top_bottom = ImageChops.difference(rgb.crop((0, 0, rgb.width, 1)),
                                      rgb.crop((0, rgb.height - 1, rgb.width, rgb.height)))
    seam = {"left_right_MAD_RGB": ImageStat.Stat(left_right).mean,
            "top_bottom_MAD_RGB": ImageStat.Stat(top_bottom).mean,
            "visual_seam_status": "UNVERIFIED",
            "note": "Measurements have no automatic seam PASS threshold"}
    if rgb.width > 1 and rgb.height > 1:
        seam["internal_adjacent_MAD_RGB_x"] = ImageStat.Stat(ImageChops.difference(
            rgb.crop((0, 0, rgb.width - 1, rgb.height)), rgb.crop((1, 0, rgb.width, rgb.height)))).mean
        seam["internal_adjacent_MAD_RGB_y"] = ImageStat.Stat(ImageChops.difference(
            rgb.crop((0, 0, rgb.width, rgb.height - 1)), rgb.crop((0, 1, rgb.width, rgb.height)))).mean
    result = {
        "asset": ASSET, "sha256": digest(raw), "size_bytes": len(raw),
        "width": image.width, "height": image.height, "format": actual_format,
        "mode": image.mode, "icc": profile, "checks": checks,
        "automatic_status": "PASS" if all(value == "PASS" for value in checks.values()) else "FAIL",
        "RGB_mean": means, "RGB_target_mean": list(TARGET_MEAN),
        "RGB_mean_delta": [value - target for value, target in zip(means, TARGET_MEAN)],
        "RGB_min": [low for low, high in extrema], "RGB_max": [high for low, high in extrema],
        "contrast_reference": {
            "reference": list(REFERENCE_RANGE), "all_channels_strictly_within_reference": reference_pass,
            "per_channel_fraction_below_60": [sum(histogram[:60]) / pixels for histogram in histograms],
            "per_channel_fraction_above_225": [sum(histogram[226:]) / pixels for histogram in histograms],
            "manual_status": "UNVERIFIED",
            "note": "Brief says around 60/225; measured extrema do not replace manual contrast review",
        },
        "seam_measurements": seam,
    }
    return result, image


def write_previews(image: Image.Image, output: Path) -> list[dict]:
    rgb = image.convert("RGB")
    tiled = Image.new("RGB", (rgb.width * 2, rgb.height * 2))
    for y in range(2):
        for x in range(2):
            tiled.paste(rgb, (x * rgb.width, y * rgb.height))
    mini = rgb.resize((512, 512), Image.Resampling.LANCZOS)
    repeated = Image.new("RGB", (1536, 1536))
    for y in range(3):
        for x in range(3):
            repeated.paste(mini, (x * 512, y * 512))
    artifacts = []
    for filename, preview in [("macro_2x2_full.png", tiled), ("macro_3x3_1536.png", repeated)]:
        path = output / filename
        private_output(path, output)
        preview.save(path, "PNG", icc_profile=image.info.get("icc_profile"))
        data = path.read_bytes()
        artifacts.append({"file": filename, "width": preview.width, "height": preview.height,
                          "sha256": digest(data), "size_bytes": len(data)})
    return artifacts


def manual_review_stub(asset_sha: str) -> dict:
    return {
        "asset": ASSET, "asset_sha256": asset_sha, "reviewer": None,
        "checked_at_utc": None, "overall_visual_status": "UNVERIFIED",
        "checks": {name: {"status": "UNVERIFIED", "notes": ""} for name in [
            "orthographic_flat_light_no_hard_sun_shadows",
            "sand_caliche_grass_and_shrubs_1_to_4_px",
            "dry_drainages_4_to_10_px_with_opposite_edge_continuity",
            "no_roads_tracks_fences_buildings_water_text_or_map_symbols",
            "low_to_moderate_contrast_around_60_to_225",
            "no_distinctive_single_shape_larger_than_300_m",
            "seams_both_directions_in_2x2",
            "repetition_in_3x3"]},
        "runtime_status": "UNVERIFIED; Claude owns integration and runtime validation",
        "instruction": "Human reviewer fills this file after viewing actual asset and both QA previews",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("web_root", type=Path)
    parser.add_argument("--export-from", type=Path, help="Generator source to resize/encode; otherwise QA only")
    parser.add_argument("--quality", type=int, default=85)
    parser.add_argument("--replace", action="store_true", help="Explicitly replace this one final JPEG during export")
    args = parser.parse_args()
    if not 1 <= args.quality <= 95:
        raise ValueError("JPEG quality must be 1..95")
    web = args.web_root.expanduser().resolve(strict=True)
    art = web / "src" / "assets" / "art"
    asset = art / ASSET
    output = art / "production" / "round12_qa"
    if output.resolve() != web / "src" / "assets" / "art" / "production" / "round12_qa":
        raise ValueError("QA output directory redirects outside its allowed location")
    if asset.parent.resolve() != web / "src" / "assets" / "art" / "road":
        raise ValueError("Asset directory redirects outside its allowed location")
    export = None
    if args.export_from:
        export = export_jpeg(args.export_from, asset, args.quality, args.replace)
    result, image = inspect_asset(asset)
    output.mkdir(parents=True, exist_ok=True)
    report = {"checked_at_utc": datetime.now(timezone.utc).isoformat(),
              "contract": "SIGNAL-47 main 1e27e6c:web/ART_BRIEF.md, Runde 12",
              **result, "export": export, "qa_artifacts": write_previews(image, output),
              "runtime_status": "UNVERIFIED", "visual_review": "visual_review.json"}
    report["asset_sha_unchanged_after_QA"] = digest(asset.read_bytes()) == result["sha256"]
    if not report["asset_sha_unchanged_after_QA"]:
        report["automatic_status"] = "FAIL"
    review_path = output / "visual_review.json"
    private_output(review_path, output)
    if review_path.is_file():
        previous = json.loads(review_path.read_text())
        if previous.get("asset_sha256") != result["sha256"]:
            old_sha = previous.get("asset_sha256", "unknown")
            if not isinstance(old_sha, str) or any(c not in "0123456789abcdef" for c in old_sha):
                old_sha = "unknown"
            archive = output / f"visual_review_previous_{old_sha[:12]}.json"
            private_output(archive, output)
            if not archive.exists():
                archive.write_text(json.dumps(previous, ensure_ascii=False, indent=2) + "\n")
            review_path.write_text(json.dumps(manual_review_stub(result["sha256"]), indent=2) + "\n")
    else:
        review_path.write_text(json.dumps(manual_review_stub(result["sha256"]), indent=2) + "\n")
    report_path = output / "round12_qa_report.json"
    private_output(report_path, output)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({key: report[key] for key in ["automatic_status", "sha256", "size_bytes",
                                                  "RGB_mean", "RGB_min", "RGB_max"]}, indent=2))
    print(f"QA: {output}")
    return 0 if report["automatic_status"] == "PASS" else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, TypeError, SyntaxError) as error:
        print(f"Round 12 QA/export stopped: {error}", file=sys.stderr)
        raise SystemExit(2)
