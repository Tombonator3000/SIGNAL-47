"""Felles byggeklosser for gårdsbyggene. Ingen Blender-avhengighet.

Bygger videre på meshkit fra substation (samme oppskrift som townbuildings). Meter, Blender-akser:
X øst, Y nord, Z opp, nullpunkt midt i fotavtrykket på bakkenivå. Hver flate har egne hjørner,
UV-er i ekte meter delt på materialets tile og en fargefaktor som blir vertex color.
"""
import importlib.util
import math
import random
from pathlib import Path

_p = Path(__file__).resolve().parent.parent / 'substation/meshkit.py'
_s = importlib.util.spec_from_file_location('s47_farm_base_meshkit', _p)
base = importlib.util.module_from_spec(_s)
_s.loader.exec_module(base)
add, sub, mul, dot, cross, unit = base.add, base.sub, base.mul, base.dot, base.cross, base.unit

# Meter per teksturrepetisjon. Må stemme med textures.py.
TILE = {'Board': 2.4, 'Tin': 3.2, 'Stucco': 6.0, 'Wood': 1.6, 'Paint': 1.2, 'Concrete': 2.0,
        'Steel': 1.0, 'Glass': 2.0, 'WindowLit': 1.0, 'Dirt': 3.0}
ORDER = list(TILE)
X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)
# Veggene fortsetter så langt under bakken. Terrenget under et fotavtrykk varierer opptil 0,23 m, se README.
WALL_BASE = -.5
# Skitten stripe i selve veggmaterialet: 13 cm, så en mykere overgang opp til 75 cm. Flatene kuttes her i finish().
STRIPE, FADE = .13, .75


def smoothstep(a, b, v):
    t = max(0, min(1, (v - a) / (b - a)))
    return t * t * (3 - 2 * t)


