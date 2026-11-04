"""Reconstruct periodic linear height DATA from generated OpenGL normal maps.

Run with Python + numpy + Pillow (no engine required). Inputs are preserved.
The result is an estimated integrable relief field, not measured displacement.
"""
from pathlib import Path
import argparse
import hashlib
import json
import numpy as np
from PIL import Image


def reconstruct(normal):
    n = normal.astype(np.float64) / 127.5 - 1.0
    z = np.maximum(n[:, :, 2], 0.25)
    # Image columns are +U, rows are -OpenGL V. Unreal flips the green
    # channel on import, so these slopes also match its world-XY projection.
    gx = np.clip(-n[:, :, 0] / z, -2.0, 2.0)
    gy = np.clip(n[:, :, 1] / z, -2.0, 2.0)
    gx -= gx.mean()
    gy -= gy.mean()
    rows, cols = gx.shape
    kx = 2 * np.pi * np.fft.fftfreq(cols)[None, :]
    ky = 2 * np.pi * np.fft.fftfreq(rows)[:, None]
    k2 = kx*kx + ky*ky
    # Screened Poisson least-squares integration rejects nonintegrable curl
    # and damps broad generated-map drift beyond about one third of a tile.
    screening = (2 * np.pi * 3 / min(rows, cols)) ** 2
    spectrum = (-1j*kx*np.fft.fft2(gx) - 1j*ky*np.fft.fft2(gy)) / (k2 + screening)
    spectrum[0, 0] = 0
    height = np.fft.ifft2(spectrum).real
    low, high = float(height.min()), float(height.max())
    if high-low < 1e-8:
        raise ValueError('Normal map contains no reconstructable relief')
    height = (height-low)/(high-low)
    # Use the full bounded range without clipping away stones or cracks.
    quantized = np.rint(height*65535).astype(np.uint16)
    dx = (np.roll(height, -1, 1)-np.roll(height, 1, 1))*.5
    dy = (np.roll(height, -1, 0)-np.roll(height, 1, 0))*.5
    corr = lambda a,b: float(np.corrcoef(a.ravel(),b.ravel())[0,1])
    edge = np.concatenate((np.abs(height[:,0]-height[:,-1]), np.abs(height[0]-height[-1])))
    interior = np.concatenate((np.abs(np.diff(height,axis=0)).ravel(),np.abs(np.diff(height,axis=1)).ravel()))
    stats = {'range_u16':[int(quantized.min()),int(quantized.max())],
             'slope_correlation_u':corr(dx,gx), 'slope_correlation_image_v':corr(dy,gy),
             'wrap_step_mean':float(edge.mean()),'interior_step_mean':float(interior.mean()),
             'wrap_step_max':float(edge.max()), 'interior_step_p99':float(np.percentile(interior,99))}
    if min(stats['slope_correlation_u'],stats['slope_correlation_image_v']) <= 0:
        raise ValueError('Height slopes oppose the source normal convention')
    return quantized, stats


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=Path(__file__).resolve().parent/'exports'/'ground')
    args=parser.parse_args()
    records={}
    for layer in ('Sand','Crust','Gravel'):
        src=args.source/f'T_Ground{layer}_N.png'
        dst=args.source/f'T_Ground{layer}_H.png'
        rgb=np.asarray(Image.open(src).convert('RGB'))
        if rgb.shape != (2048,2048,3):
            raise ValueError(f'Expected 2048 square normal: {src}')
        height,stats=reconstruct(rgb)
        Image.fromarray(height).save(dst)
        check=np.asarray(Image.open(dst))
        assert np.array_equal(height,check), '16-bit PNG roundtrip mismatch'
        records[layer]={'source':src.name,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
                        'output':dst.name,**stats}
    print(json.dumps(records,indent=2))


if __name__=='__main__':main()
