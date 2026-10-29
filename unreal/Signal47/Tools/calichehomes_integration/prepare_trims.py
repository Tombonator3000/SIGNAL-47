"""Lag ENV02-kutt og to skiltkopier fra uendret eksport. Bare ren Python.

Eksisterende TB2/signal-union reproduseres og hashkontrolleres først. Ingen
TownBuildings-filer, import, prosesser eller nativeassets endres av verktøyet.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import math
import struct
from pathlib import Path

import schema as S

PROJECT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('homes_original_glb', PROJECT / 'Tools/freemodels/trim_web.py')
GLB = importlib.util.module_from_spec(spec)
spec.loader.exec_module(GLB)


def index_hash(values):
    return hashlib.sha256(struct.pack('<' + 'I' * len(values), *values)).hexdigest()


def write_glb(document, binary, path):
    binary = bytes(binary) + b'\0' * (-len(binary) % 4)
    document['buffers'] = [{'byteLength': len(binary)}]
    data = json.dumps(document, separators=(',', ':')).encode()
    data += b' ' * (-len(data) % 4)
    path.write_bytes(struct.pack('<III', 0x46546c67, 2, 28 + len(data) + len(binary))
                     + struct.pack('<II', len(data), 0x4e4f534a) + data
                     + struct.pack('<II', len(binary), 0x004e4942) + binary)


def append_accessor(document, binary, values, source, target=34962):
    binary.extend(b'\0' * (-len(binary) % 4))
    offset = len(binary)
    fmt = {5121: 'B', 5123: 'H', 5125: 'I', 5126: 'f'}[source['componentType']]
    flat = [value for row in values for value in row]
    raw = struct.pack('<' + fmt * len(flat), *flat)
    binary.extend(raw)
    document['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(raw), 'target': target})
    accessor = {key: value for key, value in source.items() if key not in ('bufferView', 'byteOffset', 'min', 'max')}
    accessor.update(bufferView=len(document['bufferViews']) - 1, byteOffset=0, count=len(values))
    if accessor['type'] in ('VEC3', 'SCALAR'):
        accessor['min'] = [min(row[a] for row in values) for a in range(len(values[0]))]
        accessor['max'] = [max(row[a] for row in values) for a in range(len(values[0]))]
    document['accessors'].append(accessor)
    return len(document['accessors']) - 1


def sign_selection(geo, positions, triangle, sign):
    if geo not in (999, 1000, 1001):
        return False
    return S.inside_triangle(positions, triangle,
                             [sign['x'] - 2.1, sign['x'] + 2.1, -20., 50.,
                              sign['old_z'] - .15, sign['old_z'] + .15])


def home_selection(geo, positions, triangle):
    if geo not in (1002, 1003, 1005, 1006, 1013):
        return None
    for name, house in S.HOUSES.items():
        shell, stoop = S.house_boxes(house)
        if geo == 1006 and S.inside_triangle(positions, triangle, stoop):
            continue
        if S.inside_triangle(positions, triangle, shell):
            return name
    return None


def same_rect(actual, expected):
    return len(actual) == 4 and all(math.isclose(a, b, abs_tol=1e-6) for a, b in zip(actual, expected))


def prepare_signs(document, binary, meshes, out):
    target = {'asset': {'version': '2.0', 'generator': 'SIGNAL47 ENV02 precise sign relocation'},
              'bufferViews': [], 'accessors': [], 'meshes': [],
              'materials': [{'name': name} for label in S.SIGNS for name in S.sign_materials(label)]}
    data, rows, proof = bytearray(), {}, {}
    for sign_index, (label, sign) in enumerate(S.SIGNS.items()):
        name = 'SM_Homes_Sign' + label
        old_ground = S.terrain_low(sign['x'], sign['old_z'])
        ground = S.terrain_low(sign['x'], sign['z'])
        primitive_rows, points, source_points = [], [], []
        expected_attributes = []
        contacts = [{'x': sign['x'] + dx, 'z': sign['z'], 'y': S.terrain_low(sign['x'] + dx, sign['z'])}
                    for dx in (0., -1.4, 1.4)]
        for slot, geo in enumerate((999, 1000, 1001)):
            mesh = meshes[geo]
            assert len(mesh['primitives']) == 1
            source = mesh['primitives'][0]
            arrays = {key: GLB._values(document, binary, value) for key, value in source['attributes'].items()}
            indices = [value[0] for value in GLB._values(document, binary, source['indices'])]
            selected = [indices[k:k + 3] for k in range(0, len(indices), 3)
                        if sign_selection(geo, arrays['POSITION'], indices[k:k + 3], sign)]
            assert len(selected) == (24, 12, 2)[slot], (name, 'sign source triangle identity', slot)
            used = sorted({i for triangle in selected for i in triangle})
            mapping = {old: i for i, old in enumerate(used)}
            attributes, expected = {}, {}
            for key, source_values in arrays.items():
                values = [source_values[i] for i in used]
                if key == 'POSITION':
                    transformed = []
                    for x, y, z in values:
                        dy = 0.
                        if slot == 0:
                            dx = -1.4 if x < sign['x'] else 1.4
                            assert abs(x - (sign['x'] + dx)) < .051
                            dy = S.terrain_low(sign['x'] + dx, sign['z']) - ground
                        transformed.append((x - sign['x'], y - old_ground + dy, z - sign['old_z']))
                    source_points.extend(values)
                    values = transformed
                    points.extend(values)
                accessor = document['accessors'][source['attributes'][key]]
                attributes[key] = append_accessor(target, data, values, accessor)
                expected[key] = values
            flat_indices = [mapping[i] for triangle in selected for i in triangle]
            index = append_accessor(target, data, [(i,) for i in flat_indices],
                                    {'componentType': 5125, 'type': 'SCALAR'}, 34963)
            primitive_rows.append({'attributes': attributes, 'indices': index, 'material': sign_index * 3 + slot, 'mode': 4})
            expected_attributes.append(expected)
        # The old posts really start at the predicted graded verge, rather than
        # an assumed zero. New poles each follow their own measured contact.
        assert abs(min(p[1] for p in source_points) - old_ground) < 2e-5
        target['meshes'].append({'name': name, 'primitives': primitive_rows})
        rows[name] = {'asset': f'{S.DEST}/{name}.{name}', 'triangles': 38, 'result_tris': 38,
                      'lod_triangles': [38], 'materials': S.sign_materials(label),
                      'original_material_indices': [0, 0, 0], 'source_nodes': [1448, 1449, 1450],
                      'source_material_ids': [638, 641, 642], 'site_world_three_m': [8000 + sign['x'], ground, sign['z']],
                      'parent_handle': 'world.town.group', 'bounds_three_m': S.bounds(points),
                      'bounds_local_m': S.blender_bounds(points), 'ground_samples': contacts,
                      'original_center_ground_y': old_ground, 'old_z': sign['old_z'], 'front_z': sign['front_z'],
                      'fbx': f'exports/{name}.fbx', 'normal_uv_color_policy': 'unchanged source attributes; position-only translation per post/back/face'}
        proof[name] = expected_attributes
    target['nodes'] = [{'name': mesh['name'], 'mesh': i} for i, mesh in enumerate(target['meshes'])]
    target['scenes'] = [{'nodes': list(range(len(target['nodes'])))}]
    target['scene'] = 0
    write_glb(target, data, out / 'signs.glb')
    # Re-read actual float32 bytes, including the atlas coordinates and normals.
    saved, buffer = GLB._load(out / 'signs.glb')
    checks = []
    for mesh in saved['meshes']:
        row = rows[mesh['name']]
        assert len(mesh['primitives']) == 3
        for slot, primitive in enumerate(mesh['primitives']):
            assert saved['materials'][primitive['material']]['name'] == row['materials'][slot]
            for key, accessor in primitive['attributes'].items():
                actual = GLB._values(saved, buffer, accessor)
                expected = proof[mesh['name']][slot][key]
                assert len(actual) == len(expected)
                if key == 'POSITION':
                    assert all(abs(a - b) < 2e-6 for got, want in zip(actual, expected) for a, b in zip(got, want))
                else:
                    assert actual == expected, (mesh['name'], 'serialized original sign attribute changed', key)
        metal = GLB._values(saved, buffer, mesh['primitives'][0]['attributes']['POSITION'])
        for sample in row['ground_samples'][1:]:
            foot = min(v[1] for v in metal if abs(v[0] - (sample['x'] - S.SIGNS[mesh['name'].removeprefix('SM_Homes_Sign')]['x'])) < .051)
            assert abs(foot + row['site_world_three_m'][1] - sample['y']) < 2e-5
        checks.append({'name': mesh['name'], 'pass': True, 'detail': '38 triangles, three stable slots, source normal/UV/color exact, both terrain feet within 0.02mm'})
    return rows, checks


def prepare(export, out, previous_path, contract_path):
    export, out = export.resolve(), out.resolve()
    assert out != export and not out.is_relative_to(export) and not export.is_relative_to(out)
    assert not out.is_relative_to(contract_path.resolve().parent), 'do not write the authored source package'
    assert S.sha(export / 'manifest.json') == S.MANIFEST_SHA256
    assert S.sha(export / 'geo/geo_008.glb') == S.GEO_SHA256
    source = json.loads((export / 'manifest.json').read_text())
    previous = json.loads(previous_path.read_text())
    contract = json.loads(contract_path.read_text())
    S.validate_source_contract(contract)
    assert previous['manifestSha256'] == S.MANIFEST_SHA256
    nodes = {node['i']: node for node in source['nodes']}
    assert source['handles']['world.town.group']['node'] == S.TOWN
    assert source['handles']['world.town.st']['node'] == S.PARENT
    assert nodes[S.TOWN]['m'][12:15] == [8000, 0, 0]
    for move in S.collider_moves():
        matches = [r for r in source['walks']['road']['colliders']
                   if same_rect([r['minX'], r['maxX'], r['minZ'], r['maxZ']], move['old'])]
        drive = [r for r in source['drive']['legs']['road']['obstacles'] if same_rect(r, move['old'])]
        assert len(matches) == len(drive) == 1, ('exact sign collider in both datasets', move)
    document, original_binary = GLB._load(export / 'geo/geo_008.glb')
    meshes = {int(mesh['name'][1:]): mesh for mesh in document['meshes']}
    signs, sign_checks = {}, []
    out.mkdir(parents=True, exist_ok=True)
    signs, sign_checks = prepare_signs(document, original_binary, meshes, out)
    binary = bytearray(original_binary)
    rows, chosen, removed_houses = {}, [], {name: 0 for name in S.HOUSES}
    source_old_boxes = [box for boxes in previous['boxes'].values() for box in boxes]
    for geo, (node_id, material, original_count, old_count, removal_count, final_count) in S.PATCHES.items():
        node, mesh = nodes[node_id], meshes[geo]
        assert (node['p'], node['geo'], node['mats']) == (S.PARENT, geo, [material])
        prior, retained, slots, kept_points, new_primitives = [], [], [], [], []
        house_count = sign_count = 0
        for slot, primitive in enumerate(mesh['primitives']):
            positions = GLB._values(document, original_binary, primitive['attributes']['POSITION'])
            indices = [value[0] for value in GLB._values(document, original_binary, primitive['indices'])]
            kept = []
            for start in range(0, len(indices), 3):
                triangle = indices[start:start + 3]
                old_cut = any(S.inside_triangle(positions, triangle, box) for box in source_old_boxes)
                home = home_selection(geo, positions, triangle)
                sign = any(sign_selection(geo, positions, triangle, row) for row in S.SIGNS.values())
                assert not (old_cut and (home or sign)), ('new removal overlaps accepted union', geo)
                if old_cut:
                    continue
                prior.extend(triangle)
                if home:
                    house_count += 1
                    removed_houses[home] += 1
                elif sign:
                    sign_count += 1
                else:
                    kept.extend(triangle)
                    kept_points.extend(positions[i] for i in triangle)
            retained.extend(kept)
            if kept:
                indices_accessor = append_accessor(document, binary, [(i,) for i in kept],
                                                   {'componentType': 5125, 'type': 'SCALAR'}, 34963)
                new_primitives.append({**primitive, 'indices': indices_accessor})
                slots.append(slot)
        old = previous['meshes']['g' + str(geo)]
        assert old['node'] == node_id and old['parent'] == S.PARENT and old['sourceMaterialIds'] == [material]
        assert old['tris'] == original_count and old['resultTris'] == old_count == len(prior) // 3
        assert index_hash(prior) == old['retainedIndexSha256'], ('old union index fingerprint differs', geo)
        assert house_count + sign_count == removal_count and len(retained) // 3 == final_count
        mesh['primitives'] = new_primitives
        name = 'g' + str(geo)
        row = {'node': node_id, 'geo': geo, 'parent': S.PARENT, 'source_tris': original_count,
               'accepted_previous_tris': [old_count], 'result_tris': final_count, 'lod_triangles': [final_count],
               'removed_houses': house_count, 'removed_signs': sign_count, 'hidden': final_count == 0,
               'asset': f'{S.DEST}/Trim/{name}.{name}' if final_count else '',
               'original_material_indices': slots, 'source_material_ids': [material],
               'materials': [f'M_Homes_Trim_{name}_slot{i}' for i in slots],
               'previous_retained_index_sha256': index_hash(prior), 'retained_index_sha256': index_hash(retained)}
        if kept_points:
            row.update(bounds_three_m=S.bounds(kept_points), bounds_local_m=S.blender_bounds(kept_points), fbx=f'exports/{name}.fbx')
            chosen.append(document['meshes'].index(mesh))
        rows[name] = row
    assert removed_houses == {name: 132 for name in S.HOUSES}, removed_houses
    assert sum(row['removed_houses'] for row in rows.values()) == 528
    assert sum(row['removed_signs'] for row in rows.values()) == 76
    assert 'g1007' not in rows, 'H02 original dark window remains'
    GLB._write_part(document, binary, chosen, str(out), 'geo_008.glb')
    actual, actual_binary = GLB._load(out / 'trim_geo_008.glb')
    assert actual_binary[:len(original_binary)] == original_binary, 'source normal/UV/position/color buffer changed'
    for mesh in actual['meshes']:
        indices = [value[0] for primitive in mesh['primitives'] for value in GLB._values(actual, actual_binary, primitive['indices'])]
        row = rows[mesh['name']]
        assert len(indices) // 3 == row['result_tris'] and index_hash(indices) == row['retained_index_sha256']
    protected = []
    for node_id in sorted(set(S.PROTECTED_NODES) | {source['handles'][key]['node'] for key in S.PROTECTED_HANDLES}):
        node = nodes[node_id]
        assert node_id not in {row['node'] for row in rows.values()}
        protected.append({key: node[key] for key in ('i', 'p', 'geo', 'mats', 'name') if key in node})
    files = ['trim_geo_008.glb', 'signs.glb']
    payload = {'version': S.VERSION, 'source_revision': S.REVISION, 'status': 'PREPARED_NATIVE_UNVERIFIED',
               'coordinates': 'GLB positions/bounds_three_m are Three metres (Y up); bounds_local_m are Blender (x,-z,y), then native UE (x,z,y)*100. House bounds are also Blender.',
               'manifest_sha256': S.MANIFEST_SHA256, 'manifest_sha1': S.sha(export / 'manifest.json', 'sha1'),
               'source_contract_sha256': S.sha(contract_path), 'town_patches_sha256': S.sha(previous_path),
               'town_patches_sha1': S.sha(previous_path, 'sha1'), 'source_geometry_sha256': {'geo_008.glb': S.GEO_SHA256},
               'meshes': rows, 'signs': signs, 'files': files, 'file_sha256': {name: S.sha(out / name) for name in files},
               'collider_moves': S.collider_moves(), 'protected_nodes': protected, 'protected_handles': list(S.PROTECTED_HANDLES),
               'removed_houses': removed_houses, 'removed_house_total': 528, 'removed_sign_total': 76,
               'source_verification': {'status': 'PASS', 'scope': 'Serialized source GLB/index/slot/normal/UV/color/feet only; native and gameplay unverified',
                                       'old_union_indices_match': True, 'original_buffer_prefix_identical': True, 'signs': sign_checks},
               'preserved': ['four original stoops and their analytical colliders', 'original H02 dark window g1007',
                             'all door subtrees and licensed door mesh g1075', 'live panel/proxy/light/people state',
                             'all Dressing including cars/mailboxes/roof antennas/coolers/vents/meters/downspouts']}
    (out / 'trim-manifest.json').write_text(json.dumps(payload, indent=2) + '\n')
    assert S.sha(export / 'geo/geo_008.glb') == S.GEO_SHA256 and S.sha(export / 'manifest.json') == S.MANIFEST_SHA256
    return payload


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('export', type=Path)
    parser.add_argument('out', type=Path)
    parser.add_argument('--town-patches', type=Path, default=PROJECT / 'Content/TownBuildings/town-patches.json')
    parser.add_argument('--source-contract', type=Path, default=PROJECT.parent / 'ArtSource/CalicheHomes/source-contract.json')
    args = parser.parse_args()
    result = prepare(args.export, args.out, args.town_patches, args.source_contract)
    print(json.dumps({'status': result['status'], 'house_triangles': result['removed_house_total'],
                      'sign_triangles': result['removed_sign_total'], 'meshes': len(result['meshes']), 'signs': len(result['signs'])}))
