"""GAS-STATION-LIFT-01 (Tom, 11 November 2026: the shop like a real 1980s photograph): the look material the Caliche gas
station's own post-process volumes draw, /Game/Signal47/Look/M_StationLook (S47StationLook.cpp, Config/Props/gas_look.json).

The game's global volume replaces Unreal's tonemapper with M_WebTone, so no volume's colour grading, grain or vignette
settings draw; this material does the grade itself, at Scene Color After Tonemapping (the engine runs that location after
the replacing material; verified in the game pictures of 11 November, S/gas/lift/r6). One Custom HLSL node on the scene
colour, the viewport UV, the view size and time, with the grade's values as material parameters (the names S47StationLook sets):
  Saturation, Contrast (scalars, 1 = as is), Gamma, Gain (rgb), OffsetShadows (rgb, lifted blacks), GainHighlights (rgb, a
  soft shoulder), Grain (0..1, animated hash noise, 24 frames a second, subtle, more in the dark), GrainSize (pixels per
  grain), Vignette (0..1).
Runs headless like Tools/fx: bash Tools/stationlook/make_look.sh (through the queue). The material is ours and lives in Git.
"""
import json
import os
import traceback
from pathlib import Path

import unreal

E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
REPORT = Path(os.environ.get('S47_LOOK_REPORT', '/tmp/s47-look.json'))
FOLDER = '/Game/Signal47/Look'
NAME = 'M_StationLook'
PATH = FOLDER + '/' + NAME
report = {'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'steps': []}

HLSL = r'''
float3 c = saturate(Color.rgb);
c *= Gain.rgb;
c = pow(max(c, 0.0), 1.0 / max(Gamma.rgb, 0.05));
float lum = dot(c, float3(0.299, 0.587, 0.114));
// lifted blacks: the offset weighs most where it is dark
c += OffsetShadows.rgb * 3.0 * saturate(1.0 - lum * 1.6);
// a soft shoulder: the brightest part pulled down
float hi = smoothstep(0.55, 1.0, lum);
c = lerp(c, c * GainHighlights.rgb, hi);
c = lerp(float3(0.42, 0.42, 0.42), c, Contrast);
lum = dot(c, float3(0.299, 0.587, 0.114));
c = lerp(float3(lum, lum, lum), c, Saturation);
// the vignette: a wide oval, dark only towards the corners
float2 v = (UV - 0.5) * float2(1.0, 0.8);
c *= 1.0 - Vignette * smoothstep(0.3, 0.95, length(v) * 1.7);
// the grain: a hash per block of GrainSize pixels, a new one 24 times a second, more in the dark
float2 px = floor(UV * VS / max(GrainSize, 1.0));
float f = floor(T * 24.0);
float n = frac(sin(dot(px + f * 7.13, float2(12.9898, 78.233))) * 43758.5453);
float n2 = frac(sin(dot(px + f * 3.71 + 11.0, float2(26.651, 59.149))) * 23421.631);
float g = (n + n2 - 1.0) * 0.5;
c += g * Grain * 0.09 * (0.35 + 0.65 * (1.0 - lum));
return saturate(c);
'''


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    unreal.log(('S47LOOK PASS ' if condition else 'S47LOOK FAIL ') + name + ('' if detail is None else ' ' + str(detail)))
    return bool(condition)


def expression(mat, cls, x, y, **values):
    node = M.create_material_expression(mat, cls, x, y)
    for key, value in values.items():
        node.set_editor_property(key, value)
    return node


def make():
    if not E.does_directory_exist(FOLDER):
        E.make_directory(FOLDER)
    mat = unreal.load_asset(PATH) if E.does_asset_exist(PATH) else T.create_asset(NAME, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    M.delete_all_material_expressions(mat)
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    mat.set_editor_property('blendable_priority', 0)
    scene = expression(mat, unreal.MaterialExpressionSceneTexture, -900, -200, scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    uv = expression(mat, unreal.MaterialExpressionScreenPosition, -900, -60)
    tm = expression(mat, unreal.MaterialExpressionTime, -900, 20)
    vs = expression(mat, unreal.MaterialExpressionViewSize, -900, 80)
    scalars = {'Saturation': 1.0, 'Contrast': 1.0, 'Grain': 0.0, 'GrainSize': 1.0, 'Vignette': 0.0}
    vectors = {'Gamma': (1, 1, 1, 1), 'Gain': (1, 1, 1, 1), 'OffsetShadows': (0, 0, 0, 0), 'GainHighlights': (1, 1, 1, 1)}
    params = {}
    y = 120
    for name, v in scalars.items():
        params[name] = expression(mat, unreal.MaterialExpressionScalarParameter, -900, y, parameter_name=name, default_value=float(v))
        y += 90
    for name, v in vectors.items():
        params[name] = expression(mat, unreal.MaterialExpressionVectorParameter, -900, y, parameter_name=name, default_value=unreal.LinearColor(*v))
        y += 110
    order = ['Color', 'UV', 'T', 'VS'] + list(scalars) + list(vectors)
    inputs = []
    for n in order:   # (unreal.CustomInput takes no arguments in its constructor)
        ci = unreal.CustomInput()
        ci.set_editor_property('input_name', n)
        inputs.append(ci)
    custom = expression(mat, unreal.MaterialExpressionCustom, -400, 0, code=HLSL.strip(), output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                        description='S47 station look', inputs=inputs)
    check('scene colour connected', M.connect_material_expressions(scene, 'Color', custom, 'Color'))
    check('viewport uv connected', M.connect_material_expressions(uv, 'ViewportUV', custom, 'UV'))
    check('time connected', M.connect_material_expressions(tm, '', custom, 'T'))
    check('view size connected', M.connect_material_expressions(vs, '', custom, 'VS'))
    for name, node in params.items():
        check('parameter %s connected' % name, M.connect_material_expressions(node, '', custom, name))
    check('emissive connected', M.connect_material_property(custom, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR))
    M.recompile_material(mat)
    check('saved', E.save_loaded_asset(mat), PATH)
    back = unreal.load_asset(PATH)
    check('domain post process', back.get_editor_property('material_domain') == unreal.MaterialDomain.MD_POST_PROCESS)
    check('location after tonemapping', back.get_editor_property('blendable_location') == unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    names = sorted(str(n) for n in M.get_scalar_parameter_names(back)) + sorted(str(n) for n in M.get_vector_parameter_names(back))
    check('parameters', set(names) == set(order[4:]), names)
    report['asset'] = PATH + '.' + NAME


def main():
    try:
        make()
        report['status'] = 'PASS' if all(c['pass'] for c in report['checks']) else 'FAIL'
    except Exception as e:  # noqa: BLE001 - the report carries it
        report['status'] = 'ERROR'
        report['error'] = '%s\n%s' % (e, traceback.format_exc())
        unreal.log_error('S47LOOK ' + report['error'])
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=1))
    unreal.log('S47LOOK ' + report['status'] + ' ' + str(REPORT))


main()
