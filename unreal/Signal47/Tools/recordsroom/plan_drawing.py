#!/usr/bin/env python3
"""RECORDS-ROOM-LIFT-20261113: the records room from above, measured from the code and the web export (SARO metres: x east, z south).
Left: as it stands (d69). Right: the plan for the lift. python3 plan_drawing.py <out.png>"""
import sys
from PIL import Image, ImageDraw, ImageFont

OUT = sys.argv[1]
S = 190                      # px per metre
MX, MZ = 150, 190            # margins
VX0, VX1, VZ0, VZ1 = -6.6, -0.2, 4.6, 11.0
W = int((VX1 - VX0) * S) + 2 * MX
Hh = int((VZ1 - VZ0) * S) + MZ + 330
F = '/home/tom/SIGNAL-47/unreal/Signal47/Content/Fonts/JetBrainsMono-Regular.ttf'
f10, f12, f14, f18, f26 = (ImageFont.truetype(F, n) for n in (15, 18, 21, 26, 36))
INK, WALL, DIM, RED, BLUE, GREEN, ORANGE, GREY = (25, 25, 28), (60, 62, 66), (120, 30, 30), (190, 40, 30), (30, 80, 170), (20, 120, 60), (200, 110, 10), (150, 150, 150)


def panel(title, after):
    im = Image.new('RGB', (W, Hh), (247, 245, 238))
    d = ImageDraw.Draw(im, 'RGBA')
    P = lambda x, z: (MX + (x - VX0) * S, MZ + (z - VZ0) * S)

    def rect(x0, x1, z0, z1, fill=None, outline=INK, width=2):
        a, b = P(x0, z0), P(x1, z1)
        d.rectangle([a[0], a[1], b[0], b[1]], fill=fill, outline=outline, width=width)

    def text(x, z, s, font=f12, fill=INK, anchor='mm'):
        d.text(P(x, z), s, font=font, fill=fill, anchor=anchor)

    def dim(x0, z0, x1, z1, label=None, off=0.0, col=DIM):
        a, b = P(x0, z0), P(x1, z1)
        d.line([a, b], fill=col, width=2)
        for p in (a, b):
            d.ellipse([p[0] - 4, p[1] - 4, p[0] + 4, p[1] + 4], fill=col)
        L = ((x1 - x0) ** 2 + (z1 - z0) ** 2) ** .5
        mx, mz = (x0 + x1) / 2, (z0 + z1) / 2
        vert = abs(x1 - x0) < abs(z1 - z0)
        s = label or ('%.2f' % L)
        px, pz = P(mx, mz)
        tw = d.textlength(s, font=f12)
        d.rectangle([px - tw / 2 - 4 + (off * S if vert else 0), pz - 11 + (0 if vert else off * S), px + tw / 2 + 4 + (off * S if vert else 0), pz + 11 + (0 if vert else off * S)], fill=(247, 245, 238, 235))
        d.text((px + (off * S if vert else 0), pz + (0 if vert else off * S)), s, font=f12, fill=col, anchor='mm')

    d.text((MX, 40), title, font=f26, fill=INK)
    d.text((MX, 92), 'SARO-meter (x øst, z sør). Rom 5.00 x 3.70, takhøyde 2.80 (ny himling 2.70). Gul: gangsone (spillerens midtpunkt, radius 0.27).', font=f12, fill=INK)
    d.text((MX, 118), 'Stiplet: kolliderrektangler. Blå: interaksjonsproxyer. Grønn: AI-noder (SC04) og faste ståsteder i prøvene.', font=f12, fill=INK)
    # walls (hatched fill): west and south exterior 0.30, north partition 0.20 with the doorway, east partition 0.20
    for r in ((-6.3, -6.0, 4.8, 10.8), (-6.3, -0.8, 10.5, 10.8), (-6.0, -4.4, 6.6, 6.8), (-3.4, -0.8, 6.6, 6.8), (-1.0, -0.8, 6.8, 10.5)):
        rect(*r, fill=(96, 98, 102), outline=WALL)
    rect(-6.3, -6.0, 5.22, 6.18, fill=(247, 245, 238), outline=WALL)          # fire exit opening
    rect(-6.0, -0.2, 4.6, 4.8, fill=(96, 98, 102), outline=WALL)              # control room's south wall
    rect(-2.9, -1.9, 4.6, 4.8, fill=(247, 245, 238), outline=WALL)
    text(-3.2, 5.7, 'SØRKORRIDOR  z 4.80-6.60', f14)
    text(-2.4, 4.68, 'kontrollrom', f10)
    text(-6.45, 5.7, 'BRANNDØR\n(ut, vest)', f10, RED)
    text(-0.74, 8.7, 'DALES\nKONTOR\n(operations)\nx -0.82..1.77\nz 7.30..10.31\nlåst, egen\nnøkkel', f10, anchor='lm')
    text(-6.45, 8.6, 'UTE\n(vest)\nbakke\n-0.62', f10)
    text(-3.5, 10.92, 'UTE (sør): gårdsplass, sokkel 0.6 m over bakken', f10)
    # walk zone
    rect(-5.75, -1.25, 7.05, 10.25, fill=(255, 225, 90, 60), outline=(200, 160, 0), width=1)
    rect(-4.38, -3.42, 5.6, 7.7, fill=(255, 225, 90, 40), outline=(200, 160, 0), width=1)
    # door: leaf shut, swing, open leaf
    hx, hz = P(-3.42, 6.82)
    r = 0.94 * S
    d.arc([hx - r, hz - r, hx + r, hz + r], 0, 180, fill=ORANGE, width=2)
    d.line([P(-3.42, 6.82), P(-4.36, 6.82)], fill=ORANGE, width=5)
    d.line([P(-3.42, 6.86), P(-2.48, 6.86)], fill=(200, 110, 10, 130), width=5)
    text(-3.9, 6.48, 'dør 1.00 x 2.10', f10, ORANGE)
    text(-2.95, 6.98, 'åpent blad', f10, ORANGE)
    dim(-4.4, 6.7, -3.4, 6.7, '1.00')

    def furn(x0, x1, z0, z1, label, col=(205, 200, 185), font=f10, dash=None):
        rect(x0, x1, z0, z1, fill=col + (255,), outline=INK, width=2)
        text((x0 + x1) / 2, (z0 + z1) / 2, label, font)

    def coll(x0, x1, z0, z1, col=RED):
        a, b = P(x0, z0), P(x1, z1)
        for i in range(int(a[0]), int(b[0]), 12):
            d.line([(i, a[1]), (min(i + 6, b[0]), a[1])], fill=col, width=2); d.line([(i, b[1]), (min(i + 6, b[0]), b[1])], fill=col, width=2)
        for i in range(int(a[1]), int(b[1]), 12):
            d.line([(a[0], i), (a[0], min(i + 6, b[1]))], fill=col, width=2); d.line([(b[0], i), (b[0], min(i + 6, b[1]))], fill=col, width=2)

    furn(-5.99, -5.48, 7.10, 10.20, '', (170, 185, 172))
    text(-5.735, 9.55, 'STÅL-\nREOL\n3.10\nx 0.51\nh 2.30', f10)
    furn(-3.30, -1.40, 10.12, 10.48, 'BOKHYLLE 1.90 x 0.36', (190, 160, 130))
    furn(-1.73, -0.985, 7.49, 8.42, 'KARTO-\nTEK\nh 1.08', (170, 185, 172))
    furn(-4.50, -2.80, 8.175, 9.025, '', (200, 150, 110))
    text(-3.65, 8.72, 'ARBEIDSBORD 1.70 x 0.85, h 0.77', f10)
    furn(-2.38, -1.02, 6.82, 7.28, 'KONTORISTPULT 1.36 x 0.46', (200, 200, 170))
    if not after:
        furn(-5.25, -3.67, 9.86, 10.48, 'ARKIVSKAP 3 stk, h 1.32', (170, 185, 172))
        furn(-3.89, -3.41, 9.04, 9.66, 'stol', (120, 125, 140))
        furn(-4.68, -4.24, 9.48, 9.86, 'skuff', (150, 170, 155))
        furn(-3.67, -3.30, 10.04, 10.48, 'kart.', (190, 165, 120))
        furn(-5.48, -4.92, 6.80, 7.06, 'flate kart.', (190, 165, 120))
        for c in ((-3.9, -3.4, 9.05, 9.7), (-4.7, -4.22, 9.48, 9.86), (-5.48, -4.92, 6.80, 7.06), (-4.53, -2.77, 8.15, 9.05)):
            coll(*c)
        d.ellipse([P(-4.83, 6.87)[0], P(-4.83, 6.87)[1], P(-4.67, 7.03)[0], P(-4.67, 7.03)[1]], fill=(170, 40, 30))
        text(-4.75, 7.13, 'slukker', f10, RED)
    else:
        furn(-5.25, -3.67, 9.86, 10.48, 'ARKIVSKAP 3 stk (én skuff 7 cm ut)', (170, 185, 172))
        furn(-1.74, -1.26, 7.02, 7.47, 'stol', (120, 125, 140))
        furn(-3.67, -3.30, 10.04, 10.48, 'kart.', (190, 165, 120))
        furn(-1.80, -1.00, 8.50, 8.95, 'REOL 2  0.80 x 0.45\nh 2.0, ryggplate', (150, 175, 160))
        coll(-4.53, -2.77, 8.15, 9.03)
        coll(-1.82, -1.00, 8.48, 8.97)
        coll(-1.76, -1.24, 7.26, 7.49)
        d.rectangle([P(-5.13, 6.80)[0], P(-5.13, 6.80)[1], P(-4.97, 6.96)[0], P(-4.97, 6.96)[1]], fill=(170, 40, 30))
        text(-5.05, 7.06, 'slukker i brakett', f10, RED)
        # crouch pocket behind the 4th cabinet
        d.ellipse([P(-1.54, 8.98)[0], P(-1.54, 8.98)[1], P(-1.00, 9.52)[0], P(-1.00, 9.52)[1]], outline=(120, 0, 140), width=3)
        text(-1.42, 9.72, 'siktbrudd\nbak reol 2', f10, (120, 0, 140))
        for dx, dz in ((-4.38, 7.0), (-3.42, 7.0)):
            d.line([P(dx, dz), P(-1.27, 9.25)], fill=(120, 0, 140, 150), width=2)
    for c in ((-6, -5.5, 7.1, 10.2), (-5.27, -3.65, 9.84, 10.5), (-3.32, -1.38, 10.1, 10.5), (-1.8, -1.0, 7.5, 8.4), (-2.38, -1.02, 6.80, 7.28), (-3.67, -3.30, 10.04, 10.48)):
        coll(*c)
    if not after:
        pass
    # things on the table
    rect(-4.395, -3.845, 8.51, 8.93, fill=(190, 50, 40), outline=INK, width=1); text(-4.12, 8.46, 'skrivemaskin', f10)
    d.ellipse([P(-3.16, 8.26)[0], P(-3.16, 8.26)[1], P(-2.98, 8.44)[0], P(-2.98, 8.44)[1]], fill=(30, 110, 70)); text(-3.07, 8.52, 'lampe', f10)
    # ceiling fixtures
    for (x0, x1, z0, z1) in (((-5.2, -4.0, 7.87, 8.13), (-3.0, -1.8, 9.27, 9.53)) if not after else ((-5.2, -4.0, 7.7, 8.3), (-2.8, -1.6, 7.7, 8.3))):
        a, b = P(x0, z0), P(x1, z1)
        d.rectangle([a[0], a[1], b[0], b[1]], outline=(60, 140, 220), width=3)
    text(-4.6, 7.62 if after else 7.78, 'lysarmatur (flimmerrør)' if not after else 'lysarmatur 0.6 x 1.2', f10, (60, 140, 220))
    # proxies and gameplay points
    for x0, x1, z0, z1, n in ((-5.8, -5.3, 8.35, 8.85, 'fieldBox\nbrannskuff'), (-2.545, -2.095, 9.975, 10.425, 'binder'), (-1.245, -0.995, 9.1, 9.4, 'recPhone'), (-1.28, -1.0, 10.06, 10.38, 'KSU/\nopptaker')):
        rect(x0, x1, z0, z1, outline=BLUE, width=3)
        text((x0 + x1) / 2 + (0.42 if n == 'recPhone' else 0), (z0 + z1) / 2 + (0 if n in ('recPhone',) else 0), n, f10, BLUE, 'mm' if n != 'recPhone' else 'lm')
    rect(-4.5, -2.9, 7.85, 8.75, outline=(120, 0, 140), width=2)
    text(-3.70, 8.02, 'ff:hide:annex (-3.70, 8.30)', f10, (120, 0, 140))
    d.ellipse([P(-3.74, 8.26)[0], P(-3.74, 8.26)[1], P(-3.66, 8.34)[0], P(-3.66, 8.34)[1]], fill=(120, 0, 140))
    rect(-4.795, -4.705, 6.80, 6.83, outline=BLUE, width=3); text(-4.75, 6.92, 'bryter', f10, BLUE)
    for x, z, n in ((-3.9, 7.5, 'recordsIn'), (-2.3, 7.6, 'recordsEast'), (-2.2, 9.6, 'recordsBinder'), (-5.0, 7.7, 'recordsWest'), (-5.0, 9.2, 'recordsBox'), (-2.3, 9.45, 'stå binder'), (-1.75, 9.3, 'stå tlf')):
        a = P(x, z)
        d.ellipse([a[0] - 6, a[1] - 6, a[0] + 6, a[1] + 6], fill=GREEN)
        d.text((a[0] + 9, a[1] - (16 if n.startswith('stå') else 0)), n, font=f10, fill=GREEN, anchor='lm')
    for a, b in (((-3.9, 7.5), (-2.3, 7.6)), ((-2.3, 7.6), (-2.2, 9.6)), ((-3.9, 7.5), (-5.0, 7.7)), ((-5.0, 7.7), (-5.0, 9.2))):
        d.line([P(*a), P(*b)], fill=(20, 120, 60, 110), width=2)
    # dimensions
    dim(-6.0, 10.66, -1.0, 10.66, '5.00')
    dim(-0.9, 6.8, -0.9, 10.5, '3.70')
    dim(-5.5, 8.45, -4.53, 8.45, '0.97')
    dim(-2.77, 8.9 if not after else 8.72, -1.8, 8.9 if not after else 8.72, '0.97')
    dim(-3.0, 6.8, -3.0, 8.15, '1.35', off=-0.0)
    dim(-4.95, 9.05 if not after else 9.03, -4.95, 9.84, '0.79' if not after else '0.81')
    if not after:
        text(-3.65, 9.76, '0.14', f10, RED)
        text(-4.46, 9.27, '0.43', f10, RED)
    # legend / notes
    y = MZ + (VZ1 - VZ0) * S + 30
    notes = (['MÅLT I DAG (kode + web-eksport 22.10):',
              '- Rundt bordet: vest 0.97, nord 1.35 (0.87 ved pulten), øst 0.97, sør 0.79 mellom bord og skap.',
              '- MEN sør er stengt: stolen står i gangen (0.14 til skapene) og den åpne skuffen gir 0.43 (spilleren trenger 0.54).',
              '- Fra dørkarmen sees rett inn under bordets åpne nordside (1.4-1.9 m): skjulet har ikke siktbrudd mot døra.',
              '- Ingen frittstående reol: ingen plass å komme bak. Eneste dør: nord, til korridoren.']
             if not after else
             ['PLAN FOR LØFTET (alle spillposisjoner beholdt):',
              '- Stolen flyttes til kontoristpultens knehull; den åpne skuffen blir 7 cm «på gløtt» uten egen kollider: sør 0.81 fri.',
              '- De flate kartongene ved døra fjernes; slukkeren henger i brakett ved siden av bryteren.',
              '- Forslag: en kort reol 2 (0.80 x 0.45, h 2.0, ryggplate) sør for kartoteket: siktbrudd fra døra til plassen ved veggtelefonen.',
              '- Ny nedhengt himling 2.70 med to 0.6 x 1.2 armaturer i én rad (z 7.7-8.3): sørdelen og plassen bak reol 2 blir mørkere.',
              '- Sikt dør -> under bordet kan ikke brytes uten å stenge utsynet til den som gjemmer seg eller sperre prøvenes gangveier (måles).'])
    for i, s in enumerate(notes):
        d.text((MX, y + i * 30), s, font=f14 if i == 0 else f12, fill=INK)
    return im


a, b = panel('ARKIVROMMET I DAG (d69)', False), panel('PLAN: RECORDS-ROOM-LIFT-20261113', True)
sheet = Image.new('RGB', (a.width + b.width, a.height), 'white')
sheet.paste(a, (0, 0)); sheet.paste(b, (a.width, 0))
sheet.save(OUT)
print(OUT, sheet.size)
