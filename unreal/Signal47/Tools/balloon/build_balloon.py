"""D48-20261110 (Tom, 10 November: the balloon find "needs a model"): a burst weather balloon on desert ground, made here from
primitives and noise (original geometry, no downloads, no Cosmos, no brands).

Four pieces, each its own glb so the game can take the instrument box away when it is picked up and leave the rest lying:
  envelope    the burst neoprene envelope, pale tan, 3.6 by 2.4 m of thin crumpled rubber lying in folds over the ground and
              draped over a scrub bush at its north-east corner (the bush is in this piece)
  parachute   a small orange nylon parachute, 1.4 m across, collapsed on the ground, its shroud lines run together to a ring
  cord        the long white cord from the envelope's neck to the ring
  radiosonde  a 1980s radiosonde: a white-grey cardboard-and-styrofoam box 20 x 15 x 10 cm with a printed return label (plain
              text, no agency, no logo), a short wire antenna, a small battery pack on its lead, and the little red lamp dome
              of the recovery beacon on top (the game blinks it: the beacon's place is in the manifest)
Metres, Z up in Blender; every piece's pivot is on the ground under its own middle, so the game sets each at its own offset
(S47Balloon.cpp has the offsets; the manifest has the bounds). glTF Y-up export turns Blender (x, y, z) into (x, z, -y), the
frame S47Three::ToUE expects.

Run:  nice -n 19 blender -b --factory-startup -P build_balloon.py -- <out folder>
Writes balloon.blend, <piece>.glb, balloon.json (bounds and triangle counts per piece, the beacon's place),
render_day34.png (3/4 by day) and render_night_flashlight.png (close, at night, by flashlight, the beacon lit).
"""
import json
import math
import os
import random
import sys

import bpy
import bmesh
from mathutils import Matrix, Vector, noise

OUT = sys.argv[sys.argv.index('--') + 1] if '--' in sys.argv else os.path.expanduser('~/Documents/ChatGPT/Signal 47/modeller/balloon-20261110')
os.makedirs(OUT, exist_ok=True)
random.seed(1986)
PIECES = {'envelope': [], 'parachute': [], 'cord': [], 'radiosonde': []}
EXTRA = []  # scene dressing for the renders only


# ---------- materials
def mat(name, rgb, rough=.5, metal=0., spec=.5, emit=None, emit_strength=.6):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes['Principled BSDF']
    bsdf.inputs['Base Color'].default_value = (*rgb, 1)
    bsdf.inputs['Roughness'].default_value = rough
    bsdf.inputs['Metallic'].default_value = metal
    bsdf.inputs['Specular IOR Level'].default_value = spec
    if emit:
        bsdf.inputs['Emission Color'].default_value = (*emit, 1)
        bsdf.inputs['Emission Strength'].default_value = emit_strength
    return m


M = {
    'neoprene': mat('balloon_neoprene', (.66, .6, .47), rough=.78, spec=.25),
    'neoprene_dark': mat('balloon_neoprene_dirty', (.55, .49, .38), rough=.85, spec=.2),
    'bush': mat('balloon_scrub_bush', (.22, .24, .15), rough=.95),
    'twig': mat('balloon_scrub_twig', (.3, .24, .17), rough=.9),
    'nylon': mat('balloon_parachute_nylon', (.88, .36, .07), rough=.55, spec=.4),
    'line': mat('balloon_shroud_line', (.86, .85, .8), rough=.8),
    'ring': mat('balloon_ring', (.6, .6, .58), rough=.5, metal=1.),
    'cord': mat('balloon_cord', (.9, .89, .84), rough=.85),
    'foam': mat('balloon_styrofoam', (.86, .86, .83), rough=.9),
    'card': mat('balloon_cardboard', (.6, .49, .34), rough=.92),
    'label': mat('balloon_label_paper', (.93, .91, .84), rough=.8),
    'ink': mat('balloon_label_ink', (.08, .08, .1), rough=.7),
    'wire': mat('balloon_wire', (.4, .4, .42), rough=.45, metal=1.),
    'battery': mat('balloon_battery_pack', (.12, .12, .13), rough=.6),
    'tape': mat('balloon_tape', (.7, .68, .6), rough=.6),
    'lamp': mat('balloon_beacon_lamp', (.7, .08, .05), rough=.2, spec=.8, emit=(1., .12, .06), emit_strength=0.),
}


