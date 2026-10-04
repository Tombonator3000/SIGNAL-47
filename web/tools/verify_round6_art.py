#!/usr/bin/env python3
"""Kontroller runde 6-assets, kildehash, 2x2-kopier og lokale gallerilenker.

Kjør fra web/: python3 tools/verify_round6_art.py
Pillow må være tilgjengelig. Ingen installasjon, nettleser eller runtime-kjøring.
Manglende leveranse gir INCOMPLETE og skriver ingen bevisfil. Bruk --no-write
for en foreløpig kontroll. Visuell søm-, motiv- og tekstkontroll er separat.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
from functools import lru_cache
import hashlib
from html.parser import HTMLParser
from io import BytesIO
import json
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

from PIL import Image, ImageChops, ImageCms, ImageStat, __version__ as PILLOW_VERSION

SPECS = {
    'diner/sign_diner_blank.png': ((1024, 512), 'PNG', ('RGBA',), True),
    'diner/tex_counter_laminate.jpg': ((512, 512), 'JPEG', ('RGB',), False),
    'diner/tex_floor_checker.jpg': ((1024, 1024), 'JPEG', ('RGB',), False),
    'diner/menu_board_blank.png': ((1024, 512), 'PNG', ('RGB', 'RGBA'), False),
    'diner/clipping_photo_1947.jpg': ((1024, 768), 'JPEG', ('RGB',), False),
    'road/tex_gravel_track.jpg': ((1024, 1024), 'JPEG', ('RGB',), False),
}
REPEATS = {
    'diner/counter-repeat-preview.jpg': 'diner/tex_counter_laminate.jpg',
    'diner/checker-repeat-preview.jpg': 'diner/tex_floor_checker.jpg',
    'road/gravel-repeat-preview.jpg': 'road/tex_gravel_track.jpg',
}
MANIFESTS = ['diner/DINER_SURFACES.json', 'diner/DINER_ROAD_PROPS.json']
GALLERY = 'production/round6.html'
BRIEF_REF = '1781093'
SHA_RE = re.compile(r'^[0-9a-fA-F]{64}$')
IMAGE_SUFFIXES = {'.png', '.jpg', '.jpeg', '.webp'}


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def quant_key(tables):
    return tuple((key, tuple(values)) for key, values in sorted((tables or {}).items()))


@lru_cache(maxsize=1)
def pillow_jpeg_tables():
    result = {}
    for quality in range(1, 101):
        stream = BytesIO()
        Image.new('RGB', (8, 8), (110, 150, 130)).save(stream, 'JPEG', quality=quality)
        with Image.open(BytesIO(stream.getvalue())) as im:
            result.setdefault(quant_key(im.quantization), []).append(quality)
    return result


def image_info(path: Path):
    data = path.read_bytes()
    with Image.open(BytesIO(data)) as test:
        test.verify()
    with Image.open(BytesIO(data)) as im:
        im.load()
        icc = im.info.get('icc_profile')
        description = ImageCms.getProfileDescription(ImageCms.ImageCmsProfile(BytesIO(icc))).strip() if icc else None
        result = {
            'sha256': sha(data), 'bytes': len(data), 'dimensions': list(im.size),
            'format': im.format, 'mode': im.mode,
            'icc_bytes': len(icc or b''), 'icc_description': description,
            'icc_sha256': sha(icc) if icc else None,
            'embedded_srgb': bool(description and 'srgb' in description.lower()),
        }
        if 'A' in im.getbands():
            alpha = im.getchannel('A'); hist = alpha.histogram()
            result['alpha'] = {
                'range': list(alpha.getextrema()), 'transparent_pixels': hist[0],
                'opaque_pixels': hist[255], 'partial_pixels': sum(hist[1:255]),
                'pixels': im.width * im.height, 'padding_threshold_rule': 'alpha > threshold', 'padding_by_threshold': {},
            }
            for threshold in [0, 8, 32, 128, 254]:
                bounds = alpha.point(lambda a: 255 if a > threshold else 0).getbbox()
                result['alpha']['padding_by_threshold'][str(threshold)] = (
                    {'left': bounds[0], 'top': bounds[1], 'right': im.width - bounds[2], 'bottom': im.height - bounds[3]}
                    if bounds else None
                )
            edges = [alpha.crop((0, 0, im.width, 1)), alpha.crop((0, im.height-1, im.width, im.height)),
                     alpha.crop((0, 0, 1, im.height)), alpha.crop((im.width-1, 0, im.width, im.height))]
            result['alpha']['outer_edge_max'] = dict(zip(['top', 'bottom', 'left', 'right'], [edge.getextrema()[1] for edge in edges]))
        else:
            result['alpha'] = None
        if im.format == 'JPEG':
            result['jpeg_quantization_compatible_pillow_quality'] = pillow_jpeg_tables().get(quant_key(im.quantization), [])
            result['jpeg_quality_note'] = 'Lik kvantiseringstabell er forenlig med Pillow-standard ved angitt kvalitet, ikke bevis på opprinnelig encoder eller eksakt lagringsparameter.'
            rgb = ImageStat.Stat(im.convert('RGB')); lum = im.convert('L'); stat = ImageStat.Stat(lum); hist = lum.histogram(); n = im.width * im.height
            result['srgb_8bit_luma'] = {'mean': round(stat.mean[0], 3), 'stddev': round(stat.stddev[0], 3), 'range': list(lum.getextrema()),
                                       'dark_0_to_3_fraction': round(sum(hist[:4])/n, 6), 'light_252_to_255_fraction': round(sum(hist[252:])/n, 6)}
            result['mean_rgb'] = [round(v, 3) for v in rgb.mean]
        return result


class Links(HTMLParser):
    def __init__(self):
        super().__init__(); self.refs = []; self.ids = set()

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if 'id' in attrs: self.ids.add(attrs['id'])
        for key in ['href', 'src']:
            if key in attrs: self.refs.append((tag, key, attrs[key]))


class Audit:
    def __init__(self, repo: Path):
        self.repo = repo; self.art = repo/'web/src/assets/art'
        self.errors = []; self.warnings = []; self.metadata = {}; self.hash_checks = []

    def error(self, kind, target, detail):
        self.errors.append({'kind': kind, 'target': str(target), 'detail': detail})

    def warn(self, kind, target, detail):
        self.warnings.append({'kind': kind, 'target': str(target), 'detail': detail})

    def info(self, path: Path):
        key = str(path)
        if key not in self.metadata: self.metadata[key] = image_info(path)
        return self.metadata[key]

    def display(self, path: Path):
        return str(path.relative_to(self.repo)) if path.is_relative_to(self.repo) else str(path)

    def resolve(self, value: str, manifest: Path):
        p = Path(value)
        if p.is_absolute(): return p
        options = [self.repo/p, self.art/p, manifest.parent/p, self.repo/'web'/p]
        return next((candidate for candidate in options if candidate.is_file()), manifest.parent/p)

    def hash_record(self, value, expected, manifest, pointer, reference=None, dimensions=None, nbytes=None, mode=None, fmt=None):
        path = self.resolve(value, manifest)
        snapshot = reference if path.suffix.lower() not in IMAGE_SUFFIXES and reference else None
        try:
            if snapshot:
                git_path = value if value.startswith(('web/', 'Docs/')) else self.display(path)
                data = subprocess.check_output(['git', 'show', snapshot+':'+git_path], cwd=self.repo, stderr=subprocess.PIPE)
            else:
                data = path.read_bytes()
            actual = sha(data); ok = actual.lower() == expected.lower()
            if not ok: self.error('manifest_hash', self.display(manifest)+pointer, f'{value}: forventet {expected}, faktisk {actual}')
            if nbytes is not None and isinstance(nbytes, int) and nbytes != len(data):
                self.error('manifest_bytes', value, f'{nbytes} i manifestet, {len(data)} i filen')
            if isinstance(dimensions, list) and len(dimensions) == 2 and path.suffix.lower() in IMAGE_SUFFIXES:
                actual_size = self.info(path)['dimensions']
                if dimensions != actual_size: self.error('manifest_dimensions', value, f'{dimensions} i manifestet, {actual_size} i filen')
            if path.suffix.lower() in IMAGE_SUFFIXES:
                meta = self.info(path)
                if mode and mode != meta['mode']: self.error('manifest_mode', value, f"{mode} i manifestet, {meta['mode']} i filen")
                if isinstance(fmt, str) and fmt.upper().split()[0] in ['PNG', 'JPEG', 'JPG']:
                    expected_format = 'JPEG' if fmt.upper().startswith(('JPEG', 'JPG')) else 'PNG'
                    if expected_format != meta['format']: self.error('manifest_format', value, f"{fmt} i manifestet, {meta['format']} i filen")
            self.hash_checks.append({'kind': 'file_content', 'manifest': self.display(manifest), 'field': pointer, 'file': self.display(path),
                                     'snapshot': snapshot, 'expected_sha256': expected, 'actual_sha256': actual, 'status': 'PASS' if ok else 'FAIL'})
        except (OSError, ValueError, ImageCms.PyCMSError, subprocess.CalledProcessError) as exc:
            self.error('manifest_source', self.display(manifest)+pointer, f'{value}: {exc}')

    def manifest(self, rel):
        path = self.art/rel; data = path.read_bytes(); doc = json.loads(data)
        start = len(self.hash_checks)

        def walk(obj, pointer='', inherited_file=None, inherited_ref=None):
            if isinstance(obj, list):
                for i, value in enumerate(obj): walk(value, pointer+f'/{i}', inherited_file, inherited_ref)
                return
            if not isinstance(obj, dict): return
            reference = next((obj[k] for k in ['commit', 'sourceCommit', 'source_commit', 'git_ref', 'sourceSnapshotCommit'] if isinstance(obj.get(k), str)), inherited_ref)
            own_file = next((obj[k] for k in ['file', 'path', 'git_path', 'output', 'asset', 'export_file', 'relative_path'] if isinstance(obj.get(k), str)), None)
            source = next((obj[k] for k in ['source_png', 'generated_source_png', 'source_path', 'source_file', 'source'] if isinstance(obj.get(k), str)), None)
            preview = next((obj[k] for k in ['preview', 'repeat_preview'] if isinstance(obj.get(k), str)), None)
            for key, value in obj.items():
                if not isinstance(value, str) or not SHA_RE.fullmatch(value): continue
                field = key.lower().replace('_', '')
                if field in ['iccprofilesha256', 'iccsha256']:
                    target = own_file or inherited_file
                    if not target:
                        self.error('unmapped_manifest_hash', rel+pointer+'/'+key, 'ICC-hash uten tilknyttet bildefil.')
                        continue
                    image_path = self.resolve(target, path); actual = self.info(image_path)['icc_sha256']; ok = actual == value
                    self.hash_checks.append({'kind': 'embedded_icc', 'manifest': self.display(path), 'field': pointer+'/'+key,
                                             'file': self.display(image_path), 'expected_sha256': value, 'actual_sha256': actual, 'status': 'PASS' if ok else 'FAIL'})
                    if not ok: self.error('icc_hash', target, f'Forventet {value}, faktisk {actual}')
                    continue
                elif field in ['sourcesha256', 'generatedsourcesha256']:
                    target = source; dims = obj.get('source_dimensions', obj.get('source_size')); size = obj.get('source_bytes'); mode = obj.get('source_mode'); fmt = obj.get('source_format')
                elif field in ['previewsha256', 'repeatpreviewsha256']:
                    target = preview; dims = obj.get('preview_dimensions'); size = obj.get('preview_bytes'); mode = obj.get('preview_mode'); fmt = obj.get('preview_format')
                elif field in ['sha256', 'outputsha256', 'exportsha256', 'filesha256']:
                    target = own_file or source or inherited_file; dims = obj.get('dimensions', obj.get('size')); size = obj.get('bytes'); mode = obj.get('mode'); fmt = obj.get('format')
                else:
                    self.error('unmapped_manifest_hash', rel+pointer+'/'+key, 'Ukjent hashfelt krever eksplisitt kobling til fil.')
                    continue
                if target: self.hash_record(target, value, path, pointer+'/'+key, reference, dims, size, mode, fmt)
                else: self.error('unmapped_manifest_hash', rel+pointer+'/'+key, 'Hash uten tilknyttet filsti.')
            for key, value in obj.items():
                filename = key if Path(key).suffix.lower() in IMAGE_SUFFIXES else own_file or inherited_file
                if isinstance(value, (dict, list)): walk(value, pointer+'/'+key, filename, reference)

        walk(doc)
        if len(self.hash_checks) == start: self.error('empty_manifest_audit', rel, 'Ingen filhash ble kontrollert.')
        return {'file': rel, 'sha256': sha(data), 'hash_records': len(self.hash_checks)-start}

    def asset(self, rel, spec):
        dimensions, fmt, modes, transparent = spec; data = self.info(self.art/rel)
        start = len(self.errors)
        if data['dimensions'] != list(dimensions): self.error('dimensions', rel, f"Forventet {dimensions}, fikk {data['dimensions']}")
        if data['format'] != fmt or data['mode'] not in modes: self.error('format', rel, f"Forventet {fmt} {modes}, fikk {data['format']} {data['mode']}")
        if not data['embedded_srgb']: self.error('color_profile', rel, 'Mangler lesbar innebygd sRGB ICC-profil.')
        if transparent:
            alpha = data['alpha']
            if not alpha or alpha['range'] != [0, 255] or alpha['partial_pixels'] == 0:
                self.error('alpha', rel, 'Krever ekte RGBA med alfa 0, 255 og mellomverdier for kant/glød.')
            elif any(v > 32 for v in alpha['outer_edge_max'].values()):
                self.warn('alpha_padding', rel, 'Alfa over 32 finnes på ytterkanten. Målt padding følger rapporten; visuell QA må avklare om skilt/glød er klippet.')
        elif data['alpha'] and data['alpha']['range'] != [255, 255]:
            self.error('alpha', rel, 'Dette bildet skal være ugjennomsiktig.')
        if fmt == 'JPEG':
            qualities = data['jpeg_quantization_compatible_pillow_quality']
            if qualities and not any(80 <= q <= 90 for q in qualities): self.error('jpeg_quality', rel, f'Tabeller matcher Pillow {qualities}, utenfor kontrollintervallet 80 til 90.')
            if not qualities: self.warn('jpeg_quality', rel, 'Ingen eksakt standardtabellmatch; kvalitet kan ikke utledes sikkert.')
        return {'file': rel, **data, 'status': 'PASS' if len(self.errors) == start else 'FAIL'}

    def repeat(self, rel, source):
        data = self.info(self.art/rel); start = len(self.errors)
        if data['format'] != 'JPEG' or data['mode'] != 'RGB': self.error('repeat_format', rel, 'Forventet RGB-JPEG.')
        w, h = data['dimensions']; maes = []
        if w != h: self.error('repeat_aspect', rel, 'De tre kvadratiske teksturene skal gi kvadratisk 2x2-visning uten strekk.')
        if w % 2 or h % 2: self.error('repeat_dimensions', rel, '2x2-forhåndsvisningen må kunne deles i fire like rektangler.')
        else:
            with Image.open(self.art/source) as original, Image.open(self.art/rel) as preview:
                tile = original.convert('RGB').resize((w//2, h//2), Image.Resampling.LANCZOS)
                for x, y in [(0, 0), (w//2, 0), (0, h//2), (w//2, h//2)]:
                    quadrant = preview.convert('RGB').crop((x, y, x+w//2, y+h//2))
                    maes.append(round(sum(ImageStat.Stat(ImageChops.difference(tile, quadrant)).mean)/3, 4))
            if max(maes) > 4.0: self.error('repeat_copies', rel, f'Kvadrantene avviker fra vanlige kopier av eksporten: MAE {maes}; kontrollgrense 4/255.')
        return {'file': rel, 'source': source, **data, 'quadrant_mean_absolute_rgb_error_0_255': maes,
                'status': 'PASS' if len(self.errors) == start else 'FAIL',
                'limit': 'Kontrollerer ordinære 2x2-kopier med rom for JPEG/resampling. Beviser ikke visuelt sømløs repeat, motiv eller antall fliser.'}

    def gallery(self):
        path = self.art/GALLERY; data = path.read_bytes(); parsed = Links(); parsed.feed(data.decode('utf-8')); refs = []; targets = set()
        for tag, attr, value in parsed.refs:
            url = urlsplit(value)
            if url.scheme or url.netloc:
                refs.append({'tag': tag, 'attribute': attr, 'url': value, 'status': 'EXTERNAL_NOT_CHECKED'})
                if attr == 'src': self.error('external_gallery_asset', GALLERY, value)
                continue
            target = (path.parent/unquote(url.path)).resolve() if url.path else path
            valid = target.is_file() and not url.path.startswith('/')
            if valid and url.fragment:
                p = parsed
                if target != path: p = Links(); p.feed(target.read_text())
                valid = unquote(url.fragment) in p.ids
            refs.append({'tag': tag, 'attribute': attr, 'url': value, 'target': self.display(target), 'status': 'PASS' if valid else 'FAIL'})
            if valid: targets.add(target)
            else: self.error('gallery_reference', GALLERY, value)
        for rel in [*SPECS, *REPEATS]:
            if self.art/rel not in targets: self.error('gallery_coverage', GALLERY, f'Mangler direkte lenke eller bilde til {rel}')
        return {'file': GALLERY, 'sha256': sha(data), 'references': refs}

    def run(self):
        required = [*SPECS, *REPEATS, *MANIFESTS, GALLERY]
        missing = [rel for rel in required if not (self.art/rel).is_file()]
        report = {'schema': 1, 'checked_utc': datetime.now(timezone.utc).isoformat(), 'pillow_version': PILLOW_VERSION,
                  'scope': 'Teknisk kontroll av seks runde 6-assets, to manifester, tre 2x2-forhåndsvisninger og galleriets lokale referanser.',
                  'brief': {'ref': BRIEF_REF, 'path': 'web/ART_BRIEF.md'}, 'missing': missing,
                  'visual_qa': {'status': 'NOT_PERFORMED_BY_SCRIPT', 'items': ['Synlige sømmer ved vanlig repeat', '8x8 fliser i gulvet', 'Neonform, glød og visuell padding', 'Tom tekstflate og fravær av feil tekst/merkevarer', 'Avismotiv, hjulspor og stil']},
                  'runtime': 'NOT_TESTED: Ikke integrasjon, spilling eller maskinvareytelse.'}
        if missing: return {**report, 'status': 'INCOMPLETE', 'errors': [], 'warnings': []}
        initial = {rel: sha((self.art/rel).read_bytes()) for rel in required}
        try:
            commit = subprocess.check_output(['git', 'rev-parse', BRIEF_REF], cwd=self.repo, text=True).strip()
            brief = subprocess.check_output(['git', 'show', commit+':web/ART_BRIEF.md'], cwd=self.repo)
            report['brief'].update(commit=commit, sha256=sha(brief))
            report['assets'] = [self.asset(rel, spec) for rel, spec in SPECS.items()]
            report['manifests'] = [self.manifest(rel) for rel in MANIFESTS]
            covered = {c['file'] for c in self.hash_checks if c['kind'] == 'file_content'}
            for rel in [*SPECS, *REPEATS]:
                if self.display(self.art/rel) not in covered: self.error('manifest_coverage', rel, 'Ingen manifesthash kontrollert for denne leverte filen.')
            report['manifest_hash_checks'] = self.hash_checks
            report['repeat_previews'] = [self.repeat(rel, source) for rel, source in REPEATS.items()]
            report['gallery'] = self.gallery()
        except (OSError, ValueError, ImageCms.PyCMSError, subprocess.CalledProcessError) as exc:
            self.error('audit_exception', 'round6', str(exc))
        changed = [rel for rel, digest in initial.items() if not (self.art/rel).is_file() or sha((self.art/rel).read_bytes()) != digest]
        if changed: self.error('concurrent_changes', ', '.join(changed), 'Filene ble endret under kontrollen. Kjør på nytt når leveransen er stabil.')
        report.update(errors=self.errors, warnings=self.warnings,
                      status='FAIL' if self.errors else 'PASS_WITH_WARNINGS' if self.warnings else 'PASS')
        return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--no-write', action='store_true')
    args = parser.parse_args(); repo = args.repo.resolve(); report = Audit(repo).run()
    output = repo/'web/evidence/diner-road-2026-10-04/asset-verification.json'
    if report['status'] != 'INCOMPLETE' and not args.no_write:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps({'status': report['status'], 'missing': report['missing'], 'errors': report['errors'], 'warnings': report['warnings'],
                      'report': str(output) if report['status'] != 'INCOMPLETE' and not args.no_write else 'Ikke skrevet'}, ensure_ascii=False, indent=2))
    return 0 if report['status'].startswith('PASS') else 2 if report['status'] == 'INCOMPLETE' else 1


if __name__ == '__main__':
    raise SystemExit(main())
