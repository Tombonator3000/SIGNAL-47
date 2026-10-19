# Shared pieces of the Blender scripts (tools/blender/*.py, run by tools/blender/build.py inside
# Blender 5.2): the game's coordinates, boxes, booleans, bevels, the two UV sets, the bake on the
# graphics card, and the packing of the baked mask into the exported .glb.
#
# The game is Y up with the array to the north (-Z); Blender is Z up. A point (x, y, z) in the
# game is (x, -z, y) here, and the glTF exporter turns it back.
import bpy, bmesh, math
import numpy as np
from mathutils import Vector

def G(x, y, z):
    return Vector((x, -z, y))

def game(v):
    """A Blender point or direction in the game's terms."""
    return (v[0], v[2], -v[1])

def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0     # (no .blend1 beside the .blend)

def gpu(samples=128):
    """Cycles on the RTX 3080 through OptiX."""
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    prefs = bpy.context.preferences.addons['cycles'].preferences
    prefs.compute_device_type = 'OPTIX'
    prefs.get_devices()
    for d in prefs.devices: d.use = d.type == 'OPTIX'
    sc.cycles.device = 'GPU'
    sc.cycles.samples = samples
    sc.cycles.use_denoising = False
    return [d.name for d in prefs.devices if d.use]

def collection(name):
    c = bpy.data.collections.get(name) or bpy.data.collections.new(name)
    if c.name not in bpy.context.scene.collection.children: bpy.context.scene.collection.children.link(c)
    return c

def mesh_object(name, bm, coll=None):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me)
    (coll or bpy.context.scene.collection).objects.link(o)
    return o

def add_box(bm, x0, x1, y0, y1, z0, z1):
    """A closed box in the game's terms, into a bmesh."""
    vs = [bm.verts.new(G(x, y, z)) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
    i = lambda a, b, c: vs[a * 4 + b * 2 + c]
    for f in [(i(0,0,0), i(0,0,1), i(0,1,1), i(0,1,0)), (i(1,0,0), i(1,1,0), i(1,1,1), i(1,0,1)),
              (i(0,0,0), i(1,0,0), i(1,0,1), i(0,0,1)), (i(0,1,0), i(0,1,1), i(1,1,1), i(1,1,0)),
              (i(0,0,0), i(0,1,0), i(1,1,0), i(1,0,0)), (i(0,0,1), i(1,0,1), i(1,1,1), i(0,1,1))]:
        bm.faces.new(f)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[-6:])

def box_object(name, b, coll=None):
    bm = bmesh.new(); add_box(bm, *b)
    return mesh_object(name, bm, coll)

def boxes_of(o):
    """The boxes a merged three.js mesh was built from (kit.box, kit.faced), as game bounds
    (x0, x1, y0, y1, z0, z1). kit.mergeStatic lays each box's twelve triangles one after the
    other, and the glTF keeps that order; each run of twelve must have a box's eight corners."""
    me, mw = o.data, o.matrix_world
    if len(me.polygons) % 12: raise SystemExit(f'{o.name}: {len(me.polygons)} triangles, not boxes')
    out = []
    for k in range(0, len(me.polygons), 12):
        pts = {tuple(round(c, 4) for c in game(mw @ me.vertices[v].co)) for p in me.polygons[k:k + 12] for v in p.vertices}
        if len(pts) != 8: raise SystemExit(f'{o.name}: triangles {k} to {k + 11} are not a box')
        out.append(tuple(f(p[i] for p in pts) for i in range(3) for f in (min, max)))
    return out

def apply_mods(o):
    bpy.context.view_layer.objects.active = o
    for m in list(o.modifiers): bpy.ops.object.modifier_apply(modifier=m.name)

def boolean(o, others, op):
    """o UNION / DIFFERENCE the others (exact solver), applied; the others are removed."""
    if not others: return o
    tmp = bpy.data.collections.new('_bool')
    for x in others:
        for c in list(x.users_collection): c.objects.unlink(x)
        tmp.objects.link(x)
    m = o.modifiers.new('bool', 'BOOLEAN')
    m.operation = op; m.operand_type = 'COLLECTION'; m.collection = tmp; m.solver = 'EXACT'
    m.use_self = op == 'UNION'
    apply_mods(o)
    for x in list(tmp.objects): bpy.data.objects.remove(x)
    bpy.data.collections.remove(tmp)
    return o

