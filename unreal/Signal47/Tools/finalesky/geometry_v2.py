"""Optional original hero-craft variant, using the existing source-builder API.

Keeps the low, continuous silhouette; concentrates readable detail in three
industrial collars, short radial webs and eight small recessed running lights.
The archived recipe and its verified packages are not modified.
"""
import math

from geometry import Solid, lathe


PROFILE = [
    (-.092, .055), (-.086, .25), (-.068, .53), (-.039, .79),
    (-.015, .97), (-.003, 1), (.012, .993), (.029, .89),
    (.055, .66), (.077, .34), (.085, .045),
]


def underside_z(radius):
    lower = PROFILE[:6]
    for (z0, r0), (z1, r1) in zip(lower, lower[1:]):
        if r0 <= radius <= r1:
            return z0 + (z1 - z0) * (radius - r0) / (r1 - r0)
    raise ValueError(radius)


def dished_wedge(name, radii, a0, a1, depth, burial, material, steps):
    """Closed casing following the hull rather than floating under its slope."""
    s = Solid(name, material)
    arc = [a0 + (a1-a0) * i/steps for i in range(steps+1)]
    lo = [[(r*math.cos(a), r*math.sin(a), underside_z(r)-depth)
           for a in arc] for r in radii]
    hi = [[(x, y, z+depth+burial) for x, y, z in row] for row in lo]
    for k in range(len(radii)-1):
        for i in range(steps):
            j = i+1
            s.face([lo[k][i], lo[k][j], lo[k+1][j], lo[k+1][i]])
            s.face([hi[k+1][i], hi[k+1][j], hi[k][j], hi[k][i]])
        s.face([lo[k][0], lo[k+1][0], hi[k+1][0], hi[k][0]])
        s.face([lo[k+1][-1], lo[k][-1], hi[k][-1], hi[k+1][-1]])
    for i in range(steps):
        j = i+1
        s.face([lo[0][i], hi[0][i], hi[0][j], lo[0][j]])
        s.face([lo[-1][i], lo[-1][j], hi[-1][j], hi[-1][i]])
    return s


def build(lod):
    n = (192, 112, 64)[lod]
    out = [lathe('Hero_continuous_dished_hull', PROFILE, n, 'Hull')]

    # Broad, separated structures read as an immense physical miniature.
    # Each top edge intersects the hull; the lower edge catches a restrained key.
    for i, (r, width, drop) in enumerate((
            (.965, .010, .0035), (.777, .017, .0042), (.313, .012, .0038))):
        z = underside_z(r)
        out.append(lathe('Hero_industrial_collar_%d' % i, [
            (z-drop, r-width), (z-drop+.0011, r+width),
            (z+.0020, r+width), (z+.0030, r-width)], n, 'Ribs'))

    # A quiet dark equatorial seam. It does not expand the silhouette radius.
    out.append(lathe('Hero_recessed_equatorial_belt', [
        (-.0136, .982), (-.0113, .990), (-.0056, .997),
        (-.0041, .994)], n, 'Recess'))

    for i in range(24):
        a = i * 2*math.pi/24
        out.append(dished_wedge('Hero_short_radial_web_%02d' % i,
                                (.365, .53, .665), a+.027, a+.042,
                                .0031, .0013, 'Ribs', 3 if lod == 0 else 2))

    # Twelve small service wells, rather than a second full band of greebles.
    for i in range(12):
        a = i * 2*math.pi/12 + .071
        out.append(dished_wedge('Hero_service_well_%02d' % i,
                                (.837, .858), a, a+.036,
                                .0018, .0010, 'Recess', 2))

    # Eight tiny light apertures. The well is larger than its dim lamp.
    for i in range(8):
        a = i * 2*math.pi/8 + .16
        out.append(dished_wedge('Hero_running_light_well_%02d' % i,
                                (.978, .984, .990), a, a+.012,
                                .0020, .0012, 'Recess', 2))
        out.append(dished_wedge('Hero_running_light_%02d' % i,
                                (.983, .988), a+.0036, a+.0084,
                                .0027, -.0018, 'Light', 2))

    out.append(lathe('Hero_central_dark_optical_well', [
        (-.096, .04), (-.094, .13), (-.089, .14), (-.087, .04)],
        max(32, n//2), 'Recess'))
    out.append(lathe('Hero_central_dim_lens', [
        (-.097, .021), (-.096, .032), (-.094, .032)],
        max(24, n//2), 'Light'))

    for s in out:
        assert s.volume() > 0, (s.name, 'closed solid inward/degenerate', s.volume())
        assert len(s.triangles) == len(s.uv)
    return out


# Also useful to lightweight authoring callers; build_source.py uses build.
make = build
