"""Eksporter konseptene fra bevarte generatororiginaler. Krever Pillow."""
from collections import Counter
from pathlib import Path
import hashlib
import json
import subprocess

from PIL import Image, ImageDraw, ImageFont

PROD = Path(__file__).resolve().parent
ART = PROD.parents[1]
WEB = PROD.parents[4]
REPO = WEB.parent
GALLERY = ART / 'concept/night_20261018'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n')


def main():
    plan = json.loads((PROD / 'prompts.json').read_text())
    attempts = json.loads((PROD / 'attempts.json').read_text())
    selected = {a['id']: a for a in attempts if a['selected']}
    assert len(selected) == 8
    profile = (PROD / 'srgb.icc').read_bytes()
    checks = []
    records = []
    for attempt in attempts:
        original = PROD / attempt['source']
        assert sha(original) == attempt['source_sha256'], original
        with Image.open(original) as im:
            im.verify()
        checks.append({'check': attempt['source'] + ' kildehash og dekoding', 'result': 'PASS'})

    for job in plan['jobs']:
        attempt = selected[job['id']]
        original = PROD / attempt['source']
        dest = GALLERY / job['file']
        with Image.open(original) as source:
            source_size = list(source.size)
            assert abs(source.width / source.height - 16 / 9) < 0.005
            exported = source.convert('RGB').resize((1920, 1080), Image.Resampling.LANCZOS)
            exported.save(dest, 'JPEG', quality=90, subsampling=0, optimize=True, icc_profile=profile)
        with Image.open(dest) as im:
            assert im.size == (1920, 1080) and im.format == 'JPEG' and im.mode == 'RGB'
            assert im.info.get('icc_profile') == profile
            im.load()
        checks.append({'check': job['file'] + ' mål, JPEG, RGB, sRGB og dekoding', 'result': 'PASS'})
        records.append({'id': job['id'], 'file': str(dest.relative_to(ART)), 'title': job['title'],
                        'size': [1920, 1080], 'source_size': source_size,
                        'source': attempt['source'], 'source_sha256': sha(original),
                        'sha256': sha(dest), 'bytes': dest.stat().st_size,
                        'selected_revision': attempt['revision'], 'sources': job['sources']})

    cover = Image.new('RGB', (1600, 1980), '#0b1018')
    pen = ImageDraw.Draw(cover)
    try:
        font = ImageFont.truetype('DejaVuSans.ttf', 20)
    except OSError:
        font = ImageFont.load_default(size=20)
    pen.text((24, 18), 'SIGNAL / 47   |   Konsepter, 18. oktober 2026', fill='#ffb052', font=font)
    for i, job in enumerate(plan['jobs']):
        x = 24 + (i % 2) * 788
        y = 62 + (i // 2) * 476
        with Image.open(GALLERY / job['file']) as im:
            thumb = im.resize((764, 430), Image.Resampling.LANCZOS)
            cover.paste(thumb, (x, y))
        pen.text((x, y + 438), job['id'] + '  ' + job['title'], fill='#e8e1d3', font=font)
    cover.save(PROD / 'contact-sheet.jpg', 'JPEG', quality=90, icc_profile=profile)

    runtime_paths = ['web/src', 'web/index.html', 'web/package.json', 'web/package-lock.json',
                     'web/vite.config.ts', ':(exclude)web/src/assets/art/concept',
                     ':(exclude)web/src/assets/art/production']
    changed = subprocess.check_output(['git', 'diff', '--name-only', plan['base_commit'], '--', *runtime_paths],
                                      cwd=REPO, text=True).splitlines()
    assert changed == [], changed
    assert 'night_20261018' not in (WEB / 'src/core/art.ts').read_text()
    assert 'night_20261018' not in (WEB / 'src/core/portraits.ts').read_text()
    checks.append({'check': 'Spillkode og eksisterende runtimegrafikk uendret mot grunnlaget', 'result': 'PASS'})
    checks.append({'check': 'Konseptmappen er ikke lagt til i grafikkinnlastingene', 'result': 'PASS'})
    manifest = {'base_commit': plan['base_commit'], 'tool': plan['tool'], 'generator_model': plan['generator_model'],
                'export': 'Pillow; Lanczos til 1920x1080; JPEG 90, subsampling 0; frosset sRGB-profil',
                'concepts_only': True, 'assets': records, 'attempts': len(attempts)}
    write_json(PROD / 'manifest.json', manifest)
    write_json(PROD / 'verification.json', {'result': 'PASS', 'checks': checks,
               'selected_count': len(selected), 'attempt_count': len(attempts),
               'source_dimensions': dict(Counter('x'.join(map(str, a['source_size'])) for a in records)),
               'runtime_changes': changed, 'total_jpeg_bytes': sum(a['bytes'] for a in records)})
    print(json.dumps({'result': 'PASS', 'checks': len(checks), 'selected': len(records),
                      'jpeg_bytes': sum(a['bytes'] for a in records)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
