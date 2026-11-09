"""NPC-ANIM (Tom, 9 November): a townsperson that walks on its feet. Blender, one role at a time.

Caliche's people are Tom's Tripo figures, and every one of them came out of Tripo already rigged and skinned (65 Mixamo bones,
revision-v2/active-assets.json); the game has so far worn static bakes of them (prepare.py). This keeps the figure's own rig and
skin, renames the bones to the UE5 names the game's clip player reads (prepare_roswell.ALIASES, as for Roswell 2), and gives it
clips retargeted from a human-proportioned Tripo body that came with Tripo's own clips (default: nordic.glb of 9 November, UE5
bones, same Tripo clip set as the Tall One's). Clip, skin and rig share one frame with prepare_pale_one.py, so import_pale_one.py
takes the result as it is, into /Game/Licensed/TripoNPC/Rigged (outside Git).

Retargeting: for each mapped bone the source's rotation away from its rest pose, in world space, is put on the target bone after its
rest direction has been turned onto the source's (so a T-pose and an A-pose meet); bone lengths stay the target's. The hips carry
the source's path scaled by the ratio of hip heights, so a walk's travel per cycle fits these legs; FS47ClipBank reads that travel
and plays the walk by distance walked. Run:

    blender -b -t 4 --python-exit-code 1 --python prepare_rigged_townsfolk.py -- --role deputy --output DIR

The figure's FBX and the source GLB are only read. Coordinates as prepare_pale_one.py: Blender metres, Z up, forward -Y; feet at
Z=0 under the pelvis, which is also where the static bake of the same role has its pivot.
"""
import argparse
import json
import os
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

TOOLS = Path(os.environ.get('S47_TRIPONPCS_TOOLS', str(Path(__file__).resolve().parent)))
sys.path.insert(0, str(TOOLS))
import prepare_roswell as PR  # noqa: E402
import prepare_pale_one as PP  # noqa: E402

MODELS = Path.home() / 'Documents/ChatGPT/Signal 47/modeller'
ROSTER = MODELS / 'tripo-npc-2026-10-29/revision-v2/active-assets.json'
DEFAULT_SOURCE = MODELS / 'tripo-new-2026-11-09/raw/nordic.glb'
ROLES = json.loads((TOOLS / 'roles.json').read_text())
# Source clip (name start) -> the file name FS47ClipBank reads (ClipFile in S47CreatureClips.cpp).
CLIPS = {'idle.001': 'idle', 'walk.001': 'walk', 'sit.001': 'sit', 'standing_relax.001': 'relax', 'look_around.001': 'look_around'}
# Target (after the Mixamo aliases) -> source, UE5 names. The target has three spine bones, the source five.
MAP = {'pelvis': 'pelvis', 'spine_01': 'spine_01', 'spine_02': 'spine_03', 'spine_03': 'spine_05', 'neck_01': 'neck_01', 'head': 'head'}
for s in ('l', 'r'):
    for b in ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot', 'ball'):
        MAP[b + '_' + s] = b + '_' + s
    side = 'Left' if s == 'l' else 'Right'
    for finger, ue in (('Index', 'index'), ('Middle', 'middle'), ('Ring', 'ring'), ('Pinky', 'pinky'), ('Thumb', 'thumb')):
        for i in (1, 2, 3):
            MAP['mixamorig:%sHand%s%d' % (side, finger, i)] = '%s_%02d_%s' % (ue, i, s)


def args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--role', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--source', default=str(DEFAULT_SOURCE))
    parser.add_argument('--dest', default='/Game/Licensed/TripoNPC/Rigged')
    parser.add_argument('--max-texture', type=int, default=2048)
    return parser.parse_args(sys.argv[sys.argv.index('--') + 1:])


def skinned(objects):
    return [o for o in objects if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers)]


def rot(matrix):
    return matrix.to_3x3().normalized().to_quaternion()


def canon(arm, meshes):
    """Rotation onto forward -Y, and the offset that puts the feet at 0 under the rest pelvis (world space, rest pose)."""
    arm.data.pose_position = 'REST'
    PP.still(arm)
    rotation, method = PR.facing(arm, '-Y')
    points = PP.posed_points(meshes) @ np.array(rotation)[:3, :3].T
    hip = rotation @ arm.matrix_world @ arm.data.bones['pelvis'].head_local
    arm.data.pose_position = 'POSE'
    return rotation, Vector((hip.x, hip.y, float(points[:, 2].min()))), float(np.ptp(points[:, 2])), method


