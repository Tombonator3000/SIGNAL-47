"""Original 1986 cobra-head street lamp family for SIGNAL / 47. Blender 5.2, --background -t 2.

Head (die-cast aluminium housing, prismatic drop refractor, twist-lock photocell, slipfitter),
upswept davit arm, tapered galvanised pole and a short wood-pole arm. Three authored LODs per
mesh, metric FBX and procedural PBR maps. Local contract: Three metres (x, y up, z),
Blender = (x,-z,y), measured UE import = (x,z,y)*100 cm. No downloaded mesh, generated image or
third-party texture is used; real luminaires are only described in words in README.md.
"""
from pathlib import Path
import json, math
import bpy, bmesh, numpy as np
from mathutils import Vector

HERE = Path(__file__).resolve().parent
OUT = HERE / 'Export'; TEX = HERE / 'textures'
OUT.mkdir(parents=True, exist_ok=True); TEX.mkdir(exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
SC = bpy.context.scene; SC.unit_settings.system = 'METRIC'; SC.unit_settings.scale_length = 1
SC.render.threads_mode = 'FIXED'; SC.render.threads = 2
MATS = {}; ROWS = []; TILE = {}; LOD = 0; COL = None; PART = []
PFX = 'M_StreetLamp_'

# ---------------------------------------------------------------- procedural PBR maps
N = 512
def srgb(lin): return np.where(lin < .0031308, lin * 12.92, 1.055 * np.power(lin, 1 / 2.4) - .055)
def fbm(rng, octaves=5, base=4):
    """Tileable value-noise fBm in [-1,1]-ish."""
    out = np.zeros((N, N)); amp = 1.; tot = 0.
    for o in range(octaves):
        k = base * 2 ** o
        g = rng.normal(0, 1, (k + 1, k + 1)); g[-1, :] = g[0, :]; g[:, -1] = g[:, 0]
        t = np.linspace(0, k, N, endpoint=False); i = t.astype(int); f = t - i; f = f * f * (3 - 2 * f)
        a = g[i][:, i]; b = g[i][:, i + 1]; c = g[i + 1][:, i]; d = g[i + 1][:, i + 1]
        fy = f[:, None]; fx = f[None, :]
        out += amp * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy); tot += amp; amp *= .5
    return out / tot
def cells(rng, c):
    """Tileable Voronoi cell values and edge distance (hot-dip galvanising spangle)."""
    m = N // c; jit = rng.random((m, m, 2)) * c; val = rng.random((m, m))
    yy, xx = np.mgrid[:N, :N]; cy = yy // c; cx = xx // c
    best = np.full((N, N), 1e9); second = np.full((N, N), 1e9); out = np.zeros((N, N))
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            ny = (cy + dy) % m; nx = (cx + dx) % m
            py = (cy + dy) * c + jit[ny, nx, 0]; px = (cx + dx) * c + jit[ny, nx, 1]
            d = (yy - py) ** 2 + (xx - px) ** 2
            closer = d < best
            second = np.where(closer, best, np.minimum(second, d))
            out = np.where(closer, val[ny, nx], out); best = np.where(closer, d, best)
    return out, np.sqrt(second) - np.sqrt(best)

def image(name, data, linear=False):
    im = bpy.data.images.new(name, width=data.shape[1], height=data.shape[0], alpha=True)
    if linear: im.colorspace_settings.name = 'Non-Color'
    im.pixels.foreach_set(np.asarray(data, dtype=np.float32).ravel())
    im.filepath_raw = str(TEX / (name + '.png')); im.file_format = 'PNG'; im.save()
    return im

