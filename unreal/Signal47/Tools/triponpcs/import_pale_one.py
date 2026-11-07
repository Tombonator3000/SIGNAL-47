"""Import a body from prepare_pale_one.py into the Licensed folder its manifest names: skin, three LODs, material, clips.

Run in UnrealEditor-Cmd through run_pale_one.py, against the project that owns Content/Licensed. Nothing here enters Git.
Reads back what it made: height, floor, each LOD's actual render data, and each clip's bones against Blender's.
"""
import json
import math
import os
import sys
from pathlib import Path

import unreal

ROOT = Path(os.environ['S47_PALE_ONE_PREPARED']).resolve()
os.environ['S47_ROSWELL_DIR'] = str(ROOT)  # its helpers read the same layout of folder and manifest
sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_roswell as IR  # noqa: E402

DEST = json.loads((ROOT / 'manifest.json').read_text())['dest']
R = IR.R
require = IR.require


def configure_anim_data(settings):
    settings.set_editor_property('import_uniform_scale', 1.)
    settings.set_editor_property('convert_scene', True)
    settings.set_editor_property('convert_scene_unit', True)
    settings.set_editor_property('force_front_x_axis', True)
    # This one decides: the importer derives the two flags above from it, as for the skin.
    settings.set_editor_property('coordinate_system_policy', unreal.CoordinateSystemPolicy.MATCH_UP_FORWARD_AXES)
    settings.set_editor_property('import_rotation', unreal.Rotator(pitch=0, yaw=90, roll=0))
    settings.set_editor_property('animation_length', unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    settings.set_editor_property('use_default_sample_rate', False)
    settings.set_editor_property('custom_sample_rate', 24)
    settings.set_editor_property('remove_redundant_keys', False)
    settings.set_editor_property('import_bone_tracks', True)
    settings.set_editor_property('import_custom_attribute', False)


def import_mesh(data, material_map):
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / data['lods'][0]['fbx'])
    task.destination_path = DEST
    task.destination_name = data['name']
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_animations = False
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.automated_import_should_detect_type = False
    IR.configure_fbx_data(options.skeletal_mesh_import_data)
    existing = unreal.load_asset(DEST + '/' + data['name'])
    if existing:
        saved = existing.get_editor_property('asset_import_data')
        IR.configure_fbx_data(saved)
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(DEST + '/' + data['name'])
    require('Pale One skeletal mesh imported', isinstance(mesh, unreal.SkeletalMesh), task.imported_object_paths)
    IR.require_fbx_data(mesh.get_editor_property('asset_import_data'))
    editor = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    for row in data['lods'][1:]:
        imported = editor.import_lod(mesh, row['level'], str(ROOT / row['fbx']))
        require('Pale One LOD imported', imported == row['level'], (row['level'], imported))
    require('three native LODs', editor.get_lod_count(mesh) == 3)
    # A decimated LOD can lose a degenerate triangle in the build: a centimetre or two at a thin tip are no fault of the frame.
    render = [IR.verify_render_lod(mesh, row, .1 if row['level'] == 0 else 2., 0. if row['level'] == 0 else .01) for row in data['lods']]
    entries = mesh.get_editor_property('materials')
    material = next(iter(material_map.values()))
    for entry in entries:
        entry.material_interface = material
    mesh.set_editor_property('materials', entries)
    bounds = mesh.get_imported_bounds()
    height, low = float(bounds.box_extent.z) * 2, float(bounds.origin.z) - float(bounds.box_extent.z)
    require('native height', abs(height - data['height_m'] * 100) < .5, height)
    require('native foot pivot', abs(low) < .5, low)
    require('skin saved', R.E.save_loaded_asset(mesh, only_if_is_dirty=False))
    skeleton = mesh.get_editor_property('skeleton')
    require('skeleton kept', skeleton is not None)
    require('skeleton saved', R.E.save_loaded_asset(skeleton, only_if_is_dirty=False))
    return mesh, skeleton, height, low, render


def import_clip(row, skeleton):
    task = unreal.AssetImportTask()
    task.filename = str(ROOT / row['fbx'])
    task.destination_path = DEST
    task.destination_name = row['name']
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = False
    options.import_as_skeletal = False
    options.import_animations = True
    options.import_materials = False
    options.import_textures = False
    options.create_physics_asset = False
    options.skeleton = skeleton
    options.original_import_type = unreal.FBXImportType.FBXIT_ANIMATION
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.automated_import_should_detect_type = False
    configure_anim_data(options.anim_sequence_import_data)
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sequence = unreal.load_asset(DEST + '/' + row['name'])
    require('clip imported', isinstance(sequence, unreal.AnimSequence), (row['name'], task.imported_object_paths))
    require('clip saved', R.E.save_loaded_asset(sequence, only_if_is_dirty=False), row['name'])
    return sequence


