# SARO's house modelled in Blender from the game's own boxes (tools/blender/build.py saro).
# In: the house as the game builds it (export.py: house:room, house:annex, house:roof, house:wing,
# and `ctx`, what stands round it; the site's lamps). Out: src/assets/models/saro_house.glb, five
# meshes in the site's metres that stand exactly where the boxes stood:
#   house      the control room and the records annex (walls, roofs, plinths), with a parapet
#              and its coping (6.5 cm over each side, a drip down the outside) on both roofs, a
#              precast sill under the long window, a hood over the east door thick enough to
#              throw a shadow, collector heads on the five downpipes (Grounds.ts); on the roofs
#              what reads against the stars: a rooftop unit and two stacks on the wing, a cable
#              tray across the control room's roof and its conduit down the north wall, a
#              ladder up the wing's east end with its rails curled over the parapet
#   wing       the east wing and its plinth, its sixteen windows let 22 cm into the wall
#   glass_dark / glass_lit   the wing's panes, lit as the game's facade picture has them; the
#              lit ones show a room through their first UV set (a card of four: the ceiling's
#              light falling off down the back wall, a desk and a terminal, shelves, cabinets, one
#              room lit only by a desk lamp)
#   skirt      the ground round the plinths (1.4 m out), for the shade the house throws on it
# All edges that stand out are rounded (2 cm); every face square to an axis is exactly flat
# (its own normal at each of its corners), the big ones cut into rectangles round their openings.
# Two UV sets: the first tiles the game's concrete (art/concrete) every 2.5 m, the second is a
# lightmap (every island at one texel density, packed by outline) where the graphics card bakes
#   the mask (SIZE px): R occlusion (on the skirt: the multiplier the ground takes), G dirt
#      (splash, streaks, the dust on what is flat), B worn edges; the fine repeating detail (form
#      joints and tie holes, block coursing) is no longer here but tiles on the first set in the
#      game (world/blenderModels.ts)
#   the lamps' light (SIZE px): the wall packs, the fire exit's lamp, the lab's and the yard's
#      lamp posts in Cycles (their cones, the hot spot at the fitting, the spill down the wall
#      onto the plinth, the hood's and the ladder's shadows), three lamps or groups of them to a
#      channel; the game scales each by its own flood's strength now (Ultra's real lights, a lamp
#      switched off), so the story's lamps still work.
# The game reads the mask as the aoMap and the light as its own map (world/blenderModels.ts).
# Usage: blender -b --python tools/blender/saro_house.py -- EXPORT_DIR OUT_GLB WORK_DIR [SIZE]
import bpy, bmesh, json, math, os, sys, time
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bakekit as K
from bakekit import G

argv = sys.argv[sys.argv.index('--') + 1:]
EXP, OUT, WORK = argv[0], argv[1], argv[2]
SIZE = int(argv[3]) if len(argv) > 3 else 1024
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
GROUND = facts['ground']
if abs(GROUND + 0.6) > 0.05: raise SystemExit(f'the game has changed: the ground round the house is at {GROUND}, not -0.6')
log(f'{len(walls)} walls, {len(ctx)} meshes round the house, the ground at {GROUND}')
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
    # a concrete hood over the east door (its opening z 1.1 to 2.1, the leaf swings out under
    # it): 17 cm thick and 0.85 m deep, so the yard's lamp throws its shadow down the door
    (6.25, 7.15, 2.3, 2.47, 0.82, 2.38),
]
house = K.union_boxes('house', walls + [ROOF_CR, ROOF_AX, PLINTH_CR, PLINTH_AX] + add)
K.grid_cut(house)
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

