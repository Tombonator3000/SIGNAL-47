"""D45-20261110 (D45-14): migrate the Cosmos/Leartes Military Barriers wired fence (a UE 4.22 pack) to UE 5.8 for S47ArrayFence.

Run ONLY in a throwaway UE 5.8 project (README.md: ~/scratch-models/fence58/Fence58.uproject), never in Signal47: the pack's own root
/Game/MilitaryBarriers is not git-ignored there. The script moves /Game/MilitaryBarriers to /Game/Licensed/MilitaryBarriers (the path
S47ArrayFence loads, ignored by Git in Signal47), leaves no redirector, turns Nanite off on every mesh, compiles MM_Basic_PBR for instanced
static meshes (S47ArrayFence draws the panels instanced), resaves every package in the 5.8 format and writes verification.json with every
mesh's bounds and the frame checks the runtime makes. The licensed files themselves never go into Git.

S47_FENCE_MODE=import (default): migrate, save, then verify. S47_FENCE_MODE=verify: only verify (run it in a second, fresh process).
S47_FENCE_REPORT: the report's path (default: verification.json next to this script).
"""
import json
import os
from pathlib import Path
import struct
import traceback

import unreal

HERE = Path(__file__).resolve().parent
MODE = os.environ.get('S47_FENCE_MODE', 'import')
REPORT = Path(os.environ.get('S47_FENCE_REPORT', str(HERE / 'verification.json')))
OLD = '/Game/MilitaryBarriers'
DEST = '/Game/Licensed/MilitaryBarriers'
MASTER = DEST + '/Materials/MasterMaterial/MM_Basic_PBR'
# what S47ArrayFence uses; the rest of the extracted kit comes along (small) but is not placed
USED = ('SM_MilitaryBase_WiredFence_b', 'SM_MilitaryBase_WiredFence_EndCap_c', 'SM_MilitaryBase_WiredFence_Gate_R',
        'SM_MilitaryBase_WiredFence_Gate_L', 'SM_MilitaryBase_Wire_b', 'SM_MilitaryBase_Wire_a')
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
report = {'status': 'RUNNING', 'mode': MODE, 'engine': unreal.SystemLibrary.get_engine_version(), 'destination': DEST,
          'checks': [], 'meshes': {}, 'materials': {}, 'packages': {}}


def flush():
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True))


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


def dependencies(package):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    got = registry.get_dependencies(unreal.Name(package), unreal.AssetRegistryDependencyOptions())
    if isinstance(got, tuple):
        got = got[-1]
    return [str(name) for name in (got or [])]


def instancing(material):
    usage = getattr(unreal, 'MaterialUsage', None)
    flag = getattr(usage, 'MATUSAGE_INSTANCED_STATIC_MESHES', None) if usage else None
    if flag is not None:
        return bool(M.has_material_usage(material, flag))
    return bool(material.get_editor_property('used_with_instanced_static_meshes'))


def legacy_version(path):
    # FPackageFileSummary: tag, then LegacyFileVersion; UE 4.22 wrote -7, UE5 writes -8 or lower
    data = path.read_bytes()[:8]
    return struct.unpack('<i', data[4:8])[0] if len(data) == 8 else 0


def box_cm(mesh):
    b = mesh.get_bounding_box()
    return [[round(b.min.x, 2), round(b.min.y, 2), round(b.min.z, 2)], [round(b.max.x, 2), round(b.max.y, 2), round(b.max.z, 2)]]


def migrate():
    check('throwaway project, not Signal47', not unreal.Paths.get_project_file_path().endswith('Signal47.uproject'),
          unreal.Paths.get_project_file_path())
    if not E.does_directory_exist(DEST):
        check('the 4.22 pack is at ' + OLD, E.does_directory_exist(OLD) and len(E.list_assets(OLD, recursive=True)) > 0)
        # every asset loaded first, so the rename rewrites every reference in memory (RenameDirectory loads them too)
        for path in E.list_assets(OLD, recursive=True):
            E.load_asset(path)
        check('rename_directory ' + OLD + ' -> ' + DEST, E.rename_directory(OLD, DEST))
    master = E.load_asset(MASTER)
    check('MM_Basic_PBR moved', isinstance(master, unreal.Material), MASTER)
    # S47ArrayFence draws panels, posts and strands as instanced static meshes
    master.set_editor_property('used_with_instanced_static_meshes', True)
    errors = M.recompile_material(master)
    check('MM_Basic_PBR compiles for instanced static meshes (its UE4 material functions included)', not list(errors or []), list(errors or []))
    check('MM_Basic_PBR saved', E.save_loaded_asset(master, False))
    for path in E.list_assets(DEST, recursive=True):
        asset = E.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            settings = asset.get_editor_property('nanite_settings')
            settings.enabled = False
            asset.set_editor_property('nanite_settings', settings)
    # every package in the 5.8 format (a 4.22 package would be converted again on every cook)
    check('save_directory (all, 5.8 format)', E.save_directory(DEST, only_if_is_dirty=False, recursive=True))
    # no redirector left behind, anywhere in the project (the references were rewritten above and saved)
    redirectors = [str(data.package_name) for data in assets_under('/Game') if class_name(data) == 'ObjectRedirector']
    for package in redirectors:
        E.delete_asset(package)
    if E.does_directory_exist(OLD):
        E.delete_directory(OLD)
    check('no redirector left', not [str(d.package_name) for d in assets_under('/Game') if class_name(d) == 'ObjectRedirector'], redirectors)


