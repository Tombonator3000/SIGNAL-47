"""DEV-WAVES: the invented "SARO security carbine" of the F10 wave shooter (a dev toy, S47DevWaves.h).

Original work: no real firearm, brand or model is copied; the layout is our own (boxy stamped receiver, vented tube
handguard, skeleton stock, left-hand charging handle, ring-and-post sights, a lamp under the barrel).

Run (through the queue):  blender -b -P build_carbine.py -- <workdir>
Writes <workdir>/Export/SM_DevWaves_*.fbx, <workdir>/textures/carbine_*.png, <workdir>/Preview/*.png and the .blend.
Tools/devwaves/import_assets.py then brings them into /Game/Signal47/Dev/Waves.

Frame (metres): the origin is ON THE SIGHT LINE at the rear aperture; +X to the muzzle, +Z up, +Y the gun's left.
The project's FBX recipe turns Blender (x, y, z) m into Unreal (x, -y, z) cm, so +X stays "ahead" and the left side
(the one the player sees) is Unreal -Y. The bore is 55 mm under the sight line.
"""
import bpy, bmesh, math, sys, os
from mathutils import Matrix
from pathlib import Path

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
WORK = Path(argv[0] if argv else os.path.expanduser('~/scratch-bm/Signal47-devwaves')).resolve()
for sub in ('Export', 'textures', 'Preview'):
    (WORK / sub).mkdir(parents=True, exist_ok=True)
TEX = int(os.environ.get('S47_DEVWAVES_TEX', '2048'))
BORE = -0.055

bpy.ops.wm.read_factory_settings(use_empty=True)
scn = bpy.context.scene
PARTS = {'Carbine': 0, 'Mag': 1, 'Bolt': 2, 'Charge': 3}
made = []  # (object, part index)


# ---------------------------------------------------------------- geometry helpers
def finish(name, bm, mat, part):
    me = bpy.data.meshes.new(name)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    scn.collection.objects.link(ob)
    me.materials.append(mat)
    made.append((ob, part))
    return ob


def sharp_edges(bm, deg=28):
    lim = math.radians(deg)
    return [e for e in bm.edges if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim]


def bevel(bm, width, seg=2, deg=28):
    if width <= 0:
        return
    edges = sharp_edges(bm, deg)
    if edges:
        bmesh.ops.bevel(bm, geom=edges, offset=width, segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)


def box(name, lo, hi, mat, part=0, bev=0.0015, seg=2):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]), verts=bm.verts)
    bmesh.ops.translate(bm, vec=((hi[0] + lo[0]) / 2, (hi[1] + lo[1]) / 2, (hi[2] + lo[2]) / 2), verts=bm.verts)
    bevel(bm, bev, seg)
    return finish(name, bm, mat, part)


