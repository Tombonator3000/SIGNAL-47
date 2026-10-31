"""Narrow Pure Sky import. Never regenerate clouds, stars, horizon or night material.

Run in UnrealEditor-Cmd with -run=pythonscript -script=this-file -nullrhi.
Sources are downloaded using fetch.py. Only /Game/Sky/PureSky is written.
"""
from pathlib import Path
import hashlib
import json
import os
import sys
import unreal

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from material_code import day_code

P = ROOT.parents[2]
E, M = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
DEST = '/Game/Sky/PureSky'
NIGHT = '/Game/CalicheStreet/Atmosphere/M_Caliche_Sky'
MANIFEST = json.loads((ROOT / 'source.json').read_text())
SOURCE = Path(os.environ.get('S47_PURESKY_SOURCE', str(Path.home() / MANIFEST['local_source_relative'])))
REPORT = Path(os.environ.get('S47_PURESKY_REPORT', str(P / 'Saved/puresky-import.json')))


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


protected = [*sorted((P / 'Content/CalicheStreet/Atmosphere').glob('*.uasset')),
             *sorted((P / 'Content/Sky').glob('*.uasset'))]
before = {str(p.relative_to(P)): sha(p) for p in protected}
assert SOURCE.is_file() and sha(SOURCE) == MANIFEST['sha256'], 'Missing or changed CC0 source'
assert E.does_asset_exist(NIGHT), 'Original night material is required'
original = E.load_asset(NIGHT)
original_customs = [n for n in M.get_material_expressions(original)
                    if isinstance(n, unreal.MaterialExpressionCustom) and 'S47SkyFinite' in n.get_editor_property('code')]
assert len(original_customs) == 1
original_code = original_customs[0].get_editor_property('code')
code = day_code(original_code)

texture_path = DEST + '/T_S47_DaySky'
if not E.does_asset_exist(texture_path):
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE)
    task.destination_path = DEST
    task.destination_name = 'T_S47_DaySky'
    task.automated = True
    task.replace_existing = False
    task.save = True
    task.factory = unreal.TextureFactory()
    T.import_asset_tasks([task])
texture = E.load_asset(texture_path)
assert isinstance(texture, unreal.Texture2D), 'Pure Sky must be an equirectangular Texture2D'
import_source = Path(texture.get_editor_property('asset_import_data').get_first_filename()).resolve()
assert import_source == SOURCE.resolve(), 'Existing daytime texture uses another source; inspect before replacing'
texture.set_editor_property('srgb', False)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_HDR_COMPRESSED)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
texture.set_editor_property('max_texture_size', 8192)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
texture.set_editor_property('address_x', unreal.TextureAddress.TA_WRAP)
texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property('never_stream', True)
E.save_loaded_asset(texture)

material_path = DEST + '/M_S47_PureSky'
mat = E.load_asset(material_path) if E.does_asset_exist(material_path) else E.duplicate_asset(NIGHT, material_path)
assert mat and mat != original
expressions = list(M.get_material_expressions(mat))
customs = [n for n in expressions if isinstance(n, unreal.MaterialExpressionCustom)
           and 'S47SkyFinite' in n.get_editor_property('code')]
assert len(customs) == 1
custom = customs[0]
custom.set_editor_property('code', code)


def parameter(cls, name, **properties):
    nodes = [n for n in M.get_material_expressions(mat) if isinstance(n, cls)
             and str(n.get_editor_property('parameter_name')) == name]
    assert len(nodes) <= 1
    node = nodes[0] if nodes else M.create_material_expression(mat, cls)
    node.set_editor_property('parameter_name', name)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


params = {
    'SunsetStrength': parameter(unreal.MaterialExpressionScalarParameter, 'SunsetStrength', default_value=0.0),
    'PureSkyTexture': parameter(unreal.MaterialExpressionTextureObjectParameter, 'PureSkyTexture', texture=texture),
    'PureSkyWeight': parameter(unreal.MaterialExpressionScalarParameter, 'PureSkyWeight', default_value=0.0),
    'PureSkyYaw': parameter(unreal.MaterialExpressionScalarParameter, 'PureSkyYaw', default_value=MANIFEST['yaw_radians']),
    'PureSkyGain': parameter(unreal.MaterialExpressionScalarParameter, 'PureSkyGain', default_value=MANIFEST['radiance_gain']),
    'PureSkySunUV': parameter(unreal.MaterialExpressionVectorParameter, 'PureSkySunUV',
                              default_value=unreal.LinearColor(*MANIFEST['sun_uv'], 0.0, 0.0)),
}
inputs = list(custom.get_editor_property('inputs'))
names = {str(i.get_editor_property('input_name')) for i in inputs}
for name in params:
    if name not in names:
        entry = unreal.CustomInput()
        entry.set_editor_property('input_name', name)
        inputs.append(entry)
custom.set_editor_property('inputs', inputs)
for name, node in params.items():
    assert M.connect_material_expressions(node, '', custom, name), name
errors = list(M.recompile_material(mat))
assert not errors, errors
E.save_loaded_asset(mat)
after = {str(p.relative_to(P)): sha(p) for p in protected}
assert before == after, 'Protected night/cloud assets changed'
REPORT.parent.mkdir(parents=True, exist_ok=True)
REPORT.write_text(json.dumps({'status': 'PASS', 'source': str(SOURCE), 'source_sha256': sha(SOURCE),
    'texture': texture_path, 'material': material_path, 'texture_size': [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()],
    'protected_before': before, 'protected_after': after, 'material_code_sha256': hashlib.sha256(code.encode()).hexdigest()}, indent=2) + '\n')
unreal.log_warning('S47 PURE SKY IMPORT PASS ' + str(REPORT))
