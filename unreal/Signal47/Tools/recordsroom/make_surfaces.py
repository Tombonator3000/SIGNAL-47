#!/usr/bin/env python3
"""RECORDS-ROOM-LIFT-20261113: the records room's surface textures, from Tom's ChatGPT base-colour pictures
(~/scratch-bm/refs/textures-20261112/saro, never in Git, no Cosmos picture among them) and numpy.

  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 make_surfaces.py <out dir> [what ...]
what: block floor ceiling lens wallmacro floormacro cork board   (default: all)

World mapping (the game's material, M_Rec_Block / M_Rec_Floor, projects by world position, SARO metres x, y up, z):
  block   1.6 x 1.6 m a tile, 4 blocks by 8 courses (40 x 20 cm), course joints at y = 0.2 k
  floor   1.2 x 1.2 m a tile, 4 x 4 tiles of 30 cm, joints at x = 0.3 k and z = 0.3 k
  wall macro atlas 2048 x 1024, four cells of 1024 x 512: N (z 6.8) | S (z 10.5) over W (x -6) | E (x -1);
          u = (x + 6) / 5 or (z - 6.8) / 3.7, v = 1 - y / 2.8;  RGB multiplies the block colour x2 (128 neutral), A adds roughness (128 neutral)
  floor macro 2048 x 1536 over x -6..-1, z 6.8..10.5 (row 0 at z 6.8); same channels
Normals are DirectX (green down), as the project's import recipe reads them.
"""
import math, os, sys
from pathlib import Path
import numpy as np
sys.dont_write_bytecode = True
sys.path.insert(0, '/home/tom/SIGNAL-47/unreal/ArtSource')
import photo_tex as PT
from PIL import Image

OUT = Path(sys.argv[1]).resolve()
OUT.mkdir(parents=True, exist_ok=True)
WHAT = set(sys.argv[2:]) or {'block', 'floor', 'ceiling', 'lens', 'wallmacro', 'floormacro', 'cork', 'board'}
SRC = Path(os.path.expanduser('~/scratch-bm/refs/textures-20261112/saro'))
X0, X1, Z0, Z1, H = -6.0, -1.0, 6.8, 10.5, 2.8
LUMA = np.array([.2126, .7152, .0722], np.float32)


def save(name, a):
    a = np.clip(np.asarray(a, np.float32), 0, 1)
    im = Image.fromarray((a * 255 + .5).astype(np.uint8))
    im.save(OUT / name)
    print('wrote', name, im.size, im.mode, flush=True)


def noise(h, w, beta=2.0, seed=0, lo=1.0):
    """Tileable fractal noise, mean 0, std 1: random phases under a 1/f^beta spectrum; lo: lowest frequency kept (cycles)."""
    rng = np.random.default_rng(seed)
    fy = np.fft.fftfreq(h)[:, None] * h
    fx = np.fft.rfftfreq(w)[None, :] * w
    f = np.sqrt(fx * fx + fy * fy)
    amp = np.where(f >= lo, 1.0 / np.maximum(f, 1e-6) ** (beta / 2), 0.0)
    spec = amp * np.exp(2j * math.pi * rng.random(amp.shape))
    a = np.fft.irfft2(spec, s=(h, w))
    return ((a - a.mean()) / (a.std() + 1e-9)).astype(np.float32)


def sstep(a, lo, hi):
    t = np.clip((a - lo) / (hi - lo + 1e-9), 0, 1)
    return t * t * (3 - 2 * t)


def normal_dx(hm, span):
    return PT.normal(hm.astype(np.float32), span, 1.0)


