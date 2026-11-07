"""Prepare Tom's two rigged GLBs (Tripo, 2026-11-07) for Unreal: skinned mesh, three LODs and one FBX per clip.

--kind pale is the Pale One and --kind tall the Tall One, both already on UE5 mannequin bone names. --kind roswell is the new small one, on Mixamo names:
they are renamed in this copy with the same aliases as prepare_roswell.py, and its clips' channels with them.

Run through run_pale_one.py (reservation and idle guard). The GLB is read only; everything goes to the private folder.
Unlike the Roswell copy the transform is applied to the data, and the clips' bone locations are scaled with it, so that
the rig, the mesh and every clip share one frame with no scale on any object. The armature keeps the name Armature:
Unreal drops that one dummy node from Blender files, and `root` is then the real root bone.

Coordinates as the Roswell recipe: Blender metres, Z up, forward -Y; Unreal centimetres, Z up, forward +Y, feet at Z=0.
"""
import argparse
import json
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prepare_roswell as PR  # noqa: E402

BODIES = {
    'pale': {'name': 'SK_Tripo_PaleOne', 'clip_prefix': 'A_PaleOne_', 'material': 'M_Tripo_PaleOne_00', 'height': 2.25,
             'dest': '/Game/Licensed/TripoNPC/Animated',
             'clips': {'idle.001': 'idle', 'walk.001': 'walk', 'climb.001': 'climb', 'jump_down.001': 'jump_down',
                       'angry_02.001': 'angry', 'frustrated_02.001': 'frustrated'}},
    'tall': {'name': 'SK_Tripo_TallOne', 'clip_prefix': 'A_TallOne_', 'material': 'M_Tripo_TallOne_00', 'height': 2.45,
             'dest': '/Game/Licensed/TripoNPC/Animated',
             'clips': {'idle.001': 'idle', 'walk.001': 'walk', 'run.001': 'run', 'jump.001': 'jump', 'jump_down.001': 'jump_down',
                       'fall.001': 'fall', 'sit.001': 'sit', 'standing_relax.001': 'relax', 'look_around.001': 'look_around',
                       'sneak walking': 'sneak', 'standing still': 'observe', 'completely still': 'listen',
                       'A small thin': 'freeze', 'breating': 'shh'}},
    'roswell': {'name': 'SK_Tripo_Roswell2', 'clip_prefix': 'A_Roswell2_', 'material': 'M_Tripo_Roswell2_00', 'height': 1.20,
                'dest': '/Game/Licensed/TripoNPC/Roswell',
                'clips': {'sneak walking': 'sneak', 'completely still': 'listen', 'walk.001': 'walk', 'run.001': 'run',
                          'climb.001': 'climb', 'jump_down.001': 'jump_down', 'fall.001': 'fall', 'afraid.001': 'afraid'}},
}
PROBE_BONES = ('pelvis', 'head', 'foot_l', 'foot_r', 'hand_r', 'ball_l')


def args():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('output')
    parser.add_argument('--kind', choices=tuple(BODIES), default='pale')
    parser.add_argument('--height', type=float, default=0)
    parser.add_argument('--max-texture', type=int, default=4096)
    return parser.parse_args(sys.argv[sys.argv.index('--') + 1:])


def fcurves(action):
    return [c for layer in action.layers for strip in layer.strips for bag in strip.channelbags for c in bag.fcurves]


def use_action(arm, action):
    arm.animation_data.action = action
    if action and len(action.slots):
        arm.animation_data.action_slot = action.slots[0]


def pin_root(arm, action, scene):
    """Move what a clip does with the root down into the hips, frame by frame: the path through the world stays the same."""
    use_action(arm, action)
    hips = arm.pose.bones['pelvis']
    first, last = (int(round(v)) for v in action.frame_range)
    world = {}
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        bpy.context.view_layer.update()
        world[frame] = hips.matrix.copy()
    for layer in action.layers:
        for strip in layer.strips:
            for bag in strip.channelbags:
                for curve in [c for c in bag.fcurves if c.data_path.startswith('pose.bones["root"]')]:
                    bag.fcurves.remove(curve)
    root = arm.pose.bones.get('root')
    if root:
        root.matrix_basis = Matrix.Identity(4)
    for frame, matrix in world.items():
        scene.frame_set(frame)
        if root:
            root.matrix_basis = Matrix.Identity(4)
        bpy.context.view_layer.update()
        hips.matrix = matrix
        hips.keyframe_insert('location', frame=frame)
        hips.keyframe_insert('rotation_quaternion', frame=frame)


