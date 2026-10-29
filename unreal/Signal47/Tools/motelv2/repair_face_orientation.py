"""R6: rett bare de identifiserte, innadvendte motelgulvøyene i en ny pakke.

Koordinatoren kjører Blender i guard-vindu. Dette verktøyet starter aldri en
prosess. --preflight-fbx kan kjøres i vanlig Python; --apply skriver separat
utdata, og --audit leser og sammenligner en allerede skrevet pakke med R5.
Ingen kildemodell, atlas, møbel, vegg eller kollisjonspolicy endres her.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import pickle
import re
import runpy
import shutil
import sys


ROOT = Path(__file__).resolve().parent
PACKAGE = ROOT.parents[2] / 'ArtSource/MotelV2'
LAYOUT = ROOT / 'runtime_layout.json'
ROUTE = ROOT.parents[1] / 'Source/Signal47/Private/S47MotelV2RouteTest.cpp'
REPAIR_ID = 'motel-floor-face-orientation-20261029-r6'
INPUT_MANIFEST = '71818fd0f3362febcfb7a605f5aa4e067bf8ffb3ae060bc39bbbd105fe72e175'
INPUT_FREEZE = '43bbb6bb97299091a34406d2cab764b918171d6458d7579d9645309f5d02700c'
CONTRACT_SHA = '98c2b3d764c654e209996f0f7e388b16518666acd32ced91d87ba4750ae37463'
LAYOUT_SHA = '222bca3fbb147e35aee9c06eb598be27edfdc5c7fd46b84bf3f3bcccf5bf8aae'
SITE = (-45.9, -2.0985683853163195, 674.9)
OFFSETS = ((0., 0.), (.035, 0.), (-.035, 0.), (0., .035), (0., -.035))

_spec = importlib.util.spec_from_file_location('motel_grounding_r5', ROOT / 'repair_grounding.py')
G = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(G)


def digest(value):
    """Stabil signatur uten en stor tekstkopi av hvert mesh-attributt."""
    if isinstance(value, dict):
        value = tuple((key, digest(item)) for key, item in sorted(value.items()))
    return hashlib.sha256(pickle.dumps(value, protocol=4)).hexdigest()


def signature(snapshot):
    # FLOAT_VECTOR-payload kontrolleres eksakt; dekodede retninger kontrolleres
    # separat med R5s målte float-/dot-grenser, også etter save/reopen.
    return {key: digest(value) for key, value in snapshot.items() if key != 'corner_normals'}


def expect_signature(actual, wanted, label):
    got = signature(actual)
    assert set(got) == set(wanted), (label, 'snapshot fields differ')
    assert all(got[key] == wanted[key] for key in wanted), (
        label, 'mesh data differs', [key for key in wanted if got[key] != wanted[key]])


def targets():
    """Eksakt eier/bounds-populasjon målt i R5, ikke et generelt box-filter."""
    result = {}

    def add(part, label, lo, hi, vertices=96, triangles=188):
        result.setdefault(part, []).append({'name': label, 'bounds': [list(lo), list(hi)],
                                           'vertices': vertices, 'triangles': triangles})

    for i in range(11):
        for j in range(13):
            x, y = -2.01 + .4 * i, -2.40 + .4 * j
            add('SM_Sierra_Office', f'office_tile_{i:02}_{j:02}',
                (x - .196, y - .196, 0), (x + .196, y + .196, .012))
    add('SM_Sierra_Office', 'office_underlay', (-2.22, -2.675, -.09), (2.22, 2.675, 0))
    owners = ([0] * 3 + [1] + [2] * 2 + [3] * 3 + [4] * 2 + [5] * 2 +
              [6] * 2 + [7] * 2 + [8] * 3)
    for k, owner in enumerate(owners):
        part = 'SM_Sierra_Office' if owner == 0 else f'SM_Sierra_Room{owner:02}'
        add(part, f'porch_slab_{k:02}', (2.295, .835 - 2.45 * k, -.17),
            (5.145, 3.265 - 2.45 * k, 0))
    for label, rect in [('office_threshold', (2.22, 2.295, .1, 1.5)),
                        ('service_west', (-.42, 2.295, 3.23, 3.4)),
                        ('service_bridge', (2.295, 5., 3.265, 3.4))]:
        x0, x1, y0, y1 = rect
        add('SM_Sierra_Office', label, (x0, y0, -.12), (x1, y1, 0))
    add('SM_Sierra_CourtSurface', 'asphalt', (5.1, -47., -.30), (15.7, 7., -.08))
    for label, rect in [('pool_east', (-10.9, -8., -8., 9.)),
                        ('pool_west', (-19., -15.1, -8., 9.)),
                        ('pool_north', (-15.1, -10.9, 5.1, 9.)),
                        ('pool_south', (-15.1, -10.9, -8., -3.1))]:
        x0, x1, y0, y1 = rect
        add('SM_Sierra_PoolDeck', label, (x0, y0, -.16), (x1, y1, .06))
    assert len(result) == 11 and sum(map(len, result.values())) == 172
    return result


def outward_approaches():
    return [{'name': label, 'bounds': [list(lo), list(hi)], 'vertices': 8, 'triangles': 12}
            for label, lo, hi in [
                ('west_pool_approach', (-7.9, -6.5, -.12), (-6.7, 5., 0)),
                ('north_pool_approach', (-8., 3.4, -.12), (5., 5., 0)),
                ('pool_gate_threshold', (-8., -6.5, -.12), (-7.8, -4.9, 0))]]


def island_inventory(positions, faces):
    islands = G.components(positions, faces)
    owner = {vertex: i for i, row in enumerate(islands) for vertex in row['vertices']}
    for row in islands:
        row['faces'] = []
    for i, face in enumerate(faces):
        assert len(face) == 3 and len(set(face)) == 3, ('nontriangle/degenerate indices', i)
        group = owner[face[0]]
        assert all(owner[v] == group for v in face)
        islands[group]['faces'].append(i)
    return islands


def topology(positions, faces, row):
    edges, oriented = Counter(), Counter()
    center = [(row['bounds'][0][a] + row['bounds'][1][a]) * .5 for a in range(3)]
    volumes = []
    for index in row['faces']:
        face = faces[index]
        for a, b in zip(face, face[1:] + face[:1]):
            edges[tuple(sorted((a, b)))] += 1
            oriented[(a, b)] += 1
        a, b, c = [tuple(positions[v][axis] - center[axis] for axis in range(3)) for v in face]
        volumes.append(sum(a[j] * (b[(j + 1) % 3] * c[(j + 2) % 3] -
                                   b[(j + 2) % 3] * c[(j + 1) % 3]) for j in range(3)) / 6)
    closed = bool(edges) and all(n == 2 for n in edges.values())
    consistent = closed and all(oriented[(a, b)] == oriented[(b, a)] == 1 for a, b in edges)
    return {'closed': closed, 'consistently_wound': consistent,
            'signed_volume_m3': math.fsum(volumes), 'edges': len(edges)}


def plan(part, positions, faces, wanted, positive=False):
    islands = island_inventory(positions, faces)
    chosen, used = [], set()
    for target in wanted:
        matches = [row for row in islands if G.close_bounds(row['bounds'], target['bounds'])]
        assert len(matches) == 1, (part, target['name'], 'exact complete island', len(matches))
        row = matches[0]
        assert len(row['vertices']) == target['vertices'] and len(row['faces']) == target['triangles'], (
            part, target['name'], 'unexpected island topology', len(row['vertices']), len(row['faces']))
        assert not used.intersection(row['faces']), (part, target['name'], 'duplicate target')
        used.update(row['faces'])
        state = topology(positions, faces, row)
        assert state['closed'] and state['consistently_wound'], (part, target['name'], state)
        box_volume = math.prod(row['bounds'][1][a] - row['bounds'][0][a] for a in range(3))
        volume = state['signed_volume_m3']
        assert .9 * box_volume < abs(volume) <= 1.001 * box_volume, (part, target['name'], 'volume differs', state)
        assert volume > 0 if positive else volume < 0, (part, target['name'], 'unexpected orientation', state)
        chosen.append({**row, **state, 'name': target['name']})
    return chosen


def flip_snapshot(before, face_ids):
    """(a,b,c)->(a,c,b); UV følger vertex, UV-kantvalg følger faktisk kant."""
    face_ids = set(face_ids)
    assert all(0 <= i < len(before['faces']) for i in face_ids)
    permutation = list(range(len(before['loop_vertices'])))
    edge_permutation = list(permutation)
    faces = list(before['faces'])
    for i in face_ids:
        a, b, c = faces[i]
        faces[i] = (a, c, b)
        start = before['loop_starts'][i]
        assert before['loop_totals'][i] == 3
        permutation[start:start + 3] = (start, start + 2, start + 1)
        edge_permutation[start:start + 3] = (start + 2, start + 1, start)
    flipped_loops = {before['loop_starts'][i] + j for i in face_ids for j in range(3)}
    after = dict(before)
    after['faces'] = faces
    after['loop_vertices'] = [before['loop_vertices'][i] for i in permutation]
    after['loop_edge_keys'] = [tuple(sorted((face[j], face[(j + 1) % 3])))
                               for face in faces for j in range(3)]
    after['uvs'] = {name: [values[i] for i in permutation] for name, values in before['uvs'].items()}
    after['selection_attributes'] = {
        name: [values[i] for i in (edge_permutation if name == '.uv_select_edge' else permutation)]
        if G.SELECTION_ATTRIBUTES[name] == 'CORNER' else values
        for name, values in before['selection_attributes'].items()}
    normals = [tuple(-v if loop in flipped_loops else v for v in before['corner_normals'][old])
               for loop, old in enumerate(permutation)]
    after.update(corner_normals=normals, has_custom_normals=True, normals_domain='CORNER',
                 attribute_schema={**before['attribute_schema'], 'custom_normal': ('CORNER', 'FLOAT_VECTOR')},
                 custom_normal_payload=[v for normal in normals for v in normal])
    after['custom_normal_sha256'] = G.normal_payload_hash(after['custom_normal_payload'], 'FLOAT_VECTOR')
    return after


def restore_attribute_defaults(mesh, expected, label):
    """Bevar faktisk seleksjon, og gjenopprett fravær etter from_pydata.

    Blender d13f752e3b9c, editors/mesh/mesh_data.cc:692-789 oppretter
    .select_vert/.select_edge/.select_poly med TRUE ved collections.add.
    RNA-getterne i makesrna/intern/rna_mesh.cc bruker FALSE ved fravær.
    Bare disse kjente fabrikkdataene nullstilles når R5 mangler attributtet;
    forfatterens eksisterende TRUE/FALSE-lister kopieres alltid uendret.
    """
    counts = {'POINT': len(expected['positions']), 'EDGE': len(expected['edges']),
              'FACE': len(expected['faces']), 'CORNER': len(expected['loop_vertices'])}
    receipt = {'authored_selection_preserved': {}, 'factory_defaults_removed': {}}

    def values(attribute, domain, data_type):
        assert (attribute.domain, attribute.data_type) == (domain, data_type), (
            label, attribute.name, 'generated attribute schema differs', attribute.domain, attribute.data_type)
        assert len(attribute.data) == counts[domain], (label, attribute.name, 'attribute count differs')
        result = [False if data_type == 'BOOLEAN' else 0] * counts[domain]
        attribute.data.foreach_get('value', result)
        return result

    for name, domain in G.SELECTION_ATTRIBUTES.items():
        attribute = mesh.attributes.get(name)
        wanted = expected['selection_attributes'].get(name)
        if wanted is not None:
            assert expected['attribute_schema'][name] == (domain, 'BOOLEAN')
            assert len(wanted) == counts[domain] and all(type(v) is bool for v in wanted), (
                label, name, 'invalid authored TRUE/FALSE payload')
            if attribute is None:
                attribute = mesh.attributes.new(name=name, domain=domain, type='BOOLEAN')
            values(attribute, domain, 'BOOLEAN')  # Validate before writing.
            attribute.data.foreach_set('value', wanted)
            assert values(attribute, domain, 'BOOLEAN') == wanted, (label, name, 'authored selection changed')
            receipt['authored_selection_preserved'][name] = {
                'domain': domain, 'count': len(wanted), 'true': sum(wanted),
                'false': len(wanted) - sum(wanted), 'payload_sha256': digest(wanted)}
        else:
            assert name not in expected['attribute_schema'], (label, name, 'missing authored selection payload')
            if attribute is None:
                continue
            generated = values(attribute, domain, 'BOOLEAN')
            defaults = [False] * counts[domain]
            attribute.data.foreach_set('value', defaults)
            assert values(attribute, domain, 'BOOLEAN') == defaults, (label, name, 'implicit FALSE default not restored')
            mesh.attributes.remove(attribute)
            assert mesh.attributes.get(name) is None, (label, name, 'source attribute absence not restored')
            receipt['factory_defaults_removed'][name] = {
                'domain': domain, 'count': len(generated), 'factory_true': sum(generated),
                'factory_false': len(generated) - sum(generated), 'restored_implicit_default': False}
    for attribute in list(mesh.attributes):
        if attribute.name in expected['attribute_schema']:
            continue
        name = attribute.name
        # These two known RNA setters create storage even when every value is
        # the implicit default. Never remove a nondefault material/smooth flag.
        assert name in {'material_index', 'sharp_face'}, (label, 'unexpected generated attribute', name)
        data_type = 'INT' if name == 'material_index' else 'BOOLEAN'
        wanted = expected['material_indices'] if name == 'material_index' else [not v for v in expected['smooth']]
        default = 0 if name == 'material_index' else False
        assert len(wanted) == counts['FACE'] and all(v == default for v in wanted), (
            label, name, 'source absence incompatible with nondefault authored values')
        assert values(attribute, 'FACE', data_type) == wanted, (label, name, 'generated values differ from source defaults')
        attribute.data.foreach_set('value', [default] * counts['FACE'])
        assert values(attribute, 'FACE', data_type) == wanted, (label, name, 'explicit default readback differs')
        mesh.attributes.remove(attribute)
        assert mesh.attributes.get(name) is None, (label, name, 'source attribute absence not restored')
        receipt['factory_defaults_removed'][name] = {
            'domain': 'FACE', 'count': counts['FACE'], 'restored_implicit_default': default}
    return receipt


def replace_mesh(bpy, ob, before, expected):
    """Ny tom mesh, ikke resizing/flip av et populert short2-normalbuffer."""
    old = ob.data
    old_name, fake = old.name, old.use_fake_user
    assert old.users == 1 + int(fake), (ob.name, 'mesh must have one real owner')
    new = bpy.data.meshes.new(old_name + '_OutwardFloors')
    try:
        new.from_pydata(expected['positions'], expected['edges'], expected['faces'], shade_flat=False)
        for material in old.materials:
            new.materials.append(material)
        new.polygons.foreach_set('material_index', expected['material_indices'])
        new.polygons.foreach_set('use_smooth', expected['smooth'])
        for name in ('material_index', 'sharp_face'):
            if name in before['attribute_schema'] and name not in new.attributes:
                domain, kind = before['attribute_schema'][name]
                new.attributes.new(name=name, domain=domain, type=kind)
        for name, values in expected['uvs'].items():
            layer = new.uv_layers.new(name=name)
            layer.data.foreach_set('uv', [v for uv in values for v in uv])
        assert [tuple(edge.vertices) for edge in new.edges] == expected['edges'], (ob.name, 'edge order changed')
        if expected['sharp_edges'] is not None:
            attribute = new.attributes.get('sharp_edge') or new.attributes.new(name='sharp_edge', domain='EDGE', type='BOOLEAN')
            attribute.data.foreach_set('value', expected['sharp_edges'])
        G.set_float_corner_normals(new, expected['corner_normals'])
        attribute_receipt = restore_attribute_defaults(new, expected, ob.name)
        new.update()
        expect_signature(G.mesh_snapshot(new, include_normals=False), signature(expected), ob.name + ':replacement')
        G.normal_check([tuple(n.vector) for n in new.corner_normals], expected['corner_normals'], ob.name + ':replacement')
    except Exception:
        bpy.data.meshes.remove(new)
        raise
    ob.data = new
    new.use_fake_user, old.use_fake_user = fake, False
    assert old.users == 0
    bpy.data.meshes.remove(old)
    new.name = old_name
    return attribute_receipt


def fbx_geometry(path):
    read = runpy.run_path(str(ROOT.parent / 'calichenature/verify_unreal.py'))['fbx_nodes']
    objects = next(n for n in read(path) if n['name'] == 'Objects')
    rows = [n for n in objects['children'] if n['name'] == 'Geometry' and n['properties'][-1] == 'Mesh']
    assert len(rows) == 1, (str(path), 'one exported mesh required')
    fields = {n['name']: n for n in rows[0]['children']}
    flat = fields['Vertices']['properties'][0]
    positions, faces, face = list(zip(flat[::3], flat[1::3], flat[2::3])), [], []
    for index in fields['PolygonVertexIndex']['properties'][0]:
        face.append(index if index >= 0 else -index - 1)
        if index < 0:
            assert len(face) == 3, (str(path), 'nontriangle export')
            faces.append(tuple(face))
            face = []
    assert not face

    def corners(layer, field, width):
        children = {n['name']: n for n in fields[layer]['children']}
        assert children['MappingInformationType']['properties'][0] == 'ByPolygonVertex', (path, layer, 'corner mapping')
        values = children[field]['properties'][0]
        direct = list(zip(*(values[a::width] for a in range(width))))
        reference = children['ReferenceInformationType']['properties'][0]
        if reference == 'Direct':
            result = direct
        else:
            assert reference == 'IndexToDirect'
            key = {'UV': 'UVIndex', 'Normals': 'NormalsIndex'}[field]
            result = [direct[i] for i in children[key]['properties'][0]]
        assert len(result) == len(faces) * 3, (path, layer, 'corner count')
        return result

    return {'positions': positions, 'faces': faces,
            'corner_normals': corners('LayerElementNormal', 'Normals', 3),
            'uvs': corners('LayerElementUV', 'UV', 2)}


def verify_export(path, spec, snapshot=None):
    actual = fbx_geometry(path)
    assert len(actual['faces']) == spec['triangles'], (path, 'actual FBX triangle count')
    box = G.bounds(actual['positions'], range(len(actual['positions'])))
    assert G.close_bounds(box, spec['bounds_local_m']), (path, 'actual FBX bounds', box)
    result = {'path': str(path), 'sha256': G.sha(path), 'triangles': len(actual['faces']), 'bounds_blender_m': box}
    if snapshot is not None:
        assert actual['positions'] == snapshot['positions'] and actual['faces'] == snapshot['faces'], (path, 'FBX changed geometry/winding')
        assert actual['uvs'] == snapshot['uvs']['UVMap'], (path, 'FBX changed corner UV')
        result['corner_normals'] = G.normal_check(actual['corner_normals'], snapshot['corner_normals'], str(path))
    return result


def read_inputs(args, report):
    package = args.package.resolve()
    assert G.sha(package / 'manifest.json') == INPUT_MANIFEST, 'requires frozen canonical R5 manifest'
    assert G.sha(package / 'package-files.json') == INPUT_FREEZE, 'requires frozen canonical R5 file list'
    frozen = json.loads((package / 'package-files.json').read_text())
    for name, wanted in frozen.items():
        assert G.sha(package / name) == wanted, ('immutable R5 input differs', name)
    assert G.sha(package / 'grounding-contract.json') == CONTRACT_SHA, 'grounding contract differs'
    assert G.sha(args.layout) == LAYOUT_SHA, '14-floor runtime contract differs'
    manifest = json.loads((package / 'manifest.json').read_text())
    assert len(manifest['parts']) == 24 and manifest['grounding_repair']['id'] == G.REPAIR_ID
    layout = json.loads(args.layout.read_text())
    assert len(layout['floors']) == 14
    source = args.route.read_text()
    block = re.search(r'static const FPoint Route\[\]\s*=\s*\{(.*?)\n\s*\};', source, re.S)
    assert block, 'physical route array not found'
    points = [(float(a), float(b)) for a, b in re.findall(r'\{\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*,', block[1])]
    assert len(points) == 58 and points[0] == (-44.8, 674.1), ('physical route differs', len(points))
    report.update(repair_id=REPAIR_ID, input_package=str(package),
                  input_manifest_sha256=INPUT_MANIFEST, input_file_list_sha256=INPUT_FREEZE,
                  input_blend_sha256=frozen['Sierra_Production.blend'], contract_sha256=CONTRACT_SHA,
                  runtime_layout_sha256=LAYOUT_SHA, physical_route_sha256=G.sha(args.route),
                  tool_sha256=G.sha(__file__), grounding_helper_sha256=G.sha(ROOT / 'repair_grounding.py'),
                  target_parts=sorted(targets()), target_islands=172, reversed_triangles=32336,
                  scope='Only 172 measured closed inward floor islands; native/gameplay/material appearance unverified')
    return package, frozen, manifest, layout, points


def scene_objects(bpy, manifest):
    objects = {ob.name: ob for ob in bpy.data.objects}
    assert set(objects) == set(manifest['parts'])
    assert all(ob.type == 'MESH' for ob in objects.values())
    assert len(bpy.data.meshes) == 24 and len(bpy.data.materials) == 24 and len(bpy.data.images) == 67
    for ob in objects.values():
        assert all(abs(ob.matrix_world[i][j] - int(i == j)) < 1e-6 for i in range(4) for j in range(4)), (ob.name, 'nonidentity world transform')
        assert not ob.vertex_groups and not ob.modifiers, (ob.name, 'unexpected geometry dependency')
    for image in bpy.data.images:
        assert image.packed_file and image.filepath.startswith('//textures/'), ('nonportable image', image.filepath)
    return objects


def summary(rows):
    return [{key: value for key, value in row.items() if key not in ('vertices', 'faces')} |
            {'vertices': len(row['vertices']), 'triangles': len(row['faces'])} for row in rows]


def surface_probes(objects, layout, points, wanted):
    """BVH av faktiske gulvtriangler, ingen nye flater eller tosidig aksept.

    Dette er kildebevis. Legacy-overflater, actual native collision/material og
    Walker må fortsatt prøves av koordinatorens uendrede fysiske runtime-rute.
    """
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree

    vertices, faces, identities = [], [], []
    centers = []
    for part, specifications in [*wanted.items(), ('SM_Sierra_CourtDetail', outward_approaches())]:
        mesh = objects[part].data
        coordinates = [tuple(v.co) for v in mesh.vertices]
        source_faces = [tuple(p.vertices) for p in mesh.polygons]
        rows = plan(part, coordinates, source_faces, specifications, positive=True)
        vertex_ids = sorted({v for row in rows for v in row['vertices']})
        mapping = {v: len(vertices) + i for i, v in enumerate(vertex_ids)}
        vertices.extend(coordinates[v] for v in vertex_ids)
        for row in rows:
            local = {v: i for i, v in enumerate(row['vertices'])}
            own_faces = [tuple(local[v] for v in source_faces[i]) for i in row['faces']]
            isolated = BVHTree.FromPolygons([coordinates[v] for v in row['vertices']], own_faces, all_triangles=True)
            low, high = row['bounds']
            x, y, top = (low[0] + high[0]) * .5, (low[1] + high[1]) * .5, high[2]
            location, normal, index, distance = isolated.ray_cast(Vector((x, y, top + .025)), Vector((0, 0, -1)), .225)
            assert location is not None and abs(location.z - top) < .0002 and normal.z > .999, (part, row['name'], 'top geometric face not outward', location, normal)
            source_face = row['faces'][index]
            normals = [tuple(mesh.corner_normals[mesh.polygons[source_face].loop_start + j].vector) for j in range(3)]
            assert all(n[2] > 0 for n in normals), (part, row['name'], 'top custom normal not upward', normals)
            centers.append({'part': part, 'island': row['name'], 'location_blender_m': list(location),
                            'geometric_normal': list(normal), 'minimum_corner_normal_up': min(n[2] for n in normals)})
            for face_id in row['faces']:
                faces.append(tuple(mapping[v] for v in source_faces[face_id]))
                identities.append((part, row['name'], face_id))
    tree = BVHTree.FromPolygons(vertices, faces, all_triangles=True)

    def probe(label, x, z, expected):
        rays, passes = [], 0
        for dx, dz in OFFSETS:
            origin = Vector((x + dx - SITE[0], SITE[2] - z - dz, expected - SITE[1] + .025))
            location, normal, index, distance = tree.ray_cast(origin, Vector((0, 0, -1)), .225)
            row = {'offset_three_xz_m': [dx, dz], 'hit': location is not None}
            if location is not None:
                height = location.z + SITE[1]
                up = normal.z
                okay = abs(height - expected) < .015 and up > 0
                passes += int(okay)
                row.update(part=identities[index][0], island=identities[index][1], source_face=identities[index][2],
                           height_three_m=height, expected_three_m=expected, geometric_normal=list(normal), passes=okay)
            rays.append(row)
        assert passes >= 3, (label, 'authored walking floor absent/inward', passes, rays)
        return {'label': label, 'three_xz_m': [x, z], 'expected_three_y_m': expected, 'passes': passes, 'rays': rays}

    def floor(x, z):
        for row in layout['floors']:
            x0, x1, z0, z1 = row['rect']
            if x0 <= x <= x1 and z0 <= z <= z1:
                return row['y']
        raise AssertionError(('route point outside 14 authored floors', x, z))

    # Representative interiors avoid the actual 2 cm porch joints. The office
    # underlay has its own exposed 14 mm rim; every repaired island is also
    # tested independently above, including all 143 tiles and the donor slab.
    samples = [(-17.5, 0), (-9.5, 0), (-13., -5.5), (-13., 7), (-7.3, 0), (-3., 4.2),
               (-7.9, -5.7), (3.9, -.4), (1.1, .8), (-2.21, 0), (2.2575, .8),
               (1., 3.315), (3.5, 3.3325), (10., -20.)]
    surface_rows = []
    for i, ((bx, by), row) in enumerate(zip(samples, layout['floors'])):
        x, z = SITE[0] + bx, SITE[2] - by
        x0, x1, z0, z1 = row['rect']
        assert x0 <= x <= x1 and z0 <= z <= z1, ('surface sample outside its floor', i)
        surface_rows.append(probe('surface_' + str(i) + '_' + row.get('name', 'pool'), x, z, row['y']))
    route_rows = [probe('route_' + str(i), x, z, floor(x, z)) for i, (x, z) in enumerate(points)]
    assert len(centers) == 175 and len(surface_rows) == 14 and len(route_rows) == 58
    return {'method': 'Blender BVHTree of actual selected triangles; upward winding required, same five offsets/15mm height/3-of-5 rule as native route',
            'scope_limit': 'Source floor topology only; no claim of native render/material/collision/legacy occlusion or human-input execution',
            'island_centers': centers, 'contract_surfaces': surface_rows, 'physical_route_waypoint_footprints': route_rows}


def preflight(args, report, package):
    rows = {}
    for part, specifications in [*targets().items(), ('SM_Sierra_CourtDetail', outward_approaches())]:
        geometry = fbx_geometry(package / 'exports' / (part + '.fbx'))
        positive = part == 'SM_Sierra_CourtDetail'
        chosen = plan(part, geometry['positions'], geometry['faces'], specifications, positive)
        rows[part] = summary(chosen)
        if not positive:
            faces = list(geometry['faces'])
            for row in chosen:
                for i in row['faces']:
                    a, b, c = faces[i]
                    faces[i] = (a, c, b)
            plan(part, geometry['positions'], faces, specifications, positive=True)
    report.update(status='PREFLIGHT_FBX_PASS_BLENDER_REOPEN_AND_NATIVE_UNVERIFIED', measured_targets=rows)


def export_lods(bpy, objects, manifest, out, report):
    bpy.context.window.scene = bpy.data.scenes['Sierra production exports']
    report['actual_exports'] = []
    for part in targets():
        ob, spec = objects[part], manifest['parts'][part]
        lods = []
        for lod, ratio in ((0, 1.), (1, .5), (2, .22)):
            lower = None
            try:
                selected = ob
                if lod:
                    lower = ob.copy()
                    lower.data = ob.data.copy()
                    lower.name = part + '_LOD' + str(lod)
                    bpy.context.scene.collection.objects.link(lower)
                    bpy.ops.object.select_all(action='DESELECT')
                    lower.select_set(True)
                    bpy.context.view_layer.objects.active = lower
                    modifier = lower.modifiers.new('Authored distance simplification', 'DECIMATE')
                    modifier.ratio, modifier.use_collapse_triangulate = ratio, True
                    bpy.ops.object.modifier_apply(modifier=modifier.name)
                    selected = lower
                    assert 0 < len(lower.data.polygons) < len(ob.data.polygons)
                snapshot = G.mesh_snapshot(selected.data)
                lod_spec = {'triangles': len(snapshot['faces']),
                            'bounds_local_m': G.bounds(snapshot['positions'], range(len(snapshot['positions']))),
                            'materials': spec['materials']}
                path = out / 'exports' / (selected.name + '.fbx')
                G.export_fbx(bpy, selected, path)
                report['actual_exports'].append(verify_export(path, lod_spec, snapshot))
                lods.append(lod_spec)
            finally:
                if lower is not None:
                    mesh = lower.data
                    bpy.data.objects.remove(lower, do_unlink=True)
                    bpy.data.meshes.remove(mesh)
                    for mesh in list(bpy.data.meshes):
                        if mesh.users == 0:
                            bpy.data.meshes.remove(mesh)
        spec.update(lods=lods, lod_triangles=[row['triangles'] for row in lods],
                    source_geometry_sha256=G.sha(out / 'exports' / (part + '.fbx')))


def preserved_files(package, out, frozen):
    changed = {f'exports/{part}{suffix}.fbx' for part in targets() for suffix in ('', '_LOD1', '_LOD2')}
    keep = {name: wanted for name, wanted in frozen.items()
            if name.startswith(('exports/', 'textures/')) and name not in changed}
    for name, wanted in keep.items():
        assert G.sha(out / name) == wanted == G.sha(package / name), ('unrelated payload changed', name)
    for name in ('materials.json', 'CREDITS.md', 'grounding-contract.json'):
        assert G.sha(out / name) == frozen[name], ('preserved metadata differs', name)
    return {'unchanged_export_files': sum(name.startswith('exports/') for name in keep),
            'unchanged_texture_files': sum(name.startswith('textures/') for name in keep),
            'file_sha256': keep}


def run(args, report):
    package, frozen, manifest, layout, points = read_inputs(args, report)
    if args.preflight_fbx:
        preflight(args, report, package)
        return
    import bpy

    bpy.ops.wm.open_mainfile(filepath=str(package / 'Sierra_Production.blend'))
    objects = scene_objects(bpy, manifest)
    expected_signatures, measured = {}, {}
    for part, ob in objects.items():
        before = G.mesh_snapshot(ob.data, include_normals=part in targets())
        assert len(before['faces']) == manifest['parts'][part]['triangles'], (part, 'manifest geometry count')
        if part in targets():
            assert before['custom_normal_payload'] is not None, (part, 'authored custom normals required')
            chosen = plan(part, before['positions'], before['faces'], targets()[part])
            measured[part] = summary(chosen)
            expected = flip_snapshot(before, [i for row in chosen for i in row['faces']])
            expected_signatures[part] = signature(expected)
            if args.apply:
                report.setdefault('replacement_attributes', {})[part] = replace_mesh(bpy, ob, before, expected)
        else:
            expected_signatures[part] = signature(before)
    report.update(blender_version=bpy.app.version_string, measured_targets=measured,
                  preservation_method='Exact all-field mesh signatures; only selected winding/corner order and normal sign differ; float corner normal conversion only on 11 changed objects')
    if not args.apply and args.audit is None:
        report['status'] = 'INVENTORY_PASS_APPLY_REOPEN_AND_NATIVE_UNVERIFIED'
        return
    out = (args.out if args.apply else args.audit).resolve()
    assert out != package and not out.is_relative_to(package) and not package.is_relative_to(out), 'separate output package required'
    if args.apply:
        assert not out.exists() or not any(out.iterdir()), 'output must be absent or empty; never overwrite an accepted package'
        out.mkdir(parents=True, exist_ok=True)
        for name in frozen:
            destination = out / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(package / name, destination)
        # A partial output cannot pass audit: it has no accepted file list or
        # orientation manifest until export, reopen and every BVH check passes.
        export_lods(bpy, objects, manifest, out, report)
        scene_objects(bpy, manifest)
        temporary = out / 'Sierra_Production.building.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(temporary), compress=True, relative_remap=False)
        temporary.replace(out / 'Sierra_Production.blend')
    else:
        accepted = json.loads((out / 'manifest.json').read_text())
        repair = accepted.get('face_orientation_repair', {})
        assert repair.get('id') == REPAIR_ID and repair.get('input_manifest_sha256') == INPUT_MANIFEST
        for key in ('input_blend_sha256', 'input_file_list_sha256', 'contract_sha256', 'runtime_layout_sha256',
                    'reversed_triangles', 'tool_sha256', 'grounding_helper_sha256'):
            assert repair.get(key) == report[key], ('output repair binding differs', key)
        assert repair.get('physical_route_sha256') == report['physical_route_sha256'], 'audit route differs'
        assert repair.get('changed_parts') == sorted(targets()) and repair.get('target_islands') == 172
        assert repair.get('measured_targets') == measured, 'accepted target inventory differs'
        output_freeze = json.loads((out / 'package-files.json').read_text())
        assert set(output_freeze) == set(frozen), 'output portable file list differs'
        for name, wanted in output_freeze.items():
            assert G.sha(out / name) == wanted, ('output freeze differs', name)
        assert set(accepted) == set(manifest) | {'face_orientation_repair'}
        for key in set(manifest) - {'parts', 'status'}:
            assert accepted[key] == manifest[key], ('original manifest field changed', key)
        assert set(accepted['parts']) == set(manifest['parts'])
        for part, original in manifest['parts'].items():
            actual = accepted['parts'][part]
            mutable = {'lods', 'lod_triangles', 'source_geometry_sha256'} if part in targets() else set()
            assert set(actual) == set(original)
            assert all(actual[key] == original[key] for key in set(original) - mutable), ('part metadata changed', part)
            assert actual['source_geometry_sha256'] == G.sha(out / 'exports' / (part + '.fbx')), (part, 'source FBX binding differs')
        for name in ('selection.json', 'provenance.json'):
            original = json.loads((package / name).read_text())
            actual = json.loads((out / name).read_text())
            assert actual.pop('face_orientation_repair') == repair
            assert actual == original, ('original metadata fields changed', name)
        manifest = accepted
    saved_sha = G.sha(out / 'Sierra_Production.blend')
    bpy.ops.wm.open_mainfile(filepath=str(out / 'Sierra_Production.blend'))
    objects = scene_objects(bpy, manifest)
    report['reopened_mesh_data'] = {}
    for part, ob in objects.items():
        actual = G.mesh_snapshot(ob.data, include_normals=part in targets())
        expect_signature(actual, expected_signatures[part], part + ':saved-reopen')
        output_islands = []
        if part in targets():
            assert actual['attribute_schema']['custom_normal'] == ('CORNER', 'FLOAT_VECTOR')
            G.normal_check(actual['corner_normals'], list(zip(*[iter(actual['custom_normal_payload'])] * 3)), part + ':saved-normal-directions')
            output_islands = summary(plan(part, actual['positions'], actual['faces'], targets()[part], positive=True))
            # Read back the actual LOD0 FBX and its corner UV/normal assignment.
            if args.audit is not None:
                report.setdefault('actual_exports', []).append(verify_export(out / 'exports' / (part + '.fbx'), manifest['parts'][part], actual))
        report['reopened_mesh_data'][part] = {'all_expected_field_hashes_match': True,
                                             'fields': expected_signatures[part],
                                             'outward_floor_islands': output_islands,
                                             'custom_normal_sha256': actual['custom_normal_sha256']}
    if args.audit is not None:
        for part in targets():
            for lod in (1, 2):
                report['actual_exports'].append(verify_export(out / 'exports' / f'{part}_LOD{lod}.fbx', manifest['parts'][part]['lods'][lod]))
    report['bvh_probes'] = surface_probes(objects, layout, points, targets())
    report['preserved_files'] = preserved_files(package, out, frozen)
    assert G.sha(out / 'Sierra_Production.blend') == saved_sha, 'read-only reopen/audit changed output blend'
    if args.apply:
        repair = {key: report[key] for key in ('repair_id', 'input_manifest_sha256', 'input_blend_sha256',
                                              'input_file_list_sha256', 'contract_sha256', 'runtime_layout_sha256',
                                              'physical_route_sha256', 'target_islands', 'reversed_triangles',
                                              'tool_sha256', 'grounding_helper_sha256', 'blender_version')}
        repair.update(id=repair.pop('repair_id'), changed_parts=sorted(targets()),
                      measured_targets=measured, status='SOURCE_REOPEN_BVH_PASS_NATIVE_UNVERIFIED',
                      rule='Whole closed floor islands reversed; (0,2,1) corner permutation; selected custom normals negated; all other source values preserved',
                      normal_storage='CORNER/FLOAT_VECTOR on 11 changed objects only; no texture bake/material/collision change')
        manifest['face_orientation_repair'] = repair
        manifest['status'] = 'SOURCE_ORIENTATION_REPAIRED_NATIVE_UNVERIFIED'
        for name in ('manifest.json', 'selection.json', 'provenance.json'):
            document = manifest if name == 'manifest.json' else json.loads((out / name).read_text())
            document['face_orientation_repair'] = repair
            (out / name).write_text(json.dumps(document, indent=2) + '\n')
        (out / 'package-files.json').write_text(json.dumps({name: G.sha(out / name) for name in frozen}, indent=2) + '\n')
    # Recheck every immutable input after all work, including exports/textures.
    for name, wanted in frozen.items():
        assert G.sha(package / name) == wanted, ('immutable R5 input changed during operation', name)
    report.update(status='SOURCE_REOPEN_BVH_PASS_NATIVE_UNVERIFIED' if args.apply else 'READ_ONLY_AUDIT_REOPEN_BVH_PASS_NATIVE_UNVERIFIED',
                  out=str(out), output_blend_sha256=saved_sha, output_manifest_sha256=G.sha(out / 'manifest.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, default=PACKAGE)
    parser.add_argument('--layout', type=Path, default=LAYOUT)
    parser.add_argument('--route', type=Path, default=ROUTE)
    parser.add_argument('--report', type=Path, required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--preflight-fbx', action='store_true')
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--audit', type=Path)
    parser.add_argument('--out', type=Path)
    arguments = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    args = parser.parse_args(arguments)
    assert not args.apply or args.out, '--apply requires --out'
    assert args.out is None or args.apply, '--out requires --apply'
    assert args.report.suffix == '.json' and not args.report.resolve().is_relative_to(args.package.resolve()), 'separate JSON report required'
    if args.out or args.audit:
        assert not args.report.resolve().is_relative_to((args.out or args.audit).resolve()), 'report must be outside portable output'
    report = {'schema': 1, 'status': 'FAIL', 'scope': 'Source-only R6; no engine jobs started'}
    try:
        run(args, report)
    except Exception as error:
        report['error'] = {'type': type(error).__name__, 'detail': str(error)}
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print('SIERRA_FACE_ORIENTATION', json.dumps({'status': report['status'], 'report': str(args.report)}), flush=True)


if __name__ == '__main__':
    main()
