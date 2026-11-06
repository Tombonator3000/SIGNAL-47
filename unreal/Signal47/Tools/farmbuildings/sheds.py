"""To skjul. Finnes ikke i dagens spill. Plassering er valgt ut fra driften, se README.

Skjul A: åpent maskinskjul med pulttak, 9 x 5,5 m, bølgeblikk på stolper, ulike bukter mot sør.
Skjul B: lite fôr- og redskapsskjul, 5 x 4 m, stående bord og lavt saltak, dør som står åpen ut.
"""
import math

from fbkit import Kit, wall, battens, roof, finish, ucx_box, rough_skirt
from barn import plank_leaf, frame

A_W2, A_D2 = 4.5, 2.75
A_HF, A_HB = 3.55, 2.65
A_POSTS = [-4.4, -1.75, 1.6, 4.4]
B_W2, B_D2 = 2.5, 2.0
B_EAVE, B_RIDGE = 2.3, 3.05


def azf(y):
    return A_HF + (A_HB - A_HF) * (y + A_D2) / (2 * A_D2)


def tin_ribs(k, F, u0, u1, top, base, step=.8, holes=()):
    """Blikkplater lagt utenpå veggen, hver med egen alder, skjøt og en sjelden lapp."""
    if k.lod == 2:
        return
    step = step if k.lod == 0 else step * 2.2
    u = u0
    while u < u1 - .05:
        un = min(u1, u + step)
        zt = min(top(u), top(un)) - .02
        roll = k.rng.random()
        if roll < .16:
            tint = (1.12, 1.11, 1.06)        # Nyere og lysere.
        elif roll < .5:
            tint = (.97, .84, .72)           # Eldre, rustbrun.
        else:
            w = .88 + k.rng.random() * .14
            tint = (w, w * .98, w * .94)
        off = (k.rng.random() * 3, .08)
        rects = [(u, un, base, zt)]
        for h0, h1, h2, h3 in holes:
            new = []
            for a, b, c, e in rects:
                if b <= h0 or a >= h1 or e <= h2 or c >= h3:
                    new.append((a, b, c, e))
                    continue
                if a < h0:
                    new.append((a, h0, c, e))
                if b > h1:
                    new.append((h1, b, c, e))
                if c < h2:
                    new.append((max(a, h0), min(b, h1), c, h2))
                if e > h3:
                    new.append((max(a, h0), min(b, h1), h3, e))
            rects = new
        for a, b, c, e in rects:
            F.quad('tinSheet', a, b, c, e, .004, 'Tin', tint, 2, off)
        clear = not any(h0 - .1 < un and h1 + .1 > u for h0, h1, _, _ in holes)
        if k.lod == 0 and zt - base > 2.3 and clear and k.rng.random() < .6:
            # Skjøt på høyden: øvre plate ligger over den nedre.
            zs = 1.7 + k.rng.random() * .6
            F.box('tinLapH', u, un, zs - .012, zs + .012, .004, .012, 'Tin', 0, ((1, -1),), (.82, .78, .72))
        if k.lod == 0 and not any(h0 - .03 < un < h1 + .03 for h0, h1, _, _ in holes):
            F.box('tinLap', un - .022, un + .022, base, zt, .004, .014, 'Tin', 2, ((1, -1),), (.86, .84, .8))
        if k.lod == 0 and clear and k.rng.random() < .16:
            # Løs plate som står ut nederst.
            start = len(k.v)
            F.box('tinLoose', u + .04, un - .04, base + .1, min(base + 1.5, zt), .012, .022, 'Tin', 2, ((1, -1),), (.93, .86, .78))
            k.rotate_since(start, F.P((u + un) / 2, min(base + 1.5, zt), .015), F.du, .02)
        u = un


