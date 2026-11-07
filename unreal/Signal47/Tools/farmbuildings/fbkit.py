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
# Bølgeblikkets ribber leses på tvers: 11 ribber per tekstur skal gi 8 cm periode, langs platen beholder den 3,2 m.
TILE_U = {'Tin': .88}
ORDER = list(TILE)
# Lineær faktor på BC i materialet, målt mot spillets sol (7. november). Samme tall i import_assets.py MATERIALS.
ALBEDO = {'Board': (.58, .58, .58), 'Tin': (.66, .66, .66), 'Stucco': (.88, .88, .88), 'Wood': (.9, .9, .9), 'Paint': (.8, .8, .8), 'Concrete': (.62, .62, .62),
          'Steel': (.8, .8, .8), 'Glass': (7.5, 7.8, 8.4), 'WindowLit': (1, 1, 1), 'Dirt': (.48, .46, .44)}
# Jord i bunnen av veggen: vertex alpha sier hvor mye. Albedo til sRGB (74,50,34), som leses omtrent (105,70,50) i sol.
EARTH_COLOR = (.069, .032, .016)
X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)
# Veggene fortsetter så langt under bakken. Terrenget under et fotavtrykk varierer opptil 0,23 m, se README.
WALL_BASE = -.5
# Skitten stripe i selve veggmaterialet: 13 cm, så en mykere overgang opp til 75 cm. Flatene kuttes her i finish().
STRIPE, FADE = .13, .6
BOARD_W = (.20, .25)
EAVE_BAND = .45             # Skyggebåndet under takskjegget.
EAVE_COL = (.5, .47, .44)
RUN_COL = (.62, .56, .48)   # Avrenning fra takkanten.
RUST_COL = (.86, .62, .42)  # Rust nedover fra mønet.


def smoothstep(a, b, v):
    t = max(0, min(1, (v - a) / (b - a)))
    return t * t * (3 - 2 * t)


def wob(p):
    """Jevn ujevnhet 0 til 1 etter plass: fukt og støv følger ikke en rett linje."""
    return .5 + .25 * math.sin(p[0] * 3.1 + p[1] * 2.3) + .25 * math.sin(p[0] * 7.7 - p[1] * 5.9)


