#!/usr/bin/env python3
"""Package explicit dinerlight captures without changing their raw evidence.

Example (a new target is mandatory):
  python3 package_evidence.py --captures shots/dinerlight_low_844x390_20261006 \
    --target production/drivelook_dinerlight_20261006/evidence_20261006 \
    --pages-tar /tmp/artifact.tar --expected-tar-sha256 SHA256 \
    --served-from https://tombonator3000.github.io/SIGNAL-47/ --require-all

PASS concerns recorded evidence integrity only. Rendered lighting, gameplay,
physical GPU/phone, audio and performance remain UNVERIFIED for manual review.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import html
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import shutil
import sys
import tarfile
import textwrap
from urllib.parse import unquote, urlsplit

from PIL import Image, ImageDraw, ImageFont

PRODUCTION_ROOT = Path(__file__).resolve().parent
WEB = PRODUCTION_ROOT.parent.parent
PHASES = ('dinerlight_arrival', 'dinerlight_departure')
QUALITIES = ('low', 'high', 'ultra')
SIZES = ((844, 390), (1280, 800))
EXPECTED_CONFIGS = {f'{quality}_{w}x{h}' for quality in QUALITIES for w, h in SIZES}
FONT = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
BG, FG, MUTED = '#151a20', '#dde4e8', '#a9b6bf'


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''): digest.update(block)
    return digest.hexdigest()


def dump(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n', encoding='utf-8')


def load_capture():
    spec = importlib.util.spec_from_file_location('dinerlight_package_capture', WEB / 'tools/drivelook.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def image_path(folder, name):
    if not isinstance(name, str) or not name.endswith('.png') or Path(name).name != name:
        raise ValueError(f'Unsafe PNG reference: {name!r}')
    path = folder / name
    if path.is_symlink() or path.resolve().parent != folder.resolve():
        raise ValueError(f'PNG reference leaves its capture: {name}')
    return path


def snapshot(folder):
    if folder.is_symlink() or not folder.is_dir(): raise ValueError(f'Not a regular capture directory: {folder}')
    files = {}
    for path in sorted(folder.rglob('*')):
        if path.is_symlink(): raise ValueError(f'Symlink in evidence: {path}')
        if path.is_file(): files[path.relative_to(folder).as_posix()] = sha(path)
        elif not path.is_dir(): raise ValueError(f'Non-regular evidence entry: {path}')
    return files


def artifact_index(path):
    """Read tar bytes without extracting, executing, or trusting archive paths."""
    files = {}
    with tarfile.open(path, 'r:*') as archive:
        for member in archive:
            name = member.name
            while name.startswith('./'): name = name[2:]
            if name in ('', '.'): continue
            relative = PurePosixPath(name)
            if relative.is_absolute() or '..' in relative.parts or '\\' in name:
                raise ValueError(f'Unsafe artifact member: {member.name}')
            if member.isdir(): continue
            if not member.isfile(): raise ValueError(f'Non-regular artifact member: {member.name}')
            name = relative.as_posix()
            if name in files: raise ValueError(f'Duplicate artifact member: {name}')
            stream = archive.extractfile(member)
            digest = hashlib.sha256()
            for block in iter(lambda: stream.read(1024 * 1024), b''): digest.update(block)
            files[name] = {'sha256': digest.hexdigest(), 'bytes': member.size}
    if 'index.html' not in files: raise ValueError('Pages tar has no index.html')
    return files


def resource_member(url, served_from):
    source, base = urlsplit(url), urlsplit(served_from)
    prefix = base.path.rstrip('/') + '/'
    if (source.scheme, source.netloc) != (base.scheme, base.netloc): return None
    if not source.path.startswith(prefix): return None
    relative = unquote(source.path[len(prefix):]) or 'index.html'
    p = PurePosixPath(relative)
    if p.is_absolute() or '..' in p.parts or '\\' in relative: return None
    return p.as_posix()


def camera_distance(state):
    """Recompute observed XZ camera/player separation, with no pose repair."""
    camera, player = state.get('camera_xyz'), state.get('player_xyz')
    if any(not isinstance(v, list) or len(v) != 3 or not all(isinstance(n, (int, float)) and math.isfinite(n) for n in v) for v in (camera, player)):
        raise ValueError('Missing/non-finite camera or player XYZ')
    recorded = state.get('camera_player_horizontal_m')
    measured = math.hypot(camera[0] - player[0], camera[2] - player[2])
    if not isinstance(recorded, (int, float)) or not math.isfinite(recorded) or not math.isclose(recorded, measured, abs_tol=1e-7):
        raise ValueError('Missing or inconsistent observed camera/player horizontal distance')
    return measured


def render_clock_errors(state, capture):
    """Require the raw clock inputs before using the capture's clock contract."""
    inputs, sky = state.get('sky_clock_inputs'), state.get('sky_uniforms')
    if not isinstance(inputs, dict) or not all(k in inputs for k in ('dawn_lift', 'sun_elev_deg')):
        return ['Missing explicit observed Sky clock inputs']
    if not isinstance(sky, dict) or not all(k in sky for k in ('uDawn', 'uSun', 'uSunDir')):
        return ['Missing observed Sky clock uniforms']
    return capture.dinerlight_render_clock_errors(state)


