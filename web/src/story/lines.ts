import { stationToRoad, roadToStation, SARO_IN_ROAD, MOTEL_AT } from '../world/geo';
import { HERD, DINER, TOWN, CROSS_MI } from '../drive/roadShape';
import { DEPUTY_CAR } from '../drive/roadTerrain';
import { STATION_TRANSIT, STATION_STAKES_C } from '../world/stationLayout';
import { YARD } from '../world/ServiceYard';
import { DISH_LAYOUT, DISH_LAYOUT_FAR, FENCE_Z } from '../world/arrayLayout';

// Where the 1947 lines A, B and C cross the ground (HENDELSER.md, section 3): the one place that
// says so. Everything that asks which line a place is on, or which way a line runs there, asks
// here: the time away (story/abduction.ts, main.ts), the lights along B (story/lineLights.ts), the
// marks and the lines drawn in on the notebook's map (story/townEvents.ts, ui/areaMap.ts), and the
// log of what was seen (story/seen.ts).
//
// The map is the road's terms (world/geo.ts, drive/roadShape.ts): road-local metres, x east,
// z south, the highway's centre at x 0, SARO's site 300 m north of the road's start. Each area
// is in its own terms and is turned onto the map here (toMap, fromMap, dirIn).
//
// All three are sight lines from the transit at STATION 01 (the pier, world/stationLayout.ts):
//
//   A  from the transit through Caliche: across the diner's lot and the payphone at the gas
//      station, over the main street at the sheriff's substation, and out at the south end of
//      town on the east side. It runs down the street at about 21 degrees to it, south to north
//      (the street lamps go out along it, south to north). Beyond the town, the highway north of
//      it (the cutout, the deputy's car) is still its ground.
//   B  from the transit to B-12's vane in SARO's service yard (B-12 keeps STATION 01's survey:
//      chapter two's lineage card), on through the east half of the antenna field. It crosses
//      the highway at SARO's turn-off (23 m south of the site), runs 210 to 270 m west of the
//      highway 280 to 330 m south of SARO (where the three lights keep pace with the truck and
//      turn off along it), and goes on south-west to STATION 01 through the pasture's west end.
//      It is SARO's line: the yard, the field, the shoulder down to the motel, the motel and the
//      herd are all within its reach (520 m), the motel 400 m from it, the herd 500.
//   C  the closing line. Its ground is in two places, each in its own area's terms: the old
//      stakes east from the transit at STATION 01 (the station's ground is C's, where the three
//      meet), and the old Roswell road, where C is the line z 0 of that area and crosses the
//      road just past the eight-mile post (drive/oldRoadLayout.ts, C_LINE). Between them the map
//      is not to scale (memory.md, 5 October), and the notebook's map never draws C.
//
// A and B leave the transit 23 and 9 degrees off the ways the station's own footing and vane
// point (STATION_SIGHT_A, STATION_VANE_B): the station was built in its own terms, turned a
// quarter round onto the map, and nothing in the game sights from it to the town or to SARO.

export type LineId = 'A' | 'B' | 'C';
/** A point or a direction on the map: road-local metres, x east, z south. */
export type MapPt = readonly [number, number];
/** The areas (World.area): each has its own terms. */
export type LineArea = 'saro' | 'road' | 'diner' | 'station01' | 'roswell' | 'room6' | string;

const unit = (x: number, z: number): MapPt => { const l = Math.hypot(x, z); return [x / l, z / l]; };
const site = (x: number, z: number): MapPt => [x + SARO_IN_ROAD.x, z + SARO_IN_ROAD.z];

/** The transit's pier at STATION 01, on the map. */
export const TRANSIT: MapPt = stationToRoad(STATION_TRANSIT.x, STATION_TRANSIT.z);
/** B-12's vane in SARO's service yard (world/ServiceYard.ts), on the map. */
export const B12: MapPt = site(YARD.vane.x, YARD.vane.z);

/** Where the night's marked events happen, on the map (the marks on the notebook's map are on
 *  the line their place is on; story/townEvents.ts, main.ts mapMarks):
 *    phone, phone2  the payphone at the gas station (story/townEvents.ts)
 *    repeat         the attendant's night window, beside it (story/caliche.ts)
 *    lamps          the main street's lamps, the middle of the row (world/Town.ts LAMPS)
 *    boy            1420 AM, the Night Line's studio (world/Town.ts)
 *    regular        the last stool at Mesa Diner's counter (story/Chapter5.ts)
 *    cutout         the cardboard alien on the west shoulder north of town (world/Town.ts cutoutAt)
 *    car            the deputy's car on the east shoulder (drive/roadTerrain.ts DEPUTY_CAR)
 *    lights         where the three lights turn off along B (story/lineLights.ts)
 *    cows, cows2    the herd in the pasture (drive/roadShape.ts HERD)
 *    tv             the office of Sierra Motor Court (world/MotelFront.ts)
 *    truck4, truck7 the truck on B, where B crosses the highway at SARO's turn-off (story/truckOnB.ts;
 *                   set below, once B is known)
 *    truck7m        the truck's seven on the highway in front of Sierra Motor Court's lot, pulling out
 *                   of it after room 6 (story/truckOnB.ts; the motel is B's ground, 400 m off it) */
