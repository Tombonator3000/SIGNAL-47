"""Låven. 16 x 11 m som dagens boks. Ett bredt, tungt saltak med gavl mot sør og tunet, og ett lavt
sideskip mot vest. Skjevheten er med vilje: referansene fra New Mexico er ikke symmetriske.

Stående bord, bølgeblikk, skyvedører på skinne, høyluke med bjelke. Rom å gå inn i: bred midtgang,
stolperekker, båser mot øst og i sideskipet, høyloft over bakre halvdel, stige, jordgulv.
"""
from fbkit import Kit, wall, battens, roof, lamp, finish, ucx_box, WALL_BASE

W2, D2 = 8.0, 5.5          # Halv bredde og dybde.
MX, MW = 1.8, 6.2          # Hovedskipets midtlinje og halve bredde: x -4,4 til 8.
PITCH = .445               # Omtrent 24 grader.
EAVE = 3.9
RIDGE = EAVE + PITCH * MW  # 6,66 m.
LEAN_OUT, LEAN_IN = 2.5, 3.4
LS = (LEAN_IN - LEAN_OUT) / 3.6
XW = MX - MW               # Skillet mellom hovedskip og sideskip.
LIFT = .13
FLOOR = .11                # Gulvet, over høyeste terreng i fotavtrykket: S47FarmBuildings.cpp Floors.
DOOR = (MX - 1.9, MX + 1.9, 3.4)
FREE = (DOOR[0], MX + .9)  # Åpningen skyvedøra faktisk lar stå fri.
POSTS = [-3.65, -1.15, 1.35, 3.85]
ROWS = [MX - 3.0, MX + 3.0]
LOFT_Z, LOFT_Y = 2.75, .95


def plank_leaf(F, name, u0, u1, z0, z1, d0, d1, k, brace=True, mat='Board', broken=False):
    """Dør av stående bord med overliggere og skråstag. broken: det ene staget har falt av."""
    tone = 1 + (k.rng.random() - .5) * .14
    F.box(name, u0, u1, z0, z1, d0, d1, mat, 2, tint=(tone, tone * .97, tone * .94))
    if k.lod > 1:
        return
    b = .11
    for zz in (z0 + .1, (z0 + z1) / 2, z1 - .1):
        F.box(name + 'Rail', u0, u1, zz - b / 2, zz + b / 2, d1, d1 + .025, 'Wood', 0)
    if k.lod == 0 and brace:
        a = F.P(u0 + .08, z0 + .15, d1 + .012)
        c = F.P(u1 - .08, (z0 + z1) / 2 - .05, d1 + .012)
        k.beam(name + 'Brace', a, c, .1, .022, 'Wood', F.dd)
        if not broken:
            a2 = F.P(u0 + .08, (z0 + z1) / 2 + .05, d1 + .012)
            c2 = F.P(u1 - .08, z1 - .15, d1 + .012)
            k.beam(name + 'Brace', a2, c2, .1, .022, 'Wood', F.dd)


def frame(F, name, u0, u1, z0, z1, c=.07, mat='Wood', sill=True, d=.03):
    """Karm uten overlappende flater, med sålbrett."""
    F.box(name, u0 - c, u0, z0, z1 + c, 0, d, mat)
    F.box(name, u1, u1 + c, z0, z1 + c, 0, d, mat)
    F.box(name, u0, u1, z1, z1 + c, 0, d * .9, mat, 0)
    if sill:
        F.box(name, u0 - c - .04, u1 + c + .04, z0 - .06, z0, -.02, d + .03, mat, 0)


