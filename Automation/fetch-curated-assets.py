#!/usr/bin/env python3
"""Fetch explicitly selected public assets; retain source hashes and provenance."""
import hashlib,json,re,urllib.request
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Unity/Assets/Signal47/Art/ThirdParty'
STAGE=ROOT/'Artifacts/AssetStaging'
entries=[]
STAGE.mkdir(parents=True,exist_ok=True)
def metadata(asset):
 p=STAGE/(asset+'.json')
 if not p.exists():
  req=urllib.request.Request('https://api.polyhaven.com/files/'+asset,headers={'User-Agent':'Signal47AssetImport/1.0'})
  p.write_bytes(urllib.request.urlopen(req,timeout=30).read())
 return json.loads(p.read_text())
def queue(url,path,author,source,license='CC0-1.0',md5=None):
 entries.append(dict(url=url,path=path,author=author,source=source,license=license,expected_md5=md5))
def ph(asset,res,maps,author,model=False):
 d=metadata(asset);source='https://polyhaven.com/a/'+asset
 if model:
  x=d['fbx'][res]['fbx'];queue(x['url'],f'PolyHaven/{asset}/{asset}.fbx',author,source,md5=x['md5'])
 for m in maps:
  ext='png' if 'nor_' in m else 'jpg';x=d[m][res][ext]
  queue(x['url'],f'PolyHaven/{asset}/{m}.{ext}',author,source,md5=x['md5'])
ph('old_linoleum_flooring_01','2k',['Diffuse','nor_gl','Rough'],'Charlotte Baglioni')
ph('metal_office_desk','1k',['Diffuse','nor_gl','Rough','Metal'],'Ulan Cabanilla',True)
ph('vintage_radio_transceiver','1k',['Diffuse','nor_gl','Rough','Metal','accessories_diff','accessories_nor_gl','accessories_rough','accessories_metal'],'Mateusz Sadek',True)
ph('desk_lamp_arm_01','1k',['Diffuse','nor_gl','Rough','Metal'],'Kuutti Siitonen; Yann Kervran',True)
x=metadata('qwantani_night_puresky')['hdri']['2k']['hdr']
queue(x['url'],'PolyHaven/qwantani_night_puresky/Sky.hdr','Greg Zaal; Jarod Guest','https://polyhaven.com/a/qwantani_night_puresky',md5=x['md5'])
for name,author,id in [('printer','viertelnachvier','181420'),('phone','transitking','15826'),('cup','geraldfiebig','524999'),('wind','DarkShroom','645305')]:
 page=STAGE/(name+'.html')
 if not page.exists():page.write_bytes(urllib.request.urlopen(f'https://freesound.org/people/{author}/sounds/{id}/',timeout=30).read())
 html=page.read_text();url=re.search(r'https://cdn.freesound.org/previews/[^\s"<>]+-hq.mp3',html).group()
 queue(url,f'Freesound/{name}-hq-preview.mp3',author,f'https://freesound.org/people/{author}/sounds/{id}/')
queue('https://raw.githubusercontent.com/google/fonts/main/ofl/vt323/VT323-Regular.ttf','VT323/VT323-Regular.ttf','Peter Hull','https://github.com/google/fonts/tree/main/ofl/vt323','OFL-1.1')
queue('https://raw.githubusercontent.com/google/fonts/main/ofl/vt323/OFL.txt','VT323/OFL.txt','Peter Hull','https://github.com/google/fonts/tree/main/ofl/vt323','OFL-1.1')
queue('https://www.scottbuckley.com.au/library/wp-content/uploads/2020/04/sb_signaltonoise_nomelody.mp3','ScottBuckley/SignalToNoise-NoPiano.mp3','Scott Buckley','https://www.scottbuckley.com.au/library/signal-to-noise/','CC-BY-4.0')
queue('https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip','Kenney/interface-sounds.zip','Kenney','https://kenney.nl/assets/interface-sounds')
def fetch(e):
 p=OUT/e['path'];p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():data=p.read_bytes()
 else:
  req=urllib.request.Request(e['url'],headers={'User-Agent':'Signal47AssetImport/1.0'})
  data=urllib.request.urlopen(req,timeout=60).read()
 if e['expected_md5'] and hashlib.md5(data).hexdigest()!=e['expected_md5']:raise ValueError('Checksum mismatch '+e['path'])
 if not p.exists():p.write_bytes(data)
 e['sha256']=hashlib.sha256(data).hexdigest();e['bytes']=len(data)
 print(e['path'],len(data),flush=True)
with ThreadPoolExecutor(max_workers=4) as pool:list(pool.map(fetch,entries))
(OUT/'manifest.json').write_text(json.dumps({'downloaded':'2026-09-09','note':'Freesound inputs are public HQ MP3 previews, not original WAV masters.','files':entries},indent=2)+'\n')
