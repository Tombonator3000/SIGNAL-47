"""Cane/spiny cholla: en treaktig stamme nede, leddete armer som deler seg med sideledd, og
piggete silhuett. Bygd etter referanse 08 (Spiny Cholla, Sumner Lake), men glissere.

Hvert ledd er et eget rør som knipes inn i skjøten. I LOD0 får hvert ledd to kryssede piggkort (alpha).
"""
import math
import random

from dpkit import Kit, add, mul, norm, polar, sub, lerp, length, basis, tint

VAR = {
    # stems: stammer fra samme rot. side: sjanse for krans per ledd. limbs: tørre, knekte grener.
    'a': dict(seed=11, height=1.84, stems=4, side=.92, lean=(.26, .62), dead=.05, fruit=9, cap=88, limbs=1),
    'b': dict(seed=47, height=1.58, stems=4, side=.8, lean=(.34, .78), dead=.12, fruit=5, cap=78, limbs=2),
}
SWAY = 0.1  # Stiv plante: bare tuppene rører seg litt.


class Joint:
    __slots__ = ('a', 'b', 'r', 'gen', 'parent', 'order', 'dead', 'wood', 'kids')

    def __init__(self, a, b, r, gen, parent, order, dead=False, wood=False):
        self.a, self.b, self.r, self.gen, self.parent, self.order = a, b, r, gen, parent, order
        self.dead, self.wood = dead, wood
        self.kids = []


def skeleton(cfg):
    r = random.Random(cfg['seed'])
    out = []

    def add_joint(a, d, ln, rad, gen, parent, wood=False, dead=False):
        j = Joint(a, add(a, mul(d, ln)), rad, gen, parent, len(out), dead=dead, wood=wood)
        out.append(j)
        if parent:
            parent.kids.append(j)
        return j

    def side_chain(parent, n, az):
        # Sideledd: 40 til 65 grader ut fra stammen og så opp, korte og tykke.
        axis = norm(sub(parent.b, parent.a))
        d = norm(add(mul(axis, r.uniform(.55, 1.0)), polar(az, 1.5)))
        cur, a = parent, parent.b
        for k in range(n):
            ln = r.uniform(.07, .14) * (1 - .1 * k)
            j = add_joint(a, d, ln, r.uniform(.029, .036), 2, cur, dead=r.random() < cfg['dead'] * .5)
            cur, a = j, j.b
            d = norm(add(d, (r.uniform(-.14, .14), r.uniform(-.14, .14), r.uniform(.55, .9))))
        return n

    def whorl(parent, height_frac, quota):
        # Krans: to til fire ledd rundt samme node. Nesten ingenting i nederste tredjedel.
        if height_frac < .28:
            return 0
        n = r.choice((2, 2, 3, 3, 4)) if height_frac > .45 else r.choice((1, 2))
        az0 = r.uniform(0, math.tau)
        used = 0
        for i in range(n):
            if used >= quota:
                break
            used += side_chain(parent, min(quota - used, r.choice((1, 1, 2))), az0 + i * math.tau / n + r.uniform(-.5, .5))
        return used

    def stem(parent, a, d, top, quota, fork=True):
        # Stamme: nesten rett, nesten lik tykkelse helt opp, til den når sin egen topphøyde. Sideleddene
        # fordeles etter høyden, så toppen ikke står naken. Én gaffel gir kronen flere armer.
        cur, used, k, sides = parent, 0, 0, 0
        forked = not fork
        est = top / .16
        while (cur is parent or cur.b[2] < top) and k < 18:
            wood = k == 0 and fork
            rad = .042 if wood else r.uniform(.036, .043) * (1 - .15 * cur.b[2] / top)
            ln = r.uniform(.17, .24) if wood else r.uniform(.12, .2)
            cur = add_joint(a, d, ln, rad, 1, cur, wood=wood)
            used += 1
            k += 1
            a = cur.b
            d = norm(add(d, (r.uniform(-.07, .07), r.uniform(-.07, .07), r.uniform(.09, .2))))
            frac = cur.b[2] / top
            if not forked and frac > .36:
                forked = True
                az = r.uniform(0, math.tau)
                nd = norm(add(d, mul(polar(az, 1.5), .7)))
                used += stem(cur, a, nd, top * r.uniform(.78, .98), int((quota - used) * .4), False)
            allowed = int(max(0.0, quota - est) * frac ** .8) - sides
            if not wood and allowed > 0 and r.random() < cfg['side']:
                n = whorl(cur, frac, allowed)
                sides += n
                used += n
        # Tuppen: null til tre korte ledd opp og ut, ingen T-gaffel.
        az0 = r.uniform(0, math.tau)
        n = r.choice((0, 1, 1, 2, 3))
        for i in range(n):
            axis = norm(sub(cur.b, cur.a))
            dt = norm(add(mul(axis, 1.1), polar(az0 + i * math.tau / n, 1.5)))
            add_joint(cur.b, dt, r.uniform(.08, .14), r.uniform(.029, .035), 2, cur)
        return used + n

    # Kort, mørk stamme nede. Roten litt under bakken.
    root = (0.0, 0.0, -.06)
    trunk_top = (r.uniform(-.02, .02), r.uniform(-.02, .02), r.uniform(.14, .22))
    trunk = add_joint(root, norm(sub(trunk_top, root)), length(sub(trunk_top, root)), .062, 0, None, wood=True)
    H = cfg['height']
    quota = (cfg['cap'] - 1) // cfg['stems']
    for m in range(cfg['stems']):
        az = m * 2.39996 + r.uniform(-.3, .3)
        start = lerp(trunk.a, trunk.b, r.uniform(.45, 1.0))
        # Yttersiden lener mest, midten står nesten rett. Ulik topphøyde: 70 til 100 prosent.
        el = r.uniform(*cfg['lean'])
        top = H * (1.0 if m == 0 else r.uniform(.56, .94))
        stem(trunk, start, polar(az, el), top, quota)
    # Tørre, knekte grener som ligger skrått ut lavt nede, slik referansen viser nederst til høyre.
    for m in range(cfg['limbs']):
        cur, a = trunk, lerp(trunk.a, trunk.b, r.uniform(.6, 1.0))
        d = polar(r.uniform(0, math.tau), r.uniform(.85, 1.2))
        for k in range(r.randint(2, 3)):
            cur = add_joint(a, d, r.uniform(.12, .19), .027, 1, cur, dead=True)
            a = cur.b
            d = norm(add(d, (r.uniform(-.4, .4), r.uniform(-.4, .4), r.uniform(-.25, .2))))
    # Akkurat bestilt høyde.
    f = H / max(j.b[2] for j in out)
    for j in out:
        j.a = (j.a[0] * f, j.a[1] * f, j.a[2] * f if j.a[2] > 0 else j.a[2])
        j.b = (j.b[0] * f, j.b[1] * f, j.b[2] * f)
    return out, r


