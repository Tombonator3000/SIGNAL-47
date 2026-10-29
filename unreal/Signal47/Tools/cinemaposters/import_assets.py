"""ART01: import/verify only two owned exterior Plaza prints and their plane copy.

Root runs this through unreal_guard. S47_CINEMA_MODE=import|verify and a fresh
S47_CINEMA_REPORT are mandatory. No TB2, Engine or shared material is saved.
The activation record lives in the existing cooked WebData data lane.
"""
import hashlib
import json
import os
import re
import traceback
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
UNREAL_DIR = HERE.parents[2]
CONTENT = Path(unreal.Paths.project_content_dir()).resolve()
SPEC = json.loads((HERE / 'surfaces.json').read_text())
SPEC_SHA = hashlib.sha256((HERE / 'surfaces.json').read_bytes()).hexdigest()
DEST = '/Game/Signal47/Art/CinemaPosters'
BINDING = CONTENT / 'WebData/cinema-posters-binding.json'
REPORT = Path(os.environ['S47_CINEMA_REPORT']).resolve()
MODE = os.environ.get('S47_CINEMA_MODE', 'verify')
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
TOL_CM = 0.025
report = {'status': 'RUNNING', 'mode': MODE, 'process_id': os.getpid(), 'engine': unreal.SystemLibrary.get_engine_version(),
          'spec_sha256': SPEC_SHA, 'checks': [], 'textures': [], 'materials': []}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')
    temporary.replace(path)


def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    atomic_json(REPORT, report)
    if not condition:
        raise AssertionError((name, detail))


def asset_path(name):
    return DEST + '/' + name + '.' + name


def native_file(asset):
    package = asset.split('.')[0]
    if package.startswith('/Game/'):
        return CONTENT / (package[6:] + '.uasset')
    if package.startswith('/Engine/'):
        return Path(unreal.Paths.engine_content_dir()).resolve() / (package[8:] + '.uasset')
    raise AssertionError(('unowned package root', asset))


def protected_inventory():
    paths = list((CONTENT / 'TownBuildings').rglob('*'))
    paths += [native_file(SPEC['master']), native_file(SPEC['plane']['source'])]
    engine = Path(unreal.Paths.engine_content_dir()).resolve()
    return {('Content/' + p.relative_to(CONTENT).as_posix() if p.is_relative_to(CONTENT)
             else 'Engine/' + p.relative_to(engine).as_posix()): sha(p)
            for p in sorted(set(paths)) if p.is_file()}


def ident(cls, index):
    result = cls()
    assert result.import_text('(IDValue=' + str(index) + ')')
    return result


def id_number(value):
    return int(re.search(r'IDValue\s*=\s*(-?\d+)', value.export_text())[1])


def saved_triangles(mesh, lod):
    desc = mesh.get_static_mesh_description(lod)
    check(mesh.get_name() + ' saved LOD' + str(lod), desc is not None)
    result = []
    index = 0
    while len(result) < desc.get_triangle_count() and index < desc.get_triangle_count() * 4 + 64:
        tid = ident(unreal.TriangleID, index)
        index += 1
        if not desc.is_triangle_valid(tid):
            continue
        vertices = list(desc.get_triangle_vertex_instances(tid))
        assert len(vertices) == 3
        positions, uvs = [], []
        for vi in vertices:
            p = desc.get_vertex_position(desc.get_vertex_instance_vertex(vi))
            uv = desc.get_vertex_instance_uv(vi, 0)
            positions.append([float(p.x), float(p.y), float(p.z)])
            uvs.append([float(uv.x), float(uv.y)])
        result.append({'positions_ue_cm': positions, 'uv0': uvs,
                       'polygon_group': id_number(desc.get_triangle_polygon_group(tid))})
    check(mesh.get_name() + ' complete saved triangle scan LOD' + str(lod), len(result) == desc.get_triangle_count())
    return result


def bounds(points):
    return [[fn(p[k] for p in points) for k in range(3)] for fn in (min, max)]


def local_three(point):
    return [point[0] / 100, point[2] / 100, point[1] / 100]


def near_plane(triangle, centre, width, height):
    return all(abs(p[0] / 100 - centre[0]) < TOL_CM / 100
               and abs(p[2] / 100 - centre[1]) <= height / 2 + TOL_CM / 100
               and abs(p[1] / 100 - centre[2]) <= width / 2 + TOL_CM / 100
               for p in triangle['positions_ue_cm'])


