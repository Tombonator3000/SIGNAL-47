"""LAND-PLACES kit (14 November 2026): the geometry of the land places' models, in plain Python (no bpy), so a model can be
measured and counted without Blender. A model is a set of parts; a part has one mesh per detail level (LOD); every mesh is
made of primitives (sweep, revolve, sheet) that lay out their own UV charts in the part's atlas. A chart is asked for by name:
the same primitive at a lower detail level lands on the same texels, so all levels wear the one baked set of maps.

Metres, Blender's frame (x east, y north, z up). The project's FBX recipe turns Blender (x, y, z) m into Unreal (x, -y, z) cm.
"""
import math

TAU = 2 * math.pi
LOD = 0


def N(*a):
    """A value by the detail level being built: N(12, 8, 6, 4)."""
    return a[min(LOD, len(a) - 1)]


# ---------------------------------------------------------------- vectors
def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))
def lerp(a, b, t): return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t)


def unit(a):
    n = length(a)
    return (a[0] / n, a[1] / n, a[2] / n) if n > 1e-12 else (0.0, 0.0, 1.0)


def rot_about(v, axis, ang):
    """v turned about the unit axis (Rodrigues)."""
    c, s = math.cos(ang), math.sin(ang)
    return add(add(mul(v, c), mul(cross(axis, v), s)), mul(axis, dot(axis, v) * (1 - c)))


def basis(z, ref=(0, 0, 1)):
    """X, Y, Z with Z along z and Y as near ref as it can be; X x Y = Z."""
    z = unit(z)
    y = sub(ref, mul(z, dot(ref, z)))
    if length(y) < 1e-6:
        y = sub((1, 0, 0), mul(z, z[0]))
    y = unit(y)
    return cross(y, z), y, z


def place(o, x, y, z):
    """A function that puts a local point into the frame (o; x, y, z)."""
    return lambda p: (o[0] + x[0] * p[0] + y[0] * p[1] + z[0] * p[2], o[1] + x[1] * p[0] + y[1] * p[1] + z[1] * p[2], o[2] + x[2] * p[0] + y[2] * p[1] + z[2] * p[2])


# ---------------------------------------------------------------- atlas
class Chart:
    __slots__ = ('key', 'w', 'h', 'dens', 'x', 'y', 'pw', 'ph', 'rot')

    def __init__(self, key, w, h, dens):
        self.key, self.w, self.h, self.dens = key, w, h, dens
        self.x = self.y = 0.0
        self.pw = self.ph = 1.0
        self.rot = False


class Atlas:
    """Rectangles in metres packed into a square of texels at one density (times each chart's own weight)."""

    def __init__(self, name, size=4096, pad=8, minpx=6):
        self.name, self.size, self.pad, self.minpx = name, size, pad, minpx
        self.charts, self.locked, self.scale, self.missed = {}, False, 0.0, []

    def chart(self, key, w, h, dens=1.0):
        c = self.charts.get(key)
        if c:
            return c
        if self.locked:                      # first asked for at a lower level: a spare corner
            self.missed.append(key)
            return self.charts['_spare']
        c = Chart(key, max(w, 1e-4), max(h, 1e-4), dens)
        self.charts[key] = c
        return c

    def _try(self, s, commit=False):
        S, pad = self.size, self.pad
        items = []
        for c in self.charts.values():
            w, h = max(self.minpx, c.w * c.dens * s), max(self.minpx, c.h * c.dens * s)
            rot = h > w
            if rot:
                w, h = h, w
            if w + pad > S:
                return False
            items.append((h, w, c, rot))
        items.sort(key=lambda t: (-t[0], -t[1], t[2].key))
        shelves, top = [], 0.0               # [y, height, x used]
        for h, w, c, rot in items:
            W, H = w + pad, h + pad
            for sh in shelves:
                if H <= sh[1] and sh[2] + W <= S:
                    break
            else:
                if top + H > S:
                    return False
                sh = [top, H, 0.0]
                shelves.append(sh)
                top += H
            if commit:
                c.x, c.y, c.pw, c.ph, c.rot = sh[2] + pad / 2, sh[0] + pad / 2, w, h, rot
            sh[2] += W
        return True

    def pack(self):
        self.chart('_spare', 0.03, 0.03)
        lo, hi = 1.0, 50000.0
        for _ in range(48):
            mid = (lo + hi) / 2
            if self._try(mid):
                lo = mid
            else:
                hi = mid
        self._try(lo, True)
        self.scale, self.locked = lo, True
        return lo

    def uv(self, c, s, t):
        if c.rot:
            s, t = t, 1.0 - s
        return ((c.x + s * c.pw) / self.size, (c.y + t * c.ph) / self.size)

    def fill(self):
        return sum(c.pw * c.ph for c in self.charts.values()) / float(self.size * self.size)


