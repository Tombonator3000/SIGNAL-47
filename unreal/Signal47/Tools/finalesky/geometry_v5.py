"""Original v5 craft: eight broad tapered underside bearing fields.

The complete v4 shell, outer collar, optical well and eight apertures are reused
unchanged. Only the eight shallow webs are replaced. At the current1000m radius,
the new fields run320..780m, stand6..10m below the hull and have one restrained
Ribs bevel per field. No new lamps, material slots or external source assets.
"""
import math

from geometry import Solid, cross
from geometry_v2 import underside_z
from geometry_v4 import build as build_v4, quiet_uv, unit


# Radius, half angular width, underside relief. The broad middle tapers toward
# both load paths; the narrow ends leave the optical collar and outer shell clear.
STATIONS = (
    (.320, .048, .006), (.340, .070, .008), (.380, .102, .010),
    (.530, .124, .010), (.650, .112, .009), (.740, .082, .008),
    (.760, .064, .007), (.780, .044, .006),
)
ARC_STEPS = (12, 6, 3)
BEVEL_RUN = .012  #12m lateral run; bevel height is3.3..5.5m.
BURIAL = .0012


def bearing_field(index, lod):
    solid = Solid('V5_broad_tapered_bearing_field_%02d'%index, 'Hull')
    solid.normals, solid.materials = [], []
    steps = ARC_STEPS[lod]
    angle = index*2*math.pi/8+.10
    rows, bottoms, tops = [], [], []
    lower_z = [underside_z(r)-depth for r, _, depth in STATIONS]
    upper_z = [underside_z(r)+BURIAL for r, _, _ in STATIONS]

    def point(r, a, z):
        return (r*math.cos(a), r*math.sin(a), z)

    def slope(values, k):
        edges = [j for j in (k-1, k) if 0 <= j < len(STATIONS)-1]
        return sum((values[j+1]-values[j])/(STATIONS[j+1][0]-STATIONS[j][0])
                   for j in edges)/len(edges)

    def curved_normal(p, k, top=False):
        a = math.atan2(p[1], p[0])
        radial = slope(upper_z if top else lower_z, k)
        return unit((-radial*math.cos(a), -radial*math.sin(a), 1) if top
                    else (radial*math.cos(a), radial*math.sin(a), -1))

    def face(points, material, normals=None):
        solid.face(points)
        for k in range(1, len(points)-1):
            if normals is None:
                a, b, c = points[0], points[k], points[k+1]
                normal = unit(cross(tuple(b[j]-a[j] for j in range(3)),
                                    tuple(c[j]-a[j] for j in range(3))))
                solid.normals.append((normal, normal, normal))
            else:
                solid.normals.append((normals[0], normals[k], normals[k+1]))
            solid.materials.append('M_S47_Final'+material)

    for k, (r, half_width, depth) in enumerate(STATIONS):
        inner_width = half_width-BEVEL_RUN/r
        assert inner_width > 0
        bottom = [point(r, angle+inner_width*(2*i/steps-1), lower_z[k]) for i in range(steps+1)]
        top = [point(r, angle+half_width*(2*i/steps-1), upper_z[k]) for i in range(steps+1)]
        shoulder_z = underside_z(r)-depth*.45
        left = point(r, angle-half_width, shoulder_z)
        right = point(r, angle+half_width, shoulder_z)
        # Counterclockwise cross-section when viewed along the outward radial
        # direction: broad bottom, right bevel/wall, buried top, left wall/bevel.
        rows.append(bottom+[right]+top[::-1]+[left])
        bottoms.append(bottom)
        tops.append(top)

    width = len(rows[0])
    for k in range(len(rows)-1):
        for j in range(width):
            next_j = (j+1)%width
            points = [rows[k][j], rows[k][next_j], rows[k+1][next_j], rows[k+1][j]]
            normals = None
            if j < steps:  # Broad continuous curved underside, most of the visible field.
                role = 'Hull'
                normals = [curved_normal(p, row) for p, row in zip(points, (k,k,k+1,k+1))]
            elif j == steps:
                role = 'Ribs'  # One long grazing bevel, not a second ring of lights.
            elif j in (steps+1, 2*steps+2):
                role = 'Recess'  # Dark side joints return into the original hull.
            elif steps+2 <= j < 2*steps+2:
                role = 'Hull'
                normals = [curved_normal(p, row, True) for p, row in zip(points, (k,k,k+1,k+1))]
            else:
                role = 'Hull'  # The opposite broad bevel stays in the hull finish.
            face(points, role, normals)

    # Strip-wise end closures avoid a fan across a concave curved cross-section.
    for k, reverse in ((0, True), (len(rows)-1, False)):
        for j in range(steps):
            points = [bottoms[k][j], bottoms[k][j+1], tops[k][j+1], tops[k][j]]
            face(points[::-1] if reverse else points, 'Recess')
        left = [bottoms[k][0], tops[k][0], rows[k][-1]]
        right = [bottoms[k][-1], rows[k][steps+1], tops[k][-1]]
        face(left[::-1] if reverse else left, 'Recess')
        face(right[::-1] if reverse else right, 'Recess')
    return quiet_uv(solid)


def build(lod):
    out = []
    for solid in build_v4(lod):
        if solid.name.startswith('V4_subdued_radial_web_'):
            continue
        solid.name = solid.name.replace('V4_', 'V5_', 1)
        out.append(solid)
    out.extend(bearing_field(index, lod) for index in range(8))
    for solid in out:
        assert solid.volume() > 0, (solid.name, 'closed solid inward/degenerate', solid.volume())
        assert len(solid.triangles) == len(solid.uv) == len(solid.normals)
        assert not hasattr(solid, 'materials') or len(solid.materials) == len(solid.triangles)
    assert sum(len(s.triangles) for s in out) < 40000
    return out


make = build