def validate(folder, manifest, artifact, served_from, capture):
    """Fail closed for missing inventories and all supplied image/phase records."""
    if not isinstance(manifest, dict): raise ValueError('Manifest must be a JSON object')
    trips = manifest.get('trips')
    if not isinstance(trips, list) or any(not isinstance(t, dict) for t in trips):
        raise ValueError('Manifest trips must be a list of phase objects')
    if any(not isinstance(t.get('shots'), list) or any(not isinstance(s, dict) for s in t['shots']) for t in trips):
        raise ValueError('Each phase must contain a list of image objects')
    checks, images, resources = [], [], []
    def check(ok, name, detail=None):
        checks.append({'check': name, 'status': 'PASS' if ok else 'FAIL', 'detail': detail})
    check(manifest.get('status') == 'CAPTURED' and not manifest.get('failures'), 'completed capture without failures')
    check(manifest.get('visual_review') == 'UNVERIFIED', 'capture makes no automatic visual verdict')
    check(manifest.get('requested_trips') == list(PHASES), 'both requested phases in order')
    check([t.get('id') for t in trips] == list(PHASES), 'exactly both actual phases in order')
    check(manifest.get('interval_m') == 40 and manifest.get('fps') == 30, '40 m schedule and 30 Hz physics')
    check(manifest.get('url') == served_from, 'explicit served build URL matches capture')
    quality = manifest.get('quality_requested')
    all_shots = [s for t in trips for s in t.get('shots', [])]
    sizes = {(s.get('viewport', {}).get('width'), s.get('viewport', {}).get('height')) for s in all_shots}
    size = next(iter(sizes)) if len(sizes) == 1 else None
    check(quality in QUALITIES and size in SIZES, 'single known quality and viewport', {'quality': quality, 'sizes': sorted(sizes, key=str)})
    config = f'{quality}_{size[0]}x{size[1]}' if quality in QUALITIES and size in SIZES else None
    check(bool(all_shots), 'image rows are present')
    check(len({s.get('file') for s in all_shots}) == len(all_shots), 'unique image filenames')
    listed = {s.get('file') for s in all_shots}
    check(listed == {p.name for p in folder.glob('*.png')}, 'every raw PNG has exactly one manifest row')
    for trip in trips:
        identity = trip.get('id'); shots = trip.get('shots', [])
        prefix = str(identity) + ': '
        check(trip.get('status') == 'CAPTURED', prefix + 'phase completed')
        check(all(isinstance(trip.get(k), list) for k in ('errors', 'warnings', 'console_messages', 'failed_http')), prefix + 'raw console/page/HTTP lists are present')
        check(trip.get('quality_requested') == quality and trip.get('visual_review') == 'UNVERIFIED', prefix + 'quality and visual scope')
        check(trip.get('route_placement_calls') == 0 and trip.get('clock_assignment_calls') == 0, prefix + 'no route placement or clock assignment')
        inventory = trip.get('observed_code_responses')
        check(trip.get('code_hash_policy') == 'observed_responses_v1' and isinstance(inventory, list) and bool(inventory), prefix + 'explicit document/script inventory is required')
        errors = capture.code_hash_errors(trip, require_closed=True)
        check(not errors and trip.get('code_observation_closed') is True and trip.get('code_observation_sealed') is True, prefix + 'complete closed code observation', errors)
        replay = {'errors': [], 'warnings': [], 'console_messages': []}
        for event in trip.get('console_messages', []):
            capture.record_console(replay, event['type'], event['text'], event.get('location', {}), page_url=served_from)
        check(not trip.get('errors') and not replay['errors'], prefix + 'strict console/page policy', {'recorded_errors': trip.get('errors'), 'replayed_errors': replay['errors'], 'warnings': trip.get('warnings')})
        try:
            capture.check_capture_diagnostics(trip, served_from, strict_http=True)
            check(True, prefix + 'strict HTTP policy')
        except (RuntimeError, KeyError, TypeError, ValueError) as error:
            check(False, prefix + 'strict HTTP policy', str(error))
        observed = inventory if isinstance(inventory, list) else []
        phase_resources = []
        for response in observed:
            url, digest = response.get('url'), response.get('sha256')
            member = resource_member(url, served_from)
            expected = artifact.get(member, {}).get('sha256')
            matched = response.get('hash_state') == 'hashed' and digest == expected and expected is not None
            row = {'phase': identity, 'url': url, 'artifact_file': member,
                   'captured_sha256': digest, 'artifact_sha256': expected, 'matches': matched}
            resources.append(row); phase_resources.append(row)
        check(bool(phase_resources) and all(r['matches'] for r in phase_resources), prefix + 'every observed document/script byte hash matches Pages tar', phase_resources)
        check(any(r['artifact_file'] == 'index.html' for r in phase_resources), prefix + 'document was observed')
        check(any((r['artifact_file'] or '').startswith('assets/Diner-') for r in phase_resources), prefix + 'Diner runtime chunk was observed')
        check(any((r['artifact_file'] or '').startswith('assets/OldRoad-') for r in phase_resources) if identity == PHASES[1] else True, prefix + 'departure OldRoad chunk was observed')
        geometry = trip.get('route_geometry', {})
        check(bool(geometry.get('points')) and bool(geometry.get('source_api')), prefix + 'loaded autopilot polyline and source API retained')
        route_phase = 'arrival_drive' if identity == PHASES[0] else 'departure_drive'
        route = [s for s in shots if s.get('capture_phase') == route_phase]
        thresholds = [v for s in route for v in s.get('interval_thresholds_m', []) if v != 300]
        check(thresholds == list(range(40, 281, 40)), prefix + 'all thresholds 40..280 once in order', thresholds)
        check(bool(route) and route[0].get('name') == 'route_start', prefix + 'first route image is the cab start')
        expected_names = ['route_start'] + (["approach_start_300m"] if identity == PHASES[0] else [])
        expected_names += [f'forward_{n:03d}m' for n in range(40, 281, 40)]
        expected_names += (['prepark_forward_lights_on', 'prepark_facade_lights_on', 'prepark_sign_lights_on',
                            'arrival_end_parked', 'parked_facade_lights_off', 'parked_sign_lights_off']
                           if identity == PHASES[0] else ['departure_end_300m'])
        if quality == 'ultra': expected_names += ['ultra_onfoot_after_0_5s', 'ultra_onfoot_facade', 'ultra_onfoot_sign']
        check([s.get('name') for s in shots] == expected_names, prefix + 'every required route/held-look/endpoint image appears exactly once in order')
        start, end = trip.get('route_start', {}), trip.get('route_end', {})
        pose_keys = ('map_xyz', 'camera_xyz', 'camera_quaternion', 'clock_seconds', 'route_distance_m')
        check(bool(route) and all(k in start and route[0].get(k) == start[k] for k in pose_keys), prefix + 'first PNG and route-start metadata have identical pose and clock')
        setup = trip.get('setup_sync', {})
        before, after = setup.get('before', {}), setup.get('after', {})
        check(type(setup.get('normal_frames')) is int and setup['normal_frames'] == 1
              and isinstance(setup.get('simulation_advance_s'), (int, float))
              and math.isclose(setup['simulation_advance_s'], 1 / 30, abs_tol=1e-8)
              and all(type(setup.get(k)) is int and setup[k] == 0 for k in
                      ('clock_assignment_calls', 'ui_assignment_calls', 'sky_assignment_calls', 'placement_calls')),
              prefix + 'setup render sync is one ordinary frame with zero assignments or placement')
        setup_pose = ('driver_xyz', 'heading_rad', 'chapter_phase', 'area', 'leg')
        check(setup.get('driver_pose_unchanged') is True
              and all(k in before and before[k] == after.get(k) for k in setup_pose),
              prefix + 'setup render sync preserves actual driver pose, chapter, area and leg')
        check(all(isinstance(st.get('clock_seconds'), (int, float)) and math.isfinite(st['clock_seconds']) for st in (before, after))
              and math.isclose(after['clock_seconds'] - before['clock_seconds'], 1 / 30, abs_tol=1e-7),
              prefix + 'ordinary setup frame advances observed game clock by exactly 1/30 s')
        for label, st in (('before', before), ('after', after)):
            setup_errors = capture.dinerlight_setup_errors(st)
            check(all(k in st for k in ('autopilot_mps', 'test_input_active', 'old_control_active'))
                  and not setup_errors and not capture.dinerlight_quality_errors(st, quality),
                  prefix + 'setup ' + label + ' is held cab at rest without autopilot/input overrides', setup_errors)
        expected_setup = ('ch5', 'saro', 'saro', 'chapter5_stage', 'to-truck') if identity == PHASES[0] else ('ch6', 'roswell', 'roswell', 'chapter6_stage', 'drive')
        check(all((st.get('chapter_phase'), st.get('area'), st.get('leg'), st.get(expected_setup[3]))
                  == (expected_setup[0], expected_setup[1], expected_setup[2], expected_setup[4]) for st in (before, after)),
              prefix + 'ordinary setup frame is in the requested ready chapter cab')
        clock_errors = render_clock_errors(after, capture)
        check(not clock_errors, prefix + 'setup after has observed HUD and Sky synchronized to game clock', clock_errors)
        start_keys = (*setup_pose, 'map_xyz', 'camera_xyz', 'camera_quaternion', 'clock_seconds', 'hud_clock', 'sky_clock_inputs')
        clock_sky = ('uDawn', 'uSun', 'uSunDir')
        check(bool(route) and all(k in after and after[k] == start.get(k) == route[0].get(k) for k in start_keys)
              and all(isinstance(st.get('sky_uniforms'), dict) for st in (after, start, route[0]))
              and all(k in after.get('sky_uniforms', {}) and after['sky_uniforms'][k] == start.get('sky_uniforms', {}).get(k)
                      == route[0].get('sky_uniforms', {}).get(k) for k in clock_sky)
              and start.get('autopilot_mps') == route[0].get('autopilot_mps') == 20,
              prefix + 'setup sync end agrees with actual first PNG and route start before any driving tick')
        check(all(s.get('held') is True and s.get('driving') is True for s in route), prefix + 'actual route is held cab driving')
        check(all(b.get('clock_seconds', -1) >= a.get('clock_seconds', 0) and b.get('route_distance_m', -1) >= a.get('route_distance_m', 0) for a, b in zip(route, route[1:])), prefix + 'clock and driven path do not regress')
        for shot in shots:
            name = shot.get('file')
            clock_errors = render_clock_errors(shot, capture)
            check(not clock_errors, prefix + 'observed HUD and Sky clock agree for PNG: ' + str(name), clock_errors)
            check(not capture.dinerlight_quality_errors(shot, quality), prefix + 'effective default preset/DPR/held: ' + str(name), capture.dinerlight_quality_errors(shot, quality))
            check(shot.get('preset') == {'quality': quality, 'picture': 'off' if quality == 'low' else 'vhs'}, prefix + 'recorded player-default preset: ' + str(name))
            render = shot.get('render_state', {})
            check(size is not None and (render.get('buffer_width'), render.get('buffer_height')) == size and shot.get('viewport', {}).get('device_scale_factor') == 1, prefix + 'actual buffer and viewport: ' + str(name))
            check(shot.get('post', {}).get('on') is (quality != 'low') and shot.get('post', {}).get('ultra') is (quality == 'ultra'), prefix + 'effective postprocessing: ' + str(name))
            check(shot.get('source_git_head') == manifest.get('source', {}).get('git_head'), prefix + 'local source baseline is consistently labelled: ' + str(name))
            pose = [shot.get(k) for k in ('map_xyz', 'camera_xyz', 'camera_quaternion', 'view_direction')]
            check(all(isinstance(v, list) and len(v) == n and all(isinstance(x, (int, float)) and math.isfinite(x) for x in v) for v, n in zip(pose, (3, 3, 4, 3))) and all(isinstance(shot.get(k), (int, float)) and math.isfinite(shot[k]) for k in ('clock_seconds', 'render_time_s', 'route_distance_m')), prefix + 'finite observed pose/timing: ' + str(name))
            try:
                point = [shot['map_xyz'][i] + shot['road_origin'][i] for i in range(3)]
                remaining = capture.dinerlight_route_remaining(geometry['points'], point)
                check(math.isclose(remaining['remaining_m'], shot['route_remaining_m'], abs_tol=1e-7), prefix + 'recomputed remaining polyline distance: ' + str(name))
                separation = camera_distance(shot)
                check(True, prefix + 'recomputed observed camera/player distance: ' + str(name), separation)
                if shot.get('capture_phase') in ('natural_parked_onfoot', 'ultra_onfoot_diagnostic'):
                    check(separation <= .1, prefix + 'actual foot camera is at observed player: ' + str(name), separation)
                found = capture.dinerlight_light_findings(shot['body_light_ledger'])
                check(found == shot.get('light_findings'), prefix + 'mechanical candidates reproduce without visual verdict: ' + str(name))
            except (ValueError, KeyError, TypeError, IndexError) as error:
                check(False, prefix + 'route/light metadata: ' + str(name), str(error))
            try:
                path = image_path(folder, name); digest = sha(path)
                with Image.open(path) as image:
                    actual_size, format_name = image.size, image.format; image.verify()
                images.append({'phase': identity, 'file': name, 'sha256': digest, 'size': list(actual_size)})
                check(digest == shot.get('image_sha256') and actual_size == size and format_name == 'PNG', prefix + 'lossless PNG hash and dimensions: ' + str(name))
            except (OSError, ValueError, TypeError) as error:
                check(False, prefix + 'lossless PNG: ' + str(name), str(error))
        side_checks = trip.get('side_capture_checks', [])
        expected_sides = (['prepark_facade_lights_on', 'prepark_sign_lights_on', 'parked_facade_lights_off', 'parked_sign_lights_off'] if identity == PHASES[0] else []) + (['ultra_onfoot_facade', 'ultra_onfoot_sign'] if quality == 'ultra' else [])
        check([s.get('name') for s in side_checks] == expected_sides and all(s.get('unchanged') is True and s.get('camera_restored') is True and s.get('visibility_restored') is True for s in side_checks), prefix + 'all held looks restore camera, visibility, truck, player, clock and headlights')
        by_name = {s.get('name'): s for s in shots}
        for side in side_checks:
            image = by_name.get(side.get('name'), {})
            view = image.get('view_setup', {}); before, after = view.get('before', {}), view.get('after', {})
            protected = ('driver', 'truck', 'truck_q', 'heading', 'speed', 'clock', 'player', 'truck_driving', 'headlights')
            check(bool(image) and all(k in before and before[k] == after.get(k) for k in protected), prefix + 'held-look before/after preserves protected state: ' + str(side.get('name')))
            proof = side.get('after', {})
            check(all(k in before and proof.get(k) == before[k] for k in ('camera', 'camera_q', 'cab_visible', 'shell_visible', 'truck_driving', 'clock')), prefix + 'restoration metadata agrees with actual pre-look state: ' + str(side.get('name')))
            if str(side.get('name')).startswith('parked_'):
                check(image.get('capture_phase') == 'parked_cab_camera_diagnostic' and view.get('kind') == 'diagnostic_parked_cab_from_observed_arrival'
                      and view.get('observed_cab_pose') == trip.get('park_cab_pose') and image.get('headlights_active') is False and image.get('driving') is False
                      and after.get('cab_visible') is True and after.get('shell_visible') is False, prefix + 'parked cab visibility override and camera provenance are explicitly diagnostic: ' + side['name'])
        if identity == PHASES[0]:
            approach = trip.get('approach_start', {})
            check(approach.get('leg') == 'road' and 286 <= approach.get('route_remaining_m', -1) <= 300, prefix + 'actual approach starts within final 300 m')
            prepark = trip.get('prepark_observed_pose', {})
            check(prepark.get('driving') is True and prepark.get('headlights_active') is True and 0 <= prepark.get('route_remaining_m', -1) <= 6 and abs(prepark.get('speed_mps', 99)) < .35, prefix + 'actual pre-park cab with headlights on')
            parked = [s for s in shots if s.get('name') == 'arrival_end_parked']
            check(len(parked) == 1 and parked[0].get('area') == 'diner' and parked[0].get('driving') is False and parked[0].get('headlights_active') is False and parked[0].get('chapter5_arrived') is True and parked[0].get('chapter5_stage') == 'diner', prefix + 'natural parked player/Chapter5 callback and headlights off')
            sync, callback = trip.get('park_camera_sync', {}), trip.get('park_callback_state', {})
            frames = sync.get('normal_tick_frames')
            check(isinstance(frames, int) and not isinstance(frames, bool) and 0 <= frames <= 30
                  and isinstance(sync.get('simulation_advance_s'), (int, float)) and math.isclose(sync['simulation_advance_s'], frames / 30, abs_tol=1e-8)
                  and sync.get('horizontal_tolerance_m') == .1 and sync.get('camera_assignment_calls') == 0 and sync.get('player_assignment_calls') == 0,
                  prefix + 'park camera sync records normal ticks and zero pose assignments')
            sync_keys = ('map_xyz', 'player_xyz', 'camera_xyz', 'camera_quaternion', 'clock_seconds')
            check(bool(callback) and all(k in callback and sync.get('before', {}).get(k) == callback[k] for k in sync_keys), prefix + 'park sync begins at preserved actual callback state')
            check(len(parked) == 1 and all(k in parked[0] and sync.get('after', {}).get(k) == parked[0][k] for k in sync_keys), prefix + 'parked foot PNG agrees with sync end pose and clock')
            try:
                check(camera_distance(sync['after']) <= .1, prefix + 'park sync ends at actual player camera within 0.1 m')
                check(math.isclose(camera_distance(sync['before']), camera_distance(callback), abs_tol=1e-7), prefix + 'callback camera separation is retained without repair')
            except (ValueError, KeyError, TypeError) as error:
                check(False, prefix + 'park camera synchronization proof', str(error))
            check(end.get('driving') is False and end.get('truck_at') == 'diner', prefix + 'natural route park frame retained')
            check(trip.get('park_cab_pose', {}).get('clock_seconds') == end.get('clock_seconds') and 'clock_seconds' in end, prefix + 'parked camera provenance is the actual park frame')
            for shot in route:
                if shot.get('interval_thresholds_m') and shot.get('name', '').startswith('forward_'):
                    check(all(0 <= 300 - shot['route_remaining_m'] - v <= 14 for v in shot['interval_thresholds_m']), prefix + '40 m threshold uses nearby actual polyline pose: ' + shot['name'])
        elif identity == PHASES[1]:
            check(end.get('driving') is True and end.get('chapter_phase') == 'ch6' and end.get('chapter6_stage') == 'drive' and end.get('area') == 'roswell' and 300 <= end.get('route_distance_m', -1) <= 314, prefix + 'actual Chapter6 route reaches its first 300 m')
            check(sum(s.get('name') == 'departure_end_300m' for s in shots) == 1, prefix + 'departure endpoint image exists once')
        trace = trip.get('onfoot_trace', [])
        check([s.get('step_index') for s in trace] == list(range(16)) if quality == 'ultra' else trace == [], prefix + 'only Ultra has immediate plus fifteen-frame foot trace')
        if quality == 'ultra':
            check(all(math.isclose(s.get('simulation_advance_s', -1), i / 30, abs_tol=1e-8) and s.get('state', {}).get('area') == 'diner' and s.get('state', {}).get('driving') is False for i, s in enumerate(trace)), prefix + 'foot trace records diner player at exact 1/30 s increments')
            check(sum(s.get('name') == 'ultra_onfoot_after_0_5s' for s in shots) == 1, prefix + 'Ultra foot image after 0.5 simulated seconds')
            for row in trace:
                st = row.get('state', {})
                trace_label = prefix + 'foot trace ' + str(row.get('step_index'))
                check(not capture.dinerlight_quality_errors(st, quality), trace_label + ' effective preset/DPR/held')
                check(all(isinstance(st.get(k), list) and len(st[k]) == n and all(isinstance(v, (int, float)) and math.isfinite(v) for v in st[k]) for k, n in (('map_xyz', 3), ('camera_xyz', 3), ('camera_quaternion', 4), ('player_xyz', 3)))
                      and isinstance(st.get('clock_seconds'), (int, float)) and math.isfinite(st['clock_seconds']), trace_label + ' finite observed pose/clock')
                try:
                    ledger = st['body_light_ledger']
                    separation = camera_distance(st)
                    check(separation <= .1 if row.get('step_index') == 15 else True, trace_label + 'measured camera/player distance; only final frame must be synced', separation)
                    check(ledger.get('on_foot_diner') is True and ledger.get('player_xyz') == st.get('player_xyz')
                          and capture.dinerlight_light_findings(ledger) == st.get('light_findings'), trace_label + ' complete raw ledger and reproducible mechanical candidates')
                except (ValueError, KeyError, TypeError, IndexError) as error:
                    check(False, trace_label + ' complete raw ledger', str(error))
    return {'config': config, 'quality': quality, 'size': list(size) if size else None,
            'checks': checks, 'images': images, 'loaded_code': resources,
            'integrity_status': 'PASS' if checks and all(c['status'] == 'PASS' for c in checks) else 'FAIL'}


