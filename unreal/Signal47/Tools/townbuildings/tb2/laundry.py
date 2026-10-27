"""TB2 Laundry: worn 1986 coin laundromat in Caliche, New Mexico.

Pure Python procedural geometry, same local frame as TB1 (metres, Z up, front at x=0 facing +x).
build(lod) returns the four chunks SM_Town_Laundry_Shell/Glass/Interior/Fixtures.
Footprint, wall height, door and window openings come unchanged from design.BUILDINGS['Laundry'].
New materials live in tb2/materials_laundry.json (M_Town_Laundry*).
"""
import math
import random
import sys
from pathlib import Path

_R = Path(__file__).resolve().parent.parent
if str(_R) not in sys.path:
    sys.path.insert(0, str(_R))

import geometry as G  # noqa: E402
from geometry import Mesh  # noqa: E402
from design import BUILDINGS  # noqa: E402

B = BUILDINGS['Laundry']
X0, X1, Y0, Y1 = B['footprint']          # -10, 0, -7, 7
H = B['height']                          # 3.4 main wall / roof deck
T = .22
HT = T / 2
DOOR_Y, DOOR_W, DOOR_H = B['door']       # -4.4, 1.1, 2.2
WINS = B['windows']                      # [y, w, z0, z1]
PAR = 3.75                               # side and rear parapet top
FPAR = 4.05                              # front false-front parapet top
RET_X = -1.0                             # where the false front return steps down
CEIL = 3.0                               # suspended ceiling inside
CAN_Z0, CAN_Z1 = 2.47, 2.55              # canopy slab
CAN_Y0, CAN_Y1 = -5.55, 6.25
CAN_X = .78                              # canopy outer edge (above 2.3 m, may project)
BAND_Z = 2.6                             # smooth stucco sign band starts here on the front

DOOR_HOLE = (DOOR_Y - DOOR_W / 2, DOOR_Y + DOOR_W / 2, 0.0, DOOR_H)
WIN_HOLES = [(y - w / 2, y + w / 2, z0, z1) for y, w, z0, z1 in WINS]

# Wall frames: origin on the outer face, tangent t (u axis), outward normal n; t x Z == n.
WALLS = {
    'front': ((X1 + HT, 0, 0), (0, 1, 0), (1, 0, 0)),
    'right': ((0, Y1 + HT, 0), (-1, 0, 0), (0, 1, 0)),
    'left': ((0, Y0 - HT, 0), (1, 0, 0), (0, -1, 0)),
    'rear': ((X0 - HT, 0, 0), (0, -1, 0), (-1, 0, 0)),
    # inner faces, normal pointing into the room
    'front_in': ((X1 - HT, 0, 0), (0, -1, 0), (-1, 0, 0)),
    'right_in': ((0, Y1 - HT, 0), (1, 0, 0), (0, -1, 0)),
    'left_in': ((0, Y0 + HT, 0), (-1, 0, 0), (0, 1, 0)),
    'rear_in': ((X0 + HT, 0, 0), (0, 1, 0), (1, 0, 0)),
}


# ---------------------------------------------------------------- helpers
def wp(w, u, z, d=0.0):
    o, t, n = WALLS[w]
    return (o[0] + t[0] * u + n[0] * d, o[1] + t[1] * u + n[1] * d, z)


def wn(w):
    return WALLS[w][2]


def wrect(m, w, u0, u1, z0, z1, d, mat, name=None):
    if name:
        m.tag = name
    if u1 - u0 < 1e-4 or z1 - z0 < 1e-4:
        return
    m.face([wp(w, u0, z0, d), wp(w, u1, z0, d), wp(w, u1, z1, d), wp(w, u0, z1, d)], mat)


def wtrap(m, w, u, zt, zb, wt, wb, d, mat, skew=0.0):
    """Trapezoid stain on a wall: top width wt at zt, bottom width wb at zb (zb < zt)."""
    m.face([wp(w, u + skew - wb / 2, zb, d), wp(w, u + skew + wb / 2, zb, d),
            wp(w, u + wt / 2, zt, d), wp(w, u - wt / 2, zt, d)], mat)


def wbox(m, name, w, u, z, d, su, sz, sd, mat):
    """Axis aligned box placed in a wall frame; d is the centre distance from the face."""
    _, t, _ = WALLS[w]
    c = wp(w, u, z, d)
    sx = su if abs(t[0]) > .5 else sd
    sy = su if abs(t[1]) > .5 else sd
    m.box(name, c, (sx, sy, sz), mat)


def bb(m, name, x0, x1, y0, y1, z0, z1, mat):
    G.box_bounds(m, name, min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1), min(z0, z1), max(z0, z1), mat)


def hface(m, pts, mat, up=True):
    """Horizontal polygon given counter clockwise seen from above."""
    m.face(pts if up else pts[::-1], mat)


def rects_minus(u0, u1, z0, z1, holes):
    cuts = sorted(set([u0, u1] + [min(u1, max(u0, a)) for h in holes for a in h[:2]]))
    out = []
    for a, b in zip(cuts, cuts[1:]):
        if b - a < 1e-6:
            continue
        iv = sorted((max(z0, h[2]), min(z1, h[3])) for h in holes
                    if h[0] < (a + b) / 2 < h[1] and h[3] > z0 and h[2] < z1)
        last = z0
        for lo, hi in iv:
            if lo > last + 1e-6:
                out.append((a, b, last, lo))
            last = max(last, hi)
        if z1 > last + 1e-6:
            out.append((a, b, last, z1))
    return out


_SPOTS = {}


def blob(m, w, u, z, d, r, mat, rng, n=6, squash=1.0, layer=.0004):
    """Irregular chip of paint as a triangle fan, always valid faces.
    Chips never overlap another chip in the same plane (coplanar overlaps z-fight)."""
    key = (w, round(d, 4))
    taken = _SPOTS.setdefault(key, [])
    if any(math.hypot(u - a, (z - c) / max(squash, .3)) < (r + rb) * 1.2 for a, c, rb in taken):
        return
    taken.append((u, z, r))
    pts = []
    a0 = rng.random() * math.tau
    for i in range(n):
        a = a0 + i * math.tau / n
        rr = r * (.55 + .6 * rng.random())
        pts.append((u + rr * math.cos(a), z + rr * math.sin(a) * squash))
    c = wp(w, u, z, d)
    for i in range(n):
        p, q = pts[i], pts[(i + 1) % n]
        m.face([c, wp(w, p[0], p[1], d), wp(w, q[0], q[1], d)], mat)


# Block coursing: 16 x 8 inch nominal painted concrete masonry units as real relief.
BLK_L, BLK_H, JNT, BLK_D = .406, .20, .012, .010


def coursing(m, w, u0, u1, z0, z1, holes, matf, stagger=0.0, heads=True):
    """Proud block faces with real joints; the wall core behind reads as mortar.
    heads=False (LOD1) keeps the bed joints but merges each course run into one face."""
    k = int(round(z0 / BLK_H))
    z = z0
    while z < z1 - .02:
        zt = min((k + 1) * BLK_H, z1)
        off = stagger + (k % 2) * BLK_L / 2
        for a, b, za, zb in rects_minus(u0, u1, z, zt, holes):
            fa = za + (JNT / 2 if za > z0 + 1e-6 else 0)
            fb = zb - (JNT / 2 if zb < z1 - 1e-6 else 0)
            if fb - fa < .02:
                continue
            m.tag = 'Painted_block_face'
            if not heads:
                wrect(m, w, a, b, fa, fb, BLK_D, matf(w, (a + b) / 2, (fa + fb) / 2))
                g = b
            else:
                g = off + math.floor((a - off) / BLK_L) * BLK_L
            while heads and g < b - 1e-6:
                ua, ub = max(a, g + JNT / 2), min(b, g + BLK_L - JNT / 2)
                if ub - ua > .015:
                    wrect(m, w, ua, ub, fa, fb, BLK_D, matf(w, (ua + ub) / 2, (fa + fb) / 2))
                g += BLK_L
            m.tag = 'Block_course_return'
            mid = matf(w, (a + b) / 2, (fa + fb) / 2)
            if zb < z1 - 1e-6:
                m.face([wp(w, a, fb, 0), wp(w, a, fb, BLK_D), wp(w, b, fb, BLK_D), wp(w, b, fb, 0)], mid)
            if za > z0 + 1e-6:
                m.face([wp(w, a, fa, 0), wp(w, b, fa, 0), wp(w, b, fa, BLK_D), wp(w, a, fa, BLK_D)], mid)
        z = zt
        k += 1


def flat_paint(m, w, u0, u1, z0, z1, holes, matf, d=.004):
    """LOD2 replacement for coursing: painted planes split at the blue band."""
    m.tag = 'Painted_wall_plane'
    for a, b, za, zb in rects_minus(u0, u1, z0, z1, holes):
        for lo, hi in [(za, min(zb, .6)), (max(za, .6), zb)]:
            if hi - lo > 1e-4:
                wrect(m, w, a, b, lo, hi, d, matf(w, (a + b) / 2, (lo + hi) / 2))


