"""Read this project's single-mesh binary FBX geometry and atlas UV0 only.

No FBX SDK, Blender or Unreal dependency. The fixed export/import profile is the
measured Blender (x,y,z) -> UE (100x,-100y,100z) contract, with UE's DirectX UV-V
flip. No axis/scale/winding search or tolerance-based face deletion is allowed.
"""
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path
import struct
import zlib

POSITION_TOLERANCE_CM = .0001
UV_TOLERANCE = .0000001


def require(condition, message):
    if not condition:
        raise ValueError(message)


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def binary_nodes(path):
    data = Path(path).read_bytes()
    require(data[:23] == b'Kaydara FBX Binary  \x00\x1a\x00', 'Expected binary FBX')
    version = struct.unpack_from('<I', data, 23)[0]
    require(version in (7400, 7500), 'Unreviewed binary FBX version: ' + str(version))
    header, header_size = ('<QQQB', 25) if version >= 7500 else ('<IIIB', 13)

    def node(offset):
        end, count, property_bytes, name_size = struct.unpack_from(header, data, offset)
        if end == 0:
            return None, offset + header_size
        require(offset < end <= len(data), 'Corrupt FBX node bounds')
        cursor = offset + header_size
        name = data[cursor:cursor + name_size].decode('ascii')
        cursor += name_size
        properties_start, properties = cursor, []
        for _ in range(count):
            kind = chr(data[cursor])
            cursor += 1
            if kind in 'YCFDIL':
                fmt = '<' + {'Y': 'h', 'C': '?', 'F': 'f', 'D': 'd', 'I': 'i', 'L': 'q'}[kind]
                value = struct.unpack_from(fmt, data, cursor)[0]
                cursor += struct.calcsize(fmt)
            elif kind in 'fdlibc':
                length, encoding, size = struct.unpack_from('<III', data, cursor)
                cursor += 12
                require(encoding in (0, 1), 'Unsupported FBX array encoding')
                raw = data[cursor:cursor + size]
                cursor += size
                if encoding:
                    raw = zlib.decompress(raw)
                fmt = {'f': 'f', 'd': 'd', 'l': 'q', 'i': 'i', 'b': '?', 'c': 'b'}[kind]
                require(len(raw) == length * struct.calcsize('<' + fmt), 'FBX array length mismatch')
                value = struct.unpack('<' + fmt * length, raw)
            elif kind in 'SR':
                size = struct.unpack_from('<I', data, cursor)[0]
                cursor += 4
                value = data[cursor:cursor + size]
                cursor += size
            else:
                raise ValueError('Unsupported FBX property type: ' + kind)
            properties.append(value)
        require(cursor - properties_start == property_bytes, 'FBX property byte count mismatch')
        children = []
        while cursor + header_size < end:
            child, cursor = node(cursor)
            if child is None:
                break
            children.append(child)
        return {'name': name, 'properties': properties, 'children': children}, end

    result, cursor = [], 27
    while cursor + header_size <= len(data):
        item, cursor = node(cursor)
        if item is None:
            break
        result.append(item)
    return result


def child(node, name):
    rows = [item for item in node['children'] if item['name'] == name]
    require(len(rows) == 1, 'Missing or ambiguous FBX node: ' + name)
    return rows[0]


def value(node, name):
    rows = child(node, name)['properties']
    require(len(rows) == 1, 'Unexpected FBX property shape: ' + name)
    return rows[0]


