#!/usr/bin/env python3
"""SM_FF_Range: Freeform's hills and mountains, from the real 24 km USGS 3DEP map at the relief's own placement.

NumPy, SciPy, Pillow and Numba (the drainage is walked cell by cell); starts neither Blender nor Unreal. Reads the relief table
(GridY + Off), so the foot stands on the ground that is actually drawn; if the relief is baked again, run this again.

    python3 build_range.py --terrain <terreng-2026-11-07> [--preview-only]

What is real and what is made:
- Beyond about 5 km the shape is the map itself, by size: its broad land times FAR_GAIN (2.2), its ridges (150-600 m) times
  RIDGE_GAIN, its rills times RILL_GAIN. The same 2.2 on everything turned every rill into a vertical stripe and every small
  knob of the plain into a cone.
- Near hills, 0.3-8 km out, are made, not measured: the game's stretch of the map is its lowest basin, and every band of the map
  says "low" there. Where they stand comes from the map's rough north (ROUGH_*), mirrored over the land, against the threshold
  THRESH. What they look like comes from water: the steady state of stream power (S = uk A^-m) over a drainage network that is
  grown on the land itself, as Basin and Range blocks get theirs. Noise made bread loaves and cones; a network makes crests, side
  valleys and facets, because every cell is a slope down to the valley it drains to. Height is the sum of that walk, so a low
  hill and a tall one have the same kind of shape. CAP only trims what is left too high for its distance.
- Fans (FAN_*) spread from each valley's mouth onto the plain, and a bench of harder beds (BENCH_*) goes on top. Made.
- Nothing lies over any ground the game draws. Where there is no hill the surface is the plain, under the lowest of the road's
  land, SARO's and the station's (they differ by up to 25 m in the same place), less SINK. A hill comes up through that; the
  foot line is a contour of two surfaces and has no edge of its own. Where every area has its own ground the sheet is left out.
- range_foot.bin is how far this surface stands over each area's ground, for what stops the player and the truck at the foot
  (S47Relief::HillAbove). The mesh has no collision: nothing here moves by hitting things.

Out: mesh/SM_FF_Range.glb, range/T_FF_Range_N.png, range/T_FF_Range_M.png, range_foot.{bin,json}, report/build_range.json, cache/range_*.npy
"""
import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

from numba import njit
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

# Where a hill stands: the share of the land that does, by metres from what must stay put, whichever side of the road it is. Rising
# from nothing at NEAREST, so that a lobe narrows to a nose there instead of being cut along a line that follows the road.
NEAREST = 280.0
SHARE = [(280, 0), (300, .025), (360, .11), (450, .22), (560, .30), (800, .38), (1300, .46), (2500, .56), (5000, .58), (8000, .42), (12000, .10)]
EAST_MORE, THREAD_M = .55, 85.0
MIN_HILL_M2 = 90000.0                     # no knob smaller than this: a lone cone on the plain
# The land's water. Steady state of stream power: slope = uk * area^-m, never over the rest angle.
SPL_M = .5
UK = [(280, 26.), (600, 30.), (1200, 42.), (2500, 58.), (5000, 72.), (9000, 80.)]   # metres^(2m) of slope; higher, taller
REST_DEG = (29.0, 42.0)                   # the angle of rest runs between these, softer and harder ground
SPL_STEPS = 26
# what is left too high for its distance is brought down as a whole, keeping its shape: a soft ceiling, in metres
CAP = [(300, 26), (400, 46), (600, 85), (900, 135), (1500, 210), (3000, 330), (6000, 440), (10000, 500)]
FAN_MIN_M2, FAN_RADIUS, FAN_SLOPE = 12000.0, .95, .075   # radius = FAN_RADIUS sqrt(area); height = slope * radius, falling as it grows
GULLY_DEPTH, GULLY_FULL_M2, GULLY_HEAD_M2, GULLY_REACH = 15.0, 22000.0, 500.0, 7000.0   # how deep a slope's rills get, at what area of water
FAR_IN = (2500.0, 7000.0)
ROUGH_ROWS, ROUGH_COLS = 430, 1500        # the map's rough north, in 10 m cells: row 0 is north
TALUS_DEG = 46.0
BENCH_M, BENCH_NEAR = 34.0, .20           # rock benches: their height, and how much of a step is taken
FOOT_SCALE, FOOT_BIAS = .125, -2.0        # range_foot.bin: a step and where its 0 stands
FOOT_STOP_M = .45                         # (for the report) how far a hill stands over the ground before it stops the player
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


