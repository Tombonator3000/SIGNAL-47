#!/usr/bin/env python3
"""Validate and export the authored Evening Shift pack; does not integrate Unreal.

JSON modules are the source. Run normally to export, or --check to require
byte-identical generated files without writing. Python standard library only.
"""
from __future__ import annotations

import argparse
import csv
import io
import json
import math
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
PHASES = ["commute", "handover", "early_shift", "signal", "archive", "field", "all_night"]
CHANNELS = ["physical", "phone", "walkie", "radio", "ui"]


class PackError(ValueError):
    pass


def fail(where, message):
    raise PackError(f"{where}: {message}")


def obj(value, where, keys):
    if not isinstance(value, dict):
        fail(where, "expected object")
    missing, extra = set(keys) - value.keys(), value.keys() - set(keys)
    if missing or extra:
        fail(where, f"missing keys {sorted(missing)}; unknown keys {sorted(extra)}")
    return value


def string(value, where, empty=False):
    if not isinstance(value, str) or (not empty and not value.strip()):
        fail(where, "expected string" if empty else "expected nonempty string")
    return value


def boolean(value, where):
    if type(value) is not bool:
        fail(where, "expected boolean")


def sequence(value, where):
    if not isinstance(value, list):
        fail(where, "expected array")
    return value


def mapping(value, where):
    if not isinstance(value, dict):
        fail(where, "expected object")
    return value


def number(value, where):
    if type(value) not in (int, float) or not math.isfinite(value):
        fail(where, "expected finite number")
    return value


def enum(value, options, where):
    if value not in options:
        fail(where, f"expected one of {list(options)}")


def unique_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise PackError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique_pairs)
    except (OSError, UnicodeError, json.JSONDecodeError, PackError) as exc:
        raise PackError(f"{path}: {exc}") from exc