def build_shed_a(lod=0):
    k = Kit('SM_FarmShedA', lod, 912)
    it = (1, 1, 1)
    rows = (.45,)
    base = -.25
    win_n = (6.3, 7.1, 1.25, 1.85)
    win_w = (2.5, 3.3, 1.2, 1.85)
    N = wall(k, 'north', (A_W2, A_D2), (-A_W2, A_D2), A_HB + .03, [win_n], .03, base, 'Tin', rows=rows, itint=it, rmat='Steel', v0=.08)
    ye = -A_D2 + .15
    Lq = 2 * A_D2 - .15
    E = wall(k, 'east', (A_W2, ye), (A_W2, A_D2), [(0, azf(ye) + .03), (Lq, A_HB + .03)], [], .03, base, 'Tin', rows=rows, itint=it, rmat='Steel', v0=.08)
    Wst = wall(k, 'west', (-A_W2, A_D2), (-A_W2, ye), [(0, A_HB + .03), (Lq, azf(ye) + .03)], [win_w], .03, base, 'Tin', rows=rows, itint=it, rmat='Steel', v0=.08)
    tin_ribs(k, N, 0, 9, lambda u: A_HB, base, holes=[win_n])
    tin_ribs(k, E, 0, Lq, lambda u: azf(ye + u), base)
    tin_ribs(k, Wst, 0, Lq, lambda u: azf(A_D2 - u), base, holes=[win_w])
    # Hjørnebord lukker skjøtene.
    for x, sx in ((-A_W2, -1), (A_W2, 1)):
        k.bx('corner', min(x, x + sx * .025), max(x, x + sx * .025), A_D2 - .16, A_D2 + .025, base, A_HB - .02, 'Wood', tint=(.85, .83, .8))
        x2 = x - sx * .16
        k.bx('corner', min(x2, x), max(x2, x), A_D2, A_D2 + .025, base, A_HB - .02, 'Wood', tint=(.85, .83, .8))
    # Stolper, ulike bukter. Den ene står litt skjevt, og en knekt mangler.
    head = A_HF - .47
    back = A_HB - .45
    for i, x in enumerate(A_POSTS):
        lean = .05 if i == 1 and lod < 2 else 0
        if lean:
            k.beam('postFront', (x, -A_D2 + .12, -.5), (x + lean, -A_D2 + .12, head), .19, .19, 'Wood', (0, 1, 0), (1.05, 1, .94))
        else:
            k.bx('postFront', x - .095, x + .095, -A_D2 + .025, -A_D2 + .215, -.5, head, 'Wood', tint=(1.05, 1, .94))
        k.bx('postBack', x - .08, x + .08, A_D2 - .18, A_D2 - .03, -.5, back, 'Wood', tint=(.9, .88, .84))
    # Bærebjelke oppå stolpene, toppsvill bak. Sperrene ligger oppå dem.
    k.bx('header', -A_W2, A_W2, -A_D2 + .02, -A_D2 + .22, head, A_HF - .27, 'Wood', 0, tint=(1.05, 1, .94))
    k.bx('headerBack', -A_W2 + .04, A_W2 - .04, A_D2 - .19, A_D2 - .03, back, A_HB - .27, 'Wood', 0, tint=(.8, .78, .74))
    if lod < 2:
        x = -A_W2 + .05
        while x < A_W2:
            k.beam('rafter', (x, -A_D2 - .5, azf(-A_D2 - .5) - .17), (x, A_D2 + .05, azf(A_D2 + .05) - .17), .06, .13, 'Wood', (0, 0, 1))
            x += 1.0 if lod == 0 else 2.2
        for a, s in ((A_POSTS[0], 1), (A_POSTS[1], 1), (A_POSTS[2], -1), (A_POSTS[3], -1)):
            k.beam('knee', (a, -A_D2 + .12, head - .65), (a + s * .65, -A_D2 + .12, head), .1, .1, 'Wood', (0, 1, 0))
    if lod == 0:
        for frac in (.2, .4, .6, .8):
            y = -A_D2 + frac * 2 * A_D2
            k.bx('purlin', -A_W2 + .05, A_W2 - .05, y - .035, y + .035, azf(y) - .1, azf(y) - .03, 'Wood', 0)
        for z in (.9, 1.9):
            k.bx('girt', -A_W2 + .05, A_W2 - .05, A_D2 - .11, A_D2 - .04, z - .05, z + .05, 'Wood', 0, tint=(.75, .72, .68))
            for x0 in (-A_W2 + .04, A_W2 - .11):
                k.bx('girt', x0, x0 + .07, -A_D2 + .22, A_D2 - .11, z - .05, z + .05, 'Wood', 1, tint=(.75, .72, .68))
    # Vinduer.
    N.quad('northGlass', win_n[0], win_n[1], win_n[2], win_n[3], -.02, 'Glass')
    Wst.quad('westGlass', win_w[0], win_w[1], win_w[2], win_w[3], -.02, 'Glass')
    if lod < 2:
        for F, h in ((N, win_n), (Wst, win_w)):
            frame(F, 'frame', h[0], h[1], h[2], h[3], .08, 'Wood', True, .035)
            um = (h[0] + h[1]) / 2
            F.box('muntin', um - .015, um + .015, h[2], h[3], -.035, 0, 'Wood')
    # Tak: tydelig utstikk og kant.
    y0, y1 = -A_D2 - .6, A_D2 + .3
    z0, z1 = azf(y0) + .04, azf(y1) + .04
    ov = .4
    roof(k, 'roof', (A_W2 + ov, y1, z1), (-A_W2 - ov, y1, z1), (-A_W2 - ov, y0, z0), (A_W2 + ov, y0, z0), [.84, 1.68, 3.0][lod], .55)
    k.beam('fascia', (-A_W2 - ov, y0, z0 - .08), (A_W2 + ov, y0, z0 - .08), .04, .15, 'Wood', (0, 0, 1))
    k.beam('fasciaBack', (-A_W2 - ov, y1, z1 - .08), (A_W2 + ov, y1, z1 - .08), .05, .15, 'Wood', (0, 0, 1))
    for x in (-A_W2 - ov, A_W2 + ov):
        k.beam('rake', (x, y0, z0 - .07), (x, y1, z1 - .07), .04, .14, 'Wood', (0, 0, 1))
    # Jordgulv og en arbeidsbenk bak.
    k.ambient = 1
    k.bx('floor', -A_W2 + .03, A_W2 - .03, -A_D2 - .02, A_D2 - .03, -.1, .02, 'Dirt', skip=((2, -1),))
    k.ambient = .9
    if lod < 2:
        k.bx('bench', 1.0, 4.1, A_D2 - .9, A_D2 - .12, .78, .85, 'Wood', 0)
        for x in (1.1, 2.55, 4.0):
            for y in (A_D2 - .82, A_D2 - .2):
                k.bx('benchLeg', x - .05, x + .05, y - .05, y + .05, .02, .78, 'Wood')
        k.bx('benchShelf', 1.05, 4.05, A_D2 - .85, A_D2 - .15, .28, .32, 'Wood', 0)
    k.ambient = 1
    for r in ((-A_W2, A_W2, A_D2 - .05, A_D2), (-A_W2, -A_W2 + .05, ye, A_D2), (A_W2 - .05, A_W2, ye, A_D2)):
        k.solid(*r)
    for x in A_POSTS:
        k.solid(x - .095, x + .095, -A_D2 + .025, -A_D2 + .215)
    return finish(k)