def cyl(name, a, b, r, mat, part=0, at=(0.0, BORE), axis='X', n=24, bev=0.0008, r2=None, seg=2):
    """A cylinder from a to b along the axis; `at` is the other two coordinates (y, z) for X, (x, z) for Y, (x, y) for Z."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=n, radius1=r, radius2=r if r2 is None else r2, depth=abs(b - a))
    mid = (a + b) / 2
    if axis == 'X':
        bmesh.ops.rotate(bm, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(90), 3, 'Y'), verts=bm.verts)
        bmesh.ops.translate(bm, vec=(mid, at[0], at[1]), verts=bm.verts)
    elif axis == 'Y':
        bmesh.ops.rotate(bm, cent=(0, 0, 0), matrix=Matrix.Rotation(math.radians(-90), 3, 'X'), verts=bm.verts)
        bmesh.ops.translate(bm, vec=(at[0], mid, at[1]), verts=bm.verts)
    else:
        bmesh.ops.translate(bm, vec=(at[0], at[1], mid), verts=bm.verts)
    bevel(bm, bev, seg, deg=50)
    return finish(name, bm, mat, part)


def prism(name, pts, half_w, mat, part=0, bev=0.002, seg=2, y=0.0):
    """A side profile (x, z) pushed out to both sides."""
    bm = bmesh.new()
    face = bm.faces.new([bm.verts.new((x, y - half_w, z)) for x, z in pts])
    ext = bmesh.ops.extrude_face_region(bm, geom=[face])
    bmesh.ops.translate(bm, vec=(0, 2 * half_w, 0), verts=[g for g in ext['geom'] if isinstance(g, bmesh.types.BMVert)])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bevel(bm, bev, seg, deg=20)
    return finish(name, bm, mat, part)


def only(ob):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob


def cut(ob, cutters):
    """Take the cutters out of ob; the cutters are thrown away."""
    for c in cutters:
        m = ob.modifiers.new('cut', 'BOOLEAN')
        m.operation = 'DIFFERENCE'
        m.object = c
        m.solver = 'EXACT'
        only(ob)
        bpy.ops.object.modifier_apply(modifier=m.name)
    for c in cutters:
        made[:] = [row for row in made if row[0] is not c]
        bpy.data.objects.remove(c, do_unlink=True)


# ---------------------------------------------------------------- materials (procedural, baked to one atlas below)
bake_sockets = {}  # material -> dict(col, rough, metal, emit, bsdf, out, img)


def sock(node, ident, out=False):
    for s in (node.outputs if out else node.inputs):
        if s.identifier == ident:
            return s
    raise KeyError(ident + ' on ' + node.bl_idname + ': ' + str([s.identifier for s in (node.outputs if out else node.inputs)]))


def put(nt, value, target):
    if hasattr(value, 'is_output'):
        nt.links.new(value, target)
    elif isinstance(value, (tuple, list)):
        target.default_value = (value[0], value[1], value[2], 1.0)
    else:
        target.default_value = value


def fmath(nt, op, a, b=None, clamp=False):
    n = nt.nodes.new('ShaderNodeMath')
    n.operation = op
    n.use_clamp = clamp
    put(nt, a, n.inputs[0])
    if b is not None:
        put(nt, b, n.inputs[1])
    return n.outputs[0]


def ramp(nt, value, lo, hi):
    n = nt.nodes.new('ShaderNodeMapRange')
    n.clamp = True
    nt.links.new(value, n.inputs[0])
    n.inputs[1].default_value = lo
    n.inputs[2].default_value = hi
    n.inputs[3].default_value = 0.0
    n.inputs[4].default_value = 1.0
    return n.outputs[0]


def mixc(nt, fac, a, b):
    n = nt.nodes.new('ShaderNodeMix')
    n.data_type = 'RGBA'
    n.clamp_factor = True
    put(nt, fac, sock(n, 'Factor_Float'))
    put(nt, a, sock(n, 'A_Color'))
    put(nt, b, sock(n, 'B_Color'))
    return sock(n, 'Result_Color', True)


def mixf(nt, fac, a, b):
    n = nt.nodes.new('ShaderNodeMix')
    n.data_type = 'FLOAT'
    n.clamp_factor = True
    put(nt, fac, sock(n, 'Factor_Float'))
    put(nt, a, sock(n, 'A_Float'))
    put(nt, b, sock(n, 'B_Float'))
    return sock(n, 'Result_Float', True)


def noise(nt, coord, scale, detail=5.0, rough=0.6, stretch=None):
    n = nt.nodes.new('ShaderNodeTexNoise')
    n.inputs['Scale'].default_value = scale
    n.inputs['Detail'].default_value = detail
    n.inputs['Roughness'].default_value = rough
    if stretch:
        m = nt.nodes.new('ShaderNodeMapping')
        m.inputs['Scale'].default_value = stretch
        nt.links.new(coord, m.inputs['Vector'])
        nt.links.new(m.outputs[0], n.inputs['Vector'])
    else:
        nt.links.new(coord, n.inputs['Vector'])
    return n.outputs[0]


def material(name, base, metal, rough, worn, worn_metal, worn_rough, wear=1.0, scratch=0.5, dust=0.6, grain=0.35, grain_scale=1400.0):
    """base under worn edges, scratches along the gun, desert dust in the corners, a fine grain in the normal."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    co = nt.nodes.new('ShaderNodeTexCoord').outputs['Object']
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    # the edges: where a wide bevel's normal leaves the face's own
    wide = nt.nodes.new('ShaderNodeBevel')
    wide.samples = 8
    wide.inputs['Radius'].default_value = 0.0035
    dot = nt.nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    nt.links.new(wide.outputs['Normal'], dot.inputs[0])
    nt.links.new(geo.outputs['Normal'], dot.inputs[1])
    edge = ramp(nt, dot.outputs['Value'], 0.985, 0.86)
    patch = ramp(nt, noise(nt, co, 70.0, 6.0, 0.65), 0.40, 0.66)
    edge_wear = fmath(nt, 'MULTIPLY', edge, fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', patch, 0.8), 0.2), True)
    # scratches: long along the gun, thin across it; and a few rubbed patches on the flats
    scr = ramp(nt, noise(nt, co, 1.0, 3.0, 0.5, stretch=(6.0, 420.0, 420.0)), 0.655, 0.70)
    rub = ramp(nt, noise(nt, co, 26.0, 4.0, 0.7), 0.63, 0.74)
    flat_wear = fmath(nt, 'MAXIMUM', fmath(nt, 'MULTIPLY', scr, scratch), fmath(nt, 'MULTIPLY', rub, 0.35 * scratch))
    w = fmath(nt, 'MULTIPLY', fmath(nt, 'MAXIMUM', edge_wear, flat_wear), wear, True)
    # dust: in the corners (occlusion), a little everywhere on top
    ao = nt.nodes.new('ShaderNodeAmbientOcclusion')
    ao.samples = 8
    ao.inputs['Distance'].default_value = 0.012
    corner = ramp(nt, ao.outputs['AO'], 0.92, 0.45)
    speck = ramp(nt, noise(nt, co, 240.0, 3.0, 0.6), 0.52, 0.72)
    d = fmath(nt, 'MULTIPLY', fmath(nt, 'ADD', corner, fmath(nt, 'MULTIPLY', speck, 0.16)), dust, True)
    # tone: a slow shift so no flat is one flat value
    tone = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', noise(nt, co, 9.0, 3.0, 0.5), 0.5), 0.75)
    col = mixc(nt, w, base, worn)
    toned = nt.nodes.new('ShaderNodeMix')
    toned.data_type = 'RGBA'
    toned.blend_type = 'MULTIPLY'
    sock(toned, 'Factor_Float').default_value = 1.0
    nt.links.new(col, sock(toned, 'A_Color'))
    nt.links.new(tone, sock(toned, 'B_Color'))
    col = mixc(nt, fmath(nt, 'MULTIPLY', d, 0.62), sock(toned, 'Result_Color', True), (0.30, 0.22, 0.15))
    r = mixf(nt, w, rough, worn_rough)
    r = fmath(nt, 'ADD', r, fmath(nt, 'MULTIPLY', fmath(nt, 'SUBTRACT', noise(nt, co, 55.0, 5.0, 0.6), 0.5), 0.22))
    r = fmath(nt, 'ADD', r, fmath(nt, 'MULTIPLY', d, 0.30), True)
    m = fmath(nt, 'MULTIPLY', mixf(nt, w, metal, worn_metal), fmath(nt, 'SUBTRACT', 1.0, fmath(nt, 'MULTIPLY', d, 0.7)), True)
    # the normal: the surface's grain, scratches cut in, then a small round on every hard edge
    height = fmath(nt, 'SUBTRACT', fmath(nt, 'MULTIPLY', noise(nt, co, grain_scale, 2.0, 0.5), grain), fmath(nt, 'MULTIPLY', scr, 0.6 * scratch))
    bump = nt.nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.5
    bump.inputs['Distance'].default_value = 0.00025
    nt.links.new(height, bump.inputs['Height'])
    small = nt.nodes.new('ShaderNodeBevel')
    small.samples = 8
    small.inputs['Radius'].default_value = 0.0011
    nt.links.new(bump.outputs['Normal'], small.inputs['Normal'])
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    nt.links.new(col, bsdf.inputs['Base Color'])
    nt.links.new(r, bsdf.inputs['Roughness'])
    nt.links.new(m, bsdf.inputs['Metallic'])
    nt.links.new(small.outputs['Normal'], bsdf.inputs['Normal'])
    emit = nt.nodes.new('ShaderNodeEmission')
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(bsdf.outputs[0], out.inputs['Surface'])
    img = nt.nodes.new('ShaderNodeTexImage')
    bake_sockets[mat] = {'col': col, 'rough': r, 'metal': m, 'emit': emit, 'bsdf': bsdf, 'out': out, 'img': img, 'nt': nt}
    return mat


# (a phosphated finish is a dull dark coat, not bare metal: in daylight metallic 0.9 at roughness 0.5 read as polished silver down the sights;
# only the worn edges and scratches show the steel under it)
STEEL = material('steel', (0.034, 0.036, 0.040), 0.35, 0.70, (0.46, 0.46, 0.47), 1.0, 0.30, wear=1.0, scratch=0.6, dust=0.55, grain=0.5, grain_scale=1700.0)
POLY = material('polymer', (0.022, 0.024, 0.022), 0.0, 0.62, (0.075, 0.078, 0.072), 0.0, 0.40, wear=0.8, scratch=0.45, dust=0.75, grain=1.0, grain_scale=900.0)
BRIGHT = material('bright', (0.36, 0.36, 0.37), 1.0, 0.30, (0.62, 0.62, 0.63), 1.0, 0.20, wear=0.7, scratch=0.9, dust=0.35, grain=0.2)
MAGM = material('magsteel', (0.040, 0.046, 0.058), 0.45, 0.48, (0.42, 0.42, 0.44), 1.0, 0.28, wear=1.0, scratch=1.0, dust=0.5, grain=0.3)
RUBBER = material('rubber', (0.012, 0.012, 0.012), 0.0, 0.86, (0.05, 0.05, 0.05), 0.0, 0.7, wear=0.5, scratch=0.2, dust=0.9, grain=1.4, grain_scale=600.0)
PAINT = material('paint', (0.62, 0.60, 0.52), 0.0, 0.6, (0.05, 0.05, 0.055), 0.6, 0.45, wear=1.6, scratch=1.6, dust=0.5, grain=0.3)
LENS = material('lens', (0.75, 0.74, 0.66), 0.0, 0.08, (0.75, 0.74, 0.66), 0.0, 0.2, wear=0.0, scratch=0.2, dust=0.3, grain=0.0)

# ---------------------------------------------------------------- the carbine
# receiver: a boxy stamped body with a rounded top cover
receiver = box('receiver', (-0.060, -0.019, -0.085), (0.200, 0.019, -0.022), STEEL, bev=0.0028, seg=3)
box('cover', (-0.056, -0.0150, -0.026), (0.196, 0.0150, -0.0085), STEEL, bev=0.0050, seg=3)
box('cover_rib_a', (0.030, -0.0158, -0.0245), (0.036, 0.0158, -0.0078), STEEL, bev=0.0012)
box('cover_rib_b', (0.120, -0.0158, -0.0245), (0.126, 0.0158, -0.0078), STEEL, bev=0.0012)
# right side: the ejection port (a pocket the bolt shows in); left side: the charging handle's track
port = box('c_port', (0.055, -0.030, -0.052), (0.135, 0.000, -0.030), STEEL, bev=0)
track = box('c_track', (0.058, 0.0172, -0.0345), (0.162, 0.030, -0.0275), STEEL, bev=0)
cut(receiver, [port, track])
box('deflector', (0.038, -0.0228, -0.052), (0.052, -0.0185, -0.034), STEEL, bev=0.0012)
for i, (px, pz) in enumerate(((0.018, -0.070), (0.076, -0.076), (0.166, -0.062), (-0.040, -0.050))):
    cyl('pin%d' % i, -0.0197, 0.0197, 0.0030, BRIGHT, at=(px, pz), axis='Y', n=14, bev=0.0004)
box('selector', (0.004, 0.0188, -0.064), (0.030, 0.0212, -0.058), STEEL, bev=0.0008)
cyl('selector_hub', 0.0188, 0.0222, 0.0052, STEEL, at=(0.006, -0.061), axis='Y', n=16, bev=0.0005)
box('magwell', (0.092, -0.0178, -0.110), (0.152, 0.0178, -0.084), STEEL, bev=0.0022, seg=3)
box('mag_catch', (0.086, 0.0176, -0.104), (0.094, 0.0205, -0.090), STEEL, bev=0.0008)
# the stamped name: ours (an invented service number)
try:
    cu = bpy.data.curves.new('stamp', 'FONT')
    cu.body = 'SARO SEC-7   CAL 7.2   No 0047'
    cu.size = 0.0052
    cu.extrude = 0.00025
    stamp = bpy.data.objects.new('stamp', cu)
    scn.collection.objects.link(stamp)
    stamp.rotation_euler = (math.radians(90), 0, math.radians(180))
    stamp.location = (0.085, 0.01915, -0.0475)
    only(stamp)
    bpy.ops.object.convert(target='MESH')
    stamp = bpy.context.view_layer.objects.active
    stamp.data.materials.append(PAINT)
    made.append((stamp, 0))
except Exception as exc:  # a build without the font: no stamp
    print('S47 DEVWAVES: no stamp:', exc)

# barrel, muzzle device, gas tube
cyl('barrel', 0.195, 0.598, 0.0085, STEEL, n=24, bev=0.0006)
cyl('chamber', 0.195, 0.262, 0.0112, STEEL, n=24, bev=0.0008)
muzzle = cyl('muzzle', 0.588, 0.650, 0.0120, STEEL, n=28, bev=0.0012)
cut(muzzle, [cyl('c_bore', 0.612, 0.660, 0.0046, STEEL, n=20, bev=0),
             box('c_slot_y', (0.606, -0.03, BORE - 0.0022), (0.637, 0.03, BORE + 0.0022), STEEL, bev=0),
             box('c_slot_z', (0.606, -0.0022, BORE - 0.03), (0.637, 0.0022, BORE + 0.03), STEEL, bev=0)])
cyl('muzzle_ring', 0.584, 0.590, 0.0128, STEEL, n=28, bev=0.0008)
cyl('gastube', 0.200, 0.470, 0.0042, STEEL, at=(0.0, -0.0362), n=14, bev=0.0004)

# handguard: a vented polymer tube, steel rings at both ends
guard = cyl('handguard', 0.208, 0.442, 0.0240, POLY, n=28, bev=0.0016)
slots = []
for i in range(6):
    x = 0.232 + i * 0.034
    slots.append(box('c_vy%d' % i, (x, -0.04, BORE - 0.0038), (x + 0.021, 0.04, BORE + 0.0038), POLY, bev=0))
    slots.append(box('c_vz%d' % i, (x + 0.004, -0.0032, BORE), (x + 0.017, 0.0032, BORE + 0.04), POLY, bev=0))
cut(guard, slots)
for i in range(5):  # grip ribs under the tube
    x = 0.246 + i * 0.034
    box('rib%d' % i, (x + 0.022, -0.0125, BORE - 0.0252), (x + 0.032, 0.0125, BORE - 0.0205), POLY, bev=0.0012)
cyl('guard_ring_a', 0.200, 0.209, 0.0252, STEEL, n=28, bev=0.0009)
cyl('guard_ring_b', 0.441, 0.450, 0.0252, STEEL, n=28, bev=0.0009)

# sights: a ring between two ears at the back, a post between two ears at the front; both ON the sight line (z = 0)
box('rear_base', (-0.020, -0.0125, -0.0105), (0.020, 0.0125, -0.0070), STEEL, bev=0.0012)
for s in (-1, 1):
    prism('rear_ear%d' % s, [(-0.014, -0.0085), (0.014, -0.0085), (0.012, 0.0075), (0.004, 0.0125), (-0.004, 0.0125), (-0.012, 0.0075)], 0.0011, STEEL, bev=0.0005, y=s * 0.0128)
bpy.ops.mesh.primitive_torus_add(major_radius=0.0080, minor_radius=0.0017, major_segments=28, minor_segments=10, location=(0, 0, 0), rotation=(0, math.radians(90), 0))
ring = bpy.context.view_layer.objects.active
ring.name = 'rear_ring'
ring.data.materials.append(STEEL)
made.append((ring, 0))
box('rear_stem', (-0.0016, -0.0022, -0.0105), (0.0016, 0.0022, -0.0078), STEEL, bev=0.0004)
box('front_base', (0.466, -0.0105, BORE - 0.0125), (0.502, 0.0105, BORE + 0.019), STEEL, bev=0.0022, seg=3)
prism('front_tower', [(0.470, BORE + 0.018), (0.498, BORE + 0.018), (0.492, -0.0135), (0.476, -0.0135)], 0.0062, STEEL, bev=0.0012)
for s in (-1, 1):
    prism('front_ear%d' % s, [(0.476, -0.0150), (0.492, -0.0150), (0.4905, 0.0040), (0.487, 0.0072), (0.481, 0.0072), (0.4775, 0.0040)], 0.0010, STEEL, bev=0.0004, y=s * 0.0086)
box('front_post', (0.4828, -0.0011, -0.0150), (0.4852, 0.0011, 0.0000), STEEL, bev=0.0003, seg=1)

# grip, trigger, guard
prism('grip', [(0.020, -0.084), (-0.036, -0.084), (-0.066, -0.176), (-0.060, -0.184), (-0.026, -0.188), (-0.018, -0.182), (0.004, -0.112)], 0.0138, POLY, bev=0.0042, seg=3)
box('tguard_low', (0.014, -0.0062, -0.1262), (0.092, 0.0062, -0.1232), STEEL, bev=0.0008)
prism('tguard_front', [(0.088, -0.1262), (0.092, -0.1262), (0.097, -0.086), (0.093, -0.086)], 0.0062, STEEL, bev=0.0008)
prism('trigger', [(0.050, -0.084), (0.057, -0.084), (0.0555, -0.099), (0.050, -0.111), (0.0465, -0.109), (0.0505, -0.098)], 0.0030, BRIGHT, bev=0.0008)

# stock: a skeleton of polymer with a rubber pad
stock = prism('stock', [(-0.058, -0.024), (-0.300, -0.030), (-0.300, -0.152), (-0.282, -0.153), (-0.058, -0.083)], 0.0125, POLY, bev=0.0034, seg=3)
cut(stock, [prism('c_stock', [(-0.105, -0.046), (-0.276, -0.050), (-0.276, -0.128), (-0.118, -0.080)], 0.03, POLY, bev=0)])
box('buttpad', (-0.313, -0.0152, -0.156), (-0.299, 0.0152, -0.026), RUBBER, bev=0.0034, seg=3)
cyl('stock_collar', -0.066, -0.056, 0.0205, STEEL, at=(0.0, -0.052), n=24, bev=0.001)

# the lamp under the barrel (its light and glow are the game's)
box('lamp_mount', (0.392, -0.0085, -0.0885), (0.438, 0.0085, -0.0770), STEEL, bev=0.0012)
cyl('lamp_body', 0.372, 0.456, 0.0128, STEEL, at=(0.0, -0.0990), n=24, bev=0.001)
cyl('lamp_head', 0.452, 0.480, 0.0162, STEEL, at=(0.0, -0.0990), n=28, bev=0.0014)
cyl('lamp_lens', 0.4796, 0.4812, 0.0134, LENS, at=(0.0, -0.0990), n=28, bev=0)
cyl('lamp_switch', -0.0990 - 0.0128 - 0.003, -0.0990 - 0.0128 + 0.001, 0.0040, RUBBER, at=(0.385, 0.0), axis='Z', n=14, bev=0.0006)

# ---- the moving parts (their own meshes, in the gun's frame)
mag_pts = []
for side in (0, 1):
    for k in range(9):
        t = k / 8 if side == 0 else 1 - k / 8
        z = -0.090 - 0.150 * t
        shift = 0.026 * t ** 1.7
        mag_pts.append(((0.099 if side == 0 else 0.146) + shift, z))
prism('mag_body', mag_pts, 0.0108, MAGM, part=1, bev=0.0016)
for s in (-1, 1):
    for k in range(3):
        box('mag_rib%d_%d' % (s, k), (0.118 + k * 0.004, s * 0.0106 - 0.0006, -0.215 + k * 0.004), (0.122 + k * 0.004, s * 0.0106 + 0.0006, -0.115), MAGM, part=1, bev=0.0004, seg=1)
box('mag_plate', (0.121, -0.0122, -0.2445), (0.176, 0.0122, -0.2385), MAGM, part=1, bev=0.0014)
cyl('bolt', 0.046, 0.142, 0.0105, BRIGHT, part=2, n=20, bev=0.001)
cyl('bolt_lug', 0.070, 0.086, 0.0112, BRIGHT, part=2, n=20, bev=0.0008)
cyl('charge_arm', 0.0165, 0.0300, 0.0036, BRIGHT, part=3, at=(0.150, -0.031), axis='Y', n=14, bev=0.0005)
cyl('charge_knob', 0.0285, 0.0390, 0.0062, STEEL, part=3, at=(0.150, -0.031), axis='Y', n=18, bev=0.0016)

# ---------------------------------------------------------------- one mesh, triangles, weighted normals, one atlas
for ob, part in made:
    me = ob.data
    attr = me.attributes.get('part') or me.attributes.new('part', 'INT', 'FACE')
    attr.data.foreach_set('value', [part] * len(me.polygons))
for o in bpy.context.view_layer.objects:
    o.select_set(False)
for ob, _ in made:
    ob.select_set(True)
bpy.context.view_layer.objects.active = made[0][0]
bpy.ops.object.join()
gun = bpy.context.view_layer.objects.active
gun.name = 'DevWavesCarbine'
bm = bmesh.new()
bm.from_mesh(gun.data)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.00002)
bmesh.ops.triangulate(bm, faces=bm.faces)
bm.to_mesh(gun.data)
bm.free()
me = gun.data
me.polygons.foreach_set('use_smooth', [True] * len(me.polygons))
try:
    me.set_sharp_from_angle(angle=math.radians(42))
