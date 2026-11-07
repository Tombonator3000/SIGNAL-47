#!/usr/bin/env python3
"""SM_FF_Range: Freeform's hills and mountains, from the real 24 km USGS 3DEP map at the relief's own placement.

Plain NumPy/SciPy/Pillow; starts neither Blender nor Unreal. Reads the relief table (GridY + Off), so the foot
stands on the ground that is actually drawn; if the relief is baked again, run this again.

    python3 build_range.py --terrain <terreng-2026-11-07> [--preview-only]

What is real and what is made:
- Beyond about 5 km the shape is the map itself, by size: its broad land times FAR_GAIN (2.2), its ridges (150-600 m) times
  RIDGE_GAIN, its rills times RILL_GAIN. The same 2.2 on everything turned every rill into a vertical stripe and every small
  knob of the plain into a cone.
- Near hills, 0.3-8 km out, are moved, not measured where they stand: the game's stretch of the map is its lowest basin, and
  every band of the map says "low" there, so their masses, ridges and gullies come from the map's rough north (ROUGH_*), mirrored
  over the land, with a height from the tables below. Only their second size also listens to the map's swells in their own place.
- Stream-power erosion (ERODE_*) and rock benches (BENCH_*) go on top. They are made, not measured.
- Nothing lies over any ground the game draws. Where there is no hill the surface is the plain, under the lowest of the road's
  land, SARO's and the station's (they differ by up to 25 m in the same place), less SINK. A hill comes up through that; the
  foot line is a contour of two surfaces and has no edge of its own. Where every area has its own ground the sheet is left out.

Out: mesh/SM_FF_Range.glb, range/T_FF_Range_N.png, range/T_FF_Range_M.png, report/build_range.json, cache/range_*.npy
"""
import argparse
import heapq
import json
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

sys.path.insert(0, str(Path(__file__).resolve().parent))
import base_port as B
import masks_game as MG
from build_mesh import write_glb

CELLS = 4096
EXTENT = 24000.0
CELL = EXTENT / CELLS
PLACE_X, PLACE_Z = -1100.0, -200.0        # map centre in road-local metres: bake_relief.py's placement
X0, Z0 = PLACE_X - EXTENT / 2, PLACE_Z - EXTENT / 2
GROUND = (-3000.0, 2300.0, -2700.0, 4400.0)   # the rim of SM_FF_Ground_Road: x0, x1, z0, z1
MESA_Z = 2060.0                           # the mesh goes on prop:streetGroundMesas, which stands at road-local (0, 0, 2060)
SARO_RIM = (-1876.0, 1924.0, -2200.0, 1600.0)  # the rim of SM_FF_Ground_Saro in road terms (site -1900..1900, + SARO_IN_ROAD)
STATION_REACH = 2600.0                    # S47Legs FStationTerrain: its axis, each way from the station's origin
PLAIN_M = 1419.0                          # real height of the road's own stretch of the map
FAR_GAIN, RIDGE_GAIN, RILL_GAIN = 2.2, 1.7, 1.1   # the vertical factor on the real mountains, by the size of the shape
SINK = 1.5                                # the plain under the lowest ground; the meshes are up to .7 m off their tables
FOOT_ROUND = 4.0                          # metres of hill over which the foot comes out of the plain
SKIRT_M = 25.0
BUDGET = 2000000
SEED = 4747

# metres (rms, so the tops are about 2.5 times this) by distance from what must stay put
AMP_MASS = [(280, 0), (400, 5), (700, 18), (1300, 36), (2500, 52), (5000, 42), (8000, 14), (12000, 4)]
AMP_WIDE = [(280, 0), (400, 6), (700, 20), (1300, 36), (2200, 48), (4000, 46), (6000, 30), (8000, 12), (11000, 4)]
BIAS = [(280, -6), (380, -2), (520, 4), (800, 11), (1200, 15), (2000, 12), (3000, 18), (5000, 20), (8000, 12), (12000, 5)]
CAP = [(400, 30), (800, 70), (1500, 120), (3000, 190), (6000, 300), (10000, 400)]   # how tall a near hill gets: a soft ceiling, in metres
FAR_IN = (2500.0, 7000.0)
ROUGH_ROWS, ROUGH_COLS = 430, 1500        # the map's rough north, in 10 m cells: row 0 is north
RIDGE_SHARES = (.30, .20)                 # knife edges: where the 250-950 m and the 75-250 m bands cross zero a crest runs, as on real spurs
ROUGH_SHARES = (.34, .22, .14, .06)       # of a hill's own height, in rms, for shapes of 1-4 km, 250-950 m, 75-250 m and under 75 m
ERODE_STEPS, ERODE_K = 8, .012
TALUS_DEG = 38.0
BENCH_M, BENCH_NEAR = 38.0, .13           # rock benches: their height, and how much of a step is taken
HAZE_NOTE = 'the haze is in the material (M_S47_FreeformRange), by distance from the camera'


def log(*a):
    print('%7.1f' % (time.time() - T0), *a, flush=True)


T0 = time.time()


