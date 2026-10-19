# SARO 07 modelled in Blender from the game's own truck (tools/blender/build.py truck).
# In: the truck as drive/Truck.ts builds it, standing at the origin (export.py: body:*, shell:*,
# cab:*, the merged meshes named after the truck's materials, and `wheel`). The pieces three.js
# merged are cut apart again and each is found by where it stands (the script stops if the game
# has changed). Most are kept as the game made them, rounded where they were flat facets and
# with their box edges chamfered; these are modelled again in their place:
#   the front clip    fenders and header in the game's side profile, a pocket for the hood
#   the hood          crowned, in its pocket with a real gap round it (the game paints the gaps)
#   the arch lips     a flared lip swept round both wheel arches
#   the cab's doors   the game's panel with the door seams cut in (where the door decal draws
#                     them), and the seams up the A and B pillars
#   the bumpers       chrome C-section stampings that wrap round the corners
#   the mirrors       rounded heads with a rim round the glass
#   the grille        an egg-crate of slats in front of the game's grille picture
#   the plates        frames round both licence plates
#   the cab's roof    crowned (2.5 cm), its edges rounded down to the drip rails; the glasshouse
#                     tapered above the belt (6 cm a side at the roof)
#   the fenders       their front corners rounded (9 cm)
#   the glass         rubber gaskets round the windscreen, the door windows and the back window
#   the lamps         chrome cups behind the headlamps' lenses, bezels round the tail lamps
#   the wheel         tyre with shoulders and tread blocks, a steel wheel with five slots, the
#                     lug nuts and a cap (the game's four instances turn and steer it)
# Out: src/assets/models/saro_truck.glb, in the truck's own metres: body_paint, body_chrome,
# body_dark (always seen), shell_paint, shell_chrome, shell_dark, shell_glass (the cab from
# outside, hidden while driving; its dark trim has in it what shows of the cab through the glass,
# the bench's back and its headrests, the wheel, the dash's top, told apart by the vertex colour),
# wheel, and shadow (a quad on the ground under it with its shade, 256 by 512 px: the game
# multiplies the ground by it). The paint has two UV sets, the first tiling the game's paint
# picture every 1.1 m (as Truck.ts's boxUV); everything has a lightmap (the first and only
# set where nothing tiles), where the graphics card bakes one mask (SIZE px, 1024 by default):
# R occlusion (the ground under the truck and the wheels in their arches), G dirt (the
# road's dust up the sides and sprayed behind the wheels, on the tailgate, on what is flat and in
# the seams; wash streaks), B worn edges (chipped paint, the bed floor's scratches). The wheel has
# its own small mask (512 px), baked on its own (it turns). The game reads both as aoMap
# (world/blenderModels.ts).
# Usage: blender -b --python tools/blender/truck.py -- EXPORT_DIR OUT_GLB WORK_DIR [SIZE]
import bpy, bmesh, math, os, sys, time
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bakekit as K
from bakekit import G

argv = sys.argv[sys.argv.index('--') + 1:]
EXP, OUT, WORK = argv[0], argv[1], argv[2]
SIZE = int(argv[3]) if len(argv) > 3 else 1024
WHEEL_SIZE = 512
t0 = time.time()
def log(*a): print(f'[truck {time.time() - t0:5.1f}s]', *a, flush=True)

K.reset()
log('devices:', K.gpu())
bpy.ops.import_scene.gltf(filepath=os.path.join(EXP, 'truck_game.glb'))
src = {}
for o in list(bpy.context.scene.objects):
    if o.type != 'MESH': continue
    name = (o.data.materials[0].name if o.data.materials else o.name).split('.')[0]
    if name.startswith('cab:') or name in ('body:decal', 'shell:decal'): bpy.data.objects.remove(o); continue
    src.setdefault(name, []).append(o)

# ---------- the game's pieces ----------
# Each merged mesh cut into its loose parts: {group, mat, obj, bounds}
pool = []
for name, objs in src.items():
    if name == 'wheel':
        for o in objs: bpy.data.objects.remove(o)
        continue
    g, mat = name.split(':')
    for o in objs:
        for part in K.loose_parts(o):
            pool.append({'g': g, 'mat': mat, 'o': part, 'b': K.bounds(part)})
log(f'{len(pool)} pieces from the game')

def take(g, mat, want, tol=0.006):
    for p in pool:
        if p['g'] == g and p['mat'] == mat and all(abs(p['b'][i] - want[i]) < tol for i in range(6)):
            pool.remove(p)
            return p['o']
    raise SystemExit(f'the game has changed: no {g}:{mat} piece at {want}')
def drop(g, mat, want):
    bpy.data.objects.remove(take(g, mat, want))

# what goes: modelled again below, or (the hood's painted gaps) made real
drop('body', 'paint', (-0.99, 0.99, 0.5, 1.13, -2.62, -0.86))                 # the front clip
for s in (-1, 1):
    x = (-1.007, -0.973) if s < 0 else (0.973, 1.007)
    drop('body', 'paint', (*x, 0.496, 0.868, -2.279, -1.321))               # the arch lips
    drop('body', 'paint', (*x, 0.545, 0.868, 1.04, 1.96))
    drop('body', 'trim', ((-1.325, -1.175) if s < 0 else (1.175, 1.325)) + (1.04, 1.4, -0.69, -0.6))   # the mirror heads
    drop('body', 'trim', ((-0.804, -0.796) if s < 0 else (0.796, 0.804)) + (1.11, 1.136, -2.56, -0.92))   # the hood's side gaps
drop('body', 'trim', (-0.8, 0.8, 1.103, 1.109, -2.524, -2.516))              # its front gap
drop('body', 'chrome', (-1.02, 1.02, 0.37, 0.565, -2.775, -2.6))             # the bumpers
drop('body', 'chrome', (-1.0, 1.0, 0.38, 0.56, 2.6, 2.8))
drop('shell', 'paint', (-0.99, 0.99, 0.5, 1.13, -0.86, 0.76))                # the doors and cab corners
drop('shell', 'paint', (-0.93, 0.93, 1.765, 1.815, -0.44, 0.75))             # the cab's roof (crowned below)
frames = [take('shell', 'paint', ((-0.95, -0.9) if s < 0 else (0.9, 0.95)) + (1.12, 1.8, -0.86, 0.76)) for s in (-1, 1)]
# the lamps and the mirror glass stay the game's (lit by it): here only for the occlusion
CONTEXT_MATS = {'head', 'park', 'tail', 'rev', 'mirror', 'beacon'}
ctx = [p['o'] for p in pool if p['mat'] in CONTEXT_MATS]
glass = [p['o'] for p in pool if p['mat'] == 'glass']
kept = [p for p in pool if p['mat'] not in CONTEXT_MATS and p['mat'] != 'glass']
log(f'{len(kept)} kept, {len(ctx)} lamps and mirror glass for the bake, {len(glass)} panes')

