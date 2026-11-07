#!/usr/bin/env python3
"""Freeform's two ground meshes as GLB: the exported ground refined where the relief needs it.

Plain NumPy; starts neither Blender nor Unreal. The refinement is longest-edge bisection (Rivara) of the
source triangles: conforming with no T-junctions, and a new point on a source edge lies exactly on the
source surface. Triangles with Off = 0 throughout are left as they are unless a neighbour's split reaches
them, and such a split changes the surface by nothing. So the old ground is kept to float precision where
Off = 0, split as before wherever possible. Every vertex is base + Off(x, z); the strips and tufts of g873
are kept as they are, with Off added.

    python3 build_mesh.py --root <freeformterrain> --web-geo <Signal47-web/geo> --trim-g4 <trim_relocated_geo_000.glb>
"""
import argparse
import json
import struct
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import base_port as B


def log(*a):
    print(*a, flush=True)


def read_glb_mesh(path, name):
    b = Path(path).read_bytes()
    size = struct.unpack_from('<I', b, 12)[0]
    j = json.loads(b[20:20 + size])
    binary = b[20 + size + 8:]
    mesh = next(m for m in j['meshes'] if m['name'] == name)
    assert len(mesh['primitives']) == 1
    p = mesh['primitives'][0]

    def acc(ai):
        a = j['accessors'][ai]
        v = j['bufferViews'][a['bufferView']]
        t = {5126: 'f4', 5125: 'u4', 5123: 'u2', 5121: 'u1'}[a['componentType']]
        n = {'VEC3': 3, 'SCALAR': 1, 'VEC2': 2, 'VEC4': 4}[a['type']]
        return np.frombuffer(binary, '<' + t, a['count'] * n, v.get('byteOffset', 0) + a.get('byteOffset', 0)).reshape(-1, n)
    return (acc(p['attributes']['POSITION']).astype(np.float64), acc(p['attributes']['NORMAL']).astype(np.float64),
            acc(p['attributes']['TEXCOORD_0']).astype(np.float64), acc(p['attributes']['COLOR_0']).astype(np.float64),
            acc(p['indices']).ravel().astype(np.int64).reshape(-1, 3))


def components(n, tris):
    parent = np.arange(n)

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    for a, b, c in tris.tolist():
        ra, rb, rc = find(a), find(b), find(c)
        parent[rb] = ra
        parent[rc] = ra
    return np.array([find(i) for i in range(n)])


class Field:
    def __init__(self, root):
        meta = json.loads((root / 'relief.json').read_text())
        self.x0, self.z0, self.cell = meta['origin_x'], meta['origin_z'], meta['cell_m']
        self.w, self.h = meta['width'], meta['height']
        self.scale = meta['scale_m']
        self.q = np.fromfile(root / 'relief.bin', dtype='<i2').reshape(self.h, self.w).astype(np.float64) * self.scale
        f = np.load(root / 'cache/fields.npz')
        self.sediment = f['sediment'].astype(np.float32) / 255
        self.stone = f['stone'].astype(np.float32) / 255
        self.dist = f['dist']
        self.weight = f['w']
        # SARO's mesh goes from the old ring to the floor by distance, not by the relief's weight: it lies on the floor
        # wherever the table is live, and the step between the two is taken inside the 8 m kept at 0 outside the masks.
        from bake_relief import jfa
        dist, _, _ = jfa(self.weight > 0, passes=4)
        t = np.clip((dist * self.cell - 4.5) / 3.5, 0, 1)
        self.to_floor = (1 - t * t * (3 - 2 * t)).astype(np.float32)

    def sample(self, a, x, z):
        """Off's own rule: bilinear in the cell, 0 (or a's edge) outside the map."""
        c = (np.asarray(x) - self.x0) / self.cell
        r = (np.asarray(z) - self.z0) / self.cell
        inside = (c >= 0) & (r >= 0) & (c <= self.w - 1) & (r <= self.h - 1)
        c = np.clip(c, 0, self.w - 1.000001)
        r = np.clip(r, 0, self.h - 1.000001)
        c0 = c.astype(np.int64)
        r0 = r.astype(np.int64)
        fc = c - c0
        fr = r - r0
        v = a[r0, c0] * (1 - fr) * (1 - fc) + a[r0, c0 + 1] * (1 - fr) * fc + a[r0 + 1, c0] * fr * (1 - fc) + a[r0 + 1, c0 + 1] * fr * fc
        return np.where(inside, v, 0.0)

    def off(self, x, z):
        return self.sample(self.q, x, z)


