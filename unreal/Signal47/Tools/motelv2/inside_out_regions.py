"""PLAY-31-01: inside-out solids hidden inside a larger island. Pure Python, no engine or Blender process.

The earlier checks cannot see them: S47MotelProbe compares winding with the stored normal (they agree), and
repair_playtest_walls.py judges whole vertex-connected islands by signed volume. The office front wall of the 31 October
package is one island of 1128 triangles made of several boxes (piers, spandrel, lintels) that touch along shared edges
(edges used by four faces). Some boxes face out, the south pier, the pier between window and door and the spandrel under
the window face in; their volumes cancel, so the island as a whole looks closed, consistent and positive (0.04 m3 where
the 25 cm wall holds about 3 m3). From inside the room the inner skin of an inverted box is culled and one sees the back
of the outer skin, 25 cm deeper; from the court one sees the inner skin. The jambs of the window and the south jamb of the
door belong to those boxes and face away from the opening.

Here the island is split into solids along manifold edges only (an edge used by exactly two faces joins them; an edge used
by four does not). A solid is judged only when it is closed on its own (each of its edges used exactly twice by its own
faces) and consistently wound; it is inside out when its signed volume is negative. Open surfaces (glass, water, leaves,
cards) and solids that are not closed are never guessed.

Audit: python3 inside_out_regions.py FBX [FBX ...] [--json report.json]
"""
from __future__ import annotations

import argparse
import collections
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parent
WELD = 4                 # decimals: 0.1 mm, as the exporter writes metres
MIN_VOLUME = 1e-6        # m3; flatter solids (cards, decals) are left alone


def _vol(A, B, C):
    return (A[0] * (B[1] * C[2] - B[2] * C[1]) - A[1] * (B[0] * C[2] - B[2] * C[0]) + A[2] * (B[0] * C[1] - B[1] * C[0])) / 6


def solids(positions, faces):
    """Closed solids after welding by position and splitting at non-manifold edges:
    [{'faces': [...], 'volume_m3': v, 'bounds_m': [lo, hi]}]. Open or inconsistently wound pieces are not returned."""
    key = {}
    weld = [key.setdefault(tuple(round(c, WELD) for c in p), len(key)) for p in positions]
    edge_faces = collections.defaultdict(list)
    for t, face in enumerate(faces):
        a, b, c = (weld[v] for v in face)
        if len({a, b, c}) < 3:
            continue
        for u, v in ((a, b), (b, c), (c, a)):
            edge_faces[(min(u, v), max(u, v))].append((t, u < v))
    parent = list(range(len(faces)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for users in edge_faces.values():
        if len(users) == 2:
            parent[find(users[1][0])] = find(users[0][0])
    groups = collections.defaultdict(list)
    for t, face in enumerate(faces):
        if len({weld[v] for v in face}) == 3:
            groups[find(t)].append(t)
    out = []
    for ts in groups.values():
        mine = set(ts)
        directed = collections.Counter()
        for t in ts:
            a, b, c = (weld[v] for v in faces[t])
            for u, v in ((a, b), (b, c), (c, a)):
                directed[(u, v)] += 1
        closed = all(directed[(u, v)] == 1 and directed[(v, u)] == 1 for (u, v) in directed)
        if not closed:
            continue
        volume = sum(_vol(*(positions[v] for v in faces[t])) for t in mine)
        lo = [min(positions[v][i] for t in ts for v in faces[t]) for i in range(3)]
        hi = [max(positions[v][i] for t in ts for v in faces[t]) for i in range(3)]
        out.append({'faces': sorted(ts), 'volume_m3': volume, 'bounds_m': [lo, hi]})
    return out


def inward_faces(positions, faces):
    """Faces of closed solids with negative signed volume, and one row per such solid."""
    flips, rows = [], []
    for s in solids(positions, faces):
        if s['volume_m3'] < -MIN_VOLUME:
            flips.extend(s['faces'])
            rows.append({'triangles': len(s['faces']), 'volume_m3': round(s['volume_m3'], 5), 'bounds_m': s['bounds_m']})
    return sorted(flips), rows


def fbx_mesh(path):
    """positions, faces of the single mesh of a production FBX (same reader as repair_face_orientation)."""
    read = runpy.run_path(str(ROOT.parent / 'calichenature/verify_unreal.py'))['fbx_nodes']
    objects = next(n for n in read(path) if n['name'] == 'Objects')
    rows = [n for n in objects['children'] if n['name'] == 'Geometry' and n['properties'][-1] == 'Mesh']
    assert len(rows) == 1, (str(path), 'one exported mesh required')
    fields = {n['name']: n for n in rows[0]['children']}
    flat = fields['Vertices']['properties'][0]
    positions, faces, face = list(zip(flat[::3], flat[1::3], flat[2::3])), [], []
    for index in fields['PolygonVertexIndex']['properties'][0]:
        face.append(index if index >= 0 else -index - 1)
        if index < 0:
            assert len(face) == 3, (str(path), 'nontriangle export')
            faces.append(tuple(face))
            face = []
    return positions, faces


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('fbx', nargs='+', type=Path)
    ap.add_argument('--json', type=Path)
    a = ap.parse_args()
    report = {}
    for path in a.fbx:
        positions, faces = fbx_mesh(path)
        flips, rows = inward_faces(positions, faces)
        report[path.name] = {'triangles': len(faces), 'inward_faces': len(flips), 'solids': rows}
        print(f'{path.name}: {len(faces)} triangles, {len(flips)} faces in {len(rows)} inside-out solids', flush=True)
        for r in sorted(rows, key=lambda r: r['volume_m3'])[:20]:
            lo, hi = r['bounds_m']
            print(f"   {r['triangles']:5d} tris vol {r['volume_m3']:9.5f} m3  x {lo[0]:.2f}..{hi[0]:.2f} y {lo[1]:.2f}..{hi[1]:.2f} "
                  f"z {lo[2]:.2f}..{hi[2]:.2f}", flush=True)
    if a.json:
        a.json.write_text(json.dumps(report, indent=1) + '\n')


if __name__ == '__main__':
    main()
