"""Sequential, owned Blender source/export/preview job behind the project's guard."""
import argparse,importlib.util,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parent
ap=argparse.ArgumentParser();ap.add_argument('action',choices=['build','verify','render','refine']);args=ap.parse_args()
os.environ.pop('S47_INSTALLED_GAME_GRACE',None)
spec=importlib.util.spec_from_file_location('s47_guard',ROOT.parents[2]/'Scripts'/'unreal_guard.py')
guard=importlib.util.module_from_spec(spec);spec.loader.exec_module(guard)
command=['blender','-b','-t','2','--factory-startup','--python-exit-code','1','--python',str(ROOT/({'build':'build_station.py','verify':'verify_station.py','render':'render_revision_two.py','refine':'refine_revision_two.py'}[args.action]))]
evidence=ROOT/'verification';evidence.mkdir(exist_ok=True)
result=guard.run_owned(command,evidence/('r2-'+args.action+'.log'),timeout=900,cwd=str(ROOT))
(evidence/('r2-'+args.action+'-guard.json')).write_text(json.dumps(result,indent=2));print('R2_GUARD',json.dumps(result))
if result['returncode'] or result['stop_reason']:raise SystemExit(1)
log=(evidence/('r2-'+args.action+'.log')).read_text()
if 'Traceback (most recent call last)' in log:raise RuntimeError('Blender reported Python exception; source result rejected')

if args.action=='verify':
 report=json.loads((ROOT/'verification.json').read_text())
 if report['status']!='PASS' or report['passed']!=report['total']:raise RuntimeError('Verification report is not complete PASS')
