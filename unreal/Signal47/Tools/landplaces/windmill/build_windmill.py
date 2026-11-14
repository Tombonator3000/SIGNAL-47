"""LAND-PLACES-WINDMILL-20261114: the stock-water windmill of the Baca lease (and the farm's), an original model after Tom's
reference pack (~/scratch-bm/refs/land-places-20261113/windmill: hero, four views, details; the stated measures win over the
pictures). Geometry: Tools/landplaces/lpkit.py (plain Python); surfaces, bake and export: lpbake.py (Blender 5.2).

  python3 build_windmill.py --dry                       measures and triangles, no Blender
  blender -b --python build_windmill.py -- <work> [clay] [bake] [export] [look]      (through the queue: run.py blender ...)

Writes <work>/Export/SM_LP_Mill_*.fbx (+ _LOD1.._LOD3), <work>/textures/T_LP_Mill*_{BC,N,ORM}.png, <work>/Preview/*.png and
<work>/contract.json (triangles, bounds, pivots: Tools/landplaces/import_assets.py and S47LandWindmill.cpp read the same numbers).

The model's frame: metres, x east, y north, z up, the tower's middle on the ground at the origin; the wheel faces south (-y),
the tail runs north. Measures: hub 9.50 m, wheel 2.40 m with 18 blades on two rims, tower base 2.50 m square on four concrete
piers, pump housing 0.80 x 0.65 x 0.60 m with a hinged lid, stock tank 3.00 m across and 0.60 m high.
Parts (each a mesh of its own, its origin where the game turns or moves it):
  Tower, Pier (four of it), Rod (strokes along z), Head (yaws about z; gearbox, tail and vane), Wheel (spins about y),
  Housing (the pump in it), Lid (opens about x at its hinge), Tank, Float (bobs about y at the valve), Water, Page (Reyes's page
  in its bag), VaneLabel (BACA LEASE NO. 2, worn; the Baca mill only), WireRun and WireKnot (the bright new wire).
"""
import json, math, os, random, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import lpkit as K
from lpkit import N, add, sub, mul, unit, cross, basis, place, ring, TAU

DRY = '--dry' in sys.argv
argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
WORK = Path(argv[0] if argv else os.path.expanduser('~/scratch-bm/land-places-20261114/windmill/work')).resolve()
WANT = argv[1:] or ['clay']
SRC = Path(os.path.expanduser(os.environ.get('S47_LP_SRC', '~/scratch-bm/land-places-20261114/windmill'))).resolve()

# ---------------------------------------------------------------- measures (metres)
HUB_Z, WHEEL_R, BLADES, HUB_OFF = 9.50, 1.20, 18, 0.46
PIER_TOP, LEG_TOP, FOOT, TOPH = 0.26, 9.05, 1.25, 0.15
GIRTS = [2.05, 3.85, 5.45, 6.85, 8.05]
MAST_TOP = 9.30
HOUSE = (-0.40, 0.40, -0.47, 0.18, 0.60)          # x0, x1, y0 (front, south), y1, top
HINGE = (0.0, -0.11, 0.615)
TANK = (4.35, -0.35)
TANK_R, TANK_H, WATER_Z = 1.50, 0.60, 0.47
VALVE = (TANK[0] - TANK_R + 0.20, TANK[1], 0.74)
OVER_AZ = math.radians(-55)                       # the overflow, south-east
PAGE_AT = (0.0, -0.078, 0.355)                   # the bag hangs on the pump's gland by a twist of wire, where the lifted lid shows it
STROKE = 0.19
CORNERS = [(1, 1), (-1, 1), (-1, -1), (1, -1)]    # NE, NW, SW, SE
UP = (0, 0, 1)


def half(z):
    return FOOT + (TOPH - FOOT) * (z - PIER_TOP) / (LEG_TOP - PIER_TOP)


def corner(c, z, inset=0.0):
    h = half(z) - inset
    return (CORNERS[c][0] * h, CORNERS[c][1] * h, z)


TOWER_A, TOP_A, BASE_A = K.Atlas('MillTower', 4096), K.Atlas('MillTop', 2048), K.Atlas('MillBase', 4096)


class Direct(K.Atlas):
    """Charts given their own place in the picture (labels)."""
    def __init__(self, name, rects):
        K.Atlas.__init__(self, name, 1024)
        self.rects, self.locked = rects, True

    def chart(self, key, w, h, dens=1.0):
        c = self.charts.get(key)
        if not c:
            c = K.Chart(key, w, h, dens)
            u0, v0, u1, v1 = self.rects.get(key, (0, 0, 1, 1))
            c.x, c.y, c.pw, c.ph = u0 * self.size, v0 * self.size, (u1 - u0) * self.size, (v1 - v0) * self.size
            self.charts[key] = c
        return c


PLAIN_A = Direct('plain', {})
PAGE_A = Direct('page', {'page/f': (0, 0, 1, 1), 'page/b': (0, 0, 1, 1)})
LABEL_A = Direct('label', {'labelE/f': (0, 0, 1, 1), 'labelW/f': (0, 0, 1, 1)})

P = {}
for _name, _atlas, _pivot, _group in (
        ('Tower', TOWER_A, (0, 0, 0), 0), ('Rod', TOWER_A, (0, 0, 0), 1), ('Pier', BASE_A, (0, 0, 0), 2),
        ('Head', TOP_A, (0, 0, HUB_Z), 3), ('Wheel', TOP_A, (0, -HUB_OFF, HUB_Z), 4),
        ('Housing', BASE_A, (0, 0, 0), 5), ('Lid', BASE_A, HINGE, 6), ('Tank', BASE_A, (TANK[0], TANK[1], 0), 7), ('Float', BASE_A, VALVE, 8),
        ('Water', PLAIN_A, (TANK[0], TANK[1], 0), 9), ('Page', PAGE_A, PAGE_AT, 10), ('VaneLabel', LABEL_A, (0, 0, HUB_Z), 11),
        ('WireRun', PLAIN_A, (0, 0, 0), 12), ('WireKnot', PLAIN_A, (0, 0, 0), 13)):
    P[_name] = K.Part(_name, _atlas, _pivot, _group)
LODS = {'Tower': 5, 'Rod': 4, 'Pier': 3, 'Head': 4, 'Wheel': 5, 'Housing': 4, 'Lid': 3, 'Tank': 4, 'Float': 2, 'Water': 2, 'Page': 1, 'VaneLabel': 1, 'WireRun': 1, 'WireKnot': 1}


def angle_between(a, b, z, inset, mat, key, size=0.045, t=0.005, dens=1.0, flip=False):
    """An angle iron on the inside of a face from corner line a to corner line b at height z: one flange up the face, one a shelf inward."""
    pa, pb = corner(a, z, inset), corner(b, z, inset)
    out = unit(((CORNERS[a][0] + CORNERS[b][0]) / 2, (CORNERS[a][1] + CORNERS[b][1]) / 2, 0))
    inward = mul(out, -1)
    d = cross(inward, UP)
    if K.dot(d, sub(pb, pa)) < 0:
        pa, pb = pb, pa
    e = unit(sub(pb, pa))
    pa, pb = add(pa, mul(e, 0.02)), sub(pb, mul(e, 0.02))
    k = N(1, 1, 1.5, 3.0)
    K.bar(key, pa, pb, K.sec_L(size * k, size * k, t * k, 0.002), mat, ref=UP, dens=dens, cap0=LOD_CAPS(), cap1=LOD_CAPS())


