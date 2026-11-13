"""RECORDS-ROOM-LIFT-20261113: the SARO records room's own models (Blender 5.2, original geometry; Tom's reference sheet
~/scratch-bm/refs/records-room-20261113 for surfaces, fittings and props).

  blender -b -P build_records.py -- <workdir> <asset> [<asset> ...]      (through the queue: ArtSource/Lived/run_blender.py)
Writes <workdir>/Export/SM_Rec_*.fbx, <workdir>/textures/<asset>_*.png, <workdir>/Preview/*.png and contract.json.
Tools/recordsroom/import_assets.py then brings them into /Game/RecordsRoom.

Frames. Room pieces (ceiling, fittings) are built where they stand, origin at SARO's origin: SARO metres (x, y up, z) are Blender
(x, -z, y), and the project's FBX recipe turns Blender (x, y, z) m into Unreal (x, -y, z) cm = S47Three::ToUE. Props stand on
their own origin (middle of the base), front to Blender -Y (Unreal +Y after import), right +X, up +Z.
Texture sources: Tools/recordsroom/make_surfaces.py's output (S47_REC_SRC/tex) and three CC0 Poly Haven sets
(S47_REC_SRC/dl/polyhaven: wood_table_001, green_metal_rust, rusty_metal_02).
"""
import bpy, bmesh, json, math, os, random, sys
from mathutils import Matrix, Vector
from pathlib import Path

argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
WORK = Path(argv[0] if argv else os.path.expanduser('~/scratch-bm/records-room-20261113/work')).resolve()
WANT = argv[1:]
SRC = Path(os.path.expanduser(os.environ.get('S47_REC_SRC', '~/scratch-bm/records-room-20261113'))).resolve()
TEXDIR, PH = SRC / 'tex', SRC / 'dl' / 'polyhaven'
SAMPLES = int(os.environ.get('S47_REC_SAMPLES', '16'))
PREVIEW = os.environ.get('S47_REC_PREVIEW', '1') != '0'
for sub in ('Export', 'textures', 'Preview'):
    (WORK / sub).mkdir(parents=True, exist_ok=True)
scn = None
made = []           # (object, part index)
bake_sockets = {}   # material -> its sockets for the bake


def reset():
    global scn
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scn = bpy.context.scene
    made.clear()
    bake_sockets.clear()


# ---------------------------------------------------------------- geometry helpers
def finish(name, bm, mat, part=0):
    me = bpy.data.meshes.new(name)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    scn.collection.objects.link(ob)
    me.materials.append(mat)
    made.append((ob, part))
    return ob


def bevel(bm, width, seg=2, deg=28):
    if width <= 0:
        return
    lim = math.radians(deg)
    edges = [e for e in bm.edges if len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim]
    if edges:
        bmesh.ops.bevel(bm, geom=edges, offset=width, segments=seg, profile=0.5, affect='EDGES', clamp_overlap=True)


def box(name, lo, hi, mat, part=0, bev=0.0015, seg=2):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]), verts=bm.verts)
    bmesh.ops.translate(bm, vec=((hi[0] + lo[0]) / 2, (hi[1] + lo[1]) / 2, (hi[2] + lo[2]) / 2), verts=bm.verts)
    bevel(bm, bev, seg)
    return finish(name, bm, mat, part)


def cyl(name, a, b, r, mat, part=0, at=(0.0, 0.0), axis='Z', n=24, bev=0.0008, r2=None, seg=2):
    """A cylinder from a to b along the axis; `at` is the other two coordinates: (y, z) for X, (x, z) for Y, (x, y) for Z."""
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


def prism_x(name, pts, x0, x1, mat, part=0, bev=0.002, seg=2):
    """A side profile (y, z) pushed along X from x0 to x1."""
    bm = bmesh.new()
    face = bm.faces.new([bm.verts.new((x0, y, z)) for y, z in pts])
    ext = bmesh.ops.extrude_face_region(bm, geom=[face])
    bmesh.ops.translate(bm, vec=(x1 - x0, 0, 0), verts=[g for g in ext['geom'] if isinstance(g, bmesh.types.BMVert)])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bevel(bm, bev, seg, deg=20)
    return finish(name, bm, mat, part)


def quad(name, pts, mat, part=0, uvs=None):
    """One flat face from its corners (counter-clockwise seen from its front); uvs: a corner each."""
    bm = bmesh.new()
    f = bm.faces.new([bm.verts.new(p) for p in pts])
    if uvs:
        layer = bm.loops.layers.uv.new('UVMap')
        for loop, uv in zip(f.loops, uvs):
            loop[layer].uv = uv
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    scn.collection.objects.link(ob)
    me.materials.append(mat)
    made.append((ob, part))
    return ob


