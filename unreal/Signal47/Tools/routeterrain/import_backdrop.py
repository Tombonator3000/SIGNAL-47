"""Import only the original route geology, with a separate light-tracing proxy.

Run inside the guarded UE editor with the legacy FBX factory. All source and
raster triangles remain intact; only the independent RT representation is
reduced. No sky, gameplay floor, global budget or vendor asset is touched.
"""
from pathlib import Path
import importlib.util
import json
import os
import runpy
import traceback
import unreal

TOOLS = Path(__file__).resolve().parents[1]
SOURCE = TOOLS / 'calichestreet/backdrop'
REPORT = Path(os.environ['S47_ROUTE_TERRAIN_REPORT'])
REPORT.parent.mkdir(parents=True, exist_ok=True)
report = {'status': 'RUNNING', 'scope': 'original backdrop only'}

def flush():
    REPORT.write_text(json.dumps(report, indent=2) + '\n')

try:
    flush()
    contract = json.loads((SOURCE / 'model_contract.json').read_text())['parts']['SM_Caliche_Mesas']
    os.environ['S47_STREET_ONLY'] = 'backdrop'
    os.environ['S47_STREET_REPORT'] = str(REPORT.with_name(REPORT.stem + '-import.json'))
    imported = runpy.run_path(str(TOOLS / 'calichestreet/import_assets.py'))
    assert imported['report']['status'] == 'PASS'
    mesh = unreal.load_asset('/Game/CalicheStreet/Backdrop/SM_Caliche_Mesas')
    assert isinstance(mesh, unreal.StaticMesh)
    spec = importlib.util.spec_from_file_location('route_rt_proxy', TOOLS / 'office/rt_proxy.py')
    proxy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(proxy)
    proxy.KEEP = .45
    before = proxy.signature(mesh)
    assert before['source']['triangles'] == contract['triangles']
    assert before['triangles'][0] == contract['triangles']
    assert before['rt'], 'The existing large landform must remain an RT light blocker'
    proxy.configure(mesh)
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
    for lod in range(mesh.get_num_lods()):
        build = subsystem.get_lod_build_settings(mesh, lod)
        build.set_editor_property('use_full_precision_u_vs', True)
        subsystem.set_lod_build_settings(mesh, lod, build)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    after = proxy.signature(mesh)
    for key in ('source', 'triangles', 'materials', 'rt', 'collision'):
        assert before[key] == after[key], ('Authored source/raster changed', key)
    lo, hi = contract['bounds_local_m']
    expected = [[lo[0]*100, -hi[1]*100, lo[2]*100],
                [hi[0]*100, -lo[1]*100, hi[2]*100]]
    error = max(abs(a-b) for aa, bb in zip(after['source']['bounds_cm'], expected) for a, b in zip(aa, bb))
    assert error < .15, ('4km-scale import bounds error exceeds 1.5mm', error)
    report.update(status='PASS', asset=mesh.get_path_name(),
                  before=before, after=after, source_bounds_error_cm=error,
                  configuration=proxy.verify_configuration(mesh),
                  original_import_report=os.environ['S47_STREET_REPORT'],
                  runtime_memory='Requires the final packaged route measurement')
    flush()
    unreal.log('S47 ROUTE TERRAIN IMPORT PASS')
except Exception:
    report.update(status='FAIL', error=traceback.format_exc())
    flush()
    raise
