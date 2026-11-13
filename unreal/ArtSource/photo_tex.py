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
  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 photo_tex.py saro [dir]    (the SARO textures without a generator hook; dir: lived_tex.py's output folder, for the east wing's wall)
  /home/tom/stable-diffusion-webui-forge/venv/bin/python3 photo_tex.py town <dir>    (Caliche: texture sets and town-photo.json for photo_town_import.py)
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
#        match: how far its mean colour is pulled to the old painted texture's (0 the picture's own, 1 the old one's);
#        target: that painted colour, linear, where the generator's paint is one colour (else the caller hands over `old`)
#        contrast: the picture's contrast about its mean (after the match), where a material mutes or a pattern shouts
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
    # The east wing's own walls, /Game/EastWing/Shell (Shell/import_unreal.py): its material takes a quarter of the picture's
    # grey over a cream paint and a normal at 0.3, with the UVs at 2 m a tile: 4 blocks by 10 courses of the picture, its
    # contrast doubled so that joints and stains come through that paint. Written by `photo_tex.py saro <dir>`.
    'EW_Shell_Masonry': dict(file='saro/block_int.png', crop=(7, 2037, 6, 1855), span=(2.0, 2.0), uv=(2.0, 2.0), size=2048,
                             grid=(4, 5), match=0.0, contrast=2.0, rough=(0.25, 0.95), relief=0.0075, grain=0.12),
    # The office set's 1 m cube-projected finishes, /Game/Signal47/Art/Office (Office/build_office.py), which the control
    # room's and the corridor's finishes tint (SaroInteriors): their colour stays the painted one's.
    'M_Office_SageWall': dict(target=(.255, .282, .250), file='saro/wall_panel.png', crop=(512, 1536, 512, 1536), span=(1.0, 1.0), uv=(1.0, 1.0), size=1024,
                              grid=None, match=1.0, rough=(0.84, 0.95), relief=0.00002),
    # Ceiling panels and floor tiles are real geometry there (T-bars, 30 cm quads): the texture is tile faces without joints.
    'M_Office_CeilingTile': dict(target=(.46, .47, .43), file='saro/ceiling_tile.png', mosaic=(2, 2, 0.05), span=(1.2, 1.2), uv=(1.0, 1.0), size=1024,
                                 grid=None, match=1.0, rough=(0.93, 0.99), relief=0.00006),
    'M_Office_FloorLinoleum': dict(target=(.235, .255, .232), file='saro/vct_floor.png', mosaic=(4, 3, 0.05), span=(1.2, 1.2), uv=(1.0, 1.0), size=1024,
                                   grid=None, match=1.0, contrast=0.5, rough=(0.45, 0.75), relief=0.00001),   # its streaks at half strength: the picture's pattern is loud
    # Caliche, /Game/TownBuildings and /Game/CalicheHomes (`photo_tex.py town <dir>`, photo_town_import.py). The town's
    # materials each tinted one of nine near-white painted textures over the meshes' 1 m UVs; a wall picture is 2 to 3 m and
    # has a dust band at the ground, so these are mapped in the mesh's own space instead (see TOWN): uv = span, nothing repeated.
    #        fade: (u, v) axes that may be cross-faded (a wall with a dust band is never faded up-down); limit: seam ratio that asks for it
    #        notch: (cycles, width) a repeat across the picture that is taken out (the tin's ribs: the sheets' profile is geometry)
    'Town_StuccoBeige': dict(file='town/stucco_beige.png', crop=None, span=(3.0, 3.0), uv=(3.0, 3.0), size=2048, grid=None,
                             fade=(True, False), limit=2.3, match=0.0, rough=(0.86, 0.97), relief=0.0012),
    'Town_StuccoWhite': dict(file='town/stucco_white.png', crop=None, span=(3.0, 3.0), uv=(3.0, 3.0), size=2048, grid=None,
                             limit=2.3, match=0.0, rough=(0.84, 0.96), relief=0.0016),
    'Town_Adobe': dict(file='town/adobe.png', crop=None, span=(2.0, 2.0), uv=(2.0, 2.0), size=2048, grid=None,
                       limit=2.3, match=0.0, rough=(0.90, 0.99), relief=0.0022),
    # The church's boards are geometry, 15 cm to the weather from z 0.33 m: ten whole boards of the picture (150 px each) are
    # laid on them, lap line on lap line, so the picture is 2.05 m wide by 1.5 m (its own boards would be 11 cm).
    'Town_Clapboard': dict(file='town/clapboard_white.png', crop=(0, 2048, 161, 1658), span=(2.05, 1.5), uv=(2.05, 1.5), size=2048,
                           grid=(None, 10), limit=2.3, match=0.0, rough=(0.62, 0.90), relief=0.0012, grain=0.3),
    'Town_RoofTar': dict(file='town/roof_tar.png', crop=None, span=(2.0, 2.0), uv=(2.0, 2.0), size=1024, grid=None,
                         limit=2.3, match=0.0, rough=(0.82, 0.97), relief=0.0020),
    'Town_TinRust': dict(file='town/tin_rust.png', crop=None, span=(1.0, 1.0), uv=(1.0, 1.0), size=1024, grid=None,
                         notch=(20, 3), limit=2.3, match=0.0, rough=(0.38, 0.90), relief=0.00025),
    # The terrazzo picture is two colours, red above row 1016 and cream below: one square of each.
    'Town_TerrazzoRed': dict(file='town/terrazzo.png', crop=(0, 1000, 0, 1000), span=(0.73, 0.73), uv=(0.73, 0.73), size=1024,
                             grid=None, limit=1.0, flat=50, match=0.0, rough=(0.30, 0.55), relief=0.00003),
    'Town_TerrazzoCream': dict(file='town/terrazzo.png', crop=(0, 1000, 1046, 2046), span=(0.73, 0.73), uv=(0.73, 0.73), size=1024,
                               grid=None, limit=1.0, flat=50, match=0.0, rough=(0.30, 0.55), relief=0.00003),
    'Town_HomeTan': dict(file='homes/stucco_tan.png', crop=None, span=(3.0, 3.0), uv=(3.0, 3.0), size=2048, grid=None,
                         fade=(True, False), limit=2.3, match=0.0, rough=(0.88, 0.98), relief=0.0012),
    'Town_HomeCream': dict(file='homes/stucco_cream.png', crop=None, span=(3.0, 3.0), uv=(3.0, 3.0), size=2048, grid=None,
                           fade=(True, False), limit=2.3, match=0.0, rough=(0.86, 0.97), relief=0.0010),
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