# ------------------------------------------------------------------ painted block wall
def block():
    N = 2048
    a = PT.load(SRC / 'block_int.png')                      # 4 blocks x 11 courses, joints every ~186 px
    lum = a @ LUMA
    row = lum.mean(1)
    base = np.convolve(row, np.ones(61) / 61, 'same')
    d = np.abs(row - base)
    # the picture's course joints: the strongest row near each k * 186.2 (a dark line over a light one: its middle)
    pitch = 2048 / 11.0
    rows = []
    for k in range(12):
        c = int(round(k * pitch))
        lo, hi = max(0, c - 14), min(2047, c + 14)
        seg = d[lo:hi + 1]
        idx = np.argsort(seg)[-2:] + lo
        rows.append(float(idx.mean()))
    rows[0] = min(rows[0], 9.0)
    # eight courses from joint 1 to joint 9
    y0, y1 = int(round(rows[1])), int(round(rows[9]))
    crop = a[y0:y1]
    Hc = crop.shape[0]
    crs = [(r - y0) / Hc for r in rows[1:10]]               # joint rows 0..1 in the crop
    img = PT.resample(crop, N, N)
    lum = img @ LUMA
    jr = [c * N for c in crs]                                # 9 joint rows (first 0, last N)
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    # distance to the nearest course joint (wrapping) and the signed offset from it, and the course each pixel is in
    dist_h = np.full((N, N), 1e9, np.float32)
    off_h = np.zeros((N, N), np.float32)
    for r in jr:
        dy = (yy - r + N / 2) % N - N / 2
        near = np.abs(dy) < dist_h
        off_h = np.where(near, dy, off_h)
        dist_h = np.minimum(dist_h, np.abs(dy))
    course = np.clip(np.searchsorted(np.array(jr[1:-1]), yy[:, 0]), 0, 7)
    # head joints per course: near k * 512 (+256 on every other course), refined on the picture's own vertical edges
    gx = np.abs(np.roll(lum, -2, 1) - np.roll(lum, 2, 1))
    dist_v = np.full((N, N), 1e9, np.float32)
    off_v = np.zeros((N, N), np.float32)
    heads = []
    # which parity has its joints at 0: the course whose band shows the stronger edge response near x = 0 / 512
    def band(k):
        return int(jr[k]) + 16, int(jr[k + 1]) - 16
    def score(k, off):
        b0, b1 = band(k)
        col = gx[b0:b1].mean(0)
        return sum(col[int((off + 512 * i) % N) - 10:int((off + 512 * i) % N) + 11].max() for i in range(1, 4))
    par0 = 0 if score(0, 0) >= score(0, 256) else 1
    for k in range(8):
        off = 0 if (k % 2) == par0 else 256
        b0, b1 = band(k)
        col = gx[b0:b1].mean(0)
        col = np.concatenate([col, col, col])
        cols = []
        for i in range(4):
            c = int(off + 512 * i) + N
            seg = col[c - 22:c + 23]
            x = (c - 22 + int(np.argmax(seg))) if seg.max() > 2.5 * np.median(col) else c
            cols.append((x - N) % N)
        heads.append(cols)
        m = (course == k)[:, None]
        for x in cols:
            dx = (xx - x + N / 2) % N - N / 2
            near = m & (np.abs(dx) < dist_v)
            off_v = np.where(near, dx, off_v)
            dist_v = np.where(m, np.minimum(dist_v, np.abs(dx)), dist_v)
    print('block: course joints', [round(r) for r in jr], 'heads', heads[:2], flush=True)
    dist = np.minimum(dist_h, dist_v)
    # the block each pixel belongs to (course, index along it): a random lean and set-back per block
    rng = np.random.default_rng(47)
    bid = np.zeros((N, N), np.int32)
    for k in range(8):
        cols = sorted(heads[k])
        idx = np.searchsorted(np.array(cols), xx[0]) % 4
        bid[course == k] = (k * 4 + idx[None, :].repeat(N, 0))[course == k]
    setb = rng.normal(0, 0.00035, 32).astype(np.float32)[bid]
    tilt = rng.normal(0, 0.0006, (32, 2)).astype(np.float32)
    # the picture's own joint shading (a dark and a light line) taken out of the colour: filled from the faces beside it
    # (mirrored about a line 15 px from the joint, so the face's own grain runs up to the new groove)
    R = 15.0
    J = 1 - sstep(dist, 5.0, 9.0)
    keep = 1 - J                                               # 1 on the face, 0 in the groove
    use_h = dist_h <= dist_v
    sy = np.where(use_h & (dist_h < R), yy + np.sign(off_h + 1e-6) * 2 * (R - dist_h), yy)
    sx = np.where(~use_h & (dist_v < R), xx + np.sign(off_v + 1e-6) * 2 * (R - dist_v), xx)
    col = img[np.round(sy).astype(np.int64) % N, np.round(sx).astype(np.int64) % N]
    # one paint, one hue: the picture's yellow patches would be counted every 1.6 m (the macro map carries the wall's own marks)
    cl = col @ LUMA
    mono = (cl / cl.mean())[..., None] * col.reshape(-1, 3).mean(0)
    col = mono * 0.8 + col * 0.2
    # colour: Tom's reference has a slate blue-grey paint; the picture is cream. Its stains and shade stay.
    target = np.array([0.125, 0.180, 0.215], np.float32)     # linear
    col = np.clip(PT.match_mean(col, target, 1.0), 0, 1)
    m = col.reshape(-1, 3).mean(0)
    col = np.clip(m + (col - m) * 1.35, 0, 1)               # the stains a little stronger through the darker paint
    # paint is thinner and dirt sits in the tooled joints
    groove = 1 - sstep(dist, 1.5, 7.5)
    col = col * (1 - 0.30 * groove[..., None])
    fine = noise(N, N, 1.2, 3, lo=24)
    mott = noise(N, N, 2.6, 4, lo=2)
    col = np.clip(col * (1 + 0.035 * mott[..., None]) * (1 + 0.02 * fine[..., None]), 0, 1)
    # height (metres): a concave tooled joint 11 mm wide and 3.5 mm deep, the block faces' pores and small leans
    half = 7.0
    prof = np.clip(1 - dist / half, 0, 1)
    hm = -0.0035 * (prof * prof * (3 - 2 * prof))
    u = (xx / N * 4) % 1 - .5
    v = (yy / N * 8) % 1 - .5
    hm += setb * keep + (tilt[bid, 0] * u + tilt[bid, 1] * v) * 0.6 * keep
    hi = lum - PT.blur(lum, 2.5)
    pores = np.clip(-(fine + 0.9), -3, 0) * 0.00022           # sparse pits
    hm += (hi / (hi.std() + 1e-6) * 0.00006 + pores + noise(N, N, 2.0, 5, lo=6) * 0.00012) * keep
    # chips at a few block corners
    chips = sstep(noise(N, N, 2.2, 6, lo=10), 1.9, 2.6) * sstep(-dist, -26, -6)
    hm -= chips * 0.0018
    col = col * (1 - 0.18 * chips[..., None]) + 0.10 * chips[..., None] * np.array([.62, .60, .55], np.float32)
    rough = 0.56 + 0.26 * groove + 0.05 * mott + 0.03 * fine + 0.20 * chips
    save('T_Rec_Block_base.png', col)
    save('T_Rec_Block_rough.png', rough)
    save('T_Rec_Block_normal.png', normal_dx(hm, (1.6, 1.6)))
    save('T_Rec_Block_height.png', np.clip((hm + 0.004) / 0.006, 0, 1))


