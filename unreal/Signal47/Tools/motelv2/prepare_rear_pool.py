"""Move the existing corrected pool kit behind the room row without new baking.

Root runs Blender serially: --python prepare_rear_pool.py -- --source PRIVATE_WALLS
--baseline ArtSource/MotelV2 --out NEW_PRIVATE_DIRECTORY. Original atlases, normals,
UVs, doors, office and world anchors are retained. Exports all three source LODs.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import sys

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('motel_orientation', HERE / 'repair_face_orientation.py')
O = importlib.util.module_from_spec(spec)
spec.loader.exec_module(O)
G = O.G
POOL = {'SM_Sierra_PoolDeck', 'SM_Sierra_PoolFence', 'SM_Sierra_PoolFurniture', 'SM_Sierra_PoolWater'}
SHIFT = Vector((0, -25, 0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--baseline', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    source, baseline, out = args.source.resolve(), args.baseline.resolve(), args.out.resolve()
    assert not out.exists() and out not in (source, baseline), 'Fresh private output required'
    shutil.copytree(baseline, out)
    shutil.copytree(source / 'exports', out / 'exports', dirs_exist_ok=True)
    contract = json.loads((source / 'manifest.json').read_text())
    bpy.ops.wm.open_mainfile(filepath=str(source / 'Sierra_Walls_Repaired.blend'))
    objects = {o.name: o for o in bpy.data.objects if o.type == 'MESH' and o.name in contract['parts']}
    assert set(objects) == set(contract['parts']), 'All unchanged original production parts required'
    # Translate base coordinates only; UVs/custom-normal storage remain byte-identical.
    for name in POOL:
        ob = objects[name]
        for vertex in ob.data.vertices:
            vertex.co += SHIFT
        ob.data.update()
    court = objects['SM_Sierra_CourtDetail']
    snap = G.mesh_snapshot(court.data)
    # Avoid joining Blender5 short2 and float3 custom-normal buffers.
    G.set_float_corner_normals(court.data, snap['corner_normals'])
    islands = O.island_inventory(snap['positions'], snap['faces'])
    old_paths = [((-7.9,-6.5,-.12),(-6.7,5.,0)), ((-8.,-6.5,-.12),(-7.8,-4.9,0))]
    for low, high in old_paths:
        matches = [r for r in islands if max(abs(a-b) for aa,bb in zip(r['bounds'], [low,high]) for a,b in zip(aa,bb)) < .001]
        assert len(matches) == 1, ('Exact original approach island', low, high, len(matches))
        for index in matches[0]['vertices']:
            court.data.vertices[index].co += SHIFT
    court.data.update()
    # Reuse the original baked concrete atlas and surface UVs for two new path slabs.
    bpy.context.window.scene = next(s for s in bpy.data.scenes if court.name in s.objects)
    bpy.ops.object.select_all(action='DESELECT')
    template = old_paths[0]
    old = next(r for r in islands if max(abs(a-b) for aa,bb in zip(r['bounds'], template) for a,b in zip(aa,bb)) < .001)
    vertices = old['vertices']
    lookup = {value:index for index,value in enumerate(vertices)}
    additions = []
    for label, low, high in [('rear-route',(-5.2,-20,-.12),(-4.,3.4,0)),
                             ('rear-link',(-8.,-21.6,-.12),(-4.,-20.,0))]:
        positions = []
        for index in vertices:
            p = snap['positions'][index]
            positions.append(tuple(low[k] + (p[k]-template[0][k])/(template[1][k]-template[0][k])*(high[k]-low[k]) for k in range(3)))
        data = bpy.data.meshes.new('PoolPath_' + label)
        data.from_pydata(positions, [], [tuple(lookup[v] for v in snap['faces'][f]) for f in old['faces']])
        for material in court.data.materials:
            data.materials.append(material)
        uv = data.uv_layers.new(name='UVMap')
        normals = []
        for polygon, face in zip(data.polygons, old['faces']):
            polygon.material_index = snap['material_indices'][face]
            polygon.use_smooth = snap['smooth'][face]
            for dst, src in zip(polygon.loop_indices, range(snap['loop_starts'][face], snap['loop_starts'][face]+3)):
                uv.data[dst].uv = snap['uvs']['UVMap'][src]
                normals.append(snap['corner_normals'][src])
        G.set_float_corner_normals(data, normals)
        ob = bpy.data.objects.new('PoolPath_' + label, data)
        bpy.context.collection.objects.link(ob)
        ob.select_set(True)
        additions.append(ob)
    court.select_set(True)
    bpy.context.view_layer.objects.active = court
    bpy.ops.object.join()
    court.data.update()
    modified = POOL | {court.name}
    # Existing parts retain their accepted FBX files except the five changed owners.
    for name in sorted(modified):
        ob = objects[name]
        bpy.context.window.scene = next(s for s in bpy.data.scenes if ob.name in s.objects)
        rows = []
        for lod, ratio in [(0,1.), (1,.5), (2,.22)]:
            chosen = ob
            if lod:
                chosen = ob.copy()
                chosen.data = ob.data.copy()
                chosen.name = name + '_LOD' + str(lod)
                bpy.context.collection.objects.link(chosen)
                bpy.ops.object.select_all(action='DESELECT')
                chosen.select_set(True)
                bpy.context.view_layer.objects.active = chosen
                modifier = chosen.modifiers.new('Consistent rear pool LOD', 'DECIMATE')
                modifier.ratio = ratio
                modifier.use_collapse_triangulate = True
                bpy.ops.object.modifier_apply(modifier=modifier.name)
            snapshot = G.mesh_snapshot(chosen.data)
            rows.append({'triangles':len(snapshot['faces']), 'bounds_local_m':G.bounds(snapshot['positions'],range(len(snapshot['positions']))),
                         'materials':contract['parts'][name]['materials']})
            G.export_fbx(bpy, chosen, out / 'exports' / (name + ('_LOD'+str(lod) if lod else '') + '.fbx'))
            if lod:
                bpy.data.objects.remove(chosen, do_unlink=True)
        contract['parts'][name].update(lods=rows,lod_triangles=[r['triangles'] for r in rows],
                                       triangles=rows[0]['triangles'],bounds_local_m=rows[0]['bounds_local_m'])
    contract['rear_pool'] = {'offset_blender_m':list(SHIFT), 'water_center_world_three_m':[-58.9,-2.2385683853,698.9],
                             'changed_parts':sorted(modified), 'anchors':'Original lobby, rooms, room6, sign and highway retained'}
    # All native parts get the same legacy FBX translator; the corrected saved source
    # has the earlier wall-normal repair and the new pool location in one package.
    (out / 'manifest.json').write_text(json.dumps(contract,indent=2)+'\n')
    (out / 'repair.json').write_text(json.dumps({'status':'SOURCE_READY_NATIVE_UNVERIFIED',
        'parts':{name:{} for name in contract['parts']},'source':str(source),'rear_pool':contract['rear_pool']},indent=2)+'\n')
    bpy.ops.wm.save_as_mainfile(filepath=str(out / 'Sierra_Production.blend'),compress=True)
    print('MOTEL_REAR_POOL_SOURCE_READY', out, flush=True)


if __name__ == '__main__':
    main()
