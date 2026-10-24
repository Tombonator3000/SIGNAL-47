"""Unchanged Caliche road heights and exclusions for authored nature, in metres.

The old 70 m authoring helper supplies the canonical analytic functions, but
its uniform grid assumption is intentionally NOT extended beyond x=60 m.
The full native axis, float32 samples and triangle split are used here.
"""
import bisect
from functools import lru_cache
import importlib.util
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT.parents[1] / 'calichestreet/ground/terrain.py'
spec = importlib.util.spec_from_file_location('s47_original_caliche_terrain', SOURCE)
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)
ANCHOR = (8000.0, 0.0, 2060.0)
REGION = (-600.0, 600.0, 1650.0, 2450.0)


def axis(start, end, step, low, high):
    middle = []
    value = start
    while value <= end + 1e-6:
        middle.append(value)
        value += step
    left, right = [], []
    for output, origin, boundary, sign in ((left, start, low, -1), (right, end, high, 1)):
        distance, value = step, origin
        while (value > boundary if sign < 0 else value < boundary):
            distance = min(120.0, distance * 1.25)
            value = max(boundary, value - distance) if sign < 0 else min(boundary, value + distance)
            output.append(value)
    return list(reversed(left)) + middle + right


GX = axis(-800.0, 60.0, 10.0, -3000.0, 2300.0)
GZ = axis(-480.0, 2760.0, 10.0, -2700.0, 4400.0)


def grid_y(x, z):
    i = min(len(GX) - 2, max(0, bisect.bisect_right(GX, x) - 1))
    j = min(len(GZ) - 2, max(0, bisect.bisect_right(GZ, z) - 1))
    a, b, c, d = [old.grid_h(a, b) for a, b in
                  ((GX[i], GZ[j]), (GX[i+1], GZ[j]), (GX[i], GZ[j+1]), (GX[i+1], GZ[j+1]))]
    u = max(0.0, min(1.0, (x - GX[i]) / (GX[i+1] - GX[i])))
    v = max(0.0, min(1.0, (z - GZ[j]) / (GZ[j+1] - GZ[j])))
    return a + (b-a)*u + (c-a)*v if u+v <= 1 else d + (c-d)*(1-u) + (b-d)*(1-v)


def floor_y(x, z):
    assert REGION[0] <= x <= REGION[1] and REGION[2] <= z <= REGION[3], (x, z)
    ax = abs(x)
    low = old.hlow(x, z)
    if ax <= 6:
        return low + (.06 + .06*(1-(ax/4.8)**2) if ax <= 4.8 else .06*(6-ax)/1.2)
    distance = old.old_near(x, z)
    if distance <= 4.4:
        blend = old.smooth(18, 26, x)
        base = low + (grid_y(x, z)-low)*blend
        lift = .1+.04*(1-(distance/3.2)**2) if distance <= 3.2 else .1*(4.4-distance)/1.2
        return base + lift
    y = grid_y(x, z)
    return max(y, low+(0 if ax <= 16.5 else -.6*(ax-16.5)/1.5)) if ax <= 18 else y


def box(x, z, limits, pad=0):
    return limits[0]-pad <= x <= limits[1]+pad and limits[2]-pad <= z <= limits[3]+pad


# Entire original town pedestrian/parking band, diner forecourt and all
# original story/proxy/photo positions are excluded from upright vegetation.
# The accepted 70 m street terrain also remains untouched at its centre.
TOWN = (-42.0, 48.0, 2017.95, 2300.0)
DINER = (-49.0, 22.0, 1965.0, 2024.0)
DEPUTY = (-15.0, 18.0, 1645.0, 1740.0)
STREET = (-30.0, 40.0, 2020.0, 2090.0)
TOWN_WALK = (-18.3, 20.9, 2012.0, 2260.0)
BUILDINGS = ((-28.2,-16.4,2065.5,2088.7), (-27,-15,2099,2113),
             (-26,-14,2125,2141), (-30,-14,2151,2173), (-32,-14,2189,2215),
             (18.6,31,2029,2043), (17.9,30.3,2057.3,2070.7), (20,36,2084,2102), (23,34,2127,2139), (23.5,33,2151,2162),
             (24,35,2179,2191), (-29,-18,2235,2247))
KEEP_CLEAR = (TOWN_WALK, DINER, DEPUTY) + BUILDINGS


@lru_cache(maxsize=150000)
def old_distance(x, z):
    # The small authoring helper and native SurfaceY optimize away points
    # outside a 120 m Z strip. Exclusions must protect the actual full curve.
    if x < -20 or x > 660 or z < 1800 or z > 2020:
        return math.inf
    result = math.inf
    for (ax, az), (bx, bz) in zip(old.OLD, old.OLD[1:]):
        dx, dz = bx-ax, bz-az
        t = max(0.0, min(1.0, ((x-ax)*dx+(z-az)*dz)/(dx*dx+dz*dz)))
        result = min(result, math.hypot(x-ax-t*dx, z-az-t*dz))
    return result


def dressing_allowed(x, z, radius=0.0):
    if abs(x) < 7.0+radius or old_distance(x, z) < 17.0+radius:
        return False
    return not any(box(x, z, limits, radius) for limits in KEEP_CLEAR)


def terrain_allowed(x, z):
    # Original town sidewalks/apron remain visible and retain their own height.
    if 2017.95 <= z <= 2300 and ((-17.6 <= x <= -14.2) or (16.6 <= x <= 20.2)):
        return False
    return (abs(x) > 5.0 and old_distance(x, z) > 5.0
            and not any(box(x, z, limits, .2) for limits in (STREET, DINER))
            and not any(box(x, z, limits, .2) for limits in BUILDINGS))


def visual_y(x, z):
    """Town's drawn paving has a small offset above its analytic walker floor."""
    y=floor_y(x,z)
    if box(x,z,TOWN):
        if 2040 < z < 2090 and -17.6 < x < -4.72:
            y += .055 if x <= -5.6 else .01+.045*(-4.72-x)/.88
        elif x < -14.2 or x > 16.6:
            y += .05
    return y


def smooth(a, b, x):
    return old.smooth(a, b, x)


def patch_field(x, z):
    """Continuous broad density fields; never a visible random grid."""
    broad = .50 + .22*math.sin(x*.024+math.sin(z*.013)*1.4) + .15*math.sin(z*.039-x*.011)
    detail = .12*math.sin(x*.17+z*.093)*math.sin(z*.19-x*.07)
    return max(.03, min(.97, broad+detail))


def blender_point(x, z, height):
    return (x, -(z-ANCHOR[2]), height)
