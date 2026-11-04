"""EASTWING-01 extras: original printed surfaces (posters, calendar, clock face) for build_extras.py.
Plain Python + Pillow, deterministic. Output: Extras/textures/*_BaseColor.png (sRGB).
"""
from pathlib import Path
import calendar, math, random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = Path(__file__).resolve().parent
OUT = HERE / 'textures'
OUT.mkdir(exist_ok=True)
FONTS = Path('/usr/share/fonts/truetype')


def font(size, bold=True, cond=False):
    names = (['ibm-plex/IBMPlexSansCondensed-Bold.ttf', 'ibm-plex/IBMPlexSansCondensed-SemiBold.ttf'] if cond else []) + \
            (['dejavu/DejaVuSans-Bold.ttf'] if bold else ['dejavu/DejaVuSans.ttf'])
    for n in names:
        p = FONTS / n
        if p.is_file():
            return ImageFont.truetype(str(p), size)
    found = sorted(FONTS.rglob('*Bold*.ttf' if bold else '*.ttf'))
    return ImageFont.truetype(str(found[0]), size)


def centre(d, y, body, f, fill, w):
    for line in body.split('\n'):
        b = d.textbbox((0, 0), line, font=f)
        d.text(((w - (b[2] - b[0])) / 2, y), line, font=f, fill=fill)
        y += (b[3] - b[1]) * 1.32
    return y


