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
import math
import struct
from pathlib import Path
import clip_ground as groundclip

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
# These are identified predecessor objects, not general spatial clean-up.
# In particular the three original pot islands and the pool pump are untouched.
GROUNDED_CUTS = {
    'legacy_ice': {'geos': {457, 459, 463, 464},
                   'boxes': [[-41.03, -40.15, -.01, 1.44, 27.65, 28.55]], 'triangles': 38},
    'legacy_front_substrate': {'geos': {457},
                               'boxes': [[-50.25, -27.65, -.72, .001, 21.55, 71.0]], 'triangles': 52},
    'legacy_porch_chairs': {'geos': {528},
                            'boxes': [[-41.02, -40.20, -.02, .90, 40.75, 41.5],
                                      [-41.02, -40.20, -.02, .90, 60.62, 61.35]], 'triangles': 872},
    'legacy_newspaper': {'geos': {528},
                         'boxes': [[-39.32, -38.78, -.02, 1.10, 26.32, 26.88]], 'triangles': 96},
    'legacy_replaced_prop_contacts': {'geos': {529},
                                      'boxes': [[-39.4002, -38.6998, .010, .016, 26.2248, 26.9752],
                                                [-41.0263, -40.0737, .010, .016, 40.6237, 41.5763],
                                                [-41.01185, -40.08815, .010, .016, 60.53815, 61.46185]], 'triangles': 6},
}
GROUNDING_CONTRACT = ROOT.parent / 'ArtSource/MotelV2/grounding-contract.json'


def disjoint_rectangles(rectangles):
    """Exact union as nonoverlapping rectangles for the proven single-hole clipper."""
    xs = sorted({x for r in rectangles for x in r[:2]})
    result = []
    for left, right in zip(xs, xs[1:]):
        ranges = sorted((r[2], r[3]) for r in rectangles if r[0] <= left and right <= r[1])
        merged = []
        for low, high in ranges:
            if merged and low <= merged[-1][1] + 1e-9:
                merged[-1][1] = max(high, merged[-1][1])
            else:
                merged.append([low, high])
        for low, high in merged:
            previous = next((r for r in result if abs(r[1] - left) < 1e-9 and r[2:] == [low, high]), None)
            if previous is None:
                result.append([left, right, low, high])
            else:
                previous[1] = right
    return result


def ground_openings(contract):
    rectangles = []
    for surface in contract['surface_rectangles_blender_xy_m'].values():
        x0, x1, y0, y1 = surface['rect']
        rectangles.append([-45.9 + x0, -45.9 + x1, 674.9 - y1, 674.9 - y0])
    return disjoint_rectangles(rectangles)


def polygon_surface_area(vertices):
    """Actual 3D polygon area, including old asphalt's vertical skirt faces."""
    if len(vertices) < 3:
        return 0.
    origin = vertices[0]['POSITION']
    normal = [0., 0., 0.]
    for i in range(1, len(vertices) - 1):
        a = [vertices[i]['POSITION'][k] - origin[k] for k in range(3)]
        b = [vertices[i + 1]['POSITION'][k] - origin[k] for k in range(3)]
        for k in range(3):
            normal[k] += a[(k + 1) % 3] * b[(k + 2) % 3] - a[(k + 2) % 3] * b[(k + 1) % 3]
    return math.sqrt(sum(v * v for v in normal)) * .5