# ------------------------------------------------------------------ vinyl tile floor
def floor():
    N = 2048
    a = PT.load(SRC / 'vct_floor.png')                       # 4 x 4 tiles of 512 px
    img = a.copy()
    gm = a.reshape(-1, 3).mean(0)
    for j in range(4):
        for i in range(4):
            t = img[j * 512:(j + 1) * 512, i * 512:(i + 1) * 512]
            inner = t[24:-24, 24:-24].reshape(-1, 3).mean(0)
            t += (gm - inner) * 0.72                         # the one yellowed tile would be seen every 1.2 m
    img = np.clip(img, 0, 1)
    target = PT.srgb2lin(np.array([150, 141, 118], np.float32) / 255)
    img = np.clip(PT.match_mean(img, target, 0.9), 0, 1)
    m = img.reshape(-1, 3).mean(0)
    img = np.clip(m + (img - m) * 0.8, 0, 1)
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    dx = np.abs((xx + 256) % 512 - 256)
    dy = np.abs((yy + 256) % 512 - 256)
    dist = np.minimum(dx, dy)
    joint = 1 - sstep(dist, 0.8, 3.2)
    grime = 1 - sstep(dist, 2.0, 16.0)
    col = img * (1 - 0.34 * joint[..., None]) * (1 - 0.10 * grime[..., None])
    fine = noise(N, N, 1.0, 11, lo=40)
    scr = noise(N, N, 1.6, 12, lo=8)
    col = np.clip(col * (1 + 0.015 * fine[..., None]), 0, 1)
    lum = img @ LUMA
    hi = lum - PT.blur(lum, 3.0)
    # tiles lie flat; the joint is a hairline, each tile's edge a touch low, old scratches
    edge = 1 - sstep(dist, 0.0, 9.0)
    hm = -0.00028 * joint - 0.00006 * edge + hi / (hi.std() + 1e-6) * 0.000012 + np.clip(scr - 1.7, 0, 2) * -0.00004
    rough = 0.42 + 0.30 * joint + 0.10 * grime + 0.06 * np.clip(scr, -2, 2) * .5 + 0.04 * fine - 0.10 * sstep(lum, lum.mean(), lum.max())
    save('T_Rec_Floor_base.png', col)
    save('T_Rec_Floor_rough.png', rough)
    save('T_Rec_Floor_normal.png', normal_dx(hm, (1.2, 1.2)))