# metal: the coping on both parapets (6.5 cm over each side, a drip down the outside), and a
# collector head on each downpipe (Grounds.things: 0.09 m square, their tops at 3.55)
def coping(x0, x1, z0, z1, y, sides):
    o, w, h, f, t = 0.065, 0.38, 0.05, 0.13, 0.024
    out = []
    zn, zs = z0 - (o if 'n' in sides else 0), z1 + (o if 's' in sides else 0)
    for s in sides:
        if s == 'n': out += [(x0 - o, x1 + o, y, y + h, z0 - o, z0 - o + w), (x0 - o, x1 + o, y + h - f, y + h, z0 - o, z0 - o + t)]
        if s == 's': out += [(x0 - o, x1 + o, y, y + h, z1 + o - w, z1 + o), (x0 - o, x1 + o, y + h - f, y + h, z1 + o - t, z1 + o)]
        if s == 'w': out += [(x0 - o, x0 - o + w, y, y + h, zn, zs), (x0 - o, x0 - o + t, y + h - f, y + h, zn, zs)]
        if s == 'e': out += [(x1 + o - w, x1 + o, y, y + h, zn, zs), (x1 + o - t, x1 + o, y + h - f, y + h, zn, zs)]
    return out
cope = K.union_boxes('coping', coping(-6.3, 6.3, -4.8, 4.8, PAR_CR, 'nswe') + coping(-6.3, 2.3, 4.8, 10.8, PAR_AX, 'swe'))
K.bevel(cope, 0.006, 1)
PIPES = [(-6.36, -4.86, 3.42, 3.7), (6.36, -4.86, 3.42, 3.7), (-6.36, 4.6, 3.42, 3.7), (-6.36, 10.86, 3.05, 3.6), (2.36, 10.86, 3.05, 3.6)]
heads = K.union_boxes('heads', [(x - 0.13, x + 0.13, y0, y1, z - 0.12, z + 0.12) for x, z, y0, y1 in PIPES]
                      + [(x - 0.07, x + 0.07, y0 - 0.12, y0, z - 0.07, z + 0.07) for x, z, y0, y1 in PIPES])
K.bevel(heads, 0.008, 1)

# ---------- on the roofs and the walls: what reads against the stars ----------
# (clear of what the dressing puts up there, dressSaro.ts: the coolers, the vents, the mast)
kit = []
# the wing's rooftop unit on its curb, a fan's hood on it and the duct going down through the deck
kit.append(K.union_boxes('unit', [(27.8, 30.3, 3.58, 3.72, -1.45, 0.45), (27.9, 30.2, 3.7, 4.95, -1.35, 0.35),
                                  (28.35, 29.25, 4.93, 5.12, -0.95, -0.05), (29.4, 30.0, 3.6, 4.4, 0.33, 1.25)]))
# two stacks: a tall flue with its cap, and a fat exhaust
for x, z, h, r, cap in ((23.4, -3.9, 1.9, 0.09, 0.2), (35.6, 1.7, 1.15, 0.17, 0.3)):
    kit.append(K.cylinder('stack', x, z, 3.5, 3.6 + h, r, 14))
    kit.append(K.cylinder('cap', x, z, 3.6 + h + 0.05, 3.6 + h + 0.11, cap, 14))
    kit.append(K.cylinder('collar', x, z, 3.55, 3.68, r + 0.05, 14))
# the cable tray across the control room's roof to its north-east corner, on stanchions, and
# the conduit that takes the array's feeds down the north wall from it
TY = 4.18
tray = [(0.7, 5.95, TY, TY + 0.03, -4.12, -3.82), (0.7, 5.95, TY + 0.03, TY + 0.1, -4.12, -4.1), (0.7, 5.95, TY + 0.03, TY + 0.1, -3.84, -3.82),
        (0.75, 5.9, TY + 0.03, TY + 0.08, -4.06, -3.88)]
