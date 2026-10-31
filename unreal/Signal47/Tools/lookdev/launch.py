"""Run lookdev Blender through the project's existing heavy-job guard.

Arguments following -- go to pipeline.py. No shell interpolation or installs.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--blender', default='blender')
    parser.add_argument('--timeout', type=int, default=600)
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    forwarded = args.args[1:] if args.args[:1] == ['--'] else args.args
    if '--output' not in forwarded or forwarded.index('--output') + 1 == len(forwarded):
        parser.error('Pass pipeline --output with a new private directory')
    output = Path(forwarded[forwarded.index('--output') + 1]).expanduser().absolute()
    if output.exists(): parser.error('Output directory already exists; choose a new directory')
    executable = shutil.which(args.blender)
    if not executable: parser.error('Blender executable not found')
    if not 1 <= args.timeout <= 7200: parser.error('Timeout must be 1..7200 seconds')
    here = Path(__file__).resolve().parent
    module_spec = importlib.util.spec_from_file_location('s47_lookdev_guard', here.parents[1] / 'Scripts/unreal_guard.py')
    guard = importlib.util.module_from_spec(module_spec); module_spec.loader.exec_module(guard)
    command = [executable, '--background', '--factory-startup', '-Y', '--threads', '4',
               '--python-exit-code', '1', '--python', str(here / 'pipeline.py'), '--', *forwarded]
    env = os.environ.copy()
    # This asset tool never uses the historical installed-game grace exception.
    env.pop('S47_INSTALLED_GAME_GRACE', None)
    os.environ.pop('S47_INSTALLED_GAME_GRACE', None)
    result = guard.run_owned(command, output.with_name(output.name + '.blender.log'),
                             timeout=args.timeout, env=env)
    print(json.dumps(result, indent=2))
    if result['returncode'] or result['stop_reason'] or result['contention']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
