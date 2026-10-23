#!/usr/bin/env python3
"""Record short PSO tours, expand frozen same-cook keys, or summarize startup evidence.

  pso-cache.py record --build ~/scratch-bm/Signal47-build --out /path/to/evidence/pso --dry-run
  pso-cache.py record --build ~/scratch-bm/Signal47-build --out /path/to/evidence/pso
  pso-cache.py expand --recordings /path/to/evidence/pso
  Scripts/package-linux.sh

Default record matrix: all 20 cameras, Low/Clean + High/Ultra VHS, night AND dawn,
max four cameras per process. A busy machine blocks immediately. Recorded .upipelinecache
files and shader keys remain in evidence; only the stable .spc is a shipping cook input.
"""
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

from unreal_guard import require_idle, run_owned

PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_KEYS = PROJECT / 'Saved/Cooked/Linux/Signal47/Metadata/PipelineCaches'


def digest(path):
    sha = hashlib.sha256()
    with path.open('rb') as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b''):
            sha.update(data)
    return sha.hexdigest()


def key_manifest(directory):
    return [{'name': p.name, 'bytes': p.stat().st_size, 'sha256': digest(p)}
            for p in sorted(directory.glob('*SF_VULKAN_SM6.shk'))]


def record_plan(args):
    for quality in args.qualities:
        for clock in args.clocks:
            picture = 'off' if quality == 'low' else 'vhs'
            run_out = args.out.expanduser().resolve() / f'{quality}-{clock}'
            command = [sys.executable, str(PROJECT / 'Scripts/graphics-review.py'), '--build', str(args.build.expanduser().resolve()),
                       '--out', str(run_out), '--quality', quality, '--picture', picture,
                       '--clock', clock, '--batch-size', str(args.batch_size)]
            if args.seconds:
                command += ['--seconds', str(args.seconds)]
            if args.views:
                command += ['--views'] + args.views
            command += ['--', '-logPSO']
            yield quality, clock, run_out, command


def record(args):
    plan = list(record_plan(args))
    if args.dry_run:
        print(json.dumps([{'quality': q, 'clock': c, 'command': cmd} for q, c, _, cmd in plan], indent=2))
        return
    require_idle()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    if any(out.iterdir()):
        raise RuntimeError('Use an empty output directory so recordings cannot mix builds: ' + str(out))
    # Freeze keys BEFORE recording; expand never quietly substitutes keys from a later cook.
    keys_dir = args.keys.expanduser().resolve()
    frozen = key_manifest(keys_dir)
    if not frozen or any(x['bytes'] == 0 for x in frozen):
        raise RuntimeError('Need nonempty Vulkan SM6 stable shader keys from the same cook: ' + str(keys_dir))
    keys_copy = out / 'keys'
    keys_copy.mkdir()
    for item in frozen:
        shutil.copy2(keys_dir / item['name'], keys_copy / item['name'])
    if key_manifest(keys_copy) != frozen:
        raise RuntimeError('Stable key snapshot changed while copying; recook / record again')
    recorded = out / 'recorded'
    recorded.mkdir()
    manifest = {'status': 'INCOMPLETE', 'complete': False, 'keys': frozen, 'keys_source': str(keys_dir),
                'planned_matrix': [{'quality': q, 'clock': c} for q, c, _, _ in plan], 'runs': [], 'caches': []}
    manifest_path = out / 'recordings.json'
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    baseline_build = None
    for index, (quality, clock, run_out, command) in enumerate(plan):
        print(f'PSO {index+1}/{len(plan)} {quality}/{clock}: short sequential camera batches', flush=True)
        subprocess.run(command, check=True)
        root = json.loads((run_out / 'meta.json').read_text())
        runs = root.get('batches', [root])
        if not root.get('valid_evidence') or any(not r.get('valid_evidence') or r.get('contention') for r in runs):
            raise RuntimeError('Incomplete or contended tour cannot supply PSO evidence: ' + str(run_out))
        if baseline_build is None:
            baseline_build = root['build']
        if root['build'] != baseline_build:
            raise RuntimeError('Packaged executable changed during PSO recording')
        for meta_path in sorted(run_out.rglob('meta.json')):
            data = json.loads(meta_path.read_text())
            if 'batches' in data:
                continue
            text = (meta_path.parent / 'run.log').read_text(errors='replace')
            if not re.search(r'Vulkan PSO Precaching = 1, PipelineFileCache = 1', text):
                raise RuntimeError('Vulkan did not enable BOTH precaching and pipeline file cache: ' + str(meta_path.parent))
            caches = sorted((meta_path.parent / 'profile').rglob('*.rec.upipelinecache'))
            if not caches:
                raise RuntimeError('No recorded PSOs in ' + str(meta_path.parent))
            for source in caches:
                target = recorded / (f'{quality}_{clock}_{meta_path.parent.name}_{len(manifest["caches"]):03d}_' + source.name)
                shutil.copy2(source, target)
                manifest['caches'].append({'quality': quality, 'clock': clock, 'source': str(source),
                                           'copy': str(target), 'bytes': target.stat().st_size, 'sha256': digest(target)})
        if key_manifest(keys_dir) != frozen:
            raise RuntimeError('Stable keys changed during recording; mixed cook evidence is invalid')
        manifest['runs'].append({'quality': quality, 'clock': clock, 'meta': str(run_out / 'meta.json'),
                                 'expected_views': root['expected_views']})
        manifest['build'] = baseline_build
        manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    all_views = ('control', 'crt', 'annex', 'corridor', 'lab-normal', 'lab-safe', 'lab-print',
                 'yard', 'service-yard', 'lot', 'motel', 'room6', 'station', 'hut', 'highway',
                 'diner-outside', 'diner', 'caliche', 'old-road', 'cab')
    full_matrix = (set(args.qualities) == {'low', 'high', 'ultra'} and set(args.clocks) == {'night', 'dawn'} and
                   all(tuple(run['expected_views']) == all_views for run in manifest['runs']))
    manifest.update({'complete': True, 'status': 'PASS', 'shipping_coverage_complete': full_matrix})
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Recorded {len(manifest["caches"])} cache files across {len(plan)} quality/clock tours. Shipping coverage complete={full_matrix}.', flush=True)