def union_boxes(name, boxes, coll=None):
    """One clean solid from overlapping or touching boxes."""
    objs = [box_object(f'{name}.{i}', b, coll) for i, b in enumerate(boxes)]
    o = boolean(objs[0], objs[1:], 'UNION')
    o.name = name
    tidy(o)
    return o

def tidy(o, angle=0.5):
    """Coplanar pieces left by the booleans merged back into whole faces."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(angle), verts=bm.verts, edges=bm.edges)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(o.data); bm.free()

def bevel(o, width, segments=2, angle=30, max_len=None):
    """Rounds the convex edges sharper than `angle` (a chamfer with one segment); the inside
    corners stay sharp (the bake darkens them). Faces keep their own normals (hardened)."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    lay = bm.edges.layers.float.get('bevel_weight_edge') or bm.edges.layers.float.new('bevel_weight_edge')
    for e in bm.edges:
        ok = e.is_manifold and e.is_convex and e.calc_face_angle(0) > math.radians(angle)
        if ok and max_len is not None and e.calc_length() > max_len: ok = False
        e[lay] = 1.0 if ok else 0.0
    bm.to_mesh(o.data); bm.free()
    for p in o.data.polygons: p.use_smooth = True
    m = o.modifiers.new('bevel', 'BEVEL')
    m.width = width; m.segments = segments; m.limit_method = 'WEIGHT'
    m.harden_normals = True; m.use_clamp_overlap = True; m.miter_outer = 'MITER_ARC'
    apply_mods(o)

def join(objs, name):
    bpy.ops.object.select_all(action='DESELECT')
    for x in objs: x.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    o = bpy.context.view_layer.objects.active
    o.name = name
    return o

def set_materials(o, mats):
    o.data.materials.clear()
    for m in mats: o.data.materials.append(m)

def paint(o, fn, layer='Col'):
    """Per face: fn(centre, normal in the game's terms, polygon) -> (material index, sRGB hex)."""
    me = o.data
    ca = me.color_attributes.get(layer) or me.color_attributes.new(layer, 'FLOAT_COLOR', 'CORNER')
    me.color_attributes.active_color = ca
    lin = lambda c: c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    for p in me.polygons:
        mi, hexc = fn(game(p.center), game(p.normal), p)
        p.material_index = mi
        rgb = [lin(((hexc >> s) & 255) / 255) for s in (16, 8, 0)]
        for li in p.loop_indices: ca.data[li].color = (*rgb, 1.0)

def tile_uv(o, size, layer='UVMap'):
    """World-scale UVs for a tiling picture: each face onto the plane its normal is closest to,
    `size` metres to one repeat (the game's concrete, art/concrete)."""
    me = o.data
    uv = me.uv_layers.get(layer) or me.uv_layers.new(name=layer)
    for p in me.polygons:
        n = p.normal; a = max(range(3), key=lambda k: abs(n[k]))
        for li in p.loop_indices:
            c = me.vertices[me.loops[li].vertex_index].co
            if a == 0: u, v = (c.y if n.x > 0 else -c.y), c.z
            elif a == 1: u, v = (-c.x if n.y > 0 else c.x), c.z
            else: u, v = c.x, (c.y if n.z > 0 else -c.y)
            uv.data[li].uv = (u / size, v / size)

