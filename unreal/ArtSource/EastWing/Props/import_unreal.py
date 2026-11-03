"""Import the delivered EastWing props, without placing actors or changing gameplay.

Run with ordinary Python for source validation. In a leased Unreal editor set
S47_EASTWING_APPLY=1 and S47_EASTWING_REPORT to a private JSON output path.
S47_EASTWING_PRIVATE_ROOT optionally relocates the private RM565 delivery.
The caller owns require-idle/production_lease. No editor is launched here.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import traceback

HERE = Path(__file__).resolve().parent
UNREAL_ROOT = HERE.parents[2]
DEST = '/Game/EastWing/Props'
PRIVATE_DEST = '/Game/Licensed/EastWing/Props'
OWNER = 'S47EastWingProps_v1'


def require(condition, message, detail=None):
    if not condition:
        raise RuntimeError(f'{message}: {detail}')


def read(path):
    return json.loads(path.read_text())


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resolve_map(value, source):
    """Relocate public paths from the authoring worktree before considering originals."""
    path = Path(value)
    marker = '/ArtSource/EastWing/Props/'
    if marker in str(path):
        path = HERE / str(path).split(marker, 1)[1]
    elif '/signal47-private/EastWing/Props/Lab/' in str(path):
        path = source.parent / str(path).split('/signal47-private/EastWing/Props/Lab/', 1)[1]
    elif not path.is_absolute():
        path = source.parent / path
    require(path.is_file(), 'Missing material image', str(path))
    return path


def material_recipe(row, source, surfaces):
    textures = {}
    extra = {}
    key = row.get('texture_key')
    if key:
        require(key in surfaces, 'Unknown surface key', key)
        for role, item in surfaces[key]['maps'].items():
            if role in ('BaseColor', 'Roughness', 'Metallic', 'NormalDX'):
                textures[{'BaseColor': 'base_color', 'Roughness': 'roughness',
                          'Metallic': 'metallic', 'NormalDX': 'normal'}[role]] = str(HERE / 'Textures' / item['file'])
    else:
        for filename, value in row.get('maps', {}).items():
            path = resolve_map(value, source)
            lower = filename.lower()
            if 'basecolor' in lower or 'base_color' in lower:
                textures['base_color'] = str(path)
            elif 'metallicroughness' in lower:
                extra['metallic_roughness'] = str(path)
            elif 'emissive' in lower:
                extra['emissive'] = str(path)
            elif 'normal' in lower:
                textures['normal'] = str(path)
                # RM565 comes from glTF through Blender; its original map is GL.
                extra['normal_gl'] = 'normaldx' not in lower
            elif 'roughness' in lower:
                textures['roughness'] = str(path)
            elif 'metallic' in lower:
                textures['metallic'] = str(path)
            else:
                raise RuntimeError(f'Unmapped image role: {filename}')
    recipe = {
        'name': row['name'], 'base_color_linear': row['Base Color'],
        'roughness': row['Roughness'], 'metallic': row['Metallic'],
        'specular': .5, 'emission': row['Emission Color'],
        'emission_strength': row['Emission Strength'], 'normal_strength': 1.,
        'textures': textures,
        # Shared tile UV density is already baked into the authored mesh UVs.
        'uv_scale': 1.,
    }
    transmission = float(row.get('Transmission Weight', 0))
    if transmission > 0:
        recipe['alpha'] = max(.08, 1. - transmission)
        extra['ior'] = float(row.get('IOR', 1.5))
    return recipe, extra


def make_plan():
    delivery = read(HERE / 'delivery-manifest.json')
    surface_manifest = read(HERE / 'Textures/materials.json')
    surfaces = {row['key']: row for row in surface_manifest['materials']}
    require(len(surfaces) == 8, 'Expected eight surface materials')
    for row in surfaces.values():
        for item in row['maps'].values():
            path = HERE / 'Textures' / item['file']
            require(path.is_file(), 'Missing PBR channel', str(path))
            require(digest(path) == item['sha256'], 'PBR channel changed', str(path))
    private_root = Path(os.environ.get('S47_EASTWING_PRIVATE_ROOT',
                            str(Path.home() / 'Assets/signal47-private/EastWing/Props/Lab'))).resolve()
    models = []
    for item in delivery['assets']:
        private = item.get('public_binary') is False
        source = private_root / (item['id'] + '.asset.json') if private else HERE / item['manifest']
        require(source.is_file(), 'Missing asset manifest', str(source))
        row = read(source)
        require(row['asset'] == item['id'], 'Asset identity mismatch', str(source))
        require(row['front'] == '-Y' and row['up'] == 'Z', 'Unexpected source axes', row['asset'])
        require(len(row['lods']) == 2, 'Expected two authored reduced LODs', row['asset'])
        lods = [{'file': row['fbx'], 'triangles': row['triangles']}] + row['lods']
        for lod in lods:
            require((source.parent / lod['file']).is_file(), 'Missing FBX', lod['file'])
        require(digest(source.parent / row['fbx']) == row['fbx_sha256'], 'LOD0 source changed', row['asset'])
        require(len(row['materials']) == len(set(row['materials'])), 'Duplicate slots', row['asset'])
        require([m['name'] for m in row['material_parameters']] == row['materials'], 'Material manifest order differs', row['asset'])
        if private:
            license_row = read(HERE / item['manifest'])
            require(license_row['public_binary'] is False and license_row['author'] == 'Thusko', 'Private asset attribution')
            require((private_root / 'RM565-LICENSE.txt').is_file(), 'Missing private license evidence')
        else:
            require((HERE / item['license']).is_file(), 'Missing per-asset license', row['asset'])
        group = 'Lab' if private else Path(item['manifest']).parts[0]
        materials = []
        for parameter in row['material_parameters']:
            recipe, extra = material_recipe(parameter, source, surfaces)
            materials.append({'recipe': recipe, 'extra': extra})
        models.append({'row': row, 'source': str(source), 'private': private,
                       'destination': (PRIVATE_DEST if private else DEST) + '/' + group,
                       'lods': lods, 'materials': materials})
    require(len(models) == delivery['asset_count'] == 26, 'Expected 26 models')
    shared = {}
    for model in models:
        for material in model['materials']:
            path = model['destination'] + '/Materials/' + material['recipe']['name']
            signature = json.dumps(material, sort_keys=True)
            require(path not in shared or shared[path] == signature,
                    'Shared material recipes differ before import', path)
            shared[path] = signature
    return {'models': models, 'surfaces': list(surfaces.values())}


def import_plan(plan, report_path):
    import unreal
    require(all(hasattr(unreal, name) for name in ('GeometryScript_AssetUtils',
                'GeometryScript_MeshQueries', 'GeometryScript_List')),
            'GeometryScripting is required for native geometry readback')
    E = unreal.EditorAssetLibrary
    T = unreal.AssetToolsHelpers.get_asset_tools()
    M = unreal.MaterialEditingLibrary
    S = (unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
         or unreal.new_object(unreal.StaticMeshEditorSubsystem))
    recipe_path = UNREAL_ROOT / 'Signal47/Tools/fieldintegration/import_recipe.py'
    spec = importlib.util.spec_from_file_location('s47_eastwing_recipe', recipe_path)
    R = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(R)
    R.check = lambda name, condition, detail=None: require(condition, name, detail)
    report = {'status': 'RUNNING', 'meshes': [], 'surfaces': [],
              'scope': 'Native meshes/materials only; placement, collision choices, power, glass appearance and runtime acceptance remain integration work.'}
    report_path.parent.mkdir(parents=True, exist_ok=True)

    def flush():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    def own(path):
        if E.does_asset_exist(path):
            asset = E.load_asset(path)
            require(E.get_metadata_tag(asset, 'S47Owner') == OWNER,
                    'Refusing to overwrite an asset not owned by this importer', path)

    def mark(asset):
        E.set_metadata_tag(asset, 'S47Owner', OWNER)
        require(E.save_loaded_asset(asset, only_if_is_dirty=False), 'Save failed', asset.get_path_name())

    def sample(mat, path, role, dest, name):
        asset_path = dest + '/Textures/T_' + name + '_' + role
        own(asset_path)
        task = unreal.AssetImportTask()
        task.filename = str(path)
        task.destination_path = dest + '/Textures'
        task.destination_name = 'T_' + name + '_' + role
        task.replace_existing = True
        task.automated = True
        task.save = True
        T.import_asset_tasks([task])
        tex = E.load_asset(asset_path)
        require(isinstance(tex, unreal.Texture2D), 'Texture import failed', path)
        tex.set_editor_property('srgb', role == 'emissive')
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        mark(tex)
        return R.expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                            sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR if role == 'emissive'
                            else unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)

    made = {}

    def material(recipe, extra, dest):
        path = dest + '/' + recipe['name']
        signature = json.dumps([recipe, extra], sort_keys=True)
        if path in made:
            require(made[path][0] == signature, 'Conflicting shared material', path)
            return made[path][1]
        own(path)
        owned_paths = [path]
        for role in recipe['textures']:
            texture_path = dest + '/Textures/T_' + recipe['name'] + '_' + role
            own(texture_path)
            owned_paths.append(texture_path)
        # Keep texture ownership and private/public namespaces independent.
        R.TEXTURE_CACHE.clear()
        try:
            mat = R.material(recipe, dest, HERE)
        finally:
            # These paths passed the ownership guard before the recipe ran.
            # Preserve ownership even if shader compilation interrupts this pass.
            for owned_path in owned_paths:
                if E.does_asset_exist(owned_path):
                    mark(E.load_asset(owned_path))
        for node in M.get_material_expressions(mat):
            if isinstance(node, unreal.MaterialExpressionTextureSample):
                tex = node.get_editor_property('texture')
                if tex:
                    if tex.get_editor_property('compression_settings') == unreal.TextureCompressionSettings.TC_NORMALMAP:
                        tex.set_editor_property('flip_green_channel', bool(extra.get('normal_gl')))
                    mark(tex)
        if extra.get('metallic_roughness'):
            node = sample(mat, extra['metallic_roughness'], 'metallic_roughness', dest, recipe['name'])
            require(M.connect_material_property(node, 'G', unreal.MaterialProperty.MP_ROUGHNESS), 'Packed roughness connection')
            require(M.connect_material_property(node, 'B', unreal.MaterialProperty.MP_METALLIC), 'Packed metallic connection')
        if extra.get('emissive'):
            node = sample(mat, extra['emissive'], 'emissive', dest, recipe['name'])
            factor = next(n for n in M.get_material_expressions(mat)
                          if isinstance(n, unreal.MaterialExpressionVectorParameter)
                          and str(n.get_editor_property('parameter_name')) == 'EmissiveFactor')
            multiply = R.expression(mat, unreal.MaterialExpressionMultiply)
            require(M.connect_material_expressions(node, 'RGB', multiply, 'A'), 'Emissive map connection')
            require(M.connect_material_expressions(factor, 'RGB', multiply, 'B'), 'Emissive factor connection')
            require(M.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR), 'Emissive output connection')
        if extra.get('opacity'):
            node = sample(mat, extra['opacity'], 'opacity', dest, recipe['name'])
            mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
            require(M.connect_material_property(node, 'R', unreal.MaterialProperty.MP_OPACITY_MASK), 'Stripe opacity connection')
        if extra.get('ior'):
            node = R.expression(mat, unreal.MaterialExpressionScalarParameter,
                                parameter_name='IOR', default_value=extra['ior'])
            require(M.connect_material_property(node, '', unreal.MaterialProperty.MP_REFRACTION), 'Glass IOR connection')
        errors = list(M.recompile_material(mat))
        require(not errors, 'Material compile failed', errors)
        mark(mat)
        made[path] = (signature, mat)
        return mat

    flag = 'Interchange.FeatureFlags.Import.FBX'
    old_flag = unreal.SystemLibrary.get_console_variable_int_value(flag)
    try:
        unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
        require(unreal.SystemLibrary.get_console_variable_int_value(flag) == 0, 'Legacy FBX translator required')
        for surface in plan['surfaces']:
            roles = {'BaseColor': 'base_color', 'Roughness': 'roughness', 'Metallic': 'metallic', 'NormalDX': 'normal'}
            recipe = {'name': 'M_' + surface['key'], 'base_color_linear': [1, 1, 1, 1],
                      'roughness': .5, 'metallic': 0., 'specular': .5, 'normal_strength': 1.,
                      'textures': {roles[k]: str(HERE / 'Textures' / v['file'])
                                   for k, v in surface['maps'].items() if k in roles}}
            extra = {'opacity': str(HERE / 'Textures' / surface['maps']['Opacity']['file'])} if 'Opacity' in surface['maps'] else {}
            mat = material(recipe, extra, DEST + '/Surfaces')
            E.set_metadata_tag(mat, 'S47PhysicalTileMetres', json.dumps(surface['physical_tile_metres']))
            E.set_metadata_tag(mat, 'S47HeightDisplacement', 'Disabled; source Height channel retained outside native shader')
            mark(mat)
            report['surfaces'].append(mat.get_path_name())
        for model in plan['models']:
            row = model['row']
            source = Path(model['source'])
            dest = model['destination']
            mats = {item['recipe']['name']: material(item['recipe'], item['extra'], dest + '/Materials')
                    for item in model['materials']}
            own(dest + '/' + row['asset'])
            mesh = R.import_mesh(source.parent / row['fbx'], dest)
            # Mark immediately so an interrupted owned import is safely restartable.
            mark(mesh)
            mesh.set_editor_property('allow_cpu_access', True)
            for lod, item in enumerate(model['lods'][1:], 1):
                require(S.import_lod(mesh, lod, str(source.parent / item['file'])) == lod,
                        'LOD import failed', [row['asset'], lod])
            slots = [str(slot.material_slot_name) for slot in mesh.get_editor_property('static_materials')]
            require(len(slots) == len(row['materials']) and set(slots) == set(row['materials']),
                    'Material slots changed', [row['asset'], slots])
            for index, name in enumerate(slots):
                mesh.set_material(index, mats[name])
            settings = mesh.get_editor_property('nanite_settings')
            settings.enabled = False
            mesh.set_editor_property('nanite_settings', settings)
            require(S.get_lod_count(mesh) == 3, 'Expected three native LODs', row['asset'])
            for lod in range(3):
                build = S.get_lod_build_settings(mesh, lod)
                build.set_editor_property('use_full_precision_u_vs', True)
                build.set_editor_property('remove_degenerates', False)
                S.set_lod_build_settings(mesh, lod, build)
                require(mesh.get_num_triangles(lod) == model['lods'][lod]['triangles'],
                        'LOD triangle count changed', [row['asset'], lod, mesh.get_num_triangles(lod), model['lods'][lod]['triangles']])
                for section in range(mesh.get_num_sections(lod)):
                    index = S.get_lod_material_slot(mesh, lod, section)
                    require(0 <= index < len(slots), 'LOD material section is unbound',
                            [row['asset'], lod, section, index])
            require(S.set_lod_screen_sizes(mesh, [1., .42, .16]), 'LOD screen sizes')
            E.set_metadata_tag(mesh, 'S47CollisionNote', row['collision'])
            E.set_metadata_tag(mesh, 'S47License', row['license']['id'])
            if model['private']:
                E.set_metadata_tag(mesh, 'S47Attribution', 'Tektronix RM565 Oscilloscope by Thusko, CC-BY-4.0')
            mark(mesh)
            lod_report = []
            lo, hi = row['bounds_m']
            expected = [[lo[0]*100, -hi[1]*100, lo[2]*100], [hi[0]*100, -lo[1]*100, hi[2]*100]]
            for lod in range(3):
                options = unreal.GeometryScriptCopyMeshFromAssetOptions()
                requested = unreal.GeometryScriptMeshReadLOD()
                requested.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
                requested.set_editor_property('lod_index', lod)
                dynamic, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, unreal.DynamicMesh(), options, requested)
                require(outcome == unreal.GeometryScriptOutcomePins.SUCCESS, 'Native geometry readback failed')
                _, values, _ = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic, True)
                points = [(p.x, p.y, p.z) for p in unreal.GeometryScript_List.convert_vector_list_to_array(values)]
                require(bool(points), 'Empty native LOD')
                actual = [[fn(p[k] for p in points) for k in range(3)] for fn in (min, max)]
                error = max(abs(a-b) for aa, bb in zip(actual, expected) for a, b in zip(aa, bb))
                require(error < 1.5, 'Native LOD axes, scale or frame changed', [row['asset'], lod, actual, expected])
                lod_report.append({'lod': lod, 'triangles': mesh.get_num_triangles(lod), 'bounds_cm': actual, 'bounds_error_cm': error})
            report['meshes'].append({'asset': mesh.get_path_name(), 'private': model['private'], 'slots': slots, 'lods': lod_report})
            flush()
        report['status'] = 'PASS_NATIVE_IMPORT'
        flush()
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        flush()
        raise
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag + ' ' + str(old_flag))
    return report


def main():
    plan = make_plan()
    if os.environ.get('S47_EASTWING_APPLY') == '1':
        report_path = Path(os.environ.get('S47_EASTWING_REPORT', '/tmp/s47-eastwing-import.json')).resolve()
        require(not report_path.is_relative_to(HERE), 'Native report belongs outside source delivery')
        result = import_plan(plan, report_path)
        print(result['status'], report_path, flush=True)
    else:
        print(json.dumps({'status': 'PASS_SOURCE_PLAN', 'models': len(plan['models']),
                          'fbx_files': sum(len(m['lods']) for m in plan['models']),
                          'surface_materials': len(plan['surfaces']),
                          'public_destination': DEST, 'private_destination': PRIVATE_DEST,
                          'native_import': 'NOT_RUN'}, indent=2))


if __name__ == '__main__':
    main()
