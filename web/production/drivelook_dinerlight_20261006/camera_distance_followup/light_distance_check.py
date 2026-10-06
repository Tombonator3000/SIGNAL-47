#!/usr/bin/env python3
"""Targeted camera/player regressions; no browser, game or image capture.

The historical manifests are read without changing their recorded findings.
--capture-path permits a negative control against the tool before the fix.
"""
import argparse
import copy
from datetime import datetime, timezone
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parent
WEB = ROOT.parents[2]
CAPTURE_PATH = WEB / 'tools/drivelook.py'
CAPTURE = None
HISTORICAL_COUNT = 0
JS_CHECK = None


def ledger(*, driving=True, camera=(100, 2, 0), player=(0, 1.7, 0), spot=(100, 900, 0)):
    return {'driving': driving, 'camera_xyz': list(camera), 'player_xyz': list(player),
            'sets': [], 'spots': [{'uuid': 'distance-probe', 'position': list(spot),
                'target': [0, 0, 0], 'colour_rgb': [1, .3, .1], 'intensity': 4.4,
                'distance': 30, 'visible': True, 'effective_visible': True,
                'cast_shadow': True, 'matched_slots': []}]}


def legacy(value):
    value = copy.deepcopy(value)
    del value['driving']; del value['camera_xyz']
    return value


def historical_states():
    for path in sorted((ROOT.parent / 'evidence_20261006_v4/captures').glob('*/manifest.json')):
        data = json.loads(path.read_text())
        for trip in data['trips']:
            yield from trip['shots']
            yield from (row['state'] for row in trip.get('onfoot_trace', []))


class DistanceRegression(unittest.TestCase):
    def test_driving_spot_near_camera_is_not_far_from_left_behind_player(self):
        self.assertEqual(CAPTURE.dinerlight_light_findings(ledger()), [])

    def test_driving_spot_near_player_but_far_from_camera_is_flagged(self):
        value = ledger(spot=(0, 900, 0))
        result = CAPTURE.dinerlight_light_findings(value)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]['kind'], 'active_spot_far_from_observed_camera')
        self.assertEqual(result[0]['horizontal_camera_distance_m'], 100)
        self.assertEqual(result[0]['distance_reference'], 'camera_xyz')
        self.assertEqual(result[0]['reference_xyz'], value['camera_xyz'])
        self.assertEqual(result[0]['threshold_m'], 45)

    def test_foot_and_parked_camera_do_not_replace_player(self):
        for camera in ((100, 2, 0), (8000, 1000, -300)):
            with self.subTest(camera=camera):
                self.assertEqual(CAPTURE.dinerlight_light_findings(
                    ledger(driving=False, camera=camera, spot=(0, 900, 0))), [])

    def test_foot_spot_near_camera_but_far_from_player_is_flagged(self):
        result = CAPTURE.dinerlight_light_findings(ledger(driving=False))
        self.assertEqual(result[0]['kind'], 'active_spot_far_from_observed_player')
        self.assertEqual(result[0]['horizontal_player_distance_m'], 100)
        self.assertEqual(result[0]['distance_reference'], 'player_xyz')
        self.assertEqual(result[0]['reference_xyz'], [0, 1.7, 0])

    def test_camera_threshold_uses_xz_strictly_over_45_metres(self):
        for x, expected in ((145, 0), (145.1, 1)):
            with self.subTest(x=x):
                result = CAPTURE.dinerlight_light_findings(ledger(spot=(x, 900, 0)))
                self.assertEqual(len(result), expected)
                if expected:
                    self.assertAlmostEqual(result[0]['horizontal_camera_distance_m'], 45.1)

    def test_invalid_modern_observer_instrumentation_blocks(self):
        changes = ({'driving': 'false'}, {'driving': 0}, {'driving': None},
                   {'camera_xyz': None}, {'camera_xyz': [100, 2]},
                   {'camera_xyz': [100, math.nan, 0]}, {'camera_xyz': [math.inf, 2, 0]})
        for change in changes:
            for driving in (True, False):
                value = ledger(driving=driving); value.update(change)
                with self.subTest(change=change, driving=driving), self.assertRaises(ValueError):
                    CAPTURE.dinerlight_light_findings(value)
        for missing in ('driving', 'camera_xyz'):
            value = ledger(); del value[missing]
            with self.subTest(missing=missing), self.assertRaises(ValueError):
                CAPTURE.dinerlight_light_findings(value)

    def test_inactive_hidden_and_black_spots_do_not_create_distance_candidates(self):
        for change in ({'intensity': 0}, {'effective_visible': False}, {'colour_rgb': [0, 0, 0]}):
            value = ledger(spot=(0, 900, 0)); value['spots'][0].update(change)
            with self.subTest(change=change):
                self.assertEqual(CAPTURE.dinerlight_light_findings(value), [])

    def test_legacy_result_shape_and_input_are_unchanged(self):
        value = legacy(ledger()); before = copy.deepcopy(value)
        self.assertEqual(CAPTURE.dinerlight_light_findings(value), [{
            'kind': 'active_spot_far_from_observed_player', 'spot_uuid': 'distance-probe',
            'horizontal_player_distance_m': 100, 'threshold_m': 45,
            'classification': 'mechanical_candidate_not_visual_verdict'}])
        self.assertEqual(value, before)

    def test_all_220_historical_ledger_results_reproduce_without_changes(self):
        global HISTORICAL_COUNT
        HISTORICAL_COUNT = 0
        for state in historical_states():
            value = state['body_light_ledger']; before = copy.deepcopy(value)
            self.assertNotIn('driving', value); self.assertNotIn('camera_xyz', value)
            self.assertEqual(CAPTURE.dinerlight_light_findings(value), state['light_findings'])
            self.assertEqual(value, before)
            HISTORICAL_COUNT += 1
        self.assertEqual(HISTORICAL_COUNT, 220)

    def test_recorded_immediate_foot_camera_separation_uses_player(self):
        states = [s for s in historical_states() if s['body_light_ledger'].get('on_foot_diner')]
        state = max(states, key=lambda s: math.dist(s['camera_xyz'], s['player_xyz']))
        self.assertFalse(state['driving'])
        self.assertGreater(math.dist(state['camera_xyz'], state['player_xyz']), 300)
        value = copy.deepcopy(state['body_light_ledger'])
        value.update(driving=state['driving'], camera_xyz=list(state['camera_xyz']))
        self.assertEqual(CAPTURE.dinerlight_light_findings(value), state['light_findings'])

    def test_actual_js_observer_records_camera_and_driving_without_pose_changes(self):
        global JS_CHECK
        node = shutil.which('node')
        self.assertIsNotNone(node, 'Node runtime is required for the actual observer check')
        run = subprocess.run([node, str(ROOT / 'ledger_observation_check.cjs')],
                             input=CAPTURE.DINERLIGHT_EXTRA, text=True,
                             capture_output=True, timeout=20)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        JS_CHECK = json.loads(run.stdout)
        self.assertEqual(JS_CHECK['status'], 'PASS')


