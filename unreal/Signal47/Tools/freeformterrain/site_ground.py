#!/usr/bin/env python3
"""CALICHE-GROUND: the sites' pads, banks and cuts in Freeform's road ground (drawn mesh and collision mesh).

The game says where a site wants the land level and where it wants it gone (S47GroundEdits; run it with
-S47GroundEditsDump and it writes Saved/S47GroundEdits/site_ground.json). This puts the same edits into
mesh/SM_FF_Ground_Road.glb and mesh/SM_FF_Ground_Road_Col.glb, always from the untouched meshes kept beside
them as *.base.glb (made on the first run), so it can be run again with another list:

- round each pad and its bank the triangles are bisected (build_mesh.refine) to about a metre,
- every vertex there goes to lerp(land, pad height, 1 - smoothstep(distance outside the pad / bank)),
  the rule S47GroundEdits::Apply has,
- steep triangles there (the tufts drawn into the mesh) are dropped,
- each cut is clipped out exactly along its rectangle: a hole.

Then: run_import.py --root <freeformterrain> --apply --only Road, and copy the list to
Content/FreeformTerrain/site_ground.json (what S47GroundEdits::Baked reads). Plain NumPy.

    python3 site_ground.py --root <freeformterrain> --edits <site_ground.json> [--baked <Content/FreeformTerrain/site_ground.json>]
"""
import argparse
import json
import shutil
import struct
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_mesh as BM

L_MAX = 1.0      # longest edge on a pad and its bank, metres
GROW = 1.5       # refined this far past the bank


def read_glb(path):
    b = Path(path).read_bytes()
    size = struct.unpack_from('<I', b, 12)[0]
    j = json.loads(b[20:20 + size])
    binary = b[20 + size + 8:]
    p = j['meshes'][0]['primitives'][0]

    def acc(ai):
        a = j['accessors'][ai]
        v = j['bufferViews'][a['bufferView']]
        t = {5126: 'f4', 5125: 'u4', 5123: 'u2'}[a['componentType']]
        n = {'VEC3': 3, 'SCALAR': 1, 'VEC2': 2, 'VEC4': 4}[a['type']]
        return np.frombuffer(binary, '<' + t, a['count'] * n, v.get('byteOffset', 0) + a.get('byteOffset', 0)).reshape(-1, n).astype(np.float64)
    at = p['attributes']
    return (j['meshes'][0]['name'], acc(at['POSITION']), acc(at['NORMAL']), acc(at['TEXCOORD_0']), acc(at['TEXCOORD_1']), acc(at['COLOR_0']),
            acc(p['indices']).ravel().astype(np.int64).reshape(-1, 3))


def local(r, x, z):
    a = np.radians(r.get('yaw', 0.0))
    c, s = np.cos(a), np.sin(a)
    dx, dz = x - r['x'], z - r['z']
    return dx * c + dz * s, -dx * s + dz * c


def outside(r, x, z):
    u, v = local(r, x, z)
    du = np.maximum(0.0, np.abs(u) - r['hx'])
    dv = np.maximum(0.0, np.abs(v) - r['hz'])
    return np.sqrt(du * du + dv * dv)


def smooth(t):
    t = np.clip(t, 0, 1)
    return t * t * (3 - 2 * t)


