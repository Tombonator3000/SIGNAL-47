"""Validate the brief's independent file contract and create visual QA sheets.

This checks deliverable assets, not game logic. Runtime checks are recorded separately.
"""
from pathlib import Path
from collections import defaultdict
import hashlib
import json
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageCms

HERE=Path(__file__).resolve().parent
ART=HERE.parent.parent
QA=HERE.parent/'round13_15_qa'
QA.mkdir(exist_ok=True)
EXPECTED={}
for name in ['nora','waitress','driver','ward','dale','tomas_1947','deputy','host','attendant','radio_woman','souvenir_man']:
    EXPECTED[f'people/portrait_{name}.jpg']=(512,640)
for name in ['gas','sheriff','studio','studio_empty','laundromat','tv','kitchen','souvenir']:
    EXPECTED[f'town/win_{name}.jpg']=(1024,512)
EXPECTED.update({'town/cutout_alien.png':(512,1024),'town/paper_sentinel_blank.jpg':(768,1024)})
for stem,size in [('sign_caliche',(1024,512)),('sign_gas',(1024,512)),('sign_1420',(1024,512)),('sign_souvenir',(1024,512)),('sign_sheriff',(1024,256)),('marquee_plaza',(2048,256)),('board_church',(1024,256))]:
    for suffix in ['','_blank']:EXPECTED[f'town/{stem}{suffix}.png']=size
for stem,size in [('stand',(512,1024)),('turned',(512,1024)),('far',(256,512))]:EXPECTED[f'alien/silhouette_{stem}.png']=size
for stem in ['above','light','truck','figures']:EXPECTED[f'abduct/frame_{stem}.jpg']=(1024,768)
EXPECTED.update({'board/tex_cork.jpg':(1024,1024),'board/frame_wood.png':(2048,1280),'board/thread_red.png':(512,32),'board/card_index_blank.png':(768,512),'board/tape_strip.png':(256,96),'board/stain_coffee.png':(512,512)})
for name in ['red','blue','yellow']:EXPECTED[f'board/pin_{name}.png']=(128,128)
EXPECTED.update({'album/tex_album_page.jpg':(1024,1024),'album/photo_corner.png':(128,128),'album/frame_instant_blank.png':(768,900),'album/frame_sheetfilm_blank.png':(1024,820)})
for stem in ['notebook','jacket','camera','flashlight','keys','papers','nickel_1947']:EXPECTED[f'kit/kit_{stem}.png']=(256,256)
EXPECTED.update({'motel/tv_testcard_blank.jpg':(640,480),'motel/tv_lot_above.jpg':(640,480),'brand/og_cover.jpg':(1200,630),'brand/icon_512.png':(512,512)})
WINDOWS={'album/frame_instant_blank.png':(48,48,672,672),'album/frame_sheetfilm_blank.png':(64,60,896,700),'board/frame_wood.png':(64,64,1920,1152)}


