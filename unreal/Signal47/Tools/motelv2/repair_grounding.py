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
import math
import shutil
import sys
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[3] / 'ArtSource/MotelV2'
REPAIR_ID = 'motel-grounded-20261029-v1'
EPS = .0002
NORMAL_COMPONENT_EPS = .0002
NORMAL_DOT_EPS = .0000001
SELECTION_ATTRIBUTES = {
    '.select_vert': 'POINT', '.select_edge': 'EDGE', '.select_poly': 'FACE',
    '.uv_select_vert': 'CORNER', '.uv_select_edge': 'CORNER', '.uv_select_face': 'FACE',
}
KNOWN_ATTRIBUTES = {
    'position': ('POINT', 'FLOAT_VECTOR'), '.edge_verts': ('EDGE', 'INT32_2D'),
    '.corner_vert': ('CORNER', 'INT'), '.corner_edge': ('CORNER', 'INT'),
    'UVMap': ('CORNER', 'FLOAT2'), 'custom_normal': ('CORNER', 'INT16_2D'),
    'material_index': ('FACE', 'INT'), 'sharp_face': ('FACE', 'BOOLEAN'),
    # The normal setter may create sharp flags; original edge semantics are
    # restored after it encodes only the new, disconnected donor islands.
    'sharp_edge': ('EDGE', 'BOOLEAN'),
    **{name: (domain, 'BOOLEAN') for name, domain in SELECTION_ATTRIBUTES.items()},
}


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


def unit_normal(vector):
    assert len(vector) == 3 and all(math.isfinite(v) for v in vector), ('invalid normal', vector)
    length = math.sqrt(sum(v * v for v in vector))
    assert length > 1e-12, ('zero normal must not become automatic shading', vector)
    return tuple(v / length for v in vector)


def scaled_normal(normal, scale):
    """Normal of an axis-scaled donor: normalize((S^-1)^T * n)."""
    assert all(math.isfinite(v) and v > 0 for v in scale), ('invalid infill scale', scale)
    return unit_normal(tuple(normal[a] / scale[a] for a in range(3)))


def normal_check(actual, expected, label):
    assert len(actual) == len(expected), (label, 'corner normal count', len(actual), len(expected))
    max_component, min_dot = 0., 1.
    for i, (got, wanted) in enumerate(zip(actual, expected)):
        got_unit, wanted_unit = unit_normal(got), unit_normal(wanted)
        delta = max(abs(got[a] - wanted[a]) for a in range(3))
        dot = sum(got_unit[a] * wanted_unit[a] for a in range(3))
        max_component, min_dot = max(max_component, delta), min(min_dot, dot)
        assert delta <= NORMAL_COMPONENT_EPS and dot >= 1 - NORMAL_DOT_EPS, (
            label, 'corner normal changed', i, delta, dot, got, wanted)
    return {'corners': len(expected), 'max_component_delta': max_component, 'min_normalized_dot': min_dot}


def mesh_snapshot(mesh, include_normals=True):
    schema = {attribute.name: (attribute.domain, attribute.data_type) for attribute in mesh.attributes}
    assert all(KNOWN_ATTRIBUTES.get(name) == kind for name, kind in schema.items()), ('unknown mesh attribute', schema)
    assert not mesh.color_attributes and mesh.shape_keys is None, 'unsupported color attributes or shape keys'
    assert [layer.name for layer in mesh.uv_layers] == ['UVMap'], 'unsupported additional UV layers'
    positions = [tuple(v.co) for v in mesh.vertices]
    faces = [tuple(p.vertices) for p in mesh.polygons]
    uvs = {layer.name: [tuple(v.uv) for v in layer.data] for layer in mesh.uv_layers}
    assert all(len(face) == 3 for face in faces), 'production geometry must remain triangulated'
    custom_normals_raw = None
    if 'custom_normal' in schema:
        custom_normals_raw = [0] * (len(mesh.loops) * 2)
        mesh.attributes['custom_normal'].data.foreach_get('value', custom_normals_raw)
    snapshot = {'positions': positions, 'faces': faces, 'uvs': uvs,
                'loop_vertices': [loop.vertex_index for loop in mesh.loops],
                'edges': [tuple(edge.vertices) for edge in mesh.edges],
                'loop_edge_keys': [tuple(sorted(mesh.edges[loop.edge_index].vertices)) for loop in mesh.loops],
                'loop_starts': [p.loop_start for p in mesh.polygons],
                'loop_totals': [p.loop_total for p in mesh.polygons],
                'smooth': [p.use_smooth for p in mesh.polygons],
                'material_indices': [p.material_index for p in mesh.polygons],
                'materials': [m.name if m else None for m in mesh.materials],
                'has_custom_normals': mesh.has_custom_normals,
                'custom_normals_raw': custom_normals_raw,
                'attribute_schema': schema,
                'selection_attributes': {name: [bool(value.value) for value in mesh.attributes[name].data]
                                         for name in SELECTION_ATTRIBUTES if name in schema},
                'sharp_edges': [bool(value.value) for value in mesh.attributes['sharp_edge'].data]
                               if 'sharp_edge' in schema else None}
    if include_normals:
        snapshot['corner_normals'] = [tuple(normal.vector) for normal in mesh.corner_normals]
        assert len(snapshot['corner_normals']) == len(snapshot['loop_vertices'])
    return snapshot


