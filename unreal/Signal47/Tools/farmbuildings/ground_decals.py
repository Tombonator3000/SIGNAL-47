"""Egne prosedyrale RGBA-kart til dekalene på bakken rundt gården. Ingen foto, ingen nedlastinger.

Kjøres med en Python som har numpy og Pillow (som textures.py):
  python3 ground_decals.py <utmappe>
RGB er albedo, A er dekningen. U går langs veien eller veggen (sømløst), V på tvers, og alt er symmetrisk på tvers,
så det ikke spiller noen rolle hvilken vei dekalen står. Målene står også i S47FarmWorld.cpp (DecalRoad, DecalBand).

Fargene er regnet fra spillets sol: en flate med albedo a leses omtrent som 2a lineært (Board BC 155 ble 202 i sol).
Mål i sol: veien (180,140,105), mørk jord ved veggene (105,70,50).
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('textures')
OUT.mkdir(parents=True, exist_ok=True)


def srgb(v):
    v = np.clip(v, 0, 1)
    return np.where(v <= .0031308, v * 12.92, 1.055 * np.power(v, 1 / 2.4) - .055)


def lin(r, g, b):
    c = np.array([r, g, b], dtype=np.float32) / 255
    return np.where(c <= .04045, c / 12.92, np.power((c + .055) / 1.055, 2.4))


def noise(rng, h, w, ny, nx, oct=4, gain=.55):
    """Sømløs verdistøy i begge retninger."""
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


def sstep(a, b, v):
    t = np.clip((v - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def save(name, rgb, alpha):
    a8 = np.clip(alpha, 0, 1)
    img = np.dstack([srgb(rgb) * 255, a8 * 255]).astype(np.uint8)
    Image.fromarray(img, 'RGBA').save(OUT / (name + '.png'))
    print(name, img.shape, 'mean sRGB where covered', [int(img[..., i][a8 > .5].mean()) for i in range(3)], 'coverage', round(float(a8.mean()), 3))


ROAD_L, ROAD_W = 16., 7.6    # Meter i ett kart: lengde langs U, bredde på tvers.
TRACK_APART, TRACK_W = 1.5, .18


def road_layers(rng):
    h, w = 1024, 2048
    u = np.arange(w, dtype=np.float32) / w * ROAD_L
    v = (np.arange(h, dtype=np.float32) / h - .5) * ROAD_W
    uu, vv = np.meshgrid(u, v)
    # Veikanten: 0,3 m bølge med periode 3 til 6 m, ulik på hver side, med en blanding på omtrent 0,8 m.
    tau = 2 * np.pi / ROAD_L
    wave_a = (.5 * np.sin(u * tau * 3 + .7) + .32 * np.sin(u * tau * 4 + 2.1) + .18 * np.sin(u * tau * 5 + 4.2)) * .3
    wave_b = (.5 * np.sin(u * tau * 4 + 3.3) + .32 * np.sin(u * tau * 3 + 5.0) + .18 * np.sin(u * tau * 5 + 1.2)) * .3
    edge = 2.55 + np.where(vv < 0, wave_a[None, :], wave_b[None, :])
    n_edge = noise(rng, h, w, 4, 8, 5)
    dist = np.abs(vv) + (n_edge - .5) * .55
    road_a = 1 - sstep(edge - .45, edge + .4, dist)
    # Sporene slynger seg litt, men holder 1,5 m mellom seg.
    sway = .07 * np.sin(u * tau * 2 + 1.) + .03 * np.sin(u * tau * 5 + 2.)
    d_track = np.abs(np.abs(vv - sway[None, :]) - TRACK_APART / 2)
    grain = noise(rng, h, w, 8, 2, 4)      # Strekt langs veien, som hjulene drar den.
    track = 1 - sstep(TRACK_W * .32, TRACK_W * .8, d_track + (grain - .5) * .06)
    mid = 1 - sstep(.28, .55, np.abs(vv - sway[None, :]))
    shoulder = sstep(1.0, 2.0, np.abs(vv))
    # Tonen: kompakte spor, lys midtstripe, støvete skuldre.
    base = lin(137, 106, 80)
    mott = noise(rng, h, w, 6, 12, 5)
    fine = noise(rng, h, w, 64, 128, 2)
    k = (.86 + .28 * mott) * (.9 + .2 * fine)
    k = k * (1 - .3 * track) * (1 + .1 * mid * (1 - track)) * (1 + .12 * shoulder)
    rgb = base[None, None, :] * k[..., None]
    # Småstein i det slitte.
    pebbles = sstep(.8, .86, noise(rng, h, w, 128, 256, 1)) * (1 - track * .8)
    rgb = rgb * (1 - .28 * pebbles[..., None])
    return uu, rgb, road_a, track


def main():
    rng = np.random.default_rng(47)
    uu, rgb, road_a, track = road_layers(rng)
    save('T_Farm_RoadDecal', rgb, road_a)
    # Veiens ende: hele veien de første 8 m, så to spor, så ingenting. Starten deler kant med vanlig vei.
    t = uu / ROAD_L
    whole = 1 - sstep(.05, .55, t)
    tracks = (1 - sstep(.62, 1., t)) * track
    save('T_Farm_RoadEnd', rgb, np.maximum(road_a * whole, tracks * .95))
    # Spor alene over tunet, svakt inn og ut i begge ender.
    ends = sstep(0, .22, t) * (1 - sstep(.78, 1., t))
    save('T_Farm_TrackDecal', rgb, track * ends * .85)

    # Båndet mot veggen: 1,8 m på tvers med veggen i midten. 4 m langs. Mørkest i kontakten, ujevn ytterkant.
    h, w = 256, 512
    v = (np.arange(h, dtype=np.float32) / h - .5) * 1.8
    u = np.arange(w, dtype=np.float32) / w * 4
    uu2, vv2 = np.meshgrid(u, v)
    tau = 2 * np.pi / 4
    n1 = noise(rng, h, w, 3, 6, 5)
    edge = .52 + .13 * np.sin(u[None, :] * tau + .5) + .07 * np.sin(u[None, :] * tau * 3 + 2.) + (n1 - .5) * .5
    dist = np.abs(vv2)
    alpha = 1 - sstep(edge - .22, edge + .16, dist)
    contact = 1 - sstep(.03, .3, dist)
    dust = sstep(.45, .7, noise(rng, h, w, 12, 24, 3))
    outer, inner = lin(82, 56, 40), lin(46, 33, 25)
    mix = contact[..., None]
    band = outer[None, None, :] * (1 - mix) + inner[None, None, :] * mix
    band = band * (1 + .3 * (dust * (1 - contact))[..., None]) * (.9 + .2 * noise(rng, h, w, 32, 64, 2))[..., None]
    save('T_Farm_GroundBand', band, alpha)

    # Slitt, rund flekk: kompakt felt foran dører, og mørk jord i hjørner via Tint.
    h = w = 1024
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    r = np.sqrt((x / w - .5) ** 2 + (y / h - .5) ** 2) * 2
    n2 = noise(rng, h, w, 3, 3, 5)
    alpha = 1 - sstep(.42, .95, r + (n2 - .5) * .5)
    base = lin(127, 97, 73)
    k = (.84 + .32 * noise(rng, h, w, 5, 5, 5)) * (.92 + .16 * noise(rng, h, w, 64, 64, 2))
    pebbles = sstep(.8, .86, noise(rng, h, w, 128, 128, 1))
    rgb2 = base[None, None, :] * k[..., None] * (1 - .25 * pebbles[..., None])
    save('T_Farm_WornField', rgb2, alpha)


main()
