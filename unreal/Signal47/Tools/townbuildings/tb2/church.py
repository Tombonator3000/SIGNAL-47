"""TB2 Caliche church: weathered white clapboard country church, New Mexico 1986.

Pure Python, no Blender dependency. build(lod) -> four Mesh parts, same local frame as TB1:
metres, Z up, front gable faces -X at x=0, nave x 0..14, y -4.5..4.5, eaves 4.2.
Footprint, wall height, door and window openings follow design.BUILDINGS['Church'] and TB1.
"""
import math, random, sys
from pathlib import Path
R = Path(__file__).resolve().parent.parent
if str(R) not in sys.path:
    sys.path.insert(0, str(R))
from geometry import Mesh, seg, add, sub, mul, dot, cross, unit
from geometry import cabinet, chair
from design import BUILDINGS

B = BUILDINGS['Church']
WT = .20                 # wall thickness
XF, XR, YS = -.10, 14.10, 4.60   # outer faces: front, rear, sides
EAVE = 4.20              # top of main walls (unchanged)
YE, ZE, K, DECK = 4.97, 4.32, .66, .12   # fascia line, deck underside at fascia, pitch, deck depth
RIDGE = ZE + YE * K      # deck underside at ridge
SIDE_WIN = [3, 7, 11]
WEATHER = random.Random(47)

LODP = {
    0: dict(lod=0, arc=8, siding=True, caps=True, shseg=1.0, courses=True, pieces=True, d=2),
    1: dict(lod=1, arc=5, siding=True, caps=False, shseg=2.2, courses=True, pieces=False, d=1),
    2: dict(lod=2, arc=2, siding=False, caps=False, shseg=None, courses=True, pieces=False, d=0),
}


def zu(y):
    """Roof deck underside at lateral offset y."""
    return ZE + (YE - abs(y)) * K


# ---------------------------------------------------------------- primitives
def newell(ps):
    n = [0., 0., 0.]
    for i in range(len(ps)):
        a, b = ps[i], ps[(i + 1) % len(ps)]
        n[0] += (a[1] - b[1]) * (a[2] + b[2]); n[1] += (a[2] - b[2]) * (a[0] + b[0]); n[2] += (a[0] - b[0]) * (a[1] + b[1])
    return tuple(n)


def F(m, ps, mat, want=None, uv=None, smooth=False):
    ps = list(ps)
    if want is not None and dot(newell(ps), want) < 0:
        ps = ps[::-1]; uv = uv[::-1] if uv else None
    # rotate so the first three points are not collinear (meshkit tests those)
    for k in range(len(ps)):
        q = ps[k:] + ps[:k]
        c = cross(sub(q[1], q[0]), sub(q[2], q[0]))
        if dot(c, c) > 1e-14:
            ps = q; uv = (uv[k:] + uv[:k]) if uv else None; break
    m.face(ps, mat, uv, smooth)


def area2(pts):
    return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1] for i in range(len(pts)))


def convex(pts):
    s = 0
    for i in range(len(pts)):
        a, b, c = pts[i - 1], pts[i], pts[(i + 1) % len(pts)]
        cr = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0])
        if abs(cr) < 1e-12: continue
        if s == 0: s = 1 if cr > 0 else -1
        elif (cr > 0) != (s > 0): return False
    return True


def earclip(pts):
    n = len(pts); idx = list(range(n))
    if area2(pts) < 0: idx.reverse()
    def inside(p, a, b, c):
        def sd(p1, p2, p3): return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
        d1, d2, d3 = sd(p, a, b), sd(p, b, c), sd(p, c, a)
        return not ((d1 < -1e-12 or d2 < -1e-12 or d3 < -1e-12) and (d1 > 1e-12 or d2 > 1e-12 or d3 > 1e-12))
    tris = []; guard = 0
    while len(idx) > 3 and guard < 4 * n * n:
        guard += 1; done = False
        for k in range(len(idx)):
            a, b, c = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            A, Bp, C = pts[a], pts[b], pts[c]
            cr = (Bp[0] - A[0]) * (C[1] - A[1]) - (Bp[1] - A[1]) * (C[0] - A[0])
            if cr <= 1e-12: continue
            if any(inside(pts[p], A, Bp, C) for p in idx if p not in (a, b, c)): continue
            tris.append((a, b, c)); idx.pop(k); done = True; break
        if not done: break
    if len(idx) == 3: tris.append(tuple(idx))
    return tris


def cap_faces(m, P3, pts, mat, want):
    if convex(pts):
        F(m, P3, mat, want)
    else:
        for a, b, c in earclip(pts):
            pa, pb, pc = P3[a], P3[b], P3[c]
            cc = cross(sub(pb, pa), sub(pc, pa))
            if dot(cc, cc) > 1e-14: F(m, [pa, pb, pc], mat, want)


def prism(m, tag, pts, P, D, mat, sides=None, mats=None, front=True, back=True):
    """Extrude 2D polygon pts mapped by affine P(u,v) along D. sides: edge indices to close (None=all)."""
    m.tag = tag
    o = P(0, 0); U = sub(P(1, 0), o); V = sub(P(0, 1), o)
    A = [P(*p) for p in pts]; Bk = [add(a, D) for a in A]
    if front: cap_faces(m, A, pts, mat, mul(D, -1))
    if back: cap_faces(m, Bk, pts, mat, D)
    sgn = 1 if area2(pts) > 0 else -1
    for i in range(len(pts)):
        if sides is not None and i not in sides: continue
        j = (i + 1) % len(pts)
        du, dv = pts[j][0] - pts[i][0], pts[j][1] - pts[i][1]
        if du * du + dv * dv < 1e-12: continue
        out = add(mul(U, sgn * dv), mul(V, -sgn * du))
        F(m, [A[i], A[j], Bk[j], Bk[i]], (mats or {}).get(i, mat), out)


def obox(m, tag, c, ax, ay, az, size, mat):
    m.tag = tag
    ax, ay, az = unit(ax), unit(ay), unit(az); h = [s / 2 for s in size]
    def P(i, j, k): return add(c, add(mul(ax, i * h[0]), add(mul(ay, j * h[1]), mul(az, k * h[2]))))
    for v, quad in [(ax, [(1, -1, -1), (1, 1, -1), (1, 1, 1), (1, -1, 1)]), (ay, [(-1, 1, -1), (1, 1, -1), (1, 1, 1), (-1, 1, 1)]), (az, [(-1, -1, 1), (1, -1, 1), (1, 1, 1), (-1, 1, 1)])]:
        for s in (1, -1):
            F(m, [P(*(s * q if k == [ax, ay, az].index(v) else q for k, q in enumerate(t))) for t in quad], mat, mul(v, s))


def beam(m, tag, a, b, w, h, mat, up=(0, 0, 1)):
    ax = unit(sub(b, a))
    if abs(dot(ax, unit(up))) > .98: up = (1, 0, 0)
    side = unit(cross(up, ax)); az = cross(ax, side)
    L = math.sqrt(dot(sub(b, a), sub(b, a)))
    obox(m, tag, mul(add(a, b), .5), ax, side, az, (L, w, h), mat)


def bx(m, tag, x0, x1, y0, y1, z0, z1, mat):
    if min(x1 - x0, y1 - y0, z1 - z0) <= 1e-6: return
    m.box(tag, ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0), mat)


class Wall:
    """Wall frame: u along the wall, z up, d outward from the outer face."""
    def __init__(s, O, U, N): s.O, s.U, s.N = O, U, N
    def p(s, u, z, d=0.): return add(s.O, add(mul(s.U, u), add((0, 0, z), mul(s.N, d))))
    def prism(s, m, tag, pts, d0, d1, mat, sides=None, mats=None):
        prism(m, tag, pts, lambda u, z: s.p(u, z, d0), mul(s.N, d1 - d0), mat, sides, mats)
    def rect(s, m, tag, u0, u1, z0, z1, d0, d1, mat):
        if min(u1 - u0, z1 - z0, d1 - d0) <= 1e-6: return
        s.prism(m, tag, [(u0, z0), (u1, z0), (u1, z1), (u0, z1)], d0, d1, mat)
    def bar(s, m, tag, a, b, w, d0, d1, mat):
        du, dz = b[0] - a[0], b[1] - a[1]; L = math.hypot(du, dz); nu, nz = -dz / L * w / 2, du / L * w / 2
        s.prism(m, tag, [(a[0] + nu, a[1] + nz), (a[0] - nu, a[1] - nz), (b[0] - nu, b[1] - nz), (b[0] + nu, b[1] + nz)], d0, d1, mat)
    def core(s, m, u0, u1, z0, z1, holes, mat, d0=-WT, d1=0.):
        cuts = sorted(set([u0, u1] + [max(u0, min(u1, a)) for h in holes for a in h[:2]]))
        for a, b in zip(cuts, cuts[1:]):
            if b - a < 1e-6: continue
            iv = sorted((h[2], h[3]) for h in holes if h[0] < (a + b) / 2 < h[1]); last = z0
            for lo, hi in iv:
                s.rect(m, 'Wall_core_timber_frame', a, b, last, lo, d0, d1, mat); last = max(last, hi)
            s.rect(m, 'Wall_core_timber_frame', a, b, last, z1, d0, d1, mat)


FRONT = Wall((XF, 0, 0), (0, 1, 0), (-1, 0, 0))
REAR = Wall((XR, 0, 0), (0, 1, 0), (1, 0, 0))
SIDES = {s: Wall((0, s * YS, 0), (1, 0, 0), (0, s, 0)) for s in (-1, 1)}


