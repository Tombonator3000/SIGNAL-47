"""Egne prosedyrale atlas til ørkenplantene. Ingen foto, ingen nedlastinger, ingen uttrekk.

Kjøres med en Python som har numpy og Pillow:
  python3 textures.py <utmappe>
Skriver T_DP_<Art>_BC.png (farge, alpha der masked) og T_DP_<Art>_R.png (ruhet). Regionene står i spec.ATLAS.
Flora-materialene (M_WebFlora_*) leser ikke normalkart, så detaljene males inn i fargen.
Gjennomsiktige piksler får regionens snittfarge i RGB, så mipkart ikke gir mørke kanter.
"""
import math
import random
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from spec import ATLAS

SS = 3  # Tegnes i tre ganger størrelse og skaleres ned med forhåndsmultiplisert alpha.


def fbm(rng, h, w, cx, cy, oct=4, gain=.55):
    """Sømløs verdistøy: cx x cy celler i første oktav."""
    out = np.zeros((h, w), np.float32)
    amp, tot = 1.0, 0.0
    for o in range(oct):
        nx, ny = cx * 2 ** o, cy * 2 ** o
        g = rng.random((ny, nx), dtype=np.float32)
        tx = np.arange(w, dtype=np.float32) * nx / w
        ty = np.arange(h, dtype=np.float32) * ny / h
        ix, iy = tx.astype(int), ty.astype(int)
        fx, fy = tx - ix, ty - iy
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        x1, y1 = (ix + 1) % nx, (iy + 1) % ny
        a = g[iy[:, None], ix[None, :]] * (1 - fx)[None, :] + g[iy[:, None], x1[None, :]] * fx[None, :]
        b = g[y1[:, None], ix[None, :]] * (1 - fx)[None, :] + g[y1[:, None], x1[None, :]] * fx[None, :]
        out += amp * (a * (1 - fy)[:, None] + b * fy[:, None])
        tot += amp
        amp *= gain
    return out / tot