def validate():
    pack = read_json(ROOT / "pack.json")
    obj(pack, "pack", ["schema_version", "pack_id", "language", "status", "engine_target",
                       "phases", "channels", "actors", "flags", "external_flags", "modules",
                       "canon", "production", "recording_aliases"])
    if type(pack["schema_version"]) is not int or pack["schema_version"] != 1:
        fail("pack.schema_version", "only schema 1 supported")
    for key in ("pack_id", "language", "engine_target"):
        string(pack[key], f"pack.{key}")
    enum(pack["language"], ["en-US"], "pack.language")
    enum(pack["status"], ["authored_not_integrated"], "pack.status")
    if pack["phases"] != PHASES or pack["channels"] != CHANNELS:
        fail("pack", "phase/channel declarations must match FORMAT.md")
    actors = mapping(pack["actors"], "pack.actors")
    for actor, value in actors.items():
        string(actor, "actor ID")
        obj(value, f"actor {actor}", ["display_name", "voice_profile", "record", "allowed_channels", "direction_nb"])
        string(value["display_name"], f"actor {actor}.display_name", empty=True)
        string(value["direction_nb"], f"actor {actor}.direction_nb")
        boolean(value["record"], f"actor {actor}.record")
        if value["voice_profile"] is not None:
            string(value["voice_profile"], f"actor {actor}.voice_profile")
        if value["record"] and value["voice_profile"] is None:
            fail(actor, "recording actor needs voice_profile")
        allowed = sequence(value["allowed_channels"], f"actor {actor}.allowed_channels")
        if not allowed or len(set(allowed)) != len(allowed):
            fail(actor, "allowed_channels must be nonempty and unique")
        for channel in allowed:
            enum(channel, CHANNELS, actor)
    if "DESCRIPTION" not in actors or actors["DESCRIPTION"]["record"]:
        fail("DESCRIPTION", "must be declared with record=false")
    if "DALE" not in actors or set(actors["DALE"]["allowed_channels"]) - {"phone", "walkie", "radio"}:
        fail("DALE", "may only use phone, walkie or radio")
    if actors["DALE"]["voice_profile"] != "kinski":
        fail("DALE", "requires kinski voice_profile")

    internal, external = {}, {}

    def flags(values, target, where, is_external=False):
        for name, spec in mapping(values, where).items():
            string(name, where)
            if name in internal or name in external:
                fail(where, f"duplicate flag {name}")
            obj(spec, f"flag {name}", ["purpose_nb"] if is_external else ["default", "purpose_nb"])
            string(spec["purpose_nb"], f"flag {name}.purpose_nb")
            if not is_external:
                boolean(spec["default"], f"flag {name}.default")
            target[name] = spec

    flags(pack["flags"], internal, "pack.flags")
    flags(pack["external_flags"], external, "pack.external_flags", True)
    paths = sequence(pack["modules"], "pack.modules")
    if not paths:
        fail("pack.modules", "must include at least one module")
    modules, seen_paths, module_ids = [], set(), set()
    for relative in paths:
        string(relative, "module path")
        path = (ROOT / relative).resolve()
        if path.parent != (ROOT / "modules").resolve() or path.suffix != ".json":
            fail(relative, "expected direct JSON file under modules/")
        if path in seen_paths:
            fail(relative, "duplicate module path")
        seen_paths.add(path)
        module = read_json(path)
        obj(module, relative, ["module_id", "flags", "scenes", "documents", "media", "strings"])
        string(module["module_id"], relative)
        if module["module_id"] in module_ids:
            fail(relative, "duplicate module_id")
        module_ids.add(module["module_id"])
        flags(module["flags"], internal, f"{relative}.flags")
        for key in ("scenes", "documents", "media", "strings"):
            sequence(module[key], f"{relative}.{key}")
        modules.append(module)

    canon = obj(pack["canon"], "pack.canon", ["shift_date", "shift_start", "proposed_residual_date",
                "proposed_residual", "event_start", "event_end", "receiver_mhz", "dishes", "azimuth_deg"])
    expected = {"shift_date": "1986-04-13", "shift_start": "18:00:00", "proposed_residual_date": "1986-04-14",
                "proposed_residual": "00:13:47", "event_start": "05:29:00", "event_end": "05:29:47",
                "receiver_mhz": 1420.405, "dishes": 27, "azimuth_deg": 26}
    for key, value in canon.items():
        if type(value) is not type(expected[key]) or value != expected[key]:
            fail(f"canon.{key}", f"expected {expected[key]!r}")
    production = obj(pack["production"], "production", ["audio_status", "video_status", "runtime_status", "voice_target", "note_nb"])
    for key in ("audio_status", "video_status"):
        enum(production[key], ["pending"], f"production.{key}")
    enum(production["runtime_status"], ["not_integrated"], "production.runtime_status")
    target = obj(production["voice_target"], "voice_target", ["sample_rate_hz", "sample_format", "channels"])
    if (type(target["sample_rate_hz"]) is not int or type(target["channels"]) is not int
            or target != {"sample_rate_hz": 48000, "sample_format": "PCM16", "channels": 1}):
        fail("voice_target", "expected planned 48000 Hz PCM16 mono")
    string(production["note_nb"], "production.note_nb")

    ids, scenes, docs, refs, next_refs = {}, {}, {}, [], []
    line_records = {}

    def identifier(value, kind):
        value = string(value, f"{kind}.id")
        if value in ids:
            fail(value, f"duplicate ID ({ids[value]} and {kind})")
        ids[value] = kind
        return value

    def requires(value, where):
        obj(value, where, ["all", "none"])
        for key in ("all", "none"):
            names = sequence(value[key], f"{where}.{key}")
            for name in names:
                string(name, where)
                if name not in internal and name not in external:
                    fail(where, f"undeclared flag {name}")
            if len(set(names)) != len(names):
                fail(where, f"duplicate {key} requirement")
        if set(value["all"]) & set(value["none"]):
            fail(where, "same flag in all and none")

    def effects(value, where):
        obj(value, where, ["set_flags", "give_documents"])
        for name, enabled in mapping(value["set_flags"], where).items():
            if name not in internal:
                fail(where, f"cannot set external/undeclared flag {name}")
            boolean(enabled, f"{where}.{name}")
        for doc in sequence(value["give_documents"], where):
            string(doc, where)
            refs.append((doc, where))

    for module in modules:
        for scene in module["scenes"]:
            obj(scene, "scene", ["id", "phase", "channel", "mode", "entry", "trigger_nb", "requires", "direction_nb", "lines", "choices", "on_complete"])
            sid = identifier(scene["id"], "scene")
            scenes[sid] = scene
            enum(scene["phase"], PHASES, sid)
            enum(scene["channel"], CHANNELS, sid)
            enum(scene["mode"], ["live", "fixed"], sid)
            enum(scene["entry"], ["trigger", "choice"], sid)
            for key in ("trigger_nb", "direction_nb"):
                string(scene[key], f"{sid}.{key}")
            requires(scene["requires"], f"{sid}.requires")
            effects(scene["on_complete"], f"{sid}.on_complete")
            for line in sequence(scene["lines"], f"{sid}.lines"):
                obj(line, f"{sid}.line", ["id", "speaker", "text", "delivery_nb"])
                lid = identifier(line["id"], "line")
                string(line["speaker"], lid)
                if line["speaker"] not in actors:
                    fail(lid, f"unknown actor {line['speaker']}")
                if scene["channel"] not in actors[line["speaker"]]["allowed_channels"]:
                    fail(lid, f"actor {line['speaker']} cannot use {scene['channel']}")
                string(line["text"], f"{lid}.text")
                string(line["delivery_nb"], f"{lid}.delivery_nb")
                line_records[lid] = line
            choices = sequence(scene["choices"], f"{sid}.choices")
            if scene["mode"] == "fixed" and choices:
                fail(sid, "fixed imprint cannot have choices")
            for choice in choices:
                obj(choice, sid, ["id", "label", "next", "requires", "on_select"])
                cid = identifier(choice["id"], "choice")
                string(choice["label"], cid)
                string(choice["next"], cid)
                next_refs.append((choice["next"], cid))
                requires(choice["requires"], f"{cid}.requires")
                effects(choice["on_select"], f"{cid}.on_select")
        for document in module["documents"]:
            obj(document, "document", ["id", "title", "kind", "stamp", "body", "production_nb"])
            did = identifier(document["id"], "document")
            docs[did] = document
            enum(document["kind"], ["hand", "typed", "photo", "map"], did)
            for key in ("title", "stamp", "production_nb"):
                string(document[key], f"{did}.{key}")
            body = sequence(document["body"], f"{did}.body")
            if not body:
                fail(did, "body cannot be empty")
            for paragraph in body:
                string(paragraph, f"{did}.body")
        for media in module["media"]:
            obj(media, "media", ["id", "title", "duration_seconds", "production_nb", "beats"])
            mid = identifier(media["id"], "media")
            for key in ("title", "production_nb"):
                string(media[key], f"{mid}.{key}")
            duration = number(media["duration_seconds"], mid)
            if duration <= 0:
                fail(mid, "duration must be positive")
            cursor = 0
            for beat in sequence(media["beats"], f"{mid}.beats"):
                obj(beat, mid, ["at", "until", "picture_nb", "audio_nb", "subtitle"])
                at, until = number(beat["at"], mid), number(beat["until"], mid)
                if not math.isclose(at, cursor, abs_tol=1e-7, rel_tol=0) or until <= at or until > duration:
                    fail(mid, f"beat must continue at {cursor} without gap/overlap and finish by {duration}")
                for key in ("picture_nb", "audio_nb"):
                    string(beat[key], f"{mid}.{key}")
                string(beat["subtitle"], f"{mid}.subtitle", empty=True)
                cursor = until
            if not math.isclose(cursor, duration, abs_tol=1e-7, rel_tol=0):
                fail(mid, "beats must cover full duration")
        for item in module["strings"]:
            obj(item, "string", ["id", "text", "context_nb"])
            tid = identifier(item["id"], "string")
            string(item["text"], tid)
            string(item["context_nb"], tid)
    for target_id, where in refs:
        if target_id not in docs:
            fail(where, f"unknown document {target_id}")
    for target_id, where in next_refs:
        if target_id not in scenes:
            fail(where, f"unknown next scene {target_id}")
        if scenes[target_id]["entry"] != "choice":
            fail(where, f"next {target_id} must have entry=choice")
    aliases = mapping(pack["recording_aliases"], "pack.recording_aliases")
    for alias, source in aliases.items():
        string(alias, "recording alias")
        string(source, f"recording alias {alias}")
        if alias not in line_records or source not in line_records:
            fail("recording_aliases", f"{alias} -> {source} must reference two existing lines")
        if alias == source or source in aliases:
            fail("recording_aliases", f"{alias} -> {source}: self-reuse, cycles and alias chains are forbidden")
        alias_line, source_line = line_records[alias], line_records[source]
        alias_actor, source_actor = actors[alias_line["speaker"]], actors[source_line["speaker"]]
        if not alias_actor["record"] or not source_actor["record"]:
            fail("recording_aliases", f"{alias} -> {source}: both lines must be recorded actors")
        if (alias_line["speaker"] != source_line["speaker"]
                or alias_actor["voice_profile"] != source_actor["voice_profile"]
                or alias_line["text"] != source_line["text"]):
            fail("recording_aliases", f"{alias} -> {source}: actor, voice profile and exact text must match")
    return pack, modules