except Exception as exc:
    print('S47 DEVWAVES: set_sharp_from_angle:', exc)
only(gun)
wn = gun.modifiers.new('wn', 'WEIGHTED_NORMAL')
wn.keep_sharp = True
wn.weight = 60
try:
    bpy.ops.object.modifier_apply(modifier=wn.name)
except Exception as exc:
    print('S47 DEVWAVES: weighted normals:', exc)
if not me.uv_layers:
    me.uv_layers.new(name='UVMap')
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.smart_project(angle_limit=math.radians(62), island_margin=0.0022, area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
bpy.ops.object.mode_set(mode='OBJECT')
print('S47 DEVWAVES: mesh', len(me.vertices), 'verts', len(me.polygons), 'tris')

# ---------------------------------------------------------------- bake
scn.render.engine = 'CYCLES'
scn.cycles.samples = int(os.environ.get('S47_DEVWAVES_SAMPLES', '24'))
try:
    prefs = bpy.context.preferences.addons['cycles'].preferences
    for kind in ('OPTIX', 'CUDA'):
        try:
            prefs.compute_device_type = kind
            prefs.get_devices()
            if any(d.type == kind for d in prefs.devices):
                for d in prefs.devices:
                    d.use = d.type == kind
                scn.cycles.device = 'GPU'
                print('S47 DEVWAVES: cycles on', kind)
                break
        except Exception as exc:
            print('S47 DEVWAVES: no', kind, exc)
except Exception as exc:
    print('S47 DEVWAVES: cycles on the CPU:', exc)
scn.render.bake.margin = 6
scn.render.bake.use_clear = True
scn.render.bake.use_selected_to_active = False
images = {}
for key, space in (('basecolor', 'sRGB'), ('roughness', 'Non-Color'), ('metallic', 'Non-Color'), ('normal', 'Non-Color')):
    img = bpy.data.images.new('carbine_' + key, TEX, TEX, alpha=False)
    img.colorspace_settings.name = space
    images[key] = img
only(gun)


def bake(key):
    for mat, row in bake_sockets.items():
        nt = row['nt']
        row['img'].image = images[key]
        for n in nt.nodes:
            n.select = False
        row['img'].select = True
        nt.nodes.active = row['img']
        for link in list(row['out'].inputs['Surface'].links):
            nt.links.remove(link)
        if key == 'normal':
            nt.links.new(row['bsdf'].outputs[0], row['out'].inputs['Surface'])
        else:
            for link in list(row['emit'].inputs['Color'].links):
                nt.links.remove(link)
            nt.links.new(row[{'basecolor': 'col', 'roughness': 'rough', 'metallic': 'metal'}[key]], row['emit'].inputs['Color'])
            nt.links.new(row['emit'].outputs[0], row['out'].inputs['Surface'])
    if key == 'normal':
        # tangent space, DirectX green (the project's import recipe reads it without a flip)
        bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', normal_r='POS_X', normal_g='NEG_Y', normal_b='POS_Z')
    else:
        bpy.ops.object.bake(type='EMIT')
    img = images[key]
    img.filepath_raw = str(WORK / 'textures' / ('carbine_' + key + '.png'))
    img.file_format = 'PNG'
    img.save()
    print('S47 DEVWAVES: baked', key)


for key in ('basecolor', 'roughness', 'metallic', 'normal'):
    bake(key)

# ---------------------------------------------------------------- the game's material: the baked atlas (also what the preview shows)
final = bpy.data.materials.new('M_DevWaves_Carbine')
final.use_nodes = True
nt = final.node_tree
bsdf = next(n for n in nt.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
for key, target in (('basecolor', 'Base Color'), ('roughness', 'Roughness'), ('metallic', 'Metallic')):
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = images[key]
    nt.links.new(t.outputs['Color'], bsdf.inputs[target])
t = nt.nodes.new('ShaderNodeTexImage')
t.image = images['normal']
flip = nt.nodes.new('ShaderNodeSeparateColor')
back = nt.nodes.new('ShaderNodeCombineColor')
inv = nt.nodes.new('ShaderNodeMath')
inv.operation = 'SUBTRACT'
inv.inputs[0].default_value = 1.0
nt.links.new(t.outputs['Color'], flip.inputs[0])
nt.links.new(flip.outputs[0], back.inputs[0])
nt.links.new(flip.outputs[1], inv.inputs[1])
nt.links.new(inv.outputs[0], back.inputs[1])
nt.links.new(flip.outputs[2], back.inputs[2])
nm = nt.nodes.new('ShaderNodeNormalMap')
nt.links.new(back.outputs[0], nm.inputs['Color'])
nt.links.new(nm.outputs['Normal'], bsdf.inputs['Normal'])
me.materials.clear()
me.materials.append(final)
me.polygons.foreach_set('material_index', [0] * len(me.polygons))
me.update()


def export(ob, name):
    ob.name = name
    ob.data.name = name
    only(ob)
    bpy.ops.export_scene.fbx(filepath=str(WORK / 'Export' / (name + '.fbx')), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False, add_leaf_bones=False,
                             mesh_smooth_type='FACE', use_tspace=True)
    print('S47 DEVWAVES: exported', name, len(ob.data.polygons), 'tris')


pieces = []
for pname, index in PARTS.items():
    ob = gun.copy()
    ob.data = gun.data.copy()
    scn.collection.objects.link(ob)
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    layer = bm.faces.layers.int.get('part')
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f[layer] != index], context='FACES')
    bm.to_mesh(ob.data)
    bm.free()
    export(ob, 'SM_DevWaves_' + pname)
    pieces.append(ob)

