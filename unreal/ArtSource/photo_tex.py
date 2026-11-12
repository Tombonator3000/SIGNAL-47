#!/usr/bin/env python3
"""Photo input for the buildings' painted texture generators (TEXTURES-PHOTO-20261112, Tom 12 Nov 2026).

Tom's ChatGPT base-colour pictures (32 of them, ~/scratch-bm/refs/textures-20261112, never in Git; no Cosmos picture is
ever one of them) replace a generator's painted albedo where one is listed here. The gas station's gs_photo.py made general:
  - the part of the picture that is whole blocks or tiles is cut out (crop), so that it repeats in register;
  - an axis that still does not tile is cross-faded with itself shifted half a tile (a whole number of blocks when `grid` says so);
  - the picture is repeated to the span the mesh's UVs already use, so no mesh or material changes: only the textures do;
  - its colour can be pulled to the old painted texture's mean (materials that tint one shared texture keep their tints);
  - roughness and height are read from its own light and shade.
A generator asks surface(key, old=...) and keeps its painted texture when that returns None (no entry, or the picture is absent).

numpy only; Pillow or Blender's bpy reads the picture, whichever is there. Outside Blender:
  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 photo_tex.py saro          (the SARO textures that have no generator hook)
  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 photo_tex.py check         (what each picture's seams and grid look like)
S47_PHOTO_DIR moves the picture folder; S47_PHOTO_OFF=1 gives the painted textures again.
"""
import math, os, struct, sys, zlib
from pathlib import Path
import numpy as np

HERE = Path(__file__).resolve().parent
PHOTO_DIR = Path(os.path.expanduser(os.environ.get('S47_PHOTO_DIR', '~/scratch-bm/refs/textures-20261112')))
LUMA = np.array([0.2126, 0.7152, 0.0722], np.float32)

# key -> file: the picture under PHOTO_DIR
#        crop: (x0, x1, y0, y1) pixels of it that are whole blocks/tiles (None: all of it); span: metres (u, v) that part covers
#        uv:   metres (u, v) the mesh's UVs repeat the texture over;  size: pixels of the texture (the import's largest size too)
#        grid: (nu, nv) pattern repeats in the cropped part: the seam fade shifts by whole repeats (None: half the picture)
#        match: how far its mean colour is pulled to the old painted texture's (0 the picture's own, 1 the old one's)
#        rough: (lo, hi) roughness from bright to dark; relief: metres of height per standard deviation of fine shade;
#        grain: how much of the pixel-sized shade goes into the relief (1 all of it; blocks little: their joints carry it)
#        mosaic: (n_photo, n_out, inset) for a floor or ceiling whose joints are geometry: n_out x n_out tile faces without joints
SURFACES = {
    # SARO outside, /Game/Lived/Saro (lived_tex.py): 40 x 20 cm block, the skin's UVs are 1.6 m a tile: 4 blocks by 8 courses
    # of the picture's 6 by 11.5. One texture, tinted per wall (BlockSaro, BlockWing): its colour stays the painted one's.
    'lived_block': dict(file='saro/block_ext.png', crop=(2, 1361, 6, 1418), span=(1.6, 1.6), uv=(1.6, 1.6), size=2048,
                        grid=(4, 4), match=1.0, rough=(0.78, 0.96), relief=0.0045, grain=0.12),
    # East wing surfaces, /Game/EastWing/Props/Surfaces (EastWing/Props/Textures/build_textures.py).
    # The picture's blocks are 4 by 11 over 2.4 m; 3 blocks by 6 courses twice over gives the 6 by 12 the UVs were made for.
    'EW_CinderblockCream': dict(file='saro/block_int.png', crop=(7, 1538, 6, 1113), span=(1.2, 1.2), uv=(2.4, 2.4), size=2048,
                                grid=(3, 3), match=0.0, rough=(0.70, 0.86), relief=0.0030, grain=0.12),
    'EW_Concrete': dict(file='saro/concrete_floor.png', crop=None, span=(4.0, 4.0), uv=(4.0, 4.0), size=2048,
                        grid=None, match=0.0, rough=(0.62, 0.93), relief=0.00025),
    'EW_GreenSteel': dict(file='saro/enamel_green.png', crop=None, span=(1.0, 1.0), uv=(1.0, 1.0), size=1024,
                          grid=None, match=0.7, rough=(0.32, 0.62), relief=0.00004),
    'EW_Galvanized': dict(file='saro/galvanized.png', crop=None, span=(1.0, 1.0), uv=(1.0, 1.0), size=1024,
                          grid=None, match=0.0, rough=(0.36, 0.58), relief=0.000004),
    # The office set's 1 m cube-projected finishes, /Game/Signal47/Art/Office (Office/build_office.py), which the control
    # room's and the corridor's finishes tint (SaroInteriors): their colour stays the painted one's.
    'M_Office_SageWall': dict(file='saro/wall_panel.png', crop=(512, 1536, 512, 1536), span=(1.0, 1.0), uv=(1.0, 1.0), size=1024,
                              grid=None, match=1.0, rough=(0.84, 0.95), relief=0.00002),
    # Ceiling panels and floor tiles are real geometry there (T-bars, 30 cm quads): the texture is tile faces without joints.
    'M_Office_CeilingTile': dict(file='saro/ceiling_tile.png', mosaic=(2, 2, 0.05), span=(1.2, 1.2), uv=(1.0, 1.0), size=1024,
                                 grid=None, match=1.0, rough=(0.93, 0.99), relief=0.00006),
    'M_Office_FloorLinoleum': dict(file='saro/vct_floor.png', mosaic=(4, 3, 0.05), span=(1.2, 1.2), uv=(1.0, 1.0), size=1024,
                                   grid=None, match=1.0, rough=(0.45, 0.75), relief=0.00001),
}
# Kept painted, on purpose (12 Nov): M_Lived_VCTFloor (vct_floor.py). The control room's floor is one picture of the whole
# 12 x 9 m room with its walked lanes, caster arcs, burns and coffee; a repeating photo would lose all of it.