def lightmap_uv(o, layer='Lightmap', margin=0.004, shrink=None):
    """A second UV set where no two faces share texels, for the bake: smart projection, then
    packed. shrink(centre, normal) -> scale of a face's island (the roofs nobody sees close)."""
    me = o.data
    uv = me.uv_layers.get(layer) or me.uv_layers.new(name=layer)
    me.uv_layers.active = uv
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.0, area_weight=0.0, correct_aspect=True, scale_to_bounds=False)
    bpy.ops.object.mode_set(mode='OBJECT')
    if shrink:
        # scale each island about its own centre by the weight of its faces
        bm = bmesh.new(); bm.from_mesh(me)
        lay = bm.loops.layers.uv[layer]
        islands = uv_islands(bm, lay)
        for faces in islands:
            k = min(shrink(game(f.calc_center_median()), game(f.normal)) for f in faces)
            if k == 1: continue
            ls = [l for f in faces for l in f.loops]
            cx = sum(l[lay].uv.x for l in ls) / len(ls); cy = sum(l[lay].uv.y for l in ls) / len(ls)
            for l in ls: l[lay].uv = ((l[lay].uv.x - cx) * k + cx, (l[lay].uv.y - cy) * k + cy)
        bm.to_mesh(me); bm.free()
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.select_all(action='SELECT')
    bpy.ops.uv.pack_islands(margin=margin, rotate=True, shape_method='CONCAVE', scale=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    me.uv_layers.active = me.uv_layers[0]

def uv_islands(bm, lay):
    """Faces grouped by shared UV edges."""
    key = lambda l: (round(l[lay].uv.x, 6), round(l[lay].uv.y, 6))
    parent = {f: f for f in bm.faces}
    def find(f):
        while parent[f] is not f: parent[f] = parent[parent[f]]; f = parent[f]
        return f
    for e in bm.edges:
        if len(e.link_faces) != 2: continue
        a, b = e.link_faces
        la = [l for l in a.loops if l.edge is e][0]; lb = [l for l in b.loops if l.edge is e][0]
        # the same edge, walked the other way round in the other face
        if {key(la), key(la.link_loop_next)} == {key(lb), key(lb.link_loop_next)}:
            parent[find(a)] = find(b)
    groups = {}
    for f in bm.faces: groups.setdefault(find(f), []).append(f)
    return list(groups.values())

def image(name, size, float_=False):
    img = bpy.data.images.get(name)
    if img: bpy.data.images.remove(img)
    img = bpy.data.images.new(name, size, size, alpha=True, float_buffer=float_)
    img.colorspace_settings.name = 'Non-Color'
    return img

def bake_emit(o, img, build, samples=1, margin=8, uv='Lightmap'):
    """Bakes an emission shader into img on the UV set `uv`. build(nodes, links) -> the colour
    output socket to emit. Every material of o is swapped for the bake one and put back."""
    sc = bpy.context.scene
    sc.cycles.samples = samples
    m = bpy.data.materials.new('_bake')
    nt = m.node_tree
    for n in list(nt.nodes): nt.nodes.remove(n)
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    em = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(build(nt.nodes, nt.links), em.inputs['Color'])
    nt.links.new(em.outputs['Emission'], out.inputs['Surface'])
    tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = img
    nt.nodes.active = tex
    keep = [s.material for s in o.material_slots]
    for s in o.material_slots: s.material = m
    o.data.uv_layers.active = o.data.uv_layers[uv]
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    sc.render.bake.margin = margin; sc.render.bake.margin_type = 'EXTEND'
    sc.render.bake.use_clear = True
    bpy.ops.object.bake(type='EMIT')
    for s, k in zip(o.material_slots, keep): s.material = k
    o.data.uv_layers.active = o.data.uv_layers[0]
    bpy.data.materials.remove(m)

def baked_ok(img, what):
    """Stops when a bake came out empty or flat (seen once while the graphics card was full of
    browser tests: an all-black occlusion, and a mask that looked like nothing)."""
    a = pixels(img)[..., :3]
    if not np.isfinite(a).all() or a.max() <= 0.0 or a.std() < 1e-3:
        raise SystemExit(f'the {what} bake came out empty (max {a.max():.3f}, spread {a.std():.4f}): run it again with the graphics card free')
    return a

def pixels(img):
    a = np.empty(img.size[0] * img.size[1] * 4, np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(img.size[1], img.size[0], 4)

def save_png(img, path, arr=None):
    if arr is not None: img.pixels.foreach_set(np.ascontiguousarray(arr, np.float32).ravel())
    img.filepath_raw = path; img.file_format = 'PNG'; img.save()

def save_mask(arr, path):
    """An RGB array 0..1 (rows bottom up, as Blender keeps them) to an 8-bit PNG, as data."""
    h, w = arr.shape[:2]
    img = bpy.data.images.new('_mask', w, h, alpha=False, float_buffer=False)
    img.colorspace_settings.name = 'Non-Color'
    a = np.ones((h, w, 4), np.float32); a[..., :3] = np.clip(arr[..., :3], 0, 1)
    img.pixels.foreach_set(a.ravel())
    img.filepath_raw = path; img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)

# ---------- noise for the masks (numpy, deterministic) ----------
def _hash(ix, iy, iz, seed):
    h = (ix * 374761393 + iy * 668265263 + iz * 1274126177 + seed * 2246822519) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0

def noise3(x, y, z, seed=0):
    """Value noise 0..1 at points (arrays), smooth between the lattice."""
    fx, fy, fz = np.floor(x), np.floor(y), np.floor(z)
    ix, iy, iz = fx.astype(np.int64), fy.astype(np.int64), fz.astype(np.int64)
    tx, ty, tz = x - fx, y - fy, z - fz
    s = lambda t: t * t * (3 - 2 * t)
    tx, ty, tz = s(tx), s(ty), s(tz)
    acc = 0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (tx if dx else 1 - tx) * (ty if dy else 1 - ty) * (tz if dz else 1 - tz)
                acc = acc + w * _hash(ix + dx, iy + dy, iz + dz, seed)
    return acc

def fbm(x, y, z, octaves=4, seed=0):
    acc, amp, tot = 0, 1.0, 0
    for k in range(octaves):
        acc = acc + amp * noise3(x * 2 ** k, y * 2 ** k, z * 2 ** k, seed + k * 17)
        tot += amp; amp *= 0.5
    return acc / tot

def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)

