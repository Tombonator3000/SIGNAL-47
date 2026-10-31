"""Root-run separation of inspected real patrol-door surface triangles, no new geometry.

Run after prepare.py, before native import. Keeps original.blend/raw GLB immutable.
"""
from pathlib import Path
import argparse
import copy
import json
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prepare

# Source overview/side inspection, canonical Blender front+Y. This follows the
# front door from rocker, up A pillar, along window frame and down B pillar.
PROFILE_YZ = [(1.04, .56), (1.00, 1.05), (.59, 1.45), (-.13, 1.45), (-.18, .56)]
X_RANGE = [-1.19, -.63]
PIVOT = [-.99, 1.00, .83]
NAME = 'SM_TripoFleet_PoliceCarDoor'


def inside(y, z):
    found = False
    previous = PROFILE_YZ[-1]
    for current in PROFILE_YZ:
        a, b = previous, current
        if (a[1] > z) != (b[1] > z) and y < (b[0] - a[0]) * (z - a[1]) / (b[1] - a[1]) + a[0]:
            found = not found
        previous = current
    return found


def selected(face):
    center = face.calc_center_median()
    return all(X_RANGE[0] <= v.co.x <= X_RANGE[1] for v in face.verts) and inside(center.y, center.z)


def subset(mesh, keep_door):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    remove = [f for f in bm.faces if selected(f) != keep_door]
    bmesh.ops.delete(bm, geom=remove, context='FACES')
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()


def main():
    import bpy
    from mathutils import Matrix, Vector

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sources', required=True, type=Path)
    options = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    root, selected_rows = prepare.read_sources(options.sources.expanduser().resolve(), {'police_car'})
    row = selected_rows[0]
    output = root / 'police_car'
    contract_path = output / 'model_contract.json'
    contract = json.loads(contract_path.read_text())
    profile = {'polygon_yz': PROFILE_YZ, 'x_range': X_RANGE, 'hinge_blender_m': PIVOT,
               'method': 'whole source triangles by inspected outer door profile; no new faces'}
    if contract.get('source_panel_split') == profile:
        print('S47_PATROL_DOOR_ALREADY_READY', str(contract_path), flush=True)
        return
    prepare.require(not contract.get('source_panel_split'), 'Changed door selection requires fresh prepare.py source generation')
    source_blend = output / 'original.blend'
    original_hash = prepare.sha(source_blend)
    bpy.ops.wm.open_mainfile(filepath=str(output / 'prepared.blend'))
    original_part = contract['parts'][row['name']]
    rows = json.loads((output / 'materials.json').read_text())
    by_material = {m['name']: m for m in rows}
    body_lods, door_lods, door_objects = [], [], []
    for level in range(3):
        suffix = '' if level == 0 else '_LOD' + str(level)
        body = bpy.data.objects[row['name'] + suffix]
        before = prepare.measurements(body)['triangles']
        door = body.copy()
        door.data = body.data.copy()
        door.name = NAME + suffix
        door.hide_render = False
        bpy.context.scene.collection.objects.link(door)
        subset(door.data, True)
        subset(body.data, False)
        prepare.require(len(door.data.polygons) >= 10, 'Source driver-door profile missed actual panel in LOD' + str(level))
        door.data.transform(Matrix.Translation(-Vector(PIVOT)))
        for index, material in enumerate(list(door.data.materials)):
            new_name = material.name + '_Door'
            double = bpy.data.materials.get(new_name)
            if double is None:
                double = material.copy()
                double.name = new_name
                double.use_backface_culling = False
                sidecar = copy.deepcopy(by_material[material.name])
                sidecar.update(name=new_name, two_sided=True,
                               source_name=sidecar['source_name'] + ' / source driver door panel')
                rows.append(sidecar)
            door.data.materials[index] = double
        body_measure = prepare.measurements(body)
        door_measure = prepare.measurements(door)
        prepare.require(body_measure['triangles'] + door_measure['triangles'] == before,
                        'Door/body must be an exact disjoint union of the actual source faces')
        for obj, measured, destination in [(body, body_measure, body_lods), (door, door_measure, door_lods)]:
            relative = 'exports/' + obj.name + '.fbx'
            prepare.export_fbx(obj, output / relative)
            destination.append(dict(measured, lod=level, fbx=relative, source_face_union_triangles=before))
        door.location = Vector(PIVOT)
        door.hide_render = level > 0
        door.hide_set(level > 0)
        body.hide_render = level > 0
        body.hide_set(level > 0)
        door_objects.append(door)
    contract['parts'][row['name']] = dict(body_lods[0], asset_folder=original_part['asset_folder'], lods=body_lods)
    contract['parts'][NAME] = dict(door_lods[0], asset_folder=original_part['asset_folder'], lods=door_lods,
                                  hinge_blender_m=PIVOT)
    contract['source_panel_split'] = profile
    prepare.select([bpy.data.objects[row['name']]])
    bpy.ops.wm.save_as_mainfile(filepath=str(output / 'prepared.blend'))
    prepare.require(prepare.sha(source_blend) == original_hash, 'Original editable source was modified')
    prepare.write_json(output / 'materials.json', rows)
    prepare.write_json(contract_path, contract)
    print('S47_PATROL_DOOR_READY', [r['triangles'] for r in body_lods],
          [r['triangles'] for r in door_lods], str(contract_path), flush=True)


if __name__ == '__main__':
    main()
