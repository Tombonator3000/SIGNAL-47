"""Read actual saved Blender/FBX/GLB files in root's guarded DCC slot.

This proves source export integrity. It does not run Unreal, render a concept
view, establish terrain contact in the game, or approve a packaged candidate.
"""
import argparse
import itertools
import json
import math
import struct
import sys
from collections import defaultdict
from pathlib import Path

import bpy

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from design import CONTRACT, HOUSES, PACKAGE, PARTS, sha
from check_source import bounds, evaluate, triangles
from source_data import source_inputs, topology_digest, write_json
import home_geometry

TOL = .0001


class Report:
    def __init__(self, path):
        self.path = path
        self.data = dict(status='RUNNING', scope='Actual source files only; native/visual/packaged UNVERIFIED', checks=[])
        self.flush()

    def flush(self):
        write_json(self.path, self.data)

    def check(self, name, passed, **detail):
        self.data['checks'].append(dict(name=name, passed=bool(passed), **detail))
        if not passed:
            self.data['status'] = 'FAIL'
            self.flush()
            raise AssertionError(name, detail)


def empty():
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat)
    for image in list(bpy.data.images):
        bpy.data.images.remove(image)


def triangle_correspondence(obj, shape):
    """Full oriented triangle and corner UV match, insensitive to index order.

    A positive aggregate volume alone could hide one reversed primitive. Only
    cyclic vertex permutations are allowed here, so an inward face fails even
    if all points, total volume and bounds look plausible.
    """
    cells = defaultdict(list)
    expected = list(triangles(shape))
    for index, (ids, _, _, _, _) in enumerate(expected):
        center = [sum(shape.v[i][k] for i in ids) / 3 for k in range(3)]
        cells[tuple(math.floor(v / TOL) for v in center)].append(index)
    used, maximum_error = set(), 0.
    mesh = obj.data
    mesh.calc_loop_triangles()
    uv = mesh.uv_layers[0].data
    for actual in mesh.loop_triangles:
        points = [obj.matrix_world @ mesh.vertices[i].co for i in actual.vertices]
        texcoords = [uv[i].uv for i in actual.loops]
        cell = tuple(math.floor(sum(p[k] for p in points) / (3 * TOL)) for k in range(3))
        name = mesh.materials[actual.material_index].name
        matched = None
        for delta in itertools.product((-1, 0, 1), repeat=3):
            key = tuple(a + b for a, b in zip(cell, delta))
            for index in cells.get(key, []):
                if index in used:
                    continue
                ids, expected_uv, material, _, _ = expected[index]
                if material != name:
                    continue
                for shift in range(3):
                    error = max(abs(points[k][axis] - shape.v[ids[(k + shift) % 3]][axis])
                                for k in range(3) for axis in range(3))
                    uv_error = max(abs(texcoords[k][axis] - expected_uv[(k + shift) % 3][axis])
                                   for k in range(3) for axis in range(2))
                    if error < TOL and uv_error < TOL:
                        matched = index
                        maximum_error = max(maximum_error, error)
                        break
                if matched is not None:
                    break
            if matched is not None:
                break
        if matched is None:
            return False, dict(reason='Missing oriented source triangle/UV/material', triangle=actual.index,
                               material=name, points=[list(v) for v in points])
        used.add(matched)
    return len(used) == len(expected), dict(matched=len(used), expected=len(expected), maximum_error_m=maximum_error)


def load_glb(path):
    raw = path.read_bytes()
    assert raw[:4] == b'glTF' and struct.unpack_from('<I', raw, 4)[0] == 2
    length = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20 + length])
    return doc, raw[28 + length:]


def check_glb(path, rows, report):
    doc, binary = load_glb(path)
    points, count = [], 0
    for mesh in doc['meshes']:
        for primitive in mesh['primitives']:
            count += doc['accessors'][primitive['indices']]['count'] // 3
            accessor = doc['accessors'][primitive['attributes']['POSITION']]
            view = doc['bufferViews'][accessor['bufferView']]
            offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
            for i in range(accessor['count']):
                x, y, z = struct.unpack_from('<fff', binary, offset + i * view.get('byteStride', 12))
                points.append((x, -z, y))
            report.check(path.name + ' textured UV/normal primitive',
                         'TEXCOORD_0' in primitive['attributes'] and 'NORMAL' in primitive['attributes'])
    expected = bounds([p for row in rows for p in row['bounds_local_m']])
    error = max(abs(a - b) for v, w in zip(bounds(points), expected) for a, b in zip(v, w))
    report.check(path.name + ' exact source count/bounds',
                 count == sum(row['triangles'] for row in rows) and error < TOL,
                 triangles=count, maximum_error_m=error)


