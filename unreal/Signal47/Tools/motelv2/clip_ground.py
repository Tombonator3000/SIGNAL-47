"""Exact rectangular pool excavation in the original corridor ground triangles.

Each intersecting triangle is split against four rectangle planes. Positions,
UVs and colours are interpolated; normals are interpolated and normalized.
Unintersected triangles retain their original vertex indices and attributes.
"""
import copy
import math
import struct

HOLE = [-37.0, -32.8, 369.8, 378.0]  # Three X/Z, parent saroCorridor=(-24,0,300)


def area(vertices):
    if len(vertices) < 3:
        return 0.
    p = [v['POSITION'] for v in vertices]
    return abs(sum(p[i][0] * p[(i + 1) % len(p)][2] - p[(i + 1) % len(p)][0] * p[i][2]
                   for i in range(len(p)))) * .5


def split(polygon, axis, limit, sign):
    inside, outside = [], []
    if not polygon:
        return inside, outside
    for a, b in zip(polygon, polygon[1:] + polygon[:1]):
        da = sign * (a['POSITION'][axis] - limit)
        db = sign * (b['POSITION'][axis] - limit)
        (inside if da >= 0 else outside).append(a)
        if (da >= 0) != (db >= 0):
            t = da / (da - db)
            vertex = {key: tuple(x + t * (y - x) for x, y in zip(a[key], b[key]))
                      for key in a if key != '_index'}
            position = list(vertex['POSITION'])
            position[axis] = limit
            vertex['POSITION'] = tuple(position)
            if 'NORMAL' in vertex:
                n = vertex['NORMAL']
                length = math.sqrt(sum(x * x for x in n))
                if length > 1e-12:
                    vertex['NORMAL'] = tuple(x / length for x in n)
            inside.append(vertex)
            outside.append(vertex)
    return inside, outside


def append_accessor(doc, binary, values, original):
    binary.extend(b'\0' * (-len(binary) % 4))
    offset = len(binary)
    flat = [x for value in values for x in value]
    binary.extend(struct.pack('<' + 'f' * len(flat), *flat))
    doc['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(flat) * 4, 'target': 34962})
    accessor = copy.deepcopy(original)
    accessor.update(bufferView=len(doc['bufferViews']) - 1, byteOffset=0,
                    componentType=5126, count=len(values))
    accessor.pop('normalized', None)
    accessor.pop('min', None)
    accessor.pop('max', None)
    if accessor['type'] == 'VEC3':
        accessor['min'] = [min(v[a] for v in values) for a in range(3)]
        accessor['max'] = [max(v[a] for v in values) for a in range(3)]
    doc['accessors'].append(accessor)
    return len(doc['accessors']) - 1


def clip(doc, binary, mesh, helper, hole=HOLE):
    original_count = result_count = clipped_count = new_vertices = 0
    area_removed = 0.
    hole_heights = []
    kept_points = []
    planes = [(0, hole[0], 1), (0, hole[1], -1), (2, hole[2], 1), (2, hole[3], -1)]
    for primitive in mesh['primitives']:
        assert primitive.get('mode', 4) == 4
        arrays = {key: helper._values(doc, binary, accessor) for key, accessor in primitive['attributes'].items()}
        for key, accessor_id in primitive['attributes'].items():
            accessor = doc['accessors'][accessor_id]
            if accessor.get('normalized'):
                divisor = {5121: 255., 5123: 65535.}[accessor['componentType']]
                arrays[key] = [tuple(x / divisor for x in value) for value in arrays[key]]
        indices = [v[0] for v in helper._values(doc, binary, primitive['indices'])]
        original_count += len(indices) // 3
        result = []
        def index(vertex):
            if '_index' not in vertex:
                vertex['_index'] = len(arrays['POSITION'])
                for key in arrays:
                    arrays[key].append(vertex[key])
            return vertex['_index']
        old_vertex_count = len(arrays['POSITION'])
        for start in range(0, len(indices), 3):
            triangle = [{**{key: arrays[key][i] for key in arrays}, '_index': i} for i in indices[start:start + 3]]
            positions = [v['POSITION'] for v in triangle]
            if (max(p[0] for p in positions) <= hole[0] or min(p[0] for p in positions) >= hole[1]
                    or max(p[2] for p in positions) <= hole[2] or min(p[2] for p in positions) >= hole[3]):
                result.extend(indices[start:start + 3])
                continue
            candidate, outside = triangle, []
            for axis, limit, sign in planes:
                candidate, strip = split(candidate, axis, limit, sign)
                if area(strip) > 1e-10:
                    outside.append(strip)
            lost = area(candidate)
            if lost <= 1e-10:
                result.extend(indices[start:start + 3])
                continue
            clipped_count += 1
            area_removed += lost
            hole_heights.extend(v['POSITION'][1] for v in candidate)
            retained_area = 0.
            for polygon in outside:
                for i in range(1, len(polygon) - 1):
                    tri = [polygon[0], polygon[i], polygon[i + 1]]
                    projected = area(tri)
                    if projected > 1e-10:
                        retained_area += projected
                        result.extend(index(v) for v in tri)
            assert abs(area(triangle) - retained_area - lost) < 1e-6, 'polygon area not conserved'
        new_vertices += len(arrays['POSITION']) - old_vertex_count
        result_count += len(result) // 3
        kept_points.extend(arrays['POSITION'][i] for i in result)
        for key, accessor_id in list(primitive['attributes'].items()):
            primitive['attributes'][key] = append_accessor(doc, binary, arrays[key], doc['accessors'][accessor_id])
        binary.extend(b'\0' * (-len(binary) % 4))
        offset = len(binary)
        binary.extend(struct.pack('<' + 'I' * len(result), *result))
        doc['bufferViews'].append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(result) * 4, 'target': 34963})
        accessor = copy.deepcopy(doc['accessors'][primitive['indices']])
        accessor.update(bufferView=len(doc['bufferViews']) - 1, byteOffset=0, componentType=5125,
                        count=len(result), min=[min(result)], max=[max(result)])
        doc['accessors'].append(accessor)
        primitive['indices'] = len(doc['accessors']) - 1
    expected_area = (hole[1] - hole[0]) * (hole[3] - hole[2])
    assert abs(area_removed - expected_area) < .00001, ('complete exact pool opening', area_removed, expected_area)
    assert clipped_count > 0 and new_vertices > 0
    return {'source_tris': original_count, 'result_tris': result_count, 'ground_hole_local_xz_m': hole,
            'clipped_source_triangles': clipped_count, 'interpolated_vertices': new_vertices,
            'removed_area_m2': area_removed, 'expected_removed_area_m2': expected_area,
            'ground_hole_y_range': [min(hole_heights), max(hole_heights)],
            'bounds_three_m': [[fn(p[a] for p in kept_points) for a in range(3)] for fn in (min, max)],
            'bounds_local_m': [[fn(p[a] for p in [(p[0], -p[2], p[1]) for p in kept_points])
                                for a in range(3)] for fn in (min, max)]}
