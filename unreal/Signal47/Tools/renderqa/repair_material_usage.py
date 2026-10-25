"""Targeted saved-asset repair/audit; no model or texture reimport.

S47_RENDERQA_MODE=apply enables missing usage on RadioArray/Dish and the Free
model manifest. Two exact absent, unplaced catalogue rows are reported separately
only while reference guards still prove that normal play does not request them.
Default audit is read-only. Reports every actual LOD section,
mesh/material binding and effective ISM/Nanite usage. Mesh bytes are untouched.
Run with UnrealEditor-Cmd -run=pythonscript -script=<this> -unattended -nullrhi.
Requires UE5.8. S47_RENDERQA_REPORT selects the JSON output.
"""
import json
import os
from pathlib import Path
import sys
import unreal

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from material_usage import ensure_mesh_material_usage, required_usage

E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
mode = os.environ.get('S47_RENDERQA_MODE', 'audit')
assert mode in ('audit', 'apply'), mode
out = Path(os.environ.get('S47_RENDERQA_REPORT', str(Path(__file__).with_name('material-usage.json'))))
paths = [f'/Game/RadioArray/Dish/SM_Array_{name}' for name in ('Base', 'Azimuth', 'Elevation')]
manifest = json.loads((content / 'Free/free_models.json').read_text())
paths.extend(row['asset'] for row in manifest['models'])
# These historical optional catalogue rows have no Content folder and no normal
# game placement. They are not the exported Room6 walkie or the placed cb_base.
# Keep the catalogue/source records; do not delete them to manufacture an audit PASS.
UNPLACED_CATALOGUE_MISSING = {
    '/Game/Free/Radio/vintage_radio_transceiver/StaticMeshes/vintage_radio_transceiver.vintage_radio_transceiver',
    '/Game/Free/Radio/walkie/StaticMeshes/walkie.walkie',
}
project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
prop_references = set()
def collect_models(value):
    if isinstance(value, dict):
        for key, item in value.items():
            if key == 'model' and isinstance(item, str):
                prop_references.add(item)
            elif key == 'models' and isinstance(item, list):
                prop_references.update(model for model in item if isinstance(model, str))
            collect_models(item)
    elif isinstance(value, list):
        for item in value:
            collect_models(item)
prop_files = sorted((project / 'Config/Props').glob('*.json'))
assert prop_files, 'Cannot prove placement scope without Config/Props'
for prop_file in prop_files:
    collect_models(json.loads(prop_file.read_text()))
source_files = [source for source in (project / 'Source/Signal47').rglob('*')
                if source.suffix in ('.cpp', '.h')]
assert source_files, 'Cannot prove hardcoded-asset scope without runtime sources'
source_text = {str(source.relative_to(project)): source.read_text() for source in source_files}
def unused_catalogue_proof(path):
    rows = [row for row in manifest['models'] if row['asset'] == path]
    assert len(rows) == 1, 'Unexpected duplicate catalogue row: ' + path
    aliases = {path, path.split('.', 1)[0], rows[0]['name'], path.rsplit('/', 1)[-1].split('.', 1)[0]}
    placements = sorted(prop_references.intersection(aliases))
    package = path.split('.', 1)[0]
    hardcoded = [source for source, text in source_text.items() if package in text]
    return {'asset': path, 'catalogue_name': rows[0]['name'],
            'reason': 'Absent optional catalogue asset; no normal prop placement or direct runtime asset reference. Explicit FreeModels=<name> diagnostic is not covered.',
            'prop_files_checked': len(prop_files), 'runtime_source_files_checked': len(source_files),
            'placement_references': placements, 'hardcoded_asset_references': hardcoded,
            'unused': not placements and not hardcoded}

report = {'engine': unreal.SystemLibrary.get_engine_version(), 'mode': mode,
          'scope': 'RadioArray/Dish and present Free manifest assets; actual LOD material sections. Two guarded absent optional catalogue rows reported separately.',
          'meshes': [], 'usage': [], 'errors': [], 'unused_catalogue_missing': []}
for path in sorted(set(paths)):
    if path in UNPLACED_CATALOGUE_MISSING and not E.does_asset_exist(path):
        proof = unused_catalogue_proof(path)
        report['unused_catalogue_missing'].append(proof)
        if proof['unused']:
            continue
        report['errors'].append('Previously unplaced catalogue asset is now referenced but missing: ' + path)
        continue
    mesh = E.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        report['errors'].append('Missing static mesh: ' + path)
        continue
    try:
        report['usage'].extend(ensure_mesh_material_usage(mesh, apply=mode == 'apply'))
        sections = []
        for lod in range(S.get_lod_count(mesh)):
            for section in range(mesh.get_num_sections(lod)):
                slot = S.get_lod_material_slot(mesh, lod, section)
                material = mesh.get_material(slot)
                uses = {str(flag): bool(material) and M.has_material_usage(material, flag)
                        for flag in required_usage(mesh)}
                sections.append({'lod': lod, 'section': section, 'material_slot': slot,
                                 'material': material.get_path_name() if material else None, 'usage': uses})
                if not material or not all(uses.values()):
                    report['errors'].append(f'{path}: LOD{lod} section{section} missing usage/binding')
        report['meshes'].append({'asset': mesh.get_path_name(), 'nanite': mesh.get_editor_property('nanite_settings').enabled,
                                'ray_tracing': mesh.get_editor_property('support_ray_tracing'),
                                'triangles': [mesh.get_num_triangles(lod) for lod in range(S.get_lod_count(mesh))],
                                'sections': sections})
    except Exception as error:
        report['errors'].append(path + ': ' + str(error))
report['status'] = 'PASS' if not report['errors'] else 'FAIL'
report['catalogue_status'] = 'INCOMPLETE_OPTIONAL_ASSETS' if report['unused_catalogue_missing'] else 'COMPLETE'
report['limits'] = 'Saved usage/bindings only. Fresh cook, runtime usage audit and image review are required.'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2) + '\n')
assert not report['errors'], report['errors']
unreal.log('S47 material usage ' + mode + ' PASS: ' + str(len(report['meshes'])) + ' meshes')
