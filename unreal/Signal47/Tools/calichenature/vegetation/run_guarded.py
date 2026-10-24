"""Owned serial 2-thread DCC source jobs; explicit slot authorization required."""
import argparse,importlib.util,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parent
ap=argparse.ArgumentParser();ap.add_argument('action',choices=['build','verify','preview']);ap.add_argument('--reuse-textures',action='store_true');args=ap.parse_args()
os.environ.pop('S47_INSTALLED_GAME_GRACE',None)
spec=importlib.util.spec_from_file_location('s47_guard',ROOT.parents[2]/'Scripts'/'unreal_guard.py');guard=importlib.util.module_from_spec(spec);spec.loader.exec_module(guard)
cmd=['blender','-b','-t','2','--factory-startup','--python-exit-code','1','--python',str(ROOT/{'build':'build_vegetation.py','verify':'verify_vegetation.py','preview':'preview_botanical.py'}[args.action])]
if args.reuse_textures:cmd+=['--','--reuse-textures']
e=ROOT/'verification';e.mkdir(exist_ok=True);result=guard.run_owned(cmd,e/(args.action+'.log'),timeout=600,cwd=str(ROOT));(e/(args.action+'-guard.json')).write_text(json.dumps(result,indent=2));print('GUARD',json.dumps(result))
assert result['returncode']==0 and result['stop_reason'] is None
assert 'Traceback (most recent call last)' not in (e/(args.action+'.log')).read_text()
if args.action=='verify':
 report=json.loads((ROOT/'verification.json').read_text());assert report['status']=='PASS' and report['passed']==report['total']

from update_manifest import update
update()
