#!/usr/bin/env python3
"""Validate H1 authoring and export review files without touching runtime data.

Reuses the strict scene/actor/flag/canon validation from EveningShift. This
adapter deliberately emits no C++ include and does not modify EveningShift.
"""
from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("s47_evening_authoring", ROOT.parent / "EveningShift" / "build_pack.py")
EV = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EV)
EV.ROOT = ROOT


def validate_missions(modules):
    data = EV.read_json(ROOT / "missions.json")
    EV.obj(data, "missions", ["schema_version", "status", "missions"])
    if data["schema_version"] != 1 or data["status"] != "authored_not_integrated":
        EV.fail("missions", "expected authored schema 1")
    scenes = {s["id"]: s for m in modules for s in m["scenes"]}
    rows = EV.sequence(data["missions"], "missions.missions")
    expected = [f"H{i:02}" for i in range(1, 11)]
    if [r["id"] for r in rows] != expected:
        EV.fail("missions", "H01 through H10 must appear once in order")
    for row in rows:
        EV.obj(row, row["id"], ["id", "title", "phase", "question", "physical_action", "control",
               "consequence", "insufficient_evidence", "scene_ids", "existing_evidence_ids",
               "source_files", "optional_clues", "human_minutes", "runtime_status"])
        for key in ("title", "question", "physical_action", "control", "consequence", "insufficient_evidence"):
            EV.string(row[key], f"{row['id']}.{key}")
        EV.enum(row["phase"], EV.PHASES, row["id"])
        if row["human_minutes"] is not None:
            EV.fail(row["id"], "human duration is unmeasured; keep null")
        if row["runtime_status"] != "authored_not_integrated":
            EV.fail(row["id"], "production writing is not runtime integration")
        for key in ("scene_ids", "existing_evidence_ids", "source_files", "optional_clues"):
            EV.sequence(row[key], row["id"])
            for item in row[key]:
                EV.string(item, row["id"])
        for source in row["source_files"]:
            path = (ROOT.parents[2] / source).resolve()
            if not path.is_relative_to(ROOT.parents[2]) or not path.is_file():
                EV.fail(row["id"], f"source file does not exist inside repository: {source}")
        required = [row["id"] + suffix for suffix in ("_BRIEF", "_CHECK", "_INCOMPLETE")]
        if row["scene_ids"] != required:
            EV.fail(row["id"], "need brief, verified result and incomplete-result scene")
        for sid in required:
            if sid not in scenes or scenes[sid]["phase"] != row["phase"]:
                EV.fail(row["id"], f"missing scene or inconsistent phase: {sid}")
        check = scenes[row["id"] + "_CHECK"]
        if not any(x.endswith("_checked") for x in check["requires"]["all"]):
            EV.fail(row["id"], "result must require a world-owned check")
        incomplete = scenes[row["id"] + "_INCOMPLETE"]
        if incomplete["on_complete"] != {"set_flags": {}, "give_documents": []}:
            EV.fail(row["id"], "insufficient evidence cannot award a supported conclusion")
    # Keep claims about world results distinct from presenting a conclusion.
    factual_gates = {
        "H07_ESCAPE_COMPLETE": {"ctx_hr_h07_original_secured", "ctx_hr_h07_escape_complete"},
        "H08_CHECK": {"ctx_hr_remote_sources_verified"},
        "H08_WARNING_ACK": {"ctx_hr_remote_sources_verified", "ctx_hr_warning_acknowledged"},
        "H09_BRIEF": {"ctx_hr_warning_acknowledged"},
        "H10_BRIEF": {"ctx_hr_warning_acknowledged"},
        "H10_CHECK": {"ctx_hr_h10_stalled"},
    }
    for sid, required in factual_gates.items():
        if sid not in scenes or not required.issubset(scenes[sid]["requires"]["all"]):
            EV.fail(sid, "world claim needs its independent factual gate")
    return rows


def render_missions(rows):
    out = ["# H01 til H10: produksjonskort", "", "Generert fra missions.json. Ingen menneskelig varighet er målt.", ""]
    for row in rows:
        out += [f"## {row['id']}: {row['title']}", "", f"**Spørsmål:** {row['question']}", ""]
        for label, key in [("Fysisk handling", "physical_action"), ("Kontroll", "control"),
                           ("Følge", "consequence"), ("Utilstrekkelig bevis", "insufficient_evidence")]:
            out += [f"**{label}:** {row[key]}", ""]
        out += ["**Eksisterende ID-er beholdes:** " + ", ".join(f"`{x}`" for x in row["existing_evidence_ids"]), "",
                "**Valgfri fordypning:** " + ("; ".join(row["optional_clues"]) or "Ingen valgfri kilde bærer hovedslutningen."), "",
                "**Menneskelig tid:** UNVERIFIED. **Runtime:** ikke integrert.", ""]
    return ("\n".join(out).rstrip() + "\n").encode("utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        pack, modules = EV.validate()
        missions = validate_missions(modules)
        outputs = EV.render(pack, modules)
        # Keep this authoring package disconnected from FS47EveningPack's build.
        del outputs["EveningPackData.inl"]
        outputs["SCRIPT.md"] = outputs["SCRIPT.md"].replace(
            b"# SIGNAL / 47: Evening Shift", b"# SIGNAL / 47: Horror Story H1", 1)
        outputs["MISSION_CARDS.md"] = render_missions(missions)
        outputs["integration_index.json"] = EV.json_bytes({
            "pack_id": pack["pack_id"], "status": "authored_not_integrated",
            "note_nb": "Lesbar adapterindeks. Ingen runtime-loader eller kapittel-/lagringsendring er koblet inn.",
            "missions": [{"id": m["id"], "scene_ids": m["scene_ids"],
                          "existing_evidence_ids": m["existing_evidence_ids"],
                          "source_files": m["source_files"]} for m in missions]})
        directory = ROOT / "generated"
        if directory.is_symlink():
            EV.fail("generated", "directory cannot be a symlink")
        stale = []
        for name, payload in outputs.items():
            path = directory / name
            if path.is_symlink():
                EV.fail(name, "output cannot be a symlink")
            if args.check:
                if not path.is_file() or path.read_bytes() != payload:
                    stale.append(name)
            else:
                directory.mkdir(exist_ok=True)
                path.write_bytes(payload)
        if stale:
            EV.fail("generated", "stale or missing: " + ", ".join(stale))
        print(f"PASS: {len(missions)} missions, {sum(len(m['scenes']) for m in modules)} scenes; "
              f"{len(outputs)} exports {'byte-identical' if args.check else 'written'}. Audio, runtime and human timing pending.")
        return 0
    except (EV.PackError, OSError, TypeError, KeyError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
