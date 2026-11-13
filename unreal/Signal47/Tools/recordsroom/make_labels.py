#!/usr/bin/env python3
"""RECORDS-ROOM-LIFT-20261113: the label atlas of the records room's cartons, binders and drawers (T_Rec_Labels, 2048 px).

  python3 make_labels.py <out dir>           (Pillow; the project's own fonts, Content/Fonts: Special Elite, Reenie Beanie)
Cells (S47RecordsRoom.cpp places one card a carton by its cell):
  0..23   carton cards, 512 x 256 (4 x 6): typed on the station's typewriter 1977-86, ordinary station business; the names are
          the room's own (S47RecordsDressing's cards, the ArchiveProps kit's shelf labels), so nothing new is claimed by them
  24..31  drawer slips, 512 x 128 (two rows of four)
  32..39  binder spine slips, 512 x 128 (two rows of four); the game turns them to run up the spine
Each card is aged card stock with its own tone, a typed or printed frame, ink that varies, a stamp or a pencilled note on some.
Invented names only; no real brand, person or document.
"""
import math, random, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

OUT = Path(sys.argv[1]).resolve()
FONTS = Path(__file__).resolve().parents[2] / 'Content' / 'Fonts'
TYPE, HAND, PRINT = FONTS / 'SpecialElite-Regular.ttf', FONTS / 'ReenieBeanie-Regular.ttf', FONTS / 'Oswald-Medium.ttf'
S = 2048
rng = random.Random(1986)

CARDS = [
    ('OBS LOGS', '1979 - 80', 'typed'), ('RECEIVER CAL', '1984', 'frame'), ('PURCHASE ORDERS', 'FY 85', 'typed'), ('DISH S-14', 'SERVICE', 'red'),
    ('STRIP CHARTS', 'JAN - JUN 1983', 'frame'), ('TIME CARDS', '1982 - 84', 'typed'), ('GENERATOR LOGS', '1981', 'red'), ('INTERFERENCE RPTS', '1985', 'typed'),
    ('SITE PLANS', 'SUPERSEDED', 'frame'), ('MAINT. TICKETS', '1985', 'typed'), ('CORRESPONDENCE', '1980 - 81', 'red'), ('FCC FILINGS', '1977 - 84', 'frame'),
    ('CRYO SERVICE', '1983', 'typed'), ('SURVEY PLATES', 'AZ 000 - 090', 'frame'), ('TAPE CATALOGUE', '1978 - 81', 'typed'), ('FIELD SURVEYS', 'FIXED POINTS', 'red'),
    ('SERVICE COPIES', '1978 - 1985', 'frame'), ('RX LOGS', '1979 - 1980', 'typed'), ('WEATHER LOG', '1984 - 85', 'typed'), ('DRIVE MOTORS', 'VOL 1 - 3', 'frame'),
    ('DO NOT DISCARD', None, 'hand'), ('duplicates', 'do not file', 'hand'), ('RETURNS', 'TO BE REFILED', 'red'), ('INVENTORY', '1985', 'typed'),
]
DRAWERS = ['A - D', 'E - K', 'L - R', 'S - Z', 'OBS LOGS 77-82', 'PERSONNEL', 'PURCHASING', 'SITE / UTILITIES']
SPINES = ['PURCHASE ORDERS FY85', 'RECEIVER CAL 1985', 'DISH S-09 SERVICE', 'OBS LOGS 1980', 'SAFETY MANUAL REV C', 'LO CHAIN SCHEMATICS', 'BUDGET FY86', 'SITE VISITS 1984']
NOTES = ['refiled 4/86', 'see box 12', 'chk. w/ Dale', 'incomplete', '2 of 3', 'copy only']


