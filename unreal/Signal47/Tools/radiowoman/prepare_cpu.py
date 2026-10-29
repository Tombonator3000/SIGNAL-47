"""HUM01: behold CPU-data for bare g1031 etter full preflight og privat bytebackup.

Kjøres av root i bevoktet editor. Ingen import eller endring av geometri/materialer.
PASS gjelder strukturell lagring; runtime må kontrollere faktiske pakkede buffere.
"""
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import struct


MANIFEST_SHA256 = '69fcf95db3adf8eff76274f59b03f94e662936a147f1ee7389630c1a8a0925c9'
GLB_SHA256 = '0e87d2acf6bbe9044304423d096ea75f7c731dd1af15c545b2d79691e5e2bfee'
ASSET = '/Game/Web/Geo/geo_008/StaticMeshes/g1031.g1031'
MATERIAL = '/Game/Web/Geo/geo_008/Materials/slot0.slot0'


def sha256(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def validate_manifest(manifest):
    nodes = manifest['nodes']
    assert manifest['handles']['world.town.woman']['node'] == 1480
    assert manifest['handles']['world.town.proxies.woman']['node'] == 1544
    assert nodes[1480]['p'] == nodes[1544]['p'] == 1431
    assert {n['i'] for n in nodes if n.get('p') == 1480} == set(range(1481, 1487))
    n = nodes[1484]
    assert n['i'] == 1484 and n['p'] == 1480 and n['geo'] == 1031 and n['mats'] == [641]
    assert n['m'] == [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    assert [n['i'] for n in nodes if n.get('geo') == 1031] == [1484]


def source_geometry(glb):
    """Read the frozen indexed source with the project's existing GLB parser.

    Source accessor rows/corners and native render vertices are distinct counts:
    exact P/N/UV row sharing needs 72 vertices for these unchanged 108 indices.
    This only measures the source; it never welds or changes a native asset.
    """
    assert sha256(glb) == GLB_SHA256, 'frozen original GLB differs'
    helper = runpy.run_path(str(Path(__file__).resolve().parents[1] / 'freemodels' / 'trim_web.py'))
    document, binary = helper['_load'](glb)
    meshes = [(i, mesh) for i, mesh in enumerate(document['meshes']) if mesh.get('name') == 'g1031']
    assert len(meshes) == 1 and len(meshes[0][1]['primitives']) == 1, 'exact original g1031 primitive'
    mesh_index, mesh = meshes[0]
    primitive = mesh['primitives'][0]
    assert primitive.get('mode', 4) == 4 and document['materials'][primitive['material']]['name'] == 'slot0'
    assert set(primitive['attributes']) == {'POSITION', 'NORMAL', 'TEXCOORD_0'}
    attributes = {}
    for name, kind in (('POSITION', 'VEC3'), ('NORMAL', 'VEC3'), ('TEXCOORD_0', 'VEC2')):
        accessor = document['accessors'][primitive['attributes'][name]]
        assert (accessor['count'], accessor['componentType'], accessor['type']) == (108, 5126, kind)
        attributes[name] = helper['_values'](document, binary, primitive['attributes'][name])
        assert len(attributes[name]) == 108
    index_accessor = document['accessors'][primitive['indices']]
    assert (index_accessor['count'], index_accessor['componentType'], index_accessor['type']) == (108, 5125, 'SCALAR')
    indices = [row[0] for row in helper['_values'](document, binary, primitive['indices'])]
    assert len(indices) == 108 and set(indices) == set(range(108))
    rows = [struct.pack('<8f', *attributes['POSITION'][i], *attributes['NORMAL'][i], *attributes['TEXCOORD_0'][i]) for i in range(108)]
    unique = {}
    remap = [unique.setdefault(row, len(unique)) for row in rows]
    assert len(unique) == 72, 'exact full-attribute native vertex count differs'
    compact_indices = [remap[index] for index in indices]
    assert all(len(set(compact_indices[i:i + 3])) == 3 for i in range(0, 108, 3)), 'source triangle degenerated'
    return {'mesh': 'g1031', 'mesh_index': mesh_index, 'source_attribute_rows': 108,
            'source_index_count': 108, 'triangles': 36, 'unique_full_attribute_rows': 72,
            'duplicate_full_attribute_rows': 36, 'full_attribute_row_format': 'exact little-endian float32 POSITION3/NORMAL3/TEXCOORD_0_2',
            'material': 'slot0', 'source_glb_sha256': GLB_SHA256}


def signature(mesh):
    body = mesh.get_editor_property('body_setup')
    return {
        'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
        'lod_vertices': [mesh.get_num_vertices(i) for i in range(mesh.get_num_lods())],
        'materials': [
            {'slot': str(slot.material_slot_name),
             'imported_slot': str(slot.get_editor_property('imported_material_slot_name')),
             'material': slot.material_interface.get_path_name() if slot.material_interface else None}
            for slot in mesh.get_editor_property('static_materials')
        ],
        'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing')),
        'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled')),
        'collision_trace_flag': str(body.get_editor_property('collision_trace_flag')) if body else None,
        'lightmap_coordinate_index': mesh.get_editor_property('light_map_coordinate_index'),
    }


