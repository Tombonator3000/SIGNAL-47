"""Four original 1986 Caliche houses, using the accepted Town metre meshkit.

No Blender imports or runtime changes. Existing doors, dark/live window planes,
stoops, roof equipment, cars and people remain separate runtime geometry.
"""
import importlib.util
import math
import random
from pathlib import Path

from design import CONTRACT, HOUSES, PREFIX, front_x

PATH = Path(__file__).resolve().parents[1] / 'townbuildings' / 'geometry.py'
SPEC = importlib.util.spec_from_file_location('s47_homes_town_geometry', PATH)
TOWN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TOWN)


class Mesh(TOWN.Mesh):
    def __init__(self, name):
        super().__init__(name)
        self.closed_solids = []

    def face(self, ps, mat, uv=None, smooth=False):
        super().face(ps, mat, uv, smooth)
        self.mat[-1] = self.mat[-1].replace('M_Town_', 'M_Homes_', 1)

    def closed(self, tag, first_face):
        # Check every emitted solid, not a positive total hiding one inside-out
        # box. Position-weld only for this proof; authoring UV seams stay intact.
        faces = self.f[first_face:]
        edges = {}
        volume = 0.
        for face in faces:
            points = [tuple(self.v[i]) for i in face]
            for a, b in zip(points, points[1:] + points[:1]):
                key = tuple(sorted((a, b)))
                count, winding = edges.get(key, (0, 0))
                edges[key] = (count + 1, winding + (1 if (a, b) == key else -1))
            a = points[0]
            for b, c in zip(points[1:-1], points[2:]):
                volume += TOWN.dot(a, TOWN.cross(b, c)) / 6
        assert all(count == 2 and winding == 0 for count, winding in edges.values()), (self.name, tag, 'open or inconsistent solid')
        assert volume > 1e-10, (self.name, tag, 'inward or degenerate solid', volume)
        self.closed_solids.append({'tag': tag, 'first_face': first_face,
                                   'faces': len(faces), 'signed_volume_m3': volume})

    def box(self, name, center, size, mat):
        first = len(self.f)
        super().box(name, center, size, mat)
        self.closed(name, first)

    def tube(self, name, a, b, r, mat, n=12, caps=True):
        first = len(self.f)
        super().tube(name, a, b, r, mat, n, caps)
        if caps:
            self.closed(name, first)


def box(m, tag, x0, x1, y0, y1, z0, z1, material):
    if min(x1 - x0, y1 - y0, z1 - z0) > 1e-8:
        m.box(tag, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2),
              (x1 - x0, y1 - y0, z1 - z0), material)


def front_box(m, h, tag, d0, d1, y0, y1, z0, z1, material):
    fx, s = front_x(h), h['front']
    x0, x1 = sorted((fx + s * d0, fx + s * d1))
    box(m, tag, x0, x1, y0, y1, z0, z1, material)


def prism(m, tag, outline, z0, z1, material):
    first = len(m.f)
    if sum(outline[i][0] * outline[(i + 1) % len(outline)][1] -
           outline[(i + 1) % len(outline)][0] * outline[i][1] for i in range(len(outline))) < 0:
        outline = list(reversed(outline))
    low = [(x, y, z0) for x, y in outline]
    high = [(x, y, z1) for x, y in outline]
    m.tag = tag
    m.face(low[::-1], material)
    m.face(high, material)
    for i in range(len(low)):
        j = (i + 1) % len(low)
        m.face([low[i], low[j], high[j], high[i]], material)
    m.closed(tag, first)


def corner(m, x, y, sx, sy, thickness, z0, z1, material, lod):
    radius, steps = .115, [6, 3, 1][lod]
    points = [(radius, 0), (thickness, 0), (thickness, thickness),
              (0, thickness), (0, radius)]
    points += [(radius + radius * math.cos(math.pi + i * math.pi / (2 * steps)),
                radius + radius * math.sin(math.pi + i * math.pi / (2 * steps)))
               for i in range(1, steps)]
    prism(m, 'Rounded_mineral_corner', [(x + sx * a, y + sy * b) for a, b in points], z0, z1, material)


