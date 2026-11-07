#!/usr/bin/env python3
"""Import Freeform's terrain under the shared production lease and idle guard. Never stops anything.

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run_import.py --root <freeformterrain> --apply
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import time

HERE = Path(__file__).resolve().parent


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--project', type=Path, default=Path.home() / 'SIGNAL-47/unreal/Signal47')
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--only', default='', help='a word in the mesh names to import; material, table and other meshes stay')
    args = ap.parse_args()
    project = args.project.resolve()
    sys.path.insert(0, str(project / 'Scripts'))
    from production_lease import HeavyReservation
    from unreal_guard import run_owned
    root = args.root.resolve()
    output = root / 'report' / ('ue-%s' % time.strftime('%Y%m%d-%H%M%S'))
    engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
    editor = engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'
    command = [str(editor), str(project / 'Signal47.uproject'), '-run=pythonscript', '-script=' + str(HERE / 'import_terrain.py'),
               '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoSound', '-UserDir=' + str(output / 'profile'),
               '-EnablePlugins=GeometryScripting,ProceduralMeshComponent']
    print(json.dumps({'command': command, 'report': str(output / 'import.json'), 'apply': args.apply}, indent=1))
    if not args.apply:
        return 0
    if shutil.disk_usage(project).free < 110_000_000_000:
        raise RuntimeError('The import must leave the 100 GB disk reserve')
    report = output / 'import.json'
    with HeavyReservation('FREEFORM terrain import') as lease:
        output.mkdir(parents=True)     # not before: a refused run (q.py tries again) leaves nothing behind
        env = os.environ.copy()
        env.pop('S47_INSTALLED_GAME_GRACE', None)
        env.update(lease.environment())
        os.environ.update(lease.environment())
        env.update(S47_FF_APPLY='1', S47_FF_ROOT=str(root), S47_FF_REPORT=str(report), S47_FF_ONLY=args.only)
        result = run_owned(command, output / 'editor.log', timeout=5400, env=env, cwd=project)
        (output / 'process.json').write_text(json.dumps(result, indent=1) + '\n')
        data = json.loads(report.read_text()) if report.exists() else {}
        if not args.only and result['returncode'] == 0 and not result['stop_reason'] and data.get('status') == 'PASS_FREEFORM_TERRAIN':
            # the table and the meshes belong together: to the game's folder in the same reservation
            dest = project / 'Content/FreeformTerrain'
            for name in ('relief.bin', 'relief.json'):
                shutil.copyfile(root / name, dest / (name + '.tmp'))
                os.replace(dest / (name + '.tmp'), dest / name)
    print(json.dumps({'returncode': result.get('returncode'), 'stop_reason': result.get('stop_reason'), 'status': data.get('status'), 'error': data.get('error')}, indent=1))
    if result['returncode'] != 0 or result['stop_reason']:
        return 1
    return 0 if data.get('status') == 'PASS_FREEFORM_TERRAIN' else 1


if __name__ == '__main__':
    sys.exit(main())
