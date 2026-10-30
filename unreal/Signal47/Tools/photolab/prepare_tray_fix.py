"""Prepare/read an immutable owned g264 clone through the compiled native proof bridge.

Fresh commandlet reader: -S47PhotoLabTrayFixVerifyOnly. No Web source save/delete.
Requires an editor build containing S47PhotoLabTrayFixTools before either mode.
"""
from pathlib import Path
import hashlib
import json
import traceback
import unreal

E=unreal.EditorAssetLibrary
SOURCE='/Game/Web/Geo/geo_002/StaticMeshes/g264'
DEST='/Game/Signal47/Art/PhotoLab/SM_PhotoLab_TrayInnerFix'
VERIFY='S47PhotoLabTrayFixVerifyOnly' in unreal.SystemLibrary.get_command_line()
REPORT=Path(unreal.Paths.project_saved_dir())/'photolab-tray-fix.json'
report={'status':'RUNNING','mode':'verify_saved_only' if VERIFY else 'prepare_clone',
        'source':SOURCE,'destination':DEST,'engine':unreal.SystemLibrary.get_engine_version()}

def flush():
    REPORT.parent.mkdir(parents=True,exist_ok=True)
    REPORT.write_text(json.dumps(report,indent=2)+'\n')

def source_sha():
    path=Path(unreal.Paths.project_content_dir())/'Web/Geo/geo_002/StaticMeshes/g264.uasset'
    return hashlib.sha256(path.read_bytes()).hexdigest()

flush()
try:
    tools=getattr(unreal,'S47PhotoLabTrayFixTools',None)
    assert tools is not None,'Build the editor containing the native tray proof bridge first'
    before=source_sha()
    source=E.load_asset(SOURCE)
    assert isinstance(source,unreal.StaticMesh) and source.get_path_name()==SOURCE+'.g264'
    assert source.get_num_lods()==1 and source.get_num_triangles(0)==544
    if E.does_asset_exist(DEST):
        fixed=E.load_asset(DEST)
        assert isinstance(fixed,unreal.StaticMesh) and fixed.get_path_name()==DEST+'.SM_PhotoLab_TrayInnerFix'
    else:
        assert not VERIFY,'Native tray clone absent in verify-only mode'
        fixed=E.duplicate_asset(SOURCE,DEST)
        assert isinstance(fixed,unreal.StaticMesh) and fixed.get_path_name()==DEST+'.SM_PhotoLab_TrayInnerFix'
    proof=json.loads(tools.prepare(source,fixed,VERIFY))
    report['native_proof']=proof
    assert proof['status']=='PASS',proof
    if not VERIFY:
        assert E.save_loaded_asset(fixed,only_if_is_dirty=False)
    assert source_sha()==before,'Original Web mesh bytes changed'
    report['source_uasset_sha256']=before
    report['status']='PASS'
    report['limitations']='Native proof and mesh separation only; final images and active-paper/develop/use checks remain separate.'
    flush()
    unreal.log('S47 PHOTOLAB TRAY FIX PASS '+report['mode'])
except Exception:
    report['status']='FAIL';report['error']=traceback.format_exc();flush();raise
