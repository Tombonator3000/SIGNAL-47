"""Avgrenset geometriereparasjon av den portable motelpakken, uten baking.

Kjøres bare av koordinatoren i et eksklusivt Blender-vindu. Standard er lesende
inventar; --apply skriver en separat pakke. Original .blend, atlas og teksturer
endres aldri. En allerede reparert pakke valideres og kopieres uten nye flytt.
Ingen Unreal-, import-, pakke- eller prosesskommandoer startes her.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[3] / 'ArtSource/MotelV2'
REPAIR_ID = 'motel-grounded-20261029-v1'
EPS = .0002


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def bounds(coordinates, indices):
    return [[fn(coordinates[i][a] for i in indices) for a in range(3)] for fn in (min, max)]


def close_bounds(actual, expected):
    return all(abs(actual[i][a] - expected[i][a]) <= EPS for i in range(2) for a in range(3))


def inside_bounds(actual, region):
    return all(region[0][a] - EPS <= actual[0][a] <= actual[1][a] <= region[1][a] + EPS for a in range(3))


def shifted(box, delta):
    return [[box[i][a] + delta[a] for a in range(3)] for i in range(2)]


def components(coordinates, faces):
    """Connected islands after the production export's vertex weld.

    Classify complete islands rather than coordinate-selected individual verts,
    so an unexpected connection to a wall fails the exact body checks.
    """
    parent = list(range(len(coordinates)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for face in faces:
        first = find(face[0])
        for i in face[1:]:
            parent[find(i)] = first
    islands = {}
    for i in range(len(coordinates)):
        islands.setdefault(find(i), []).append(i)
    return [{'vertices': ids, 'bounds': bounds(coordinates, ids)} for ids in islands.values()]


def plan_moves(part, coordinates, faces, contract, repaired=False):
    islands = components(coordinates, faces)
    moves = []

    def move(label, selected, delta):
        assert selected, (part, label, 'no complete islands')
        ids = sorted(i for island in selected for i in island['vertices'])
        assert len(ids) == len(set(ids))
        used = {i for row in moves for i in row['vertices']}
        assert not used.intersection(ids), (part, label, 'ambiguous island')
        before = bounds(coordinates, ids)
        moves.append({'object': label, 'vertices': ids, 'islands': len(selected),
                      'before_blender_m': before, 'translation_blender_m': list(delta),
                      'after_blender_m': shifted(before, delta)})

    def cabinet(label, body, details, minimum, maximum):
        translation = contract['translations_blender_m'][label]
        if repaired:
            body, details = shifted(body, translation), shifted(details, translation)
        body_islands = [island for island in islands if close_bounds(island['bounds'], body)]
        assert len(body_islands) == 1, (part, label, 'exact cabinet body', len(body_islands))
        selected = body_islands + [island for island in islands if inside_bounds(island['bounds'], details)]
        assert minimum <= len(selected) <= maximum, (part, label, 'unexpected detail islands', len(selected))
        move(label, selected, [0, 0, 0] if repaired else translation)

    if part == 'SM_Sierra_Office':
        cabinet('ice', [[.8, 2.905, 0], [1.6, 3.555, 1.26]],
                [[.79, 3.55, .1], [1.61, 3.61, 1.4]], 10, 30)
        cabinet('newspaper', [[.73, 3.15, 0], [1.27, 3.71, 1.08]],
                [[.69, 3.71, .50], [1.31, 3.75, .98]], 5, 40)
    elif part == 'SM_Sierra_Glass':
        pane = [[.795, 3.7095, .485], [1.205, 3.7245, .975]]
        if repaired:
            pane = shifted(pane, contract['translations_blender_m']['newspaper'])
        selected = [island for island in islands if close_bounds(island['bounds'], pane)]
        assert len(selected) == 1, (part, 'exact newspaper glass', len(selected))
        move('newspaper_glass', selected, [0, 0, 0] if repaired else contract['translations_blender_m']['newspaper'])
    elif part in {'SM_Sierra_Room02', 'SM_Sierra_Room04', 'SM_Sierra_Room07'}:
        cy = {'SM_Sierra_Room02': -9, 'SM_Sierra_Room04': -19, 'SM_Sierra_Room07': -34}[part]
        region = [[2.79, cy - .26, -.001], [3.31, cy + .32, .89]]
        selected = [island for island in islands if inside_bounds(island['bounds'], region)]
        assert 10 <= len(selected) <= 30, (part, 'unexpected chair islands', len(selected))
        foot = min(island['bounds'][0][2] for island in selected)
        assert (-EPS <= foot <= EPS) if repaired else (.001 < foot < .04), (part, 'unexpected chair foot', foot)
        move('porch_chair', selected, [0, 0, -foot])
    return moves, islands


def mesh_snapshot(mesh):
    positions = [tuple(v.co) for v in mesh.vertices]
    faces = [tuple(p.vertices) for p in mesh.polygons]
    uvs = {layer.name: [tuple(v.uv) for v in layer.data] for layer in mesh.uv_layers}
    assert all(len(face) == 3 for face in faces), 'production geometry must remain triangulated'
    return {'positions': positions, 'faces': faces, 'uvs': uvs,
            'smooth': [p.use_smooth for p in mesh.polygons],
            'materials': [m.name if m else None for m in mesh.materials]}


def append_service_pads(mesh, snapshot, source_island, contract):
    """Reuse an existing concrete slab's geometry/atlas islands for tiny infill.

    The new fields meet the porch and north path edge to edge. No new atlas,
    material, procedural source dependency or coplanar covering plane is added.
    """
    import bmesh
    from mathutils import Vector

    assert not mesh.has_custom_normals, 'custom normals need an explicit preservation path'
    source_ids = set(source_island['vertices'])
    source_faces = [i for i, face in enumerate(snapshot['faces']) if set(face) <= source_ids]
    assert source_faces and all(set(face).isdisjoint(source_ids) or set(face) <= source_ids for face in snapshot['faces'])
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv_layers = {name: bm.loops.layers.uv.get(name) for name in snapshot['uvs']}
    assert all(uv_layers.values())
    source_box = source_island['bounds']
    added = []
    for label, surface in contract['surface_rectangles_blender_xy_m'].items():
        if not surface.get('new_geometry'):
            continue
        x0, x1, y0, y1 = surface['rect']
        target = [[x0, y0, -.12], [x1, y1, surface['top_z']]]
        vertices = {}
        for i in sorted(source_ids):
            old = snapshot['positions'][i]
            co = [target[0][a] + (old[a] - source_box[0][a]) / (source_box[1][a] - source_box[0][a])
                  * (target[1][a] - target[0][a]) for a in range(3)]
            vertices[i] = bm.verts.new(Vector(co))
        for face_index in source_faces:
            old_face = snapshot['faces'][face_index]
            face = bm.faces.new([vertices[i] for i in old_face])
            face.material_index = 0
            face.smooth = snapshot['smooth'][face_index]
            for corner, loop in enumerate(face.loops):
                for name, layer in uv_layers.items():
                    loop[layer].uv = snapshot['uvs'][name][face_index * 3 + corner]
        added.append({'name': label, 'bounds_blender_m': target,
                      'vertices': len(vertices), 'triangles': len(source_faces)})
    bm.normal_update()
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    return added


def export_fbx(bpy, ob, path):
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH'},
                             apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                             axis_forward='-Y', axis_up='Z', bake_anim=False,
                             add_leaf_bones=False, mesh_smooth_type='FACE')


def repair(args, report):
    import bpy

    package = args.package.resolve()
    contract_path = args.contract.resolve()
    contract = json.loads(contract_path.read_text())
    manifest = json.loads((package / 'manifest.json').read_text())
    assert contract['version'] == 1 and contract['repair_id'] == REPAIR_ID
    assert len(manifest['parts']) == 24
    repaired = manifest.get('grounding_repair', {}).get('id') == REPAIR_ID
    if repaired:
        assert manifest['grounding_repair']['contract_sha256'] == sha(contract_path), 'already-repaired contract differs'
    frozen = json.loads((package / 'package-files.json').read_text())
    for name in ['Sierra_Production.blend', 'manifest.json', 'materials.json', 'selection.json']:
        assert sha(package / name) == frozen[name], ('input package freeze differs', name)
    immutable = {name: sha(package / name) for name in frozen if name.startswith(('textures/', 'exports/'))}
    assert all(value == frozen[name] for name, value in immutable.items()), 'input FBX/texture freeze differs'
    assert len([name for name in immutable if name.startswith('textures/')]) == 67
    report.update(repair_id=REPAIR_ID, package=str(package), blender_version=bpy.app.version_string,
                  source_blend_sha256=sha(package / 'Sierra_Production.blend'),
                  source_manifest_sha256=sha(package / 'manifest.json'),
                  contract_sha256=sha(contract_path), already_repaired=repaired)
    bpy.ops.wm.open_mainfile(filepath=str(package / 'Sierra_Production.blend'))
    objects = {ob.name: ob for ob in bpy.data.objects}
    assert set(objects) == set(manifest['parts'])
    assert all(ob.type == 'MESH' for ob in objects.values())
    assert len(bpy.data.meshes) == 24 and len(bpy.data.materials) == 24 and len(bpy.data.images) == 67
    assert all(abs(ob.matrix_world[i][j] - int(i == j)) < 1e-6
               for ob in objects.values() for i in range(4) for j in range(4))
    snapshots, plans = {}, {}
    for name in ['SM_Sierra_Office', 'SM_Sierra_Glass', 'SM_Sierra_Room02', 'SM_Sierra_Room04', 'SM_Sierra_Room07']:
        snap = mesh_snapshot(objects[name].data)
        assert len(snap['faces']) == manifest['parts'][name]['triangles']
        snapshots[name] = snap
        moves, islands = plan_moves(name, snap['positions'], snap['faces'], contract, repaired)
        plans[name] = moves
        if name == 'SM_Sierra_Office':
            slabs = [island for island in islands if close_bounds(island['bounds'],
                      [[2.295, .835, -.17], [5.145, 3.265, 0]])]
            assert len(slabs) == 1, ('exact existing concrete donor slab', len(slabs))
            donor = slabs[0]
            office_floor = [island for island in islands if close_bounds(island['bounds'],
                            [[-2.22, -2.675, -.09], [2.22, 2.675, 0]])]
            assert len(office_floor) == 1, ('exact reception floor underlay', len(office_floor))
            report['office_floor_underlay_blender_m'] = office_floor[0]['bounds']
    planting = mesh_snapshot(objects['SM_Sierra_Planting'].data)
    plant_islands = components(planting['positions'], planting['faces'])
    report['retained_native_pots'] = []
    for cy in (1.9, -16.1):
        pot = [island for island in plant_islands if inside_bounds(island['bounds'],
               [[2.72, cy - .28, -.001], [3.28, cy + .28, .47]])]
        assert len(pot) == 2, ('exact pot and lip islands', cy, len(pot))
        ids = [i for island in pot for i in island['vertices']]
        box = bounds(planting['positions'], ids)
        assert abs(box[0][2]) < EPS, ('native pot foot', cy, box)
        report['retained_native_pots'].append({'bounds_blender_m': box, 'change': 'none'})
    report['moves'] = {name: [{**{k: v for k, v in row.items() if k != 'vertices'},
                              'vertex_count': len(row['vertices'])} for row in rows] for name, rows in plans.items()}
    report['new_service_fields'] = {name: row for name, row in contract['surface_rectangles_blender_xy_m'].items()
                                    if row.get('new_geometry')}
    report['preserved'] = contract['preserve']
    if not args.apply:
        report['status'] = 'INVENTORY_PASS_SOURCE_AND_NATIVE_UNVERIFIED'
        return
    assert args.out, '--apply requires --out'
    out = args.out.resolve()
    assert out != package and not out.is_relative_to(package) and not package.is_relative_to(out), 'separate output package required'
    out.mkdir(parents=True, exist_ok=True)
    # Copy only declared portable package files, never authoring backups/output.
    for name in frozen:
        destination = out / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(package / name, destination)
    shutil.copy2(package / 'package-files.json', out / 'package-files.json')
    shutil.copy2(contract_path, out / 'grounding-contract.json')
    if repaired:
        report['status'] = 'NOOP_ALREADY_REPAIRED_NATIVE_UNVERIFIED'
        report['out'] = str(out)
        return
    for name, rows in plans.items():
        mesh = objects[name].data
        for row in rows:
            delta = row['translation_blender_m']
            for i in row['vertices']:
                for axis in range(3):
                    mesh.vertices[i].co[axis] += delta[axis]
        mesh.update()
    pads = append_service_pads(objects['SM_Sierra_Office'].data, snapshots['SM_Sierra_Office'], donor, contract)
    for name, before in snapshots.items():
        after = mesh_snapshot(objects[name].data)
        count = len(before['positions'])
        assert after['materials'] == before['materials']
        assert after['faces'][:len(before['faces'])] == before['faces'], (name, 'original topology changed')
        assert all(after['uvs'][key][:len(values)] == values for key, values in before['uvs'].items()), (name, 'original atlas UV changed')
        assert after['smooth'][:len(before['smooth'])] == before['smooth'], (name, 'original smoothing changed')
        expected = list(before['positions'])
        for row in plans[name]:
            for i in row['vertices']:
                expected[i] = tuple(expected[i][a] + row['translation_blender_m'][a] for a in range(3))
        assert all(abs(after['positions'][i][a] - expected[i][a]) < 1e-5 for i in range(count) for a in range(3)), (name, 'unplanned vertex change')
        if name != 'SM_Sierra_Office':
            assert len(after['positions']) == count and len(after['faces']) == len(before['faces'])
    scene = bpy.data.scenes['Sierra production exports']
    bpy.context.window.scene = scene
    for name in plans:
        ob = objects[name]
        spec = manifest['parts'][name]
        export_fbx(bpy, ob, out / 'exports' / (name + '.fbx'))
        lods = [{'triangles': len(ob.data.polygons), 'bounds_local_m': bounds([tuple(v.co) for v in ob.data.vertices], range(len(ob.data.vertices))),
                 'materials': spec['materials']}]
        for lod, ratio in [(1, .5), (2, .22)]:
            lower = ob.copy()
            lower.data = ob.data.copy()
            lower.name = name + '_LOD' + str(lod)
            scene.collection.objects.link(lower)
            bpy.ops.object.select_all(action='DESELECT')
            lower.select_set(True)
            bpy.context.view_layer.objects.active = lower
            modifier = lower.modifiers.new('Authored distance simplification', 'DECIMATE')
            modifier.ratio = ratio
            modifier.use_collapse_triangulate = True
            bpy.ops.object.modifier_apply(modifier=modifier.name)
            assert 0 < len(lower.data.polygons) < len(ob.data.polygons)
            lods.append({'triangles': len(lower.data.polygons),
                         'bounds_local_m': bounds([tuple(v.co) for v in lower.data.vertices], range(len(lower.data.vertices))),
                         'materials': spec['materials']})
            export_fbx(bpy, lower, out / 'exports' / (lower.name + '.fbx'))
            data = lower.data
            bpy.data.objects.remove(lower, do_unlink=True)
            bpy.data.meshes.remove(data)
            for temporary_mesh in list(bpy.data.meshes):
                if temporary_mesh.users == 0:
                    bpy.data.meshes.remove(temporary_mesh)
        spec.update(triangles=lods[0]['triangles'], vertices=len(ob.data.vertices), lods=lods,
                    lod_triangles=[lod['triangles'] for lod in lods], bounds_local_m=lods[0]['bounds_local_m'],
                    source_geometry_sha256=sha(out / 'exports' / (name + '.fbx')))
        if name == 'SM_Sierra_Office':
            spec['source_objects'].extend('Native grounded ' + row['name'] for row in pads)
    changed_fbx = {f'exports/{name}{suffix}.fbx' for name in plans for suffix in ['', '_LOD1', '_LOD2']}
    assert all(sha(out / name) == value for name, value in immutable.items() if name not in changed_fbx), 'unrelated FBX/texture changed'
    assert sha(out / 'materials.json') == frozen['materials.json'], 'material manifest changed'
    assert set(ob.name for ob in bpy.data.objects) == set(manifest['parts'])
    assert len(bpy.data.meshes) == 24 and len(bpy.data.materials) == 24 and len(bpy.data.images) == 67
    for image in bpy.data.images:
        assert image.packed_file and image.filepath.startswith('//textures/'), ('nonportable texture', image.filepath)
    temporary = out / 'Sierra_Production.building.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(temporary), compress=True, relative_remap=False)
    temporary.replace(out / 'Sierra_Production.blend')
    manifest['status'] = 'SOURCE_GEOMETRY_REPAIRED_NATIVE_UNVERIFIED'
    manifest['grounding_repair'] = {'id': REPAIR_ID, 'input_blend_sha256': report['source_blend_sha256'],
                                    'input_manifest_sha256': report['source_manifest_sha256'],
                                    'contract_sha256': report['contract_sha256'], 'added_pads': pads,
                                    'changed_parts': sorted(plans), 'material_bake': 'unchanged atlas/UV; no rebake'}
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    provenance = json.loads((out / 'provenance.json').read_text())
    provenance['grounding_repair'] = manifest['grounding_repair']
    (out / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    selection = json.loads((out / 'selection.json').read_text())
    selection['chunks']['SM_Sierra_Office'].extend('Native grounded ' + row['name'] for row in pads)
    selection['grounding_repair'] = manifest['grounding_repair']
    (out / 'selection.json').write_text(json.dumps(selection, indent=2) + '\n')
    frozen['grounding-contract.json'] = ''
    (out / 'package-files.json').write_text(json.dumps({name: sha(out / name) for name in frozen}, indent=2) + '\n')
    assert sha(package / 'Sierra_Production.blend') == report['source_blend_sha256'], 'immutable input changed'
    report.update(status='REPAIRED_SOURCE_AND_NATIVE_UNVERIFIED', out=str(out), added_pads=pads,
                  untouched_fbx_files_preserved=len([name for name in immutable if name.startswith('exports/') and name not in changed_fbx]),
                  texture_files_preserved=67, material_manifest_sha256=sha(out / 'materials.json'),
                  output_blend_sha256=sha(out / 'Sierra_Production.blend'), output_manifest_sha256=sha(out / 'manifest.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, default=PACKAGE)
    parser.add_argument('--contract', type=Path, default=PACKAGE / 'grounding-contract.json')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--apply', action='store_true')
    parser.add_argument('--out', type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    assert args.report.suffix == '.json' and not args.report.resolve().is_relative_to(args.package.resolve()), 'separate report path required'
    report = {'schema': 1, 'status': 'FAIL', 'scope': 'Source geometry only; native/gameplay acceptance unverified'}
    try:
        repair(args, report)
    except Exception as error:
        report['error'] = {'type': type(error).__name__, 'detail': str(error)}
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print('SIERRA_GROUNDING', json.dumps({'status': report['status'], 'report': str(args.report)}), flush=True)


if __name__ == '__main__':
    main()
