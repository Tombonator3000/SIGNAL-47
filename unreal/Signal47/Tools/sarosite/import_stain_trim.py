"""Import/verify only /Game/SaroSite/Trim/StaticMeshes/g455. No process launch.
S47_SARO_STAIN_REPORT=new report path; optional S47_SARO_STAIN_VERIFY_ONLY=1.
S47_SARO_STAIN_SOURCE may override the sibling legacy_stains directory.
"""
from pathlib import Path
import hashlib,json,os,shutil,struct,traceback
import unreal
SRC=Path(os.environ.get('S47_SARO_STAIN_SOURCE',str(Path(__file__).resolve().parent/'legacy_stains')))
OUT=Path(os.environ['S47_SARO_STAIN_REPORT']);DEST='/Game/SaroSite/Trim';ASSET=DEST+'/StaticMeshes/g455'
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)or unreal.new_object(unreal.StaticMeshEditorSubsystem))
report={'status':'RUNNING','checks':[],'scope':'Actual saved native g455; runtime original-atlas rebinding and screenshots are separate.'}
def flush():OUT.parent.mkdir(parents=True,exist_ok=True);OUT.write_text(json.dumps(report,indent=2)+'\n')
def check(name,ok,detail=None):report['checks'].append({'name':name,'pass':bool(ok),'detail':detail});flush();assert ok,(name,detail)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
try:
 meta=SRC/'stain-patches.json';m=json.loads(meta.read_text());r=m['meshes']['g455'];check('exact owned source contract',m['version']==2 and set(m['meshes'])=={'g455'}and r['tris']==1712 and r['resultTris']==1224 and r['originalMaterialIndices']==[0])
 check('one GLB source',len(m['files'])==1);f=m['files'][0];check('safe source file',Path(f).name==f and f=='trim_geo_003.glb');path=SRC/f;check('actual prepared GLB hash',sha(path)==m['fileSha256'][f])
 raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0];doc=json.loads(raw[20:20+n]);check('single exact source mesh name',[x['name']for x in doc['meshes']]==['g455'])
 if os.environ.get('S47_SARO_STAIN_VERIFY_ONLY')!='1':
  # Clear only this one owned model; never delete the three SaroSite authored parts.
  for p in E.list_assets(DEST,recursive=True):
   obj=E.load_asset(p)
   if isinstance(obj,unreal.StaticMesh)and obj.get_name()in['g455',path.stem]:check('remove previous owned stain mesh',E.delete_asset(p),p)
  task=unreal.AssetImportTask();task.filename=str(path);task.destination_path=DEST;task.replace_existing=True;task.automated=True;task.save=True;T.import_asset_tasks([task]);E.make_directory(DEST+'/StaticMeshes')
  meshes=[(p,E.load_asset(p))for p in E.list_assets(DEST,recursive=True)if isinstance(E.load_asset(p),unreal.StaticMesh)]
  check('single imported native mesh',len(meshes)==1,[p for p,_ in meshes]);old,mesh=meshes[0];check('imported name matches source or single-file fallback',mesh.get_name()in['g455',path.stem],mesh.get_name())
  if mesh.get_path_name().split('.')[0]!=ASSET:check('canonical g455 path/name',E.rename_asset(old,ASSET))
  mesh=E.load_asset(ASSET);check('normalized object',isinstance(mesh,unreal.StaticMesh)and mesh.get_name()=='g455')
  mesh.set_material(0,E.load_asset('/Engine/EngineMaterials/DefaultMaterial'))
  body=mesh.get_editor_property('body_setup')
  if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
  settings=mesh.get_editor_property('nanite_settings');settings.enabled=False;mesh.set_editor_property('nanite_settings',settings)
  for lod in range(S.get_lod_count(mesh)):
   build=S.get_lod_build_settings(mesh,lod);build.set_editor_property('use_full_precision_u_vs',True);S.set_lod_build_settings(mesh,lod,build)
  E.save_loaded_asset(mesh)
 mesh=E.load_asset(ASSET);check('saved native mesh exists',isinstance(mesh,unreal.StaticMesh));check('saved native exact1224tri',mesh.get_num_triangles(0)==1224,mesh.get_num_triangles(0));check('saved1LOD1materialslot',S.get_lod_count(mesh)==1 and len(mesh.get_editor_property('static_materials'))==1)
 check('saved FP32 UV policy',S.get_lod_build_settings(mesh,0).get_editor_property('use_full_precision_u_vs')is True)
 b=mesh.get_bounds();actual=[b.origin.x,b.origin.y,b.origin.z,2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z];expected=r['expectedUE_center_cm']+r['expectedUE_size_cm'];check('retained metric bounds/source transform',max(abs(a-b)for a,b in zip(actual,expected))<.25,{'actual':actual,'expected':expected})
 target=Path(unreal.Paths.project_content_dir())/'SaroSite';target.mkdir(parents=True,exist_ok=True)
 if os.environ.get('S47_SARO_STAIN_VERIFY_ONLY')=='1':check('staged metadata hash',sha(target/'stain-patches.json')==sha(meta))
 else:shutil.copy2(meta,target/'stain-patches.json')
 report['status']='PASS';report['metadataSha256']=sha(meta);flush();unreal.log('SARO STAIN TRIM NATIVE PASS')
except Exception:
 report['status']='FAIL';report['error']=traceback.format_exc();flush();raise
