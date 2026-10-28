"""TB2 Plaza cinema, Caliche. Original small-town Pueblo Deco picture house, faded 1986.

build(lod) -> {SM_Town_Cinema_Shell, _Glass, _Interior, _Fixtures}. Metres, Z up, TB1 local frame:
front wall on x=0 (outer face x=.11, facing +x), footprint x -14..0, y -10..10, main walls 6.2 m.
Anchors (door, windows, footprint, height) come unchanged from design.BUILDINGS['Cinema'].
The door and both door-height openings stay closed glazed doors; no new player admission.
Below 2.3 m nothing projects more than .6 m from a wall (booth .51, poster cases .27, apron 2 cm).
"""
import math, sys
from pathlib import Path
TB = Path(__file__).resolve().parent.parent
if str(TB) not in sys.path:
    sys.path.insert(0, str(TB))
from geometry import Mesh, box_bounds, front_wall, pane, add, sub, mul, cross, unit, dot
from design import BUILDINGS

B = BUILDINGS['Cinema']
X0, X1, Y0, Y1 = B['footprint']          # -14, 0, -10, 10
H = B['height']                          # 6.2
DOOR_Y, DOOR_W, DOOR_H = B['door']
WO = .11                                 # outer face of front wall
WI = -.19                                # inner face of front wall (.30 thick masonry)
SIDE_T = .22

# Marquee and blade sign
MQ_X = 2.45; MQ_Y = 4.0; MQ_ZB = 2.95; MQ_ZT = 4.25
BOOTH_Y = -.8; BOOTH_HW = .75; BOOTH_RX = .72; BOOTH_CX = -.10   # front at x=.62 (.51 proud)


# ---------------------------------------------------------------- helpers
def newell(ps):
    n = [0, 0, 0]
    for i, p in enumerate(ps):
        q = ps[(i + 1) % len(ps)]
        n[0] += (p[1] - q[1]) * (p[2] + q[2]); n[1] += (p[2] - q[2]) * (p[0] + q[0]); n[2] += (p[0] - q[0]) * (p[1] + q[1])
    return tuple(n)


def extrude(m, name, ps, d, mat, caps=(True, True), smooth=False):
    """Prism from a planar convex polygon ps, extruded by vector d. Outward winding."""
    m.tag = name
    ps = list(ps)
    if dot(newell(ps), d) > 0:
        ps = ps[::-1]
    top = [add(p, d) for p in ps]
    if caps[0]: m.face(ps, mat)
    if caps[1]: m.face(top[::-1], mat)
    for i in range(len(ps)):
        j = (i + 1) % len(ps)
        m.face([ps[i], top[i], top[j], ps[j]], mat, smooth=smooth)


def obox(m, name, c, ax, ay, size, mat, back=True):
    """Oriented box. ax, ay unit and orthogonal; az = ax x ay. back=False drops the -az face."""
    m.tag = name; az = cross(ax, ay); hx, hy, hz = [s / 2 for s in size]
    P = lambda i, j, k: add(c, add(mul(ax, i * hx), add(mul(ay, j * hy), mul(az, k * hz))))
    v = [P(-1, -1, -1), P(1, -1, -1), P(1, 1, -1), P(-1, 1, -1), P(-1, -1, 1), P(1, -1, 1), P(1, 1, 1), P(-1, 1, 1)]
    for k, ids in enumerate([(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]):
        if k == 0 and not back:
            continue
        m.face([v[i] for i in ids], mat)


def bb(m, name, x0, x1, y0, y1, z0, z1, mat):
    box_bounds(m, name, min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1), min(z0, z1), max(z0, z1), mat)


def quad_xy(m, name, x0, x1, y0, y1, z, mat):
    m.tag = name; m.face([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], mat)


def frame_of(normal):
    n = unit(normal); r = unit(cross(mul(n, -1), (0, 0, 1))); u = cross(n, r)
    return n, r, u


GLYPHS = {
    'P': [[(0, 0), (0, 1), (.62, 1), (.8, .86), (.8, .64), (.62, .5), (0, .5)]],
    'L': [[(0, 1), (0, 0), (.75, 0)]],
    'A': [[(0, 0), (.4, 1), (.8, 0)], [(.17, .42), (.63, .42)]],
    'Z': [[(0, 1), (.8, 1), (0, 0), (.8, 0)]],
    'C': [[(.8, .82), (.64, 1), (.16, 1), (0, .82), (0, .18), (.16, 0), (.64, 0), (.8, .18)]],
    'O': [[(.16, 0), (.64, 0), (.8, .18), (.8, .82), (.64, 1), (.16, 1), (0, .82), (0, .18), (.16, 0)]],
    'S': [[(.8, .84), (.64, 1), (.16, 1), (0, .84), (0, .66), (.16, .5), (.64, .5), (.8, .34), (.8, .16), (.64, 0), (.16, 0), (0, .16)]],
    'E': [[(.78, 1), (0, 1), (0, 0), (.78, 0)], [(0, .5), (.6, .5)]],
    'D': [[(0, 0), (0, 1), (.55, 1), (.8, .75), (.8, .25), (.55, 0), (0, 0)]],
}


def glyph_segments(ch, origin, r, u, h):
    for line in GLYPHS[ch]:
        pts = [add(origin, add(mul(r, x * h), mul(u, y * h))) for x, y in line]
        for a, b in zip(pts, pts[1:]):
            yield a, b


def stroke_text(m, name, text, center, normal, h, sw, depth, mat, adv=.98, flat=False):
    """Original block lettering as physical strokes standing out of a surface along normal."""
    n, r, u = frame_of(normal); total = (len(text) - 1) * adv * h + .8 * h; seg = 0
    for k, ch in enumerate(text):
        if ch == ' ':
            continue
        org = add(center, add(mul(r, -total / 2 + k * adv * h), mul(u, -h / 2)))
        for a, b in glyph_segments(ch, org, r, u, h):
            d = sub(b, a); L = math.sqrt(dot(d, d)); ax = mul(d, 1 / L); ay = cross(n, ax)
            seg += 1; dd = depth + .0015 * (seg % 3)
            c = add(mul(add(a, b), .5), mul(n, dd / 2))
            if flat:
                m.tag = name; hx, hy = mul(ax, (L + sw) / 2), mul(ay, sw / 2); c = add(c, mul(n, depth / 2))
                m.face([add(c, add(mul(hx, i), mul(hy, j))) for i, j in [(-1, -1), (1, -1), (1, 1), (-1, 1)]], mat)
            else:
                obox(m, name, c, ax, ay, (L + sw, sw, dd), mat, back=False)


def neon_glyph(m, ch, origin, normal, h, off, rad, nseg, mat='CinemaNeonRed'):
    n, r, u = frame_of(normal); org = add(origin, mul(n, off))
    for a, b in glyph_segments(ch, org, r, u, h):
        m.tube('Exposed_red_neon_letter_tube', a, b, rad, mat, nseg, caps=True)


def bulb(f, at, axis, n, lamp='CinemaBulb', dead=False):
    lamp = 'CinemaDeadBulb' if dead else lamp
    """Socket ring plus round incandescent lamp, pointing along axis."""
    f.lathe('Marquee_bulb_socket', at, [(0, .024), (.028, .024)], 'Metal', n, axis=axis, caps=False)
    f.lathe('Marquee_incandescent_bulb', at, [(.028, .02), (.05, .034), (.085, .009)], lamp, n, axis=axis, caps=False)


def ellipse_arc(cx, cy, rx, ry, n, t0=-math.pi / 2, t1=math.pi / 2):
    return [(cx + rx * math.cos(t0 + (t1 - t0) * k / n), cy + ry * math.sin(t0 + (t1 - t0) * k / n)) for k in range(n + 1)]


