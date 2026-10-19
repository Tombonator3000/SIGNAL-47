// The antenna array north of SARO, as numbers: where the 27 antennas stand (Exterior.ts draws
// them), and the gravel tracks out to them through the gate in the array fence (Field.ts walks
// and draws them; FORSLAG.md, section 4). No three.js here, so the ground (Exterior.ts), the
// field and the tests share one layout.
//
//   the gate      in the chain-link fence at z -20, north of the windows side, on the line of
//                 the cable trench from the control room (x -1)
//   the spine     a gravel track from the gate north along the trench, to the last row
//   the rows      east-west tracks off the spine, between the antennas' latitudes; they cross
//                 the highway at grade
//   the highway   walked from just inside the fence to where the truck's way north ends, so the
//                 truck can be left on it among the masts (World.canGetOut; Tom, 19 October); not
//                 through the fence's opening: on foot the gate is the way in
//   the spurs     a short straight track from a row, or the spine, to each antenna
//   the aprons    gravel round each antenna's base, with its drive cabinet where the spur comes in
//
// Every track is an axis-aligned rectangle (a walk zone), and where two meet they overlap by the
// whole width of the narrower, so there is no seam to stand in (memory.md).

export type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
export type Side = 'n' | 's' | 'e' | 'w';

// World layout (metres): control room centred at the origin, windows face north (-Z).
// S-03 is the central antenna at the end of the east service walk (chapter one).
export const DISH_LAYOUT: [string, number, number, number][] = [
  ['S-01', -48, -66, 0.85], ['S-02', -16, -52, 0.85], ['S-03', 9.5, -42, 0.9], ['S-04', 6, -88, 0.85],
  ['S-05', 44, -96, 0.85], ['S-06', -35, -116, 0.85], ['S-07', -80, -104, 0.85], ['S-08', 20, -146, 0.85],
  ['S-09', 66, -156, 0.85], ['S-10', -58, -168, 0.85], ['S-11', -10, -200, 0.85], ['S-12', 100, -120, 0.85],
  ['S-13', -120, -150, 0.85], ['S-14', 40, -230, 0.85], ['S-15', -96, -230, 0.85],
];
// SARO has 27 antennas (story bible, chapter X and the LISTEN ending). The far twelve use the light model.
export const DISH_LAYOUT_FAR: [string, number, number, number][] = [
  ['S-16', 130, -210, 0.85], ['S-17', -150, -270, 0.85], ['S-18', 80, -290, 0.85], ['S-19', -40, -300, 0.85],
  ['S-20', 170, -260, 0.85], ['S-21', -200, -210, 0.85], ['S-22', 10, -360, 0.85], ['S-23', 120, -350, 0.85],
  ['S-24', -120, -340, 0.85], ['S-25', -230, -300, 0.85], ['S-26', 210, -330, 0.85], ['S-27', -60, -420, 0.85],
];

/** The chain-link fence between the windows side and the array. */
export const FENCE_Z = -20;
/** The walk gate in it: its posts, east and west of the cable trench's line. */
export const GATE = { x0: -1.75, x1: -0.25 };
/** The spine runs north from the gate along the cable trench. */
export const SPINE_X = -1;
/** The rows, east-west, clear of every antenna's base and of the lamp posts. */
export const ROWS = [-74, -138, -195, -255, -315, -375];
/** Width of a track's walk zone; the gravel is drawn a little narrower (Field.ts). */
export const TRACK_W = 3.2;
/** Half the side of the gravel square round an antenna. Its base (a collider) is 7 m square at
 *  scale 1 (Dish.ts). */
export const APRON = 6;
/** The lamps out in the array (Exterior.ts): on posts beside the tracks, never on one. */
export const LAMPS: [number, number][] = [[30, -30], [-10, -42], [-40, -56], [12, -78], [48, -86],
  [-70, -94], [-33, -100], [60, -146], [-52, -158], [-4, -190], [94, -110]];
/** The highway north through the array (Exterior.ts draws it to z -480): the rows cross it. The
 *  truck's way north on it ends at z0 (drive/legs.ts SARO_HWY.z0), and so does the walk on it. */
