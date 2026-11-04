"""EASTWING photo pass: import grime textures and build deferred-decal materials in Unreal.
Run inside a leased editor (run_decal_import.py). Writes only /Game/EastWing/Decals; owner tag
S47EastWingDecals_v1 guards against overwriting assets this importer does not own."""
import json
import os
from pathlib import Path
import traceback
import unreal

HERE = Path(__file__).resolve().parent
DEST = '/Game/EastWing/Decals'
OWNER = 'S47EastWingDecals_v1'
DECALS = ['T_EW_GrimeWallBase', 'T_EW_GrimeStreaks', 'T_EW_GrimeScuff', 'T_EW_FloorEdgeDust']
E = unreal.EditorAssetLibrary
L = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()


def own(path):
    if E.does_asset_exist(path):
        tag = E.get_metadata_tag(E.load_asset(path), 'S47Owner')
        if tag != OWNER:
            raise RuntimeError('Refusing to overwrite an asset not owned by this importer: ' + path)


def texture(name):
    own(DEST + '/' + name)
    task = unreal.AssetImportTask()
    task.filename = str(HERE / 'textures' / (name + '.png')); task.destination_path = DEST
    task.automated = True; task.replace_existing = True; task.save = True
    T.import_asset_tasks([task])
    tex = E.load_asset(DEST + '/' + name)
    tex.set_editor_property('srgb', True)
    tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
    E.set_metadata_tag(tex, 'S47Owner', OWNER); E.save_loaded_asset(tex)
    return tex


def decal_material(name, tex):
    path = DEST + '/M_' + name[2:]
    own(path)
    if E.does_asset_exist(path):
        E.delete_asset(path)
    mat = T.create_asset('M_' + name[2:], DEST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property('material_domain', unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    s = L.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 0)
    s.set_editor_property('texture', tex)
    tint = L.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 250)
    tint.set_editor_property('parameter_name', 'Tint'); tint.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    mul = L.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
    L.connect_material_expressions(s, 'RGB', mul, 'A'); L.connect_material_expressions(tint, '', mul, 'B')
    L.connect_material_property(mul, '', unreal.MaterialProperty.MP_BASE_COLOR)
    op = L.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 400)
    op.set_editor_property('parameter_name', 'Opacity'); op.set_editor_property('default_value', 1.0)
    amul = L.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 300)
    L.connect_material_expressions(s, 'A', amul, 'A'); L.connect_material_expressions(op, '', amul, 'B')
    L.connect_material_property(amul, '', unreal.MaterialProperty.MP_OPACITY)
    rough = L.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 500)
    rough.set_editor_property('r', .92)
    L.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
    L.recompile_material(mat)
    E.set_metadata_tag(mat, 'S47Owner', OWNER); E.save_loaded_asset(mat)
    return mat


def main():
    report_path = Path(os.environ['S47_EASTWING_DECAL_REPORT'])
    report = {'status': 'RUNNING', 'materials': []}
    try:
        for name in DECALS:
            tex = texture(name)
            mat = decal_material(name, tex)
            report['materials'].append(mat.get_path_name())
        report['status'] = 'PASS_NATIVE_DECAL_IMPORT'
    except Exception:
        report['status'] = 'FAIL'; report['error'] = traceback.format_exc()
    report_path.write_text(json.dumps(report, indent=2) + '\n')


if os.environ.get('S47_EASTWING_DECAL_APPLY') == '1':
    main()
else:
    print(json.dumps({'decals': DECALS, 'destination': DEST, 'native_import': 'NOT_RUN'}, indent=2))
