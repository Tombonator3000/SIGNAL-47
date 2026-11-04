#!/usr/bin/env python3
"""Derive linear data maps from image-generated albedo; never modify BaseColor.

uv run --no-project --with numpy --with pillow --with scipy python photo_texture_tools.py \
    textures/EW_PolishedConcrete_BaseColor.png
Height is artist-estimated microrelief, not a measured scan or luminance displacement.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter


CONFIG = {
    'EW_PolishedConcrete': ('concrete', 2., 2., True),
    'EW_GarageSlab': ('garage', 6.7, 10.5, False),
    'EW_XEnamelGreen': ('paint', .5, .5, True),
    'EW_XEnamelRed': ('paint', .5, .5, True),
    'EW_XEnamelWhite': ('paint', .5, .5, True),
    'EW_XOliveDrab': ('paint', .5, .5, True),
    'EW_XInstrumentGrey': ('paint', .5, .5, True),
    'EW_XStainless': ('stainless', .5, .5, True),
    'EW_XWood': ('wood', .5, .5, True),
    'EW_XRubber': ('rubber', .5, .5, True),
    'EW_CinderblockPainted': ('block', 1.6, 1.6, True),
}


def smooth(a, sigma, periodic):
    return gaussian_filter(a, sigma, mode='wrap' if periodic else 'reflect')


def periodic_edges(a):
    """Condition scalar data edges with a narrow smooth strip, not the albedo."""
    a = a.copy()
    for axis in (0, 1):
        x = np.moveaxis(a, axis, 0)
        width = max(8, min(48, x.shape[0] // 32))
        avg = (x[0] + x[-1]) * .5
        # Blend from saved border errors, keeping C0 exact.
        left, right = x[0].copy(), x[-1].copy()
        for i in range(width):
            weight = .5 + .5 * np.cos(np.pi * i / width)
            x[i] += weight * (avg-left)
            x[-1-i] += weight * (avg-right)
        x[0] = avg
        x[-1] = avg
    return a


def normal_gl(height, width_m, height_m, periodic):
    """Image rows point down, UV V up: normal RGB = (-dh/dU,+dh/drow,1)."""
    rows, cols = height.shape
    if periodic:
        dx = (np.roll(height, -1, 1)-np.roll(height, 1, 1)) / (2*width_m/cols)
        dy = (np.roll(height, -1, 0)-np.roll(height, 1, 0)) / (2*height_m/rows)
    else:
        dy, dx = np.gradient(height, height_m/rows, width_m/cols)
    n = np.stack((-dx, dy, np.ones_like(height)), -1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return np.rint(np.clip(n*.5+.5, 0, 1)*255).astype(np.uint8)


def derive(rgb, kind, periodic):
    # Luminance is only a bounded microdetail cue. Broad pigmentation is excluded
    # from height, especially oil, tyre tracks, paint and concrete clouding.
    lum = rgb @ np.array([.2126, .7152, .0722], np.float32)
    low = smooth(lum, 12, periodic)
    micro = np.clip((lum-smooth(lum, 2.2, periodic))/.12, -1, 1)
    detail = np.clip((lum-low)/.20, -1, 1)
    dark = np.clip((np.median(low)-low)/.3, 0, 1)
    if kind == 'concrete':
        height = micro*.000018 + detail*.000012
        rough = .31 + .10*dark + .035*detail + .055*smooth(abs(detail), 22, periodic)
    elif kind == 'garage':
        # Oil is smoother, sand is rougher. Colour masks do not displace the slab.
        oil = np.clip((.32-low)/.24, 0, 1)
        sand = np.clip((rgb[..., 0]-rgb[..., 2]-.04)*4, 0, 1)
        top = np.linspace(1, 0, lum.shape[0])[:, None]**6
        height = micro*.000035
        rough = .40 + .08*detail - .16*oil + .30*sand*top
        # Saw cuts are known layout constraints, 2 mm wide and 0.65 mm deep.
        yy, xx = np.indices(lum.shape, dtype=np.float32)
        u, v = xx/(lum.shape[1]-1), yy/(lum.shape[0]-1)
        cuts = np.exp(-.5*((u-.5)/.00025)**2)
        cuts = np.maximum(cuts, np.exp(-.5*((v-1/3)/.00018)**2))
        cuts = np.maximum(cuts, np.exp(-.5*((v-2/3)/.00018)**2))
        height -= cuts*.00065
        rough += cuts*.20
    elif kind == 'paint':
        # Small dark interruptions indicate chips; reject broad cabinet shading.
        chips = np.clip((smooth(lum, 5, periodic)-lum-.045)*8, 0, 1)
        height = micro*.000005 - chips*.00007
        rough = .29 + .12*dark + .25*chips + .025*detail
    elif kind == 'stainless':
        # Horizontal brushing with restrained relief, never silver-to-height.
        grain = lum-smooth(lum, (1.2, 0), periodic)
        height = np.clip(grain/.1, -1, 1)*.000008
        rough = .27 + .06*detail + .12*dark
    elif kind == 'wood':
        height = detail*.000025 + micro*.000008
        rough = .35 + .08*dark + .055*detail
    elif kind == 'rubber':
        height = micro*.000012 + detail*.000008
        rough = .70 + .08*detail + .10*dark
    elif kind == 'block':
        # Dark, locally continuous painted mortar reveals joints. Suppress fine
        # aggregate so isolated dark pores do not become deep holes.
        broad = smooth(lum, 2, periodic)
        local = smooth(lum, 35, periodic)
        mortar = np.clip((local-broad-.025)/.14, 0, 1)
        mortar = smooth(mortar, 1.1, periodic)
        height = -.0015*mortar + micro*.00009 + detail*.00005
        rough = .39 + .14*mortar + .06*dark + .035*detail
    else:
        raise ValueError(kind)
    if periodic:
        height, rough = periodic_edges(height), periodic_edges(rough)
    return height.astype(np.float32), np.clip(rough, .14, .94)


def process(path, height_dir):
    name = path.stem.removesuffix('_BaseColor')
    kind, width, depth, periodic = CONFIG[name]
    image = Image.open(path).convert('RGB')
    expected = (2048, 4096) if kind == 'garage' else (2048, 2048)
    if image.size != expected:
        raise ValueError(f'{path}: expected {expected}, got {image.size}; resize albedo first')
    height, roughness = derive(np.asarray(image, dtype=np.float32)/255, kind, periodic)
    height_dir.mkdir(parents=True, exist_ok=True)
    lo, hi = float(height.min()), float(height.max())
    scale = max(hi-lo, 1e-8)
    encoded = np.rint((height-lo)/scale*65535).astype(np.uint16)
    hpath = height_dir/f'{name}_Height.png'
    Image.fromarray(encoded).save(hpath)
    # Derive normals from the actual saved 16-bit height, not a hidden float field.
    saved = np.asarray(Image.open(hpath), dtype=np.float32)/65535*scale+lo
    normal = normal_gl(saved, width, depth, periodic)
    Image.fromarray(normal).save(path.with_name(f'{name}_NormalGL.png'))
    Image.fromarray(np.rint(roughness*255).astype(np.uint8)).save(path.with_name(f'{name}_Roughness.png'))
    meta = dict(material=name, dimensions=list(image.size), span_m=[width, depth],
                tileable=periodic, height_offset_m=lo, height_scale_m=scale,
                height_source='artist-estimated material relief; not measured geometry',
                normal_convention='OpenGL Y+, image row down; n=(-dh/dU,+dh/drow,1)',
                data_color_space='linear, no sRGB chunk', base_color_untouched=True,
                roughness_range=[float(roughness.min()), float(roughness.max())])
    hpath.with_suffix('.json').write_text(json.dumps(meta, indent=2)+'\n')
    print(f'{name}: height {scale*1000:.3f} mm, roughness {roughness.min():.3f}..{roughness.max():.3f}')


def self_check():
    # A height increasing toward the image bottom yields positive OpenGL green.
    y, x = np.indices((32, 32), dtype=np.float32)
    n = normal_gl((x+y)*.001, 1, 1, False)[16, 16]
    assert n[0] < 128 and n[1] > 128 and n[2] > 240, n
    print('NormalGL ramp convention: PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('base_colors', nargs='*', type=Path)
    parser.add_argument('--height-dir', type=Path, default=Path(__file__).resolve().parent/'photo_sources'/'height')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    for base_color in args.base_colors:
        process(base_color, args.height_dir)
