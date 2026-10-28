"""Run explicitly in Blender: --background --python convert_trims.py -- TRIM_DIR.

Converts immutable index-trimmed GLBs to FBX, avoiding Interchange's loss of
thin triangles. No modelling/decimation: counts, slot order and metric bounds
must survive. Does not run another process or touch the authored motel model.
"""
import hashlib
import importlib.util
import runpy
import json
import sys
from pathlib import Path
import bpy

TOOLS = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('original_glb', TOOLS / 'freemodels/trim_web.py')
glb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(glb)
fbx_nodes = runpy.run_path(str(TOOLS / 'calichenature/verify_unreal.py'))['fbx_nodes']


def values(doc, binary, accessor):
    result = glb._values(doc, binary, accessor)
    spec = doc['accessors'][accessor]
    if spec.get('normalized'):
        divisor = {5121: 255., 5123: 65535.}[spec['componentType']]
        result = [tuple(v / divisor for v in row) for row in result]
    return result


def make_mesh(doc, binary, source, row):
    """Build RNA arrays directly. GLTF import validates away repeated faces."""
    name = source['name']
    vertices, loops, slot_ids, normals, uv, colors = [], [], [], [], [], []
    for slot, primitive in enumerate(source['primitives']):
        assert primitive.get('mode', 4) == 4
        arrays = {key: values(doc, binary, accessor) for key, accessor in primitive['attributes'].items()}
        indices = [v[0] for v in values(doc, binary, primitive['indices'])]
        assert len(indices) % 3 == 0
        # Compact only unused vertices, never weld positions or duplicate faces.
        used = sorted(set(indices))
        mapping = {old: len(vertices) + i for i, old in enumerate(used)}
        vertices.extend((arrays['POSITION'][i][0], -arrays['POSITION'][i][2], arrays['POSITION'][i][1]) for i in used)
        loops.extend(mapping[i] for i in indices)
        slot_ids.extend([slot] * (len(indices) // 3))
        for i in indices:
            n = arrays['NORMAL'][i]
            normals.append((n[0], -n[2], n[1]))
            tex = arrays.get('TEXCOORD_0', [(0, 0)] * len(arrays['POSITION']))[i]
            uv.append((tex[0], 1 - tex[1]))  # same glTF-to-Blender V convention
            col = arrays.get('COLOR_0', [(1, 1, 1, 1)] * len(arrays['POSITION']))[i]
            colors.append(tuple(col[:3]) + (col[3] if len(col) == 4 else 1.,))
    assert len(loops) // 3 == row['result_tris']
    mesh = bpy.data.meshes.new(name)
    mesh.vertices.add(len(vertices))
    mesh.vertices.foreach_set('co', [v for p in vertices for v in p])
    mesh.loops.add(len(loops))
    mesh.loops.foreach_set('vertex_index', loops)
    mesh.polygons.add(len(slot_ids))
    mesh.polygons.foreach_set('loop_start', list(range(0, len(loops), 3)))
    mesh.polygons.foreach_set('loop_total', [3] * len(slot_ids))
    mesh.polygons.foreach_set('material_index', slot_ids)
    mesh.polygons.foreach_set('use_smooth', [True] * len(slot_ids))
    # No mesh.validate, BMesh or Edit-mode: the corridor's 3,815 repeated faces
    # are deliberate source topology. All faces are already triangles.
    mesh.update(calc_edges=True)
    layer = mesh.uv_layers.new(name='UVMap')
    layer.data.foreach_set('uv', [v for p in uv for v in p])
    color = mesh.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
    color.data.foreach_set('color', [v for p in colors for v in p])
    mesh.normals_split_custom_set(normals)
    for original_slot in row['original_material_indices']:
        mesh.materials.append(bpy.data.materials.new(f'M_MotelV2_Trim_{name}_slot{original_slot}'))
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    mesh.calc_loop_triangles()
    assert len(mesh.polygons) == len(mesh.loop_triangles) == row['result_tris'], (name, len(mesh.polygons), len(mesh.loop_triangles), row['result_tris'])
    return obj


def verify_fbx(path, expected):
    roots = fbx_nodes(path)
    objects = next(n for n in roots if n['name'] == 'Objects')
    geometry = [n for n in objects['children'] if n['name'] == 'Geometry' and n['properties'][-1] == 'Mesh']
    assert len(geometry) == 1
    indices = next(n for n in geometry[0]['children'] if n['name'] == 'PolygonVertexIndex')['properties'][0]
    assert len(indices) == expected * 3 and sum(v < 0 for v in indices) == expected, (path, 'actual exported FBX lost faces')


def bounds(points):
    return [[fn(p[a] for p in points) for a in range(3)] for fn in (min, max)]


def main(root):
    data = json.loads((root / 'trim-manifest.json').read_text())
    out = root / 'exports'
    out.mkdir(exist_ok=True)
    checks = []
    found = set()
    for filename in data['files']:
        source = root / filename
        assert hashlib.sha256(source.read_bytes()).hexdigest() == data['file_sha256'][filename]
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.context.scene.unit_settings.system = 'METRIC'
        bpy.context.scene.unit_settings.scale_length = 1
        document, binary = glb._load(source)
        for source_mesh in document['meshes']:
            name = source_mesh['name']
            assert name in data['meshes'] and name not in found, name
            row = data['meshes'][name]
            assert not row['hidden']
            obj = make_mesh(document, binary, source_mesh, row)
            actual = bounds([v.co for v in obj.data.vertices])
            error = max(abs(a - b) for u, v in zip(actual, row['bounds_local_m']) for a, b in zip(u, v))
            assert error < .00002, (name, 'GLB to Blender axis/scale', actual, row['bounds_local_m'])
            assert len(obj.data.materials) == len(row['original_material_indices']), name
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            path = out / (name + '.fbx')
            bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                                     axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
                                     apply_scale_options='FBX_SCALE_UNITS', use_mesh_modifiers=False,
                                     mesh_smooth_type='OFF', use_triangles=False, bake_anim=False,
                                     path_mode='AUTO')
            verify_fbx(path, row['result_tris'])
            found.add(name)
            checks.append({'name': name, 'triangles': len(obj.data.loop_triangles),
                           'bounds_local_m': actual, 'fbx': str(path.relative_to(root)),
                           'fbx_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    assert found == {name for name, row in data['meshes'].items() if not row['hidden']}
    (root / 'trim-fbx.json').write_text(json.dumps({'status': 'PASS', 'meshes': checks,
        'trim_manifest_sha256': hashlib.sha256((root / 'trim-manifest.json').read_bytes()).hexdigest()}, indent=2) + '\n')
    print('MOTEL_V2_TRIM_FBX_PASS', len(checks))


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:]
    assert len(args) == 1, 'Pass exactly one prepared trim directory after --'
    main(Path(args[0]).resolve())
