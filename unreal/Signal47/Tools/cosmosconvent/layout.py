#!/usr/bin/env python3
"""Write Config/Props/church_convent.json: San Isidro in Caliche as a small mission church of bought Cosmos pieces.

The church's own metres (S47Convent): x east from the middle plane of the front wall (the CalicheAccess room "Church", its root at
world x 8021, z 2095.5, the floor at the root's height), up from the floor, z south of the church's axis. A piece's "at" is where its
pivot goes, "yaw" is Unreal degrees (0: a wall piece runs west from its pivot with its outside to the south; 90: outside west, it
runs north; 180: outside north, it runs east; 270: outside east, it runs south), "scale" its own x, y, z. "land": it stands on the
land there instead of at the floor's height. The measures of the pieces are the pack's own (Tools/cosmosconvent/verification.json).

The plan (metres):
  forecourt   x -9 .. 0,   z -6 .. 6         a low wall round three sides, a gate gap in the west side, open by both church corners
  nave        x  0 .. 12,  z -4.5 .. 4.5     walls 0.4 thick, 4.5 high; the front door in the west wall's arch (1.8 wide)
  sacristy    x 12 .. 15,  z -2.25 .. 2.25   behind the altar wall, in through the arch behind the altar screen, out by the back door
  back door   x 15, z 0                      in the east wall's arch, 2 m from the cemetery gate (road x 38, z 2095.5)

    python3 layout.py            (writes the file; the same list every run)
"""
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUT = HERE.parents[1] / 'Config' / 'Props' / 'church_convent.json'

NAVE_X, HALF, SAC_X, SAC_HALF, T, H = 12.0, 4.5, 15.0, 2.25, 0.2, 4.5   # T: half a wall's thickness
DOOR = 1.8          # the arch's clear width (SM_ChapelPosaArcade_01: 4.5 m of wall, the arch in its middle)
ATR_X, ATR_HALF = -9.0, 6.0
pieces, boxes, walls, blocks, lights, words = [], [], [], [], [], []


def piece(pid, mesh, x, up, z, yaw=0, scale=(1, 1, 1), **more):
    s = [scale, scale, scale] if isinstance(scale, (int, float)) else list(scale)
    p = {'id': pid, 'mesh': mesh, 'at': [round(x, 3), round(up, 3), round(z, 3)], 'yaw': yaw, 'scale': [round(v, 4) for v in s]}
    p.update(more)
    pieces.append(p)


def box(bid, cx, cy, cz, sx, sy, sz, colour, **more):
    b = {'id': bid, 'at': [round(cx, 3), round(cy, 3), round(cz, 3)], 'size': [round(sx, 3), round(sy, 3), round(sz, 3)], 'colour': colour}
    b.update(more)
    boxes.append(b)


def wall(x0, x1, z0, z1, wid='wall'):
    walls.append([round(min(x0, x1), 3), round(max(x0, x1), 3), round(min(z0, z1), 3), round(max(z0, z1), 3), wid])


def block(x0, x1, z0, z1, bid):
    blocks.append([round(min(x0, x1), 3), round(max(x0, x1), 3), round(min(z0, z1), 3), round(max(z0, z1), 3), bid])


# ---------- the walls: plaster (SM_FlatWallTall_01, 3 m by 4.5 m, made 0.4 thick), a window piece in every other bay ----------
FLAT, WIN, ARCH, TRIM = 'SM_FlatWallTall_01', 'SM_WindowTall_01', 'SM_ChapelPosaArcade_01', 'SM_OutsideTrims_01'
BAY = (NAVE_X + 2 * T) / 4          # the side walls run past the corners: four bays of 3.1 m
for i in range(4):
    kind = WIN if i in (1, 3) else FLAT
    sy = 1.6 if kind == WIN else 2
    piece('south%d' % i, kind, -T + BAY * (i + 1), 0, HALF, 0, (BAY / 3, sy, 1))
    piece('north%d' % i, kind, -T + BAY * i, 0, -HALF, 180, (BAY / 3, sy, 1))
    piece('trimS%d' % i, TRIM, -T + BAY * (i + 1), H - .6, HALF, 0, (BAY / 3, 2, 1))
    piece('trimN%d' % i, TRIM, -T + BAY * i, H - .6, -HALF, 180, (BAY / 3, 2, 1))
