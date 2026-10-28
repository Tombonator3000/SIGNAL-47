"""The waitress's 1986 uniform, built on her exact exported MetaHuman body.

blender -b -P uniform.py -- waitress_body.fbx out_dir

Light blue short-sleeved dress with white collar and cuffs, below-the-knee skirt draped by a short
cloth simulation, white bib apron. Every piece is skinned from the body's own weights and exported
with the same armature, so Unreal can drive it with Leader Pose from the MetaHuman Body.
The body itself is only a fitting dummy and is not exported (it is licensed MetaHuman content).
"""
import json
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

argv = sys.argv[sys.argv.index('--') + 1:]
BODY_FBX, OUT = argv[0], argv[1]
os.makedirs(OUT, exist_ok=True)
REPORT = {'body': BODY_FBX}

# Sizes in metres after import (the FBX from Unreal is in centimetres; we normalise below).
EASE_TOP = 0.012      # dress top stands this far off the skin
EASE_APRON = 0.006    # apron over the dress
SLEEVE = 0.42         # share of the upper arm the short sleeve covers
HEM_BELOW_KNEE = 0.06
FLARE = 1.12          # skirt hem circumference over the hip circumference
COLOURS = {'M_Uniform_Blue': (0.42, 0.56, 0.74, 1), 'M_Uniform_White': (0.86, 0.85, 0.80, 1), 'M_Apron': (0.90, 0.89, 0.85, 1)}


