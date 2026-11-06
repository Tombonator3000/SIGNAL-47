"""Egne prosedyrale PBR-kart til gårdsbyggene. Ingen nedlastinger, ingen foto, ingen uttrekk.

Kjøres med en Python som har numpy og Pillow (samme som townbuildings/make_textures.py):
  python3 textures.py <utmappe>
Skriver T_Farm_<Materiale>_BC/N/R.png, 1024 px og sømløse. Normalkart er DirectX (grønn ned),
som resten av prosjektet. Tile i meter står i fbkit.TILE.
"""
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

N = 1024
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('textures')


def vn(rng, nx, ny):
    """Sømløs verdistøy med nx x ny celler."""
    g = rng.random((ny, nx), dtype=np.float32)
    g = np.pad(g, ((0, 1), (0, 1)), mode='wrap')
    tx = np.arange(N, dtype=np.float32) * nx / N
    ty = np.arange(N, dtype=np.float32) * ny / N
    ix, iy = tx.astype(int), ty.astype(int)
    fx, fy = tx - ix, ty - iy
    fx = fx * fx * (3 - 2 * fx)
    fy = fy * fy * (3 - 2 * fy)
    a = g[iy[:, None], ix[None, :]] * (1 - fx)[None, :] + g[iy[:, None], ix[None, :] + 1] * fx[None, :]
    b = g[iy[:, None] + 1, ix[None, :]] * (1 - fx)[None, :] + g[iy[:, None] + 1, ix[None, :] + 1] * fx[None, :]
    return a * (1 - fy)[:, None] + b * fy[:, None]


def fbm(rng, nx, ny, oct=4, gain=.5):
    out = np.zeros((N, N), np.float32)
    amp, tot = 1., 0.
    for i in range(oct):
        out += vn(rng, nx * 2 ** i, ny * 2 ** i) * amp
        tot += amp
        amp *= gain
    return out / tot


