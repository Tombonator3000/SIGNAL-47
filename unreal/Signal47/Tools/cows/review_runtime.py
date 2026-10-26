#!/usr/bin/env python3
"""Capture a narrow, sequential packaged-cow review; images still need human inspection.

Without --run this prints the launch plan and starts no Unreal process. BUILD is
the archive directory containing Linux/, or Linux/ itself. OUT must be fresh.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
sys.path.insert(0, str(PROJECT / 'Scripts'))
from unreal_guard import GRACE_ENV, require_idle, run_owned

SOURCE_PATHS = (
    'Signal47/Source/Signal47/Private/S47CowMeshes.h',
    'Signal47/Source/Signal47/Private/S47CowMeshes.cpp',
    'Signal47/Source/Signal47/Private/S47Herd.cpp',
    'Signal47/Source/Signal47/Public/S47Herd.h',
    'Signal47/Source/Signal47/Private/S47NinthModel.cpp',
    'Signal47/Source/Signal47/Public/S47NinthModel.h',
    'Signal47/Source/Signal47/Private/S47RenderAudit.cpp',
    'Signal47/Source/Signal47/Private/S47TownLayer.cpp',
    'Signal47/Config/DefaultGame.ini',
    'Signal47/Tools/cows/import_assets.py',
    'Signal47/Tools/cows/verify_assets.py',
    'Signal47/Tools/cows/review_runtime.py',
    'Signal47/Tools/cows/parse_rt_csv.py',
    'ArtSource/Cows/build_cows.py',
    'ArtSource/Cows/materials.json',
    'ArtSource/Cows/model_contract.json',
)
ALIVE_MARKERS = ('S47: cow models alive ready: saro, 9 bodies and heads',
                 'S47: cow models alive ready: road, 9 bodies and heads')
DEAD_MARKER = 'S47: cow models abducted ready'
ENGINE_ERRORS = ('Critical error:', 'Assertion failed:', 'Signal 11 caught',
                 'LowLevelFatalError', 'Fatal error:', 'Failed to compile Material',
                 'Default Material will be used in game')
CASES = (
    ('selftest', 'high', ('-S47SelfTest', '-S47Face=room'), (), True),
    ('alive-still-high', 'high', ('-S47Pasture=still',),
     ('S47: pasture test still',), False),
    ('alive-missing-high', 'high', ('-S47Pasture=missing',),
     ('S47: pasture test short',), False),
    ('alive-grazing-close-high', 'high',
     ('-S47PropsAt=saro:-58.3;861.5;-0.759;-0.14', '-S47Light'),
     ('S47: props view at saro:-58.3;861.5;-0.759;-0.14',), False),
    ('alive-still-low', 'low', ('-S47Pasture=still',),
     ('S47: pasture test still',), False),
    ('abducted-found-high', 'high', ('-S47Ninth=found', '-S47Light'),
     ('S47: ninth cow test',), False),
    ('abducted-found-low', 'low', ('-S47Ninth=found', '-S47Light'),
     ('S47: ninth cow test',), False),
    ('abducted-details-high', 'high',
     ('-S47Ninth=details', '-S47Light'),
     ('S47: ninth cow test found',), False),
    ('baseline-alive-off', 'high', ('-S47Pasture=still', '-S47CowsOff'),
     ('S47: pasture test still',), False),
    ('baseline-abducted-off', 'high', ('-S47Ninth=found', '-S47Light', '-S47CowsOff'),
     ('S47: ninth cow test',), False),
)


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(8 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def file_record(path):
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha256': sha(path)}


def source_identity(root):
    rows = {name: sha(root / name) for name in SOURCE_PATHS}
    aggregate = hashlib.sha256(json.dumps(rows, sort_keys=True).encode()).hexdigest()
    return {'root': str(root), 'files': rows, 'sha256': aggregate}


def package_identity(linux):
    executable = linux / 'Signal47/Binaries/Linux/Signal47'
    containers = sorted((linux / 'Signal47/Content/Paks').glob('*'))
    if not executable.is_file() or not containers or not all(p.is_file() for p in containers):
        raise ValueError('Packaged executable and package containers are required')
    rows = {str(p.relative_to(linux)): file_record(p) for p in [executable] + containers}
    digest = hashlib.sha256(json.dumps(rows, sort_keys=True).encode()).hexdigest()
    return {'root': str(linux), 'executable': file_record(executable),
            'files': rows, 'sha256': digest}


def report_identity(path):
    value = json.loads(path.read_text())
    if value.get('status') != 'PASS' or not value.get('checks') or not all(row.get('pass') for row in value['checks']):
        raise ValueError('Import report is not a completed PASS: ' + str(path))
    expected = {'SM_Cow_Alive_Body', 'SM_Cow_Alive_Head', 'SM_Cow_Abducted'}
    if value.get('destination') != '/Game/Signal47/Art/Cows' or {row.get('name') for row in value.get('meshes', [])} != expected:
        raise ValueError('Import report does not identify the complete native cow set')
    return dict(file_record(path), imported_meshes=len(value.get('meshes', [])))


def png_record(path):
    data = path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('Screenshot is missing or is not PNG')
    offset, compressed, complete, size = 8, bytearray(), False, None
    while offset + 12 <= len(data):
        length, kind = struct.unpack_from('>I4s', data, offset)
        if offset + length + 12 > len(data):
            raise ValueError('Truncated screenshot')
        chunk = data[offset + 8:offset + 8 + length]
        crc = struct.unpack_from('>I', data, offset + 8 + length)[0]
        if zlib.crc32(kind + chunk) & 0xffffffff != crc:
            raise ValueError('Screenshot PNG CRC mismatch')
        if kind == b'IHDR':
            size = struct.unpack_from('>II', chunk)
        elif kind == b'IDAT':
            compressed.extend(chunk)
        elif kind == b'IEND':
            complete = True
            break
        offset += length + 12
    if not complete or size != (1600, 900) or not compressed or not zlib.decompress(compressed):
        raise ValueError('Screenshot is incomplete or is not 1600x900')
    return dict(file_record(path), width=size[0], height=size[1])


def read_log(path):
    data = path.read_bytes()
    return data.decode('utf-16' if data[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig', errors='replace')


def render_audit(leaf, log, cows_off):
    """Check every emitted section audit; the exception remains limited to cow bindings."""
    paths = sorted((leaf / 'audit').glob('*.json'))
    problems, reports, meshes = [], [], set()
    if not paths:
        problems.append('No native render-audit JSON emitted')
    for path in paths:
        value = json.loads(path.read_text())
        counts = {key: value.get(key) for key in ('active_fallback_sections', 'inactive_fallback_sections',
                                                 'active_single_material_mapping_errors')}
        if any(count != 0 for count in counts.values()):
            problems.append('Render-audit fallback/mapping counters are not all zero: ' + str(path))
        components = value.get('components', [])
        if not components:
            problems.append('Render audit contains no component evidence')
        for component in components:
            if component.get('binding_policy') == 'authored_cow_sections':
                meshes.add(component.get('mesh'))
            for section in component.get('sections', []):
                if not section.get('ism_usage_ok') or not section.get('nanite_usage_ok'):
                    problems.append('Native section material usage failed: ' + str(path))
        reports.append(dict(file_record(path), area=value.get('area'), counters=counts,
                            component_count=len(components)))
    expected = {'/Game/Signal47/Art/Cows/' + name + '.' + name
                for name in ('SM_Cow_Alive_Body', 'SM_Cow_Alive_Head', 'SM_Cow_Abducted')}
    if (cows_off and meshes) or (not cows_off and meshes != expected):
        problems.append('Unexpected/vacuous authored-cow section policy set: ' + str(sorted(meshes)))
    logged = re.findall(r'render audit saved=(\d+) activeFallbackSections=(\d+) inactiveFallbackSections=(\d+) mappingErrors=(\d+)', log)
    if not logged or any(row != ('1', '0', '0', '0') for row in logged):
        problems.append('Render-audit log does not confirm saved=1 and zero fallback/mapping errors')
    dispatches = re.findall(r'RT residency dump dispatched=(\d+) rayTracing=(\d+) referenceBasedResidency=(\d+) profilingDir=([^\r\n]+)', log)
    dumps = sorted((leaf / 'profile').rglob('UnreferencedAlwaysResidentGeometries*.csv'))
    return {'status': 'FAIL' if problems else 'PASS', 'problems': problems, 'reports': reports,
            'authored_cow_meshes': sorted(meshes), 'log_counters': logged,
            'rt_dump_dispatches': [{'dispatched': int(row[0]), 'ray_tracing': int(row[1]),
                                    'reference_based_residency': int(row[2]), 'profiling_dir': row[3]}
                                   for row in dispatches],
            'available_unreferenced_geometry_dumps': [file_record(path) for path in dumps],
            'rt_dump_limit': 'Unreferenced geometry dump is an inventory, not total resident memory.'}


def rt_memory(leaf, log):
    """Use the existing finite-sample parser and report the unchanged real cvar queries."""
    completed = [line for line in log.splitlines() if 'Capture Ended. Writing CSV to file' in line]
    candidates = [path for path in (leaf / 'profile').rglob('*')
                  if path.is_file() and (path.name.endswith('.csv') or path.name.endswith('.csv.gz'))
                  and any(path.name in line for line in completed)]
    problems = []
    values, lines = {}, []
    queries = {'pool_mib': ('r.RayTracing.ResidentGeometryMemoryPoolSizeInMB', 400),
               'warning_percent': ('r.RayTracing.Debug.GeometryMemoryPool.AlwaysResidentWarningPercentage', 20),
               'warnings_enabled': ('r.RayTracing.ShowOnScreenWarnings', 1)}
    for key, (name, expected) in queries.items():
        found = [(line, match.group(1)) for line in log.splitlines()
                 for match in [re.search(re.escape(name) + r'\s*=\s*"?([\w.]+)', line)] if match]
        lines += [line for line, _ in found]
        raw = found[-1][1].lower() if found else None
        actual = (1 if raw == 'true' else 0 if raw == 'false' else float(raw)) if raw is not None else None
        values[key] = actual
        if actual != expected:
            problems.append('Real runtime cvar unavailable/changed: ' + name + '=' + str(raw))
    result = {'status': 'FAIL', 'problems': problems, 'actual_cvar_values': values,
              'actual_cvar_query_log_lines': lines, 'completed_csv_log_lines': completed,
              'limits': 'Finite engine-tracked world-active RT geometry sample only; not FPS or total VRAM.'}
    if len(candidates) != 1:
        problems.append('Expected exactly one completed memory CSV in isolated profile')
        return result
    output = leaf / 'rt-memory.json'
    parser_log = leaf / 'rt-memory-parser.log'
    argv = [sys.executable, str(HERE / 'parse_rt_csv.py'), str(candidates[0]),
            '--log', str(leaf / 'runtime.log'), '--out', str(output),
            '--min-samples', '120', '--margin-frames', '30', '--pool-mib', '400', '--warning-percent', '20']
    with parser_log.open('w') as stream:
        run = subprocess.run(argv, stdout=stream, stderr=subprocess.STDOUT, check=False)
    parsed = json.loads(output.read_text()) if output.is_file() else None
    if run.returncode or not parsed or parsed.get('status') != 'PASS_ENGINE_MEMORY_SAMPLE_ONLY':
        problems.append('Finite RT memory sample parser did not pass')
    result.update(status='FAIL' if problems else 'PASS_ENGINE_MEMORY_SAMPLE_ONLY',
                  csv=file_record(candidates[0]), parser_command=argv,
                  parser_report=file_record(output) if output.is_file() else None,
                  parser_log=file_record(parser_log), sample=parsed)
    return result


def command(linux, out, seconds, case):
    name, quality, flags, markers, selftest = case
    leaf = out / name
    argv = [str(linux / 'Signal47/Binaries/Linux/Signal47'), 'Signal47',
            '-UserDir=' + str(leaf / 'profile'), '-unattended', '-NoCrashDialog',
            '-RenderOffscreen', '-windowed', '-ForceRes', '-ResX=1600', '-ResY=900',
            '-S47SmokeTest', '-S47Seconds=' + str(seconds), '-S47Jump=chapter1',
            '-S47Quality=' + quality, '-S47Picture=off',
            '-S47Shot=' + str(leaf / 'view.png'),
            '-S47RenderAudit=' + str(leaf / 'audit'), '-log', *flags]
    memory_sample = name == 'alive-grazing-close-high'
    if memory_sample:
        argv += ['-csvCaptureFrames=900', '-csvCompression=0', '-csvCategories=RayTracingGeometry,VSM',
                 '-ExecCmds=r.RayTracing.ResidentGeometryMemoryPoolSizeInMB,r.RayTracing.Debug.GeometryMemoryPool.AlwaysResidentWarningPercentage,r.RayTracing.ShowOnScreenWarnings']
    return {'name': name, 'quality': quality, 'command': argv,
            'out': str(leaf), 'expected_markers': list(markers),
            'selftest': selftest, 'cows_off': '-S47CowsOff' in flags, 'rt_memory_sample': memory_sample,
            'visual_inspection': 'UNVERIFIED'}


def save(path, report):
    temporary = path.with_suffix('.writing')
    temporary.write_text(json.dumps(report, indent=2) + '\n')
    temporary.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--import-report', type=Path,
                        default=Path.home() / 'scratch-bm/Signal47-cows-20261025/native-import.json')
    parser.add_argument('--source-root', type=Path, default=PROJECT.parent,
                        help='Unreal source directory containing Signal47/ and ArtSource/; may be a frozen copy without Git')
    parser.add_argument('--git-repo', type=Path,
                        help='Optional repository for informational HEAD only; no Git metadata is required')
    parser.add_argument('--seconds', type=float, default=12)
    parser.add_argument('--only', nargs='+', choices=[row[0] for row in CASES],
                        help='Capture a selected subset; report preserves the requested scope')
    parser.add_argument('--run', action='store_true', help='Run sequential captures; otherwise print plan only')
    args = parser.parse_args()
    build, out = args.build.expanduser().resolve(), args.out.expanduser().resolve()
    source_root = args.source_root.expanduser().resolve()
    linux = build if (build / 'Signal47/Binaries/Linux/Signal47').is_file() else build / 'Linux'
    installed = (Path.home() / 'Games/Signal47').resolve()
    if installed == linux or installed in linux.parents:
        parser.error('Use a scratch candidate, not the installed game')
    if linux == out or linux in out.parents:
        parser.error('Evidence must be outside the packaged Linux directory')
    if not 9 <= args.seconds <= 60:
        parser.error('Use 9..60 visible seconds')
    if args.only and len(set(args.only)) != len(args.only):
        parser.error('Duplicate --only case')
    jobs = [command(linux, out, args.seconds, case) for case in CASES if not args.only or case[0] in args.only]
    if not args.run:
        print(json.dumps({'status': 'PLAN_ONLY', 'starts_unreal': False,
                          'source_root': str(source_root), 'jobs': jobs}, indent=2))
        return 0
    if out.exists():
        parser.error('Preserve previous evidence; choose a fresh output directory')
    # Never inherit the optional exemption for a running installed game.
    os.environ.pop(GRACE_ENV, None)
    require_idle()
    package, source = package_identity(linux), source_identity(source_root)
    imported = report_identity(args.import_report.expanduser().resolve())
    out.mkdir(parents=True, exist_ok=False)
    report_path = out / 'cow-runtime-review.json'
    git_info = None
    if args.git_repo:
        git_repo = args.git_repo.expanduser().resolve()
        result = subprocess.run(['git', '-C', str(git_repo), 'rev-parse', 'HEAD'],
                                capture_output=True, text=True, check=False)
        git_info = {'path': str(git_repo), 'informational_only': True,
                    'head': result.stdout.strip() if result.returncode == 0 else None,
                    'available': result.returncode == 0}
    report = {'status': 'INCOMPLETE', 'automated_checks': 'INCOMPLETE',
              'visual_inspection': 'UNVERIFIED', 'started_utc': datetime.now(timezone.utc).isoformat(),
              'git': git_info, 'package': package, 'source': source, 'import_report': imported,
              'requested_cases': [job['name'] for job in jobs], 'results': [],
              'limitations': ['Diagnostic teleports, not walking or human input evidence.',
                              'No FPS, GPU timing or visual approval is inferred.',
                              'Mesh-load logs prove validation/binding, not pixel visibility.',
                              'Tag/photo/frost/restore logic is covered by existing selftests; these captures do not exercise each via input.']}
    save(report_path, report)
    try:
        for job in jobs:
            if source_identity(source_root) != source or file_record(Path(imported['path'])) != {k: imported[k] for k in ('path', 'bytes', 'sha256')}:
                raise RuntimeError('Sources/import report changed before capture')
            before = package_identity(linux)
            if before != package:
                raise RuntimeError('Package changed before capture')
            leaf = Path(job['out'])
            leaf.mkdir(parents=True, exist_ok=False)
            run = run_owned(job['command'], leaf / 'runtime.log', timeout=args.seconds + 160,
                            cwd=linux / 'Signal47/Binaries/Linux')
            log = read_log(leaf / 'runtime.log')
            errors = [marker for marker in ENGINE_ERRORS if marker in log]
            problems = ['Engine error: ' + marker for marker in errors]
            markers = ['S47: smoke test OK', *job['expected_markers']]
            if job['cows_off']:
                if any(marker in log for marker in (*ALIVE_MARKERS, DEAD_MARKER)):
                    problems.append('CowsOff still loaded native cow models')
            else:
                markers += [*ALIVE_MARKERS, DEAD_MARKER]
                if 'cow models incomplete or invalid' in log:
                    problems.append('Native cow validation fell back to original models')
            problems += ['Missing log marker: ' + marker for marker in markers if marker not in log]
            checks = re.findall(r'S47: self test (OK|FAILED) \((\d+) checks\)', log)
            if job['selftest'] and (not checks or checks[-1][0] != 'OK' or int(checks[-1][1]) <= 0):
                problems.append('Selftest result/count missing or failed')
            if run['returncode'] or run['stop_reason'] or run['contention']:
                problems.append('Guarded process failed, timed out or was contended')
            audit = render_audit(leaf, log, job['cows_off'])
            problems += audit['problems']
            memory = rt_memory(leaf, log) if job['rt_memory_sample'] else {'status': 'NOT_REQUESTED'}
            if memory['status'] == 'FAIL':
                problems += memory['problems']
            after = package_identity(linux)
            if after != package or source_identity(source_root) != source or sha(Path(imported['path'])) != imported['sha256']:
                problems.append('Package/source/import identity changed during capture')
            image = None
            try:
                image = png_record(leaf / 'view.png')
            except (OSError, ValueError, struct.error, zlib.error) as error:
                problems.append(str(error))
            row = dict(job, status='FAIL' if problems else 'PASS', problems=problems,
                       run=run, image=image, runtime_log=file_record(leaf / 'runtime.log'),
                       render_audit=audit, rt_memory=memory,
                       selftest_checks=int(checks[-1][1]) if checks else None,
                       binding={'binary_sha256': package['executable']['sha256'],
                                'package_sha256': package['sha256'], 'source_sha256': source['sha256'],
                                'import_report_sha256': imported['sha256']})
            report['results'].append(row)
            save(report_path, report)
            print(job['name'], row['status'], 'visual UNVERIFIED', flush=True)
        passed = all(row['status'] == 'PASS' for row in report['results'])
        report['automated_checks'] = 'PASS' if passed else 'FAIL'
        report['status'] = 'AUTOMATED_PASS_VISUAL_UNVERIFIED' if passed else 'FAIL'
        report['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save(report_path, report)
        return 0 if passed else 1
    except BaseException as error:
        report.update(status='INCOMPLETE', automated_checks='INCOMPLETE', error=str(error))
        save(report_path, report)
        raise


if __name__ == '__main__':
    sys.exit(main())
