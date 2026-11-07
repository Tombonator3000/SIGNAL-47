"""What must stay where it is: the contract's masks, road-local metres. W = 0 in them, then a smooth overgang."""
import numpy as np

import base_port as B

SARO_IN_ROAD = (24.0, -300.0)     # road-local = site + this
ARRAY_FIELD = (-245.0, 225.0, -435.0, -21.0)  # site terms: S47Legs.h
RANCH_TO = 405.0


def smooth(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def outside(rect, x, z):
    ax = np.maximum(np.maximum(rect[0] - x, x - rect[1]), 0)
    az = np.maximum(np.maximum(rect[2] - z, z - rect[3]), 0)
    return np.hypot(ax, az)


def ranch_z(x):
    return 196 + (x - 15) * 0.012 + np.sin(x * 0.021) * 1.2


def _rect(name, rect, zero, over):
    return (name, 'rect', rect, zero, over)


def _sar(r):
    return (r[0] + SARO_IN_ROAD[0], r[1] + SARO_IN_ROAD[0], r[2] + SARO_IN_ROAD[1], r[3] + SARO_IN_ROAD[1])


RECTS = [
    # name, rectangle in road terms, grown by this before the overgang, overgang length
    # S47Relief.cpp CheckZones: exactly its rectangles, at 0, with the same overganger outside them
    ('farm', (20, 201, 130, 290), 0, 60),
    ('saro_patch', (-120, 120, -200, 600), 0, 80),
    ('array', _sar((-290, 270, -515, 0)), 0, 80),      # the graded field: S47GroundCover ARRAY_FLAT, in SARO's terms
    ('old_ranch', (385, 490, 135, 240), 0, 40),        # g910-g912 stand fixed here (S47HighwayRanch), away from the road's own plot
    ('pasture', (-760, -21, 440, 720), 0, 60),
    ('station_patch', (-592, -190, 430, 610), 0, 60),
    ('diner_town', (-60, 60, 1960, 2310), 0, 80),
]
CIRCLES = [
    ('saro_site', SARO_IN_ROAD, 340, 80),
    ('motel', (B.MOTEL_FLATX, B.MOTEL_Z), 110, 80),
]
EXTRA = []  # (name, rect, grow, overgang): things found standing outside the contract's list


def _bbox_mask(x, z, x0, x1, z0, z1):
    return (x >= x0) & (x <= x1) & (z >= z0) & (z <= z1)


def weight(x, z, margin=8.0):
    """W(x, z) in 0..1, and the null area. Every list item is held at exactly 0 out to `margin` past
    its edge, so that a bilinear Off from a 4 m map is exactly 0 throughout the contract's own area."""
    x = np.asarray(x, dtype=np.float64)
    z = np.asarray(z, dtype=np.float64)
    w = np.ones(x.shape, np.float32)
    parts = {}

    def apply(name, d, zero, over):
        nonlocal w
        s = smooth(zero + margin, zero + max(over, margin + 1), d).astype(np.float32)
        w = w * s
        parts[name] = {'zero_m': zero, 'overgang_m': over, 'share_zero': float((s <= 0).mean())}

    # highway: |x| <= 22, overgang over 90 m
    apply('highway', np.abs(x), 22, 90)
    # survey track
    d = np.full(x.shape, np.inf)
    m = _bbox_mask(x, z, -780, 170, 330, 700)
    d[m] = B.seg_dist(x[m], z[m], B.TRACK)
    apply('track', d, 15, 60)
    # the old road's first stretch
    d = np.full(x.shape, np.inf)
    bx0, bx1, bz0, bz1 = B.OLD[:, 0].min(), B.OLD[:, 0].max(), B.OLD[:, 1].min(), B.OLD[:, 1].max()
    m = _bbox_mask(x, z, bx0 - 80, bx1 + 80, bz0 - 80, bz1 + 80)
    d[m] = B.seg_dist(x[m], z[m], B.OLD)
    apply('old_road', d, 11, 40)
    # the farm road: x 6..470, +-8 of RanchZ
    xc = np.clip(x, 6, 470)
    d = np.hypot(x - xc, z - ranch_z(xc))
    apply('farm_road', d, 8, 40)
    for name, rect, grow, over in RECTS + EXTRA:
        apply(name, outside(rect, x, z), grow, over)
    for name, c, r, over in CIRCLES:
        apply(name, np.hypot(x - c[0], z - c[1]), r, over)
    return w, w <= 0, parts


def view_distance(x, z):
    """How far from where a player looks out: highway, farm and pasture. For the map placement."""
    d = np.abs(x) - 22
    for rect in ((20, 175, 140, 280), (-760, -21, 440, 720), (-60, 60, 1960, 2310)):
        d = np.minimum(d, outside(rect, x, z))
    return np.maximum(d, 0)