def edge_table(T, n):
    key = np.stack([np.minimum(T[:, [0, 1, 2]], T[:, [1, 2, 0]]), np.maximum(T[:, [0, 1, 2]], T[:, [1, 2, 0]])], axis=2)
    k = key[:, :, 0] * n + key[:, :, 1]
    u, inv = np.unique(k, return_inverse=True)
    return u, inv.reshape(-1, 3)


def refine(V, T, want):
    """Bisect by longest edge until `want` asks for no more. V: dict of float64 arrays per vertex
    (x, z, y and attributes), T: triangle indices, src: source triangle per triangle."""
    src = np.arange(len(T))
    for it in range(40):
        x, z = V['x'], V['z']
        n = len(x)
        a, b, c = T[:, 0], T[:, 1], T[:, 2]
        l0 = (x[a] - x[b]) ** 2 + (z[a] - z[b]) ** 2
        l1 = (x[b] - x[c]) ** 2 + (z[b] - z[c]) ** 2
        l2 = (x[c] - x[a]) ** 2 + (z[c] - z[a]) ** 2
        lens = np.stack([l0, l1, l2], axis=1)
        longest = np.argmax(np.round(lens, 4), axis=1)
        mark_tri = want(x, z, a, b, c, np.sqrt(lens.max(axis=1)))
        count = int(mark_tri.sum())
        if count == 0:
            log('  refine done at pass', it, 'triangles', len(T), 'vertices', n)
            break
        u, E = edge_table(T, n)
        edge_mark = np.zeros(len(u), bool)
        row = np.arange(len(T))
        edge_mark[E[row, longest][mark_tri]] = True
        while True:
            has = edge_mark[E].any(axis=1)
            target = E[row, longest][has]
            if edge_mark[target].all():
                break
            edge_mark[target] = True
        # new vertices at marked edge midpoints
        marked = np.nonzero(edge_mark)[0]
        ea = u[marked] // n
        eb = u[marked] % n
        new_id = np.full(len(u), -1, np.int64)
        new_id[marked] = n + np.arange(len(marked))
        for key in V:
            V[key] = np.concatenate([V[key], (V[key][ea] + V[key][eb]) * .5], axis=0)
        # rotate each triangle so that its longest edge is first (a, b); c opposite
        rot = longest
        i0 = np.stack([T[row, rot], T[row, (rot + 1) % 3], T[row, (rot + 2) % 3]], axis=1)
        m_ab = new_id[E[row, rot]]
        m_bc = new_id[E[row, (rot + 1) % 3]]
        m_ca = new_id[E[row, (rot + 2) % 3]]
        A, B_, C = i0[:, 0], i0[:, 1], i0[:, 2]
        keep = m_ab < 0
        out = [T[keep]]
        outs = [src[keep]]

        def emit(mask, *tris):
            for t in tris:
                out.append(np.stack([t[0][mask], t[1][mask], t[2][mask]], axis=1))
                outs.append(src[mask])
        only = (m_ab >= 0) & (m_bc < 0) & (m_ca < 0)
        emit(only, (A, m_ab, C), (m_ab, B_, C))
        bc = (m_ab >= 0) & (m_bc >= 0) & (m_ca < 0)
        emit(bc, (A, m_ab, C), (m_ab, B_, m_bc), (m_ab, m_bc, C))
        ca = (m_ab >= 0) & (m_bc < 0) & (m_ca >= 0)
        emit(ca, (A, m_ab, m_ca), (m_ca, m_ab, C), (m_ab, B_, C))
        all3 = (m_ab >= 0) & (m_bc >= 0) & (m_ca >= 0)
        emit(all3, (A, m_ab, m_ca), (m_ca, m_ab, C), (m_ab, B_, m_bc), (m_ab, m_bc, C))
        T = np.concatenate(out)
        src = np.concatenate(outs)
        log('  pass', it, 'marked', count, '-> triangles', len(T), 'vertices', len(V['x']))
    return V, T, src