tray += [(x - 0.03, x + 0.03, 3.6, TY, -4.0, -3.94) for x in (1.0, 2.6, 4.2, 5.7)]
tray += [(x - 0.25, x + 0.03, TY - 0.03, TY, -4.0, -3.94) for x in (1.0, 2.6, 4.2, 5.7)]
tray += [(5.9, 6.0, TY - 0.02, TY + 0.1, -4.12, -3.82), (5.92, 6.0, 3.84, TY + 0.06, -4.92, -4.74), (5.92, 6.0, TY - 0.02, TY + 0.06, -4.92, -4.1)]
kit.append(K.union_boxes('tray', tray))
kit.append(K.union_boxes('conduit', [(5.86, 5.94, -0.05, TY + 0.02, -4.94, -4.86), (5.84, 5.96, 0.3, 0.36, -4.96, -4.8), (5.84, 5.96, 1.6, 1.66, -4.96, -4.8),
                                     (5.84, 5.96, 2.9, 2.96, -4.96, -4.8)]))
# the ladder up the wing's east end (north of its meter), its rails curled over the parapet
LX, LZ = 38.6, (-3.85, -3.35)
lad = [(LX + 0.14, LX + 0.19, 0.0, 4.45, z - 0.025, z + 0.025) for z in LZ]
lad += [(LX - 0.3, LX + 0.19, 4.4, 4.45, z - 0.025, z + 0.025) for z in LZ]
lad += [(LX - 0.3, LX - 0.25, 3.6, 4.45, z - 0.025, z + 0.025) for z in LZ]
lad += [(LX + 0.15, LX + 0.18, y - 0.015, y + 0.015, LZ[0], LZ[1]) for y in np.arange(0.3, 3.7, 0.3)]
lad += [(LX, LX + 0.19, y - 0.02, y + 0.02, z - 0.02, z + 0.02) for y in (0.6, 2.2, 3.4) for z in LZ]
kit.append(K.union_boxes('ladder', lad))
for o in kit: K.bevel(o, 0.006, 1)

# ---------- the wing ----------
wing = K.union_boxes('wing', [WING, PLINTH_WING])
# its windows (the facade picture's, core/textures facadeWindows; the dressing's sills and frames
# stand round them, dressSaro.ts): let 22 cm into the wall
WIN = [(20.6 + 2.25 * i + 0.4, 20.6 + 2.25 * i + 1.85) for i in range(8)]
WY, DEPTH = (0.58, 2.34), 0.22
cuts = [K.box_object(f'cut{i}{s}', (a0, a1, WY[0], WY[1], *((5.5 - DEPTH, 5.7) if s == 's' else (-5.7, -5.5 + DEPTH))))
        for i, (a0, a1) in enumerate(WIN) for s in 'sn']
K.boolean(wing, cuts, 'DIFFERENCE')
K.tidy(wing)
K.grid_cut(wing)
K.bevel(wing, 0.02, 2)
under(wing)
log('wing:', len(wing.data.polygons), 'faces')

# ---------- the skirt: the ground round the plinths ----------
SK = 1.4
PLINTHS = [(-6.7, 6.7, -5.9, 5.9), (-6.6, 2.6, 5.8, 11.1), (20.2, 39.0, -5.9, 5.9)]
SKY = GROUND + 0.012
skirt = K.union_boxes('skirt', [(x0 - SK, x1 + SK, SKY - 0.02, SKY, z0 - SK, z1 + SK) for x0, x1, z0, z1 in PLINTHS])
K.boolean(skirt, [K.box_object(f'p{i}', (x0, x1, SKY - 1, SKY + 1, z0, z1)) for i, (x0, x1, z0, z1) in enumerate(PLINTHS)], 'DIFFERENCE')
bm = bmesh.new(); bm.from_mesh(skirt.data)
bmesh.ops.delete(bm, geom=[f for f in bm.faces if K.game(f.normal)[1] < 0.9], context='FACES')
bm.to_mesh(skirt.data); bm.free()
K.tidy(skirt); K.grid_cut(skirt, 5)
log('skirt:', len(skirt.data.polygons), 'faces')

