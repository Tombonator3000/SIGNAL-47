"""Bake the existing rigged Tripo sources into private static mannequin meshes.

Run in Blender, for example: blender -b -t 2 --python prepare.py -- --only dale
No source FBX or original rig is written. Outputs stay outside public Git.
"""
from pathlib import Path
import argparse
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector


HERE = Path(__file__).resolve().parent
DEFAULT_ROOT = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-npc-2026-10-29'
ROOT = Path(os.environ.get('S47_TRIPO_SOURCE_ROOT', str(DEFAULT_ROOT)))
OUTPUT = Path(os.environ.get('S47_TRIPO_STATIC_DIR', str(ROOT / 'unreal-static-2026-10-30')))
ROLES = json.loads((HERE / 'roles.json').read_text())
ASSETS = json.loads((ROOT / 'revision-v2/active-assets.json').read_text())['assets']


def bone(armature, name):
    """The selected exports use either Mixamo or UE mannequin bone names."""
    aliases = {
        'Hips': 'pelvis', 'LeftUpLeg': 'thigh_l', 'RightUpLeg': 'thigh_r',
        'LeftLeg': 'calf_l', 'RightLeg': 'calf_r',
        'LeftFoot': 'foot_l', 'RightFoot': 'foot_r',
        'LeftToeBase': 'ball_l', 'RightToeBase': 'ball_r',
        'LeftArm': 'upperarm_l', 'RightArm': 'upperarm_r',
        'LeftForeArm': 'lowerarm_l', 'RightForeArm': 'lowerarm_r',
    }
    for key in ('mixamorig:' + name, name, aliases.get(name, '')):
        found = armature.pose.bones.get(key)
        if found:
            return found
    return None


def canonical_rotation(armature):
    if not armature:
        return Matrix.Identity(4), 'FBX authored forward retained'
    vectors = []
    for side in ('Left', 'Right'):
        foot, toe = bone(armature, side + 'Foot'), bone(armature, side + 'ToeBase')
        if foot and toe:
            direction = armature.matrix_world.to_3x3() @ (toe.head - foot.head)
            direction.z = 0
            if direction.length > 1e-5:
                vectors.append(direction.normalized())
    if vectors:
        forward = sum(vectors, Vector())
        return Matrix.Rotation(-math.pi / 2 - math.atan2(forward.y, forward.x), 4, 'Z'), 'paired ankle-to-toe direction'
    head = armature.pose.bones.get('tripo::Head_0')
    spine = armature.pose.bones.get('tripo::Spine_0')
    if head and spine:
        forward = armature.matrix_world.to_3x3() @ (head.head - spine.head)
        forward.z = 0
        if forward.length > 1e-5:
            return Matrix.Rotation(-math.pi / 2 - math.atan2(forward.y, forward.x), 4, 'Z'), 'quadruped spine-to-head direction'
    return Matrix.Identity(4), 'FBX authored forward retained'


def aim(armature, name, target, canonical):
    part = bone(armature, name)
    if part is None:
        raise RuntimeError('Missing pose bone ' + name)
    direction = part.tail - part.head
    desired_world = canonical.to_3x3().inverted() @ Vector(target)
    desired_local = armature.matrix_world.to_3x3().inverted() @ desired_world
    delta = direction.normalized().rotation_difference(desired_local.normalized())
    transform = (delta @ part.matrix.to_quaternion()).to_matrix().to_4x4()
    transform.translation = part.head
    part.matrix = transform
    bpy.context.view_layer.update()