# ---------------------------------------------------------------- shell: walls, parapets, roof
def walls_and_roof(s, L):
    holes = [(DOOR_Y - DOOR_W / 2, DOOR_Y + DOOR_W / 2, 0, DOOR_H)]
    holes += [(y - w / 2, y + w / 2, z0, z1) for y, w, z0, z1 in B['windows']]
    holes += [(BOOTH_Y - BOOTH_HW, BOOTH_Y + BOOTH_HW, 0, 2.45)]          # ticket booth recess
    yi = Y1 - SIDE_T / 2                                                     # 9.89, inside of side walls
    front_wall(s, (WO + WI) / 2, -yi, yi, H, holes, 'CinemaStucco', WO - WI)
    # Front parapet: centre section higher than the wings (real setbacks, solid masonry).
    bb(s, 'Front_centre_parapet', WI, WO, -4.15, 4.15, H, 6.95, 'CinemaStucco')
    for a in [-1, 1]:
        bb(s, 'Front_wing_parapet', WI, WO, a * 5.05, a * yi, H, 6.75, 'CinemaStucco')
        bb(s, 'Front_wing_parapet_coping', WI - .04, WO + .05, a * 5.05, a * (Y1 + .11), 6.75, 6.81, 'Concrete')
    bb(s, 'Front_centre_coping', WI - .04, WO + .05, -4.15, 4.15, 6.95, 7.01, 'Concrete')
    # Side walls (full length, outer face at y=+-10.11), false-front return near the street.
    for a in [-1, 1]:
        y0, y1 = a * (Y1 - SIDE_T / 2), a * (Y1 + SIDE_T / 2)
        bb(s, 'Side_stucco_wall', X0 - .11, WO, y0, y1, 0, 6.55, 'CinemaStucco')
        bb(s, 'Side_false_front_return', -1.05, WO, y0, y1, 6.55, 6.75, 'CinemaStucco')
        bb(s, 'Side_parapet_flashing', X0 - .15, -1.05, y0 - .03, y1 + .03, 6.55, 6.59, 'Metal')
        bb(s, 'Side_base_splash_band', X0 - .11, WO, y1, y1 + a * .03, 0, .45, 'Concrete')
    for a in [-1, 1]:
        y1 = a * (Y1 + SIDE_T / 2)
        for x in [-4.4, -7.4, -10.4, -12.9]:
            bb(s, 'Side_wall_buttress_pilaster', x - .25, x + .25, y1, y1 + a * .12, 0, 6.45, 'CinemaStucco')
            bb(s, 'Pilaster_weathered_cap', x - .29, x + .29, y1, y1 + a * .16, 6.45, 6.52, 'Concrete')
    bb(s, 'Rear_stucco_wall', X0 - .11, X0 + .11, -yi, yi, 0, 6.55, 'CinemaStucco')
    bb(s, 'Rear_parapet_flashing', X0 - .15, X0 + .15, -yi, yi, 6.55, 6.59, 'Metal')
    bb(s, 'Deep_foundation', X0 - .17, WO + .06, -Y1 - .17, Y1 + .17, -.7, 0, 'Concrete')
    bb(s, 'Built_up_tar_roof', X0 + .11, WI, -yi, yi, H - .12, H, 'CinemaRoofTar')
    # Interior face of the street front above the lobby ceiling is hidden; lobby ceiling lives in Interior.
    # Base course across the front: dark glazed tile under the marquee, splash-darkened plaster on the wings.
    for y0, y1, mat in [(-4.15, -3.25, 'CinemaBlackTile'), (-1.95, -1.55, 'CinemaBlackTile'), (-.05, .35, 'CinemaBlackTile'),
                        (1.65, 1.95, 'CinemaBlackTile'), (3.25, 4.15, 'CinemaBlackTile'),
                        (-yi - .11, -5.05, 'Concrete'), (5.05, yi + .11, 'Concrete')]:
        bb(s, 'Front_base_course', WO, WO + .035, y0, y1, 0, .5 if mat == 'Concrete' else .62, mat)
        if mat != 'Concrete':
            bb(s, 'Base_tile_bullnose_cap', WO, WO + .05, y0, y1, .62, .66, 'CinemaBlackTile')


def facade_deco(s, L):
    n = L['n']
    # Outer stepped pylons, full height, framing the marquee bay.
    for a in [-1, 1]:
        yc = a * 4.6
        for z0, z1, w, dx in [(0, 6.95, .9, .20), (6.95, 7.25, .74, .17), (7.25, 7.5, .54, .14), (7.5, 7.68, .32, .11)]:
            bb(s, 'Stepped_Deco_pylon', WO - .3 if z0 > 6 else WO, WO + dx, yc - w / 2, yc + w / 2, z0, z1, 'CinemaStucco')
        for dy in [-.2, .2]:
            bb(s, 'Pylon_terracotta_flute', WO + .20, WO + .245, yc + dy - .065, yc + dy + .065, .66, 6.85, 'CinemaFinRed')
            s.rounded_box('Pylon_terracotta_flute_round', (WO + .2, yc + dy, 3.755), (.09, .13, 6.19), .04, 'CinemaFinRed', 3)
        bb(s, 'Pylon_base_block', WO, WO + .25, yc - .47, yc + .47, 0, .66, 'CinemaBlackTile')
        # Cast ziggurat relief at the pylon head.
        for k, (w, zz) in enumerate([(.62, 6.1), (.44, 6.28), (.26, 6.46)]):
            bb(s, 'Cast_ziggurat_relief', WO + .20, WO + .24 + .02 * k, yc - w / 2, yc + w / 2, zz, zz + .18, 'CinemaFinRed')
    # Central tower behind the blade sign, rising from the marquee roof.
    for z0, z1, w, dx in [(MQ_ZT, 7.6, 1.5, .20), (7.6, 7.9, 1.2, .17), (7.9, 8.2, .86, .14), (8.2, 8.36, .5, .11)]:
        bb(s, 'Central_stepped_tower', WO - .3 if z0 > 6.9 else WO, WO + dx, -w / 2, w / 2, z0, z1, 'CinemaStucco')
    bb(s, 'Central_tower_back', WI - .5, WI, -.75, .75, H, 7.6, 'CinemaStucco')
    # Three terracotta fins either side of the tower, tops stepping down outward.
    for a in [-1, 1]:
        for k, (yy, top) in enumerate([(1.0, 7.75), (1.27, 7.5), (1.54, 7.25)]):
            y = a * yy
            bb(s, 'Terracotta_Deco_fin', WO, WO + .15, y - .06, y + .06, MQ_ZT, top, 'CinemaFinRed')
            s.rounded_box('Terracotta_Deco_fin_round', (WO + .06, y, (MQ_ZT + top) / 2), (.18, .12, top - MQ_ZT), .045, 'CinemaFinRed', 3)
            bb(s, 'Fin_stepped_cap', WO - .02, WO + .19, y - .08, y + .08, top, top + .07, 'CinemaFinRed')
            if top > 6.95:
                bb(s, 'Fin_parapet_back', WI, WO, y - .06, y + .06, 6.95, top, 'CinemaFinRed')
        # Pueblo Deco stepped diamond medallion, original, three nested cast layers.
        if L['ornament']:
            c = (WO, a * 2.85, 5.55)
            ax = (0, math.sqrt(.5), math.sqrt(.5)); ay = (0, -math.sqrt(.5), math.sqrt(.5))   # az = +x
            for k, (sz, mat) in enumerate([(.62, 'CinemaFinRed'), (.42, 'TealPaint'), (.2, 'Ceramic')]):
                dep = .03 + .025 * k
                obox(s, 'Cast_stepped_diamond_medallion', (WO + dep / 2, c[1], c[2]), ax, ay, (sz, sz, dep), mat, back=False)
            # small stepped keystones above and below
            for dz in [-.52, .52]:
                bb(s, 'Medallion_keystone', WO, WO + .04, c[1] - .07, c[1] + .07, c[2] + dz - .07, c[2] + dz + .07, 'CinemaFinRed')
        # Speed-line bands on the wings under the coping, and a moulded cornice line.
        for z in ([5.95, 6.12, 6.29] if L['ornament'] else [6.12]):
            bb(s, 'Wing_speed_line_relief', WO, WO + .035, a * 5.05, a * (Y1 + .11), z, z + .06, 'CinemaStucco')
        bb(s, 'Bay_cornice_band', WO, WO + .06, a * 1.66, a * 4.15, 6.6, 6.7, 'CinemaStucco')
        if L['ornament']:
            # Cast zigzag (stepped chevron) frieze under the bay cornice.
            nz = 10; ya, yb = 1.7, 4.1
            for k in range(nz):
                y0 = a * (ya + k * (yb - ya) / nz); y1 = a * (ya + (k + 1) * (yb - ya) / nz); ym = (y0 + y1) / 2
                extrude(s, 'Cast_zigzag_frieze', [(WO, y0, 6.32), (WO, y1, 6.32), (WO, ym, 6.56)], (.035, 0, 0), 'CinemaFinRed', caps=(False, True))
    # Weathering: rain streaks under the copings and a patched repair, very restrained.
    if L['ornament']:
        for y, w, ln in [(-8.6, .5, .9), (6.6, .7, 1.2), (-3.4, .4, .7)]:
            zt = (6.75 if abs(y) > 5 else 6.6) - .02; xs = WO + .006
            s.tag = 'Coping_rain_stain_tapered'
            s.face([(xs, y - w * .3, zt - ln), (xs, y + w * .25, zt - ln), (xs, y + w / 2, zt), (xs, y - w / 2, zt)], 'CinemaStain')
        for y, w, ln in [(-7.0, .6, .5), (8.6, .45, .8)]:
            zt = 6.73; xs = WO + .005
            s.tag = 'Fine_rain_stain'
            s.face([(xs, y - w * .35, zt - ln), (xs, y + w * .3, zt - ln), (xs, y + w / 2, zt), (xs, y - w / 2, zt)], 'CinemaStain')
        for a in [-1, 1]:
            s.tube('Coping_drip_edge', (WO + .045, a * 5.05, 6.745), (WO + .045, a * (Y1 + .11), 6.745), .012, 'Concrete', 5, caps=False)
        s.tube('Coping_drip_edge', (WO + .045, -4.15, 6.945), (WO + .045, 4.15, 6.945), .012, 'Concrete', 5, caps=False)
        for y, z, w, h in [(-8.0, 1.5, .9, .55), (8.3, 3.3, .6, .8)]:
            bb(s, 'Stucco_patch_repair', WO, WO + .008, y - w / 2, y + w / 2, z, z + h, 'CinemaStain')