def pipe(name, pts, r, mat, part=0, n=8):
    """A round tube along a polyline (its corners eased)."""
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    cu.bevel_depth = r
    cu.bevel_resolution = max(0, n // 4 - 1)
    cu.use_fill_caps = True
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for p, c in zip(sp.points, pts):
        p.co = (c[0], c[1], c[2], 1.0)
    ob = bpy.data.objects.new(name, cu)
    scn.collection.objects.link(ob)
    only(ob)
    bpy.ops.object.convert(target='MESH')
    ob = bpy.context.view_layer.objects.active
    ob.data.materials.append(mat)
    made.append((ob, part))
    return ob


def text(name, body, size, mat, loc, rot, part=0, extrude=0.0002, align='CENTER'):
    try:
        cu = bpy.data.curves.new(name, 'FONT')
        cu.body = body
        cu.size = size
        cu.extrude = extrude
        cu.resolution_u = 2
        cu.align_x = align
        ob = bpy.data.objects.new(name, cu)
        scn.collection.objects.link(ob)
        ob.rotation_euler = rot
        ob.location = loc
        only(ob)
        bpy.ops.object.convert(target='MESH')
        ob = bpy.context.view_layer.objects.active
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        ob.data.materials.append(mat)
        made.append((ob, part))
        return ob
    except Exception as exc:
        print('S47 REC: no text', body, exc)
        return None


def xf(ob, m):
    ob.data.transform(m)
    ob.data.update()
    return ob


def rot_about(ob, axis, deg, pivot=(0, 0, 0)):
    p = Vector(pivot)
    return xf(ob, Matrix.Translation(p) @ Matrix.Rotation(math.radians(deg), 4, axis) @ Matrix.Translation(-p))


def move(ob, v):
    return xf(ob, Matrix.Translation(Vector(v)))


def only(ob):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob


def dent(ob, spots, cuts=5):
    """Push the surface in at a few places: spots = [(centre, radius, depth, direction)]. The mesh is cut finer first."""
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    bmesh.ops.subdivide_edges(bm, edges=[e for e in bm.edges if e.calc_length() > 0.08], cuts=cuts, use_grid_fill=True)
    for c, r, d, n in spots:
        c, n = Vector(c), Vector(n).normalized()
        for v in bm.verts:
            k = (v.co - c).length / r
            if k < 1:
                v.co += n * (-d * (1 - k * k) ** 2)
    bm.to_mesh(ob.data)
    bm.free()
    return ob


def W(x, y, z):
    """SARO metres (x, y up, z) as Blender's."""
    return (x, -z, y)


def wbox(name, x0, x1, y0, y1, z0, z1, mat, part=0, bev=0.0015, seg=2):
    """A box by its SARO extents."""
    return box(name, (min(x0, x1), -max(z0, z1), min(y0, y1)), (max(x0, x1), -min(z0, z1), max(y0, y1)), mat, part, bev, seg)


# ---------------------------------------------------------------- materials (procedural over photo textures, baked to one atlas per asset)
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


def mixc(nt, fac, a, b, blend='MIX'):
    n = nt.nodes.new('ShaderNodeMix')
    n.data_type = 'RGBA'
    n.blend_type = blend
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


def image(nt, coord, path, size=1.0, color=True, turn=0.0):
    """A picture laid on by the object's own metres from three sides (box), `size` metres a repeat; turn: degrees about Z."""
    m = nt.nodes.new('ShaderNodeMapping')
    m.inputs['Scale'].default_value = (1.0 / size, 1.0 / size, 1.0 / size)
    m.inputs['Rotation'].default_value = (0, 0, math.radians(turn))
    nt.links.new(coord, m.inputs['Vector'])
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = bpy.data.images.load(str(path), check_existing=True)
    t.image.colorspace_settings.name = 'sRGB' if color else 'Non-Color'
    t.projection = 'BOX'
    t.projection_blend = 0.2
    nt.links.new(m.outputs[0], t.inputs['Vector'])
    return t.outputs['Color']


def material(name, base, metal, rough, worn, worn_metal, worn_rough, wear=1.0, scratch=0.5, dust=0.6, grain=0.35, grain_scale=1400.0,
             tex=None, tex_size=1.0, tex_turn=0.0, tint=None, tex_rough=None, rust=0.0, dust_col=(0.20, 0.17, 0.13), edge=0.0035, along=(6.0, 420.0, 420.0)):
    """base (a colour, or a picture `tex` times `tint`) under worn edges and scratches, dust and rust in the corners, a fine grain."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    co = nt.nodes.new('ShaderNodeTexCoord').outputs['Object']
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    wide = nt.nodes.new('ShaderNodeBevel')
    wide.samples = 8
    wide.inputs['Radius'].default_value = edge
    dot = nt.nodes.new('ShaderNodeVectorMath')
    dot.operation = 'DOT_PRODUCT'
    nt.links.new(wide.outputs['Normal'], dot.inputs[0])
    nt.links.new(geo.outputs['Normal'], dot.inputs[1])
    edge_m = ramp(nt, dot.outputs['Value'], 0.985, 0.86)
    patch = ramp(nt, noise(nt, co, 70.0, 6.0, 0.65), 0.40, 0.66)
    edge_wear = fmath(nt, 'MULTIPLY', edge_m, fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', patch, 0.8), 0.2), True)
    scr = ramp(nt, noise(nt, co, 1.0, 3.0, 0.5, stretch=along), 0.655, 0.70)
    rub = ramp(nt, noise(nt, co, 26.0, 4.0, 0.7), 0.63, 0.74)
    flat_wear = fmath(nt, 'MAXIMUM', fmath(nt, 'MULTIPLY', scr, scratch), fmath(nt, 'MULTIPLY', rub, 0.35 * scratch))
    w = fmath(nt, 'MULTIPLY', fmath(nt, 'MAXIMUM', edge_wear, flat_wear), wear, True)
    ao = nt.nodes.new('ShaderNodeAmbientOcclusion')
    ao.samples = 8
    ao.inputs['Distance'].default_value = 0.02
    corner = ramp(nt, ao.outputs['AO'], 0.92, 0.45)
    speck = ramp(nt, noise(nt, co, 240.0, 3.0, 0.6), 0.52, 0.72)
    d = fmath(nt, 'MULTIPLY', fmath(nt, 'ADD', corner, fmath(nt, 'MULTIPLY', speck, 0.16)), dust, True)
    tone = fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', noise(nt, co, 9.0, 3.0, 0.5), 0.3), 0.85)
    b = base
    lum = None
    if tex:
        b = image(nt, co, tex, tex_size, True, tex_turn)
        bw = nt.nodes.new('ShaderNodeRGBToBW')
        nt.links.new(b, bw.inputs[0])
        lum = bw.outputs[0]
        if tint:
            b = mixc(nt, 1.0, b, tint, 'MULTIPLY')
    col = mixc(nt, w, b, worn)
    col = mixc(nt, 1.0, col, tone, 'MULTIPLY')
    r = mixf(nt, w, rough, worn_rough)
    if tex_rough:
        tr = image(nt, co, tex_rough, tex_size, False, tex_turn)
        r = mixf(nt, w, fmath(nt, 'ADD', fmath(nt, 'MULTIPLY', tr, 0.6), fmath(nt, 'MULTIPLY', rough, 0.4)), worn_rough)
    m = mixf(nt, w, metal, worn_metal)
    if rust > 0:
        # rust creeps out of the corners and from a few sores on the flats
        rpic = image(nt, co, PH / 'rusty_metal_02_diff_2k.jpg', 0.7, True)
        rbw = nt.nodes.new('ShaderNodeRGBToBW')
        nt.links.new(rpic, rbw.inputs[0])
        sore = ramp(nt, noise(nt, co, 5.0, 4.0, 0.7), 0.66, 0.78)
        rm = fmath(nt, 'MULTIPLY', fmath(nt, 'MAXIMUM', corner, sore), ramp(nt, rbw.outputs[0], 0.75, 0.35), True)
        rm = fmath(nt, 'MULTIPLY', rm, rust, True)
        col = mixc(nt, rm, col, mixc(nt, 1.0, rpic, (0.55, 0.30, 0.16), 'MULTIPLY'))
        r = mixf(nt, rm, r, 0.9)
        m = mixf(nt, rm, m, 0.0)
    col = mixc(nt, fmath(nt, 'MULTIPLY', d, 0.62), col, dust_col)
    r = fmath(nt, 'ADD', r, fmath(nt, 'MULTIPLY', fmath(nt, 'SUBTRACT', noise(nt, co, 55.0, 5.0, 0.6), 0.5), 0.16))
    r = fmath(nt, 'ADD', r, fmath(nt, 'MULTIPLY', d, 0.30), True)
    m = fmath(nt, 'MULTIPLY', m, fmath(nt, 'SUBTRACT', 1.0, fmath(nt, 'MULTIPLY', d, 0.7)), True)
    height = fmath(nt, 'SUBTRACT', fmath(nt, 'MULTIPLY', noise(nt, co, grain_scale, 2.0, 0.5), grain), fmath(nt, 'MULTIPLY', scr, 0.6 * scratch))
    if lum is not None:
        height = fmath(nt, 'ADD', height, fmath(nt, 'MULTIPLY', lum, 1.2))
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


def plain(name, color, rough=0.5, metal=0.0, emit=0.0):
    """A material that goes to the game by its own name (a slot of its own, not baked): the import script makes it."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    b = next(n for n in mat.node_tree.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
    b.inputs['Base Color'].default_value = (color[0], color[1], color[2], 1)
    b.inputs['Roughness'].default_value = rough
    b.inputs['Metallic'].default_value = metal
    if emit > 0:
        b.inputs['Emission Color'].default_value = (color[0], color[1], color[2], 1)
        b.inputs['Emission Strength'].default_value = emit
    return mat


# ---------------------------------------------------------------- join, shade, unwrap, bake, export
def join(obs, name):
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    for o in obs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = obs[0]
    if len(obs) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = name
    ob.data.name = name
    return ob


def tag_parts():
    for ob, part in made:
        me = ob.data
        a = me.attributes.get('part') or me.attributes.new('part', 'INT', 'FACE')
        a.data.foreach_set('value', [part] * len(me.polygons))


def shade(ob, deg=40, weld=0.00002, tri=True):
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    if weld:
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=weld)
    if tri:
        bmesh.ops.triangulate(bm, faces=bm.faces)
    lim = math.radians(deg)
    for e in bm.edges:
        e.smooth = not (len(e.link_faces) == 2 and e.calc_face_angle(0.0) > lim)
    for f in bm.faces:
        f.smooth = True
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()


def unwrap(ob, margin=0.003, angle=62):
    if not ob.data.uv_layers:
        ob.data.uv_layers.new(name='UVMap')
    only(ob)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=margin, area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
    bpy.ops.object.mode_set(mode='OBJECT')


def cycles():
    scn.render.engine = 'CYCLES'
    scn.cycles.samples = SAMPLES
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
                    print('S47 REC: cycles on', kind)
                    return
            except Exception as exc:
                print('S47 REC: no', kind, exc)
    except Exception as exc:
        print('S47 REC: cycles on the CPU:', exc)


def bake(ob, key, size, final_name):
    """The procedural materials of ob into one atlas (base colour, roughness, metallic, normal); ob then wears the atlas material."""
    cycles()
    scn.render.bake.margin = max(4, size // 512)
    scn.render.bake.use_clear = True
    scn.render.bake.use_selected_to_active = False
    images = {}
    for k, space in (('basecolor', 'sRGB'), ('roughness', 'Non-Color'), ('metallic', 'Non-Color'), ('normal', 'Non-Color')):
        im = bpy.data.images.new(key + '_' + k, size, size, alpha=False)
        im.colorspace_settings.name = space
        images[k] = im
    only(ob)
    for k in ('basecolor', 'roughness', 'metallic', 'normal'):
        for mat, row in bake_sockets.items():
            nt = row['nt']
            row['img'].image = images[k]
            for n in nt.nodes:
                n.select = False
            row['img'].select = True
            nt.nodes.active = row['img']
            for link in list(row['out'].inputs['Surface'].links):
                nt.links.remove(link)
            if k == 'normal':
                nt.links.new(row['bsdf'].outputs[0], row['out'].inputs['Surface'])
            else:
                for link in list(row['emit'].inputs['Color'].links):
                    nt.links.remove(link)
                nt.links.new(row[{'basecolor': 'col', 'roughness': 'rough', 'metallic': 'metal'}[k]], row['emit'].inputs['Color'])
                nt.links.new(row['emit'].outputs[0], row['out'].inputs['Surface'])
        if k == 'normal':
            bpy.ops.object.bake(type='NORMAL', normal_space='TANGENT', normal_r='POS_X', normal_g='NEG_Y', normal_b='POS_Z')   # DirectX green
        else:
            bpy.ops.object.bake(type='EMIT')
        im = images[k]
        im.filepath_raw = str(WORK / 'textures' / (key + '_' + k + '.png'))
        im.file_format = 'PNG'
        im.save()
        print('S47 REC: baked', key, k, flush=True)
    final = bpy.data.materials.new(final_name)
    final.use_nodes = True
    nt = final.node_tree
    bsdf = next(n for n in nt.nodes if n.bl_idname == 'ShaderNodeBsdfPrincipled')
    for k, target in (('basecolor', 'Base Color'), ('roughness', 'Roughness'), ('metallic', 'Metallic')):
        t = nt.nodes.new('ShaderNodeTexImage')
        t.image = images[k]
        nt.links.new(t.outputs['Color'], bsdf.inputs[target])
    t = nt.nodes.new('ShaderNodeTexImage')
    t.image = images['normal']
    sep = nt.nodes.new('ShaderNodeSeparateColor')
    back = nt.nodes.new('ShaderNodeCombineColor')
    inv = nt.nodes.new('ShaderNodeMath')
    inv.operation = 'SUBTRACT'
    inv.inputs[0].default_value = 1.0
    nt.links.new(t.outputs['Color'], sep.inputs[0])
    nt.links.new(sep.outputs[0], back.inputs[0])
    nt.links.new(sep.outputs[1], inv.inputs[1])
    nt.links.new(inv.outputs[0], back.inputs[1])
    nt.links.new(sep.outputs[2], back.inputs[2])
    nm = nt.nodes.new('ShaderNodeNormalMap')
    nt.links.new(back.outputs[0], nm.inputs['Color'])
    nt.links.new(nm.outputs['Normal'], bsdf.inputs['Normal'])
    # the baked faces take the atlas; faces of a plain material keep their slot
    me = ob.data
    baked = set(bake_sockets.keys())
    for i, slot in enumerate(list(me.materials)):
        if slot in baked:
            me.materials[i] = final
    me.update()
    return final


def merge_slots(ob):
    """One slot per material (the joined mesh has the baked one many times over)."""
    only(ob)
    try:
        bpy.ops.object.material_slot_remove_unused()
    except Exception as exc:
        print('S47 REC: slots:', exc)
    me = ob.data
    names = []
    for m in me.materials:
        if m not in names:
            names.append(m)
    index = [names.index(m) for m in me.materials]
    idx = [0] * len(me.polygons)
    me.polygons.foreach_get('material_index', idx)
    idx = [index[i] for i in idx]
    me.materials.clear()
    for m in names:
        me.materials.append(m)
    me.polygons.foreach_set('material_index', idx)
    me.update()


CONTRACT = WORK / 'contract.json'


def export(ob, name):
    ob.name = name
    ob.data.name = name
    only(ob)
    bpy.ops.export_scene.fbx(filepath=str(WORK / 'Export' / (name + '.fbx')), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False, add_leaf_bones=False,
                             mesh_smooth_type='FACE', use_tspace=True)
    co = [ob.matrix_world @ v.co for v in ob.data.vertices]
    lo = [round(min(c[i] for c in co), 4) for i in range(3)]
    hi = [round(max(c[i] for c in co), 4) for i in range(3)]
    data = json.loads(CONTRACT.read_text()) if CONTRACT.exists() else {}
    data[name] = {'triangles': len(ob.data.polygons), 'min_m': lo, 'max_m': hi, 'slots': [m.name for m in ob.data.materials]}
    CONTRACT.write_text(json.dumps(data, indent=1, sort_keys=True))
    print('S47 REC: exported', name, len(ob.data.polygons), 'tris', lo, hi, [m.name for m in ob.data.materials], flush=True)


def split_parts(ob, names, offsets=None):
    """The joined, baked mesh as one object per part index (moved back by its offset), each exported under its name."""
    out = []
    for index, name in enumerate(names):
        o = ob.copy()
        o.data = ob.data.copy()
        scn.collection.objects.link(o)
        bm = bmesh.new()
        bm.from_mesh(o.data)
        layer = bm.faces.layers.int.get('part')
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if f[layer] != index], context='FACES')
        if offsets:
            bmesh.ops.translate(bm, vec=[-c for c in offsets[index]], verts=bm.verts)
        bm.to_mesh(o.data)
        bm.free()
        merge_slots(o)
        export(o, name)
        out.append(o)
    return out