def age(im, seed, yellow=.10, crease=True):
    """Sun yellowing, edge dirt, fine paper grain and an old fold line; never pure white."""
    rnd = random.Random(seed)
    w, h = im.size
    tint = Image.new('RGB', im.size, (226, 205, 150))
    im = Image.blend(im, tint, yellow)
    edge = Image.new('L', im.size, 0)
    d = ImageDraw.Draw(edge)
    for i in range(60):
        d.rectangle((i, i, w - 1 - i, h - 1 - i), outline=int(120 * (1 - i / 60) ** 2))
    dirt = Image.new('RGB', im.size, (92, 74, 44))
    im = Image.composite(dirt, im, edge.filter(ImageFilter.GaussianBlur(18)))
    grain = Image.effect_noise(im.size, 22).convert('L')
    im = Image.composite(Image.new('RGB', im.size, (60, 50, 34)), im, grain.point(lambda v: max(0, v - 150) // 3))
    if crease:
        d = ImageDraw.Draw(im)
        y = int(h * rnd.uniform(.44, .56))
        d.line((0, y, w, y + rnd.randint(-6, 6)), fill=(178, 166, 132), width=3)
        d.line((0, y + 3, w, y + 3), fill=(236, 228, 204), width=1)
    for _ in range(14):
        x, y, r = rnd.randint(0, w), rnd.randint(0, h), rnd.randint(4, 26)
        spot = Image.new('L', im.size, 0)
        ImageDraw.Draw(spot).ellipse((x - r, y - r, x + r, y + r), fill=rnd.randint(10, 34))
        im = Image.composite(Image.new('RGB', im.size, (120, 98, 60)), im, spot.filter(ImageFilter.GaussianBlur(r * .7)))
    return im


def rf_poster():
    w, h = 1024, 1448
    im = Image.new('RGB', (w, h), (238, 232, 214)); d = ImageDraw.Draw(im)
    d.rectangle((0, 0, w, 330), fill=(232, 182, 20))
    for x in range(-h, w, 90):
        d.polygon([(x, 0), (x + 45, 0), (x + 45 + 80, 80), (x + 80, 80)], fill=(22, 20, 16))
    d.rectangle((0, 80, w, 330), fill=(232, 182, 20))
    centre(d, 120, 'CAUTION', font(150, cond=True), (20, 18, 14), w)
    # radio-frequency trefoil-free symbol: antenna mast with radiating arcs
    cx, cy = w // 2, 700
    tip = cy - 150
    d.polygon([(cx - 70, cy + 120), (cx + 70, cy + 120), (cx + 12, tip), (cx - 12, tip)], fill=(20, 18, 14))
    d.ellipse((cx - 26, tip - 26, cx + 26, tip + 26), fill=(20, 18, 14))
    for r in (90, 150, 210):
        box = (cx - r, tip - r, cx + r, tip + r)
        d.arc(box, -38, 38, fill=(20, 18, 14), width=20)
        d.arc(box, 142, 218, fill=(20, 18, 14), width=20)
    y = centre(d, 850, 'RADIO FREQUENCY\nRADIATION HAZARD', font(84, cond=True), (20, 18, 14), w)
    y = centre(d, y + 30, 'DO NOT ENTER THE ANTENNA BEAM\nWHILE THE TEST TRANSMITTER IS ENERGISED', font(40, cond=True), (40, 36, 30), w)
    d.rectangle((70, h - 230, w - 70, h - 222), fill=(20, 18, 14))
    centre(d, h - 200, 'POWER DENSITY MAY EXCEED ANSI C95.1-1982\nCHECK THE TX LOG AT CALIBRATION / REPAIR', font(32, bold=False), (40, 36, 30), w)
    centre(d, h - 82, 'SARO SAFETY OFFICE  ·  FORM S-14', font(26, bold=False), (70, 64, 52), w)
    return age(im, 11)


def glasses_poster():
    w, h = 1024, 1448
    im = Image.new('RGB', (w, h), (236, 234, 222)); d = ImageDraw.Draw(im)
    d.rectangle((0, 0, w, 300), fill=(26, 98, 62))
    centre(d, 90, 'NOTICE', font(140, cond=True), (240, 238, 226), w)
    # goggles pictogram
    cx, cy = w // 2, 620
    d.rounded_rectangle((cx - 330, cy - 120, cx + 330, cy + 120), 110, fill=(26, 98, 62))
    for s in (-1, 1):
        d.ellipse((cx + s * 165 - 125, cy - 90, cx + s * 165 + 125, cy + 90), fill=(196, 222, 214))
        d.ellipse((cx + s * 165 - 70, cy - 60, cx + s * 165 - 20, cy - 25), fill=(240, 246, 242))
    d.rectangle((cx - 40, cy - 30, cx + 40, cy + 20), fill=(26, 98, 62))
    d.rectangle((cx - 420, cy - 26, cx - 330, cy + 26), fill=(26, 98, 62)); d.rectangle((cx + 330, cy - 26, cx + 420, cy + 26), fill=(26, 98, 62))
    y = centre(d, 830, 'EYE PROTECTION\nMUST BE WORN', font(96, cond=True), (24, 30, 26), w)
    centre(d, y + 40, 'WHEN SOLDERING, GRINDING\nOR HANDLING CHEMICALS', font(46, cond=True), (52, 58, 52), w)
    centre(d, h - 100, 'SARO SAFETY OFFICE  ·  FORM S-09', font(26, bold=False), (70, 74, 66), w)
    return age(im, 23, .13)


def calendar_1986():
    w, h = 1024, 1536
    im = Image.new('RGB', (w, h), (240, 236, 224)); d = ImageDraw.Draw(im)
    # stylised photo: a dish at dusk
    top = Image.new('RGB', (w, 720))
    td = ImageDraw.Draw(top)
    for y in range(720):
        t = y / 720
        td.line((0, y, w, y), fill=(int(40 + 190 * t), int(52 + 110 * t), int(110 + 20 * t)))
    td.polygon([(0, 600), (220, 520), (420, 580), (640, 500), (1024, 590), (1024, 720), (0, 720)], fill=(92, 58, 40))
    td.rectangle((0, 650, w, 720), fill=(150, 104, 66))
    td.polygon([(470, 650), (550, 650), (520, 430), (500, 430)], fill=(40, 36, 34))
    td.chord((330, 210, 690, 520), 160, 340, fill=(214, 210, 198))
    td.line((510, 365, 590, 250), fill=(60, 56, 52), width=8)
    for i in range(70):
        r = random.Random(i)
        td.point((r.randint(0, w), r.randint(0, 260)), fill=(250, 248, 236))
    im.paste(top.filter(ImageFilter.GaussianBlur(1.2)), (0, 0))
    d.rectangle((0, 720, w, 812), fill=(30, 40, 34))
    centre(d, 735, 'NOVEMBER 1986', font(64, cond=True), (236, 230, 210), w)
    days = ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT']
    cw = (w - 80) / 7; y0 = 840
    for i, n in enumerate(days):
        d.text((40 + i * cw + 20, y0), n, font=font(30, cond=True), fill=(120, 40, 30) if i == 0 else (40, 40, 36))
    cal = calendar.Calendar(firstweekday=6).monthdayscalendar(1986, 11)
    ch = (h - 120 - (y0 + 50)) / len(cal)
    for r, week in enumerate(cal):
        for c, day in enumerate(week):
            x, y = 40 + c * cw, y0 + 50 + r * ch
            d.rectangle((x, y, x + cw, y + ch), outline=(150, 146, 134), width=2)
            if day:
                d.text((x + 10, y + 6), str(day), font=font(40, cond=True), fill=(120, 40, 30) if c == 0 else (34, 34, 30))
    # handwritten shift marks
    pen = (30, 52, 120)
    for day, note in [(3, 'S-03 svc'), (4, 'night'), (12, 'fuel'), (19, 'Caliche')]:
        for r, week in enumerate(cal):
            if day in week:
                c = week.index(day); x, y = 40 + c * cw, y0 + 50 + r * ch
                d.text((x + 12, y + ch * .52), note, font=font(26, bold=False), fill=pen)
    cx, cy = 40 + 2 * cw + cw / 2, y0 + 50 + ch * 1.45
    d.ellipse((cx - cw * .46, cy - ch * .5, cx + cw * .46, cy + ch * .5), outline=(160, 30, 26), width=5)
    centre(d, h - 90, 'CALICHE FEED & SUPPLY  ·  ROUTE 47', font(30, bold=False), (60, 56, 48), w)
    return age(im, 37, .12, crease=False)


def clock_face():
    s = 1024
    im = Image.new('RGB', (s, s), (232, 228, 212)); d = ImageDraw.Draw(im)
    c = s / 2
    for i in range(60):
        a = math.radians(i * 6); long = i % 5 == 0
        r0, r1 = c * (.80 if long else .86), c * .92
        d.line((c + r0 * math.sin(a), c - r0 * math.cos(a), c + r1 * math.sin(a), c - r1 * math.cos(a)), fill=(24, 24, 22), width=16 if long else 5)
    f = font(92)
    for n in range(1, 13):
        a = math.radians(n * 30); r = c * .64
        b = d.textbbox((0, 0), str(n), font=f)
        d.text((c + r * math.sin(a) - (b[2] - b[0]) / 2, c - r * math.cos(a) - (b[3] - b[1]) / 2 - b[1]), str(n), font=f, fill=(24, 24, 22))
    centre(d, c + 140, 'SARO\nSTANDARD TIME', font(34, cond=True), (130, 30, 26), s)
    return age(im, 41, .08, crease=False)


for name, make in [('EW_Print_PosterRF', rf_poster), ('EW_Print_PosterGlasses', glasses_poster),
                   ('EW_Print_Calendar1986', calendar_1986), ('EW_Print_ClockFace', clock_face)]:
    random.seed(name)
    im = make()
    # Store power-of-two (mipmapped in Unreal); the mesh UV restores the printed aspect.
    pot = (1024, 2048) if im.height > im.width else (1024, 1024)
    im.resize(pot, Image.LANCZOS).save(OUT / (name + '_BaseColor.png'), optimize=True)
    print('print', name)