class Lancet:
    """Pointed (acute) arch: width w, sill 'bottom', apex 'top', arc radius ratio*w."""
    def __init__(s, c, w, bottom, top, ratio=1.4):
        s.c, s.w, s.bottom, s.top = c, w, bottom, top
        s.r = ratio * w; s.cl = s.r - w / 2; s.sh = top - math.sqrt(s.r ** 2 - s.cl ** 2)
    def arc(s, rad, side, n):
        cu = s.c - side * s.cl
        a1 = math.acos(max(-1, min(1, (s.c - cu) / rad))); a0 = math.pi if side < 0 else 0.
        return [(cu + rad * math.cos(a0 + (a1 - a0) * k / n), s.sh + rad * math.sin(a0 + (a1 - a0) * k / n)) for k in range(n + 1)]
    def apex(s, band=0.): return s.sh + math.sqrt((s.r + band) ** 2 - s.cl ** 2)
    def hw(s, z, band):
        if z <= s.sh: return s.w / 2 + band
        ro = s.r + band; dz = z - s.sh
        if dz >= ro: return 0.
        return max(0., math.sqrt(ro * ro - dz * dz) - s.cl)
    def outline(s, n, band=0.):
        right = s.arc(s.r + band, 1, n); left = s.arc(s.r + band, -1, n)
        return [(s.c - s.w / 2 - band, s.bottom), (s.c + s.w / 2 + band, s.bottom)] + right + left[::-1][1:]


def arch_band(m, w, L, r0, r1, d0, d1, mat, n, tag):
    for side in (-1, 1):
        o = L.arc(r1, side, n); i = L.arc(r0, side, n)
        pts = o + i[::-1]
        w.prism(m, tag, pts, d0, d1, mat, sides=set(range(n)) | set(range(n + 1, 2 * n + 1)))


def lancet_window(s, g, w, L, lp, band=.10, interior=True):
    n = lp['arc']; d = lp['d']
    # spandrels fill the rectangular wall hole above the arch: real arched reveal through the wall
    for side in (-1, 1):
        arc = L.arc(L.r, side, n)
        w.prism(s, 'Lancet_arched_reveal', arc + [(L.c + side * L.w / 2, L.top)], -WT, 0, 'ChurchWhiteBoard', sides=set(range(n)))
    # exterior casing
    for side in (-1, 1):
        u0, u1 = sorted([L.c + side * L.w / 2, L.c + side * (L.w / 2 + band)])
        w.rect(s, 'Lancet_exterior_casing', u0, u1, L.bottom, L.sh, 0, .036, 'ChurchTrim')
    arch_band(s, w, L, L.r, L.r + band, 0, .036, 'ChurchTrim', n, 'Lancet_arched_casing')
    if d: w.rect(s, 'Lancet_keystone', L.c - .045, L.c + .045, L.apex(band) - .07, L.apex(band) + .03, 0, .05, 'ChurchTrim')
    # sloped sill and apron
    w.rect(s, 'Lancet_drip_sill', L.c - L.w / 2 - band - .05, L.c + L.w / 2 + band + .05, L.bottom - .06, L.bottom, -.10, .075, 'ChurchTrim')
    w.rect(s, 'Lancet_sill_apron', L.c - L.w / 2 - band + .02, L.c + L.w / 2 + band - .02, L.bottom - .20, L.bottom - .06, 0, .03, 'ChurchTrim')
    # sash frame in the reveal
    sb = .055 if L.w > .8 else .045
    for side in (-1, 1):
        u0, u1 = sorted([L.c + side * L.w / 2, L.c + side * (L.w / 2 - sb)])
        w.rect(s, 'Sash_stile', u0, u1, L.bottom, L.sh, -.11, -.05, 'ChurchTrim')
    w.rect(s, 'Sash_bottom_rail', L.c - L.w / 2, L.c + L.w / 2, L.bottom, L.bottom + .07, -.11, -.05, 'ChurchTrim')
    arch_band(s, w, L, L.r - sb, L.r, -.11, -.05, 'ChurchTrim', n, 'Sash_arched_head')
    # muntins and Y tracery
    mw = .028; top_m = L.sh + (L.apex() - L.sh) * .35
    w.bar(s, 'Sash_mullion', (L.c, L.bottom + .05), (L.c, top_m), mw * 1.3, -.10, -.06, 'ChurchTrim')
    rows = 3 if L.w > .8 else 2
    for k in range(1, rows + 1):
        z = L.bottom + (L.sh - L.bottom) * k / (rows + 1)
        w.bar(s, 'Sash_meeting_muntin', (L.c - L.w / 2 + .02, z), (L.c + L.w / 2 - .02, z), mw, -.095, -.065, 'ChurchTrim')
    if d:
        for side in (-1, 1):
            pt = L.arc(L.r - sb, side, 10)[5]
            w.bar(s, 'Y_tracery_bar', (L.c, top_m - .02), pt, mw, -.095, -.065, 'ChurchTrim')
    # glass
    g.tag = 'Amber_cathedral_glazing'
    F(g, [w.p(u, z, -.08) for u, z in L.outline(n)], 'ChurchGlass', w.N)
    if interior:
        for side in (-1, 1):
            u0, u1 = sorted([L.c + side * L.w / 2, L.c + side * (L.w / 2 + .08)])
            w.rect(s, 'Lancet_interior_casing', u0, u1, L.bottom, L.sh, -WT - .022, -WT, 'ChurchTrim')
        arch_band(s, w, L, L.r, L.r + .08, -WT - .022, -WT, 'ChurchTrim', n, 'Lancet_interior_arch_casing')
        w.rect(s, 'Interior_window_stool', L.c - L.w / 2 - .12, L.c + L.w / 2 + .12, L.bottom - .035, L.bottom, -WT - .06, -.10, 'ChurchTrim')


def siding(m, w, z0, z1, ext, excl, lp, e=.15, t=.022, mat='ChurchWhiteBoard'):
    """Real lap siding: tilted boards with butt edges. ext(z)->(ua,ub); excl: (c, hw(z), z0, z1)."""
    m.tag = 'Lapped_clapboard_siding'
    zb = z0
    while zb + e <= z1 + 1e-6:
        zt = zb + e
        ea, eb = ext(zb); fa, fb = ext(zt)
        cuts = []
        for c, hw, a, b in excl:
            if zt <= a or zb >= b: continue
            if zb >= a and zt <= b: hb, ht = hw(zb), hw(zt)
            else: hb = ht = max(hw(min(max(zb, a), b)), hw(min(max(zt, a), b)), hw(min(max((zb + zt) / 2, a), b)))
            if hb <= 0 and ht <= 0: continue
            cuts.append((c - hb, c - ht, c + hb, c + ht))
        cuts.sort()
        segs = []; cur = (ea, fa)
        for lb, lt, rb, rt in cuts:
            segs.append((cur[0], lb, cur[1], lt)); cur = (max(cur[0], rb), max(cur[1], rt))
        segs.append((cur[0], eb, cur[1], fb))
        row = round(zb / e); pieces = []
        for u0b, u1b, u0t, u1t in segs:
            if u1b - u0b < .04 or u1t - u0t < .04: continue
            lo, hi = max(u0b, u0t) + .35, min(u1b, u1t) - .35
            L = 3.66; j = math.floor(lo / L) * L + (row * 1.43) % L; cut = []
            while j < hi:
                if j > lo: cut.append(j)
                j += L
            ends = [(u0b, u0t)] + [(c, c) for c in cut] + [(u1b, u1t)]
            for k in range(len(ends) - 1):
                g0 = .0015 if k > 0 else 0; g1 = .0015 if k < len(ends) - 2 else 0
                pieces.append((ends[k][0] + g0, ends[k + 1][0] - g1, ends[k][1] + g0, ends[k + 1][1] - g1))
        for u0b, u1b, u0t, u1t in pieces:
            a = w.p(u0b, zb, t); b = w.p(u1b, zb, t); c = w.p(u1t, zt, .003); d = w.p(u0t, zt, .003)
            bm = 'ChurchWornBoard' if (mat == 'ChurchWhiteBoard' and WEATHER.random() < (.6 if zb < .7 else .16)) else mat
            F(m, [a, b, c, d], bm, w.N)
            F(m, [w.p(u0b, zb, 0), w.p(u1b, zb, 0), b, a], bm, (0, 0, -1))
            if lp['caps']:
                F(m, [w.p(u0b, zb, 0), a, d, w.p(u0t, zt, 0)], bm, mul(w.U, -1))
                F(m, [w.p(u1b, zb, 0), b, c, w.p(u1t, zt, 0)], bm, w.U)
        zb = zt


def shingled(m, A, Bp, C, D, nout, courses, lp, rng, T=.014, tag='Asphalt_shingle_course'):
    """Stepped shingle courses on quad A,B (eave) -> D,C (top)."""
    m.tag = tag
    def L(p, q, t): return add(p, mul(sub(q, p), t))
    for i in range(courses):
        t0, t1 = i / courses, (i + 1) / courses
        L0, R0, L1, R1 = L(A, D, t0), L(Bp, C, t0), L(A, D, t1), L(Bp, C, t1)
        if lp['shseg']:
            width = math.sqrt(dot(sub(R0, L0), sub(R0, L0))); nseg = max(1, round(width / lp['shseg']))
            br = [0.] + sorted(min(.999, max(.001, (k + .5 * (i % 2) + rng.uniform(-.22, .22)) / nseg)) for k in range(1, nseg)) + [1.]
        else:
            br = [0., 1.]
        for f0, f1 in zip(br, br[1:]):
            if f1 - f0 < .01: continue
            th = T + (rng.uniform(-.004, .004) if lp['shseg'] else 0); off = rng.uniform(0, .003) if lp['shseg'] else 0
            p0, p1, q0, q1 = L(L0, R0, f0), L(L0, R0, f1), L(L1, R1, f0), L(L1, R1, f1)
            top = [add(p0, mul(nout, th + off)), add(p1, mul(nout, th + off)), add(q1, mul(nout, .005 + off)), add(q0, mul(nout, .005 + off))]
            sm = 'ChurchShingleFaded' if WEATHER.random() < .13 else 'RoofShingle'
            F(m, top, sm, nout)
            v = sub(p0, q0)
            F(m, [add(p0, mul(nout, .001)), add(p1, mul(nout, .001)), top[1], top[0]], sm, v)


