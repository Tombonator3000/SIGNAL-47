#!/usr/bin/env python3
"""One exclusive native-input run, with an isolated case archive and fresh evidence."""
import argparse, fcntl, json, os, shutil, subprocess, time, threading
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--name',required=True);p.add_argument('--profile',required=True);p.add_argument('--hold-on-fail',action='store_true');p.add_argument('--audio',action='store_true');p.add_argument('--player',default=str(ROOT/'Artifacts/GauntletLinux/Signal47.x86_64'));p.add_argument('journey_args',nargs=argparse.REMAINDER);a=p.parse_args()
out=ROOT/'Artifacts/Chapter09'/a.name;out.mkdir(parents=True,exist_ok=True)
player=Path(a.player).resolve();telemetry=player.parent.parent/'Gauntlet';telemetry.mkdir(parents=True,exist_ok=True)
profile=Path(a.profile).resolve();profile.mkdir(parents=True,exist_ok=True)
runtime=f'/run/user/{os.getuid()}';lock=open(runtime+'/signal47-gauntlet.lock','w');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
assert subprocess.run(['pgrep','-x','Signal47.x86_64'],stdout=subprocess.DEVNULL).returncode!=0,'Another SIGNAL 47 player is running; no input sent.'
env=os.environ.copy();env['XDG_RUNTIME_DIR']=runtime;env['PULSE_SERVER']='unix:'+runtime+'/pulse/native'
env['SIGNAL47_OBSERVER_DIR']=str(telemetry)
found=False
for sock in Path('/tmp/.X11-unix').glob('X*'):
 for auth in [Path.home()/'.Xauthority',*Path(runtime).glob('*xauth*')]:
  if not auth.is_file():continue
  env.update(DISPLAY=':'+sock.name[1:],XAUTHORITY=str(auth))
  if subprocess.run(['xdpyinfo'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0:found=True;break
 if found:break
assert found,'Authenticated desktop unavailable'
# Fail before launching a player or clearing telemetry when the desktop is locked.
import importlib.util
desktop_spec=importlib.util.spec_from_file_location('signal47_desktop_preflight',ROOT/'Automation/gauntlet-user-journey.py')
desktop_module=importlib.util.module_from_spec(desktop_spec);desktop_spec.loader.exec_module(desktop_module)
desktop_module.Desktop.require_unlocked()
for pattern in ['state.json','command.txt','performance.json','frames.csv','frame-work.csv','chapter09-result.json','journey-*.png','audio-audit.json']:
 for path in telemetry.glob(pattern):path.unlink()
manifest=player.parent/'build-manifest.json'
if manifest.exists():shutil.copy2(manifest,out/'build-manifest.json')
before={path.name:__import__('hashlib').sha256(path.read_bytes()).hexdigest() for path in profile.glob('case*.json')}
game=subprocess.Popen([str(player),'--signal47-gauntlet',*(['--signal47-audio-audit'] if a.audio else []),'--signal47-save-dir',str(profile),'-screen-width','1280','-screen-height','800','-screen-fullscreen','0','-logFile',str(out/'player.log')],env=env,stdout=open(out/'startup.log','w'),stderr=subprocess.STDOUT)
normal_exit=False;code=1;memory=[];sampling_done=threading.Event()
def sample_memory():
 while not sampling_done.is_set():
  try:
   values={line.split(':',1)[0]:line.split(':',1)[1].strip() for line in Path(f'/proc/{game.pid}/status').read_text().splitlines() if ':' in line}
   memory.append({'wall':time.time(),'rssKiB':int(values['VmRSS'].split()[0]),'peakRssKiB':int(values['VmHWM'].split()[0])})
  except (OSError,KeyError,ValueError):pass
  sampling_done.wait(.5)
sampler=threading.Thread(target=sample_memory,daemon=True);sampler.start()
try:
 for _ in range(200):
  if (telemetry/'state.json').exists():break
  assert game.poll() is None,'Player exited before observer state'
  time.sleep(.15)
 else:raise RuntimeError('No fresh observation state')
 args=a.journey_args[1:] if a.journey_args[:1]==['--'] else a.journey_args
 run=subprocess.run(['python3',str(ROOT/'Automation/chapter09-journey.py'),*args],env=env)
 code=run.returncode
 if code==0 and '--quit' in args:
  game.wait(timeout=20);normal_exit=game.returncode==0
  if not normal_exit:code=1
 elif code and a.hold_on_fail:
  print(f'Player held for native diagnosis, pid={game.pid}',flush=True);game.wait()
finally:
 sampling_done.set();sampler.join(timeout=1)
 if game.poll() is None:
  game.terminate()
  try:game.wait(timeout=8)
  except subprocess.TimeoutExpired:game.kill();game.wait()
 for pattern in ['state.json','performance.json','frames.csv','frame-work.csv','chapter09-result.json','journey-*.png','audio-audit.json']:
  for path in telemetry.glob(pattern):shutil.copy2(path,out/path.name)
 after={path.name:__import__('hashlib').sha256(path.read_bytes()).hexdigest() for path in profile.glob('case*.json')}
 (out/'process.json').write_text(json.dumps({'pid':game.pid,'processExit':game.returncode,'normalMenuQuit':normal_exit,'profile':str(profile),'caseHashesBefore':before,'caseHashesAfter':after,'journeyExit':code},indent=2))
 (out/'process-memory.json').write_text(json.dumps({'method':'Linux /proc/PID/status sampled every0.5s across entire player lifetime, including explicit loading; VmHWM is process resident peak, not Unity engine memory.','samples':memory},indent=2))
raise SystemExit(code)
