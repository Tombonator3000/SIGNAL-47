"""EASTWING photo pass: RGBA grime textures for deferred decals (Blender Python: numpy + bpy image IO).
  blender -b --python-exit-code 1 --python make_grime.py
Each map: RGB = dirt colour (sRGB), A = coverage. Horizontal maps tile along U."""
from pathlib import Path
import numpy as np
import bpy
OUT = Path(__file__).resolve().parent / 'textures'

def periodic_noise(w, h, sx, sy, seed):
    fy = np.fft.fftfreq(h)[:, None]; fx = np.fft.fftfreq(w)[None, :]
    a = np.random.default_rng(seed).random((h, w)) - .5
    a = np.real(np.fft.ifft2(np.fft.fft2(a) * np.exp(-2 * np.pi ** 2 * ((sx * fx) ** 2 + (sy * fy) ** 2))))
    return (a / (np.abs(a).max() + 1e-6)).astype(np.float32)

def save(name, rgb, alpha):
    h, w = alpha.shape
    img = bpy.data.images.new(name, w, h, alpha=True); img.colorspace_settings.name = 'sRGB'
    px = np.concatenate([np.broadcast_to(rgb, (h, w, 3)), alpha[..., None]], -1).astype(np.float32)
    img.pixels.foreach_set(np.clip(px[::-1], 0, 1).ravel())
    img.filepath_raw = str(OUT / (name + '.png')); img.file_format = 'PNG'; img.alpha_mode = 'STRAIGHT'; img.save()

# 1. Wall base: kick marks, mop splash and dust, densest at the floor (bottom of the image), 2 m x 0.5 m.
W, H = 2048, 512
v = np.linspace(1, 0, H)[:, None]                       # 1 at bottom row (image top is v=1 after flip)
band = np.clip(1 - v, 0, 1) ** 2.2                      # strongest at floor
n1 = periodic_noise(W, H, 40, 12, 1); n2 = periodic_noise(W, H, 6, 6, 2)
alpha = np.clip((band * (.55 + .35 * n1) + .25 * np.clip(n2 - .35, 0, 1) * band ** .5) * 1.25, 0, .95)
save('T_EW_GrimeWallBase', np.array([.13, .11, .085]), alpha.astype(np.float32))  # darkest at the floor (Unreal maps texture top to decal +Z)

# 2. Vertical run-off streaks (under windows, sills, sign): 1 m x 1 m, streaks fade downward.
W = H = 1024
x = np.arange(W)
cols = np.zeros(W, np.float32)
rng = np.random.default_rng(3)
for _ in range(26):
    c = rng.integers(0, W); wdt = rng.integers(4, 22); s = rng.uniform(.3, 1)
    cols += s * np.exp(-((((x - c + W / 2) % W) - W / 2) / wdt) ** 2)
fade = np.linspace(0, 1, H)[:, None] ** .8               # top of the texture strongest (rows from top)
length = periodic_noise(W, 1, 30, 1, 4)[0] * .35 + .65
streak = np.clip(cols[None, :], 0, 1) * np.clip(1 - fade / length[None, :], 0, 1)
alpha = np.clip(streak * (.7 + .3 * periodic_noise(W, H, 3, 40, 5)), 0, 1) * .6
save('T_EW_GrimeStreaks', np.array([.18, .15, .12]), alpha.astype(np.float32))

# 3. Scuffs and hand smudges for door jambs and switch plates: 1 m x 1 m blotches.
n = periodic_noise(W, H, 30, 30, 6); m = periodic_noise(W, H, 5, 5, 7)
r = np.hypot(*np.meshgrid(np.linspace(-1, 1, W), np.linspace(-1, 1, H)))
alpha = np.clip((n - .05) * 1.6, 0, 1) * np.clip(1 - r, 0, 1) ** .6 * (.7 + .3 * m) * .55
save('T_EW_GrimeScuff', np.array([.17, .15, .12]), alpha.astype(np.float32))

# 4. Floor edge dust: settles along the wall line (top of texture = wall side), 2 m x 0.4 m.
W, H = 2048, 512
d = np.linspace(0, 1, H)[:, None]
n1 = periodic_noise(W, H, 25, 8, 8); n2 = periodic_noise(W, H, 3, 3, 9)
alpha = np.clip(((1 - d) ** 3 * (.6 + .4 * n1) + .15 * np.clip(n2, 0, 1) * (1 - d)) * 1.15, 0, .9)
save('T_EW_FloorEdgeDust', np.array([.33, .29, .22]), alpha.astype(np.float32))
print('GRIME OK', flush=True)
