import * as THREE from 'three';
import { Flora, ragged, type Rect } from './flora';
import type { FloodSet } from './kit';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { saroLand } from './Exterior';
import { RAISED, ROAD_W, PATH, roadLine, TRUCK_RAMP, STEPS } from './Grounds';
import { SHOULDER } from './Crossing';
import { FENCE_Z, GATE, onField } from './arrayLayout';
import { SARO_IN_ROAD, SARO_PATCH, STATION_PATCH, MOTEL_RECT, STATION_DY, inRect, roadToStation, stationToRoad, stationBend, fromRoad } from './geo';
import { HWY, TRK, OLD, GUARD, PASTURE, DINER, TOWN, MOTEL, hLow, surfaceY, trackNearest, trackHalf, oldNearest, onRanchRoad } from '../drive/roadTerrain';
import type { Obstacle } from '../drive/Drive';

// Where the desert's ground cover (flora.ts) grows in each area, and how thick: none on the roads,
// the tracks, the paths, the lots and the floors, thin at their edges, thick a metre or two out
// (where the grader leaves its windrow, in the ditches), along the fences and at the feet of
// walls, posts and poles, and only the low kinds where the player walks.
//
//   SARO        the site, the antenna field and the highway down to the motel and past it, one
//               patch in the road's terms (fromRoad): north of site z 80 the site's own rules,
//               south of it the road's, as the road area has them
//   the road    the highway from SARO to past Caliche, the survey track, the pasture, the diner's
//               and the town's surroundings (World adds their walls and walks when they are built)
//   STATION 01  round the station (its own rules, Station01.clear) and the last of the track (the
//               road's), one patch in the road's terms turned into the station's
//   the old road   its own terms (OldRoad.ts builds it)
//
// The same seed and the same rules give the same plants: where the truck goes from one area into
// the next, the cover beside it does not change.
//
// The edges of the ways are soft (Flora's seam and spill): in the seam just inside a track's,
// a lot's, a curb's or a sidewalk's edge (and the bare strip outside it) tiny grass and pebbles
// grow, never in a wheel track or the middle of a walk, and the gravel spills a little out onto
// the dirt beside it.

export const COVER_SEED = 1986;
const site = (x: number, z: number) => [x - SARO_IN_ROAD.x, z - SARO_IN_ROAD.z] as const;   // road-local to SARO's
const toRoadRect = (r: Rect): Rect => ({ minX: r.minX + SARO_IN_ROAD.x, maxX: r.maxX + SARO_IN_ROAD.x, minZ: r.minZ + SARO_IN_ROAD.z, maxZ: r.maxZ + SARO_IN_ROAD.z });
const within = (r: Rect, x: number, z: number, m = 0) => x >= r.minX - m && x <= r.maxX + m && z >= r.minZ - m && z <= r.maxZ + m;
function segDist(px: number, pz: number, ax: number, az: number, bx: number, bz: number) {
  const dx = bx - ax, dz = bz - az, l2 = dx * dx + dz * dz;
  const t = l2 > 0 ? Math.min(1, Math.max(0, ((px - ax) * dx + (pz - az) * dz) / l2)) : 0;
  return Math.hypot(px - ax - dx * t, pz - az - dz * t);
}
function lineDist(px: number, pz: number, pts: readonly { x: number; y: number }[]) {
  let d = Infinity;
  for (let i = 0; i < pts.length - 1; i++) d = Math.min(d, segDist(px, pz, pts[i].x, pts[i].y, pts[i + 1].x, pts[i + 1].y));
  return d;
}
/** A way's edge: none on it (to `on`), thin just off it (to `thin`), thick out to `ditch`; j
 *  (ragged, 0 to 1) takes the thin band and the thick one further out here and there, by up to
 *  1.2 m, so the edge of the cover is not a line. */
const edge = (d: number, on: number, thin: number, ditch: number, thick = 1.9, j = 0) => d < on ? 0 : d < thin + 1.2 * j ? 0.3 : d < ditch + 1.2 * j ? thick : 1;
/** Along a fence: none in its line, thick either side. */
const fence = (d: number) => d < 0.22 ? 0 : d < 1.9 ? 2.4 : 1;