def sst(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def col(*c):
    """sRGB 0-255 til float 0-1 (arbeidet skjer i sRGB, slik PNG-en lagres)."""
    return np.array(c, np.float32) / 255


def mix(a, b, t):
    t = t[..., None] if hasattr(t, 'ndim') and t.ndim == 2 else t
    return a * (1 - t) + b * t


def box(name, sp):
    """Regionen i piksler: (x0, y0, x1, y1) med y ned fra toppen av bildet."""
    N = ATLAS[sp]['size']
    u0, v0, u1, v1 = ATLAS[sp][name]
    return round(u0 * N), round((1 - v1) * N), round(u1 * N), round((1 - v0) * N)


def put(arr, name, sp, rgb, alpha=None, rough=None, R=None):
    x0, y0, x1, y1 = box(name, sp)
    arr[y0:y1, x0:x1, :3] = rgb
    arr[y0:y1, x0:x1, 3] = 1 if alpha is None else alpha
    if R is not None:
        R[y0:y1, x0:x1] = rough


def supersample(im):
    """RGBA-tegning til float, nedskalert med forhåndsmultiplisert alpha. RGB fylles med snittfargen utenfor."""
    a = np.asarray(im, np.float32) / 255
    alpha = a[..., 3:4]
    pre = a[..., :3] * alpha
    h, w = a.shape[0] // SS, a.shape[1] // SS
    pre = pre[:h * SS, :w * SS].reshape(h, SS, w, SS, 3).mean((1, 3))
    al = alpha[:h * SS, :w * SS].reshape(h, SS, w, SS, 1).mean((1, 3))
    rgb = pre / np.maximum(al, 1e-4)
    mean = pre.sum((0, 1)) / max(al.sum(), 1e-4)
    rgb = np.where(al > .004, rgb, mean)
    return rgb, al[..., 0]


# ---------- cholla ----------

def cholla(out):
    sp = 'cholla'
    N = ATLAS[sp]['size']
    rng = np.random.default_rng(801)
    rnd = random.Random(802)
    arr = np.zeros((N, N, 4), np.float32)
    R = np.full((N, N), .8, np.float32)
    # Hud: skrå knøler i et forskjøvet gitter (12 rundt), areole og korte pigger på hver knøl.
    x0, y0, x1, y1 = box('skin', sp)
    h, w = y1 - y0, x1 - x0
    cols, rows = 9, 12
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    best = np.zeros((h, w), np.float32)
    areole = np.zeros((h, w), np.float32)
    cw, ch = w / cols, h / rows
    centres = []
    for j in range(rows + 1):
        for i in range(cols):
            cx, cy = (i + .5 * (j % 2)) * cw + (j / rows) * cw * 1.5, j * ch + ch / 2
            centres.append(((cx + rnd.uniform(-.14, .14) * cw) % w, cy + rnd.uniform(-.14, .14) * ch))
    for cx, cy in centres:
        for dxw in (-w, 0, w):
            dx = (xs - cx - dxw) / (cw * .56)
            dy = (ys - cy) / (ch * .86)
            best = np.maximum(best, np.exp(-(dx * dx + dy * dy) * 1.5))
            ax = (xs - cx - dxw) / (cw * .2)
            ay = (ys - cy + ch * .34) / (ch * .11)
            areole = np.maximum(areole, np.exp(-(ax * ax + ay * ay)))
    n = fbm(rng, h, w, 6, 8)
    fine = fbm(rng, h, w, 40, 50, 3)
    groove, top = col(64, 70, 58), col(124, 132, 110)
    rgb = mix(groove, top, sst(.12, .86, best + (n - .5) * .25))
    rgb = mix(rgb, col(150, 152, 130), sst(.75, 1.0, best) * .5)
    blotch = fbm(rng, h, w, 3, 3, 3)
    rgb = mix(rgb, col(112, 104, 82), sst(.5, .78, blotch) * .35)
    rgb = rgb * (.86 + .28 * fine[..., None])
    rgb = mix(rgb, col(66, 56, 44), sst(.3, .9, areole) * .75)
    im = Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8)).resize((w * SS, h * SS), Image.BILINEAR)
    d = ImageDraw.Draw(im)
    for cx, cy in centres:
        px, py = cx * SS, (cy - ch * .34) * SS
        for _ in range(rnd.randint(3, 6)):
            a = rnd.uniform(0, math.tau)
            ln = rnd.uniform(.4, 1.0) * ch * SS * .62
            shade = rnd.randint(165, 225)
            for dxw in (-w * SS, 0, w * SS):
                d.line((px + dxw, py, px + dxw + math.cos(a) * ln, py + math.sin(a) * ln), fill=(shade, shade - 12, shade - 45), width=SS)
    rgb = np.asarray(im.resize((w, h), Image.BOX), np.float32) / 255
    put(arr, 'skin', sp, rgb, None, .68 + .2 * (1 - best), R)

    # Bark: mørk grå-brun, fibret og oppsprukket gitter (gamle ledd blir treaktige).
    x0, y0, x1, y1 = box('bark', sp)
    h, w = y1 - y0, x1 - x0
    fib = fbm(rng, h, w, 14, 2, 4)
    big = fbm(rng, h, w, 7, 5, 3)
    crack = sst(.012, .06, np.abs(fbm(rng, h, w, 16, 3, 3) - .5))
    rgb = mix(col(82, 74, 64), col(146, 134, 116), sst(.25, .75, fib * .7 + big * .3))
    rgb = rgb * (.5 + .5 * crack[..., None])
    put(arr, 'bark', sp, rgb, None, .9 + .08 * fib, R)

    # Tørt ledd: halm og grått med mørke ribber.
    x0, y0, x1, y1 = box('dry', sp)
    h, w = y1 - y0, x1 - x0
    rib = np.abs(np.sin(np.arange(w, dtype=np.float32)[None, :] / w * math.tau * 5 + fbm(rng, h, w, 4, 4)[..., ] * 3)) ** .6
    n = fbm(rng, h, w, 6, 10)
    rgb = mix(col(58, 50, 42), col(134, 122, 104), sst(.2, .9, rib * .6 + n * .5))
    put(arr, 'dry', sp, rgb, None, .95, R)

    # Frukt: gulgrønn med knøler.
    x0, y0, x1, y1 = box('fruit', sp)
    h, w = y1 - y0, x1 - x0
    n = fbm(rng, h, w, 7, 7)
    rgb = mix(col(150, 150, 84), col(186, 178, 100), sst(.3, .75, n))
    put(arr, 'fruit', sp, rgb, None, .72, R)

    # Piggkort: nåler fra rørets kant og utover, gjennomsiktig ellers.
    x0, y0, x1, y1 = box('spines', sp)
    h, w = y1 - y0, x1 - x0
    im = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for _ in range(115):
        side = rnd.choice((-1, 1))
        rx = w * SS * (.5 + side * rnd.uniform(.17, .28))
        ry = rnd.uniform(.03, .97) * h * SS
        for _ in range(rnd.randint(3, 6)):
            a = rnd.uniform(-1.05, 1.05)
            ln = rnd.uniform(.09, .22) * w * SS
            ex, ey = rx + side * math.cos(a) * ln, ry + math.sin(a) * ln
            shade = rnd.randint(190, 240)
            d.line((rx, ry, ex, ey), fill=(shade, shade - 12, shade - 40, 255), width=max(2, SS))
    rgb, al = supersample(im)
    put(arr, 'spines', sp, rgb, al, .6, R)
    save(out, 'Cholla', arr, R, masked=True)


