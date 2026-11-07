#!/usr/bin/env python3
"""Import /Game/RoadEdge under the shared production lease and idle guard. Never stops anything.

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run_import.py --root <folder with T_RoadEdge_M.png> --apply
"""
import argparse
import json
import os
from pathlib import Path
import sys
import time

HERE = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--project', type=Path, default=Path.home() / 'SIGNAL-47/unreal/Signal47')
    ap.add_argument('--apply', action='store_true')
    args = ap.parse_args()
    project = args.project.resolve()
    sys.path.insert(0, str(project / 'Scripts'))
    from production_lease import HeavyReservation
    from unreal_guard import run_owned
    root = args.root.resolve()
    output = root / 'report' / ('ue-%s' % time.strftime('%Y%m%d-%H%M%S'))
    engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
    command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(project / 'Signal47.uproject'), '-run=pythonscript', '-script=' + str(HERE / 'import_assets.py'),
               '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoSound', '-UserDir=' + str(output / 'profile')]
    print(json.dumps({'command': command, 'report': str(output / 'import.json'), 'apply': args.apply}, indent=1))
    if not args.apply:
        return 0
    report = output / 'import.json'
    with HeavyReservation('ROADEDGE import') as lease:
        output.mkdir(parents=True)     # not before: a refused run (q.py tries again) leaves nothing behind
        env = os.environ.copy()
        env.pop('S47_INSTALLED_GAME_GRACE', None)
        env.update(lease.environment())
        os.environ.update(lease.environment())
        env.update(S47_RE_ROOT=str(root), S47_RE_REPORT=str(report))
        result = run_owned(command, output / 'editor.log', timeout=1800, env=env, cwd=project)
    data = json.loads(report.read_text()) if report.exists() else {}
    print(json.dumps({'returncode': result.get('returncode'), 'stop_reason': result.get('stop_reason'), 'status': data.get('status'), 'error': data.get('error'), 'log': str(output / 'editor.log')}, indent=1))
    return 0 if result['returncode'] == 0 and not result['stop_reason'] and data.get('status') == 'PASS_ROADEDGE' else 1


if __name__ == '__main__':
    sys.exit(main())