def srgb2lin(c):
    c = np.asarray(c, np.float32)
    return np.where(c <= .04045, c / 12.92, ((c + .055) / 1.055) ** 2.4)


def lin2srgb(c):
    c = np.asarray(c, np.float32)
    return np.where(c <= .0031308, c * 12.92, 1.055 * np.power(np.maximum(c, 1e-7), 1 / 2.4) - .055)


def load(path):
    """sRGB values 0..1, H x W x 3, row 0 at the top."""
    try:
        from PIL import Image
        return np.asarray(Image.open(path).convert('RGB'), np.float32) / 255
    except ImportError:
        import bpy
        img = bpy.data.images.load(str(path), check_existing=False)
        w, h = img.size
        a = np.array(img.pixels[:], np.float32).reshape(h, w, 4)[::-1, :, :3].copy()
        bpy.data.images.remove(img)
        return a


def save(path, a):
    """8-bit PNG, grey or RGB, without Pillow."""
    a = (np.clip(a, 0, 1) * 255 + .5).astype(np.uint8)
    h, w = a.shape[:2]
    c = 1 if a.ndim == 2 else a.shape[2]
    raw = np.concatenate([np.zeros((h, 1), np.uint8), a.reshape(h, w * c)], 1).tobytes()
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    Path(path).write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, {1: 0, 3: 2}[c], 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def blur(a, sigma):
    """Gaussian, wrapping (the textures tile)."""
    if sigma <= 0:
        return a
    fy = np.fft.fftfreq(a.shape[0])[:, None]
    fx = np.fft.rfftfreq(a.shape[1])[None, :]
    g = np.exp(-2 * math.pi ** 2 * sigma ** 2 * (fx * fx + fy * fy))
    if a.ndim == 2:
        return np.fft.irfft2(np.fft.rfft2(a) * g, s=a.shape).astype(np.float32)
    return np.stack([blur(a[..., k], sigma) for k in range(a.shape[2])], -1)


def resample(a, H, W):
    """Bilinear, wrapping, with a blur first when it shrinks."""
    sy, sx = a.shape[0] / H, a.shape[1] / W
    s = max(sy, sx)
    if s > 1.05:
        a = blur(a, .45 * (s - 1) + .3)
    y = (np.arange(H) + .5) * sy - .5
    x = (np.arange(W) + .5) * sx - .5
    y0 = np.floor(y).astype(np.int64); x0 = np.floor(x).astype(np.int64)
    fy = (y - y0).astype(np.float32)[:, None, None]; fx = (x - x0).astype(np.float32)[None, :, None]
    y1 = (y0 + 1) % a.shape[0]; x1 = (x0 + 1) % a.shape[1]; y0 %= a.shape[0]; x0 %= a.shape[1]
    top = a[y0][:, x0] * (1 - fx) + a[y0][:, x1] * fx
    bot = a[y1][:, x0] * (1 - fx) + a[y1][:, x1] * fx
    return (top * (1 - fy) + bot * fy).astype(np.float32)


