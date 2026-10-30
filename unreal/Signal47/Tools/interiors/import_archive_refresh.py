"""Refresh only the eleven original ArchiveProps and their assembly measurements.

Run in root's reserved UE process with the established legacy FBX importer.
S47_ARCHIVE_VERIFY_ONLY=1 or -S47ArchiveRefreshVerifyOnly checks saved assets
without asset/assembly mutation. This script never launches an editor, imports
other families, rebuilds trim patches, changes use IDs, or retires scene nodes.
"""
from pathlib import Path
import copy
import hashlib
import importlib.util
import json
import os
import stat
import sys
import tempfile
import traceback

import unreal

HERE = Path(__file__).resolve().parent
PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = Path(os.environ.get('S47_ARCHIVE_SOURCE', str(HERE.parent / 'archiveprops'))).resolve()
ASSEMBLY = PROJECT / 'Content/FieldModels/assembly.json'
DEST = '/Game/ArchiveProps'
NAMES = {'SM_ArchiveProps_' + suffix for suffix in (
    'ShelvingStorage', 'FilingCabinets', 'CardIndex', 'Bookcase', 'WorkTable',
    'FieldBox', 'FireDrawer', 'Reel047Box', 'Switchboard', 'CallRecorder', 'ServiceBinder')}
MUTABLE_FIELDS = {'materials', 'lod_triangles', 'bounds_local_m'}
VERIFY_ONLY = (os.environ.get('S47_ARCHIVE_VERIFY_ONLY') == '1' or
               'S47ArchiveRefreshVerifyOnly' in unreal.SystemLibrary.get_command_line())
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
sys.path.insert(0, str(HERE))
import import_recipe as R
import verify_archive_lod_axes as LOD

R.REPORT = Path(unreal.Paths.project_saved_dir()) / 'archive-refresh-native.json'
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(),
            'mode': 'verify_saved_only' if VERIFY_ONLY else 'archive_import_and_refresh',
            'source': str(SOURCE), 'checks': [], 'meshes': [], 'asset_root': DEST,
            'runtime_appearance': 'UNVERIFIED', 'installed_game': 'UNCHANGED'}
R.TEXTURE_CACHE.clear()
R.flush()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def raw_members(text, start):
    """Value spans of one JSON object; preserve all unrelated source bytes."""
    decoder = json.JSONDecoder()
    skip = lambda pos: pos + len(text[pos:]) - len(text[pos:].lstrip())
    assert text[start] == '{'
    cursor, result = skip(start + 1), {}
    while text[cursor] != '}':
        key, end = decoder.raw_decode(text, cursor)
        assert isinstance(key, str) and key not in result, 'Duplicate or invalid assembly key'
        cursor = skip(end)
        assert text[cursor] == ':'
        first = skip(cursor + 1)
        _, last = decoder.raw_decode(text, first)
        result[key] = (first, last)
        cursor = skip(last)
        if text[cursor] == ',':
            cursor = skip(cursor + 1)
        else:
            assert text[cursor] == '}'
    return result


def raw_parts(text):
    top = raw_members(text, len(text) - len(text.lstrip()))
    first, _ = top['parts']
    assert text[first] == '['
    decoder = json.JSONDecoder()
    cursor, parts = first + 1, []
    while True:
        cursor += len(text[cursor:]) - len(text[cursor:].lstrip())
        if text[cursor] == ']':
            break
        row, last = decoder.raw_decode(text, cursor)
        parts.append((row, cursor, last))
        cursor = last
        cursor += len(text[cursor:]) - len(text[cursor:].lstrip())
        if text[cursor] == ',':
            cursor += 1
        else:
            assert text[cursor] == ']'
    return top, parts