def LOD_CAPS():
    return K.LOD < 2


# ---------------------------------------------------------------- the parts
def build_tower():
    K.target(P['Tower'])
    rnd = random.Random(11)
    levels = [PIER_TOP] + GIRTS + [LEG_TOP]
    if K.LOD >= 4:
        # beyond half a kilometre a lattice is less than a pixel a member: one slim tapered pylon, about as dark as the lattice was
        x, y = (1, 0, 0), (0, 1, 0)
        K.sweep('cap', K.sec_rect(1.3, 1.3), [ring((0, 0, PIER_TOP), x, y), ring((0, 0, LEG_TOP), x, y, 0.2), ring((0, 0, MAST_TOP), x, y, 0.12)], 'galv', cap0=False, cap1=False, vmax=40.0)   # (on the cap's dark iron: a far tower reads dark against the sky)
        K.bar('plank0', (0, 0, GIRTS[-1] + 0.06), (0, 0, GIRTS[-1] + 0.14), K.sec_rect(1.06, 1.08), 'wood', ref=(0, 1, 0))
        return
    k = N(1, 1, 1.6, 3.4)                                      # far off the members are drawn thicker, so the tower still reads
    for c, (sx, sy) in enumerate(CORNERS):
        x, y = ((-sx, 0, 0), (0, -sy, 0)) if sx * sy > 0 else ((0, -sy, 0), (-sx, 0, 0))
        for i in range(len(levels) - 1):
            z0, z1 = levels[i], levels[i + 1]
            K.sweep('leg%d_%d' % (c, i), K.sec_L(0.065 * k, 0.065 * k, 0.006 * k, 0.003), [ring(corner(c, z0), x, y), ring(corner(c, z1), x, y)], 'galv',
                    cap0=False, cap1=(i == len(levels) - 2), dens=(1.25 if i == 0 else 1.0 if i < 2 else 0.75), tone=rnd.random() * .5)
        # the foot: a plate on the pier and its two anchor bolts
        if K.LOD < 3:
            px, py = sx * (FOOT - 0.045), sy * (FOOT - 0.045)
            K.bar('foot%d' % c, (px, py, PIER_TOP - 0.002), (px, py, PIER_TOP + 0.012), K.sec_rect(0.21, 0.21, 0.006), 'galv', ref=(0, 1, 0), dens=1.3, cap0=False, tone=.6)
            if K.LOD == 0:
                for j, (bx, by) in enumerate(((-0.065, 0.065), (0.065, -0.065))):
                    K.bolt('anchor', (px + bx, py + by, PIER_TOP + 0.012), UP, 0.017, 0.03, 'cast', dens=1.3, tone=1.0)
                    K.tube('anchorstud', [(px + bx, py + by, PIER_TOP + 0.03), (px + bx, py + by, PIER_TOP + 0.055)], 0.008, 6, 'cast', cap0=False, dens=1.3)
    # girts round the tower at each level, and a plate with two bolts where each meets a leg
    for li, z in enumerate(GIRTS):
        for f in range(4):
            a, b = f, (f + 1) % 4
            angle_between(a, b, z, 0.008, 'galv', 'girt%d_%d' % (li, f), dens=(1.0 if li < 2 else 0.75))
            if K.LOD < 2:
                out = unit(((CORNERS[a][0] + CORNERS[b][0]) / 2, (CORNERS[a][1] + CORNERS[b][1]) / 2, 0))
                along = unit(sub(corner(b, z), corner(a, z)))
                for e, cc in ((0, a), (1, b)):
                    p = add(corner(cc, z), mul(along, 0.075 if e == 0 else -0.075))
                    p = add(p, mul(out, 0.001))
                    xx, yy, zz = basis(out, UP)
                    K.sweep('gus%d_%d_%d' % (li, f, e), K.sec_rect(0.13, 0.11, 0.008), [ring(p, xx, yy), ring(add(p, mul(out, 0.005)), xx, yy)], 'galv', cap0=False, dens=(1.0 if li < 2 else 0.6), tone=.5)
                    if K.LOD == 0 and li < 3:
                        for s in (-0.03, 0.03):
                            K.bolt('gbolt', add(add(p, mul(along, s)), mul(out, 0.005)), out, 0.011, 0.009, 'cast', tone=1.0)
    if K.LOD == 0:
        # each leg is two lengths: a splice angle outside the joint, four bolts
        zs = 4.62
        for c, (sx, sy) in enumerate(CORNERS):
            x, y = ((-sx, 0, 0), (0, -sy, 0)) if sx * sy > 0 else ((0, -sy, 0), (-sx, 0, 0))
            o0, o1 = corner(c, zs - 0.16, -0.0065), corner(c, zs + 0.16, -0.0065)
            K.sweep('splice%d' % c, K.sec_L(0.076, 0.076, 0.0065, 0.003), [ring(o0, x, y), ring(o1, x, y)], 'galv', dens=0.8, tone=.75)
            for dz in (-0.09, 0.09):
                h = half(zs + dz)
                K.bolt('gbolt', (sx * (h + 0.0065), sy * (h - 0.04), zs + dz), (sx, 0, 0), 0.011, 0.009, 'cast', tone=1.0)
                K.bolt('gbolt', (sx * (h - 0.04), sy * (h + 0.0065), zs + dz), (0, sy, 0), 0.011, 0.009, 'cast', tone=1.0)
        # the brake: a hand lever on the south-west leg, down and latched (the brake is on), and its wire up to the head
        cz = 1.22
        p = add(corner(2, cz), (-0.012, -0.012, 0))
        K.bar('brakeplate', add(p, (0.035, 0.035, -0.06)), add(p, (0.035, 0.035, 0.06)), K.sec_rect(0.012, 0.09), 'cast', ref=unit((1, -1, 0)), dens=1.2, tone=1.0)
        tip = add(corner(2, 0.74), (-0.10, -0.10, 0))
        K.bar('brakelever', p, tip, K.sec_rect(0.03, 0.007, 0.002), 'cast', ref=unit((1, 1, 0)), dens=1.2, tone=.9)
        K.tube('brakegrip', [tip, add(tip, (-0.012, -0.012, -0.11))], 0.011, 8, 'wood', dens=1.2)
        K.bolt('gbolt', add(p, (-0.008, -0.008, 0)), unit((-1, -1, 0)), 0.012, 0.01, 'cast', tone=1.0)
        K.tube('brakewire', [add(p, (0, 0, 0.05)), add(corner(2, 4.4), (-0.02, -0.02, 0)), add(corner(2, GIRTS[-1] - 0.05), (-0.02, -0.02, 0))], 0.0028, 4, 'cast', cap0=False, cap1=False, dens=0.5, vmax=3.0, tone=.5)
    # crossed rods in every panel of every face (the lowest south panel is left open: the way in to the pump)
    if K.LOD < 3:
        rr, n = 0.006 * N(1, 1, 1.6), N(6, 4, 3)
        for i in range(len(levels) - 2):
            z0, z1 = levels[i] + (0.06 if i == 0 else 0.03), levels[i + 1] - 0.03
            for f in range(4):
                if i == 0 and f == 2:
                    continue
                a, b = f, (f + 1) % 4
                for q, (c0, c1) in enumerate(((a, b), (b, a))):
                    ins = 0.016 + 0.014 * q
                    p0, p1 = corner(c0, z0, ins), corner(c1, z1, ins)
                    e = unit(sub(corner(c1, z0, ins), p0))
                    p0, p1 = add(p0, mul(e, 0.05)), sub(p1, mul(e, 0.05))
                    K.tube('brace%d_%d_%d' % (i, f, q), [p0, p1], rr, n, 'galv', cap0=False, cap1=False, dens=0.7, tone=rnd.random() * .8, vmax=2.6)
    # the platform under the wheel: bearers on the top girt, four planks round the tower
    z = GIRTS[-1]
    if K.LOD < 3:
        for s in (-1, 1):
            K.bar('bearer%d' % s, (-0.52, s * 0.33, z + 0.06), (0.52, s * 0.33, z + 0.06), K.sec_rect(0.07, 0.07, 0.006), 'wood', ref=UP, cham=0.004, dens=0.7)
        for j, (a, b, w) in enumerate((((-0.42, -0.54, z + 0.111), (-0.42, 0.54, z + 0.111), 0.245), ((0.42, -0.54, z + 0.111), (0.42, 0.54, z + 0.111), 0.245),
                                       ((-0.29, 0.42, z + 0.111), (0.29, 0.42, z + 0.111), 0.235), ((-0.29, -0.42, z + 0.111), (0.29, -0.42, z + 0.111), 0.235))):
            K.bar('plank%d' % j, a, b, K.sec_rect(w, 0.032, 0.005), 'wood', ref=UP, cham=0.004, dens=0.7, tone=rnd.random())
    else:
        K.bar('plank0', (0, 0, z + 0.06), (0, 0, z + 0.13), K.sec_rect(1.06, 1.08), 'wood', ref=(0, 1, 0))
    # the top: a cap plate on the four legs, a collar, and the mast pipe the head turns on
    K.bar('cap', (0, 0, LEG_TOP), (0, 0, LEG_TOP + 0.02), K.sec_rect(0.36, 0.36, 0.02), 'cast', ref=(0, 1, 0), dens=0.7, cap0=LOD_CAPS())
    K.revolve('mast', [(0.075, LEG_TOP + 0.02), (0.075, LEG_TOP + 0.08), (0.046, LEG_TOP + 0.09), (0.046, MAST_TOP)], place((0, 0, 0), (1, 0, 0), (0, 1, 0), UP), N(12, 8, 6, 4), 'galv', dens=0.7, tone=.4)
    # two guides for the pump rod, a flat across the tower with a wooden block
    if K.LOD < 2:
        for gi, z in enumerate((GIRTS[1], GIRTS[3])):
            h = half(z) - 0.02
            K.bar('guide%d' % gi, (-h, 0, z + 0.03), (h, 0, z + 0.03), K.sec_rect(0.05, 0.006), 'galv', ref=UP, dens=0.7, cap0=False, cap1=False)
            K.bar('gblock%d' % gi, (0, -0.07, z + 0.065), (0, 0.07, z + 0.065), K.sec_rect(0.11, 0.06, 0.006), 'wood', ref=UP, cham=0.004, dens=0.7)
    # the ladder on the west face, from knee height to the third girt
    if K.LOD < 3:
        z0, z1 = 0.50, GIRTS[2] + 0.12
        for s in (-1, 1):
            y = 0.25 + s * 0.19
            K.bar('rail%d' % s, (-(half(z0) + 0.05), y, z0), (-(half(z1) + 0.05), y, z1), K.sec_rect(0.006 * N(1, 1, 2), 0.04), 'galv', ref=(0, 1, 0), dens=0.8, vmax=2.0, tone=.3)
        zz = z0 + 0.2
        ri = 0
        while zz < z1 - 0.1:
            x = -(half(zz) + 0.05)
            K.tube('rung', [(x, 0.06, zz), (x, 0.44, zz)], 0.008 * N(1, 1, 1.5), N(6, 4, 3), 'galv', ref=UP, cap0=False, cap1=False, dens=0.8, tone=(0.9 if ri in (3, 8) else 0.2))
            zz += 0.30
            ri += 1
        for gi in (0, 1, 2):                                    # stand-offs to the girts
            z = GIRTS[gi]
            for s in (-1, 1):
                y = 0.25 + s * 0.19
                K.bar('stand%d_%d' % (gi, s), (-(half(z) - 0.01), y, z + 0.02), (-(half(z) + 0.055), y, z + 0.02), K.sec_rect(0.03, 0.005), 'galv', ref=UP, dens=0.6, cap0=False)


