#!/usr/bin/env python3
"""Bake Freeform's relief: relief.bin/json, masks and shaded maps from real USGS 3DEP heights.

Run with any Python that has NumPy and Pillow (not Blender: unreal_guard counts Blender as a heavy
job). It changes nothing in the game; everything goes to --out.

    python3 bake_relief.py --terrain <terreng-2026-11-07> --out <freeformterrain> --project <unreal/Signal47>

The height offset Off(x, z), in road-local metres, is the real map cut up by wavelength with the
gain rising with the distance from what must stay put, plus arroyos cut along the real drainage and
noise stretched along the flow. GridY + Off is the ground; Off is exactly 0 in the masks.
"""
import argparse
import heapq
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import base_port as B
import masks_game as MG

CELL = 4.0
X0, X1 = -3040.0, 3040.0
Z0, Z1 = -2740.0, 4444.0
SCALE_M = 0.002
SEED = 4747
BIG, SMALL = 5.5, 5.0      # log10 upstream area (m2) where an arroyo and a gully begin
# map centre (utsnitt A) and where it lies in road-local metres; found by placement() and fixed here
PLACE_DEFAULT = None


def log(*a):
    print(*a, flush=True)


def smooth(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def gauss(a, sig, pad=0):
    """Gaussian by FFT; sig in cells; reflects `pad` cells so that the far side does not wrap in."""
    if pad:
        a = np.pad(a, pad, mode='reflect')
    h, w = a.shape
    ky = np.fft.fftfreq(h)[:, None]
    kx = np.fft.rfftfreq(w)[None, :]
    g = np.exp(-2 * (np.pi * sig) ** 2 * (kx ** 2 + ky ** 2))
    out = np.fft.irfft2(np.fft.rfft2(a) * g, s=a.shape).astype(np.float32)
    return out[pad:-pad, pad:-pad] if pad else out


def bilinear(a, c, r):
    h, w = a.shape
    c = np.clip(c, 0, w - 1.001)
    r = np.clip(r, 0, h - 1.001)
    c0 = c.astype(np.int64)
    r0 = r.astype(np.int64)
    fc = (c - c0).astype(np.float32)
    fr = (r - r0).astype(np.float32)
    return a[r0, c0] * (1 - fr) * (1 - fc) + a[r0, c0 + 1] * (1 - fr) * fc + a[r0 + 1, c0] * fr * (1 - fc) + a[r0 + 1, c0 + 1] * fr * fc


def load_dem(path_json):
    meta = json.loads(Path(path_json).read_text())
    u = np.fromfile(str(path_json)[:-5] + '.raw', dtype='<u2').reshape(meta['height'], meta['width']).astype(np.float32)
    return meta['zmin_m'] + u / 65535 * (meta['zmax_m'] - meta['zmin_m']), meta


def priority_flood(z, eps=1e-4):
    """Epsilon filling (Barnes 2014); the edges drain out. From the height data's own tools."""
    h, w = z.shape
    zf = z.astype(np.float64).ravel().tolist()
    closed = bytearray(h * w)
    heap = []
    for x in range(w):
        for y in (0, h - 1):
            i = y * w + x
            if not closed[i]:
                closed[i] = 1
                heap.append((zf[i], i))
    for y in range(h):
        for x in (0, w - 1):
            i = y * w + x
            if not closed[i]:
                closed[i] = 1
                heap.append((zf[i], i))
    heapq.heapify(heap)
    pop, push = heapq.heappop, heapq.heappush
    offs = ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1))
    while heap:
        c, i = pop(heap)
        y, x = divmod(i, w)
        for dy, dx in offs:
            yy = y + dy
            xx = x + dx
            if 0 <= yy < h and 0 <= xx < w:
                j = yy * w + xx
                if not closed[j]:
                    closed[j] = 1
                    v = zf[j]
                    if v <= c:
                        v = c + eps
                        zf[j] = v
                    push(heap, (v, j))
    return np.array(zf, dtype=np.float64).reshape(h, w)


def flow_acc(z, cell):
    """D8 upstream area in m2 on the filled surface; also the downstream direction per cell."""
    zf = priority_flood(z)
    h, w = zf.shape
    p = np.pad(zf, 1, mode='constant', constant_values=-1e9)
    best = np.zeros((h, w))
    rec = np.arange(h * w).reshape(h, w)
    yy, xx = np.mgrid[0:h, 0:w]
    for dy, dx in ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)):
        nb = p[1 + dy:1 + dy + h, 1 + dx:1 + dx + w]
        drop = (zf - nb) / np.hypot(dy, dx)
        m = drop > best
        best = np.where(m, drop, best)
        rec = np.where(m, np.clip(yy + dy, 0, h - 1) * w + np.clip(xx + dx, 0, w - 1), rec)
    r = rec.ravel().tolist()
    order = np.argsort(-zf.ravel(), kind='stable').tolist()
    acc = [1.0] * (h * w)
    for i in order:
        j = r[i]
        if j != i:
            acc[j] += acc[i]
    return (np.array(acc).reshape(h, w) * cell * cell).astype(np.float32), rec.astype(np.int32)


