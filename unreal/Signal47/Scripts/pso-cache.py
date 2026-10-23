#!/usr/bin/env python3
"""Record short PSO tours, expand frozen same-cook keys, or summarize startup evidence.

  pso-cache.py record --build ~/scratch-bm/Signal47-build --out /path/to/evidence/pso --dry-run
  pso-cache.py record --build ~/scratch-bm/Signal47-build --out /path/to/evidence/pso
  pso-cache.py expand --recordings /path/to/evidence/pso
  Scripts/package-linux.sh

Default record matrix: original20 cameras plus power/light/menu and native-terminal views,
Low/Clean + High/Ultra VHS, night AND dawn,
max four cameras per process. A busy machine blocks immediately. Recorded .upipelinecache
files and shader keys remain in evidence; only the stable .spc is a shipping cook input.
"""
import argparse
import csv
import functools
import hashlib
import importlib.util
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


@functools.cache
def graphics_review():
    spec = importlib.util.spec_from_file_location('s47_graphics_plan', PROJECT / 'Scripts/graphics-review.py')
    graphics = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(graphics)
    return graphics


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
    warmup, hold = getattr(args, 'warmup', 8), getattr(args, 'hold', 7)
    if not 0 <= warmup <= 30 or not 4 <= hold <= 15:
        raise ValueError('use hold 4..15s and warmup 0..30s')
    for quality in args.qualities:
        for clock in args.clocks:
            picture = 'off' if quality == 'low' else 'vhs'
            run_out = args.out.expanduser().resolve() / f'{quality}-{clock}'
            command = [sys.executable, str(PROJECT / 'Scripts/graphics-review.py'), '--build', str(args.build.expanduser().resolve()),
                       '--out', str(run_out), '--quality', quality, '--picture', picture,
                       '--clock', clock, '--batch-size', str(args.batch_size), '--suite', 'all',
                       '--warmup', str(warmup), '--hold', str(hold),
                       '--quarantine-root', str(args.out.expanduser().resolve().parent / (args.out.name + '-rejected')),
                       '--driver-cache', str(args.out.expanduser().resolve() / 'driver-cache')]
            if args.seconds:
                command += ['--seconds', str(args.seconds)]
            if args.views:
                command += ['--views'] + args.views
            if getattr(args, 'resume', False):
                command += ['--resume']
            command += ['--', '-logPSO']
            yield quality, clock, run_out, command


