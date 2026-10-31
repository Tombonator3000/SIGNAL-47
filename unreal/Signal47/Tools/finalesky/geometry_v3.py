"""Original v3 craft: a deep structural collar and a quiet dished underside.

The lens silhouette, optical well and eight running lights follow v2. A closed
annular collar adds actual depth without filling its central void. At scale1000
its outer wall is26m high, with an8m grazing bevel and a dark inner setback.
"""
import math

from geometry import Solid, lathe
from geometry_v2 import PROFILE, dished_wedge, underside_z


def quiet_uv(solid):
    # The shared material has a dense circumferential seam pattern. Map only
    # four large panel intervals over v3's metal, rather than scores of spokes.
    solid.uv = [tuple((.017+u/24,v) for u,v in face) for face in solid.uv]
    return solid


def ring(name, profile, segments, material):
    """Closed annulus, profile ordered bottom-inner -> outer -> top -> inner."""
    solid = Solid(name,material)
    points = [[(r*math.cos(2*math.pi*i/segments),r*math.sin(2*math.pi*i/segments),z)
               for i in range(segments)] for z,r in profile]
    for k in range(len(profile)):
        next_ring = (k+1)%len(profile)
        for i in range(segments):
            j = (i+1)%segments
            solid.face([points[k][i],points[k][j],points[next_ring][j],points[next_ring][i]],
                       [(i/segments,k/len(profile)),((i+1)/segments,k/len(profile)),
                        ((i+1)/segments,(k+1)/len(profile)),(i/segments,(k+1)/len(profile))])
    return quiet_uv(solid)


def build(lod):
    n = (192,112,64)[lod]
    out = [quiet_uv(lathe('V3_continuous_dished_hull',PROFILE,n,'Hull'))]
    # The upper face is buried in the existing hull. The exposed stepped shell
    # has a broad lower land, sloped wall and bevel, instead of thin bright rings.
    out.append(ring('V3_deep_structural_outer_collar',[
        (-.070,.815),(-.070,.887),(-.062,.916),(-.036,.930),
        (-.028,.942),(-.010,.925),(-.026,.810)],n,'Hull'))
    # A thin, intersecting metal skin follows the wall and its8m bevel. It gives
    # the sunset one broad curved surface to describe, using the existing Ribs MI.
    out.append(ring('V3_broad_grazing_wall_and_bevel',[
        (-.0618,.9158),(-.0624,.9166),(-.0364,.9306),
        (-.0284,.9426),(-.0278,.9418),(-.0358,.9298)],n,'Ribs'))
    out.append(ring('V3_dark_reentrant_inner_setback',[
        (-.067,.8139),(-.067,.8154),(-.034,.8115),(-.034,.8100)],n,'Recess'))

    z = underside_z(.313)
    out.append(quiet_uv(lathe('V3_quiet_inner_optical_collar',[
        (z-.0038,.301),(z-.0027,.325),(z+.002,.325),(z+.003,.301)],n,'Hull')))
    out.append(ring('V3_recessed_equatorial_belt',[
        (-.0136,.982),(-.0113,.990),(-.0056,.997),(-.0041,.994)],n,'Recess'))

    # Eight shallower, short webs share the hull colour. The radial decoration
    # no longer competes with the broad structural collar.
    for i in range(8):
        a = i*2*math.pi/8+.10
        out.append(quiet_uv(dished_wedge('V3_subdued_radial_web_%02d'%i,
            (.39,.53,.65),a+.027,a+.037,.0014,.0008,'Hull',3 if lod == 0 else 2)))
    for i in range(8):
        a = i*2*math.pi/8+.16
        out.append(dished_wedge('V3_running_light_well_%02d'%i,
            (.978,.984,.990),a,a+.012,.0020,.0012,'Recess',2))
        out.append(dished_wedge('V3_running_light_%02d'%i,
            (.983,.988),a+.0036,a+.0084,.0027,-.0018,'Light',2))
    out.append(lathe('V3_central_dark_optical_well',[
        (-.096,.04),(-.094,.13),(-.089,.14),(-.087,.04)],max(32,n//2),'Recess'))
    out.append(lathe('V3_central_dim_lens',[
        (-.097,.021),(-.096,.032),(-.094,.032)],max(24,n//2),'Light'))
    for solid in out:
        assert solid.volume() > 0,(solid.name,'closed solid inward/degenerate',solid.volume())
        assert len(solid.triangles) == len(solid.uv)
    return out


make = build