def clip_hole(A, T, r, report):
    """Triangles minus the rectangle r. A: vertex rows (x, y, z, attributes...). Returns A, T."""
    u, v = local(r, A[:, 0], A[:, 2])
    tu, tv = u[T], v[T]
    hit = (tu.max(1) > -r['hx']) & (tu.min(1) < r['hx']) & (tv.max(1) > -r['hz']) & (tv.min(1) < r['hz'])
    rows = [A]
    n = len(A)
    extra = []          # new vertex rows
    coords = {}         # index -> (u, v) for new vertices
    made = {}           # (lo, hi, axis, value) -> index: neighbours share the point on their common edge
    U = u.tolist()
    V = v.tolist()

    def uv(i):
        return (U[i], V[i]) if i < n else coords[i]

    def cross(i, k, axis, value):
        lo, hi = (i, k) if i < k else (k, i)
        key = (lo, hi, axis, value)
        if key in made:
            return made[key]
        a, b = uv(lo)[axis], uv(hi)[axis]
        t = (value - a) / (b - a)
        ra = A[lo] if lo < n else extra[lo - n]
        rb = A[hi] if hi < n else extra[hi - n]
        row = ra + (rb - ra) * t
        idx = n + len(extra)
        extra.append(row)
        pa, pb = uv(lo), uv(hi)
        c = [pa[0] + (pb[0] - pa[0]) * t, pa[1] + (pb[1] - pa[1]) * t]
        c[axis] = value
        coords[idx] = tuple(c)
        made[key] = idx
        return idx

    def split(poly, axis, value):
        """(part below value, part at or above it), each a convex polygon or []."""
        lo, hi = [], []
        m = len(poly)
        for q in range(m):
            i, k = poly[q], poly[(q + 1) % m]
            di, dk = uv(i)[axis] - value, uv(k)[axis] - value
            if di < 0:
                lo.append(i)
            elif di > 0:
                hi.append(i)
            else:
                lo.append(i)
                hi.append(i)
            if (di < 0 and dk > 0) or (di > 0 and dk < 0):
                c = cross(i, k, axis, value)
                lo.append(c)
                hi.append(c)
        return (lo if len(lo) >= 3 else []), (hi if len(hi) >= 3 else [])

    out = []
    for tri in T[hit].tolist():
        keep = []
        rest = tri
        a, rest = split(rest, 0, -r['hx'])
        if a:
            keep.append(a)
        if rest:
            rest, a = split(rest, 0, r['hx'])
            if a:
                keep.append(a)
        if rest:
            a, rest = split(rest, 1, -r['hz'])
            if a:
                keep.append(a)
        if rest:
            rest, a = split(rest, 1, r['hz'])
            if a:
                keep.append(a)
        for poly in keep:
            for q in range(1, len(poly) - 1):
                p0, p1, p2 = uv(poly[0]), uv(poly[q]), uv(poly[q + 1])
                area = abs((p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1])) * .5
                if area > 1e-9:
                    out.append([poly[0], poly[q], poly[q + 1]])
    report.append({'triangles_touching_the_cut': int(hit.sum()), 'pieces_kept': len(out), 'new_vertices': len(extra)})
    if extra:
        A = np.concatenate([A, np.array(extra)])
    T = np.concatenate([T[~hit], np.array(out, np.int64).reshape(-1, 3)])
    return A, T