SIDE = (HALF - T) - SAC_HALF          # the end walls beside the arch piece: 2.05 m each
# the front (outside west): south part, the arch with the door, north part
piece('frontS', FLAT, 0, 0, HALF - T, 90, (SIDE / 3, 2, 1))
piece('frontArch', ARCH, 0, 0, SAC_HALF, 90)
piece('frontN', FLAT, 0, 0, -SAC_HALF, 90, (SIDE / 3, 2, 1))
for k in range(3):
    piece('trimW%d' % k, TRIM, 0, H - .6, HALF + T - k * (2 * HALF + 2 * T) / 3, 90, ((2 * HALF + 2 * T) / 9, 2, 1))
# the altar wall (outside east): north part, the arch into the sacristy, south part
piece('altarWallN', FLAT, NAVE_X, 0, -(HALF - T), 270, (SIDE / 3, 2, 1))
piece('altarWallArch', ARCH, NAVE_X, 0, -SAC_HALF, 270)
piece('altarWallS', FLAT, NAVE_X, 0, SAC_HALF, 270, (SIDE / 3, 2, 1))
piece('trimEN', TRIM, NAVE_X, H - .6, -(HALF + T), 270, ((SIDE + 2 * T) / 3, 2, 1))
piece('trimES', TRIM, NAVE_X, H - .6, SAC_HALF + T, 270, ((SIDE + 2 * T) / 3, 2, 1))
# the sacristy: plaster sides, the back door's arch in its east wall
SAC_LEN = SAC_X - NAVE_X
piece('sacN', FLAT, NAVE_X + T, 0, -SAC_HALF, 180, (SAC_LEN / 3, 2, 1))
piece('sacS', FLAT, SAC_X + T, 0, SAC_HALF, 0, (SAC_LEN / 3, 2, 1))
piece('sacArch', ARCH, SAC_X, 0, -SAC_HALF, 270)
piece('trimSacN', TRIM, NAVE_X + T, H - .6, -SAC_HALF, 180, (SAC_LEN / 3, 2, 1))
piece('trimSacS', TRIM, SAC_X + T, H - .6, SAC_HALF, 0, (SAC_LEN / 3, 2, 1))
for k in range(2):
    piece('trimSacE%d' % k, TRIM, SAC_X, H - .6, -(SAC_HALF + T) + k * (SAC_HALF + T), 270, ((SAC_HALF + T) / 3, 2, 1))
# what the walker meets
wall(-T, NAVE_X + T, -HALF - T, -HALF + T); wall(-T, NAVE_X + T, HALF - T, HALF + T)
wall(-T, T, -HALF - T, -DOOR / 2); wall(-T, T, DOOR / 2, HALF + T)
wall(NAVE_X - T, NAVE_X + T, -HALF - T, -DOOR / 2); wall(NAVE_X - T, NAVE_X + T, DOOR / 2, HALF + T)
wall(NAVE_X + T, SAC_X + T, -SAC_HALF - T, -SAC_HALF + T); wall(NAVE_X + T, SAC_X + T, SAC_HALF - T, SAC_HALF + T)
wall(SAC_X - T, SAC_X + T, -SAC_HALF - T, -DOOR / 2); wall(SAC_X - T, SAC_X + T, DOOR / 2, SAC_HALF + T)

# ---------- the bell gable over the door: the arch piece small, the bell in it, the cross on top ----------
G = .6
piece('gable', ARCH, 0, H, 4.5 * G / 2, 90, G)
piece('bell', 'SM_Bell', .02, H + 1.15, 0, 90, .2)
piece('cross', 'SM_Cross', 0, H + 4.5 * G, 0, 90, .3)

# ---------- the roof: a flat slab under the walls' tops (own boxes), round beams under it, their ends out through the side walls ----------
ROOF = '4a3b2c'
box('roofNave', NAVE_X / 2, H - .25, 0, NAVE_X - 2 * T + .1, .3, 2 * (HALF - T) + .1, ROOF, shadow=True)
box('roofSacristy', (NAVE_X + SAC_X) / 2, H - .25, 0, SAC_LEN + .1, .3, 2 * (SAC_HALF - T) + .1, ROOF, shadow=True)
for i in range(8):
    piece('viga%d' % i, 'SM_Log', 1.0 + i * 1.43, H - .52, 0, 0, (.8, 9.9 / 1.21, .8))
for i in range(2):
    piece('vigaSac%d' % i, 'SM_Log', NAVE_X + .9 + i * 1.3, H - .52, 0, 0, (.7, 5.2 / 1.21, .7))