def textures_of(fbx, output, name, max_size):
    """The figure's maps from the FBX's own .fbm folder, as prepare.py finds them."""
    files = [p for p in fbx.parent.rglob('*') if p.is_file() and p.suffix.lower() in ('.png', '.jpg', '.jpeg')]
    tests = {'base_color': lambda p: any(t in p.stem.lower() for t in ('basecolor', 'base_color', 'tripo_rgb', 'albedo')),
             'normal': lambda p: 'normal' in p.stem.lower(), 'roughness': lambda p: 'roughness' in p.stem.lower(),
             'metallic': lambda p: 'metallic' in p.stem.lower()}
    result = {}
    for role, test in tests.items():
        found = [p for p in files if test(p)] or (files if role == 'base_color' and len(files) == 1 else [])
        if not found:
            continue
        if len(found) != 1:
            raise RuntimeError('Ambiguous texture %s: %s' % (role, found))
        image = bpy.data.images.load(str(found[0]), check_existing=False)
        target = output / 'textures' / (name + '_' + role + '.png')
        PR.texture_copy(image, None, target, role, max_size)
        bpy.data.images.remove(image)
        result[role] = str(target.relative_to(output))
    if 'base_color' not in result:
        raise RuntimeError('No base colour next to ' + str(fbx))
    return result


def depth(bone):
    n = 0
    while bone.parent:
        bone, n = bone.parent, n + 1
    return n


def retarget(scene, src, src_rot, src_off, tgt, action, name, k):
    """Bake one source action onto the target as a new action; returns its frame range and the hips' path."""
    PP.use_action(src, action)
    first, last = (int(round(v)) for v in action.frame_range)
    bones = sorted(tgt.data.bones, key=depth)
    rest = {b.name: b.matrix_local.copy() for b in bones}
    rel = {b.name: (rest[b.parent.name].inverted() @ rest[b.name]) if b.parent else rest[b.name] for b in bones}
    srot4 = src_rot @ src.matrix_world
    pairs = {t: s for t, s in MAP.items() if t in tgt.data.bones and s in src.data.bones}
    # Rest frames: the target's bone turned onto the source's direction first, so different rest poses meet.
    aligned = {}
    src_rest = {}
    for t, s in pairs.items():
        sm = srot4 @ src.data.bones[s].matrix_local
        tm = rest[t]
        src_rest[t] = rot(sm)
        sdir = (sm.to_3x3().normalized() @ Vector((0, 1, 0))).normalized()
        tdir = (tm.to_3x3().normalized() @ Vector((0, 1, 0))).normalized()
        aligned[t] = tdir.rotation_difference(sdir) @ rot(tm)
    src_pelvis_rest = srot4 @ src.data.bones['pelvis'].head_local - src_off
    tgt_pelvis_rest = rest['pelvis'].translation.copy()
    out = bpy.data.actions.new(name)
    out.use_fake_user = True
    tgt.animation_data_create()
    tgt.animation_data.action = out
    for pb in tgt.pose.bones:
        pb.rotation_mode = 'QUATERNION'
    previous = {}
    path = []
    for frame in range(first, last + 1):
        scene.frame_set(frame)
        posed = {}
        for b in bones:
            n = b.name
            base = posed[b.parent.name] @ rel[n] if b.parent else rest[n].copy()
            if n in pairs:
                s = pairs[n]
                q = rot(srot4 @ src.pose.bones[s].matrix) @ src_rest[n].inverted() @ aligned[n]
                at = base.translation
                if n == 'pelvis':
                    p = srot4 @ src.pose.bones['pelvis'].head - src_off
                    at = Vector((tgt_pelvis_rest.x + (p.x - src_pelvis_rest.x) * k, tgt_pelvis_rest.y + (p.y - src_pelvis_rest.y) * k, p.z * k))
                    path.append(tuple(at))
                base = Matrix.Translation(at) @ q.to_matrix().to_4x4()
            posed[n] = base
        for b in bones:
            n = b.name
            basis = (rel[n].inverted() @ posed[b.parent.name].inverted() @ posed[n]) if b.parent else rest[n].inverted() @ posed[n]
            loc, q, _ = basis.decompose()
            if n in previous and previous[n].dot(q) < 0:
                q.negate()
            previous[n] = q
            pb = tgt.pose.bones[n]
            pb.location, pb.rotation_quaternion = loc, q
            pb.keyframe_insert('location', frame=frame, group=n)
            pb.keyframe_insert('rotation_quaternion', frame=frame, group=n)
    return first, last, np.array(path)