def read_geometry(path):
    nodes = {'children': binary_nodes(path)}
    settings = child(child(nodes, 'GlobalSettings'), 'Properties70')
    properties = {row['properties'][0]: row['properties'][4:] for row in settings['children'] if row['name'] == 'P'}
    profile = {b'UpAxis': [2], b'UpAxisSign': [1], b'FrontAxis': [1], b'FrontAxisSign': [1],
               b'CoordAxis': [0], b'CoordAxisSign': [-1], b'UnitScaleFactor': [100.]}
    require(all(properties.get(key) == expected for key, expected in profile.items()), 'FBX axis/unit export profile differs')
    objects = child(nodes, 'Objects')['children']
    meshes = [row for row in objects if row['name'] == 'Geometry' and row['properties'][2] == b'Mesh']
    models = [row for row in objects if row['name'] == 'Model' and row['properties'][2] == b'Mesh']
    require(len(meshes) == len(models) == 1, 'Only the reviewed single-mesh FBX is supported')
    props = {row['properties'][0]: row['properties'][4:] for row in child(models[0], 'Properties70')['children'] if row['name'] == 'P'}
    rotation = props.get(b'Lcl Rotation', [0., 0., 0.])
    require(len(rotation) == 3 and max(abs(a - b) for a, b in zip(rotation, (0., 0., 180.))) < .0001,
            'FBX model axis compensation differs')
    for key, default in ((b'Lcl Translation', [0., 0., 0.]), (b'Lcl Scaling', [1., 1., 1.]),
                          (b'GeometricTranslation', [0., 0., 0.]), (b'GeometricRotation', [0., 0., 0.]),
                          (b'GeometricScaling', [1., 1., 1.])):
        require(props.get(key, default) == default, 'Unreviewed FBX model transform: ' + key.decode())
    mesh = meshes[0]
    vertices, polygon_indices = value(mesh, 'Vertices'), value(mesh, 'PolygonVertexIndex')
    require(len(vertices) % 3 == 0 and all(math.isfinite(number) for number in vertices), 'Nonfinite or invalid source vertices')
    layers = [row for row in mesh['children'] if row['name'] == 'LayerElementUV']
    require(len(layers) == 1, 'Expected exactly the baked atlas UV layer')
    uv_layer = layers[0]
    require(value(uv_layer, 'Name') == b'S47_Atlas' and
            value(uv_layer, 'MappingInformationType') == b'ByPolygonVertex' and
            value(uv_layer, 'ReferenceInformationType') == b'IndexToDirect', 'Atlas UV mapping differs')
    uv, uv_indices = value(uv_layer, 'UV'), value(uv_layer, 'UVIndex')
    require(len(uv) % 2 == 0 and len(uv_indices) == len(polygon_indices) and
            all(math.isfinite(number) for number in uv), 'Nonfinite or invalid source atlas UVs')
    triangles, zero_indices, corners, points = [], [], [], []
    for corner_index, raw_index in enumerate(polygon_indices):
        index = raw_index if raw_index >= 0 else -raw_index - 1
        require(0 <= index < len(vertices) // 3 and 0 <= uv_indices[corner_index] < len(uv) // 2,
                'Source corner index out of range')
        point = tuple(vertices[index * 3:index * 3 + 3])
        u, v = uv[uv_indices[corner_index] * 2:uv_indices[corner_index] * 2 + 2]
        corners.append((100 * point[0], -100 * point[1], 100 * point[2], f32(u), f32(1. - f32(v))))
        points.append(point)
        if raw_index < 0:
            require(len(corners) == 3, 'Source FBX is not exactly triangulated')
            a, b, c = points
            ab, ac = [b[k] - a[k] for k in range(3)], [c[k] - a[k] for k in range(3)]
            cross = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
            zero = sum(number * number for number in cross) == 0.
            if zero:
                zero_indices.append(len(triangles))
            triangles.append({'source_id': len(triangles), 'corners': corners, 'exact_zero_area': zero})
            corners, points = [], []
    require(not corners, 'Unterminated source triangle')
    return {'fbx_sha256': hashlib.sha256(Path(path).read_bytes()).hexdigest(), 'triangles': triangles,
            'source_triangles': len(triangles), 'exact_zero_area_indices': zero_indices,
            'renderable_triangles': len(triangles) - len(zero_indices),
            'fixed_axes': 'Blender metres (x,y,z) to UE centimetres (100x,-100y,100z)',
            'uv0_convention': 'float32(U), float32(1-float32(V)); UE5.8 FbxStaticMeshImport.cpp:758'}


def correspond(source, native_triangles):
    """Complete ordered, oriented corner/UV correspondence; no reverse winding.

    FBX importer inserts surviving polygons in their source order. Native numeric
    triangle IDs may have holes, but their supplied order must be increasing.
    Only cyclic corner rotations preserve orientation and are accepted.
    """
    expected = [row for row in source['triangles'] if not row['exact_zero_area']]
    require(len(expected) == len(native_triangles), 'Finite source/native triangle count differs')
    identifiers = [row['id'] for row in native_triangles]
    require(identifiers == sorted(set(identifiers)), 'Native triangle IDs are not unique and increasing')
    maximum_position, maximum_uv, rotations = 0., 0., defaultdict(int)
    matched = []
    for original, actual in zip(expected, native_triangles):
        corners = actual['corners']
        require(len(corners) == 3 and all(len(point) == 5 and all(math.isfinite(v) for v in point) for point in corners),
                'Nonfinite or malformed native position/UV triangle')
        a, b, c = corners
        ab, ac = [b[k] - a[k] for k in range(3)], [c[k] - a[k] for k in range(3)]
        cross = (ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0])
        require(sum(value * value for value in cross) > 0., 'Native MeshDescription contains an exact zero-area triangle')
        accepted = None
        for rotation in range(3):
            position = max(abs(original['corners'][(index + rotation) % 3][axis] - corners[index][axis])
                           for index in range(3) for axis in range(3))
            uv = max(abs(original['corners'][(index + rotation) % 3][axis] - corners[index][axis])
                     for index in range(3) for axis in (3, 4))
            if position <= POSITION_TOLERANCE_CM and uv <= UV_TOLERANCE:
                accepted = position, uv, rotation
                break
        require(accepted is not None, 'Native triangle ' + str(actual['id']) + ' differs from finite FBX source face ' + str(original['source_id']))
        maximum_position = max(maximum_position, accepted[0])
        maximum_uv = max(maximum_uv, accepted[1])
        rotations[accepted[2]] += 1
        matched.append((original['source_id'], actual['id'], accepted[2]))
    return {'status': 'PASS_COMPLETE_FINITE_SOURCE_UV_CORRESPONDENCE',
            'source_fbx_sha256': source['fbx_sha256'], 'source_triangles': source['source_triangles'],
            'source_exact_zero_area_triangles': len(source['exact_zero_area_indices']),
            'source_exact_zero_area_indices_sha256': hashlib.sha256(json.dumps(source['exact_zero_area_indices']).encode()).hexdigest(),
            'native_mesh_description_triangles': len(native_triangles), 'matched_finite_triangles': len(matched),
            'oriented_corner_rotations': dict(rotations), 'reversed_winding_allowed': False,
            'maximum_position_error_cm': maximum_position, 'position_tolerance_cm': POSITION_TOLERANCE_CM,
            'maximum_uv0_error': maximum_uv, 'uv0_tolerance': UV_TOLERANCE,
            'correspondence_sha256': hashlib.sha256(json.dumps(matched).encode()).hexdigest(),
            'axes': source['fixed_axes'], 'uv0_convention': source['uv0_convention']}
