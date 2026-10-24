"""R7 actual PNG/embedded GLB alpha, support, contrast and coverage checks."""
from pathlib import Path
from PIL import Image
import numpy as np,json,struct,io,hashlib,math
R=Path(__file__).resolve().parent;old=R/'evidence/r6-before-r7';stats=[];mats={m['name']:m for m in json.loads((R/'materials.json').read_text())}
for name,gain in [('CompactedUse',2.2),('ConcreteUse',3.5)]:
    a=np.asarray(Image.open(old/'textures'/(name+'_color.png')).convert('RGBA'))
    b=np.asarray(Image.open(R/'textures'/(name+'_color.png')).convert('RGBA'))
    assert np.array_equal(a[:,:,3]>0,b[:,:,3]>0),name+' zero-mask footprint changed'
    assert np.all(b[:,:,3]>=a[:,:,3]),name+' mask weakened'
    assert np.max(b[:,:,3])==224 and np.min(b[:,:,3])==0
    assert all(np.max(edge)==0 for edge in [b[0,:,3],b[-1,:,3],b[:,0,3],b[:,-1,3]])
    if name=='ConcreteUse':assert np.array_equal(a[:,:,:3],b[:,:,:3]),'Concrete albedo changed'
    else:
        dark=(b[:,:,0]<=105)&(b[:,:,0]>=85)&(b[:,:,3]>60)
        assert np.mean(dark)>.03,('no readable interrupted wheel bands',np.mean(dark))
        assert np.mean((b[:,:,0]<75)&(b[:,:,3]>0))<.001,'black patch'
    stats.append({'name':name,'alpha_gain':gain,'alpha_cap':.88,'alpha_byte_range':[int(b[:,:,3].min()),int(b[:,:,3].max())],'nonzero_alpha_support_unchanged':True,'RGB_unchanged':bool(np.array_equal(a[:,:,:3],b[:,:,:3])),'mean_alpha_before':float(np.mean(a[:,:,3])/255),'mean_alpha_after':float(np.mean(b[:,:,3])/255),'RGB_R85_105_alpha_over60_fraction':float(np.mean((b[:,:,0]>=85)&(b[:,:,0]<=105)&(b[:,:,3]>60)))})
unchanged=[]
for p,h in json.loads((old/'baseline_file_hashes.json').read_text()).items():
    if not p.startswith('textures/') or p in ['textures/CompactedUse_color.png','textures/ConcreteUse_color.png']:continue
    assert hashlib.sha256((R/p).read_bytes()).hexdigest()==h,p
    unchanged.append(p)
embedded=[]
for name in ['SM_Caliche_Surface.glb','Caliche_Ground_Assembly.glb']:
    raw=(R/'exports'/name).read_bytes();size,typ=struct.unpack_from('<II',raw,12);doc=json.loads(raw[20:20+size]);binary=raw[28+size:]
    for mat in doc['materials']:
        if mat['name'] not in ['M_CS_CompactedUse','M_CS_ConcreteUse']:continue
        assert mat['alphaMode'] in ['BLEND','MASK']
        image=doc['images'][doc['textures'][mat['pbrMetallicRoughness']['baseColorTexture']['index']]['source']];view=doc['bufferViews'][image['bufferView']]
        pixels=Image.open(io.BytesIO(binary[view.get('byteOffset',0):view.get('byteOffset',0)+view['byteLength']])).convert('RGBA')
        source=Image.open(R/mats[mat['name']]['textures']['base_color']).convert('RGBA')
        assert pixels.tobytes()==source.tobytes() and pixels.size==(2048,2048),mat['name']
        embedded.append({'file':name,'material':mat['name'],'RGBA_pixel_identical':True,'alpha_byte_range':pixels.getchannel('A').getextrema()})
# Same proven native R6 geometry is immutable in R7; integrate sampled RGBA
# over its actual material-assigned triangle UV/Color alpha. This estimates
# pre-reconstruction dither area, not scene radiance or a native visual pass.
tri=json.loads((R/'r6_native_wear_triangles.json').read_text());fields=json.loads((R/'wear_contact_samples.json').read_text())['fields'];textures={m:np.asarray(Image.open(R/mats[m]['textures']['base_color']).convert('RGBA'))/255 for m in ['M_CS_CompactedUse','M_CS_ConcreteUse']};values={f['name']:[] for f in fields}
for t in tri:
    p=np.asarray([v['p'] for v in t['pts']]);uv=np.mean([v['uv'] for v in t['pts']],axis=0);va=np.mean([v['a'] for v in t['pts']]);pos=p.mean(axis=0);x,z=pos[0],2060-pos[1];field=None
    for f in fields:
        if (f['kind']=='soil')!=(t['mat']=='M_CS_CompactedUse'):continue
        dx=x-f['center'][0];dz=z-f['center'][1];cs=math.cos(f['angle']);sn=math.sin(f['angle'])
        if abs(dx*cs+dz*sn)<=f['size'][0]/2+.0001 and abs(-dx*sn+dz*cs)<=f['size'][1]/2+.0001:field=f;break
    assert field
    tex=textures[t['mat']];iy=int(np.clip(round((1-uv[1])*(tex.shape[0]-1)),0,tex.shape[0]-1));ix=int(np.clip(round(uv[0]*(tex.shape[1]-1)),0,tex.shape[1]-1));area=np.linalg.norm(np.cross(p[1]-p[0],p[2]-p[0]))*.5
    values[field['name']].append((area,tex[iy,ix,3]*va))
coverage=[]
for name,vs in values.items():
    a=np.array(vs);fraction=np.average(a[:,1],weights=a[:,0]);coverage.append({'field':name,'drawn_area_m2':float(a[:,0].sum()),'expected_dither_area_fraction':float(fraction)})
report={'status':'PASS','texture_stats':stats,'all_other_texture_files_SHA256_unchanged':unchanged,'embedded_GLBRGBA':embedded,'expected_dither_coverage':coverage,'native_visual_status':'PENDING: only source texture and previously proven native geometry integration.'}
(R/'r7_texture_verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('R7_TEXTURE_PASS',json.dumps(stats),json.dumps(coverage))