def build_rod():
    K.target(P['Rod'])
    r = 0.011 * N(1, 1, 1.5, 2.2)
    K.tube('rod', [(0, 0, 0.72), (0, 0, MAST_TOP - 0.04)], r, N(8, 6, 4, 3), 'cast', cap0=False, cap1=False, dens=0.8, vmax=2.4, tone=.7)
    if K.LOD < 2:
        for i, z in enumerate((2.75, 4.85, 7.35)):
            K.revolve('coupling', [(0.011, z - 0.04), (0.018, z - 0.035), (0.018, z + 0.035), (0.011, z + 0.04)], place((0, 0, 0), (1, 0, 0), (0, 1, 0), UP), N(8, 6), 'cast', dens=0.8, tone=1.0)


def build_pier():
    K.target(P['Pier'])
    x, y = (1, 0, 0), (0, 1, 0)
    rings = [ring((0, 0, -0.70), x, y, 1.28), ring((0.004, -0.003, -0.05), x, y, 1.07), ring((0, 0, PIER_TOP - 0.025), x, y, 1.0), ring((0, 0, PIER_TOP), x, y, 0.94)]
    K.sweep('pier', K.sec_rect(0.40, 0.40, 0.028), rings if K.LOD < 2 else [rings[0], rings[2]], 'conc', cap0=False, dens=1.0, sharp=50)