def benches(A, n, hard):
    """Harder beds that stand out as ledges across the slope, so that the eye meets something other than what runs downhill.
    They dip and wander; a step takes the slope behind it, a riser the rock in front. More of them in the harder ground."""
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
    k = BENCH_NEAR * smooth(8, 45, A) * patches * (.45 + .55 * smooth(-.6, 1.0, hard))
    return A + k * (stair * BENCH_M - h)


@njit(cache=True)
def route(z, active, seed):
    """Priority flood over the active cells, from the seeds. rec: which neighbour a cell drains to (-1 at a seed). order: receivers
    before donors. No pit survives, since a cell is closed by the first neighbour that reaches it, the lowest on the way up."""
    H, W = z.shape
    N = H * W
    rec = np.full(N, -1, np.int32)
    order = np.zeros(N, np.int32)
    closed = np.zeros(N, np.uint8)
    key = np.zeros(N + 1, np.float32)
    val = np.zeros(N + 1, np.int32)
    size = 0

    def_offs = ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1))
    for r in range(H):
        for c in range(W):
            if seed[r, c]:
                i = r * W + c
                closed[i] = 1
                # push
                k = z[r, c]
                j = size
                size += 1
                while j > 0:
                    p = (j - 1) >> 1
                    if key[p] <= k:
                        break
                    key[j] = key[p]
                    val[j] = val[p]
                    j = p
                key[j] = k
                val[j] = i
    count = 0
    while size > 0:
        c0 = key[0]
        i = val[0]
        size -= 1
        if size > 0:
            k = key[size]
            v = val[size]
            j = 0
            while True:
                a = 2 * j + 1
                if a >= size:
                    break
                if a + 1 < size and key[a + 1] < key[a]:
                    a += 1
                if key[a] >= k:
                    break
                key[j] = key[a]
                val[j] = val[a]
                j = a
            key[j] = k
            val[j] = v
        order[count] = i
        count += 1
        r, c = i // W, i % W
        for dr, dc in def_offs:
            r2, c2 = r + dr, c + dc
            if r2 < 0 or r2 >= H or c2 < 0 or c2 >= W:
                continue
            j2 = r2 * W + c2
            if closed[j2] or not active[r2, c2]:
                continue
            closed[j2] = 1
            rec[j2] = i
            k = max(z[r2, c2], c0 + 1e-3)
            j = size
            size += 1
            while j > 0:
                p = (j - 1) >> 1
                if key[p] <= k:
                    break
                key[j] = key[p]
                val[j] = val[p]
                j = p
            key[j] = k
            val[j] = j2
    return rec, order[:count]


@njit(cache=True)
def steepest(zf, order, rec, active, jr, jc, cell):
    """Each cell's drain, the steepest way down the filled surface. Over a grid whose points are shifted each its own way (jr, jc,
    in cells): on a true grid every run of water goes along one of eight directions, and from above the land is striped with them."""
    H, W = zf.shape
    offs = ((-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1))
    dist = np.zeros(H * W, np.float32)
    for k in range(len(order)):
        i = order[k]
        if rec[i] < 0:
            continue
        r, c = i // W, i % W
        best = 0.0
        bj = rec[i]
        for dr, dc in offs:
            r2, c2 = r + dr, c + dc
            if r2 < 0 or r2 >= H or c2 < 0 or c2 >= W or not active[r2, c2]:
                continue
            drop = zf[r, c] - zf[r2, c2]
            if drop <= 0:
                continue
            dy, dx = dr + jr[r2, c2] - jr[r, c], dc + jc[r2, c2] - jc[r, c]
            s = drop / (dy * dy + dx * dx) ** .5
            if s > best:
                best = s
                bj = r2 * W + c2
        rec[i] = bj
        r2, c2 = bj // W, bj % W
        dy, dx = r2 - r + jr[r2, c2] - jr[r, c], c2 - c + jc[r2, c2] - jc[r, c]
        dist[i] = (dy * dy + dx * dx) ** .5 * cell
    return rec, dist


