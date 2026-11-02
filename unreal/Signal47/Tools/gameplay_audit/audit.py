#!/usr/bin/env python3
"""Read-only evidence collection and agent-finding aggregation; never runs the game."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
CPP = "unreal/Signal47/Source/Signal47/Private/S47Freeform.cpp"
HEADER = "unreal/Signal47/Source/Signal47/Public/S47Freeform.h"
STAGES = ["New", "Pointed", "Integrating", "Chart", "Recorded", "Registered", "Packet", "Filed"]
REQUIRED_FINDING = ("id", "priority", "owner", "scenario", "confidence", "trigger", "consequence", "fix", "acceptance", "evidence")
CONFIDENCE = ("source_confirmed", "save_observed", "risk", "proposal", "runtime_observed", "human_observed")


def function_body(source, name):
    # Tokenize strings/comments so nested lambdas and one-line functions hash safely.
    match = re.search(r"^[^\n]*FS47FreeformSession::" + re.escape(name) + r"\([^\n]*", source, re.M)
    if not match:
        return ""
    start = source.find("{", match.start())
    if start < 0:
        return ""
    depth = 0
    tokens = re.finditer(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[start:], re.S)
    for token in tokens:
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():start + token.end()]
    return ""


def source_context(repo):
    def git(*args):
        return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()
    source = (repo / CPP).read_text()
    header = (repo / HEADER).read_text()
    contract = json.loads((HERE / "source_contract.json").read_text())
    observed = {}
    checks = [hashlib.sha256(source.encode()).hexdigest() == contract.get("source_sha256")]
    for name, expected in contract["functions"].items():
        body = function_body(source, name)
        observed[name] = hashlib.sha256(body.encode()).hexdigest()
        checks.append(bool(body) and observed[name] == expected)
    enum = re.search(r"enum class EJobStage\s*:\s*uint8\s*\{([^}]+)\}", header)
    enum_matches = bool(enum) and [s.strip() for s in enum.group(1).split(",")] == STAGES
    return {
        "revision": git("rev-parse", "HEAD"),
        "dirty": bool(git("status", "--porcelain", "--untracked-files=normal")),
        "source_file": CPP,
        "source_contract": "recognized" if all(checks) and enum_matches else "UNVERIFIED_SOURCE_CHANGED",
        "function_sha256": observed,
        "receiver_hint_line": next((i for i, line in enumerate(source.splitlines(), 1) if "FString FS47FreeformSession::ReceiverNextAction" in line), None),
    }


def job_next(job, stage):
    if stage == "Recorded":
        if not job.get("anomaly"):
            return "OPS: Register routine result / $20. Original already collected."
        if not job.get("control"):
            return "Tracking: measure west, east and centre beams." if job.get("phenomenon") == "wrong_source" else "Receiver rack: record independent internal control."
        if job.get("controlAt", 0) < 8:
            return "Wait for the independent control to complete."
        if job.get("phenomenon") == "duplicate_print" and not job.get("duplicateCollected"):
            return "Printer: collect the separate duplicate; preserve the original."
        if job.get("reviewed"):
            return "OPS: register the supported conclusion."
        if job.get("phenomenon") in ("wrong_source", "time_disagreement", "duplicate_print"):
            return "OPS: compare the retained instrument measurements."
        return "Reel deck by telephone: review both recordings and mark the extra ticks."
    return {
        "New": "Tracking PC: point the selected source.",
        "Pointed": "Check live encoder agreement, settling and receiver settings before acquisition.",
        "Integrating": "Check live tuning: if invalid, restore work-card tuning. With valid tuning, wait during automatic reference-scan movement; otherwise restore pointing only if live encoders/settling require it. Valid recorded seconds are retained.",
        "Chart": "Printer: collect the original chart.",
        "Registered": "Printer: collect the labelled registered packet.",
        "Packet": "Records room: file the originals in the binder.",
        "Filed": "OPS: select more work or follow up the archive.",
    }.get(stage)


def save_summary(payload, source):
    if not isinstance(payload, dict):
        raise ValueError("Save must be an object")
    state = payload.get("state", payload)
    if not isinstance(state, dict):
        raise ValueError("Save state must be an object")
    freeform = state.get("freeform")
    if not isinstance(freeform, dict):
        return {"status": "NO_FREEFORM_STATE", "interpretation": "Story and other save formats require a separate review."}
    jobs, selected = freeform.get("jobs", []), freeform.get("selected")
    if not isinstance(jobs, list) or type(selected) is not int or not 0 <= selected < len(jobs):
        raise ValueError("Invalid selected Freeform job")
    job = jobs[selected]
    if not isinstance(job, dict):
        raise ValueError("Selected job must be an object")
    stage = job.get("stage")
    if type(stage) is not int or not 0 <= stage < len(STAGES):
        raise ValueError("Unknown Freeform job stage")
    # Export only generated identifiers and mechanics, never documents, photos or the full save.
    job_id = job.get("id", "")
    if not isinstance(job_id, str) or not re.fullmatch(r"FF-D\d+-J\d+", job_id):
        job_id = "[nonstandard identifier omitted]"
    if not isinstance(job.get("controlAt", 0), (int, float)):
        raise ValueError("Control time must be numeric")
    result = {"status": "save_observed", "job_id": job_id,
              "stage_number": stage, "stage": STAGES[stage] if source["source_contract"] == "recognized" else "UNVERIFIED",
              "printed": bool(job.get("printed")), "paid": bool(job.get("paid")),
              "anomaly": bool(job.get("anomaly")),
              "service_focus": (freeform.get("serviceGuide") == 1 and bool(freeform.get("guideFocus", True))) if source["source_contract"] == "recognized" else None,
              "power_fault": bool(freeform.get("powerFault")),
              "next_action_basis": "Reviewed source plus saved stage; backup power, live encoders, physical access and load normalization are not simulated."}
    if source["source_contract"] != "recognized":
        result["next_action"] = None
        result["interpretation"] = "Relevant source changed. Review and update the contract before interpreting this save."
        return result
    result["next_action"] = job_next(job, STAGES[stage])
    result["receiver_hint_when_powered"] = result["next_action"] if stage >= 3 else "Requires live pointing/tuning/settling assessment."
    result["hint_conflict"] = False
    result["power_policy"] = ("Original chart pickup and packet filing do not require power."
                              if stage in (3, 6) else "Receiver next action allows Filed; HUD still prioritizes unresolved outage."
                              if stage == 7 else "Restore feeder or start fuelled backup if power is unavailable.")
    if result["power_fault"] and stage not in (3, 6, 7):
        result["next_action"] = "If backup is not supplying power: restore feeder or start backup. Otherwise: " + result["next_action"]
    result["objective_override"] = "Load a saved case." if freeform.get("dead") else "Service guide may override HUD while focused; station/job guidance remains stage-specific." if result["service_focus"] else None
    def waiting(candidate):
        return isinstance(candidate, dict) and (candidate.get("stage") == 3 or
            (candidate.get("stage") == 5 and candidate.get("anomaly")) or
            (candidate.get("stage") == 4 and candidate.get("phenomenon") == "duplicate_print" and
             candidate.get("control") and isinstance(candidate.get("controlAt", 0), (int, float)) and
             candidate.get("controlAt", 0) >= 8 and not candidate.get("duplicateCollected")))
    pending = [i for i, candidate in enumerate(jobs) if waiting(candidate)]
    target = selected if selected in pending else next(iter(pending), None)
    blocked = target is not None and target != selected and (stage == 2 or (job.get("control") and job.get("controlAt", 0) < 8))
    result["printer_queue"] = {"waiting_count": len(pending), "target_saved_index": target,
        "selection_blocked_by_active_work": blocked,
        "action": "Finish active recording/control before printer can select waiting work." if blocked else
                  "Collect queued original without power." if target is not None and jobs[target].get("stage") == 3 else
                  "Collect queued packet or duplicate with power; retained-original prerequisites still apply." if target is not None else
                  "No queued paper; printer failure feedback follows current NextAction.",
        "basis": "Saved valid job order only; not full loader normalization or duplicate-document validation."}
    result["interpretation"] = "Source/save observation, not a runtime reproduction. Installed executable revision has not been checked."
    return result


def read_findings(paths, scenarios):
    findings, seen = [], set()
    for path in paths:
        batch = json.loads(Path(path).read_text())
        if not isinstance(batch, list):
            raise ValueError("Finding input must be a JSON array")
        for finding in batch:
            if not isinstance(finding, dict) or any(k not in finding for k in REQUIRED_FINDING):
                raise ValueError("Finding missing required schema fields")
            for key in ("id", "priority", "owner", "scenario", "confidence", "trigger", "consequence", "fix", "acceptance"):
                if not isinstance(finding[key], str) or not finding[key].strip():
                    raise ValueError("Finding field must be nonempty text: " + key)
            if finding["id"] in seen:
                raise ValueError("Duplicate finding id: " + finding["id"])
            if finding["priority"] not in ("P1", "P2", "P3") or finding["confidence"] not in CONFIDENCE:
                raise ValueError("Invalid priority or confidence")
            if finding["scenario"] not in scenarios:
                raise ValueError("Unknown scenario: " + str(finding["scenario"]))
            evidence = finding["evidence"]
            if not isinstance(evidence, list) or not evidence or any(not isinstance(e, str) or not e.strip() for e in evidence):
                raise ValueError("Finding must cite at least one evidence reference")
            # Whitelist fields: reports must not accidentally carry full save data.
            findings.append({key: finding[key] for key in REQUIRED_FINDING})
            seen.add(finding["id"])
    return sorted(findings, key=lambda f: (f["priority"], f["id"]))


def markdown(report):
    lines = ["# Gameplay review context", "", "Static/read-only collection. Runtime: NOT RUN. Human playtest: NOT RUN.", "",
             f"Source: `{report['source']['revision']}`; dirty: {report['source']['dirty']}; contract: {report['source']['source_contract']}.", ""]
    if report.get("save"):
        lines += ["## Selected job", "", "```json", json.dumps(report["save"], ensure_ascii=False, indent=2), "```", ""]
    lines += ["## Scenarios", ""]
    for scenario in report["scenarios"]:
        lines.append(f"- {scenario['id']}: {scenario['title']} ({scenario['owner']}); runtime NOT RUN / human NOT RUN.")
    lines += ["", "## Agent findings", "", "Agent confidence is reported as submitted and requires coordinator adjudication.", ""]
    for f in report["findings"]:
        lines += [f"### {f['id']} / {f['priority']} / {f['confidence']}", ""]
        lines += [f"{key}: {f[key]}" for key in ("owner", "scenario", "trigger", "consequence", "fix", "acceptance")]
        lines += ["Evidence: " + "; ".join(f["evidence"]), ""]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--save", type=Path, help="Optional existing JSON save; only read")
    parser.add_argument("--findings", type=Path, nargs="*", default=[])
    parser.add_argument("--format", choices=("json", "markdown"), default="json")
    args = parser.parse_args()
    try:
        plan = json.loads((HERE / "scenarios.json").read_text())
        source = source_context(args.repo)
        report = {"schema_version": 1, "runtime": "NOT_RUN", "human_playtest": "NOT_RUN", "source": source,
                  "scenarios": plan["scenarios"], "agent_roles": plan["agent_roles"],
                  "findings": read_findings(args.findings, {s["id"] for s in plan["scenarios"]})}
        if args.save:
            report["save"] = save_summary(json.loads(args.save.read_text()), source)
        print(json.dumps(report, ensure_ascii=False, indent=2) if args.format == "json" else markdown(report))
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        parser.exit(2, "audit: " + str(exc) + "\n")


if __name__ == "__main__":
    main()
