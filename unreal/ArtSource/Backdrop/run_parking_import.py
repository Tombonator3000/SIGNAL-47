"""Build SARO parking materials using existing texture assets, with idle/lease guards."""
from pathlib import Path
import argparse
import json
import os
import shutil
import sys

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1] / 'Signal47'
sys.path.insert(0, str(PROJECT / 'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.expanduser().resolve()
    engine = Path(os.environ.get('UE_ROOT', str(Path.home() / '.local/opt/UnrealEngine-5.8.4')))
    editor = engine / 'Engine/Binaries/Linux/UnrealEditor-Cmd'
    command = [str(editor), str(PROJECT / 'Signal47.uproject'), '-run=pythonscript',
               '-script=' + str(HERE / 'import_parking.py'), '-unattended', '-nop4', '-nosplash',
               '-NullRHI', '-NoSound', '-UserDir=' + str(output / 'profile')]
    report = output / 'import.json'
    print(json.dumps({'command': command, 'report': str(report), 'apply': args.apply}, indent=2))
    if not args.apply:
        return 0
    if not editor.is_file():
        raise RuntimeError('Unreal commandlet editor is missing')
    if shutil.disk_usage(PROJECT).free < 110_000_000_000:
        raise RuntimeError('Parking material import must retain the 100 GB reserve plus 10 GB allowance')
    if output.exists():
        raise RuntimeError('Use a fresh output directory')
    output.mkdir(parents=True)
    with HeavyReservation('SARO continuous parking materials') as lease:
        os.environ.pop('S47_INSTALLED_GAME_GRACE', None)
        os.environ.update(lease.environment())
        env = os.environ.copy()
        env.update(S47_PARKING_APPLY='1', S47_PARKING_REPORT=str(report))
        result = run_owned(command, output / 'editor.log', timeout=1800, env=env, cwd=PROJECT)
        (output / 'process.json').write_text(json.dumps(result, indent=2) + '\n')
    if result['returncode'] != 0 or result['stop_reason']:
        return 1
    data = json.loads(report.read_text()) if report.exists() else {}
    return 0 if data.get('status') == 'PASS_PARKING_MATERIALS' else 1


if __name__ == '__main__':
    sys.exit(main())