// ---------- the road's land (road-local) ----------
const MOTEL_ROAD = toRoadRect(MOTEL_RECT);
const SHOULDER_ROAD = toRoadRect({ minX: SHOULDER.x0, maxX: SHOULDER.x1, minZ: SHOULDER.z0, maxZ: SHOULDER.z1 });
// the gaps in the highway's fences, as drive/roadProps.ts leaves them (the road's own chunk: not imported)
const DINER_GAP = [DINER.z - 40, TOWN.z1], EAST_GAP = [DINER.z - 18, TOWN.z1], MOTEL_GAP = [MOTEL.z - 46, MOTEL.z + 46];
// the town's sidewalks and the gas station's apron (world/Town.ts, ground and gasStation)
const PAVED: Rect[] = [[-17.6, -14.2, 2016, 2040], [-17.6, -14.2, 2090, 2250], [16.6, 20.2, 2024, 2200], [-17.6, -5.6, 2040, 2090]]
  .map(([minX, maxX, minZ, maxZ]) => ({ minX, maxX, minZ, maxZ }));
const inTown = (x: number, z: number) => x > TOWN.west - 8 && x < TOWN.east + 8 && z > DINER.z - 20 && z < TOWN.z1 + 20;
/** The cover on the road's land: the highway, its verges, ditches and fences, the survey track,
 *  the old road's first stretch, the motel, the pasture, the diner's lot, Caliche's street. */
export function roadCover(x: number, z: number) {
  const ax = Math.abs(x), j = ragged(x, z);
  let d = 1;
  if (z > HWY.z0 && z < HWY.z1) {
    if (ax < 6.3) return 0;                                  // the asphalt and its gravel shoulders
    d = ax < 6.9 + 1.4 * j ? 0.3 : ax < 15.6 + 1.2 * j ? 1 : ax < 18.6 ? 1.9 : 1;   // the verge, the ditch past it
    const town = inTown(x, z);
    if (!town && z > -1000 && z < 2900) {
      const fx = Math.abs(ax - HWY.fence);
      const gap = x > 0 ? z > EAST_GAP[0] && z < EAST_GAP[1] : (z > MOTEL_GAP[0] && z < MOTEL_GAP[1]) || (z > GUARD.z0 - 0.6 && z < GUARD.z1 + 0.6) || (z > DINER_GAP[0] && z < DINER_GAP[1]);
      if (!gap) d *= fence(fx);
    }
    if (town) for (const r of PAVED) if (within(r, x, z, 0.15)) return 0;
  }
  // the survey track: gravel, its soft edges, its ditches
  const t = trackNearest(x, z);
  if (t.d < 40) { const w = trackHalf(t.s); d = Math.min(d, 1) * edge(t.d, w + 0.5, w + TRK.soft, TRK.margin + 0.6, 1.9, j); if (d === 0) return 0; }
  if (x > GUARD.x0 - 0.6 && x < GUARD.x1 + 0.6 && z > GUARD.z0 - 0.6 && z < GUARD.z1 + 0.6) return 0;   // the cattle guard
  if (onRanchRoad(x, z)) return 0;                           // the ranch's road east of the highway (drive/roadProps.ts)
  // the old road's first stretch across from the diner, and its barricades
  const o = oldNearest(x, z);
  if (o.d < 30) { d = Math.min(d, 1) * edge(o.d, OLD.shoulder + 0.4, OLD.shoulder + 0.9, OLD.margin + 0.5, 1.9, j); if (d === 0) return 0; }
  // the pasture south of the motel: grazed grass; the fences round it
  if (x < -HWY.fence && x > PASTURE.west - 2 && z > PASTURE.north - 2 && z < PASTURE.south + 2) {
    d *= Math.min(fence(Math.abs(z - PASTURE.north)), fence(Math.abs(z - PASTURE.south)), fence(Math.abs(x - PASTURE.west))) * 1.25;
  }
  // Sierra Motor Court's lot, walk and rooms: none, thick round them
  if (within(MOTEL_ROAD, x, z, 0.3)) return 0;
  if (within(MOTEL_ROAD, x, z, 1.2 + 2 * j)) d *= 1.8;
  // the diner's building and lot, and the station and the ranch's yard far off (drawn as they
  // are seen from the road: their own areas, or nothing, have the rest)
  if (x > DINER.x - 9 && x < DINER.x + 17.5 && Math.abs(z - DINER.z) < 17) return 0;
  if (x > -672 && x < -598 && z > 500 && z < 566) return 0;
  if (x > 395 && x < 485 && z > 150 && z < 235) return 0;
  return d;
}
/** Tests: a way the story uses on the road's land at a point. */
function roadWays(x: number, z: number): string | null {
  if (Math.abs(x) < 6 && z > HWY.z0 && z < HWY.z1) return 'the highway';
  const t = trackNearest(x, z);
  if (t.d < trackHalf(t.s)) return 'the survey track';
  if (oldNearest(x, z).d < OLD.half) return 'the old road';
  if (onRanchRoad(x, z)) return "the ranch's road";
  if (within(MOTEL_ROAD, x, z)) return "the motel's lot";
  if (inTown(x, z)) for (const r of PAVED) if (within(r, x, z)) return 'a sidewalk in Caliche';
  if (x > DINER.x + 3 && x < DINER.x + 16.7 && Math.abs(z - DINER.z) < 16) return "the diner's lot";
  return null;
}
/** A seam's weeds only where the way's edge is near: on a way, there must be ground off every way
 *  0.55 m one way or another (where two ways meet, a lot opens on a road, a row crosses the
 *  highway, there is none: the wheels go there). */
