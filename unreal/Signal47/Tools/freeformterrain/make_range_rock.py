#!/usr/bin/env python3
"""The rock of the hills, for M_S47_FreeformRange (T16-02, Tom 7 November: "fjell: ser ok ut, men må få bedre New Mexico texturer").

    ~/.venvs/signal47/bin/python -I make_range_rock.py --root <freeformterrain>

Pillow only. Reads range/src/ (Poly Haven, CC0, KILDER.md): cliff_side_{diff,nor_gl}_2k.png, a layered red-brown sedimentary
cliff, and dark_rock_02_{diff,nor_gl}_2k.png, a dark fractured rock that stands for the basalt and the old lava.
Out, in range/ beside the other maps, 1024 px (the rock is seen from 50 m and more; 2k would be grain nobody sees):
- T_FF_RangeCliff_D.png, T_FF_RangeBasalt_D.png: the colour, sRGB as the scans have it
- T_FF_RangeRock_N.png: RG the cliff's normal, BA the basalt's, each x and y in DirectX terms (green down), linear
- range_rock.json: each colour's linear mean, so the material reads them as a pattern of mean 1 and the beds give the colour
- T_FF_Range_B.png, the beds (it was bake_range_color.py's): 1024 x 4, wraps along U, a section SECTION_M thick, as a ratio to the
  plain's soil over RATIO. Bolder than the first: the red beds and grey-brown volcanics of the Organ and Dona Ana mountains, with
  yellow sandstone, thick pale limestone ledges and thin dark basalt flows between them, so a slope reads in bands from 2 km. The
  units are those named for the region (rhyolite, andesite, limestone, red beds, basalt); their order and thickness are made.
"""
import argparse
import json
import math
from pathlib import Path
import random

from PIL import Image, ImageStat

SIZE = 1024
SCANS = {'Cliff': 'cliff_side', 'Basalt': 'dark_rock_02'}
RATIO = 2.5                   # as bake_range_color.py
SECTION_M = 512.0             # as bake_range_color.py and import_range.py BED_SECTION_M
SOIL = (.92, .76, .66)        # the section's mean: so the beds shift no slope's tone as a whole
SEED = 47
# ratio to the plain's soil, thickness range in metres, weight
UNITS = {
    'red':       ((1.12, .64, .46), (8, 30), 5),
    'redbrown':  ((.84, .55, .43), (8, 30), 5),
    'greybrown': ((.80, .72, .66), (6, 26), 4),
    'yellow':    ((1.30, 1.05, .66), (3, 12), 3),
    'limestone': ((1.42, 1.36, 1.20), (3, 11), 3),
    'grey':      ((.92, .90, .88), (5, 16), 2),
    'basalt':    ((.40, .37, .36), (2, 7), 2),
}


def beds():
    rng = random.Random(SEED)
    names = list(UNITS)
    n = 1024
    per_m = n / SECTION_M
    row, last = [], None
    while len(row) < n:
        name = rng.choices(names, weights=[UNITS[k][2] for k in names])[0]
        if name == last:
            continue
        last = name
        ratio, (lo, hi), _ = UNITS[name]
        tone = rng.uniform(.9, 1.1)
        row += [tuple(c * tone for c in ratio)] * max(2, int(rng.uniform(lo, hi) * per_m))
    row = row[:n]
    # soft by a texel, wrapping, and a fine grain through each unit
    soft = [tuple((row[i - 1][c] + 2 * row[i][c] + row[(i + 1) % n][c]) / 4 for c in range(3)) for i in range(n)]
    grain = [rng.gauss(0, 1) for _ in range(n)]
    grain = [(grain[i - 1] + grain[i] + grain[(i + 1) % n]) / 3 for i in range(n)]
    sd = math.sqrt(sum(g * g for g in grain) / n)
    out = [tuple(v * (1 + .10 * grain[i] / sd) for v in soft[i]) for i in range(n)]
    mean = [sum(p[c] for p in out) / n for c in range(3)]
    return [tuple(p[c] / mean[c] * SOIL[c] for c in range(3)) for p in out]


def linear_mean(im):
    """The mean of the linear values, from each channel's histogram."""
    lut = [((v / 255 + .055) / 1.055) ** 2.4 if v > 10 else v / 255 / 12.92 for v in range(256)]
    out = []
    for band in im.split():
        h = band.histogram()
        out.append(sum(n * lut[v] for v, n in enumerate(h)) / max(1, sum(h)))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    args = ap.parse_args()
    src, out = args.root / 'range' / 'src', args.root / 'range'
    means, normals = {}, []
    for name, scan in SCANS.items():
        d = Image.open(src / (scan + '_diff_2k.png')).convert('RGB').resize((SIZE, SIZE), Image.LANCZOS)
        d.save(out / ('T_FF_Range%s_D.png' % name))
        means[name] = [round(v, 5) for v in linear_mean(d)]
        n = Image.open(src / (scan + '_nor_gl_2k.png')).convert('RGB').resize((SIZE, SIZE), Image.LANCZOS)
        r, g, _ = n.split()
        normals += [r, g.point(lambda v: 255 - v)]     # OpenGL to DirectX: green down
        print(name, scan, 'linear mean', means[name], 'sRGB mean', [round(v) for v in ImageStat.Stat(d).mean])
    Image.merge('RGBA', normals).save(out / 'T_FF_RangeRock_N.png')
    b = Image.new('RGB', (1024, 4))
    b.putdata([tuple(int(min(1, c / RATIO) * 255 + .5) for c in p) for p in beds()] * 4)
    b.save(out / 'T_FF_Range_B.png')
    (out / 'range_rock.json').write_text(json.dumps({'means_linear': means, 'size': SIZE, 'scans': SCANS}, indent=1) + '\n')


if __name__ == '__main__':
    main()
