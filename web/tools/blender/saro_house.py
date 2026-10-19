# SARO's house modelled in Blender from the game's own boxes (tools/blender/build.py saro).
# In: the house as the game builds it (export.py: house:room, house:annex, house:roof, house:wing,
# and `ctx`, what stands round it). Out: src/assets/models/saro_house.glb, four meshes in the
# site's metres that stand exactly where the boxes stood:
#   house      the control room and the records annex (walls, roofs, plinths), with a parapet
#              and its coping on both roofs, a precast sill under the long window, a hood over
#              the east door, collector heads on the five downpipes (Grounds.ts)
#   wing       the east wing and its plinth, its sixteen windows let into the wall
#   glass_dark / glass_lit   the wing's panes, lit as the game's facade picture has them
# All edges that stand out are rounded (2 cm), with hardened normals. Two UV sets: the first
# tiles the game's concrete (art/concrete) every 2.5 m, the second is a lightmap where the
# graphics card bakes one mask: R occlusion, G dirt (splash, streaks, the dust on what is flat,
# form joints and tie holes on the house, block coursing on the wing), B worn edges. The game
# reads the mask as the aoMap (world/blenderModels.ts).
# Usage: blender -b --python tools/blender/saro_house.py -- EXPORT_DIR OUT_GLB WORK_DIR [SIZE]
import bpy, bmesh, json, math, os, sys, time
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bakekit as K
from bakekit import G

argv = sys.argv[sys.argv.index('--') + 1:]
EXP, OUT, WORK = argv[0], argv[1], argv[2]
SIZE = int(argv[3]) if len(argv) > 3 else 2048
t0 = time.time()
def log(*a): print(f'[saro {time.time() - t0:5.1f}s]', *a, flush=True)

K.reset()
log('devices:', K.gpu())
facts = json.load(open(os.path.join(EXP, 'saro_game.json')))
bpy.ops.import_scene.gltf(filepath=os.path.join(EXP, 'saro_game.glb'))
parts, ctx = {}, []
for o in list(bpy.context.scene.objects):
    if o.type != 'MESH': continue
    name = o.data.materials[0].name if o.data.materials else ''
    if name.startswith('house:'): parts[name.split('.')[0]] = o
    else: ctx.append(o)
boxes = {k: K.boxes_of(o) for k, o in parts.items()}
for o in parts.values(): bpy.data.objects.remove(o)

# ---------- what the game built (checked: the model only fits these numbers) ----------
def find(lst, want, tol=0.012):
    for b in lst:
        if all(abs(b[i] - want[i]) < tol for i in range(6)): return b
    raise SystemExit(f'the game has changed: no box {want} among {lst}')
ROOF_CR = find(boxes['house:roof'], (-6.3, 6.3, 3.1, 3.6, -4.8, 4.8))
PLINTH_CR = find(boxes['house:roof'], (-6.7, 6.7, -0.61, -0.01, -5.9, 5.9))
PLINTH_WING = find(boxes['house:roof'], (20.2, 39.0, -0.61, -0.01, -5.9, 5.9))
WING = find(boxes['house:wing'], (20.6, 38.6, -0.6, 3.6, -5.5, 5.5))
PLINTH_AX = find(boxes['house:annex'], (-6.6, 2.6, -0.61, -0.01, 5.8, 11.1))
ROOF_AX = find(boxes['house:annex'], (-6.3, 2.3, 2.8, 3.2, 4.8, 10.8))
walls = [b for b in boxes['house:room']] + [b for b in boxes['house:annex'] if b not in (PLINTH_AX, ROOF_AX)]
assert len(walls) == 17, f'{len(walls)} wall boxes (11 of the control room, 6 of the annex)'
log(f'{len(walls)} walls, {len(ctx)} meshes round the house')
# into the plinth, so the walls stand on it (the game leaves 1 cm)
walls = [(x0, x1, -0.05 if y0 == 0 else y0, y1, z0, z1) for (x0, x1, y0, y1, z0, z1) in walls]

