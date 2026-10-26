"""Create the eight selected PRIVATE route-nature copies in UE5.8.

No process launch. Run after staged-copy inspection/channel review under the shared
heavy-job guard. Set S47_RN_VERIFY_ONLY=1 for a fresh, read-only reopen verification.
Keeps source geometry, UVs, authored LODs and Nanite raster settings. Only local
material bindings and independent RT proxies change; vendor originals are immutable.
"""
from pathlib import Path
import hashlib
import json
import os
import sys
import traceback
import unreal

HERE = Path(__file__).resolve().parent
PROJECT = HERE.parents[1]
WORK = Path(os.environ.get('S47_RN_WORK', str(Path.home() / 'scratch-bm/Signal47-terrain-20261026/assets')))
CONFIG = json.loads((HERE / 'selection.json').read_text())
DEST = CONFIG['destination']
VERIFY = os.environ.get('S47_RN_VERIFY_ONLY') == '1'
REPORT = WORK / ('native-verify.json' if VERIFY else 'native-import.json')
E = unreal.EditorAssetLibrary
M = unreal.MaterialEditingLibrary
T = unreal.AssetToolsHelpers.get_asset_tools()
S = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.new_object(unreal.StaticMeshEditorSubsystem)
REG = unreal.AssetRegistryHelpers.get_asset_registry()
sys.path.insert(0, str(HERE.parent))
import material_usage

report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(),
          'verify_only': VERIFY, 'checks': [], 'meshes': [], 'materials': {}, 'textures': {},
          'destination': DEST}

def flush():
    WORK.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2) + '\n')

def check(name, condition, detail=None):
    report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
    flush()
    assert condition, (name, detail)

def source_geometry(mesh):
    desc = mesh.get_static_mesh_description(0)
    assert desc, mesh.get_path_name()
    return {'vertices': desc.get_vertex_count(), 'triangles': desc.get_triangle_count()}

def source_settings(mesh):
    # export_text is the engine StructBase serialization API, not repr(), whose
    # memory addresses vary. Compare every reflected setting, not a short subset.
    # PyWrapperStruct.cpp registers this API as export_text(self) -> str.
    lods = []
    for lod in range(mesh.get_num_lods()):
        desc = mesh.get_static_mesh_description(lod)
        lods.append({'source': {'vertices': desc.get_vertex_count(), 'triangles': desc.get_triangle_count()} if desc else None,
                     'build': S.get_lod_build_settings(mesh, lod).export_text(),
                     'reduction': S.get_lod_reduction_settings(mesh, lod).export_text()})
    return {'nanite': mesh.get_editor_property('nanite_settings').export_text(), 'authored_lods': lods}

def mesh_info(mesh):
    bounds = mesh.get_bounding_box()
    return {'bounds_cm': [[bounds.min.x, bounds.min.y, bounds.min.z], [bounds.max.x, bounds.max.y, bounds.max.z]],
            'lod_triangles': [mesh.get_num_triangles(i) for i in range(mesh.get_num_lods())],
            'lod_count': mesh.get_num_lods(), 'source': source_geometry(mesh),
            'nanite_enabled': bool(mesh.get_editor_property('nanite_settings').enabled),
            'screens': list(S.get_lod_screen_sizes(mesh)),
            'slots': [{'name': str(x.material_slot_name), 'material': x.material_interface.get_path_name() if x.material_interface else None}
                      for x in mesh.get_editor_property('static_materials')]}

def copy_asset(source, destination, cls):
    check('private destination ' + destination, destination.startswith(DEST + '/'))
    asset = E.load_asset(destination) if E.does_asset_exist(destination) else E.duplicate_asset(source, destination)
    check('local copy ' + destination, isinstance(asset, cls))
    return asset

