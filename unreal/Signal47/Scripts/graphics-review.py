#!/usr/bin/env python3
"""Short sequential packaged graphics proofs; every launch requires 10 GiB and an idle GPU.

Default: all 20 cameras in batches of four. Run once at --clock night and again at dawn.
Use --views for a short retake. Startup requires the actual world-start/visible-frame trace;
a historical package without that instrumentation is explicitly UNVERIFIED.
"""
import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import sys
import uuid
import zlib

from unreal_guard import require_idle, run_owned

VIEWS = ('control', 'crt', 'annex', 'corridor', 'lab-normal', 'lab-safe', 'lab-print',
         'yard', 'service-yard', 'lot', 'motel', 'room6', 'station', 'hut', 'highway',
         'diner-outside', 'diner', 'caliche', 'old-road', 'cab')
STATE_VIEWS = ('rack-online', 'rack-dark', 'indicators-online', 'indicators-dark',
               'receiver-online', 'receiver-dark', 'flashlight-off', 'flashlight-on',
               'headlights-off', 'headlights-on', 'menu-title', 'menu-pause',
               'menu-settings', 'menu-save', 'menu-load', 'menu-notebook',
               'terminal-online', 'terminal-dark')
ASSET_VIEWS = ('array-front', 'array-rear', 'array-field', 'radio-exterior', 'radio-lobby', 'radio-studio')
STREET_VIEWS = ('street-west-front', 'street-east-front', 'street-north-route', 'street-souvenir-window')
ALL_VIEWS = VIEWS + STATE_VIEWS + ASSET_VIEWS + STREET_VIEWS
CLOCKS = {'night': '02:15', 'dawn': '05:40', 'day': '12:00', 'dusk': '18:30'}


def review_clock(args):
    wall = CLOCKS.get(args.clock, args.clock)
    hours, minutes = (int(part) for part in wall.split(':'))
    seconds = hours * 3600 + minutes * 60
    timeline = getattr(args, 'timeline', 'legacy')
    if timeline == 'evening' and seconds >= 12 * 3600:
        seconds -= 86400
    return {'wall': wall, 'timeline': timeline, 'seconds': seconds}


def static_world(args, views):
    return bool(getattr(args, 'static_world', False) or getattr(args, 'suite', '') == 'street' or
                any(view in STREET_VIEWS for view in views))


def clock_evidence(log, args, views):
    expected = review_clock(args)
    expected['static'] = static_world(args, views)
    records = re.findall(r'S47: graphics clock request=(\d{2}:\d{2}) timeline=(legacy|evening) seconds=(-?\d+) static=([01])', log)
    actual = [{'wall': wall, 'timeline': timeline, 'seconds': int(seconds), 'static': held == '1'}
              for wall, timeline, seconds, held in records]
    problems = []
    if actual != [expected]:
        problems.append('runtime clock/timeline/static contract differs: ' + repr(actual))
    poses = [list(pose) for pose in re.findall(r'S47: graphics street pose (\S+) walkable=([01]) eyeHeight=([-\d.]+) clockSeconds=(-?\d+) timeline=(legacy|evening) cinematic=([01])', log)]
    wanted = [view for view in views if view in STREET_VIEWS]
    if [pose[0] for pose in poses] != wanted:
        problems.append('missing or duplicate street player-floor camera proof')
    for view, walkable, eye, seconds, timeline, cinematic in poses:
        if (walkable != '1' or not abs(float(eye) - 1.62) <= .08 or int(seconds) != expected['seconds'] or
                timeline != expected['timeline'] or (expected['static'] and cinematic != '1')):
            problems.append('invalid street player-floor/clock/static proof: ' + view)
    return {'complete': not problems, 'expected': expected, 'actual': actual,
            'street_poses': poses, 'problems': problems}


def partition_views(views, batch_size):
    """Keep global order, with a process boundary before returning to SARO's cab.

    The old-road view is19km away. Splitting before cab avoids that diagnostic
    teleport in one renderer; it changes neither cameras nor game-world settings.
    """
    if batch_size < 1:
        raise ValueError('batch_size must be positive')
    result, current = [], []
    for view in views:
        if current and (len(current) >= batch_size or view == 'cab'):
            result.append(current)
            current = []
        current.append(view)
    if current:
        result.append(current)
    return result


def fingerprint(path):
    sha = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b''):
            sha.update(data)
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha256': sha.hexdigest()}


