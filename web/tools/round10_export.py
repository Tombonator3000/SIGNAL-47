"""Only exact-size, color-space and JPEG export; composition is made in imagegen.

No painted corrections, retouching, content synthesis or local compositing.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
from PIL import Image, ImageCms, __version__ as pillow_version

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'src/assets/art'
SOURCES = ART / 'production/round10_qa/sources'
SPEC = {
    'photo_1947_master': (2400,1600),
    'poster_halley_1986_blank': (1024,1536),
    'fanfold_1986': (1024,1186),
    'fanfold_1947': (1024,1186),
}

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('asset', choices=SPEC)
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    SOURCES.mkdir(parents=True, exist_ok=True)
    saved_source = SOURCES / f'{args.asset}_generated.png'
    if args.source.resolve() != saved_source.resolve():
        saved_source.write_bytes(args.source.read_bytes())
    profile_path = SOURCES / 'srgb.icc'
    if not profile_path.exists():
        profile_path.write_bytes(ImageCms.ImageCmsProfile(ImageCms.createProfile('sRGB')).tobytes())
    icc = profile_path.read_bytes()
    im = Image.open(saved_source)
    im.load()
    original_size = im.size
    has_profile = bool(im.info.get('icc_profile'))
    if has_profile:
        input_profile = ImageCms.ImageCmsProfile(io.BytesIO(im.info['icc_profile']))
        target_profile = ImageCms.ImageCmsProfile(io.BytesIO(icc))
        im = ImageCms.profileToProfile(im.convert('RGB'), input_profile, target_profile, outputMode='RGB')
    else:
        im = im.convert('RGB')
    im = im.resize(SPEC[args.asset], Image.Resampling.LANCZOS)
    if args.asset == 'photo_1947_master':
        im = im.convert('L').convert('RGB')
    dest = ART / 'docs' / f'{args.asset}.jpg'
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.save(dest, 'JPEG', quality=85, subsampling=0, optimize=True, icc_profile=icc)
    record = {
        'asset':args.asset, 'source':str(saved_source.relative_to(ROOT)),
        'source_sha256':sha(saved_source), 'source_size':list(original_size),
        'source_embedded_icc':has_profile,
        'file':str(dest.relative_to(ROOT)), 'size':list(im.size),
        'sha256':sha(dest), 'bytes':dest.stat().st_size,
        'export':{'pillow':pillow_version,'resize':'Lanczos, exact-size only',
                  'color':'embedded frozen sRGB ICC, RGB',
                  'source_without_icc':'numeric RGB interpreted as sRGB per art brief',
                  'grayscale_rgb':args.asset=='photo_1947_master',
                  'jpeg_quality':85,'subsampling':0,'optimize':True},
        'srgb_profile':str(profile_path.relative_to(ROOT)), 'srgb_sha256':sha(profile_path),
    }
    (SOURCES / f'{args.asset}_export.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record))

if __name__ == '__main__':
    main()