def add(name, me, material, piece, smooth=False, bevel=0.):
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    me.materials.append(material)
    if smooth:
        for p in me.polygons:
            p.use_smooth = True
    if bevel > 0:
        b = ob.modifiers.new('bevel', 'BEVEL')
        b.width = bevel
        b.segments = 2
        b.limit_method = 'ANGLE'
        b.angle_limit = math.radians(40)
    if piece in PIECES:
        PIECES[piece].append(ob)
    else:
        EXTRA.append(ob)
    return ob


def box(name, size, at, material, piece, bevel=0., rot=(0, 0, 0)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    if any(rot):
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(rot[0], 3, 'X') @ Matrix.Rotation(rot[1], 3, 'Y') @ Matrix.Rotation(rot[2], 3, 'Z'), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(at), verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return add(name, me, material, piece, bevel=bevel)


def ball(name, r, at, material, piece, scale=(1, 1, 1), subdiv=1):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=r)
    bmesh.ops.scale(bm, vec=Vector(scale), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(at), verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return add(name, me, material, piece, smooth=True)


def cyl(name, r, length, at, material, piece, axis='Z', segs=12, r2=None, caps=True, tilt=(0, 0, 0)):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=caps, cap_tris=False, segments=segs, radius1=r, radius2=r if r2 is None else r2, depth=length)
    if axis == 'Y':
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(math.radians(-90), 3, 'X'), verts=bm.verts)
    elif axis == 'X':
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(math.radians(90), 3, 'Y'), verts=bm.verts)
    if any(tilt):
        bmesh.ops.rotate(bm, cent=Vector((0, 0, 0)), matrix=Matrix.Rotation(tilt[0], 3, 'X') @ Matrix.Rotation(tilt[1], 3, 'Y') @ Matrix.Rotation(tilt[2], 3, 'Z'), verts=bm.verts)
    bmesh.ops.translate(bm, vec=Vector(at), verts=bm.verts)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return add(name, me, material, piece, smooth=True)


def cable(name, points, r, material, piece, res=8):
    cu = bpy.data.curves.new(name + '_c', type='CURVE')
    cu.dimensions = '3D'
    sp = cu.splines.new('BEZIER')
    sp.bezier_points.add(len(points) - 1)
    for p, v in zip(sp.bezier_points, points):
        p.co = v
        p.handle_left_type = p.handle_right_type = 'AUTO'
    cu.bevel_depth = r
    cu.bevel_resolution = 2
    cu.resolution_u = res
    tmp = bpy.data.objects.new(name + '_t', cu)
    bpy.context.scene.collection.objects.link(tmp)
    bpy.context.view_layer.update()
    dg = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(tmp.evaluated_get(dg))
    bpy.data.objects.remove(tmp)
    bpy.data.curves.remove(cu)
    return add(name, me, material, piece, smooth=True)


def label(name, text, at, size, material, piece, rot=(0, 0, 0), depth=.0006):
    """Printed text as a thin mesh lying on a face. rot (0,0,0): flat, read from above."""
    cu = bpy.data.curves.new(name + '_c', type='FONT')
    cu.body = text
    cu.size = size
    cu.extrude = depth / 2
    cu.align_x = 'CENTER'
    cu.align_y = 'CENTER'
    cu.resolution_u = 1
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
    return add(name, me, material, piece)