def patch_assembly_values(text, candidate):
    """Replace just the three permitted values in existing archive rows."""
    top, parts = raw_parts(text)
    edits = []
    for index, (before, first, _) in enumerate(parts):
        after = candidate['parts'][index]
        assert before['name'] == after['name']
        if before['name'] not in NAMES:
            assert before == after
            continue
        assert {key: value for key, value in before.items() if key not in MUTABLE_FIELDS} == {
            key: value for key, value in after.items() if key not in MUTABLE_FIELDS}
        fields = raw_members(text, first)
        for key in MUTABLE_FIELDS:
            if before[key] == after[key]:
                continue
            start, end = fields[key]
            line = text[text.rfind('\n', 0, start) + 1:start]
            indent = ' ' * (len(line) - len(line.lstrip(' ')))
            replacement = json.dumps(after[key], indent=2, ensure_ascii=False).replace('\n', '\n' + indent)
            edits.append((start, end, replacement))
    updated = text
    for start, end, value in sorted(edits, reverse=True):
        updated = updated[:start] + value + updated[end:]
    assert json.loads(updated) == candidate
    new_top, new_parts = raw_parts(updated)
    assert len(parts) == len(new_parts)
    for key, (start, end) in top.items():
        if key != 'parts':
            a, b = new_top[key]
            assert text[start:end] == updated[a:b], 'Unrelated assembly field changed: ' + key
    for (row, start, end), (_, new_start, new_end) in zip(parts, new_parts):
        if row['name'] not in NAMES:
            assert text[start:end] == updated[new_start:new_end], 'Unrelated part bytes changed'
        else:
            old_fields, new_fields = raw_members(text, start), raw_members(updated, new_start)
            for key in old_fields.keys() - MUTABLE_FIELDS:
                a, b = old_fields[key]
                c, d = new_fields[key]
                assert text[a:b] == updated[c:d], 'Archive anchor bytes changed: ' + key
    return updated.encode('utf-8'), len(edits)


def validate_inputs(assembly):
    spec = importlib.util.spec_from_file_location('s47_archive_refresh_design', SOURCE / 'design.py')
    design = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(design)
    contract = json.loads((SOURCE / 'model_contract.json').read_text())
    materials = json.loads((SOURCE / 'materials.json').read_text())
    proof_path = SOURCE / 'verification.json'
    proof = json.loads(proof_path.read_text())
    texture_proof = json.loads((SOURCE / 'texture_authoring.json').read_text())
    policy = json.loads((HERE.parent / 'fieldintegration/runtime_policy.json').read_text())
    R.check('Current source revision and exact eleven archive names',
            contract['source_revision'] == design.REVISION and set(contract['parts']) == NAMES == set(design.PARTS))
    R.check('Source preserves dynamic handles and original story hierarchy',
            contract['native_dynamic'] == design.NATIVE_DYNAMIC and contract['preserve_handles'] == design.PRESERVE_HANDLES)
    R.check('Source export contract follows current model recipes',
            (SOURCE / 'model_contract.json').stat().st_mtime_ns >= max((SOURCE / name).stat().st_mtime_ns
             for name in ('design.py', 'geometry.py', 'build_archive.py')))
    R.check('Current original paper and PBR textures accompany this model revision',
            texture_proof.get('source') == design.REVISION and texture_proof.get('external_assets') is False and
            texture_proof.get('ordinary_label_subcells') == {'cell': 14, 'grid': [2, 2]} and
            texture_proof.get('packed_texture_count') == 18 and
            (SOURCE / 'materials.json').stat().st_mtime_ns >= (SOURCE / 'make_textures.py').stat().st_mtime_ns)
    R.check('All exported source verification checks pass at this revision', proof.get('status') == 'PASS' and
            proof.get('source_revision') == design.REVISION and proof.get('check_count') == len(proof.get('checks', [])) and
            bool(proof.get('checks')) and all(row['pass'] for row in proof['checks']))
    R.check('Assembly version and declared population agree', assembly['version'] == 1 and
            assembly['part_count'] == len(assembly['parts']))
    old = {row['name']: row for row in assembly['parts']}
    R.check('Unique existing names with exactly eleven archive rows', len(old) == len(assembly['parts']) and
            {name for name in old if name.startswith('SM_ArchiveProps_')} == NAMES)
    material_rows = {row['name']: row for row in materials}
    R.check('Only original archive material namespace', len(material_rows) == len(materials) and
            all(name.startswith('M_ArchiveProps_') for name in material_rows))
    paths = {SOURCE / name for name in ('model_contract.json', 'materials.json', 'verification.json',
                                      'design.py', 'geometry.py', 'build_archive.py', 'make_textures.py', 'texture_authoring.json')}
    for name, row in contract['parts'].items():
        local = bool(row.get('local', False))
        position = row.get('native_local_three', [0, 0, 0]) if local else row['site_world_three_m']
        expected = {'asset': DEST + '/' + name + '.' + name, 'parent_handle': row['parent_handle'],
                    'at': position, 'local': local, 'yaw': float(row.get('runtime_yaw_degrees', 0)),
                    'use': policy.get('use', {}).get(name, '')}
        R.check(name + ' original asset parent local position yaw and use preserved',
                all(old[name].get(key) == value for key, value in expected.items()), expected)
        R.check(name + ' Source material population exists', set(row['materials']) <= set(material_rows))
        for lod in range(3):
            suffix = '' if lod == 0 else '_LOD' + str(lod)
            for extension in ('.fbx', '.glb'):
                path = SOURCE / 'exports' / (name + suffix + extension)
                R.check(name + ' source export exists ' + str(lod) + extension, path.is_file())
                paths.add(path)
            R.check(name + ' explicit LOD contract agrees ' + str(lod),
                    contract['lods'][str(lod)][name]['triangles'] == row['lod_triangles'][lod])
    for row in materials:
        R.check(row['name'] + ' original material is nonemissive', row.get('emission_strength', 0) == 0 and
                max(abs(value) for value in row.get('emission', [0, 0, 0])[:3]) == 0)
        for relative in row.get('textures', {}).values():
            path = (SOURCE / relative).resolve()
            R.check(row['name'] + ' contained original texture', path.is_relative_to(SOURCE) and path.is_file())
            paths.add(path)
    paths.add(SOURCE / 'exports/axis_probe.fbx')
    R.check('Exactly eighteen original packed texture files retained', len({path for path in paths if path.suffix == '.png'}) == 18)
    R.check('Source verification follows all current exports and textures',
            all(path.is_file() and path.stat().st_mtime_ns <= proof_path.stat().st_mtime_ns
                for path in paths if path.suffix in ('.fbx', '.glb', '.png')))
    R.report['counts'] = {'total_parts': len(old), 'archive_parts': len(NAMES), 'other_parts': len(old) - len(NAMES),
                          'patches': len(assembly.get('patches', [])), 'live_bindings': len(assembly.get('bindings', []))}
    return contract, material_rows, {str(path.relative_to(SOURCE)): sha(path) for path in sorted(paths)}


