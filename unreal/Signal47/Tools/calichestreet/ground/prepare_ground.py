"""Small original texture set and source-height audit. Run with system Python.

Uses standard Python and Pillow already present on this workstation.
This script does not start Blender or Unreal.
"""
import hashlib
import json
import math
import random
from pathlib import Path
import struct
import sys
from PIL import Image, ImageDraw, ImageFilter
from terrain import floor_y, ANCHOR, REGION

ROOT=Path(__file__).resolve().parent
EXPORT=Path(sys.argv[1]) if len(sys.argv)>1 else Path.home()/'scratch-bm/Signal47-web'

def read_meshes(ids):
    man=json.loads((EXPORT/'manifest.json').read_text()); result={}
    for f in man['geoFiles']:
        chosen=[i for i in ids if f['from']<=i<f['to']]
        if not chosen:continue
        b=(EXPORT/'geo'/f['file']).read_bytes(); size=struct.unpack_from('<I',b,12)[0]
        j=json.loads(b[20:20+size]); off=20+size; binary=b[off+8:]
        def values(ai):
            a=j['accessors'][ai]; v=j['bufferViews'][a['bufferView']]
            typ={5126:'f',5125:'I',5123:'H',5121:'B'}[a['componentType']]
            n={'VEC3':3,'SCALAR':1}[a['type']]; fmt='<'+typ*n
            step=v.get('byteStride',struct.calcsize(fmt)); start=v.get('byteOffset',0)+a.get('byteOffset',0)
            return [struct.unpack_from(fmt,binary,start+k*step) for k in range(a['count'])]
        for mesh in j['meshes']:
            if mesh.get('name') not in ['g'+str(i) for i in chosen]:continue
            tris=[]
            for p in mesh['primitives']:
                vs=values(p['attributes']['POSITION']); ix=values(p['indices'])
                for k in range(0,len(ix),3):
                    t=[vs[ix[k+i][0]] for i in range(3)]
                    if max(v[0] for v in t)<-30 or min(v[0] for v in t)>40 or max(v[2] for v in t)<2020 or min(v[2] for v in t)>2090:continue
                    tris.append(t)
            result[mesh['name']]=tris
    return man,result

def at(tri,x,z):
    a,b,c=tri; den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
    if abs(den)<1e-9:return None
    u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/den
    v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/den
    return u*a[1]+v*b[1]+(1-u-v)*c[1] if min(u,v,1-u-v)>=-1e-6 else None

def audit():
    man,meshes=read_meshes([873,874,1004]); rows=[]
    for x,z in [(-26,2028),(-21,2054),(-12,2033),(7,2045),(13,2066),(16,2080),(33,2060),(38,2084),(4.65,2075),(-15,2050),(-15.5,2074),(18,2032),(18,2064)]:
        hits=[h for key,ts in meshes.items() for t in ts if (h:=at(t,x,z)) is not None]
        y=floor_y(x,z); top=max((h for h in hits if h<y+.12),default=None)
        rows.append({'road_local_xz':[x,z],'runtime_floor_m':y,'source_rendered_top_m':top,'difference_m':None if top is None else top-y})
    # Keep only relevant original triangles as source evidence, not full world.
    out={'anchor_world_three_m':ANCHOR,'region_road_local_xz':REGION,
         'manifest_sha256':hashlib.sha256((EXPORT/'manifest.json').read_bytes()).hexdigest(),
         'source_geometry_ids':[873,874,1004],'height_samples':rows,
         'source_triangle_counts':{k:len(v) for k,v in meshes.items()},
         'notes':'g873 includes desert grid, grade strips and tufts. New overlays sample the unchanged original rendered top; +6 mm on pave, +8 mm on asphalt/soil, at most +13 mm for narrow grit details. Analytic FloorAt is retained for auditing existing foot-height differences.'}
    (ROOT/'source_contact.json').write_text(json.dumps(out,indent=2)+'\n')
    (ROOT/'contact_triangles.json').write_text(json.dumps(meshes,separators=(',',':'))+'\n')
    return out

