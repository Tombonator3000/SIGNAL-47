# run_blender.py <script.py> [log name] -- Blender behind the machine's reservation
import os,sys
from pathlib import Path
SP=Path(__file__).parent;P=Path('/home/tom/SIGNAL-47/unreal/Signal47')
sys.path.insert(0,str(P/'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
script=Path(sys.argv[1]).resolve();logn=sys.argv[2] if len(sys.argv)>2 else script.stem
with HeavyReservation('Lived Blender '+script.stem) as l:
 env=os.environ.copy();env.update(l.environment());os.environ.update(env)
 r=run_owned(['blender','-b','-t','2','--python-exit-code','1','--python',str(script)]+sys.argv[3:],SP/(logn+'.blog'),timeout=1200,env=env,cwd=script.parent)
print('blender RC',r.get('returncode'),round(r.get('wall_seconds',0)))
t=(SP/(logn+'.blog')).read_text(errors='replace').splitlines()
for line in t:
 if 'Traceback' in line or line.startswith('DONE') or 'Error' in line or 'FAILED' in line or line.startswith('jacket'):print(line[:200])
if r.get('returncode'):print('\n'.join(t[-20:]))