def pose(armature, kind, canonical):
    if kind == 'source':
        return
    if not armature or not bone(armature, 'Hips'):
        raise RuntimeError('Expected a humanoid rig for ' + kind)
    for side, sign in (('Left', 1), ('Right', -1)):
        if kind == 'seated':
            aim(armature, side + 'UpLeg', (sign * .06, -1, -.04), canonical)
            aim(armature, side + 'Leg', (0, .04, -1), canonical)
            aim(armature, side + 'Foot', (0, -1, -.04), canonical)
            aim(armature, side + 'Arm', (sign * .12, -.15, -1), canonical)
            aim(armature, side + 'ForeArm', (-sign * .15, -.78, -.32), canonical)
        else:
            # Bake relaxed arms, not a continuously evaluated animation rig.
            aim(armature, side + 'Arm', (sign * .12, -.02, -1), canonical)
            aim(armature, side + 'ForeArm', (sign * .025, -.10, -1), canonical)


def evaluated_objects():
    graph = bpy.context.evaluated_depsgraph_get()
    return [(obj, obj.evaluated_get(graph)) for obj in bpy.context.scene.objects if obj.type == 'MESH']


def world_points(canonical):
    result = []
    for obj, evaluated in evaluated_objects():
        mesh = evaluated.to_mesh()
        transform = canonical @ obj.matrix_world
        result.extend(tuple(transform @ v.co) for v in mesh.vertices)
        evaluated.to_mesh_clear()
    return np.asarray(result, dtype=float)


def textures(source, output):
    files = [p for p in source.parent.rglob('*') if p.is_file() and p.suffix.lower() in ('.png', '.jpg', '.jpeg')]
    result = {}
    predicates = {
        'base_color': lambda p: any(t in p.stem.lower() for t in ('basecolor', 'base_color', 'tripo_rgb', 'albedo')),
        'normal': lambda p: 'normal' in p.stem.lower(),
        'roughness': lambda p: 'roughness' in p.stem.lower(),
        'metallic': lambda p: 'metallic' in p.stem.lower(),
    }
    for role, predicate in predicates.items():
        matches = [p for p in files if predicate(p)]
        if role == 'base_color' and not matches and len(files) == 1:
            matches = files
        if not matches:
            continue
        if len(matches) != 1:
            raise RuntimeError('Ambiguous texture ' + role + ': ' + str(matches))
        im = bpy.data.images.load(str(matches[0]), check_existing=False)
        im.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
        width, height = im.size
        scale = min(1, 2048 / max(width, height))
        if scale < 1:
            im.scale(round(width * scale), round(height * scale))
        if role == 'normal':
            # As in prepare_dale_prop: convert Tripo's OpenGL tangent normals to DirectX.
            pixels = np.empty(len(im.pixels), dtype=np.float32)
            im.pixels.foreach_get(pixels)
            pixels[1::4] = 1 - pixels[1::4]
            im.pixels.foreach_set(pixels)
        target = output / (role + '.png')
        im.filepath_raw = str(target)
        im.file_format = 'PNG'
        im.save()
        bpy.data.images.remove(im)
        result[role] = str(target.relative_to(OUTPUT))
    if 'base_color' not in result:
        raise RuntimeError('Missing base colour for ' + str(source))
    return result


