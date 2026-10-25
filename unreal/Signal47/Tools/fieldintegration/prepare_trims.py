"""Prepare precise leaf/index replacements from the immutable export. Pure Python, no DCC launch."""
import argparse, copy, hashlib, importlib.util, json, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('original_trim',ROOT/'Tools/freemodels/trim_web.py')
helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
def sha(data):return hashlib.sha256(data).hexdigest()
def inside(p,b):return all(b[k]-1e-5<=p[k//2]<=b[k+1]+1e-5 for k in (0,2,4))
def prepare(export,out,recipe):
 mb=(export/'manifest.json').read_bytes();m=json.loads(mb);rules=json.loads(recipe.read_text())
 assert rules['manifest_sha256']==sha(mb),'Recipe belongs to a different export'
 nodes={n['i']:n for n in m['nodes']};bygeo={r['geo']:r for r in rules['meshes']}
 assert len(bygeo)==len(rules['meshes']), 'Duplicate geometry recipes'
 out.mkdir(parents=True,exist_ok=True);rows={};files=[];sourcehashes={};checks=[]
 for source in m['geoFiles']:
  if not any(source['from']<=g<source['to'] for g in bygeo):continue
  path=export/'geo'/source['file'];j,original=helper._load(path);binary=bytearray(original);chosen=[]
  sourcehashes[source['file']]=sha(path.read_bytes())
  for mesh in j['meshes']:
   name=mesh.get('name','');geo=int(name[1:]) if name.startswith('g') and name[1:].isdigit() else -1
   if geo not in bygeo:continue
   r=bygeo[geo];n=nodes[r['node']]
   assert n.get('geo')==geo and n['p']==r['parent'] and n['mats']==r['source_material_ids'],name
   retained=[];before=[];slots=[];keptprim=[];protected=0
   for pi,primitive in enumerate(mesh['primitives']):
    pos=helper._values(j,binary,primitive['attributes']['POSITION'])
    idx=[v[0]for v in helper._values(j,binary,primitive['indices'])];assert len(idx)%3==0
    before.extend(idx);kept=[]
    for k in range(0,len(idx),3):
     tri=idx[k:k+3]
     safe=any(all(inside(pos[v],b)for v in tri)for b in r.get('protect_boxes',[]))
     cut=r.get('whole',False) or any(all(inside(pos[v],b)for v in tri)for b in r.get('cut_boxes',[]))
     if safe:protected+=1
     if not cut or safe:kept.extend(tri)
    if not kept:continue
    while len(binary)%4:binary+=b'\0'
    offset=len(binary);binary+=struct.pack('<'+'I'*len(kept),*kept)
    j['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(kept)*4,'target':34963})
    a=copy.deepcopy(j['accessors'][primitive['indices']]);a.update(bufferView=len(j['bufferViews'])-1,byteOffset=0,componentType=5125,count=len(kept),min=[min(kept)],max=[max(kept)])
    j['accessors'].append(a);primitive['indices']=len(j['accessors'])-1;keptprim.append(primitive);slots.append(pi);retained.extend(kept)
   tris=len(before)//3;result=len(retained)//3;assert result<tris,(name,'recipe removed nothing')
   if 'expected_removed' in r:assert tris-result==r['expected_removed'],(name,tris-result,r['expected_removed'])
   mesh['primitives']=keptprim
   if keptprim:chosen.append(mesh)
   rows[name]={'node':n['i'],'parent':n['p'],'geo':geo,'tris':tris,'resultTris':result,'removed':tris-result,'hidden':not keptprim,
    'asset':'' if not keptprim else f'/Game/FieldStation/Trim/StaticMeshes/{name}.{name}',
    'originalMaterialIndices':slots,'sourceMaterialIds':n['mats'],'protectedTrianglesRetained':protected,
    'retainedIndexSha256':sha(struct.pack('<'+'I'*len(retained),*retained)),'recipe':r}
  if not chosen:continue
  assert binary[:len(original)]==original,'Position/normal/UV bytes changed'
  j['meshes']=chosen;j['nodes']=[{'name':x['name'],'mesh':i}for i,x in enumerate(chosen)]
  j['scenes']=[{'nodes':list(range(len(chosen)))}];j['scene']=0;j['buffers'][0]['byteLength']=len(binary)
  data=json.dumps(j,separators=(',',':')).encode();data+=b' '*(-len(data)%4);binary+=b'\0'*(-len(binary)%4)
  filename='trim_'+source['file'];raw=struct.pack('<III',0x46546c67,2,28+len(data)+len(binary))+struct.pack('<II',len(data),0x4e4f534a)+data+struct.pack('<II',len(binary),0x004e4942)+binary
  (out/filename).write_bytes(raw);files.append(filename)
  reread,rb=helper._load(out/filename);assert rb[:len(original)]==original
  for mesh in reread['meshes']:
   indices=[v[0]for p in mesh['primitives']for v in helper._values(reread,rb,p['indices'])]
   assert sha(struct.pack('<'+'I'*len(indices),*indices))==rows[mesh['name']]['retainedIndexSha256']
  checks.append(filename+' source attributes and exact retained indices')
 assert len(rows)==len(bygeo),'A recipe geometry was absent'
 report={'version':1,'status':'PASS','scope':'Prepared source geometry, not native/runtime validation','manifestSha256':sha(mb),'recipeSha256':sha(recipe.read_bytes()),'meshes':rows,'files':files,'sourceGeometrySha256':sourcehashes,'fileSha256':{f:sha((out/f).read_bytes())for f in files},'checks':checks}
 (out/'field-patches.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('export',type=Path);p.add_argument('out',type=Path);p.add_argument('recipe',type=Path);a=p.parse_args()
 r=prepare(a.export,a.out,a.recipe);print(json.dumps({'status':r['status'],'meshes':len(r['meshes']),'removed':sum(x['removed']for x in r['meshes'].values())}))