# ---------- materials and colours ----------
def mat(name, rough, metal=0.0, rgb=(1, 1, 1)):
    m = bpy.data.materials.new(name)
    b = m.node_tree.nodes.get('Principled BSDF')
    b.inputs['Base Color'].default_value = (*rgb, 1); b.inputs['Roughness'].default_value = rough; b.inputs['Metallic'].default_value = metal
    vc = m.node_tree.nodes.new('ShaderNodeVertexColor'); vc.layer_name = 'Col'
    m.node_tree.links.new(vc.outputs['Color'], b.inputs['Base Color'])
    m.use_backface_culling = True
    return m
MATS = [mat('house', 0.95), mat('wing', 0.92), mat('glass_dark', 1.0), mat('glass_lit', 1.0), mat('skirt', 1.0)]
# the vertex colours multiply the concrete picture (as M.concrete's 0xd1c8b8 does); the metal is
# lighter than it looks so it comes out at the poles' and the coping's grey under that picture
# (the game tells the metal from the concrete by the colour: the concrete's is warm, world/blenderModels.ts)
CONCRETE, PLINTH, DECK, COPE, HEAD, KIT = 0xd1c8b8, 0xdcd5c7, 0x6e6860, 0xc4c6c2, 0x8f8f8a, 0xa9aaa6
WALL_WING, DECK_WING, GLASS_DARK, GLASS_LIT = 0xc6bcaa, 0x5f5a54, 0x2a2e33, 0xffe0b0
for o in (house, cope, heads, wing, skirt, *kit): K.set_materials(o, MATS)
def house_paint(c, n, p):
    if c[1] < -0.005: return 0, PLINTH
    if n[1] > 0.7 and ((c[1] > 3.55 and abs(c[0]) < 6.06 and abs(c[2]) < 4.56) or (2.9 < c[1] < 3.25 and c[2] > 4.79)): return 0, DECK
    return 0, CONCRETE
K.paint(house, house_paint)
K.paint(cope, lambda c, n, p: (0, COPE))
K.paint(heads, lambda c, n, p: (0, HEAD))
for o in kit: K.paint(o, lambda c, n, p: (0, KIT))
K.paint(skirt, lambda c, n, p: (4, 0xffffff))
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
model = K.join([house, cope, heads, wing, skirt, *kit], 'saro_house')
K.flat_normals(model)
K.tile_uv(model, 2.5)

# the lit panes: their first set is the room behind (rooms.png, four rooms side by side), each
# pane the whole of one room, the rooms taken in turn along each face
ROOM = {('s', i): k for k, i in enumerate(i for i in range(8) if lit['s'][i])}
ROOM.update({('n', i): (k + 2) for k, i in enumerate(i for i in range(8) if lit['n'][i])})
me = model.data
uv0 = me.uv_layers['UVMap']
lit_panes = 0
for p in me.polygons:
    if p.material_index != 3: continue
    c = K.game(p.center); side = 's' if c[2] > 0 else 'n'
    i = next(i for i, (a0, a1) in enumerate(WIN) if a0 < c[0] < a1)
    a0, a1 = WIN[i]; k = ROOM[side, i] % 4
    for li in p.loop_indices:
        x, y, _ = K.game(me.vertices[me.loops[li].vertex_index].co)
        u = (x - a0) / (a1 - a0) if side == 's' else (a1 - x) / (a1 - a0)
        uv0.data[li].uv = ((k + min(max(u, 0), 1)) / 4, min(max((y - WY[0]) / (WY[1] - WY[0]), 0), 1))
    lit_panes += 1
log('lit panes:', lit_panes)
# the roofs and the decks get a third of the texels (nobody walks up there), the skirt as much
fill = K.lightmap_uv(model, shrink=lambda c, n: 0.33 if n[1] > 0.7 and (c[1] > 2.9 or c[1] < -0.4) else 1.0)
log('model:', len(model.data.polygons), 'faces,', len(model.data.vertices), f'vertices; the lightmap covers {fill * 100:.0f}% of its atlas')

# ---------- the bake ----------
# the ground the house stands in (saroLand: flat here), under the context from the game
bm = bmesh.new()
for v in [(-80, -60), (110, -60), (110, 70), (-80, 70)]: bm.verts.new(G(v[0], GROUND - 0.02, v[1]))
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
covered = K.coverage(model, SIZE)
log(f'occlusion and edges baked; {covered.mean() * 100:.0f}% of the texels covered')

