"""Create a conventional masked material for the licensed bob's rigid hair cards.

Run with guarded UE Python against the local assembled character. No vendor data
is embedded here. The generated material references the local coverage atlas and
stays in ignored Content/MetaHumans, alongside the licensed card geometry.
"""
import unreal
folder='/Game/MetaHumans/Waitress/Uniform'
name='M_Waitress_HairCards'
texture=unreal.load_asset('/Game/MetaHumans/Common/Optional/Grooms/GroomAssets/Hair/Hair_M_Layered/Hair_M_Layered_RooUVSeedCoverage')
mesh=unreal.load_asset('/Game/MetaHumans/MHC_Waitress/Grooms/Hair_M_Layered_CardsMesh_Group0_LOD1')
if not texture or not mesh:raise RuntimeError('Authored hair cards/coverage atlas missing')
mat=unreal.load_asset(folder+'/'+name)
if not mat:mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
lib=unreal.MaterialEditingLibrary
lib.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
mat.set_editor_property('two_sided',True)
mat.set_editor_property('opacity_mask_clip_value',.22)
color=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
color.set_editor_property('constant',unreal.LinearColor(.022,.012,.008,1))
lib.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant)
rough.set_editor_property('r',.52)
lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
tex=lib.create_material_expression(mat,unreal.MaterialExpressionTextureSample)
tex.set_editor_property('texture',texture)
# Static mesh UV0 is atlas UV. Layout3's packed coverage is the alpha channel.
lib.connect_material_property(tex,'A',unreal.MaterialProperty.MP_OPACITY_MASK)
lib.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('S47: prepared rigid card material '+mat.get_path_name())