def build_head():
    K.target(P['Head'])
    C = (0, 0, HUB_Z)
    ax = place(C, (1, 0, 0), UP, (0, -1, 0))                     # local z along the axle, toward the wheel
    seg = N(20, 14, 8, 6)
    K.revolve('case', [(0.0, -0.24), (0.10, -0.24), (0.13, -0.21), (0.13, 0.27), (0.10, 0.30), (0.05, 0.30), (0.05, 0.385), (0.0, 0.385)], ax, seg, 'cast', dens=1.2, tone=.6)
    if K.LOD < 2:
        for i, z in enumerate((-0.12, 0.17)):
            K.revolve('band%d' % i, [(0.13, z - 0.02), (0.137, z - 0.018), (0.137, z + 0.018), (0.13, z + 0.02)], ax, seg, 'galv', dens=1.0, tone=.8)
        K.bar('lug', (0, 0.02, HUB_Z + 0.12), (0, 0.02, HUB_Z + 0.185), K.sec_rect(0.05, 0.11, 0.008), 'cast', ref=(0, 1, 0), dens=1.0)   # the oil cover on top
    if K.LOD == 0:
        K.revolve('cover', [(0.045, -0.24), (0.108, -0.24), (0.108, -0.252), (0.045, -0.258)][::-1], ax, seg, 'cast', dens=1.0, tone=.9)
        for i in range(6):
            a = TAU * i / 6 + 0.3
            K.bolt('cbolt', ax((0.09 * math.cos(a), 0.09 * math.sin(a), -0.252)), (0, 1, 0), 0.011, 0.009, 'cast', tone=1.0)
        for s in (-1, 1):
            K.bar('cheek%d' % s, (s * 0.012, 0.19, HUB_Z), (s * 0.012, 0.34, HUB_Z + 0.002), K.sec_rect(0.006, 0.075, 0.002), 'cast', ref=UP, dens=1.0, tone=.9)
        K.bolt('cbolt', (0.015, 0.30, HUB_Z), (1, 0, 0), 0.012, 0.01, 'cast', tone=1.0)
        K.bolt('cbolt', (-0.015, 0.30, HUB_Z), (-1, 0, 0), 0.012, 0.01, 'cast', tone=1.0)
    zax = place((0, 0, 0), (1, 0, 0), (0, 1, 0), UP)
    K.revolve('turn', [(0.05, MAST_TOP), (0.115, MAST_TOP), (0.115, MAST_TOP + 0.035), (0.078, MAST_TOP + 0.045), (0.078, HUB_Z - 0.10)], zax, N(16, 12, 8, 6), 'cast', dens=1.0, tone=.5)
    # the tail: a spine out to the vane, two stays, the sheet with its rolled edge and stiffeners
    k = N(1, 1, 1.5, 2.5)
    K.bar('spine', (0, 0.20, HUB_Z), (0, 2.34, HUB_Z + 0.02), K.sec_rect(0.008 * k, 0.04 * k), 'galv', ref=UP, dens=0.9, cap0=False, vmax=1.3, tone=.5)
    if K.LOD < 3:
        for s in (-1, 1):
            K.tube('stay%d' % s, [(0, 0.06, HUB_Z + s * 0.125), (0.004, 1.86, HUB_Z + s * 0.385)], 0.006 * k, N(6, 4, 3), 'galv', cap0=False, cap1=False, dens=0.8, vmax=2.0, tone=.6)
    y0, L, h0, h1 = 1.32, 1.02, 0.20, 0.47

    def halfh(u):
        return 0.035 + (h0 + (h1 - h0) * 0.12 - 0.035) * (u / 0.12) if u < 0.12 else h0 + (h1 - h0) * u

    nu = N(5, 3, 2, 2)

    def vane(u, w):
        uu = u if nu < 3 else (0.0 if u == 0 else 0.12 + (u * nu - 1) / (nu - 1) * 0.88)
        hh = halfh(uu)
        z = HUB_Z + 0.01 + (2 * w - 1) * hh
        x = 0.012 * math.sin(uu * 2.6 + 0.4) * (2 * w - 1) + 0.006 * math.sin(uu * 7.0)       # a sheet that has been out in the wind
        return (x, y0 + L * uu, z)
    K.sheet('vane', vane, nu, N(4, 2, 1, 1), N(0.003, 0.003, 0, 0), 'galv', (L, 2 * h1), dens=1.15, tone=.35)
    if K.LOD < 2:
        edge = [vane(0, 0), vane(1.0 / nu, 0), vane(1, 0), vane(1, 1), vane(1.0 / nu, 1), vane(0, 1), vane(0, 0)]
        K.tube('vrim', [add(p, (0.0015, 0, 0)) for p in edge], 0.006, N(5, 4), 'galv', ref=(1, 0, 0), cap0=False, cap1=False, dens=0.8, vmax=1.2, tone=.7)
        for i, uu in enumerate((0.42, 0.80)):
            yy, hh = y0 + L * uu, halfh(uu) - 0.015
            K.bar('stiff%d' % i, (0.004, yy, HUB_Z + 0.01 - hh), (0.004, yy, HUB_Z + 0.01 + hh), K.sec_rect(0.004, 0.028), 'galv', ref=(0, 1, 0), dens=0.8, cap0=False, cap1=False, tone=.8)
            if K.LOD == 0:
                for j in range(5):
                    K.bolt('rivet', (0.006, yy, HUB_Z + 0.01 + hh * (j - 2) * 0.42), (1, 0, 0), 0.008, 0.004, 'galv', segs=5, tone=1.0)


def build_label():
    K.target(P['VaneLabel'])
    y0, y1, hh, z = 1.50, 2.26, 0.20, HUB_Z + 0.01
    K.sheet('labelE', lambda u, w: (0.0075, y0 + (y1 - y0) * u, z + (2 * w - 1) * hh), 1, 1, 0, 'label', (y1 - y0, 2 * hh), back=False)
    K.sheet('labelW', lambda u, w: (-0.0045, y1 - (y1 - y0) * u, z + (2 * w - 1) * hh), 1, 1, 0, 'label', (y1 - y0, 2 * hh), back=False)


def build_wheel():
    K.target(P['Wheel'])
    rnd = random.Random(5)
    C = (0, -HUB_OFF, HUB_Z)
    e1, e2, ea = (1, 0, 0), UP, (0, -1, 0)
    ax = place(C, e1, e2, ea)
    if K.LOD >= 4:
        K.revolve('hub', [(0.0, -0.04), (1.15, -0.04), (1.15, 0.02), (0.0, 0.02)], ax, 12, 'galv')
        return
    K.revolve('hub', [(0.0, -0.10), (0.07, -0.10), (0.07, -0.012), (0.15, -0.012), (0.15, 0.012), (0.07, 0.012), (0.07, 0.08), (0.045, 0.105), (0.0, 0.105)], ax, N(16, 12, 8, 6), 'cast', dens=1.1, tone=.7)
    if K.LOD == 0:
        K.bolt('hubnut', add(C, mul(ea, 0.105)), ea, 0.028, 0.03, 'cast', tone=1.0)
    k = N(1, 1, 1.6, 2.6)
    for i, (r, a) in enumerate(((0.50, 0.0125), (0.97, 0.0125))):
        t = 0.0025 * k
        K.revolve('rim%d' % i, [(r - t, -0.03 - a * k), (r + t, -0.03 - a * k), (r + t, -0.03 + a * k), (r - t, -0.03 + a * k), (r - t, -0.03 - a * k)], ax, N(54, 36, 18, 12), 'galv', charts=2, dens=0.8, tone=.5)
    if K.LOD < 3:
        for i in range(6):
            th = math.radians(10 + 60 * i)
            er = add(mul(e1, math.cos(th)), mul(e2, math.sin(th)))
            K.bar('arm%d' % i, add(add(C, mul(er, 0.10)), mul(ea, -0.012)), add(add(C, mul(er, 0.985)), mul(ea, -0.045)), K.sec_rect(0.03 * k, 0.005 * k), 'galv', ref=ea, dens=0.8, cap0=False, cap1=False, tone=rnd.random())
    r0, r1, c0, c1 = 0.42, 1.19, 0.105, 0.30
    for b in range(BLADES):
        th = TAU * b / BLADES
        er = add(mul(e1, math.cos(th)), mul(e2, math.sin(th)))
        et = add(mul(e1, -math.sin(th)), mul(e2, math.cos(th)))
        bent = 0.035 if b == 7 else (0.015 if b == 13 else 0.0)

        def blade(u, w, er=er, et=et, bent=bent):
            r = r0 + (r1 - r0) * u
            c = c0 + (c1 - c0) * u
            ph = math.radians(40 - 12 * u)
            q = (w - 0.5) * c
            cam = 0.10 * c * (1 - 4 * (w - 0.5) ** 2)
            p = add(C, mul(er, r))
            p = add(p, add(mul(et, q * math.cos(ph) - cam * math.sin(ph)), mul(ea, q * math.sin(ph) + cam * math.cos(ph) + 0.012 - bent * u * u)))
            return p
        K.sheet('blade%d' % b, blade, N(4, 2, 1, 1), N(4, 2, 1, 1), N(0.0025, 0.0025, 0, 0), 'galv', (r1 - r0, c1), dens=1.0, tone=rnd.random() ** 2)
        if K.LOD == 0:                                           # the ears that hold a blade to the two rims
            for i, r in enumerate((0.50, 0.97)):
                u = (r - r0) / (r1 - r0)
                p = blade(u, 0.5)
                K.bar('ear', add(add(C, mul(er, r)), mul(ea, -0.03)), p, K.sec_rect(0.022, 0.004), 'galv', ref=er, dens=0.8, cap0=False, cap1=False, tone=.9)


