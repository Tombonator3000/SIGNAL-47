#!/usr/bin/env python3
"""A lit look at a texture set outside the engine: base x (ambient + a raking lamp on the normal map), tiled 2 x 2, with a specular hint from the roughness.
  preview_lit.py <tex dir> <name> <out.jpg> [crop px]"""
import sys
import numpy as np
from PIL import Image
d, name, out = sys.argv[1], sys.argv[2], sys.argv[3]
crop = int(sys.argv[4]) if len(sys.argv) > 4 else 0
b = np.asarray(Image.open(f'{d}/T_Rec_{name}_base.png').convert('RGB'), np.float32) / 255
n = np.asarray(Image.open(f'{d}/T_Rec_{name}_normal.png').convert('RGB'), np.float32) / 255 * 2 - 1
r = np.asarray(Image.open(f'{d}/T_Rec_{name}_rough.png').convert('L'), np.float32) / 255
n[..., 1] *= -1                                    # DirectX green down -> image rows down = -v
n /= np.linalg.norm(n, axis=-1, keepdims=True)
lin = np.where(b <= .04045, b / 12.92, ((b + .055) / 1.055) ** 2.4)
rows = []
for L in ((-0.25, 0.85, 0.46), (0.75, 0.25, 0.60)):   # from above (a ceiling troffer on a wall), from the side
    L = np.array(L, np.float32); L /= np.linalg.norm(L)
    nl = np.clip(n @ L, 0, 1)
    Hh = L + np.array([0, 0, 1], np.float32); Hh /= np.linalg.norm(Hh)
    spec = np.clip(n @ Hh, 0, 1) ** (2 / np.maximum(r, .05) ** 2) * (1 - r) * 0.25
    c = lin * (0.18 + 1.5 * nl[..., None]) + spec[..., None]
    c = np.clip(c, 0, 1) ** (1 / 2.2)
    rows.append(c)
img = np.concatenate([np.tile(rows[0], (1, 2, 1))[:1024 if not crop else crop], np.tile(rows[1], (1, 2, 1))[:1024 if not crop else crop]], 0)
if crop:
    img = np.concatenate([rows[0][:crop, :crop], rows[1][:crop, :crop], b[:crop, :crop]], 1)
Image.fromarray((img * 255 + .5).astype(np.uint8)).resize((img.shape[1] // 2, img.shape[0] // 2) if not crop else (img.shape[1], img.shape[0]), Image.LANCZOS).save(out, quality=90)
print(out)
