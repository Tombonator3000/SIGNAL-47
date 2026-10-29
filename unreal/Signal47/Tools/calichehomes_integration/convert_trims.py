"""Eksplisitt Blender-adapter: --background --python convert_trims.py -- TRIM_DIR.

Bare nye, tomme Mesh-datablokker. Ingen sveising, decimering eller validering
som kan fjerne opprinnelige dobbeltflater. Skriver bare under TRIM_DIR/exports.
Faktisk FBX leses tilbake før kvitteringen får PASS; ingen Unreal startes her.
"""
import importlib.util
import json
import runpy
import sys
from pathlib import Path

import bpy
sys.path.insert(0, str(Path(__file__).resolve().parent))
import schema as S

TOOLS = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('homes_original_glb', TOOLS / 'freemodels/trim_web.py')
GLB = importlib.util.module_from_spec(spec)
spec.loader.exec_module(GLB)
FBX = runpy.run_path(str(TOOLS / 'calichenature/verify_unreal.py'))


def values(document, binary, accessor):
    result = GLB._values(document, binary, accessor)
    row = document['accessors'][accessor]
    if row.get('normalized'):
        divisor = {5121: 255., 5123: 65535.}[row['componentType']]
        result = [tuple(v / divisor for v in item) for item in result]
    return result


