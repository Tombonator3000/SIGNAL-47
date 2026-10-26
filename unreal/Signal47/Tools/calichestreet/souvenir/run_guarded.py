"""Bounded source-only authoring jobs. Never launches Unreal or an importer."""
import argparse, json, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT.parents[2]/'Scripts'))
from unreal_guard import run_owned

p=argparse.ArgumentParser()
p.add_argument('phase',choices=['build','verify','preview'])
p.add_argument('--baseline',type=Path)
p.add_argument('--reuse-textures',action='store_true')
args=p.parse_args()
script={'build':'build_souvenir.py','verify':'verify_souvenir.py','preview':'render_previews.py'}[args.phase]
command=['blender','-b','-t','2','--factory-startup','--python-exit-code','1','--python',str(ROOT/script)]
extra=[]
if args.phase=='verify' and args.baseline: extra=['--baseline',str(args.baseline.resolve())]
if args.phase=='build' and args.reuse_textures: extra=['--reuse-textures']
if args.phase=='preview': extra=['--quick']
if extra: command+=['--',*extra]
result=run_owned(command,str(ROOT/('authoring-'+args.phase+'.log')),timeout=480 if args.phase=='build' else 180)
report_ok=args.phase!='verify' or json.loads((ROOT/'verification.json').read_text())['status']=='PASS'
proof_path=ROOT/'authoring_guard.json'
proof=json.loads(proof_path.read_text()) if proof_path.exists() else {}
proof[args.phase]={'returncode':result['returncode'],'wall_seconds':result['wall_seconds'],
 'stop_reason':result['stop_reason'],'contention':bool(result['contention']),
 'available_gib':result['preflight']['available_gib'],
 'status':'PASS' if result['returncode']==0 and not result['stop_reason'] and report_ok else 'FAIL'}
proof_path.write_text(json.dumps(proof,indent=2)); print('SOUVENIR_GUARD',args.phase,proof[args.phase])
if proof[args.phase]['status']!='PASS': raise SystemExit(1)