const seamed = (v: number, ways: (x: number, z: number) => string | null, x: number, z: number) =>
  v > 0 && (!ways(x, z) || !ways(x + 0.55, z) || !ways(x - 0.55, z) || !ways(x, z + 0.55) || !ways(x, z - 0.55)) ? v : 0;
/** The seam of a way on the road's land (where roadCover is 0): the outer edge of the highway's
 *  gravel shoulder (not the asphalt's), the survey track's and the old road's edges, the motel's
 *  lot, the ends of the diner's lot, the edges and ends of Caliche's sidewalks and the gas
 *  station's apron. */
export function roadSeam(x: number, z: number) { return seamed(roadSeamAt(x, z), roadWays, x, z); }
function roadSeamAt(x: number, z: number) {
  if (x > -672 && x < -598 && z > 500 && z < 566) return 0;   // (the station's and the ranch's own)
  if (x > 395 && x < 485 && z > 150 && z < 235) return 0;
  const ax = Math.abs(x);
  // (none where a lot, the diner's or the gas station's apron opens on the highway: the truck turns in there)
  const mouth = ax < 7.2 && (within(MOTEL_ROAD, x, z, 1) || (x < 0 && x > DINER.x - 9 && Math.abs(z - DINER.z) < 18) || (inTown(x, z) && PAVED.some((r) => within(r, x, z, 1))));
  if (z > HWY.z0 && z < HWY.z1 && ax < 6.3) return ax > 5.5 && !mouth ? 0.8 : 0;
  if (inTown(x, z)) for (const r of PAVED) if (within(r, x, z, 0.15)) {
    const ex = Math.min(x - r.minX, r.maxX - x), ez = Math.min(z - r.minZ, r.maxZ - z);
    return (ex < 0.5 || ez < 0.5) && !mouth ? 0.9 : 0;
  }
  if (x > GUARD.x0 - 0.6 && x < GUARD.x1 + 0.6 && z > GUARD.z0 - 0.6 && z < GUARD.z1 + 0.6) return 0;
  const t = trackNearest(x, z);
  if (t.d < 40) { const w = trackHalf(t.s); if (t.d < w + 0.5) return t.d > w - 0.5 ? 0.8 : 0; }
  const o = oldNearest(x, z);
  if (o.d < OLD.shoulder + 0.4) return o.d > OLD.shoulder - 0.45 ? 0.8 : 0;
  if (within(MOTEL_ROAD, x, z, 0.3)) return within(MOTEL_ROAD, x, z, -0.5) || mouth ? 0 : 0.7;
  if (x > DINER.x - 9 && x < DINER.x + 17.5 && Math.abs(z - DINER.z) < 17) return x > DINER.x + 3 && Math.abs(z - DINER.z) > 15.5 && !mouth ? 0.7 : 0;
  return 0;
}
/** The gravel spilt onto the dirt beside the road's ways (more pebbles, small): 4 by a gravel
 *  way, 2 by a lot. */
