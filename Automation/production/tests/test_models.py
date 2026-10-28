"""Validate production contracts before a worker can claim or edit anything."""

import math
import unittest

from Automation.production.models import ProductionError, Task, positive_number, relative_path


def contract(**changes):
    data = {
        "id": "PROD-01",
        "title": "Check a production source",
        "description": "Run an independent source check and preserve its evidence.",
        "checks": [{"tool": "source_check", "params": {}}],
    }
    data.update(changes)
    return data


class ModelTests(unittest.TestCase):
    def test_valid_contract_round_trip_and_stable_fingerprint(self):
        first = Task.from_dict(contract())
        second = Task.from_dict(dict(reversed(list(first.to_dict().items()))))
        self.assertEqual(first.to_dict(), second.to_dict())
        self.assertEqual(first.fingerprint(), second.fingerprint())
        self.assertEqual(len(first.fingerprint()), 64)
        self.assertNotEqual(first.fingerprint(), Task.from_dict(contract(description="Another requirement")).fingerprint())

    def test_schema_requires_known_fields_and_supported_version(self):
        for value in (None, [], "task", {"id": "A"}, contract(unexpected="command")):
            with self.subTest(value=value), self.assertRaises(ProductionError):
                Task.from_dict(value)
        for version in (0, 2, True, "1", None):
            with self.subTest(version=version), self.assertRaises(ProductionError):
                Task.from_dict(contract(schema_version=version))

    def test_ids_titles_descriptions_and_kinds_are_bounded(self):
        for task_id in ("", "lowercase", "A/B", "A B", "1TASK", "A" * 65, None):
            with self.subTest(task_id=task_id), self.assertRaises(ProductionError):
                Task.from_dict(contract(id=task_id))
        for field in ("title", "description"):
            for value in ("", "   ", "x" * 24001, None, 42):
                with self.subTest(field=field, value=repr(value)[:50]), self.assertRaises(ProductionError):
                    Task.from_dict(contract(**{field: value}))
        for kind in ("shell", "", None):
            with self.subTest(kind=kind), self.assertRaises(ProductionError):
                Task.from_dict(contract(kind=kind))
        for kind in ("command", "report"):
            self.assertEqual(Task.from_dict(contract(kind=kind)).kind, kind)

    def test_lists_and_dependencies_reject_duplicates_invalid_ids_and_self(self):
        for field in ("depends_on", "allowed_paths", "acceptance", "artifacts"):
            for value in (None, "A", [""], [1], ["A", "A"], [str(index) for index in range(101)]):
                with self.subTest(field=field, value=value), self.assertRaises(ProductionError):
                    Task.from_dict(contract(**{field: value}))
        for dependencies in (["PROD-01"], ["lowercase"], ["../OTHER"], ["1BAD"]):
            with self.subTest(dependencies=dependencies), self.assertRaises(ProductionError):
                Task.from_dict(contract(depends_on=dependencies))

    def test_edit_permissions_are_explicit_and_match_path_boundaries(self):
        with self.assertRaises(ProductionError):
            Task.from_dict(contract(kind="edit"))
        with self.assertRaises(ProductionError):
            Task.from_dict(contract(kind="report", allowed_paths=["Docs/Active/"]))
        task = Task.from_dict(contract(
            kind="edit", allowed_paths=["Docs/Active/generated/", "unreal/Signal47/Scripts/exact.py"],
        ))
        self.assertTrue(task.allows("Docs/Active/generated/report.md"))
        self.assertTrue(task.allows("Docs/Active/generated/nested/data.json"))
        self.assertTrue(task.allows("unreal/Signal47/Scripts/exact.py"))
        self.assertFalse(task.allows("Docs/Active/generated-other/report.md"))
        self.assertFalse(task.allows("unreal/Signal47/Scripts/exact.py.bak"))
        self.assertFalse(task.allows("Docs/Active/STATUS.md"))
        with self.assertRaises(ProductionError):
            task.allows("Docs/Active/generated/../../STATUS.md")

    def test_paths_reject_escape_and_protected_targets(self):
        unsafe = [
            "", "/tmp/outside", "../outside", "Docs/../outside", "./Docs/file",
            "Docs//file", "Docs/./file", "Docs\\file", "Docs/file\x00.py", ".",
            ".git/config", "nested/.git/config", ".claude/settings.json",
            ".codex/settings.toml", "unreal/Saved/data.json", "unreal/Licensed/file",
            "nested/node_modules/package.json",
        ]
        for path in unsafe:
            with self.subTest(path=path), self.assertRaises(ProductionError):
                relative_path(path)
            with self.subTest(artifact=path), self.assertRaises(ProductionError):
                Task.from_dict(contract(artifacts=[path]))
        with self.assertRaises(ProductionError):
            relative_path("Docs/file/")
        self.assertEqual(relative_path("Docs/Active/", directory=True), "Docs/Active/")
        self.assertEqual(relative_path("Docs/Active/file.md"), "Docs/Active/file.md")

    def test_budget_timeout_review_and_attempt_values_are_strict(self):
        for name, maximum in (("budget_usd", 100), ("timeout_seconds", 14400)):
            for value in (-1, 0, True, None, "2", math.nan, math.inf, maximum + 1, 10**1000):
                with self.subTest(name=name, value=value), self.assertRaises(ProductionError):
                    Task.from_dict(contract(**{name: value}))
        for attempts in (0, 6, True, 1.5, "2", None):
            with self.subTest(attempts=attempts), self.assertRaises(ProductionError):
                Task.from_dict(contract(max_attempts=attempts))
        for review in (None, "false", 0, 1):
            with self.subTest(review=review), self.assertRaises(ProductionError):
                Task.from_dict(contract(requires_review=review))
        self.assertEqual(positive_number(1, "value", 1), 1.0)

    def test_checks_need_independent_named_tools_and_object_parameters(self):
        bad_checks = (
            [], None, "source_check", [{}], [{"tool": ""}], [{"tool": 1}],
            [{"tool": "source_check", "params": []}],
            [{"tool": "source_check", "command": "rm -rf /"}],
            [{"tool": "source_check", "timeout_seconds": 0}],
            [{"tool": "source_check", "timeout_seconds": math.inf}],
            [{"tool": "source_check"}] * 31,
        )
        for checks in bad_checks:
            with self.subTest(checks=checks), self.assertRaises(ProductionError):
                Task.from_dict(contract(checks=checks))
        task = Task.from_dict(contract(checks=[{"tool": "source_check"}]))
        self.assertEqual(len(task.checks), 1)

    def test_context_query_is_text_and_bounded(self):
        for query in (None, [], 10, "x" * 4001):
            with self.subTest(query=repr(query)[:40]), self.assertRaises(ProductionError):
                Task.from_dict(contract(context_query=query))


if __name__ == "__main__":
    unittest.main()