class Kit(base.Mesh):
    def __init__(self, name, lod, seed=47):
        super().__init__(name)
        self.lod = lod
        self.rng = random.Random(seed * 10 + lod)
        self.tint = []
        self.ambient = 1.0      # Mørkere innendørs og under tak, som enkel bakt skygge.
        self.solids = []        # Plan-rektangler som stenger spilleren (lokale meter).
        self.tone = {}          # Fargefaktor per materiale for hele bygget.
        self.fx = {}            # Flatenummer til [(farge, styrke, ztopp, zlengde)], regnes per hjørne etter z.
        self.cur_fx = None

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
        tone = self.tone.get(mat)
        if tone:
            tint = tuple(a * b for a, b in zip(tint, tone))
        self.tint.append(tuple(c * self.ambient for c in tint))
        if self.cur_fx:
            self.fx[len(self.f) - 1] = self.cur_fx

    def uvq(self, ps, mat, ua, va, off=(0, 0), tint=(1, 1, 1), n=None, smooth=False):
        """Flate med UV langs to retninger i meter. Korn og bølger følger va."""
        t, tu = TILE[mat], TILE_U.get(mat, TILE[mat])
        self.face(ps, mat, [(dot(p, ua) / tu + off[0], dot(p, va) / t + off[1]) for p in ps],
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


class Column:
    """Ett bord, eller en loddrett stripe av pussen: egen UV, tone og avrenning."""

    def __init__(self, u0, u1, ou, ov, tone, run, ln):
        self.u0, self.u1, self.ou, self.ov, self.tone, self.run, self.ln = u0, u1, ou, ov, tone, run, ln


def columns(k, mat, length):
    """Bord 0,20 til 0,25 m med egen UV og tone (8 prosent). Pussen deles bare, så fukt og avrenning får hjørner."""
    out, u = [], 0.
    wide = 1 if k.lod == 0 else 2.5
    while u < length - 1e-3:
        if mat == 'Board':
            w = (BOARD_W[0] + k.rng.random() * (BOARD_W[1] - BOARD_W[0])) * wide
            g = .92 + (k.rng.random() - .5) * .16
            tone = (g, g * (.985 + k.rng.random() * .015), g * (.96 + k.rng.random() * .04))
            ou, ov = k.rng.random() * 5, k.rng.random() * 5
            run = k.rng.random() ** 2.2 * .95
        else:
            w = .7 * (1 if k.lod == 0 else 2.2)
            tone, ou, ov = (1, 1, 1), None, None
            run = k.rng.random() ** 3 * .5
        if length - u - w < w * .5:
            w = length - u
        out.append(Column(u, u + w, ou, ov, tone, run, .9 + k.rng.random() * 1.5))
        u += w
    return out


def emit_columns(k, F, cols, mat, u0, u1, z, t0, t1, poly, d=0., off=(0, 0), weather=True, name=None):
    """Vegg- eller dørflaten fra u0 til u1 og z til toppen (t0, t1), stripe for stripe."""
    tm, tu = TILE[mat], TILE_U.get(mat, TILE[mat])
    for c in cols:
        a, b = max(u0, c.u0), min(u1, c.u1)
        if b - a < 1e-4:
            continue
        ta = t0 + (t1 - t0) * (a - u0) / (u1 - u0)
        tb = t0 + (t1 - t0) * (b - u0) / (u1 - u0)
        top = _ztop(poly, (a + b) / 2)
        fx = None
        if weather:
            fx = [(EAVE_COL, 1., top, EAVE_BAND)]
            if c.run > .05:
                fx.append((RUN_COL, c.run, top, c.ln))
        low = min(ta, tb)
        rows = [z]
        if weather:
            rows += sorted(s for s in {top - EAVE_BAND, top - c.ln} if z + .03 < s < low - .03)
        ou, ov = (off if c.ou is None else (c.ou, c.ov))
        k.cur_fx = fx
        if name:
            k.tag = name
        for i, r0 in enumerate(rows):
            last = i + 1 == len(rows)
            r1 = rows[i + 1] if not last else None
            pa, pb = (ta, tb) if last else (r1, r1)
            corners = [(a, r0), (b, r0), (b, pb), (a, pa)]
            uv = [((u - (0 if c.ou is None else c.u0)) / tu + ou, v / tm + ov) for u, v in corners]
            k.face([F.P(u, v, d) for u, v in corners], mat, uv, False, c.tone, F.dd)
        k.cur_fx = None
        # Mørk fuge mellom bordene, bare nær.
        if k.lod == 0 and mat == 'Board' and b == c.u1 and b < u1 - 1e-3 and low > 0:
            k.tag = 'boardGap'
            w = .006
            ps = [F.P(b - w, max(z, -.02), d + .003), F.P(b + w, max(z, -.02), d + .003), F.P(b + w, tb, d + .003), F.P(b - w, tb, d + .003)]
            k.face(ps, mat, [(.1, 0), (.105, 0), (.105, 1), (.1, 1)], False, (.2, .18, .16), F.dd)


def broken_boards(k, F, cols, holes, poly):
    """To til tre ødelagte bord per vegg: mørkt hull med skrå brudd, helt nede eller litt oppe."""
    if k.lod or F.L < 1.2:
        return
    want = 2 + (k.rng.random() < .5) if F.L > 2.5 else 1
    tries, done = 0, 0
    k.tag = 'boardGap'
    while done < want and tries < 30:
        tries += 1
        c = cols[k.rng.randrange(len(cols))]
        z0 = (-.02 if k.rng.random() < .7 else .15 + k.rng.random() * .7)
        h = .16 + k.rng.random() * .34
        top = min(_ztop(poly, c.u0), _ztop(poly, c.u1))
        if z0 + h > top - .3 or any(c.u1 > a - .05 and c.u0 < b + .05 and z0 + h > e - .05 and z0 < f + .05 for a, b, e, f in holes):
            continue
        a, b = c.u0 + .01, c.u1 - .01
        ps = [F.P(a, z0, .004), F.P(b, z0 + k.rng.random() * .05, .004), F.P(b, z0 + h, .004), F.P(a, z0 + h - .04 - k.rng.random() * .14, .004)]
        k.face(ps, 'Board', [(.2, .1), (.25, .1), (.25, .3), (.2, .3)], False, (.2, .18, .16), F.dd)
        done += 1


def board_rect(k, F, name, u0, u1, z0, z1, d, tone=1.):
    """Flat kledning av bord utenpå noe, som et dørblad."""
    cols = columns(k, 'Board', u1 - u0)
    for c in cols:
        c.u0, c.u1 = c.u0 + u0, c.u1 + u0
        c.tone = tuple(v * tone for v in c.tone)
    emit_columns(k, F, cols, 'Board', u0, u1, z0, z1, z1, [(u0, z1), (u1, z1)], d, weather=False, name=name)


def dust_skirt(k, F, skip=(), u0=0., u1=None):
    """Støv som har lagt seg mot veggen: en ujevn lav kant av jord som dykker under terrenget. Høyest i hjørnene."""
    u1 = F.L if u1 is None else u1
    step = (.45, .9, 3.)[k.lod]
    n = max(1, int((u1 - u0) / step))
    pts = []
    for i in range(n + 1):
        u = u0 + (u1 - u0) * i / n
        cor = max(0., 1 - min(u - u0, u1 - u) / .8) ** 2
        pts.append((u, .04 + k.rng.random() * .09 + .08 * cor, .24 + k.rng.random() * .14 + .2 * cor))
    k.tag = 'dust'
    for (ua, ha, oa), (ub, hb, ob) in zip(pts, pts[1:]):
        if any(a < (ua + ub) / 2 < b for a, b in skip):
            continue
        for (da0, za0, db0, zb0, da1, za1, db1, zb1) in ((-.02, ha, -.02, hb, oa * .45, ha * .55, ob * .45, hb * .55),
                                                          (oa * .45, ha * .55, ob * .45, hb * .55, oa, -.22, ob, -.22)):
            ps = [F.P(ua, za0, da0), F.P(ub, zb0, db0), F.P(ub, zb1, db1), F.P(ua, za1, da1)]
            k.face(ps, 'Dirt', None, False, (.88, .82, .74), (0, 0, 1))


def dust_corner(k, x, y, sx, sy):
    """Støvtrekanten utenfor et ytterhjørne, mellom to kanter."""
    if k.lod == 2:
        return
    k.tag = 'dust'
    o = .48 + k.rng.random() * .1
    h = .1 + k.rng.random() * .06
    apex = (x - sx * .02, y - sy * .02, h)
    rim = [(x + sx * o, y, -.22), (x + sx * o * .72, y + sy * o * .72, -.22), (x, y + sy * o, -.22)]
    for a, b in zip(rim, rim[1:]):
        k.face([apex, a, b], 'Dirt', None, False, (.88, .82, .74), (0, 0, 1))


def drum(k, F, u, d=.34, r=.29, h=.88, tint=(.6, .55, .5), mat='Steel'):
    """Oljefat mot veggen, med to vulster. Bare nær: lengre unna er det lite nok til å falle bort."""
    if k.lod == 2:
        return
    k.pipe('drum', F.P(u, -.05, d), F.P(u, h, d), r, mat, (14, 8)[k.lod], tint)
    if k.lod == 0:
        for z in (h * .35, h * .68):
            k.pipe('drumRib', F.P(u, z - .012, d), F.P(u, z + .012, d), r + .008, mat, 14, tint)


def pallet(k, F, u0, u1=None, tone=(.82, .78, .72)):
    """Trepall som står lent mot veggen."""
    if k.lod:
        return
    u1 = u0 + 1.1 if u1 is None else u1
    z_top, foot, lean = 1.0, .26, .03
    at = lambda z: foot + (lean - foot) * z / z_top
    for u in (u0 + .05, (u0 + u1) / 2, u1 - .05):
        k.beam('palletRail', F.P(u, -.02, at(0) + .04), F.P(u, z_top, at(z_top) + .04), .09, .07, 'Wood', F.dd, tone)
    z = .06
    while z < z_top:
        k.beam('palletSlat', F.P(u0, z, at(z) + .09), F.P(u1, z, at(z) + .09), .09, .018, 'Wood', F.dd, tone)
        z += .16


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
    tmu = TILE_U.get(mat, tm)
    cols = columns(k, mat, L) if k.lod < 2 and mat in ('Board', 'Stucco') else None
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
            if cols:
                emit_columns(k, F, cols, mat, u0, u1, z, t0, t1, poly, off=off, name=name)
                k.tag = name
            else:
                k.face(ps, mat, [(p_u / tmu + off[0], p_z / tm + off[1]) for p_u, p_z in
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
    if cols and mat == 'Board':
        broken_boards(k, F, cols, holes, poly)
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
    slope_len = math.sqrt(dot(sub(r0, e0), sub(r0, e0)))
    # Platene ligger i to til tre partier med ulik alder over flaten, og skjøtes hver 2,4 m nedover.
    parts = 2 + (k.rng.random() < .6)
    breaks = sorted(k.rng.randrange(1, max(2, count)) for _ in range(parts - 1))
    ages = [(.98, .97, .94), (.8, .77, .72), (.9, .84, .75)]
    k.rng.shuffle(ages)
    laps = max(1, int(slope_len // 2.4)) if k.lod < 2 else 1
    ztop = max(e0[2], e1[2], r0[2], r1[2])
    for i in range(count):
        t0, t1 = i / count, (i + 1) / count
        lift = mul(n, .014 if i % 2 else 0)
        age = ages[sum(1 for q in breaks if q <= i)]
        rusty = k.rng.random() < rust
        w = k.rng.random()
        roll = k.rng.random()
        if roll < .12:          # Nyere plate, byttet senere.
            tint = (1.02 + .08 * w, 1.01 + .08 * w, 1 + .08 * w)
        elif rusty:             # Eldre plate som drar mot rustbrunt.
            tint = (.97 + .05 * w, .86 + .07 * w, .76 + .08 * w)
        else:
            tint = (.92 + .1 * w, .91 + .1 * w, .88 + .1 * w)
        tint = tuple(a * b for a, b in zip(tint, age))
        # V = 0 ved raftet, med litt ulik platelengde. Da havner rusten nederst på hver plate.
        v_eave = dot(e0, slope) / TILE['Tin']
        off = (k.rng.randrange(7) * .25, -v_eave + (k.rng.random() - .5) * .06)
        # Rust som har rent nedover fra mønet, på noen plater.
        if k.rng.random() < .3 + rust:
            k.cur_fx = [(RUST_COL, .55 + w * .45, ztop, .9 + k.rng.random() * 2.)]
        loose = k.lod == 0 and k.rng.random() < .07     # Løs plate: hjørnet ved raftet har løftet seg.
        for row in range(laps):
            s0, s1 = row / laps, (row + 1) / laps
            shade = 1 if row % 2 == 0 or laps == 1 else .95
            pa = add(add(e0, mul(ua, L * t0)), mul(sub(r0, e0), s0))
            pb = add(add(e0, mul(ua, L * t1)), mul(sub(r0, e0), s0))
            pc = add(add(e0, mul(ua, L * t1)), mul(sub(r0, e0), s1))
            pd = add(add(e0, mul(ua, L * t0)), mul(sub(r0, e0), s1))
            ps = [add(pa, lift), add(pb, lift), add(pc, lift), add(pd, lift)]
            if loose and row == 0:
                ps[0] = add(ps[0], mul(n, .035))
            k.uvq(ps, 'Tin', ua, slope, off, tuple(c * shade for c in tint), n)
            if k.lod == 0 and row > 0:
                # Skjøten: den øvre platen ligger over den nedre.
                band = mul(slope, .022)
                up = mul(n, .004)
                q = [add(add(pa, lift), up), add(add(pb, lift), up), add(add(add(pb, band), lift), up), add(add(add(pa, band), lift), up)]
                k.uvq(q, 'Tin', ua, slope, off, tuple(c * .52 for c in tint), n)
        k.cur_fx = None
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
            tu = TILE_U['Tin']
            uv = [(s0 / tu, .02), (s1 / tu, .02), (s1 / tu, .05), (s0 / tu, .05)]
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
    cuts = (0, STRIPE, .3, FADE) if k.lod < 2 else (0, .3)
    v, f, uv, mat, smooth, tags, tint, fx = [], [], [], [], [], [], [], {}
    for i, ids in enumerate(k.f):
        pieces = [([k.v[n] for n in ids], list(k.uv[i]))]
        if grime_of(k.mat[i]) and k.tags[i] != 'boardGap':
            for z in cuts + ((.45,) if k.mat[i] == 'M_Farm_Stucco' and k.lod < 2 else ()):
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
            if i in k.fx:
                fx[len(f)] = k.fx[i]
            v.extend(ps)
            f.append(list(range(start, start + len(ps))))
            uv.append(ts)
            mat.append(k.mat[i])
            smooth.append(k.smooth[i])
            tags.append(k.tags[i])
            tint.append(k.tint[i])
    k.v, k.f, k.uv, k.mat, k.smooth, k.tags, k.tint, k.fx = v, f, uv, mat, smooth, tags, tint, fx


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
    low, mid = (.7, .88) if mat == 'M_Farm_Tin' else (.62, .84)
    z = p[2]
    if z <= 0:
        out = low
    elif z <= STRIPE:
        out = low + (mid - low) * z / STRIPE
    elif z <= FADE:
        out = mid + (1 - mid) * (z - STRIPE) / (FADE - STRIPE)
    else:
        out = 1.0
    if mat == 'M_Farm_Stucco':
        # Oppstigende fukt i pussen, omtrent til 0,3 m, med ujevn øvre kant.
        top = .2 + .16 * wob(p)
        out *= .74 + .26 * smoothstep(top, top + .12, z)
    return out


def earth_mix(mat, p):
    """Hvor mye jordfarge materialet blander inn nederst, 20 til 30 prosent ved bakken og ingenting ved 0,6 m."""
    if mat in ('M_Farm_Glass', 'M_Farm_WindowLit', 'M_Farm_Steel', 'M_Farm_Dirt') or p[2] >= FADE:
        return 0.
    peak = .16 if mat == 'M_Farm_Concrete' else .25
    return peak * (.8 + .4 * wob(p)) * (1 - smoothstep(0, FADE, max(p[2], 0)))


def fx_tint(fx, p):
    """Skyggebånd, avrenning og rust regnes per hjørne etter høyden."""
    out = [1., 1., 1.]
    for col, strength, top, length in fx or ():
        t = strength * smoothstep(top - length, top, p[2])
        for i in range(3):
            out[i] *= 1 + (col[i] - 1) * t
    return out


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