# ---------------------------------------------------------------- meshes
class Geo:
    def __init__(self):
        self.V, self.T, self.UV, self.M = [], [], [], []      # points, triangles, their (chart, s, t) corners, material names
        self.A, self.sharp = [], set()                        # per point (stain, tone); sharp edges (a < b)


class Part:
    def __init__(self, name, atlas, pivot=(0, 0, 0), group=0):
        self.name, self.atlas, self.pivot, self.group = name, atlas, pivot, group
        self.lods = {}

    def tris(self, lod=0):
        return len(self.lods[lod].T)

    def bounds(self, lod=0):
        V = self.lods[lod].V
        return [min(v[k] for v in V) for k in range(3)], [max(v[k] for v in V) for k in range(3)]


CUR = None      # the Geo being filled
ATL = None      # its atlas


def target(part):
    global CUR, ATL
    CUR = part.lods.setdefault(LOD, Geo())
    ATL = part.atlas


def emit(verts, faces, uvs, mat, sharp=35.0, tone=0.0, attr=None):
    """faces: triangles or quads of indices into verts; uvs: for each face, a (chart, s, t) per corner."""
    g, base = CUR, len(CUR.V)
    g.V.extend(verts)
    g.A.extend(((attr(v) if attr else 0.0), tone) for v in verts)
    tris, tuv = [], []
    for f, u in zip(faces, uvs):
        if len(f) == 3:
            tris.append(f); tuv.append(u)
        else:
            a, b, c, d = f
            if length(sub(verts[a], verts[c])) <= length(sub(verts[b], verts[d])):
                tris += [(a, b, c), (a, c, d)]; tuv += [(u[0], u[1], u[2]), (u[0], u[2], u[3])]
            else:
                tris += [(a, b, d), (b, c, d)]; tuv += [(u[0], u[1], u[3]), (u[1], u[2], u[3])]
    lim = math.cos(math.radians(sharp))
    edges = {}
    for t, u in zip(tris, tuv):
        a, b, c = verts[t[0]], verts[t[1]], verts[t[2]]
        n = cross(sub(b, a), sub(c, a))
        if dot(n, n) < 1e-18:
            continue
        n = unit(n)
        g.T.append((t[0] + base, t[1] + base, t[2] + base)); g.UV.append(u); g.M.append(mat)
        for i in range(3):
            e = (t[i], t[(i + 1) % 3]) if t[i] < t[(i + 1) % 3] else (t[(i + 1) % 3], t[i])
            edges.setdefault(e, []).append(n)
    for e, ns in edges.items():
        if len(ns) == 2 and dot(ns[0], ns[1]) < lim:
            g.sharp.add((e[0] + base, e[1] + base))


def earclip(pts):
    """Triangles (index triples) of a simple polygon given counter-clockwise."""
    idx, out, guard = list(range(len(pts))), [], 0
    def area2(a, b, c): return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
    while len(idx) > 3 and guard < 10000:
        guard += 1
        n = len(idx)
        for k in range(n):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % n]
            a, b, c = pts[i0], pts[i1], pts[i2]
            if area2(a, b, c) <= 1e-14:
                continue
            if any(area2(a, b, pts[j]) > 1e-12 and area2(b, c, pts[j]) > 1e-12 and area2(c, a, pts[j]) > 1e-12 for j in idx if j not in (i0, i1, i2)):
                continue
            out.append((i0, i1, i2)); del idx[k]
            break
        else:
            break
    if len(idx) == 3:
        out.append((idx[0], idx[1], idx[2]))
    return out


# ---------------------------------------------------------------- sections (counter-clockwise, in a ring's own x, y)
def sec_rect(w, h, c=0.0):
    a, b = w / 2, h / 2
    if c <= 0 or LOD >= 2:
        return [(-a, -b), (a, -b), (a, b), (-a, b)]
    return [(-a + c, -b), (a - c, -b), (a, -b + c), (a, b - c), (a - c, b), (-a + c, b), (-a, b - c), (-a, -b + c)]


