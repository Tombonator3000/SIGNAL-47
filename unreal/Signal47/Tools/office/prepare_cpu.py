"""Behold CPU-data for seks eksisterende kontormesher, uten ny geometriimport."""
import json
from pathlib import Path

import unreal

EXPECTED = {341: 20, 342: 52, 346: 48, 349: 24, 351: 36, 352: 2}


def signature(mesh):
    return {
        'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
        'materials': [
            {'slot': str(slot.material_slot_name),
             'material': slot.material_interface.get_path_name() if slot.material_interface else None}
            for slot in mesh.get_editor_property('static_materials')
        ],
    }


meshes = []
# Valider hele settet før metadata lagres.
for geo, triangles in EXPECTED.items():
    path = f'/Game/Web/Geo/geo_002/StaticMeshes/g{geo}.g{geo}'
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), (path, 'original mesh missing')
    before = signature(mesh)
    assert before['lod_triangles'] == [triangles], (path, before)
    meshes.append((path, mesh, before))

rows = []
for path, mesh, before in meshes:
    retained = bool(mesh.get_editor_property('allow_cpu_access'))
    mesh.set_editor_property('allow_cpu_access', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False), path
    after = signature(mesh)
    assert after == before, (path, before, after)
    assert mesh.get_editor_property('allow_cpu_access'), path
    rows.append({'asset': path, 'before_cpu_access': retained, 'allow_cpu_access': True,
                 'before': before, 'after': after})

report = {'status': 'PASS', 'scope': 'CPU retention; original LOD triangles and material slots retained',
          'assets': rows}
(Path(unreal.Paths.project_saved_dir()) / 'office-native-cpu.json').write_text(
    json.dumps(report, indent=2) + '\n', encoding='utf-8')
unreal.log('S47_OFFICE_CPU_PASS ' + json.dumps(report))