def preview(obs, key, views, lights=None):
    """The finished pieces under three lamps, as the game might light them (a look only)."""
    if not PREVIEW:
        return
    cycles()
    for o in scn.objects:
        if o.type == 'MESH':
            o.hide_render = o not in obs
    world = bpy.data.worlds.new('w')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs[0].default_value = (0.03, 0.032, 0.036, 1)
    scn.world = world
    aim = bpy.data.objects.new('aim', None)
    scn.collection.objects.link(aim)
    for loc, energy, col, size in (lights or (((0.6, -1.4, 1.6), 60, (1.0, 0.95, 0.86), 1.0), ((-1.6, -0.6, 0.9), 22, (0.8, 0.88, 1.0), 1.2), ((0.9, 1.3, 1.2), 18, (1.0, 0.85, 0.7), 1.0))):
        ld = bpy.data.lights.new('l', 'AREA')
        ld.energy = energy
        ld.color = col
        ld.size = size
        lo = bpy.data.objects.new('l', ld)
        scn.collection.objects.link(lo)
        lo.location = loc
        con = lo.constraints.new('TRACK_TO')
        con.target = aim
        con.track_axis = 'TRACK_NEGATIVE_Z'
        con.up_axis = 'UP_Y'
    cam_d = bpy.data.cameras.new('c')
    cam = bpy.data.objects.new('c', cam_d)
    scn.collection.objects.link(cam)
    scn.camera = cam
    con = cam.constraints.new('TRACK_TO')
    con.target = aim
    con.track_axis = 'TRACK_NEGATIVE_Z'
    con.up_axis = 'UP_Y'
    scn.render.resolution_x, scn.render.resolution_y = 1280, 720
    scn.cycles.samples = 40
    scn.render.image_settings.file_format = 'PNG'
    try:
        scn.view_settings.view_transform = 'AgX'
    except Exception:
        pass
    for name, loc, at, lens in views:
        cam.location = loc
        aim.location = at
        cam_d.lens = lens
        cam_d.clip_start = 0.01
        scn.render.filepath = str(WORK / 'Preview' / (key + '_' + name + '.png'))
        bpy.ops.render.render(write_still=True)
    print('S47 REC: previews', key, flush=True)


def baked_asset(key, name, size, views, deg=40, keep=()):
    """made -> one mesh, shaded, unwrapped, baked to its atlas, exported. keep: objects of plain materials, joined after the bake."""
    tag_parts()
    body = join([o for o, _ in made if o not in keep], name)
    shade(body, deg)
    unwrap(body)
    bake(body, key, size, 'M_Rec_' + key)
    for o in keep:
        shade(o, deg)
        unwrap(o)
    ob = join([body] + list(keep), name) if keep else body
    merge_slots(ob)
    return ob


# ================================================================ the room's pieces
X0, X1, Z0, Z1 = -6.0, -1.0, 6.8, 10.5
CEIL = 2.70
GX = [round(-5.8 + 0.6 * k, 3) for k in range(8)]           # -5.8 .. -1.6 (the last cell ends at the east wall)
GZ = [7.1, 7.7, 8.3, 8.9, 9.5, 10.1]
TROFFERS = (('A', -5.2, -4.0, 7.7, 8.3), ('B', -2.8, -1.6, 7.7, 8.3))