# the walls' feet go a metre into the ground (own boxes, hidden where the land is level with the floor)
FOOT = 'b9aa90'
box('footN', NAVE_X / 2, -.5, -HALF, NAVE_X + 2 * T, 1, 2 * T, FOOT); box('footS', NAVE_X / 2, -.5, HALF, NAVE_X + 2 * T, 1, 2 * T, FOOT)
box('footW', 0, -.5, 0, 2 * T, 1, 2 * HALF, FOOT); box('footE', NAVE_X, -.5, 0, 2 * T, 1, 2 * HALF, FOOT)
box('footSacN', NAVE_X + SAC_LEN / 2, -.5, -SAC_HALF, SAC_LEN + 2 * T, 1, 2 * T, FOOT); box('footSacS', NAVE_X + SAC_LEN / 2, -.5, SAC_HALF, SAC_LEN + 2 * T, 1, 2 * T, FOOT)
box('footSacE', SAC_X, -.5, 0, 2 * T, 1, 2 * SAC_HALF, FOOT)

# ---------- the floor: brick paving, its top at the floor's height ----------
TILE = 'SM_PavingStoneTileBig_01'
for ix in range(4):
    for iz in range(3):
        piece('floor%d%d' % (ix, iz), TILE, 3.0 * (ix + 1), -.1, -HALF + 3.0 * iz, 0)
piece('floorSac', TILE, SAC_X, -.1, -SAC_HALF, 0, (1, 1.5, 1))
for ix in range(3):
    piece('walk%d' % ix, TILE, -3.0 * ix, -.1, -1.5, 0)

# ---------- inside ----------
# pews: two blocks of five, facing the altar; an aisle in the middle and a narrow one along each wall
for r in range(5):
    px = 2.6 + 1.25 * r
    for side, zc in (('S', 2.3), ('N', -2.3)):
        piece('pew%s%d' % (side, r), 'SM_Bench', px, 0, zc, 270, (.8, 1, .85))      # (the bench looks along its own +Y: yaw 270 turns that east)
        block(px - .62, px - .12, zc - 1.21, zc + 1.21, 'pew')
# the altar: the table, the screen behind it standing free of the wall (a way round it to the sacristy's arch, and out of sight)
piece('altarTable', 'SM_Desk', 9.1, 0, 0, 90, .75)
block(8.45, 9.75, -1.06, 1.06, 'altar')
piece('altarScreen', 'SM_AltarWallOrnament', 10.6, 0, 0, 90, .36)
# (the screen is a relief with a front only, starting 0.8 m up: a plain panel of the game's own behind it and under it)
box('screenBack', 10.72, 1.7, 0, .16, 3.4, 3.0, 'cfc5b0', shadow=True)
block(10.5, 10.8, -1.5, 1.5, 'screen')
piece('altarCross', 'SM_ChurchTableCross', 9.4, .83, 0, 90)
piece('altarCandleS', 'SM_ChurchCandle', 9.3, .83, .7, 0, .6)
piece('altarCandleN', 'SM_ChurchCandle', 9.3, .83, -.7, 0, .6)
piece('chalice', 'SM_ChurchChalice', 9.0, .83, .25, 0)
piece('candleS', 'SM_ChurchCandle', 10.25, 0, 1.95, 0, 1.6)
piece('candleN', 'SM_ChurchCandle', 10.25, 0, -1.95, 0, 1.6)
piece('lectern', 'SM_Podium', 9.3, 0, -2.4, 270)
block(9.05, 9.55, -2.8, -2.0, 'lectern')
piece('chairN1', 'SM_Chair', 9.9, 0, -3.85, 0)
piece('chairN2', 'SM_Chair', 10.7, 0, -3.85, 0)
piece('chairS', 'SM_Chair', 10.6, 0, 3.85, 180)
piece('frameN', 'SM_ChurchFrame01_1', 7.55, 2.3, -(HALF - T) + .01, 90)
piece('frameS', 'SM_ChurchFrame02_1', 7.55, 2.3, (HALF - T) - .01, 270)
piece('frameW', 'SM_ChurchFrame03_1', T + .01, 2.3, -3.0, 0)
piece('chandelier', 'SM_Chandelier', 7.0, 3.1, 0, 0, .3)
piece('bellRope', 'SM_Rope', .75, .25, -3.7, 0, (1.5, 1.5, .262))
# the sacristy: a vestment corner behind curtains along the south side (a place out of sight), barrels and a chair on the north
for k in range(3):      # (the cloth has a front only: two sheets back to back)
    piece('curtain%dN' % k, 'SM_ChurchCurtain', NAVE_X + T + .435 + .87 * k, 3.0, .98, 180, (1, 1, .75), shadow=False)
    piece('curtain%dS' % k, 'SM_ChurchCurtain', NAVE_X + T + .435 + .87 * k, 3.0, .98, 0, (1, 1, .75), shadow=False)
