"""EASTWING photo pass 3: unique (non-tiling) garage slab, x31.675..38.35, z-5.25..5.25 (6.675 x 10.5 m).
Blender Python (numpy + bpy IO):  blender -b --python-exit-code 1 --python make_garage_floor.py
Array row 0 = south (roll-up door, z+5.25), column 0 = west wall (x31.675); save() flips rows so UV v=0 is south.
Paints: trowelled concrete, saw-cut joints, hairline cracks, the two worn yellow lane lines (x33.6, x37.2 from
z-4.25), Jeep tyre tracks, soaked oil under the parked engine, sand blown in at the door, dirt along walls."""
from pathlib import Path
import numpy as np
import bpy
OUT = Path(__file__).resolve().parent / 'textures'
W, H = 2048, 4096
X0, X1, Z0, Z1 = 31.675, 38.35, -5.25, 5.25
xs = X0 + (np.arange(W) + .5) / W * (X1 - X0); zs = Z1 - (np.arange(H) + .5) / H * (Z1 - Z0)
X, Z = np.meshgrid(xs, zs)
rng = np.random.default_rng(1986)
fy = np.fft.fftfreq(H)[:, None]; fx = np.fft.fftfreq(W)[None, :]

def noise(s, seed, ax=1.):
    a = np.random.default_rng(seed).random((H, W)) - .5
    a = np.real(np.fft.ifft2(np.fft.fft2(a) * np.exp(-2 * np.pi ** 2 * s ** 2 * ((fx * ax) ** 2 + fy ** 2))))
    return (a / (np.abs(a).max() + 1e-6)).astype(np.float32)

def blur(a, s):
    return np.real(np.fft.ifft2(np.fft.fft2(a) * np.exp(-2 * np.pi ** 2 * s ** 2 * (fx ** 2 + fy ** 2)))).astype(np.float32)

big = noise(160, 1); mid = noise(30, 2); fine = noise(1.2, 3); swirl = noise(8, 4, ax=.35)
base = np.array([.40, .39, .36], np.float32) * (1 + .05 * big + .04 * mid + .05 * fine + .03 * swirl)[..., None]
agg = np.clip((noise(.9, 5) - .55) * 6, 0, 1); base = base * (1 - .12 * agg)[..., None] + .05 * np.clip((noise(.9, 6) - .6) * 6, 0, 1)[..., None]
rough = .45 + .08 * mid + .05 * fine  # reference: sealed, slightly glossy garage slab
height = .02 * fine + .03 * swirl
# Saw-cut control joints every ~3.3 m along z and at mid-width in x.
cut = np.zeros((H, W), np.float32)
for zc in (-1.75, 1.75):
    cut += np.exp(-((Z - zc) / .004) ** 2)
cut += np.exp(-((X - 35.0) / .004) ** 2)
cut = np.clip(cut, 0, 1); cut_dirt = np.clip(blur(cut, 3) * 3, 0, 1)
base *= (1 - .5 * cut - .12 * cut_dirt)[..., None]; rough += .1 * cut; height -= .5 * cut
# Hairline cracks: random walks from the joints and corners.
crack = np.zeros((H, W), np.float32)
for _ in range(9):
    r, c = rng.integers(0, H), rng.integers(0, W); a = rng.uniform(0, 2 * np.pi)
    for i in range(int(rng.integers(300, 900))):
        a += rng.normal(0, .18); r = int(r + np.sin(a) * 1.5) % H; c = int(c + np.cos(a) * 1.5) % W; crack[r, c] = 1
crack = np.clip(blur(crack, 1.6) * 1.6, 0, 1)
base *= (1 - .16 * crack)[..., None]; height -= .25 * crack
# Yellow lane lines (0.10 m), painted, worn where tyres cross and at random chips.
line = np.zeros((H, W), np.float32)
for xc in (33.6, 37.2):
    line = np.maximum(line, ((np.abs(X - xc) < .05) & (Z > Z0 + 1.0)).astype(np.float32))
line = blur(line, 1.2)
wear = np.clip((noise(3, 7) + .35) * 2, 0, 1) * np.clip(1 - np.exp(-((Z - 4.6) / .9) ** 2) * .6, 0, 1)
paint = line * wear
yellow = np.array([.62, .45, .06], np.float32)
base = base * (1 - paint[..., None]) + yellow * paint[..., None] * (1 + .06 * fine[..., None])
rough = rough * (1 - paint) + .5 * paint; height += .15 * paint
# Jeep tyre tracks: two 0.25 m bands 1.5 m apart along the lane centre, fading inward from the door.
track = np.zeros((H, W), np.float32)
for xc in (35.4 - .76, 35.4 + .76):
    track += np.exp(-((X - xc - .05 * noise(60, 8)) / .13) ** 4)
track *= np.clip((Z + 3.2) / 3, 0, 1) * (.6 + .4 * np.clip(noise(20, 9) + .5, 0, 1))
base *= (1 - .18 * track)[..., None]; rough -= .12 * track
# Soaked oil under the engine (parked nose north), plus drips.
oil = np.exp(-(((X - 35.35) / .55) ** 2 + ((Z + 2.2) / .75) ** 2)) * np.clip(noise(12, 10) * .9 + .7, 0, 1)
for _ in range(14):
    cx, cz, r = 35.4 + rng.normal(0, .5), -2.0 + rng.normal(0, 1.2), rng.uniform(.03, .12)
    oil = np.maximum(oil, np.exp(-(((X - cx) ** 2 + (Z - cz) ** 2) / r ** 2)) * rng.uniform(.4, .9))
oil = np.clip(oil, 0, 1)
base = base * (1 - .55 * oil)[..., None] + np.array([.03, .028, .024]) * (.4 * oil)[..., None]; rough -= .35 * oil
# Desert sand blown in at the roll-up door, and darker dirt along the walls.
sand = np.clip((Z - 3.2) / 2, 0, 1) ** 1.5 * np.clip(noise(25, 11) * .7 + .6, 0, 1) * (np.abs(X - 35.4) < 2.4)
base = base * (1 - .45 * sand)[..., None] + np.array([.55, .45, .32]) * (.45 * sand)[..., None]; rough += .1 * sand
edge = np.exp(-np.minimum.reduce([X - X0, X1 - X, Z - Z0, Z1 - Z]) / .25)
base *= (1 - .25 * edge * (.6 + .4 * np.clip(noise(10, 12) + .5, 0, 1)))[..., None]

def save(name, rgb, srgb):
    img = bpy.data.images.new(name, W, H, alpha=False); img.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    px = np.ones((H, W, 4), np.float32); px[..., :3] = np.clip(rgb, 0, 1)
    img.pixels.foreach_set(px[::-1].ravel()); img.filepath_raw = str(OUT / (name + '.png')); img.file_format = 'PNG'; img.save()

save('EW_GarageSlab_BaseColor', base, True)
save('EW_GarageSlab_Roughness', np.repeat(np.clip(rough, 0, 1)[..., None], 3, -1), False)
gy, gx = np.gradient(height); nx, ny, nz = -gx * 5, gy * 5, np.ones_like(height); l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
save('EW_GarageSlab_NormalGL', np.stack([nx / l, ny / l, nz / l], -1) * .5 + .5, False)
print('GARAGE OK', flush=True)
