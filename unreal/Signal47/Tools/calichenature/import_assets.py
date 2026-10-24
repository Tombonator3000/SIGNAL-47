"""UE Python importer for original Caliche nature sources. Never launches an editor.

Reuse the proven street material/legacy-FBX functions without importing its source
sets, atmosphere or trims. Each nature base mesh is freshly imported at its stable
owned path. Authored FBXs become LODs; no automatic mesh reduction is requested.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import runpy
import traceback
import unreal

ROOT = Path(__file__).resolve().parent
SETS = {'ground': '/Game/CalicheStreet/NatureGround',
        'vegetation': '/Game/CalicheStreet/NatureVegetation'}
DEFAULT_SCREENS = [1.0, .12, .025]
DEFAULT_CULL_M = {'grass': 90, 'sage': 220, 'yucca': 260, 'juniper': 1200, 'stone': 110}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def labels():
    result = os.environ.get('S47_NATURE_ONLY', 'ground,vegetation').split(',')
    if not result or len(set(result)) != len(result) or any(label not in SETS for label in result):
        raise ValueError('S47_NATURE_ONLY must select ground and/or vegetation exactly once')
    return result


def material_rows(src):
    value = json.loads((src / 'materials.json').read_text())
    rows = [dict(row, name=name) for name, row in value.items()] if isinstance(value, dict) else value
    if not rows or len({row['name'] for row in rows}) != len(rows):
        raise ValueError('Missing/duplicate material definitions: ' + str(src))
    return rows


def levels(name, row):
    result = row.get('lods') or [dict(row, name=name, lod_index=0, fbx='exports/' + name + '.fbx')]
    if not isinstance(result, list) or [int(level.get('lod_index', index)) for index, level in enumerate(result)] != list(range(len(result))):
        raise ValueError('Authored LOD indices must be contiguous from zero: ' + name)
    return result


def screens(contract, row, count):
    value = row.get('screen_sizes', contract.get('screen_sizes'))
    if value is None:
        value = json.loads(os.environ['S47_NATURE_SCREENS']) if 'S47_NATURE_SCREENS' in os.environ else DEFAULT_SCREENS[:count]
    result = [float(number) for number in value]
    if len(result) != count or not all(math.isfinite(x) and 0 < x <= 1 for x in result) or any(a <= b for a, b in zip(result, result[1:])):
        raise ValueError('Provide one strictly descending explicit screen size per LOD')
    return result


def cull_policy(contract, row):
    # Culling is per runtime component, not a StaticMesh asset setting. Persist
    # the explicit policy for the HISM owner; do not claim it was applied here.
    family = row.get('family', '')
    overrides = json.loads(os.environ.get('S47_NATURE_CULL_METERS', '{}'))
    value = row.get('cull_m', overrides.get(family, contract.get('cull_m', DEFAULT_CULL_M.get(family))))
    if value is None:
        return None
    # Match the actual HISM SetCullDistances(end*.82, end) runtime policy.
    if isinstance(value, (int, float)):
        end_cm = math.floor(float(value) * 100 + .5)
        result = [math.floor(end_cm * .82 + .5) / 100, end_cm / 100]
    else:
        result = [float(x) for x in value]
    if len(result) != 2 or not all(math.isfinite(x) for x in result) or not 0 <= result[0] <= result[1] or result[1] == 0:
        raise ValueError('Invalid explicit cull distance policy')
    return result


def source_path(src, name, level):
    path = (src / level.get('fbx', 'exports/' + name + '.fbx')).resolve()
    if not path.is_relative_to(src.resolve()) or not path.is_file() or path.suffix.lower() != '.fbx':
        raise ValueError('Missing/out-of-source FBX: ' + str(path))
    return path


def preflight(selected):
    result = {}
    for label in selected:
        src = ROOT / label
        contract = json.loads((src / 'model_contract.json').read_text())
        rows = material_rows(src)
        materials = {row['name'] for row in rows}
        parts = contract['parts']
        if not parts:
            raise ValueError('Empty source contract: ' + label)
        if label == 'vegetation' and len(parts) != 12:
            raise ValueError('Vegetation contract must have twelve main prototypes')
        for row in rows:
            for filename in row.get('textures', {}).values():
                path = (src / filename).resolve()
                if not path.is_relative_to(src.resolve()) or not path.is_file():
                    raise ValueError('Missing/out-of-source texture: ' + str(path))
        files = {str(Path(filename)): sha(src / filename)
                 for row in rows for filename in row.get('textures', {}).values()}
        for name, row in parts.items():
            if not name.startswith('SM_CN_') or '_LOD' in name:
                raise ValueError('Contract parts must list only SM_CN_ base meshes: ' + name)
            lods = levels(name, row)
            if label == 'vegetation' and len(lods) != 3:
                raise ValueError('Vegetation requires three authored levels: ' + name)
            screens(contract, row, len(lods))
            cull_policy(contract, row)
            for index, level in enumerate(lods):
                expected_name = name if index == 0 else name + '_LOD' + str(index)
                if level.get('name', expected_name) != expected_name or int(level['triangles']) <= 0:
                    raise ValueError('Wrong LOD name/triangle count: ' + name)
                path = source_path(src, expected_name, level)
                files[str(path.relative_to(src))] = sha(path)
                if not set(level.get('used_materials', level.get('materials', row.get('materials', [])))) <= materials:
                    raise ValueError('LOD refers to unknown source material: ' + expected_name)
        result[label] = {'src': src, 'contract': contract, 'rows': rows,
                         'files': files, 'contract_sha256': sha(src / 'model_contract.json'),
                         'materials_sha256': sha(src / 'materials.json')}
    return result


def bounds_cm(row):
    bounds = row['bounds_local_m']
    center = [(bounds[0][i] + bounds[1][i]) * 50 for i in range(3)]
    center[1] *= -1
    size = [(bounds[1][i] - bounds[0][i]) * 100 for i in range(3)]
    return center + size


def native_bounds(mesh):
    b = mesh.get_bounds()
    return [b.origin.x, b.origin.y, b.origin.z,
            2 * b.box_extent.x, 2 * b.box_extent.y, 2 * b.box_extent.z]


def used_materials(row, level=None):
    level = level or row
    result = level.get('used_materials', level.get('materials', row.get('materials', row.get('material_slots'))))
    if not result or not isinstance(result, list):
        raise ValueError('Expected nonempty material names on source part/LOD')
    return result


def legacy_bridge(probe_report):
    previous = {key: os.environ.get(key) for key in ('S47_STREET_ONLY', 'S47_STREET_REPORT')}
    os.environ['S47_STREET_ONLY'] = ''
    os.environ['S47_STREET_REPORT'] = str(probe_report)
    try:
        bridge = runpy.run_path(str(ROOT.parent / 'calichestreet/import_assets.py'))
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
    proof = json.loads(probe_report.read_text())
    if proof.get('status') != 'PASS' or proof.get('meshes'):
        raise RuntimeError('Legacy bridge must import only the asymmetric coordinate probe')
    return bridge


def import_all():
    selected = labels()
    output = Path(os.environ.get('S47_NATURE_REPORT', str(ROOT / 'unreal-import.json')))
    output.parent.mkdir(parents=True, exist_ok=True)
    probe_report = Path(os.environ.get('S47_NATURE_PROBE_REPORT', str(output.with_name(output.stem + '-axis-probe.json'))))
    probe_report.parent.mkdir(parents=True, exist_ok=True)
    report = {'status': 'INCOMPLETE', 'engine': unreal.SystemLibrary.get_engine_version(),
              'sets': selected, 'checks': [], 'meshes': [], 'materials': [],
              'coordinate_policy': 'Blender (x,y,z) -> UE cm (x,-y,z)*100',
              'probe_report': str(probe_report), 'sources': {},
              'limits': 'LOD screens are native asset settings. Cull metadata is a policy for runtime HISM, not proof of runtime distance culling.'}

    def write():
        output.write_text(json.dumps(report, indent=2))

    def check(name, value, detail=None):
        report['checks'].append({'name': name, 'pass': bool(value), 'detail': detail})
        write()
        if not value:
            raise AssertionError((name, detail))

    try:
        # Check every source before deleting any previously generated native mesh.
        sources = preflight(selected)
        report['sources'] = {label: {key: value for key, value in data.items() if key in ('files', 'contract_sha256', 'materials_sha256')} for label, data in sources.items()}
        write()
        bridge = legacy_bridge(probe_report)
        # The reused functions resolve globals in their original runpy namespace.
        # Redirect their checks to our report after keeping the isolated probe proof.
        bridge['material'].__globals__['check'] = check
        bridge['import_mesh'].__globals__['check'] = check
        E, M, S = bridge['E'], bridge['M'], bridge['S']
        for label, data in sources.items():
            src, contract, dest = data['src'], data['contract'], SETS[label]
            mats = {}
            for row in data['rows']:
                mat = bridge['material'](row, dest + '/Materials', src)
                if label == 'vegetation':
                    # These assets are drawn by HISM at runtime. Static-mesh
                    # preview shaders alone cook a gray fallback for that path.
                    # Same explicit usage policy as webexport/import_web.py.
                    mat.set_editor_property('used_with_instanced_static_meshes', True)
                    check(row['name'] + ' HISM material usage enabled', mat.get_editor_property('used_with_instanced_static_meshes'))
                    errors = list(M.recompile_material(mat))
                    check(row['name'] + ' HISM shader material compiles', not errors, errors)
                if row.get('vertex_rgb_tint'):
                    base = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
                    output_name = M.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_BASE_COLOR)
                    check(row['name'] + ' existing textured base color', base is not None)
                    vertex = bridge['expression'](mat, unreal.MaterialExpressionVertexColor)
                    multiply = bridge['expression'](mat, unreal.MaterialExpressionMultiply)
                    multiply.set_editor_property('desc', 'CN_BaseColor_times_VertexRGB')
                    check(row['name'] + ' existing base to tint A', M.connect_material_expressions(base, output_name, multiply, 'A'))
                    # VertexColor's native RGB output is unnamed (output0,
                    # mask RGB=1, A=0); UE5.8 does not accept the label "RGB".
                    check(row['name'] + ' vertex RGB to tint B', M.connect_material_expressions(vertex, '', multiply, 'B'))
                    check(row['name'] + ' vertex tint output', M.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR))
                    errors = list(M.recompile_material(mat))
                    check(row['name'] + ' vertex RGB material compiles', not errors, errors)
                E.set_metadata_tag(mat, 'CN_SOURCE_MATERIALS_SHA256', data['materials_sha256'])
                for role, filename in row.get('textures', {}).items():
                    if role not in ('base_color', 'normal', 'roughness'):
                        continue
                    texture = E.load_asset(dest + '/Materials/Textures/T_' + row['name'] + '_' + role)
                    E.set_metadata_tag(texture, 'CN_SOURCE_TEXTURE_SHA256', sha(src / filename))
                    E.save_loaded_asset(texture)
                E.save_loaded_asset(mat)
                mats[row['name']] = mat
                report['materials'].append({'name': row['name'], 'asset': mat.get_path_name(), 'vertex_rgb_tint': bool(row.get('vertex_rgb_tint'))})
            for name, row in contract['parts'].items():
                lods = levels(name, row)
                mesh = bridge['import_mesh'](source_path(src, name, lods[0]), dest)
                for index, level in enumerate(lods[1:], 1):
                    path = source_path(src, name + '_LOD' + str(index), level)
                    check(name + ' authored LOD' + str(index), S.import_lod(mesh, index, str(path)) == index)
                check(name + ' exact authored LOD count', S.get_lod_count(mesh) == len(lods))
                if label == 'ground':
                    # Physical 6m tiling uses UVs up to400 over this field.
                    # Float16 would quantize those to1.5m steps. Reuse the
                    # established finish_mesh policy in freemodels/import_free.py.
                    for index in range(len(lods)):
                        build = S.get_lod_build_settings(mesh, index)
                        build.set_editor_property('use_full_precision_u_vs', True)
                        S.set_lod_build_settings(mesh, index, build)
                        check(name + ' full-precision physical-scale UVs', S.get_lod_build_settings(mesh, index).get_editor_property('use_full_precision_u_vs'))
                screen_values = screens(contract, row, len(lods))
                check(name + ' explicit LOD screen sizes applied', S.set_lod_screen_sizes(mesh, screen_values))
                actual_screens = list(S.get_lod_screen_sizes(mesh))
                check(name + ' native LOD screen values', len(actual_screens) == len(screen_values) and all(abs(a - b) < 1e-6 for a, b in zip(actual_screens, screen_values)), actual_screens)
                slots = list(mesh.get_editor_property('static_materials'))
                for index, slot in enumerate(slots):
                    material_name = str(slot.get_editor_property('material_slot_name'))
                    check(name + ' source slot ' + material_name, material_name in mats)
                    mesh.set_material(index, mats[material_name])
                for index, level in enumerate(lods):
                    triangles = mesh.get_num_triangles(index)
                    check(name + ' LOD' + str(index) + ' exact triangles', triangles == int(level['triangles']), {'expected': level['triangles'], 'actual': triangles})
                    section_materials = [mesh.get_material(S.get_lod_material_slot(mesh, index, section)).get_name() for section in range(mesh.get_num_sections(index))]
                    expected = used_materials(row, level)
                    check(name + ' LOD' + str(index) + ' real section bindings', set(section_materials) == set(expected), {'expected': expected, 'actual': section_materials})
                expected_bounds, actual_bounds = bounds_cm(row), native_bounds(mesh)
                check(name + ' native scale axes pivot bounds', max(abs(a - b) for a, b in zip(expected_bounds, actual_bounds)) < .15, {'expected': expected_bounds, 'actual': actual_bounds})
                if any(material.get('vertex_rgb_tint') or material.get('vertex_alpha_opacity') for material in data['rows'] if material['name'] in used_materials(row)):
                    check(name + ' nonwhite authored vertex colors', S.has_vertex_colors(mesh))
                nanite = mesh.get_editor_property('nanite_settings')
                nanite.enabled = False
                mesh.set_editor_property('nanite_settings', nanite)
                policy = cull_policy(contract, row)
                E.set_metadata_tag(mesh, 'CN_LOD_SCREEN_SIZES', json.dumps(screen_values))
                E.set_metadata_tag(mesh, 'CN_SOURCE_CONTRACT_SHA256', data['contract_sha256'])
                E.set_metadata_tag(mesh, 'CN_SOURCE_LOD_FBX_SHA256', json.dumps([sha(source_path(src, name if index == 0 else name + '_LOD' + str(index), level)) for index, level in enumerate(lods)]))
                if policy:
                    E.set_metadata_tag(mesh, 'CN_RUNTIME_CULL_METERS', json.dumps(policy))
                E.save_loaded_asset(mesh)
                report['meshes'].append({'name': name, 'asset': mesh.get_path_name(),
                    'triangles': [mesh.get_num_triangles(index) for index in range(len(lods))],
                    'screen_sizes': actual_screens, 'runtime_cull_m_policy': policy,
                    'bounds_cm': actual_bounds, 'slots': [str(slot.material_slot_name) for slot in slots]})
                write()
        report['status'] = 'PASS'
        write()
        unreal.log('CALICHE NATURE IMPORT PASS ' + str(len(report['meshes'])))
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        write()
        raise


if __name__ == '__main__':
    import_all()