def still(arm):
    arm.animation_data.action = None
    for bone in arm.pose.bones:
        bone.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


def posed_points(meshes):
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    out = []
    for obj in meshes:
        evaluated = obj.evaluated_get(depsgraph)
        data = evaluated.to_mesh()
        co = np.empty(len(data.vertices) * 3)
        data.vertices.foreach_get('co', co)
        matrix = np.array(evaluated.matrix_world)
        out.append(co.reshape((-1, 3)) @ matrix[:3, :3].T + matrix[:3, 3])
        evaluated.to_mesh_clear()
    return np.vstack(out)


def dark_eyes(meshes, arm):
    """Where the black shows on the head, per side: from the base colour under each front-facing face of the head."""
    head = arm.data.bones['head'].head_local
    found = {1: [], -1: []}
    for obj in meshes:
        data = obj.data
        uv = data.uv_layers.active.data
        for poly in data.polygons:
            centre = obj.matrix_world @ poly.center
            if centre.z < head.z - .02 or centre.y > head.y:  # above the head bone's base, and on the face (-Y)
                continue
            slot = obj.material_slots[poly.material_index].material
            source = PR.image_link(next(n for n in slot.node_tree.nodes if n.type == 'BSDF_PRINCIPLED').inputs['Base Color'])
            if not source:
                continue
            image = source[0]
            if not hasattr(dark_eyes, 'cache'):
                w, h = image.size
                pixels = np.empty(w * h * image.channels, dtype=np.float32)
                image.pixels.foreach_get(pixels)
                dark_eyes.cache = (pixels.reshape((h, w, image.channels)), w, h)
            pixels, w, h = dark_eyes.cache
            u = float(np.mean([uv[i].uv[0] for i in poly.loop_indices]))
            v = float(np.mean([uv[i].uv[1] for i in poly.loop_indices]))
            r, g, b = pixels[min(h - 1, max(0, int(v % 1 * h))), min(w - 1, max(0, int(u % 1 * w))), :3]
            if .2126 * r + .7152 * g + .0722 * b < .05:
                side = 1 if centre.x - head.x > 0 else -1
                found[side].append((tuple(centre), poly.area))
    result = {}
    for side, rows in found.items():
        if not rows:
            continue
        points = np.array([r[0] for r in rows])
        areas = np.array([r[1] for r in rows])
        centre = (points * areas[:, None]).sum(axis=0) / areas.sum()
        result['left' if side > 0 else 'right'] = {
            'faces': len(rows), 'area_cm2': float(areas.sum() * 1e4),
            'centre_ue_cm': [float(100 * centre[0]), float(-100 * centre[1]), float(100 * centre[2])],
            'extent_cm': (100 * np.ptp(points, axis=0)).tolist(),
            'most_forward_ue_cm': [float(100 * points[points[:, 1].argmin()][0]), float(-100 * points[:, 1].min()), float(100 * points[points[:, 1].argmin()][2])],
        }
    return {'head_bone_ue_cm': [100 * head.x, -100 * head.y, 100 * head.z], 'sides': result}