def checker(w,h,dark=False):
    im=Image.new('RGB',(w,h),'#242a31' if dark else '#e7e1d1');d=ImageDraw.Draw(im)
    for y in range(0,h,16):
        for x in range(0,w,16):
            if (x//16+y//16)%2:d.rectangle((x,y,x+15,y+15),fill='#303741' if dark else '#d4cdba')
    return im


def contacts(files,name,cols=4,thumb=(280,260)):
    tw,th=thumb;rows=math.ceil(len(files)/cols)
    sheet=Image.new('RGB',(cols*(tw+20)+20,rows*(th+48)+20),'#111923')
    draw=ImageDraw.Draw(sheet)
    font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',12)
    for i,f in enumerate(files):
        im=Image.open(ART/f).convert('RGBA');im.thumbnail((tw,th),Image.Resampling.LANCZOS)
        canvas=checker(tw,th,dark=True)
        canvas.paste(im,((tw-im.width)//2,(th-im.height)//2),im)
        x=20+(i%cols)*(tw+20);y=20+(i//cols)*(th+48)
        sheet.paste(canvas,(x,y));draw.text((x,y+th+8),f,fill='#e9e2cd',font=font)
    sheet.save(QA/name,quality=90,optimize=True)


def main():
    sources=json.loads((HERE/'raster-specs.json').read_text())+json.loads((HERE/'vector-specs.json').read_text())
    byfile={s['file']:s for s in sources}
    assert len(EXPECTED)==66
    assert set(EXPECTED)==set(byfile), 'Incomplete delivery'
    checks=[];assets=[]
    for f,size in EXPECTED.items():
        path=ART/f;im=Image.open(path);im.load();ok=im.size==size
        png=path.suffix=='.png'
        ok=ok and im.format==('PNG' if png else 'JPEG') and im.mode==('RGBA' if png else 'RGB')
        profile=im.info.get('icc_profile',b'')
        ok=ok and bool(profile) and 'srgb' in ImageCms.getProfileDescription(ImageCms.ImageCmsProfile(__import__('io').BytesIO(profile))).lower()
        checks.append([f+' dimensions, format, channels and sRGB',bool(ok)])
        rec=dict(byfile[f]);rec.update(bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),format=im.format,mode=im.mode)
        if f.startswith('town/win_'):
            checks.append([f+' under 350000 bytes',path.stat().st_size<=350000])
        if png:
            a=np.asarray(im.getchannel('A'))
            checks.append([f+' actual alpha and visible artwork',bool(a.min()==0 and a.max()>0)])
            rec['alpha']={'min':int(a.min()),'max':int(a.max()),'nonzero_pixels':int((a>0).sum())}
        if f in WINDOWS:
            x,y,w,h=WINDOWS[f];a=np.asarray(im.getchannel('A'))
            checks.append([f+' fully transparent exact photo window',bool((a[y:y+h,x:x+w]==0).all())])
            checks.append([f+' opaque boundary around window',bool((a[y-1,x:x+w]>0).all() and (a[y+h,x:x+w]>0).all() and (a[y:y+h,x-1]>0).all() and (a[y:y+h,x+w]>0).all())])
        if f in ['board/tex_cork.jpg','album/tex_album_page.jpg','board/thread_red.png']:
            a=np.asarray(im).astype(float)
            rec['seam_metrics']={'x_edge_mean':float(abs(a[:,0]-a[:,-1]).mean()),'x_adjacent_mean':float(abs(a[:,1:]-a[:,:-1]).mean()),'y_edge_mean':float(abs(a[0]-a[-1]).mean()),'y_adjacent_mean':float(abs(a[1:]-a[:-1]).mean())}
            tile=Image.new(im.mode,(im.width*2,im.height*2))
            for tx in [0,im.width]:
                for ty in [0,im.height]:tile.paste(im,(tx,ty))
            if tile.width>1600:tile.thumbnail((1600,1600))
            tile.save(QA/(Path(f).stem+'-2x2.png'))
        assets.append(rec)
    groups=defaultdict(list)
    for f in EXPECTED:
        k=f.split('/')[0]
        if k=='town':k='town-signs' if ('sign_' in f or 'marquee_' in f or 'board_' in f) else 'town-windows'
        groups[k].append(f)
    for k,files in groups.items():contacts(files,k+'-contact.jpg',3 if k=='people' else 4,(256,320) if k=='people' else (280,260))
    manifest={'brief_base':'497a0d5','tool':'built-in image_gen.imagegen plus original SVG','generator_model':'not exposed by tool','export':'Pillow 13.0.0, Lanczos size adaptation only; JPEG85/subsampling0; sRGB ICC','assets':assets}
    (HERE.parent/'round13_15_manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
    visual_path=QA/'visual-review.json'
    visual_status=json.loads(visual_path.read_text())['status'] if visual_path.exists() else 'UNVERIFIED until recorded visual inspection'
    report={'asset_count':len(assets),'total_deliverable_bytes':sum(a['bytes'] for a in assets),'checks':checks,'status':'PASS' if all(ok for _,ok in checks) else 'FAIL','visual_review':visual_status,'runtime':'recorded separately'}
    (QA/'asset-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(f"{report['status']}: {sum(ok for _,ok in checks)}/{len(checks)} checks; {len(assets)} assets; {report['total_deliverable_bytes']/1e6:.2f} MB")
    for label,ok in checks:
        if not ok:print('FAIL',label)
    if report['status']=='FAIL':raise SystemExit(1)


if __name__=='__main__':main()