def material(short, col, rough, metal, kind, normal_strength, tile):
    name = PFX + short
    rng = np.random.default_rng(1986 + len(ROWS)); v, u = np.mgrid[:N, :N] / N
    fine = rng.normal(0, 1, (N, N)); col = np.asarray(col, dtype=float)
    tint = np.zeros((N, N, 3)); rough_f = np.zeros((N, N))
    if kind == 'cast':
        # Weathered unpainted die-cast aluminium: chalky oxide bloom, darker road grime, pitting.
        broad = fbm(rng, 5, 6); bloom = np.clip((fbm(rng, 4, 8) - .30) * 2.0, 0, 1); grime = np.clip((-fbm(rng, 4, 10) - .35) * 2.0, 0, 1)
        field = broad * .05 + fine * .02 + bloom * .10 - grime * .12
        rough_f = bloom * .16 + grime * .08 + fine * .02
        height = fbm(rng, 3, 32) * .0006 + fine * .00025
    elif kind == 'lens':
        # Vertical prisms around the bowl (U) and faint horizontal bands (V); aged, slightly yellowed.
        ridges = np.cos(u * math.tau * 24); bands = np.cos(v * math.tau * 10)
        field = ridges * .08 + bands * .03 + fine * .01 + fbm(rng, 3, 2) * .05
        rough_f = fbm(rng, 3, 4) * .05
        height = np.sin(u * math.tau * 24) * .0045 + np.sin(v * math.tau * 10) * .0012
    elif kind == 'galv':
        spang, edge = cells(rng, 10); white = np.clip((fbm(rng, 4, 3) - .30) * 2.0, 0, 1)
        field = (spang - .5) * .16 + fine * .012 + white * .30 + fbm(rng, 4, 2) * .05
        rough_f = white * .22 - (spang - .5) * .10
        height = np.clip(1 - edge / 2.5, 0, 1) * .00035 + fine * .00012
    elif kind == 'hardware':
        rust = np.clip((fbm(rng, 5, 4) - .28) * 2.6, 0, 1)
        field = fine * .02 + fbm(rng, 3, 6) * .06
        tint = rust[:, :, None] * (np.array((.11, .045, .018)) - col)[None, None, :]
        rough_f = rust * .25
        height = rust * .0006 + fine * .0002
    elif kind == 'plastic':
        field = fine * .012 + fbm(rng, 3, 3) * .08; rough_f = fbm(rng, 3, 4) * .06
        height = fine * .00008
    else:  # rubber
        field = fine * .03 + fbm(rng, 3, 4) * .05; rough_f = fine * .02
        height = fine * .00015
    base = np.ones((N, N, 4), dtype=np.float32)
    lin = np.clip((col[None, None, :] + tint) * (1 + field[:, :, None]), .001, .95)
    base[:, :, :3] = srgb(lin)
    ri = np.ones_like(base); ri[:, :, :3] = np.clip(rough + rough_f, .06, .97)[:, :, None]
    dy, dx = np.gradient(height); normal = np.stack((-dx * N, -dy * N, np.ones_like(dx)), axis=2)
    normal /= np.linalg.norm(normal, axis=2)[:, :, None]
    nm = np.ones_like(base); nm[:, :, :3] = normal * .5 + .5; nm[:, :, 1] = 1 - nm[:, :, 1]  # DirectX
    bi = image(name + '_base', base); rr = image(name + '_rough', ri, True); ni = image(name + '_normal', nm, True)
    m = bpy.data.materials.new(name); m.use_nodes = True; nodes = m.node_tree.nodes; links = m.node_tree.links
    p = nodes.get('Principled BSDF'); p.inputs['Metallic'].default_value = metal; p.inputs['Roughness'].default_value = rough
    for im, socket in ((bi, 'Base Color'), (rr, 'Roughness')):
        t = nodes.new('ShaderNodeTexImage'); t.image = im; links.new(t.outputs['Color'], p.inputs[socket])
    t = nodes.new('ShaderNodeTexImage'); t.image = ni
    split = nodes.new('ShaderNodeSeparateColor'); combine = nodes.new('ShaderNodeCombineColor')
    inv = nodes.new('ShaderNodeMath'); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1
    links.new(t.outputs['Color'], split.inputs['Color']); links.new(split.outputs['Red'], combine.inputs['Red'])
    links.new(split.outputs['Blue'], combine.inputs['Blue'])
    links.new(split.outputs['Green'], inv.inputs[1]); links.new(inv.outputs[0], combine.inputs['Green'])
    no = nodes.new('ShaderNodeNormalMap'); no.inputs['Strength'].default_value = normal_strength
    links.new(combine.outputs['Color'], no.inputs['Color']); links.new(no.outputs['Normal'], p.inputs['Normal'])
    MATS[short] = m; TILE[short] = tile
    ROWS.append({'name': name, 'base_color_linear': [1, 1, 1, 1], 'metallic': metal, 'roughness': rough, 'specular': .35,
                 'emission': [0, 0, 0, 1], 'emission_strength': 0, 'normal_convention': 'DirectX',
                 'normal_strength': normal_strength, 'texture_tint_linear': [1, 1, 1], 'uv_tile_m': tile,
                 'textures': {'base_color': 'textures/' + bi.name + '.png', 'roughness': 'textures/' + rr.name + '.png',
                              'normal': 'textures/' + ni.name + '.png'}})
    return m

material('Housing', (.30, .305, .295), .60, .45, 'cast', .18, .5)
material('Lens', (.56, .55, .50), .26, .0, 'lens', .40, .4)
material('Galvanized', (.30, .31, .30), .50, .62, 'galv', .14, 1.)
material('Hardware', (.17, .17, .16), .55, .70, 'hardware', .16, .2)
material('Photocell', (.045, .05, .055), .42, .0, 'plastic', .10, .3)
material('Gasket', (.010, .010, .010), .86, .02, 'rubber', .10, .3)

# ---------------------------------------------------------------- geometry kernel (Three coords in, Blender out)
def B(p): return Vector((p[0], -p[2], p[1]))
def V3(p): return Vector((float(p[0]), float(p[1]), float(p[2])))
def lerp(a, b, t): return a + (b - a) * t
def basis(d):
    d = V3(d).normalized(); h = Vector((0, 1, 0)) if abs(d.y) < .9 else Vector((1, 0, 0))
    u = d.cross(h).normalized(); return u, d.cross(u)
def circle(c, d, r, n, phase=0.):
    u, v = basis(d); c = V3(c)
    return [tuple(c + (u * math.cos(phase + math.tau * k / n) + v * math.sin(phase + math.tau * k / n)) * r) for k in range(n)]

def make_part(name, verts, faces, uvs, mat, sharp=50):
    me = bpy.data.meshes.new(name); bm = bmesh.new()
    bv = [bm.verts.new(B(p)) for p in verts]; uvl = bm.loops.layers.uv.new('UVMap')
    for f, uv in zip(faces, uvs):
        bf = bm.faces.new([bv[i] for i in f])
        for loop, (a, b) in zip(bf.loops, uv): loop[uvl].uv = (a, b)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    for f in bm.faces: f.smooth = True
    for e in bm.edges:
        assert e.is_manifold, name + ' open edge'
        if e.calc_face_angle(0.) > math.radians(sharp): e.smooth = False
    vol = bm.calc_volume(signed=True); assert vol > 0, (name, 'inward or empty', vol)
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); COL.objects.link(o); me.materials.append(MATS[mat]); PART.append(o)
    return o