def check_source_prefix(before, after, label):
    assert after['materials'] == before['materials'], (label, 'material slots changed')
    for key in ('faces', 'edges', 'loop_vertices', 'loop_starts', 'loop_totals', 'smooth', 'material_indices', 'loop_edge_keys'):
        assert after[key][:len(before[key])] == before[key], (label, 'original prefix changed', key)
    assert all(after['uvs'][key][:len(values)] == values for key, values in before['uvs'].items()), (label, 'original atlas UV changed')
    assert all(after['attribute_schema'].get(name) == kind for name, kind in before['attribute_schema'].items()), (label, 'attribute schema lost')
    if before['custom_normals_raw'] is not None:
        assert after['custom_normals_raw'] is not None, (label, 'custom normals lost')
        assert after['custom_normals_raw'][:len(before['custom_normals_raw'])] == before['custom_normals_raw'], (label, 'original raw custom-normal prefix changed')
    edge_by_key = {tuple(sorted(edge)): i for i, edge in enumerate(after['edges'])}
    assert len(edge_by_key) == len(after['edges']), (label, 'duplicate replacement edges')
    for name, values in before['selection_attributes'].items():
        actual = after['selection_attributes'][name]
        if SELECTION_ATTRIBUTES[name] == 'EDGE':
            assert all(actual[edge_by_key[tuple(sorted(edge))]] == value for edge, value in zip(before['edges'], values)), (label, 'edge selection changed', name)
        else:
            assert actual[:len(values)] == values, (label, 'selection prefix changed', name)
    if before['sharp_edges'] is not None:
        assert all(after['sharp_edges'][edge_by_key[tuple(sorted(edge))]] == value
                   for edge, value in zip(before['edges'], before['sharp_edges'])), (label, 'original sharp edges changed')
    elif after['sharp_edges'] is not None:
        assert not any(after['sharp_edges'][edge_by_key[tuple(sorted(edge))]]
                       for edge in before['edges']), (label, 'previously smooth source edge became sharp')