def build(lod=0):
    k = Kit('SM_FarmBarn', lod, 910)
    t = .1
    it = (.82, .82, .82)
    w = 2 * MW
    door = (DOOR[0] - XW, DOOR[1] - XW, 0, DOOR[2])
    hay = (MW - .75, MW + .75, 4.2, 5.25)
    win_s = (10.0, 10.8, 1.3, 2.05)
    gable = [(0, EAVE + .1), (MW, RIDGE + .1), (w, EAVE + .1)]
    # Glipper mellom bord: små, ekte hull lavt i veggen som slipper lys inn.
    gaps = [(2.05, 2.17, .42, 1.12)] if lod == 0 else []
    S = wall(k, 'south', (XW, -D2), (W2, -D2), gable, [door, hay, win_s] + gaps, t, WALL_BASE, itint=it)
    lean_s = [(0, LEAN_OUT + .08), (3.6, LEAN_IN + .08)]
    pass_door = (1.25, 2.2, 0, 2.1)
    SW = wall(k, 'southLean', (-W2, -D2), (XW, -D2), lean_s, [pass_door], t, WALL_BASE, itint=it)
    east_h = [(1.0, 1.8, 1.35, 2.1), (4.5, 5.7, 1.2, 2.4), (8.3, 9.1, 1.35, 2.1), (3.0, 3.35, .38, .54), (7.2, 7.55, .38, .54)]
    if lod == 0:
        east_h += [(6.35, 6.47, .5, 1.05), (10.1, 10.2, .34, .9)]
    E = wall(k, 'east', (W2, -D2), (W2, D2), EAVE + .08, east_h, t, WALL_BASE, itint=it)
    rear = (MW - .6, MW + .6, 0, 2.15)
    hay2 = (MW - .55, MW + .55, 4.0, 4.95)
    N = wall(k, 'north', (W2, D2), (XW, D2), gable, [rear, hay2], t, WALL_BASE, itint=it)
    lean_n = [(0, LEAN_IN + .08), (3.6, LEAN_OUT + .08)]
    NW = wall(k, 'northLean', (XW, D2), (-W2, D2), lean_n, [(1.5, 2.2, 1.2, 1.85)], t, WALL_BASE, itint=it)
    west_h = [(1.4, 2.15, 1.2, 1.9), (6.4, 7.2, 1.15, 1.9)]
    if lod == 0:
        west_h += [(4.1, 4.2, .38, .95)]
    Wst = wall(k, 'west', (-W2, D2), (-W2, -D2), LEAN_OUT + .08, west_h, t, WALL_BASE, itint=it)
    # Veggstykket over sideskipstaket.
    up = wall(k, 'upperWest', (XW, D2), (XW, -D2), EAVE + .08, [], .08, LEAN_IN - .12, rows=(), itint=it)

    if lod == 0:
        # Noen eldre, utskiftede bordfelt i annen tone.
        for F, u, z, wd, hd, tone in ((N, 2.2, .35, 1.3, .95, .84), (Wst, 3.4, .35, .9, 1.1, .86)):
            F.box('patchBoards', u, u + wd, z, z + hd, .004, .02, 'Board', 0, ((1, -1),), (tone, tone, tone * .97))
        # Mer gliper lavt på veggene: lyset kommer inn.
    # Dekklister. Skyvedør og karm ligger utenpå.
    if lod < 2:
        step = .62 if lod == 0 else 1.5
        battens(k, S, 'batten', 0, w, step, gable, [door, hay, win_s] + gaps, WALL_BASE)
        battens(k, SW, 'batten', 0, 3.6, step, lean_s, [pass_door], WALL_BASE)
        battens(k, E, 'batten', 0, 2 * D2, step, EAVE, east_h, WALL_BASE)
        battens(k, Wst, 'batten', 0, 2 * D2, step, LEAN_OUT, west_h, WALL_BASE)
        battens(k, N, 'batten', 0, w, step, gable, [rear, hay2], WALL_BASE)
        battens(k, NW, 'batten', 0, 3.6, step, lean_n, [(1.5, 2.2, 1.2, 1.85)], WALL_BASE)
        battens(k, up, 'batten', 0, 2 * D2, step * 1.6, EAVE, [], LEAN_IN - .1)
        # Hjørnebord i to lag rundt alle ytterhjørner. Lean-to får egne.
        for x, y, sx, sy, z1 in ((-W2, -D2, -1, -1, LEAN_OUT), (W2, -D2, 1, -1, EAVE), (W2, D2, 1, 1, EAVE), (-W2, D2, -1, 1, LEAN_OUT)):
            a, b = (x, x + .16 * -sx) if sx > 0 else (x + .16 * -sx, x)
            k.bx('cornerA', min(x - .0, x - sx * .16), max(x, x - sx * .16), y, y + sy * .035, WALL_BASE, z1, 'Wood') if sy > 0 else \
                k.bx('cornerA', min(x, x - sx * .16), max(x, x - sx * .16), y + sy * .035, y, WALL_BASE, z1, 'Wood')
            k.bx('cornerB', x, x + sx * .035, min(y, y - sy * .16), max(y, y - sy * .16), WALL_BASE, z1, 'Wood') if sx > 0 else \
                k.bx('cornerB', x + sx * .035, x, min(y, y - sy * .16), max(y, y - sy * .16), WALL_BASE, z1, 'Wood')
        # Skjøtelist mellom hovedskip og sideskip på begge gavler.
        S.box('seam', -.08, .08, WALL_BASE, EAVE, 0, .03, 'Wood')
        N.box('seam', w - .08, w + .0, WALL_BASE, EAVE, 0, .03, 'Wood')

    # Skyvedører. Vestre står helt åpen, østre står halvveis igjen. Skinne langt forbi åpningen.
    track_z = DOOR[2] + .2
    uf = FREE[1] - XW
    plank_leaf(S, 'slideWest', door[0] - 2.1, door[0] - .02, .05, track_z, .07, .125, k, True, broken=True)
    start = len(k.v)
    plank_leaf(S, 'slideEast', uf, uf + 2.2, .1, track_z - .03, .07, .125, k, True)
    if lod == 0:
        k.rotate_since(start, S.P(uf + 1.1, track_z, .1), S.dd, .012)   # Henger litt på skakke.
    S.box('track', door[0] - 2.5, door[1] + 1.3, track_z - .02, track_z + .08, .03, .15, 'Steel', 0)
    if lod < 2:
        S.box('hood', door[0] - 2.55, door[1] + 1.35, track_z + .08, track_z + .2, .03, .24, 'Wood', 0)
        for u in (door[0] - 1.7, door[0] - .5, uf + .4, uf + 1.8):
            S.box('hanger', u - .035, u + .035, track_z - .26, track_z - .02, .125, .145, 'Steel')
            if lod == 0:
                k.pipe('roller', S.P(u, track_z + .03, .1), S.P(u, track_z + .03, .165), .05, 'Steel', 8)
        for u, sgn in ((door[0], -1), (door[1], 1)):
            a, b = sorted((u, u + sgn * .16))
            S.box('doorPost', a, b, WALL_BASE, DOOR[2], -.04, .05, 'Wood')
        S.box('doorHead', door[0] - .16, door[1] + .16, DOOR[2], DOOR[2] + .13, -.04, .045, 'Wood', 0)
    # Høyluke: lem som er slått til siden og henger litt skjevt, bjelke og trinse under mønet.
    start = len(k.v)
    plank_leaf(S, 'hayLeaf', hay[0] - 1.53, hay[0] - .1, 4.22, 5.2, .035, .075, k, False)
    if lod == 0:
        k.rotate_since(start, S.P(hay[0] - .1, 5.2, .05), S.dd, -.03)
    if lod < 2:
        frame(S, 'hayFrame', hay[0], hay[1], hay[2], hay[3], .1, 'Wood', True, .04)
    k.bx('hayBeam', MX - .09, MX + .09, -D2 - .95, -D2 + .55, 5.82, 6.05, 'Wood', 1)
    if lod == 0:
        k.pipe('hook', (MX, -D2 - .82, 5.82), (MX, -D2 - .82, 5.62), .02)
        k.torus('pulley', (MX, -D2 - .82, 5.53), .09, .018, 'Steel', 10, 5, (1, 0, 0))
    # Sidedør i sideskipet, plankedør med karm.
    plank_leaf(SW, 'passDoor', pass_door[0], pass_door[1], .0, pass_door[3], -t * .7, -t * .3, k, True)
    if lod < 2:
        frame(SW, 'passFrame', pass_door[0], pass_door[1], 0.05, pass_door[3], .08, 'Wood', False)
    # Vindu ved skyvedøra.
    S.quad('southGlass', win_s[0], win_s[1], win_s[2], win_s[3], -.075, 'Glass')
    if lod < 2:
        frame(S, 'southWindow', win_s[0], win_s[1], win_s[2], win_s[3])
        um, zm = (win_s[0] + win_s[1]) / 2, (win_s[2] + win_s[3]) / 2
        S.box('muntin', um - .015, um + .015, win_s[2], win_s[3], -.085, -.055, 'Wood')
        S.box('muntin', win_s[0], um - .015, zm - .015, zm + .015, -.085, -.055, 'Wood', 0)
        S.box('muntin', um + .015, win_s[1], zm - .015, zm + .015, -.085, -.055, 'Wood', 0)
    # Båsvinduer. Noen med lem, ett stort luftehull med lem i øst.
    if lod < 2:
        for F, h, lem in ((E, east_h[0], None), (E, east_h[2], 'closed'), (Wst, west_h[0], 'side'), (Wst, west_h[1], None), (NW, (1.5, 2.2, 1.2, 1.85), 'closed')):
            frame(F, 'stallWin', h[0], h[1], h[2], h[3])
        frame(E, 'vent', east_h[1][0], east_h[1][1], east_h[1][2], east_h[1][3], .09)
        plank_leaf(E, 'ventLeaf', east_h[1][1] + .1, east_h[1][1] + 1.25, 1.22, 2.38, .03, .06, k, False)
    # Lukkede lemmer, inne i veggen.
    E.box('stallLem', east_h[2][0], east_h[2][1], east_h[2][2], east_h[2][3], -t * .7, -t * .35, 'Board')
    NW.box('stallLem', 1.5, 2.2, 1.2, 1.85, -t * .7, -t * .35, 'Board')
    # Lavt ventilasjonsgitter.
    if lod == 0:
        for h in east_h[3:5]:
            for dz in (.03, .09):
                E.box('ventSlat', h[0], h[1], h[2] + dz, h[2] + dz + .025, -.09, -.03, 'Wood', 0)
    # Bakdør og høyluke bak, lukket.
    plank_leaf(N, 'rearDoor', rear[0], rear[1], 0, rear[3], -t * .7, -t * .3, k, True)
    plank_leaf(N, 'hayRear', hay2[0], hay2[1], hay2[2], hay2[3], -t * .7, -t * .3, k, False)
    if lod < 2:
        frame(N, 'rearFrame', rear[0], rear[1], .03, rear[3], .09, 'Wood', False)
        frame(N, 'hayRearFrame', hay2[0], hay2[1], hay2[2], hay2[3], .08, 'Wood')

    # Tak. Utstikk 0,6 på gavl, 0,45 på raft.
    ee, ry = .45, D2 + .6
    zr = RIDGE + LIFT
    ze = zr - PITCH * (MW + ee)
    xe0, xe1 = XW - ee, W2 + ee
    strip = [.84, 1.68, 3.4][lod]
    roof(k, 'roofEast', (xe1, -ry, ze), (xe1, ry, ze), (MX, ry, zr), (MX, -ry, zr), strip, .4)
    roof(k, 'roofWest', (xe0, ry, ze), (xe0, -ry, ze), (MX, -ry, zr), (MX, ry, zr), strip, .4)
    lz = LEAN_IN + .1
    xo = -W2 - .42
    zo = lz - LS * (XW - xo)
    roof(k, 'roofLean', (xo, ry - .25, zo), (xo, -ry + .25, zo), (XW + .02, -ry + .25, lz), (XW + .02, ry - .25, lz), strip, .55)
    if lod < 2:
        cap = .17
        for s in (-1, 1):
            k.face([(MX, -ry, zr + .03), (MX, ry, zr + .03), (MX + s * cap, ry, zr + .03 - cap * PITCH + .012), (MX + s * cap, -ry, zr + .03 - cap * PITCH + .012)],
                   'Tin', [(0, 0), (0, 7), (.1, 7), (.1, 0)], False, (1, .9, .78), (0, 0, 1))
    for y in (-ry, ry):
        k.beam('rake', (xe1, y, ze - .03), (MX, y, zr - .03), .05, .19, 'Wood', (0, 0, 1))
        k.beam('rake', (xe0, y, ze - .03), (MX, y, zr - .03), .05, .19, 'Wood', (0, 0, 1))
    for y in (-ry + .25, ry - .25):
        k.beam('rakeLean', (XW, y, lz - .02), (xo, y, zo - .02), .045, .18, 'Wood', (0, 0, 1))
    k.beam('eave', (xe1, -ry, ze - .1), (xe1, ry, ze - .1), .05, .2, 'Wood', (0, 0, 1))
    k.beam('eave', (xe0, -ry, ze - .1), (xe0, ry, ze - .1), .05, .2, 'Wood', (0, 0, 1))
    k.beam('eaveLean', (xo, -ry + .25, zo - .09), (xo, ry - .25, zo - .09), .045, .17, 'Wood', (0, 0, 1))

    # Lampe over døra, med trukket rør.
    lamp(k, S, MW, DOOR[2] + .62)
    if lod == 0:
        k.pipe('conduit', S.P(MW - .12, DOOR[2] + .48, .03), S.P(MW - .12, 4.15, .03), .014, 'Steel', 5)

    # --- Innvendig ---
    k.ambient = .95
    k.bx('floor', -W2 + .1, W2 - .1, -D2 + .1, D2 - .1, -.1, FLOOR, 'Dirt', skip=((2, -1),))
    k.ambient = .85
    k.bx('sill', DOOR[0], DOOR[1], -D2 - .12, -D2 + .12, WALL_BASE, FLOOR, 'Concrete', 0, skip=((2, -1),))
    k.ambient = .85
    for x in ROWS + [XW]:
        for y in POSTS:
            k.bx('post', x - .1, x + .1, y - .1, y + .1, 0, EAVE - .22, 'Wood')
    if lod < 2:
        for y in POSTS:
            k.bx('tie', XW, W2 - .1, y - .08, y + .08, EAVE - .22, EAVE, 'Wood', 0)
            k.beam('rafterMain', (XW, y, EAVE + .02), (MX, y, RIDGE - .02), .1, .18, 'Wood', (0, 0, 1))
            k.beam('rafterMain', (W2 - .1, y, EAVE + .02), (MX, y, RIDGE - .02), .1, .18, 'Wood', (0, 0, 1))
            k.beam('rafterLean', (XW, y, LEAN_IN - .05), (-W2 + .1, y, LEAN_OUT + .04), .08, .14, 'Wood', (0, 0, 1))
        for x in ROWS + [XW, W2 - .17]:
            k.bx('plate', x - .08, x + .08, -D2 + .1, D2 - .1, EAVE - .4, EAVE - .22, 'Wood', 1)
        k.bx('ridgeBeam', MX - .06, MX + .06, -D2 + .1, D2 - .1, RIDGE - .22, RIDGE - .03, 'Wood', 1)
        for frac in (.33, .66):
            z = EAVE + (RIDGE - EAVE) * frac + .03
            for s in (-1, 1):
                x = MX + s * MW * (1 - frac)
                k.bx('purlin', x - .05, x + .05, -D2 + .1, D2 - .1, z - .07, z + .05, 'Wood', 1)
    if lod == 0:
        for z in (1.1, 2.2):
            k.bx('girt', W2 - .22, W2 - .12, -D2 + .15, D2 - .15, z - .05, z + .05, 'Wood', 1)
            k.bx('girt', -W2 + .12, -W2 + .22, -D2 + .15, D2 - .15, z - .05, z + .05, 'Wood', 1)
            k.bx('girt', -W2 + .22, W2 - .22, D2 - .22, D2 - .12, z - .05, z + .05, 'Wood', 0) if z < 2 else None
    # Båser mot øst og i sideskipet. Rekkverk mot midtgangen, med åpne porter.
    for x0, x1 in ((ROWS[1] + .1, W2 - .12), (-W2 + .12, XW - .1)):
        for y in POSTS[1:3]:
            k.bx('stallWall', x0, x1, y - .03, y + .03, .03, 1.4, 'Board', 0)
    if lod == 0:
        for x in (ROWS[1], XW):
            for a, b in ((POSTS[0], POSTS[1]), (POSTS[1], POSTS[2])):
                for z in (.55, .95, 1.35):
                    k.bx('stallRail', x - .03, x + .03, a + .1, b - .95, z - .05, z + .05, 'Wood', 1)
    # Høyloft over bakre halvdel, med stige.
    k.bx('loftFloor', XW + .1, W2 - .12, LOFT_Y, D2 - .12, LOFT_Z, LOFT_Z + .06, 'Board', 0)
    if lod < 2:
        x = XW + .4
        while x < W2 - .2:
            k.bx('joist', x - .05, x + .05, LOFT_Y, D2 - .12, LOFT_Z - .18, LOFT_Z, 'Wood', 1)
            x += 1.25
        k.bx('loftEdge', XW + .1, W2 - .12, LOFT_Y - .08, LOFT_Y, LOFT_Z - .22, LOFT_Z + .06, 'Wood', 0)
    if lod == 0:
        # Tau med krok fra bindbjelken.
        k.pipe('rope', (.1, -1.15, EAVE - .22), (.1, -1.15, 1.9), .012, 'Wood', 5)
        k.pipe('hookIn', (.1, -1.15, 1.9), (.1, -1.15, 1.75), .012, 'Steel', 5)
        k.bx('loftRail', XW + .1, W2 - .12, LOFT_Y - .05, LOFT_Y, LOFT_Z + .9, LOFT_Z + 1.0, 'Wood', 0)
        for x in (XW + .2, -1.2, 1.0 - .4, 1.0 + .4, 4.8, W2 - .3):
            k.bx('loftPost', x - .04, x + .04, LOFT_Y - .05, LOFT_Y + .03, LOFT_Z + .06, LOFT_Z + .9, 'Wood')
        lx, ly = 1.0, LOFT_Y - .36
        for s in (-.24, .24):
            k.bx('ladderRail', lx + s - .03, lx + s + .03, ly - .03, ly + .04, .03, LOFT_Z + .55, 'Wood')
        z = .32
        while z < LOFT_Z + .3:
            k.bx('rung', lx - .21, lx + .21, ly - .03, ly + .03, z - .02, z + .02, 'Wood', 0)
            z += .3
    k.ambient = 1
    # Spilleren: ytterveggene med åpning i skyvedøra, stolper og båsvegger.
    k.solid(-W2, FREE[0], -D2, -D2 + .1)
    k.solid(FREE[1], W2, -D2, -D2 + .1)
    k.solid(-W2, W2, D2 - .1, D2)
    k.solid(-W2, -W2 + .1, -D2, D2)
    k.solid(W2 - .1, W2, -D2, D2)
    for x in ROWS + [XW]:
        for y in POSTS:
            k.solid(x - .1, x + .1, y - .1, y + .1)
    for x0, x1 in ((ROWS[1] + .1, W2 - .12), (-W2 + .12, XW - .1)):
        for y in POSTS[1:3]:
            k.solid(x0, x1, y - .03, y + .03)
    return finish(k)