def sec_circle(r, n, a0=0.0):
    return [(r * math.cos(a0 + TAU * k / n), r * math.sin(a0 + TAU * k / n)) for k in range(n)]


def sec_L(a, b, t, c=0.0):
    """Angle iron: heel at the origin, one flange along +x (a long), one along +y (b long), t thick."""
    if LOD >= 3:
        return [(0, 0), (a, 0), (a * .45, b * .45), (0, b)]
    if c <= 0 or LOD >= 1:
        return [(0, 0), (a, 0), (a, t), (t, t), (t, b), (0, b)]
    return [(c, 0), (a, 0), (a, t), (t + t * .7, t), (t, t + t * .7), (t, b), (0, b), (0, c)]


def turn2(sec, ang, dx=0.0, dy=0.0):
    c, s = math.cos(ang), math.sin(ang)
    return [(x * c - y * s + dx, x * s + y * c + dy) for x, y in sec]


def mirror2(sec):
    """Mirrored in x (the order turned so it stays counter-clockwise)."""
    return [(-x, y) for x, y in reversed(sec)]


# ---------------------------------------------------------------- primitives
def ring(o, x, y, s=1.0):
    return (o, x, y, s if isinstance(s, tuple) else (s, s))


def rings_line(a, b, ref=(0, 0, 1), cham=0.0, rsec=0.03):
    """The rings of a straight bar from a to b; cham: its ends drawn in by that much (a chamfer on both end edges)."""
    x, y, z = basis(sub(b, a), ref)
    if cham > 0 and LOD == 0:
        k = max(0.2, 1 - cham / rsec)
        return [ring(a, x, y, k), ring(add(a, mul(z, cham)), x, y), ring(sub(b, mul(z, cham)), x, y), ring(b, x, y, k)]
    return [ring(a, x, y), ring(b, x, y)]


def rings_path(pts, ref=(0, 0, 1), fillet=0.0, arc=4):
    """Rings along a polyline, its corners rounded by fillet (arc steps each), the frame carried without twist."""
    P = [pts[0]]
    for i in range(1, len(pts) - 1):
        a, b, c = pts[i - 1], pts[i], pts[i + 1]
        d0, d1 = unit(sub(a, b)), unit(sub(c, b))
        f = min(fillet, length(sub(a, b)) * .45, length(sub(c, b)) * .45)
        if f <= 0 or arc < 1 or dot(d0, d1) < -0.999:
            P.append(b); continue
        p0, p1 = add(b, mul(d0, f)), add(b, mul(d1, f))
        for k in range(arc + 1):
            t = k / arc
            q0, q1 = lerp(p0, b, t), lerp(b, p1, t)
            P.append(lerp(q0, q1, t))
    P.append(pts[-1])
    out, n = [], len(P)
    x, y, z = basis(sub(P[1], P[0]), ref)
    for i in range(n):
        d = unit(sub(P[min(i + 1, n - 1)], P[max(i - 1, 0)]))
        ax = cross(z, d)
        if length(ax) > 1e-9:
            ang = math.atan2(length(ax), dot(z, d)); ax = unit(ax)
            x, y = rot_about(x, ax, ang), rot_about(y, ax, ang)
        z = d
        out.append(ring(P[i], x, y))
    return out