def sst(a, b, v):
    t = np.clip((v - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def mix(a, b, t):
    t = t[..., None] if t.ndim == 2 else t
    return a * (1 - t) + b * t


def rgb(r, g, b):
    return np.array([r, g, b], np.float32)


def save(name, color, height, rough, strength=5.0, emit=None):
    OUT.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.clip(color, 0, 255).astype('uint8'), 'RGB').save(OUT / f'T_Farm_{name}_BC.png')
    dx = (np.roll(height, -1, 1) - np.roll(height, 1, 1)) * .5 * strength
    dy = (np.roll(height, -1, 0) - np.roll(height, 1, 0)) * .5 * strength
    n = np.stack([-dx, dy, np.ones_like(height)], axis=2)
    n /= np.linalg.norm(n, axis=2)[:, :, None]
    Image.fromarray(np.clip((n * .5 + .5) * 255, 0, 255).astype('uint8'), 'RGB').save(OUT / f'T_Farm_{name}_N.png')
    Image.fromarray(np.clip(rough * 255, 0, 255).astype('uint8'), 'L').save(OUT / f'T_Farm_{name}_R.png')
    if emit is not None:
        Image.fromarray(np.clip(emit, 0, 255).astype('uint8'), 'RGB').save(OUT / f'T_Farm_{name}_E.png')


def cracks(rng, count, length, wobble, width=1):
    """Sprekker tegnet med sømløs omvikling. Returnerer maske 0..1."""
    im = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(im)
    for _ in range(count):
        x, y = rng.random(2) * N
        a = rng.random() * 6.28
        pts = [(x, y)]
        for _ in range(int(length * (.4 + rng.random()))):
            a += (rng.random() - .5) * wobble
            x += math.cos(a) * 9
            y += math.sin(a) * 9
            pts.append((x, y))
        for ox in (-N, 0, N):
            for oy in (-N, 0, N):
                d.line([(p[0] + ox, p[1] + oy) for p in pts], fill=255, width=width)
    return np.asarray(im, np.float32) / 255


def grain_field(rng, boards=None, across=240, along=5):
    """Trekorn langs v. Avlange striper med litt svai."""
    warp = (vn(rng, 3, 2) - .5) * 26
    yy, xx = np.mgrid[0:N, 0:N]
    field = np.zeros((N, N), np.float32)
    amp = 1.
    for a, b in ((across, along), (across * 2, along * 2), (across // 2, along)):
        n = vn(rng, a, b)
        sx = ((xx + warp).astype(int)) % N
        field += n[yy, sx] * amp
        amp *= .6
    return field / 2.0


def board():
    rng = np.random.default_rng(1986)
    # Ti bord med ulik bredde over 2,4 m. Bredden i meter blir piksler.
    widths = [.2, .3, .18, .26, .22, .28, .18, .3, .24, .24]
    edges = np.round(np.cumsum([0] + widths) / 2.4 * N).astype(int)
    col = np.zeros((N, N, 3), np.float32)
    edge = np.zeros((N, N), np.float32)
    g = grain_field(rng, across=150, along=3)
    for i in range(len(widths)):
        a, b = edges[i], edges[i + 1]
        tone = rng.random()
        warm = rng.random()
        base = mix(rgb(150, 144, 134), rgb(170, 154, 132), np.full((N, b - a), warm))
        # Noen bord nesten hvite av sol, noen mørke og nyere.
        scale = 1.18 if tone > .82 else .66 if tone < .2 else .86 + tone * .22
        col[:, a:b] = base * scale
        ramp = np.minimum(np.arange(b - a), np.arange(b - a)[::-1]) / 8
        edge[:, a:b] = np.clip(ramp, 0, 1)[None, :]
    col *= (.84 + .26 * g)[..., None]
    # Avskallet rest av gammel rødbrun maling, svak og i lange striper.
    ghost = sst(.5, .72, fbm(rng, 22, 3, 4)) * .2
    col = mix(col, rgb(136, 108, 92) * (.85 + .25 * g[..., None]), ghost)
    # Solbleking og regnstriper i lange baner, og noen knaster.
    col *= (.9 + .2 * fbm(rng, 34, 2, 3))[..., None]
    im = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(im)
    for _ in range(5):
        x, y = rng.random(2) * N
        ex, ey = 4 + rng.random() * 6, 8 + rng.random() * 14
        d.ellipse((x - ex, y - ey, x + ex, y + ey), fill=255)
    knots = np.asarray(im.filter(ImageFilter.GaussianBlur(3)), np.float32) / 255
    col = mix(col, rgb(62, 50, 42), knots * .75)
    # Færre og større spikere med rustspor under.
    im = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(im)
    trail = Image.new('L', (N, N), 0)
    dt = ImageDraw.Draw(trail)
    for i in range(0, len(widths)):
        for row in (rng.random() * .4, .45 + rng.random() * .5):
            x = (edges[i] + edges[i + 1]) * .5 + (rng.random() - .5) * 26
            y = row * N
            d.ellipse((x - 3, y - 3, x + 3, y + 3), fill=255)
            dt.line((x, y, x + rng.random() * 2, y + 30 + rng.random() * 50), fill=110, width=3)
    nails = np.asarray(im, np.float32) / 255
    trails = np.asarray(trail.filter(ImageFilter.GaussianBlur(2)), np.float32) / 255
    col = mix(col, rgb(92, 62, 44), trails * .55)
    col = mix(col, rgb(42, 32, 28), nails * .9)
    gap = 1 - sst(0, .55, edge)
    col *= (1 - gap * .88)[..., None]
    height = g * .55 * edge + edge * 1.1 - nails * .2 - knots * .3
    rough = .96 - ghost * .1 + (g - .5) * .04
    save('Board', col, height, rough, 2.4)


def tin():
    """Én platelengde per tile (3,2 m). V = 0 er nederste kant. Rust følger kant og spikerrader."""
    rng = np.random.default_rng(47)
    xx = np.arange(N) / N
    waves = 42
    prof = np.sin(xx * 2 * np.pi * waves)
    h = np.tile(prof, (N, 1)).astype(np.float32)
    patina = fbm(rng, 5, 7, 5)
    speck = fbm(rng, 90, 90, 2)
    col = mix(rgb(150, 145, 134), rgb(116, 112, 104), sst(.3, .72, patina))
    yy = np.arange(N, dtype=np.float32) / N   # Bildets rad 0 er toppen av platen.
    bottom = np.exp(-(1 - yy) / .07)
    top = np.exp(-yy / .02) * .5
    rows = np.zeros(N, np.float32)
    nail_rows = (.06, .5, .94)
    for row in nail_rows:
        rows += np.exp(-np.maximum(yy - row, 0) / .09) * (yy > row)
    streaks = fbm(rng, 42, 2, 3)
    run = sst(.3, .7, streaks) * (rows[:, None] * .75 + bottom[:, None] * 1.2)
    blooms = sst(.7, .84, fbm(rng, 8, 8, 5) + .1 * speck) * .55
    rust = np.clip(run + blooms + top[:, None] * sst(.4, .7, streaks), 0, 1)
    valley = sst(.2, -.6, h)
    rust = np.clip(rust * (.7 + .5 * valley), 0, 1)
    rustcol = mix(rgb(112, 74, 48), rgb(82, 56, 40), sst(.35, .7, speck))
    col = mix(col, rustcol, rust * .85)
    col *= (.92 + .16 * speck)[..., None]
    # Bølgene som bakt skygge, så de leses på avstand.
    col *= (.84 + .16 * (h[..., None] * .5 + .5))
    # Spikerhoder på toppene.
    im = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(im)
    for row in nail_rows:
        for k in range(0, waves, 4):
            x = (k + .25) / waves * N
            d.ellipse((x - 2.5, row * N - 2.5, x + 2.5, row * N + 2.5), fill=255)
    nails = np.asarray(im, np.float32) / 255
    col = mix(col, rgb(52, 42, 36), nails)
    lap = np.exp(-((yy - .997) / .004) ** 2) + np.exp(-(yy / .004) ** 2)
    col *= (1 - .6 * lap[:, None, None])
    rough = .58 + rust * .36 + (speck - .5) * .08
    save('Tin', col, h * 1.1 + rust * .08, rough, 3.6)


def stucco():
    rng = np.random.default_rng(1948)
    coarse = fbm(rng, 5, 5, 5)
    fine = fbm(rng, 110, 110, 2)
    trowel = fbm(rng, 14, 30, 3)
    col = mix(rgb(158, 132, 106), rgb(178, 154, 126), sst(.32, .7, coarse))
    # Solblekt og litt mørkere der regnet renner.
    col = mix(col, rgb(136, 110, 88), sst(.55, .8, fbm(rng, 18, 4, 4)) * .35)
    col *= (.9 + .2 * fine)[..., None]
    col *= (.9 + .2 * fbm(rng, 26, 26, 3))[..., None]
    # Få og små reparerte flekker i lysere sementpuss.
    patch = sst(.76, .8, fbm(rng, 5, 7, 5))
    col = mix(col, rgb(176, 166, 146) * (.88 + .24 * fine[..., None]), patch * .6)
    # Sjeldne steder der pussen har falt av, med adobesteinen bak.
    spall = sst(.8, .815, fbm(rng, 7, 9, 6))
    yy, xx = np.mgrid[0:N, 0:N]
    course = np.abs(((yy + 50 * ((xx // 160) % 2)) % 64) / 64 - .5) < .05
    adobe = rgb(122, 92, 68) * (.82 + .3 * fine[..., None])
    adobe = mix(adobe, rgb(84, 64, 50), course.astype(np.float32) * .7)
    col = mix(col, adobe, spall)
    # Hårfine sprekker, ikke slanger.
    c = cracks(rng, 20, 14, .7, 1)
    c = np.asarray(Image.fromarray((c * 255).astype('uint8')).filter(ImageFilter.GaussianBlur(.6)), np.float32) / 255
    col *= (1 - c * .38)[..., None]
    h = trowel * 1.3 + fine * .3 - c * .5 - spall * 1.0 + patch * .2
    rough = .9 + (fine - .5) * .1
    save('Stucco', col, h, rough, 3.0)


def wood():
    rng = np.random.default_rng(1910)
    g = grain_field(rng, across=320, along=5)
    col = mix(rgb(84, 74, 64), rgb(136, 122, 106), sst(.3, .75, g))
    col *= (.84 + .3 * fbm(rng, 5, 8, 4))[..., None]
    k = cracks(rng, 22, 20, .12, 2)
    col *= (1 - k * .6)[..., None]
    save('Wood', col, g * 1.3 - k * .7, .88 + (g - .5) * .1, 3.0)


def paint():
    rng = np.random.default_rng(1956)
    g = grain_field(rng, across=240, along=4)
    wooden = mix(rgb(96, 88, 78), rgb(136, 124, 108), sst(.35, .75, g))
    base = mix(rgb(96, 122, 116), rgb(120, 142, 132), sst(.3, .7, fbm(rng, 6, 10, 4)))
    base = mix(base, rgb(148, 164, 152), sst(.5, .8, fbm(rng, 30, 8, 3)) * .4)
    base *= (.84 + .22 * g[..., None])
    # Avflasset maling i kornretningen, grått tre under. Ingen rust på tre.
    chipf = fbm(rng, 6, 14, 5) * .8 + g * .2
    chip = sst(.585, .61, chipf)
    edge = sst(.555, .585, chipf) * (1 - chip)
    col = mix(base, wooden, chip)
    col = mix(col, rgb(64, 58, 52), edge * .35)
    col *= (.92 + .16 * fbm(rng, 70, 70, 2))[..., None]
    save('Paint', col, (1 - chip) * .6 + g * .7, .68 + chip * .24 + (g - .5) * .1, 3.4)


def concrete():
    rng = np.random.default_rng(1961)
    c = fbm(rng, 6, 6, 5)
    f = fbm(rng, 170, 170, 2)
    col = mix(rgb(146, 138, 122), rgb(172, 162, 144), sst(.3, .7, c))
    col *= (.9 + .2 * f)[..., None]
    cr = cracks(rng, 10, 26, .9, 2)
    col *= (1 - cr * .7)[..., None]
    col = mix(col, rgb(105, 88, 70), sst(.58, .78, fbm(rng, 5, 14, 4)) * .55)
    save('Concrete', col, f * .9 - cr, .88 + (f - .5) * .1, 2.4)


def steel():
    rng = np.random.default_rng(1973)
    c = fbm(rng, 8, 8, 5)
    f = fbm(rng, 100, 100, 2)
    col = mix(rgb(60, 56, 52), rgb(106, 62, 38), sst(.42, .66, c))
    col *= (.82 + .34 * f)[..., None]
    save('Steel', col, c * .7 + f * .5, .55 + sst(.42, .66, c) * .35, 2.0)


def glass():
    rng = np.random.default_rng(1982)
    dust = fbm(rng, 5, 7, 5)
    streak = fbm(rng, 40, 3, 3)
    col = mix(rgb(20, 26, 28), rgb(58, 56, 50), sst(.45, .8, dust) * .55)
    col = mix(col, rgb(70, 66, 58), sst(.55, .8, streak) * .22)
    rough = .08 + sst(.4, .8, dust) * .42
    save('Glass', col, np.zeros((N, N), np.float32), rough, 0)


def windowlit():
    rng = np.random.default_rng(1986)
    xx = np.arange(N) / N
    folds = .68 + .32 * np.sin(xx * 2 * np.pi * 7 + 4 * vn(rng, 4, 1)[0])
    folds = np.tile(folds, (N, 1)).astype(np.float32)
    yy = np.arange(N) / N
    grad = 1 - .35 * np.abs(yy - .55)
    emit = np.stack([255 * folds, 190 * folds, 112 * folds], 2) * grad[:, None, None]
    # Teksturens V peker opp, så rader i bildet er snudd. Gardinen er likevel symmetrisk nok.
    col = emit * .35
    save('WindowLit', col, np.zeros((N, N), np.float32), np.full((N, N), .25, np.float32), 0, emit)


def dirt():
    rng = np.random.default_rng(1947)
    c = fbm(rng, 6, 6, 5)
    f = fbm(rng, 140, 140, 2)
    col = mix(rgb(96, 80, 60), rgb(132, 110, 82), sst(.3, .72, c))
    col *= (.8 + .4 * f)[..., None]
    # Halm.
    im = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(im)
    for _ in range(900):
        x, y = rng.random(2) * N
        a = rng.random() * 6.28
        ln = 8 + rng.random() * 32
        for ox in (-N, 0, N):
            for oy in (-N, 0, N):
                d.line((x + ox, y + oy, x + ox + math.cos(a) * ln, y + oy + math.sin(a) * ln), fill=255, width=1)
    straw = np.asarray(im, np.float32) / 255 * sst(.35, .6, fbm(rng, 7, 7, 3))
    col = mix(col, rgb(176, 150, 96), straw * .8)
    save('Dirt', col, c * .9 + f * .5 + straw * .4, .95 - straw * .1, 2.4)


if __name__ == '__main__':
    for fn in (board, tin, stucco, wood, paint, concrete, steel, glass, windowlit, dirt):
        fn()
        print('ok', fn.__name__, flush=True)