def notch(a, cycles, width):
    """Take a repeat across the picture out (`cycles` of it over the width, and its overtones, each +- `width`):
    corrugation that the mesh already has as geometry. Rust, streaks and dents stay."""
    F = np.fft.rfft(a, axis=1)
    for k in range(cycles, F.shape[1], cycles):
        F[:, max(1, k - width):k + width + 1] = 0
    return np.clip(np.fft.irfft(F, n=a.shape[1], axis=1), 0, 1).astype(np.float32)


def match_mean(a, old, amount=1.0):
    """Pull the picture's mean colour towards the old texture's (per channel, in linear light); its detail stays."""
    if amount <= 0 or old is None:
        return a
    la = srgb2lin(a)
    old = np.asarray(old, np.float32)
    want = old if old.ndim == 1 else srgb2lin(old[..., :3]).reshape(-1, 3).mean(0)   # a linear colour, or a picture
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
        if s.get('notch'):
            a = notch(a, *s['notch'])
        if s.get('flat'):
            a = np.clip(a - blur_plain(a, s['flat']) + a.reshape(-1, 3).mean(0), 0, 1)
        a = seamless(a, s.get('grid'), *s.get('fade', (True, True)), limit=s.get('limit', 1.8))
        reps = (max(1, round(s['uv'][0] / s['span'][0])), max(1, round(s['uv'][1] / s['span'][1])))
        a = resample(np.tile(a, (reps[1], reps[0], 1)), N, N)
    a = np.clip(match_mean(a, s.get('target', old), s.get('match', 0)), 0, 1)
    if s.get('contrast'):
        m = a.reshape(-1, 3).mean(0)
        a = np.clip(m + (a - m) * s['contrast'], 0, 1)
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
        P = surface(key)
        if P is None:
            continue
        save(base, P['base']); save(out / (key + '_rough.png'), P['rough']); save(out / (key + '_normal_dx.png'), P['normal'])
        done.append(key)
    return done


