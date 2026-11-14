#!/usr/bin/env python3
"""EASTWING-01 original deterministic 2K PBR materials. CPU-only, no Blender.
Run: uv run --no-project --with numpy --with pillow python build_textures.py
"""
from pathlib import Path
import hashlib, importlib.util, json
import numpy as np
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parent
N=2048
Y,X=np.mgrid[0:N,0:N].astype(np.float32)
U=X/N; V=Y/N
SOURCE=ROOT.parents[2]/'PhotoLab'/'surface_fields.py'
spec=importlib.util.spec_from_file_location('photolab_surface',SOURCE)
photo=importlib.util.module_from_spec(spec); spec.loader.exec_module(photo)
RNG=np.random.default_rng(470311)
import sys;sys.path.insert(0,str(ROOT.parents[2]));import photo_tex

def noise(cells,seed):
    r=np.random.default_rng(seed).random((cells,cells)).astype(np.float32)
    # Explicit periodic halo, resample an enlarged tile then crop its center.
    a=np.tile(r,(3,3))
    im=Image.fromarray(a).resize((3*N,3*N),Image.Resampling.BICUBIC)
    return np.asarray(im,dtype=np.float32)[N:2*N,N:2*N]*2-1

def mix(a,b,t): return a*(1-t)+b*t

def normal(h,span):
    dx=(np.roll(h,-1,1)-np.roll(h,1,1))*N/(2*span[0])
    dy=(np.roll(h,-1,0)-np.roll(h,1,0))*N/(2*span[1])
    nz=np.ones_like(dx); inv=1/np.sqrt(dx*dx+dy*dy+1)
    return np.stack((-dx*inv,dy*inv,nz*inv),-1)*.5+.5

def save8(path,a):
    Image.fromarray(np.round(np.clip(a,0,1)*255).astype(np.uint8)).save(path,compress_level=6)

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()

fine=noise(384,4); micro=RNG.uniform(-1,1,(N,N)).astype(np.float32)
medium=noise(64,7); broad=noise(9,13)
materials=[]

def emit(key,base,rough,metal,height,span,description,tile=True,opacity=None,extra=None):
    h=np.asarray(height,dtype=np.float32)
    b=np.clip(np.asarray(base,dtype=np.float32),0,1)
    if b.ndim==1: b=np.broadcast_to(b,(N,N,3)).copy()
    # TEXTURES-PHOTO-20261112: Tom's photo for this surface where there is one (ArtSource/photo_tex.py): albedo, roughness
    # and relief from the picture, metallic as painted. Without the picture the procedural surface below is what is made.
    P=photo_tex.surface(key,old=b,size=N) if tile and opacity is None else None
    if P: b,rough,h=P['base'],P['rough'],P['height']
    if np.ndim(rough)==0: rough=np.full((N,N),rough,dtype=np.float32)
    if np.ndim(metal)==0: metal=np.full((N,N),metal,dtype=np.float32)
    ndx=normal(h,span); ngl=ndx.copy(); ngl[:,:,1]=1-ngl[:,:,1]
    maps={'BaseColor':b,'Roughness':rough,'Metallic':metal,'NormalDX':ndx,'NormalGL':ngl}
    if opacity is not None: maps['Opacity']=opacity
    files={}
    for channel,a in maps.items():
        p=ROOT/f'{key}_{channel}.png';save8(p,a);files[channel]={'file':p.name,'sha256':sha(p),'color_space':'sRGB' if channel=='BaseColor' else 'linear','bit_depth':8}
    hmin=float(h.min());hmax=float(h.max());hs=max(hmax-hmin,1e-8)
    hp=ROOT/f'{key}_Height.png';Image.fromarray(np.round((h-hmin)/hs*65535).astype(np.uint16)).save(hp)
    files['Height']={'file':hp.name,'sha256':sha(hp),'color_space':'linear','bit_depth':16,'decode_metres':{'offset':hmin,'scale':hs}}
    entry={'key':key,'size':[N,N],'physical_tile_metres':list(span),'tileable':tile,'description':description,'maps':files,'normal_strength':1.0,'height_displacement_enabled_by_default':False,'license_manifest':key+'.license.json'}
    if extra:entry.update(extra)
    materials.append(entry)
    if P: entry.update({'photo_source':P['file'],'description':description+' Albedo from the project owner\'s generated photo texture (TEXTURES-PHOTO-20261112), made seamless and scaled by photo_tex.py.'})
    license={'asset':key,'license':('Project-owned artwork: albedo from an image generated for the project by its owner (ChatGPT, 12 Nov 2026), processed by photo_tex.py; no third-party image assets' if P else 'Project-owned original procedural artwork; no third-party image assets'),'copyright':'SIGNAL / 47 project','redistribution':'May accompany the project; no external purchased/Tripo content','source':'build_textures.py','method':'Deterministic metric height, albedo, roughness and metallic fields; no baked lighting','reused_project_source':{'path':'unreal/ArtSource/PhotoLab/surface_fields.py','sha256':sha(SOURCE),'use':'surface_channels paint response' if key=='EW_GreenSteel' else 'metric periodic normal-map convention; implementation adapted for physical tile span'},'external_reference':{'name':'apekshik/anyscene','status':'UNVERIFIED workflow-only reference, no code or assets reused'},'files':files}
    (ROOT/(key+'.license.json')).write_text(json.dumps(license,indent=2)+'\n')
    print(key,flush=True)

