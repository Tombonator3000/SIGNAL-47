"""Guarded UE5.8 import of three privately baked, already-owned interior props.

Select the private bake with S47_INTERIOR_BAKE. This script starts no process.
S47_INTERIOR_VERIFY_ONLY=1 or -S47InteriorOwnedVerifyOnly opens saved assets without
changing them. The default old r3 bake is deliberately rejected until it has the
explicit source-material-index preservation proof required below.
"""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import traceback

import unreal

HERE = Path(__file__).resolve().parent
PROJECT = Path(unreal.Paths.project_dir()).resolve()
SOURCE = Path(os.environ.get('S47_INTERIOR_BAKE',
              str(Path.home() / 'scratch-bm/Signal47-interiors-20261029/owned-bake-r3'))).resolve()
DEST = '/Game/Licensed/SaroInteriors'
LOCAL_DEST = PROJECT / 'Content/Licensed/SaroInteriors'
NAMES = {'SM_InteriorOwned_SignalCable', 'SM_InteriorOwned_ArchiveBox', 'SM_InteriorOwned_Bookcase'}
BOX = 'SM_InteriorOwned_ArchiveBox'
DERIVATIVES = {BOX + 'Body', BOX + 'Lid'}
ROLES = {'base_color', 'roughness', 'metallic', 'normal'}
VERIFY_ONLY = (os.environ.get('S47_INTERIOR_VERIFY_ONLY') == '1' or
               'S47InteriorOwnedVerifyOnly' in unreal.SystemLibrary.get_command_line())
sys.path.insert(0, str(HERE.parent / 'fieldintegration'))
sys.path.insert(0, str(HERE))
import import_recipe as R
import fbx_geometry as G

R.REPORT = Path(unreal.Paths.project_saved_dir()) / 'interiors-owned-native.json'
R.report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(),
            'mode': 'verify_saved_only' if VERIFY_ONLY else 'import_and_verify',
            'source': str(SOURCE), 'asset_root': DEST, 'checks': [], 'meshes': [],
            'materials': [], 'geometry_correspondence': [], 'source_visual_acceptance': 'UNVERIFIED',
            'runtime_appearance': 'UNVERIFIED'}