def front_wall(m, h, rect, thickness, z0, z1, material, openings):
    """Partition a real wall around voids; no solid box behind old image planes."""
    x0, x1, y0, y1 = rect
    fx, s = x1 if h['front'] > 0 else x0, h['front']
    a, b = y0 + thickness, y1 - thickness
    cuts = sorted({a, b} | {max(a, min(b, v)) for hole in openings for v in hole[:2]})
    for lo, hi in zip(cuts, cuts[1:]):
        mid = (lo + hi) / 2
        holes = [hole for hole in openings if hole[0] < mid < hole[1]]
        heights = sorted({z0, z1} | {max(z0, min(z1, v)) for hole in holes for v in hole[2:]})
        for bottom, top in zip(heights, heights[1:]):
            if any(hole[2] < (bottom + top) / 2 < hole[3] for hole in holes):
                continue
            xa, xb = sorted((fx, fx - s * thickness))
            box(m, 'Actual_front_aperture_wall', xa, xb, lo, hi, bottom, top, material)


def ring(m, h, z0, z1, thickness, material, lod, openings=(), outset=0):
    x0, x1, y0, y1 = h['footprint_blender_xy_m']
    rect = [x0 - outset, x1 + outset, y0 - outset, y1 + outset]
    x0, x1, y0, y1 = rect
    front_wall(m, h, rect, thickness, z0, z1, material, openings)
    back = x0 if h['front'] > 0 else x1
    xa, xb = sorted((back, back + h['front'] * thickness))
    box(m, 'Rear_mineral_wall', xa, xb, y0 + thickness, y1 - thickness, z0, z1, material)
    for ya, yb in [(y0, y0 + thickness), (y1 - thickness, y1)]:
        box(m, 'Side_mineral_wall', x0 + thickness, x1 - thickness, ya, yb, z0, z1, material)
    for x, sx in [(x0, 1), (x1, -1)]:
        for y, sy in [(y0, 1), (y1, -1)]:
            corner(m, x, y, sx, sy, thickness, z0, z1, material, lod)


def openings(h):
    dy, wy = h['door_y_m'], h['window_y_m']
    door_w, door_h = CONTRACT['door_surround_clear_size_m']
    w, height = CONTRACT['window_size_m']
    z = CONTRACT['window_center_z_m']
    return [(dy - door_w / 2, dy + door_w / 2, -.10, door_h),
            (wy - w / 2, wy + w / 2, z - height / 2, z + height / 2)]


def shell(home, h, lod):
    m = Mesh(PREFIX + home + '_Shell')
    x0, x1, y0, y1 = h['footprint_blender_xy_m']
    # The buried foundation reaches the measured old wall base on sloping land.
    ring(m, h, h['foundation_bottom_m'], 0, .27, 'Concrete', lod, outset=.03)
    ring(m, h, 0, .22, .25, 'Concrete', lod, openings(h), outset=.03)
    ring(m, h, .22, CONTRACT['wall_height_m'], .22, h['wall_material'], lod, openings(h))
    box(m, 'Closed_house_floor_no_new_admission', x0 + .22, x1 - .22,
        y0 + .22, y1 - .22, -.11, 0, 'RecessShadow')
    # A physically deep, dark front room remains behind the retained image/pane.
    wy = h['window_y_m']
    front_box(m, h, 'Window_room_rear', -2.05, -1.96, wy - 1.18, wy + 1.18, .02, 2.55, 'RecessShadow')
    for a, b in [(wy - 1.18, wy - 1.10), (wy + 1.10, wy + 1.18)]:
        front_box(m, h, 'Window_room_return', -2.05, -.23, a, b, .02, 2.55, 'RecessShadow')
    front_box(m, h, 'Window_room_ceiling', -2.05, -.23, wy - 1.18, wy + 1.18, 2.51, 2.58, 'RecessShadow')
    return m


