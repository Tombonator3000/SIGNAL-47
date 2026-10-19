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
         'ext/sign_sierra_on.png', 'lab/sign_blank.png', 'yard/floor_paint_frame.png',
         'annex/vending_front.png',
         'diner/sign_diner_blank.png', 'diner/menu_board_blank.png',
         'road/milepost_blank.png', 'road/witness_post.png', 'road/survey_disk_brass.png',
         # round 14 (Caliche and them) and round 15 (the notebook's Kit tab)
         'town/sign_caliche.png', 'town/sign_gas.png', 'town/sign_1420.png', 'town/sign_souvenir.png',
         'town/sign_sheriff.png', 'town/marquee_plaza.png', 'town/board_church.png', 'town/cutout_alien.png',
         'alien/silhouette_stand.png', 'alien/silhouette_turned.png', 'alien/silhouette_far.png',
         'kit/kit_notebook.png', 'kit/kit_jacket.png', 'kit/kit_camera.png', 'kit/kit_flashlight.png',
         'kit/kit_keys.png', 'kit/kit_papers.png', 'kit/kit_nickel_1947.png']
# Round 16, the painted horizon (world/backdrop.ts): only the rows round the horizon go to the game,
# 576 to 832 of the 1024 (8.44 degrees over it to 2.81 under it; nothing is painted above 654), at
# half size: the two generated panels behind them had about 4300 pixels round, and a phone keeps the
# decoded picture in memory (4096 x 128 is 2 MB, the whole crop 8).
CROPS = {'sky/horizon_night.png': ((0, 576, 8192, 832), (4096, 128)), 'sky/horizon_dawn.png': ((0, 576, 8192, 832), (4096, 128))}
FILES += list(CROPS)
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
        icc = im.info.get('icc_profile', b'')
        if relative in CROPS: im = im.crop(CROPS[relative][0]).resize(CROPS[relative][1], Image.LANCZOS)
        im.save(target, 'WEBP', quality=88, method=6, icc_profile=icc)
        size = im.size
    with Image.open(target) as check:
        check.load()
        assert check.size == size
    manifest.append(dict(source=relative, runtime='runtime/' + target.name,
                         dimensions=size, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                         runtime_sha256=hashlib.sha256(target.read_bytes()).hexdigest(), bytes=target.stat().st_size))
(OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'{len(manifest)} runtime images, {sum(m["bytes"] for m in manifest):,} bytes')