# ------------------------------------------------------------------ ceiling tile faces (the T-bars are geometry)
def ceiling():
    N = 1024
    a = PT.load(SRC / 'ceiling_tile.png')
    face = PT.mosaic(a, 2, 2, 0.06, N)                        # 2 x 2 faces of 60 cm, no grid
    # old mineral-fibre tiles gone tan-grey with smoke and age (Tom's sheet: a dark ceiling)
    target = PT.srgb2lin(np.array([118, 108, 90], np.float32) / 255)
    col = np.clip(PT.match_mean(face, target, 1.0), 0, 1)
    m = col.reshape(-1, 3).mean(0)
    col = np.clip(m + (col - m) * 1.5, 0, 1)
    # one face of the four water-stained (the room's leak), one slightly darker
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    n1 = noise(N, N, 2.8, 21, lo=2)
    cx, cy = 256 + 40, 512 + 256 - 30
    r = np.hypot(xx - cx, yy - cy) / 190 + 0.22 * n1
    ring = np.exp(-((r - 0.85) / 0.10) ** 2) * (yy >= 512) * (xx < 512)
    blot = (1 - sstep(r, 0.55, 0.9)) * (yy >= 512) * (xx < 512)
    stain = np.clip(0.55 * ring + 0.30 * blot, 0, 1)[..., None]
    col = col * (1 - stain * np.array([0.35, 0.47, 0.62], np.float32))
    col[512:, 512:] *= 0.93
    lum = face @ LUMA
    hi = lum - PT.blur(lum, 2.0)
    pits = np.clip(-(hi / (hi.std() + 1e-6)) - 1.2, 0, 4)
    hm = -pits * 0.0005 + noise(N, N, 2.0, 22, lo=6) * 0.00015
    save('T_Rec_Ceiling_base.png', col)
    save('T_Rec_Ceiling_rough.png', np.full((N, N), 0.94, np.float32) + 0.03 * noise(N, N, 1.5, 23, lo=8))
    save('T_Rec_Ceiling_normal.png', normal_dx(hm, (1.2, 1.2)))


# ------------------------------------------------------------------ troffer lens: a prismatic acrylic sheet over two tubes
def lens():
    W, Hh = 1024, 512                                         # 1.2 x 0.6 m
    yy, xx = np.mgrid[0:Hh, 0:W].astype(np.float32)
    # two tubes along the long side, 15 cm either side of the middle: the glow the lens shows (also the emissive mask)
    v = yy / Hh
    tube = np.exp(-((v - 0.30) / 0.13) ** 2) + np.exp(-((v - 0.70) / 0.13) ** 2)
    ends = sstep(xx / W, 0.0, 0.06) * sstep(1 - xx / W, 0.0, 0.06)
    glow = (0.55 + 0.45 * np.clip(tube, 0, 1)) * (0.80 + 0.20 * ends)
    # yellowed acrylic, dead flies and dust in the corners
    n = noise(Hh, W, 2.4, 31, lo=2)
    dust = sstep(np.maximum(np.abs(v - .5) * 2, np.abs(xx / W - .5) * 2), 0.86, 1.0)
    specks = sstep(noise(Hh, W, 0.6, 32, lo=30), 2.6, 3.0)
    base = glow[..., None] * np.array([1.0, 0.97, 0.88], np.float32) * (1 - 0.10 * dust[..., None]) * (1 + 0.02 * n[..., None]) * (1 - 0.55 * specks[..., None])
    save('T_Rec_Lens_base.png', base)
    # K12 pattern: small square pyramids, 4.8 mm pitch (250 across 1.2 m is too fine for 1024 px: 128 across, 9.4 mm)
    p = 8.0
    fx = (xx % p) / p - .5
    fy = (yy % p) / p - .5
    hm = (0.5 - np.maximum(np.abs(fx), np.abs(fy))) * 0.004
    save('T_Rec_Lens_normal.png', normal_dx(hm, (1.2, 0.6)))
    save('T_Rec_Lens_rough.png', np.full((Hh, W), 0.32, np.float32) + 0.25 * dust)


# ------------------------------------------------------------------ macro maps
def blobs(h, w, seed, n, rmin, rmax, soft=0.5):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    out = np.zeros((h, w), np.float32)
    for _ in range(n):
        cx, cy, r = rng.random() * w, rng.random() * h, rmin + rng.random() * (rmax - rmin)
        out = np.maximum(out, 1 - sstep(np.hypot(xx - cx, yy - cy), r * (1 - soft), r))
    return out