def esc(value): return html.escape(str(value), quote=True)
def font(size): return ImageFont.truetype(str(FONT), size) if FONT.is_file() else ImageFont.load_default()


def label(shot):
    value = shot.get('clock_seconds', 0)
    clock = f'{int(value // 3600):02d}:{int(value % 3600 // 60):02d}:{value % 60:04.1f}'
    xyz = ', '.join(f'{v:.2f}' for v in shot['map_xyz'])
    return (f'{shot["name"]} | {shot["capture_phase"]} | {clock} | map XYZ [{xyz}] | '
            f'path {shot["route_distance_m"]:.2f} m | remaining {shot["route_remaining_m"]:.2f} m | '
            f'{shot["quality"]}/{shot["picture"]} | headlights {shot["headlights_active"]}')


def contact(folder, shots, target, title):
    width, height, columns = 360, 225 if shots[0]['viewport']['height'] == 800 else 166, 3
    cellw, cellh = width + 24, height + 105
    panel = Image.new('RGB', (columns * cellw, math.ceil(len(shots) / columns) * cellh + 65), BG)
    draw = ImageDraw.Draw(panel)
    draw.text((12, 9), title, fill=FG, font=font(18))
    draw.text((12, 36), 'Deriverte miniatyrer. Tapsfrie originaler bevart. Visuell vurdering UNVERIFIED.', fill=MUTED, font=font(12))
    for i, shot in enumerate(shots):
        x, y = i % columns * cellw + 12, i // columns * cellh + 63
        with Image.open(image_path(folder, shot['file'])) as original:
            image = original.convert('RGB'); image.thumbnail((width, height), Image.Resampling.LANCZOS)
        panel.paste(image, (x + (width - image.width) // 2, y + (height - image.height) // 2))
        for line_index, line in enumerate(textwrap.wrap(label(shot), 48)[:5]):
            draw.text((x, y + height + 4 + line_index * 17), line, fill=FG, font=font(11))
    panel.save(target, format='PNG')
    return {'file': target.name, 'sha256': sha(target), 'width': panel.width, 'height': panel.height,
            'evidence_kind': 'derived_contact_sheet', 'originals': [s['file'] for s in shots]}


def html_page(title, body):
    return ('<!doctype html><html lang="nb"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">'
            f'<title>{esc(title)}</title><style>body{{background:{BG};color:{FG};font:16px system-ui;margin:24px auto;padding:0 20px;max-width:1400px}}'
            f'a{{color:#a9cfe9}}figure{{margin:20px 0}}img{{max-width:100%;height:auto}}figcaption{{color:{MUTED};font-size:13px}}'
            '.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:20px}code{overflow-wrap:anywhere}</style>' + body + '</html>')


def gallery(raw, derived, manifest, config):
    derived.mkdir()
    body = (f'<h1>{esc(config)}</h1><p>Capture-integritet står i den samlede capture_checks.json. Visuell lysvurdering UNVERIFIED. '
            'Råmanifest og bilder er kopiert byteidentisk. Kapitteloppsett, holdte kameraer og Ultra-fotdiagnoser merkes separat.</p>'
            '<p><a href="../../index.html">Samlet oversikt</a> | '
            f'<a href="../../captures/{esc(raw.name)}/manifest.json">Uendret råmanifest</a></p>')
    assets = []
    for trip in manifest['trips']:
        body += f'<h2>{esc(trip["id"])}</h2>'
        route = [s for s in trip['shots'] if s['capture_phase'] in ('arrival_drive', 'departure_drive') and s['view_setup']['kind'] == 'actual_current_camera']
        diagnostics = [s for s in trip['shots'] if s not in route]
        for group, shots in (('kjort-rute', route), ('merkede-diagnoser', diagnostics)):
            for offset in range(0, len(shots), 12):
                name = f'{trip["id"]}-{group}-{offset // 12 + 1:02d}.png'
                assets.append(contact(raw, shots[offset:offset + 12], derived / name, f'{config}: {trip["id"]}, {group}'))
                body += f'<figure><a href="{esc(name)}"><img src="{esc(name)}" loading="lazy"></a><figcaption>{esc(group)}. Originaler under.</figcaption></figure>'
        body += '<h3>Tapsfrie originalbilder og faktisk metadata</h3>'
        for shot in trip['shots']:
            link = f'../../captures/{raw.name}/{shot["file"]}'
            body += f'<figure><a href="{esc(link)}"><img src="{esc(link)}" loading="lazy"></a><figcaption>{esc(label(shot))}</figcaption></figure>'
    (derived / 'index.html').write_text(html_page(config, body), encoding='utf-8')
    return assets


def overview(target, report):
    body = (f'<h1>Lys ved dineren</h1><p>{esc(report["status"])}: {report["completed_configs"]}/6 konfigurasjoner pakket med capture-integritet PASS. '
            'Ingen automatisk visuell eller gameplay PASS.</p><p><a href="capture_checks.json">Alle integritetskontroller</a> | '
            '<a href="evidence_manifest.json">Pakkemanifest og råfilhasher</a> | <a href="pages_artifact_index.json">Pages-tar og filhasher</a> | '
            '<a href="../review.md">Manuell review hvis skrevet</a> | <a href="../review.json">Manuell review-JSON hvis skrevet</a></p>')
    for entry in report['entries']:
        body += f'<h2>{esc(entry.get("config") or entry["input_name"])}</h2><p>Integritet {esc(entry["integrity_status"])}. <a href="captures/{esc(entry["package_name"])}/manifest.json">Råmanifest</a>'
        if entry.get('gallery'): body += f' | <a href="{esc(entry["gallery"])}">Kontaktark og originalbilder</a>'
        if entry.get('failure'): body += ' | ' + esc(entry['failure'])
        body += '</p>'
    body += '<h2>Fasade før og etter naturlig parkering</h2><p>Ulike spillklokker og lysstatus oppgis. Kamera etter parkering er en merket diagnose med gjenopprettet synlighet.</p><div class="grid">'
    for entry in report['entries']:
        for shot in entry.get('comparison_shots', []):
            link = f'captures/{entry["package_name"]}/{shot["file"]}'
            body += f'<figure><a href="{esc(link)}"><img src="{esc(link)}" loading="lazy"></a><figcaption>{esc(entry["config"] + " | " + label(shot))}</figcaption></figure>'
    body += '</div><p>Dette kontrollerer observerte kodebytes, råbilder og metadata. Lyskandidater er mekaniske data, ikke en visuell dom. Lokal source-head er separat fra artefaktets kodebytes. Ekte GPU, fysisk telefon, manuell kjøring, lyd og fps er UNVERIFIED.</p>'
    (target / 'index.html').write_text(html_page('Dinerlys: opptaksbevis', body), encoding='utf-8')


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=Path, nargs='+', required=True)
    parser.add_argument('--target', type=Path, required=True)
    parser.add_argument('--pages-tar', type=Path, required=True)
    parser.add_argument('--expected-tar-sha256', required=True)
    parser.add_argument('--served-from', required=True)
    parser.add_argument('--require-all', action='store_true')
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv); target = args.target.expanduser().resolve()
    captures = [path.expanduser().resolve() for path in args.captures]
    if (not target.is_relative_to(PRODUCTION_ROOT.resolve()) or target == PRODUCTION_ROOT.resolve()
            or any(target.is_relative_to(source) or source.is_relative_to(target) for source in captures)):
        print('FAIL: target must be a new child of the owned production directory, outside every input', flush=True)
        return 2
    try: target.mkdir(parents=True, exist_ok=False)
    except OSError as error:
        print(f'FAIL: refusing existing/unavailable target: {error}', flush=True); return 2
    (target / 'captures').mkdir(); (target / 'derived').mkdir()
    report = {'schema_version': 1, 'created_utc': datetime.now(timezone.utc).isoformat(),
              'status': 'PACKAGING', 'integrity_scope': 'Images, metadata and complete observed code inventory against supplied Pages tar',
              'visual_status': 'UNVERIFIED', 'gameplay_status': 'UNVERIFIED', 'hardware_performance': 'UNVERIFIED',
              'expected_configs': 6, 'completed_configs': 0, 'entries': [], 'failures': [],
              'served_from': args.served_from, 'packager_sha256': sha(Path(__file__)),
              'local_source_to_artifact_relationship': 'UNVERIFIED; recorded source baseline is retained separately from matched build bytes'}
    artifact = {}
    artifact_report = {'tar_path': str(args.pages_tar.resolve()), 'expected_tar_sha256': args.expected_tar_sha256,
                       'trust_scope': 'Caller supplies tar from separately digest-validated GitHub artifact; this packer checks this tar hash and loaded code bytes.'}
    try:
        if not re.fullmatch(r'[0-9a-f]{64}', args.expected_tar_sha256): raise ValueError('Malformed expected tar SHA-256')
        artifact_report['tar_sha256'] = sha(args.pages_tar)
        if artifact_report['tar_sha256'] != args.expected_tar_sha256: raise ValueError('Supplied Pages tar SHA-256 differs from expected digest')
        base = urlsplit(args.served_from)
        if base.scheme not in ('http', 'https') or not base.netloc or not base.path.endswith('/') or base.query or base.fragment:
            raise ValueError('served-from must be an explicit HTTP(S) directory URL')
        artifact = artifact_index(args.pages_tar); artifact_report.update(status='PASS', files=artifact)
        if sha(args.pages_tar) != artifact_report['tar_sha256']: raise ValueError('Pages tar changed while reading')
    except (OSError, ValueError, tarfile.TarError) as error:
        artifact_report.update(status='FAIL', failure=f'{type(error).__name__}: {error}')
        report['failures'].append({'scope': 'Pages artifact', 'error': artifact_report['failure']})
    dump(target / 'pages_artifact_index.json', artifact_report)
    helper = None
    try:
        helper = load_capture(); report['capture_helper_sha256'] = sha(WEB / 'tools/drivelook.py')
    except Exception as error:
        report['failures'].append({'scope': 'capture helper', 'error': f'{type(error).__name__}: {error}'})
    seen = set()
    for i, source in enumerate(captures, 1):
        package_name = f'{i:02d}-' + re.sub(r'[^A-Za-z0-9_.-]', '_', source.name)
        raw = target / 'captures' / package_name
        entry = {'input_path': str(source), 'input_name': source.name, 'package_name': package_name, 'integrity_status': 'FAIL', 'visual_status': 'UNVERIFIED'}
        report['entries'].append(entry)
        try:
            before = snapshot(source)
            shutil.copytree(source, raw)
            entry['raw_files_sha256'] = before
            if snapshot(raw) != before or snapshot(source) != before: raise ValueError('Raw evidence changed during copying')
            def reject_nonfinite(value): raise ValueError(f'Non-finite JSON value: {value}')
            manifest = json.loads((raw / 'manifest.json').read_text(encoding='utf-8'), parse_constant=reject_nonfinite)
            if helper is None: raise ValueError('Capture validation helper unavailable')
            checked = validate(raw, manifest, artifact, args.served_from, helper); entry.update(checked)
            if artifact_report['status'] != 'PASS': entry['integrity_status'] = 'FAIL'
            config = entry.get('config')
            if config in seen:
                entry['checks'].append({'check': 'configuration supplied exactly once', 'status': 'FAIL', 'detail': config})
                entry['integrity_status'] = 'FAIL'
            if config: seen.add(config)
            entry['originals_unchanged'] = snapshot(source) == before and snapshot(raw) == before
            if not entry['originals_unchanged']: raise ValueError('Raw evidence changed while validating')
            if entry['integrity_status'] == 'PASS':
                derived = target / 'derived' / package_name
                entry['derived_assets'] = gallery(raw, derived, manifest, config)
                entry['gallery'] = f'derived/{package_name}/index.html'
                entry['comparison_shots'] = [s for t in manifest['trips'] for s in t['shots'] if s['name'] in ('prepark_facade_lights_on', 'parked_facade_lights_off')]
                if snapshot(source) != before or snapshot(raw) != before:
                    entry['originals_unchanged'] = False
                    raise ValueError('Raw evidence changed while generating derived gallery')
            else: report['failures'].append({'input': str(source), 'failed_checks': [c for c in entry.get('checks', []) if c['status'] == 'FAIL']})
        except Exception as error:
            entry['integrity_status'] = 'FAIL'; entry['failure'] = f'{type(error).__name__}: {error}'
            report['failures'].append({'input': str(source), 'error': entry['failure']})
        print(f'{source.name}: integrity {entry["integrity_status"]}', flush=True)
    report['completed_configs'] = sum(e['integrity_status'] == 'PASS' for e in report['entries'])
    report['missing_configs'] = sorted(EXPECTED_CONFIGS - seen)
    report['status'] = 'FAIL' if report['failures'] else 'PACKAGED' if seen == EXPECTED_CONFIGS else 'PARTIAL'
    if args.require_all and report['status'] == 'PARTIAL':
        report['status'] = 'FAIL'; report['failures'].append({'scope': 'coverage', 'missing_configs': report['missing_configs']})
    # These files are new derived evidence; raw copies remain byte-identical.
    dump(target / 'evidence_manifest.json', report)
    dump(target / 'capture_checks.json', {'status': report['status'], 'visual_status': 'UNVERIFIED',
          'scope': report['integrity_scope'], 'failures': report['failures'],
          'entries': [{k: e.get(k) for k in ('package_name', 'config', 'integrity_status', 'checks', 'failure')} for e in report['entries']]})
    overview(target, report)
    print(f'{report["status"]}: {report["completed_configs"]}/6; {target / "index.html"}', flush=True)
    return 1 if report['status'] == 'FAIL' else 0


if __name__ == '__main__': raise SystemExit(main())
