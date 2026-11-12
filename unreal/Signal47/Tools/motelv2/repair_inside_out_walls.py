"""PLAY-31-01: turn the inside-out solids inside larger islands (inside_out_regions.py) the right way, originals preserved.
Root's serial Blender command (queue): blender -b --python repair_inside_out_walls.py -- --source PACKAGE --out NEW_DIR
  [--parts SM_Sierra_Office ...]
PACKAGE is the package the native assets were imported from (31 October rear-pool package for the office). Only faces of
closed solids with negative signed volume are re-wound; positions, UVs and materials stay; corner normals turn with them
(repair_face_orientation's flip_snapshot/replace_mesh, as repair_playtest_walls.py). LOD1/LOD2 are decimated from the
repaired LOD0 with the same ratios as repair_playtest_walls.py, which made the package being replaced. No engine process.
Then: import_playtest_walls.py (S47_MOTEL_WALL_SOURCE=NEW_DIR) and set_rt_lod.py for the distant LOD3.
"""
import argparse, importlib.util, json, sys
from pathlib import Path
import bpy
R = Path(__file__).resolve().parent
sp = importlib.util.spec_from_file_location('motel_orient', R / 'repair_face_orientation.py')
O = importlib.util.module_from_spec(sp); sp.loader.exec_module(O); G = O.G
sys.path.insert(0, str(R))
import inside_out_regions as IO

ap = argparse.ArgumentParser()
ap.add_argument('--source', type=Path, required=True); ap.add_argument('--out', type=Path, required=True)
ap.add_argument('--parts', nargs='*', default=None)
a = ap.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
source = a.source.resolve(); out = a.out.resolve()
assert source != out and not out.exists(), 'new output directory required'
out.mkdir(parents=True); (out / 'exports').mkdir()
manifest = json.loads((source / 'manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(source / 'Sierra_Production.blend'))
objects = {ob.name: ob for ob in bpy.data.objects if ob.type == 'MESH'}
report = {'status': 'RUNNING', 'parts': {}, 'source': str(source), 'tool': 'repair_inside_out_walls.py'}
for part, ob in objects.items():
    if part not in manifest['parts'] or (a.parts and part not in a.parts):
        continue
    if any(x in part for x in ['Glass', 'Water', 'Glow', 'Planting']):
        continue
    bpy.context.window.scene = next(scene for scene in bpy.data.scenes if ob.name in scene.objects)
    before = G.mesh_snapshot(ob.data, include_normals=True)
    flips, rows = IO.inward_faces(before['positions'], before['faces'])
    if not flips:
        continue
    expected = O.flip_snapshot(before, flips); O.replace_mesh(bpy, ob, before, expected)
    after = G.mesh_snapshot(ob.data, include_normals=True)
    assert after['positions'] == before['positions'], (part, 'positions moved')
    assert after['uvs'] == expected['uvs'], (part, 'UVs differ from the planned flip')
    left, _ = IO.inward_faces(after['positions'], after['faces'])
    assert not left, (part, 'inside-out solids remain', len(left))
    spec = manifest['parts'][part]; lods = []
    for lod, ratio in [(0, 1.), (1, .5), (2, .22)]:
        selected = ob
        if lod:
            selected = ob.copy(); selected.data = ob.data.copy(); selected.name = part + '_LOD' + str(lod)
            bpy.context.scene.collection.objects.link(selected)
            bpy.ops.object.select_all(action='DESELECT'); selected.select_set(True); bpy.context.view_layer.objects.active = selected
            mod = selected.modifiers.new('Authored distance simplification', 'DECIMATE'); mod.ratio = ratio; mod.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=mod.name)
        snap = G.mesh_snapshot(selected.data)
        lods.append({'triangles': len(snap['faces']), 'bounds_local_m': G.bounds(snap['positions'], range(len(snap['positions']))),
                     'materials': spec['materials'],
                     'inside_out_faces_left': len(IO.inward_faces(snap['positions'], snap['faces'])[0])})
        G.export_fbx(bpy, selected, out / 'exports' / (selected.name + '.fbx'))
        if lod:
            bpy.data.objects.remove(selected, do_unlink=True)
    spec['lods'] = [{k: v for k, v in l.items() if k != 'inside_out_faces_left'} for l in lods]
    spec['lod_triangles'] = [l['triangles'] for l in lods]
    report['parts'][part] = {'flipped_triangles': len(flips), 'solids': rows, 'lod_triangles': spec['lod_triangles'],
                             'inside_out_faces_left_per_lod': [l['inside_out_faces_left'] for l in lods]}
assert report['parts'], 'no inside-out solids found'
bpy.ops.wm.save_as_mainfile(filepath=str(out / 'Sierra_InsideOut_Repaired.blend'), compress=True)
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
report['status'] = 'SOURCE_READY_NATIVE_UNVERIFIED'; (out / 'repair.json').write_text(json.dumps(report, indent=2) + '\n')
print('MOTEL_INSIDE_OUT_REPAIR_SOURCE_READY', json.dumps({k: v['flipped_triangles'] for k, v in report['parts'].items()}), flush=True)
