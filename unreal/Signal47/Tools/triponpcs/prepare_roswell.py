"""Prepare a real, skinned Tripo Roswell source; never manufacture a replacement.

Run in Blender through the project guard. Source and prepared assets stay private.
The original rig/skin is retained; bone aliases are renamed only in this copy.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector


DEFAULT_ROOT = Path.home() / 'Documents/ChatGPT/Signal 47/modeller/roswell-glimpse-20261031'
REQUIRED = ('pelvis', 'spine_01', 'head', 'upperarm_l', 'lowerarm_l', 'hand_l',
            'upperarm_r', 'lowerarm_r', 'hand_r', 'thigh_l', 'calf_l', 'foot_l',
            'thigh_r', 'calf_r', 'foot_r')
ALIASES = {'Hips': 'pelvis', 'Spine': 'spine_01', 'Spine1': 'spine_02',
           'Spine2': 'spine_03', 'Neck': 'neck_01', 'Head': 'head'}
for side, suffix in (('Left', 'l'), ('Right', 'r')):
    ALIASES.update({side + key: value + '_' + suffix for key, value in
                    (('Shoulder', 'clavicle'), ('Arm', 'upperarm'),
                     ('ForeArm', 'lowerarm'), ('Hand', 'hand'),
                     ('UpLeg', 'thigh'), ('Leg', 'calf'),
                     ('Foot', 'foot'), ('ToeBase', 'ball'))})


def args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', default=os.environ.get('S47_ROSWELL_SOURCE'))
    parser.add_argument('--output', default=os.environ.get('S47_ROSWELL_DIR', str(DEFAULT_ROOT / 'prepared')))
    parser.add_argument('--height', type=float, default=1.20)
    parser.add_argument('--forward', choices=('-Y', '+Y', '-X', '+X'), default='-Y',
                        help='Fallback Blender-space source forward if no paired toe bones exist')
    parser.add_argument('--bone-map', help='Optional JSON object of original bone name to runtime alias')
    parser.add_argument('--max-texture', type=int, default=2048)
    return parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def points(meshes):
    return np.asarray([tuple(obj.matrix_world @ v.co) for obj in meshes for v in obj.data.vertices], dtype=float)


def geometry_probes(xyz):
    # Asymmetric extrema catch mirrored/rotated LODs that a shared asset bound hides.
    directions = np.asarray([(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
                             (0, 0, 1), (0, 0, -1), (1, 2, 3), (-3, 2, 1), (2, -3, 1)])
    return [xyz[int(np.argmax(xyz @ direction))].tolist() for direction in directions]


def select(objects, active):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = active


def alias_bones(arm, custom):
    mapping = {}
    for bone in arm.data.bones:
        source = bone.name
        short = source.rsplit(':', 1)[-1]
        target = custom.get(source, ALIASES.get(short, source))
        if target != source:
            if arm.data.bones.get(target):
                raise RuntimeError('Bone alias collides: ' + source + ' -> ' + target)
            bone.name = target
            # Bone rename updates vertex groups for linked armature modifiers.
            mapping[source] = target
    missing = [name for name in REQUIRED if arm.data.bones.get(name) is None]
    if missing:
        raise RuntimeError('Unmapped humanoid bones ' + str(missing) + '; actual=' +
                           str([b.name for b in arm.data.bones]) + '. Supply --bone-map.')
    return mapping


def facing(arm, fallback):
    vectors = []
    for suffix in ('l', 'r'):
        foot, toe = arm.data.bones.get('foot_' + suffix), arm.data.bones.get('ball_' + suffix)
        if foot and toe:
            direction = arm.matrix_world.to_3x3() @ (toe.head_local - foot.head_local)
            direction.z = 0
            if direction.length > 1e-5:
                vectors.append(direction.normalized())
    direction = sum(vectors, Vector()) if vectors else {'-Y': Vector((0, -1, 0)),
                '+Y': Vector((0, 1, 0)), '-X': Vector((-1, 0, 0)), '+X': Vector((1, 0, 0))}[fallback]
    if direction.length < 1e-5:
        raise RuntimeError('Ambiguous paired foot direction; inspect original rig')
    return Matrix.Rotation(-math.pi / 2 - math.atan2(direction.y, direction.x), 4, 'Z'), \
        'paired ankle-to-toe direction' if vectors else 'explicit source forward ' + fallback


def image_link(socket, seen=None):
    """Find the image and channel feeding a glTF Principled input."""
    seen = set() if seen is None else seen
    if not socket or not socket.is_linked:
        return None
    link = socket.links[0]
    node = link.from_node
    if node.as_pointer() in seen:
        return None
    seen.add(node.as_pointer())
    if node.type == 'TEX_IMAGE' and node.image:
        return node.image, None
    if node.type in {'SEPARATE_COLOR', 'SEPRGB'}:
        result = image_link(node.inputs[0], seen)
        if result:
            channel = {'Red': 0, 'Green': 1, 'Blue': 2, 'R': 0, 'G': 1, 'B': 2}.get(link.from_socket.name)
            return result[0], channel
    for candidate in node.inputs:
        result = image_link(candidate, seen)
        if result:
            return result
    return None


def texture_copy(image, channel, target, role, max_size):
    # Work on a copy so the Blender source keeps OpenGL normals and packed originals.
    copy = image.copy()
    copy.name = target.stem
    copy.colorspace_settings.name = 'sRGB' if role == 'base_color' else 'Non-Color'
    width, height = copy.size
    if min(width, height) <= 0:
        raise RuntimeError('Unreadable source texture ' + image.name)
    scale = min(1, max_size / max(width, height))
    if scale < 1:
        copy.scale(max(1, round(width * scale)), max(1, round(height * scale)))
    pixels = np.empty(len(copy.pixels), dtype=np.float32)
    copy.pixels.foreach_get(pixels)
    rgba = pixels.reshape((-1, 4))
    if channel is not None:
        value = rgba[:, channel].copy()
        rgba[:, :3] = value[:, None]
        rgba[:, 3] = 1
    if role == 'normal':
        rgba[:, 1] = 1 - rgba[:, 1]  # OpenGL -> DirectX only in the UE texture.
    copy.pixels.foreach_set(pixels)
    copy.filepath_raw = str(target)
    copy.file_format = 'PNG'
    copy.save()
    bpy.data.images.remove(copy)


def material_rows(meshes, output, max_size):
    rows = []
    seen = set()
    for obj in meshes:
        for slot in obj.material_slots:
            material = slot.material
            if not material or material.as_pointer() in seen:
                continue
            seen.add(material.as_pointer())
            original_name = material.name
            name = 'M_Tripo_Roswell_' + str(len(rows)).zfill(2)
            material.name = name
            shader = next((n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None) if material.use_nodes else None
            if not shader:
                raise RuntimeError('Expected source PBR Principled material: ' + original_name)
            row = {'name': name, 'source_name': original_name,
                   'base_color': list(shader.inputs['Base Color'].default_value),
                   'roughness': float(shader.inputs['Roughness'].default_value),
                   'metallic': float(shader.inputs['Metallic'].default_value),
                   'specular': .5, 'normal_strength': 1., 'textures': {}}
            for role, key in (('base_color', 'Base Color'), ('normal', 'Normal'),
                              ('roughness', 'Roughness'), ('metallic', 'Metallic')):
                source = image_link(shader.inputs.get(key))
                if not source:
                    continue
                target = output / 'textures' / (name + '_' + role + '.png')
                texture_copy(source[0], source[1], target, role, max_size)
                row['textures'][role] = str(target.relative_to(output))
            if 'base_color' not in row['textures']:
                raise RuntimeError('No base atlas in ' + original_name)
            rows.append(row)
    if not rows:
        raise RuntimeError('No source PBR materials')
    return rows


def export(path, arm, meshes):
    select([arm] + meshes, arm)
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True,
        object_types={'MESH', 'ARMATURE'}, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z',
        add_leaf_bones=False, use_armature_deform_only=False,
        bake_anim=False, mesh_smooth_type='FACE', use_tspace=True)


def main():
    options = args()
    if not options.source:
        raise RuntimeError('--source or S47_ROSWELL_SOURCE must point to the actual downloaded rigged Tripo file')
    source = Path(options.source).expanduser().resolve(strict=True)
    output = Path(options.output).expanduser().resolve()
    if source.suffix.lower() not in {'.fbx', '.glb', '.gltf'} or output == source.parent:
        raise RuntimeError('Use a real GLB/FBX source and a separate prepared output directory')
    if not .5 <= options.height <= 1.4:
        raise RuntimeError('Unexpected small Roswell height')
    original_sha = sha(source)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'textures').mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    if source.suffix.lower() == '.fbx':
        bpy.ops.import_scene.fbx(filepath=str(source), use_anim=False)
    else:
        bpy.ops.import_scene.gltf(filepath=str(source))
    arms = [obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if len(arms) != 1 or not meshes:
        raise RuntimeError('Expected exactly one real Tripo skeleton and skinned meshes, not a static stand-in')
    arm = arms[0]
    arm.data.pose_position = 'REST'
    for obj in meshes:
        if not obj.vertex_groups or not any(m.type == 'ARMATURE' and m.object == arm for m in obj.modifiers):
            raise RuntimeError('Original skinning absent: ' + obj.name)
        if not obj.data.uv_layers:
            raise RuntimeError('Missing original atlas UVs: ' + obj.name)
    custom = json.loads(Path(options.bone_map).read_text()) if options.bone_map else {}
    aliases = alias_bones(arm, custom)
    rotation, facing_method = facing(arm, options.forward)
    original = points(meshes)
    rotated = np.asarray([tuple(rotation @ Vector(p)) for p in original])
    factor = options.height / (rotated[:, 2].max() - rotated[:, 2].min())
    hip = rotation @ arm.matrix_world @ arm.data.bones['pelvis'].head_local
    pivot = Vector((hip.x * factor, hip.y * factor, rotated[:, 2].min() * factor))
    transform = Matrix.Translation(-pivot) @ Matrix.Scale(factor, 4) @ rotation
    objects = [obj for obj in bpy.context.scene.objects if obj.type in {'MESH', 'ARMATURE', 'EMPTY'}]
    worlds = {obj: obj.matrix_world.copy() for obj in objects}
    # Parent first avoids changing children twice through their inherited world transform.
    def depth(obj):
        return 1 + depth(obj.parent) if obj.parent else 0
    for obj in sorted(objects, key=depth):
        obj.matrix_world = transform @ worlds[obj]
        obj.animation_data_clear()
    arm.name = 'TripoRoswellRig'
    arm.data.pose_position = 'POSE'
    for pose_bone in arm.pose.bones:
        pose_bone.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    bounds = points(meshes)
    if abs(bounds[:, 2].min()) > .0001 or abs(np.ptp(bounds[:, 2]) - options.height) > .0001:
        raise RuntimeError('Normalized height/ground pivot mismatch')
    materials = material_rows(meshes, output, options.max_texture)
    for image in bpy.data.images:
        if image.source != 'VIEWER' and image.size[0]:
            image.pack()
    blend = output / 'SK_Tripo_Roswell.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    lods = []
    for level, ratio in enumerate((1., .5, .2)):
        copies = []
        for obj in meshes:
            copy = obj.copy()
            copy.data = obj.data.copy()
            bpy.context.collection.objects.link(copy)
            copy.matrix_world = obj.matrix_world.copy()
            if level:
                select([copy], copy)
                modifier = copy.modifiers.new('RoswellLOD', 'DECIMATE')
                modifier.ratio = ratio
                modifier.use_collapse_triangulate = True
                # Rest pose, original weights retained by collapse; skeleton never reduced.
                bpy.ops.object.modifier_move_up(modifier=modifier.name)
                bpy.ops.object.modifier_apply(modifier=modifier.name)
            copies.append(copy)
        path = output / ('SK_Tripo_Roswell' + ('' if level == 0 else '_LOD' + str(level)) + '.fbx')
        export(path, arm, copies)
        xyz = points(copies)
        lods.append({'level': level, 'fbx': path.name, 'ratio': ratio,
                     'triangles': sum(sum(len(p.vertices) - 2 for p in obj.data.polygons) for obj in copies),
                     'bounds_blender_m': [xyz.min(axis=0).tolist(), xyz.max(axis=0).tolist()],
                     'probe_points_blender_m': geometry_probes(xyz)})
        for obj in copies:
            data = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(data)
    if sha(source) != original_sha:
        raise RuntimeError('Original source changed')
    result = {'version': 1, 'name': 'SK_Tripo_Roswell', 'role': 'roswell_glimpse',
              'source': str(source), 'source_sha256': original_sha, 'blend': blend.name,
              'height_m': options.height, 'skeletal': True,
              'coordinates': 'Blender metres/Z-up/forward -Y; UE centimetres/Z-up/forward +Y; feet pivot Z=0',
              'facing_method': facing_method, 'bone_aliases': aliases,
              'bones': [bone.name for bone in arm.data.bones],
              'materials': materials, 'lods': lods,
              'bounds_blender_m': [bounds.min(axis=0).tolist(), bounds.max(axis=0).tolist()]}
    (output / 'manifest.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n')
    print('ROSWELL PREPARED ' + json.dumps({'source': str(source), 'height_m': options.height,
          'bones': len(result['bones']), 'lod_triangles': [row['triangles'] for row in lods],
          'manifest': str(output / 'manifest.json')}), flush=True)


if __name__ == '__main__':
    main()
