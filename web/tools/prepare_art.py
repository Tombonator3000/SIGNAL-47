"""Encode lightweight runtime copies; preserve ART_BRIEF PNG sources unchanged.
Run from web/: python3 tools/prepare_art.py (Pillow required).
A copy whose source is unchanged since the manifest was written is left as it is,
so adding a file does not re-encode (and change the bytes of) the earlier ones.
"""
import hashlib, json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1] / 'src/assets/art'
FILES = ['room/poster_listen.png', 'room/poster_saro.png', 'room/map_new_mexico.png',
         'brand/logo_saro.png', 'yard/sign_service_yard.png', 'yard/label_s03_procedure.png',
         'ext/sign_sierra_on.png', 'lab/sign_blank.png', 'yard/floor_paint_frame.png']
OUT = ROOT / 'runtime'
OUT.mkdir(exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
try:
    earlier = {m['source']: m for m in json.loads((OUT / 'manifest.json').read_text())}
except FileNotFoundError:
    earlier = {}
manifest = []
for relative in FILES:
    source = ROOT / relative
    target = OUT / (source.stem + '.webp')
    old = earlier.get(relative)
    if old and target.exists() and old['source_sha256'] == sha(source) and old['runtime_sha256'] == sha(target):
        manifest.append(old)
        continue
    with Image.open(source) as im:
        im.save(target, 'WEBP', quality=88, method=6, icc_profile=im.info.get('icc_profile', b''))
        size = im.size
    with Image.open(target) as check:
        check.load()
        assert check.size == size
    manifest.append(dict(source=relative, runtime='runtime/' + target.name,
                         dimensions=size, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                         runtime_sha256=hashlib.sha256(target.read_bytes()).hexdigest(), bytes=target.stat().st_size))
(OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'{len(manifest)} runtime images, {sum(m["bytes"] for m in manifest):,} bytes')
