"""Compose eight original-town building cuts from immutable GLBs, never previous cuts.
Pure Python: no Blender/Unreal, no geometry/normal/UV rewrites, only index removal.
"""
from __future__ import annotations
import argparse,copy,hashlib,importlib.util,json,runpy,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('s47_original_trim',ROOT/'Tools/freemodels/trim_web.py');helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
STREET=runpy.run_path(str(ROOT/'Tools/calichestreet/trim_street.py'))
DESIGN=runpy.run_path(str(ROOT/'Tools/townbuildings/design.py'))
LABELS=['Sheriff','Laundry','Feed','Cinema','Church']
DEST='/Game/TownBuildings/Trim'
# ENV-03: the old crossing's span wires, hanger, box and visors (Town.ts crossing()). The
# Unreal-only signal under ArtSource/TrafficSignal replaces them; the wooden poles stay.
# Measured from the immutable export: 96 town/dark (g1000) + 12 town/roof (g1003) triangles.
SIGNAL_BOX=[-8.75,9.75,9.9,11.95,1980.8,2019.2];SIGNAL_TRIS={'g1000':96,'g1003':12}
def sha(data):return hashlib.sha256(data).hexdigest()
def inside(p,boxes):return any(b[0]<=p[0]<=b[1] and b[2]<=p[1]<=b[3] and b[4]<=p[2]<=b[5] for b in boxes)
def cutboxes():
 result={}
 # Front trims reach the old frames. Only the named stoops/dock/marquee extend forward.
 front={'Sheriff':.35,'Laundry':.35,'Feed':2.75,'Cinema':2.60,'Church':.35}
 for name in LABELS:
  row=DESIGN['BUILDINGS'][name];x,y,z=row['root'];x-=8000;x0,x1,by0,by1=row['footprint'];height=row['height']
  a=x+x0-.20;b=x+x1+.20
  if row['front']>0:b=x+x1+front[name]
  else:a=x+x0-front[name]
  result[name]=[[a,b,-5,y+height+(4.1 if name=='Church' else 1.9),z-by1-.20,z-by0+.20]]
 # Stoop boxes are narrow and low: they do not take the Sheriff's flag or laundry bench.
 result['Sheriff'].append([-16.10,-15.0,-5,DESIGN['BUILDINGS']['Sheriff']['root'][1]+.025,2100.80,2102.40])
 result['Laundry'].append([-15.10,-14.0,-5,DESIGN['BUILDINGS']['Laundry']['root'][1]+.025,2136.55,2138.25])
 result['Church'].append([20.0,21.10,-5,DESIGN['BUILDINGS']['Church']['root'][1]+.025,2093.95,2097.05])
 return result


def verify_prepared(out,rows,files,export):
 found={};checks=[]
 for filename in files:
  j,binary=helper._load(out/filename);source_name=filename.removeprefix('trim_');_,original=helper._load(export/'geo'/source_name)
  assert binary[:len(original)]==original,(filename,'source attribute bytes changed')
  checks.append(filename+' immutable position/normal/UV/color byte prefix preserved')
  for mesh in j['meshes']:
   name=mesh['name'];assert name not in found
   idx=[]
   for p in mesh['primitives']:idx.extend(v[0]for v in helper._values(j,binary,p['indices']))
   row=rows[name];assert len(idx)//3==row['resultTris'] and len(mesh['primitives'])==len(row['originalMaterialIndices'])
   assert sha(struct.pack('<'+'I'*len(idx),*idx))==row['retainedIndexSha256'],(name,'retained triangle order differs')
   found[name]=len(idx)//3;checks.append(name+' exact retained triangle indices and slot order')
 assert set(found)=={n for n,r in rows.items() if not r['hidden']}
 return {'status':'PASS','scope':'Actual prepared GLB buffers/index order; native rendering not run','checks':checks,'emittedMeshes':len(found),'hiddenMeshes':sum(r['hidden']for r in rows.values())}