R.flush()


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_inputs(source):
    contract_path, material_path, proof_path = [source / name for name in
                                               ('model_contract.json', 'materials.json', 'build-report.json')]
    for path in (contract_path, material_path, proof_path):
        R.check('private source exists ' + path.name, path.is_file())
    contract, rows, proof = [json.loads(path.read_text()) for path in (contract_path, material_path, proof_path)]
    population = set(contract['parts'])
    R.check('three original parts with optional exact body/lid pair', contract.get('schema') == 1 and
            population in (NAMES, NAMES | DERIVATIVES))
    R.check('completed source bake and matching part contract',
            proof.get('status') == 'PASS_PRIVATE_BAKE_AND_FBX_EXPORT' and proof.get('parts') == contract['parts'])
    preserved = (proof.get('source_material_indices_preserved') is True and
                 all(proof['parts'][name].get('source_material_indices_preserved') is True for name in NAMES))
    R.check('source material polygon indices explicitly preserved', preserved,
            'Older r3/R4 atlases without this proof are not accepted')
    R.check('only supported additional native feature',
            set(contract.get('required_native_features', [])) <= {'textures.metallic'})
    materials = {row['name']: row for row in rows}
    R.check('unique three material rows', len(materials) == len(rows) == 3)
    required = {name for part in contract['parts'].values() for name in part['materials']}
    R.check('exact contracted material population', set(materials) == required)
    for name in sorted(NAMES):
        part = contract['parts'][name]
        histogram = part.get('source_material_index_histogram', [])
        source_names = part['material_source_names']
        R.check(name + ' complete original slot histogram', len(histogram) == len(source_names) and
                all(row.get('slot') == index and row.get('material') == source_names[index] and
                    row.get('polygons', -1) >= 0 and row.get('triangles', -1) >= 0
                    for index, row in enumerate(histogram)) and
                sum(row['triangles'] for row in histogram) == part['triangles'] and
                sum(row['polygons'] for row in histogram) > 0)
        passes = part.get('bake_passes', {})
        R.check(name + ' every PBR pass preserved frozen polygon mapping', set(passes) == ROLES and
                all(row.get('source_material_indices_preserved') is True and
                    row.get('source_material_indices_sha256') == part.get('source_material_indices_sha256') and
                    len(row.get('source_material_indices_sha256', '')) == 64 for row in passes.values()))
        chrome = {row['material'] for row in histogram if row['polygons'] and 'chrome' in row['material'].lower()}
        R.check(name + ' used chrome has a nonzero actual metallic bake',
                set(passes['metallic'].get('used_chrome_source_materials', [])) == chrome and
                (not chrome or passes['metallic'].get('metallic_pixels', {}).get('nonzero_texels_over_002', 0) > 0))
        path = source / 'Export' / (name + '.fbx')
        R.check(name + ' exact exported FBX hash', path.is_file() and sha(path) == part['fbx_sha256'])
        R.check(name + ' one atlas slot and positive authored topology', len(part['materials']) == 1 and part['triangles'] > 0)
        row = materials[part['materials'][0]]
        R.check(name + ' all four PBR texture roles', set(row['textures']) == ROLES and set(part['texture_sha256']) == ROLES)
        R.check(name + ' full-strength baked normal and opaque mapping',
                row.get('normal_strength') == 1 and not row.get('opacity_from_base_color') and
                row.get('alpha', 1) == 1 and not row.get('two_sided', False))
        for role, relative in row['textures'].items():
            path = (source / relative).resolve()
            R.check(name + ' contained private texture ' + role, path.is_relative_to(source))
            R.check(name + ' exact baked texture ' + role, path.is_file() and sha(path) == part['texture_sha256'][role])
    partitions = contract.get('derived_partitions', {})
    if population == NAMES:
        R.check('no claimed subsets without derivative parts', not partitions)
    else:
        base = contract['parts'][BOX]
        R.check('only closed-box partition provenance', set(partitions) == {BOX})
        union = partitions[BOX]
        R.check('body/lid share exact closed source and atlas', union.get('base') == BOX and
                union.get('base_fbx_sha256') == base['fbx_sha256'] and
                union.get('body_triangles') == 515 and union.get('lid_triangles') == 145 and
                union.get('union_triangles') == base['triangles'] == 660 and
                all(union.get(key) is True for key in ('source_faces_disjoint', 'source_faces_complete',
                    'closed_geometry_unchanged', 'shared_origin', 'shared_baked_atlas')))
        islands = union.get('source_components', [])
        R.check('actual evaluated body and two lid islands',
                sorted((row['subset'],row['triangles']) for row in islands) ==
                [('body',515),('lid',25),('lid',120)] and
                sum(row['triangles'] for row in islands if row['subset'] == 'body') == 515 and
                sum(row['triangles'] for row in islands if row['subset'] == 'lid') == 145 and
                all((row['minimum_z_m'] > .2) == (row['subset'] == 'lid') for row in islands))
        selected = {}
        for name, count in ((BOX + 'Body', 515), (BOX + 'Lid', 145)):
            row = contract['parts'][name]
            R.check(name + ' geometry-only derivative inherits base bake proof',
                    row.get('derived_from') == BOX and row.get('geometry_only_derivative') is True and
                    row.get('source_material_proof_base') == BOX and row.get('shared_atlas_source') == BOX and
                    row.get('source_base_fbx_sha256') == base['fbx_sha256'] and
                    row.get('materials') == base['materials'] and row.get('texture_sha256') == base['texture_sha256'] and
                    row.get('shared_origin') == BOX + ' bottom centre' and
                    'bake_passes' not in row and 'source_material_indices_preserved' not in row)
            R.check(name + ' shared complete closed-geometry snapshot',
                    row.get('source_geometry_snapshot_sha256') == union.get('source_geometry_snapshot_sha256') and
                    len(row.get('source_geometry_snapshot_sha256', '')) == 64)
            proof = row.get('geometry_subset_proof', {})
            R.check(name + ' source coordinates UV winding normals and tangents verified',
                    all(proof.get(key) is True for key in ('exact_source_faces', 'vertex_coordinates_equal',
                        'uv_values_equal', 'triangle_winding_equal', 'bitangent_signs_equal')) and
                    0 <= proof.get('normal_max_error', 1) <= 1e-6 and
                    0 <= proof.get('tangent_max_error', 1) <= 1e-6)
            indices = row.get('source_triangle_indices', [])
            R.check(name + ' exact source triangle subset count', row['triangles'] == count and
                    len(indices) == len(set(indices)) == count and all(isinstance(index, int) for index in indices))
            selected[name] = set(indices)
            path = source / 'Export' / (name + '.fbx')
            R.check(name + ' exact geometry-only FBX hash', path.is_file() and sha(path) == row['fbx_sha256'])
        body, lid = [selected[BOX + suffix] for suffix in ('Body', 'Lid')]
        R.check('subset union exactly reconstructs all 660 closed-box triangles',
                not body.intersection(lid) and body.union(lid) == set(range(660)))
        bounds = [contract['parts'][name]['bounds_local_m'] for name in sorted(DERIVATIVES)]
        combined = [[min(row[0][axis] for row in bounds) for axis in range(3)],
                    [max(row[1][axis] for row in bounds) for axis in range(3)]]
        R.check('subset bounds reconstruct closed bounds without lid recentering',
                max(abs(combined[side][axis] - base['bounds_local_m'][side][axis])
                    for side in range(2) for axis in range(3)) < 1e-7 and
                abs(contract['parts'][BOX + 'Body']['bounds_local_m'][0][2]) < 1e-7 and
                contract['parts'][BOX + 'Lid']['bounds_local_m'][0][2] > .2)
    geometry = {}
    for name, row in contract['parts'].items():
        actual = G.read_geometry(source / 'Export' / (name + '.fbx'))
        expected_zeros = 192 if name == 'SM_InteriorOwned_SignalCable' else 0
        R.check(name + ' exact FBX geometry source identity', actual['fbx_sha256'] == row['fbx_sha256'] and
                actual['source_triangles'] == row['triangles'])
        R.check(name + ' only documented exact zero area source triangles',
                len(actual['exact_zero_area_indices']) == expected_zeros,
                {'source_triangles': actual['source_triangles'], 'exact_zero_area': len(actual['exact_zero_area_indices']),
                 'renderable_triangles': actual['renderable_triangles']})
        if expected_zeros:
            R.check(name + ' reviewed cable source and renderable populations',
                    actual['source_triangles'] == 14386 and actual['renderable_triangles'] == 14194)
        geometry[name] = actual
    return contract, materials, {path.name: sha(path) for path in (contract_path, material_path, proof_path)}, geometry


