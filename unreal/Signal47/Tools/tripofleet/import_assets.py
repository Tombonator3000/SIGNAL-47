"""Prepare a plan, or explicitly import private Tripo fleet exports in UE 5.8.

Outside Unreal: python3 import_assets.py --manifest /private/prepared/manifest.json
Editor: set S47_TRIPOFLEET_MANIFEST and S47_TRIPOFLEET_APPLY=1, execute this file
with GeometryScripting enabled. Do not run while Tom plays. Nothing runs on import.
This creates candidate assets only; no map, runtime binding or project setting changes.
"""
import argparse
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import sys
import traceback


DEST = '/Game/Licensed/TripoFleet'
OWNER = 'S47TripoFleet_v1'
HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]


def require(condition, message, detail=None):
    if not condition:
        raise RuntimeError(message + (': ' + str(detail) if detail is not None else ''))


def safe_name(value):
    require(isinstance(value, str) and re.fullmatch(r'[A-Za-z][A-Za-z0-9_]*', value),
            'Expected Unreal-safe name', value)
    return value


def source_file(root, value):
    path = (root / value).resolve()
    require(path.is_relative_to(root), 'Prepared source escapes its private directory', path)
    require(path.is_file(), 'Prepared file missing', path)
    return path


def vector(value):
    require(isinstance(value, list) and len(value) == 3 and
            all(isinstance(v, (float, int)) and math.isfinite(v) for v in value),
            'Expected three finite coordinates', value)
    return value


def bounds(value):
    require(isinstance(value, list) and len(value) == 2, 'Expected min/max bounds', value)
    low, high = [vector(v) for v in value]
    require(all(a < b for a, b in zip(low, high)), 'Empty or inverted model bounds', value)
    return value