def build_ceiling():
    """A hung grid at 2.70 m: T-bars, mineral-fibre tiles (each its own quad, UVs to one of the texture's four faces), two 0.6 x 1.2
    troffers with a prismatic lens, and a closed slab over it all. Tiling materials, no bake."""
    reset()
    TILE, BAR, VOID, BODY = plain('M_Rec_CeilTile', (0.2, 0.18, 0.14), 0.95), plain('M_Rec_TBar', (0.45, 0.43, 0.37), 0.6), plain('M_Rec_CeilVoid', (0.01, 0.01, 0.01), 1.0), plain('M_Rec_TrofferBody', (0.6, 0.59, 0.53), 0.5)
    LENS = {'A': plain('M_Rec_TrofferLensA', (0.9, 0.9, 0.8), 0.3, 0, 2.0), 'B': plain('M_Rec_TrofferLensB', (0.9, 0.88, 0.75), 0.3, 0, 2.0)}
    rng = random.Random(47)
    wbox('void', X0, X1, CEIL + 0.02, CEIL + 0.04, Z0, Z1, VOID, bev=0)
    xs = [X0] + GX + [X1]
    zs = [Z0] + GZ + [Z1]

    def in_troffer(xa, xb, za, zb):
        for _, tx0, tx1, tz0, tz1 in TROFFERS:
            if xa >= tx0 - 1e-3 and xb <= tx1 + 1e-3 and za >= tz0 - 1e-3 and zb <= tz1 + 1e-3:
                return True
        return False
    # bars: along z at every x line, along x at every z line; a wall angle all round
    for x in GX:
        cuts = sorted((tz0, tz1) for _, tx0, tx1, tz0, tz1 in TROFFERS if tx0 + 1e-3 < x < tx1 - 1e-3)
        za = Z0
        for c0, c1 in cuts + [(Z1, Z1)]:
            if c0 - za > 0.01:
                wbox('bar_x%.1f_%.1f' % (x, za), x - 0.012, x + 0.012, CEIL, CEIL + 0.03, za, c0, BAR, bev=0)
            za = c1
    for z in GZ:
        for xa, xb in zip(xs[:-1], xs[1:]):
            if any(tx0 - 1e-3 <= xa and xb <= tx1 + 1e-3 and abs(z - (tz0 + tz1) / 2) < 0.29 for _, tx0, tx1, tz0, tz1 in TROFFERS):
                continue
            wbox('bar_z%.1f_%.1f' % (z, xa), xa + 0.012, xb - 0.012, CEIL, CEIL + 0.028, z - 0.012, z + 0.012, BAR, bev=0)
    for (xa, xb, za, zb) in ((X0, X1, Z0, Z0 + 0.022), (X0, X1, Z1 - 0.022, Z1), (X0, X0 + 0.022, Z0, Z1), (X1 - 0.022, X1, Z0, Z1)):
        wbox('angle', xa, xb, CEIL, CEIL + 0.022, za, zb, BAR, bev=0)
    # tiles
    for i, (xa, xb) in enumerate(zip(xs[:-1], xs[1:])):
        for j, (za, zb) in enumerate(zip(zs[:-1], zs[1:])):
            if in_troffer(xa, xb, za, zb):
                continue
            q = rng.randrange(4)
            if (i, j) == (1, 5):
                q = 2                                        # the water-stained face, under the old leak in the south-west
            elif q == 2:
                q = rng.choice((0, 1, 3))
            u0, v0 = (q % 2) * 0.5, (q // 2) * 0.5
            fu, fv = (xb - xa) / 0.6 * 0.5, (zb - za) / 0.6 * 0.5
            lift = [rng.uniform(-0.002, 0.004) for _ in range(4)]
            if (i, j) == (1, 5):
                lift = [0.0, -0.012, -0.004, 0.0]            # it has sagged
            if (i, j) == (6, 2):
                lift = [0.018, 0.018, 0.0, 0.0]              # one pushed up and left askew
            y = CEIL + 0.006
            pts = [W(xa, y + lift[0], za), W(xb, y + lift[1], za), W(xb, y + lift[2], zb), W(xa, y + lift[3], zb)]
            uv = [(u0, v0), (u0 + fu, v0), (u0 + fu, v0 + fv), (u0, v0 + fv)]
            k = rng.randrange(4) if abs(fu - 0.5) < 1e-3 and abs(fv - 0.5) < 1e-3 and q != 2 else 0
            uv = uv[k:] + uv[:k]
            quad('tile_%d_%d' % (i, j), pts, TILE, uvs=uv)   # seen from below: counter-clockwise from under
    # troffers: a white door frame a little proud of the grid, the lens set back in it
    for key, tx0, tx1, tz0, tz1 in TROFFERS:
        f = 0.035
        for (xa, xb, za, zb) in ((tx0, tx1, tz0, tz0 + f), (tx0, tx1, tz1 - f, tz1), (tx0, tx0 + f, tz0 + f, tz1 - f), (tx1 - f, tx1, tz0 + f, tz1 - f)):
            wbox('troffer%s_frame' % key, xa, xb, CEIL - 0.004, CEIL + 0.02, za, zb, BODY, bev=0.002)
        y = CEIL + 0.005
        quad('troffer%s_lens' % key, [W(tx0 + f, y, tz0 + f), W(tx1 - f, y, tz0 + f), W(tx1 - f, y, tz1 - f), W(tx0 + f, y, tz1 - f)], LENS[key],
             uvs=[(0, 0), (1, 0), (1, 1), (0, 1)])
        for hx in (tx0 + 0.25, tx1 - 0.25):                  # the two latches
            wbox('troffer%s_latch' % key, hx - 0.012, hx + 0.012, CEIL - 0.007, CEIL - 0.003, tz1 - 0.03, tz1 - 0.012, BODY, bev=0.001)
    ob = join([o for o, _ in made], 'SM_Rec_Ceiling')
    # the tiles are seen from below: make every face of the tiles and lenses look down, the rest out
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    uvl = bm.loops.layers.uv.verify()
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        name = ob.data.materials[f.material_index].name
        if (name.startswith('M_Rec_CeilTile') or name.startswith('M_Rec_TrofferLens')) and f.normal.z > 0:
            f.normal_flip()
        if not (name.startswith('M_Rec_CeilTile') or name.startswith('M_Rec_TrofferLens')):
            for loop in f.loops:                             # a metre a repeat from the two axes the face lies in
                c = loop.vert.co
                n = f.normal
                loop[uvl].uv = (c.x, c.y) if abs(n.z) > 0.7 else ((c.x, c.z) if abs(n.y) > 0.7 else (c.y, c.z))
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(ob.data)
    bm.free()
    merge_slots(ob)
    export(ob, 'SM_Rec_Ceiling')
    preview([ob], 'ceiling', (('below', (-3.5, -7.6, 0.3), (-3.5, -8.6, 2.7), 18),),
            lights=(((-3.5, -8.6, 0.2), 300, (1, 1, 1), 3.0),))


# ================================================================ shared looks
def m_wood():
    return material('wood', (0.2, 0.08, 0.04), 0.0, 0.30, (0.46, 0.31, 0.18), 0.0, 0.62, wear=0.9, scratch=0.22, dust=0.35, grain=0.12, grain_scale=600.0,
                    tex=PH / 'wood_table_001_diff_2k.jpg', tex_size=1.5, tex_turn=0.0, tex_rough=PH / 'wood_table_001_rough_2k.jpg', dust_col=(0.16, 0.12, 0.09), edge=0.006)


def m_steel(name='steel', tint=(1.25, 1.22, 1.2), rust=0.6, wear=0.9, dust=0.7):
    """Enamelled office steel: the green paint picture, lighter or greyer by `tint`, bare steel on worn edges, rust from the corners."""
    return material(name, (0.1, 0.12, 0.1), 0.0, 0.48, (0.30, 0.29, 0.27), 0.85, 0.36, wear=wear, scratch=0.6, dust=dust, grain=0.2,
                    tex=PH / 'green_metal_rust_diff_2k.jpg', tex_size=1.0, tint=tint, tex_rough=PH / 'green_metal_rust_rough_2k.jpg', rust=rust)


def m_chrome():
    return material('chrome', (0.60, 0.60, 0.61), 1.0, 0.20, (0.72, 0.72, 0.72), 1.0, 0.32, wear=0.5, scratch=0.9, dust=0.55, grain=0.1)


def m_brass():
    return material('brass', (0.52, 0.34, 0.11), 1.0, 0.30, (0.80, 0.64, 0.32), 1.0, 0.20, wear=0.9, scratch=0.7, dust=0.9, grain=0.15, dust_col=(0.09, 0.07, 0.035))


def m_black(name='black', rough=0.6):
    return material(name, (0.018, 0.018, 0.02), 0.0, rough, (0.07, 0.07, 0.07), 0.0, 0.42, wear=0.7, scratch=0.4, dust=0.8, grain=0.8, grain_scale=700.0)


def m_paper(name='paper', col=(0.72, 0.69, 0.58)):
    return material(name, col, 0.0, 0.85, (0.80, 0.77, 0.68), 0.0, 0.9, wear=0.4, scratch=0.05, dust=0.5, grain=0.5, grain_scale=500.0, dust_col=(0.35, 0.30, 0.2), edge=0.002)


def annulus(name, centre, r, w, mat, part=0, n=28, squash=1.0):
    bm = bmesh.new()
    ring = []
    for i in range(n):
        a = 2 * math.pi * i / n
        ring.append((bm.verts.new((centre[0] + (r - w / 2) * math.cos(a), centre[1] + (r - w / 2) * math.sin(a) * squash, centre[2])),
                     bm.verts.new((centre[0] + (r + w / 2) * math.cos(a), centre[1] + (r + w / 2) * math.sin(a) * squash, centre[2]))))
    for (a, b), (c, d) in zip(ring, ring[1:] + ring[:1]):
        bm.faces.new((a, b, d, c))
    ob = finish(name, bm, mat, part)
    for f in ob.data.polygons:
        if f.normal.z < 0:
            f.flip()
    return ob


# ================================================================ the work table
def build_table():
    """1.70 x 0.85 x 0.77 m: a worn varnished top on a square-tube steel frame; nothing lower than 0.69 m between the legs (the game
    hides the player under it). Origin: the middle of the floor under it; its long side along X."""
    reset()
    WOOD, FRAME, FOOT = m_wood(), m_steel('frame', tint=(0.95, 0.98, 1.0), rust=0.5, wear=1.1), m_black('foot')
    RING = material('ring', (0.07, 0.03, 0.015), 0.0, 0.62, (0.07, 0.03, 0.015), 0.0, 0.62, wear=0.0, scratch=0.0, dust=0.0, grain=0.0)
    box('top', (-0.85, -0.425, 0.738), (0.85, 0.425, 0.770), WOOD, bev=0.004, seg=3)
    for sx in (-1, 1):
        for sy in (-1, 1):
            box('leg', (sx * 0.79 - 0.02, sy * 0.365 - 0.02, 0.012), (sx * 0.79 + 0.02, sy * 0.365 + 0.02, 0.738), FRAME, bev=0.003)
            cyl('glide', 0.0, 0.013, 0.017, FOOT, at=(sx * 0.79, sy * 0.365), n=12, bev=0.002)
            box('gusset', (sx * 0.79 - 0.06 if sx > 0 else sx * 0.79, sy * 0.365 - 0.004, 0.690), (sx * 0.79 if sx > 0 else sx * 0.79 + 0.06, sy * 0.365 + 0.004, 0.738), FRAME, bev=0.001)
    for sy in (-1, 1):
        box('rail_long', (-0.77, sy * 0.365 - 0.0125, 0.690), (0.77, sy * 0.365 + 0.0125, 0.738), FRAME, bev=0.002)
    for sx in (-1, 1):
        box('rail_short', (sx * 0.79 - 0.0125, -0.345, 0.690), (sx * 0.79 + 0.0125, 0.345, 0.738), FRAME, bev=0.002)
    box('rail_mid', (-0.0125, -0.353, 0.702), (0.0125, 0.353, 0.738), FRAME, bev=0.002)
    for c, r in (((0.62, 0.16, 0.7702), 0.042), ((0.655, 0.20, 0.7703), 0.040), ((-0.30, -0.27, 0.7702), 0.038)):
        annulus('ring', c, r, 0.0035, RING)
    ob = baked_asset('WorkTable', 'SM_Rec_WorkTable', 2048, None)
    export(ob, 'SM_Rec_WorkTable')
    preview([ob], 'table', (('front', (1.6, -1.9, 1.5), (0, 0, 0.6), 45), ('top', (0.3, -0.6, 1.5), (0.2, 0, 0.77), 40), ('under', (1.4, -1.6, 0.35), (0, 0, 0.5), 40)))


# ================================================================ the banker's lamp
def trough(name, length, radius, a0, a1, zc, mat, inward=False, n=14):
    """A half-pipe along X: the arc a0..a1 degrees in the y-z plane (0 = back, 90 = top, 180 = front), closed at both ends."""
    bm = bmesh.new()
    rows = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        y, z = radius * math.cos(a), zc + radius * math.sin(a)
        rows.append((bm.verts.new((-length / 2, y, z)), bm.verts.new((length / 2, y, z))))
    for (a, b), (c, d) in zip(rows[:-1], rows[1:]):
        bm.faces.new((a, b, d, c))
    for side, x in ((0, -length / 2), (1, length / 2)):
        mid = bm.verts.new((x, 0, zc))
        for r0, r1 in zip(rows[:-1], rows[1:]):
            bm.faces.new((mid, r0[side], r1[side]))
    me = bpy.data.meshes.new(name)
    bm.normal_update()
    axis = Vector((0, 0, zc))
    for f in bm.faces:
        c = f.calc_center_median()
        out = Vector((c.x * 0.2 if abs(abs(c.x) - length / 2) < 1e-6 else 0, c.y, c.z - zc)) if abs(abs(c.x) - length / 2) > 1e-6 else Vector((c.x, 0, 0))
        if (f.normal.dot(out) < 0) != inward:
            f.normal_flip()
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    scn.collection.objects.link(ob)
    me.materials.append(mat)
    made.append((ob, 0))
    return ob


def build_lamp():
    """A brass banker's lamp, 0.40 m: a weighted round foot, a straight stem (it stands where the export's rod stood), a yoke and a
    green glass trough shade, white inside, tipped a little to the front (-Y). The glass and the inside are slots of their own:
    the game lights them with the lamp (S47RecordsRoom.cpp)."""
    reset()
    BRASS, BLACK = m_brass(), m_black('cord', 0.5)
    GLASS = plain('M_Rec_LampGlass', (0.02, 0.20, 0.08), 0.12)
    INNER = plain('M_Rec_LampInner', (0.85, 0.85, 0.78), 0.4, 0.0, 2.0)
    cyl('foot', 0.0, 0.016, 0.082, BRASS, n=40, bev=0.004, seg=3)
    cyl('foot2', 0.016, 0.032, 0.058, BRASS, n=40, bev=0.002, r2=0.030)
    cyl('collar', 0.032, 0.046, 0.017, BRASS, n=20, bev=0.0015)
    cyl('stem', 0.046, 0.300, 0.0115, BRASS, n=16, bev=0.0006)
    cyl('knuckle', 0.296, 0.312, 0.016, BRASS, n=20, bev=0.003)
    cyl('yoke', -0.136, 0.136, 0.0055, BRASS, at=(0.0, 0.305), axis='X', n=12, bev=0.0006)
    zc, tip = 0.345, -11.0
    keep = []
    for sx in (-1, 1):
        box('yoke_arm', (sx * 0.136 - 0.004, -0.007, 0.300), (sx * 0.136 + 0.004, 0.007, zc + 0.008), BRASS, bev=0.0015)
        cyl('pivot', sx * 0.126 if sx > 0 else sx * 0.143, sx * 0.143 if sx > 0 else sx * 0.126, 0.011, BRASS, at=(0.0, zc), axis='X', n=18, bev=0.002)
    shade = trough('shade', 0.25, 0.060, -38, 218, zc, GLASS, n=18)
    inner = trough('shade_inner', 0.246, 0.0585, -38, 218, zc, INNER, inward=True, n=18)
    bulb = cyl('bulb', -0.055, 0.055, 0.013, INNER, at=(0.0, zc + 0.012), axis='X', n=12, bev=0.004, seg=2)
    made[:] = [row for row in made if row[0] is not bulb]
    made.append((bulb, 0))
    for o in (shade, inner, bulb):
        rot_about(o, 'X', tip, (0, 0, zc))
        keep.append(o)
    cyl('finial', zc + 0.058, zc + 0.070, 0.008, BRASS, n=14, bev=0.002)
    cyl('chain', 0.235, 0.300, 0.0011, BRASS, at=(0.045, -0.012), n=6, bev=0)
    cyl('chain_ball', 0.226, 0.236, 0.0045, BRASS, at=(0.045, -0.012), n=10, bev=0.002)
    pipe('cord', [(0.0, 0.07, 0.006), (0.01, 0.14, 0.004), (0.05, 0.22, 0.004)], 0.003, BLACK)
    ob = baked_asset('BankerLamp', 'SM_Rec_BankerLamp', 1024, None, keep=keep)
    export(ob, 'SM_Rec_BankerLamp')
    preview([ob], 'lamp', (('front', (0.35, -0.75, 0.42), (0, 0, 0.2), 60), ('back', (-0.4, 0.7, 0.5), (0, 0, 0.2), 60), ('under', (0.0, -0.6, 0.12), (0, 0, 0.3), 60)))


# ================================================================ the typewriter
def build_typewriter():
    """An invented red office electric of the 1970s ("CALDERA 210"): a wedge body between two cheeks, a keyboard of separate caps on
    a 17 degree slope, a platen with its knobs, a paper bail, a ribbon carrier in the well and a sheet in the machine.
    0.47 x 0.40 x 0.30 m with the paper; front (the keyboard) to -Y."""
    reset()
    RED = material('red', (0.50, 0.035, 0.03), 0.0, 0.34, (0.62, 0.10, 0.07), 0.0, 0.5, wear=0.7, scratch=0.5, dust=0.75, grain=0.5, grain_scale=900.0, dust_col=(0.16, 0.10, 0.08), edge=0.004)
    DARK, KEY = m_black('dark', 0.55), material('key', (0.03, 0.03, 0.032), 0.0, 0.5, (0.09, 0.09, 0.09), 0.0, 0.28, wear=1.2, scratch=0.3, dust=0.9, grain=0.6, grain_scale=800.0, edge=0.002)
    RUB, CHROME, PAPER = m_black('rubber', 0.85), m_chrome(), m_paper()
    WHITE = material('legend', (0.75, 0.74, 0.68), 0.0, 0.6, (0.75, 0.74, 0.68), 0.0, 0.6, wear=0.0, scratch=0.0, dust=0.3, grain=0.0)
    W2 = 0.235
    cheek = [(-0.200, 0.0), (-0.200, 0.044), (-0.190, 0.054), (-0.048, 0.098), (-0.040, 0.128), (0.150, 0.152), (0.200, 0.138), (0.200, 0.0)]
    mid = [(-0.198, 0.0), (-0.198, 0.030), (-0.188, 0.040), (-0.050, 0.083), (-0.046, 0.118), (-0.020, 0.122), (-0.020, 0.0)]
    back = [(0.112, 0.0), (0.112, 0.140), (0.150, 0.146), (0.198, 0.132), (0.198, 0.0)]
    for sx in (-1, 1):
        prism_x('cheek', cheek, sx * W2 - (0.032 if sx > 0 else 0), sx * W2 + (0 if sx > 0 else 0.032), RED, bev=0.005, seg=3)
    prism_x('front', mid, -W2 + 0.03, W2 - 0.03, RED, bev=0.003)
    prism_x('back', back, -W2 + 0.03, W2 - 0.03, RED, bev=0.003)
    box('well', (-W2 + 0.03, -0.022, 0.0), (W2 - 0.03, 0.114, 0.086), DARK, bev=0)
    box('bottom', (-W2 + 0.01, -0.19, 0.004), (W2 - 0.01, 0.19, 0.012), DARK, bev=0.002)
    for sx in (-1, 1):
        for sy in (-1, 1):
            cyl('foot', 0.0, 0.006, 0.014, RUB, at=(sx * 0.19, sy * 0.16), n=10, bev=0.001)
    # the keyboard: four rows on the slope from (-0.188, 0.040) to (-0.050, 0.083)
    tilt = math.degrees(math.atan2(0.083 - 0.040, 0.188 - 0.050))
    rows = ['1234567890-=', 'QWERTYUIOP[]', "ASDFGHJKL;'", 'ZXCVBNM,./']
    pitch = 0.0192
    for r, chars in enumerate(rows):
        yk = -0.158 + r * 0.0005 + (3 - r) * 0.0 + r * 0.0
        yk = -0.106 - r * 0.0193 * 1.0 if False else -0.1005 - (r) * 0.0195
        zk = 0.040 + (yk + 0.188) * math.tan(math.radians(tilt)) + 0.004
        x0 = -(len(chars) - 1) * pitch / 2 + (0.0, 0.004, 0.009, 0.018)[r] - 0.008
        for i, ch in enumerate(chars):
            x = x0 + i * pitch
            k = box('cap', (x - 0.0074, yk - 0.0074, zk), (x + 0.0074, yk + 0.0074, zk + 0.0085), KEY, bev=0.0022, seg=2)
            rot_about(k, 'X', tilt, (x, yk, zk))
            t = text('leg', ch, 0.0062, WHITE, (x, yk - 0.0022, zk + 0.0089), (0, 0, 0), extrude=0.0)
            if t:
                rot_about(t, 'X', tilt, (x, yk, zk))
        for sx, wd in ((-1, 0.026), (1, 0.030)):                       # the wide keys at both ends of each row
            x = x0 - pitch * 0.5 - wd / 2 - 0.004 if sx < 0 else x0 + (len(chars) - 1) * pitch + pitch * 0.5 + wd / 2 + 0.004
            if abs(x) + wd / 2 < W2 - 0.036:
                k = box('cap_w', (x - wd / 2, yk - 0.0074, zk), (x + wd / 2, yk + 0.0074, zk + 0.0085), KEY, bev=0.0022, seg=2)
                rot_about(k, 'X', tilt, (x, yk, zk))
    ys = -0.1005 - 4 * 0.0195
    zs = 0.040 + (ys + 0.188) * math.tan(math.radians(tilt)) + 0.004
    sp = box('space', (-0.062, ys - 0.007, zs), (0.062, ys + 0.007, zs + 0.008), KEY, bev=0.0025, seg=2)
    rot_about(sp, 'X', tilt, (0, ys, zs))
    # the top: the platen and its knobs, the bail with three rollers, the ribbon carrier on its rail, a paper rest
    cyl('platen', -0.172, 0.172, 0.021, RUB, at=(0.085, 0.128), axis='X', n=28, bev=0.002)
    for sx in (-1, 1):
        cyl('axle', sx * 0.172 if sx > 0 else sx * 0.250, sx * 0.250 if sx > 0 else sx * 0.172, 0.006, CHROME, at=(0.085, 0.128), axis='X', n=10, bev=0.0005)
        cyl('knob', sx * 0.238 if sx > 0 else sx * 0.254, sx * 0.254 if sx > 0 else sx * 0.238, 0.024, DARK, at=(0.085, 0.128), axis='X', n=24, bev=0.003)
    cyl('bail', -0.165, 0.165, 0.0028, CHROME, at=(0.066, 0.153), axis='X', n=8, bev=0)
    for x in (-0.09, 0.0, 0.09):
        cyl('bail_roll', x - 0.008, x + 0.008, 0.007, RUB, at=(0.066, 0.153), axis='X', n=12, bev=0.001)
    for sx in (-1, 1):
        box('bail_arm', (sx * 0.165 - 0.002, 0.064, 0.128), (sx * 0.165 + 0.002, 0.090, 0.156), CHROME, bev=0.0008)
    cyl('rail', -0.190, 0.190, 0.004, CHROME, at=(0.030, 0.100), axis='X', n=8, bev=0)
    box('carrier', (-0.075, 0.000, 0.094), (0.045, 0.062, 0.112), DARK, bev=0.003)
    box('ribbon', (-0.060, 0.008, 0.112), (0.030, 0.052, 0.121), DARK, bev=0.004, seg=2)
    box('element', (-0.022, 0.050, 0.100), (0.002, 0.066, 0.128), CHROME, bev=0.002)
    rest = box('rest', (-0.150, 0.118, 0.140), (0.150, 0.122, 0.215), RED, bev=0.0015)
    rot_about(rest, 'X', -14, (0, 0.120, 0.140))
    # a sheet in the machine: up behind the platen and leaning back on the rest
    bm = bmesh.new()
    cols = []
    for i in range(9):
        t = i / 8
        y = 0.104 + 0.062 * t + 0.012 * math.sin(t * math.pi)
        z = 0.136 + 0.236 * t - 0.03 * t * t
        cols.append((bm.verts.new((-0.108, y, z)), bm.verts.new((0.108, y, z))))
    for (a, b), (c, d) in zip(cols[:-1], cols[1:]):
        bm.faces.new((a, b, d, c))
    sheet = finish('sheet', bm, PAPER)
    m = sheet.modifiers.new('s', 'SOLIDIFY')
    m.thickness = 0.0004
    only(sheet)
    bpy.ops.object.modifier_apply(modifier=m.name)
    # the maker's plate and the switch
    box('plate', (-0.062, -0.0475, 0.094), (0.062, -0.0440, 0.112), DARK, bev=0.001)
    t = text('name', 'CALDERA 210', 0.0105, CHROME, (0.0, -0.0478, 0.0985), (math.radians(90), 0, 0), extrude=0.0003)
    box('switch', (W2 - 0.012, -0.150, 0.046), (W2 + 0.004, -0.120, 0.058), DARK, bev=0.002)
    ob = baked_asset('Typewriter', 'SM_Rec_Typewriter', 2048, None, deg=38)
    export(ob, 'SM_Rec_Typewriter')
    preview([ob], 'typewriter', (('front', (0.35, -0.75, 0.50), (0, 0, 0.09), 55), ('keys', (0.05, -0.42, 0.36), (0, -0.09, 0.07), 55), ('back', (-0.5, 0.7, 0.45), (0, 0, 0.1), 55), ('side', (0.8, -0.1, 0.25), (0, 0, 0.1), 55)))


# ================================================================ steel shelving
SHELF_TOPS = (0.132, 0.632, 1.132, 1.632, 2.132)        # the heights the room's cartons already stand at (the STATION 01 box at 1.135)


def bar(name, a, b, width, thick, mat, part=0, normal=(0, 1, 0)):
    """A flat strap from a to b, its face to `normal`."""
    a, b, n = Vector(a), Vector(b), Vector(normal).normalized()
    d = b - a
    x = d.normalized()
    y = n.cross(x).normalized()
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(d.length, width, thick), verts=bm.verts)
    m = Matrix((x, y, n)).transposed().to_4x4()
    m.translation = (a + b) / 2
    bmesh.ops.transform(bm, matrix=m, verts=bm.verts)
    return finish(name, bm, mat, part)


