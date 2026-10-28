"""TB2 Feed: Caliche Feed & Seed, a weathered 1986 false-front feed and farm supply store.

build(lod) -> {SM_Town_Feed_Shell, _Glass, _Interior, _Fixtures}. Pure Python, no Blender.
Same local frame as TB1 (metres, Z up, facade plane x=0, outward +x). Footprint, wall height,
door and windows are read from design.BUILDINGS['Feed'] and never changed. The 0.95 m loading
dock keeps exactly the TB1 'Existing_loading_dock' volume (x 0..2.6, y -4..6, z -.27..0.95).
"""
import math, sys
from pathlib import Path
R = Path(__file__).resolve().parent.parent
if str(R) not in sys.path:
    sys.path.insert(0, str(R))
from geometry import Mesh, front_wall, seg, add, sub, mul, dot, cross, unit  # noqa: E402
from design import BUILDINGS  # noqa: E402

B = BUILDINGS['Feed']
X0, X1, Y0, Y1 = B['footprint']
H = B['height']                      # 5.2 eave/main wall height
DOOR_Y, DOOR_W, DOOR_H = B['door']
WINDOWS = B['windows']
T = .22                               # wall thickness, centred on footprint lines as in TB1
FF = T / 2                            # outer face of facade
FLOOR = .95                           # retail/warehouse floor shares the dock datum (TB1)
RIDGE = 6.15                          # roof deck underside at ridge (y=0); falls 1:12 to the eaves
STORE_X = -9.5                        # masonry store / corrugated warehouse split
DOCK = (0, 2.6, -4, 6, -.27, .95)


# ---------------------------------------------------------------- generic helpers
class LodMesh(Mesh):
    """Mesh whose round primitives thin out with LOD (fewer sides, no small caps)."""
    def __init__(self, name, lod):
        super().__init__(name); self.lod = lod

    def tube(self, name, a, b, r, mat, n=12, caps=True):
        n = max(4, min(n, [n, 5, 4][self.lod]))
        super().tube(name, a, b, r, mat, n, caps and self.lod == 0)

    def lathe(self, name, origin, profile, mat, n=24, axis=(0, 0, 1), caps=True):
        n = max(5, int(round(n * [1, .72, .5][self.lod])))
        super().lathe(name, origin, profile, mat, n, axis, caps)



def oq(m, ps, mat, want, uv=None):
    """Face with winding chosen so its normal agrees with want."""
    n = cross(sub(ps[1], ps[0]), sub(ps[2], ps[0]))
    if dot(n, want) < 0:
        ps = ps[::-1]
        if uv:
            uv = uv[::-1]
    m.face(ps, mat, uv)


def hexa(m, tag, v, mat):
    """Closed 8-corner solid (v[0:4] bottom ring, v[4:8] top ring), faces oriented outward."""
    m.tag = tag
    c = mul(tuple(sum(p[k] for p in v) for k in range(3)), 1 / 8)
    for ids in [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]:
        ps = [v[i] for i in ids]
        fc = mul(tuple(sum(p[k] for p in ps) for k in range(3)), .25)
        oq(m, ps, mat, sub(fc, c))


def beam(m, tag, a, b, w, h, mat, upv=(0, 0, 1)):
    """Rectangular member from a to b, width w (side) and depth h (up)."""
    ax = unit(sub(b, a))
    side = cross(ax, upv)
    if dot(side, side) < 1e-9:
        side = cross(ax, (1, 0, 0))
    side = unit(side)
    up = cross(side, ax)
    v = []
    for p in (a, b):
        pass
    corners = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
    v = [add(a, add(mul(side, sx * w / 2), mul(up, sz * h / 2))) for sx, sz in corners] + \
        [add(b, add(mul(side, sx * w / 2), mul(up, sz * h / 2))) for sx, sz in corners]
    hexa(m, tag, [v[0], v[1], v[5], v[4], v[3], v[2], v[6], v[7]], mat)


def bx(m, tag, x0, x1, y0, y1, z0, z1, mat):
    if min(x1 - x0, y1 - y0, z1 - z0) <= 1e-6:
        return
    m.box(tag, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0), mat)


def prism_x(m, tag, x0, x1, poly, mat):
    """Wall slab between x0 and x1 with convex outline poly [(y,z)...]."""
    m.tag = tag
    a = [(x0, y, z) for y, z in poly]
    b = [(x1, y, z) for y, z in poly]
    oq(m, a, mat, (-1, 0, 0))
    oq(m, b, mat, (1, 0, 0))
    cy = sum(p[0] for p in poly) / len(poly); cz = sum(p[1] for p in poly) / len(poly)
    for i in range(len(poly)):
        j = (i + 1) % len(poly)
        mid = ((poly[i][0] + poly[j][0]) / 2 - cy, (poly[i][1] + poly[j][1]) / 2 - cz)
        oq(m, [a[i], a[j], b[j], b[i]], mat, (0, mid[0], mid[1]))


def corr_offset(u, per, amp, segs):
    if segs <= 1:
        return 0
    return amp * .5 * (1 - math.cos(2 * math.pi * u / per))


def corr_sheet(m, tag, origin, U, ulen, V, vlen, up, per, amp, segs, mat, under=False):
    """Corrugated sheet: profile across U, flutes run along V, bulges toward up.
    segs per period (6 sine, 3 coarse, 2 zigzag, 1 flat single quad)."""
    m.tag = tag
    U = unit(U); V = unit(V); N = unit(cross(U, V))
    if dot(N, up) < 0:
        N = mul(N, -1)
    if segs <= 1:
        us = [0, ulen]
    else:
        n = max(1, round(ulen / per)); p = ulen / n
        us = [i * p / segs for i in range(n * segs + 1)]
        per = p
    for u0, u1 in zip(us, us[1:]):
        d0 = corr_offset(u0, per, amp, segs); d1 = corr_offset(u1, per, amp, segs)
        P = lambda u, v, d: add(origin, add(mul(U, u), add(mul(V, v), mul(N, d))))
        ps = [P(u0, 0, d0), P(u1, 0, d1), P(u1, vlen, d1), P(u0, vlen, d0)]
        oq(m, ps, mat, N)
        if under:
            q = [add(p, mul(N, -.006)) for p in ps]
            oq(m, q, mat, mul(N, -1))


def corr_wall(m, tag, p0, along, length, out, z0, ztop, per, amp, segs, mat, off=.004):
    """Vertical corrugated cladding. ztop(u) gives the top edge (gables)."""
    m.tag = tag
    along = unit(along); out = unit(out)
    if segs <= 1:
        n = max(1, round(length / 3.0)); us = [i * length / n for i in range(n + 1)]; p = 1
    else:
        n = max(1, round(length / per)); p = length / n
        us = [i * p / segs for i in range(n * segs + 1)]
    for u0, u1 in zip(us, us[1:]):
        d0 = off + corr_offset(u0, p, amp, segs); d1 = off + corr_offset(u1, p, amp, segs)
        a0 = add(p0, add(mul(along, u0), mul(out, d0))); a1 = add(p0, add(mul(along, u1), mul(out, d1)))
        t0 = ztop(u0); t1 = ztop(u1)
        oq(m, [add(a0, (0, 0, z0)), add(a1, (0, 0, z0)), add(a1, (0, 0, t1)), add(a0, (0, 0, t0))], mat, out)


def roof_z(y):
    """Roof deck underside height at |y|."""
    return RIDGE - abs(y) / 12.0


def lathe_x(m, tag, origin, profile, mat, n, axis=(1, 0, 0), caps=True):
    m.lathe(tag, origin, profile, mat, n, axis=axis, caps=caps)


