#!/usr/bin/env python3
"""Lightweight dinerlight contracts with fake pages; no browser or game is launched.

Run from any directory with the capture Python runtime:
    python3 tool_check.py --result /tmp/dinerlight-tool-checks.json
The optional JSON contains mock/contract results, never rendered lighting proof.
"""
from __future__ import annotations

import argparse
import asyncio
import copy
from datetime import datetime, timezone
import hashlib
import importlib.util
import io
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from types import ModuleType, SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parent
WEB = ROOT.parent.parent
CAPTURE_PATH = WEB / 'tools/drivelook.py'
HASH_CHECK_PATH = ROOT.parent / 'drivelook_kessler_20261006/code_hash_check.py'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


capture = load('dinerlight_contract_capture', CAPTURE_PATH)
# Reuse the existing response/context fakes, not their tests or image revalidation.
hash_checks = load('dinerlight_existing_hash_fakes', HASH_CHECK_PATH)


def quality_state(quality='high'):
    ultra = quality == 'ultra'
    return {'quality': quality, 'picture': 'off' if quality == 'low' else 'vhs',
            'ultra_enabled': ultra, 'ultra_on': ultra, 'post': {'supported': True},
            'render_state': {'pixel_ratio': 1}, 'held': True}


def light_ledger():
    return {'player_xyz': [0, 1.7, 0],
            'sets': [{'key': 'diner', 'count': 1, 'capacity': 13, 'scale': .5,
                      'skip': [], 'headlight_slots': [],
                      'slots': [{'index': 0, 'position': [0, 2, 0],
                                 'colour_rgb': [1, .3, .1], 'strength_w': -4.4,
                                 'skipped': False}]}],
            'spots': [{'uuid': 'mock-spot', 'position': [0, 2, 0],
                       'target': [0, 0, 0], 'colour_rgb': [1, .3, .1],
                       'intensity': 4.4, 'distance': 30, 'visible': True,
                       'effective_visible': True, 'cast_shadow': True,
                       'matched_slots': [{'key': 'diner', 'index': 0}]}]}


class GeometryContracts(unittest.TestCase):
    def test_remaining_distance_follows_bends_in_loaded_polyline(self):
        # The goal is only 100 m away as the crow flies, but 700 m by this road.
        points = [[0, 0, 0], [0, 30, 300], [100, 10, 300], [100, 0, 0]]
        start = capture.dinerlight_route_remaining(points, [0, 1000, 0])
        self.assertAlmostEqual(start['remaining_m'], 700)
        self.assertAlmostEqual(start['total_m'], 700)
        self.assertEqual(start['goal_xyz'], points[-1])
        self.assertGreater(start['remaining_m'], 300)
        near = capture.dinerlight_route_remaining(points, [100, -500, 250])
        self.assertAlmostEqual(near['remaining_m'], 250)
        self.assertAlmostEqual(near['nearest_s_m'], 450)
        self.assertAlmostEqual(near['lateral_m'], 0)

    def test_projection_clamps_endpoints_and_ignores_height_origin(self):
        points = [[0, 0, 0], [100, 10, 0], [100, 20, 100]]
        before = capture.dinerlight_route_remaining(points, [-20, 400, 0])
        self.assertEqual(before['remaining_m'], 200)
        self.assertEqual(before['lateral_m'], 20)
        after = capture.dinerlight_route_remaining(points, [100, -20, 150])
        self.assertEqual(after['remaining_m'], 0)
        self.assertEqual(after['lateral_m'], 50)
        origin = [5000, 300, -4000]
        shift = lambda p: [v + o for v, o in zip(p, origin)]
        current = [110, 500, 40]
        a = capture.dinerlight_route_remaining(points, current)
        b = capture.dinerlight_route_remaining([shift(p) for p in points], shift(current))
        for key in ('remaining_m', 'total_m', 'nearest_s_m', 'lateral_m'):
            self.assertAlmostEqual(a[key], b[key])

    def test_missing_degenerate_and_nonfinite_route_is_blocking(self):
        for points, xyz in (([], [0, 0, 0]), ([[0, 0, 0]], [0, 0, 0]),
                            ([[0, 0, 0], [0, 2, 0]], [0, 0, 0]),
                            ([[0, 0, 0], [100, 0, math.nan]], [0, 0, 0]),
                            ([[0, 0, 0], [100, 0, 0]], [math.inf, 0, 0])):
            with self.subTest(points=points, xyz=xyz), self.assertRaises(ValueError):
                capture.dinerlight_route_remaining(points, xyz)

    def test_forty_metre_schedule_retains_crossed_thresholds(self):
        due, next_at = capture.capture_due(0, 40, 40)
        self.assertEqual((due, next_at), ([], 40))
        all_due = []
        for travelled in (39.9, 40, 121, 300):
            due, next_at = capture.capture_due(travelled, next_at, 40)
            all_due += due
        self.assertEqual(all_due, [40, 80, 120, 160, 200, 240, 280])
        self.assertEqual(next_at, 320)

    def test_tick_batches_preserve_precision_near_arrival(self):
        for remaining, seconds in ((900, 4), (450, .5), (300, .5),
                                   (20, 1 / 30), (0, 1 / 30)):
            with self.subTest(remaining=remaining):
                self.assertEqual(capture.dinerlight_tick_seconds(
                    {'route_remaining_m': remaining}, 'dinerlight_arrival'), seconds)
        self.assertEqual(capture.dinerlight_tick_seconds(
            {'route_remaining_m': 900}, 'dinerlight_departure'), .5)