class Kit(base.Mesh):
    def __init__(self, name, lod, seed=47):
        super().__init__(name)
        self.lod = lod
        self.rng = random.Random(seed * 10 + lod)
        self.tint = []
        self.ambient = 1.0      # Mørkere innendørs og under tak, som enkel bakt skygge.
        self.solids = []        # Plan-rektangler som stenger spilleren (lokale meter).

    # --- flater ---
    def face(self, ps, mat, uv=None, smooth=False, tint=(1, 1, 1), n=None):
        ps = list(ps)
        if uv is None:
            nn = cross(sub(ps[1], ps[0]), sub(ps[2], ps[0]))
            axis = max(range(3), key=lambda k: abs(nn[k]))
            a, b = ([1, 2] if axis == 0 else [0, 2] if axis == 1 else [0, 1])
            t = TILE[mat]
            uv = [(p[a] / t, p[b] / t) for p in ps]
        else:
            uv = list(uv)
        if n is not None:
            nn = cross(sub(ps[1], ps[0]), sub(ps[2], ps[0]))
            if dot(nn, n) < 0:
                ps.reverse()
                uv.reverse()
        super().face(ps, mat, uv, smooth)
        self.mat[-1] = self.mat[-1].replace('M_Substation_', 'M_Farm_', 1)
        self.tint.append(tuple(c * self.ambient for c in tint))

    def uvq(self, ps, mat, ua, va, off=(0, 0), tint=(1, 1, 1), n=None, smooth=False):
        """Flate med UV langs to retninger i meter. Korn og bølger følger va."""
        t = TILE[mat]
        self.face(ps, mat, [(dot(p, ua) / t + off[0], dot(p, va) / t + off[1]) for p in ps],
                  smooth, tint, n)

    # --- bokser og bjelker ---
    def obox(self, name, c, ax, ay, az, s, mat, grain=2, skip=(), tint=(1, 1, 1)):
        """Orientert boks. ax, ay, az er enhetsakser, s størrelser. Korn langs akse nummer grain."""
        self.tag = name
        axes = [ax, ay, az]
        off = (self.rng.random() * 3, self.rng.random() * 3)
        if mat == 'Wood':
            # Hver bjelke og stolpe har sin egen alder: noen blekere, noen mørkere.
            w = .84 + self.rng.random() * .34
            grey = self.rng.random() * .1
            tint = (tint[0] * w, tint[1] * w * (1 + grey * .2), tint[2] * w * (1 + grey))
        for k in range(3):
            for sign in (-1, 1):
                if (k, sign) in skip:
                    continue
                i, j = [q for q in range(3) if q != k]
                n = mul(axes[k], sign)
                c0 = add(c, mul(axes[k], sign * s[k] / 2))
                ai, aj = mul(axes[i], s[i] / 2), mul(axes[j], s[j] / 2)
                ps = [add(c0, add(mul(ai, a), mul(aj, b))) for a, b in [(-1, -1), (1, -1), (1, 1), (-1, 1)]]
                if i == grain:
                    self.uvq(ps, mat, axes[j], axes[i], off, tint, n)
                else:
                    self.uvq(ps, mat, axes[i], axes[j], off, tint, n)

    def bx(self, name, x0, x1, y0, y1, z0, z1, mat, grain=2, skip=(), tint=(1, 1, 1)):
        self.obox(name, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), X, Y, Z,
                  (x1 - x0, y1 - y0, z1 - z0), mat, grain, skip, tint)

    def beam(self, name, a, b, w, h, mat='Wood', up=Z, tint=(1, 1, 1), skip=()):
        axis = unit(sub(b, a))
        if abs(dot(axis, up)) > .95:
            up = Y
        r = unit(cross(axis, up))
        n = cross(r, axis)
        ln = math.sqrt(dot(sub(b, a), sub(b, a)))
        self.obox(name, mul(add(a, b), .5), axis, r, n, (ln, w, h), mat, 0, skip, tint)

    def pipe(self, name, a, b, r, mat='Steel', n=None, tint=(1, 1, 1)):
        self.tag = name
        n = n or [10, 6, 4][self.lod]
        axis = unit(sub(b, a))
        rr = unit(cross(axis, Z if abs(axis[2]) < .9 else Y))
        up = cross(axis, rr)
        t = TILE[mat]
        ln = math.sqrt(dot(sub(b, a), sub(b, a)))
        rings = [[add(c, add(mul(rr, r * math.cos(2 * math.pi * i / n)), mul(up, r * math.sin(2 * math.pi * i / n))))
                  for i in range(n)] for c in (a, b)]
        for i in range(n):
            j = (i + 1) % n
            mid = mul(add(rings[0][i], rings[0][j]), .5)
            uv = [(i * r * 6.28 / n / t, 0), (j * r * 6.28 / n / t if j else r * 6.28 / t, 0),
                  (j * r * 6.28 / n / t if j else r * 6.28 / t, ln / t), (i * r * 6.28 / n / t, ln / t)]
            self.face([rings[0][i], rings[0][j], rings[1][j], rings[1][i]], mat, uv, True, tint, sub(mid, a))
        self.face(rings[1], mat, None, False, tint, axis)
        self.face(rings[0], mat, None, False, tint, mul(axis, -1))

    def rough_log(self, name, a, b, r, mat='Wood', tint=(1, 1, 1)):
        """Rund viga med litt ujevn form."""
        self.pipe(name, a, b, r, mat, [8, 5, 4][self.lod], tint)

    def rotate_since(self, start, center, axis, angle):
        """Rotér alt lagt til siden hjørneindeks start, f.eks. en dør som henger skjevt."""
        ax = unit(axis)
        c, s = math.cos(angle), math.sin(angle)
        for i in range(start, len(self.v)):
            p = sub(self.v[i], center)
            d = dot(ax, p)
            r = add(add(mul(p, c), mul(cross(ax, p), s)), mul(ax, d * (1 - c)))
            self.v[i] = add(r, center)

    def blob(self, F, u, z, ru, rz, d, mat, tint, n=9):
        """Ujevn flekk flatt på veggen: lapp i puss eller sement."""
        self.tag = 'patch'
        ang = [2 * math.pi * i / n for i in range(n)]
        pts = [F.P(u + ru * (.72 + self.rng.random() * .4) * math.cos(a), z + rz * (.72 + self.rng.random() * .4) * math.sin(a), d) for a in ang]
        self.uvq(pts, mat, F.du, Z, (self.rng.random() * 3, self.rng.random() * 3), tint, F.dd)

    def streak(self, F, u, z, w, ln, d, mat='Stucco', tint=(.9, .87, .83)):
        """Avrenning: smal, spiss stripe nedover fra en sålbenk eller et rør."""
        self.tag = 'streak'
        lean = (self.rng.random() - .5) * .06
        pts = [F.P(u - w / 2, z, d), F.P(u + lean, z - ln, d), F.P(u + w / 2, z, d)]
        pts = [pts[0], pts[1], pts[2]]
        self.uvq(pts, mat, F.du, Z, (self.rng.random() * 3, self.rng.random() * 3), tint, F.dd)

    def solid(self, x0, x1, y0, y1):
        self.solids.append((x0, x1, y0, y1))


