"""GAS-STATION-LIFT-01: move the Cosmos shop props (extract.py) to /Game/Licensed/CosmosShop and check them in UE 5.8.

Run ONLY in the throwaway UE 5.8 project import.sh makes (never in Signal47: the packs' own roots are not git-ignored there), the
same way as Tools/militarybarriers. S47_COSMOS_SHOP_MODE=import (default): every pack root that is there is moved to
/Game/Licensed/CosmosShop/<dest> (selection.json), Nanite is turned off on every mesh, every base material is recompiled, everything is
saved in the 5.8 format and no redirector is left; then the checks. S47_COSMOS_SHOP_MODE=verify: only the checks (a fresh process).

The checks, per chosen mesh: it loads, every slot has a material under /Game/Licensed/CosmosShop, every package it reaches is there
(no missing texture), its bounds in centimetres. That the materials compile is checked in the game (README.md). The report
(S47_COSMOS_SHOP_REPORT) holds names and measures only and may go into Git (verification.json). A mesh that fails is listed under
"failed" with why, not stopped on: import.sh and the placements leave it out. No picture is opened, exported or read here: the Cosmos
licence allows no use as reference for AI tools.
"""
import json
import os
import struct
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
MODE = os.environ.get('S47_COSMOS_SHOP_MODE', 'import')
REPORT = Path(os.environ.get('S47_COSMOS_SHOP_REPORT', str(HERE / 'verification.json')))
SEL = json.loads((HERE / 'selection.json').read_text())['packs']
DEST = '/Game/Licensed/CosmosShop'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
report = {'status': 'RUNNING', 'mode': MODE, 'engine': unreal.SystemLibrary.get_engine_version(), 'destination': DEST,
          'checks': [], 'meshes': {}, 'failed': {}, 'materials': {}}


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


def dependencies(package):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    got = registry.get_dependencies(unreal.Name(package), unreal.AssetRegistryDependencyOptions())
    if isinstance(got, tuple):
        got = got[-1]
    return [str(name) for name in (got or [])]


def legacy_version(path):
    data = path.read_bytes()[:8]
    return struct.unpack('<i', data[4:8])[0] if len(data) == 8 else 0


def migrate():
    check('throwaway project, not Signal47', not unreal.Paths.get_project_file_path().endswith('Signal47.uproject'),
          unreal.Paths.get_project_file_path())
    for key, pack in SEL.items():
        old, new = '/Game/' + pack['root'], DEST + '/' + pack['dest']
        if E.does_directory_exist(new) and E.list_assets(new, recursive=True):
            continue
        if not (E.does_directory_exist(old) and E.list_assets(old, recursive=True)):
            report.setdefault('packs_absent', []).append(key)
            continue
        for path in E.list_assets(old, recursive=True):
            E.load_asset(path)
        check('rename_directory ' + old + ' -> ' + new, E.rename_directory(old, new))
    for path in E.list_assets(DEST, recursive=True):
        asset = E.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            settings = asset.get_editor_property('nanite_settings')
            if settings.enabled:
                settings.enabled = False
                asset.set_editor_property('nanite_settings', settings)
        elif isinstance(asset, unreal.Material):
            M.recompile_material(asset)
    check('save_directory (all, 5.8 format)', E.save_directory(DEST, only_if_is_dirty=False, recursive=True))
    redirectors = [str(d.package_name) for d in assets_under('/Game') if class_name(d) == 'ObjectRedirector']
    for package in redirectors:
        E.delete_asset(package)
    for pack in SEL.values():
        if E.does_directory_exist('/Game/' + pack['root']):
            E.delete_directory('/Game/' + pack['root'])
    check('no redirector left', not [str(d.package_name) for d in assets_under('/Game') if class_name(d) == 'ObjectRedirector'], redirectors)


def material_facts(mi):
    facts = {'path': mi.get_path_name()}
    base = mi.get_base_material() if hasattr(mi, 'get_base_material') else None
    facts['base'] = base.get_path_name() if base else None
    if isinstance(mi, unreal.MaterialInstance):
        try:
            facts['scalars'] = {str(p.parameter_info.name): round(p.parameter_value, 4) for p in mi.get_editor_property('scalar_parameter_values')}
            facts['vectors'] = {str(p.parameter_info.name): [round(p.parameter_value.r, 3), round(p.parameter_value.g, 3), round(p.parameter_value.b, 3)]
                                for p in mi.get_editor_property('vector_parameter_values')}
            facts['textures'] = {str(p.parameter_info.name): p.parameter_value.get_name() if p.parameter_value else None
                                 for p in mi.get_editor_property('texture_parameter_values')}
        except Exception as error:  # a fact the Python API does not give is not a failure
            facts['params'] = 'n/a: ' + str(error)
    if base:
        try:
            st = M.get_statistics(base)
            facts['base_pixel_instructions'] = int(st.num_pixel_shader_instructions)
        except Exception as error:
            facts['base_pixel_instructions'] = 'n/a: ' + str(error)
    return facts


