"""D46-10 (Tom, 10 November): the trip camera as a model, made here from primitives (original geometry, no downloads).

A 1986 field trip camera of the kind an operator builds at a bench: a weatherproof olive steel box on a driven ground
stake, a cheap 35 mm camera's lens behind a port in the front, a small flash reflector, a counter window, a timer dial
and an ARM / SAFE switch on the lid, two latches, a webbing carry strap over the top, a tripwire spool on the left side
with its lead into the box. Stencilled generic labels (no brands). Metres, Z up in Blender; the lens faces Blender +Y,
which the glTF exporter makes -Z: the way a three.js object faces at yaw 0, so S47Three::Euler(0,Yaw,0) in the game
points the lens where the player looked when it was set. Pivot: the middle of the stake's foot, on the ground.

Run:  nice -n 19 blender -b --factory-startup -P build_tripcam.py -- <out folder>
Writes tripcam.blend, tripcam.glb, tripcam.json (bounds in the game's frame), render_front34.png, render_inhand.png.
"""
import json
import math
import os
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector

OUT = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/tripcam-20261110')
os.makedirs(OUT, exist_ok=True)
MODEL = []  # objects that go into the glb


# ---------- materials
def mat(name, rgb, rough=.5, metal=0., spec=.5, emit=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (*rgb, 1)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Specular IOR Level'].default_value = spec
    if emit:
        bsdf.inputs['Emission Color'].default_value = (*emit, 1)
        bsdf.inputs['Emission Strength'].default_value = .6
    return m


M = {
    'olive': mat('tripcam_olive_paint', (.16, .19, .10), rough=.55, metal=.2),
    'olive_dark': mat('tripcam_olive_worn', (.11, .13, .07), rough=.7, metal=.1),
    'steel': mat('tripcam_steel', (.55, .55, .53), rough=.45, metal=1.),
    'steel_dark': mat('tripcam_steel_dark', (.25, .26, .27), rough=.6, metal=1.),
    'rubber': mat('tripcam_rubber', (.03, .03, .03), rough=.85),
    'glass': mat('tripcam_lens_glass', (.02, .03, .06), rough=.06, spec=1.),
    'reflector': mat('tripcam_reflector', (.92, .92, .9), rough=.2, metal=1.),
    'webbing': mat('tripcam_webbing', (.42, .37, .24), rough=.92),
    'stencil': mat('tripcam_stencil', (.88, .86, .78), rough=.75),
    'wire': mat('tripcam_wire', (.35, .36, .36), rough=.5, metal=1.),
    'counter': mat('tripcam_counter', (.9, .88, .8), rough=.6),
    'red': mat('tripcam_red_mark', (.6, .1, .08), rough=.6),
}


def add(name, me, material, smooth=False, bevel=0., parent_model=True):
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    me.materials.append(material)
    if smooth:
        for p in me.polygons:
            n = p.normal
            p.use_smooth = abs(abs(n.z) - 1) > 1e-3 and abs(abs(n.y) - 1) > 1e-3 and abs(abs(n.x) - 1) > 1e-3
    if bevel > 0:
        b = ob.modifiers.new('bevel', 'BEVEL')
        b.width = bevel
        b.segments = 2
        b.limit_method = 'ANGLE'
        b.angle_limit = math.radians(40)
    if parent_model:
        MODEL.append(ob)
    return ob


def box(name, size, at, material, bevel=0., parent_model=True, yaw=0.):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    if yaw:
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(yaw, 3, 'Z'), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(at), verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return add(name, me, material, bevel=bevel, parent_model=parent_model)


def cyl(name, r, length, at, material, axis='Z', segs=24, r2=None, parent_model=True, caps=True, tilt=0.):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=caps, cap_tris=False, segments=segs, radius1=r, radius2=r if r2 is None else r2, depth=length)
    if tilt:
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(tilt, 3, 'X'), verts=bm.verts)
    if axis == 'Y':
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(math.radians(-90), 3, 'X'), verts=bm.verts)
    elif axis == 'X':
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(math.radians(90), 3, 'Y'), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(at), verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return add(name, me, material, smooth=True, parent_model=parent_model)


