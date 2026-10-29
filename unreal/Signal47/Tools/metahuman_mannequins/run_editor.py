"""Guarded UE Python entry. Job path comes from S47_MANNEQUIN_JOB, never personal Saved.

probe/bake import only into a fresh private run package. verify runs in a second
editor process and only loads the saved output. This script does not launch UE.
"""
import json
import os
import sys
from pathlib import Path

import unreal

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from prepare import clothing_check, file_row, load, preservation, sha, write


def require(test, message):
    if not test:
        raise RuntimeError(message)


def native_file(project, asset):
    require(asset.startswith('/Game/MetaHumans/Mannequins/RadioWoman/') and '..' not in asset,
            'Only bounded private derivative assets may be written')
    return Path(project) / 'Content' / (asset.removeprefix('/Game/') + '.uasset')


def sources_available(cfg):
    result = {}
    for name, path in cfg['sources'].items():
        obj = unreal.load_asset(path)
        require(obj is not None, 'Missing actual local source asset: ' + path)
        result[name] = obj
    require(isinstance(result['body'], unreal.SkeletalMesh) and isinstance(result['face'], unreal.SkeletalMesh)
            and isinstance(result['skeleton'], unreal.Skeleton), 'Actual female mesh/skeleton classes')
    require(result['body'].get_editor_property('skeleton') == result['skeleton'], 'Current native body skeleton identity')
    for name in ('hair_near', 'hair_far', 'brows_near', 'brows_far'):
        require(isinstance(result[name], unreal.StaticMesh), 'Actual authored card mesh: ' + name)
    return result


def create_surface(path, color, roughness, texture=None, coverage_channel=None, parameters=None):
    require(not unreal.EditorAssetLibrary.does_asset_exist(path), 'No material replacement: ' + path)
    folder, name = path.rsplit('/', 1)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    require(mat is not None, 'Create private surface material')
    lib = unreal.MaterialEditingLibrary
    color_type = unreal.MaterialExpressionVectorParameter if parameters else unreal.MaterialExpressionConstant3Vector
    color_node = lib.create_material_expression(mat, color_type)
    if parameters:
        color_node.set_editor_property('parameter_name', parameters['color_parameter'])
    color_node.set_editor_property('default_value' if parameters else 'constant', unreal.LinearColor(*color, 1))
    require(lib.connect_material_property(color_node, '', unreal.MaterialProperty.MP_BASE_COLOR), 'Actual base-color graph binding')
    rough_type = unreal.MaterialExpressionScalarParameter if parameters else unreal.MaterialExpressionConstant
    rough = lib.create_material_expression(mat, rough_type)
    if parameters:
        rough.set_editor_property('parameter_name', parameters['roughness_parameter'])
    rough.set_editor_property('default_value' if parameters else 'r', roughness)
    require(lib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS), 'Actual roughness graph binding')
    if texture is not None:
        mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
        mat.set_editor_property('two_sided', True)
        mat.set_editor_property('opacity_mask_clip_value', .22)
        node = lib.create_material_expression(mat, unreal.MaterialExpressionTextureSample)
        node.set_editor_property('texture', texture)
        require(lib.connect_material_property(node, coverage_channel, unreal.MaterialProperty.MP_OPACITY_MASK), 'Actual coverage graph binding')
    errors = list(lib.recompile_material(mat))
    require(not errors, 'Actual material compile errors: ' + repr(errors))
    require(unreal.EditorAssetLibrary.save_loaded_asset(mat), 'Save only new private material')
    return mat


