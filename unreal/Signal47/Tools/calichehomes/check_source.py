"""Lightweight source proof. This does not run Blender or prove native assets."""
import argparse
import importlib.util
import json
import math
from pathlib import Path

from design import CONTRACT, CONTRACT_PATH, HOUSES, PACKAGE, PARTS, TOOLS, front_x, sha, world
import home_geometry
from materials import material_rows


def bounds(points):
    return [[fn(p[k] for p in points) for k in range(3)] for fn in (min, max)]


def triangles(shape):
    # All authored profiles are convex. Keep these explicit source triangles in
    # Blender too, so DCC tessellation cannot choose a different n-gon diagonal.
    for face, coords, material, smooth, tag in zip(
            shape.f, shape.uv, shape.mat, shape.smooth, shape.tags):
        for i in range(1, len(face) - 1):
            yield ([face[k] for k in (0, i, i + 1)],
                   [coords[k] for k in (0, i, i + 1)], material, smooth, tag)


def projected_hit_x(a, b, c, y, z):
    """X of a triangle hit by a facade-normal ray; parallel triangles miss."""
    by, bz = b[1] - a[1], b[2] - a[2]
    cy, cz = c[1] - a[1], c[2] - a[2]
    det = by * cz - bz * cy
    if abs(det) < 1e-12:
        return None
    v = ((y - a[1]) * cz - (z - a[2]) * cy) / det
    w = (by * (z - a[2]) - bz * (y - a[1])) / det
    if min(v, w, 1 - v - w) < -1e-8:
        return None
    return a[0] + v * (b[0] - a[0]) + w * (c[0] - a[0])


def opening_obstructions(shapes, house, kind):
    """Test retained leaf/casing or old image, not just the nominal wall hole."""
    if kind == 'window':
        ys = [house['window_y_m'] + d for d in (-.74, -.37, 0, .37, .74)]
        zs = [.935, 1.08, 1.5, 1.92, 2.065]
    else:
        ys = [house['door_y_m'] + d for d in (-.55, -.27, 0, .27, .55)]
        zs = [.02, .14, .55, 1.05, 1.55, 2.08, 2.17]
    hits = []
    for shape in shapes:
        for ids, _, _, _, tag in triangles(shape):
            points = [shape.v[i] for i in ids]
            for y in ys:
                for z in zs:
                    x = projected_hit_x(*points, y, z)
                    if x is not None:
                        depth = (x - front_x(house)) * house['front']
                        # Images/leaf/casing occupy +.025..+.08. Anything from
                        # the facade to the outer sill would hide original art.
                        if -.005 < depth < .25:
                            hits.append(dict(part=shape.name, tag=tag, y=y, z=z, outward_m=depth))
    return hits


