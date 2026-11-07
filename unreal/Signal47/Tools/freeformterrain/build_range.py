#!/usr/bin/env python3
"""SM_FF_Range: Freeform's hills and mountains, from the real 24 km USGS 3DEP map at the relief's own placement.

Plain NumPy/SciPy/Pillow; starts neither Blender nor Unreal. Reads the relief table (GridY + Off), so the foot
stands on the ground that is actually drawn; if the relief is baked again, run this again.

    python3 build_range.py --terrain <terreng-2026-11-07> [--preview-only]

What is real and what is made:
- Beyond about 5 km the shape is the map itself: FAR_GAIN times the true height above the plain (2.2, none of it added).
- Near hills, 0.3-8 km out, stand where the map has its own swells, at the same place: the real 150-600 m and 40-150 m
  bands, normalised and given a height by the tables below. Their position and direction are measured; their size is not.
- Stream-power erosion (ERODE_*) is added on top, so the drainage cuts them as it cuts real hills. It is made, not measured.
- The foot is GridY + Off itself, less a metre: the surface comes out of the ground where the hills first rise above it,
  so the foot line is a contour and has no edge of its own. Under it a skirt.

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
PLAIN_M = 1419.0                          # real height of the road's own stretch of the map
FAR_GAIN = 2.2                            # the vertical factor on the real mountains
FOOT_SINK = 1.0
SKIRT_M = 25.0
BUDGET = 1300000
SEED = 4747

# metres (rms, so the tops are about 2.5 times this) by distance from what must stay put
AMP_MASS = [(280, 0), (450, 4), (800, 14), (1400, 30), (2500, 42), (5000, 32), (8000, 12), (12000, 4)]
AMP_WIDE = [(280, 0), (450, 5), (800, 18), (1300, 36), (2200, 48), (4000, 46), (6000, 30), (8000, 12), (11000, 4)]
AMP_NARROW = [(280, 0), (500, .6), (900, 4), (1500, 10), (2500, 16), (4000, 18), (6000, 13), (9000, 4)]
AMP_FINE = [(280, 0), (700, .3), (1500, 2.0), (3000, 4.0), (6000, 4.5), (10000, 2.5)]
BIAS = [(280, -8), (500, -3), (800, 2), (1200, 6), (2000, 12), (3000, 18), (5000, 20), (8000, 12), (12000, 5)]
FAR_IN = (2500.0, 7000.0)
ERODE_STEPS, ERODE_K = 16, .017
TALUS_DEG = 36.0
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
    """GridY + Off, the drawn ground, on the fine grid; beyond the rim its nearest value, fading into the plain."""
    t = np.arange(n) * EXTENT / (n - 1)
    # only the rim's rectangle has ground, so only that is worth the port's time
    xs, zs = X0 + t, Z0 + t
    x_in = np.nonzero((xs >= GROUND[0] - CELL) & (xs <= GROUND[1] + CELL))[0]
    z_in = np.nonzero((zs >= GROUND[2] - CELL) & (zs <= GROUND[3] + CELL))[0]
    XX, ZZ = np.meshgrid(xs[x_in], zs[z_in])
    base = B.grid_y(XX, ZZ).astype(np.float32)
    c = np.clip((XX - rj['origin_x']) / rj['cell_m'], 0, rj['width'] - 1.001)
    r = np.clip((ZZ - rj['origin_z']) / rj['cell_m'], 0, rj['height'] - 1.001)
    Off = ndi.map_coordinates(off, [r, c], order=1)
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
    g40 = gauss_fft(dem, 4)
    g150 = gauss_fft(dem, 15)
    g600 = gauss_fft(dem, 60)
    g2400 = gauss_fft(dem, 240)
    mass = g600 - g2400
    wide = g150 - g600
    narrow = g40 - g150
    fine = g12 - g40
    # normalised over about a kilometre and a half: on the plain the swells are slight, but they are the swells that are there
    wide_n = wide / np.sqrt(gauss_fft(wide * wide, 150) + .4 ** 2)
    narrow_n = narrow / np.sqrt(gauss_fft(narrow * narrow, 100) + .15 ** 2)
    fine_n = fine / np.sqrt(gauss_fft(fine * fine, 60) + .05 ** 2)
    mass_n = mass / np.sqrt(gauss_fft(mass * mass, 300) + 1.0 ** 2)
    log('rms', float(mass.std()), float(wide.std()), float(narrow.std()))
    rng = np.random.default_rng(SEED)
    # the foot wanders, so that it is not an offset of the road
    wander = gauss(rng.standard_normal((1025, 1025)).astype(np.float32), 18)
    wander /= wander.std()
    D_w = D_c + 130 * np.clip(wander, -1.6, 1.6) * smooth(200, 700, D_c)
    D_f = resample(D_w, n)
    wide_f = resample(wide_n, n)
    narrow_f = resample(narrow_n, n)
    mass_f = resample(mass_n, n)
    fine_f = resample(fine_n, n)
    dem_f = resample(gauss_fft(dem, 1.2), n)
    log('ground')
    G, inside = ground_field(off, rj, n)
    real = dem_f - PLAIN_M
    real = np.where(real > 0, real * FAR_GAIN, real * .3)
    # sharper tops than floors: a slope steepens towards its crest and runs out long at its foot, the way these ranges do
    crest = lambda v: np.where(v > 0, 1.15 * np.abs(v) ** 1.45, .55 * v)
    broad = table(D_f, AMP_MASS) * crest(mass_f) + table(D_f, AMP_WIDE) * crest(wide_f) + table(D_f, BIAS)
    # not every hill the same height: the map's own broadest swells (5-20 km) pick a few to be tall and leave the rest at half
    sway = gauss_fft(dem, 240) - gauss_fft(dem, 900)
    sway = resample(sway / sway.std(), n)
    broad = np.where(broad > 0, broad * np.clip(.85 + .55 * sway, .42, 1.6), broad)
    # aprons: what comes down off a hill lies round it, long and low, so a foot runs out instead of stopping in a line
    k_a = 8
    apron = gauss(np.maximum(broad[::k_a, ::k_a], 0), 13)
    broad = broad + .42 * resample(apron, n) * smooth(350, 900, D_f)
    del sway, apron
    # the smaller the shape the more it belongs to the tops: rough above, long smooth aprons at the foot
    rough = smooth(5, 90, broad)
    hills = broad + (table(D_f, AMP_NARROW) * narrow_f + table(D_f, AMP_FINE) * fine_f) * (.2 + .8 * rough)
    del broad, rough, fine_f
    hills = hills * (1 - .85 * smooth(FAR_IN[0] + 2000, FAR_IN[1] + 3500, D_f))
    A = hills * smooth(250, 520, D_f) + real * smooth(FAR_IN[0], FAR_IN[1], D_f)
    del wide_f, narrow_f, mass_f, real, dem_f
    # the rim of the ground must never be seen from behind
    xs, zs = X0 + np.arange(n) * CELL, Z0 + np.arange(n) * CELL
    ax = np.maximum(np.maximum(GROUND[0] - xs, xs - GROUND[1]), -1e9)[None, :]
    az = np.maximum(np.maximum(GROUND[2] - zs, zs - GROUND[3]), -1e9)[:, None]
    past = np.maximum(ax, az).astype(np.float32)       # > 0 outside the rim, < 0 inside it, in metres
    floor = 4.0 * smooth(-160, -20, past) + 10 * smooth(0, 700, past)
    A = .5 * (A + floor + np.sqrt((A - floor) ** 2 + 16))
    log('erosion')
    k = 2
    A2 = A[::k, ::k]
    w = smooth(3, 40, A2).astype(np.float64)
    eroded, acc = erode(A2, w, CELL * k, ERODE_STEPS, ERODE_K)
    eroded = talus(eroded, CELL * k, TALUS_DEG)
    delta = resample(eroded - A2, n)
    flow = resample(np.log10(np.maximum(acc, 1)).astype(np.float32), n)
    A = A + delta
    del delta, A2, w, eroded, acc
    # where the land follows the ground: every hollow of it at the foot, only its broad lie higher up
    Gs = resample(gauss(G[::4, ::4], 12), n)
    k_foot = 1 - smooth(1, 30, A)
    R = Gs + (G - Gs) * k_foot + A - FOOT_SINK * (1 - smooth(1, 6, A))
    # a last, light smoothing of what the map's 10 m cells leave on the amplified land: no steps
    R = gauss(R, .7)
    # (that smoothing may not lift the foot out of a hollow of the ground)
    R = np.where(A < 1, np.minimum(R, G - .4), R)
    return R.astype(np.float32), G.astype(np.float32), inside, D_f, flow


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
    foot = (1 - smooth(0, 40, above)).astype(np.float32)
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


def build_mesh(R, G, inside, D, budget):
    n = R.shape[0]
    # tolerance by how far from anywhere one looks out: an angle, a good half metre at a kilometre and a half
    tol = (.28 * np.maximum(D / 700, 1)).astype(np.float32)
    # under the ground nothing is seen: triangles there go, so no detail need be spent on them
    above = R - G
    under = inside & (above < -.3)
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


def measures(R, G, inside, D):
    """What the road sees, and where the foot lies."""
    n = R.shape[0]
    above = R - G
    out = {}
    xs = X0 + np.arange(n) * CELL
    zs = Z0 + np.arange(n) * CELL
    seen = (above > 0)
    near = D[seen].min() if seen.any() else None
    out['range_above_ground_nearest_to_masks_m'] = float(near)
    tall = above > 3
    rows = np.nonzero(tall.any(axis=1))[0]
    x_of = np.broadcast_to(xs[None, :], R.shape)
    z_in = (zs >= -1700) & (zs <= 3910)
    west = np.where(x_of < 0, -x_of, 1e9)
    east = np.where(x_of > 0, x_of, 1e9)
    out['foot_more_than_3m_up_from_highway_m'] = {'west_min': float(west[z_in][tall[z_in]].min()), 'east_min': float(east[z_in][tall[z_in]].min()),
                                                  'west_median_by_row': float(np.median(np.where(tall[z_in] & (x_of[z_in] < 0), west[z_in], 1e9).min(axis=1))),
                                                  'east_median_by_row': float(np.median(np.where(tall[z_in] & (x_of[z_in] > 0), east[z_in], 1e9).min(axis=1)))}
    del rows
    out['range_height_m'] = [float(R.min()), float(R.max())]
    gz, gx = np.gradient(R, CELL)
    deg = np.degrees(np.arctan(np.hypot(gx, gz)))
    out['slope_deg_percentiles_above_ground'] = {str(p): float(np.percentile(deg[seen][::37], p)) for p in (50, 90, 99, 99.9)}
    road = {}
    for name, ex, ez in (('road_z1000', 0, 1000), ('saro_24_-300', 24, -300), ('town_0_2100', 0, 2100), ('road_z-1500', 0, -1500), ('road_z3500', 0, 3500)):
        ey = float(R[int((ez - Z0) / CELL), int((ex - X0) / CELL)]) + FOOT_SINK + 1.7
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
    R, G, inside, D, flow = shape(dem, off, rj)
    cache = root / 'cache'
    np.save(cache / 'range_R.npy', R)
    np.save(cache / 'range_G.npy', G)
    np.save(cache / 'range_D.npy', D[::4, ::4])
    report = {'vertical_factor_far': FAR_GAIN, 'plain_m': PLAIN_M, 'amp_wide': AMP_WIDE, 'amp_narrow': AMP_NARROW, 'bias': BIAS, 'far_in': FAR_IN, 'erode': [ERODE_STEPS, ERODE_K],
               'cell_m': CELL, 'extent_m': EXTENT, 'x0': X0, 'z0': Z0, 'ground_rim': GROUND, 'relief_sha256': rj.get('sha256'), 'haze': HAZE_NOTE}
    report.update(measures(R, G, inside, D))
    log(json.dumps({k: report[k] for k in ('range_above_ground_nearest_to_masks_m', 'range_height_m', 'slope_deg_percentiles_above_ground')}))
    log('masks')
    N, M, deg = masks(R, G, inside, D, flow)
    np.save(cache / 'range_M.npy', (np.clip(M[::2, ::2], 0, 1) * 255 + .5).astype(np.uint8))
    if not args.preview_only:
        write_textures(root, N, M)
        del N, M, deg
        P, Nv, uv, tris, mesh_report = build_mesh(R, G, inside, D, args.budget)
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
