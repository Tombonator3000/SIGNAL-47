"""Alderete-huset. 12 x 8 m som dagens boks, bebodd våningshus.

Adobe med jordpuss og sementsokkel, bølgeblikktak med møne øst-vest som dagens tak, overbygd portal
mot sør innenfor fotavtrykket (veien går rett sør for bygget), dype vindusnisjer, turkise karmer.
Tegn på folk: rund kjøler i nordvinduet, TV-antenne, pipe, strømstigerør, dørmatte og vaskesnor-stolpe.
Vinduet på østveggen lyser, der spillet allerede tegner en varm glød.
"""
from fbkit import Kit, Frame, wall, roof, lamp, finish, chamfer, ucx_box, rough_skirt, wall_stains

W2, D2 = 6.0, 4.0
POSTS = [-5.82, -2.35, 1.15, 5.82]   # Ulik avstand, og døra står ikke midt mellom to stolper.
BODY_S = -2.2             # Sørveggen ligger inne under portalen.
PITCH = .36
RIDGE_TOP = 4.85
FLOOR = .16
T = .36                   # Adobeveggens tykkelse.


def zroof(y):
    return RIDGE_TOP - PITCH * abs(y)


def window(k, F, name, u0, u1, z0, z1, *, lit=False, mid=True, trim='Paint', blind=0.):
    """Vindu i dyp nisje: glass langt inne, karm, sprosser, sålbenk og tynn omramming."""
    rec = -T * .78
    mat = 'WindowLit' if lit else 'Glass'
    if lit:
        F.k.tag = name
        F.quad(name + 'Glass', u0, u1, z0, z1, rec, 'WindowLit', grain=2, off=(0, 0))
    else:
        F.quad(name + 'Glass', u0, u1, z0, z1, rec, 'Glass')
    fw = .055
    if k.lod < 2:
        F.box(name + 'Sash', u0, u0 + fw, z0, z1, rec - .02, rec + .035, trim)
        F.box(name + 'Sash', u1 - fw, u1, z0, z1, rec - .02, rec + .035, trim)
        F.box(name + 'Sash', u0 + fw, u1 - fw, z1 - fw, z1, rec - .02, rec + .035, trim, 0)
        F.box(name + 'Sash', u0 + fw, u1 - fw, z0, z0 + fw, rec - .02, rec + .035, trim, 0)
        if mid:
            zm = (z0 + z1) / 2
            F.box(name + 'Rail', u0 + fw, u1 - fw, zm - .025, zm + .025, rec - .01, rec + .04, trim, 0)
            um = (u0 + u1) / 2
            F.box(name + 'Mullion', um - .018, um + .018, z0 + fw, z1 - fw, rec - .01, rec + .03, trim)
        # Tung overligger i tre over nisjen, i stedet for omramming på pussen.
        F.box(name + 'Lintel', u0 - .16, u1 + .16, z1 + .01, z1 + .14, -.015, .025, 'Wood', 0, tint=(.95, .9, .85))
    # Sålbenk som stikker ut og kaster skygge.
    F.box(name + 'Sill', u0 - .1, u1 + .1, z0 - .075, z0, -T * .78, .1, 'Wood', 0, tint=(.9, .88, .85))
    if k.lod == 0 and blind > 0:
        # Persienne halvveis ned, som i referansene.
        n = int((z1 - z0) * blind / .058)
        for i in range(n):
            zz = z1 - .08 - i * .058
            F.box(name + 'Slat', u0 + .055, u1 - .055, zz - .014, zz + .014, rec + .005, rec + .018, 'Concrete', 0, tint=(1.0, .98, .92))
    # Enkel gardinkant innenfor glasset ved lavt nivå: gir ikke mer enn mørkt rom.


