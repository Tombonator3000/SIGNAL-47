#!/usr/bin/env python3
"""Short sequential packaged graphics proofs; every launch requires 10 GiB and an idle GPU.

Default: all 20 cameras in batches of four. Run once at --clock night and again at dawn.
Use --views for a short retake. Startup requires the actual world-start/visible-frame trace;
a historical package without that instrumentation is explicitly UNVERIFIED.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import sys
import zlib

from unreal_guard import require_idle, run_owned

VIEWS = ('control', 'crt', 'annex', 'corridor', 'lab-normal', 'lab-safe', 'lab-print',
         'yard', 'service-yard', 'lot', 'motel', 'room6', 'station', 'hut', 'highway',
         'diner-outside', 'diner', 'caliche', 'old-road', 'cab')
CLOCKS = {'night': '02:15', 'dawn': '05:40'}


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


def one_run(args, out, views, build_id):
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        raise RuntimeError(f'Evidence directory must be empty (avoid stale PASS/screenshots): {out}')
    clock = CLOCKS.get(args.clock, args.clock)
    picture = args.picture or ('off' if args.quality == 'low' else 'vhs')
    duration = args.seconds or (25 if args.mode == 'startup' else args.warmup + args.hold * len(views) + 5)
    if args.mode == 'tour' and duration < args.warmup + args.hold * len(views) + 3:
        raise RuntimeError('--seconds is too short for the requested camera batch')
    command = [build_id['path'], 'Signal47', '-UserDir=' + str(out / 'profile'), '-unattended',
               '-NoCrashDialog', '-RenderOffscreen', '-windowed', '-ResX=' + str(args.width),
               '-ResY=' + str(args.height), '-S47SmokeTest', '-S47Jump=chapter1',
               '-S47Quality=' + args.quality, '-S47Picture=' + picture, '-S47Seconds=' + str(duration),
               '-S47Face=room', '-ExecCmds=r.VSync 0,t.MaxFPS 0', '-S47Shot=' + str(out / 'final.png')]
    if args.mode == 'tour':
        command += ['-S47GraphicsReview=' + str(out), '-S47GraphicsClock=' + clock,
                    '-S47GraphicsWarmup=' + str(args.warmup), '-S47GraphicsHold=' + str(args.hold),
                    '-S47GraphicsViews=' + ','.join(views)]
    else:
        # CSV profiler is auxiliary only. It has no world/visible origin on old builds.
        command += ['-csvCaptureFrames=2400', '-S47StartupTrace=' + str(out / 'startup.csv')]
        if args.cold:
            command += ['-clearPSODriverCache']
    command += args.extra
    env = os.environ.copy()
    env['__GL_SHADER_DISK_CACHE_PATH'] = str(out / 'driver-cache')
    print('START', str(out), 'views=' + ','.join(views), flush=True)
    meta = {'command': command, 'mode': args.mode, 'clock': clock if args.mode == 'tour' else 'chapter1-default', 'quality': args.quality,
            'picture': picture, 'build': build_id, 'expected_views': list(views)}
    try:
        meta.update(run_owned(command, out / 'run.log', timeout=duration + args.startup_timeout,
                              env=env, cwd=Path(build_id['path']).parent))
    except Exception as error:
        meta.update({'valid_evidence': False, 'status': 'BLOCKED', 'error': str(error)})
        (out / 'meta.json').write_text(json.dumps(meta, indent=2) + '\n')
        raise
    log = (out / 'run.log').read_text(errors='replace')
    ok = (meta['returncode'] == 0 and not meta['contention'] and not meta['stop_reason'] and
          'smoke test OK' in log and not any(x in log for x in
          ('Critical error:', 'Assertion failed:', 'Signal 11 caught')))
    if args.mode == 'tour':
        evidence = tour_evidence(out, views, clock, args.width, args.height, args.hold)
        meta['capture'] = evidence
        ok = ok and evidence['complete'] and 'graphics review complete' in log
    else:
        # Import only after dispatch to keep the CLI module side-effect free for offline validation.
        import importlib.util
        spec = importlib.util.spec_from_file_location('s47_pso_cache', Path(__file__).with_name('pso-cache.py'))
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        meta['startup'] = module.summarize_trace(out / 'startup.csv')
        ok = ok and meta['startup']['valid_evidence']
        auxiliary = sorted((out / 'profile').rglob('*.csv'))
        meta['auxiliary_csv'] = [str(x) for x in auxiliary]
    meta['valid_evidence'] = bool(ok)
    missing_trace = args.mode == 'startup' and not (out / 'startup.csv').is_file()
    meta['status'] = 'PASS' if ok else ('UNVERIFIED' if missing_trace and not meta['contention'] else 'FAIL')
    (out / 'meta.json').write_text(json.dumps(meta, indent=2) + '\n')
    print('END', meta['status'], str(out), 'contention=' + str(len(meta['contention'])), flush=True)
    return meta


def run(args):
    out = args.out.expanduser().resolve()
    build = args.build.expanduser().resolve()
    binary = build / 'Linux/Signal47/Binaries/Linux/Signal47'
    if not binary.is_file():
        raise RuntimeError('Packaged executable missing: ' + str(binary))
    views = args.views or ([args.view] if args.view else list(VIEWS))
    if len(set(views)) != len(views):
        raise RuntimeError('Duplicate camera selection')
    require_idle()
    build_id = package_fingerprint(binary)
    if args.mode == 'startup' or len(views) <= args.batch_size:
        meta = one_run(args, out, views if args.mode == 'tour' else [], build_id)
        return 0 if meta['valid_evidence'] else 1
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        raise RuntimeError('Evidence root must be empty: ' + str(out))
    batches = []
    for offset in range(0, len(views), args.batch_size):
        chosen = views[offset:offset + args.batch_size]
        print(f'BATCH {offset//args.batch_size+1}/{math.ceil(len(views)/args.batch_size)}: {", ".join(chosen)}', flush=True)
        result = one_run(args, out / f'batch-{offset//args.batch_size+1:02d}', chosen, build_id)
        batches.append(result)
        complete = all(x['valid_evidence'] for x in batches) and sum(len(x['expected_views']) for x in batches) == len(views)
        manifest = {'status': 'PASS' if complete else 'INCOMPLETE', 'valid_evidence': complete,
                    'build': build_id, 'expected_views': views, 'batches': batches}
        (out / 'meta.json').write_text(json.dumps(manifest, indent=2) + '\n')
        if not result['valid_evidence']:
            return 1
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--mode', choices=['tour', 'startup'], default='tour')
    ap.add_argument('--clock', default='night', help='night, dawn or HH:MM')
    ap.add_argument('--quality', choices=['low', 'high', 'ultra'], default='high')
    ap.add_argument('--picture', choices=['off', 'vhs'], help='default: Low clean; High/Ultra VHS')
    selection = ap.add_mutually_exclusive_group()
    selection.add_argument('--view', choices=VIEWS)
    selection.add_argument('--views', nargs='+', choices=VIEWS)
    ap.add_argument('--batch-size', type=int, default=4, choices=range(1, 5))
    ap.add_argument('--seconds', type=int, help='per process, excludes the loading gate; default is computed')
    ap.add_argument('--warmup', type=float, default=8)
    ap.add_argument('--hold', type=float, default=7)
    ap.add_argument('--startup-timeout', type=int, default=120)
    ap.add_argument('--width', type=int, default=2560)
    ap.add_argument('--height', type=int, default=1440)
    ap.add_argument('--cold', action='store_true')
    ap.add_argument('extra', nargs=argparse.REMAINDER)
    args = ap.parse_args()
    if args.extra[:1] == ['--']:
        args.extra = args.extra[1:]
    clock = CLOCKS.get(args.clock, args.clock)
    parts = clock.split(':')
    if len(parts) != 2 or not all(x.isdigit() for x in parts) or not (0 <= int(parts[0]) < 24 and 0 <= int(parts[1]) < 60):
        ap.error('--clock must be night, dawn or HH:MM')
    if args.hold < 4 or args.hold > 15 or args.warmup < 0 or args.warmup > 30 or not 10 <= args.startup_timeout <= 180:
        ap.error('use hold 4..15s, warmup 0..30s and startup timeout 10..180s')
    if args.seconds is not None and not 12 <= args.seconds <= 120:
        ap.error('--seconds must be 12..120 for a short test')
    try:
        return run(args)
    except (RuntimeError, OSError, ValueError) as error:
        print('BLOCKED:', error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