def texture(source, role):
    name = source.rsplit('/', 1)[-1]
    target = DEST + '/Textures/' + name
    if target in report['textures']:
        return E.load_asset(target)
    tex = E.load_asset(target) if VERIFY else copy_asset(source, target, unreal.Texture2D)
    check('texture exists ' + name, isinstance(tex, unreal.Texture2D))
    desired = {'srgb': role == 'base',
               'compression_settings': unreal.TextureCompressionSettings.TC_NORMALMAP if role == 'normal'
                   else unreal.TextureCompressionSettings.TC_DEFAULT if role == 'base' else unreal.TextureCompressionSettings.TC_MASKS,
               'max_texture_size': 4096 if role == 'base' and 'Cliff' in name else 2048}
    changes = {key: value for key, value in desired.items() if tex.get_editor_property(key) != value}
    if not VERIFY and changes:
        # No setter/save for already-correct copies during a resumed import.
        for key, value in changes.items():
            tex.set_editor_property(key, value)
        check('save texture ' + name, E.save_loaded_asset(tex, only_if_is_dirty=False))
    check('texture settings ' + name, all(tex.get_editor_property(key) == value for key, value in desired.items()))
    check('texture color space ' + name, tex.get_editor_property('srgb') == (role == 'base'))
    report['textures'][target] = {'source': source, 'role': role,
                                'srgb': bool(tex.get_editor_property('srgb')),
                                'compression': str(tex.get_editor_property('compression_settings')),
                                'max_texture_size': tex.get_editor_property('max_texture_size')}
    return tex

def node(mat, cls, **props):
    expr = M.create_material_expression(mat, cls)
    for key, value in props.items():
        expr.set_editor_property(key, value)
    return expr

def make_material(name, spec):
    path = DEST + '/Materials/' + name
    textures = {role: texture(source, 'normal' if role == 'normal' else 'base' if role == 'base' else 'mask')
                for role, source in spec['textures'].items()}
    reuse = E.does_asset_exist(path)
    if VERIFY or reuse:
        # Existing graphs are fully verified below without rewriting nodes or
        # material state IDs, avoiding an unnecessary dependent Nanite rebuild.
        material = E.load_asset(path)
        check('saved material ' + name, isinstance(material, unreal.Material))
    else:
        material = E.load_asset(path) if E.does_asset_exist(path) else T.create_asset(name, DEST + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
        for expr in list(M.get_material_expressions(material)):
            M.delete_material_expression(material, expr)
        check('clean local graph ' + name, M.get_num_material_expressions(material) == 0)
        material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED if spec.get('masked') else unreal.BlendMode.BLEND_OPAQUE)
        material.set_editor_property('two_sided', bool(spec.get('masked')))
        material.set_editor_property('opacity_mask_clip_value', float(spec.get('clip', .333)))
        samples = {}
        for role, source in spec['textures'].items():
            tex = textures[role]
            samples[role] = node(material, unreal.MaterialExpressionTextureSample, texture=tex,
                                 sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if role == 'normal' else
                                 unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'base' else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        tint = node(material, unreal.MaterialExpressionVectorParameter, parameter_name='TextureTint', default_value=unreal.LinearColor(1, 1, 1, 1))
        multiply = node(material, unreal.MaterialExpressionMultiply)
        check('connect base RGB ' + name, M.connect_material_expressions(samples['base'], 'RGB', multiply, 'A'))
        check('connect tint RGB ' + name, M.connect_material_expressions(tint, 'RGB', multiply, 'B'))
        check('connect BaseColor ' + name, M.connect_material_property(multiply, '', unreal.MaterialProperty.MP_BASE_COLOR))
        check('connect normal RGB ' + name, M.connect_material_property(samples['normal'], 'RGB', unreal.MaterialProperty.MP_NORMAL))
        for prop_name, binding in spec['channels'].items():
            role, output = binding
            check(name + ' channel exists ' + prop_name, role in samples and output in ('R', 'G', 'B', 'A', 'RGB'))
            check('connect ' + name + ' ' + prop_name, M.connect_material_property(samples[role], output, getattr(unreal.MaterialProperty, prop_name)))
        for pname, prop_name, value in [('Metallic', 'MP_METALLIC', 0.), ('Specular', 'MP_SPECULAR', .18)]:
            scalar = node(material, unreal.MaterialExpressionScalarParameter, parameter_name=pname, default_value=value)
            check('connect ' + name + ' ' + prop_name, M.connect_material_property(scalar, '', getattr(unreal.MaterialProperty, prop_name)))
        material_usage.ensure_usage(material, [unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES,
                                              unreal.MaterialUsage.MATUSAGE_NANITE], apply=True)
        errors = M.recompile_material(material)
        check('material compile ' + name, not errors, str(errors))
        check('save material ' + name, E.save_loaded_asset(material, only_if_is_dirty=False))
    check('instancing usage ' + name, M.has_material_usage(material, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES))
    check('material blend settings ' + name,
          material.get_editor_property('blend_mode') == (unreal.BlendMode.BLEND_MASKED if spec.get('masked') else unreal.BlendMode.BLEND_OPAQUE) and
          material.get_editor_property('two_sided') == bool(spec.get('masked')))
    classes = [expr.get_class().get_name() for expr in M.get_material_expressions(material)]
    check('direct PBR only ' + name, not any('FunctionCall' in cls or 'VirtualTexture' in cls or 'Landscape' in cls for cls in classes), classes)
    for prop_name, (role, output) in dict(spec['channels'], MP_NORMAL=['normal', 'RGB']).items():
        native_prop = getattr(unreal.MaterialProperty, prop_name)
        sample = M.get_material_property_input_node(material, native_prop)
        check('actual property sample ' + name + ' ' + prop_name,
              isinstance(sample, unreal.MaterialExpressionTextureSample) and
              sample.get_editor_property('texture') == textures[role] and
              M.get_material_property_input_node_output_name(material, native_prop) == output)
    # GetUsedTextures traverses matched FMaterialResource shader-platform/quality
    # resources (UE5.8 Material.cpp:1300), which may be absent under NullRHI.
    # Verify the saved editor graph instead: BaseColor <- Multiply <- exact
    # private RGB texture and neutral TextureTint. The remaining properties were
    # verified above against their exact sample, texture object and output pin.
    base = M.get_material_property_input_node(material, unreal.MaterialProperty.MP_BASE_COLOR)
    check('actual BaseColor multiply ' + name,
          isinstance(base, unreal.MaterialExpressionMultiply) and
          M.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_BASE_COLOR) == '')
    inputs = [value for value in M.get_inputs_for_material_expression(material, base) if value]
    base_samples = [value for value in inputs if isinstance(value, unreal.MaterialExpressionTextureSample)]
    tints = [value for value in inputs if isinstance(value, unreal.MaterialExpressionVectorParameter)]
    check('actual BaseColor private RGB input ' + name,
          len(inputs) == 2 and len(base_samples) == 1 and
          base_samples[0].get_editor_property('texture') == textures['base'] and
          M.get_input_node_output_name_for_material_expression(base, base_samples[0]) == 'RGB')
    check('actual BaseColor tint input ' + name,
          len(tints) == 1 and str(tints[0].get_editor_property('parameter_name')) == 'TextureTint' and
          M.get_input_node_output_name_for_material_expression(base, tints[0]) == 'RGB')
    color = tints[0].get_editor_property('default_value')
    check('neutral BaseColor tint ' + name,
          all(abs(value - 1.) < 1.e-6 for value in (color.r, color.g, color.b, color.a)))
    actual_textures = {expr.get_editor_property('texture').get_path_name()
                       for expr in M.get_material_expressions(material)
                       if isinstance(expr, unreal.MaterialExpressionTextureSample) and expr.get_editor_property('texture')}
    expected_textures = {tex.get_path_name() for tex in textures.values()}
    check('exact private texture set ' + name, actual_textures == expected_textures, sorted(actual_textures))
    report['materials'][name] = {'path': path, 'source_maps': spec['textures'], 'channels': spec['channels'],
                                'masked': bool(spec.get('masked')),
                                'texture_binding_evidence': 'Saved material property pins and complete reachable BaseColor multiply/RGB/tint graph; exact texture objects. Render resource queries are not used as NullRHI editor-graph evidence.'}
    return material

