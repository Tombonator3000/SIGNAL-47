"""Read the saved cow family in a fresh Unreal process; no asset mutations."""
import hashlib
import json
import os
from pathlib import Path
import traceback
import unreal

WORK = Path(os.environ.get('S47_COW_WORK', str(Path.home() / 'scratch-bm/Signal47-cows-20261025')))
IMPORT = Path(os.environ.get('S47_COW_REPORT', str(WORK / 'native-import.json')))
REPORT = Path(os.environ.get('S47_COW_VERIFY_REPORT', str(WORK / 'native-verify.json')))
REPORT.parent.mkdir(parents=True, exist_ok=True)
result = {'status': 'RUNNING', 'checks': [], 'meshes': []}
E = unreal.EditorAssetLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)


def check(name, ok, detail=None):
    result['checks'].append({'name': name, 'pass': bool(ok), 'detail': detail})
    REPORT.write_text(json.dumps(result, indent=2) + '\n')
    assert ok, (name, detail)


try:
    imported = json.loads(IMPORT.read_text())
    check('import completed with passing checks', imported['status'] == 'PASS' and all(row['pass'] for row in imported['checks']))
    result['import_report_sha256'] = hashlib.sha256(IMPORT.read_bytes()).hexdigest()
    source = Path(imported['source'])
    for name, expected in imported['source_sha256'].items():
        path = source / name
        check('source hash ' + name, path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == expected)
    for row in imported['meshes']:
        name = row['name']
        mesh = E.load_asset(row['asset'])
        check(name + ' saved StaticMesh', isinstance(mesh, unreal.StaticMesh))
        count = S.get_lod_count(mesh)
        triangles = [mesh.get_num_triangles(lod) for lod in range(count)]
        check(name + ' saved three LOD topology', count == 3 and triangles == row['triangles'], triangles)
        materials = list(mesh.get_editor_property('static_materials'))
        check(name + ' saved material count', len(materials) == len(row['materials']))
        for index, slot in enumerate(materials):
            material = slot.material_interface
            check(name + ' authored material slot ' + str(index), material is not None and material.get_name() == row['materials'][index],
                  material.get_path_name() if material else None)
            check(name + ' instancing usage ' + str(index), material.get_editor_property('used_with_instanced_static_meshes'))
        bounds = mesh.get_bounds()
        values = [bounds.origin.x, bounds.origin.y, bounds.origin.z, bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
        check(name + ' saved bounds', max(abs(a-b) for a,b in zip(values, row['center_cm'] + row['size_cm'])) < .01, values)
        check(name + ' conventional LOD rendering', not mesh.get_editor_property('nanite_settings').enabled)
        check(name + ' solid animal supports hardware RT geometry', mesh.get_editor_property('support_ray_tracing'))
        result['meshes'].append({'name': name, 'asset': row['asset'], 'triangles': triangles, 'materials': row['materials'], 'bounds_cm': values})
    check('complete live body/head and abducted family', len(result['meshes']) == 3)
    result['status'] = 'PASS'
    REPORT.write_text(json.dumps(result, indent=2) + '\n')
    unreal.log('S47 COWS SAVED ASSET VERIFY PASS')
except Exception:
    result['status'] = 'FAIL'
    result['error'] = traceback.format_exc()
    REPORT.write_text(json.dumps(result, indent=2) + '\n')
    raise
