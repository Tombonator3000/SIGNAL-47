"""Run a selected EastWing Blender source under global lease and idle guard."""
from pathlib import Path
import sys,os,json
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[3]
sys.path.insert(0,str(REPO/'unreal/Signal47/Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
script=Path(sys.argv[1]).resolve()
assert script.is_relative_to(ROOT)
logs=Path.home()/'scratch-bm/Signal47-eastwing-props-20261103';logs.mkdir(exist_ok=True)
with HeavyReservation('EASTWING-01 Codex props '+script.stem) as lease:
 env=os.environ.copy();env.update(lease.environment());os.environ.update(lease.environment())
 command=['/home/tom/.local/bin/blender','-b','-t','4','--python-exit-code','1','--python',str(script)]
 result=run_owned(command,logs/(script.stem+'.log'),timeout=1800,env=env,cwd=ROOT)
 (logs/(script.stem+'-result.json')).write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result));sys.exit(result['returncode'] or (1 if result['stop_reason'] else 0))
