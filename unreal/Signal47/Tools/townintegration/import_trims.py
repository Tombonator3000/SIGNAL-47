"""Import only owned /Game/TownBuildings/Trim assets; no process launching.
S47_TOWN_TRIM=<prepared dir>, S47_TOWN_TRIM_REPORT=<new report path>.
A fresh editor process may set S47_TOWN_TRIM_VERIFY_ONLY=1 to verify saved meshes
and metadata without reimport. Native code restores original slot bindings.
"""
import hashlib,json,os,shutil,traceback
from pathlib import Path
import unreal
SRC=Path(os.environ['S47_TOWN_TRIM']);REPORT=Path(os.environ['S47_TOWN_TRIM_REPORT']);DEST='/Game/TownBuildings/Trim'
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
report={'status':'RUNNING','checks':[],'meshes':[],'scope':'Actual native imported trim meshes; runtime binding/visibility/collision is separate.'}
def flush():REPORT.parent.mkdir(parents=True,exist_ok=True);REPORT.write_text(json.dumps(report,indent=2)+'\n')
def check(name,ok,detail=None):
 report['checks'].append({'name':name,'pass':bool(ok),'detail':detail});flush();assert ok,(name,detail)
def filehash(p):return hashlib.sha256(p.read_bytes()).hexdigest()
try:
 metadata=SRC/'town-patches.json';data=json.loads(metadata.read_text());rows=data['meshes'];check('metadata schema',data['version']==2 and len(rows)>0)
 for f in data['files']:
  check('owned source filename '+f,Path(f).name==f and f.startswith('trim_')and f.endswith('.glb'))
  check('source GLB fingerprint '+f,(SRC/f).is_file()and filehash(SRC/f)==data['fileSha256'][f])
 for name,row in rows.items():
  check(name+' owned asset path',row['asset']==('' if row['hidden']else f'{DEST}/StaticMeshes/{name}.{name}'))
  check(name+' exact triangle arithmetic',row['tris']-row['removed']==row['resultTris']and sum(row['removedBy'].values())==row['removed'])
 if os.environ.get('S47_TOWN_TRIM_VERIFY_ONLY')!='1':
  if E.does_directory_exist(DEST):check('clear only owned trim namespace',E.delete_directory(DEST))
  for f in data['files']:
   task=unreal.AssetImportTask();task.filename=str(SRC/f);task.destination_path=DEST;task.replace_existing=True;task.automated=True;task.save=True;T.import_asset_tasks([task])
  E.make_directory(DEST+'/StaticMeshes')
  fallback=E.load_asset('/Engine/EngineMaterials/DefaultMaterial');check('placeholder material exists',fallback is not None)
  for path in E.list_assets(DEST,recursive=True):
   mesh=E.load_asset(path)
   if not isinstance(mesh,unreal.StaticMesh):continue
   name=mesh.get_name();check(name+' belongs to prepared union',name in rows and not rows[name]['hidden'])
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
 if os.environ.get('S47_TOWN_TRIM_VERIFY_ONLY')=='1':
  check('staged trim metadata exact fingerprint',(target/'town-patches.json').is_file()and filehash(target/'town-patches.json')==filehash(metadata))
 else:shutil.copy2(metadata,target/'town-patches.json')
 report['metadataSha256']=filehash(metadata);report['status']='PASS';flush();unreal.log('TOWN UNION TRIMS PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();flush();raise
