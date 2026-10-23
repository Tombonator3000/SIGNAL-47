#!/usr/bin/env python3
"""Isolated anomaly pilot controls. API actions/test poses; human input is UNVERIFIED."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import importlib.util
import json
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location('s47_unreal_guard', HERE / 'unreal_guard.py')
GUARD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GUARD)
SCENARIOS = ('pilot', 'late', 'legacy', 'already-seen', 'missing-phone', 'archive', 'missing-photo')
# Explicit legacy fixtures, not a claim of a continuous playthrough. Continue alone
# reuses P13's real on-disk case in the same isolated profile, via the title action.
REGRESSION = (
    ('saro', 12, ['-S47SelfTest', '-S47Jump=chapter2', '-S47Face=room', '-S47Open=notebook:findings'],
     [r'self test OK', r'chapter ch2 begins']),
    ('station01', 14, ['-S47Jump=chapter3', '-S47Ch3Drive=hut'],
     [r'chapter ch3 begins', r'ch3 drive: at the hut']),
    ('room6', 12, ['-S47Jump=chapter4', '-S47Ch4Drive=table'],
     [r'chapter ch4 begins', r'ch4 drive: table: wrong']),
    ('p13', 12, ['-S47Case=1', '-S47Jump=chapter5', '-S47Ch5Drive=p13'],
     [r'chapter ch5 begins', r'ch5 drive: map:.*p13 1']),
    ('continue', 12, ['-S47Title', '-S47Open=continue'],
     [r'title shown', r'night started, case 1, chapter5']),
    ('oldroad', 12, ['-S47Jump=chapter6'],
     [r'chapter ch6 begins']),
    ('event', 99, ['-S47Jump=chapter6:line', '-S47Auto=20'],
     [r'ch6 THE EVENT begins, 47 seconds', r'ch6 pullout begins at 05:29:47', r'ch6 ending title']),
)


def heavy_guard():
    """Strict task policy: no grace exception, no pre-existing Unreal process stopped."""
    memory = subprocess.run(['free', '-g'], capture_output=True, text=True, check=True)
    print(memory.stdout.rstrip(), flush=True)
    pids = subprocess.run(['pidof', 'UnrealEditor', 'Signal47', 'UnrealBuildTool'],
                          capture_output=True, text=True, check=False)
    if pids.returncode not in (0, 1):
        raise RuntimeError('pidof failed: ' + pids.stderr)
    print('pidof UnrealEditor Signal47 UnrealBuildTool: ' + (pids.stdout.strip() or '(none)'), flush=True)
    jobs = GUARD.heavy_jobs()
    available = GUARD.available_gib()
    if pids.stdout.strip() or jobs:
        raise RuntimeError('Unreal is busy; wait your turn. No process was stopped: ' +
                           ', '.join(f"{job['pid']} {job['exe']}" for job in jobs))
    if available < 10:
        raise RuntimeError(f'Need 10+ GiB available; found {available:.2f}. No run started.')
    return {'available_gib': available, 'heavy_jobs': jobs, 'pidof': pids.stdout,
            'performance_status': 'UNCONTENDED'}


# Reuse only the existing private-child-session cleanup; apply this task's stricter guard.
GUARD.require_idle = heavy_guard
os.environ.pop(GUARD.GRACE_ENV, None)


def png_review(path, ending_title=False):
    """Reject blank/startup evidence; these metrics do not replace visual QA."""
    from PIL import Image
    review = {'path': str(path), 'status': 'FAIL',
              'rule': 'PNG >=640x360, >5% pixels luma>8, luminance range>24'}
    if not path.is_file() or path.stat().st_size <= 1000:
        review['reason'] = 'screenshot absent or <=1000 bytes'
        return review
    if path.read_bytes()[:8] != b'\x89PNG\r\n\x1a\n':
        review['reason'] = 'file is not PNG'
        return review
    try:
        with Image.open(path) as image:
            width, height = image.size
            histogram = image.convert('L').histogram()
        total = width * height
        low = next(index for index, count in enumerate(histogram) if count)
        high = max(index for index, count in enumerate(histogram) if count)
        lit_fraction = sum(histogram[9:]) / total
        review.update(width=width, height=height, luma_min=low, luma_max=high,
                      luma_range=high-low, pixels_luma_above_8=lit_fraction,
                      mean_luma=sum(index*count for index, count in enumerate(histogram))/total)
        if ending_title:
            # The authored ending is sparse white/red lettering on black, not a
            # gameplay frame. Require its two colours and central placement as
            # well as the separately asserted THE EVENT/pullout/ending logs.
            with Image.open(path) as source:
                rgb = source.convert('RGB')
                red = sum(r > 150 and g < 150 and b < 130 for r,g,b in rgb.getdata())
                outside = rgb.copy()
                outside.paste((0,0,0), (int(width*.2),int(height*.3),int(width*.8),int(height*.72)))
                outside_lit = sum(outside.convert('L').histogram()[9:]) / total
            review.update(rule='ending card: 2..12% lit, range>200, red slash>0.1%, outside central title<0.2%',
                          red_fraction=red/total, outside_title_lit_fraction=outside_lit)
            if width >= 640 and height >= 360 and .02 < lit_fraction < .12 and high-low > 200 and red/total > .001 and outside_lit < .002:
                review['status'] = 'PASS'
            else:
                review['reason'] = 'sparse ending card fails title-specific image gate'
        elif width >= 640 and height >= 360 and lit_fraction > .05 and high-low > 24:
            review['status'] = 'PASS'
        else:
            review['reason'] = 'blank/dark/startup-sized image fails evidence readability gate'
    except (OSError, ValueError, StopIteration) as error:
        review['reason'] = 'PNG decode failed: ' + str(error)
    return review


def png_ok(path):
    return png_review(path)['status'] == 'PASS'


def write_report(out, results):
    lines = ['# Anomaly pilot, automatisk kontroll', '',
             'API-handlinger og merkede testposisjoner/tidskutt. Ekte tastaturinput, lydlytting,',
             'spillerforståelse, uro og normal reisetid er UNVERIFIED.', '',
             '| Scenario | Status | Kjøretid |', '|---|---|---|']
    for result in results:
        lines.append(f"| {result['scenario']} | {result['status']} | {result.get('wall_seconds', 0):.1f} s |")
    for result in results:
        event_path = out / result['scenario'] / 'events.json'
        if not event_path.exists():
            continue
        events = json.loads(event_path.read_text())
        lines += ['', f"## {result['scenario']}: minutt for minutt", '',
                  '| Testminutt | Fortellerklokke | Spillerhandling i kontrollen |', '|---|---|---|']
        for event in events:
            action = event['action'].replace('|', '/')
            lines.append(f"| {int(event['seconds'] // 60):02}:{int(event['seconds'] % 60):02} | {event['clock_text']} | {action} |")
    (out / 'report.md').write_text('\n'.join(lines) + '\n')
    (out / 'results.json').write_text(json.dumps(results, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    binary = parser.add_mutually_exclusive_group(required=True)
    binary.add_argument('--build', type=Path, help='packaged build root containing Linux/Signal47.sh')
    binary.add_argument('--editor', type=Path, help='UnrealEditor executable; uses this checkout project with -game')
    parser.add_argument('--out', type=Path, required=True, help='new evidence directory')
    parser.add_argument('--scenario', choices=SCENARIOS + ('all', 'regression', 'ending'), default='pilot')
    parser.add_argument('--timeout', type=float, default=600)
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--picture', choices=('vhs', 'off'), default='vhs')
    parser.add_argument('--quality', choices=('low', 'high', 'ultra'), default='high')
    args = parser.parse_args()
    out = args.out.expanduser().resolve()
    if out.exists() and any(out.iterdir()):
        parser.error('--out must be empty, to prevent stale screenshots from passing')
    out.mkdir(parents=True, exist_ok=True)
    if args.editor:
        base = [str(args.editor.expanduser().resolve()), str(HERE.parent / 'Signal47.uproject'), '-game']
    else:
        build = args.build.expanduser().resolve()
        if build == (Path.home() / 'Games/Signal47').resolve() or (Path.home() / 'Games/Signal47').resolve() in build.parents:
            parser.error('Installed game cannot be the test candidate')
        base = [str(build / 'Linux/Signal47.sh')]
    if args.scenario in ('regression', 'ending'):
        jobs = [('regression-' + name, seconds, flags, expected)
                for name, seconds, flags, expected in (REGRESSION if args.scenario == 'regression' else REGRESSION[-1:])]
        regression_profile = Path(tempfile.mkdtemp(prefix='s47-anomaly-regression-'))
    else:
        selected = list(SCENARIOS) if args.scenario == 'all' else [args.scenario]
        jobs = [(name, None, [], []) for name in selected]
        regression_profile = None
    results = []
    for scenario, seconds, flags, expected_patterns in jobs:
        evidence = out / scenario
        evidence.mkdir()
        # Profiles are new temporary directories. Only the regression's Continue
        # intentionally reuses the P13 fixture's isolated on-disk case.
        profile = regression_profile or Path(tempfile.mkdtemp(prefix='s47-anomaly-' + scenario + '-'))
        command = base + [f'-UserDir={profile}', '-windowed', '-ResX=1600', '-ResY=900', '-log',
                          f'-S47Quality={args.quality}', f'-S47Picture={args.picture}']
        if regression_profile:
            command += ['-S47SmokeTest', f'-S47Seconds={seconds}',
                        f'-S47Shot={evidence / "regression.png"}'] + flags
            expected_patterns = [r'smoke test OK'] + expected_patterns
        else:
            command += [f'-S47AnomalyRoute={scenario}', f'-S47AnomalyShots={evidence}']
            expected_patterns = [re.escape(f'S47: anomaly route {scenario} PASS COMPLETE')]
            if scenario == 'pilot':
                command.append('-S47SelfTest')
                expected_patterns.append(r'self test OK')
        print(json.dumps({'scenario': scenario, 'command': command, 'profile': str(profile)}), flush=True)
        if args.dry_run:
            results.append({'scenario': scenario, 'status': 'UNVERIFIED', 'command': command,
                            'profile': str(profile), 'wall_seconds': 0})
            continue
        run = GUARD.run_owned(command, evidence / 'runtime.log', timeout=min(args.timeout, 120) if scenario == 'archive' else args.timeout, env=os.environ.copy())
        log = (evidence / 'runtime.log').read_text(errors='replace')
        failures = [text for text in ('Critical error:', 'Assertion failed:', 'Signal 11 caught',
                                      f'S47: anomaly route {scenario} FAIL') if text in log]
        shots = ['regression'] if regression_profile else (['missing-photo'] if scenario == 'missing-photo' else ['archive'] if scenario == 'archive' else ['reference', 'anomaly', 'comparison', 'notebook'] if scenario in ('pilot', 'missing-phone') else [])
        image_reviews = {name: png_review(evidence / (name + '.png'), ending_title=scenario == 'regression-event') for name in shots}
        (evidence / 'png-review.json').write_text(json.dumps(image_reviews, indent=2) + '\n')
        missing = [name for name, review in image_reviews.items() if review['status'] != 'PASS']
        absent = [pattern for pattern in expected_patterns if not re.search(pattern, log)]
        if scenario in ('regression-p13', 'regression-continue'):
            records = []
            for saved_path in profile.rglob('save-1-auto-*.json'):
                record = json.loads(saved_path.read_text())
                state = record.get('state', {})
                if (state.get('checkpoint') == 'chapter5' and
                        (state.get('case') or {}).get('ch5', {}).get('p13') is True):
                    records.append(str(saved_path))
            if not records:
                absent.append('actual on-disk chapter5 save with case.ch5.p13=true')
            (evidence / 'disk-p13-records.json').write_text(json.dumps(records, indent=2) + '\n')
        ok = run['returncode'] == 0 and not run['stop_reason'] and not run['contention'] and not absent and not failures and not missing
        results.append(dict(run, scenario=scenario, status='PASS' if ok else 'FAIL', profile=str(profile),
                            failures=failures, missing_or_invalid_screenshots=missing, image_reviews=image_reviews, absent_log_assertions=absent,
                            command=command, evidence=str(evidence),
                            recorded_at=datetime.now(timezone.utc).isoformat()))
        write_report(out, results)
        if not ok:
            print(f'FAIL {scenario}: inspect {evidence / "runtime.log"}', file=sys.stderr)
            return 1
    write_report(out, results)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as error:
        print('BLOCKED: ' + str(error), file=sys.stderr)
        raise SystemExit(2)