def make_plan(filename, selected=()):
    """Validate all prepared input before any Unreal import or asset mutation."""
    path = Path(filename).expanduser().resolve()
    require(not path.is_relative_to(REPO), 'Fleet manifest must stay outside public checkout', path)
    data = json.loads(path.read_text())
    require(data.get('version') == 1, 'Unsupported fleet manifest version')
    frame = data['coordinates']
    require(frame.get('blender') == 'metres; X width, Y length, Z up; front +Y',
            'Prepared nose must match existing native truck controller', frame)
    require(frame['native_mapping'] == 'UEcm=(Bx,-By,Bz)*100' and
            frame['fbx_forward'] == '-Y' and frame['fbx_up'] == 'Z' and
            frame['native_import_yaw_deg'] == 90, 'Unmeasured FBX coordinate frame', frame)
    models = []
    for entry in data['models']:
        ident = safe_name(entry['id'])
        if selected and ident not in selected:
            continue
        require(entry.get('status') == 'PREPARED_NATIVE_UNVERIFIED',
                'Model has not completed Blender preparation', ident)
        root = Path(entry['output_dir']).expanduser().resolve()
        require(root.is_relative_to(path.parent), 'Model output escapes prepared root', root)
        contract_path = source_file(path.parent, entry['contract'])
        materials_path = source_file(path.parent, entry['materials'])
        require(contract_path.parent == root and materials_path.parent == root,
                'Model sidecars disagree with output_dir', ident)
        contract = json.loads(contract_path.read_text())
        rows = json.loads(materials_path.read_text())
        require(isinstance(rows, list) and rows, 'No original materials', ident)
        names = [safe_name(row['name']) for row in rows]
        require(len(set(names)) == len(names), 'Duplicate material names', ident)
        for row in rows:
            require(not row.get('unsupported'), 'Unsupported original PBR semantics', row)
            require(row.get('normal_format', 'OpenGL') in ('OpenGL', 'DirectX'),
                    'Unsupported normal texture convention', row['name'])
            require(row.get('alpha_mode', 'OPAQUE').upper() in ('OPAQUE', 'MASK', 'MASKED', 'BLEND', 'TRANSLUCENT'),
                    'Unsupported source alpha mode', row['name'])
            for role, name in row.get('textures', {}).items():
                require(role in ('base_color', 'normal', 'roughness', 'metallic', 'emission'),
                        'Unsupported texture role', role)
                source_file(root, name)
            for role in ('packed_metallic_roughness', 'occlusion'):
                if row.get(role):
                    source_file(root, row[role])
        parts = []
        for name, part in contract['parts'].items():
            safe_name(name)
            require(part['asset_folder'] == DEST + '/' + ident, 'Unexpected private destination', part['asset_folder'])
            require(len(part['materials']) == len(set(part['materials'])) and
                    set(part['materials']).issubset(names), 'Missing or duplicate original slots', name)
            bounds(part['bounds_local_m'])
            require([row['lod'] for row in part['lods']] == [0, 1, 2], 'Need three authored LODs', name)
            for lod in part['lods']:
                bounds(lod['bounds_local_m'])
                fbx = source_file(root, lod['fbx'])
                expected_stem = name + ('_LOD' + str(lod['lod']) if lod['lod'] else '')
                require(fbx.stem == expected_stem, 'FBX filename differs from part/LOD', fbx)
                require(lod['triangles'] > 0, 'Empty LOD', name)
                require(len(lod['geometry_probes_m']) >= 6, 'Need asymmetric actual-LOD surface probes', name)
                for point in lod['geometry_probes_m']:
                    vector(point)
                require(lod['materials'] == part['materials'], 'LOD slot frame differs from original', name)
            require(part['lods'][0]['triangles'] == part['triangles'], 'LOD0 triangle contract differs', name)
            parts.append(dict(part, name=name))
        require(parts, 'No prepared static mesh parts', ident)
        models.append({'id': ident, 'root': str(root), 'materials': rows, 'parts': parts})
    require(models, 'No selected prepared models')
    require(len({m['id'] for m in models}) == len(models), 'Duplicate model identifiers')
    require(not selected or set(selected) == {m['id'] for m in models}, 'Unknown selected model')
    return {'version': 1, 'manifest': str(path), 'destination': DEST,
            'native_mapping': frame['native_mapping'], 'models': models,
            'runtime_replacement': False}