export function roadSpill(x: number, z: number) {
  if (z > HWY.z0 && z < HWY.z1 && Math.abs(x) < 7) return 4;
  const t = trackNearest(x, z);
  if (t.d < 40 && t.d < trackHalf(t.s) + 1.1) return 4;
  if (oldNearest(x, z).d < OLD.shoulder + 1) return 4;
  if (within(MOTEL_ROAD, x, z, 0.9)) return 2;
  return 1;
}
/** The things beside the road that hold the truck, for the cover: the posts, the poles, the signs
 *  and the big rocks (not the long walls that keep it on the road). Road-local. */
export const postsOf = (obstacles: readonly Obstacle[], dx = 0, dz = 0): Rect[] => obstacles.flatMap((o) => o.kind === 'circle'
  ? [{ minX: o.x - o.r - dx, maxX: o.x + o.r - dx, minZ: o.z - o.r - dz, maxZ: o.z + o.r - dz }] : []);
/** World colliders and zones to a patch's terms (moved by -dx, -dz). */
const moved = (list: readonly (Collider | Zone)[], dx: number, dz: number): Rect[] => list.map((c) => ({ minX: c.minX - dx, maxX: c.maxX - dx, minZ: c.minZ - dz, maxZ: c.maxZ - dz }));

/** The road area's patch (in its group, road-local); the diner's and the town's walls and walks
 *  come when they are built (World.ensureDiner, Flora.add). */
export function roadFlora(set: FloodSet, posts: Rect[]) {
  return new Flora({ name: 'road', seed: COVER_SEED, set, ground: surfaceY, cover: roadCover, seam: roadSeam, spill: roadSpill, blocks: posts, walks: [SHOULDER_ROAD], ways: roadWays });
}

// ---------- SARO (site terms, via the road's) ----------
const ROAD_LINE = roadLine();
const RANCH: [number, number][] = [[-18.6, 34.5], [46.5, 34.5], [46.5, -19.9]];
const RANCH_PTS = RANCH.map(([x, y]) => ({ x, y }));
const PATH_PTS = PATH.map((p) => ({ x: p.x, y: p.y }));
/** The cover on SARO's own ground (site terms). */
export function siteCover(x: number, z: number) {
  for (const b of RAISED) if (within(b, x, z, 0.15)) return 0;
  if (x > TRUCK_RAMP.minX - 0.2 && x < TRUCK_RAMP.maxX + 0.2 && z > TRUCK_RAMP.top && z < TRUCK_RAMP.foot + 0.3) return 0;
  if (x > STEPS.x0 - 0.5 && x < STEPS.x1 + 0.2 && z > STEPS.z0 - 0.2 && z < STEPS.z1 + 0.2) return 0;
  const j = ragged(x, z);
  // the service road and its gravel, the gravel at the foot of the ramp, the worn path, the path
  // from the fire exit to the highway, the cable trench's covers
  const rd = lineDist(x, z, ROAD_LINE);
  let d = edge(rd, ROAD_W / 2 + 0.85, ROAD_W / 2 + 1.2, ROAD_W / 2 + 3, 1.9, j);
  if (d === 0 || (x > 5.2 && x < 14.4 && z > 15.8 && z < 20.2)) return 0;
  d *= edge(lineDist(x, z, PATH_PTS), 0.7, 0.9, 1.8, 1.4, j * 0.6);
  if (d === 0 || (x > -20.2 && x < -10.4 && Math.abs(z - 5.7) < 1.0) || (Math.abs(x + 1) < 0.5 && z < -6 && z > -19.6)) return 0;
  // the highway north through the array, from the site's own south end
  const hx = Math.abs(x + 24);
  if (hx < 4.6) return 0;
  if (hx < 6) d *= 0.35; else if (hx < 9) d *= 1.6;
  // the array's gravel: none on it, thin at its edge, thick a metre or two out
  if (z < FENCE_Z + 0.5 && onField(x, z, 3)) {
    if (onField(x, z, 0.25)) return 0;
    d *= onField(x, z, 0.5 + 1.3 * j) ? 0.35 : 1.8;
  }
  // the chain-link fence to the array (open at the highway and at the walk gate), the ranch fence
  if (x > -81 && x < 61) {
    const gap = (x > -28.6 && x < -19.4) || (x > GATE.x0 - 0.3 && x < GATE.x1 + 0.3);
    if (!gap) d *= fence(Math.abs(z - FENCE_Z));
  }
  d *= fence(lineDist(x, z, RANCH_PTS));
  return d;
}
/** The seam of a way on SARO's ground (where siteCover is 0): the service road's gravel at the
 *  asphalt's edge and the turning place's edges, the worn path's edges, the highway's edge through
 *  the array, the edges of the array's gravel. Not on the floors, the ramp, the steps or the
 *  trench's covers. */