# ---------- the house: control room and annex ----------
PAR_CR, PAR_AX = 3.84, 3.4      # the parapets' tops (the roofs' decks stay at 3.6 and 3.2)
add = [
    # the control room's parapet, on the outer 0.25 m of its walls
    (-6.3, 6.3, 3.55, PAR_CR, -4.8, -4.55), (-6.3, 6.3, 3.55, PAR_CR, 4.55, 4.8),
    (-6.3, -6.05, 3.55, PAR_CR, -4.8, 4.8), (6.05, 6.3, 3.55, PAR_CR, -4.8, 4.8),
    # the annex's on its west, south and east sides (its north side is the control room's wall)
    (-6.3, -6.08, 3.15, PAR_AX, 4.8, 10.8), (-6.3, 2.3, 3.15, PAR_AX, 10.58, 10.8), (2.08, 2.3, 3.15, PAR_AX, 4.8, 10.8),
    # a precast sill under the long window, standing 6 cm out of the wall
    (-5.52, 5.52, 0.885, 0.958, -4.86, -4.6),
    # a concrete hood over the east door (its opening z 1.1 to 2.1, the leaf swings out under it)
    (6.25, 6.92, 2.33, 2.43, 0.92, 2.28),
]
house = K.union_boxes('house', walls + [ROOF_CR, ROOF_AX, PLINTH_CR, PLINTH_AX] + add)
K.bevel(house, 0.02, 2)
log('house solid:', len(house.data.polygons), 'faces')

