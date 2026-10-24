#!/usr/bin/env python3
"""Plan, or explicitly run, fixed nature pictures on an existing packaged build.

No engine build/import. Uses the existing PropsAt teleport and a graphics-clock
hold whose warmup exceeds the entire smoke run, so no named camera takes over.
Default is a read-only plan. Each --run output directory must be new.
"""
import argparse
import csv
from datetime import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import runpy
import struct
import sys
import zlib

from unreal_guard import require_idle, run_owned, GRACE_ENV

BASELINE = Path.home() / 'scratch-bm/Signal47-caliche-concept/candidate'
BASELINE_SHA = '6103dd97769d675de05c77bf112cf568e1fbb9536335a458c71b83e6986113af'
CLOCKS = {'day': '12:00', 'dusk': '18:30', 'night': '02:15'}
# World Three metres, yaw/pitch radians. Enter diner supplies the analytical town
# floor immediately; Enter road clears that floor hook in the historical package.
POSES = {
    'road-edge-long': (8004.8, 2150.0, .04, -.06),
    'valley': (8008.0, 2180.0, -.55, -.05),
    'roadside-close': (8005.4, 2110.0, -.9, -.35),
    'across-desert': (8018.0, 2105.0, -1.2, -.1),
    'outskirts': (8005.2, 2310.0, .02, -.08),
}
DEFAULT_VIEWS = ['road-edge-long', 'valley', 'roadside-close', 'across-desert']
PROJECT = Path(__file__).resolve().parents[1]
PERFORMANCE_WINDOW = [4.0, 10.0]


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def package(build):
    root = build / 'Linux'
    executable = root / 'Signal47/Binaries/Linux/Signal47'
    containers = sorted((root / 'Signal47/Content/Paks').glob('*'))
    if not executable.is_file() or not containers or not all(path.is_file() for path in containers):
        raise RuntimeError('Packaged executable/containers missing')
    identity = hashlib.sha256()
    rows = []
    for path in [executable] + containers:
        digest = hashlib.sha256()
        identity.update(path.name.encode())
        with path.open('rb') as stream:
            for block in iter(lambda: stream.read(8 * 1024 * 1024), b''):
                identity.update(block)
                digest.update(block)
        rows.append({'path': str(path.relative_to(root)), 'bytes': path.stat().st_size,
                     'sha256': digest.hexdigest()})
    return {'package_sha256': identity.hexdigest(), 'files': rows}


