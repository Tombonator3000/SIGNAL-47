"""Prepare Tom's nine Tripo models of 9 November (TRIPO-NEW-20261108) as private static meshes for Unreal.

blender -b -t 4 --python prepare.py -- [--only singer,zet] [--previews]

Paid Tripo plan: the GLBs, the prepared files and the native assets stay out of Git. Sources are read from the private copy in
S47_TRIPO_NEW_ROOT (default ~/Documents/ChatGPT/Signal 47/modeller/tripo-new-2026-11-09/raw, copied from Tom's desktop with a
SHA256SUMS beside them); everything written goes to <root>/prepared. Nothing is read from anywhere else.

The recipe is triponpcs/prepare.py's: a rigged figure is baked in a pose (here the first frame of its own idle clip), scaled to its
height with the foot pivot under the pelvis, and written as one static FBX with its base atlas; a vehicle or prop is scaled to its
size with the pivot on the ground at the centre. Blender metres, Z up; a figure (and the walkie's face) looks along -Y, a vehicle's
front is +Y. The FBX recipe then gives Unreal centimetres, Z up: a figure faces +Y (Three +Z, as the TripoNPC figures and
S47Happenings' stand-ins), a vehicle's front -Y (Three -Z, as RvParts and the TripoFleet trucks).

The manifest is the one triponpcs/import_assets.py reads (S47_TRIPO_STATIC_DIR=<root>/prepared S47_TRIPO_DEST=/Game/Licensed/TripoNew).
A figure also gets <name>_eyes.blend: the baked mesh and a one-bone armature whose head bone stands where the posed head is, for
triponpcs/measure_eyes.py; its dot_world, as (100x, -100y, 100z), is the eye in the static mesh's own centimetres.
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

ROOT = Path(os.environ.get('S47_TRIPO_NEW_ROOT', str(Path.home() / 'Documents/ChatGPT/Signal 47/modeller/tripo-new-2026-11-09')))
RAW = ROOT / 'raw'
OUTPUT = ROOT / 'prepared'

# height_m: the standing figure's height with all it wears; length_m: a vehicle's or prop's longest horizontal; height_m for a prop
# that stands up (the walkie). squash_x: a vehicle's width taken in (Tripo made both a little broad for their length). front: where
# the raw model's front is after the glTF import (figures: -Y, as Tripo's rigs come).
MODELS = [
    {'id': 'singer', 'src': 'Elvis.glb', 'name': 'SM_Tripo_Singer', 'kind': 'figure', 'height_m': 1.83, 'clip': 'idle', 'roughness': .6,
     'label': 'Big man in a white suit (an Elvis lookalike, never named)'},
    {'id': 'bigfoot', 'src': 'Biggie.glb', 'name': 'SM_Tripo_Bigfoot', 'kind': 'figure', 'height_m': 2.45, 'clip': 'idle', 'roughness': .85,
     'label': 'Something tall and hairy (Bigfoot, never named)'},
    {'id': 'zet', 'src': 'zet.glb', 'name': 'SM_Tripo_Zet', 'kind': 'figure', 'height_m': 1.40, 'clip': 'idle', 'roughness': .7, 'eyes': True,
     'label': "Zet from Tom's book: small and gray, shirt and hat"},
    {'id': 'nordic', 'src': 'nordic.glb', 'name': 'SM_Tripo_Nordic', 'kind': 'figure', 'height_m': 1.96, 'clip': 'idle', 'roughness': .5, 'eyes': True,
     'label': 'Nordic alien: tall, pale, long fair hair, a gray suit'},
    {'id': 'cosmic', 'src': 'cosmic.glb', 'name': 'SM_Tripo_Cosmic', 'kind': 'figure', 'height_m': 2.30, 'clip': 'idle', 'roughness': .35, 'eyes': True,
     'label': 'Cosmic alien: very tall and thin, blue-green and shining'},
    {'id': 'motorhome', 'src': 'camper.glb', 'name': 'SM_Tripo_Motorhome', 'kind': 'vehicle', 'length_m': 7.3, 'squash_x': .86, 'front': '-Y', 'roughness': .75,
     'label': 'Old motorhome, cream and brown (a nod to a TV show, no name or logo)'},
    {'id': 'convertible', 'src': 'caddilac.glb', 'name': 'SM_Tripo_PinkConvertible', 'kind': 'vehicle', 'length_m': 5.7, 'squash_x': .88, 'front': '-Y', 'roughness': .38,
     'label': 'Pink convertible with fins (no make named)'},
    {'id': 'walkie', 'src': 'walkie talkie 3d model.glb', 'name': 'SM_Tripo_Walkie', 'kind': 'prop', 'height_m': .34, 'front': '-Y', 'roughness': .55,
     'label': 'Walkie-talkie, face toward -Y, antenna up'},
    {'id': 'parcel', 'src': 'pakke tripo3d-1.glb', 'name': 'SM_Tripo_Parcel', 'kind': 'prop', 'length_m': .31, 'front': '-Y', 'roughness': .9,
     'base_color': [.38, .27, .16], 'label': 'The small package Tom made (no texture in the file)'},
]


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def points_of(objects):
    graph = bpy.context.evaluated_depsgraph_get()
    out = []
    for obj in objects:
        ev = obj.evaluated_get(graph)
        me = ev.to_mesh()
        co = np.empty(len(me.vertices) * 3)
        me.vertices.foreach_get('co', co)
        m = np.array(ev.matrix_world)
        out.append(co.reshape((-1, 3)) @ m[:3, :3].T + m[:3, 3])
        ev.to_mesh_clear()
    return np.vstack(out)


def front_turn(front):
    """The rotation that brings the raw front onto the wanted one."""
    return {'-Y': 0., '+Y': math.pi, '+X': -math.pi / 2, '-X': math.pi / 2}[front]


def base_image(material):
    if not material or not material.node_tree:
        return None
    bsdf = next((n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if not bsdf:
        return None
    seen = set()

    def walk(socket):
        if not socket.is_linked:
            return None
        node = socket.links[0].from_node
        if node.as_pointer() in seen:
            return None
        seen.add(node.as_pointer())
        if node.type == 'TEX_IMAGE' and node.image:
            return node.image
        for i in node.inputs:
            found = walk(i)
            if found:
                return found
        return None
    return walk(bsdf.inputs['Base Color'])


def save_texture(image, target, limit=2048):
    copy = image.copy()
    copy.pixels[:] = image.pixels[:]
    w, h = copy.size
    scale = min(1, limit / max(w, h))
    if scale < 1:
        copy.scale(round(w * scale), round(h * scale))
    copy.filepath_raw = str(target)
    copy.file_format = 'PNG'
    copy.save()
    bpy.data.images.remove(copy)


def use_clip(arm, start):
    found = [a for a in bpy.data.actions if a.name.startswith(start + '.') or a.name == start]
    if len(found) != 1:
        raise RuntimeError('Clip %s not found once in %s' % (start, [a.name for a in bpy.data.actions]))
    action = found[0]
    arm.animation_data_create()
    for track in list(arm.animation_data.nla_tracks):
        arm.animation_data.nla_tracks.remove(track)
    arm.animation_data.action = action
    if len(action.slots):
        arm.animation_data.action_slot = action.slots[0]
    bpy.context.scene.frame_set(int(round(action.frame_range[0])))
    bpy.context.view_layer.update()
    return action.name


def prepare(row, previews):
    source = RAW / row['src']
    out = OUTPUT / row['id']
    out.mkdir(parents=True, exist_ok=True)
    before = sha(source)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1
    scene.render.fps = 24
    bpy.ops.import_scene.gltf(filepath=str(source))
    arms = [o for o in scene.objects if o.type == 'ARMATURE']
    if row['kind'] == 'figure':
        meshes = [o for o in scene.objects if o.type == 'MESH' and any(m.type == 'ARMATURE' for m in o.modifiers)]
        if len(arms) != 1 or len(meshes) != 1:
            raise RuntimeError('%s: expected one rig and one skinned mesh' % row['id'])
        arm = arms[0]
        clip = use_clip(arm, row['clip'])
    else:
        meshes = [o for o in scene.objects if o.type == 'MESH']
        arm, clip = None, None
        if arms:
            raise RuntimeError('%s: a rig where none was expected' % row['id'])
    raw = points_of(meshes)
    turn = Matrix.Rotation(front_turn(row.get('front', '-Y')) + (math.pi if row['kind'] == 'vehicle' else 0.), 4, 'Z')
    turned = raw @ np.array(turn)[:3, :3].T
    lo, hi = turned.min(axis=0), turned.max(axis=0)
    if 'height_m' in row:
        factor = row['height_m'] / (hi[2] - lo[2])
    else:
        factor = row['length_m'] / max(hi[0] - lo[0], hi[1] - lo[1])
    squash = Matrix.Diagonal((row.get('squash_x', 1.), 1., 1., 1.))
    if arm:
        pelvis = arm.matrix_world @ arm.pose.bones['pelvis'].head
        pelvis = turn @ pelvis
        centre = Vector((pelvis.x * factor, pelvis.y * factor, lo[2] * factor))
    else:
        centre = Vector(((lo[0] + hi[0]) / 2 * factor, (lo[1] + hi[1]) / 2 * factor, lo[2] * factor))
    transform = squash @ Matrix.Translation(-centre) @ Matrix.Scale(factor, 4) @ turn

    # The base atlas, before the scene is changed.
    material_src = meshes[0].material_slots[0].material if meshes[0].material_slots else None
    image = base_image(material_src)
    textures = {}
    if image:
        save_texture(image, out / 'base_color.png')
        textures['base_color'] = str((out / 'base_color.png').relative_to(OUTPUT))
    elif 'base_color' not in row:
        raise RuntimeError('%s: no base atlas and no colour given' % row['id'])

    graph = bpy.context.evaluated_depsgraph_get()
    baked = []
    for obj in meshes:
        ev = obj.evaluated_get(graph)
        mesh = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=graph)
        mesh.transform(transform @ obj.matrix_world)
        if transform.determinant() < 0:
            mesh.flip_normals()
        bake = bpy.data.objects.new('StaticBake', mesh)
        scene.collection.objects.link(bake)
        baked.append(bake)
    head = None
    if arm:
        head = transform @ (arm.matrix_world @ arm.pose.bones['head'].head)
    for obj in [o for o in scene.objects if o not in baked]:
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in baked:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = baked[0]
    if len(baked) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = row['name']
    if not obj.data.uv_layers:
        # The package came without UVs or a texture: one simple projection, so the importer's lightmap UV has a channel to start from.
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=.02)
        bpy.ops.object.mode_set(mode='OBJECT')
    obj.data.uv_layers[0].name = 'S47_Atlas'
    material_name = 'M_Tripo_' + row['id']
    keep = material_src if image else None
    obj.data.materials.clear()
    if keep:
        keep.name = material_name
        obj.data.materials.append(keep)
    else:
        mat = bpy.data.materials.new(material_name)
        mat.use_nodes = True
        bsdf = mat.node_tree.nodes.get('Principled BSDF')
        bsdf.inputs['Base Color'].default_value = (*row['base_color'], 1)
        obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.material_index = 0
    tri = obj.modifiers.new('StaticTriangles', 'TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=tri.name)
    pts = np.array([tuple(v.co) for v in obj.data.vertices])
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    if abs(lo[2]) > 1e-4:
        raise RuntimeError('%s: not on the floor (%.4f)' % (row['id'], lo[2]))
    fbx = out / (row['name'] + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(fbx), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False,
                             mesh_smooth_type='FACE', use_tspace=True)
    for im in bpy.data.images:
        if im.source == 'FILE' or im.packed_file is None:
            try:
                im.pack()
            except RuntimeError:
                pass
    bpy.ops.wm.save_as_mainfile(filepath=str(out / (row['name'] + '.blend')))
    eyes_blend = None
    if head is not None:
        # One bone where the posed head stands, for measure_eyes.py (it looks for the bone 'head' in the first armature).
        ref = bpy.data.armatures.new('EyeRef')
        rig = bpy.data.objects.new('EyeRef', ref)
        scene.collection.objects.link(rig)
        bpy.context.view_layer.objects.active = rig
        bpy.ops.object.mode_set(mode='EDIT')
        b = ref.edit_bones.new('head')
        b.head, b.tail = head, head + Vector((0, 0, .1))
        bpy.ops.object.mode_set(mode='OBJECT')
        eyes_blend = out / (row['name'] + '_eyes.blend')
        bpy.ops.wm.save_as_mainfile(filepath=str(eyes_blend), copy=True)
        bpy.data.objects.remove(rig, do_unlink=True)
    if previews:
        render_views(obj, out / 'previews', row)
    if sha(source) != before:
        raise RuntimeError('Original source changed')
    result = {
        'role': row['id'], 'name': row['name'], 'label': row['label'], 'kind': row['kind'],
        'mesh_path': '/Game/Licensed/TripoNew/' + row['name'],
        'source': str(source), 'source_sha256': before, 'fbx': str(fbx.relative_to(OUTPUT)),
        'blend': str((out / (row['name'] + '.blend')).relative_to(OUTPUT)),
        'eyes_blend': str(eyes_blend.relative_to(OUTPUT)) if eyes_blend else None,
        'pose': 'clip %s first frame' % clip if clip else 'source', 'clip': clip,
        'height_m': float(hi[2] - lo[2]), 'seat_z_m': None,
        'size_m': [float(x) for x in (hi - lo)], 'factor': factor, 'squash_x': row.get('squash_x', 1.),
        'bounds_blender_m': [lo.tolist(), hi.tolist()], 'triangles': len(obj.data.polygons),
        'head_blender_m': list(head) if head is not None else None,
        'coordinates': 'Blender metres/Z-up; figure faces -Y, vehicle front +Y; UE cm/Z-up via the triponpcs FBX recipe',
        'material': {'name': material_name, 'roughness': row.get('roughness', .7), 'metallic': 0, 'specular': .4,
                     'normal_strength': 1, 'two_sided': True, 'textures': textures,
                     **({'base_color_linear': row['base_color'] + [1]} if not textures else {})},
    }
    print('TRIPO NEW READY ' + json.dumps({k: result[k] for k in ('role', 'height_m', 'size_m', 'triangles', 'pose')}), flush=True)
    return result


def render_views(obj, folder, row):
    folder.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'TEXTURE'
    scene.render.resolution_x = scene.render.resolution_y = 420
    pts = np.array([tuple(v.co) for v in obj.data.vertices])
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    c = Vector(((lo + hi) / 2).tolist())
    r = float(np.linalg.norm(hi - lo))
    cam = bpy.data.objects.new('PrevCam', bpy.data.cameras.new('PrevCam'))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = 'ORTHO'
    cam.data.ortho_scale = r * 1.02
    # A floor line: a thin plane at Z=0.
    for view, d in (('minusY', Vector((0, -1, 0))), ('plusY', Vector((0, 1, 0))), ('minusX', Vector((-1, 0, 0))), ('three', Vector((-.7, -1, .5)).normalized())):
        cam.location = c + d * r * 2
        cam.rotation_euler = (-d).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(folder / (view + '.png'))
        bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--only')
    parser.add_argument('--previews', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
    wanted = set(args.only.split(',')) if args.only else {m['id'] for m in MODELS}
    unknown = wanted - {m['id'] for m in MODELS}
    if unknown:
        raise RuntimeError('Unknown models: %s' % sorted(unknown))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / 'manifest.json'
    rows = {r['role']: r for r in json.loads(path.read_text())['assets']} if path.exists() else {}
    for row in MODELS:
        if row['id'] in wanted:
            rows[row['id']] = prepare(row, args.previews)
            manifest = {'version': 1, 'source': "Tom's Tripo models of 9 November 2026 (paid plan, private)",
                        'coordinates': 'Blender metres/Z-up; imported UE cm/Z-up, figure facing +Y, vehicle front -Y; foot pivot z=0',
                        'static_only': True, 'assets': [rows[m['id']] for m in MODELS if m['id'] in rows]}
            path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n')
    print('TRIPO NEW PREPARED %d' % len(rows), flush=True)


if __name__ == '__main__':
    main()