def jfa(seed, passes=None, posy=None, posx=None):
    """Jump flood: distance (cells) to the nearest seed and that seed's (row, col). posy/posx give
    a seed's exact position inside its cell, so a smoothed line gives a smooth distance."""
    h, w = seed.shape
    yy, xx = np.mgrid[0:h, 0:w].astype(np.int32)
    if posy is None:
        posy, posx = yy.astype(np.float32), xx.astype(np.float32)
    ny = np.where(seed, yy, -1).astype(np.int32)
    nx = np.where(seed, xx, -1).astype(np.int32)
    far = np.float32(1e12)
    d = np.where(seed, (posy - yy) ** 2 + (posx - xx) ** 2, far).astype(np.float32)
    step = 1 << int(np.ceil(np.log2(max(h, w)))) if passes is None else passes
    while step >= 1:
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)):
            sy, sx = dy * step, dx * step
            if abs(sy) >= h or abs(sx) >= w:
                continue
            py = np.full((h, w), -1, np.int32)
            px = np.full((h, w), -1, np.int32)
            ys, yd = (slice(0, h - sy), slice(sy, h)) if sy >= 0 else (slice(-sy, h), slice(0, h + sy))
            xs, xd = (slice(0, w - sx), slice(sx, w)) if sx >= 0 else (slice(-sx, w), slice(0, w + sx))
            py[yd, xd] = ny[ys, xs]
            px[yd, xd] = nx[ys, xs]
            valid = py >= 0
            dn = np.full((h, w), far, np.float32)
            dn[valid] = (posy[py[valid], px[valid]] - yy[valid]) ** 2 + (posx[py[valid], px[valid]] - xx[valid]) ** 2
            m = dn < d
            d[m] = dn[m]
            ny[m] = py[m]
            nx[m] = px[m]
        step //= 2
    return np.sqrt(d), ny.clip(0, h - 1), nx.clip(0, w - 1)


def trace_channels(la, receivers, thresh):
    """The drainage as polylines: heads to junctions and on, from the D8 receivers."""
    h, w = la.shape
    r = receivers.ravel()
    mask = (la >= thresh).ravel()
    cells = np.nonzero(mask)[0]
    indeg = np.zeros(h * w, np.int32)
    to = r[cells]
    ok = (to != cells) & mask[to]
    np.add.at(indeg, to[ok], 1)
    heads = cells[indeg[cells] == 0].tolist()
    started = set(heads)
    paths = []
    stack = list(heads)
    while stack:
        c = stack.pop()
        path = [c]
        while True:
            n = int(r[c])
            if n == c or not mask[n]:
                break
            path.append(n)
            if indeg[n] >= 2:
                if n not in started:
                    started.add(n)
                    stack.append(n)
                break
            c = n
        if len(path) >= 2:
            paths.append(np.array(path))
    return paths


def smooth_line(v, sigma):
    """Gaussian along a line with both ends pinned (odd reflection about them)."""
    n = len(v)
    if n < 3 or sigma <= 0:
        return v.copy()
    line = v[0] + (v[-1] - v[0]) * np.linspace(0, 1, n)
    q = v - line
    r = int(max(3 * sigma, 3))
    k = np.exp(-.5 * (np.arange(-r, r + 1) / sigma) ** 2)
    k /= k.sum()
    qq = np.concatenate([-q[1:r + 1][::-1] if n > r else -np.resize(q[1:][::-1], r), q, -q[-r - 1:-1][::-1] if n > r else -np.resize(q[:-1][::-1], r)])
    return line + np.convolve(qq, k, mode='same')[r:-r]


def channel_seeds(paths, la, rng, h, w):
    """Smoothed, gently meandering centre lines rasterised to seeds with their exact positions,
    width-deciding upstream area, tangent and signed curvature. Coordinates in cells."""
    out = {k: np.zeros((h, w), np.float32) for k in ('py', 'px', 'la', 'tx', 'tz', 'k')}
    seed = np.zeros((h, w), bool)
    ww = w
    for path in paths:
        rows, cols = np.divmod(path, ww)
        x = cols.astype(np.float64)
        y = rows.astype(np.float64)
        val = np.maximum.accumulate(la[rows, cols].astype(np.float64))
        # take the 8-direction stairs out: about 45 m of smoothing along the line
        x = smooth_line(smooth_line(x, 6), 4)
        y = smooth_line(smooth_line(y, 6), 4)
        seg = np.hypot(np.diff(x), np.diff(y))
        s = np.concatenate([[0], np.cumsum(seg)])
        length = s[-1]
        if length < 3:
            continue
        # resample at half a cell
        m = max(int(length / .5), 2)
        si = np.linspace(0, length, m)
        x = np.interp(si, s, x)
        y = np.interp(si, s, y)
        val = np.interp(si, s, val)
        wd = channel_width(val) / CELL
        # meanders, a real river's length of about ten widths; none at the pinned ends
        lam = np.maximum(wd * (7 + 5 * rng.random()), 12)
        phase = np.cumsum(.5 / lam) * 2 * np.pi + rng.random() * 6.28
        off = (.20 * np.sin(phase) + .12 * np.sin(phase * .53 + rng.random() * 6.28)) * wd
        taper = np.minimum(si, length - si) / np.maximum(lam * .5, 1)
        off *= np.clip(taper, 0, 1)
        tx = np.gradient(x)
        ty = np.gradient(y)
        n = np.maximum(np.hypot(tx, ty), 1e-6)
        x = x + off * (-ty / n)
        y = y + off * (tx / n)
        x = smooth_line(x, 6)
        y = smooth_line(y, 6)
        tx = np.gradient(x)
        ty = np.gradient(y)
        n = np.maximum(np.hypot(tx, ty), 1e-6)
        tx /= n
        ty /= n
        ang = np.unwrap(np.arctan2(ty, tx))
        kappa = np.gradient(ang) / (np.maximum(np.hypot(np.gradient(x), np.gradient(y)), 1e-6) * CELL)  # per metre
        kappa = smooth_line(kappa, 12) * np.clip(np.minimum(si, length - si) / np.maximum(wd * 1.5, 4), 0, 1)
        ir = np.clip(np.rint(y).astype(np.int64), 0, h - 1)
        ic = np.clip(np.rint(x).astype(np.int64), 0, w - 1)
        # the wider channel wins a cell two lines share
        better = ~seed[ir, ic] | (val >= out['la'][ir, ic])
        ir, ic = ir[better], ic[better]
        seed[ir, ic] = True
        out['py'][ir, ic] = y[better]
        out['px'][ir, ic] = x[better]
        out['la'][ir, ic] = val[better]
        out['tx'][ir, ic] = tx[better]
        out['tz'][ir, ic] = ty[better]
        out['k'][ir, ic] = kappa[better]
    return seed, out