# ---------------------------------------------------------------- shell
def shell_walls(s, g, lp, excl_front):
    # wall cores with real openings
    door, dw, dh = B['door']
    fwin = [Lancet(y, ww, z0, z1) for y, ww, z0, z1 in B['windows']]
    FRONT.core(s, -YS, YS, 0, EAVE, [(door - dw / 2, door + dw / 2, 0, dh)] + [(L.c - L.w / 2, L.c + L.w / 2, L.bottom, L.top) for L in fwin], 'ChurchWhiteBoard')
    REAR.core(s, -YS, YS, 0, EAVE, [], 'ChurchWhiteBoard')
    gable = [(-YS, EAVE), (YS, EAVE), (YS, zu(YS)), (0, RIDGE), (-YS, zu(YS))]
    for w in (FRONT, REAR): w.prism(s, 'Gable_end_timber_frame', gable, -WT, 0, 'ChurchWhiteBoard')
    swin = {}
    for sd, w in SIDES.items():
        swin[sd] = [Lancet(x, 1.3, 1.25, 3.75) for x in SIDE_WIN]
        w.core(s, XF + WT, XR - WT, 0, EAVE, [(L.c - L.w / 2, L.c + L.w / 2, L.bottom, L.top) for L in swin[sd]], 'ChurchWhiteBoard')
    for L in fwin: lancet_window(s, g, FRONT, L, lp, band=.08)
    for sd, w in SIDES.items():
        for L in swin[sd]: lancet_window(s, g, w, L, lp)
    # siding
    if lp['siding']:
        def hwg(z): return min(YS - .05, (RIDGE - .17 - z) / K)
        def gext(z): v = hwg(z) if z > EAVE - .05 else YS - .05; return (-v, v)
        ex = [(L.c, (lambda z, L=L: L.hw(z, .08)), L.bottom - .2, L.apex(.08)) for L in fwin]
        ex += [(door, lambda z: dw / 2 + .15, 0, dh + .24)]
        ex += excl_front
        siding(s, FRONT, .33, RIDGE - .35, gext, ex, lp)
        siding(s, REAR, .33, RIDGE - .35, gext, excl_front, lp)
        for sd, w in SIDES.items():
            exs = [(L.c, (lambda z, L=L: L.hw(z, .10)), L.bottom - .2, L.apex(.10)) for L in swin[sd]]
            siding(s, w, .33, 3.93, lambda z: (XF + .04, XR - .04), exs, lp)
    # plinth, water table, corner boards, frieze
    for w, spans in [(FRONT, [(-YS, door - dw / 2), (door + dw / 2, YS)]), (REAR, [(-YS, YS)])]:
        for a, b in spans:
            w.rect(s, 'Concrete_plinth_course', a, b, -.15, .20, -.05, .018, 'Concrete')
            w.rect(s, 'Water_table_board', a, b, .20, .34, 0, .028, 'ChurchTrim')
            w.rect(s, 'Water_table_drip_cap', a, b, .34, .36, 0, .045, 'ChurchTrim')
        for sd in (-1, 1):
            u0, u1 = sorted([sd * (YS - .13), sd * (YS + .036)])
            w.rect(s, 'Corner_board', u0, u1, .2, EAVE, 0, .036, 'ChurchTrim')
        for sd in (-1, 1):  # rake frieze on gable face under the roof
            a = sd * (YS + .02)
            w.prism(s, 'Gable_rake_frieze', [(a, zu(a) - .20), (0, RIDGE - .20), (0, RIDGE), (a, zu(a))], 0, .03, 'ChurchTrim')
    for sd, w in SIDES.items():
        w.rect(s, 'Concrete_plinth_course', XF, XR, -.15, .20, -.05, .018, 'Concrete')
        w.rect(s, 'Water_table_board', XF, XR, .20, .34, 0, .028, 'ChurchTrim')
        w.rect(s, 'Water_table_drip_cap', XF, XR, .34, .36, 0, .045, 'ChurchTrim')
        for x0, x1 in [(XF, XF + .13), (XR - .13, XR)]:
            w.rect(s, 'Corner_board', x0, x1, .2, EAVE, 0, .036, 'ChurchTrim')
        w.rect(s, 'Eave_frieze_board', XF, XR, 3.93, EAVE, 0, .03, 'ChurchTrim')
        y0, y1 = sorted([sd * YS, sd * YE])
        bx(s, 'Boxed_eave_soffit', XF, XR, y0, y1, EAVE - .02, EAVE, 'ChurchTrim')
        for x, nx in [(XF, -1), (XR, 1)]:  # eave box end closures
            s.tag = 'Eave_return_end'
            F(s, [(x, sd * YS, EAVE), (x, sd * YE, EAVE), (x, sd * YE, ZE), (x, sd * YS, zu(YS))], 'ChurchTrim', (nx, 0, 0))
    bx(s, 'Below_grade_foundation', XF, XR, -YS, YS, -.5, -.15, 'Concrete')
    return fwin, swin


def gable_vent(s, w, lp, zc=6.0):
    w.rect(s, 'Louvre_vent_back', -.26, .26, zc - .46, zc + .46, -.02, .004, 'DarkWood')
    for u0, u1, z0, z1 in [(-.32, -.25, zc - .52, zc + .52), (.25, .32, zc - .52, zc + .52), (-.32, .32, zc + .45, zc + .52), (-.32, .32, zc - .52, zc - .45)]:
        w.rect(s, 'Louvre_vent_casing', u0, u1, z0, z1, 0, .04, 'ChurchTrim')
    w.rect(s, 'Louvre_vent_cap', -.37, .37, zc + .52, zc + .57, 0, .07, 'ChurchTrim')
    k = 7 if lp['d'] else 3
    for i in range(k):
        z = zc - .4 + (i + .5) * .8 / k
        dirv = add(mul((0, 0, 1), .72), mul(w.N, -.69))
        obox(s, 'Louvre_slat', w.p(0, z, .022), w.U, dirv, cross(w.U, dirv), (.50, .13, .012), 'ChurchTrim')


def roof(s, lp, rng):
    for sd in (-1, 1):
        pts = [(sd * YE, ZE), (0, RIDGE), (0, RIDGE + DECK), (sd * YE, ZE + DECK)]
        mats = {0: 'ChurchTrim', 2: 'RoofShingle', 3: 'ChurchTrim'}
        prism(s, 'Roof_deck_and_rake_soffit', pts, lambda y, z: (XF - .25, y, z), (XR - XF + .5, 0, 0), 'ChurchTrim', sides={0, 2, 3}, mats=mats)
        nout = unit((0, sd * K, 1))
        if lp['courses']:
            A = (XF - .25, sd * YE, ZE + DECK); Bp = (XR + .25, sd * YE, ZE + DECK)
            C = (XR + .25, 0, RIDGE + DECK); D = (XF - .25, 0, RIDGE + DECK)
            shingled(s, A, Bp, C, D, nout, 42 if lp['lod'] < 2 else 21, lp, rng)
        # rake fascia boards
        for x, dx in [(XF - .25, -.035), (XR + .25, .035)]:
            prism(s, 'Rake_fascia_board', [(sd * (YE + .03), ZE - .06), (0, RIDGE - .06), (0, RIDGE + DECK + .04), (sd * (YE + .03), ZE + DECK + .04)], lambda y, z, x=x: (x, y, z), (dx, 0, 0), 'ChurchTrim', sides={0, 2, 3})
        y0, y1 = sorted([sd * YE, sd * (YE + .03)])
        bx(s, 'Eave_fascia_board', XF - .285, XR + .285, y0, y1, EAVE - .02, ZE + DECK + .02, 'ChurchTrim')
        # ridge cap boards
        hw = .17
        a = (XF - .25, 0, RIDGE + DECK + .012); b = (XR + .25, 0, RIDGE + DECK + .012)
        e0 = (XF - .25, sd * hw, RIDGE + DECK + .012 - hw * K); e1 = (XR + .25, sd * hw, RIDGE + DECK + .012 - hw * K)
        s.tag = 'Ridge_cap'; F(s, [a, b, e1, e0], 'RoofShingle', (0, sd * K, 1))
        F(s, [e0, e1, add(e1, (0, 0, -.02)), add(e0, (0, 0, -.02))], 'RoofShingle', (0, sd, 0))
    if lp['pieces']:
        x = XF - .25; s.tag = 'Ridge_cap_shingle'
        while x < XR + .25 - .05:
            x1 = min(x + .30, XR + .25); z0 = RIDGE + DECK + .012 + .016; z1 = RIDGE + DECK + .012 + .004
            for sd in (-1, 1):
                F(s, [(x, 0, z0), (x1, 0, z1), (x1, sd * .16, z1 - .16 * K), (x, sd * .16, z0 - .16 * K)], 'RoofShingle', (0, sd * K, 1))
                F(s, [(x, 0, z0), (x, sd * .16, z0 - .16 * K), (x, sd * .16, z0 - .16 * K - .014), (x, 0, z0 - .014)], 'RoofShingle', (-1, 0, 0))
            x = x1


TOWER_DZ = .45


