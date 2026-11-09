"""Editor-only, run once (M6 part 2, 9 November 2026): keep CPU copies of the two MotelV2 trim meshes that hold the motel lot's old lamps
(MotelFront.ts lotLamps(): the pole and arm in the metal batch, /Game/MotelV2/Trim/g463; the box head in the black batch, g461), so
S47StreetLamps can clone them without the lamps. These assets are in Git, so the change is committed. Nothing else changes: no
geometry, no material. It also counts the triangles inside each old lamp's box (motel terms, the same as the batches' own) as a check.

MOTEL-FACADE-20261109: the same for the court's wear layer, /Game/MotelV2/Trim/g529 (node 769), so S47MotelV2 can cut the old office
front out of it at runtime: 52 triangles in one plane at x -40.988, z 22..27.8 (the layer's own terms). Without this the runtime sets
the whole layer 3 cm back instead and logs it. Run again after a new MotelV2 trim import; already prepared assets are left as they are.
"""
import json
from pathlib import Path
import unreal
LAMP_BOXES = [((-29.9, -0.3, z - .3), (-28.15, 6.75, z + .3)) for z in (18, 47)]  # x, y, z in metres, three.js terms
OFFICE_FRONT = [((-41.0, -.05, 21.98), (-40.975, 2.85, 27.82))]  # S47MotelV2.cpp OfficeFront, the same box
ASSETS = [(463, 'lamp_box_triangles', LAMP_BOXES, None), (461, 'lamp_box_triangles', LAMP_BOXES, None), (529, 'office_front_triangles', OFFICE_FRONT, [52])]


def inside(mesh, boxes):
    counts = [0] * len(boxes)
    for s in range(mesh.get_num_sections(0)):
        r = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, s)
        verts, tris = r[0], r[1]
        three = [(v.x / 100, v.z / 100, v.y / 100) for v in verts]  # UE (x, y, z) cm -> three (x, z, y) m
        for t in range(0, len(tris) - 2, 3):
            pts = [three[tris[t]], three[tris[t + 1]], three[tris[t + 2]]]
            for k, (lo, hi) in enumerate(boxes):
                if all(lo[i] <= p[i] <= hi[i] for p in pts for i in range(3)):
                    counts[k] += 1; break
    return counts


rows = []
for geo, key, boxes, expected in ASSETS:
    path = f'/Game/MotelV2/Trim/g{geo}.g{geo}'
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), (path, 'missing motel trim')
    before = bool(mesh.get_editor_property('allow_cpu_access'))
    row = {'asset': path, 'lods': mesh.get_num_lods(), 'triangles': mesh.get_num_triangles(0), 'before': before, key: inside(mesh, boxes)}
    assert expected is None or row[key] == expected, (path, key, row[key], expected)
    if not before:
        mesh.set_editor_property('allow_cpu_access', True)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    row['allow_cpu_access'] = bool(mesh.get_editor_property('allow_cpu_access'))
    rows.append(row)
report = {'status': 'PASS', 'scope': 'CPU data preservation only; no geometry or material modification', 'assets': rows}
out = Path(unreal.Paths.project_saved_dir()) / 'streetlamps-motel-cpu.json'; out.write_text(json.dumps(report, indent=2) + '\n')
unreal.log('S47_STREETLAMPS_MOTEL_CPU_PASS ' + json.dumps(report))