def smooth(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def table(d, pts):
    xs, ys = zip(*pts)
    return np.interp(d, xs, ys).astype(np.float32)


def gauss(a, sig):
    return ndi.gaussian_filter(a, sig, mode='nearest', truncate=3.0)


def gauss_fft(a, sig):
    pad = int(min(4 * sig, a.shape[0] // 2))
    p = np.pad(a, pad, mode='reflect')
    ky = np.fft.fftfreq(p.shape[0])[:, None]
    kx = np.fft.rfftfreq(p.shape[1])[None, :]
    out = np.fft.irfft2(np.fft.rfft2(p) * np.exp(-2 * (np.pi * sig) ** 2 * (kx ** 2 + ky ** 2)), s=p.shape).astype(np.float32)
    return out[pad:-pad, pad:-pad] if pad else out


def resample(a, n):
    """A square map over the whole extent, vertices on its corners, to n x n the same way."""
    t = np.linspace(0, a.shape[0] - 1.000001, n)
    i = t.astype(np.int64)
    f = (t - i).astype(np.float32)
    rows = a[i] * (1 - f)[:, None] + a[i + 1] * f[:, None]
    return rows[:, i] * (1 - f)[None, :] + rows[:, i + 1] * f[None, :]


def load_inputs(terrain):
    meta = json.loads((terrain / 'dem/far_24km_10m_h16.json').read_text())
    u = np.fromfile(terrain / 'dem/far_24km_10m_h16.raw', dtype='<u2').reshape(meta['height'], meta['width']).astype(np.float32)
    dem = meta['zmin_m'] + u / 65535 * (meta['zmax_m'] - meta['zmin_m'])
    root = terrain / 'freeformterrain'
    rj = json.loads((root / 'relief.json').read_text())
    off = np.fromfile(root / 'relief.bin', dtype='<i2').reshape(rj['height'], rj['width']).astype(np.float32) * rj['scale_m']
    return dem, off, rj, root


def distance_field(n):
    """Metres from what must stay put, over the whole map. Its roads stop at the rim of the ground."""
    t = X0 + np.arange(n) * EXTENT / (n - 1), Z0 + np.arange(n) * EXTENT / (n - 1)
    XX, ZZ = np.meshgrid(t[0], t[1])
    _, null, _ = MG.weight(XX, ZZ, margin=8.0)
    inside = (XX >= GROUND[0]) & (XX <= GROUND[1]) & (ZZ >= GROUND[2]) & (ZZ <= GROUND[3])
    null &= inside
    return (ndi.distance_transform_edt(~null) * EXTENT / (n - 1)).astype(np.float32)


def ground_field(off, rj, n):
    """GridY + Off, the road's drawn ground, on the fine grid; beyond the rim its nearest value, fading into the plain."""
    t = np.arange(n) * EXTENT / (n - 1)
    # only the rim's rectangle has ground, so only that is worth the port's time
    xs, zs = X0 + t, Z0 + t
    x_in = np.nonzero((xs >= GROUND[0] - CELL) & (xs <= GROUND[1] + CELL))[0]
    z_in = np.nonzero((zs >= GROUND[2] - CELL) & (zs <= GROUND[3] + CELL))[0]
    XX, ZZ = np.meshgrid(xs[x_in], zs[z_in])
    base = B.grid_y(XX, ZZ).astype(np.float32)
    Off = off_at(off, rj, XX, ZZ)
    G = np.full((n, n), np.nan, np.float32)
    G[np.ix_(z_in, x_in)] = base + Off
    inside = ~np.isnan(G)
    # beyond the rim, on a coarser grid
    k = 8
    coarse = G[::k, ::k]
    mask = np.isnan(coarse)
    idx = ndi.distance_transform_edt(mask, return_distances=False, return_indices=True)
    filled = coarse[idx[0], idx[1]]
    dist = ndi.distance_transform_edt(mask).astype(np.float32) * CELL * k
    filled = gauss(filled, 6)
    filled = filled * (1 - smooth(600, 5000, dist))
    ext = resample(filled, n)
    G2 = np.where(inside, G, ext)
    return G2, inside


def off_at(off, rj, XX, ZZ):
    c = np.clip((XX - rj['origin_x']) / rj['cell_m'], 0, rj['width'] - 1.001)
    r = np.clip((ZZ - rj['origin_z']) / rj['cell_m'], 0, rj['height'] - 1.001)
    return ndi.map_coordinates(off, [r, c], order=1).astype(np.float32)


def station_land(XX, ZZ):
    """S47Legs FStationTerrain in the road's terms: its height less STATION_DY, as the shared landscape stands in station01. No Off:
    the relief does not reach the station's own ground. Its far triangles are long, so the grid is ported too."""
    v = [0.0]
    x, step = 0.0, 4.0
    while x < STATION_REACH:
        if x >= 60:
            step *= 1.2
        x += step
        v.append(round(x, 2))
    axis = np.array([-a for a in v[:0:-1]] + v)
    ground = -.15                                        # S47Geo STATION_GROUND
    at_end = float(B.hlow(np.array(B.END[0]), np.array(B.END[1])))
    dy = ground - at_end
    truck = (-19.25, 31.4)                               # S47Geo STATION_TRUCK
    pad = (-27.6, -12.2, 26.3, 39.8)                     # S47Geo STATION_PAD
    NX, NZ = np.meshgrid(axis, axis)
    rx, rz = B.END[0] + (NZ - truck[1]), B.END[1] - (NX - truck[0])
    h = B.hlow(rx, rz)
    bend = (h - at_end) * (1 - smooth(-600, -560, rx)) * (1 - smooth(60, 160, np.hypot(rx - B.END[0], rz - B.END[1])))
    land = h + dy - bend
    flat = np.maximum(1 - smooth(70, 130, np.hypot(NX, NZ - 8)), 1 - smooth(0, 8, MG.outside(pad, NX, NZ)))
    H = land + (ground - land) * flat
    patch = (rx >= -594) & (rx <= -188) & (rz >= 428) & (rz <= 612)
    H = np.where(patch, land - .7, H)
    sx, sz = truck[0] - (ZZ - B.END[1]), truck[1] + (XX - B.END[0])
    ci = np.clip(np.searchsorted(axis, sx) - 1, 0, len(axis) - 2)
    ri = np.clip(np.searchsorted(axis, sz) - 1, 0, len(axis) - 2)
    u = np.clip((sx - axis[ci]) / (axis[ci + 1] - axis[ci]), 0, 1)
    w = np.clip((sz - axis[ri]) / (axis[ri + 1] - axis[ri]), 0, 1)
    y = (H[ri, ci] * (1 - u) + H[ri, ci + 1] * u) * (1 - w) + (H[ri + 1, ci] * (1 - u) + H[ri + 1, ci + 1] * u) * w
    # (each cell's two triangles can lie below this bilinear value by a quarter of the cell's twist)
    twist = np.abs(H[ri, ci] + H[ri + 1, ci + 1] - H[ri, ci + 1] - H[ri + 1, ci]) * .25
    return (y - twist - dy).astype(np.float32), (np.abs(sx) <= axis[-1]) & (np.abs(sz) <= axis[-1])


def beyond(delta, have, reach):
    """What a ground asks of the plain, carried on past its rim and fading there, so that the plain has no cliff at the rim."""
    idx = ndi.distance_transform_edt(~have, return_distances=False, return_indices=True)
    dist = ndi.distance_transform_edt(~have).astype(np.float32) * CELL * 4
    return delta[idx[0], idx[1]] * (1 - smooth(0, reach, dist))


def plain_field(G, off, rj, n):
    """The surface where there is no hill: smooth, so few triangles carry it, and SINK under every ground drawn over it.
    The shared sheet is the same in all areas; only the ground under it differs (road: G. SARO: SaroLand + Off. Station: its own).
    Out: P, the lowest ground U on the fine grid, and the land where all three have their own ground (the sheet can go there)."""
    k = 4
    Gc = G[::k, ::k]
    t = X0 + np.arange(n)[::k] * CELL, Z0 + np.arange(n)[::k] * CELL
    XX, ZZ = np.meshgrid(t[0], t[1])
    in_saro = (XX >= SARO_RIM[0]) & (XX <= SARO_RIM[1]) & (ZZ >= SARO_RIM[2]) & (ZZ <= SARO_RIM[3])
    d_saro = np.where(in_saro, B.saro_land(XX - MG.SARO_IN_ROAD[0], ZZ - MG.SARO_IN_ROAD[1]) + off_at(off, rj, XX, ZZ) - Gc, 0).astype(np.float32)
    d_saro = np.minimum(d_saro, 0)
    Y_st, in_st = station_land(XX, ZZ)
    d_st = np.where(in_st, np.minimum(Y_st - Gc, 0), 0).astype(np.float32)
    # the road itself runs on HLow, not on the grid
    near_road = np.abs(XX) < 60
    d_road = np.where(near_road, np.minimum(B.hlow(XX, ZZ) - .7 - Gc, 0), 0).astype(np.float32)
    lack = np.minimum.reduce([beyond(d_saro, in_saro, 900), beyond(d_st, in_st, 900), d_road])
    U = Gc + lack
    log('plain: SaroLand under the road by', float(d_saro.min()), 'the station by', float(d_st.min()), 'the road deck by', float(d_road.min()))
    low = ndi.grey_erosion(U, size=(7, 7))
    Pc = gauss(low, 2.0)
    Uf = G + resample(lack, n)
    P = resample(Pc, n) - SINK
    P = np.minimum(P, Uf - SINK)
    in_rim = (XX >= GROUND[0]) & (XX <= GROUND[1]) & (ZZ >= GROUND[2]) & (ZZ <= GROUND[3])
    covered = resample((in_saro & in_st & in_rim).astype(np.float32), n) > .999
    return P.astype(np.float32), Uf.astype(np.float32), covered


def rough_bands(dem, n):
    """The bands of the map's rough north, each normalised, mirrored over the whole land with a slow wander: real ridges and real
    gullies for the near hills, which have only a plain's slight swells in their own place."""
    g12, g40, g150, g600, g2400 = (gauss_fft(dem, s) for s in (1.2, 4, 15, 60, 240))
    rng = np.random.default_rng(SEED + 3)
    grid = np.linspace(0, dem.shape[0] - 1, n)
    wx = gauss(rng.standard_normal((n // 8 + 1, n // 8 + 1)).astype(np.float32), 9)
    wz = gauss(rng.standard_normal((n // 8 + 1, n // 8 + 1)).astype(np.float32), 9)
    wx, wz = resample(wx / wx.std(), n) * 16, resample(wz / wz.std(), n) * 16
    UU = grid[None, :] + wx
    VV = grid[:, None] + wz

    def mirror(v, top):
        return top - np.abs(np.mod(v, 2 * top) - top)
    U, V = mirror(UU, ROUGH_COLS - 1), mirror(VV, ROUGH_ROWS - 1)
    out = []
    for a, b, sig, eps in ((g600, g2400, 200, 1.0), (g150, g600, 90, .4), (g40, g150, 60, .15), (g12, g40, 30, .05), (dem, g12, 20, .03)):
        band = (a - b)[:ROUGH_ROWS, :ROUGH_COLS]
        norm = band / np.sqrt(gauss_fft(band * band, sig) + eps ** 2)
        out.append(ndi.map_coordinates(norm, [V, U], order=1).astype(np.float32))
    return out


def benches(A, n):
    """Harder beds that stand out as ledges across the slope, so that the eye meets something other than what runs downhill.
    They dip and wander; a step takes the slope behind it, a riser the rock in front."""
    rng = np.random.default_rng(SEED + 4)
    warp = gauss(rng.standard_normal((n // 8 + 1, n // 8 + 1)).astype(np.float32), 22)
    warp = resample(warp / warp.std(), n) * 16
    patches = gauss(rng.standard_normal((n // 8 + 1, n // 8 + 1)).astype(np.float32), 12)
    patches = smooth(-.1, 1.0, resample(patches / patches.std(), n))
    jj, ii = np.meshgrid(np.arange(n, dtype=np.float32) * CELL, np.arange(n, dtype=np.float32) * CELL)
    h = A + warp + .018 * jj + .011 * ii
    q = h / BENCH_M + .22 * np.sin(h / BENCH_M * 1.7)   # no two beds the same
    f = q - np.floor(q)
    stair = np.floor(q) + smooth(.45, .95, f)
    k = BENCH_NEAR * smooth(8, 45, A) * patches
    return A + k * (stair * BENCH_M - h)


def priority_flood(z):
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
    offs = (-w - 1, -w, -w + 1, -1, 1, w - 1, w, w + 1)
    while heap:
        c, i = pop(heap)
        x = i % w
        for o in offs:
            j = i + o
            if j < 0 or j >= h * w or abs(j % w - x) > 1 or closed[j]:
                continue
            closed[j] = 1
            v = zf[j]
            if v <= c:
                v = c + 1e-3
                zf[j] = v
            push(heap, (v, j))
    return np.array(zf, np.float64).reshape(h, w)


def erode(h0, weight, cell, steps, k):
    """Stream power on a land that has only just been given its height: the drainage it would have had."""
    h = h0.astype(np.float64).copy()
    H, W = h.shape
    yy, xx = np.mgrid[0:H, 0:W]
    acc = None
    rng = np.random.default_rng(SEED + 2)
    route_noise = gauss(rng.standard_normal((H, W)), 2.5)
    route_noise = 2.2 * route_noise / route_noise.std() * weight
    for it in range(steps):
        # routed over a slightly uneven surface: on an even slope D8 alone draws parallel stripes, and real rills meet
        zf = priority_flood(h + route_noise)
        p = np.pad(zf, 1, mode='edge')
        best = np.zeros((H, W))
        rec = yy * W + xx
        for dy, dx in ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)):
            nb = p[1 + dy:1 + dy + H, 1 + dx:1 + dx + W]
            drop = (zf - nb) / (np.hypot(dy, dx) * cell)
            m = drop > best
            best[m] = drop[m]
            rec[m] = (yy[m] + dy) * W + xx[m] + dx
        order = np.argsort(-zf.ravel(), kind='stable').tolist()
        r = rec.ravel().tolist()
        acc = [cell * cell] * (H * W)
        for i in order:
            j = r[i]
            if j != i:
                acc[j] += acc[i]
        acc = np.array(acc).reshape(H, W)
        cut = np.minimum(k * np.sqrt(acc) * best, .8 * best * cell) * weight
        h -= cut
        # the slopes creep after it, more on the steep, so no valley has walls of one cell
        lap = ndi.laplace(h)
        h += .16 * lap * weight
        if it % 5 == 0:
            log('  erode', it, 'largest cut', float(cut.max()))
    return h.astype(np.float32), acc.astype(np.float32)


def talus(h, cell, deg):
    """What is steeper than the angle of rest slides down: no wall of a single cell anywhere."""
    limit = np.tan(np.radians(deg)) * cell
    h = h.copy()
    for _ in range(24):
        d = np.zeros_like(h)
        for ax in (0, 1):
            diff = np.diff(h, axis=ax)
            ex = np.sign(diff) * np.maximum(np.abs(diff) - limit, 0) * .2
            if ax == 0:
                d[:-1, :] += ex
                d[1:, :] -= ex
            else:
                d[:, :-1] += ex
                d[:, 1:] -= ex
        h += d
    return h


def shape(dem, off, rj):
    n = CELLS + 1
    log('distance from what stays put')
    D_c = distance_field(1025)
    log('real bands')
    g12 = gauss_fft(dem, 1.2)
    g150 = gauss_fft(dem, 15)
    g600 = gauss_fft(dem, 60)
    wide = g150 - g600
    # normalised over about a kilometre: on the plain the swells are slight, but they are the swells that are there. Over three
    # kilometres the mountains in the sum left the plain with nothing
    wide_n = wide / np.sqrt(gauss_fft(wide * wide, 70) + .25 ** 2)
    log('rms', float(wide.std()))
    rng = np.random.default_rng(SEED)
    # the foot wanders, so that it is not an offset of the road
    wander = gauss(rng.standard_normal((1025, 1025)).astype(np.float32), 18)
    wander /= wander.std()
    spur = gauss(rng.standard_normal((1025, 1025)).astype(np.float32), 6)
    wander = np.clip(wander + .5 * spur / spur.std(), -1.8, 1.8)
    # (only outward: the order is 280 at the nearest; at its nearest the foot would otherwise be an offset of the road for kilometres)
    D_w = D_c - 240 * (1 + wander) * smooth(280, 620, D_c)
    D_f = resample(D_w, n)
    wide_f = resample(wide_n, n)
    log('ground')
    G, inside = ground_field(off, rj, n)
    P, U, covered = plain_field(G, off, rj, n)
    # the real land, by the size of the shape: a rill times 2.2 is a vertical stripe, and a knob of the plain a cone
    broad_real = g600 - PLAIN_M
    real = np.where(broad_real > 0, broad_real * FAR_GAIN, broad_real * .3) + (g150 - g600) * RIDGE_GAIN + (g12 - g150) * RILL_GAIN
    real = resample(real, n)
    # sharper tops than floors
    crest = lambda v: np.where(v > 0, 1.1 * np.abs(v) ** 1.2, .55 * v)
    log('rough')
    rm, rw, rn, rf, rr = (resample(b, n) for b in rough_bands(dem, 2049))
    broad = table(D_f, AMP_MASS) * crest(rm) + table(D_f, AMP_WIDE) * crest(.45 * wide_f + .65 * rw) + table(D_f, BIAS)
    # not every hill the same height: a few tall, the rest at half
    sway = gauss(rng.standard_normal((257, 257)).astype(np.float32), 7)
    sway = resample(sway / sway.std(), n)
    broad = np.where(broad > 0, broad * np.clip(1.0 + .5 * sway, .45, 1.7), broad)
    # they are hills, not spires: past their ceiling they widen instead of rising
    cap = table(D_f, CAP)
    broad = np.where(broad > 0, cap * (1 - np.exp(-np.maximum(broad, 0) / cap)), broad)
    del cap
    del sway, wide_f, rm
    env = np.maximum(broad, 0)
    rough = smooth(3, 35, env)
    # what a hill is made of: a share of its own height, so a low hill has low ridges and its foot stays quiet
    shares = ROUGH_SHARES
    ridged = RIDGE_SHARES[0] * (.7 - np.minimum(np.abs(rn), 2.2)) + RIDGE_SHARES[1] * (.7 - np.minimum(np.abs(rf), 2.2))
    hills = broad + env * (shares[0] * np.clip(rw, -1.5, 2.4) + (shares[1] * np.clip(rn, -2, 2.6) + shares[2] * np.clip(rf, -2.4, 2.8) + shares[3] * np.clip(rr, -3, 3) + ridged) * (.25 + .75 * rough))
    del ridged
    del broad, env, rough, rw, rn, rf, rr
    hills = hills * (1 - .85 * smooth(FAR_IN[0] + 2000, FAR_IN[1] + 3500, D_f))
    A = hills * smooth(250, 480, D_f) + real * smooth(FAR_IN[0], FAR_IN[1], D_f)
    del hills, real
    log('erosion')
    k = 2
    A2 = np.maximum(A[::k, ::k], -8)
    w = smooth(3, 40, A2).astype(np.float64)
    eroded, acc = erode(A2, w, CELL * k, ERODE_STEPS, ERODE_K)
    eroded = talus(eroded, CELL * k, TALUS_DEG)
    delta = resample(eroded - A2, n)
    flow = resample(np.log10(np.maximum(acc, 1)).astype(np.float32), n)
    A = A + delta * (A > -8)
    del delta, A2, w, eroded, acc
    A = benches(A, n)
    # the rim of the ground must never be seen from behind: over it just past the rim, not before
    xs, zs = X0 + np.arange(n) * CELL, Z0 + np.arange(n) * CELL
    ax = np.maximum(GROUND[0] - xs, xs - GROUND[1])[None, :]
    az = np.maximum(GROUND[2] - zs, zs - GROUND[3])[:, None]
    past = np.maximum(ax, az).astype(np.float32)       # > 0 outside the rim, < 0 inside it, in metres
    A = np.maximum(A, (SINK + 3) * smooth(0, 120, past) + 10 * smooth(120, 900, past))
    # the foot comes out of the plain round, not on an edge; no hill at all leaves the plain exactly as it was
    A = np.where(A <= 0, 0, np.where(A < FOOT_ROUND, A * A / (2 * FOOT_ROUND), A - FOOT_ROUND / 2))
    A = np.where(A > 0, gauss(A, .7), 0).astype(np.float32)
    # as a hill rises its base goes from the plain to the broad lie of the road's land, so that the road sees it rise as well as SARO
    Gs = resample(gauss(G[::4, ::4], 12), n)
    R = P + (Gs - P) * smooth(.5, 18, A) + A
    return R.astype(np.float32), G.astype(np.float32), inside, D_f, flow, U, A, covered


def rtin(R, tol):
    """Right-triangulated irregular network (Martini's), by level in NumPy: conforming, no T-junctions, no cracks."""
    n = R.shape[0]
    size = n - 1
    levels = []
    a = np.array([[size, size], [0, 0]], np.int16)
    b = np.array([[0, 0], [size, size]], np.int16)
    c = np.array([[0, size], [size, 0]], np.int16)
    E = np.zeros(R.shape, np.float32)
    while True:
        mx = (a[:, 0].astype(np.int32) + b[:, 0]) // 2
        my = (a[:, 1].astype(np.int32) + b[:, 1]) // 2
        levels.append((a, b, c, mx.astype(np.int16), my.astype(np.int16)))
        if np.abs(a[0, 0].astype(np.int32) - b[0, 0]) + np.abs(a[0, 1].astype(np.int32) - b[0, 1]) <= 2:
            break
        # children: (c, a, m), (b, c, m)
        m = np.stack([mx, my], 1).astype(np.int16)
        a, b, c = np.concatenate([c, b]), np.concatenate([a, c]), np.concatenate([m, m])
    log('  rtin levels', len(levels), 'triangles at the last', len(levels[-1][0]))
    for li in range(len(levels) - 1, -1, -1):
        a, b, c, mx, my = levels[li]
        err = np.abs(R[my, mx] - (R[a[:, 1], a[:, 0]] + R[b[:, 1], b[:, 0]]) * .5) / tol[my, mx]
        if li < len(levels) - 1:
            lx = (c[:, 0].astype(np.int32) + a[:, 0]) // 2
            ly = (c[:, 1].astype(np.int32) + a[:, 1]) // 2
            rx = (b[:, 0].astype(np.int32) + c[:, 0]) // 2
            ry = (b[:, 1].astype(np.int32) + c[:, 1]) // 2
            err = np.maximum.reduce([err, E[ly, lx], E[ry, rx]])
        np.maximum.at(E, (my, mx), err)
    return levels, E


def extract(levels, E, budget):
    pos = E[E > 0]
    # each split vertex adds two triangles
    want = min(budget // 2, pos.size - 1)
    t = np.partition(pos, pos.size - want)[pos.size - want]
    leaves = []
    alive = np.ones(2, bool)
    a, b, c, mx, my = levels[0]
    tri = (a, b, c)
    for li in range(len(levels)):
        a, b, c = tri
        mx = (a[:, 0].astype(np.int32) + b[:, 0]) // 2
        my = (a[:, 1].astype(np.int32) + b[:, 1]) // 2
        split = E[my, mx] > t if li < len(levels) - 1 else np.zeros(len(a), bool)
        leaves.append(np.stack([a[~split], b[~split], c[~split]], 1))
        if not split.any():
            break
        m = np.stack([mx[split], my[split]], 1).astype(np.int16)
        tri = (np.concatenate([c[split], b[split]]), np.concatenate([a[split], c[split]]), np.concatenate([m, m]))
    del alive
    return np.concatenate(leaves), float(t)


def boundary_skirts(tri_ids, n_vertices):
    e = np.concatenate([tri_ids[:, [0, 1]], tri_ids[:, [1, 2]], tri_ids[:, [2, 0]]])
    key = np.minimum(e[:, 0], e[:, 1]).astype(np.int64) * n_vertices + np.maximum(e[:, 0], e[:, 1])
    u, inv, cnt = np.unique(key, return_inverse=True, return_counts=True)
    return e[cnt[inv] == 1]


def masks(R, G, inside, D, flow):
    """The material's maps, one texel to a vertex of the fine grid. Row 0 is the smallest z, UV0 v = 0."""
    gz, gx = np.gradient(R, CELL)
    # object space of the imported mesh is (x, z, y) of road-local; v is rows of z
    nx, ny, nz = -gx, -gz, np.ones_like(gx)
    length = np.sqrt(nx * nx + ny * ny + 1)
    N = np.dstack([nx / length, ny / length, nz / length])
    deg = np.degrees(np.arctan(np.hypot(gx, gz))).astype(np.float32)
    rng = np.random.default_rng(SEED + 1)
    # cavity: how far below its surroundings a point lies, at two sizes
    cav = (gauss(R[::4, ::4], 8) - R[::4, ::4]) / 16 + (gauss(R[::4, ::4], 30) - R[::4, ::4]) / 70
    ao = 1 - np.clip(cav, 0, 1) * .75
    ao = resample(ao, R.shape[0])
    gully = smooth(4.3, 6.0, flow)
    # shrubs: dense in the washes and low on the slopes, a little on the fans, almost none on the rock
    above = R - G
    foot = (1 - smooth(0, 30, above)).astype(np.float32)
    k = 2
    noise = gauss(rng.standard_normal((R.shape[0] // k + 1, R.shape[0] // k + 1)).astype(np.float32), .9)
    noise /= noise.std()
    patch = gauss(rng.standard_normal((1025, 1025)).astype(np.float32), 14)
    patch /= patch.std()
    density = np.clip(.30 + .6 * gully, 0, 1) * (1 - smooth(16, 32, deg)) * (.75 + .25 * foot) * np.clip(.7 + .5 * resample(patch, R.shape[0]), .1, 1.3)
    speck = smooth(1.0, 1.9, resample(noise, R.shape[0]) + 1.5 * (density - .4))
    veg = np.clip(speck * np.clip(density * 1.9, 0, 1), 0, 1)
    # rock: where the slope bares it, with the broad shape of the land deciding which side first
    rock_noise = gauss(rng.standard_normal((1025, 1025)).astype(np.float32), 3)
    rock_noise /= rock_noise.std()
    rock = smooth(17, 34, deg + 6 * resample(rock_noise, R.shape[0])) * (1 - .35 * gully) * (1 - foot)
    M = np.dstack([veg, rock, ao, foot])
    return N, M, deg


def write_textures(out, N, M):
    tex = out / 'range'
    tex.mkdir(exist_ok=True)
    # (the world normal's z is rebuilt from x and y: BC5, never below the horizon here)
    rgb = np.clip(N * .5 + .5, 0, 1)
    rgb[..., 2] = 1
    Image.fromarray((rgb[:-1, :-1] * 255 + .5).astype(np.uint8)).save(tex / 'T_FF_Range_N.png', compress_level=4)
    Image.fromarray((np.clip(M[:-1, :-1], 0, 1) * 255 + .5).astype(np.uint8), 'RGBA').save(tex / 'T_FF_Range_M.png', compress_level=4)


def build_mesh(R, U, covered, D, budget):
    n = R.shape[0]
    # tolerance by how far from anywhere one looks out: an angle, a good half metre at a kilometre and a half
    tol = (.28 * np.maximum(D / 700, 1)).astype(np.float32)
    # under ground in every area nothing is seen: triangles there go, so no detail need be spent on them. Only there: past
    # SARO's rim the road's land is not drawn in SARO, and this sheet is the land.
    under = covered & (R - U < -.3)
    hide = ndi.binary_erosion(under, iterations=3, border_value=0)
    deep = ndi.binary_erosion(hide, iterations=6, border_value=0)
    tol = np.where(deep, 1e6, tol).astype(np.float32)
    log('rtin')
    levels, E = rtin(R, tol)
    leaves, t = extract(levels, E, budget)
    del levels, E
    log('leaves', len(leaves), 'threshold', t)
    ids = leaves[:, :, 1].astype(np.int64) * n + leaves[:, :, 0]
    drop = hide.ravel()[ids].all(axis=1)
    ids = ids[~drop]
    log('dropped under the ground', int(drop.sum()), 'left', len(ids))
    used, inv = np.unique(ids, return_inverse=True)
    tris = inv.reshape(-1, 3)
    vr, vc = np.divmod(used, n)
    P = np.stack([X0 + vc * CELL, R[vr, vc], Z0 + vr * CELL], 1).astype(np.float64)
    # glTF: anticlockwise from above, y up (as g873 has it)
    a, b, c = P[tris[:, 0]], P[tris[:, 1]], P[tris[:, 2]]
    flip = np.cross(b - a, c - a)[:, 1] < 0
    tris[flip] = tris[flip][:, ::-1]
    n0 = len(P)
    open_edges = boundary_skirts(tris, n0)
    # a wall down from every open edge, anticlockwise seen from outside: from the hole's rim and from the map's own edge
    verts, ix = np.unique(open_edges, return_inverse=True)
    ix = ix.reshape(-1, 2)
    outer = (vr[verts] == 0) | (vr[verts] == n - 1) | (vc[verts] == 0) | (vc[verts] == n - 1)
    low = P[verts].copy()
    low[:, 1] -= np.where(outer, 300.0, SKIRT_M)
    # the edge (a, b) belongs to a triangle in that order, so the wall below it runs (b, a, a-low), (b, a-low, b-low)
    ea, eb = open_edges[:, 0], open_edges[:, 1]
    la, lb = n0 + ix[:, 0], n0 + ix[:, 1]
    wall = np.concatenate([np.stack([eb, ea, la], 1), np.stack([eb, la, lb], 1)])
    uv_all = np.concatenate([np.stack([(vc + .5) / n, (vr + .5) / n], 1), np.stack([(vc[verts] + .5) / n, (vr[verts] + .5) / n], 1)])
    Pall = np.concatenate([P, low])
    tris_all = np.concatenate([tris, wall])
    # normals from the fine grid, not from the triangles (the walls keep the land's, unseen)
    gz, gx = np.gradient(R, CELL)
    nv = np.stack([-gx[vr, vc], np.ones(n0), -gz[vr, vc]], 1)
    nv /= np.linalg.norm(nv, axis=1, keepdims=True)
    Nall = np.concatenate([nv, nv[verts]])
    report = {'land_triangles': int(len(tris)), 'skirt_triangles': int(len(wall)), 'triangles': int(len(tris_all)), 'vertices': int(len(Pall)),
              'rtin_threshold': t, 'dropped_under_ground': int(drop.sum()), 'open_edges': int(len(open_edges)), 'outer_edge_vertices': int(outer.sum())}
    return Pall, Nall, uv_all, tris_all, report


def measures(R, G, inside, D, U, A):
    """What the road sees, and where the foot lies."""
    n = R.shape[0]
    above = R - G
    out = {}
    xs = X0 + np.arange(n) * CELL
    zs = Z0 + np.arange(n) * CELL
    seen = (above > 0)
    near = D[seen].min() if seen.any() else None
    out['range_above_ground_nearest_to_masks_m'] = float(near)
    plain = A <= 0
    in_map = (xs >= -3040) & (xs <= 3040), (zs >= -2740) & (zs <= 4444)
    rim = np.ix_(in_map[1], in_map[0])
    out['plain_share_of_the_map'] = float(plain[rim].mean())
    out['plain_R_minus_lowest_ground_m'] = {'max': float((R - U)[plain & inside].max()), 'median': float(np.median((R - U)[plain & inside][::37]))}
    out['R_over_road_ground_share'] = {'at_all': float((above[inside] > 0).mean()), 'over_3m': float((above[inside] > 3).mean()),
                                       'near_lt_250m': float((above[inside & (D < 250)] > -1).mean())}
    tall = above > 3
    x_of = np.broadcast_to(xs[None, :], R.shape)
    z_in = (zs >= -1700) & (zs <= 3910)
    west = np.where(x_of < 0, -x_of, 1e9)
    east = np.where(x_of > 0, x_of, 1e9)
    out['foot_more_than_3m_up_from_highway_m'] = {'west_min': float(west[z_in][tall[z_in]].min()), 'east_min': float(east[z_in][tall[z_in]].min()),
                                                  'west_median_by_row': float(np.median(np.where(tall[z_in] & (x_of[z_in] < 0), west[z_in], 1e9).min(axis=1))),
                                                  'east_median_by_row': float(np.median(np.where(tall[z_in] & (x_of[z_in] > 0), east[z_in], 1e9).min(axis=1)))}
    out['range_height_m'] = [float(R.min()), float(R.max())]
    gz, gx = np.gradient(R, CELL)
    deg = np.degrees(np.arctan(np.hypot(gx, gz)))
    out['slope_deg_percentiles_above_ground'] = {str(p): float(np.percentile(deg[seen][::37], p)) for p in (50, 90, 99, 99.9)}
    road = {}
    for name, ex, ez in (('road_z1000', 0, 1000), ('saro_24_-300', 24, -300), ('town_0_2100', 0, 2100), ('road_z-1500', 0, -1500), ('road_z3500', 0, 3500)):
        ey = float(G[int((ez - Z0) / CELL), int((ex - X0) / CELL)]) + 1.7
        row = {}
        for b in range(0, 360, 45):
            s, c = np.sin(np.radians(b)), -np.cos(np.radians(b))
            d = np.arange(100, 20000, 20.)
            px, pz = ex + s * d, ez + c * d
            ok = (px > X0) & (px < X0 + EXTENT) & (pz > Z0) & (pz < Z0 + EXTENT)
            h = np.maximum(R, G)[((pz[ok] - Z0) / CELL).astype(int), ((px[ok] - X0) / CELL).astype(int)]
            ang = np.degrees(np.arctan2(h - ey, d[ok]))
            k = int(np.argmax(ang))
            row[str(b)] = [round(float(ang[k]), 2), int(d[ok][k]), round(float(h[k]), 1)]
        road[name] = row
    out['horizon_deg_at_m_height_by_bearing'] = road
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--terrain', type=Path, required=True)
    ap.add_argument('--budget', type=int, default=BUDGET)
    ap.add_argument('--preview-only', action='store_true', help='fields and measures only, no mesh')
    args = ap.parse_args()
    dem, off, rj, root = load_inputs(args.terrain)
    R, G, inside, D, flow, U, A, covered = shape(dem, off, rj)
    cache = root / 'cache'
    np.save(cache / 'range_R.npy', R)
    np.save(cache / 'range_G.npy', G)
    np.save(cache / 'range_D.npy', D[::4, ::4])
    np.save(cache / 'range_U.npy', U)
    report = {'vertical_factor_far': [FAR_GAIN, RIDGE_GAIN, RILL_GAIN], 'plain_m': PLAIN_M, 'sink_m': SINK, 'amp_mass': AMP_MASS, 'amp_wide': AMP_WIDE, 'bias': BIAS, 'rough': [ROUGH_ROWS, ROUGH_COLS, ROUGH_SHARES, RIDGE_SHARES], 'bench': [BENCH_M, BENCH_NEAR], 'far_in': FAR_IN, 'erode': [ERODE_STEPS, ERODE_K],
               'cell_m': CELL, 'extent_m': EXTENT, 'x0': X0, 'z0': Z0, 'ground_rim': GROUND, 'relief_sha256': rj.get('sha256'), 'haze': HAZE_NOTE}
    report.update(measures(R, G, inside, D, U, A))
    log(json.dumps({k: report[k] for k in ('range_above_ground_nearest_to_masks_m', 'plain_share_of_the_map', 'plain_R_minus_lowest_ground_m', 'R_over_road_ground_share', 'foot_more_than_3m_up_from_highway_m', 'range_height_m', 'slope_deg_percentiles_above_ground')}))
    log('masks')
    N, M, deg = masks(R, G, inside, D, flow)
    np.save(cache / 'range_M.npy', (np.clip(M[::2, ::2], 0, 1) * 255 + .5).astype(np.uint8))
    if not args.preview_only:
        write_textures(root, N, M)
        del N, M, deg
        P, Nv, uv, tris, mesh_report = build_mesh(R, U, covered, D, args.budget)
        report['mesh'] = mesh_report
        # the node's own frame: prop:streetGroundMesas stands at road-local z = 2060
        Pl = P.copy()
        Pl[:, 2] -= MESA_Z
        uv1 = np.zeros((len(P), 2))
        col = np.ones((len(P), 4))
        (root / 'mesh').mkdir(exist_ok=True)
        path = root / 'mesh/SM_FF_Range.glb'
        write_glb(path, 'SM_FF_Range', Pl, Nv, uv, uv1, col, tris)
        report['bounds_min'] = Pl.min(0).tolist()
        report['bounds_max'] = Pl.max(0).tolist()
        report['glb_mb'] = path.stat().st_size / 1e6
        log(json.dumps(mesh_report))
    (root / 'report/build_range.json').write_text(json.dumps(report, indent=1, default=float) + '\n')
    log(json.dumps(report.get('horizon_deg_at_m_height_by_bearing')))


if __name__ == '__main__':
    main()