class Frame:
    """Koordinater langs en vegg: u langs a til b, z opp, d ut fra veggen.

    Gå rundt bygget mot klokka sett ovenfra, så peker d ut: sørvegg vest til øst, østvegg sør til
    nord, nordvegg øst til vest, vestvegg nord til sør.
    """

    def __init__(self, kit, a, b):
        self.k, self.a = kit, a
        dx, dy = b[0] - a[0], b[1] - a[1]
        self.L = math.hypot(dx, dy)
        self.du = (dx / self.L, dy / self.L, 0)
        self.dd = (self.du[1], -self.du[0], 0)

    def P(self, u, z, d=0):
        return (self.a[0] + self.du[0] * u + self.dd[0] * d, self.a[1] + self.du[1] * u + self.dd[1] * d, z)

    def box(self, name, u0, u1, z0, z1, d0, d1, mat, grain=2, skip=(), tint=(1, 1, 1)):
        """Boks i veggkoordinater. grain 0: korn langs veggen, 2: loddrett."""
        c = self.P((u0 + u1) / 2, (z0 + z1) / 2, (d0 + d1) / 2)
        self.k.obox(name, c, self.du, self.dd, Z, (u1 - u0, d1 - d0, z1 - z0), mat, grain, skip, tint)

    def quad(self, name, u0, u1, z0, z1, d, mat, tint=(1, 1, 1), grain=2, off=None):
        self.k.tag = name
        o = off or (self.k.rng.random() * 3, self.k.rng.random() * 3)
        ps = [self.P(u0, z0, d), self.P(u1, z0, d), self.P(u1, z1, d), self.P(u0, z1, d)]
        if grain == 2:
            self.k.uvq(ps, mat, self.du, Z, o, tint, self.dd)
        else:
            self.k.uvq(ps, mat, Z, self.du, o, tint, self.dd)


def _ztop(poly, u):
    for (u0, z0), (u1, z1) in zip(poly, poly[1:]):
        if u0 <= u <= u1:
            return z0 + (z1 - z0) * (u - u0) / max(u1 - u0, 1e-9)
    return poly[-1][1]


def wall(k, name, a, b, top, holes=(), t=.12, base=0., mat='Board', imat=None, rmat=None,
         rows=(0,), inner=True, itint=(1, 1, 1), trim=(0, 0), v0=None):
    """Vegg med ekte hull. top er høyde eller [(u, z), ...]. holes er (u0, u1, z0, z1)."""
    F = Frame(k, a, b)
    k.tag = name
    imat, rmat = imat or mat, rmat or mat
    L = F.L
    poly = [(0, top), (L, top)] if isinstance(top, (int, float)) else list(top)
    ucuts = []
    for c in sorted({0, L} | {p[0] for p in poly} | {h[0] for h in holes} | {h[1] for h in holes}):
        if not ucuts or c - ucuts[-1] > 1e-5:
            ucuts.append(min(c, L))
    poly[-1] = (L, poly[-1][1])
    zcuts = sorted({base} | {r for r in rows if r > base} | {h[2] for h in holes} | {h[3] for h in holes})
    off = (k.rng.random() * 5, k.rng.random() * 5 if v0 is None else v0)
    ioff = (k.rng.random() * 5, k.rng.random() * 5)
    tm, tim = TILE[mat], TILE[imat]
    for u0, u1 in zip(ucuts, ucuts[1:]):
        a0, a1 = _ztop(poly, u0), _ztop(poly, u1)
        lo = min(a0, a1)
        levels = [z for z in zcuts if z < lo - 1e-6]
        for i, z in enumerate(levels):
            last = i + 1 == len(levels)
            z1 = lo if last else levels[i + 1]
            um, zm = (u0 + u1) / 2, (z + z1) / 2
            if any(h[0] < um < h[1] and h[2] < zm < h[3] for h in holes):
                continue
            t0, t1 = (a0, a1) if last else (z1, z1)
            ps = [F.P(u0, z), F.P(u1, z), F.P(u1, t1), F.P(u0, t0)]
            k.face(ps, mat, [(p_u / tm + off[0], p_z / tm + off[1]) for p_u, p_z in
                             [(u0, z), (u1, z), (u1, t1), (u0, t0)]], False, (1, 1, 1), F.dd)
            # Innsiden under gulvet er aldri synlig.
            if inner and z1 > 1e-6:
                ia, ib = max(u0, trim[0]), min(u1, L - trim[1])
                if ib - ia > 1e-4:
                    ta = t0 + (t1 - t0) * (ia - u0) / (u1 - u0)
                    tb = t0 + (t1 - t0) * (ib - u0) / (u1 - u0)
                    ps = [F.P(ia, z, -t), F.P(ib, z, -t), F.P(ib, tb, -t), F.P(ia, ta, -t)]
                    k.face(ps, imat, [(p_u / tim + ioff[0], p_z / tim + ioff[1]) for p_u, p_z in
                                      [(ia, z), (ib, z), (ib, tb), (ia, ta)]], False, itint, mul(F.dd, -1))
    # Lysninger: veggens tykkelse synes i hvert hull.
    tr = TILE[rmat]
    for u0, u1, z0, z1 in holes:
        o = (k.rng.random() * 3, k.rng.random() * 3)
        for u, sign in ((u0, 1), (u1, -1)):
            ps = [F.P(u, z0, 0), F.P(u, z0, -t), F.P(u, z1, -t), F.P(u, z1, 0)]
            k.face(ps, rmat, [(d / tr + o[0], zz / tr + o[1]) for d, zz in [(0, z0), (t, z0), (t, z1), (0, z1)]],
                   False, (1, 1, 1), mul(F.du, sign))
        if z1 < lo + 99:
            ps = [F.P(u0, z1, 0), F.P(u1, z1, 0), F.P(u1, z1, -t), F.P(u0, z1, -t)]
            k.face(ps, rmat, None, False, (.8, .8, .8), (0, 0, -1))
        if z0 > base + 1e-6:
            ps = [F.P(u0, z0, 0), F.P(u1, z0, 0), F.P(u1, z0, -t), F.P(u0, z0, -t)]
            k.face(ps, rmat, None, False, (1, 1, 1), (0, 0, 1))
    return F


