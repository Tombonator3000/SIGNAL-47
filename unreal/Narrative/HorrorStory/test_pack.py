#!/usr/bin/env python3
"""Protect source contracts against false observations and circular evidence gates."""
from __future__ import annotations

from copy import deepcopy
import json
import re
import unittest

import build_pack as author


class UnifiedContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pack, cls.modules = author.EV.validate()

    def fresh(self):
        return deepcopy(self.pack), deepcopy(self.modules)

    @staticmethod
    def scene(modules, sid):
        return next(s for m in modules for s in m["scenes"] if s["id"] == sid)

    def test_complete_authored_route(self):
        result = author.validate_scenarios(*self.fresh())
        self.assertEqual(len(result["scenarios"]), 13)
        self.assertEqual([m["id"] for m in author.validate_missions(self.modules)],
                         [f"H{i:02}" for i in range(1, 9)] + ["H10"])

    def test_reports_need_their_distinct_actual_readback(self):
        cases = [("SC01_FIRST_REPORT", "ctx_hr_sc01_report_readback"),
                 ("SC03_DISPATCH_RECEIPT", "ctx_hr_sc03_dispatch_readback"),
                 ("SC03_MISSING_RECEIPT", "ctx_hr_sc03_missing_readback")]
        for sid, required in cases:
            with self.subTest(sid=sid):
                pack, modules = self.fresh()
                self.scene(modules, sid)["requires"]["all"].remove(required)
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_dispatch_receipt_cannot_claim_missing_person(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC03_DISPATCH_RECEIPT")["on_complete"]["give_documents"] = ["HR_MISSING_RECEIPT"]
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_named_dispatch_receipt_needs_the_named_assignment(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC03_DISPATCH_RECEIPT")["requires"]["all"].remove("ctx_hr_sc03_ortiz_assigned")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_visible_loss_cannot_follow_assignment_alone(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC03_HELPER_TAKEN")["requires"]["all"].remove("ctx_hr_sc03_lift_observed")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_schedule_hold_requires_physical_isolation(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC04_LOCAL_HOLD")["requires"]["all"].remove("ctx_hr_sc04_schedule_isolated")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_passive_residual_does_not_release_schedule_hold(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC05_PASSIVE_RESIDUAL")["on_complete"]["set_flags"]["hr_schedule_running"] = True
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_both_live_and_isolated_tape_are_needed_for_comparison(self):
        for gate in ("ctx_hr_by02_tape_sample_verified", "ctx_hr_by02_live_sample_verified", "ctx_hr_by02_question_compared"):
            with self.subTest(gate=gate):
                pack, modules = self.fresh()
                self.scene(modules, "BY02_SOURCE_COMPARE")["requires"]["all"].remove(gate)
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_late_copy_cannot_be_reported_as_original_reel(self):
        pack, modules = self.fresh()
        self.scene(modules, "BY02_LATE_COPY")["on_complete"]["give_documents"] = ["HR_EARL_SOURCE_CHECK"]
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_late_copy_metadata_cannot_establish_heard_question_result(self):
        pack, modules = self.fresh()
        self.scene(modules, "BY02_LATE_COPY")["requires"]["all"].remove("ctx_hr_by02_question_compared")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_earl_speech_needs_his_actual_presence(self):
        pack, modules = self.fresh()
        self.scene(modules, "BY02_REQUEST")["requires"]["all"].remove("ctx_hr_by02_earl_present")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_array_audio_or_visit_cannot_replace_geometry(self):
        for gate in ("ctx_hr_array_command_checked", "ctx_hr_array_command_history_compared", "ctx_hr_array_reference_fixed", "ctx_hr_array_return_observed", "ctx_hr_array_geometry_compared"):
            with self.subTest(gate=gate):
                pack, modules = self.fresh()
                self.scene(modules, "ARRAY_RESULT")["requires"]["all"].remove(gate)
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_array_operator_order_remains_free(self):
        for sid, opposite in (("ARRAY_POINT_A", "ctx_hr_array_b_observed"),
                              ("ARRAY_POINT_B", "ctx_hr_array_a_observed")):
            with self.subTest(sid=sid):
                pack, modules = self.fresh()
                self.scene(modules, sid)["requires"]["all"].append(opposite)
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_capture_retry_awards_no_evidence(self):
        for sid in ("SC02_RETRY", "SC04_ANNEX_RETRY", "ARRAY_RETRY"):
            with self.subTest(sid=sid):
                pack, modules = self.fresh()
                self.scene(modules, sid)["on_complete"]["give_documents"] = ["HR_ARRAY_CONTROL_NOTE"]
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_route_needs_actual_patrol_sheet_and_courier(self):
        for gate in ("ctx_hr_sc12_patrol_sheet_read", "ctx_hr_sc12_courier_104_confirmed"):
            with self.subTest(gate=gate):
                pack, modules = self.fresh()
                self.scene(modules, "SC12_ROUTE_READBACK")["requires"]["all"].remove(gate)
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_source_comparison_uses_durable_samples_after_switching(self):
        pack, modules = self.fresh()
        compare = self.scene(modules, "BY02_SOURCE_COMPARE")
        compare["requires"]["all"].append("ctx_hr_by02_tape_isolated")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_array_baseline_does_not_require_completed_route(self):
        pack, modules = self.fresh()
        self.scene(modules, "ARRAY_COMMAND_CHECK")["requires"]["all"].append("ctx_hr_array_command_history_compared")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_scene_cannot_require_its_own_completed_presentation(self):
        pack, modules = self.fresh()
        scene = self.scene(modules, "ARRAY_RESULT")
        scene["requires"]["all"].append("hr_array_result_presented")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_annex_imprint_needs_the_previously_heard_line(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC04_ANNEX_IMPRINT")["requires"]["all"].remove("ctx_hr_sc01_dale_reference_heard")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_siege_cannot_claim_complete_bundle_before_acquisition(self):
        pack, modules = self.fresh()
        self.scene(modules, "H07_FIRST_WARNING")["requires"]["all"].remove("ctx_hr_h07_bundle_secured")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_runtime_registry_covers_all_authored_scenes_and_external_owners(self):
        scenarios = author.validate_scenarios(*self.fresh())
        registry = author.runtime_registry(self.pack, self.modules, scenarios)
        authored = [s["id"] for m in self.modules for s in m["scenes"]]
        self.assertEqual([s["id"] for s in registry["scenes"]], authored)
        self.assertEqual(len(set(authored)), 77)
        self.assertEqual({g["id"] for g in registry["external_flags"]}, set(self.pack["external_flags"]))
        self.assertEqual(len(registry["external_flags"]), 90)
        self.assertTrue(all(g["owner"] == "runtime_host" for g in registry["external_flags"]))
        self.assertTrue(all(s["scenario_ids"] and s["mission_ids"] for s in registry["scenes"]))
        self.assertEqual({sc for s in registry["scenes"] for sc in s["scenario_ids"]},
                         {f"SC{i:02}" for i in range(1, 14)})

    def test_six_original_h_cards_keep_scenario_and_authored_phase(self):
        scenarios = author.validate_scenarios(*self.fresh())
        registry = author.runtime_registry(self.pack, self.modules, scenarios)
        rows = {s["id"]: s for s in registry["scenes"]}
        for mission, scenario in (("H01", "SC01"), ("H02", "SC02")):
            for suffix in ("_BRIEF", "_CHECK", "_INCOMPLETE"):
                sid = mission + suffix
                self.assertEqual(rows[sid]["scenario_ids"], [scenario])
                self.assertEqual(rows[sid]["phase"], self.scene(self.modules, sid)["phase"])

    def test_compiled_export_roundtrips_exact_source_and_registry(self):
        scenarios = author.validate_scenarios(*self.fresh())
        registry = author.runtime_registry(self.pack, self.modules, scenarios)
        runtime = author.render_runtime(self.pack, self.modules, registry)
        chunks = re.findall(r'TEXT\(R"S47HR\((.*?)\)S47HR"\)', runtime.decode("ascii"), re.S)
        self.assertTrue(chunks)
        self.assertTrue(all(len(chunk) <= 6000 for chunk in chunks))
        decoded = json.loads("".join(chunks))
        self.assertEqual(decoded, {"pack": self.pack, "modules": self.modules, "registry": registry})
        self.assertEqual(author.render_runtime(self.pack, self.modules, registry), runtime)

    def test_runtime_export_does_not_mark_future_gameplay_integrated(self):
        scenarios = author.validate_scenarios(*self.fresh())
        registry = author.runtime_registry(self.pack, self.modules, scenarios)
        decoded = registry["status"]
        self.assertEqual(decoded, "runtime_data_exported_gameplay_acceptance_separate")
        self.assertTrue(all(s["runtime_status"] == "authored_not_integrated" for s in scenarios["scenarios"]))
        self.assertEqual(self.pack["production"]["runtime_status"], "not_integrated")

    def test_new_report_does_not_depend_on_optional_sc01_window_sighting(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC03_DISPATCH_RECEIPT")["requires"]["all"].append("ctx_hr_sc01_report_readback")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_receiver_log_request_needs_the_deputy_actually_at_the_stop(self):
        pack, modules = self.fresh()
        self.scene(modules, "SC03_RECEIVER_QUESTION")["requires"]["all"].remove("ctx_hr_sc03_deputy_at_stop")
        with self.assertRaises(author.EV.PackError):
            author.validate_scenarios(pack, modules)

    def test_revised_route_has_one_creature_rule_and_no_extra_car_meeting(self):
        scenarios = author.validate_scenarios(*self.fresh())
        scenes = {s["id"] for m in self.modules for s in m["scenes"]}
        self.assertFalse({"SC03_SHADOW_RULE", "SC04_CAR_READ", "SC04_CAR_ESCAPE", "SC04_CAR_RETRY"} & scenes)
        self.assertNotIn("DRIVER", self.pack["actors"])
        self.assertFalse(any(l["speaker"] == "DRIVER" for m in self.modules for s in m["scenes"] for l in s["lines"]))
        sc12 = next(s for s in scenarios["scenarios"] if s["id"] == "SC12")
        self.assertEqual(sc12["mission_ids"], ["H08"])
        self.assertTrue({"H09_BRIEF", "H09_CHECK", "H09_INCOMPLETE", "SC12_PATROL_SHEET"}.issubset(sc12["scene_ids"]))

    def test_four_act_delivery_preserves_scenario_level_checks(self):
        output = author.render_scenarios(author.validate_scenarios(*self.fresh())).decode("utf-8")
        self.assertIn("SC02 til SC05", output)
        self.assertIn("SC06 til SC08", output)
        self.assertIn("SC09 og SC10", output)
        self.assertIn("SC11 til SC13", output)
        self.assertIn("én gang per akt", output)


class WorldProducers(unittest.TestCase):
    """Every SC fact needs a named producer in real code and a test."""

    @classmethod
    def setUpClass(cls):
        cls.pack, cls.modules = author.EV.validate()
        scenarios = author.validate_scenarios(deepcopy(cls.pack), deepcopy(cls.modules))
        cls.registry = author.runtime_registry(cls.pack, cls.modules, scenarios)
        cls.data = author.EV.read_json(author.ROOT / "world_producers.json")

    def check(self, data):
        return author.validate_producers(self.pack, self.modules, self.registry, data)

    def test_all_flags_scenes_and_scenarios_are_covered(self):
        data = self.check(deepcopy(self.data))
        self.assertEqual(len(data["flags"]), len(self.pack["external_flags"]))
        self.assertEqual(len(data["scenes"]), 77)
        self.assertEqual([p["scenario"] for p in data["work_packages"]], [f"SC{i:02}" for i in range(1, 14)])

    def test_unowned_flag_is_rejected(self):
        data = deepcopy(self.data)
        data["flags"] = [f for f in data["flags"] if f["flag"] != "ctx_hr_sc02_escape_complete"]
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_invented_code_symbol_is_rejected(self):
        data = deepcopy(self.data)
        row = next(f for f in data["flags"] if f["status"] == "existing")
        row["symbols"] = ["S47NoSuchFunctionAnywhere"]
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_existing_producer_cannot_point_at_a_planned_file(self):
        data = deepcopy(self.data)
        row = next(f for f in data["flags"] if f["status"] == "existing")
        row["sources"] = ["unreal/Signal47/Source/Signal47/Private/S47NotYetWritten.cpp (ny)"]
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_flag_must_belong_to_its_first_scenario(self):
        data = deepcopy(self.data)
        next(f for f in data["flags"] if f["flag"] == "ctx_hr_sc02_threat_visible")["scenario"] = "SC03"
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_each_scenario_needs_all_eight_test_categories(self):
        data = deepcopy(self.data)
        for test in data["tests"]:
            if test["scenario"] == "SC02":
                test["covers"] = [c for c in test["covers"] if c != "continue"] or ["normal"]
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_scene_matrix_row_needs_a_known_test(self):
        data = deepcopy(self.data)
        data["scenes"][0]["tests"] = ["SC01-T99"]
        with self.assertRaises(author.EV.PackError):
            self.check(data)

    def test_first_world_owner_follows_scenario_order_not_module_order(self):
        registry = deepcopy(self.registry)
        registry["scenes"].reverse()
        author.validate_producers(self.pack, self.modules, registry, deepcopy(self.data))


if __name__ == "__main__":
    unittest.main()
