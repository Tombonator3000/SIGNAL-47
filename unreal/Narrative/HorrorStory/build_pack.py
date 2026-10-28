#!/usr/bin/env python3
"""Validate H/SC authoring and deterministically export review and runtime data.

Reuses EveningShift's strict scene/actor/flag/canon validation without changing
that pack. Compiling scenes supplies no world result or integration acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
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
    if type(data["schema_version"]) is not int or data["schema_version"] != 1 or data["status"] != "authored_not_integrated":
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


def validate_scenarios(pack, modules):
    """Validate the unified route without turning dialogue into world evidence."""
    data = EV.read_json(ROOT / "scenarios.json")
    EV.obj(data, "scenarios", ["schema_version", "status", "scenarios", "character_knowledge"])
    if type(data["schema_version"]) is not int or data["schema_version"] != 1 or data["status"] != "authored_not_integrated":
        EV.fail("scenarios", "expected authored schema 1")
    scenes = {s["id"]: s for m in modules for s in m["scenes"]}
    rows = EV.sequence(data["scenarios"], "scenarios.scenarios")
    if [r["id"] for r in rows] != [f"SC{i:02}" for i in range(1, 14)]:
        EV.fail("scenarios", "SC01 through SC13 must appear exactly once in order")
    for row in rows:
        EV.obj(row, row["id"], ["id", "title", "phase", "mission_ids", "scene_ids", "result_scene_ids",
               "existing_evidence_ids", "shared_owner_contracts", "world_contract", "runtime_status"])
        for key in ("title", "world_contract"):
            EV.string(row[key], f"{row['id']}.{key}")
        EV.enum(row["phase"], EV.PHASES, row["id"])
        if row["runtime_status"] != "authored_not_integrated":
            EV.fail(row["id"], "scenario writing does not establish runtime integration")
        for key in ("mission_ids", "scene_ids", "result_scene_ids", "existing_evidence_ids", "shared_owner_contracts"):
            items = EV.sequence(row[key], f"{row['id']}.{key}")
            if len(set(items)) != len(items):
                EV.fail(row["id"], f"duplicate {key}")
            for item in items:
                EV.string(item, row["id"])
        if not row["scene_ids"] or not row["result_scene_ids"] or not row["shared_owner_contracts"]:
            EV.fail(row["id"], "scenario needs scenes, factual results and existing-owner contracts")
        if not set(row["mission_ids"]).issubset({f"H{i:02}" for i in range(1, 11)}):
            EV.fail(row["id"], "existing H01 through H10 mission identity must be retained")
        if not set(row["result_scene_ids"]).issubset(row["scene_ids"]):
            EV.fail(row["id"], "result scenes must belong to this scenario")
        for sid in row["scene_ids"]:
            if sid not in scenes:
                EV.fail(row["id"], f"unknown scene {sid}")
        for sid in row["result_scene_ids"]:
            if not any(g in pack["external_flags"] for g in scenes[sid]["requires"]["all"]):
                EV.fail(sid, "a presentation flag cannot establish a world result")
    gates = {
        "SC01_FIRST_REPORT": {"ctx_hr_sc01_report_readback"},
        "SC02_ESCAPE": {"ctx_hr_sc02_t01_complete", "ctx_hr_sc02_escape_complete"},
        "SC03_DISPATCH_RECEIPT": {"ctx_hr_sc01_report_readback", "ctx_hr_sc03_dispatch_readback", "ctx_hr_sc03_ortiz_assigned"},
        "SC03_ORTIZ_IDENTIFY": {"ctx_hr_sc03_ortiz_assigned", "ctx_hr_sc03_ortiz_identified"},
        "SC03_HELPER_TAKEN": {"ctx_hr_sc03_lift_observed", "ctx_hr_sc03_safe_retreat"},
        "SC03_HELPER_TRACES": {"ctx_hr_sc03_safe_retreat", "ctx_hr_sc03_traces_checked"},
        "SC03_MISSING_RECEIPT": {"ctx_hr_sc03_ortiz_identified", "ctx_hr_sc03_dispatch_readback", "ctx_hr_sc03_lift_observed", "ctx_hr_sc03_missing_readback"},
        "SC03_LOCAL_ORDER_HOLD": {"ctx_hr_sc03_local_order_suspended"},
        "SC04_LOCAL_HOLD": {"ctx_hr_sc04_schedule_isolated", "ctx_hr_sc04_shift_originals_secured"},
        "SC04_ANNEX_LONG_ESCAPE": {"ctx_hr_sc04_annex_long_escape"},
        "SC04_ANNEX_SOUND_ESCAPE": {"ctx_hr_sc04_annex_sound_escape"},
        "SC04_ANNEX_IMPRINT": {"ctx_hr_sc01_dale_reference_heard", "ctx_hr_sc04_annex_imprint_playing"},
        "SC04_CAR_READ": {"ctx_hr_sc04_car_threat_visible", "ctx_hr_sc04_after_annex_calm", "ctx_hr_sc04_car_route_known"},
        "SC04_CAR_ESCAPE": {"ctx_hr_sc04_car_escape", "ctx_hr_sc04_car_route_known"},
        "SC04_SAFE_ROOM_AGREEMENT": {"ctx_hr_sc04_control_room_safe", "ctx_hr_sc04_ward_on_line", "ctx_hr_sc04_schedule_isolated"},
        "SC05_PASSIVE_RESIDUAL": {"ctx_hr_sc04_schedule_isolated", "ctx_hr_sc05_residual_passive"},
        "BY02_SOURCE_COMPARE": {"ctx_hr_by02_reel_identity", "ctx_hr_by02_tape_sample_verified", "ctx_hr_by02_live_sample_verified", "ctx_hr_by02_question_compared"},
        "BY02_LATE_COPY": {"ctx_hr_by02_dated_lead", "ctx_hr_by02_source_copy_verified", "ctx_hr_by02_question_compared"},
        "ARRAY_RESULT": {"ctx_hr_array_command_checked", "ctx_hr_array_command_history_compared", "ctx_hr_array_reference_fixed", "ctx_hr_array_a_observed", "ctx_hr_array_b_observed", "ctx_hr_array_return_observed", "ctx_hr_array_geometry_compared"},
        "ARRAY_RETREAT": {"ctx_hr_array_geometry_compared", "ctx_hr_array_escape_complete"},
        "ARRAY_EMPTY_HOLD": {"ctx_hr_array_escape_complete", "ctx_hr_array_empty_hold_observed"},
        "SC12_ROUTE_READBACK": {"ctx_hr_warning_acknowledged", "ctx_hr_h09_checked", "ctx_hr_sc12_driver_recent_passage", "ctx_hr_sc12_exit_sighting_readback", "ctx_hr_sc12_courier_104_confirmed"},
        "H09_BRIEF": {"ctx_hr_sc12_driver_recent_passage"},
        "H09_CHECK": {"ctx_hr_sc12_courier_104_confirmed"},
        "H10_BRIEF": {"ctx_hr_sc12_driver_recent_passage", "ctx_hr_sc12_exit_sighting_readback", "ctx_hr_sc12_courier_104_confirmed"},
        "H07_FIRST_WARNING": {"ctx_hr_h07_bundle_secured"},
        "H07_CHECK": {"ctx_hr_h07_bundle_secured"},
        "H07_ESCAPE_COMPLETE": {"ctx_hr_h07_bundle_secured"},
        "H08_COMPARE_RECORDS": {"ctx_hr_h07_original_secured", "ctx_hr_h07_bundle_secured"},
    }
    for sid, required in gates.items():
        if sid not in scenes or not required.issubset(scenes[sid]["requires"]["all"]):
            EV.fail(sid, "unified scenario world-result gate missing")
    document_sources = {
        "SC01_FIRST_REPORT": ["HR_INITIAL_REPORT"],
        "SC03_DISPATCH_RECEIPT": ["HR_DISPATCH_RECEIPT"],
        "SC03_MISSING_RECEIPT": ["HR_MISSING_RECEIPT"],
        "SC03_LOCAL_ORDER_HOLD": ["HR_LOCAL_HOLD_RECEIPT"],
        "SC04_LOCAL_HOLD": ["HR_LOCAL_ISOLATION_NOTE"],
        "BY02_SOURCE_COMPARE": ["HR_EARL_SOURCE_CHECK"],
        "BY02_LATE_COPY": ["HR_EARL_SOURCE_COPY"],
        "ARRAY_RESULT": ["HR_ARRAY_CONTROL_NOTE"],
        "SC12_ROUTE_READBACK": ["HR_ROUTE_READBACK"],
    }
    for sid, expected in document_sources.items():
        if scenes[sid]["on_complete"]["give_documents"] != expected:
            EV.fail(sid, "keep initial report, dispatch, missing case, holds, source paths and route receipts separate")
    for sid, forbidden in (("ARRAY_POINT_A", "ctx_hr_array_b_observed"),
                           ("ARRAY_POINT_B", "ctx_hr_array_a_observed")):
        if forbidden in scenes[sid]["requires"]["all"]:
            EV.fail(sid, "either operator position must be available first")
    future_array_results = {"ctx_hr_array_a_observed", "ctx_hr_array_b_observed",
                            "ctx_hr_array_return_observed", "ctx_hr_array_geometry_compared",
                            "ctx_hr_array_command_history_compared"}
    if future_array_results.intersection(scenes["ARRAY_COMMAND_CHECK"]["requires"]["all"]):
        EV.fail("ARRAY_COMMAND_CHECK", "initial command baseline cannot require the future field result")
    if {"ctx_hr_by02_tape_isolated", "ctx_hr_by02_live_selected"}.intersection(scenes["BY02_SOURCE_COMPARE"]["requires"]["all"]):
        EV.fail("BY02_SOURCE_COMPARE", "use durable completed samples, not simultaneous source-selector positions")
    for scene in scenes.values():
        awarded = {flag for flag, enabled in scene["on_complete"]["set_flags"].items() if enabled}
        if awarded.intersection(scene["requires"]["all"]):
            EV.fail(scene["id"], "scene cannot require its own completion as an entry condition")
    for module in modules:
        if module["module_id"] != "unified_scenarios":
            continue
        for scene in module["scenes"]:
            sid = scene["id"]
            for name in scene["on_complete"]["set_flags"]:
                if not name.endswith("_presented"):
                    EV.fail(sid, "new dialogue can record presentation only, not a physical outcome")
            if sid.endswith(("_RETRY", "_INCOMPLETE")) and scene["on_complete"] != {"set_flags": {}, "give_documents": []}:
                EV.fail(sid, "retry or incomplete result cannot award evidence or completion")
            if any(line["speaker"] == "EARL" for line in scene["lines"]):
                if "ctx_hr_by02_earl_present" not in scene["requires"]["all"]:
                    EV.fail(sid, "Earl cannot speak after his departure")
    knowledge = EV.sequence(data["character_knowledge"], "character_knowledge")
    seen = set()
    for row in knowledge:
        EV.obj(row, "knowledge", ["actor", "knows", "does_not_know", "gates"])
        if row["actor"] not in pack["actors"] or row["actor"] in seen:
            EV.fail("knowledge", "actor must be declared and unique")
        seen.add(row["actor"])
        for key in ("knows", "does_not_know", "gates"):
            if not EV.sequence(row[key], f"knowledge.{key}"):
                EV.fail(row["actor"], f"empty knowledge boundary: {key}")
            for item in row[key]:
                EV.string(item, row["actor"])
        if not set(row["gates"]).issubset(pack["external_flags"]):
            EV.fail(row["actor"], "knowledge needs declared world-owned provenance gates")
    return data


def render_scenarios(data):
    out = ["# SC01 til SC13: produksjonskontrakt", "", "Generert fra scenarios.json. Forfatterkilder; ingen ny runtime eller målte spillertider.", ""]
    for row in data["scenarios"]:
        out += [f"## {row['id']}: {row['title']}", "", row["world_contract"], "",
                "**Bevarte hovedkort:** " + ", ".join(row["mission_ids"]), "",
                "**Delsscener:** " + ", ".join(f"`{sid}`" for sid in row["scene_ids"]), "",
                "**Fysiske resultater før presentasjon:** " + ", ".join(f"`{sid}`" for sid in row["result_scene_ids"]), "",
                "**Eierskap:** " + " ".join(row["shared_owner_contracts"]), ""]
    out += ["## Personkunnskap", ""]
    for row in data["character_knowledge"]:
        out += [f"### {row['actor']}", "", "**Kan vite:** " + " ".join(row["knows"]), "",
                "**Kan ikke vite:** " + " ".join(row["does_not_know"]), "",
                "**Kildeporter:** " + ", ".join(f"`{x}`" for x in row["gates"]), ""]
    return ("\n".join(out).rstrip() + "\n").encode("utf-8")


PRODUCER_STATUS = ("existing", "partial", "new")
TEST_KINDS = ("data", "api", "input", "image", "human")
TEST_COVERS = ("normal", "alternative", "interrupt", "continue", "negative", "image", "audio", "regression")


def validate_producers(pack, modules, registry, data=None):
    """Name a world producer, scene matrix row and test for every SC fact.

    This is planning data for the runtime host. It cannot mark a scene
    integrated; it only rejects unowned flags, invented code references and
    scenarios whose test set misses one of the plan's eight test categories.
    """
    if data is None:
        data = EV.read_json(ROOT / "world_producers.json")
    EV.obj(data, "world_producers", ["schema_version", "status", "note_nb", "flags", "scenes", "tests", "work_packages"])
    if type(data["schema_version"]) is not int or data["schema_version"] != 1 or data["status"] != "planned_not_integrated":
        EV.fail("world_producers", "expected planning schema 1")
    repo = ROOT.parents[2]
    owners = {row["id"]: row["scenario_ids"] for row in registry["scenes"]}
    scenario_ids = [f"SC{i:02}" for i in range(1, 14)]
    first = {}
    for row in registry["scenes"]:
        for gate in row["external_requirements"]:
            first.setdefault(gate, row["scenario_ids"][0])
    tests = {}
    for row in EV.sequence(data["tests"], "tests"):
        EV.obj(row, "test", ["id", "scenario", "kind", "covers", "description_nb"])
        tid = row["id"]
        if tid in tests or not tid.startswith(row["scenario"] + "-T"):
            EV.fail(tid, "test id must be unique and prefixed by its scenario")
        EV.enum(row["scenario"], scenario_ids, tid)
        EV.enum(row["kind"], TEST_KINDS, tid)
        EV.string(row["description_nb"], tid)
        if not EV.sequence(row["covers"], tid):
            EV.fail(tid, "test must cover at least one category")
        for item in row["covers"]:
            EV.enum(item, TEST_COVERS, tid)
        tests[tid] = row

    def refs(row, where, scenario):
        items = EV.sequence(row["tests"], where)
        if not items:
            EV.fail(where, "needs at least one named test")
        for tid in items:
            if tid not in tests:
                EV.fail(where, f"unknown test {tid}")
        # Later scenarios may also exercise a fact; its own scenario must test it first.
        if not any(tests[tid]["scenario"] == scenario for tid in items):
            EV.fail(where, f"needs a test in {scenario}")

    seen = set()
    for row in EV.sequence(data["flags"], "flags"):
        EV.obj(row, "flag", ["flag", "scenario", "status", "producer", "sources", "symbols",
                             "world_action_nb", "new_work_nb", "tests"])
        flag = row["flag"]
        if flag not in pack["external_flags"] or flag in seen:
            EV.fail(flag, "producer must name one declared external flag exactly once")
        seen.add(flag)
        if row["scenario"] != first.get(flag):
            EV.fail(flag, f"scenario must be the first scenario that requires it ({first.get(flag)})")
        EV.enum(row["status"], PRODUCER_STATUS, flag)
        for key in ("producer", "world_action_nb"):
            EV.string(row[key], f"{flag}.{key}")
        EV.string(row["new_work_nb"], f"{flag}.new_work_nb", empty=row["status"] == "existing")
        sources = EV.sequence(row["sources"], flag)
        symbols = EV.sequence(row["symbols"], flag)
        if not sources:
            EV.fail(flag, "producer needs an owning source file")
        texts = []
        for source in sources:
            EV.string(source, flag)
            planned = source.endswith(" (ny)")
            path = (repo / source.removesuffix(" (ny)")).resolve()
            if not path.is_relative_to(repo):
                EV.fail(flag, f"source outside repository: {source}")
            if planned:
                if row["status"] == "existing" or path.exists():
                    EV.fail(flag, f"planned file must be absent and belong to unfinished work: {source}")
                continue
            if not path.is_file():
                EV.fail(flag, f"source file does not exist: {source}")
            texts.append(path.read_text(encoding="utf-8", errors="replace"))
        if row["status"] != "new" and (not symbols or not texts):
            EV.fail(flag, "existing or partial producer needs a verifiable code symbol")
        for symbol in symbols:
            EV.string(symbol, flag)
            if not any(symbol in text for text in texts):
                EV.fail(flag, f"symbol not found in named sources: {symbol}")
        refs(row, flag, row["scenario"])
    missing = sorted(set(pack["external_flags"]) - seen)
    if missing:
        EV.fail("world_producers", "flags without a named world producer: " + ", ".join(missing))

    seen = set()
    for row in EV.sequence(data["scenes"], "scenes"):
        EV.obj(row, "scene row", ["scene", "scenario", "start_nb", "normal_nb", "interrupt_nb",
                                  "alternative_nb", "models", "audio", "documents", "tests"])
        sid = row["scene"]
        if sid not in owners or sid in seen:
            EV.fail(sid, "matrix needs each authored scene exactly once")
        seen.add(sid)
        if row["scenario"] != owners[sid][0]:
            EV.fail(sid, f"scenario must match the runtime registry ({owners[sid][0]})")
        for key in ("start_nb", "normal_nb", "interrupt_nb", "alternative_nb"):
            EV.string(row[key], f"{sid}.{key}")
        for key in ("models", "audio", "documents"):
            for item in EV.sequence(row[key], f"{sid}.{key}"):
                EV.string(item, sid)
        refs(row, sid, row["scenario"])
    missing = sorted(set(owners) - seen)
    if missing:
        EV.fail("world_producers", "scenes without a matrix row: " + ", ".join(missing))

    for scenario in scenario_ids:
        covered = {c for t in tests.values() if t["scenario"] == scenario for c in t["covers"]}
        gap = [c for c in TEST_COVERS if c not in covered]
        if gap:
            EV.fail(scenario, "test set misses plan categories: " + ", ".join(gap))
    packages = EV.sequence(data["work_packages"], "work_packages")
    if [row.get("scenario") for row in packages] != scenario_ids:
        EV.fail("work_packages", "need one package per scenario in SC order")
    for row in packages:
        EV.obj(row, row["scenario"], ["scenario", "summary_nb", "code_nb", "assets_nb", "voice_nb",
                                      "dependencies_nb", "kubuntu_only_nb", "risks_nb"])
        EV.string(row["summary_nb"], row["scenario"])
        EV.string(row["voice_nb"], row["scenario"])
        for key in ("code_nb", "assets_nb", "dependencies_nb", "kubuntu_only_nb", "risks_nb"):
            for item in EV.sequence(row[key], f"{row['scenario']}.{key}"):
                EV.string(item, row["scenario"])
    return data


def cell(text):
    return text.replace("|", "\\|").replace("\n", " ")


def render_matrix(data, recordings):
    """Readable per-SC matrix; voice cues come from the measured manifest."""
    cues = {}
    for rec in recordings["recordings"]:
        cues.setdefault(rec["scene_id"], []).append(rec)
    status = {s: sum(1 for f in data["flags"] if f["status"] == s) for s in PRODUCER_STATUS}
    out = ["# SC01 til SC13: verdensprodusenter, scenematrise og testsett", "",
           "Generert fra world_producers.json. Dette er planleggingsdata: ingen rad betyr at scenen er "
           "integrert, bygget eller prøvd i spillet.", "",
           f"Flagg: {len(data['flags'])} ({status['existing']} finnes i koden, {status['partial']} delvis, "
           f"{status['new']} må bygges). Scener: {len(data['scenes'])}. Tester: {len(data['tests'])}.", ""]
    for package in data["work_packages"]:
        sc = package["scenario"]
        out += [f"## {sc}", "", package["summary_nb"], ""]
        for label, key in (("Kode", "code_nb"), ("Ressurser", "assets_nb"), ("Avhenger av", "dependencies_nb"),
                           ("Bare på Kubuntu", "kubuntu_only_nb"), ("Risiko og avklaringer", "risks_nb")):
            if package[key]:
                out += [f"**{label}:**", ""] + [f"- {item}" for item in package[key]] + [""]
        out += [f"**Stemmer:** {package['voice_nb']}", "", "### Verdensflagg", "",
                "| Flagg | Status | Produsent | Handling i spillet | Tester |", "|---|---|---|---|---|"]
        for row in data["flags"]:
            if row["scenario"] == sc:
                out.append(f"| `{row['flag']}` | {row['status']} | {cell(row['producer'])} | "
                           f"{cell(row['world_action_nb'])} | " + ", ".join(row["tests"]) + " |")
        out += ["", "### Scener", ""]
        for row in data["scenes"]:
            if row["scenario"] != sc:
                continue
            recs = cues.get(row["scene"], [])
            recorded = sum(1 for r in recs if r["status"] == "recorded")
            out += [f"**`{row['scene']}`**: {row['start_nb']}", "",
                    f"- Normalt: {row['normal_nb']}", f"- Avbrudd: {row['interrupt_nb']}",
                    f"- Alternativ: {row['alternative_nb']}",
                    f"- Stemmecues: {len(recs)}, innspilt {recorded}",
                    "- Modeller: " + ("; ".join(row["models"]) or "ingen nye"),
                    "- Lyd: " + ("; ".join(row["audio"]) or "ingen nye"),
                    "- Dokumenter: " + (", ".join(row["documents"]) or "ingen"),
                    "- Tester: " + ", ".join(row["tests"]), ""]
        out += ["### Testsett", "", "| Test | Type | Dekker | Prøve |", "|---|---|---|---|"]
        for row in data["tests"]:
            if row["scenario"] == sc:
                out.append(f"| {row['id']} | {row['kind']} | {', '.join(row['covers'])} | {cell(row['description_nb'])} |")
        out.append("")
    return ("\n".join(out).rstrip() + "\n").encode("utf-8")


def runtime_registry(pack, modules, scenarios):
    """Map every authored scene to its SC/H owners, without changing its phase.

    Six original H01/H02 cards precede the later SC table. They belong to SC01
    and SC02 respectively; omitting them would silently drop the first cards.
    This registry describes data ownership, never accepted gameplay completion.
    """
    all_scenes = [scene for module in modules for scene in module["scenes"]]
    registrations = {scene["id"]: [] for scene in all_scenes}
    rows = {row["id"]: row for row in scenarios["scenarios"]}
    for row in scenarios["scenarios"]:
        for sid in row["scene_ids"]:
            registrations[sid].append(row["id"])
    for mission, scenario in (("H01", "SC01"), ("H02", "SC02")):
        for suffix in ("_BRIEF", "_CHECK", "_INCOMPLETE"):
            sid = mission + suffix
            if not registrations[sid]:
                registrations[sid].append(scenario)
    missing = [sid for sid, owners in registrations.items() if not owners]
    if missing:
        EV.fail("runtime registry", "unmapped authored scenes: " + ", ".join(missing))
    result = []
    for scene in all_scenes:
        sid = scene["id"]
        scenario_ids = registrations[sid]
        mission_ids = list(dict.fromkeys(mid for sc in scenario_ids for mid in rows[sc]["mission_ids"]))
        result.append({"id": sid, "phase": scene["phase"], "scenario_ids": scenario_ids,
                       "mission_ids": mission_ids,
                       "external_requirements": [gate for gate in scene["requires"]["all"]
                                                 if gate in pack["external_flags"]]})
    return {"schema_version": 1, "pack_id": pack["pack_id"],
            "status": "runtime_data_exported_gameplay_acceptance_separate",
            "note_nb": "Alle forfatterscener er registrert. Verdensporter og per-SC aksept eies av runtime-verten.",
            "external_flags": [{"id": gate, "owner": "runtime_host",
                                "provenance_nb": pack["external_flags"][gate]["purpose_nb"]}
                               for gate in sorted(pack["external_flags"])],
            "scenes": result}


def render_runtime(pack, modules, registry):
    data = {"pack": pack, "modules": modules, "registry": registry}
    runtime_json = json.dumps(data, ensure_ascii=True, separators=(",", ":"), allow_nan=False)
    lines = ["// Generated by Narrative/HorrorStory/build_pack.py. Do not edit.",
             "// Source scenes are data, not an assertion of gameplay integration.",
             "static const TCHAR* S47HorrorPackJson ="]
    for start in range(0, len(runtime_json), 6000):
        chunk = runtime_json[start:start + 6000]
        if ')S47HR"' in chunk:
            EV.fail("runtime JSON", "raw string delimiter collision")
        lines.append('TEXT(R"S47HR(' + chunk + ')S47HR")')
    lines.append(";\n")
    return "\n".join(lines).encode("ascii")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        pack, modules = EV.validate()
        missions = validate_missions(modules)
        scenarios = validate_scenarios(pack, modules)
        outputs = EV.render(pack, modules)
        recordings = json.loads(outputs["recording_manifest.json"])
        recordings["note_nb"] = (
            "Rene takes (48 kHz PCM16 mono, uten kanalfilter). audio_file er relativt "
            "til lydroten (se voice/README.md); varighet er målt fra filen. "
            "Runtime-status gjelder hele forfatterpakken. Delvis SC01-import og "
            "faktisk spillaksept føres separat i voice/README.md og SC01_RUNTIME.md."
        )
        outputs["recording_manifest.json"] = EV.json_bytes(recordings)
        # Keep the separate pack ID and symbols out of FS47EveningPack's build.
        del outputs["EveningPackData.inl"]
        registry = runtime_registry(pack, modules, scenarios)
        source = EV.json_bytes({"pack": pack, "modules": modules})
        registry["source_sha256"] = hashlib.sha256(source).hexdigest()
        outputs["HorrorPackData.inl"] = render_runtime(pack, modules, registry)
        outputs["runtime_registry.json"] = EV.json_bytes(registry)
        producers = validate_producers(pack, modules, registry)
        outputs["SCENE_MATRIX.md"] = render_matrix(producers, recordings)
        outputs["SCRIPT.md"] = outputs["SCRIPT.md"].replace(
            b"# SIGNAL / 47: Evening Shift", b"# SIGNAL / 47: Horror Story H1", 1)
        outputs["MISSION_CARDS.md"] = render_missions(missions)
        outputs["SCENARIO_CONTRACTS.md"] = render_scenarios(scenarios)
        outputs["integration_index.json"] = EV.json_bytes({
            "pack_id": pack["pack_id"], "status": "authored_not_integrated",
            "note_nb": "Lesbar forfatterindeks. runtime_registry.json registrerer alle scener; spillaksept føres separat per SC.",
            "missions": [{"id": m["id"], "scene_ids": m["scene_ids"],
                          "existing_evidence_ids": m["existing_evidence_ids"],
                          "source_files": m["source_files"]} for m in missions],
            "scenarios": scenarios["scenarios"], "character_knowledge": scenarios["character_knowledge"]})
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
        print(f"PASS: {len(missions)} missions, {len(scenarios['scenarios'])} scenarios, {sum(len(m['scenes']) for m in modules)} scenes; "
              f"{len(outputs)} exports {'byte-identical' if args.check else 'written'}. Runtime data exported; gameplay acceptance and media tracked separately.")
        return 0
    except (EV.PackError, OSError, TypeError, KeyError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
