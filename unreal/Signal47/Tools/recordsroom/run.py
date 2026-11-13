# run.py blender <asset> [<asset> ...]   Blender (build_records.py) behind the machine's reservation
# run.py import [<what> ...]             the editor's python (import_assets.py) behind it
# Through the queue: python3 ~/.claude/skills/signal47-nattskift/scripts/q.py python3 Tools/recordsroom/run.py ...
# Work folder: S47_REC_WORK (default ~/scratch-bm/records-room-20261113/work); logs land there.
import os, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
P = HERE.parent.parent
sys.path.insert(0, str(P / 'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
WORK = Path(os.path.expanduser(os.environ.get('S47_REC_WORK', '~/scratch-bm/records-room-20261113/work'))).resolve()
WORK.mkdir(parents=True, exist_ok=True)
kind, rest = sys.argv[1], sys.argv[2:]
if kind == 'blender':
    log = WORK / ('blender-' + '-'.join(rest or ['all']) + '.log')
    with HeavyReservation('Records room Blender') as l:
        env = os.environ.copy(); env.update(l.environment()); os.environ.update(env)
        r = run_owned(['blender', '-b', '-t', '6', '--python-exit-code', '1', '--python', str(HERE / 'build_records.py'), '--', str(WORK)] + rest, log, timeout=3000, env=env, cwd=HERE)
    print('blender RC', r.get('returncode'), round(r.get('wall_seconds', 0)))
    t = log.read_text(errors='replace').splitlines()
    for line in t:
        if 'Traceback' in line or line.startswith('DONE') or 'S47 REC: exported' in line or 'Error' in line:
            print(line[:230])
    if r.get('returncode'):
        print('\n'.join(t[-25:]))
    sys.exit(1 if r.get('returncode') else 0)
elif kind == 'import':
    ue = Path.home() / '.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd'
    if not ue.exists():
        ue = ue.with_name('UnrealEditor')
    log = WORK / 'import.log'
    rep = WORK / 'import-report.json'
    if rep.exists():
        rep.unlink()
    with HeavyReservation('Records room import') as l:
        env = os.environ.copy(); env.update(l.environment()); env['S47_REC_WORK'] = str(WORK); env['S47_REC_WHAT'] = ','.join(rest); os.environ.update(env)
        r = run_owned([str(ue), str(P / 'Signal47.uproject'), '-run=pythonscript', '-script=' + str(HERE / 'import_assets.py'), '-unattended', '-nullrhi', '-nosplash', '-NoSound',
                       '-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.FBX=0'], log, timeout=2400, env=env, cwd=P)
    print('import RC', r.get('returncode'), round(r.get('wall_seconds', 0)))
    for line in log.read_text(errors='replace').splitlines():
        if 'S47: REC' in line or ('Error:' in line and 'LogPython' in line):
            print(line[-260:])
    import json
    if rep.exists():
        d = json.loads(rep.read_text())
        print(d['status'], len(d.get('meshes', [])), 'meshes', len(d.get('checks', [])), 'checks')
        for c in d.get('checks', []):
            if not c['pass']:
                print('FAIL', c)
        if d.get('error'):
            print(d['error'][-1500:])
        sys.exit(0 if d['status'] == 'PASS' else 1)
    sys.exit(1)