def prepare(export,out,previous=None):
 manifest_bytes=(export/'manifest.json').read_bytes();man=json.loads(manifest_bytes);mh=sha(manifest_bytes);h=man['handles'];groups=['world.town.st','world.town.dressing.group'];groupids={h[g]['node']:g for g in groups};townst=h['world.town.st']['node']
 candidates={'g'+str(n['geo']):n for n in man['nodes'] if n.get('p') in groupids and n.get('geo') is not None}
 gas=json.loads((ROOT/'Tools/freemodels/models.json').read_text())['gas_station']['trim'];keep={h[k]['mat'] for k in gas.get('keep',[]) if k in h};boxes={'gas':gas['boxes'],'radio':[STREET['RADIO_BOX']],'souvenir':[STREET['SOUVENIR_BOX']],**cutboxes(),'signal':[SIGNAL_BOX]};previousrows={}
 if previous:
  old=json.loads(previous.read_text());assert old['manifestSha256']==mh,('previous street source differs',old['manifestSha256'],mh);previousrows=old['meshes']
 out.mkdir(parents=True,exist_ok=True)
 # An owned output is safe to rerun; never erase arbitrary files/directories.
 for p in out.glob('trim_*.glb'):p.unlink()
 report={};sourcehashes={};totals={k:0 for k in boxes};files=[]
 for source in man['geoFiles']:
  if not any(source['from']<=n['geo']<source['to'] for n in candidates.values()):continue
  path=export/'geo'/source['file'];sourcehashes[source['file']]=sha(path.read_bytes());j,bin_=helper._load(path);binary=bytearray(bin_);chosen=[]
  for mi,mesh in enumerate(j['meshes']):
   name=mesh.get('name')
   if name not in candidates:continue
   n=candidates[name];is_static=n['p']==townst;preserve_gas=bool(set(n.get('mats',[]))&keep);original=0;removed={k:0 for k in boxes};prim_data=[];before_indices=[];after_indices=[]
   for pi,primitive in enumerate(mesh['primitives']):
    pos=helper._values(j,binary,primitive['attributes']['POSITION']);idx=[v[0]for v in helper._values(j,binary,primitive['indices'])];assert len(idx)%3==0;original+=len(idx)//3;kept=[];before_indices.extend(idx)
    for k in range(0,len(idx),3):
     tri=idx[k:k+3];label=None
     for key,bs in boxes.items():
      if key in ['gas','radio','souvenir'] and (not is_static or preserve_gas):continue
      if key=='signal' and not is_static:continue
      if all(inside(pos[v],bs)for v in tri):label=key;break
     if label:removed[label]+=1
     else:kept.extend(tri)
    # Original slot ordinal, not the per-file placeholder GLB material index.
    prim_data.append((pi,primitive,kept));after_indices.extend(kept)
   changed=sum(removed.values())
   if not changed:continue
   result=original-changed;keptprim=[];slotmap=[]
   for pi,primitive,kept in prim_data:
    if not kept:continue
    while len(binary)%4:binary+=b'\0'
    offset=len(binary);binary+=struct.pack('<'+'I'*len(kept),*kept);j['bufferViews'].append({'buffer':0,'byteOffset':offset,'byteLength':len(kept)*4,'target':34963})
    acc=copy.deepcopy(j['accessors'][primitive['indices']]);acc.update(bufferView=len(j['bufferViews'])-1,byteOffset=0,componentType=5125,count=len(kept),min=[min(kept)],max=[max(kept)]);j['accessors'].append(acc);primitive['indices']=len(j['accessors'])-1;keptprim.append(primitive);slotmap.append(pi)
   mesh['primitives']=keptprim
   # Actual known prior source states: immutable, gas-only, gas+radio, accepted street union.
   # New dressing cuts did not previously exist, so only original is valid there.
   previous_counts={original}
   if is_static:
    previous_counts.update([original-removed['gas'],original-removed['gas']-removed['radio'],original-removed['gas']-removed['radio']-removed['souvenir']])
   if name in previousrows:
    row=previousrows[name];assert row['tris']==original,(name,'previous original count mismatch');assert row['resultTris']==original-removed['gas']-removed['radio']-removed['souvenir'],(name,'existing street union mismatch');previous_counts.add(row['resultTris'])
   report[name]={'node':n['i'],'group':groupids[n['p']],'parent':n['p'],'geo':n['geo'],'tris':original,'acceptedPreviousTris':sorted(previous_counts),'resultTris':result,'removed':changed,'removedBy':removed,'gasRemoved':removed['gas'],'radioRemoved':removed['radio'],'souvenirRemoved':removed['souvenir'],'hidden':not keptprim,'asset':''if not keptprim else f'{DEST}/StaticMeshes/{name}.{name}','originalMaterialIndices':slotmap,'sourceMaterialCount':len(n.get('mats',[])),'sourceMaterialIds':n.get('mats',[]),'retainedIndexSha256':sha(struct.pack('<'+'I'*len(after_indices),*after_indices)),'originalIndexSha256':sha(struct.pack('<'+'I'*len(before_indices),*before_indices))}
   assert result==len(after_indices)//3 and all(x<len(n['mats'])for x in slotmap),(name,'index or slot mismatch')
   for key,num in removed.items():totals[key]+=num
   if keptprim:chosen.append(mi)
  if chosen:helper._write_part(j,binary,chosen,str(out),source['file']);files.append('trim_'+Path(source['file']).stem+'.glb')
 assert all(totals[k]>=30 for k in ['radio','souvenir']+LABELS),('expected building shell absent',totals)
 assert {k:v['removedBy']['signal'] for k,v in report.items() if v['removedBy']['signal']}==SIGNAL_TRIS,('crossing signal cut differs',totals['signal'])
 # Protected dynamic/owned models are deliberately absent from candidate nodes, regardless of bounds.
 protected={k:v for k,v in h.items() if k in DESIGN['PROTECTED_RUNTIME'] or k.startswith(('world.town.runtime.panels.','world.town.proxies.','world.town.models.'))}
 protected.pop('world.town.dressing.props',None)
 for value in protected.values():assert value.get('node') not in [r['node']for r in report.values()]
 payload={'version':2,'status':'SOURCE_TRIM_PREPARED_NATIVE_UNVERIFIED','group':'world.town.st','groups':groups,'manifestSha256':mh,'sourceGeometrySha256':sourcehashes,'buildingPlanSha256':sha((ROOT/'Tools/townbuildings/design.py').read_bytes()),'composedGasTrim':gas,'boxes':boxes,'meshes':report,'files':sorted(files),'fileSha256':{f:sha((out/f).read_bytes())for f in files},'totalsRemovedBy':totals,'protectedHandles':protected,'preserved':['licensed laundry bench and its collider','patrol vehicle/door','people/paper/A02 figure','all gameplay proxies','all runtime panel objects and state','church street letterboard and light','sheriff flagpole','crossing wooden poles (g1002) and the blink panel/light/glow state'],'sourceVerification':verify_prepared(out,report,files,export), 'runtimeNotes':['Union cuts must replace existing CalicheStreet shared meshes atomically after all five building kits are ready.','Bind each native trim slot from originalMaterialIndices; imported default material is a placeholder.','Original dynamic sheriff/laundry opaque pictures need explicit rendering treatment by native building integration; this index-only trim leaves state/proxy objects intact.','No walk/drive collider changes are performed by this tool.']}
 (out/'town-patches.json').write_text(json.dumps(payload,indent=2)+'\n');return payload
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('export',type=Path);ap.add_argument('out',type=Path);ap.add_argument('--previous-street',type=Path);a=ap.parse_args();r=prepare(a.export,a.out,a.previous_street);print(json.dumps({'status':r['status'],'meshes':{k:{x:v[x]for x in ['tris','acceptedPreviousTris','resultTris','hidden','originalMaterialIndices']}for k,v in r['meshes'].items()},'totalsRemovedBy':r['totalsRemovedBy'],'files':r['files']},indent=2))
