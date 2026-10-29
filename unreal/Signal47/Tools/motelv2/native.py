"""Narrow UE5.8 FBX/PBR helpers; imported only by explicit editor entry points.

Adapted from this project's verified townintegration import and per-LOD validator.
No directory deletion and no writes outside /Game/MotelV2.
"""
import hashlib
import json
import math
import struct
from pathlib import Path
import unreal

DEST = '/Game/MotelV2'
E = unreal.EditorAssetLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
M = unreal.MaterialEditingLibrary
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
# Measured unchanged on all 24 parts in the independently saved/reopened native
# acceptance of 2026-10-27 (UE5.8.4-58902052), not fresh FBX/autocompute defaults.
AUTHORED_SCREEN_SIZES = (2., .75, .5625)
SCREEN_POLICY_BASIS = {
    'report': 'Signal47-motel-v2-20261027/native-verify-r4.json',
    'sha256': '4ccd813e9cd9684a6c79e496e2bc8b62a1e7c6444873c4457f590f0d21a4426d',
    'accepted_checks': '24/24 final LOD3 distant screen threshold PASS',
    'engine': '5.8.4-58902052+++UE5+Release-5.8',
}


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



def apply_water_policy(mat, row, report):
    """Oversett klart kildevann til en ensidig, tonet overflate i Unreal.

    Det lyse atlaset kom fra et materiale med Transmission=1. Det kan ikke
    brukes alene som diffus farge i en vanlig gjennomskinnelig overflate.
    Behold atlas, ORM, normaler og opacity; bruk vannfargen som multiplikator.
    """
    if not row.get('water_surface'):
        return
    name = row['name']
    report.check(name + ' water policy targets exact pool material', name == 'M_MotelV2_PoolWater')
    report.check(name + ' water opacity retained', 0 < float(row.get('alpha', 1)) < 1)
    base_texture_path = DEST + '/Textures/T_' + name.removeprefix('M_') + '_base_color'
    nodes = list(M.get_material_expressions(mat))
    samples = [node for node in nodes if isinstance(node, unreal.MaterialExpressionTextureSample)
               and node.get_editor_property('texture') is not None
               and node.get_editor_property('texture').get_path_name().split('.')[0] == base_texture_path]
    report.check(name + ' existing water colour atlas', len(samples) == 1)
    # Gjenbruk bare noder fra denne avgrensede policyen ved gjentatt kjøring.
    tints = [node for node in nodes if isinstance(node, unreal.MaterialExpressionVectorParameter)
             and str(node.get_editor_property('parameter_name')) == 'WaterTint']
    mixes = [node for node in nodes if isinstance(node, unreal.MaterialExpressionMultiply)
             and str(node.get_editor_property('desc')) == 'MotelV2WaterTint']
    report.check(name + ' unique water policy nodes', len(tints) <= 1 and len(mixes) <= 1)
    tint = tints[0] if tints else expr(mat, unreal.MaterialExpressionVectorParameter, parameter_name='WaterTint')
    tint.set_editor_property('default_value', unreal.LinearColor(*row['base_color_linear'][:3], 1))
    mix = mixes[0] if mixes else expr(mat, unreal.MaterialExpressionMultiply, desc='MotelV2WaterTint')
    report.check(name + ' water atlas into tint', M.connect_material_expressions(samples[0], 'RGB', mix, 'A'))
    report.check(name + ' water tint multiplier', M.connect_material_expressions(tint, 'RGB', mix, 'B'))
    report.check(name + ' tinted water base colour', M.connect_material_property(mix, '', unreal.MaterialProperty.MP_BASE_COLOR))
    mat.set_editor_property('two_sided', False)
    mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)


