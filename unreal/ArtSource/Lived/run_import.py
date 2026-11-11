# run_import.py <set> <root> -- the editor's python importer behind the reservation
import os,sys,json
from pathlib import Path
SP=Path(__file__).parent;P=Path('/home/tom/SIGNAL-47/unreal/Signal47')
sys.path.insert(0,str(P/'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
kind,root=sys.argv[1],str(Path(sys.argv[2]).resolve())
ue=Path.home()/'.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd'
if not ue.exists(): ue=ue.with_name('UnrealEditor')
with HeavyReservation('Lived import '+kind) as l:
 env=os.environ.copy();env.update(l.environment());os.environ.update(env)
 r=run_owned([str(ue),str(P/'Signal47.uproject'),'-run=pythonscript','-script='+str(SP/'src/import_lived.py'),'-S47LivedSet='+kind,'-S47LivedRoot='+root,'-nullrhi','-unattended','-nosplash','-NoSound','-AllowCommandletRendering'],SP/('import-'+kind+'.log'),timeout=1800,env=env,cwd=P)
print('import RC',r.get('returncode'),round(r.get('wall_seconds',0)))
rep=P/'Saved'/('lived-'+kind+'-import.json')
if rep.exists():
 d=json.loads(rep.read_text());print(d['status'],len(d['checks']),'checks',d['meshes'])
 for c in d['checks']:
  if not c['pass']:print('FAIL',c)
log=(SP/('import-'+kind+'.log')).read_text(errors='replace').splitlines()
for line in log:
 if 'Failed to compile' in line or 'S47 LIVED' in line or ('Error:' in line and 'LogPython' in line):print(line[:240])