def package_fingerprint(binary):
    result = fingerprint(binary)
    package = binary.parents[3]
    containers = sorted(p for p in package.rglob('*') if p.suffix in ('.pak', '.ucas', '.utoc'))
    if not containers:
        raise RuntimeError('Packaged content containers missing: ' + str(package))
    print(f'Fingerprinting executable and {len(containers)} packaged content files...', flush=True)
    result['containers'] = []
    for path in containers:
        item = fingerprint(path)
        item['path'] = str(path.relative_to(package))
        result['containers'].append(item)
    return result


def png_size(path):
    """Validate the PNG's chunks/CRCs, including a complete IDAT and final IEND."""
    with path.open('rb') as stream:
        if stream.read(8) != b'\x89PNG\r\n\x1a\n':
            raise ValueError(f'Invalid PNG signature: {path}')
        size = None
        compressed = bytearray()
        while True:
            header = stream.read(8)
            if len(header) != 8:
                raise ValueError(f'Truncated PNG: {path}')
            length, kind = struct.unpack('>I4s', header)
            if length > 128 * 1024 * 1024:
                raise ValueError(f'Invalid PNG chunk length: {path}')
            data = stream.read(length)
            checksum = stream.read(4)
            if len(data) != length or len(checksum) != 4 or struct.unpack('>I', checksum)[0] != zlib.crc32(kind + data):
                raise ValueError(f'Truncated PNG / CRC mismatch: {path}')
            if kind == b'IHDR':
                size = struct.unpack('>II', data[:8])
            elif kind == b'IDAT':
                compressed.extend(data)
            elif kind == b'IEND':
                if not size or not compressed or not zlib.decompress(compressed):
                    raise ValueError(f'PNG has no pixels: {path}')
                from PIL import Image
                with Image.open(path) as pixels:
                    if max(channel[1] for channel in pixels.convert('RGB').getextrema()) == 0:
                        raise ValueError(f'Entirely black screenshot (all RGB pixels zero): {path}')
                return size


def tour_evidence(out, views, clock, width, height, hold):
    problems, rows, images = [], [], []
    for view in views:
        path = out / (view + '.png')
        try:
            size = png_size(path)
            if size != (width, height):
                problems.append(f'{view}: expected {width}x{height}, got {size}')
            images.append(fingerprint(path))
        except (OSError, ValueError, zlib.error, struct.error) as error:
            problems.append(str(error))
    try:
        with (out / 'frames.csv').open(newline='') as stream:
            rows = list(csv.DictReader(stream))
        if [row.get('view') for row in rows] != list(views):
            problems.append('frames.csv must contain each expected camera exactly once, in order')
        for row in rows:
            count = int(row['samples'])
            seconds = float(row['sample_seconds'])
            mean = float(row['mean_ms'])
            if row['clock'] != clock or count < 30 or seconds < hold * .5 - .5:
                problems.append('incomplete/wrong-clock frame sample: ' + row['view'])
            if int(row['last_frame']) - int(row['first_frame']) + 1 != count:
                problems.append('missing frame IDs in sample: ' + row['view'])
            if not all(math.isfinite(float(row[k])) and float(row[k]) >= 0 for k in
                       ('mean_ms', 'p95_ms', 'game_ms', 'draw_ms', 'gpu_ms')) or mean <= 0:
                problems.append('invalid frame timing: ' + row['view'])
            if abs(float(row['frame_sum_ms']) - count * mean) > max(1, count * .001):
                problems.append('frame timing sum mismatch: ' + row['view'])
            row['fps'] = 1000 / mean if mean > 0 else None
            row['high_60fps_mean'] = mean <= 1000 / 60
            row['high_60fps_p95'] = float(row['p95_ms']) <= 1000 / 60
    except (OSError, KeyError, ValueError) as error:
        problems.append('missing/invalid frames.csv: ' + str(error))
    return {'complete': not problems, 'problems': problems, 'views': rows, 'images': images}


