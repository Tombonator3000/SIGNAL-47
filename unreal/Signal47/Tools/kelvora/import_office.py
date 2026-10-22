"""UE Python: import only the office geometry with the old TV removed."""
import json
import os
from pathlib import Path
import unreal

src = Path(os.environ.get('S47_KELVORA_OFFICE', str(Path.home()/'scratch-models/kelvora-office')))
e = unreal.EditorAssetLibrary
t = unreal.AssetImportTask()
t.filename = str(src / 'OfficeWithoutTV.glb')
t.destination_path = '/Game/Kelvora/OfficeImport'
t.replace_existing = True
t.automated = True
t.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
report = json.loads((src / 'office-patches.json').read_text())
found = set()
for path in e.list_assets('/Game/Kelvora/OfficeImport', recursive=True):
    mesh = e.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        continue
    name = mesh.get_name()
    assert name in report, name
    # Runtime keeps the original component's materials. Never retain the temporary GLB material.
    mesh.set_material(0, e.load_asset('/Engine/EngineMaterials/DefaultMaterial'))
    mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    e.save_loaded_asset(mesh)
    assert e.rename_asset(path, '/Game/Kelvora/Office/' + name)
    found.add(name)
assert found == {n for n, r in report.items() if not r['hidden']}, found
e.delete_directory('/Game/Kelvora/OfficeImport')
unreal.log('KELVORA OFFICE IMPORT PASS ' + str(sorted(found)))