def main():
    global CAPTURE, CAPTURE_PATH
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture-path', type=Path, default=CAPTURE_PATH)
    parser.add_argument('--result', type=Path)
    args = parser.parse_args(); CAPTURE_PATH = args.capture_path.resolve()
    spec = importlib.util.spec_from_file_location('camera_distance_subject', CAPTURE_PATH)
    CAPTURE = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = CAPTURE; spec.loader.exec_module(CAPTURE)
    stream = io.StringIO()
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(DistanceRegression)
    result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    report = {'checked_utc': datetime.now(timezone.utc).isoformat(),
              'status': 'PASS' if result.wasSuccessful() else 'FAIL',
              'tests_run': result.testsRun, 'failures': len(result.failures),
              'errors': len(result.errors), 'historical_ledger_replays': HISTORICAL_COUNT,
              'actual_js_observer': JS_CHECK, 'browser_launched': False,
              'gameplay_rendering_hardware_audio': 'UNVERIFIED',
              'output': stream.getvalue(), 'capture_path': str(CAPTURE_PATH),
              'files_sha256': {str(p.relative_to(WEB)) if p.is_relative_to(WEB) else str(p):
                               hashlib.sha256(p.read_bytes()).hexdigest() for p in
                               (CAPTURE_PATH, Path(__file__), ROOT / 'ledger_observation_check.cjs')}}
    if args.result:
        with args.result.open('x') as f: json.dump(report, f, indent=2); f.write('\n')
    print(stream.getvalue(), end='')
    print(f'{report["status"]}: {result.testsRun} regressions, {HISTORICAL_COUNT} historical ledgers')
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    raise SystemExit(main())