def label(name, text, at, size, material, rot=(90, 0, 180), depth=.0006):
    """Stencilled text as a thin mesh lying on a face. rot (90,0,180): upright, read from +Y; (90,0,0): read from -Y; (0,0,0): flat on the lid."""
    cu = bpy.data.curves.new(name + '_c', type='FONT')
    cu.body = text
    cu.size = size
    cu.extrude = depth / 2
    cu.align_x = 'CENTER'
    cu.align_y = 'CENTER'
    cu.resolution_u = 2
    tmp = bpy.data.objects.new(name + '_t', cu)
    bpy.context.scene.collection.objects.link(tmp)
    tmp.rotation_euler = tuple(math.radians(a) for a in rot)
    tmp.location = at
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(tmp.evaluated_get(dg))
    me.transform(tmp.matrix_world)
    bpy.data.objects.remove(tmp)
    bpy.data.curves.remove(cu)
    ob = add(name, me, material)
    return ob


def strip(name, pts, width, thick, material):
    """A webbing band: quads along pts (x, z) in the XZ plane, width along Y, with thickness."""
    bm = bmesh.new()
    rows = []
    for (x, z) in pts:
        rows.append((bm.verts.new((x, -width / 2, z)), bm.verts.new((x, width / 2, z))))
    for a, b in zip(rows, rows[1:]):
        bm.faces.new((a[0], a[1], b[1], b[0]))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = add(name, me, material, smooth=False)
    s = ob.modifiers.new('solid', 'SOLIDIFY')
    s.thickness = thick
    s.offset = 0
    return ob


def cable(name, points, r, material):
    cu = bpy.data.curves.new(name + '_c', type='CURVE')
    cu.dimensions = '3D'
    sp = cu.splines.new('BEZIER')
    sp.bezier_points.add(len(points) - 1)
    for p, v in zip(sp.bezier_points, points):
        p.co = v
        p.handle_left_type = p.handle_right_type = 'AUTO'
    cu.bevel_depth = r
    cu.bevel_resolution = 3
    cu.resolution_u = 6
    tmp = bpy.data.objects.new(name + '_t', cu)
    bpy.context.scene.collection.objects.link(tmp)
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(tmp.evaluated_get(dg))
    bpy.data.objects.remove(tmp)
    bpy.data.curves.remove(cu)
    return add(name, me, material, smooth=True)


# ---------- the scene
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)

# the housing: X 17, Y 10.5, Z 13 cm, its bottom 13.6 cm up on the stake's bracket
HX, HY, HZ, HB = .17, .105, .13, .136
LID = .022
FRONT = HY / 2
body = box('tripcam_body', (HX, HY, HZ - LID), (0, 0, HB + (HZ - LID) / 2), M['olive'], bevel=.005)
lid = box('tripcam_lid', (HX + .004, HY + .004, LID), (0, 0, HB + HZ - LID / 2 + .001), M['olive'], bevel=.005)
seam = box('tripcam_seal', (HX + .001, HY + .001, .004), (0, 0, HB + HZ - LID), M['rubber'])
TOP = HB + HZ + .002

# the ground stake: a T post with a step plate, driven in; the bracket plate it carries
stake = box('tripcam_stake', (.02, .02, HB), (0, 0, HB / 2), M['steel_dark'])
stake_web = box('tripcam_stake_web', (.04, .006, HB * .8), (0, 0, HB * .4), M['steel_dark'])
foot = box('tripcam_stake_foot', (.12, .02, .008), (0, 0, .004), M['steel_dark'])
box('tripcam_stake_foot2', (.02, .09, .008), (0, 0, .004), M['steel_dark'])
bracket = box('tripcam_bracket', (.09, .07, .006), (0, 0, HB - .003), M['steel_dark'], bevel=.001)
for sx in (-1, 1):
    cyl('tripcam_bracket_bolt%d' % sx, .004, .003, (sx * .035, -.02, HB - .0075), M['steel'], segs=12)