def verify_water_policy(mat, row, report):
    """Kontroller lagret vannpolicy; visuell godkjenning krever et nytt spillbilde."""
    if not row.get('water_surface'):
        return
    name = row['name']
    check = report.check
    check(name + ' water single sided prevents bottom layer', not mat.get_editor_property('two_sided'))
    check(name + ' water remains translucent', mat.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_TRANSLUCENT)
    check(name + ' water supported lit shading', mat.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    check(name + ' water surface per pixel lighting', mat.get_editor_property('translucency_lighting_mode') == unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    base = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_BASE_COLOR)
    check(name + ' water base colour uses multiply', isinstance(base, unreal.MaterialExpressionMultiply))
    inputs = list(M.get_inputs_for_material_expression(mat, base))
    tints = [node for node in inputs if isinstance(node, unreal.MaterialExpressionVectorParameter)
             and str(node.get_editor_property('parameter_name')) == 'WaterTint']
    samples = [node for node in inputs if isinstance(node, unreal.MaterialExpressionTextureSample)]
    check(name + ' water tint and atlas connected', len(tints) == 1 and len(samples) == 1)
    value = tints[0].get_editor_property('default_value')
    actual_tint = [value.r, value.g, value.b]
    check(name + ' water authored tint', max(abs(a-b) for a,b in zip(actual_tint, row['base_color_linear'][:3])) < 1e-6, actual_tint)
    expected_texture_prefix = DEST + '/Textures/T_' + name.removeprefix('M_') + '_'
    check(name + ' water retains source colour atlas', samples[0].get_editor_property('texture').get_path_name().split('.')[0] == expected_texture_prefix + 'base_color')
    opacity = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_OPACITY)
    check(name + ' water opacity parameter connected', isinstance(opacity, unreal.MaterialExpressionScalarParameter)
          and str(opacity.get_editor_property('parameter_name')) == 'Opacity')
    check(name + ' water authored opacity retained', abs(float(opacity.get_editor_property('default_value'))-float(row['alpha'])) < 1e-6)
    for role, prop, channel in [('normal', unreal.MaterialProperty.MP_NORMAL, 'RGB'),
                                ('orm', unreal.MaterialProperty.MP_ROUGHNESS, 'G'),
                                ('orm', unreal.MaterialProperty.MP_METALLIC, 'B')]:
        node = M.get_material_property_input_node(mat, prop)
        check(name + ' water retains ' + str(prop), isinstance(node, unreal.MaterialExpressionTextureSample)
              and node.get_editor_property('texture').get_path_name().split('.')[0] == expected_texture_prefix + role)
        check(name + ' water channel ' + str(prop), str(M.get_material_property_input_node_output_name(mat, prop)) == channel)


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
    apply_water_policy(mat, row, report)
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


def configure_query_collision(mesh, report):
    """Keep thin triangles and explicitly enable their visibility-query geometry.

    UE5.8 FbxStaticMeshImport.cpp disables section collision when no custom
    collision is imported and RemoveDegenerates is false. BodySetup's complex
    trace flag alone does not re-enable those sections. Walker keeps its 2D
    gameplay colliders; runtime components still choose which channels to block.
    """
    body = mesh.get_editor_property('body_setup')
    report.check(mesh.get_name() + ' query BodySetup exists', body is not None)
    counts = [mesh.get_num_sections(lod) for lod in range(S.get_lod_count(mesh))]
    report.check(mesh.get_name() + ' query section population', bool(counts) and all(n > 0 for n in counts), counts)
    before = [[bool(S.is_section_collision_enabled(mesh, lod, section)) for section in range(count)]
              for lod, count in enumerate(counts)]
    report.data.setdefault('query_collision_before', {})[mesh.get_name()] = before
    report.flush()
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for lod, sections in enumerate(before):
        for section, enabled in enumerate(sections):
            if not enabled:
                S.enable_section_collision(mesh, True, lod, section)