def entry_doors(s, g, f, L):
    """Three aluminium double-door bays in the unchanged openings. Fixed closed."""
    x = -.05
    for yc, w, z0, zh in [(DOOR_Y, DOOR_W, 0, DOOR_H)] + [(y, w, z0, z1) for y, w, z0, z1 in B['windows']]:
        z0 = 0; fw = .055
        for a in [-1, 1]:
            bb(s, 'Aluminium_door_jamb', x - .06, x + .06, yc + a * w / 2, yc + a * (w / 2 - fw), .018, zh - .07, 'CinemaBronze')
        bb(s, 'Aluminium_door_head', x - .06, x + .06, yc - w / 2, yc + w / 2, zh - .07, zh, 'CinemaBronze')
        bb(s, 'Brass_threshold', x - .1, WO + .02, yc - w / 2, yc + w / 2, 0, .018, 'CinemaBrass')
        iy0, iy1 = yc - w / 2 + fw, yc + w / 2 - fw; lw = (iy1 - iy0 - .006) / 2
        for k in [0, 1]:
            ly0 = iy0 + k * (lw + .006); ly1 = ly0 + lw; lc = (ly0 + ly1) / 2; t = .045
            st, tr, br = .075, .10, .24
            bb(s, 'Door_leaf_stile', x - t / 2, x + t / 2, ly0, ly0 + st, .02, zh - .07, 'CinemaBronze')
            bb(s, 'Door_leaf_stile', x - t / 2, x + t / 2, ly1 - st, ly1, .02, zh - .07, 'CinemaBronze')
            bb(s, 'Door_leaf_top_rail', x - t / 2, x + t / 2, ly0 + st, ly1 - st, zh - .07 - tr, zh - .07, 'CinemaBronze')
            bb(s, 'Door_leaf_bottom_rail', x - t / 2, x + t / 2, ly0 + st, ly1 - st, .02, .02 + br, 'CinemaBronze')
            bb(s, 'Brass_kick_plate', x + t / 2, x + t / 2 + .004, ly0 + .02, ly1 - .02, .03, .22, 'CinemaBrass')
            gz0, gz1 = .02 + br, zh - .07 - tr
            for zz in [.35, 1.1, 1.95]:
                hy = ly0 + .01 if k == 0 else ly1 - .01
                s.tube('Door_butt_hinge_knuckle', (x + t / 2 + .006, hy, zz - .06), (x + t / 2 + .006, hy, zz + .06), .009, 'CinemaBronze', 6)
            s.lathe('Door_lock_cylinder', (x + t / 2, ly1 - .035 if k == 0 else ly0 + .035, .9), [(0, .016), (.012, .016)], 'CinemaBrass', 8, axis=(1, 0, 0))
            pane(g, x, lc, (gz0 + gz1) / 2, ly1 - ly0 - 2 * st, gz1 - gz0)
            bb(s, 'Glazing_bead', x - .012, x + .012, ly0 + st, ly1 - st, gz0, gz0 + .02, 'CinemaBronze')
            # Push bar on the street side with two cast standoffs.
            px = x + t / 2 + .075; ya, yb = ly0 + .07, ly1 - .07
            f.tube('Aluminium_push_bar', (px, ya, 1.02), (px, yb, 1.02), .017, 'CinemaBronze', L['n'] // 2 + 2)
            for yy in [ya + .05, yb - .05]:
                f.tube('Push_bar_standoff', (x + t / 2, yy, 1.02), (px, yy, 1.02), .012, 'CinemaBronze', 6)
        # Bronze transom grille in the wall band between door head and marquee soffit.
        bb(s, 'Transom_grille_frame', WO, WO + .03, yc - w / 2 - .05, yc + w / 2 + .05, zh + .1, zh + .58, 'CinemaBronze')
        for k in range(L['grille']):
            yy = yc - w / 2 + (k + .5) * w / L['grille']
            bb(s, 'Transom_grille_bar', WO + .03, WO + .05, yy - .018, yy + .018, zh + .14, zh + .54, 'CinemaBronze')
        bb(s, 'Transom_grille_infill', WO + .031, WO + .033, yc - w / 2, yc + w / 2, zh + .14, zh + .54, 'DarkMetal')
        # Faded taped notice on one leaf of the middle bay only.
        if abs(yc - 1.0) < .01:
            s.panel('Taped_closed_card', (x + .03, iy0 + lw / 2, 1.45), .34, .19, 15)


def ticket_booth(s, g, i, f, L):
    n = L['booth']; cy = BOOTH_Y
    arc = ellipse_arc(BOOTH_CX, cy, BOOTH_RX, BOOTH_HW, n)
    def ring(rx, ry, z):
        return [(p[0], p[1], z) for p in ellipse_arc(BOOTH_CX, cy, rx, ry, n)]
    # Black glazed tile base, brass-edged walnut counter, curved glass, stepped crown.
    extrude(s, 'Booth_black_tile_base', ring(BOOTH_RX - .02, BOOTH_HW, .0), (0, 0, .98), 'CinemaBlackTile', smooth=False)
    extrude(s, 'Booth_walnut_counter', ring(BOOTH_RX + .03, BOOTH_HW, .98), (0, 0, .06), 'DarkWood')
    f.tag = 'Booth_brass_counter_nosing'
    pts = ring(BOOTH_RX + .035, BOOTH_HW, 1.0)
    for a, b in zip(pts, pts[1:]):
        f.tube('Booth_brass_counter_nosing', a, b, .016, 'CinemaBrass', 6)
    gl = ring(BOOTH_RX - .06, BOOTH_HW - .02, 1.04)
    g.tag = 'Booth_curved_glass'
    for a, b in zip(gl, gl[1:]):
        g.face([b, a, (a[0], a[1], 2.12), (b[0], b[1], 2.12)], 'Glass', smooth=True)
    for k in ([n // 4, n // 2, 3 * n // 4] if n >= 8 else [n // 2]):
        p = gl[k]
        f.tube('Booth_brass_mullion', (p[0], p[1], 1.04), (p[0], p[1], 2.12), .014, 'CinemaBrass', 6)
    extrude(s, 'Booth_header', ring(BOOTH_RX - .02, BOOTH_HW, 2.12), (0, 0, .2), 'DarkWood')
    extrude(s, 'Booth_header_brass_band', ring(BOOTH_RX, BOOTH_HW, 2.32), (0, 0, .05), 'CinemaBrass')
    for k, (rx, z0, z1) in enumerate([(.62, 2.37, 2.47), (.46, 2.47, 2.57), (.28, 2.57, 2.65)]):
        extrude(s, 'Booth_stepped_crown', ring(rx, BOOTH_HW - .05 * (k + 1), z0), (0, 0, z1 - z0), 'CinemaFinRed')
    # Closed roller shutter behind the glass and the interior counter slab.
    bb(s, 'Booth_inner_counter', -.19, .5, cy - .62, cy + .62, 1.0, 1.04, 'DarkWood')
    bb(s, 'Booth_shutter_box', .05, .18, cy - .66, cy + .66, 2.02, 2.12, 'Metal')
    for k in range(L['slats']):
        z = 1.06 + (k + .5) * .96 / L['slats']
        bb(s, 'Booth_lowered_shutter_slat', .11, .135, cy - .64, cy + .64, z - .96 / L['slats'] * .42, z + .96 / L['slats'] * .42, 'Metal')
    for a in [-1, 1]:
        bb(s, 'Booth_shutter_guide', .1, .15, cy + a * .66 - .02, cy + a * .66 + .02, 1.04, 2.12, 'DarkMetal')
    s.panel('Booth_CLOSED_card', (.27, cy + .25, 1.22), .36, .2, 15)
    # Speaking grille on the glass, money trough in the counter.
    gx = BOOTH_CX + BOOTH_RX - .06
    f.lathe('Booth_speaking_grille_disc', (gx - .004, cy, 1.55), [(0, .085), (.012, .085)], 'CinemaAluminium', L['n'], axis=(1, 0, 0))
    f.torus('Booth_grille_brass_ring', (gx + .01, cy, 1.55), .09, .012, 'CinemaBrass', L['n'], 4, axis=(1, 0, 0))
    if L['ornament']:
        for k in range(5):
            f.box('Booth_grille_slot', (gx + .01, cy, 1.51 + k * .02), (.006, .12, .007), 'DarkMetal')
    bb(f, 'Booth_brass_money_trough', gx - .14, gx + .02, cy - .13, cy + .13, 1.035, 1.06, 'CinemaBrass')
    # Lobby side of the booth: panelled walnut box with a small door.
    bx0 = -1.3
    bb(i, 'Booth_back_walnut_wall', bx0, bx0 + .06, cy - BOOTH_HW, cy + BOOTH_HW, 0, 2.6, 'DarkWood')
    for a in [-1, 1]:
        bb(i, 'Booth_side_walnut_wall', bx0, WI, cy + a * BOOTH_HW - .03, cy + a * BOOTH_HW + .03, 0, 2.6, 'DarkWood')
    bb(i, 'Booth_lid', bx0, WI, cy - BOOTH_HW - .03, cy + BOOTH_HW + .03, 2.6, 2.66, 'DarkWood')
    bb(i, 'Booth_back_door', bx0 - .02, bx0, cy - .38, cy + .38, .05, 2.0, 'Timber')
    bb(i, 'Booth_black_tile_skirting', bx0 - .015, bx0, cy - BOOTH_HW, cy + BOOTH_HW, 0, .25, 'CinemaBlackTile')
    f.lathe('Booth_door_knob', (bx0 - .02, cy + .28, 1.0), [(0, .025), (.04, .025)], 'CinemaBrass', 8, axis=(-1, 0, 0))


def poster_cases(s, g, f, L):
    for y, cell in [(-7.0, 11), (7.0, 12)]:
        w, z0, z1 = 1.3, .62, 2.4; x = WO
        bb(s, 'Poster_case_back', x, x + .05, y - w / 2, y + w / 2, z0, z1, 'DarkWood')
        s.panel('Original_fictional_film_poster', (x + .052, y, (z0 + z1) / 2), w - .17, z1 - z0 - .17, cell)
        for a in [-1, 1]:
            bb(s, 'Poster_case_aluminium_stile', x, x + .17, y + a * w / 2 - .07, y + a * w / 2, z0, z1, 'CinemaAluminium')
        bb(s, 'Poster_case_bottom_rail', x + .002, x + .168, y - w / 2 + .07, y + w / 2 - .07, z0, z0 + .07, 'CinemaAluminium')
        bb(s, 'Poster_case_top_rail', x + .002, x + .168, y - w / 2 + .07, y + w / 2 - .07, z1 - .07, z1, 'CinemaAluminium')
        pane(g, x + .14, y, (z0 + z1) / 2, w - .14, z1 - z0 - .14)
        bb(s, 'Poster_case_sill', x, x + .21, y - w / 2 - .06, y + w / 2 + .06, z0 - .07, z0, 'Concrete')
        for k, (ww, h, dx) in enumerate([(w + .1, .1, .2), (w - .3, .1, .16), (w - .7, .09, .12)]):
            zz = z1 + k * .1
            bb(s, 'Poster_case_stepped_crown', x, x + dx, y - ww / 2, y + ww / 2, zz, zz + h, 'CinemaFinRed')
        # Gooseneck lamp washing the poster case.
        zl = 3.05; arm = [(x, y, zl), (x + .14, y, zl + .08), (x + .3, y, zl + .06), (x + .4, y, zl - .04)]
        f.lathe('Gooseneck_wall_plate', (x, y, zl), [(0, .07), (.025, .07)], 'DarkMetal', L['n'], axis=(1, 0, 0))
        for a, b in zip(arm, arm[1:]):
            f.tube('Gooseneck_arm', a, b, .016, 'DarkMetal', 6)
        f.lathe('Gooseneck_enamel_shade', (x + .42, y, zl - .02), [(0, .03), (.05, .06), (.13, .17)], 'DarkMetal', L['n'], axis=(0, 0, -1), caps=False)
        f.lathe('Gooseneck_lamp', (x + .42, y, zl - .07), [(0, .03), (.05, .045), (.08, .01)], 'CinemaBulb', L['n'] // 2 + 2, axis=(0, 0, -1), caps=False)


def marquee(s, f, L):
    n = L['n']; X, Y, ZB, ZT = MQ_X, MQ_Y, MQ_ZB, MQ_ZT
    # Main sheet-metal body (recessed behind the board frame) and the soffit.
    bb(s, 'Marquee_body', WO, X - .06, -Y + .04, Y - .04, ZB + .05, ZT - .2, 'CreamEnamel')
    bb(s, 'Marquee_soffit_panel', WO, X + .02, -Y - .02, Y + .02, ZB, ZB + .05, 'CreamEnamel')
    for xx in [.55, 1.15, 1.75]:
        bb(s, 'Soffit_panel_seam', xx - .015, xx + .015, -Y, Y, ZB - .012, ZB, 'CinemaAluminium')
    # Lower bulb fascia (front and returns) in faded oxblood enamel.
    bb(s, 'Front_bulb_fascia', X - .06, X + .03, -Y - .02, Y + .02, ZB, 3.24, 'CinemaEnamelRed')
    for a in [-1, 1]:
        bb(s, 'Side_bulb_fascia', WO, X - .06, a * (Y - .06), a * (Y + .03), ZB, 3.24, 'CinemaEnamelRed')
    # Board frames: front and both sides, aluminium rails, emissive milk boards, letter tracks.
    by0, by1 = -Y + .2, Y - .2; bz0, bz1 = 3.3, 3.98
    bb(s, 'Board_frame_bottom', X - .06, X + .02, -Y, Y, 3.24, bz0, 'CinemaAluminium')
    bb(s, 'Board_frame_top', X - .06, X + .02, -Y, Y, bz1, 4.05, 'CinemaAluminium')
    for a in [-1, 1]:
        bb(s, 'Board_corner_pilaster', X - .06, X + .05, a * (Y - .2), a * (Y + .02), 3.24, 4.05, 'CinemaEnamelRed')
        bb(s, 'Board_frame_side_bottom', WO + .25, X - .06, a * (Y - .06), a * (Y + .02), 3.24, bz0, 'CinemaAluminium')
        bb(s, 'Board_frame_side_top', WO + .25, X - .06, a * (Y - .06), a * (Y + .02), bz1, 4.05, 'CinemaAluminium')
        bb(s, 'Board_frame_side_rear_post', WO, WO + .25, a * (Y - .06), a * (Y + .03), 3.24, 4.05, 'CinemaEnamelRed')
    f.tag = 'Front_reader_board_milk_glass'
    xb = X - .058
    f.face([(xb, by0, bz0), (xb, by1, bz0), (xb, by1, bz1), (xb, by0, bz1)], 'CinemaReaderGlow')
    sx0, sx1 = WO + .25, X - .2
    for a in [-1, 1]:
        yb = a * (Y - .058); ps = [(sx0, yb, bz0), (sx1, yb, bz0), (sx1, yb, bz1), (sx0, yb, bz1)]
        f.face(ps if a < 0 else ps[::-1], 'CinemaReaderGlow')
    for z in [bz0 + .02, 3.47, 3.81, bz1 - .02]:
        bb(s, 'Changeable_letter_track', xb, xb + .014, by0, by1, z - .007, z + .007, 'CinemaAluminium')
        for a in [-1, 1]:
            bb(s, 'Side_letter_track', sx0, sx1, a * (Y - .058), a * (Y - .044), z - .007, z + .007, 'CinemaAluminium')
    # Original physical changeable letters: CLOSED, centred on the middle row.
    mid = (bz0 + bz1) / 2
    stroke_text(s, 'Changeable_marquee_letter', 'CLOSED', (xb + .016, 0, mid), (1, 0, 0), .32, .065, .008, 'Rubber', flat=L['flat'])
    for a in [-1, 1]:
        stroke_text(s, 'Side_changeable_marquee_letter', 'CLOSED', ((sx0 + sx1) / 2, a * (Y - .042), mid), (0, a, 0), .26, .055, .008, 'Rubber', flat=L['flat'])
    # Stepped cornice and the marquee roof.
    bb(s, 'Marquee_cornice_lower', WO, X + .05, -Y - .05, Y + .05, 4.05, 4.13, 'CinemaEnamelRed')
    bb(s, 'Marquee_cornice_upper', WO, X + .1, -Y - .1, Y + .1, 4.13, ZT, 'CreamEnamel')
    if L['ornament']:
        for k in range(int(2 * Y / .5)):
            yy = -Y + .25 + k * .5
            bb(s, 'Cornice_dentil', X + .1, X + .12, yy - .06, yy + .06, 4.15, 4.23, 'CinemaEnamelRed')
    # Marquee roof: flashing upstand at the wall, flasher/transformer cabinet and a small junction box.
    bb(s, 'Marquee_roof_flashing', WO, WO + .12, -Y, Y, ZT, ZT + .1, 'Metal')
    bb(s, 'Marquee_flasher_cabinet', WO + .3, WO + .85, 2.1, 2.6, ZT, ZT + .38, 'Metal')
    bb(s, 'Marquee_junction_box', WO + .3, WO + .55, -2.6, -2.35, ZT, ZT + .2, 'DarkMetal')
    s.tube('Flasher_cabinet_conduit', (WO + .02, 2.35, 4.5), (WO + .3, 2.35, ZT + .25), .02, 'Metal', 6)
    # Red neon verticals on the corner pilasters and a neon line under the cornice.
    for a in [-1, 1]:
        for dy in [-.06, .06]:
            y = a * (Y - .09) + dy
            f.tube('Marquee_corner_neon', (X + .075, y, 3.3), (X + .075, y, 3.98), .015, 'CinemaNeonRed', L['neon'])
            for z in [3.36, 3.92]:
                f.box('Neon_glass_standoff', (X + .06, y, z), (.03, .012, .012), 'Ceramic')
    f.tube('Marquee_cornice_neon', (X + .14, -Y + .2, 4.09), (X + .14, Y - .2, 4.09), .013, 'CinemaNeonRed', L['neon'])
    for a in [-1, 1]:
        f.tube('Marquee_cornice_neon_return', (WO + .3, a * (Y + .09), 4.09), (X + .05, a * (Y + .09), 4.09), .013, 'CinemaNeonRed', L['neon'])
    # Bulbs: one chaser row on the fascia, three rows hanging under the soffit.
    nb = L['bulbs']; zb = (ZB + 3.24) / 2
    for k in range(nb):
        y = -Y + .14 + k * (2 * Y - .28) / (nb - 1)
        bulb(f, (X + .03, y, zb), (1, 0, 0), L['bn'], dead=k % 11 == 6)
    ns = max(3, nb // 5)
    for a in [-1, 1]:
        for k in range(ns):
            x = WO + .35 + k * (X - .5 - WO) / (ns - 1)
            bulb(f, (x, a * (Y + .03), zb), (0, a, 0), L['bn'])
    for xx in L['soffit_rows']:
        nn = L['soffit_n']
        for k in range(nn):
            y = -Y + .35 + k * (2 * Y - .7) / (nn - 1)
            bulb(f, (xx, y, ZB - .012), (0, 0, -1), L['bn'], dead=(k * 7 + int(xx * 10)) % 9 == 4)
    # Tie rods back to the facade with turnbuckles and wall plates.
    for y in [-3.55, -1.9, 1.9, 3.55]:
        a, b = (X - .35, y, ZT), (WO + .02, y, 5.95)
        s.tube('Marquee_tie_rod', a, b, .018, 'DarkMetal', 6)
        m = add(mul(a, .55), mul(b, .45)); d = unit(sub(b, a))
        s.tube('Tie_rod_turnbuckle', add(m, mul(d, -.12)), add(m, mul(d, .12)), .03, 'DarkMetal', 6)
        s.box('Tie_rod_wall_plate', (WO + .01, y, 5.95), (.02, .12, .15), 'DarkMetal')
        s.box('Tie_rod_marquee_clevis', (X - .35, y, ZT + .03), (.14, .06, .06), 'DarkMetal')


def blade_sign(s, f, L):
    n = L['n']; x0, x1, z0, zc, r, t = .36, 1.44, 4.32, 9.12, .54, .17
    xc = (x0 + x1) / 2
    prof = [(x0, z0), (x1, z0)] + [(xc + r * math.cos(math.pi * k / L['arc']), zc + r * math.sin(math.pi * k / L['arc'])) for k in range(L['arc'] + 1)]
    pts = [(x, -t, z) for x, z in [(x0, z0), (x1, z0), (x1, zc)] + prof[3:]]
    extrude(s, 'PLAZA_blade_sign_body', pts, (0, 2 * t, 0), 'CinemaEnamelRed')
    # Raised cream border, inset, on both faces, following the outline.
    ib = .07; inner = [(x0 + ib, z0 + ib), (x1 - ib, z0 + ib)] + [(xc + (r - ib) * math.cos(math.pi * k / L['arc']), zc + (r - ib) * math.sin(math.pi * k / L['arc'])) for k in range(L['arc'] + 1)] + [(x0 + ib, z0 + ib)]
    for a in [-1, 1]:
        y = a * t
        for kk, (p, q) in enumerate(zip(inner, inner[1:])):
            d = (q[0] - p[0], 0, q[1] - p[1]); Ld = math.sqrt(d[0] ** 2 + d[2] ** 2); ax = (d[0] / Ld, 0, d[2] / Ld)
            nrm = (0, a, 0); ay = cross(nrm, ax)
            obox(s, 'Blade_cream_border', ((p[0] + q[0]) / 2, y + a * .015, (p[1] + q[1]) / 2), ax, ay, (Ld + .05, .05, .03 + .002 * (kk % 2)), 'CreamEnamel', back=False)
            f.tube('Blade_outline_neon', (p[0], y + a * .05, p[1]), (q[0], y + a * .05, q[1]), .014, 'CinemaNeonRed', L['neon'])
        # Channel letters PLAZA, top to bottom, cream cans with exposed red neon.
        nrm = (0, a, 0); _, rr, uu = frame_of(nrm); h = .64; w = .8 * h
        for k, ch in enumerate('PLAZA'):
            zc_l = 8.86 - k * .93
            org = add((xc, y, zc_l), add(mul(rr, -w / 2), mul(uu, -h / 2)))
            for kk, (p, q) in enumerate(glyph_segments(ch, org, rr, uu, h)):
                d = sub(q, p); Ld = math.sqrt(dot(d, d)); ax = mul(d, 1 / Ld); dd = .06 + .003 * (kk % 3)
                obox(s, 'Blade_channel_letter_can', add(mul(add(p, q), .5), mul(nrm, dd / 2)), ax, cross(nrm, ax), (Ld + .1, .1, dd), 'CreamEnamel', back=False)
            neon_glyph(f, ch, org, nrm, h, .075, .016, L['neon'])
    # Front edge: a column of chaser bulbs in the sign's narrow face.
    for k in range(L['blade_bulbs']):
        z = z0 + .25 + k * (zc - z0 - .45) / (L['blade_bulbs'] - 1)
        bulb(f, (x1, 0, z), (1, 0, 0), L['bn'])
    # Steel mounting: angle brackets onto the tower and two pipe braces back to the roos.
    for z in [4.6, 6.2, 7.8]:
        for a in [-1, 1]:
            s.box('Blade_angle_bracket', ((WO + .2 + x0) / 2 + .03, a * (t + .02), z), (x0 - WO - .1 + .12, .015, .16), 'DarkMetal')
    for a in [-1, 1]:
        s.tube('Blade_pipe_brace', (x0 + .1, a * .1, 9.35), (-2.0, a * .5, H), .028, 'DarkMetal', 6)
        s.box('Blade_brace_roof_pad', (-2.0, a * .5, H + .03), (.22, .22, .06), 'DarkMetal')
    s.tube('Blade_feed_conduit', (x0 + .06, .0, z0), (x0 + .06, 0, MQ_ZT), .02, 'Metal', 6)


def side_and_rear(s, f, L):
    n = L['n']
    for a in [-1, 1]:
        yo = a * (Y1 + SIDE_T / 2)
        # Steel exit door with frame, concrete stoop and a caged hood light.
        xd = -12.0
        bb(s, 'Exit_door_steel_frame', xd - .55, xd + .55, yo, yo + a * .05, 0, 2.2, 'DarkMetal')
        bb(s, 'Exit_door_leaf', xd - .47, xd + .47, yo + a * .05, yo + a * .07, .02, 2.14, 'OliveEnamel')
        bb(s, 'Exit_door_stoop', xd - .75, xd + .75, yo, yo + a * .5, 0, .15, 'Concrete')
        s.lathe('Exit_door_key_cylinder', (xd - .38, yo + a * .07, 1.05), [(0, .022), (.02, .022)], 'CinemaBrass', 8, axis=(0, a, 0))
        bb(s, 'Exit_hood', xd - .45, xd + .45, yo, yo + a * .35, 2.42, 2.48, 'Metal')
        f.lathe('Exit_caged_bulb', (xd, yo + a * .18, 2.42), [(0, .045), (.08, .05), (.14, .02)], 'WarmGlow', 8, axis=(0, 0, -1), caps=False)
        # High louvred exhaust grilles of the auditorium.
        for xv in ([-6.0, -8.9] if L['ornament'] else [-6.0]):
            bb(s, 'Auditorium_vent_frame', xv - .45, xv + .45, yo, yo + a * .05, 4.6, 5.2, 'Metal')
            for k in range(L['louvres']):
                z = 4.66 + k * .48 / max(1, L['louvres'] - 1)
                obox(s, 'Vent_louvre_blade', (xv, yo + a * .06, z), (1, 0, 0), (0, a * .7071, .7071) if a > 0 else (0, -.7071, .7071), (.84, .07, .012), 'Metal')
        # Rear scupper and downspout with straps and a splash block.
        xs = -13.4
        bb(s, 'Roof_scupper', xs - .14, xs + .14, yo, yo + a * .16, 6.05, 6.25, 'Metal')
        bb(s, 'Downspout_head', xs - .16, xs + .16, yo + a * .08, yo + a * .26, 5.75, 6.05, 'Metal')
        s.tube('Galvanized_downspout', (xs, yo + a * .17, 5.75), (xs, yo + a * .17, .32), .05, 'Metal', 8)
        s.tube('Downspout_kick_shoe', (xs, yo + a * .17, .34), (xs + .25, yo + a * .30, .1), .05, 'Metal', 8)
        for z in [1.2, 3.0, 4.8]:
            s.box('Downspout_wall_strap', (xs, yo + a * .12, z), (.13, .1, .03), 'DarkMetal')
        bb(s, 'Splash_block', xs + .1, xs + .65, yo + a * .15, yo + a * .5, 0, .06, 'Concrete')
    # Electric service on the street-visible (y+) side: meter, disconnect, mast and weatherhead.
    yo = Y1 + SIDE_T / 2; xm = -2.6
    bb(s, 'Service_meter_base', xm - .17, xm + .17, yo, yo + .14, 1.35, 1.85, 'Metal')
    s.lathe('Service_meter_glass_dome', (xm, yo + .14, 1.66), [(0, .1), (.08, .1), (.11, .07)], 'Ceramic', L['n'], axis=(0, 1, 0))
    bb(s, 'Service_disconnect_box', xm + .4, xm + .8, yo, yo + .16, 1.3, 1.85, 'DarkMetal')
    s.tube('Service_mast_conduit', (xm, yo + .07, 1.85), (xm, yo + .07, 7.7), .035, 'Metal', 8)
    s.tube('Service_weatherhead', (xm, yo + .07, 7.7), (xm, yo + .2, 7.85), .045, 'DarkMetal', 8)
    for z in [3.0, 4.8, 6.5]:
        s.box('Mast_strap', (xm, yo + .05, z), (.12, .06, .03), 'DarkMetal')
    s.tube('Disconnect_feed_conduit', (xm + .6, yo + .08, 1.3), (xm + .6, yo + .08, .35), .025, 'Metal', 6)
    s.tube('Disconnect_feed_conduit', (xm + .6, yo + .08, .35), (-1.3, yo + .08, .35), .025, 'Metal', 6)
    s.tube('Sign_feed_conduit_riser', (-1.3, yo + .08, .35), (-1.3, yo + .08, 6.72), .025, 'Metal', 6)
    s.tube('Sign_feed_conduit_over_parapet', (-1.3, yo + .08, 6.72), (-1.3, Y1 - .3, 6.72), .025, 'Metal', 6)
    s.tube('Sign_feed_conduit_drop', (-1.3, Y1 - .3, 6.72), (-1.3, Y1 - .3, H + .12), .025, 'Metal', 6)
    s.box('Conduit_pull_box', (-1.3, yo + .1, 3.5), (.22, .08, .3), 'DarkMetal')
    for z in [1.5, 2.6, 4.6, 5.7]:
        s.box('Conduit_strap', (-1.3, yo + .05, z), (.08, .06, .025), 'DarkMetal')
    # Rear: a plain service door and a gas meter.
    bb(s, 'Rear_service_door', X0 - .16, X0 - .11, 5.0, 5.95, .02, 2.1, 'DarkMetal')
    bb(s, 'Rear_door_stoop', X0 - .6, X0 - .11, 4.8, 6.15, 0, .12, 'Concrete')
    s.tube('Gas_meter_riser', (X0 - .2, 3.6, .05), (X0 - .2, 3.6, .9), .025, 'OliveEnamel', 6)
    bb(s, 'Gas_meter_body', X0 - .38, X0 - .12, 3.4, 3.8, .75, 1.15, 'OliveEnamel')


def roof_equipment(s, f, L):
    n = L['n']
    # Projection room penthouse above the lobby, hidden from the street by the parapet.
    px0, px1, py0, py1, ph = -5.3, -2.4, -2.0, 2.0, 7.55
    bb(s, 'Projection_room_penthouse', px0, px1, py0, py1, H, ph, 'CinemaStucco')
    bb(s, 'Penthouse_tar_roof', px0 - .06, px1 + .06, py0 - .06, py1 + .06, ph, ph + .06, 'CinemaRoofTar')
    bb(s, 'Penthouse_steel_door', px0 - .03, px0, -.4, .4, H + .02, H + 1.95, 'DarkMetal')
    bb(s, 'Projection_port_vent', px1, px1 + .04, -1.2, -.6, 6.85, 7.15, 'Metal')
    f.lathe('Projection_exhaust_mushroom', (-3.6, 1.0, ph + .06), [(0, .12), (.35, .12), (.36, .3), (.48, .3), (.55, .05)], 'Metal', n)
    f.lathe('Projection_vent_stack', (-4.6, -1.2, ph + .06), [(0, .08), (.5, .08), (.52, .14), (.6, .14)], 'Metal', n)
    # Evaporative (swamp) cooler on an angle-iron stand with duct into the roof.
    cx, cy, cz = -8.2, 3.8, H
    for dx in [-.5, .5]:
        for dy in [-.5, .5]:
            bb(s, 'Cooler_angle_stand_leg', cx + dx - .03, cx + dx + .03, cy + dy - .03, cy + dy + .03, cz, cz + .45, 'DarkMetal')
    bb(s, 'Cooler_stand_frame', cx - .55, cx + .55, cy - .55, cy + .55, cz + .42, cz + .47, 'DarkMetal')
    bb(s, 'Evaporative_cooler_cabinet', cx - .55, cx + .55, cy - .55, cy + .55, cz + .47, cz + 1.47, 'Metal')
    bb(s, 'Cooler_top_cap', cx - .6, cx + .6, cy - .6, cy + .6, cz + 1.47, cz + 1.53, 'Metal')
    for face, (nx, ny) in enumerate([(1, 0), (-1, 0), (0, 1)]):
        for k in range(L['louvres']):
            z = cz + .6 + k * .74 / max(1, L['louvres'] - 1)
            if nx:
                bb(s, 'Cooler_pad_louvre', cx + nx * .55, cx + nx * .58, cy - .45, cy + .45, z - .02, z + .02, 'RustTin')
            else:
                bb(s, 'Cooler_pad_louvre', cx - .45, cx + .45, cy + ny * .55, cy + ny * .58, z - .02, z + .02, 'RustTin')
    bb(s, 'Cooler_supply_duct', cx - .3, cx + .3, cy - 1.45, cy - .55, cz + .6, cz + 1.2, 'Metal')
    bb(s, 'Cooler_duct_drop', cx - .3, cx + .3, cy - 1.75, cy - 1.15, cz, cz + 1.2, 'Metal')
    bb(s, 'Duct_roof_curb', cx - .4, cx + .4, cy - 1.85, cy - 1.05, cz, cz + .12, 'Concrete')
    f.tube('Cooler_water_line', (cx + .55, cy + .3, cz + .5), (cx + 2.5, cy + .3, cz + .05), .012, 'Metal', 5)
    # Roof hatch, plumbing vents, a gooseneck vent and conduit on wooden sleepers to the sign.
    bb(s, 'Roof_hatch_curb', -11.6, -10.8, -4.4, -3.6, H, H + .3, 'Metal')
    bb(s, 'Roof_hatch_lid', -11.65, -10.75, -4.45, -3.55, H + .3, H + .36, 'Metal')
    for x, y in [(-6.8, -6.5), (-11.9, 6.8), (-12.6, -1.2)]:
        f.lathe('Plumbing_vent_stack', (x, y, H), [(0, .05), (.45, .05), (.47, .06)], 'DarkMetal', max(6, n // 2))
    f.lathe('Roof_gooseneck_vent_base', (-9.8, -6.0, H), [(0, .14), (.35, .14)], 'Metal', n)
    f.tube('Gooseneck_vent_hood', (-9.8, -6.0, H + .35), (-9.6, -6.0, H + .55), .14, 'Metal', n)
    for x in [-1.0, -3.0, -5.0, -7.0]:
        bb(s, 'Conduit_wood_sleeper', x - .07, x + .07, -1.1, -.6, H, H + .08, 'Timber')
    f.tube('Roof_sign_conduit', (-7.2, -.85, H + .12), (-1.3, -.85, H + .12), .025, 'Metal', 6)
    f.tube('Roof_sign_conduit_up', (-1.3, -.85, H + .12), (WI - .52, -.5, 7.0), .025, 'Metal', 6)
    f.tube('Roof_service_conduit', (-1.3, Y1 - .3, H + .12), (-1.3, -.85, H + .12), .025, 'Metal', 6)
    if L['ornament']:
        for x in [-3.0, -7.0, -11.0]:
            bb(s, 'Roof_membrane_seam', x - .04, x + .04, -9.7, 9.7, H, H + .012, 'CinemaRoofTar')


def apron(s, L):
    """Terrazzo entry apron 2 cm high under the marquee, red border and brass-like divider strips."""
    x0, x1, y0, y1, z = WO, MQ_X + .05, -MQ_Y - .05, MQ_Y + .05, .02
    bb(s, 'Terrazzo_apron_slab', x0, x1, y0, y1, 0, z - .001, 'Terrazzo')
    xc = [x0, x0 + .25, x0 + .37, x1 - .37, x1 - .25, x1]
    yc = [y0, y0 + .25, y0 + .37, -.3, .3, y1 - .37, y1 - .25, y1]
    for i in range(len(xc) - 1):
        for j in range(len(yc) - 1):
            cx, cy = (xc[i] + xc[i + 1]) / 2, (yc[j] + yc[j + 1]) / 2
            inside = xc[1] < cx < xc[-2] and yc[1] < cy < yc[-2]
            stripe = inside and (xc[1] < cx < xc[2] or xc[-3] < cx < xc[-2] or yc[1] < cy < yc[2] or yc[-3] < cy < yc[-2])
            mat = 'CinemaTerrazzoRed' if stripe or (inside and -.3 < cy < .3) else ('CinemaTerrazzoCream' if inside else 'Terrazzo')
            quad_xy(s, 'Terrazzo_apron_inlay', xc[i], xc[i + 1], yc[j], yc[j + 1], z, mat)


# ---------------------------------------------------------------- interior: lobby and concession
def lobby(i, g, f, L):
    n = L['n']; x0, x1 = -5.51, WI; yi = Y1 - SIDE_T / 2; ceil = 3.6
    # Terrazzo floor as non-overlapping inlay cells (no coplanar overlap, no z-fighting).
    bb(i, 'Lobby_floor_bed', x0, x1, -yi, yi, -.11, -.004, 'Concrete')
    xc = [x0, x0 + .35, x0 + .47, x1 - .47, x1 - .35, x1]
    yc = [-yi, -yi + .35, -yi + .47, -3.7, -3.58, 3.58, 3.7, yi - .47, yi - .35, yi]
    for a in range(len(xc) - 1):
        for b in range(len(yc) - 1):
            cx, cy = (xc[a] + xc[a + 1]) / 2, (yc[b] + yc[b + 1]) / 2
            inside = xc[1] < cx < xc[-2] and yc[1] < cy < yc[-2]
            stripe = inside and (xc[1] < cx < xc[2] or xc[-3] < cx < xc[-2] or yc[1] < cy < yc[2] or yc[-3] < cy < yc[-2]
                                 or yc[3] < cy < yc[4] or yc[5] < cy < yc[6])
            runner = inside and yc[4] < cy < yc[5] and xc[2] < cx < xc[-3]
            mat = 'CinemaTerrazzoRed' if stripe else 'CinemaTerrazzoCream' if runner else 'Terrazzo'
            quad_xy(i, 'Lobby_terrazzo_inlay', xc[a], xc[a + 1], yc[b], yc[b + 1], 0, mat)
    # Partition to the auditorium with two curtained openings.
    front_wall(i, -5.6, -yi, yi, ceil, [(-3.4, -1.4, 0, 2.6), (1.4, 3.4, 0, 2.6)], 'CinemaStucco', .18)
    bb(i, 'Auditorium_darkness', -6.2, -5.7, -yi, yi, 0, ceil, 'Rubber')
    for yc_ in [-2.4, 2.4]:
        for a in [-1, 1]:
            bb(i, 'Opening_walnut_casing', -5.51, -5.47, yc_ + a * 1.0 - .09, yc_ + a * 1.0 + .09, 0, 2.7, 'DarkWood')
        bb(i, 'Opening_walnut_head', -5.51, -5.45, yc_ - 1.09, yc_ + 1.09, 2.6, 2.8, 'DarkWood')
        bb(i, 'Curtain_valance', -5.66, -5.55, yc_ - 1.0, yc_ + 1.0, 2.25, 2.6, 'CinemaVelvet')
        nf = L['folds']
        for k in range(nf):
            y = yc_ - .95 + k * 1.9 / (nf - 1)
            i.tube('Closed_velvet_curtain_fold', (-5.64 + .03 * (k % 2), y, .02), (-5.64 + .03 * (k % 2), y, 2.25), .068, 'CinemaVelvet', L['fold_seg'], caps=False)
        bb(i, 'Curtain_backing', -5.7, -5.68, yc_ - 1.0, yc_ + 1.0, 0, 2.25, 'CinemaVelvet')
        bb(i, 'Curtain_gold_hem', -5.7, -5.57, yc_ - 1.0, yc_ + 1.0, .02, .09, 'CinemaBrass')
    # Lobby ceiling with a stepped cove and walnut crown.
    bb(i, 'Lobby_plaster_ceiling', x0, x1, -yi, yi, ceil, ceil + .08, 'Ceiling')
    for a in [-1, 1]:
        bb(i, 'Ceiling_walnut_crown', x0, x1, a * yi, a * (yi - .1), ceil - .12, ceil, 'DarkWood')
    bb(i, 'Ceiling_walnut_crown', x0, x0 + .1, -yi, yi, ceil - .12, ceil, 'DarkWood')
    bb(i, 'Ceiling_walnut_crown', x1 - .1, x1, -yi, yi, ceil - .12, ceil, 'DarkWood')
    # Walnut wainscot on side walls and partition, black tile base and cap rail.
    for a in [-1, 1]:
        y = a * yi
        bb(i, 'Walnut_wainscot', x0, x1, y, y - a * .03, .15, 1.15, 'DarkWood')
        bb(i, 'Black_tile_base', x0, x1, y, y - a * .035, 0, .15, 'CinemaBlackTile')
        bb(i, 'Wainscot_cap_rail', x0, x1, y, y - a * .06, 1.15, 1.2, 'Timber')
        if L['ornament']:
            for k in range(8):
                xx = x0 + .35 + k * (x1 - x0 - .7) / 7
                bb(i, 'Wainscot_raised_stile', xx - .04, xx + .04, y - a * .03, y - a * .045, .15, 1.15, 'Timber')
    for y0_, y1_ in [(-yi, -3.4), (-1.4, 1.4), (3.4, yi)]:
        bb(i, 'Walnut_wainscot', -5.51, -5.48, y0_, y1_, .15, 1.15, 'DarkWood')
        bb(i, 'Black_tile_base', -5.51, -5.475, y0_, y1_, 0, .15, 'CinemaBlackTile')
        bb(i, 'Wainscot_cap_rail', -5.51, -5.45, y0_, y1_, 1.15, 1.2, 'Timber')
    # Framed foyer posters on the side walls, each with a picture light.
    for a in [-1, 1]:
        for x, cell in [(-1.6, 12), (-3.7, 11)]:
            y = a * yi
            bb(i, 'Foyer_poster_frame', x - .62, x + .62, y, y - a * .05, 1.38, 3.22, 'DarkWood')
            i.panel('Framed_foyer_poster', (x, y - a * .052, 2.3), 1.08, 1.68, cell if a < 0 else 23 - cell, normal=(0, -a, 0))
            f.tube('Picture_light_arm', (x, y - a * .02, 3.32), (x, y - a * .18, 3.36), .01, 'CinemaBrass', 6)
            f.tube('Picture_light_hood', (x - .3, y - a * .2, 3.34), (x + .3, y - a * .2, 3.34), .035, 'CinemaBrass', 8)
            f.tube('Picture_light_tube', (x - .27, y - a * .2, 3.31), (x + .27, y - a * .2, 3.31), .015, 'WarmGlow', 6)
    # Fan-shaped wall sconces on the partition piers.
    for y in [-5.5, 0, 5.5]:
        f.lathe('Deco_wall_sconce_backplate', (-5.51, y, 1.95), [(0, .1), (.02, .1)], 'CinemaBrass', n, axis=(1, 0, 0))
        f.lathe('Deco_wall_sconce_glass', (-5.49, y, 1.95), [(0, .085), (.1, .06), (.14, .02)], 'WarmGlow', n, axis=(1, 0, 0), caps=False)
    # Ceiling drum lights with brass rings.
    for x, y in [(-2.9, -5.2), (-2.9, 0), (-2.9, 5.2)]:
        f.lathe('Ceiling_drum_light_glass', (x, y, ceil - .02), [(0, .22), (.12, .2), (.16, .12)], 'WarmGlow', n, axis=(0, 0, -1))
        f.torus('Ceiling_drum_brass_ring', (x, y, ceil - .1), .21, .012, 'CinemaBrass', n, 4)
    # Concession stand: walnut counter, terrazzo top, glass candy case, popcorn machine, back bar.
    cx0, cx1, cy0, cy1 = -4.9, -1.4, 7.6, 8.35
    bb(i, 'Concession_walnut_counter', cx0, cx1, cy0, cy1, .1, 1.0, 'DarkWood')
    bb(i, 'Concession_black_tile_kick', cx0, cx1, cy0 - .02, cy1, 0, .1, 'CinemaBlackTile')
    bb(i, 'Concession_terrazzo_top', cx0 - .05, cx1 + .05, cy0 - .06, cy1 + .03, 1.0, 1.06, 'Terrazzo')
    if L['ornament']:
        for k in range(5):
            xx = cx0 + .35 + k * (cx1 - cx0 - .7) / 4
            bb(i, 'Counter_fluted_pilaster', xx - .05, xx + .05, cy0 - .025, cy0, .1, 1.0, 'Timber')
    # Candy case: glass box on the counter with coloured boxes inside.
    kx0, kx1 = -3.0, -1.6
    bb(i, 'Candy_case_base', kx0, kx1, cy0 + .02, cy1 - .05, 1.06, 1.1, 'CinemaAluminium')
    bb(i, 'Candy_case_lid', kx0, kx1, cy0 + .02, cy1 - .05, 1.42, 1.45, 'CinemaAluminium')
    for xx in [kx0, kx1]:
        bb(i, 'Candy_case_post', xx - .015, xx + .015, cy0 + .02, cy0 + .05, 1.1, 1.42, 'CinemaAluminium')
    g.tag = 'Candy_case_glass'
    g.face([(kx0, cy0 + .03, 1.1), (kx1, cy0 + .03, 1.1), (kx1, cy0 + .03, 1.42), (kx0, cy0 + .03, 1.42)][::-1], 'Glass')
    g.face([(kx0, cy0 + .03, 1.1), (kx1, cy0 + .03, 1.1), (kx1, cy0 + .03, 1.42), (kx0, cy0 + .03, 1.42)], 'Glass')
    if L['props']:
        cols = ['CinemaEnamelRed', 'CreamEnamel', 'TealPaint', 'OliveEnamel', 'CinemaBrass']
        for k in range(L['candy']):
            xx = kx0 + .12 + (k % 8) * .155; zz = 1.1 + (k // 8) * .1
            bb(i, 'Candy_box', xx - .06, xx + .06, cy0 + .12, cy0 + .32, zz + .005, zz + .085, cols[(k * 3) % 5])
    # Popcorn machine with glass kettle cabinet and popcorn heap.
    pxc, pyc = -4.3, 7.98
    bb(i, 'Popcorn_machine_base', pxc - .38, pxc + .38, pyc - .28, pyc + .28, 1.06, 1.26, 'CinemaEnamelRed')
    bb(i, 'Popcorn_machine_roof', pxc - .4, pxc + .4, pyc - .3, pyc + .3, 1.92, 2.04, 'CinemaEnamelRed')
    for dx in [-.37, .37]:
        for dy in [-.27, .27]:
            i.tube('Popcorn_cabinet_post', (pxc + dx, pyc + dy, 1.26), (pxc + dx, pyc + dy, 1.92), .012, 'CinemaAluminium', 6)
    for ps in [[(pxc - .37, pyc - .27), (pxc + .37, pyc - .27)], [(pxc + .37, pyc - .27), (pxc + .37, pyc + .27)], [(pxc - .37, pyc + .27), (pxc - .37, pyc - .27)]]:
        (a0, a1), (b0, b1) = ps
        g.tag = 'Popcorn_cabinet_glass'
        g.face([(a0, a1, 1.26), (b0, b1, 1.26), (b0, b1, 1.92), (a0, a1, 1.92)], 'Glass')
    i.lathe('Popcorn_kettle', (pxc, pyc, 1.62), [(0, .13), (.16, .15), (.18, .02)], 'CinemaAluminium', max(8, n // 2))
    i.tube('Kettle_hanger_rod', (pxc, pyc, 1.8), (pxc, pyc, 1.92), .01, 'CinemaAluminium', 5)
    i.lathe('Popcorn_heap', (pxc, pyc, 1.26), [(0, .34), (.08, .3), (.16, .12), (.19, .03)], 'LampFabric', max(8, n // 2), caps=False)
    f.lathe('Popcorn_warmer_lamp', (pxc, pyc, 1.92), [(0, .05), (.03, .05)], 'WarmGlow', 8, axis=(0, 0, -1))
    if L['props']:
        # Cup tower, register and napkin holder.
        for k in range(3):
            i.lathe('Paper_cup_stack', (-3.4 + k * .14, 7.85, 1.06), [(0, .035), (.3, .045)], 'CreamEnamel', 8, caps=True)
        i.rounded_box('Cash_register', (-1.75, 8.0, 1.23), (.4, .38, .34), .03, 'DarkMetal', 2)
        bb(i, 'Register_key_bank', -1.7, -1.52, 7.85, 8.15, 1.35, 1.42, 'Ceramic')
    # Back bar on the side wall: shelf, syrup dispenser, menu board.
    bb(i, 'Back_bar_cabinet', -4.9, -1.6, 9.35, yi - .06, 0, .95, 'DarkWood')
    bb(i, 'Back_bar_top', -4.95, -1.55, 9.3, yi - .06, .95, 1.0, 'Terrazzo')
    bb(i, 'Back_bar_mirror_shelf', -4.9, -1.6, 9.6, yi - .06, 1.55, 1.59, 'Timber')
    bb(i, 'Menu_board', -4.4, -2.1, yi - .09, yi - .06, 2.35, 3.15, 'DarkMetal')
    for k in range(5):
        bb(i, 'Menu_board_price_strip', -4.2, -2.3 - (k % 2) * .5, yi - .1, yi - .09, 2.5 + k * .13, 2.55 + k * .13, 'Paper')
    if L['props']:
        i.rounded_box('Soda_syrup_dispenser', (-2.4, 9.6, 1.27), (.5, .35, .54), .04, 'CinemaAluminium', 2)
        for k in range(3):
            i.tube('Soda_tap', (-2.55 + k * .15, 9.43, 1.2), (-2.55 + k * .15, 9.38, 1.12), .012, 'CinemaBrass', 6)
        for k in range(6):
            i.lathe('Syrup_bottle', (-4.6 + k * .26, 9.68, 1.59), [(0, .04), (.17, .04), (.22, .015), (.25, .015)], 'CinemaEnamelRed' if k % 2 else 'OliveEnamel', 8)
    # Brass stanchions with a sagging velvet rope in front of the left curtain.
    if L['props']:
        posts = [(-4.6, -3.6), (-4.6, -2.4), (-4.6, -1.2)]
        for x, y in posts:
            i.lathe('Brass_stanchion_base', (x, y, 0), [(0, .16), (.03, .15), (.05, .04)], 'CinemaBrass', max(8, n // 2))
            i.tube('Brass_stanchion_post', (x, y, .05), (x, y, .92), .025, 'CinemaBrass', 8)
            i.lathe('Stanchion_ball_top', (x, y, .92), [(0, .02), (.03, .045), (.07, .03), (.08, .005)], 'CinemaBrass', 8, caps=False)
        for (xa, ya), (xb, yb) in zip(posts, posts[1:]):
            pts = [(xa, ya + (yb - ya) * t, .85 - .22 * math.sin(math.pi * t)) for t in [k / 6 for k in range(7)]]
            for p, q in zip(pts, pts[1:]):
                i.tube('Velvet_rope', p, q, .02, 'CinemaVelvet', 6)
        # Sand urn ashtray, waste bin, steam radiator, upholstered bench.
        i.lathe('Sand_urn_ashtray', (-1.0, -4.3, 0), [(0, .12), (.05, .09), (.55, .11), (.62, .16), (.66, .16)], 'CinemaAluminium', n)
        i.lathe('Lobby_waste_bin', (-1.0, 4.4, 0), [(0, .17), (.6, .18), (.64, .18)], 'DarkMetal', n)
        for k in range(L['radiator']):
            i.box('Radiator_column', (-1.5 - k * .07, -9.72, .45), (.05, .16, .7), 'CreamEnamel')
        bb(i, 'Lobby_bench_seat', -4.6, -3.2, -9.65, -9.2, .42, .5, 'CinemaVelvet')
        bb(i, 'Lobby_bench_frame', -4.6, -3.2, -9.65, -9.25, .05, .42, 'DarkWood')


# ---------------------------------------------------------------- build
LODS = {
    0: dict(n=12, flat=False, grille=13, booth=12, slats=22, bulbs=34, bn=6, soffit_rows=[.6, 1.25, 1.9], soffit_n=13, neon=6, arc=12,
            blade_bulbs=14, louvres=9, ornament=True, props=True, folds=16, fold_seg=6, candy=16, radiator=8),
    1: dict(n=8, flat=True, grille=9, booth=8, slats=12, bulbs=24, bn=5, soffit_rows=[.75, 1.75], soffit_n=10, neon=5, arc=8,
            blade_bulbs=10, louvres=6, ornament=True, props=True, folds=10, fold_seg=5, candy=8, radiator=5),
    2: dict(n=6, flat=True, grille=5, booth=6, slats=6, bulbs=14, bn=4, soffit_rows=[1.25], soffit_n=8, neon=4, arc=6,
            blade_bulbs=6, louvres=3, ornament=False, props=False, folds=6, fold_seg=4, candy=0, radiator=0),
}


# Tag-based LOD reduction: small parts that do not read at distance are removed from the
# coarser LODs after authoring, so all three LODs share exactly the same placement.
DROP_LOD0 = {'Terracotta_Deco_fin', 'Pylon_terracotta_flute'}
DROP_LOD1 = {'Terracotta_Deco_fin_round', 'Pylon_terracotta_flute_round', 'Fine_rain_stain', 'Coping_drip_edge',
             'Door_butt_hinge_knuckle', 'Door_lock_cylinder', 'Syrup_bottle', 'Velvet_rope', 'Stanchion_ball_top',
             'Paper_cup_stack', 'Soda_tap', 'Fine_wainscot_panel', 'Fine_coffer', 'Wainscot_raised_stile', 'Booth_grille_slot', 'Cornice_dentil', 'Stucco_patch_repair', 'Picture_light_arm',
             'Neon_glass_standoff', 'Tie_rod_turnbuckle', 'Push_bar_standoff', 'Downspout_wall_strap', 'Mast_strap',
             'Conduit_strap', 'Exit_door_key_cylinder', 'Glazing_bead', 'Menu_board_price_strip', 'Roof_membrane_seam',
             'Booth_door_knob', 'Kettle_hanger_rod', 'Tie_rod_marquee_clevis', 'Gooseneck_wall_plate', 'Booth_shutter_guide',
             'Soffit_panel_seam', 'Curtain_gold_hem', 'Cooler_water_line', 'Splash_block'}
DROP_LOD2 = DROP_LOD1 | {'Side_changeable_marquee_letter', 'Side_letter_track', 'Transom_grille_bar', 'Brass_kick_plate',
             'Pilaster_weathered_cap', 'Picture_light_hood', 'Plumbing_vent_stack', 'Popcorn_cabinet_post', 'Blade_angle_bracket',
             'Booth_brass_mullion', 'Cooler_stand_frame', 'Duct_roof_curb', 'Conduit_wood_sleeper', 'Roof_sign_conduit',
             'Roof_sign_conduit_up', 'Roof_service_conduit', 'Gas_meter_riser', 'Gas_meter_body', 'Projection_vent_stack',
             'Roof_hatch_lid', 'Downspout_kick_shoe', 'Disconnect_feed_conduit', 'Service_disconnect_box', 'Wainscot_cap_rail',
             'Ceiling_drum_brass_ring', 'Deco_wall_sconce_backplate', 'Booth_black_tile_skirting', 'Base_tile_bullnose_cap',
             'Marquee_junction_box', 'Flasher_cabinet_conduit', 'Sign_feed_conduit_over_parapet', 'Sign_feed_conduit_drop',
             'Conduit_pull_box', 'Blade_feed_conduit', 'Rear_door_stoop', 'Tie_rod_wall_plate', 'Fin_stepped_cap',
             'Booth_header_brass_band', 'Poster_case_sill', 'Exit_hood', 'Marquee_roof_flashing', 'Booth_grille_brass_ring',
             'Gooseneck_vent_hood', 'Roof_gooseneck_vent_base', 'Side_parapet_flashing', 'Rear_parapet_flashing'}


def strip(m, drop):
    keep = [k for k, tg in enumerate(m.tags) if tg not in drop]
    out = Mesh(m.name); remap = {}
    for k in keep:
        face = []
        for vi in m.f[k]:
            if vi not in remap:
                remap[vi] = len(out.v); out.v.append(m.v[vi])
            face.append(remap[vi])
        out.f.append(face); out.mat.append(m.mat[k]); out.uv.append(m.uv[k]); out.smooth.append(m.smooth[k]); out.tags.append(m.tags[k])
    return out


def build(lod):
    L = LODS[lod]
    s = Mesh('SM_Town_Cinema_Shell'); g = Mesh('SM_Town_Cinema_Glass')
    i = Mesh('SM_Town_Cinema_Interior'); f = Mesh('SM_Town_Cinema_Fixtures')
    walls_and_roof(s, L)
    facade_deco(s, L)
    entry_doors(s, g, f, L)
    ticket_booth(s, g, i, f, L)
    poster_cases(s, g, f, L)
    marquee(s, f, L)
    blade_sign(s, f, L)
    side_and_rear(s, f, L)
    roof_equipment(s, s, L)
    apron(s, L)
    lobby(i, g, f, L)
    drop = [DROP_LOD0, DROP_LOD1, DROP_LOD2][lod]
    return {m.name: strip(m, drop) for m in [s, g, i, f]}


if __name__ == '__main__':
    for lod in [0, 1, 2]:
        d = build(lod)
        print(lod, {k[15:]: sum(len(p) - 2 for p in m.f) for k, m in d.items()})