def wallmacro():
    CW, CH = 1024, 512
    atlas = np.zeros((CH * 2, CW * 2, 4), np.float32)

    def cell(length, seed):
        """(mult RGB around 1, rough add) for one wall, u 0..1 along `length` metres, row 0 at the ceiling."""
        yy, xx = np.mgrid[0:CH, 0:CW].astype(np.float32)
        u = xx / CW * length                                   # metres along the wall
        y = (1 - yy / CH) * H                                  # metres up
        n1 = noise(CH, CW, 2.6, seed, lo=1.5)
        n2 = noise(CH, CW, 1.6, seed + 1, lo=6)
        mult = np.ones((CH, CW, 3), np.float32)
        ra = np.zeros((CH, CW), np.float32)
        # the mop's splash and dust low on the wall
        low = (1 - sstep(y, 0.0, 0.42 + 0.10 * n1)) ** 1.6 * (0.85 + 0.15 * sstep(n2, -1.5, 1.5))
        mult *= 1 - 0.26 * low[..., None] * np.array([1.0, 0.95, 0.85], np.float32)
        ra += 0.16 * low
        # smoke and dust under the ceiling
        top = sstep(y, 2.45 + 0.05 * n1, 2.72)
        mult *= 1 - 0.14 * top[..., None]
        # slow tone drift, so no wall is one flat value
        mult *= 1 + 0.05 * n1[..., None]
        return yy, xx, u, y, n1, n2, mult, ra

    def band(u, y, n, u0, u1, y0, y1, k=1.0):
        return sstep(u, u0, u0 + .08) * sstep(-u, -u1, -u1 + .08) * sstep(y, y0, y0 + .03) * sstep(-y, -y1, -y1 + .03) * k

    def scuffs(u, y, n2, seed, u0, u1, yc, spread, n):
        """short horizontal rubs at chair or cart height: lighter where the paint is burnished and chipped"""
        rng = np.random.default_rng(seed)
        out = np.zeros_like(u)
        for _ in range(n):
            cu = u0 + rng.random() * (u1 - u0)
            cy = yc + rng.normal(0, spread)
            lu = 0.06 + rng.random() * 0.30
            th = 0.004 + rng.random() * 0.012
            out = np.maximum(out, np.exp(-((y - cy) / th) ** 2) * sstep(u, cu - lu / 2, cu - lu / 2 + .03) * sstep(-u, -(cu + lu / 2), -(cu + lu / 2) + .03))
        return out * (0.6 + 0.4 * sstep(n2, -1, 1))

    def grime(u, y, n1, cu, cy, ru, ry, k=1.0):
        return np.exp(-(((u - cu) / ru) ** 2 + ((y - cy) / ry) ** 2)) * (0.8 + 0.2 * n1) * k

    def drip(u, y, n2, cu, ytop, length, width, seed):
        rng = np.random.default_rng(seed)
        out = np.zeros_like(u)
        for k in range(5):
            x = cu + rng.normal(0, width)
            ln = length * (0.4 + 0.6 * rng.random())
            w = 0.006 + 0.012 * rng.random()
            out = np.maximum(out, np.exp(-((u - x - 0.01 * n2) / w) ** 2) * sstep(-y, -ytop, -ytop + .02) * sstep(y, ytop - ln, ytop - ln + 0.5 * ln))
        return out

    # ---- north wall (z 6.8): u = x + 6. The doorway is x -4.4..-3.4 (u 1.6..2.6); the switch at u 1.25..1.38, y 1.25..1.35; the
    # clerk's desk u 3.62..4.98 (top 0.74); the open leaf lies on u 2.58..3.54; the cork board u 3.73..4.77, y 1.18..1.92.
    yy, xx, u, y, n1, n2, mult, ra = cell(5.0, 100)
    g = grime(u, y, n1, 1.31, 1.28, 0.16, 0.20, 0.34) + grime(u, y, n1, 1.5, 1.05, 0.10, 0.45, 0.16) + grime(u, y, n1, 2.70, 1.05, 0.10, 0.5, 0.14)
    sc = scuffs(u, y, n2, 101, 0.55, 1.5, 0.80, 0.05, 9) + scuffs(u, y, n2, 102, 2.7, 3.5, 0.98, 0.02, 4) + scuffs(u, y, n2, 103, 3.6, 5.0, 0.75, 0.015, 7) + scuffs(u, y, n2, 104, 0.5, 1.5, 0.32, 0.06, 5)
    touch = band(u, y, n1, 0.62, 1.22, 1.5, 2.2, 0.06)       # a touched-up patch, rolled a shade lighter
    mult *= (1 - g[..., None] * np.array([0.9, 0.95, 1.0], np.float32)) * (1 + 0.22 * sc[..., None]) * (1 + touch[..., None])
    ra += 0.20 * g - 0.12 * sc
    atlas[0:CH, 0:CW, :3] = mult; atlas[0:CH, 0:CW, 3] = ra
    # ---- south wall (z 10.5): u = x + 6. Cabinets u 0.75..2.33 to 1.32 (boxes above), bookcase u 2.7..4.6 to 1.2, the ceiling's
    # leak at u 0.4..0.9 (the old drip: x -5.62).
    yy, xx, u, y, n1, n2, mult, ra = cell(5.0, 200)
    dr = drip(u, y, n2, 0.42, 2.74, 1.5, 0.07, 201) + 0.6 * drip(u, y, n2, 0.95, 2.74, 0.7, 0.05, 202)
    shadow = band(u, y, n1, 0.75, 2.33, 1.30, 1.42, 0.10) + band(u, y, n1, 2.7, 4.6, 1.18, 1.30, 0.10)   # dust line over the furniture
    sc = scuffs(u, y, n2, 203, 2.4, 2.7, 0.6, 0.2, 4)
    mult *= (1 - np.clip(dr, 0, 1)[..., None] * np.array([0.30, 0.42, 0.55], np.float32)) * (1 - shadow[..., None]) * (1 + 0.2 * sc[..., None])
    ra += 0.22 * np.clip(dr, 0, 1)
    atlas[0:CH, CW:, :3] = mult; atlas[0:CH, CW:, 3] = ra
    # ---- west wall (x -6): u = z - 6.8. Behind the shelving; the leak's corner at u 2.9..3.7.
    yy, xx, u, y, n1, n2, mult, ra = cell(3.7, 300)
    dr = drip(u, y, n2, 3.35, 2.74, 1.9, 0.10, 301)
    dustb = sum(band(u, y, n1, 0.3, 3.4, s - 0.01, s + 0.05, 0.10) for s in (0.12, 0.62, 1.12, 1.62, 2.12))
    mult *= (1 - np.clip(dr, 0, 1)[..., None] * np.array([0.30, 0.42, 0.55], np.float32)) * (1 - dustb[..., None])
    ra += 0.22 * np.clip(dr, 0, 1) + 0.1 * dustb
    atlas[CH:, 0:CW, :3] = mult; atlas[CH:, 0:CW, 3] = ra
    # ---- east wall (x -1): u = z - 6.8. Card index u 0.69..1.62 to 1.08, the plate above; the phone u 2.3..2.6, y 1.28..1.56; the
    # calendar u 2.87..3.13; the KSU u 3.25..3.59; bare wall u 1.62..3.3 where carts and chairs have rubbed.
    yy, xx, u, y, n1, n2, mult, ra = cell(3.7, 400)
    g = grime(u, y, n1, 2.45, 1.30, 0.22, 0.26, 0.30) + grime(u, y, n1, 2.2, 1.0, 0.25, 0.35, 0.10)
    sc = scuffs(u, y, n2, 401, 1.65, 3.3, 0.82, 0.05, 12) + scuffs(u, y, n2, 402, 1.65, 3.3, 0.34, 0.05, 8) + scuffs(u, y, n2, 403, 0.0, 0.66, 0.8, 0.1, 4)
    touch = band(u, y, n1, 1.75, 2.15, 1.55, 2.25, 0.05)
    mult *= (1 - g[..., None] * np.array([0.9, 0.95, 1.0], np.float32)) * (1 + 0.22 * sc[..., None]) * (1 + touch[..., None])
    ra += 0.20 * g - 0.12 * sc
    atlas[CH:, CW:, :3] = mult; atlas[CH:, CW:, 3] = ra
    rgb = np.clip(atlas[..., :3] * 0.5, 0, 1)
    al = np.clip(0.5 + atlas[..., 3], 0, 1)
    Image.fromarray((np.concatenate([rgb, al[..., None]], -1) * 255 + .5).astype(np.uint8), 'RGBA').save(OUT / 'T_Rec_WallMacro.png')
    save('_preview_WallMacro_rgb.png', np.clip(atlas[..., :3] * 0.5 * 1.6, 0, 1))
    print('wrote T_Rec_WallMacro.png', flush=True)