def zone_table(d, values):
    return np.interp(d, [100, 350, 1000], values)


def make_want(field, to_road, l_max, tol, l_min, budget, land=None):
    dx, dz = to_road

    def want(x, z, a, b, c, longest):
        rx, rz = x + dx, z + dz
        cx = (rx[a] + rx[b] + rx[c]) / 3
        cz = (rz[a] + rz[b] + rz[c]) / 3
        fa, fb, fc = field.off(rx[a], rz[a]), field.off(rx[b], rz[b]), field.off(rx[c], rz[c])
        fm0 = field.off((rx[a] + rx[b]) / 2, (rz[a] + rz[b]) / 2)
        fm1 = field.off((rx[b] + rx[c]) / 2, (rz[b] + rz[c]) / 2)
        fm2 = field.off((rx[c] + rx[a]) / 2, (rz[c] + rz[a]) / 2)
        fce = field.off(cx, cz)
        any_off = (fa != 0) | (fb != 0) | (fc != 0) | (fm0 != 0) | (fm1 != 0) | (fm2 != 0) | (fce != 0)
        d = field.sample(field.dist.astype(np.float64), cx, cz)
        belt = np.zeros(len(a), bool)
        chord = 0.0
        if land is not None:
            # the band where the mesh leaves the old ring needs its own triangles, Off or no Off
            ka = field.sample(field.to_floor.astype(np.float64), rx[a], rz[a])
            kb = field.sample(field.to_floor.astype(np.float64), rx[b], rz[b])
            kc = field.sample(field.to_floor.astype(np.float64), rx[c], rz[c])
            kce = field.sample(field.to_floor.astype(np.float64), cx, cz)
            k_any = (ka > 0) | (kb > 0) | (kc > 0) | (kce > 0)
            any_off = any_off | k_any
            belt = k_any & ((np.minimum.reduce([ka, kb, kc]) < 1) | (kce < 1)) & (longest > 2.9)
            la, lb, lc = land(x[a], z[a]), land(x[b], z[b]), land(x[c], z[c])
            lm = land((x[a] + x[b]) / 2, (z[a] + z[b]) / 2)
            lm2 = land((x[b] + x[c]) / 2, (z[b] + z[c]) / 2)
            lm3 = land((x[c] + x[a]) / 2, (z[c] + z[a]) / 2)
            chord = np.maximum.reduce([np.abs(lm - (la + lb) * .5), np.abs(lm2 - (lb + lc) * .5), np.abs(lm3 - (lc + la) * .5)]) * k_any
        size = belt | (longest > np.interp(d, [100, 100.01, 350, 350.01, 1000, 1000.01], [l_max[0], l_max[1], l_max[1], l_max[2], l_max[2], l_max[3]]))
        err = np.maximum.reduce([np.abs(fm0 - (fa + fb) * .5), np.abs(fm1 - (fb + fc) * .5), np.abs(fm2 - (fc + fa) * .5), np.abs(fce - (fa + fb + fc) / 3)])
        err = err + chord
        rough = (err > np.interp(d, [100, 350, 1000], tol)) & (longest > np.interp(d, [100, 350, 1000], l_min))
        marked = any_off & (size | rough)
        # each mark adds about two triangles with what it drags along. Over budget: the worst first.
        room = max(0, budget - len(a)) // 2
        if marked.sum() > room:
            worst = np.where(marked, err / np.maximum(np.interp(d, [100, 350, 1000], tol), 1e-6) + size * .5, -1.0)
            if room == 0:
                return np.zeros(len(a), bool)
            cut = np.partition(worst, len(worst) - room)[len(worst) - room]
            marked = marked & (worst >= cut)
        return marked
    return want