def corrugated(key, o, eu, ew, en, lu, lw, mat, waves_along='w', pitch=0.076, amp=0.009, dens=1.0, tone=0.0, thick=0.0018):
    """A corrugated sheet from o: lu along eu, lw along ew, its face toward en; the waves run across `waves_along`."""
    span = lw if waves_along == 'w' else lu
    nwav = span / pitch
    cuts = max(1, int(round(nwav * N(6, 4, 0.01, 0.01))))
    nu, nw = (1, cuts) if waves_along == 'w' else (cuts, 1)
    a = amp if K.LOD < 2 else 0.0

    def pt(u, w):
        d = a * math.sin(TAU * (w if waves_along == 'w' else u) * nwav)
        return add(add(add(o, mul(eu, lu * u)), mul(ew, lw * w)), mul(en, d))
    K.sheet(key, pt, nu, nw, thick if K.LOD < 2 else 0.0, mat, (lu, lw), dens=dens, tone=tone)


def build_housing():
    K.target(P['Housing'])
    x0, x1, y0, y1, top = HOUSE
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    K.bar('slab', (cx, cy, -0.22), (cx, cy, 0.03), K.sec_rect(x1 - x0 + 0.12, y1 - y0 + 0.12, 0.015), 'conc', ref=(0, 1, 0), dens=1.1, cap0=False)
    k = N(1, 1, 1.4, 2.0)
    # the frame: four posts, rails round the top and the foot
    for i, (sx, sy) in enumerate(CORNERS):
        px, py = (x1 if sx > 0 else x0), (y1 if sy > 0 else y0)
        x, y = ((-sx, 0, 0), (0, -sy, 0)) if sx * sy > 0 else ((0, -sy, 0), (-sx, 0, 0))
        K.sweep('post%d' % i, K.sec_L(0.032 * k, 0.032 * k, 0.0035 * k, 0.002), [ring((px, py, 0.03), x, y), ring((px, py, top), x, y)], 'galv', dens=1.4, cap0=False, tone=.7)
        if K.LOD == 0:
            for zz in (0.09, top - 0.075):
                K.bolt('hbolt', (px, py - sy * 0.017, zz), (sx, 0, 0), 0.007, 0.005, 'cast', dens=1.4, tone=1.0)
                K.bolt('hbolt', (px - sx * 0.017, py, zz), (0, sy, 0), 0.007, 0.005, 'cast', dens=1.4, tone=1.0)
    if K.LOD < 3:
        for i, (a, b) in enumerate((((x0, y0), (x1, y0)), ((x1, y0), (x1, y1)), ((x1, y1), (x0, y1)), ((x0, y1), (x0, y0)))):
            for j, z in enumerate((0.03, top - 0.03)):
                e = unit((b[0] - a[0], b[1] - a[1], 0))
                inw = cross(UP, e)
                pa, pb = (a[0] + inw[0] * 0.0045, a[1] + inw[1] * 0.0045, z + 0.015), (b[0] + inw[0] * 0.0045, b[1] + inw[1] * 0.0045, z + 0.015)
                K.bar('rail%d_%d' % (i, j), add(pa, mul(e, 0.004)), sub(pb, mul(e, 0.004)), K.sec_rect(0.004 * k, 0.03), 'galv', ref=UP, dens=1.4, cap0=False, cap1=False, tone=.8)
    # corrugated sheet on the four sides, the waves lying
    ins = 0.012
    corrugated('wallS', (x0 + 0.004, y0 + ins, 0.035), (1, 0, 0), UP, (0, -1, 0), x1 - x0 - 0.008, top - 0.045, 'galv', dens=1.4, tone=.3)
    corrugated('wallE', (x1 - ins, y0 + 0.004, 0.035), (0, 1, 0), UP, (1, 0, 0), y1 - y0 - 0.008, top - 0.045, 'galv', dens=1.4, tone=.5)
    corrugated('wallN', (x1 - 0.004, y1 - ins, 0.035), (-1, 0, 0), UP, (0, 1, 0), x1 - x0 - 0.008, top - 0.045, 'galv', dens=1.4, tone=.2)
    corrugated('wallW', (x0 + ins, y1 - 0.004, 0.035), (0, -1, 0), UP, (-1, 0, 0), y1 - y0 - 0.008, top - 0.045, 'galv', dens=1.4, tone=.6)
    # the fixed strip of the top, behind the hinge, that the stand pipe comes through
    K.bar('strip', (cx, (HINGE[1] + y1) / 2, top), (cx, (HINGE[1] + y1) / 2, top + 0.004), K.sec_rect(x1 - x0 + 0.02, y1 - HINGE[1] + 0.008, 0.002), 'galv', ref=(0, 1, 0), dens=1.4, cap0=(K.LOD < 2), tone=.4)
    if K.LOD < 3:
        zax = place((0, 0, 0), (1, 0, 0), (0, 1, 0), UP)
        n = N(12, 8, 6)
        K.revolve('standpipe', [(0.028, 0.46), (0.028, 1.12), (0.014, 1.12)], zax, n, 'galv', dens=1.3, tone=.8)
        K.revolve('collar', [(0.028, 0.602), (0.045, 0.604), (0.045, 0.624), (0.028, 0.63)], zax, n, 'cast', dens=1.3, tone=.9)
        # the pump on its slab: a flanged foot, the barrel, the gland
        K.revolve('pump', [(0.11, 0.03), (0.11, 0.05), (0.062, 0.058), (0.058, 0.36), (0.085, 0.37), (0.085, 0.40), (0.046, 0.41), (0.046, 0.455), (0.03, 0.47)], zax, N(16, 10, 6), 'cast', dens=1.5, tone=.8)
        K.tube('outlet', [(0.04, 0, 0.21), (0.475, 0, 0.21), (0.475, 0, -0.16)], 0.028, N(10, 6, 4), 'galv', fillet=0.06, arc=N(4, 2, 1), dens=1.2, cap0=False, cap1=False, tone=.9)
        if K.LOD == 0:
            for i in range(4):
                a = math.radians(45 + 90 * i)
                K.bolt('pbolt', (0.088 * math.cos(a), 0.088 * math.sin(a), 0.05), UP, 0.011, 0.012, 'cast', dens=1.3, tone=1.0)
            K.revolve('oflange', [(0.028, 0.0), (0.05, 0.0), (0.05, 0.012), (0.028, 0.012)], place((0.065, 0, 0.21), (0, 1, 0), UP, (1, 0, 0)), 10, 'cast', dens=1.2, tone=.9)
            for s in (-1, 1):                                    # the fixed leaves of the two hinges, and the staple the hasp drops over
                K.bar('hleaf%d' % s, (s * 0.25, HINGE[1] + 0.006, top + 0.0045), (s * 0.25, HINGE[1] + 0.075, top + 0.0045), K.sec_rect(0.035, 0.003), 'cast', ref=UP, dens=1.3, cap0=False, tone=.9)
            K.tube('staple', [(-0.012, y0 - 0.004, top - 0.045), (-0.012, y0 - 0.028, top - 0.045), (0.012, y0 - 0.028, top - 0.045), (0.012, y0 - 0.004, top - 0.045)], 0.0035, 5, 'cast', ref=UP, fillet=0.008, arc=2, cap0=False, cap1=False, tone=1.0)