@njit(cache=True)
def filled(z, rec, order, W):
    """The surface the flood walked up, each cell at least a hair over its drain."""
    zf = z.copy()
    for k in range(len(order)):
        i = order[k]
        j = rec[i]
        r, c = i // W, i % W
        if j >= 0:
            v = zf[j // W, j % W] + 1e-3
            if zf[r, c] < v:
                zf[r, c] = v
    return zf


@njit(cache=True)
def accumulate(rec, order, shape, cell):
    H, W = shape
    acc = np.full(H * W, float(cell * cell), np.float32)
    for k in range(len(order) - 1, -1, -1):
        i = order[k]
        j = rec[i]
        if j >= 0:
            acc[j] += acc[i]
    return acc


@njit(cache=True)
def walk_up(rec, order, acc, dist, uk, rest, m, W):
    """Heights from the seeds up the channels, each its slope over its drain's height. Above the channel head (where stream power would
    ask for more than the rest angle) a cell gives no answer, and the slopes come from the channels: see slopes_from."""
    h = np.full(acc.shape[0], 1e9, np.float32)
    for k in range(len(order)):
        i = order[k]
        j = rec[i]
        if j < 0:
            h[i] = 0
            continue
        r, c = i // W, i % W
        s = uk[r, c] * acc[i] ** -m
        if s < rest[r, c] and h[j] < 1e8:
            h[i] = h[j] + s * dist[i]
    return h


@njit(cache=True)
def slopes_from(h, rest, mask, cell, passes):
    """Everything over the channels: the rest angle up from the nearest of them, so a slope is a plane and its crest where two meet.
    Sixteen neighbours, or the planes turn octagonal."""
    H, W = h.shape
    back = ((-1, 0), (-1, -1), (-1, 1), (0, -1), (-2, -1), (-2, 1), (-1, -2), (-1, 2))
    lens = np.array([1.0, 1.41421356, 1.41421356, 1.0, 2.23606798, 2.23606798, 2.23606798, 2.23606798], np.float32) * cell
    for _ in range(passes):
        for r in range(H):
            for c in range(W):
                if not mask[r, c]:
                    continue
                t = rest[r, c]
                v = h[r, c]
                for k in range(8):
                    r2, c2 = r + back[k][0], c + back[k][1]
                    if r2 >= 0 and c2 >= 0 and c2 < W:
                        v = min(v, h[r2, c2] + t * lens[k])
                h[r, c] = v
        for r in range(H - 1, -1, -1):
            for c in range(W - 1, -1, -1):
                if not mask[r, c]:
                    continue
                t = rest[r, c]
                v = h[r, c]
                for k in range(8):
                    r2, c2 = r - back[k][0], c - back[k][1]
                    if r2 < H and c2 >= 0 and c2 < W:
                        v = min(v, h[r2, c2] + t * lens[k])
                h[r, c] = v
    return h


def drained_land(mask, start, uk, rest, cell, steps, cap):
    """The hills' own height: the steady state of stream power over a network grown on them, from the plain inward.
    What stands too high for its distance (cap) has its valleys brought down, and its slopes rise from them at the same angle as
    any other's: a shape scaled down as a whole is a loaf."""
    active = ndi.binary_dilation(mask, iterations=1)
    seed = active & ~mask
    rng = np.random.default_rng(SEED + 2)
    # routed over an uneven surface at more than one size
    jitter = gauss(rng.standard_normal(mask.shape).astype(np.float32), 1.2)
    jitter2 = gauss(rng.standard_normal(mask.shape).astype(np.float32), 5.0)
    jitter = jitter / jitter.std() + 1.5 * jitter2 / jitter2.std()
    jr = rng.uniform(-.46, .46, mask.shape).astype(np.float32)
    jc = rng.uniform(-.46, .46, mask.shape).astype(np.float32)
    H, W = mask.shape
    h = start.astype(np.float32)
    rec = acc = None
    for it in range(steps):
        # the jitter fades, and the network settles into what its own slopes say
        k = 2.2 * (1 - it / max(steps - 1, 1)) ** 1.2 + .5
        z = h + k * jitter
        rec, order = route(z, active, seed)
        zf = filled(z, rec, order, W)
        rec, dist = steepest(zf, order, rec, active, jr, jc, cell)
        acc = accumulate(rec, order, (H, W), cell)
        new = walk_up(rec, order, acc, dist, uk, rest, SPL_M, W).reshape(H, W)
        if it == steps - 1:
            env = gauss(ndi.maximum_filter(h, size=31), 14)
            scale = gauss(np.minimum(1, cap / np.maximum(env, 1)), 8)
            new = np.where(new < 1e8, new * scale, new)
        new = slopes_from(np.where(active, new, 0).astype(np.float32), rest, mask, cell, 1 if it < steps - 1 else 3)
        d = 1.0 if it >= steps - 2 else .6
        h = h + (new - h) * d
        if it % 5 == 0:
            log('  drainage', it, 'highest', float(h.max()), 'mean over the hills', float(h[mask].mean()))
    return np.where(mask, h, 0).astype(np.float32), acc.reshape(H, W), rec.reshape(H, W)


@njit(cache=True)
def spread_fans(mask, rec, acc, cell, min_area, radius_k, slope0, warp):
    """A cone from each valley's mouth, lower and longer the bigger the valley. Their maximum: where they meet they make the bajada."""
    H, W = mask.shape
    fan = np.zeros((H, W), np.float32)
    count = 0
    for r in range(H):
        for c in range(W):
            if not mask[r, c] or acc[r, c] < min_area:
                continue
            j = rec[r, c]
            if j < 0:
                continue
            if mask[j // W, j % W]:
                continue
            a = acc[r, c]
            rad = radius_k * a ** .5
            rad = min(rad, 900.0)
            top = rad * slope0 * (a / 1e5) ** -.14
            top = min(top, 34.0)
            n = int(rad * 1.5 / cell) + 1
            count += 1
            for r2 in range(max(0, r - n), min(H, r + n + 1)):
                for c2 in range(max(0, c - n), min(W, c + n + 1)):
                    d = ((r2 - r) ** 2 + (c2 - c) ** 2) ** .5 * cell
                    d = d * (1 + .24 * warp[r2, c2]) / rad
                    if d >= 1:
                        continue
                    v = top * (1 - d) ** 1.25
                    if v > fan[r2, c2]:
                        fan[r2, c2] = v
    return fan, count


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


def noise(rng, n, sig, cn=1025):
    v = gauss(rng.standard_normal((cn, cn)).astype(np.float32), sig)
    return resample(v / v.std(), n)


def _disc(r):
    y, x = np.ogrid[-r:r + 1, -r:r + 1]
    return x * x + y * y <= r * r


def near_hills(dem_bands, D2, rng):
    """Height of the near hills on the half grid, with their network's flow and the plan they stand on."""
    rm, rw, rn, rf, wide = dem_bands
    n2 = D2.shape[0]
    c2 = EXTENT / (n2 - 1)
    # Where they stand: the map's rough north with its largest sizes taken out, so both sides of the road have their share, as a rank
    big = noise(rng, n2, 70)      # 3 km or so
    field = .80 * rm + .55 * rw + .30 * rn + .14 * rf + .30 * big + .25 * wide
    field = field - gauss_fft(field, 1800 / c2)
    field = field + .25 * noise(rng, n2, 14)
    order = np.argsort(field[::4, ::4].ravel())
    level = field[::4, ::4].ravel()[order]
    rank = np.interp(field, level, np.linspace(0, 1, level.size)).astype(np.float32)
    # (east of the road the land to spare is less, and every farm and ranch keeps the hills off: it gets more of what it has)
    x2 = (X0 + np.arange(n2) * c2)[None, :]
    plan = rank - (1 - table(D2, SHARE) * (1 + EAST_MORE * smooth(0, 200, x2)))
    mask = (plan > 0) & (D2 > NEAREST)
    # no thread of a ridge alone on the plain: only what a disc as wide as a small hill fits inside, and a little more
    wide_part = ndi.binary_opening(mask, structure=_disc(int(THREAD_M / c2)))
    mask = ndi.binary_dilation(wide_part, structure=_disc(2)) & mask
    # no lone knobs, and no hole that is only a pit
    labels, count = ndi.label(mask)
    sizes = ndi.sum(mask, labels, np.arange(1, count + 1)) * c2 * c2
    keep = np.zeros(count + 1, bool)
    keep[1:] = sizes >= MIN_HILL_M2
    mask = keep[labels]
    labels, count = ndi.label(~mask)
    sizes = ndi.sum(~mask, labels, np.arange(1, count + 1)) * c2 * c2
    fill = np.zeros(count + 1, bool)
    fill[1:] = sizes < 40000
    mask = mask | fill[labels]
    mask &= D2 > NEAREST
    log('hills cover', float(mask.mean()), 'of the map')
    # harder and softer ground, as much as size, decide how a block wears
    hard = noise(rng, n2, 22)
    uk = table(D2, UK) * np.clip(1 + .34 * hard + .22 * noise(rng, n2, 8), .45, 1.9)
    rest = np.tan(np.radians(REST_DEG[0] + (REST_DEG[1] - REST_DEG[0]) * smooth(-1.1, 1.1, hard + .55 * noise(rng, n2, 3, 2049)))).astype(np.float32)
    start = np.where(mask, 60 * np.maximum(plan, 0) + .5 * c2 * ndi.distance_transform_edt(mask).astype(np.float32), 0).astype(np.float32)
    h, acc, rec = drained_land(mask, start, uk.astype(np.float32), rest, c2, SPL_STEPS, table(D2, CAP))
    # fans at the mouths
    warp = noise(rng, n2, 9)
    fan, mouths = spread_fans(mask, rec, acc, c2, FAN_MIN_M2, FAN_RADIUS, FAN_SLOPE, np.clip(warp, -1.6, 1.6))
    # (no piece of a fan cut off from its hill by the warp)
    labels, count = ndi.label((fan > 0) | mask)
    has_hill = np.zeros(count + 1, bool)
    has_hill[np.unique(labels[mask])] = True
    fan = fan * has_hill[labels]
    log('fans from', mouths, 'valleys; tallest', float(fan.max()))
    return h, fan, mask, acc, hard


def gullied(A, D, rng):
    """Every slope's own rills, on the full grid: the water of the land as it stands, cutting by how much of it there is, three times.
    What the half grid's network leaves is planes, and planes, however sharp their crests, read as a loaf."""
    n = A.shape[0]
    active = (A > 2.5) & (D < GULLY_REACH)
    seed = ndi.binary_dilation(active, iterations=1) & ~active
    active = active | seed
    fine = gauss(rng.standard_normal(A.shape).astype(np.float32), 1.3)
    fine = fine / fine.std()
    depth = np.minimum(GULLY_DEPTH * smooth(6, 70, A), .17 * A).astype(np.float32)
    out = A.copy()
    for it in range(3):
        z = out + (1.1 - .3 * it) * fine
        rec, order = route(z, active, seed)
        acc = accumulate(rec, order, A.shape, CELL).reshape(A.shape)
        cut = depth * (np.minimum(acc / GULLY_FULL_M2, 1) ** .5 - np.minimum(GULLY_HEAD_M2 / np.maximum(acc, 1), 1) ** 2 * .35)
        out = out - np.where(active, np.maximum(cut, 0), 0) / 3 * (1.5 - .25 * it)
        out = np.where(active, .86 * out + .14 * gauss(out, .8), out).astype(np.float32)
        log('  rills', it, 'mean cut', float(cut[active].mean()))
    return np.where(active, out, A).astype(np.float32), acc


def warp_field(a, wx, wz):
    """Sampled off to the side a little: D8 walks on eight directions, and an eye finds them in the crests."""
    n = a.shape[0]
    rows, cols = np.meshgrid(np.arange(n, dtype=np.float32), np.arange(n, dtype=np.float32), indexing='ij')
    return ndi.map_coordinates(a, [rows + wz, cols + wx], order=1, mode='nearest').astype(np.float32)


def shape(dem, off, rj):
    n = CELLS + 1
    n2 = CELLS // 2 + 1
    log('distance from what stays put')
    D_c = distance_field(1025)
    D_f = resample(D_c, n)
    D2 = D_f[::2, ::2]
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
    log('ground')
    G, inside = ground_field(off, rj, n)
    P, U, covered = plain_field(G, off, rj, n)
    # the real land, by the size of the shape: a rill times 2.2 is a vertical stripe, and a knob of the plain a cone
    broad_real = g600 - PLAIN_M
    real = np.where(broad_real > 0, broad_real * FAR_GAIN, broad_real * .3) + (g150 - g600) * RIDGE_GAIN + (g12 - g150) * RILL_GAIN
    real = resample(real, n)
    log('rough')
    rm, rw, rn, rf, rr = (resample(b, n) for b in rough_bands(dem, 2049))
    log('the hills and their water')
    h, fan, mask2, acc, hard = near_hills((rm[::2, ::2], rw[::2, ::2], rn[::2, ::2], rf[::2, ::2], resample(wide_n, n2)), D2, rng)
    # (the same warp for the land and its fans, kept off the ground it must not reach)
    wx = 3.2 * noise(rng, n2, 30) + 1.3 * noise(rng, n2, 6)
    wz = 3.2 * noise(rng, n2, 30) + 1.3 * noise(rng, n2, 6)
    h = warp_field(h, wx, wz)
    fan = warp_field(fan, wx, wz)
    fan *= smooth(NEAREST - 30, NEAREST + 60, D2)
    near = np.maximum(h, fan)
    near *= smooth(NEAREST - 28, NEAREST + 5, D2)
    near = resample(near, n)
    hard_f = resample(hard, n)
    # what the half grid cannot hold: a share of the hill's own height, nothing where it is low
    near += np.minimum(near * .09, 7.0) * smooth(4, 30, near) * (.5 * np.clip(rf, -2.4, 2.8) + .35 * np.clip(rr, -3, 3))
    log('rills')
    near, _ = gullied(near, D_f, rng)
    near = near * (1 - .85 * smooth(FAR_IN[0] + 2000, FAR_IN[1] + 3500, D_f))
    A = near + real * smooth(FAR_IN[0], FAR_IN[1], D_f)
    del near, real, rm, rw, rn, rf, rr
    k = 2
    A2 = talus(np.maximum(A[::k, ::k], -8), CELL * k, TALUS_DEG)
    A = A + resample(A2 - np.maximum(A[::k, ::k], -8), n) * (A > -8)
    del A2
    A = benches(A, n, hard_f)
    # the rim of the ground must never be seen from behind: over it just past the rim, not before
    xs, zs = X0 + np.arange(n) * CELL, Z0 + np.arange(n) * CELL
    ax = np.maximum(GROUND[0] - xs, xs - GROUND[1])[None, :]
    az = np.maximum(GROUND[2] - zs, zs - GROUND[3])[:, None]
    past = np.maximum(ax, az).astype(np.float32)       # > 0 outside the rim, < 0 inside it, in metres
    A = np.maximum(A, (SINK + 3) * smooth(0, 120, past) + 10 * smooth(120, 900, past))
    # the foot comes out of the plain round, not on an edge; no hill at all leaves the plain exactly as it was
    A = np.where(A <= 0, 0, np.where(A < FOOT_ROUND, A * A / (2 * FOOT_ROUND), A - FOOT_ROUND / 2))
    A = np.where(A > 0, gauss(A, .55), 0).astype(np.float32)
    # the water of the finished land, all of it, for the masks
    A2 = A[::2, ::2]
    edge = np.zeros(A2.shape, bool)
    edge[0, :] = edge[-1, :] = edge[:, 0] = edge[:, -1] = True
    rec, order = route(A2 + .05 * noise(rng, n2, 2), np.ones(A2.shape, bool), edge | (A2 <= 0))
    flow = resample(np.log10(np.maximum(accumulate(rec, order, A2.shape, CELL * 2).reshape(A2.shape), 1)).astype(np.float32), n)
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
    rock = smooth(26, 40, deg + 6 * resample(rock_noise, R.shape[0])) * (1 - .35 * gully) * (1 - foot)
    M = np.dstack([veg, rock, ao, foot])
    return N, M, deg


def foot_table(R, G, off, rj):
    """How far the range stands over the road's own ground, near the rim: what stops the player and the truck (S47Relief::HillAbove).
    Row 0 is the smallest z. Bytes (metres - FOOT_BIAS) / FOOT_SCALE, clipped. Only the road's land: SARO's and the station's walks
    have no hill in them, and foot_measures says so each time this runs (the other two layers are for that)."""
    n = R.shape[0]
    margin = 60.0
    q0 = int(np.floor((GROUND[0] - margin - X0) / CELL))
    q1 = int(np.ceil((GROUND[1] + margin - X0) / CELL))
    r0 = int(np.floor((GROUND[2] - margin - Z0) / CELL))
    r1 = int(np.ceil((GROUND[3] + margin - Z0) / CELL))
    xs, zs = X0 + np.arange(q0, q1 + 1) * CELL, Z0 + np.arange(r0, r1 + 1) * CELL
    XX, ZZ = np.meshgrid(xs, zs)
    Rr, Gr = R[r0:r1 + 1, q0:q1 + 1], G[r0:r1 + 1, q0:q1 + 1]
    off_here = off_at(off, rj, XX, ZZ)
    # (US47World::MakeLands: SARO walks the road's own land on its piece, and its ring lies sunk under that)
    sx, sz = XX - MG.SARO_IN_ROAD[0], ZZ - MG.SARO_IN_ROAD[1]
    on_piece = ((sx >= -146) & (sx <= 98) & (sz >= 98) & (sz <= 902)) | ((XX >= -120) & (XX <= 120) & (ZZ >= -200) & (ZZ <= 600))
    saro = Rr - np.where(on_piece, Gr, B.saro_land(sx, sz).astype(np.float32) + off_here)
    y_st, _ = station_land(XX, ZZ)
    layers = np.stack([Rr - Gr, saro, Rr - y_st])
    data = (np.clip((layers[0] - FOOT_BIAS) / FOOT_SCALE + .5, 0, 255)).astype(np.uint8)
    meta = {'origin_x': float(xs[0]), 'origin_z': float(zs[0]), 'cell_m': float(CELL), 'width': int(len(xs)), 'height': int(len(zs)),
            'scale_m': FOOT_SCALE, 'bias_m': FOOT_BIAS, 'sha256': hashlib.sha256(data.tobytes()).hexdigest()}
    return data, meta, layers


def foot_measures(layers, meta, D):
    """Is the foot where it may be? Nothing of what stays put under it, and how much of each area's walk is left."""
    r0 = int(round((meta['origin_z'] - Z0) / CELL))
    q0 = int(round((meta['origin_x'] - X0) / CELL))
    d = D[r0:r0 + meta['height'], q0:q0 + meta['width']]
    xs = meta['origin_x'] + np.arange(meta['width']) * CELL
    zs = meta['origin_z'] + np.arange(meta['height']) * CELL
    XX, ZZ = np.meshgrid(xs, zs)
    # what each area walks (US47World::MakeLands)
    sx, sz = XX - MG.SARO_IN_ROAD[0], ZZ - MG.SARO_IN_ROAD[1]
    truck = (-19.25, 31.4)
    tx, tz = truck[0] - (ZZ - B.END[1]), truck[1] + (XX - B.END[0])
    walks = {'road': (XX >= -2950) & (XX <= 2250) & (ZZ >= 450) & (ZZ <= 4350) & ~((XX < -560) & (ZZ < 730)),
             'saro': (sx * sx + sz * sz <= 600 ** 2) | ((sx >= -296) & (sx <= 276) & (sz >= -521) & (sz <= 0)),
             'station': tx * tx + tz * tz <= 600 ** 2}
    out = {}
    for name, layer in zip(('road', 'saro', 'station'), layers):
        walk = walks[name]
        stop = (layer > FOOT_STOP_M) & walk
        out[name] = {'share_of_its_walk_stopped': float(stop.sum() / walk.sum()), 'nearest_stopped_to_what_stays_put_m': float(d[stop].min()) if stop.any() else None}
        if name == 'saro' and stop.any():
            out[name]['nearest_stopped_to_the_site_m'] = float(np.hypot(sx, sz)[stop].min())
        if name == 'station' and stop.any():
            out[name]['nearest_stopped_to_the_station_m'] = float(np.hypot(tx, tz)[stop].min())
    return out


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
    rows = tall[z_in]
    w_row = np.where(rows & (x_of[z_in] < 0), west[z_in], 1e9).min(axis=1)
    e_row = np.where(rows & (x_of[z_in] > 0), east[z_in], 1e9).min(axis=1)
    out['rows_with_foot_280_540m_from_highway'] = {'west': float(((w_row >= 280) & (w_row <= 540)).mean()), 'east': float(((e_row >= 280) & (e_row <= 540)).mean()),
                                                   'west_under_1000': float((w_row < 1000).mean()), 'east_under_1000': float((e_row < 1000).mean())}
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
    report = {'vertical_factor_far': [FAR_GAIN, RIDGE_GAIN, RILL_GAIN], 'plain_m': PLAIN_M, 'sink_m': SINK, 'share': SHARE, 'nearest': NEAREST, 'cap': CAP, 'uk': UK, 'spl': [SPL_M, SPL_STEPS, REST_DEG], 'fan': [FAN_MIN_M2, FAN_RADIUS, FAN_SLOPE], 'rough': [ROUGH_ROWS, ROUGH_COLS], 'bench': [BENCH_M, BENCH_NEAR], 'far_in': FAR_IN,
               'cell_m': CELL, 'extent_m': EXTENT, 'x0': X0, 'z0': Z0, 'ground_rim': GROUND, 'relief_sha256': rj.get('sha256'), 'haze': HAZE_NOTE}
    report.update(measures(R, G, inside, D, U, A))
    log(json.dumps({k: report[k] for k in ('range_above_ground_nearest_to_masks_m', 'plain_share_of_the_map', 'plain_R_minus_lowest_ground_m', 'R_over_road_ground_share', 'foot_more_than_3m_up_from_highway_m', 'rows_with_foot_280_540m_from_highway', 'range_height_m', 'slope_deg_percentiles_above_ground')}))
    log('masks')
    data, meta, layers = foot_table(R, G, off, rj)
    report['foot'] = foot_measures(layers, meta, D)
    log(json.dumps(report['foot']))
    if report['foot']['saro']['share_of_its_walk_stopped'] or report['foot']['station']['share_of_its_walk_stopped']:
        log('NOTE: hills stand in SARO\'s or the station\'s walk, and only the road\'s land stops the player: give range_foot.bin their layers')
    del layers
    N, M, deg = masks(R, G, inside, D, flow)
    np.save(cache / 'range_M.npy', (np.clip(M[::2, ::2], 0, 1) * 255 + .5).astype(np.uint8))
    if not args.preview_only:
        data.tofile(root / 'range_foot.bin')
        (root / 'range_foot.json').write_text(json.dumps(meta) + '\n')
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