def write_glb(path, name, pos, normal, uv0, uv1, color, indices):
    """Same shape as the exported geometry files: one mesh, one primitive, one material."""
    pos = pos.astype('<f4')
    normal = normal.astype('<f4')
    uv0 = uv0.astype('<f4')
    uv1 = uv1.astype('<f4')
    color = color.astype('<f4')
    indices = indices.astype('<u4').ravel()
    chunks, views, accessors = [], [], []
    offset = 0

    def add(arr, ctype, kind, target, with_bounds=False):
        nonlocal offset
        data = arr.tobytes()
        views.append({'buffer': 0, 'byteOffset': offset, 'byteLength': len(data), 'target': target})
        item = {'bufferView': len(views) - 1, 'componentType': ctype, 'count': int(arr.shape[0] if arr.ndim > 1 else arr.size), 'type': kind}
        if with_bounds:
            item['min'] = arr.min(axis=0).astype(float).tolist()
            item['max'] = arr.max(axis=0).astype(float).tolist()
        accessors.append(item)
        chunks.append(data)
        offset += len(data)
        pad = (4 - offset % 4) % 4
        if pad:
            chunks.append(b'\0' * pad)
            offset += pad
        return len(accessors) - 1
    p = add(pos, 5126, 'VEC3', 34962, True)
    n = add(normal, 5126, 'VEC3', 34962)
    t0 = add(uv0, 5126, 'VEC2', 34962)
    t1 = add(uv1, 5126, 'VEC2', 34962)
    c = add(color, 5126, 'VEC4', 34962)
    i = add(indices, 5125, 'SCALAR', 34963)
    gltf = {'asset': {'version': '2.0', 'generator': 'S47 freeformterrain build_mesh'}, 'scene': 0, 'scenes': [{'nodes': [0]}],
            'nodes': [{'name': name, 'mesh': 0}],
            'meshes': [{'name': name, 'primitives': [{'attributes': {'POSITION': p, 'NORMAL': n, 'TEXCOORD_0': t0, 'TEXCOORD_1': t1, 'COLOR_0': c}, 'indices': i, 'material': 0}]}],
            'materials': [{'name': 'M_FF_GroundSlot', 'pbrMetallicRoughness': {'baseColorFactor': [1, 1, 1, 1], 'metallicFactor': 0, 'roughnessFactor': 1}}],
            'accessors': accessors, 'bufferViews': views, 'buffers': [{'byteLength': offset}]}
    js = json.dumps(gltf, separators=(',', ':')).encode()
    js += b' ' * ((4 - len(js) % 4) % 4)
    total = 12 + 8 + len(js) + 8 + offset
    tmp = Path(str(path) + '.tmp')
    with open(tmp, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, total))
        f.write(struct.pack('<I', len(js)) + b'JSON' + js)
        f.write(struct.pack('<I', offset) + b'BIN\0')
        for c_ in chunks:
            f.write(c_)
    tmp.replace(path)   # an import can start at any moment: never a half-written file


def vertex_normals(P, T):
    a, b, c = P[T[:, 0]], P[T[:, 1]], P[T[:, 2]]
    fn = np.cross(b - a, c - a)
    N = np.zeros_like(P)
    for k in range(3):
        for col in range(3):
            N[:, col] += np.bincount(T[:, k], fn[:, col], minlength=len(P))
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
    return N