def roof(home, h, lod):
    m = Mesh(PREFIX + home + '_Roof')
    x0, x1, y0, y1 = h['footprint_blender_xy_m']
    datum = CONTRACT['roof_equipment_datum_m']
    box(m, 'Roof_membrane_existing_equipment_datum', x0 + .25, x1 - .25,
        y0 + .25, y1 - .25, datum - .08, datum, 'RoofTar')
    ring(m, h, 3., 3.26, .25, h['wall_material'], lod)
    ring(m, h, 3.26, 3.30, .29, 'DoorMasonry', lod, outset=.025)
    # A real drip nose, rather than a painted line over a coplanar wall.
    if lod < 2:
        for yy in [y0 - .016, y1 + .016]:
            box(m, 'Coping_drip_nose', x0 + .14, x1 - .14, yy - .012, yy + .012, 3.235, 3.255, 'Concrete')
        for xx in [x0 - .016, x1 + .016]:
            box(m, 'Coping_drip_nose', xx - .012, xx + .012, y0 + .14, y1 - .14, 3.235, 3.255, 'Concrete')
    if lod == 0:
        # Short recessed-looking coping joints keep individual capstones legible.
        count = max(2, round((y1 - y0) / 1.15))
        for k in range(1, count):
            yy = y0 + (y1 - y0) * k / count
            front_box(m, h, 'Coping_joint_end', -.29, .026, yy - .004, yy + .004, 3.301, 3.304, 'CreamWear')
    if home == 'H01':
        fx, s = front_x(h), h['front']
        for yy in [y0 + .80 + k * 1.15 for k in range(int((y1 - y0 - 1.5) / 1.15) + 1)]:
            m.tube('Exposed_timber_viga_end', (fx - s * .05, yy, 2.91),
                   (fx + s * .23, yy, 2.91), .085, 'WarmTimber', [12, 8, 5][lod])
    return m


def window_frame(m, h, lod):
    y = h['window_y_m']
    width, height = CONTRACT['window_size_m']
    z = CONTRACT['window_center_z_m']
    material = h['trim_material']
    for a, b in [(y - width / 2 - .09, y - width / 2), (y + width / 2, y + width / 2 + .09)]:
        front_box(m, h, 'Deep_window_jamb', -.12, .115, a, b, z - height / 2 - .09, z + height / 2 + .09, material)
    for bottom, top in [(z - height / 2 - .09, z - height / 2), (z + height / 2, z + height / 2 + .09)]:
        front_box(m, h, 'Window_head_and_rail', -.12, .115, y - width / 2, y + width / 2, bottom, top, material)
    # Kept outside the opening; no extra glass plane or mullion over live TV art.
    front_box(m, h, 'Projecting_sill_nose', -.06, .20, y - width / 2 - .13,
              y + width / 2 + .13, z - height / 2 - .16, z - height / 2 - .10, 'Concrete')
    if lod < 2:
        front_box(m, h, 'Sill_drip_return', .145, .19, y - width / 2 - .08,
                  y + width / 2 + .08, z - height / 2 - .185, z - height / 2 - .16, 'DarkMetal')
        for side in [-1, 1]:
            yy = y + side * (width / 2 + .054)
            for zz in [z - height / 2 + .12, z + height / 2 - .12]:
                fx, s = front_x(h), h['front']
                m.tube('Window_casing_fastener', (fx + s * .115, yy, zz),
                       (fx + s * .12, yy, zz), .012, 'DarkMetal', [8, 5, 4][lod])
    if home_shutters(h):
        for side in [-1, 1]:
            center = y + side * (width / 2 + .27)
            front_box(m, h, 'Sage_shutter_back', .012, .055, center - .145, center + .145,
                      z - height / 2 - .03, z + height / 2 + .03, material)
            for k in range([9, 5, 2][lod]):
                zz = z - height / 2 + .06 + k * (height - .08) / [9, 5, 2][lod]
                front_box(m, h, 'Shutter_louver', .055, .085, center - .125, center + .125, zz, zz + .025, material)


def home_shutters(h):
    return h['trim_material'] == 'SagePaint'