def resolution_evidence(out, log, width, height):
    problems = []
    sizes = re.findall(r'S47: smoke test OK[^\n]*viewport (\d+)x(\d+)', log)
    viewport = tuple(int(value) for value in sizes[-1]) if sizes else None
    if viewport != (width, height):
        problems.append(f'Expected {width}x{height} smoke viewport; got {viewport}')
    final = None
    try:
        image_path = out / 'final.png'
        dimensions = png_size(image_path)
        final = dict(fingerprint(image_path), dimensions=list(dimensions))
        if dimensions != (width, height):
            problems.append(f'Expected {width}x{height} final PNG; got {dimensions}')
    except (OSError, ValueError, zlib.error, struct.error) as error:
        problems.append('Missing/invalid final PNG: ' + str(error))
    return {'complete': not problems, 'expected': [width, height],
            'viewport': list(viewport) if viewport else None, 'final_png': final, 'problems': problems}


def request_spec(args, views):
    return {'mode': args.mode, 'views': list(views), 'clock': CLOCKS.get(args.clock, args.clock),
            'timeline': review_clock(args)['timeline'], 'clock_seconds': review_clock(args)['seconds'],
            'static_world': static_world(args, views),
            'quality': args.quality, 'picture': args.picture or ('off' if args.quality == 'low' else 'vhs'),
            'width': args.width, 'height': args.height, 'warmup': args.warmup, 'hold': args.hold,
            'seconds': args.seconds, 'cold': args.cold, 'extra': list(args.extra),
            'driver_cache': str(args.driver_cache.expanduser().resolve()) if args.driver_cache else None}


def reusable_leaf(args, out, views, build_id):
    """Only uncontended PASS leaves with unchanged inputs AND hashed artifacts can skip."""
    try:
        meta = json.loads((out / 'meta.json').read_text())
        if (meta.get('status') != 'PASS' or not meta.get('valid_evidence') or not meta.get('diagnostic_pass') or
                meta.get('contention') or meta.get('stop_reason') or meta.get('returncode') != 0 or
                meta.get('build') != build_id or meta.get('request') != request_spec(args, views)):
            return None
        log_path = out / 'run.log'
        if fingerprint(log_path) != meta.get('run_log'):
            return None
        resolution = resolution_evidence(out, log_path.read_text(errors='replace'), args.width, args.height)
        if not resolution['complete'] or resolution != meta.get('resolution'):
            return None
        if args.mode == 'tour':
            if clock_evidence(log_path.read_text(errors='replace'), args, views) != meta.get('clock_evidence'):
                return None
            capture = tour_evidence(out, views, CLOCKS.get(args.clock, args.clock), args.width, args.height, args.hold)
            capture['frame_file'] = fingerprint(out / 'frames.csv')
            if not capture['complete'] or capture != meta.get('capture'):
                return None
        elif fingerprint(out / 'startup.csv') != meta.get('startup_trace') or not meta.get('startup', {}).get('valid_evidence'):
            return None
        if '-logpso' in (value.lower() for value in args.extra):
            caches = sorted((out / 'profile').rglob('*.rec.upipelinecache'))
            if not caches or [fingerprint(path) for path in caches] != meta.get('pso_caches'):
                return None
        return meta
    except (OSError, ValueError, KeyError, TypeError, zlib.error, struct.error):
        return None


def quarantine_leaf(args, out):
    # Outside args.out: record's active-tree rglob must never ingest rejected evidence.
    root = args.out.expanduser().resolve()
    configured = getattr(args, 'quarantine_root', None)
    archive = configured.expanduser().resolve() if configured else root.parent / (root.name + '-rejected')
    archive.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    destination = archive / (out.name + '-' + stamp + '-' + uuid.uuid4().hex[:8])
    shutil.move(str(out), destination)
    print('PRESERVED rejected/incomplete leaf:', destination, flush=True)


