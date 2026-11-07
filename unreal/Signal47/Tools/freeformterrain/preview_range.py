#!/usr/bin/env python3
"""Eye-level pictures of SM_FF_Range on the ground, no engine: a height-field ray cast with Numba, for the land's shape.

The light and the colours are this script's, near what M_S47_FreeformRange does. Judge the shape and the haze here,
the material in the game.    python3 preview_range.py <freeformterrain> <out dir> [name x z yaw ...]

Also writes plan.png: the land from above, hills shaded, what stops the player in red, 8 x 9 km round the road.
"""
import json
import sys
from pathlib import Path

from numba import njit
import numpy as np
from PIL import Image

CELL = 24000 / 4096
X0, Z0 = -1100 - 12000., -200 - 12000.
VIEWS = [('array', -320, -500, 92), ('farm', 25, 300, 110), ('road_w', -40, 900, 267), ('road_e', 40, 1100, 90), ('road_n', 0, -1500, 340),
         ('town', -40, 1640, 175), ('road_s', -14, 700, 178), ('station', -560, 560, 260), ('road_e2', 14, 2600, 70), ('road_w2', -14, 3200, 290)]
SUN = np.array([.55, .55, .62])
SUN /= np.linalg.norm(SUN)
HAZE = np.array([.40, .48, .62])
SKY = np.array([.52, .66, .86])
SOIL = np.array([.42, .27, .18], np.float32)
ROCK = np.array([.25, .20, .18], np.float32)


@njit(cache=True)
def cast(H, is_range, light, M, ex, ez, yaw, w, hh, fov, X0, Z0, CELL, soil, rock, haze, sky):
    ang = np.radians(yaw)
    fwd = (np.sin(ang), -np.cos(ang))
    right = (np.cos(ang), np.sin(ang))
    tanf = np.tan(np.radians(fov) / 2)
    focal = w * .5 / tanf
    horizon = hh * .62
    n = H.shape[0]
    img = np.zeros((hh, w, 3), np.float32)
    for r in range(hh):
        t = r / hh
        for ch in range(3):
            img[r, :, ch] = sky[ch] + ((.78, .82, .88)[ch] - sky[ch]) * min(t / .62, 1.0) ** 3
    eye = H[int((ez - Z0) / CELL), int((ex - X0) / CELL)] + 1.7
    for c in range(w):
        col = (c + .5) / w * 2 - 1
        dx, dz = fwd[0] + col * tanf * right[0], fwd[1] + col * tanf * right[1]
        norm = (dx * dx + dz * dz) ** .5
        top = hh
        d = 6.0
        while d < 26000 and top > 0:
            px, pz = ex + d * dx, ez + d * dz
            fq, fr = (px - X0) / CELL, (pz - Z0) / CELL
            if fq < 0 or fr < 0 or fq >= n - 1 or fr >= n - 1:
                break
            q, rr = int(fq), int(fr)
            u, v = fq - q, fr - rr
            y = (H[rr, q] * (1 - u) + H[rr, q + 1] * u) * (1 - v) + (H[rr + 1, q] * (1 - u) + H[rr + 1, q + 1] * u) * v
            sy = horizon - (y - eye) / d * focal
            if sy < top:
                a = max(int(sy), 0)
                mr, mc = min(rr // 2, M.shape[0] - 1), min(q // 2, M.shape[1] - 1)
                if is_range[rr, q]:
                    k = M[mr, mc, 1]
                    base = [(soil[i] + (rock[i] - soil[i]) * k) * (1 - .55 * M[mr, mc, 0]) * (.55 + .45 * M[mr, mc, 2]) for i in range(3)]
                else:
                    base = [soil[i] * .95 for i in range(3)]
                li = (light[rr, q] * (1 - u) + light[rr, q + 1] * u) * (1 - v) + (light[rr + 1, q] * (1 - u) + light[rr + 1, q + 1] * u) * v
                hz = 1 - np.exp(-max(d - 900, 0) / 3800) * 1.0
                hz *= .88
                for i in range(3):
                    lit = base[i] * (.10 + 1.05 * li)
                    img[a:top, c, i] = lit + (haze[i] * .95 - lit) * hz
                top = a
            d += max(1.5, d * .0035)
    return img


def main():
    root, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    R = np.load(root / 'cache/range_R.npy')
    G = np.load(root / 'cache/range_G.npy')
    M = np.load(root / 'cache/range_M.npy').astype(np.float32) / 255
    H = np.maximum(R, G)
    is_range = R > G
    gz, gx = np.gradient(H, CELL)
    length = np.sqrt(gx * gx + gz * gz + 1)
    # the sun, with no shadows (the range casts none), and the sky round it from every side but below
    light = (np.clip((-gx * SUN[0] - gz * SUN[2] + SUN[1]) / length, 0, 1) * .85 + .20 * (1 + (-gx * -.5 - gz * -.3 + 1) / length)).astype(np.float32)
    args = sys.argv[3:]
    views = [(args[i], float(args[i + 1]), float(args[i + 2]), float(args[i + 3])) for i in range(0, len(args), 4)] or VIEWS
    for name, ex, ez, yaw in views:
        img = cast(H, is_range, light, M, ex, ez, yaw, 1600, 620, 75.0, X0, Z0, CELL, SOIL, ROCK, HAZE.astype(np.float32), SKY.astype(np.float32))
        Image.fromarray((np.clip(img, 0, 1) ** (1 / 2.2) * 255).astype(np.uint8)).save(out / (name + '.png'))
    # the plan
    x0, x1, z0, z1 = -3400, 2700, -3000, 4700
    q0, q1, r0, r1 = int((x0 - X0) / CELL), int((x1 - X0) / CELL), int((z0 - Z0) / CELL), int((z1 - Z0) / CELL)
    lit = light[r0:r1, q0:q1]
    above = (R - G)[r0:r1, q0:q1]
    rgb = np.dstack([lit * .9 + .1] * 3)
    rgb[above > .45] *= np.array([1.0, .78, .62])
    rgb[above <= .45] = rgb[above <= .45] * .5 + np.array([.42, .36, .3])
    road = np.abs(np.arange(q0, q1) * CELL + X0) < 12
    rgb[:, road] = (.1, .1, .1)
    Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8)).save(out / 'plan.png')
    print(json.dumps({'views': [v[0] for v in views]}))


if __name__ == '__main__':
    main()
