"""ART-01: import finished document artwork as readable Texture2D assets.

Run as an Unreal Python commandlet (-run=pythonscript). Writes only the
destination in surfaces.json. S47_DOCPROPS_REPORT names a fresh report file.
The source JPGs stay unchanged; Unreal keeps them as texture source data.
"""
import hashlib
import json
import os
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
UNREAL_DIR = HERE.parents[2]
REPORT = Path(os.environ.get('S47_DOCPROPS_REPORT', str(Path(unreal.Paths.project_saved_dir()) / 'documentprops-import.json')))
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'textures': []}


def flush():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n')


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    flush()
    assert condition, (name, detail)


def plane_uv_layout():
    """Record the engine plane's UV corners so runtime UV rows are measured, not assumed."""
    mesh = E.load_asset('/Engine/BasicShapes/Plane')
    rows = []
    try:
        desc = mesh.get_static_mesh_description(0)
        for i in range(desc.get_vertex_instance_count()):
            vi = unreal.VertexInstanceID(i)
            v = desc.get_vertex_instance_vertex(vi)
            p = desc.get_vertex_position(v)
            uv = desc.get_vertex_instance_uv(vi, 0)
            rows.append({'pos_cm': [round(p.x, 3), round(p.y, 3), round(p.z, 3)], 'uv': [round(uv.x, 4), round(uv.y, 4)]})
    except Exception as error:  # informative only
        rows.append({'error': repr(error)})
    report['engine_plane_uv'] = rows


def main():
    spec = json.loads((HERE / 'surfaces.json').read_text())
    dest = spec['destination']
    check('destination is the owned documents folder', dest == '/Game/Signal47/Art/Documents', dest)
    for row in spec['surfaces']:
        src = UNREAL_DIR / row['source']
        check(row['asset'] + ' source exists', src.is_file(), str(src))
        task = unreal.AssetImportTask()
        task.filename = str(src)
        task.destination_path = dest
        task.destination_name = row['asset']
        task.replace_existing = True
        task.automated = True
        task.save = False
        T.import_asset_tasks([task])
        tex = E.load_asset(dest + '/' + row['asset'])
        check(row['asset'] + ' imported Texture2D', isinstance(tex, unreal.Texture2D))
        tex.set_editor_property('srgb', True)
        # Printed text: high-quality block compression and a full mip chain.
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_BC7)
        tex.set_editor_property('power_of_two_mode', unreal.TexturePowerOfTwoSetting.STRETCH_TO_SQUARE_POWER_OF_TWO)
        tex.set_editor_property('max_texture_size', 2048)
        tex.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP)
        tex.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_WORLD)
        tex.set_editor_property('filter', unreal.TextureFilter.TF_DEFAULT)
        check(row['asset'] + ' saved', E.save_loaded_asset(tex, False))
        report['textures'].append({'asset': tex.get_path_name(), 'object': row['object'], 'source': row['source'],
                                   'source_sha256': hashlib.sha256(src.read_bytes()).hexdigest(),
                                   'imported_size': [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]})
    plane_uv_layout()
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 DOCUMENT PROPS IMPORT PASS')


try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    flush()
    unreal.log_error('S47 DOCUMENT PROPS IMPORT FAIL')
    raise
