"""TB2 Caliche County Sheriff Substation. Pure Python geometry, no Blender dependency.

build(lod) returns the four TB1 chunks (Shell, Glass, Interior, Fixtures) with the same
footprint, wall height, front, door and window anchors as design.BUILDINGS['Sheriff'].
The TB1 interior is reused read-only and enriched; shell, glass and fixtures are new.
Blender metres, Z up, same local frame as TB1 (front wall at x=0 facing +x).
"""
import math, sys
from pathlib import Path

_TB = Path(__file__).resolve().parent.parent
if str(_TB) not in sys.path:
    sys.path.insert(0, str(_TB))

from geometry import Mesh, seg, add, sub, mul, dot, cross, unit, cabinet  # noqa: E402
from design import BUILDINGS  # noqa: E402
import town_geometry  # noqa: E402

B = BUILDINGS['Sheriff']
X0, X1, Y0, Y1 = B['footprint']
H = B['height']                      # main wall/roof line, unchanged
T = .11                              # half wall thickness (TB1 walls are .22)
HF, HR, HS = 4.05, 4.45, 3.90        # front parapet, raised centre parapet, side/rear parapet
DOOR_Y, DOOR_W, DOOR_H = B['door']
DU0, DU1 = DOOR_Y - DOOR_W / 2, DOOR_Y + DOOR_W / 2
(WIN_Y, WIN_W, WZ0, WZ1), = B['windows']
WU0, WU1 = WIN_Y - WIN_W / 2, WIN_Y + WIN_W / 2
RAISE_U = (-3.6, 1.0)                # raised parapet over the window and sign
SIDE_WIN = (-5.05, -4.15, 1.15, 2.20)  # x range and z range of the small shaded side windows
ROOF = H + .07

# Wall face frames: origin on the outer face, right vector (u), outward normal (d). v is world z.
FACES = {
    'front': ((X1 + T, 0, 0), (0, 1, 0), (1, 0, 0)),
    'rear': ((X0 - T, 0, 0), (0, -1, 0), (-1, 0, 0)),
    'left': ((0, Y0 - T, 0), (1, 0, 0), (0, -1, 0)),
    'right': ((0, Y1 + T, 0), (-1, 0, 0), (0, 1, 0)),
}


class SMesh(Mesh):
    """Mesh whose lathe never collapses a ring to a point (keeps every quad non-degenerate)."""
    def lathe(self, name, origin, profile, mat, n=24, axis=(0, 0, 1), caps=True):
        super().lathe(name, origin, [(z, max(r, .003)) for z, r in profile], mat, n, axis, caps)


def P(face, u, v, d=0.):
    o, r, n = FACES[face]
    return (o[0] + r[0] * u + n[0] * d, o[1] + r[1] * u + n[1] * d, v)


def wbox(m, face, name, u, v, d, su, sv, sd, mat):
    """Axis aligned box in wall-face coordinates (centre u,v,d and sizes)."""
    if min(su, sv, sd) <= 1e-6:
        return
    o, r, n = FACES[face]
    size = tuple(abs(r[k]) * su + abs(n[k]) * sd + (sv if k == 2 else 0) for k in range(3))
    m.box(name, P(face, u, v, d), size, mat)


def newell(ps):
    n = [0., 0., 0.]
    for i, a in enumerate(ps):
        b = ps[(i + 1) % len(ps)]
        n[0] += (a[1] - b[1]) * (a[2] + b[2]); n[1] += (a[2] - b[2]) * (a[0] + b[0]); n[2] += (a[0] - b[0]) * (a[1] + b[1])
    return tuple(n)


def face_toward(m, name, ps, direction, mat):
    m.tag = name
    m.face(ps if dot(newell(ps), direction) >= 0 else ps[::-1], mat)


def extrude(m, name, cap, vec, mat, start=True, end=True):
    """Planar cap polygon (convex or simply concave) swept along vec; outward winding."""
    m.tag = name
    cap = list(cap)
    if dot(newell(cap), vec) < 0:
        cap = cap[::-1]
    top = [add(p, vec) for p in cap]
    if start:
        m.face(cap[::-1], mat)
    if end:
        m.face(top, mat)
    k = len(cap)
    for i in range(k):
        j = (i + 1) % k
        m.face([cap[i], cap[j], top[j], top[i]], mat)


def wprism(m, face, name, poly, d0, d1, mat, back=False):
    """Polygon drawn in (u,v) on a wall face, pushed out from d0 to d1."""
    n = FACES[face][2]
    extrude(m, name, [P(face, u, v, d0) for u, v in poly], mul(n, d1 - d0), mat, start=back)


def uprism(m, face, name, section, u0, u1, mat):
    """Section drawn in (d,v), swept along the face from u0 to u1."""
    r = FACES[face][1]
    extrude(m, name, [P(face, u0, v, d) for d, v in section], mul(r, u1 - u0), mat)


def wtube(m, face, name, a, b, rad, mat, n=6, caps=True):
    m.tube(name, P(face, *a), P(face, *b), rad, mat, n, caps)


def polytube(m, name, pts, rad, mat, n=6):
    for a, b in zip(pts, pts[1:]):
        m.tube(name, a, b, rad, mat, n, caps=False)


def strip(m, face, name, pts, width, d, mat):
    """Flat crack or streak: a chain of thin quads lying on the wall face."""
    m.tag = name
    for (u0, v0), (u1, v1) in zip(pts, pts[1:]):
        du, dv = u1 - u0, v1 - v0
        ln = math.hypot(du, dv)
        nu, nv = -dv / ln * width / 2, du / ln * width / 2
        m.face([P(face, u0 - nu, v0 - nv, d), P(face, u1 - nu, v1 - nv, d), P(face, u1 + nu, v1 + nv, d), P(face, u0 + nu, v0 + nv, d)], mat)


def taper(m, face, name, u, v_top, v_bot, w_top, w_bot, d, mat):
    m.tag = name
    m.face([P(face, u - w_bot / 2, v_bot, d), P(face, u + w_bot / 2, v_bot, d), P(face, u + w_top / 2, v_top, d), P(face, u - w_top / 2, v_top, d)], mat)


def blob(cu, cv, ru, rv, k=7, seed=0.):
    """Irregular convex patch outline."""
    out = []
    for i in range(k):
        a = 2 * math.pi * i / k + seed
        f = 1 + .16 * math.sin(3 * a + seed * 2.3)
        out.append((cu + ru * f * math.cos(a), cv + rv * f * math.sin(a)))
    return out


# ------------------------------------------------------------------ walls

def wall_holes(m, face, u0, u1, h, holes, mat, d0=-.22, d1=0., z0=0., name='Stucco_wall_with_real_opening'):
    cuts = sorted(set([u0, u1] + [max(u0, min(u1, a)) for hole in holes for a in hole[:2]]))
    for a, b in zip(cuts, cuts[1:]):
        spans = sorted((hz0, hz1) for hu0, hu1, hz0, hz1 in holes if hu0 < (a + b) / 2 < hu1)
        last = z0
        for lo, hi in spans:
            wbox(m, face, name, (a + b) / 2, (last + lo) / 2, (d0 + d1) / 2, b - a, lo - last, d1 - d0, mat)
            last = max(last, hi)
        wbox(m, face, name, (a + b) / 2, (last + h) / 2, (d0 + d1) / 2, b - a, h - last, d1 - d0, mat)


def coping(m, face, u0, u1, z, lod=0, seed=0., mat='SheriffCoping'):
    """Hand-trowelled parapet cap: rounded section, top line wanders by a couple of cm.

    The cap reaches 4 cm down over the wall top, so the lift never opens a gap."""
    if lod == 0:
        sec = [(-.25, z - .04), (.03, z - .04), (.03, z + .012), (.022, z + .034), (.0, z + .052), (-.04, z + .062),
               (-.21, z + .058), (-.24, z + .045), (-.255, z + .02)]
    elif lod == 1:
        sec = [(-.25, z - .04), (.03, z - .04), (.03, z + .02), (.0, z + .052), (-.21, z + .058), (-.255, z + .02)]
    else:
        sec = [(-.25, z - .04), (.03, z - .04), (.03, z + .03), (-.11, z + .06), (-.255, z + .035)]
    steps = 1 if lod == 2 else max(2, int(round((u1 - u0) / (.7 if lod == 0 else 1.6))))
    us = [u0 + (u1 - u0) * k / steps for k in range(steps + 1)]
    lift = [0. if k in (0, steps) else .018 * (1 + math.sin(2.7 * k + seed)) * (.6 + .4 * math.sin(1.3 * k * k + seed)) for k in range(steps + 1)]
    rings = [[P(face, u, v + h, d) for d, v in sec] for u, h in zip(us, lift)]
    m.tag = 'Parapet_coping_cap'
    r = FACES[face][1]
    if dot(newell(rings[0]), r) < 0:
        rings = [ring[::-1] for ring in rings]
    m.face(rings[0][::-1], mat)
    m.face(rings[-1], mat)
    k = len(sec)
    for a, b in zip(rings, rings[1:]):
        for i in range(k):
            j = (i + 1) % k
            m.face([a[i], a[j], b[j], b[i]], mat)