def door(k, F, name, u0, u1, z0, z1, *, screen=False):
    """Turkis dør i nisje med speil, klinke og eventuelt nettingdør utenpå."""
    rec = -T * .75
    F.box(name, u0 + .02, u1 - .02, z0, z1 - .02, rec, rec + .05, 'Paint', 2)
    if k.lod < 2:
        p = .11
        for a, b, c, e in ((u0 + .02 + p, u1 - .02 - p, z1 - p - .02, z1 - .02), (u0 + .02 + p, u1 - .02 - p, z0, z0 + p + .05),
                           (u0 + .02 + p, u1 - .02 - p, (z0 + z1) / 2 - .04, (z0 + z1) / 2 + .04)):
            F.box(name + 'Rail', a, b, c, e, rec + .05, rec + .075, 'Paint', 0)
        for a, b in ((u0 + .02, u0 + .02 + p), (u1 - .02 - p, u1 - .02)):
            F.box(name + 'Stile', a, b, z0, z1 - .02, rec + .05, rec + .075, 'Paint', 2)
        # Bred turkis karm inne i lysningen, tung overligger i tre ute.
        F.box(name + 'Jamb', u0, u0 + .1, z0, z1 - .02, rec + .07, -.02, 'Paint')
        F.box(name + 'Jamb', u1 - .1, u1, z0, z1 - .02, rec + .07, -.02, 'Paint')
        F.box(name + 'Jamb', u0 + .1, u1 - .1, z1 - .1, z1, rec + .07, -.02, 'Paint', 0)
        F.box(name + 'Lintel', u0 - .2, u1 + .2, z1 + .01, z1 + .15, -.015, .025, 'Wood', 0, tint=(.95, .9, .85))
    if k.lod == 0:
        zk = z0 + 1.0
        F.box(name + 'Plate', u1 - .13, u1 - .08, zk - .1, zk + .1, rec + .075, rec + .085, 'Steel')
        k.pipe(name + 'Knob', F.P(u1 - .105, zk, rec + .08), F.P(u1 - .105, zk, rec + .125), .028, 'Steel', 8)
        if screen:
            d0, d1 = -.05, -.015
            for a, b in ((u0 + .02, u0 + .09), (u1 - .09, u1 - .02)):
                F.box(name + 'Screen', a, b, z0 + .02, z1 - .03, d0, d1, 'Wood', 2, tint=(1.1, 1.06, 1))
            for c0, c1 in ((z0 + .02, z0 + .12), (z0 + .9, z0 + .98), (z1 - .1, z1 - .03)):
                F.box(name + 'Screen', u0 + .09, u1 - .09, c0, c1, d0, d1, 'Wood', 0, tint=(1.1, 1.06, 1))
            F.quad(name + 'Mesh', u0 + .09, u1 - .09, z0 + .98, z1 - .1, -.033, 'Steel', (.3, .3, .3))
            F.quad(name + 'Mesh', u0 + .09, u1 - .09, z0 + .12, z0 + .9, -.033, 'Steel', (.3, .3, .3))