# ---------------------------------------------------------------- lettering
def _ell(cx, cy, rx, ry, a0, a1, n):
    return [(cx + rx * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
             cy + ry * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]


def font(n):
    return {
        'L': (.50, [[(0, 1), (0, 0), (.5, 0)]]),
        'A': (.64, [[(0, 0), (.32, 1), (.64, 0)], [(.14, .34), (.50, .34)]]),
        'U': (.60, [[(0, 1)] + _ell(.3, .32, .3, .32, 180, 360, n) + [(.6, 1)]]),
        'N': (.60, [[(0, 0), (0, 1), (.6, 0), (.6, 1)]]),
        'D': (.60, [[(.22, 0), (0, 0), (0, 1), (.22, 1)] + _ell(.22, .5, .38, .5, 90, -90, n + 2)]),
        'R': (.60, [[(0, 0), (0, 1), (.3, 1)] + _ell(.3, .74, .27, .26, 90, -90, n) + [(0, .48)], [(.26, .48), (.6, 0)]]),
        'O': (.62, [_ell(.31, .5, .31, .5, 0, 360, 2 * n + 4)]),
        'M': (.74, [[(0, 0), (0, 1), (.37, .36), (.74, 1), (.74, 0)]]),
        'T': (.60, [[(0, 1), (.6, 1)], [(.3, 1), (.3, 0)]]),
        'P': (.58, [[(0, 0), (0, 1), (.3, 1)] + _ell(.3, .74, .27, .26, 90, -90, n) + [(0, .48)]]),
        'E': (.52, [[(.52, 1), (0, 1), (0, 0), (.52, 0)], [(0, .5), (.42, .5)]]),
    }


def layout(text, h, gap, n):
    f = font(n)
    x = 0.0
    out = []
    for ch in text:
        wid, strokes = f[ch]
        out.append((x, strokes))
        x += (wid + gap) * h
    return out, x - gap * h


def _stroke_outline(pts, th):
    """Mitred offset of a polyline: left and right point lists, shared between segments."""
    closed = math.dist(pts[0], pts[-1]) < 1e-6
    if closed:
        pts = pts[:-1]
    k = len(pts)
    left, right = [], []
    for i in range(k):
        if closed:
            a, b, c = pts[i - 1], pts[i], pts[(i + 1) % k]
        else:
            a = pts[i - 1] if i > 0 else None
            b = pts[i]
            c = pts[i + 1] if i < k - 1 else None
        dirs = []
        for p, q in [(a, b), (b, c)]:
            if p is not None and q is not None:
                ln = math.dist(p, q)
                dirs.append(((q[0] - p[0]) / ln, (q[1] - p[1]) / ln))
        dx, dy = sum(d[0] for d in dirs), sum(d[1] for d in dirs)
        ln = math.hypot(dx, dy) or 1.0
        dx, dy = dx / ln, dy / ln
        px, py = -dy, dx
        cosh = max(.55, abs(px * -dirs[0][1] + py * dirs[0][0]))
        r = th / 2 / cosh
        left.append((b[0] + px * r, b[1] + py * r))
        right.append((b[0] - px * r, b[1] - py * r))
    if closed:
        left.append(left[0])
        right.append(right[0])
    return left, right


def painted_text(m, w, text, uc, z0, h, th, d, mat, n):
    """Flat painted letters on a wall. Mitred strokes share edges, so nothing overlaps
    coplanar; crossing strokes sit on their own depth layer."""
    items, width = layout(text, h, .26, n)
    m.tag = 'Painted_wall_letters'
    for x, strokes in items:
        for si, line in enumerate(strokes):
            pts = [(uc - width / 2 + x + ax * h, z0 + az * h) for ax, az in line]
            pts = [p for i, p in enumerate(pts) if i == 0 or math.dist(p, pts[i - 1]) > 1e-6]
            left, right = _stroke_outline(pts, th)
            dd = d + si * .0006
            for i in range(len(pts) - 1 if math.dist(pts[0], pts[-1]) > 1e-6 else len(left) - 1):
                quad = [right[i], right[i + 1], left[i + 1], left[i]]
                m.face([wp(w, u, z, dd) for u, z in quad], mat)
    return width


def neon_text(m, text, x, yc, z0, h, r, mat, n, sides):
    """Bent glass neon tubes, letters facing +x (readable from the street)."""
    items, width = layout(text, h, .3, n)
    m.tag = 'Neon_tube'
    for ox, strokes in items:
        for line in strokes:
            pts = [(x, yc - width / 2 + ox + px * h, z0 + pz * h) for px, pz in line]
            for a, b in zip(pts, pts[1:]):
                if math.dist(a, b) > 1e-4:
                    m.tube('Neon_tube', a, b, r, mat, sides)
    return width


# ---------------------------------------------------------------- appliances (local, facing +x)
def rbox(m, lod, name, c, size, r, mat):
    if lod >= 2:
        m.box(name, c, size, mat)
    else:
        m.rounded_box(name, c, size, r, mat, 2)


def frontloader(lod, load=None):
    m = Mesh('frontloader')
    n = [16, 10, 6][lod]
    k = [4, 3, 3][lod]
    D, W = .70, .76
    fx = D / 2
    m.box('Washer_toe_kick', (-.02, 0, .04), (D - .08, W - .06, .08), 'DarkMetal')
    rbox(m, lod, 'Washer_enamel_cabinet', (0, 0, .54), (D, W, .92), .03, 'CreamEnamel')
    m.box('Washer_stainless_console', (fx - .08, 0, 1.08), (.16, W, .16), 'LaundryAluminum')
    m.box('Coin_slide', (fx + .012, .15, 1.08), (.026, .17, .065), 'DarkMetal')
    m.box('Coin_slide_handle', (fx + .03, .15, 1.08), (.02, .05, .025), 'Metal')
    if lod < 1:
        m.lathe('Cycle_selector', (fx, -.22, 1.08), [(0, .032), (.028, .032), (.034, .02)], 'Rubber', max(5, n // 2), axis=(1, 0, 0))
    else:
        m.box('Cycle_selector', (fx + .015, -.22, 1.08), (.03, .05, .05), 'Rubber')
    m.box('Instruction_label', (fx + .002, -.04, 1.08), (.004, .17, .1), 'Paper')
    m.box('Detergent_drawer', (fx + .006, -.2, .9), (.014, .26, .085), 'LaundryAluminum')
    if lod < 2:
        m.lathe('Drum_shadow', (fx + .001, 0, .5), [(0, .205), (.004, .205)], 'DarkMetal', n, axis=(1, 0, 0))
    if load and lod < 2:
        m.lathe('Wet_laundry_load', (fx + .004, .03, .45), [(0, .15), (.03, .16), (.05, .1)], load, max(6, n // 2), axis=(1, 0, 0))
    m.torus('Chrome_door_ring', (fx + .035, 0, .5), .225, .034, 'Metal', n, k, axis=(1, 0, 0))
    m.lathe('Porthole_glass', (fx + .045, 0, .5), [(0, .195), (.015, .17), (.02, .1)], 'WasherGlass', n, axis=(1, 0, 0))
    m.box('Door_hinge_block', (fx + .03, -.25, .5), (.05, .06, .14), 'Metal')
    m.box('Door_latch_handle', (fx + .06, .245, .5), (.04, .035, .13), 'Metal')
    if lod < 2:
        m.box('Service_panel_seam', (fx + .002, 0, .16), (.004, W - .1, .012), 'DarkMetal')
    return m


def toploader(lod):
    m = Mesh('toploader')
    n = [10, 8, 6][lod]
    rbox(m, lod, 'Toploader_cabinet', (0, 0, .45), (.66, .66, .9), .03, 'CreamEnamel')
    m.box('Toploader_lid', (.03, 0, .912), (.5, .56, .024), 'CreamEnamel')
    m.box('Lid_seam_shadow', (.03, 0, .9005), (.52, .58, .004), 'DarkMetal')
    m.box('Toploader_backsplash', (-.27, 0, 1.0), (.12, .66, .2), 'LaundryAluminum')
    m.box('Coin_slide', (-.205, .12, 1.0), (.03, .17, .06), 'DarkMetal')
    if lod < 1:
        m.lathe('Cycle_selector', (-.21, -.18, 1.0), [(0, .03), (.026, .03)], 'Rubber', n, axis=(1, 0, 0))
    if lod < 2:
        m.box('Toploader_label', (-.209, -.02, 1.04), (.004, .12, .06), 'Paper')
    return m


def dryer_stack(lod, loads=(None, None)):
    m = Mesh('dryer')
    n = [14, 9, 6][lod]
    k = [4, 3, 3][lod]
    D, W = .80, .80
    fx = D / 2
    m.box('Dryer_base', (-.02, 0, .05), (D - .06, W - .04, .1), 'DarkMetal')
    rbox(m, lod, 'Stack_dryer_cabinet', (0, 0, 1.0), (D, W, 1.8), .03, 'CreamEnamel')
    for zc, load in zip([.6, 1.38], loads):
        if lod < 2:
            m.lathe('Drum_shadow', (fx + .001, 0, zc), [(0, .25), (.004, .25)], 'DarkMetal', n, axis=(1, 0, 0))
        if load and lod < 2:
            m.lathe('Tumbling_clothes', (fx + .004, 0, zc - .08), [(0, .17), (.03, .18), (.05, .12)], load, max(6, n // 2), axis=(1, 0, 0))
        m.torus('Dryer_door_ring', (fx + .03, 0, zc), .27, .03, 'Metal', n, k, axis=(1, 0, 0))
        m.lathe('Dryer_window', (fx + .04, 0, zc), [(0, .245), (.012, .23)], 'WasherGlass', n, axis=(1, 0, 0))
        m.box('Dryer_door_pull', (fx + .05, .29, zc), (.03, .03, .12), 'Metal')
    m.box('Dryer_coin_panel', (fx + .01, 0, 1.0), (.02, .72, .14), 'LaundryAluminum')
    for yy in [-.17, .17]:
        m.box('Dryer_coin_slot', (fx + .025, yy, 1.0), (.012, .12, .05), 'DarkMetal')
    m.box('Dryer_header_label', (fx + .003, 0, 1.83), (.006, .5, .08), 'Paper')
    if lod < 2:
        m.box('Lint_screen_slot', (fx + .004, 0, .2), (.008, .5, .035), 'DarkMetal')
    return m


def plastic_chair(lod):
    m = Mesh('chair')
    sides = [6, 5, 4][lod]
    rbox(m, lod, 'Molded_shell_seat', (0, 0, .44), (.42, .44, .035), .05, 'LaundryOrangePlastic')
    rbox(m, lod, 'Molded_shell_back', (-.2, 0, .69), (.035, .42, .3), .03, 'LaundryOrangePlastic')
    for dy in [-.17, .17]:
        for dx in [-.16, .16]:
            m.tube('Chair_tube_leg', (dx * 1.1, dy * 1.1, 0), (dx, dy, .425), .011, 'Metal', sides)
        m.tube('Chair_back_strut', (-.17, dy, .43), (-.2, dy, .56), .011, 'Metal', sides)
    return m


def wire_cart(lod):
    m = Mesh('cart')
    sides = [6, 5, 4][lod]
    hx, hy = .3, .23
    corners = [(-hx, -hy), (hx, -hy), (hx, hy), (-hx, hy)]
    for z in [.48, .86]:
        for (ax, ay), (bx, by) in zip(corners, corners[1:] + corners[:1]):
            m.tube('Basket_rim', (ax, ay, z), (bx, by, z), .01, 'Metal', sides)
    k = [8, 4, 0][lod]
    for i in range(k):
        xx = -hx + (i + .5) * 2 * hx / k
        for yy in [-hy, hy]:
            m.tube('Basket_wire', (xx, yy, .48), (xx, yy, .86), .005, 'Metal', 4)
    if lod < 1:
        for i in range(k):
            xx = -hx + (i + .5) * 2 * hx / k
            m.tube('Basket_floor_wire', (xx, -hy, .48), (xx, hy, .48), .005, 'Metal', 4)
    for dx, dy in corners:
        m.tube('Cart_leg', (dx * .85, dy * .85, .08), (dx, dy, .48), .011, 'Metal', sides)
        if lod < 2:
            m.torus('Cart_caster', (dx * .85, dy * .85, .045), .04, .015, 'Rubber', [8, 5][lod], [4, 3][lod], axis=(1, 0, 0))
        else:
            m.box('Cart_caster', (dx * .85, dy * .85, .045), (.08, .03, .08), 'Rubber')
    for dy in [-hy, hy]:
        m.tube('Hanger_post', (-hx, dy, .86), (-hx, dy, 1.72), .011, 'Metal', sides)
    m.tube('Hanger_rail', (-hx, -hy, 1.72), (-hx, hy, 1.72), .012, 'Metal', sides)
    return m


def place(dst, tpl, x, y, yaw=0.0):
    dst.merge(tpl, (x, y, 0), yaw)


# ---------------------------------------------------------------- build
def build(lod):
    rng = random.Random(1986)
    _SPOTS.clear()
    n = G.seg(lod)
    shell = Mesh('SM_Town_Laundry_Shell')
    glass = Mesh('SM_Town_Laundry_Glass')
    inside = Mesh('SM_Town_Laundry_Interior')
    fix = Mesh('SM_Town_Laundry_Fixtures')

    _walls(shell, lod, rng)
    _storefront(shell, glass, inside, fix, lod)
    _canopy_and_sign(shell, fix, lod, rng)
    _side_services(shell, fix, lod, rng)
    _roof(shell, fix, lod)
    _street_props(shell, lod)
    _interior(inside, fix, lod, rng)
    return {m.name: m for m in [shell, glass, inside, fix]}


def _matf_painted(w, u, z):
    if w == 'rear':
        return 'LaundryRawBlock'
    return 'LaundryFadedBlue' if z < .6 else 'LaundryBlockPaint'


def _walls(s, lod, rng):
    holes = [DOOR_HOLE] + WIN_HOLES
    # structural block core; its exposed joints read as mortar
    G.front_wall(s, X1, Y0 - HT, Y1 + HT, FPAR, holes, 'LaundryMortar', T)
    for yc in [Y0, Y1]:
        bb(s, 'Block_side_wall', X0 - HT, X1 - HT, yc - HT, yc + HT, 0, PAR, 'LaundryMortar')
        bb(s, 'False_front_return', RET_X, X1 - HT, yc - HT, yc + HT, PAR, FPAR, 'LaundryMortar')
    bb(s, 'Raw_block_rear_wall', X0 - HT, X0 + HT, Y0 + HT, Y1 - HT, 0, PAR, 'LaundryMortar')
    bb(s, 'Deep_foundation', X0 - HT - .05, X1 + HT + .05, Y0 - HT - .05, Y1 + HT + .05, -.7, -.01, 'Concrete')

    E = HT + BLK_D  # extend runs so corners close over the neighbouring blocks
    runs = [
        ('front', -(Y1 + E), Y1 + E, 0, BAND_Z, holes, 0.0),
        ('right', -E, -RET_X, 0, FPAR, [], .1),
        ('right', -RET_X, -X0 + E, 0, PAR, [], .1),
        ('left', X0 - E, RET_X, 0, PAR, [], .1),
        ('left', RET_X, E, 0, FPAR, [], .1),
        ('rear', -(Y1 + E), Y1 + E, 0, PAR, [], .2),
    ]
    for w, u0, u1, z0, z1, hl, st in runs:
        if lod < 2 or w == 'front':
            coursing(s, w, u0, u1, z0, z1, hl, _matf_painted, st, heads=lod == 0 or (lod == 1 and w == 'front'))
        else:
            flat_paint(s, w, u0, u1, z0, z1, hl, _matf_painted)

    # smooth stucco sign band on the false front
    s.tag = 'Stucco_sign_band'
    wrect(s, 'front', -(Y1 + E), Y1 + E, BAND_Z, FPAR, .014, 'LaundryBlockPaint')
    s.face([wp('front', -(Y1 + E), BAND_Z, 0), wp('front', Y1 + E, BAND_Z, 0),
            wp('front', Y1 + E, BAND_Z, .014), wp('front', -(Y1 + E), BAND_Z, .014)], 'LaundryBlockPaint')

    # parapet coping, galvanized with drip turn-downs
    cw = T + .07
    bb(s, 'Front_parapet_coping', X1 - HT - .035, X1 + HT + .035, Y0 - HT - .035, Y1 + HT + .035, FPAR, FPAR + .05, 'Metal')
    for yc, sg in [(Y0, -1), (Y1, 1)]:
        bb(s, 'Side_parapet_coping', X0 - HT - .035, RET_X, yc - cw / 2, yc + cw / 2, PAR, PAR + .05, 'Metal')
        bb(s, 'Return_coping', RET_X - .02, X1 - HT, yc - cw / 2, yc + cw / 2, FPAR, FPAR + .05, 'Metal')
        bb(s, 'Return_step_cap', RET_X - .02, RET_X + .02, yc - cw / 2, yc + cw / 2, PAR, FPAR, 'Metal')
        if lod < 2:
            bb(s, 'Coping_drip_edge', X0 - HT - .035, RET_X, yc + sg * (HT + .03), yc + sg * (HT + .036), PAR - .07, PAR, 'Metal')
    bb(s, 'Rear_parapet_coping', X0 - HT - .035, X0 + HT + .035, Y0 - HT, Y1 + HT, PAR, PAR + .05, 'Metal')
    if lod < 2:
        bb(s, 'Coping_drip_edge', X1 + HT + .03, X1 + HT + .036, Y0 - HT - .035, Y1 + HT + .035, FPAR - .07, FPAR, 'Metal')

    # roof deck and built-up roofing
    bb(s, 'Built_up_roof', X0 + HT, X1 - HT, Y0 + HT, Y1 - HT, H, H + .07, 'RoofShingle')
    if lod < 2:
        for yc, sg in [(Y0 + HT, 1), (Y1 - HT, -1)]:
            bb(s, 'Roof_cant_strip', X0 + HT, X1 - HT, yc, yc + sg * .08, H + .07, H + .14, 'RoofShingle')
        for xc, sg in [(X0 + HT, 1), (X1 - HT, -1)]:
            bb(s, 'Roof_cant_strip', xc, xc + sg * .08, Y0 + HT, Y1 - HT, H + .07, H + .14, 'RoofShingle')

    # peeling paint: old cream and raw block showing through
    if lod < 2:
        chips = [190, 80][lod]
        d = BLK_D + .0015
        for _ in range(chips):
            w = rng.choice(['front', 'front', 'front', 'right', 'left'])
            if w == 'front':
                u = rng.uniform(-7.0, 7.0)
                z = rng.choice([rng.uniform(.03, .62), rng.uniform(.03, .62), rng.uniform(.6, 2.4)])
                if any(h[0] - .05 < u < h[1] + .05 and h[2] - .05 < z < h[3] + .05 for h in holes):
                    continue
            elif w == 'right':
                u, z = rng.uniform(.1, 9.9), rng.choice([rng.uniform(.03, .62), rng.uniform(.6, 3.5)])
            else:
                u, z = rng.uniform(-9.9, -.1), rng.choice([rng.uniform(.03, .62), rng.uniform(.6, 3.5)])
            under = 'LaundryBlockPaint' if z < .6 and rng.random() < .65 else 'LaundryRawBlock'
            s.tag = 'Peeled_paint_chip'
            blob(s, w, u, z, d, rng.uniform(.025, .1), under, rng, 6 if lod == 0 else 5, .7)
        # blue paint creeping above the band, stucco patches on the sign band
        for _ in range([40, 16][lod]):
            u = rng.uniform(-6.8, 6.8)
            if any(h[0] - .1 < u < h[1] + .1 for h in WIN_HOLES + [DOOR_HOLE]):
                continue
            s.tag = 'Faded_band_overrun'
            blob(s, 'front', u, .62 + rng.uniform(0, .06), d, rng.uniform(.04, .1), 'LaundryFadedBlue', rng, 5, .6)
        for _ in range([30, 12][lod]):
            s.tag = 'Sign_band_stucco_loss'
            blob(s, 'front', rng.uniform(-6.9, 6.9), rng.uniform(2.7, 3.95), .0146, rng.uniform(.05, .2), 'LaundryRawBlock', rng, 6, .6, .0002)

    # dirt and water: darker base splash and streaks under the coping
    s.tag = 'Ground_splash_grime'
    gd = BLK_D + .001 if lod < 2 else .005
    for w, u0, u1 in [('front', -7.1, DOOR_HOLE[0]), ('front', DOOR_HOLE[1], 7.1), ('right', -.1, 10.1), ('left', -10.1, .1), ('rear', -7.1, 7.1)]:
        steps = max(2, int((u1 - u0) / 1.6))
        for i in range(steps):
            a, b = u0 + i * (u1 - u0) / steps, u0 + (i + 1) * (u1 - u0) / steps
            hz = .09 + .07 * rng.random()
            s.face([wp(w, a, .0, gd), wp(w, b, .0, gd), wp(w, b, hz * (.7 + .6 * rng.random()), gd), wp(w, a, hz, gd)], 'LaundryDirt')
    if lod < 2:
        s.tag = 'Coping_rain_streak'
        for w, u0, u1, top in [('right', .4, 9.6, PAR), ('left', -9.6, -.4, PAR), ('front', -6.8, 6.8, FPAR)]:
            used = []
            for _ in range([7, 4][lod]):
                u = rng.uniform(u0, u1)
                if any(abs(u - v) < .75 for v in used):
                    continue
                used.append(u)
                dd = .0159 if w == 'front' else BLK_D + .002
                ln = rng.uniform(.25, .7) if w == 'front' else rng.uniform(.4, 1.1)
                zt = top - .06
                if w == 'front':
                    ln = min(ln, zt - BAND_Z - .05)
                wtrap(s, w, u, zt, zt - ln, rng.uniform(.3, .6), rng.uniform(.12, .25), dd, 'LaundryStreak', rng.uniform(-.06, .06))


def _storefront(s, g, ins, fix, lod):
    al = 'LaundryAluminum'
    xf0, xf1 = X1 - .02, X1 + .08      # storefront frame depth
    xg = X1 + .03                       # glazing plane
    for (a, b, z0, z1) in WIN_HOLES:
        mid = (a + b) / 2
        bb(s, 'Storefront_head', xf0, xf1, a, b, z1 - .05, z1, al)
        bb(s, 'Storefront_sill_frame', xf0, xf1, a, b, z0, z0 + .05, al)
        bb(s, 'Storefront_jamb', xf0, xf1, a, a + .05, z0 + .05, z1 - .05, al)
        bb(s, 'Storefront_jamb', xf0, xf1, b - .05, b, z0 + .05, z1 - .05, al)
        bb(s, 'Storefront_mullion', xf0, xf1, mid - .025, mid + .025, z0 + .05, z1 - .05, al)
        if lod < 2:
            for ya, yb in [(a + .05, mid - .025), (mid - .025 + .05, b - .05)]:
                ya2 = ya if ya == a + .05 else mid + .025
                for zz in [z0 + .05, z1 - .065]:
                    bb(s, 'Glazing_stop', xg + .002, xg + .02, ya2, yb, zz, zz + .015, al)
        bb(s, 'Sheet_metal_sill', X1 - HT, X1 + HT + .07, a - .05, b + .05, z0 - .02, z0 + .003, 'Metal')
        if lod < 2:
            bb(s, 'Sill_drip', X1 + HT + .06, X1 + HT + .07, a - .05, b + .05, z0 - .06, z0 - .02, 'Metal')
            bb(s, 'Steel_lintel', X1 + HT - .005, X1 + HT + .012, a - .1, b + .1, z1, z1 + .1, 'DarkMetal')
        for ya, yb in [(a + .05, mid - .025), (mid + .025, b - .05)]:
            G.pane(g, xg, (ya + yb) / 2, (z0 + z1) / 2, yb - ya, z1 - z0 - .1, 'Glass', 1)

    # aluminium storefront door, closed; frame, kick plate, push bar, closer
    a, b = DOOR_HOLE[0], DOOR_HOLE[1]
    bb(s, 'Door_frame_jamb', xf0, xf1, a, a + .05, 0, DOOR_H, al)
    bb(s, 'Door_frame_jamb', xf0, xf1, b - .05, b, 0, DOOR_H, al)
    bb(s, 'Door_frame_head', xf0, xf1, a + .05, b - .05, DOOR_H - .06, DOOR_H, al)
    bb(s, 'Bronze_threshold', X1 - HT, X1 + HT + .04, a, b, 0, .015, al)
    lx0, lx1 = X1 + .01, X1 + .055
    la, lb = a + .055, b - .055
    bb(s, 'Door_leaf_hinge_stile', lx0, lx1, la, la + .1, .02, DOOR_H - .065, al)
    bb(s, 'Door_leaf_lock_stile', lx0, lx1, lb - .1, lb, .02, DOOR_H - .065, al)
    bb(s, 'Door_leaf_top_rail', lx0, lx1, la + .1, lb - .1, DOOR_H - .165, DOOR_H - .065, al)
    bb(s, 'Door_leaf_bottom_rail', lx0, lx1, la + .1, lb - .1, .02, .28, al)
    G.pane(g, (lx0 + lx1) / 2, (la + lb) / 2, (.28 + DOOR_H - .165) / 2, lb - la - .2, DOOR_H - .165 - .28, 'Glass', 1)
    bb(fix, 'Stainless_kick_plate', lx1, lx1 + .003, la + .02, lb - .02, .03, .26, 'Metal')
    bb(fix, 'Kick_plate_inside', lx0 - .003, lx0, la + .02, lb - .02, .03, .26, 'Metal')
    sides = [8, 6, 5][lod]
    py = lb - .05
    fix.tube('Door_pull_handle', (lx1 + .06, py, .85), (lx1 + .06, py, 1.3), .015, 'Metal', sides)
    for zz in ([.87, 1.28] if lod < 2 else []):
        fix.tube('Pull_standoff', (lx1, py, zz), (lx1 + .06, py, zz), .01, 'Metal', sides)
    fix.tube('Push_bar', (lx0 - .07, la + .08, 1.0), (lx0 - .07, lb - .06, 1.0), .018, 'Metal', sides)
    for yy in [la + .08, lb - .06]:
        bb(fix, 'Push_bar_bracket', lx0 - .08, lx0, yy - .02, yy + .02, .97, 1.03, 'Metal')
    bb(fix, 'Door_closer_body', X1 - HT - .08, X1 - HT, la + .1, la + .42, DOOR_H + .02, DOOR_H + .1, 'DarkMetal')
    if lod < 2:
        fix.tube('Door_closer_arm', (X1 - HT - .05, la + .4, DOOR_H + .04), (lx0 - .02, la + .55, DOOR_H - .1), .009, 'DarkMetal', 4)
        bb(fix, 'Lock_cylinder', lx1, lx1 + .012, lb - .07, lb - .03, 1.1, 1.14, 'Metal')

    # hours and price cards taped inside the glass
    xp = xg - .012
    ins.panel('Door_hours_card', (xp, DOOR_Y, 1.5), .3, .37, 9, (1, 0, 0))
    ins.panel('Window_price_card', (xp, WIN_HOLES[1][0] + .35, .92), .3, .37, 9, (1, 0, 0))
    if lod < 2:
        ins.tag = 'Tape_corner'
        for yy, zz in [(DOOR_Y - .15, 1.68), (DOOR_Y + .15, 1.68)]:
            bb(ins, 'Tape_corner', xp - .002, xp, yy - .03, yy + .03, zz - .015, zz + .015, 'Paper')

    # neon OPEN in the right window, with chains and transformer
    wy = WINS[0][0] + .55
    xn = X1 - .16
    if lod < 2:
        wid = neon_text(fix, 'OPEN', xn, wy, 1.72, .22, .0085, 'CinemaRedGlow', [4, 2][lod], [6, 4][lod])
        bb(fix, 'Neon_back_rail', xn - .03, xn - .015, wy - wid / 2 - .03, wy + wid / 2 + .03, 1.69, 1.71, 'DarkMetal')
        bb(fix, 'Neon_transformer', xn - .08, xn - .02, wy - .08, wy + .08, 1.58, 1.66, 'DarkMetal')
        for yy in [wy - wid / 2, wy + wid / 2]:
            fix.tube('Neon_hanging_chain', (xn - .02, yy, 1.71), (xn - .02, yy, CEIL), .004, 'Metal', 4)
    else:
        bb(fix, 'Neon_sign_glow', xn - .01, xn, wy - .3, wy + .3, 1.72, 1.94, 'CinemaRedGlow')


def _canopy_and_sign(s, fix, lod, rng):
    xw = X1 + HT + BLK_D
    bb(s, 'Canopy_deck', xw, CAN_X, CAN_Y0, CAN_Y1, CAN_Z0, CAN_Z1, 'CreamEnamel')
    bb(s, 'Canopy_fascia', CAN_X, CAN_X + .025, CAN_Y0, CAN_Y1, CAN_Z0 - .05, CAN_Z1 + .23, 'TealPaint')
    for yy in [CAN_Y0, CAN_Y1]:
        bb(s, 'Canopy_end_cap', xw, CAN_X + .025, yy - .012, yy + .012, CAN_Z0 - .05, CAN_Z1 + .04, 'TealPaint')
    if lod < 2:
        bb(s, 'Canopy_wall_flashing', xw, xw + .06, CAN_Y0, CAN_Y1, CAN_Z1, CAN_Z1 + .05, 'Metal')
        bb(s, 'Canopy_gutter_lip', CAN_X - .03, CAN_X, CAN_Y0, CAN_Y1, CAN_Z1, CAN_Z1 + .04, 'Metal')
    # the existing atlas sign card on the fascia: dark green panel with cream lettering
    s.panel('Fascia_laundromat_sign', (CAN_X + .027, (WIN_HOLES[0][1] + WIN_HOLES[1][0]) / 2, CAN_Z1 + .065), 2.4, .36, 1, (1, 0, 0))
    # tie rods from the sign band to the canopy edge
    sides = [6, 5, 4][lod]
    for yy in [CAN_Y0 + .3, -1.6, 2.4, CAN_Y1 - .3]:
        fix.tube('Canopy_tie_rod', (xw + .02, yy, 2.98), (CAN_X - .02, yy, CAN_Z1 + .02), .011, 'DarkMetal', sides)
        bb(fix, 'Tie_rod_wall_plate', xw + .014, xw + .03, yy - .05, yy + .05, 2.93, 3.03, 'DarkMetal')
        if lod < 2:
            fix.lathe('Tie_rod_turnbuckle', (xw + .25, yy, 2.98 - (2.98 - CAN_Z1) * (.25 / (CAN_X - xw))), [(0, .018), (.08, .018)], 'DarkMetal', sides,
                      axis=G.unit((CAN_X - xw, 0, CAN_Z1 - 2.98)))
    # big painted wall letters, faded blue, on the stucco band like the concept sheet
    letters_n = [4, 3, 2][lod]
    width = painted_text(s, 'front', 'LAUNDROMAT', .3, 3.08, .74, .13, .0172, 'LaundryFadedBlue', letters_n)
    if lod < 2:
        for _ in range([45, 18][lod]):
            u = .3 + rng.uniform(-width / 2, width / 2)
            s.tag = 'Letter_paint_loss'
            blob(s, 'front', u, rng.uniform(3.08, 3.82), .0195, rng.uniform(.025, .07), 'LaundryBlockPaint', rng, 5, .8, .0002)
    # warm lamp over the door under the canopy
    bb(fix, 'Door_lamp_housing', X1 + HT + .2, X1 + HT + .48, DOOR_Y - .13, DOOR_Y + .13, CAN_Z0 - .07, CAN_Z0, 'DarkMetal')
    bb(fix, 'Door_lamp_lens', X1 + HT + .22, X1 + HT + .46, DOOR_Y - .11, DOOR_Y + .11, CAN_Z0 - .1, CAN_Z0 - .07, 'WarmGlow')
    fix.tube('Lamp_conduit', (xw + .01, DOOR_Y, CAN_Z0 - .04), (X1 + HT + .2, DOOR_Y, CAN_Z0 - .04), .012, 'DarkMetal', sides)


def _hood(s, w, u, z, lod):
    """Dryer exhaust hood with gravity flap and dark opening."""
    d0 = BLK_D
    wbox(s, 'Exhaust_hood_flange', w, u, z, d0 + .006, .32, .32, .012, 'Metal')
    wbox(s, 'Exhaust_hood_body', w, u, z + .03, d0 + .1, .26, .2, .18, 'Metal')
    s.tag = 'Exhaust_hood_opening'
    zb = z - .07 - .001
    s.face([wp(w, u - .12, zb, d0 + .025), wp(w, u + .12, zb, d0 + .025), wp(w, u + .12, zb, d0 + .175), wp(w, u - .12, zb, d0 + .175)], 'DarkMetal')
    if lod < 2:
        wbox(s, 'Exhaust_gravity_flap', w, u, z - .1, d0 + .15, .22, .06, .01, 'Metal')


def _lint_stain(s, w, u, z, rng, lod):
    d = BLK_D + .002 if lod < 2 else .006
    s.tag = 'Lint_and_soot_stain'
    wtrap(s, w, u, z - .12, z - rng.uniform(1.0, 1.5), .5, .18, d, 'LaundryLint', rng.uniform(-.06, .06))
    wtrap(s, w, u, z - .15, z - rng.uniform(.45, .7), .26, .06, d + .0008, 'LaundryGrime', rng.uniform(-.03, .03))
    if lod < 2:
        blob(s, w, u, z + .2, d, .26, 'LaundryLint', rng, 7, .6)


def _downspout(s, w, u, lod, top=PAR, ground_shoe=True):
    d0 = BLK_D
    wbox(s, 'Scupper_box', w, u, H + .1, d0 + .07, .22, .14, .16, 'Metal')
    wbox(s, 'Conductor_head', w, u, H - .2, d0 + .1, .3, .28, .2, 'Metal')
    wbox(s, 'Rect_downspout', w, u, (H - .34 + .22) / 2, d0 + .07, .1, H - .34 - .22, .08, 'Metal')
    wbox(s, 'Downspout_kick_shoe', w, u, .14, d0 + .14, .1, .1, .2, 'Metal')
    wbox(s, 'Splash_block', w, u, .02, d0 + .3, .3, .04, .4, 'Concrete')
    if lod < 2:
        for zz in [.9, 1.9, 2.8]:
            wbox(s, 'Downspout_strap', w, u, zz, d0 + .07, .14, .03, .095, 'DarkMetal')


def _side_services(s, fix, lod, rng):
    sides = [8, 6, 5][lod]
    # dryer exhausts on the right (+y) wall and rear wall, with lint stains
    for x in [-7.7, -6.84, -5.98]:
        _hood(s, 'right', -x, 2.55, lod)
        _lint_stain(s, 'right', -x, 2.55, rng, lod)
    for y in [1.0, 1.86, 2.72, 3.58, 4.44]:
        _hood(s, 'rear', -y, 2.55, lod)
        _lint_stain(s, 'rear', -y, 2.55, rng, lod)
    _downspout(s, 'right', 1.5, lod)
    _downspout(s, 'right', 9.45, lod)
    _downspout(s, 'left', -5.6, lod)
    if lod < 2:
        s.tag = 'Downspout_overflow_stain'
        for w, u in [('right', 1.5), ('right', 9.45), ('left', -5.6)]:
            wtrap(s, w, u, H - .34, H - 1.4, .4, .12, BLK_D + .002, 'LaundryGrime')

    # electric service on the left (-y) wall: meter, disconnect, mast, weatherhead
    w = 'left'
    um = -2.4
    wbox(fix, 'Meter_socket_box', w, um, 1.55, BLK_D + .06, .28, .42, .12, 'Metal')
    fix.lathe('Meter_glass_dome', wp(w, um, 1.6, BLK_D + .12), [(0, .085), (.06, .085), (.1, .06), (.12, .02)], 'WasherGlass', [12, 8, 6][lod], axis=wn(w))
    wbox(fix, 'Main_disconnect', w, um + .4, 1.45, BLK_D + .06, .3, .4, .12, 'DarkMetal')
    if lod < 2:
        wbox(fix, 'Disconnect_handle', w, um + .53, 1.5, BLK_D + .13, .03, .12, .03, 'Metal')
        fix.tube('Bonding_conduit', wp(w, um + .14, 1.4, BLK_D + .04), wp(w, um + .25, 1.4, BLK_D + .04), .016, 'Metal', sides)
    dm = BLK_D + .085
    fix.tube('Service_mast', wp(w, um, 1.76, dm), wp(w, um, 4.5, dm), .032, 'Metal', sides)
    fix.tube('Panel_feed_conduit', wp(w, um + .4, 1.25, BLK_D + .04), wp(w, um + .4, .45, BLK_D + .04), .02, 'Metal', sides)
    fix.tube('Panel_feed_entry', wp(w, um + .4, .45, BLK_D + .04), wp(w, um + .4, .45, -.02), .02, 'Metal', sides)
    for zz in ([2.4, 3.3, PAR + .25] if lod < 2 else []):
        wbox(fix, 'Mast_strap', w, um, zz, BLK_D + .04, .12, .03, .07, 'DarkMetal')
    wbox(fix, 'Mast_parapet_standoff', w, um, PAR + .25, BLK_D + .05, .05, .04, .1, 'DarkMetal')
    fix.lathe('Weatherhead', wp(w, um, 4.5, dm), [(0, .034), (.05, .05), (.12, .055), (.15, .02)], 'DarkMetal', sides)
    if lod < 1:
        fix.lathe('Clevis_insulator', wp(w, um + .06, 4.2, dm + .05), [(0, .025), (.05, .035), (.09, .02)], 'Ceramic', sides, axis=wn(w))
        for k, du in enumerate([-.03, 0, .03]):
            pts = [wp(w, um + du, 4.56, dm + .02), wp(w, um + du * 2, 4.4, dm + .2), wp(w, um + .06 + du, 4.12, dm + .14 + .02 * k), wp(w, um + .06, 4.2, dm + .1)]
            for p, q in zip(pts, pts[1:]):
                fix.tube('Drip_loop_conductor', p, q, .007, 'Rubber', 4)
        fix.tube('Ground_wire', wp(w, um - .1, 1.36, BLK_D + .02), wp(w, um - .1, .02, BLK_D + .02), .004, 'Metal', 4)
    # wall pack light at the rear corner of the left wall
    wbox(fix, 'Wall_pack_housing', w, -8.0, 2.95, BLK_D + .09, .26, .3, .18, 'DarkMetal')
    wbox(fix, 'Wall_pack_lens', w, -8.0, 2.88, BLK_D + .19, .2, .16, .02, 'WarmGlow')
    # gas meter set by the utility closet
    ug = -9.0
    gd = BLK_D + .17
    fix.tube('Gas_riser', wp(w, ug, -.05, gd), wp(w, ug, .55, gd), .022, 'OliveEnamel', sides)
    fix.lathe('Gas_regulator', wp(w, ug, .62, gd), [(0, .05), (.03, .09), (.08, .09), (.1, .03)], 'OliveEnamel', sides)
    wbox(fix, 'Gas_meter_body', w, ug + .3, .85, gd, .32, .38, .22, 'Metal')
    fix.tube('Gas_meter_inlet', wp(w, ug, .62, gd), wp(w, ug + .2, .62, gd), .022, 'OliveEnamel', sides)
    fix.tube('Gas_meter_outlet', wp(w, ug + .42, 1.06, gd), wp(w, ug + .42, 1.2, gd), .022, 'OliveEnamel', sides)
    fix.tube('Gas_wall_entry', wp(w, ug + .42, 1.2, gd), wp(w, ug + .42, 1.2, -.03), .022, 'OliveEnamel', sides)
    if lod < 2:
        wbox(fix, 'Gas_meter_index', w, ug + .3, .92, gd + .112, .14, .1, .006, 'Paper')
        fix.torus('Gas_shutoff_handle', wp(w, ug, .35, gd + .04), .03, .008, 'LaundryRedEnamel', 6, 4, axis=wn(w))
        # hose bib
        hb = wp(w, -4.4, .5, BLK_D)
        fix.tube('Hose_bib_pipe', hb, wp(w, -4.4, .5, BLK_D + .12), .014, 'Metal', 6)
        fix.tube('Hose_bib_spout', wp(w, -4.4, .5, BLK_D + .12), wp(w, -4.4, .42, BLK_D + .14), .012, 'Metal', 6)
        fix.torus('Hose_bib_wheel', wp(w, -4.4, .56, BLK_D + .12), .025, .006, 'LaundryRedEnamel', 6, 4)
        s.tag = 'Hose_bib_wet_stain'
        wtrap(s, w, -4.38, .42, .02, .1, .3, BLK_D + .002, 'LaundryGrime')

    # rear steel service door (closed) with hood light
    w = 'rear'
    ud = 3.2  # u = -y on the rear wall
    wbox(s, 'Rear_door_frame', w, ud, 1.06, BLK_D + .02, 1.02, 2.12, .04, 'DarkMetal')
    wbox(s, 'Rear_steel_door', w, ud, 1.03, BLK_D + .04, .9, 2.04, .04, 'LaundryPaleGreen')
    wbox(s, 'Rear_door_sill', w, ud, .03, BLK_D + .08, 1.1, .06, .16, 'Concrete')
    wbox(fix, 'Rear_door_knob_rose', w, ud - .36, 1.0, BLK_D + .07, .06, .1, .02, 'Metal')
    fix.lathe('Rear_door_knob', wp(w, ud - .36, 1.0, BLK_D + .07), [(0, .025), (.05, .03), (.07, .01)], 'Metal', 6, axis=wn(w))
    wbox(fix, 'Rear_light_housing', w, ud, 2.45, BLK_D + .1, .24, .14, .2, 'DarkMetal')
    wbox(fix, 'Rear_light_lens', w, ud, 2.37, BLK_D + .11, .18, .02, .14, 'WarmGlow')


def _roof(s, fix, lod):
    rz = H + .07
    sides = [10, 8, 6][lod]
    # evaporative swamp cooler on an angle iron stand, side draft into a roof jack duct
    cx, cy = -3.4, -3.2
    leg_h = .38
    for dx in [-.48, .48]:
        for dy in [-.48, .48]:
            bb(s, 'Cooler_stand_leg', cx + dx - .025, cx + dx + .025, cy + dy - .025, cy + dy + .025, rz, rz + leg_h, 'DarkMetal')
    for dy in [-.48, .48]:
        bb(s, 'Cooler_stand_rail', cx - .5, cx + .5, cy + dy - .025, cy + dy + .025, rz + leg_h - .04, rz + leg_h, 'DarkMetal')
    cz0, cz1 = rz + leg_h, rz + leg_h + .95
    bb(s, 'Swamp_cooler_cabinet', cx - .48, cx + .48, cy - .48, cy + .48, cz0, cz1, 'RustTin')
    bb(s, 'Swamp_cooler_lid', cx - .52, cx + .52, cy - .52, cy + .52, cz1, cz1 + .05, 'RustTin')
    louvres = [9, 5, 0][lod]
    for k in range(louvres):
        zz = cz0 + .12 + k * .085
        bb(s, 'Cooler_pad_louvre', cx + .48, cx + .51, cy - .4, cy + .4, zz, zz + .03, 'RustTin')
        bb(s, 'Cooler_pad_louvre', cx - .4, cx + .4, cy - .51, cy - .48, zz, zz + .03, 'RustTin')
        bb(s, 'Cooler_pad_louvre', cx - .4, cx + .4, cy + .48, cy + .51, zz, zz + .03, 'RustTin')
    if lod == 2:
        for (a0, a1, b0, b1) in [(cx + .48, cx + .5, cy - .4, cy + .4), (cx - .4, cx + .4, cy - .5, cy - .48), (cx - .4, cx + .4, cy + .48, cy + .5)]:
            bb(s, 'Cooler_pad_face', a0, a1, b0, b1, cz0 + .1, cz1 - .1, 'DarkMetal')
    dz = (cz0 + cz1) / 2 - .05
    bb(s, 'Cooler_supply_duct', cx - 1.25, cx - .48, cy - .28, cy + .28, dz - .28, dz + .28, 'Metal')
    bb(s, 'Cooler_duct_drop', cx - 1.81, cx - 1.25, cy - .28, cy + .28, rz + .2, dz + .28, 'Metal')
    bb(s, 'Roof_jack_curb', cx - 1.9, cx - 1.16, cy - .37, cy + .37, rz, rz + .2, 'RustTin')
    if lod < 2:
        bb(s, 'Duct_seam_band', cx - .9, cx - .86, cy - .29, cy + .29, dz - .29, dz + .29, 'DarkMetal')
        bb(s, 'Duct_canvas_boot', cx - .5, cx - .44, cy - .3, cy + .3, dz - .3, dz + .3, 'Burlap')
        fix.tube('Cooler_water_line', (cx + .3, cy + .48, cz0 + .05), (cx + .3, cy + 1.2, rz + .02), .007, 'RustTin', 5)
        fix.tube('Cooler_water_line', (cx + .3, cy + 1.2, rz + .02), (X0 + HT + .3, cy + 1.2, rz + .02), .007, 'RustTin', 5)
        bb(fix, 'Cooler_float_box', cx + .48, cx + .56, cy + .2, cy + .32, cz0 + .02, cz0 + .1, 'Metal')
    # water heater B-vent with cone cap over the utility closet
    fx, fy = -9.05, -6.0
    s.lathe('Water_heater_flue', (fx, fy, rz), [(0, .16), (.04, .08), (.06, .07), (.95, .07)], 'Metal', sides)
    s.lathe('Flue_storm_collar', (fx, fy, rz + .12), [(0, .1), (.05, .07)], 'Metal', sides, caps=False)
    s.lathe('Flue_cone_cap', (fx, fy, rz + .95), [(0, .07), (.03, .18), (.1, .16), (.2, .02)], 'Metal', sides)
    for k in range(3 if lod < 2 else 0):
        a = k * math.tau / 3
        s.tube('Flue_cap_bracket', (fx + .07 * math.cos(a), fy + .07 * math.sin(a), rz + .95), (fx + .16 * math.cos(a), fy + .16 * math.sin(a), rz + 1.04), .006, 'Metal', 4)
    # plumbing vents and a turbine ventilator
    for vx, vy in [(-7.4, 1.2), (-8.6, -3.0)]:
        s.lathe('Plumbing_vent_boot', (vx, vy, rz), [(0, .16), (.03, .06), (.07, .055)], 'RoofShingle', sides)
        s.lathe('Plumbing_vent_pipe', (vx, vy, rz), [(0, .05), (.42, .05)], 'DarkMetal', sides)
    tx, ty = -2.0, 4.2
    bb(s, 'Turbine_curb', tx - .25, tx + .25, ty - .25, ty + .25, rz, rz + .18, 'RustTin')
    s.lathe('Turbine_throat', (tx, ty, rz + .18), [(0, .17), (.2, .17)], 'Metal', sides)
    s.lathe('Turbine_ventilator', (tx, ty, rz + .38), [(0, .2), (.06, .25), (.22, .24), (.3, .16), (.36, .03)], 'Metal', [16, 12, 8][lod])
    if lod < 2:
        for k in range([12, 8][lod]):
            a = k * math.tau / [12, 8][lod]
            s.tube('Turbine_vane', (tx + .245 * math.cos(a), ty + .245 * math.sin(a), rz + .42), (tx + .24 * math.cos(a + .35), ty + .24 * math.sin(a + .35), rz + .62), .008, 'Metal', 4)
    # roof hatch
    hx, hy = -8.6, 4.4
    bb(s, 'Roof_hatch_curb', hx - .45, hx + .45, hy - .45, hy + .45, rz, rz + .3, 'RustTin')
    bb(s, 'Roof_hatch_lid', hx - .5, hx + .5, hy - .5, hy + .5, rz + .3, rz + .36, 'DarkMetal')
    if lod < 2:
        # ponding stains and patched membrane
        rng = random.Random(47)
        s.tag = 'Roof_patch'
        for k in range(8):
            px, py = rng.uniform(-9.2, -.8), rng.uniform(-6.2, 6.2)
            r = rng.uniform(.3, .8)
            rz = H + .07 + k * .0008
            hface(s, [(px - r, py - r * .7, rz + .003), (px + r, py - r * .6, rz + .003), (px + r * .8, py + r * .7, rz + .003), (px - r * .9, py + r * .5, rz + .003)], 'Rubber')


def _street_props(s, lod):
    sides = [10, 8, 6][lod]
    xw = X1 + HT
    # worn concrete stoop at the door, only 3 cm proud
    bb(s, 'Door_stoop_slab', xw, xw + .58, DOOR_HOLE[0] - .35, DOOR_HOLE[1] + .35, -.06, .03, 'Concrete')
    # bench under the left window, green slats on cast frames
    by0, by1 = WINS[1][0] - .95, WINS[1][0] + .95
    for yy in [by0 + .12, by1 - .12]:
        bb(s, 'Bench_cast_leg_front', xw + .48, xw + .52, yy - .025, yy + .025, 0, .43, 'DarkMetal')
        bb(s, 'Bench_cast_leg_back', xw + .1, xw + .14, yy - .025, yy + .025, 0, .82, 'DarkMetal')
        bb(s, 'Bench_seat_bearer', xw + .1, xw + .54, yy - .025, yy + .025, .4, .44, 'DarkMetal')
        bb(s, 'Bench_foot', xw + .08, xw + .56, yy - .03, yy + .03, 0, .03, 'DarkMetal')
    for k in range(4):
        xx = xw + .15 + k * .1
        bb(s, 'Bench_seat_slat', xx, xx + .075, by0, by1, .44, .47, 'TealPaint')
    for zz in [.56, .7]:
        bb(s, 'Bench_back_slat', xw + .13, xw + .16, by0, by1, zz, zz + .085, 'TealPaint')
    # steel trash can and sand urn by the door
    tx, ty = xw + .32, DOOR_HOLE[1] + .42
    s.lathe('Trash_can_body', (tx, ty, 0), [(0, .2), (.02, .22), (.75, .23)], 'DarkMetal', sides)
    s.lathe('Trash_can_lid', (tx, ty, .75), [(0, .245), (.04, .245), (.08, .1), (.13, .06)], 'DarkMetal', sides)
    if lod < 2:
        for zz in [.18, .55]:
            s.torus('Trash_can_rib', (tx, ty, zz), .225, .012, 'DarkMetal', sides, 4)
    ux, uy = xw + .22, DOOR_HOLE[0] - .55
    s.lathe('Sand_urn', (ux, uy, 0), [(0, .12), (.05, .15), (.58, .15), (.62, .17), (.65, .17)], 'Metal', sides)
    s.lathe('Urn_sand', (ux, uy, .63), [(0, .15), (.005, .15)], 'Concrete', sides)
    # newspaper vending box at the left corner
    nx, ny = xw + .27, -6.45
    bb(s, 'News_box_pedestal', nx - .1, nx + .1, ny - .1, ny + .1, 0, .32, 'DarkMetal')
    bb(s, 'News_box_body', nx - .2, nx + .2, ny - .22, ny + .22, .32, .92, 'LaundryRedEnamel')
    bb(s, 'News_box_lid', nx - .21, nx + .21, ny - .23, ny + .23, .92, .97, 'LaundryRedEnamel')
    bb(s, 'News_box_window_frame', nx + .2, nx + .215, ny - .19, ny + .19, .52, .86, 'Metal')
    bb(s, 'News_box_newspaper', nx + .214, nx + .218, ny - .16, ny + .16, .55, .83, 'Paper')
    bb(s, 'News_box_coin_mech', nx + .2, nx + .24, ny + .08, ny + .18, .4, .5, 'Metal')
    # payphone between door and window, louvred exhaust fan high up
    py = -1.95
    bb(s, 'Payphone_side_panel', xw + BLK_D, xw + .34, py - .3, py - .28, .9, 1.8, 'LaundryAluminum')
    bb(s, 'Payphone_side_panel', xw + BLK_D, xw + .34, py + .28, py + .3, .9, 1.8, 'LaundryAluminum')
    bb(s, 'Payphone_hood', xw + BLK_D, xw + .34, py - .3, py + .3, 1.8, 1.84, 'LaundryAluminum')
    bb(s, 'Payphone_housing', xw + BLK_D, xw + .16, py - .11, py + .11, 1.05, 1.58, 'Metal')
    bb(s, 'Payphone_handset', xw + .16, xw + .21, py - .1, py - .05, 1.18, 1.5, 'DarkMetal')
    bb(s, 'Payphone_keypad', xw + .16, xw + .165, py - .02, py + .08, 1.3, 1.42, 'DarkMetal')
    bb(s, 'Payphone_coin_return', xw + .16, xw + .175, py + .0, py + .06, 1.1, 1.15, 'DarkMetal')
    bb(s, 'Payphone_directory_shelf', xw + BLK_D, xw + .3, py - .28, py + .28, .9, .93, 'LaundryAluminum')
    if lod < 2:
        s.tube('Payphone_armored_cord', (xw + .19, py - .075, 1.18), (xw + .18, py - .02, 1.06), .008, 'Metal', 5)
        bb(s, 'Payphone_instruction_card', xw + .16, xw + .163, py + .0, py + .08, 1.45, 1.55, 'Paper')
    ly = -3.0
    bb(s, 'Exhaust_fan_box', xw + BLK_D, xw + .22, ly - .3, ly + .3, 1.92, 2.34, 'Metal')
    for k in range([6, 4, 0][lod]):
        zz = 1.97 + k * .06
        bb(s, 'Exhaust_louvre_blade', xw + .22, xw + .24, ly - .27, ly + .27, zz, zz + .025, 'DarkMetal')
    if lod == 2:
        bb(s, 'Exhaust_louvre_face', xw + .22, xw + .225, ly - .27, ly + .27, 1.96, 2.3, 'DarkMetal')
    if lod < 2:
        s.tag = 'Exhaust_soot_stain'
        wtrap(s, 'front', ly, 2.34 + .2, 2.34, .45, .62, BLK_D + .002, 'LaundryGrime')


def _interior(m, fix, lod, rng):
    xi0, xi1, yi0, yi1 = X0 + HT, X1 - HT, Y0 + HT, Y1 - HT
    # floor: cream vinyl tile with green checker, ceiling with T-bar grid
    bb(m, 'Vinyl_tile_floor', xi0, xi1, yi0, yi1, -.11, 0, 'LaundryTileCream')
    if lod < 2:
        ts = .3048 if lod == 0 else .6096
        m.tag = 'Green_checker_tile'
        nx = int((xi1 - xi0) / ts) + 1
        ny = int((yi1 - yi0) / ts) + 1
        for i in range(nx):
            for j in range(ny):
                if (i + j) % 2:
                    continue
                a0, a1 = xi1 - (i + 1) * ts, xi1 - i * ts
                b0, b1 = yi0 + j * ts, yi0 + (j + 1) * ts
                a0, b1 = max(a0, xi0), min(b1, yi1)
                if a1 - a0 > .01 and b1 - b0 > .01:
                    hface(m, [(a0, b0, .002), (a1, b0, .002), (a1, b1, .002), (a0, b1, .002)], 'LaundryTileGreen')
    bb(m, 'Suspended_ceiling', xi0, xi1, yi0, yi1, CEIL, CEIL + .05, 'Ceiling')
    m.tag = 'Ceiling_T_bar'
    tb = .022
    step_y = .6096 if lod < 2 else 1.2192
    y = yi0 + step_y
    while y < yi1 - .1:
        hface(m, [(xi0, y - tb / 2, CEIL - .004), (xi0, y + tb / 2, CEIL - .004), (xi1, y + tb / 2, CEIL - .004), (xi1, y - tb / 2, CEIL - .004)], 'Metal', up=False)
        y += step_y
    x = xi1 - 1.2192
    while x > xi0 + .1:
        hface(m, [(x - tb / 2, yi0, CEIL - .005), (x - tb / 2, yi1, CEIL - .005), (x + tb / 2, yi1, CEIL - .005), (x + tb / 2, yi0, CEIL - .005)], 'Metal', up=False)
        x -= 1.2192

    # wall lining: pale green wainscot, cap rail, cream paint above
    front_holes = [(-h[1], -h[0], h[2], h[3]) for h in [DOOR_HOLE] + WIN_HOLES]
    lining = [('front_in', -(Y1 - HT), -(Y0 + HT), front_holes), ('right_in', X0 + HT, X1 - HT, []),
              ('left_in', -(X1 - HT), -(X0 + HT), []), ('rear_in', Y0 + HT, Y1 - HT, [])]
    for w, u0, u1, hl in lining:
        m.tag = 'Interior_wall_paint'
        for a, b, za, zb in rects_minus(u0, u1, 0, CEIL, hl):
            for lo, hi, mat in [(za, min(zb, 1.2), 'LaundryPaleGreen'), (max(za, 1.2), zb, 'LaundryWallCream')]:
                if hi - lo > 1e-4:
                    wrect(m, w, a, b, lo, hi, .004, mat)
        if lod < 2:
            for a, b, za, zb in rects_minus(u0, u1, 1.18, 1.24, hl):
                wbox(m, 'Wainscot_cap_rail', w, (a + b) / 2, 1.21, .016, b - a, .05, .024, 'DarkWood')
            for a, b, za, zb in rects_minus(u0, u1, 0, .1, hl):
                wbox(m, 'Vinyl_cove_base', w, (a + b) / 2, .05, .01, b - a, .1, .012, 'Rubber')

    # washers: front loaders facing the windows, top loaders back to back
    loads = ['Bedding', None, 'BookCloth', None, None, 'Upholstery', None, 'LampFabric', None, None, 'Bedding', None, None, 'Upholstery', None]
    tpls = {}
    for i in range(15):
        ld = loads[i] if lod < 2 else None
        if ld not in tpls:
            tpls[ld] = frontloader(lod, ld)
        place(m, tpls[ld], -4.74, -5.46 + i * .78)
    tl = toploader(lod)
    for i in range(12):
        place(m, tl, -5.53, -4.29 + i * .78, math.pi)
    bb(m, 'Service_chase_cover', -5.2, -5.08, -5.85, 5.85, 0, 1.12, 'CreamEnamel')

    # dryers: five stacks on the rear wall, three on the right wall
    dl = [('Bedding', None), (None, 'Upholstery'), (None, None), ('LampFabric', None), (None, None)]
    for i, y in enumerate([1.0, 1.86, 2.72, 3.58, 4.44]):
        place(m, dryer_stack(lod, dl[i] if lod < 2 else (None, None)), X0 + HT + .4, y)
    for x in [-7.7, -6.84, -5.98]:
        place(m, dryer_stack(lod), x, Y1 - HT - .4, -math.pi / 2)
    if lod < 2:
        for y in [1.0, 1.86, 2.72, 3.58, 4.44]:
            m.tube('Dryer_exhaust_duct', (X0 + HT + .02, y, 1.9), (X0 + HT + .02, y, 2.55), .07, 'Metal', 8)

    # folding: island table, wall counter
    _fold_table(m, -2.3, 2.6, .9, 2.4, lod)
    bb(m, 'Wall_folding_counter', -4.3, -.6, Y1 - HT - .62, Y1 - HT, .87, .91, 'CreamEnamel')
    bb(m, 'Counter_edge_band', -4.3, -.6, Y1 - HT - .635, Y1 - HT - .62, .86, .91, 'LaundryAluminum')
    bb(m, 'Counter_lower_shelf', -4.25, -.65, Y1 - HT - .58, Y1 - HT, .2, .23, 'Timber')
    for x in [-4.2, -2.45, -.7]:
        bb(m, 'Counter_leg', x - .025, x + .025, Y1 - HT - .6, Y1 - HT - .55, 0, .87, 'Metal')
    if lod < 2:
        for k, x in enumerate([-3.8, -3.3, -1.6]):
            bb(m, 'Folded_laundry_stack', x - .18, x + .18, Y1 - HT - .45, Y1 - HT - .1, .91, .91 + .05 + .04 * k, ['Bedding', 'LampFabric', 'BookCloth'][k])
        m.lathe('Plastic_laundry_basket', (-2.3, 3.3, .9), [(0, .2), (.28, .25)], 'LaundryOrangePlastic', [12, 8][lod], caps=False)
        m.lathe('Basket_bottom', (-2.3, 3.3, .9), [(0, .2), (.005, .2)], 'LaundryOrangePlastic', [12, 8][lod])

    # chairs under the windows (backs to the street), carts
    ch = plastic_chair(lod)
    for y in [-.35, .25, .85, 1.45, 3.1, 3.7, 4.3]:
        place(m, ch, -.62, y, math.pi)
    ct = wire_cart(lod)
    for x, y, yaw in [(-3.2, -1.9, .2), (-2.9, 5.0, -.15), (-7.1, -2.6, 1.3)]:
        place(m, ct, x, y, yaw)

    # utility closet in the rear left corner, pale green door
    bb(m, 'Closet_partition', -8.26, -8.14, yi0, -5.0, 0, CEIL, 'LaundryWallCream')
    bb(m, 'Closet_partition', xi0, -8.14, -5.06, -4.94, 0, CEIL, 'LaundryWallCream')
    bb(m, 'Closet_door_frame', -8.14, -8.11, -6.42, -5.48, 0, 2.1, 'DarkMetal')
    bb(m, 'Closet_door_leaf', -8.11, -8.08, -6.37, -5.53, 0, 2.05, 'LaundryPaleGreen')
    m.lathe('Closet_knob', (-8.08, -5.62, 1.0), [(0, .025), (.05, .028), (.06, .01)], 'Metal', 6, axis=(1, 0, 0))
    # rear wall: steel back door seen from inside, detergent machine, shelf, board, clock, price card
    bb(m, 'Back_door_inside', X0 + HT, X0 + HT + .035, -3.65, -2.75, 0, 2.04, 'LaundryPaleGreen')
    bb(m, 'Back_door_frame_in', X0 + HT, X0 + HT + .02, -3.71, -2.69, 0, 2.1, 'DarkMetal')
    m.tube('Back_door_panic_bar', (X0 + HT + .1, -3.55, 1.0), (X0 + HT + .1, -2.85, 1.0), .02, 'Metal', 6)
    _soap_vendor(m, X0 + HT, -1.2, lod)
    bb(m, 'Detergent_shelf', X0 + HT, X0 + HT + .3, -2.55, -.25, 2.12, 2.15, 'Timber')
    if lod < 1:
        for k in range(8):
            yy = -2.4 + k * .28
            col = ['LaundryOrangePlastic', 'CreamEnamel', 'BluePaint', 'LaundryRedEnamel'][k % 4]
            m.lathe('Detergent_jug', (X0 + HT + .15, yy, 2.15), [(0, .055), (.2, .055), (.24, .025), (.28, .02)], col, 8)
    bb(m, 'Bulletin_board_frame', X0 + HT, X0 + HT + .03, -.6, .3, 1.2, 1.95, 'Timber')
    bb(m, 'Cork_board', X0 + HT + .03, X0 + HT + .035, -.56, .26, 1.24, 1.91, 'Burlap')
    if lod < 2:
        for k in range([14, 6][lod]):
            yy, zz = rng.uniform(-.48, .18), rng.uniform(1.33, 1.82)
            ww, hh = rng.uniform(.09, .16), rng.uniform(.1, .17)
            bb(m, 'Pinned_notice', X0 + HT + .035 + k * .0008, X0 + HT + .038 + k * .0008, yy - ww / 2, yy + ww / 2, zz - hh / 2, zz + hh / 2, 'Paper')
    m.panel('Laundry_price_notice', (X0 + HT + .006, 1.0, 2.38), .6, .74, 9, (1, 0, 0))
    m.lathe('Wall_clock_rim', (X0 + HT, 2.75, 2.45), [(0, .17), (.05, .17)], 'DarkMetal', [16, 12, 8][lod], axis=(1, 0, 0))
    m.lathe('Wall_clock_face', (X0 + HT + .051, 2.75, 2.45), [(0, .15), (.002, .15)], 'Paper', [16, 12, 8][lod], axis=(1, 0, 0))
    bb(m, 'Clock_hour_hand', X0 + HT + .054, X0 + HT + .058, 2.74, 2.76, 2.45, 2.54, 'DarkMetal')
    bb(m, 'Clock_minute_hand', X0 + HT + .058, X0 + HT + .062, 2.75, 2.87, 2.44, 2.46, 'DarkMetal')
    # left wall: change machine, notice, trash can
    _change_machine(m, -1.75, lod)
    m.panel('Rules_notice', (-2.6, Y0 + HT + .006, 1.6), .3, .37, 9, (0, 1, 0))
    m.lathe('Grey_trash_can', (-1.25, -2.7, 0), [(0, .2), (.68, .23), (.7, .24)], 'Metal', [12, 8, 6][lod], caps=True)
    m.lathe('Trash_liner_lip', (-1.25, -2.7, .7), [(0, .245), (.03, .245)], 'Rubber', [12, 8, 6][lod], caps=False)
    bb(m, 'Entry_rubber_mat', -1.3, X1 - HT - .01, DOOR_HOLE[0] + .05, DOOR_HOLE[1] - .05, 0, .012, 'Rubber')
    bb(m, 'Corner_trash_drum', -8.9, -8.3, 5.6, 6.2, 0, .02, 'Rubber')
    m.lathe('Corner_trash_drum', (-8.6, 5.9, 0), [(0, .28), (.85, .28)], 'OliveEnamel', [12, 8, 6][lod])
    # small CRT television on a ceiling bracket in the front right corner
    if lod < 2:
        tvx, tvy = -.75, 6.3
        bb(m, 'TV_bracket_arm', tvx - .02, tvx + .02, tvy - .02, tvy + .02, 2.6, CEIL, 'DarkMetal')
        bb(m, 'TV_shelf', tvx - .25, tvx + .25, tvy - .22, tvy + .22, 2.18, 2.21, 'DarkMetal')
        bb(m, 'CRT_cabinet', tvx - .22, tvx + .2, tvy - .21, tvy + .21, 2.21, 2.58, 'DarkWood')
        bb(m, 'CRT_screen', tvx - .225, tvx - .22, tvy - .15, tvy + .1, 2.27, 2.52, 'WasherGlass')

    # fluorescent wraparound fixtures: three rows over the aisles
    for x in [-1.6, -4.7, -7.7]:
        for y in [-4.8, -1.6, 1.6, 4.8]:
            bb(fix, 'Fluorescent_housing', x - .16, x + .16, y - .63, y + .63, CEIL - .05, CEIL, 'Metal')
            bb(fix, 'Wraparound_acrylic_lens', x - .14, x + .14, y - .6, y + .6, CEIL - .1, CEIL - .05, 'LaundryTubeLens')
            if lod < 1:
                for yy in [y - .63, y + .63]:
                    bb(fix, 'Fixture_end_cap', x - .15, x + .15, yy - .01, yy + .01, CEIL - .1, CEIL - .02, 'Metal')


def _fold_table(m, x, y, sx, sy, lod):
    bb(m, 'Folding_table_top', x - sx / 2, x + sx / 2, y - sy / 2, y + sy / 2, .87, .91, 'CreamEnamel')
    bb(m, 'Folding_table_edge', x - sx / 2 - .01, x + sx / 2 + .01, y - sy / 2 - .01, y + sy / 2 + .01, .865, .885, 'LaundryAluminum')
    sides = [6, 5, 4][lod]
    for dx in [-sx / 2 + .08, sx / 2 - .08]:
        for dy in [-sy / 2 + .1, sy / 2 - .1]:
            m.tube('Table_leg', (x + dx, y + dy, 0), (x + dx, y + dy, .865), .016, 'Metal', sides)
    bb(m, 'Table_lower_shelf', x - sx / 2 + .06, x + sx / 2 - .06, y - sy / 2 + .08, y + sy / 2 - .08, .18, .2, 'Timber')


def _soap_vendor(m, xw, y, lod):
    """Detergent box vending machine against the rear wall."""
    xf = xw + .5
    bb(m, 'Vendor_cabinet', xw, xf, y - .36, y + .36, 0, 1.75, 'LaundryRedEnamel')
    bb(m, 'Vendor_window', xf, xf + .005, y - .27, y + .27, .85, 1.55, 'WasherGlass')
    bb(m, 'Vendor_window_frame_top', xf, xf + .02, y - .3, y + .3, 1.55, 1.6, 'LaundryAluminum')
    bb(m, 'Vendor_window_frame_bottom', xf, xf + .02, y - .3, y + .3, .8, .85, 'LaundryAluminum')
    cols = ['BluePaint', 'LaundryOrangePlastic', 'Paper', 'TealPaint']
    if lod < 2:
        for r, zz in enumerate([.9, 1.13, 1.36]):
            for c in range(4):
                yy = y - .2 + c * .133
                bb(m, 'Detergent_box_display', xf - .1, xf - .02, yy - .05, yy + .05, zz, zz + .15, cols[(c + r) % 4])
    for c in range(4):
        yy = y - .2 + c * .133
        bb(m, 'Vendor_pull_knob', xf, xf + .05, yy - .025, yy + .025, .66, .7, 'Metal')
    bb(m, 'Vendor_delivery_bin', xf, xf + .03, y - .25, y + .25, .2, .4, 'DarkMetal')
    bb(m, 'Vendor_coin_plate', xf, xf + .015, y + .2, y + .32, .95, 1.25, 'LaundryAluminum')


def _change_machine(m, x, lod):
    """Wall hung bill changer on the left wall, coin cup below."""
    yw = Y0 + HT
    bb(m, 'Changer_cabinet', x - .23, x + .23, yw, yw + .3, 1.05, 1.75, 'LaundryAluminum')
    bb(m, 'Changer_front_panel', x - .2, x + .2, yw + .3, yw + .31, 1.1, 1.7, 'BluePaint')
    bb(m, 'Bill_acceptor_slot', x - .08, x + .08, yw + .31, yw + .33, 1.45, 1.5, 'DarkMetal')
    bb(m, 'Changer_coin_cup', x - .08, x + .08, yw + .26, yw + .36, 1.1, 1.18, 'Metal')
    bb(m, 'Changer_label', x - .15, x + .15, yw + .31, yw + .315, 1.56, 1.66, 'Paper')
