"""Honey mesquite i buskform: mange tynne, krokete greiner fra én rotkrone, grå-brune, med luft
mellom. Referanse 10 (bar) og 03 (sparsomt olivengrønt løv). Grove greiner er ekte rør, de tynneste
kvistene og løvet er alpha-kort.

Vind i UV1.x vokser med avstand langs treet, og et barn starter med forelderens tall i festepunktet.
Kortets to rothjørner har festepunktets vindtall, så kortet følger greinen i FLORA_WPO og løsner aldri.
"""
import math
import random

from dpkit import Kit, add, mul, norm, polar, sub, lerp, length, basis, tint

VAR = {
    # stems: greiner fra roten. leaf/bare/pod: andel av kortene. el: hvor utoverliggende greinene er.
    'a': dict(seed=5, height=2.28, stems=7, el=(.35, 1.12), gens=(3, 2, 1), leaf=.42, bare=.5, cards=230, l0=.7, reach=1.6, pins=6),
    'b': dict(seed=17, height=1.36, stems=7, el=(.5, 1.22), gens=(3, 2, 1), leaf=.16, bare=.68, cards=170, l0=.6, reach=1.35, pins=6),
}
SWAY_MAX = .85


class Seg:
    __slots__ = ('path', 'r0', 'r1', 'gen', 's0', 's1', 'kids', 'order', 'parent')

    def __init__(self, path, r0, r1, gen, s0, s1, order, parent):
        self.path, self.r0, self.r1, self.gen, self.s0, self.s1, self.order = path, r0, r1, gen, s0, s1, order
        self.parent = parent
        self.kids = []


def crooked(r, a, d, ln, zig, n, lift=.0):
    """Krokete løp med knekk i hver node: mesquitens sikksakk."""
    pts = [a]
    step = ln / n
    u, w = basis(d)
    sign = r.choice((-1, 1))
    cur = a
    for i in range(n):
        t = r.uniform(0, math.tau)
        ofs = add(mul(u, math.cos(t) * zig * sign), mul(w, math.sin(t) * zig * sign))
        sign = -sign
        d = norm(add(d, add(ofs, (0, 0, lift))))
        cur = add(cur, mul(d, step))
        pts.append(cur)
    return pts, d


def skeleton(cfg):
    r = random.Random(cfg['seed'])
    segs = []
    tree_len = cfg['l0'] * 2.0

    def grow(a, d, ln, rad, gen, s0, dist, parent):
        n = 4 if gen == 0 else (3 if gen == 1 else 2)
        pts, dn = crooked(r, a, d, ln, (.38 if gen else .22) * (1 + gen * .2), n, lift=.05 if gen else .1)
        dist2 = dist + ln
        s1 = SWAY_MAX * min(1.0, dist2 / tree_len) ** 1.2
        s = Seg(pts, rad, rad * .66, gen, s0, s1, len(segs), parent)
        segs.append(s)
        if parent:
            parent.kids.append(s)
        if gen >= len(cfg['gens']):
            return s
        kids = cfg['gens'][gen] + r.randint(-1, 1)
        for i in range(max(1, kids)):
            # Forgrening helt nede fra 15-20 cm over bakken.
            t = .18 + .82 * (i + r.random()) / max(1, kids)
            if i == kids - 1:
                t = 1.0
            at = _on_path(pts, t)
            local = s0 + (s1 - s0) * t
            az = r.uniform(0, math.tau)
            nd = norm(add(dn, mul(polar(az, 1.25), .55 + gen * .12)))
            if nd[2] < .05:
                nd = norm((nd[0], nd[1], .15))
            grow(at, nd, ln * r.uniform(.62, .82), rad * .6, gen + 1, local, dist + ln * t, s)
        return s

    n = cfg['stems']
    for i in range(n):
        az = i * math.tau / n + r.uniform(-.35, .35)
        el = r.uniform(*cfg['el'])
        start = (math.cos(az) * r.uniform(.01, .05), math.sin(az) * r.uniform(.01, .05), -.05)
        # To eller tre tykke stammer, resten tynne. Én kan ligge nesten flatt.
        rad = r.uniform(.03, .042) if i % 3 == 0 else r.uniform(.016, .023)
        if i == n - 1:
            el = r.uniform(1.25, 1.42)
        grow(start, polar(az, el), cfg['l0'] * r.uniform(.72, 1.12), rad, 0, 0.0, 0.0, None)
    # Skaler til bestilt høyde, med roten igjen under bakken. Rørtykkelser beholdes.
    top = max(max(p[2] for p in s.path) for s in segs)
    f = cfg['height'] * .9 / top  # Kortene går litt over greinene.
    fx = min(f, cfg['reach'] / max((p[0] * p[0] + p[1] * p[1]) ** .5 for s in segs for p in s.path))
    for s in segs:
        # Roten står under bakken, alt annet over.
        s.path = [(p[0] * fx, p[1] * fx, p[2] * f if p[2] > 0 else p[2]) for p in s.path]
        s.path = [p if (s.gen == 0 and i == 0) else (p[0], p[1], max(p[2], .015)) for i, p in enumerate(s.path)]
    return segs, r


def _on_path(pts, t):
    x = t * (len(pts) - 1)
    i = min(int(x), len(pts) - 2)
    return lerp(pts[i], pts[i + 1], x - i)


def kept(segs, lod):
    """Greinene som er med på et detaljnivå. En grein tas bare med hvis forelderen er med, så ingenting svever."""
    if lod == 0:
        return list(segs)
    if lod == 1:
        keep = set()
        out = []
        for s in segs:  # Foreldre står foran barna i listen.
            if s.gen <= 1 or (s.order % 2 == 0 and s.parent in keep):
                keep.add(s)
                out.append(s)
        return out
    # LOD2: alle stammer, så de seks høyeste armene og den ytterste kvisten på dem. De når opp i kronen.
    out = [s for s in segs if s.gen == 0]
    arms = sorted((s for s in segs if s.gen == 1), key=lambda s: -s.path[-1][2])[:6]
    out += arms
    for a in arms:
        tips = [c for c in a.kids if c.gen == 2]
        if tips:
            out.append(max(tips, key=lambda c: c.path[-1][2]))
    return out


