#!/usr/bin/env python3
"""Read-only evidence validation. Requires Pillow and NumPy; never edits captures."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from identity import files, aggregate

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('evidence',type=Path)
p.add_argument('--identity',required=True,type=Path)
p.add_argument('--player',type=Path)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args()
if a.output.exists():raise SystemExit('Refusing to overwrite verification')
r=json.loads((a.evidence/'result.json').read_text())
identity=json.loads(a.identity.read_text())
assert r['completed'] and r['errors']==0 and all(c['passed'] for c in r['checks']), 'Runtime checks not complete/pass'
assert r['renderer']=='Vulkan' and r['width']==1280 and r['height']==800
assert len(r['phases'])==4
for phase in r['phases']:
    values=np.loadtxt(a.evidence/(phase['name']+'-frame-ms.csv'))
    assert values.size==phase['frames'] and np.isfinite(values).all() and (values>=0).all()
    assert abs(float(values.mean())-phase['meanMs'])<1e-7
    if phase['splats']:assert phase['assetsReady'] and phase['uploadedCount']==phase['assetCount']==r['declaredSplatCount']
images={}
for f in a.evidence.iterdir():
    if f.suffix.lower() in ['.png','.jpg']:
        with Image.open(f) as im:
            im.load(); assert im.size==(1280,800)
            images[f.name]=np.asarray(im.convert('RGB')).astype(np.int16)
for e in r['exposures']:
    assert e['method'].startswith('Separate camera / URP SingleCameraRequest')
    assert hashlib.sha256((a.evidence/e['file']).read_bytes()).hexdigest()==e['sha256']
    assert e['file'] in images
    if e.get('proxyConfigured'):assert e['proxyReady'] and e['proxyRenderSubmitted'] and e['samePoseProjection']
if a.player:
    actual=files(a.player.resolve(),['.'])
    package_manifest=a.player/'package-manifest.json'
    if package_manifest.exists():
        package=json.loads(package_manifest.read_text())
        assert set(actual)==set(package['files'])|{'package-manifest.json'}
        assert all(actual.get(k)==v for k,v in package['files'].items())
        assert aggregate(package['files'])==package['sha256']
        actual={k:actual[k] for k in identity['player_files']}
    assert actual==identity['player_files']
    assert aggregate(actual)==identity['player_sha256']
# Isolate only the flat, unlit cyan comparison object using the mesh phase.
mesh=images['mesh-light-on.png'];hybrid=images['hybrid-light-on.png']
mask=(mesh[:,:,0]<90)&(mesh[:,:,1]>100)&(mesh[:,:,2]>150)&(mesh[:,:,2]>mesh[:,:,1])
for _ in range(2):
    eroded=np.zeros_like(mask)
    eroded[1:-1,1:-1]=mask[1:-1,1:-1]&mask[:-2,1:-1]&mask[2:,1:-1]&mask[1:-1,:-2]&mask[1:-1,2:]
    mask=eroded
assert mask.sum()>100,'Cyan reference was not found'
diff=np.abs(mesh-hybrid).sum(axis=2)
changed=int(((diff>=12)&mask).sum())
photo_diffs={}
for e in r['exposures']:
    screen=e['name']+'.png'
    if screen in images:
        photo_diffs[e['name']]=float(np.abs(images[screen]-images[e['file']]).mean())
result={'runtime_checks_passed':len(r['checks']),'runtime_errors':r['errors'],
        'decoded_original_images':len(images),'separate_exposures_verified':len(r['exposures']),
        'source_sha256':identity['source_sha256'],'player_sha256':identity['player_sha256'],
        'player_bytes_verified':bool(a.player),'cyan_interior_pixels':int(mask.sum()),
        'cyan_pixels_changed_by_splats':changed,'cyan_reference_pass':changed==0,
        'screen_photo_mean_absolute_rgb_0_255':photo_diffs,
        'scope':'Data and a single opaque cyan reference at the fixed comparison pose. Not global occlusion, image quality, native input, GPU profiling or canonical game-camera verification.',
        'evidence_files':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(a.evidence.iterdir()) if f.is_file()}}
a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='evidence_files'},indent=2))