def battens(k, F, name, u0, u1, step, top, holes=(), base=0., w=.07, h=.022, mat='Board', spread=.12):
    """Loddrette dekklister med ujevn rytme, noen bredere bord som står ut og noen som mangler."""
    poly = [(0, top), (F.L, top)] if isinstance(top, (int, float)) else list(top)
    u = u0 + step * .4
    while u < u1:
        uj = u
        tone = 1 + (k.rng.random() - .5) * spread * 2
        roll = k.rng.random()
        ww, hh = w, h
        if roll < .06:          # Dekklist som har falt av.
            u += step * (.75 + k.rng.random() * .5)
            continue
        if roll < .24:          # Utskiftet eller bredere bord som står litt ut.
            ww, hh = .12 + k.rng.random() * .18, .014 + k.rng.random() * .016
            tone *= .78 + k.rng.random() * .42
        ztop = min(_ztop(poly, uj - ww / 2), _ztop(poly, uj + ww / 2)) - .02
        spans = [(base, ztop)]
        if roll > .9:           # Kort list som ikke når helt ned.
            spans = [(max(base, 0) + .25 + k.rng.random() * .5, ztop)]
        for h0, h1, h2, h3 in holes:
            if h0 - ww / 2 < uj < h1 + ww / 2:
                new = []
                for a, b in spans:
                    if h2 > a:
                        new.append((a, min(b, h2)))
                    if h3 < b:
                        new.append((max(a, h3), b))
                spans = new
        for a, b in spans:
            if b > 0 and b - a > .05 and uj - ww / 2 > 0 and uj + ww / 2 < F.L:
                start = len(k.v)
                F.box(name, uj - ww / 2, uj + ww / 2, a, b, 0, hh, mat, 2, ((1, -1),), (tone, tone, tone * .97))
                if k.lod == 0 and .2 < roll < .34 and b - a > 1:   # Bord som har sklidd litt.
                    k.rotate_since(start, F.P(uj, (a + b) / 2, 0), F.dd, (k.rng.random() - .5) * .018)
        u += step * (.72 + k.rng.random() * .56)


def chamfer(k, x, y, z0, z1, w, mat='Stucco', sx=1, sy=1):
    """Avrundet hjørne: en smal skrå flate over hjørnet, med to flater inn i veggene."""
    k.tag = 'chamfer'
    a0 = (x - sx * w, y, z0)
    a1 = (x, y - sy * w, z0)
    b1 = (x, y - sy * w, z1)
    b0 = (x - sx * w, y, z1)
    d = .012
    n = (sx, sy, 0)
    o = (sx * d, sy * d, 0)
    k.uvq([add(a0, o), add(a1, o), add(b1, o), add(b0, o)], mat, (sx * -.7, sy * .7, 0), Z, (k.rng.random(), 0), (1, 1, 1), n)


