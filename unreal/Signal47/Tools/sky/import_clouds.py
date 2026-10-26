"""SIGNAL / 47 volumetric cloud material (SKY.md). Writes only /Game/Sky/M_S47_Clouds.

Run with the editor's commandlet (Tools/sky/import.sh). Procedural density in clouds.hlsl; the
runtime (S47Clouds.cpp) drives coverage, cirrus, wind, the story's clear directions and the flash.
"""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parent
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
DEST, NAME = '/Game/Sky', 'M_S47_Clouds'
path = DEST + '/' + NAME
mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
for previous in list(M.get_material_expressions(mat)):
    M.delete_material_expression(mat, previous)
assert M.get_num_material_expressions(mat) == 0
mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_VOLUME)
mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
mat.set_editor_property('used_with_volumetric_cloud', True)
y = [0]


def node(cls, **props):
    y[0] += 120
    n = M.create_material_expression(mat, cls, -1200, y[0])
    for key, value in props.items():
        n.set_editor_property(key, value)
    return n


def scalar(key, value):
    return node(unreal.MaterialExpressionScalarParameter, parameter_name=key, default_value=value)


def vector(key, value):
    return node(unreal.MaterialExpressionVectorParameter, parameter_name=key, default_value=unreal.LinearColor(*value))


def custom(code, inputs, out=3):
    kinds = {1: unreal.CustomMaterialOutputType.CMOT_FLOAT1, 3: unreal.CustomMaterialOutputType.CMOT_FLOAT3,
             4: unreal.CustomMaterialOutputType.CMOT_FLOAT4}
    n = node(unreal.MaterialExpressionCustom, code=code, output_type=kinds[out])
    rows = []
    for key, _source, _output in inputs:
        ci = unreal.CustomInput(); ci.set_editor_property('input_name', key); rows.append(ci)
    n.set_editor_property('inputs', rows)
    for key, source, output in inputs:
        assert M.connect_material_expressions(source, output, n, key), key
    return n


def v4(key, value):
    v = vector(key, value)
    return custom('return float4(C, A);', [('C', v, ''), ('A', v, 'A')], 4)


wp = node(unreal.MaterialExpressionWorldPosition)
cam = node(unreal.MaterialExpressionCameraPositionWS)
alt = node(unreal.MaterialExpressionCloudSampleAttribute)
cover, cirrus, ext = scalar('Cover', .55), scalar('Cirrus', .4), scalar('Ext', .010)
wind = vector('Wind', (0, 0, 0, 0))
clear0, clear1 = v4('Clear0', (0, 0, 1, 0)), v4('Clear1', (0, 0, 1, 0))
code = (ROOT / 'clouds.hlsl').read_text()
inputs = lambda mode: [('WP', wp, ''), ('Alt', alt, 'NormAltitudeInLayer'), ('Cam', cam, ''), ('Cover', cover, ''),
                       ('Cirrus', cirrus, ''), ('Wind', wind, ''), ('Clear0', clear0, ''), ('Clear1', clear1, ''),
                       ('Ext', ext, ''), ('Mode', mode, '')]
full = custom(code, inputs(node(unreal.MaterialExpressionConstant, r=0.)))
conservative = custom(code, inputs(node(unreal.MaterialExpressionConstant, r=1.)))
# Extinction: alto in r, cirrus in g. Albedo: bright, slightly warmer in the thin cirrus.
extinction = custom('return (D.r + D.g).xxx;', [('D', full, '')])
albedo = custom('float a = D.r + D.g; return a > 0. ? lerp(float3(.93,.94,.96), float3(.97,.97,.98), D.g / max(a, 1e-6)) : float3(.93,.94,.96);', [('D', full, '')])
# The night's horizon lightning (Sky.ts uFlash) lights the clouds toward it from inside.
# Emissive is per metre like the extinction: D.r * colour converges to that colour's luminance in a
# thick cloud. Ambient is the art-directed sky light on the cloud (S47Clouds.cpp sets it from the clock),
# so undersides read blue-grey at dusk instead of black; the flash adds the horizon lightning.
flash, flash_dir, ambient = scalar('Flash', 0), vector('FlashDir', (.957, .048, -.287, 0)), vector('Ambient', (.1, .1, .14, 0))
emissive = custom('float3 d = normalize(WP - Cam); float lower = 1. - .45 * saturate(Alt * 2.5); return D.r * (Ambient * lower + float3(.20,.25,.40) * Flash * 40. * pow(saturate(dot(d, FlashDir) * .5 + .5), 10.));',
                  [('WP', wp, ''), ('Cam', cam, ''), ('Alt', alt, 'NormAltitudeInLayer'), ('Flash', flash, ''), ('FlashDir', flash_dir, ''), ('Ambient', ambient, ''), ('D', full, '')])
cons = custom('return D.r;', [('D', conservative, '')], 1)
assert M.connect_material_property(albedo, '', unreal.MaterialProperty.MP_BASE_COLOR)
assert M.connect_material_property(extinction, '', unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
assert M.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
adv = node(unreal.MaterialExpressionVolumetricAdvancedMaterialOutput)
for key, value in {'const_phase_g': .62, 'const_phase_g2': -.28, 'const_phase_blend': .32,
                   'const_multi_scattering_contribution': .7, 'const_multi_scattering_occlusion': .3,
                   'const_multi_scattering_eccentricity': .6, 'gray_scale_material': True,
                   'ray_march_volume_shadow': True, 'ground_contribution': False}.items():
    adv.set_editor_property(key, value)
assert M.connect_material_expressions(cons, '', adv, 'ConservativeDensity')
errors = list(M.recompile_material(mat))
assert not errors, errors
E.save_loaded_asset(mat)
unreal.log_warning('S47 CLOUDS SAVED ' + path)