def frame(x, depth, height, STEEL, HOLE, part=0, holes=True, sign=1):
    """Two angle posts (front and back) at run position x, their flanges turned to +X when sign is 1."""
    h, t, f = depth / 2, 0.0025, 0.038
    for sy in (-1, 1):
        y0 = sy * h
        box('post_face', (min(x, x + sign * f), min(y0, y0 - sy * t), 0.004), (max(x, x + sign * f), max(y0, y0 - sy * t), height), STEEL, part, bev=0.0006, seg=1)
        box('post_side', (min(x, x + sign * t), min(y0, y0 - sy * f), 0.004), (max(x, x + sign * t), max(y0, y0 - sy * f), height), STEEL, part, bev=0.0006, seg=1)
        box('post_foot', (min(x, x + sign * 0.05), min(y0, y0 - sy * 0.05), 0.0), (max(x, x + sign * 0.05), max(y0, y0 - sy * 0.05), 0.004), STEEL, part, bev=0.0005, seg=1)
    if holes:                                             # the punched slots of the front flange
        xc = x + sign * f / 2
        z = 0.07
        while z < height - 0.03:
            y = -h - 0.0003
            quad('slot', [(xc - 0.0045, y, z - 0.007), (xc + 0.0045, y, z - 0.007), (xc + 0.0045, y, z + 0.007), (xc - 0.0045, y, z + 0.007)], HOLE, part)
            z += 0.0381
    for z in (0.35, height - 0.25):                       # side straps between the two posts
        box('strap', (min(x, x + sign * 0.0015), -h + f, z), (max(x, x + sign * 0.0015), h - f, z + 0.03), STEEL, part, bev=0.0004, seg=1)