def roof(k, name, e0, e1, r1, r0, strip=.84, rust=.35, under=True, under_tint=.3, eave_lip=True):
    """Takplan fra raft e0-e1 til møne r0-r1, delt i plater med egen slitasje."""
    k.tag = name
    along = sub(e1, e0)
    L = math.sqrt(dot(along, along))
    ua = unit(along)
    slope = unit(sub(r0, e0))
    n = unit(cross(ua, slope))
    count = max(1, int(round(L / strip)))
    for i in range(count):
        t0, t1 = i / count, (i + 1) / count
        lift = mul(n, .014 if i % 2 else 0)
        ps = [add(add(e0, mul(ua, L * t0)), lift), add(add(e0, mul(ua, L * t1)), lift),
              add(add(r0, mul(ua, L * t1)), lift), add(add(r0, mul(ua, L * t0)), lift)]
        rusty = k.rng.random() < rust
        w = k.rng.random()
        roll = k.rng.random()
        if roll < .12:          # Nyere plate, byttet senere.
            tint = (1.14 + .08 * w, 1.13 + .08 * w, 1.09 + .08 * w)
        elif rusty:             # Eldre plate som drar mot rustbrunt.
            tint = (.97 + .05 * w, .86 + .07 * w, .76 + .08 * w)
        else:
            tint = (.92 + .1 * w, .91 + .1 * w, .88 + .1 * w)
        # V = 0 ved raftet, med litt ulik platelengde. Da havner rusten nederst på hver plate.
        v_eave = dot(e0, slope) / TILE['Tin']
        off = (k.rng.randrange(7) * .25, -v_eave + (k.rng.random() - .5) * .06)
        if k.lod == 0 and k.rng.random() < .07:    # Løs plate: hjørnet ved raftet har løftet seg.
            ps[0] = add(ps[0], mul(n, .035))
        k.uvq(ps, 'Tin', ua, slope, off, tint, n)
        if k.lod == 0 and i % 2:
            for edge in (t0, t1):
                a = add(e0, mul(ua, L * edge))
                b = add(r0, mul(ua, L * edge))
                k.face([a, b, add(b, lift), add(a, lift)], 'Tin', [(0, 0), (0, 3), (.01, 3), (.01, 0)], False,
                       tint, mul(ua, -1 if edge == t0 else 1))
    if k.lod == 0 and eave_lip:
        # Ekte bølger ytterst på raftet, så blikket leses fra bakken.
        pitch, depth, lip, amp = .076, 4, .075, .011
        count = int(L / pitch * depth)
        down = mul(slope, -lip)
        v0 = -lip / TILE['Tin']
        for i in range(count):
            s0, s1 = L * i / count, L * (i + 1) / count
            z0 = mul(n, amp * math.sin(s0 / pitch * 2 * math.pi) + .018)
            z1 = mul(n, amp * math.sin(s1 / pitch * 2 * math.pi) + .018)
            a, b = add(e0, mul(ua, s0)), add(e0, mul(ua, s1))
            ps = [add(add(a, down), z0), add(add(b, down), z1), add(add(b, mul(slope, .02)), z1), add(add(a, mul(slope, .02)), z0)]
            uv = [(s0 / TILE['Tin'], .02), (s1 / TILE['Tin'], .02), (s1 / TILE['Tin'], .05), (s0 / TILE['Tin'], .05)]
            k.face(ps, 'Tin', uv, True, (.9, .84, .78), n)
            under_ps = [add(p, mul(n, -.004)) for p in ps]
            k.face(under_ps, 'Tin', uv, True, (under_tint, under_tint, under_tint), mul(n, -1))
    if under:
        d = mul(n, -.015)
        amb = k.ambient
        k.ambient = 1
        ps = [add(e0, d), add(e1, d), add(r1, d), add(r0, d)]
        k.uvq(ps, 'Tin', ua, slope, (.3, .1), (under_tint,) * 3, mul(n, -1))
        k.ambient = amb


def lamp(k, F, u, z, out=.55):
    """Gåsehalslampe på vegg, bare nær."""
    if k.lod:
        return
    a, b = F.P(u, z, 0), F.P(u, z + .28, out * .5)
    c = F.P(u, z + .26, out)
    k.pipe('lampArm', a, b, .018, 'Steel', 6)
    k.pipe('lampArm', b, c, .018, 'Steel', 6)
    k.lathe('lampShade', add(c, (0, 0, -.2)), [(0, .19), (.04, .17), (.17, .05), (.2, .02)], 'Steel', 10, caps=True)
    k.tint.extend([(.8, .78, .72)] * (len(k.f) - len(k.tint)))


