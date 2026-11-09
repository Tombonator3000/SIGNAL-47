#!/usr/bin/env python3
"""Run the TripoNew jobs under the shared reservation and idle guard. Never stops anything.

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run.py prepare [--only zet,nordic] [--previews]
    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run.py eyes zet|nordic|cosmic
    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run.py import [singer,zet]

Blender steps run this checkout's scripts against the private folder (S47_TRIPO_NEW_ROOT). The Unreal import runs against the
main project (Licensed lies outside Git and is read by every copy from there) with triponpcs/import_assets.py, sent to
/Game/Licensed/TripoNew; the importer's own checks decide PASS. Logs and the editor profile stay in the private folder.
"""
import json
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'Scripts'))
from production_lease import HeavyReservation  # noqa: E402
from unreal_guard import run_owned  # noqa: E402

ROOT = Path(os.environ.get('S47_TRIPO_NEW_ROOT', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-new-2026-11-09'))
PROJECT = Path(os.environ.get('S47_PROJECT', Path.home() / 'SIGNAL-47/unreal/Signal47'))


def blender(script, *args):
    return ['blender', '-b', '-t', '4', '--python-exit-code', '1', '--python', str(script), '--', *map(str, args)]


def main():
    job = sys.argv[1]
    rest = sys.argv[2:]
    logs = ROOT / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    env_extra = {'S47_TRIPO_NEW_ROOT': str(ROOT)}
    if job == 'prepare':
        command, timeout = blender(HERE / 'prepare.py', *rest), 1800
    elif job == 'eyes':
        manifest = json.loads((ROOT / 'prepared/manifest.json').read_text())
        row = next(r for r in manifest['assets'] if r['role'] == rest[0])
        shot = ROOT / 'prepared' / row['role'] / 'previews' / 'eyes.png'
        command, timeout = blender(HERE.parent / 'triponpcs' / 'measure_eyes.py', ROOT / 'prepared' / row['eyes_blend'], '--dark',
                                   rest[1] if len(rest) > 1 else '.12', '--render', shot), 900
    elif job == 'import':
        engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
        command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(PROJECT / 'Signal47.uproject'), '-run=pythonscript',
                   '-script=' + str(HERE.parent / 'triponpcs' / 'import_assets.py'), '-unattended', '-nop4', '-nosplash', '-NullRHI',
                   '-NoSound', '-UserDir=' + str(logs / 'ue-profile'),
                   '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0']
        env_extra.update({'S47_TRIPO_STATIC_DIR': str(ROOT / 'prepared'), 'S47_TRIPO_DEST': '/Game/Licensed/TripoNew',
                          'S47_FIELD_REPORT': str(logs / 'import-report.json')})
        if rest:
            env_extra['S47_TRIPO_ONLY'] = rest[0]
        timeout = 3600
    else:
        raise SystemExit('unknown job ' + job)
    with HeavyReservation('TripoNew: ' + job) as lease:
        os.environ.update(lease.environment())
        env = dict(os.environ, OMP_NUM_THREADS='4', OPENBLAS_NUM_THREADS='4', PYTHONDONTWRITEBYTECODE='1', **env_extra)
        env.pop('S47_INSTALLED_GAME_GRACE', None)
        log = logs / (job + '-' + time.strftime('%H%M%S') + '.log')
        result = run_owned(command, log, timeout=timeout, cwd=str(HERE), env=env)
    print(json.dumps({'job': job, 'returncode': result['returncode'], 'seconds': round(result['wall_seconds']),
                      'stop_reason': result['stop_reason'], 'log': str(log)}))
    return 0 if result['returncode'] == 0 and not result['stop_reason'] else 1


if __name__ == '__main__':
    sys.exit(main())
