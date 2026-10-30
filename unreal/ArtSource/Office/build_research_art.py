"""Draw the editable array worksheet from the actual 27 manifest positions.

Ordinary Python + Pillow only; no Blender, renderer, API or image-atlas mutation.
The same drawing primitives produce SVG and PNG. SVG embeds only the shared
paper background; all diagram lines, symbols, labels and notes remain editable.
"""
import argparse
import base64
import hashlib
import html
import io
import json
import math
from pathlib import Path
import random

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
DEFAULT_MANIFEST = HERE.parents[1] / 'Signal47/Content/WebData/manifest.json'
WIDTH, HEIGHT = 1600, 1152
INK = (54, 58, 55)
RULE = (126, 126, 111)
GRID = (177, 174, 153)
PENCIL = (111, 108, 95)
FONTS = {
    'mono': Path('/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf'),
    'bold': Path('/usr/share/fonts/truetype/liberation/LiberationMono-Bold.ttf'),
    'note': Path('/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Oblique.ttf'),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rgb(colour):
    return '#%02x%02x%02x' % colour


def paper():
    rng = random.Random(47026)
    image = Image.new('RGB', (400, 288))
    pixels = []
    for y in range(288):
        for x in range(400):
            grain = rng.randint(-4, 4) + round(1.5 * math.sin(x / 63) + y / 155)
            edge = 2 if x < 5 or x > 394 or y < 4 or y > 283 else 0
            pixels.append((226 + grain - edge, 218 + grain - edge, 198 + grain - edge))
    image.putdata(pixels)
    return image.resize((WIDTH, HEIGHT), Image.Resampling.BICUBIC)


class Drawing:
    def __init__(self, background):
        self.image = background.copy()
        self.draw = ImageDraw.Draw(self.image)
        self.svg = []
        self.fonts = {}
        self.elements = []
        stream = io.BytesIO()
        background.save(stream, format='PNG')
        encoded = base64.b64encode(stream.getvalue()).decode()
        self.svg.append(f'<image x="0" y="0" width="{WIDTH}" height="{HEIGHT}" href="data:image/png;base64,{encoded}"/>')

    def font(self, size, style):
        key = (size, style)
        if key not in self.fonts:
            self.fonts[key] = ImageFont.truetype(str(FONTS[style]), size)
        return self.fonts[key]

    def line(self, points, colour=RULE, width=1):
        self.draw.line(points, fill=colour, width=max(1, round(width)), joint='curve')
        coords = ' '.join(f'{x:.2f},{y:.2f}' for x, y in points)
        self.svg.append(f'<polyline points="{coords}" fill="none" stroke="{rgb(colour)}" stroke-width="{width}" stroke-linejoin="round"/>')

    def rect(self, box, colour=RULE, width=1, fill=None):
        self.draw.rectangle(box, outline=colour, width=max(1, round(width)), fill=fill)
        x0, y0, x1, y1 = box
        self.svg.append(f'<rect x="{x0:.2f}" y="{y0:.2f}" width="{x1-x0:.2f}" height="{y1-y0:.2f}" fill="{rgb(fill) if fill else "none"}" stroke="{rgb(colour)}" stroke-width="{width}"/>')

    def ellipse(self, box, colour=INK, width=1, fill=None):
        self.draw.ellipse(box, outline=colour, width=max(1, round(width)), fill=fill)
        x0, y0, x1, y1 = box
        self.svg.append(f'<ellipse cx="{(x0+x1)/2:.2f}" cy="{(y0+y1)/2:.2f}" rx="{(x1-x0)/2:.2f}" ry="{(y1-y0)/2:.2f}" fill="{rgb(fill) if fill else "none"}" stroke="{rgb(colour)}" stroke-width="{width}"/>')

    def text(self, text, x, y, size=22, style='mono', colour=INK):
        font = self.font(size, style)
        self.draw.text((x, y), text, fill=colour, font=font, anchor='lt')
        family = 'DejaVu Sans Mono' if style == 'note' else 'Liberation Mono'
        weight = 'bold' if style == 'bold' else 'normal'
        italic = 'italic' if style == 'note' else 'normal'
        self.svg.append(f'<text x="{x:.2f}" y="{y:.2f}" font-family="{family}" font-size="{size}" font-weight="{weight}" font-style="{italic}" dominant-baseline="text-before-edge" fill="{rgb(colour)}">{html.escape(text)}</text>')
        self.elements.append(text)
        return font.getbbox(text, anchor='lt')

    def save_svg(self, path, metadata):
        xml = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}">\n'
               '<title>SARO array working layout, 1986</title>\n'
               '<metadata>' + html.escape(json.dumps(metadata, sort_keys=True)) + '</metadata>\n'
               + '\n'.join(self.svg) + '\n</svg>\n')
        path.write_text(xml)