def archive_mesh(name, destination):
    """Refresh only the eleven generated meshes and the exact axis probe.

    Atomic FBX reimport can retain an earlier derived source description in
    UE5.8. Match the existing field/Office importers by recreating only the
    named generated mesh; keep materials and every other asset untouched.
    """
    R.check(name + ' exact generated archive mesh destination',
            (name in NAMES and destination == DEST) or
            (name == 'axis_probe' and destination == DEST + '/Verification'),
            {'name': name, 'destination': destination})
    asset = destination + '/' + name
    if VERIFY_ONLY:
        return R.E.load_asset(asset)
    if R.E.does_asset_exist(asset):
        existing = R.E.load_asset(asset)
        R.check(name + ' replace only the exact generated StaticMesh',
                isinstance(existing, unreal.StaticMesh) and
                existing.get_path_name() == asset + '.' + name, asset)
        R.check(name + ' exact generated mesh removed before fresh import', R.E.delete_asset(asset))
    return R.import_mesh(SOURCE / 'exports' / (name + '.fbx'), destination)


def mesh_measurements(name, mesh, source, materials, lod_contract):
    R.check(name + ' native mesh exists', isinstance(mesh, unreal.StaticMesh))
    slots = mesh.get_editor_property('static_materials')
    names = [str(slot.material_slot_name) for slot in slots]
    R.check(name + ' exact actual material population', len(names) == len(set(names)) and set(names) == set(source['materials']))
    R.check(name + ' saved named material bindings', all(slot.material_interface == materials[str(slot.material_slot_name)] for slot in slots))
    triangles = [mesh.get_num_triangles(lod) for lod in range(mesh.get_num_lods())]
    R.check(name + ' exact three authored LODs', triangles == source['lod_triangles'], triangles)
    bound = mesh.get_bounds()
    centre = [bound.origin.x, bound.origin.y, bound.origin.z]
    extent = [bound.box_extent.x, bound.box_extent.y, bound.box_extent.z]
    low, high = source['bounds_local_m']
    expected = [(low[0] + high[0]) * 50, -(low[1] + high[1]) * 50, (low[2] + high[2]) * 50]
    R.check(name + ' source metric pivot and dimensions',
            max(abs(a - b) for a, b in zip(centre, expected)) < .35 and
            max(abs(extent[k] - (high[k] - low[k]) * 50) for k in range(3)) < .35)
    R.check(name + ' saved ray tracing and non Nanite mesh', mesh.get_editor_property('support_ray_tracing') and
            not mesh.get_editor_property('nanite_settings').enabled)
    body = mesh.get_editor_property('body_setup')
    R.check(name + ' original complex interaction query geometry', body is not None and
            body.get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for lod in range(3):
        render = LOD.measure_render_lod(mesh, lod, lod_contract[str(lod)][name])
        R.report.setdefault('actual_render_lod_geometry', []).append({'name': name, **render})
        R.check(name + ' actual render axes position and topology LOD ' + str(lod), render['pass'], render)
        build = R.S.get_lod_build_settings(mesh, lod)
        R.check(name + ' saved UV precision LOD ' + str(lod), build.get_editor_property('use_full_precision_u_vs') and
                not build.get_editor_property('remove_degenerates') and R.S.get_num_uv_channels(mesh, lod) >= 1)
        count = mesh.get_num_sections(lod)
        R.check(name + ' actual visibility query sections LOD ' + str(lod), count > 0 and
                all(R.S.is_section_collision_enabled(mesh, lod, section) for section in range(count)))
    # Convert actual native centimetre bounds back to the assembly's Blender metres.
    bounds = [[(centre[0] - extent[0]) / 100, -(centre[1] + extent[1]) / 100, (centre[2] - extent[2]) / 100],
              [(centre[0] + extent[0]) / 100, -(centre[1] - extent[1]) / 100, (centre[2] + extent[2]) / 100]]
    result = {'materials': names, 'lod_triangles': triangles, 'bounds_local_m': bounds}
    R.report['meshes'].append({'name': name, 'asset': mesh.get_path_name(), **result})
    return result


previous_interchange = None
try:
    R.report['recipe_sha256'] = sha(Path(__file__))
    before = ASSEMBLY.read_bytes()
    assembly = json.loads(before)
    contract, rows, hashes = validate_inputs(assembly)
    # Fail before material/mesh mutation if the render-reader plugin is absent.
    LOD.require_render_api()
    if not VERIFY_ONLY:
        R.check('Global Interchange override is restricted to the import commandlet',
                '-run=pythonscript' in unreal.SystemLibrary.get_command_line().lower())
        variable = 'Interchange.FeatureFlags.Import.Enable'
        R.check('Installed global Interchange variable exists',
                bool(unreal.SystemLibrary.get_console_variable_string_value(variable)))
        previous_interchange = unreal.SystemLibrary.get_console_variable_int_value(variable)
        unreal.SystemLibrary.execute_console_command(None, variable + ' 0')
        R.check('Custom LOD import uses the same legacy FBX transform as LOD0',
                unreal.SystemLibrary.get_console_variable_int_value(variable) == 0,
                {'variable': variable, 'previous_value': previous_interchange, 'import_value': 0,
                 'scope': 'This Python import commandlet only; restored on exit'})
    R.report['source_sha256'] = hashes
    R.report['assembly_before_sha256'] = hashlib.sha256(before).hexdigest()
    materials = {}
    for name, row in rows.items():
        mat = R.E.load_asset(DEST + '/Materials/' + name) if VERIFY_ONLY else R.material(row, DEST + '/Materials', SOURCE)
        R.check(name + ' saved original material exists', isinstance(mat, unreal.Material) and
                mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE)
        materials[name] = mat
    measured = {}
    for name, source in contract['parts'].items():
        mesh = archive_mesh(name, DEST)
        R.check(name + ' native archive mesh available', isinstance(mesh, unreal.StaticMesh))
        if not VERIFY_ONLY:
            for lod in (1, 2):
                path = SOURCE / 'exports' / (name + '_LOD' + str(lod) + '.fbx')
                R.check(name + ' explicit native LOD imported ' + str(lod), R.S.import_lod(mesh, lod, str(path)) == lod)
            for index, slot in enumerate(mesh.get_editor_property('static_materials')):
                label = str(slot.material_slot_name)
                R.check(name + ' known original material slot ' + label, label in materials)
                mesh.set_material(index, materials[label])
            nanite = mesh.get_editor_property('nanite_settings')
            nanite.enabled = False
            mesh.set_editor_property('nanite_settings', nanite)
            mesh.set_editor_property('support_ray_tracing', True)
            body = mesh.get_editor_property('body_setup')
            R.check(name + ' body setup exists', body is not None)
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            for lod in range(3):
                build = R.S.get_lod_build_settings(mesh, lod)
                build.set_editor_property('use_full_precision_u_vs', True)
                build.set_editor_property('remove_degenerates', False)
                R.S.set_lod_build_settings(mesh, lod, build)
                for section in range(mesh.get_num_sections(lod)):
                    R.S.enable_section_collision(mesh, True, lod, section)
            R.check(name + ' native mesh saved', R.E.save_loaded_asset(mesh))
        measured[name] = mesh_measurements(name, mesh, source, materials, contract['lods'])
    probe = archive_mesh('axis_probe', DEST + '/Verification')
    R.check('Saved asymmetric archive axis probe exists', isinstance(probe, unreal.StaticMesh))
    bound = probe.get_bounds()
    actual = [bound.origin.x, bound.origin.y, bound.origin.z,
              bound.box_extent.x * 2, bound.box_extent.y * 2, bound.box_extent.z * 2]
    R.check('Actual asymmetric FBX origin and scale probe',
            max(abs(a - b) for a, b in zip(actual, [200, -400, 600, 20, 40, 60])) < .35, actual)
    candidate = copy.deepcopy(assembly)
    for row in candidate['parts']:
        if row['name'] in measured:
            row.update(measured[row['name']])
    for binding in candidate.get('bindings', []):
        row = next(row for row in candidate['parts'] if row['name'] == binding['part'])
        R.check('Existing live material binding still addresses the same slot',
                row['materials'][binding['slot']] == binding['material'], binding)
    updated, edits = patch_assembly_values(before.decode('utf-8'), candidate)
    R.check('Source bytes remained unchanged throughout native work', all(sha(SOURCE / path) == value for path, value in hashes.items()))
    R.check('Shared assembly was not changed by another owner', ASSEMBLY.read_bytes() == before)
    if VERIFY_ONLY:
        R.check('Saved assembly already matches all eleven measured native meshes', candidate == assembly)
        R.check('Verify only leaves assembly byte identical', ASSEMBLY.read_bytes() == before)
    elif edits:
        backup = R.REPORT.parent / ('archive-refresh-assembly-before-' + hashlib.sha256(before).hexdigest()[:16] + '.json')
        if backup.exists():
            R.check('Existing assembly backup is exact', backup.read_bytes() == before)
        else:
            with backup.open('xb') as stream:
                stream.write(before)
        descriptor, temporary = tempfile.mkstemp(prefix='.archive-refresh-', dir=ASSEMBLY.parent)
        try:
            with os.fdopen(descriptor, 'wb') as stream:
                stream.write(updated)
                stream.flush()
                os.fsync(stream.fileno())
            os.chmod(temporary, stat.S_IMODE(ASSEMBLY.stat().st_mode))
            R.check('Assembly compare before atomic publication', ASSEMBLY.read_bytes() == before)
            os.replace(temporary, ASSEMBLY)
        finally:
            Path(temporary).unlink(missing_ok=True)
        R.check('Published only validated archive value replacements', ASSEMBLY.read_bytes() == updated)
        R.report['assembly_backup'] = str(backup)
    R.report['assembly_after_sha256'] = sha(ASSEMBLY)
    R.report['archive_value_replacements'] = edits
    R.report['preservation'] = 'Other part objects, patches, live bindings and immutable archive anchor values retain exact JSON bytes'
    R.report['status'] = 'PASS_SAVED_ARCHIVE_REFRESH_VERIFY' if VERIFY_ONLY else 'PASS_ARCHIVE_ONLY_NATIVE_REFRESH'
    R.flush()
    unreal.log('S47 archive-only native refresh PASS: 11 archive parts; other ' +
               str(R.report['counts']['other_parts']) + ' parts and all patches/bindings preserved')
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    raise
finally:
    if previous_interchange is not None:
        unreal.SystemLibrary.execute_console_command(
            None, 'Interchange.FeatureFlags.Import.Enable ' + str(previous_interchange))
