"""Root-run UE import only. Reuses the established FBX/material API without other imports.
All licensed derivatives stay /Game/Free/SaroFleet; generated source lives outside public Git.
"""
import os,json,traceback
from pathlib import Path
import unreal
HERE=Path(__file__).resolve().parent
WORK=Path(os.environ.get('S47_FLEET_WORK','/home/tom/scratch-bm/Signal47-production-models/fleet'))
BASE=HERE.parent/'radioarray'/'import_assets.py'
# Only definitions above import_set. No dish/station import or global build is executed.
ns={'__file__':str(BASE),'__name__':'saro_fleet_material_helpers'}
exec(compile(BASE.read_text().split('\ndef import_set(')[0],str(BASE),'exec'),ns)
ns['REPORT']=WORK/'native-import.json';ns['report']={'checks':[],'meshes':[]}
check=ns['check'];E=unreal.EditorAssetLibrary;T=unreal.AssetToolsHelpers.get_asset_tools();M=unreal.MaterialEditingLibrary
S=(unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem))
DEST='/Game/Free/ServiceFleet'
OWN_DEST='/Game/SaroFleet'

def imported_texture(filename,name,srgb):
 task=unreal.AssetImportTask();task.filename=str(WORK/filename);task.destination_path=DEST+'/Textures';task.destination_name=name;task.replace_existing=True;task.automated=True;task.save=True;T.import_asset_tasks([task]);tex=E.load_asset(task.destination_path+'/'+name);check(name+' imported',isinstance(tex,unreal.Texture2D));tex.set_editor_property('srgb',srgb);E.save_loaded_asset(tex);return tex


def import_material(row):
 # Share this exact material-only path with repairs; never reimport vehicle geometry.
 material_dest=(OWN_DEST if row['name'].startswith('M_Fleet_') else DEST)+'/Materials'
 mat=ns['material'](row,material_dest,WORK)
 if row.get('textures',{}).get('base_color'):
  # glTF multiplies sampled sRGB colour by the linear BaseColor factor.
  sample=M.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
  factor=next(n for n in M.get_material_expressions(mat) if isinstance(n,unreal.MaterialExpressionVectorParameter) and str(n.get_editor_property('parameter_name'))=='Color')
  multiply=ns['expression'](mat,unreal.MaterialExpressionMultiply)
  check(row['name']+' sampled base colour',isinstance(sample,unreal.MaterialExpressionTextureSample))
  check(row['name']+' base texture factor A',M.connect_material_expressions(sample,'RGB',multiply,'A'))
  check(row['name']+' base texture factor B',M.connect_material_expressions(factor,'RGB',multiply,'B'))
  check(row['name']+' base texture factor output',M.connect_material_property(multiply,'',unreal.MaterialProperty.MP_BASE_COLOR))
 if row.get('normal_format')=='OpenGL' and row.get('textures',{}).get('normal'):
  tex=E.load_asset(material_dest+'/Textures/T_'+row['name']+'_normal');tex.set_editor_property('flip_green_channel',True);E.save_loaded_asset(tex)
 if row.get('packed_metallic_roughness'):
  tex=imported_texture(row['packed_metallic_roughness'],'T_'+row['name']+'_ORM',False)
  n=ns['expression'](mat,unreal.MaterialExpressionTextureSample,texture=tex,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
  check(row['name']+' roughness G',M.connect_material_property(n,'G',unreal.MaterialProperty.MP_ROUGHNESS));check(row['name']+' metallic B',M.connect_material_property(n,'B',unreal.MaterialProperty.MP_METALLIC))
 check(row['name']+' final material compiles',not list(M.recompile_material(mat)));E.save_loaded_asset(mat)
 return mat

def main():
 contract=json.loads((WORK/'model_contract.json').read_text());rows=json.loads((WORK/'materials.json').read_text());mats={}
 for row in rows:mats[row['name']]=import_material(row)
 for name,part in contract['parts'].items():
  mesh_dest=part['asset_folder'];path=mesh_dest+'/'+name
  if E.does_asset_exist(path):check(name+' fresh generated asset',E.delete_asset(path))
  mesh=ns['import_mesh'](WORK/'exports'/(name+'.fbx'),mesh_dest)
  slots=list(mesh.get_editor_property('static_materials'))
  if name!='SM_SaroPark_LegacyTrim':
   for i,slot in enumerate(slots):
    n=str(slot.material_slot_name);check(name+' material '+n,n in mats);mesh.set_material(i,mats[n])
  check(name+' exact LOD0 triangles',mesh.get_num_triangles(0)==part['triangles'],[mesh.get_num_triangles(0),part['triangles']])
  for lod in [1,2]:
   got=S.import_lod(mesh,lod,str(WORK/'exports'/(name+'_LOD'+str(lod)+'.fbx')));check(name+' imported authored LOD'+str(lod),got==lod)
   check(name+' exact LOD'+str(lod)+' triangles',mesh.get_num_triangles(lod)==part['lods'][lod]['triangles'],[mesh.get_num_triangles(lod),part['lods'][lod]['triangles']])
  check(name+' authored LOD screen sizes',S.set_lod_screen_sizes(mesh,[1.0,.42,.16]))
  E.save_loaded_asset(mesh)
  ns['report']['meshes'].append({'name':name,'triangles':mesh.get_num_triangles(0),'asset':path})
 ns['report']['status']='PASS';ns['REPORT'].write_text(json.dumps(ns['report'],indent=2));unreal.log('S47_FLEET_IMPORT_PASS '+str(ns['REPORT']))
try:main()
except Exception:
 ns['report']['status']='FAIL';ns['report']['error']=traceback.format_exc();ns['REPORT'].write_text(json.dumps(ns['report'],indent=2));raise
