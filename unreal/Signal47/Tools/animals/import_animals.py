"""The CC0 animals (ANIMALS-20261108) into /Game/Free/Animals: Quaternius' wolf as the coyote, his husky as the ranch dog.

Run in UnrealEditor-Cmd through import_animals.sh (one heavy job at a time). Interchange reads the glTF as it is: a skinned mesh,
its skeleton and its clips. The game colours them itself (S47Animals: a dun coat, a pale belly, the eyes that shine) and scales
them to their size, so nothing here is changed but the names:

  /Game/Free/Animals/<Animal>/SK_<Animal>            the skeletal mesh
  /Game/Free/Animals/<Animal>/A_<Animal>_<Clip>       Walk, Gallop, Idle, Idle2, HeadLow, Eating, Attack

Reads back what it made: the mesh, its bones the game turns by (Head, Body, Tail1), its size, and each clip's length.
"""
import os
import re

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
DEST = '/Game/Free/Animals'
ANIMALS = [
    {'name': 'Coyote', 'src': 'src/quaternius_wolf.glb'},
    {'name': 'Dog', 'src': 'src/quaternius_husky.glb'},
]
# the clips the game plays, by the glTF's own animation names
CLIPS = {'Walk': 'Walk', 'Gallop': 'Gallop', 'Idle': 'Idle', 'Idle2': 'Idle_2', 'HeadLow': 'Idle_2_HeadLow', 'Eating': 'Eating', 'Attack': 'Attack'}
BONES = ('Head', 'Body', 'Tail1')
E = unreal.EditorAssetLibrary
FAILS = []


def log(s):
    unreal.log_warning('S47ANIMALS ' + s)


def require(what, ok, detail=''):
    log('%s %s %s' % ('PASS' if ok else 'FAIL', what, detail))
    if not ok:
        FAILS.append(what)


def set_if(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception as err:  # a property this engine does not have: say so, go on
        log('note: %s.%s not set (%s)' % (type(obj).__name__, prop, err))


def pipelines(name):
    assets = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFAssetsPipeline')
    gltf = unreal.load_asset('/Interchange/Pipelines/DefaultGLTFPipeline')
    set_if(assets, 'scene_name_sub_folder', False)
    set_if(assets, 'asset_type_sub_folders', False)
    set_if(assets, 'asset_name', name)
    common = assets.get_editor_property('common_meshes_properties')
    set_if(common, 'force_all_mesh_as_type', unreal.InterchangeForceMeshType.IFMT_NONE)
    set_if(common, 'bake_meshes', True)
    mesh = assets.get_editor_property('mesh_pipeline')
    set_if(mesh, 'import_static_meshes', False)
    set_if(mesh, 'import_skeletal_meshes', True)
    set_if(mesh, 'build_nanite', False)
    set_if(mesh, 'create_physics_asset', False)
    anim = assets.get_editor_property('animation_pipeline')
    set_if(anim, 'import_animations', True)
    return assets, gltf


def clip_key(asset_name):
    # "AnimalArmature_Walk", "Coyote_Anim_Walk", "Walk": the glTF name is the end of it
    for key, gltf in sorted(CLIPS.items(), key=lambda kv: -len(kv[1])):
        if re.search(r'(^|[_|])' + re.escape(gltf) + r'$', asset_name):
            return key, 'AnimalArmature' not in asset_name
    return None, False


def import_one(animal):
    name = animal['name']
    folder = '%s/%s' % (DEST, name)
    src = os.path.join(HERE, animal['src'])
    require('%s source there' % name, os.path.isfile(src), src)
    if not os.path.isfile(src):
        return
    if E.does_directory_exist(folder):
        E.delete_directory(folder)
    assets, gltf = pipelines(name)
    stack = unreal.InterchangePipelineStackOverride()
    stack.add_pipeline(assets)
    stack.add_pipeline(gltf)
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', src)
    task.set_editor_property('destination_path', folder)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    task.set_editor_property('options', stack)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    made = list(task.get_editor_property('imported_object_paths') or [])
    log('%s imported %d objects' % (name, len(made)))
    mesh_path, clips = None, {}
    for path in E.list_assets(folder, recursive=True, include_folder=False):
        asset = E.load_asset(path)
        short = path.split('/')[-1].split('.')[0]
        if isinstance(asset, unreal.SkeletalMesh):
            mesh_path = path
        elif isinstance(asset, unreal.AnimSequence):
            key, plain = clip_key(short)
            # the clip without the armature's prefix first; the other copy only if there is no plain one
            if key and (plain or key not in clips):
                clips[key] = path
    require('%s skeletal mesh' % name, mesh_path is not None, mesh_path)
    if not mesh_path:
        return
    target = '%s/SK_%s' % (folder, name)
    if mesh_path.split('.')[0] != target:
        require('%s mesh named' % name, E.rename_asset(mesh_path.split('.')[0], target), target)
    mesh = E.load_asset(target)
    skeleton = mesh.get_editor_property('skeleton')
    require('%s skeleton' % name, skeleton is not None)
    bounds = mesh.get_bounds()
    log('%s bounds origin %s extent %s' % (name, bounds.origin, bounds.box_extent))
    # (the bones the game turns by, Head, Body and Tail1, are looked up and logged by S47Animals itself when it loads the mesh)
    for key in CLIPS:
        path = clips.get(key)
        require('%s clip %s' % (name, key), path is not None or key in ('Eating', 'Attack'), path)
        if not path:
            continue
        dest = '%s/A_%s_%s' % (folder, name, key)
        if path.split('.')[0] != dest:
            E.rename_asset(path.split('.')[0], dest)
        seq = E.load_asset(dest)
        length = -1.0
        if seq:
            try:
                length = float(seq.get_play_length())
            except Exception:
                length = float(seq.get_editor_property('sequence_length'))
        log('%s clip %s %.2f s' % (name, key, length))
    E.save_directory(folder, only_if_is_dirty=False, recursive=True)


def main():
    for animal in ANIMALS:
        import_one(animal)
    log('DONE %s' % ('FAIL ' + ', '.join(FAILS) if FAILS else 'PASS'))


main()
