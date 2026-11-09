"""Editor-only, run once (M6 part 2, 9 November 2026): keep CPU copies of the two MotelV2 trim meshes that hold the motel lot's old lamps
(MotelFront.ts lotLamps(): the pole and arm in the metal batch, /Game/MotelV2/Trim/g463; the box head in the black batch, g461), so
S47StreetLamps can clone them without the lamps. These assets are in Git, so the change is committed. Nothing else changes: no
geometry, no material. It also counts the triangles inside each old lamp's box (motel terms, the same as the batches' own) as a check.
"""
import json
from pathlib import Path
import unreal
BOXES = [((-29.9, -0.3, z - .3), (-28.15, 6.75, z + .3)) for z in (18, 47)]  # x, y, z in metres, three.js terms


def inside(mesh):
    counts = [0] * len(BOXES)
    for s in range(mesh.get_num_sections(0)):
        r = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, s)
        verts, tris = r[0], r[1]
        three = [(v.x / 100, v.z / 100, v.y / 100) for v in verts]  # UE (x, y, z) cm -> three (x, z, y) m
        for t in range(0, len(tris) - 2, 3):
            pts = [three[tris[t]], three[tris[t + 1]], three[tris[t + 2]]]
            for k, (lo, hi) in enumerate(BOXES):
                if all(lo[i] <= p[i] <= hi[i] for p in pts for i in range(3)):
                    counts[k] += 1; break
    return counts


rows = []
for geo in (463, 461):
    path = f'/Game/MotelV2/Trim/g{geo}.g{geo}'
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), (path, 'missing motel trim')
    before = bool(mesh.get_editor_property('allow_cpu_access'))
    row = {'asset': path, 'lods': mesh.get_num_lods(), 'triangles': mesh.get_num_triangles(0), 'before': before, 'lamp_box_triangles': inside(mesh)}
    if not before:
        mesh.set_editor_property('allow_cpu_access', True)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    row['allow_cpu_access'] = bool(mesh.get_editor_property('allow_cpu_access'))
    rows.append(row)
report = {'status': 'PASS', 'scope': 'CPU data preservation only; no geometry or material modification', 'assets': rows}
out = Path(unreal.Paths.project_saved_dir()) / 'streetlamps-motel-cpu.json'; out.write_text(json.dumps(report, indent=2) + '\n')
unreal.log('S47_STREETLAMPS_MOTEL_CPU_PASS ' + json.dumps(report))