def png(path, width, height):
    raw = path.read_bytes()
    if raw[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Missing/invalid PNG: ' + str(path))
    offset = 8
    pixels = bytearray()
    size = None
    complete = False
    while offset + 12 <= len(raw):
        length, kind = struct.unpack_from('>I4s', raw, offset)
        if length > 128 * 1024 * 1024 or offset + length + 12 > len(raw):
            raise ValueError('Truncated PNG chunk')
        data = raw[offset + 8:offset + 8 + length]
        checksum = struct.unpack_from('>I', raw, offset + 8 + length)[0]
        if zlib.crc32(kind + data) & 0xffffffff != checksum:
            raise ValueError('PNG CRC mismatch')
        if kind == b'IHDR':
            size = struct.unpack_from('>II', data)
        elif kind == b'IDAT':
            pixels.extend(data)
        elif kind == b'IEND':
            complete = True
            break
        offset += length + 12
    if not complete or size != (width, height) or not pixels or not zlib.decompress(pixels):
        raise ValueError('Incomplete/wrong-resolution PNG')
    return {'path': str(path), 'width': width, 'height': height,
            'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def performance_evidence(path, log, props_at):
    """Existing startup trace, fixed visible4..10s; no camera takeover or GPU claim."""
    result = {'status': 'UNVERIFIED', 'metric_kind': 'Game-thread wall-frame interval; not isolated CPU work or GPU timing',
              'window_visible_s': PERFORMANCE_WINDOW, 'problems': [],
              'texture_streaming': 'UNMEASURED; this trace has no residency/budget counters',
              'streaming_warnings': [line for line in log.splitlines() if re.search(r'(texture|streaming).*(over.{0,20}budget|pool.{0,20}over|failed|out of memory)', line, re.I)],
              'device': re.findall(r'DeviceName:\s*([^\r\n]+)', log),
              'driver': re.findall(r'Driver Version:\s*([^\r\n]+)', log)}
    problems = result['problems']
    try:
        raw = path.read_bytes()
        result['trace'] = {'path': str(path), 'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}
        rows = list(csv.DictReader(raw.decode().splitlines()))
        samples = [row for row in rows if row['covered'] == '0' and PERFORMANCE_WINDOW[0] <= float(row['visible_s']) <= PERFORMANCE_WINDOW[1]]
        if len(samples) < 30 or not rows or max(float(row['visible_s']) for row in rows) < PERFORMANCE_WINDOW[1]:
            raise ValueError('No complete visible4..10s trace window with at least30frames')
        times = [float(row['visible_s']) for row in samples]
        ids = [int(row['frame']) for row in samples]
        if times[0] > PERFORMANCE_WINDOW[0] + .25 or times[-1] < PERFORMANCE_WINDOW[1] - .25 or any(a >= b for a, b in zip(times, times[1:])) or ids != list(range(ids[0], ids[-1] + 1)):
            problems.append('Missing/nonmonotonic trace samples within fixed timing window')
        durations = [float(row['frame_ms']) for row in samples]
        if any(not math.isfinite(value) or value <= 0 for value in durations):
            raise ValueError('Invalid frame intervals')
        queues = {key: max(int(row[key]) for row in samples) for key in ('pso_remaining', 'rhi_requests', 'material_jobs', 'shader_jobs')}
        result['maximum_compile_queues'] = queues
        result['shader_compile_in_window'] = bool(queues['shader_jobs'] or queues['material_jobs'])
        if any(queues.values()):
            problems.append('Shader/material/PSO work overlaps timing window')
        # The diagnostic teleport is delayed after READY. Prove it and the
        # held clock occur before the fixed window; do not time the old view.
        def timestamp(marker):
            records = [line for line in log.splitlines() if marker in line]
            if len(records) != 1:
                raise ValueError('Missing/duplicate performance origin: ' + marker)
            stamp = re.match(r'\[([^]]+)\]', records[0])
            if stamp is None:
                raise ValueError('Performance origin has no native wall timestamp')
            return datetime.strptime(stamp[1], '%Y.%m.%d-%H.%M.%S:%f')
        ready = timestamp('S47: startup READY after')
        if re.search(r'S47: startup READY[^\n]*bypass=0', log) is None:
            problems.append('Startup gate bypassed; timing origin is not comparable')
        result['props_at_visible_s'] = (timestamp('S47: props view at ' + props_at) - ready).total_seconds()
        result['clock_hold_visible_s'] = (timestamp('S47: graphics clock request=') - ready).total_seconds()
        if not all(0 <= result[key] < PERFORMANCE_WINDOW[0] - .25 for key in ('props_at_visible_s', 'clock_hold_visible_s')):
            problems.append('Requested pose/held clock was not established before sample window')
        ordered = sorted(durations)
        percentile = lambda fraction: ordered[min(len(ordered) - 1, math.ceil(fraction * len(ordered)) - 1)]
        mean = sum(durations) / len(durations)
        result.update({'samples': len(samples), 'first_frame': ids[0], 'last_frame': ids[-1],
                       'sample_seconds': times[-1] - times[0], 'mean_ms': mean, 'p95_ms': percentile(.95),
                       'p99_ms': percentile(.99), 'max_ms': max(durations), 'fps_from_mean_interval': 1000 / mean,
                       'high_60fps_mean': mean <= 1000 / 60, 'high_60fps_p95': percentile(.95) <= 1000 / 60})
        result['status'] = 'PASS' if not problems else 'UNVERIFIED'
    except (OSError, ValueError, KeyError, TypeError) as error:
        problems.append(str(error))
    return result


def compare_performance(before, after):
    rows, problems = [], []
    def indexed(report):
        result = {(row['clock'], row['view']): row for row in report['results']}
        if len(result) != len(report['results']):
            raise ValueError('Duplicate timing pose/clock')
        return result
    left, right = indexed(before), indexed(after)
    if set(left) != set(right):
        problems.append('Before/after must contain exactly the same pose/clock set')
    for key in sorted(set(left) & set(right)):
        a, b = left[key], right[key]
        pa, pb = a.get('performance', {}), b.get('performance', {})
        if a.get('status') != 'PASS' or b.get('status') != 'PASS' or pa.get('status') != 'PASS' or pb.get('status') != 'PASS':
            problems.append('Unverified timing capture: ' + '/'.join(key))
            continue
        if not a.get('settings') or a['settings'] != b.get('settings') or a['pose'] != b['pose'] or pa['window_visible_s'] != pb['window_visible_s'] or not pa.get('device') or pa['device'] != pb.get('device') or not pa.get('driver') or pa['driver'] != pb.get('driver'):
            problems.append('Unmatched settings/pose/window/device/driver: ' + '/'.join(key))
            continue
        rows.append({'clock': key[0], 'view': key[1], 'before_mean_ms': pa['mean_ms'], 'after_mean_ms': pb['mean_ms'],
                     'mean_ms_change': pb['mean_ms'] - pa['mean_ms'], 'before_p95_ms': pa['p95_ms'], 'after_p95_ms': pb['p95_ms'],
                     'p95_ms_change': pb['p95_ms'] - pa['p95_ms']})
    return {'status': 'PASS' if rows and not problems else 'UNVERIFIED', 'problems': problems, 'rows': rows,
            'meaning': 'Matched timing evidence, not an automatic performance acceptance threshold',
            'before_package': before['package']['package_sha256'], 'after_package': after['package']['package_sha256']}


def capture_settings(args, job):
    return {'quality': 'high', 'picture': 'off', 'resolution': [args.width, args.height],
            'timeline': args.timeline, 'wall_clock': job['wall_clock'], 'seconds': args.seconds,
            'performance_window_visible_s': PERFORMANCE_WINDOW if args.performance else None}


def command(args, clock, view):
    leaf = args.out / clock / view
    pose = POSES[view]
    at = 'diner:' + ';'.join(format(value, '.8g') for value in pose)
    argv = [str(args.build / 'Linux/Signal47/Binaries/Linux/Signal47'), 'Signal47',
            '-UserDir=' + str(leaf / 'profile'), '-unattended', '-NoCrashDialog',
            '-RenderOffscreen', '-windowed', '-ForceRes', f'-ResX={args.width}',
            f'-ResY={args.height}', '-S47SmokeTest', f'-S47Seconds={args.seconds}',
            '-S47Jump=chapter1', '-S47Face=room', '-S47Quality=high', '-S47Picture=off',
            '-ExecCmds=r.VSync 0,t.MaxFPS 0', '-S47Shot=' + str(leaf / 'view.png'),
            '-S47PropsAt=' + at, '-S47GraphicsReview=' + str(leaf / 'clock-hold'),
            '-S47GraphicsViews=highway', '-S47GraphicsWarmup=3600',
            '-S47GraphicsClock=' + CLOCKS[clock], '-S47GraphicsTimeline=' + args.timeline,
            '-S47GraphicsStatic', '-S47LifeLog']
    if args.timeline == 'evening':
        argv.append('-S47EveningTimeline')
    if args.performance:
        argv.append('-S47StartupTrace=' + str(leaf / 'startup.csv'))
    return {'view': view, 'clock': clock, 'wall_clock': CLOCKS[clock],
            'area': 'diner', 'pose': {'world_x': pose[0], 'world_z': pose[1],
                                    'yaw_radians': pose[2], 'pitch_radians': pose[3]},
            'props_at': at, 'out': str(leaf), 'command': argv}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=BASELINE)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--clocks', nargs='+', choices=list(CLOCKS), default=['day', 'dusk'])
    parser.add_argument('--views', nargs='+', choices=list(POSES), default=DEFAULT_VIEWS,
                        help='Default four original views; outskirts is an optional separate before/after pair')
    parser.add_argument('--timeline', choices=['legacy', 'evening'], default='legacy')
    parser.add_argument('--expect-package', default=BASELINE_SHA,
                        help='Required frozen package SHA256; set explicitly for a new candidate')
    parser.add_argument('--seconds', type=float, default=20)
    parser.add_argument('--width', type=int, default=2560)
    parser.add_argument('--height', type=int, default=1440)
    parser.add_argument('--run', action='store_true', help='launch sequential guarded captures; otherwise print plan only')
    parser.add_argument('--expect-nature', action='store_true', help='require twelve actual HISM components, exact placement counts/culls and cooked LOD screens1/.12/.025 per family')
    parser.add_argument('--nature-placement', type=Path, default=PROJECT / 'Tools/calichenature/ground/nature-placement.json')
    parser.add_argument('--performance', action='store_true', help='record existing startup trace; validate visible4..10s frame intervals without camera takeover')
    parser.add_argument('--compare', type=Path, help='compare timing evidence to a prior nature-review.json with exactly matched settings/poses/clocks')
    args = parser.parse_args()
    args.build = args.build.expanduser().resolve()
    args.out = args.out.expanduser().resolve()
    if args.compare and not args.performance:
        parser.error('--compare requires --performance')
    prior = json.loads(args.compare.read_text()) if args.compare else None
    placement_raw = args.nature_placement.read_bytes() if args.expect_nature else None
    placement = json.loads(placement_raw) if placement_raw is not None else None
    runtime_audit = runpy.run_path(str(PROJECT / 'Tools/calichenature/verify_unreal.py'), run_name='runtime_helpers')['nature_runtime_evidence']
    installed = (Path.home() / 'Games/Signal47').resolve()
    if args.build == installed or installed in args.build.parents:
        parser.error('Use a frozen scratch candidate, never the user installation')
    if args.out == args.build or args.build in args.out.parents:
        parser.error('Evidence must be outside the immutable package')
    if args.out.exists():
        parser.error('Preserve existing evidence; choose a new output directory')
    if len(set(args.clocks)) != len(args.clocks) or len(set(args.views)) != len(args.views):
        parser.error('Duplicate clock/view')
    if not 12 <= args.seconds <= 90 or args.width < 640 or args.height < 360:
        parser.error('Use 12..90 seconds and at least 640x360')
    if not re.fullmatch(r'[0-9a-f]{64}', args.expect_package):
        parser.error('Expected package must be an explicit SHA256')
    jobs = [command(args, clock, view) for clock in args.clocks for view in args.views]
    if prior is not None:
        prior_rows = {(row['clock'], row['view']): row for row in prior['results']}
        if len(prior_rows) != len(prior['results']) or set(prior_rows) != {(job['clock'], job['view']) for job in jobs}:
            parser.error('Comparison baseline must contain exactly the requested pose/clock set')
        for job in jobs:
            row = prior_rows[(job['clock'], job['view'])]
            if row.get('status') != 'PASS' or row.get('performance', {}).get('status') != 'PASS' or row.get('settings') != capture_settings(args, job) or row.get('pose') != job['pose']:
                parser.error('Comparison baseline has unmatched/unverified timing/settings/pose: ' + job['view'])
    if not args.run:
        print(json.dumps({'status': 'PLAN_ONLY', 'expected_package_sha256': args.expect_package,
                          'nature_runtime_required': args.expect_nature,
                          'cooked_lod_screens_required': args.expect_nature,
                          'performance_window_visible_s': PERFORMANCE_WINDOW if args.performance else None,
                          'image_count': len(jobs), 'jobs': jobs}, indent=2))
        return 0
    # Disable the optional installed-game grace: every capture requires strict idle.
    os.environ.pop(GRACE_ENV, None)
    require_idle()
    initial = package(args.build)
    if initial['package_sha256'] != args.expect_package:
        raise RuntimeError('Candidate differs from the requested frozen package')
    args.out.mkdir(parents=True, exist_ok=False)
    summary = {'status': 'INCOMPLETE', 'package': initial, 'results': [],
               'test_kind': 'Fixed eye-height diagnostic teleports; visual inspection UNVERIFIED',
               'limitations': 'Historical package logs the requested teleport, not final eye XYZ. This is not walking/input or FPS evidence.'}
    if placement_raw is not None:
        summary['nature_placement'] = {'path': str(args.nature_placement.resolve()), 'sha256': hashlib.sha256(placement_raw).hexdigest()}
    if args.performance:
        summary['limitations'] = 'Fixed diagnostic teleport, not walking/input evidence. Performance uses game-thread wall-frame intervals at visible4..10s; it has no isolated GPU timing or texture residency counters.'
    report_path = args.out / 'nature-review.json'
    report_path.write_text(json.dumps(summary, indent=2))
    for job in jobs:
        before = package(args.build)
        if before != initial:
            raise RuntimeError('Candidate changed before capture')
        leaf = Path(job['out'])
        leaf.mkdir(parents=True, exist_ok=False)
        result = run_owned(job['command'], leaf / 'runtime.log', timeout=180,
                           cwd=args.build / 'Linux/Signal47/Binaries/Linux')
        after = package(args.build)
        log = (leaf / 'runtime.log').read_text(errors='replace')
        seconds = int(CLOCKS[job['clock']][:2]) * 3600 + int(CLOCKS[job['clock']][3:]) * 60
        if args.timeline == 'evening' and seconds >= 43200:
            seconds -= 86400
        markers = [f'S47: props view at {job["props_at"]}',
                   f'S47: graphics clock request={job["wall_clock"]} timeline={args.timeline} seconds={seconds} static=1',
                   f'viewport {args.width}x{args.height}', 'S47: smoke test OK']
        problems = [f'Missing log marker: {marker}' for marker in markers if marker not in log]
        problems += [marker for marker in ('Critical error:', 'Assertion failed:', 'Signal 11 caught',
                     'invalid/duplicate graphics camera', 'S47: graphics camera ',
                     'S47: graphics captured ') if marker in log]
        if result['returncode'] or result['stop_reason'] or result['contention']:
            problems.append('Guarded capture failed or was contended')
        if after != initial:
            problems.append('Package changed during capture')
        nature = runtime_audit(log, placement, args.expect_nature)
        if nature['status'] not in ('PASS', 'NOT_REQUESTED'):
            problems.append('Native nature component telemetry failed')
        if placement_raw is not None and args.nature_placement.read_bytes() != placement_raw:
            problems.append('Authored placement changed during capture')
        performance = performance_evidence(leaf / 'startup.csv', log, job['props_at']) if args.performance else {'status': 'NOT_REQUESTED'}
        if args.performance and performance['status'] != 'PASS':
            problems.append('Requested frame timing evidence incomplete/contaminated')
        image = None
        try:
            image = png(leaf / 'view.png', args.width, args.height)
        except (OSError, ValueError, zlib.error, struct.error) as error:
            problems.append(str(error))
        row = dict(job, run=result, package_before=before['package_sha256'],
                   package_after=after['package_sha256'], image=image, problems=problems,
                   status='FAIL' if problems else 'PASS', visual_inspection='UNVERIFIED',
                   nature_runtime=nature, performance=performance,
                   runtime_log={'sha256': sha(leaf / 'runtime.log'), 'path': str(leaf / 'runtime.log')},
                   settings=capture_settings(args, job),
                   pose_evidence='Requested PropsAt marker and source floor contract; no final-eye telemetry in historical package')
        (leaf / 'report.json').write_text(json.dumps(row, indent=2))
        summary['results'].append(row)
        summary['status'] = 'FAIL' if problems else 'CAPTURES_COMPLETE_VISUAL_UNVERIFIED' if len(summary['results']) == len(jobs) else 'INCOMPLETE'
        report_path.write_text(json.dumps(summary, indent=2))
        print(job['clock'], job['view'], row['status'], flush=True)
        if problems:
            return 1
    if prior is not None:
        summary['performance_comparison'] = compare_performance(prior, summary)
        summary['comparison_source'] = {'path': str(args.compare.resolve()), 'sha256': sha(args.compare)}
        if summary['performance_comparison']['status'] != 'PASS':
            summary['status'] = 'FAIL'
        report_path.write_text(json.dumps(summary, indent=2))
        if summary['status'] == 'FAIL':
            return 1
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError) as error:
        print('BLOCKED:', error, file=sys.stderr)
        raise SystemExit(2)