piece('barrel1', 'SM_Barrel01_1', 12.75, 0, -1.55, 0, .7)
piece('barrel2', 'SM_Barrel01_1', 13.55, 0, -1.6, 40, .7)
block(12.4, 13.9, -1.95, -1.2, 'barrels')
piece('sacChair', 'SM_Chair', 14.35, 0, -1.5, 45)

# ---------- the forecourt: a low wall (SM_Fence_01, 4.5 m, made 1.3 m high), posts at the corners and the gate, benches, agaves ----------
FENCE, POST = 'SM_Fence_01', 'SM_FenceColumn_01'
FZ, PZ = .52, .5
GATE = 2.6
for k in range(2):      # the north and south sides, from the west corners to the church's front line
    piece('atrN%d' % k, FENCE, ATR_X + 4.5 * k, 0, -ATR_HALF, 180, (1, 1, FZ), land=True)
    piece('atrS%d' % k, FENCE, ATR_X + 4.5 * (k + 1), 0, ATR_HALF, 0, (1, 1, FZ), land=True)
RUN = ATR_HALF - GATE / 2          # the west side: a run each side of the gate
piece('atrWS', FENCE, ATR_X, 0, ATR_HALF, 90, (RUN / 4.5, 1, FZ), land=True)
piece('atrWN', FENCE, ATR_X, 0, -GATE / 2, 90, (RUN / 4.5, 1, FZ), land=True)
for name, x, z in (('postSW', ATR_X, ATR_HALF), ('postNW', ATR_X, -ATR_HALF), ('postGateS', ATR_X, GATE / 2 + .2), ('postGateN', ATR_X, -GATE / 2 - .2),
                   ('postSE', 0, ATR_HALF), ('postNE', 0, -ATR_HALF)):
    piece(name, POST, x, 0, z, 0, (1, 1, PZ), land=True)
    block(x - .38, x + .38, z - .38, z + .38, 'post')
wall(ATR_X, 0, -ATR_HALF - .2, -ATR_HALF + .2, 'atrium'); wall(ATR_X, 0, ATR_HALF - .2, ATR_HALF + .2, 'atrium')
wall(ATR_X - .2, ATR_X + .2, GATE / 2, ATR_HALF, 'atrium'); wall(ATR_X - .2, ATR_X + .2, -ATR_HALF, -GATE / 2, 'atrium')
piece('benchN', 'SM_BenchExterior', -4.5, 0, -ATR_HALF + .6, 90, 1, land=True)
piece('benchS', 'SM_BenchExterior', -4.5, 0, ATR_HALF - .6, 90, 1, land=True)
block(-5.92, -3.08, -ATR_HALF + .34, -ATR_HALF + .86, 'bench'); block(-5.92, -3.08, ATR_HALF - .86, ATR_HALF - .34, 'bench')
for i, (mesh, x, z, yaw, s) in enumerate((('SM_Agave01_1', -7.9, -4.9, 20, .8), ('SM_Agave02_1', -7.8, 4.8, 130, .75), ('SM_Agave02_1', -.5, -7.0, 250, .6),
                                          ('SM_Agave01_1', -1.4, 6.9, 70, .7), ('SM_Agave01_1', -10.2, 3.3, 200, .65), ('SM_Agave02_1', 15.7, 2.6, 310, .55))):
    piece('agave%d' % i, mesh, x, 0, z, yaw, s, land=True, shadow=True)
    block(x - .45 * s, x + .45 * s, z - .45 * s, z + .45 * s, 'agave')

# ---------- the letter board by the street, where the old church had it (road x 18.9, z 2087.3; Town.ts): two posts, a dark board, the words ----------
BX, BZ = 18.9 - 21, 2087.3 - 2095.5
for name, z in (('boardPostN', BZ - 1.3), ('boardPostS', BZ + 1.3)):
    box(name, BX, .97, z, .12, 1.95, .12, '4a3b2c', land=True, shadow=True)
    block(BX - .1, BX + .1, z - .1, z + .1, 'boardPost')
