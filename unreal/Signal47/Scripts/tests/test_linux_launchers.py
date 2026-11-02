#!/usr/bin/env python3
"""Fast Linux launcher fixtures; never starts Unreal or touches personal Saved."""
from pathlib import Path
import tempfile,subprocess,os,shutil,json,hashlib
repo=Path(__file__).resolve().parents[4]; src=repo/'unreal/Signal47/Build/Linux/launch'
root=Path(tempfile.mkdtemp(prefix='s47-release-fixtures-'))
try:
 app=root/'game spaces $money `tick` %percent "quote"'; app.mkdir()
 for name in ['Play.sh','Play-Low.sh','Install-Shortcut.sh']: shutil.copy2(src/name,app/name); (app/name).chmod(0o755)
 linux=app/'Linux'; (linux/'Signal47/Content/Paks').mkdir(parents=True); (linux/'Signal47/Binaries/Linux').mkdir(parents=True)
 (linux/'Signal47/Binaries/Linux/Signal47').write_text('fixture')
 (linux/'Signal47.sh').write_text('#!/usr/bin/env python3\nimport os,sys,json\nopen(os.environ["CAPTURE"],"w").write(json.dumps({"args":sys.argv[1:],"sdl":os.environ.get("SDL_VIDEO_DRIVER")}))\nsys.exit(int(os.environ.get("FAKE_EXIT","0")))\n');(linux/'Signal47.sh').chmod(0o755)
 u=linux/'Signal47/Content/Paks/Signal47-Linux.ucas'; u.write_bytes(b'abcdef');(app/'UCAS.sha256').write_text(hashlib.sha256(u.read_bytes()).hexdigest()+'  Linux/Signal47/Content/Paks/Signal47-Linux.ucas\n')
 shutil.copy2(repo/'Spill-SIGNAL47.sh',root/'Spill-SIGNAL47.sh'); (root/'Spill-SIGNAL47.sh').chmod(0o755)
 env=os.environ.copy();env.update(CAPTURE=str(root/'capture.json'),S47_VULKAN_DESCRIPTORS='buffer',XDG_DATA_HOME=str(root/'xdg'),XDG_CONFIG_HOME=str(root/'config'),HOME=str(root/'home'));Path(env['HOME']).mkdir();env.pop('DISPLAY',None);env.pop('SDL_VIDEO_DRIVER',None);env.pop('SDL_VIDEODRIVER',None)
 passed=[]
 def run(script,args=[],code=0,extra={}):
  p=subprocess.run([str(script),*args],env=env|extra,capture_output=True,text=True);assert p.returncode==code,(script,args,p.returncode,p.stdout,p.stderr);return p
 run(app/'Play.sh',['--descriptor-buffer','--wayland','-Example=a b']); d=json.loads((root/'capture.json').read_text());assert '-Example=a b' in d['args'] and d['sdl']=='wayland' and '-ini:Engine:[ConsoleVariables]:r.Vulkan.Bindless.PreferredExtension=0' in d['args'];passed.append('exact argv and buffer override')
 run(app/'Play.sh',['--descriptor-heap']); assert '-ini:Engine:[ConsoleVariables]:r.Vulkan.Bindless.PreferredExtension=1' in json.loads((root/'capture.json').read_text())['args'];passed.append('heap explicit override')
 run(app/'Play.sh',['--x11'],2);passed.append('missing DISPLAY rejects X11')
 run(app/'Play.sh',[],2,{'S47_VULKAN_DESCRIPTORS':'invalid'});passed.append('invalid descriptor mode rejects')
 run(app/'Play-Low.sh');assert '-S47Quality=low' in json.loads((root/'capture.json').read_text())['args'];passed.append('low wrapper')
 u.unlink();Path(str(u)+'.part1').write_bytes(b'abc');Path(str(u)+'.part2').write_bytes(b'def');run(app/'Play.sh');assert u.read_bytes()==b'abcdef' and not Path(str(u)+'.part1').exists();passed.append('UCAS joins/hash verifies/removes parts')
 u.unlink();Path(str(u)+'.part1').write_bytes(b'BAD');Path(str(u)+'.part2').write_bytes(b'def');run(app/'Play.sh',[],1);assert not u.exists() and Path(str(u)+'.part1').exists() and not Path(str(u)+'.join-lock').exists();passed.append('bad UCAS hash preserves parts and removes transient')
 u.write_bytes(b'abcdef')
 for path in [app,linux]:
  run(root/'Spill-SIGNAL47.sh',['--pakke',str(path),'--descriptor-heap','--','-Foo=two words']);assert '-Foo=two words' in json.loads((root/'capture.json').read_text())['args']
 passed.append('manual package and Linux layouts preserve argv')
 run(root/'Spill-SIGNAL47.sh',['--pakke',str(app)],17,{'FAKE_EXIT':'17'}); assert 'exit=17' in (root/'Spill/siste-oppstart.txt').read_text();passed.append('launcher preserves error code/log')
 run(root/'Spill-SIGNAL47.sh',['--pakke',str(app),'--snarvei']);entries=list((root/'xdg/applications').glob('*.desktop'));assert len(entries)==1;txt=entries[0].read_text();assert 'Exec=/bin/bash "' in txt and '%%percent' in txt and '\\\\$money' in txt;passed.append('shortcut escaping spaces percent dollar quotes')
 # Reinstallation is idempotent; never replace edited entries or follow desktop symlinks.
 run(app/'Install-Shortcut.sh'); entry=entries[0]; original=entry.read_text()
 entry.write_text(original+'# personal edit\n');run(app/'Install-Shortcut.sh',code=2)
 assert entry.read_text().endswith('# personal edit\n');entry.write_text(original)
 desktop=root/'Desktop';desktop.mkdir();bin_dir=root/'bin';bin_dir.mkdir()
 finder=bin_dir/'xdg-user-dir';finder.write_text('#!/bin/sh\nprintf \'%s\\n\' "$FAKE_DESKTOP"\n');finder.chmod(0o755)
 target=root/'personal-file';target.write_text('preserve me');(desktop/entry.name).symlink_to(target)
 run(app/'Install-Shortcut.sh',extra={'PATH':str(bin_dir)+':'+env['PATH'],'FAKE_DESKTOP':str(desktop)})
 assert target.read_text()=='preserve me' and (desktop/entry.name).is_symlink()
 passed.append('shortcut reinstall preserves edits and desktop symlink targets')
 # Exercise the production detector with only its /proc path redirected to fixture data.
 info=root/'gpu-information'
 for launcher in [repo/'Spill-SIGNAL47.sh',src/'Play.sh']:
  text=launcher.read_text(); detector=text[text.index('is_rtx50() {'):]
  detector=detector[:detector.index('\n}\n')+3]
  detector=detector.replace('/proc/driver/nvidia/gpus/*/information',str(info))
  for model,want in [('NVIDIA GeForce RTX 5090',0),('NVIDIA GeForce RTX 5080',0),('NVIDIA GeForce RTX 3080',1),('AMD Radeon RX 7900',1)]:
   info.write_text('Model: '+model+'\n')
   result=subprocess.run(['bash','-c',detector+'\nis_rtx50'],env=env,capture_output=True)
   assert result.returncode==want,(launcher,model,result.returncode)
 passed.append('8 automatic detector fixtures: RTX50 selected; RTX30/AMD unchanged')
 print(json.dumps({'passed':passed},indent=2))
finally:
 shutil.rmtree(root)
