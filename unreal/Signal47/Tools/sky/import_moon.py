"""PT-MOON-20261109: the moon's texture and material, /Game/Sky/T_S47_Moon and /Game/Sky/M_S47_Moon (UnrealEditor-Cmd Python).

Run by Tools/sky/import_moon.sh, one heavy Unreal job at a time. Touches nothing else in /Game/Sky.

T_S47_Moon: Tools/sky/moon/T_S47_Moon.png (bake_moon.py, NASA SVS CGI Moon Kit, public domain), the near side as a disc, north up.
Never streamed: a disc two degrees across far off would otherwise sit on a low mip and read as a pale blur.

M_S47_Moon, for the moon's plane in S47Sky (the engine's 1 m Plane, its local X the moon's north, local Y its right or left as Flip says):
- unlit, alpha composite: the lit side adds its light, the whole disc hides the stars behind it at night (the dark side too: OccDark),
  and by day the sky shows through it (OccLit, OccDark low), as a daytime moon is pale
- the phase from the sun's direction in the disc's own frame (SunL: x right, y up, z toward the eye), Lommel-Seeliger, so a full moon is
  as bright at its limb as in the middle and a half moon fades into the terminator; a little earthshine on the dark side (Earth)
- apply_cloud_fogging, as the stars (M_Caliche_Stars): the volumetric clouds in front of it hide it. Before 9 November the moon was an
  additive plane in M_Web_Unlit_Add, and it shone through every cloud.
"""
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parent
DEST = '/Game/Sky'
TEX, MAT = 'T_S47_Moon', 'M_S47_Moon'
OWNER = 'S47Moon_v1'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()


def owned_or_absent(path):
    if E.does_asset_exist(path) and E.get_metadata_tag(E.load_asset(path), 'S47Owner') != OWNER:
        raise RuntimeError('not owned: ' + path)


def import_texture():
    path = DEST + '/' + TEX
    owned_or_absent(path)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(ROOT / 'moon' / (TEX + '.png')))
    task.set_editor_property('destination_path', DEST)
    task.set_editor_property('destination_name', TEX)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    T.import_asset_tasks([task])
    tex = E.load_asset(path)
    if tex is None:
        raise RuntimeError('moon texture did not import')
    tex.set_editor_property('srgb', True)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
    tex.set_editor_property('never_stream', True)
    tex.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    tex.set_editor_property('virtual_texture_streaming', False)
    E.set_metadata_tag(tex, 'S47Owner', OWNER)
    if not E.save_loaded_asset(tex):
        raise RuntimeError('could not save ' + path)
    return tex


MOON = """
float2 d = P.xy / 50.0;                 // the engine's Plane: 100 units across
float2 q = float2(d.y * Flip, d.x);     // (right, up) as the eye sees the disc
float r = length(q);
float edge = saturate((1.0 - r) * 90.0);
if (edge <= 0.0) return float4(0, 0, 0, 0);
float3 n = float3(q, sqrt(saturate(1.0 - dot(q, q))));
float3 L = normalize(SunL);
float mu0 = saturate(dot(n, L));
float mu = max(n.z, 0.03);
// Lommel-Seeliger: no limb darkening at full, a soft fall into the terminator at the quarters
float lit = saturate(2.0 * mu0 / (mu0 + mu));
float3 alb = Texture2DSample(Map, MapSampler, float2(0.5 + 0.5 * q.x, 0.5 - 0.5 * q.y)).rgb;
float3 c = Color * alb * (lit + Earth);
float a = edge * lerp(OccDark, OccLit, saturate(lit * 3.0));
return float4(c * edge, a);
"""


def build_material(tex):
    path = DEST + '/' + MAT
    owned_or_absent(path)
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(MAT, DEST, unreal.Material, unreal.MaterialFactoryNew())
    for previous in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, previous)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_ALPHA_COMPOSITE)
    mat.set_editor_property('two_sided', True)
    # as the stars: the real volumetric clouds hide it, and the night's fog is on it as on them
    mat.set_editor_property('use_translucency_vertex_fog', True)
    mat.set_editor_property('apply_cloud_fogging', True)
    y = [0]

    def node(cls, **props):
        y[0] += 120
        n = M.create_material_expression(mat, cls, -900, y[0])
        for key, value in props.items():
            n.set_editor_property(key, value)
        return n

    def scalar(key, value):
        return node(unreal.MaterialExpressionScalarParameter, parameter_name=key, default_value=value)

    def vector(key, value):
        return node(unreal.MaterialExpressionVectorParameter, parameter_name=key, default_value=unreal.LinearColor(*value))

    inputs = [('P', node(unreal.MaterialExpressionLocalPosition)),
              ('Map', node(unreal.MaterialExpressionTextureObjectParameter, parameter_name='Map', texture=tex)),
              ('Color', vector('Color', (1.2, 1.16, 1.08, 0))), ('SunL', vector('SunL', (0, 0, 1, 0))),
              ('Flip', scalar('Flip', 1.)), ('Earth', scalar('Earth', .02)), ('OccLit', scalar('OccLit', 1.)), ('OccDark', scalar('OccDark', .9))]
    c = M.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 200)
    c.set_editor_property('code', MOON)
    c.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    rows = []
    for key, _ in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property('input_name', key)
        rows.append(ci)
    c.set_editor_property('inputs', rows)
    for key, source in inputs:
        assert M.connect_material_expressions(source, '', c, key), key
    rgb = M.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 150)
    alpha = M.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -250, 300)
    for mask, chans in ((rgb, 'rgb'), (alpha, 'a')):
        for ch in 'rgba':
            mask.set_editor_property(ch, ch in chans)
        assert M.connect_material_expressions(c, '', mask, '')
    assert M.connect_material_property(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    assert M.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
    errors = [str(e) for e in M.recompile_material(mat)]
    if errors:
        raise RuntimeError('M_S47_Moon failed: ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('could not save ' + path)
    return mat


tex = import_texture()
mat = build_material(tex)
unreal.log_warning('S47 MOON SAVED %s %s' % (tex.get_path_name(), mat.get_path_name()))
