"""Fast own-material iteration; root runs one guarded Unreal job, no mesh import."""
from pathlib import Path
import importlib.util
import json
import os
import unreal

here=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('s47_finale_material_recipe',here/'import_assets.py')
recipe=importlib.util.module_from_spec(spec)
spec.loader.exec_module(recipe)
contract=json.loads((here.parents[2]/'ArtSource/FinaleSky/source-contract.json').read_text())
recipe.make_materials(contract)
cloud = recipe.load('M_S47_FinalCloud')
volume_names = ['V_S47_CloudDensity'+suffix for suffix in 'ABC']
volume_paths = ['/Game/FinaleSky/Clouds/'+name for name in volume_names]
mode = os.environ.get('S47_CLOUD_FIELD', 'auto')
assert mode in ('auto', 'analytic', 'volume')
use_volume = mode != 'analytic' and all(recipe.E.does_asset_exist(path) for path in volume_paths)
if mode == 'volume':
    assert use_volume, 'Import the three owned Mantaflow density volumes first'
if use_volume:
    fields = [n for n in recipe.M.get_material_expressions(cloud) if isinstance(n, unreal.MaterialExpressionCustom)]
    assert len(fields) == 1
    field = fields[0]
    rows = list(field.get_editor_property('inputs'))
    assert len(rows) == 13
    for name in ('DensityA', 'DensityB', 'DensityC'):
        row = unreal.CustomInput()
        row.set_editor_property('input_name', name)
        rows.append(row)
    row = unreal.CustomInput()
    row.set_editor_property('input_name', 'Density')
    rows.append(row)
    row = unreal.CustomInput()
    row.set_editor_property('input_name', 'DecodeScale')
    rows.append(row)
    field.set_editor_property('inputs', rows)
    field.set_editor_property('code', (here/'cloud_volume.hlsl').read_text())
    scales = []
    for name, path in zip(('DensityA', 'DensityB', 'DensityC'), volume_paths):
        texture = recipe.E.load_asset(path)
        assert isinstance(texture, unreal.VolumeTexture)
        scales.append(float(recipe.E.get_metadata_tag(texture, 'S47DensityScale') or '1'))
        sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE
                   if texture.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_GRAYSCALE
                   else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        tex = recipe.node(cloud, unreal.MaterialExpressionTextureObjectParameter,
                          parameter_name=name, texture=texture,
                          sampler_type=sampler)
        recipe.expression_link(cloud, tex, '', field, name)
    density = recipe.scalar(cloud, 'Density', 1)
    recipe.expression_link(cloud, density, '', field, 'Density')
    assert max(scales)-min(scales) < 1.e-6, 'Cloud frames require one common density decode'
    decode = recipe.scalar(cloud, 'DecodeScale', scales[0])
    recipe.expression_link(cloud, decode, '', field, 'DecodeScale')
    assert len(recipe.M.get_inputs_for_material_expression(cloud, field)) == 18
# Runtime draws only cube exit faces. Ray integration still terminates at SceneDepth.
cloud.set_editor_property('disable_depth_test', True)
# Fine cloud silhouettes must not inherit a coarse image-based shading rate.
# This is local to the two cloud proxies; their AFTER_DOF ordering is unchanged.
cloud.set_editor_property('allow_variable_rate_shading', False)
# Share the stars' pass so the finite clouds can cover them; foreground glass sorts later.
cloud.set_editor_property('translucency_pass', unreal.MaterialTranslucencyPass.MTP_AFTER_DOF)
recipe.M.recompile_material(cloud)
assert recipe.E.save_loaded_asset(cloud)
unreal.log('S47 FINALE MATERIAL ITERATION COMPLETE; field='+('Mantaflow volume' if use_volume else 'analytic')+'; own assets only')