class QualityContracts(unittest.TestCase):
    def test_each_quality_uses_its_player_default(self):
        for quality in ('low', 'high', 'ultra'):
            with self.subTest(quality=quality):
                self.assertFalse(capture.dinerlight_quality_errors(quality_state(quality), quality))
        low = quality_state('low'); low['post']['supported'] = False
        self.assertFalse(capture.dinerlight_quality_errors(low, 'low'))

    def test_fallback_wrong_picture_dpr_and_unheld_loop_block(self):
        cases = [('ultra', {'quality': 'high'}),
                 ('ultra', {'ultra_enabled': False}),
                 ('ultra', {'ultra_on': False}),
                 ('ultra', {'post': {'supported': False}}),
                 ('high', {'picture': 'off'}), ('low', {'picture': 'vhs'}),
                 ('high', {'render_state': {'pixel_ratio': 2}}),
                 ('high', {'held': False}), ('high', {'ultra_enabled': 'false'})]
        for quality, change in cases:
            with self.subTest(quality=quality, change=change):
                state = quality_state(quality); state.update(change)
                self.assertTrue(capture.dinerlight_quality_errors(state, quality))
        self.assertTrue(capture.dinerlight_quality_errors(quality_state(), 'unknown'))


class LightLedgerContracts(unittest.TestCase):
    def test_owned_spot_with_fake_disabled_has_no_double_candidate(self):
        self.assertEqual(capture.dinerlight_light_findings(light_ledger()), [])

    def test_double_and_skipped_slot_are_candidates_not_visual_pass(self):
        ledger = light_ledger(); slot = ledger['sets'][0]['slots'][0]
        slot['strength_w'] = 4.4; slot['skipped'] = True
        ledger['sets'][0]['skip'] = [0]
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual({f['kind'] for f in findings},
                         {'positive_fake_with_matching_spot', 'active_spot_on_skipped_slot'})
        self.assertTrue(all(f['classification'] == 'mechanical_candidate_not_visual_verdict'
                            for f in findings))
        self.assertTrue(all(f['slot_index'] == 0 for f in findings))
        self.assertNotIn('PASS', json.dumps(findings))

    def test_stale_distance_is_horizontal_and_threshold_is_explicit(self):
        ledger = light_ledger(); spot = ledger['spots'][0]
        spot['matched_slots'] = []; spot['position'] = [45, 900, 0]
        self.assertEqual(capture.dinerlight_light_findings(ledger), [])
        spot['position'][0] = 45.1
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual([f['kind'] for f in findings], ['active_spot_far_from_observed_player'])
        self.assertAlmostEqual(findings[0]['horizontal_player_distance_m'], 45.1)
        self.assertEqual(findings[0]['threshold_m'], 45)

    def test_inactive_inherited_hidden_or_black_spots_do_not_count(self):
        for change in ({'intensity': 0}, {'effective_visible': False},
                       {'colour_rgb': [0, 0, 0]}):
            with self.subTest(change=change):
                ledger = light_ledger(); ledger['sets'][0]['slots'][0]['strength_w'] = 4.4
                ledger['spots'][0].update(change)
                self.assertEqual(capture.dinerlight_light_findings(ledger), [])

    def test_negative_skipped_alias_is_not_a_second_real_source(self):
        ledger = light_ledger()
        alias = copy.deepcopy(ledger['sets'][0])
        alias.update(key='road_body', skip=[0], headlight_slots=[0])
        alias['slots'][0].update(strength_w=-2.2, skipped=True, colour_rgb=[.3, .09, .03])
        ledger['sets'].append(alias)
        ledger['spots'][0]['matched_slots'].append({'key': 'road_body', 'index': 0})
        self.assertEqual(capture.dinerlight_light_findings(ledger), [])
        alias['slots'][0]['strength_w'] = 2.2
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual([f['kind'] for f in findings], ['positive_fake_with_matching_spot'])
        self.assertEqual(findings[0]['set_key'], 'road_body')

    def test_zero_scaled_fake_set_is_not_positive_light_energy(self):
        ledger = light_ledger()
        ledger['sets'][0]['slots'][0]['strength_w'] = 4.4
        ledger['sets'][0]['scale'] = 0
        self.assertEqual(capture.dinerlight_light_findings(ledger), [])
        ledger['sets'][0]['scale'] = .5
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual([f['kind'] for f in findings], ['positive_fake_with_matching_spot'])
        self.assertEqual(findings[0]['fake_set_scale'], .5)

    def test_wrong_colour_at_same_position_is_not_a_source_association(self):
        ledger = light_ledger()
        ledger['sets'][0]['slots'][0].update(strength_w=4.4, skipped=True, colour_rgb=[0, 1, 1])
        self.assertEqual(capture.dinerlight_light_findings(ledger), [])
        ledger['on_foot_diner'] = True
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual([f['kind'] for f in findings], ['active_spot_without_diner_source'])
        self.assertEqual(findings[0]['matched_set_keys'], [])

    def test_on_foot_nearby_wrong_set_is_still_a_candidate(self):
        ledger = light_ledger(); ledger['on_foot_diner'] = True
        self.assertEqual(capture.dinerlight_light_findings(ledger), [])
        ledger['sets'][0]['key'] = 'road_body'
        ledger['spots'][0]['matched_slots'][0]['key'] = 'road_body'
        findings = capture.dinerlight_light_findings(ledger)
        self.assertEqual([f['kind'] for f in findings], ['active_spot_without_diner_source'])
        self.assertEqual(findings[0]['matched_set_keys'], ['road_body'])
        self.assertEqual(findings[0]['classification'], 'mechanical_candidate_not_visual_verdict')

    def test_missing_nonfinite_and_invalid_matches_are_blocking(self):
        cases = [{}]
        ledger = light_ledger(); ledger['spots'][0]['intensity'] = math.nan; cases.append(ledger)
        ledger = light_ledger(); ledger['sets'][0]['count'] = 2; cases.append(ledger)
        ledger = light_ledger(); ledger['spots'][0]['position'][0] = .051; cases.append(ledger)
        ledger = light_ledger(); ledger['spots'][0]['matched_slots'][0]['index'] = 12; cases.append(ledger)
        for ledger in cases:
            with self.subTest(ledger=ledger), self.assertRaises(ValueError):
                capture.dinerlight_light_findings(ledger)