def collision_shed_a():
    ye = -A_D2 + .15
    out = [ucx_box(-A_W2, A_W2, A_D2 - .08, A_D2, 0, A_HB), ucx_box(-A_W2, -A_W2 + .08, ye, A_D2, 0, A_HB), ucx_box(A_W2 - .08, A_W2, ye, A_D2, 0, A_HB)]
    for x in A_POSTS:
        out.append(ucx_box(x - .095, x + .095, -A_D2 + .025, -A_D2 + .215, 0, A_HF - .47))
    y0, y1 = -A_D2 - .6, A_D2 + .3
    ov = .4
    a, b = azf(y0), azf(y1)
    out.append([(-A_W2 - ov, y0, a - .25), (A_W2 + ov, y0, a - .25), (-A_W2 - ov, y0, a + .05), (A_W2 + ov, y0, a + .05),
                (-A_W2 - ov, y1, b - .25), (A_W2 + ov, y1, b - .25), (-A_W2 - ov, y1, b + .05), (A_W2 + ov, y1, b + .05)])
    return out


def build_shed_b(lod=0):
    k = Kit('SM_FarmShedB', lod, 913)
    t = .05
    it = (.8, .8, .8)
    rows = (.2, .7)
    gable = [(0, B_EAVE + .08), (B_W2, B_RIDGE + .08), (2 * B_W2, B_EAVE + .08)]
    door = (1.0, 1.98, 0, 1.95)
    win_s = (3.2, 3.85, 1.12, 1.68)
    win_e = (1.6, 2.3, 1.2, 1.78)
    hatch = (2.15, 2.85, 2.05, 2.62)
    S = wall(k, 'south', (-B_W2, -B_D2), (B_W2, -B_D2), gable, [door, win_s], t, 0., rows=rows, itint=it)
    E = wall(k, 'east', (B_W2, -B_D2), (B_W2, B_D2), B_EAVE + .08, [win_e], t, 0., rows=rows, itint=it)
    N = wall(k, 'north', (B_W2, B_D2), (-B_W2, B_D2), gable, [hatch], t, 0., rows=rows, itint=it)
    Wst = wall(k, 'west', (-B_W2, B_D2), (-B_W2, -B_D2), B_EAVE + .08, [], t, 0., rows=rows, itint=it)
    # Lav betongsokkel, støpt i omganger, varm av jord.
    rough_skirt(k, S, -.05, door[0], -.5, .17, .05)
    rough_skirt(k, S, door[1], 2 * B_W2 + .05, -.5, .17, .05)
    rough_skirt(k, E, .05, 2 * B_D2 - .05, -.5, .17, .05)
    rough_skirt(k, N, -.05, 2 * B_W2 + .05, -.5, .17, .05)
    rough_skirt(k, Wst, .05, 2 * B_D2 - .05, -.5, .17, .05)
    k.bx('doorStone', door[0] - B_W2 - .15, door[1] - B_W2 + .1, -B_D2 - .5, -B_D2 + .02, -.4, .1, 'Concrete', 0, skip=((2, -1),))
    if lod < 2:
        step = .5 if lod == 0 else 1.4
        battens(k, S, 'batten', 0, 2 * B_W2, step, gable, [door, win_s], .17)
        battens(k, E, 'batten', 0, 2 * B_D2, step, B_EAVE, [win_e], .17)
        battens(k, N, 'batten', 0, 2 * B_W2, step, gable, [hatch], .17)
        battens(k, Wst, 'batten', 0, 2 * B_D2, step, B_EAVE, [], .17)
        for x, sx in ((-B_W2, -1), (B_W2, 1)):
            for y, sy in ((-B_D2, -1), (B_D2, 1)):
                k.bx('corner', min(x, x - sx * .15), max(x, x - sx * .15), min(y, y + sy * .03), max(y, y + sy * .03), .17, B_EAVE - .02, 'Wood')
                k.bx('corner', min(x, x + sx * .03), max(x, x + sx * .03), min(y - sy * .15, y), max(y - sy * .15, y), .17, B_EAVE - .02, 'Wood')
    # Dør med rester av samme turkise maling som huset. Står åpen ut, holdt av en kloss.
    if lod < 2:
        frame(S, 'doorFrame', door[0], door[1], .05, door[3], .1, 'Paint', False)
    a = math.radians(-68)
    hinge = S.P(door[0] + .01, 0, .01)
    ln = door[1] - door[0] - .02
    dx, dy = math.cos(a), math.sin(a)
    tip = (hinge[0] + dx * ln, hinge[1] + dy * ln)
    cx, cy = (hinge[0] + tip[0]) / 2, (hinge[1] + tip[1]) / 2
    nrm = (dy, -dx, 0)   # Yttersiden når døra er lukket.
    door_start = len(k.v)
    k.obox('doorLeaf', (cx, cy, 1.0), (dx, dy, 0), nrm, (0, 0, 1), (ln, .04, 1.9), 'Paint', 2)
    if lod < 2:
        for z, h in ((.2, .12), (1.0, .12), (1.8, .12)):
            k.obox('doorRail', (cx + nrm[0] * .03, cy + nrm[1] * .03, z), (dx, dy, 0), nrm, (0, 0, 1), (ln, .022, h), 'Paint', 0)
    if lod == 0:
        for g in (.2, .4, .58, .77):
            c = (hinge[0] + dx * g + nrm[0] * .0215, hinge[1] + dy * g + nrm[1] * .0215, 1.0)
            k.obox('doorGroove', c, (dx, dy, 0), nrm, (0, 0, 1), (.012, .003, 1.88), 'Wood', 2, tint=(.25, .24, .22))
        k.pipe('doorHandle', (hinge[0] + dx * .86 + nrm[0] * .03, hinge[1] + dy * .86 + nrm[1] * .03, 1.0),
               (hinge[0] + dx * .86 + nrm[0] * .08, hinge[1] + dy * .86 + nrm[1] * .08, 1.0), .012, 'Wood', 6, (.3, .28, .26))
        for z in (.4, 1.55):
            c = (hinge[0] + dx * .16 + nrm[0] * .044, hinge[1] + dy * .16 + nrm[1] * .044, z)
            k.obox('hinge', c, (dx, dy, 0), nrm, (0, 0, 1), (.32, .008, .035), 'Wood', 0, tint=(.3, .28, .26))
    if lod == 0:
        k.rotate_since(door_start, (hinge[0], hinge[1], 1.9), nrm, .014)   # Døra henger litt ned i ytterkant.
    # Vinduer: dyp karm og sålbrett.
    for F, h in ((S, win_s), (E, win_e)):
        F.quad('glass', h[0], h[1], h[2], h[3], -.035, 'Glass')
        if lod < 2:
            frame(F, 'frame', h[0], h[1], h[2], h[3], .065, 'Paint', True, .035)
            um, zm = (h[0] + h[1]) / 2, (h[2] + h[3]) / 2
            F.box('muntin', um - .012, um + .012, h[2], h[3], -.045, -.018, 'Paint')
            F.box('muntin', h[0], um - .012, zm - .012, zm + .012, -.045, -.018, 'Paint', 0)
            F.box('muntin', um + .012, h[1], zm - .012, zm + .012, -.045, -.018, 'Paint', 0)
    # Lukket lem i bakgavlen, med hengselbånd.
    plank_leaf(N, 'hatchLeaf', hatch[0], hatch[1], hatch[2], hatch[3], -t * .8, .012, k, False)
    if lod < 2:
        frame(N, 'hatchFrame', hatch[0], hatch[1], hatch[2], hatch[3], .06, 'Wood', False)
    if lod == 0:
        for z in (hatch[2] + .12, hatch[3] - .12):
            N.box('hatchHinge', hatch[1] - .3, hatch[1] + .04, z - .015, z + .015, .037, .043, 'Wood', 0, tint=(.3, .28, .26))

    # Tak: lavt, utstikk 0,4 på gavl og 0,42 på raft.
    ov, ry = .42, B_D2 + .4
    sl = (B_RIDGE - B_EAVE) / B_W2
    zr = B_RIDGE + .09
    ze = zr - sl * (B_W2 + ov)
    xe = B_W2 + ov
    strip = [.84, 1.68, 3.0][lod]
    roof(k, 'roofE', (xe, -ry, ze), (xe, ry, ze), (0, ry, zr), (0, -ry, zr), strip, .45)
    roof(k, 'roofW', (-xe, ry, ze), (-xe, -ry, ze), (0, -ry, zr), (0, ry, zr), strip, .45)
    if lod < 2:
        cap = .16
        for s in (-1, 1):
            k.face([(0, -ry, zr + .025), (0, ry, zr + .025), (s * cap, ry, zr + .025 - cap * sl + .01), (s * cap, -ry, zr + .025 - cap * sl + .01)],
                   'Tin', [(0, 0), (0, 3), (.1, 3), (.1, 0)], False, (1, .9, .78), (0, 0, 1))
    for y in (-ry, ry):
        for s in (-1, 1):
            k.beam('rake', (s * xe, y, ze - .03), (0, y, zr - .03), .04, .15, 'Wood', (0, 0, 1))
    for s in (-1, 1):
        k.beam('eave', (s * xe, -ry, ze - .07), (s * xe, ry, ze - .07), .04, .13, 'Wood', (0, 0, 1))
    # Innvendig: jordgulv, sperrer, hylle.
    k.ambient = .95
    k.bx('floor', -B_W2 + .04, B_W2 - .04, -B_D2 + .04, B_D2 - .04, -.1, .03, 'Dirt', skip=((2, -1),))
    k.ambient = .6
    if lod < 2:
        y = -1.5
        while y < 1.6:
            for s in (-1, 1):
                k.beam('rafter', (s * (B_W2 - .06), y, B_EAVE), (0, y, B_RIDGE - .02), .05, .09, 'Wood', (0, 0, 1))
            y += 1.0
        k.bx('tie', -B_W2 + .05, B_W2 - .05, -.05, .05, B_EAVE - .1, B_EAVE, 'Wood', 0)
    if lod == 0:
        for z in (1.0, 1.55):
            k.bx('shelf', -B_W2 + .06, -B_W2 + .46, -1.5, 1.5, z, z + .03, 'Wood', 1)
        for y in (-1.45, 0, 1.45):
            k.bx('shelfPost', -B_W2 + .4, -B_W2 + .46, y - .03, y + .03, .03, 1.6, 'Wood')
    k.ambient = 1
    k.solid(-B_W2, door[0] - B_W2, -B_D2, -B_D2 + .05)
    k.solid(door[1] - B_W2, B_W2, -B_D2, -B_D2 + .05)
    k.solid(-B_W2, B_W2, B_D2 - .05, B_D2)
    k.solid(-B_W2, -B_W2 + .05, -B_D2, B_D2)
    k.solid(B_W2 - .05, B_W2, -B_D2, B_D2)
    return finish(k)


def collision_shed_b():
    z = B_EAVE
    door0, door1 = 1.0 - B_W2, 1.98 - B_W2
    ov, ry = .42, B_D2 + .4
    sl = (B_RIDGE - B_EAVE) / B_W2
    zr = B_RIDGE + .09
    xe = B_W2 + ov
    ze = zr - sl * xe
    return [ucx_box(-B_W2, door0, -B_D2, -B_D2 + .05, 0, z), ucx_box(door1, B_W2, -B_D2, -B_D2 + .05, 0, z),
            ucx_box(-B_W2, B_W2, B_D2 - .05, B_D2, 0, z), ucx_box(-B_W2, -B_W2 + .05, -B_D2, B_D2, 0, z),
            ucx_box(B_W2 - .05, B_W2, -B_D2, B_D2, 0, z),
            [(-xe, -ry, ze - .05), (xe, -ry, ze - .05), (0, -ry, zr), (-xe, ry, ze - .05), (xe, ry, ze - .05), (0, ry, zr)]]