def import_candidates(plan, report_path):
    import unreal
    # Reuse the measured legacy recipe, not an Interchange import with different LOD axes.
    recipe = HERE.parent / 'fieldintegration/import_recipe.py'
    spec = importlib.util.spec_from_file_location('s47_tripofleet_recipe', recipe)
    R = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(R)
    report_path = Path(report_path).resolve()
    require(not report_path.is_relative_to(REPO), 'Native report must stay outside public checkout', report_path)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report = {'status': 'RUNNING', 'engine': unreal.SystemLibrary.get_engine_version(),
              'manifest': plan['manifest'], 'checks': [], 'meshes': [], 'runtime_replacement': False}

    def flush():
        report_path.write_text(json.dumps(report, indent=2) + '\n')

    def check(name, condition, detail=None):
        report['checks'].append({'name': name, 'pass': bool(condition), 'detail': detail})
        require(condition, name, detail)

    R.check = check
    E, M, S, T = R.E, R.M, R.S, R.T

    def own(asset_path, cls=None):
        obj = E.load_asset(asset_path) if E.does_asset_exist(asset_path) else None
        if obj:
            check('Replace only this importer\'s candidate', E.get_metadata_tag(obj, 'S47Owner') == OWNER, asset_path)
            if cls:
                check('Owned candidate type', isinstance(obj, cls), asset_path)
        return obj

    def mark(obj):
        E.set_metadata_tag(obj, 'S47Owner', OWNER)
        # Persist ownership before later material/LOD validation can fail, so a
        # retry cannot strand our freshly imported asset as an unowned candidate.
        check('Candidate ownership saved', E.save_loaded_asset(obj, only_if_is_dirty=False), obj.get_path_name())

    def connect(node, output, prop):
        check('PBR property connected', M.connect_material_property(node, output, prop), str(prop))

    def texture(root, filename, folder, role):
        path = source_file(root, filename)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()[:16]
        name = 'T_' + role + '_' + digest
        asset_path = folder + '/Textures/' + name
        tex = own(asset_path, unreal.Texture2D)
        if not tex:
            task = unreal.AssetImportTask()
            task.filename, task.destination_path, task.destination_name = str(path), folder + '/Textures', name
            task.automated, task.save = True, False
            T.import_asset_tasks([task])
            tex = E.load_asset(asset_path)
            check('Original texture imported', isinstance(tex, unreal.Texture2D), asset_path)
            mark(tex)
        tex.set_editor_property('srgb', role == 'emission')
        tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
        check('Original texture saved', E.save_loaded_asset(tex, only_if_is_dirty=False), asset_path)
        return tex

    def multiply(mat, node, output, value):
        scalar = R.expression(mat, unreal.MaterialExpressionConstant, r=float(value))
        result = R.expression(mat, unreal.MaterialExpressionMultiply)
        check('PBR sample connected', M.connect_material_expressions(node, output, result, 'A'))
        check('Original PBR factor connected', M.connect_material_expressions(scalar, '', result, 'B'))
        return result

    def material(original, folder, root):
        row = dict(original)
        mode = row.get('alpha_mode', 'OPAQUE').upper()
        # The common recipe accepts its own lowercase alpha vocabulary.
        row['alpha_mode'] = 'masked' if mode in ('MASK', 'MASKED') else 'blend'
        color = row.get('base_color_linear', [1, 1, 1, 1])
        row['texture_tint_linear'] = color[:3]
        row['normal_strength'] = row.get('normal_strength', 1.)
        row['alpha'] = 1.  # Apply original factor together with alpha texture below.
        row['opacity_from_base_color'] = mode != 'OPAQUE' and bool(row.get('textures', {}).get('base_color'))
        asset_path = folder + '/' + row['name']
        mat = own(asset_path, unreal.Material)
        if not mat:
            mat = T.create_asset(row['name'], folder, unreal.Material, unreal.MaterialFactoryNew())
            check('Candidate material created', mat is not None, asset_path)
            mark(mat)
        # The recipe owns these deterministic texture names. Guard existing assets first.
        for role in ('base_color', 'normal', 'roughness', 'metallic'):
            if row.get('textures', {}).get(role):
                own(folder + '/Textures/T_' + row['name'] + '_' + role, unreal.Texture2D)
        mat = R.material(row, folder, root)
        for node in M.get_material_expressions(mat):
            if isinstance(node, unreal.MaterialExpressionTextureSample):
                tex = node.get_editor_property('texture')
                if tex:
                    mark(tex)
                    if node.get_editor_property('sampler_type') == unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL:
                        tex.set_editor_property('flip_green_channel', row.get('normal_format', 'OpenGL') == 'OpenGL')
                        # glTF normalTexture.scale multiplies decoded X/Y, retaining Z.
                        # The common recipe's neutral-normal lerp has different semantics.
                        strength = float(row.get('normal_strength', 1))
                        scale = R.expression(mat, unreal.MaterialExpressionConstant3Vector,
                                             constant=unreal.LinearColor(strength, strength, 1))
                        scaled = R.expression(mat, unreal.MaterialExpressionMultiply)
                        normalized = R.expression(mat, unreal.MaterialExpressionNormalize)
                        check('Original normal XY scale', M.connect_material_expressions(node, 'RGB', scaled, 'A'))
                        check('Original normal Z retained', M.connect_material_expressions(scale, '', scaled, 'B'))
                        check('Original scaled normal normalized', M.connect_material_expressions(scaled, '', normalized, 'VectorInput'))
                        connect(normalized, '', unreal.MaterialProperty.MP_NORMAL)
                    check('Recipe texture saved', E.save_loaded_asset(tex, only_if_is_dirty=False))
        if row.get('packed_metallic_roughness'):
            tex = texture(root, row['packed_metallic_roughness'], folder, 'ORM')
            sample = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            for channel, factor, prop in [('G', row.get('roughness', 1), unreal.MaterialProperty.MP_ROUGHNESS),
                                           ('B', row.get('metallic', 1), unreal.MaterialProperty.MP_METALLIC)]:
                connect(multiply(mat, sample, channel, factor), '', prop)
        else:
            for role, prop in [('roughness', unreal.MaterialProperty.MP_ROUGHNESS), ('metallic', unreal.MaterialProperty.MP_METALLIC)]:
                if row.get('textures', {}).get(role):
                    connect(multiply(mat, M.get_material_property_input_node(mat, prop), 'R', row.get(role, 1)), '', prop)
        if row.get('occlusion'):
            tex = texture(root, row['occlusion'], folder, 'Occlusion')
            sample = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
            # glTF AO is mix(1, sampledR, strength), not sampledR * strength.
            strength = R.expression(mat, unreal.MaterialExpressionConstant, r=float(row.get('occlusion_strength', 1)))
            white = R.expression(mat, unreal.MaterialExpressionConstant, r=1.)
            mix = R.expression(mat, unreal.MaterialExpressionLinearInterpolate)
            for node, output, pin in [(white, '', 'A'), (sample, 'R', 'B'), (strength, '', 'Alpha')]:
                check('Original AO connected', M.connect_material_expressions(node, output, mix, pin))
            connect(mix, '', unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
        emission = next(n for n in M.get_material_expressions(mat)
                        if isinstance(n, unreal.MaterialExpressionVectorParameter) and
                        str(n.get_editor_property('parameter_name')) == 'EmissiveFactor')
        # Base colour does not modulate glTF emissiveFactor; override that recipe convention.
        connect(emission, 'RGB', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if row.get('textures', {}).get('emission'):
            tex = texture(root, row['textures']['emission'], folder, 'emission')
            sample = R.expression(mat, unreal.MaterialExpressionTextureSample, texture=tex,
                                  sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
            mul = R.expression(mat, unreal.MaterialExpressionMultiply)
            check('Original emissive texture connected', M.connect_material_expressions(sample, 'RGB', mul, 'A'))
            check('Original emissive factor connected', M.connect_material_expressions(emission, 'RGB', mul, 'B'))
            connect(mul, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if row.get('vehicle_lamps'):
            # Source geometry masks are authored only over inspected head/tail lenses.
            # Keep the original emissive texture/factor branch and add switched lamps.
            existing = M.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            existing_output = M.get_material_property_input_node_output_name(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            colors = R.expression(mat, unreal.MaterialExpressionVertexColor)
            combined = existing
            output = existing_output
            for channel, parameter in [('R', 'S47Headlamp'), ('G', 'S47Taillamp')]:
                factor = R.expression(mat, unreal.MaterialExpressionVectorParameter,
                                      parameter_name=parameter, default_value=unreal.LinearColor(0, 0, 0, 1))
                glow = R.expression(mat, unreal.MaterialExpressionMultiply)
                check('Inspected lamp mask connected', M.connect_material_expressions(colors, channel, glow, 'A'))
                check('Switched lamp colour connected', M.connect_material_expressions(factor, 'RGB', glow, 'B'))
                add = R.expression(mat, unreal.MaterialExpressionAdd)
                check('Original emissive branch retained', M.connect_material_expressions(combined, output, add, 'A'))
                check('Switched source lens added', M.connect_material_expressions(glow, '', add, 'B'))
                combined, output = add, ''
            connect(combined, output, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if mode != 'OPAQUE':
            masked = mode in ('MASK', 'MASKED')
            mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_TRANSLUCENT)
            prop = unreal.MaterialProperty.MP_OPACITY_MASK if masked else unreal.MaterialProperty.MP_OPACITY
            alpha = M.get_material_property_input_node(mat, prop)
            factor = float(color[3] if len(color) > 3 else original.get('alpha', 1.))
            if alpha:
                output = 'A' if isinstance(alpha, unreal.MaterialExpressionTextureSample) else ''
                connect(multiply(mat, alpha, output, factor), '', prop)
            else:
                connect(R.expression(mat, unreal.MaterialExpressionConstant, r=factor), '', prop)
            if masked:
                mat.set_editor_property('opacity_mask_clip_value', float(row.get('alpha_cutoff', .5)))
            else:
                mat.set_editor_property('translucency_lighting_mode', unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        check('Native original PBR compiles', not list(M.recompile_material(mat)), row['name'])
        check('Native material saved', E.save_loaded_asset(mat, only_if_is_dirty=False), asset_path)
        return mat

    def verify_lod(mesh, row):
        options = unreal.GeometryScriptCopyMeshFromAssetOptions()
        options.set_editor_property('apply_build_settings', False)
        options.set_editor_property('use_build_scale', False)
        options.set_editor_property('request_tangents', False)
        requested = unreal.GeometryScriptMeshReadLOD()
        requested.set_editor_property('lod_type', unreal.GeometryScriptLODType.RENDER_DATA)
        requested.set_editor_property('lod_index', row['lod'])
        target = unreal.DynamicMesh()
        dynamic, outcome = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(mesh, target, options, requested)
        check('Actual render LOD copied', dynamic == target and outcome == unreal.GeometryScriptOutcomePins.SUCCESS, row['lod'])
        _, values, _ = unreal.GeometryScript_MeshQueries.get_all_vertex_positions(dynamic, True)
        positions = [(p.x, p.y, p.z) for p in unreal.GeometryScript_List.convert_vector_list_to_array(values)]
        check('Actual render LOD finite vertices', positions and all(math.isfinite(v) for p in positions for v in p))
        actual = [[fn(p[axis] for p in positions) for axis in range(3)] for fn in (min, max)]
        low, high = row['bounds_local_m']
        expected = [[low[0]*100, -high[1]*100, low[2]*100], [high[0]*100, -low[1]*100, high[2]*100]]
        error = max(abs(a-b) for aa, bb in zip(actual, expected) for a, b in zip(aa, bb))
        check('Actual render LOD dimensions and frame', error < .15, {'lod': row['lod'], 'actual_cm': actual, 'expected_cm': expected})
        errors = []
        for x, y, z in row['geometry_probes_m']:
            point = [x*100, -y*100, z*100]
            errors.append(math.sqrt(min(sum((p[k]-point[k])**2 for k in range(3)) for p in positions)))
        check('Actual asymmetric surface probes', max(errors) < .15, {'lod': row['lod'], 'errors_cm': errors})
        check('Actual render triangle count', dynamic.get_triangle_count() == row['triangles'], row['lod'])
        return {'lod': row['lod'], 'bounds_cm': actual, 'maximum_bounds_error_cm': error,
                'maximum_probe_error_cm': max(errors), 'triangles': dynamic.get_triangle_count()}

    flag = 'Interchange.FeatureFlags.Import.FBX'
    old_flag = unreal.SystemLibrary.get_console_variable_int_value(flag)
    try:
        check('GeometryScripting available for actual render validation', all(hasattr(unreal, n) for n in
              ('GeometryScript_AssetUtils', 'GeometryScript_MeshQueries', 'GeometryScript_List')),
              'Enable GeometryScripting for this editor job; no project plugin mutation is performed')
        unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
        check('Legacy FBX translator for every LOD', unreal.SystemLibrary.get_console_variable_int_value(flag) == 0)
        for model in plan['models']:
            root = Path(model['root'])
            folder = DEST + '/' + model['id']
            mats = {row['name']: material(row, folder + '/Materials', root) for row in model['materials']}
            for part in model['parts']:
                asset_path = folder + '/' + part['name']
                own(asset_path, unreal.StaticMesh)
                base = source_file(root, part['lods'][0]['fbx'])
                check('FBX stem matches candidate name', base.stem == part['name'], base.name)
                mesh = R.import_mesh(base, folder)
                mark(mesh)
                mesh.set_editor_property('allow_cpu_access', True)
                for lod in part['lods'][1:]:
                    check('Legacy translator remains selected', unreal.SystemLibrary.get_console_variable_int_value(flag) == 0)
                    result = S.import_lod(mesh, lod['lod'], str(source_file(root, lod['fbx'])))
                    check('Authored consistent LOD imported', result == lod['lod'], part['name'])
                slots = [str(slot.material_slot_name) for slot in mesh.get_editor_property('static_materials')]
                check('Every original material slot retained', len(slots) == len(part['materials']) and set(slots) == set(part['materials']), slots)
                for slot, name in enumerate(slots):
                    mesh.set_material(slot, mats[name])
                settings = mesh.get_editor_property('nanite_settings')
                settings.enabled = False
                mesh.set_editor_property('nanite_settings', settings)
                check('Three actual authored LODs', S.get_lod_count(mesh) == 3)
                for lod in range(3):
                    build = S.get_lod_build_settings(mesh, lod)
                    build.set_editor_property('use_full_precision_u_vs', True)
                    build.set_editor_property('remove_degenerates', False)
                    S.set_lod_build_settings(mesh, lod, build)
                    used = []
                    for section in range(mesh.get_num_sections(lod)):
                        index = S.get_lod_material_slot(mesh, lod, section)
                        check('Native LOD section retains assigned original PBR', 0 <= index < len(slots), [lod, section, index])
                        used.append(slots[index])
                    if 'used_materials' in part['lods'][lod]:
                        check('Native LOD used material set', set(used) == set(part['lods'][lod]['used_materials']), used)
                # UE5.8 SetLodScreenSizes disables automatic sizing itself; the
                # former private UStaticMesh bit is not an exposed Python property.
                check('Authored LOD screen transitions', S.set_lod_screen_sizes(mesh, [1., .42, .16]))
                check('Candidate static mesh saved', E.save_loaded_asset(mesh, only_if_is_dirty=False), asset_path)
                readback = [verify_lod(mesh, row) for row in part['lods']]
                report['meshes'].append({'name': part['name'], 'asset': mesh.get_path_name(), 'materials': slots, 'lods': readback})
                flush()
        report['status'] = 'PASS'
        flush()
        unreal.log('S47 TRIPO FLEET CANDIDATE IMPORT PASS ' + str(report_path))
    except Exception:
        report['status'] = 'FAIL'
        report['error'] = traceback.format_exc()
        flush()
        raise
    finally:
        unreal.SystemLibrary.execute_console_command(None, flag + ' ' + str(old_flag))
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', default=os.environ.get('S47_TRIPOFLEET_MANIFEST'))
    parser.add_argument('--model', action='append', default=[])
    parser.add_argument('--apply', action='store_true', default=os.environ.get('S47_TRIPOFLEET_APPLY') == '1')
    parser.add_argument('--report', default=os.environ.get('S47_TRIPOFLEET_REPORT'))
    args = parser.parse_args(argv)
    require(args.manifest, 'Specify --manifest or S47_TRIPOFLEET_MANIFEST')
    plan = make_plan(args.manifest, args.model)
    if args.apply:
        return import_candidates(plan, args.report or str(Path(args.manifest).resolve().parent / 'native-import.json'))
    print(json.dumps(plan, indent=2))
    return plan


if __name__ == '__main__':
    # Unreal's script runner can pass editor flags through sys.argv. Its explicit env
    # contract takes precedence; CLI execution outside the editor uses normal args.
    main([] if 'unreal' in sys.modules and os.environ.get('S47_TRIPOFLEET_MANIFEST') else None)
