"""Fresh-file checks: LODs, slots, closed outward topology, pivots and named points; re-reads the FBX."""
from pathlib import Path
import json, math
import bpy, bmesh
HERE = Path(__file__).resolve().parent
bpy.ops.wm.open_mainfile(filepath=str(HERE / 'SIGNAL47_StreetLamps.blend'))
d = json.loads((HERE / 'model_contract.json').read_text()); checks = []
def check(name, ok, detail=None):
    checks.append({'name': name, 'pass': bool(ok), **({'detail': detail} if detail is not None else {})}); assert ok, (name, detail)
def three(v): return (v[0], v[2], -v[1])
BUDGET = {'SM_StreetLamp_Head': 3000, 'SM_StreetLamp_Arm': 800, 'SM_StreetLamp_Pole': 1500, 'SM_StreetLamp_WoodArm': 800}
for name, row in d['parts'].items():
    tris = []; slots = []
    for lod in range(3):
        ob = bpy.data.objects[name + ('_LOD' + str(lod) if lod else '')]; me = ob.data
        me.calc_loop_triangles(); tris.append(len(me.loop_triangles)); slots.append([m.name for m in me.materials])
        check(name + ' LOD' + str(lod) + ' triangles', tris[-1] == row['lod_triangles'][lod])
        check(name + ' LOD' + str(lod) + ' triangulated with UVs', all(len(p.vertices) == 3 and p.area > 1e-12 for p in me.polygons) and bool(me.uv_layers))
        check(name + ' LOD' + str(lod) + ' identity transform at pivot', tuple(ob.location) == (0, 0, 0) and tuple(ob.scale) == (1, 1, 1))
        bm = bmesh.new(); bm.from_mesh(me)
        check(name + ' LOD' + str(lod) + ' closed (no open edges)', all(e.is_manifold for e in bm.edges))
        check(name + ' LOD' + str(lod) + ' outward normals', bm.calc_volume(signed=True) > 0)
        bm.free()
        check(name + ' LOD' + str(lod) + ' export exists', (HERE / 'Export' / (ob.name + '.fbx')).is_file())
    check(name + ' budget', tris[0] < BUDGET[name], tris)
    check(name + ' three meaningful LODs', tris[1] < tris[0] * .65 and tris[2] < tris[0] * .25, tris)
    check(name + ' identical slot order on all LODs', all(s == slots[0] for s in slots) and slots[0] == row['materials'], slots)
h = d['parts']['SM_StreetLamp_Head']
n = h['lens_normal_three']; c = h['lens_centre_three_m']; em = h['emitter_three_m']
check('head lens slot separate', d['lens_material'] in h['materials'])
check('head unit optical normal', abs(sum(x * x for x in n) - 1) < 1e-9)
check('head emitter just beyond glass', .005 < sum((em[i] - c[i]) * n[i] for i in range(3)) < .011)
check('head pivot at slipfitter mouth (x min = 0)', abs(h['bounds_three_m'][0][0]) < 1e-6)
check('head size within brief', abs(h['bounds_three_m'][1][0] - .78) < 1e-4 and abs(h['bounds_three_m'][1][2] - .17) < .003)
a = d['parts']['SM_StreetLamp_Arm']
check('arm end point measured', max(abs(a['measured_end_cap_centre_three_m'][k] - a['head_mount_three_m'][k]) for k in range(3)) < 1e-5)
check('arm reach 1.8 m and rise 0.45 m', a['head_mount_three_m'][:2] == [1.8, .45])
p = d['parts']['SM_StreetLamp_Pole']
check('pole on ground, 8.0 m', abs(p['bounds_three_m'][0][1]) < 1e-6 and abs(p['bounds_three_m'][1][1] - 8.0) < 1e-6)
check('arm clamp fits pole at mount', 0 < a['clamp_band']['inner_radius_m'] - p['radius_at_arm_mount_m'] < .002)
asm = d['assembly_three_m']
check('assembled head pivot', abs(asm['head_pivot'][0] - 1.8) < 1e-6 and abs(asm['head_pivot'][1] - (p['arm_mount_three_m'][1] + .45)) < 1e-6)
# Read the exported LOD0 FBX files back and compare bounds with the contract (metric, axes).
for name, row in d['parts'].items():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(HERE / 'Export' / (name + '.fbx')))
    obs = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    pts = [o.matrix_world @ v.co for o in obs for v in o.data.vertices]
    lo = [min(q[k] for q in pts) for k in range(3)]; hi = [max(q[k] for q in pts) for k in range(3)]
    want = row['bounds_local_m']
    err = max(max(abs(lo[k] - want[0][k]), abs(hi[k] - want[1][k])) for k in range(3))
    check(name + ' FBX read-back bounds (m)', err < 1e-3, {'read': [lo, hi], 'err': err})
    check(name + ' FBX read-back slots', sorted(m.name.split('.')[0] for o in obs for m in o.data.materials) == sorted(row['materials']))
rows = json.loads((HERE / 'materials.json').read_text())
for r in rows:
    check(r['name'] + ' maps on disk', all((HERE / f).is_file() for f in r['textures'].values()))
    check(r['name'] + ' DirectX normal contract', r['normal_convention'] == 'DirectX')
result = {'status': 'PASS', 'scope': 'Fresh Blender source and FBX read-back, not native Unreal or runtime',
          'checks': checks, 'passed': len(checks), 'total': len(checks)}
(HERE / 'source-verification.json').write_text(json.dumps(result, indent=2) + '\n')
print('STREETLAMP SOURCE VERIFIED', len(checks))
