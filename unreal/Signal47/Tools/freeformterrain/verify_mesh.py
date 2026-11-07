#!/usr/bin/env python3
"""Read the written GLBs back: triangle counts, cracks, base + Off at the vertices, UV1, and (with
report/ue-*/import.json) the imported asset's frame and values. Numbers go to report/verify_mesh.json."""
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import base_port as B
import build_mesh as M


def open_edges(T, n):
    e = np.concatenate([T[:, [0, 1]], T[:, [1, 2]], T[:, [2, 0]]])
    k = np.minimum(e[:, 0], e[:, 1]) * n + np.maximum(e[:, 0], e[:, 1])
    _, inv, c = np.unique(k, return_inverse=True, return_counts=True)
    return e[c[inv] == 1], int((c > 2).sum())


def glb(path, name):
    b = Path(path).read_bytes()
    size = int.from_bytes(b[12:16], 'little')
    j = json.loads(b[20:20 + size])
    data = b[20 + size + 8:]
    p = j['meshes'][0]['primitives'][0]

    def acc(i):
        a = j['accessors'][i]
        v = j['bufferViews'][a['bufferView']]
        t = {5126: 'f4', 5125: 'u4'}[a['componentType']]
        n = {'VEC3': 3, 'SCALAR': 1, 'VEC2': 2, 'VEC4': 4}[a['type']]
        return np.frombuffer(data, '<' + t, a['count'] * n, v['byteOffset']).reshape(-1, n)
    assert j['meshes'][0]['name'] == name
    return {k: acc(i) for k, i in p['attributes'].items()}, acc(p['indices']).reshape(-1, 3).astype(np.int64)