# ---------- tools ----------
def bevel_by(o, width, segments, weight, angle=30):
    """Edges sharper than `angle` rounded, each at width x weight(midpoint in game terms)."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    lay = bm.edges.layers.float.get('bevel_weight_edge') or bm.edges.layers.float.new('bevel_weight_edge')
    for e in bm.edges:
        ok = len(e.link_faces) == 2 and e.calc_face_angle(0) > math.radians(angle)
        e[lay] = weight(K.game((e.verts[0].co + e.verts[1].co) / 2)) if ok else 0.0
    bm.to_mesh(o.data); bm.free()
    m = o.modifiers.new('bevel', 'BEVEL')
    m.width = width; m.segments = segments; m.limit_method = 'WEIGHT'
    m.use_clamp_overlap = True; m.miter_outer = 'MITER_ARC'
    K.apply_mods(o)

def is_box(o):
    return len(o.data.vertices) == 8 and len(o.data.polygons) in (6, 12)

def boxes(name, bs):
    """Several boxes (game bounds) as one object."""
    bm = bmesh.new()
    for b in bs: K.add_box(bm, *b)
    return K.mesh_object(name, bm)

new = []        # (object, group, kind, smoothing angle)
def add(o, g, kind, angle=35):
    new.append((o, g, kind, angle))
    return o

# ---------- the front clip and the hood ----------
FRONT = K.Shape(0.86, 0.5).line(1.348, 0.5).arc(1.8, 0.37, 0.47, math.pi - 0.2795, 0.2795, True, 28)
FRONT.line(2.44, 0.5).quad(2.6, 0.5, 2.62, 0.62).line(2.62, 1.04).quad(2.62, 1.1, 2.56, 1.105).line(1.4, 1.12).line(0.86, 1.13)
HOOD_X, HOOD_Z0, HOOD_Z1, GAP = 0.8, -2.565, -0.975, 0.006
fenders = K.side('fenders', FRONT, 1.98)
K.boolean(fenders, [K.box_object('pocket', (-HOOD_X, HOOD_X, 1.095, 1.4, HOOD_Z0, HOOD_Z1))], 'DIFFERENCE')
K.tidy(fenders)
in_pocket = lambda p: abs(p[0]) < HOOD_X + 0.002 and p[1] > 1.09 and HOOD_Z0 - 0.002 < p[2] < HOOD_Z1 + 0.002
# the fenders' front corners rounded hard (9 cm: a stamped wing, not a box's corner)
front_corner = lambda p: abs(p[0]) > 0.95 and p[2] < -2.55 and 0.55 < p[1] < 1.08
bevel_by(fenders, 0.03, 4, lambda p: 0.2 if in_pocket(p) else 3.0 if front_corner(p) else 1.0)
add(fenders, 'body', 'paint')

def hood_top(x, z):
    """The profile's top line (FRONT) under the hood, and its crown across."""
    base = float(np.interp(-z, [0.86, 1.4, 2.56], [1.13, 1.12, 1.105]))
    return base + 0.003 + 0.013 * (1 - (x / HOOD_X) ** 2)