def main():
    o = args()
    role = o.role
    spec = ROLES[role]
    row = next(r for r in json.loads(ROSTER.read_text())['assets'] if r['role'] == role)
    fbx = Path(row['fbx']).resolve(strict=True)
    source = Path(o.source).resolve(strict=True)
    output = Path(o.output).resolve()
    (output / 'textures').mkdir(parents=True, exist_ok=True)
    shas = {fbx: PR.sha(fbx), source: PR.sha(source)}
    NAME, PREFIX, MATERIAL = 'SK_Tripo_' + role, 'A_Tripo_' + role + '_', 'M_Tripo_' + role + '_Rigged'

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system, scene.unit_settings.scale_length = 'METRIC', 1
    scene.render.fps, scene.render.fps_base = 24, 1

    # The figure: its own rig and skin, renamed, scaled to its role, feet at 0 under the pelvis, facing -Y, applied to the data.
    bpy.ops.import_scene.fbx(filepath=str(fbx), use_anim=False)
    arm = next(ob for ob in scene.objects if ob.type == 'ARMATURE')
    meshes = skinned(scene.objects)
    if not meshes:
        raise RuntimeError('No skinned mesh in ' + str(fbx))
    aliases = PR.alias_bones(arm, {})
    arm.animation_data_create()
    rotation, offset, height, facing = canon(arm, meshes)
    factor = spec['height_m'] / height
    transform = Matrix.Scale(factor, 4) @ Matrix.Translation(-offset) @ rotation
    worlds = {ob: ob.matrix_world.copy() for ob in [arm] + meshes}
    for ob in meshes:
        ob.parent = None
    for ob, world in worlds.items():
        ob.matrix_world = transform @ world
    bpy.context.view_layer.update()
    PR.select([arm] + meshes, arm)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    arm.name = 'Armature'
    if 'root' not in arm.data.bones:
        PR.select([arm], arm)
        bpy.ops.object.mode_set(mode='EDIT')
        root = arm.data.edit_bones.new('root')
        root.head, root.tail = (0, 0, 0), (0, 0, .1)
        arm.data.edit_bones['pelvis'].parent = root
        arm.data.edit_bones['pelvis'].use_connect = False
        bpy.ops.object.mode_set(mode='OBJECT')
    PP.still(arm)
    bounds = PP.posed_points(meshes)
    if abs(bounds[:, 2].min()) > 1e-4 or abs(np.ptp(bounds[:, 2]) - spec['height_m']) > 1e-3:
        raise RuntimeError('Normalized height or floor wrong: %s' % (bounds[:, 2].min(), np.ptp(bounds[:, 2])))

    # The source: Tripo's clips on a human-proportioned body, in the same frame (but not moved: read through src_rot/src_off).
    before = set(scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(source))
    added = [ob for ob in scene.objects if ob not in before]
    src = next(ob for ob in added if ob.type == 'ARMATURE')
    src_meshes = skinned(added)
    src.animation_data_create()
    for track in list(src.animation_data.nla_tracks):
        src.animation_data.nla_tracks.remove(track)
    src_rot, src_off, src_height, _ = canon(src, src_meshes)
    src_rot4 = src_rot @ src.matrix_world
    src_hip = (src_rot4 @ src.data.bones['pelvis'].head_local - src_off).z
    tgt_hip = arm.data.bones['pelvis'].head_local.z
    k = tgt_hip / src_hip

    material_name = MATERIAL
    for ob in meshes:
        for slot in ob.material_slots:
            if slot.material:
                slot.material.name = material_name
    maps = textures_of(fbx, output, material_name, o.max_texture)

    clips = []
    actions = {}
    for start, short in CLIPS.items():
        found = [a for a in bpy.data.actions if a.name.startswith(start)]
        if len(found) != 1:
            print('NPCANIM clip missing in source: ' + start, flush=True)
            continue
        first, last, path = retarget(scene, src, src_rot, src_off, arm, found[0], PREFIX + short, k)
        actions[short] = (bpy.data.actions[PREFIX + short], first, last)
        travel = float(np.hypot(*(path[-1][:2] - path[0][:2])))
        seconds = (last - first) / 24
        print('NPCANIM CLIP %s frames %d-%d seconds %.2f travel %.3f m speed %.3f m/s pelvis z %.3f..%.3f' % (
            short, first, last, seconds, travel, travel / max(seconds, .01), path[:, 2].min(), path[:, 2].max()), flush=True)
        clips.append({'short': short, 'travel_m': travel, 'seconds': seconds, 'pelvis_z': [float(path[:, 2].min()), float(path[:, 2].max())]})

    # Only the figure goes out.
    for ob in added:
        data = ob.data
        bpy.data.objects.remove(ob, do_unlink=True)
        if isinstance(data, bpy.types.Mesh):
            bpy.data.meshes.remove(data)
    PP.use_action(arm, None)
    PP.still(arm)
    for image in bpy.data.images:
        if image.source != 'VIEWER' and image.size[0]:
            image.pack()
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (NAME + '.blend')))

    lods = []
    for level, ratio in enumerate((1., .5, .2)):
        copies = []
        for ob in meshes:
            copy = ob.copy()
            copy.data = ob.data.copy()
            scene.collection.objects.link(copy)
            if level:
                PR.select([copy], copy)
                modifier = copy.modifiers.new('TownLOD', 'DECIMATE')
                modifier.ratio = ratio
                modifier.use_collapse_triangulate = True
                bpy.ops.object.modifier_move_up(modifier=modifier.name)
                bpy.ops.object.modifier_apply(modifier=modifier.name)
            copies.append(copy)
        path = output / (NAME + ('' if level == 0 else '_LOD' + str(level)) + '.fbx')
        PR.export(path, arm, copies)
        xyz = PR.points(copies)
        lods.append({'level': level, 'fbx': path.name, 'ratio': ratio,
                     'triangles': sum(sum(len(p.vertices) - 2 for p in ob.data.polygons) for ob in copies),
                     'bounds_blender_m': [xyz.min(axis=0).tolist(), xyz.max(axis=0).tolist()],
                     'probe_points_blender_m': PR.geometry_probes(xyz)})
        for ob in copies:
            data = ob.data
            bpy.data.objects.remove(ob, do_unlink=True)
            bpy.data.meshes.remove(data)

    rows = []
    for info in clips:
        short = info['short']
        action, first, last = actions[short]
        PP.use_action(arm, action)
        scene.frame_start, scene.frame_end = first, last
        check = []
        for frame in sorted({first, first + (last - first) // 3, first + 2 * (last - first) // 3, last}):
            scene.frame_set(frame)
            bpy.context.view_layer.update()
            check.append({'seconds': (frame - first) / 24, 'ue_cm': {
                b: [100 * arm.pose.bones[b].head.x, -100 * arm.pose.bones[b].head.y, 100 * arm.pose.bones[b].head.z] for b in PP.PROBE_BONES}})
        feet = []
        for frame in range(first, last + 1):
            scene.frame_set(frame)
            feet.append(min(arm.pose.bones['ball_l'].head.z, arm.pose.bones['ball_r'].head.z))
        scene.frame_set(first)
        path = output / (PREFIX + short + '.fbx')
        PR.select([arm] + meshes, arm)
        bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={'MESH', 'ARMATURE'},
                                 apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z',
                                 add_leaf_bones=False, use_armature_deform_only=False, mesh_smooth_type='FACE', use_tspace=True,
                                 bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
                                 bake_anim_use_all_actions=False, bake_anim_force_startend_keying=True,
                                 bake_anim_step=1, bake_anim_simplify_factor=0)
        rows.append({'name': PREFIX + short, 'short': short, 'source': source.name + ':' + short, 'fbx': path.name,
                     'frames': [first, last], 'seconds': (last - first) / 24, 'check': check,
                     'travel_m': info['travel_m'], 'pelvis_z_m': info['pelvis_z'], 'ball_low_m': [float(min(feet)), float(max(feet))]})
    PP.use_action(arm, None)
    PP.still(arm)
    for path, sha in shas.items():
        if PR.sha(path) != sha:
            raise RuntimeError('Source changed: ' + str(path))
    materials = [{'name': material_name, 'base_color': [1, 1, 1, 1], 'roughness': .68, 'metallic': 0., 'specular': .4,
                  'normal_strength': 1., 'two_sided': False, 'textures': maps}]
    manifest = {'version': 1, 'name': NAME, 'role': role, 'dest': o.dest, 'bone_aliases': aliases,
                'source': str(fbx), 'source_sha256': shas[fbx], 'clip_source': str(source), 'clip_source_sha256': shas[source],
                'retarget': {'method': 'world rotation from rest, rest directions aligned, hips path scaled by hip height',
                             'hip_ratio': k, 'target_hip_m': tgt_hip, 'source_hip_m': src_hip, 'source_height_m': src_height, 'map': MAP},
                'blend': NAME + '.blend', 'height_m': spec['height_m'], 'factor': factor, 'skeletal': True,
                'coordinates': 'Blender metres/Z-up/forward -Y; UE centimetres/Z-up/forward +Y; feet pivot Z=0 under the rest pelvis (as SM_Tripo_' + role + ')',
                'facing_method': facing, 'chain_error_m': 0.,
                'bones': [[b.name, b.parent.name if b.parent else None] for b in arm.data.bones],
                'materials': materials, 'lods': lods, 'clips': rows,
                'bounds_blender_m': [bounds.min(axis=0).tolist(), bounds.max(axis=0).tolist()]}
    (output / 'manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print('NPCANIM PREPARED ' + json.dumps({'role': role, 'factor': factor, 'hip_ratio': k, 'target_hip_m': tgt_hip, 'source_hip_m': src_hip,
                                            'lod_triangles': [r['triangles'] for r in lods],
                                            'clips': {r['short']: [round(r['seconds'], 2), round(r['travel_m'], 3), [round(v, 3) for v in r['ball_low_m']]] for r in rows}}), flush=True)


if __name__ == '__main__':
    main()
