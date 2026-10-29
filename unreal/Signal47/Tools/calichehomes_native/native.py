"""Avgrenset UE5.8-import og lagret verifikasjon av Caliche Homes.

Gjenbruker prosjektets FBX-importør og faktiske FBX-leser. Bare egne mesh og
materialinstanser skrives. Town-grafer, teksturer og PBR-kilder beholdes.
Alle funksjoner i verify_all er lesende, bortsett fra JSON-bevisrapporten.
"""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import runpy
import struct

import unreal

PROJECT = Path(__file__).resolve().parents[2]
TOOLS = PROJECT / 'Tools'
DEST = '/Game/CalicheHomes'
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


FBX = module('s47_homes_fbx_helpers', TOOLS / 'motelv2/native.py')
FBX.DEST = DEST
RULES = module('s47_homes_integration_rules', TOOLS / 'calichehomes_integration/schema.py')
FBX_NODES = runpy.run_path(str(TOOLS / 'calichenature/verify_unreal.py'))['fbx_nodes']
SCREENS = [2., .75, .5625]
TOL_CM = .025
VALUE_TOL = 1e-6


def sha(path):
    return RULES.sha(path)


def read(path):
    return json.loads(Path(path).read_text())


class Report(FBX.Report):
    def __init__(self, path, kind):
        super().__init__(path, kind)
        self.data.update(source_sha256={}, source_files_sha256={}, native_files_sha256={},
                         town_dependency_files_sha256={}, parent_materials={}, materials=[],
                         scope='Saved native source/render counts, actual FBX geometry and inherited material graph. Runtime/packaged/visual UNVERIFIED.',
                         geometry_tolerance_cm=TOL_CM)
        self.flush()


def source_path(root, relative):
    path = (root / relative).resolve()
    assert path.is_relative_to(root.resolve()), ('FBX/GLB outside source output', relative)
    return path


