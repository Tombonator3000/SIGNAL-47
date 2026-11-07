"""Look at the delivered Pale One GLB: true size, rig, clips and pictures. Reads the private copy, changes nothing.

blender -b -t 4 --python-exit-code 1 --python inspect_pale_one.py -- <source.glb> <out dir>
Writes rig.json and PNGs: rest front/side/back, a close view of the head, and one frame per clip.
"""
import json
import math
import sys
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

ARGS = sys.argv[sys.argv.index('--') + 1:]
SOURCE, OUT = Path(ARGS[0]), Path(ARGS[1])
OUT.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(SOURCE))
scene = bpy.context.scene
arm = next(o for o in scene.objects if o.type == 'ARMATURE')
meshes = [o for o in scene.objects if o.type == 'MESH']


def evaluated_points():
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    out = []
    for obj in meshes:
        ev = obj.evaluated_get(depsgraph)
        data = ev.to_mesh()
        matrix = np.array(ev.matrix_world)
        co = np.empty(len(data.vertices) * 3)
        data.vertices.foreach_get('co', co)
        co = co.reshape((-1, 3))
        out.append(co @ matrix[:3, :3].T + matrix[:3, 3])
        ev.to_mesh_clear()
    return np.vstack(out)


def bone_world(name):
    bone = arm.pose.bones.get(name)
    return list(arm.matrix_world @ bone.head) if bone else None


def set_action(action, frame):
    arm.animation_data_create()
    arm.animation_data.action = action
    slots = getattr(action, 'slots', None)
    if slots and len(slots):
        arm.animation_data.action_slot = slots[0]
    scene.frame_set(int(frame), subframe=frame - int(frame))
    bpy.context.view_layer.update()


actions = list(bpy.data.actions)
if arm.animation_data:
    for track in arm.animation_data.nla_tracks:
        track.mute = True
    arm.animation_data.action = None
arm.data.pose_position = 'REST'
rest = evaluated_points()
report = {
    'source': str(SOURCE),
    'objects': [(o.name, o.type, [round(v, 5) for v in o.matrix_world.translation], [round(v, 5) for v in o.matrix_world.to_scale()]) for o in scene.objects],
    'triangles': sum(sum(len(p.vertices) - 2 for p in o.data.polygons) for o in meshes),
    'vertices': sum(len(o.data.vertices) for o in meshes),
    'materials': [m.name for m in bpy.data.materials],
    'images': [(i.name, list(i.size)) for i in bpy.data.images],
    'rest_bounds': [rest.min(axis=0).tolist(), rest.max(axis=0).tolist()],
    'rest_height': float(np.ptp(rest[:, 2])),
    'fps': scene.render.fps,
    'bones': [(b.name, b.parent.name if b.parent else None, [round(v, 4) for v in arm.matrix_world @ b.head_local]) for b in arm.data.bones],
    'clips': {},
}
arm.data.pose_position = 'POSE'
for action in actions:
    a, b = action.frame_range
    set_action(action, a)
    first_hips = None
    lows, highs, roots, hips = [], [], [], []
    for frame in np.linspace(a, b, 13):
        set_action(action, float(frame))
        p = evaluated_points()
        lows.append(float(p[:, 2].min()))
        highs.append(float(p[:, 2].max()))
        roots.append(bone_world('root') or [0, 0, 0])
        hips.append(bone_world('pelvis') or bone_world('mixamorig:Hips'))
    report['clips'][action.name] = {
        'frames': [a, b], 'seconds': (b - a) / scene.render.fps,
        'lowest': min(lows), 'highest': max(highs), 'low_start_end': [lows[0], lows[-1]],
        'root_start_end': [roots[0], roots[-1]], 'pelvis_start_end': [hips[0], hips[-1]],
        'pelvis_range': [np.min(np.array(hips), axis=0).tolist(), np.max(np.array(hips), axis=0).tolist()],
    }