def sway_at(p, h):
    return SWAY * max(0.0, p[2] / h) ** 1.4


def build(variant, lod):
    cfg = VAR[variant]
    joints, r = skeleton(cfg)
    h = cfg['height']
    k = Kit('cholla')
    skin = (.92, .96, .94)
    if lod == 2:
        return _lod2(k, joints, h)
    sides = 5 if lod == 0 else 4
    # LOD1: samme ledd med færre sider og uten piggkort; tykkere rør bevarer dekningen.
    for j in joints:
        if j.wood:
            mid = lerp(j.a, j.b, .5)
            sway = [sway_at(j.a, h), sway_at(mid, h), sway_at(j.b, h)]
            top = j.r * .82
            rads = [j.r * 1.2, j.r, top]
            rr = random.Random(j.order * 7 + 3)
            bump = [[rr.uniform(.82, 1.14) for _ in range(sides + 1)] for _ in range(3)]
            k.tube([j.a, mid, j.b], rads, sides + 1 if lod == 0 else 4, 'bark', sway, tint(skin, .95),
                   v0=rr.random(), vscale=.8, rough=lambda i, s: bump[i][s % sides], cap=True, tip_len=j.r * .4)
            continue
        v0 = random.Random(j.order * 13 + 1).random() * .3
        reg = 'dry' if j.dead else 'skin'
        vscale = 3.1  # 32 cm per atlashøyde
        mid2 = lerp(j.a, j.b, .62)  # Bredest mot enden: kølle, ikke pølse.
        thick = 1.0 if lod == 0 else 1.2
        col = tint(skin, .86 + .2 * (j.b[2] / h) * (0.6 + .4 * (j.gen == 2))) if not j.dead else (.6, .58, .56)
        terminal = not j.kids
        rr = random.Random(j.order * 5 + 2)
        bump = [[rr.uniform(.92, 1.1) for _ in range(sides + 1)] for _ in range(3)]
        if lod == 0:
            # Klemt inn i begge ender, litt tykkere på midten: pølseleddet.
            # Hvert ledd er lukket i enden, så skjøten aldri viser et hull. Knipingen er igjen i omrisset.
            start = j.r * .64
            jsides = 5 if j.gen == 1 else 4
            k.tube([j.a, mid2, j.b], [start * thick, j.r * 1.12 * thick, j.r * .74 * thick], jsides, reg,
                   [sway_at(j.a, h), sway_at(mid2, h), sway_at(j.b, h)], col, v0=v0, vscale=vscale,
                   cap=True, tip_len=j.r * (.7 if terminal else .25), rough=lambda i, s: bump[i][s % jsides])
            _spines(k, j, h, rr, 1)
        else:
            k.tube([j.a, j.b], [j.r * .95 * thick, j.r * thick], 3, reg, [sway_at(j.a, h), sway_at(j.b, h)], col,
                   v0=v0, vscale=vscale, flat=True)
    # Tørre gule frukter på noen endeledd, bare nær.
    if lod == 0:
        ends = [j for j in joints if not j.kids and not j.wood]
        r.shuffle(ends)
        for j in ends[:cfg['fruit']]:
            at = add(j.b, (0, 0, .02))
            k.bead(at, .019, .011, 'fruit', sway_at(at, h), (.72, .74, .6), az=j.order)
    return k