def sheet(name, sx, sy, nx, ny, height, material, piece, thick=.004, edge_fade=.25, rim=None):
    """A crumpled sheet lying on the ground: a grid displaced by height(x, y) (metres), its rim pressed flat, with thickness.
    rim(theta) (0..1 of the half sizes, 1 the ellipse sx by sy): the torn outline; cells outside it are left out."""
    bm = bmesh.new()
    grid = [[None] * (ny + 1) for _ in range(nx + 1)]
    for i in range(nx + 1):
        for j in range(ny + 1):
            u, v = (i / nx - .5) * 2, (j / ny - .5) * 2
            x, y = u * sx / 2, v * sy / 2
            if rim is None:
                room = min(1 - abs(u), 1 - abs(v))
            else:
                d = math.hypot(u, v)
                room = rim(math.atan2(v, u)) - d
            if room < 0:
                continue
            # the rim lies flat: fade the lift toward the edge
            fade = min(1., room / edge_fade)
            fade = fade * fade * (3 - 2 * fade)
            z = max(0., height(x, y)) * fade + .002
            grid[i][j] = bm.verts.new((x, y, z))
    for i in range(nx):
        for j in range(ny):
            q = (grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1])
            if all(q):
                bm.faces.new(q)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = add(name, me, material, piece, smooth=True)
    s = ob.modifiers.new('solid', 'SOLIDIFY')
    s.thickness = thick
    s.offset = -1
    return ob


def fold_noise(x, y, scale, amp, seed):
    p = Vector((x * scale + seed, y * scale - seed * .7, seed * .3))
    n = noise.noise(p)                      # -1..1
    n2 = noise.noise(p * 2.3 + Vector((7, 1, 3)))
    return amp * (.6 * n + .4 * n2)


# ---------- the scene
for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)

# the envelope: a 3.6 x 2.4 m rubber sheet in folds, a dome where it lies over the bush (north-east corner), the neck to the south-west
BUSH = Vector((1.55, .72, 0))
BUSH_R, BUSH_H = .5, .34


def envelope_height(x, y):
    folds = .07 + fold_noise(x, y, 2.3, .1, 11.) + abs(fold_noise(x, y, 5.1, .06, 23.)) + abs(fold_noise(x, y, 11., .02, 31.))
    # long creases where the rubber lay folded on itself
    folds += .035 * max(0., math.sin(x * 5.2 + y * 1.7 + .6)) ** 6
    folds += .03 * max(0., math.sin(y * 7.3 - x * .8 + 2.)) ** 8
    # draped over the bush: a smooth dome
    d = math.hypot(x - BUSH.x, y - BUSH.y)
    dome = BUSH_H * max(0., 1 - (d / (BUSH_R * 1.15)) ** 2)
    return max(folds, dome + folds * .4)


def envelope_rim(t):
    # a burst envelope's torn outline: lobes and a few long tongues of rubber
    return .84 + .07 * math.sin(3 * t + 1) + .05 * math.sin(7 * t) + .03 * math.sin(13 * t + 2) + .16 * max(0., math.sin(2 * t + .5)) ** 9 + .1 * max(0., math.sin(5 * t + 2.2)) ** 7


env = sheet('balloon_envelope', 3.8, 2.6, 46, 32, envelope_height, M['neoprene'], 'envelope', thick=.004, edge_fade=.14, rim=envelope_rim)
# the neck: a short rubber tube at the south-west end where the cord was tied, and its tie of tape
cyl('balloon_neck', .05, .3, (-1.72, -.95, .05), M['neoprene_dark'], 'envelope', axis='X', segs=12, r2=.035, tilt=(0, 0, math.radians(20)))
cyl('balloon_neck_tape', .052, .04, (-1.62, -.915, .052), M['tape'], 'envelope', axis='X', segs=12, tilt=(0, 0, math.radians(20)))
# the scrub bush under the north-east corner: a cluster of squashed balls and a few twigs that stick out from under the rubber
for k, (dx, dy, r, h) in enumerate(((0, 0, .5, .42), (.3, .15, .36, .3), (-.28, .2, .33, .28), (.1, -.3, .3, .26), (-.2, -.22, .28, .24), (.42, -.12, .26, .22))):
    ball('balloon_bush_%d' % k, 1., (BUSH.x + dx, BUSH.y + dy, h * .5), M['bush'], 'envelope', scale=(r, r * .9, h * .5), subdiv=1)