def json_bytes(value):
    return (json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + "\n").encode("utf-8")


def render(pack, modules):
    script = ["# SIGNAL / 47: Evening Shift", "", "Forfatterpakke. Ikke runtime-integrert. Ingen lyd eller video er produsert.", "",
              "Spilltekst: engelsk. Regi: norsk. Medietider er planlagte, ikke ferdige opptakstider.", ""]
    voice = io.StringIO(newline="")
    writer = csv.writer(voice, lineterminator="\n")
    writer.writerow(["line_id", "scene_id", "actor", "channel", "voice_profile", "text", "delivery_nb", "status", "reuse_of"])
    recordings, subtitle_scenes, subtitle_media = [], [], []
    for module in modules:
        script += [f"## {module['module_id']}", ""]
        for scene in module["scenes"]:
            script += [f"### {scene['id']} ({scene['phase']} / {scene['channel']} / {scene['mode']})", "",
                       f"**Utløser:** {scene['trigger_nb']}", "", f"**Regi:** {scene['direction_nb']}", "",
                       f"**Krav:** {json.dumps(scene['requires'], ensure_ascii=False)}", ""]
            subs = []
            for line in scene["lines"]:
                actor = pack["actors"][line["speaker"]]
                script += [f"**{line['speaker']} [{line['id']}]**: {line['text']}", "", f"*{line['delivery_nb']}*", ""]
                if actor["record"]:
                    writer.writerow([line["id"], scene["id"], line["speaker"], scene["channel"], actor["voice_profile"], line["text"], line["delivery_nb"], "pending", pack["recording_aliases"].get(line["id"], "")])
                    recordings.append({"line_id": line["id"], "scene_id": scene["id"], "actor": line["speaker"],
                                       "channel": scene["channel"], "voice_profile": actor["voice_profile"],
                                       "status": "pending", "reuse_of": pack["recording_aliases"].get(line["id"]),
                                       "recording_required": line["id"] not in pack["recording_aliases"],
                                       "audio_file": None, "duration_seconds": None})
                subs.append({"line_id": line["id"], "speaker": line["speaker"], "text": line["text"],
                             "voiced": actor["record"], "start_seconds": None, "end_seconds": None,
                             "timing_status": "pending_audio" if actor["record"] else "interaction_controlled"})
            for choice in scene["choices"]:
                script += [f"- **{choice['id']}**: {choice['label']} → `{choice['next']}`",
                           f"  Krav: `{json.dumps(choice['requires'], ensure_ascii=False)}`; ved valg: `{json.dumps(choice['on_select'], ensure_ascii=False)}`"]
            script += ["", f"**Ved fullføring:** `{json.dumps(scene['on_complete'], ensure_ascii=False)}`", ""]
            subtitle_scenes.append({"scene_id": scene["id"], "channel": scene["channel"], "lines": subs})
        for doc in module["documents"]:
            script += [f"### {doc['id']}: {doc['title']}", "", f"{doc['kind']} / {doc['stamp']}", ""]
            for paragraph in doc["body"]:
                script += [paragraph, ""]
            script += [f"**Produksjon:** {doc['production_nb']}", ""]
        for media in module["media"]:
            script += [f"### {media['id']}: {media['title']}", "", f"Planlagt varighet: {media['duration_seconds']} s. Status: ikke produsert.", "", media["production_nb"], ""]
            beats = []
            for beat in media["beats"]:
                script += [f"**{beat['at']}–{beat['until']} s (planlagt)**", "", f"Bilde: {beat['picture_nb']}", "", f"Lyd: {beat['audio_nb']}", "", f"Undertekst: {beat['subtitle'] or '(ingen)'}", ""]
                beats.append({"start_seconds": beat["at"], "end_seconds": beat["until"], "text": beat["subtitle"]})
            subtitle_media.append({"media_id": media["id"], "title": media["title"], "timing_status": "planned_not_produced", "duration_seconds": media["duration_seconds"], "beats": beats})
        for item in module["strings"]:
            script += [f"**{item['id']}**: {item['text']}", "", f"*{item['context_nb']}*", ""]
    manifest = {"pack_id": pack["pack_id"], "status": "pending", "runtime_status": "not_integrated",
                "note_nb": "Kun opptaksplan. Ingen lydfiler er generert, målt eller importert.",
                "planned_format": pack["production"]["voice_target"],
                "cue_count": len(recordings),
                "required_recording_count": sum(item["recording_required"] for item in recordings),
                "recordings": recordings}
    subtitles = {"pack_id": pack["pack_id"], "language": pack["language"], "status": "authored_not_integrated",
                 "note_nb": "Dialogtider venter på lyd. Stille undersøkelsestekst styres av interaksjon. VHS-tider er kun planlagte.",
                 "scenes": subtitle_scenes, "media": subtitle_media}
    return {"SCRIPT.md": ("\n".join(script).rstrip() + "\n").encode("utf-8"),
            "voice_lines.csv": voice.getvalue().encode("utf-8"),
            "recording_manifest.json": json_bytes(manifest), "subtitles.json": json_bytes(subtitles)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="validate and compare all exports without writing")
    args = parser.parse_args()
    try:
        pack, modules = validate()
        outputs = render(pack, modules)
        output_dir = ROOT / "generated"
        if output_dir.is_symlink():
            fail(output_dir, "generated directory must not be a symlink")
        stale = []
        for name, payload in outputs.items():
            path = output_dir / name
            if path.is_symlink():
                fail(path, "output must not be a symlink")
            if args.check:
                if not path.is_file() or path.read_bytes() != payload:
                    stale.append(name)
            else:
                output_dir.mkdir(exist_ok=True)
                path.write_bytes(payload)
        if stale:
            fail("generated", "missing or stale: " + ", ".join(stale) + "; run build_pack.py")
        scene_count = sum(len(module["scenes"]) for module in modules)
        print(f"PASS: {len(modules)} modules, {scene_count} scenes; four exports {'byte-identical' if args.check else 'written'}; audio/video pending, runtime not integrated.")
        return 0
    except (PackError, OSError, TypeError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