def textures():
    path=ROOT/'textures';path.mkdir(exist_ok=True);size=512
    specs=[('CalicheSoil',[.49,.385,.264],.93),('PeaGravel',[.43,.369,.286],.96),('OldAsphalt',[.22,.212,.192],.87),('DrySilt',[.56,.437,.291],.98),('Limestone',[.54,.478,.363],.91),('DryPlant',[.36,.315,.17],.91),('TarRepair',[.17,.162,.146],.82),('WornConcrete',[.49,.464,.406],.95)]
    strengths={'CalicheSoil':.12,'DrySilt':.12,'PeaGravel':.16,'OldAsphalt':.10,'TarRepair':.10,'WornConcrete':.10,'Limestone':.22,'DryPlant':.14}
    rows=[]
    for seed,(name,color,rough) in enumerate(specs):
        rng=random.Random(4700+seed);height=[];col=bytearray();roughness=bytearray()
        byte=lambda f:max(0,min(255,int(f*255)))
        # Wrapped value noise replaces the regular trigonometric waves that
        # became conspicuous at low player-camera angles in the first QA.
        grids={n:[[rng.random() for _ in range(n)] for _ in range(n)] for n in [4,17,53]}
        def noise(u,v,n):
            px=u*n;py=v*n;ix=int(px);iy=int(py);a=px-ix;b=py-iy
            a=a*a*(3-2*a);b=b*b*(3-2*b);g=grids[n]
            return (g[iy%n][ix%n]*(1-a)+g[iy%n][(ix+1)%n]*a)*(1-b)+(g[(iy+1)%n][ix%n]*(1-a)+g[(iy+1)%n][(ix+1)%n]*a)*b
        crack=Image.new('L',(size,size),0);draw=ImageDraw.Draw(crack)
        if name in ['OldAsphalt','TarRepair','WornConcrete']:
            for _ in range(9):
                px=rng.uniform(0,size);py=rng.uniform(0,size);heading=rng.random()*math.tau
                points=[(px,py)]
                for _ in range(rng.randint(4,10)):
                    heading+=rng.uniform(-.58,.58);length=rng.uniform(7,19)
                    px+=math.cos(heading)*length;py+=math.sin(heading)*length;points.append((px,py))
                draw.line(points,fill=rng.randint(85,150),width=1)
        crackbytes=crack.filter(ImageFilter.GaussianBlur(.45)).tobytes()
        for y in range(size):
            v=y/size
            for x in range(size):
                u=x/size;grain=rng.random()
                macro=noise(u,v,4);mid=noise(u,v,17);fine=noise(u,v,53)
                fissure=crackbytes[y*size+x]/255
                h=.30*grain+.10*mid+.04*fine-.10*fissure
                tint=.79+.12*grain+.05*macro+.02*mid-.05*fissure
                height.append(h);col.extend(byte(c*tint) for c in color);roughness.append(byte(rough+(.5-grain)*.06))
        normals=bytearray()
        for y in range(size):
            for x in range(size):
                dx=(height[y*size+(x+1)%size]-height[y*size+(x-1)%size])*.6
                dy=(height[((y+1)%size)*size+x]-height[((y-1)%size)*size+x])*.6
                length=math.sqrt(dx*dx+dy*dy+1)
                normals.extend(byte(q/length*.5+.5) for q in [-dx,dy,1])
        Image.frombytes('RGB',(size,size),bytes(col)).save(path/(name+'_color.png'))
        Image.frombytes('RGB',(size,size),bytes(normals)).save(path/(name+'_normal.png'))
        Image.frombytes('L',(size,size),bytes(roughness)).save(path/(name+'_roughness.png'))
        rows.append({'name':'M_CS_'+name,'base_color_linear':[1,1,1,1],'metallic':0,'roughness':rough,'emission':[0,0,0,1],'emission_strength':0,'alpha':1,'two_sided':name=='DryPlant','normal_strength':strengths[name],'uv_scale':1.0,'normal_convention':'DirectX','texture_color_space':'sRGB','roughness_compression':'TC_GRAYSCALE','roughness_sampler':'SAMPLERTYPE_LINEAR_GRAYSCALE','textures':{k:'textures/'+name+'_'+s+'.png' for k,s in [('base_color','color'),('normal','normal'),('roughness','roughness')]}})
    (ROOT/'materials.json').write_text(json.dumps(rows,indent=2)+'\n')

if __name__=='__main__':
    print(json.dumps(audit(),indent=2));textures();print('GROUND_PREPARED')