def record(args):
    plan = list(record_plan(args))
    graphics = graphics_review()
    if args.dry_run:
        groups = graphics.partition_views(args.views or graphics.ALL_VIEWS, args.batch_size)
        print(json.dumps([{'quality': q, 'clock': c, 'command': cmd,
                           'batch_count': len(groups), 'batches': groups} for q, c, _, cmd in plan], indent=2))
        return
    require_idle()
    out = args.out.expanduser().resolve()
    out.mkdir(parents=True, exist_ok=True)
    existing = any(out.iterdir())
    if existing and not getattr(args, 'resume', False):
        raise RuntimeError('Use an empty output directory so recordings cannot mix builds: ' + str(out))
    # Freeze keys BEFORE recording; expand never quietly substitutes keys from a later cook.
    keys_dir = args.keys.expanduser().resolve()
    frozen = key_manifest(keys_dir)
    if not frozen or any(x['bytes'] == 0 for x in frozen):
        raise RuntimeError('Need nonempty Vulkan SM6 stable shader keys from the same cook: ' + str(keys_dir))
    baseline_build = graphics.package_fingerprint(args.build.expanduser().resolve() / 'Linux/Signal47/Binaries/Linux/Signal47')
    record_request = {'qualities': args.qualities, 'clocks': args.clocks,
                      'views': args.views or list(graphics.ALL_VIEWS), 'batch_size': args.batch_size, 'seconds': args.seconds,
                      'warmup': getattr(args, 'warmup', 8), 'hold': getattr(args, 'hold', 7)}
    manifest_path = out / 'recordings.json'
    keys_copy = out / 'keys'
    if existing:
        if not manifest_path.is_file():
            raise RuntimeError('Cannot resume recordings without their original manifest; use a fresh output root')
        previous = json.loads(manifest_path.read_text())
        if (previous.get('build') != baseline_build or previous.get('keys') != frozen or
                previous.get('record_request') != record_request or key_manifest(keys_copy) != frozen):
            raise RuntimeError('Resume package, stable keys or requested matrix differ; use a fresh output root')
    else:
        keys_copy.mkdir()
        for item in frozen:
            shutil.copy2(keys_dir / item['name'], keys_copy / item['name'])
    if key_manifest(keys_copy) != frozen:
        raise RuntimeError('Stable key snapshot changed while copying; recook / record again')
    recorded = out / 'recorded'
    recorded.mkdir(exist_ok=True)
    manifest = {'status': 'INCOMPLETE', 'complete': False, 'keys': frozen, 'keys_source': str(keys_dir),
                'build': baseline_build, 'record_request': record_request,
                'contended': False, 'performance_status': 'UNVERIFIED',
                'planned_matrix': [{'quality': q, 'clock': c} for q, c, _, _ in plan], 'runs': [], 'caches': []}
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    for index, (quality, clock, run_out, command) in enumerate(plan):
        print(f'PSO {index+1}/{len(plan)} {quality}/{clock}: short sequential camera batches', flush=True)
        subprocess.run(command, check=True)
        root = json.loads((run_out / 'meta.json').read_text())
        runs = root.get('batches', [root])
        if not root.get('diagnostic_pass', root.get('valid_evidence')) or any(not r.get('diagnostic_pass', r.get('valid_evidence')) for r in runs):
            raise RuntimeError('Incomplete or interrupted tour cannot supply even diagnostic PSO coverage: ' + str(run_out))
        manifest['contended'] |= any(bool(r.get('contention')) for r in runs)
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
    all_views = graphics.ALL_VIEWS
    full_matrix = (set(args.qualities) == {'low', 'high', 'ultra'} and set(args.clocks) == {'night', 'dawn'} and
                   len(manifest['runs']) == 6 and
                   all(tuple(run['expected_views']) == all_views for run in manifest['runs']))
    copied = {Path(item['copy']).name for item in manifest['caches']}
    for stale in sorted(recorded.glob('*.rec.upipelinecache')):
        if stale.name not in copied:
            graphics.quarantine_leaf(args, stale)
    manifest.update({'complete': True, 'status': 'CONTENDED' if manifest['contended'] else 'PASS',
                     'performance_status': 'UNVERIFIED' if manifest['contended'] else 'UNCONTENDED',
                     'shipping_coverage_complete': full_matrix})
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
    if not recorded.get('complete') or recorded.get('status') not in ('PASS', 'CONTENDED'):
        raise RuntimeError('Recordings are incomplete; refusing to ship partial evidence')
    if not recorded.get('shipping_coverage_complete'):
        raise RuntimeError(f'Need all {len(graphics_review().ALL_VIEWS)} world/power/light/menu/terminal views at night/dawn across Low, High and Ultra before shipping')
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
    if {path.name for path in (recordings / 'recorded').glob('*.rec.upipelinecache')} != {path.name for path in caches}:
        raise RuntimeError('Unmanifested PSO files in recorded/: refusing wildcard expansion of stale evidence')
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
    if execution['returncode'] or execution['stop_reason'] or not counts or int(counts[-1]) == 0:
        (recordings / 'expanded.json').write_text(json.dumps(manifest, indent=2) + '\n')
        raise RuntimeError(f'Expansion failed, was contended or produced no stable PSOs: {log_path}')
    header = stable_header(temporary)
    pending = output.with_suffix('.spc.pending')
    shutil.copy2(temporary, pending)
    os.replace(pending, output)
    contended = bool(recorded.get('contended') or execution['contention'])
    manifest.update({'valid_evidence': not contended, 'valid_cache_artifact': True,
                     'status': 'CONTENDED' if contended else 'PASS', 'performance_status': 'UNVERIFIED' if contended else 'UNCONTENDED',
                     'stable_psos': int(counts[-1]), 'header': header,
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


def binary_cache_header(path):
    # PipelineFileCache.cpp:274 serializes these fields individually, without C++ padding.
    with path.open('rb') as stream:
        raw = stream.read(49)
    if len(raw) != 49:
        raise ValueError('Truncated bundled pipeline-cache header')
    magic, version, game, platform, a, b, c, d, table, gc_time = struct.unpack('<QIIB4IQq', raw)
    if magic != 0x5049504543414348 or version != 31 or not 49 <= table < path.stat().st_size:
        raise ValueError('Invalid UE5.8 bundled pipeline-cache header')
    return {'path': str(path), 'guid': ''.join(f'{value:08X}' for value in (a, b, c, d)),
            'version': version, 'game_version': game, 'platform': platform,
            'bytes': path.stat().st_size, 'sha256': digest(path)}


def graphics_cache_proof(recorded, expanded, cook, binary, runtime):
    """Read-only proof chain. Nonzero compute-only caches deliberately cannot pass."""
    matrix = {(run['quality'], run['clock']) for run in recorded.get('runs', [])}
    required = {(q, c) for q in ('low', 'high', 'ultra') for c in ('night', 'dawn')}
    all_views = graphics_review().ALL_VIEWS
    coverage = (recorded.get('complete') and recorded.get('shipping_coverage_complete') and matrix == required and
                len(recorded.get('runs', [])) == 6 and
                all(tuple(run.get('expected_views', [])) == all_views for run in recorded['runs']))
    spc_name = Path(expanded.get('output', '')).name
    loaded = re.findall(r'Loaded (\d+) stable PSOs from (.+?\.spc)\. (\d+) PSOs rejected, (\d+) PSOs merged', cook)
    consumed = [dict(count=int(n), path=p.strip(), rejected=int(r), merged=int(m)) for n, p, r, m in loaded
                if Path(p.strip()).name == spc_name and int(n) > 0]
    written = re.findall(r'Wrote (\d+) binary PSOs \(graphics: (\d+) compute: (\d+) RT: (\d+)\), \((\d+)KB\) to ([^\n]+\.upipelinecache)', cook)
    counts = [dict(total=int(n), graphics=int(g), compute=int(c), raytracing=int(r), path=p.strip())
              for n, g, c, r, kb, p in written if Path(p.strip()).name == Path(binary['path']).name]
    final = counts[-1] if counts else {}
    binary_name = Path(binary['path']).name
    bundle_key = binary_name.split('_SF_', 1)[0]
    physical = re.findall(r'Opened FPipelineCacheFile: (.+?) \(GUID: ([0-9A-Fa-f-]+)\) with (\d+) entries\.', runtime)
    matching_physical = [dict(filename=f, guid=guid.replace('-', '').upper(), entries=int(n))
                         for f, guid, n in physical
                         if Path(f).name == binary_name and guid.replace('-', '').upper() == binary['guid']
                         and int(n) == final.get('total')]
    opens = re.findall(r'FPipelineCacheFile\[([^\]]+)\] opened [^\n]*?filename (.*?), guid ([0-9A-Fa-f-]+)\.', runtime)
    matching = [dict(key=k, filename=f, guid=guid.replace('-', '').upper()) for k, f, guid in opens
                if k == bundle_key and guid.replace('-', '').upper() == binary['guid']
                and (Path(f).name == binary_name or (f == bundle_key and matching_physical))]
    keys = {item['key'] for item in matching}
    active, graphics_processed, queues = None, 0, []
    for line in runtime.splitlines():
        begin = re.search(r'BeginNextPrecompileCacheTask\(\) - (.+?) begining compile\.', line)
        if begin:
            active = begin[1]
        queued = re.search(r"starting pipeline cache '([^']+)' and enqueued (\d+) tasks.*cache contains (\d+)", line)
        if queued and queued[1] in keys:
            queues.append({'key': queued[1], 'enqueued': int(queued[2]), 'contains': int(queued[3])})
        if active in keys and re.search(r'Precompiling PSO \d+ \(type 1\)', line):
            graphics_processed += 1
    checks = {'all_required_views_quality_clock_coverage': bool(coverage),
              'expanded_stable_cache': bool(expanded.get('valid_cache_artifact') and expanded.get('stable_psos', 0) > 0),
              'cook_consumed_our_spc': bool(consumed),
              'cooked_graphics_psos_positive': bool(final.get('graphics', 0) > 0),
              'consistent_cook_counts': bool(final and final['total'] == final['graphics'] + final['compute'] + final['raytracing']),
              'runtime_opened_same_binary_guid': bool(matching),
              'runtime_cache_count_matches_cook': bool(any(q['enqueued'] > 0 and q['contains'] == final.get('total') for q in queues)),
              'runtime_processed_bundled_graphics': graphics_processed > 0}
    return {'status': 'PASS' if all(checks.values()) else 'UNVERIFIED', 'valid_cache_proof': all(checks.values()),
            'checks': checks, 'cook_spc_inputs': consumed, 'cook_final_counts': final, 'binary_cache': binary,
            'runtime_matching_opens': matching, 'runtime_physical_opens': matching_physical,
            'runtime_queues': queues, 'runtime_graphics_processed': graphics_processed,
            'performance_status': 'UNVERIFIED; cache-content proof is not FPS evidence',
            'problems': [name for name, passed in checks.items() if not passed]}


def verify(args):
    recordings = args.recordings.expanduser().resolve()
    recorded = json.loads((recordings / 'recordings.json').read_text())
    expanded = json.loads((recordings / 'expanded.json').read_text())
    stable = Path(expanded['output'])
    stable_header(stable)
    if digest(stable) != expanded['sha256']:
        raise RuntimeError('Stable .spc changed after expansion')
    binary = binary_cache_header(args.binary_cache.expanduser().resolve())
    cook = args.cook_log.expanduser().read_text(errors='replace')
    runtime = args.runtime_log.expanduser().read_text(errors='replace')
    result = graphics_cache_proof(recorded, expanded, cook, binary, runtime)
    written = Path(result.get('cook_final_counts', {}).get('path', '/missing-cook-cache'))
    same_bytes = written.is_file() and digest(written) == binary['sha256']
    result['checks']['cook_output_matches_binary_bytes'] = same_bytes
    if not same_bytes:
        result['problems'].append('cook_output_matches_binary_bytes')
        result.update({'valid_cache_proof': False, 'status': 'UNVERIFIED'})
    result['inputs'] = {str(path): digest(path) for path in
                        (recordings / 'recordings.json', recordings / 'expanded.json',
                         args.cook_log.expanduser(), args.runtime_log.expanduser())}
    if args.out:
        args.out.expanduser().write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
    return 0 if result['valid_cache_proof'] else 1


def summary(args):
    result = summarize_trace(args.csv.expanduser().resolve())
    # An existing contention manifest also invalidates otherwise complete CSV data.
    meta_path = args.csv.expanduser().resolve().parent / 'meta.json'
    if meta_path.exists():
        meta = json.loads(meta_path.read_text())
        invalid_resolution = meta.get('resolution', {}).get('complete') is False or meta.get('status') == 'INVALID_RESOLUTION'
        if meta.get('contention') or meta.get('stop_reason') or invalid_resolution:
            result['valid_evidence'] = False
            result['status'] = 'INVALID_RESOLUTION' if invalid_resolution else ('FAIL' if meta.get('stop_reason') else 'CONTENDED')
            result['performance_status'] = 'UNVERIFIED'
            result['problems'].append('run metadata reports wrong resolution, contention or interruption; target FPS is UNVERIFIED')
            for name in ('initial_10s', 'visible_10s'):
                if name in result:
                    result[name]['performance_status'] = 'UNVERIFIED'
                    result[name]['diagnostic_fps'] = result[name].get('fps')
                    result[name]['fps'] = None
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
    rec.add_argument('--warmup', type=float, default=8, help='pre-tour warmup seconds, 0..30 (default: 8)')
    rec.add_argument('--hold', type=float, default=7, help='seconds per view, 4..15 (default: 7)')
    rec.add_argument('--dry-run', action='store_true')
    rec.add_argument('--resume', action='store_true', help='resume only the same package/keys/matrix; validate and reuse matching PASS leaves')
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
    proof = commands.add_parser('verify', help='read-only graphics cache proof; runtime diagnostic needs -LogCmds="LogRHI Verbose"')
    proof.add_argument('--recordings', type=Path, required=True)
    proof.add_argument('--cook-log', type=Path, required=True)
    proof.add_argument('--binary-cache', type=Path, required=True)
    proof.add_argument('--runtime-log', type=Path, required=True)
    proof.add_argument('--out', type=Path)
    proof.set_defaults(action=verify)
    args = parser.parse_args()
    try:
        return args.action(args) or 0
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as error:
        print('BLOCKED:', error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