def connected(mat, expression):
    return list(R.M.get_inputs_for_material_expression(mat, expression))


def output_pin(parent, child):
    result = R.M.get_input_node_output_name_for_material_expression(parent, child)
    # Reflected UE functions with a bool result and FString out parameter can be
    # exposed as the out string, or as a tuple by a Python binding version.
    if isinstance(result, tuple):
        R.check('connected native graph pin query', len(result) == 2 and bool(result[0]))
        result = result[1]
    return str(result)


def texture_proof(name, role, sample, expected_hash):
    R.check(name + ' actual texture sample ' + role, isinstance(sample, unreal.MaterialExpressionTextureSample))
    texture = sample.get_editor_property('texture')
    R.check(name + ' private actual texture ' + role, isinstance(texture, unreal.Texture2D) and
            texture.get_path_name().startswith(DEST + '/Materials/Textures/'))
    data = texture.get_editor_property('asset_import_data')
    R.check(name + ' actual texture import source ' + role, data is not None)
    path = Path(data.get_first_filename())
    R.check(name + ' imported source pixels hash ' + role, path.is_file() and sha(path) == expected_hash)
    R.check(name + ' texture colour space ' + role, texture.get_editor_property('srgb') == (role == 'base_color'))
    expected_sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                        unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base_color' else
                        unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    R.check(name + ' native sampler ' + role, sample.get_editor_property('sampler_type') == expected_sampler)
    if role == 'normal':
        R.check(name + ' saved DirectX tangent texture',
                texture.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP and
                not texture.get_editor_property('flip_green_channel'))
    elif role in ('roughness', 'metallic'):
        R.check(name + ' saved linear PBR scalar texture ' + role,
                texture.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_DEFAULT)
    return texture.get_path_name()