def seam_ratio(a, axis):
    """How much the picture jumps across its own wrap on an axis, against its ordinary neighbour step (~1: it tiles already)."""
    e = np.abs(a[0] - a[-1]).mean() if axis == 0 else np.abs(a[:, 0] - a[:, -1]).mean()
    return float(e / (np.abs(np.diff(a, axis=axis)).mean() + 1e-6))


def _fade(a, axis, repeats=None):
    n = a.shape[axis]
    shift = n // 2 if not repeats or repeats < 2 else int(round(n * (repeats // 2) / repeats))
    b = np.roll(a, shift, axis)
    m = np.clip((1 - np.abs(np.linspace(-1, 1, n, dtype=np.float32))) * 3.0, 0, 1)   # 1 in the middle, 0 at the wrap
    m = m * m * (3 - 2 * m)
    m = m[:, None, None] if axis == 0 else m[None, :, None]
    return a * m + b * (1 - m)


def seamless(a, grid=None, u=True, v=True, limit=1.8):
    """Cross-fade with itself shifted half a tile, only on an axis where the picture does not already tile.
    grid (nu, nv): the shift is a whole number of the pattern's repeats, so blocks, planks and panels stay in register."""
    if u and seam_ratio(a, 1) > limit:
        a = _fade(a, 1, grid[0] if grid else None)
    if v and seam_ratio(a, 0) > limit:
        a = _fade(a, 0, grid[1] if grid else None)
    return a


def match_mean(a, old, amount=1.0):
    """Pull the picture's mean colour towards the old texture's (per channel, in linear light); its detail stays."""
    if amount <= 0 or old is None:
        return a
    la = srgb2lin(a)
    want = srgb2lin(np.asarray(old, np.float32)[..., :3]).reshape(-1, 3).mean(0)
    have = la.reshape(-1, 3).mean(0)
    gain = 1 + amount * (want / np.maximum(have, 1e-4) - 1)
    return lin2srgb(np.clip(la * gain, 0, 1))