def build_lid():
    K.target(P['Lid'])
    x0, x1, y0, y1, top = HOUSE
    hy, hz = HINGE[1], HINGE[2]
    fx0, fx1, fy = x0 - 0.02, x1 + 0.02, y0 - 0.02
    z = top + 0.012
    corrugated('lidtop', (fx0 + 0.006, fy + 0.006, z + 0.006), (1, 0, 0), (0, 1, 0), UP, fx1 - fx0 - 0.012, hy - fy - 0.01, 'galv', waves_along='u', amp=0.007, dens=1.4, tone=.45)
    k = N(1, 1, 1.5)
    # an angle frame round three sides (its flange hangs over the box), a flat along the hinge side
    for i, (a, b) in enumerate((((fx0, hy), (fx0, fy)), ((fx0, fy), (fx1, fy)), ((fx1, fy), (fx1, hy)))):
        e = unit((b[0] - a[0], b[1] - a[1], 0))
        outw = cross(e, UP)
        x, y, zz = cross(mul(outw, -1), e), mul(outw, -1), e                    # the section's x up... (heel at the outer top edge)
        sec = [(0, 0), (0.028 * k, 0), (0.028 * k, 0.0035 * k), (0.0035 * k, 0.0035 * k), (0.0035 * k, 0.03), (0, 0.03)]
        xx, yy = mul(outw, -1), (0, 0, -1)
        if K.dot(cross(xx, yy), e) < 0:
            sec = K.mirror2(sec); xx = mul(xx, -1)
            sec = [(-px, py) for px, py in sec][::-1] if False else sec
        pa, pb = (a[0], a[1], z + 0.016), (b[0], b[1], z + 0.016)
        K.sweep('lidframe%d' % i, sec if K.dot(cross(xx, yy), e) > 0 else K.mirror2(sec), [ring(pa, xx, yy), ring(pb, xx, yy)], 'galv', dens=1.4, tone=.75, cap0=(K.LOD < 2), cap1=(K.LOD < 2))
    K.bar('lidback', (fx0, hy + 0.004, z + 0.006), (fx1, hy + 0.004, z + 0.006), K.sec_rect(0.022, 0.004 * k), 'galv', ref=UP, dens=1.3, tone=.7, cap0=(K.LOD < 2), cap1=(K.LOD < 2))
    if K.LOD == 0:
        for s in (-1, 1):                                        # the hinges: a barrel on the axis, a strap along the lid
            K.tube('hbarrel%d' % s, [(s * 0.25 - 0.03, hy, hz), (s * 0.25 + 0.03, hy, hz)], 0.008, 8, 'cast', dens=1.3, tone=1.0)
            K.bar('hstrap%d' % s, (s * 0.25, hy - 0.004, z + 0.0185), (s * 0.25, hy - 0.16, z + 0.0185), K.sec_rect(0.035, 0.003), 'cast', ref=UP, dens=1.3, cap0=False, tone=.9)
        K.bar('hasp', (0, fy - 0.0055, z + 0.02), (0, fy - 0.0055, top - 0.07), K.sec_rect(0.034, 0.003), 'cast', ref=(0, 1, 0), dens=1.3, tone=1.0)
        K.bar('hasptop', (0, fy - 0.004, z + 0.0185), (0, fy + 0.07, z + 0.0185), K.sec_rect(0.034, 0.003), 'cast', ref=UP, dens=1.3, cap1=False, tone=1.0)


def build_tank():
    K.target(P['Tank'])
    T = (TANK[0], TANK[1], 0)
    zax = place(T, (1, 0, 0), (0, 1, 0), UP)
    R, pitch, amp = TANK_R, 0.0675, 0.009
    per = N(6, 4, 0, 0)
    prof = [(R, -0.25), (R, 0.02)]
    if per:
        steps = int(round(0.54 / pitch)) * per
        prof += [(R + amp * math.sin(TAU * i / per), 0.02 + 0.54 * i / steps) for i in range(1, steps + 1)]
    else:
        prof += [(R, 0.56)]
    rr, zc = 0.015, TANK_H - 0.015
    arc = N(5, 3, 1, 1)
    prof += [(R + rr * math.cos(math.pi * i / arc), zc + rr * math.sin(math.pi * i / arc)) for i in range(arc + 1)]
    if per:
        steps = 2 * per
        prof += [(R - 0.004 - amp * math.sin(TAU * i / per), zc - 0.02 - 2 * pitch * i / steps) for i in range(steps + 1)]
    else:
        prof += [(R - 0.006, WATER_Z - 0.04)]

    def stain(p):
        dx, dy = p[0] - T[0], p[1] - T[1]
        r = math.hypot(dx, dy)
        if r < R - 0.0025 and p[2] < zc + 0.004:
            return 2.0                                           # the inside wall
        da = math.atan2(math.sin(math.atan2(dy, dx) - OVER_AZ), math.cos(math.atan2(dy, dx) - OVER_AZ))
        return max(0.0, 1.0 - (da / 0.30) ** 2) * (1.0 if p[2] < 0.50 else 0.25)
    K.revolve('wall', prof, zax, N(64, 48, 24, 16), 'galv', charts=4, dens=0.72, attr=stain, sharp=40)
    if K.LOD < 3:
        # three lapped seams with their bolts
        for i, az in enumerate((math.radians(38), math.radians(162), math.radians(284))):
            er, et = (math.cos(az), math.sin(az), 0), (-math.sin(az), math.cos(az), 0)
            p = add(T, mul(er, R + amp + 0.001))
            K.bar('seam%d' % i, add(p, (0, 0, 0.0)), add(p, (0, 0, 0.57)), K.sec_rect(0.07, 0.005), 'galv', ref=er, dens=0.8, cap0=False, tone=.8)
            if K.LOD == 0:
                for j in range(7):
                    K.bolt('tbolt', add(add(p, mul(er, 0.0025)), add(mul(et, 0.018 * (1 if j % 2 else -1)), (0, 0, 0.06 + 0.075 * j))), er, 0.009, 0.007, 'cast', tone=1.0)
        # the fill line up from the ground at the west side, over the rim to the float valve
        x = T[0] - R - 0.05
        K.tube('riser', [(x, T[1], -0.22), (x, T[1], 0.80), (VALVE[0] - 0.02, T[1], 0.80)], 0.017, N(8, 6, 4), 'galv', fillet=0.05, arc=N(4, 2, 1), dens=1.0, cap0=False, cap1=False, tone=.9)
        K.revolve('valve', [(0.0, 0.69), (0.03, 0.69), (0.036, 0.70), (0.036, 0.83), (0.022, 0.845), (0.022, 0.87), (0.0, 0.87)], place((VALVE[0], VALVE[1], 0), (1, 0, 0), (0, 1, 0), UP), N(12, 8, 6), 'cast', dens=1.2, tone=.9)
        # the overflow: a stub through the wall near the rim, an elbow, and the pipe down the outside
        eo = (math.cos(OVER_AZ), math.sin(OVER_AZ), 0)
        K.tube('overflow', [add(add(T, mul(eo, R - 0.09)), (0, 0, 0.50)), add(add(T, mul(eo, R + 0.07)), (0, 0, 0.50)), add(add(T, mul(eo, R + 0.07)), (0, 0, -0.22))], 0.024, N(10, 6, 4), 'galv',
               fillet=0.05, arc=N(4, 2, 1), dens=1.0, cap1=False, tone=1.0)
        if K.LOD == 0:
            K.bar('ostrap', add(add(T, mul(eo, R + 0.07 + 0.026)), (0, 0, 0.20)), add(add(T, mul(eo, R + 0.07 + 0.026)), (0, 0, 0.235)), K.sec_rect(0.12, 0.004), 'cast', ref=eo, dens=0.8, tone=1.0)