# ---------- the truck's tools (tools/blender/truck.py): outlines, extrusions, sweeps, parts ----------
class Shape:
    """A closed 2D outline as three.js's Shape draws it (drive/Truck.ts): lines, arcs (absarc,
    clockwise or not) and quadratic curves, as a list of points."""
    def __init__(self, x, y): self.pts = [(x, y)]
    def line(self, x, y):
        if (x, y) != self.pts[-1]: self.pts.append((x, y))
        return self
    def arc(self, cx, cy, r, a0, a1, cw, n=24):
        d = a1 - a0
        while d < 0: d += 2 * math.pi
        while d > 2 * math.pi: d -= 2 * math.pi
        if cw: d -= 2 * math.pi
        for i in range(n + 1):
            a = a0 + d * i / n
            self.line(cx + r * math.cos(a), cy + r * math.sin(a))
        return self
    def quad(self, cx, cy, x, y, n=6):
        x0, y0 = self.pts[-1]
        for i in range(1, n + 1):
            t = i / n
            self.line((1 - t) ** 2 * x0 + 2 * (1 - t) * t * cx + t * t * x, (1 - t) ** 2 * y0 + 2 * (1 - t) * t * cy + t * t * y)
        return self
    def points(self):
        p = self.pts[:-1] if self.pts[0] == self.pts[-1] else self.pts
        return [q for i, q in enumerate(p) if i == 0 or (abs(q[0] - p[i - 1][0]) > 1e-7 or abs(q[1] - p[i - 1][1]) > 1e-7)]

def rrect(x0, y0, x1, y1, r, n=4):
    """Truck.ts's rrect: a rectangle with quadratic corners."""
    s = Shape(x0 + r, y0).line(x1 - r, y0).quad(x1, y0, x1, y0 + r, n).line(x1, y1 - r).quad(x1, y1, x1 - r, y1, n)
    s.line(x0 + r, y1).quad(x0, y1, x0, y1 - r, n).line(x0, y0 + r).quad(x0, y0, x0 + r, y0, n)
    return s