export const PLACES: Record<string, MapPt> = {
  phone: [-15.5, 2064.6], phone2: [-15.5, 2064.6], repeat: [-13.8, 2071],
  lamps: [-7, 2146], boy: [32, 2036], regular: [DINER.x - 4.8, DINER.z],
  cutout: [-6.4, 1450], car: [DEPUTY_CAR.x, DEPUTY_CAR.z],
  lights: [-210, 30], cows: [HERD.x, HERD.z], cows2: [HERD.x, HERD.z],
  tv: site(-41.1 + MOTEL_AT.x, 24.1 + MOTEL_AT.z),
  truck7m: [0, site(0, 11.25 + MOTEL_AT.z)[1]],
};

interface Straight {
  /** A point it goes through (the transit) and the way it runs from there, towards `to`. */
  through: MapPt; dir: MapPt; to: string;
  /** How far from it its ground reaches (metres): what lies nearer is on it. */
  reach: number;
}
/** A and B on the map. */
export const LINES: Record<'A' | 'B', Straight> = {
  A: { through: TRANSIT, dir: unit(PLACES.phone[0] - TRANSIT[0], PLACES.phone[1] - TRANSIT[1]), to: 'Caliche', reach: 300 },
  B: { through: TRANSIT, dir: unit(B12[0] - TRANSIT[0], B12[1] - TRANSIT[1]), to: 'B-12 at SARO', reach: 520 },
};
/** C, in the terms of the two areas where it is on the ground. */
export const C_GROUND = {
  /** STATION 01 (station-local): the stakes from the transit; within `own` metres of the transit
   *  the ground is C's, where the three lines meet. */
  station: { from: STATION_TRANSIT, dir: [STATION_STAKES_C.x, STATION_STAKES_C.z] as MapPt, own: 80 },
  /** The old Roswell road (its area's local terms): the line z 0, at the crossing CROSS_MI miles
   *  from the diner; within `reach` metres of it the road is on C. */
  oldRoad: { z: 0, mi: CROSS_MI, reach: 300 },
};

/** Signed distance of a map point from A or B (positive to the right as it runs towards `to`). */
export function offset(id: 'A' | 'B', x: number, z: number) {
  const L = LINES[id];
  return (x - L.through[0]) * -L.dir[1] + (z - L.through[1]) * L.dir[0];
}
/** Distance of a map point from A or B. */
export const distTo = (id: 'A' | 'B', x: number, z: number) => Math.abs(offset(id, x, z));
/** Metres along A or B from the transit (positive towards `to`). */
export function alongFrom(id: 'A' | 'B', x: number, z: number) {
  const L = LINES[id];
  return (x - L.through[0]) * L.dir[0] + (z - L.through[1]) * L.dir[1];
}
/** The point of A or B this far along it from the transit. */
export function pointAt(id: 'A' | 'B', s: number): MapPt {
  const L = LINES[id];
  return [L.through[0] + L.dir[0] * s, L.through[1] + L.dir[1] * s];
}
/** The nearest point of A or B to a map point. */
export const foot = (id: 'A' | 'B', x: number, z: number) => pointAt(id, alongFrom(id, x, z));

/** Where A or B crosses the highway's centre line (road x 0): road z. */
export function highwayZ(id: 'A' | 'B') {
  const L = LINES[id];
  return L.through[1] + (-L.through[0] / L.dir[0]) * L.dir[1];
}
// (the truck on B: the lights went out where B crosses the highway, story/truckOnB.ts)
PLACES.truck4 = PLACES.truck7 = [0, highwayZ('B')];
/** Metres south of SARO, as the notebook's map counts the highway (ui/areaMap.ts). */
export const mSouth = (roadZ: number) => roadZ - SARO_IN_ROAD.z;

/** Where a line enters and leaves a box on the map (minX, maxX, minZ, maxZ), or null. */
function clip(id: 'A' | 'B', b: { minX: number; maxX: number; minZ: number; maxZ: number }): [MapPt, MapPt] | null {
  const L = LINES[id];
  let t0 = -1e9, t1 = 1e9;
  for (const [p, d, lo, hi] of [[L.through[0], L.dir[0], b.minX, b.maxX], [L.through[1], L.dir[1], b.minZ, b.maxZ]] as const) {
    if (Math.abs(d) < 1e-9) { if (p < lo || p > hi) return null; continue; }
    const a = (lo - p) / d, c = (hi - p) / d;
    t0 = Math.max(t0, Math.min(a, c)); t1 = Math.min(t1, Math.max(a, c));
  }
  return t0 <= t1 ? [pointAt(id, t0), pointAt(id, t1)] : null;
}
const ALL_DISHES = [...DISH_LAYOUT, ...DISH_LAYOUT_FAR];
/** The antenna field on the map: from the array fence north to the last antennas. */
const FIELD = (() => {
  const xs = ALL_DISHES.map((d) => d[1]), zs = ALL_DISHES.map((d) => d[2]);
  const [x0, z0] = site(Math.min(...xs) - 8, Math.min(...zs) - 8), [x1, z1] = site(Math.max(...xs) + 8, FENCE_Z);
  return { minX: x0, maxX: x1, minZ: z0, maxZ: z1 };
})();