export const HWY = { x0: -28, x1: -20, z0: -200 };
/** The highway's walk zone in the array, as wide as the opening in the fence (Field.ts). */
export const HWY_WALK: Rect = { minX: HWY.x0 - 0.6, maxX: HWY.x1 + 0.6, minZ: HWY.z0 - 1, maxZ: FENCE_Z - 0.15 };

const H = TRACK_W / 2;
const rect = (minX: number, maxX: number, minZ: number, maxZ: number): Rect => ({ minX, maxX, minZ, maxZ });
const centred = (x: number, z: number, hx: number, hz: number) => rect(x - hx, x + hx, z - hz, z + hz);

export interface Antenna {
  id: string; x: number; z: number; scale: number; far: boolean;
  /** Gravel round the base (a walk zone), and the base itself (a collider). */
  apron: Rect; base: Rect;
  /** The track it is reached from (the spine, or a row's z), and the spur to it (null when that
   *  track runs through the apron itself). */
  from: 'spine' | number; spur: Rect | null;
  /** The side of the apron the spur comes in on: the drive cabinet stands there, facing it. */
  side: Side;
  /** The drive cabinet: where it stands, and which way its door faces (rotation about y). */
  cab: { x: number; z: number; ry: number };
}

/** Where the spine ends: the last row. */
const SPINE_END = Math.min(...ROWS);

function plan(): { antennas: Antenna[]; rows: Rect[]; spine: Rect } {
  const all = [...DISH_LAYOUT.map((d) => [...d, false] as const), ...DISH_LAYOUT_FAR.map((d) => [...d, true] as const)];
  const antennas: Antenna[] = [];
  const reach: Record<number, number[]> = {};   // the x of the antennas each row serves
  for (const [id, x, z, scale, far] of all) {
    // the nearest way in: east-west from the spine, or north-south from a row that clears the base
    const bh = 3.5 * scale + 0.05;
    let best: { from: 'spine' | number; len: number } = { from: 'spine', len: z >= SPINE_END && z <= FENCE_Z ? Math.abs(x - SPINE_X) - APRON - H : Infinity };
    for (const r of ROWS) {
      if (Math.abs(z - r) < bh + H + 0.3) continue;   // it would run into the base
      const len = Math.abs(z - r) - APRON - H;
      if (len < best.len) best = { from: r, len };
    }
    const apron = centred(x, z, APRON, APRON), base = centred(x, z, bh, bh);
    let side: Side, spur: Rect | null = null;
    const overlap = 1.5;   // a spur runs on 1.5 m into the apron, and from the middle of its track
    if (best.from === 'spine') {
      side = x > SPINE_X ? 'w' : 'e';
      spur = x > SPINE_X ? rect(SPINE_X, x - APRON + overlap, z - H, z + H) : rect(x + APRON - overlap, SPINE_X, z - H, z + H);
    } else {
      const r = best.from;
      (reach[r] ??= []).push(x);
      side = r > z ? 's' : 'n';
      // the row may run through the apron itself (S-11, S-20): then it needs no spur
      if (Math.abs(z - r) > APRON - overlap + H) spur = r > z ? rect(x - H, x + H, z + APRON - overlap, r) : rect(x - H, x + H, r, z - APRON + overlap);
    }
    // the cabinet: on the apron where the spur comes in, beside it, its door towards it; where
    // the row runs through the apron, on the side towards the spine, by the row
    const off = 2.6, edge = APRON - 1.4;
    if (!spur && best.from !== 'spine') {
      const toward = best.from > z ? 1 : -1;
      side = x > SPINE_X ? 'w' : 'e';
      const cab = side === 'e' ? { x: x + edge, z: z + off * toward, ry: Math.PI / 2 } : { x: x - edge, z: z + off * toward, ry: -Math.PI / 2 };
      antennas.push({ id, x, z, scale, far, apron, base, from: best.from, spur, side, cab });
      continue;
    }
    const cab = side === 's' ? { x: x + off, z: z + edge, ry: 0 } : side === 'n' ? { x: x - off, z: z - edge, ry: Math.PI }
      : side === 'e' ? { x: x + edge, z: z - off, ry: Math.PI / 2 } : { x: x - edge, z: z + off, ry: -Math.PI / 2 };
    antennas.push({ id, x, z, scale, far, apron, base, from: best.from, spur, side, cab });
  }
  // each row runs from the spine out to the last antenna it serves
  const rows = ROWS.map((r) => {
    const xs = [SPINE_X, ...(reach[r] ?? [])];
    return rect(Math.min(...xs) - H, Math.max(...xs) + H, r - H, r + H);
  });
  const spine = rect(SPINE_X - H, SPINE_X + H, SPINE_END - H, FENCE_Z - 0.15);
  // an apron that only just reaches into a track would leave a seam too narrow to cross
  // (S-04 by the spine): it stops at the track's edge instead, and its spur joins them
  for (const a of antennas) for (const t of [spine, ...rows]) {
    const dx = Math.min(a.apron.maxX, t.maxX) - Math.max(a.apron.minX, t.minX), dz = Math.min(a.apron.maxZ, t.maxZ) - Math.max(a.apron.minZ, t.minZ);
    if (dx <= 0 || dz <= 0 || (dx >= 1 && dz >= 1)) continue;
    if (dx < 1) { if (t.maxX < a.x) a.apron.minX = t.maxX; else a.apron.maxX = t.minX; }
    else if (t.maxZ < a.z) a.apron.minZ = t.maxZ; else a.apron.maxZ = t.minZ;
  }
  return { antennas, rows, spine };
}