# ---------- yucca ----------

def blade_map(rng, h, w, dark, light, margin, tip):
    """Blad med roten nederst i bildet: mørk rand-til-rand-stripe, lys midtribbe, gul kant, tørr tupp."""
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    t = 1 - ys / h  # 0 ved roten, 1 i tuppen
    u = xs / w
    streak = fbm(rng, h, w, 18, 2, 3)
    rgb = mix(dark, light, sst(.3, .75, streak))
    rib = np.exp(-((u - .5) / .06) ** 2)
    rgb = mix(rgb, light * 1.2, rib * .5)
    edge = sst(.42, .49, np.abs(u - .5))
    rgb = mix(rgb, margin, edge)
    rgb = mix(rgb, col(226, 214, 172), sst(.2, .03, t))
    rgb = mix(rgb, tip, sst(.86, .97, t))
    return rgb


def yucca(out):
    sp = 'yucca'
    N = ATLAS[sp]['size']
    rng = np.random.default_rng(811)
    arr = np.zeros((N, N, 4), np.float32)
    R = np.full((N, N), .6, np.float32)

    def size(name):
        x0, y0, x1, y1 = box(name, sp)
        return y1 - y0, x1 - x0

    h, w = size('green_a')
    put(arr, 'green_a', sp, blade_map(rng, h, w, col(82, 96, 58), col(126, 136, 88), col(198, 192, 140), col(176, 158, 104)), None, .55, R)
    h, w = size('green_b')
    put(arr, 'green_b', sp, blade_map(rng, h, w, col(92, 102, 64), col(142, 148, 98), col(208, 198, 148), col(186, 166, 112)), None, .58, R)

    for name, a, b in (('dead_a', col(150, 132, 104), col(208, 194, 162)), ('dead_b', col(110, 98, 84), col(176, 164, 144))):
        h, w = size(name)
        streak = fbm(rng, h, w, 22, 2, 3)
        rgb = mix(a, b, sst(.28, .72, streak))
        split = sst(.008, .04, np.abs(fbm(rng, h, w, 16, 2, 2) - .5))
        rgb = rgb * (.55 + .45 * split[..., None])
        put(arr, name, sp, rgb, None, .92, R)

    h, w = size('stalk')
    n = fbm(rng, h, w, 8, 2, 3)
    ys = np.mgrid[0:h, 0:w][0].astype(np.float32) / h
    rgb = mix(col(116, 118, 72), col(160, 146, 96), sst(.3, .8, n))
    rgb = mix(rgb, col(122, 96, 72), sst(.45, .1, ys))
    put(arr, 'stalk', sp, rgb, None, .7, R)

    h, w = size('trunk')
    n = fbm(rng, h, w, 9, 3, 4)
    rgb = mix(col(40, 34, 28), col(104, 90, 72), sst(.3, .75, n))
    put(arr, 'trunk', sp, rgb, None, .95, R)

    h, w = size('pod')
    n = fbm(rng, h, w, 5, 4, 3)
    rgb = mix(col(84, 70, 54), col(154, 134, 104), sst(.3, .78, n))
    put(arr, 'pod', sp, rgb, None, .92, R)

    h, w = size('flower')
    n = fbm(rng, h, w, 6, 6, 3)
    ys = np.mgrid[0:h, 0:w][0].astype(np.float32) / h
    rgb = mix(col(238, 232, 192), col(250, 246, 216), sst(.3, .7, n))
    rgb = mix(rgb, col(196, 192, 126), sst(.72, 1.0, ys))
    put(arr, 'flower', sp, rgb, None, .7, R)

    h, w = size('bud')
    n = fbm(rng, h, w, 5, 5, 3)
    put(arr, 'bud', sp, mix(col(168, 170, 102), col(214, 210, 150), sst(.3, .7, n)), None, .7, R)

    h, w = size('bract')
    n = fbm(rng, h, w, 6, 2, 3)
    put(arr, 'bract', sp, mix(col(142, 120, 82), col(198, 176, 128), sst(.3, .7, n)), None, .85, R)
    save(out, 'Yucca', arr, R, masked=False)