def gooseneck(m, base, out, lod, R=.30, sweep=.80):
    """Wall-mounted gooseneck barn light: wall plate, curved arm, RLM shade, emissive lens."""
    out = unit(out); n = [14, 10, 6][lod]
    m.lathe('Gooseneck_wall_plate', base, [(0, .07), (.025, .07), (.03, .05)], 'DarkMetal', 8, axis=out)
    steps = [8, 5, 3][lod]
    pts = []
    for k in range(steps + 1):
        t = math.pi * sweep * k / steps
        pts.append(add(base, add(mul(out, .03 + R - R * math.cos(t)), (0, 0, R * math.sin(t)))))
    for a, b in zip(pts, pts[1:]):
        m.tube('Gooseneck_arm', a, b, .018, 'DarkMetal', 6, caps=False)
    tip = pts[-1]
    c = add(tip, (0, 0, -.05))
    m.lathe('RLM_barn_shade', add(c, (0, 0, .05)), [(0, .03), (-.03, .08), (-.09, .17), (-.13, .21)], 'FeedGalv', n, caps=False)
    m.lathe('RLM_shade_inner', add(c, (0, 0, -.081)), [(0, .205), (.04, .16), (.10, .07), (.13, .025)], 'CreamEnamel', n, caps=False)
    m.lathe('Barn_light_lens', add(c, (0, 0, -.06)), [(0, .085), (-.035, .07), (-.045, .001)], 'WarmGlow', max(8, n // 2), caps=False)
    m.lathe('Barn_light_lens_cap', add(c, (0, 0, -.06)), [(0, .001), (.0005, .085)], 'WarmGlow', max(8, n // 2), caps=False)


GLYPHS = {
    # Stroke polylines in a unit cell (y right, z up), height 1. Serifs are short bars.
    'F': [[(0, 0), (0, 1)], [(0, 1), (.58, 1)], [(0, .52), (.42, .52)], [(-.1, 0), (.2, 0)], [(-.1, 1), (0, 1)],
          [(.58, 1), (.58, .82)], [(.42, .62), (.42, .42)]],
    'E': [[(0, 0), (0, 1)], [(0, 1), (.58, 1)], [(0, .52), (.42, .52)], [(0, 0), (.62, 0)], [(-.1, 0), (0, 0)], [(-.1, 1), (0, 1)],
          [(.58, 1), (.58, .82)], [(.42, .62), (.42, .42)], [(.62, 0), (.62, .2)]],
    'D': [[(0, 0), (0, 1)], [(-.1, 0), (.28, 0)], [(-.1, 1), (.28, 1)], 'arcD'],
    'S': ['arcS'],
    '&': ['arcAmp'],
}


def _arc(cx, cz, r, a0, a1, k):
    return [(cx + r * math.cos(a0 + (a1 - a0) * i / k), cz + r * math.sin(a0 + (a1 - a0) * i / k)) for i in range(k + 1)]


def grime_and_repairs(s, lod):
    """Darker splash-zone grime with a ragged top edge, and raised patchy stucco repairs."""
    import random
    rnd = random.Random(1986)
    def strip(face_pt, along, out, a, b, top_lo, top_hi):
        u = a
        t1 = rnd.uniform(top_lo, top_hi)
        while u < b - .05:
            w = min(rnd.uniform(.4, 1.1), b - u)
            t0, t1 = t1, min(top_hi, max(top_lo, t1 + rnd.uniform(-.12, .12)))
            p = lambda uu, z: add(face_pt, add(mul(along, uu), (0, 0, z)))
            s.tag = 'Splash_zone_grime'
            oq(s, [add(p(u, .5), out), add(p(u + w, .5), out), add(p(u + w, t1), out), add(p(u, t0), out)], 'FeedGrime', out)
            u += w
    # facade (beside the dock) and both stucco side walls
    for a, b in [(-11.45, DOCK[2]), (DOCK[3], 11.45)]:
        strip((FF, 0, 0), (0, 1, 0), (.005, 0, 0), a, b, .78, 1.08)
    for sgn in [-1, 1]:
        strip((0, sgn * (12 + FF), 0), (1, 0, 0), (0, sgn * .005, 0), STORE_X + .1, -.02, .74, 1.0)
    # raised repairs: overlapping trowelled rectangles, 6 to 14 mm proud, on facade and parapet
    reps = [(-10.2, 3.6, 1.1, .7), (-9.4, 3.2, .6, .5), (-6.3, .95, .9, .45), (-2.9, 4.2, .7, .5), (4.4, 4.5, .9, .55),
            (8.2, 3.4, 1.2, .8), (9.1, 2.9, .5, .45), (10.6, 5.6, .8, .5), (-11.0, 5.7, .6, .7), (6.5, 6.4, .9, .3),
            (-5.6, 6.45, .7, .35), (2.9, 7.0, .8, .3), (-1.4, 6.9, .6, .25), (11.0, 2.2, .5, .9), (-8.4, 1.5, .5, .4)]
    for k, (y, z, w, h) in enumerate(reps if lod < 2 else reps[:6]):
        d = .006 + .004 * (k % 3)
        bx(s, 'Stucco_repair_patch', FF, FF + d, y - w / 2, y + w / 2, z - h / 2, z + h / 2, 'FeedStuccoPatch')
    for sgn in [-1, 1]:
        for x, z, w, h in [(-3.0, 3.1, 1.2, .8), (-7.2, 1.6, .8, .6), (-5.0, 4.4, .9, .4)]:
            yy = sgn * (12 + FF)
            bx(s, 'Stucco_repair_patch', x - w / 2, x + w / 2, *(sorted([yy, yy + sgn * .008])), z - h / 2, z + h / 2, 'FeedStuccoPatch')


def painted_letters(m, text, yc, z0, h, lod):
    st = .15                       # stroke in units of letter height
    k = [10, 7, 4][lod]
    adv = {'F': .80, 'E': .82, 'D': .86, 'S': .78, '&': .92, ' ': .38}
    total = sum(adv[c] for c in text) * h
    # A viewer facing the facade looks along -x, so +y is to the viewer's right.
    y = yc - total / 2
    x = FF + .002
    for c in text:
        strokes = []
        for sdef in GLYPHS.get(c, []):
            if sdef == 'arcD':
                strokes.append(_arc(.28, .5, .5 - st / 2, math.pi / 2, -math.pi / 2, k + 2))
            elif sdef == 'arcS':
                strokes.append(_arc(.32, .745, .245, .15, math.pi * 1.5, k + 2))
                strokes.append(_arc(.32, .255, .245, math.pi / 2, -math.pi * .85, k + 2))
                strokes.append([(.565, .76), (.565, .9)])
                strokes.append([(.075, .24), (.075, .1)])
            elif sdef == 'arcAmp':
                strokes.append(_arc(.30, .76, .17, -math.pi * .25, math.pi * 1.6, k + 2))
                strokes.append(_arc(.30, .27, .25, math.pi * .35, math.pi * 1.85, k + 2))
                strokes.append([(.2, .62), (.72, 0)])
                strokes.append([(.48, .3), (.74, .44)])
            else:
                strokes.append(sdef)
        P = lambda u, v, dx: (x + dx, y + (u + .1) * h, z0 + v * h)
        si = 0
        for pl in strokes:
            if len(pl) > 2:
                # Curved stroke as a mitred ribbon: smooth bowls, front face plus both edges.
                L_, R_ = [], []
                for j in range(len(pl)):
                    a_ = pl[max(0, j - 1)]; b_ = pl[min(len(pl) - 1, j + 1)]
                    tu, tv = b_[0] - a_[0], b_[1] - a_[1]; tl = math.hypot(tu, tv)
                    nu, nv = -tv / tl * st / 2, tu / tl * st / 2
                    L_.append((pl[j][0] + nu, pl[j][1] + nv)); R_.append((pl[j][0] - nu, pl[j][1] - nv))
                m.tag = 'Painted_letter_stroke'
                for j in range(len(pl) - 1):
                    oq(m, [P(*L_[j], .008), P(*L_[j + 1], .008), P(*R_[j + 1], .008), P(*R_[j], .008)], 'FeedLetterPaint', (1, 0, 0))
                    for E_ in (L_, R_):
                        mid = ((E_[j][0] + E_[j + 1][0]) / 2 - (pl[j][0] + pl[j + 1][0]) / 2, (E_[j][1] + E_[j + 1][1]) / 2 - (pl[j][1] + pl[j + 1][1]) / 2)
                        oq(m, [P(*E_[j], 0), P(*E_[j + 1], 0), P(*E_[j + 1], .008), P(*E_[j], .008)], 'FeedLetterPaint', (0, mid[0], mid[1]))
                for j, o in [(0, -1), (len(pl) - 1, 1)]:
                    k2 = j - o
                    oq(m, [P(*L_[j], 0), P(*R_[j], 0), P(*R_[j], .008), P(*L_[j], .008)], 'FeedLetterPaint', (0, pl[j][0] - pl[k2][0], pl[j][1] - pl[k2][1]))
                continue
            (u0, v0), (u1, v1) = pl
            d = math.hypot(u1 - u0, v1 - v0)
            serif = d < .32
            w = st * (.5 if serif else 1.0)
            e = w / 2 / d if not serif else 0
            dep = .009 + .0012 * (si % 4)          # staggered depth: no coplanar overlaps
            si += 1
            A = P(u0 - (u1 - u0) * e, v0 - (v1 - v0) * e, dep / 2); Bp = P(u1 + (u1 - u0) * e, v1 + (v1 - v0) * e, dep / 2)
            beam(m, 'Painted_letter_stroke', A, Bp, w * h, dep, 'FeedLetterPaint', upv=(1, 0, 0))
        y += adv[c] * h


def pendant(m, x, y, z, lod, label='WarmGlow', inner='CreamEnamel'):
    """Spun enamel pendant (TB1 profile) at a lighter segment count."""
    n = [14, 10, 6][lod]
    m.tube('Fixture_stem', (x, y, z), (x, y, z + .4), .012, 'DarkMetal', 6, caps=False)
    m.lathe('Spun_enamel_shade', (x, y, z - .14), [(0, .25), (.05, .26), (.15, .10), (.2, .04)], 'DarkMetal', n, caps=False)
    m.lathe('Spun_shade_enamel_inside', (x, y, z - .145), [(.2, .035), (.15, .095), (.05, .25)], inner, n, caps=False)
    m.lathe('Opal_bulb', (x, y, z - .14), [(0, .07), (.07, .09), (.12, .05)], label, n)


def bin_(m, x, y, z, lod, r=.30, h=.72, mat='FeedGalv', fill='FeedGrain', lid=False):
    """Galvanized stock bin: outer wall, rolled rims, real inner wall and visible feed level."""
    n = seg(lod)
    m.lathe('Galvanized_bin_wall', (x, y, z), [(0, r * .93), (.03, r), (h * .5, r), (h * .5 + .02, r * 1.02), (h * .5 + .04, r), (h, r * 1.03)], mat, n, caps=False)
    m.lathe('Galvanized_bin_bottom', (x, y, z), [(0, 0.001), (0.0005, r * .93)], mat, n, caps=False)
    m.lathe('Galvanized_bin_rim', (x, y, z + h), [(0, r * 1.03), (.025, r * 1.05), (.03, r * .99), (0, r * .99)], mat, n, caps=False)
    if lid:
        m.lathe('Galvanized_bin_lid', (x, y, z + h + .01), [(0, r * 1.07), (.03, r * 1.07), (.07, r * .5), (.09, r * .12), (.13, r * .1), (.135, .001)], mat, n, caps=False)
    else:
        m.lathe('Galvanized_bin_inner', (x, y, z + h), [(0, r * .99), (-.10, r * .99)], mat, n, caps=False)
        m.lathe('Feed_level_in_bin', (x, y, z + h - .10), [(0, r * .99), (0.0005, .001)], fill, n, caps=False)


def pallet(m, x, y, z, lod, sx=1.2, sy=1.0, mat='Timber'):
    for a in [-1, 0, 1]:
        bx(m, 'Pallet_stringer', x - sx / 2, x + sx / 2, y + a * (sy / 2 - .05) - .045, y + a * (sy / 2 - .05) + .045, z, z + .1, mat)
    k = [7, 5, 1][lod]
    if k == 1:
        bx(m, 'Pallet_top_deck', x - sx / 2, x + sx / 2, y - sy / 2, y + sy / 2, z + .1, z + .125, mat)
    else:
        w = sx / k * .78
        for i in range(k):
            xc = x - sx / 2 + (i + .5) * sx / k
            bx(m, 'Pallet_deck_board', xc - w / 2, xc + w / 2, y - sy / 2, y + sy / 2, z + .1, z + .125, mat)
    return z + .125


def sack(m, x, y, z, lod=0, yaw=0, mat='Burlap'):
    """50 lb feed sack lying flat: pillow body, pinched sewn ends, slight sag."""
    n = [10, 8, 6][lod]
    prof = [(0, .27, .38), (.035, .31, .44), (.10, .32, .45), (.155, .28, .41), (.175, .20, .33)]
    rings = []
    for zz, sx, sy in prof:
        ring = []
        for k in range(n):
            a = k * math.tau / n
            # squarer pillow outline (superellipse) rotated by yaw
            ca, sa = math.cos(a), math.sin(a)
            px = sx * math.copysign(abs(ca) ** .6, ca); py = sy * math.copysign(abs(sa) ** .6, sa)
            ring.append((x + px * math.cos(yaw) - py * math.sin(yaw), y + px * math.sin(yaw) + py * math.cos(yaw), z + zz))
        rings.append(ring)
    m.tag = 'Flat_sewn_feed_sack'
    m.face(rings[0][::-1], mat); m.face(rings[-1], mat)
    for a, b in zip(rings, rings[1:]):
        for k in range(n):
            j = (k + 1) % n
            m.face([a[k], a[j], b[j], b[k]], mat, smooth=True)
    if lod == 0:
        for e in [-1, 1]:
            c = (x - math.sin(yaw) * e * .43, y + math.cos(yaw) * e * .43, z + .09)
            d = (-math.cos(yaw) * .25, -math.sin(yaw) * .25, 0)
            m.tube('Sack_sewn_end_seam', add(c, d), sub(c, d), .012, 'DarkWood' if mat == 'Burlap' else 'Burlap', 4, caps=False)


def sack_stack(m, x, y, z, layers, lod, sx=1.2, sy=1.0, skip=()):
    top = pallet(m, x, y, z, lod, sx, sy)
    for L in range(layers):
        zz = top + L * .165
        if L % 2 == 0:
            spots = [(x - .3, y, 0), (x + .3, y, 0)]
        else:
            spots = [(x, y - .23, math.pi / 2), (x, y + .23, math.pi / 2)]
        for k, (sxp, syp, yaw) in enumerate(spots):
            if (L, k) in skip:
                continue
            sack(m, sxp + .02 * ((L * 3 + k) % 3 - 1), syp + .015 * ((L + k) % 2), zz, lod, yaw + .04 * ((L + k) % 3 - 1),
                 'FeedSackPrint' if (L + k + int(x * 3)) % 3 == 0 else 'Burlap')


# ---------------------------------------------------------------- shell
def shell_architecture(s, g, f, lod):
    n = seg(lod)
    cs = [6, 3, 2][lod]                      # corrugation segments per period
    per = .19 if lod < 2 else .38
    amp = .022
    # Foundation stem, kept just inside the TB1 foundation outline.
    bx(s, 'Concrete_stem_wall', X0 - .14, X1 + .02, Y0 - .14, Y1 + .14, -.35, .16, 'Concrete')

    # --- Facade: real wall voids for door and windows, unchanged design apertures.
    holes = [(DOOR_Y - DOOR_W / 2, DOOR_Y + DOOR_W / 2, 0, DOOR_H)] + [(y - w / 2, y + w / 2, z0, z1) for y, w, z0, z1 in WINDOWS]
    front_wall(s, X1, Y0, Y1, H, holes, 'FeedStucco', T)
    # False front rises in three steps above the low gable roof behind it.
    steps = [(12.0, 6.25), (7.6, 6.75), (3.8, 7.35)]
    zlo = H
    for half, top in steps:
        bx(s, 'False_front_parapet', -FF, FF, -half, half, zlo, top, 'FeedStucco')
        zlo = top
    # Cap trim: corbelled stucco band and a folded galvanized coping on every step.
    prev = None
    for k, (half, top) in enumerate(steps):
        nxt = steps[k + 1][0] if k + 1 < len(steps) else 0
        for sgn in [-1, 1]:
            a, b = sorted([sgn * (half + (.10 if k == 0 else .0)), sgn * nxt])
            if k == len(steps) - 1:
                a, b = -half, half
                if sgn > 0:
                    continue
            bx(s, 'False_front_corbel_band', -FF - .03, FF + .09, a, b, top - .20, top - .10, 'FeedStucco')
            bx(s, 'False_front_cap_mould', -FF - .05, FF + .13, a, b, top - .10, top - .02, 'WhitePaint')
            bx(s, 'Galvanized_parapet_coping', -FF - .07, FF + .15, a, b, top - .02, top + .025, 'FeedGalv')
            if lod < 2:
                bx(s, 'Coping_drip_lip', FF + .13, FF + .155, a, b, top - .06, top, 'FeedGalv')
        if k > 0:
            lower = steps[k - 1][1]
            for sgn in [-1, 1]:
                e = sgn * half
                bx(s, 'Step_return_coping', -FF - .07, FF + .15, min(e, e + sgn * .09), max(e, e + sgn * .09), lower - .02, top + .025, 'FeedGalv')
                bx(s, 'Step_return_mould', -FF - .05, FF + .13, min(e, e + sgn * .07), max(e, e + sgn * .07), lower - .10, top - .02, 'WhitePaint')
    # Corner pilasters with a plinth; they return onto the side walls.
    for sgn in [-1, 1]:
        y_out = sgn * (12 + FF); y_in = sgn * 11.45
        bx(s, 'Corner_pilaster', FF - .01, FF + .10, min(y_out, y_in), max(y_out, y_in), .45, 6.05, 'FeedStucco')
        bx(s, 'Corner_pilaster_plinth', FF - .01, FF + .13, min(y_out, y_in) - (.03 if sgn < 0 else 0), max(y_out, y_in) + (.03 if sgn > 0 else 0), 0, .45, 'FeedStuccoBase')
        bx(s, 'Pilaster_capital', FF - .01, FF + .14, min(y_out, y_in) - (.03 if sgn < 0 else 0), max(y_out, y_in) + (.03 if sgn > 0 else 0), 6.05 - .12, 6.05, 'WhitePaint')
    # Weathered cement base band on the exposed facade (not behind the dock).
    for a, b in [(-11.45, DOCK[2]), (DOCK[3], 11.45)]:
        bx(s, 'Facade_cement_base_band', FF - .01, FF + .018, a, b, 0, .55, 'FeedStuccoBase')
    grime_and_repairs(s, lod)
    # Spalled plaster showing adobe beneath, thin convex patches proud of the stucco.
    patches = [(-9.6, 1.9, .7, .22), (8.9, 3.3, .45, .16), (10.4, 1.05, .5, .3), (-10.9, .85, .4, .2)]
    for y, z, w, h in (patches if lod < 2 else patches[:3]):
        pts = [(FF + .004, y + w * .5 * math.cos(a) * (1 + .25 * math.sin(3 * a)), z + h * .5 * math.sin(a)) for a in [k * math.tau / 9 for k in range(9)]]
        s.tag = 'Spalled_plaster_adobe_patch'
        oq(s, pts, 'FeedAdobe', (1, 0, 0))

    # Dirt and rain streaks below the copings and window sills (proud quads, not decals).
    s.tag = 'Rain_streak_stain'
    import random
    rnd = random.Random(47)
    streaks = []
    for k, (half, top) in enumerate(steps):
        nxt = steps[k + 1][0] if k + 1 < len(steps) else -half
        for lo, hi in ([(nxt, half), (-half, -nxt)] if nxt > 0 else [(-half, half)]):
            y = lo + rnd.uniform(.05, .3)
            while y < hi - .4:
                w = rnd.uniform(.05, .22)
                streaks.append((y, min(y + w, hi - .05), top - .20, rnd.uniform(.3, 1.3)))
                y += w + rnd.uniform(.4, 2.2)
    for yw, w, z0, z1 in WINDOWS:
        for k in range(3):
            y = yw - w / 2 + rnd.uniform(.1, w - .5)
            streaks.append((y, y + rnd.uniform(.15, .35), z0 - .09, rnd.uniform(.25, .7)))
    for a, b, ztop, ln in streaks[:None if lod < 2 else 12]:
        w_ = b - a
        oq(s, [(FF + .006, a + w_ * .40, ztop - ln), (FF + .006, b - w_ * .40, ztop - ln * .93), (FF + .006, b, ztop), (FF + .006, a, ztop)], 'FeedStuccoBase', (1, 0, 0))

    # --- FEED & SEED painted straight onto the false front: large dark serif letters,
    # modelled as 8 mm proud paint relief (no signboard), lit by the goosenecks above.
    painted_letters(s, 'FEED & SEED', 0.0, 5.08, 1.02, lod)

    # --- Side walls: stucco store returns, then a corrugated metal warehouse.
    for sgn in [-1, 1]:
        y = sgn * 12
        bx(s, 'Side_stucco_store_wall', STORE_X, X1, y - FF, y + FF, 0, H, 'FeedStucco')
        bx(s, 'Side_warehouse_frame_wall', X0, STORE_X, y - FF, y + FF, 0, H, 'Timber')
        bx(s, 'Side_cement_base_band', STORE_X + .1, X1 - .02, *(sorted([y + sgn * FF - sgn * .01, y + sgn * (FF + .018)])), 0, .55, 'FeedStuccoBase')
        bx(s, 'Store_wall_end_pier', STORE_X - .05, STORE_X + .22, *(sorted([y + sgn * FF - sgn * .02, y + sgn * (FF + .07)])), 0, H + .02, 'FeedStucco')
        p0 = (X0 - FF, y + sgn * FF, 0)
        for z0, z1, extra, mat in [(.18, 2.75, 0, 'RustTin'), (2.6, H - .02, .007, 'RustTin')]:
            corr_wall(s, 'Corrugated_warehouse_side', p0, (1, 0, 0), STORE_X - (X0 - FF) - .05, (0, sgn, 0), z0, lambda u, z1=z1: z1, per, amp, cs, mat, .004 + extra)
        # Replacement galvanized sheets where the rust ate through.
        if lod < 2:
            gx = -13.6 if sgn > 0 else -11.2
            corr_wall(s, 'Galvanized_patch_sheet', (gx, y + sgn * FF, 0), (1, 0, 0), .95, (0, sgn, 0), .3, lambda u: 2.2, per, amp, cs, 'FeedGalv', .016)
        for x in [X0 - FF - .01, STORE_X - .05]:
            bx(s, 'Warehouse_corner_trim', x - .05, x + .05, *(sorted([y + sgn * FF, y + sgn * (FF + .06)])), .15, H, 'FeedGalv')
        bx(s, 'Warehouse_base_flashing', X0 - FF, STORE_X - .05, *(sorted([y + sgn * FF, y + sgn * (FF + .045)])), .14, .22, 'FeedGalv')
    # Rear gable wall, timber frame clad in corrugated tin following the roof line.
    gable = [(Y0 - FF, 0), (Y1 + FF, 0), (Y1 + FF, H), (0, roof_z(0) + .02), (Y0 - FF, H)]
    prism_x(s, 'Rear_gable_frame_wall', X0 - FF, X0 + FF, gable, 'Timber')
    top = lambda u: (H + (12 - abs(u - 12.0)) / 12.0) if True else 0
    for z0, z1fn, extra in [(.18, lambda u: 2.75, 0), (2.6, lambda u: min(top(u), roof_z(u - 12.0) + .01), .007)]:
        corr_wall(s, 'Corrugated_rear_wall', (X0 - FF, Y1 + FF, 0), (0, -1, 0), 24 + T, (-1, 0, 0), z0, z1fn, per, amp, cs, 'RustTin', .004 + extra)
    bx(s, 'Rear_base_flashing', X0 - FF - .045, X0 - FF, Y0 - FF, Y1 + FF, .14, .22, 'FeedGalv')

    # --- Low 1:12 gable roof behind the false front, corrugated with stepped laps.
    xr0, xr1 = X0 - .45, -FF
    for sgn in [-1, 1]:
        ye = sgn * (12 + .45)
        a = (xr0, 0, roof_z(0)); b = (xr0, ye, roof_z(ye))
        # Deck slab
        v = [(xr0, 0, roof_z(0)), (xr1, 0, roof_z(0)), (xr1, ye, roof_z(ye)), (xr0, ye, roof_z(ye))]
        hexa(s, 'Roof_board_deck', v + [add(p, (0, 0, .10)) for p in v], 'DarkWood')
        V = unit((0, ye, roof_z(ye) - roof_z(0))); L = math.hypot(ye, roof_z(ye) - roof_z(0))
        courses = [3, 3, 1][lod]
        clen = L / courses
        for k in range(courses):
            v0 = L - (k + 1) * clen - (.14 if k + 1 < courses else 0)
            v1 = L - k * clen
            o = add(add((xr0, 0, roof_z(0) + .10), mul(V, max(0, v0))), (0, 0, .004 + .009 * k))
            corr_sheet(s, 'Corrugated_roof_course', o, (1, 0, 0), xr1 - xr0, V, v1 - max(0, v0), (0, 0, 1), per, amp, cs, 'RustTin')
        # Eave drip edge, fascia and gutter
        bx(s, 'Eave_fascia_board', xr0, xr1, *(sorted([ye, ye + sgn * .04])), roof_z(ye) - .12, roof_z(ye) + .12, 'DarkWood')
        gy = ye + sgn * .10
        bx(s, 'Galvanized_box_gutter_floor', xr0, xr1, *(sorted([gy - .07, gy + .07])), roof_z(ye) - .08, roof_z(ye) - .065, 'FeedGalv')
        bx(s, 'Galvanized_box_gutter_outer', xr0, xr1, *(sorted([gy + sgn * .065, gy + sgn * .075])), roof_z(ye) - .08, roof_z(ye) + .04, 'FeedGalv')
        bx(s, 'Galvanized_box_gutter_inner', xr0, xr1, *(sorted([gy - sgn * .075, gy - sgn * .065])), roof_z(ye) - .08, roof_z(ye) - .01, 'FeedGalv')
        for xe in [xr0, xr1 - .005]:
            bx(s, 'Gutter_end_cap', xe, xe + .005, gy - .075, gy + .075, roof_z(ye) - .08, roof_z(ye) + .04, 'FeedGalv')
        # Downspouts down the side wall, with elbows and a splash block.
        for xd in [X0 + .35, STORE_X - .45, -.55]:
            yw = sgn * (12 + FF + .07)
            beam(s, 'Downspout_elbow', (xd, gy, roof_z(ye) - .08), (xd, yw, roof_z(ye) - .45), .07, .055, 'FeedGalv')
            bx(s, 'Rect_downspout', xd - .035, xd + .035, yw - .03, yw + .03, .32, roof_z(ye) - .40, 'FeedGalv')
            beam(s, 'Downspout_shoe', (xd, yw, .36), (xd, yw + sgn * .20, .10), .07, .055, 'FeedGalv')
            bx(s, 'Concrete_splash_block', xd - .16, xd + .16, *(sorted([yw + sgn * .08, yw + sgn * .50])), 0, .05, 'Concrete')
            if lod < 2:
                for zz in [1.4, 3.3]:
                    bx(s, 'Downspout_strap', xd - .045, xd + .045, *(sorted([yw - sgn * .04, yw + sgn * .04])), zz, zz + .03, 'DarkMetal')
    # Ridge cap and rake trims
    for sgn in [-1, 1]:
        beam(s, 'Ridge_cap_flashing', (xr0, sgn * .005, roof_z(0) + .135), (xr0, sgn * .26, roof_z(.26) + .127), .006, .006, 'FeedGalv')
        v = [(xr0, 0, roof_z(0) + .13), (xr1, 0, roof_z(0) + .13), (xr1, sgn * .26, roof_z(.26) + .13 + .002), (xr0, sgn * .26, roof_z(.26) + .13 + .002)]
        hexa(s, 'Ridge_cap_flashing', v + [add(p, (0, 0, .012)) for p in v], 'FeedGalv')
        ye = sgn * 12.45
        beam(s, 'Rear_rake_trim', (xr0 - .02, 0, roof_z(0) + .08), (xr0 - .02, ye, roof_z(ye) + .08), .05, .20, 'FeedGalv', upv=(0, 0, 1))
    # Flashing where the roof dies into the back of the false front, and timber kickers.
    for sgn in [-1, 1]:
        v = [(xr1 - .25, 0, roof_z(0) + .14), (xr1, 0, roof_z(0) + .14), (xr1, sgn * 12.0, roof_z(12.0) + .14), (xr1 - .25, sgn * 12.0, roof_z(12.0) + .14)]
        hexa(s, 'Counter_flashing', v + [add(p, (0, 0, .03)) for p in v[:1]] + [add(v[1], (0, 0, .25)), add(v[2], (0, 0, .25))] + [add(v[3], (0, 0, .03))], 'FeedGalv')
        for yk in ([2.6, 6.8, 10.6] if lod < 2 else [6.8]):
            y = sgn * yk
            zt = 6.2 if yk > 7.6 else (6.65 if yk > 3.8 else 7.2)
            beam(s, 'False_front_kicker_brace', (-FF - .05, y, zt), (-2.2, y, roof_z(y) + .17), .09, .14, 'Timber')
            bx(s, 'Kicker_roof_block', -2.45, -2.0, y - .1, y + .1, roof_z(y) + .10, roof_z(y) + .2, 'Timber')
    # Back face of the false front is plain plaster; vertical timber strongbacks behind the steps.
    for y in [-7.6, -3.8, 0, 3.8, 7.6]:
        z_top = 6.2 if abs(y) > 7.0 else (6.7 if abs(y) > 3.0 else 7.3)
        bx(s, 'False_front_strongback', -FF - .12, -FF, y - .05, y + .05, roof_z(y) + .1, z_top, 'Timber')

    # --- Roof equipment: swamp cooler on a level stand, turbine vents, stovepipe.
    cx, cy = -6.2, 5.0
    zb = roof_z(cy + .7) + .12
    for dx in [-.55, .55]:
        for dy in [-.55, .55]:
            beam(s, 'Cooler_stand_leg', (cx + dx, cy + dy, roof_z(cy + dy) + .1), (cx + dx, cy + dy, zb + .5), .07, .07, 'Timber')
    bx(s, 'Cooler_stand_deck', cx - .62, cx + .62, cy - .62, cy + .62, zb + .5, zb + .56, 'Timber')
    s.rounded_box('Evaporative_swamp_cooler', (cx, cy, zb + .56 + .48), (1.02, 1.02, .96), .03, 'FeedGalv', 2)
    bx(s, 'Cooler_lid_cap', cx - .54, cx + .54, cy - .54, cy + .54, zb + 1.52, zb + 1.56, 'FeedGalv')
    louv = [9, 5, 0][lod]
    for side in [(1, 0), (-1, 0), (0, 1)]:
        for k in range(louv):
            zz = zb + .70 + k * .075
            if side[0]:
                xx = cx + side[0] * .515
                bx(s, 'Cooler_louvre', min(xx, xx + side[0] * .025), max(xx, xx + side[0] * .025), cy - .40, cy + .40, zz, zz + .022, 'FeedGalv')
            else:
                yy = cy + side[1] * .515
                bx(s, 'Cooler_louvre', cx - .40, cx + .40, min(yy, yy + side[1] * .025), max(yy, yy + side[1] * .025), zz, zz + .022, 'FeedGalv')
    beam(s, 'Cooler_supply_duct', (cx, cy - .3, zb + .58), (cx, cy - .3, roof_z(cy - .3) + .05), .48, .48, 'FeedGalv', upv=(1, 0, 0))
    bx(s, 'Cooler_duct_curb', cx - .34, cx + .34, cy - .64, cy + .04, roof_z(cy + .04) + .08, roof_z(cy - .64) + .18, 'FeedGalv')
    for x, y in [(-10.5, -5.2), (-3.8, -7.8), (-13.0, 6.4)][:3 if lod < 2 else 1]:
        z = roof_z(y) + .1
        s.lathe('Turbine_vent_throat', (x, y, z - .05), [(0, .17), (.30, .15)], 'FeedGalv', n, caps=False)
        s.lathe('Turbine_vent_head', (x, y, z + .25), [(0, .17), (.08, .27), (.24, .27), (.33, .17), (.36, .04)], 'FeedGalv', n)
        if lod == 0:
            for k in range(12):
                a = k * math.tau / 12
                s.tube('Turbine_vane_ridge', (x + .275 * math.cos(a), y + .275 * math.sin(a), z + .33), (x + .275 * math.cos(a + .35), y + .275 * math.sin(a + .35), z + .49), .008, 'FeedGalv', 4, caps=False)
    x, y = -12.6, -3.2
    s.lathe('Stovepipe_roof_jack', (x, y, roof_z(y) + .08), [(0, .32), (.06, .14), (.25, .12)], 'FeedGalv', n)
    s.lathe('Stovepipe_flue', (x, y, roof_z(y) + .30), [(0, .10), (2.1, .10)], 'DarkMetal', n, caps=False)
    s.lathe('Stovepipe_rain_cap', (x, y, roof_z(y) + 2.42), [(0, .03), (.02, .24), (.12, .02)], 'DarkMetal', n)
    for a in range(3):
        aa = a * math.tau / 3
        s.tube('Rain_cap_leg', (x + .095 * math.cos(aa), y + .095 * math.sin(aa), roof_z(y) + 2.36), (x + .16 * math.cos(aa), y + .16 * math.sin(aa), roof_z(y) + 2.45), .006, 'DarkMetal', 4)
    if lod < 2:
        for k in range(3):
            aa = k * math.tau / 3 + .4
            s.tube('Stovepipe_guy_wire', (x, y, roof_z(y) + 1.9), (x + 1.2 * math.cos(aa), y + 1.2 * math.sin(aa), roof_z(y + 1.2 * math.sin(aa)) + .11), .005, 'DarkMetal', 4)


def storefront(s, g, f, lod):
    """Timber shopfront infill in the 5.2 x 3.6 m aperture: bulkheads, closed glazed pair, transom."""
    y0, y1 = DOOR_Y - DOOR_W / 2, DOOR_Y + DOOR_W / 2
    xf = -.05                                   # infill plane, recessed into the 0.22 m wall
    bx(s, 'Storefront_sub_sill_fill', -FF, -.0, y0, y1, 0, FLOOR, 'Concrete')
    bx(s, 'Storefront_oak_threshold', xf - .06, .10, y0, y1, FLOOR - .02, FLOOR + .05, 'DarkWood')
    mull = [y0 + .05, -.30, 1.30, y1 - .05]
    for y in mull:
        bx(s, 'Storefront_mullion', xf - .06, xf + .06, y - .05, y + .05, FLOOR + .05, DOOR_H, 'DarkWood')
    bx(s, 'Storefront_head', xf - .06, xf + .07, y0, y1, DOOR_H - .10, DOOR_H, 'DarkWood')
    bx(s, 'Storefront_transom_bar', xf - .06, xf + .08, y0, y1, 3.00, 3.10, 'DarkWood')
    nt = [6, 4, 2][lod]
    for k in range(1, nt):
        y = y0 + k * (y1 - y0) / nt
        bx(s, 'Transom_muntin', xf - .03, xf + .03, y - .025, y + .025, 3.10, DOOR_H - .10, 'DarkWood')
    pane_x(g, xf - .005, y0 + .05, y1 - .05, 3.10, DOOR_H - .10)
    # Sidelights with panelled bulkheads
    for a, b in [(mull[0] + .05, mull[1] - .05), (mull[2] + .05, mull[3] - .05)]:
        bx(s, 'Sidelight_bulkhead', xf - .05, xf + .03, a, b, FLOOR + .05, 1.50, 'Timber')
        bx(s, 'Bulkhead_raised_panel', xf + .03, xf + .05, a + .08, b - .08, FLOOR + .14, 1.38, 'Timber')
        bx(s, 'Sidelight_sill', xf - .05, xf + .08, a, b, 1.50, 1.56, 'DarkWood')
        bx(s, 'Sidelight_glazing_bead', xf - .02, xf + .03, a, b, 2.94, 3.0, 'DarkWood')
        pane_x(g, xf - .005, a, b, 1.56, 2.94)
        if lod < 2:
            bx(s, 'Sidelight_centre_bar', xf - .02, xf + .025, (a + b) / 2 - .025, (a + b) / 2 + .025, 1.56, 2.94, 'DarkWood')
    # Closed pair of glazed doors
    for a, b in [(mull[1] + .05, DOOR_Y - .005), (DOOR_Y + .005, mull[2] - .05)]:
        x = xf + .005
        bx(s, 'Door_leaf_bottom_rail', x - .025, x + .035, a, b, FLOOR + .05, FLOOR + .45, 'Timber')
        bx(s, 'Door_leaf_top_rail', x - .025, x + .035, a, b, 2.88, 3.0, 'Timber')
        for yy in [a + .055, b - .055]:
            bx(s, 'Door_leaf_stile', x - .025, x + .035, yy - .055, yy + .055, FLOOR + .05, 3.0, 'Timber')
        pane_x(g, x - .01, a + .11, b - .11, FLOOR + .45, 2.88)
        if lod < 2:
            bx(s, 'Door_muntin', x - .01, x + .02, (a + b) / 2 - .018, (a + b) / 2 + .018, FLOOR + .45, 2.88, 'Timber')
            for zz in [1.93, 2.40]:
                bx(s, 'Door_muntin', x - .01, x + .02, a + .11, b - .11, zz - .018, zz + .018, 'Timber')
        bx(f, 'Brass_kick_plate', x + .035, x + .04, a + .03, b - .03, FLOOR + .07, FLOOR + .32, 'DarkMetal')
        yh = b - .12 if a < DOOR_Y else a + .12
        f.tube('Door_pull_handle', (x + .09, yh, 1.85), (x + .09, yh, 2.25), .014, 'Metal', 6)
        for zz in [1.88, 2.22]:
            f.tube('Door_pull_post', (x + .035, yh, zz), (x + .09, yh, zz), .01, 'Metal', 6)
    f.panel('Door_closed_card', (xf - .03, DOOR_Y - .45, 2.45), .30, .17, 15)
    f.lathe('Door_lock_cylinder', (xf + .04, DOOR_Y - .07, 1.95), [(0, .022), (.02, .02)], 'Metal', 8, axis=(1, 0, 0))


def pane_x(g, x, y0, y1, z0, z1):
    g.tag = 'Thin_actual_glazing'
    oq(g, [(x, y0, z0), (x, y1, z0), (x, y1, z1), (x, y0, z1)], 'Glass', (1, 0, 0), [(0, 0), (1, 0), (1, 1), (0, 1)])


def windows(s, g, f, lod):
    for y, w, z0, z1 in WINDOWS:
        a, b = y - w / 2, y + w / 2
        xf = -.04
        for yy in [a + .05, b - .05]:
            bx(s, 'Window_frame_jamb', xf - .06, xf + .07, yy - .05, yy + .05, z0, z1, 'DarkWood')
        bx(s, 'Window_frame_head', xf - .06, xf + .07, a, b, z1 - .10, z1, 'DarkWood')
        bx(s, 'Window_frame_sill', xf - .06, xf + .09, a, b, z0, z0 + .08, 'DarkWood')
        bx(s, 'Window_transom_rail', xf - .05, xf + .06, a, b, z1 - .55, z1 - .47, 'DarkWood')
        bx(s, 'Window_centre_mullion', xf - .05, xf + .06, y - .045, y + .045, z0 + .08, z1 - .55, 'DarkWood')
        pane_x(g, xf - .01, a + .1, b - .1, z0 + .08, z1 - .10)
        if lod < 2:
            for k in range(1, 4):
                yy = a + .1 + k * (w - .2) / 4
                bx(s, 'Window_transom_muntin', xf - .02, xf + .03, yy - .02, yy + .02, z1 - .47, z1 - .10, 'DarkWood')
            for half in [(a + .1, y - .045), (y + .045, b - .1)]:
                for k in [1, 2]:
                    zz = z0 + .08 + k * (z1 - .55 - z0 - .08) / 3
                    bx(s, 'Window_sash_muntin', xf - .02, xf + .03, half[0], half[1], zz - .018, zz + .018, 'DarkWood')
                yy = (half[0] + half[1]) / 2
                bx(s, 'Window_sash_muntin', xf - .02, xf + .03, yy - .018, yy + .018, z0 + .08, z1 - .55, 'DarkWood')
        # Projecting concrete sill and a timber lintel board on the stucco.
        bx(s, 'Window_concrete_sill', -FF, FF + .08, a - .10, b + .10, z0 - .09, z0 - .0, 'Concrete')
        bx(s, 'Window_lintel_board', FF, FF + .035, a - .14, b + .14, z1, z1 + .20, 'Timber')


def awning_and_dock(s, f, lod):
    n = seg(lod); cs = [6, 3, 2][lod]; per = .19 if lod < 2 else .38
    ya, yb = -4.6, 6.6
    x_in, x_out, z_in, z_out = FF, 2.42, 4.42, 4.00
    # Ledger, cantilevered outriggers, purlins, fascia.
    bx(s, 'Awning_ledger', FF, FF + .09, ya, yb, z_in - .30, z_in - .08, 'DarkWood')
    nr = [9, 7, 4][lod]
    for k in range(nr):
        y = ya + .08 + k * (yb - ya - .16) / (nr - 1)
        beam(s, 'Awning_outrigger', (FF + .09, y, z_in - .13), (x_out - .05, y, z_out - .13), .075, .19, 'DarkWood')
    for t in ([.33, .66] if lod < 2 else [.5]):
        x = x_in + t * (x_out - x_in); z = z_in + t * (z_out - z_in)
        bx(s, 'Awning_purlin', x - .045, x + .045, ya, yb, z - .03 - .04, z - .03, 'Timber')
    bx(s, 'Awning_fascia_board', x_out - .07, x_out - .02, ya - .02, yb + .02, z_out - .26, z_out - .02, 'Timber')
    for y in [ya, yb]:
        beam(s, 'Awning_end_rafter', (FF + .09, y, z_in - .13), (x_out - .05, y, z_out - .13), .06, .19, 'Timber')
    V = unit((x_out + .08 - x_in, 0, (z_out - .015) - z_in))
    L = math.hypot(x_out + .08 - x_in, z_out - z_in)
    corr_sheet(s, 'Awning_corrugated_tin', (x_in, ya - .05, z_in + .005), (0, 1, 0), yb - ya + .10, V, L, (0, 0, 1), per, .02, cs, 'FeedRustTin', under=True)
    bx(s, 'Awning_wall_flashing', FF, FF + .03, ya - .05, yb + .05, z_in - .01, z_in + .16, 'FeedGalv')
    # Knee braces where wall is solid, tie rods to the false front at the ends (no posts).
    for y in [-3.4, 4.1]:
        beam(s, 'Awning_knee_brace', (FF + .02, y, 2.95), (1.45, y, z_in - .13 - 1.43 * (z_in - z_out) / (x_out - x_in) - .08), .07, .09, 'DarkWood')
        bx(f, 'Brace_heel_strap', FF, FF + .012, y - .05, y + .05, 2.85, 3.10, 'DarkMetal')
    for y in [ya + .1, yb - .1]:
        if y is None:
            continue
        if abs(y) < 4:   # middle rod anchors above the sign
            a = (x_out - .12, y, z_out + .05); b = (FF + .02, y, 6.32)
        else:
            a = (x_out - .12, y, z_out + .05); b = (FF + .02, y, 5.9)
        f.tube('Awning_tie_rod', a, b, .013, 'DarkMetal', 6)
        f.lathe('Tie_rod_wall_eye', b, [(0, .05), (.015, .05)], 'DarkMetal', 8, axis=(1, 0, 0))
        f.tube('Tie_rod_turnbuckle', add(a, mul(unit(sub(b, a)), .5)), add(a, mul(unit(sub(b, a)), .62)), .022, 'DarkMetal', 6)
    # Under-awning porch lights (enamel shade, emissive opal) on short stems.
    for y in [-2.6, 3.9]:
        x = 1.3; zc = z_in + (z_out - z_in) * (x - x_in) / (x_out - x_in) - .32
        pendant(f, x, y, zc - .25, lod)

    # --- Loading dock: same volume as TB1, now concrete, timber deck, steel edge, bumpers.
    x0, x1, dy0, dy1, dz0, dz1 = DOCK
    bx(s, 'Dock_concrete_body', x0, x1 - .12, dy0, dy1, dz0, dz1 - .09, 'Concrete')
    planks = [17, 9, 1][lod]
    pw = (x1 - .04 - x0) / planks
    for k in range(planks):
        xa = x0 + k * pw
        bx(s, 'Dock_deck_plank', xa + .006, xa + pw - .006, dy0, dy1, dz1 - .09, dz1 - .002 * (k % 3), 'Timber' if k % 5 else 'DarkWood')
    bx(s, 'Dock_steel_edge_angle', x1 - .14, x1, dy0, dy1, dz1 - .2, dz1, 'DarkMetal')
    bx(s, 'Dock_face_backing', x1 - .12, x1 - .06, dy0, dy1, dz0 + .1, dz1 - .2, 'Concrete')
    for y in [-3.0, -0.5, 2.5, 5.0][:4 if lod < 2 else 2]:
        bx(s, 'Rubber_dock_bumper', x1 - .06, x1, y - .22, y + .22, .30, .78, 'Rubber')
        if lod < 2:
            for zz in []:
                f.lathe('Bumper_anchor_bolt', (x1 - .002, y + .16, zz), [(0, .018), (.008, .012)], 'Metal', 6, axis=(1, 0, 0))
                f.lathe('Bumper_anchor_bolt', (x1 - .002, y - .16, zz), [(0, .018), (.008, .012)], 'Metal', 6, axis=(1, 0, 0))
    for y in [dy0, dy1]:
        sgn = -1 if y == dy0 else 1
        bx(s, 'Dock_end_steel_angle', x0 + .2, x1, *(sorted([y, y - sgn * .012])), dz1 - .2, dz1, 'DarkMetal')
    # Stock on the dock: feed sacks on a pallet, salt blocks, tubs, crate, hand truck.
    sack_stack(s, 1.25, 4.55, dz1, 7, lod, 1.2, 1.0, skip={(6, 1)})
    sack_stack(s, 1.15, -3.15, dz1, 4, lod, 1.2, 1.0, skip={(3, 0)})
    top = pallet(s, 1.95, 1.7, dz1, lod, 1.0, .8)
    for i, (dx, dy, dz, mat) in enumerate([(-.2, -.2, 0, 'FeedSalt'), (.1, -.2, 0, 'FeedSalt'), (-.2, .15, 0, 'FeedMineral'), (.1, .15, 0, 'FeedSalt'), (-.05, -.05, .25, 'FeedMineral')]):
        if lod == 2 and i > 2:
            break
        s.rounded_box('Pressed_salt_block', (1.95 + dx + .05, 1.7 + dy + .02, top + dz + .125), (.25, .25, .25), .02, mat, 1)
    for y, z in [(-1.75, dz1)] + ([(-1.2, dz1)] if lod < 2 else []):
        bin_(s, .62, y, z, lod, .26, .62, mat='Metal')
    s.lathe('Galvanized_wash_tub', (1.7, -1.6, dz1), [(0, .30), (.02, .31), (.30, .40), (.32, .42)], 'Metal', seg(lod), caps=False)
    s.lathe('Wash_tub_inner', (1.7, -1.6, dz1 + .32), [(0, .41), (-.30, .30)], 'Metal', seg(lod), caps=False)
    s.lathe('Wash_tub_floor', (1.7, -1.6, dz1 + .02), [(0, .30), (0.0005, .001)], 'Metal', seg(lod), caps=False)
    # Hand truck leaning on the facade, right of the door.
    hx, hy = .30, 3.55
    for dy in [-.18, .18]:
        f.tube('Hand_truck_rail', (hx + .30, hy + dy, dz1 + .08), (hx + .06, hy + dy, dz1 + 1.32), .016, 'FeedRedPaint', 6)
    for k, t in enumerate([.25, .5, .75] if lod < 2 else [.5]):
        x = hx + .30 - t * .24; z = dz1 + .08 + t * 1.24
        f.tube('Hand_truck_crossbar', (x, hy - .18, z), (x, hy + .18, z), .011, 'FeedRedPaint', 6)
    f.tube('Hand_truck_grip', (hx + .06, hy - .18, dz1 + 1.32), (hx + .02, hy - .12, dz1 + 1.40), .016, 'FeedRedPaint', 6)
    f.tube('Hand_truck_grip', (hx + .06, hy + .18, dz1 + 1.32), (hx + .02, hy + .12, dz1 + 1.40), .016, 'FeedRedPaint', 6)
    bx(f, 'Hand_truck_nose_plate', hx + .30, hx + .56, hy - .19, hy + .19, dz1, dz1 + .012, 'DarkMetal')
    f.tube('Hand_truck_axle', (hx + .25, hy - .25, dz1 + .13), (hx + .25, hy + .25, dz1 + .13), .012, 'DarkMetal', 6)
    for dy in [-.25, .25]:
        f.torus('Hand_truck_tyre', (hx + .25, hy + dy, dz1 + .13), .10, .035, 'Rubber', [12, 8, 6][lod], [5, 4, 3][lod], axis=(0, 1, 0))
        f.lathe('Hand_truck_hub', (hx + .25, hy + dy - .025, dz1 + .13), [(0, .05), (.05, .05)], 'DarkMetal', 8, axis=(0, 1, 0))
    # Wooden crate by the left end.
    bx(s, 'Slatted_crate_core', 1.75, 2.30, -3.92, -3.37, dz1 + .001, dz1 + .45, 'DarkWood')
    if lod < 2:
        for sgn in [-1, 1]:
            for zz in [.06, .22, .38]:
                bx(s, 'Crate_slat', 1.73, 2.32, *(sorted([-3.645 + sgn * .275, -3.645 + sgn * .29])), dz1 + zz - .05, dz1 + zz + .05, 'Timber')

    # --- Ground-level props along the walls (within 0.6 m): bench, hay, propane cage.
    bx(s, 'Bench_seat_plank', .16, .52, -7.45, -5.45, .44, .48, 'Timber')
    bx(s, 'Bench_back_rail', .14, .19, -7.45, -5.45, .70, .84, 'Timber')
    for y in [-7.3, -5.6]:
        bx(s, 'Bench_leg_frame', .20, .48, y - .03, y + .03, 0, .44, 'DarkWood')
        beam(s, 'Bench_back_post', (.18, y, .44), (.16, y, .86), .05, .05, 'DarkWood')
    for k, (x, y, z, yaw) in enumerate([(.36, 9.2, 0, 0), (.36, 10.35, 0, 0), (.37, 9.75, .36, 0)][:3 if lod < 2 else 2]):
        s.rounded_box('Small_square_hay_bale', (x, y, z + .18), (.45, 1.02, .36), .04, 'FeedHay', 2)
        if lod < 2:
            for dy in [-.25, .25]:
                beam(s, 'Bale_twine', (x - .228, y + dy, z + .005), (x - .228, y + dy, z + .355), .008, .01, 'Burlap')
                beam(s, 'Bale_twine', (x - .228, y + dy, z + .363), (x + .228, y + dy, z + .363), .008, .01, 'Burlap')
    # Propane cylinder cage against the stucco side wall (+y), within 0.6 m.
    gy0 = 12 + FF + .02; gy1 = gy0 + .55; gx0, gx1 = -3.6, -2.2; gh = 1.45
    for x in [gx0, gx1]:
        for y in [gy0, gy1]:
            f.tube('Cage_corner_post', (x, y, 0), (x, y, gh), .02, 'DarkMetal', 6)
    for z in [.06, .75, gh]:
        for a, b in [((gx0, gy1, z), (gx1, gy1, z)), ((gx0, gy0, z), (gx0, gy1, z)), ((gx1, gy0, z), (gx1, gy1, z))]:
            f.tube('Cage_rail', a, b, .014, 'DarkMetal', 6)
    nb = [9, 5, 0][lod]
    for k in range(nb):
        x = gx0 + (k + 1) * (gx1 - gx0) / (nb + 1)
        f.tube('Cage_bar', (x, gy1, .06), (x, gy1, gh), .007, 'DarkMetal', 4, caps=False)
    bx(f, 'Cage_roof_sheet', gx0 - .03, gx1 + .03, gy0, gy1 + .03, gh, gh + .02, 'FeedGalv')
    for k in range(4):
        x = gx0 + .2 + k * .33
        y = gy0 + .25
        s.lathe('Propane_cylinder', (x, y, .02), [(0, .14), (.04, .155), (.92, .155), (1.0, .11), (1.04, .05)], 'WhitePaint', [16, 10, 8][lod])
        s.lathe('Propane_valve_collar', (x, y, 1.04), [(0, .055), (.12, .06), (.13, .035)], 'Metal', 8, caps=False)


def services(s, f, lod):
    n = seg(lod)
    # Electric service on the +y store wall: meter base, disconnect, conduit and mast with weatherhead.
    y = 12 + FF
    mx = -6.3
    bx(f, 'Meter_base_box', mx - .16, mx + .16, y, y + .12, 1.35, 1.80, 'FeedGalv')
    f.lathe('Meter_glass_dome', (mx, y + .12, 1.60), [(0, .10), (.03, .10), (.10, .085), (.12, .05), (.125, .001)], 'Ceramic', [16, 10, 8][lod], axis=(0, 1, 0), caps=False)
    bx(f, 'Service_disconnect', mx + .30, mx + .62, y, y + .14, 1.20, 1.66, 'OliveEnamel')
    f.tube('Disconnect_handle', (mx + .63, y + .10, 1.42), (mx + .63, y + .10, 1.56), .012, 'FeedRedPaint', 6)
    f.tube('Service_mast_conduit', (mx, y + .07, 1.80), (mx, y + .07, 6.55), .03, 'FeedGalv', 8)
    f.tube('Feeder_conduit_to_disconnect', (mx + .16, y + .05, 1.45), (mx + .30, y + .05, 1.45), .018, 'FeedGalv', 6)
    f.tube('Ground_conduit', (mx + .46, y + .05, 1.20), (mx + .46, y + .05, .05), .016, 'FeedGalv', 6)
    for zz in [2.6, 3.9, 5.0]:
        bx(f, 'Mast_wall_strap', mx - .05, mx + .05, y, y + .045, zz, zz + .04, 'DarkMetal')
    f.lathe('Weatherhead_cap', (mx, y + .07, 6.55), [(0, .045), (.05, .06), (.10, .055), (.13, .001)], 'DarkMetal', 8)
    f.tube('Mast_roof_boot', (mx, y + .07, roof_z(12.2) + .02), (mx, y + .07, roof_z(12.2) + .22), .07, 'Rubber', 8)
    # Service drip loops: three conductors leave the weatherhead, sag and dead-end on a
    # spool rack on the mast. The span to the street pole is owned by the town wiring.
    rack = (mx, y + .07, 6.18)
    f.tube('Spool_rack_arm', rack, add(rack, (0, .34, 0)), .014, 'DarkMetal', 6)
    for k in range(3 if lod < 2 else 1):
        sp = add(rack, (0, .12 + k * .10, 0))
        f.lathe('Spool_insulator', add(sp, (0, 0, -.04)), [(0, .028), (.02, .036), (.06, .036), (.08, .028)], 'Ceramic', 8)
        if lod < 2:
            pts = []
            for t in range(9):
                u = t / 8
                pts.append(add((mx + .03 * (k - 1), y + .07 + .05, 6.62), (0, u * (.12 + k * .10 - .05), -.30 * math.sin(math.pi * u) - .44 * u)))
            for a_, b_ in zip(pts, pts[1:]):
                f.tube('Service_drip_loop', a_, b_, .008, 'Rubber', 4, caps=False)

    # Gooseneck barn lights over the sign and over the warehouse door.
    for yy in ([-2.4, 0.0, 2.4] if lod < 2 else [-2.4, 2.4]):
        gooseneck(f, (FF + .035, yy, 6.30), (1, 0, 0), lod)
    gooseneck(f, (-12.9, 12 + FF + .03, 3.72), (0, 1, 0), lod)
    gooseneck(f, (-3.2, -12 - FF - .03, 3.2), (0, -1, 0), lod)

    # Sliding warehouse stock door on the +y corrugated wall, track, hangers, stop and handle.
    x0d, x1d, h = -14.6, -11.3, 3.25
    yd = 12 + FF + .07
    nbd = [11, 6, 1][lod]
    for k in range(nbd):
        xa = x0d + k * (x1d - x0d) / nbd
        bx(s, 'Sliding_door_board', xa + .004, xa + (x1d - x0d) / nbd - .004, yd - .025, yd + .025, .08, h, 'Timber' if k % 4 else 'DarkWood')
    for z in [.25, h / 2, h - .2]:
        bx(s, 'Sliding_door_rail', x0d, x1d, yd + .025, yd + .065, z - .09, z + .09, 'DarkWood')
    beam(s, 'Sliding_door_z_brace', (x0d + .15, yd + .045, .35), (x1d - .15, yd + .045, h / 2 - .1), .14, .04, 'DarkWood', upv=(0, 1, 0))
    beam(s, 'Sliding_door_z_brace', (x0d + .15, yd + .045, h / 2 + .1), (x1d - .15, yd + .045, h - .3), .14, .04, 'DarkWood', upv=(0, 1, 0))
    bx(f, 'Barn_door_box_track', x0d - .2, x1d + 3.4, yd - .02, yd + .07, h + .12, h + .22, 'DarkMetal')
    for k in range(5):
        xx = x0d - .1 + k * (x1d + 3.3 - x0d) / 4
        bx(f, 'Track_wall_bracket', xx - .04, xx + .04, 12 + FF, yd - .02, h + .10, h + .24, 'DarkMetal')
    for xx in [x0d + .35, x1d - .35]:
        bx(f, 'Door_hanger_strap', xx - .04, xx + .04, yd + .025, yd + .04, h - .25, h + .16, 'DarkMetal')
        f.lathe('Hanger_trolley_wheel', (xx, yd + .02, h + .17), [(0, .055), (.05, .055)], 'DarkMetal', 10, axis=(0, 1, 0))
    bx(f, 'Floor_door_guide', x0d - .1, x0d + .1, yd - .05, yd + .06, 0, .08, 'DarkMetal')
    f.tube('Sliding_door_pull', (x1d - .25, yd + .09, 1.0), (x1d - .25, yd + .09, 1.5), .015, 'DarkMetal', 6)
    f.tube('Hasp_staple', (x1d + .05, yd + .02, 1.25), (x1d + .05, yd + .07, 1.25), .012, 'DarkMetal', 6)
    f.torus('Door_padlock_shackle', (x1d + .05, yd + .08, 1.17), .03, .007, 'Metal', 8, 4, axis=(0, 1, 0))
    bx(f, 'Door_padlock_body', x1d + .02, x1d + .08, yd + .07, yd + .10, 1.05, 1.15, 'Metal')


# ---------------------------------------------------------------- interior
def interior(i, f, lod):
    n = seg(lod)
    xa, xb, ya, yb = X0 + FF, X1 - FF, Y0 + FF, Y1 - FF
    bx(i, 'Plank_floor_authority', xa, xb, ya, yb, FLOOR - .10, FLOOR, 'Timber')
    if lod < 2:
        for k in range(1, 24):
            y = ya + k * (yb - ya) / 24
            bx(i, 'Floor_board_joint', STORE_X, xb, y - .006, y + .006, FLOOR, FLOOR + .002, 'DarkWood')
    bx(i, 'Board_ceiling', xa, xb, ya, yb, 5.02, 5.10, 'Timber')
    for x in ([-1.6, -4.0, -6.4, -8.8, -11.6, -14.2] if lod < 2 else [-4.0, -8.8]):
        bx(i, 'Exposed_ceiling_beam', x - .10, x + .10, ya, yb, 4.74, 5.02, 'DarkWood')
    bx(i, 'Exposed_ceiling_girder', xa, xb, -.13, .13, 4.70, 4.74, 'DarkWood')
    # Board-and-batten partition to the warehouse, with a wide open passage.
    for a, b in [(ya, -1.3), (1.3, yb)]:
        bx(i, 'Warehouse_partition', STORE_X - .08, STORE_X + .08, a, b, FLOOR, 5.02, 'Clapboard')
        if lod < 2:
            nb = int((b - a) / .6)
            for k in range(nb):
                y = a + (k + .5) * (b - a) / nb
                bx(i, 'Partition_batten', STORE_X + .08, STORE_X + .11, y - .03, y + .03, FLOOR, 5.02, 'Clapboard')
    for a, b in [(ya, -1.3), (1.3, yb)]:
        i.tag = 'Lamplit_partition_face'
        oq(i, [(STORE_X + .086, a, FLOOR), (STORE_X + .086, b, FLOOR), (STORE_X + .086, b, 5.0), (STORE_X + .086, a, 5.0)], 'FeedWallGlow', (1, 0, 0))
    bx(i, 'Partition_passage_header', STORE_X - .08, STORE_X + .08, -1.3, 1.3, 3.4, 5.02, 'Clapboard')
    for y in [-1.3, 1.3]:
        bx(i, 'Passage_casing', STORE_X + .08, STORE_X + .13, y - .07, y + .07, FLOOR, 3.45, 'DarkWood')
    # Wainscot boards on the stucco store walls.
    for sgn in [-1, 1]:
        yw = sgn * (12 - FF)
        bx(i, 'Store_wall_wainscot', STORE_X + .1, xb, *(sorted([yw, yw - sgn * .02])), FLOOR, FLOOR + 1.1, 'DarkWood')
    for y0, y1 in [(ya, -7.2), (-4.4, 5.0), (7.8, yb)]:
        bx(i, 'Front_wall_wainscot', xb - .02, xb, y0, y1, FLOOR, FLOOR + .25 if (y0 < DOOR_Y < y1) else FLOOR + .25, 'DarkWood')

    # Sales counter with panelled front, register, scale and paperwork.
    cx0, cx1, cy0, cy1 = -3.4, -2.7, 1.9, 6.6
    bx(i, 'Counter_carcass', cx0, cx1, cy0, cy1, FLOOR, FLOOR + .92, 'Timber')
    bx(i, 'Counter_worn_top', cx0 - .04, cx1 + .06, cy0 - .05, cy1 + .05, FLOOR + .92, FLOOR + .97, 'DarkWood')
    bx(i, 'Counter_kick', cx1, cx1 + .03, cy0, cy1, FLOOR, FLOOR + .12, 'DarkWood')
    if lod < 2:
        npnl = 6
        for k in range(npnl):
            ya_ = cy0 + .08 + k * (cy1 - cy0 - .16) / npnl
            bx(i, 'Counter_front_panel', cx1, cx1 + .02, ya_ + .05, ya_ + (cy1 - cy0 - .16) / npnl - .05, FLOOR + .2, FLOOR + .82, 'DarkWood')
    zt = FLOOR + .97
    i.rounded_box('Brass_cash_register_body', (-3.05, 5.4, zt + .17), (.48, .42, .34), .04, 'DarkMetal', 2)
    i.rounded_box('Register_drawer', (-2.9, 5.4, zt + .04), (.5, .44, .08), .01, 'DarkMetal', 1)
    i.rounded_box('Register_display_head', (-3.15, 5.4, zt + .44), (.2, .30, .18), .03, 'Metal', 2)
    if lod == 0:
        for k in range(16):
            i.lathe('Register_key', (-2.88 + (k // 4) * .035, 5.27 + (k % 4) * .08, zt + .30 - (k // 4) * .03), [(0, .016), (.012, .016)], 'Ceramic', 6)
    i.lathe('Platform_scale_base', (-3.0, 3.0, zt), [(0, .18), (.08, .16), (.10, .05)], 'OliveEnamel', n)
    i.lathe('Scale_dial', (-3.12, 3.0, zt + .35), [(0, .13), (.05, .13)], 'CreamEnamel', n, axis=(1, 0, 0))
    i.tube('Scale_column', (-3.1, 3.0, zt + .1), (-3.1, 3.0, zt + .25), .03, 'OliveEnamel', 6)
    i.lathe('Scale_pan', (-2.9, 3.0, zt + .12), [(0, .03), (.03, .18), (.05, .19)], 'Metal', n, caps=False)
    for k in range([4, 2, 1][lod]):
        bx(i, 'Order_slip', -3.2 + .03 * k, -2.95 + .03 * k, 4.2 + .02 * k, 4.4 + .02 * k, zt + .002 * (k + 1), zt + .002 * (k + 1) + .002, 'Paper')

    # Wall shelving along both store side walls, stocked with cans and boxes.
    for sgn in [-1, 1]:
        yw = sgn * (12 - FF - .02)
        ys = yw - sgn * .45
        for xs0, xs1 in [(-8.9, -5.9), (-5.6, -2.6)]:
            for x in [xs0, xs1]:
                bx(i, 'Shelf_upright', x - .04, x + .04, *(sorted([yw, ys])), FLOOR, FLOOR + 2.6, 'DarkWood')
            for z in [.15, .75, 1.35, 1.95, 2.5]:
                bx(i, 'Shelf_board', xs0, xs1, *(sorted([yw, ys])), FLOOR + z, FLOOR + z + .035, 'Timber')
                if z > 2.4:
                    continue
                cnt = [10, 6, 3][lod]
                for k in range(cnt):
                    x = xs0 + .15 + k * (xs1 - xs0 - .3) / max(1, cnt - 1)
                    yy = ys + sgn * .2
                    kind = (k + int(z * 10) + (sgn > 0)) % 4
                    zz = FLOOR + z + .035
                    if kind == 0:
                        i.lathe('Paint_or_feed_can', (x, yy, zz), [(0, .085), (.22, .085)], 'Metal' if k % 2 else 'CreamEnamel', [10, 8, 6][lod])
                    elif kind == 1:
                        bx(i, 'Boxed_stock', x - .12, x + .12, yy - .14, yy + .14, zz, zz + .30, 'Paper' if k % 3 else 'Burlap')
                    elif kind == 2:
                        i.lathe('Small_tin_can', (x - .06, yy, zz), [(0, .06), (.13, .06)], 'OliveEnamel', [10, 8, 6][lod])
                        i.lathe('Small_tin_can', (x + .07, yy, zz), [(0, .06), (.13, .06)], 'FeedRedPaint', [10, 8, 6][lod])
                    else:
                        i.rounded_box('Paper_seed_bag', (x, yy, zz + .14), (.18, .24, .28), .03, 'FeedSackPrint', 1)
    # Hanging tack and coiled rope on pegs along the -y wall above the shelving's end.
    for k, x in enumerate([-1.2, -1.75, -2.25] if lod < 2 else [-1.5]):
        i.torus('Hanging_leather_tack', (x, -11.75, FLOOR + 2.1), .28, .02, 'DarkWood', [16, 10, 6][lod], 4, axis=(0, 1, 0))
        i.torus('Coiled_lariat_rope', (x, -11.78, FLOOR + 1.45), .2, .025, 'Burlap', [16, 10, 6][lod], 4, axis=(0, 1, 0))
        i.tube('Tack_peg', (x, -11.89, FLOOR + 2.36), (x, -11.6, FLOOR + 2.38), .015, 'DarkWood', 6)
    # Long-handled tools in a rack by the partition.
    for k in range([6, 4, 2][lod]):
        y = -7.6 + k * .22
        i.tube('Tool_ash_handle', (STORE_X + .25, y, FLOOR + .05), (STORE_X + .18, y, FLOOR + 1.6), .016, 'Timber', 6)
        if k % 2:
            bx(i, 'Hay_fork_head', STORE_X + .14, STORE_X + .30, y - .12, y + .12, FLOOR + 1.6, FLOOR + 1.9, 'DarkMetal')
        else:
            bx(i, 'Shovel_blade', STORE_X + .23, STORE_X + .29, y - .11, y + .11, FLOOR + .02, FLOOR + .34, 'DarkMetal')
    bx(i, 'Tool_rack_rail', STORE_X + .1, STORE_X + .16, -7.8, -6.3, FLOOR + 1.45, FLOOR + 1.52, 'DarkWood')

    # Floor stock seen through the windows: sacks on pallets, galvanized bins, seed crates.
    sack_stack(i, -1.6, -6.9, FLOOR, 7 if lod < 2 else 5, lod)
    sack_stack(i, -4.6, -8.3, FLOOR, 5, lod)
    sack_stack(i, -1.7, 8.4, FLOOR, 6, lod)
    if lod < 2:
        sack_stack(i, -6.8, 8.9, FLOOR, 8, lod)
    for k, (x, y) in enumerate([(-5.2, -2.6), (-5.2, -1.9), (-5.2, -1.2), (-5.9, -2.25), (-1.6, 4.9), (-1.6, 5.6)]):
        if lod == 2 and k % 2:
            continue
        bin_(i, x, y, FLOOR, lod, .29, .74, lid=(k % 3 == 2))
    # Open seed bins with sloped fronts and visible grain.
    for k, y in enumerate([-4.6, -3.7]):
        x = -2.4
        bx(i, 'Seed_bin_back', x - .35, x - .30, y - .42, y + .42, FLOOR, FLOOR + .95, 'Timber')
        bx(i, 'Seed_bin_front', x + .30, x + .35, y - .42, y + .42, FLOOR, FLOOR + .55, 'Timber')
        for s_ in [-1, 1]:
            bx(i, 'Seed_bin_side', x - .35, x + .35, *(sorted([y + s_ * .42, y + s_ * .38])), FLOOR, FLOOR + .8, 'Timber')
        v = [(x - .30, y - .38, FLOOR + .78), (x + .30, y - .38, FLOOR + .48), (x + .30, y + .38, FLOOR + .48), (x - .30, y + .38, FLOOR + .78)]
        i.tag = 'Grain_heap_surface'
        oq(i, v, 'FeedGrain' if k == 0 else 'FeedHay', (0, 0, 1))
        i.panel('Seed_price_card', (x + .36, y, FLOOR + .43), .30, .22, 14)
    # Warehouse: deep sack stacks and hay visible through the passage.
    sack_stack(i, -12.2, -.5, FLOOR, 9 if lod < 2 else 5, lod)
    sack_stack(i, -12.2, 1.0, FLOOR, 7 if lod < 2 else 3, lod)
    for k in range([6, 4, 2][lod]):
        i.rounded_box('Stacked_hay_bale', (-14.7 + (k % 2) * .02, -2.5 + (k // 2 % 3) * 1.05, FLOOR + .18 + (k // 2) // 3 * .36 + (k % 2) * .36), (.45, 1.0, .36), .04, 'FeedHay', 2)

    # Lighting: enamel pendants in the store, one in the warehouse.
    for x, y in ([(-1.9, -5.8), (-1.9, 5.4), (-5.0, -5.0), (-5.0, 3.0), (-7.4, 0.0), (-12.5, 0.2)] if lod < 2 else [(-1.9, -5.8), (-1.9, 5.4), (-6.0, 0.0), (-12.5, .2)]):
        pendant(f, x, y, 4.22, lod, 'FeedBulbGlow', 'FeedShadeGlow')
        f.tube('Pendant_drop_stem', (x, y, 4.62), (x, y, 4.74), .01, 'DarkMetal', 6)


def build(lod=0):
    s = Mesh('SM_Town_Feed_Shell'); g = Mesh('SM_Town_Feed_Glass'); i = Mesh('SM_Town_Feed_Interior'); f = LodMesh('SM_Town_Feed_Fixtures', lod)
    shell_architecture(s, g, f, lod)
    storefront(s, g, f, lod)
    windows(s, g, f, lod)
    awning_and_dock(s, f, lod)
    services(s, f, lod)
    interior(i, f, lod)
    return {m.name: m for m in [s, g, i, f]}


if __name__ == '__main__':
    for lod in range(3):
        d = build(lod)
        print(lod, {k.replace('SM_Town_Feed_', ''): sum(len(fc) - 2 for fc in m.f) for k, m in d.items()})