def one_run(args, out, views, build_id):
    if out.exists() and any(out.iterdir()):
        if not getattr(args, 'resume', False):
            raise RuntimeError(f'Evidence directory must be empty (avoid stale PASS/screenshots): {out}')
        saved = reusable_leaf(args, out, views, build_id)
        if saved is not None:
            print('REUSE verified PASS:', out, flush=True)
            return saved
        if (out / 'meta.json').is_file():
            try:
                old = json.loads((out / 'meta.json').read_text())
            except (OSError, ValueError, TypeError):
                old = {}  # Interrupted writes are incomplete evidence, preserved below.
            if old.get('build') and old['build'] != build_id:
                raise RuntimeError('Resume package fingerprint differs; use a fresh evidence root')
        quarantine_leaf(args, out)
    out.mkdir(parents=True, exist_ok=True)
    clock = CLOCKS.get(args.clock, args.clock)
    picture = args.picture or ('off' if args.quality == 'low' else 'vhs')
    duration = args.seconds or (25 if args.mode == 'startup' else args.warmup + args.hold * len(views) + 5)
    if args.mode == 'tour' and duration < args.warmup + args.hold * len(views) + 3:
        raise RuntimeError('--seconds is too short for the requested camera batch')
    command = [build_id['path'], 'Signal47', '-UserDir=' + str(out / 'profile'), '-unattended',
               '-NoCrashDialog', '-RenderOffscreen', '-windowed', '-ForceRes', '-ResX=' + str(args.width),
               '-ResY=' + str(args.height), '-S47SmokeTest', '-S47Jump=chapter1',
               '-S47Quality=' + args.quality, '-S47Picture=' + picture, '-S47Seconds=' + str(duration),
               '-S47Face=room', '-ExecCmds=r.VSync 0,t.MaxFPS 0', '-S47Shot=' + str(out / 'final.png')]
    if args.mode == 'tour':
        command += ['-S47GraphicsReview=' + str(out), '-S47GraphicsClock=' + clock,
                    '-S47GraphicsWarmup=' + str(args.warmup), '-S47GraphicsHold=' + str(args.hold),
                    '-S47GraphicsViews=' + ','.join(views),
                    '-S47GraphicsTimeline=' + review_clock(args)['timeline']]
        if review_clock(args)['timeline'] == 'evening':
            command += ['-S47EveningTimeline']
        if static_world(args, views):
            command += ['-S47GraphicsStatic']
    else:
        # CSV profiler is auxiliary only. It has no world/visible origin on old builds.
        command += ['-csvCaptureFrames=2400', '-S47StartupTrace=' + str(out / 'startup.csv')]
        if args.cold:
            command += ['-clearPSODriverCache']
    command += args.extra
    env = os.environ.copy()
    driver_cache = args.driver_cache.expanduser().resolve() if args.driver_cache else out / 'driver-cache'
    driver_cache.mkdir(parents=True, exist_ok=True)
    env['__GL_SHADER_DISK_CACHE_PATH'] = str(driver_cache)
    env['__GL_SHADER_DISK_CACHE'] = '1'
    print('START', str(out), 'views=' + ','.join(views), flush=True)
    meta = {'command': command, 'mode': args.mode, 'clock': clock if args.mode == 'tour' else 'chapter1-default', 'quality': args.quality,
            'picture': picture, 'build': build_id, 'expected_views': list(views),
            'request': request_spec(args, views),
            'driver_cache': str(driver_cache), 'shared_driver_cache': bool(args.driver_cache), 'cold': args.cold}
    try:
        meta.update(run_owned(command, out / 'run.log', timeout=duration + args.startup_timeout,
                              env=env, cwd=Path(build_id['path']).parent))
    except Exception as error:
        meta.update({'valid_evidence': False, 'status': 'BLOCKED', 'error': str(error)})
        (out / 'meta.json').write_text(json.dumps(meta, indent=2) + '\n')
        raise
    log = (out / 'run.log').read_text(errors='replace')
    meta['run_log'] = fingerprint(out / 'run.log')
    meta['pso_caches'] = [fingerprint(path) for path in sorted((out / 'profile').rglob('*.rec.upipelinecache'))]
    meta['runtime_errors'] = [marker for marker in ('Critical error:', 'Assertion failed:',
                              'Signal 11 caught', 'VK_ERROR_DEVICE_LOST') if marker in log]
    ok = (meta['returncode'] == 0 and not meta['stop_reason'] and
          'smoke test OK' in log and not meta['runtime_errors'])
    meta['smoke_pass'] = bool(ok)
    meta['resolution'] = resolution_evidence(out, log, args.width, args.height)
    ok = ok and meta['resolution']['complete']
    if args.mode == 'tour':
        meta['clock_evidence'] = clock_evidence(log, args, views)
        ok = ok and meta['clock_evidence']['complete']
        evidence = tour_evidence(out, views, clock, args.width, args.height, args.hold)
        meta['capture'] = evidence
        if (out / 'frames.csv').is_file():
            meta['capture']['frame_file'] = fingerprint(out / 'frames.csv')
        ok = ok and evidence['complete'] and 'graphics review complete' in log
    else:
        # Import only after dispatch to keep the CLI module side-effect free for offline validation.
        import importlib.util
        spec = importlib.util.spec_from_file_location('s47_pso_cache', Path(__file__).with_name('pso-cache.py'))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        meta['startup'] = module.summarize_trace(out / 'startup.csv')
        if (out / 'startup.csv').is_file():
            meta['startup_trace'] = fingerprint(out / 'startup.csv')
        ok = ok and meta['startup']['valid_evidence']
        auxiliary = sorted((out / 'profile').rglob('*.csv'))
        meta['auxiliary_csv'] = [str(x) for x in auxiliary]
    meta['diagnostic_pass'] = bool(ok)
    meta['valid_evidence'] = bool(ok and not meta['contention'])
    meta['performance_status'] = 'PASS' if meta['valid_evidence'] else 'UNVERIFIED'
    if meta['contention'] or not meta['resolution']['complete']:
        if args.mode == 'tour':
            for row in meta['capture']['views']:
                row['performance_status'] = 'UNVERIFIED'
                row['diagnostic_fps'], row['fps'] = row.get('fps'), None
                row['high_60fps_mean'] = row['high_60fps_p95'] = None
        else:
            meta['startup'].update({'valid_evidence': False,
                                    'status': 'FAIL' if not meta['smoke_pass'] else ('CONTENDED' if meta['contention'] else 'INVALID_RESOLUTION'),
                                    'performance_status': 'UNVERIFIED'})
            if not meta['resolution']['complete']:
                meta['startup']['problems'].extend(meta['resolution']['problems'])
            for name in ('initial_10s', 'visible_10s'):
                if name in meta['startup']:
                    row = meta['startup'][name]
                    row['performance_status'] = 'UNVERIFIED'
                    row['diagnostic_fps'], row['fps'] = row.get('fps'), None
    missing_trace = args.mode == 'startup' and not (out / 'startup.csv').is_file()
    meta['status'] = ('CONTENDED' if meta['contention'] else 'PASS') if ok else ('UNVERIFIED' if missing_trace and not meta['contention'] else 'FAIL')
    if not meta['smoke_pass']:
        meta['status'] = 'FAIL'
    elif not meta['resolution']['complete']:
        meta['status'] = 'INVALID_RESOLUTION'
    (out / 'meta.json').write_text(json.dumps(meta, indent=2) + '\n')
    print('END', meta['status'], str(out), 'contention=' + str(len(meta['contention'])), flush=True)
    return meta