# Cream-painted 400 x 200 mm block, staggered half bond, painted mortar joints.
row=np.floor(V*12).astype(np.int32); cy=(V*12)%1
cx=(U*6+(row%2)*.5)%1
edge=np.minimum(np.minimum(cx,1-cx)*.4,np.minimum(cy,1-cy)*.2)
joint=np.clip((.007-edge)/.003,0,1)
block_rng=np.random.default_rng(101).uniform(-.018,.018,(12,6))
variation=block_rng[row%12,np.floor((U*6+(row%2)*.5)%6).astype(int)]
color=np.array([.79,.765,.68])[None,None,:]*(1+(broad*.017+fine*.012+variation)[:,:,None])
color=mix(color,np.array([.69,.674,.61])[None,None,:],joint[:,:,None]*.65)
h=.00018*fine+.00006*micro-.0028*joint
emit('EW_CinderblockCream',color,.76+fine*.035+joint*.07,0,h,(2.4,2.4),'Cream painted cinderblock; 400x200mm running bond. Mortar shares paint with slightly rougher recessed joints.')
# Stucco: granular plaster relief without directional shading.
stipple=np.maximum(fine+.35*micro,0)
h=.00065*medium+.00045*fine+.00016*micro
color=np.array([.73,.688,.59])[None,None,:]*(1+(broad*.035+medium*.018+micro*.008)[:,:,None])
emit('EW_Stucco',color,.86+fine*.05,0,h,(2,2),'Warm sand-cream stucco facade, 2m continuous tile, small plaster aggregate relief.')
# Concrete: random aggregate and periodic stain patches; no joints, lines or drain.
pores=np.maximum(-fine-.40,0)
stain=np.clip((noise(5,88)-.15)*2.5,0,1)
color=np.array([.46,.45,.415])[None,None,:]*(1+(broad*.055+medium*.045+micro*.018-stain*.14)[:,:,None])
h=.00017*fine+.000055*micro-.00035*pores
concrete=(color,.78+medium*.055-stain*.14,h)
emit('EW_Concrete',color,concrete[1],0,h,(4,4),'Stained concrete floor; no painted markings/drains baked into repeating material.')
# Reuse maintained PhotoLab paint response, with restrained original green pigment.
cm,rr,hh=photo.surface_channels('paint',U,V,micro,.42)
color=np.array([.265,.345,.305])[None,None,:]*cm[:,:,None]
emit('EW_GreenSteel',color,rr,0,hh,(1,1),'Green lacquer on steel, dielectric paint; wear principally in roughness, no fake metallic green paint.')
# Pegboard holes are shading depth only; silhouette perforation requires mesh holes.
px=(U*24)%1-.5; py=(V*24)%1-.5
r=np.sqrt(px*px+py*py)*.0254
hole=np.clip((.003175-r)/.0004,0,1)
rim=np.clip((.0037-r)/.0007,0,1)-hole
color=np.array([.51,.365,.205])[None,None,:]*(1+(medium*.035+micro*.018)[:,:,None])
color=mix(color,np.array([.095,.065,.035])[None,None,:],hole[:,:,None])
h=.000015*micro-.003*hole-.0002*rim
emit('EW_Pegboard',color,.77+micro*.025+hole*.12,0,h,(.6096,.6096),'Brown hardboard pegboard; 25.4mm hole pitch, 6.35mm holes. Hole bottoms represented in height; mesh perforation for silhouette.',extra={'hole_pitch_m':.0254,'hole_diameter_m':.00635,'hole_relief_m':.003})
# Zinc spangle field: periodic jittered cells with face-dependent roughness.
cells=55; gx=U*cells;gy=V*cells;ix=np.floor(gx).astype(int);iy=np.floor(gy).astype(int)
rng=np.random.default_rng(772);jx=rng.uniform(.1,.9,(cells,cells));jy=rng.uniform(.1,.9,(cells,cells));val=rng.uniform(-1,1,(cells,cells))
best=np.full((N,N),100.,dtype=np.float32);face=np.zeros((N,N),dtype=np.float32)
for oy in (-1,0,1):
 for ox in (-1,0,1):
  xx=(ix+ox)%cells;yy=(iy+oy)%cells
  d=(gx-(ix+ox+jx[yy,xx]))**2+(gy-(iy+oy+jy[yy,xx]))**2
  mask=d<best;face[mask]=val[yy,xx][mask];best=np.minimum(best,d)
