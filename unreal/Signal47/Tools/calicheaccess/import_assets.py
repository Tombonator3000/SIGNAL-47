"""Root's serial Unreal Python import. S47_ACCESS_SOURCE points at separate source output."""
import json,os,traceback,sys
from pathlib import Path
import unreal
sys.path.insert(0,str(Path(__file__).resolve().parent))
from lod_geometry import verify_lod
ROOT=Path(os.environ['S47_ACCESS_SOURCE']);REPORT=Path(os.environ['S47_ACCESS_REPORT'])
E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
report={'status':'RUNNING','parts':[]}
def check(name,good):
 assert good,name

def mesh(path):
 # An Interchange-created predecessor can retain its translator/import scale
 # through AssetTools' replacement path. Recreate only this generated namespace.
 asset='/Game/CalicheAccess/'+path.stem
 if E.does_asset_exist(asset):check(path.stem+' remove stale import provenance',E.delete_asset(asset))
 options=unreal.FbxImportUI();options.automated_import_should_detect_type=False;options.import_as_skeletal=False;options.import_mesh=True;options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;options.import_materials=False;options.import_textures=False
 d=options.static_mesh_import_data;d.combine_meshes=True;d.auto_generate_collision=False;d.generate_lightmap_u_vs=True;d.convert_scene=True;d.convert_scene_unit=True;d.force_front_x_axis=True;d.set_editor_property('coordinate_system_policy',unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES);d.import_rotation=unreal.Rotator(pitch=0,yaw=90,roll=0);d.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
 d.import_uniform_scale=1.
 task=unreal.AssetImportTask();task.filename=str(path);task.destination_path='/Game/CalicheAccess';task.destination_name=path.stem;task.replace_existing=True;task.replace_existing_settings=True;task.automated=True;task.save=True;task.options=options;task.factory=unreal.FbxFactory();T.import_asset_tasks([task]);m=E.load_asset('/Game/CalicheAccess/'+path.stem);check(path.stem+' mesh',isinstance(m,unreal.StaticMesh));return m
flag='Interchange.FeatureFlags.Import.FBX';previous=unreal.SystemLibrary.get_console_variable_int_value(flag)
try:
 unreal.SystemLibrary.execute_console_command(None,flag+' 0')
 check('legacy FBX translator for all source/LOD imports',unreal.SystemLibrary.get_console_variable_int_value(flag)==0)
 j=json.loads((ROOT/'source.json').read_text())
 for name,row in j['parts'].items():
  m=E.load_asset('/Game/CalicheAccess/'+name) if os.environ.get('S47_ACCESS_RESUME')=='1' else None
  retained=isinstance(m,unreal.StaticMesh) and S.get_lod_count(m)==3 and [m.get_num_triangles(l)for l in range(3)]==row['lod_triangles']
  if retained:
   try:
    geometry=[verify_lod(m,lod,ROOT/'exports'/(name+('_LOD'+str(lod)if lod else'')+'.fbx'),row['lod_triangles'][lod])for lod in range(3)]
    check(name+' resume Nanite disabled',not m.get_editor_property('nanite_settings').enabled)
    for lod in range(3):
     settings=S.get_lod_build_settings(m,lod);scale=settings.get_editor_property('build_scale3d')
     check(name+' resume unit build scale and precision',settings.get_editor_property('use_full_precision_u_vs') and max(abs(v-1)for v in [scale.x,scale.y,scale.z])<1e-6)
    for i,slot in enumerate(m.get_editor_property('static_materials')):
     ma=str(slot.get_editor_property('material_slot_name'));material_name={'M_Town_Linoleum':'M_Town_FloorTile'}.get(ma,ma)
     expected=E.load_asset(('/Game/CalicheStreet/Souvenir/Materials/'if ma.startswith('M_Souvenir_')else'/Game/TownBuildings/Materials/')+material_name)
     check(name+' resume retains original PBR',expected is not None and m.get_material(i)==expected)
   except AssertionError as reason:
    unreal.log_warning('Caliche resume will replace '+name+': '+str(reason));retained=False
  if retained:
   # Property setters call PostEditChange even for identical values, rebuilding
   # all LODs unnecessarily. A completed resume must be strictly read-only.
   unreal.log('Caliche resume keeps measured native geometry '+name)
   row['asset']=m.get_path_name();report['parts'].append({'name':name,'geometry':geometry});continue
  if not retained:m=mesh(ROOT/'exports'/(name+'.fbx'))
  for i,slot in enumerate(m.get_editor_property('static_materials')):
   ma=str(slot.get_editor_property('material_slot_name'));material_name={'M_Town_Linoleum':'M_Town_FloorTile'}.get(ma,ma);mat=E.load_asset(('/Game/CalicheStreet/Souvenir/Materials/' if ma.startswith('M_Souvenir_') else '/Game/TownBuildings/Materials/')+material_name);check(ma+' shared original material',isinstance(mat,unreal.MaterialInterface));m.set_material(i,mat)
  if not retained:
   for lod in (1,2):check(name+' authored LOD',S.import_lod(m,lod,str(ROOT/'exports'/(name+'_LOD'+str(lod)+'.fbx')))==lod)
  n=m.get_editor_property('nanite_settings');n.enabled=False;m.set_editor_property('nanite_settings',n)
  body=m.get_editor_property('body_setup')
  if body:body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
  for lod in range(S.get_lod_count(m)):
   settings=S.get_lod_build_settings(m,lod);settings.set_editor_property('use_full_precision_u_vs',True);settings.set_editor_property('build_scale3d',unreal.Vector(1,1,1));S.set_lod_build_settings(m,lod,settings)
  for lod in range(S.get_lod_count(m)):
   for section in range(m.get_num_sections(lod)):S.enable_section_collision(m,True,lod,section)
  E.save_loaded_asset(m);check(name+' LOD counts',[m.get_num_triangles(l)for l in range(3)]==row['lod_triangles'])
  geometry=[verify_lod(m,lod,ROOT/'exports'/(name+('_LOD'+str(lod)if lod else'')+'.fbx'),row['lod_triangles'][lod])for lod in range(3)]
  row['asset']=m.get_path_name();report['parts'].append({'name':name,'geometry':geometry})
 j['status']='NATIVE_READY';dest=Path(unreal.Paths.project_content_dir())/'CalicheAccess';dest.mkdir(exist_ok=True);(dest/'assembly.json').write_text(json.dumps(j,indent=2)+'\n');report['status']='PASS'
except Exception:report['status']='FAIL';report['error']=traceback.format_exc();raise
finally:
 unreal.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
 REPORT.parent.mkdir(parents=True,exist_ok=True);REPORT.write_text(json.dumps(report,indent=2)+'\n')
unreal.log('CALICHE_ACCESS_IMPORT_PASS')
