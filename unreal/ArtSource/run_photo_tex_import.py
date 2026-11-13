# run_photo_tex_import.py <set> <texture dir> -- photo_tex_import.py (saro) or photo_town_import.py (town; the dir is
# `photo_tex.py town <dir>`'s) in the editor's python, behind the reservation
# (run it through ~/.claude/skills/signal47-nattskift/scripts/q.py). Log: Saved/photo-tex-import.log.
import json,os,sys
from pathlib import Path
SP=Path(__file__).resolve().parent;P=Path('/home/tom/SIGNAL-47/unreal/Signal47')
sys.path.insert(0,str(P/'Scripts'))
from production_lease import HeavyReservation
from unreal_guard import run_owned
kind,lived=sys.argv[1],str(Path(sys.argv[2]).resolve())
ue=Path.home()/'.local/opt/UnrealEngine-5.8.4/Engine/Binaries/Linux/UnrealEditor-Cmd'
if not ue.exists(): ue=ue.with_name('UnrealEditor')
log=P/'Saved'/'photo-tex-import.log';rep=P/'Saved'/'photo-tex-import.json'
if rep.exists():rep.unlink()
with HeavyReservation('Photo texture import '+kind) as l:
 env=os.environ.copy();env.update(l.environment());os.environ.update(env)
 r=run_owned([str(ue),str(P/'Signal47.uproject'),'-run=pythonscript','-script='+str(SP/('photo_town_import.py' if kind=='town' else 'photo_tex_import.py')),'-S47PhotoSet='+kind,'-S47PhotoLived='+lived,'-nullrhi','-unattended','-nosplash','-NoSound','-AllowCommandletRendering'],log,timeout=1800,env=env,cwd=P)
print('import RC',r.get('returncode'),round(r.get('wall_seconds',0)))
for line in log.read_text(errors='replace').splitlines():
 if 'S47 PHOTOTEX' in line or ('Error:' in line and 'LogPython' in line):print(line[-200:])
if rep.exists():
 d=json.loads(rep.read_text());print(d['status'],len(d['textures']),'textures')
 for t in d['textures']+d.get('materials',[]):
  if not t['pass']:print('FAIL',t)
 sys.exit(0 if d['status']=='PASS' else 1)
sys.exit(1)
