"""Import only the 11 mixed Motel/Room6 cuts into owned /Game/TownBuildings/RoomTrim assets; no process launching.
S47_ROOM_TRIM=<prepared dir>, S47_ROOM_TRIM_REPORT=<new report path>.
A fresh editor process may set S47_ROOM_TRIM_VERIFY_ONLY=1 to verify saved meshes
and metadata without reimport. Native code restores original slot bindings.
"""
import hashlib,json,os,shutil,struct,traceback
from pathlib import Path
import unreal
SRC=Path(os.environ['S47_ROOM_TRIM']);REPORT=Path(os.environ['S47_ROOM_TRIM_REPORT']);DEST='/Game/TownBuildings/RoomTrim'
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
report={'status':'RUNNING','checks':[],'meshes':[],'scope':'Actual native imported trim meshes; runtime binding/visibility/collision is separate.'}
def flush():REPORT.parent.mkdir(parents=True,exist_ok=True);REPORT.write_text(json.dumps(report,indent=2)+'\n')
def check(name,ok,detail=None):
 report['checks'].append({'name':name,'pass':bool(ok),'detail':detail});flush();assert ok,(name,detail)
def filehash(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def source_mesh_names(path):
 raw=path.read_bytes();assert struct.unpack_from('<III',raw,0)[:2]==(0x46546c67,2)
 size,kind=struct.unpack_from('<II',raw,12);assert kind==0x4e4f534a
 doc=json.loads(raw[20:20+size]);return [m['name']for m in doc['meshes']]
def resolve_imported_name(name,file_meshes,rows):
 if name in rows:return name
 if name not in file_meshes:raise ValueError(('unrecognized imported mesh',name))
 names=file_meshes[name]
 if len(names)!=1:raise ValueError(('file-stem fallback requires exactly one source mesh',name,names))
 target=names[0]
 if target not in rows or rows[target]['hidden']:raise ValueError(('source mesh missing from nonempty metadata',name,target))
 return target
try:
 metadata=SRC/'room-patches.json';data=json.loads(metadata.read_text());rows=data['meshes'];check('metadata schema',data['version']==2 and len(rows)>0);file_meshes={}
 for f in data['files']:
  check('owned source filename '+f,Path(f).name==f and f.startswith('trim_')and f.endswith('.glb'))
  check('source GLB fingerprint '+f,(SRC/f).is_file()and filehash(SRC/f)==data['fileSha256'][f])
  names=source_mesh_names(SRC/f);check('source named meshes '+f,len(names)>0 and len(set(names))==len(names) and all(n in rows and not rows[n]['hidden']for n in names),names);file_meshes[Path(f).stem]=names
 for name,row in rows.items():
  check(name+' owned asset path',row['asset']==('' if row['hidden']else f'{DEST}/StaticMeshes/{name}.{name}'))
  check(name+' exact triangle arithmetic',row['tris']-row['removed']==row['resultTris']and sum(row['removedBy'].values())==row['removed'])
 if os.environ.get('S47_ROOM_TRIM_VERIFY_ONLY')!='1':
  if E.does_directory_exist(DEST):check('clear only owned trim namespace',E.delete_directory(DEST))
  for f in data['files']:
   task=unreal.AssetImportTask();task.filename=str(SRC/f);task.destination_path=DEST;task.replace_existing=True;task.automated=True;task.save=True;T.import_asset_tasks([task])
  E.make_directory(DEST+'/StaticMeshes')
  fallback=E.load_asset('/Engine/EngineMaterials/DefaultMaterial');check('placeholder material exists',fallback is not None)
  for path in E.list_assets(DEST,recursive=True):
   mesh=E.load_asset(path)
   if not isinstance(mesh,unreal.StaticMesh):continue
   imported_name=mesh.get_name();name=resolve_imported_name(imported_name,file_meshes,rows)
   if name!=imported_name:
    wanted=DEST+'/StaticMeshes/'+name;check(imported_name+' single source mesh normalized to '+name,E.rename_asset(path,wanted),{'sourceMeshes':file_meshes[imported_name]});path=wanted;mesh=E.load_asset(wanted);check(name+' normalized native object',isinstance(mesh,unreal.StaticMesh) and mesh.get_name()==name)
   check(name+' belongs to prepared union',name in rows and not rows[name]['hidden'])
   for slot in range(len(mesh.get_editor_property('static_materials'))):mesh.set_material(slot,fallback)
   body=mesh.get_editor_property('body_setup')
   if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
   settings=mesh.get_editor_property('nanite_settings');settings.enabled=False;mesh.set_editor_property('nanite_settings',settings)
   for lod in range(S.get_lod_count(mesh)):
    build=S.get_lod_build_settings(mesh,lod);build.set_editor_property('use_full_precision_u_vs',True);S.set_lod_build_settings(mesh,lod,build)
   E.save_loaded_asset(mesh);wanted=DEST+'/StaticMeshes/'+name
   if mesh.get_path_name().split('.')[0]!=wanted:check(name+' normalized owned path',E.rename_asset(path,wanted))
 for name,row in rows.items():
  if row['hidden']:
   check(name+' fully removed has no asset',row['resultTris']==0 and len(row['originalMaterialIndices'])==0);continue
  mesh=E.load_asset(row['asset']);check(name+' actual native mesh',isinstance(mesh,unreal.StaticMesh))
  tris=mesh.get_num_triangles(0);slots=len(mesh.get_editor_property('static_materials'))
  check(name+' saved UV precision',S.get_lod_build_settings(mesh,0).get_editor_property('use_full_precision_u_vs') is True)
  check(name+' actual native triangle count',tris==row['resultTris'],{'actual':tris,'expected':row['resultTris']})
  check(name+' native slots map back to original slots',slots==len(row['originalMaterialIndices']) and all(0<=i<row['sourceMaterialCount']for i in row['originalMaterialIndices']),{'nativeSlots':slots,'originalMaterialIndices':row['originalMaterialIndices']})
  report['meshes'].append({'name':name,'asset':mesh.get_path_name(),'triangles':tris,'slots':slots})
 # Stage only a fully imported/validated union, so a failed import cannot advertise success.
 target=Path(unreal.Paths.project_content_dir())/'TownBuildings';target.mkdir(parents=True,exist_ok=True)
 if os.environ.get('S47_ROOM_TRIM_VERIFY_ONLY')=='1':
  check('staged trim metadata exact fingerprint',(target/'room-patches.json').is_file()and filehash(target/'room-patches.json')==filehash(metadata))
 else:shutil.copy2(metadata,target/'room-patches.json')
 report['metadataSha256']=filehash(metadata);report['status']='PASS';flush();unreal.log('MOTEL ROOM PARTIAL TRIMS PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();flush();raise