def shelf(x0, x1, depth, top, sag, STEEL, part=0):
    """A folded steel pan, its top at `top` at the posts and `sag` lower in the middle of its span."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(x1 - x0, depth, 0.028), verts=bm.verts)
    bmesh.ops.translate(bm, vec=((x0 + x1) / 2, 0, top - 0.014), verts=bm.verts)
    for k in range(1, 6):
        xc = x0 + (x1 - x0) * k / 6
        bmesh.ops.bisect_plane(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:], plane_co=(xc, 0, 0), plane_no=(1, 0, 0))
    for v in bm.verts:
        u = (v.co.x - x0) / (x1 - x0) * 2 - 1
        v.co.z -= sag * (1 - u * u)
    bevel(bm, 0.0012, 1, deg=60)
    return finish('shelf', bm, STEEL, part)


def build_shelving():
    """The west wall's run as two pieces that share one baked skin: a 1.0 m bay (its left frame, five pans, a back brace) and the
    frame that ends a run. Three bays and an end make the 3.0 m run. Depth 0.50, posts 2.30. Front to -Y; the bay's origin is the
    middle of its floor, the end frame's its own foot line."""
    reset()
    rng = random.Random(47)
    STEEL = m_steel('shelf', tint=(1.12, 1.2, 1.14), rust=1.0, wear=1.0, dust=0.8)
    HOLE = material('hole', (0.004, 0.004, 0.004), 0.0, 0.95, (0.004, 0.004, 0.004), 0.0, 0.95, wear=0.0, scratch=0.0, dust=0.0, grain=0.0)
    depth, height = 0.50, 2.30
    frame(-0.5, depth, height, STEEL, HOLE, 0, sign=1)
    for top in SHELF_TOPS:
        shelf(-0.497, 0.497, depth - 0.008, top, rng.uniform(0.002, 0.009), STEEL, 0)
        for sx in (-1, 1):                                # the bolt at each front corner
            cyl('bolt', -depth / 2 - 0.0035, -depth / 2, 0.0055, STEEL, 0, at=(sx * 0.481, top - 0.014), axis='Y', n=6, bev=0)
    bar('brace_a', (-0.49, depth / 2 + 0.001, 0.25), (0.49, depth / 2 + 0.001, 1.95), 0.022, 0.002, STEEL, 0)
    bar('brace_b', (-0.49, depth / 2 + 0.003, 1.95), (0.49, depth / 2 + 0.003, 0.25), 0.022, 0.002, STEEL, 0)
    frame(0.5, depth, height, STEEL, HOLE, 1, sign=-1)
    tag_parts()
    body = join([o for o, _ in made], 'shelving')
    shade(body, 40)
    unwrap(body)
    bake(body, 'Shelving', 2048, 'M_Rec_Shelving')
    obs = split_parts(body, ['SM_Rec_ShelfBay', 'SM_Rec_ShelfEnd'], [(0, 0, 0), (0.5, 0, 0)])
    body.hide_render = True
    preview(obs[:1], 'shelving', (('front', (1.2, -2.6, 1.5), (0, 0, 1.15), 40), ('close', (0.1, -0.9, 1.4), (-0.45, -0.25, 1.15), 50)))