def clip_native_footprints(doc, binary, mesh, holes, include_vertical_faces=False):
    """Clip the union in double precision, then serialize attributes only once.

    Repeatedly calling the single-hole clipper rounds each intermediate mesh to
    float32. Adjacent footprints can then drift across the common boundary.
    Use its tested polygon/attribute helpers without that intermediate write.
    """
    measure = polygon_surface_area if include_vertical_faces else groundclip.area
    source_count = result_count = clipped_count = new_vertices = 0
    removed_area, heights, kept_points = 0., [], []
    for primitive in mesh['primitives']:
        assert primitive.get('mode', 4) == 4
        arrays = {key: helper._values(doc, binary, accessor) for key, accessor in primitive['attributes'].items()}
        for key, accessor in primitive['attributes'].items():
            spec = doc['accessors'][accessor]
            if spec.get('normalized'):
                divisor = {5121: 255., 5123: 65535.}[spec['componentType']]
                arrays[key] = [tuple(c / divisor for c in value) for value in arrays[key]]
        indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
        source_count += len(indices) // 3
        result = []
        old_vertex_count = len(arrays['POSITION'])

        def index(vertex):
            if '_index' not in vertex:
                vertex['_index'] = len(arrays['POSITION'])
                for key in arrays:
                    arrays[key].append(vertex[key])
            return vertex['_index']

        for start in range(0, len(indices), 3):
            triangle = [{**{key: arrays[key][i] for key in arrays}, '_index': i} for i in indices[start:start + 3]]
            polygons, lost = [triangle], 0.
            for hole in holes:
                remaining = []
                for polygon in polygons:
                    positions = [v['POSITION'] for v in polygon]
                    if (max(p[0] for p in positions) <= hole[0] or min(p[0] for p in positions) >= hole[1]
                            or max(p[2] for p in positions) <= hole[2] or min(p[2] for p in positions) >= hole[3]):
                        remaining.append(polygon)
                        continue
                    candidate, outside = polygon, []
                    for axis, limit, sign in [(0, hole[0], 1), (0, hole[1], -1), (2, hole[2], 1), (2, hole[3], -1)]:
                        candidate, strip = groundclip.split(candidate, axis, limit, sign)
                        if measure(strip) > 1e-10:
                            outside.append(strip)
                    area = measure(candidate)
                    if area > 1e-10:
                        lost += area
                        heights.extend(v['POSITION'][1] for v in candidate)
                        remaining.extend(outside)
                    else:
                        remaining.append(polygon)
                polygons = remaining
            if lost <= 1e-10:
                result.extend(indices[start:start + 3])
                continue
            clipped_count += 1
            removed_area += lost
            retained_area = sum(measure(p) for p in polygons)
            assert abs(measure(triangle) - retained_area - lost) < 1e-6, 'union polygon area not conserved'
            for polygon in polygons:
                for k in range(1, len(polygon) - 1):
                    tri = [polygon[0], polygon[k], polygon[k + 1]]
                    if measure(tri) > 1e-10:
                        result.extend(index(v) for v in tri)
        new_vertices += len(arrays['POSITION']) - old_vertex_count
        result_count += len(result) // 3
        kept_points.extend(arrays['POSITION'][i] for i in result)
        for key, accessor in list(primitive['attributes'].items()):
            primitive['attributes'][key] = groundclip.append_accessor(doc, binary, arrays[key], doc['accessors'][accessor])
        binary.extend(b'\0' * (-len(binary) % 4))
        offset = len(binary)
        binary.extend(struct.pack('<' + 'I' * len(result), *result))
        doc['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(result) * 4, 'target': 34963})
        accessor = copy.deepcopy(doc['accessors'][primitive['indices']])
        accessor.update(bufferView=len(doc['bufferViews']) - 1, byteOffset=0, componentType=5125,
                        count=len(result), min=[min(result)], max=[max(result)])
        doc['accessors'].append(accessor)
        primitive['indices'] = len(doc['accessors']) - 1
    expected = sum((r[1] - r[0]) * (r[3] - r[2]) for r in holes)
    # The original ground also contains a second court surface in the same
    # merged mesh. Projected source triangle area can exceed the union area;
    # conservation is checked per source triangle and serialized rays below
    # prove that every remaining layer is clear inside the union.
    assert removed_area > 0
    assert clipped_count > 0 and new_vertices > 0
    return {'source_tris': source_count, 'result_tris': result_count, 'ground_holes_local_xz_m': holes,
            'clipped_source_triangles': clipped_count, 'interpolated_vertices': new_vertices,
            'removed_projected_source_area_m2': removed_area, 'opening_union_area_m2': expected,
            'ground_hole_y_range': [min(heights), max(heights)], 'bounds_three_m': bounds(kept_points),
            'bounds_local_m': bounds([(p[0], -p[2], p[1]) for p in kept_points])}


def removal_reason(geo, positions, triangle):
    for key, box in CUTS.items():
        if in_one_box(positions, triangle, [box]):
            return key
    for key, rule in GROUNDED_CUTS.items():
        if geo in rule['geos'] and in_one_box(positions, triangle, rule['boxes']):
            return key
    return None


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


def mesh_triangles(doc, binary, mesh):
    result = []
    for primitive in mesh['primitives']:
        positions = helper._values(doc, binary, primitive['attributes']['POSITION'])
        indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
        assert len(indices) % 3 == 0
        result.extend([[positions[k] for k in indices[i:i + 3]] for i in range(0, len(indices), 3)])
    return result


def vertical_heights(triangles, x, z):
    """Independent point/triangle query on serialized positions, not the clipper."""
    hits = []
    for a, b, c in triangles:
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if abs(denominator) < 1e-12:
            continue
        u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        if min(u, v, 1 - u - v) >= -1e-8:
            height = u * a[1] + v * b[1] + (1 - u - v) * c[1]
            if not any(abs(height - old) < 1e-7 for old in hits):
                hits.append(height)
    return sorted(hits)


def verify_asphalt(original, actual, holes, translation):
    """Retain the real road approach; clear every replaced asphalt layer.

    The original g456 contains two 12-triangle boxes, not a decorative plane.
    XZ cell boundaries include both old slabs and all native hole edges, so the
    samples test both sides of every retained/replaced boundary. Area checks
    independently bind complete top/bottom coverage to those rectangles.
    """
    assert len(original) == 24
    rectangles = []
    for start in (0, 12):
        lo, hi = bounds([p for triangle in original[start:start + 12] for p in triangle])
        rectangles.append([lo[0], hi[0], lo[2], hi[2]])
        assert abs(lo[1] + .62) < 1e-7 and abs(hi[1]) < 1e-7
    for i, a in enumerate(holes):
        for b in holes[i + 1:]:
            assert max(0., min(a[1], b[1]) - max(a[0], b[0])) * max(0., min(a[3], b[3]) - max(a[2], b[2])) < 1e-9

    def inside(x, z):
        return any(a <= x <= b and c <= z <= d for a, b, c, d in holes)

    covered = sum(max(0., min(a[1], b[1]) - max(a[0], b[0])) *
                  max(0., min(a[3], b[3]) - max(a[2], b[2])) for a in rectangles for b in holes)
    original_area = sum((b - a) * (d - c) for a, b, c, d in rectangles)
    remaining_area = original_area - covered
    area_checks = []
    for height in (0., -.62):
        measured = []
        for label, triangles in (('before', original), ('after', actual)):
            area = sum(groundclip.area([{'POSITION': p} for p in triangle]) for triangle in triangles
                       if all(abs(p[1] - height) < 1e-7 for p in triangle))
            expected = original_area if label == 'before' else remaining_area
            assert abs(area - expected) < .0001, ('asphalt layer coverage differs', height, label, area, expected)
            measured.append(area)
        area_checks.append({'local_y_m': height, 'before_m2': measured[0], 'after_m2': measured[1]})
    xs = sorted({v for r in rectangles for v in r[:2]} |
                {v for r in holes for v in r[:2] if min(a[0] for a in rectangles) < v < max(a[1] for a in rectangles)})
    zs = sorted({v for r in rectangles for v in r[2:]} |
                {v for r in holes for v in r[2:] if min(a[2] for a in rectangles) < v < max(a[3] for a in rectangles)})
    cleared = kept = empty = 0
    for x0, x1 in zip(xs, xs[1:]):
        for z0, z1 in zip(zs, zs[1:]):
            for fx in (.17, .5, .83):
                for fz in (.17, .5, .83):
                    x, z = x0 + (x1 - x0) * fx, z0 + (z1 - z0) * fz
                    before, after = vertical_heights(original, x, z), vertical_heights(actual, x, z)
                    if inside(x, z):
                        assert not after, ('old asphalt remains under native surface', x, z, after)
                        cleared += bool(before)
                    elif before:
                        assert len(before) == len(after) and max(abs(a - b) for a, b in zip(before, after)) < 1e-7, (
                            'road approach was overcut', x, z, before, after)
                        kept += 1
                    else:
                        assert not after, ('asphalt added outside its original footprint', x, z)
                        empty += 1
    assert cleared > 0 and kept > 0
    # A 2D ground-only clip leaves the old southern vertical sides standing
    # above the lowered native asphalt. Require the 3D cut for this slab alone.
    for triangle in actual:
        x, z = (sum(p[k] for p in triangle) / 3 for k in (0, 2))
        assert not any(a + .000002 < x < b - .000002 and c + .000002 < z < d - .000002
                       for a, b, c, d in holes), ('asphalt face remains inside native union', triangle)
    regression = []
    for dx, dz in ((0., 0.), (.035, 0.), (-.035, 0.), (0., .035), (0., -.035)):
        wx, wz = -42.002 + dx, 671.066 + dz
        x, z = wx - translation[0], wz - translation[2]
        before, after = vertical_heights(original, x, z), vertical_heights(actual, x, z)
        assert before and inside(x, z) and not after, ('waypoint11 old asphalt hit remains', wx, wz)
        regression.append({'three_xz_m': [wx, wz], 'old_hit_y_m': [y + translation[1] for y in before], 'new_hit_y_m': []})
    return {'status': 'PASS', 'method': 'Serialized float32 before/after point-triangle rays and independent slab/union area',
            'node': 671, 'geo': 456, 'parent': 663, 'source_material_ids': [320],
            'original_top_area_m2': original_area, 'native_overlap_top_area_m2': covered,
            'retained_approach_top_area_m2': remaining_area, 'layer_area_checks': area_checks,
            'cleared_inside_rays': cleared, 'preserved_outside_rays': kept, 'unchanged_empty_rays': empty,
            'waypoint11_footprint': regression}


def prepare_asphalt(export, out, manifest, nodes, openings):
    """Separate packet: existing ten patches and GLB bytes stay unchanged."""
    node, parent, root = nodes[671], nodes[663], nodes[662]
    assert node['geo'] == 456 and node['p'] == 663 and node['mats'] == [320]
    assert parent['p'] == 662 and root['name'] == 'MotelFront'
    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    assert node['m'] == parent['m'] == identity
    translation = root['m'][12:15]
    assert all(abs(a - b) < 1e-8 for a, b in zip(translation, [-2.5, -2.098568384594018, 650.]))
    assert root['m'][:12] == identity[:12] and root['m'][15] == 1
    source = next(r for r in manifest['geoFiles'] if r['from'] <= 456 < r['to'])
    source_path = export / 'geo' / source['file']
    raw = source_path.read_bytes()
    assert source['file'] == 'geo_003.glb' and sha(raw) == 'fd60fa957655d289cd05909020f2f39f85e8a8e9692f3c402d9418f9b3d1a5f9'
    doc, original_binary = helper._load(source_path)
    binary = bytearray(original_binary)
    index, mesh = next((i, m) for i, m in enumerate(doc['meshes']) if m.get('name') == 'g456')
    original = mesh_triangles(doc, original_binary, mesh)
    holes = [[a - translation[0], b - translation[0], c - translation[2], d - translation[2]] for a, b, c, d in openings]
    result = clip_native_footprints(doc, binary, mesh, holes, include_vertical_faces=True)
    assert result['source_tris'] == 24 and result['result_tris'] == 30
    result['removed_source_surface_area_m2'] = result.pop('removed_projected_source_area_m2')
    result['clip_area_metric'] = '3D polygon area; includes vertical old asphalt sides, not only its XZ projection'
    filename = 'trim_asphalt_geo_003.glb'
    helper._write_part(doc, binary, [index], str(out), 'asphalt_geo_003.glb')
    check_doc, check_binary = helper._load(out / filename)
    assert check_binary[:len(original_binary)] == original_binary, 'old asphalt source attributes changed'
    actual = check_doc['meshes'][0]
    indices = [v[0] for p in actual['primitives'] for v in helper._values(check_doc, check_binary, p['indices'])]
    assert len(indices) == 30 * 3
    row = {**result, 'node': 671, 'parent': 663, 'geo': 456, 'accepted_previous_tris': [24],
           'asset': '/Game/MotelV2/Trim/g456.g456', 'original_material_indices': [0],
           'source_material_ids': [320], 'hidden': False, 'fbx': 'exports/g456.fbx',
           'retained_index_sha256': sha(struct.pack('<' + 'I' * len(indices), *indices)),
           'attribute_policy': 'Original g456 buffer prefix preserved; clipped boundary normal/UV/position attributes appended',
           'source_parent_translation_three_m': translation}
    proof = verify_asphalt(original, mesh_triangles(check_doc, check_binary, actual), holes, translation)
    proof.update(source_geometry_sha256=sha(raw), original_buffer_prefix_identical=True,
                 ground_holes_local_xz_m=holes)
    return row, filename, proof


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
            holes = rows[name]['ground_holes_local_xz_m']
            triangles = []
            for primitive in mesh['primitives']:
                pos = helper._values(doc, binary, primitive['attributes']['POSITION'])
                indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
                triangles.extend([[pos[k] for k in indices[i:i + 3]] for i in range(0, len(indices), 3)])
            interior, exterior = [], []
            for hole in holes:
                interior.extend((hole[0] + (hole[1] - hole[0]) * x / 6, hole[2] + (hole[3] - hole[2]) * z / 6)
                                for x in range(1, 6) for z in range(1, 6))
                for p in [(hole[0] - .05, (hole[2] + hole[3]) / 2), (hole[1] + .05, (hole[2] + hole[3]) / 2),
                          ((hole[0] + hole[1]) / 2, hole[2] - .05), ((hole[0] + hole[1]) / 2, hole[3] + .05)]:
                    if not any(r[0] <= p[0] <= r[1] and r[2] <= p[1] <= r[3] for r in holes):
                        exterior.append(p)
            assert all(not any(covers(p, *t) for t in triangles) for p in interior), (name, 'ground under native surfaces')
            assert all(any(covers(p, *t) for t in triangles) for p in exterior), (name, 'ground overcut outside native surfaces')
            proof.append({'name': name, 'interior_rays_clear': len(interior),
                          'outside_5cm_rays_preserved': len(exterior), 'rectangles': len(holes)})
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
    contract_bytes = GROUNDING_CONTRACT.read_bytes()
    contract = json.loads(contract_bytes)
    assert contract['version'] == 1
    openings = ground_openings(contract)
    totals = {key: 0 for key in (*CUTS, *GROUNDED_CUTS)}
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
                holes = [[r[0] - translation[0], r[1] - translation[0],
                          r[2] - translation[2], r[3] - translation[2]] for r in openings]
                ground = clip_native_footprints(doc, binary, mesh, holes)
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
            removed_by = {key: 0 for key in totals}
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
                    reason = removal_reason(geo, positions, triangle)
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
    assert all(totals[key] == rule['triangles'] for key, rule in GROUNDED_CUTS.items()), ('identified predecessor triangle count changed', totals)
    assert all(r['geo'] != 530 for r in rows.values()), 'preserve ICE/office dressing'
    asphalt, filename, asphalt_proof = prepare_asphalt(export, out, manifest, nodes, openings)
    assert 'g456' not in rows and len(rows) == 10
    rows['g456'] = asphalt
    files.append(filename)
    assert source_hashes['geo_003.glb'] == asphalt_proof['source_geometry_sha256']
    # Runtime receives only these changed leaves, after the existing room integration.
    payload = {'version': 1, 'status': 'PREPARED_NATIVE_UNVERIFIED',
               'manifest_sha256': sha(manifest_bytes), 'previous_room_sha256': sha(room_path.read_bytes()),
               'source_geometry_sha256': source_hashes, 'cut_boxes_three_m': CUTS,
               'grounding_contract_sha256': sha(contract_bytes), 'ground_openings_world_xz_m': openings,
               'identified_predecessor_cuts': {key: {**rule, 'geos': sorted(rule['geos'])} for key, rule in GROUNDED_CUTS.items()},
               'meshes': rows, 'files': files, 'totals_removed': totals,
               'file_sha256': {f: sha((out / f).read_bytes()) for f in files},
               'ground_hole_verification': verify_ground_holes(out, files, rows),
               'legacy_asphalt_verification': asphalt_proof,
               'patches': [{k: r[k] for k in ('node', 'geo', 'parent', 'accepted_previous_tris',
                            'result_tris', 'asset', 'original_material_indices', 'hidden')} for r in rows.values()],
               'preserved': ['all original buffer bytes; ground hole has explicitly interpolated boundary attributes', 'original detailed motel cars',
                             'live KELVORA televisions', 'all plants and unrelated dressing', 'gameplay proxies']}
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
