"""Explicit editor-only derivative build. Never save the three source meshes.

Run through the existing serialized harness after compiling S47SaroSeamTools.
The three source assets need no CPU-access mutation: editor render buffers are read;
only new derived assets retain CPU data for packaged floor queries.
"""
import hashlib
import json
import os
from pathlib import Path
import traceback
import unreal

E = unreal.EditorAssetLibrary
CONTENT = Path(unreal.Paths.project_content_dir())
SOURCES = ['/Game/Web/Geo/geo_005/StaticMeshes/g664', '/Game/MotelV2/Trim/g663', '/Game/MotelV2/Trim/g4']
DEST = ['/Game/Signal47/Art/SaroRoad/SM_SaroRoadSeam', '/Game/Signal47/Art/SaroRoad/SM_SaroVergeSeam']
VERIFY = 'S47SaroSeamVerifyOnly' in unreal.SystemLibrary.get_command_line()
REPORT = Path(os.environ.get('S47_SARO_SEAM_REPORT', str(Path(unreal.Paths.project_saved_dir()) / 'saro-road-seam.json')))
report = {'status': 'RUNNING', 'verify_only': VERIFY}

def flush():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + '\n')

def source_hashes():
    return [hashlib.sha256((CONTENT / (s.removeprefix('/Game/') + '.uasset')).read_bytes()).hexdigest() for s in SOURCES]

flush()
try:
    tools = getattr(unreal, 'S47SaroSeamTools', None)
    assert tools, 'Compile editor first; this script does not build or launch another process'
    before = source_hashes()
    source = [E.load_asset(s) for s in SOURCES]
    assert all(isinstance(s, unreal.StaticMesh) for s in source)
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
    report['source_full_precision_uvs'] = [bool(subsystem.get_lod_build_settings(s, 0).get_editor_property('use_full_precision_u_vs')) for s in source]
    fixed = []
    for src, dst in zip(SOURCES, DEST):
        if E.does_asset_exist(dst):
            m = E.load_asset(dst)
        else:
            assert not VERIFY, 'Derivative missing in verify-only mode'
            m = E.duplicate_asset(src, dst)
        assert isinstance(m, unreal.StaticMesh)
        fixed.append(m)
    result = json.loads(tools.prepare(source[0], source[1], source[2], fixed[0], fixed[1], VERIFY))
    report['native'] = result
    assert result['status'] == 'PASS', result
    # Both are prepared successfully before either completed candidate is saved.
    if not VERIFY:
        for mesh in fixed:
            assert E.save_loaded_asset(mesh, only_if_is_dirty=False)
    assert source_hashes() == before, 'Original source bytes changed'
    report['status'] = 'PASS'
    report['limits'] = 'Native derivative/collision cook prepared. Runtime atomic activation and before/after driving images remain to be checked.'
    flush()
    unreal.log('S47 SARO ROAD ASSET PREP PASS')
except Exception:
    report.update(status='FAIL', error=traceback.format_exc())
    flush()
    raise