def channel_width(la):
    """Metres: 9 m where a swale begins, 24 m at 25 ha of catchment, 60 m at about 8 km2."""
    return np.interp(la, [SMALL, BIG, 6.9], [9.0, 24.0, 60.0])


def channel_depth(la):
    return np.interp(la, [SMALL, BIG, 6.9], [.25, 1.0, 3.0])


def carve_field(seed, f, wob, rng_gain, spread):
    """Depth (positive) and position across the channel. Banks are abrupt only on the outside of a bend;
    on the inside and along straight reaches the bed slopes up gently."""
    h, w = seed.shape
    d, ry, rx = jfa(seed, passes=32, posy=f['py'], posx=f['px'])
    d = d * CELL
    la = f['la'][ry, rx]
    width = channel_width(la) * rng_gain * spread
    depth = channel_depth(la) * np.clip(rng_gain ** 1.5, .6, 1.25)   # no two reaches alike
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    tx, tz, k = f['tx'][ry, rx], f['tz'][ry, rx], f['k'][ry, rx]
    side = tx * (yy - f['py'][ry, rx]) - tz * (xx - f['px'][ry, rx])
    outer = np.where(side * k < 0, smooth(.0012, .007, np.abs(k)), 0).astype(np.float32)
    outer = gauss(outer, 2.0, 8)     # no jump where the nearest point of the line changes
    t = d / (width * .5) + .05 * wob
    # a flat bed; on the outside of a bend an abrupt cut bank, elsewhere a slope of its own width
    soft = .5 + .5 * wob.clip(-1, 1)
    bi = (.45 + .15 * soft) + (.84 - (.45 + .15 * soft)) * outer
    bo = (1.25 + .35 * soft) + (1.0 - (1.25 + .35 * soft)) * outer
    prof = 1 - smooth(bi, bo, t)
    prof = np.where(d > 300, 0, prof)
    return gauss(depth * prof, 1.0, 4), t, depth, outer


def oriented_noise(fx, fz, rng, along_m, across_m, n_dirs=8):
    """Noise stretched along the flow without converging into blots as streamline integration does:
    the same white noise filtered by n_dirs oriented gaussians, picked per cell by the local direction."""
    h, w = fx.shape
    white = np.fft.rfft2(rng.standard_normal((h, w)).astype(np.float32))
    ky = np.fft.fftfreq(h, CELL)[:, None]
    kx = np.fft.rfftfreq(w, CELL)[None, :]
    fields = []
    for k in range(n_dirs):
        th = np.pi * k / n_dirs
        ka = kx * np.cos(th) + ky * np.sin(th)   # along direction th (x, z)
        kc = -kx * np.sin(th) + ky * np.cos(th)
        g = np.exp(-2 * np.pi ** 2 * ((along_m * ka) ** 2 + (across_m * kc) ** 2))
        f = np.fft.irfft2(white * g, s=(h, w)).astype(np.float32)
        fields.append(f / f.std())
    ang = (np.arctan2(fz, fx) % np.pi) / np.pi * n_dirs
    i0 = ang.astype(np.int64) % n_dirs
    i1 = (i0 + 1) % n_dirs
    t = (ang - np.floor(ang)).astype(np.float32)
    stack = np.stack(fields)
    rows, cols = np.mgrid[0:h, 0:w]
    a, b = stack[i0, rows, cols], stack[i1, rows, cols]
    # equal-power so the blend between two directions does not lose amplitude
    out = a * np.cos(t * np.pi / 2) + b * np.sin(t * np.pi / 2)
    return (out / out.std()).astype(np.float32)


def table(d, pts):
    xs, ys = zip(*pts)
    return np.interp(d, xs, ys).astype(np.float32)


def hillshade(z, cell, az=300, alt=32, vz=1.0):
    """Row 0 is z smallest (north); x to the right, so north is up in the picture."""
    gz, gx = np.gradient(z.astype(np.float64) * vz, cell)
    # light from azimuth `az` measured from north (-z) clockwise (towards +x)
    azr = np.radians(az)
    lx, lz = np.sin(azr) * np.cos(np.radians(alt)), -np.cos(azr) * np.cos(np.radians(alt))
    ly = np.sin(np.radians(alt))
    n = np.sqrt(gx * gx + gz * gz + 1)
    return np.clip((-gx * lx - gz * lz + ly) / n, 0, 1)


def to_png(path, a, lo=None, hi=None):
    lo = np.nanmin(a) if lo is None else lo
    hi = np.nanmax(a) if hi is None else hi
    Image.fromarray((np.clip((a - lo) / (hi - lo), 0, 1) * 255 + .5).astype(np.uint8)).save(path)


