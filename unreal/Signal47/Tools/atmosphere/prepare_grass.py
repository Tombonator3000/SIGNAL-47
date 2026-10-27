"""Add bounded grass-only wind to a copy of the project's own foliage material.

The shared tree/shrub material and every existing mesh remain unchanged. Run via
UE Python commandlet and project guard; output is an original generated material.
"""
import unreal
base='/Game/CalicheStreet/NatureVegetation/Materials'
source=unreal.load_asset(base+'/M_CN_Foliage')
collection=unreal.load_asset('/Game/Web/Base/MPC_Web')
if not source or not collection:raise RuntimeError('Existing foliage material / wind collection missing')
name='M_CN_GrassWind'; mat=unreal.load_asset(base+'/'+name)
if mat:raise RuntimeError('Wind material already exists; inspect before replacing an authored result')
mat=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name,base,source)
lib=unreal.MaterialEditingLibrary
wpo=lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
wpo.set_editor_property('description','Bounded grass tips, world wind, fixed roots; 0.28 x existing wind amplitude')
wpo.set_editor_property('code','''float weight = pow(saturate(LP.z / 25.0), 2.0);
float phase = WT * 1.4 + dot(WP.xy, float2(0.0017,0.0013));
float gust = sin(phase) * 0.65 + sin(phase * 2.31) * 0.2;
float nearFade = 1.0 - smoothstep(2000.0, 7000.0, distance(WP, Cam));
return float3(0.8,0.6,0.0) * min(WA * 28.0, 2.2) * weight * gust * nearFade;''')
wpo.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs=[]
for name in ['LP','WP','WT','WA','Cam']:
 i=unreal.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
wpo.set_editor_property('inputs',inputs)
local=lib.create_material_expression(mat,unreal.MaterialExpressionPreSkinnedPosition)
world=lib.create_material_expression(mat,unreal.MaterialExpressionWorldPosition)
camera=lib.create_material_expression(mat,unreal.MaterialExpressionCameraPositionWS)
lib.connect_material_expressions(local,'',wpo,'LP');lib.connect_material_expressions(world,'',wpo,'WP');lib.connect_material_expressions(camera,'',wpo,'Cam')
for name,target in [('WindTime','WT'),('WindAmp','WA')]:
 node=lib.create_material_expression(mat,unreal.MaterialExpressionCollectionParameter)
 node.set_editor_property('collection',collection);node.set_editor_property('parameter_name',name)
 lib.connect_material_expressions(node,'',wpo,target)
lib.connect_material_property(wpo,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('S47: grass-only wind prepared '+mat.get_path_name())