class DinerPage(hash_checks.Page):
    """Deterministic API model, not JS execution, simulation or rendered evidence."""
    route = [[0, 0, 0], [0, 30, 300], [100, 10, 300], [100, 0, 0]]

    def __init__(self, fail_at='', quality='high'):
        super().__init__(fail_at)
        self.quality = quality
        self.phase, self.travel, self.clock, self.park_ticks = None, 0., 0., 0
        self.parked = self.onfoot = False
        self.init_scripts, self.calls, self.tick_args, self.looks = [], [], [], []
        self.camera, self.camera_q = [0, 2, 0], [0, 0, 0, 1]

    async def add_init_script(self, code): self.init_scripts.append(code)
    async def wait_for_timeout(self, _): pass

    async def goto(self, url, **kwargs):
        if not self.init_scripts: raise AssertionError('Missing pre-boot init script')
        await super().goto(url, **kwargs)

    def xyz(self):
        if self.phase == 'chapter6': return [100 + self.travel, 0, 0]
        s = min(self.travel, 696)
        if s <= 300: return [0, 0, s]
        if s <= 400: return [s - 300, 0, 300]
        return [100, 0, 700 - s]

    def model_state(self):
        arrival = self.phase == 'chapter5'
        natural_diner = self.parked and self.park_ticks >= 2 and self.fail_at != 'missing_transition'
        area = 'diner' if self.onfoot or natural_diner else 'road' if arrival and self.travel else 'saro' if arrival else 'roswell'
        driving = not self.parked and not self.onfoot
        xyz = self.xyz()
        player = [100, 1.7, 0] if self.onfoot or natural_diner else [v for v in xyz]
        headlights = driving or self.fail_at == 'parked_lights_on'
        if self.fail_at == 'prepark_lights_off': headlights = False
        st = {'world_xyz': xyz, 'local_xyz': xyz, 'map_xyz': xyz,
              'driver_xyz': xyz, 'camera_xyz': list(self.camera), 'heading_rad': 0,
              'view_direction': [0, 0, 1], 'cab_yaw_rad': 0, 'cab_pitch_rad': 0,
              'camera_role': 'driver_cab' if driving else 'player_after_parking',
              'clock_seconds': self.clock, 'draw_calls': 0, 'area': area,
              'leg': area if driving else None, 'driving': driving,
              'truck_at': 'diner', 'mile': None if arrival else self.travel / 1609.34,
              'oldroad_s_m': None, 'oldroad_distance_to_center_m': None,
              'oldroad_distance_finite': True, 'road_local_z': xyz[2],
              'speed_mps': .2 if arrival and self.travel >= 696 and driving else 20 if driving else 0,
              'chapter6_stage': 'drive', 'chapter_phase': 'ch5' if arrival else 'ch6',
              'chapter5_stage': 'diner' if natural_diner else 'to-truck',
              'chapter5_arrived': natural_diner and self.fail_at != 'missing_callback',
              'cinematic': self.parked and not natural_diner, 'busy': False,
              'camera_quaternion': list(self.camera_q), 'camera_projection': [1] * 16,
              'camera_world': [1] * 16, 'truck_quaternion': [0, 0, 0, 1],
              'player_xyz': player, 'headlights_active': headlights and not self.onfoot,
              'sky_uniforms': {}, 'diner_dawn': 0, 'diner_origin': [0, 0, 0],
              'truck_view_state': {'driving': driving, 'group_visible': True, 'body_visible': True,
                                   'cab_visible': driving, 'shell_visible': not driving},
              'road_origin': [0, 0, 0],
              'route_points': copy.deepcopy(self.route) if arrival else [[100, 0, 0], [400, 0, 0]],
              'route_source_api': 'mock loaded diner arrival polyline' if arrival else 'mock loaded OldRoad park polyline'}
        st.update(quality_state(self.quality))
        st['render_state'].update(buffer_width=844, buffer_height=390,
                                  tone_mapping=0, exposure=1, ultra_mapped=0)
        st['body_light_ledger'] = light_ledger()
        st['body_light_ledger']['player_xyz'] = player
        # Keep the mocked real source near the modelled player in every phase.
        spot = st['body_light_ledger']['spots'][0]
        spot['position'] = [player[0], 2, player[2]]
        st['body_light_ledger']['sets'][0]['slots'][0]['position'] = list(spot['position'])
        if self.fail_at == 'wrong_preset': st['picture'] = 'off'
        if self.fail_at == 'changed_route' and self.travel > 0: st['route_points'][-1][0] += 1
        return st

    def frozen(self):
        st = self.model_state()
        return {'camera': list(self.camera), 'camera_q': list(self.camera_q),
                'driver': st['driver_xyz'], 'truck': st['world_xyz'],
                'truck_q': st['truck_quaternion'], 'heading': 0, 'speed': st['speed_mps'],
                'clock': self.clock, 'player': st['player_xyz']}

    async def evaluate(self, code, arg=None):
        self.calls.append((code, copy.deepcopy(arg)))
        if code in (capture.STATE, capture.DINERLIGHT_EXTRA): return self.model_state()
        if code == 'performance.now()/1000': return 123.456
        if code == capture.DINERLIGHT_DRAW: return {'render_time_s': arg}
        if code == capture.DINERLIGHT_LOOK:
            before = self.frozen()
            if arg['cab']:
                self.camera = list(arg['cab']['camera_xyz'])
            self.camera_q = [0, 1, 0, 0]
            saved = {'before': before, 'after': self.frozen(), 'render_time_s': arg['time'],
                     'target_xyz': [100, 2, 0], 'proxy': arg['proxy'], 'cab_pose': bool(arg['cab'])}
            self.looks.append(copy.deepcopy(saved))
            return saved
        if code == capture.DINERLIGHT_RESTORE:
            self.camera, self.camera_q = list(arg['before']['camera']), list(arg['before']['camera_q'])
            return {'unchanged': self.fail_at != 'look_drift', 'camera_restored': self.fail_at != 'camera_restore',
                    'visibility_restored': self.fail_at != 'visibility_restore',
                    'render_time_s': arg['render_time_s'], 'after': self.frozen()}
        if code == 'chapter=>{S47.hold=true;S47.jump(chapter);}':
            self.phase = arg
            return True
        if 'S47.ch3.truckOverride.use()' in code: return True
        if code in ('S47.hold=true', 'S47.tick(0.25,30)'): return True
        if code.startswith('S47.world.driving && !S47.world.busy'): return True
        if code.startswith('([seconds,fps])'):
            seconds, fps = arg; self.tick_args.append((seconds, fps)); self.clock += seconds
            if self.onfoot: return True
            if self.parked: self.park_ticks += 1; return True
            if self.phase == 'chapter5' and self.travel >= 696:
                self.parked = True
            else:
                self.travel += seconds * 20
                if self.phase == 'chapter5': self.travel = min(self.travel, 696)
                self.camera = [self.xyz()[0], 2, self.xyz()[2]]
            return True
        if 'world.enter(\'diner\')' in code:
            self.onfoot, self.parked, self.park_ticks = True, True, 2
            return True
        if code in ('S47.hold=true;S47.world.autopilot=20', 'S47.world.autopilot=null'): return True
        if 'getContext()' in code: return {'renderer': 'mock API model, not a GPU test', 'pixel_ratio': 1}
        raise AssertionError(f'Unexpected fake-page API call: {code[:90]}')


