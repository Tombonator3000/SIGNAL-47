"""Root's serial Unreal Python import. S47_ACCESS_SOURCE points at separate source output."""
import json,os,traceback
from pathlib import Path
import unreal
ROOT=Path(os.environ['S47_ACCESS_SOURCE']);REPORT=Path(os.environ['S47_ACCESS_REPORT'])
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report={'status':'RUNNING','parts':[]}
def check(name,good):
 assert good,name

def mesh(path):
 options=unreal.FbxImportUI();options.automated_import_should_detect_type=False;options.import_as_skeletal=False;options.import_mesh=True;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.import_materials=False;options.import_textures=False
 d=options.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES);d.import_rotation=unreal.Rotator(pitch=0,yaw=90,roll=0);d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
 task=unreal.AssetImportTask();task.filename=str(path);task.destination_path='/Game/CalicheAccess';task.destination_name=path.stem;task.replace_existing=True;task.replace_existing_settings=True;task.automated=True;task.save=True;task.options=options;task.factory=unreal.FbxFactory();T.import_asset_tasks([task]);m=E.load_asset('/Game/CalicheAccess/'+path.stem);check(path.stem+' mesh',isinstance(m,unreal.StaticMesh));return m
try:
 j=json.loads((ROOT/'source.json').read_text())
 for name,row in j['parts'].items():
  m=E.load_asset('/Game/CalicheAccess/'+name) if os.environ.get('S47_ACCESS_RESUME')=='1' else None
  retained=isinstance(m,unreal.StaticMesh) and S.get_lod_count(m)==3 and [m.get_num_triangles(l)for l in range(3)]==row['lod_triangles']
  if not retained:m=mesh(ROOT/'exports'/(name+'.fbx'))
  for i,slot in enumerate(m.get_editor_property('static_materials')):
   ma=str(slot.get_editor_property('material_slot_name'));material_name={'M_Town_Linoleum':'M_Town_FloorTile'}.get(ma,ma);mat=E.load_asset(('/Game/CalicheStreet/Souvenir/Materials/' if ma.startswith('M_Souvenir_') else '/Game/TownBuildings/Materials/')+material_name);check(ma+' shared original material',isinstance(mat,unreal.MaterialInterface));m.set_material(i,mat)
  if not retained:
   for lod in (1,2):check(name+' authored LOD',S.import_lod(m,lod,str(ROOT/'exports'/(name+'_LOD'+str(lod)+'.fbx')))==lod)
  n=m.get_editor_property('nanite_settings');n.enabled=False;m.set_editor_property('nanite_settings',n)
  body=m.get_editor_property('body_setup')
  if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
  for lod in range(S.get_lod_count(m)):
   settings=S.get_lod_build_settings(m,lod);settings.set_editor_property('use_full_precision_u_vs',True);S.set_lod_build_settings(m,lod,settings)
  for lod in range(S.get_lod_count(m)):
   for section in range(m.get_num_sections(lod)):S.enable_section_collision(m,True,lod,section)
  E.save_loaded_asset(m);check(name+' LOD counts',[m.get_num_triangles(l)for l in range(3)]==row['lod_triangles']);row['asset']=m.get_path_name();report['parts'].append(name)
 j['status']='NATIVE_READY';dest=Path(unreal.Paths.project_content_dir())/'CalicheAccess';dest.mkdir(exist_ok=True);(dest/'assembly.json').write_text(json.dumps(j,indent=2)+'\n');report['status']='PASS'
except Exception:report['status']='FAIL';report['error']=traceback.format_exc();raise
finally:REPORT.write_text(json.dumps(report,indent=2)+'\n')
unreal.log('CALICHE_ACCESS_IMPORT_PASS')