def collision():
    """Konvekse UCX-deler. Litt innenfor det synlige der spilleren kan hekte seg fast."""
    out = []
    z = DOOR[2] + .2
    out.append(ucx_box(-W2, FREE[0], -D2, -D2 + .1, 0, z))
    out.append(ucx_box(FREE[1], W2, -D2, -D2 + .1, 0, z))
    out.append(ucx_box(-W2, W2, D2 - .1, D2, 0, z))
    out.append(ucx_box(-W2, -W2 + .1, -D2, D2, 0, LEAN_OUT))
    out.append(ucx_box(W2 - .1, W2, -D2, D2, 0, EAVE))
    for x in ROWS + [XW]:
        for y in POSTS:
            out.append(ucx_box(x - .1, x + .1, y - .1, y + .1, 0, EAVE - .22))
    for x0, x1 in ((ROWS[1] + .1, W2 - .12), (-W2 + .12, XW - .1)):
        for y in POSTS[1:3]:
            out.append(ucx_box(x0, x1, y - .03, y + .03, 0, 1.4))
    out.append(ucx_box(XW + .1, W2 - .12, LOFT_Y, D2 - .12, LOFT_Z, LOFT_Z + .06))
    # Taket som lukkede kiler, så stråler stopper der det ser ut til å være tak.
    ry = D2 + .6
    zr = RIDGE + LIFT + .03
    ze = EAVE + LIFT - PITCH * .45
    xe0, xe1 = XW - .45, W2 + .45
    out.append([(xe0, -ry, ze), (xe1, -ry, ze), (MX, -ry, zr), (xe0, ry, ze), (xe1, ry, ze), (MX, ry, zr)])
    xo = -W2 - .42
    out.append([(XW, -ry + .25, LEAN_IN - .2), (xo, -ry + .25, LEAN_OUT - .1), (XW, -ry + .25, LEAN_IN + .1),
                (XW, ry - .25, LEAN_IN - .2), (xo, ry - .25, LEAN_OUT - .1), (XW, ry - .25, LEAN_IN + .1)])
    return out