# ---------------------------------------------------------------- small extras with plain materials: a casing, a supply crate
made.clear()
BRASS = bpy.data.materials.new('M_DevWaves_Brass')
CRATE = bpy.data.materials.new('M_DevWaves_Crate')
CRATE_TRIM = bpy.data.materials.new('M_DevWaves_CrateTrim')


def simple(obs, name):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for o in obs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = obs[0]
    if len(obs) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    bm2 = bmesh.new()
    bm2.from_mesh(ob.data)
    bmesh.ops.triangulate(bm2, faces=bm2.faces)
    bm2.to_mesh(ob.data)
    bm2.free()
    ob.data.polygons.foreach_set('use_smooth', [True] * len(ob.data.polygons))
    try:
        ob.data.set_sharp_from_angle(angle=math.radians(42))
    except Exception:
        pass
    if not ob.data.uv_layers:
        ob.data.uv_layers.new(name='UVMap')
    only(ob)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(62), island_margin=0.01)
    bpy.ops.object.mode_set(mode='OBJECT')
    export(ob, name)
    ob.hide_render = True
    return ob


# the casing: along X, its middle at the origin (an invented 7.2 x 33 round)
simple([cyl('case_body', -0.0165, 0.0125, 0.0048, BRASS, at=(0, 0), n=14, bev=0.0004),
        cyl('case_neck', 0.0125, 0.0165, 0.0048, BRASS, at=(0, 0), n=14, bev=0.0003, r2=0.0040),
        cyl('case_rim', -0.0178, -0.0165, 0.0050, BRASS, at=(0, 0), n=14, bev=0.0002)], 'SM_DevWaves_Casing')