def edit(src, dst, sites, report):
    name, P, N, UV0, UV1, C, T = read_glb(src)
    rep = report.setdefault(name, {'source_triangles': int(len(T)), 'source_vertices': int(len(P)), 'cuts': []})
    pads = [p for s in sites.values() for p in s.get('pads', [])]
    cuts = [c for s in sites.values() for c in s.get('cuts', [])]
    x, z = P[:, 0], P[:, 2]
    # how far each point is inside some pad's reach: 0 on the pad, 1 at the end of its bank and of the refined ring
    def reach(xx, zz, grow):
        d = np.full(len(xx), np.inf)
        for p in pads:
            d = np.minimum(d, outside(p, xx, zz) - p['bank'] - grow)
        return d
    # 1. the tufts and other steep bits on the pads and banks go
    near_v = reach(x, z, GROW) < 0
    tri_near = near_v[T].any(axis=1)
    fn = np.cross(P[T[:, 1]] - P[T[:, 0]], P[T[:, 2]] - P[T[:, 0]])
    up = np.abs(fn[:, 1]) / np.maximum(np.linalg.norm(fn, axis=1), 1e-12)
    cx, cz = x[T].mean(axis=1), z[T].mean(axis=1)
    steep = tri_near & (up < .6) & (reach(cx, cz, 0) < 0)
    rep['steep_triangles_dropped'] = int(steep.sum())
    T = T[~steep]
    tri_near = tri_near[~steep]
    # 2. bisect to about a metre there
    V = {'x': P[:, 0].copy(), 'z': P[:, 2].copy(), 'y': P[:, 1].copy(), 'n': N.copy(), 'uv0': UV0.copy(), 'uv1': UV1.copy(), 'col': C.copy()}

    def want(xx, zz, a, b, c, longest):
        mx, mz = (xx[a] + xx[b] + xx[c]) / 3, (zz[a] + zz[b] + zz[c]) / 3
        return (longest > L_MAX) & (reach(mx, mz, GROW) < 0)
    sub = T[tri_near]
    V, sub, _ = BM.refine(V, sub, want)
    T = np.concatenate([T[~tri_near], sub])
    rep['triangles_after_refine'] = int(len(T))
    # 3. heights: S47GroundEdits::Apply's rule, pad after pad
    x, z, y = V['x'], V['z'], V['y']
    moved = np.zeros(len(x), bool)
    most = 0.0
    for p in pads:
        d = outside(p, x, z)
        m = d < p['bank']
        w = 1 - smooth(d[m] / p['bank']) if p['bank'] > 0 else np.ones(int(m.sum()))
        dy = (p['y'] - y[m]) * w
        most = max(most, float(np.abs(dy).max()) if len(dy) else 0.0)
        y[m] += dy
        moved |= m
    rep['vertices_moved'] = int(moved.sum())
    rep['largest_move_m'] = most
    # 4. normals again where the land moved
    Pn = np.stack([x, y, z], axis=1)
    touch = moved[T].any(axis=1)
    Tn = T[touch]
    fn = np.cross(Pn[Tn[:, 1]] - Pn[Tn[:, 0]], Pn[Tn[:, 2]] - Pn[Tn[:, 0]])
    if (fn[:, 1] < 0).mean() > .5:
        fn = -fn
    acc = np.zeros_like(Pn)
    for k in range(3):
        for col in range(3):
            acc[:, col] += np.bincount(Tn[:, k], fn[:, col], minlength=len(Pn))
    acc /= np.maximum(np.linalg.norm(acc, axis=1, keepdims=True), 1e-12)
    V['n'][moved] = acc[moved]
    # 5. the cuts: holes, exactly along their rectangles
    A = np.concatenate([Pn, V['n'], V['uv0'], V['uv1'], V['col']], axis=1)
    for c in cuts:
        A, T = clip_hole(A, T, c, rep['cuts'])
    nn = A[:, 3:6]
    nn /= np.maximum(np.linalg.norm(nn, axis=1, keepdims=True), 1e-12)
    # unused vertices out
    used = np.zeros(len(A), bool)
    used[T.ravel()] = True
    remap = np.cumsum(used) - 1
    A = A[used]
    T = remap[T]
    rep['triangles'] = int(len(T))
    rep['vertices'] = int(len(A))
    # checks: nothing left inside a cut, the pads level
    cx, cz = A[:, 0][T].mean(axis=1), A[:, 2][T].mean(axis=1)
    inside = 0
    for c in cuts:
        u, v = local(c, cx, cz)
        inside += int(((np.abs(u) < c['hx'] - 1e-6) & (np.abs(v) < c['hz'] - 1e-6)).sum())
    rep['triangles_left_inside_cuts'] = inside
    worst = 0.0
    for p in pads:
        m = outside(p, A[:, 0], A[:, 2]) <= 0
        # (a later pad may overlap an earlier one's bank: only same-height pads are checked against their own height)
        if m.any():
            worst = max(worst, float(np.abs(A[m, 1] - p['y']).max()))
    rep['pads_worst_off_level_m'] = worst
    if inside:
        raise RuntimeError('%s: %d triangles left inside the cuts' % (name, inside))
    BM.write_glb(dst, name, A[:, 0:3], A[:, 3:6], A[:, 6:8], A[:, 8:10], A[:, 10:14], T)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--edits', type=Path, required=True)
    ap.add_argument('--baked', type=Path, help='where to write the list the installed mesh holds (after a good import)')
    args = ap.parse_args()
    data = json.loads(args.edits.read_text())
    sites = data['sites']
    report = {'edits': str(args.edits), 'sites': {k: {'pads': len(v.get('pads', [])), 'cuts': len(v.get('cuts', []))} for k, v in sites.items()}}
    for name in ('SM_FF_Ground_Road', 'SM_FF_Ground_Road_Col'):
        dst = args.root / 'mesh' / (name + '.glb')
        base = args.root / 'mesh' / (name + '.base.glb')
        if not base.exists():
            shutil.copy2(dst, base)
        BM.log(name)
        edit(base, dst, sites, report)
    out = args.root / 'report' / 'site_ground.json'
    out.write_text(json.dumps(report, indent=1) + '\n')
    print(json.dumps(report, indent=1))
    if args.baked:
        args.baked.write_text(json.dumps(data, indent=1) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
