"""Import private Caliche pump candidates; does not change original assets.
Set S47_PUMP_MANIFEST=/private/prepared/manifest.json and S47_PUMP_APPLY=1
then execute in the leased editor. Outside Unreal this prints a validated plan.
"""
from pathlib import Path
import argparse,importlib.util,json,os,sys
HERE=Path(__file__).resolve().parent
REPO=HERE.parents[3]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
F=module('s47_pump_fleet_import',HERE.parent/'tripofleet/import_assets.py')
F.DEST='/Game/Licensed/TripoPump'
F.OWNER='S47TripoPump_v1'
STATION_DEST=F.DEST+'/Station'
def station_import(contract,report):
 import unreal
 S=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);E=unreal.EditorAssetLibrary
 R=module('s47_pump_recipe',HERE.parent/'fieldintegration/import_recipe.py')
 data=json.loads(contract.read_text());source=unreal.load_asset(data['originalAsset']);assert isinstance(source,unreal.StaticMesh)
 original_slots=list(source.get_editor_property('static_materials'));materials={str(v.material_slot_name):v.material_interface for v in original_slots}
 before_lods=S.get_lod_count(source);before_tri=[source.get_num_triangles(l) for l in range(before_lods)]
 flag='Interchange.FeatureFlags.Import.FBX';old=unreal.SystemLibrary.get_console_variable_int_value(flag)
 try:
  unreal.SystemLibrary.execute_console_command(None,flag+' 0');mesh=R.import_mesh(Path(data['fbx']),STATION_DEST);assert isinstance(mesh,unreal.StaticMesh)
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   name=str(slot.material_slot_name);assert name in materials,('Original station material slot missing',name);mesh.set_material(i,materials[name])
  assert all(str(v.material_slot_name) in materials for v in mesh.get_editor_property('static_materials')),'Station material identity changed'
  mesh.set_editor_property('allow_cpu_access',True)
  build=S.get_lod_build_settings(mesh,0);build.set_editor_property('use_full_precision_u_vs',True);build.set_editor_property('remove_degenerates',False);S.set_lod_build_settings(mesh,0,build)
  # Every far LOD is derived from the already-cut station. An old pump cannot
  # reappear by selecting an original untrimmed far LOD.
  if before_lods>1:
   options=unreal.StaticMeshReductionOptions();options.set_editor_property('auto_compute_lod_screen_size',False);settings=[];sizes=list(S.get_lod_screen_sizes(source))
   for lod in range(before_lods):
    v=unreal.StaticMeshReductionSettings();v.set_editor_property('percent_triangles',before_tri[lod]/before_tri[0]);v.set_editor_property('screen_size',sizes[lod]);settings.append(v)
   options.set_editor_property('reduction_settings',settings);assert S.set_lods(mesh,options)==before_lods;S.set_lod_screen_sizes(mesh,sizes)
  # Inspect actual render geometry, not import bounds alone.
  options=unreal.GeometryScriptCopyMeshFromAssetOptions();requested=unreal.GeometryScriptMeshReadLOD();requested.set_editor_property('lod_type',unreal.GeometryScriptLODType.RENDER_DATA);requested.set_editor_property('lod_index',0)
  dynamic,outcome=unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh,unreal.DynamicMesh(),options,requested);assert outcome==unreal.GeometryScriptOutcomePins.SUCCESS
  _,values,_=unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic,True);positions=[(p.x,p.y,p.z) for p in unreal.GeometryScript_List.convert_vector_list_to_array(values)]
  actual=[[fn(p[k] for p in positions) for k in range(3)] for fn in (min,max)];lo,hi=data['after']['bounds_local_m'];expected=[[lo[0]*100,-hi[1]*100,lo[2]*100],[hi[0]*100,-lo[1]*100,hi[2]*100]]
  assert max(abs(a-b) for aa,bb in zip(actual,expected) for a,b in zip(aa,bb))<.2,('Station axes/bounds differ',actual,expected)
  assert mesh.get_num_triangles(0)==data['after']['triangles'],'Station render triangle count differs'
  assert E.save_loaded_asset(mesh,only_if_is_dirty=False)
  report.update(status='PASS_NATIVE_CANDIDATES',station={'asset':mesh.get_path_name(),'originalLods':before_tri,'candidateLods':[mesh.get_num_triangles(l) for l in range(S.get_lod_count(mesh))],'boundsCm':actual,'originalMaterials':list(materials),'removedTriangles':data['removedTriangles']})
 finally:unreal.SystemLibrary.execute_console_command(None,flag+' '+str(old))
 return report

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',default=os.environ.get('S47_PUMP_MANIFEST'));p.add_argument('--apply',action='store_true',default=os.environ.get('S47_PUMP_APPLY')=='1');p.add_argument('--stage',choices=['pump','station','all'],default=os.environ.get('S47_PUMP_STAGE','all'));a=p.parse_args([] if 'unreal' in sys.modules else None)
 assert a.manifest,'S47_PUMP_MANIFEST required';manifest=Path(a.manifest).resolve();assert not manifest.is_relative_to(REPO)
 plan=F.make_plan(str(manifest),['caliche_regular_pump']);contract=manifest.parent/'station/station-contract.json'
 if not a.apply:print(json.dumps(plan,indent=2));return
 report={}
 if a.stage in ('pump','all'):report=F.import_candidates(plan,manifest.parent/'pump-native-import.json')
 if a.stage in ('station','all'):report=station_import(contract,report)
 (manifest.parent/'native-import.json').write_text(json.dumps(report,indent=2)+'\n')
 print('S47_PUMP_NATIVE_READY',manifest,flush=True)
if __name__=='__main__':main()
