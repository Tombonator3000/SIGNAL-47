"""Native import of the three original field-investigation families. Root owns the editor job."""
import json, os, sys, traceback
from pathlib import Path
import unreal
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
import import_recipe as R
P=HERE.parents[1];WORK=Path(os.environ['S47_FIELD_WORK'])
R.REPORT=WORK/'native-import.json';R.report={'status':'RUNNING','checks':[],'meshes':[]}
E=R.E;S=R.S
FAMILIES=[('fieldstation','FieldStation'),('fieldinstruments','FieldInstruments'),('archiveprops','ArchiveProps')]
assembly={'version':1,'parts':[],'patches':[],'bindings':[]}
try:
 for family,namespace in FAMILIES:
  source=P/'Tools'/family;contract=json.loads((source/'model_contract.json').read_text())
  rows=json.loads((source/'materials.json').read_text())
  mats={v['name']:R.material(v,'/Game/'+namespace+'/Materials',source)for v in rows}
  for name,row in contract['parts'].items():
   path=source/'exports'/(name+'.fbx')
   asset='/Game/'+namespace+'/'+name
   if E.does_asset_exist(asset):R.check(name+' replace only owned mesh',E.delete_asset(asset))
   mesh=R.import_mesh(path,'/Game/'+namespace)
   actual=[]
   for k,slot in enumerate(mesh.get_editor_property('static_materials')):
    material=str(slot.material_slot_name);R.check(name+' original material '+material,material in mats)
    mesh.set_material(k,mats[material]);actual.append(material)
   R.check(name+' source material set',set(actual)==set(row['materials']),{'native':actual,'source':row['materials']})
   for lod in (1,2):R.check(name+' import LOD'+str(lod),S.import_lod(mesh,lod,str(path.with_stem(name+'_LOD'+str(lod))))==lod)
   settings=mesh.get_editor_property('nanite_settings');settings.enabled=False;mesh.set_editor_property('nanite_settings',settings)
   body=mesh.get_editor_property('body_setup')
   if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
   for lod in range(S.get_lod_count(mesh)):
    build=S.get_lod_build_settings(mesh,lod);build.set_editor_property('use_full_precision_u_vs',True);S.set_lod_build_settings(mesh,lod,build)
   E.save_loaded_asset(mesh)
   expected=row.get('lod_triangles') or [contract['lods'][str(l)][name]['triangles']for l in range(3)]
   R.check(name+' three actual LODs',S.get_lod_count(mesh)==3)
   for lod in range(3):R.check(name+' actual triangles LOD'+str(lod),mesh.get_num_triangles(lod)==expected[lod],{'actual':mesh.get_num_triangles(lod),'expected':expected[lod]})
   low,hi=row['bounds_local_m'];bounds=mesh.get_bounds();center=[(low[0]+hi[0])*50,-(low[1]+hi[1])*50,(low[2]+hi[2])*50]
   size=[(hi[k]-low[k])*50 for k in range(3)]
   R.check(name+' metric bounds center',all(abs(a-b)<.35 for a,b in zip([bounds.origin.x,bounds.origin.y,bounds.origin.z],center)),{'actual':str(bounds.origin),'expected':center})
   R.check(name+' metric bounds extent',all(abs(a-b)<.35 for a,b in zip([bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],size)))
   local=bool(row.get('local',False));at=row.get('native_local_three',[0,0,0])if local else row['site_world_three_m']
   part={'name':name,'asset':mesh.get_path_name(),'parent_handle':row['parent_handle'],'at':at,'local':local,'materials':actual,'bounds_local_m':row['bounds_local_m'],'lod_triangles':expected,'use':'','yaw':float(row.get('runtime_yaw_degrees',0))}
   assembly['parts'].append(part);R.report['meshes'].append(part)
  probe_source=source/'exports/axis_probe.fbx'
  if not probe_source.exists():probe_source=P/'Tools/fieldstation/exports/axis_probe.fbx'
  probe=R.import_mesh(probe_source,'/Game/'+namespace+'/Verification')
  b=probe.get_bounds();R.check(namespace+' asymmetric FBX axis probe',all(abs(a-b)<.35 for a,b in zip([b.origin.x,b.origin.y,b.origin.z,b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2],[200,-400,600,20,40,60])),str(b))
 patches=json.loads((WORK/'trim/field-patches.json').read_text())
 dest='/Game/FieldStation/Trim'
 if E.does_directory_exist(dest):R.check('clear only owned trim namespace',E.delete_directory(dest))
 for name,original in patches['meshes'].items():
  row=dict(original)
  # Original importers can remove degenerate faces. Pin the actual saved predecessor,
  # independently of the immutable source's raw topology count.
  manifest=json.loads((P/'Content/WebData/manifest.json').read_text())
  source_file=next(x['file']for x in manifest['geoFiles']if x['from']<=row['geo']<x['to'])
  old=E.load_asset('/Game/Web/Geo/'+Path(source_file).stem+'/StaticMeshes/'+name)
  R.check(name+' actual predecessor present',isinstance(old,unreal.StaticMesh))
  row['sourceRawTris']=row['tris'];row['tris']=old.get_num_triangles(0)
  R.check(name+' predecessor topology bounded by immutable source',0<row['tris']<=row['sourceRawTris'])
  if row['hidden']:assembly['patches'].append(row);continue
  mesh=R.import_mesh(WORK/'trim/fbx'/(name+'.fbx'),dest+'/StaticMeshes')
  R.check(name+' exact retained triangles',mesh.get_num_triangles(0)==row['resultTris'],{'actual':mesh.get_num_triangles(0),'expected':row['resultTris']})
  R.check(name+' retained original material slots',len(mesh.get_editor_property('static_materials'))==len(row['originalMaterialIndices']))
  fallback=E.load_asset('/Engine/EngineMaterials/DefaultMaterial')
  for slot in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(slot,fallback)
  body=mesh.get_editor_property('body_setup')
  if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
  settings=mesh.get_editor_property('nanite_settings');settings.enabled=False;mesh.set_editor_property('nanite_settings',settings)
  build=S.get_lod_build_settings(mesh,0);build.set_editor_property('use_full_precision_u_vs',True);build.set_editor_property('remove_degenerates',False);S.set_lod_build_settings(mesh,0,build);E.save_loaded_asset(mesh)
  assembly['patches'].append(row)
 policy=json.loads((HERE/'runtime_policy.json').read_text())
 for row in assembly['parts']:
  row['use']=policy.get('use',{}).get(row['name'],'')
 for b in policy.get('bindings',[]):
  row=next(p for p in assembly['parts']if p['name']==b['part'])
  slot=row['materials'].index(b['material']);assembly['bindings'].append(dict(b,slot=slot))
 assembly['part_count']=len(assembly['parts'])
 R.check('complete coherent field kit',assembly['part_count']>=20 and len(assembly['patches'])>=10)
 # Metadata is published only after all actual native checks have passed.
 out=P/'Content/FieldModels';out.mkdir(exist_ok=True)
 (out/'assembly.json').write_text(json.dumps(assembly,indent=2)+'\n')
 (WORK/'native-assembly.json').write_text(json.dumps(assembly,indent=2)+'\n')
 R.report['status']='PASS';R.flush();unreal.log('FIELD MODELS NATIVE IMPORT PASS')
except Exception:
 R.report['status']='FAIL';R.report['error']=traceback.format_exc();R.flush();raise