class DinerContext(hash_checks.Context):
    async def close(self):
        if self.page.fail_at == 'close_http':
            response = hash_checks.Response('http://test/late.png', 'image')
            response.status = 500
            self.page.events['response'](response)
        if self.page.fail_at == 'close_nested_body':
            later = hash_checks.Response('http://test/second-late.js', fail=True)
            first = hash_checks.Response('http://test/first-late.js', during=lambda: self.page.events['response'](later))
            self.page.events['response'](first)
        await super().close()


class DinerBrowser(hash_checks.Browser):
    def __init__(self, fail_at='', quality='high'):
        self.fail_at, self.quality, self.contexts, self.options, self.closed = fail_at, quality, [], [], False

    async def new_context(self, **options):
        context = DinerContext(DinerPage(self.fail_at, self.quality))
        self.contexts.append(context); self.options.append(options)
        return context

    async def close(self): self.closed = True


class RunnerContracts(unittest.IsolatedAsyncioTestCase):
    async def run_fake(self, identity='dinerlight_arrival', mode='', quality='high'):
        browser = DinerBrowser(mode, quality)
        report = {'trips': [], 'status': 'CAPTURING', 'failures': [], 'source': {'git_head': 'mock'}}
        args = SimpleNamespace(size=(844, 390), quality=quality)
        with tempfile.TemporaryDirectory() as tmp, patch('sys.stdout', new=io.StringIO()):
            failure = None
            try:
                await capture.run_dinerlight_trip(browser, args, report, identity, 'http://test/', Path(tmp))
            except (RuntimeError, OSError, AssertionError) as error: failure = error
            saved = json.loads((Path(tmp) / 'manifest.json').read_text())['trips'][0]
            # Cleanup must seal/detach every observer before the final manifest write.
            before = (Path(tmp) / 'manifest.json').read_bytes()
            await asyncio.sleep(0)
            self.assertEqual((Path(tmp) / 'manifest.json').read_bytes(), before)
        page = browser.contexts[0].page
        self.assertFalse(page.events)
        self.assertTrue(saved['code_observation_sealed'])
        self.assertFalse([r for r in saved['observed_code_responses'] if r['hash_state'] == 'pending'])
        self.assertFalse([t for t in asyncio.all_tasks() if t is not asyncio.current_task() and not t.done()])
        return saved, browser, failure

    async def test_full_arrival_and_departure_default_quality_contracts(self):
        for identity in ('dinerlight_arrival', 'dinerlight_departure'):
            for quality in ('low', 'high', 'ultra'):
                with self.subTest(identity=identity, quality=quality):
                    trip, browser, failure = await self.run_fake(identity, quality=quality)
                    self.assertIsNone(failure); self.assertEqual(trip['status'], 'CAPTURED')
                    self.assertEqual(trip['visual_review'], 'UNVERIFIED')
                    self.assertEqual(trip['route_placement_calls'], 0)
                    self.assertEqual(trip['clock_assignment_calls'], 0)
                    first = trip['shots'][0]
                    self.assertEqual(first['name'], 'route_start')
                    for key in ('map_xyz', 'camera_xyz', 'camera_quaternion', 'clock_seconds', 'route_distance_m'):
                        self.assertEqual(first[key], trip['route_start'][key])
                    thresholds = [v for shot in trip['shots'] for v in shot['interval_thresholds_m'] if v != 300]
                    self.assertEqual(thresholds, list(range(40, 281, 40)))
                    self.assertTrue(trip['code_observation_closed'])
                    self.assertFalse(capture.code_hash_errors(trip, require_closed=True))
                    page = browser.contexts[0].page
                    self.assertIn('JSON.stringify("' + quality + '")', page.init_scripts[0])
                    self.assertTrue(all(fps == 30 for _, fps in page.tick_args))
                    self.assertFalse(any('setQuality' in code or 'setPicture' in code for code, _ in page.calls))
                    if identity.endswith('arrival'):
                        self.assertLessEqual(trip['approach_start']['route_remaining_m'], 300)
                        self.assertTrue(trip['prepark_observed_pose']['headlights_active'])
                        parked = next(s for s in trip['shots'] if s['name'] == 'arrival_end_parked')
                        self.assertFalse(parked['headlights_active']); self.assertTrue(parked['chapter5_arrived'])
                        self.assertEqual(parked['area'], 'diner')
                        self.assertEqual(len(trip['side_capture_checks']), 6 if quality == 'ultra' else 4)
                        self.assertEqual(trip['park_cab_pose']['clock_seconds'], trip['route_end']['clock_seconds'])
                    else:
                        self.assertGreaterEqual(trip['route_end']['route_distance_m'], 300)
                    if quality == 'ultra':
                        self.assertEqual([s['step_index'] for s in trip['onfoot_trace']], list(range(16)))
                        self.assertAlmostEqual(trip['onfoot_trace'][-1]['simulation_advance_s'], .5)
                        self.assertTrue(all(s['state']['area'] == 'diner' and not s['state']['driving'] for s in trip['onfoot_trace']))
                    else: self.assertEqual(trip['onfoot_trace'], [])

    async def test_early_body_wrong_preset_or_changed_geometry_fail_closed(self):
        for mode, message in (('goto', 'code hash'), ('wrong_preset', 'player default'),
                ('changed_route', 'route changed')):
            with self.subTest(mode=mode):
                trip, browser, failure = await self.run_fake(mode=mode)
                self.assertIsNotNone(failure); self.assertIn(message, str(failure))
                self.assertEqual(trip['status'], 'FAIL')
                self.assertEqual(browser.contexts[0].page.screenshots, 1 if mode == 'changed_route' else 0)
                self.assertTrue(browser.contexts[0].closed)

    async def test_close_response_console_http_and_drain_response_revert_captured(self):
        for mode in ('close', 'close_console', 'close_http', 'close_nested_body', 'close_error'):
            with self.subTest(mode=mode):
                trip, browser, failure = await self.run_fake('dinerlight_departure', mode)
                self.assertIsNotNone(failure); self.assertEqual(trip['status'], 'FAIL')
                self.assertIn('cleanup_failure', trip)
                self.assertGreater(browser.contexts[0].page.screenshots, 0)
                if mode == 'close_http': self.assertEqual(trip['failed_http'][0]['status'], 500)
                if mode in ('close', 'close_nested_body'): self.assertTrue(trip['errors'])
                if mode == 'close_error':
                    self.assertNotIn('code_observation_closed', trip)
                    self.assertEqual(trip['observed_code_responses'][-1]['hash_state'], 'cancelled')
                else: self.assertTrue(trip['code_observation_closed'])

    async def test_missing_arrival_transition_callback_or_headlights_off_block(self):
        for mode, message in (('missing_transition', 'parking/getOut'), ('missing_callback', 'Chapter5 callback'),
                              ('parked_lights_on', 'headlights were not off'),
                              ('prepark_lights_off', 'no active headlights')):
            with self.subTest(mode=mode):
                trip, _, failure = await self.run_fake(mode=mode)
                self.assertIsNotNone(failure); self.assertIn(message, str(failure))
                self.assertEqual(trip['status'], 'FAIL')
                self.assertFalse(any(s['name'] == 'arrival_end_parked' for s in trip['shots']))

    async def test_held_look_pose_drift_or_failed_restoration_is_blocking(self):
        for mode in ('look_drift', 'camera_restore', 'visibility_restore'):
            with self.subTest(mode=mode):
                trip, _, failure = await self.run_fake(mode=mode)
                self.assertIsNotNone(failure); self.assertIn('Diner look changed', str(failure))
                self.assertEqual(trip['status'], 'FAIL')
                self.assertEqual(len(trip['side_capture_checks']), 1)

    async def test_cleanup_failure_never_hides_the_initial_failure(self):
        # Body failure at goto occurs before shots; closing also fails independently.
        browser = DinerBrowser('goto')
        original_new = browser.new_context
        async def new_context(**kwargs):
            ctx = await original_new(**kwargs)
            async def failing_close():
                ctx.page.events['response'](hash_checks.Response(gate=asyncio.Event()))
                raise OSError('independent late cleanup failure')
            ctx.close = failing_close
            return ctx
        browser.new_context = new_context
        report = {'trips': [], 'status': 'CAPTURING', 'failures': [], 'source': {'git_head': 'mock'}}
        with tempfile.TemporaryDirectory() as tmp, patch('sys.stdout', new=io.StringIO()):
            with self.assertRaisesRegex(RuntimeError, 'code hash'):
                await capture.run_dinerlight_trip(browser, SimpleNamespace(size=(844, 390), quality='high'),
                                                 report, 'dinerlight_arrival', 'http://test/', Path(tmp))
            saved = json.loads((Path(tmp) / 'manifest.json').read_text())['trips'][0]
        self.assertIn('code hash', saved['failure'])
        self.assertIn('independent late cleanup failure', saved['cleanup_failure'])
        self.assertFalse(browser.contexts[0].page.events)
        self.assertEqual(saved['observed_code_responses'][-1]['hash_state'], 'cancelled')

    async def test_actual_capture_dispatch_uses_two_fresh_contexts_even_after_failed_arrival(self):
        for mode in ('', 'prepark_lights_off'):
            with self.subTest(mode=mode):
                browser = DinerBrowser(mode)
                class MockChromium:
                    async def launch(self, **_): return browser
                class MockPlaywright:
                    async def __aenter__(self): return SimpleNamespace(chromium=MockChromium())
                    async def __aexit__(self, *_): return False
                module = ModuleType('playwright.async_api')
                module.async_playwright = MockPlaywright
                package = ModuleType('playwright'); package.__path__ = []
                report = {'trips': [], 'status': 'CAPTURING', 'failures': [],
                          'requested_trips': ['dinerlight_arrival', 'dinerlight_departure'],
                          'source': {'git_head': 'mock'}}
                with tempfile.TemporaryDirectory() as tmp, patch('sys.stdout', new=io.StringIO()), \
                        patch.dict(sys.modules, {'playwright': package, 'playwright.async_api': module}), \
                        patch.dict(os.environ, {'S47_URL': 'http://test/'}):
                    await capture.capture(SimpleNamespace(size=(844, 390), quality='high'), report, Path(tmp))
                self.assertEqual([t['id'] for t in report['trips']], report['requested_trips'])
                self.assertEqual([t['status'] for t in report['trips']],
                                 ['FAIL' if mode else 'CAPTURED', 'CAPTURED'])
                self.assertEqual(len(report['failures']), 1 if mode else 0)
                self.assertEqual(len(browser.contexts), 2)
                self.assertIsNot(browser.contexts[0].page, browser.contexts[1].page)
                self.assertTrue(browser.closed)
                self.assertTrue(all(c.closed and not c.page.events for c in browser.contexts))
                self.assertTrue(all(o['device_scale_factor'] == 1 and not o['has_touch']
                                    and not o['is_mobile'] for o in browser.options))


