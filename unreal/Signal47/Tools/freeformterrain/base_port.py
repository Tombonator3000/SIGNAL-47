"""NumPy port of S47RoadShape.cpp / S47RoadTerrain.cpp: the road's desert grid (GridY), road-local metres.

Exact port of what the exported g873 grid is built from. Checked against the mesh itself and the
height probes by bake_relief.py (see port_check). No game file is read or changed by this module.
"""
import math
import numpy as np

PI = math.pi
MILE = 1609.34
CROSS_MI = 8.15
HWY_FLAT, HWY_MARGIN = 16.5, 18.0
HWY_PAVED, HWY_SHOULDER = 4.8, 6.0
TRK_HALF, TRK_SOFT, TRK_FLAT, TRK_MARGIN = 2.3, 0.7, 11.5, 13.0
OLD_HALF, OLD_SHOULDER, OLD_FLAT, OLD_MARGIN, OLD_LEN = 3.2, 4.4, 9.0, 11.0, 640.0
START = (-1.8, 0.0)
END = (-603.0, 532.0)
KEYS = [(-5, 520), (-24, 520), (-70, 518), (-150, 507), (-240, 496), (-330, 497), (-420, 508), (-500, 522), (-560, 530), (-606, 532)]


def smooth(a, b, x):
    t = np.clip((np.asarray(x, dtype=np.float64) - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def base(x, z):
    return 1.1 * np.sin(x * 0.0061 + 0.7) * np.cos(z * 0.0047 + 0.3) + 0.7 * np.sin((x + z) * 0.0032 + 1.9) + 0.4 * np.sin(z * 0.011 - x * 0.004 + 0.4)


B0 = float(base(START[0], START[1]))


def wash_at(x, z):
    w = x + 330 + 0.25 * (z - 500)
    return np.exp(-w * w / 520) * smooth(-150, 60, z)


def raw(x, z):
    dx = x + 250.0
    dz = z - 300.0
    a = np.arctan2(dz, dx)
    hills = smooth(1300, 2400, np.sqrt(dx * dx + dz * dz)) * (12 + 9 * np.sin(a * 5 + 1.3) + 5 * np.sin(a * 13))
    return base(x, z) - B0 - 1.3 * wash_at(x, z) + hills


DINER_Z, DINER_FLATX = 1999.95, -11.05
MOTEL_FLATX, MOTEL_Z = -14.75, 386.0
DINER_Y = float(raw(DINER_FLATX, DINER_Z))
MOTEL_Y = float(raw(MOTEL_FLATX, MOTEL_Z))


def hlow(x, z):
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    r = raw(x, z)
    d = np.hypot(x - DINER_FLATX, z - DINER_Z)
    m = np.hypot(x - MOTEL_FLATX, z - MOTEL_Z)
    a = r + (MOTEL_Y - r) * (1 - smooth(45, 110, m))
    b = np.where(d > 110, r, r + (DINER_Y - r) * (1 - smooth(45, 110, d)))
    return np.where(m < 110, a, b)


def _smooth1(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def old_bearing(mi):
    def b(a, c, m0, m1):
        return a + (c - a) * _smooth1(m0, m1, mi)
    if mi < 5.6:
        lead = 63 + 27 * (1 - _smooth1(0.02, 0.3, mi))
        m = mi - 0.3
        return lead + (8 * math.sin(m * 2.3) + 4 * math.sin(m * 5.1 + 1)) * _smooth1(0.3, 1.0, mi) * (1 - _smooth1(4.6, 5.5, mi))
    if mi < 6.75:
        return b(63, 45, 6.25, 6.75)
    if mi < 7.75:
        return b(45, 35, 7.25, 7.75)
    return b(35, 27, 8.5, 8.9)


def old_stub():
    s0 = -math.floor(CROSS_MI * MILE / 4) * 4
    pts = []
    x, z, s = 0.0, DINER_Z, 0.0
    while s <= OLD_LEN:
        pts.append((x, z))
        bb = old_bearing(CROSS_MI + (s0 + s + 2) / MILE) * PI / 180
        x += math.sin(bb) * 4
        z -= math.cos(bb) * 4
        s += 4
    return np.array(pts)


OLD = old_stub()


class _Curve:
    """three.js CatmullRomCurve3, centripetal, open; arc length table of 200 divisions."""

    def __init__(self, pts):
        self.p = [np.array(p, dtype=np.float64) for p in pts]
        self.lengths = [0.0]
        last = self.point(0.0)
        total = 0.0
        for i in range(1, 201):
            cur = self.point(i / 200)
            total += float(np.linalg.norm(cur - last))
            self.lengths.append(total)
            last = cur

    def point(self, t):
        p = self.p
        n = len(p)
        v = (n - 1) * t
        i = int(math.floor(v))
        w = v - i
        if w == 0 and i == n - 1:
            i, w = n - 2, 1.0
        p0 = p[i - 1] if i > 0 else p[0] - p[1] + p[0]
        p1, p2 = p[i], p[i + 1]
        p3 = p[i + 2] if i + 2 < n else p[n - 1] - p[n - 2] + p[n - 1]
        d0 = float(np.sum((p0 - p1) ** 2)) ** .25
        d1 = float(np.sum((p1 - p2) ** 2)) ** .25
        d2 = float(np.sum((p2 - p3) ** 2)) ** .25
        if d1 < 1e-4:
            d1 = 1.0
        if d0 < 1e-4:
            d0 = d1
        if d2 < 1e-4:
            d2 = d1
        t1 = ((p1 - p0) / d0 - (p2 - p0) / (d0 + d1) + (p2 - p1) / d1) * d1
        t2 = ((p2 - p1) / d1 - (p3 - p1) / (d1 + d2) + (p3 - p2) / d2) * d1
        c0, c1 = p1, t1
        c2 = -3 * p1 + 3 * p2 - 2 * t1 - t2
        c3 = 2 * p1 - 2 * p2 + t1 + t2
        return c0 + c1 * w + c2 * w * w + c3 * w * w * w

    def u_to_t(self, u):
        ls = self.lengths
        il = len(ls)
        target = u * ls[il - 1]
        low, high, i = 0, il - 1, 0
        while low <= high:
            i = int(math.floor(low + (high - low) / 2.0))
            c = ls[i] - target
            if c < 0:
                low = i + 1
            elif c > 0:
                high = i - 1
            else:
                high = i
                break
        i = high
        if ls[i] == target:
            return i / (il - 1)
        return (i + (target - ls[i]) / (ls[i + 1] - ls[i])) / (il - 1)

    def spaced(self, div):
        return np.array([self.point(self.u_to_t(d / div)) for d in range(div + 1)])


def _track():
    c = _Curve(KEYS)
    return c.spaced(int(math.ceil(c.lengths[-1] / 3)))


TRACK = _track()


def seg_dist(px, pz, poly):
    """Distance from points to a polyline (NumPy arrays of any shape; loops over segments)."""
    best = np.full(np.shape(px), np.inf)
    for i in range(len(poly) - 1):
        a, b = poly[i], poly[i + 1]
        dx, dz = b[0] - a[0], b[1] - a[1]
        t = np.clip(((px - a[0]) * dx + (pz - a[1]) * dz) / (dx * dx + dz * dz), 0, 1)
        d = (a[0] + dx * t - px) ** 2 + (a[1] + dz * t - pz) ** 2
        best = np.minimum(best, d)
    return np.sqrt(best)


def track_nearest(x, z):
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    out = np.full(x.shape, np.inf)
    m = ~((x > 40) | (x < -650) | (z < 450) | (z > 580))
    if m.any():
        out[m] = seg_dist(x[m], z[m], TRACK)
    return out


def old_nearest(x, z):
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    out = np.full(x.shape, np.inf)
    m = ~((x < -2) | (x > OLD_LEN + 30) | (np.abs(z - DINER_Z) > 120))
    if not m.any():
        return out
    px, pz = x[m], z[m]
    best = np.full(px.shape, np.inf)
    for i in range(len(OLD) - 1):
        a, b = OLD[i], OLD[i + 1]
        dx, dz = b[0] - a[0], b[1] - a[1]
        tr = ((px - a[0]) * dx + (pz - a[1]) * dz) / (dx * dx + dz * dz)
        t = np.clip(tr, 0, 1)
        d = (a[0] + dx * t - px) ** 2 + (a[1] + dz * t - pz) ** 2
        if i == 0:
            d = np.where(tr < 0, np.inf, d)
        best = np.minimum(best, d)
    out[m] = np.sqrt(best)
    return out


def detail(x, z):
    return 0.16 * np.sin(x * 0.13 + 1.7) * np.sin(z * 0.11 + 0.3) + 0.08 * np.sin(x * 0.31 - z * 0.23 + 0.9)


def grid_h(x, z):
    hd = np.abs(x)
    td = track_nearest(x, z)
    od = old_nearest(x, z)
    meet = smooth(-200, -140, z) * smooth(40, 90, np.hypot(x - END[0], z - END[1])) * smooth(40, 90, np.hypot(x - DINER_FLATX, z - DINER_Z)) * smooth(40, 90, np.hypot(x - MOTEL_FLATX, z - MOTEL_Z))
    h = hlow(x, z) + detail(x, z) * smooth(0, 15, np.minimum(np.minimum(hd - HWY_MARGIN, td - TRK_MARGIN), od - OLD_MARGIN)) * meet
    return np.where((hd < HWY_FLAT) | (td < TRK_FLAT), h - 0.45, h)


def axis(c0, c1, step, lo, hi):
    mid = []
    v = c0
    while v <= c1 + 1e-6:
        mid.append(v)
        v += step
    left, right = [], []
    s, v = step, c0
    while v > lo:
        s = min(120.0, s * 1.25)
        v = max(lo, v - s)
        left.append(v)
    s, v = step, c1
    while v < hi:
        s = min(120.0, s * 1.25)
        v = min(hi, v + s)
        right.append(v)
    return np.array(left[::-1] + mid + right, dtype=np.float64)


GX = axis(-800, 60, 10, -3000, 2300)
GZ = axis(-480, 2760, 10, -2700, 4400)


def grid_heights():
    xx, zz = np.meshgrid(GX, GZ)
    return grid_h(xx, zz).astype(np.float32)


_H = None


def grid_y(x, z):
    """S47RoadTerrain::GridY: float32 grid heights, split so that u+v<=1 is the first triangle."""
    global _H
    if _H is None:
        _H = grid_heights().astype(np.float64)
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    i = np.clip(np.searchsorted(GX, x, side='right') - 1, 0, len(GX) - 2)
    j = np.clip(np.searchsorted(GZ, z, side='right') - 1, 0, len(GZ) - 2)
    u = np.clip((x - GX[i]) / (GX[i + 1] - GX[i]), 0, 1)
    v = np.clip((z - GZ[j]) / (GZ[j + 1] - GZ[j]), 0, 1)
    a, b, c, d = _H[j, i], _H[j, i + 1], _H[j + 1, i], _H[j + 1, i + 1]
    return np.where(u + v <= 1, a + (b - a) * u + (c - a) * v, d + (c - d) * (1 - u) + (b - d) * (1 - v))


def fake_nodes(x, z):
    """The parts of the grid's height that read as drawn rather than as land: the straight wash, the
    regular sine bumps (their 16 m diagonal gives parallel stripes) and the lobes of the far hills.
    Fading exactly as HLow and GridH fade them."""
    d = np.hypot(x - DINER_FLATX, z - DINER_Z)
    m = np.hypot(x - MOTEL_FLATX, z - MOTEL_Z)
    keep = np.where(m < 110, smooth(45, 110, m), np.where(d > 110, 1.0, smooth(45, 110, d)))
    dx = x + 250.0
    dz = z - 300.0
    a = np.arctan2(dz, dx)
    lobes = smooth(1300, 2400, np.sqrt(dx * dx + dz * dz)) * (9 * np.sin(a * 5 + 1.3) + 5 * np.sin(a * 13))
    wash = -1.3 * wash_at(x, z)
    hd = np.abs(x)
    td = track_nearest(x, z)
    od = old_nearest(x, z)
    meet = smooth(-200, -140, z) * smooth(40, 90, np.hypot(x - END[0], z - END[1])) * smooth(40, 90, np.hypot(x - DINER_FLATX, z - DINER_Z)) * smooth(40, 90, np.hypot(x - MOTEL_FLATX, z - MOTEL_Z))
    bumps = detail(x, z) * smooth(0, 15, np.minimum(np.minimum(hd - HWY_MARGIN, td - TRK_MARGIN), od - OLD_MARGIN)) * meet
    return wash * keep + bumps, lobes * keep


_F = None


def fake_y(x, z):
    """fake_nodes as the grid draws it: on the grid's nodes, with GridY's triangle split.
    Returns (wash and bumps, far lobes): the road itself rides on the lobes where it meets them."""
    global _F
    if _F is None:
        xx, zz = np.meshgrid(GX, GZ)
        _F = fake_nodes(xx, zz)
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    i = np.clip(np.searchsorted(GX, x, side='right') - 1, 0, len(GX) - 2)
    j = np.clip(np.searchsorted(GZ, z, side='right') - 1, 0, len(GZ) - 2)
    u = np.clip((x - GX[i]) / (GX[i + 1] - GX[i]), 0, 1)
    v = np.clip((z - GZ[j]) / (GZ[j + 1] - GZ[j]), 0, 1)
    out = []
    for F in _F:
        a, b, c, d = F[j, i], F[j, i + 1], F[j + 1, i], F[j + 1, i + 1]
        out.append(np.where(u + v <= 1, a + (b - a) * u + (c - a) * v, d + (c - d) * (1 - u) + (b - d) * (1 - v)))
    return out[0], out[1]


ARRAY_FLAT = (-290.0, 270.0, -515.0, 0.0)    # SARO's terms: dishes -230..210, -420..-42, plus 60 and 95 (S47Relief.cpp, S47GroundCover.cpp)
SARO_PATCH = (-120.0, 120.0, -200.0, 600.0)
SIR = (24.0, -300.0)


def _outside(rect, x, z):
    ax = np.maximum(np.maximum(rect[0] - x, x - rect[1]), 0)
    az = np.maximum(np.maximum(rect[2] - z, z - rect[3]), 0)
    return np.hypot(ax, az)


def saro_land(x, z):
    """S47GroundCover SaroLand without the relief: the floor in SARO's terms. The exported g4 only approximates it."""
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    far = np.hypot(x, z)
    y = np.where(far > 300, -2 + np.sin(x * 0.004 + z * 0.003) * 6 * np.minimum(1.0, (far - 300) / 600), -0.62)
    e = _outside(ARRAY_FLAT, x, z)
    y = np.where(e < 40, -0.62 + (y + 0.62) * smooth(0, 40, e), y)
    rx, rz = x + SIR[0], z + SIR[1]
    w = smooth(40, 100, z) * (1 - smooth(0, 160, _outside(SARO_PATCH, rx, rz)))
    inside = (rx >= SARO_PATCH[0] - 2) & (rx <= SARO_PATCH[1] + 2) & (rz >= SARO_PATCH[2] - 2) & (rz <= SARO_PATCH[3] + 2)   # InRect(..., -2) widens
    return np.where(w > 0, y + (hlow(rx, rz) - np.where(inside, 0.7, 0.05) - y) * w, y)
