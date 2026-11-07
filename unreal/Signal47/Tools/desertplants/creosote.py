"""Kreosot (Larrea tridentata): åpen vase fra én rotkrone, mange tynne mørke grå-brune stengler med luft
mellom, og små olivengrønne blad samlet ytterst på greinene. Referanse 06 (forgrunnsbusken), 01 og 10.
Nederst er planten bar. Stengler og grove greiner er ekte rør, de ytterste kvistene og bladene er alpha-kort.

Vind i UV1.x vokser med avstand langs planten; et barn starter med forelderens tall i festepunktet, og kortets to
rothjørner har festepunktets tall, så ingenting løsner i FLORA_WPO. Høyden er målt ved skala 1. Spillet ganger med
sin egen størrelse (S47Flora.cpp), og kapper høyden ved 1,25 ganger: A blir aldri over 2 m.
"""
import math
import random

from dpkit import Kit, add, cross, mul, norm, polar, sub, lerp, basis, tint

VAR = {
    # stems: stengler fra roten. el: hvor utoverliggende (fra rett opp). arc: hvor mye de bøyer utover.
    # gens: barn per stengel og per grein. cards: kort på LOD0. sparse: andel kort med nesten bare kvist.
    'a': dict(seed=31, height=1.6, reach=.94, stems=13, el=(.1, .62), arc=.11, gens=(3, 2), l0=.98, cards=465, sparse=.08),
    'b': dict(seed=47, height=1.38, reach=1.06, stems=12, el=(.32, .95), arc=.15, gens=(3, 2), l0=.98, cards=470, sparse=.08),
    'c': dict(seed=63, height=1.1, reach=.76, stems=9, el=(.16, .82), arc=.12, gens=(3, 2), l0=.74, cards=430, sparse=.14),
}
SWAY_MAX = .85
LOD_CARDS = (None, 270, 64)  # LOD1, LOD2: resten av budsjettet går til rørene.


class Seg:
    __slots__ = ('path', 'r0', 'r1', 'gen', 's0', 's1', 'kids', 'order', 'parent')

    def __init__(self, path, r0, r1, gen, s0, s1, order, parent):
        self.path, self.r0, self.r1, self.gen, self.s0, self.s1, self.order = path, r0, r1, gen, s0, s1, order
        self.parent = parent
        self.kids = []


def _on_path(pts, t):
    x = t * (len(pts) - 1)
    i = min(int(x), len(pts) - 2)
    return lerp(pts[i], pts[i + 1], x - i)


def skeleton(cfg):
    r = random.Random(cfg['seed'])
    segs = []
    tree_len = cfg['l0'] * 1.9

    def grow(a, d, ln, rad, gen, s0, dist, parent):
        n = 4 if gen == 0 else (3 if gen == 1 else 1)
        pts = [a]
        cur = a
        step = ln / n
        out = norm((d[0], d[1], 0.0))
        u, w = basis(d)
        sign = r.choice((-1, 1))
        for _ in range(n):
            # Knekk i hver node, og hele tiden litt utover og så opp igjen: buede, kantete stengler.
            t = r.uniform(0, math.tau)
            kink = (.16 if gen == 0 else .3) * sign
            sign = -sign
            d = norm(add(d, add(add(mul(u, math.cos(t) * kink), mul(w, math.sin(t) * kink)), mul(out, cfg['arc'] * (1.2 if gen == 0 else .5)))))
            if gen and d[2] < .18:
                d = norm((d[0], d[1], .25))
            cur = add(cur, mul(d, step))
            pts.append(cur)
        dist2 = dist + ln
        s1 = SWAY_MAX * min(1.0, dist2 / tree_len) ** 1.25
        s = Seg(pts, rad, rad * .62, gen, s0, s1, len(segs), parent)
        segs.append(s)
        if parent:
            parent.kids.append(s)
        if gen >= len(cfg['gens']):
            return s
        kids = cfg['gens'][gen] + r.randint(0, 1)
        for i in range(kids):
            # Sidegreinene sitter fra midt på stengelen og ut: det nederste av planten er bart.
            at_t = (.38 if gen == 0 else .25) + (.62 if gen == 0 else .75) * (i + r.uniform(.2, .8)) / kids
            if i == kids - 1:
                at_t = 1.0
            at = _on_path(pts, at_t)
            local = s0 + (s1 - s0) * at_t
            az = r.uniform(0, math.tau)
            nd = norm(add(d, mul(polar(az, 1.3), .62 if i < kids - 1 else .28)))
            if nd[2] < .1:
                nd = norm((nd[0], nd[1], .2))
            grow(at, nd, ln * (r.uniform(.5, .72) if gen == 0 else r.uniform(.42, .62)), rad * .62, gen + 1, local, dist + ln * at_t, s)
        return s

    n = cfg['stems']
    for i in range(n):
        az = i * math.tau / n + r.uniform(-.3, .3)
        el = r.uniform(*cfg['el'])
        start = (math.cos(az) * r.uniform(.01, .055), math.sin(az) * r.uniform(.01, .055), -.05)
        # Noen få grove stengler, resten tynne.
        rad = r.uniform(.013, .017) if i % 4 == 0 else r.uniform(.0075, .011)
        grow(start, polar(az, el), cfg['l0'] * r.uniform(.68, 1.1), rad, 0, 0.0, 0.0, None)
    # Skaler til bestilt høyde og bredde. Rørtykkelser beholdes. Kortene går litt over greinene.
    top = max(max(p[2] for p in s.path) for s in segs)
    f = cfg['height'] * .9 / top
    fx = cfg['reach'] * .9 / max((p[0] * p[0] + p[1] * p[1]) ** .5 for s in segs for p in s.path)
    for s in segs:
        s.path = [(p[0] * fx, p[1] * fx, p[2] * f if p[2] > 0 else p[2]) for p in s.path]
        s.path = [p if (s.gen == 0 and i == 0) else (p[0], p[1], max(p[2], .015)) for i, p in enumerate(s.path)]
    return segs