def main():
    options = args()
    body = BODIES[options.kind]
    NAME = body['name']
    options.height = options.height or body['height']
    source = Path(options.source).resolve(strict=True)
    output = Path(options.output).resolve()
    (output / 'textures').mkdir(parents=True, exist_ok=True)
    original_sha = PR.sha(source)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.render.fps, scene.render.fps_base = 24, 1
    bpy.ops.import_scene.gltf(filepath=str(source))
    arms = [o for o in scene.objects if o.type == 'ARMATURE']
    meshes = [o for o in scene.objects if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers)]  # not the importer's bone shape
    if len(arms) != 1 or len(meshes) != 1:
        raise RuntimeError('Expected one skeleton and one skinned mesh')
    arm = arms[0]
    arm.animation_data_create()
    for track in list(arm.animation_data.nla_tracks):
        arm.animation_data.nla_tracks.remove(track)
    arm.animation_data.action = None
    actions = {}
    for start, short in body['clips'].items():
        found = [a for a in bpy.data.actions if a.name.startswith(start)]
        if len(found) != 1:
            raise RuntimeError('Clip not found once: %s in %s' % (start, [a.name for a in bpy.data.actions]))
        actions[found[0].name] = found[0]
    clip_names = {next(a.name for a in actions.values() if a.name.startswith(start)): short for start, short in body['clips'].items()}
    aliases = PR.alias_bones(arm, {})
    # Renaming a bone only reaches the action in use: every other clip's channels are told here.
    for action in actions.values():
        for curve in fcurves(action):
            for old, new in aliases.items():
                if '"%s"' % old in curve.data_path:
                    curve.data_path = curve.data_path.replace('"%s"' % old, '"%s"' % new)
    missing = [n for n in PR.REQUIRED if n not in arm.data.bones]
    if missing:
        raise RuntimeError('Missing bones ' + str(missing))

    # Scale, floor and facing from the rest pose.
    arm.data.pose_position = 'REST'
    still(arm)
    rest = posed_points(meshes)
    rotation, facing = PR.facing(arm, '-Y')
    turned = rest @ np.array(rotation)[:3, :3].T
    factor = options.height / np.ptp(turned[:, 2])
    hip = rotation @ arm.matrix_world @ arm.data.bones['pelvis'].head_local
    pivot = Vector((hip.x * factor, hip.y * factor, turned[:, 2].min() * factor))
    transform = Matrix.Translation(-pivot) @ Matrix.Scale(factor, 4) @ rotation

    # One posed frame before, to compare after: the whole chain from fcurves to vertices must survive this.
    arm.data.pose_position = 'POSE'
    walk = next(a for a in actions.values() if a.name.startswith('walk'))
    use_action(arm, walk)
    scene.frame_set(20)
    before = posed_points(meshes)
    before = before @ np.array(transform)[:3, :3].T + np.array(transform)[:3, 3]

    # Apply to the data: no object keeps a transform and the bones' local location keys scale with the rig.
    arm.data.pose_position = 'REST'
    still(arm)
    worlds = {o: o.matrix_world.copy() for o in [arm] + meshes}
    for obj in meshes:
        obj.parent = None
    for obj, world in worlds.items():
        obj.matrix_world = transform @ world
    bpy.context.view_layer.update()
    PR.select([arm] + meshes, arm)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for action in actions.values():
        for curve in fcurves(action):
            if curve.data_path.endswith('.location'):
                for key in curve.keyframe_points:
                    key.co[1] *= factor
                    key.handle_left[1] *= factor
                    key.handle_right[1] *= factor
                curve.update()
    arm.name = 'Armature'
    # Unreal takes the file's unit and frame into the root bone of the skin (scale 100), but not into a clip's root track. The
    # root must therefore stand still and the hips hang under it, as in the UE5 rigs: a Mixamo rig gets one, at the floor.
    if 'root' not in arm.data.bones:
        PR.select([arm], arm)
        bpy.ops.object.mode_set(mode='EDIT')
        root = arm.data.edit_bones.new('root')
        root.head, root.tail = (0, 0, 0), (0, 0, .1)
        arm.data.edit_bones['pelvis'].parent = root
        arm.data.edit_bones['pelvis'].use_connect = False
        bpy.ops.object.mode_set(mode='OBJECT')
    arm.data.pose_position = 'POSE'
    still(arm)
    # For the same reason a clip's root must not turn: some of these do, by a few degrees, each its own.
    for action in actions.values():
        pin_root(arm, action, scene)
    still(arm)
    bounds = posed_points(meshes)
    if abs(bounds[:, 2].min()) > 1e-4 or abs(np.ptp(bounds[:, 2]) - options.height) > 1e-4:
        raise RuntimeError('Normalized height or floor wrong')
    if (arm.matrix_world.to_scale() - Vector((1, 1, 1))).length > 1e-6:
        raise RuntimeError('The rig still has scale')
    use_action(arm, walk)
    scene.frame_set(20)
    after = posed_points(meshes)
    chain_error = float(np.abs(after - before).max())
    if chain_error > 5e-4:
        raise RuntimeError('A clip changed under normalization: %.5f m' % chain_error)

    # Material, as delivered: the base colour. The GLB is two-sided with one atlas.
    still(arm)
    slot = meshes[0].material_slots[0].material
    material_name = body['material']
    shader = next(n for n in slot.node_tree.nodes if n.type == 'BSDF_PRINCIPLED')
    textures = {}
    for role, key in (('base_color', 'Base Color'), ('normal', 'Normal'), ('roughness', 'Roughness'), ('metallic', 'Metallic')):
        found = PR.image_link(shader.inputs.get(key))
        if found:
            target = output / 'textures' / (material_name + '_' + role + '.png')
            PR.texture_copy(found[0], found[1], target, role, options.max_texture)
            textures[role] = str(target.relative_to(output))
    if 'base_color' not in textures:
        raise RuntimeError('No base atlas')
    eyes = dark_eyes(meshes, arm)
    slot.name = material_name
    materials = [{'name': material_name, 'source_name': slot.name, 'base_color': [1, 1, 1, 1], 'roughness': float(shader.inputs['Roughness'].default_value),
                  'metallic': float(shader.inputs['Metallic'].default_value), 'specular': .4, 'normal_strength': 1., 'two_sided': True,
                  'textures': textures}]
    for image in bpy.data.images:
        if image.source != 'VIEWER' and image.size[0]:
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (NAME + '.blend')))

    lods = []
    for level, ratio in enumerate((1., .5, .2)):
        copies = []
        for obj in meshes:
            copy = obj.copy()
            copy.data = obj.data.copy()
            scene.collection.objects.link(copy)
            if level:
                PR.select([copy], copy)
                modifier = copy.modifiers.new('PaleLOD', 'DECIMATE')
                modifier.ratio = ratio
                modifier.use_collapse_triangulate = True
                bpy.ops.object.modifier_move_up(modifier=modifier.name)
                bpy.ops.object.modifier_apply(modifier=modifier.name)
            copies.append(copy)
        path = output / (NAME + ('' if level == 0 else '_LOD' + str(level)) + '.fbx')
        PR.export(path, arm, copies)
        xyz = PR.points(copies)
        lods.append({'level': level, 'fbx': path.name, 'ratio': ratio,
                     'triangles': sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in copies),
                     'bounds_blender_m': [xyz.min(axis=0).tolist(), xyz.max(axis=0).tolist()],
                     'probe_points_blender_m': PR.geometry_probes(xyz)})
        for obj in copies:
            data = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(data)

    clips = []
    for source_name, short in clip_names.items():
        action = actions[source_name]
        first, last = (int(round(v)) for v in action.frame_range)
        use_action(arm, action)
        scene.frame_start, scene.frame_end = first, last
        check = []
        for frame in sorted({first, first + (last - first) // 3, first + 2 * (last - first) // 3, last}):
            scene.frame_set(frame)
            bpy.context.view_layer.update()
            check.append({'seconds': (frame - first) / 24, 'ue_cm': {
                b: [100 * arm.pose.bones[b].head.x, -100 * arm.pose.bones[b].head.y, 100 * arm.pose.bones[b].head.z] for b in PROBE_BONES}})
        scene.frame_set(first)
        path = output / (body['clip_prefix'] + short + '.fbx')
        PR.select([arm] + meshes, arm)
        bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH', 'ARMATURE'},
                                 apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z',
                                 add_leaf_bones=False, use_armature_deform_only=False, mesh_smooth_type='FACE', use_tspace=True,
                                 bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
                                 bake_anim_use_all_actions=False, bake_anim_force_startend_keying=True,
                                 bake_anim_step=1, bake_anim_simplify_factor=0)
        clips.append({'name': body['clip_prefix'] + short, 'short': short, 'source': source_name, 'fbx': path.name,
                      'frames': [first, last], 'seconds': (last - first) / 24, 'check': check})
    still(arm)
    if PR.sha(source) != original_sha:
        raise RuntimeError('Original source changed')
    manifest = {'version': 1, 'name': NAME, 'role': options.kind, 'dest': body['dest'], 'bone_aliases': aliases, 'source': str(source), 'source_sha256': original_sha,
                'blend': NAME + '.blend', 'height_m': options.height, 'factor': factor, 'skeletal': True,
                'coordinates': 'Blender metres/Z-up/forward -Y; UE centimetres/Z-up/forward +Y; feet pivot Z=0',
                'facing_method': facing, 'chain_error_m': chain_error,
                'bones': [[b.name, b.parent.name if b.parent else None] for b in arm.data.bones],
                'materials': materials, 'lods': lods, 'clips': clips, 'eyes': eyes,
                'bounds_blender_m': [bounds.min(axis=0).tolist(), bounds.max(axis=0).tolist()]}
    (output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print('PALE ONE PREPARED ' + json.dumps({'factor': factor, 'height_m': options.height, 'chain_error_m': chain_error, 'eyes': eyes,
                                             'lod_triangles': [r['triangles'] for r in lods], 'clips': [c['short'] for c in clips]}), flush=True)


if __name__ == '__main__':
    main()