def main():
    root = Path(sys.argv[1])
    field = M.Field(root)
    out = {}
    for name, to_road in (('SM_FF_Ground_Road', (0.0, 0.0)), ('SM_FF_Ground_Saro', (24.0, -300.0))):
        A, T = glb(root / 'mesh' / (name + '.glb'), name)
        P = A['POSITION'].astype(np.float64)
        r = {'triangles': len(T), 'vertices': len(P), 'uv1_max': A['TEXCOORD_1'].max(0).tolist(), 'uv1_min': A['TEXCOORD_1'].min(0).tolist(),
             'color_min': A['COLOR_0'].min(0).tolist(), 'color_max': A['COLOR_0'].max(0).tolist(), 'normal_up_min': float(A['NORMAL'][:, 1].min())}
        e = np.linalg.norm(np.cross(P[T[:, 1]] - P[T[:, 0]], P[T[:, 2]] - P[T[:, 0]]), axis=1)
        r['degenerate_triangles'] = int((e < 1e-9).sum())
        if name.endswith('Road'):
            # base + Off against the port, at grid vertices: everything of the grid has y up
            n_static_up = np.abs(A['NORMAL'][:, 1]) > 0
            x, z = P[:, 0], P[:, 2]
            off = field.off(x + to_road[0], z + to_road[1])
            # the grid's own part is the first vertices; strips and tufts sit elsewhere over it
            d = P[:, 1] - off - B.grid_y(x, z)
            r['y_minus_gridY_plus_off_abs_p50'] = float(np.percentile(np.abs(d), 50))
            grid_like = np.abs(d) < .001
            r['vertices_on_gridY_plus_off_share'] = float(grid_like.mean())
            r['vertices_on_gridY_plus_off_where_off_0'] = float(grid_like[off == 0].mean())
            r['max_error_on_grid_like_mm'] = float(np.abs(d[grid_like]).max() * 1000)
        # no crack in the land: open edges only on the rim or where the source itself is open
        eo, nonmanifold = open_edges(T, len(P))
        rim = np.hypot(P[eo[:, 0], 0], P[eo[:, 0], 2])
        r['open_edges'] = len(eo)
        r['edges_shared_by_more_than_two'] = nonmanifold
        out[name] = r
        print(name, json.dumps(r), flush=True)
    reports = sorted(root.glob('report/ue-*/import.json'))
    if reports:
        d = json.loads(reports[-1].read_text())
        out['ue'] = {}
        for name, to_road in (('SM_FF_Ground_Road', (0.0, 0.0)), ('SM_FF_Ground_Saro', (24.0, -300.0))):
            m = d.get('meshes', {}).get(name)
            if not m or not m.get('vertex_samples'):
                continue
            rows = m['vertex_samples']
            xyz = np.array([r[:3] for r in rows], dtype=np.float64) / 100
            # UE (x, y, z) = Three (x, z, y) * 100
            xx, zz, yy = xyz[:, 0], xyz[:, 1], xyz[:, 2]
            off = field.off(xx + to_road[0], zz + to_road[1])
            q = {'samples': len(rows), 'bounds_cm': m.get('bounds_cm'), 'old_bounds_cm': m.get('old_bounds_cm'), 'source_triangles': m.get('source_triangles'),
                 'fallback_triangles_lod0': m.get('fallback_triangles_lod0'), 'uv_channels': m.get('uv_channels'), 'slots': m.get('slots')}
            if name.endswith('Road'):
                e = np.abs(yy - off - B.grid_y(xx, zz))
                q['frame_error_vs_gridY_plus_off_m_p50'] = float(np.percentile(e, 50))
            uv1 = np.array([r[4] for r in rows if len(r) > 4], dtype=np.float64)
            if len(uv1):
                sed = field.sample(field.sediment.astype(np.float64), xx[:len(uv1)] + to_road[0], zz[:len(uv1)] + to_road[1])
                sto = field.sample(field.stone.astype(np.float64), xx[:len(uv1)] + to_road[0], zz[:len(uv1)] + to_road[1])
                q['uv1_error_vs_fields_p50'] = [float(np.percentile(np.abs(uv1[:, 0] - sed), 50)), float(np.percentile(np.abs(uv1[:, 1] - sto), 50))]
            colors = np.array([r[5] for r in rows if len(r) > 5], dtype=np.float64)
            if len(colors):
                q['vertex_color_mean_min_max'] = [colors.mean(0).tolist(), colors.min(0).tolist(), colors.max(0).tolist()]
            out['ue'][name] = q
            print(name, json.dumps(q), flush=True)
    # the probe's measure, ported: points in the drawn and collision meshes against the floor where the table is live
    rs = np.random.default_rng(47203)
    for name, to_road in (('SM_FF_Ground_Road', (0.0, 0.0)), ('SM_FF_Ground_Saro', (24.0, -300.0)), ('SM_FF_Ground_Road_Col', (0.0, 0.0)), ('SM_FF_Ground_Saro_Col', (24.0, -300.0))):
        A, T = glb(root / 'mesh' / (name + '.glb'), name)
        P = A['POSITION'].astype(np.float64)
        # the ground only: strips, tufts and skirts are not the floor's own
        fn = np.cross(P[T[:, 1]] - P[T[:, 0]], P[T[:, 2]] - P[T[:, 0]])
        area = np.linalg.norm(fn, axis=1) * .5
        ground = np.nonzero((fn[:, 1] / np.maximum(area * 2, 1e-9) > .7) & (area > .3))[0]   # no tufts, no skirts
        pick = ground[rs.integers(0, len(ground), 1500000)]
        w = rs.dirichlet((1, 1, 1), len(pick))
        tp = T[pick]
        sx, sy, sz = (P[tp, 0] * w).sum(1), (P[tp, 1] * w).sum(1), (P[tp, 2] * w).sum(1)
        off = field.off(sx + to_road[0], sz + to_road[1])
        floor = (B.grid_y(sx, sz) if 'Road' in name else B.saro_land(sx, sz)) + off
        d = field.sample(field.dist.astype(np.float64), sx + to_road[0], sz + to_road[1])
        wv = field.sample(field.weight.astype(np.float64), sx + to_road[0], sz + to_road[1])
        e = np.abs(sy - floor)
        ok = np.abs(off) >= .05     # the probe's own test: where the table is live
        row = {}
        for lo, hi in ((0, 300), (300, 1000), (1000, 1e9)):
            m = ok & (d >= lo) & (d < hi)
            if m.any():
                row['%d-%d' % (lo, min(hi, 99999))] = {'n': int(m.sum()), 'p50': float(np.percentile(e[m], 50)), 'p99': float(np.percentile(e[m], 99)), 'p999': float(np.percentile(e[m], 99.9)), 'max': float(e[m].max())}
        out.setdefault('mesh_against_floor', {})[name] = row
        print(name, 'against floor', json.dumps({k: {a: round(b, 3) for a, b in v.items()} for k, v in row.items()}), flush=True)
    # the belt north of the antenna field: every point of the drawn and collision ring against SaroLand + Off,
    # at the probe's own worst points and all over x -333..313, z -580..-116 where the table is live or next to it
    worst = [(-233.9, -522.2), (-115.5, -522.7), (-58.0, -556.3), (105.1, -522.6), (9.5, -554.7), (215.8, -523.9), (278.7, -511.4), (299.5, -186.0)]
    for name in ('SM_FF_Ground_Saro', 'SM_FF_Ground_Saro_Col'):
        A, T = glb(root / 'mesh' / (name + '.glb'), name)
        P = A['POSITION'].astype(np.float64)
        fn = np.cross(P[T[:, 1]] - P[T[:, 0]], P[T[:, 2]] - P[T[:, 0]])
        keep = (fn[:, 1] / np.maximum(np.linalg.norm(fn, axis=1), 1e-9)) > .7
        Tg = T[keep]
        cx = P[Tg].mean(axis=1)
        box = (cx[:, 0] > -340) & (cx[:, 0] < 320) & (cx[:, 2] > -590) & (cx[:, 2] < -110)
        Tb = Tg[box]
        w = rs.dirichlet((1, 1, 1), len(Tb) * 8 // 4 + 1)
        pick = rs.integers(0, len(Tb), len(w))
        tp = Tb[pick]
        sx, sy, sz = (P[tp, 0] * w).sum(1), (P[tp, 1] * w).sum(1), (P[tp, 2] * w).sum(1)
        off = field.off(sx + 24, sz - 300)
        e = sy - B.saro_land(sx, sz) - off
        live = field.sample(field.weight.astype(np.float64), sx + 24, sz - 300) > 0
        in_box = (sx > -333) & (sx < 313) & (sz > -580) & (sz < -116)
        m = live & in_box
        row = {'belt_points': int(m.sum()), 'belt_p99': float(np.percentile(np.abs(e[m]), 99)), 'belt_p999': float(np.percentile(np.abs(e[m]), 99.9)), 'belt_max': float(np.abs(e[m]).max())}
        far = np.abs(e[m]) > .1
        row['belt_over_10cm'] = int(far.sum())
        row['worst_at'] = [round(float(v), 1) for v in (sx[m][np.argmax(np.abs(e[m]))], sz[m][np.argmax(np.abs(e[m]))])]
        pts = []
        for wx, wz in worst:
            n = np.argmin((sx - wx) ** 2 + (sz - wz) ** 2)
            pts.append(round(float(e[n]), 3))
        row['probe_points_nearest'] = pts
        out.setdefault('saro_belt', {})[name] = row
        print(name, 'belt', json.dumps(row), flush=True)
    # the self-test's zones, ported
    def zone(x0, x1, z0, z1):
        xs = np.arange(x0, x1 + .5, 1.0)
        zs = np.arange(z0, z1 + .5, 1.0)
        X, Z = np.meshgrid(xs, zs)
        return float(np.abs(field.off(X, Z)).max())
    zones = {'SARO_PATCH': zone(-120, 120, -200, 600), 'STATION_PATCH': zone(-592, -190, 430, 610), 'farm': zone(27, 201, 130, 290), 'array': zone(-290 + 24, 270 + 24, -515 - 300, -300)}
    th = np.arange(0, 6.2832, .002)
    rr = np.arange(0, 300.5, 2.0)[None]
    zones['SARO site'] = float(np.abs(field.off(np.cos(th)[:, None] * rr + 24, np.sin(th)[:, None] * rr - 300)).max())
    zo = np.arange(-2000, 3400.5, 1.0)
    zones['highway edge'] = max(float(np.abs(field.off(np.full_like(zo, s), zo)).max()) for s in (-6.0, 6.0))
    pts = B.TRACK
    zones['track edge (5 m)'] = float(max(np.abs(field.off(pts[:, 0] + dx, pts[:, 1] + dz)).max() for dx in (-5.6, 5.6) for dz in (-5.6, 5.6)))
    o = B.OLD
    t = np.gradient(o, axis=0)
    t /= np.linalg.norm(t, axis=1, keepdims=True)
    zones['old road edge'] = float(max(np.abs(field.off(o[:, 0] - t[:, 1] * s, o[:, 1] + t[:, 0] * s)).max() for s in (-4.4, 4.4)))
    out['zones_largest_off_m'] = zones
    print('zones', json.dumps({k: round(v, 4) for k, v in zones.items()}), flush=True)
    (root / 'report/verify_mesh.json').write_text(json.dumps(out, indent=1) + '\n')


if __name__ == '__main__':
    main()
