"""Re-encode the pinned NASA catalogue map for the Unity skybox; no AI imagery.

Run Blender 4.5.13 with --background --factory-startup --python this_file.
Original EXR is cached outside Unity under Artifacts/Sky12/Source.
The derived 8-bit sRGB PNG keeps all 8192x4096 pixels and fits normal Git.
"""
import bpy
import hashlib
import json
from pathlib import Path
import urllib.request
import numpy as np

ROOT = Path(__file__).resolve().parents[3]
URL = 'https://svs.gsfc.nasa.gov/vis/a000000/a004800/a004851/starmap_2020_8k.exr'
EXPECTED = 'dc6c4f413e85707a29a25a9451148154554ecca2c996f84fa8f47b65ef9ff7c4'
SOURCE = ROOT / 'Artifacts/Sky12/Source/starmap_2020_8k.exr'
OUTPUT = ROOT / 'Unity/Assets/Signal47/Art/NightSky12'
SOURCE.parent.mkdir(parents=True, exist_ok=True)
OUTPUT.mkdir(parents=True, exist_ok=True)
if not SOURCE.exists():
    with urllib.request.urlopen(URL, timeout=120) as response:
        SOURCE.write_bytes(response.read())
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest() == EXPECTED
source = bpy.data.images.load(str(SOURCE), check_existing=False)
assert tuple(source.size) == (8192, 4096)
scene = bpy.context.scene
scene.view_settings.view_transform = 'Standard'
scene.view_settings.look = 'None'
scene.view_settings.exposure = 0
scene.view_settings.gamma = 1
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGB'
scene.render.image_settings.color_depth = '8'
scene.render.image_settings.compression = 90
destination = OUTPUT / 'NASA_DeepStarMap_8k.png'
source.save_render(str(destination), scene=scene)
derived = bpy.data.images.load(str(destination), check_existing=False)
assert tuple(derived.size) == tuple(source.size)
a = np.empty(len(source.pixels), dtype=np.float32)
b = np.empty(len(derived.pixels), dtype=np.float32)
source.pixels.foreach_get(a)
derived.pixels.foreach_get(b)
a, b = a.reshape(-1, 4)[:, :3], b.reshape(-1, 4)[:, :3]
assert np.isfinite(a).all() and np.isfinite(b).all()
# Blender exposes byte-image pixels in their encoded sRGB space; compare both
# maps in linear light, as Unity does when sampling an sRGB texture.
b = np.where(b <= .04045, b / 12.92, ((b + .055) / 1.055) ** 2.4)
error = np.abs(a - b)
report = {
    'source_page': 'https://svs.gsfc.nasa.gov/4851/',
    'source_url': URL, 'source_sha256': EXPECTED,
    'source_bytes': SOURCE.stat().st_size,
    'output': destination.name,
    'output_sha256': hashlib.sha256(destination.read_bytes()).hexdigest(),
    'output_bytes': destination.stat().st_size,
    'dimensions': list(derived.size),
    'conversion': 'Blender 4.5.13 Standard, exposure 0, gamma 1; linear EXR to 8-bit sRGB PNG. No resize or painted stars. Quantized derivative, not a lossless master.',
    'linear_mean_absolute_error': float(error.mean()),
    'linear_max_absolute_error': float(error.max()),
    'credits': 'NASA/Goddard Space Flight Center Scientific Visualization Studio; Ernie Wright (USRA). Gaia DR2: ESA/Gaia/DPAC. Hipparcos-2, Tycho-2, Yale Bright Star, UCAC3 and XHIP catalogue sources.',
    'terms': 'https://www.nasa.gov/nasa-brand-center/images-and-media/',
    'scope': 'Star-map layer only; no IAU constellation figures, NASA logo or identifiable people. Artistic sky orientation, not a dated 1986 sky simulation.'
}
assert report['output_bytes'] < 95_000_000
assert report['linear_mean_absolute_error'] < .001
(OUTPUT / 'provenance.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
