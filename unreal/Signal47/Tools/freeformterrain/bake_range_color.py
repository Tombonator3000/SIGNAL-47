#!/usr/bin/env python3
"""T_FF_Range_C and T_FF_Range_B: what the hills are made of, for M_S47_FreeformRange. NumPy, SciPy and Pillow; starts neither Blender nor Unreal.

    python3 bake_range_color.py --root <freeformterrain> [--preview]

Reads cache/range_{R,G,D}.npy (build_range.py) and naip/naip_r{0,1}c{0,1}.jpg, USGS NAIP Plus over the map's own 24 km (public domain, KILDER.md).
Out: range/T_FF_Range_C.png. (range/T_FF_Range_B.png was written here until 9 November; make_range_rock.py writes it now.)

T_FF_Range_C, 4096 px over the map, north at the top, linear:
- RGB: the soil as a ratio to the plain's soil, over RATIO, so the material's tint still sets the middle tone by day and by night.
  Beyond FAR_IN the shapes are the map's, so the photograph is the land's own colour there. Nearer the hills are made, not measured, and the photograph is only
  the soil's broad colour: its arroyos would run straight across a made ridge. There the colour follows the hill: ochre and yellow-tan on the dry southern sides, greyer
  and scrubby on the northern ones and down the gullies, paler on the fans.
- A: how much bare rock shows, from the slope, the crests and a patchy field.
T_FF_Range_B, 1024 x 4, wraps along U: the beds as the same ratio, a section SECTION_M thick. The material reads it by a tilted height, so the beds dip and carry on
across a valley, at any distance. Red-brown and grey-brown volcanics with limestone, a pale caliche ledge now and then, and ochre partings. The units are from the
geology of the Organ and Dona Ana mountains (rhyolite, andesite, limestone); their order and thickness are made.
"""
import argparse
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

Image.MAX_IMAGE_PIXELS = None
CELLS = 4096
CELL = 24000.0 / CELLS
RATIO = 2.5
FAR_IN = (2500.0, 7000.0)     # as build_range.py: inside it the hills are made
SECTION_M = 512.0
SEED = 47