def verify():
    content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    folder = content / 'Licensed' / 'MilitaryBarriers'
    files = sorted(folder.rglob('*.uasset'))
    check('packages on disk under Content/Licensed/MilitaryBarriers', len(files) > 0, str(folder))
    old = [str(p.relative_to(content)) for p in files if legacy_version(p) > -8]
    for p in files:
        report['packages'][str(p.relative_to(content))] = {'bytes': p.stat().st_size, 'legacy_file_version': legacy_version(p)}
    check('every package saved by UE5 (legacy file version -8 or lower)', not old, old)
    check('nothing left at ' + OLD, not (content / 'MilitaryBarriers').exists() or not list((content / 'MilitaryBarriers').rglob('*.uasset')))
    outside = {}
    for data in assets_under(DEST):
        package = str(data.package_name)
        bad = [d for d in dependencies(package) if not (d.startswith(DEST + '/') or d.startswith('/Engine/') or d.startswith('/Script/'))]
        if bad:
            outside[package] = bad
    check('every reference stays inside /Game/Licensed/MilitaryBarriers or the engine', not outside, outside)
    master = E.load_asset(MASTER)
    check('MM_Basic_PBR used with instanced static meshes', master and instancing(master))
    for path in E.list_assets(DEST + '/Materials', recursive=True):
        asset = E.load_asset(path)
        if isinstance(asset, unreal.MaterialInterface):
            parent = asset.get_editor_property('parent') if isinstance(asset, unreal.MaterialInstance) else None
            report['materials'][asset.get_name()] = {'path': asset.get_path_name(), 'parent': parent.get_path_name() if parent else None,
                                                     'instancing': instancing(asset)}
            check(asset.get_name() + ' instancing', instancing(asset))
    for path in E.list_assets(DEST + '/Meshes', recursive=True):
        mesh = E.load_asset(path)
        if not isinstance(mesh, unreal.StaticMesh):
            continue
        name = mesh.get_name()
        slots = [{'slot': str(s.get_editor_property('material_slot_name')),
                  'material': s.get_editor_property('material_interface').get_path_name() if s.get_editor_property('material_interface') else None}
                 for s in mesh.get_editor_property('static_materials')]
        report['meshes'][name] = {'bounds_cm': box_cm(mesh), 'triangles_lod0': mesh.get_num_triangles(0), 'lods': mesh.get_num_lods(),
                                  'nanite': bool(mesh.get_editor_property('nanite_settings').enabled), 'slots': slots}
        check(name + ' Nanite off', not mesh.get_editor_property('nanite_settings').enabled)
        check(name + ' materials under ' + DEST, all(s['material'] and s['material'].startswith(DEST + '/') for s in slots), slots)
    check('the meshes S47ArrayFence uses', all(n in report['meshes'] for n in USED), sorted(report['meshes']))
    # the frame S47ArrayFence checks before it places anything (S47ArrayFence.cpp PostFrame/PanelFrame/StrandFrame/GateFrame)
    (pmin, pmax) = report['meshes']['SM_MilitaryBase_WiredFence_EndCap_c']['bounds_cm']
    half = pmax[0]
    check('post: centred on its pivot, on its foot, 2.8 m with its arm', abs(pmin[0] + pmax[0]) < 1 and 1 < pmax[0] < 10 and abs(pmin[2]) < 1.5 and pmax[2] > 150 and pmin[1] < -5,
          [pmin, pmax])
    (amin, amax) = report['meshes']['SM_MilitaryBase_WiredFence_b']['bounds_cm']
    check('panel: its post at the pivot, 1.5-4 m to the next post along +x, the arm to -y', abs(amin[0] + half) < 1.5 and 150 < amax[0] < 400 and abs(amin[2]) < 1.5
          and 150 < amax[2] < 400 and amin[1] < -5 and abs(amax[1] - half) < 1.5, [amin, amax])
    (wmin, wmax) = report['meshes']['SM_MilitaryBase_Wire_b']['bounds_cm']
    check('barbed strand: along +x, standing up, centred', wmax[0] > 100 and abs(wmin[2]) < 1.5 and 10 < wmax[2] < 80 and abs(wmin[1] + wmax[1]) < 1.5, [wmin, wmax])
    for gate in ('SM_MilitaryBase_WiredFence_Gate_R', 'SM_MilitaryBase_WiredFence_Gate_L'):
        (gmin, gmax) = report['meshes'][gate]['bounds_cm']
        check(gate + ': over 1.5 m wide, on its foot', gmax[0] - gmin[0] > 150 and abs(gmin[2]) < 1.5 and gmax[2] > 150, [gmin, gmax])
    report['panel_module_m'] = amax[0] / 100


try:
    flush()
    if MODE == 'import':
        migrate()
    elif MODE != 'verify':
        raise RuntimeError('S47_FENCE_MODE must be import or verify, not ' + MODE)
    verify()
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 MILITARYBARRIERS FENCE ' + MODE.upper() + ' PASS ' + str(REPORT))
except Exception as error:
    report['status'] = 'FAIL'
    report['error'] = str(error)
    report['traceback'] = traceback.format_exc()
    flush()
    unreal.log_error('S47 MILITARYBARRIERS FENCE ' + MODE.upper() + ' FAIL ' + str(error))
    raise