# lens port, hood and glass; four front screws
LX, LZ = -.032, HB + .062
cyl('tripcam_lens_port', .024, .018, (LX, FRONT + .009, LZ), M['olive_dark'], axis='Y')
cyl('tripcam_lens_hood', .021, .012, (LX, FRONT + .024, LZ), M['rubber'], axis='Y', r2=.023, caps=False)
cyl('tripcam_lens_ring', .024, .001, (LX, FRONT + .0185, LZ), M['steel_dark'], axis='Y', segs=24)
cyl('tripcam_lens_barrel', .016, .006, (LX, FRONT + .021, LZ), M['steel_dark'], axis='Y')
cyl('tripcam_lens_glass', .014, .002, (LX, FRONT + .023, LZ), M['glass'], axis='Y', segs=32)
for sx in (-1, 1):
    for sz in (-1, 1):
        cyl('tripcam_screw_%d%d' % (sx, sz), .0035, .002, (sx * (HX / 2 - .011), FRONT + .001, HB + (HZ - LID) / 2 + sz * ((HZ - LID) / 2 - .011)), M['steel'], axis='Y', segs=12)

# the flash: a small reflector behind a glass cover, upper right of the front
FX, FZ = .048, HB + .078
box('tripcam_flash_frame', (.046, .006, .034), (FX, FRONT + .002, FZ), M['steel_dark'], bevel=.001)
box('tripcam_flash_reflector', (.038, .004, .026), (FX, FRONT + .003, FZ), M['reflector'])
cyl('tripcam_flash_tube', .003, .03, (FX, FRONT + .0045, FZ), M['counter'], axis='X', segs=10)
box('tripcam_flash_glass', (.04, .001, .028), (FX, FRONT + .0056, FZ), M['glass'])

# the frame counter window, lower right
box('tripcam_counter_frame', (.034, .004, .014), (FX, FRONT + .001, HB + .03), M['steel_dark'])
box('tripcam_counter_face', (.028, .002, .009), (FX, FRONT + .002, HB + .03), M['counter'])
box('tripcam_counter_glass', (.03, .0006, .011), (FX, FRONT + .0033, HB + .03), M['glass'])

# the lid: timer dial with its knob, pointer and ticks; the ARM / SAFE toggle; a red dot for armed
DX = .045
cyl('tripcam_dial_plate', .021, .004, (DX, .012, TOP + .002), M['steel_dark'], segs=32)
cyl('tripcam_dial_face', .018, .001, (DX, .012, TOP + .0045), M['counter'], segs=32)
cyl('tripcam_dial_knob', .009, .012, (DX, .012, TOP + .011), M['rubber'], segs=16, r2=.0075)
box('tripcam_dial_pointer', (.003, .014, .002), (DX, .012 + .011, TOP + .0175), M['stencil'])
for i in range(12):
    a = math.radians(i * 30)
    ln = .004 if i % 3 == 0 else .0025
    box('tripcam_tick%d' % i, (.0012, ln, .0007), (DX + math.sin(a) * (.0155 - ln / 2), .012 + math.cos(a) * (.0155 - ln / 2), TOP + .0052), M['rubber'], yaw=-a)
SX = -.045
cyl('tripcam_switch_base', .007, .004, (SX, .018, TOP + .002), M['steel'], segs=16)
cyl('tripcam_switch_lever', .0025, .018, (SX, .018 - .004, TOP + .011), M['steel'], segs=10, tilt=math.radians(-25))
cyl('tripcam_switch_tip', .0035, .004, (SX, .018 - .008, TOP + .019), M['red'], segs=10)
cyl('tripcam_arm_lamp', .004, .003, (SX + .018, .018, TOP + .0015), M['red'], segs=12)

# the strap: a webbing loop over the lid, on two D-rings, and the latches on both sides
pts = [(-.056, TOP - .006), (-.056, TOP + .01), (-.045, TOP + .03), (-.025, TOP + .041), (0, TOP + .045), (.025, TOP + .041), (.045, TOP + .03), (.056, TOP + .01), (.056, TOP - .006)]
strip('tripcam_strap', pts, .02, .0025, M['webbing'])
for sx in (-1, 1):
    box('tripcam_strap_tab%d' % sx, (.006, .024, .012), (sx * .059, 0, TOP - .009), M['steel_dark'], bevel=.001)
    box('tripcam_latch%d' % sx, (.012, .03, .03), (sx * (HX / 2 + .006), -.028, HB + HZ - LID - .012), M['steel'], bevel=.0015)
    box('tripcam_latch_lip%d' % sx, (.013, .032, .004), (sx * (HX / 2 + .0075), -.028, HB + HZ - LID + .004), M['steel'], bevel=.001)

