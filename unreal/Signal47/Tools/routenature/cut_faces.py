"""PT-ROCKS-20261109 (Tom: the large rocks in the desert are only partly modelled, one side cut off or flat, parts missing): read-only.

For each of the four rock families of S47RouteNature (/Game/Licensed/RouteNature, private Fab copies): what is cut, missing or flat.
- the drawn LOD0 (for the Nanite cliff its fallback, what the instances draw with Nanite off and what Lumen's ray tracing sees with
  the proxy) exported as OBJ to the work folder, never into Git
- open edges after welding (a missing back or bottom: an open boundary), their length and the facing of each hole
- large planar patches (a flat cut: neighbouring triangles within 4 degrees of one plane, within 1.5 cm of it), their normal in the
  mesh's own axes, share of the area, where they lie and how high their top edge stands over the mesh's lowest point
- the share of the surface facing each of the six ways (a side with almost nothing is a side not scanned)
- each material slot: two-sided or not (backface culling)
- the LODs: count and each LOD's bounds against LOD0 (a turned LOD, as ApronGrade's was)
Writes cut_faces.json and prints the C++ table for S47RouteNature.cpp (Cuts[]): the dominant cut's normal and how far to sink.

UnrealEditor-Cmd ... -run=pythonscript -script=<this> -unattended -nullrhi -nosplash -NoSound
S47_RN_WORK: folder for the OBJ and the report (default ~/scratch-bm/Signal47-rocks-20261109). Saves nothing in the project.
"""
import json
import math
import os
from pathlib import Path

import unreal

WORK = Path(os.environ.get('S47_RN_WORK', str(Path.home() / 'scratch-bm' / 'Signal47-rocks-20261109')))
ROCKS = ('Cliff_A', 'Boulder_A', 'Boulder_B', 'Outcrop_A')
E = unreal.EditorAssetLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)


def export_obj(mesh, path):
    task = unreal.AssetExportTask()
    task.set_editor_property('object', mesh)
    task.set_editor_property('filename', str(path))
    task.set_editor_property('automated', True)
    task.set_editor_property('prompt', False)
    task.set_editor_property('replace_identical', True)
    task.set_editor_property('exporter', unreal.StaticMeshExporterOBJ())
    if not unreal.Exporter.run_asset_export_task(task) or not path.exists():
        raise RuntimeError('OBJ export failed: ' + str(path))


def read_obj(path):
    verts, tris = [], []
    with open(path) as f:
        for line in f:
            if line.startswith('v '):
                # the exporter writes Unreal's X, Z, Y (Lightwave's axes): back to Unreal's, Z up
                _, x, z, y = line.split()[:4]
                verts.append((float(x), float(y), float(z)))
            elif line.startswith('f '):
                idx = [int(p.split('/')[0]) for p in line.split()[1:]]
                idx = [i - 1 if i > 0 else len(verts) + i for i in idx]
                for k in range(1, len(idx) - 1):
                    tris.append((idx[0], idx[k], idx[k + 1]))
    return verts, tris


def sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def norm(a):
    l = math.sqrt(dot(a, a))
    return (a[0] / l, a[1] / l, a[2] / l) if l > 0 else (0., 0., 0.)