def newell(ring):
    n = Vector((0, 0, 0))
    for a, b in zip(ring, ring[1:] + ring[:1]):
        n += Vector(((a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x), (a.x - b.x) * (a.y + b.y)))
    return n.normalized() if n.length > 1e-12 else Vector((0, 1, 0))

def loft(name, rings, mat, cap0='flat', cap1='flat', vclosed=False, sharp=50):
    """Rings of equal length (closed loops). Caps: None, 'flat' or ('tip', point). Metric UVs per material tile."""
    tile = TILE[mat]; R = [[V3(p) for p in r] for r in rings]; n = len(R[0]); assert all(len(r) == n for r in R)
    per = [sum((r[(j + 1) % n] - r[j]).length for j in range(n)) for r in R]; mp = sum(per) / len(per)
    U = []
    for r, pp in zip(R, per):
        acc = [0.]
        for j in range(n): acc.append(acc[-1] + (r[(j + 1) % n] - r[j]).length)
        U.append([a / pp * mp / tile for a in acc])
    Vv = [0.]
    for a, b in zip(R, R[1:] + (R[:1] if vclosed else [])):
        Vv.append(Vv[-1] + sum((b[j] - a[j]).length for j in range(n)) / n / tile)
    verts = [p for r in R for p in r]; faces = []; uvs = []
    for i in range(len(R) - 1 + (1 if vclosed else 0)):
        i2 = (i + 1) % len(R)
        for j in range(n):
            faces.append((i * n + j, i * n + (j + 1) % n, i2 * n + (j + 1) % n, i2 * n + j))
            uvs.append(((U[i][j], Vv[i]), (U[i][j + 1], Vv[i]), (U[i2][j + 1], Vv[i + 1]), (U[i2][j], Vv[i + 1])))
    def planar(ring, pts):
        c = sum(ring, Vector()) / len(ring); e1, e2 = basis(newell(ring))
        return [((p - c).dot(e1) / tile, (p - c).dot(e2) / tile) for p in pts]
    for cap, ri in ((cap0, 0), (cap1, len(R) - 1)):
        if cap is None or vclosed: continue
        ring = R[ri]; idx = [ri * n + j for j in range(n)]
        if cap == 'flat':
            faces.append(tuple(idx)); uvs.append(planar(ring, ring))
        else:
            tip = V3(cap[1]); verts.append(tip); t = len(verts) - 1; puv = planar(ring, ring + [tip])
            for j in range(n):
                faces.append((idx[j], idx[(j + 1) % n], t)); uvs.append((puv[j], puv[(j + 1) % n], puv[-1]))
    return make_part(name, verts, faces, uvs, mat, sharp)

def cyl(name, a, b, r, mat, n, r2=None, sharp=50, phase=0.):
    d = V3(b) - V3(a); return loft(name, [circle(a, d, r, n, phase), circle(b, d, r if r2 is None else r2, n, phase)], mat, sharp=sharp)
def hexnut(name, a, b, r, mat): return cyl(name, a, b, r, mat, 6, sharp=50, phase=math.pi / 6)
def box(name, c, s, mat):
    x, y, z = c; sx, sy, sz = s[0] / 2, s[1] / 2, s[2] / 2
    rect = lambda yy: [(x + sx, yy, z + sz), (x + sx, yy, z - sz), (x - sx, yy, z - sz), (x - sx, yy, z + sz)]
    return loft(name, [rect(y - sy), rect(y + sy)], mat)
def rounded_rect_xz(cx, cz, w, d, r, y, seg):
    pts = []
    for ox, oz, start in ((w / 2 - r, d / 2 - r, 0), (-w / 2 + r, d / 2 - r, 90), (-w / 2 + r, -d / 2 + r, 180), (w / 2 - r, -d / 2 + r, 270)):
        for i in range(seg + 1):
            a = math.radians(start + i * 90 / seg); pts.append((cx + ox + r * math.cos(a), y, cz + oz + r * math.sin(a)))
    return pts
def crescent(cx, cz, rin, rout, a0, a1, k, y):
    """Closed thin arc section around a vertical axis (cast cover / curved pole plate)."""
    out = [(cx + rout * math.cos(lerp(a0, a1, i / (k - 1))), y, cz + rout * math.sin(lerp(a0, a1, i / (k - 1)))) for i in range(k)]
    inn = [(cx + rin * math.cos(lerp(a1, a0, i / (k - 1))), y, cz + rin * math.sin(lerp(a1, a0, i / (k - 1)))) for i in range(k)]
    return out + inn

# ---------------------------------------------------------------- SM_StreetLamp_Head
# Pivot: centre of the slipfitter mouth at the rear. Nose toward +X. y up, z to the side.
XR, XN, YT = .05, .78, -.032            # neck end, nose tip, nose tip height
LIP_R, NECK_R, TUBE_R = .060, .055, .030
XC = .42                                # refractor centre along the head
Y_RIM, DEPTH, AX, AZ = -.075, .090, .25, .14   # refractor rim height, drop, plan half axes (0.50 x 0.28)
DOOR_TOP, DOOR_BOT, DOOR_OUT = -.061, -.078, (.262, .142)
PC_X = .20                              # photocell axis
def head_params(x):
    if x <= XR: return NECK_R, NECK_R, -NECK_R, 0., 2., 2., 0.
    s = (x - XR) / (XN - XR)
    q = math.sqrt(max(0., 1 - ((.22 - s) / .22) ** 2)) if s < .22 else 1.
    Wb = .17 * (1 - .42 * max(0., (s - .30) / .70) ** 1.3)
    Tb = .135 - .077 * max(0., (s - .22) / .78) ** 1.35
    W = lerp(NECK_R, Wb, q); T = lerp(NECK_R, Tb, q); Bt = lerp(-NECK_R, -.065, q); Ys = lerp(0., -.030, q)
    nt = lerp(2., 2.4, q); nb = lerp(2., 6., q)
    if s > .8:
        g = math.sqrt(max(0., 1 - ((s - .8) / .2) ** 2))
        W *= g; T = YT + (T - YT) * g; Bt = YT + (Bt - YT) * g; Ys = YT + (Ys - YT) * g
    return W, T, Bt, Ys, nt, nb, q