def cloth_materials(job, cfg):
    """Read actual native parent, graph, overrides, values and skeletal bindings."""
    lib, policy = unreal.MaterialEditingLibrary, cfg['cloth_surface']
    master = unreal.load_asset(job['cloth_master'])
    require(isinstance(master, unreal.Material), 'Actual shared private cloth master')
    color_name, rough_name = policy['color_parameter'], policy['roughness_parameter']
    require(set(map(str, lib.get_vector_parameter_names(master))) == {color_name}
            and set(map(str, lib.get_scalar_parameter_names(master))) == {rough_name},
            'Shared cloth master has exactly RWColor and RWRoughness')
    for prop, cls, name in ((unreal.MaterialProperty.MP_BASE_COLOR, unreal.MaterialExpressionVectorParameter, color_name),
                            (unreal.MaterialProperty.MP_ROUGHNESS, unreal.MaterialExpressionScalarParameter, rough_name)):
        node = lib.get_material_property_input_node(master, prop)
        require(isinstance(node, cls) and str(node.get_editor_property('parameter_name')) == name,
                'Shared cloth parameter is actually wired to ' + str(prop))
    require(master.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE
            and master.get_editor_property('material_domain') == unreal.MaterialDomain.MD_SURFACE
            and not master.get_editor_property('two_sided'), 'Shared opaque one-sided game surface')
    instances = {}
    for name, expected in cfg['materials'].items():
        material = unreal.load_asset(job['asset_root'] + '/' + name)
        require(isinstance(material, unreal.MaterialInstanceConstant)
                and material.get_editor_property('parent') == master, 'Actual shared parent: ' + name)
        color = lib.get_material_instance_vector_parameter_value(material, color_name)
        actual_color = [float(color.r), float(color.g), float(color.b), float(color.a)]
        roughness = float(lib.get_material_instance_scalar_parameter_value(material, rough_name))
        overrides = {key: bool(lib.is_material_instance_parameter_overridden(material, key))
                     for key in (color_name, rough_name)}
        require(all(overrides.values()) and max(abs(a-b) for a, b in zip(actual_color, [*expected['color'], 1])) < 1e-6
                and abs(roughness-expected['roughness']) < 1e-6, 'Actual cloth instance parameter values: ' + name)
        instances[name] = {'asset': material.get_path_name(), 'parent': master.get_path_name(),
                           color_name: actual_color, rough_name: roughness, 'overrides': overrides}
    cloth = unreal.load_asset(job['clothes_asset'])
    require(isinstance(cloth, unreal.SkeletalMesh), 'Actual imported private clothes')
    bindings = {str(slot.get_editor_property('material_slot_name')):
                slot.get_editor_property('material_interface').get_path_name()
                for slot in cloth.get_editor_property('materials')}
    require(bindings == {name: row['asset'] for name, row in instances.items()}, 'Actual five named MI bindings on clothes')
    return {'master': master.get_path_name(), 'color_parameter': color_name, 'roughness_parameter': rough_name,
            'instances': instances, 'clothing_bindings': bindings}


def brow_coverage(sources):
    # Exact installed HairCardsAttributeCommon.ush: layout0/1/2 use texture1.R;
    # layout3 uses texture2.A. Read the actual groom descriptors, do not guess an atlas.
    layouts = {unreal.HairTextureLayout.LAYOUT0: (1, 'R'), unreal.HairTextureLayout.LAYOUT1: (1, 'R'),
               unreal.HairTextureLayout.LAYOUT2: (1, 'R'), unreal.HairTextureLayout.LAYOUT3: (2, 'A')}
    found = {}
    for card in sources['brows_groom'].get_editor_property('hair_groups_cards'):
        mesh = card.get_editor_property('imported_mesh')
        if mesh not in (sources['brows_near'], sources['brows_far']):
            continue
        textures = card.get_editor_property('textures')
        layout = textures.get_editor_property('layout')
        require(layout in layouts, 'Known local engine card coverage layout')
        index, channel = layouts[layout]
        entries = textures.get_editor_property('textures')
        require(len(entries) > index and isinstance(entries[index], unreal.Texture2D), 'Actual authored brow coverage texture')
        found[mesh.get_path_name()] = (entries[index], channel, str(layout))
    require(set(found) == {sources['brows_near'].get_path_name(), sources['brows_far'].get_path_name()},
            'Both actual brow card sources have groom coverage provenance')
    near = found[sources['brows_near'].get_path_name()]
    far = found[sources['brows_far'].get_path_name()]
    require(near[:2] == far[:2], 'Near/far brow cards share the measured coverage atlas/channel')
    return near, {name: {'texture': value[0].get_path_name(), 'channel': value[1], 'layout': value[2]}
                  for name, value in found.items()}