def build(lod=0):
    k = Kit('SM_FarmHouse', lod, 911)
    rows = (.45, .8)
    ztop = lambda y: zroof(y) - .13
    # Sørvegg under portalen: hoveddør, stuevindu, kjøkkenvindu.
    fd = (5.0 - .49, 5.0 + .49, FLOOR, 2.26)
    w1 = (1.3, 2.85, .9, 2.2)
    w2 = (8.0, 9.25, 1.0, 2.2)
    holes_s = [fd, w1, w2]
    S = wall(k, 'south', (-W2, BODY_S), (W2, BODY_S), ztop(BODY_S), holes_s, T, -.5, 'Stucco', rows=rows, itint=(.7, .7, .7))
    # Østgavl: det lysende vinduet ligger ved lokal y = -1, som gløden i spillet.
    ez = [(0, ztop(BODY_S)), (-BODY_S, ztop(0)), (D2 - BODY_S, ztop(D2))]
    we = (.65, 1.75, .95, 2.15)
    we2 = (3.9, 4.8, 1.1, 2.1)
    E = wall(k, 'east', (W2, BODY_S), (W2, D2), ez, [we, we2], T, -.5, 'Stucco', rows=rows, itint=(.7, .7, .7))
    # Nord: bakdør, badevindu, kjøkkenvindu med kjøler.
    bd = (3.2, 4.1, FLOOR, 2.1)
    wn1 = (6.6, 7.2, 1.45, 2.1)
    wn2 = (8.6, 9.8, 1.05, 2.1)
    wn3 = (.9, 2.1, 1.05, 2.1)
    N = wall(k, 'north', (W2, D2), (-W2, D2), ztop(D2), [bd, wn1, wn2, wn3], T, -.5, 'Stucco', rows=rows, itint=(.7, .7, .7))
    # Vestgavl.
    wz = [(0, ztop(D2)), (D2, ztop(0)), (D2 - BODY_S, ztop(BODY_S))]
    ww = (1.8, 3.0, .95, 2.15)
    Wst = wall(k, 'west', (-W2, D2), (-W2, BODY_S), wz, [ww], T, -.5, 'Stucco', rows=rows, itint=(.7, .7, .7))

    # Sementsokkel som står litt ut fra pussen, ujevn topp, varm av jord.
    def skirt(F, cuts):
        prev = 0
        for a, b in cuts + [(F.L, F.L)]:
            if a > prev + .02:
                rough_skirt(k, F, prev, a, -.5, .46, .04)
            prev = b
    for x, y, sx, sy in ((-W2, BODY_S, -1, -1), (W2, BODY_S, 1, -1), (W2, D2, 1, 1), (-W2, D2, -1, 1)):
        chamfer(k, x, y, -.5, min(zroof(y), 3.0), .09, 'Stucco', sx, sy)
    skirt(S, [(fd[0], fd[1])])
    skirt(N, [(bd[0], bd[1])])
    skirt(E, [])
    skirt(Wst, [])

    if lod == 0:
        # Avrenning under sålbenker og rør, lapper i sement og bart adobe ved hjørner og sokkel.
        for F, holes in ((S, [w1, w2]), (E, [we, we2]), (N, [wn1, wn3]), (Wst, [ww])):
            for h0, h1, z0, z1 in holes:
                wall_stains(k, F, [(h0 + .12, z0 - .08, .06, .5), ((h0 + h1) / 2, z0 - .08, .08, .7), (h1 - .14, z0 - .08, .05, .35)])
        for F, u, z, ru, rz, tint in ((S, .55, .85, .34, .26, (1.06, 1.03, .98)), (E, 5.65, .75, .42, .22, (.9, .87, .82)), (N, 11.3, .95, .38, .3, (1.08, 1.04, .98)),
                                      (N, 5.2, .75, .5, .2, (.9, .86, .8)), (Wst, 5.7, 2.5, .3, .24, (1.07, 1.04, .98))):
            k.blob(F, u, z, ru, rz, .005, 'Stucco', tint)
    window(k, S, 'winLiving', *w1, blind=.45)
    window(k, S, 'winKitchen', *w2, blind=.3)
    window(k, E, 'winLit', *we, lit=True)
    window(k, E, 'winEast2', *we2, blind=.55)
    # Gardin i soverommet og ett vindu der persiennen henger skjevt.
    if lod == 0:
        rec = -T * .78
        for F, h0 in ((N, wn3), (Wst, ww)):
            F.quad('curtain', h0[0] + .06, h0[0] + .4, h0[2] + .06, h0[3] - .06, rec + .003, 'Concrete', (.96, .9, .74))
            F.quad('curtain', h0[1] - .4, h0[1] - .06, h0[2] + .06, h0[3] - .06, rec + .003, 'Concrete', (.96, .9, .74))
    window(k, N, 'winBath', *wn1, mid=False)
    window(k, N, 'winBack', *wn2)
    window(k, N, 'winBedroom', *wn3, blind=.6)
    window(k, Wst, 'winWest', *ww, blind=.4)
    door(k, S, 'frontDoor', *fd)
    door(k, N, 'backDoor', *bd)

    # Portal: stolper, sadelknekter, hovedbjelke, vigaer, himling av bord.
    y_post = -D2 + .14
    xs = POSTS
    zb = 2.66
    for x in xs:
        start = len(k.v)
        wd = .1 + (k.rng.random() - .5) * .018
        k.bx('post', x - wd, x + wd, y_post - wd, y_post + wd, FLOOR, zb, 'Wood', tint=(1.05, 1, .95))
        if lod == 0:
            k.rotate_since(start, (x, y_post, zb), (0, 1, 0), (k.rng.random() - .5) * .018)
        if lod == 0:
            # Enkel bunnsokkel under stolpen, og knekt med avfaset ende.
            k.bx('postBase', x - .125, x + .125, y_post - .125, y_post + .125, FLOOR, FLOOR + .05, 'Steel', tint=(.5, .48, .45))
        if lod < 2:
            k.bx('zapata', x - .5, x + .5, y_post - .095, y_post + .095, zb, zb + .12, 'Wood', 0, tint=(1.05, 1, .95))
            if lod == 0:
                for s in (-1, 1):
                    k.bx('zapata2', x + s * .1 + s * .16 - .16, x + s * .1 + s * .16 + .16, y_post - .09, y_post + .09, zb - .09, zb, 'Wood', 0, tint=(1.05, 1, .95))
    k.bx('header', -W2 - .02, W2 + .02, y_post - .12, y_post + .12, zb + .12, zb + .4, 'Wood', 0)
    if lod < 2:
        # Lavt rekkverk i det vestligste feltet, som i referansen.
        # Rekkverk på begge sider av feltet foran døra, som står åpent.
        for x0, x1 in ((xs[0] + .1, xs[1] - .1), (xs[2] + .1, xs[3] - .1)):
            k.bx('rail', x0, x1, y_post - .035, y_post + .035, .92, 1.0, 'Wood', 0, tint=(1.05, 1, .95))
            k.bx('rail', x0, x1, y_post - .03, y_post + .03, .32, .38, 'Wood', 0, tint=(1.05, 1, .95))
            step = .16 if lod == 0 else .48
            x = x0 + .1
            while x < x1 - .05:
                k.bx('baluster', x - .018, x + .018, y_post - .018, y_post + .018, .38, .92, 'Wood', tint=(1.05, 1, .95))
                x += step
    # Portalgulv: betongplate med kant og trappetrinn. Går ned i bakken.
    k.bx('slab', -W2 - .02, W2 + .02, -D2 - .03, BODY_S, -.5, FLOOR, 'Concrete', 0, skip=((2, -1),))
    k.bx('step', -1.75, -.25, -D2 - .36, -D2 - .03, -.5, FLOOR * .5, 'Concrete', 0, skip=((2, -1),))
    # Hele huset får plate under, og mørke vegger inni bak vinduene.
    k.ambient = .35
    k.bx('floorIn', -W2 + T, W2 - T, BODY_S + T, D2 - T, -.1, FLOOR - .02, 'Wood', 0, skip=((2, -1),))
    k.face([(-W2 + T, BODY_S + T, 2.62), (W2 - T, BODY_S + T, 2.62), (W2 - T, D2 - T, 2.62), (-W2 + T, D2 - T, 2.62)], 'Wood', n=(0, 0, -1))
    k.ambient = 1
    # Baktrapp.
    bx0, bx1 = W2 - bd[1] - .25, W2 - bd[0] + .25
    k.bx('backStep', bx0, bx1, D2, D2 + .9, -.5, FLOOR, 'Concrete', 0, skip=((2, -1),))

    # Vigaer som bærer portaltaket.
    if lod < 2:
        step = 1.12 if lod == 0 else 2.24
        x = -5.6
        while x < 5.7:
            z0 = zroof(BODY_S) - .2
            z1 = zroof(-D2) - .2
            k.rough_log('viga', (x, BODY_S, z0), (x, -D2 - .32, z1 - PITCH * .32), .085)
            x += step
    # Himling under portaltaket, bord som følger taket.
    k.ambient = .75
    yo = -D2 - .35
    k.uvq([(-W2 - .3, yo, zroof(yo) - .1), (W2 + .3, yo, zroof(yo) - .1), (W2 + .3, BODY_S, zroof(BODY_S) - .1), (-W2 - .3, BODY_S, zroof(BODY_S) - .1)],
          'Board', (0, 1, 0), (1, 0, 0), (.2, .1), (.75, .72, .68), (0, 0, -1))
    k.ambient = 1
    # Trekant over portalåpningen ved gavlene: kledd med bord.
    for s, xw in ((1, W2), (-1, -W2)):
        ps = [(xw, -D2, zb + .4), (xw, BODY_S, zb + .4), (xw, BODY_S, zroof(BODY_S) - .1), (xw, -D2, zroof(-D2) - .1)]
        k.uvq(ps, 'Board', (0, 1, 0), (0, 0, 1), (k.rng.random(), .2), (.9, .88, .84), (s, 0, 0))
        ps2 = [(p[0] - s * .03, p[1], p[2]) for p in ps]
        k.uvq(ps2, 'Board', (0, 1, 0), (0, 0, 1), (k.rng.random(), .2), (.6, .58, .55), (-s, 0, 0))

    # Tak med utstikk.
    ro, re = .42, .42
    zr = RIDGE_TOP
    strip = [.84, 1.68, 3.2][lod]
    zs = zroof(-D2 - re)
    zn = zroof(D2 + re)
    roof(k, 'roofS', (-W2 - ro, -D2 - re, zs), (W2 + ro, -D2 - re, zs), (W2 + ro, 0, zr), (-W2 - ro, 0, zr), strip, .16)
    roof(k, 'roofN', (W2 + ro, D2 + re, zn), (-W2 - ro, D2 + re, zn), (-W2 - ro, 0, zr), (W2 + ro, 0, zr), strip, .16)
    if lod < 2:
        cap = .16
        for s in (-1, 1):
            k.face([(-W2 - ro, 0, zr + .025), (W2 + ro, 0, zr + .025), (W2 + ro, s * cap, zr + .025 - cap * PITCH + .01), (-W2 - ro, s * cap, zr + .025 - cap * PITCH + .01)],
                   'Tin', [(0, 0), (8, 0), (8, .1), (0, .1)], False, (.95, .92, .86), (0, 0, 1))
    # Vindskier og rafteplanker i malt tre.
    for x in (-W2 - ro, W2 + ro):
        k.beam('rake', (x, -D2 - re, zs - .05), (x, 0, zr - .05), .05, .2, 'Wood', (0, 0, 1))
        k.beam('rake', (x, D2 + re, zn - .05), (x, 0, zr - .05), .05, .2, 'Wood', (0, 0, 1))
    k.beam('fascia', (-W2 - ro, -D2 - re, zs - .09), (W2 + ro, -D2 - re, zs - .09), .05, .2, 'Wood', (0, 0, 1))
    k.beam('fascia', (-W2 - ro, D2 + re, zn - .09), (W2 + ro, D2 + re, zn - .09), .05, .2, 'Wood', (0, 0, 1))

    # Pipe i puss med stålrør, og ovnsrør på nordsiden av taket.
    cx, cy = 2.9, 1.6
    k.bx('chimney', cx - .35, cx + .35, cy - .35, cy + .35, zroof(cy) - .4, 5.7, 'Stucco', 2, skip=((2, -1),))
    k.bx('chimneyCap', cx - .4, cx + .4, cy - .4, cy + .4, 5.7, 5.78, 'Concrete', 0, skip=((2, -1),))
    k.pipe('flue', (cx, cy, 5.78), (cx, cy, 6.0), .1, 'Steel')
    if lod < 2:
        sx, sy = -2.6, 1.9
        k.pipe('stove', (sx, sy, zroof(sy) - .1), (sx, sy, 5.2), .075, 'Steel')
        k.lathe('stoveCap', (sx, sy, 5.2), [(.04, .075), (.1, .18), (.22, .02)], 'Steel', [10, 6, 4][lod], caps=True)
        # Flens på taket.
        k.pipe('flash', (sx, sy, zroof(sy) - .02), (sx, sy, zroof(sy) + .05), .16, 'Tin', [10, 6, 4][lod])

    # Rund kjøler ("swamp cooler") ved vest-nord-vinduet, på stativ.
    F = N
    u0, u1 = wn2[0], wn2[1]
    cz0, cz1 = wn2[2] + .02, wn2[2] + .95
    uc0, uc1 = u0 + .12, u1 - .12
    F.box('coolerBody', uc0, uc1, cz0 + .08, cz1 + .08, .02, .78, 'Tin', 2, tint=(.66, .63, .56), skip=((1, -1),))
    F.box('coolerTop', uc0 - .03, uc1 + .03, cz1 + .08, cz1 + .12, -.01, .81, 'Steel', 0)
    F.box('coolerStand', uc0, uc1, cz0 - .22, cz0 + .08, .06, .72, 'Steel', 0)
    if lod < 2:
        for a in (uc0 + .03, uc1 - .08):
            for d0 in (.72, .08):
                F.box('coolerLeg', a, a + .05, -.5, cz0 - .22, d0, d0 + .05, 'Steel')
    if lod == 0:
        # Lameller på fronten og sidene, slange inn til veggen.
        for i in range(7):
            zz = cz0 + .2 + i * .1
            F.box('coolerLouver', uc0 + .06, uc1 - .06, zz, zz + .05, .78, .8, 'Tin', 0, tint=(.46, .44, .4))
            F.box('coolerLouver', uc0 - .015, uc0, zz, zz + .05, .12, .68, 'Tin', 0, tint=(.46, .44, .4))
            F.box('coolerLouver', uc1, uc1 + .015, zz, zz + .05, .12, .68, 'Tin', 0, tint=(.46, .44, .4))
        k.pipe('coolerHose', F.P(uc0 + .1, cz0, .3), F.P(uc0 + .1, .0, .3), .012, 'Steel', 5)

    # Strøm: stigerør med hode og måler på vestgavlen. TV-antenne på mast.
    Fw = Wst
    if lod < 2:
        Fw.box('meter', 4.68, 4.98, 1.25, 1.65, 0, .15, 'Steel')
        k.pipe('riser', Fw.P(4.83, 1.65, .05), Fw.P(4.83, 4.75, .05), .022, 'Steel', [6, 4, 4][lod])
    if lod == 0:
        k.pipe('head', Fw.P(4.83, 4.75, .05), Fw.P(4.83, 4.88, .22), .022, 'Steel', 6)
        k.pipe('riser2', Fw.P(4.9, .9, .03), Fw.P(4.9, 1.25, .03), .012, 'Steel', 5)
    if lod < 2:
        mx, my = -W2 - .06, 1.1
        k.pipe('mast', (mx, my, 2.4), (mx, my, 6.7), .02, 'Steel', [6, 4, 4][lod])
        Fw.box('mastBracket', 2.85, 2.95, 3.12, 3.17, 0, .08, 'Steel', 0)
        Fw.box('mastBracket', 2.85, 2.95, 4.05, 4.1, 0, .08, 'Steel', 0)
    if lod == 0:
        mx, my = -W2 - .06, 1.1
        for i, z in enumerate((6.62, 6.42, 6.22, 6.02)):
            half = .62 - i * .08
            k.pipe('antenna', (mx, my - half, z), (mx, my + half, z), .006, 'Steel', 4)
        k.pipe('boom', (mx - .5, my, 6.55), (mx + .5, my, 6.55), .008, 'Steel', 4)
        # TV-kabel ned langs masten og inn i veggen.
        k.pipe('tvCable', (mx - .04, my, 6.0), (mx - .04, my, 2.35), .006, 'Steel', 4, (.25, .25, .25))
        k.pipe('tvCable', (mx - .04, my, 2.35), (mx + .08, my, 2.2), .006, 'Steel', 4, (.25, .25, .25))
    if lod < 2:
        # Takrenne foran med nedløp i østenden. Liten lufteluke i gavlene.
        gz = zs - .15
        k.bx('gutter', -W2 - ro, W2 + ro, -D2 - re - .14, -D2 - re - .03, gz - .05, gz + .05, 'Steel', 0)
        k.pipe('downspout', (W2 + ro - .15, -D2 - re - .08, gz - .03), (W2 - .2, -D2 + .1, 2.55), .035, 'Steel', [6, 4, 4][lod])
        for F, u in ((E, -BODY_S), (Wst, D2)):
            F.box('gableVent', u - .22, u + .22, 3.45, 3.89, .0, .03, 'Wood')
            if lod == 0:
                for i in range(5):
                    F.box('gableSlat', u - .19, u + .19, 3.49 + i * .08, 3.53 + i * .08, .03, .045, 'Wood', 0, tint=(.6, .58, .55))
    lamp(k, S, 3.95, 2.48, .3)
    lamp(k, N, 3.65, 2.45, .3)
    if lod == 0:
        # Dørmatte og utvendig kran ved døren.
        k.bx('mat', -1.42, -.55, BODY_S - .72, BODY_S - .08, FLOOR, FLOOR + .012, 'Concrete', 0, skip=((2, -1),), tint=(.42, .3, .22))
        S.box('faucetPipe', 3.02, 3.06, FLOOR, .7, .0, .04, 'Steel')

    k.solid(-W2, W2, BODY_S, D2)
    for x in xs:
        k.solid(x - .1, x + .1, y_post - .1, y_post + .1)
    k.solid(xs[0], xs[1], y_post - .04, y_post + .04)
    k.solid(xs[2], xs[3], y_post - .04, y_post + .04)
    return finish(k)


def collision():
    bz = 3.25
    ry = D2 + .42
    out = [ucx_box(-W2, W2, BODY_S, D2, 0, bz)]
    for x in POSTS:
        out.append(ucx_box(x - .1, x + .1, -D2 + .04, -D2 + .24, 0, 2.66))
    out.append(ucx_box(POSTS[0], POSTS[1], -D2 + .1, -D2 + .18, 0, 1.0))
    out.append(ucx_box(POSTS[2], POSTS[3], -D2 + .1, -D2 + .18, 0, 1.0))
    out.append(ucx_box(-W2 - .02, W2 + .02, -D2 - .03, BODY_S, -.2, FLOOR))
    # Takkile over hele bygget. Under ligger portalen åpen.
    z = zroof
    out.append([(-W2 - .42, -ry, z(ry) - .05), (W2 + .42, -ry, z(ry) - .05), (-W2 - .42, 0, RIDGE_TOP), (W2 + .42, 0, RIDGE_TOP),
                (-W2 - .42, ry, z(ry) - .05), (W2 + .42, ry, z(ry) - .05)])
    return out