def verify(output, report):
    output = output.resolve()
    manifest_file = output / 'manifest.json'
    contract = json.loads(manifest_file.read_text())
    report.data['manifest_sha256'] = sha(manifest_file)
    report.check('single canonical manifest and identical compatibility copy',
                 manifest_file.read_bytes() == (output / 'model_contract.json').read_bytes())
    report.check('four houses, twelve parts, source policy',
                 set(contract['parts']) == set(PARTS) and set(contract['houses']) == set(HOUSES)
                 and contract['native_import_policy'] == CONTRACT['native_policy'] and contract['no_new_admission'])
    report.check('read-only authored code/Town dependencies/pixels unchanged', source_inputs() == contract['source_inputs_sha256'])
    report.check('material manifest binding', sha(output / 'materials.json') == contract['materials_manifest_sha256'])
    report.check('complete concrete generated-file inventory', len(contract['generated_sha256']) == 58)
    for name, expected in contract['generated_sha256'].items():
        path = output / name
        report.check(name + ' exact generated file', path.is_file() and sha(path) == expected)
    preflight, _ = evaluate()
    report.check('independent primitive/retained opening proof', preflight['status'] == 'PASS'
                 and preflight['lod_triangle_totals'] == contract['lod_triangle_totals'])
    for lod in range(3):
        shapes = home_geometry.build(lod)
        for name, shape in shapes.items():
            row = contract['lods'][str(lod)][name]
            empty()
            path = output / row['fbx']
            result = bpy.ops.import_scene.fbx(filepath=str(path), use_custom_normals=True)
            objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
            report.check(path.name + ' single actual exported mesh', result == {'FINISHED'} and len(objects) == 1)
            obj = objects[0]
            mesh = obj.data
            mesh.calc_loop_triangles()
            got = bounds([obj.matrix_world @ v.co for v in mesh.vertices])
            error = max(abs(a - b) for v, w in zip(got, row['bounds_local_m']) for a, b in zip(v, w))
            report.check(path.name + ' actual metric bounds/pivot/slots/count', error < TOL
                         and obj.location.length < 1e-5 and max(abs(v - 1) for v in obj.scale) < 1e-5
                         and len(mesh.loop_triangles) == row['triangles']
                         and [m.name for m in mesh.materials] == row['materials'], maximum_error_m=error)
            report.check(path.name + ' UV and valid actual normals', len(mesh.uv_layers) == 1
                         and min(t.area for t in mesh.loop_triangles) >= 5e-9
                         and all(p.normal.length > .99 for p in mesh.polygons)
                         and all(n.vector.length > .99 for n in mesh.corner_normals))
            matched, detail = triangle_correspondence(obj, shape)
            report.check(path.name + ' every oriented source triangle/material/corner UV', matched, **detail)
    empty()
    bpy.ops.import_scene.fbx(filepath=str(output / 'exports' / 'axis_probe.fbx'))
    objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    report.check('actual axis probe single mesh', len(objects) == 1)
    lo, hi = bounds([objects[0].matrix_world @ v.co for v in objects[0].data.vertices])
    got = [(a + b) / 2 for a, b in zip(lo, hi)] + [b - a for a, b in zip(lo, hi)]
    report.check('actual FBX asymmetric metre/axis probe', max(abs(a-b) for a, b in zip(got, [2, 4, 6, .2, .4, .6])) < 1e-5)
    for name, row in contract['parts'].items():
        check_glb(output / row['glb'], [row], report)
    materials = {r['name']: r for r in json.loads((output / 'materials.json').read_text())}
    for home, row in contract['houses'].items():
        own_rows = [contract['parts'][n] for n in row['parts']]
        check_glb(output / (home + '.glb'), own_rows, report)
        bpy.ops.wm.open_mainfile(filepath=str(output / row['blend']))
        report.check(home + ' saved source scene/metric units/editable collection',
                     row['scene'] in bpy.data.scenes and row['editable_collection'] in bpy.data.collections
                     and bpy.context.scene.unit_settings.system == 'METRIC'
                     and bpy.context.scene.unit_settings.scale_length == 1)
        report.check(home + ' exactly six saved original/source meshes',
                     {o.name for o in bpy.data.objects if o.type == 'MESH'} == set(row['parts']) | {'SRC_' + n for n in row['parts']})
        used = set()
        for name in row['parts']:
            obj, original = bpy.data.objects.get(name), bpy.data.objects.get('SRC_' + name)
            part = contract['parts'][name]
            report.check(name + ' saved identity mesh and editable copy', obj is not None and original is not None
                         and obj.data != original.data and len(obj.vertex_groups) > 0
                         and obj.location.length < 1e-8 and max(abs(v) for v in obj.rotation_euler) < 1e-8
                         and max(abs(v-1) for v in obj.scale) < 1e-8)
            report.check(name + ' saved topology/UV/material/smoothing unchanged',
                         topology_digest(obj.data) == part['source_mesh_sha256'] == topology_digest(original.data))
            report.check(name + ' saved world mapping and admission policy',
                         list(obj['site_world_three_m']) == part['site_world_three_m'] and obj['no_new_admission'])
            used.update(part['materials'])
        required = {(output / path).resolve() for name in used for path in materials[name].get('textures', {}).values()}
        packed = set()
        for image in bpy.data.images:
            if image.source == 'FILE':
                disk = Path(bpy.path.abspath(image.filepath)).resolve()
                report.check(home + ' ' + image.name + ' original packed pixels and portable path',
                             image.filepath.startswith('//') and disk.is_file() and image.packed_file is not None
                             and image.packed_file.data == disk.read_bytes())
                packed.add(disk)
        report.check(home + ' all used PBR packed', required == packed, required=len(required), actual=len(packed))
    report.check('all source/dependency hashes still unchanged after reopen', source_inputs() == contract['source_inputs_sha256'])
    report.data['status'] = 'PASS'
    report.data['lod_triangle_totals'] = contract['lod_triangle_totals']
    report.flush()
    print('CALICHEHOMES_SOURCE_PASS', len(report.data['checks']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=PACKAGE)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    report = Report(args.report or args.out / 'source-verification.json')
    try:
        verify(args.out, report)
    except Exception as exc:
        report.data.update(status='FAIL', exception=repr(exc))
        report.flush()
        raise