def floormacro():
    W, Hh = 2048, 1536
    yy, xx = np.mgrid[0:Hh, 0:W].astype(np.float32)
    x = X0 + xx / W * (X1 - X0)
    z = Z0 + yy / Hh * (Z1 - Z0)
    n1 = noise(Hh, W, 2.6, 500, lo=1.5)
    n2 = noise(Hh, W, 1.5, 501, lo=10)
    mult = np.ones((Hh, W, 3), np.float32)
    ra = np.zeros((Hh, W), np.float32)
    # every tile its own shade; a few laid later, lighter and cleaner (the joints are at 0.3 k in x and z)
    rng = np.random.default_rng(47)
    ti = np.floor(x / 0.3 + 1e-4).astype(np.int32) - int(math.floor(X0 / 0.3)) + 1
    tj = np.floor(z / 0.3 + 1e-4).astype(np.int32) - int(math.floor(Z0 / 0.3))
    tone = rng.normal(1.0, 0.035, (16, 20)).astype(np.float32)
    hue = rng.normal(0.0, 0.005, (16, 20, 3)).astype(np.float32)
    newer = [(3, 9), (4, 9), (11, 3), (9, 14), (2, 5)]
    for j, i in newer:
        tone[j, i] = 1.13
        hue[j, i] = (0.012, 0.006, -0.012)
    t = tone[tj, ti][..., None] + hue[tj, ti]
    mult *= t
    isnew = np.zeros((Hh, W), np.float32)
    for j, i in newer:
        isnew = np.maximum(isnew, ((tj == j) & (ti == i)).astype(np.float32))
    # dust and old wax along the walls, under the shelving, behind and between the furniture
    dwall = np.minimum(np.minimum(x - X0, X1 - x), np.minimum(z - Z0, Z1 - z))
    edge = (1 - sstep(dwall, 0.03, 0.30 + 0.06 * n1))
    under = np.maximum(sstep(-x, 5.38, 5.52), sstep(z, 9.78, 9.9) * sstep(-x, 3.6, 3.7))     # under the shelves, by the cabinets' feet
    dirt = np.clip(edge * 0.75 + under * 0.8, 0, 1) * (0.8 + 0.2 * sstep(n2, -1, 1))
    mult *= 1 - 0.30 * dirt[..., None] * np.array([1.0, 0.96, 0.88], np.float32)
    ra += 0.22 * dirt

    def lane(points, width):
        d = np.full((Hh, W), 1e9, np.float32)
        for (ax, az), (bx, bz) in zip(points[:-1], points[1:]):
            vx, vz = bx - ax, bz - az
            L2 = vx * vx + vz * vz
            tt = np.clip(((x - ax) * vx + (z - az) * vz) / L2, 0, 1)
            d = np.minimum(d, np.hypot(x - (ax + tt * vx), z - (az + tt * vz)))
        return 1 - sstep(d, width * 0.35, width)

    # where people walk: in at the door, round both ends of the table, to the cabinets, the bookcase, the wall phone and the card index
    walk = np.maximum.reduce([
        lane([(-3.9, 6.8), (-3.9, 7.55)], 0.62),
        lane([(-3.9, 7.5), (-4.98, 7.75), (-5.0, 9.45), (-4.5, 9.45)], 0.48),
        lane([(-3.9, 7.5), (-2.3, 7.75), (-2.25, 9.55), (-3.2, 9.5)], 0.50),
        lane([(-2.25, 8.9), (-1.6, 9.2)], 0.42),
        lane([(-4.5, 9.45), (-3.2, 9.5)], 0.36),
    ])
    walk = walk * (0.7 + 0.3 * sstep(n1, -1.2, 1.2)) * (1 - 0.75 * isnew)
    thresh = lane([(-3.9, 6.8), (-3.9, 7.15)], 0.55)
    mult *= 1 - (0.17 * walk + 0.10 * thresh)[..., None] * np.array([1.0, 0.98, 0.93], np.float32)
    ra += 0.20 * walk + 0.05 * thresh
    # the chair's feet at the table and at the clerk's desk; the cart's turn by the shelving
    rng = np.random.default_rng(48)
    scr = np.zeros((Hh, W), np.float32)
    for cx, cz, n, sx, sz, ang in ((-3.65, 9.3, 26, 0.30, 0.22, 1.57), (-1.75, 7.6, 16, 0.28, 0.20, 1.57), (-4.95, 8.6, 14, 0.12, 0.7, 1.57), (-3.9, 7.1, 12, 0.3, 0.2, 1.3)):
        for _ in range(n):
            px, pz = cx + rng.normal(0, sx), cz + rng.normal(0, sz)
            a = ang + rng.normal(0, 0.35)
            ln = 0.05 + rng.random() * 0.16
            wd = 0.002 + rng.random() * 0.003
            ux, uz = math.cos(a), math.sin(a)
            along = (x - px) * ux + (z - pz) * uz
            across = -(x - px) * uz + (z - pz) * ux
            scr = np.maximum(scr, np.exp(-(across / wd) ** 2) * sstep(along, -ln / 2, -ln / 2 + .02) * sstep(-along, -ln / 2, -ln / 2 + .02) * (0.5 + 0.5 * rng.random()))
    mult *= 1 + 0.10 * scr[..., None]
    ra += 0.22 * scr
    # under the ceiling's leak, south-west: a dried pool with its tide line
    r = np.hypot((x + 5.15) / 0.42, (z - 9.75) / 0.34) + 0.16 * n1
    ring = np.exp(-((r - 1.0) / 0.07) ** 2)
    pool = 1 - sstep(r, 0.7, 1.0)
    mult *= 1 - (0.30 * ring + 0.10 * pool)[..., None] * np.array([0.7, 0.85, 1.0], np.float32)
    ra += 0.2 * ring + 0.12 * pool
    # a mug's worth spilt by the table's east end, long ago; a burn by the clerk's desk
    for cx, cz, rr, k in ((-2.55, 8.75, 0.16, 0.16), (-2.0, 7.55, 0.035, 0.5), (-4.75, 9.2, 0.10, 0.12)):
        b = 1 - sstep(np.hypot(x - cx, z - cz) + 0.03 * n1, rr * 0.6, rr)
        mult *= 1 - k * b[..., None] * np.array([0.8, 0.9, 1.0], np.float32)
    mult *= 1 + 0.04 * n1[..., None]
    rgb = np.clip(mult * 0.5, 0, 1)
    al = np.clip(0.5 + ra, 0, 1)
    Image.fromarray((np.concatenate([rgb, al[..., None]], -1) * 255 + .5).astype(np.uint8), 'RGBA').save(OUT / 'T_Rec_FloorMacro.png')
    save('_preview_FloorMacro_rgb.png', np.clip(mult * 0.5 * 1.6, 0, 1))
    print('wrote T_Rec_FloorMacro.png', flush=True)


