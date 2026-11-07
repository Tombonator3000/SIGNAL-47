"""UnrealEditor-Cmd Python: materials for the old farmhouse's cube pieces (S47FarmHouse), /Game/FarmBuildings/Materials/M_Farm_World*.

A unit cube has no useful UV: stretched over a 12 m wall any map is a smear. These read the farm's own maps (textures.py) from world position, by the face's
axis, at the map's own size in metres, so a wall run, its parapet and the head over a window carry on as one surface. The stucco wears as an abandoned house's
does: mud splashed up from the ground (FootCm), rain streaks down from the top, plaster gone in patches down to the adobe. No new textures. Writes S47_T16_OUT/oldhouse.json.
"""
import json
import os
from pathlib import Path
import traceback

import unreal

E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
DEST = '/Game/FarmBuildings/Materials/'
TEX = '/Game/FarmBuildings/Textures/T_Farm_'
OWNER = 'S47FarmOldHouse_v1'
F1, F2, F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT1, unreal.CustomMaterialOutputType.CMOT_FLOAT2, unreal.CustomMaterialOutputType.CMOT_FLOAT3
# (map, its size in metres as textures.py saves it, linear tint, specular)
SETS = {'WorldStucco': ('Stucco', 3.0, (.86, .80, .72), .1), 'WorldWood': ('Wood', 3.0, (.62, .56, .5), .1), 'WorldPaint': ('Paint', 3.4, (.34, .72, .68), .15),
        'WorldRoof': ('Tin', 3.6, (.5, .46, .42), .1)}
STUCCO = """
float h = (World.z - Foot) * 0.01;
// mud from the ground, to about half a metre, in tongues
float splash = 1.0 - smoothstep(0.0, 0.62, h + (Tall - 0.5) * 0.45);
float3 c = C * lerp(float3(1.0, 1.0, 1.0), float3(0.60, 0.50, 0.41), splash);
// rain down from the top, in streaks
float rain = smoothstep(0.48, 0.74, Tall) * smoothstep(0.9, 2.5, h);
c *= lerp(float3(1.0, 1.0, 1.0), float3(0.70, 0.66, 0.62), rain);
// the plaster's own unevenness, as a hand trowelled it
c *= 0.86 + 0.28 * Wide;
// where it has come off: the adobe, darker and rougher, an edge that is short
float gone = smoothstep(0.66, 0.70, Patch * 0.8 + Wide * 0.2);
c = lerp(c, float3(0.20, 0.125, 0.08) * (0.7 + 0.6 * Tall), gone);
return c;
"""


def node(mat, cls, x, y, **p):
    n = L.create_material_expression(mat, cls, x, y)
    for k, v in p.items():
        n.set_editor_property(k, v)
    return n


def wire(a, out, b, pin):
    if not L.connect_material_expressions(a, out, b, pin):
        raise RuntimeError('No such pin: %s %r to %s %r' % (a.get_name(), out, b.get_name(), pin))


def custom(mat, code, inputs, output, x, y):
    n = node(mat, unreal.MaterialExpressionCustom, x, y, code=code, output_type=output)
    pins = []
    for name in inputs:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        pins.append(pin)
    n.set_editor_property('inputs', pins)
    for name, value in inputs.items():
        expression, out = value if isinstance(value, tuple) else (value, '')
        wire(expression, out, n, name)
    return n


def sample(mat, stem, role, uv, sampler, x, y):
    t = E.load_asset(TEX + stem + '_' + role)
    if t is None:
        raise RuntimeError('Missing ' + stem + role)
    s = node(mat, unreal.MaterialExpressionTextureSample, x, y, texture=t, sampler_type=sampler)
    wire(uv, '', s, 'UVs')
    return s


def build(name, stem, tile, tint, spec, report):
    path = DEST + 'M_Farm_' + name
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset('M_Farm_' + name, DEST[:-1], unreal.Material, unreal.MaterialFactoryNew())
    if E.get_metadata_tag(mat, 'S47Owner') not in (None, '', OWNER):
        raise RuntimeError('not owned: ' + path)
    L.delete_all_material_expressions(mat)
    wp = node(mat, unreal.MaterialExpressionWorldPosition, -1500, 0)
    vn = node(mat, unreal.MaterialExpressionVertexNormalWS, -1500, 100)
    # world metres along the face, the same however a piece stands
    face = custom(mat, 'float3 a = abs(N); float2 p = a.z >= max(a.x, a.y) ? W.xy : (a.x >= a.y ? W.yz : W.xz); return p * 0.01;', dict(W=wp, N=vn), F2, -1300, 0)
    uv = custom(mat, 'return P / %f;' % tile, dict(P=face), F2, -1100, 0)
    bc = sample(mat, stem, 'BC', uv, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -900, 0)
    nm = sample(mat, stem, 'N', uv, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -900, 200)
    rg = sample(mat, stem, 'R', uv, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, -900, 400)
    tint_n = node(mat, unreal.MaterialExpressionVectorParameter, -900, -200, parameter_name='Tint', default_value=unreal.LinearColor(tint[0], tint[1], tint[2], 1))
    base = custom(mat, 'return C.rgb * T.rgb;', dict(C=(bc, 'RGB'), T=(tint_n, 'RGB')), F3, -700, 0)
    rough = (rg, 'R')
    if stem == 'Stucco':
        noise = E.load_asset('/Game/FreeformTerrain/T_FF_Noise')
        if noise is None:
            raise RuntimeError('T_FF_Noise is needed')
        foot = node(mat, unreal.MaterialExpressionScalarParameter, -900, -300, parameter_name='FootCm', default_value=0.)
        linear = unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR

        def noise_at(code, y):
            s = node(mat, unreal.MaterialExpressionTextureSample, -900, y, texture=noise, sampler_type=linear)
            wire(custom(mat, code, dict(P=face), F2, -1100, y), '', s, 'UVs')
            return s
        tall = noise_at('return float2(P.x / 2.3, P.y / 19.0);', 600)     # streaks: narrow along the wall, long down it
        wide = noise_at('return P / 7.3;', 800)
        patch = noise_at('return float2(P.x * 0.81 + P.y * 0.59, P.y * 0.81 - P.x * 0.59) / 3.1;', 1000)
        base = custom(mat, STUCCO, dict(C=base, World=wp, Foot=foot, Tall=(tall, 'R'), Wide=(wide, 'R'), Patch=(patch, 'G')), F3, -500, 0)
    L.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
    L.connect_material_property(rough[0], rough[1], unreal.MaterialProperty.MP_ROUGHNESS)
    L.connect_material_property(nm, 'RGB', unreal.MaterialProperty.MP_NORMAL)
    L.connect_material_property(node(mat, unreal.MaterialExpressionConstant, -300, 300, r=spec), '', unreal.MaterialProperty.MP_SPECULAR)
    errors = [str(e) for e in (L.recompile_material(mat) or [])]
    report[name] = {'errors': errors}
    if errors:
        raise RuntimeError(name + ': ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('could not save ' + path)


def main():
    report = {}
    try:
        for name, (stem, tile, tint, spec) in SETS.items():
            build(name, stem, tile, tint, spec, report)
        report['status'] = 'PASS'
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
    Path(os.environ.get('S47_T16_OUT', '/tmp')).joinpath('oldhouse.json').write_text(json.dumps(report, indent=1))


main()
