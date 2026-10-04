#!/usr/bin/env python3
"""Runde 10: les uendrede JPEG-er, kontroller format og lag kun QA-kopier.

Fra web/: python3 tools/round10_qa.py
Valgfritt: --art-dir PATH --out-dir PATH --kapitler PATH --nightshift PATH
--self-test kontrollerer egne tekst-, rektangel- og metadataregler uten assets.

PASS fra automatikk beviser ikke personantall, ansikter, komet, tekstfrihet,
hullantall eller historisk uttrykk. Disse står som UNVERIFIED i rapporten.
Ingen produksjonsbilder eller runtimefiler endres.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import re
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont, __version__ as PIL_VERSION

try:
    from PIL import ImageCms
except ImportError:
    ImageCms = None

WEB = Path(__file__).resolve().parents[1]
ASSETS = {
    'photo': ('photo_1947_master.jpg', (2400, 1600)),
    'poster': ('poster_halley_1986_blank.jpg', (1024, 1536)),
    'fanfold_1986': ('fanfold_1986.jpg', (1024, 1186)),
    'fanfold_1947': ('fanfold_1947.jpg', (1024, 1186)),
}
CROPS = {'A': (600, 300, 960, 720), 'B': (0, 220, 1840, 1380), 'C': (0, 0, 2400, 1600)}
POSTER_ZONES = {
    'title': (60, 50, 904, 240), 'note': (50, 300, 330, 220),
    'chart': (70, 1010, 884, 280), 'lines': (60, 1310, 904, 190),
}
EXPECTED_POSTER = [
    "HALLEY'S COMET", 'APRIL 1986', 'CLOSEST TO EARTH APRIL 11',
    'LOOK LOW IN THE SOUTH', 'EARLY APRIL: BEFORE DAWN',
    'AFTER THE 12TH: AROUND MIDNIGHT', 'GET AWAY FROM TOWN LIGHTS',
]
FONT_DIRS = [Path('/usr/share/fonts/truetype/dejavu'), Path('/usr/share/fonts/truetype/liberation')]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def check(name, status, detail=None):
    return {'check': name, 'status': status, 'detail': detail}


def aggregate(checks):
    statuses = {c['status'] for c in checks}
    return 'FAIL' if 'FAIL' in statuses else 'UNVERIFIED' if 'UNVERIFIED' in statuses else 'PASS'


def rect_box(rect):
    x, y, w, h = rect
    return x, y, x + w, y + h


def rect_inside(rect, size):
    x, y, w, h = rect
    return x >= 0 and y >= 0 and w > 0 and h > 0 and x + w <= size[0] and y + h <= size[1]


def canonical_text(kapitler_path, nightshift_path):
    chapters = kapitler_path.read_text()
    section = re.search(r'\*\*4\. Halley-plakaten\*\*.*?```\s*\n(.*?)\n```', chapters, re.S)
    if not section:
        raise ValueError('Halley-plakatens kodeblokk mangler i KAPITLER.md')
    lines = [line.strip() for line in section.group(1).splitlines() if line.strip()]
    if lines != EXPECTED_POSTER:
        raise ValueError('Plakatens sju linjer har endret seg; kontroller gjeldende brief før ny QA')
    note = re.search(r'Wards hånd: \*(.*?)\*', chapters)
    if not note:
        raise ValueError('Wards notat mangler i KAPITLER.md')
    source = nightshift_path.read_text()
    binder = re.search(r'export const BINDER:.*?\bpage:\s*`(.*?)`', source, re.S)
    if not binder:
        raise ValueError('BINDER.page mangler i nightshift.ts')
    sections = re.split(r'\n(?:- ){5,}-\s*\n', binder.group(1))
    if len(sections) != 5 or 'DATE    09/22/81' not in sections[1] or 'DATE    07/--/47' not in sections[-1]:
        raise ValueError('Permasider er endret; prøveteksten kan ikke velges sikkert')
    header = '\n'.join(sections[0].splitlines()[:3])
    return {'poster': lines, 'ward_note': note.group(1),
            'fanfold_1986': sections[0].strip() + '\n\n' + sections[1].strip(),
            'fanfold_1947': header + '\n\n' + sections[-1].strip(),
            'sources': {'kapitler': {'path': str(kapitler_path), 'sha256': sha(kapitler_path)},
                        'nightshift': {'path': str(nightshift_path), 'sha256': sha(nightshift_path)}}}


def srgb_check(icc):
    if not icc:
        return check('valid sRGB ICC', 'FAIL', 'JPEG mangler ICC-profil')
    if ImageCms is None:
        return check('valid sRGB ICC', 'UNVERIFIED', 'Pillow ImageCms er utilgjengelig')
    try:
        profile = ImageCms.ImageCmsProfile(io.BytesIO(icc))
        name = ImageCms.getProfileName(profile).strip()
        description = ImageCms.getProfileDescription(profile).strip()
        space = profile.profile.xcolor_space.strip()
        # En gyldig RGB-profil med sRGB-navn og praktisk identisk overføring
        # til lcms sRGB. Testbildet er kun en intern fargeromsprobe.
        levels = (0, 32, 64, 96, 128, 160, 192, 224, 255)
        samples = [(r, g, b) for r in levels for g in levels for b in levels]
        probe = Image.new('RGB', (len(samples), 1)); probe.putdata(samples)
        transformed = ImageCms.profileToProfile(probe, profile, ImageCms.createProfile('sRGB'), outputMode='RGB')
        maximum = max(high for low, high in ImageChops.difference(probe, transformed).getextrema())
        valid = space == 'RGB' and 'srgb' in (name + description).lower() and maximum <= 2
        return check('valid sRGB ICC', 'PASS' if valid else 'FAIL',
                     {'name': name, 'description': description, 'colour_space': space,
                      'ICC_sha256': hashlib.sha256(icc).hexdigest(),
                      'sampled_identity_max_byte_delta': maximum, 'sample_count': len(samples)})
    except Exception as exc:
        return check('valid sRGB ICC', 'FAIL', str(exc))


def metadata_checks(meta, expected_size, master=False):
    result = [
        check('actual JPEG format', 'PASS' if meta['format'] == 'JPEG' else 'FAIL', meta['format']),
        check('exact dimensions', 'PASS' if tuple(meta['size']) == tuple(expected_size) else 'FAIL',
              {'actual': meta['size'], 'expected': list(expected_size)}),
        check('RGB channels', 'PASS' if meta['mode'] == 'RGB' else 'FAIL', meta['mode']),
        check('no alpha', 'PASS' if 'A' not in meta['bands'] and not meta.get('transparency') else 'FAIL', meta['bands']),
        check('file bytes measured', 'PASS', meta['bytes']),
    ]
    if master:
        result.append(check('master <= 1200000 bytes', 'PASS' if meta['bytes'] <= 1_200_000 else 'FAIL', meta['bytes']))
    return result


def grayscale_measure(im):
    red, green, blue = im.split()
    difference = ImageChops.lighter(ImageChops.difference(red, green), ImageChops.difference(red, blue))
    difference = ImageChops.lighter(difference, ImageChops.difference(green, blue))
    return {'max_channel_delta': difference.getextrema()[1],
            'pixels_outside_tolerance': sum(difference.histogram()[3:])}


def inspect_asset(path, size, master=False):
    if not path.is_file():
        return {'path': str(path), 'checks': [check('file exists', 'FAIL', 'Mangler')], 'status': 'FAIL'}, None
    try:
        with Image.open(path) as opened:
            opened.load()
            meta = {'path': str(path), 'sha256': sha(path), 'bytes': path.stat().st_size,
                    'format': opened.format, 'size': list(opened.size), 'mode': opened.mode,
                    'bands': list(opened.getbands()), 'transparency': 'transparency' in opened.info}
            checks = metadata_checks(meta, size, master)
            checks.append(srgb_check(opened.info.get('icc_profile')))
            im = opened.convert('RGB')  # Kun QA-kopi; ingen produksjonsfil skrives.
        if master:
            gray = grayscale_measure(im)
            checks.append(check('master RGB grayscale within 2', 'PASS' if gray['max_channel_delta'] <= 2 else 'FAIL', gray))
        meta['checks'] = checks; meta['status'] = aggregate(checks)
        return meta, im
    except Exception as exc:
        return {'path': str(path), 'checks': [check('image readable', 'FAIL', str(exc))], 'status': 'FAIL'}, None


class QA:
    def __init__(self, out):
        self.out = out; self.outputs = []; self.fonts = []
        out.mkdir(parents=True, exist_ok=True)

    def font(self, size, mono=False, bold=False):
        candidates = ['DejaVuSansMono.ttf', 'LiberationMono-Regular.ttf'] if mono else (
            ['DejaVuSans-Bold.ttf', 'LiberationMono-Bold.ttf'] if bold else ['DejaVuSans.ttf', 'LiberationMono-Regular.ttf'])
        for directory in FONT_DIRS:
            for candidate in candidates:
                path = directory / candidate
                if path.is_file():
                    self.fonts.append(str(path))
                    return ImageFont.truetype(str(path), size)
        self.fonts.append('Pillow default, substitution')
        try:
            return ImageFont.load_default(size=size)
        except TypeError:
            return ImageFont.load_default()

    def save(self, im, name, role, source=None, rect=None, scale=None):
        path = self.out / name
        # PNG beholder de faktiske dekodede JPEG-pikslene i utsnittene.
        im.save(path, format='PNG')
        item = {'path': str(path), 'sha256': sha(path), 'bytes': path.stat().st_size,
                'size': list(im.size), 'mode': im.mode, 'role': role}
        if source is not None: item['source'] = str(source)
        if rect is not None: item['source_rectangle_xywh'] = list(rect)
        if scale is not None: item['scale'] = scale
        self.outputs.append(item)
        return item


def photo_qa(qa, im, source):
    frames = im.copy(); draw = ImageDraw.Draw(frames); font = qa.font(32, bold=True)
    colours = {'A': (255, 80, 50), 'B': (20, 210, 255), 'C': (100, 255, 80)}
    for key, rect in CROPS.items():
        x, y, right, bottom = rect_box(rect)
        draw.rectangle((x, y, right - 1, bottom - 1), outline=colours[key], width=5)
        label = f'{key}: {rect[0]},{rect[1]},{rect[2]},{rect[3]}'
        draw.text((x + 12, y + 12), label, fill=colours[key], font=font, stroke_width=2, stroke_fill=(0, 0, 0))
        qa.save(im.crop(rect_box(rect)), f'photo_crop_{key}.png', f'Faktisk utsnitt {key}, uten nye elementer', source, rect)
    qa.save(frames, 'photo_master_frames.png', 'A/B/C-rammer på QA-kopi', source)
    # Hele høyden på B, siste 240 kildepiksler. Ingen del av kanten er utelatt.
    strip_rect = (1600, 220, 240, 1380)
    strip = im.crop(rect_box(strip_rect)).resize((480, 2760), Image.Resampling.NEAREST)
    qa.save(strip, 'photo_B_right_edge_zoom.png', 'B høyrekant x=1840, hele høyden, 2x nærmeste nabo', source, strip_rect, 2)


def fit_text(draw, text, box, qa, size, fill, bold=False, mono=False):
    x, y, w, h = box
    while size > 8:
        font = qa.font(size, mono, bold)
        bounds = draw.textbbox((0, 0), text, font=font)
        if bounds[2] - bounds[0] <= w and bounds[3] - bounds[1] <= h: break
        size -= 1
    bounds = draw.textbbox((0, 0), text, font=font)
    draw.text((x + (w - (bounds[2] - bounds[0])) / 2 - bounds[0], y - bounds[1]), text, font=font, fill=fill)
    return {'text': text, 'zone': list(box), 'font_size': size,
            'fits': bounds[2] - bounds[0] <= w and bounds[3] - bounds[1] <= h}


def poster_qa(qa, im, source, text):
    zones = im.copy(); draw = ImageDraw.Draw(zones); font = qa.font(28, bold=True)
    colours = [(255, 90, 90), (255, 225, 20), (20, 200, 255), (90, 255, 120)]
    for (name, rect), colour in zip(POSTER_ZONES.items(), colours):
        x, y, right, bottom = rect_box(rect)
        draw.rectangle((x, y, right - 1, bottom - 1), outline=colour, width=4)
        draw.text((x + 8, y + 6), name, font=font, fill=colour, stroke_width=1, stroke_fill=(0, 0, 0))
    qa.save(zones, 'poster_zones.png', 'Fire eksakte skrivefelt, kun QA-markering', source)
    sample = im.copy(); draw = ImageDraw.Draw(sample); records = []
    records.append(fit_text(draw, text['poster'][0], (60, 62, 904, 112), qa, 92, '#f4e7ca', True))
    records.append(fit_text(draw, text['poster'][1], (60, 184, 904, 78), qa, 58, '#f4e7ca', True))
    for i, line in enumerate(text['poster'][2:]):
        records.append(fit_text(draw, line, (60, 1310 + i * 38, 904, 35), qa, 30, '#f4e7ca'))
    # Kartets tekst er en lesbarhetsprøve. Innkoblingens endelige plassering eies av Claude.
    chart_font = qa.font(30); ink = '#2b2a27'
    for x, label in [(108, 'SW'), (502, 'S'), (865, 'SE')]:
        draw.text((x, 1250), label, font=chart_font, fill=ink)
    for x, y, date in [(790, 1110, '5'), (640, 1170, '10'), (480, 1190, '14'), (330, 1126, '20')]:
        draw.ellipse((x - 6, y - 6, x + 6, y + 6), fill=ink)
        draw.text((x - 8, y - 44), date, font=chart_font, fill=ink)
    # Notatets eksakte tekst brytes kun for å få plass i det bestilte QA-feltet.
    draw.rectangle((58, 312, 372, 512), fill='#efe39a')
    note_font = qa.font(20); words = text['ward_note'].split(); lines = []; current = ''
    for word in words:
        candidate = (current + ' ' + word).strip()
        if draw.textlength(candidate, font=note_font) > 296 and current:
            lines.append(current); current = word
        else: current = candidate
    if current: lines.append(current)
    for i, line in enumerate(lines): draw.text((68, 318 + i * 26), line, fill='#26324f', font=note_font)
    qa.save(sample, 'poster_sample_text.png', 'Eksakt KAPITLER-tekst med diagnostisk prøveoppsett', source)
    qa.save(im.resize((256, 384), Image.Resampling.LANCZOS), 'poster_blank_256x384.png', 'Tekstfri bakgrunn i veggstørrelse', source, scale=.25)
    qa.save(sample.resize((256, 384), Image.Resampling.LANCZOS), 'poster_sample_256x384.png', 'Lesbarhetsprøve i veggstørrelse, ikke runtime', source, scale=.25)
    return {'exact_title_and_five_lines': text['poster'], 'ward_note': text['ward_note'],
            'text_fit': records, 'ward_note_lines': lines,
            'note_fit': len(lines) * 26 <= 194,
            'layout_status': 'diagnostic only; actual runtime layout UNVERIFIED'}


def fanfold_qa(qa, images, sources, text):
    sheet = Image.new('RGB', (2048, 1226), '#242424'); draw = ImageDraw.Draw(sheet)
    label_font = qa.font(26, bold=True); font = qa.font(27, mono=True); fits = []
    for i, key in enumerate(['fanfold_1986', 'fanfold_1947']):
        page = images[key].copy(); page_draw = ImageDraw.Draw(page)
        lines = text[key].splitlines(); line_height = 35
        max_width = max((page_draw.textlength(line[2:] if line.startswith('~ ') else line, font=font) for line in lines), default=0)
        for line_index, line in enumerate(lines):
            pencil = line.startswith('~ ')
            page_draw.text((90, 65 + line_index * line_height), line[2:] if pencil else line,
                           fill='#4c4840' if pencil else '#25221d', font=font)
        sheet.paste(page, (i * 1024, 40))
        draw.text((i * 1024 + 18, 6), key, fill='white', font=label_font)
        fits.append({'page': key, 'text': text[key], 'max_line_width': max_width,
                     'fits_text_field': max_width <= 844 and 65 + len(lines) * line_height <= 1130,
                     'source': text['sources']['nightshift']})
    qa.save(sheet, 'fanfold_side_by_side_sample.png', 'To uendrede arkbakgrunner med faktisk permatekst; QA-kopi')
    return fits


def row_candidates(im, side, threshold):
    """Mål mørke radkomponenter i hullstripen, uten antatt antall eller fase."""
    x0, x1 = (8, 46) if side == 'left' else (978, 1016)
    w = x1 - x0; h = im.height
    strip = im.crop((x0, 0, x1, h)).convert('L')
    read_pixels = getattr(strip, 'get_flattened_data', strip.getdata)
    data = list(read_pixels())
    dark_counts = [sum(v < threshold for v in data[y*w:(y+1)*w]) for y in range(h)]
    active = [count >= 3 for count in dark_counts]
    # Kun enkeltpikselbrudd fra JPEG fylles i den numeriske radindikatoren.
    active = [value or (0 < y < h-1 and active[y-1] and active[y+1]) for y, value in enumerate(active)]
    runs = []; start = None
    for y, value in enumerate(active + [False]):
        if value and start is None: start = y
        if not value and start is not None:
            area = sum(dark_counts[start:y]); height = y - start
            if 4 <= height <= 42 and area >= 20:
                centre = sum(row * dark_counts[row] for row in range(start, y)) / area
                runs.append({'center_y': round(centre, 3), 'start_y': start, 'end_y_exclusive': y,
                             'dark_pixel_count': area, 'height': height})
            start = None
    return {'threshold_luma': round(threshold, 3), 'window_x': [x0, x1],
            'candidate_count': len(runs), 'candidates': runs}


def hole_analysis(im):
    """Flerterskelestimat. Ingen rad settes inn for å nå bestilte 22 hull."""
    histogram = im.crop((90, 60, 934, 1130)).convert('L').histogram()
    half = sum(histogram) / 2; total = 0; median = 0
    for median, count in enumerate(histogram):
        total += count
        if total >= half: break
    result = {'paper_reference_median_luma': median, 'expected_count_per_side': 22,
              'method': 'Actual dark row components in narrow edge windows, five relative thresholds; no count or phase fitting.',
              'semantic_status': 'UNVERIFIED: dark row candidates do not prove physical holes', 'sides': {}}
    for side in ('left', 'right'):
        profiles = [row_candidates(im, side, median * factor) for factor in (.50, .65, .75, .85, .90)]
        groups = {}
        for profile in profiles:
            if profile['candidate_count']:
                groups.setdefault(profile['candidate_count'], []).append(profile)
        if not groups:
            result['sides'][side] = {'status': 'UNVERIFIED', 'reason': 'No robust dark row components', 'threshold_profiles': profiles}
            continue
        count, dominant = max(groups.items(), key=lambda pair: len(pair[1]))
        conflicting = any(len(values) >= 2 for other, values in groups.items() if other != count)
        centres = [[item['center_y'] for item in profile['candidates']] for profile in dominant]
        drift = max((max(values) - min(values) for values in zip(*centres)), default=0)
        stable = len(dominant) >= 3 and not conflicting and drift <= 3
        selected = dominant[len(dominant)//2]
        ys = [item['center_y'] for item in selected['candidates']]
        result['sides'][side] = {
            'status': ('PASS' if count == 22 else 'FAIL') if stable else 'UNVERIFIED',
            'reason': 'Stable measured candidate rows' if stable else 'Threshold count or centre ambiguity; inspect manually',
            'selected_count': count, 'selected_centers_y': ys,
            'consecutive_spacing_y': [round(b-a, 3) for a, b in zip(ys, ys[1:])],
            'threshold_agreement_count': len(dominant), 'maximum_center_drift_pixels': round(drift, 3),
            'threshold_profiles': profiles}
    result['candidate_measurement_status'] = aggregate([check(side, value['status']) for side, value in result['sides'].items()])
    return result


def compare_hole_rows(analyses):
    measurements = [(f'{page}/{side}', analysis['sides'][side])
                    for page, analysis in analyses.items() for side in ('left', 'right')]
    if not all(item.get('selected_count') == 22 and item['status'] == 'PASS' for _, item in measurements):
        return {'status': 'UNVERIFIED', 'reason': 'All four sides need unambiguous 22-row measurements before row pairing.'}
    reference_name, reference = measurements[0]; rows = reference['selected_centers_y']
    comparisons = []
    for name, other in measurements[1:]:
        delta = [round(b-a, 3) for a, b in zip(rows, other['selected_centers_y'])]
        maximum = max(map(abs, delta), default=0)
        comparisons.append({'reference': reference_name, 'other': name, 'row_deltas_pixels': delta,
                            'max_abs_delta_pixels': maximum, 'status': 'PASS' if maximum <= 4 else 'FAIL'})
    return {'status': aggregate([check(item['other'], item['status']) for item in comparisons]),
            'tolerance_pixels': 4, 'comparisons': comparisons,
            'semantic_status': 'UNVERIFIED: confirms measured row alignment, not hole authenticity'}


def manual_checks():
    requirements = [
        ('photo two ranchers', 'Nøyaktig to hattesilhuetter nederst til venstre, ingen ansikter.'),
        ('photo three surveyors', 'Nora, Tomás og en uskarp tredje med refleksbånd, stativ og stang helt til høyre.'),
        ('photo no identifiable faces', 'Alle personer uten gjenkjennelige ansikter.'),
        ('photo comet', 'Svak komet ved (1380,840), kort hale opp/venstre; lesbar i faktisk utsnitt A.'),
        ('photo calm light and mesa', 'Rolig rund glød ved (900,420); flat mesa rundt y960-1000, ingen farkost eller stråler.'),
        ('crop A semantic contents', 'Lyset, mesatoppen og kometen; ingen ranchere eller landmålere.'),
        ('crop B surveyor exclusion', 'Ingen landmåler, stativdel, målestang, skygge eller uskarphet i B, særlig ved x1840.'),
        ('crop C full reveal', 'Alle fem personer, de tre landmålerne som eneste gruppe på tre.'),
        ('all assets text-free', 'Ingen genererte bokstaver, tall, bildetekst, logo eller vannmerke.'),
        ('poster quiet writing zones', 'Tittel, lapp og fem linjer er rolige/mørke; komet/halen holdes unna lappen.'),
        ('poster blank inset chart', 'Kremfarget tom boks med tynn mørk ramme og svak horisont rundt y1240.'),
        ('fanfold 22 holes each side', '22 hull per side på begge ark; samme posisjoner, ingen falske hulldotter.'),
        ('fanfold folds and perforations', 'Rivelinjer rundt x54/x970, halvperforerte bretter oppe/nede.'),
        ('fanfold comparative age', '1986 rent/grønnstripet; 1947 tydelig eldre, ett revnet hull, rolig skrivefelt.'),
        ('historical style and raster', '1947-avisraster og 1986-plakat/papir vurderes visuelt, ingen påstand fra metadata.'),
        ('runtime integration and hardware', 'Claude integrerer og tester; dette verktøyet måler ikke spill eller maskinvare.'),
    ]
    return [check(name, 'UNVERIFIED', detail) for name, detail in requirements]


def self_test(kapitler, nightshift):
    assert all(rect_inside(rect, (2400, 1600)) for rect in CROPS.values())
    assert CROPS['A'][2] * 3 == CROPS['A'][3] * 4
    assert CROPS['B'][2] * 3 == CROPS['B'][3] * 4
    assert not rect_inside((-1, 0, 2, 2), (2, 2))
    assert not rect_inside((0, 0, 3, 2), (2, 2))
    text = canonical_text(kapitler, nightshift)
    assert text['poster'] == EXPECTED_POSTER and len(text['poster'][2:]) == 5
    good = {'format': 'JPEG', 'size': [2400, 1600], 'mode': 'RGB', 'bands': ['R', 'G', 'B'], 'bytes': 1_200_000}
    assert aggregate(metadata_checks(good, (2400, 1600), True)) == 'PASS'
    for changes in [{'format': 'PNG'}, {'size': [2399, 1600]}, {'mode': 'L'}, {'bands': ['R', 'G', 'B', 'A']}, {'bytes': 1_200_001}]:
        assert aggregate(metadata_checks(dict(good, **changes), (2400, 1600), True)) == 'FAIL'
    assert srgb_check(None)['status'] == 'FAIL'
    assert srgb_check(b'invalid ICC')['status'] in ('FAIL', 'UNVERIFIED')
    if ImageCms is not None:
        assert srgb_check(ImageCms.ImageCmsProfile(ImageCms.createProfile('sRGB')).tobytes())['status'] == 'PASS'
    # Tiny in-memory numeric fixtures, never generated or saved as game art.
    probe = Image.new('RGB', (2, 1)); probe.putdata([(10, 11, 12), (0, 0, 0)])
    assert grayscale_measure(probe) == {'max_channel_delta': 2, 'pixels_outside_tolerance': 0}
    probe.putdata([(10, 10, 13), (0, 0, 0)])
    assert grayscale_measure(probe) == {'max_channel_delta': 3, 'pixels_outside_tolerance': 1}
    assert all(item['status'] == 'UNVERIFIED' for item in manual_checks())
    print('PASS: crop geometry, canonical sample text, positive/negative metadata, ICC, grayscale boundary and honest manual status')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--art-dir', type=Path, default=WEB / 'src/assets/art/docs')
    parser.add_argument('--out-dir', type=Path, default=WEB / 'src/assets/art/production/round10_qa')
    parser.add_argument('--kapitler', type=Path, default=WEB / 'KAPITLER.md')
    parser.add_argument('--nightshift', type=Path, default=WEB / 'src/story/nightshift.ts')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test(args.kapitler, args.nightshift); return 0
    report = {'schema': 'signal47-round10-qa-v1', 'created_utc': datetime.now(timezone.utc).isoformat(),
              'tool': {'path': str(Path(__file__).resolve()), 'sha256': sha(__file__), 'Pillow': PIL_VERSION},
              'assets': {}, 'crop_rectangles_xywh': CROPS, 'poster_zones_xywh': POSTER_ZONES,
              'manual_checks': manual_checks(), 'limitations': [
                  'QA-kopier viser diagnostisk tekst, ikke Claudes faktiske runtime-oppsett.',
                  'ICC-kontrollen validerer profil og testoverføring, ikke motivets opphav eller historiske riktighet.',
                  'Ingen OCR eller persondetektor; motivkrav krever visuell inspeksjon. Hulldiagnostikken måler kun mørke radkandidater.',
                  'PNG-utsnittene er faktiske dekodede JPEG-piksler; produksjons-JPEG-ene endres aldri.']}
    images = {}; sources = {}; automatic = []
    for key, (filename, size) in ASSETS.items():
        path = args.art_dir / filename; sources[key] = path
        meta, im = inspect_asset(path, size, key == 'photo')
        report['assets'][key] = meta; automatic.extend(meta['checks'])
        if im is not None and im.size == size: images[key] = im
    try:
        text = canonical_text(args.kapitler, args.nightshift)
        report['text_sources'] = text['sources']; automatic.append(check('canonical sample text extraction', 'PASS'))
    except Exception as exc:
        text = None; automatic.append(check('canonical sample text extraction', 'FAIL', str(exc)))
    qa = QA(args.out_dir)
    if 'photo' in images: photo_qa(qa, images['photo'], sources['photo'])
    if 'poster' in images and text:
        report['poster_sample'] = poster_qa(qa, images['poster'], sources['poster'], text)
        fits = all(item['fits'] for item in report['poster_sample']['text_fit']) and report['poster_sample']['note_fit']
        automatic.append(check('QA poster sample text fits specified zones', 'PASS' if fits else 'FAIL', report['poster_sample']['text_fit']))
    if all(key in images for key in ('fanfold_1986', 'fanfold_1947')) and text:
        report['fanfold_samples'] = fanfold_qa(qa, images, sources, text)
        automatic.append(check('QA binder sample text fits specified fields', 'PASS' if all(item['fits_text_field'] for item in report['fanfold_samples']) else 'FAIL', report['fanfold_samples']))
    # Innholdsdiagnostikk holdes separat fra de tekniske formatkravene.
    # En tvetydig terskel får aldri et oppfunnet 22-hulls PASS.
    hole_reports = {key: hole_analysis(images[key]) for key in ('fanfold_1986', 'fanfold_1947') if key in images}
    if hole_reports:
        report['fanfold_hole_diagnostics'] = hole_reports
        if len(hole_reports) == 2: report['fanfold_row_alignment'] = compare_hole_rows(hole_reports)
    # Hashene etter QA skal fortsatt være nøyaktig de som ble lest før QA.
    for meta in report['assets'].values():
        if 'sha256' in meta:
            automatic.append(check('production input unchanged: ' + Path(meta['path']).name,
                                   'PASS' if sha(meta['path']) == meta['sha256'] else 'FAIL'))
    report['qa_outputs'] = qa.outputs; report['sample_fonts'] = sorted(set(qa.fonts))
    report['automatic_status'] = aggregate(automatic); report['automatic_checks'] = automatic
    report['delivery_status'] = 'FAIL' if report['automatic_status'] == 'FAIL' else 'UNVERIFIED'
    report_path = args.out_dir / 'round10_report.json'
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({'report': str(report_path), 'automatic': report['automatic_status'],
                      'manual': 'UNVERIFIED', 'QA_files': len(qa.outputs)}, ensure_ascii=False))
    return 1 if report['automatic_status'] == 'FAIL' else 2 if report['automatic_status'] == 'UNVERIFIED' else 0


if __name__ == '__main__':
    sys.exit(main())
