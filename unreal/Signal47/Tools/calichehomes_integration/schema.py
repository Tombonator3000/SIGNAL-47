"""ENV02: presise kildeidentiteter, koordinater og uavhengig aktiveringsschema.

Ingen DCC-/Unreal-avhengighet. TB2s 28-delskontrakt endres ikke.
"""
from __future__ import annotations

import hashlib
import math
from pathlib import Path

VERSION = 1
REVISION = 'caliche-homes-20261029-v1'
MANIFEST_SHA256 = '69fcf95db3adf8eff76274f59b03f94e662936a147f1ee7389630c1a8a0925c9'
GEO_SHA256 = '0e87d2acf6bbe9044304423d096ea75f7c731dd1af15c545b2d79691e5e2bfee'
PARENT = 1432
TOWN = 1431
DEST = '/Game/CalicheHomes'
HOUSES = {
    'H01': {'rect': [24., 33., 2128., 2138.], 'front': -1, 'door_z': 2136.4,
            'root': [8028.5, 7.314197063446045, 2133.]},
    'H02': {'rect': [24.5, 32., 2152., 2161.], 'front': -1, 'door_z': 2159.4,
            'root': [8028.25, 8.003066062927246, 2156.5]},
    'H03': {'rect': [25., 34., 2180., 2190.], 'front': -1, 'door_z': 2181.6,
            'root': [8029.5, 8.81874942779541, 2185.]},
    'H04': {'rect': [-28., -19., 2236., 2246.], 'front': 1, 'door_z': 2244.4,
            'root': [7976.5, 11.268553733825684, 2241.]},
}
# node, original material, immutable source tris, current TB2+signal tris,
# this module's house/sign removal, final tris. Never accept an unknown prior cut.
PATCHES = {
    999: (1448, 638, 1916, 1404, 48, 1356),
    1000: (1449, 641, 512, 132, 24, 108),
    1001: (1450, 642, 30, 8, 4, 4),
    1002: (1451, 639, 754, 332, 192, 140),
    1003: (1452, 643, 180, 48, 48, 0),
    1005: (1454, 645, 240, 120, 120, 0),
    1006: (1455, 646, 360, 154, 48, 106),
    1013: (1462, 652, 300, 120, 120, 0),
}
SIGNS = {
    'North': {'x': -8.6, 'old_z': 1880., 'z': 1895., 'front_z': -1},
    'South': {'x': 8.6, 'old_z': 2335., 'z': 2315., 'front_z': 1},
}
PROTECTED_NODES = [1436, 1437, 1438, 1439, 1440, 1441, 1442, 1443, 1444,
                   1445, 1446, 1447, 1534, 1535, 1536, 1537, 1538, 1539,
                   1546, 1547, 1551, 1552, 1553, 1554, 1563, 1594, 1595, 1596]
PROTECTED_HANDLES = ['world.town.car', 'world.town.carDoor', 'world.town.woman',
                     'world.town.smoker', 'world.town.paper', 'world.town.cutout',
                     'world.town.proxies.tv1', 'world.town.proxies.tv2',
                     'world.town.proxies.kitchen', 'world.town.proxies.smoker']


def sha(path, algorithm='sha256'):
    digest = hashlib.new(algorithm)
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(4 * 1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def part_names():
    return {f'SM_Homes_{home}_{role}' for home in HOUSES for role in ('Shell', 'Roof', 'Fixtures')}


def sign_materials(label):
    return [f'M_Homes_Sign{label}_slot{i}' for i in range(3)]


def bounds(points):
    return [[op(p[axis] for p in points) for axis in range(3)] for op in (min, max)]


def blender_bounds(points):
    return bounds([(p[0], -p[2], p[1]) for p in points])


def inside_triangle(positions, triangle, box):
    """Alle tre hjørner må ligge i samme prisme, ikke bare i en union."""
    return all(all(box[axis * 2] <= positions[i][axis] <= box[axis * 2 + 1]
                   for axis in range(3)) for i in triangle)


def house_boxes(home):
    x0, x1, z0, z1 = home['rect']
    fx = x1 if home['front'] > 0 else x0
    other = fx + home['front'] * .9
    # 2 mm includes source float32 z coordinates at 2.2 km, without taking
    # neighbouring dressing. Preserve the entire stoop, including its back face.
    return ([x0 - .1, x1 + .1, -20., 50., z0 - .1, z1 + .1],
            [min(fx, other) - .002, max(fx, other) + .002, -20., 50.,
             home['door_z'] - .652, home['door_z'] + .652])


def smooth(a, b, x):
    t = min(1., max(0., (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def terrain_low(x, z):
    """S47RoadShape::HLow; signs lie on HWY.Flat's graded, unbumped verge.

    Runtime must also compare this prediction to actual SurfaceY at all six
    new contacts. A changed terrain cannot silently activate this old layout.
    """
    def base(px, pz):
        return (1.1 * math.sin(px * .0061 + .7) * math.cos(pz * .0047 + .3)
                + .7 * math.sin((px + pz) * .0032 + 1.9)
                + .4 * math.sin(pz * .011 - px * .004 + .4))

    def raw(px, pz):
        dx, dz = px + 250, pz - 300
        a = math.atan2(dz, dx)
        hills = smooth(1300, 2400, math.hypot(dx, dz)) * (12 + 9 * math.sin(a * 5 + 1.3) + 5 * math.sin(a * 13))
        wash = math.exp(-(px + 330 + .25 * (pz - 500)) ** 2 / 520) * smooth(-150, 60, pz)
        return base(px, pz) - base(-1.8, 0) - 1.3 * wash + hills

    value = raw(x, z)
    distance = math.hypot(x + 11.05, z - 1999.95)
    return value if distance > 110 else value + (raw(-11.05, 1999.95) - value) * (1 - smooth(45, 110, distance))


def collider_moves():
    result = []
    for name, sign in SIGNS.items():
        for dx in (-1.4, 1.4):
            x, old, new = 8000 + sign['x'] + dx, sign['old_z'], sign['z']
            result.append({'sign': name, 'old': [x - .15, x + .15, old - .15, old + .15],
                           'new': [x - .15, x + .15, new - .15, new + .15]})
    return result


def validate_source_contract(contract):
    assert contract['version'] == VERSION and contract['source_revision'] == REVISION
    assert contract['original_manifest_sha256'] == MANIFEST_SHA256
    assert contract['parent_handle'] == 'world.town.group'
    assert contract['parts'] == ['Shell', 'Roof', 'Fixtures']
    assert contract['no_new_admission'] is True and set(contract['houses']) == set(HOUSES)
    assert contract['native_policy']['material_prefix'] == 'M_Homes_'
    for name, expected in HOUSES.items():
        actual = contract['houses'][name]
        assert actual['original_road_rect_m'] == expected['rect'] and actual['front'] == expected['front']
        assert len(actual['root_world_three_m']) == 3
        assert all(math.isclose(a, b, abs_tol=1e-5) for a, b in zip(actual['root_world_three_m'], expected['root']))


def expected_meshes(trim):
    return part_names() | {name for name, row in trim['meshes'].items() if not row['hidden']} | set(trim['signs'])