def run(args):
    out = args.out.expanduser().resolve()
    build = args.build.expanduser().resolve()
    binary = build / 'Linux/Signal47/Binaries/Linux/Signal47'
    if not binary.is_file():
        raise RuntimeError('Packaged executable missing: ' + str(binary))
    suite = {'all': ALL_VIEWS, 'states': STATE_VIEWS, 'street': STREET_VIEWS, 'world': VIEWS}[args.suite]
    views = args.views or ([args.view] if args.view else list(suite))
    if len(set(views)) != len(views):
        raise RuntimeError('Duplicate camera selection')
    require_idle()
    build_id = package_fingerprint(binary)
    groups = partition_views(views, args.batch_size)
    if args.mode == 'startup' or len(groups) == 1:
        meta = one_run(args, out, views if args.mode == 'tour' else [], build_id)
        return 0 if meta['diagnostic_pass'] else 1
    out.mkdir(parents=True, exist_ok=True)
    root_request = dict(request_spec(args, views), batch_size=args.batch_size)
    if any(out.iterdir()) and not getattr(args, 'resume', False):
        raise RuntimeError('Evidence root must be empty: ' + str(out))
    if (out / 'meta.json').exists():
        previous = json.loads((out / 'meta.json').read_text())
        if previous.get('build') != build_id or previous.get('request') != root_request:
            raise RuntimeError('Resume root package/request differs; use a fresh output root')
    batches = []
    (out / 'meta.json').write_text(json.dumps({'status': 'INCOMPLETE', 'diagnostic_pass': False,
        'valid_evidence': False, 'build': build_id, 'request': root_request, 'expected_views': views, 'batches': []}, indent=2) + '\n')
    for index, chosen in enumerate(groups, 1):
        print(f'BATCH {index}/{len(groups)}: {", ".join(chosen)}', flush=True)
        result = one_run(args, out / f'batch-{index:02d}', chosen, build_id)
        batches.append(result)
        complete = all(x['diagnostic_pass'] for x in batches) and sum(len(x['expected_views']) for x in batches) == len(views)
        contended = any(x['contention'] for x in batches)
        manifest = {'status': ('CONTENDED' if contended else 'PASS') if complete else 'INCOMPLETE',
                    'diagnostic_pass': complete, 'valid_evidence': complete and not contended,
                    'performance_status': 'PASS' if complete and not contended else 'UNVERIFIED',
                    'build': build_id, 'request': root_request, 'expected_views': views, 'batches': batches}
        (out / 'meta.json').write_text(json.dumps(manifest, indent=2) + '\n')
        if not result['diagnostic_pass']:
            return 1
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--mode', choices=['tour', 'startup'], default='tour')
    ap.add_argument('--clock', default='night', help='night (02:15), dawn (05:40), day (12:00), dusk (18:30) or HH:MM')
    ap.add_argument('--timeline', choices=['legacy', 'evening'], default='legacy',
                    help='explicit diagnostic clock axis; evening bootstraps the signed evening timeline')
    ap.add_argument('--static-world', action='store_true',
                    help='hold autonomous narrative events with the cinematic guard; automatic for street cameras')
    ap.add_argument('--quality', choices=['low', 'high', 'ultra'], default='high')
    ap.add_argument('--picture', choices=['off', 'vhs'], help='default: Low clean; High/Ultra VHS')
    ap.add_argument('--suite', choices=['world', 'states', 'street', 'all'], default='world',
                    help='world: original20; states: power/lights/menus; street: four public Caliche player-height views')
    selection = ap.add_mutually_exclusive_group()
    selection.add_argument('--view', choices=ALL_VIEWS)
    selection.add_argument('--views', nargs='+', choices=ALL_VIEWS)
    ap.add_argument('--batch-size', type=int, default=4, choices=range(1, 5))
    ap.add_argument('--seconds', type=int, help='per process, excludes the loading gate; default is computed')
    ap.add_argument('--warmup', type=float, default=8)
    ap.add_argument('--hold', type=float, default=7)
    ap.add_argument('--startup-timeout', type=int, default=120)
    ap.add_argument('--width', type=int, default=2560)
    ap.add_argument('--height', type=int, default=1440)
    ap.add_argument('--cold', action='store_true')
    ap.add_argument('--resume', action='store_true', help='reuse only hash-verified matching PASS leaves; preserve/retry rejected leaves')
    ap.add_argument('--quarantine-root', type=Path, help='external archive for rejected leaves (used by PSO recording)')
    ap.add_argument('--driver-cache', type=Path, help='explicit same-build shared NVIDIA cache for split tours; forbidden with --cold')
    ap.add_argument('extra', nargs=argparse.REMAINDER)
    args = ap.parse_args()
    if args.extra[:1] == ['--']:
        args.extra = args.extra[1:]
    clock = CLOCKS.get(args.clock, args.clock)
    parts = clock.split(':')
    if len(parts) != 2 or not all(x.isdigit() for x in parts) or not (0 <= int(parts[0]) < 24 and 0 <= int(parts[1]) < 60):
        ap.error('--clock must be night, dawn, day, dusk or HH:MM')
    args.clock = f'{int(parts[0]):02d}:{int(parts[1]):02d}'
    if any(value.lower().split('=')[0] in ('-s47graphicstimeline', '-s47graphicsclock', '-s47graphicsstatic',
                '-s47eveningtimeline', '-s47evening', '-s47jump') for value in args.extra):
        ap.error('use --clock/--timeline/--static-world; extra may not override the review bootstrap')
    if args.mode != 'tour' and (args.timeline != 'legacy' or args.static_world):
        ap.error('--timeline/--static-world are tour diagnostics, not startup overrides')
    if args.hold < 4 or args.hold > 15 or args.warmup < 0 or args.warmup > 30 or not 10 <= args.startup_timeout <= 180:
        ap.error('use hold 4..15s, warmup 0..30s and startup timeout 10..180s')
    if args.seconds is not None and not 12 <= args.seconds <= 120:
        ap.error('--seconds must be 12..120 for a short test')
    if args.driver_cache and (not args.driver_cache.is_absolute() or args.cold):
        ap.error('--driver-cache must be absolute and cannot be reused for --cold evidence')
    try:
        return run(args)
    except (RuntimeError, OSError, ValueError) as error:
        print('BLOCKED:', error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
