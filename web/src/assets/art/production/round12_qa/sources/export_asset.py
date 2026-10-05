#!/usr/bin/env python3
"""Reproduce the chosen JPEG using only size/format/profile export."""
from pathlib import Path
from PIL import Image
import hashlib

sources = Path(__file__).resolve().parent
art = sources.parents[2]
source = sources / "tex_oldroad_macro_pinpoint_edit_generated.png"
destination = art / "road/tex_oldroad_macro.jpg"
if source.is_symlink() or destination.is_symlink():
    raise ValueError("Export requires regular source and destination paths")
with Image.open(source) as opened:
    image = opened.convert("RGB").resize((2048, 2048), Image.Resampling.LANCZOS)
image.save(destination, "JPEG", quality=85, subsampling=0, optimize=True,
           icc_profile=(sources / "srgb.icc").read_bytes())
print(hashlib.sha256(destination.read_bytes()).hexdigest())
