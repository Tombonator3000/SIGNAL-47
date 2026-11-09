#!/usr/bin/env python3
"""NPC-ANIM: prepare (Blender) and import (Unreal) one rigged townsperson, under the shared reservation. Never stops anything.

    python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 run_rigged_townsfolk.py prepare|import deputy

Output: Tom's private model folder, tripo-npc-2026-10-29/rigged-2026-11-09/<role>; Unreal: /Game/Licensed/TripoNPC/Rigged (outside Git).
The import is import_pale_one.py as it is: it reads the destination, skin, LODs, material and clips from the manifest.
"""
import json
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOOLS = Path(os.environ.get('S47_TRIPONPCS_TOOLS', str(Path.home() / 'SIGNAL-47/unreal/Signal47/Tools/triponpcs')))
sys.path.insert(0, str(TOOLS.parents[1] / 'Scripts'))
from production_lease import HeavyReservation  # noqa: E402
from unreal_guard import run_owned  # noqa: E402

ROOT = Path(os.environ.get('S47_RIGGED_ROOT', Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-npc-2026-10-29/rigged-2026-11-09'))
PROJECT = Path(os.environ.get('S47_PROJECT', Path.home() / 'SIGNAL-47/unreal/Signal47'))


def main():
    job, role = sys.argv[1], sys.argv[2]
    out = ROOT / role
    logs = ROOT / 'logs'
    logs.mkdir(parents=True, exist_ok=True)
    env_extra = {'S47_TRIPONPCS_TOOLS': str(TOOLS)}
    if job == 'prepare':
        script = HERE / 'prepare_rigged_townsfolk.py'
        command = ['blender', '-b', '-t', '4', '--python-exit-code', '1', '--python', str(script), '--', '--role', role, '--output', str(out)]
        timeout = 1800
    elif job == 'import':
        engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
        command = [str(engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'), str(PROJECT / 'Signal47.uproject'), '-run=pythonscript',
                   '-script=' + str(TOOLS / 'import_pale_one.py'), '-unattended', '-nop4', '-nosplash', '-NullRHI', '-NoSound',
                   '-UserDir=' + str(logs / 'ue-profile'), '-EnablePlugins=GeometryScripting',
                   '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0']
        env_extra['S47_PALE_ONE_PREPARED'] = str(out)
        timeout = 3600
    else:
        raise SystemExit('unknown job ' + job)
    with HeavyReservation('NPC-ANIM rigged townsfolk: ' + job + ' ' + role) as lease:
        os.environ.update(lease.environment())
        env = dict(os.environ, OMP_NUM_THREADS='4', OPENBLAS_NUM_THREADS='4', PYTHONDONTWRITEBYTECODE='1', **env_extra)
        env.pop('S47_INSTALLED_GAME_GRACE', None)
        log = logs / (job + '-' + role + '-' + time.strftime('%H%M%S') + '.log')
        result = run_owned(command, log, timeout=timeout, cwd=str(TOOLS), env=env)
    print(json.dumps({'job': job, 'role': role, 'returncode': result['returncode'], 'seconds': round(result['wall_seconds']),
                      'stop_reason': result['stop_reason'], 'log': str(log)}))
    return 0 if result['returncode'] == 0 and not result['stop_reason'] else 1


if __name__ == '__main__':
    sys.exit(main())
