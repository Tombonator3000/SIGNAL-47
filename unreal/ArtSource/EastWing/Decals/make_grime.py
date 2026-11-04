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

# 1. Wall base: mop-water tide line, isotropic splash speckle and short kick marks, densest at the floor.
#    2 m x 0.5 m; built with row 0 at the floor edge, then flipped so the PNG bottom is the floor.
W, H = 2048, 512
v = np.linspace(0, 1, H)[:, None]                        # 0 at the floor edge
grad = np.clip(1 - v / .45, 0, 1) ** 1.8                 # fades out by ~0.22 m
tide = np.exp(-((v - .1 - .02 * periodic_noise(W, 1, 60, 1, 10)[0][None, :]) / .025) ** 2) * .35
spk = np.clip((periodic_noise(W, H, 2.2, 2.2, 11) - .25) * 3, 0, 1) * np.clip(1 - v / .7, 0, 1)
blot = np.clip(periodic_noise(W, H, 14, 14, 12) * .8 + .2, 0, 1)
kicks = np.zeros((H, W), np.float32); rng = np.random.default_rng(13)
for _ in range(18):                                      # rubber kick marks 3-18 cm up, short arcs
    x, y = rng.uniform(0, W), rng.uniform(.03, .18) * H / .5 * .5 * 2; a = rng.uniform(-.5, .5); L = int(rng.integers(20, 70))
    for i in range(L):
        a += rng.uniform(-.05, .05); x += np.cos(a); y += np.sin(a) * .5
        kicks[int(y) % H, int(x) % W] = 1
fyk = np.fft.fftfreq(H)[:, None]; fxk = np.fft.fftfreq(W)[None, :]
kicks = np.clip(np.real(np.fft.ifft2(np.fft.fft2(kicks) * np.exp(-2 * np.pi ** 2 * (2.5 ** 2) * (fxk ** 2 + fyk ** 2)))) * 6, 0, 1)
alpha = np.clip(grad * (.45 + .35 * blot) + tide * blot + .25 * spk + .3 * kicks, 0, .9)
# Flip so the floor edge is the bottom of the PNG (Unreal maps texture top to decal +Z).
save('T_EW_GrimeWallBase', np.array([.13, .11, .085]), alpha[::-1].astype(np.float32))

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
alpha = np.clip(streak * (.7 + .3 * periodic_noise(W, H, 3, 40, 5)) * .85, 0, .85)
save('T_EW_GrimeStreaks', np.array([.13, .11, .085]), alpha.astype(np.float32))

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
