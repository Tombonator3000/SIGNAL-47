"""EASTWING reference pass: tileable polished concrete (lab/storage/calibration), one texture = 2 m.
Matches the concept boards: light grey-beige, cloudy trowel mottling, fine aggregate, low gloss with
duller worn paths. Blender Python:  blender -b --python-exit-code 1 --python make_polished.py"""
from pathlib import Path
import numpy as np
import bpy
OUT = Path(__file__).resolve().parent / 'textures'
N = 2048
fy = np.fft.fftfreq(N)[:, None]; fx = np.fft.fftfreq(N)[None, :]
def noise(s, seed, ax=1.):
    a = np.random.default_rng(seed).random((N, N)) - .5
    a = np.real(np.fft.ifft2(np.fft.fft2(a) * np.exp(-2 * np.pi ** 2 * s ** 2 * ((fx * ax) ** 2 + fy ** 2))))
    return (a / (np.abs(a).max() + 1e-6)).astype(np.float32)
cloud = noise(140, 1); mid = noise(40, 2); fine = noise(1.1, 3); trowel = noise(10, 4, ax=.4)
base = np.array([.52, .49, .44], np.float32) * (1 + .10 * cloud + .06 * mid + .04 * fine + .03 * trowel)[..., None]
agg = np.clip((noise(.8, 5) - .5) * 5, 0, 1)
base = base * (1 - .1 * agg)[..., None] + .05 * np.clip((noise(.8, 6) - .58) * 6, 0, 1)[..., None]
stain = np.clip(noise(70, 7) - .25, 0, 1); base *= (1 - .18 * stain)[..., None]
wear = np.clip(noise(200, 8) + .2, 0, 1)
rough = np.clip(.26 + .18 * wear + .05 * fine + .1 * stain, 0, 1)
height = .02 * fine + .02 * trowel
def save(name, rgb, srgb):
    img = bpy.data.images.new(name, N, N, alpha=False); img.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    px = np.ones((N, N, 4), np.float32); px[..., :3] = np.clip(rgb, 0, 1)
    img.pixels.foreach_set(px[::-1].ravel()); img.filepath_raw = str(OUT / (name + '.png')); img.file_format = 'PNG'; img.save()
save('EW_PolishedConcrete_BaseColor', base, True)
save('EW_PolishedConcrete_Roughness', np.repeat(rough[..., None], 3, -1), False)
gy, gx = np.gradient(height); nx, ny, nz = -gx * 4, gy * 4, np.ones_like(height); l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
save('EW_PolishedConcrete_NormalGL', np.stack([nx / l, ny / l, nz / l], -1) * .5 + .5, False)
print('POLISHED OK', flush=True)