def stable_header(path):
    # UE 5.8 PipelineCacheUtilities.cpp FStablePipelineCacheSerializedHeader, LE Linux.
    with path.open('rb') as stream:
        raw = stream.read(48)
        length_bytes = stream.read(4)
        if len(length_bytes) != 4:
            raise ValueError('Truncated stable PSO platform string')
        length = struct.unpack('<i', length_bytes)[0]
        if not 0 < abs(length) <= 128:
            raise ValueError('Invalid stable PSO platform string length')
        platform_bytes = stream.read(abs(length) * (2 if length < 0 else 1))
        platform = platform_bytes.decode('utf-16-le' if length < 0 else 'utf-8').rstrip('\0')
        header_bytes = stream.tell()
    if len(raw) != 48:
        raise ValueError('Truncated stable PSO header')
    magic, version, pso_size, keys, groups, payload, uncompressed = struct.unpack('<QiiqqQQ', raw)
    if magic != 0x484341434C425453 or version != 8 or pso_size <= 0 or keys <= 0 or groups <= 0 or payload <= 0:
        raise ValueError('Invalid/empty UE 5.8 stable PSO cache header')
    if platform != 'SF_VULKAN_SM6':
        raise ValueError('Stable PSO platform is not SF_VULKAN_SM6: ' + platform)
    if payload != path.stat().st_size - header_bytes:
        raise ValueError('Truncated stable PSO payload')
    return {'version': version, 'platform': platform, 'stable_keys': keys, 'permutation_groups': groups,
            'payload_bytes': payload, 'uncompressed_bytes': uncompressed}