def analyse(verts, tris):
    # weld by position (1 mm): the exporter splits vertices at UV seams and hard edges
    key, weld = {}, []
    for v in verts:
        k = (round(v[0] * 10), round(v[1] * 10), round(v[2] * 10))
        weld.append(key.setdefault(k, len(key)))
    P = [None] * len(key)
    for i, v in enumerate(verts):
        P[weld[i]] = v
    T = [(weld[a], weld[b], weld[c]) for a, b, c in tris]
    T = [t for t in T if len(set(t)) == 3]
    N, A = [], []
    for a, b, c in T:
        n = cross(sub(P[b], P[a]), sub(P[c], P[a]))
        l = math.sqrt(dot(n, n))
        A.append(l / 2)
        N.append((n[0] / l, n[1] / l, n[2] / l) if l > 0 else (0., 0., 1.))
    total = sum(A)
    zmin = min(p[2] for p in P)
    # outward: the winding's sign is not trusted, a normal faces away from the mesh's centre of area
    mid = tuple(sum(sum(P[i][k] for i in t) / 3 * a for t, a in zip(T, A)) / total for k in range(3))

    def outward(n, at):
        return n if dot(n, sub(at, mid)) >= 0 else (-n[0], -n[1], -n[2])
    # facing shares
    faces = {'+X': 0., '-X': 0., '+Y': 0., '-Y': 0., '+Z': 0., '-Z': 0.}
    for t, n, a in zip(T, N, A):
        n = outward(n, tuple(sum(P[i][k] for i in t) / 3 for k in range(3)))
        ax = max(range(3), key=lambda i: abs(n[i]))
        faces[('+' if n[ax] >= 0 else '-') + 'XYZ'[ax]] += a / total
    # edges and adjacency
    edges = {}
    for ti, (a, b, c) in enumerate(T):
        for u, v in ((a, b), (b, c), (c, a)):
            edges.setdefault((min(u, v), max(u, v)), []).append(ti)
    open_edges = [e for e, ts in edges.items() if len(ts) == 1]
    open_len = sum(math.sqrt(dot(sub(P[u], P[v]), sub(P[u], P[v]))) for u, v in open_edges)
    # open boundary loops: chain the open edges, each loop's Newell normal (which way the hole faces) and area
    nbr = {}
    for u, v in open_edges:
        nbr.setdefault(u, []).append(v); nbr.setdefault(v, []).append(u)
    seen, holes = set(), []
    for start in nbr:
        if start in seen:
            continue
        loop, cur, prev = [], start, None
        while cur is not None and cur not in seen:
            seen.add(cur); loop.append(cur)
            nxt = [w for w in nbr[cur] if w != prev and w not in seen]
            prev, cur = cur, (nxt[0] if nxt else None)
        if len(loop) < 3:
            continue
        nx = ny = nz = 0.
        for i in range(len(loop)):
            p, q = P[loop[i]], P[loop[(i + 1) % len(loop)]]
            nx += (p[1] - q[1]) * (p[2] + q[2]); ny += (p[2] - q[2]) * (p[0] + q[0]); nz += (p[0] - q[0]) * (p[1] + q[1])
        area = math.sqrt(nx * nx + ny * ny + nz * nz) / 2
        c = tuple(sum(P[i][k] for i in loop) / len(loop) for k in range(3))
        holes.append({'vertices': len(loop), 'area_cm2': round(area, 1), 'share_of_surface': round(area / total, 4),
                      'facing': [round(x, 3) for x in outward(norm((nx, ny, nz)), c)], 'centre_cm': [round(x, 1) for x in c],
                      'top_over_base_cm': round(max(P[i][2] for i in loop) - zmin, 1)})
    holes.sort(key=lambda h: -h['area_cm2'])
    # planar patches by region growing from the largest triangles
    adj = [[] for _ in T]
    for ts in edges.values():
        if len(ts) == 2:
            adj[ts[0]].append(ts[1]); adj[ts[1]].append(ts[0])
    used = [False] * len(T)
    patches = []
    cos_tol = math.cos(math.radians(4.))
    for seed in sorted(range(len(T)), key=lambda i: -A[i]):
        if used[seed]:
            continue
        n0, p0 = N[seed], P[T[seed][0]]
        d0 = dot(n0, p0)
        region, stack = [], [seed]
        used[seed] = True
        while stack:
            t = stack.pop(); region.append(t)
            for o in adj[t]:
                if used[o] or dot(N[o], n0) < cos_tol:
                    continue
                if max(abs(dot(n0, P[i]) - d0) for i in T[o]) > 1.5:
                    continue
                used[o] = True; stack.append(o)
        ra = sum(A[t] for t in region)
        if ra / total < .02:
            continue
        pts = [P[i] for t in region for i in T[t]]
        pc = tuple(sum(p[k] for p in pts) / len(pts) for k in range(3))
        nn = outward(norm(tuple(sum(N[t][k] * A[t] for t in region) for k in range(3))), pc)
        patches.append({'triangles': len(region), 'share_of_surface': round(ra / total, 4), 'normal': [round(x, 3) for x in nn],
                        'centre_cm': [round(x, 1) for x in pc],
                        'top_over_base_cm': round(max(p[2] for p in pts) - zmin, 1),
                        'bottom_over_base_cm': round(min(p[2] for p in pts) - zmin, 1)})
    patches.sort(key=lambda p: -p['share_of_surface'])
    return {'vertices_welded': len(P), 'triangles': len(T), 'area_cm2': round(total, 1), 'facing_share': {k: round(v, 4) for k, v in faces.items()},
            'open_edges': len(open_edges), 'open_edge_length_cm': round(open_len, 1), 'holes': holes[:8], 'planar_patches': patches[:8],
            'bounds_cm': {'min': [round(min(p[k] for p in P), 1) for k in range(3)], 'max': [round(max(p[k] for p in P), 1) for k in range(3)]}}