def expected_proxy(row, source_triangles):
    result = {'enabled': row['ray_tracing']}
    if row['ray_tracing']:
        result.update({'fallback_target': unreal.NaniteFallbackTarget.PERCENT_TRIANGLES,
                       'fallback_percent_triangles': min(1., row['rt_proxy_target_triangles'] / source_triangles),
                       'fallback_relative_error': 0., 'foliage_over_occlusion_bias': 0.})
    return result

def proxy_matches(mesh, desired):
    current = mesh.get_editor_property('ray_tracing_proxy_settings')
    for key, value in desired.items():
        actual = current.get_editor_property(key)
        if isinstance(value, float):
            if abs(actual - value) > 1.e-7:
                return False
        elif actual != value:
            return False
    return True

def dependencies(roots):
    options = unreal.AssetRegistryDependencyOptions()
    options.set_editor_property('include_hard_package_references', True)
    options.set_editor_property('include_soft_package_references', True)
    queue, result = list(roots), {}
    while queue:
        path = queue.pop()
        if path in result:
            continue
        deps = [str(x) for x in REG.get_dependencies(path, options)]
        result[path] = deps
        queue.extend(x for x in deps if x.startswith('/Game/'))
    return result

def main():
    channels = json.loads((HERE / 'materials.json').read_text())
    check('native channel graph reviewed', channels['native_channel_review'] == 'PASS', channels.get('proof'))
    REG.scan_paths_synchronous(['/Game/Northwood', '/Game/JustBStudios', DEST], True)
    original = json.loads((WORK / 'native-source-inspection.json').read_text())
    check('original native source inspection available', original['status'] == 'PASS_READ_ONLY_NATIVE_INSPECTION')
    before = {row['name']: row for row in original['meshes']}
    mats = {name: make_material(name, spec) for name, spec in channels['materials'].items()}
    for row in CONFIG['meshes']:
        name, path = row['name'], row['destination']
        mesh = E.load_asset(path) if VERIFY else copy_asset(row['source_package'], path, unreal.StaticMesh)
        check('selected mesh ' + name, isinstance(mesh, unreal.StaticMesh))
        source = E.load_asset(row['source_package'])
        check('original mesh available ' + name, isinstance(source, unreal.StaticMesh))
        original_settings = source_settings(source)
        check('original source settings before edit ' + name,
              source_settings(mesh) == original_settings)
        bindings = channels['mesh_materials'][name]
        slots = mesh.get_editor_property('static_materials')
        check('slot count ' + name, len(bindings) == len(slots))
        expected_materials = [mats[material] for material in bindings]
        desired_proxy = expected_proxy(row, before[name]['source_triangles'])
        desired_metadata = {'S47_RN_SOURCE_SHA256': row['source_sha256'],
                            'S47_RN_ORIGINAL_PACKAGE': row['source_package']}
        desired_screens = [1., .35, .14, .04, .00001] if name == 'SM_RN_DryScrub_A' else None
        screens_match = desired_screens is None or (len(S.get_lod_screen_sizes(mesh)) == len(desired_screens) and
                        all(abs(a - b) < 1.e-7 for a, b in zip(S.get_lod_screen_sizes(mesh), desired_screens)))
        changes = []
        if not VERIFY:
            # Resume is conditional on actual native state, not an earlier PASS
            # or mere asset existence. Avoid all setters and Save/PostEditChange
            # for a correctly saved mesh: those can rebuild a 2M-triangle cliff.
            for slot, material in enumerate(expected_materials):
                if slots[slot].material_interface != material:
                    mesh.set_material(slot, material)
                    changes.append('material:' + str(slot))
            if mesh.get_editor_property('support_ray_tracing') != row['ray_tracing']:
                mesh.set_editor_property('support_ray_tracing', row['ray_tracing'])
                changes.append('support_ray_tracing')
            if not proxy_matches(mesh, desired_proxy):
                if row['ray_tracing']:
                    check('Nanite prerequisite for independent proxy ' + name, mesh.get_editor_property('nanite_settings').enabled)
                proxy = mesh.get_editor_property('ray_tracing_proxy_settings')
                for key, value in desired_proxy.items():
                    proxy.set_editor_property(key, value)
                mesh.set_editor_property('ray_tracing_proxy_settings', proxy)
                changes.append('ray_tracing_proxy_settings')
            if not screens_match:
                check('scrub keeps five authored LODs', mesh.get_num_lods() == 5)
                check('scrub billboard after component cull', S.set_lod_screen_sizes(mesh, desired_screens))
                changes.append('scrub_screen_sizes')
            for key, value in desired_metadata.items():
                if E.get_metadata_tag(mesh, key) != value:
                    E.set_metadata_tag(mesh, key, value)
                    changes.append('metadata:' + key)
            if changes:
                check('save mesh ' + name, E.save_loaded_asset(mesh, only_if_is_dirty=False))
        # Materials have already been checked and saved above. Verification here
        # is read-only, including resume and optional maintenance verification.
        for usage in material_usage.ensure_mesh_material_usage(mesh, instanced=True, apply=False):
            check('actual mesh usage ' + name, usage['after'], usage)
        data = mesh_info(mesh)
        check('original source vertex/triangle counts ' + name,
              data['source'] == {'vertices': before[name]['source_vertices'], 'triangles': before[name]['source_triangles']}, data['source'])
        current_settings = source_settings(mesh)
        check('all original Nanite raster and authored LOD settings ' + name,
              current_settings == original_settings, current_settings)
        check('original LOD count ' + name, data['lod_count'] == len(before[name]['lod_triangles']))
        reference_lods = before[name]['lod_triangles']
        # GetNumTriangles(i) always reads built RenderData.LODResources[i],
        # including ordinary reduced LODs. It never establishes authored-source
        # fidelity. That is checked above for every available MeshDescription
        # and every original build/reduction/Nanite setting. Rebuilding source
        # packages in this engine may change generated render counts slightly.
        render_deltas = [actual - original for actual, original in zip(data['lod_triangles'], reference_lods)]
        check('built render LOD counts bounded ' + name,
              all(actual > 0 and abs(actual - original) <= max(1., original * .15)
                  for actual, original in zip(data['lod_triangles'], reference_lods)),
              {'inspected': reference_lods, 'rebuilt': data['lod_triangles'],
               'delta': render_deltas, 'max_fraction': .15})
        # Bounds can include the regenerated fallback/proxy union. Limit that
        # union drift to 0.25% of the original largest dimension (min 1mm);
        # source topology/settings above must still be unchanged. Runtime uses
        # the final actual bounds for contact and conservative colliders.
        original_bounds = before[name]['bounds_cm']
        bound_limit = max(.1, max(b - a for a, b in zip(*original_bounds)) * .0025)
        bound_deltas = [[a - b for a, b in zip(actual, original)]
                        for actual, original in zip(data['bounds_cm'], original_bounds)]
        check('bounds preserve original scale and pivot ' + name,
              max(abs(x) for pair in bound_deltas for x in pair) <= bound_limit,
              {'delta_cm': bound_deltas, 'limit_cm': bound_limit})
        check('exact private material bindings ' + name,
              [slot.material_interface for slot in mesh.get_editor_property('static_materials')] == expected_materials)
        check('RT policy ' + name, mesh.get_editor_property('support_ray_tracing') == row['ray_tracing'])
        check('independent RT proxy settings ' + name, proxy_matches(mesh, desired_proxy))
        for key, value in desired_metadata.items():
            check('source identity ' + name + ' ' + key, E.get_metadata_tag(mesh, key) == value)
        if desired_screens:
            check('scrub saved screen thresholds ' + name,
                  len(data['screens']) == len(desired_screens) and
                  all(abs(a - b) < 1.e-7 for a, b in zip(data['screens'], desired_screens)), data['screens'])
        data.update({'name': name, 'path': path, 'source_package': row['source_package'],
                     'casts_shadow': row['casts_shadow'], 'ray_tracing': row['ray_tracing'],
                     'rt_proxy': mesh.get_editor_property('ray_tracing_proxy_settings').export_text(),
                     'nanite_raster_settings': current_settings['nanite'],
                     'inspected_render_lod_triangles': reference_lods,
                     'render_lod_triangle_deltas': render_deltas,
                     'render_lod0_kind': 'generated Nanite fallback' if data['nanite_enabled'] else 'ordinary raster LOD',
                     'bounds_delta_cm': bound_deltas,
                     'changes': changes, 'reused_without_mesh_rebuild': not VERIFY and not changes})
        report['meshes'].append(data)
    REG.scan_paths_synchronous([DEST], True)
    graph = dependencies([r['destination'] for r in CONFIG['meshes']])
    report['native_dependency_closure'] = graph
    foreign = sorted({x for deps in graph.values() for x in deps if x.startswith('/Game/') and not x.startswith(DEST + '/')})
    check('final package closure has no vendor world/material dependency', not foreign, foreign)
    # Source/source-staging bytes remain unchanged even if UE rebuilt derived data.
    stage = json.loads((WORK / 'stage-applied.json').read_text())
    for row in stage['packages']:
        for item in row['files']:
            for base in (Path(stage['source_roots'][row['source_pack']]), PROJECT / 'Content'):
                file = base / item['relative_file']
                h = hashlib.sha256()
                with file.open('rb') as stream:
                    for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b''):
                        h.update(chunk)
                check('immutable source ' + str(file), h.hexdigest() == item['sha256'])
    report['status'] = 'PASS'
    flush()
    (WORK / ('model-contract-verified.json' if VERIFY else 'model-contract.json')).write_text(json.dumps({'status': 'NATIVE_REOPEN_VERIFIED' if VERIFY else 'NATIVE_IMPORTED', 'units': 'UE centimetres, Z up; original pivots retained', 'meshes': report['meshes']}, indent=2) + '\n')
    print('S47 ROUTENATURE ' + ('VERIFY' if VERIFY else 'IMPORT') + ' PASS')

try:
    main()
except Exception:
    report['status'] = 'FAIL'
    report['exception'] = traceback.format_exc()
    flush()
    raise