def expand(args):
    recordings = args.recordings.expanduser().resolve()
    manifest_path = recordings / 'recordings.json'
    if not manifest_path.is_file():
        raise RuntimeError('Need recordings.json proving complete, uncontended same-cook tours')
    recorded = json.loads(manifest_path.read_text())
    if not recorded.get('complete') or recorded.get('status') != 'PASS':
        raise RuntimeError('Recordings are incomplete; refusing to ship partial evidence')
    if not recorded.get('shipping_coverage_complete'):
        raise RuntimeError('Need all 20 cameras at night/dawn across Low, High and Ultra before shipping')
    keys_copy = recordings / 'keys'
    if key_manifest(keys_copy) != recorded['keys']:
        raise RuntimeError('Frozen stable keys differ from the recorded cook')
    if args.keys and key_manifest(args.keys.expanduser().resolve()) != recorded['keys']:
        raise RuntimeError('--keys differs from the recorded cook; use frozen recordings/keys')
    caches = []
    for item in recorded['caches']:
        path = recordings / 'recorded' / Path(item['copy']).name
        if not path.is_file() or path.stat().st_size != item['bytes'] or digest(path) != item['sha256']:
            raise RuntimeError('Recorded PSO cache missing or changed: ' + str(path))
        caches.append(path)
    if not caches:
        raise RuntimeError('No PSO caches in the recording manifest')
    output = args.output.expanduser().resolve()
    if not output.name.endswith('Signal47_SF_VULKAN_SM6.spc'):
        raise RuntimeError('Output must end in Signal47_SF_VULKAN_SM6.spc so the Linux cooker finds it')
    output.parent.mkdir(parents=True, exist_ok=True)
    # Never accept an old output after a failed expansion. Promote only verified fresh output.
    temporary = recordings / 'expanded_Signal47_SF_VULKAN_SM6.spc'
    if temporary.exists():
        raise RuntimeError('Fresh expansion path already exists: ' + str(temporary))
    engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
    command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(PROJECT / 'Signal47.uproject'),
               '-run=ShaderPipelineCacheTools', 'expand', str(recordings / 'recorded/*.rec.upipelinecache'),
               str(keys_copy / '*.shk'), str(temporary), '-unattended', '-nullrhi', '-nosplash',
               '-ddc=InstalledNoZenLocalFallback']
    log_path = recordings / 'expand.log'
    execution = run_owned(command, log_path, timeout=args.timeout)
    text = log_path.read_text(errors='replace')
    # Verified in local UE 5.8.4 ShaderPipelineCacheToolsCommandlet.cpp:1451. Binary
    # SaveStablePipelineCacheFile has no separate success log: check exit + fresh header.
    counts = re.findall(r'Generated (\d+) stable PSOs total', text)
    manifest = {'command': command, 'execution': execution, 'valid_evidence': False}
    if execution['returncode'] or execution['contention'] or execution['stop_reason'] or not counts or int(counts[-1]) == 0:
        (recordings / 'expanded.json').write_text(json.dumps(manifest, indent=2) + '\n')
        raise RuntimeError(f'Expansion failed, was contended or produced no stable PSOs: {log_path}')
    header = stable_header(temporary)
    pending = output.with_suffix('.spc.pending')
    shutil.copy2(temporary, pending)
    os.replace(pending, output)
    manifest.update({'valid_evidence': True, 'stable_psos': int(counts[-1]), 'header': header,
                     'output': str(output), 'bytes': output.stat().st_size, 'sha256': digest(output),
                     'inputs': [{'path': str(p), 'bytes': p.stat().st_size, 'sha256': digest(p)}
                                for p in caches + sorted(keys_copy.glob('*.shk'))]})
    (recordings / 'expanded.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Expanded {counts[-1]} stable PSOs to {output}. Repackage; verify cook and runtime cache-open logs.', flush=True)