def materials(mesh):
    out = []
    for slot in mesh.get_editor_property('static_materials'):
        mi = slot.get_editor_property('material_interface')
        if mi is None:
            out.append(None); continue
        base = mi.get_base_material()
        row = {'slot': str(slot.get_editor_property('material_slot_name')), 'material': mi.get_path_name(), 'base_two_sided': bool(base.get_editor_property('two_sided'))}
        if isinstance(mi, unreal.MaterialInstance):
            o = mi.get_editor_property('base_property_overrides')
            row['override_two_sided'] = bool(o.get_editor_property('override_two_sided')) and bool(o.get_editor_property('two_sided'))
        out.append(row)
    return out


def lods(mesh):
    out = []
    for lod in range(S.get_lod_count(mesh)):
        try:
            d = mesh.get_static_mesh_description(lod)
            n = d.get_vertex_count()
            pts = [d.get_vertex_position(unreal.VertexID(i)) for i in range(0, n, max(1, n // 4000))]
            out.append({'lod': lod, 'source_vertices': n,
                        'min': [round(min(p.x for p in pts), 1), round(min(p.y for p in pts), 1), round(min(p.z for p in pts), 1)],
                        'max': [round(max(p.x for p in pts), 1), round(max(p.y for p in pts), 1), round(max(p.z for p in pts), 1)]})
        except Exception as e:  # generated LODs have no source description
            out.append({'lod': lod, 'note': 'no source description (%s)' % type(e).__name__, 'render_triangles': mesh.get_num_triangles(lod)})
    return out


def main():
    WORK.mkdir(parents=True, exist_ok=True)
    report, table = {}, []
    for name in ROCKS:
        mesh = E.load_asset('/Game/Licensed/RouteNature/Meshes/SM_RN_%s' % name)
        if mesh is None:
            report[name] = {'error': 'missing'}; continue
        obj = WORK / ('SM_RN_%s.obj' % name)
        export_obj(mesh, obj)
        verts, tris = read_obj(obj)
        r = analyse(verts, tris)
        r['materials'] = materials(mesh)
        r['lods'] = lods(mesh)
        r['nanite'] = bool(mesh.get_editor_property('nanite_settings').enabled)
        report[name] = r
        # the cut to hide: the largest flat patch over 4 % or, failing that, the largest hole; its top edge says how far to sink a bottom
        cut = next((p for p in r['planar_patches'] if p['share_of_surface'] >= .04), None)
        hole = r['holes'][0] if r['holes'] and r['holes'][0]['share_of_surface'] >= .01 else None
        pick = cut or hole
        if pick:
            n = pick['normal'] if cut else pick['facing']
            table.append(' {TEXT("%s"),FVector(%.3f,%.3f,%.3f),%.2f}, // %s %.1f %% of the surface, top %.0f cm over the base' % (
                name, n[0], n[1], n[2], pick['top_over_base_cm'] / 100., 'flat patch' if cut else 'hole', 100 * pick['share_of_surface'], pick['top_over_base_cm']))
    (WORK / 'cut_faces.json').write_text(json.dumps(report, indent=1) + '\n')
    unreal.log_warning('S47 ROCK CUTS -> %s' % (WORK / 'cut_faces.json'))
    for line in table:
        unreal.log_warning('S47 ROCK CUT ' + line)


main()