export function siteSeam(x: number, z: number) { return seamed(siteSeamAt(x, z), siteWays, x, z); }
function siteSeamAt(x: number, z: number) {
  for (const b of RAISED) if (within(b, x, z, 0.15)) return 0;
  if (x > TRUCK_RAMP.minX - 0.2 && x < TRUCK_RAMP.maxX + 0.2 && z > TRUCK_RAMP.top && z < TRUCK_RAMP.foot + 0.3) return 0;
  if (x > STEPS.x0 - 0.5 && x < STEPS.x1 + 0.2 && z > STEPS.z0 - 0.2 && z < STEPS.z1 + 0.2) return 0;
  const rd = lineDist(x, z, ROAD_LINE);
  if (rd < ROAD_W / 2 + 0.85) return rd > ROAD_W / 2 + 0.3 ? 0.8 : 0;
  if (x > 5.2 && x < 14.4 && z > 15.8 && z < 20.2) return Math.min(x - 5.2, 14.4 - x, z - 15.8, 20.2 - z) < 0.5 ? 0.8 : 0;
  const pd = lineDist(x, z, PATH_PTS);
  if (pd < 0.7) return pd > 0.38 ? 0.7 : 0;
  if ((x > -20.2 && x < -10.4 && Math.abs(z - 5.7) < 1.0) || (Math.abs(x + 1) < 0.5 && z < -6 && z > -19.6)) return 0;
  const hx = Math.abs(x + 24);
  if (hx < 4.6) return hx > 3.9 ? 0.8 : 0;
  if (z < FENCE_Z + 0.5 && onField(x, z, 0.25)) return 0.8;
  return 0;
}
/** The gravel spilt onto the dirt beside SARO's ways (as roadSpill). */
export function siteSpill(x: number, z: number) {
  if (lineDist(x, z, ROAD_LINE) < ROAD_W / 2 + 1.5) return 4;
  if (z < FENCE_Z + 0.5 && onField(x, z, 0.85)) return 4;
  if (Math.abs(x + 24) < 5.4) return 2;
  return 1;
}
/** Tests: a way the story uses on SARO's ground (site terms). */
function siteWays(x: number, z: number): string | null {
  for (const b of RAISED) if (within(b, x, z)) return 'a floor of the house';
  if (lineDist(x, z, ROAD_LINE) < ROAD_W / 2) return 'the service road';
  if (lineDist(x, z, PATH_PTS) < 0.5) return 'the worn path';
  if (Math.abs(x + 24) < 4) return 'the highway';
  if (z < FENCE_Z && onField(x, z)) return "the array's gravel";
  return null;
}

/** SARO's patch: in the road's terms (its group placed with fromRoad), the site's rules north of
 *  site z 80 and the road's south of it, on the ground drawn there; SARO's colliders and walks on
 *  its own ground. World adds the posts along the highway's stretch when it is built (initSaro). */
