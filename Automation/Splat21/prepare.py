#!/usr/bin/env python3
"""Prepare a fresh isolated Unity splat probe from the SIGNAL47 checkout."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--package-repo',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args();repo=Path(__file__).resolve().parents[2];out=a.output.resolve();package=a.package_repo.resolve()
revision=subprocess.check_output(['git','-C',str(package),'rev-parse','HEAD'],text=True).strip()
if revision!='2c6fed37da67a217367261fcfcd3316d34c73e76':raise SystemExit('Unexpected package revision; inspect changes before updating this probe')
if subprocess.check_output(['git','-C',str(package),'status','--porcelain'],text=True).strip():raise SystemExit('Expected unchanged upstream checkout')
out.mkdir(parents=True,exist_ok=False)
shutil.copytree(repo/'Unity/ProjectSettings',out/'ProjectSettings')
(out/'Packages').mkdir()
j=json.loads((repo/'Unity/Packages/manifest.json').read_text())
j['dependencies']['org.nesnausk.gaussian-splatting']='file:'+str(package/'package')
# Pin the resolved native dependencies measured in this probe.
for name, version in [('burst','1.8.30'),('collections','2.6.8'),('mathematics','1.3.3')]:
 j['dependencies']['com.unity.'+name]=version
j['dependencies']['com.unity.modules.xr']='1.0.0'
j['dependencies']['com.unity.modules.vr']='1.0.0'
(out/'Packages/manifest.json').write_text(json.dumps(j,indent=2)+'\n')
source_hashes={}
for name in ['Assets/UniversalRenderer.asset','Assets/UniversalRenderPipelineGlobalSettings.asset','Assets/Signal47/Art/PrototypePort/Signal47_URP.asset']:
 for ext in ['', '.meta']:
  src=repo/'Unity'/(name+ext);dst=out/(name+ext);dst.parent.mkdir(parents=True,exist_ok=True)
  shutil.copy2(src,dst);source_hashes[name+ext]=hashlib.sha256(src.read_bytes()).hexdigest()
(out/'Assets/Editor').mkdir();(out/'Assets/Probe').mkdir()
shutil.copy2(repo/'Docs/Research/ProductionTools20/Evidence/final/fixture.ply',out/'Assets/Probe/fixture.ply')
spec=importlib.util.spec_from_file_location('probe',repo/'Automation/probe_splats.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
rows=module.read_ply(out/'Assets/Probe/fixture.ply');names=list(rows[0]);count=len(rows)*64
header='ply\nformat binary_little_endian 1.0\ncomment Original SIGNAL47 synthetic grid stress fixture\nelement vertex '+str(count)+'\n'+''.join('property float '+n+'\n' for n in names)+'end_header\n'
with (out/'Assets/Probe/grid.ply').open('wb') as f:
 f.write(header.encode())
 for i in range(8):
  for j in range(8):
   for row in rows:
    row=dict(row);row['x']+=(i-3.5)*2.4;row['z']+=(j-3.5)*2.4
    f.write(struct.pack('<'+str(len(names))+'f',*(row[n] for n in names)))
for name,target in [('Splat21Build.cs','Editor'),('Splat21Runtime.cs','Probe')]:shutil.copy2(Path(__file__).with_name(name),out/'Assets'/target/name)
(out/'preparation.json').write_text(json.dumps({'upstream_revision':revision,'source_assets_sha256':source_hashes,'small_count':len(rows),'grid_count':count,'fixture_sha256':hashlib.sha256((out/'Assets/Probe/fixture.ply').read_bytes()).hexdigest(),'grid_sha256':hashlib.sha256((out/'Assets/Probe/grid.ply').read_bytes()).hexdigest()},indent=2)+'\n')
print(out)
