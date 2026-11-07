"""Mesh-klosser uten bpy, så check.py kan måle uten Blender.

Alt er trekanter med egne hjørner per vertex: posisjon, normal, UV0 (atlas), vind (UV1.x, som aSway i
flora.ts: 0 ved roten, 1 ved et grasstrå-tupp) og farge (lineær, ganges med teksturen).
Vind er alltid kontinuerlig: et barn starter med forelderens vindtall, så ingen del løsner i FLORA_WPO.
"""
import math

from spec import ATLAS


def add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def mul(a, k): return (a[0] * k, a[1] * k, a[2] * k)
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def length(a): return math.sqrt(dot(a, a))
def lerp(a, b, t): return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t)


def norm(a):
    n = length(a)
    return (a[0] / n, a[1] / n, a[2] / n) if n > 1e-9 else (0.0, 0.0, 1.0)


def polar(az, el):
    """Enhetsvektor: az rundt Z, el vinkel fra rett opp."""
    return (math.cos(az) * math.sin(el), math.sin(az) * math.sin(el), math.cos(el))


def basis(d):
    """To vektorer vinkelrett på d."""
    u = cross((0.31, 0.27, 0.91), d)
    if dot(u, u) < 1e-4:
        u = cross((1.0, 0.0, 0.0), d)
    u = norm(u)
    return u, cross(d, u)


class Kit:
    def __init__(self, species):
        self.species = species
        self.atlas = ATLAS[species]
        self.v, self.n, self.uv, self.s, self.c, self.f = [], [], [], [], [], []
        self.joins = []  # (punkt, vindtall hos forelder, vindtall hos barn) til check.py

    def tris(self):
        return len(self.f)

    def vert(self, p, n, uv, s, c):
        self.v.append(p)
        self.n.append(n)
        self.uv.append(uv)
        self.s.append(s)
        self.c.append(c)
        return len(self.v) - 1

    def tri(self, a, b, c):
        self.f.append((a, b, c))

    def quad(self, a, b, c, d):
        self.f.append((a, b, c))
        self.f.append((a, c, d))

    def region(self, name, u, v):
        r = self.atlas[name]
        return (r[0] + (r[2] - r[0]) * u, r[1] + (r[3] - r[1]) * v)

    def tube(self, path, radii, sides, reg, sway, col, v0=0.0, vscale=1.0, cap=False, rough=None, tip_len=0.0, flat=False):
        """Rør langs path. radii, sway og col per ring (col kan være én farge). rough: funksjon(ring, side) -> faktor.

        cap: spiss ende. flat: tresidig rør lukkes med én trekant, for åpne ender vises på avstand.
        """
        n = len(path)
        d = norm(sub(path[1], path[0]))
        u0, w0 = basis(d)
        rings = []
        dist = 0.0
        for i in range(n):
            if i:
                dist += length(sub(path[i], path[i - 1]))
            if 0 < i < n - 1:
                dn = norm(sub(path[i + 1], path[i - 1]))
            else:
                dn = norm(sub(path[min(i + 1, n - 1)], path[max(i - 1, 0)]))
            # Parallelltransport, så røret ikke vris.
            u0 = norm(sub(u0, mul(dn, dot(u0, dn))))
            w0 = cross(dn, u0)
            ring = []
            for k in range(sides + 1):
                t = k / sides * math.tau
                nrm = add(mul(u0, math.cos(t)), mul(w0, math.sin(t)))
                f = rough(i, k % sides) if rough else 1.0
                p = add(path[i], mul(nrm, radii[i] * f))
                c = col[i] if isinstance(col[0], (tuple, list)) else col
                ring.append(self.vert(p, nrm, self.region(reg, k / sides, min(0.999, (v0 + dist * vscale) % 1.0)), sway[i], c))
            rings.append(ring)
        # Vindtall og UV på siste ring brukes til spissen.
        for i in range(n - 1):
            for k in range(sides):
                self.quad(rings[i][k], rings[i][k + 1], rings[i + 1][k + 1], rings[i + 1][k])
        if flat:
            self.tri(rings[-1][0], rings[-1][1], rings[-1][2])
        if cap:
            dn = norm(sub(path[-1], path[-2]))
            apex = add(path[-1], mul(dn, tip_len if tip_len else radii[-1] * 0.7))
            c = col[-1] if isinstance(col[0], (tuple, list)) else col
            a = self.vert(apex, dn, self.region(reg, 0.5, min(0.999, (v0 + (dist + tip_len) * vscale) % 1.0)), sway[-1], c)
            last = rings[-1]
            for k in range(sides):
                self.tri(last[k], last[k + 1], a)

    def card(self, root, along, side, w, h, reg, s0, s1, nrm, col, col_tip=None):
        """Firkant med roten midt på nederste kant. Hjørnene ved roten har forelderens vindtall s0."""
        hw = mul(side, w / 2)
        top = add(root, mul(along, h))
        c2 = col_tip or col
        a = self.vert(sub(root, hw), nrm, self.region(reg, 0, 0), s0, col)
        b = self.vert(add(root, hw), nrm, self.region(reg, 1, 0), s0, col)
        c = self.vert(add(top, hw), nrm, self.region(reg, 1, 1), s1, c2)
        d = self.vert(sub(top, hw), nrm, self.region(reg, 0, 1), s1, c2)
        self.quad(a, b, c, d)

    def bead(self, at, rad, tall, reg, s, col, az=0.0):
        """Liten dobbelt pyramide, seks trekanter: blomst, knopp, belg."""
        ring = []
        for i in range(3):
            t = az + i * math.tau / 3
            n = (math.cos(t), math.sin(t), 0.0)
            ring.append(self.vert(add(at, mul(n, rad)), n, self.region(reg, i / 3, 0.5), s, col))
        top = self.vert(add(at, (0, 0, tall)), (0, 0, 1), self.region(reg, 0.5, 1.0), s, col)
        bot = self.vert(add(at, (0, 0, -tall)), (0, 0, -1), self.region(reg, 0.5, 0.0), s, col)
        for i in range(3):
            self.tri(ring[i], ring[(i + 1) % 3], top)
            self.tri(ring[(i + 1) % 3], ring[i], bot)

    def bounds(self):
        xs, ys, zs = zip(*self.v)
        return (min(xs), min(ys), min(zs)), (max(xs), max(ys), max(zs))


def tint(c, k):
    return (c[0] * k, c[1] * k, c[2] * k)