def verify():
    content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
    folder = content / 'Licensed' / 'CosmosShop'
    files = sorted(folder.rglob('*.uasset'))
    check('packages on disk under Content/Licensed/CosmosShop', len(files) > 0, 'Content/Licensed/CosmosShop')
    old = [str(p.relative_to(content)) for p in files if legacy_version(p) > -8]
    check('every package saved by UE5 (legacy file version -8 or lower)', not old, old)
    report['bytes_on_disk'] = sum(p.stat().st_size for p in folder.rglob('*') if p.is_file())
    registry_has = {str(d.package_name) for d in assets_under(DEST)}
    for key, pack in SEL.items():
        for rel in pack['meshes']:
            name = rel.split('/')[-1]
            path = '%s/%s/%s' % (DEST, pack['dest'], rel)
            why = []
            mesh = E.load_asset(path) if E.does_asset_exist(path) else None
            if not isinstance(mesh, unreal.StaticMesh):
                report['failed'][name] = ['does not load as a static mesh: ' + path]
                continue
            # every package it reaches is there (a missing texture shows here)
            seen, todo, missing, outside = set(), [path], [], []
            while todo:
                p = todo.pop()
                if p in seen:
                    continue
                seen.add(p)
                for d in dependencies(p):
                    if d.startswith(DEST + '/'):
                        if d not in registry_has:
                            missing.append(d)
                        else:
                            todo.append(d)
                    elif d.startswith('/Game/'):
                        outside.append(d)
            if missing:
                why.append('missing packages: %s' % sorted(set(missing)))
            if outside:
                why.append('references outside CosmosShop: %s' % sorted(set(outside)))
            slots = []
            for s in mesh.get_editor_property('static_materials'):
                mi = s.get_editor_property('material_interface')
                slots.append({'slot': str(s.get_editor_property('material_slot_name')), 'material': mi.get_path_name() if mi else None})
                if not mi or not mi.get_path_name().startswith(DEST + '/'):
                    why.append('slot %s: material %s' % (slots[-1]['slot'], slots[-1]['material']))
                elif mi.get_name() not in report['materials']:
                    # (the instruction counts are facts only: with -nullrhi no shader is compiled here, so they read 0; that the
                    # materials compile is seen in the game's log, S47 "Failed to compile Material" lines for CosmosShop)
                    report['materials'][mi.get_name()] = material_facts(mi)
            b = mesh.get_bounding_box()
            size = [round(b.max.x - b.min.x, 1), round(b.max.y - b.min.y, 1), round(b.max.z - b.min.z, 1)]
            report['meshes'][name] = {'pack': key, 'path': path, 'bounds_cm': [[round(b.min.x, 1), round(b.min.y, 1), round(b.min.z, 1)],
                                      [round(b.max.x, 1), round(b.max.y, 1), round(b.max.z, 1)]], 'size_cm': size,
                                      'triangles_lod0': mesh.get_num_triangles(0), 'lods': mesh.get_num_lods(), 'slots': slots,
                                      'nanite': bool(mesh.get_editor_property('nanite_settings').enabled), 'packages': len(seen)}
            if max(size) <= 0.5 or max(size) > 400:
                why.append('size %s cm is not a shop prop' % size)
            if why:
                report['failed'][name] = why
            flush()
    report['passed'] = sorted(set(report['meshes']) - set(report['failed']))
    check('at least ten meshes pass', len(report['passed']) >= 10, report['failed'])


try:
    flush()
    if MODE == 'import':
        migrate()
    elif MODE != 'verify':
        raise RuntimeError('S47_COSMOS_SHOP_MODE must be import or verify, not ' + MODE)
    verify()
    report['status'] = 'PASS'
    flush()
    unreal.log('S47 COSMOS SHOP ' + MODE.upper() + ' PASS ' + str(REPORT))
except Exception as error:
    report['status'] = 'FAIL'
    report['error'] = str(error)
    report['traceback'] = traceback.format_exc()
    flush()
    unreal.log_error('S47 COSMOS SHOP ' + MODE.upper() + ' FAIL ' + str(error))