export function saroFlora(set: FloodSet, colliders: readonly Collider[], zones: readonly Zone[]) {
  const own = (r: Rect) => r.minZ < 80;   // the site's own ground; south of it the road's rules, as the road area's
  const f = new Flora({
    name: 'saro', seed: COVER_SEED, set,
    ground: (x, z) => {
      if (z >= -200 && inRect(SARO_PATCH, x, z)) return surfaceY(x, z);
      const [sx, sz] = site(x, z);
      let y = saroLand(sx, sz);
      if (sz > 36 && Math.abs(x) < HWY.flat) y = Math.max(y, hLow(x, z));   // the verge of the road's stretch
      return y;
    },
    cover: (x, z) => { const [sx, sz] = site(x, z); return sz < 80 ? siteCover(sx, sz) : roadCover(x, z); },
    seam: (x, z) => { const [sx, sz] = site(x, z); return sz < 80 ? siteSeam(sx, sz) : roadSeam(x, z); },
    spill: (x, z) => { const [sx, sz] = site(x, z); return sz < 80 ? siteSpill(sx, sz) : roadSpill(x, z); },
    blocks: moved(colliders.filter(own), -SARO_IN_ROAD.x, -SARO_IN_ROAD.z),
    walks: [...moved(zones.filter(own), -SARO_IN_ROAD.x, -SARO_IN_ROAD.z), SHOULDER_ROAD],
    ways: (x, z) => { const [sx, sz] = site(x, z); return sz < 80 ? siteWays(sx, sz) : roadWays(x, z); },
  });
  fromRoad(f.group, 'saro');
  return f;
}

// ---------- STATION 01 (station terms, via the road's) ----------
/** A station rectangle (local) in the road's terms. */
function stationRect(r: Rect): Rect {
  const [ax, az] = stationToRoad(r.minX, r.minZ), [bx, bz] = stationToRoad(r.maxX, r.maxZ);
  return { minX: Math.min(ax, bx), maxX: Math.max(ax, bx), minZ: Math.min(az, bz), maxZ: Math.max(az, bz) };
}
export interface StationSite {
  clear(x: number, z: number, rad: number): boolean;
  groundAt(x: number, z: number): number;
  colliders: Collider[];
  zones: Zone[];
}
/** STATION 01's patch: in the road's terms turned into the station's (a child of its group, at its
 *  origin); round the station its own rules and ground, on the last of the track the road's.
 *  origin: the station's (its colliders and zones are in world terms). */
export function stationFlora(set: FloodSet, s: StationSite, origin: THREE.Vector3, posts: Rect[]) {
  const loc = (r: Rect): Rect => ({ minX: r.minX - origin.x, maxX: r.maxX - origin.x, minZ: r.minZ - origin.z, maxZ: r.maxZ - origin.z });
  const f = new Flora({
    name: 'station01', seed: COVER_SEED, set,
    ground: (x, z) => {
      if (inRect(STATION_PATCH, x, z)) return surfaceY(x, z) - stationBend(x, z);
      const [sx, sz] = roadToStation(x, z);
      return s.groundAt(sx + origin.x, sz + origin.z) - origin.y - STATION_DY;
    },
    cover: (x, z) => {
      if (inRect(STATION_PATCH, x, z)) return roadCover(x, z);
      const [sx, sz] = roadToStation(x, z);
      if (!s.clear(sx, sz, 0.15)) return 0;
      // thick along the fence, the paths and the track (just out of their way), raggedly
      return s.clear(sx, sz, 0.9 + 1.4 * ragged(x, z)) ? 1 : 1.9;
    },
    // in the edge of a path, the pad, the track: what stays a quarter of a metre clear of it
    seam: (x, z) => {
      if (inRect(STATION_PATCH, x, z)) return roadSeam(x, z);
      const [sx, sz] = roadToStation(x, z);
      return s.clear(sx, sz, -0.25) ? 0.7 : 0;
    },
    spill: (x, z) => {
      if (inRect(STATION_PATCH, x, z)) return roadSpill(x, z);
      const [sx, sz] = roadToStation(x, z);
      return s.clear(sx, sz, 0.75) ? 1 : 2;   // (pebbles by the paths, the pad and the fence)
    },
    blocks: [...posts, ...s.colliders.map((c) => stationRect(loc(c)))],
    walks: s.zones.map((z) => stationRect(loc(z))),
    ways: (x, z) => {
      if (inRect(STATION_PATCH, x, z)) return roadWays(x, z);
      const [sx, sz] = roadToStation(x, z);
      return s.clear(sx, sz, -0.3) ? null : 'a path, the pad or the fence at the station';
    },
  });
  fromRoad(f.group, 'station01');
  return f;
}
