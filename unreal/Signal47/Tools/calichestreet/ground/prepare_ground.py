"""Small original texture set and source-height audit. Run with system Python.

The audit uses standard Python; texture_detail.py runs with the bundled
NumPy/Pillow workspace runtime. This script starts neither Blender nor Unreal.
"""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
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
    # The bundled runtime supplies NumPy; system Python remains sufficient
    # for the geometry-height audit and launches this isolated texture step.
    import subprocess
    runtime=Path.home()/'.cache/codex-runtimes/codex-primary-runtime/dependencies/python/bin/python3'
    subprocess.run([str(runtime),str(ROOT/'texture_detail.py')],check=True)

if __name__=='__main__':
    print(json.dumps(audit(),indent=2));textures();print('GROUND_PREPARED')