def hood():
    nx, nz = 8, 6
    x0, x1, z0, z1 = -HOOD_X + GAP, HOOD_X - GAP, HOOD_Z0 + GAP, HOOD_Z1
    bm = bmesh.new()
    T = [[bm.verts.new(G(x0 + (x1 - x0) * i / nx, hood_top(x0 + (x1 - x0) * i / nx, z0 + (z1 - z0) * j / nz), z0 + (z1 - z0) * j / nz))
          for j in range(nz + 1)] for i in range(nx + 1)]
    for i in range(nx):
        for j in range(nz):
            bm.faces.new((T[i][j], T[i + 1][j], T[i + 1][j + 1], T[i][j + 1]))
    ring = [(i, 0) for i in range(nx)] + [(nx, j) for j in range(nz)] + [(i, nz) for i in range(nx, 0, -1)] + [(0, j) for j in range(nz, 0, -1)]
    B = {(i, j): bm.verts.new(G(x0 + (x1 - x0) * i / nx, 1.085, z0 + (z1 - z0) * j / nz)) for i, j in ring}
    for k in range(len(ring)):
        (a, b), (c, d) = ring[k], ring[(k + 1) % len(ring)]
        bm.faces.new((T[a][b], B[a, b], B[c, d], T[c][d]))
    bm.faces.new([B[a, b] for a, b in ring][::-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = K.mesh_object('hood', bm)
    K.round_edges(o, 0.007, 2, 40)
    return o
add(hood(), 'body', 'paint', 30)

# ---------- the arch lips ----------
LIP = [(-0.004, -0.014), (0.01, -0.014), (0.019, -0.006), (0.023, 0.006), (0.019, 0.016), (0.008, 0.022), (-0.004, 0.022)]
def lip(s, cu, a0):
    rings = []
    for a in np.linspace(a0, math.pi - a0, 22):
        R = (0.0, math.sin(a), -math.cos(a))
        rings.append([(s * (0.99 + dx), 0.37 + (0.47 + dr) * R[1], -cu + (0.47 + dr) * R[2]) for dx, dr in LIP])
    return K.sweep(f'lip{s}{cu}', rings)
for s in (-1, 1):
    add(lip(s, 1.8, 0.2795), 'body', 'paint', 50)
    add(lip(s, -1.5, 0.3929), 'body', 'paint', 50)

# ---------- the doors: the game's panel with the seams cut in ----------
door = K.side('doors', K.rrect(-0.76, 0.5, 0.86, 1.13, 0.02), 1.98)
K.round_edges(door, 0.022, 3, 30)
DOOR_Z0, DOOR_Z1, DOOR_Y0, CR = -0.79, 0.41, 0.528, 0.033     # where Truck.ts's door decal paints the gap
def seam(s, path, w=0.0022, d=0.006):
    """A groove along a path on the side (game y, z points), as a cutter."""
    rings = []
    for i, (y, z) in enumerate(path):
        y1, z1 = path[min(i + 1, len(path) - 1)]; y0, z0 = path[max(i - 1, 0)]
        ty, tz = y1 - y0, z1 - z0; l = math.hypot(ty, tz); ny, nz = -tz / l, ty / l
        xs = (s * (0.99 - d), s * 1.05)
        rings.append([(xs[0], y + ny * w, z + nz * w), (xs[1], y + ny * w, z + nz * w), (xs[1], y - ny * w, z - nz * w), (xs[0], y - ny * w, z - nz * w)])
    return K.sweep('seam', rings)
def door_path():
    p = [(1.2, DOOR_Z0), (DOOR_Y0 + CR, DOOR_Z0)]
    p += [(DOOR_Y0 + CR - CR * math.sin(a), DOOR_Z0 + CR - CR * math.cos(a)) for a in np.linspace(0, math.pi / 2, 6)[1:]]
    p += [(DOOR_Y0, DOOR_Z1 - CR)]
    p += [(DOOR_Y0 + CR - CR * math.cos(a), DOOR_Z1 - CR + CR * math.sin(a)) for a in np.linspace(0, math.pi / 2, 6)[1:]]
    p += [(1.2, DOOR_Z1)]
    return p
K.boolean(door, [seam(s, door_path()) for s in (-1, 1)], 'DIFFERENCE')
add(door, 'shell', 'paint', 35)
# up the pillars: the B pillar straight up, the A pillar along the windscreen's slope
for s, f in zip((-1, 1), frames):
    f.name = f'frame{s}'
    K.weld(f)
    cut = [seam(s, [(1.1, DOOR_Z1), (1.79, DOOR_Z1)], d=0.006), seam(s, [(1.1, -0.802), (1.77, -0.425)], d=0.006)]
    for c in cut:   # the frames stand at 0.9 to 0.95: the cutters' faces moved out to them
        for v in c.data.vertices:
            gx = K.game(v.co)[0]
            v.co = G(s * (0.95 - 0.006) if abs(gx) < 1.0 else s * 1.05, K.game(v.co)[1], K.game(v.co)[2])
    K.boolean(f, cut, 'DIFFERENCE')
    add(f, 'shell', 'paint', 35)

# ---------- the bumpers: C-section chrome, wrapped round the corners ----------
def bumper(name, face_z, back, straight, R, ang, prof):
    """prof: (depth behind the face, y) round the section; back: +1 if 'behind' is +z."""
    path = []   # (x, z, nx, nz): points on the face line and the outward normal
    n_wrap = 6
    for side in (-1, 1):
        pts = []
        for i in range(n_wrap, 0, -1):
            a = ang * i / n_wrap
            pts.append((side * (straight + R * math.sin(a)), face_z + back * R * (1 - math.cos(a)), side * math.sin(a), -back * math.cos(a)))
        path += pts if side < 0 else pts[::-1]
        if side < 0:
            path += [(x, face_z, 0.0, -back) for x in (-straight, straight)]
    rings = [[(x - nx * d, y, z - nz * d) for d, y in prof] for x, z, nx, nz in path]
    return K.sweep(name, rings)
def channel(outer, t=0.004):
    """An open section (depth, y) given its outer line, as a closed section t thick."""
    inner = []
    for i, (d, y) in enumerate(outer):
        d1, y1 = outer[min(i + 1, len(outer) - 1)]; d0, y0 = outer[max(i - 1, 0)]
        td, ty = d1 - d0, y1 - y0; l = math.hypot(td, ty)
        inner.append((d - ty / l * t, y + td / l * t))
    return outer + inner[::-1]
FRONT_BUMPER = channel([(0.06, 0.565), (0.04, 0.562), (0.02, 0.552), (0.006, 0.535), (0.0, 0.5), (0.0, 0.43), (0.006, 0.397), (0.02, 0.382), (0.04, 0.373), (0.06, 0.37)])
REAR_BUMPER = channel([(0.2, 0.56), (0.02, 0.56), (0.006, 0.554), (0.0, 0.535), (0.0, 0.42), (0.006, 0.394), (0.02, 0.381), (0.07, 0.38)])
add(bumper('bumper_front', -2.775, 1, 0.93, 0.1, math.radians(62), FRONT_BUMPER), 'body', 'chrome', 50)
add(bumper('bumper_rear', 2.8, -1, 0.86, 0.21, math.radians(50), REAR_BUMPER), 'body', 'chrome', 50)
# brackets from the frame horns to them, dark
add(boxes('brackets', [(sx - 0.035, sx + 0.035, 0.42, 0.5, -2.74, -2.38) for sx in (-0.46, 0.46)]
          + [(sx - 0.035, sx + 0.035, 0.42, 0.52, 2.45, 2.62) for sx in (-0.46, 0.46)]), 'body', 'under', 50)

# ---------- the mirrors ----------
for s in (-1, 1):
    m = K.side(f'mirror{s}', K.rrect(0.6, 1.04, 0.69, 1.4, 0.02), 0.15, s * 1.25)
    K.round_edges(m, 0.014, 3, 30)
    add(m, 'body', 'dark', 35)
    # a rim round the glass (the game's, 0.13 x 0.33 at z -0.598), standing 8 mm back
    cx, cy, hw, hh, w = s * 1.25, 1.22, 0.068, 0.168, 0.008
    rim = boxes(f'rim{s}', [(cx - hw - w, cx + hw + w, cy + hh, cy + hh + w, -0.6, -0.592), (cx - hw - w, cx + hw + w, cy - hh - w, cy - hh, -0.6, -0.592),
                            (cx - hw - w, cx - hw, cy - hh, cy + hh, -0.6, -0.592), (cx + hw, cx + hw + w, cy - hh, cy + hh, -0.6, -0.592)])
    add(rim, 'body', 'dark', 50)

# ---------- the grille's egg-crate, the plates' frames ----------
# (the slats stand on the lines the grille picture draws, Truck.ts DECAL.grille: 14 columns
# between the lamps, four rows; the chrome bars and the centre bar are the game's)
crate = [(-0.45 + i * 0.9 / 14 - 0.0025, -0.45 + i * 0.9 / 14 + 0.0025, 0.652, 1.028, -2.633, -2.6235) for i in range(15) if i != 7]
crate += [(-0.45, 0.45, y - 0.0025, y + 0.0025, -2.633, -2.6235) for y in (1.04 - (10 + 27 * j) / 128 * 0.4 for j in range(5))]
add(boxes('crate', crate), 'body', 'grille', 50)
def plate_frame(z0, z1):
    return [(-0.155, 0.155, 0.535, 0.547, z0, z1), (-0.155, 0.155, 0.383, 0.395, z0, z1), (-0.157, -0.145, 0.395, 0.535, z0, z1), (0.145, 0.157, 0.395, 0.535, z0, z1)]
add(boxes('plates', plate_frame(-2.781, -2.7755) + plate_frame(2.8025, 2.808)), 'body', 'chrome', 50)

# ---------- the cab's roof, crowned ----------
# (Truck.ts's roof profile along the cab, 1.765 to 1.815, now 2.5 cm higher in the middle, its
# edges rounded down to the drip rails)
RU = [0.44, 0.435, 0.42, 0.39, 0.36, 0.2, 0.0, -0.3, -0.6, -0.7, -0.725, -0.745, -0.75]
def roof_top(u): return float(np.interp(-u, [-0.44, -0.43, -0.36, 0.7, 0.75], [1.768, 1.79, 1.815, 1.815, 1.8]))
def roof():
    rings = []
    xs = np.linspace(-0.93, 0.93, 15)
    for u in RU:
        top = roof_top(u); c = 0.025 * min(1.0, (top - 1.765) / 0.05)
        rings.append([(x, top + c * (1 - (x / 0.93) ** 2), -u) for x in xs] + [(0.93, 1.765, -u), (-0.93, 1.765, -u)])
    o = K.sweep('roof', rings)
    K.round_edges(o, 0.012, 2, 25)
    return o
add(roof(), 'shell', 'paint', 30)

# ---------- rubber round the glass, cups behind the headlamps, bezels round the tail lamps ----------
def offset(pts, d):
    """A convex outline moved out by d (mitred corners)."""
    n = len(pts)
    sgn = 1 if sum(pts[i][0] * pts[(i + 1) % n][1] - pts[(i + 1) % n][0] * pts[i][1] for i in range(n)) > 0 else -1
    def nrm(a, b):
        dx, dy = b[0] - a[0], b[1] - a[1]; l = math.hypot(dx, dy)
        return (sgn * dy / l, -sgn * dx / l)
    out = []
    for i in range(n):
        n1, n2 = nrm(pts[i - 1], pts[i]), nrm(pts[i], pts[(i + 1) % n])
        m = (n1[0] + n2[0], n1[1] + n2[1]); k = m[0] * n1[0] + m[1] * n1[1]
        out.append((pts[i][0] + m[0] * d / k, pts[i][1] + m[1] * d / k))
    return out
def ring(name, inner, d, at, t0, t1):
    """A flat band round an outline (2D), d wide, as a solid from t0 to t1 across it; at(p, t)
    places a point of the outline's plane in the game."""
    outer = offset(inner, d)
    bm = bmesh.new()
    I0, O0 = [bm.verts.new(G(*at(p, t0))) for p in inner], [bm.verts.new(G(*at(p, t0))) for p in outer]
    I1, O1 = [bm.verts.new(G(*at(p, t1))) for p in inner], [bm.verts.new(G(*at(p, t1))) for p in outer]
    n = len(inner)
    for k in range(n):
        j = (k + 1) % n
        for f in ((I1[k], I1[j], O1[j], O1[k]), (I0[k], O0[k], O0[j], I0[j]), (O0[k], O1[k], O1[j], O0[j]), (I0[k], I0[j], I1[j], I1[k])):
            bm.faces.new(f)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return K.mesh_object(name, bm)
PANE = [(0.746, 1.17), (-0.38, 1.17), (-0.38, 1.71), (-0.36, 1.73), (0.396, 1.73)]   # Truck.ts: the door windows (u = -z, y)
for sx in (-1, 1):
    add(ring(f'gasket{sx}', PANE, 0.016, lambda p, t, sx=sx: (sx * t, p[1], -p[0]), 0.917, 0.953), 'shell', 'dark', 50)
WS_A = 0.552   # the windscreen: 1.76 by 0.705 at (0, 1.45, -0.615), leaning back
ca, sa = math.cos(WS_A), math.sin(WS_A)
WS = [(-0.88, -0.3525), (0.88, -0.3525), (0.88, 0.3525), (-0.88, 0.3525)]
add(ring('gasket_ws', WS, 0.022, lambda p, t: (p[0], 1.45 + p[1] * ca - t * sa, -0.615 + p[1] * sa + t * ca), -0.009, 0.009), 'shell', 'dark', 50)
BACK = [(-0.62, 1.25), (0.62, 1.25), (0.62, 1.68), (-0.62, 1.68)]                   # the back window
add(ring('gasket_back', BACK, 0.02, lambda p, t: (p[0], p[1], t), 0.704, 0.766), 'shell', 'dark', 50)
cups = []
for cx in (-0.657, 0.657):
    for y in (0.931, 0.756):
        lens = [(cx - 0.172, y - 0.07), (cx + 0.172, y - 0.07), (cx + 0.172, y + 0.07), (cx - 0.172, y + 0.07)]
        cups.append(ring('cup', lens, 0.006, lambda p, t: (p[0], p[1], t), -2.656, -2.636))
add(K.join(cups, 'cups'), 'body', 'chrome', 50)
bez = []
for sx in (-1, 1):
    for y0, y1 in ((0.855, 1.105), (0.78, 0.85)):
        lens = [(sx * 0.93 - 0.055, y0), (sx * 0.93 + 0.055, y0), (sx * 0.93 + 0.055, y1), (sx * 0.93 - 0.055, y1)]
        bez.append(ring('bezel', lens, 0.01, lambda p, t: (p[0], p[1], t), 2.64, 2.652))
add(K.join(bez, 'bezels'), 'body', 'chrome', 50)
# the headrests on the bench's back (what shows of the cab through the glass from behind)
heads_ = boxes('headrests', [(x - 0.13, x + 0.13, 1.41, 1.6, 0.53, 0.63) for x in (-0.42, 0.42)])
K.round_edges(heads_, 0.035, 3, 30)
add(heads_, 'shell', 'inner', 40)

log('modelled:', ', '.join(o.name for o, *_ in new))

# ---------- the wheel ----------
# Axis X, the rim's face to +X, centred on the origin, as Truck.ts's wheelGeometry; colours per
# vertex (one draw call for the four).
def wheel_model():
    bm = bmesh.new()
    col = {}
    def tag(faces, c):
        for f in faces: col[f] = c
    def outward(faces, centre):
        """Each face turned to look away from a game point (closed little solids)."""
        c = G(*centre)
        for f in faces:
            if f.normal.dot(f.calc_center_median() - c) < 0: f.normal_flip()
    # the tyre: inner bead over the tread to the outer bead (its faces look out, revolve())
    TYRE = [(-0.098, 0.214), (-0.112, 0.24), (-0.121, 0.285), (-0.119, 0.326), (-0.106, 0.353), (-0.09, 0.3605), (0.0, 0.3615),
            (0.09, 0.3605), (0.106, 0.353), (0.119, 0.326), (0.121, 0.285), (0.112, 0.24), (0.098, 0.214)]
    tag(K.revolve(bm, TYRE, 28)[1].values(), (0.045, 0.045, 0.045))
    # tread: two rows of blocks, staggered, standing 11 mm proud of the tread's base
    NB, R0, R1 = 24, 0.355, 0.3725
    for row, (x0, x1, off) in enumerate([(0.008, 0.088, 0.0), (-0.088, -0.008, 0.5)]):
        for i in range(NB):
            a0 = (i + off) * 2 * math.pi / NB; a1 = a0 + 0.62 * 2 * math.pi / NB
            xo = x1 if row == 0 else x0      # the shoulder end of each block runs down a little
            v = {}
            for xa in (0, 1):
                for ra in (0, 1):
                    for aa in (0, 1):
                        x, r, a = (x0, x1)[xa], (R0, R1)[ra], (a0, a1)[aa]
                        if ra == 1 and x == xo: r = R1 - 0.004
                        v[xa, ra, aa] = bm.verts.new(G(x, r * math.cos(a), r * math.sin(a)))
            q = lambda *k: bm.faces.new([v[x] for x in k])
            fs = [q((0, 1, 0), (1, 1, 0), (1, 1, 1), (0, 1, 1)), q((0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)),
                  q((0, 0, 1), (0, 1, 1), (1, 1, 1), (1, 0, 1)), q((0, 0, 0), (0, 1, 0), (0, 1, 1), (0, 0, 1)), q((1, 0, 0), (1, 0, 1), (1, 1, 1), (1, 1, 0))]
            am, xm = (a0 + a1) / 2, (x0 + x1) / 2
            outward(fs, (xm, 0.35 * math.cos(am), 0.35 * math.sin(am)))
            tag(fs, (0.05, 0.05, 0.048))
    # the steel wheel, from the flange's curl down to the hub (r falls all the way: it looks to +x)
    RIM = [(0.097, 0.224), (0.101, 0.219), (0.094, 0.212), (0.077, 0.193), (0.07, 0.17), (0.068, 0.1475),
           (0.07, 0.125), (0.08, 0.108), (0.088, 0.095), (0.088, 0.05)]
    N = 40
    rings, faces = K.revolve(bm, RIM, N)
    # five slots, each two steps round, between the profile's r 0.17 and 0.125 (segments 4 and 5)
    slots = [faces[i, k] for j in range(5) for i in (j * 8 + 4, j * 8 + 5) for k in (4, 5)]
    rim = [f for f in faces.values() if f not in slots]
    edges = {e for f in slots for e in f.edges if sum(1 for g in e.link_faces if g in slots) == 1}
    bmesh.ops.delete(bm, geom=slots, context='FACES_ONLY')
    # the holes' walls: 5 mm of steel, facing into each hole
    r = bmesh.ops.extrude_edge_only(bm, edges=list(edges))
    bmesh.ops.translate(bm, verts=[v for v in r['geom'] if isinstance(v, bmesh.types.BMVert)], vec=G(-0.005, 0, 0))
    walls = [f for f in r['geom'] if isinstance(f, bmesh.types.BMFace)]
    for f in walls:
        c = K.game(f.calc_center_median()); a = math.atan2(c[2], c[1])
        j = round((a / (2 * math.pi) * N - 5) / 8) % 5            # its slot: centred on step j * 8 + 5
        ac = (j * 8 + 5) * 2 * math.pi / N
        hole = G(c[0], 0.1475 * math.cos(ac), 0.1475 * math.sin(ac))
        if f.normal.dot(hole - f.calc_center_median()) < 0: f.normal_flip()
    tag(rim + walls, (0.6, 0.585, 0.53))
    tag(K.revolve(bm, [(0.088, 0.05), (0.1, 0.048), (0.108, 0.04), (0.113, 0.026), (0.115, 0.0)], 16)[1].values(), (0.7, 0.7, 0.68))
    for k in range(5):        # the nuts, between the slots
        a = (k * 8 + 1) * 2 * math.pi / N
        cy, cz = 0.073 * math.cos(a), 0.073 * math.sin(a)
        hexa = [(cy + 0.0105 * math.cos(b), cz + 0.0105 * math.sin(b)) for b in np.linspace(0, 2 * math.pi, 7)[:-1]]
        lo = [bm.verts.new(G(0.086, y, z)) for y, z in hexa]
        hi = [bm.verts.new(G(0.099, cy + (y - cy) * 0.8, cz + (z - cz) * 0.8)) for y, z in hexa]
        fs = [bm.faces.new((lo[j], lo[(j + 1) % 6], hi[(j + 1) % 6], hi[j])) for j in range(6)] + [bm.faces.new(hi)]
        outward(fs, (0.09, cy, cz))
        tag(fs, (0.42, 0.41, 0.38))
    # behind the slots, the brake drum's face; the back of the wheel closed (looking inboard)
    dark = list(K.revolve(bm, [(0.0, 0.17), (0.055, 0.12), (0.055, 0.0)], 20)[1].values())
    dark += list(K.revolve(bm, [(-0.09, 0.0), (-0.09, 0.214)], 20)[1].values())
    tag(dark, (0.03, 0.03, 0.03))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bm.faces.index_update()
    cols = {f.index: c for f, c in col.items() if f.is_valid}
    o = K.mesh_object('wheel', bm)
    ca = o.data.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER')
    o.data.color_attributes.active_color = ca
    for p in o.data.polygons:
        c = cols.get(p.index, (0.045, 0.045, 0.045))
        for li in p.loop_indices: ca.data[li].color = (*c, 1.0)
    K.smooth(o, 40)
    return o

wheel = wheel_model()
log('wheel:', len(wheel.data.polygons), 'faces')

# ---------- materials ----------
for m in list(bpy.data.materials): m.name = '_game_' + m.name      # (the export's names are ours to give)
def mat(name, rough, metal=0.0, rgb=(1, 1, 1), alpha=1.0):
    m = bpy.data.materials.new(name)
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*rgb, 1); b.inputs['Roughness'].default_value = rough; b.inputs['Metallic'].default_value = metal
    if alpha < 1: b.inputs['Alpha'].default_value = alpha
    m.use_backface_culling = alpha >= 1
    return m
lin = lambda h: tuple((c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4) for c in (((h >> 16) & 255) / 255, ((h >> 8) & 255) / 255, (h & 255) / 255))
# the game's own values (Truck.ts materials()): the game sets them again on what it loads
MATS = {}
for g in ('body', 'shell'):
    MATS[g, 'paint'] = mat(f'{g}_paint', 0.5, 0.05, lin(0xe6e2d8))
    MATS[g, 'chrome'] = mat(f'{g}_chrome', 0.26, 0.62, lin(0xc8cbcc))
    MATS[g, 'dark'] = mat(f'{g}_dark', 0.8, 0.0, lin(0x1a1b1d))
MATS['shell', 'inner'] = mat('shell_inner', 0.7, 0.0, lin(0x5a4c3e))
GLASS = mat('shell_glass', 0.1, 0.45, lin(0x0b0f14), 0.82)
WHEEL = mat('wheel', 0.82)
vc = WHEEL.node_tree.nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
WHEEL.node_tree.links.new(vc.outputs['Color'], WHEEL.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
K.set_materials(wheel, [WHEEL])

# the pieces: kept (their edges rounded, the flat facets of the game's curves smoothed) and new
KIND = {'paint': 'paint', 'chrome': 'chrome', 'trim': 'dark', 'under': 'dark', 'grille': 'dark', 'dark': 'dark', 'inner': 'inner'}
parts = []
for p in kept:
    o = p['o']
    if is_box(o):
        # the boxes' edges: rounded on the paint, a chamfer on the chrome and the trim, none
        # underneath (nobody sees them)
        K.tidy(o)
        d = sorted(K.bounds(o)[i + 1] - K.bounds(o)[i] for i in (0, 2, 4))
        if d[0] > 0.008 and p['mat'] != 'under': K.round_edges(o, min(0.008, d[0] * 0.3), 2 if p['mat'] == 'paint' and d[0] > 0.02 else 1, 30)
        K.smooth(o, 50)
    else:
        K.smooth(o, 61)
    # (what shows of the cab through the glass: the bench's back, the wheel, the dash's top)
    parts.append((o, p['g'], 'inner' if p['g'] == 'shell' and p['mat'] == 'under' else KIND[p['mat']]))
    # the beacon's base up onto the crowned roof
    if p['g'] == 'shell' and p['mat'] == 'trim' and K.bounds(o)[2] > 1.8:
        for v in o.data.vertices: v.co.z += 0.026
for o, g, kind, angle in new:
    K.smooth(o, angle)
    parts.append((o, g, KIND[kind]))
# the glasshouse tapered: above the belt (1.12) the cab leans in, 6 cm a side at the roof
# (doors' frames, glass, rubber, roof, drip rails, the back above the belt)
def taper(o):
    mw = o.matrix_world; inv = mw.inverted()
    for v in o.data.vertices:
        w = mw @ v.co
        if w.z > 1.12:
            w.x *= 1 - 0.063 * min((w.z - 1.12) / 0.68, 1.0)
            v.co = inv @ w
for o, g, kind in parts:
    if g == 'shell' and kind != 'inner': taper(o)
for o in glass: taper(o)
for o, g, kind in parts:
    K.set_materials(o, [MATS[g, kind]])
for o in glass:
    K.weld(o); K.smooth(o, 30, False); K.set_materials(o, [GLASS])
truck = K.join([o for o, *_ in parts], 'saro_truck')
K.flat_normals(truck)
glass = K.join(glass, 'glass') if len(glass) > 1 else glass[0]
for o in (truck, glass):     # (the game's dust came along in the colours: the mask has it now)
    for ca in list(o.data.color_attributes): o.data.color_attributes.remove(ca)
# what shows of the cab through the glass goes in with the dark trim (one draw call less), told
# apart by its vertex colour: the dark material takes the vinyl's colour, the trim's corners its
# own share of it (the game: blenderModels.ts truckBaked, 'dark')
VINYL, TRIM = lin(0x5a4c3e), lin(0x1a1b1d)
RATIO = tuple(t / v for t, v in zip(TRIM, VINYL))
me = truck.data
ca = me.color_attributes.new('Col', 'FLOAT_COLOR', 'CORNER'); me.color_attributes.active_color = ca
names = [m.name for m in me.materials]
dark_i = {g: names.index(f'{g}_dark') for g in ('body', 'shell')}
for p in me.polygons:
    n = names[p.material_index]
    c = (1.0, 1.0, 1.0) if n == 'shell_inner' else RATIO if n.endswith('_dark') else (1.0, 1.0, 1.0)
    if n == 'shell_inner': p.material_index = dark_i['shell']
    for li in p.loop_indices: ca.data[li].color = (*c, 1.0)
log('truck:', len(truck.data.polygons), 'faces,', len(truck.data.vertices), 'vertices')

# ---------- UVs ----------
K.tile_uv(truck, 1.1)
def shrink(c, n):
    if n[1] < -0.7 or (c[1] < 0.5 and abs(c[0]) < 0.75 and abs(c[2]) < 2.5): return 0.3      # underneath
    if abs(c[0]) < 0.87 and 0.8 < c[1] < 1.75 and -0.84 < c[2] < 0.7: return 0.4               # inside the cab
    return 1.0
K.lightmap_uv(truck, shrink=shrink)
K.tile_uv(wheel, 1.1)
K.lightmap_uv(wheel)
log('UVs laid')

# ---------- the bake ----------
bm = bmesh.new()
for v in [(-30, -30), (30, -30), (30, 30), (-30, 30)]: bm.verts.new(G(v[0], 0.0, v[1]))
bm.faces.new(bm.verts); ground = K.mesh_object('ground', bm)
# the four wheels where the game puts them (Truck.ts WHEELS: track 0.84, axles -1.8 and 1.5)
copies = []
for i, (x, z) in enumerate([(-0.84, -1.8), (0.84, -1.8), (-0.84, 1.5), (0.84, 1.5)]):
    c = wheel.copy(); c.data = wheel.data; bpy.context.scene.collection.objects.link(c)
    c.location = G(x, 0.37, z); c.rotation_euler = (0, 0, math.pi if x < 0 else 0)
    copies.append(c)
glass.hide_render = True
def geom(out):
    return lambda nodes, links: nodes.new('ShaderNodeNewGeometry').outputs[out]
def masks(big, small, edge_r, local=False):
    def build(nodes, links):
        a = nodes.new('ShaderNodeAmbientOcclusion'); a.samples = 16; a.inputs['Distance'].default_value = big; a.only_local = local
        b = nodes.new('ShaderNodeAmbientOcclusion'); b.samples = 16; b.inputs['Distance'].default_value = small; b.only_local = local
        bev = nodes.new('ShaderNodeBevel'); bev.samples = 16; bev.inputs['Radius'].default_value = edge_r
        geo = nodes.new('ShaderNodeNewGeometry')
        dot = nodes.new('ShaderNodeVectorMath'); dot.operation = 'DOT_PRODUCT'
        links.new(bev.outputs['Normal'], dot.inputs[0]); links.new(geo.outputs['Normal'], dot.inputs[1])
        edge = nodes.new('ShaderNodeMath'); edge.operation = 'SUBTRACT'; edge.inputs[0].default_value = 1.0
        links.new(dot.outputs['Value'], edge.inputs[1])
        comb = nodes.new('ShaderNodeCombineColor')
        links.new(a.outputs['AO'], comb.inputs['Red']); links.new(b.outputs['AO'], comb.inputs['Green']); links.new(edge.outputs['Value'], comb.inputs['Blue'])
        return comb.outputs['Color']
    return build
P = K.image('pos', SIZE, True); N = K.image('nrm', SIZE, True); A = K.image('ao', SIZE, True)
K.bake_emit(truck, P, geom('Position'), samples=1)
K.bake_emit(truck, N, geom('True Normal'), samples=1)
K.bake_emit(truck, A, masks(0.7, 0.06, 0.008), samples=256)
K.baked_ok(P, 'position'); K.baked_ok(A, 'occlusion')
log('truck baked')
# its shade on the ground: a quad under it (3.2 by 7 m, 1.2 cm up), the occlusion the truck and
# its wheels throw on it, fading out before its edges (SHADE: the game multiplies the ground by it)
SW, SL = 1.6, 3.5
bm = bmesh.new()
vs = [bm.verts.new(G(x, 0.012, z)) for x, z in [(-SW, SL), (SW, SL), (SW, -SL), (-SW, -SL)]]
bm.faces.new(vs); bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
shadow = K.mesh_object('shadow', bm)
SHADOW = mat('shadow', 1.0); K.set_materials(shadow, [SHADOW])
if K.game(shadow.data.polygons[0].normal)[1] < 0: shadow.data.flip_normals()
suv = shadow.data.uv_layers.new(name='UVMap')
for p in shadow.data.polygons:
    for li in p.loop_indices:
        x, _, z = K.game(shadow.data.vertices[shadow.data.loops[li].vertex_index].co)
        suv.data[li].uv = ((x + SW) / (2 * SW), (z + SL) / (2 * SL))
ground.hide_render = True
def shade(nodes, links):
    a = nodes.new('ShaderNodeAmbientOcclusion'); a.samples = 32; a.inputs['Distance'].default_value = 1.1
    return a.outputs['AO']
SP = bpy.data.images.new('shade_pos', 256, 512, alpha=True, float_buffer=True); SP.colorspace_settings.name = 'Non-Color'
SA = bpy.data.images.new('shade', 256, 512, alpha=True, float_buffer=True); SA.colorspace_settings.name = 'Non-Color'
K.bake_emit(shadow, SP, geom('Position'), samples=1, uv='UVMap')
K.bake_emit(shadow, SA, shade, samples=256, uv='UVMap')
ground.hide_render = False
sp, sa = K.pixels(SP), K.pixels(SA)[..., 0]
sx, sz = sp[..., 0], -sp[..., 1]
edge = K.smoothstep(0.0, 0.45, np.minimum(SW - np.abs(sx), SL - np.abs(sz)))
SHADE = np.clip(1 - (1 - np.clip(sa, 0, 1)) * 1.15 * edge, 0.2, 1)
spng = os.path.join(WORK, 'saro_truck_shadow.png')
K.save_mask(np.stack([SHADE] * 3, -1), spng)
log(f'its shade baked: darkest {SHADE.min():.2f} under it')
for c in copies: bpy.data.objects.remove(c)
WP = K.image('wpos', WHEEL_SIZE, True); WN = K.image('wnrm', WHEEL_SIZE, True); WA = K.image('wao', WHEEL_SIZE, True)
K.bake_emit(wheel, WP, geom('Position'), samples=1)
K.bake_emit(wheel, WN, geom('True Normal'), samples=1)
K.bake_emit(wheel, WA, masks(0.12, 0.025, 0.004, True), samples=256)
K.baked_ok(WP, 'wheel position'); K.baked_ok(WA, 'wheel occlusion')
log('wheel baked')

# ---------- the masks ----------
def fields(Pi, Ni):
    p, nr = K.pixels(Pi), K.pixels(Ni)
    return p[..., 0], p[..., 2], -p[..., 1], nr[..., 0], nr[..., 2], -nr[..., 1]
gx, gy, gz, nx, ny, nz = fields(P, N)
ao = K.pixels(A)
vert, up, down = np.abs(ny) < 0.45, ny > 0.7, ny < -0.6
along = np.where(np.abs(nx) > np.abs(nz), gz, gx)
occl = 0.3 + 0.7 * np.clip(ao[..., 0], 0, 1) ** 1.1
cav = 1 - np.clip(ao[..., 1], 0, 1)
noise = lambda s, o=3, seed=0, k=(1, 1, 1): K.fbm(gx * s * k[0], gy * s * k[1], gz * s * k[2], o, seed)
# the road's dust up the sides, thickest low down and sprayed back from each wheel; on the back
low = vert * K.smoothstep(0.9, 0.4, gy) * (0.4 + 0.6 * noise(3.0, 3, 5, (0.6, 2.2, 0.6)))
spray = 0
for zw in (-1.8, 1.5):
    dz = gz - zw
    spray = spray + vert * np.exp(-((dz - 0.62) / 0.42) ** 2) * K.smoothstep(1.08, 0.5, gy) * (dz > 0.25) * (0.5 + 0.5 * noise(6.0, 3, 7))
back = (nz > 0.5) * (gz > 2.4) * K.smoothstep(1.25, 0.45, gy) * (0.55 + 0.45 * noise(4.0, 3, 9))
# flat tops: dust, the bed floor most (a work truck's)
bed = up & (np.abs(gx) < 0.89) & (gy > 0.69) & (gy < 0.79) & (gz > 0.8) & (gz < 2.58)
top = up * (0.12 + 0.3 * noise(1.6, 4, 13)) + bed * (0.35 + 0.3 * K.smoothstep(0.45, 0.7, K.fbm(gx * 2, 0, gz * 0.7, 3, 17)))
# wash streaks down the sides under the rails, the belt and the tailgate's top
sn = K.fbm(along * 3.2, gy * 0.22, gx * 0.1 + gz * 0.05, 3, 23)
streak = vert * (gy > 0.55) * (gy < 1.22) * K.smoothstep(0.5, 0.74, sn) * 0.45
blotch = (noise(0.8, 4, 29) - 0.5) * 0.25
dirt = low * 0.75 + spray * 0.65 + back * 0.55 + top + streak + cav * 0.55 + down * 0.5 + blotch
dirt = np.clip(dirt, 0, 1)
edge = np.clip(ao[..., 2] * 8, 0, 1)
chips = K.smoothstep(0.5, 0.72, K.fbm(gx * 11, gy * 11, gz * 11, 3, 61))
# where hands, boots and loads wear it: the rails' caps, the tailgate's top, the step, the doors' bottoms
handled = ((gy > 1.17) & (gz > 0.8)) | ((gz > 2.5) & (gy > 1.12)) | ((gz > 2.55) & (gy > 0.5) & (gy < 0.6)) | ((gy < 0.6) & (gz > -0.86) & (gz < 0.76))
scratches = bed * K.smoothstep(0.62, 0.8, K.fbm(gx * 60, gy, gz * 1.2, 2, 71)) * 0.7
wear = np.clip(edge * (0.2 + 0.8 * chips) * np.where(handled, 1.0, 0.55) + scratches, 0, 1)
mask = np.stack([occl, dirt, wear], -1)
os.makedirs(WORK, exist_ok=True)
png = os.path.join(WORK, 'saro_truck_mask.png')
K.save_mask(mask, png)
gx, gy, gz, nx, ny, nz = fields(WP, WN)
wao = K.pixels(WA)
r = np.hypot(gy, gz)
tyre = r > 0.212
wocc = 0.28 + 0.72 * np.clip(wao[..., 0], 0, 1) ** 1.2
wcav = 1 - np.clip(wao[..., 1], 0, 1)
side_wall = tyre & (np.abs(nx) > 0.5)
wdirt = np.clip(side_wall * (0.3 + 0.25 * K.fbm(gx * 4, gy * 9, gz * 9, 3, 81)) + tyre * (r > 0.355) * 0.25 + (~tyre) * (0.18 + 0.3 * K.smoothstep(0.17, 0.215, r)) + wcav * 0.7, 0, 1)
wwear = np.clip(np.clip(wao[..., 2] * 8, 0, 1) * (~tyre) * K.smoothstep(0.45, 0.7, K.fbm(gx * 30, gy * 30, gz * 30, 2, 91)), 0, 1)
wpng = os.path.join(WORK, 'saro_truck_wheel_mask.png')
K.save_mask(np.stack([wocc, wdirt, wwear], -1), wpng)
log('masks written', os.path.getsize(png) // 1024, 'and', os.path.getsize(wpng) // 1024, 'KB')

# ---------- out ----------
for o in [ground] + ctx: bpy.data.objects.remove(o)
shadow.data.name = 'shadow'
glass.hide_render = False
bpy.ops.object.select_all(action='DESELECT'); truck.select_set(True); bpy.context.view_layer.objects.active = truck
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.separate(type='MATERIAL'); bpy.ops.object.mode_set(mode='OBJECT')
out = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('saro_truck')]
for o in out:
    o.name = o.data.name = o.active_material.name
    K.set_materials(o, [o.active_material])
    if not o.name.endswith('_dark'):         # (the colours are the dark trim's only)
        for c in list(o.data.color_attributes): o.data.color_attributes.remove(c)
glass.name = glass.data.name = 'shell_glass'
wheel.data.name = 'wheel'
# only the paint tiles a picture (the game's paintTex on the first set): the others keep just
# their lightmap, which is then their first set (build.py points their masks at it)
for o in out + [glass, wheel]:
    if not o.name.endswith('_paint') and 'UVMap' in o.data.uv_layers: o.data.uv_layers.remove(o.data.uv_layers['UVMap'])
out.append(shadow)
# the masks in the .blend, on the second UV set, for a look in Blender
for img_path, ms in ((png, [m for m in MATS.values()]), (wpng, [WHEEL])):
    im = bpy.data.images.load(img_path); im.colorspace_settings.name = 'Non-Color'
    for m in ms:
        nt = m.node_tree
        tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = im
        uv = nt.nodes.new('ShaderNodeUVMap'); uv.uv_map = 'Lightmap'
        nt.links.new(uv.outputs['UV'], tex.inputs['Vector'])
bpy.ops.object.select_all(action='DESELECT')
for o in out + [glass, wheel]: o.select_set(True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format='GLB', use_selection=True, export_yup=True, export_apply=True,
                          export_texcoords=True, export_normals=True, export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                          export_materials='EXPORT', export_image_format='NONE')
log('glb', os.path.getsize(OUT) // 1024, 'KB without the masks (build.py puts them in)')
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(WORK, 'saro_truck.blend'), compress=True)
log('done')