def head_ring(x, n):
    """Upper cast shell overhangs the lower skirt/door frame: a small step at the shoulder (parting line)."""
    W, T, Bt, Ys, nt, nb, q = head_params(x); pts = []; Wl = W * (.994 - .046 * q)
    for k in list(range(n // 2 + 1)) + list(range(n // 2, n + 1)):
        lower = len(pts) > n // 2
        th = math.tau * k / n; c, s = math.cos(th), (-1. if lower else 1.) * abs(math.sin(th))
        e = 2 / (nb if lower else nt)
        z = (Wl if lower else W) * math.copysign(abs(c) ** e, c)
        y = Ys - (Ys - Bt) * abs(s) ** e if lower else Ys + (T - Ys) * abs(s) ** e
        pts.append((x, y, z))
    return pts
def head_top(x):
    return head_params(x)[1]

def head():
    na, nl = (28, 20, 12)[LOD], (22, 14, 8)[LOD]
    xs = [.012] + [XR + (XN - XR) * (.5 - .5 * math.cos(math.pi * i / nl)) for i in range(nl)]
    loft('Die_cast_aluminium_housing', [head_ring(x, na) for x in xs], 'Housing', cap0='flat', cap1=('tip', (XN, YT, 0)), sharp=60)
    # Slipfitter mouth: cast lip around the 60 mm arm entry, dark rubber seal behind it.
    nlip = (20, 12, 8)[LOD]; ax = (1, 0, 0)
    loft('Slipfitter_cast_lip', [circle((.016, 0, 0), ax, LIP_R, nlip), circle((0, 0, 0), ax, LIP_R, nlip),
         circle((0, 0, 0), ax, TUBE_R + .0012, nlip), circle((.016, 0, 0), ax, TUBE_R + .0012, nlip)], 'Housing', vclosed=True, sharp=60)
    cyl('Slipfitter_seal', (.010, 0, 0), (.016, 0, 0), TUBE_R + .0016, 'Gasket', (16, 10, 8)[LOD])
    # Prismatic drop refractor, superelliptic in section so it reads as a full glass bowl, not a dish.
    nb_, nr = (32, 20, 12)[LOD], (6, 4, 3)[LOD]; p = 2.6; rings = []
    for i in range(nr):
        a = (math.pi / 2) * i / nr; sc = abs(math.cos(a)) ** (2 / p); t = abs(math.sin(a)) ** (2 / p)
        y = Y_RIM - DEPTH * t
        rings.append([(XC + AX * sc * math.cos(math.tau * k / nb_), y, AZ * sc * math.sin(math.tau * k / nb_)) for k in range(nb_)])
    loft('Prismatic_drop_refractor', rings, 'Lens', cap0='flat', cap1=('tip', (XC, Y_RIM - DEPTH, 0)), sharp=60)
    # Cast refractor door frame, slightly proud of the housing skirt.
    nd = (32, 20, 12)[LOD]
    def ell(a, b, y): return [(XC + a * math.cos(math.tau * k / nd), y, b * math.sin(math.tau * k / nd)) for k in range(nd)]
    loft('Refractor_door_frame', [ell(*DOOR_OUT, DOOR_TOP), ell(*DOOR_OUT, DOOR_BOT), ell(AX - .003, AZ - .003, DOOR_BOT),
         ell(AX - .003, AZ - .003, DOOR_TOP)], 'Housing', vclosed=True, sharp=60)
    # NEMA twist-lock photocell on a cast receptacle boss over the ballast compartment.
    npc = (16, 10, 8)[LOD]; top = head_top(PC_X); up = (0, 1, 0)
    cyl('Photocell_receptacle_boss', (PC_X, top - .010, 0), (PC_X, top + .010, 0), .047, 'Housing', npc)
    y0 = top + .010
    if LOD < 2:
        rings = [circle((PC_X, y0, 0), up, .041, npc), circle((PC_X, y0 + .046, 0), up, .041, npc),
                 circle((PC_X, y0 + .054, 0), up, .036, npc)]
        if LOD == 0: rings.append(circle((PC_X, y0 + .059, 0), up, .025, npc))
        loft('Twist_lock_photocell', rings, 'Photocell', cap1=('tip', (PC_X, y0 + .062, 0)), sharp=60)
    else:
        loft('Twist_lock_photocell', [circle((PC_X, y0, 0), up, .041, npc), circle((PC_X, y0 + .050, 0), up, .038, npc)],
             'Photocell', cap1=('tip', (PC_X, y0 + .062, 0)), sharp=60)
    # Door latch at the nose; hinge at the rear end of the door; slipfitter clamp bolts underneath.
    box('Door_spring_latch', (XC + DOOR_OUT[0] + .006, DOOR_BOT + .004, 0), (.030, .016, .046), 'Hardware')
    if LOD < 2:
        box('Door_latch_thumb_tab', (XC + DOOR_OUT[0] + .024, DOOR_BOT + .001, 0), (.012, .006, .030), 'Hardware')
        hx = XC - DOOR_OUT[0] - .004
        cyl('Door_hinge_pin', (hx, DOOR_BOT + .002, -.034), (hx, DOOR_BOT + .002, .034), .0065, 'Hardware', (10, 8, 6)[LOD])
        for zz in (-.022, .022): box('Door_hinge_knuckle', (hx + .008, DOOR_BOT + .004, zz), (.020, .010, .016), 'Hardware')
    if LOD == 0:
        for zz in (-.028, .028):
            hexnut('Slipfitter_clamp_bolt', (.080, -.054, zz), (.080, -.068, zz), .011, 'Hardware')

# ---------------------------------------------------------------- SM_StreetLamp_Arm
# Pivot: pole axis at the arm root (centre of the clamp ring), arm toward +X.
POLE_H, PLATE_T, SHAFT_TOP = 8.0, .025, 7.955
R_BASE, R_TOP = .100, .0552
def pole_r(y): return R_BASE - (R_BASE - R_TOP) * (y - PLATE_T) / (SHAFT_TOP - PLATE_T)
ARM_MOUNT_Y = 7.60
ARM_P = [(.060, 0.), (.50, .24), (.80, .45), (1.15, .45)]; ARM_END = (1.80, .45, 0.)
CLAMP_IN = pole_r(ARM_MOUNT_Y) + .0005
def bez(t):
    P = ARM_P; m = 1 - t
    p = [m ** 3 * P[0][k] + 3 * m * m * t * P[1][k] + 3 * m * t * t * P[2][k] + t ** 3 * P[3][k] for k in (0, 1)]
    d = [3 * m * m * (P[1][k] - P[0][k]) + 6 * m * t * (P[2][k] - P[1][k]) + 3 * t * t * (P[3][k] - P[2][k]) for k in (0, 1)]
    return (p[0], p[1], 0.), (d[0], d[1], 0.)
def arm():
    n, nbz = (12, 8, 6)[LOD], (14, 8, 4)[LOD]
    rings = [circle(bez(i / nbz)[0], bez(i / nbz)[1], TUBE_R, n) for i in range(nbz + 1)]
    rings.append(circle(ARM_END, (1, 0, 0), TUBE_R, n))
    loft('Upswept_davit_tube', rings, 'Galvanized', sharp=60)
    nc = (20, 12, 8)[LOD]; up = (0, 1, 0)
    loft('Pole_clamp_band', [circle((0, .08, 0), up, .073, nc), circle((0, -.08, 0), up, .073, nc),
         circle((0, -.08, 0), up, CLAMP_IN, nc), circle((0, .08, 0), up, CLAMP_IN, nc)], 'Galvanized', vclosed=True, sharp=60)
    if LOD < 2:
        p0, d0 = bez(0.); d0 = V3(d0).normalized()
        cyl('Welded_arm_socket', p0, tuple(V3(p0) + d0 * .075), .037, 'Galvanized', (14, 10, 8)[LOD], r2=.034)
    for zz in (-.013, .013): box('Clamp_bolt_ear', (-.088, 0, zz), (.034, .110, .008), 'Hardware')
    if LOD == 0:
        for yy in (-.035, .035):
            hexnut('Clamp_bolt_head', (-.094, yy, .017), (-.094, yy, .027), .011, 'Hardware')
            cyl('Clamp_bolt_shank', (-.094, yy, -.030), (-.094, yy, .017), .0055, 'Hardware', 8)

# ---------------------------------------------------------------- SM_StreetLamp_Pole
ANCHOR = .13; HANDHOLE_Y = (.38, .56)
def pole():
    n = (20, 12, 8)[LOD]; up = (0, 1, 0); seg = (3, 2, 1)[LOD]
    loft('Square_foot_plate', [rounded_rect_xz(0, 0, .34, .34, .02, 0., seg), rounded_rect_xz(0, 0, .34, .34, .02, PLATE_T, seg)], 'Galvanized', sharp=50)
    ys = [PLATE_T - .002, 4.0, SHAFT_TOP] if LOD < 2 else [PLATE_T - .002, SHAFT_TOP]
    loft('Tapered_round_shaft', [circle((0, y, 0), up, pole_r(y), n) for y in ys], 'Galvanized', sharp=60)
    if LOD < 2:
        loft('Base_fillet_weld', [circle((0, PLATE_T - .002, 0), up, .107, n), circle((0, PLATE_T + .004, 0), up, .106, n),
             circle((0, PLATE_T + .022, 0), up, pole_r(PLATE_T + .022) + .0012, n)], 'Galvanized', sharp=60)
    cr = [circle((0, SHAFT_TOP - .012, 0), up, .060, n), circle((0, SHAFT_TOP + .012, 0), up, .060, n)]
    if LOD < 2: cr.append(circle((0, SHAFT_TOP + .030, 0), up, .050, n))
    if LOD == 0: cr.append(circle((0, SHAFT_TOP + .040, 0), up, .030, n))
    loft('Pole_top_cap', cr, 'Galvanized', cap1=('tip', (0, POLE_H, 0)), sharp=60)
    for sx in (-1, 1):
        for sz in (-1, 1):
            x, z = sx * ANCHOR, sz * ANCHOR
            if LOD == 0: cyl('Anchor_washer', (x, PLATE_T - .001, z), (x, PLATE_T + .004, z), .025, 'Hardware', 12)
            hexnut('Anchor_nut', (x, PLATE_T + .003, z), (x, PLATE_T + .026, z), .022, 'Hardware')
            if LOD < 2: cyl('Anchor_bolt_thread', (x, PLATE_T + .025, z), (x, PLATE_T + .068, z), .0125, 'Hardware', (10, 6)[LOD])
    if LOD < 2:
        # Hand-hole cover facing -X (away from the street), curved to the cone.
        k = (8, 5)[LOD]; a = .42
        loft('Hand_hole_cover', [crescent(0, 0, pole_r(y) - .003, pole_r(y) + .006, math.pi - a, math.pi + a, k, y) for y in HANDHOLE_Y], 'Galvanized', sharp=50)
    if LOD == 0:
        for y in (HANDHOLE_Y[0] + .022, HANDHOLE_Y[1] - .022):
            r = pole_r(y) + .006; cyl('Hand_hole_screw', (-r + .001, y, 0), (-r - .005, y, 0), .0075, 'Hardware', 8)

# ---------------------------------------------------------------- SM_StreetLamp_WoodArm
# Pivot: contact point on the side of a wooden utility pole, at the arm axis; arm toward +X.
WOOD_R = .13; WOOD_REACH = 1.20; WOOD_PLATE = (-.48, .10); BRACE_LOW = -.44; BRACE_HI_X = .72
def woodarm():
    n = (12, 8, 6)[LOD]; ax = (1, 0, 0); k = (9, 5, 3)[LOD]; a = .40
    loft('Curved_pole_plate', [crescent(-WOOD_R, 0, WOOD_R, WOOD_R + .009, -a, a, k, y) for y in WOOD_PLATE], 'Galvanized', sharp=50)
    loft('Straight_mast_arm', [circle((.006, 0, 0), ax, TUBE_R, n), circle((WOOD_REACH, 0, 0), ax, TUBE_R, n)], 'Galvanized', sharp=60)
    if LOD < 2: cyl('Welded_arm_socket', (.006, 0, 0), (.075, 0, 0), .038, 'Galvanized', (14, 10)[LOD], r2=.035)
    lo = (.012, BRACE_LOW, 0); hi = (BRACE_HI_X, -TUBE_R + .004, 0)
    cyl('Tension_brace_rod', lo, hi, .011, 'Galvanized', (8, 6, 4)[LOD], sharp=60)
    if LOD < 2:
        box('Brace_lower_lug', (.020, BRACE_LOW, 0), (.022, .040, .030), 'Galvanized')
        box('Brace_upper_strap', (BRACE_HI_X, -TUBE_R - .002, 0), (.040, .010, .066), 'Galvanized')
    for y in (.070, -.38):
        if LOD < 2:
            box('Through_bolt_square_washer', (.0125, y, 0), (.006, .045, .045), 'Hardware')
            hexnut('Through_bolt_nut', (.015, y, 0), (.032, y, 0), .016, 'Hardware')
            if LOD == 0: cyl('Through_bolt_end', (.031, y, 0), (.052, y, 0), .008, 'Hardware', 8)
        elif y > 0:
            box('Through_bolt_nut', (.020, y, 0), (.020, .040, .040), 'Hardware')

# ---------------------------------------------------------------- assembly, LODs, export, contract
HEAD_EMIT = (XC, Y_RIM - DEPTH - .008, 0.)
BUILDERS = {'SM_StreetLamp_Head': head, 'SM_StreetLamp_Arm': arm, 'SM_StreetLamp_Pole': pole, 'SM_StreetLamp_WoodArm': woodarm}
ORDER = {'SM_StreetLamp_Head': ['Housing', 'Lens', 'Photocell', 'Hardware', 'Gasket']}
BUDGET = {'SM_StreetLamp_Head': 3000, 'SM_StreetLamp_Arm': 800, 'SM_StreetLamp_Pole': 1500, 'SM_StreetLamp_WoodArm': 800}
r3 = lambda v: [round(float(c), 5) for c in v]
DETAIL = {
 'SM_StreetLamp_Head': {
   'pivot': 'centre of the slipfitter mouth (rear face of the cast lip), where the 60 mm arm tube enters; nose toward +X',
   'slipfitter': {'mouth_centre_three_m': [0, 0, 0], 'entry_axis_three': [1, 0, 0], 'tube_diameter_m': .06,
                  'lip_outer_diameter_m': 2 * LIP_R, 'note': 'place the head pivot exactly on the arm end point, same yaw as the arm'},
   'housing_size_m': {'length_incl_lip': XN, 'width': .34, 'height': .20, 'top_three_m_y': .135, 'skirt_three_m_y': -.065},
   'lens_rim_centre_three_m': [XC, Y_RIM, 0], 'lens_volume_centre_three_m': [XC, Y_RIM - DEPTH / 2, 0],
   'lens_centre_three_m': [XC, Y_RIM - DEPTH, 0], 'lens_normal_three': [0, -1, 0], 'aperture_m': [2 * AX, 2 * AZ],
   'lens_depth_m': DEPTH, 'emitter_three_m': list(HEAD_EMIT),
   'lamp_arc_three_m': [XC, -.035, 0],
   'emitter_note': 'lens_centre is the lowest outer point of the glass drop; emitter is 8 mm below it on the normal so a '
                   'shadow-casting light is not occluded by the opaque lens mesh. lamp_arc is the HPS arc tube inside the housing.',
   'suggested_light': {'type': 'spot pointing along lens_normal', 'outer_cone_deg': 70, 'inner_cone_deg': 45,
                       'colour_temperature_K': 2100, 'source': 'high-pressure sodium, 150-250 W class'},
   'photocell_top_three_m': None, 'bottom_three_m_y': Y_RIM - DEPTH},
 'SM_StreetLamp_Arm': {
   'pivot': 'pole axis at the arm root (centre of the clamp band); arm toward +X, rises toward +Y',
   'head_mount_three_m': list(ARM_END), 'head_mount_axis_three': [1, 0, 0], 'reach_m': ARM_END[0], 'rise_m': ARM_END[1],
   'tube_diameter_m': .06, 'clamp_band': {'inner_radius_m': round(CLAMP_IN, 5), 'outer_radius_m': .073, 'height_m': .16, 'centre_y_m': 0},
   'fits_pole_at_three_m_y': ARM_MOUNT_Y},
 'SM_StreetLamp_Pole': {
   'pivot': 'centre of the foot-plate underside; y=0 is the ground / top of the concrete base',
   'height_m': POLE_H, 'shaft_top_three_m_y': SHAFT_TOP, 'foot_plate_top_three_m_y': PLATE_T,
   'foot_plate_m': [.34, PLATE_T, .34], 'shaft_diameter_m': {'base': 2 * R_BASE, 'top': round(2 * R_TOP, 4)},
   'shaft_radius_formula': 'r(y) = %.4f - %.4f*(y-%.3f)/%.3f' % (R_BASE, R_BASE - R_TOP, PLATE_T, SHAFT_TOP - PLATE_T),
   'arm_mount_three_m': [0, ARM_MOUNT_Y, 0], 'radius_at_arm_mount_m': round(pole_r(ARM_MOUNT_Y), 5),
   'anchor_bolts_three_m': [[sx * ANCHOR, PLATE_T, sz * ANCHOR] for sx in (-1, 1) for sz in (-1, 1)],
   'hand_hole': {'facing_three': [-1, 0, 0], 'y_range_m': list(HANDHOLE_Y)}},
 'SM_StreetLamp_WoodArm': {
   'pivot': 'contact point on the side of a wooden utility pole at the arm axis height; arm toward +X',
   'head_mount_three_m': [WOOD_REACH, 0, 0], 'head_mount_axis_three': [1, 0, 0], 'reach_m': WOOD_REACH,
   'assumed_wood_pole_radius_m': WOOD_R, 'wood_pole_axis_three_m': [-WOOD_R, 0, 0],
   'plate_y_range_m': list(WOOD_PLATE), 'through_bolts_three_m_y': [.070, -.38]}}
ASSEMBLY = {'pole_origin': [0, 0, 0], 'arm_origin': [0, ARM_MOUNT_Y, 0],
            'head_pivot': r3((ARM_END[0], ARM_MOUNT_Y + ARM_END[1], 0)),
            'emitter': r3((ARM_END[0] + HEAD_EMIT[0], ARM_MOUNT_Y + ARM_END[1] + HEAD_EMIT[1], 0)),
            'lens_bottom': r3((ARM_END[0] + XC, ARM_MOUNT_Y + ARM_END[1] + Y_RIM - DEPTH, 0)),
            'note': 'Three metres, pole at origin and arm/head unrotated (arm along +X). Rotate arm and head together about the pole axis.'}

def active(o):
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
def reorder_slots(ob, order):
    me = ob.data; names = [m.name for m in me.materials]; want = [PFX + s for s in order]
    assert sorted(names) == sorted(want), (ob.name, names, want)
    idx = [0] * len(me.polygons); me.polygons.foreach_get('material_index', idx)
    remap = [want.index(nm) for nm in names]; idx = [remap[i] for i in idx]
    me.materials.clear()
    for nm in want: me.materials.append(bpy.data.materials[nm])
    me.polygons.foreach_set('material_index', idx); me.update()
def three(v): return (v[0], v[2], -v[1])

CONTRACT = {'status': 'SOURCE_GENERATED_NATIVE_UNVERIFIED', 'task': 'STREETLAMPS-20261108', 'units': 'metres',
 'mapping': 'Three (x,y,z) -> Blender (x,-z,y) -> UE centimetres (x,z,y)*100', 'destination': '/Game/Signal47/Art/StreetLamps',
 'originality': 'Original metric bpy geometry and procedural PBR textures made for SIGNAL / 47. No third-party imagery or meshes.',
 'lens_material': PFX + 'Lens',
 'lens_runtime_parameters': {'EmissiveFactor': 'linear RGB, zero when off, multiplies the lens base texture', 'TextureTint': 'linear RGB, default one'},
 'collision': 'none authored; import keeps auto_generate_collision=False and complex-as-simple like ExteriorFixtures; gameplay uses its own 2D colliders',
 'assembly_three_m': ASSEMBLY, 'parts': {}}
EXPORT = bpy.data.collections.new('DELIVERY_FBX'); SC.collection.children.link(EXPORT)
for name, builder in BUILDERS.items():
    counts = []; data = None; components = []; slots = []
    for LOD in range(3):
        COL = bpy.data.collections.new(name + '_EDIT_LOD' + str(LOD)); SC.collection.children.link(COL); PART = []
        builder(); components.append(len(PART))
        bpy.ops.object.select_all(action='DESELECT')
        for o in PART: o.select_set(True)
        bpy.context.view_layer.objects.active = PART[0]; bpy.ops.object.duplicate(); copies = list(bpy.context.selected_objects)
        bpy.context.view_layer.objects.active = copies[0]; bpy.ops.object.join(); ob = bpy.context.object
        ob.name = name if LOD == 0 else name + '_LOD' + str(LOD); ob.data.name = ob.name
        for c in list(ob.users_collection): c.objects.unlink(ob)
        EXPORT.objects.link(ob); active(ob)
        SC.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        reorder_slots(ob, ORDER.get(name, ['Galvanized', 'Hardware'])); slots.append([m.name for m in ob.data.materials])
        t = ob.modifiers.new('Explicit_delivery_triangles', 'TRIANGULATE'); bpy.ops.object.modifier_apply(modifier=t.name)
        me = ob.data; points = [ob.matrix_world @ v.co for v in me.vertices]
        bounds = [[float(fn(p[k] for p in points)) for k in range(3)] for fn in (min, max)]
        count = len(me.polygons); counts.append(count)
        assert all(len(p.vertices) == 3 and p.area > 1e-12 for p in me.polygons), name + ' degenerate triangle'
        assert all(math.isfinite(f) for p in points for f in p), name + ' nonfinite vertex'
        assert me.uv_layers and me.uv_layers[0].name == 'UVMap', name + ' UV'
        if LOD == 0:
            tb = [[bounds[0][0], bounds[0][2], -bounds[1][1]], [bounds[1][0], bounds[1][2], -bounds[0][1]]]
            data = {'bounds_local_m': [r3(bounds[0]), r3(bounds[1])], 'bounds_three_m': [r3(tb[0]), r3(tb[1])],
                    'materials': [m.name for m in me.materials], 'origin': [0, 0, 0], **DETAIL[name]}
            # Measured checks of the named points against the mesh.
            def verts_of(mat):
                i = [k for k, m in enumerate(me.materials) if m.name == PFX + mat][0]; vs = set()
                for p in me.polygons:
                    if p.material_index == i: vs.update(p.vertices)
                return [three(me.vertices[v].co) for v in vs]
            if name == 'SM_StreetLamp_Head':
                lens = verts_of('Lens'); low = min(lens, key=lambda p: p[1])
                assert abs(low[1] - (Y_RIM - DEPTH)) < 1e-5 and abs(low[0] - XC) < 1e-5, low
                assert abs(max(p[0] for p in lens) - min(p[0] for p in lens) - 2 * AX) < 1e-4
                assert abs(tb[0][0]) < 1e-6 and abs(tb[1][0] - XN) < 1e-5, tb
                assert abs(tb[1][2] - .17) < .003 and abs(tb[1][2] + tb[0][2]) < 1e-5, tb
                pc = verts_of('Photocell'); data['photocell_top_three_m'] = r3(max(pc, key=lambda p: p[1]))
            if name == 'SM_StreetLamp_Arm':
                ends = [three(v.co) for v in me.vertices if abs(three(v.co)[0] - ARM_END[0]) < 1e-5]
                c = [sum(p[k] for p in ends) / len(ends) for k in range(3)]
                assert abs(tb[1][0] - ARM_END[0]) < 1e-5 and max(abs(c[k] - ARM_END[k]) for k in range(3)) < 1e-5, c
                data['measured_end_cap_centre_three_m'] = r3(c)
            if name == 'SM_StreetLamp_Pole':
                assert abs(tb[0][1]) < 1e-6 and abs(tb[1][1] - POLE_H) < 1e-6, tb
            if name == 'SM_StreetLamp_WoodArm':
                assert abs(tb[1][0] - WOOD_REACH) < 1e-5, tb
        bpy.ops.export_scene.fbx(filepath=str(OUT / (ob.name + '.fbx')), use_selection=True, object_types={'MESH'}, apply_unit_scale=True,
            apply_scale_options='FBX_SCALE_UNITS', axis_forward='-Y', axis_up='Z', bake_anim=False, add_leaf_bones=False,
            mesh_smooth_type='FACE', use_tspace=True)
        ob.hide_render = True; ob.hide_set(True)
        COL.hide_viewport = LOD > 0; COL.hide_render = True
    assert counts[0] > counts[1] > counts[2] > 0, (name, counts)
    assert counts[0] < BUDGET[name], (name, counts)
    assert counts[1] < counts[0] * .65 and counts[2] < counts[0] * .25, (name, counts)
    assert all(s == slots[0] for s in slots), (name, slots)
    data['lod_triangles'] = counts; data['editable_components'] = components; CONTRACT['parts'][name] = data
CONTRACT['total_LOD0_triangles'] = sum(r['lod_triangles'][0] for r in CONTRACT['parts'].values())
(HERE / 'materials.json').write_text(json.dumps(ROWS, indent=2) + '\n')
def tidy(v):
    if isinstance(v, float): return round(v, 6) + 0.
    if isinstance(v, dict): return {k: tidy(x) for k, x in v.items()}
    if isinstance(v, (list, tuple)): return [tidy(x) for x in v]
    return v
(HERE / 'model_contract.json').write_text(json.dumps(tidy(CONTRACT), indent=2) + '\n')
SC.world = bpy.data.worlds.new('Neutral_lamp_world'); SC.world.color = (.18, .18, .18)
SC.render.engine = 'CYCLES'; SC.cycles.device = 'CPU'; SC.cycles.samples = 16
SC.view_settings.view_transform = 'AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(HERE / 'SIGNAL47_StreetLamps.blend'))
bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_mainfile()
print('STREETLAMPS SOURCE PASS', json.dumps({k: v['lod_triangles'] for k, v in CONTRACT['parts'].items()}))
