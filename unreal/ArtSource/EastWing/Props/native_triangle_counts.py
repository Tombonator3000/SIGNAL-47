"""Predict UE render triangles from raw FBX, without launching Blender or Unreal.

Blender's importer calls mesh.validate(), which removes duplicate polygons even
when opposite winding was authored. UE preserves those faces. Never use a bpy
reimport's polygon count as UE's source count; retain the raw FBX polygon stream. Both the importer's null-area test and renderer's
repeated-corner test operate on float32 positions, even with cleanup disabled.
"""
import collections
import hashlib
import importlib
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import sys
import types

ROOT = Path(__file__).resolve().parent


def raw_parser():
    """Load only Blender's standalone FBX parser, not its bpy-dependent package."""
    override = os.environ.get('S47_FBX_PARSER_DIR')
    if override:
        addon = Path(override).expanduser().resolve()
    else:
        executable = Path(os.environ.get('BLENDER', str(Path.home() / '.local/bin/blender'))).expanduser().resolve()
        candidates = sorted(executable.parent.glob('*/scripts/addons_core/io_scene_fbx'))
        assert len(candidates) == 1, ('Set S47_FBX_PARSER_DIR to Blender io_scene_fbx directory', candidates)
        addon = candidates[0]
    assert (addon / 'parse_fbx.py').is_file(), addon
    package = types.ModuleType('_s47_raw_fbx')
    package.__path__ = [str(addon)]
    sys.modules[package.__name__] = package
    return importlib.import_module(package.__name__ + '.parse_fbx')


def properties(element):
    return {p.props[0]: tuple(p.props[4:]) for child in element.elems
            if child.id == b'Properties70' for p in child.elems if p.id == b'P'}


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def count_file(parser, path):
    root, version = parser.parse(str(path))
    assert version == 7400, (path.name, 'Unmeasured FBX version', version)
    objects = next(e for e in root.elems if e.id == b'Objects').elems
    settings = properties(next(e for e in root.elems if e.id == b'GlobalSettings'))
    # Current exporter and measured native importer contract: metres, Z up.
    expected = {b'UnitScaleFactor': (100.0,), b'UpAxis': (2,), b'UpAxisSign': (1,),
                b'FrontAxis': (1,), b'FrontAxisSign': (1,), b'CoordAxis': (0,), b'CoordAxisSign': (-1,)}
    assert all(settings.get(k) == v for k, v in expected.items()), (path.name, settings)
    models = {e.props[0]: e for e in objects if e.id == b'Model'}
    links = collections.defaultdict(list)
    for element in root.elems:
        if element.id == b'Connections':
            for c in element.elems:
                if len(c.props) >= 3 and c.props[0] == b'OO':
                    links[c.props[1]].append(c.props[2])
    total = removed = zero_area = duplicate_polygons = render_meshes = 0
    for geo in objects:
        if geo.id != b'Geometry' or geo.props[2] != b'Mesh':
            continue
        owners = [models[i] for i in links[geo.props[0]] if i in models]
        assert len(owners) == 1, (path.name, 'Geometry must have one model', geo.props)
        model = owners[0]
        # FBX Geometry names may be 'Cube.073'; UCX belongs to the connected Model.
        if model.props[1].startswith((b'UCX_', b'UBX_', b'USP_', b'UCP_')):
            continue
        assert not any(i in models for i in links[model.props[0]]), (path.name, 'Unmeasured model parent transform')
        props = properties(model)
        for key, value in props.items():
            if key in (b'Lcl Translation', b'GeometricTranslation', b'GeometricRotation',
                       b'PreRotation', b'PostRotation', b'RotationPivot', b'ScalingPivot',
                       b'RotationOffset', b'ScalingOffset'):
                assert value == (0., 0., 0.), (path.name, key, value)
            if key in (b'Lcl Scaling', b'GeometricScaling'):
                assert value == (1., 1., 1.), (path.name, key, value)
        rotation = props.get(b'Lcl Rotation', (0., 0., 0.))
        assert rotation == (0., -0., 180.00000500895632), (path.name, 'Unmeasured rotation', rotation)
        # Axis conversion and the importer's +90 yaw are a signed permutation.
        # Apply authored rotation in double precision, metre->cm, then UE float32.
        co, si = math.cos(math.radians(rotation[2])), math.sin(math.radians(rotation[2]))
        data = {e.id: e.props[0] for e in geo.elems if e.id in (b'Vertices', b'PolygonVertexIndex')}
        raw = data[b'Vertices']
        assert len(raw) % 3 == 0
        points = [(f32((co*x-si*y)*100), f32((si*x+co*y)*100), f32(z*100))
                  for x, y, z in zip(raw[::3], raw[1::3], raw[2::3])]
        assert all(math.isfinite(v) for point in points for v in point), path.name
        surfaces = collections.Counter()
        polygon = []
        for index in data[b'PolygonVertexIndex']:
            vertex = index if index >= 0 else ~index
            assert 0 <= vertex < len(points), (path.name, vertex)
            polygon.append(vertex)
            if index >= 0:
                continue
            assert len(polygon) == 3, (path.name, 'Delivery must already be triangulated', polygon)
            total += 1
            a, b, c = (points[i] for i in polygon)
            repeated = len({a, b, c}) < 3
            removed += repeated
            # FbxStaticMeshImport.cpp:853-886 rejects zero/NaN area normals
            # before MeshDescription, even with GetTriangleAreaThreshold()==0.
            # Match FVector3f subtraction, multiply, cross and squared length.
            u = [f32(b[i]-c[i]) for i in range(3)]
            v = [f32(a[i]-c[i]) for i in range(3)]
            cross = [f32(f32(u[(i+1)%3]*v[(i+2)%3])-f32(u[(i+2)%3]*v[(i+1)%3])) for i in range(3)]
            sq = f32(f32(f32(cross[0]*cross[0])+f32(cross[1]*cross[1]))+f32(cross[2]*cross[2]))
            invalid_area = sq == 0 or not math.isfinite(sq)
            assert not repeated or invalid_area, (path.name, polygon)
            zero_area += invalid_area
            surfaces[tuple(sorted(polygon))] += 1
            polygon = []
        assert not polygon, path.name
        duplicate_polygons += sum(n-1 for n in surfaces.values())
        render_meshes += 1
    assert render_meshes == 1 and total > 0, (path.name, render_meshes, total)
    return total, removed, zero_area, duplicate_polygons


