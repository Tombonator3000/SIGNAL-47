"""Named adapters for the existing production tools, with no shell interpolation."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sys

from .models import ProductionError, Task, relative_path
from .process import run_command


SCRIPTS = "unreal/Signal47/Scripts"
TOOLS = {
    "files": False, "json_assert": False, "task_contract": False, "python_compile": False,
    "unittest": False, "git_diff": False,
    "build_editor": True, "package_linux": True, "graphics_review": True,
    "anomaly_pilot": True, "full_matrix": True, "blender_script": True,
    "asset_script": True,
}


def _json_equal(actual, expected) -> bool:
    if type(actual) is not type(expected):
        return False
    if isinstance(actual, dict):
        return actual.keys() == expected.keys() and all(_json_equal(actual[key], expected[key]) for key in actual)
    if isinstance(actual, list):
        return len(actual) == len(expected) and all(_json_equal(a, b) for a, b in zip(actual, expected))
    return actual == expected


def repo_path(root: Path, value: str, *, must_exist=True) -> Path:
    value = relative_path(value)
    root = root.resolve()
    path = root / value
    cursor = path
    while cursor != root:
        if cursor.is_symlink():
            raise ProductionError("Symlinked source/output is not allowed: " + value)
        cursor = cursor.parent
    if must_exist and not path.is_file():
        raise ProductionError("Missing file: " + value)
    return path


def load_script(root: Path, name: str):
    path = repo_path(root, SCRIPTS + "/" + name + ".py")
    spec = importlib.util.spec_from_file_location("_s47_production_" + name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def validate_check(check: dict):
    tool = check["tool"]
    if tool not in TOOLS:
        raise ProductionError("Unknown tool: " + tool)
    params = check.get("params", {})
    allowed = {
        "files": {"paths"}, "json_assert": {"path", "equals"},
        "task_contract": {"path"}, "python_compile": {"paths"}, "unittest": {"start", "pattern"},
        "git_diff": set(), "build_editor": set(), "package_linux": set(),
        "graphics_review": {"build", "views", "clock", "quality", "picture", "timeline"},
        "anomaly_pilot": {"build", "scenario", "quality", "picture"},
        "full_matrix": {"build"}, "blender_script": {"script", "args"},
        "asset_script": {"script", "args"},
    }[tool]
    if set(params) - allowed:
        raise ProductionError("Unknown parameters for " + tool)
    if tool in ("files", "python_compile"):
        if not isinstance(params.get("paths"), list) or not params["paths"]:
            raise ProductionError(tool + " needs a nonempty paths list")
        for path in params["paths"]:
            relative_path(path)
    if tool == "json_assert":
        relative_path(params.get("path", ""))
        if not isinstance(params.get("equals"), dict) or not params["equals"]:
            raise ProductionError("json_assert needs nonempty equals assertions")
    if tool == "task_contract":
        relative_path(params.get("path", ""))
    if tool == "unittest":
        relative_path(params.get("start", ""))
        pattern = params.get("pattern", "test*.py")
        if not isinstance(pattern, str) or "/" in pattern or "\\" in pattern or not pattern.endswith(".py"):
            raise ProductionError("Invalid unittest pattern")
    if tool in ("asset_script", "blender_script"):
        path = relative_path(params.get("script", ""))
        if not path.endswith(".py") or not path.startswith(("unreal/Signal47/Tools/", "Automation/")):
            raise ProductionError("Asset scripts must be repository Python production scripts")
        _arguments(params.get("args", []))
    if tool in ("graphics_review", "anomaly_pilot", "full_matrix"):
        build = params.get("build")
        if not isinstance(build, str) or not (Path(build).is_absolute() or build.startswith("{run_dir}/")):
            raise ProductionError("A packaged build needs an absolute path or {run_dir}/package")
        if build.startswith("{run_dir}/"):
            relative_path(build[len("{run_dir}/"):])
    if tool == "graphics_review":
        if not isinstance(params.get("views"), list) or not params["views"] or not all(isinstance(x, str) and x and not x.startswith("-") for x in params["views"]):
            raise ProductionError("Graphics review needs explicit views, not an implicit whole-world suite")
    for name, choices in (("quality", ("low", "high", "ultra")),
                          ("picture", ("off", "vhs", "carpenter", "16mm", "super8")),
                          ("timeline", ("legacy", "evening"))):
        if name in params and params[name] not in choices:
            raise ProductionError("Invalid " + name)
    if tool == "anomaly_pilot" and params.get("picture", "vhs") not in ("vhs", "off"):
        raise ProductionError("anomaly_pilot supports only vhs or off")
    for name in ("clock", "scenario"):
        if name in params and (not isinstance(params[name], str) or not params[name] or params[name].startswith("-")):
            raise ProductionError("Invalid " + name)


def _arguments(values) -> list[str]:
    if not isinstance(values, list) or len(values) > 80 or not all(isinstance(x, str) and "\x00" not in x and len(x) <= 4000 for x in values):
        raise ProductionError("Tool args must be a bounded list of strings")
    return values


def execute_check(check: dict, *, root: Path, run_dir: Path, index: int,
                  env: dict, heartbeat=None, guard=None) -> dict:
    validate_check(check)
    tool, params = check["tool"], check.get("params", {})
    result = {"tool": tool, "params": params, "passed": False}
    files = []
    if tool == "files":
        for name in params["paths"]:
            path = repo_path(root, name)
            size = path.stat().st_size
            with path.open("rb") as stream:
                first = stream.read(150)
            if not size or first.startswith(b"version https://git-lfs.github.com/spec/v1"):
                raise ProductionError("Empty or unmaterialized LFS file: " + name)
            files.append({"path": name, "bytes": size, "sha256": _sha(path)})
        result["files"] = files
    elif tool == "task_contract":
        path = repo_path(root, params["path"])
        contract = Task.from_dict(json.loads(path.read_text()))
        for configured in contract.checks:
            validate_check(configured)
        result["task_id"] = contract.id
        result["task_fingerprint"] = contract.fingerprint()
    elif tool == "json_assert":
        path = repo_path(root, params["path"])
        value = json.loads(path.read_text())
        for dotted, expected in params["equals"].items():
            actual = value
            for part in dotted.split("."):
                actual = actual[part]
            if not _json_equal(actual, expected):
                raise ProductionError(f"JSON assertion failed at {dotted}: expected {expected!r}, found {actual!r}")
        result["sha256"] = _sha(path)
    elif tool == "python_compile":
        for name in params["paths"]:
            compile(repo_path(root, name).read_text(), name, "exec")
        result["files"] = params["paths"]
    else:
        scripts = root / SCRIPTS
        if tool == "unittest":
            start = root / relative_path(params["start"])
            if start.is_symlink() or not start.is_dir() or not start.resolve().is_relative_to(root.resolve()):
                raise ProductionError("Invalid unittest directory")
            command = [sys.executable, "-m", "unittest", "discover", "-s", str(start), "-p", params.get("pattern", "test*.py")]
        elif tool == "git_diff":
            command = ["git", "diff", "--check"]
        elif tool == "build_editor":
            command = ["bash", str(repo_path(root, SCRIPTS + "/build-editor.sh"))]
        elif tool == "package_linux":
            command = ["bash", str(repo_path(root, SCRIPTS + "/package-linux.sh")), str(run_dir / "package")]
        elif tool == "graphics_review":
            command = [sys.executable, str(repo_path(root, SCRIPTS + "/graphics-review.py")),
                       "--build", params["build"].replace("{run_dir}", str(run_dir)), "--out", str(run_dir / f"graphics-{index}"),
                       "--clock", params.get("clock", "night"), "--quality", params.get("quality", "high"),
                       "--timeline", params.get("timeline", "evening"), "--picture", params.get("picture", "carpenter"),
                       "--views", *params["views"]]
        elif tool == "anomaly_pilot":
            command = [sys.executable, str(repo_path(root, SCRIPTS + "/anomaly-pilot.py")),
                       "--build", params["build"].replace("{run_dir}", str(run_dir)), "--out", str(run_dir / f"scenario-{index}"),
                       "--scenario", params.get("scenario", "pilot"), "--quality", params.get("quality", "high"),
                       "--picture", params.get("picture", "vhs")]
        elif tool == "full_matrix":
            command = ["bash", str(repo_path(root, SCRIPTS + "/screenshots.sh")), str(run_dir / "full-matrix"), params["build"].replace("{run_dir}", str(run_dir))]
        else:
            script = repo_path(root, params["script"])
            args = [x.replace("{run_dir}", str(run_dir)).replace("{repo}", str(root))
                    for x in _arguments(params.get("args", []))]
            if tool == "blender_script":
                blender = shutil.which("blender")
                if not blender:
                    raise ProductionError("Blender is not installed")
                command = [blender, "--background", "--factory-startup", "--python-exit-code", "1", "--python", str(script), "--", *args]
            else:
                command = [sys.executable, str(script), *args]
        result.update(run_command(command, cwd=root, log_path=run_dir / f"check-{index:02d}-{tool}.log",
                                  timeout=check.get("timeout_seconds", 120), env=env,
                                  heartbeat=heartbeat, guard=guard if TOOLS[tool] else None))
        if result["returncode"]:
            raise ProductionError(f"{tool} exited with {result['returncode']}; inspect {result['log']}")
        if tool == "unittest":
            log = Path(result["log"]).read_text(errors="replace")
            import re
            count = re.search(r"Ran ([0-9]+) tests? in ", log)
            if not count or int(count.group(1)) == 0 or not re.search(r"^OK(?:\s|$)", log, re.MULTILINE):
                raise ProductionError("unittest did not execute and pass a nonempty test suite")
            result["tests_run"] = int(count.group(1))
        if tool == "graphics_review":
            meta_path = run_dir / f"graphics-{index}" / "meta.json"
            metadata = json.loads(meta_path.read_text())
            if metadata.get("status") != "PASS" or metadata.get("valid_evidence") is not True:
                raise ProductionError("Graphics diagnostic exited successfully but its evidence is not valid PASS")
            result["evidence"] = metadata
            result["evidence_sha256"] = _sha(meta_path)
        if tool == "package_linux":
            binary = run_dir / "package/Linux/Signal47/Binaries/Linux/Signal47"
            if not binary.is_file() or binary.stat().st_size == 0:
                raise ProductionError("Packaging returned success without a packaged executable")
            result["package"] = str(run_dir / "package")
            result["binary_sha256"] = _sha(binary)
        if tool == "anomaly_pilot":
            evidence_path = run_dir / f"scenario-{index}" / "results.json"
            metadata = json.loads(evidence_path.read_text())
            if not isinstance(metadata, list) or not metadata or any(item.get("status") != "PASS" for item in metadata):
                raise ProductionError("Anomaly route did not provide nonempty PASS evidence")
            result["evidence"] = metadata
            result["evidence_sha256"] = _sha(evidence_path)
    result["passed"] = True
    return result


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()