def tower(s, f, lp, rng):
    cx, hw = 1.4, .80
    xs = (cx - hw, cx + hw); top = 8.4 + TOWER_DZ
    bx(s, 'Tower_base_frame', xs[0], xs[1], -hw, hw, 6.9, top, 'ChurchWhiteBoard')
    TW = [Wall((xs[0], 0, 0), (0, 1, 0), (-1, 0, 0)), Wall((xs[1], 0, 0), (0, 1, 0), (1, 0, 0)), Wall((0, -hw, 0), (1, 0, 0), (0, -1, 0)), Wall((0, hw, 0), (1, 0, 0), (0, 1, 0))]
    for i, w in enumerate(TW):
        ext = (-hw + .04, hw - .04) if i < 2 else (xs[0] + .04, xs[1] - .04)
        if lp['siding']: siding(s, w, 7.0, top - .02, lambda z, e=ext: e, [], lp)
        for a, b in ([(-hw - .036, -hw + .11), (hw - .11, hw + .036)] if i < 2 else [(xs[0], xs[0] + .11), (xs[1] - .11, xs[1])]):
            w.rect(s, 'Tower_corner_board', a, b, 6.9, top, 0, .036, 'ChurchTrim')
    S, Fm = s, f; s = Mesh('tower_upper'); f = Mesh('tower_upper_fix')
    bx(s, 'Tower_belt_course', xs[0] - .08, xs[1] + .08, -hw - .08, hw + .08, 8.40, 8.52, 'ChurchTrim')
    bx(s, 'Tower_belt_drip', xs[0] - .11, xs[1] + .11, -hw - .11, hw + .11, 8.52, 8.55, 'ChurchTrim')
    # belfry corner posts
    for x in (xs[0] + .08, xs[1] - .08):
        for y in (-hw + .08, hw - .08):
            bx(s, 'Belfry_corner_post', x - .08, x + .08, y - .08, y + .08, 8.55, 10.0, 'ChurchTrim')
    BW = [Wall((xs[0], 0, 0), (0, 1, 0), (-1, 0, 0)), Wall((xs[1], 0, 0), (0, 1, 0), (1, 0, 0)), Wall((0, -hw, 0), (1, 0, 0), (0, -1, 0)), Wall((0, hw, 0), (1, 0, 0), (0, 1, 0))]
    for i, w in enumerate(BW):
        c = 0 if i < 2 else cx; half = hw - .16
        w.rect(s, 'Louvre_bottom_rail', c - half, c + half, 8.55, 8.60, -.10, 0, 'ChurchTrim')
        w.rect(s, 'Louvre_top_rail', c - half, c + half, 8.98, 9.05, -.10, 0, 'ChurchTrim')
        for k in range(4 if lp['d'] else 2):
            z = 8.62 + (k + .5) * .36 / (4 if lp['d'] else 2)
            dirv = add(mul((0, 0, 1), .72), mul(w.N, -.69))
            obox(s, 'Belfry_louvre_slat', w.p(c, z, -.05), w.U, dirv, cross(w.U, dirv), (2 * half, .13, .014), 'ChurchTrim')
        L = Lancet(c, 2 * half, 9.05, 10.0, ratio=.586)
        n = lp['arc']
        for side in (-1, 1):
            arc = L.arc(L.r, side, n)
            w.prism(s, 'Belfry_arch_spandrel', arc + [(c + side * half, 10.0)], -.09, 0, 'ChurchWhiteBoard', sides=set(range(n)))
        if lp['d']: arch_band(s, w, L, L.r - .001, L.r + .05, 0, .02, 'ChurchTrim', n, 'Belfry_arch_trim')
    for x0, x1, y0, y1 in [(xs[0], xs[1], -hw, -hw + .1), (xs[0], xs[1], hw - .1, hw), (xs[0], xs[0] + .1, -hw + .1, hw - .1), (xs[1] - .1, xs[1], -hw + .1, hw - .1)]:
        bx(s, 'Belfry_top_beam', x0, x1, y0, y1, 10.0, 10.16, 'ChurchTrim')
    s.tag = 'Belfry_ceiling'; F(s, [(xs[0], -hw, 10.0), (xs[1], -hw, 10.0), (xs[1], hw, 10.0), (xs[0], hw, 10.0)], 'ChurchTrim', (0, 0, -1))
    bx(s, 'Belfry_cornice', xs[0] - .1, xs[1] + .1, -hw - .1, hw + .1, 10.16, 10.24, 'ChurchTrim')
    # bell frame timbers (Shell) on the belfry floor
    for y in (-.55, .55):
        bx(s, 'Bell_frame_sill', 0.74, 2.06, y - .05, y + .05, 8.55, 8.65, 'DarkWood')
        for x in (1.0, 1.8): bx(s, 'Bell_frame_post', x - .05, x + .05, y - .05, y + .05, 8.65, 9.58, 'DarkWood')
        bx(s, 'Bell_frame_head', 0.9, 1.9, y - .055, y + .055, 9.58, 9.68, 'DarkWood')
        for x, xx in [(1.0, .78), (1.8, 2.02)]: beam(s, 'Bell_frame_brace', (x, y, 9.2), (xx, y, 8.65), .07, .07, 'DarkWood', up=(0, 1, 0))
    # bellcast pyramid roof
    e, ze, k2, zs, ap = 1.06, 10.20, .88, 10.38, 11.35
    sk = [(cx - e, -e), (cx + e, -e), (cx + e, e), (cx - e, e)]
    up = [(cx - k2, -k2), (cx + k2, -k2), (cx + k2, k2), (cx - k2, k2)]
    tip = .07; zt = zs + (ap - zs) * (1 - tip / k2)
    tp = [(cx - tip, -tip), (cx + tip, -tip), (cx + tip, tip), (cx - tip, tip)]
    for i in range(4):
        j = (i + 1) % 4
        a, b = sk[i], sk[j]; c2, d2 = up[j], up[i]
        mid = ((a[0] + b[0]) / 2 - cx, (a[1] + b[1]) / 2)
        out = unit((mid[0], mid[1], .9))
        s.tag = 'Bellcast_skirt'; F(s, [(a[0], a[1], ze), (b[0], b[1], ze), (c2[0], c2[1], zs), (d2[0], d2[1], zs)], 'RoofShingle', out)
        out2 = unit((mid[0] * .8, mid[1] * .8, .9))
        P = [(d2[0], d2[1], zs), (c2[0], c2[1], zs), (tp[j][0], tp[j][1], zt), (tp[i][0], tp[i][1], zt)]
        s.tag = 'Belfry_spire_face'; F(s, P, 'RoofShingle', out2)
        if lp['courses']:
            nrm = unit(newell(P)); shingled(s, P[0], P[1], P[2], P[3], nrm, [9, 6, 4][lp['lod']], dict(lp, shseg=None), rng, T=.012)
        s.tag = 'Spire_cap'; F(s, [(tp[i][0], tp[i][1], zt), (tp[j][0], tp[j][1], zt), (cx, 0, ap)], 'RoofShingle', out2)
        s.tag = 'Bellcast_soffit'; F(s, [(a[0], a[1], ze), (b[0], b[1], ze), ((b[0] - cx) * .85 + cx, b[1] * .85, ze), ((a[0] - cx) * .85 + cx, a[1] * .85, ze)], 'ChurchTrim', (0, 0, -1))
    for sd in (-1, 1):
        bx(s, 'Bellcast_fascia', cx - e - .02, cx + e + .02, *sorted([sd * (e - .02), sd * (e + .02)]), ze - .05, ze + .01, 'ChurchTrim')
        bx(s, 'Bellcast_fascia', *sorted([cx + sd * (e - .02), cx + sd * (e + .02)]), -e + .02, e - .02, ze - .05, ze + .01, 'ChurchTrim')
    s.lathe('Spire_finial', (cx, 0, ap - .06), [(0, .07), (.08, .085), (.15, .05), (.2, .035)], 'ChurchTrim', max(6, seg(lp['lod']) // 2))
    # iron cross (Fixtures)
    bx(f, 'Wrought_iron_cross_upright', cx - .035, cx + .035, -.035, .035, ap + .12, ap + .95, 'DarkMetal')
    bx(f, 'Wrought_iron_cross_arm', cx - .035, cx + .035, -.26, .26, ap + .62, ap + .69, 'DarkMetal')
    bell(f, cx, lp)
    S.merge(s, (0, 0, TOWER_DZ)); Fm.merge(f, (0, 0, TOWER_DZ))
    return cx


def lp_lod(lp):
    return lp['lod']


def bell(f, cx, lp):
    n = [20, 12, 8][lp_lod(lp)]
    z0 = 9.0
    outer = [(0, .34), (.035, .336), (.08, .30), (.16, .252), (.30, .226), (.44, .214), (.52, .19), (.57, .13), (.60, .06)]
    inner = [(z, max(.03, r - .024)) for z, r in outer[1:-1]] + [(.575, .03)]
    f.lathe('Bronze_bell_outer', (cx, 0, z0), outer, 'ChurchBronze', n, caps=False)
    f.lathe('Bronze_bell_inner', (cx, 0, z0), inner[::-1], 'ChurchBronze', n, caps=False)
    f.lathe('Bronze_bell_lip', (cx, 0, z0), [(.035, .312), (0, .316), (0, .34)][::-1], 'ChurchBronze', n, caps=False)
    f.lathe('Bronze_bell_crown', (cx, 0, z0 + .60), [(0, .06), (.03, .05)], 'ChurchBronze', n)
    f.tube('Bell_clapper_shank', (cx, 0, z0 + .57), (cx, 0, z0 + .14), .014, 'DarkMetal', 6)
    f.lathe('Bell_clapper_ball', (cx, 0, z0 + .06), [(0, .02), (.03, .05), (.08, .04), (.11, .015)], 'DarkMetal', max(6, n // 2))
    f.box('Bell_oak_yoke', (cx, 0, z0 + .70), (.16, .96, .16), 'DarkWood')
    for y in (-.3, .3): f.box('Yoke_iron_strap', (cx, y, z0 + .60), (.18, .03, .2), 'DarkMetal')
    f.tube('Bell_gudgeon_axle', (cx, -.6, z0 + .70), (cx, .6, z0 + .70), .025, 'Metal', 8)
    for y in (-.55, .55): f.box('Gudgeon_bearing_block', (cx, y, 9.70), (.12, .1, .04), 'Metal')
    if lp['d']:
        f.torus('Bell_rope_wheel_rim', (cx, .66, z0 + .70), .22, .02, 'DarkWood', [20, 12, 8][lp_lod(lp)], 4, axis=(0, 1, 0))
        for a in range(4):
            ang = a * math.pi / 4
            f.tube('Bell_wheel_spoke', (cx - .21 * math.cos(ang), .66, z0 + .70 - .21 * math.sin(ang)), (cx + .21 * math.cos(ang), .66, z0 + .70 + .21 * math.sin(ang)), .012, 'DarkWood', 5)
        f.tube('Bell_rope', (cx + .22, .66, z0 + .70), (cx + .22, .66, 8.4), .012, 'Burlap', 5)


def porch(s, f, lp, rng):
    y_e, zpe, k2, xo, xi = 1.75, 3.22, .80, -1.05, XF - .005
    rz = zpe + y_e * k2
    # stone landing and brick edging, flush with the door threshold (<= 0.05 m)
    stones = [(-.60, -.38, -1.75, -.95), (-.60, -.40, -.95, -.05), (-.60, -.36, -.05, .80), (-.60, -.42, .80, 1.75), (-.38, XF, -1.75, -.70), (-.40, XF, -.70, .35), (-.36, XF, .35, 1.0), (-.42, XF, 1.0, 1.75)]
    for i, (x0, x1, y0, y1) in enumerate(stones if lp['d'] else [(-.60, XF, -1.75, 1.75)]):
        g = .008 if lp['d'] else 0
        bx(s, 'Flagstone_landing', x0 + g, x1 - g, y0 + g, y1 - g, -.10, .028 + (.006 * ((i * 7) % 3) / 2 if lp['d'] else 0), 'ChurchStone')
    if lp['d'] == 2:
        y = -1.95
        while y < 1.95 - .01:
            bx(s, 'Soldier_brick_edging', -.70, -.60, y + .004, min(y + .21, 1.95) - .004, -.12, .04, 'ChurchBrick'); y += .21
        for sd in (-1, 1):
            x = -.70
            while x < XF - .01:
                y0, y1 = sorted([sd * 1.75, sd * 1.95]); bx(s, 'Soldier_brick_edging', x + .004, min(x + .21, XF) - .004, y0, y1, -.12, .04, 'ChurchBrick'); x += .21
    else:
        bx(s, 'Brick_edging', -.70, -.60, -1.95, 1.95, -.12, .04, 'ChurchBrick')
        for sd in (-1, 1): y0, y1 = sorted([sd * 1.75, sd * 1.95]); bx(s, 'Brick_edging', -.70, XF, y0, y1, -.12, .04, 'ChurchBrick')
    # posts within 0.6 m of the wall
    for y in (-1.45, 1.45):
        bx(s, 'Porch_post_plinth', -.64, -.46, y - .09, y + .09, .03, .30, 'ChurchTrim')
        bx(s, 'Porch_post', -.615, -.485, y - .065, y + .065, .30, 2.92, 'ChurchTrim')
        bx(s, 'Porch_post_capital', -.635, -.465, y - .085, y + .085, 2.92, 2.98, 'ChurchTrim')
        if lp['d']:
            sg = -1 if y > 0 else 1
            prism(s, 'Porch_scroll_bracket', [(0, 2.98), (sg * .38, 2.98), (sg * .06, 2.62), (0, 2.62)], lambda u, z, y=y, sg=sg: (-.575, y + sg * .065 + u, z), (.05, 0, 0), 'ChurchTrim')
            prism(s, 'Porch_scroll_bracket', [(0, 2.98), (.30, 2.98), (.05, 2.70), (0, 2.70)], lambda u, z, y=y: (-.485 + u, y - .025, z), (0, .05, 0), 'ChurchTrim')
        # side railing post to wall
        bx(s, 'Porch_rail_top', -.49, XF, y - .03, y + .03, .86, .92, 'ChurchTrim')
        bx(s, 'Porch_rail_bottom', -.49, XF, y - .025, y + .025, .12, .17, 'ChurchTrim')
        if lp['d']:
            for x in (-.40, -.29, -.18):
                bx(s, 'Porch_rail_baluster', x - .02, x + .02, y - .02, y + .02, .17, .86, 'ChurchTrim')
    bx(s, 'Porch_header_beam', -.63, -.47, -1.62, 1.62, 2.98, 3.18, 'ChurchTrim')
    for y in (-1.45, 1.45): bx(s, 'Porch_side_beam', -.47, XF, y - .07, y + .07, 2.98, 3.18, 'ChurchTrim')
    bx(s, 'Porch_beadboard_ceiling', -.63, XF, -1.62, 1.62, 3.18, 3.20, 'ChurchTrim')
    # pediment
    def zp(y): return zpe + (y_e - abs(y)) * k2
    ped = [(-1.62, 3.18), (1.62, 3.18), (1.62, zp(1.62)), (0, rz), (-1.62, zp(1.62))]
    prism(s, 'Porch_pediment', ped, lambda y, z: (-.60, y, z), (-.05, 0, 0), 'ChurchWhiteBoard')
    for sd in (-1, 1):
        a = sd * 1.62
        prism(s, 'Pediment_rake_trim', [(a, zp(a) - .16), (0, rz - .16), (0, rz), (a, zp(a))], lambda y, z: (-.65, y, z), (-.03, 0, 0), 'ChurchTrim')
        bx(s, 'Porch_side_soffit', -.65, XF, *sorted([sd * 1.62, sd * y_e]), 3.18, 3.20, 'ChurchTrim')
    bx(s, 'Pediment_bed_mould', -.68, -.60, -1.66, 1.66, 3.16, 3.22, 'ChurchTrim')
    if lp['d']:
        prism(s, 'Pediment_cross_panel', [(-.09, 3.52), (.09, 3.52), (.09, 3.72), (-.09, 3.72)], lambda y, z: (-.68, y, z), (.03, 0, 0), 'ChurchTrim')
        prism(s, 'Pediment_cross', [(-.022, 3.38), (.022, 3.38), (.022, 3.92), (-.022, 3.92)], lambda y, z: (-.69, y, z), (.02, 0, 0), 'DarkWood')
        prism(s, 'Pediment_cross', [(-.14, 3.76), (.14, 3.76), (.14, 3.80), (-.14, 3.80)], lambda y, z: (-.69, y, z), (.02, 0, 0), 'DarkWood')
    # porch roof
    for sd in (-1, 1):
        pts = [(sd * y_e, zpe), (0, rz), (0, rz + .08), (sd * y_e, zpe + .08)]
        prism(s, 'Porch_roof_deck', pts, lambda y, z: (xo, y, z), (xi - xo, 0, 0), 'ChurchTrim', sides={0, 2, 3}, mats={2: 'RoofShingle'})
        nout = unit((0, sd * k2, 1))
        if lp['courses']:
            shingled(s, (xo, sd * y_e, zpe + .08), (xi, sd * y_e, zpe + .08), (xi, 0, rz + .08), (xo, 0, rz + .08), nout, 16 if lp['lod'] < 2 else 8, dict(lp, shseg=.5 if lp['shseg'] else None), rng)
        prism(s, 'Porch_rake_fascia', [(sd * (y_e + .03), zpe - .05), (0, rz - .05), (0, rz + .12), (sd * (y_e + .03), zpe + .12)], lambda y, z: (xo, y, z), (-.03, 0, 0), 'ChurchTrim', sides={0, 2, 3})
        y0, y1 = sorted([sd * y_e, sd * (y_e + .03)])
        bx(s, 'Porch_eave_fascia', xo - .03, xi, y0, y1, zpe - .05, zpe + .1, 'ChurchTrim')
        s.tag = 'Porch_ridge_cap'
        F(s, [(xo - .03, 0, rz + .1), (xi, 0, rz + .1), (xi, sd * .12, rz + .1 - .12 * k2), (xo - .03, sd * .12, rz + .1 - .12 * k2)], 'RoofShingle', (0, sd * k2, 1))
    # ceiling jelly-jar porch light
    f.lathe('Porch_light_canopy', (-.37, 0, 3.12), [(0, .07), (.06, .07), (.08, .04)], 'DarkMetal', 12 if lp['d'] else 8)
    f.lathe('Porch_light_lens', (-.37, 0, 2.96), [(0, .03), (.02, .058), (.13, .058), (.16, .05)], 'ChurchPorchGlow', 12 if lp['d'] else 8)
    if lp['d']:
        for a in range(4):
            ang = a * math.pi / 2 + math.pi / 4
            f.tube('Porch_light_cage_bar', (-.37 + .066 * math.cos(ang), .066 * math.sin(ang), 2.97), (-.37 + .066 * math.cos(ang), .066 * math.sin(ang), 3.12), .005, 'DarkMetal', 4)
        f.torus('Porch_light_cage_ring', (-.37, 0, 3.02), .066, .006, 'DarkMetal', 12, 4)


def door(s, f, lp):
    d0, dw, dh = B['door']; W = FRONT; hw = dw / 2
    # jamb lining and oak threshold
    for a, b in [(d0 - hw, d0 - hw + .025), (d0 + hw - .025, d0 + hw)]:
        W.rect(s, 'Door_jamb_lining', a, b, 0, dh, -WT, 0, 'ChurchTrim')
    W.rect(s, 'Door_head_lining', d0 - hw, d0 + hw, dh - .025, dh, -WT, 0, 'ChurchTrim')
    W.rect(s, 'Oak_threshold', d0 - hw, d0 + hw, 0, .035, -WT, .05, 'Timber')
    # casing with cornice head
    for a, b in [(d0 - hw - .14, d0 - hw), (d0 + hw, d0 + hw + .14)]:
        W.rect(s, 'Door_casing', a, b, .035, dh, 0, .04, 'ChurchTrim')
        W.rect(s, 'Door_plinth_block', a - .01, b + .01, .035, .26, 0, .05, 'ChurchTrim')
    W.rect(s, 'Door_head_casing', d0 - hw - .17, d0 + hw + .17, dh, dh + .17, 0, .045, 'ChurchTrim')
    W.rect(s, 'Door_head_cornice', d0 - hw - .22, d0 + hw + .22, dh + .17, dh + .22, 0, .075, 'ChurchTrim')
    # fixed closed double plank doors
    W.rect(s, 'Door_backing_boards', d0 - hw + .025, d0 + hw - .025, .035, dh - .025, -.10, -.06, 'DarkWood')
    for side in (-1, 1):
        u0, u1 = sorted([d0 + side * .003, d0 + side * (hw - .025)])
        n = 6 if lp['d'] else 1; pw = (u1 - u0) / n
        for k in range(n):
            g = .0035 if n > 1 else 0
            W.rect(s, 'Door_vertical_plank', u0 + k * pw + g, u0 + (k + 1) * pw - g, .04, dh - .03, -.06, -.016 - (.004 * ((k * 5 + side) % 3) if lp['d'] else 0), 'Timber')
        if lp['d']:
            for z in (.45, 1.45, 2.45): W.rect(s, 'Door_interior_ledge', u0 + .04, u1 - .04, z - .08, z + .08, -.13, -.10, 'Timber')
        # strap hinges, pintles, ring pull
        if True:
            for z in (.45, 1.45, 2.45):
                e = d0 + side * (hw - .025); tipu = e - side * .62
                a, b = sorted([e, tipu])
                W.rect(f, 'Strap_hinge', a, b, z - .024, z + .024, -.016, -.007, 'DarkMetal')
                if lp['d']:
                    W.prism(f, 'Strap_hinge_spear', [(tipu, z - .036), (tipu - side * .07, z), (tipu, z + .036)], -.016, -.007, 'DarkMetal')
                    W.rect(f, 'Hinge_pintle', *sorted([e - side * .01, e + side * .03]), z - .05, z + .05, -.03, -.0, 'DarkMetal')
        ux = d0 + side * .14
        if lp['lod'] < 2: f.lathe('Ring_pull_escutcheon', W.p(ux, 1.10, -.016), [(0, .045), (.008, .045), (.014, .028)], 'DarkMetal', 10 if lp['d'] else 6, axis=W.N)
        f.torus('Iron_ring_pull', W.p(ux, 1.045, -.004), .052, .008, 'DarkMetal', [12, 8, 6][lp['lod']], 4 if lp['lod'] < 2 else 3, axis=W.N)
        if side > 0: W.rect(f, 'Keyhole_plate', ux - .02, ux + .02, .90, 1.0, -.016, -.010, 'DarkMetal')


def notice_board(s, lp):
    W = FRONT; c, z0, z1, hw = -2.05, 1.0, 1.75, .40
    if c - hw < -2.47: c = -2.47 + hw
    W.rect(s, 'Notice_board_back', c - hw, c + hw, z0, z1, .025, .055, 'Timber')
    for a, b, za, zb in [(c - hw - .04, c - hw, z0 - .04, z1 + .04), (c + hw, c + hw + .04, z0 - .04, z1 + .04), (c - hw, c + hw, z0 - .04, z0), (c - hw, c + hw, z1, z1 + .04)]:
        W.rect(s, 'Notice_board_frame', a, b, za, zb, .025, .075, 'DarkWood')
    W.rect(s, 'Notice_board_header', c - hw - .06, c + hw + .06, z1 + .04, z1 + .16, .025, .07, 'DarkWood')
    if lp['d']:
        sheets = [(-.30, 1.30, .20, .27), (-.04, 1.42, .21, .28), (.22, 1.28, .16, .22), (-.18, 1.10, .26, .12), (.18, 1.08, .15, .11)] if lp['d'] == 2 else [(-.1, 1.35, .5, .4)]
        for du, z, ww, hh in sheets:
            W.rect(s, 'Faded_notice_sheet', c + du - ww / 2, c + du + ww / 2, z - hh / 2, z + hh / 2, .055, .058, 'ChurchOldPaper')
        # small gabled weather hood
        for sd in (-1, 1):
            a, b = c, c + sd * (hw + .1)
            W.prism(s, 'Notice_board_hood', [(a, z1 + .30), (b, z1 + .16), (b, z1 + .19), (a, z1 + .33)], 0, .16, 'RoofShingle')


def gutters(f, lp):
    # K-style (ogee) aluminium gutter, typical of a 1970s/80s re-roof; solid profile, ends flush with the roof
    prof = [(0, -.11), (.10, -.11), (.125, -.085), (.112, -.05), (.14, -.018), (.14, 0), (0, 0)] if lp['d'] else [(0, -.11), (.13, -.11), (.14, 0), (0, 0)]
    for sd in (-1, 1):
        y0, zc = sd * (YE + .03), ZE + DECK + .005
        prism(f, 'K_style_gutter', [(sd * u, v) for u, v in prof], lambda u, v, y0=y0: (XF - .25, y0 + u, zc + v), (XR - XF + .5, 0, 0), 'ChurchGalv')
        yc = y0 + sd * .06; zc = zc - .05
        for x in (XF + .25, XR - .25):
            yw = sd * (YS + .085)
            beam(f, 'Downspout_offset', (x, yc, zc - .06), (x, yw, 3.95), .07, .09, 'ChurchGalv', up=(1, 0, 0))
            bx(f, 'Downspout', x - .045, x + .045, *sorted([yw - .035, yw + .035]), .30, 3.97, 'ChurchGalv')
            beam(f, 'Downspout_shoe', (x, yw, .32), (x, sd * (YS + .42), .06), .07, .09, 'ChurchGalv', up=(1, 0, 0))
            if lp['d']:
                for z in (1.2, 2.6): bx(f, 'Downspout_strap', x - .055, x + .055, *sorted([sd * (YS + .036), sd * (YS + .13)]), z, z + .03, 'ChurchGalv')
                bx(f, 'Concrete_splash_block', x - .16, x + .16, *sorted([sd * (YS + .2), sd * (YS + .58)]), -.04, .03, 'Concrete')


def stovepipe(f, lp, x=1.0, y=-3.6):
    zr = zu(y) + DECK
    n = 12 if lp['d'] else 8
    f.tube('Stovepipe_through_roof', (x, y, zr - .3), (x, y, zr + 1.3), .085, 'ChurchGalv', n)
    f.lathe('Stovepipe_roof_flashing', (x, y, zr - .06), [(0, .26), (.12, .10)], 'ChurchGalv', n)
    f.lathe('Stovepipe_storm_collar', (x, y, zr + .25), [(0, .13), (.05, .088)], 'ChurchGalv', n)
    f.lathe('Stovepipe_rain_cap', (x, y, zr + 1.42), [(0, .20), (.13, .03)], 'ChurchGalv', n)
    if lp['d']:
        for a in range(3):
            ang = a * 2 * math.pi / 3
            f.tube('Rain_cap_leg', (x + .08 * math.cos(ang), y + .08 * math.sin(ang), zr + 1.28), (x + .15 * math.cos(ang), y + .15 * math.sin(ang), zr + 1.44), .008, 'ChurchGalv', 4)


# ---------------------------------------------------------------- interior
def pew(i, xp, yc, lp):
    L = 2.34
    if lp['d'] == 0:
        bx(i, 'Pew_seat', xp - .25, xp + .25, yc - L / 2, yc + L / 2, .42, .47, 'Timber')
        bx(i, 'Pew_back', xp - .33, xp - .27, yc - L / 2, yc + L / 2, .47, .97, 'Timber')
        for y in (yc - L / 2 - .03, yc + L / 2 + .03): bx(i, 'Pew_end', xp - .35, xp + .27, y - .028, y + .028, 0, .9, 'Timber')
        return
    bx(i, 'Pew_seat', xp - .25, xp + .25, yc - L / 2, yc + L / 2, .42, .465, 'Timber')
    bx(i, 'Pew_seat_apron', xp + .21, xp + .25, yc - L / 2, yc + L / 2, .33, .42, 'Timber')
    tilt = unit((-.07, 0, .5))
    obox(i, 'Pew_inclined_back', (xp - .285, yc, .71), (0, 1, 0), tilt, cross((0, 1, 0), tilt), (L, .50, .035), 'Timber')
    obox(i, 'Pew_back_cap_rail', (xp - .318, yc, .965), (0, 1, 0), tilt, cross((0, 1, 0), tilt), (L, .05, .06), 'DarkWood')
    prof = [(-.33, 0), (.27, 0), (.27, .58), (.235, .64), (.12, .68), (-.04, .76), (-.17, .88), (-.25, .97), (-.31, 1.0), (-.36, .98), (-.37, .92)]
    for y in (yc - L / 2 - .028, yc + L / 2 + .028):
        prism(i, 'Pew_shaped_end', prof, lambda u, z, y=y: (xp + u, y - .028, z), (0, .056, 0), 'Timber')
    bx(i, 'Hymnal_rack_shelf', xp - .44, xp - .31, yc - L / 2 + .05, yc + L / 2 - .05, .60, .615, 'DarkWood')
    bx(i, 'Hymnal_rack_lip', xp - .45, xp - .43, yc - L / 2 + .05, yc + L / 2 - .05, .615, .72, 'DarkWood')
    if lp['d'] == 2:
        rng = random.Random(int(xp * 100 + yc * 10))
        for k in range(3):
            y = yc - .8 + k * .7 + rng.uniform(-.1, .1)
            bx(i, 'Hymnal', xp - .425, xp - .33, y - .03, y + .03, .615, .82, 'BookCloth')


def interior(i, f, lp):
    d = lp['d']
    # plank floor
    bx(i, 'Subfloor', XF + WT, XR - WT, -YS + WT, YS - WT, -.06, -.01, 'DarkWood')
    if d:
        y = -YS + WT; k = 0; w = .145 if d == 2 else .29
        while y < YS - WT - .01:
            y1 = min(y + w, YS - WT); i.tag = 'Worn_floor_plank'
            F(i, [(XF + WT, y + .003, 0), (XR - WT, y + .003, 0), (XR - WT, y1 - .003, 0), (XF + WT, y1 - .003, 0)], 'Timber', (0, 0, 1)); y = y1; k += 1
    else:
        bx(i, 'Plank_floor', XF + WT, XR - WT, -YS + WT, YS - WT, -.01, 0, 'Timber')
    bx(i, 'Aisle_runner', .5, 11.55, -.55, .55, 0, .008, 'Carpet')
    # wainscot, chair rail, skirting
    walls = [(SIDES[-1], [(XF + WT, XR - WT)]), (SIDES[1], [(XF + WT, XR - WT)]), (FRONT, [(-YS + WT, -1.22), (1.22, YS - WT)]), (REAR, [(-YS + WT, YS - WT)])]
    for w, spans in walls:
        for a, b in spans:
            w.rect(i, 'Wainscot_board', a, b, .12, 1.0, -WT - .016, -WT, 'Clapboard')
            w.rect(i, 'Chair_rail', a, b, .98, 1.045, -WT - .034, -WT, 'Timber')
            w.rect(i, 'Skirting_board', a, b, 0, .14, -WT - .024, -WT, 'DarkWood')
            if d == 2:
                u = a + .45
                while u < b - .1:
                    w.rect(i, 'Wainscot_batten', u - .012, u + .012, .14, .98, -WT - .026, -WT - .016, 'Clapboard'); u += .45
    # interior door casing
    FRONT.rect(i, 'Interior_door_casing', -1.22, -1.1, 0, 2.9, -WT - .025, -WT, 'Timber')
    FRONT.rect(i, 'Interior_door_casing', 1.1, 1.22, 0, 2.9, -WT - .025, -WT, 'Timber')
    FRONT.rect(i, 'Interior_door_head', -1.22, 1.22, 2.9, 3.06, -WT - .025, -WT, 'Timber')
    # vaulted board ceiling with collar flat
    yi, zc = YS - WT, 4.2 + (YS - WT - 1.8) * K
    for sd in (-1, 1):
        i.tag = 'Vaulted_board_ceiling'
        F(i, [(XF + WT, sd * yi, EAVE), (XR - WT, sd * yi, EAVE), (XR - WT, sd * 1.8, zc), (XF + WT, sd * 1.8, zc)], 'Clapboard', (0, -sd * K, -1))
    i.tag = 'Collar_flat_ceiling'
    F(i, [(XF + WT, -1.8, zc), (XR - WT, -1.8, zc), (XR - WT, 1.8, zc), (XF + WT, 1.8, zc)], 'Clapboard', (0, 0, -1))
    if d:
        for sd in (-1, 1):
            for t in ([.2, .4, .6, .8] if d == 2 else [.5]):
                y = sd * (yi + (1.8 - yi) * t); z = EAVE + (zc - EAVE) * t
                beam(i, 'Ceiling_board_batten', (XF + WT, y, z - .015), (XR - WT, y, z - .015), .05, .03, 'Timber')
        for y in ([-.9, 0, .9] if d == 2 else [0]): bx(i, 'Ceiling_board_batten', XF + WT, XR - WT, y - .025, y + .025, zc - .03, zc, 'Timber')
        for sd in (-1, 1): bx(i, 'Ceiling_cornice', XF + WT, XR - WT, *sorted([sd * yi, sd * (yi - .07)]), EAVE - .12, EAVE, 'Timber')
    # scissor-braced trusses with wall posts
    for x in (1.0, 5.0, 9.0, 13.0):
        for sd in (-1, 1):
            beam(i, 'Truss_rafter', (x, sd * (yi - .02), EAVE - .1), (x, sd * 1.8, zc - .1), .12, .2, 'DarkWood', up=(0, 0, 1))
            bx(i, 'Truss_wall_post', x - .07, x + .07, *sorted([sd * yi, sd * (yi - .12)]), 2.95, EAVE, 'DarkWood')
            bx(i, 'Truss_post_corbel', x - .09, x + .09, *sorted([sd * yi, sd * (yi - .16)]), 2.82, 2.95, 'DarkWood')
            if d: beam(i, 'Truss_knee_brace', (x, sd * (yi - .1), 3.3), (x, sd * (yi - 1.05), EAVE + .6), .1, .1, 'DarkWood', up=(1, 0, 0))
        bx(i, 'Truss_collar_tie', x - .06, x + .06, -1.85, 1.85, zc - .24, zc - .02, 'DarkWood')
        if d: bx(i, 'Truss_king_strut', x - .05, x + .05, -.05, .05, zc - .5, zc - .24, 'DarkWood')
    # pews: eight pairs, TB1 positions
    for xp in [2.3, 3.5, 4.7, 5.9, 7.1, 8.3, 9.5, 10.7]:
        for yc in (-2.25, 2.25): pew(i, xp, yc, lp)
    # chancel
    bx(i, 'Chancel_dais', 11.8, XR - WT, -YS + WT, 2.55, 0, .20, 'Timber')
    bx(i, 'Dais_nosing', 11.77, 11.83, -YS + WT, 2.55, .17, .21, 'DarkWood')
    bx(i, 'Chancel_carpet', 11.9, XR - WT - .1, -1.2, 1.2, .20, .208, 'Carpet')
    for y0, y1 in ((-3.3, -.6), (.6, 2.4)):
        for y in (y0, y1): bx(i, 'Altar_rail_newel', 11.6, 11.7, y - .05, y + .05, 0, .92, 'DarkWood')
        bx(i, 'Altar_rail_top', 11.57, 11.73, y0, y1, .86, .92, 'DarkWood')
        bx(i, 'Altar_rail_bottom', 11.62, 11.68, y0, y1, .06, .12, 'DarkWood')
        if d:
            step = .15 if d == 2 else .3; y = y0 + step
            while y < y1 - .05:
                i.lathe('Turned_rail_baluster', (11.65, y, .12), [(0, .02), (.1, .028), (.35, .016), (.6, .024), (.74, .018)], 'Timber', 6 if d == 2 else 4); y += step
        bx(i, 'Rail_kneeler', 11.38, 11.56, y0, y1, 0, .10, 'Upholstery')
    # altar
    bx(i, 'Altar_body', 12.95, 13.65, -1.0, 1.0, .20, 1.12, 'DarkWood')
    bx(i, 'Altar_mensa', 12.9, 13.7, -1.07, 1.07, 1.12, 1.17, 'Timber')
    bx(i, 'Altar_linen', 12.89, 13.71, -1.0, 1.0, 1.17, 1.175, 'LampFabric')
    bx(i, 'Altar_frontal', 12.925, 12.95, -.38, .38, .55, 1.17, 'LampFabric')
    if d:
        for y in (-.85, .85): bx(i, 'Altar_panel_moulding', 12.935, 12.95, y - .1, y + .1, .35, 1.0, 'Timber')
    # rear wall dossal, cross, hymn board
    bx(i, 'Dossal_panel', XR - WT - .02, XR - WT, -.9, .9, 1.3, 3.9, 'Clapboard')
    for y0, y1, z0, z1 in [(-.98, -.9, 1.3, 3.98), (.9, .98, 1.3, 3.98), (-.98, .98, 3.9, 3.98)]:
        bx(i, 'Dossal_frame', XR - WT - .05, XR - WT, y0, y1, z0, z1, 'DarkWood')
    bx(i, 'Altar_cross_upright', XR - WT - .1, XR - WT - .02, -.05, .05, 1.85, 3.55, 'DarkWood')
    bx(i, 'Altar_cross_arm', XR - WT - .1, XR - WT - .02, -.48, .48, 2.93, 3.03, 'DarkWood')
    bx(i, 'Hymn_board', XR - WT - .04, XR - WT, -2.75, -2.15, 1.55, 2.55, 'DarkWood')
    bx(i, 'Hymn_board_header', XR - WT - .05, XR - WT, -2.78, -2.12, 2.55, 2.66, 'DarkWood')
    if d:
        for k in range(4):
            z = 1.67 + k * .22
            bx(i, 'Hymn_number_card', XR - WT - .055, XR - WT - .04, -2.68, -2.22, z, z + .15, 'Paper')
            bx(i, 'Hymn_card_ledge', XR - WT - .07, XR - WT - .04, -2.7, -2.2, z - .02, z, 'DarkWood')
    # pulpit / lectern
    bx(i, 'Pulpit_body', 12.0, 12.55, -2.05, -1.45, .20, 1.10, 'DarkWood')
    bx(i, 'Pulpit_base', 11.97, 12.58, -2.08, -1.42, .20, .30, 'DarkWood')
    tilt = unit((.35, 0, 1))
    obox(i, 'Pulpit_sloped_desk', (12.29, -1.75, 1.16), (0, 1, 0), cross(tilt, (0, 1, 0)), tilt, (.68, .55, .05), 'Timber')
    if d: obox(i, 'Pulpit_bible', (12.28, -1.75, 1.205), (0, 1, 0), cross(tilt, (0, 1, 0)), tilt, (.30, .24, .05), 'BookCloth')
    if d == 2:
        for y in (-2.05, -1.45): bx(i, 'Pulpit_raised_panel', 11.985, 12.0, y + (.08 if y < -1.5 else -.22), y + (.22 if y < -1.5 else -.08), .4, .95, 'Timber')
    # upright piano and bench
    bx(i, 'Upright_piano_case', 12.1, 13.5, -YS + WT, -YS + WT + .58, .20, 1.45, 'DarkWood')
    bx(i, 'Piano_lid', 12.07, 13.53, -YS + WT, -YS + WT + .62, 1.45, 1.49, 'DarkWood')
    bx(i, 'Piano_key_bed', 12.15, 13.45, -YS + WT + .58, -YS + WT + .85, .86, .92, 'DarkWood')
    bx(i, 'Piano_keys', 12.2, 13.4, -YS + WT + .6, -YS + WT + .82, .92, .945, 'Ceramic')
    bx(i, 'Piano_fallboard', 12.15, 13.45, -YS + WT + .58, -YS + WT + .62, .945, 1.12, 'DarkWood')
    if d:
        for k in range(18):
            xk = 12.24 + k * 1.12 / 18
            if k % 7 in (2, 6): continue
            bx(i, 'Piano_black_key', xk, xk + .03, -YS + WT + .62, -YS + WT + .74, .945, .965, 'DarkMetal')
        for x in (12.15, 13.45): bx(i, 'Piano_toe_block', x - .04, x + .04, -YS + WT + .58, -YS + WT + .85, .20, .86, 'DarkWood')
        i.lathe('Piano_vase', (12.4, -YS + WT + .3, 1.49), [(0, .05), (.12, .08), (.22, .04), (.26, .05)], 'Ceramic', 10 if d == 2 else 6)
        for a in range(5 if d == 2 else 2):
            ang = a * 1.3
            i.tube('Dried_flower_stalk', (12.4, -YS + WT + .3, 1.7), (12.4 + .14 * math.cos(ang), -YS + WT + .3 + .14 * math.sin(ang), 2.05 + .05 * (a % 2)), .006, 'Burlap', 4)
    bx(i, 'Piano_bench', 12.4, 13.1, -YS + WT + .95, -YS + WT + 1.3, .62, .67, 'DarkWood')
    for x in (12.45, 13.05):
        for y in (-YS + WT + 1.0, -YS + WT + 1.25): bx(i, 'Bench_leg', x - .025, x + .025, y - .025, y + .025, .20, .62, 'DarkWood')
    # pot-belly stove, its pipe continues outside (Fixtures)
    sx, sy = 1.0, -3.6
    bx(i, 'Stove_floor_plate', sx - .45, sx + .45, sy - .45, sy + .45, 0, .006, 'Metal')
    n = [16, 10, 6][lp_lod(lp)]
    i.lathe('Potbelly_stove', (sx, sy, .14), [(0, .2), (.05, .23), (.12, .2), (.35, .27), (.55, .26), (.68, .2), (.74, .22), (.8, .17)], 'DarkMetal', n)
    for a in range(4):
        ang = a * math.pi / 2 + .6
        i.tube('Stove_leg', (sx + .16 * math.cos(ang), sy + .16 * math.sin(ang), 0), (sx + .17 * math.cos(ang), sy + .17 * math.sin(ang), .16), .022, 'DarkMetal', 5)
    i.tube('Interior_stovepipe', (sx, sy, .94), (sx, sy, EAVE + (YS - WT - abs(sy)) * K + .02), .075, 'DarkMetal', n // 2 + 2)
    if d: i.box('Stove_door', (sx - .25, sy, .45), (.04, .2, .18), 'DarkMetal')
    # sacristy: real enclosed corner room with doorway
    A = Wall((11.65, 0, 0), (0, 1, 0), (1, 0, 0)); Bw = Wall((0, 2.65, 0), (1, 0, 0), (0, 1, 0))
    A.core(i, 2.55, YS - WT, 0, 2.6, [(3.1, 3.9, 0, 2.1)], 'Clapboard', -.1, 0)
    Bw.core(i, 11.55, XR - WT, 0, 2.6, [], 'Clapboard', -.1, 0)
    bx(i, 'Sacristy_cap_rail', 11.52, 11.68, 2.52, YS - WT, 2.6, 2.65, 'Timber')
    bx(i, 'Sacristy_cap_rail', 11.52, XR - WT, 2.52, 2.68, 2.6, 2.65, 'Timber')
    for a, b in ((3.04, 3.1), (3.9, 3.96)): bx(i, 'Sacristy_door_casing', 11.52, 11.68, a, b, 0, 2.16, 'Timber')
    bx(i, 'Sacristy_door_casing', 11.52, 11.68, 3.04, 3.96, 2.1, 2.16, 'Timber')
    cabinet(i, 13.55, 3.75, .50, .75, 1.65, 3, 1 if d else 2, 'DarkWood')
    chair(i, 12.65, 3.6, 1 if d else 2)
    if d:
        bx(i, 'Sacristy_table', 12.2, 13.0, 4.0, 4.38, .74, .78, 'Timber')
        for x in (12.25, 12.95): bx(i, 'Sacristy_table_leg', x - .025, x + .025, 4.05, 4.33, 0, .74, 'Timber')
        bx(i, 'Parish_register', 12.4, 12.7, 4.1, 4.32, .78, .83, 'BookCloth')
        bx(i, 'Hanging_vestment', 13.88 - .12, 13.88 - .05, 2.85, 3.35, .9, 2.0, 'LampFabric')
    # guest book stand by the door
    bx(i, 'Guest_book_stand', .4, .75, 2.6, 3.1, 0, .95, 'DarkWood')
    if d: bx(i, 'Guest_book', .45, .7, 2.7, 3.0, .95, .985, 'Paper')
    # candles (Fixtures)
    for y in (-.7, .7):
        f.lathe('Brass_candlestick', (13.3, y, 1.175), [(0, .06), (.02, .06), (.05, .02), (.2, .015), (.24, .03), (.26, .03)] if lp['lod'] == 0 else [(0, .06), (.05, .02), (.26, .03)], 'ChurchBrass', [12, 6, 4][lp_lod(lp)])
        f.lathe('Altar_candle', (13.3, y, 1.435), [(0, .017), (.28, .017)], 'Paper', [10, 6, 4][lp_lod(lp)])
        f.lathe('Candle_flame', (13.3, y, 1.72), [(0, .004), (.02, .011), (.05, .002)], 'ChurchFlameGlow', 6)


def lights(f, lp):
    n = [16, 8, 5][lp_lod(lp)]
    zc = 4.2 + (YS - WT - 1.8) * K
    dead = (7.0, 1.6)
    for x in (3.5, 7.0, 10.5):
        for y in (-1.6, 1.6):
            if lp['lod'] == 0: f.lathe('Pendant_canopy', (x, y, zc - .04), [(0, .08), (.04, .06)], 'ChurchBrass', max(6, n // 2))
            f.tube('Pendant_rod', (x, y, zc - .04), (x, y, 4.1), .009, 'ChurchBrass', 5)
            f.lathe('Pendant_fitter', (x, y, 3.98), [(0, .05), (.12, .045), (.14, .03)] if lp['lod'] == 0 else [(0, .05), (.14, .03)], 'ChurchBrass', n)
            mat = 'LampFabric' if (x, y) == dead else 'ChurchLampGlow'
            f.lathe('Schoolhouse_globe', (x, y, 3.70), [(0, .02), (.04, .09), (.14, .12), (.24, .09), (.28, .05)] if lp['lod'] < 2 else [(0, .04), (.14, .12), (.28, .05)], mat, n)
    for sd in (-1, 1):
        for x in (5.0, 9.0):
            yw = sd * (YS - WT)
            if lp['lod'] < 2: f.lathe('Sconce_backplate', (x, yw, 2.35), [(0, .06), (.015, .06), (.02, .04)], 'ChurchBrass', max(6, n // 2), axis=(0, -sd, 0))
            f.tube('Sconce_arm', (x, yw - sd * .01, 2.35), (x, yw - sd * .16, 2.42), .01, 'ChurchBrass', 5)
            if lp['lod'] < 2: f.lathe('Sconce_cup', (x, yw - sd * .17, 2.40), [(0, .025), (.04, .04)], 'ChurchBrass', max(6, n // 2))
            f.lathe('Sconce_tulip_shade', (x, yw - sd * .17, 2.44), [(0, .03), (.08, .07), (.16, .06)], 'ChurchLampGlow', n)


def meter(f, lp):
    # 1980s electric service on the -Y wall near the rear: meter, conduit, weatherhead
    W = SIDES[-1]; u = 13.1
    W.rect(f, 'Meter_base', u - .14, u + .14, 1.35, 1.75, .036, .14, 'ChurchGalv')
    if lp['lod'] < 2:
        f.lathe('Meter_glass_dome', W.p(u, 1.57, .14), [(0, .10), (.08, .10), (.12, .07)], 'Metal', 10 if lp['lod'] == 0 else 6, axis=W.N)
    f.tube('Service_conduit', W.p(u, 1.75, .07), W.p(u, 4.6, .07), .025, 'ChurchGalv', 6)
    f.lathe('Weatherhead', W.p(u, 4.6, .07), [(0, .035), (.06, .05), (.1, .02)], 'DarkMetal', 6)
    W.rect(f, 'Disconnect_box', u + .25, u + .5, 1.3, 1.65, .036, .13, 'ChurchGalv')


# ---------------------------------------------------------------- build
def build(lod):
    lp = LODP[lod]; rng = random.Random(1986 + lod); WEATHER.seed(47)
    s = Mesh('SM_Town_Church_Shell'); g = Mesh('SM_Town_Church_Glass'); i = Mesh('SM_Town_Church_Interior'); f = Mesh('SM_Town_Church_Fixtures')
    vent_z = 6.0
    shell_walls(s, g, lp, [(0, lambda z: .33, vent_z - .55, vent_z + .60)])
    for w in (FRONT, REAR): gable_vent(s, w, lp, vent_z)
    roof(s, lp, rng)
    cx = tower(s, f, lp, rng)
    porch(s, f, lp, rng)
    door(s, f, lp)
    notice_board(s, lp)
    gutters(f, lp)
    stovepipe(f, lp)
    interior(i, f, lp)
    lights(f, lp)
    meter(f, lp)
    return {m.name: m for m in (s, g, i, f)}


if __name__ == '__main__':
    for lod in (0, 1, 2):
        parts = build(lod)
        print(lod, {k: sum(len(fc) - 2 for fc in m.f) for k, m in parts.items()})