def material_proof(name, mat, hashes):
    R.check(name + ' native opaque material', isinstance(mat, unreal.Material) and
            mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE)
    paths = {}
    for role, prop in (('roughness', unreal.MaterialProperty.MP_ROUGHNESS),
                       ('metallic', unreal.MaterialProperty.MP_METALLIC)):
        sample = R.M.get_material_property_input_node(mat, prop)
        R.check(name + ' direct native R channel ' + role,
                R.M.get_material_property_input_node_output_name(mat, prop) == 'R')
        paths[role] = texture_proof(name, role, sample, hashes[role])
    base = R.M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    R.check(name + ' base colour atlas multiply', isinstance(base, unreal.MaterialExpressionMultiply))
    inputs = connected(mat, base)
    R.check(name + ' exact base colour inputs', len(inputs) == 2 and
            isinstance(inputs[0], unreal.MaterialExpressionTextureSample) and
            isinstance(inputs[1], unreal.MaterialExpressionVectorParameter))
    tint = inputs[1].get_editor_property('default_value')
    R.check(name + ' neutral base colour tint and RGB',
            max(abs(tint.r - 1), abs(tint.g - 1), abs(tint.b - 1)) < 1e-7 and output_pin(base, inputs[0]) == 'RGB')
    paths['base_color'] = texture_proof(name, 'base_color', inputs[0], hashes['base_color'])
    normal = R.M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_NORMAL)
    R.check(name + ' normalized actual tangent graph', isinstance(normal, unreal.MaterialExpressionNormalize))
    inputs = connected(mat, normal)
    R.check(name + ' exact normal normalize input', len(inputs) == 1 and
            isinstance(inputs[0], unreal.MaterialExpressionLinearInterpolate))
    blend = inputs[0]
    values = connected(mat, blend)
    R.check(name + ' normal lerp A/B/Alpha wiring', len(values) == 3 and
            isinstance(values[0], unreal.MaterialExpressionConstant3Vector) and
            isinstance(values[1], unreal.MaterialExpressionTextureSample) and
            isinstance(values[2], unreal.MaterialExpressionScalarParameter))
    neutral = values[0].get_editor_property('constant')
    R.check(name + ' neutral normal and full-strength source normal',
            max(abs(neutral.r), abs(neutral.g), abs(neutral.b - 1)) < 1e-7 and
            str(values[2].get_editor_property('parameter_name')) == 'NormalStrength' and
            abs(values[2].get_editor_property('default_value') - 1) < 1e-7 and output_pin(blend, values[1]) == 'RGB')
    paths['normal'] = texture_proof(name, 'normal', values[1], hashes['normal'])
    R.report['materials'].append({'asset': mat.get_path_name(), 'textures': paths,
                                 'roughness_channel': 'R', 'metallic_channel': 'R', 'normal': 'DirectX tangent, strength 1'})


def configure_mesh(mesh, name, materials):
    for index, slot in enumerate(mesh.get_editor_property('static_materials')):
        label = str(slot.material_slot_name)
        R.check(name + ' known imported material slot ' + label, label in materials)
        mesh.set_material(index, materials[label])
    nanite = mesh.get_editor_property('nanite_settings')
    nanite.enabled = False
    mesh.set_editor_property('nanite_settings', nanite)
    mesh.set_editor_property('support_ray_tracing', True)
    settings = []
    for ratio, screen in ((1., 1.), (.55, .25), (.25, .08)):
        setting = unreal.StaticMeshReductionSettings()
        setting.set_editor_property('percent_triangles', ratio)
        setting.set_editor_property('screen_size', screen)
        settings.append(setting)
    options = unreal.StaticMeshReductionOptions()
    options.set_editor_property('auto_compute_lod_screen_size', False)
    options.set_editor_property('reduction_settings', settings)
    R.check(name + ' three native LODs created', R.S.set_lods(mesh, options) == 3)
    R.S.remove_collisions(mesh)
    body = mesh.get_editor_property('body_setup')
    R.check(name + ' body setup available', body is not None)
    default = body.get_editor_property('default_instance')
    default.set_editor_property('collision_profile_name', 'NoCollision')
    default.set_editor_property('collision_enabled', unreal.CollisionEnabled.NO_COLLISION)
    body.set_editor_property('default_instance', default)
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    for lod in range(3):
        build = R.S.get_lod_build_settings(mesh, lod)
        for prop, value in (('use_full_precision_u_vs', True), ('remove_degenerates', False),
                            ('recompute_normals', False), ('recompute_tangents', False)):
            build.set_editor_property(prop, value)
        R.S.set_lod_build_settings(mesh, lod, build)
        for section in range(mesh.get_num_sections(lod)):
            R.S.enable_section_collision(mesh, False, lod, section)
    R.check(name + ' native mesh saved', R.E.save_loaded_asset(mesh))


