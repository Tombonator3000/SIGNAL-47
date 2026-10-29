"""Import only the two original reference textures; no new document/interactions."""
from pathlib import Path
import hashlib
import json
import traceback
import unreal

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / 'ArtSource/Office'
SPEC = json.loads((SOURCE / 'research-art-bindings.json').read_text())
REPORT = Path(unreal.Paths.project_saved_dir()) / 'office-reference-import.json'
report = {'status': 'RUNNING', 'textures': []}


def flush():
    REPORT.write_text(json.dumps(report, indent=2) + '\n')


flush()
try:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for row in SPEC['textures']:
        path = SOURCE / row['source']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == row['sha256'], row['name']
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = SPEC['destination']
        task.destination_name = row['name']
        task.replace_existing = True
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
        texture = unreal.EditorAssetLibrary.load_asset(SPEC['destination'] + '/' + row['name'])
        assert isinstance(texture, unreal.Texture2D), row['name']
        texture.set_editor_property('srgb', True)
        texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        assert unreal.EditorAssetLibrary.save_loaded_asset(texture), row['name']
        assert [texture.blueprint_get_size_x(), texture.blueprint_get_size_y()] == row['size'], row['name']
        report['textures'].append({'asset': texture.get_path_name(), 'source_sha256': row['sha256'], 'size': row['size']})
        flush()
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 OFFICE REFERENCE IMPORT PASS')
except Exception:
    report['status'] = 'FAIL'
    report['error'] = traceback.format_exc()
    flush()
    raise