def prepare(row):
    role = row['role']
    spec = ROLES[role]
    source = Path(row['fbx'])
    output = OUTPUT / role
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.import_scene.fbx(filepath=str(source), use_anim=False)
    armatures = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
    if len(armatures) > 1:
        raise RuntimeError('Multiple source armatures for ' + role)
    armature = armatures[0] if armatures else None
    canonical, facing_method = canonical_rotation(armature)
    rest = world_points(canonical)
    standing_height = rest[:, 2].max() - rest[:, 2].min()
    factor = spec['height_m'] / standing_height
    pose(armature, spec['pose'], canonical)
    points = world_points(canonical) * factor
    low, high = points.min(axis=0), points.max(axis=0)
    pelvis = bone(armature, 'Hips') if armature else None
    if pelvis:
        pelvis_position = canonical @ armature.matrix_world @ pelvis.head
        centre = Vector((pelvis_position.x * factor, pelvis_position.y * factor, low[2]))
        seat_height = pelvis_position.z * factor - low[2]
    else:
        centre = Vector(((low[0] + high[0]) / 2, (low[1] + high[1]) / 2, low[2]))
        seat_height = None
    baked = []
    for original, evaluated in evaluated_objects():
        mesh = bpy.data.meshes.new_from_object(evaluated, preserve_all_data_layers=True, depsgraph=bpy.context.evaluated_depsgraph_get())
        if not mesh.uv_layers:
            raise RuntimeError('Missing UV in ' + role)
        transform = Matrix.Translation(-centre) @ Matrix.Scale(factor, 4) @ canonical @ original.matrix_world
        mesh.transform(transform)
        obj = bpy.data.objects.new('StaticBake', mesh)
        bpy.context.collection.objects.link(obj)
        baked.append(obj)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in baked:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = baked[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    name = 'SM_Tripo_' + role
    obj.name = name
    obj.data.uv_layers[0].name = 'S47_Atlas'
    obj.data.materials.clear()
    material = bpy.data.materials.new('M_Tripo_' + role)
    obj.data.materials.append(material)
    for polygon in obj.data.polygons:
        polygon.material_index = 0
    tri = obj.modifiers.new('StaticTriangles', 'TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=tri.name)
    for other in list(bpy.context.scene.objects):
        if other != obj:
            bpy.data.objects.remove(other, do_unlink=True)
    maps = textures(source, output)
    # Keep the .blend preview self-contained with the exact base atlas.
    material.use_nodes = True
    principled = material.node_tree.nodes.get('Principled BSDF')
    texture = material.node_tree.nodes.new('ShaderNodeTexImage')
    texture.image = bpy.data.images.load(str(OUTPUT / maps['base_color']))
    material.node_tree.links.new(texture.outputs['Color'], principled.inputs['Base Color'])
    principled.inputs['Roughness'].default_value = .68
    texture.image.pack()
    fbx = output / (name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True,
        object_types={'MESH'}, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', bake_anim=False,
        mesh_smooth_type='FACE', use_tspace=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (name + '.blend')))
    points = np.asarray([tuple(v.co) for v in obj.data.vertices])
    result = {
        'role': role, 'name': name, 'label': row['label'],
        'mesh_path': '/Game/Licensed/TripoNPC/' + name,
        'source_fbx': str(source), 'fbx': str(fbx.relative_to(OUTPUT)),
        'pose': spec['pose'], 'standing_height_m': spec['height_m'],
        'height_m': float(points[:, 2].max() - points[:, 2].min()),
        'seat_z_m': seat_height if spec['pose'] == 'seated' else None,
        'bounds_blender_m': [points.min(axis=0).tolist(), points.max(axis=0).tolist()],
        'triangles': len(obj.data.polygons), 'facing_method': facing_method,
        'material': {'name': 'M_Tripo_' + role, 'roughness': .68, 'metallic': 0,
                     'normal_strength': 1, 'unlit_black': spec.get('unlit_black', False), 'textures': maps},
    }
    print('TRIPO STATIC READY', role, json.dumps({k: result[k] for k in ('pose', 'height_m', 'seat_z_m', 'triangles', 'facing_method')}), flush=True)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--only', help='comma-separated active role ids; default all 14')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    requested = set(args.only.split(',')) if args.only else set(ROLES)
    if requested - set(ROLES):
        raise RuntimeError('Unknown roles: ' + str(requested - set(ROLES)))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / 'manifest.json'
    previous = json.loads(path.read_text())['assets'] if path.exists() else []
    rows = {r['role']: r for r in previous}
    for row in ASSETS:
        if row['role'] in requested:
            rows[row['role']] = prepare(row)
            manifest = {'version': 1, 'source': 'active image-based Tripo figures',
                        'coordinates': 'Blender metres/Z-up/forward -Y; imported UE cm/Z-up/forward +Y (Three +Z); foot pivot z=0',
                        'static_only': True, 'assets': [rows[a['role']] for a in ASSETS if a['role'] in rows]}
            path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print('TRIPO STATIC PREPARED', len(rows), str(path), flush=True)


if __name__ == '__main__':
    main()