def native_geometry_proof(name, mesh, source):
    description = mesh.get_static_mesh_description(0)
    expected = source['renderable_triangles']
    R.check(name + ' imported source contains every finite FBX triangle only', description is not None and
            description.get_triangle_count() == expected,
            {'fbx_source_triangles': source['source_triangles'], 'fbx_exact_zero_area': len(source['exact_zero_area_indices']),
             'expected_mesh_description_triangles': expected})
    triangles = []
    # IDs can be sparse. Use indexed corner instances; UE5.8 GetTriangleVertices
    # has an appended-array wrapper issue, so do not use that convenience call.
    for index in range(expected * 4 + 64):
        triangle = unreal.TriangleID()
        if not triangle.import_text('(IDValue=' + str(index) + ')'):
            R.check(name + ' reflected triangle ID conversion', False, index)
        if not description.is_triangle_valid(triangle):
            continue
        corners = []
        for corner in range(3):
            instance = description.get_triangle_vertex_instance(triangle, corner)
            vertex = description.get_vertex_instance_vertex(instance)
            position = description.get_vertex_position(vertex)
            uv = description.get_vertex_instance_uv(instance, 0)
            corners.append((position.x, position.y, position.z, uv.x, uv.y))
        triangles.append({'id': index, 'corners': corners})
        if len(triangles) == expected:
            break
    R.check(name + ' complete actual MeshDescription triangle enumeration', len(triangles) == expected)
    # Fixed measured axes + DirectX V flip, complete one-to-one oriented corners
    # and UV0. Count differences alone never authorise a missing source face.
    proof = G.correspond(source, triangles)
    R.check(name + ' full finite position orientation and UV0 correspondence',
            proof['matched_finite_triangles'] == expected, proof)
    R.report['geometry_correspondence'].append({'name': name, **proof})
    return description


