"""Production adapter assertions using synthetic assets and captured commands."""

import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from Automation.production.models import ProductionError
from Automation.production.tools import execute_check, validate_check


class ToolTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.repo = self.directory / "repo"
        self.repo.mkdir()
        self.run_dir = self.directory / "run"
        self.run_dir.mkdir()
        self.commands = []

    def put(self, name, content):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(content, bytes):
            path.write_bytes(content)
        else:
            path.write_text(content, encoding="utf-8")
        return path

    def execute(self, tool, params, index=1):
        return execute_check({"tool": tool, "params": params}, root=self.repo,
                             run_dir=self.run_dir, index=index, env=os.environ.copy())

    def fake_command(self, command, **arguments):
        self.commands.append((command, arguments))
        log = arguments["log_path"]
        log.write_text("synthetic command completion, no engine executed\n")
        return {"command": command, "returncode": 0, "log": str(log), "log_sha256": hashlib.sha256(log.read_bytes()).hexdigest()}

    def graphics_fixture(self, metadata, index=1):
        self.put("unreal/Signal47/Scripts/graphics-review.py", "# command fixture, never executed\n")
        path = self.run_dir / f"graphics-{index}" / "meta.json"
        path.parent.mkdir(parents=True)
        path.write_text(json.dumps(metadata))
        return path

    def test_python_compile_rejects_top_level_return_and_never_executes_code(self):
        self.put("Automation/invalid.py", "return 1\n")
        with self.assertRaises(SyntaxError):
            self.execute("python_compile", {"paths": ["Automation/invalid.py"]})
        marker = self.directory / "must-not-execute"
        self.put("Automation/valid.py", "from pathlib import Path\nPath(" + repr(str(marker)) + ").write_text('executed')\n")
        self.assertTrue(self.execute("python_compile", {"paths": ["Automation/valid.py"]})["passed"])
        self.assertFalse(marker.exists())

    def test_json_assert_distinguishes_boolean_from_number_at_top_level(self):
        self.put("data.json", '{"value": 1}')
        with self.assertRaises(ProductionError):
            self.execute("json_assert", {"path": "data.json", "equals": {"value": True}})
        self.assertTrue(self.execute("json_assert", {"path": "data.json", "equals": {"value": 1}})["passed"])

    def test_json_assert_distinguishes_nested_boolean_from_number(self):
        fixtures = [
            ({"flag": 1}, {"flag": True}),
            ({"flag": False}, {"flag": 0}),
            ([1, {"flag": 0}], [True, {"flag": False}]),
            ({"deep": [{"flag": 1}]}, {"deep": [{"flag": True}]}),
        ]
        for actual, expected in fixtures:
            self.put("data.json", json.dumps({"value": actual}))
            with self.subTest(actual=actual, expected=expected), self.assertRaises(ProductionError):
                self.execute("json_assert", {"path": "data.json", "equals": {"value": expected}})
        self.put("data.json", json.dumps({"value": {"nested": [True, {"flag": 1}]}}))
        result = self.execute("json_assert", {"path": "data.json", "equals": {"value": {"nested": [True, {"flag": 1}]}}})
        self.assertTrue(result["passed"])

    def test_json_assert_checks_dotted_paths_and_hashes_the_actual_source(self):
        path = self.put("data.json", '{"value": {"nested": {"flag": true}}}')
        result = self.execute("json_assert", {"path": "data.json", "equals": {"value.nested.flag": True}})
        self.assertEqual(result["sha256"], hashlib.sha256(path.read_bytes()).hexdigest())

    def test_graphics_diagnostic_pass_is_not_valid_evidence(self):
        self.graphics_fixture({"status": "PASS", "valid_evidence": False})
        with patch("Automation.production.tools.run_command", side_effect=self.fake_command):
            with self.assertRaisesRegex(ProductionError, "evidence"):
                self.execute("graphics_review", {"build": "/synthetic/build", "views": ["entry"]})

    def test_graphics_requires_true_boolean_and_pass_status(self):
        for index, metadata in enumerate((
            {"status": "PASS", "valid_evidence": 1},
            {"status": "FAIL", "valid_evidence": True},
            {"status": "PASS"},
        ), 1):
            self.graphics_fixture(metadata, index=index)
            with self.subTest(metadata=metadata), patch("Automation.production.tools.run_command", side_effect=self.fake_command):
                with self.assertRaises(ProductionError):
                    self.execute("graphics_review", {"build": "/synthetic/build", "views": ["entry"]}, index=index)

    def test_package_then_graphics_uses_the_same_expanded_candidate_path(self):
        self.put("unreal/Signal47/Scripts/package-linux.sh", "# synthetic fixture, never executed\n")
        binary = self.run_dir / "package/Linux/Signal47/Binaries/Linux/Signal47"
        binary.parent.mkdir(parents=True)
        binary.write_bytes(b"synthetic packaged binary for adapter testing")
        meta = self.graphics_fixture({"status": "PASS", "valid_evidence": True}, index=2)
        with patch("Automation.production.tools.run_command", side_effect=self.fake_command):
            packaged = self.execute("package_linux", {})
            reviewed = self.execute("graphics_review", {
                "build": "{run_dir}/package", "views": ["entry", "office"], "quality": "low",
            }, index=2)
        package_command, graphics_command = [value[0] for value in self.commands]
        self.assertEqual(package_command[-1], str(self.run_dir / "package"))
        self.assertEqual(graphics_command[graphics_command.index("--build") + 1], package_command[-1])
        self.assertEqual(graphics_command[graphics_command.index("--out") + 1], str(self.run_dir / "graphics-2"))
        self.assertEqual(graphics_command[-3:], ["--views", "entry", "office"])
        self.assertEqual(packaged["binary_sha256"], hashlib.sha256(binary.read_bytes()).hexdigest())
        self.assertEqual(reviewed["evidence_sha256"], hashlib.sha256(meta.read_bytes()).hexdigest())

    def test_packaging_success_without_a_real_candidate_cannot_pass(self):
        self.put("unreal/Signal47/Scripts/package-linux.sh", "# fixture\n")
        with patch("Automation.production.tools.run_command", side_effect=self.fake_command):
            with self.assertRaisesRegex(ProductionError, "packaged executable"):
                self.execute("package_linux", {})

    def test_full_matrix_argument_order_is_outdir_then_build(self):
        script = self.put("unreal/Signal47/Scripts/screenshots.sh", "# fixture\n")
        with patch("Automation.production.tools.run_command", side_effect=self.fake_command):
            self.execute("full_matrix", {"build": "{run_dir}/package"})
        self.assertEqual(self.commands[0][0], [
            "bash", str(script), str(self.run_dir / "full-matrix"), str(self.run_dir / "package"),
        ])

    def test_empty_unittest_suite_cannot_pass(self):
        (self.repo / "tests").mkdir()
        # Python versions differ: discovery may exit 5 or exit 0 with Ran 0.
        with self.assertRaises(ProductionError):
            self.execute("unittest", {"start": "tests"})

    def test_zero_tests_with_success_exit_is_still_rejected(self):
        (self.repo / "tests").mkdir()
        def empty_success(command, **arguments):
            result = self.fake_command(command, **arguments)
            arguments["log_path"].write_text("Ran 0 tests in 0.000s\n\nOK\n")
            return result
        with patch("Automation.production.tools.run_command", side_effect=empty_success):
            with self.assertRaisesRegex(ProductionError, "nonempty test suite"):
                self.execute("unittest", {"start": "tests"})

    def test_actual_nonempty_unittest_suite_can_pass(self):
        self.put("tests/test_fixture.py", "import unittest\nclass Fixture(unittest.TestCase):\n    def test_actual(self):\n        self.assertEqual(2 + 2, 4)\n")
        result = self.execute("unittest", {"start": "tests", "pattern": "test_fixture.py"})
        self.assertTrue(result["passed"])
        self.assertEqual(result["tests_run"], 1)

    def test_files_reject_empty_and_unmaterialized_lfs_assets(self):
        for name, content in (("empty.txt", ""), ("pointer.bin", "version https://git-lfs.github.com/spec/v1\noid sha256:123\nsize 42\n")):
            self.put(name, content)
            with self.subTest(name=name), self.assertRaises(ProductionError):
                self.execute("files", {"paths": [name]})
        path = self.put("materialized.bin", b"actual fixture bytes")
        result = self.execute("files", {"paths": ["materialized.bin"]})
        self.assertEqual(result["files"][0]["sha256"], hashlib.sha256(path.read_bytes()).hexdigest())

    def test_validation_rejects_unknown_tools_parameters_and_unsafe_placeholders(self):
        bad = [
            {"tool": "shell", "params": {}},
            {"tool": "files", "params": {"paths": ["data.json"], "command": "echo unsafe"}},
            {"tool": "graphics_review", "params": {"build": "relative/build", "views": ["entry"]}},
            {"tool": "graphics_review", "params": {"build": "{run_dir}/../other", "views": ["entry"]}},
            {"tool": "graphics_review", "params": {"build": "/build", "views": ["--help"]}},
            {"tool": "asset_script", "params": {"script": "../other.py", "args": []}},
            {"tool": "asset_script", "params": {"script": "Automation/fixture.py", "args": "not-a-list"}},
            {"tool": "unittest", "params": {"start": "tests", "pattern": "../test*.py"}},
        ]
        for check in bad:
            with self.subTest(check=check), self.assertRaises(ProductionError):
                validate_check(check)


if __name__ == "__main__":
    unittest.main()