/** Where each line meets the places the next steps of the night need (FORSLAG.md, 20 October,
 *  the car on B, taken in the car, the last mile): computed once. */
export const CROSSINGS = {
  A: {
    /** road z where A crosses the highway (in Caliche, at the sheriff's substation), and in metres south of SARO */
    highway: highwayZ('A'), highwayM: mSouth(highwayZ('A')),
    /** where A enters and leaves Caliche's strip (TOWN, road-local) */
    town: clip('A', { minX: TOWN.west, maxX: TOWN.east, minZ: TOWN.z0, maxZ: TOWN.z1 }),
  },
  B: {
    /** road z where B crosses the highway (SARO's turn-off), and in metres south of SARO */
    highway: highwayZ('B'), highwayM: mSouth(highwayZ('B')),
    /** where B enters the antenna field at the array fence and leaves it (map terms) */
    field: clip('B', FIELD),
    /** the antennas B passes nearest (within 45 m), from the fence out */
    dishes: ALL_DISHES.map(([id, x, z]) => ({ id, d: distTo('B', ...site(x, z)), s: alongFrom('B', ...site(x, z)) }))
      .filter((a) => a.d < 45).sort((a, b) => a.s - b.s).map((a) => a.id),
  },
  C: {
    /** miles from the diner where C crosses the old road (THE EVENT, 05:29) */
    oldRoad: CROSS_MI,
  },
};

// ---------- the areas' own terms ----------
/** A point of an area, in its own local terms (SARO's site; the road's for the road and the diner;
 *  the station's), on the map; room 6 is the motel's room 6; the old road has no place on the map. */
export function toMap(area: LineArea, x: number, z: number): MapPt | null {
  switch (area) {
    case 'saro': return site(x, z);
    case 'road': case 'diner': return [x, z];
    case 'station01': return stationToRoad(x, z);
    case 'room6': return site(-41.07 + MOTEL_AT.x, 55 + MOTEL_AT.z);
    default: return null;
  }
}
/** A map point in an area's own local terms (null where the area has no place on the map). */
export function fromMap(area: LineArea, x: number, z: number): MapPt | null {
  switch (area) {
    case 'saro': return [x - SARO_IN_ROAD.x, z - SARO_IN_ROAD.z];
    case 'road': case 'diner': return [x, z];
    case 'station01': return roadToStation(x, z);
    default: return null;
  }
}
/** A direction on the map in an area's own terms (the station's are a quarter turn round). */
export function dirIn(area: LineArea, d: MapPt): MapPt {
  return area === 'station01' ? [-d[1], d[0]] : d;
}

/** On a line: which, how far from it, and which way it runs there in the area's own terms. */
export interface OnLine { line: LineId; dist: number; along: MapPt }
/** Which line's ground a point of an area is on (its local terms), or null between the lines.
 *  The station's own ground is C's; the old road near the crossing is C's; elsewhere the nearer
 *  of A and B within its reach. */
export function lineAt(area: LineArea, x: number, z: number): OnLine | null {
  if (area === 'station01') {
    const c = C_GROUND.station, t = c.from;
    if (Math.hypot(x - t.x, z - t.z) <= c.own) return { line: 'C', dist: Math.abs((x - t.x) * -c.dir[1] + (z - t.z) * c.dir[0]), along: c.dir };
  }
  if (area === 'roswell') {
    const d = Math.abs(z - C_GROUND.oldRoad.z);
    return d <= C_GROUND.oldRoad.reach ? { line: 'C', dist: d, along: [1, 0] } : null;
  }
  const m = toMap(area, x, z);
  if (!m) return null;
  let best: OnLine | null = null;
  for (const id of ['A', 'B'] as const) {
    const d = distTo(id, m[0], m[1]);
    if (d <= LINES[id].reach && (!best || d < best.dist)) best = { line: id, dist: d, along: dirIn(area, LINES[id].dir) };
  }
  return best;
}
/** The line a marked event's place is on (PLACES); unknown kinds are the town's, on A. */
export function lineOfMark(kind: string): LineId {
  const p = PLACES[kind];
  return (p && lineAt('road', p[0], p[1])?.line) || 'A';
}