def stock(w, h, tone):
    """Aged card: its tone, a slow mottle, fibres, darker at the edges where fingers and dust have been."""
    base = Image.new('RGB', (w, h), tone)
    n = Image.effect_noise((w // 6, h // 6), 30).resize((w, h), Image.BICUBIC).filter(ImageFilter.GaussianBlur(5))
    f = Image.effect_noise((w, h // 3), 22).resize((w, h), Image.BICUBIC)
    m = ImageChops.multiply(base, Image.merge('RGB', [n.point(lambda v: 240 + (v - 128) * 0.32)] * 3))
    m = ImageChops.multiply(m, Image.merge('RGB', [f.point(lambda v: 248 + (v - 128) * 0.16)] * 3))
    edge = Image.new('L', (w, h), 0)
    d = ImageDraw.Draw(edge)
    d.rectangle([10, 10, w - 10, h - 10], fill=255)
    edge = edge.filter(ImageFilter.GaussianBlur(9)).point(lambda v: 214 + v * 41 // 255)
    m = ImageChops.multiply(m, Image.merge('RGB', [edge] * 3))
    # a foxed spot or a ring now and then
    if rng.random() < 0.5:
        sp = Image.new('L', (w, h), 255)
        d = ImageDraw.Draw(sp)
        for _ in range(rng.randint(1, 4)):
            x, y, r = rng.randint(20, w - 20), rng.randint(20, h - 20), rng.randint(6, 22)
            d.ellipse([x - r, y - r, x + r, y + r], fill=rng.randint(205, 235))
        m = ImageChops.multiply(m, Image.merge('RGB', [sp.filter(ImageFilter.GaussianBlur(7))] * 3))
    return m


def typed(im, text, font, xy, ink=(34, 30, 28), anchor='mm', jitter=1.2):
    """Typewriter text: each letter its own pressure and a hair off its line."""
    layer = Image.new('L', im.size, 0)
    d = ImageDraw.Draw(layer)
    w = d.textlength(text, font=font)
    x = xy[0] - (w / 2 if anchor == 'mm' else 0)
    for ch in text:
        cw = d.textlength(ch, font=font)
        d.text((x + rng.uniform(-jitter, jitter) * 0.5, xy[1] + rng.uniform(-jitter, jitter)), ch, font=font, fill=rng.randint(150, 255), anchor='ls')
        x += cw
    layer = layer.filter(ImageFilter.GaussianBlur(0.55))
    im.paste(Image.new('RGB', im.size, ink), (0, 0), layer)


def fit(text, path, size, width):
    f = ImageFont.truetype(str(path), size)
    while size > 14 and ImageDraw.Draw(Image.new('L', (4, 4))).textlength(text, font=f) > width:
        size -= 2
        f = ImageFont.truetype(str(path), size)
    return f


def card(a, b, kind, w=512, h=256):
    tones = [(226, 216, 190), (216, 205, 176), (232, 225, 204), (206, 196, 168), (222, 214, 196)]
    im = stock(w, h, rng.choice(tones) if kind != 'red' else (232, 226, 208))
    d = ImageDraw.Draw(im)
    if kind == 'frame':
        d.rectangle([14, 14, w - 15, h - 15], outline=(96, 88, 72), width=3)
    elif kind == 'red':                       # a printed gummed label: a red border, a ruled line for the number
        d.rectangle([12, 12, w - 13, h - 13], outline=(150, 46, 36), width=7)
        d.rectangle([24, 24, w - 25, h - 25], outline=(150, 46, 36), width=2)
        d.line([40, h - 46, w - 40, h - 46], fill=(150, 46, 36), width=2)
        d.text((44, h - 44), 'FILE No.', font=ImageFont.truetype(str(PRINT), 20), fill=(150, 46, 36))
    if kind == 'hand':
        f = fit(a, HAND, 104 if b is None else 84, w - 50)
        t = Image.new('L', (w, h), 0)
        td = ImageDraw.Draw(t)
        td.text((w / 2, h * (0.52 if b is None else 0.36)), a, font=f, fill=235, anchor='mm')
        if b:
            td.text((w / 2, h * 0.72), b, font=fit(b, HAND, 70, w - 60), fill=225, anchor='mm')
        t = t.rotate(rng.uniform(-3.5, 3.5), resample=Image.BICUBIC).filter(ImageFilter.GaussianBlur(0.6))
        im.paste(Image.new('RGB', (w, h), (30, 34, 58)), (0, 0), t)
    else:
        typed(im, a, fit(a, TYPE, 58, w - 64), (w / 2, h * (0.44 if kind != 'red' else 0.40)))
        if b:
            typed(im, b, fit(b, TYPE, 48, w - 84), (w / 2, h * (0.74 if kind != 'red' else 0.66)))
        r = rng.random()
        if r < 0.28:                         # a pencilled note in a corner
            t = Image.new('L', (w, h), 0)
            ImageDraw.Draw(t).text((w - 150, 44), rng.choice(NOTES), font=ImageFont.truetype(str(HAND), 44), fill=200, anchor='mm')
            t = t.rotate(rng.uniform(-8, 8), resample=Image.BICUBIC)
            im.paste(Image.new('RGB', (w, h), (70, 72, 80)), (0, 0), t)
        elif r < 0.45 and kind != 'red':     # a date stamp, tired ink
            t = Image.new('L', (w, h), 0)
            td = ImageDraw.Draw(t)
            txt = rng.choice(['RECEIVED  %d %s 198%d' % (rng.randint(2, 27), rng.choice(['JAN', 'MAR', 'JUN', 'SEP', 'NOV']), rng.randint(1, 5)), 'SARO RECORDS', 'CONTROL COPY'])
            f = ImageFont.truetype(str(PRINT), 26)
            tw = td.textlength(txt, font=f)
            td.rectangle([w - tw - 62, h - 66, w - 30, h - 26], outline=150, width=3)
            td.text((w - tw / 2 - 46, h - 46), txt, font=f, fill=150, anchor='mm')
            t = t.rotate(rng.uniform(-6, 6), resample=Image.BICUBIC).filter(ImageFilter.GaussianBlur(0.8))
            im.paste(Image.new('RGB', (w, h), (118, 34, 58)), (0, 0), ImageChops.multiply(t, Image.effect_noise((w, h), 90).point(lambda v: 255 if v > 96 else 90)))
    return im


def slip(text, w, h, upright=False):
    im = stock(w, h, rng.choice([(228, 220, 198), (218, 209, 184), (234, 228, 210)]))
    if upright:
        t = Image.new('RGB', (h, w), (0, 0, 0))
        t.paste(im.rotate(90, expand=True))
        d = ImageDraw.Draw(t)
        d.rectangle([8, 8, h - 9, w - 9], outline=(110, 100, 84), width=2)
        typed(t, text, fit(text, TYPE, 46, h - 46), (h / 2, w * 0.60))
        return t.rotate(-90, expand=True)
    d = ImageDraw.Draw(im)
    d.rectangle([8, 8, w - 9, h - 9], outline=(110, 100, 84), width=2)
    typed(im, text, fit(text, TYPE, 54, w - 50), (w / 2, h * 0.66))
    return im


atlas = Image.new('RGB', (S, S), (224, 216, 194))
cells = {}
for i, (a, b, kind) in enumerate(CARDS):
    x, y = (i % 4) * 512, (i // 4) * 256
    atlas.paste(card(a, b, kind), (x, y))
    cells[i] = (x, y, 512, 256, a if not b else a + ' / ' + b)
for i, t in enumerate(DRAWERS):
    x, y = (i % 4) * 512, 1536 + (i // 4) * 128
    atlas.paste(slip(t, 512, 128), (x, y))
    cells[24 + i] = (x, y, 512, 128, t)
for i, t in enumerate(SPINES):
    x, y = (i % 4) * 512, 1792 + (i // 4) * 128
    atlas.paste(slip(t, 512, 128), (x, y))
    cells[32 + i] = (x, y, 512, 128, t)
OUT.mkdir(parents=True, exist_ok=True)
atlas.save(OUT / 'T_Rec_Labels.png')
print('wrote', OUT / 'T_Rec_Labels.png', atlas.size)
for k in sorted(cells):
    print(k, cells[k])
