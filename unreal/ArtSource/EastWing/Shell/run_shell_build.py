"""Build the EastWing shell sources (build_shell.py -> exports/) under the shared lease and idle guard."""
from pathlib import Path
import json, os, sys
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[2] / 'Signal47/Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
log = Path.home() / 'scratch-bm/Signal47-eastwing-props-20261103'; log.mkdir(exist_ok=True)
with HeavyReservation('EASTWING-01 shell build (steps)') as lease:
    env = os.environ.copy(); env.update(lease.environment()); os.environ.update(lease.environment())
    cmd = [str(Path.home() / '.local/bin/blender'), '-b', '-t', '4', '--python-exit-code', '1', '--python', str(HERE / 'build_shell.py')]
    result = run_owned(cmd, log / 'build_shell.log', timeout=1800, env=env, cwd=HERE)
print(json.dumps({k: result[k] for k in ('returncode', 'stop_reason')}))
sys.exit(result['returncode'] or (1 if result['stop_reason'] else 0))
