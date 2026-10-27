"""Run by guarded UE Python commandlet against the game project, without Creator/cloud.

S47_UNIFORM_FBX: private output of ArtSource/WaitressUniform/build_uniform.py.
S47_METAHUMAN_REPORT: private JSON evidence destination.
Never changes the authored character, rig, face takes, sky or game state.
"""
import hashlib
import json
import os
from pathlib import Path
import unreal

fbx = Path(os.environ['S47_UNIFORM_FBX']).expanduser().resolve()
report_path = Path(os.environ['S47_METAHUMAN_REPORT']).expanduser().resolve()
if not fbx.is_file():
    raise RuntimeError('Uniform FBX is missing')
body_path = '/Game/MetaHumans/MHC_Waitress/Body/SKM_MHC_Waitress_BodyMesh'
face_path = '/Game/MetaHumans/MHC_Waitress/Face/SKM_MHC_Waitress_FaceMesh'
body, face = unreal.load_asset(body_path), unreal.load_asset(face_path)
if not body or not face:
    raise RuntimeError('The locally assembled MetaHuman body/face must exist first')
skeleton = body.get_editor_property('skeleton')
destination = '/Game/MetaHumans/Waitress/Uniform'
task = unreal.AssetImportTask()
task.filename = str(fbx)
task.destination_path = destination
task.destination_name = 'SKM_Waitress_Uniform'
task.automated = True
task.replace_existing = True
task.save = True
options = unreal.FbxImportUI()
options.import_as_skeletal = True
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.original_import_type = unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.skeleton = skeleton
options.import_materials = False
options.import_textures = False
options.import_animations = False
options.create_physics_asset = False
options.skeletal_mesh_import_data.set_editor_property('import_content_type',unreal.FBXImportContentType.FBXICT_ALL)
task.options = options
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
uniform = unreal.load_asset(destination + '/SKM_Waitress_Uniform')
if not uniform or uniform.get_editor_property('skeleton') != skeleton:
    raise RuntimeError('Uniform failed to import on the exact body skeleton')

palette = {
    'M_Uniform_Blue': ((.29,.43,.62), .82),
    'M_Uniform_White': ((.82,.80,.72), .82),
    'M_Apron': ((.87,.85,.77), .86),
    'M_Uniform_Seam': ((.19,.30,.46), .83),
    'M_Uniform_Shoe': ((.025,.022,.021), .43),
    'M_Uniform_Sole': ((.012,.011,.010), .88),
    'M_Uniform_Button': ((.70,.69,.63), .40),
}
materials = {}
tools = unreal.AssetToolsHelpers.get_asset_tools()
for name,(colour,roughness) in palette.items():
    path = destination + '/Materials/' + name
    material = unreal.load_asset(path)
    if not material:
        material = tools.create_asset(name, destination+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        rgb = unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-300,0)
        rgb.constant = unreal.LinearColor(*colour,1)
        rough = unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-300,120)
        rough.r = roughness
        specular = unreal.MaterialEditingLibrary.create_material_expression(material,unreal.MaterialExpressionConstant,-300,220)
        specular.r = .22
        unreal.MaterialEditingLibrary.connect_material_property(rgb,'',unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        unreal.MaterialEditingLibrary.connect_material_property(specular,'',unreal.MaterialProperty.MP_SPECULAR)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    materials[name] = material
slots = uniform.get_editor_property('materials')
for slot in slots:
    name = str(slot.material_slot_name)
    if name not in materials:
        raise RuntimeError('Unexpected uniform material slot: '+name)
    slot.material_interface = materials[name]
uniform.set_editor_property('materials',slots)
unreal.EditorAssetLibrary.save_loaded_asset(uniform)
bounds=uniform.get_bounds()
if not 50 < bounds.box_extent.z < 100:
    raise RuntimeError('Unexpected uniform height in cm: '+str(bounds))

# The copy is a cooked runtime dependency. Never cook the Creator/live-link actor graph.
anim_path='/Game/MetaHumans/Waitress/Anims/ABP_Face_Waitress'
if not unreal.EditorAssetLibrary.does_asset_exist(anim_path):
    if not unreal.EditorAssetLibrary.duplicate_asset('/Game/MetaHumans/Common/Face/ABP_Face',anim_path):
        raise RuntimeError('Cannot create runtime face animation blueprint')
if not unreal.EditorAssetLibrary.load_blueprint_class(anim_path):
    raise RuntimeError('Runtime face animation class is missing')

registry=unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(['/Game/MetaHumans/Waitress'],force_rescan=True)
take_assets=registry.get_assets_by_path('/Game/MetaHumans/Waitress/FaceAnims',recursive=True)
take_reports=[]
face_skeleton=face.get_editor_property('skeleton')
for asset in take_assets:
    if str(asset.asset_class_path.asset_name)!='AnimSequence':continue
    sequence=asset.get_asset()
    curves=unreal.AnimationLibrary.get_animation_curve_names(sequence,unreal.RawCurveTrackTypes.RCT_FLOAT)
    if sequence.get_editor_property('skeleton')!=face_skeleton or sequence.get_play_length()<=0 or len(curves)<100:
        raise RuntimeError('Invalid or incompatible face take: '+sequence.get_path_name())
    take_reports.append({'path':sequence.get_path_name(),'duration':sequence.get_play_length(),'curve_count':len(curves),'skeleton':face_skeleton.get_path_name()})
if len(take_reports)!=43:
    raise RuntimeError('Expected 43 validated face takes, found '+str(len(take_reports)))
report = {'status':'PASS','uniform_fbx_sha256':hashlib.sha256(fbx.read_bytes()).hexdigest(),
          'uniform':uniform.get_path_name(),'skeleton':skeleton.get_path_name(),
          'uniform_bounds_cm':{'origin':[bounds.origin.x,bounds.origin.y,bounds.origin.z],'half_extent':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z]},
          'materials':[str(slot.material_slot_name) for slot in slots],'takes':take_reports,
          'blueprint_class':anim_path,'source':'existing licensed local Creator assembly; no new cloud build'}
report_path.parent.mkdir(parents=True,exist_ok=True)
report_path.write_text(json.dumps(report,indent=2)+'\n')
print('S47_RUNTIME_PREPARED',len(take_reports),'validated face takes',uniform.get_path_name())