def main():
    parser = raw_parser()
    spec = importlib.util.spec_from_file_location('eastwing_import', ROOT / 'import_unreal.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    plan = module.make_plan()
    validated = {(r['asset'], r['lod']): r for r in json.loads((ROOT / ('fbx-validation' + module.SUFFIX + '.json')).read_text())['checks']}
    rows = []
    for model in plan['models']:
        for lod, item in enumerate(model['lods']):
            path = Path(model['source']).parent / item['file']
            before = hashlib.sha256(path.read_bytes()).hexdigest()
            total, removed, zero_area, duplicate_polygons = count_file(parser, path)
            assert total == item['triangles'], (path.name, 'Raw FBX differs from authored manifest', total, item['triangles'])
            bpy_total = validated[(model['row']['asset'], lod)]['triangles']
            assert total - duplicate_polygons == bpy_total, (path.name, 'Unexplained bpy validation difference', total, duplicate_polygons, bpy_total)
            assert hashlib.sha256(path.read_bytes()).hexdigest() == before
            rows.append({'asset': model['row']['asset'], 'lod': lod, 'file': path.name,
                         'sha256': before, 'manifest_triangles': item['triangles'], 'source_triangles': total,
                         'bpy_validated_triangles': bpy_total, 'duplicate_index_polygons': duplicate_polygons,
                         'repeated_corner_triangles': removed, 'zero_area_triangles': zero_area,
                         'distinct_collinear_triangles': zero_area - removed, 'render_triangles': total - zero_area})
    expected = 3 * len(plan['models']) if module.EXTRAS else 78
    assert len(rows) == expected and len({(r['asset'], r['lod']) for r in rows}) == expected
    output = {'engine_rule': 'UE5.8.4 FbxStaticMeshImport: zero/NaN float32 area normal discarded with threshold0 before MeshDescription; StaticMeshBuilder additionally rejects repeated corner positions (already within zero-area set here)'
              ,'removal_stages': {'import': 'FbxStaticMeshImport.cpp853-886; GetTriangleAreaThreshold()==0; FVector3f cross product squared length zero/nonfinite', 'render': 'StaticMeshBuilder.cpp1644,1667-1675; repeated float32 positions with threshold0; subset of import removal for these sources'},
              'source_method': 'Raw binary FBX PolygonVertexIndex/Vertices via Blender standalone parser; UCX excluded through connected Model; no bpy mesh.validate()',
              'blender_difference': 'mesh.validate removes duplicate-index polygons regardless of winding; raw source minus those duplicates must exactly match previous bpy validation',
              'source_files_unchanged': True, 'lods': rows}
    (ROOT / ('native-triangle-counts' + module.SUFFIX + '.json')).write_text(json.dumps(output, indent=2) + '\n')
    print('EASTWING RAW FBX PREDICTION', len(rows), 'FBX files', sum(r['repeated_corner_triangles'] for r in rows),
          'zero-edge triangles;', sum(r['distinct_collinear_triangles'] for r in rows), 'additional null-area triangles;', sum(r['duplicate_index_polygons'] for r in rows), 'bpy-only duplicate polygon removals')


if __name__ == '__main__':
    main()
