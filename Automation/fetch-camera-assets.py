#!/usr/bin/env python3
"""Fetch only the pass08 CC0 camera/foley; verify hashes and prepare URP maps."""
import hashlib,json,re,subprocess,urllib.request
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from PIL import Image,ImageOps
ROOT=Path(__file__).resolve().parents[1];stage=ROOT/'Artifacts/Research/Camera08';stage.mkdir(parents=True,exist_ok=True)
out=ROOT/'Unity/Assets/Signal47/Art/ThirdParty/PolyHaven/Camera_01';out.mkdir(parents=True,exist_ok=True)
def get(url):return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Signal47AssetImport/1.0'}),timeout=60).read()
meta=stage/'polyhaven-files.json'
if not meta.exists():meta.write_bytes(get('https://api.polyhaven.com/files/Camera_01'))
d=json.loads(meta.read_text());items=[(d['fbx']['1k']['fbx'],stage/'Camera_01-original.fbx')]
for part in ('body','lens_body','strap'):
    for suffix in ('diff','roughness','metallic','nor_gl'):
        key=part+'_'+suffix;ext='png' if suffix=='nor_gl' else 'jpg';items.append((d[key]['1k'][ext],out/(key+'.'+ext)))
def fetch(item):
    x,p=item;data=p.read_bytes() if p.exists() else get(x['url']);assert hashlib.md5(data).hexdigest()==x['md5'],str(p)
    if not p.exists():p.write_bytes(data)
    return dict(path=str(p.relative_to(ROOT)),url=x['url'],sha256=hashlib.sha256(data).hexdigest(),bytes=len(data))
with ThreadPoolExecutor(max_workers=4) as pool:manifest=list(pool.map(fetch,items))
for part in ('body','lens_body','strap'):
    m=Image.open(out/(part+'_metallic.jpg')).convert('L');a=ImageOps.invert(Image.open(out/(part+'_roughness.jpg')).convert('L'));black=Image.new('L',m.size)
    Image.merge('RGBA',(m,black,black,a)).save(out/(part+'_MetalSmooth.png'))
source='https://freesound.org/people/roachpowder/sounds/170229/'
page=stage/'shutter-source.html'
if not page.exists():page.write_bytes(get(source))
html=page.read_text();assert 'creativecommons.org/publicdomain/zero/1.0' in html
url=re.search(r'https://cdn.freesound.org/previews/[^\s"<>]+-hq.mp3',html).group();preview=stage/'Shutter-preview.mp3'
if not preview.exists():preview.write_bytes(get(url))
data=preview.read_bytes();manifest.append(dict(path=str(preview.relative_to(ROOT)),url=url,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data)))
subprocess.run(['ffmpeg','-nostdin','-y','-loglevel','error','-i',str(preview),'-af','highpass=f=100,alimiter=limit=0.85:level=false','-ar','44100','-ac','1',str(ROOT/'Unity/Assets/Signal47/Art/ThirdParty/PreparedAudio/CameraShutter.wav')],check=True)
(out/'source-manifest.json').write_text(json.dumps(dict(date='2026-09-10',model=dict(author='Rajil Jose Macatangay',source='https://polyhaven.com/a/Camera_01',license='CC0-1.0'),sound=dict(author='roachpowder',source=source,license='CC0-1.0',note='Public HQ MP3 preview, not original master'),files=manifest),indent=2)+'\n')
print('CAMERA08_ASSET_IMPORT_PASS',len(manifest),'verified source files. Powered by Poly Haven.')
