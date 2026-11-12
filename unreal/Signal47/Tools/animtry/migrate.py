"""ANIM-TRY-20261112: move the Free100Animations packages to /Game/Licensed/Free100 in a throwaway UE 5.8 project, and check them.

Run ONLY in the throwaway project import.sh makes (never in Signal47.uproject: the pack's own roots, /Game/Characters and
/Game/ThirdPerson, are not git-ignored there). The way Tools/cosmosshop/migrate.py does it.

S47_ANIMTRY_MODE=import (default): every folder in selection.json "moves" is renamed to its /Game/Licensed/Free100 path (references
are fixed by the editor), everything is saved in this engine's format and no redirector is left; then the checks.
S47_ANIMTRY_MODE=verify: only the checks, in a fresh process.

The checks: every package under /Game/Licensed/Free100 is saved by UE5; the source mesh and skeleton load; every chosen clip and extra
loads as an AnimSequence on that skeleton. The report (S47_ANIMTRY_REPORT) has names and facts only (length in seconds, frames, frame
rate, root motion on or off, how far root and pelvis travel over the clip) and may go into Git as verification.json. Nothing is run
from the pack; no picture is opened.
"""
import json
import os
import struct
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
MODE = os.environ.get('S47_ANIMTRY_MODE', 'import')
REPORT = Path(os.environ.get('S47_ANIMTRY_REPORT', str(HERE / 'verification.json')))
SEL = json.loads((HERE / 'selection.json').read_text())
DEST = SEL['dest']
E = unreal.EditorAssetLibrary
report = {'status': 'RUNNING', 'mode': MODE, 'engine': unreal.SystemLibrary.get_engine_version(), 'destination': DEST,
          'checks': [], 'clips': {}, 'failed': {}}


def flush():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=1, sort_keys=True, default=str))


def check(name, ok, detail=None):
    report['checks'].append({'name': name, 'pass': bool(ok), 'detail': detail})
    flush()
    if not ok:
        raise RuntimeError('CHECK FAILED: ' + name + ' ' + json.dumps(detail, default=str))


def class_name(data):
    path = getattr(data, 'asset_class_path', None)
    return str(path.asset_name) if path is not None else str(getattr(data, 'asset_class', ''))


def assets_under(root):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([root], True)
    return list(registry.get_assets_by_path(unreal.Name(root), recursive=True) or [])


def legacy_version(path):
    data = path.read_bytes()[:8]
    return struct.unpack('<i', data[4:8])[0] if len(data) == 8 else 0


def migrate():
    check('throwaway project, not Signal47', not unreal.Paths.get_project_file_path().endswith('Signal47.uproject'),
          unreal.Paths.get_project_file_path())
    for old, new in SEL['moves'].items():
        if E.does_directory_exist(new) and E.list_assets(new, recursive=True):
            continue
        check('source folder there: ' + old, E.does_directory_exist(old) and bool(E.list_assets(old, recursive=True)), old)
        for path in E.list_assets(old, recursive=True):
            E.load_asset(path)
        check('rename_directory ' + old + ' -> ' + new, E.rename_directory(old, new))
    check('save_directory (all, this engine\'s format)', E.save_directory(DEST, only_if_is_dirty=False, recursive=True))
    redirectors = [str(d.package_name) for d in assets_under('/Game') if class_name(d) == 'ObjectRedirector']
    for package in redirectors:
        E.delete_asset(package)
    for old in SEL['moves']:
        if E.does_directory_exist(old) and not E.list_assets(old, recursive=True):
            E.delete_directory(old)
    check('no redirector left', not [str(d.package_name) for d in assets_under('/Game') if class_name(d) == 'ObjectRedirector'], redirectors)


def facts(seq):
    out = {'length_s': round(float(seq.get_play_length()), 4)}
    lib = unreal.AnimationLibrary
    try:
        out['frames'] = int(lib.get_num_frames(seq))
    except Exception as error:  # a fact the Python API does not give is not a failure
        out['frames'] = 'n/a: ' + str(error)
    try:
        rate = seq.get_editor_property('target_frame_rate') if hasattr(seq, 'get_editor_property') else None
        out['frame_rate'] = '%s/%s' % (rate.numerator, rate.denominator) if rate else None
    except Exception:
        out['frame_rate'] = None
    try:
        out['root_motion'] = bool(lib.is_root_motion_enabled(seq))
    except Exception as error:
        out['root_motion'] = 'n/a: ' + str(error)
    # travel of root and pelvis over the clip, component space (the clip player reads the pelvis: S47CreatureClips.cpp)
    try:
        opts = unreal.AnimPoseEvaluationOptions()
        opts.set_editor_property('extract_root_motion', False)
        n = out['frames'] if isinstance(out['frames'], int) else 0
        if n > 1:
            first = unreal.AnimPoseExtensions.get_anim_pose_at_frame(seq, 0, opts)
            last = unreal.AnimPoseExtensions.get_anim_pose_at_frame(seq, n - 1, opts)
            for bone in ('root', 'pelvis', 'foot_l', 'foot_r'):
                a = unreal.AnimPoseExtensions.get_bone_pose(first, bone, unreal.AnimPoseSpaces.WORLD).translation
                b = unreal.AnimPoseExtensions.get_bone_pose(last, bone, unreal.AnimPoseSpaces.WORLD).translation
                out[bone + '_travel_cm'] = [round(b.x - a.x, 1), round(b.y - a.y, 1), round(b.z - a.z, 1)]
    except Exception as error:
        out['travel'] = 'n/a: ' + str(error)
    return out


def verify():
    content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    folder = content / 'Licensed' / 'Free100'
    files = sorted(folder.rglob('*.uasset'))
    check('packages on disk under Content/Licensed/Free100', len(files) > 0, str(folder))
    old = [str(p.relative_to(content)) for p in files if legacy_version(p) > -8]
    check('every package saved by UE5 (legacy file version -8 or lower)', not old, old)
    report['bytes_on_disk'] = sum(p.stat().st_size for p in folder.rglob('*') if p.is_file())
    skeleton = E.load_asset(SEL['source_skeleton'])
    check('source skeleton loads', isinstance(skeleton, unreal.Skeleton), SEL['source_skeleton'])
    mesh = E.load_asset(SEL['source_mesh'])
    check('source mesh loads on that skeleton', isinstance(mesh, unreal.SkeletalMesh) and mesh.skeleton == skeleton, SEL['source_mesh'])
    for name, path in list(SEL['clips'].items()) + list(SEL['extras'].items()):
        seq = E.load_asset(path) if E.does_asset_exist(path) else None
        if not isinstance(seq, unreal.AnimSequence):
            report['failed'][name] = 'does not load as an AnimSequence: ' + path
            continue
        if seq.get_skeleton() != skeleton:
            report['failed'][name] = 'skeleton %s, not the source skeleton' % seq.get_skeleton().get_path_name()
            continue
        report['clips'][name] = dict(facts(seq), path=path)
        flush()
    check('every chosen clip (walk, idle, run, sit) passes', not [c for c in SEL['clips'] if c in report['failed']], report['failed'])


try:
    flush()
    if MODE == 'import':
        migrate()
    elif MODE != 'verify':
        raise RuntimeError('S47_ANIMTRY_MODE must be import or verify, not ' + MODE)
    verify()
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 ANIMTRY ' + MODE.upper() + ' PASS ' + str(REPORT))
except Exception as error:
    report['status'] = 'FAIL'
    report['error'] = str(error)
    report['traceback'] = traceback.format_exc()
    flush()
    unreal.log_error('S47 ANIMTRY ' + MODE.upper() + ' FAIL ' + str(error))