made.clear()
# the supply crate: a steel box with a lid, clasps and two handles; its foot at the origin
crate = [box('crate_body', (-0.26, -0.17, 0.0), (0.26, 0.17, 0.24), CRATE, bev=0.006, seg=2),
         box('crate_lid', (-0.268, -0.178, 0.24), (0.268, 0.178, 0.30), CRATE, bev=0.008, seg=2),
         box('crate_band_a', (-0.20, -0.1815, 0.02), (-0.16, 0.1815, 0.305), CRATE_TRIM, bev=0.002),
         box('crate_band_b', (0.16, -0.1815, 0.02), (0.20, 0.1815, 0.305), CRATE_TRIM, bev=0.002),
         box('crate_clasp_a', (-0.07, -0.186, 0.20), (-0.03, -0.176, 0.27), CRATE_TRIM, bev=0.002),
         box('crate_clasp_b', (0.03, -0.186, 0.20), (0.07, -0.176, 0.27), CRATE_TRIM, bev=0.002),
         box('crate_handle_a', (-0.285, -0.06, 0.15), (-0.262, 0.06, 0.175), CRATE_TRIM, bev=0.004),
         box('crate_handle_b', (0.262, -0.06, 0.15), (0.285, 0.06, 0.175), CRATE_TRIM, bev=0.004)]
