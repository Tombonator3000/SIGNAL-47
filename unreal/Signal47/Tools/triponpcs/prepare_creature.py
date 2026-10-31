"""Normalize the existing skinned Tripo alien without baking away its rig.

Run through the project's serialized Blender guard. Originals are read-only;
all FBX, blend and manifest output stays in the private model directory.
"""
from pathlib import Path
import json
import os
import sys

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prepare as source_tools


def main():
    root = source_tools.ROOT
    output = Path(os.environ.get('S47_TRIPO_CREATURE_DIR', str(root / 'unreal-creature-2026-10-31')))
    output.mkdir(parents=True, exist_ok=True)
    row = next(r for r in source_tools.ASSETS if r['role'] == 'tall_one')
    source = Path(row['fbx'])
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1
    bpy.ops.import_scene.fbx(filepath=str(source), use_anim=False)
    arms = [o for o in bpy.context.scene.objects if o.type == 'ARMATURE']
    if len(arms) != 1:
        raise RuntimeError('Expected exactly one original Tripo skeleton')
    arm = arms[0]
    canonical, facing = source_tools.canonical_rotation(arm)
    points = source_tools.world_points(canonical)
    scale = 2.25 / (points[:, 2].max() - points[:, 2].min())
    pelvis = source_tools.bone(arm, 'Hips')
    if not pelvis:
        raise RuntimeError('Tall One is missing its pelvis bone')
    hip = canonical @ arm.matrix_world @ pelvis.head
    centre = Vector((hip.x * scale, hip.y * scale, points[:, 2].min() * scale))
    transform = Matrix.Translation(-centre) @ Matrix.Scale(scale, 4) @ canonical
    objects = [o for o in bpy.context.scene.objects if o.type in {'ARMATURE', 'MESH'}]
    original_matrices = {o.name: o.matrix_world.copy() for o in objects}
    # Parent-first, with all original worlds retained before moving the skeleton.
    for obj in sorted(objects, key=lambda o: 0 if o.type == 'ARMATURE' else 1):
        obj.matrix_world = transform @ original_matrices[obj.name]
        obj.animation_data_clear()
    bpy.context.view_layer.update()
    meshes = [o for o in objects if o.type == 'MESH']
    for mesh in meshes:
        if not mesh.vertex_groups or not any(m.type == 'ARMATURE' for m in mesh.modifiers):
            raise RuntimeError('Source skinning was lost: ' + mesh.name)
        if not mesh.data.uv_layers:
            raise RuntimeError('Source UV atlas missing')
    name = 'SK_Tripo_Pale'
    arm.name = 'TripoPaleRig'
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    fbx = output / (name + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True,
        object_types={'MESH', 'ARMATURE'}, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z',
        add_leaf_bones=False, use_armature_deform_only=False,
        bake_anim=False, mesh_smooth_type='FACE', use_tspace=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / (name + '.blend')))
    result = {'name': name, 'fbx': str(fbx), 'source_fbx': str(source),
              'height_m': 2.25, 'bones': len(arm.data.bones),
              'facing_method': facing, 'skeletal': True,
              'material': '/Game/Licensed/TripoNPC/Materials/M_Tripo_tall_one'}
    (output / 'manifest.json').write_text(json.dumps(result, indent=2) + '\n')
    print('TRIPO SKINNED READY ' + json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
