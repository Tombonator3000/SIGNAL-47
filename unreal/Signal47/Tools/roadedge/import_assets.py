"""/Game/RoadEdge: Toms veikant, 7. november: «må få motorveien til å passe bedre inn, med støv/jord mot vei». UnrealEditor-Cmd Python.

M_S47_RoadEdge er bakkens eget materiale (kopi av M_S47_FreeformGround, grafen urørt) gjort om til en mesh-dekal: farge og ruhet fra bakken,
dekning fra veikantens regel under. Der dekken slipper bakken ute på jorda er fargen den samme som bakken under, så det finnes ingen kant å se;
inn over skulderen og asfalten er det samme jord som ligger der. Bakkens normal røres ikke (Normal er ikke koblet, så dekalen skriver den ikke).

Kjøres på nytt når bakkematerialet er endret, så kopien følger med. Eier bare /Game/RoadEdge. S47_RE_ROOT = mappa med T_RoadEdge_M.png,
S47_RE_REPORT = JSON-rapporten.
"""
import json
import os
from pathlib import Path
import traceback

import unreal

DEST = '/Game/RoadEdge'
NAME = 'M_S47_RoadEdge'
SOURCE = '/Game/FreeformTerrain/M_S47_FreeformGround'
TEXTURE = 'T_RoadEdge_M'
OWNER = 'S47RoadEdge_v1'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()

# Samme regel som edge_rule i make_textures.py. x og z er meter fra veiens midte, med fortegn på x: asfalten til 4,8 m, skulderen til 6, bakken etter det.
# Dekket: tynt og flekkvis inn på asfalten i lange tunger, tett men brutt over skulderen, så fullt til det fader ut på bakken der tungene sier.
EDGE_HLSL = r"""
float x = Uv.x, z = Uv.y, ax = abs(x);
float side = x > 0.0 ? 0.31 : 0.77;
float4 a = Texture2DSample(Map, MapSampler, float2(z / 256.0, x / 32.0));
float4 b = Texture2DSample(Map, MapSampler, float2(z / 83.0 + 0.37 + (a.r - 0.5) * 0.05, x / 21.0 + 0.11 + (a.g - 0.5) * 0.05));
float4 c = Texture2DSample(Map, MapSampler, float2(z / 1900.0 + side, 0.27));
float4 d = Texture2DSample(Map, MapSampler, float2(z / 5.3, x / 2.7));
float t = lerp(a.r, b.r, smoothstep(0.35, 0.65, c.g));
float width = 0.55 + 0.9 * c.r;
float p = saturate((a.g + b.g) * 0.8 - 0.3);

float fine = d.g;
float reach = (0.3 + 2.3 * pow(t, 2.2)) * width;
float dIn = ax - (4.8 - reach) + ((p - 0.5) * 0.8 + (fine - 0.5) * 0.35) * (0.25 + 0.3 * reach);
float inner = saturate(dIn / (0.12 + 0.4 * reach));
float onRoad = smoothstep(0.0, 1.0, inner) * lerp(0.25, 0.95, saturate(dIn / max(reach, 0.05))) * smoothstep(0.15, 0.6, p * 0.7 + fine * 0.3 + 0.35 * inner);
float flecks = smoothstep(0.7, 0.88, a.g * 0.5 + b.r * 0.5) * smoothstep(2.7, 4.2, ax) * smoothstep(0.35, 0.7, fine) * 0.4;
onRoad = max(onRoad, flecks);
float s = saturate((ax - 4.8) / 1.2);
float shoulder = 0.68 + 0.32 * smoothstep(0.2, 0.72, p + 0.45 * s);
float cover = lerp(onRoad, shoulder, smoothstep(4.55, 5.05, ax + (p - 0.5) * 0.5));
float far = 6.6 + (1.2 + 7.5 * t) * width;
float fadeOut = (1.0 - smoothstep(6.3, far, ax + (p - 0.5) * 1.6)) * (1.0 - smoothstep(14.6, 16.3, ax));
float opacity = cover * fadeOut;

float amount = saturate(1.0 - (ax - 6.0 + (p - 0.5) * 2.5) / (1.0 + 6.5 * t * width)) * smoothstep(0.15, 0.7, p + 0.25 * s);
float3 tone = lerp(float3(1.0, 1.0, 1.0), Dust, amount);
float speck = smoothstep(0.74, 0.86, d.a) * smoothstep(4.5, 5.4, ax) * (1.0 - smoothstep(6.5 + 3.0 * t, 8.5 + 4.0 * t, ax));
float ruts = a.b * smoothstep(0.35, 0.5, c.b);
float3 colour = Soil * Gain * tone * (1.0 - 0.24 * speck) * (1.0 - 0.3 * ruts);
opacity = max(opacity, ruts * 0.9 * smoothstep(4.2, 4.8, ax));
return float4(colour, opacity * Keep);
"""


def node(mat, cls, x, y, **props):
    n = M.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        n.set_editor_property(k, v)
    return n


def wire(a, out, b, pin):
    if not M.connect_material_expressions(a, out, b, pin):
        raise RuntimeError('No such pin: %s %r to %s %r' % (a.get_name(), out, b.get_name(), pin))