for k in range(9):
    a = k * .7 + .3
    cyl('balloon_bush_twig_%d' % k, .006, .5, (BUSH.x + math.cos(a) * .45, BUSH.y + math.sin(a) * .42, .22), M['twig'], 'envelope', segs=5, r2=.002,
        tilt=(math.radians(60 + 10 * math.sin(k)), 0, a + math.pi / 2))

# the parachute: a 1.4 m orange canopy collapsed on the ground, its shroud lines run together to a ring .9 m off to the side
PARA_C = Vector((0, 0, 0))
RING = Vector((-.95, .35, .02))


def canopy_height(x, y):
    d = math.hypot(x, y) / .7
    if d > 1:
        return 0.
    dome = .07 * (1 - d * d)
    a = math.atan2(y, x)
    creases = .045 * max(0., math.sin(9 * a + 1.5 * d)) ** 2 * (1 - d * .5) + .03 * max(0., math.sin(x * 9 + y * 3 + 1)) ** 3
    return dome + creases + fold_noise(x, y, 5.5, .035, 5.)


def canopy_rim(t):
    return .96 + .04 * math.sin(5 * t + .3) + .03 * math.sin(11 * t)


canopy = sheet('balloon_parachute', 1.7, 1.3, 26, 20, canopy_height, M['nylon'], 'parachute', thick=.0025, edge_fade=.16, rim=canopy_rim)
for k in range(8):
    a = k * math.pi / 4 + .2
    edge = Vector((math.cos(a) * .76, math.sin(a) * .56, .004))
    mid = (edge + RING) / 2 + Vector((0, 0, .0 + .03 * math.sin(k)))
    cable('balloon_shroud_%d' % k, [edge, mid, RING], .0018, M['line'], 'parachute', res=4)
cyl('balloon_ring', .03, .006, RING, M['ring'], 'parachute', axis='Y', segs=16, r2=.03, caps=False)
cyl('balloon_ring_bar', .005, .06, RING, M['ring'], 'parachute', axis='Y', segs=8)

# the cord: 2.6 m of white cord from the envelope's neck (south-west of the envelope) toward the parachute's ring, lying in a lazy curve
cable('balloon_cord', [(0, 0, .01), (.5, .18, .012), (1.0, .05, .01), (1.55, -.2, .012), (2.1, -.05, .01), (2.6, .12, .012)], .0045, M['cord'], 'cord', res=8)

# the radiosonde: the box on its side in the dirt, a little tilted, its pivot on the ground under its middle
TILT = (math.radians(-6), math.radians(4), math.radians(28))
BOXZ = .052
body = box('sonde_foam', (.2, .15, .1), (0, 0, BOXZ), M['foam'], 'radiosonde', bevel=.004, rot=TILT)
card = box('sonde_card_sleeve', (.202, .09, .102), (0, 0, BOXZ), M['card'], 'radiosonde', rot=TILT)
box('sonde_card_band', (.03, .152, .102), (.06, 0, BOXZ), M['card'], 'radiosonde', rot=TILT)
box('sonde_tape', (.204, .018, .104), (-.04, 0, BOXZ), M['tape'], 'radiosonde', rot=TILT)
# the printed return label on the top: paper and its plain text (no agency, no logo)
R = Matrix.Rotation(TILT[0], 3, 'X') @ Matrix.Rotation(TILT[1], 3, 'Y') @ Matrix.Rotation(TILT[2], 3, 'Z')
top = Vector((0, 0, BOXZ)) + R @ Vector((-.02, 0, .0505))
lab = box('sonde_label_paper', (.13, .085, .001), (0, 0, 0), M['label'], 'radiosonde')
lab.data.transform(Matrix.Translation(top) @ R.to_4x4())
deg = tuple(math.degrees(a) for a in TILT)
up = R @ Vector((0, 0, 1))
for k, (line, size) in enumerate((('WEATHER INSTRUMENT', .0105), ('HARMLESS', .0095), ('RETURN BY MAIL', .0085), ('POSTAGE PAID', .0085), ('UPPER AIR STATION  BOX 2090', .006))):
    at = top + up * .0012 + R @ Vector((-.02, .03 - k * .0155, 0))
    label('sonde_label_%d' % k, line, at, size, M['ink'], 'radiosonde', rot=deg)
