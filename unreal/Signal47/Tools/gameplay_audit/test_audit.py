"""Small fixture checks for evidence handling, not tests of game behavior."""
import copy
import json
import tempfile
import unittest
from pathlib import Path
import audit


class AuditTests(unittest.TestCase):
    def setUp(self):
        self.source = {"source_contract": "recognized"}
        self.save = {"thumb": "SECRET_THUMB", "state": {"freeform": {
            "selected": 0, "jobs": [{"id": "FF-D0001-J3", "stage": 4, "printed": True,
                                     "paid": False, "anomaly": False, "originalPage": "SECRET_DOCUMENT"}]
        }}}

    def test_collected_routine_and_no_private_fields(self):
        original = copy.deepcopy(self.save)
        out = audit.save_summary(self.save, self.source)
        self.assertEqual(out["stage"], "Recorded")
        self.assertFalse(out["hint_conflict"])
        self.assertIn("Register routine", out["next_action"])
        self.assertNotIn("SECRET", json.dumps(out))
        self.assertEqual(self.save, original)

    def test_unknown_source_suppresses_advice(self):
        out = audit.save_summary(self.save, {"source_contract": "UNVERIFIED_SOURCE_CHANGED"})
        self.assertIsNone(out["next_action"])
        self.assertEqual(out["stage"], "UNVERIFIED")
        self.assertNotIn("hint_conflict", out)
        self.assertIsNone(out["service_focus"])

    def test_legacy_service_focus_default_and_explicit_deferral(self):
        freeform = self.save["state"]["freeform"]
        freeform["serviceGuide"] = 1
        self.assertTrue(audit.save_summary(self.save, self.source)["service_focus"])
        freeform["guideFocus"] = False
        self.assertFalse(audit.save_summary(self.save, self.source)["service_focus"])
        del freeform["guideFocus"]
        freeform["serviceGuide"] = 0
        self.assertFalse(audit.save_summary(self.save, self.source)["service_focus"])

    def test_separate_duplicate_and_filed_paths(self):
        job = self.save["state"]["freeform"]["jobs"][0]
        job.update(anomaly=True, phenomenon="duplicate_print", control=True, controlAt=8)
        out = audit.save_summary(self.save, self.source)
        self.assertIn("separate duplicate", out["next_action"])
        self.assertFalse(out["hint_conflict"])
        job["stage"] = 7
        self.assertFalse(audit.save_summary(self.save, self.source)["hint_conflict"])

    def test_queue_switch_and_power_exceptions(self):
        state = self.save["state"]["freeform"]
        state["powerFault"] = True
        state["jobs"].append({"id": "FF-D0001-J4", "stage": 3})
        out = audit.save_summary(self.save, self.source)
        self.assertEqual(out["printer_queue"]["target_saved_index"], 1)
        self.assertIn("backup", out["next_action"])
        state["jobs"][0]["stage"] = 2
        self.assertTrue(audit.save_summary(self.save, self.source)["printer_queue"]["selection_blocked_by_active_work"])
        for stage in (3, 6):
            state["jobs"][0]["stage"] = stage
            out = audit.save_summary(self.save, self.source)
            self.assertNotIn("If backup", out["next_action"])
            self.assertIn("do not require power", out["power_policy"])

    def test_function_extraction_nested_and_inline(self):
        body = 'FString FS47FreeformSession::NextAction()const {return TEXT("}");}'
        self.assertEqual(audit.function_body(body, "NextAction"), body)
        nested = 'bool FS47FreeformSession::CollectChart()\n{\n auto F=[](){return true;}; /* } */\n}'
        self.assertEqual(audit.function_body(nested, "CollectChart"), nested)
        self.assertEqual(audit.function_body(body, "Missing"), "")

    def test_bad_selection_and_story_are_explicit(self):
        self.save["state"]["freeform"]["selected"] = -1
        with self.assertRaises(ValueError):
            audit.save_summary(self.save, self.source)
        self.assertEqual(audit.save_summary({"state": {"sessionMode": "story"}}, self.source)["status"], "NO_FREEFORM_STATE")

    def test_scenarios_and_duplicate_findings(self):
        plan = json.loads((audit.HERE / "scenarios.json").read_text())
        ids = {s["id"] for s in plan["scenarios"]}
        self.assertEqual(len(ids), len(plan["scenarios"]))
        self.assertTrue({"routine", "anomaly", "service", "power", "continue", "day_loop", "story", "live_capture", "audio_identity", "upgrade_effect"} <= ids)
        for s in plan["scenarios"]:
            self.assertIn(s["owner"], plan["agent_roles"])
            self.assertTrue(s["steps"] and s["acceptance"])
        finding = {key: "example" for key in audit.REQUIRED_FINDING}
        finding.update(priority="P1", scenario="routine", confidence="source_confirmed", evidence=["source.cpp:1"])
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "findings.json"
            path.write_text(json.dumps([finding]))
            self.assertEqual(len(audit.read_findings([path], ids)), 1)
            with self.assertRaises(ValueError):
                audit.read_findings([path, path], ids)


if __name__ == "__main__":
    unittest.main()