# ---------- the lamps ----------
# Each: the flood the game lights it with (found among the site's lamps by where it stands), and
# the fittings it stands for: where the light leaves them, where they aim, the cone (degrees)
WALLPACK = 150
LAMPS = [
    ('west wall packs', (-7.5, 2.4, 2.0), [((-6.45, 2.44, 2.0), (-0.6, -1, 0)), ((-6.45, 2.44, -3.0), (-0.6, -1, 0))], WALLPACK),
    ('the wing\'s wall packs', (24.4, 2.6, 7.0), [((20.9, 2.64, 5.84), (0, -1, 0.6)), ((30.4, 2.64, 5.84), (0, -1, 0.6))], WALLPACK),
    ('the annex\'s wall pack', (-2.0, 2.2, 11.6), [((-2.0, 2.24, 10.98), (0, -1, 0.6))], WALLPACK),
    ('the fire exit\'s lamp', (-6.9, 2.1, 5.7), [((-6.45, 2.19, 5.7), (-0.35, -1, 0))], 160),
    ('the lab\'s door lamp', (12.2, 2.6, -2.0), [((12.4, 2.62, -2.0), (-0.6, -1, 0))], WALLPACK),
    # the yard's lamp posts (ServiceYard.ts lampPost: the head under the lantern)
    ('yard post by the east door', (10.3, 4.15, 1.6), [((10.3, 4.13, 1.6), (0, -1, 0))], 140),
    ('yard post by the lab', (10.3, 4.15, -6.0), [((10.3, 4.13, -6.0), (0, -1, 0))], 140),
    ('yard post north-east', (8.7, 4.15, -9.2), [((8.7, 4.13, -9.2), (0, -1, 0))], 140),
    ('yard post by the pad', (7.8, 4.15, 11.6), [((7.8, 4.13, 11.6), (0, -1, 0))], 140),
]
floods = facts['floods']
for L in LAMPS:
    hit = [f for f in floods if math.dist(f[:3], L[1]) < 0.05]
    if not hit: raise SystemExit(f'the game has changed: no lamp of the site\'s set at {L[1]} ({L[0]})')
    if hit[0][3] <= 0: raise SystemExit(f'{L[0]}: its lamp is off in the export ({hit[0][3]})')
p, nr, ao = K.pixels(P), K.pixels(N), K.pixels(A)
gx, gy, gz = p[..., 0], p[..., 2], -p[..., 1]
nx, ny, nz = nr[..., 0], nr[..., 2], -nr[..., 1]
vert = np.abs(ny) < 0.35
up = ny > 0.7
skirt_t = up & (gy < GROUND + 0.2)
onwing = gx > 20.0
glass = vert & onwing & (np.abs(np.abs(gz) - (5.5 - DEPTH)) < 0.01) & (gy > WY[0]) & (gy < WY[1])
fall = np.where(onwing, 0.03, 0.012)       # the falloffs the boxes had (M.concrete, the wing's)
FAKE = 0.6                                 # how much of a baked lamp's fake flood the model keeps
PEAK = 2.2                                 # the baked light's bright end against the fake pool's
for o in ctx: o.hide_render = False
lights = []
for k, (name, fl, fits, cone) in enumerate(LAMPS):
    ls = [K.spot(f'lamp{k}.{j}', at, aim, cone, 0.45) for j, (at, aim) in enumerate(fits)]
    # and the glow of the lens itself, all round, a little in front of it: the hot spot it puts on
    # the wall it hangs on (a wall pack's light is not only what its reflector throws down)
    for j, (at, aim) in enumerate(fits):
        if aim[0] == aim[2] == 0: continue        # (a lamp post's lantern: its shade keeps it to the ground)
        o = (aim[0] * 0.08, -0.03, aim[2] * 0.08)
        ls.append(K.point(f'lens{k}.{j}', (at[0] + o[0], at[1] + o[1], at[2] + o[2]), 18.0, 0.03))
    lights.append(ls)
