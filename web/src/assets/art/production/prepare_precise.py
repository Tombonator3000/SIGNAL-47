#!/usr/bin/env python3
"""Eksporter presise spillmarkører og kart uten bildegenerert geometri eller tekst.

Kjør fra vilkårlig mappe. Krever Node og sharp; overstyr ART_NODE/ART_SHARP ved behov.
SVG-kildene er redigerbare. Målene viderefører core/textures.ts på e5706fb.
"""
from pathlib import Path
import hashlib
import json
import os
import random
import subprocess

HERE = Path(__file__).resolve().parent
ART = HERE.parent
ROOT = ART.parents[3]
RUNTIME = Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies'
NODE = os.environ.get('ART_NODE', str(RUNTIME/'node/bin/node'))
SHARP = os.environ.get('ART_SHARP', str(RUNTIME/'node/node_modules/sharp'))


def svg(w, h, body):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">\n{body}\n</svg>\n'


def grain(w, h, seed, count=700):
    r = random.Random(seed)
    # Subtle wear does not change the count or boundaries of the evidence marks.
    return '\n'.join(f'<rect x="{r.uniform(8,w-8):.2f}" y="{r.uniform(8,h-8):.2f}" width="{r.uniform(.3,2):.2f}" height="{r.uniform(.3,1):.2f}" fill="#d6d1bc" opacity=".06"/>' for _ in range(count))


assets = []


def write(relative, w, h, body, contract):
    png = ART/relative
    png.parent.mkdir(parents=True, exist_ok=True)
    source = HERE/(png.stem+'.svg')
    source.write_text(svg(w,h,body))
    code = 'require(process.argv[1])(process.argv[2]).withIccProfile("srgb").png().toFile(process.argv[3]).catch(e=>{console.error(e);process.exit(1)});'
    subprocess.run([NODE,'-e',code,SHARP,str(source),str(png)],check=True,capture_output=True,text=True)
    assets.append(dict(output=relative,source=str(source.relative_to(ART)),size=[w,h],outputSha256=hashlib.sha256(png.read_bytes()).hexdigest(),sourceSha256=hashlib.sha256(source.read_bytes()).hexdigest(),contract=contract))


# Same normalized primary stripe and tick geometry as vaneFace().
ticks = '\n'.join(f'<rect x="168.96" y="{1024*(.08+i*.075):.2f}" width="46.08" height="8"/>' for i in range(12))
write('yard/vane_b12.png',256,1024,
      '<rect width="256" height="1024" fill="#16191b"/>'
      + grain(256,1024,47)
      + '<rect x="6" y="6" width="244" height="1012" fill="none" stroke="#3a3f42" stroke-width="12"/>'
      + '<rect id="reference-stripe" x="56.32" y="51.2" width="76.8" height="921.6" fill="#efe6cd"/>'
      + f'<g id="twelve-ticks" fill="#c9c0a4">{ticks}</g>',
      {'primaryStripe':[.22,.05,.30,.90],'tickCount':12,'text':False,'physicalSizeM':[.34,1.36],'note':'Echo-materialet er separat og skal ikke erstattes av dette bildet.'})

bars = '\n'.join(f'<rect x="61.44" y="{512*(.18+i*.25):.2f}" width="389.12" height="61.44"/>' for i in range(3))
write('yard/board_r07.png',512,512,
      '<rect width="512" height="512" fill="#16191b"/>'
      +grain(512,512,7)
      +f'<g id="three-bars" fill="#efe6cd">{bars}</g>',
      {'barCount':3,'barsNormalized':[[.12,.18+i*.25,.76,.12] for i in range(3)],'text':False,'physicalSizeM':[.42,.42],'labelOverlay':'Behold R-07 som engelsk kodetekst i nederste frie felt.'})

# Canvas-space contract is deliberately 512x384, matching fieldMap(). Export is 2x.
# Only the geometry is illustrated. Labels remain exact code-native English text.
map_body = '''<rect width="1024" height="768" fill="#ddd4ba"/>
<g transform="scale(2)">
<rect x="8" y="8" width="496" height="368" rx="2" fill="none" stroke="#9f967f" stroke-width="1"/>
<g id="buildings" fill="#c2c8b9" stroke="#3c3a33" stroke-width="2.4" stroke-linejoin="round">
<path d="M190 304V350H60V250H190V284"/>
<path d="M300 260V230H430V320H300V280"/>
</g>
<g id="east-walk" fill="#e8dfc8" stroke="#5c6054" stroke-width="2.3">
<path d="M220 90H266V270H300V286H266V324H220V318H190V286H220Z"/>
</g>
<g id="fence" stroke="#686d60" stroke-width="1.6" fill="none">
<path d="M20 80H492" stroke-dasharray="4 4"/>
<path d="M30 76V84M66 76V84M102 76V84M138 76V84M174 76V84M210 76V84M246 76V84M282 76V84M318 76V84M354 76V84M390 76V84M426 76V84M462 76V84"/>
</g>
<g id="evidence-points" fill="#9b3b2c" stroke="#9b3b2c">
<circle id="motor-cabinet" cx="258" cy="190" r="6"/>
<circle id="b12-reference" cx="240" cy="110" r="6"/>
<path d="M258 190H293M240 110H290" fill="none" stroke-width="1.5"/>
</g>
<path id="sightline" d="M243 200L236 30M231 40L236 30L241 40" fill="none" stroke="#9b3b2c" stroke-width="1.8" stroke-dasharray="6 5"/>
<path id="north-arrow" d="M478 90V48M472 57L478 48L484 57" fill="none" stroke="#2a2a26" stroke-width="2"/>
</g>'''
write('lab/map_field_yard.png',1024,768,map_body,
      {'text':False,'physicalSizeM':[1,.75],'overlayCanvas':[512,384],
       'labelSlots':{'CONTROL ROOM':[66,300],'PHOTO LAB':[320,280],'EAST WALK':[274,340],'S-03 / MOTOR BUS':[300,195],'B-12 / OPTICAL':[300,112],'TO S-03 (ANTENNA)':[140,26],'N':[472,40]},
       'north':'up (-Z)','scope':'Kapittel 1s feltkart; arkiv og nye reisemål skal ikke avsløres her.',
       'geometry':'S-03-punktet er motorskapet. Antennen ligger nord utenfor nærutsnittet. B-12 er nord for skapet. Ganglinjer er skjematiske, ikke målt kollisjon.'})

manifest = {'date':'2026-10-04','method':'Originale prosjekt-SVG-er, rasterisert med sharp. Ingen bilde-AI for bevisgeometri eller karttopologi.',
            'sourceBranch':'origin/ccr-30e38858-767d90','sourceCommit':'e5706fb',
            'sources':['web/src/core/textures.ts: vaneFace, barsBoard, fieldMap','web/src/world/ServiceYard.ts','web/ART_BRIEF.md: runde 3'],
            'integration':'Leverte kilder. Ingen runtime-import er endret i denne grafikkgrenen.',
            'assets':assets}
(HERE/'PRECISE_GRAPHICS.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
for a in assets: print(a['output'],a['size'])