def shell_masonry(out):
    """The east wing shell's block wall (texture-only: the shell's importer and its CC0 source set are left as they are).
    Written beside lived_tex.py's output, outside Git like it (24 MB that the picture and this script make again)."""
    out = Path(out)
    P = surface('EW_Shell_Masonry', green=-1.0)   # that texture asset flips green on import (an OpenGL source)
    if P is None:
        return []
    out.mkdir(parents=True, exist_ok=True)
    save(out / 'EW_Shell_Masonry_BaseColor.png', P['base']); save(out / 'EW_Shell_Masonry_Roughness.png', P['rough'])
    save(out / 'EW_Shell_Masonry_NormalGL.png', P['normal'])
    return ['EW_Shell_Masonry']


# Caliche: which materials take which picture. The town's materials are whole graphs (townintegration/import_assets.py), one per
# name, each a tint over a shared painted texture; photo_town_import.py rebuilds the graphs named here around the picture,
# mapped in the mesh's own space: along the wall and up from the floor (band: the picture is not repeated upwards, its dust
# band stays at the ground and the wall above 3 m takes the picture's upper part mirrored), or flat on roofs and floors.
#   family: (main material, its variants): the main one's tint goes `pull` of the way (in log) from the picture's colour to the
#   old tint x old texture (TOWN_PULL, or the entry's own: the feed store is to be adobe, not the old tan); a variant keeps its old ratio to the main one (patches, plinth, worn boards).
#   homes: MI_Homes_* are instances of the town's graphs and cannot swap a texture: they get a new parent with the picture.
TOWN_SRC = HERE.parent / 'Signal47/Tools/townbuildings'
TOWN = [
    dict(set='Town_StuccoBeige', band=True, families=[('SheriffStucco', ['SheriffStuccoPatch', 'SheriffCoping', 'SheriffStuccoBase']),
                                                         ('CinemaStucco', ['CinemaStain'])]),
    dict(set='Town_StuccoWhite', band=False, families=[('Stucco', [])]),
    dict(set='Town_Adobe', band=False, pull=0.2, families=[('FeedStucco', ['FeedStuccoPatch', 'FeedStuccoBase', 'FeedAdobe'])]),
    dict(set='Town_Clapboard', band=False, phase=0.33, families=[('ChurchWhiteBoard', ['ChurchWornBoard'])]),
    dict(set='Town_RoofTar', band=False, families=[('CinemaRoofTar', []), ('SheriffRoofing', [])]),
    dict(set='Town_TinRust', band=False, families=[('RustTin', []), ('FeedRustTin', [])]),
    dict(set='Town_TerrazzoRed', band=False, families=[('CinemaTerrazzoRed', [])]),
    dict(set='Town_TerrazzoCream', band=False, families=[('CinemaTerrazzoCream', []), ('Terrazzo', [])]),
    dict(set='Town_HomeTan', band=True, parent='PhotoStuccoTan', like='Stucco', homes=[('TanStucco', [185, 143, 104]), ('TanWear', [169, 132, 100])]),
    dict(set='Town_HomeCream', band=True, parent='PhotoStuccoCream', like='Stucco',
         homes=[('CreamStucco', [220, 201, 166]), ('CreamWear', [198, 182, 151]), ('DoorMasonry', [203, 188, 158])]),
]
# Kept as they were (13 Nov): the church's brick is a row of modelled soldier bricks, no wall for brick_red (the school has one);
# the laundromat's floor is 30 cm vinyl squares laid as geometry in two materials, tile_green_cream is a ceramic wall tile with
# a band; bell bronze and galvanized trim still tint the painted tin; streaks, cracks and grime are thin tinted overlays.
TOWN_PULL = 0.5


