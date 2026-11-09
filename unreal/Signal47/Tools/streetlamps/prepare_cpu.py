"""Editor-only preparation for S47StreetLamps, run in a reserved editor slot (as Tools/highwaypower/prepare_cpu.py).
Keeps CPU copies of the generated web batches that hold old lamp poles and arms, so the runtime can clone them without them:
the highway's (g26, 992 triangles) and, since M6 part 2 (9 November), SARO's service yard's four lamp posts (g235, the pole
material's batch of the yard's statics). Changes nothing else: no reimport, no geometry, no material. /Game/Web is generated and not in Git;
run this again after a new web import.
"""
import json
from pathlib import Path
import unreal
rows = []
for geo, tris, folder in [(26, 992, 'geo_000'), (235, None, 'geo_001')]:
    path = f'/Game/Web/Geo/{folder}/StaticMeshes/g{geo}.g{geo}'
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), (path, 'missing imported web batch')
    actual = mesh.get_num_triangles(0)
    assert tris is None or actual == tris, (path, actual, tris)
    before = bool(mesh.get_editor_property('allow_cpu_access'))
    mesh.set_editor_property('allow_cpu_access', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    assert mesh.get_editor_property('allow_cpu_access')
    rows.append({'asset': path, 'triangles': actual, 'before': before, 'allow_cpu_access': True})
report = {'status': 'PASS', 'scope': 'CPU data preservation only; no geometry or material modification', 'assets': rows}
out = Path(unreal.Paths.project_saved_dir()) / 'streetlamps-cpu.json'; out.write_text(json.dumps(report, indent=2) + '\n')
unreal.log('S47_STREETLAMPS_CPU_PASS ' + json.dumps(report))