class JavascriptCameraContracts(unittest.TestCase):
    def test_actual_held_helpers_restore_visibility_render_time_and_protected_state(self):
        # Executes only the helper strings against tiny API doubles, never Three.js
        # or a game. Assertions inspect actual helper effects, not source substrings.
        node = os.environ.get('S47_NODE') or shutil.which('node')
        if not node:
            bundled = Path.home() / '.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
            if bundled.is_file(): node = str(bundled)
        self.assertTrue(node, 'Node is required for JS contract execution; set S47_NODE')
        payload = {'look': capture.DINERLIGHT_LOOK, 'restore': capture.DINERLIGHT_RESTORE,
                   'draw': capture.DINERLIGHT_DRAW}
        harness = r'''
const assert=require('node:assert/strict'), fs=require('node:fs');
const src=JSON.parse(fs.readFileSync(0,'utf8'));
class V {constructor(...a){this.a=a} toArray(){return [...this.a]}
  fromArray(a){this.a=[...a];return this} clone(){return new V(...this.a)}}
function env() {
  const camera={position:new V(10,2,30),quaternion:new V(0,0,0,1),
    lookAt(){this.quaternion.fromArray([0,1,0,0])},updateMatrixWorld(){}};
  const truck={group:{position:new V(100,0,4),quaternion:new V(0,0,0,1)},
    driving:false,cab:{visible:false},shell:{visible:true}};
  const heads={slots:[0],set:{pos:[new V(100,1,4,0)],col:[new V(0,0,0)]}};
  const D={pos:new V(100,0,4),heading:1,speed:0,truck,area:{headlights:heads}};
  const world={leg:null,truckAt:'diner',drive:D,driving:false,
    diner:{proxies:{window:{getWorldPosition:v=>v.fromArray([120,3,4])}}}};
  const s={hold:true,world,camera,player:{pos:new V(105,1.7,8)},
    vhs:{render(_scene,_camera,t){s.renderTimes.push(t)}},renderTimes:[],draws:[],
    game:{clock:19001,d:{view:{draw(){
      s.draws.push({cab:truck.cab.visible,shell:truck.shell.visible,
        camera:camera.position.toArray(),driving:truck.driving});
      s.vhs.render(null,camera,9999);
      if(s.throwDraw)throw Error('forced draw failure');
    }}}}};
  const helper=k=>new Function('S47','return ('+src[k]+');')(s);
  const state=()=>({camera:camera.position.toArray(),q:camera.quaternion.toArray(),
    player:s.player.pos.toArray(),truck:D.truck.group.position.toArray(),
    driver:D.pos.toArray(),clock:s.game.clock,cab:truck.cab.visible,
    shell:truck.shell.visible,driving:truck.driving,worldDriving:world.driving,
    headlights:heads.set.pos[0].toArray(),colour:heads.set.col[0].toArray()});
  return {s,D,truck,heads,state,look:helper('look'),restore:helper('restore'),draw:helper('draw')};
}
let cases=0;
{
  const e=env(), original=e.state(), render=e.s.vhs.render;
  const saved=e.look({proxy:'window',cab:{camera_xyz:[100,2,4],camera_quaternion:[0,0,0,1]},time:123});
  assert.equal(saved.before.cab_visible,false);assert.equal(saved.before.shell_visible,true);
  assert.equal(saved.after.cab_visible,true);assert.equal(saved.after.shell_visible,false);
  assert.deepEqual(e.s.draws[0],{cab:true,shell:false,camera:[100,2,4],driving:false});
  assert.deepEqual(e.s.renderTimes,[123]);assert.equal(e.s.vhs.render,render);
  assert.deepEqual(saved.before.headlights,saved.after.headlights);
  const proof=e.restore(saved);
  assert.equal(proof.unchanged,true);assert.equal(proof.camera_restored,true);
  assert.equal(proof.visibility_restored,true);assert.deepEqual(e.state(),original);
  assert.deepEqual(e.s.renderTimes,[123,123]);assert.equal(e.s.vhs.render,render);cases++;
}
{
  const e=env(), original=e.state();
  const saved=e.look({proxy:'window',cab:null,time:51});
  assert.equal(saved.after.cab_visible,false);assert.equal(saved.after.shell_visible,true);
  assert.equal(e.restore(saved).unchanged,true);assert.deepEqual(e.state(),original);cases++;
}
for(const mutation of [e=>e.s.game.clock++,e=>e.s.player.pos.a[0]++,
  e=>e.D.pos.a[2]++,e=>e.truck.group.position.a[0]++,e=>e.truck.driving=true,
  e=>e.heads.set.pos[0].a[3]=1,e=>e.heads.set.col[0].a[0]=1]) {
  const e=env(), saved=e.look({proxy:'window',cab:null,time:12});mutation(e);
  assert.equal(e.restore(saved).unchanged,false);cases++;
}
{
  const e=env(), original=e.state(), render=e.s.vhs.render;e.s.throwDraw=true;
  assert.throws(()=>e.look({proxy:'window',cab:{camera_xyz:[100,2,4],camera_quaternion:[0,0,0,1]},time:12}),/forced draw/);
  assert.deepEqual(e.state(),original);assert.equal(e.s.vhs.render,render);cases++;
}
{
  const e=env(), original=e.state(), render=e.s.vhs.render;
  const saved=e.look({proxy:'window',cab:{camera_xyz:[100,2,4],camera_quaternion:[0,0,0,1]},time:12});
  e.s.throwDraw=true;assert.throws(()=>e.restore(saved),/forced draw/);
  assert.deepEqual(e.state(),original);assert.equal(e.s.vhs.render,render);cases++;
}
{
  const e=env(), original=e.state(), render=e.s.vhs.render;
  assert.equal(e.draw(42).render_time_s,42);assert.deepEqual(e.s.renderTimes,[42]);
  assert.deepEqual(e.state(),original);assert.equal(e.s.vhs.render,render);
  e.s.hold=false;assert.throws(()=>e.draw(43),/held loop/);
  assert.deepEqual(e.s.renderTimes,[42]);cases++;
}
console.log(JSON.stringify({status:'PASS',cases,evidence_kind:'helper_API_mocks',rendering:'UNVERIFIED'}));
'''
        result = subprocess.run([node, '-e', harness], input=json.dumps(payload), text=True,
                                capture_output=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stderr)
        checked = json.loads(result.stdout)
        self.assertEqual(checked['status'], 'PASS'); self.assertEqual(checked['cases'], 12)
        self.assertEqual(checked['rendering'], 'UNVERIFIED')