# ---------- mesquite ----------

def line(d, a, b, width, fill):
    d.line((a[0], a[1], b[0], b[1]), fill=fill, width=max(1, round(width)))


def lc(a, b, t):
    return tuple(round(a[i] + (b[i] - a[i]) * t) for i in range(3)) + (255,)


class Spray:
    """Tegner en kvist med sikksakk, torner og valgfritt løv/belger. Roten nederst på midten av kortet."""

    def __init__(self, seed, w, h, leaf=0.0, pods=0, thick=1.0, far=False):
        self.r = random.Random(seed)
        self.w, self.h = w, h
        self.leaf, self.pods, self.thick, self.far = leaf, pods, thick, far
        self.im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)
        self.nodes = []

    def twig(self, p, a, ln, width, gen):
        r = self.r
        n = max(3, int(ln / (self.h * .05)))
        step = ln / n
        sign = r.choice((-1, 1))
        for i in range(n):
            t = (gen + i / n) / 4
            a2 = a + sign * r.uniform(.2, .42)
            sign = -sign
            q = (p[0] + math.sin(a2) * step, p[1] - math.cos(a2) * step)
            base = lc((96, 86, 76), (196, 186, 170), min(1, t))
            line(self.d, p, q, max(self.thick * SS * 1.1, width * (1 - i / n * .45)), base)  # Minst én piksel: ellers klippes kvisten bort, og løvet svever.
            # Torner parvis i knekken.
            if not self.far and r.random() < .55:
                for s in (-1, 1):
                    ta = a2 + s * r.uniform(.7, 1.15)
                    tl = r.uniform(.012, .022) * self.h
                    line(self.d, q, (q[0] + math.sin(ta) * tl, q[1] - math.cos(ta) * tl), max(SS * .6, width * .32), (178, 156, 120, 255))
            self.nodes.append((q, a2, gen))
            # Sideskudd.
            if gen < 3 and r.random() < (.72 - gen * .08):
                s = r.choice((-1, 1))
                self.twig(q, a2 + s * r.uniform(.45, .95), ln * r.uniform(.42, .68), width * .68, gen + 1)
            p = q

    def compound(self, q, a, size):
        """Mesquiteblad: en kort stilk som deler seg i to fjær med tynne småblad."""
        r = self.r
        green = r.choice([(120, 132, 60), (140, 146, 66), (150, 148, 72), (108, 122, 54), (160, 146, 96)])
        hx, hy = q[0] + math.sin(a) * size * .25, q[1] - math.cos(a) * size * .25
        line(self.d, q, (hx, hy), SS * 1.1, (120, 120, 70, 255))
        for fork in (-.4, .4):
            b = a + fork
            for i in range(11):
                t = (i + 1) / 11
                sx, sy = hx + math.sin(b) * size * t, hy - math.cos(b) * size * t
                ll = size * .24 * (1 - .4 * abs(t - .4)) * r.uniform(.7, 1.15)
                for side in (-1, 1):
                    if r.random() < .14:
                        continue
                    c = b + side * r.uniform(.9, 1.3)
                    line(self.d, (sx, sy), (sx + math.sin(c) * ll, sy - math.cos(c) * ll), SS * (1.5 if self.far else 1.2), green + (255,))
            line(self.d, (hx, hy), (hx + math.sin(b) * size, hy - math.cos(b) * size), SS * 1.0, (110, 116, 62, 255))

    def pod(self, q, ln):
        """Tørr belg som henger: smal, lett krum, med frøknuter."""
        r = self.r
        p = q
        bow = r.uniform(-.25, .25)
        for i in range(7):
            a = math.pi + bow * (i / 7 - .5)
            e = (p[0] + math.sin(a) * ln / 7, p[1] - math.cos(a) * ln / 7)
            shade = r.randint(175, 215)
            line(self.d, p, e, SS * 3.0, (shade, shade - 22, shade - 74, 255))
            p = e

    def finish(self, root_w, length):
        self.twig((self.w / 2, self.h * .995), self.r.uniform(-.15, .15), length, root_w, 0)
        r = self.r
        nodes = list(self.nodes)
        r.shuffle(nodes)
        n = int(len(nodes) * self.leaf)
        for (q, a, gen) in nodes[:n]:
            self.compound(q, a + r.uniform(-.8, .8), self.h * r.uniform(.09, .15))
        for (q, a, gen) in nodes[n:n + self.pods]:
            self.pod(q, self.h * r.uniform(.11, .18))
        return supersample(self.im)