def import_private(job, cfg, sources):
    receipt_path = Path(job['python_report']).parent / 'import-python.json'
    fbx, _ = clothing_check(job['clothing_report'])
    require(str(fbx) == job['clothing_fbx'] and sha(fbx) == job['clothing_fbx_sha256'], 'Pinned actual rest-clothes FBX')
    if receipt_path.exists():
        previous = load(receipt_path)
        require(previous['status'] == 'PASS' and previous['clothing_fbx_sha256'] == job['clothing_fbx_sha256'],
                'Existing private import must have its exact successful receipt')
        for row in previous['saved_assets']:
            require(file_row(native_file(job['project'], row['asset'])) == row['file'], 'Imported private asset changed since probe')
        require(previous['cloth_materials'] == cloth_materials(job, cfg), 'Actual shared materials unchanged since import')
        return previous
    targets = [job['private_skeleton'], job['clothes_asset'], job['brow_material'], job['cloth_master']]
    targets += [job['asset_root'] + '/' + name for name in cfg['materials']]
    require(len(targets) == len(set(targets)) == 9, 'Exactly nine private import assets')
    require(not any(unreal.EditorAssetLibrary.does_asset_exist(path) or native_file(job['project'], path).exists()
                    for path in targets), 'Fresh private import only; partial previous attempts need a new run')
    coverage, provenance = brow_coverage(sources)
    unreal.EditorAssetLibrary.make_directory(job['asset_root'])
    skeleton = unreal.EditorAssetLibrary.duplicate_asset(cfg['sources']['skeleton'], job['private_skeleton'])
    require(isinstance(skeleton, unreal.Skeleton) and skeleton != sources['skeleton'], 'Private exact skeleton duplicate')
    require(unreal.EditorAssetLibrary.save_loaded_asset(skeleton), 'Save only private skeleton copy')
    task = unreal.AssetImportTask()
    task.filename = str(fbx)
    task.destination_path, task.destination_name = job['clothes_asset'].rsplit('/', 1)
    task.automated, task.replace_existing, task.save = True, False, True
    options = unreal.FbxImportUI()
    options.import_as_skeletal = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.original_import_type = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.skeleton = skeleton
    options.import_materials, options.import_textures, options.import_animations = False, False, False
    options.create_physics_asset = False
    data = options.skeletal_mesh_import_data
    data.set_editor_property('import_content_type', unreal.FBXImportContentType.FBXICT_ALL)
    data.set_editor_property('update_skeleton_reference_pose', False)
    data.set_editor_property('use_t0_as_ref_pose', False)
    data.set_editor_property('preserve_smoothing_groups', True)
    data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    data.set_editor_property('import_uniform_scale', 1.0)
    data.set_editor_property('import_translation', unreal.Vector(0, 0, 0))
    data.set_editor_property('import_rotation', unreal.Rotator(0, 0, 0))
    data.set_editor_property('convert_scene', True)
    data.set_editor_property('convert_scene_unit', True)
    data.set_editor_property('force_front_x_axis', False)
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    cloth = unreal.load_asset(job['clothes_asset'])
    require(isinstance(cloth, unreal.SkeletalMesh) and cloth.get_editor_property('skeleton') == skeleton,
            'Clothes imported against the private native skeleton')
    defaults = cfg['materials']['M_RW_Coat']
    master = create_surface(job['cloth_master'], defaults['color'], defaults['roughness'], parameters=cfg['cloth_surface'])
    materials = {}
    lib, policy = unreal.MaterialEditingLibrary, cfg['cloth_surface']
    for name, item in cfg['materials'].items():
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, job['asset_root'], unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        require(isinstance(material, unreal.MaterialInstanceConstant), 'Create private cloth instance: ' + name)
        lib.set_material_instance_parent(material, master)
        # The installed UE5.8 setters return false even after setting/updating.
        # Accept only actual values/overrides/parent read back by cloth_materials.
        lib.set_material_instance_vector_parameter_value(material, policy['color_parameter'], unreal.LinearColor(*item['color'], 1))
        lib.set_material_instance_scalar_parameter_value(material, policy['roughness_parameter'], item['roughness'])
        require(unreal.EditorAssetLibrary.save_loaded_asset(material), 'Save private cloth instance: ' + name)
        materials[name] = material
    slots = list(cloth.get_editor_property('materials'))
    actual_names = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
    require(set(actual_names) == set(materials) and len(actual_names) == len(materials), 'Exact authored clothing material slots')
    for slot in slots:
        slot.set_editor_property('material_interface', materials[str(slot.get_editor_property('material_slot_name'))])
    cloth.set_editor_property('materials', slots)
    require(unreal.EditorAssetLibrary.save_loaded_asset(cloth), 'Save only new private cloth mesh/material bindings')
    create_surface(job['brow_material'], [.022, .012, .008], .52, coverage[0], coverage[1])
    # The import may update only its private skeleton. Persist and fingerprint that copy too.
    require(unreal.EditorAssetLibrary.save_loaded_asset(skeleton), 'Persist private import skeleton only')
    saved = [{'asset': path, 'file': file_row(native_file(job['project'], path))} for path in targets]
    result = {'status': 'PASS', 'clothing_fbx_sha256': sha(fbx), 'private_skeleton': skeleton.get_path_name(),
              'actual_clothing_slots': actual_names, 'saved_assets': saved, 'brow_coverage': provenance,
              'cloth_materials': cloth_materials(job, cfg),
              'limits': 'Native pose, surface rendering and runtime adoption not proved by import'}
    write(receipt_path, result)
    return result


