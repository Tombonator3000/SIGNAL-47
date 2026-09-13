#!/usr/bin/env python3
"""Render retained trained Gaussians with the installed PlayCanvas rasterizer."""
import argparse,json,subprocess,os
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--crop',required=True,type=Path);p.add_argument('--out',required=True,type=Path)
p.add_argument('--node',required=True);p.add_argument('--cli',required=True)
p.add_argument('--view',choices=['front','oblique','detail','all'],default='all')
a=p.parse_args();repo=Path(__file__).resolve().parents[2]
a.out.mkdir(parents=True,exist_ok=True);data=json.loads((a.crop/'crop.json').read_text())
for view in data['views']:
 if a.view!='all' and a.view!=view['name']:continue
 out=a.out/(view['name']+'.webp');log=out.with_suffix('.log')
 if out.exists() or log.exists():raise SystemExit('Preserve reference renders; use a fresh directory')
 vec=lambda value:','.join(format(v,'.10g') for v in value)
 cmd=['flock','-n',os.environ.get('XDG_RUNTIME_DIR','/run/user/'+str(os.getuid()))+'/signal47-gauntlet.lock',
 'bash',str(repo/'Automation/Splat21/bounded.sh'),'2147483648','60',a.node,a.cli,
 str(a.crop/'rock-trained.ply'),str(out),'--resolution','1280x800','--camera-fov','70',
 '--camera-pos',vec(view['playcanvasPosition']),'--camera-target',vec(view['playcanvasTarget']),
 '--camera-up',vec(view['playcanvasUp']),'--camera-near','.08','--background','.035,.05,.08']
 with log.open('w') as f:r=subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT)
 if r.returncode:raise SystemExit('Reference renderer failed: '+str(log))
 print(out)
