"""Soaptree yucca: stiv tue av smale blad fra en kort stamme, skjegg av døde blad under, og i to av
variantene blomsterstengel 2-3 m, med blomster (referanse 07) eller med fjorårets tomme belger.

Ingen alpha: alt er ekte blad, derfor er FLORA_WPO trygg. Vind i UV1.x: 0 ved roten, opptil 1 i bladtuppen.
"""
import math
import random

from dpkit import Kit, add, mul, norm, polar, lerp, cross, tint

VAR = {
    # height: toppen av tuen, trunk: stammehøyde (skjegget henger under), n: levende blad i LOD0, dead: døde blad i LOD0.
    'a': dict(seed=21, height=.94, trunk=.17, n=110, dead=156, stalk=None),
    'b': dict(seed=29, height=.68, trunk=.13, n=104, dead=146, stalk=None),
    'c': dict(seed=37, height=.88, trunk=.16, n=84, dead=100, stalk=(2.72, 'bloom')),
    'd': dict(seed=43, height=.76, trunk=.15, n=92, dead=130, stalk=(2.34, 'pods')),
}
BLADE_TIP = .9  # Vind i tuppen av de lengste bladene.


def blade(k, root, d, ln, wid, bend, reg, stations, fold, sway_tip, col_root, col_tip, s0=.02):
    """Stivt blad: smalt ved roten, bredest under midten, spiss tupp. Bøyes ut mot tuppen.

    stations: ts (0..1) før tuppen. fold: 3 hjørner tvers over med løftet midtribbe, ellers 2.
    Normalene peker opp og ut, så begge sider tar lyset likt som ellers i flora.ts.
    """
    out = norm((d[0], d[1], 0.0)) if d[0] * d[0] + d[1] * d[1] > 1e-6 else (1.0, 0.0, 0.0)
    side = norm(cross(d, out))
    if side == (0.0, 0.0, 1.0):
        side = (-out[1], out[0], 0.0)
    nrm = norm(add((0, 0, 1), mul(out, .6)))
    fn = cross(side, d)
    wob = (root[0] * 37 + root[1] * 53 + ln * 11) % .04 - .02  # Lett S-form, ulik per blad.

    def point(t):
        p = add(root, mul(d, ln * t))
        p = add(p, mul(out, bend * ln * t * t))
        p = add(p, mul(side, wob * ln * math.sin(t * math.pi * 1.6)))
        # Bladet legger seg på bakken i stedet for å gå gjennom den.
        return (p[0], p[1], max(-.015, p[2] - bend * bend * ln * .9 * t * t))

    def sw(t):
        return s0 + (sway_tip - s0) * t ** 1.5

    prev = None
    for t in stations:
        # Bredde: 30 % ved roten, full ved 35 %, smalner mot tuppen.
        w = wid * (.34 + .66 * min(1.0, t / .35)) * (1.0 if t < .55 else (1.0 - (t - .55) / .45 * .78))
        c = lerp(col_root, col_tip, t)
        p = point(t)
        row = []
        cols = 3 if fold else 2
        for i in range(cols):
            f = (i / (cols - 1) - .5) * 2
            q = add(p, mul(side, f * w / 2))
            if fold and i == 1:
                q = add(q, mul(fn, w * .22))
            row.append(k.vert(q, nrm, k.region(reg, i / (cols - 1) * .92 + .04, t), sw(t), c))
        if prev:
            for i in range(cols - 1):
                k.quad(prev[i], prev[i + 1], row[i + 1], row[i])
        prev = row
    tip = k.vert(point(1.0), nrm, k.region(reg, .5, .998), sw(1.0), col_tip)
    for i in range(len(prev) - 1):
        k.tri(prev[i], prev[i + 1], tip)