# ------------------------------------------------------------------ cork (the board), kraft and grey archive board
def cork():
    N = 1024                                                   # 0.5 m
    g1 = noise(N, N, 0.4, 61, lo=30)
    g2 = noise(N, N, 1.6, 62, lo=6)
    g3 = noise(N, N, 2.6, 63, lo=2)
    grain = sstep(g1, -0.4, 1.6)
    dark = sstep(noise(N, N, 0.8, 64, lo=40), 1.4, 2.2)
    base = np.array([0.50, 0.335, 0.19], np.float32) * (0.86 + 0.20 * grain[..., None]) * (1 + 0.05 * g2[..., None]) * (1 + 0.04 * g3[..., None]) * (1 - 0.45 * dark[..., None])
    hm = grain * 0.0004 - dark * 0.0007 + g2 * 0.00008
    save('T_Rec_Cork_base.png', PT.lin2srgb(np.clip(base * 0.62, 0, 1)))
    save('T_Rec_Cork_rough.png', 0.90 + 0.05 * g2)
    save('T_Rec_Cork_normal.png', normal_dx(hm, (0.5, 0.5)))


def board():
    """Archive-box board for the Blender materials: kraft (brown) and grey chip board, 1024 px over 0.5 m; fibres run along u."""
    N = 1024
    for name, lin, seed in (('Kraft', (0.37, 0.255, 0.145), 71), ('GreyBoard', (0.36, 0.345, 0.30), 81), ('WhiteBoard', (0.62, 0.60, 0.53), 91)):
        f1 = noise(N, N, 1.0, seed, lo=20)
        yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
        fib = PT.blur(noise(N, N, 0.2, seed + 1, lo=60), 0.0)
        # fibres: fine noise smeared along u
        F = np.fft.rfft2(noise(N, N, 0.0, seed + 2, lo=1))
        fy = np.fft.fftfreq(N)[:, None]; fx = np.fft.rfftfreq(N)[None, :]
        F *= np.exp(-(fx * 380) ** 2) * (1 - np.exp(-(fy * 90) ** 2))
        fib = np.fft.irfft2(F, s=(N, N)); fib = (fib / (fib.std() + 1e-9)).astype(np.float32)
        mot = noise(N, N, 2.4, seed + 3, lo=2)
        fleck = sstep(noise(N, N, 0.5, seed + 4, lo=50), 2.2, 2.9)
        # the flutes of corrugated board telegraph faintly through the liner: 7 mm pitch across v
        flute = np.sin(yy / N * 2 * math.pi * (0.5 / 0.007)) if name == 'Kraft' else 0 * yy
        base = np.array(lin, np.float32) * (1 + 0.07 * fib[..., None]) * (1 + 0.06 * mot[..., None]) * (1 + 0.025 * f1[..., None]) * (1 - 0.30 * fleck[..., None]) * (1 + 0.012 * flute[..., None])
        hm = fib * 0.00006 + flute * 0.00005 + mot * 0.0001
        save('T_Rec_%s_base.png' % name, PT.lin2srgb(np.clip(base, 0, 1)))
        save('T_Rec_%s_rough.png' % name, 0.88 + 0.04 * mot + 0.03 * fib)
        save('T_Rec_%s_normal.png' % name, normal_dx(hm, (0.5, 0.5)))


for key, fn in (('block', block), ('floor', floor), ('ceiling', ceiling), ('lens', lens), ('wallmacro', wallmacro), ('floormacro', floormacro), ('cork', cork), ('board', board)):
    if key in WHAT:
        fn()
print('SURFACES DONE', sorted(WHAT))