def extrude(name, pts3, along, length, coll=None):
    """An outline (game points, a flat polygon) as a solid: the face and a copy `length` along
    the game direction `along`."""
    bm = bmesh.new()
    vs = [bm.verts.new(G(*p)) for p in pts3]
    f = bm.faces.new(vs)
    r = bmesh.ops.extrude_face_region(bm, geom=[f])
    moved = [v for v in r['geom'] if isinstance(v, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, verts=moved, vec=G(*along) * length)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(name, bm, coll)

def side(name, shape, w, x=0.0, coll=None):
    """Truck.ts's side(): a side profile (u = -z forward, y up) across the truck, w wide, centred on x."""
    return extrude(name, [(x - w / 2, y, -u) for u, y in shape.points()], (1, 0, 0), w, coll)

def face_z(name, shape, d, z=0.0, coll=None):
    """Truck.ts's face(): a face outline (x, y) along z, d deep, centred on z."""
    return extrude(name, [(x, y, z - d / 2) for x, y in shape.points()], (0, 0, 1), d, coll)

def round_edges(o, width, segments=3, angle=30):
    """Every edge sharper than `angle` rounded (the modifier's angle limit), hardened normals."""
    for p in o.data.polygons: p.use_smooth = True
    m = o.modifiers.new('bevel', 'BEVEL')
    m.width = width; m.segments = segments; m.limit_method = 'ANGLE'; m.angle_limit = math.radians(angle)
    m.harden_normals = True; m.use_clamp_overlap = True; m.miter_outer = 'MITER_ARC'
    apply_mods(o)

def sweep(name, rings, closed=False, caps=True, coll=None):
    """Rings of game points (each as many, in the same turn) joined in order into a tube; the
    ends capped. The caller lays each ring: a profile carried along a path."""
    bm = bmesh.new()
    R = [[bm.verts.new(G(*p)) for p in ring] for ring in rings]
    n, m = len(R), len(R[0])
    for i in range(n if closed else n - 1):
        r0, r1 = R[i], R[(i + 1) % n]
        for k in range(m):
            bm.faces.new((r0[k], r0[(k + 1) % m], r1[(k + 1) % m], r1[k]))
    if caps and not closed:
        bm.faces.new(R[0][::-1]); bm.faces.new(R[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(name, bm, coll)

def revolve(bm, profile, n, a0=0.0, a1=2 * math.pi, closed=True):
    """A lathe about the game's X axis: profile (x, r) points, n steps round. Its faces look the
    way the profile turns: towards +x where r falls along it, outwards where x grows. Returns
    the rings of verts (game x, r cos a, r sin a) and the faces by (step, profile segment)."""
    rings = []
    steps = n if closed else n + 1
    for i in range(steps):
        a = a0 + (a1 - a0) * i / n
        rings.append([bm.verts.new(G(x, r * math.cos(a), r * math.sin(a))) for x, r in profile])
    faces = {}
    for i in range(n):
        r0, r1 = rings[i], rings[(i + 1) % steps]
        for k in range(len(profile) - 1):
            faces[i, k] = bm.faces.new((r0[k], r1[k], r1[k + 1], r0[k + 1]))
    return rings, faces

def weld(o, dist=1e-5):
    bm = bmesh.new(); bm.from_mesh(o.data)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=dist)
    bm.to_mesh(o.data); bm.free()
    if o.data.has_custom_normals:
        with bpy.context.temp_override(object=o, active_object=o):
            bpy.ops.mesh.customdata_custom_splitnormals_clear()

def loose_parts(o):
    """o welded and cut into its loose parts (the boxes and extrusions three.js merged), each its own object."""
    weld(o)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.separate(type='LOOSE'); bpy.ops.object.mode_set(mode='OBJECT')
    return list(bpy.context.selected_objects)

def bounds(o):
    """Game bounds (x0, x1, y0, y1, z0, z1) of an object's vertices."""
    mw = o.matrix_world
    ps = [game(mw @ v.co) for v in o.data.vertices]
    return tuple(f(p[i] for p in ps) for i in range(3) for f in (min, max))

def smooth(o, angle=35, weighted=True):
    """Smooth where the faces meet at less than `angle` (the game's flat facets become round),
    sharp elsewhere; weighted normals, so the big faces stay flat."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    for e in bm.edges:
        e.smooth = len(e.link_faces) == 2 and e.calc_face_angle(0) < math.radians(angle)
    for f in bm.faces: f.smooth = True
    bm.to_mesh(o.data); bm.free()
    if weighted:
        m = o.modifiers.new('wn', 'WEIGHTED_NORMAL'); m.keep_sharp = True; m.weight = 50; m.mode = 'FACE_AREA'
        apply_mods(o)
