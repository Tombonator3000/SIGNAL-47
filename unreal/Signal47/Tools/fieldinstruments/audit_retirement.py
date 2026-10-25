"""Read exported GLB positions and report exact wholly-contained old triangles.
Read only with respect to game sources/assets. Writes candidates beside this script.
Not an importer and never edits original GLBs or the manifest.
"""
import argparse,json,struct
from pathlib import Path
R=Path(__file__).resolve().parent

def glb(path):
 data=path.read_bytes();jl,jt=struct.unpack_from('<II',data,12);doc=json.loads(data[20:20+jl]);off=20+jl;bl,bt=struct.unpack_from('<II',data,off)
 return doc,memoryview(data)[off+8:off+8+bl]

def accessor(doc,bin,index):
 a=doc['accessors'][index];v=doc['bufferViews'][a['bufferView']];formats={5121:'B',5123:'H',5125:'I',5126:'f'};widths={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}
 fmt='<'+formats[a['componentType']]*widths[a['type']];stride=v.get('byteStride',struct.calcsize(fmt));base=v.get('byteOffset',0)+a.get('byteOffset',0)
 rows=[struct.unpack_from(fmt,bin,base+i*stride) for i in range(a['count'])]
 return rows if widths[a['type']]>1 else [v[0] for v in rows]

def main():
 p=argparse.ArgumentParser();p.add_argument('--manifest',type=Path,default=Path('/home/tom/SIGNAL-47/unreal/Signal47/Content/WebData/manifest.json'));p.add_argument('--geo',type=Path,default=Path('/home/tom/scratch-bm/Signal47-web/geo'));a=p.parse_args()
 manifest=json.loads(a.manifest.read_text());volumes=json.loads((R/'retire_volumes.json').read_text())['volumes'];groups={}
 for node in manifest['nodes']:
  if 'geo' in node and (1005<=node['i']<=1028 or 1035<=node['i']<=1050):groups[node['geo']]=node
 geo={}
 for part in manifest['geoFiles']:
  if not any(part['from']<=k<part['to'] for k in groups):continue
  doc,bin=glb(a.geo/part['file'])
  for mesh in doc['meshes']:
   key=int(mesh['name'][1:])
   if key not in groups:continue
   prim=mesh['primitives'][0];pts=accessor(doc,bin,prim['attributes']['POSITION']);idx=accessor(doc,bin,prim['indices']);geo[key]=(pts,idx)
 result=[]
 for volume in volumes:
  lo,hi=volume['min'],volume['max'];candidates=[]
  for key,(pts,indices) in geo.items():
   node=groups[key]
   if key in volume.get('exclude_geos',[]):continue
   if volume.get('materials_only') and node['mats']!=[457]:continue
   hit=0;protected=[box for box in volume.get('protect_boxes',[]) if key in box.get('geos_only',[key])]
   for i in range(0,len(indices),3):
    tri=[pts[j] for j in indices[i:i+3]]
    intersects_protected=any(all(max(v[k] for v in tri)>=box['min'][k] and min(v[k] for v in tri)<=box['max'][k] for k in range(3)) for box in protected)
    if not intersects_protected and all(all(lo[k]-1e-6<=v[k]<=hi[k]+1e-6 for k in range(3)) for v in tri):hit+=1
   if hit:candidates.append({'node':node['i'],'geo':key,'materials':node['mats'],'triangles_fully_inside':hit,'triangles_total':len(indices)//3,'cut_boxes':[{'min':lo,'max':hi}],'protect_boxes':protected,'geometry_local_three_coordinates':True})
  result.append({'id':volume['id'],'candidates':candidates,'triangles':sum(c['triangles_fully_inside'] for c in candidates)})
 out={'status':'EXACT_TRIANGLE_COUNT_NOT_RUNTIME_VERIFIED','manifest_sha256':__import__('hashlib').sha256(a.manifest.read_bytes()).hexdigest(),'scope':'Static meshes with identity transform in world.site st/interior only. Animated descendants excluded. No clipping performed.','volumes':result,'hide_original_mesh_descendants_only':['world.site.objs.lampHood','world.site.objs.fieldPhoneHandset'],'additional_retire_named_meshes':[{'node':1133,'reason':'existing replacement car battery at field lamp; original new battery supplied by LampStand; confirm manifest name lic:car_battery before retiring'}]}
 (R/'retire_candidates.json').write_text(json.dumps(out,indent=2)+'\n')
 print(json.dumps({r['id']:{'triangles':r['triangles'],'geos':[g['geo'] for g in r['candidates']]} for r in result},indent=2))
if __name__=='__main__':main()
