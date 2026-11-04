"""EASTWING photo pass: tileable 12-inch vinyl composition tile (VCT) PBR set, 4 x 4 tiles = 1.2192 m.
Run with Blender's Python (numpy + bpy image IO), deterministic:
  blender -b --python-exit-code 1 --python make_vct.py
Writes textures/EW_VCT_{BaseColor (sRGB), Roughness, NormalGL} .png, 2048 px."""
from pathlib import Path
import numpy as np
import bpy
OUT = Path(__file__).resolve().parent / 'textures'
N, T = 2048, 512
rng = np.random.default_rng(1986)
fy = np.fft.fftfreq(N)[:, None]; fx = np.fft.fftfreq(N)[None, :]

def blur(a, sigma):
    """Periodic gaussian blur (FFT), so every map tiles."""
    k = np.exp(-2 * (np.pi * sigma) ** 2 * (fx ** 2 + fy ** 2))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * k)).astype(np.float32)

def noise(sigma, seed, aniso=1.):
    a = np.random.default_rng(seed).random((N, N)).astype(np.float32)
    k = np.exp(-2 * (np.pi * sigma) ** 2 * (fx ** 2 * aniso ** 2 + fy ** 2))
    a = np.real(np.fft.ifft2(np.fft.fft2(a - .5) * k))
    return (a / (np.abs(a).max() + 1e-6)).astype(np.float32)   # roughly -1..1

base = np.zeros((N, N, 3), np.float32)
cream = np.array([.64, .615, .55], np.float32); grey = np.array([.58, .57, .53], np.float32)
for ty in range(4):
    for tx in range(4):
        c = cream * (1. if (tx + ty) % 2 == 0 else .982)
        if (tx, ty) == (2, 1): c = grey
        base[ty * T:(ty + 1) * T, tx * T:(tx + 1) * T] = c * (1 + rng.normal(0, .012)) * (1 + rng.normal(0, .003, 3))
flecks = noise(1.5, 3, aniso=.25)                    # streaky chips along one axis (calendered vinyl)
fine = noise(.8, 5)
chip_d = np.clip((noise(.7, 7) - .30) * 4, 0, 1); chip_l = np.clip((noise(.7, 9) - .38) * 4, 0, 1)
base *= (1 + .07 * flecks + .03 * fine)[..., None]
base = base * (1 - .30 * chip_d)[..., None] + (.07 * chip_l)[..., None]
wear = noise(90, 11)
base *= (1 - .06 * wear)[..., None]
scuff = np.zeros((N, N), np.float32)
for _ in range(26):  # rubber heel marks: short curved strokes, soft edges
    x, y = rng.integers(0, N, 2); L = int(rng.integers(25, 70)); a = rng.uniform(0, np.pi); bend = rng.uniform(-.03, .03)
    for i in range(L * 2):
        a += bend; x += np.cos(a) * .5; y += np.sin(a) * .5
        scuff[int(y) % N, int(x) % N] = 1
scuff = np.clip(blur(scuff, 2.6) * 5, 0, 1)
base *= (1 - .32 * scuff)[..., None]
seam = np.zeros((N, N), np.float32)
for k in range(4):
    p = k * T
    seam[:, [(p + d) % N for d in (-2, -1, 0, 1)]] = 1; seam[[(p + d) % N for d in (-2, -1, 0, 1)], :] = 1
seam = np.clip(blur(seam, 1.2), 0, 1)
base *= (1 - .32 * seam)[..., None]
rough = np.clip(.32 + .12 * np.clip(wear, 0, 1) + .04 * fine + .4 * seam + .2 * scuff, 0, 1)
h = -seam * .8 + .02 * fine
gy, gx = np.gradient(h)
nx, ny, nz = -gx * 6, gy * 6, np.ones_like(h)
l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
nrm = np.stack([nx / l, ny / l, nz / l], -1) * .5 + .5

def save(name, rgb, srgb):
    img = bpy.data.images.new(name, N, N, alpha=False, float_buffer=False)
    img.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    px = np.ones((N, N, 4), np.float32); px[..., :3] = rgb
    img.pixels.foreach_set(px[::-1].ravel())         # Blender rows start at the bottom
    img.filepath_raw = str(OUT / (name + '.png')); img.file_format = 'PNG'; img.save()

save('EW_VCT_BaseColor', np.clip(base, 0, 1), True)    # linear values written into an sRGB-tagged 8-bit image
save('EW_VCT_Roughness', np.repeat(rough[..., None], 3, -1), False)
save('EW_VCT_NormalGL', nrm, False)
print('VCT OK', flush=True)