def kept(segs, lod):
    """Greinene på et detaljnivå. En grein tas bare med hvis forelderen er med, så ingenting svever."""
    if lod == 0:
        return list(segs)
    if lod == 1:
        keep, out = set(), []
        for s in segs:  # Foreldre står foran barna.
            if s.gen == 0 or (s.gen == 1 and s.order % 2 == 0 and s.parent in keep):
                keep.add(s)
                out.append(s)
        return out
    # LOD2: de åtte lengste stenglene, som rør med ett ledd.
    return sorted((s for s in segs if s.gen == 0), key=lambda s: -s.path[-1][2])[:8]


def build(variant, lod):
    cfg = VAR[variant]
    segs = skeleton(cfg)
    k = Kit('creosote')
    H = cfg['height']
    use = kept(segs, lod)
    for s in use:
        if lod == 0:
            path = s.path
            sides = 4 if s.gen == 0 else 3
        elif lod == 1:
            path = [s.path[0], s.path[len(s.path) // 2], s.path[-1]] if s.gen == 0 else [s.path[0], s.path[-1]]
            sides = 3
        else:
            path = [s.path[0], s.path[-1]]
            sides = 3
        n = len(path)
        thick = (1.0, 1.35, 1.9)[lod]
        rads = [(s.r0 + (s.r1 - s.r0) * (i / (n - 1))) * thick for i in range(n)]
        sway = [s.s0 + (s.s1 - s.s0) * (i / (n - 1)) for i in range(n)]
        rr = random.Random(s.order * 3 + 1)
        cols = [tint((1.0, .98, .95), .72 + .28 * min(1.0, max(0.0, p[2]) / (H * .7))) for p in path]
        k.tube(path, rads, sides, 'bark' if s.gen < 2 else 'twig', sway, cols, v0=rr.random() * .3, vscale=.42, cap=False, flat=lod > 0)
    _cards(k, cfg, segs, H, lod)
    return k


def _cards(k, cfg, segs, H, lod):
    """Korte blad- og kvistkort ytterst på greinene. Roten sitter på en grein, og kortet peker utover og opp."""
    r2 = random.Random(cfg['seed'] * 13 + 7)
    outer = [s for s in segs if s.gen >= 1]
    tips = [s for s in segs if not s.kids]
    n = cfg['cards'] if lod == 0 else LOD_CARDS[lod]
    grow = (1.0, 2.2, 4.4)[lod]
    for i in range(n):
        # Først ett kort i hver greinende, resten fordelt på den ytre delen av greinene.
        if i < len(tips):
            s, t = tips[i], 1.0
        else:
            s = r2.choice(outer)
            t = r2.uniform(.45, 1.0)
        at = _on_path(s.path, t)
        s_at = s.s0 + (s.s1 - s.s0) * t
        tilt, turn, hh, tone, pick, side = r2.random(), r2.random(), r2.random(), r2.random(), r2.random(), r2.random()
        high = at[2] / H
        if lod == 2:
            reg = ('far_a', 'far_b', 'far_c')[int(pick * 3)]
        elif lod == 1:
            reg = ('far_a', 'far_b', 'leaf_a', 'leaf_b')[int(pick * 4)]
        elif high < .36 or pick < cfg['sparse']:
            reg = 'bare_a'
        elif t > .85 and pick < .45:
            reg = ('tip_a', 'tip_b')[int(side * 2)]
        else:
            reg = ('leaf_a', 'leaf_b', 'leaf_c')[int(side * 3)]
        d = norm(sub(s.path[-1], s.path[0]))
        radial = norm((at[0], at[1], .02))
        # Godt opp, utover litt: et kort som peker mot øyet i ståhøyde, blir en strek av utstrakte blad.
        along = norm(add(add(mul(d, .6), mul(radial, .2 + tilt * .35)), ((turn - .5) * .5, (hh - .5) * .5, .4 + tone * .4)))
        # Bredden ligger rundt plantens omkrets, så flaten vender utover og ikke mot naboen; litt skjevt, så ikke alle er like.
        round_ = norm(cross((0.0, 0.0, 1.0), radial))
        a = (turn - .5) * 1.5
        wide = norm(add(mul(round_, math.cos(a)), mul(norm(cross(along, round_)), math.sin(a))))
        h = (.13 + hh * .11) * grow
        # Kortet går ikke over bestilt høyde og bredde (spec.py).
        if along[2] > .05:
            h = max(.08, min(h, (H - at[2]) / along[2]))
        room = cfg['reach'] - (at[0] * at[0] + at[1] * at[1]) ** .5
        out = (along[0] * along[0] + along[1] * along[1]) ** .5
        if out > .05:
            h = max(.08, min(h, max(.0, room) / out))
        wid = h * .75
        nrm = norm(add(radial, (0, 0, 1.4)))
        # Lineær tone: mørkere inne og lavt, lysere og litt gulere ytterst; noen kvister mer gule.
        lit = .72 + .28 * min(1.0, high / .8)
        warm = (.9 + tone * .14, .92 + tone * .08, .8 + (1 - tone) * .12)
        col = tint(warm, lit)
        k.card(at, along, wide, wid, h, reg, s_at, min(1.0, s_at + .12), nrm, col, tint(col, 1.08))
