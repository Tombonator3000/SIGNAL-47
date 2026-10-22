#!/usr/bin/env python3
# The notebook's pictures for the Unreal version (S47Image::Art reads Content/WebData/art):
#   the kit drawings (web/src/assets/art/runtime/kit_*.webp, ui/kitArt.ts) as PNG, and
#   the portraits of the people in the case (web/src/assets/art/people/portrait_*.jpg, core/portraits.ts).
# Content/WebData is made from the web version and ignored by Git; import.sh runs this.
# Usage: Tools/webexport/notebook_art.py   (needs Pillow with WebP)
import glob
import os
import shutil
import sys

from PIL import Image

D = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(D, '..', '..', '..', '..'))
ART = os.path.join(ROOT, 'web', 'src', 'assets', 'art')
OUT = os.path.join(D, '..', '..', 'Content', 'WebData', 'art')
os.makedirs(OUT, exist_ok=True)
n = 0
for f in sorted(glob.glob(os.path.join(ART, 'runtime', 'kit_*.webp'))):
    name = os.path.splitext(os.path.basename(f))[0]
    Image.open(f).convert('RGBA').save(os.path.join(OUT, name + '.png'))
    n += 1
for f in sorted(glob.glob(os.path.join(ART, 'people', 'portrait_*.jpg'))):
    shutil.copy(f, os.path.join(OUT, os.path.basename(f)))
    n += 1
print('notebook art: %d pictures in %s' % (n, os.path.normpath(OUT)))
sys.exit(0 if n else 1)