def bone_cm(sequence, parents, bone, seconds):
    """The bone in component space, up its chain from the clip's own local transforms.

    The root is the skin's own: its unit and frame reach only the skin, and the game reads it from there too (FS47ClipBank).
    """
    chain = []
    at = bone
    while at:
        chain.append(at)
        at = parents[at]
    world = unreal.Transform()
    reference = unreal.AnimPoseExtensions.get_reference_pose(sequence.get_editor_property('skeleton'))
    for name in reversed(chain):
        if parents[name] is None:
            local = unreal.AnimPoseExtensions.get_bone_pose(reference, name, unreal.AnimPoseSpaces.LOCAL)
        else:
            local = unreal.AnimationLibrary.get_bone_pose_for_time(sequence, name, seconds, False)
        world = unreal.MathLibrary.compose_transforms(local, world)
    location = world.translation
    return [location.x, location.y, location.z]


def check_clip(sequence, row, parents):
    worst = 0.
    for sample in row['check']:
        seconds = min(sample['seconds'], float(unreal.AnimationLibrary.get_sequence_length(sequence)) - 1e-4)
        for bone, expected in sample['ue_cm'].items():
            actual = bone_cm(sequence, parents, bone, seconds)
            worst = max(worst, math.dist(actual, expected))
    return worst


def read_back():
    """Nothing imported: the clips already there against Blender, bone by bone, for when the check below says no."""
    data = json.loads((ROOT / 'manifest.json').read_text())
    parents = dict(data['bones'])
    rows = []
    for row in data['clips']:
        sequence = unreal.load_asset(DEST + '/' + row['name'])
        sample = row['check'][1]
        for bone in ('pelvis', 'head', 'foot_l'):
            chain = []
            at = bone
            while at:
                chain.append(at)
                at = parents[at]
            local = {name: str(unreal.AnimationLibrary.get_bone_pose_for_time(sequence, name, sample['seconds'], False)) for name in chain}
            rows.append({'clip': row['name'], 'seconds': sample['seconds'], 'bone': bone, 'expected': sample['ue_cm'][bone],
                         'actual': bone_cm(sequence, parents, bone, sample['seconds']), 'local': local if bone == 'pelvis' else {}})
    sequence = unreal.load_asset(DEST + '/' + data['clips'][0]['name'])
    settings = sequence.get_editor_property('asset_import_data')
    mesh = unreal.load_asset(DEST + '/' + data['name'])
    pose = unreal.AnimPoseExtensions.get_reference_pose(mesh.get_editor_property('skeleton'))
    rows.append({'settings': {key: str(settings.get_editor_property(key)) for key in ('convert_scene', 'convert_scene_unit', 'force_front_x_axis', 'import_rotation', 'import_uniform_scale', 'coordinate_system_policy')},
                 'reference_local': {name: str(unreal.AnimPoseExtensions.get_bone_pose(pose, name, unreal.AnimPoseSpaces.LOCAL)) for name in ('root', 'pelvis', 'spine_01') if name in dict(data['bones'])}})
    (ROOT / 'read_back.json').write_text(json.dumps(rows, indent=1) + '\n')


def main():
    if os.environ.get('S47_PALE_ONE_READ_BACK'):
        read_back()
        return
    flag = 'Interchange.FeatureFlags.Import.FBX'
    unreal.SystemLibrary.execute_console_command(None, flag + ' 0')
    require('legacy FBX translator selected', unreal.SystemLibrary.get_console_variable_int_value(flag) == 0)
    data = json.loads((ROOT / 'manifest.json').read_text())
    R.check = require
    material_map = {}
    for row in data['materials']:
        material = R.material(row, DEST + '/Materials', ROOT)
        material.set_editor_property('used_with_skeletal_mesh', True)
        require(row['name'] + ' compiles', not list(R.M.recompile_material(material)))
        require(row['name'] + ' saved', R.E.save_loaded_asset(material, only_if_is_dirty=False))
        material_map[row['name']] = material
    # All generated here, and nothing else refers to them: a bone added since the last time must not meet the old skeleton.
    for path in [row['name'] for row in data['clips']] + [data['name'], data['name'] + '_Skeleton']:
        if R.E.does_asset_exist(DEST + '/' + path):
            require('old asset removed', R.E.delete_asset(DEST + '/' + path), path)
    mesh, skeleton, height, low, render = import_mesh(data, material_map)
    parents = dict(data['bones'])
    clips = []
    for row in data['clips']:
        sequence = import_clip(row, skeleton)
        length = float(unreal.AnimationLibrary.get_sequence_length(sequence))
        require('clip length', abs(length - row['seconds']) < .05, (row['name'], length, row['seconds']))
        worst = check_clip(sequence, row, parents)
        clips.append({'name': row['name'], 'seconds': length, 'worst_bone_error_cm': worst})
        unreal.log('PALE ONE CLIP ' + json.dumps(clips[-1]))
    result = {'mesh': mesh.get_path_name(), 'skeleton': skeleton.get_path_name(), 'height_cm': height, 'foot_z_cm': low,
              'render_lods': render, 'clips': clips, 'source_sha256': data['source_sha256'],
              'materials': [m.get_path_name() for m in material_map.values()]}
    (ROOT / 'imported.json').write_text(json.dumps(result, indent=2) + '\n')
    unreal.log('PALE ONE IMPORT COMPLETE ' + json.dumps(result))
    worst_all = max(c['worst_bone_error_cm'] for c in clips)
    require('clips match Blender within 1 cm', worst_all < 1., worst_all)


if __name__ == '__main__':
    main()
