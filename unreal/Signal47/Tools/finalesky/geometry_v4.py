"""Original v4 craft: one broad grazing facet, dark structural setbacks.

The v3 lens envelope, optical well and eight running lights are retained. The
outer collar is one closed solid: only its broad outward facet uses Ribs; the
returning ledges use Recess. Authored corner normals round the circumference
without blending the collar's deliberate profile breaks. Ordinary Python only.
"""
import math

from geometry import Solid, cross
from geometry_v2 import PROFILE, dished_wedge, underside_z


SEGMENTS = (384, 192, 96)
COLLAR_PROFILE = [
    (-.070, .815), (-.070, .887), (-.062, .916), (-.028, .942),
    (-.022, .928), (-.010, .925), (-.026, .810),
]
COLLAR_MATERIALS = ('Recess', 'Hull', 'Ribs', 'Recess', 'Recess', 'Hull', 'Recess')


def unit(vector):
    length = math.sqrt(sum(value*value for value in vector))
    assert length > 0
    return tuple(value/length for value in vector)


def quiet_uv(solid):
    # Match v3's four large circumferential panel intervals.
    solid.uv = [tuple((.017+u/24, v) for u, v in face) for face in solid.uv]
    return solid


def turned(name, profile, segments, materials, closed=False, smooth_profile=False):
    """A lathed solid with analytic arc normals and explicit profile-edge slots."""
    count = len(profile) if closed else len(profile)-1
    roles = [materials]*count if isinstance(materials, str) else list(materials)
    assert len(roles) == count
    solid = Solid(name, roles[0])
    solid.normals, solid.materials = [], []
    angles = [2*math.pi*i/segments for i in range(segments)]
    points = [[(r*math.cos(a), r*math.sin(a), z) for a in angles] for z, r in profile]
    # For the authored outward winding, (dz, -dr) is the radial/Z normal.
    edge_normals = [unit((profile[(k+1)%len(profile)][0]-profile[k][0],
                          profile[k][1]-profile[(k+1)%len(profile)][1]))
                    for k in range(count)]

    def normal(k, angle, edge):
        radial, vertical = edge_normals[edge]
        if smooth_profile and (closed or 0 < k < len(profile)-1):
            before, after = edge_normals[(k-1)%count], edge_normals[k%count]
            radial, vertical = unit((before[0]+after[0], before[1]+after[1]))
        return (radial*math.cos(angle), radial*math.sin(angle), vertical)

    def face(vertices, normals, role, uv=None):
        solid.face(vertices, uv)
        for k in range(1, len(vertices)-1):
            solid.normals.append((normals[0], normals[k], normals[k+1]))
            solid.materials.append('M_S47_Final'+role)

    for k in range(count):
        next_ring = (k+1)%len(profile)
        for i, angle in enumerate(angles):
            j = (i+1)%segments
            face([points[k][i], points[k][j], points[next_ring][j], points[next_ring][i]],
                 [normal(k, angle, k), normal(k, angles[j], k),
                  normal(next_ring, angles[j], k), normal(next_ring, angle, k)], roles[k],
                 [(i/segments, k/count), ((i+1)/segments, k/count),
                  ((i+1)/segments, (k+1)/count), (i/segments, (k+1)/count)])
    if not closed:
        face(points[0][::-1], [(0, 0, -1)]*segments, roles[0])
        face(points[-1], [(0, 0, 1)]*segments, roles[-1])
    return solid


def flat_normals(solid):
    """Small webs/apertures retain their deliberately flat casing faces."""
    solid.normals = []
    for triangle in solid.triangles:
        a, b, c = (solid.vertices[index] for index in triangle)
        normal = unit(cross(tuple(b[k]-a[k] for k in range(3)),
                            tuple(c[k]-a[k] for k in range(3))))
        solid.normals.append((normal, normal, normal))
    return solid


def build(lod):
    n = SEGMENTS[lod]
    out = [quiet_uv(turned('V4_continuous_dished_hull', PROFILE, n, 'Hull', smooth_profile=True))]
    # One 34m-high sloping facet at scale1000 replaces v3's overlapping bright
    # wall/bevel skin. The lower land and returning upper steps stay dark.
    out.append(quiet_uv(turned('V4_single_grazing_structural_collar',
                               COLLAR_PROFILE, n, COLLAR_MATERIALS, closed=True)))
    z = underside_z(.313)
    out.append(quiet_uv(turned('V4_quiet_inner_optical_collar', [
        (z-.0038, .301), (z-.0027, .325), (z+.002, .325), (z+.003, .301)], n, 'Hull')))
    out.append(quiet_uv(turned('V4_recessed_equatorial_belt', [
        (-.0136, .982), (-.0113, .990), (-.0056, .997), (-.0041, .994)], n, 'Recess', closed=True)))

    for i in range(8):
        a = i*2*math.pi/8+.10
        out.append(quiet_uv(flat_normals(dished_wedge('V4_subdued_radial_web_%02d'%i,
            (.39, .53, .65), a+.027, a+.037, .0014, .0008, 'Hull', 3 if lod == 0 else 2))))
    for i in range(8):
        a = i*2*math.pi/8+.16
        out.append(flat_normals(dished_wedge('V4_running_light_well_%02d'%i,
            (.978, .984, .990), a, a+.012, .0020, .0012, 'Recess', 2)))
        out.append(flat_normals(dished_wedge('V4_running_light_%02d'%i,
            (.983, .988), a+.0036, a+.0084, .0027, -.0018, 'Light', 2)))
    out.append(turned('V4_central_dark_optical_well', [
        (-.096, .04), (-.094, .13), (-.089, .14), (-.087, .04)], max(32, n//2), 'Recess'))
    out.append(turned('V4_central_dim_lens', [
        (-.097, .021), (-.096, .032), (-.094, .032)], max(24, n//2), 'Light'))
    for solid in out:
        assert solid.volume() > 0, (solid.name, 'closed solid inward/degenerate', solid.volume())
        assert len(solid.triangles) == len(solid.uv) == len(solid.normals)
        assert not hasattr(solid, 'materials') or len(solid.materials) == len(solid.triangles)
    return out


make = build