def _cut(ps, uv, z):
    """Del en konveks flate med et vannrett plan. Gir delen under og delen over, med UV-er interpolert."""
    lo, hi = ([], []), ([], [])
    for i, p in enumerate(ps):
        q, tp, tq = ps[(i + 1) % len(ps)], uv[i], uv[(i + 1) % len(ps)]
        for side, ok in ((lo, p[2] <= z), (hi, p[2] >= z)):
            if ok:
                side[0].append(p)
                side[1].append(tp)
        if (p[2] - z) * (q[2] - z) < 0 and abs(p[2] - z) > 1e-7 and abs(q[2] - z) > 1e-7:
            t = (z - p[2]) / (q[2] - p[2])
            m = (p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t, z)
            mt = (tp[0] + (tq[0] - tp[0]) * t, tp[1] + (tq[1] - tp[1]) * t)
            for side in (lo, hi):
                side[0].append(m)
                side[1].append(mt)
    return lo, hi


def slice_grime(k):
    """Vertex color interpoleres over flaten. Kutt alt som krysser stripen, så vegg, list, stolpe og karm får samme stripe."""
    cuts = (0, STRIPE, FADE) if k.lod < 2 else (0, STRIPE)
    v, f, uv, mat, smooth, tags, tint = [], [], [], [], [], [], []
    for i, ids in enumerate(k.f):
        pieces = [([k.v[n] for n in ids], list(k.uv[i]))]
        if grime_of(k.mat[i]):
            for z in cuts:
                nxt = []
                for ps, ts in pieces:
                    zs = [p[2] for p in ps]
                    if min(zs) < z - 1e-4 and max(zs) > z + 1e-4:
                        for side in _cut(ps, ts, z):
                            if len(side[0]) >= 3:
                                nxt.append(side)
                    else:
                        nxt.append((ps, ts))
                pieces = nxt
        for ps, ts in pieces:
            start = len(v)
            v.extend(ps)
            f.append(list(range(start, start + len(ps))))
            uv.append(ts)
            mat.append(k.mat[i])
            smooth.append(k.smooth[i])
            tags.append(k.tags[i])
            tint.append(k.tint[i])
    k.v, k.f, k.uv, k.mat, k.smooth, k.tags, k.tint = v, f, uv, mat, smooth, tags, tint


def finish(k):
    """Lathe og andre arvede metoder legger flater uten fargefaktor. Fyll inn til slutt, og kutt for skittstripen."""
    k.tint.extend([(1, 1, 1)] * (len(k.f) - len(k.tint)))
    slice_grime(k)
    return k


def grime_of(mat):
    return mat not in ('M_Farm_Glass', 'M_Farm_WindowLit', 'M_Farm_Dirt', 'M_Farm_Steel', 'M_Farm_Concrete')


def grime(mat, p):
    """Skitt nederst på vegger: mørk, smal stripe mot bakken, ingen effekt på glass og stål."""
    if mat == 'M_Farm_Concrete':
        return .82 + .18 * smoothstep(-.02, .22, p[2])
    if not grime_of(mat):
        return 1.0
    low, mid = (.52, .82) if mat == 'M_Farm_Tin' else (.4, .76)
    z = p[2]
    if z <= 0:
        return low
    if z <= STRIPE:
        return low + (mid - low) * z / STRIPE
    if z <= FADE:
        return mid + (1 - mid) * (z - STRIPE) / (FADE - STRIPE)
    return 1.0


def earth(g):
    """Faktoren som farge: mørkere jord er varmere, ikke grå."""
    d = 1 - g
    return (g, g * (1 - .12 * d), g * (1 - .26 * d))


def triangles(k):
    return sum(len(f) - 2 for f in k.f)


def mats_used(k):
    used = set(k.mat)
    return ['M_Farm_' + m for m in ORDER if 'M_Farm_' + m in used]


def ucx_box(x0, x1, y0, y1, z0, z1):
    return [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]


UCX_FACES_BOX = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]


def wall_stains(k, F, spots, d=.004):
    """Avrenning og lapper på veggen, bare nær. spots: (u, z, bredde, lengde)."""
    if k.lod:
        return
    for u, z, w, ln in spots:
        k.streak(F, u, z, w, ln, d)
