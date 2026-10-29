"""Original field kit meshes in site B metres. Pure Python; Blender only builds/exports them."""
import importlib.util, json, math, random
from functools import lru_cache
from pathlib import Path
from design import *

R = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location('substation_field_meshkit', R.parent / 'substation/meshkit.py')
_k = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(_k)
add, sub, mul, cross, dot, unit = _k.add, _k.sub, _k.mul, _k.cross, _k.dot, _k.unit
NEW_MATERIALS = {'WhitePaint'}


class Mesh(_k.Mesh):
    def face(self, ps, mat, uv=None, smooth=False):
        super().face(ps, mat, uv, smooth)
        if mat in NEW_MATERIALS:
            self.mat[-1] = 'M_SubstationField_' + mat


# Native visible ground, measured in the running game (ground_probe.json, world Three, 0.25 m).
_G = json.loads((R / 'ground_probe.json').read_text())


def ground(bx, by):
    """Bilinear measured visible ground height in B z."""
    x, z = 7890 - bx, 536.2 + by
    fx = (x - _G['x0']) / _G['step']; fz = (z - _G['z0']) / _G['step']
    i = max(0, min(_G['nx'] - 2, int(math.floor(fx)))); j = max(0, min(_G['nz'] - 2, int(math.floor(fz))))
    u = min(1, max(0, fx - i)); v = min(1, max(0, fz - j)); Y = _G['y']
    y = (Y[j][i] * (1 - u) + Y[j][i + 1] * u) * (1 - v) + (Y[j + 1][i] * (1 - u) + Y[j + 1][i + 1] * u) * v
    return y + 1.10


def ramp_top(by):
    return -0.25 + (min(max(by, RAMP[2]), RAMP[3]) - RAMP[2]) / (RAMP[3] - RAMP[2]) * 0.25


def inside(r, x, y, pad=0.0):
    return r[0] - pad <= x <= r[1] + pad and r[2] - pad <= y <= r[3] + pad


def nearest(r, x, y):
    return min(max(x, r[0]), r[1]), min(max(y, r[2]), r[3])


def wobble(t, seed):
    return 1 + .22 * math.sin(t * 2.1 + seed) + .10 * math.sin(t * 5.3 + 2 * seed)


def grade(bx, by):
    """Highest designed grade surface; -inf where nothing is authored."""
    h = -1e9
    # 1. Bed front between the PR184 shoulder end (x -2.8) and the right corner, plus that corner side.
    if by < BED[2] and -2.8 <= bx <= 10.0:
        d = BED[2] - by; h = max(h, -0.085 - d * .42 * wobble(bx, 0.3))
    if bx > 10.0 and BED[2] - 0.9 <= by <= -7.05:
        qy = min(max(by, BED[2]), -7.05); d = math.hypot(bx - 10.0, by - qy)
        h = max(h, -0.085 - d * .42 * wobble(by, 1.1))
    # 2. Ramp sides and nose: a 1.5 cm concrete lip stays readable, never a vertical slab edge.
    if not (by > RAMP[3] and RAMP[0] <= bx <= RAMP[1]) and not inside(RAMP, bx, by):
        qx, qy = nearest(RAMP, bx, by); d = math.hypot(bx - qx, by - qy)
        slope = (.30 if by < RAMP[2] else .38) * wobble(qy * 1.4 + qx, 2.0)
        h = max(h, ramp_top(qy) - .015 - d * slope)
    # 3. Pockets between ramp back, plinth sides and bed front: compacted fill, outward falloff.
    for k, p in enumerate(POCKETS):
        qx, qy = nearest(p, bx, by); d = math.hypot(bx - qx, by - qy)
        top = -.015 - (qy - p[2]) / (p[3] - p[2]) * .06
        if d < 1e-9 or not inside(RAMP, bx, by):
            h = max(h, top - d * .40 * wobble(qy * 3 + k, 3.0))
    # 4. Culvert berm, only behind the endwall and over the existing bed.
    if by >= CULVERT_BOX[3]:
        dx = max(0, CULVERT_BOX[0] - bx, bx - CULVERT_BOX[1]); dy = by - CULVERT_BOX[3]
        h = max(h, 0.60 - math.hypot(dx * 1.0, dy * .65))
    return h