def skirts(V_pos, attrs, T, on_rim, drop=80.0):
    """A wall down from the map's outer edge, so that lowered land can never show a gap under it."""
    e = np.concatenate([T[:, [0, 1]], T[:, [1, 2]], T[:, [2, 0]]])
    key = np.minimum(e[:, 0], e[:, 1]) * len(V_pos) + np.maximum(e[:, 0], e[:, 1])
    u, inv, cnt = np.unique(key, return_inverse=True, return_counts=True)
    open_edge = cnt[inv] == 1
    be = e[open_edge]
    rim = on_rim(V_pos[be[:, 0]]) & on_rim(V_pos[be[:, 1]])
    be = be[rim]
    used, ix = np.unique(be, return_inverse=True)
    ix = ix.reshape(-1, 2)
    base = len(V_pos)
    low = V_pos[used].copy()
    low[:, 1] -= drop
    new_pos = np.concatenate([V_pos, low])
    new_attrs = {k: np.concatenate([v, v[used]]) for k, v in attrs.items()}
    a, b = be[:, 0], be[:, 1]
    al, bl = base + ix[:, 0], base + ix[:, 1]
    new_t = np.concatenate([np.stack([b, a, al], 1), np.stack([b, al, bl], 1)])
    return new_pos, new_attrs, np.concatenate([T, new_t]), len(new_t)