def door_surround(m, h, lod):
    y = h['door_y_m']
    width, height = CONTRACT['door_surround_clear_size_m']
    # This surrounds the *old casing*, not just the leaf. It therefore adds no
    # duplicate six-panel door and does not need retirement of the old holder.
    for a, b in [(y - width / 2 - .11, y - width / 2), (y + width / 2, y + width / 2 + .11)]:
        front_box(m, h, 'Masonry_door_reveal', -.19, .095, a, b, 0, height, 'DoorMasonry')
    # Top at2.30 leaves the existing west-house porch bulb (bottom2.32) clear.
    front_box(m, h, 'Masonry_door_lintel_return', -.19, .095,
              y - width / 2 - .11, y + width / 2 + .11, height, 2.30, 'DoorMasonry')
    front_box(m, h, 'Inside_threshold_meets_retained_stoop', -.22, 0,
              y - .475, y + .475, -.06, 0, 'Concrete')
    # Wall-hung only: no new pole/step in the smoker's place or public passage.
    underside = CONTRACT['canopy_underside_m']
    projection = h['canopy_projection_m']
    material = 'RustTin' if h['trim_material'] == 'DarkTimber' else h['trim_material']
    front_box(m, h, 'Porch_hood_solid_deck', -.02, projection, y - .85, y + .85,
              underside, underside + .085, material)
    front_box(m, h, 'Porch_hood_front_fascia', projection - .06, projection + .015,
              y - .87, y + .87, underside - .015, underside + .11, material)
    for side in [-1, 1]:
        yy = y + side * .73
        fx, s = front_x(h), h['front']
        m.tube('Wall_mounted_porch_bracket', (fx + s * .035, yy, underside - .27),
               (fx + s * (projection - .10), yy, underside), .022, 'DarkMetal', [10, 6, 4][lod])
    if lod < 2:
        for k in range([12, 6][lod]):
            yy = y - .80 + k * 1.60 / ([12, 6][lod] - 1)
            front_box(m, h, 'Porch_deck_board_or_tin_seam', .02, projection - .06,
                      yy - .009, yy + .009, underside + .086, underside + .095, material)


def fixtures(home, h, lod):
    m = Mesh(PREFIX + home + '_Fixtures')
    window_frame(m, h, lod)
    door_surround(m, h, lod)
    # A new louver on the rear wall, clear of retained roof antennas and coolers.
    x0, x1, y0, y1 = h['footprint_blender_xy_m']
    back = x0 if h['front'] > 0 else x1
    out = -h['front']
    yy = (y0 + y1) / 2
    xa, xb = sorted((back + out * .005, back + out * .035))
    box(m, 'Rear_wall_vent_recess', xa, xb, yy - .30, yy + .30, 2.44, 2.82, 'DarkMetal')
    xa, xb = sorted((back + out * .035, back + out * .075))
    for k in range([8, 4, 2][lod]):
        zz = 2.47 + k * .30 / [8, 4, 2][lod]
        box(m, 'Galvanized_wall_vent_louver', xa, xb, yy - .27, yy + .27, zz, zz + .018, 'Galvanized')
    if lod < 2:
        # Small mineral repairs follow the lower wall, never the protected openings.
        rng = random.Random(1947 + int(home[1:]))
        holes = openings(h)
        wear = 'TanWear' if h['wall_material'] == 'TanStucco' else 'CreamWear'
        for k in range([17, 7][lod]):
            yy = rng.uniform(y0 + .35, y1 - .35)
            zz = rng.uniform(.28, .82)
            width, height = rng.uniform(.11, .33), rng.uniform(.035, .10)
            if any(yy + width / 2 > a - .01 and yy - width / 2 < b + .01 and
                   zz + height / 2 > c - .01 and zz - height / 2 < d + .01 for a, b, c, d in holes):
                continue
            front_box(m, h, 'Localized_mineral_repair', .002, .006,
                      yy - width / 2, yy + width / 2, zz - height / 2, zz + height / 2, wear)
    return m


def build(lod):
    assert lod in (0, 1, 2)
    result = {}
    for home, h in HOUSES.items():
        for fn in (shell, roof, fixtures):
            part = fn(home, h, lod)
            result[part.name] = part
    return result