def mesa_footprint(project, xs, zs):
    """Cells under a backdrop mesa (Three (8000 + bx, bz, 2060 - by), so road-local (x, 2060 + glb z))."""
    glb = Path(project) / 'Tools/calichestreet/backdrop/exports/SM_Caliche_Mesas.glb'
    b = glb.read_bytes()
    size = struct.unpack_from('<I', b, 12)[0]
    j = json.loads(b[20:20 + size])
    bin_ = b[20 + size + 8:]
    fx, fz, fy = [], [], []
    for m in j['meshes']:
        for p in m['primitives']:
            a = j['accessors'][p['attributes']['POSITION']]
            v = j['bufferViews'][a['bufferView']]
            P = np.frombuffer(bin_, '<f4', a['count'] * 3, v.get('byteOffset', 0) + a.get('byteOffset', 0)).reshape(-1, 3)
            fx.append(P[:, 0])
            fy.append(P[:, 1])
            fz.append(2060 + P[:, 2])
    fx, fy, fz = np.concatenate(fx), np.concatenate(fy), np.concatenate(fz)
    H = np.zeros((len(zs) // 4 + 1, len(xs) // 4 + 1), bool)
    c = np.clip(((fx - xs[0]) / (CELL * 4)).astype(int), 0, H.shape[1] - 1)
    r = np.clip(((fz - zs[0]) / (CELL * 4)).astype(int), 0, H.shape[0] - 1)
    H[r, c] = True
    return H, {'bounds_x': [float(fx.min()), float(fx.max())], 'bounds_z': [float(fz.min()), float(fz.max())], 'bottom_y': float(fy.min())}


def placement(near_flow_log, near_step, score_masks, rng_x, rng_z):
    """Which map placement puts the most real drainage where it can be seen from road and farm."""
    k = int(round(20 / near_step))
    h, w = near_flow_log.shape
    h -= h % k
    w -= w % k
    flow = near_flow_log[:h, :w].reshape(h // k, k, w // k, k).max(axis=(1, 3))
    strength = np.clip(flow - 5.0, 0, 1.7)
    ex = (np.arange(flow.shape[1]) + .5) * 20 - h // k * 10.0
    ey = (np.arange(flow.shape[0]) + .5) * 20 - h // k * 10.0
    EX, EY = np.meshgrid(ex, ey)
    best, rows = None, []
    for gx0 in rng_x:
        for gz0 in rng_z:
            # column = east, row 0 = north: x = E - E0 + gx0, z = (N0 - N) + gz0
            x, z = EX + gx0, EY + gz0
            dist = score_masks(x, z)
            view = smooth(40, 110, dist) * (1 - smooth(250, 550, dist))
            farm = 1 - smooth(200, 700, MG.outside((20, 175, 140, 280), x, z))
            s = float((strength * view * (1 + 2 * farm)).sum())
            rows.append((s, float(gx0), float(gz0)))
            if best is None or s > best[0]:
                best = (s, float(gx0), float(gz0))
    return best, sorted(rows, reverse=True)[:8]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--terrain', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--project', type=Path, default=Path.home() / 'SIGNAL-47/unreal/Signal47')
    ap.add_argument('--place', type=float, nargs=2, default=None, help='road-local x, z of the map centre; searched when left out')
    ap.add_argument('--cache', type=Path, default=None)
    ap.add_argument('--no-mesa-floor', action='store_true', help='SM_FF_Range stands in Freeform: no lift to stay above the mesas foot, arroyos cut in full (the mesas, if they ever stand on this table, then float over its hollows)')
    args = ap.parse_args()
    out = args.out
    for d in ('masks', 'preview', 'report'):
        (out / d).mkdir(parents=True, exist_ok=True)
    cache = args.cache or (out / 'cache')
    cache.mkdir(parents=True, exist_ok=True)
    report = {}

    # ---------- port check against the mesh's own numbers ----------
    probes = json.loads((args.project / 'Tools/calichenature/ground/nature-placement.json').read_text())['height_probes']
    errs = []
    for x, z, y in probes:
        ax = abs(x)
        v = float(B.hlow(np.array(float(x)), np.array(float(z)))) if ax <= 18 else float(B.grid_y(x, z))
        errs.append(abs(v - y))
    contact = json.loads((args.project / 'Tools/calichestreet/ground/source_contact.json').read_text())['height_samples']
    errs2 = []
    for s in contact:
        x, z = s['road_local_xz']
        if abs(x) > 18:  # the rest stands on the road's and pavement's own strips, not on the grid
            errs2.append(abs(float(B.grid_y(x, z)) - s['runtime_floor_m']))
    report['port_check'] = {'height_probes_n': len(errs), 'height_probes_max_error_m': max(errs),
                            'source_contact_n': len(errs2), 'source_contact_max_error_m': max(errs2)}
    log('port', report['port_check'])
    assert max(errs) < 1e-3 and max(errs2) < 1e-3

    # ---------- the game's grid ----------
    xs = np.arange(X0, X1 + .5, CELL)
    zs = np.arange(Z0, Z1 + .5, CELL)
    W, H = len(xs), len(zs)
    XX, ZZ = np.meshgrid(xs.astype(np.float64), zs.astype(np.float64))
    log('grid', W, H, W * H * 2 / 1e6, 'MB')

    # ---------- masks: W = 0 in them, then the contract's smooth overgang ----------
    w_mask, null_mask, parts = MG.weight(XX, ZZ, margin=2 * CELL)
    report['masks'] = parts
    log('masks done, null share', float(null_mask.mean()))
    # distance from the null area, at 16 m, for the amplitudes
    c16 = (null_mask[::4, ::4])
    dist16, ny16, nx16 = jfa(c16)
    dist16 *= CELL * 4
    d_cols = np.linspace(0, dist16.shape[1] - 1, W)
    d_rows = np.linspace(0, dist16.shape[0] - 1, H)
    Dg = bilinear(dist16.astype(np.float32), *np.meshgrid(d_cols, d_rows))
    # a fair distance measure for placement: the highway, farm and the roads, coarsely
    def score_dist(x, z):
        d = MG.view_distance(x, z)
        return d

    # ---------- the real map under the game ----------
    near, near_meta = load_dem(args.terrain / 'dem/near_4km_2m_h16.json')
    far, far_meta = load_dem(args.terrain / 'dem/far_24km_10m_h16.json')
    near_flow = np.asarray(Image.open(args.terrain / 'baked/near_mask_flow.png')).astype(np.float32) / 255 * 4.5 + 2
    if args.place is None:
        best, rows = placement(near_flow, 2.0, score_dist, np.arange(-1800, 1801, 100), np.arange(-800, 3001, 100))
        gx0, gz0 = best[1], best[2]
        log('placement', best, rows[:5])
        report['placement_search_top'] = rows[:8]
    else:
        gx0, gz0 = args.place
    report['placement'] = {'map_centre_E': 299000.0, 'map_centre_N': 3588000.0, 'at_road_x': float(gx0), 'at_road_z': float(gz0),
                           'axes': 'x east, -z north (no rotation)'}
    ee = 299000.0 + (XX - gx0)
    nn = 3588000.0 - (ZZ - gz0)
    fc = (ee - far_meta['origin_utm_sw']['easting']) / far_meta['meters_per_pixel']
    fr = ((far_meta['origin_utm_sw']['northing'] + far_meta['extent_m']) - nn) / far_meta['meters_per_pixel']
    z_far = bilinear(far, fc, fr).astype(np.float32)
    nc = (ee - near_meta['origin_utm_sw']['easting']) / near_meta['meters_per_pixel']
    nr = ((near_meta['origin_utm_sw']['northing'] + near_meta['extent_m']) - nn) / near_meta['meters_per_pixel']
    inside = (nc >= 0) & (nc <= near.shape[1] - 1) & (nr >= 0) & (nr <= near.shape[0] - 1)
    z_near = np.where(inside, bilinear(near, nc, nr), z_far).astype(np.float32)
    # the near map is identical to the far map at its edge (tapered there); no seam expected
    edge = np.minimum(np.minimum(nc, near.shape[1] - 1 - nc), np.minimum(nr, near.shape[0] - 1 - nr)) * near_meta['meters_per_pixel']
    k = smooth(0, 150, edge)
    z = z_far + (z_near - z_far) * np.where(inside, k, 0)
    report['map_range_m'] = [float(z.min()), float(z.max())]

    # noise for the part outside the near map, so that its D8 paths wander as the near map's do
    rng = np.random.default_rng(SEED)
    wander = gauss(rng.standard_normal((H, W)).astype(np.float32), 3)
    wander /= wander.std()
    zflow = z + 0.12 * wander * (1 - np.where(inside, k, 0))

    flow_file = cache / ('flow_%g_%g.npy' % (gx0, gz0))
    rec_file = cache / ('rec_%g_%g.npy' % (gx0, gz0))
    if flow_file.exists() and rec_file.exists():
        acc = np.load(flow_file)
        receivers = np.load(rec_file)
    else:
        log('flow accumulation ...')
        acc, receivers = flow_acc(zflow, CELL)
        np.save(flow_file, acc)
        np.save(rec_file, receivers)
    la = np.log10(np.maximum(acc, 1))
    report['flow_max_m2'] = float(acc.max())
    log('flow max', acc.max())

    # ---------- bands of the real relief ----------
    pad = 200
    g2 = gauss(z, 2, pad)        # 8 m
    g10 = gauss(z, 10, pad)      # 40 m
    g55 = gauss(z, 55, pad)      # 220 m
    # the wide band and the regional fall from the 24 km map, so that nothing wraps
    far1200 = gauss(far, 120)
    far300 = gauss(far, 30)
    l1200 = bilinear(far1200, fc, fr).astype(np.float32)
    l300 = bilinear(far300, fc, fr).astype(np.float32)
    b1 = g2 - g10
    b2 = g10 - g55
    b3 = l300 - l1200
    rms1 = np.sqrt(gauss(b1 * b1, 100, pad))
    rms2 = np.sqrt(gauss(b2 * b2, 200, pad))
    rms3 = float(np.sqrt((b3 * b3).mean()))
    report['real_band_rms_m'] = {'8-40 m': float(b1.std()), '40-220 m': float(b2.std()), '300-1200 m': float(b3.std())}
    log('band rms', report['real_band_rms_m'])
    n1 = b1 / np.maximum(rms1, .03)
    n2 = b2 / np.maximum(rms2, .1)
    n2 = np.tanh(n2 * 1.3) / .75     # flatter tops and floors, sharper edges between them: old fan surfaces
    n3 = b3 / rms3

    # how the amplitudes follow distance from the null areas; rms in metres, peak about twice
    D = Dg
    A1 = table(D, [(0, 0), (40, .07), (120, .12), (300, .16), (600, .14), (1000, .10), (1600, .06)])
    A2 = table(D, [(0, 0), (40, .30), (70, .65), (150, 1.3), (300, 1.7), (600, 2.6), (1000, 3.4), (2000, 4.2)])
    A3 = table(D, [(0, 0), (40, .25), (70, .5), (150, 1.1), (300, 1.8), (600, 4.2), (1000, 7.0), (1600, 10.5), (2600, 13.0)])
    rel = A1 * n1 + A2 * n2 + A3 * n3
    # fans falling from the mountains: part of the real regional fall, measured from the nearest null
    # point so it is nothing at the road, and only from a distance
    ref_rows = np.clip(ny16.astype(np.int64) * 4, 0, H - 1)
    ref_cols = np.clip(nx16.astype(np.int64) * 4, 0, W - 1)
    ref = l1200[ref_rows, ref_cols].astype(np.float32)
    fall16 = gauss(l1200[::4, ::4] - ref, 30, 60)
    fall = bilinear(fall16, *np.meshgrid(d_cols, d_rows))
    G_fall = table(D, [(0, 0), (600, 0), (1200, .12), (2000, .30), (3000, .38)])
    fall = np.where(fall < 0, fall * .35, fall)   # the land rises to the mountains; little sink towards the valley
    rel = rel + G_fall * np.clip(fall, -40, 110)

    # ---------- arroyos along the real drainage ----------
    wob = gauss(rng.standard_normal((H, W)).astype(np.float32), 6, 20)
    wob /= wob.std()
    gain = gauss(rng.standard_normal((H, W)).astype(np.float32), 30, 80)
    gain = 1 + .22 * np.clip(gain / gain.std(), -1.6, 1.6)   # the width comes and goes along the channel as well
    paths = trace_channels(la, receivers, SMALL)
    log('channel lines', len(paths), 'nodes', sum(len(p) for p in paths))
    seed_all, fields_c = channel_seeds(paths, la, rng, H, W)
    is_big = seed_all & (fields_c['la'] >= BIG)
    is_small = seed_all & (fields_c['la'] < BIG)
    # towards a road the channels widen, grow shallow and end in a little fan instead of stopping
    spread = 1 + 1.4 * (1 - smooth(70, 300, D))
    cut_big, t, depth, outer_big = carve_field(is_big, fields_c, wob, gain, spread)
    cut_small, t_s, depth_s, _ = carve_field(is_small, fields_c, wob, gain, spread)
    carve = -cut_big
    carve2 = -cut_small
    t_size = np.clip((depth - 1) / 2, 0, 1)
    terrace = (1 - smooth(1.2, 1.8, t)) * smooth(.2, .7, t_size) * .22 * depth * (1 - outer_big)
    carve = np.minimum(carve, -terrace)
    # direction of flow, from the slightly smoothed real surface
    g5 = gauss(z, 25, pad)      # 100 m: the fall of the land, not of every small hollow
    gz_, gx_ = np.gradient(g5.astype(np.float64), CELL)
    mag = np.maximum(np.hypot(gx_, gz_), 1e-4)
    # rows run with z, so downhill in z is rows; flow unit vector per (col, row)
    fx = (-gx_ / mag).astype(np.float32)
    fz = (-gz_ / mag).astype(np.float32)
    # stretched noise: thin rills and wider ribs, along the flow
    rills = oriented_noise(fx, fz, rng, 30, 8)
    ribs = oriented_noise(fx, fz, rng, 70, 16)
    A_r = table(D, [(0, 0), (60, .01), (140, .04), (300, .05), (600, .045), (1000, .03), (2000, .015)])
    A_b = table(D, [(0, 0), (60, .03), (140, .09), (300, .16), (600, .24), (1000, .26), (2000, .26)])
    patch = gauss(rng.standard_normal((H, W)).astype(np.float32), 50, 120)
    patch /= patch.std()
    patch = np.clip(.55 + .45 * patch, .12, 1.2)     # ribs come and go: no even combing of the whole land
    rel = rel + patch * (A_r * rills + A_b * ribs)
    # hummocks, 0.3-1 m at wavelengths of 12-50 m and in no direction: this is what shows from eye height
    def band(a, b):
        w_ = rng.standard_normal((H, W)).astype(np.float32)
        f_ = gauss(w_, a, 12) - gauss(w_, b, 24)
        return f_ / f_.std()
    wider = band(2.0, 6.0)     # 50-150 m: soft swells, no lumps
    patch2 = gauss(rng.standard_normal((H, W)).astype(np.float32), 30, 80)
    patch2 = np.clip(.45 + .7 * patch2 / patch2.std(), 0, 1.3)    # whole stretches of smooth ground between the rough
    A_w = table(D, [(0, 0), (60, .015), (120, .05), (350, .09), (1000, .10), (2000, .10)])   # rms; 0.3 m at the very most
    rel = rel + patch2 * A_w * wider

    # arroyos begin where the order lets them: from 60 m, in full by 110 m
    Zc = smooth(55, 240, D)
    Zs = smooth(70, 240, D) * (1 - smooth(1100, 1700, D) * .5)
    mesa, mesa_info = mesa_footprint(args.project, xs, zs)
    from_mesa, _, _ = jfa(mesa, passes=16)
    mesa_near = (from_mesa <= 2.5) & (not args.no_mesa_floor)
    mesa_w = bilinear(gauss(mesa_near.astype(np.float32), 1.0), *np.meshgrid(np.linspace(0, mesa.shape[1] - 1, W), np.linspace(0, mesa.shape[0] - 1, H)))
    # the straight wash and the sine stripes out wherever we may; the hills' lobes only far from the road, which rides on them
    fake_near, fake_lobes = B.fake_y(XX, ZZ)
    fake = (fake_near + fake_lobes * smooth(500, 1200, D)).astype(np.float32)
    base = B.grid_y(XX, ZZ).astype(np.float32)
    h0 = base + w_mask * (rel - fake)
    # under a mesa the ground stays above its foot (-4 m), arroyos included: the land is lifted softly and
    # widely, with no clamp and no edge, enough that an arroyo still has room
    lack16 = np.where(mesa_w[::4, ::4] > .05, np.maximum(-.5 - h0[::4, ::4], 0), 0).astype(np.float32)
    lift16 = lack16.copy()
    for _ in range(6):
        lift16 = np.maximum(lack16, gauss(lift16, 8, 30))
    lift16 = gauss(lift16, 2.5, 12)
    lift = bilinear(lift16, *np.meshgrid(np.linspace(0, lift16.shape[1] - 1, W), np.linspace(0, lift16.shape[0] - 1, H)))
    report['mesa'] = mesa_info
    report['mesa']['footprint_share'] = float((mesa_w > .05).mean())
    report['mesa']['lift_max_m'] = float(lift.max())
    room = smooth(.2, 2.6, h0 + lift + 3.5)
    mesa_keep = np.where(mesa_w > .05, room, 1.0).astype(np.float32)
    mesa_keep = gauss(mesa_keep, 3, 12)
    total_carve = np.minimum(carve * Zc, carve2 * Zs) * mesa_keep
    # what is cut is about what the land stands higher round it: the amplitude stays centred
    off = w_mask * (rel - fake + lift + total_carve - gauss(total_carve, 45, 100))
    # by the road, soft enough for a truck: curvature under 0.006/m
    soft = gauss(off, 3, 12)
    k_soft = 1 - smooth(50, 110, D)
    off = off + (soft - off) * k_soft
    off[w_mask <= 0] = 0
    # far out the mesh's triangles are 8-16 m: keep out of Off what they cannot carry, so that feet and wheels
    # on GridY + Off meet the drawn ground there too
    far_soft = gauss(off, 1.4, 8)
    off = off + (far_soft - off) * (.75 * smooth(400, 1000, D))

    # ---------- limits ----------
    h = base + off
    under = float(np.maximum(-3.5 - h, 0)[mesa_w > .05].max(initial=0))
    report['mesa']['deepest_below_minus_3_5_m'] = under

    # slope limit: 12 % within reach of the road, walkable (30 deg) from a distance
    lim = table(D, [(0, .12), (60, .12), (120, .36), (300, .577)]).astype(np.float32)
    locked = (w_mask <= 0)
    cell = CELL
    for it in range(60):
        dh = np.zeros_like(h)
        for dy, dx, dist in ((0, 1, 1.0), (1, 0, 1.0), (1, 1, 1.41421356), (1, -1, 1.41421356)):
            ys, yd = (slice(0, H - dy), slice(dy, H))
            xs_, xd = (slice(0, W - dx), slice(dx, W)) if dx >= 0 else (slice(-dx, W), slice(0, W + dx))
            diff = h[yd, xd] - h[ys, xs_]
            limit = np.minimum(lim[yd, xd], lim[ys, xs_]) * cell * dist
            ex = np.maximum(np.abs(diff) - limit, 0) * np.sign(diff) * .22
            dh[ys, xs_] += ex
            dh[yd, xd] -= ex
        dh[locked] = 0
        h += dh
        if it % 15 == 0:
            log('limiter', it, float(np.abs(dh).max()))
    # whatever is still below a mesa's foot comes up the same soft way, then the last centimetres exactly
    for _ in range(3):
        lack = np.where(mesa_w > .05, np.maximum(-3.45 - h, 0), 0).astype(np.float32)
        if lack.max() <= 0:
            break
        env = lack.copy()
        for _ in range(4):
            env = np.maximum(lack, gauss(env, 5, 20))
        h = h + gauss(env, 1.5, 8) * w_mask
    h = np.where(mesa_w > .05, np.maximum(h, -3.5), h)
    report['mesa']['deepest_below_minus_3_5_m'] = float(np.maximum(-3.5 - h, 0)[mesa_w > .05].max(initial=0))
    off = h - base
    off[locked] = 0

    # ---------- field for UV1: sediment in arroyos and on fans, exposed stone on edges and ridges ----------
    bed = (1 - smooth(.62, 1.05, t)) * Zc * mesa_keep
    gravel_band = (1 - smooth(1.0, 1.9, t)) * Zc * mesa_keep
    small_bed = (1 - smooth(.5, 1.1, t_s)) * Zs * mesa_keep
    gzz, gxx = np.gradient(gauss(z, 1.5, pad).astype(np.float64), CELL)
    real_slope = np.degrees(np.arctan(np.hypot(gxx, gzz))).astype(np.float32)
    fan_noise = gauss(rng.standard_normal((H, W)).astype(np.float32), 45, 120)
    fan_noise /= fan_noise.std()
    fan = smooth(1.8, 4.5, gauss(real_slope, 12, 40)) * smooth(-.6, .9, fan_noise) * smooth(120, 400, D)
    sediment = np.maximum.reduce([.30 * fan, .42 * gravel_band, .75 * small_bed, bed])
    gz2, gx2 = np.gradient(gauss(h, 1.0, 12).astype(np.float64), CELL)
    final_slope = np.degrees(np.arctan(np.hypot(gx2, gz2))).astype(np.float32)
    # exposed stone only where the ground is really steep: cut banks and the real map's own ridges. Never on flat land.
    stone = smooth(20, 32, final_slope)
    stone = np.maximum(stone, smooth(11, 18, real_slope) * smooth(9, 16, final_slope) * .7 * smooth(150, 500, D))
    stone *= (1 - bed)
    fade = smooth(0, .6, w_mask)
    sediment = np.clip(sediment * fade, 0, 1).astype(np.float32)
    stone = np.clip(stone * fade, 0, 1).astype(np.float32)

    # ---------- write ----------
    q = np.rint(off / SCALE_M)
    assert np.abs(q).max() < 32767, np.abs(q).max()
    q[null_mask] = 0
    relief = q.astype('<i2')
    relief.tofile(out / 'relief.bin')
    import hashlib
    sha = hashlib.sha256((out / 'relief.bin').read_bytes()).hexdigest()
    (out / 'relief.json').write_text(json.dumps({'origin_x': X0, 'origin_z': Z0, 'cell_m': CELL, 'width': W, 'height': H, 'scale_m': SCALE_M, 'sha256': sha}) + '\n')
    off = relief.astype(np.float32) * SCALE_M
    h = base + off
    np.savez_compressed(cache / 'fields.npz', sediment=(sediment * 255 + .5).astype(np.uint8), stone=(stone * 255 + .5).astype(np.uint8),
                        dist=np.minimum(D, 60000).astype(np.float32), w=w_mask.astype(np.float32))

    # measures
    gz3, gx3 = np.gradient(h.astype(np.float64), CELL)
    sl = np.hypot(gx3, gz3)
    def zone(a, b):
        return (D >= a) & (D < b)
    amp = {}
    for a, b in ((0, 60), (60, 300), (300, 1000), (1000, 6000)):
        m = zone(a, b) & ~null_mask
        amp['%d-%d' % (a, b)] = {'off_p02': float(np.percentile(off[m], 2)), 'off_p98': float(np.percentile(off[m], 98)), 'off_min': float(off[m].min()), 'off_max': float(off[m].max()),
                                 'slope_pct_p99': float(np.percentile(sl[m], 99) * 100), 'slope_pct_max': float(sl[m].max() * 100), 'share_over_12pct': float((sl[m] > .12).mean()), 'share_over_30deg': float((sl[m] > .577).mean())}
    report['zones'] = amp
    # curvature over 8 m where one drives
    cur = np.abs(h[2:-2, 4:] + h[2:-2, :-4] - 2 * h[2:-2, 2:-2]) / (2 * CELL) ** 2
    cur2 = np.abs(h[4:, 2:-2] + h[:-4, 2:-2] - 2 * h[2:-2, 2:-2]) / (2 * CELL) ** 2
    cz = np.maximum(cur, cur2)
    drive = zone(0, 60)[2:-2, 2:-2]
    report['curvature_over_8m_drive_zone'] = {'p99': float(np.percentile(cz[drive], 99)), 'max': float(cz[drive].max()), 'share_over_0006': float((cz[drive] > .006).mean())}
    report['gain'] = {'note': 'Real rms of each band becomes the table below where one measures it; vertical exaggeration of the near relief',
                      'exaggeration_40_220m_at_300m': float(1.7 / report['real_band_rms_m']['40-220 m']),
                      'exaggeration_40_220m_at_1000m': float(3.4 / report['real_band_rms_m']['40-220 m']),
                      'exaggeration_300_1200m_at_300m': float(1.8 / rms3), 'exaggeration_300_1200m_at_1000m': float(7.0 / rms3),
                      'exaggeration_300_1200m_at_2600m': float(13. / rms3)}
    report['arroyos'] = {'big_seed_cells': int(is_big.sum()), 'small_seed_cells': int(is_small.sum()), 'carved_share': float(((total_carve < -.3) & (w_mask > .5)).mean()), 'lines': len(paths)}
    report['relief_bin_mb'] = (out / 'relief.bin').stat().st_size / 1e6
    report['null_share'] = float(null_mask.mean())
    report['null_exact_zero'] = bool((relief[null_mask] == 0).all())

    # masks and maps
    to_png(out / 'masks/mask_weight.png', w_mask, 0, 1)
    to_png(out / 'masks/sediment.png', sediment, 0, 1)
    to_png(out / 'masks/stone.png', stone, 0, 1)
    to_png(out / 'masks/distance.png', D, 0, 1500)
    to_png(out / 'masks/flow_log.png', la, 2, 6.8)
    (out / 'report/bake_relief.json').write_text(json.dumps(report, indent=1, default=float) + '\n')

    shade = hillshade(h, CELL, vz=1.0)
    shade2 = hillshade(h, CELL, vz=2.0)
    Image.fromarray((np.clip(shade * 1.25, 0, 1) * 255).astype(np.uint8)).save(out / 'preview/skygge_v1.png')
    Image.fromarray((np.clip(shade2 * 1.25, 0, 1) * 255).astype(np.uint8)).save(out / 'preview/skygge_v2.png')
    # old ground alongside
    Image.fromarray((np.clip(hillshade(base, CELL) * 1.25, 0, 1) * 255).astype(np.uint8)).save(out / 'preview/skygge_for.png')
    # with what stays put tinted, to read the map
    tint = np.dstack([shade, shade, shade])
    col = np.array([.82, .66, .52])[None, None] * np.clip(shade * 1.25, 0, 1)[..., None]
    col = col * (1 - .3 * sediment[..., None]) + np.array([.93, .86, .7])[None, None] * sediment[..., None] * .3 * np.clip(shade * 1.25, 0, 1)[..., None]
    col = col * (1 - .35 * stone[..., None]) + np.array([.45, .4, .36])[None, None] * stone[..., None] * .35 * np.clip(shade * 1.25, 0, 1)[..., None]
    col[null_mask] = col[null_mask] * .6 + np.array([.55, .55, .6]) * .4
    Image.fromarray((np.clip(col, 0, 1) * 255).astype(np.uint8)).save(out / 'preview/kart_farget.png')
    np.save(cache / 'height.npy', h.astype(np.float32))
    log(json.dumps(report, indent=1, default=float))


if __name__ == '__main__':
    main()