def finish(name, field, to_road, grid_T, V, src_pos, src_T, src, static, out_dir, keep_normals=None):
    """Vertex heights, attributes, checks; writes the GLB. static: (P, N, UV, C, T) carried over."""
    dx, dz = to_road
    x, z, ybase = V['x'], V['z'], V['ybase']
    off = field.off(x + dx, z + dz)
    y = ybase + off
    P = np.stack([x, y, z], axis=1)
    # the surface under Off = 0 is the source's, to floating point
    tri = src_T[src]
    a, b, c = src_pos[tri[:, 0]], src_pos[tri[:, 1]], src_pos[tri[:, 2]]
    check = {}
    zero_v = off == 0
    if zero_v.any():
        # each new vertex against the source triangle of a triangle using it (barycentric)
        vid = grid_T[:, 0]
        den = (b[:, 2] - c[:, 2]) * (a[:, 0] - c[:, 0]) + (c[:, 0] - b[:, 0]) * (a[:, 2] - c[:, 2])
        errs = []
        for col in range(3):
            v = grid_T[:, col]
            s = zero_v[v]
            u0 = ((b[s, 2] - c[s, 2]) * (x[v[s]] - c[s, 0]) + (c[s, 0] - b[s, 0]) * (z[v[s]] - c[s, 2])) / den[s]
            v0 = ((c[s, 2] - a[s, 2]) * (x[v[s]] - c[s, 0]) + (a[s, 0] - c[s, 0]) * (z[v[s]] - c[s, 2])) / den[s]
            errs.append(np.abs(u0 * a[s, 1] + v0 * b[s, 1] + (1 - u0 - v0) * c[s, 1] - y[v[s]]))
        e = np.concatenate(errs)
        check['max_error_vs_source_where_off_0_mm'] = float(e.max() * 1000)
        strict = field.sample(field.weight.astype(np.float64), x + dx, z + dz) <= 0
        if dx or dz:
            strict = field.sample(field.to_floor.astype(np.float64), x + dx, z + dz) <= 0
        errs = []
        for col in range(3):
            v = grid_T[:, col]
            s_ = strict[v]
            u0 = ((b[s_, 2] - c[s_, 2]) * (x[v[s_]] - c[s_, 0]) + (c[s_, 0] - b[s_, 0]) * (z[v[s_]] - c[s_, 2])) / den[s_]
            v0 = ((c[s_, 2] - a[s_, 2]) * (x[v[s_]] - c[s_, 0]) + (a[s_, 0] - c[s_, 0]) * (z[v[s_]] - c[s_, 2])) / den[s_]
            errs.append(np.abs(u0 * a[s_, 1] + v0 * b[s_, 1] + (1 - u0 - v0) * c[s_, 1] - y[v[s_]]))
        check['max_error_vs_source_in_masks_mm'] = float(np.concatenate(errs).max() * 1000)
        check['vertices_off_0'] = int(zero_v.sum())
    # same triangles as the source wherever nothing asked for a split
    check['source_triangles_kept_as_they_were'] = int((np.bincount(src, minlength=len(src_T)) == 1).sum())
    check['source_triangles'] = int(len(src_T))
    N = vertex_normals(P, grid_T)
    # orientation as in the source
    src_up = np.cross(src_pos[src_T[:, 1]] - src_pos[src_T[:, 0]], src_pos[src_T[:, 2]] - src_pos[src_T[:, 0]])[:, 1]
    new_up = np.cross(P[grid_T[:, 1]] - P[grid_T[:, 0]], P[grid_T[:, 2]] - P[grid_T[:, 0]])[:, 1]
    check['source_up_share'] = float((src_up > 0).mean())
    check['new_up_share'] = float((new_up > 0).mean())
    sign = 1.0 if (src_up > 0).mean() > .5 else -1.0
    if sign < 0:
        N = -N
    if keep_normals is not None:
        k, n_src = keep_normals
        m = k & zero_v[:len(k)]
        N[:len(k)][m] = n_src[m]
    sed = field.sample(field.sediment.astype(np.float64), x + dx, z + dz)
    sto = field.sample(field.stone.astype(np.float64), x + dx, z + dz)
    attrs = {'n': N, 'uv0': V['uv0'], 'uv1': np.stack([sed, sto], axis=1), 'col': V['col']}
    T = grid_T
    # how far the mesh lies from base + Off over the triangles' insides, by zone
    rs = np.random.default_rng(1)
    pick = rs.integers(0, len(T), 400000)
    w = rs.dirichlet((1, 1, 1), len(pick))
    tp = T[pick]
    sx = (x[tp] * w).sum(axis=1)
    sz = (z[tp] * w).sum(axis=1)
    sy = (y[tp] * w).sum(axis=1)
    by = (ybase[tp] * w).sum(axis=1)
    e = np.abs(sy - by - field.off(sx + dx, sz + dz))
    d = field.sample(field.dist.astype(np.float64), sx + dx, sz + dz)
    for lo, hi in ((0, 100), (100, 350), (350, 1000), (1000, 1e9)):
        m = (d >= lo) & (d < hi)
        if m.any():
            check['mesh_vs_off_%d_%d_m' % (lo, min(hi, 99999))] = {'p50': float(np.percentile(e[m], 50)), 'p99': float(np.percentile(e[m], 99)), 'max': float(e[m].max())}
    if static is not None:
        Ps, Ns, UVs, Cs, Ts = static
        os_ = field.off(Ps[:, 0] + dx, Ps[:, 2] + dz)
        Ps2 = Ps.copy()
        Ps2[:, 1] += os_
        sx2 = field.sample(field.sediment.astype(np.float64), Ps[:, 0] + dx, Ps[:, 2] + dz)
        st2 = field.sample(field.stone.astype(np.float64), Ps[:, 0] + dx, Ps[:, 2] + dz)
        check['static_vertices'] = int(len(Ps))
        check['static_triangles'] = int(len(Ts))
        check['static_vertices_moved'] = int((os_ != 0).sum())
        base_n = len(P)
        P = np.concatenate([P, Ps2])
        attrs = {'n': np.concatenate([attrs['n'], Ns]), 'uv0': np.concatenate([attrs['uv0'], UVs]),
                 'uv1': np.concatenate([attrs['uv1'], np.stack([sx2, st2], axis=1)]), 'col': np.concatenate([attrs['col'], Cs])}
        T = np.concatenate([T, Ts + base_n])
    return P, attrs, T, check


