#!/usr/bin/env python3
"""The Freeform ground's detail textures, from Poly Haven scans (CC0) and generated noise. Writes textures/out/*.png.

  uv run --no-project --with numpy --with pillow --with scipy python make_ground_textures.py [root]

T_FF_Stones_D  RGB: a stone's colour as a ratio to the soil under it, /RATIO (ring round it: its contact shade). A: the stone's rank, 0 off any stone.
T_FF_Stones_N  RG: normal xy (DirectX). B: the soil's own detail, stones painted out, /RATIO. A: height, soil and stones.
T_FF_Patch_D   RGB: 20 m of dry ground as a ratio to its own blur, /RATIO.
T_FF_Gravel_D  RGB: dense mixed gravel as a ratio to its own mean, /RATIO. A: height. T_FF_Gravel_N its normal.
T_FF_Crust_D   RGB: dry cracked clay as a ratio to its mean, /RATIO. A: height. T_FF_Crust_N its normal.
T_FF_Grit      RG: normal xy of fine sandy gravel. B: its brightness as a ratio, /RATIO. A: height. The grain under everything.
T_FF_Noise     tileable smooth fields, all mean .5. R features 40-200 m at 1100 m a tile, G the same with another seed,
               B a thresholded field for gravel washes, A a smooth index for the stochastic tiling.
T_FF_Field     road-local metres, 4096 m a side, 1 m a texel. What the flora (S47Flora.cpp) has decided, so soil and plants agree: R Stand, G StoneField,
               B the soil darker under a creosote and against a farm building, A Meadow. Same integers as FS47Flora, ported below.
"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage as ni

Image.MAX_IMAGE_PIXELS = None
ROOT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
SRC, OUT = ROOT / 'textures/src', ROOT / 'textures/out'
RATIO = 2.5
SIZE = 2048


def load(name, size=SIZE, mode='RGB'):
    im = Image.open(SRC / name).convert(mode)
    if im.size[0] != size:
        im = im.resize((size, size), Image.LANCZOS)
    return np.asarray(im, np.float32) / 255


def linear(c):
    return np.where(c <= .04045, c / 12.92, ((c + .055) / 1.055) ** 2.4)


def blur(a, sigma):
    return ni.gaussian_filter(a, sigma if a.ndim == 2 else (sigma, sigma, 0), mode='wrap')


def save(name, rgba):
    Image.fromarray((np.clip(rgba, 0, 1) * 255 + .5).astype(np.uint8), 'RGBA' if rgba.shape[2] == 4 else 'RGB').save(OUT / (name + '.png'))


def stones():
    d = linear(load('dry_ground_rocks_diff_4k.jpg'))
    h = load('dry_ground_rocks_disp_4k.jpg', mode='L')
    n = load('dry_ground_rocks_nor_dx_4k.jpg')
    lum = d @ np.array([.2126, .7152, .0722], np.float32)
    # 1 px = 2 mm. The soil beside a stone: a blur wide enough to pass over it
    soil = blur(lum, 60)
    hp = h - blur(h, 30)
    pale = lum / np.maximum(soil, 1e-3)
    # stones stand up and are pale or grey; cracks go down, so never by brightness alone
    seed = (hp > np.percentile(hp, 96.5)) & (pale > 1.15)
    grown = (hp > np.percentile(hp, 91)) & (pale > .95)
    grown = ni.binary_opening(grown, iterations=2)
    label, count = ni.label(grown)
    keep = np.zeros(count + 1, bool)
    keep[np.unique(label[seed])] = True
    keep[0] = False
    area = ni.sum_labels(np.ones_like(label), label, np.arange(count + 1))
    keep &= (area >= 40) & (area < 26000)
    mask = keep[label]
    mask = ni.binary_closing(mask, iterations=2)
    mask = ni.binary_fill_holes(mask)
    label, count = ni.label(mask)
    rng = np.random.default_rng(47)
    # wrap: a stone across the edge is one stone
    for a, b in ((label[0], label[-1]), (label[:, 0], label[:, -1])):
        for x, y in zip(a, b):
            if x and y and x != y:
                label[label == y] = x
    ids = np.unique(label)
    ids = ids[ids > 0]
    rank = np.zeros(label.max() + 1, np.float32)
    rank[ids] = rng.permutation(len(ids)) / max(1, len(ids) - 1) * .9 + .1
    # a mixed desert gravel: pale limestone, tan and rust, grey, dark basalt; one colour to a stone, independent of its rank
    kinds = np.array([[1.0, .98, .95], [.72, .62, .52], [.62, .60, .60], [.34, .33, .34], [.70, .50, .38]], np.float32)
    kind = rng.choice(len(kinds), label.max() + 1, p=[.26, .26, .22, .16, .10])
    tone = kinds[kind] * rng.uniform(.85, 1.1, (label.max() + 1, 1)).astype(np.float32)
    # nearest stone and how far: its contact ring reaches 3 cm (15 px)
    dist, (iy, ix) = ni.distance_transform_edt(~np.pad(mask, 64, mode='wrap'), return_indices=True)
    dist = dist[64:-64, 64:-64]
    iy, ix = (iy[64:-64, 64:-64] - 64) % SIZE, (ix[64:-64, 64:-64] - 64) % SIZE
    near = label[iy, ix]
    ring = np.clip(1 - dist / 15, 0, 1)
    a = rank[near] * (dist <= 15)
    # the soil with the stones painted out: filled from the nearest soil outside them
    fat = ni.binary_dilation(mask, iterations=5)
    _, (jy, jx) = ni.distance_transform_edt(np.pad(fat, 64, mode='wrap'), return_indices=True)
    jy, jx = (jy[64:-64, 64:-64] - 64) % SIZE, (jx[64:-64, 64:-64] - 64) % SIZE
    filled = lum.copy()
    filled[fat] = lum[jy, jx][fat]
    filled = np.where(fat, blur(filled, 2.5), filled)
    wide = blur(filled, 90)
    soil_ratio = np.clip(filled / np.maximum(wide, 1e-3), .4, 1.6)
    soil_ratio = soil_ratio / soil_ratio.mean()
    # a stone's colour against the soil at that place; outside it, the shade of its ring
    colour = d / np.maximum(blur(np.where(fat[..., None], np.broadcast_to(filled[..., None], d.shape), d), 60), 1e-3)
    grey = (colour @ np.array([.3, .5, .2], np.float32))[..., None]
    colour = grey + (colour - grey) * .55            # the scan's yellow is its own soil's; a stone is greyer
    colour = colour * tone[label]
    shade = 1 - .42 * ring[..., None] ** 1.6
    ratio = np.where(mask[..., None], colour, shade)
    stone_d = np.dstack([ratio / RATIO, a])
    print('stones', len(ids), 'cover %.3f' % mask.mean(), 'median ratio %.2f' % np.median(ratio[mask]))
    save('T_FF_Stones_D', stone_d)
    # height: the soil keeps a small part, stones stand from it
    height = np.clip(.5 + .75 * (h - np.median(h)), 0, 1)
    save('T_FF_Stones_N', np.dstack([n[..., 0], n[..., 1], soil_ratio / RATIO, height]))
    Image.fromarray((np.clip(mask * 1., 0, 1) * 255).astype(np.uint8)).save(OUT / 'debug_stones_mask.png')


def patch():
    d = linear(load('aerial_ground_rock_diff_2k.jpg'))
    wide = blur(d, 320)         # 1 px = 1 cm; a blur of 3 m: what is left is patches and everything finer
    r = d / np.maximum(wide, 1e-3)
    grey = (r @ np.array([.3, .5, .2], np.float32))[..., None]
    r = grey + (r - grey) * .6
    r = r / r.reshape(-1, 3).mean(0)
    print('patch p1 p50 p99', np.percentile(r, [1, 50, 99]))
    save('T_FF_Patch_D', r / RATIO)


def scan_layer(source, name, sigma, chroma):
    """A scan as a ratio of mean 1 to its own blur, height in alpha, and its normal copied as it is (DirectX)."""
    d = linear(load(source + '_diff_2k.jpg'))
    h = load(source + '_disp_2k.jpg', mode='L')
    r = d / np.maximum(blur(d, sigma), 1e-3)
    grey = (r @ np.array([.3, .5, .2], np.float32))[..., None]
    r = grey + (r - grey) * chroma
    r = np.maximum(r, .22)                 # a crack is dark, not black: the light gets in
    r = r / r.reshape(-1, 3).mean(0)
    print(name, 'ratio p1 p50 p99', np.percentile(r, [1, 50, 99]))
    save(name + '_D', np.dstack([r / RATIO, np.clip(.5 + (h - np.median(h)), 0, 1)]))
    save(name + '_N', load(source + '_nor_dx_2k.jpg'))


def grit():
    d = linear(load('sandy_gravel_02_diff_2k.jpg'))
    h = load('sandy_gravel_02_disp_2k.jpg', mode='L')
    n = load('sandy_gravel_02_nor_dx_2k.jpg')
    lum = d @ np.array([.2126, .7152, .0722], np.float32)
    r = lum / np.maximum(blur(lum, 100), 1e-3)
    r = r / r.mean()
    print('grit ratio p1 p50 p99', np.percentile(r, [1, 50, 99]))
    save('T_FF_Grit', np.dstack([n[..., 0], n[..., 1], r / RATIO, np.clip(.5 + (h - np.median(h)), 0, 1)]))


def field(size, low, high, seed):
    """Tileable noise with features between `low` and `high` texels across, by the spectrum, ranked to uniform."""
    rng = np.random.default_rng(seed)
    f = np.fft.fftfreq(size)
    fr = np.hypot(*np.meshgrid(f, f, indexing='ij'))
    a = np.where(fr > 0, 1 / np.maximum(fr, 1e-6), 0.)
    keep = np.exp(-(1 / (fr * high + 1e-6)) ** 2) * np.exp(-(fr * low) ** 2)
    x = np.fft.ifft2(np.fft.fft2(rng.standard_normal((size, size))) * a * keep).real
    x = (x - x.mean()) / x.std()
    # a soft contrast curve, not hard: mean .5, most of it .2-.8
    return (.5 + .5 * np.tanh(x * .9)).astype(np.float32)


def noise():
    size = 1024
    m = 1100 / size
    r = field(size, 40 / m, 240 / m, 11)
    g = field(size, 25 / m, 150 / m, 12)
    b = np.clip(.65 * field(size, 18 / m, 120 / m, 13) + .35 * field(size, 6 / m, 20 / m, 14), 0, 1)
    a = field(size, 10, 90, 15)
    print('noise means', r.mean(), g.mean(), b.mean(), a.mean())
    save('T_FF_Noise', np.dstack([r, g, b, a]))


# ---------- FS47Flora's integers (S47Flora.cpp), as they are ----------
M32 = 0xFFFFFFFF


def u32(a):
    return np.asarray(a, np.int64).astype(np.uint64) & M32


def hash4(a, b, c, d):
    h = (2166136261 ^ (u32(a) * 0x27d4eb2d & M32)) & M32
    h = ((h ^ u32(b)) * 16777619) & M32
    h = ((h ^ u32(c)) * 0x165667b1) & M32
    h = ((h ^ u32(d)) * 0x9e3779b1) & M32
    h ^= h >> np.uint64(15)
    h = (h * 0x85ebca6b) & M32
    h ^= h >> np.uint64(13)
    return h


class Mulberry:
    def __init__(self, seed):
        self.s = u32(seed)

    def __call__(self):
        self.s = (self.s + 0x6d2b79f5) & M32
        s = self.s
        t = ((s ^ (s >> np.uint64(15))) * (1 | s)) & M32
        t = ((t + ((t ^ (t >> np.uint64(7))) * (61 | t) & M32)) & M32) ^ t
        return (t ^ (t >> np.uint64(14))).astype(np.float64) / 4294967296.0


def vnoise(x, z, size, seed):
    u, v = x / size, z / size
    i, j = np.floor(u), np.floor(v)
    fu, fv = u - i, v - j
    h = lambda a, b: hash4(a.astype(np.int64), b.astype(np.int64), seed, 77).astype(np.float64) / 4294967296.0
    su, sv = fu * fu * (3 - 2 * fu), fv * fv * (3 - 2 * fv)
    ha, hb, hc, hd = h(i, j), h(i + 1, j), h(i, j + 1), h(i + 1, j + 1)
    return ha + (hb - ha) * su + (hc - ha) * sv + (ha - hb - hc + hd) * su * sv


def sstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


SEED = 1986
def stand(x, z):
    return sstep(.51, .62, .68 * vnoise(x, z, 90, SEED + 31) + .32 * vnoise(x, z, 26, SEED + 37))


def meadow(x, z):
    return sstep(.44, .56, .62 * vnoise(x, z, 64, SEED + 917) + .38 * vnoise(x, z, 19, SEED + 919))


def wash(x, z):
    """S47Flora CreosoteWash: long lines down a 30 degree fall, a noise stretched four times along it."""
    u, v = x * .8660 + z * .5, -x * .5 + z * .8660
    wu = u / 4 + 12 * (vnoise(x, z, 60, SEED + 75) - .5)
    wv = v + 30 * (vnoise(x, z, 60, SEED + 77) - .5)
    return sstep(.86, .97, 1 - np.abs(2 * vnoise(wu, wv, 70, SEED + 73) - 1))


def stone_field(x, z):
    return sstep(.5, .62, .6 * vnoise(x, z, 34, SEED + 41) + .4 * vnoise(x, z, 7, SEED + 43))


def creosote():
    """Every creosote candidate that the stands keep: (x, z, size). The cover of ways and plots is not ported: a spot with no bush is only soil."""
    cell, ki = 8.0, 2
    ii, jj = np.meshgrid(np.arange(-256, 256, dtype=np.int64), np.arange(-256, 256, dtype=np.int64), indexing='ij')
    ii, jj = ii.ravel(), jj.ravel()
    r = Mulberry(hash4(ii, jj, ki, SEED))
    xs, zs, sizes = [], [], []
    for t in range(9):
        u, v, a, b = r(), r(), r(), r()
        for _ in range(9):
            r()                     # CA CB Ry L1 L2 Rank N1 N2 N3: the same count for each
        x0 = (ii + ((t % 3) + .5 + 1.25 * (u - .5)) / 3) * cell
        z0 = (jj + ((t // 3) + .5 + 1.25 * (v - .5)) / 3) * cell
        x = x0 + 5 * (vnoise(x0, z0, 17, SEED + 61) - .5)
        z = z0 + 5 * (vnoise(x0, z0, 17, SEED + 67) - .5)
        st = stand(x, z)
        fld = st * (.5 + .5 * vnoise(x, z, 11, SEED + 39))
        # (8 Nov: spread over all of it, looser stands, lines along the washes; S47Flora NoiseAt and CreosoteWash)
        spread = .06 * (.6 + .8 * vnoise(x, z, 45, SEED + 71))
        keep = a < spread + .55 * fld + .55 * wash(x, z)
        xs.append(x[keep]); zs.append(z[keep]); sizes.append(.5 + b[keep] ** 2.4 * .8)
    return np.concatenate(xs), np.concatenate(zs), np.concatenate(sizes)


def rect_gap(rect, x, z):
    return np.hypot(np.maximum(np.maximum(rect[0] - x, x - rect[1]), 0), np.maximum(np.maximum(rect[2] - z, z - rect[3]), 0))


def field_map():
    size, half = 4096, 2048
    c = np.arange(size, dtype=np.float64) + .5 - half
    x, z = np.meshgrid(c, c, indexing='ij')       # first index road x, second road z
    r_st = stand(x, z).astype(np.float32)
    g_st = stone_field(x, z).astype(np.float32)
    a_me = meadow(x, z).astype(np.float32)
    bx, bz, bs = creosote()
    print('creosote spots', len(bx), 'share of ground in a stand %.3f' % r_st.mean())
    spots = np.zeros((size, size), np.float32)
    for low, high, sigma in ((.5, .75, 1.0), (.75, 1.05, 1.5), (1.05, 1.4, 2.0)):
        pick = (bs >= low) & (bs < high)
        one = np.zeros((size, size), np.float32)
        i, j = np.floor(bx[pick] + half).astype(np.int64), np.floor(bz[pick] + half).astype(np.int64)
        ok = (i >= 0) & (i < size) & (j >= 0) & (j < size)
        np.add.at(one, (i[ok], j[ok]), 1)
        one = ni.gaussian_filter(one, sigma) * (2 * np.pi * sigma * sigma)   # peak 1 for one bush
        spots = np.maximum(spots, one)
    # against the farm's buildings (S47GroundCover FARM_PLOTS, their plots with a margin of 1 m) and the motel: 3 m of shaded, trodden ground
    contact = np.zeros((size, size), np.float32)
    for rect in ((127, 145, 163.5, 176.5), (101, 115, 185, 195), (111, 122, 166.25, 173.75), (81, 88, 203.5, 209.5), (91, 105, 213, 226)):
        contact = np.maximum(contact, (1 - sstep(0, 3.5, rect_gap(rect, x, z))).astype(np.float32))
    b = np.maximum(np.clip(spots, 0, 1), contact)
    # gone by the map's edge over 200 m, so no straight line across the land past it; the rim itself 0, since the sampler wraps
    edge = sstep(1, 200, np.minimum(np.minimum(x + half, half - x), np.minimum(z + half, half - z))).astype(np.float32)
    r_st, g_st, b, a_me = r_st * edge, g_st * edge, b * edge, a_me * edge
    # row = road z, column = road x
    save('T_FF_Field', np.dstack([r_st, g_st, b, a_me]).transpose(1, 0, 2))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    what = sys.argv[2] if len(sys.argv) > 2 else 'all'
    if what in ('all', 'stones'):
        stones()
    if what in ('all', 'patch'):
        patch()
    if what in ('all', 'scans'):
        scan_layer('rocky_trail', 'T_FF_Gravel', 400, .9)
        scan_layer('dry_ground_01', 'T_FF_Crust', 200, .5)
        grit()
    if what in ('all', 'noise'):
        noise()
    if what in ('all', 'field'):
        field_map()


main()