def legacy_check(export):
    export = Path(export).resolve()
    manifest = export / 'manifest.json'
    assert sha(manifest) == CONTRACT['original_manifest_sha256'], 'Original export changed'
    man = json.loads(manifest.read_text())
    spec = importlib.util.spec_from_file_location('s47_homes_read_glb', TOOLS.parent / 'freemodels' / 'trim_web.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    cache = {}

    def geo_points(geo):
        entry = next(r for r in man['geoFiles'] if r['from'] <= geo < r['to'])
        path = export / 'geo' / entry['file']
        if path not in cache:
            cache[path] = reader._load(path)
        doc, binary = cache[path]
        mesh = next(m for m in doc['meshes'] if m.get('name') == 'g' + str(geo))
        return [p for prim in mesh['primitives']
                for p in reader._values(doc, binary, prim['attributes']['POSITION'])]

    checked = []
    for home, house in HOUSES.items():
        node = man['nodes'][house['legacy_door_holder']]
        assert node.get('geo') is None, (home, 'door holder became a mesh')
        points = geo_points(house['legacy_door_leaf_geo'])
        lo, hi = bounds(points)
        # The original placeholder leaf is a plane in the Town group's Three
        # coordinates. Its licensed native leaf stays owned by that holder.
        dimensions = [b - a for a, b in zip(lo, hi)]
        assert max(abs(a - b) for a, b in zip(dimensions, [0, 2.1, .95])) < 1e-4, (home, dimensions)
        leaf_center = world(house, (front_x(house) + house['front'] * .025, house['door_y_m'], 0))
        got = [(lo[0] + hi[0]) / 2 + 8000, lo[1], (lo[2] + hi[2]) / 2]
        assert max(abs(a - b) for a, b in zip(got, leaf_center)) < 1e-4, (home, got, leaf_center)
        for handle in (house.get('window_panel_handle'), house.get('window_proxy_handle')):
            assert handle is None or handle in man['handles'], handle
        checked.append(home)
    return dict(status='PASS', original_manifest_sha256=sha(manifest), houses=checked,
                scope='Original leaf dimensions and protected handle identity; source geometry is read only')


def evaluate(export=None):
    known = {r['name'] for r in material_rows()}
    lods, checks = {}, []
    for lod in range(3):
        meshes = home_geometry.build(lod)
        assert set(meshes) == set(PARTS), 'Expected four houses, three parts each'
        rows = {}
        for name, shape in meshes.items():
            minimum_area, minimum_uv = math.inf, math.inf
            for face in shape.f:
                points = [shape.v[i] for i in face]
                normal = home_geometry.TOWN.cross(home_geometry.TOWN.sub(points[1], points[0]),
                                                 home_geometry.TOWN.sub(points[2], points[0]))
                turns = [home_geometry.TOWN.dot(home_geometry.TOWN.cross(
                    home_geometry.TOWN.sub(points[(i + 1) % len(points)], points[i]),
                    home_geometry.TOWN.sub(points[(i + 2) % len(points)], points[(i + 1) % len(points)])), normal)
                         for i in range(len(points))]
                assert min(turns) >= -1e-16, (name, lod, 'nonconvex profile needs explicit tessellation')
            for ids, coords, material, _, _ in triangles(shape):
                assert material in known, (name, material)
                a, b, c = [shape.v[i] for i in ids]
                normal = home_geometry.TOWN.cross(home_geometry.TOWN.sub(b, a), home_geometry.TOWN.sub(c, a))
                area = math.sqrt(home_geometry.TOWN.dot(normal, normal)) / 2
                u, v, w = coords
                uv_area = abs((v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])) / 2
                minimum_area, minimum_uv = min(minimum_area, area), min(minimum_uv, uv_area)
                assert area >= 5e-9 and uv_area > 1e-12, (name, lod, area, uv_area)
            assert all(math.isfinite(c) for v in shape.v for c in v), name
            rows[name] = dict(PARTS[name], bounds_local_m=bounds(shape.v),
                              vertices=len(set(shape.v)), vertices_before_weld=len(shape.v),
                              triangles=sum(len(f) - 2 for f in shape.f),
                              materials=[r['name'] for r in material_rows() if r['name'] in set(shape.mat)],
                              minimum_triangle_area_m2=minimum_area, minimum_uv_area=minimum_uv,
                              closed_solids=len(shape.closed_solids),
                              minimum_signed_volume_m3=min(s['signed_volume_m3'] for s in shape.closed_solids))
        lods[str(lod)] = rows
        for home, h in HOUSES.items():
            own = [m for name, m in meshes.items() if PARTS[name]['house'] == home]
            for kind in ('door', 'window'):
                bad = opening_obstructions(own, h, kind)
                assert not bad, (home, lod, kind, bad[:3])
                checks.append(f'{home} LOD{lod} retained {kind} clear')
            x0, x1, y0, y1 = h['footprint_blender_xy_m']
            corner_world = [world(h, p) for p in [(x0, y0, 0), (x1, y1, 0)]]
            road = h['original_road_rect_m']
            actual = [min(p[0] for p in corner_world) - 8000, max(p[0] for p in corner_world) - 8000,
                      min(p[2] for p in corner_world), max(p[2] for p in corner_world)]
            assert max(abs(a - b) for a, b in zip(actual, road)) < 1e-8, (home, actual, road)
            assert CONTRACT['canopy_underside_m'] - .015 > 2.45 + .105
    for name in PARTS:
        assert lods['0'][name]['triangles'] > lods['1'][name]['triangles'] > lods['2'][name]['triangles']
    for row in material_rows():
        assert all(Path(p).is_file() for p in row.get('textures', {}).values()), row['name']
    report = dict(status='PASS', scope='Pure Python source geometry only; DCC/native/visual UNVERIFIED',
                  source_revision=CONTRACT['source_revision'], source_contract_sha256=sha(CONTRACT_PATH),
                  authoring_code_sha256={path.name: sha(path) for path in sorted(TOOLS.glob('*.py'))},
                  parts=12, houses=4, lod_triangle_totals={k: sum(r['triangles'] for r in rows.values()) for k, rows in lods.items()},
                  closed_solid_totals={k: sum(r['closed_solids'] for r in rows.values()) for k, rows in lods.items()},
                  opening_checks=checks,
                  winding='Each emitted solid has two opposite uses per welded edge and positive signed volume')
    if export:
        report['legacy'] = legacy_check(export)
    plan = dict(status='PRIMITIVE_SOURCE_PLAN_DCC_NATIVE_UNVERIFIED', source_revision=CONTRACT['source_revision'],
                source_contract_sha256=sha(CONTRACT_PATH), native_import_policy=CONTRACT['native_policy'],
                parts=lods['0'], lods=lods)
    return report, plan


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', help='Optional immutable raw original web export directory')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--plan', type=Path)
    args = parser.parse_args()
    result, plan = evaluate(args.export)
    for path, value in ((args.report, result), (args.plan, plan)):
        if path:
            path.write_text(json.dumps(value, indent=2) + '\n')
    print(json.dumps(result, indent=2))