def mesquite(out):
    sp = 'mesquite'
    N = ATLAS[sp]['size']
    rng = np.random.default_rng(821)
    arr = np.zeros((N, N, 4), np.float32)
    R = np.full((N, N), .85, np.float32)

    x0, y0, x1, y1 = box('bark', sp)
    h, w = y1 - y0, x1 - x0
    fib = fbm(rng, h, w, 10, 3, 4)
    big = fbm(rng, h, w, 5, 6, 3)
    crack = sst(.01, .055, np.abs(fbm(rng, h, w, 12, 2, 3) - .5))
    rgb = mix(col(88, 80, 72), col(166, 156, 142), sst(.25, .78, fib * .7 + big * .3))
    rgb = rgb * (.42 + .58 * crack[..., None])
    put(arr, 'bark', sp, rgb, None, .93, R)

    x0, y0, x1, y1 = box('twig', sp)
    h, w = y1 - y0, x1 - x0
    fib = fbm(rng, h, w, 6, 4, 3)
    put(arr, 'twig', sp, mix(col(104, 94, 84), col(176, 166, 150), sst(.28, .75, fib)), None, .85, R)

    cards = {
        'leaf_a': dict(seed=1, leaf=.16, pods=0),
        'leaf_b': dict(seed=2, leaf=.2, pods=0),
        'leaf_c': dict(seed=3, leaf=.12, pods=1),
        'bare_a': dict(seed=4, leaf=.0, pods=0),
        'bare_b': dict(seed=5, leaf=.015, pods=0),
        'pod_a': dict(seed=6, leaf=.06, pods=5),
        'far_a': dict(seed=7, leaf=.14, pods=0, thick=2.6, far=True),
        'far_b': dict(seed=8, leaf=.06, pods=0, thick=2.6, far=True),
        'far_c': dict(seed=9, leaf=.0, pods=0, thick=2.6, far=True),
    }
    for name, cfg in cards.items():
        far = cfg.get('far', False)
        x0, y0, x1, y1 = box(name, sp)
        W, H = (x1 - x0) * SS, (y1 - y0) * SS
        s = Spray(cfg['seed'] * 31, W, H, cfg['leaf'], cfg['pods'], cfg.get('thick', 1.0), far)
        rgb, al = s.finish(SS * (3.6 if not far else 6.5), H * .6)
        rough = np.where(al > .5, .85, .9)
        put(arr, name, sp, rgb, al, rough, R)
    save(out, 'Mesquite', arr, R, masked=True)