def mesh_proof(name, mesh, row, materials, source_geometry):
    triangles = [mesh.get_num_triangles(lod) for lod in range(mesh.get_num_lods())]
    R.check(name + ' exact renderable LOD0 and decreasing native LODs', len(triangles) == 3 and
            triangles[0] == source_geometry['renderable_triangles'] and triangles[0] > triangles[1] > triangles[2] > 0, triangles)
    description = native_geometry_proof(name, mesh, source_geometry)
    slots = mesh.get_editor_property('static_materials')
    labels = [str(slot.material_slot_name) for slot in slots]
    R.check(name + ' exact saved material slot population', labels == row['materials'] and all(
            slot.material_interface == materials[str(slot.material_slot_name)] for slot in slots))
    imported = mesh.get_editor_property('asset_import_data')
    R.check(name + ' original FBX normal and tangent import', imported is not None and
            imported.get_editor_property('normal_import_method') == unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    R.check(name + ' RT on and raster LOD geometry retained', mesh.get_editor_property('support_ray_tracing') and
            not mesh.get_editor_property('nanite_settings').enabled)
    low, high = row['bounds_local_m']
    bounds = mesh.get_bounds()
    centre = [bounds.origin.x, bounds.origin.y, bounds.origin.z]
    size = [bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2]
    wanted = [(low[0] + high[0]) * 50, -(low[1] + high[1]) * 50, (low[2] + high[2]) * 50]
    error = max([abs(a - b) for a, b in zip(centre, wanted)] +
                [abs(size[k] - (high[k] - low[k]) * 100) for k in range(3)])
    R.check(name + ' metric FBX dimensions and bottom-centred origin', error < .2,
            {'centre_cm': centre, 'dimensions_cm': size, 'max_error_cm': error})
    default = mesh.get_editor_property('body_setup').get_editor_property('default_instance')
    R.check(name + ' saved decorative NoCollision default',
            str(default.get_editor_property('collision_profile_name')) == 'NoCollision' and
            default.get_editor_property('collision_enabled') == unreal.CollisionEnabled.NO_COLLISION)
    uvs = []
    for lod in range(3):
        build = R.S.get_lod_build_settings(mesh, lod)
        R.check(name + ' saved imported normal/tangent and UV precision LOD' + str(lod),
                not build.get_editor_property('recompute_normals') and not build.get_editor_property('recompute_tangents') and
                build.get_editor_property('use_full_precision_u_vs') and not build.get_editor_property('remove_degenerates'))
        uv_count = R.S.get_num_uv_channels(mesh, lod)
        R.check(name + ' saved atlas UV channel LOD' + str(lod), uv_count >= 1)
        uvs.append(uv_count)
        count = mesh.get_num_sections(lod)
        R.check(name + ' no collision sections LOD' + str(lod), count > 0 and
                all(not R.S.is_section_collision_enabled(mesh, lod, section) for section in range(count)))
    R.report['meshes'].append({'asset': mesh.get_path_name(), 'triangles': triangles,
                              'source_triangles': row['triangles'],
                              'source_exact_zero_area_triangles': len(source_geometry['exact_zero_area_indices']),
                              'source_renderable_triangles': source_geometry['renderable_triangles'],
                              'native_mesh_description_triangles': description.get_triangle_count(),
                              'materials': labels, 'centre_cm': centre, 'dimensions_cm': size, 'uv_channels': uvs,
                              'collision': 'NoCollision', 'support_ray_tracing': True})


try:
    private = subprocess.run(['git', '-C', str(PROJECT), 'check-ignore', '-q',
                              str(LOCAL_DEST / '__private_probe__.uasset')], check=False)
    R.check('licensed native destination is actually Git-ignored', private.returncode == 0)
    tracked = subprocess.run(['git', '-C', str(PROJECT), 'ls-files', '--', str(LOCAL_DEST)],
                             capture_output=True, text=True, check=False)
    R.check('licensed destination contains no Git-tracked assets', tracked.returncode == 0 and not tracked.stdout.strip())
    R.check('licensed native destination has no symlink redirection', LOCAL_DEST.absolute() == LOCAL_DEST.resolve())
    contract, rows, hashes, geometry = source_inputs(SOURCE)
    R.report['source_receipts_sha256'] = hashes
    R.report['geometry_reader_sha256'] = sha(HERE / 'fbx_geometry.py')
    R.report['source_material_indices_preserved'] = True
    # Validate every source byte before creating any licensed material/mesh.
    materials = {}
    by_material = {contract['parts'][name]['materials'][0]: contract['parts'][name] for name in NAMES}
    for name, row in rows.items():
        mat = R.E.load_asset(DEST + '/Materials/' + name) if VERIFY_ONLY else R.material(row, DEST + '/Materials', SOURCE)
        material_proof(name, mat, by_material[name]['texture_sha256'])
        materials[name] = mat
    for name, row in contract['parts'].items():
        mesh = R.E.load_asset(DEST + '/' + name) if VERIFY_ONLY else R.import_mesh(SOURCE / 'Export' / (name + '.fbx'), DEST)
        R.check(name + ' native StaticMesh exists', isinstance(mesh, unreal.StaticMesh))
        if not VERIFY_ONLY:
            configure_mesh(mesh, name, materials)
        mesh_proof(name, mesh, row, materials, geometry[name])
    R.report['status'] = 'PASS_NATIVE_SAVED_VERIFY' if VERIFY_ONLY else 'PASS_NATIVE_IMPORT_AND_VERIFY'
    R.flush()
    unreal.log('S47 PRIVATE SARO INTERIORS NATIVE PASS: ' + str(len(contract['parts'])) +
               ' meshes; geometry-only subsets share base bake; hollow/runtime acceptance separate')
except Exception:
    R.report['status'] = 'FAIL'
    R.report['error'] = traceback.format_exc()
    R.flush()
    raise
