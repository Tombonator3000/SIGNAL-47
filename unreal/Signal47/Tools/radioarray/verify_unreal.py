"""Read-only UE asset checks against the authored geometry contracts, not just import return codes."""
import json
import os
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parent
OUT = Path(os.environ.get('S47_RADIOARRAY_VERIFY', str(ROOT / 'unreal-verification.json')))
E = unreal.EditorAssetLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
checks = []


def check(name, condition, detail=None):
    checks.append({'name': name, 'pass': bool(condition), 'detail': detail})


for label, filename, key in [('dish', 'verification.json', 'meshes'), ('station', 'model_contract.json', 'parts')]:
    contract = json.loads((ROOT / label / filename).read_text())[key]
    for name, source in contract.items():
        if '_LOD' in name:
            continue
        asset = '/Game/RadioArray/' + label.title() + '/' + name
        mesh = E.load_asset(asset)
        check(name + ' exists', isinstance(mesh, unreal.StaticMesh))
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        bounds = source['bounds_local_m']
        expected_center = [(bounds[0][i] + bounds[1][i]) * 50 for i in range(3)]
        expected_center[1] *= -1
        expected_size = [(bounds[1][i] - bounds[0][i]) * 100 for i in range(3)]
        b = mesh.get_bounds()
        center = [b.origin.x, b.origin.y, b.origin.z]
        size = [2*b.box_extent.x, 2*b.box_extent.y, 2*b.box_extent.z]
        check(name + ' source scale, orientation and pivot',
              max(abs(a-b) for a, b in zip(center + size, expected_center + expected_size)) < .1,
              {'center_cm': center, 'size_cm': size, 'expected_center_cm': expected_center})
        check(name + ' collision', mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') ==
              unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        check(name + ' Nanite disabled', not mesh.get_editor_property('nanite_settings').enabled)
        slots = mesh.get_editor_property('static_materials')
        check(name + ' original materials', bool(slots) and all(slot.material_interface and
              slot.material_interface.get_name().startswith('M_Array_' if label == 'dish' else 'M_1420_') for slot in slots),
              [slot.material_interface.get_name() if slot.material_interface else None for slot in slots])
        if label=='station':
            actual_materials=[mesh.get_material(S.get_lod_material_slot(mesh,0,i)).get_name() for i in range(mesh.get_num_sections(0))]
            check(name+' source materials used by real sections',set(actual_materials)==set(source['materials']),{'expected':source['materials'],'actual':actual_materials})
        check(name + ' LOD count', S.get_lod_count(mesh) == (3 if label == 'dish' else 1))
        for lod in range(S.get_lod_count(mesh)):
            source_name = name + ('_LOD' + str(lod) if lod else '')
            triangles = mesh.get_num_triangles(lod)
            expected = contract[source_name]['triangles']
            check(name + ' LOD' + str(lod) + ' triangles',
                  triangles == expected,
                  {'source': expected, 'imported': triangles})

metadata = json.loads((Path(unreal.Paths.project_content_dir()) / 'RadioArray/Station/station-patches.json').read_text())
for name, patch in metadata['meshes'].items():
    if patch['hidden']:
        continue
    mesh = E.load_asset(patch['asset'])
    check(name + ' exact composed trim asset', isinstance(mesh, unreal.StaticMesh) and
          mesh.get_num_triangles(0) == patch['resultTris'],
          dict(patch, importedTris=mesh.get_num_triangles(0) if isinstance(mesh, unreal.StaticMesh) else None))

result = {'status': 'PASS' if all(row['pass'] for row in checks) else 'FAIL', 'checks': checks,
          'engine': unreal.SystemLibrary.get_engine_version(), 'passed': sum(row['pass'] for row in checks), 'total': len(checks)}
OUT.write_text(json.dumps(result, indent=2))
assert result['status'] == 'PASS', [row for row in checks if not row['pass']]
unreal.log('RADIOARRAY VERIFICATION PASS ' + str(result['passed']))