def main():
    import unreal

    content = Path(unreal.Paths.project_content_dir()).resolve()
    project = Path(unreal.Paths.project_dir()).resolve()
    manifest_path = content / 'WebData' / 'manifest.json'
    glb = Path(os.environ.get('S47_RADIOWOMAN_SOURCE_GLB', '/home/tom/scratch-bm/Signal47-web/geo/geo_008.glb')).resolve()
    assert sha256(manifest_path) == MANIFEST_SHA256, 'frozen manifest differs'
    assert sha256(glb) == GLB_SHA256, 'frozen original GLB differs'
    geometry = source_geometry(glb)
    validate_manifest(json.loads(manifest_path.read_text(encoding='utf-8')))
    private = Path(os.environ.get('S47_RADIOWOMAN_PRIVATE_ROOT', '/home/tom/scratch-bm/Signal47-mannequins-20261029/private')).resolve()
    out = Path(os.environ.get('S47_RADIOWOMAN_CPU_DIR', str(private / 'cpu-r1'))).resolve()
    assert out.is_relative_to(private) and out != private and not out.is_relative_to(project), 'backup must be a fresh private directory outside project'
    assert not out.exists(), 'do not overwrite a previous backup or result; select a fresh S47_RADIOWOMAN_CPU_DIR'

    native = content / 'Web' / 'Geo' / 'geo_008' / 'StaticMeshes' / 'g1031.uasset'
    assert native.is_file() and not native.is_symlink(), 'expected local non-symlink native asset'
    files = sorted(native.parent.glob('g1031.*'))
    assert files and native in files
    assert all(p.is_file() and not p.is_symlink() and p.suffix in ('.uasset', '.uexp', '.ubulk', '.uptnl') and p.stat().st_nlink == 1 for p in files), 'unexpected or shared source sidecar'
    before_hashes = {p.name: sha256(p) for p in files}
    mesh = unreal.load_asset(ASSET)
    assert isinstance(mesh, unreal.StaticMesh) and mesh.get_path_name() == ASSET, 'exact original native mesh required'
    before = signature(mesh)
    assert before['lod_triangles'] == [36] and before['lod_vertices'] == [72], before
    assert before['materials'] == [{'slot': 'slot0', 'imported_slot': 'slot0', 'material': MATERIAL}], before
    assert before['support_ray_tracing'] and not before['nanite_enabled'], before
    retained = bool(mesh.get_editor_property('allow_cpu_access'))

    # No UObject mutation before all checks and independently hashed byte copies exist.
    out.mkdir(parents=True, exist_ok=False)
    backup = out / 'original'
    backup.mkdir()
    for p in files:
        shutil.copy2(p, backup / p.name)
    assert {p.name: sha256(backup / p.name) for p in files} == before_hashes
    assert {p.name: sha256(p) for p in files} == before_hashes, 'source changed during backup'
    initial = {'status': 'BACKED_UP_BEFORE_MUTATION', 'asset': ASSET,
               'manifest_sha256': MANIFEST_SHA256, 'source_glb_sha256': GLB_SHA256,
               'source_geometry': geometry,
               'native_before_sha256': before_hashes, 'backup_directory': str(backup),
               'before_cpu_access': retained, 'before': before}
    report_path = out / 'cpu-report.json'
    report_path.write_text(json.dumps(initial, indent=2) + '\n', encoding='utf-8')
    mesh.set_editor_property('allow_cpu_access', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False), ASSET
    assert mesh.get_editor_property('allow_cpu_access')
    after = signature(mesh)
    assert before == after, (before, after)
    after_files = sorted(native.parent.glob('g1031.*'))
    assert [p.name for p in files] == [p.name for p in after_files], 'sidecar inventory changed'
    assert {p.name: sha256(backup / p.name) for p in files} == before_hashes, 'backup changed'
    initial.update(status='PASS_CPU_RETENTION_STRUCTURAL_SIGNATURE', allow_cpu_access=True,
                   after=after, native_after_sha256={p.name: sha256(p) for p in after_files},
                   scope='Only g1031 CPU retention saved. Geometry/UV/normal/index-byte preservation, radio cut, pose, feet, interaction and disk/Continue require runtime acceptance.')
    report_path.write_text(json.dumps(initial, indent=2) + '\n', encoding='utf-8')
    unreal.log('S47_RADIO_WOMAN_CPU_PASS ' + str(report_path))


if __name__ == '__main__':
    main()
