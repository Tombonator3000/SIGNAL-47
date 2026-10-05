#!/usr/bin/env python3
"""Start the local pinned Ghidra toolchain or import the research executable.

Import only: no automatic analysis, no game execution, no overwrite of projects.
All projects, settings, cache, logs, and import receipts stay under private/.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import subprocess
import sys

from pak_inspect import sha256_file

ROOT = Path(__file__).resolve().parent.parent
PROJECT = "VotV_0_9_0n"
SHIPPING = "VotV/Binaries/Win64/VotV-Win64-Shipping.exe"


def configuration() -> tuple[dict, dict, Path, Path, Path]:
    pins = json.loads((ROOT / "toolchain-pins.json").read_text())
    release = json.loads((ROOT / "release.json").read_text())
    private = (ROOT / "private").resolve()
    ghidra = private / "tools" / pins["ghidra"]["directory"]
    java = private / "tools" / pins["java"]["directory"]
    if not (ghidra / "support" / "analyzeHeadless").is_file() or not (java / "bin" / "java").is_file():
        raise ValueError("Pinned portable Ghidra/JDK not found; see README setup instructions")
    properties = (ghidra / "Ghidra" / "application.properties").read_text()
    if f"application.version={pins['ghidra']['version']}" not in properties:
        raise ValueError("Ghidra installation does not match the pinned version")
    if 'JAVA_VERSION="21.0.12.1"' not in (java / "release").read_text():
        raise ValueError("JDK installation does not match the pinned version")
    return pins, release, private, ghidra, java


def environment(private: Path, java: Path) -> dict[str, str]:
    env = os.environ.copy()
    data = private / "tools" / "data"
    for name in ("home", "settings", "cache", "tmp"):
        (data / name).mkdir(parents=True, exist_ok=True)
    env["JAVA_HOME"] = str(java)
    # Paths passed to the JVM through JAVA_TOOL_OPTIONS may contain spaces.
    properties = {
        "user.home": data / "home", "application.settingsdir": data / "settings",
        "application.cachedir": data / "cache", "application.tempdir": data / "tmp",
        "java.io.tmpdir": data / "tmp",
    }
    env["JAVA_TOOL_OPTIONS"] = " ".join(f'-D{key}="{value}"' for key, value in properties.items()) + " -XX:ActiveProcessorCount=2"
    env["XDG_CONFIG_HOME"] = str(data / "settings")
    env["XDG_CACHE_HOME"] = str(data / "cache")
    env["TMPDIR"] = str(data / "tmp")
    env["GHIDRA_HEADLESS_MAXMEM"] = "2G"
    env["GHIDRA_HEADLESS_JAVA_OPTIONS"] = "-Dcpu.core.limit=2"
    env["GHIDRA_MAXMEM"] = "2G"
    env["GHIDRA_GUI_MAXMEM"] = "2G"
    env["GHIDRA_JAVA_OPTIONS"] = "-Dcpu.core.limit=2"
    return env


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("import", "gui", "status"))
    args = parser.parse_args()
    try:
        pins, release, private, ghidra, java = configuration()
        projects = private / "projects"
        project_file = projects / f"{PROJECT}.gpr"
        receipt_file = private / "reports" / "ghidra-import.json"
        if args.action == "status":
            status = {"ghidra": pins["ghidra"]["version"], "java": pins["java"]["version"],
                      "project": str(project_file), "project_exists": project_file.is_file()}
            if receipt_file.is_file():
                status["last_import"] = json.loads(receipt_file.read_text())
            print(json.dumps(status, indent=2))
            return 0
        env = environment(private, java)
        if args.action == "gui":
            print(f"Open project in Ghidra: {project_file}", flush=True)
            # Opening the GUI is explicit. This command is not used by preparation.
            return subprocess.run([str(ghidra / "ghidraRun")], env=env).returncode
        if project_file.exists() or (projects / f"{PROJECT}.rep").exists():
            raise ValueError("Research project already exists; use status/gui. Import never overwrites it")
        preparation = json.loads((private / "reports" / "preparation.json").read_text())
        if preparation.get("release") != release["version"] or preparation.get("all_copy_hashes_match_source") is not True:
            raise ValueError("Run prepare.py successfully before importing")
        executable = private / "game" / release["version"] / "WindowsNoEditor" / SHIPPING
        if sha256_file(executable) != release["files"][SHIPPING]["sha256"]:
            raise ValueError("Research executable checksum differs from the pinned release")
        projects.mkdir(parents=True, exist_ok=True)
        reports = private / "reports"
        log = reports / "ghidra-import.log"
        command = [str(ghidra / "support" / "analyzeHeadless"), str(projects), PROJECT,
                   "-import", str(executable), "-noanalysis", "-max-cpu", "2",
                   "-log", str(reports / "ghidra-application.log")]
        print("Importing native executable without automatic analysis; log: " + str(log), flush=True)
        with log.open("w") as output:
            run = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT)
        text = log.read_text(errors="replace")
        success = run.returncode == 0 and project_file.is_file() and "Import succeeded" in text
        receipt = {
            "checked_at_utc": datetime.now(timezone.utc).isoformat(),
            "exit_code": run.returncode, "import_pass": success,
            "project": str(project_file), "program": executable.name,
            "program_sha256": release["files"][SHIPPING]["sha256"],
            "ghidra_version": pins["ghidra"]["version"],
            "java_version": pins["java"]["version"],
            "max_heap": "2G", "max_cpu": 2, "automatic_analysis": False,
            "decompilation_done": False, "log": str(log),
        }
        receipt_file.write_text(json.dumps(receipt, indent=2) + "\n")
        print(json.dumps(receipt, indent=2))
        return 0 if success else 1
    except (OSError, ValueError) as error:
        print(f"Ghidra preparation stopped: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