def intersects(a, b, padding=0):
    return (a[0] < b[2] + padding and a[2] > b[0] - padding
            and a[1] < b[3] + padding and a[3] > b[1] - padding)


def diagram(drawing, dishes):
    # Equal scale on both actual manifest axes; no synthetic arm/grid positions.
    xmin, xmax, zmin, zmax = -260, 240, -440, -20
    scale = min(930 / (xmax - xmin), 700 / (zmax - zmin))
    x0 = 130 + (930 - (xmax - xmin) * scale) / 2
    y0 = 280

    def point(x, z):
        return x0 + (x - xmin) * scale, y0 + (z - zmin) * scale

    frame = (*point(xmin, zmin), *point(xmax, zmax))
    drawing.rect(frame, RULE, 1)
    for x in range(-250, 241, 50):
        a, b = point(x, zmin), point(x, zmax)
        drawing.line([a, b], RULE if x == 0 else GRID, 1)
        drawing.text(str(x), b[0] - 19, b[1] + 14, 17)
    for z in range(-400, -19, 50):
        a, b = point(xmin, z), point(xmax, z)
        drawing.line([a, b], GRID, 1)
        drawing.text(str(z), a[0] - 66, a[1] - 8, 17)
    drawing.text('LOCAL Z (m)', 116, 245, 18)
    drawing.text('LOCAL X (m)', 517, 1031, 19)
    positions = [(dish, *point(dish['x'], dish['z'])) for dish in dishes]
    marker_boxes = [(x - 9, y - 9, x + 9, y + 9) for _, x, y in positions]
    labels, result = [], []
    for dish, x, y in positions:
        drawing.ellipse((x - 7, y - 7, x + 7, y + 7), INK, 2)
        drawing.line([(x - 3, y), (x + 3, y)], INK, 1)
        drawing.line([(x, y - 3), (x, y + 3)], INK, 1)
        label = dish['id']
        bbox = drawing.font(19, 'mono').getbbox(label, anchor='lt')
        width, height = bbox[2] - bbox[0], bbox[3] - bbox[1]
        candidates = []
        for distance in (15, 25, 37):
            candidates.extend(((x + distance, y - height / 2),
                               (x - distance - width, y - height / 2),
                               (x - width / 2, y - distance - height),
                               (x - width / 2, y + distance)))
        chosen = None
        for lx, ly in candidates:
            area = (lx, ly, lx + width, ly + height)
            if area[0] < frame[0] + 3 or area[2] > frame[2] - 3 or area[1] < frame[1] + 3 or area[3] > frame[3] - 3:
                continue
            if not any(intersects(area, other, 4) for other in marker_boxes + labels):
                chosen = (lx, ly, area)
                break
        if chosen is None:
            raise ValueError('No clear label position for ' + label)
        lx, ly, area = chosen
        drawing.text(label, lx, ly, 19)
        labels.append(area)
        result.append({'id': label, 'symbol_px': [x, y], 'label_box_px': list(area)})
    # A metric scale bar and a quiet pencil registration stroke, not new evidence.
    sx, sy, distance = 160, 1086, 100 * scale
    drawing.line([(sx, sy), (sx + distance, sy)], INK, 2)
    for x in (sx, sx + distance / 2, sx + distance):
        drawing.line([(x, sy - 5), (x, sy + 5)], INK, 1)
    drawing.text('0', sx - 3, sy + 12, 16)
    drawing.text('100 m', sx + distance - 28, sy + 12, 16)
    drawing.line([(106, 274), (106, 253), (102, 260), (106, 253), (110, 260)], PENCIL, 1)
    return {'equal_axis_pixels_per_metre': scale, 'domain_m': {'x': [xmin, xmax], 'z': [zmin, zmax]},
            'labels': result}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()
    source = json.loads(args.manifest.read_text())
    dishes = source['dishes']
    if len(dishes) != 27 or len({dish['id'] for dish in dishes}) != 27:
        raise ValueError('The worksheet requires exactly 27 distinct actual dish IDs')
    for dish in dishes:
        if not all(math.isfinite(float(dish[key])) for key in ('x', 'z', 'az')):
            raise ValueError('Non-finite dish position/orientation: ' + dish['id'])
        if not (-260 < dish['x'] < 240 and -440 < dish['z'] < -20):
            raise ValueError('Dish outside reviewed map extent: ' + dish['id'])
    for font in FONTS.values():
        if not font.is_file():
            raise FileNotFoundError(font)
    image_path = HERE / 'textures/research_array_map.png'
    svg_path = HERE / 'research_array_map.svg'
    metadata_path = HERE / 'research_array_map.source.json'
    drawing = Drawing(paper())
    # Fine border, old fold lines and small handling marks all share SVG/PIL geometry.
    drawing.rect((37, 37, 1563, 1115), (170, 165, 144), 1)
    drawing.line([(792, 43), (795, 384), (793, 710), (796, 1108)], (205, 197, 177), 1)
    drawing.line([(43, 572), (445, 575), (963, 573), (1555, 577)], (207, 199, 179), 1)
    drawing.text('SARO / OPERATIONS', 87, 65, 31, 'bold')
    drawing.text('ARRAY WORKING LAYOUT', 86, 113, 47, 'bold')
    drawing.text('1986', 1370, 66, 34)
    drawing.text('WORKING COPY', 1258, 121, 22)
    drawing.line([(87, 184), (1512, 184)], INK, 2)
    drawing.text('LOCAL POSITION GRID / METRES', 89, 211, 22)
    drawing.line([(1117, 209), (1117, 1038)], RULE, 1)
    map_metadata = diagram(drawing, dishes)
    drawing.text('REFERENCE', 1164, 214, 25, 'bold')
    references = (('ANTENNAS', '27'), ('CHANNEL', '1420.405 MHz'),
                  ('INTERVAL', '47 seconds'), ('REF. AZIMUTH', '026 deg'))
    for index, (label, value) in enumerate(references):
        y = 283 + index * 124
        drawing.text(label, 1164, y, 20)
        drawing.text(value, 1164, y + 34, 29 if index != 1 else 25, 'bold')
        drawing.line([(1164, y + 83), (1507, y + 83)], RULE, 1)
    drawing.text('DALE', 1170, 830, 24, 'note', PENCIL)
    drawing.text('layout copy', 1170, 871, 20, 'note', PENCIL)
    drawing.text('positions in metres', 1164, 940, 17)
    drawing.text('labels retained', 1164, 973, 17)
    # Two discreet pencil gestures concern existing reference values only.
    drawing.line([(1167, 473), (1267, 475), (1379, 473), (1481, 475)], PENCIL, 1)
    drawing.line([(1169, 1063), (1202, 1059), (1290, 1061), (1371, 1058)], PENCIL, 1)
    drawing.text('OPERATIONS / LOCAL ARRAY', 465, 1082, 20)
    drawing.text('PENCIL WORKING SHEET', 1092, 1091, 17)
    image_path.parent.mkdir(parents=True, exist_ok=True)
    drawing.image.save(image_path, format='PNG')
    source_hash = sha(args.manifest)
    dishes_hash = hashlib.sha256(json.dumps(dishes, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    drawing.save_svg(svg_path, {'source_manifest_sha256': source_hash, 'dishes_sha256': dishes_hash,
                                'dish_count': 27, 'units': 'metres', 'axes': 'manifest x,z'})
    metadata = {'status': 'GENERATED_2D_SOURCE', 'native_import': 'UNVERIFIED',
                'runtime_appearance': 'UNVERIFIED', 'source_manifest': 'unreal/Signal47/Content/WebData/manifest.json',
                'source_manifest_sha256': source_hash, 'dishes_sha256': dishes_hash,
                'dishes': dishes, 'image_size_px': [WIDTH, HEIGHT], 'diagram': map_metadata,
                'recipe_sha256': sha(Path(__file__)),
                'font_sha256': {key: sha(path) for key, path in FONTS.items()},
                'svg_sha256': sha(svg_path), 'png_sha256': sha(image_path),
                'content_scope': 'Actual dish positions and IDs, documented 1986/1420.405 MHz/47 seconds/27/az026 reference values; no new case findings.',
                'rendering': 'Shared vector primitives in SVG/Pillow; only common paper grain is raster-embedded in editable SVG.'}
    metadata_path.write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps({'status': metadata['status'], 'dishes': len(dishes), 'png': str(image_path),
                      'svg': str(svg_path), 'snapshot': str(metadata_path)}))


if __name__ == '__main__':
    main()