def main():
    job_path = Path(os.environ['S47_MANNEQUIN_JOB']).resolve(strict=True)
    job, report = load(job_path), {'status': 'STARTED_NOT_ACCEPTED', 'pid': os.getpid(),
                                 'engine_version': unreal.SystemLibrary.get_engine_version()}
    try:
        require(Path(job['project']).resolve() == Path(unreal.Paths.project_dir()).resolve(), 'Explicit authoring project matches editor')
        require(sha(job['contract']) == job['contract_sha256'], 'Frozen authoring contract')
        require(sha(job['clothing_report']) == job['clothing_report_sha256'], 'Byte-identical actual private clothing receipt')
        cfg = load(job['contract'])
        require(job['mode'] in ('probe', 'bake', 'verify'), 'Known native authoring operation')
        null_rhi = '-nullrhi' in unreal.SystemLibrary.get_command_line().lower().split()
        require(job['mode'] == 'probe' or not null_rhi, 'Bake and saved verification require actual RHI, never NullRHI')
        report['render_context'] = {'null_rhi': null_rhi,
                                    'probe_is_pose_preflight_only': job['mode'] == 'probe'}
        require(not Path(job['native_report']).exists() and not Path(job['python_report']).exists(), 'Fresh operation receipts')
        report['preservation_before'] = preservation(job)
        sources = sources_available(cfg)
        if job['mode'] in ('probe', 'bake'):
            report['private_import'] = import_private(job, cfg, sources)
        else:
            require(unreal.EditorAssetLibrary.does_asset_exist(job['output_asset']), 'Actual saved output exists for fresh-process verify')
            baked = load(Path(job['python_report']).parent / 'bake-python.json')
            require(baked['status'] == 'PASS' and baked['pid'] != os.getpid(), 'Verifier is a fresh editor process after accepted bake')
            require(file_row(native_file(job['project'], job['output_asset'])) ==
                    {key: baked['output_asset'][key] for key in ('bytes', 'sha256')}, 'Unchanged actual saved bake payload before reload')
        report['cloth_materials'] = cloth_materials(job, cfg)
        imported = load(Path(job['python_report']).parent / 'import-python.json')
        require(report['cloth_materials'] == imported['cloth_materials'], 'Actual saved shared material policy retained')
        if job['mode'] == 'bake':
            # A NullRHI probe updates/persists the graph, not actual shader proof.
            # Re-evaluate both private masters in the real bake; native Audit then
            # requires complete actual shader maps and zero errors for every slot.
            report['private_master_compile_errors'] = {}
            for asset in (job['cloth_master'], job['brow_material']):
                errors = list(unreal.MaterialEditingLibrary.recompile_material(unreal.load_asset(asset)))
                report['private_master_compile_errors'][asset] = errors
                require(not errors, 'Actual private master compile errors: ' + repr(errors))
        unreal.SystemLibrary.execute_console_command(None, 'S47.MannequinBake')
        require(Path(job['native_report']).is_file(), 'Editor-only bake command is compiled and actually wrote a receipt')
        native = load(job['native_report'])
        require(native.get('status') == 'PASS' and native.get('mode') == job['mode'], 'Native authoring acceptance: ' + native.get('error', 'missing receipt'))
        report['native_report'] = {'path': job['native_report'], **file_row(job['native_report'])}
        report['preservation_after'] = preservation(job)
        if job['mode'] != 'probe':
            report['output_asset'] = {'path': job['output_asset'], **file_row(native_file(job['project'], job['output_asset']))}
        if job['mode'] == 'verify':
            require(report['output_asset'] == baked['output_asset'], 'Read-only fresh-process verifier did not rewrite the baked asset')
        report.update(status='PASS', mode=job['mode'], job_sha256=sha(job_path),
                      limits='Private source/import/native bake only. No runtime integration, visual acceptance, FPS, installation or release.')
        write(job['python_report'], report)
        unreal.log('S47 MANNEQUIN ' + job['mode'].upper() + ' PASS')
    except Exception as error:
        report.update(status='FAIL', error=str(error))
        try:
            report['preservation_after'] = preservation(job)
        except Exception as changed:
            report['preservation_error'] = str(changed)
        write(job['python_report'], report)
        unreal.log_error('S47 MANNEQUIN ' + job['mode'].upper() + ' FAIL: ' + str(error))
        raise


main()