def mosaic(a, n_photo, n_out, inset, size):
    """n_out x n_out tile faces cut from inside the joints of a picture of n_photo x n_photo tiles, each brought to the
    picture's mean and feathered into its neighbours (wrapping): a floor or a ceiling whose joints are geometry."""
    H, W = a.shape[:2]
    cell = size // n_out
    pad = cell // 8
    out = np.zeros((size, size, 3), np.float32); wsum = np.zeros((size, size, 1), np.float32)
    mean = a.reshape(-1, 3).mean(0)
    w1 = np.clip(np.minimum(np.arange(cell + 2 * pad) + .5, cell + 2 * pad - np.arange(cell + 2 * pad) - .5) / (2 * pad), 0, 1)
    w1 = (w1 * w1 * (3 - 2 * w1)).astype(np.float32)
    win = (w1[:, None] * w1[None, :])[..., None]
    tiles = [(i, j) for j in range(n_photo) for i in range(n_photo)]
    rng = np.random.default_rng(47); rng.shuffle(tiles)
    for k in range(n_out * n_out):
        i, j = tiles[k % len(tiles)]
        x0 = int((i + inset) * W / n_photo); x1 = int((i + 1 - inset) * W / n_photo)
        y0 = int((j + inset) * H / n_photo); y1 = int((j + 1 - inset) * H / n_photo)
        t = a[y0:y1, x0:x1]
        t = t - blur_plain(t, t.shape[0] / 3) + mean      # no slow shade across a face: faces meet evenly
        t = resample_plain(t, cell + 2 * pad, cell + 2 * pad)
        oy = (k // n_out) * cell - pad; ox = (k % n_out) * cell - pad
        ys = np.arange(oy, oy + cell + 2 * pad) % size; xs = np.arange(ox, ox + cell + 2 * pad) % size
        out[np.ix_(ys, xs)] += t * win; wsum[np.ix_(ys, xs)] += win
    return np.clip(out / np.maximum(wsum, 1e-4), 0, 1)


def blur_plain(a, sigma):
    """Gaussian of a patch that does not wrap (mirrored first)."""
    b = np.concatenate([a, a[::-1]], 0); b = np.concatenate([b, b[:, ::-1]], 1)
    return blur(b, sigma)[:a.shape[0], :a.shape[1]]


def resample_plain(a, H, W):
    b = np.concatenate([a, a[::-1]], 0); b = np.concatenate([b, b[:, ::-1]], 1)   # mirrored: the wrap in resample() meets itself
    return resample(b, 2 * H, 2 * W)[:H, :W]


def maps(a, rough=(0.6, 0.9), fine=6.0, grain=1.0):
    """(roughness, height) from the albedo's own light and shade: darker and dirtier is rougher; grooves, joints and pits
    are dark against their surroundings, so low. Height is in standard deviations of that fine shade.
    grain < 1 holds back the pixel-sized part of it (an upscaled picture's noise), so that joints carry the relief."""
    lum = a @ LUMA
    soft = blur(lum, 1.6) if grain < 1 else lum
    hgt = ((soft - blur(lum, fine)) + grain * (lum - soft)) / (lum.std() + 1e-6)
    lo, hi = np.percentile(lum, 1), np.percentile(lum, 99)
    ln = np.clip((lum - lo) / (hi - lo + 1e-6), 0, 1)
    return (rough[1] - (rough[1] - rough[0]) * ln).astype(np.float32), hgt.astype(np.float32)


def normal(h, span, green=1.0):
    """Tangent normal map from a height in metres over `span` metres (u, v); rows run down the picture.
    green +1: (-dh/du, +dh/drow) as build_textures.py and build_office.py write it; -1 as lived_tex.py does."""
    H, W = h.shape
    dx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * W / (2 * span[0])
    dy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * H / (2 * span[1])
    inv = 1 / np.sqrt(dx * dx + dy * dy + 1)
    return np.stack((-dx * inv, green * dy * inv, inv), -1) * .5 + .5


def surface(key, old=None, size=None, green=1.0):
    """The textures for `key` from its picture: {'base' sRGB HxWx3, 'rough', 'height' (metres), 'normal', 'span', 'file'},
    or None when there is none (the caller keeps its painted texture). old: the painted albedo, for the colour match."""
    s = SURFACES.get(key)
    if s is None or os.environ.get('S47_PHOTO_OFF'):
        return None
    p = PHOTO_DIR / s['file']
    if not p.is_file():
        print('S47PHOTO', key, 'no picture at', p, '- painted texture kept', flush=True)
        return None
    a = load(p)
    N = size or s['size']
    if s.get('mosaic'):
        a = mosaic(a, *s['mosaic'], N)
        reps = (1, 1)
    else:
        if s.get('crop'):
            x0, x1, y0, y1 = s['crop']
            a = a[y0:y1, x0:x1]
        a = seamless(a, s.get('grid'))
        reps = (max(1, round(s['uv'][0] / s['span'][0])), max(1, round(s['uv'][1] / s['span'][1])))
        a = resample(np.tile(a, (reps[1], reps[0], 1)), N, N)
    a = np.clip(match_mean(a, old, s.get('match', 0)), 0, 1)
    rough, hgt = maps(a, s['rough'], grain=s.get('grain', 1.0))
    hm = hgt * s['relief']
    print('S47PHOTO', key, s['file'], 'x%d,%d' % reps, '%d px' % N, 'mean', np.round(a.reshape(-1, 3).mean(0), 3), flush=True)
    return {'base': a, 'rough': rough, 'height': hm, 'normal': normal(hm, s['uv'], green), 'span': s['uv'], 'file': s['file']}


def office(out=None):
    """The office set's three finishes, written over build_office.py's painted files (same names; build_office.py calls
    this after it has painted, and `photo_tex.py saro` does without Blender)."""
    out = Path(out) if out else HERE / 'Office' / 'textures'
    done = []
    for key in ('M_Office_SageWall', 'M_Office_CeilingTile', 'M_Office_FloorLinoleum'):
        base = out / (key + '_base.png')
        P = surface(key, old=load(base) if base.is_file() else None)
        if P is None:
            continue
        save(base, P['base']); save(out / (key + '_rough.png'), P['rough']); save(out / (key + '_normal_dx.png'), P['normal'])
        done.append(key)
    return done


if __name__ == '__main__':
    what = sys.argv[1] if len(sys.argv) > 1 else 'check'
    if what == 'saro':
        print('office:', office())
    else:
        for key, s in SURFACES.items():
            p = PHOTO_DIR / s['file']
            if not p.is_file():
                print(key, 'MISSING', p); continue
            a = load(p)
            if s.get('crop'):
                x0, x1, y0, y1 = s['crop']; a = a[y0:y1, x0:x1]
            b = seamless(a, s.get('grid'))
            print('%-24s %-24s %4dx%-4d seam u %.1f v %.1f -> %.1f %.1f' % (key, s['file'], a.shape[1], a.shape[0],
                  seam_ratio(a, 1), seam_ratio(a, 0), seam_ratio(b, 1), seam_ratio(b, 0)))