HARD = (RAMP, PLINTH, (CULVERT_BOX[0], 8.95, -9.40, CULVERT_BOX[3]))


def grade_cell_surface(bx, by):
    """Vertex height for the visible grade: rests on the bed inside it, on measured ground elsewhere."""
    g = grade(bx, by)
    rest = -0.08 + .004 if inside(BED, bx, by) else ground(bx, by) + .006
    return max(g, rest), g - rest - .0005, rest


def axis(lo, hi, step, keys):
    vals = {round(lo + i * step, 4) for i in range(int(round((hi - lo) / step)) + 1)}
    vals |= {k for k in keys if lo <= k <= hi}
    return sorted(vals)


def wall_with_hole(m, name, x0, x1, z0, z1, y, cx, cz, r, n, normal_sign, mat):
    """Planar wall face at depth y with a circular opening; outline includes the exact corners."""
    m.tag = name
    angles = sorted({2 * math.pi * i / n for i in range(n)} |
                    {math.atan2(zz - cz, xx - cx) % (2 * math.pi) for xx in (x0, x1) for zz in (z0, z1)})
    outer, inner = [], []
    for a in angles:
        dx, dz = math.cos(a), math.sin(a); t = 1e9
        if abs(dx) > 1e-9: t = min(t, ((x1 if dx > 0 else x0) - cx) / dx)
        if abs(dz) > 1e-9: t = min(t, ((z1 if dz > 0 else z0) - cz) / dz)
        outer.append((cx + dx * t, y, cz + dz * t)); inner.append((cx + dx * r, y, cz + dz * r))
    for i in range(len(angles)):
        j = (i + 1) % len(angles)
        q = [inner[i], outer[i], outer[j], inner[j]]
        if normal_sign > 0: q.reverse()
        m.face(q, mat)


def prism(m, name, quad, tops, bottom, mat):
    """Vertical prism over a 2D polygon with per-corner top heights; outward faces."""
    m.tag = name
    area = sum(quad[i][0] * quad[(i + 1) % len(quad)][1] - quad[(i + 1) % len(quad)][0] * quad[i][1] for i in range(len(quad)))
    if area < 0: quad, tops = quad[::-1], tops[::-1]
    top = [(p[0], p[1], t) for p, t in zip(quad, tops)]
    m.face(top, mat)
    for i in range(len(quad)):
        j = (i + 1) % len(quad); a, b = quad[i], quad[j]
        m.face([(a[0], a[1], bottom), (b[0], b[1], bottom), top[j], top[i]], mat)


def tube_inside(m, name, a, b, r, mat, n, corrugate=0.0):
    """Inward-facing (visible from inside) cylinder from a to b along Y, optional corrugation rings."""
    m.tag = name; steps = max(1, int(abs(b[1] - a[1]) / .068)) if corrugate else 1
    rings = []
    for s in range(steps + 1):
        y = a[1] + (b[1] - a[1]) * s / steps; rr = r - (corrugate if s % 2 else 0)
        rings.append([(a[0] + rr * math.cos(2 * math.pi * i / n), y, a[2] + rr * math.sin(2 * math.pi * i / n)) for i in range(n)])
    for k in range(len(rings) - 1):
        for i in range(n):
            j = (i + 1) % n
            m.face([rings[k][i], rings[k][j], rings[k + 1][j], rings[k + 1][i]], mat, smooth=True)


