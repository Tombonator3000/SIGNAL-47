"""Validated job contracts. Model output never supplies executable commands."""
from __future__ import annotations

from dataclasses import asdict, dataclass, field
import hashlib
import json
import math
from pathlib import PurePosixPath
import re


class ProductionError(RuntimeError):
    """An actionable configuration, execution or evidence error."""


def relative_path(value: str, *, directory: bool = False) -> str:
    if not isinstance(value, str) or not value or "\\" in value or any(ord(ch) < 32 or ord(ch) == 127 for ch in value):
        raise ProductionError("Expected a nonempty relative POSIX path")
    path = PurePosixPath(value)
    if path.is_absolute() or any(part in ("", ".", "..") for part in value.rstrip("/").split("/")):
        raise ProductionError(f"Unsafe relative path: {value!r}")
    if any(part in (".git", ".claude", ".codex", "Saved", "Licensed", "node_modules") for part in path.parts):
        raise ProductionError(f"Protected path: {value}")
    if not directory and value.endswith("/"):
        raise ProductionError("A file path cannot end in /")
    normalized = str(path) + ("/" if directory and value.endswith("/") else "")
    if normalized != value:
        raise ProductionError("Paths must be normalized: " + value)
    return normalized


def positive_number(value, name: str, maximum: float) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not 0 < value <= maximum or not math.isfinite(value):
        raise ProductionError(f"{name} must be positive, finite and <= {maximum:g}")
    return float(value)


@dataclass(frozen=True)
class Task:
    id: str
    title: str
    description: str
    kind: str = "report"
    role: str = "producer"
    schema_version: int = 1
    depends_on: list[str] = field(default_factory=list)
    allowed_paths: list[str] = field(default_factory=list)
    context_query: str = ""
    checks: list[dict] = field(default_factory=list)
    acceptance: list[str] = field(default_factory=list)
    artifacts: list[str] = field(default_factory=list)
    requires_review: bool = True
    max_attempts: int = 2
    budget_usd: float = 2.0
    timeout_seconds: float = 600.0

    @classmethod
    def from_dict(cls, data: dict) -> "Task":
        if not isinstance(data, dict):
            raise ProductionError("Task must be a JSON object")
        unknown = set(data) - set(cls.__dataclass_fields__)
        if unknown:
            raise ProductionError("Unknown task fields: " + ", ".join(sorted(unknown)))
        try:
            task = cls(**data)
        except TypeError as exc:
            raise ProductionError(str(exc)) from exc
        if type(task.schema_version) is not int or task.schema_version != 1:
            raise ProductionError("Unsupported task schema_version")
        if not isinstance(task.id, str) or not re.fullmatch(r"[A-Z][A-Z0-9_-]{0,63}", task.id):
            raise ProductionError("Task id must use uppercase letters, digits, _ or -")
        for name in ("title", "description"):
            value = getattr(task, name)
            if not isinstance(value, str) or not value.strip() or len(value) > 24000:
                raise ProductionError(f"{name} must contain 1..24000 characters")
        if task.kind not in ("command", "report", "edit"):
            raise ProductionError("kind must be command, report or edit")
        if task.role not in ("producer", "graphics", "gameplay", "qa"):
            raise ProductionError("role must be producer, graphics, gameplay or qa")
        if not isinstance(task.context_query, str) or len(task.context_query) > 4000:
            raise ProductionError("context_query must be a string <=4000 characters")
        for name in ("depends_on", "allowed_paths", "acceptance", "artifacts"):
            values = getattr(task, name)
            if not isinstance(values, list) or len(values) > 100 or not all(isinstance(x, str) and x for x in values):
                raise ProductionError(f"{name} must be a list of at most 100 nonempty strings")
            if len(values) != len(set(values)):
                raise ProductionError(f"Duplicate {name}")
        if task.id in task.depends_on or any(not re.fullmatch(r"[A-Z][A-Z0-9_-]{0,63}", x) for x in task.depends_on):
            raise ProductionError("Invalid dependency")
        for value in task.allowed_paths:
            relative_path(value, directory=True)
        for value in task.artifacts:
            relative_path(value)
        if task.kind == "edit" and not task.allowed_paths:
            raise ProductionError("An edit task needs explicit allowed_paths")
        if task.kind != "edit" and task.allowed_paths:
            raise ProductionError("Only edit tasks may declare writable paths")
        if type(task.requires_review) is not bool:
            raise ProductionError("requires_review must be boolean")
        if type(task.max_attempts) is not int or not 1 <= task.max_attempts <= 5:
            raise ProductionError("max_attempts must be between 1 and 5")
        positive_number(task.budget_usd, "budget_usd", 100)
        positive_number(task.timeout_seconds, "timeout_seconds", 14400)
        if not isinstance(task.checks, list) or not 1 <= len(task.checks) <= 30:
            raise ProductionError("Every task needs 1..30 independent automated checks")
        for check in task.checks:
            if not isinstance(check, dict) or set(check) - {"tool", "params", "timeout_seconds"}:
                raise ProductionError("Invalid check fields")
            if not isinstance(check.get("tool"), str) or not check["tool"].strip() or not isinstance(check.get("params", {}), dict):
                raise ProductionError("Checks need tool and object params")
            positive_number(check.get("timeout_seconds", 120), "check timeout", 14400)
        try:
            json.dumps(task.to_dict(), allow_nan=False)
        except (TypeError, ValueError, OverflowError) as exc:
            raise ProductionError("Task values must be finite JSON data") from exc
        return task

    def to_dict(self) -> dict:
        return asdict(self)

    def fingerprint(self) -> str:
        return hashlib.sha256(json.dumps(self.to_dict(), sort_keys=True).encode()).hexdigest()

    def allows(self, path: str) -> bool:
        path = relative_path(path)
        return any(path == scope.rstrip("/") or scope.endswith("/") and path.startswith(scope)
                   for scope in self.allowed_paths)
