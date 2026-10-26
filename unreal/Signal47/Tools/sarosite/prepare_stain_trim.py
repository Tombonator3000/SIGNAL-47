"""Index-only removal of obsolete wing-window-shaped grime. No DCC/UE launch.
Reads immutable original GLBs; preserves all retained positions/normals/UV/colors.
"""
import argparse,copy,hashlib,importlib.util,json,struct
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
spec=importlib.util.spec_from_file_location('s47_original_glb_trim',ROOT/'Tools/freemodels/trim_web.py');H=importlib.util.module_from_spec(spec);spec.loader.exec_module(H)
def sha(data):return hashlib.sha256(data).hexdigest()
def obsolete(t):
 return all(20.1<=p[0]<=39.1 and -.7<=p[1]<=3.7 and abs(abs(p[2])-5.512)<.0001 for p in t) and (all(p[2]>0 for p in t)or all(p[2]<0 for p in t))
def prepare(export,out):
 mb=(export/'manifest.json').read_bytes();man=json.loads(mb);parent=man['handles']['world.grounds.dressing.group']['node'];nodes=[n for n in man['nodes']if n.get('geo')==455 and n.get('p')==parent];assert len(nodes)==1 and nodes[0]['mats']==[319];node=nodes[0]
 source=next(r for r in man['geoFiles']if r['from']<=455<r['to']);path=export/'geo'/source['file'];j,original=H._load(path);binary=bytearray(original);mi=next(i for i,m in enumerate(j['meshes'])if m['name']=='g455');mesh=j['meshes'][mi]
 tris=0;removed={'south':0,'north':0};retained=[];retained_points=[];primitives=[];slots=[]
 for slot,p in enumerate(mesh['primitives']):
  positions=H._values(j,binary,p['attributes']['POSITION']);indices=[v[0]for v in H._values(j,binary,p['indices'])];tris+=len(indices)//3;keep=[]
  for k in range(0,len(indices),3):
   ids=indices[k:k+3];triangle=[positions[v]for v in ids]
   if obsolete(triangle):removed['south'if triangle[0][2]>0 else'north']+=1
   else:keep.extend(ids);retained_points.extend(triangle)
  retained.extend(keep)
  if not keep:continue
  while len(binary)%4:binary+=b'\0'
  off=len(binary);binary+=struct.pack('<'+'I'*len(keep),*keep);j['bufferViews'].append({'buffer':0,'byteOffset':off,'byteLength':len(keep)*4,'target':34963});a=copy.deepcopy(j['accessors'][p['indices']]);a.update(bufferView=len(j['bufferViews'])-1,byteOffset=0,componentType=5125,count=len(keep),min=[min(keep)],max=[max(keep)]);j['accessors'].append(a);p['indices']=len(j['accessors'])-1;primitives.append(p);slots.append(slot)
 assert tris==1712 and removed=={'south':246,'north':242}and len(retained)//3==1224,(tris,removed,len(retained)//3)
 mesh['primitives']=primitives;out.mkdir(parents=True,exist_ok=True);H._write_part(j,binary,[mi],str(out),source['file']);filename='trim_'+Path(source['file']).stem+'.glb';oj,ob=H._load(out/filename);om=oj['meshes'][0];got=[v[0]for p in om['primitives']for v in H._values(oj,ob,p['indices'])]
 assert ob[:len(original)]==original and got==retained and len(oj['meshes'])==1 and om['name']=='g455'
 bounds=[[fn(p[k]for p in retained_points)for k in range(3)]for fn in [min,max]];lo,hi=bounds;center=[(lo[0]+hi[0])*50,(lo[2]+hi[2])*50,(lo[1]+hi[1])*50];size=[(hi[k]-lo[k])*100 for k in [0,2,1]]
 row={'node':node['i'],'parent':parent,'group':'world.grounds.dressing.group','geo':455,'tris':tris,'acceptedPreviousTris':[1712],'resultTris':1224,'removed':488,'removedBy':{'wingSouthGrime':246,'wingNorthGrime':242},'hidden':False,'asset':'/Game/SaroSite/Trim/StaticMeshes/g455.g455','originalMaterialIndices':slots,'sourceMaterialCount':len(node['mats']),'sourceMaterialIds':node['mats'],'bounds_three_m':bounds,'expectedUE_center_cm':center,'expectedUE_size_cm':size,'retainedIndexSha256':sha(struct.pack('<'+'I'*len(retained),*retained))}
 proof={'status':'PASS','scope':'Actual prepared GLB only; native/render acceptance separate','source_file':source['file'],'source_sha256':sha(path.read_bytes()),'manifest_sha256':sha(mb),'source_triangles':1712,'removed_triangles':488,'retained_triangles':1224,'removed_by_plane':removed,'source_attribute_buffer_prefix_byte_identical':ob[:len(original)]==original,'retained_triangle_index_order_identical':got==retained,'originalMaterialIndices':slots,'bounds_three_m':bounds,'criteria':{'x_m':[20.1,39.1],'y_m':[-.7,3.7],'z_planes_m':[-5.512,5.512],'z_tolerance_m':.0001,'selection':'Every triangle vertex must lie on ONE matching wing-wall plane; other site walls/roof/plinth/ground stain geometry is retained.'},'preserved':'Original normals/UV/color/position buffer bytes, triangle order and material slot; all1224 unrelated stain triangles; no hierarchy/collider/story changes.'}
 metadata={'version':2,'status':'SOURCE_STAIN_TRIM_PREPARED_NATIVE_UNVERIFIED','group':'world.grounds.dressing.group','groups':['world.grounds.dressing.group'],'manifestSha256':sha(mb),'sourceGeometrySha256':{source['file']:sha(path.read_bytes())},'meshes':{'g455':row},'files':[filename],'fileSha256':{filename:sha((out/filename).read_bytes())},'sourceVerification':proof}
 (out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n');(out/'stain-patches.json').write_text(json.dumps(metadata,indent=2)+'\n');print(json.dumps({'status':proof['status'],'row':row,'file':filename},indent=2))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--export',type=Path,default=Path('/home/tom/scratch-bm/Signal47-web'));p.add_argument('--out',type=Path,default=HERE/'legacy_stains');a=p.parse_args();prepare(a.export,a.out)