class ImportAndCliContracts(unittest.TestCase):
    def test_module_import_does_not_import_or_launch_playwright(self):
        import builtins
        original = builtins.__import__
        def guarded(name, *args, **kwargs):
            if name == 'playwright' or name.startswith('playwright.'):
                raise AssertionError('Import attempted to load a browser dependency')
            return original(name, *args, **kwargs)
        with patch('builtins.__import__', side_effect=guarded):
            module = load('dinerlight_import_guard', CAPTURE_PATH)
        self.assertTrue(callable(module.run_dinerlight_trip))

    def test_cli_accepts_both_sizes_and_all_presets(self):
        for size in ('844x390', '1280x800'):
            for quality in ('low', 'high', 'ultra'):
                with self.subTest(size=size, quality=quality):
                    args = capture.parse_args(['/tmp/unused', size, '--trip', 'dinerlight', '--quality', quality])
                    self.assertEqual(args.trip, 'dinerlight')
                    self.assertEqual(args.quality, quality)
                    self.assertEqual(args.size, tuple(map(int, size.split('x'))))

    def run_main_with(self, fake_capture, out):
        with patch.object(capture, 'capture', fake_capture), \
                patch.object(capture, 'source_info', return_value={'git_head': 'mock-source'}), \
                patch('sys.stdout', new=io.StringIO()):
            code = capture.main([str(out), '844x390', '--trip', 'dinerlight', '--quality', 'high'])
        saved = json.loads((out / 'manifest.json').read_text()) if (out / 'manifest.json').exists() else None
        return code, saved

    def test_cli_requires_both_phases_and_rejects_duplicate_or_missing_phase(self):
        for ids, expected_code in ((['dinerlight_arrival', 'dinerlight_departure'], 0),
                                   (['dinerlight_arrival'], 1),
                                   (['dinerlight_arrival', 'dinerlight_arrival'], 1)):
            async def fake_capture(args, report, out):
                report['trips'] += [{'id': i, 'status': 'CAPTURED', 'shots': []} for i in ids]
            with self.subTest(ids=ids), tempfile.TemporaryDirectory() as tmp:
                code, saved = self.run_main_with(fake_capture, Path(tmp) / 'new')
                self.assertEqual(code, expected_code)
                self.assertEqual(saved['requested_trips'], ['dinerlight_arrival', 'dinerlight_departure'])
                self.assertEqual(saved['interval_m'], 40)
                self.assertEqual(saved['visual_review'], 'UNVERIFIED')
                self.assertEqual(saved['status'], 'CAPTURED' if expected_code == 0 else 'FAIL')

    def test_cli_failure_keeps_partial_report_and_existing_output_is_exclusive(self):
        async def failing(args, report, out):
            report['trips'].append({'id': 'dinerlight_arrival', 'status': 'FAIL', 'shots': []})
            raise RuntimeError('forced mock arrival failure')
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / 'new'; code, saved = self.run_main_with(failing, out)
            self.assertEqual(code, 1); self.assertEqual(saved['status'], 'FAIL')
            self.assertIn('forced mock arrival failure', ' '.join(saved['failures']))
            before = (out / 'manifest.json').read_bytes()
            async def forbidden(*_): raise AssertionError('Existing output was reused')
            code, _ = self.run_main_with(forbidden, out)
            self.assertEqual(code, 2); self.assertEqual((out / 'manifest.json').read_bytes(), before)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--result', type=Path)
    args = parser.parse_args(argv)
    stream = io.StringIO()
    suite = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    report = {'checked_utc': datetime.now(timezone.utc).isoformat(),
              'status': 'PASS' if result.wasSuccessful() else 'FAIL',
              'evidence_kind': 'mock_and_contract_tests', 'browser_launched': False,
              'rendered_lighting': 'UNVERIFIED', 'gameplay_integration': 'UNVERIFIED',
              'hardware_performance': 'UNVERIFIED',
              'scope': 'Pure route/quality/ledger contracts and fake-page/CLI failure paths; no game or browser capture.',
              'tests_run': result.testsRun, 'failures': len(result.failures),
              'errors': len(result.errors), 'output': stream.getvalue(),
              'files_sha256': {str(p.relative_to(WEB)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in (CAPTURE_PATH, HASH_CHECK_PATH, Path(__file__))}}
    if args.result:
        args.result.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(stream.getvalue(), end='')
    print(f'{report["status"]}: {result.testsRun} mock/contract tests; rendering UNVERIFIED')
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    raise SystemExit(main())