# the tripwire spool on the left side, its lead into a jack under the box, and the loose end's clip
SPX = -HX / 2 - .03
cyl('tripcam_spool_core', .02, .034, (SPX, -.01, HB + .05), M['olive_dark'], axis='X', segs=20)
cyl('tripcam_spool_wire', .027, .028, (SPX, -.01, HB + .05), M['wire'], axis='X', segs=24)
for sx in (-1, 1):
    cyl('tripcam_spool_flange%d' % sx, .032, .003, (SPX + sx * .0175, -.01, HB + .05), M['steel_dark'], axis='X', segs=28)
cyl('tripcam_spool_axle', .005, .054, (SPX, -.01, HB + .05), M['steel'], axis='X', segs=10)
box('tripcam_spool_arm', (.026, .012, .012), (SPX + .021, -.01, HB + .05 + .018), M['steel_dark'], bevel=.001)
box('tripcam_spool_mount', (.006, .03, .06), (-HX / 2 - .003, -.01, HB + .055), M['steel_dark'], bevel=.001)
cyl('tripcam_crank', .003, .022, (SPX - .022, -.01 + .006, HB + .05 + .022), M['steel'], axis='Z', segs=8)
cyl('tripcam_crank_arm', .0025, .024, (SPX - .021, -.01 + .006, HB + .05 + .012), M['steel'], axis='Y', segs=8)
cable('tripcam_lead', [(SPX, -.01 + .028, HB + .05), (SPX + .01, -.01 + .052, HB + .035), (-.04, FRONT - .035, HB + .004), (-.02, FRONT - .02, HB - .002)], .0022, M['rubber'])
cyl('tripcam_jack', .006, .01, (-.02, FRONT - .02, HB - .005), M['steel_dark'], axis='Z', segs=12)
# the loose end: a short run off the spool to a clip on the bracket
cable('tripcam_wire_end', [(SPX, -.01 - .028, HB + .05), (SPX - .005, -.01 - .045, HB + .028), (-.035, -.03, HB - .004)], .0008, M['wire'])
box('tripcam_wire_clip', (.012, .006, .004), (-.035, -.03, HB - .005), M['steel'])

# stencils: on the front, the lid and the back
label('tripcam_lbl_name', 'TRIP CAMERA', (0, FRONT + .0003, HB + HZ - LID - .013), .011, M['stencil'])
label('tripcam_lbl_unit', 'FIELD UNIT 3', (0, FRONT + .0003, HB + .012), .007, M['stencil'])
label('tripcam_lbl_frames', 'FRAMES', (FX, FRONT + .0003, HB + .042), .005, M['stencil'])
label('tripcam_lbl_timer', 'TIMER  HRS', (DX, -.014, TOP + .0003), .0055, M['stencil'], rot=(0, 0, 0))
label('tripcam_lbl_arm', 'ARM', (SX, .034, TOP + .0003), .005, M['stencil'], rot=(0, 0, 0))
label('tripcam_lbl_safe', 'SAFE', (SX, .002, TOP + .0003), .005, M['stencil'], rot=(0, 0, 0))
label('tripcam_lbl_back', 'KEEP DRY', (0, -FRONT - .0003, HB + HZ - LID - .016), .009, M['stencil'], rot=(90, 0, 0))
label('tripcam_lbl_back2', 'OPERATOR BUILT', (0, -FRONT - .0003, HB + .03), .007, M['stencil'], rot=(90, 0, 0))

# ---------- counts, bounds, files
bpy.context.view_layer.update()
dg = bpy.context.evaluated_depsgraph_get()
tris = 0
lo = Vector((1e9, 1e9, 1e9))
hi = Vector((-1e9, -1e9, -1e9))
for ob in MODEL:
    ev = ob.evaluated_get(dg)
    me = ev.to_mesh()
    tris += sum(len(p.vertices) - 2 for p in me.polygons)
    for v in me.vertices:
        w = ev.matrix_world @ v.co
        lo = Vector(map(min, lo, w))
        hi = Vector(map(max, hi, w))
    ev.to_mesh_clear()
