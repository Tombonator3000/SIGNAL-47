"""Read immutable glTF geometry and measure the narrow baked-plant seam only."""
import hashlib
import json
import struct
from pathlib import Path

source=Path('/home/tom/scratch-bm/Signal47-web/geo/geo_003.glb')
b=source.read_bytes()
n=struct.unpack_from('<I',b,12)[0]
j=json.loads(b[20:20+n]);blob=b[28+n:]
def read(i):
    q=j['accessors'][i];v=j['bufferViews'][q['bufferView']]
    off=v.get('byteOffset',0)+q.get('byteOffset',0)
    count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[q['type']]
    fmt='<'+{5126:'f',5125:'I',5123:'H'}[q['componentType']]*count
    return [struct.unpack_from(fmt,blob,off+k*v.get('byteStride',struct.calcsize(fmt))) for k in range(q['count'])]
primitive=next(m for m in j['meshes'] if m['name']=='g454')['primitives'][0]
p=read(primitive['attributes']['POSITION']);uv=read(primitive['attributes']['TEXCOORD_0'])
ids=[a[0] for a in read(primitive['indices'])]
def car(v):
    x,y,z=p[v]
    return y>-.57 and ((25.8<=x<=28.2 and 6.6<=z<=12.6) or (29.8<=x<=32.2 and 6.55<=z<=13.25))
def window(v,side,k):
    x,y,z=p[v];a0=21+2.25*k;a1=a0+1.45
    return a0-.061<=x<=a1+.061 and .529<=y<=2.387 and (5.4999<=z<=5.5501 if side>0 else -5.5501<=z<=-5.4999)
def plant(v):
    x,y,z=p[v];u,w=uv[v]
    return 19.8<=x<=40 and -1<=y<=.1 and 5.6<=z<=6.6 and abs(w-.1875)<.0001 and (abs(u-.5625)<.0001 or abs(u-.6875)<.0001)
def wheel_remnant(v):
    x,y,z=p[v];u,w=uv[v]
    in_car=y<-.1 and ((25.8<=x<=28.2 and 6.6<=z<=12.6) or (29.8<=x<=32.2 and 6.55<=z<=13.25))
    return in_car and ((abs(u-.9375)<.0001 and abs(w-.9375)<.0001) or (abs(u-.3125)<.0001 and abs(w-.1875)<.0001))
counts=dict(cars=0,windows=0,plants=0,wheel_remnants=0,kept=0);kept=[];removed=[]
for i in range(0,len(ids),3):
    vv=ids[i:i+3]
    if all(car(v) for v in vv):counts['cars']+=1
    elif any(all(window(v,side,k) for v in vv) for side in [-1,1] for k in range(8)):counts['windows']+=1
    elif all(plant(v) for v in vv):counts['plants']+=1;removed.extend(p[v] for v in vv)
    elif all(wheel_remnant(v) for v in vv):counts['wheel_remnants']+=1
    else:counts['kept']+=1;kept.append(tuple((p[v],uv[v]) for v in vv))
assert counts==dict(cars=976,windows=1872,plants=230,wheel_remnants=144,kept=10328),counts
report={'status':'SOURCE_PASS_NATIVE_UNVERIFIED','source':str(source),'source_sha256':hashlib.sha256(b).hexdigest(),'source_triangles':13550,'existing_legacy_trim_triangles':10702,'counts':counts,'plant_bounds_three_m':[[min(q[d] for q in removed),max(q[d] for q in removed)] for d in range(3)],'retained_corner_position_uv_winding_sha256':hashlib.sha256(json.dumps(kept,separators=(',',':')).encode()).hexdigest(),'selection':'only constant brushDry/grassDry UV swatches in x19.8..40 y-1...1 z5.6..6.6; complete front edge plant triangles; 144 rubber/well bottom-crossing remnants in two old car prisms removed; concrete wheel stops retained; no whole merged-node hiding','native_followup':'prepare_cpu.py before cook; production saro suite checks transient10328-triangle composition and original atlas; inspect day/dusk/night east-wing and staff-cars pictures.'}
Path(__file__).with_name('source_audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