def append_service_pads(mesh, snapshot, source_island, contract):
    """Reuse an existing concrete slab's geometry/atlas islands for tiny infill.

    The new fields meet the porch and north path edge to edge. No new atlas,
    material, procedural source dependency or coplanar covering plane is added.
    """
    import bpy

    # Never resize the populated source mesh. Blender5.2.2 crashed in update()
    # after that operation with old edge/custom-normal buffers still attached.
    # Build an empty mesh via the shipped Mesh.from_pydata implementation and
    # the project's mesh_cleanup.py path, then bind UV/material/normal data.
    source_ids = set(source_island['vertices'])
    source_faces = [i for i, face in enumerate(snapshot['faces']) if set(face) <= source_ids]
    assert source_faces and all(set(face).isdisjoint(source_ids) or set(face) <= source_ids for face in snapshot['faces'])
    assert [loop.vertex_index for loop in mesh.loops] == snapshot['loop_vertices']
    assert [tuple(p.vertices) for p in mesh.polygons] == snapshot['faces']
    positions = [tuple(v.co) for v in mesh.vertices]  # Includes the planned cabinet translations.
    faces = list(snapshot['faces'])
    edges = list(snapshot['edges'])
    source_edge_ids = [i for i, edge in enumerate(edges) if set(edge) <= source_ids]
    assert all(set(edge).isdisjoint(source_ids) or set(edge) <= source_ids for edge in edges), 'donor has external loose edge'
    loop_vertices = list(snapshot['loop_vertices'])
    loop_starts, loop_totals = list(snapshot['loop_starts']), list(snapshot['loop_totals'])
    smooth, material_indices = list(snapshot['smooth']), list(snapshot['material_indices'])
    expected_normals = list(snapshot['corner_normals'])
    uvs = {name: list(values) for name, values in snapshot['uvs'].items()}
    selections = {name: list(values) for name, values in snapshot['selection_attributes'].items()}
    sharp_edges = list(snapshot['sharp_edges']) if snapshot['sharp_edges'] is not None else None
    source_box = source_island['bounds']
    added = []
    for label, surface in contract['surface_rectangles_blender_xy_m'].items():
        if not surface.get('new_geometry'):
            continue
        x0, x1, y0, y1 = surface['rect']
        target = [[x0, y0, -.12], [x1, y1, surface['top_z']]]
        scale = tuple((target[1][a] - target[0][a]) / (source_box[1][a] - source_box[0][a]) for a in range(3))
        vertices = {}
        for i in sorted(source_ids):
            old = snapshot['positions'][i]
            vertices[i] = len(positions)
            positions.append(tuple(target[0][a] + (old[a] - source_box[0][a]) * scale[a] for a in range(3)))
            for name, values in selections.items():
                if SELECTION_ATTRIBUTES[name] == 'POINT':
                    values.append(snapshot['selection_attributes'][name][i])
        for edge_index in source_edge_ids:
            edges.append(tuple(vertices[i] for i in snapshot['edges'][edge_index]))
            for name, values in selections.items():
                if SELECTION_ATTRIBUTES[name] == 'EDGE':
                    values.append(snapshot['selection_attributes'][name][edge_index])
            if sharp_edges is not None:
                sharp_edges.append(snapshot['sharp_edges'][edge_index])
        for face_index in source_faces:
            old_face = snapshot['faces'][face_index]
            faces.append(tuple(vertices[i] for i in old_face))
            loop_starts.append(len(loop_vertices))
            loop_totals.append(len(old_face))
            material_indices.append(snapshot['material_indices'][face_index])
            smooth.append(snapshot['smooth'][face_index])
            for name, values in selections.items():
                if SELECTION_ATTRIBUTES[name] == 'FACE':
                    values.append(snapshot['selection_attributes'][name][face_index])
            for corner, vertex in enumerate(old_face):
                old_loop = snapshot['loop_starts'][face_index] + corner
                assert snapshot['loop_vertices'][old_loop] == vertex
                loop_vertices.append(vertices[vertex])
                expected_normals.append(scaled_normal(snapshot['corner_normals'][old_loop], scale))
                for name, values in uvs.items():
                    values.append(snapshot['uvs'][name][old_loop])
                for name, values in selections.items():
                    if SELECTION_ATTRIBUTES[name] == 'CORNER':
                        values.append(snapshot['selection_attributes'][name][old_loop])
        added.append({'name': label, 'bounds_blender_m': target,
                      'vertices': len(vertices), 'triangles': len(source_faces),
                      'normal_transform': 'normalized inverse transpose of positive diagonal scale',
                      'scale_blender_m': scale})
    edge_keys = [tuple(sorted(edge)) for edge in edges]
    assert len(set(edge_keys)) == len(edge_keys), 'duplicate source/donor edge'
    new = bpy.data.meshes.new(mesh.name + '_Grounded')
    try:
        new.from_pydata(positions, edges, faces, shade_flat=False)
        assert [loop.vertex_index for loop in new.loops] == loop_vertices
        assert [p.loop_start for p in new.polygons] == loop_starts
        assert [p.loop_total for p in new.polygons] == loop_totals
        for material in mesh.materials:
            new.materials.append(material)
        new.polygons.foreach_set('material_index', material_indices)
        new.polygons.foreach_set('use_smooth', smooth)
        # Preserve explicit schema even if a whole attribute is default-valued.
        for name in ('material_index', 'sharp_face'):
            if name not in new.attributes:
                domain, kind = KNOWN_ATTRIBUTES[name]
                new.attributes.new(name=name, type=kind, domain=domain)
        new.attributes['material_index'].data.foreach_set('value', material_indices)
        new.attributes['sharp_face'].data.foreach_set('value', [not value for value in smooth])
        for name, values in uvs.items():
            layer = new.uv_layers.new(name=name)
            layer.data.foreach_set('uv', [c for v in values for c in v])
        edge_index = {key: i for i, key in enumerate(edge_keys)}
        actual_edge_keys = [tuple(sorted(edge.vertices)) for edge in new.edges]
        assert len(actual_edge_keys) == len(edge_keys) and set(actual_edge_keys) == set(edge_keys), 'edge connectivity changed'
        assert [tuple(edge.vertices) for edge in new.edges][:len(snapshot['edges'])] == snapshot['edges'], 'original edge order changed'
        if sharp_edges is not None:
            if 'sharp_edge' not in new.attributes:
                new.attributes.new(name='sharp_edge', type='BOOLEAN', domain='EDGE')
            new.attributes['sharp_edge'].data.foreach_set('value', [sharp_edges[edge_index[key]] for key in actual_edge_keys])
        # Blender5.2.2's setter averages/re-encodes smooth fans even on an exact
        # unchanged copy (measured A/B probe). The fresh-mesh raw-copy probe C
        # preserves every old corner exactly. Encode only new islands here;
        # old corners are placeholders until their original short2 data and
        # original sharp-edge semantics have both been restored.
        old_corners = len(snapshot['loop_vertices'])
        original_raw = snapshot['custom_normals_raw']
        assert original_raw is not None, 'source custom-normal buffer required'
        new.normals_split_custom_set([(0., 0., 0.)] * old_corners + expected_normals[old_corners:])
        normal_attribute = new.attributes['custom_normal']
        assert (normal_attribute.domain, normal_attribute.data_type) == ('CORNER', 'INT16_2D')
        encoded = [0] * (len(new.loops) * 2)
        normal_attribute.data.foreach_get('value', encoded)
        encoded[:len(original_raw)] = original_raw
        normal_attribute.data.foreach_set('value', encoded)
        assert [tuple(edge.vertices) for edge in new.edges][:len(snapshot['edges'])] == snapshot['edges'], 'normal encoding changed original edge order'
        if 'sharp_edge' in new.attributes:
            attribute = new.attributes['sharp_edge']
            flags = [bool(value.value) for value in attribute.data]
            for i in range(len(snapshot['edges'])):
                flags[i] = snapshot['sharp_edges'][i] if snapshot['sharp_edges'] is not None else False
            attribute.data.foreach_set('value', flags)
        for name, values in selections.items():
            domain = SELECTION_ATTRIBUTES[name]
            attribute = new.attributes.get(name) or new.attributes.new(name=name, type='BOOLEAN', domain=domain)
            assert attribute.domain == domain and attribute.data_type == 'BOOLEAN'
            if domain == 'EDGE':
                values = [values[edge_index[key]] for key in actual_edge_keys]
            attribute.data.foreach_set('value', values)
        new.update()
        after = mesh_snapshot(new, include_normals=False)
        check_source_prefix(snapshot, after, 'Office replacement before binding')
        assert len(after['positions']) == len(positions)
        assert all(abs(a - b) < 1e-5 for got, expected in zip(after['positions'], positions) for a, b in zip(got, expected))
        normal_check([tuple(normal.vector) for normal in new.corner_normals], expected_normals, 'Office replacement before binding')
    except Exception:
        bpy.data.meshes.remove(new)
        raise
    return new, added, expected_normals


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
        assert not objects[name].vertex_groups, (name, 'unsupported vertex groups')
        snap = mesh_snapshot(objects[name].data)
        assert snap['custom_normals_raw'] is not None, (name, 'original INT16_2D corner normals required')
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
    report['source_attribute_inventory'] = {
        name: {'attributes': before['attribute_schema'], 'color_attributes': [], 'shape_keys': False,
               'vertex_groups': [], 'selection_attributes_preserved': sorted(before['selection_attributes'])}
        for name, before in snapshots.items()}
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
    expected_normals = {name: before['corner_normals'] for name, before in snapshots.items()}
    office = objects['SM_Sierra_Office']
    old_mesh = office.data
    old_name, old_fake_user = old_mesh.name, old_mesh.use_fake_user
    assert len(bpy.data.meshes) == 24 and old_mesh.users == 1 + int(old_fake_user), 'Office mesh must have one real owner'
    replacement, pads, expected_normals['SM_Sierra_Office'] = append_service_pads(
        old_mesh, snapshots['SM_Sierra_Office'], donor, contract)
    # The complete replacement has already passed geometry/UV/material/normal
    # checks. Only now bind it, then retire the unreferenced in-memory old data.
    office.data = replacement
    replacement.use_fake_user = old_fake_user
    old_mesh.use_fake_user = False
    assert old_mesh.users == 0
    bpy.data.meshes.remove(old_mesh)
    replacement.name = old_name
    assert len(bpy.data.meshes) == 24, 'replacement must not leave a 25th mesh datablock'
    # The other four parts only translate whole islands. Their original short2
    # buffers and sharp flags remain untouched; passing them through the normal
    # setter would change authored directions even without topology changes.
    after_snapshots = {}
    report['corner_normal_preservation'] = {
        'method': 'Fresh empty Mesh.from_pydata; exact original INT16_2D normal prefix and edge semantics; setter only for new inverse-transpose donor normals',
        'max_component_tolerance': NORMAL_COMPONENT_EPS, 'min_normalized_dot': 1 - NORMAL_DOT_EPS,
        'in_memory': {}, 'reopened_saved_blend': {}}
    for name, before in snapshots.items():
        after = mesh_snapshot(objects[name].data, include_normals=False)
        after_snapshots[name] = after
        count = len(before['positions'])
        check_source_prefix(before, after, name)
        expected = list(before['positions'])
        for row in plans[name]:
            for i in row['vertices']:
                expected[i] = tuple(expected[i][a] + row['translation_blender_m'][a] for a in range(3))
        assert all(abs(after['positions'][i][a] - expected[i][a]) < 1e-5 for i in range(count) for a in range(3)), (name, 'unplanned vertex change')
        if name != 'SM_Sierra_Office':
            assert len(after['positions']) == count and len(after['faces']) == len(before['faces'])
        normals = [tuple(normal.vector) for normal in objects[name].data.corner_normals]
        old_corners = len(before['loop_vertices'])
        report['corner_normal_preservation']['in_memory'][name] = {
            'source_had_custom_normals': before['has_custom_normals'],
            'output_has_custom_normals': after['has_custom_normals'],
            'original_raw_int16_prefix_identical': after['custom_normals_raw'][:len(before['custom_normals_raw'])] == before['custom_normals_raw'],
            'original_edge_order_identical': after['edges'][:len(before['edges'])] == before['edges'],
            'original': normal_check(normals[:old_corners], expected_normals[name][:old_corners], name + ':original'),
            'added': normal_check(normals[old_corners:], expected_normals[name][old_corners:], name + ':added')}
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
    saved_blend = out / 'Sierra_Production.blend'
    temporary.replace(saved_blend)
    saved_hash = sha(saved_blend)
    # Acceptance reads the actual saved mesh/custom-normal data, not an
    # in-memory modifier result. Do not publish a success manifest before this.
    bpy.ops.wm.open_mainfile(filepath=str(saved_blend))
    saved_objects = {ob.name: ob for ob in bpy.data.objects}
    assert set(saved_objects) == set(manifest['parts'])
    assert len(bpy.data.meshes) == 24 and len(bpy.data.materials) == 24 and len(bpy.data.images) == 67
    for name, expected_snapshot in after_snapshots.items():
        mesh = saved_objects[name].data
        actual_snapshot = mesh_snapshot(mesh, include_normals=False)
        for key, expected_value in expected_snapshot.items():
            assert actual_snapshot[key] == expected_value, (name, 'saved source geometry/UV/material changed', key)
        normals = [tuple(normal.vector) for normal in mesh.corner_normals]
        old_corners = len(snapshots[name]['loop_vertices'])
        report['corner_normal_preservation']['reopened_saved_blend'][name] = {
            'original': normal_check(normals[:old_corners], expected_normals[name][:old_corners], name + ':saved-original'),
            'added': normal_check(normals[old_corners:], expected_normals[name][old_corners:], name + ':saved-added'),
            'raw_int16_and_sharp_flags_identical_to_verified_in_memory': True,
            'topology_uv_material_smoothing_identical': True}
    assert sha(saved_blend) == saved_hash, 'saved source verification changed output file'
    assert sha(package / 'Sierra_Production.blend') == report['source_blend_sha256'], 'immutable input changed'
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
