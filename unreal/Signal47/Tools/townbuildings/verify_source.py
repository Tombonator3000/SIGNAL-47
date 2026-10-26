"""Read actual exported files and saved editable scene, without Unreal or rendering."""
import bpy
import json
import math
import struct
from pathlib import Path
from mathutils import Vector

R = Path(__file__).resolve().parent
contract = json.loads((R / 'model_contract.json').read_text())
checks = []


def check(name, ok, **detail):
    checks.append(dict(name=name, passed=bool(ok), **detail))
    if not ok:
        finish('FAIL')
        raise AssertionError(name, detail)


def finish(status):
    (R / 'verification.json').write_text(json.dumps({
        'status': status, 'checks': checks, 'source_revision': contract['source_revision'],
        'scope': 'Actual FBX, GLB, source scene and packed original pixels. Native and visual acceptance are separate.'
    }, indent=2) + '\n')


def empty():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat)


def bounds(points):
    return [[fn(p[k] for p in points) for k in range(3)] for fn in (min, max)]


for lod in range(3):
    for name, row in contract['lods'][str(lod)].items():
        empty()
        suffix = '' if lod == 0 else '_LOD' + str(lod)
        bpy.ops.import_scene.fbx(filepath=str(R / 'exports' / (name + suffix + '.fbx')), use_custom_normals=True)
        objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        check(name + suffix + ' single exported chunk', len(objects) == 1)
        ob = objects[0]
        me = ob.data
        me.calc_loop_triangles()
        actual = bounds([ob.matrix_world @ v.co for v in me.vertices])
        error = max(abs(x-y) for a, b in zip(actual, row['bounds_local_m']) for x, y in zip(a, b))
        slots = [m.name for m in me.materials]
        check(name + suffix + ' metric pivot, triangles and slots',
              error < .0001 and ob.location.length < 1e-5 and
              max(abs(v-1) for v in ob.scale) < 1e-5 and
              len(me.loop_triangles) == row['triangles'] and slots == row['materials'],
              maximum_error_m=error, triangles=len(me.loop_triangles), materials=slots)
        check(name + suffix + ' valid geometry and normals',
              min(t.area for t in me.loop_triangles) >= 5e-9 and
              all(p.normal.length > .99 for p in me.polygons))
        check(name + suffix + ' UV layer', len(me.uv_layers) == 1)
        uv = me.uv_layers[0].data
        minimum = math.inf
        for t in me.loop_triangles:
            a, b, c = [uv[i].uv for i in t.loops]
            minimum = min(minimum, abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
        check(name + suffix + ' finite noncollapsed UVs', minimum > 1e-12 and
              all(math.isfinite(x) for v in uv for x in v.uv), minimum_uv_area=minimum)

for name, row in contract['parts'].items():
    raw = (R / 'exports' / (name + '.glb')).read_bytes()
    n = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20+n])
    binary = raw[28+n:]
    masked = {r['name'] for r in json.loads((R / 'materials.json').read_text())
              if r.get('opacity_from_base_color') and r.get('alpha_mode') in ('masked', 'dithered_masked')}
    for material in doc.get('materials', []):
        if material['name'] in masked:
            texture = material.get('pbrMetallicRoughness', {}).get('baseColorTexture', {})
            check(name + ' ' + material['name'] + ' actual glTF texture mask',
                  material.get('alphaMode') == 'MASK' and 'index' in texture,
                  alpha_mode=material.get('alphaMode'))
            image = doc['images'][doc['textures'][texture['index']]['source']]
            view = doc['bufferViews'][image['bufferView']]
            start = view.get('byteOffset', 0)
            png = binary[start:start+view['byteLength']]
            check(name + ' ' + material['name'] + ' embedded RGBA mask pixels',
                  png[:8] == b'\x89PNG\r\n\x1a\n' and png[25] == 6)
    triangles = 0
    points = []
    for mesh in doc['meshes']:
        for primitive in mesh['primitives']:
            triangles += doc['accessors'][primitive['indices']]['count'] // 3
            a = doc['accessors'][primitive['attributes']['POSITION']]
            view = doc['bufferViews'][a['bufferView']]
            offset = view.get('byteOffset', 0) + a.get('byteOffset', 0)
            stride = view.get('byteStride', 12)
            for i in range(a['count']):
                x, y, z = struct.unpack_from('<fff', binary, offset+i*stride)
                points.append((x, -z, y))
            check(name + ' glTF UV present', 'TEXCOORD_0' in primitive['attributes'])
    error = max(abs(x-y) for a, b in zip(bounds(points), row['bounds_local_m']) for x, y in zip(a, b))
    check(name + ' GLB matches actual FBX contract', triangles == row['triangles'] and error < .0001,
          maximum_error_m=error, triangles=triangles)

empty()
bpy.ops.import_scene.fbx(filepath=str(R / 'exports' / 'axis_probe.fbx'))
probe = next(o for o in bpy.context.scene.objects if o.type == 'MESH')
lo, hi = bounds([probe.matrix_world @ v.co for v in probe.data.vertices])
got = [(a+b)*.5 for a, b in zip(lo, hi)] + [b-a for a, b in zip(lo, hi)]
check('actual asymmetric export probe', max(abs(a-b) for a, b in zip(got, [2, 4, 6, .2, .4, .6])) < 1e-5)


rows = json.loads((R / 'materials.json').read_text())
byname = {r['name']:r for r in rows}
check('six authored families and28 separate chunks', len(contract['families']) == 6 and len(contract['parts']) == 28)
for family, spec in contract['families'].items():
    blend = R / spec['blend']
    check(family + ' actual original Blender exists', blend.is_file())
    bpy.ops.wm.open_mainfile(filepath=str(blend))
    check(family + ' explicit independently editable scenes', set(spec['scenes']) <= set(bpy.data.scenes.keys()))
    check(family + ' editable original component collections', all(n in bpy.data.collections for n in spec['editable_collections']))
    used = set()
    for name in spec['parts']:
        row = contract['parts'][name]
        ob = bpy.data.objects.get(name)
        check(name + ' saved editable mesh and single root placement', ob is not None and len(ob.vertex_groups) > 0 and ob.location.length < 1e-5 and max(abs(v) for v in ob.rotation_euler) < 1e-5 and max(abs(v-1) for v in ob.scale) < 1e-5)
        check(name + ' original components retained', bpy.data.objects.get('SRC_' + name) is not None)
        used.update(row['materials'])
    required = {(R/path).resolve() for n in used for path in byname[n].get('textures', {}).values()}
    packed = set()
    for im in bpy.data.images:
        if im.source != 'FILE': continue
        disk = Path(bpy.path.abspath(im.filepath)).resolve()
        check(family + ' ' + im.name + ' packed original pixels', im.packed_file is not None and disk.is_file() and im.packed_file.data == disk.read_bytes())
        packed.add(disk)
    check(family + ' all actually used original PBR packed', required <= packed, required=len(required), packed=len(packed))
check('complete descending three authored LODs', all(contract['lods']['0'][n]['triangles'] >= contract['lods']['1'][n]['triangles'] >= contract['lods']['2'][n]['triangles'] for n in contract['parts']))
check('per chunk explicit roots and no baked world kilometre coordinates', all(row['site_world_three_m'] and row['site_local_offset_m'] == [0,0,0] and max(abs(x)for b in row['bounds_local_m']for x in b) < 60 for row in contract['parts'].values()))
finish('PASS')
print('TOWNBUILDINGS_SOURCE_PASS', len(checks))