def rock(m, tag, x, y, z, r, h, seed, sides):
    rng = random.Random(seed); rings = []
    for zz, scale in [(z - .03, .9), (z + h * .35, 1), (z + h, .5)]:
        rings.append([(x + math.cos(2 * math.pi * j / sides) * r * scale * rng.uniform(.78, 1.1),
                       y + math.sin(2 * math.pi * j / sides) * r * scale * rng.uniform(.78, 1.1),
                       zz + rng.uniform(-.006, .006)) for j in range(sides)])
    m.tag = tag
    for a, b in zip(rings, rings[1:]):
        for j in range(sides):
            k = (j + 1) % sides; m.face([a[j], a[k], b[k], b[j]], 'Concrete')
    m.face(rings[-1], 'Concrete'); m.face(rings[0][::-1], 'Concrete')


def build(lod=0):
    n = [24, 14, 8][lod]
    meshes = {k: Mesh(k) for k in PARTS}
    get = lambda s: meshes['SM_SubstationField_' + s]

    # ---------- Culvert: precast endwall at the proxy footprint, mouth around the feature point.
    m = get('Culvert'); x0, x1, yf, yb = CULVERT_BOX; zt, zb = .60, -.55; cx = (x0 + x1) / 2
    n = [32, 24, 16][lod]  # round mouth at every LOD
    wall_with_hole(m, 'Endwall_front', x0, x1, zb, zt, yf, cx, PIPE_CENTER_Z, PIPE_RADIUS + .02, n, -1, 'Concrete')
    wall_with_hole(m, 'Endwall_back', x0, x1, zb, zt, yb, cx, PIPE_CENTER_Z, PIPE_RADIUS + .02, n, 1, 'Concrete')
    m.tag = 'Endwall_sides'
    m.face([(x0, yf, zb), (x0, yf, zt), (x0, yb, zt), (x0, yb, zb)], 'Concrete')
    m.face([(x1, yb, zb), (x1, yb, zt), (x1, yf, zt), (x1, yf, zb)], 'Concrete')
    m.face([(x0, yf, zt), (x1, yf, zt), (x1, yb, zt), (x0, yb, zt)], 'Concrete')
    # Coping with a chamfered drip edge.
    m.tag = 'Endwall_coping'; c0, c1, cf, cb, ct = x0 - .03, x1 + .03, yf - .035, yb + .02, zt + .06
    top = [(c0 + .02, cf + .02, ct), (c1 - .02, cf + .02, ct), (c1 - .02, cb - .02, ct), (c0 + .02, cb - .02, ct)]
    low = [(c0, cf, zt), (c1, cf, zt), (c1, cb, zt), (c0, cb, zt)]
    m.face(top, 'Concrete'); m.face(low[::-1], 'Concrete')
    for i in range(4):
        j = (i + 1) % 4; m.face([low[i], low[j], top[j], top[i]], 'Concrete')
    # Corrugated galvanized pipe: rolled rim, dark weathered inside, closed far baffle.
    m.torus('Pipe_rolled_rim', (cx, yf - .028, PIPE_CENTER_Z), PIPE_RADIUS + .005, .018, 'GalvanizedSteel', n, 6 if lod < 2 else 4, axis=(0, -1, 0))
    m.lathe('Pipe_mouth_band', (cx, yf - .028, PIPE_CENTER_Z), [(0, PIPE_RADIUS + .021), (.03, PIPE_RADIUS + .021)], 'GalvanizedSteel', n, axis=(0, 1, 0), caps=False)
    tube_inside(m, 'Pipe_corrugated_inside', (cx, yf - .028, PIPE_CENTER_Z), (cx, -7.85, PIPE_CENTER_Z), PIPE_RADIUS, 'DarkSteel', n, .012 if lod == 0 else 0)
    m.tag = 'Pipe_far_silt_baffle'
    m.face([(cx + PIPE_RADIUS * math.cos(2 * math.pi * i / n), -7.85, PIPE_CENTER_Z + PIPE_RADIUS * math.sin(2 * math.pi * i / n)) for i in range(n)], 'DarkSteel')
    # Silt on the invert so the inside reads as a used drain, not a hole.
    m.box('Pipe_invert_silt', (cx, -8.35, PIPE_CENTER_Z - PIPE_RADIUS + .035), (.36, .90, .02), 'Gravel')
    # Wings: left straight beside the ramp, right flared; sloped tops die into the measured ground.
    for name, quad in (('Wingwall_left', [(x0, yf), (x0 + .14, yf), (x0 + .14, yf - .55), (x0, yf - .55)]),
                       ('Wingwall_right', [(x1 - .14, yf), (x1, yf), (x1 + .40, yf - .55), (x1 + .26, yf - .55)])):
        tops = [zt - .02 if abs(p[1] - yf) < 1e-9 else ground(p[0], p[1]) + .07 for p in quad]
        prism(m, name, quad, tops, -.5, 'Concrete')
    # Outlet apron: from the pipe invert down to the measured ground, edges buried.
    m.tag = 'Outlet_apron'; inv = PIPE_CENTER_Z - PIPE_RADIUS + .01; ya, yz = yf, -9.38
    g0 = ground(cx, yz); pts = [(x0 + .16, ya, inv), (x1 - .16, ya, inv), (x1 - .02, yz, g0 + .015), (x0 + .16, yz, g0 + .015)]
    m.face(pts[::-1], 'Concrete')
    for i in range(4):
        j = (i + 1) % 4; a, b = pts[i], pts[j]
        m.face([a, b, (b[0], b[1], b[2] - .14), (a[0], a[1], a[2] - .14)], 'Concrete')
    rng = random.Random(1907)
    for i in range([16, 9, 4][lod]):
        x = rng.uniform(x0 - .05, x1 + .45); y = rng.uniform(-9.95, -9.30)
        if x0 + .2 < x < x1 - .1 and y > -9.40: continue
        rock(m, 'Outlet_riprap', x, y, ground(x, y), rng.uniform(.07, .15), rng.uniform(.05, .11), 70 + i, [7, 6, 5][lod])

    # ---------- Reference post, sign post and survey slab.
    m = get('Reference'); px, py = BAND_POST
    gp = max(ground(px, py), grade(px, py))
    m.lathe('Post_concrete_collar', (px, py, gp - .12), [(0, .15), (.12, .15), (.165, .12)], 'Concrete', n)
    m.tube('Reference_post', (px, py, -.45), (px, py, 1.73), .045, 'GalvanizedSteel', max(8, n // 2))
    m.lathe('Post_cap', (px, py, 1.73), [(0, .052), (.02, .052), (.045, .02)], 'GalvanizedSteel', max(8, n // 2))
    for k, zc in enumerate(BAND_CENTERS_Z):
        m.tube('Painted_sleeve_%d' % (k + 1), (px, py, zc - .10), (px, py, zc + .10), .056, 'WhitePaint', max(8, n // 2))
        for zz in (zc - .10, zc + .10):
            m.torus('Sleeve_clamp', (px, py, zz), .057, .006, 'GalvanizedSteel', max(8, n // 2), 4)
    sx, sy = REFERENCE_MARKER; gs = ground(sx, sy)
    m.lathe('Sign_post_collar', (sx, sy, gs - .1), [(0, .11), (.1, .11), (.13, .085)], 'Concrete', max(8, n // 2))
    m.box('Sign_post_channel', (sx, sy, (gs - .3 + 1.02) / 2), (.05, .04, 1.32 - gs), 'GalvanizedSteel')
    m.box('Sign_backing', (sx, sy - .03, 1.03), (.30, .012, .22), 'DarkSteel')
    m.box('Sign_face_REF', (sx, sy - .038, 1.03), (.26, .004, .18), 'WhitePaint')
    m.box('Sign_stripe', (sx, sy - .041, 1.03), (.26, .003, .035), 'DarkSteel')
    for zz in (.97, 1.09): m.tube('Sign_bolt', (sx, sy - .045, zz), (sx, sy - .02, zz), .008, 'GalvanizedSteel', 6)
    # Flush precast slab at the standing mark: follows measured ground; edges sink, no float.
    fx, fy = FOOT_MARK; hw, hd = .45, .325; m.tag = 'Survey_slab'; steps = [4, 3, 2][lod]
    grid = [[(fx - hw + 2 * hw * i / steps, fy - hd + 2 * hd * j / steps) for i in range(steps + 1)] for j in range(steps + 1)]
    top = [[(x, y, ground(x, y) + .022) for x, y in row] for row in grid]
    for j in range(steps):
        for i in range(steps):
            m.face([top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i]], 'Concrete')
    ring = [top[0][i] for i in range(steps + 1)] + [top[j][steps] for j in range(1, steps + 1)] + \
           [top[steps][i] for i in range(steps - 1, -1, -1)] + [top[j][0] for j in range(steps - 1, 0, -1)]
    for i in range(len(ring)):
        a, b = ring[i], ring[(i + 1) % len(ring)]
        m.face([(a[0], a[1], a[2] - .08), (b[0], b[1], b[2] - .08), b, a], 'Concrete')
    m.lathe('Survey_disk', (fx, fy, ground(fx, fy) + .021), [(0, .045), (.006, .045), (.009, .03)], 'Copper', max(8, n // 2))
    for dx in (-.24, .24):
        x = fx + dx; m.tag = 'Painted_foot_bar'
        m.face([(x - .05, fy - .22, ground(x - .05, fy - .22) + .0235), (x + .05, fy - .22, ground(x + .05, fy - .22) + .0235),
                (x + .05, fy + .22, ground(x + .05, fy + .22) + .0235), (x - .05, fy + .22, ground(x - .05, fy + .22) + .0235)], 'WhitePaint')

    # ---------- Test fixture around the existing live windows/handles.
    m = get('TestFixture'); yfc = FIXTURE_FACE_Y; yback = -8.935; zx0, zx1 = .66, 1.66; xa, xb = 3.36, 4.54
    m.rounded_box('Enclosure_body', ((xa + xb) / 2, (yfc + yback) / 2, (zx0 + zx1) / 2), (xb - xa, yback - yfc, zx1 - zx0), .025, 'OlivePaint', [3, 2, 1][lod])
    m.box('Door_seam_top', ((xa + xb) / 2, yfc - .002, zx1 - .035), (xb - xa - .05, .004, .008), 'DarkSteel')
    m.box('Door_seam_bottom', ((xa + xb) / 2, yfc - .002, zx0 + .035), (xb - xa - .05, .004, .008), 'DarkSteel')
    m.box('Rain_hood', ((xa + xb) / 2, yfc - .03, zx1 + .012), (xb - xa + .06, .17, .025), 'OlivePaint')
    for zz in (zx0 + .18, zx1 - .18):
        m.tube('Door_hinge', (xb + .008, yfc - .01, zz - .05), (xb + .008, yfc - .01, zz + .05), .012, 'GalvanizedSteel', 8)
    for x in INDICATORS_X:
        z = 1.48; w, hgt, t = .20 + .05, .19 + .05, .08
        for bx_, bz_, sx_, sz_ in ((x, z + hgt / 2 - .0125, w, .025), (x, z - hgt / 2 + .0125, w, .025),
                                   (x - w / 2 + .0125, z, .025, hgt), (x + w / 2 - .0125, z, .025, hgt)):
            m.box('Window_bezel', (bx_, yfc - t / 2 + .002, bz_), (sx_, t, sz_), 'DarkSteel')
    m.box('Isolation_handle_escutcheon', (3.6, yfc - .018, 1.07), (.15, .036, .42), 'GalvanizedSteel')
    m.box('Auxiliary_handle_escutcheon', (3.95, yfc - .018, 1.07), (.18, .036, .18), 'GalvanizedSteel')
    m.box('Report_plate_standoff', (4.3, yfc - .033, 1.05), (.17, .066, .18), 'DarkSteel')
    for x, base in ((3.47, -.04), (4.42, 0.0)):
        m.box('Strut_leg', (x, -9.0, (base + zx0) / 2), (.042, .042, zx0 - base), 'GalvanizedSteel')
        m.box('Leg_base_plate', (x, -9.0, base + .006), (.12, .12, .012), 'GalvanizedSteel')
        for dx in (-.04, .04): m.tube('Leg_anchor', (x + dx, -9.0, base + .012), (x + dx, -9.0, base + .03), .008, 'DarkSteel', 6)
    m.box('Leg_pier', (3.47, -9.0, -.17), (.20, .20, .27), 'Concrete')
    m.tube('Fixture_conduit', (4.0, -9.0, zx0), (4.0, -9.0, .10), .019, 'GalvanizedSteel', max(6, n // 3))
    m.tube('Conduit_sweep', (4.0, -9.0, .10), (4.12, -8.95, .02), .019, 'GalvanizedSteel', max(6, n // 3))
    m.tube('Conduit_hub', (4.0, -9.0, zx0 - .05), (4.0, -9.0, zx0), .030, 'DarkSteel', max(6, n // 3))

    # ---------- Grade heightfield on axis lines that include every hard edge.
    m = get('ApronGrade'); step = [.1, .2, .4][lod]
    xs = axis(-2.8, 10.9, step, [-2.8, -2.2, 2.2, 3.2, 4.1, 5.9, 6.8, 7.15, 8.85, 8.95, 10.0])
    ys = axis(-14.5, -6.6, step, [-14.0, -9.4, -9.25, -8.8, -8.0, -7.75, -7.05])
    V = [[grade_cell_surface(x, y) for x in xs] for y in ys]
    m.tag = 'Measured_grade'
    for j in range(len(ys) - 1):
        for i in range(len(xs) - 1):
            cxm, cym = (xs[i] + xs[i + 1]) / 2, (ys[j] + ys[j + 1]) / 2
            if any(inside(r, cxm, cym) for r in HARD): continue
            corners = [(xs[i], ys[j], V[j][i]), (xs[i + 1], ys[j], V[j][i + 1]), (xs[i + 1], ys[j + 1], V[j + 1][i + 1]), (xs[i], ys[j + 1], V[j + 1][i])]
            if all(c[2][1] < 0 for c in corners): continue
            # Clip each cell at the zero contour of (grade - rest): a smooth natural outline, not grid steps.
            poly = []
            for k in range(4):
                a, b = corners[k], corners[(k + 1) % 4]; fa, fb = a[2][1], b[2][1]
                if fa >= 0: poly.append((a[0], a[1], a[2][0]))
                if (fa >= 0) != (fb >= 0):
                    t = fa / (fa - fb); x = a[0] + (b[0] - a[0]) * t; y = a[1] + (b[1] - a[1]) * t
                    poly.append((x, y, a[2][2] + (b[2][2] - a[2][2]) * t))
            clean = []
            for q in poly:
                if not clean or math.dist(q[:2], clean[-1][:2]) > 1e-4: clean.append(q)
            if len(clean) > 2 and math.dist(clean[0][:2], clean[-1][:2]) <= 1e-4: clean.pop()
            if len(clean) < 3: continue
            area = sum(clean[k][0] * clean[(k + 1) % len(clean)][1] - clean[(k + 1) % len(clean)][0] * clean[k][1] for k in range(len(clean))) / 2
            if area < 2e-5: continue
            m.face(clean, 'Gravel', [(q[0] / 1.5, q[1] / 1.5) for q in clean], smooth=True)
    return meshes