def verify_query_collision(mesh, report):
    body = mesh.get_editor_property('body_setup')
    report.check(mesh.get_name() + ' saved complex query collision', body is not None and
                 body.get_editor_property('collision_trace_flag') == unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    for lod in range(S.get_lod_count(mesh)):
        count = mesh.get_num_sections(lod)
        report.check(mesh.get_name() + ' LOD' + str(lod) + ' saved query sections', count > 0 and
                     all(S.is_section_collision_enabled(mesh, lod, section) for section in range(count)), count)


def finish_mesh(mesh, report, authored_screens=None):
    authored = is_authored_motel_part(mesh)
    # SetLodBuildSettings and even proxy/Nanite property edits call PostEditChange.
    # GetLodScreenSizes reads RenderData, which can already contain zero defaults
    # after those edits. Freeze/set the approved source-model policy first.
    screens = None
    if authored:
        screens = authored_screen_policy(mesh, report, authored_screens)
        current = screens + ([.005] if S.get_lod_count(mesh) == 4 else [])
        report.check(mesh.get_name() + ' preserve accepted screens before property edits',
                     S.set_lod_screen_sizes(mesh, current))
    configure_query_collision(mesh, report)
    nanite = mesh.get_editor_property('nanite_settings')
    nanite.enabled = False
    mesh.set_editor_property('nanite_settings', nanite)
    for lod in range(S.get_lod_count(mesh)):
        settings = S.get_lod_build_settings(mesh, lod)
        settings.set_editor_property('use_full_precision_u_vs', True)
        settings.set_editor_property('remove_degenerates', False)
        S.set_lod_build_settings(mesh, lod, settings)
    if authored:
        set_resident_lod(mesh, report, screens)
    report.check(mesh.get_name() + ' saved mesh', E.save_loaded_asset(mesh))
    if authored:
        verify_authored_screens(mesh, report)


def is_authored_motel_part(mesh):
    return (mesh.get_name().startswith('SM_Sierra_') and
            mesh.get_path_name().split('.')[0] == DEST + '/' + mesh.get_name())


def rt_proxy_settings(mesh):
    settings = mesh.get_editor_property('ray_tracing_proxy_settings')
    return {'enabled': bool(settings.get_editor_property('enabled')),
            'fallback_target': str(settings.get_editor_property('fallback_target')),
            'fallback_percent_triangles': float(settings.get_editor_property('fallback_percent_triangles')),
            'fallback_relative_error': float(settings.get_editor_property('fallback_relative_error')),
            'support_ray_tracing': bool(mesh.get_editor_property('support_ray_tracing'))}


def resident_lod_policy(mesh):
    # Tiny single-plane water/glow pieces already have negligible residency.
    # Preserve their authored LOD2 exactly instead of risking zero-face reduction.
    # Source descriptions survive RenderData invalidation and are the newly
    # imported authored data. Zero transient render counts must not select LOD2.
    source = mesh.get_static_mesh_description(0)
    assert source is not None and source.get_triangle_count() > 0, ('authored LOD0 source missing', mesh.get_name())
    small = source.get_triangle_count() < 1000
    return {'source_lod': 2 if small else 0, 'percent_triangles': 1. if small else .02,
            'screen_size': .005, 'lod': 3}


def authored_screen_policy(mesh, report, approved=None):
    """Freeze actual screens, or explicitly reuse the accepted reimport policy.

    import_assets supplies the measured policy: deleting/reimporting an owned
    mesh cannot preserve its old source models. Other callers must still prove
    that the currently saved screens match the accepted values; zeros fail.
    """
    observed = list(S.get_lod_screen_sizes(mesh))[:3]
    screens = list(approved) if approved is not None else observed
    report.check(mesh.get_name() + ' accepted authored screen policy', len(screens) == 3 and
                 all(math.isfinite(a) and abs(a-b) < 1e-6 for a, b in zip(screens, AUTHORED_SCREEN_SIZES)),
                 {'observed_render_screens': observed, 'preserved_screens': screens,
                  'origin': 'explicit accepted reimport policy' if approved is not None else 'current saved policy',
                  'basis': SCREEN_POLICY_BASIS})
    return screens


def verify_authored_screens(mesh, report):
    actual = list(S.get_lod_screen_sizes(mesh))
    expected = list(AUTHORED_SCREEN_SIZES) + [.005]
    report.check(mesh.get_name() + ' saved authored screen policy retained', len(actual) == 4 and
                 all(math.isfinite(a) and abs(a-b) < 1e-6 for a, b in zip(actual, expected)),
                 {'actual': actual, 'expected': expected, 'basis': SCREEN_POLICY_BASIS})


def set_resident_lod(mesh, report, authored_screens=None):
    """Append a distant ordinary LOD; retain all three authored LODs exactly.

    UE5.8 only builds dedicated RT proxies inside the Nanite build gate. These
    non-Nanite assets therefore need an actual final ordinary LOD to reduce the
    engine's AlwaysResident member. No global policy or warning changes.
    """
    name = mesh.get_name()
    report.check(name + ' owned authored resident LOD target', is_authored_motel_part(mesh))
    report.check(name + ' initial three authored LODs or retry', S.get_lod_count(mesh) in (3, 4))
    report.check(name + ' hardware RT remains supported', mesh.get_editor_property('support_ray_tracing'))
    report.check(name + ' ordinary non-Nanite pipeline', not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
    # PostEditChange from even a proxy-only property can invalidate render data.
    # Freeze both screen sizes AND the source-triangle-based policy beforehand.
    policy = resident_lod_policy(mesh)
    old_screens = authored_screen_policy(mesh, report, authored_screens)
    report.check(name + ' existing far transition permits appended LOD', len(old_screens) == 3 and old_screens[2] > .0051, old_screens)
    report.data.setdefault('resident_lod_inputs', {})[name] = {
        'preserved_screens': old_screens, 'screen_policy_basis': SCREEN_POLICY_BASIS,
        'authored_lod0_source_triangles': mesh.get_static_mesh_description(0).get_triangle_count(), 'policy': policy}
    report.flush()
    proxy = mesh.get_editor_property('ray_tracing_proxy_settings')
    proxy.set_editor_property('enabled', False)
    mesh.set_editor_property('ray_tracing_proxy_settings', proxy)
    # UE5.8 StaticMeshEditorSubsystem.cpp:779..840 appends only when absent,
    # then overwrites destination MeshDescription/settings even for an existing
    # LOD. A 4-LOD retry must copy the NEW LOD0/2, never reduce stale old LOD3.
    report.check(name + ' refresh LOD3 from current authored source',
                 S.set_lod_from_static_mesh(mesh, 3, mesh, policy['source_lod'], True) == 3)
    reduction = S.get_lod_reduction_settings(mesh, 3)
    reduction.set_editor_property('base_lod_model', 3)
    reduction.set_editor_property('percent_triangles', policy['percent_triangles'])
    reduction.set_editor_property('percent_vertices', 1.)
    reduction.set_editor_property('max_deviation', 0.)
    S.set_lod_reduction_settings(mesh, 3, reduction)
    build = S.get_lod_build_settings(mesh, 3)
    build.set_editor_property('use_full_precision_u_vs', True)
    build.set_editor_property('remove_degenerates', False)
    S.set_lod_build_settings(mesh, 3, build)
    report.check(name + ' retain authored screens and append distant screen',
                 S.set_lod_screen_sizes(mesh, old_screens + [policy['screen_size']]))
    verify_rt_proxy(mesh, report)


def verify_rt_proxy(mesh, report):
    name = mesh.get_name()
    state = rt_proxy_settings(mesh)
    report.check(name + ' no ineffective non-Nanite dedicated proxy', not state['enabled'], state)
    if is_authored_motel_part(mesh):
        report.check(name + ' hardware RT support retained', state['support_ray_tracing'])
        report.check(name + ' actual final ordinary resident LOD exists', S.get_lod_count(mesh) == 4)
        report.check(name + ' saved non-Nanite pipeline', not mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
    return state


def verify_resident_lod(mesh, report, authored_position_hashes):
    name = mesh.get_name()
    policy = resident_lod_policy(mesh)
    actual = mesh.get_num_triangles(3)
    source_count = mesh.get_num_triangles(policy['source_lod'])
    target = source_count * policy['percent_triangles']
    reduction = S.get_lod_reduction_settings(mesh, 3)
    report.check(name + ' final LOD3 actual render triangles positive', actual > 0, actual)
    if policy['source_lod'] == 2:
        report.check(name + ' tiny final LOD retains authored LOD2 render count', actual == source_count)
    else:
        # Installed QuadricMeshReduction.cpp:277 clamps TargetNumTris to 64 to
        # preserve silhouettes. The authored .02 is a target, not an exact count.
        report.check(name + ' final LOD3 actual render triangle budget', actual <= max(64, target * 1.25),
                     {'actual': actual, 'requested_target': target, 'engine_minimum': 64, 'LOD0': source_count})
    report.check(name + ' final LOD3 reduction source', reduction.get_editor_property('base_lod_model') == 3)
    report.check(name + ' final LOD3 saved reduction fraction',
                 abs(float(reduction.get_editor_property('percent_triangles')) - policy['percent_triangles']) < 1e-6)
    screens = list(S.get_lod_screen_sizes(mesh))
    report.check(name + ' final LOD3 distant screen threshold', len(screens) == 4 and abs(screens[3] - .005) < 1e-6, screens)
    verify_authored_screens(mesh, report)
    desc = mesh.get_static_mesh_description(3)
    report.check(name + ' final LOD3 copied source MeshDescription', desc is not None and desc.get_triangle_count() == source_count)
    copied_positions = position_hash(vertices(desc))
    expected_positions = authored_position_hashes[policy['source_lod']]
    report.check(name + ' final LOD3 copied current authored vertex positions', copied_positions == expected_positions,
                 {'actual_sha256': copied_positions, 'source_sha256': expected_positions, 'source_lod': policy['source_lod']})
    return {'triangles': actual, 'generated_resident_lod': True, 'source_lod': policy['source_lod'],
            'source_triangles': source_count, 'reduction_fraction': policy['percent_triangles'], 'screen_size': screens[3],
            'copied_source_positions_sha256': copied_positions}


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


def position_hash(points):
    digest = hashlib.sha256()
    for point in points:
        digest.update(struct.pack('<ddd', *point))
    return digest.hexdigest()


def expected_bounds(row):
    lo, hi = row['bounds_local_m']
    return [[100 * lo[0], -100 * hi[1], 100 * lo[2]],
            [100 * hi[0], -100 * lo[1], 100 * hi[2]]]


def verify_mesh(name, mesh, lods, expected_materials, report):
    check = report.check
    check(name + ' saved native mesh', isinstance(mesh, unreal.StaticMesh))
    check(name + ' exact LOD count', S.get_lod_count(mesh) == len(lods) + (1 if is_authored_motel_part(mesh) else 0))
    verify_query_collision(mesh, report)
    slots = list(mesh.get_editor_property('static_materials'))
    actual_materials = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
    check(name + ' exact authored material slots', actual_materials == expected_materials,
          {'actual': actual_materials, 'expected': expected_materials})
    row = {'name': name, 'asset': mesh.get_path_name(), 'materials': actual_materials, 'lods': [],
           'ray_tracing_proxy_settings': verify_rt_proxy(mesh, report)}
    authored_position_hashes = {}
    for i, contract in enumerate(lods):
        label = name + ' LOD' + str(i)
        check(label + ' render triangles', mesh.get_num_triangles(i) == contract['triangles'],
              {'actual': mesh.get_num_triangles(i), 'expected': contract['triangles']})
        desc = mesh.get_static_mesh_description(i)
        check(label + ' saved source MeshDescription', desc is not None)
        check(label + ' source triangles', desc.get_triangle_count() == contract['triangles'])
        points = vertices(desc)
        if is_authored_motel_part(mesh):
            authored_position_hashes[i] = position_hash(points)
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
    if is_authored_motel_part(mesh):
        row['lods'].append(verify_resident_lod(mesh, report, authored_position_hashes))
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
    verify_rt_proxy(probe, report)


def verify_materials(rows, report):
    for row in rows:
        name = row['name']
        mat = E.load_asset(DEST + '/Materials/' + name)
        report.check(name + ' saved material exists', isinstance(mat, unreal.Material))
        verify_water_policy(mat, row, report)
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
    generated = [row for row in report.data['meshes'] if row['name'] in contract['parts']]
    total = sum(row['lods'][3]['triangles'] for row in generated)
    report.check('complete actual final LOD budget population', len(generated) == len(contract['parts']))
    report.check('aggregate actual final LOD triangles at most 24000', 0 < total <= 24000, total)
    report.data['resident_lod_budget'] = {
        'actual_triangles': total, 'maximum_triangles': 24000,
        'previous_authored_final_lod_triangles': sum(row['lod_triangles'][2] for row in contract['parts'].values()),
        'policy': 'Approximately 2% of LOD0, native reducer minimum64; tiny pieces retain LOD2',
        'scope': 'Actual saved render triangle count. Packaged GPU allocation remains separate.'}
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