def sstep(a, b, v):
    t = np.clip((v - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def to_linear(c):
    return np.where(c <= .04045, c / 12.92, ((c + .055) / 1.055) ** 2.4)


def to_srgb(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= .0031308, c * 12.92, 1.055 * c ** (1 / 2.4) - .055)


def fields(rng, shape, cells, octaves=4, gain=.55):
    """Smooth noise, 0-1: lowest period cells of the shape's side."""
    out = np.zeros(shape, np.float32)
    amp, tot = 1., 0.
    for k in range(octaves):
        side = max(2, int(cells * 2 ** k))
        g = rng.random((side + 1, side + 1), dtype=np.float32)
        out += ndi.zoom(g, shape[0] / side, order=3, grid_mode=False, mode='nearest')[:shape[0], :shape[1]] * amp
        tot += amp
        amp *= gain
    out /= tot
    lo, hi = np.percentile(out[::16, ::16], (1, 99))
    return np.clip((out - lo) / (hi - lo), 0, 1)


def photo(root):
    rows = []
    for r in range(2):
        rows.append(np.concatenate([np.asarray(Image.open(root / 'naip' / ('naip_r%dc%d.jpg' % (r, c))).convert('RGB'), np.float32) / 255 for c in range(2)], 1))
    return to_linear(np.concatenate(rows, 0))


def beds():
    """The section, bottom to top, as (ratio, thickness range in metres, weight)."""
    units = {
        'red':      ((1.06, .71, .54), (6, 26), 4),
        'redbrown': ((.84, .58, .46), (8, 34), 5),
        'greybrown': ((.80, .73, .68), (8, 36), 5),
        'grey':     ((.72, .71, .72), (4, 16), 2),
        'caliche':  ((1.30, 1.24, 1.08), (1.5, 5), 2),
        'ochre':    ((1.20, .98, .63), (2, 9), 2),
        'dark':     ((.52, .47, .46), (2, 10), 2),
    }
    names = list(units)
    weights = np.array([units[n][2] for n in names], float)
    weights /= weights.sum()
    rng = np.random.default_rng(SEED)
    n = 1024
    per_m = n / SECTION_M
    row = np.zeros((n, 3), np.float32)
    at, last = 0, None
    while at < n:
        name = rng.choice(names, p=weights)
        if name == last:
            continue
        last = name
        ratio, (lo, hi), _ = units[name]
        size = max(2, int(rng.uniform(lo, hi) * per_m))
        tone = rng.uniform(.9, 1.1)
        row[at:at + size] = np.array(ratio) * tone
        at += size
    # (soft a little, and a fine grain through each unit; wraps)
    row = ndi.gaussian_filter1d(row, .8, axis=0, mode='wrap')
    grain = ndi.gaussian_filter1d(rng.normal(0, 1, n), 1.2, mode='wrap')
    row *= (1 + .10 * grain / grain.std())[:, None]
    # the section's mean stays near one soil, or every rock would shift the land's tone
    row /= row.mean(0) / np.array((.92, .76, .66))
    return np.tile(row[None, :, :], (4, 1, 1))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--preview', action='store_true')
    args = ap.parse_args()
    root = args.root
    cache = root / 'cache'
    R = np.load(cache / 'range_R.npy')[:CELLS, :CELLS]
    G = np.load(cache / 'range_G.npy')[:CELLS, :CELLS]
    D = ndi.zoom(np.load(cache / 'range_D.npy'), 4, order=1)[:CELLS, :CELLS]
    rng = np.random.default_rng(SEED)
    p = photo(root)
    if p.shape[0] != CELLS:
        p = ndi.zoom(p, (CELLS / p.shape[0], CELLS / p.shape[1], 1), order=1)
    # the plain's soil is 1: the map's lowest quarter that is flat and in the open
    gz, gx = np.gradient(R, CELL)
    deg = np.degrees(np.arctan(np.hypot(gx, gz)))
    plain = (deg < 3) & (R - G < 1)
    if plain.sum() < 1000:
        plain = deg < 3
    soil_mean = p[plain].mean(0)
    print('plain, NAIP sRGB', (to_srgb(soil_mean) * 255).round(1))
    far = p / soil_mean
    # what an aerial photograph has lost to its own haze and levelling
    grey = far.mean(2, keepdims=True)
    far = np.maximum(grey + (far - grey) * 1.35, .05)
    far = 1 + (far - 1) * 1.25
    made = 1 - sstep(FAR_IN[0], FAR_IN[1], D)
    # the made hills
    broad = ndi.gaussian_filter(far, (14, 14, 0))
    r0, h0 = ndi.gaussian_filter(R, 1.5), R - G
    curve = ndi.laplace(r0) / (CELL * CELL)
    gully = sstep(.004, .03, curve)
    crest = sstep(.004, .03, -curve)
    # sun's side: x east, -z north. Facing south is +z
    n_z = -gz / np.sqrt(1 + gx * gx + gz * gz)
    south = sstep(-.35, .35, n_z)
    slope = sstep(4, 22, deg)
    patch_a = fields(rng, R.shape, 24)      # 1 km
    patch_b = fields(rng, R.shape, 96)      # 250 m
    ochre = np.array((1.22, 1.00, .66), np.float32)
    tan = np.array((1.06, .93, .78), np.float32)
    scrub = np.array((.70, .68, .56), np.float32)
    red = np.array((1.06, .76, .58), np.float32)
    hill = (tan + (ochre - tan) * (south * (.45 + .55 * patch_b))[..., None])
    hill = hill + (red - hill) * (sstep(.55, .85, patch_a) * .7)[..., None]
    hill = hill + (scrub - hill) * np.clip(gully * .75 + (1 - south) * .42, 0, 1)[..., None]
    hill = hill * (1 + .10 * crest)[..., None]
    # real colour for them as well: the map's rough north, the same land build_range.py takes their places from, mirrored over the map and softened to patches of rock and soil
    tile = ndi.gaussian_filter(far[:740, :2560], (7, 7, 0))
    rows, cols = np.mgrid[0:CELLS, 0:CELLS]
    wander = (patch_a - .5) * 220
    tr = np.abs(((rows + wander).astype(np.int32) % (2 * 739)) - 739)
    tc = np.abs(((cols - wander).astype(np.int32) % (2 * 2559)) - 2559)
    hill = hill + (tile[tr, tc] - hill) * .6
    del rows, cols, tr, tc, tile
    fine = fields(rng, R.shape, 512, 2)
    hill = hill * (.90 + .20 * fine)[..., None]
    # slopes get the hill's colour; on the fans and the flat its own soil, the map's broad colour
    upness = sstep(1.5, 10., h0) * (.25 + .75 * slope)
    near = broad + (hill - broad) * upness[..., None]
    soil = far + (near - far) * made[..., None]
    # bare rock: slope and crest, in patches; the measured hills' gains make their slopes steep, so their rock is asked for a little higher
    limit = 26 + 6 * (1 - made)
    rock = sstep(limit, limit + 15, deg + (patch_b - .5) * 14 + 8 * crest)
    rock = np.maximum(rock, .55 * crest * sstep(14, 26, deg))
    rock *= sstep(1, 8, h0)
    c = np.clip(np.dstack([soil / RATIO, rock]), 0, 1)
    out = root / 'range'
    out.mkdir(exist_ok=True)
    Image.fromarray((c * 255 + .5).astype(np.uint8), 'RGBA').save(out / 'T_FF_Range_C.png', compress_level=4)
    # (T_FF_Range_B is make_range_rock.py's since 9 November, with bolder beds; beds() here stays for the preview strip)
    print('soil ratio over the hills', np.round(soil[h0 > 10].mean(0), 3), 'rock share over the hills', round(float(rock[h0 > 10].mean()), 3))
    if args.preview:
        pv = root / 'preview'
        pv.mkdir(exist_ok=True)
        # the soil in its own day colour, by sRGB 165,130,105, under a low western sun
        base = to_linear(np.array((165, 130, 105), np.float32) / 255)
        n = np.dstack([-gx, np.ones_like(gx), -gz])
        n /= np.linalg.norm(n, axis=2, keepdims=True)
        sun = np.array((-.7, .5, -.2))
        sun /= np.linalg.norm(sun)
        lit = np.clip((n * sun).sum(2), 0, 1)[..., None] * 1.1 + .35
        Image.fromarray((to_srgb(soil * base * lit)[::2, ::2] * 255).astype(np.uint8)).save(pv / 'range_color.jpg', quality=88)
        Image.fromarray((to_srgb(p)[::2, ::2] * 255).astype(np.uint8)).save(pv / 'range_naip.jpg', quality=88)
        Image.fromarray((np.clip(rock, 0, 1)[::2, ::2] * 255).astype(np.uint8)).save(pv / 'range_rock.jpg', quality=88)
        strip = np.tile(to_srgb(beds()[0] * base)[:, None, :], (1, 64, 1))
        Image.fromarray((strip * 255).astype(np.uint8)).rotate(90, expand=True).save(pv / 'range_beds.png')


if __name__ == '__main__':
    main()