# the sensor boom and wire antenna out of one end, the battery pack on its lead in the dirt beside it
boom_at = Vector((0, 0, BOXZ)) + R @ Vector((.105, .03, .02))
cyl('sonde_boom', .004, .09, boom_at + R @ Vector((.045, 0, 0)), M['foam'], 'radiosonde', axis='X', segs=8, tilt=TILT)
cyl('sonde_antenna', .0012, .34, boom_at + Vector((.2, .06, .04)), M['wire'], 'radiosonde', axis='X', segs=6, tilt=(0, math.radians(-12), math.radians(16)))
cable('sonde_lead', [Vector((0, 0, BOXZ)) + R @ Vector((-.1, -.03, -.02)), (-.16, -.09, .012), (-.22, -.14, .012)], .0022, M['wire'], 'radiosonde', res=5)
box('sonde_battery', (.1, .062, .036), (-.26, -.17, .018), M['battery'], 'radiosonde', bevel=.003, rot=(0, 0, math.radians(-15)))
box('sonde_battery_tape', (.104, .02, .038), (-.26, -.17, .018), M['tape'], 'radiosonde', rot=(0, 0, math.radians(-15)))
# the recovery beacon: a small red lamp dome on the top corner of the box (the game pulses its emissive and a point light here)
BEACON = Vector((0, 0, BOXZ)) + R @ Vector((.07, -.045, .0505))
cyl('sonde_beacon_base', .014, .006, BEACON + up * .003, M['battery'], 'radiosonde', segs=14, tilt=TILT)
ball('sonde_beacon_lamp', .011, BEACON + up * .01, M['lamp'], 'radiosonde', scale=(1, 1, .8), subdiv=2)

bpy.context.view_layer.update()


# ---------- the manifest: bounds and triangles per piece, after modifiers, in Blender's frame (x, y, z up)
def measure(objs):
    dg = bpy.context.evaluated_depsgraph_get()
    lo, hi, tris = [1e9] * 3, [-1e9] * 3, 0
    for ob in objs:
        ev = ob.evaluated_get(dg)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        tris += len(me.loop_triangles)
        for v in me.vertices:
            w = ev.matrix_world @ v.co
            for a in range(3):
                lo[a] = min(lo[a], w[a])
                hi[a] = max(hi[a], w[a])
        ev.to_mesh_clear()
    return [round(c, 4) for c in lo], [round(c, 4) for c in hi], tris


manifest = {'_doc': 'Made by Tools/balloon/build_balloon.py (D48-20261110). Metres, Blender frame: x, y, z up; the glb is exported Y up '
                    '(x, z, -y). Each piece has its pivot on the ground under its own middle; S47Balloon.cpp sets the offsets.',
            'pieces': {}, 'triangles': 0}
for name, objs in PIECES.items():
    lo, hi, tris = measure(objs)
    manifest['pieces'][name] = {'min_m': lo, 'max_m': hi, 'triangles': tris, 'objects': len(objs)}
    manifest['triangles'] += tris
manifest['beacon_m'] = [round(c, 4) for c in (BEACON + up * .01)]
manifest['bush_m'] = [round(c, 3) for c in BUSH]
json.dump(manifest, open(os.path.join(OUT, 'balloon.json'), 'w'), indent=1)
print('BALLOON triangles', manifest['triangles'], {k: v['triangles'] for k, v in manifest['pieces'].items()})

# ---------- export: one glb a piece (Y up: Blender z becomes glTF y, the frame S47Three::ToUE expects)
for name, objs in PIECES.items():
    bpy.ops.object.select_all(action='DESELECT')
    for ob in objs:
        ob.select_set(True)
    bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, name + '.glb'), export_format='GLB', use_selection=True, export_apply=True,
                              export_yup=True, export_materials='EXPORT', export_normals=True, export_texcoords=True, export_animations=False, export_skins=False)