const P = plan();
/** All 27, near and far, in the order Exterior.ts builds them. */
export const ANTENNAS: readonly Antenna[] = P.antennas;
export const ROW_RECTS: readonly Rect[] = P.rows;
export const SPINE: Rect = P.spine;
/** The gate's doorway (enabled while it is open): it overlaps the windows side (Grounds.ts,
 *  gNorth, from z -19.55) and the spine (to z -20.15) by more than the player's width. */
export const GATE_WAY: Rect = rect(GATE.x0 + 0.05, GATE.x1 - 0.05, FENCE_Z - 1.4, FENCE_Z + 1.3);
/** Every walk zone of the field but the gate's doorway: the spine, the rows, the spurs, the aprons. */
export const FIELD_ZONES: readonly { id: string; r: Rect }[] = [
  { id: 'fSpine', r: SPINE },
  ...ROW_RECTS.map((r, i) => ({ id: `fRow${i + 1}`, r })),
  ...ANTENNAS.flatMap((a) => [{ id: `fApron:${a.id}`, r: a.apron }, ...(a.spur ? [{ id: `fSpur:${a.id}`, r: a.spur }] : [])]),
];
/** The whole field, walked or not: the ground is flat out to here and a little beyond. */
export const FIELD_BOUNDS: Rect = FIELD_ZONES.reduce((b, { r }) => rect(Math.min(b.minX, r.minX), Math.max(b.maxX, r.maxX), Math.min(b.minZ, r.minZ), Math.max(b.maxZ, r.maxZ)),
  rect(Infinity, -Infinity, Infinity, -Infinity));
/** The array is graded flat (y -0.62) over this, and the rolling land eases in past it (Exterior.ts):
 *  wide enough that the ground's coarse cells near its edge never reach a track. */
export const ARRAY_FLAT: Rect = rect(FIELD_BOUNDS.minX - 60, FIELD_BOUNDS.maxX + 60, FIELD_BOUNDS.minZ - 95, 0);
/** Where the ground's height is the array's (Grounds.floorAt): the flat ground north of the fence,
 *  the gate's doorway included, and well inside where the land starts to roll. */
export const ARRAY_GROUND: Rect = rect(ARRAY_FLAT.minX + 40, ARRAY_FLAT.maxX - 40, ARRAY_FLAT.minZ + 40, FENCE_Z + 0.5);

export const inRect = (r: Rect, x: number, z: number, m = 0) => x >= r.minX - m && x <= r.maxX + m && z >= r.minZ - m && z <= r.maxZ + m;
/** On the field's gravel (or its aprons), within m of it. */
export const onField = (x: number, z: number, m = 0) => z < FENCE_Z && FIELD_ZONES.some(({ r }) => inRect(r, x, z, m));
/** The antenna nearest a point, and how far its centre is. */
export function nearestAntenna(x: number, z: number) {
  let best = ANTENNAS[0], d = Infinity;
  for (const a of ANTENNAS) { const k = Math.hypot(a.x - x, a.z - z); if (k < d) { d = k; best = a; } }
  return { a: best, d };
}