def build(variant, lod):
    cfg = VAR[variant]
    r = random.Random(cfg['seed'])
    k = Kit('yucca')
    H, tr = cfg['height'], cfg['trunk']
    # Lavere detalj beholder silhuetten med færre og bredere blad. Dekningen holdes omtrent lik.
    n = cfg['n'] if lod == 0 else (84 if lod == 1 else 22)
    nd = cfg['dead'] if lod == 0 else (42 if lod == 1 else 15)
    wid = .027 if lod == 0 else (.042 if lod == 1 else .11)
    deadwid = .028 if lod == 0 else (.06 if lod == 1 else .12)
    stations = (0.0, .3, .6, .86) if lod == 0 else ((0.0, .4, .78) if lod == 1 else (0.0, .55))
    deadst = (0.0, .4, .78) if lod == 0 else ((0.0, .6) if lod == 1 else (0.0,))
    fold = lod == 0
    crown = (r.uniform(-.01, .01), r.uniform(-.01, .01), tr)
    # Kort stamme under tuen.
    k.tube([(0, 0, -.05), crown], [.085, .068], 5 if lod == 0 else 3, 'trunk', [0.0, .02], (.82, .8, .78), vscale=1.0)
    # Fontene: de fleste bladene nesten rett opp, bare de ytterste ned mot 15-25 grader over vannrett.
    upright = H - tr
    # Hver plante står litt skjevt, så omrisset ikke blir en perfekt stjerne.
    skew = (r.uniform(-.1, .1), r.uniform(-.1, .1), 0.0)
    for i in range(n):
        f = (i + r.random()) / n
        el = 0.03 + f ** 2.8 * 1.14 + r.uniform(-.06, .06)
        az = i * 2.39996 + r.uniform(-.3, .3)
        d = norm(add(polar(az, el), skew))
        ln = upright * r.uniform(.62, 1.0) * (1.0 - .3 * f * f) / max(.62, math.cos(min(1.2, el)))
        ln = min(ln, .78)
        # Hvert tjuende blad er knekt.
        bend = .03 + .4 * max(0.0, el - .5) + r.uniform(0, .06) + (.5 if r.random() < .08 else 0)
        # Bredden holdes i spec: bladene lengst ute når høyst 0,8 m fra midten.
        ln = min(ln, .68 / max(.3, math.sin(min(1.57, el)) + bend * .9))
        c = r.random()
        base = (.74, .76, .62)
        tip = (.98, .98, .92) if c > .15 else (1.0, .94, .8)
        lift = 1 - .22 * f
        s_tip = BLADE_TIP * min(1.0, ln / (upright * 1.1)) * (0.75 + .25 * r.random())
        blade(k, add(crown, (r.uniform(-.03, .03), r.uniform(-.03, .03), r.uniform(-.015, .015))), d, ln, wid * r.uniform(.8, 1.1), bend,
              'green_a' if r.random() < .55 else 'green_b', stations, fold, s_tip, tint(base, lift), tint(tip, .72 + .3 * (1 - f)))
    # Skjegget: en krage av døde blad rundt stammen som bøyer ut og ned til bakken, tørre og lyse,
    # noen mørke og gamle. Festet høyt nok til at det leses som skjegg under den grønne tuen.
    for i in range(nd):
        az = i * 2.39996 * 1.7 + r.uniform(-.5, .5)
        kind = r.random()
        # Tre av fire henger tett langs stammen som et skjørt, resten står ut og legger seg på bakken.
        hang = kind < .64
        el = r.uniform(2.6, 3.02) if hang else r.uniform(1.6, 2.15)
        d = polar(az, el)
        z0 = r.uniform(tr * .3, tr + .04) if hang else r.uniform(tr * .4, tr + .02)
        # Festet på utsiden av stammen, ellers blir bladene borte inni den.
        rad = r.uniform(.072, .1)
        at = (crown[0] + math.cos(az) * rad, crown[1] + math.sin(az) * rad, z0)
        ln = (at[2] + .02) / max(.2, -d[2]) + r.uniform(0, .07) if hang else r.uniform(.24, .44)
        ln = max(.08, min(ln, .5 / max(.3, (d[0] * d[0] + d[1] * d[1]) ** .5)))
        col = tint((.92, .88, .8), r.uniform(.7, 1.1))
        blade(k, at, d, ln, deadwid * r.uniform(.8, 1.25), r.uniform(.02, .12) if hang else r.uniform(.15, .35),
              'dead_a' if r.random() < .6 else 'dead_b', deadst, False, r.uniform(.04, .08), col, tint(col, .88), s0=.015)
        # og en loslitt overgang rett under tuen, så stammen ikke vises.
    for i in range(nd // 3):
        az = i * 2.39996 + r.uniform(-.5, .5)
        col = tint((.86, .8, .68), r.uniform(.75, 1.05))
        blade(k, add(crown, (0, 0, r.uniform(-.02, .05))), polar(az, r.uniform(1.2, 1.85)), r.uniform(.36, .62), deadwid, r.uniform(.25, .55),
              'dead_a' if r.random() < .6 else 'dead_b', deadst, False, r.uniform(.1, .22), col, tint(col, .9), s0=.02)
    if cfg['stalk']:
        _stalk(k, cfg, crown, r, lod)
    return k


def _stalk(k, cfg, crown, r, lod):
    height, mode = cfg['stalk']
    top = (crown[0] + r.uniform(-.1, .1), crown[1] + r.uniform(-.1, .1), height)
    # Stengelen står litt skjevt og bøyer lett.
    m1 = lerp(crown, top, .34)
    m2 = lerp(crown, top, .68)
    m1 = (m1[0] + r.uniform(-.03, .03), m1[1] + r.uniform(-.03, .03), m1[2])
    m2 = (m2[0] + r.uniform(-.04, .04), m2[1] + r.uniform(-.04, .04), m2[2])
    path = [crown, m1, m2, top]
    swz = lambda z: .03 + .65 * max(0.0, z / height) ** 1.25

    def along(t):
        if t < 1 / 3:
            return lerp(path[0], path[1], t * 3)
        if t < 2 / 3:
            return lerp(path[1], path[2], t * 3 - 1)
        return lerp(path[2], path[3], t * 3 - 2)

    rads = [.042, .036, .026, .01]
    k.tube(path, rads, 5 if lod == 0 else 3, 'stalk', [swz(p[2]) for p in path], (.95, .95, .92), vscale=.42, cap=lod == 0)
    if lod == 2:
        # Toppen som kolbe: tre tykke prismer gir klaseformen på avstand.
        c = (1.0, .98, .92) if mode == 'bloom' else (.8, .76, .68)
        reg = 'flower' if mode == 'bloom' else 'pod'
        # Smal, firesidig kolbe, ikke bredere enn klasen i LOD0.
        base, end = along(.6), along(.97)
        wide = .075 if mode == 'bloom' else .05
        k.tube([base, end], [wide, .018], 4, reg, [swz(base[2]), swz(end[2])], c, vscale=.5, cap=True)
        return
    # Skjell på stengelen, små og tørre.
    nb = 12 if lod == 0 else 0
    for i in range(nb):
        t = .1 + i * .042
        at = along(t)
        d = polar(i * 2.1, r.uniform(.7, 1.15))
        blade(k, at, d, .1 + r.uniform(0, .07), .018, r.uniform(0, .2), 'bract', (0.0, .5), False, swz(at[2]) + .02,
              (.9, .86, .74), (.98, .92, .74), s0=swz(at[2]))
    # Klasen: sidegreiner mot toppen, tettest på midten, blomster eller belger langs dem.
    branches = 22 if lod == 0 else 8
    per = (6 if mode == 'bloom' else 3) if lod == 0 else 2
    lo = .6 if mode == 'bloom' else .56
    for i in range(branches):
        u = i / max(1, branches - 1)
        t = lo + u * (.97 - lo)
        at = along(t)
        a = i * 2.39996 + r.uniform(-.7, .7)
        t = min(.98, t + r.uniform(-.02, .02))
        at = along(t)
        # Fjorårets stengel har mistet noen armer og en del belger: uregelmessig, ikke fiskebein.
        if mode == 'pods' and lod == 0 and r.random() < .22:
            continue
        # Spindel: bredest en tredjedel opp, smal mot tuppen.
        taper = math.sin(min(1.0, u ** .62) * math.pi) * .85 + .15
        ln = (.16 if mode == 'bloom' else .28) * taper * r.uniform(.6, 1.15)
        end = add(at, (math.cos(a) * ln * .85, math.sin(a) * ln * .85, ln * .5))
        s0, s1 = swz(at[2]), swz(end[2]) + .05
        k.tube([at, end], [.0085, .0045], 3, 'stalk', [s0, s1], (.92, .9, .78), v0=.5, vscale=.5)
        if mode == 'bloom':
            cols = [(1.0, .98, .88), (.98, .95, .8), (.94, .94, .76)]
        else:
            cols = [(1.0, 1.0, .96), (.9, .86, .78)]
        for j in range(per if mode == 'bloom' or lod else r.choice((0, 1, 2, 3, 4))):
            f = (j + .7) / per + r.uniform(-.05, .05)
            p = add(lerp(at, end, min(1.0, f)), (r.uniform(-.025, .025), r.uniform(-.025, .025), -.012))
            sw = s0 + (s1 - s0) * min(1.0, f)
            if mode == 'bloom':
                if t > .9 and r.random() < .7:
                    k.bead(p, .018, .03, 'bud', sw, r.choice(cols), az=r.uniform(0, 3))
                else:
                    _bell(k, p, r.uniform(.03, .04), r.uniform(.042, .054), sw, r.choice(cols), r.uniform(0, 3))
            else:
                k.bead(p, r.uniform(.016, .021), .04, 'pod', sw, r.choice(cols), az=r.uniform(0, 3))


def _bell(k, at, rad, tall, s, col, az):
    """Hengende klokke, seks trekanter: tre kronblad fra festet og ned, med flikene litt ut."""
    top = k.vert(at, (0, 0, 1), k.region('flower', .5, .98), s, col)
    ring, flare = [], []
    for i in range(3):
        t = az + i * math.tau / 3
        n = (math.cos(t), math.sin(t), .25)
        ring.append(k.vert(add(at, (n[0] * rad * .9, n[1] * rad * .9, -tall * .7)), n, k.region('flower', i / 3, .45), s, col))
        t2 = t + math.pi / 3
        n2 = (math.cos(t2), math.sin(t2), .1)
        flare.append(k.vert(add(at, (n2[0] * rad * 1.15, n2[1] * rad * 1.15, -tall)), n2, k.region('flower', (i + .5) / 3, .05), s, col))
    for i in range(3):
        k.tri(top, ring[i], ring[(i + 1) % 3])
        k.tri(ring[i], flare[i], ring[(i + 1) % 3])