simple(crate, 'SM_DevWaves_Crate')

bpy.ops.wm.save_as_mainfile(filepath=str(WORK / 'DevWavesCarbine.blend'))

# ---------------------------------------------------------------- previews: the baked gun as the game will light it
gun.hide_render = True
world = bpy.data.worlds.new('w')
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (0.02, 0.022, 0.03, 1)
scn.world = world
for loc, energy, col, size in (((0.2, 0.9, 0.7), 22, (1.0, 0.93, 0.82), 0.6), ((0.9, -0.5, 0.3), 9, (0.7, 0.8, 1.0), 0.8), ((-0.6, 0.3, -0.3), 5, (1.0, 0.8, 0.6), 0.6)):
    ld = bpy.data.lights.new('l', 'AREA')
    ld.energy = energy
    ld.color = col
    ld.size = size
    lo = bpy.data.objects.new('l', ld)
    scn.collection.objects.link(lo)
    lo.location = loc
    lo.rotation_euler = (Matrix.Translation(loc).inverted() @ Matrix.Identity(4)).to_euler()
    con = lo.constraints.new('TRACK_TO')
    con.target = pieces[0]
    con.track_axis = 'TRACK_NEGATIVE_Z'
    con.up_axis = 'UP_Y'
cam_d = bpy.data.cameras.new('c')
cam = bpy.data.objects.new('c', cam_d)
scn.collection.objects.link(cam)
scn.camera = cam
aim = bpy.data.objects.new('aim', None)
scn.collection.objects.link(aim)
con = cam.constraints.new('TRACK_TO')
con.target = aim
con.track_axis = 'TRACK_NEGATIVE_Z'
con.up_axis = 'UP_Y'
scn.render.resolution_x = 1600
scn.render.resolution_y = 900
scn.cycles.samples = 48
scn.render.image_settings.file_format = 'PNG'
for name, loc, at, lens in (('left', (0.17, 1.05, 0.06), (0.17, 0.0, -0.07), 55), ('right', (0.17, -1.05, 0.06), (0.17, 0.0, -0.07), 55),
                            ('player', (-0.42, 0.20, 0.13), (0.30, 0.0, -0.05), 32), ('sights', (-0.22, 0.0, 0.0), (0.5, 0.0, 0.0), 40),
                            ('top', (0.17, 0.25, 0.9), (0.17, 0.0, -0.05), 55)):
    cam.location = loc
    aim.location = at
    cam_d.lens = lens
    cam_d.clip_start = 0.01
    scn.render.filepath = str(WORK / 'Preview' / ('carbine_' + name + '.png'))
    bpy.ops.render.render(write_still=True)
print('S47 DEVWAVES: CARBINE BUILD DONE')