def make_mesh(document, binary, source, row):
    positions, faces, slots, normals, uv, colors = [], [], [], [], [], []
    for slot, primitive in enumerate(source['primitives']):
        assert primitive.get('mode', 4) == 4
        arrays = {key: values(document, binary, accessor) for key, accessor in primitive['attributes'].items()}
        indices = [i[0] for i in values(document, binary, primitive['indices'])]
        assert len(indices) % 3 == 0 and 'NORMAL' in arrays and 'TEXCOORD_0' in arrays
        used = sorted(set(indices))
        source_colors = arrays.get('COLOR_0', [(1, 1, 1, 1)] * len(arrays['POSITION']))
        mapping = {old: len(positions) + i for i, old in enumerate(used)}
        positions.extend((arrays['POSITION'][i][0], -arrays['POSITION'][i][2], arrays['POSITION'][i][1]) for i in used)
        faces.extend(tuple(mapping[i] for i in indices[k:k + 3]) for k in range(0, len(indices), 3))
        slots.extend([slot] * (len(indices) // 3))
        for i in indices:
            n, tex = arrays['NORMAL'][i], arrays['TEXCOORD_0'][i]
            normals.append((n[0], -n[2], n[1]))
            uv.append((tex[0], 1 - tex[1]))
            col = source_colors[i]
            colors.append(tuple(col[:3]) + (col[3] if len(col) == 4 else 1.,))
    assert len(faces) == row['result_tris']
    mesh = bpy.data.meshes.new(source['name'])
    mesh.from_pydata(positions, [], faces, shade_flat=False)
    assert [tuple(p.vertices) for p in mesh.polygons] == faces, 'topology/order changed'
    mesh.polygons.foreach_set('material_index', slots)
    for name in row['materials']:
        mesh.materials.append(bpy.data.materials.new(name))
    mesh.uv_layers.new(name='UVMap').data.foreach_set('uv', [v for pair in uv for v in pair])
    mesh.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER').data.foreach_set('color', [v for col in colors for v in col])
    # Blender 5.2s split-setter kvantiserer/utjevner også gyldige kildenormaler.
    # CORNER/FLOAT_VECTOR custom_normal lagrer disse retningene direkte.
    # FBX-eksportøren leser mesh.corner_normals.
    custom = mesh.attributes.new(name='custom_normal', type='FLOAT_VECTOR', domain='CORNER')
    custom.data.foreach_set('vector', [v for normal in normals for v in normal])
    mesh.update()
    actual = [tuple(n.vector) for n in mesh.corner_normals]
    assert mesh.has_custom_normals and len(actual) == len(normals)
    assert max(abs(a - b) for got, want in zip(actual, normals) for a, b in zip(got, want)) < 2e-6
    mesh.calc_loop_triangles()
    assert len(mesh.polygons) == len(mesh.loop_triangles) == row['result_tris']
    obj = bpy.data.objects.new(source['name'], mesh)
    bpy.context.collection.objects.link(obj)
    return obj, uv, normals, colors, slots


def verify_fbx(path, row, expected_uv, expected_normals, expected_colors, expected_slots):
    objects = next(n for n in FBX['fbx_nodes'](path) if n['name'] == 'Objects')
    geometries = [n for n in objects['children'] if n['name'] == 'Geometry' and n['properties'][-1] == 'Mesh']
    assert len(geometries) == 1
    geometry = geometries[0]
    def child(item, key):
        return next(n for n in item['children'] if n['name'] == key)
    def prop(item, key):
        return child(item, key)['properties'][0]
    polygon = prop(geometry, 'PolygonVertexIndex')
    assert len(polygon) == row['result_tris'] * 3 and sum(v < 0 for v in polygon) == row['result_tris']
    actual_uv, _ = FBX['corner_uvs'](path)
    assert len(actual_uv) == len(expected_uv)
    assert max(abs(a - b) for got, want in zip(actual_uv, expected_uv) for a, b in zip(got, want)) < 2e-6, 'exported atlas UV changed'
    def corners(layer_name, values_name, index_name, width, expected):
        layer = child(geometry, layer_name)
        assert prop(layer, 'MappingInformationType') == 'ByPolygonVertex'
        direct = prop(layer, values_name)
        assert len(direct) % width == 0
        direct = [tuple(direct[i:i + width]) for i in range(0, len(direct), width)]
        reference = prop(layer, 'ReferenceInformationType')
        if reference == 'IndexToDirect':
            indices = prop(layer, index_name)
            assert len(indices) == len(expected) and all(0 <= i < len(direct) for i in indices)
            actual = [direct[i] for i in indices]
        else:
            assert reference == 'Direct', ('unsupported FBX reference', reference)
            actual = direct
        assert len(actual) == len(expected)
        assert max(abs(a - b) for got, want in zip(actual, expected) for a, b in zip(got, want)) < 2e-6, ('exported corner values changed', layer_name)
    # Blender may deduplicate attributes as IndexToDirect. Decode the actual
    # per-corner values; storage representation does not relax the value check.
    corners('LayerElementNormal', 'Normals', 'NormalsIndex', 3, expected_normals)
    corners('LayerElementColor', 'Colors', 'ColorIndex', 4, expected_colors)
    material_layer = child(geometry, 'LayerElementMaterial')
    mapping = prop(material_layer, 'MappingInformationType')
    actual_slots = prop(material_layer, 'Materials')
    if mapping == 'AllSame':
        actual_slots = actual_slots * len(expected_slots)
    else:
        assert mapping == 'ByPolygon'
    assert actual_slots == expected_slots, 'exported polygon material ordinals changed'
    # Eksportøren lager materiallenker i mesh.materials-ordning. Kontroller
    # faktiske navn og polygonenes ordinaler, også de tre plassene på hvert skilt.
    names = [n['properties'][1].split('\x00')[0] for n in objects['children'] if n['name'] == 'Material']
    assert names == row['materials'], ('exported material slot order', names, row['materials'])


def main(root):
    manifest_path = root / 'trim-manifest.json'
    manifest_sha256 = S.sha(manifest_path)
    data = json.loads(manifest_path.read_text())
    assert data['version'] == S.VERSION and data['source_revision'] == S.REVISION
    output = root / 'exports'
    output.mkdir(exist_ok=True)
    rows = {name: row for name, row in data['meshes'].items() if not row['hidden']}
    rows.update(data['signs'])
    assert len(rows) == 7
    found, receipt = set(), []
    for filename in data['files']:
        assert S.sha(root / filename) == data['file_sha256'][filename]
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.context.scene.unit_settings.system = 'METRIC'
        bpy.context.scene.unit_settings.scale_length = 1
        document, binary = GLB._load(root / filename)
        for source in document['meshes']:
            name = source['name']
            assert name in rows and name not in found
            row = rows[name]
            obj, uv, normals, colors, slots = make_mesh(document, binary, source, row)
            actual_bounds = S.bounds([v.co for v in obj.data.vertices])
            assert max(abs(a - b) for got, want in zip(actual_bounds, row['bounds_local_m']) for a, b in zip(got, want)) < .00002, 'Three to Blender axes/scale'
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            bpy.context.view_layer.objects.active = obj
            path = output / (name + '.fbx')
            bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                                     axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
                                     apply_scale_options='FBX_SCALE_UNITS', use_mesh_modifiers=False,
                                     mesh_smooth_type='OFF', use_triangles=False, bake_anim=False, path_mode='AUTO',
                                     colors_type='LINEAR', prioritize_active_color=True)
            verify_fbx(path, row, uv, normals, colors, slots)
            receipt.append({'name': name, 'triangles': row['result_tris'], 'bounds_local_m': actual_bounds,
                            'bounds_blender_m': actual_bounds, 'materials': list(row['materials']),
                            'fbx_material_names': [m.name for m in obj.data.materials],
                            'fbx': str(path.relative_to(root)), 'fbx_sha256': S.sha(path)})
            found.add(name)
    assert found == set(rows)
    assert manifest_sha256 == S.sha(manifest_path), 'prepared source changed during conversion'
    result = {'status': 'PASS', 'kind': 'source-trim-fbx-conversion', 'meshes': receipt,
              'trim_manifest_sha256': S.sha(manifest_path),
              'coordinates': 'bounds_local_m/bounds_blender_m are Blender metres. Native UE=(100x,-100y,100z).',
              'checks': ['exact triangle topology, material ordinal/name, metric bounds, actual FBX UV, source normals and corner colors']}
    (root / 'trim-fbx.json').write_text(json.dumps(result, indent=2) + '\n')
    print('CALICHE_HOMES_TRIM_FBX_PASS', len(receipt))


if __name__ == '__main__':
    arguments = sys.argv[sys.argv.index('--') + 1:]
    assert len(arguments) == 1, 'Oppgi én trimmappe etter --'
    main(Path(arguments[0]).resolve())