def sweep(key, sec, rings, mat, closed=True, cap0=True, cap1=True, sharp=35.0, dens=1.0, vmax=1.6, tone=0.0, attr=None):
    """A section carried along rings. Charts: key (the wall, in pieces no longer than vmax metres), key/c0 and key/c1 (the caps)."""
    n = len(sec)
    cum = [0.0]
    for k in range(n if closed else n - 1):
        a, b = sec[k], sec[(k + 1) % n]
        cum.append(cum[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    per = max(cum[-1], 1e-6)
    rsec = sum(math.hypot(p[0], p[1]) for p in sec) / n
    R = [rings[0]]
    for r in rings[1:]:                                   # no step longer than vmax
        d = length(sub(r[0], R[-1][0]))
        m = int(math.ceil(d * dens / vmax - 1e-9)) if d * dens > vmax else 1
        p = R[-1]
        for j in range(1, m):
            t = j / m
            R.append((lerp(p[0], r[0], t), lerp(p[1], r[1], t), lerp(p[2], r[2], t), (p[3][0] + (r[3][0] - p[3][0]) * t, p[3][1] + (r[3][1] - p[3][1]) * t)))
        R.append(r)
    vl = [0.0]
    for a, b in zip(R, R[1:]):
        vl.append(vl[-1] + max(length(sub(b[0], a[0])), abs(b[3][0] - a[3][0]) * rsec, 1e-5))
    cuts = [0]                                            # ring indices where a new chart begins
    for i in range(1, len(R) - 1):
        if (vl[i + 1] - vl[cuts[-1]]) * dens > vmax + 1e-9:
            cuts.append(i)
    cuts.append(len(R) - 1)
    smax = max(max(r[3]) for r in R)
    verts = []
    for o, x, y, s in R:
        for px, py in sec:
            verts.append((o[0] + x[0] * px * s[0] + y[0] * py * s[1], o[1] + x[1] * px * s[0] + y[1] * py * s[1], o[2] + x[2] * px * s[0] + y[2] * py * s[1]))
    faces, uvs = [], []
    for ci in range(len(cuts) - 1):
        a, b = cuts[ci], cuts[ci + 1]
        ch = ATL.chart(key if ci == 0 else '%s#%d' % (key, ci), per * smax, vl[b] - vl[a], dens)
        for i in range(a, b):
            t0, t1 = (vl[i] - vl[a]) / (vl[b] - vl[a]), (vl[i + 1] - vl[a]) / (vl[b] - vl[a])
            for k in range(n if closed else n - 1):
                k1 = (k + 1) % n
                s0, s1 = cum[k] / per, cum[k + 1] / per
                faces.append((i * n + k, i * n + k1, (i + 1) * n + k1, (i + 1) * n + k))
                uvs.append(((ch, s0, t0), (ch, s1, t0), (ch, s1, t1), (ch, s0, t1)))
    if closed and (cap0 or cap1) and n >= 3:
        tri = earclip(sec)
        x0, x1 = min(p[0] for p in sec), max(p[0] for p in sec)
        y0, y1 = min(p[1] for p in sec), max(p[1] for p in sec)
        w, h = max(x1 - x0, 1e-5), max(y1 - y0, 1e-5)
        for end, on, name in ((0, cap0, '/c0'), (len(R) - 1, cap1, '/c1')):
            if not on:
                continue
            s = R[end][3]
            ch = ATL.chart(key + name, w * s[0], h * s[1], dens)
            for t in tri:
                f = tuple(end * n + j for j in (t if end else (t[0], t[2], t[1])))
                faces.append(f)
                uvs.append(tuple((ch, (sec[j - end * n][0] - x0) / w, (sec[j - end * n][1] - y0) / h) for j in f))
    emit(verts, faces, uvs, mat, sharp, tone, attr)


def bar(key, a, b, sec, mat, ref=(0, 0, 1), cham=0.0, **kw):
    rs = sum(math.hypot(p[0], p[1]) for p in sec) / len(sec)
    sweep(key, sec, rings_line(a, b, ref, cham, rs), mat, **kw)


def tube(key, pts, r, n, mat, ref=(0, 0, 1), fillet=0.0, arc=4, **kw):
    sweep(key, sec_circle(r, n), rings_path(pts, ref, fillet, arc), mat, **kw)


def revolve(key, prof, M, segs, mat, charts=1, sharp=35.0, dens=1.0, tone=0.0, attr=None, a0=0.0, a1=TAU):
    """prof: (r, z) points, outside to the right as it is walked (up an outer wall, inward over a top, down an inner wall).
    M puts (x, y, z) in place. Charts: key, key#1 ... (the turn in equal pieces)."""
    full = abs(a1 - a0 - TAU) < 1e-9
    m = len(prof)
    cum = [0.0]
    for a, b in zip(prof, prof[1:]):
        cum.append(cum[-1] + max(math.hypot(b[0] - a[0], b[1] - a[1]), 1e-6))
    rr = max(p[0] for p in prof)
    cols = segs if full else segs + 1
    verts = []
    for i in range(cols):
        th = a0 + (a1 - a0) * i / segs
        c, s = math.cos(th), math.sin(th)
        for r, z in prof:
            verts.append(M((r * c, r * s, z)))
    faces, uvs = [], []
    per = max(1, segs // charts)
    for i in range(segs):
        ci = min(i // per, charts - 1)
        lo, hi = ci * per, (segs if ci == charts - 1 else (ci + 1) * per)
        ch = ATL.chart(key if ci == 0 else '%s#%d' % (key, ci), rr * abs(a1 - a0) * (hi - lo) / segs, cum[-1], dens)
        s0, s1 = (i - lo) / (hi - lo), (i + 1 - lo) / (hi - lo)
        i1 = (i + 1) % cols
        for k in range(m - 1):
            t0, t1 = cum[k] / cum[-1], cum[k + 1] / cum[-1]
            q = (i * m + k, i1 * m + k, i1 * m + k + 1, i * m + k + 1)
            u = ((ch, s0, t0), (ch, s1, t0), (ch, s1, t1), (ch, s0, t1))
            if prof[k][0] < 1e-9:
                faces.append((q[0], q[2], q[3])); uvs.append((u[0], u[2], u[3]))
            elif prof[k + 1][0] < 1e-9:
                faces.append((q[0], q[1], q[2])); uvs.append((u[0], u[1], u[2]))
            else:
                faces.append(q); uvs.append(u)
    emit(verts, faces, uvs, mat, sharp, tone, attr)


def sheet(key, P, nu, nw, thick, mat, size, sharp=50.0, dens=1.0, tone=0.0, attr=None, back=True):
    """A thin plate through P(u, w), u and w from 0 to 1; its front is where du x dw points. size: its (u, w) lengths in metres.
    Charts: key/f and key/b (the back; back=False puts it on the front's texels)."""
    F = [[P(i / nu, j / nw) for j in range(nw + 1)] for i in range(nu + 1)]
    def nrm(i, j):
        a, b = F[min(i + 1, nu)][j], F[max(i - 1, 0)][j]
        c, d = F[i][min(j + 1, nw)], F[i][max(j - 1, 0)]
        return unit(cross(sub(a, b), sub(c, d)))
    W = nw + 1
    verts = [F[i][j] for i in range(nu + 1) for j in range(W)]
    verts += [sub(F[i][j], mul(nrm(i, j), thick)) for i in range(nu + 1) for j in range(W)] if thick > 0 else list(verts)
    off = (nu + 1) * W
    cf = ATL.chart(key + '/f', size[0], size[1], dens)
    cb = ATL.chart(key + '/b', size[0], size[1], dens) if back else cf
    faces, uvs = [], []
    for i in range(nu):
        for j in range(nw):
            a, b, c, d = i * W + j, (i + 1) * W + j, (i + 1) * W + j + 1, i * W + j + 1
            u = ((i / nu, j / nw), ((i + 1) / nu, j / nw), ((i + 1) / nu, (j + 1) / nw), (i / nu, (j + 1) / nw))
            faces.append((a, b, c, d)); uvs.append(tuple((cf, s, t) for s, t in u))
            faces.append((off + a, off + d, off + c, off + b)); uvs.append(tuple((cb, u[k][0], u[k][1]) for k in (0, 3, 2, 1)))
    if thick > 0:
        loop = [(i, 0) for i in range(nu)] + [(nu, j) for j in range(nw)] + [(i, nw) for i in range(nu, 0, -1)] + [(0, j) for j in range(nw, 0, -1)]
        for k, (i, j) in enumerate(loop):
            i2, j2 = loop[(k + 1) % len(loop)]
            a, b = i * W + j, i2 * W + j2
            faces.append((a, off + a, off + b, b))
            ua, ub = (cf, i / nu, j / nw), (cf, i2 / nu, j2 / nw)
            uvs.append((ua, ua, ub, ub))
    emit(verts, faces, uvs, mat, sharp, tone, attr)


def bolt(key, p, n, r, h, mat, segs=6, **kw):
    """A hex head (or nut) standing on p along n."""
    x, y, z = basis(n, (0, 0, 1) if abs(n[2]) < .9 else (1, 0, 0))
    revolve(key, [(r, 0.0), (r, h * .75), (r * .7, h), (0.0, h)], place(p, x, y, z), segs, mat, sharp=30.0, **kw)


def measure(parts, lods=(0, 1, 2, 3)):
    rows = {}
    for p in parts:
        rows[p.name] = {'triangles': [p.tris(l) for l in lods if l in p.lods], 'min_m': [round(v, 4) for v in p.bounds()[0]], 'max_m': [round(v, 4) for v in p.bounds()[1]],
                        'pivot_m': list(p.pivot), 'atlas': p.atlas.name if p.atlas else None}
    return rows
