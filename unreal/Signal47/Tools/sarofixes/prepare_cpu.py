"""Editor-only preparation for S47SaroFixes (Tom's test of d36, PT-STOP), run in a reserved editor slot like Tools/streetlamps/prepare_cpu.py.
Keeps the CPU copy of the generated web batch that holds the stop sign's pole by SARO (g436, the pole material's batch of the grounds'
signs: the stop sign, the radio quiet zone's two posts and the rest), so the runtime can clone it without the stop sign's pole; and
of the control room's desk-top and cabinet batches (g55, g60) for the field camera's old shelf by the yard door (Tom's test of d37).
Changes nothing else: no reimport, no geometry, no material. /Game/Web is generated and not in Git; run this again after a new web import
(Tools/webexport/import.sh does). Without it the stop sign and the shelf stay and LogS47SaroFixes says why.
"""
import json
from pathlib import Path
import unreal

rows = []
# SARO-GEN-RAMP-20261109: g55 (the control room's desk tops) and g60 (its cabinets) for the field camera's old shelf by the yard door.
for geo, tris, folder in [(436, 288, 'geo_003'), (55, 72, 'geo_000'), (60, 36, 'geo_000')]:
    path = f'/Game/Web/Geo/{folder}/StaticMeshes/g{geo}.g{geo}'
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), (path, 'missing imported web batch')
    actual = mesh.get_num_triangles(0)
    assert actual == tris, (path, actual, tris)
    before = bool(mesh.get_editor_property('allow_cpu_access'))
    mesh.set_editor_property('allow_cpu_access', True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    assert mesh.get_editor_property('allow_cpu_access')
    rows.append({'asset': path, 'triangles': actual, 'before': before, 'allow_cpu_access': True})
report = {'status': 'PASS', 'scope': 'CPU data preservation only; no geometry or material modification', 'assets': rows}
out = Path(unreal.Paths.project_saved_dir()) / 'sarofixes-cpu.json'
out.write_text(json.dumps(report, indent=2) + '\n')
unreal.log('S47_SAROFIXES_CPU_PASS ' + json.dumps(report))
