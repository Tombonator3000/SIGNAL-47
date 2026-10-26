"""Prepare only the 11 mixed Motel/Room6 index cuts from immutable source GLBs.
Never modifies Town trim output or the source export. No Unreal/Blender launch.
"""
from __future__ import annotations
import argparse,copy,json,struct
from pathlib import Path
from prepare_trims import ROOT,helper,sha,verify_prepared
DEST='/Game/TownBuildings/RoomTrim'
def inside(p,boxes):
 # Recipe boundaries are decimal source measurements of float32 GLB coordinates.
 e=1e-5
 return any(b[0]-e<=p[0]<=b[1]+e and b[2]-e<=p[1]<=b[3]+e and b[4]-e<=p[2]<=b[5]+e for b in boxes)
def prepare(export,out,recipe):
 mb=(export/'manifest.json').read_bytes();m=json.loads(mb);rh=recipe.read_bytes();rules=json.loads(rh);assert rules['manifest_sha256']==sha(mb)
 recipes=rules['partial_mesh_cuts'];assert len(recipes)==11
 nodes={n['i']:n for n in m['nodes']};groupids={663:'world.motel.st',690:'world.motel.inSt',1648:'world.room6.fixed'}
 for parent,handle in groupids.items():assert m['handles'][handle]['node']==parent
 bygeo={r['geo']:r for r in recipes};assert len(bygeo)==11
 out.mkdir(parents=True,exist_ok=True)
 for f in out.glob('trim_*.glb'):f.unlink()
 meshes={};files=[];sourcehashes={};sourcecounts={}
 for source in m['geoFiles']:
  relevant=[g for g in bygeo if source['from']<=g<source['to']]
  # Whole-leaf previous-state audit also needs all its source counts.
  needwhole=any(source['from']<=r['geo']<source['to'] for r in rules['whole_hide_nodes'])
  if not relevant and not needwhole:continue
  p=export/'geo'/source['file'];j,binary=helper._load(p);sourcehashes[source['file']]=sha(p.read_bytes());binary=bytearray(binary);chosen=[]
  for mi,mesh in enumerate(j['meshes']):
   name=mesh.get('name','');g=int(name[1:]) if name.startswith('g')and name[1:].isdigit()else -1
   count=sum(j['accessors'][p['indices']]['count']//3 for p in mesh['primitives']);sourcecounts[g]=count
   if g not in bygeo:continue
   r=bygeo[g];n=nodes[r['node']];assert n['geo']==g and n['p']==r['parent'] and n['mats']==r['source_material_ids']
   remove=r.get('remove_whole_triangles_inside',[]);protect=r.get('preserve_whole_triangles_inside',[]);only=r.get('keep_only_whole_triangles_inside',[]);assert bool(remove)!=bool(only)
   keptprim=[];slots=[];originalidx=[];retained=[];removed=0;protected=0;kept_only=0
   for pi,p in enumerate(mesh['primitives']):
    pos=helper._values(j,binary,p['attributes']['POSITION']);idx=[v[0]for v in helper._values(j,binary,p['indices'])];originalidx.extend(idx);kept=[]
    for k in range(0,len(idx),3):
     tri=idx[k:k+3]
     # A protected triangle must fit ONE same protection prism, not bridge two objects.
     safe=any(all(inside(pos[v],[box])for v in tri)for box in protect)
     keep_exact=any(all(inside(pos[v],[box])for v in tri)for box in only)
     cut=(not keep_exact)if only else (any(all(inside(pos[v],[box])for v in tri)for box in remove)and not safe)
     if cut:removed+=1
     else:
      kept.extend(tri);protected+=int(safe);kept_only+=int(keep_exact)
    retained.extend(kept)
    if not kept:continue
    while len(binary)%4:binary+=b'\0'
    off=len(binary);binary+=struct.pack('<'+'I'*len(kept),*kept);j['bufferViews'].append({'buffer':0,'byteOffset':off,'byteLength':len(kept)*4,'target':34963});a=copy.deepcopy(j['accessors'][p['indices']]);a.update(bufferView=len(j['bufferViews'])-1,byteOffset=0,componentType=5125,count=len(kept),min=[min(kept)],max=[max(kept)]);j['accessors'].append(a);p['indices']=len(j['accessors'])-1;keptprim.append(p);slots.append(pi)
   mesh['primitives']=keptprim;result=count-removed;assert result==len(retained)//3
   # These partial nodes are outside Kelvora's g482/486/489/492 office cuts and outside bed subtree.
   meshes[name]={'node':n['i'],'parent':n['p'],'group':groupids[n['p']],'geo':g,'tris':count,'acceptedPreviousTris':[count],'resultTris':result,'removed':removed,'removedBy':{'MotelRoomReplacement':removed},'hidden':not keptprim,'asset':''if not keptprim else f'{DEST}/StaticMeshes/{name}.{name}','originalMaterialIndices':slots,'sourceMaterialCount':len(n['mats']),'sourceMaterialIds':n['mats'],'originalIndexSha256':sha(struct.pack('<'+'I'*len(originalidx),*originalidx)),'retainedIndexSha256':sha(struct.pack('<'+'I'*len(retained),*retained)),'recipe':r,'protectedTrianglesRetained':protected,'keepOnlyTrianglesRetained':kept_only}
   assert all(s<len(n['mats'])for s in slots)
   if only:assert kept_only>0 and result==kept_only,(name,'registration surface missing')
   if keptprim:chosen.append(mi)
  if chosen:helper._write_part(j,binary,chosen,str(out),source['file']);files.append('trim_'+Path(source['file']).stem+'.glb')
 assert len(meshes)==11
 kelpath=ROOT/'Content/Kelvora/office-patches.json';kel=json.loads(kelpath.read_text());whole=[]
 for r in rules['whole_hide_nodes']:
  count=sourcecounts[r['geo']];previous={count};kr=kel.get('g'+str(r['geo']))
  if kr and not kr['hidden']:previous.add(count-kr['removed'])
  whole.append({'node':r['node'],'geo':r['geo'],'tris':count,'acceptedPreviousTris':sorted(previous),'kelvoraPreviouslyHidden':bool(kr and kr['hidden'])})
 payload={'version':2,'status':'SOURCE_ROOM_TRIM_PREPARED_NATIVE_UNVERIFIED','group':'world.motel.st','groups':list(groupids.values()),'manifestSha256':sha(mb),'replacementRecipeSha256':sha(rh),'sourceGeometrySha256':sourcehashes,'meshes':meshes,'files':sorted(files),'fileSha256':{f:sha((out/f).read_bytes())for f in files},'sourceVerification':verify_prepared(out,meshes,files,export),'wholeLeafPreviousStateAudit':whole,'kelvoraMetadataSha256':sha(kelpath.read_bytes()),'preserved':rules['retain_subtrees']+rules['retain_extra_nodes'],'neverHideParents':rules['never_hide_parents'],'runtimeNotes':['Only mixed-leaf index cuts; all 47 full-leaf retirements are separate assembly policy.','Bind native slots through originalMaterialIndices and preserve existing component transform/proxy state.','g488 keeps only the original registration-card surface. g1125 keeps table ashtray triangles outside the AC prism.','Kelvora changes only g482/486/489/g492 in whole-leaf audit; own bed replacement hides its separate subtree without changing partial mesh triangle counts.','No collider, door, pose, NPC or story changes.']}
 (out/'room-patches.json').write_text(json.dumps(payload,indent=2)+'\n');return payload
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('export',type=Path);ap.add_argument('out',type=Path);ap.add_argument('--recipe',type=Path,default=ROOT/'Tools/townbuildings/runtime_replacement.json');a=ap.parse_args();r=prepare(a.export,a.out,a.recipe);print(json.dumps({'meshes':{n:{k:v[k]for k in ['node','tris','acceptedPreviousTris','resultTris','removed','protectedTrianglesRetained','keepOnlyTrianglesRetained']}for n,v in r['meshes'].items()},'files':r['files'],'sourceVerification':r['sourceVerification'],'wholeKelvora':[x for x in r['wholeLeafPreviousStateAudit']if x['geo']in[482,486,489,492]]},indent=2))
