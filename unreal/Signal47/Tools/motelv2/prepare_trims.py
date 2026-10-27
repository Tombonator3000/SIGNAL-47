"""Compose Motel V2 cuts from immutable web GLBs, including accepted room cuts.

Pure Python. Never launches Blender/Unreal or changes source/export/Content.
The output is input to convert_trims.py, then the narrow native importer.
"""
from __future__ import annotations
import argparse
import copy
import hashlib
import importlib.util
import json
import struct
from pathlib import Path
from clip_ground import clip as clip_pool_ground

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('original_trim', ROOT / 'Tools/freemodels/trim_web.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)
MOTEL_GEOS = {457: 672, 459: 674, 460: 675, 461: 676, 463: 678,
              464: 679, 467: 682, 468: 683, 470: 685}
DRESSING_GEOS = {528: 768, 529: 769}
# Geometry-local Three metres, before the unchanged MotelFront transform.
# Five centimetres padding includes caps/labels without touching the plant/ICE islands.
CUTS = {
    'legacy_base_car': [-36.45, -34.15, -.10, 1.75, 32.15, 37.05],
    'legacy_pool': [-34.75, -28.45, -.10, 1.45, 49.15, 61.85],
    'legacy_pickup': [-36.90, -30.90, -.10, 2.55, 42.425, 44.575],
    'legacy_sedan': [-37.4288, -31.7712, -.10, 2.55, 62.468, 64.732],
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def inside(point, box):
    return all(box[2 * a] - 1e-5 <= point[a] <= box[2 * a + 1] + 1e-5 for a in range(3))


def in_one_box(positions, triangle, boxes):
    return any(all(inside(positions[i], box) for i in triangle) for box in boxes)


def prior_cut(positions, triangle, recipe):
    only = recipe.get('keep_only_whole_triangles_inside', [])
    if only:
        return not in_one_box(positions, triangle, only)
    return (in_one_box(positions, triangle, recipe.get('remove_whole_triangles_inside', []))
            and not in_one_box(positions, triangle, recipe.get('preserve_whole_triangles_inside', [])))


def bounds(points):
    return [[op(p[axis] for p in points) for axis in range(3)] for op in (min, max)]


def verify_ground_holes(out, files, rows):
    """Independent point/triangle rays against the actual serialized float32 GLB."""
    def covers(point, a, b, c):
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if abs(denominator) < 1e-12:
            return False
        u = ((b[2] - c[2]) * (point[0] - c[0]) + (c[0] - b[0]) * (point[1] - c[2])) / denominator
        v = ((c[2] - a[2]) * (point[0] - c[0]) + (a[0] - c[0]) * (point[1] - c[2])) / denominator
        return min(u, v, 1 - u - v) >= -1e-8
    proof = []
    for filename in files:
        doc, binary = helper._load(out / filename)
        for mesh in doc['meshes']:
            name = mesh['name']
            if name not in ('g4', 'g663'):
                continue
            hole = rows[name]['ground_hole_local_xz_m']
            triangles = []
            for primitive in mesh['primitives']:
                pos = helper._values(doc, binary, primitive['attributes']['POSITION'])
                indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
                triangles.extend([[pos[k] for k in indices[i:i + 3]] for i in range(0, len(indices), 3)])
            interior = [(hole[0] + (hole[1] - hole[0]) * x / 6, hole[2] + (hole[3] - hole[2]) * z / 6)
                        for x in range(1, 6) for z in range(1, 6)]
            exterior = [(hole[0] - .05, (hole[2] + hole[3]) / 2), (hole[1] + .05, (hole[2] + hole[3]) / 2),
                        ((hole[0] + hole[1]) / 2, hole[2] - .05), ((hole[0] + hole[1]) / 2, hole[3] + .05)]
            assert all(not any(covers(p, *t) for t in triangles) for p in interior), (name, 'ground inside pool')
            assert all(any(covers(p, *t) for t in triangles) for p in exterior), (name, 'ground overcut outside pool')
            proof.append({'name': name, 'interior_25_rays_clear': True, 'outside_5cm_four_sides_preserved': True})
    assert {row['name'] for row in proof} == {'g4', 'g663'}
    return {'status': 'PASS', 'method': 'Serialized float32 GLB point/triangle rays, independent of clipper', 'meshes': proof}


def prepare(export, out, previous):
    manifest_bytes = (export / 'manifest.json').read_bytes()
    manifest = json.loads(manifest_bytes)
    room_path = previous / 'room-patches.json'
    room = json.loads(room_path.read_text())
    assert room['manifestSha256'] == sha(manifest_bytes), 'accepted room source differs'
    nodes = {n['i']: n for n in manifest['nodes']}
    expected = {**MOTEL_GEOS, **DRESSING_GEOS, 663: 953, 4: 7}
    out.mkdir(parents=True, exist_ok=True)
    # Preserve unrelated output files. Reruns only overwrite deterministic owned names.
    rows, files, source_hashes = {}, [], {}
    totals = {key: 0 for key in CUTS}
    for source in manifest['geoFiles']:
        if not any(source['from'] <= g < source['to'] for g in expected):
            continue
        source_path = export / 'geo' / source['file']
        source_hashes[source['file']] = sha(source_path.read_bytes())
        doc, original_binary = helper._load(source_path)
        binary = bytearray(original_binary)
        chosen = []
        for mi, mesh in enumerate(doc['meshes']):
            name = mesh.get('name', '')
            geo = int(name[1:]) if name.startswith('g') and name[1:].isdigit() else -1
            if geo not in expected:
                continue
            node = nodes[expected[geo]]
            if geo in (4, 663):
                ground_parent = 952 if geo == 663 else 6
                ground_material = 416 if geo == 663 else 5
                translation = [-24, 0, 300] if geo == 663 else [0, 0, 0]
                assert node['p'] == ground_parent and node['geo'] == geo and node['mats'] == [ground_material]
                assert nodes[ground_parent]['m'][12:15] == translation
                hole = [-61 - translation[0], -56.8 - translation[0], 669.8 - translation[2], 678 - translation[2]]
                ground = clip_pool_ground(doc, binary, mesh, helper, hole)
                indices = [v[0] for p in mesh['primitives'] for v in helper._values(doc, binary, p['indices'])]
                rows[name] = {**ground, 'node': node['i'], 'parent': ground_parent, 'geo': geo,
                              'accepted_previous_tris': [ground['source_tris']],
                              'asset': f'/Game/MotelV2/Trim/{name}.{name}', 'original_material_indices': [0],
                              'source_material_ids': [ground_material], 'hidden': False, 'fbx': f'exports/{name}.fbx',
                              'retained_index_sha256': sha(struct.pack('<' + 'I' * len(indices), *indices)),
                              'attribute_policy': 'Original buffer prefix preserved; clipped boundary attributes interpolated into appended arrays'}
                chosen.append(mi)
                continue
            parent = 663 if geo in MOTEL_GEOS else 767
            assert node['geo'] == geo and node['p'] == parent
            old = room['meshes'].get(name)
            assert (old is not None) == (geo in MOTEL_GEOS)
            recipe = old['recipe'] if old else {}
            slots, primitives, before, previous_indices, after, points = [], [], [], [], [], []
            removed_by = {key: 0 for key in CUTS}
            for pi, primitive in enumerate(mesh['primitives']):
                positions = helper._values(doc, binary, primitive['attributes']['POSITION'])
                indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
                assert len(indices) % 3 == 0
                before.extend(indices)
                retained = []
                for k in range(0, len(indices), 3):
                    triangle = indices[k:k + 3]
                    if prior_cut(positions, triangle, recipe):
                        continue
                    previous_indices.extend(triangle)
                    reason = next((key for key, box in CUTS.items()
                                   if in_one_box(positions, triangle, [box])), None)
                    if reason:
                        removed_by[reason] += 1
                    else:
                        retained.extend(triangle)
                        points.extend(positions[v] for v in triangle)
                after.extend(retained)
                if not retained:
                    continue
                binary.extend(b'\0' * (-len(binary) % 4))
                offset = len(binary)
                binary.extend(struct.pack('<' + 'I' * len(retained), *retained))
                doc['bufferViews'].append({'buffer': 0, 'byteOffset': offset,
                                          'byteLength': len(retained) * 4, 'target': 34963})
                accessor = copy.deepcopy(doc['accessors'][primitive['indices']])
                accessor.update(bufferView=len(doc['bufferViews']) - 1, byteOffset=0,
                                componentType=5125, count=len(retained),
                                min=[min(retained)], max=[max(retained)])
                doc['accessors'].append(accessor)
                primitive['indices'] = len(doc['accessors']) - 1
                primitives.append(primitive)
                slots.append(pi)
            original_count, previous_count, result = len(before) // 3, len(previous_indices) // 3, len(after) // 3
            if old:
                assert old['node'] == node['i'] and old['parent'] == parent
                assert old['tris'] == original_count and old['resultTris'] == previous_count, name
                assert old['retainedIndexSha256'] == sha(struct.pack('<' + 'I' * len(previous_indices), *previous_indices)), name
            if previous_count == result:
                continue  # Already-hidden g468 and unaffected leaves need no runtime patch.
            assert previous_count - result == sum(removed_by.values())
            mesh['primitives'] = primitives
            asset = '' if not result else f'/Game/MotelV2/Trim/{name}.{name}'
            row = {'node': node['i'], 'parent': parent, 'geo': geo,
                   'accepted_previous_tris': [previous_count], 'result_tris': result,
                   'asset': asset, 'original_material_indices': slots, 'hidden': result == 0,
                   'source_tris': original_count, 'source_material_ids': node['mats'],
                   'removed_by': removed_by,
                   'retained_index_sha256': sha(struct.pack('<' + 'I' * len(after), *after))}
            if result:
                row['bounds_three_m'] = bounds(points)
                row['bounds_local_m'] = bounds([(p[0], -p[2], p[1]) for p in points])
                row['fbx'] = f'exports/{name}.fbx'
                chosen.append(mi)
            rows[name] = row
            for key, count in removed_by.items():
                totals[key] += count
        if chosen:
            helper._write_part(doc, binary, chosen, str(out), source['file'])
            filename = 'trim_' + Path(source['file']).stem + '.glb'
            files.append(filename)
            check_doc, check_bin = helper._load(out / filename)
            assert check_bin[:len(original_binary)] == original_binary, 'immutable source attributes changed'
            for mesh in check_doc['meshes']:
                row = rows[mesh['name']]
                indices = [v[0] for p in mesh['primitives'] for v in helper._values(check_doc, check_bin, p['indices'])]
                assert len(indices) // 3 == row['result_tris']
                assert sha(struct.pack('<' + 'I' * len(indices), *indices)) == row['retained_index_sha256']
    assert all(count > 0 for count in totals.values()), ('empty intended removal', totals)
    assert all(r['geo'] != 530 for r in rows.values()), 'preserve ICE/office dressing'
    # Runtime receives only these changed leaves, after the existing room integration.
    payload = {'version': 1, 'status': 'PREPARED_NATIVE_UNVERIFIED',
               'manifest_sha256': sha(manifest_bytes), 'previous_room_sha256': sha(room_path.read_bytes()),
               'source_geometry_sha256': source_hashes, 'cut_boxes_three_m': CUTS,
               'meshes': rows, 'files': files, 'totals_removed': totals,
               'file_sha256': {f: sha((out / f).read_bytes()) for f in files},
               'ground_hole_verification': verify_ground_holes(out, files, rows),
               'patches': [{k: r[k] for k in ('node', 'geo', 'parent', 'accepted_previous_tris',
                            'result_tris', 'asset', 'original_material_indices', 'hidden')} for r in rows.values()],
               'preserved': ['all original buffer bytes; ground hole has explicitly interpolated boundary attributes', 'original detailed motel cars',
                             'live KELVORA televisions', 'ICE, plants and unrelated dressing', 'gameplay proxies']}
    (out / 'trim-manifest.json').write_text(json.dumps(payload, indent=2) + '\n')
    return payload


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('export', type=Path)
    parser.add_argument('out', type=Path)
    parser.add_argument('--previous', type=Path, default=ROOT / 'Content/TownBuildings')
    args = parser.parse_args()
    data = prepare(args.export, args.out, args.previous)
    print(json.dumps({'status': data['status'], 'patches': data['patches'],
                      'totals_removed': data['totals_removed']}, indent=2))
