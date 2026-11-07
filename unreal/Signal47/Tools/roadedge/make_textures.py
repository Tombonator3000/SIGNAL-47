#!/usr/bin/env python3
"""T_RoadEdge_M: maskene veikanten tegnes etter. Egen prosedyral støy, ingen foto og ingen nedlastinger.

    <python med numpy og Pillow> make_textures.py <utmappe> [--preview]

Kartet går 256 m langs veien (U = z / 256) og 32 m på tvers (V = x / 32, med fortegn, så hver side har sitt), sømløst begge veier.
R: lange tunger av jord og sand, 30 til 60 m lange og 3 til 6 m brede, jevnt fordelt 0 til 1
G: flekker på 1 til 4 m, jevnt fordelt 0 til 1
B: hjulspor der biler har kjørt ut på kanten: 0 uten spor, 1 midt i sporet
A: småstein og korn. Leses i egen skala (z / 5.3, x / 2.7) av materialet, så den gjelder ikke disse målene

Regelen som bruker kartet står i EDGE_HLSL i import_assets.py. --preview tegner den samme regelen ovenfra med flate farger.
"""
import argparse
from pathlib import Path

import numpy as np
from PIL import Image

H, W = 1024, 4096
ALONG, ACROSS = 256., 32.


def noise(rng, h, w, ny, nx, oct=5, gain=.55):
    """Sømløs verdistøy i begge retninger, som farmbuildings/ground_decals.py."""
    out = np.zeros((h, w), np.float32)
    amp, tot = 1., 0.
    for i in range(oct):
        cy, cx = ny * 2 ** i, nx * 2 ** i
        g = rng.random((cy, cx), dtype=np.float32)
        g = np.pad(g, ((0, 1), (0, 1)), mode='wrap')
        ty = np.arange(h, dtype=np.float32) * cy / h
        tx = np.arange(w, dtype=np.float32) * cx / w
        iy, ix = ty.astype(int), tx.astype(int)
        fy, fx = ty - iy, tx - ix
        fy, fx = fy * fy * (3 - 2 * fy), fx * fx * (3 - 2 * fx)
        a = g[iy[:, None], ix[None, :]] * (1 - fx)[None, :] + g[iy[:, None], ix[None, :] + 1] * fx[None, :]
        b = g[iy[:, None] + 1, ix[None, :]] * (1 - fx)[None, :] + g[iy[:, None] + 1, ix[None, :] + 1] * fx[None, :]
        out += (a * (1 - fy)[:, None] + b * fy[:, None]) * amp
        tot += amp
        amp *= gain
    return out / tot


def equalize(a):
    """Jevn fordeling 0 til 1, så tersklene i materialet betyr en andel av flaten."""
    order = np.argsort(a, axis=None)
    out = np.empty(a.size, np.float32)
    out[order] = np.linspace(0, 1, a.size, dtype=np.float32)
    return out.reshape(a.shape)


def warp(a, dy, dx):
    """Forskyv oppslaget, med vrapping: ikke noe løper i rette streker."""
    h, w = a.shape
    yy, xx = np.mgrid[0:h, 0:w]
    return a[(yy + dy.astype(np.int32)) % h, (xx + dx.astype(np.int32)) % w]