def import_texture(root, out):
    png = Path(root) / (TEXTURE + '.png')
    path = DEST + '/' + TEXTURE
    state = '%d:%d' % (png.stat().st_size, png.stat().st_mtime)
    tex = E.load_asset(path) if E.does_asset_exist(path) else None
    if tex is None or E.get_metadata_tag(tex, 'S47Source') != state:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', str(png))
        task.set_editor_property('destination_path', DEST)
        task.set_editor_property('destination_name', TEXTURE)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        T.import_asset_tasks([task])
        tex = E.load_asset(path)
        if tex is None:
            raise RuntimeError('Could not import ' + str(png))
        tex.set_editor_property('srgb', False)
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
        # Ingen lav mip der dekalen først vises: dekning fra et lavt nivå er et jevnt bånd.
        tex.set_editor_property('never_stream', True)
        E.set_metadata_tag(tex, 'S47Source', state)
        E.set_metadata_tag(tex, 'S47Owner', OWNER)
        if not E.save_loaded_asset(tex):
            raise RuntimeError('Could not save ' + path)
        out['texture'] = 'imported ' + state
    else:
        out['texture'] = 'kept'
    return tex


def build_material(tex, out):
    path = DEST + '/' + NAME
    if E.does_asset_exist(path):
        old = E.load_asset(path)
        if E.get_metadata_tag(old, 'S47Owner') != OWNER:
            raise RuntimeError('not owned: ' + path)
        if not E.delete_asset(path):
            raise RuntimeError('Could not replace ' + path)
    if not E.does_asset_exist(SOURCE):
        raise RuntimeError('Missing ' + SOURCE)
    mat = E.duplicate_asset(SOURCE, path)
    if mat is None:
        raise RuntimeError('Could not copy ' + SOURCE)
    # Hva bakken gir, fra koblingene selv: bakkeagenten kan bygge grafen om uten at dette må vite hvordan.
    attrs = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if attrs is None:
        raise RuntimeError('The ground no longer goes through Make Material Attributes')
    names = [str(n) for n in M.get_material_expression_input_names(attrs)]
    inputs = list(M.get_inputs_for_material_expression(mat, attrs))
    pins = dict(zip(names, inputs))
    out['ground_pins'] = {k: (v.get_name() if v else None) for k, v in pins.items() if v}
    soil, rough = pins.get('BaseColor'), pins.get('Roughness')
    if soil is None or rough is None:
        raise RuntimeError('The ground has no BaseColor or Roughness on its attributes: %s' % out['ground_pins'])
    M.delete_material_expression(mat, attrs)
    mat.set_editor_property('use_material_attributes', False)
    mat.set_editor_property('enable_tessellation', False)
    mat.set_editor_property('used_with_nanite', False)
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)

    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, 200, -500, coordinate_index=0)
    vc = node(mat, unreal.MaterialExpressionVertexColor, 200, -400)
    obj = node(mat, unreal.MaterialExpressionTextureObject, 200, -300, texture=tex, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    dust = node(mat, unreal.MaterialExpressionVectorParameter, 200, -200, parameter_name='Dust', default_value=unreal.LinearColor(1.13, 1.12, 1.1, 1))
    gain = node(mat, unreal.MaterialExpressionVectorParameter, 200, -100, parameter_name='Gain', default_value=unreal.LinearColor(1, 1, 1, 1))
    edge = node(mat, unreal.MaterialExpressionCustom, 500, -300)
    edge.set_editor_property('code', EDGE_HLSL)
    edge.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    pin_names = ('Uv', 'Keep', 'Map', 'Soil', 'Dust', 'Gain')
    custom_pins = []
    for name in pin_names:
        pin = unreal.CustomInput()
        pin.set_editor_property('input_name', name)
        custom_pins.append(pin)
    edge.set_editor_property('inputs', custom_pins)
    wire(uv, '', edge, 'Uv')
    wire(vc, 'A', edge, 'Keep')
    wire(obj, '', edge, 'Map')
    wire(soil, '', edge, 'Soil')
    wire(dust, '', edge, 'Dust')
    wire(gain, '', edge, 'Gain')
    rgb = node(mat, unreal.MaterialExpressionComponentMask, 750, -300, r=True, g=True, b=True, a=False)
    wire(edge, '', rgb, '')
    alpha = node(mat, unreal.MaterialExpressionComponentMask, 750, -200, r=False, g=False, b=False, a=True)
    wire(edge, '', alpha, '')
    if not M.connect_material_property(rgb, '', unreal.MaterialProperty.MP_BASE_COLOR):
        raise RuntimeError('base colour')
    if not M.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY):
        raise RuntimeError('opacity')
    if not M.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS):
        raise RuntimeError('roughness')
    errors = [str(e) for e in (M.recompile_material(mat) or [])]
    out['compile_errors'] = errors
    if errors:
        raise RuntimeError('Road edge material failed: ' + '; '.join(errors))
    E.set_metadata_tag(mat, 'S47Owner', OWNER)
    if not E.save_loaded_asset(mat):
        raise RuntimeError('Could not save ' + path)
    out['path'] = mat.get_path_name()
    out['domain'] = str(mat.get_editor_property('material_domain'))


def main():
    report = {'status': 'RUNNING'}
    path = os.environ.get('S47_RE_REPORT')
    try:
        if not E.does_directory_exist(DEST):
            E.make_directory(DEST)
        tex = import_texture(os.environ['S47_RE_ROOT'], report)
        build_material(tex, report)
        report['status'] = 'PASS_ROADEDGE'
    except Exception as error:
        report['status'] = 'FAIL'
        report['error'] = str(error)
        report['trace'] = traceback.format_exc()
    if path:
        Path(path).write_text(json.dumps(report, indent=1) + '\n')
    print(json.dumps(report, indent=1))


main()