def _spines(k, j, h, rr, cards):
    """Kort med pigger. Hjørnene ved roten og toppen bruker leddets vindtall, så kortet følger leddet."""
    d = sub(j.b, j.a)
    ln = length(d)
    d = norm(d)
    u, w = basis(d)
    wid = 2 * (j.r + .036)
    # Bredere enn leddet: piggene stikker ut 3,5 cm.
    a = rr.uniform(0, math.tau)
    s0, s1 = sway_at(j.a, h), sway_at(j.b, h)
    for i in range(cards):
        t = a + i * math.pi / 2
        side = add(mul(u, math.cos(t)), mul(w, math.sin(t)))
        # Opp og ut: piggene skal være lyse også på skyggesiden, ikke sorte streker.
        nrm = norm((j.b[0], j.b[1], .9))
        k.card(j.a, d, side, wid, ln, 'spines', s0, s1, nrm, (1, 1, 1))


def _lod2(k, joints, h):
    """Kjeder av ledd blir ett prisme hver, tykkere så de ikke forsvinner på avstand. Under 200 trekanter.

    Rekkefølge: stammen, de treaktige leddene, armene etter lengde, sideleddene etter lengde.
    En kjede tas bare med når leddet den springer ut fra er med, så ingenting svever.
    """
    chains = []
    for j in joints:
        if j.parent is None or j.parent.wood != j.wood or len(j.parent.kids) > 1 or j.wood:
            run, p = [j], j
            while p.kids and j.parent is not None:
                n = max(p.kids, key=lambda c: c.b[2])
                if n.wood != j.wood:
                    break
                p = n
                run.append(p)
            chains.append((length(sub(p.b, j.a)), j, p, run))
    rank = lambda c: (0 if c[1].gen == 0 else 1 if c[1].wood else 2 if c[1].gen == 1 else 3, -c[0])
    chains.sort(key=rank)
    covered, chosen = set(), []
    changed = True
    while changed and len(chosen) * 7 + 7 <= 190:
        changed = False
        for c in chains:
            if c in chosen or len(chosen) * 7 + 7 > 190:
                continue
            if c[1].parent is None or c[1].parent in covered:
                chosen.append(c)
                covered.update(c[3])
                changed = True
                break  # Start på nytt, så viktigste tilgjengelige kjede alltid går først.
    for ln, a, b, run in chosen:
        thick = (1.5 if a.gen == 1 else 1.35) if not a.wood else 1.1
        reg = 'bark' if a.wood else 'skin'
        c = (.9, .94, .9) if not a.wood else (.86, .86, .86)
        k.tube([a.a, b.b], [a.r * thick, b.r * thick * .8], 3, reg, [sway_at(a.a, h), sway_at(b.b, h)], c, vscale=1.0, flat=True)
    return k