def build_float():
    K.target(P['Float'])
    B = (VALVE[0] + 0.52, VALVE[1], WATER_Z + 0.025)
    K.tube('arm', [VALVE, add(B, (-0.09, 0, 0.03))], 0.005, N(6, 4), 'cast', cap0=False, cap1=False, tone=.6)
    m, r = N(8, 5), 0.10
    K.revolve('ball', [(r * math.sin(math.pi * i / m), -r * math.cos(math.pi * i / m)) for i in range(m + 1)], place(B, (1, 0, 0), (0, 1, 0), UP), N(14, 8), 'galv', dens=1.2, tone=.15, sharp=60)


def build_water():
    K.target(P['Water'])
    K.revolve('water', [(TANK_R - 0.012, WATER_Z), (0.0, WATER_Z)], place((TANK[0], TANK[1], 0), (1, 0, 0), (0, 1, 0), UP), N(48, 24), 'water')


def build_page():
    K.target(P['Page'])
    o = PAGE_AT

    def pt(u, w):
        sag = 0.006 * math.sin(u * math.pi) * (1 - w)                     # it bellies a little toward the eye, low down
        return (o[0] - 0.095 + 0.19 * u, o[1] - 0.02 * (1 - w) - sag, o[2] + 0.17 * w)
    K.sheet('page', pt, 3, 3, 0, 'page', (0.19, 0.17))


def build_wires():
    K.target(P['WireRun'])
    K.tube('run', [(0, 0, 0), (0, 0, 1)], 0.004, 6, 'wire')
    K.target(P['WireKnot'])
    pts = [(0.045 * math.cos(TAU * i / 12), 0.045 * math.sin(TAU * i / 12), -0.022 + 0.011 * i / 12) for i in range(49)]
    pts += [(0.05, 0.02, 0.03), (0.035, 0.05, 0.06)]
    K.tube('knot', pts, 0.004, 5, 'wire', ref=UP)


BUILDERS = {'Tower': build_tower, 'Rod': build_rod, 'Pier': build_pier, 'Head': build_head, 'Wheel': build_wheel, 'Housing': build_housing, 'Lid': build_lid,
            'Tank': build_tank, 'Float': build_float, 'Water': build_water, 'Page': build_page, 'VaneLabel': build_label}


def build_all():
    for lod in range(5):
        K.LOD = lod
        for name, fn in BUILDERS.items():
            if lod < LODS[name]:
                fn()
        if lod == 0:
            build_wires()
            for a in (TOWER_A, TOP_A, BASE_A):
                a.pack()
    K.LOD = 0


def report():
    rows = K.measure(P.values(), (0, 1, 2, 3, 4))
    total = [sum(r['triangles'][l] if l < len(r['triangles']) else r['triangles'][-1] for n, r in rows.items() if n != 'Pier') + 4 * rows['Pier']['triangles'][min(l, 2)] for l in range(5)]
    wheel = P['Wheel'].lods[0].V
    c = P['Wheel'].pivot
    wr = max(math.hypot(v[0] - c[0], v[2] - c[2]) for v in wheel)
    t = P['Tower'].bounds()
    tk = P['Tank'].bounds()
    h = P['Housing'].bounds()
    lid = P['Lid'].bounds()
    facts = {'hub_height_m': HUB_Z, 'wheel_diameter_m': round(2 * wr, 3), 'blades': BLADES, 'tower_base_square_m': round(2 * FOOT, 3),
             'tower_bounds_m': [[round(v, 3) for v in t[0]], [round(v, 3) for v in t[1]]],
             'tank_diameter_m': round(tk[1][0] - tk[0][0], 3), 'tank_rim_height_m': round(tk[1][2], 3),
             'housing_m': [round(h[1][0] - h[0][0], 3), round(h[1][1] - h[0][1], 3), round(lid[1][2], 3)],
             'triangles_per_lod_whole_group': total,
             'atlas': {a.name: {'size': a.size, 'texels_per_m': round(a.scale, 1), 'fill': round(a.fill(), 3), 'charts': len(a.charts), 'missed': len(a.missed)} for a in (TOWER_A, TOP_A, BASE_A)}}
    return rows, facts


build_all()
ROWS, FACTS = report()
if DRY:
    for n, r in ROWS.items():
        print('%-10s tris %-28s min %s max %s' % (n, r['triangles'], r['min_m'], r['max_m']))
    print(json.dumps(FACTS, indent=1))
    sys.exit(0)

# ================================================================ Blender
import bpy
import lpbake as B

for sub_ in ('Export', 'textures', 'Preview'):
    (WORK / sub_).mkdir(parents=True, exist_ok=True)
PH = SRC / 'dl' / 'polyhaven'
B.TEX.update({'galv': Path(os.path.expanduser('~/scratch-bm/refs/textures-20261112/saro/galvanized.png')), 'rust': PH / 'rusty_metal_02_diff_2k.jpg', 'rust2': PH / 'rusty_metal_05_diff_2k.jpg',
              'wood': PH / 'wood_planks_grey_diff_2k.jpg', 'conc': PH / 'rough_concrete_diff_2k.jpg', 'conc2': PH / 'dirty_concrete_diff_2k.jpg'})
ATLAS_OF = {'MillTower': ['Tower', 'Rod'], 'MillTop': ['Head', 'Wheel'], 'MillBase': ['Pier', 'Housing', 'Lid', 'Tank', 'Float']}
B.reset()


def surfaces():
    return {'galv': B.surface('galv', 'galv', rust=0.42, ground=0.45, inner=(TANK[0], TANK[1], TANK_R, WATER_Z)), 'cast': B.surface('cast', 'cast', rust=1.0, ground=0.3),
            'wood': B.surface('wood', 'wood'), 'conc': B.surface('conc', 'conc', ground=0.16)}