def source_inputs(root, trims, report):
    manifest, materials = read(root / 'manifest.json'), read(root / 'materials.json')
    contract, trim = read(root / 'source-contract.json'), read(trims / 'trim-manifest.json')
    converted = read(trims / 'trim-fbx.json')
    RULES.validate_source_contract(contract)
    report.check('exact twelve authored parts and source revision', set(manifest['parts']) == RULES.part_names()
                 and manifest['source_revision'] == RULES.REVISION and manifest['no_new_admission'] is True)
    report.check('canonical manifest alias identical', (root / 'manifest.json').read_bytes() == (root / 'model_contract.json').read_bytes())
    report.check('authored source/material contracts bound', manifest['source_contract_sha256'] == trim['source_contract_sha256'] == sha(root / 'source-contract.json')
                 and manifest['materials_manifest_sha256'] == sha(root / 'materials.json') and manifest['native_import_policy'] == contract['native_policy'])
    report.check('known immutable trim source and exact 528+76 removal', trim['version'] == 1 and trim['source_revision'] == RULES.REVISION
                 and trim['manifest_sha256'] == RULES.MANIFEST_SHA256 and trim['source_geometry_sha256'] == {'geo_008.glb': RULES.GEO_SHA256}
                 and trim['source_verification']['status'] == 'PASS' and trim['removed_house_total'] == 528 and trim['removed_sign_total'] == 76)
    report.check('exact eight trim predecessors', set(trim['meshes']) == {'g' + str(g) for g in RULES.PATCHES})
    for geo, (node, material, original, before, removed, after) in RULES.PATCHES.items():
        row = trim['meshes']['g' + str(geo)]
        report.check('g' + str(geo) + ' immutable cut contract',
                     (row['node'], row['geo'], row['parent'], row['source_tris'], row['result_tris']) == (node, geo, RULES.PARENT, original, after)
                     and row['accepted_previous_tris'] == [before] and row['source_material_ids'] == [material]
                     and row['removed_houses'] + row['removed_signs'] == removed and row['hidden'] == (after == 0))
        name = 'g' + str(geo)
        report.check(name + ' exact trim namespace/slot mapping', row['asset'] == (DEST+'/Trim/'+name+'.'+name if after else '')
                     and row['original_material_indices'] == ([0] if after else [])
                     and row['materials'] == ([f'M_Homes_Trim_{name}_slot0'] if after else []))
    report.check('exact two preserved three-slot signs', set(trim['signs']) == {'SM_Homes_SignNorth', 'SM_Homes_SignSouth'})
    for label in RULES.SIGNS:
        row = trim['signs']['SM_Homes_Sign' + label]
        report.check(label + ' sign source slots', row['result_tris'] == 38 and row['lod_triangles'] == [38]
                     and row['materials'] == RULES.sign_materials(label) and row['source_nodes'] == [1448, 1449, 1450]
                     and row['source_material_ids'] == [638, 641, 642])
        name = 'SM_Homes_Sign' + label
        report.check(label + ' exact owned sign namespace', row['asset'] == DEST+'/'+name+'.'+name
                     and row['original_material_indices'] == [0, 0, 0])
    expected = {name: row for name, row in trim['meshes'].items() if not row['hidden']}
    expected.update(trim['signs'])
    converted_rows = {row['name']: row for row in converted['meshes']}
    report.check('exact seven converted trims/signs bound to manifest', converted['status'] == 'PASS'
                 and converted['trim_manifest_sha256'] == sha(trims / 'trim-manifest.json')
                 and len(converted['meshes']) == len(converted_rows) == len(expected) == 7 and set(converted_rows) == set(expected))
    files = {root / name for name in ('manifest.json', 'model_contract.json', 'materials.json', 'source-contract.json')}
    files |= {trims / name for name in ('trim-manifest.json', 'trim-fbx.json')}
    for name, row in manifest['parts'].items():
        house, chunk = name.split('_')[2:]
        report.check(name + ' anchored authored three-LOD contract', row['house'] == house and row['chunk'] == chunk
                     and row['site_world_three_m'] == RULES.HOUSES[house]['root'] and row['offset'] == [0, 0, 0]
                     and row['parent_handle'] == 'world.town.group' and len(row['lods']) == 3
                     and row['lod_triangles'] == [lod['triangles'] for lod in row['lods']])
        for lod, data in enumerate(row['lods']):
            filename = 'exports/' + name + ('' if lod == 0 else '_LOD' + str(lod)) + '.fbx'
            path = source_path(root, data['fbx'])
            report.check(name + ' source FBX LOD' + str(lod), data['fbx'] == filename and path.is_file()
                         and data['triangles'] > 0 and sha(path) == manifest['generated_sha256'][filename])
            files.add(path)
            validate_fbx(path, data, data['materials'], report, name + ' LOD' + str(lod))
    for name, row in expected.items():
        converted_row = converted_rows[name]
        path = source_path(trims, row['fbx'])
        report.check(name + ' converted FBX exact', path.is_file() and converted_row['fbx'] == row['fbx']
                     and sha(path) == converted_row['fbx_sha256'] and converted_row['triangles'] == row['result_tris']
                     and converted_row['materials'] == converted_row['fbx_material_names'] == row['materials'])
        files.add(path)
        validate_fbx(path, {'triangles': row['result_tris'], 'bounds_local_m': row['bounds_local_m']}, row['materials'], report, name)
    for filename, digest in trim['file_sha256'].items():
        path = source_path(trims, filename)
        report.check('serialized trim source ' + filename, path.is_file() and sha(path) == digest)
        files.add(path)
    names = {row['name'] for row in materials}
    report.check('exact unique used material rows', len(names) == len(materials) == 15
                 and names == {name for part in manifest['parts'].values() for name in part['materials']}
                 and all(name.startswith('M_Homes_') for name in names))
    for row in materials:
        report.check(row['name'] + ' inherited opaque nonemissive policy', row['source_material'].startswith('M_Town_')
                     and float(row.get('alpha', 1)) == 1 and float(row.get('emission_strength', 0)) == 0
                     and not row.get('two_sided', False) and set(row.get('textures', {})) <= {'base_color', 'roughness', 'normal'})
        for filename in row.get('textures', {}).values():
            path = (root / filename).resolve()
            report.check(row['name'] + ' existing source texture', path.is_file())
            files.add(path)
    report.data['source_sha256'] = {'manifest': sha(root / 'manifest.json'), 'materials': sha(root / 'materials.json'),
                                  'source_contract': sha(root / 'source-contract.json'), 'trim_manifest': sha(trims / 'trim-manifest.json')}
    for path in sorted(files):
        report.check('source file ' + path.name, path.is_file())
        report.data['source_files_sha256'][str(path.resolve())] = sha(path)
    return manifest, materials, trim, converted