def build_bay2():
    """The short bay that stands south of the card index: 0.80 x 0.45 x 2.00 m, sheet steel on its back and both sides (a place to
    stand out of sight of the door). Front (open) to -Y."""
    reset()
    rng = random.Random(53)
    STEEL = m_steel('shelf', tint=(1.12, 1.2, 1.14), rust=0.9, wear=1.0, dust=0.8)
    HOLE = material('hole', (0.004, 0.004, 0.004), 0.0, 0.95, (0.004, 0.004, 0.004), 0.0, 0.95, wear=0.0, scratch=0.0, dust=0.0, grain=0.0)
    depth, height, half = 0.45, 2.00, 0.40
    frame(-half, depth, height, STEEL, HOLE, 0, sign=1)
    frame(half, depth, height, STEEL, HOLE, 0, sign=-1)
    for top in (0.132, 0.582, 1.032, 1.482, 1.932):
        shelf(-half + 0.003, half - 0.003, depth - 0.008, top, rng.uniform(0.002, 0.007), STEEL)
    box('back', (-half + 0.002, depth / 2 - 0.004, 0.06), (half - 0.002, depth / 2 - 0.002, height - 0.02), STEEL, bev=0.0005, seg=1)
    for sx in (-1, 1):
        box('side', (sx * half - (0.004 if sx > 0 else -0.002), -depth / 2 + 0.04, 0.06), (sx * half - (0.002 if sx > 0 else -0.004), depth / 2 - 0.04, height - 0.02), STEEL, bev=0.0005, seg=1)
    ob = baked_asset('ShelfBay2', 'SM_Rec_ShelfBay2', 2048, None)
    export(ob, 'SM_Rec_ShelfBay2')
    preview([ob], 'bay2', (('front', (0.9, -2.2, 1.4), (0, 0, 1.0), 40),))


# ================================================================ filing cabinets
def build_cabinet(variant):
    """A four-drawer letter cabinet, 0.50 x 0.62 x 1.32 m: label holders, bar pulls and thumb latches, a lock, dents of its own.
    B has its second drawer from the top 7 cm out, folders showing. Front to -Y."""
    reset()
    rng = random.Random({'A': 11, 'B': 23, 'C': 37}[variant])
    STEEL = m_steel('cab', tint=(1.5, 1.52, 1.42), rust=0.4, wear=0.85, dust=0.6)
    BASE = m_steel('plinth', tint=(0.6, 0.6, 0.6), rust=0.8, wear=1.0)
    CHROME, PAPER, DARK = m_chrome(), m_paper('card', (0.74, 0.70, 0.58)), m_black('void', 0.9)
    MANILA = material('manila', (0.55, 0.40, 0.18), 0.0, 0.8, (0.66, 0.54, 0.32), 0.0, 0.85, wear=0.6, scratch=0.1, dust=0.4, grain=0.4, grain_scale=500.0, edge=0.002)
    body = box('body', (-0.25, -0.298, 0.04), (0.25, 0.31, 1.32), STEEL, bev=0.005, seg=2)
    box('plinth', (-0.24, -0.285, 0.0), (0.24, 0.30, 0.042), BASE, bev=0.002)
    spots = []
    for _ in range(3 if variant != 'B' else 2):
        side = rng.choice((-1, 1))
        spots.append(((side * 0.25, rng.uniform(-0.2, 0.2), rng.uniform(0.3, 1.1)), rng.uniform(0.07, 0.13), rng.uniform(0.004, 0.009), (side, 0, 0)))
    spots.append(((rng.uniform(-0.15, 0.15), rng.uniform(-0.1, 0.2), 1.32), 0.12, 0.004, (0, 0, 1)))
    dent(body, spots)
    for k in range(4):
        z0 = 0.058 + k * 0.312
        out = 0.07 if (variant == 'B' and k == 2) else 0.0
        yf = -0.310 - out
        front = box('drawer%d' % k, (-0.236, yf, z0), (0.236, yf + 0.014, z0 + 0.297), STEEL, bev=0.0035, seg=2)
        if (variant, k) in (('A', 1), ('C', 3), ('C', 0)):
            dent(front, [((rng.uniform(-0.12, 0.12), yf, z0 + rng.uniform(0.08, 0.2)), 0.08, 0.005, (0, -1, 0))])
        box('void%d' % k, (-0.232, -0.2995, z0 + 0.004), (0.232, -0.2985, z0 + 0.293), DARK, bev=0)
        zh, zl = z0 + 0.185, z0 + 0.250
        cyl('pull', -0.055, 0.055, 0.005, CHROME, at=(yf - 0.024, zh), axis='X', n=12, bev=0.001)
        for sx in (-1, 1):
            cyl('pull_post', yf - 0.024, yf, 0.004, CHROME, at=(sx * 0.045, zh), axis='Y', n=10, bev=0)
        box('latch', (0.150, yf - 0.012, zh - 0.012), (0.176, yf, zh + 0.012), CHROME, bev=0.003)
        box('card', (-0.040, yf - 0.0022, zl - 0.014), (0.040, yf - 0.0012, zl + 0.014), PAPER, bev=0)
        for (xa, xb, za, zb) in ((-0.045, 0.045, zl + 0.012, zl + 0.018), (-0.045, 0.045, zl - 0.018, zl - 0.012), (-0.045, -0.039, zl - 0.018, zl + 0.018), (0.039, 0.045, zl - 0.018, zl + 0.018)):
            box('holder', (xa, yf - 0.0045, za), (xb, yf, zb), CHROME, bev=0.0008, seg=1)
        if out > 0:
            for sx in (-1, 1):
                box('drawer_side', (sx * 0.230 - 0.004, yf + 0.014, z0 + 0.02), (sx * 0.230 + 0.004, -0.29, z0 + 0.25), STEEL, bev=0.001, seg=1)
            box('drawer_floor', (-0.23, yf + 0.014, z0 + 0.02), (0.23, -0.29, z0 + 0.03), DARK, bev=0)
            for i in range(5):
                yy = yf + 0.022 + i * 0.011
                top = z0 + 0.262 + rng.uniform(-0.006, 0.006)
                f = box('folder', (-0.205, yy, z0 + 0.04), (0.205, yy + 0.0035, top), MANILA, bev=0.0006, seg=1)
                tx = rng.uniform(-0.17, 0.10)
                box('tab', (tx, yy, top - 0.001), (tx + 0.085, yy + 0.0035, top + 0.012), MANILA, bev=0.0006, seg=1)
                rot_about(f, 'X', rng.uniform(-4, 2), (0, yy, z0 + 0.04))
    cyl('lock', -0.312, -0.306, 0.009, CHROME, at=(0.185, 1.285), axis='Y', n=14, bev=0.001)
    key = 'Cabinet' + variant
    ob = baked_asset(key, 'SM_Rec_' + key, 2048, None)
    export(ob, 'SM_Rec_' + key)
    preview([ob], 'cabinet' + variant, (('front', (0.9, -2.0, 1.3), (0, 0, 0.66), 45), ('close', (0.35, -0.8, 1.15), (0, -0.3, 0.85), 50)))