def build_road(field, web_geo, budget, l_max, tol, l_min, collision=False):
    log('road: read g873')
    P, N, UV, C, T = read_glb_mesh(web_geo / 'geo_007.glb', 'g873')
    comp = components(len(P), T)
    ids, counts = np.unique(comp, return_counts=True)
    grid = ids[np.argmax(counts)]
    t_is_grid = comp[T[:, 0]] == grid
    vmask = comp == grid
    remap = np.full(len(P), -1, np.int64)
    remap[vmask] = np.arange(vmask.sum())
    gT = remap[T[t_is_grid]]
    Vp = P[vmask]
    # the port against the mesh itself
    port_err = float(np.abs(B.grid_h(Vp[:, 0], Vp[:, 2]).astype(np.float32) - Vp[:, 1].astype(np.float32)).max())
    log('  grid', len(Vp), len(gT), 'port error', port_err)
    V = {'x': Vp[:, 0].copy(), 'z': Vp[:, 2].copy(), 'ybase': Vp[:, 1].copy(), 'uv0': UV[vmask].copy(), 'col': C[vmask].copy()}
    want = make_want(field, (0.0, 0.0), l_max, tol, l_min, budget)
    V, gt2, src = refine(V, gT.copy(), want)
    # the static parts: both highway verges, both track verges, the tufts (no tufts in the collision)
    sm = ~vmask
    t_static = ~t_is_grid
    if collision:
        big = np.isin(comp, ids[counts > 3])
        sm = sm & big
        t_static = t_static & big[T[:, 0]]
    r2 = np.full(len(P), -1, np.int64)
    r2[sm] = np.arange(sm.sum())
    sT = r2[T[t_static]]
    static = (P[sm], N[sm], UV[sm], C[sm], sT)
    keep = np.zeros(len(V['x']), bool)
    keep[:len(Vp)] = True
    n_keep = np.zeros((len(V['x']), 3))
    n_keep[:len(Vp)] = N[vmask]
    Q, attrs, tr, check = finish('SM_FF_Ground_Road', field, (0.0, 0.0), gt2, V, Vp, gT, src, static, None, (keep, n_keep))
    check['port_error_m_at_g873_grid_vertices'] = port_err
    x0, x1, z0, z1 = Vp[:, 0].min(), Vp[:, 0].max(), Vp[:, 2].min(), Vp[:, 2].max()

    def rim(p):
        return (np.abs(p[:, 0] - x0) < .01) | (np.abs(p[:, 0] - x1) < .01) | (np.abs(p[:, 2] - z0) < .01) | (np.abs(p[:, 2] - z1) < .01)
    Q, attrs, tr, added = skirts(Q, attrs, tr, rim)
    check['skirt_triangles'] = int(added)
    check['source_g873'] = {'triangles': int(len(T)), 'vertices': int(len(P)), 'bounds_min': P.min(0).tolist(), 'bounds_max': P.max(0).tolist()}
    return Q, attrs, tr, check


def build_saro(field, trim_g4, budget, l_max, tol, l_min, collision=False):
    log('saro: read g4 (the game uses the trimmed one)')
    P, N, UV, C, T = read_glb_mesh(trim_g4, 'g4')
    # the ring closes on duplicate vertices: one position, one vertex, or a split would open a crack
    key = np.round(P[:, 0] * 1000).astype(np.int64) * 10000000 + np.round(P[:, 2] * 1000).astype(np.int64)
    u, first, inv = np.unique(key, return_index=True, return_inverse=True)
    gap = np.abs(P[:, 1] - P[first][inv, 1]).max()
    log('  merged', len(P), '->', len(u), 'largest height difference at a merge', gap)
    Vp = P[first]
    mT = inv[T]
    bad = (mT[:, 0] == mT[:, 1]) | (mT[:, 1] == mT[:, 2]) | (mT[:, 2] == mT[:, 0])
    mT = mT[~bad]
    V = {'x': Vp[:, 0].copy(), 'z': Vp[:, 2].copy(), 'ybase': Vp[:, 1].copy(), 'uv0': UV[first].copy(), 'col': C[first].copy()}
    to_road = (24.0, -300.0)
    want = make_want(field, to_road, l_max, tol, l_min, budget, B.saro_land)
    V, gt2, src = refine(V, mT.copy(), want)
    # the floor here is SaroLand, which the exported ring only approximates (up to a metre out in its long triangles).
    # Where the relief is live the mesh lies on the floor; in the masks it stays the ring as it was.
    k = field.sample(field.to_floor.astype(np.float64), V['x'] + to_road[0], V['z'] + to_road[1])
    V['ybase'] = V['ybase'] + (B.saro_land(V['x'], V['z']) - V['ybase']) * k
    keep = np.zeros(len(V['x']), bool)
    keep[:len(Vp)] = True
    n_keep = np.zeros((len(V['x']), 3))
    n_keep[:len(Vp)] = N[first]
    Q, attrs, tr, check = finish('SM_FF_Ground_Saro', field, to_road, gt2, V, Vp, mT, src, None, None, (keep, n_keep))
    r = np.hypot(Vp[:, 0], Vp[:, 2]).max()

    def rim(p):
        return np.hypot(p[:, 0], p[:, 2]) > r - .6
    Q, attrs, tr, added = skirts(Q, attrs, tr, rim)
    check['skirt_triangles'] = int(added)
    check['source_g4_trim'] = {'triangles': int(len(T)), 'vertices': int(len(P)), 'merged_vertices': int(len(u)), 'degenerate_after_merge': int(bad.sum()), 'bounds_min': P.min(0).tolist(), 'bounds_max': P.max(0).tolist()}
    return Q, attrs, tr, check