def asset_file(asset):
    package = asset.split('.')[0]
    assert package.startswith('/Game/') and '..' not in package
    return PROJECT / 'Content' / (package.removeprefix('/Game/') + '.uasset')


def record_native(asset, report):
    assert asset.startswith(DEST + '/'), ('unowned native fingerprint', asset)
    path = asset_file(asset)
    report.check('saved native file ' + path.name, path.is_file())
    report.data['native_files_sha256'][str(path.relative_to(PROJECT))] = sha(path)


def town_snapshot():
    root = PROJECT / 'Content/TownBuildings'
    assert root.is_dir()
    return {str(path.relative_to(PROJECT)): sha(path) for path in sorted(root.rglob('*'))
            if path.is_file() and path.suffix in ('.uasset', '.uexp', '.ubulk', '.json')}


def protect_dependencies(report):
    snapshot = town_snapshot()
    report.check('existing Town native dependency snapshot', any(p.endswith('.uasset') for p in snapshot))
    report.data['town_dependency_files_sha256'] = snapshot
    report.flush()
    return snapshot


def check_preserved(snapshot, report):
    current = town_snapshot()
    report.check('all existing Town dependency files and bytes unchanged', current == snapshot,
                 {'added': sorted(set(current)-set(snapshot)), 'removed': sorted(set(snapshot)-set(current)),
                  'changed': sorted(p for p in set(current)&set(snapshot) if current[p] != snapshot[p])})
    for filename, digest in report.data['source_files_sha256'].items():
        report.check('source still unchanged ' + Path(filename).name, Path(filename).is_file() and sha(filename) == digest)


def instance_path(row):
    return DEST + '/Materials/' + row['name'].replace('M_Homes_', 'MI_Homes_', 1)


def object_path(row):
    path = instance_path(row)
    return path + '.' + path.rsplit('/', 1)[1]


def parameter_values(row):
    textures = row.get('textures', {})
    # Town kobler kartet direkte til Roughness. Bare den aktive grafgrenen
    # overstyres; frakoblede Color/Roughness-noder er ikke et runtime-bevis.
    vectors = {'TextureTint': row['texture_tint_linear']} if 'base_color' in textures else {'Color': row['base_color_linear']}
    scalars = {key.title(): float(row.get(key, default)) for key, default in [('metallic', 0), ('specular', .5)]}
    if 'roughness' not in textures:
        scalars['Roughness'] = float(row.get('roughness', .5))
    if 'normal' in textures:
        scalars['NormalStrength'] = float(row.get('normal_strength', .2))
    return vectors, scalars


def input_nodes(parent, node):
    return [n for n in M.get_inputs_for_material_expression(parent, node) if n is not None]


def named_inputs(parent, node):
    names = M.get_material_expression_input_names(node)
    nodes = M.get_inputs_for_material_expression(parent, node)
    assert len(names) == len(nodes), 'material input API order mismatch'
    return dict(zip(names, nodes))


def parameter(node, cls, name):
    return isinstance(node, cls) and str(node.get_editor_property('parameter_name')) == name


