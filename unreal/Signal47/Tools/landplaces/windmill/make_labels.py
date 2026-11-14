"""LAND-PLACES-WINDMILL-20261114: the windmill's two printed pieces, as pictures of their own (the important-objects rule: labels are
their own texture). Invented names only.
  python3 make_labels.py <out dir>         (Pillow; the project's own fonts)
T_LP_MillVaneLabel.png  1024 x 512 RGBA   BACA LEASE NO. 2 stencilled on the tail vane, most of it weathered off (alpha)
T_LP_MillPage.png       512 x 512 RGB     Reyes's page of April 8 in its sandwich bag (S47PredecessorText book_windmill)
"""
import random, sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops

OUT = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
OUT.mkdir(parents=True, exist_ok=True)
FONTS = Path(__file__).resolve().parents[3] / 'Content' / 'Fonts'
PRINT, HAND = FONTS / 'Oswald-Medium.ttf', FONTS / 'ReenieBeanie-Regular.ttf'


def cloud(size, seed, cells, blur):
    """Soft blotches, 0..255."""
    rng = random.Random(seed)
    small = Image.new('L', (cells, max(1, cells * size[1] // size[0])))
    small.putdata([rng.randrange(256) for _ in range(small.size[0] * small.size[1])])
    return small.resize(size, Image.BICUBIC).filter(ImageFilter.GaussianBlur(blur))


def vane_label():
    W, H = 1024, 512
    mask = Image.new('L', (W, H), 0)
    d = ImageDraw.Draw(mask)
    big, small = ImageFont.truetype(str(PRINT), 190), ImageFont.truetype(str(PRINT), 150)
    d.text((W // 2, 150), 'BACA LEASE', font=big, fill=255, anchor='mm')
    d.text((W // 2, 370), 'NO. 2', font=small, fill=255, anchor='mm')
    # stencil bridges: thin gaps down through the letters
    for x in range(40, W, 61):
        d.rectangle((x, 0, x + 5, H), fill=0)
    mask = mask.filter(ImageFilter.GaussianBlur(1.2))
    # what thirty years of sand left: blotches of it, fewer toward the windward edge, speckled
    keep = ImageChops.multiply(cloud((W, H), 3, 14, 10), cloud((W, H), 8, 60, 3))
    keep = keep.point(lambda v: 0 if v < 52 else min(255, int((v - 52) * 3.2)))
    grit = cloud((W, H), 21, 420, 0.6).point(lambda v: 255 if v > 96 else 60)
    fade = Image.linear_gradient('L').rotate(90, expand=True).resize((W, H)).point(lambda v: 110 + v * 145 // 255)
    a = ImageChops.multiply(ImageChops.multiply(ImageChops.multiply(mask, keep), grit), fade).point(lambda v: v * 150 // 255)
    out = Image.merge('RGBA', (Image.new('L', (W, H), 22), Image.new('L', (W, H), 22), Image.new('L', (W, H), 24), a))
    out.save(OUT / 'T_LP_MillVaneLabel.png')
    return a


def page():
    S = 512
    rng = random.Random(408)
    bag = Image.new('RGB', (S, S), (150, 152, 150))
    # the page, folded once, a little askew in the bag
    paper = Image.new('RGB', (360, 400), (214, 205, 176))
    d = ImageDraw.Draw(paper)
    for y in range(58, 400, 27):
        d.line((0, y, 360, y), fill=(150, 170, 190), width=1)
    d.line((44, 0, 44, 400), fill=(196, 130, 126), width=1)
    hand = ImageFont.truetype(str(HAND), 31)
    lines = ['APRIL 8', '', 'The bearing along the vane', 'from the tower foot runs to', 'the cairn on the rise. I walked', 'it as far as the fence to be', 'sure.',
             'Somebody climbs this tower', 'to turn the vane. There are', 'no ladder marks.', 'The rest of the book goes up', 'to the cairn, under the top', 'stone.']
    y = 28
    for t in lines:
        d.text((54 + rng.randrange(-3, 4), y + rng.randrange(-2, 3)), t, font=hand, fill=(66, 64, 70))
        y += 27
    d.line((0, 200, 360, 203), fill=(176, 166, 140), width=2)                     # the fold
    for _ in range(6):                                                              # damp at a corner, dried
        x, y0 = rng.randrange(360), rng.randrange(400)
        ImageDraw.Draw(paper).ellipse((x - 30, y0 - 20, x + 30, y0 + 20), outline=(190, 178, 146))
    paper = paper.rotate(-4, expand=True, fillcolor=(150, 152, 150), resample=Image.BICUBIC)
    bag.paste(paper, (66, 60))
    # the plastic over it: a haze, long soft highlights, wrinkles, the zip, dust in the corners
    haze = Image.new('RGB', (S, S), (196, 200, 200))
    bag = Image.blend(bag, haze, 0.22)
    shine = cloud((S, S), 5, 9, 14).point(lambda v: 0 if v < 150 else (v - 150) * 2)
    bag = Image.composite(Image.new('RGB', (S, S), (236, 240, 240)), bag, shine.point(lambda v: v * 120 // 255))
    d = ImageDraw.Draw(bag)
    for _ in range(9):
        x0, y0 = rng.randrange(S), rng.randrange(S)
        x1, y1 = x0 + rng.randrange(-180, 180), y0 + rng.randrange(-60, 60)
        d.line((x0, y0, x1, y1), fill=(224, 228, 228), width=1)
    d.rectangle((0, 18, S, 30), fill=(120, 150, 196))
    d.rectangle((0, 22, S, 25), fill=(196, 206, 226))
    d.rectangle((0, 0, S - 1, S - 1), outline=(120, 122, 120), width=3)
    dust = cloud((S, S), 13, 30, 6).point(lambda v: 0 if v < 140 else (v - 140) * 2)
    bag = Image.composite(Image.new('RGB', (S, S), (120, 100, 76)), bag, dust.point(lambda v: v * 90 // 255))
    bag.save(OUT / 'T_LP_MillPage.png')


vane_label()
page()
print('labels in', OUT)