def plains():
    lab = SRC / 'tex'
    return {'water': B.plain('M_LP_MillWater', (0.012, 0.016, 0.014), 0.04, 0.0), 'wire': B.plain('M_LP_MillWire', (0.72, 0.74, 0.75), 0.32, 1.0),
            'page': B.pictured('M_LP_MillPage', lab / 'T_LP_MillPage.png', 0.25) if (lab / 'T_LP_MillPage.png').is_file() else B.plain('M_LP_MillPage', (0.8, 0.78, 0.7)),
            'label': B.pictured('M_LP_MillVaneLabel', lab / 'T_LP_MillVaneLabel.png', 0.7, True) if (lab / 'T_LP_MillVaneLabel.png').is_file() else B.plain('M_LP_MillVaneLabel', (0.1, 0.1, 0.1), 0.7, 0.0, 0.6)}


def assembled(mats, final=None, lod=0):
    """Every part where it stands in the model (the four piers too), for a look."""
    obs = []
    for name, part in P.items():
        if name in ('WireRun', 'WireKnot'):
            continue
        geo = part.lods[min(lod, LODS[name] - 1)]
        use = dict(mats)
        if final:
            for k in ('galv', 'cast', 'wood', 'conc'):
                use[k] = final.get(part.atlas.name) or mats.get(k)
        spots = [(sx * FOOT, sy * FOOT, 0) for sx, sy in CORNERS] if name == 'Pier' else [(0, 0, 0)]
        for i, s in enumerate(spots):
            obs.append(B.mesh_object('%s_%d' % (name, i), [(geo, part.atlas, (0, 0, 0), 0)], use, lambda g, s=s: s))
    return obs


def ground():
    me = bpy.data.meshes.new('ground')
    me.from_pydata([(-60, -60, 0), (60, -60, 0), (60, 60, 0), (-60, 60, 0)], [], [(0, 1, 2, 3)])
    ob = bpy.data.objects.new('ground', me)
    B.scn.collection.objects.link(ob)
    me.materials.append(B.plain('dirt', (0.30, 0.24, 0.17), 0.95))
    return ob


def four_views(tag, clay=None, lod=0):
    cam = B.camera(13.0)
    B.daylight(48, 200, 3.0)
    for name, eye in (('front', (2.2, -60, 5.2)), ('left', (-60, -0.2, 5.2)), ('back', (2.2, 60, 5.2)), ('right', (60, -0.2, 5.2))):
        B.look(cam, eye, (eye[0] if abs(eye[1]) > 30 else 0, eye[1] if abs(eye[0]) > 30 else 0, 5.2))
        B.render(WORK / 'Preview' / ('%s_%s.png' % (tag, name)), (1000, 1000), 24, clay)
    bpy.data.objects.remove(cam)
    cam = B.camera()
    for name, eye, at, lens in (('hero', (9.5, -12.5, 4.2), (1.6, 0, 4.4), 35), ('base', (3.3, -4.6, 1.5), (1.2, -0.2, 0.5), 30), ('top', (2.6, -5.5, 6.9), (0.1, 0.3, 9.4), 60)):
        B.look(cam, eye, at, lens)
        B.render(WORK / 'Preview' / ('%s_%s.png' % (tag, name)), (1400, 1000), 32, clay)


if 'clay' in WANT:
    mats = surfaces()
    mats.update(plains())
    for lod in ((0, 2, 3) if 'lods' in WANT else (0,)):
        obs = assembled(mats, None, lod)
        g = ground()
        four_views('clay' if lod == 0 else 'clay_lod%d' % lod, B.plain('clay', (0.55, 0.55, 0.55), 0.8))
        for o in list(bpy.data.objects):
            bpy.data.objects.remove(o)
    print('S47 LP: clay pictures in', WORK / 'Preview', flush=True)

FINAL = {}
if 'bake' in WANT:
    B.reset()
    mats = surfaces()
    mats.update(plains())
    only = [a for a in WANT if a in ATLAS_OF] or list(ATLAS_OF)
    for a in (TOWER_A, TOP_A, BASE_A):
        if a.name not in only:
            continue
        rows = [(P[n].lods[0], a, (0, 0, 0), P[n].group) for n in ATLAS_OF[a.name]]
        ob = B.mesh_object('bake_' + a.name, rows, mats, lambda g: (g * 40.0, 0, 0))
        floors = []
        for n in ATLAS_OF[a.name]:
            if n in ('Tower', 'Pier', 'Housing', 'Tank'):
                me = bpy.data.meshes.new('floor')
                x = P[n].group * 40.0
                me.from_pydata([(x - 12, -12, 0), (x + 12, -12, 0), (x + 12, 12, 0), (x - 12, 12, 0)], [], [(0, 1, 2, 3)])
                fo = bpy.data.objects.new('floor', me)
                B.scn.collection.objects.link(fo)
                floors.append(fo)
        B.bake(ob, a.name, a.size, WORK / 'textures', 'M_LP_' + a.name)
        for o in [ob] + floors:
            bpy.data.objects.remove(o)
    print('S47 LP: baked', only, flush=True)


def finals():
    out = {}
    for a in ATLAS_OF:
        files = {}
        for suffix, space in (('BC', 'sRGB'), ('N', 'Non-Color'), ('ORM', 'Non-Color')):
            p = WORK / 'textures' / ('T_LP_%s_%s.png' % (a, suffix))
            if not p.is_file():
                return None
            files[suffix] = bpy.data.images.load(str(p), check_existing=True)
            files[suffix].colorspace_settings.name = space
        out[a] = B.baked_material('M_LP_' + a, files)
    return out


if 'export' in WANT or 'look' in WANT:
    B.reset()
    FINAL = finals()
    mats = plains()
    for k in ('galv', 'cast', 'wood', 'conc'):
        mats[k] = None
    if 'export' in WANT:
        for name, part in P.items():
            use = dict(mats)
            for k in ('galv', 'cast', 'wood', 'conc'):
                use[k] = (FINAL or {}).get(part.atlas.name) or bpy.data.materials.get('M_LP_' + part.atlas.name) or bpy.data.materials.new('M_LP_' + part.atlas.name)
            for lod in range(LODS[name]):
                ob = B.mesh_object('SM_LP_Mill_' + name, [(part.lods[lod], part.atlas, part.pivot, 0)], use)
                B.export(ob, WORK / 'Export' / ('SM_LP_Mill_%s%s.fbx' % (name, '' if lod == 0 else '_LOD%d' % lod)))
                bpy.data.objects.remove(ob)
            print('S47 LP: exported', name, ROWS[name]['triangles'], flush=True)
    if 'look' in WANT and FINAL:
        use = plains()
        obs = assembled(use, FINAL, 0)
        ground()
        four_views('look')

contract = {'facts': FACTS, 'parts': ROWS, 'lods': LODS,
            'placing_m': {'hub_z': HUB_Z, 'hub_off': HUB_OFF, 'pier_top': PIER_TOP, 'foot': FOOT, 'house': HOUSE, 'hinge': HINGE, 'tank': TANK, 'tank_r': TANK_R, 'tank_h': TANK_H,
                          'water_z': WATER_Z, 'valve': VALVE, 'page': PAGE_AT, 'stroke': STROKE, 'mast_top': MAST_TOP},
            'slots': {'MillTower': 'M_LP_MillTower', 'MillTop': 'M_LP_MillTop', 'MillBase': 'M_LP_MillBase', 'plain': None}}
(WORK / 'contract.json').write_text(json.dumps(contract, indent=1, sort_keys=True))
print('DONE', WANT, flush=True)