def town(out):
    """The Caliche texture sets (T_<set>_base_color/roughness/normal.png) and town-photo.json into `out` (outside Git)."""
    import glob, json
    out = Path(out); out.mkdir(parents=True, exist_ok=True)
    rows = {}
    for f in [TOWN_SRC / 'materials.json'] + sorted(glob.glob(str(TOWN_SRC / 'tb2' / 'materials_*.json'))):
        for r in json.loads(Path(f).read_text()):
            rows[r['name'].removeprefix('M_Town_')] = r
    old_mean = {}
    def eff(name, tint=None):   # the old look: tint x the painted texture's mean, linear
        r = rows[name]; t = r['textures']['base_color']
        if t not in old_mean:
            old_mean[t] = srgb2lin(load(TOWN_SRC / t)).reshape(-1, 3).mean(0)
        return np.asarray(tint if tint is not None else r['texture_tint_linear'][:3], np.float32) * old_mean[t]
    man = {'textures': [], 'materials': []}
    for T in TOWN:
        P = surface(T['set'])
        if P is None:
            continue
        s = SURFACES[T['set']]
        files = {}
        for role, a in (('base_color', P['base']), ('roughness', P['rough']), ('normal', P['normal'])):
            files[role] = str(out / ('T_%s_%s.png' % (T['set'], role))); save(files[role], a)
        man['textures'].append({'set': T['set'], 'files': files, 'size': s['size'], 'picture': s['file']})
        mean = srgb2lin(P['base']).reshape(-1, 3).mean(0)
        def tint_of(e):
            return np.clip(np.power(np.maximum(e, 1e-4) / mean, T.get('pull', TOWN_PULL)), 0.25, 1.6)
        common = {'set': T['set'], 'span': list(s['uv']), 'band': bool(T['band']), 'phase': T.get('phase', 0.0)}
        for main, variants in T.get('families', []):
            tm = tint_of(eff(main)); t0 = np.asarray(rows[main]['texture_tint_linear'][:3], np.float32)
            for name in [main] + variants:
                r = rows[name]
                t = tm * np.asarray(r['texture_tint_linear'][:3], np.float32) / t0
                man['materials'].append(dict(common, kind='material', name='M_Town_' + name, tint=[round(float(x), 4) for x in t],
                                             metallic=r.get('metallic', 0), specular=r.get('specular', .28), roughness=r.get('roughness', .9)))
        if T.get('homes'):
            r = rows[T['like']]
            man['materials'].append(dict(common, kind='material', name='M_Town_' + T['parent'], tint=[1, 1, 1], new=True,
                                         metallic=r.get('metallic', 0), specular=r.get('specular', .28), roughness=r.get('roughness', .9)))
            m0 = T['homes'][0][1]; tm = tint_of(eff(T['like'], srgb2lin(np.asarray(m0, np.float32) / 255)))
            for name, rgb in T['homes']:
                t = tm * srgb2lin(np.asarray(rgb, np.float32) / 255) / srgb2lin(np.asarray(m0, np.float32) / 255)
                man['materials'].append({'kind': 'instance', 'name': 'MI_Homes_' + name, 'parent': 'M_Town_' + T['parent'],
                                         'tint': [round(float(x), 4) for x in t]})
    (out / 'town-photo.json').write_text(json.dumps(man, indent=1))
    for m in man['materials']:
        print('S47PHOTO town', m['name'], m.get('set', m.get('parent')), m['tint'])
    return man


if __name__ == '__main__':
    what = sys.argv[1] if len(sys.argv) > 1 else 'check'
    if what == 'saro':
        print('office:', office())
        if len(sys.argv) > 2:
            print('shell:', shell_masonry(sys.argv[2]))
    elif what == 'town':
        town(sys.argv[2])
    else:
        for key, s in SURFACES.items():
            p = PHOTO_DIR / s['file']
            if not p.is_file():
                print(key, 'MISSING', p); continue
            a = load(p)
            if s.get('crop'):
                x0, x1, y0, y1 = s['crop']; a = a[y0:y1, x0:x1]
            b = seamless(a, s.get('grid'), *s.get('fade', (True, True)), limit=s.get('limit', 1.8))
            print('%-24s %-24s %4dx%-4d seam u %.1f v %.1f -> %.1f %.1f' % (key, s['file'], a.shape[1], a.shape[0],
                  seam_ratio(a, 1), seam_ratio(a, 0), seam_ratio(b, 1), seam_ratio(b, 0)))
