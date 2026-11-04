"""EASTWING photo pass: tileable PBR sets for Claude's Extras props (Blender Python: numpy + bpy IO).
  blender -b --python-exit-code 1 --python make_materials.py
Each set: <name>_BaseColor (sRGB), _Roughness, _NormalGL. One texture spans 0.5 m (see build_extras SPAN)."""
from pathlib import Path
import numpy as np
import bpy
OUT = Path(__file__).resolve().parent / 'textures'
N = 1024
fy = np.fft.fftfreq(N)[:, None]; fx = np.fft.fftfreq(N)[None, :]

def noise(sx, sy, seed):
    a = np.random.default_rng(seed).random((N, N)) - .5
    a = np.real(np.fft.ifft2(np.fft.fft2(a) * np.exp(-2 * np.pi ** 2 * ((sx * fx) ** 2 + (sy * fy) ** 2))))
    return (a / (np.abs(a).max() + 1e-6)).astype(np.float32)

def normal_from(h, k):
    gy, gx = np.gradient(h); nx, ny, nz = -gx * k, gy * k, np.ones_like(h)
    l = np.sqrt(nx ** 2 + ny ** 2 + nz ** 2)
    return np.stack([nx / l, ny / l, nz / l], -1) * .5 + .5

def save(name, rgb, srgb):
    img = bpy.data.images.new(name, N, N, alpha=False); img.colorspace_settings.name = 'sRGB' if srgb else 'Non-Color'
    px = np.ones((N, N, 4), np.float32); px[..., :3] = np.clip(rgb, 0, 1)
    img.pixels.foreach_set(px[::-1].ravel()); img.filepath_raw = str(OUT / (name + '.png')); img.file_format = 'PNG'; img.save()

def write(name, base, rough, height, k):
    save(name + '_BaseColor', base, True)
    save(name + '_Roughness', np.repeat(np.clip(rough, 0, 1)[..., None], 3, -1), False)
    save(name + '_NormalGL', normal_from(height, k), False)

def worn_paint(name, colour, under, rough, seed, chips=.6, dirt=.12):
    """Enamel over steel: colour mottling, chipped spots showing primer/metal, fine dirt."""
    c = np.array(colour, np.float32); u = np.array(under, np.float32)
    mott = noise(40, 40, seed); fine = noise(1.2, 1.2, seed + 1); grime = noise(120, 120, seed + 2)
    chip = np.clip((noise(6, 6, seed + 3) - (1 - chips * .5)) * 8, 0, 1)
    base = c * (1 + .06 * mott[..., None] + .03 * fine[..., None])
    base = base * (1 - chip[..., None]) + u * chip[..., None]
    base *= (1 - dirt * np.clip(grime, 0, 1))[..., None]
    r = rough + .08 * mott + .25 * chip + .06 * np.clip(grime, 0, 1)
    h = .02 * fine - .6 * chip
    write(name, base, r, h, 4)

def wood(name, colour, seed):
    """Laminated maple/beech bench top: long grain along U, rings, oiled and worn."""
    c = np.array(colour, np.float32)
    grain = noise(.6, 30, seed); rings = np.sin((np.arange(N)[:, None] / N * 2 * np.pi * 22) + 6 * noise(4, 60, seed + 1))
    strips = np.repeat(np.random.default_rng(seed).normal(0, .05, 9), N // 9 + 1)[:N][:, None]
    base = c * (1 + .12 * grain[..., None] + .07 * rings[..., None] + strips[..., None])
    stain = np.clip(noise(80, 80, seed + 2), 0, 1); base *= (1 - .22 * stain)[..., None]
    r = .55 + .1 * grain + .15 * stain
    write(name, base, r, .3 * grain + .1 * rings, 3)

def rubber(name, seed):
    n = noise(1.5, 1.5, seed); g = noise(60, 60, seed + 1)
    write(name, np.full((N, N, 3), .035, np.float32) * (1 + .15 * n[..., None] + .1 * g[..., None]), .75 + .1 * n, .2 * n, 3)

wood('EW_XWood', (.52, .37, .21), 11)
worn_paint('EW_XEnamelRed', (.44, .035, .025), (.32, .30, .27), .38, 21, chips=.7)
worn_paint('EW_XEnamelWhite', (.76, .75, .70), (.30, .29, .27), .32, 31, chips=.35, dirt=.18)
worn_paint('EW_XOliveDrab', (.16, .17, .085), (.22, .14, .07), .55, 41, chips=.8, dirt=.2)
worn_paint('EW_XInstrumentGrey', (.31, .32, .30), (.45, .46, .45), .45, 51, chips=.25)
rubber('EW_XRubber', 61)
print('MATERIALS OK', flush=True)