def soft_corner(m, cx, cy, sx, sy, z0, z1, r, lod, mat, name='Rounded_plaster_corner', h=T):
    """Square wall corner block whose one outer corner (direction sx, sy) is rounded with radius r."""
    k = [4, 2, 1][lod]
    pts = []
    for qx, qy in ((-sx, -sy), (sx, -sy), (sx, sy), (-sx, sy)):
        if (qx, qy) == (sx, sy):
            ox, oy = cx + sx * (h - r), cy + sy * (h - r)
            # arc from the x-facing side to the y-facing side
            for i in range(k + 1):
                t = i / k * math.pi / 2
                pts.append((ox + sx * r * math.cos(t), oy + sy * r * math.sin(t)))
        else:
            pts.append((cx + qx * h, cy + qy * h))
    extrude(m, name, [(x, y, z0) for x, y in pts], (0, 0, z1 - z0), mat)


def window_unit(s, g, face, u0, u1, z0, z1, lod, mullions=(), bars=True, bar_step=.24, lintel=False, depth=-.12):
    """Recessed frame in a real opening: reveal, sill with drip, glazing, optional bars."""
    fd = .10
    for uu in (u0 + .035, u1 - .035):
        wbox(s, face, 'Window_frame_jamb', uu, (z0 + z1) / 2, depth, .07, z1 - z0, fd, 'DarkWood')
    wbox(s, face, 'Window_frame_head', (u0 + u1) / 2, z1 - .035, depth, u1 - u0 - .14, .07, fd, 'DarkWood')
    wbox(s, face, 'Window_frame_bottom_rail', (u0 + u1) / 2, z0 + .045, depth, u1 - u0 - .14, .09, fd, 'DarkWood')
    edges = [u0 + .07] + [x for mu in mullions for x in (mu - .03, mu + .03)] + [u1 - .07]
    for mu in mullions:
        wbox(s, face, 'Window_mullion', mu, (z0 + z1) / 2, depth, .06, z1 - z0 - .16, fd * .9, 'DarkWood')
    za, zb = z0 + .09, z1 - .07
    for a, b in zip(edges[::2], edges[1::2]):
        g.tag = 'Recessed_window_glazing'
        g.face([P(face, a, za, depth), P(face, b, za, depth), P(face, b, zb, depth), P(face, a, zb, depth)], 'Glass', [(0, 0), (1, 0), (1, 1), (0, 1)])
        if lod == 0:
            for uu in (a + .01, b - .01):
                wbox(s, face, 'Glazing_bead', uu, (za + zb) / 2, depth + .022, .02, zb - za, .025, 'DarkWood')
            for zz in (za + .01, zb - .01):
                wbox(s, face, 'Glazing_bead', (a + b) / 2, zz, depth + .022, b - a - .04, .02, .025, 'DarkWood')
    # Precast sill: sloped wash, 5 cm projection, drip groove underneath.
    # A convex section keeps every cap fan triangle non-degenerate; the drip groove is a separate strip.
    sec = [(depth + .05, z0 + .03), (.05, z0 + .006), (.05, z0 - .045), (depth + .05, z0 - .045)]
    uprism(s, face, 'Precast_sill_with_drip', sec, u0 - .09, u1 + .09, 'SheriffCoping')
    if lod < 2:
        wbox(s, face, 'Sill_drip_groove', (u0 + u1) / 2, z0 - .0465, .03, u1 - u0 + .16, .012, .003, 'DarkMetal')
    if lod < 2:
        taper(s, face, 'Sill_runoff_stain', (u0 + u1) / 2, z0 - .045, max(.56, z0 - .75), (u1 - u0) * .9, (u1 - u0) * .45, .0015, 'SheriffStreak')
    if lintel:
        wbox(s, face, 'Exposed_timber_lintel', (u0 + u1) / 2, z1 + .09, -.005, u1 - u0 + .40, .18, .07, 'SheriffWeatheredWood')
    if bars:
        n = max(4, seg(lod) // 2)
        fz0, fz1 = z0 + .03, z1 + .03
        for zz in (fz0, fz1):
            wbox(s, face, 'Security_bar_flat', (u0 + u1) / 2, zz, .03, u1 - u0 + .10, .04, .012, 'DarkMetal')
        for uu in (u0 - .03, u1 + .03):
            wbox(s, face, 'Security_bar_flat', uu, (fz0 + fz1) / 2, .03, .04, fz1 - fz0 + .04, .012, 'DarkMetal')
            if lod < 2:
                for zz in (fz0 + .25, fz1 - .25):
                    wbox(s, face, 'Bar_wall_lug', uu, zz, .012, .06, .05, .024, 'DarkMetal')
        cnt = max(3, int(round((u1 - u0) / (bar_step * (1 if lod < 2 else 1.6)))))
        for k in range(1, cnt):
            uu = u0 + (u1 - u0) * k / cnt
            wtube(s, face, 'Forged_security_bar', (uu, fz0, .03), (uu, fz1, .03), .011, 'DarkMetal', n, caps=False)
        for zz in (z0 + (z1 - z0) * .36, z0 + (z1 - z0) * .70):
            wbox(s, face, 'Security_bar_flat', (u0 + u1) / 2, zz, .03, u1 - u0 + .06, .035, .01, 'DarkMetal')


# ------------------------------------------------------------------ signage

def letter_rects(ch, w, t):
    m = .5
    if ch == 'S':
        return [(0, 1 - t, w, 1), (0, m + t / 2, t, 1 - t), (0, m - t / 2, w, m + t / 2), (w - t, t, w, m - t / 2), (0, 0, w, t),
                (w - t, 1 - t - .13, w, 1 - t), (0, t, t, t + .13)]
    if ch == 'H':
        return [(0, 0, t, 1), (w - t, 0, w, 1), (t, m - t / 2, w - t, m + t / 2)]
    if ch == 'E':
        return [(0, 0, t, 1), (t, 1 - t, w, 1), (t, m - t / 2, w * .86, m + t / 2), (t, 0, w, t)]
    if ch == 'R':
        return [(0, 0, t, 1), (t, 1 - t, w - t, 1), (w - t, m - t / 2, w, 1), (t, m - t / 2, w - t, m + t / 2)]
    if ch == 'I':
        return [(0, 0, t, 1)]
    if ch == 'F':
        return [(0, 0, t, 1), (t, 1 - t, w, 1), (t, m - t / 2, w * .8, m + t / 2)]
    return []


def raised_letters(s, face, text, u_left, v_base, height, depth, mat):
    w, t, gap = .62, .17, .16
    u = u_left
    for ch in text:
        lw = t if ch == 'I' else w
        for a, b, c, d in letter_rects(ch, lw, t):
            poly = [(u + a * height, v_base + b * height), (u + c * height, v_base + b * height), (u + c * height, v_base + d * height), (u + a * height, v_base + d * height)]
            wprism(s, face, 'Raised_cast_letter_' + ch, poly, 0, depth, mat)
        if ch == 'R':  # diagonal leg
            a0, tw = .30 * w, t * 1.25
            poly = [(u + a0 * height, v_base + (.5 - t / 2) * height), (u + (a0 + tw) * height, v_base + (.5 - t / 2) * height), (u + w * height, v_base), (u + (w - tw) * height, v_base)]
            wprism(s, face, 'Raised_cast_letter_R_leg', poly, 0, depth, mat)
        u += (lw + gap) * height
    return u


def letters_width(text, height):
    w, t, gap = .62, .17, .16
    return sum(((t if c == 'I' else w) + gap) * height for c in text) - gap * height


def star_badge(s, face, cu, cv, R, lod, mat='SheriffBrass'):
    n = FACES[face][2]
    pts = []
    for i in range(12):
        a = math.pi / 2 + i * math.pi / 6
        r = R if i % 2 == 0 else R * .52
        pts.append((cu + r * math.cos(a), cv + r * math.sin(a)))
    # Dark backing disc so the brass reads against pale plaster, then a faceted cast star:
    # straight sides 4 cm deep and ridged arms rising to a centre point 11 cm off the wall.
    s.lathe('Star_backing_disc', P(face, cu, cv, 0), [(0, R * 1.24), (.02, R * 1.24)], 'SheriffLetters', [28, 16, 10][lod], axis=n)
    d0, d1, apex = .02, .06, .11
    s.tag = 'Six_point_star_badge'
    for i in range(12):
        j = (i + 1) % 12
        s.face([P(face, cu, cv, apex), P(face, *pts[i], d1), P(face, *pts[j], d1)], mat)
        s.face([P(face, *pts[i], d0), P(face, *pts[j], d0), P(face, *pts[j], d1), P(face, *pts[i], d1)], mat)
    if lod < 2:
        br = R * .11
        for i in range(0, 12, 2):
            u, v = pts[i]
            a = math.pi / 2 + i * math.pi / 6
            s.lathe('Star_ball_tip', P(face, u + br * .7 * math.cos(a), v + br * .7 * math.sin(a), d0),
                    [(0, br * .3), (br * .5, br), (br * 1.3, br), (br * 1.9, br * .3)], mat, [10, 6][lod], axis=n)
    s.torus('Star_ring', P(face, cu, cv, .04), R * 1.12, .026, mat, [28, 16, 10][lod], [6, 4, 4][lod], axis=n)


def gooseneck(f, face, u, v, lod):
    pts = [(0., v), (.12, v + .10), (.30, v + .14), (.44, v + .08), (.50, v - .02)]
    polytube(f, 'Gooseneck_arm', [P(face, u, vv, dd) for dd, vv in (pts if lod < 2 else pts[::2])], .014, 'DarkMetal', [6, 4, 4][lod])
    wbox(f, face, 'Gooseneck_wall_plate', u, v, .01, .12, .12, .02, 'DarkMetal')
    tip = P(face, u, v - .02, .50)
    axis = unit(add(mul(FACES[face][2], -.55), (0, 0, -1)))
    f.lathe('RLM_shade', tip, [(0, .03), (.04, .05), (.10, .13), (.11, .135)][: 4 if lod == 0 else 3], 'DarkMetal', [16, 10, 6][lod], axis=axis, caps=False)
    f.lathe('RLM_lamp', add(tip, mul(axis, .04)), [(0, .03), (.04, .045), (.09, .04), (.12, .0)], 'SheriffLampGlow', [10, 6, 4][lod], axis=axis, caps=False)


# ------------------------------------------------------------------ door, canopy, entry

def entry(s, g, f, lod):
    fc = 'front'
    n = seg(lod)
    # Concrete stoop (12 cm) and the step continued into the door opening.
    s.rounded_box('Concrete_stoop', P(fc, DOOR_Y, .06, .42), (.84, 1.72, .12), .05, 'Concrete', 2 if lod < 2 else 1)
    wbox(s, fc, 'Door_concrete_step', DOOR_Y, .06, -.11, DOOR_W, .12, .22, 'Concrete')
    wbox(f, fc, 'Aluminium_threshold', DOOR_Y, .127, -.09, DOOR_W - .02, .014, .14, 'Metal')
    # Frame inside the opening.
    for uu in (DU0 + .03, DU1 - .03):
        wbox(s, fc, 'Door_jamb', uu, (DOOR_H + .12) / 2, -.13, .06, DOOR_H - .12, .16, 'DarkWood')
    wbox(s, fc, 'Door_head_jamb', DOOR_Y, DOOR_H - .03, -.13, DOOR_W - .12, .06, .16, 'DarkWood')
    if lod < 2:
        for uu in (DU0 - .03, DU1 + .03):
            wbox(s, fc, 'Door_casing', uu, (DOOR_H + .12) / 2 + .02, .008, .06, DOOR_H - .08, .016, 'DarkWood')
        wbox(s, fc, 'Door_casing_head', DOOR_Y, DOOR_H + .03, .008, DOOR_W + .12, .06, .016, 'DarkWood')
    # Leaf: stiles, rails, glazed 2x3 upper, recessed lower panel with X brace.
    a, b, zb, zt = DU0 + .063, DU1 - .063, .135, DOOR_H - .063
    dc, dt = -.1125, .045
    st = .11
    for uu in (a + st / 2, b - st / 2):
        wbox(s, fc, 'Door_stile', uu, (zb + zt) / 2, dc, st, zt - zb, dt, 'DarkWood')
    ia, ib = a + st, b - st
    for z0, z1 in ((zb, .38), (1.0, 1.13), (1.95, zt)):
        wbox(s, fc, 'Door_rail', (ia + ib) / 2, (z0 + z1) / 2, dc, ib - ia, z1 - z0, dt, 'DarkWood')
    wbox(s, fc, 'Door_lower_panel', (ia + ib) / 2, .69, dc - .01, ib - ia, .62, .02, 'DarkWood')
    if lod < 2:
        bw = .075
        for (u0, v0), (u1, v1) in (((ia, .38), (ib, 1.0)), ((ia, 1.0), (ib, .38))):
            ln = math.hypot(u1 - u0, v1 - v0); nu, nv = -(v1 - v0) / ln * bw / 2, (u1 - u0) / ln * bw / 2
            k = .9
            poly = [(u0 - nu * k, v0 - nv * k), (u1 - nu * k, v1 - nv * k), (u1 + nu * k, v1 + nv * k), (u0 + nu * k, v0 + nv * k)]
            poly = [(min(ib, max(ia, uu)), min(1.0, max(.38, vv))) for uu, vv in poly]
            wprism(s, fc, 'Door_X_brace_board', poly, dc - .002, dc + dt / 2 + .008, 'Timber')
    cu = (ia + ib) / 2
    wbox(s, fc, 'Door_muntin', cu, 1.54, dc, .03, .82, .03, 'DarkWood')
    rows = [1.13, 1.13 + .82 / 3, 1.13 + 1.64 / 3, 1.95]
    for zz in rows[1:-1]:
        wbox(s, fc, 'Door_muntin', cu, zz, dc, ib - ia, .03, .03, 'DarkWood')
    for z0, z1 in zip(rows, rows[1:]):
        for u0, u1 in ((ia, cu - .015), (cu + .015, ib)):
            za = z0 + (.015 if z0 > 1.14 else 0); zb2 = z1 - (.015 if z1 < 1.94 else 0)
            g.tag = 'Door_glazing'
            g.face([P(fc, u0, za, dc), P(fc, u1, za, dc), P(fc, u1, zb2, dc), P(fc, u0, zb2, dc)], 'Glass', [(0, 0), (1, 0), (1, 1), (0, 1)])
    # CLOSED card hanging behind the glass on a cord.
    s.panel('Door_closed_card', P(fc, cu + .17, 1.27, dc - .03), .2, .2, 15, (1, 0, 0))
    if lod < 2:
        for du in (.08, -.08):
            wtube(s, fc, 'Closed_card_cord', (cu + .17 + du, 1.37, dc - .031), (cu + .17, 1.50, dc - .031), .002, 'Rubber', 3, caps=False)
    # Hardware (fixtures): kick plate, hinges, knob, deadbolt, closer.
    fr = dc + dt / 2
    wbox(f, fc, 'Brass_kick_plate', (a + b) / 2, .265, fr + .0015, b - a - .04, .23, .003, 'SheriffBrass')
    for zz in (.38, 1.1, 1.86) if lod < 2 else ():
        wtube(f, fc, 'Butt_hinge_knuckle', (a - .004, zz - .055, fr + .006), (a - .004, zz + .055, fr + .006), .011, 'SheriffBrass', max(6, n // 2))
        if lod == 0:
            wbox(f, fc, 'Hinge_leaf', a + .02, zz, fr + .002, .035, .1, .004, 'SheriffBrass')
    ku = b - .06
    wbox(f, fc, 'Lock_escutcheon', ku, 1.02, fr + .003, .055, .2, .006, 'SheriffBrass')
    f.lathe('Door_knob', P(fc, ku, .98, fr + .006), [(0, .02), (.035, .014), (.045, .03), (.07, .03), (.075, .0)][: 5 if lod < 2 else 3], 'SheriffBrass', [10, 6, 5][lod], axis=(1, 0, 0), caps=lod == 2)
    if lod < 2:
        f.lathe('Deadbolt_cylinder', P(fc, ku, 1.08, fr + .006), [(0, .02), (.016, .018), (.018, .0)], 'SheriffBrass', max(6, n // 2), axis=(1, 0, 0), caps=False)
    if lod < 2:
        wbox(f, fc, 'Door_closer_body', a + .2, 1.995, fr + .03, .28, .055, .06, 'DarkMetal')
        polytube(f, 'Door_closer_arm', [P(fc, a + .32, 2.0, fr + .062), P(fc, a + .46, 2.07, fr + .085), P(fc, a + .52, DOOR_H + .02, .02)], .008, 'DarkMetal', 4)
        wbox(f, fc, 'Door_closer_shoe', a + .52, DOOR_H + .03, .01, .07, .04, .02, 'DarkMetal')


def canopy(s, f, lod):
    fc = 'front'
    ua, ub = DOOR_Y - .9, DOOR_Y + .9
    D = 1.16
    top_w, top_f = 2.86, 2.66            # tin roof top at wall and at the front
    wbox(s, fc, 'Canopy_ledger', DOOR_Y, 2.72, .04, ub - ua - .06, .16, .08, 'DarkWood')

    def roof_z(d):
        return top_w + (top_f - top_w) * d / D
    # Rafters, kept clear of the centre lamp.
    for uu in ([ua + .06, DOOR_Y - .42, DOOR_Y + .42, ub - .06] if lod < 2 else [ua + .06, ub - .06]):
        sec = [(.08, roof_z(.08) - .04 - .14), (D - .04, roof_z(D - .04) - .04 - .14), (D - .04, roof_z(D - .04) - .04), (.08, roof_z(.08) - .04)]
        uprism(s, fc, 'Canopy_rafter', sec, uu - .025, uu + .025, 'DarkWood')
    uprism(s, fc, 'Canopy_plank_deck', [(0, roof_z(0) - .04), (D, roof_z(D) - .04), (D, roof_z(D) - .02), (0, roof_z(0) - .02)], ua, ub, 'Timber')
    uprism(s, fc, 'Canopy_tin_roof', [(0, roof_z(0) - .02), (D + .04, roof_z(D + .04) - .02), (D + .04, roof_z(D + .04)), (0, roof_z(0))], ua - .04, ub + .04, 'RustTin')
    if lod == 0:
        k = 14
        for i in range(k + 1):
            uu = ua - .02 + (ub - ua + .04) * i / k
            wtube(s, fc, 'Tin_corrugation_rib', (uu, roof_z(.02), .02), (uu, roof_z(D + .02), D + .02), .012, 'RustTin', 4, caps=False)
    # Fascia front and sides (bottom edge stays above 2.3 m).
    wbox(s, fc, 'Canopy_fascia', DOOR_Y, top_f - .09, D - .02, ub - ua + .02, .17, .04, 'SheriffWeatheredWood')
    for uu in (ua, ub):
        uprism(s, fc, 'Canopy_side_fascia', [(.0, roof_z(0) - .20), (D, roof_z(D) - .18), (D, roof_z(D) - .02), (0, roof_z(0) - .02)], uu - .02, uu + .02, 'SheriffWeatheredWood')
    wbox(s, fc, 'Canopy_counterflashing', DOOR_Y, top_w + .05, .008, ub - ua + .12, .1, .016, 'Metal')
    # Tie rods to the wall above.
    for uu in (ua + .08, ub - .08):
        wbox(s, fc, 'Tie_rod_wall_plate', uu, 3.32, .01, .09, .14, .02, 'DarkMetal')
        wtube(s, fc, 'Canopy_tie_rod', (uu, 3.30, .02), (uu, top_f - .02, D - .06), .009, 'DarkMetal', 5)
        if lod < 2:
            a0, a1 = P(fc, uu, 3.30, .02), P(fc, uu, top_f - .02, D - .06)
            ax = unit(sub(a1, a0))
            s.lathe('Tie_rod_turnbuckle', add(mul(add(a0, a1), .5), mul(ax, -.06)), [(0, .012), (.03, .017), (.09, .017), (.12, .012)], 'DarkMetal', 6, axis=ax)
    # Soffit light over the door.
    under = roof_z(.55) - .04
    f.lathe('Porch_light_canopy', P(fc, DOOR_Y, under - .03, .55), [(0, .09), (.03, .09)], 'DarkMetal', [16, 8, 6][lod], caps=True)
    f.lathe('Porch_light_globe', P(fc, DOOR_Y, under - .175, .55), [(0, .001), (.03, .055), (.09, .075), (.145, .06)][(0 if lod < 2 else 1):], 'SheriffLampGlow', [16, 8, 6][lod], caps=lod == 2)


def bulletin_case(s, g, face, u, v, lod):
    w, h = .92, .72
    wbox(s, face, 'Notice_case_back', u, v, .015, w, h, .03, 'SheriffCork')
    for vv in (v - h / 2 + .025, v + h / 2 - .025):
        wbox(s, face, 'Notice_case_frame', u, vv, .055, w, .05, .08, 'DarkWood')
    for uu in (u - w / 2 + .025, u + w / 2 - .025):
        wbox(s, face, 'Notice_case_frame', uu, v, .055, .05, h - .1, .08, 'DarkWood')
    g.tag = 'Notice_case_glazing'
    g.face([P(face, u - w / 2 + .05, v - h / 2 + .05, .085), P(face, u + w / 2 - .05, v - h / 2 + .05, .085),
            P(face, u + w / 2 - .05, v + h / 2 - .05, .085), P(face, u - w / 2 + .05, v + h / 2 - .05, .085)], 'Glass', [(0, 0), (1, 0), (1, 1), (0, 1)])
    sheets = [(-.25, .1, .21, .28, 0), (.0, .13, .2, .26, .05), (.24, .08, .19, .25, -.04), (-.12, -.18, .24, .17, .03), (.17, -.19, .22, .17, 0)]
    for k, (du, dv, sw, sh, rot) in enumerate(sheets if lod < 2 else sheets[:3]):
        c, sn = math.cos(rot), math.sin(rot)
        poly = [(u + du + x * c - y * sn, v + dv + x * sn + y * c) for x, y in ((-sw / 2, -sh / 2), (sw / 2, -sh / 2), (sw / 2, sh / 2), (-sw / 2, sh / 2))]
        s.tag = 'Posted_notice'
        s.face([P(face, uu, vv, .032 + .001 * k) for uu, vv in poly], 'Paper')
    if lod == 0:
        s.panel('Notice_county_road_map', P(face, u - .28, v - .19, .036), .2, .2, 10, FACES[face][2])


def bench(s, face, u, lod):
    L = 1.9
    bn = [6, 5, 4][lod]
    for uu in (u - L / 2 + .15, u + L / 2 - .15):
        # Cast end frame: front and rear legs, seat bearer, back support.
        wtube(s, face, 'Bench_front_leg', (uu, 0, .52), (uu, .43, .52), .02, 'DarkMetal', bn)
        wtube(s, face, 'Bench_rear_leg', (uu, 0, .20), (uu, .43, .24), .02, 'DarkMetal', bn)
        wtube(s, face, 'Bench_seat_bearer', (uu, .42, .18), (uu, .42, .56), .018, 'DarkMetal', bn)
        wtube(s, face, 'Bench_back_support', (uu, .42, .22), (uu, .88, .17), .018, 'DarkMetal', bn)
    for k, dd in enumerate((.25, .36, .47)):
        wbox(s, face, 'Bench_seat_slat', u, .455, dd, L, .035, .09, 'SheriffWeatheredWood')
    for vv, dd in ((.62, .205), (.78, .19)):
        wbox(s, face, 'Bench_back_slat', u, vv, dd, L, .09, .03, 'SheriffWeatheredWood')


def trash_can(s, face, u, lod):
    n = seg(lod)
    c = P(face, u, 0, .32)
    s.lathe('Galvanized_trash_can', c, [(0, .2), (.02, .215), (.66, .235), (.68, .24)], 'Metal', n, caps=False)
    s.lathe('Trash_can_base', c, [(0, .2), (.001, .0)], 'Metal', n, caps=False)
    for zz in ((.08, .36, .62) if lod == 0 else (.36,) if lod == 1 else ()):
        s.torus('Trash_can_rib', add(c, (0, 0, zz)), .222 + zz * .025, .008, 'Metal', n, 4)
    s.lathe('Trash_can_lid', add(c, (0, 0, .68)), [(0, .255), (.03, .25), (.07, .12), (.075, .0)], 'Metal', n, caps=False)
    if lod < 2:
        s.torus('Trash_can_lid_handle', add(c, (0, 0, .78)), .04, .008, 'Metal', 8, 4, axis=(0, 1, 0))


# ------------------------------------------------------------------ side wall services

def service_entrance(s, g, face, u, lod):
    n = max(6, seg(lod) // 2)
    wbox(s, face, 'Meter_base', u, 1.55, .055, .26, .38, .11, 'Metal')
    s.lathe('Meter_ring', P(face, u, 1.58, .11), [(0, .095), (.02, .095), (.02, .085)], 'Metal', seg(lod), axis=FACES[face][2], caps=False)
    s.lathe('Meter_face', P(face, u, 1.58, .115), [(0, .082), (.001, .0)], 'Ceramic', seg(lod), axis=FACES[face][2], caps=False)
    if lod < 2:
        g.lathe('Meter_glass_dome', P(face, u, 1.58, .13), [(0, .085), (.06, .075), (.1, .04), (.11, .0)], 'Glass', [12, 6][lod], axis=FACES[face][2], caps=False)
    wbox(s, face, 'Main_disconnect', u, 1.02, .05, .24, .32, .10, 'Metal')
    if lod < 2:
        wbox(s, face, 'Disconnect_handle', u + .13, 1.05, .07, .02, .1, .03, 'DarkMetal')
    wtube(s, face, 'Meter_nipple_conduit', (u, 1.36, .05), (u, 1.18, .05), .02, 'Metal', n)
    wtube(s, face, 'Service_ground_conduit', (u, .86, .05), (u, .0, .05), .02, 'Metal', n)
    wtube(s, face, 'Service_mast', (u, 1.74, .07), (u, 4.62, .07), .026, 'Metal', n)
    for zz in (2.3, 3.2, 3.85, 4.35) if lod < 2 else (2.6, 3.85):
        wbox(s, face, 'Mast_strap', u, zz, .045, .1, .03, .05, 'Metal')
    s.lathe('Weatherhead', P(face, u, 4.62, .07), [(0, .03), (.05, .055), (.12, .05), (.15, .0)], 'DarkMetal', n, caps=False)
    if lod < 2:
        # Insulator bracket and drip loops, the service drop itself is left to the town poles.
        wtube(s, face, 'Insulator_bracket', (u, 4.38, .07), (u, 4.38, .30), .012, 'Metal', 5)
        s.lathe('Spool_insulator', P(face, u, 4.34, .30), [(0, .03), (.02, .04), (.06, .04), (.08, .03)], 'Ceramic', 8)
        for k, du in enumerate((-.03, 0, .03)):
            pts = [P(face, u + du, 4.62, .11), P(face, u + du * 1.5, 4.45, .18), P(face, u + du * 1.5, 4.30, .23), P(face, u + du * .6, 4.36, .29)]
            polytube(s, 'Service_drip_loop', pts, .007, 'Rubber', 4)
        wtube(s, face, 'Ground_wire', (u + .1, 1.38, .02), (u + .12, .02, .02), .003, 'SheriffBrass', 3, caps=False)
        s.lathe('Ground_rod_clamp', P(face, u + .12, .0, .02), [(0, .01), (.04, .01)], 'SheriffBrass', 6)


def gas_meter(s, face, u, lod):
    n = max(6, seg(lod) // 2)
    wtube(s, face, 'Gas_riser', (u - .14, 0, .14), (u - .14, .62, .14), .022, 'Metal', n)
    wtube(s, face, 'Gas_riser_elbow', (u - .14, .62, .14), (u - .1, .62, .14), .022, 'Metal', n)
    s.rounded_box('Gas_meter_body', P(face, u + .04, .78, .16), (.30, .22, .40) if FACES[face][1][0] else (.22, .30, .40), .04, 'Metal', 2)
    if lod < 2:
        s.lathe('Gas_regulator', P(face, u - .14, .5, .14), [(0, .0), (.02, .06), (.07, .07), (.09, .0)], 'DarkMetal', n, axis=FACES[face][1])
    wtube(s, face, 'Gas_outlet', (u + .04, .98, .16), (u + .04, 1.12, .16), .02, 'Metal', n)
    wtube(s, face, 'Gas_house_line', (u + .04, 1.12, .16), (u + .04, 1.12, -.02), .02, 'Metal', n)
    if lod < 2:
        s.lathe('Gas_shutoff', P(face, u - .14, .3, .14), [(0, .03), (.06, .03)], 'SheriffBrass', 6)
        wbox(s, face, 'Gas_meter_dial_window', u + .04, .84, .275, .12, .08, .01, 'Ceramic')


def hose_bib(s, face, u, lod):
    n = max(6, seg(lod) // 2)
    wtube(s, face, 'Hose_bib_pipe', (u, .45, 0), (u, .45, .08), .014, 'Metal', n)
    s.lathe('Hose_bib_body', P(face, u, .45, .08), [(0, .022), (.05, .022)], 'SheriffBrass', n, axis=FACES[face][2])
    wtube(s, face, 'Hose_bib_spout', (u, .45, .11), (u, .39, .13), .011, 'SheriffBrass', 6)
    if lod < 2:
        s.torus('Hose_bib_handle', P(face, u, .5, .11), .028, .006, 'SheriffBrass', 8, 4)
        taper(s, face, 'Hose_bib_drip_stain', u, .40, .0, .06, .12, .002, 'SheriffStreak')


def canale(s, face, u, lod, splash=True):
    z = ROOF + .03
    wbox(s, face, 'Canale_trough', u, z, .155, .22, .04, .80, 'SheriffWeatheredWood')
    for uu in (u - .095, u + .095):
        wbox(s, face, 'Canale_side_board', uu, z + .08, .155, .03, .14, .80, 'SheriffWeatheredWood')
    taper(s, face, 'Canale_water_stain', u, z - .05, .7, .34, .12, .002, 'SheriffStreak')
    if splash and lod < 2:
        wbox(s, face, 'Canale_splash_block', u, .025, .42, .3, .05, .32, 'Concrete')


def patches(s, face, spots, lod):
    for k, (cu, cv, ru, rv) in enumerate(spots if lod == 0 else spots[:max(1, len(spots) // 2)]):
        poly = [(max(-1e9, a), max(.56, b)) for a, b in blob(cu, cv, ru, rv, 7 if lod == 0 else 5, k * 1.7)]
        wprism(s, face, 'Stucco_repair_patch', poly, 0, .007, 'SheriffStuccoPatch')


def base_course(s, face, u0, u1, gaps=()):
    cuts = [u0] + [x for gp in gaps for x in gp] + [u1]
    for a, b in zip(cuts[::2], cuts[1::2]):
        uprism(s, face, 'Base_course_wainscot', [(0, 0), (.025, 0), (.025, .50), (.012, .53), (0, .53)], a, b, 'SheriffStuccoBase')


# ------------------------------------------------------------------ roof

def roof(s, f, lod):
    n = seg(lod)
    s.box('Built_up_roof_deck', ((X0 + X1) / 2, (Y0 + Y1) / 2, (H - .03 + ROOF) / 2), (X1 - X0 - 2 * T, Y1 - Y0 - 2 * T, ROOF - H + .03), 'SheriffRoofing')
    xi0, xi1, yi0, yi1 = X0 + T, X1 - T, Y0 + T, Y1 - T
    c = .14
    for quad in ([(xi1, yi0, ROOF + c), (xi1, yi1, ROOF + c), (xi1 - c, yi1 - c, ROOF + .002), (xi1 - c, yi0 + c, ROOF + .002)],
                 [(xi0, yi1, ROOF + c), (xi0, yi0, ROOF + c), (xi0 + c, yi0 + c, ROOF + .002), (xi0 + c, yi1 - c, ROOF + .002)],
                 [(xi1, yi0, ROOF + c), (xi0, yi0, ROOF + c), (xi0 + c, yi0 + c, ROOF + .002), (xi1 - c, yi0 + c, ROOF + .002)],
                 [(xi0, yi1, ROOF + c), (xi1, yi1, ROOF + c), (xi1 - c, yi1 - c, ROOF + .002), (xi0 + c, yi1 - c, ROOF + .002)]):
        face_toward(s, 'Roofing_cant_strip', quad, (0, 0, 1), 'SheriffRoofing')
    if lod < 2:
        for x0, x1, y0, y1 in ((xi0 + .4, xi0 + 2.2, yi1 - 3.0, yi1 - 1.2), (-4.2, -2.6, -3.6, -1.9), (-7.1, -6.2, 1.2, 2.5)):
            s.box('Roof_repair_patch', ((x0 + x1) / 2, (y0 + y1) / 2, ROOF + .004), (x1 - x0, y1 - y0, .008), 'RoofShingle')
    # Evaporative (swamp) cooler on a timber stand over its roof duct.
    cx, cy = -5.6, 3.1
    for dy in (-.45, .45):
        s.box('Cooler_sleeper', (cx, cy + dy, ROOF + .05), (1.3, .1, .1), 'SheriffWeatheredWood')
    for dx in (-.42, .42):
        for dy in (-.42, .42):
            s.box('Cooler_stand_leg', (cx + dx, cy + dy, ROOF + .25), (.05, .05, .32), 'DarkMetal')
    z0, z1 = ROOF + .40, ROOF + 1.30
    s.box('Swamp_cooler_cabinet', (cx, cy, (z0 + z1) / 2), (.98, .98, z1 - z0), 'RustTin')
    apex = (cx, cy, z1 + .12)
    s.tag = 'Swamp_cooler_lid'
    cr = [(cx - .52, cy - .52, z1 + .01), (cx + .52, cy - .52, z1 + .01), (cx + .52, cy + .52, z1 + .01), (cx - .52, cy + .52, z1 + .01)]
    for i in range(4):
        s.face([cr[i], cr[(i + 1) % 4], apex], 'RustTin')
    s.face(cr[::-1], 'RustTin')
    nl = [10, 5, 0][lod]
    for k in range(nl):
        zz = z0 + .12 + k * (z1 - z0 - .22) / max(1, nl - 1)
        for sx, sy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if sx:
                s.box('Cooler_louvre', (cx + sx * .5, cy, zz), (.025, .78, .022), 'RustTin')
            else:
                s.box('Cooler_louvre', (cx, cy + sy * .5, zz), (.78, .025, .022), 'RustTin')
    s.box('Cooler_duct', (cx, cy, ROOF + .2), (.55, .55, .40), 'Metal')
    s.box('Duct_roof_curb', (cx, cy, ROOF + .06), (.75, .75, .12), 'SheriffCoping')
    if lod < 2:
        s.box('Canvas_duct_boot', (cx, cy, ROOF + .37), (.6, .6, .07), 'Burlap')
        pts = [(cx + .49, cy + .3, z0 + .1), (cx + .7, cy + .3, z0 + .1), (cx + .7, cy + .3, ROOF + .03), (cx + .7, Y1 - T - .4, ROOF + .03), (-1.0, Y1 - T - .4, ROOF + .03)]
        polytube(s, 'Cooler_copper_water_line', pts, .007, 'SheriffBrass', 4)
        s.box('Cooler_pump_motor', (cx - .6, cy - .2, z0 + .25), (.22, .22, .2), 'DarkMetal')
    # Plumbing vents, roof hatch.
    for x, y in ((-8.3, 4.6), (-6.4, -3.3)) if lod < 2 else ():
        if lod < 2:
            s.lathe('Roof_vent_flashing', (x, y, ROOF), [(0, .2), (.03, .12), (.05, .06)], 'Metal', n, caps=False)
        s.lathe('Plumbing_vent_stack', (x, y, ROOF), [(0, .05), (.48, .05), (.49, .045), (.49, .0)], 'DarkMetal', max(6, n // 2), caps=False)
    s.box('Roof_hatch_curb', (-8.1, .6, ROOF + .15), (.86, .86, .3), 'SheriffWeatheredWood')
    s.box('Roof_hatch_lid', (-8.1, .6, ROOF + .325), (.96, .96, .05), 'Metal')
    if lod < 2:
        s.tube('Roof_hatch_handle', (-7.62, .45, ROOF + .29), (-7.62, .75, ROOF + .29), .01, 'Metal', 5)
    radio_tower(s, f, lod)


def radio_tower(s, f, lod):
    """Guyed triangular lattice tower for the county two way radio, base on the rear roof."""
    bx, by = -9.15, -5.05
    z0, z1 = ROOF + .05, 11.6
    side = .34
    legs = [(bx + side / math.sqrt(3) * math.cos(a), by + side / math.sqrt(3) * math.sin(a)) for a in (math.radians(90), math.radians(210), math.radians(330))]
    s.box('Tower_base_plate', (bx, by, ROOF + .025), (.6, .6, .05), 'Metal')
    ln = max(4, seg(lod) // 3)
    for x, y in legs:
        s.tube('Tower_leg', (x, y, z0), (x, y, z1), .016, 'Metal', ln)
    panels = [18, 10, 4][lod]
    for k in range(panels):
        za = z0 + (z1 - z0) * k / panels
        zb = z0 + (z1 - z0) * (k + 1) / panels
        for i in range(3):
            a, b = legs[i], legs[(i + 1) % 3]
            if k % 2:
                a, b = b, a
            s.tube('Tower_zigzag_brace', (a[0], a[1], za), (b[0], b[1], zb), .006, 'Metal', 3, caps=False)
            if lod == 0:
                s.tube('Tower_girt', (a[0], a[1], zb), (b[0], b[1], zb), .005, 'Metal', 3, caps=False)
    s.tube('Antenna_mast_pipe', (bx, by, z1 - .4), (bx, by, z1 + 1.2), .025, 'Metal', ln)
    s.tube('Whip_antenna', (bx, by, z1 + 1.2), (bx, by, z1 + 3.4), .007, 'Metal', 4)
    if lod < 2:
        for i in range(4):
            a = math.pi / 4 + i * math.pi / 2
            s.tube('Ground_plane_radial', (bx, by, z1 + 1.1), (bx + .62 * math.cos(a), by + .62 * math.sin(a), z1 + .7), .005, 'Metal', 3, caps=False)
        # Side mounted folded dipole on a standoff.
        s.tube('Dipole_standoff', (bx, by, z1 - 1.2), (bx + .8, by, z1 - 1.2), .012, 'Metal', 4)
        s.tube('Folded_dipole', (bx + .8, by, z1 - 1.95), (bx + .8, by, z1 - .45), .009, 'Metal', 4)
        s.tube('Folded_dipole', (bx + .86, by, z1 - 1.9), (bx + .86, by, z1 - .5), .009, 'Metal', 4)
    gz = z0 + (z1 - z0) * .66
    for gx, gy in ((-5.0, -5.6), (-9.6, -1.0), (-6.4, -2.3)):
        s.tube('Tower_guy_wire', (bx, by, gz), (gx, gy, ROOF + .08), .004, 'Metal', 3, caps=False)
        s.box('Guy_anchor', (gx, gy, ROOF + .04), (.14, .14, .08), 'Metal')
    if lod < 2:
        lg = legs[1]
        polytube(s, 'Coax_feedline', [(lg[0] - .03, lg[1], z1 - 1.2), (lg[0] - .03, lg[1], ROOF + .12), (-8.4, -4.4, ROOF + .12), (-8.4, -4.4, ROOF + .02)], .011, 'Rubber', 4)
        s.lathe('Coax_entry_gooseneck', (-8.4, -4.4, ROOF), [(0, .06), (.25, .05), (.3, .07), (.32, .0)], 'DarkMetal', 8, caps=False)
    # Obstruction beacon (fixture).
    f.lathe('Beacon_base', (bx, by, z1), [(0, .06), (.04, .06)], 'DarkMetal', [8, 6, 5][lod])
    f.lathe('Red_obstruction_beacon', (bx + .0, by, z1 + .04), [(0, .055), (.08, .05), (.12, .0)], 'CinemaRedGlow', [10, 6, 5][lod], caps=False)


# ------------------------------------------------------------------ interior enrichment

def interior_extra(i, f, lod):
    n = seg(lod)
    # Painted plaster lining: olive wainscot under a chair rail, warm plaster above.
    for face, u0, u1, holes in (('left', X0 + T, X1 - T, [(SIDE_WIN[0], SIDE_WIN[1], SIDE_WIN[2], SIDE_WIN[3])]),
                                ('right', X1 + T, -(X0 + T), [(-SIDE_WIN[1], -SIDE_WIN[0], SIDE_WIN[2], SIDE_WIN[3])]),
                                ('front', Y0 + T, Y1 - T, [(DU0, DU1, 0, DOOR_H), (WU0, WU1, WZ0, WZ1)])):
        lo = [(a, b, za, zb) for a, b, za, zb in holes if za < 1.0]
        wall_holes(i, face, u0, u1, 1.0, lo, 'SheriffWainscot', -.235, -.22, 0., 'Painted_wainscot')
        wall_holes(i, face, u0, u1, H - .14, holes, 'Ceiling', -.232, -.22, 1.0, 'Painted_plaster_lining')
        if lod < 2:
            cuts = [u0] + [x for a, b, za, zb in holes if za < 1.06 for x in (a, b)] + [u1]
            for a, b in zip(cuts[::2], cuts[1::2]):
                wbox(i, face, 'Chair_rail', (a + b) / 2, 1.0, -.245, b - a, .05, .03, 'DarkWood')
    # Map board legs so the county map stands on the floor.
    for y in (-2.76, .16):
        i.box('Map_board_leg', (-6.36, y, .45), (.06, .06, .9), 'DarkWood')
        i.box('Map_board_foot', (-6.36, y, .02), (.42, .07, .04), 'DarkWood')
    # Low radio credenza under the front window, radio facing the room.
    i.box('Radio_credenza', (-.42, -1.3, .36), (.44, 2.4, .72), 'OliveEnamel')
    i.box('Credenza_top', (-.42, -1.3, .735), (.48, 2.46, .03), 'DarkWood')
    for y in (-2.1, -.5):
        i.box('Credenza_drawer', (-.648, y, .5), (.02, .7, .26), 'OliveEnamel')
        i.box('Credenza_drawer', (-.648, y, .2), (.02, .7, .26), 'OliveEnamel')
        if lod < 2:
            i.box('Credenza_pull', (-.665, y, .52), (.02, .16, .02), 'Metal')
    i.rounded_box('Base_station_radio', (-.4, -1.6, .89), (.32, .5, .28), .02, 'DarkMetal', 2)
    i.box('Radio_front_panel', (-.565, -1.6, .89), (.012, .46, .24), 'Metal')
    if lod < 2:
        for k in range(4):
            i.lathe('Radio_knob', (-.571, -1.78 + k * .1, .84), [(0, .018), (.02, .016)], 'Rubber', 8, axis=(-1, 0, 0))
        i.box('Radio_dial_glow', (-.572, -1.55, .95), (.004, .2, .05), 'WarmGlow')
        i.lathe('Desk_microphone_base', (-.5, -1.15, .75), [(0, .06), (.02, .05), (.03, .0)], 'DarkMetal', 10, caps=False)
        i.tube('Microphone_stem', (-.5, -1.15, .77), (-.52, -1.15, .95), .008, 'Metal', 5)
        i.lathe('Microphone_head', (-.52, -1.15, .93), [(0, .02), (.03, .03), (.07, .02)], 'DarkMetal', 8)
        i.box('Log_book', (-.4, -.7, .76), (.24, .32, .03), 'BookCloth')
        i.lathe('Coffee_mug', (-.36, -2.2, .75), [(0, .04), (.1, .042), (.1, .036), (.008, .036)], 'Ceramic', 10)
    # Desk clutter on the TB1 desk.
    if lod < 2:
        i.lathe('Glass_ashtray', (-3.0, -.85, .79), [(0, .06), (.025, .065), (.025, .045), (.012, .04)], 'WasherGlass', 10)
        i.lathe('Pencil_cup', (-3.9, -.88, .79), [(0, .035), (.11, .035)], 'DarkMetal', 8)
        for k in range(4):
            i.tube('Pencil', (-3.9 + .01 * k, -.88, .8), (-3.9 + .025 * k - .02, -.88 + .01 * k, .97), .004, 'Timber', 3)
        i.box('Desk_blotter', (-3.45, -1.25, .795), (.9, .55, .006), 'BookCloth')
    if lod == 2:
        i.lathe('Water_cooler_bottle', (-6.65, 1.65, 1.0), [(0, .14), (.4, .14)], 'WasherGlass', 6)
        i.box('Water_cooler_cabinet', (-6.65, 1.65, .5), (.34, .34, 1.0), 'CreamEnamel')
        i.box('Coat_rack_pole', (-.75, 5.35, .9), (.05, .05, 1.8), 'DarkWood')
        i.rounded_box('Hanging_jacket', (-.75 - .12, 5.35 - .05, 1.35), (.14, .46, .7), .05, 'BookCloth', 1)
    else:
        room_props(i, lod, n)
    # Drawn roller shades at the side windows glow faintly at night.
    for face, u0, u1 in (('left', SIDE_WIN[0], SIDE_WIN[1]), ('right', -SIDE_WIN[1], -SIDE_WIN[0])):
        wbox(i, face, 'Drawn_roller_shade', (u0 + u1) / 2, (SIDE_WIN[2] + SIDE_WIN[3]) / 2 + .1, -.215, u1 - u0 - .1, SIDE_WIN[3] - SIDE_WIN[2] - .2, .006, 'SheriffShadeGlow')
        wbox(i, face, 'Shade_roller', (u0 + u1) / 2, SIDE_WIN[3] - .06, -.23, u1 - u0, .05, .05, 'Metal')
    interior_lights(f, lod, n)


def room_props(i, lod, n):
    # Partition: wall clock and clipboards.
    px = -7.1 + .07
    i.lathe('Wall_clock_body', (px, 1.15, 2.45), [(0, .17), (.045, .17), (.05, .0)], 'CreamEnamel', n, axis=(1, 0, 0), caps=False)
    i.torus('Wall_clock_bezel', (px + .05, 1.15, 2.45), .17, .014, 'DarkMetal', n, 4, axis=(1, 0, 0))
    i.box('Clock_hour_hand', (px + .056, 1.15 + .03, 2.45 + .04), (.004, .02, .1), 'DarkMetal')
    i.box('Clock_minute_hand', (px + .058, 1.15 - .06, 2.45 - .01), (.004, .14, .016), 'DarkMetal')
    for k, (y, z) in enumerate(((.55, 1.75), (.85, 1.72), (1.15, 1.76), (1.5, 1.7))[: 4 if lod < 2 else 2]):
        i.box('Clipboard', (px + .008, y, z), (.012, .23, .32), 'Timber')
        i.box('Clipboard_paper', (px + .016, y, z - .02), (.004, .21, .27), 'Paper')
        i.box('Clipboard_clip', (px + .02, y, z + .14), (.012, .08, .03), 'Metal')
    # Olive filing cabinets beside the map, drawers toward the window, files and a box on top.
    for y in (-3.5, -4.25):
        cabinet(i, -6.62, y, .62, .7, 1.32, 4, lod)
    i.box('Records_carton', (-6.62, -3.5, 1.45), (.4, .32, .26), 'Paper')
    for k in range(3 if lod == 0 else 1):
        i.box('Loose_file_folder', (-6.6 + .01 * k, -4.25, 1.325 + .012 * k), (.32, .24, .01), 'Paper' if k % 2 == 0 else 'LampFabric')
    # Water cooler and coat rack with a hat near the door.
    i.box('Water_cooler_cabinet', (-6.65, 1.65, .5), (.34, .34, 1.0), 'CreamEnamel')
    i.lathe('Water_cooler_bottle', (-6.65, 1.65, 1.0), [(0, .04), (.06, .14), (.36, .14), (.42, .06), (.46, .04)], 'WasherGlass', n)
    i.lathe('Coat_rack_base', (-.75, 5.35, 0), [(0, .2), (.03, .18), (.05, .03)], 'DarkWood', max(8, n // 2))
    i.tube('Coat_rack_pole', (-.75, 5.35, .05), (-.75, 5.35, 1.82), .02, 'DarkWood', max(6, n // 2))
    for k in range(4):
        a = k * math.pi / 2 + .4
        i.tube('Coat_rack_hook', (-.75, 5.35, 1.7), (-.75 + .16 * math.cos(a), 5.35 + .16 * math.sin(a), 1.78), .01, 'DarkWood', 4)
    i.lathe('Hat_brim', (-.75 + .14, 5.35 + .06, 1.76), [(0, .2), (.012, .19), (.02, .1)], 'Upholstery', max(8, n // 2), caps=False)
    i.lathe('Hat_crown', (-.75 + .14, 5.35 + .06, 1.77), [(0, .1), (.1, .095), (.13, .06), (.135, .0)], 'Upholstery', max(8, n // 2), caps=False)
    i.rounded_box('Hanging_jacket', (-.75 - .12, 5.35 - .05, 1.35), (.14, .46, .7), .05, 'BookCloth', 2)


def interior_lights(f, lod, n):
    for x, y, z, ln, mat in ((-3.6, -1.3, 3.08, 1.2, 'SheriffTubeGlow'), (-3.6, 3.3, 3.08, 1.2, 'SheriffTubeGlow'), (-2.0, -4.3, 3.08, 1.2, 'SheriffTubeGlow'), (-8.15, 4.3, 2.9, .9, 'TubeGlow')):
        if lod < 2:
            f.box('Fluorescent_folded_housing', (x, y, z), (.22, ln, .09), 'Metal')
            for dx in (-.055, .055):
                f.tube('Frosted_fluorescent_tube', (x + dx, y - ln / 2 + .06, z - .055), (x + dx, y + ln / 2 - .06, z - .055), .019, mat, [8, 5][lod])
        else:
            f.box('Fluorescent_folded_housing', (x, y, z), (.22, ln, .09), 'Metal')
            f.box('Fluorescent_diffuser', (x, y, z - .05), (.18, ln - .1, .012), mat)
        if lod == 0 and z > 3:
            for dy in (-ln / 2 + .1, ln / 2 - .1):
                f.tube('Light_hanger_chain', (x, y + dy, z + .045), (x, y + dy, H - .14), .004, 'Metal', 3, caps=False)
    # Two lamps with lit fabric shades: the warm points seen through the front window at night.
    desk_lamp(f, -4.0, -1.6, .79, lod)
    desk_lamp(f, -.45, -.35, .75, lod)


def desk_lamp(f, lx, ly, lz, lod):
    f.lathe('Desk_lamp_base', (lx, ly, lz), [(0, .1), (.02, .1), (.035, .03)], 'DarkMetal', [10, 8, 6][lod])
    f.tube('Desk_lamp_stem', (lx, ly, lz + .03), (lx, ly, lz + .36), .01, 'SheriffBrass', [6, 5, 4][lod])
    f.lathe('Desk_lamp_lit_shade', (lx, ly, lz + .26), [(0, .19), (.2, .1)], 'SheriffShadeGlow', [16, 10, 8][lod], caps=False)
    if lod == 0:
        f.lathe('Desk_lamp_bulb', (lx, ly, lz + .28), [(0, .02), (.03, .03), (.07, .02)], 'SheriffLampGlow', 8)


# ------------------------------------------------------------------ build

# TB1 interior details that are not worth their triangles at the lower LODs.
TB1_DROP = {1: {'Typewriter_round_key', 'Drawer_pull'},
            2: {'Typewriter_round_key', 'Drawer_pull', 'Drawer_label_card', 'Bound_record_volume', 'Cloth_book_spine', 'Dial_finger_hole', 'Desk_fan_wire_guard'}}


def without_tags(m, drop):
    out = SMesh(m.name)
    for fc, uv, mat, sm, tag in zip(m.f, m.uv, m.mat, m.smooth, m.tags):
        if tag in drop:
            continue
        start = len(out.v)
        out.v.extend(m.v[k] for k in fc)
        out.f.append(list(range(start, start + len(fc)))); out.uv.append(uv); out.mat.append(mat); out.smooth.append(sm); out.tags.append(tag)
    return out

def build(lod):
    tb1 = town_geometry.sheriff(lod)
    i = tb1['SM_Town_Sheriff_Interior']
    i.__class__ = SMesh
    # TB1 archive shelf at y=5.7 poked 9 cm through the side wall: slide that bay 0.4 m inward.
    for fi, tag in enumerate(i.tags):
        if tag in ('Archive_shelves', 'Bound_record_volume', 'Cloth_book_spine'):
            vs = [i.v[k] for k in i.f[fi]]
            if min(v[1] for v in vs) > 4.9:
                for k in i.f[fi]:
                    i.v[k] = (i.v[k][0], i.v[k][1] - .4, i.v[k][2])
    i = without_tags(i, TB1_DROP.get(lod, set()))
    s = SMesh('SM_Town_Sheriff_Shell'); g = SMesh('SM_Town_Sheriff_Glass'); f = SMesh('SM_Town_Sheriff_Fixtures')
    st = 'SheriffStucco'

    # Walls with real openings; the four corners are separate blocks with a rounded outer edge.
    wall_holes(s, 'front', Y0 + T, Y1 - T, HF, [(DU0, DU1, 0, DOOR_H), (WU0, WU1, WZ0, WZ1)], st)
    wbox(s, 'front', 'Raised_centre_parapet', sum(RAISE_U) / 2, (HF + HR) / 2, -.11, RAISE_U[1] - RAISE_U[0], HR - HF, .22, st)
    wall_holes(s, 'rear', -(Y1 - T), -(Y0 + T), HS, [], st)
    wall_holes(s, 'left', X0 + T, X1 - T, HS, [SIDE_WIN], st)
    wall_holes(s, 'right', -(X1 - T), -(X0 + T), HS, [(-SIDE_WIN[1], -SIDE_WIN[0], SIDE_WIN[2], SIDE_WIN[3])], st)
    s.box('Deep_foundation', ((X0 + X1) / 2, (Y0 + Y1) / 2, -.35), (X1 - X0 + .3, Y1 - Y0 + .3, .70), 'Concrete')
    CORNERS = ((X1, Y0, 1, -1, HF), (X1, Y1, 1, 1, HF), (X0, Y0, -1, -1, HS), (X0, Y1, -1, 1, HS))
    for cx, cy, sx, sy, top in CORNERS:
        soft_corner(s, cx, cy, sx, sy, 0, top, .09, lod, st)
        soft_corner(s, cx, cy, sx, sy, top - .04, top + .05, .12, lod, 'SheriffCoping', 'Rounded_coping_corner', h=T + .03)
        soft_corner(s, cx, cy, sx, sy, 0, .53, .115, lod, 'SheriffStuccoBase', 'Rounded_base_corner', h=T + .025)

    # Copings: stepped front, lower sides and rear, each with its own wandering top line.
    e = T + .03
    coping(s, 'front', Y0 + e, RAISE_U[0], HF, lod, .3)
    coping(s, 'front', RAISE_U[1], Y1 - e, HF, lod, 1.1)
    coping(s, 'front', RAISE_U[0] - .03, RAISE_U[1] + .03, HR, lod, 2.0)
    coping(s, 'rear', -(Y1 - e), -(Y0 + e), HS, lod, 2.9)
    coping(s, 'left', X0 + e, X1 - T, HS, lod, 3.7)
    coping(s, 'right', -(X1 - T), -(X0 + e), HS, lod, 4.4)
    if lod < 2:
        for face, u0, u1 in (('front', Y0 + T + .1, Y1 - T - .1), ('rear', -(Y1 - T - .1), -(Y0 + T + .1)), ('left', X0 + T + .1, X1 - T - .1), ('right', -(X1 - T - .1), -(X0 + T + .1))):
            wbox(s, face, 'Parapet_counterflashing', (u0 + u1) / 2, ROOF + .2, -.226, u1 - u0, .05, .012, 'Metal')

    # Base course on all four sides (stops at the door), ending at the rounded corner pieces.
    b = T + .025
    base_course(s, 'front', Y0 + b, Y1 - b, [(DU0 - .065, DU1 + .065)])
    base_course(s, 'rear', -(Y1 - b), -(Y0 + b))
    base_course(s, 'left', X0 + b, X1 - b)
    base_course(s, 'right', -(X1 - b), -(X0 + b))

    # Front window, door, canopy, signage.
    window_unit(s, g, 'front', WU0, WU1, WZ0, WZ1, lod, mullions=(WU0 + .78, WU1 - .78), bars=True, bar_step=.26)
    for face, u0, u1 in (('left', SIDE_WIN[0], SIDE_WIN[1]), ('right', -SIDE_WIN[1], -SIDE_WIN[0])):
        window_unit(s, g, face, u0, u1, SIDE_WIN[2], SIDE_WIN[3], lod, mullions=((u0 + u1) / 2,), bars=lod < 2, bar_step=.15, lintel=True)
    entry(s, g, f, lod)
    canopy(s, f, lod)
    # Large cast star and raised letters on the raised parapet (the TB1 sign box is gone).
    lh = .40
    tw = letters_width('SHERIFF', lh)
    star_r = .28
    total = 2 * star_r * 1.24 + .16 + tw
    left = sum(RAISE_U) / 2 - total / 2
    star_badge(s, 'front', left + star_r * 1.24, 3.95, star_r, lod)
    raised_letters(s, 'front', 'SHERIFF', left + 2 * star_r * 1.24 + .16, 3.95 - lh / 2, lh, .045, 'SheriffLetters')
    for uu in (left + 2 * star_r * 1.24 + .16 + tw * .22, left + 2 * star_r * 1.24 + .16 + tw * .78):
        gooseneck(f, 'front', uu, 4.33, lod)

    # Street furniture kept within 0.6 m of the wall.
    bulletin_case(s, g, 'front', 2.55, 1.6, lod)
    bench(s, 'front', WIN_Y, lod)
    trash_can(s, 'front', 1.3, lod)

    # Side services.
    service_entrance(s, g, 'right', 1.25, lod)
    f.lathe('Bulkhead_light_base', P('right', 2.3, 2.55, 0), [(0, .09), (.03, .09)], 'DarkMetal', [14, 8, 6][lod], axis=(0, 1, 0))
    f.lathe('Bulkhead_light_lens', P('right', 2.3, 2.55, .03), [(0, .075), (.04, .065), (.06, .0)], 'SheriffLampGlow', [14, 8, 6][lod], axis=(0, 1, 0), caps=False)
    if lod < 2:
        for k in range(3):
            f.tube('Bulkhead_cage_bar', P('right', 2.3 - .08 + .08 * k, 2.55 - .08, .035), P('right', 2.3 - .08 + .08 * k, 2.55 + .08, .035), .005, 'DarkMetal', 3, caps=False)
    gas_meter(s, 'left', -8.6, lod)
    hose_bib(s, 'left', -2.6, lod)
    canale(s, 'left', -7.4, lod)
    canale(s, 'right', 7.6, lod)
    if lod < 2:
        canale(s, 'rear', -3.0, lod)
        canale(s, 'rear', 3.0, lod)

    # Weathering: repair patches, settlement cracks off the window corners.
    patches(s, 'front', [(-5.0, 1.3, .45, .3), (-4.6, 3.05, .35, .22), (3.1, 2.9, .3, .25), (5.6, 1.2, .3, .4), (-0.2, .62, .4, .08)], lod)
    patches(s, 'left', [(-6.6, 2.7, .5, .35), (-1.5, 1.6, .35, .5), (-9.2, 1.0, .3, .25)], lod)
    patches(s, 'right', [(3.6, 3.0, .4, .3), (6.5, 1.3, .45, .3), (8.8, 2.4, .3, .4)], lod)
    if lod < 2:
        strip(s, 'front', 'Settlement_crack', [(WU0 - .03, WZ1 + .03), (WU0 - .18, WZ1 + .22), (WU0 - .26, WZ1 + .31), (WU0 - .42, WZ1 + .55)], .009, .002, 'SheriffCrack')
        strip(s, 'front', 'Settlement_crack', [(WU1 + .03, WZ0 - .05), (WU1 + .14, WZ0 - .22), (WU1 + .22, WZ0 - .28), (WU1 + .3, WZ0 - .5)], .008, .002, 'SheriffCrack')
        strip(s, 'front', 'Settlement_crack', [(DU1 + .1, 1.75), (DU1 + .22, 1.55), (DU1 + .28, 1.38), (DU1 + .4, 1.2)], .008, .002, 'SheriffCrack')
        strip(s, 'right', 'Settlement_crack', [(-SIDE_WIN[1] - .05, SIDE_WIN[3] + .04), (-SIDE_WIN[1] - .2, SIDE_WIN[3] + .3), (-SIDE_WIN[1] - .26, SIDE_WIN[3] + .6)], .008, .002, 'SheriffCrack')

    roof(s, f, lod)
    interior_extra(i, f, lod)
    return {m.name: m for m in (s, g, i, f)}


if __name__ == '__main__':
    for lod in (0, 1, 2):
        parts = build(lod)
        print(lod, {k: sum(len(fc) - 2 for fc in m.f) for k, m in parts.items()})