def source_preflight():
    check('owned two-surface contract', SPEC['version'] == 1 and SPEC['destination'] == DEST
          and SPEC['binding'] == 'WebData/cinema-posters-binding.json'
          and [r['id'] for r in SPEC['surfaces']] == ['desert-sky', 'friday-matinee'])
    catalog = json.loads((UNREAL_DIR / 'ArtSource/EnvironmentGraphics/catalog.json').read_text())
    for row in SPEC['surfaces']:
        source = UNREAL_DIR / row['source']
        cat = next(r for r in catalog['assets'] if r['id'] == row['catalog_id'])
        check(row['id'] + ' original JPG identity', source.is_file() and sha(source) == row['source_sha256']
              and cat['image']['sha256'] == row['source_sha256']
              and [cat['image']['width'], cat['image']['height']] == row['source_dimensions_px'])
        check(row['id'] + ' uncropped 2:3 fit', abs(row['art_size_m'][0] / row['art_size_m'][1] - 2 / 3) < 1e-12
              and all(a <= f for a, f in zip(row['art_size_m'], row['frame_size_m']))
              and row['local_centre_three_m'][0] + row['lift_m'] < row['glass_x_local_m'])
    for kind in ('shell', 'glass'):
        part = SPEC['tb2'][kind]
        source = UNREAL_DIR / part['source_glb']
        check(kind + ' original GLB identity', source.is_file() and sha(source) == part['source_glb_sha256'])


def originals():
    """Fresh saved-native measurement; never modifies/CPU-prepares the originals."""
    result = {}
    for kind in ('shell', 'glass'):
        spec = SPEC['tb2'][kind]
        mesh = E.load_asset(spec['asset'])
        check(kind + ' exact native predecessor', isinstance(mesh, unreal.StaticMesh)
              and mesh.get_path_name() == spec['asset'] and S.get_lod_count(mesh) == 3)
        slots = list(mesh.get_editor_property('static_materials'))
        names = [str(v.get_editor_property('material_slot_name')) for v in slots]
        # Name and bound material identity are authoritative; index12 is not assumed.
        check(kind + ' unique named original print/glass slot', names.count(spec['material']) == 1, names)
        slot = names.index(spec['material'])
        material = slots[slot].get_editor_property('material_interface')
        check(kind + ' actual original material binding', material is not None
              and material.get_path_name() == '/Game/TownBuildings/Materials/' + spec['material'] + '.' + spec['material'])
        measured = {'asset': mesh.get_path_name(), 'native_sha256': sha(native_file(spec['asset'])),
                    'material_slot': slot, 'material_slot_name': names[slot], 'material_asset': material.get_path_name(),
                    'all_material_slot_names': names, 'lod_triangles': [], 'surfaces': {}}
        for lod in range(3):
            tris = saved_triangles(mesh, lod)
            check(kind + ' actual saved/render triangle counts LOD' + str(lod),
                  len(tris) == mesh.get_num_triangles(lod) == spec['lod_triangles'][lod])
            measured['lod_triangles'].append(len(tris))
            for row in SPEC['surfaces']:
                centre = list(row['local_centre_three_m'])
                size = row['frame_size_m'] if kind == 'shell' else row['glass_size_m']
                if kind == 'glass':
                    centre[0] = row['glass_x_local_m']
                selected = [t for t in tris if near_plane(t, centre, *size)]
                check(kind + '/' + row['id'] + ' exactly two saved triangles LOD' + str(lod), len(selected) == 2)
                groups = {t['polygon_group'] for t in selected}
                # These authored FBXs retain all named slots. Validate the saved group AND
                # actual LOD section map back to the same named material, not an ordinal alone.
                check(kind + '/' + row['id'] + ' named saved section LOD' + str(lod), groups == {slot}
                      and S.get_lod_material_slot(mesh, lod, slot) == slot)
                points = [local_three(p) for t in selected for p in t['positions_ue_cm']]
                lo, hi = bounds(points)
                actual_centre = [(a+b)/2 for a, b in zip(lo, hi)]
                actual_size = [hi[2]-lo[2], hi[1]-lo[1]]
                check(kind + '/' + row['id'] + ' actual frame position/size LOD' + str(lod),
                      max(abs(a-b) for a, b in zip(actual_centre, centre)) < TOL_CM/100
                      and max(abs(a-b) for a, b in zip(actual_size, size)) < TOL_CM/100,
                      {'centre_local_three_m': actual_centre, 'size_m': actual_size})
                measured['surfaces'].setdefault(row['id'], []).append({
                    'lod': lod, 'triangles': 2, 'centre_local_three_m': actual_centre, 'size_m': actual_size,
                    'corners_local_three_m': points, 'uv0': [uv for t in selected for uv in t['uv0']],
                    'triangle_payload_sha256': hashlib.sha256(json.dumps(selected, sort_keys=True).encode()).hexdigest()})
            if lod == 0:
                measured['bounds_local_ue_cm'] = bounds([p for t in tris for p in t['positions_ue_cm']])
        result[kind] = measured
    return result


