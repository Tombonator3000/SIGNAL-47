"""Actual R8 color-only change proof, against the archived R7 sources."""
from pathlib import Path
from PIL import Image
import numpy as np,json,hashlib,struct,io
R=Path(__file__).resolve().parent;old=R/'evidence/r7-before-r8'
a=np.asarray(Image.open(old/'textures/CompactedUse_color.png').convert('RGBA'));b=np.asarray(Image.open(R/'textures/CompactedUse_color.png').convert('RGBA'))
assert np.array_equal(a[:,:,3],b[:,:,3]),'R7 alpha altered'
assert (old/'textures/ConcreteUse_color.png').read_bytes()==(R/'textures/ConcreteUse_color.png').read_bytes(),'Concrete changed'
unchanged=[]
for p,h in json.loads((old/'baseline_file_hashes.json').read_text()).items():
    if not p.startswith('textures/') or p=='textures/CompactedUse_color.png':continue
    assert hashlib.sha256((R/p).read_bytes()).hexdigest()==h,p
    unchanged.append(p)
mask=(b[:,:,0]<108)&(b[:,:,3]>30)
rows=mask[:,int(b.shape[1]*.35):int(b.shape[1]*.65)].any(axis=1)
starts=np.flatnonzero(np.diff(np.r_[False,rows,False].astype(int))==1);ends=np.flatnonzero(np.diff(np.r_[False,rows,False].astype(int))==-1)
assert len(starts)>=3 and rows.mean()<.80,('uninterrupted dark grooves',len(starts),rows.mean())
embedded=[]
for name in ['SM_Caliche_Surface.glb','Caliche_Ground_Assembly.glb']:
    raw=(R/'exports'/name).read_bytes();size,typ=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+size]);binary=raw[28+size:]
    for mat in doc['materials']:
        if mat['name'] not in ['M_CS_CompactedUse','M_CS_ConcreteUse']:continue
        assert mat['alphaMode'] in ['BLEND','MASK']
        img=doc['images'][doc['textures'][mat['pbrMetallicRoughness']['baseColorTexture']['index']]['source']];view=doc['bufferViews'][img['bufferView']]
        png=Image.open(io.BytesIO(binary[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']])).convert('RGBA')
        source=Image.open(R/'textures'/(mat['name'][5:]+'_color.png')).convert('RGBA')
        assert png.size==(2048,2048) and png.tobytes()==source.tobytes(),mat['name']
        embedded.append({'file':name,'material':mat['name'],'RGBA_pixel_identical':True,'alpha_byte_range':png.getchannel('A').getextrema()})
report={'status':'PASS','only_changed_texture':'textures/CompactedUse_color.png RGB','R7_all_Soil_alpha_pixels_byte_identical':True,'Concrete_color_file_byte_identical':True,'all_other44texture_maps_SHA256_unchanged':unchanged,'embedded_GLBRGBA':embedded,'longitudinal_dark_row_fraction':float(rows.mean()),'longitudinal_dark_runs':[[int(x),int(y)] for x,y in zip(starts,ends)],'native_visual_status':'PENDING: source shape test is not a rendered appearance acceptance.'}
(R/'r8_texture_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('R8_TEXTURE_PASS',len(starts),'dark runs; row fraction',rows.mean())