def build(variant, lod):
    cfg = VAR[variant]
    segs, r = skeleton(cfg)
    k = Kit('mesquite')
    H = cfg['height']
    use = kept(segs, lod)
    for s in use:
        path = s.path if lod == 0 or (lod == 1 and s.gen == 0) else [s.path[0], s.path[-1]]
        if lod == 0:
            sides = 5 if s.gen == 0 else (4 if s.gen == 1 else 3)
        else:
            sides = 3
        reg = 'bark' if s.gen < 2 else 'twig'
        n = len(path)
        thick = 1.0 if lod == 0 else (1.25 if lod == 1 else 1.4)
        rads = [(s.r0 + (s.r1 - s.r0) * (i / (n - 1))) * thick for i in range(n)]
        sway = [s.s0 + (s.s1 - s.s0) * (i / (n - 1)) for i in range(n)]
        rr = random.Random(s.order * 3 + 1)
        cols = [tint((1.0, .98, .95), .8 + .2 * min(1.0, p[2] / H)) for p in path]
        k.tube(path, rads, sides, reg, sway, cols, v0=rr.random() * .6, vscale=.5, cap=lod == 0 and s.gen >= 2, flat=lod > 0)
    _cards(k, cfg, use, r, H, lod)
    if lod == 0:
        _pins(k, cfg, segs, H)
    return k


def _pins(k, cfg, segs, H):
    """Døde, rake pinner som stikker ut skjevt og bryter omrisset. Lyse og grå, tre sider, ett ledd."""
    r = random.Random(cfg['seed'] * 5 + 3)
    ends = [s for s in segs if s.gen == 1 or s.gen == 2]
    for i in range(cfg['pins']):
        s = r.choice(ends)
        t = r.uniform(.3, 1.0)
        at = _on_path(s.path, t)
        sway = s.s0 + (s.s1 - s.s0) * t
        d = norm(add(norm((at[0], at[1], .1)), (r.uniform(-.5, .5), r.uniform(-.5, .5), r.uniform(-.1, .9))))
        ln = r.uniform(.28, .55)
        end = add(at, mul(d, ln))
        if end[2] > H or (end[0] * end[0] + end[1] * end[1]) ** .5 > cfg['reach']:
            ln *= .5
            end = add(at, mul(d, ln))
        k.tube([at, end], [.008, .003], 3, 'twig', [sway, min(1.0, sway + .2)], (.98, .96, .92), v0=r.random() * .5, vscale=.5)


def _cards(k, cfg, segs, r, H, lod):
    """Kvist- og løvkort ytterst på greinene: to tredjedeler ved endene, resten nedover. Glisne med vilje."""
    ends = [s for s in segs if s.gen >= (2 if lod < 2 else 1)]
    n = cfg['cards'] if lod == 0 else (94 if lod == 1 else 22)
    grow = 1.0 if lod == 0 else (1.42 if lod == 1 else 2.4)
    far = lod == 2
    r2 = random.Random(cfg['seed'] * 9 + 1)
    for i in range(n):
        s = ends[i % len(ends)] if i < len(ends) * 2 else r2.choice(ends)
        t = r2.uniform(.1, 1.0) if lod < 2 else r2.uniform(.6, 1.0)
        at = _on_path(s.path, t)
        s_at = s.s0 + (s.s1 - s.s0) * t
        pick = r2.random()
        if far:
            reg = r2.choice(('far_a', 'far_b', 'far_c'))
        elif pick < cfg['leaf'] and at[2] > H * .42:
            # Løvet bare ytterst og høyt: under er det bare kvister.
            reg = r2.choice(('leaf_a', 'leaf_b', 'leaf_c'))
        elif pick < cfg['leaf'] + cfg['bare']:
            reg = r2.choice(('bare_a', 'bare_b'))
        else:
            reg = 'pod_a'
        # Retning: ut fra planten og oppover, med grein-retningen som grunn.
        d = norm(sub(s.path[-1], s.path[0]))
        radial = norm((at[0], at[1], .02))
        along = norm(add(add(d, mul(radial, r2.uniform(.2, .6))), (r2.uniform(-.35, .35), r2.uniform(-.35, .35), r2.uniform(.05, .55))))
        # Lavt nede peker kortet oppover, så det ikke går gjennom bakken.
        if at[2] < .3 and along[2] < .25:
            along = norm((along[0], along[1], .35))
        u, w = basis(along)
        a = r2.uniform(0, math.tau)
        side = add(mul(u, math.cos(a)), mul(w, math.sin(a)))
        h = r2.uniform(.26, .56) * grow * (H / 2.2) ** .35
        # Kortet går ikke over bestilt høyde og bredde (spec.py).
        if along[2] > .05:
            h = max(.14, min(h, (H - at[2]) / along[2]))
        reach = cfg['reach'] - (at[0] * at[0] + at[1] * at[1]) ** .5
        out = (along[0] * along[0] + along[1] * along[1]) ** .5
        if out > .05:
            h = max(.14, min(h, max(.0, reach) / out))
        wid = h * .75
        nrm = norm(add(radial, (0, 0, 1.4)))
        col = tint((.9, .9, .86), .84 + .26 * min(1.0, at[2] / H))
        k.card(at, along, side, wid, h, reg, s_at, min(1.0, s_at + .14), nrm, col, tint(col, 1.05))
