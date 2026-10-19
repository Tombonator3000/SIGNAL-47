import * as THREE from 'three';
import { Flora, type Rect } from './flora';
import type { FloodSet } from './kit';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { saroLand } from './Exterior';
import { RAISED, ROAD_W, PATH, roadLine, TRUCK_RAMP, STEPS } from './Grounds';
import { SHOULDER } from './Crossing';
import { FENCE_Z, GATE, onField } from './arrayLayout';
import { SARO_IN_ROAD, SARO_PATCH, STATION_PATCH, MOTEL_RECT, STATION_DY, inRect, roadToStation, stationToRoad, stationBend, fromRoad } from './geo';
import { HWY, TRK, OLD, GUARD, PASTURE, DINER, TOWN, MOTEL, hLow, surfaceY, trackNearest, trackHalf, oldNearest } from '../drive/roadTerrain';
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
/** A way's edge: none on it (to `on`), thin just off it (to `thin`), thick out to `ditch`. */
const edge = (d: number, on: number, thin: number, ditch: number, thick = 1.9) => d < on ? 0 : d < thin ? 0.3 : d < ditch ? thick : 1;
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
  const ax = Math.abs(x);
  let d = 1;
  if (z > HWY.z0 && z < HWY.z1) {
    if (ax < 6.3) return 0;                                  // the asphalt and its gravel shoulders
    d = ax < 7.6 ? 0.3 : ax < 16.2 ? 1 : ax < 18.6 ? 1.9 : 1;   // the verge, the ditch past it
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
  if (t.d < 40) { const w = trackHalf(t.s); d = Math.min(d, 1) * edge(t.d, w + 0.5, w + TRK.soft + 0.5, TRK.margin + 0.6); if (d === 0) return 0; }
  if (x > GUARD.x0 - 0.6 && x < GUARD.x1 + 0.6 && z > GUARD.z0 - 0.6 && z < GUARD.z1 + 0.6) return 0;   // the cattle guard
  // the old road's first stretch across from the diner, and its barricades
  const o = oldNearest(x, z);
  if (o.d < 30) { d = Math.min(d, 1) * edge(o.d, OLD.shoulder + 0.4, OLD.shoulder + 1.4, OLD.margin + 0.5); if (d === 0) return 0; }
  // the pasture south of the motel: grazed grass; the fences round it
  if (x < -HWY.fence && x > PASTURE.west - 2 && z > PASTURE.north - 2 && z < PASTURE.south + 2) {
    d *= Math.min(fence(Math.abs(z - PASTURE.north)), fence(Math.abs(z - PASTURE.south)), fence(Math.abs(x - PASTURE.west))) * 1.25;
  }
  // Sierra Motor Court's lot, walk and rooms: none, thick round them
  if (within(MOTEL_ROAD, x, z, 0.3)) return 0;
  if (within(MOTEL_ROAD, x, z, 2.5)) d *= 1.8;
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
  if (within(MOTEL_ROAD, x, z)) return "the motel's lot";
  if (inTown(x, z)) for (const r of PAVED) if (within(r, x, z)) return 'a sidewalk in Caliche';
  if (x > DINER.x + 3 && x < DINER.x + 16.7 && Math.abs(z - DINER.z) < 16) return "the diner's lot";
  return null;
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
  return new Flora({ name: 'road', seed: COVER_SEED, set, ground: surfaceY, cover: roadCover, blocks: posts, walks: [SHOULDER_ROAD], ways: roadWays });
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
  // the service road and its gravel, the gravel at the foot of the ramp, the worn path, the path
  // from the fire exit to the highway, the cable trench's covers
  const rd = lineDist(x, z, ROAD_LINE);
  let d = edge(rd, ROAD_W / 2 + 0.85, ROAD_W / 2 + 1.5, ROAD_W / 2 + 3);
  if (d === 0 || (x > 5.2 && x < 14.4 && z > 15.8 && z < 20.2)) return 0;
  d *= edge(lineDist(x, z, PATH_PTS), 0.7, 1.0, 1.8, 1.4);
  if (d === 0 || (x > -20.2 && x < -10.4 && Math.abs(z - 5.7) < 1.0) || (Math.abs(x + 1) < 0.5 && z < -6 && z > -19.6)) return 0;
  // the highway north through the array, from the site's own south end
  const hx = Math.abs(x + 24);
  if (hx < 4.6) return 0;
  if (hx < 6) d *= 0.35; else if (hx < 9) d *= 1.6;
  // the array's gravel: none on it, thin at its edge, thick a metre or two out
  if (z < FENCE_Z + 0.5 && onField(x, z, 3)) {
    if (onField(x, z, 0.25)) return 0;
    d *= onField(x, z, 0.9) ? 0.35 : 1.8;
  }
  // the chain-link fence to the array (open at the highway and at the walk gate), the ranch fence
  if (x > -81 && x < 61) {
    const gap = (x > -28.6 && x < -19.4) || (x > GATE.x0 - 0.3 && x < GATE.x1 + 0.3);
    if (!gap) d *= fence(Math.abs(z - FENCE_Z));
  }
  d *= fence(lineDist(x, z, RANCH_PTS));
  return d;
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
      // thick along the fence, the paths and the track (just out of their way)
      return s.clear(sx, sz, 1.6) ? 1 : 1.9;
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