def clean():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def import_body():
    bpy.ops.import_scene.fbx(filepath=BODY_FBX, automatic_bone_orientation=False, ignore_leaf_bones=False)
    arm = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
    body = max((o for o in bpy.context.scene.objects if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
    # Unreal exports centimetres; work in metres with the armature as the only scale carrier.
    height = max((body.matrix_world @ v.co).z for v in body.data.vertices) - min((body.matrix_world @ v.co).z for v in body.data.vertices)
    REPORT['imported_height'] = height
    REPORT['body_scale'] = tuple(body.matrix_world.to_scale())
    REPORT['armature_scale'] = tuple(arm.matrix_world.to_scale())
    # The MetaHuman body mesh ends at the neck (the head is a separate mesh): check the bones instead.
    head = arm.matrix_world @ arm.data.bones['head'].head_local
    pelvis = arm.matrix_world @ arm.data.bones['pelvis'].head_local
    REPORT['head_z'], REPORT['pelvis_z'] = head.z, pelvis.z
    if not (1.3 < head.z < 1.9 and 0.75 < pelvis.z < 1.15):
        raise SystemExit(f'unexpected skeleton heights head {head.z:.3f} pelvis {pelvis.z:.3f}: check FBX units')
    return arm, body, height


def world_verts(obj):
    return [obj.matrix_world @ v.co for v in obj.data.vertices]


def bone_head(arm, name):
    b = arm.pose.bones.get(name) or arm.data.bones.get(name)
    return arm.matrix_world @ (b.head if hasattr(b, 'head') else b.head_local)


def group_weights(body, names):
    idx = {body.vertex_groups[n].index for n in names if n in body.vertex_groups}
    out = []
    for v in body.data.vertices:
        out.append(sum(g.weight for g in v.groups if g.group in idx))
    return out


def material(name):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = COLOURS[name]
    return m


def duplicate_region(body, keep, name, ease):
    """Copy the body faces whose vertices all pass keep(i), pushed off the skin along normals."""
    me = body.data
    bm = bmesh.new(); bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    drop = [f for f in bm.faces if not all(keep(v.index) for v in f.verts)]
    bmesh.ops.delete(bm, geom=drop, context='FACES')
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context='VERTS')
    bm.normal_update()
    local = ease / body.matrix_world.to_scale().x  # metres to the body's local units
    for v in bm.verts:
        v.co += v.normal * local
    new = bpy.data.meshes.new(name); bm.to_mesh(new); bm.free()
    obj = bpy.data.objects.new(name, new)
    obj.matrix_world = body.matrix_world
    bpy.context.scene.collection.objects.link(obj)
    return obj


def smooth(obj, factor=0.6, repeat=12, preserve_volume=True):
    m = obj.modifiers.new('smooth', 'CORRECTIVE_SMOOTH' if preserve_volume else 'SMOOTH')
    m.factor, m.iterations = factor, repeat
    if preserve_volume:
        m.rest_source = 'ORCO'
        m.smooth_type = 'LENGTH_WEIGHTED'
    return apply(obj, m)


def apply(obj, mod):
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.select_set(False)
    return obj


def section(points, z, band):
    """Points of the body within band of height z, as (x, y)."""
    return [(p.x, p.y) for p in points if abs(p.z - z) < band]


def hull(points):
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def ring(outline, n, scale, ease):
    """n points around a convex outline, scaled about its centre and pushed outward by ease."""
    cx = sum(p[0] for p in outline) / len(outline); cy = sum(p[1] for p in outline) / len(outline)
    out = []
    for i in range(n):
        a = 2 * math.pi * i / n
        d = Vector((math.cos(a), math.sin(a)))
        best = max(((Vector(p) - Vector((cx, cy))).dot(d), p) for p in outline)[0]
        r = best * scale + ease
        out.append((cx + d.x * r, cy + d.y * r))
    return out


def skirt(body, arm, height):
    pts = world_verts(body)
    waist_z = bone_head(arm, 'spine_01').z
    hip_z = bone_head(arm, 'pelvis').z - 0.04
    knee_z = (bone_head(arm, 'calf_l').z + bone_head(arm, 'calf_r').z) / 2
    hem_z = knee_z - HEM_BELOW_KNEE
    hips = hull(section(pts, hip_z, 0.015))
    waist = hull(section(pts, waist_z, 0.015))
    n, rows = 96, 22
    me = bpy.data.meshes.new('Skirt'); bm = bmesh.new()
    grid = []
    for r in range(rows + 1):
        t = r / rows
        z = waist_z + (hem_z - waist_z) * t
        if z > hip_z:  # waist to hips: follow the body tightly
            u = (waist_z - z) / max(waist_z - hip_z, 1e-4)
            a, b = ring(waist, n, 1.0, EASE_TOP), ring(hips, n, 1.0, EASE_TOP * 1.4)
            row = [(a[i][0] * (1 - u) + b[i][0] * u, a[i][1] * (1 - u) + b[i][1] * u) for i in range(n)]
        else:          # hips to hem: an A-line that the cloth step relaxes
            u = (hip_z - z) / max(hip_z - hem_z, 1e-4)
            row = ring(hips, n, 1.0 + (FLARE - 1.0) * u, EASE_TOP * 1.4)
        grid.append([bm.verts.new((x, y, z)) for x, y in row])
    for r in range(rows):
        for i in range(n):
            bm.faces.new((grid[r][i], grid[r][(i + 1) % n], grid[r + 1][(i + 1) % n], grid[r + 1][i]))
    bm.normal_update(); bm.to_mesh(me); bm.free()
    obj = bpy.data.objects.new('Skirt', me); bpy.context.scene.collection.objects.link(obj)
    pin = obj.vertex_groups.new(name='pin')
    pin.add([v.index for v in me.vertices if v.co.z > hip_z - 0.02], 1.0, 'REPLACE')
    REPORT['skirt'] = {'waist_z': waist_z, 'hip_z': hip_z, 'hem_z': hem_z, 'rows': rows, 'columns': n}
    return obj


def drape(obj, body, frames=40):
    """A short cloth settle against the body: natural folds instead of a perfect cone."""
    col = body.modifiers.new('collision', 'COLLISION')
    body.collision.thickness_outer = 0.004
    cloth = obj.modifiers.new('cloth', 'CLOTH')
    s = cloth.settings
    s.quality = 8; s.mass = 0.25; s.tension_stiffness = 25; s.compression_stiffness = 25
    s.shear_stiffness = 10; s.bending_stiffness = 0.8; s.air_damping = 2.0
    s.vertex_group_mass = 'pin'
    cloth.collision_settings.use_self_collision = True
    cloth.collision_settings.distance_min = 0.003
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 1, frames
    for f in range(1, frames + 1):
        scene.frame_set(f)
    apply(obj, cloth)
    body.modifiers.remove(col)


def collar_and_cuffs(top, arm):
    """White collar from the neckline loop, white cuffs from the sleeve ends."""
    bm = bmesh.new(); bm.from_mesh(top.data)
    boundary = [e for e in bm.edges if e.is_boundary]
    neck = bone_head(arm, 'neck_01')
    white = []
    for e in boundary:
        mid = top.matrix_world @ ((e.verts[0].co + e.verts[1].co) / 2)
        white.append((e, (mid - neck).length))
    neckline = [e for e, d in white if d < 0.14]
    cuffs = [e for e, d in white if d >= 0.14 and abs((top.matrix_world @ e.verts[0].co).z - neck.z) < 0.35]
    # Collar: extrude the neckline outward and down into a flat band.
    if neckline:
        ret = bmesh.ops.extrude_edge_only(bm, edges=neckline)
        verts = [v for v in ret['geom'] if isinstance(v, bmesh.types.BMVert)]
        for v in verts:
            w = top.matrix_world @ v.co
            out = Vector((w.x - neck.x, w.y - neck.y, 0)).normalized()
            v.co += top.matrix_world.inverted().to_3x3() @ (out * 0.055 + Vector((0, 0, -0.03)))
    for e in cuffs:
        for f in e.link_faces:
            f.material_index = 1
    for f in bm.faces:
        if f.material_index != 1 and any(v.is_boundary for v in f.verts) and neckline and \
                ((top.matrix_world @ f.calc_center_median()) - neck).length < 0.11:
            f.material_index = 1
    bm.to_mesh(top.data); bm.free()
    REPORT['collar_edges'], REPORT['cuff_edges'] = len(neckline), len(cuffs)


def apron(top, skirt_obj, arm):
    """Bib and skirt panels: front-facing faces of top and skirt between the side seams."""
    pelvis = bone_head(arm, 'pelvis')
    left, right = bone_head(arm, 'thigh_l'), bone_head(arm, 'thigh_r')
    side = (left - right); side.z = 0; side.normalize()
    front = Vector((0, 0, 1)).cross(side)  # her forward, from the hips
    half = (left - right).length * 0.95
    pieces = []
    for src, z_top in ((top, bone_head(arm, 'spine_05').z + 0.02), (skirt_obj, None)):
        bm = bmesh.new(); bm.from_mesh(src.data)
        keep = []
        for f in bm.faces:
            c = src.matrix_world @ f.calc_center_median()
            lateral = abs((c - pelvis).dot(side))
            facing = (src.matrix_world.to_3x3() @ f.normal).dot(front)
            if facing > 0.35 and lateral < half and (z_top is None or c.z < z_top):
                keep.append(f)
        drop = [f for f in bm.faces if f not in set(keep)]
        bmesh.ops.delete(bm, geom=drop, context='FACES')
        bm.normal_update()
        local = EASE_APRON / src.matrix_world.to_scale().x
        for v in bm.verts:
            v.co += v.normal * local
        me = bpy.data.meshes.new('Apron'); bm.to_mesh(me); bm.free()
        o = bpy.data.objects.new('Apron_' + src.name, me); o.matrix_world = src.matrix_world
        bpy.context.scene.collection.objects.link(o)
        pieces.append(o)
    bpy.ops.object.select_all(action='DESELECT')
    for o in pieces:
        o.select_set(True)
    bpy.context.view_layer.objects.active = pieces[0]
    bpy.ops.object.join()
    ap = bpy.context.view_layer.objects.active; ap.name = 'Apron'
    solid = ap.modifiers.new('thick', 'SOLIDIFY'); solid.thickness = 0.002 / ap.matrix_world.to_scale().x; apply(ap, solid)
    return ap


def skin(piece, body, arm, pelvis_bias=0.0):
    """Body weights by nearest surface; the skirt leans toward the pelvis so it never splits."""
    for g in body.vertex_groups:
        if g.name not in piece.vertex_groups:
            piece.vertex_groups.new(name=g.name)
    dt = piece.modifiers.new('weights', 'DATA_TRANSFER')
    dt.object = body
    dt.use_vert_data = True
    dt.data_types_verts = {'VGROUP_WEIGHTS'}
    dt.vert_mapping = 'POLYINTERP_NEAREST'
    dt.layers_vgroup_select_src = 'ALL'; dt.layers_vgroup_select_dst = 'NAME'
    apply(piece, dt)
    if pelvis_bias > 0 and 'pelvis' in piece.vertex_groups:
        pg = piece.vertex_groups['pelvis']
        for v in piece.data.vertices:
            for g in v.groups:
                if piece.vertex_groups[g.group].name.startswith(('thigh', 'calf')):
                    g.weight *= (1 - pelvis_bias)
            pg.add([v.index], pelvis_bias, 'ADD')
    bpy.context.view_layer.objects.active = piece
    bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    m = piece.modifiers.new('Armature', 'ARMATURE'); m.object = arm
    world = piece.matrix_world.copy()
    piece.parent = arm
    piece.matrix_world = world


def preview(body, uni):
    """Two quick renders of the fitted uniform on the body, for review (not exported)."""
    sc = bpy.context.scene
    body.hide_render = False; body.hide_set(False)
    skin = bpy.data.materials.new('Preview_Skin'); skin.diffuse_color = (0.72, 0.55, 0.47, 1)
    body.data.materials.clear(); body.data.materials.append(skin)
    for m, c in COLOURS.items():
        mat = bpy.data.materials.get(m)
        if mat:
            mat.use_nodes = True
            mat.node_tree.nodes['Principled BSDF'].inputs['Base Color'].default_value = c
    sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = 24
    sc.render.resolution_x, sc.render.resolution_y = 600, 900
    world = bpy.data.worlds.new('W'); sc.world = world; world.color = (0.35, 0.35, 0.38)
    light = bpy.data.objects.new('Key', bpy.data.lights.new('Key', 'SUN')); sc.collection.objects.link(light)
    light.data.energy = 3.5; light.rotation_euler = (math.radians(50), 0, math.radians(-30))
    cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam')); sc.collection.objects.link(cam); sc.camera = cam
    cam.data.lens = 50
    for name, yaw in (('front', 0), ('three_quarter', 35)):
        a = math.radians(yaw)
        cam.location = (math.sin(a) * 4.2, -math.cos(a) * 4.2, 1.0)
        cam.rotation_euler = (math.radians(90), 0, a)
        sc.render.filepath = os.path.join(OUT, f'uniform_{name}.png')
        bpy.ops.render.render(write_still=True)


def main():
    clean()
    arm, body, height = import_body()
    arm_obj = arm
    pts = world_verts(body)
    # MetaHuman bodies spread weights over many helper bones, so classify by geometry: a vertex
    # belongs to the arm when it lies closer to the arm bones than to the spine axis.
    def seg(p, a, b):
        ab = b - a
        t = max(0.0, min(1.0, (p - a).dot(ab) / ab.length_squared))
        return (p - (a + ab * t)).length, t
    J = lambda n: bone_head(arm_obj, n)
    pelvis, neck0, neck1 = J('pelvis'), J('neck_01'), J('head')
    side = {s: (J(f'upperarm_{s}'), J(f'lowerarm_{s}'), J(f'hand_{s}')) for s in 'lr'}
    clav_z = (J('clavicle_l').z + J('clavicle_r').z) / 2
    waist_z = J('spine_01').z
    neck = armw = [0] * len(pts)  # kept for the debug report

    def keep(i):
        p = pts[i]
        if p.z < waist_z - 0.03:
            return False
        if p.z > clav_z + 0.015 and Vector((p.x - neck0.x, p.y - neck0.y)).length < 0.085:
            return False  # the neck stays bare above the collar line
        d_torso, _ = seg(p, pelvis, neck0)
        for s in 'lr':
            sh, el, ha = side[s]
            d_up, t_up = seg(p, sh, el)
            d_low, _ = seg(p, el, ha)
            if min(d_up, d_low) < d_torso * 0.85 and (p - sh).dot(el - sh) > 0:
                return d_up <= d_low and t_up < SLEEVE
        return True
    REPORT['top_keep'] = sum(1 for i in range(len(pts)) if keep(i))
    REPORT['debug'] = {'waist_z': waist_z, 'above_waist': sum(1 for p in pts if p.z >= waist_z - 0.03),
                       'neck': sum(1 for x in neck if x > 0.3), 'arm': sum(1 for x in armw if x > 0.5)}
    top = duplicate_region(body, keep, 'DressTop', EASE_TOP)
    REPORT['top_after_dup'] = len(top.data.vertices)
    smooth(top, 0.7, 18)
    REPORT['top_after_smooth'] = len(top.data.vertices)
    top.data.materials.append(material('M_Uniform_Blue')); top.data.materials.append(material('M_Uniform_White'))
    collar_and_cuffs(top, arm)
    sk = skirt(body, arm, height)
    drape(sk, body)
    sk.data.materials.append(material('M_Uniform_Blue'))
    ap = apron(top, sk, arm)
    ap.data.materials.clear(); ap.data.materials.append(material('M_Apron'))
    for piece, bias in ((top, 0), (sk, 0.55), (ap, 0.45)):
        skin(piece, body, arm, bias)
    # One mesh for one Unreal component: dress, collar, cuffs and apron share the leader pose.
    bpy.ops.object.select_all(action='DESELECT')
    for o in (top, sk, ap):
        o.select_set(True)
    bpy.context.view_layer.objects.active = top
    bpy.ops.object.join()
    uni = bpy.context.view_layer.objects.active; uni.name = 'SKM_Waitress_Uniform'
    REPORT['uniform'] = {'vertices': len(uni.data.vertices), 'faces': len(uni.data.polygons),
                         'materials': [m.name for m in uni.data.materials]}
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'waitress_uniform_fitting.blend'))
    try:
        preview(body, uni)
    except Exception as e:
        REPORT['preview_error'] = repr(e)
    body.hide_set(True)
    bpy.ops.object.select_all(action='DESELECT')
    uni.select_set(True); arm.select_set(True)
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, 'SKM_Waitress_Uniform.fbx'), use_selection=True,
                             object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False, bake_anim=False,
                             mesh_smooth_type='FACE', use_armature_deform_only=False)
    json.dump(REPORT, open(os.path.join(OUT, 'uniform_report.json'), 'w'), indent=2, default=str)
    print('UNIFORM', json.dumps(REPORT, default=str))


main()