for ls in lights:
    for l in ls: l.hide_render = True
baked = []
E = K.image('lamp', SIZE, True)
for k, (name, fl, fits, cone) in enumerate(LAMPS):
    for l in lights[k]: l.hide_render = False
    K.bake_diffuse(model, E, samples=96)
    for l in lights[k]: l.hide_render = True
    e = K.pixels(E)[..., 0]
    w = next(f[3] for f in floods if math.dist(f[:3], fl) < 0.05)
    # the fake flood as kit.floodlit draws it, for the level; the bake's shape: its cone, its
    # cosine and its shadows, the falloff eased to the game's past the first metre (a hot spot
    # at the fitting, the pools as wide as the game's)
    dx, dy, dz = fl[0] - gx, fl[1] - gy, fl[2] - gz
    d = np.sqrt(dx * dx + dy * dy + dz * dz) + 1e-6
    ndl = np.abs((nx * dx + ny * dy + nz * dz) / d) * 0.75 + 0.25
    win = np.clip(1 - d / (w * 1.6 + 8), 0, 1)
    F = w * ndl * win * win / (1 + d * d * fall) * 0.07
    df = np.min([np.sqrt((a[0] - gx) ** 2 + (a[1] - gy) ** 2 + (a[2] - gz) ** 2) for a, _ in fits], 0)
    S = e * (df * df + 0.5) / (1 + df * df * fall) * win * win
    ok = covered & ~skirt_t & ~glass & (S > 0)
    if ok.sum() < 200:
        log(f'lamp {name}: hardly reaches the house ({ok.sum()} texels): left to its fake flood'); baked.append(None); continue
    # the level: on the walls within 3 m of the fittings the bright end of the baked light comes to
    # PEAK times the bright end of the fake pool there (what the fake keeps, FAKE, comes on top)
    near = ok & (df < 3.0) & vert
    if near.sum() < 50: near = ok & (df < 3.0)
    if near.sum() < 50: near = ok
    kk = float(PEAK * np.percentile(F[near], 95) / max(np.percentile(S[near], 95), 1e-9))
    B = np.where(covered & ~skirt_t & ~glass, S * kk, 0.0)
    baked.append(B)
    log(f'lamp {name}: w {w}, its light on {ok.mean() * 100:.1f}% of the texels, scaled by {kk:.3g}')
used = [i for i in range(len(LAMPS)) if baked[i] is not None]
if len(used) < 4: raise SystemExit(f'only {len(used)} lamps came out of the bake: run it again with the graphics card free')
# three channels: each lamp into the one where it overlaps least with the lamps already there
# (in the game a texel takes its channel's nearest lamp's strength: only lamps that do not light
# the same wall share one)
order = sorted(used, key=lambda i: -baked[i].sum())
chan = {}
for i in order:
    over = [sum(np.minimum(baked[i], baked[j]).sum() for j in chan if chan[j] == c) for c in range(3)]
    chan[i] = int(np.argmin(over))
light = np.zeros(baked[0].shape + (3,), np.float32)
for i in used: light[..., chan[i]] += baked[i]
LMAX = float(np.percentile(light[covered].max(-1), 99.9)) * 1.05
log('channels:', {LAMPS[i][0]: chan[i] for i in used}, f'max {LMAX:.3f}')
lamp_facts = [{'name': LAMPS[i][0], 'flood': list(LAMPS[i][1]), 'w': next(f[3] for f in floods if math.dist(f[:3], LAMPS[i][1]) < 0.05), 'ch': chan[i],
               'at': [round(float(np.mean([a[0][k] for a in LAMPS[i][2]])), 3) for k in range(3)],
               'fits': [list(a[0]) for a in LAMPS[i][2]]} for i in used]