# ---------- creosote ----------

LEAF_DARK = ((72, 82, 30), (80, 88, 32), (66, 76, 32))
LEAF_MID = ((108, 116, 40), (122, 126, 44), (100, 110, 42), (132, 130, 48))
LEAF_LIGHT = ((146, 148, 56), (158, 154, 54), (136, 142, 58), (170, 160, 60))


class CreosoteSpray:
    """En kvist med motsatte, små tolappede blad, tettest ytterst. Roten nederst på midten av kortet.

    Bladene er rundt 5 % av kortets høyde, altså omtrent 1 cm på en 20 cm kvist. De tegnes i tre lag, mørke bakerst, så
    klyngene er tette nok til at alpha holder seg over klippet i mipkartene.
    """

    def __init__(self, seed, w, h, leaf=1.0, flowers=0, far=False):
        self.r = random.Random(seed)
        self.w, self.h = w, h
        self.leaf, self.flowers, self.far = leaf, flowers, far
        self.im = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)
        self.nodes = []

    def twig(self, p, a, ln, width, gen, t0=0.0):
        r = self.r
        n = max(3, int(ln / (self.h * .04)))
        step = ln / n
        sign = r.choice((-1, 1))
        for i in range(n):
            a2 = a + sign * r.uniform(.1, .3)
            sign = -sign
            q = (p[0] + math.sin(a2) * step, p[1] - math.cos(a2) * step)
            # En kvist som når kanten av kortet, stopper: ellers klippes den rett av i bildet.
            if not self.inside(q, .1):
                return
            shade = lc((48, 40, 34), (112, 100, 78), min(1.0, gen * .3 + i / n * .4))
            line(self.d, p, q, max(SS * 1.1, width * (1 - i / n * .5)), shade)
            # Mørk ring i knekken, som på referansene.
            ring = max(SS * 1.4, width * 1.15)
            self.d.ellipse((q[0] - ring / 2, q[1] - ring / 2, q[0] + ring / 2, q[1] + ring / 2), fill=(38, 32, 28, 255))
            self.nodes.append((q, a2, min(1.0, t0 + (i + 1) / n * (1 - t0))))
            if gen < 2 and i >= n // 4 and r.random() < .7 - gen * .12:
                s = r.choice((-1, 1))
                self.twig(q, a2 + s * r.uniform(.5, .95), ln * r.uniform(.42, .62), width * .66, gen + 1, t0 + (i + 1) / n * .5)
            p = q

    def inside(self, q, margin):
        return margin * self.w < q[0] < (1 - margin) * self.w and margin * self.h * .75 < q[1]

    def leaflet(self, q, a, size, fill, spot=None):
        """To smale lapper som sprer seg fra samme feste."""
        w = size * (.3 if self.far else .2)
        if not self.inside((q[0] + math.sin(a) * size * 1.1, q[1] - math.cos(a) * size * 1.1), .02):
            return
        for side in (-.24, .24):
            b = a + side
            pts = []
            for k in range(9):
                t = k / 8
                # Bred midt på, spiss i enden, bue på én side.
                half = w * math.sin(min(1.0, t * 1.08) * math.pi) ** .7
                px = q[0] + math.sin(b) * size * t
                py = q[1] - math.cos(b) * size * t
                pts.append((px + math.cos(b) * half, py + math.sin(b) * half))
            for k in range(8, -1, -1):
                t = k / 8
                half = w * .8 * math.sin(min(1.0, t * 1.08) * math.pi) ** .7
                px = q[0] + math.sin(b) * size * t
                py = q[1] - math.cos(b) * size * t
                pts.append((px - math.cos(b) * half, py - math.sin(b) * half))
            self.d.polygon(pts, fill=fill + (255,))
            if spot:
                self.d.line((q[0] + math.sin(b) * size * .2, q[1] - math.cos(b) * size * .2, q[0] + math.sin(b) * size * .8, q[1] - math.cos(b) * size * .8), fill=spot + (255,), width=max(1, SS // 2))

    def foliage(self):
        r = self.r
        size0 = self.h * (.085 if self.far else .04)
        nodes = [n for n in self.nodes if n[2] > .22]
        for colours, keep, sc in ((LEAF_DARK, 1.0, 1.08), (LEAF_MID, .9, 1.0), (LEAF_LIGHT, .42, .9)):
            order = list(nodes)
            r.shuffle(order)
            for (q, a, t) in order:
                # Tettere ytterst på kvisten.
                if r.random() > keep * self.leaf * (.45 + .75 * t):
                    continue
                for side in (-1, 1):
                    b = a + side * r.uniform(.5, 1.25)
                    size = size0 * sc * r.uniform(.8, 1.2)
                    # Lagene forskyves litt, så de ikke dekker hverandre helt.
                    shift = size * r.uniform(-.25, .35)
                    at = (q[0] + math.sin(b) * shift, q[1] - math.cos(b) * shift)
                    fill = r.choice(colours)
                    spot = None if self.far else tuple(max(0, c - 30) for c in fill)
                    self.leaflet(at, b, size, fill, spot)
        # Noen få små gule blomster og ulne frøkuler, våren 1986.
        for (q, a, t) in r.sample(nodes, min(self.flowers, len(nodes))):
            rad = self.h * .011
            c = r.choice(((206, 186, 66), (190, 170, 60), (212, 204, 170)))
            self.d.ellipse((q[0] - rad, q[1] - rad, q[0] + rad, q[1] + rad), fill=c + (255,))

    def finish(self, root_w, length):
        self.twig((self.w / 2, self.h * .995), self.r.uniform(-.12, .12), length, root_w, 0)
        if self.leaf > 0:
            self.foliage()
        return supersample(self.im)


def creosote(out):
    sp = 'creosote'
    N = ATLAS[sp]['size']
    rng = np.random.default_rng(831)
    arr = np.zeros((N, N, 4), np.float32)
    R = np.full((N, N), .85, np.float32)

    # Stammen: mørk grå-brun, lysere flekker og tette mørke ringer på tvers (v går langs stammen).
    x0, y0, x1, y1 = box('bark', sp)
    h, w = y1 - y0, x1 - x0
    fib = fbm(rng, h, w, 8, 6, 4)
    big = fbm(rng, h, w, 3, 10, 3)
    # (ringene følger bare v, og brytes av flekker: de går ikke helt rundt)
    ring = 1 - (1 - sst(.006, .03, np.abs(fbm(rng, h, 1, 1, 30, 2) - .5))) * sst(.42, .58, fbm(rng, h, w, 4, 12, 2))
    rgb = mix(col(44, 38, 34), col(118, 108, 96), sst(.28, .72, fib * .55 + big * .45))
    rgb = rgb * (.45 + .55 * ring[..., None])
    put(arr, 'bark', sp, rgb, None, .92, R)

    x0, y0, x1, y1 = box('twig', sp)
    h, w = y1 - y0, x1 - x0
    fib = fbm(rng, h, w, 5, 8, 3)
    ring = 1 - (1 - sst(.006, .03, np.abs(fbm(rng, h, 1, 1, 40, 2) - .5))) * sst(.42, .58, fbm(rng, h, w, 4, 16, 2))
    rgb = mix(col(60, 52, 44), col(128, 116, 96), sst(.3, .72, fib))
    put(arr, 'twig', sp, rgb * (.55 + .45 * ring[..., None]), None, .88, R)

    cards = {
        'leaf_a': dict(seed=11, leaf=1.0, flowers=2, root=.016, length=.72),
        'leaf_b': dict(seed=12, leaf=1.0, flowers=0, root=.018, length=.76),
        'leaf_c': dict(seed=13, leaf=.86, flowers=4, root=.015, length=.7),
        'tip_a': dict(seed=14, leaf=.95, flowers=0, root=.012, length=.56),
        'tip_b': dict(seed=15, leaf=.8, flowers=3, root=.012, length=.6),
        'bare_a': dict(seed=16, leaf=.2, flowers=0, root=.016, length=.8),
        'far_a': dict(seed=17, leaf=1.0, flowers=0, root=.03, length=.72, far=True),
        'far_b': dict(seed=18, leaf=.9, flowers=0, root=.03, length=.7, far=True),
        'far_c': dict(seed=19, leaf=.72, flowers=0, root=.034, length=.74, far=True),
    }
    for name, cfg in cards.items():
        x0, y0, x1, y1 = box(name, sp)
        W, H = (x1 - x0) * SS, (y1 - y0) * SS
        s = CreosoteSpray(cfg['seed'] * 37, W, H, cfg['leaf'], cfg['flowers'], cfg.get('far', False))
        rgb, al = s.finish(H * cfg['root'], H * cfg['length'])
        # De store flekkene på avstand har mindre mørkt mellom bladene: samme snittfarge som nærkortene (målt), så LOD-byttet ikke lyser opp.
        if cfg.get('far'):
            rgb = rgb * .86
        # Bladene er voksaktige og litt blanke, kvistene matte.
        leafy = np.clip((rgb[..., 1] - rgb[..., 2]) * 6, 0, 1)
        rough = (.8 - .28 * leafy) * (al > .3) + .9 * (al <= .3)
        put(arr, name, sp, rgb, al, rough, R)
    save(out, 'Creosote', arr, R, masked=True)


def save(out, name, arr, R, masked):
    a = np.clip(arr, 0, 1)
    if masked:
        Image.fromarray((a * 255 + .5).astype(np.uint8), 'RGBA').save(out / f'T_DP_{name}_BC.png')
    else:
        Image.fromarray((a[..., :3] * 255 + .5).astype(np.uint8), 'RGB').save(out / f'T_DP_{name}_BC.png')
    Image.fromarray((np.clip(R, 0, 1) * 255 + .5).astype(np.uint8), 'L').convert('RGB').save(out / f'T_DP_{name}_R.png')
    print('skrev', out / f'T_DP_{name}_BC.png')


if __name__ == '__main__':
    root = Path(sys.argv[1] if len(sys.argv) > 1 else 'textures')
    root.mkdir(parents=True, exist_ok=True)
    which = sys.argv[2:] or ['cholla', 'yucca', 'mesquite', 'creosote']
    for fn in which:
        {'cholla': cholla, 'yucca': yucca, 'mesquite': mesquite, 'creosote': creosote}[fn](root)