# ---------- renders: Cycles on the CPU, the pieces laid out as the game lays them (S47Balloon.cpp: the same offsets, metres)
LAYOUT = {'envelope': (0, 0, 0), 'parachute': (2.9, -1.9, 0), 'cord': (-1.75, -1.0, 0), 'radiosonde': (1.9, -1.45, 0)}
for name, objs in PIECES.items():
    for ob in objs:
        ob.location = Vector(LAYOUT[name]) + ob.location
# the cord runs from the neck toward the parachute's ring: turn it to point that way
for ob in PIECES['cord']:
    ob.rotation_euler = (0, 0, math.atan2((-1.9 + .35) - (-1.0), (2.9 - .95) - (-1.75)))
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.samples = 64
sc.cycles.use_denoising = True
sc.render.resolution_x, sc.render.resolution_y = 1280, 860
sc.render.film_transparent = False
sc.view_settings.view_transform = 'AgX'
world = bpy.data.worlds.new('w')
sc.world = world
world.use_nodes = True
bg = world.node_tree.nodes['Background']
bg.inputs['Color'].default_value = (.5, .6, .8, 1)
bg.inputs['Strength'].default_value = .7
sun = bpy.data.objects.new('sun', bpy.data.lights.new('sun', 'SUN'))
sun.data.energy = 4.
sun.data.angle = math.radians(3)
sun.rotation_euler = (math.radians(48), math.radians(-10), math.radians(140))
sc.collection.objects.link(sun)
sand = mat('ground_sand', (.52, .43, .3), rough=.95)
ground = box('ground', (16, 16, .02), (1, -.8, -.01), sand, None)
# a scatter of stones and dry grass tufts for scale and ground
for k in range(60):
    a, d = random.uniform(0, 6.28), random.uniform(1.5, 6.5)
    ball('stone_%d' % k, random.uniform(.03, .11), (1 + math.cos(a) * d, -.8 + math.sin(a) * d, .02), M['bush'] if k % 4 else M['twig'], None, scale=(1, .9, .55), subdiv=1)
cam = bpy.data.objects.new('cam', bpy.data.cameras.new('cam'))
sc.collection.objects.link(cam)
sc.camera = cam
cam.data.lens = 35


def aim(cam_ob, at, frm):
    cam_ob.location = frm
    d = Vector(at) - Vector(frm)
    cam_ob.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()


# by day, 3/4 from the south-west and a little above: the whole find
aim(cam, (1.0, -.6, .2), (-3.6, -6.2, 2.6))
sc.render.filepath = os.path.join(OUT, 'render_day34.png')
bpy.ops.render.render(write_still=True)

# at night by flashlight, close on the instrument box, the beacon lit: a spot at the camera, a faint moon, the lamp emitting
bg.inputs['Color'].default_value = (.02, .03, .06, 1)
bg.inputs['Strength'].default_value = .35
sun.data.energy = .06
sun.data.color = (.6, .7, 1.)
M['lamp'].node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value = 25.
beacon_light = bpy.data.objects.new('beacon', bpy.data.lights.new('beacon', 'POINT'))
beacon_light.data.energy = 1.2
beacon_light.data.color = (1., .15, .08)
beacon_light.data.shadow_soft_size = .02
beacon_light.location = Vector(LAYOUT['radiosonde']) + BEACON + up * .03
sc.collection.objects.link(beacon_light)
torch = bpy.data.objects.new('flashlight', bpy.data.lights.new('flashlight', 'SPOT'))
torch.data.energy = 120.
torch.data.color = (1., .9, .72)
torch.data.spot_size = math.radians(38)
torch.data.spot_blend = .5
torch.data.shadow_soft_size = .05
sc.collection.objects.link(torch)
frm, at = (1.9 - 1.9, -1.45 - 1.6, 1.3), (1.9, -1.45, .05)
aim(cam, at, frm)
torch.location = Vector(frm) + Vector((.15, 0, -.2))
torch.rotation_euler = cam.rotation_euler
cam.data.lens = 40
sc.render.filepath = os.path.join(OUT, 'render_night_flashlight.png')
bpy.ops.render.render(write_still=True)

M['lamp'].node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value = 0.
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, 'balloon.blend'))
print('BALLOON done', OUT)