height = hi.z - lo.z
# glTF / three frame: (x, y_up, z_three) = (x, z, -y)
manifest = {
    '_doc': 'Made by build_tripcam.py (D46-10). Metres, glTF / three.js frame (+Y up, the lens faces -Z). Checked by Tools/tripcam/import_assets.py.',
    'triangles': tris,
    'height_m': round(height, 4),
    'min_m': [round(lo.x, 4), round(lo.z, 4), round(-hi.y, 4)],
    'max_m': [round(hi.x, 4), round(hi.z, 4), round(-lo.y, 4)],
    'objects': len(MODEL),
    'materials': sorted(M[k].name for k in M),
}
print('TRIPCAM tris %d height %.3f m bounds %s .. %s' % (tris, height, tuple(round(v, 3) for v in lo), tuple(round(v, 3) for v in hi)))
json.dump(manifest, open(os.path.join(OUT, 'tripcam.json'), 'w'), indent=1)

for ob in bpy.data.objects:
    ob.select_set(ob in MODEL)
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, 'tripcam.glb'), export_format='GLB', use_selection=True, export_apply=True,
                          export_yup=True, export_materials='EXPORT', export_normals=True, export_texcoords=True, export_animations=False, export_skins=False)

# ---------- renders: Cycles on the CPU
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.samples = 80
sc.cycles.use_denoising = True
sc.render.resolution_x, sc.render.resolution_y = 1100, 900
sc.render.film_transparent = False
sc.view_settings.view_transform = 'AgX'
world = bpy.data.worlds.new('w')
sc.world = world
world.use_nodes = True
bg = world.node_tree.nodes['Background']
bg.inputs['Color'].default_value = (.35, .4, .5, 1)
bg.inputs['Strength'].default_value = .6
sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
sun.data.energy = 3.
sun.data.angle = math.radians(4)
sun.rotation_euler = (math.radians(50), math.radians(-15), math.radians(35))
sc.collection.objects.link(sun)
key = bpy.data.objects.new('key', bpy.data.lights.new('key', 'AREA'))
key.data.energy = 60.
key.data.size = .6
key.location = (.6, .7, .8)
key.rotation_euler = (math.radians(45), 0, math.radians(140))
sc.collection.objects.link(key)
sand = mat('ground_sand', (.46, .38, .27), rough=.95)
ground = box('ground', (3, 3, .01), (0, 0, -.005), sand, parent_model=False)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 50


def aim(cam_ob, at, frm):
    cam_ob.location = frm
    d = Vector(at) - Vector(frm)
    cam_ob.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()


# 3/4 front, a little above
aim(cam, (-.02, 0, .17), (-.46, .5, .36))
sc.render.filepath = os.path.join(OUT, 'render_front34.png')
bpy.ops.render.render(write_still=True)

# in-hand scale: a grey mannequin hand (palm, four fingers, thumb) holding it up by the bracket, and a 30 cm rule beside it
grey = mat('mannequin', (.5, .5, .5), rough=.8)
hand = []
hand.append(box('hand_palm', (.09, .1, .028), (0, -.02, -.015), grey, bevel=.01, parent_model=False))
for i, fx in enumerate((-.033, -.011, .011, .033)):
    hand.append(cyl('hand_finger%d' % i, .009, .08, (fx, .07, -.013), grey, axis='Y', segs=12, parent_model=False))
hand.append(cyl('hand_thumb', .011, .07, (-.07, .01, .0), grey, axis='Y', segs=12, parent_model=False))
for h in hand:
    h.location.z += .0
ground.hide_render = True
rule = box('rule', (.3, .025, .003), (.0, .16, .0), M['counter'], parent_model=False)
for i in range(31):
    box('rule_tick%d' % i, (.0008, .012 if i % 5 == 0 else .006, .0012), (-.15 + i * .01, .16 - .006, .0021), M['rubber'], parent_model=False)
label('rule_lbl', '0            10            20            30 cm', (0, .168, .0022), .009, M['rubber'], rot=(0, 0, 0))
MODEL.pop()  # (the rule's label is not part of the model)
aim(cam, (0, .02, .12), (-.3, -.55, .33))
sc.render.filepath = os.path.join(OUT, 'render_inhand.png')
bpy.ops.render.render(write_still=True)

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'tripcam.blend'))
print('TRIPCAM done', OUT)