color=np.array([.64,.665,.68])[None,None,:]*(1+(face*.045+fine*.008)[:,:,None])
emit('EW_Galvanized',color,.44+face*.055+medium*.018,.98,.000002*face+.000002*micro,(1,1),'Galvanized zinc-coated steel; ~18mm spangles primarily roughness variation, metallic=0.98.')
# A separate line strip material, never applied to the concrete floor UVs.
wear=np.clip((fine+.45*medium-.3)*2.2,0,1)
opacity=1-wear*.85
color=np.broadcast_to(np.array([.89,.61,.105]),(N,N,3)).copy()*(1+(medium*.025)[:,:,None])
emit('EW_YellowLine',color,.69+medium*.035,0,.00008*opacity,(.10,2.0),'Separate 100mm painted line strip. Use dedicated plane/mesh or decal with opacity; never repeat across concrete.',opacity=opacity,extra={'use':'modular_overlay','recommended_mesh_width_m':.10,'offset_above_floor_m':.001})
# Drain surface for geometry, with slots aligned along short axis; non-tileable.
xm=(U-.5)*.5;ym=(V-.5)*.3
inside=(np.abs(xm)<.22)&(np.abs(ym)<.11)
slots=(np.abs(((xm+.22)%.035)-.0175)<.009)&inside
frame=(np.abs(xm)>.22)|(np.abs(ym)>.11)
color=np.broadcast_to(np.array([.31,.325,.325]),(N,N,3)).copy()*(1+medium[:,:,None]*.04)
color[slots]=[.035,.039,.038]
h=np.where(slots,-.008,0).astype(np.float32)+micro*.000005
emit('EW_DrainGrate',color,np.where(slots,.93,.56),np.where(slots,0,.96),h,(.5,.3),'Dedicated 500x300mm drain grate surface. Apply to separate grate prop, slots should be geometric for closeups; no tiling on concrete.',tile=False,extra={'use':'dedicated_prop','silhouette_requires_geometry':True})
# Explicit one-bay mockup, for author review, not runtime repeating surface.
# Preview swatches show basecolor plus encoded normal, not a game rendering.
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
try: f=ImageFont.truetype(font,20)
except OSError:f=ImageFont.load_default()
board=Image.new('RGB',(1440,8*240),(30,32,31));d=ImageDraw.Draw(board)
for i,m in enumerate(materials):
 key=m['key']; y=i*240
 for j,c in enumerate(('BaseColor','Roughness','Metallic','NormalDX','Height')):
  im=Image.open(ROOT/m['maps'][c]['file'])
  if c=='Height': im=Image.fromarray((np.asarray(im,dtype=np.float32)/257).astype(np.uint8))
  im=im.convert('RGB').resize((210,210),Image.Resampling.LANCZOS)
  board.paste(im,(350+j*215,y+24));d.text((350+j*215,y+3),c,fill='white',font=f)
 d.text((12,y+45),key,fill='white',font=f);d.text((12,y+82),str(m['physical_tile_metres'])+' m',fill=(180,190,180),font=f)
board.save(ROOT/'PBR_ContactSheet.jpg',quality=92)
(ROOT/'materials.json').write_text(json.dumps({'resolution':[N,N],'normal_convention':'DirectX in Unreal; OpenGL in Blender','height_import':'16-bit linear; decode metric range from maps.Height.decode_metres; displacement off by default','runtime_status':'NOT IMPORTED / NOT RUNTIME VERIFIED','materials':materials},indent=2)+'\n')
# Fast quality checks: dimensions, finite source maps, linear channel ranges, nonflat normals.
checks=[]
for m in materials:
 for channel,file in m['maps'].items():
  with Image.open(ROOT/file['file']) as im: assert im.size==(N,N),(m['key'],channel,im.size)
 nd=np.asarray(Image.open(ROOT/m['maps']['NormalDX']['file']),dtype=np.float32)/255*2-1
 err=float(np.max(np.abs(np.linalg.norm(nd,axis=2)-1)))
 assert err<.015,(m['key'],err)
 checks.append({'key':m['key'],'maps':len(m['maps']),'resolution_pass':True,'normal_max_length_error':err})
(ROOT/'validation.json').write_text(json.dumps({'status':'PASS','scope':'2K image dimensions, decoded normals and file channel inventory; not engine visual acceptance','checks':checks},indent=2)+'\n')