(OUT / 'rig.json').write_text(json.dumps(report, indent=1) + '\n')
print('RIG', json.dumps({k: report[k] for k in ('objects', 'triangles', 'vertices', 'rest_bounds', 'rest_height', 'images', 'fps')}), flush=True)

# Pictures: a cheap neutral render, the texture as it is, on grey.
scene.render.engine = 'CYCLES'
scene.cycles.device = 'CPU'
scene.cycles.samples = 24
scene.cycles.use_denoising = False
scene.render.resolution_x, scene.render.resolution_y = 700, 1000
scene.render.film_transparent = False
world = bpy.data.worlds.new('w')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (.35, .35, .37, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 1.0
scene.world = world
cam_data = bpy.data.cameras.new('c')
cam = bpy.data.objects.new('c', cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
sun = bpy.data.objects.new('s', bpy.data.lights.new('s', 'SUN'))
scene.collection.objects.link(sun)
sun.data.energy = 2.5
sun.rotation_euler = (math.radians(55), 0, math.radians(30))


def look(at, direction, distance, lens=60):
    cam_data.lens = lens
    direction = Vector(direction).normalized()
    cam.location = Vector(at) + direction * distance
    cam.rotation_euler = (-direction).to_track_quat('-Z', 'Y').to_euler()
    sun.rotation_euler = (direction + Vector((.4, 0, .9))).normalized().to_track_quat('-Z', 'Y').to_euler()


def shot(name):
    scene.render.filepath = str(OUT / (name + '.png'))
    bpy.ops.render.render(write_still=True)


# Which way the toes point decides the facing.
arm.data.pose_position = 'REST'
foot, toe = arm.data.bones.get('foot_l') or arm.data.bones.get('mixamorig:LeftFoot'), arm.data.bones.get('ball_l') or arm.data.bones.get('mixamorig:LeftToeBase')
forward = (arm.matrix_world.to_3x3() @ (toe.head_local - foot.head_local)) if foot and toe else Vector((0, -1, 0))
forward.z = 0
forward.normalize()
side = forward.cross(Vector((0, 0, 1))).normalized()
report['forward'] = list(forward)
low, high = rest.min(axis=0), rest.max(axis=0)
height = high[2] - low[2]
mid = Vector(((low[0] + high[0]) / 2, (low[1] + high[1]) / 2, (low[2] + high[2]) / 2))
reach = height * 2.1
look(mid, forward, reach)
shot('rest_front')
look(mid, side, reach)
shot('rest_side')
look(mid, -forward, reach)
shot('rest_back')
head_bone = arm.data.bones.get('head') or arm.data.bones.get('mixamorig:Head')
head_at = arm.matrix_world @ head_bone.head_local
head_tail = arm.matrix_world @ head_bone.tail_local
report['head'] = {'head': list(head_at), 'tail': list(head_tail)}
face = (head_at + head_tail) / 2
scene.render.resolution_x, scene.render.resolution_y = 1000, 1000
look(face, forward, height * .55, 85)
shot('head_front')
look(face, forward + side * .8, height * .55, 85)
shot('head_quarter')
look(face, side, height * .55, 85)
shot('head_side')
(OUT / 'rig.json').write_text(json.dumps(report, indent=1) + '\n')

arm.data.pose_position = 'POSE'
scene.render.resolution_x, scene.render.resolution_y = 500, 700
scene.cycles.samples = 12
for action in actions:
    a, b = action.frame_range
    for label, frame in (('a', a + (b - a) * .25), ('b', a + (b - a) * .6)):
        set_action(action, float(frame))
        p = evaluated_points()
        lo, hi = p.min(axis=0), p.max(axis=0)
        centre = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2))
        look(centre, (forward + side * .7), max(hi[2] - lo[2], hi[0] - lo[0], hi[1] - lo[1]) * 2.4)
        shot('clip_' + action.name.replace('.', '_').replace(' ', '_')[:24] + '_' + label)
print('INSPECT DONE', flush=True)
