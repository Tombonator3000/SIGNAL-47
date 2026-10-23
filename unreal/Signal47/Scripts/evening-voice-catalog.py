#!/usr/bin/env python3
"""Validate supplied EveningShift WAV takes and write an isolated voices.json bank entry.

No synthesis, conversion, copying, content import, or existing-bank mutation. All
required takes must exist. --check compares a previously generated entry byte for byte.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
import wave

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = PROJECT.parent / "Narrative/EveningShift/generated/recording_manifest.json"


class CatalogError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise CatalogError(message)


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_pairs)


def wav_info(path):
    """Read the actual PCM frame count and reject short/truncated payloads."""
    with wave.open(str(path), "rb") as wav:
        require(wav.getcomptype() == "NONE", f"{path.name}: uncompressed PCM required")
        require(wav.getnchannels() == 1, f"{path.name}: mono required")
        require(wav.getsampwidth() == 2, f"{path.name}: PCM16 required")
        require(wav.getframerate() == 48000, f"{path.name}: 48000 Hz required")
        frames = wav.getnframes()
        require(frames > 0, f"{path.name}: empty recording")
        remaining = frames * 2
        while remaining:
            data = wav.readframes(min(65536, remaining // 2))
            require(data and len(data) % 2 == 0 and len(data) <= remaining,
                    f"{path.name}: truncated or invalid PCM payload")
            remaining -= len(data)
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return {"duration": frames / 48000, "frames": frames, "sha256": digest.hexdigest()}


def catalog(manifest, subtitles, wav_dir):
    require(isinstance(manifest, dict) and manifest.get("pack_id") == "signal47_evening_shift",
            "expected EveningShift recording_manifest.json")
    require(manifest.get("planned_format") == {"sample_rate_hz": 48000,
            "sample_format": "PCM16", "channels": 1}, "manifest PCM target differs from runtime contract")
    require(isinstance(subtitles, dict) and subtitles.get("pack_id") == manifest["pack_id"],
            "subtitles and manifest must belong to the same pack")
    lines = {}
    for scene in subtitles.get("scenes", []):
        for line in scene["lines"]:
            cue = line["line_id"]
            require(cue not in lines, f"duplicate subtitle cue: {cue}")
            lines[cue] = dict(line, scene_id=scene["scene_id"], channel=scene["channel"])
    recordings = manifest.get("recordings")
    require(isinstance(recordings, list) and recordings, "manifest has no recordings")
    require(manifest.get("cue_count") == len(recordings), "manifest cue count mismatch")
    by_id = {}
    for item in recordings:
        cue = item["line_id"]
        require(isinstance(cue, str) and re.fullmatch(r"[A-Z][A-Z0-9_]+", cue), f"invalid cue ID: {cue}")
        require(cue not in by_id, f"duplicate recording cue: {cue}")
        require(cue in lines, f"{cue}: subtitle text missing")
        line = lines[cue]
        require(line.get("voiced") is True and isinstance(line.get("text"), str) and line["text"].strip(),
                f"{cue}: expected voiced subtitle text")
        require(item["actor"] == line["speaker"] and item["scene_id"] == line["scene_id"]
                and item["channel"] == line["channel"], f"{cue}: manifest/subtitle identity mismatch")
        require(isinstance(item.get("voice_profile"), str) and item["voice_profile"], f"{cue}: voice profile missing")
        require(type(item.get("recording_required")) is bool, f"{cue}: recording_required must be boolean")
        by_id[cue] = item
    require({cue for cue, line in lines.items() if line.get("voiced")} == set(by_id),
            "manifest omits a voiced subtitle or contains an extra recording")
    required = {cue for cue, item in by_id.items() if item["recording_required"]}
    require(manifest.get("required_recording_count") == len(required), "manifest required-recording count mismatch")
    for cue, item in by_id.items():
        source = item.get("reuse_of")
        if item["recording_required"]:
            require(source is None, f"{cue}: required recording cannot be an alias")
        else:
            require(source in required and source != cue, f"{cue}: alias must reference one original take")
            origin = by_id[source]
            require(origin.get("reuse_of") is None and origin["actor"] == item["actor"]
                    and origin["voice_profile"] == item["voice_profile"]
                    and lines[source]["text"] == lines[cue]["text"], f"{cue}: alias identity/text mismatch")
    require(wav_dir.is_dir(), f"WAV directory does not exist: {wav_dir}")
    expected = {cue + ".wav" for cue in required}
    actual = {p.name for p in wav_dir.iterdir() if p.suffix.lower() == ".wav"}
    require(actual == expected, "WAV set mismatch; missing: " + ", ".join(sorted(expected - actual))
            + "; unexpected: " + ", ".join(sorted(actual - expected)))
    measured = {cue: wav_info(wav_dir / (cue + ".wav")) for cue in sorted(required)}
    clips, lookup = [], {}
    for cue, item in by_id.items():
        source = item.get("reuse_of") or cue
        info = measured[source]
        text = lines[cue]["text"]
        key = (item["voice_profile"], " ".join(text.split()))
        require(key not in lookup or lookup[key] == source,
                f"{cue}: different takes collide in S47RadioVoice's role/text lookup")
        lookup[key] = source
        declared_file = item.get("audio_file")
        require(declared_file is None or declared_file == source + ".wav", f"{cue}: supplied filename differs from manifest")
        declared_duration = item.get("duration_seconds")
        require(declared_duration is None or (type(declared_duration) in (int, float)
                and abs(declared_duration - info["duration"]) <= 1 / 48000), f"{cue}: stale declared duration")
        clips.append({"id": cue, "text": text, "voice": item["voice_profile"], "file": source + ".wav",
                      "raw": "voice/evening/" + source + ".wav", "duration": info["duration"],
                      "frames": info["frames"], "sha256": info["sha256"], "reuse_of": item.get("reuse_of")})
    return {"evening": {"folder": "/Game/Audio/Voice/Evening", "catalog": {"clips": clips},
            "validation": {"pack_id": manifest["pack_id"], "cue_count": len(clips),
                           "unique_recording_count": len(measured), "sample_rate_hz": 48000,
                           "sample_format": "PCM16", "channels": 1,
                           "scope": "File identity, PCM format, frame duration and aliases only; listening review required."}}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--subtitles", type=Path, help="defaults to subtitles.json beside the manifest")
    parser.add_argument("--wav-dir", type=Path, required=True, help="109 approved original cue-ID WAV files, without the alias")
    parser.add_argument("--output", type=Path, required=True, help="isolated JSON entry for later merge into voices.json")
    parser.add_argument("--check", action="store_true", help="compare existing output; never write")
    args = parser.parse_args()
    try:
        output, wav_dir = args.output.resolve(), args.wav_dir.resolve()
        # A preparatory catalog command must not overwrite assets or an existing bank.
        protected = [wav_dir, (PROJECT / "Content").resolve(), (PROJECT / "Content/Audio").resolve()]
        require(not any(output.is_relative_to(root) for root in protected),
                "output must be separate from supplied WAVs and current Content assets")
        require(not args.output.is_symlink(), "output must not be a symlink")
        entry = catalog(read_json(args.manifest),
                        read_json(args.subtitles or args.manifest.with_name("subtitles.json")), wav_dir)
        payload = (json.dumps(entry, ensure_ascii=False, indent=2, allow_nan=False) + "\n").encode("utf-8")
        if args.check:
            require(output.is_file() and output.read_bytes() == payload, "catalog missing or stale; regenerate the isolated entry")
        else:
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(payload)
        count = entry["evening"]["validation"]
        print(f"PASS: {count['unique_recording_count']} supplied PCM WAVs, {count['cue_count']} cues; "
              f"isolated entry {'byte-identical' if args.check else 'written'}. No import or listening approval performed.")
        return 0
    except (CatalogError, OSError, ValueError, KeyError, TypeError, wave.Error) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