def plane_payload(mesh):
    check(mesh.get_name() + ' one original plane LOD/slot', S.get_lod_count(mesh) == 1
          and mesh.get_num_triangles(0) == 2 and len(mesh.get_editor_property('static_materials')) == 1)
    triangles = saved_triangles(mesh, 0)
    corners = []
    for triangle in triangles:
        for p, uv in zip(triangle['positions_ue_cm'], triangle['uv0']):
            check('measured plane corner and UV0', abs(abs(p[0])-50) < .0001 and abs(abs(p[1])-50) < .0001
                  and abs(p[2]) < .0001 and abs(uv[0]-(p[0]+50)/100) < 1e-6 and abs(uv[1]-(p[1]+50)/100) < 1e-6)
            corners.append({'position_ue_cm': p, 'uv0': uv})
    check('all four actual plane corners', len({tuple(c['position_ue_cm']) for c in corners}) == 4)
    return triangles


def plane():
    source = E.load_asset(SPEC['plane']['source'])
    check('unchanged Engine Plane exists', isinstance(source, unreal.StaticMesh))
    expected = plane_payload(source)
    path = asset_path(SPEC['plane']['asset'])
    mesh = E.load_asset(path)
    if mesh is None and MODE == 'import':
        mesh = E.duplicate_asset(SPEC['plane']['source'], path.split('.')[0])
        check('owned plane duplicated', isinstance(mesh, unreal.StaticMesh))
        mesh.set_editor_property('allow_cpu_access', True)
        E.set_metadata_tag(mesh, 'S47_CINEMA_SPEC_SHA256', SPEC_SHA)
        check('saved owned CPU plane copy', E.save_loaded_asset(mesh, False))
    check('owned plane available', isinstance(mesh, unreal.StaticMesh))
    check('owned plane exact saved source geometry/UV', plane_payload(mesh) == expected)
    check('owned plane CPU readback enabled', bool(mesh.get_editor_property('allow_cpu_access')))
    check('owned plane exact provenance', E.get_metadata_tag(mesh, 'S47_CINEMA_SPEC_SHA256') == SPEC_SHA)
    return mesh


def texture(row):
    source = UNREAL_DIR / row['source']
    path = asset_path(row['texture'])
    tex = E.load_asset(path)
    if tex is None and MODE == 'import':
        task = unreal.AssetImportTask()
        task.filename, task.destination_path, task.destination_name = str(source), DEST, row['texture']
        task.automated, task.save, task.replace_existing = True, False, False
        T.import_asset_tasks([task])
        tex = E.load_asset(path)
        check(row['id'] + ' new Texture2D imported', isinstance(tex, unreal.Texture2D))
        for key, value in texture_settings().items():
            tex.set_editor_property(key, value)
        E.set_metadata_tag(tex, 'S47_CINEMA_SOURCE_SHA256', row['source_sha256'])
        E.set_metadata_tag(tex, 'S47_CINEMA_SPEC_SHA256', SPEC_SHA)
        check(row['id'] + ' texture saved', E.save_loaded_asset(tex, False))
    check(row['id'] + ' exact owned Texture2D', isinstance(tex, unreal.Texture2D) and tex.get_path_name() == path)
    check(row['id'] + ' actual saved texture policy', all(tex.get_editor_property(k) == v for k, v in texture_settings().items()))
    data = tex.get_editor_property('asset_import_data')
    imported = Path(data.get_first_filename()).resolve() if data else None
    check(row['id'] + ' saved source path/hash', imported == source.resolve() and imported.is_file()
          and sha(imported) == row['source_sha256'] and E.get_metadata_tag(tex, 'S47_CINEMA_SOURCE_SHA256') == row['source_sha256']
          and E.get_metadata_tag(tex, 'S47_CINEMA_SPEC_SHA256') == SPEC_SHA)
    check(row['id'] + ' actual texture size', 1024 <= tex.blueprint_get_size_x() <= 2048 and 1024 <= tex.blueprint_get_size_y() <= 2048)
    report['textures'].append({'asset': path, 'source': row['source'], 'source_sha256': sha(source),
                               'source_dimensions_px': row['source_dimensions_px'],
                               'actual_size_px': [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]})
    return tex