def texture_sample(parent, node, row, role, root, report):
    report.check(row['name'] + ' inherited ' + role + ' sample', isinstance(node, unreal.MaterialExpressionTextureSample))
    texture = node.get_editor_property('texture')
    report.check(row['name'] + ' existing Town ' + role + ' texture', isinstance(texture, unreal.Texture2D)
                 and texture.get_path_name().startswith('/Game/TownBuildings/'))
    data = texture.get_editor_property('asset_import_data')
    imported_from = Path(data.get_first_filename()).resolve() if data else None
    expected = (root / row['textures'][role]).resolve()
    report.check(row['name'] + ' inherited ' + role + ' pixels match source', imported_from is not None
                 and imported_from.is_file() and sha(imported_from) == sha(expected), str(imported_from))
    report.check(row['name'] + ' inherited ' + role + ' color space', bool(texture.get_editor_property('srgb')) == (role == 'base_color'))
    scale = float(row.get('uv_scale', 1))
    inputs = input_nodes(parent, node)
    if scale == 1:
        report.check(row['name'] + ' inherited default UV0 ' + role, not inputs and node.get_editor_property('const_coordinate') == 0)
    else:
        report.check(row['name'] + ' inherited explicit UV scale ' + role, len(inputs) == 1 and isinstance(inputs[0], unreal.MaterialExpressionTextureCoordinate))
        uv = inputs[0]
        report.check(row['name'] + ' inherited UV scale exact ' + role, uv.get_editor_property('coordinate_index') == 0
                     and abs(uv.get_editor_property('u_tiling')-scale) < VALUE_TOL and abs(uv.get_editor_property('v_tiling')-scale) < VALUE_TOL)
    if role == 'normal':
        report.check(row['name'] + ' inherited DirectX tangent normal', row.get('normal_convention', 'DirectX').upper() == 'DIRECTX'
                     and texture.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP
                     and not texture.get_editor_property('flip_green_channel')
                     and node.get_editor_property('sampler_type') == unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    elif role == 'roughness':
        report.check(row['name'] + ' inherited linear roughness sample', node.get_editor_property('sampler_type') == unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    else:
        report.check(row['name'] + ' inherited sRGB color sample', node.get_editor_property('sampler_type') == unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    return texture.get_path_name()


def parent_material(row, root, report):
    path = '/Game/TownBuildings/Materials/' + row['source_material']
    parent = E.load_asset(path)
    report.check(row['name'] + ' existing opaque Town parent', isinstance(parent, unreal.Material)
                 and parent.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE and not parent.get_editor_property('two_sided'), path)
    vectors, scalars = parameter_values(row)
    report.check(row['name'] + ' active parent vector parameters', set(vectors) <= {str(x) for x in M.get_vector_parameter_names(parent)})
    report.check(row['name'] + ' active parent scalar parameters', set(scalars) <= {str(x) for x in M.get_scalar_parameter_names(parent)})
    textures, inherited = row.get('textures', {}), {}
    color = M.get_material_property_input_node(parent, unreal.MaterialProperty.MP_BASE_COLOR)
    if 'base_color' in textures:
        report.check(row['name'] + ' active texture tint multiply', isinstance(color, unreal.MaterialExpressionMultiply))
        nodes = input_nodes(parent, color)
        samples = [n for n in nodes if isinstance(n, unreal.MaterialExpressionTextureSample)]
        tints = [n for n in nodes if parameter(n, unreal.MaterialExpressionVectorParameter, 'TextureTint')]
        report.check(row['name'] + ' active tint and sample', len(nodes) == 2 and len(samples) == len(tints) == 1)
        inherited['base_color'] = texture_sample(parent, samples[0], row, 'base_color', root, report)
    else:
        report.check(row['name'] + ' active flat color', parameter(color, unreal.MaterialExpressionVectorParameter, 'Color'))
    for key, prop in [('Metallic', unreal.MaterialProperty.MP_METALLIC), ('Specular', unreal.MaterialProperty.MP_SPECULAR)]:
        report.check(row['name'] + ' active ' + key, parameter(M.get_material_property_input_node(parent, prop), unreal.MaterialExpressionScalarParameter, key))
    roughness = M.get_material_property_input_node(parent, unreal.MaterialProperty.MP_ROUGHNESS)
    if 'roughness' in textures:
        inherited['roughness'] = texture_sample(parent, roughness, row, 'roughness', root, report)
        report.check(row['name'] + ' inherited roughness R output', M.get_material_property_input_node_output_name(parent, unreal.MaterialProperty.MP_ROUGHNESS) == 'R')
    else:
        report.check(row['name'] + ' active scalar roughness', parameter(roughness, unreal.MaterialExpressionScalarParameter, 'Roughness'))
    if 'normal' in textures:
        normal = M.get_material_property_input_node(parent, unreal.MaterialProperty.MP_NORMAL)
        report.check(row['name'] + ' inherited normalized normal', isinstance(normal, unreal.MaterialExpressionNormalize))
        nodes = input_nodes(parent, normal)
        report.check(row['name'] + ' inherited normal blend', len(nodes) == 1 and isinstance(nodes[0], unreal.MaterialExpressionLinearInterpolate))
        blend = input_nodes(parent, nodes[0])
        samples = [n for n in blend if isinstance(n, unreal.MaterialExpressionTextureSample)]
        controls = [n for n in blend if parameter(n, unreal.MaterialExpressionScalarParameter, 'NormalStrength')]
        neutral = [n for n in blend if isinstance(n, unreal.MaterialExpressionConstant3Vector)]
        report.check(row['name'] + ' inherited neutral/texture/strength normal inputs', len(blend) == 3 and len(samples) == len(controls) == len(neutral) == 1)
        pins = named_inputs(parent, nodes[0])
        report.check(row['name'] + ' inherited normal A/B/Alpha wiring', pins.get('A') == neutral[0]
                     and pins.get('B') == samples[0] and pins.get('Alpha') == controls[0])
        value = neutral[0].get_editor_property('constant')
        report.check(row['name'] + ' inherited neutral tangent normal', max(abs(value.r), abs(value.g), abs(value.b-1)) < VALUE_TOL)
        inherited['normal'] = texture_sample(parent, samples[0], row, 'normal', root, report)
    else:
        report.check(row['name'] + ' inherited flat normal', M.get_material_property_input_node(parent, unreal.MaterialProperty.MP_NORMAL) is None)
    report.check(row['name'] + ' inherited emission parameter', parameter(
        M.get_material_property_input_node(parent, unreal.MaterialProperty.MP_EMISSIVE_COLOR), unreal.MaterialExpressionVectorParameter, 'EmissiveFactor'))
    emissive = M.get_material_default_vector_parameter_value(parent, 'EmissiveFactor')
    report.check(row['name'] + ' inherited emission off', max(abs(emissive.r), abs(emissive.g), abs(emissive.b)) < VALUE_TOL)
    report.data['parent_materials'][row['name']] = {'parent': parent.get_path_name(), 'textures': inherited,
        'uv_scale': float(row.get('uv_scale', 1)), 'roughness_policy': 'inherited texture R' if 'roughness' in textures else 'MI scalar Roughness',
        'normal_policy': 'inherited DirectX sample, neutral-normal blend, MI NormalStrength, normalize' if 'normal' in textures else 'inherited flat normal'}
    return parent


def vector_error(actual, expected):
    return max(abs(a-b) for a, b in zip((actual.r, actual.g, actual.b, actual.a), expected))


def make_instance(row, root, report):
    parent = parent_material(row, root, report)
    path = instance_path(row)
    material = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(path.rsplit('/', 1)[1], DEST + '/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    report.check(row['name'] + ' owned native instance', isinstance(material, unreal.MaterialInstanceConstant) and material.get_path_name() == object_path(row))
    M.set_material_instance_parent(material, parent)
    M.clear_all_material_instance_parameters(material)
    vectors, scalars = parameter_values(row)
    # UE5.8.4s settere returnerer alltid false i den lokale C++-kilden, selv
    # etter Set*EditorOnly/UpdateMaterialInstance. Bruk avlest faktisk verdi.
    for key, value in vectors.items():
        M.set_material_instance_vector_parameter_value(material, key, unreal.LinearColor(*value))
        report.check(row['name'] + ' actual set ' + key, vector_error(M.get_material_instance_vector_parameter_value(material, key), value) < VALUE_TOL)
    for key, value in scalars.items():
        M.set_material_instance_scalar_parameter_value(material, key, value)
        report.check(row['name'] + ' actual set ' + key, abs(M.get_material_instance_scalar_parameter_value(material, key)-value) < VALUE_TOL)
    report.check(row['name'] + ' saved instance', E.save_loaded_asset(material))
    return material


def verify_instance(row, root, report):
    material = E.load_asset(instance_path(row))
    parent = parent_material(row, root, report)
    report.check(row['name'] + ' saved exact parent binding', isinstance(material, unreal.MaterialInstanceConstant)
                 and material.get_path_name() == object_path(row) and material.get_editor_property('parent') == parent)
    vectors, scalars = parameter_values(row)
    for key, value in vectors.items():
        report.check(row['name'] + ' saved ' + key, vector_error(M.get_material_instance_vector_parameter_value(material, key), value) < VALUE_TOL)
    for key, value in scalars.items():
        report.check(row['name'] + ' saved ' + key, abs(M.get_material_instance_scalar_parameter_value(material, key)-value) < VALUE_TOL)
    for prop, expected in [('vector_parameter_values', set(vectors)), ('scalar_parameter_values', set(scalars)), ('texture_parameter_values', set())]:
        values = list(material.get_editor_property(prop))
        names = {str(v.get_editor_property('parameter_info').get_editor_property('name')) for v in values}
        report.check(row['name'] + ' exact saved overrides ' + prop, len(values) == len(names) and names == expected, sorted(names))
    report.data['materials'].append({'name': row['name'], 'asset': material.get_path_name(), 'parent': parent.get_path_name(),
                                     'vectors': vectors, 'scalars': scalars, 'textures': 'inherited, no MI texture overrides'})
    record_native(material.get_path_name(), report)


def finish(mesh, report, lod_count):
    report.check(mesh.get_name() + ' authored LOD count before build', S.get_lod_count(mesh) == lod_count)
    FBX.finish_mesh(mesh, report)
    mesh.set_editor_property('support_ray_tracing', True)
    proxy = mesh.get_editor_property('ray_tracing_proxy_settings')
    proxy.set_editor_property('enabled', False)
    mesh.set_editor_property('ray_tracing_proxy_settings', proxy)
    body = mesh.get_editor_property('body_setup')
    report.check(mesh.get_name() + ' BodySetup', body is not None)
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for lod in range(lod_count):
        build = S.get_lod_build_settings(mesh, lod)
        build.set_editor_property('recompute_normals', False)
        build.set_editor_property('recompute_tangents', True)
        build.set_editor_property('use_mikk_t_space', True)
        S.set_lod_build_settings(mesh, lod, build)
        count = mesh.get_num_sections(lod)
        report.check(mesh.get_name() + ' nonempty sections LOD' + str(lod), count > 0)
        for section in range(count):
            if not S.is_section_collision_enabled(mesh, lod, section):
                S.enable_section_collision(mesh, True, lod, section)
    report.check(mesh.get_name() + ' explicit screen sizes', S.set_lod_screen_sizes(mesh, SCREENS if lod_count == 3 else [1.]))
    report.check(mesh.get_name() + ' saved final settings', E.save_loaded_asset(mesh))


def native_points(desc):
    return FBX.vertices(desc)


def fbx_geometry(path):
    objects = next(node for node in FBX_NODES(path) if node['name'] == 'Objects')
    geometries = [node for node in objects['children'] if node['name'] == 'Geometry' and node['properties'][-1] == 'Mesh']
    assert len(geometries) == 1, (path, 'one actual FBX geometry')
    return objects, geometries[0]


def expected_points(path):
    _, geometry = fbx_geometry(path)
    values = next(node for node in geometry['children'] if node['name'] == 'Vertices')['properties'][0]
    assert values and len(values) % 3 == 0 and all(math.isfinite(v) for v in values)
    return [(values[i]*100, -values[i+1]*100, values[i+2]*100) for i in range(0, len(values), 3)]


def bounds(points):
    return [[op(p[axis] for p in points) for axis in range(3)] for op in (min, max)]


def bounds_error(actual, expected):
    return max(abs(a-b) for p, q in zip(actual, expected) for a, b in zip(p, q))


def validate_fbx(path, contract, materials, report, label):
    objects, geometry = fbx_geometry(path)
    polygon = next(n for n in geometry['children'] if n['name'] == 'PolygonVertexIndex')['properties'][0]
    report.check(label + ' actual FBX triangles', len(polygon) == 3*contract['triangles']
                 and all((v < 0) == (i % 3 == 2) for i, v in enumerate(polygon)))
    names = [n['properties'][1].split('\x00')[0] for n in objects['children'] if n['name'] == 'Material']
    report.check(label + ' actual FBX material order', names == materials, {'actual': names, 'expected': materials})
    actual = bounds(expected_points(path))
    expected = FBX.expected_bounds(contract)
    report.check(label + ' actual FBX metric pivot/axes', bounds_error(actual, expected) <= TOL_CM,
                 {'actual_cm': actual, 'expected_cm': expected, 'tolerance_cm': TOL_CM})


def missing(points, expected):
    assert points and expected and all(math.isfinite(x) for p in points for x in p) and all(math.isfinite(x) for p in expected for x in p)
    cells = {}
    for p in expected:
        cells.setdefault(tuple(math.floor(x/TOL_CM) for x in p), []).append(p)
    bad, worst = [], 0.
    for p in points:
        cell = tuple(math.floor(x/TOL_CM) for x in p)
        distance = math.inf
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for q in cells.get((cell[0]+dx, cell[1]+dy, cell[2]+dz), []):
                        distance = min(distance, max(abs(a-b) for a, b in zip(p, q)))
        if distance > TOL_CM:
            if len(bad) < 8:
                bad.append(p)
        else:
            worst = max(worst, distance)
    return bad, worst


def verify_mesh(name, asset, lods, sources, slots, material_assets, report):
    mesh = E.load_asset(asset)
    report.check(name + ' saved native mesh', isinstance(mesh, unreal.StaticMesh) and mesh.get_path_name() == asset)
    report.check(name + ' exact LOD/source count', S.get_lod_count(mesh) == len(lods) == len(sources))
    static_materials = list(mesh.get_editor_property('static_materials'))
    actual_slots = [str(slot.get_editor_property('material_slot_name')) for slot in static_materials]
    actual_assets = [slot.get_editor_property('material_interface').get_path_name() if slot.get_editor_property('material_interface') else None for slot in static_materials]
    report.check(name + ' exact FBX slot order', actual_slots == slots, actual_slots)
    report.check(name + ' exact saved material bindings', actual_assets == material_assets, actual_assets)
    report.check(name + ' non-Nanite and RT supported', not mesh.get_editor_property('nanite_settings').enabled and mesh.get_editor_property('support_ray_tracing'))
    report.check(name + ' no dedicated proxy', not mesh.get_editor_property('ray_tracing_proxy_settings').enabled)
    report.check(name + ' complex visibility queries', S.get_collision_complexity(mesh) == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    row = {'name': name, 'asset': asset, 'materials': actual_slots, 'material_assets': actual_assets,
           'lods': [], 'source_positions_sha256': [], 'source_fbx_sha256': []}
    for lod, (contract, source) in enumerate(zip(lods, sources)):
        desc = mesh.get_static_mesh_description(lod)
        report.check(name + ' LOD' + str(lod) + ' exact render/source triangles', desc is not None and mesh.get_num_triangles(lod) == contract['triangles'] and desc.get_triangle_count() == contract['triangles'])
        actual, expected = native_points(desc), expected_points(source)
        bad, distance = missing(actual, expected)
        absent, reverse_distance = missing(expected, actual)
        report.check(name + ' LOD' + str(lod) + ' full bidirectional source vertices', not bad and not absent,
                     {'unexpected': bad, 'missing': absent, 'max_error_cm': max(distance, reverse_distance), 'tolerance_cm': TOL_CM})
        actual_bounds = bounds(actual)
        report.check(name + ' LOD' + str(lod) + ' saved metric bounds', bounds_error(actual_bounds, FBX.expected_bounds(contract)) <= TOL_CM)
        build = S.get_lod_build_settings(mesh, lod)
        scale = build.get_editor_property('build_scale3d')
        report.check(name + ' LOD' + str(lod) + ' unit scale/UV/thin faces/authored normals', max(abs(x-1) for x in (scale.x, scale.y, scale.z)) < VALUE_TOL
                     and build.use_full_precision_u_vs and not build.remove_degenerates and not build.recompute_normals
                     and build.recompute_tangents and build.use_mikk_t_space)
        report.check(name + ' LOD' + str(lod) + ' all query sections', mesh.get_num_sections(lod) > 0 and all(S.is_section_collision_enabled(mesh, lod, i) for i in range(mesh.get_num_sections(lod))))
        row['lods'].append({'triangles': mesh.get_num_triangles(lod), 'bounds_cm': actual_bounds})
        digest = hashlib.sha256()
        for point in actual:
            digest.update(struct.pack('<ddd', *point))
        row['source_positions_sha256'].append(digest.hexdigest())
        row['source_fbx_sha256'].append(sha(source))
    screens = list(S.get_lod_screen_sizes(mesh))
    policy = SCREENS if len(lods) == 3 else [1.]
    report.check(name + ' saved screens', len(screens) == len(policy) and all(abs(a-b) < VALUE_TOL for a, b in zip(screens, policy)), screens)
    report.data['meshes'].append(row)
    record_native(asset, report)
    return mesh


def verify_all(root, trims, report, sources=None):
    manifest, materials, trim, _ = sources or source_inputs(root, trims, report)
    report.data['meshes'], report.data['materials'] = [], []
    rows = {row['name']: row for row in materials}
    for row in materials:
        verify_instance(row, root, report)
    for name, row in manifest['parts'].items():
        verify_mesh(name, DEST+'/'+name+'.'+name, row['lods'], [source_path(root, l['fbx']) for l in row['lods']],
                    row['materials'], [object_path(rows[m]) for m in row['materials']], report)
    fallback = E.load_asset('/Engine/EngineMaterials/DefaultMaterial')
    report.check('existing runtime-rebound trim fallback', fallback is not None)
    for group in ('meshes', 'signs'):
        for name, row in trim[group].items():
            if row.get('hidden', False):
                continue
            verify_mesh(name, row['asset'], [{'triangles': row['result_tris'], 'bounds_local_m': row['bounds_local_m']}],
                        [source_path(trims, row['fbx'])], row['materials'], [fallback.get_path_name()]*len(row['materials']), report)
    report.check('complete nineteen saved native meshes', len(report.data['meshes']) == 19
                 and {m['name'] for m in report.data['meshes']} == RULES.expected_meshes(trim))
    report.check('complete nineteen mesh and fifteen MI fingerprints', len(report.data['native_files_sha256']) == 34
                 and len(report.data['materials']) == 15)
    report.flush()