# longest edge allowed, by distance from what stays put: < 100, < 350, < 1000, beyond (m)
L_MAX = [2.9, 5.8, 11.5, 23.0]
COL_L_MAX = [8.0, 16.0, 32.0, 64.0]
COL_TOL = [.04, .07, .25]
COL_L_MIN = [2.9, 2.9, 8.0]
# what a triangle may be off from base + Off by, and the size it may be split down to, at d = 100, 350, 1000
TOL = [.02, .035, .10]
L_MIN = [2.0, 2.0, 5.8]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--web-geo', type=Path, required=True)
    ap.add_argument('--trim-g4', type=Path, required=True)
    ap.add_argument('--road-budget', type=int, default=2300000)
    ap.add_argument('--saro-budget', type=int, default=800000)
    ap.add_argument('--col-road-budget', type=int, default=650000)
    ap.add_argument('--col-saro-budget', type=int, default=250000)
    ap.add_argument('--only', default='', help="a word in the names to build, for instance Saro; the rest stay as they are")
    args = ap.parse_args()
    field = Field(args.root)
    (args.root / 'mesh').mkdir(exist_ok=True)
    old_report = args.root / 'report/build_mesh.json'
    report = json.loads(old_report.read_text()) if args.only and old_report.exists() else {}
    report.pop('total_triangles_drawn', None)
    for name, fn in (('SM_FF_Ground_Road', lambda: build_road(field, args.web_geo, args.road_budget, L_MAX, TOL, L_MIN)),
                     ('SM_FF_Ground_Saro', lambda: build_saro(field, args.trim_g4, args.saro_budget, L_MAX, TOL, L_MIN)),
                     # plain meshes for complex collision: exact where one walks and drives, coarser than the drawn ones
                     ('SM_FF_Ground_Road_Col', lambda: build_road(field, args.web_geo, args.col_road_budget, COL_L_MAX, COL_TOL, COL_L_MIN, True)),
                     ('SM_FF_Ground_Saro_Col', lambda: build_saro(field, args.trim_g4, args.col_saro_budget, COL_L_MAX, COL_TOL, COL_L_MIN, True))):
        if args.only and args.only not in name:
            continue
        Q, attrs, tr, check = fn()
        path = args.root / 'mesh' / (name + '.glb')
        write_glb(path, name, Q, attrs['n'], attrs['uv0'], attrs['uv1'], attrs['col'], tr)
        check['triangles'] = int(len(tr))
        check['vertices'] = int(len(Q))
        check['bounds_min'] = Q.min(0).tolist()
        check['bounds_max'] = Q.max(0).tolist()
        check['glb_mb'] = path.stat().st_size / 1e6
        report[name] = check
        log(name, json.dumps(check))
    report['total_triangles_drawn'] = sum(v['triangles'] for k, v in report.items() if not k.endswith('_Col'))
    (args.root / 'report/build_mesh.json').write_text(json.dumps(report, indent=1) + '\n')
    log('total triangles drawn', report['total_triangles_drawn'])


if __name__ == '__main__':
    main()