def sstep(a, b, v):
    t = np.clip((v - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def tracks(rng, grain):
    """Hjulspor: et par spor med 1,55 m mellom, ut fra asfalten over skulderen, gjerne inn igjen. Hver side for seg."""
    out = np.zeros((H, W), np.float32)
    px_along, px_across = W / ALONG, H / ACROSS
    for side in (-1, 1):
        at = rng.uniform(0, ALONG)
        for _ in range(7):
            at += rng.uniform(14, 62)
            length = rng.uniform(18, 46)
            reach = rng.uniform(1.6, 6.5)
            back = rng.random() < .55      # Halvparten kjører inn på veien igjen; resten ender ute på bakken.
            start, width, strength = rng.uniform(4.3, 5.2), rng.uniform(.09, .15), rng.uniform(.55, 1)
            n = int(length * px_along)
            t = np.arange(n, dtype=np.float32) / n
            shape = np.sin(t * np.pi) if back else sstep(0, .75, t)
            sway = .12 * np.sin(t * 19 + rng.uniform(0, 6)) + .05 * np.sin(t * 47 + rng.uniform(0, 6))
            ends = sstep(0, .1, t) * (1 - sstep(.72 if not back else .9, 1, t))
            cols = (int(at * px_along) + np.arange(n)) % W
            for apart in (0., 1.55):
                centre = start + apart + reach * shape + sway
                rows = (side * centre + ACROSS / 2) * px_across
                half = int(width * 3 * px_across) + 1
                for k in range(n):
                    r0 = int(rows[k])
                    span = np.arange(r0 - half, r0 + half + 1)
                    d = np.abs(span - rows[k]) / px_across
                    v = (1 - sstep(width * .5, width * 1.6, d)) * ends[k] * strength
                    span %= H
                    out[span, cols[k]] = np.maximum(out[span, cols[k]], v)
    return out * (.55 + .9 * grain)


def build(rng):
    # U (along) er den lange aksen, så tungene strekkes av cellenes størrelse: 256/6 = 43 m langs og 32/7 = 4,6 m på tvers.
    field = noise(rng, H, W, 7, 6, 6, .5)
    # Vridd, så tungene går på skrå og ikke parallelt med veien.
    bend_a, bend_b = noise(rng, H, W, 3, 4, 3), noise(rng, H, W, 4, 5, 3)
    tongue = equalize(warp(field, (bend_a - .5) * 260, (bend_b - .5) * 900))
    patch = equalize(warp(noise(rng, H, W, 24, 64, 5, .58), (noise(rng, H, W, 12, 32, 2) - .5) * 60, (noise(rng, H, W, 12, 32, 2) - .5) * 80))
    grain = noise(rng, H, W, 68, 136, 2, .6)
    grain = equalize(grain)
    rut = tracks(rng, noise(rng, H, W, 40, 300, 2))
    return np.stack([tongue, patch, np.clip(rut, 0, 1), grain], axis=2)


def edge_rule(m, x, z, soil, under, dust=(1.13, 1.12, 1.1)):
    """EDGE_HLSL i numpy, for forhåndsvisningen: m er kartet, x og z meter, soil og under farger."""
    def tex(u, v):
        iy = (np.floor((v % 1) * H).astype(int)) % H
        ix = (np.floor((u % 1) * W).astype(int)) % W
        return m[iy, ix]
    ax = np.abs(x)
    side = np.where(x > 0, .31, .77)
    a = tex(z / 256, x / 32)
    b = tex(z / 83 + .37 + (a[..., 0] - .5) * .05, x / 21 + .11 + (a[..., 1] - .5) * .05)
    c = tex(z / 1900 + side, np.full_like(z, .27))
    d = tex(z / 5.3, x / 2.7)
    t = a[..., 0] + (b[..., 0] - a[..., 0]) * sstep(.35, .65, c[..., 1])
    width = .55 + .9 * c[..., 0]
    p = np.clip((a[..., 1] + b[..., 1]) * .5 * 1.6 - .3, 0, 1)

    fine = d[..., 1]
    reach = (.3 + 2.3 * t ** 2.2) * width
    d_in = ax - (4.8 - reach) + ((p - .5) * .8 + (fine - .5) * .35) * (.25 + .3 * reach)
    inner = np.clip(d_in / (.12 + .4 * reach), 0, 1)
    on_road = sstep(0, 1, inner) * (.25 + .7 * np.clip(d_in / np.maximum(reach, .05), 0, 1)) * sstep(.15, .6, p * .7 + fine * .3 + .35 * inner)
    flecks = sstep(.7, .88, a[..., 1] * .5 + b[..., 0] * .5) * sstep(2.7, 4.2, ax) * sstep(.35, .7, fine) * .4
    on_road = np.maximum(on_road, flecks)
    s = np.clip((ax - 4.8) / 1.2, 0, 1)
    shoulder = .68 + .32 * sstep(.2, .72, p + .45 * s)
    cover = on_road + (shoulder - on_road) * sstep(4.55, 5.05, ax + (p - .5) * .5)
    far = 6.6 + (1.2 + 7.5 * t) * width
    out = (1 - sstep(6.3, far, ax + (p - .5) * 1.6)) * (1 - sstep(14.6, 16.3, ax))
    opacity = cover * out

    amount = np.clip(1 - (ax - 6 + (p - .5) * 2.5) / (1 + 6.5 * t * width), 0, 1) * sstep(.15, .7, p + .25 * s)
    tone = 1 + (np.array(dust)[None, None, :] - 1) * amount[..., None]
    speck = sstep(.74, .86, d[..., 3]) * sstep(4.5, 5.4, ax) * (1 - sstep(6.5 + 3 * t, 8.5 + 4 * t, ax))
    gate = sstep(.35, .5, c[..., 2])
    ruts = a[..., 2] * gate
    colour = soil * tone * (1 - .24 * speck[..., None]) * (1 - .3 * ruts[..., None])
    opacity = np.maximum(opacity, ruts * .9 * sstep(4.2, 4.8, ax))
    return under + (colour - under) * opacity[..., None]


def preview(m, path, z0=900., length=120.):
    h, w = 660, 1200
    x, z = np.meshgrid(np.linspace(-16.5, 16.5, h), np.linspace(z0, z0 + length, w), indexing='ij')
    def lin(r, g, b):
        c = np.array([r, g, b], np.float32) / 255
        return np.where(c <= .04045, c / 12.92, ((c + .055) / 1.055) ** 2.4)
    ax = np.abs(x)
    under = np.where((ax < 4.8)[..., None], lin(88, 88, 90), np.where((ax < 6)[..., None], lin(205, 185, 160), lin(165, 130, 105)))
    line = np.abs(ax - 4.5) < .08
    under = np.where(line[..., None], lin(225, 225, 215), under)
    soil = lin(165, 130, 105) * (1 + .25 * (noise(np.random.default_rng(3), h, w, 60, 110, 3) - .5))[..., None]
    img = edge_rule(m, x, z, soil, under)
    img = np.clip(img, 0, 1)
    img = np.where(img <= .0031308, img * 12.92, 1.055 * np.power(img, 1 / 2.4) - .055)
    Image.fromarray((img * 255).astype(np.uint8), 'RGB').save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('out', type=Path)
    ap.add_argument('--preview', action='store_true')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    m = build(np.random.default_rng(4747))
    Image.fromarray((m * 255 + .5).astype(np.uint8), 'RGBA').save(args.out / 'T_RoadEdge_M.png')
    print('T_RoadEdge_M', m.shape, 'mean', m.reshape(-1, 4).mean(0).round(3), 'ruts cover', float((m[..., 2] > .3).mean()))
    if args.preview:
        for i, z0 in enumerate((900., 1400., 300.)):
            preview(m, args.out / ('preview_%d.png' % i), z0)


if __name__ == '__main__':
    main()
