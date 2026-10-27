"""Narrow UE5.8 FBX/PBR helpers; imported only by explicit editor entry points.

Adapted from this project's verified townintegration import and per-LOD validator.
No directory deletion and no writes outside /Game/MotelV2.
"""
import hashlib
import json
from pathlib import Path
import unreal

DEST = '/Game/MotelV2'
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class Report:
    def __init__(self, path, kind):
        self.path = Path(path)
        self.data = {'status': 'RUNNING', 'kind': kind,
                     'engine': unreal.SystemLibrary.get_engine_version(), 'checks': [], 'meshes': []}
        self.flush()

    def flush(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        payload = json.dumps(self.data, indent=2) + '\n'
        temp = self.path.with_suffix(self.path.suffix + '.tmp')
        temp.write_text(payload)
        temp.replace(self.path)

    def check(self, name, condition, detail=None):
        self.data['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
        self.flush()
        assert condition, (name, detail)


def owned(path):
    assert path.startswith(DEST + '/') and '..' not in path, ('non-owned asset', path)
    return path


def expr(material, cls, **properties):
    node = M.create_material_expression(material, cls)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def make_material(row, root, report):
    check = report.check
    name = row['name']
    check('owned material name ' + name, name.startswith('M_MotelV2_') and '/' not in name)
    path = owned(DEST + '/Materials/' + name)
    mat = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    check(name + ' native material', isinstance(mat, unreal.Material))
    # UE5.8 DeleteAllMaterialExpressions leaves stale entries; delete a snapshot.
    for node in list(M.get_material_expressions(mat)):
        M.delete_material_expression(mat, node)
    check(name + ' clean graph', M.get_num_material_expressions(mat) == 0)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided', bool(row.get('two_sided', False)))
    col = row.get('base_color_linear', [.5, .5, .5, 1])
    base = expr(mat, unreal.MaterialExpressionVectorParameter, parameter_name='Color',
                default_value=unreal.LinearColor(*col[:3], 1))
    M.connect_material_property(base, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, default, prop in [('roughness', .5, unreal.MaterialProperty.MP_ROUGHNESS),
                               ('metallic', 0, unreal.MaterialProperty.MP_METALLIC),
                               ('specular', .5, unreal.MaterialProperty.MP_SPECULAR)]:
        node = expr(mat, unreal.MaterialExpressionScalarParameter, parameter_name=key.title(), default_value=float(row.get(key, default)))
        M.connect_material_property(node, '', prop)
    samples = {}
    for role, filename in row.get('textures', {}).items():
        check(name + ' supported PBR channel ' + role, role in ('base_color', 'orm', 'normal', 'roughness', 'metallic'))
        source = Path(filename)
        if not source.is_absolute():
            source = root / source
        check(name + ' texture exists ' + role, source.is_file(), str(source))
        texture_name = 'T_' + name.removeprefix('M_') + '_' + role
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = DEST + '/Textures'
        task.destination_name = texture_name
        task.replace_existing = True
        task.automated = True
        task.save = True
        T.import_asset_tasks([task])
        texture = E.load_asset(owned(task.destination_path + '/' + texture_name))
        check(name + ' imported texture ' + role, isinstance(texture, unreal.Texture2D))
        texture.set_editor_property('srgb', role == 'base_color')
        if role == 'normal':
            convention = row.get('normal_convention', 'DIRECTX').upper()
            check(name + ' explicit normal convention', convention in ('DIRECTX', 'OPENGL'), convention)
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP)
            texture.set_editor_property('flip_green_channel', convention == 'OPENGL')
        else:
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        E.save_loaded_asset(texture)
        sampler = (unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                   unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base_color' else
                   unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        node = expr(mat, unreal.MaterialExpressionTextureSample, texture=texture, sampler_type=sampler)
        samples[role] = node
        if role == 'orm':
            # Authored packed channels are AO, roughness, metallic. Never sRGB.
            for channel, prop in [('R', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION),
                                   ('G', unreal.MaterialProperty.MP_ROUGHNESS),
                                   ('B', unreal.MaterialProperty.MP_METALLIC)]:
                check(name + ' ORM ' + channel, M.connect_material_property(node, channel, prop))
        elif role == 'base_color':
            base = node
            if row.get('texture_tint_linear'):
                tint = expr(mat, unreal.MaterialExpressionVectorParameter, parameter_name='TextureTint',
                            default_value=unreal.LinearColor(*row['texture_tint_linear'][:3], 1))
                base = expr(mat, unreal.MaterialExpressionMultiply)
                M.connect_material_expressions(node, 'RGB', base, 'A')
                M.connect_material_expressions(tint, 'RGB', base, 'B')
                M.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)
            else:
                M.connect_material_property(base, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
        else:
            prop = {'normal': unreal.MaterialProperty.MP_NORMAL, 'roughness': unreal.MaterialProperty.MP_ROUGHNESS,
                    'metallic': unreal.MaterialProperty.MP_METALLIC}[role]
            M.connect_material_property(node, 'RGB' if role == 'normal' else 'R', prop)
    strength = float(row.get('emission_strength', 0))
    if row.get('emission_from_base_color'):
        check(name + ' authored emissive atlas', 'base_color' in samples and strength > 0)
        factor = expr(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor',
                      default_value=unreal.LinearColor(strength, strength, strength, 1))
        emissive = expr(mat, unreal.MaterialExpressionMultiply)
        M.connect_material_expressions(samples['base_color'], 'RGB', emissive, 'A')
        M.connect_material_expressions(factor, 'RGB', emissive, 'B')
        M.connect_material_property(emissive, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        emission = row.get('emission', [0, 0, 0, 1])
        factor = expr(mat, unreal.MaterialExpressionVectorParameter, parameter_name='EmissiveFactor',
                      default_value=unreal.LinearColor(*[float(v) * strength for v in emission[:3]], 1))
        M.connect_material_property(factor, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if float(row.get('alpha', 1)) < 1:
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        opacity = expr(mat, unreal.MaterialExpressionScalarParameter, parameter_name='Opacity', default_value=float(row['alpha']))
        M.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
    errors = list(M.recompile_material(mat))
    check(name + ' material compilation', not errors, errors)
    check(name + ' saved material', E.save_loaded_asset(mat))
    return mat


def import_mesh(source, destination, name, report):
    source = Path(source)
    report.check(name + ' FBX source', source.is_file() and source.suffix.lower() == '.fbx')
    path = owned(destination + '/' + name)
    # Replace exactly this owned mesh, removing any stale previous LOD count.
    if E.does_asset_exist(path):
        report.check(name + ' replace exact owned mesh', E.delete_asset(path))
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.import_as_skeletal = False
    options.import_mesh = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_materials = False
    options.import_textures = False
    data = options.static_mesh_import_data
    data.combine_meshes = True
    data.auto_generate_collision = False
    data.generate_lightmap_u_vs = True
    data.remove_degenerates = False
    data.convert_scene = True
    data.convert_scene_unit = True
    data.force_front_x_axis = True
    data.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    data.import_rotation = unreal.Rotator(pitch=0, yaw=90, roll=0)
    data.vertex_color_import_option = unreal.VertexColorImportOption.REPLACE
    data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination
    task.destination_name = name
    task.replace_existing = True
    task.replace_existing_settings = True
    task.automated = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    T.import_asset_tasks([task])
    mesh = E.load_asset(path)
    report.check(name + ' native mesh', isinstance(mesh, unreal.StaticMesh), list(task.imported_object_paths))
    return mesh


def finish_mesh(mesh, report):
    nanite = mesh.get_editor_property('nanite_settings')
    nanite.enabled = False
    mesh.set_editor_property('nanite_settings', nanite)
    body = mesh.get_editor_property('body_setup')
    if body:
        body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for lod in range(S.get_lod_count(mesh)):
        settings = S.get_lod_build_settings(mesh, lod)
        settings.set_editor_property('use_full_precision_u_vs', True)
        settings.set_editor_property('remove_degenerates', False)
        S.set_lod_build_settings(mesh, lod, settings)
    report.check(mesh.get_name() + ' saved mesh', E.save_loaded_asset(mesh))


def vertices(desc):
    count = desc.get_vertex_count()
    points = []
    index = 0
    while len(points) < count and index < count * 4 + 64:
        vertex = unreal.VertexID()
        assert vertex.import_text('(IDValue=' + str(index) + ')')
        if desc.is_vertex_valid(vertex):
            p = desc.get_vertex_position(vertex)
            points.append((p.x, p.y, p.z))
        index += 1
    assert len(points) == count and points, ('saved vertices', len(points), count)
    return points


def expected_bounds(row):
    lo, hi = row['bounds_local_m']
    return [[100 * lo[0], -100 * hi[1], 100 * lo[2]],
            [100 * hi[0], -100 * lo[1], 100 * hi[2]]]


def verify_mesh(name, mesh, lods, expected_materials, report):
    check = report.check
    check(name + ' saved native mesh', isinstance(mesh, unreal.StaticMesh))
    check(name + ' exact LOD count', S.get_lod_count(mesh) == len(lods))
    slots = list(mesh.get_editor_property('static_materials'))
    actual_materials = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
    check(name + ' exact authored material slots', actual_materials == expected_materials,
          {'actual': actual_materials, 'expected': expected_materials})
    row = {'name': name, 'asset': mesh.get_path_name(), 'materials': actual_materials, 'lods': []}
    for i, contract in enumerate(lods):
        label = name + ' LOD' + str(i)
        check(label + ' render triangles', mesh.get_num_triangles(i) == contract['triangles'],
              {'actual': mesh.get_num_triangles(i), 'expected': contract['triangles']})
        desc = mesh.get_static_mesh_description(i)
        check(label + ' saved source MeshDescription', desc is not None)
        check(label + ' source triangles', desc.get_triangle_count() == contract['triangles'])
        points = vertices(desc)
        actual = [[fn(p[a] for p in points) for a in range(3)] for fn in (min, max)]
        expected = expected_bounds(contract)
        error = max(abs(a - b) for x, y in zip(actual, expected) for a, b in zip(x, y))
        check(label + ' metric bounds and orientation', error < .10,
              {'actual_cm': actual, 'expected_cm': expected, 'max_error_cm': error})
        build = S.get_lod_build_settings(mesh, i)
        scale = build.get_editor_property('build_scale3d')
        check(label + ' unit render scale', max(abs(v - 1) for v in (scale.x, scale.y, scale.z)) < 1e-6)
        check(label + ' full precision UVs', build.get_editor_property('use_full_precision_u_vs'))
        check(label + ' preserve thin triangles', not build.get_editor_property('remove_degenerates'))
        row['lods'].append({'triangles': mesh.get_num_triangles(i), 'bounds_cm': actual})
    report.data['meshes'].append(row)
    report.flush()


def load_contract(root, report):
    contract = json.loads((root / 'manifest.json').read_text())
    materials = json.loads((root / 'materials.json').read_text())
    report.check('nonempty contracted motel', bool(contract.get('parts')) and isinstance(materials, list))
    report.check('unique material names', len({r['name'] for r in materials}) == len(materials))
    policy = contract['native_import_policy']
    report.check('explicit verified FBX import policy', policy.get('force_front_x_axis') is True
                 and policy.get('coordinate_system_policy') == 'MATCH_UP_FORWARD_AXES'
                 and policy.get('import_rotation_yaw') == 90)
    for name, row in contract['parts'].items():
        report.check(name + ' owned part name', name.startswith('SM_') and '/' not in name)
        report.check(name + ' authored three LODs', len(row['lods']) == 3 and
                     row['lod_triangles'] == [lod['triangles'] for lod in row['lods']])
        report.check(name + ' base LOD contract', row['lods'][0]['bounds_local_m'] == row['bounds_local_m'])
    report.data['source_sha256'] = {'manifest': sha(root / 'manifest.json'), 'materials': sha(root / 'materials.json')}
    sources = {root / 'manifest.json', root / 'materials.json', root / 'exports/axis_probe.fbx'}
    for name, row in contract['parts'].items():
        for lod in range(len(row['lods'])):
            sources.add(root / 'exports' / (name + ('' if lod == 0 else '_LOD' + str(lod)) + '.fbx'))
    for row in materials:
        for filename in row.get('textures', {}).values():
            path = Path(filename)
            sources.add(path if path.is_absolute() else root / path)
    for path in sources:
        report.check('authored source file ' + path.name, path.is_file())
    report.data['source_files_sha256'] = {str(p.resolve()): sha(p) for p in sorted(sources)}
    return contract, materials


def verify_probe(report):
    probe = E.load_asset(DEST + '/Verification/axis_probe')
    report.check('saved asymmetric axis probe', isinstance(probe, unreal.StaticMesh))
    b = probe.get_bounds()
    actual = [b.origin.x, b.origin.y, b.origin.z, 2 * b.box_extent.x, 2 * b.box_extent.y, 2 * b.box_extent.z]
    expected = [200, -400, 600, 20, 40, 60]
    report.check('Blender metre axes and handedness', max(abs(a - b) for a, b in zip(actual, expected)) < .15,
                 {'actual_cm': actual, 'expected_cm': expected})


def verify_materials(rows, report):
    for row in rows:
        name = row['name']
        mat = E.load_asset(DEST + '/Materials/' + name)
        report.check(name + ' saved material exists', isinstance(mat, unreal.Material))
        for role in row.get('textures', {}):
            tex = E.load_asset(DEST + '/Textures/T_' + name.removeprefix('M_') + '_' + role)
            report.check(name + ' saved texture ' + role, isinstance(tex, unreal.Texture2D))
            report.check(name + ' colour space ' + role, bool(tex.get_editor_property('srgb')) == (role == 'base_color'))
            if role == 'normal':
                report.check(name + ' normal compression', tex.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP)
                report.check(name + ' normal convention conversion', bool(tex.get_editor_property('flip_green_channel')) == (row.get('normal_convention', 'DIRECTX').upper() == 'OPENGL'))


def verify_all(root, trim_root, report):
    contract, materials = load_contract(root, report)
    verify_probe(report)
    verify_materials(materials, report)
    for name, row in contract['parts'].items():
        mesh = E.load_asset(DEST + '/' + name)
        verify_mesh(name, mesh, row['lods'], row['materials'], report)
        for i, slot in enumerate(mesh.get_editor_property('static_materials')):
            material = slot.get_editor_property('material_interface')
            expected = DEST + '/Materials/' + row['materials'][i]
            report.check(name + ' bound material slot ' + str(i), material is not None and material.get_path_name().split('.')[0] == expected)
    if trim_root:
        trims = json.loads((trim_root / 'trim-manifest.json').read_text())
        report.data['source_sha256']['trim_manifest'] = sha(trim_root / 'trim-manifest.json')
        for filename in ['trim-manifest.json', 'trim-fbx.json'] + [row['fbx'] for row in trims['meshes'].values() if not row['hidden']]:
            path = trim_root / filename
            report.data['source_files_sha256'][str(path.resolve())] = sha(path)
        for name, row in trims['meshes'].items():
            if row['hidden']:
                report.check(name + ' fully removed trim', row['result_tris'] == 0 and not row['asset'] and not row['original_material_indices'])
                continue
            mesh = E.load_asset(row['asset'])
            lods = [{'triangles': row['result_tris'], 'bounds_local_m': row['bounds_local_m']}]
            slots = ['M_MotelV2_Trim_' + name + '_slot' + str(i) for i in row['original_material_indices']]
            verify_mesh(name, mesh, lods, slots, report)
    return contract