# ---------- the mask ----------
along = np.where(np.abs(nz) > np.abs(nx), gx, gz)          # along the wall
plinth = vert & (gy < -0.005)
# occlusion kept off black: the floods are fake lamps, a corner is darker, not unlit; the
# plinths' faces stay light (the skirt on the ground grounds them now)
occl = 0.3 + 0.7 * np.clip(ao[..., 0], 0, 1) ** 1.1
# the plinths' faces and the ledge on top of them where the walls stand back stay light: the
# house's base line (the skirt on the ground grounds it now)
ledge = up & (np.abs(gy) < 0.05)
occl = np.where(plinth, np.maximum(occl, 0.74), np.where(ledge, np.maximum(occl, 0.8), occl))
cav = 1 - np.clip(ao[..., 1], 0, 1)
TX = 0.016 * 2048 / SIZE                                    # about a texel in metres, for soft lines
def line(d, w):   # soft line of half width w at d = 0
    return 1 - K.smoothstep(w, w + TX, np.abs(d))
# splash and dirt rising from the ground: on the walls from the plinth they stand on, a little
# on the plinths' faces near the land
splash = vert * np.where(gy > -0.005, np.exp(-np.maximum(gy, 0) / 0.3) * 0.8, 0.14 * K.smoothstep(-0.3, GROUND, gy)) \
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
dust = up * (0.2 + 0.25 * K.fbm(gx * 0.9, gy * 0.9, gz * 0.9, 3, 41)) * np.where(ledge, 0.45, 1.0)
blotch = (K.fbm(gx * 0.33, gy * 0.45, gz * 0.33, 4, 53) - 0.5) * 0.36
dirt = splash + streak * 0.75 + cav * 0.5 * (vert | (gy > 0.02)) + dust + blotch
dirt = np.where(glass | skirt_t, 0.0, np.clip(dirt, 0, 1))
edge = np.clip(ao[..., 2] * 7, 0, 1)
chips = K.smoothstep(0.42, 0.68, K.fbm(gx * 8, gy * 8, gz * 8, 3, 61))
wear = np.where(glass | up, 0.0, np.clip(edge * (0.25 + 0.75 * chips), 0, 1))
# the skirt: the shade on the ground, fading out to its edge (the distance to the plinths)
dp = np.min([np.hypot(np.maximum(np.maximum(x0 - gx, gx - x1), 0), np.maximum(np.maximum(z0 - gz, gz - z1), 0)) for x0, x1, z0, z1 in PLINTHS], 0)
shade = (1 - np.clip(ao[..., 0], 0, 1)) * K.smoothstep(SK - 0.05, 0.25, dp)
mask = np.stack([np.where(glass, 1.0, np.where(skirt_t, 1 - np.clip(shade * 1.25, 0, 0.75), occl)), dirt, wear], -1)
mask = K.dilate(mask, covered)
os.makedirs(WORK, exist_ok=True)
png = os.path.join(WORK, 'saro_house_mask.png')
K.save_mask(mask, png)
light = K.dilate(np.sqrt(np.clip(light / LMAX, 0, 1)), covered)
lpng = os.path.join(WORK, 'saro_house_light.png')
K.save_mask(light, lpng)
log('mask and light written', os.path.getsize(png) // 1024, 'and', os.path.getsize(lpng) // 1024, 'KB')

# ---------- the rooms behind the lit panes ----------
# Four rooms side by side, 256 px each, rows bottom up (as Blender keeps them): the back wall lit
# from a fitting in the ceiling (brightest at the head of the window, falling off down to the
# desks), what stands against it as dark shapes, and one room lit only by a desk lamp.
RW, RH = 256, 256
v, u = np.mgrid[0:RH, 0:RW].astype(np.float32)
v, u = (v + 0.5) / RH, (u + 0.5) / RW
def box(a, u0, u1, v0, v1, val):
    a[(u >= u0) & (u < u1) & (v >= v0) & (v < v1)] = val
def room(k):
    if k == 3:     # lights off: a desk lamp low on the left, the rest in the dark
        lum = 0.04 + 0.75 * np.exp(-(((u - 0.28) / 0.22) ** 2 + ((v - 0.36) / 0.2) ** 2))
    else:
        cx = (0.5, 0.35, 0.62)[k]
        lum = 0.42 + 0.55 * np.exp(-((u - cx) / 0.42) ** 2) * (0.25 + 0.75 * v ** 1.6)
        lum = np.where(v > 0.9, 0.55 + 0.25 * np.exp(-((u - cx) / 0.3) ** 2), lum)            # the ceiling
        box(lum, cx - 0.22, cx + 0.22, 0.935, 0.965, 1.0)                                  # the fitting
    shade = np.ones_like(lum)
    if k == 0:     # a desk, a terminal on it (its screen faintly green), shelves to the right
        box(shade, 0.08, 0.7, 0.0, 0.33, 0.14); box(shade, 0.06, 0.72, 0.31, 0.35, 0.1)
        box(shade, 0.42, 0.6, 0.35, 0.52, 0.12)
        box(shade, 0.79, 0.98, 0.0, 0.74, 0.12)
        for s in (0.2, 0.38, 0.56): box(shade, 0.8, 0.97, s, s + 0.012, 0.35)
    elif k == 1:   # filing cabinets on the left, a chair's back, a board pinned with sheets
        box(shade, 0.04, 0.3, 0.0, 0.48, 0.16); box(shade, 0.05, 0.29, 0.24, 0.25, 0.3)
        box(shade, 0.55, 0.72, 0.0, 0.42, 0.1)
        box(shade, 0.5, 0.9, 0.55, 0.82, 0.62)
        for a, b in ((0.53, 0.6), (0.6, 0.66), (0.66, 0.59), (0.74, 0.67), (0.8, 0.6), (0.57, 0.72), (0.7, 0.73), (0.83, 0.71)):
            box(shade, a, a + 0.045, b, b + 0.06, 0.9)
    elif k == 2:   # a long bench with an instrument rack on it, a lamp hanging over it
        box(shade, 0.0, 1.0, 0.0, 0.3, 0.13); box(shade, 0.0, 1.0, 0.29, 0.32, 0.09)
        box(shade, 0.12, 0.4, 0.32, 0.62, 0.11)
        for s in (0.38, 0.46, 0.54): box(shade, 0.14, 0.38, s, s + 0.01, 0.4)
        box(shade, 0.66, 0.7, 0.62, 1.0, 0.2)
    else:          # the desk under its lamp, the chair, a shelf in the dark
        box(shade, 0.06, 0.62, 0.0, 0.3, 0.2); box(shade, 0.05, 0.63, 0.29, 0.32, 0.3)
        box(shade, 0.66, 0.84, 0.0, 0.4, 0.15)
    warm = np.stack([np.ones_like(lum), 0.74 + 0.08 * v, 0.46 + 0.1 * v], -1)
    out = (lum * shade)[..., None] * warm
    if k == 0: out[(u > 0.45) & (u < 0.57) & (v > 0.37) & (v < 0.49)] = (0.18, 0.42, 0.24)    # the terminal's screen
    return np.clip(out, 0, 1)
rooms = np.concatenate([room(k) for k in range(4)], 1)
rpng = os.path.join(WORK, 'saro_house_rooms.png')
K.save_mask(rooms, rpng)
log('rooms written', os.path.getsize(rpng) // 1024, 'KB')

# ---------- out ----------
for o in lights:
    for l in o: bpy.data.objects.remove(l)
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
json.dump({'lamps': lamp_facts, 'max': LMAX, 'fake': FAKE, 'skirt': SKY}, open(os.path.join(WORK, 'saro_house_lamps.json'), 'w'), indent=1)
log('glb', os.path.getsize(OUT) // 1024, 'KB without the pictures (build.py puts them in)')
for o in list(bpy.context.scene.objects):
    if o not in out: bpy.data.objects.remove(o)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(WORK, 'saro_house.blend'), compress=True)
log('done')