def texture_settings():
    return {'srgb': True, 'compression_settings': unreal.TextureCompressionSettings.TC_BC7,
            'power_of_two_mode': unreal.TexturePowerOfTwoSetting.STRETCH_TO_SQUARE_POWER_OF_TWO,
            'max_texture_size': 2048, 'mip_gen_settings': unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP,
            'lod_group': unreal.TextureGroup.TEXTUREGROUP_WORLD, 'filter': unreal.TextureFilter.TF_DEFAULT}


def material(row, tex):
    path = asset_path(row['material'])
    parent = E.load_asset(SPEC['master'])
    check('existing shared opaque document master', isinstance(parent, unreal.Material))
    mi = E.load_asset(path)
    if mi is None and MODE == 'import':
        mi = T.create_asset(row['material'], DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        check(row['id'] + ' new owned material instance', isinstance(mi, unreal.MaterialInstanceConstant))
        M.set_material_instance_parent(mi, parent)
        for key, value in SPEC['material_parameters']['scalars'].items():
            M.set_material_instance_scalar_parameter_value(mi, key, value)
        for key, value in SPEC['material_parameters']['vectors'].items():
            M.set_material_instance_vector_parameter_value(mi, key, unreal.LinearColor(*value))
        M.set_material_instance_texture_parameter_value(mi, 'Map', tex)
        E.set_metadata_tag(mi, 'S47_CINEMA_SPEC_SHA256', SPEC_SHA)
        check(row['id'] + ' material saved', E.save_loaded_asset(mi, False))
    check(row['id'] + ' actual material path/parent', isinstance(mi, unreal.MaterialInstanceConstant)
          and mi.get_path_name() == path and mi.get_editor_property('parent') == parent)
    for key, value in SPEC['material_parameters']['scalars'].items():
        check(row['id'] + ' actual scalar ' + key, abs(M.get_material_instance_scalar_parameter_value(mi, key) - value) < 1e-6)
    for key, value in SPEC['material_parameters']['vectors'].items():
        actual = M.get_material_instance_vector_parameter_value(mi, key)
        check(row['id'] + ' actual vector ' + key, max(abs(a-b) for a, b in zip((actual.r, actual.g, actual.b, actual.a), value)) < 1e-6)
    check(row['id'] + ' actual Map texture', M.get_material_instance_texture_parameter_value(mi, 'Map') == tex)
    expected = {'scalar_parameter_values': set(SPEC['material_parameters']['scalars']),
                'vector_parameter_values': set(SPEC['material_parameters']['vectors']), 'texture_parameter_values': {'Map'}}
    for prop, names in expected.items():
        values = list(mi.get_editor_property(prop))
        actual = {str(v.get_editor_property('parameter_info').get_editor_property('name')) for v in values}
        check(row['id'] + ' exact saved overrides ' + prop, len(values) == len(names) and actual == names)
    check(row['id'] + ' material provenance', E.get_metadata_tag(mi, 'S47_CINEMA_SPEC_SHA256') == SPEC_SHA)
    report['materials'].append({'asset': path, 'parent': parent.get_path_name(), 'Map': tex.get_path_name(),
                                 'parameters': SPEC['material_parameters']})


def main():
    check('explicit supported mode and fresh report', MODE in ('import', 'verify') and not REPORT.is_relative_to(CONTENT))
    source_preflight()
    before = protected_inventory()
    check('protected native inventory is present', len(before) >= 30 and all(native_file(SPEC['tb2'][k]['asset']).is_file() for k in ('shell', 'glass')))
    previous = json.loads(BINDING.read_text()) if BINDING.is_file() else None
    if previous is not None and previous.get('status') == 'PASS':
        intact = (previous.get('spec_sha256') == SPEC_SHA and len(previous.get('native_assets', {})) == 5
                  and all(native_file(p).is_file() and sha(native_file(p)) == v['sha256']
                          for p, v in previous['native_assets'].items()))
        if not intact and MODE == 'import':
            atomic_json(BINDING, {'status': 'REJECTED', 'reason': 'previous native payload changed; root review required', 'spec_sha256': SPEC_SHA})
        check('prior accepted payload remains byte-identical before any work', intact)
    # Incomplete native work cannot leave a previous PASS activation in place.
    if MODE == 'import' and (previous is None or previous.get('status') != 'PASS'):
        atomic_json(BINDING, {'status': 'RUNNING', 'spec_sha256': SPEC_SHA})
    measured = originals()
    report['saved_originals'] = measured
    mesh = plane()
    for row in SPEC['surfaces']:
        material(row, texture(row))
    paths = [asset_path(SPEC['plane']['asset'])] + [asset_path(r[k]) for r in SPEC['surfaces'] for k in ('texture', 'material')]
    native = {p: {'bytes': native_file(p).stat().st_size, 'sha256': sha(native_file(p))} for p in paths}
    check('all original TB2/shared/Engine bytes unchanged', protected_inventory() == before)
    for row in SPEC['surfaces']:
        check(row['id'] + ' source JPG remains unchanged', sha(UNREAL_DIR / row['source']) == row['source_sha256'])
    binding = {'version': 1, 'status': 'PASS', 'scope': SPEC['scope'], 'spec_sha256': SPEC_SHA,
               'master': SPEC['master'], 'plane_asset': mesh.get_path_name(), 'plane_saved_triangles': plane_payload(mesh),
               'original_measurement': 'fresh saved StaticMeshDescription, all three authored LODs; not live CPU triangle proof',
               'originals': measured, 'native_assets': native, 'surfaces': SPEC['surfaces'],
               'protected_native_inventory_sha256': hashlib.sha256(json.dumps(before, sort_keys=True).encode()).hexdigest(),
               'site_world_three_m': SPEC['tb2']['site_world_three_m'], 'material_parameters': SPEC['material_parameters']}
    if previous is not None and previous.get('status') == 'PASS':
        # Resume/verify never rewrites an already accepted native set. A changed file,
        # schema or saved geometry is rejected for root to inspect in a fresh output.
        expected = dict(binding, fresh_saved_reader=True, import_report_sha256=previous.get('import_report_sha256'))
        check('complete saved native/readback identity', previous == expected
              and len(previous.get('import_report_sha256', '')) == 64)
    elif MODE == 'verify':
        import_report = Path(os.environ['S47_CINEMA_IMPORT_REPORT']).resolve()
        imported = json.loads(import_report.read_text())
        check('separate successful import process', imported.get('status') == 'PASS' and imported.get('mode') == 'import'
              and imported.get('process_id') != os.getpid() and imported.get('spec_sha256') == SPEC_SHA)
        check('fresh saved readback matches full imported payload', imported.get('binding_candidate') == binding)
        atomic_json(BINDING, dict(binding, fresh_saved_reader=True, import_report_sha256=sha(import_report)))
    else:
        check('import only prepares a complete inactive candidate', MODE == 'import')
    report['binding_candidate'] = binding
    report['native_assets'] = native
    report['protected_native_before_sha256'] = before
    report['activation_status'] = json.loads(BINDING.read_text()).get('status') if BINDING.is_file() else 'ABSENT'
    report['binding_sha256'] = sha(BINDING) if BINDING.is_file() else None
    report['status'] = 'PASS'
    atomic_json(REPORT, report)
    unreal.log('S47 CINEMA POSTERS ' + MODE.upper() + ' PASS')


if REPORT.exists() or REPORT.is_relative_to(CONTENT):
    raise RuntimeError('S47_CINEMA_REPORT must be fresh and outside Content; prior evidence is preserved: ' + str(REPORT))
try:
    main()
except Exception:
    report['status'], report['error'] = 'FAIL', traceback.format_exc()
    atomic_json(REPORT, report)
    unreal.log_error('S47 CINEMA POSTERS FAIL; activation requires a complete matching PASS record')
    raise
