#!/usr/bin/env python3
"""Eye-level pictures of the baked relief without an engine: a height-field ray cast in NumPy.

Only for judging the land's shape against photographs; the material and light are not the game's.
    python3 preview_relief.py <freeformterrain> [name x z yaw_deg ...]
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import base_port as B

VIEWS = [
    ('vei_z1100_vest', 2.0, 1100, 270),
    ('vei_z1100_ost', -2.0, 1100, 90),
    ('gard_nordvest', 110, 290, 310),
    ('gard_ost', 180, 210, 95),
    ('vei_z2600_sorvest', -6, 2620, 215),
    ('vei_z-1500_nordost', 4, -1500, 40),
    ('beite_vest', -300, 730, 250),
    ('by_nordost', 62, 1950, 40),
]


def bilinear(a, c, r):
    h, w = a.shape
    c = np.clip(c, 0, w - 1.001)
    r = np.clip(r, 0, h - 1.001)
    c0 = c.astype(np.int64)
    r0 = r.astype(np.int64)
    fc = c - c0
    fr = r - r0
    return a[r0, c0] * (1 - fr) * (1 - fc) + a[r0, c0 + 1] * (1 - fr) * fc + a[r0 + 1, c0] * fr * (1 - fc) + a[r0 + 1, c0 + 1] * fr * fc


def view(h, sed, stone, meta, eye_x, eye_z, yaw, eye_up=1.7, w=1500, hh=560, fov=68):
    x0, z0, cell = meta['origin_x'], meta['origin_z'], meta['cell_m']
    ang = np.radians(yaw)
    # yaw from north (-z) clockwise towards +x
    fwd = np.array([np.sin(ang), -np.cos(ang)])
    right = np.array([np.cos(ang), np.sin(ang)])
    tanf = np.tan(np.radians(fov) / 2)
    cols = (np.arange(w) + .5) / w * 2 - 1
    dx = fwd[0] + cols * tanf * right[0]
    dz = fwd[1] + cols * tanf * right[1]
    dist = np.concatenate([np.arange(1.5, 60, .5), np.arange(60, 300, 1.5), np.arange(300, 1200, 4), np.arange(1200, 7000, 10)])
    eye_h = bilinear(h, np.array((eye_x - x0) / cell), np.array((eye_z - z0) / cell)) + eye_up
    px = eye_x + dist[:, None] * dx[None]
    pz = eye_z + dist[:, None] * dz[None]
    c = (px - x0) / cell
    r = (pz - z0) / cell
    hs = bilinear(h, c, r)
    out_of_map = (c < 0) | (r < 0) | (c > h.shape[1] - 1) | (r > h.shape[0] - 1)
    focal = hh * .5 / (tanf * hh / w)
    horizon = hh * .55
    sy = horizon - (hs - eye_h) / dist[:, None] * focal
    sy = np.where(out_of_map, np.inf, sy)
    # shading
    gz, gx = np.gradient(h, cell)
    al = np.radians(19)
    sun_az = np.radians(245)
    lx, lz, ly = np.sin(sun_az) * np.cos(al), -np.cos(sun_az) * np.cos(al), np.sin(al)
    n = np.sqrt(gx * gx + gz * gz + 1)
    shade = np.clip((-gx * lx - gz * lz + ly) / n, 0, 1)
    sh = bilinear(shade, c, r)
    se = bilinear(sed, c, r)
    st = bilinear(stone, c, r)
    rng = np.random.default_rng(3)
    speck = rng.standard_normal(h.shape).astype(np.float32)
    k1 = np.fft.fftfreq(h.shape[0])[:, None] ; k2 = np.fft.rfftfreq(h.shape[1])[None, :]
    sp = np.fft.irfft2(np.fft.rfft2(speck) * np.exp(-2 * (np.pi * 4) ** 2 * (k1 ** 2 + k2 ** 2)), s=h.shape).astype(np.float32)
    sp = np.clip(sp / sp.std(), -2, 2) * .5 + speck * .25
    mott = bilinear(sp, c, r)
    img = np.zeros((hh, w, 3), np.float32)
    t = np.clip(np.arange(hh) / horizon, 0, 1)[:, None]
    sky = np.array([.50, .62, .78])[None] * (1 - t) + np.array([.84, .85, .84])[None] * t
    img[:] = sky[:, None, :]
    base_col = np.array([.376, .223, .141])
    sand = np.array([.55, .42, .28])
    gravel = np.array([.30, .24, .20])
    rock = np.array([.22, .17, .14])
    fog = np.exp(-dist / 7000.0)[:, None]
    for i in range(w):
        y = sy[:, i]
        # cumulative minimum: what stands highest on screen so far hides what is behind and lower
        run = np.minimum.accumulate(y)
        rows = np.arange(hh)
        # for every screen row, the first distance whose top reaches it
        idx = np.searchsorted(-run, -rows, side='left')
        ok = idx < len(run)
        k = np.clip(idx, 0, len(run) - 1)
        s = se[k, i][:, None]
        gv = np.clip((s - .12) / .25, 0, 1) * (1 - np.clip((s - .5) / .3, 0, 1))
        sd = np.clip((s - .55) / .3, 0, 1)
        col = base_col[None] * (1 - gv) + gravel[None] * gv
        col = col * (1 - sd) + sand[None] * sd
        v = st[k, i][:, None]
        col = col * (1 - v) + rock[None] * v
        light = (.22 + 1.8 * sh[k, i])[:, None]
        col = col * light * (1 + .16 * mott[k, i])[:, None]
        fg = fog[k]
        col = col * fg + np.array([.78, .78, .78])[None] * (1 - fg)
        valid = ok & np.isfinite(run[k])
        img[valid, i] = col[valid]
    # sRGB
    img = np.clip(img, 0, 1)
    img = np.where(img < .0031308, 12.92 * img, 1.055 * img ** (1 / 2.4) - .055)
    return (img * 255 + .5).astype(np.uint8)


def main():
    out = Path(sys.argv[1])
    meta = json.loads((out / 'relief.json').read_text())
    H, W = meta['height'], meta['width']
    off = np.fromfile(out / 'relief.bin', dtype='<i2').reshape(H, W).astype(np.float32) * meta['scale_m']
    f = np.load(out / 'cache/fields.npz')
    sed = f['sediment'].astype(np.float32) / 255
    stone = f['stone'].astype(np.float32) / 255
    xs = meta['origin_x'] + np.arange(W) * meta['cell_m']
    zs = meta['origin_z'] + np.arange(H) * meta['cell_m']
    XX, ZZ = np.meshgrid(xs, zs)
    base = B.grid_y(XX, ZZ).astype(np.float32)
    h = base + off
    views = VIEWS
    args = sys.argv[2:]
    if args:
        views = [(args[i], float(args[i + 1]), float(args[i + 2]), float(args[i + 3])) for i in range(0, len(args), 4)]
    dst = out / 'preview'
    tiles = []
    for name, x, z, yaw in views:
        a = view(h, sed, stone, meta, x, z, yaw)
        Image.fromarray(a).save(dst / ('oye_%s.png' % name))
        tiles.append(a)
        print(name, flush=True)
    if len(tiles) >= 2:
        n = len(tiles)
        rows = (n + 1) // 2
        th, tw = tiles[0].shape[:2]
        sheet = np.zeros((rows * th // 2, tw, 3), np.uint8)
        for i, t in enumerate(tiles):
            small = np.asarray(Image.fromarray(t).resize((tw // 2, th // 2)))
            ry, rx = i // 2 * (th // 2), i % 2 * (tw // 2)
            sheet[ry:ry + th // 2, rx:rx + tw // 2] = small
        Image.fromarray(sheet).save(dst / 'oye_ark.png')


if __name__ == '__main__':
    main()
