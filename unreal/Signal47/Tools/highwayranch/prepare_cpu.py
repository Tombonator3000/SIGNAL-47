"""Retain CPU data for the three exact original ranch-bearing legacy meshes.

Run only in the parent's guarded, isolated editor. No import, reduction, normals,
material or renderer policy changes. The runtime split supplies byte-level buffer
preservation checks; this preparer checks the saved structural metadata only.
"""
import hashlib
import json
from pathlib import Path

import unreal

EXPECTED = {910: (204, 590), 911: (3708, 589), 912: (360, 587)}
CONTENT = Path(unreal.Paths.project_content_dir())
MANIFEST = CONTENT / 'WebData' / 'manifest.json'


def signature(mesh):
    body = mesh.get_editor_property('body_setup')
    return {
        'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
        'materials': [
            {'slot': str(slot.material_slot_name),
             'imported_slot': str(slot.get_editor_property('imported_material_slot_name')),
             'material': slot.material_interface.get_path_name() if slot.material_interface else None}
            for slot in mesh.get_editor_property('static_materials')
        ],
        'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing')),
        'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled')),
        'collision_trace_flag': str(body.get_editor_property('collision_trace_flag')) if body else None,
    }


manifest = json.loads(MANIFEST.read_text())
assert manifest['handles']['world.legs.road.area.group']['node'] == 1240
prepared = []
# Check the complete set before any asset metadata is changed.
for geo, (triangles, material) in EXPECTED.items():
    nodes = [n for n in manifest['nodes'] if n.get('geo') == geo]
    assert len(nodes) == 1 and nodes[0]['p'] == 1289 and nodes[0]['mats'] == [material], (geo, nodes)
    assert manifest['nodes'][1289]['p'] == 1240
    asset = f'/Game/Web/Geo/geo_007/StaticMeshes/g{geo}.g{geo}'
    mesh = unreal.load_asset(asset)
    assert isinstance(mesh, unreal.StaticMesh), (asset, 'missing original imported mesh')
    before = signature(mesh)
    assert before['lod_triangles'] == [triangles], (asset, before)
    assert before['materials'] and before['support_ray_tracing'] and not before['nanite_enabled'], (asset, before)
    native_file = CONTENT / 'Web' / 'Geo' / 'geo_007' / 'StaticMeshes' / f'g{geo}.uasset'
    assert native_file.is_file(), native_file
    prepared.append((asset, mesh, before, native_file,
                     hashlib.sha256(native_file.read_bytes()).hexdigest(),
                     bool(mesh.get_editor_property('allow_cpu_access'))))

rows = []
for asset, mesh, before, native_file, before_sha, retained in prepared:
    mesh.set_editor_property('allow_cpu_access', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False), asset
    assert mesh.get_editor_property('allow_cpu_access')
    after = signature(mesh)
    assert before == after, (asset, before, after)
    rows.append({'asset': asset, 'before_cpu_access': retained, 'allow_cpu_access': True,
                 'before': before, 'after': after,
                 'native_before_sha256': before_sha,
                 'native_after_sha256': hashlib.sha256(native_file.read_bytes()).hexdigest()})

report = {
    'status': 'PASS_CPU_RETENTION_STRUCTURAL_SIGNATURE',
    'scope': 'Exactly g910/g911/g912 CPU retention; no reimport or geometry/material/RT/Nanite policy change. '
             'Buffer-byte preservation and physical route require separate runtime acceptance.',
    'manifest_sha256': hashlib.sha256(MANIFEST.read_bytes()).hexdigest(),
    'assets': rows,
}
out = Path(unreal.Paths.project_saved_dir()) / 'highway-ranch-cpu.json'
out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
unreal.log('S47_HIGHWAY_RANCH_CPU_PASS ' + json.dumps(report))
