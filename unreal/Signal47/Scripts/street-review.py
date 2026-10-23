#!/usr/bin/env python3
"""Plan or run the six packaged Caliche street reviews, sequentially.

Default is a read-only plan, printed as JSON; only --run launches the guarded graphics driver.
Each leaf captures four unchanged player-height cameras at one exact clock/timeline. This is
static visual evidence: the cinematic guard holds narrative events, while normal clock-driven
lights and scheduled presence remain active. It is not a walking, interaction or audio proof.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys


def plan(args):
    driver = Path(__file__).with_name('graphics-review.py')
    rows = []
    for timeline in args.timelines:
        for clock, wall, seconds in [('day', '12:00', 43200), ('dusk', '18:30', 66600), ('night', '02:15', 8100)]:
            signed = seconds - 86400 if timeline == 'evening' and seconds >= 43200 else seconds
            out = args.out.expanduser().resolve() / f'{timeline}-{clock}'
            command = [sys.executable, str(driver.resolve()), '--build', str(args.build.expanduser().resolve()),
                       '--out', str(out), '--suite', 'street', '--timeline', timeline, '--clock', clock,
                       '--quality', args.quality, '--picture', args.picture, '--batch-size', '4',
                       '--warmup', '8', '--hold', '7', '--width', str(args.width), '--height', str(args.height)]
            if args.resume:
                command += ['--resume']
            if args.driver_cache:
                command += ['--driver-cache', str(args.driver_cache.expanduser().resolve())]
            rows.append({'timeline': timeline, 'clock': wall, 'clock_seconds': signed,
                         'static_world': True, 'views': ['street-west-front', 'street-east-front',
                         'street-north-route', 'street-souvenir-window'], 'out': str(out), 'command': command})
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--run', action='store_true', help='run sequential guarded processes; otherwise only print the plan')
    ap.add_argument('--timelines', nargs='+', choices=['legacy', 'evening'], default=['legacy', 'evening'])
    ap.add_argument('--quality', choices=['low', 'high', 'ultra'], default='high')
    ap.add_argument('--picture', choices=['vhs', 'off'], default='vhs')
    ap.add_argument('--width', type=int, default=2560)
    ap.add_argument('--height', type=int, default=1440)
    ap.add_argument('--resume', action='store_true')
    ap.add_argument('--driver-cache', type=Path)
    args = ap.parse_args()
    if len(args.timelines) != len(set(args.timelines)):
        ap.error('duplicate timeline')
    rows = plan(args)
    if not args.run:
        print(json.dumps({'status': 'PLAN_ONLY', 'runs': rows}, indent=2))
        return 0
    args.out.mkdir(parents=True, exist_ok=True)
    summary = {'status': 'INCOMPLETE', 'test_kind': 'static player-height visual review', 'runs': rows, 'results': []}
    destination = args.out / 'street-matrix.json'
    destination.write_text(json.dumps(summary, indent=2) + '\n')
    for row in rows:
        result = subprocess.run(row['command'], check=False)
        metadata = Path(row['out']) / 'meta.json'
        evidence = json.loads(metadata.read_text()) if metadata.is_file() else {}
        passed = result.returncode == 0 and evidence.get('valid_evidence') and evidence.get('status') == 'PASS'
        summary['results'].append({'timeline': row['timeline'], 'clock': row['clock'],
                                   'out': row['out'], 'status': 'PASS' if passed else 'FAIL',
                                   'visual_inspection': 'UNVERIFIED'})
        summary['status'] = 'CAPTURES_COMPLETE_VISUAL_UNVERIFIED' if passed and len(summary['results']) == len(rows) else 'INCOMPLETE'
        destination.write_text(json.dumps(summary, indent=2) + '\n')
        if not passed:
            return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
