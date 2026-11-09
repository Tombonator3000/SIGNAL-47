#!/usr/bin/env python3
"""Run the Pale One jobs under the shared reservation and idle guard. Never stops anything.

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run_pale_one.py inspect|prepare|import [pale|tall|roswell]

Blender steps run from this worktree's script against the private model folder. The Unreal import runs against the
main project (Licensed lies outside Git and is read by every copy from there); the importer's own checks decide PASS.
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

ROOT = Path(os.environ.get('S47_PALE_ONE_DIR', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/pale-one-2026-11-07'))
KIND = sys.argv[2] if len(sys.argv) > 2 else 'pale'
# Tom's Tripo aliens of 9 November (TRIPO-NEW) live in their own private folder; their output goes there too.
NEW = {'zet': 'zet.glb', 'nordic': 'nordic.glb', 'cosmic': 'cosmic.glb'}
if KIND in NEW:
    ROOT = Path(os.environ.get('S47_TRIPO_NEW_ROOT', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-new-2026-11-09'))
SOURCE = ROOT / 'raw' / NEW[KIND] if KIND in NEW else ROOT / {'roswell': 'Roswell.glb', 'tall': 'Tall-One.glb'}.get(KIND, 'Pale-One.glb')
SUFFIX = '' if KIND == 'pale' else '-' + KIND
PREPARED = ROOT / ('prepared' + SUFFIX)
PROJECT = Path(os.environ.get('S47_PROJECT', Path.home() / 'SIGNAL-47/unreal/Signal47'))


def blender(script, *args):
    return ['blender', '-b', '-t', '4', '--python-exit-code', '1', '--python', str(HERE / script), '--', *map(str, args)]


def main():
    job = sys.argv[1]
    logs = ROOT / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    if job == 'inspect':
        command, env_extra, timeout = blender('inspect_pale_one.py', SOURCE, ROOT / ('inspect' + SUFFIX)), {}, 1800
    elif job == 'prepare':
        command, env_extra, timeout = blender('prepare_pale_one.py', SOURCE, PREPARED, '--kind', KIND), {}, 1200
    elif job in ('import', 'readback'):
        engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
        command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(PROJECT / 'Signal47.uproject'), '-run=pythonscript',
                   '-script=' + str(HERE / 'import_pale_one.py'), '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoSound',
                   '-UserDir=' + str(ROOT / 'logs' / 'ue-profile'), '-EnablePlugins=GeometryScripting',
                   '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0']
        env_extra, timeout = {'S47_PALE_ONE_PREPARED': str(PREPARED)}, 3600
        if job == 'readback':
            env_extra['S47_PALE_ONE_READ_BACK'] = '1'
    else:
        raise SystemExit('unknown job ' + job)
    with HeavyReservation('Pale One: ' + job) as lease:
        os.environ.update(lease.environment())
        env = dict(os.environ, OMP_NUM_THREADS='4', OPENBLAS_NUM_THREADS='4', PYTHONDONTWRITEBYTECODE='1', **env_extra)
        env.pop('S47_INSTALLED_GAME_GRACE', None)
        log = logs / (job + '-' + KIND + '-' + time.strftime('%H%M%S') + '.log')
        result = run_owned(command, log, timeout=timeout, cwd=str(HERE), env=env)
    print(json.dumps({'job': job, 'returncode': result['returncode'], 'seconds': round(result['wall_seconds']), 'stop_reason': result['stop_reason'], 'log': str(log)}))
    return 0 if result['returncode'] == 0 and not result['stop_reason'] else 1


if __name__ == '__main__':
    sys.exit(main())
