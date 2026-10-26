#!/usr/bin/env python3
"""Protect source contracts against false observations and circular evidence gates."""
from __future__ import annotations

from copy import deepcopy
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
        self.assertEqual(len(author.validate_missions(self.modules)), 10)

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
        for sid in ("SC02_RETRY", "SC04_ANNEX_RETRY", "SC04_CAR_RETRY", "ARRAY_RETRY"):
            with self.subTest(sid=sid):
                pack, modules = self.fresh()
                self.scene(modules, sid)["on_complete"]["give_documents"] = ["HR_ARRAY_CONTROL_NOTE"]
                with self.assertRaises(author.EV.PackError):
                    author.validate_scenarios(pack, modules)

    def test_route_needs_fresh_exit_report_and_actual_courier(self):
        for gate in ("ctx_hr_sc12_exit_sighting_readback", "ctx_hr_sc12_courier_104_confirmed"):
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


if __name__ == "__main__":
    unittest.main()
