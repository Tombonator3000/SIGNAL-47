#!/usr/bin/env python3
"""Eye-level pictures of SM_FF_Range on the ground, no engine: a height-field ray cast in NumPy, for the land's shape.

The light and the colours are this script's, near what M_S47_FreeformRange does. Judge the shape and the haze here,
the material in the game.    python3 preview_range.py <freeformterrain> <out dir> [name x z yaw ...]
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

CELL = 24000 / 4096
X0, Z0 = -1100 - 12000., -200 - 12000.
VIEWS = [('array', 24, -130, 250), ('farm', 150, 300, 235), ('road_w', 2, 1100, 270), ('road_e', -2, 1100, 90), ('road_n', 0, -1500, 340), ('town', 62, 1950, 40), ('road_s', 0, 900, 175)]
SUN = np.array([.55, .55, .62])
SUN /= np.linalg.norm(SUN)
HAZE = np.array([.40, .48, .62])
SKY = np.array([.52, .66, .86])


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
    light = np.clip((-gx * SUN[0] - gz * SUN[2] + SUN[1]) / length, 0, 1)
    deg = np.degrees(np.arctan(np.hypot(gx, gz)))
    args = sys.argv[3:]
    views = [(args[i], float(args[i + 1]), float(args[i + 2]), float(args[i + 3])) for i in range(0, len(args), 4)] or VIEWS
    soil = np.array([.42, .27, .18])
    rock = np.array([.26, .21, .18])
    w, hh = 1600, 620
    fov = 68
    for name, ex, ez, yaw in views:
        ang = np.radians(yaw)
        fwd = np.array([np.sin(ang), -np.cos(ang)])
        right = np.array([np.cos(ang), np.sin(ang)])
        tanf = np.tan(np.radians(fov) / 2)
        cols = (np.arange(w) + .5) / w * 2 - 1
        dx, dz = fwd[0] + cols * tanf * right[0], fwd[1] + cols * tanf * right[1]
        dist = np.concatenate([np.arange(8, 300, 2.), np.arange(300, 1500, 5.), np.arange(1500, 6000, 12.), np.arange(6000, 20000, 25.)])
        eye = H[int((ez - Z0) / CELL), int((ex - X0) / CELL)] + 1.7
        focal = w * .5 / tanf
        horizon = hh * .66
        img = np.zeros((hh, w, 3), np.float32)
        t = (np.arange(hh)[:, None] / hh) ** 1.0
        img[:] = (SKY * (1 - t[..., None] * .0) + (np.array([.78, .82, .88]) - SKY) * np.clip(t[..., None] / .66, 0, 1) ** 3)
        for c in range(w):
            px = ex + dist * dx[c]
            pz = ez + dist * dz[c]
            ok = (px > X0) & (px < X0 + 24000 - 1) & (pz > Z0) & (pz < Z0 + 24000 - 1)
            px, pz, d = px[ok], pz[ok], dist[ok]
            r = ((pz - Z0) / CELL).astype(np.int64)
            q = ((px - X0) / CELL).astype(np.int64)
            sy = horizon - (H[r, q] - eye) / d * focal
            top = hh
            for k in range(len(d)):
                y0 = sy[k]
                if y0 >= top:
                    continue
                a = int(max(y0, 0))
                rr, qq = r[k], q[k]
                mm = M[min(rr // 2, M.shape[0] - 1), min(qq // 2, M.shape[1] - 1)]
                base = soil * (1 + .10 * (rr % 11 == 0))
                if is_range[rr, qq]:
                    base = soil + (rock - soil) * mm[1]
                    base = base * (1 - .55 * mm[0]) * (.55 + .45 * mm[2])
                else:
                    base = soil * .95
                lit = base * (.34 + 1.05 * light[rr, qq])
                h = 1 - np.exp(-max(d[k] - 1200, 0) / 9000)
                col = lit + (HAZE * .95 - lit) * (h * .88)
                img[a:top, c] = col
                top = a
                if top <= 0:
                    break
        srgb = np.clip(img, 0, 1) ** (1 / 2.2)
        Image.fromarray((srgb * 255).astype(np.uint8)).save(out / (name + '.png'))
        print(name, 'done', flush=True)


if __name__ == '__main__':
    main()