# ================================================================ archive cartons, binders, folders, papers (one shared skin)
def build_archive():
    reset()
    rng = random.Random(1986)

    def board(name, tex, worn):
        return material(name, (0.3, 0.2, 0.1), 0.0, 0.9, worn, 0.0, 0.92, wear=1.1, scratch=0.12, dust=0.55, grain=0.3, grain_scale=500.0,
                        tex=TEXDIR / tex, tex_size=0.5, dust_col=(0.22, 0.18, 0.12), edge=0.0045)
    KRAFT, GREY, WHITE = board('kraft', 'T_Rec_Kraft_base.png', (0.52, 0.42, 0.27)), board('grey', 'T_Rec_GreyBoard_base.png', (0.50, 0.49, 0.44)), board('white', 'T_Rec_WhiteBoard_base.png', (0.50, 0.46, 0.36))
    TAPE = material('tape', (0.40, 0.28, 0.12), 0.0, 0.28, (0.5, 0.4, 0.25), 0.0, 0.45, wear=0.3, scratch=0.3, dust=0.4, grain=0.1)
    HOLE = material('hole', (0.008, 0.006, 0.005), 0.0, 0.95, (0.008, 0.006, 0.005), 0.0, 0.95, wear=0.0, scratch=0.0, dust=0.0, grain=0.0)
    EDGE = material('edge', (0.30, 0.30, 0.31), 0.9, 0.42, (0.5, 0.5, 0.5), 1.0, 0.3, wear=0.6, scratch=0.6, dust=0.8, grain=0.2, rust=0.5)
    MANILA = material('manila', (0.55, 0.40, 0.18), 0.0, 0.8, (0.66, 0.54, 0.32), 0.0, 0.85, wear=0.6, scratch=0.1, dust=0.4, grain=0.4, grain_scale=500.0, edge=0.002)
    PAPER = m_paper()

    def vinyl(name, col):
        return material(name, col, 0.0, 0.42, tuple(min(1.0, c * 2.2 + 0.03) for c in col), 0.0, 0.6, wear=0.9, scratch=0.5, dust=0.6, grain=0.9, grain_scale=380.0, edge=0.003)
    NAVY, BLACK, MAROON = vinyl('navy', (0.025, 0.045, 0.11)), vinyl('blackv', (0.018, 0.018, 0.02)), vinyl('maroon', (0.16, 0.025, 0.028))
    CHROME = m_chrome()
    names, offsets = [], []

    def start(name):
        names.append(name)
        offsets.append((len(names) - 1) * 0.6, )
        return len(names) - 1, (len(names) - 1) * 0.6

    def carton(name, w, d, h, mat, lid=True, ajar=False, tape=False, crush=False, metal=False, holes=True):
        part, ox = start(name)
        hb = h - 0.010 if lid else h
        body = box('body', (ox - w / 2, -d / 2, 0), (ox + w / 2, d / 2, hb), mat, part, bev=0.0035, seg=2)
        spots = [((ox + rng.uniform(-w / 3, w / 3), -d / 2, rng.uniform(0.05, hb - 0.08)), rng.uniform(0.05, 0.09), rng.uniform(0.002, 0.005), (0, -1, 0)),
                 ((ox + w / 2, rng.uniform(-d / 4, d / 4), rng.uniform(0.05, hb - 0.08)), 0.07, 0.003, (1, 0, 0))]
        if crush:
            spots += [((ox - w / 2, -d / 2, hb), 0.16, 0.035, (-0.5, -0.5, 0.7)), ((ox - w / 4, -d / 2, hb * 0.5), 0.12, 0.012, (0, -1, 0))]
        dent(body, spots, cuts=4)
        if lid:
            l = box('lid', (ox - w / 2 - 0.004, -d / 2 - 0.004, h - 0.068), (ox + w / 2 + 0.004, d / 2 + 0.004, h), mat, part, bev=0.004, seg=2)
            if crush:
                dent(l, [((ox - w / 2, -d / 2, h), 0.15, 0.03, (-0.4, -0.4, 0.8))], cuts=4)
            if ajar:
                xf(l, Matrix.Translation((ox, 0, h)) @ Matrix.Rotation(math.radians(5.5), 4, 'Z') @ Matrix.Rotation(math.radians(4.0), 4, 'Y') @ Matrix.Translation((-ox + 0.006, 0.01, -h + 0.014)))
        if holes:
            zc = hb - 0.092
            for sx in (-1, 1):
                x = ox + sx * (w / 2 + 0.0004)
                box('hand', (min(x, x + sx * 0.0004), -0.043, zc - 0.013), (max(x, x + sx * 0.0004), 0.043, zc + 0.013), HOLE, part, bev=0)
        if tape:
            box('tape_top', (ox - w / 2 - 0.0046, -0.024, h + 0.0001), (ox + w / 2 + 0.0046, 0.024, h + 0.0006), TAPE, part, bev=0)
            for sx in (-1, 1):
                x = ox + sx * (w / 2 + 0.0046)
                box('tape_end', (min(x, x + sx * 0.0005), -0.024, h - 0.15), (max(x, x + sx * 0.0005), 0.024, h + 0.0006), TAPE, part, bev=0)
        if metal:
            for sx in (-1, 1):
                for sy in (-1, 1):
                    cx, cy = ox + sx * w / 2, sy * d / 2
                    box('me_a', (min(cx, cx - sx * 0.014), min(cy, cy + sy * 0.0008), 0.004), (max(cx, cx - sx * 0.014), max(cy, cy + sy * 0.0008), hb - 0.07), EDGE, part, bev=0)
                    box('me_b', (min(cx, cx + sx * 0.0008), min(cy, cy - sy * 0.014), 0.004), (max(cx, cx + sx * 0.0008), max(cy, cy - sy * 0.014), hb - 0.07), EDGE, part, bev=0)
        return part, ox

    carton('SM_Rec_BoxA', 0.39, 0.31, 0.26, KRAFT)
    carton('SM_Rec_BoxB', 0.39, 0.31, 0.26, KRAFT, ajar=True)
    carton('SM_Rec_BoxC', 0.39, 0.31, 0.26, GREY, metal=True, holes=False)
    carton('SM_Rec_BoxD', 0.39, 0.31, 0.27, WHITE, tape=True)
    carton('SM_Rec_BoxE', 0.39, 0.31, 0.20, KRAFT, tape=True)
    carton('SM_Rec_BoxF', 0.31, 0.26, 0.30, GREY, holes=False)
    carton('SM_Rec_BoxG', 0.39, 0.31, 0.26, KRAFT, crush=True, tape=True)
    # H: an open carton, folders standing in it
    part, ox = start('SM_Rec_BoxH')
    w, d, h, t = 0.39, 0.31, 0.24, 0.004
    box('floor', (ox - w / 2, -d / 2, 0), (ox + w / 2, d / 2, 0.006), KRAFT, part, bev=0.001, seg=1)
    for (xa, xb, ya, yb) in ((-w / 2, w / 2, -d / 2, -d / 2 + t), (-w / 2, w / 2, d / 2 - t, d / 2), (-w / 2, -w / 2 + t, -d / 2 + t, d / 2 - t), (w / 2 - t, w / 2, -d / 2 + t, d / 2 - t)):
        box('wall', (ox + xa, ya, 0.004), (ox + xb, yb, h), KRAFT, part, bev=0.0012, seg=1)
    for i in range(11):
        y = -d / 2 + 0.02 + i * 0.024 + rng.uniform(-0.003, 0.003)
        top = h + rng.uniform(-0.012, 0.022)
        f = box('folder', (ox - w / 2 + 0.012, y, 0.008), (ox + w / 2 - 0.012, y + 0.004, top), MANILA, part, bev=0.0006, seg=1)
        tx = ox + rng.uniform(-0.15, 0.07)
        tb = box('tab', (tx, y, top - 0.001), (tx + 0.08, y + 0.004, top + 0.012), MANILA, part, bev=0.0006, seg=1)
        for o in (f, tb):
            rot_about(o, 'X', rng.uniform(-7, 5), (ox, y, 0.008))
    # lever-arch binders: the spine to -Y
    for name, col in (('SM_Rec_BinderA', NAVY), ('SM_Rec_BinderB', BLACK), ('SM_Rec_BinderC', MAROON)):
        part, ox = start(name)
        for sx in (-1, 1):
            box('cover', (ox + sx * 0.036 - 0.0015, -0.14, 0), (ox + sx * 0.036 + 0.0015, 0.142, 0.318), col, part, bev=0.0012, seg=1)
        box('spine', (ox - 0.0375, -0.143, 0), (ox + 0.0375, -0.139, 0.318), col, part, bev=0.0015, seg=1)
        box('block', (ox - 0.031, -0.132, 0.008), (ox + 0.031, 0.135, 0.308), PAPER, part, bev=0.001, seg=1)
        box('slip', (ox - 0.026, -0.1436, 0.150), (ox + 0.026, -0.1430, 0.275), PAPER, part, bev=0)
        cyl('ring', -0.1445, -0.1432, 0.0135, CHROME, part, at=(ox, 0.058), axis='Y', n=18, bev=0)
        cyl('ring_hole', -0.1449, -0.1444, 0.0105, HOLE, part, at=(ox, 0.058), axis='Y', n=18, bev=0)
    part, ox = start('SM_Rec_Folders')
    z = 0.0
    for i in range(6):
        th = rng.uniform(0.003, 0.008)
        f = box('flat', (ox - 0.15, -0.118, z), (ox + 0.15, 0.118, z + th), MANILA, part, bev=0.0008, seg=1)
        tb = box('flat_tab', (ox + rng.uniform(-0.12, 0.04), 0.118, z), (ox + rng.uniform(0.06, 0.12), 0.130, z + th), MANILA, part, bev=0.0008, seg=1)
        a = rng.uniform(-4, 4)
        for o in (f, tb):
            rot_about(o, 'Z', a, (ox, 0, 0))
        z += th + 0.0003
    part, ox = start('SM_Rec_Papers')
    z = 0.0
    for i in range(4):
        th = rng.uniform(0.004, 0.012)
        sh = box('sheets', (ox - 0.108, -0.1397, z), (ox + 0.108, 0.1397, z + th), PAPER, part, bev=0.0005, seg=1)
        rot_about(sh, 'Z', rng.uniform(-3, 3), (ox, 0, 0))
        z += th
    tag_parts()
    body = join([o for o, _ in made], 'archive')
    shade(body, 42)
    unwrap(body, margin=0.002)
    bake(body, 'Archive', 4096, 'M_Rec_Archive')
    obs = split_parts(body, names, [(o[0], 0, 0) for o in offsets])
    body.hide_render = True
    for i, o in enumerate(obs):
        o.location.x = i * 0.5
    preview(obs, 'archive', (('row', (3.0, -4.2, 1.6), (3.0, 0, 0.12), 38), ('boxes', (0.9, -1.5, 0.8), (1.0, 0, 0.12), 45), ('binders', (4.6, -1.3, 0.6), (4.7, 0, 0.14), 45)))


ASSETS = {'ceiling': build_ceiling, 'table': build_table, 'lamp': build_lamp, 'typewriter': build_typewriter, 'shelving': build_shelving, 'bay2': build_bay2,
          'cabinetA': lambda: build_cabinet('A'), 'cabinetB': lambda: build_cabinet('B'), 'cabinetC': lambda: build_cabinet('C'), 'archive': build_archive}



if __name__ == '__main__':
    for key in WANT or list(ASSETS):
        print('S47 REC: building', key, flush=True)
        ASSETS[key]()
    print('DONE S47 REC', WANT or list(ASSETS), flush=True)