def under(o):
    """The undersides in the ground (the plinths'): never seen, no texels for them."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    gone = [f for f in bm.faces if K.game(f.normal)[1] < -0.9 and K.game(f.calc_center_median())[1] < -0.5]
    bmesh.ops.delete(bm, geom=gone, context='FACES')
    bm.to_mesh(o.data); bm.free()
under(house)

# the inside: the rooms' painted walls, floors and ceilings stand 2 mm in front of these faces
# (ControlRoom.shell, Annex.shell), so they go
ROOMS = [(-6.0, 6.0, -0.05, 3.1, -4.5, 4.5), (-6.0, 2.0, -0.05, 2.8, 4.8, 10.5)]
def inside(p, r, tol=0.004):
    return r[0] - tol <= p[0] <= r[1] + tol and r[2] - tol <= p[1] <= r[3] + tol and r[4] - tol <= p[2] <= r[5] + tol
bm = bmesh.new(); bm.from_mesh(house.data)
gone = [f for f in bm.faces for c, n in [(K.game(f.calc_center_median()), K.game(f.normal))]
        if any(inside(c, r) and inside([c[k] + n[k] * 0.05 for k in range(3)], r, 0) for r in ROOMS)]
bmesh.ops.delete(bm, geom=gone, context='FACES')
bm.to_mesh(house.data); bm.free()
log('inside faces removed:', len(gone))

# metal: the coping on both parapets (3.5 cm over each side, a drip down the outside), and a
# collector head on each downpipe (Grounds.things: 0.09 m square, their tops at 3.55)
def coping(x0, x1, z0, z1, y, sides):
    o, w, h, f = 0.035, 0.32, 0.045, 0.1
    out = []
    for s in sides:
        if s == 'n': out += [(x0 - o, x1 + o, y, y + h, z0 - o, z0 - o + w), (x0 - o, x1 + o, y + h - f, y + h, z0 - o, z0 - o + 0.022)]
        if s == 's': out += [(x0 - o, x1 + o, y, y + h, z1 + o - w, z1 + o), (x0 - o, x1 + o, y + h - f, y + h, z1 + o - 0.022, z1 + o)]
        if s == 'w': out += [(x0 - o, x0 - o + w, y, y + h, z0 - (o if 'n' in sides else 0), z1 + (o if 's' in sides else 0)),
                             (x0 - o, x0 - o + 0.022, y + h - f, y + h, z0 - (o if 'n' in sides else 0), z1 + (o if 's' in sides else 0))]
        if s == 'e': out += [(x1 + o - w, x1 + o, y, y + h, z0 - (o if 'n' in sides else 0), z1 + (o if 's' in sides else 0)),
                             (x1 + o - 0.022, x1 + o, y + h - f, y + h, z0 - (o if 'n' in sides else 0), z1 + (o if 's' in sides else 0))]
    return out
cope = K.union_boxes('coping', coping(-6.3, 6.3, -4.8, 4.8, PAR_CR, 'nswe') + coping(-6.3, 2.3, 4.8, 10.8, PAR_AX, 'swe'))
K.bevel(cope, 0.005, 1)
PIPES = [(-6.36, -4.86, 3.42, 3.7), (6.36, -4.86, 3.42, 3.7), (-6.36, 4.6, 3.42, 3.7), (-6.36, 10.86, 3.05, 3.6), (2.36, 10.86, 3.05, 3.6)]
heads = K.union_boxes('heads', [(x - 0.13, x + 0.13, y0, y1, z - 0.12, z + 0.12) for x, z, y0, y1 in PIPES]
                      + [(x - 0.07, x + 0.07, y0 - 0.12, y0, z - 0.07, z + 0.07) for x, z, y0, y1 in PIPES])
K.bevel(heads, 0.008, 1)

# ---------- the wing ----------
wing = K.union_boxes('wing', [WING, PLINTH_WING])
# its windows (the facade picture's, core/textures facadeWindows; the dressing's sills and frames
# stand round them, dressSaro.ts): let 12 cm into the wall
WIN = [(20.6 + 2.25 * i + 0.4, 20.6 + 2.25 * i + 1.85) for i in range(8)]
WY, DEPTH = (0.58, 2.34), 0.12
cuts = [K.box_object(f'cut{i}{s}', (a0, a1, WY[0], WY[1], *( (5.5 - DEPTH, 5.7) if s == 's' else (-5.7, -5.5 + DEPTH))))
        for i, (a0, a1) in enumerate(WIN) for s in 'sn']
K.boolean(wing, cuts, 'DIFFERENCE')
K.tidy(wing)
K.bevel(wing, 0.02, 2)
under(wing)
log('wing:', len(wing.data.polygons), 'faces')

# ---------- materials and colours ----------
def mat(name, rough, metal=0.0, rgb=(1, 1, 1)):
    m = bpy.data.materials.new(name)
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*rgb, 1); b.inputs['Roughness'].default_value = rough; b.inputs['Metallic'].default_value = metal
    vc = m.node_tree.nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
    m.node_tree.links.new(vc.outputs['Color'], b.inputs['Base Color'])
    m.use_backface_culling = True
    return m
MATS = [mat('house', 0.95), mat('wing', 0.92), mat('glass_dark', 0.12, 0.1), mat('glass_lit', 0.2)]
# the vertex colours multiply the concrete picture (as M.concrete's 0xd1c8b8 does); the metal is
# lighter than it looks so it comes out at the poles' and the coping's grey under that picture
CONCRETE, PLINTH, DECK, COPE, HEAD = 0xd1c8b8, 0xd1c8b8, 0x6e6860, 0xc4c6c2, 0x8f8f8a
WALL_WING, DECK_WING, GLASS_DARK, GLASS_LIT = 0xc6bcaa, 0x5f5a54, 0x2a2e33, 0xffe0b0
for o in (house, cope, heads, wing): K.set_materials(o, MATS)
def house_paint(c, n, p):
    if c[1] < -0.005: return 0, PLINTH
    if n[1] > 0.7 and ((c[1] > 3.55 and abs(c[0]) < 6.06 and abs(c[2]) < 4.56) or (2.9 < c[1] < 3.25 and c[2] > 4.79)): return 0, DECK
    return 0, CONCRETE
K.paint(house, house_paint)
K.paint(cope, lambda c, n, p: (0, COPE))
K.paint(heads, lambda c, n, p: (0, HEAD))
lit = {'s': facts['litSouth'], 'n': facts['litNorth']}
def wing_paint(c, n, p):
    if c[1] < -0.005: return 1, PLINTH
    if n[1] > 0.7 and c[1] > 3.55: return 1, DECK_WING
    if abs(abs(c[2]) - (5.5 - DEPTH)) < 0.004 and abs(n[2]) > 0.9 and WY[0] < c[1] < WY[1]:
        for i, (a0, a1) in enumerate(WIN):
            if a0 < c[0] < a1:
                on = lit['s' if c[2] > 0 else 'n'][i]
                return (3, GLASS_LIT) if on else (2, GLASS_DARK)
    return 1, WALL_WING
K.paint(wing, wing_paint)
model = K.join([house, cope, heads, wing], 'saro_house')
K.tile_uv(model, 2.5)
# the roofs and the decks get a third of the texels: nobody walks up there
K.lightmap_uv(model, shrink=lambda c, n: 0.33 if n[1] > 0.7 and c[1] > 2.9 else 1.0)
log('model:', len(model.data.polygons), 'faces,', len(model.data.vertices), 'vertices')

# ---------- the bake ----------
# the ground the house stands in (saroLand: flat at -0.62 here), under the context from the game
bm = bmesh.new()
for v in [(-80, -60), (110, -60), (110, 70), (-80, 70)]: bm.verts.new(G(v[0], -0.62, v[1]))
bm.faces.new(bm.verts); K.mesh_object('ground', bm)
def geom(out):
    def build(nodes, links):
        return nodes.new('ShaderNodeNewGeometry').outputs[out]
    return build
def masks(nodes, links):
    big = nodes.new('ShaderNodeAmbientOcclusion'); big.samples = 16; big.inputs['Distance'].default_value = 1.6
    small = nodes.new('ShaderNodeAmbientOcclusion'); small.samples = 16; small.inputs['Distance'].default_value = 0.3
    bev = nodes.new('ShaderNodeBevel'); bev.samples = 16; bev.inputs['Radius'].default_value = 0.035
    geo = nodes.new('ShaderNodeNewGeometry')
    dot = nodes.new('ShaderNodeVectorMath'); dot.operation = 'DOT_PRODUCT'
    links.new(bev.outputs['Normal'], dot.inputs[0]); links.new(geo.outputs['Normal'], dot.inputs[1])
    edge = nodes.new('ShaderNodeMath'); edge.operation = 'SUBTRACT'; edge.inputs[0].default_value = 1.0
    links.new(dot.outputs['Value'], edge.inputs[1])
    comb = nodes.new('ShaderNodeCombineColor')
    links.new(big.outputs['AO'], comb.inputs['Red']); links.new(small.outputs['AO'], comb.inputs['Green']); links.new(edge.outputs['Value'], comb.inputs['Blue'])
    return comb.outputs['Color']
P = K.image('pos', SIZE, True); N = K.image('nrm', SIZE, True); A = K.image('ao', SIZE, True)
K.bake_emit(model, P, geom('Position'), samples=1)
K.bake_emit(model, N, geom('True Normal'), samples=1)
log('position and normal baked')
K.bake_emit(model, A, masks, samples=256)
K.baked_ok(P, 'position'); K.baked_ok(A, 'occlusion')
log('occlusion and edges baked')

# ---------- the mask ----------
p, nr, ao = K.pixels(P), K.pixels(N), K.pixels(A)
gx, gy, gz = p[..., 0], p[..., 2], -p[..., 1]
nx, ny, nz = nr[..., 0], nr[..., 2], -nr[..., 1]
vert = np.abs(ny) < 0.35
up = ny > 0.7
along = np.where(np.abs(nz) > np.abs(nx), gx, gz)          # along the wall
onwing = gx > 20.0
glass = vert & onwing & (np.abs(np.abs(gz) - (5.5 - DEPTH)) < 0.01) & (gy > WY[0]) & (gy < WY[1])
# occlusion kept off black: the floods are fake lamps, a corner is darker, not unlit
occl = 0.3 + 0.7 * np.clip(ao[..., 0], 0, 1) ** 1.1
cav = 1 - np.clip(ao[..., 1], 0, 1)
TX = 0.016 * 2048 / SIZE                                    # about a texel in metres (lightmap_uv), for soft lines
def line(d, w):   # soft line of half width w at d = 0
    return 1 - K.smoothstep(w, w + TX, np.abs(d))
# splash and dirt rising from the ground: on the walls from the plinth they stand on, on the
# plinths' faces only near the land
splash = vert * np.where(gy > -0.005, np.exp(-np.maximum(gy, 0) / 0.3) * 0.8, 0.3 * K.smoothstep(-0.25, -0.62, gy)) \
    * (0.5 + 0.5 * K.fbm(along * 2.5, gy * 7, gx * 0.3, 3, 11))
# rain streaks from the parapets, strongest high up, and from under the long window's sill
sn = K.fbm(along * 2.4, gy * 0.16, (gx + gz) * 0.05, 3, 23)
streak = vert * (gy > 0.02) * K.smoothstep(0.48, 0.74, sn) * (0.25 + 0.75 * K.smoothstep(0.5, 3.4, gy))
under_sill = vert & (nz < -0.9) & (np.abs(gx) < 5.6) & (gy < 0.88) & (gy > 0.02) & ~onwing
streak = streak + under_sill * K.smoothstep(0.42, 0.68, K.fbm(gx * 5, gy * 0.4, 0, 3, 31)) * K.smoothstep(0.0, 0.85, gy) * 1.2
# the wing's sills (the dressing's) drip at their ends
for i, (a0, a1) in enumerate(WIN):
    for e in (a0 - 0.06, a1 + 0.06):
        streak = streak + (vert & onwing & (np.abs(np.abs(gz) - 5.5) < 0.02) & (gy < WY[0])) * line(gx - e, 0.03) * K.smoothstep(0.0, 0.5, gy) * 0.8
dust = up * (0.2 + 0.25 * K.fbm(gx * 0.9, gy * 0.9, gz * 0.9, 3, 41))
blotch = (K.fbm(gx * 0.33, gy * 0.45, gz * 0.33, 4, 53) - 0.5) * 0.36
# the house is cast concrete: form joints every 1.575 m along and the lifts at 1.2 and 2.4,
# tie holes in between; the wing is painted block, 8 by 16 inch courses, half bond
house_wall = vert & ~onwing & (gy > 0.02)
k = np.mod(along + 6.3, 1.575)
joints = house_wall * np.maximum(line(np.minimum(k, 1.575 - k), 0.006), np.maximum(line(gy - 1.2, 0.005), line(gy - 2.4, 0.005)))
tu, tv = np.mod(along + 6.3 + 0.39375, 0.7875) - 0.39375, np.mod(gy - 0.3, 0.6) - 0.3
ties = house_wall * (gy < 3.3) * (1 - K.smoothstep(0.006, 0.006 + TX, np.hypot(tu, tv)))
wing_wall = vert & onwing & (gy > 0.02) & ~glass
course = np.floor(gy / 0.2032)
kb = np.mod(along + np.where(np.mod(course, 2) == 0, 0, 0.2032), 0.4064)
block = wing_wall * np.maximum(line(np.mod(gy + 0.1016, 0.2032) - 0.1016, 0.004), line(np.minimum(kb, 0.4064 - kb), 0.004))
dirt = splash + streak * 0.75 + cav * 0.5 * (vert | (gy > 0.02)) + dust + blotch + joints * 0.5 + ties * 0.5 + block * 0.25
dirt = np.where(glass, 0.0, np.clip(dirt, 0, 1))
edge = np.clip(ao[..., 2] * 7, 0, 1)
chips = K.smoothstep(0.42, 0.68, K.fbm(gx * 8, gy * 8, gz * 8, 3, 61))
wear = np.where(glass | up, 0.0, np.clip(edge * (0.25 + 0.75 * chips), 0, 1))
mask = np.stack([np.where(glass, 1.0, occl), dirt, wear], -1)
os.makedirs(WORK, exist_ok=True)
png = os.path.join(WORK, 'saro_house_mask.png')
K.save_mask(mask, png)
log('mask written', os.path.getsize(png) // 1024, 'KB')

# ---------- out ----------
bpy.ops.object.select_all(action='DESELECT'); model.select_set(True); bpy.context.view_layer.objects.active = model
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.separate(type='MATERIAL'); bpy.ops.object.mode_set(mode='OBJECT')
out = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o.name.startswith('saro_house')]
for o in out:
    o.name = o.data.name = o.active_material.name
    K.set_materials(o, [o.active_material])
# the mask in the .blend, on the second UV set, for a look in Blender
im = bpy.data.images.load(png); im.colorspace_settings.name = 'Non-Color'
for m in MATS[:2]:
    nt = m.node_tree; b = nt.nodes['Principled BSDF']
    tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = im
    uv = nt.nodes.new('ShaderNodeUVMap'); uv.uv_map = 'Lightmap'
    nt.links.new(uv.outputs['UV'], tex.inputs['Vector'])
bpy.ops.object.select_all(action='DESELECT')
for o in out: o.select_set(True)
bpy.ops.export_scene.gltf(filepath=OUT, export_format='GLB', use_selection=True, export_yup=True, export_apply=True,
                          export_texcoords=True, export_normals=True, export_vertex_color='ACTIVE', export_all_vertex_colors=False,
                          export_materials='EXPORT', export_image_format='NONE')
log('glb', os.path.getsize(OUT) // 1024, 'KB without the mask (build.py puts it in)')
for o in list(bpy.context.scene.objects):
    if o not in out: bpy.data.objects.remove(o)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(WORK, 'saro_house.blend'), compress=True)
log('done')