def summarize_trace(path):
    """Exact 10-second completed-frame bins; raw intervals retain boundary-crossing hitches."""
    result = {'valid_evidence': False, 'origin': 'world start and first player-visible frame', 'problems': []}
    if not path.is_file():
        result.update({'status': 'UNVERIFIED', 'problems': ['No startup trace; old packages cannot prove world-start or visible-frame timing']})
        return result
    try:
        with path.open(newline='') as stream:
            rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(stream)]
        if not rows:
            raise ValueError('empty trace')
        required = ('frame', 'wall_s', 'visible_s', 'covered', 'frame_ms')
        for row in rows:
            if not all(math.isfinite(row[k]) for k in required) or row['frame_ms'] < 0 or row['covered'] not in (0, 1):
                raise ValueError('invalid trace values')
            if row['covered'] == 1 and row['visible_s'] >= 0:
                raise ValueError('covered frame claims to be visible')
        if any(b['frame'] != a['frame'] + 1 or b['wall_s'] <= a['wall_s'] for a, b in zip(rows, rows[1:])):
            raise ValueError('missing/duplicate frames or nonmonotone wall timestamps')
        for a, b in zip(rows, rows[1:]):
            if abs((b['wall_s'] - a['wall_s']) * 1000 - b['frame_ms']) > .1:
                raise ValueError('frame time does not match wall interval')
        visible = [r for r in rows if r['visible_s'] >= 0]
        if any(b['visible_s'] <= a['visible_s'] for a, b in zip(visible, visible[1:])):
            raise ValueError('nonmonotone visible timestamps')
        for name, key, source in [('initial_10s', 'wall_s', rows), ('visible_10s', 'visible_s', visible)]:
            completed = [r for r in source if 0 < r[key] <= 10]
            overlapping = [r for r in source if r[key] > 0 and r[key] - r['frame_ms'] / 1000 < 10]
            frames = sorted(r['frame_ms'] for r in overlapping)
            first_start = source[0][key] - source[0]['frame_ms'] / 1000 if source else None
            complete = bool(source and first_start <= .001 and source[-1][key] >= 10)
            stats = {'complete': complete, 'frames': len(completed), 'fps': len(completed) / 10,
                     'frames_each_second': [sum(second < r[key] <= second + 1 for r in completed) for second in range(10)],
                     'timed_intervals': len(frames), 'first_interval_start_s': first_start,
                     'last_sample_s': source[-1][key] if source else None,
                     'timing_policy': 'whole raw frame intervals overlapping [0,10); includes boundary-crossing hitch'}
            if frames:
                stats.update({'mean_ms': sum(frames) / len(frames),
                              'p95_ms': frames[math.ceil(len(frames) * .95) - 1], 'max_ms': max(frames),
                              'over_16_667ms': sum(x > 1000 / 60 for x in frames),
                              'over_33_333ms': sum(x > 1000 / 30 for x in frames),
                              'over_50ms': sum(x > 50 for x in frames)})
            if not complete:
                result['problems'].append(name + ': trace must cover the origin through the >=10s boundary sample')
            result[name] = stats
        result['valid_evidence'] = not result['problems']
        result['status'] = 'PASS' if result['valid_evidence'] else 'INCOMPLETE'
    except (OSError, KeyError, TypeError, ValueError) as error:
        result.update({'status': 'FAIL', 'problems': [str(error)]})
    return result


def summary(args):
    result = summarize_trace(args.csv.expanduser().resolve())
    # An existing contention manifest also invalidates otherwise complete CSV data.
    meta_path = args.csv.expanduser().resolve().parent / 'meta.json'
    if meta_path.exists():
        meta = json.loads(meta_path.read_text())
        if meta.get('contention') or meta.get('stop_reason'):
            result['valid_evidence'] = False
            result['status'] = 'FAIL'
            result['problems'].append('run metadata reports contention/interruption')
    print(json.dumps(result, indent=2))
    return 0 if result['valid_evidence'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest='command', required=True)
    rec = commands.add_parser('record')
    rec.add_argument('--build', type=Path, required=True)
    rec.add_argument('--out', type=Path, required=True)
    rec.add_argument('--keys', type=Path, default=DEFAULT_KEYS)
    rec.add_argument('--qualities', nargs='+', choices=['low', 'high', 'ultra'], default=['low', 'high', 'ultra'])
    rec.add_argument('--clocks', nargs='+', choices=['night', 'dawn'], default=['night', 'dawn'])
    rec.add_argument('--views', nargs='+', help='optional short subset / retake')
    rec.add_argument('--batch-size', type=int, choices=range(1, 5), default=4)
    rec.add_argument('--seconds', type=int, help='optional per-batch duration, at most 120s')
    rec.add_argument('--dry-run', action='store_true')
    rec.set_defaults(action=record)
    exp = commands.add_parser('expand')
    exp.add_argument('--recordings', type=Path, required=True)
    exp.add_argument('--keys', type=Path, help='optional crosscheck; expansion always uses frozen keys')
    exp.add_argument('--output', type=Path, default=PROJECT / 'Build/Linux/PipelineCaches/S47_Signal47_SF_VULKAN_SM6.spc')
    exp.add_argument('--timeout', type=int, default=300)
    exp.set_defaults(action=expand)
    summ = commands.add_parser('summary')
    summ.add_argument('csv', type=Path)
    summ.set_defaults(action=summary)
    args = parser.parse_args()
    try:
        return args.action(args) or 0
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as error:
        print('BLOCKED:', error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