box('board', BX, 1.4, BZ, .1, .95, 2.75, '22201c', land=True, shadow=True)
words.append({'text': 'SAN ISIDRO|THE HEAVENS DECLARE|THE GLORY OF GOD|PS 19:1', 'at': [round(BX - .06, 3), 1.4, round(BZ, 3)], 'yaw': 180, 'size': .15, 'land': True})

# ---------- light: the game's own lamps. Warm and few; the corners stay dark ----------
WARM = [1.0, .78, .5]
lights += [
    {'id': 'altar', 'at': [9.6, 1.75, 0], 'lumens': 260, 'reach': 5.5, 'colour': WARM, 'shadows': True},
    {'id': 'sacristy', 'at': [13.5, 2.7, -.6], 'lumens': 150, 'reach': 4.0, 'colour': WARM, 'shadows': True},
    {'id': 'door', 'at': [-.75, 3.3, 0], 'lumens': 420, 'reach': 9.0, 'colour': [1.0, .82, .58], 'shadows': True, 'night': True},
    {'id': 'back', 'at': [15.65, 3.2, 0], 'lumens': 160, 'reach': 6.0, 'colour': [1.0, .82, .58], 'shadows': True, 'night': True},
    {'id': 'board', 'at': [round(BX - .6, 3), 2.2, round(BZ, 3)], 'lumens': 120, 'reach': 3.5, 'colour': [1.0, .86, .63], 'shadows': False, 'night': True},
]

doc = ("CHURCH-COLONIAL-20261113 (Tom, 13 November: \"bytt til kolonikirka\"): San Isidro in Caliche as a small mission church of bought Cosmos "
       "pieces (pack Colonial Convent; the meshes by Tools/cosmosconvent, git-ignored under /Game/Licensed/CosmosConvent, used only when every "
       "one of them is there: the white board church stands otherwise, never both). Read by S47Convent. The church's own metres: x east from "
       "the front wall's middle plane, up from the floor, z south of the axis; the room is CalicheAccess's \"Church\". pieces: mesh, at (the "
       "pivot), yaw (Unreal degrees), scale; land: on the land there instead of at the floor's height. boxes: plain boxes of the game's own "
       "(the roof slab, the walls' feet): middle, size, colour. walls and blocks: what stops a walker [x0, x1, z0, z1, id]. lights: the game's "
       "own point lights (lumens, reach in metres; night: only when the town's lamps are lit). words: pale paint on a board (| is a new line; yaw: the way they face). doors: the clear width and height of the two "
       "arches (front: CalicheAccess's own door; back: the sacristy's, toward the cemetery). pad: the level ground it asks of S47GroundEdits "
       "(local metres; cut into the ground mesh by Tools/freeformterrain/site_ground.py). Written by Tools/cosmosconvent/layout.py; hand "
       "edits are lost on the next run.")
out = {'_doc': doc, 'room': 'Church',
       'doors': {'width': DOOR, 'height': 3.2, 'front': [0, 0], 'back': [SAC_X, 0]},
       'pad': {'x0': -9.8, 'x1': 16.0, 'z0': -7.6, 'z1': 7.6, 'under': .02, 'bank': 3.0},
       'meshes': '/Game/Licensed/CosmosConvent/Convent/Meshes',
       'pieces': pieces, 'boxes': boxes, 'walls': walls, 'blocks': blocks, 'lights': lights, 'words': words}
lines = ['{', ' "_doc": %s,' % json.dumps(doc), ' "room": "Church",', ' "meshes": "/Game/Licensed/CosmosConvent/Convent/Meshes",', ' "doors": %s,' % json.dumps(out['doors']), ' "pad": %s,' % json.dumps(out['pad'])]
for key in ('pieces', 'boxes', 'walls', 'blocks', 'lights', 'words'):
    lines.append(' "%s": [' % key)
    lines += ['  ' + json.dumps(p) + (',' if i < len(out[key]) - 1 else '') for i, p in enumerate(out[key])]
    lines.append(' ]' + (',' if key != 'words' else ''))
lines.append('}')
OUT.write_text('\n'.join(lines) + '\n')
print('S47 COSMOS CONVENT LAYOUT', len(pieces), 'pieces of', len({p['mesh'] for p in pieces}), 'meshes,', len(boxes), 'boxes,', len(walls), 'walls,', len(blocks), 'blocks,', len(lights), 'lights')
print(' '.join(sorted({p['mesh'] for p in pieces})))
