import * as THREE from 'three';
import type { Box2, DriveArea, Obstacle, Surface } from './Drive';
import type { Collider } from '../world/ControlRoom';
import type { FloodSet } from '../world/kit';
import { track, trackNearest, surfaceY, HWY, DINER } from './roadTerrain';
import { STATION_TRACK, STATION_PAD } from '../world/stationLayout';
import { SARO_IN_ROAD, STATION_TURN, STATION_PATCH, roadToStation, stationToRoad, stationBend, STATION_DY, inRect } from '../world/geo';
import { SARO_FROM } from './corridors';

// The drives are driven the whole way (Tom, 5 October: no cuts when the truck goes from one
// place to another). Each place is still its own area far from the others in the scene;
// where two of them meet on a road, the truck is handed from one to the next between two
// frames, at a spot where both show the same road, with its speed, its wheel and the
// driver's head as they were. This file has the drivable ground of SARO and STATION 01 (the
// road and the old road have their own), and the hand-overs between the areas.
//
//   saro      the pad, the ramp, the service road and the highway past SARO
//   road      the highway south and the survey track (RoadArea.ts)
//   station01 the track's last stretch and the pad by STATION 01's gate
//
// Each area is built around its own origin; a way out is checked in that area's terms and
// gives the truck's pose in the next one, in world terms.

export type LegId = 'saro' | 'road' | 'station01' | 'diner' | 'roswell';
export type Exit = { to: LegId; x: number; z: number; heading: number };
type Circle = { kind: 'circle'; x: number; z: number; r: number };
/** An area the truck can be driven in on a trip. */
export interface TripArea extends DriveArea {
  /** Where the truck is left when it arrives here (world). */
  park: Box2 | null;
  /** Waypoints for the autopilot (tests, the developer menu), towards a place (world). */
  routes: Partial<Record<LegId | 'park', THREE.Vector3[]>>;
}

export const ORIGINS: Record<LegId, THREE.Vector3> = {
  saro: new THREE.Vector3(0, 0, 0), road: new THREE.Vector3(8000, 0, 0), station01: new THREE.Vector3(0, 0, 8000),
  diner: new THREE.Vector3(8000, 0, 0), roswell: new THREE.Vector3(0, 0, -24000),
};

const S = (kind: Surface['kind'], grip: number, top: number, rough: number): Surface => ({ kind, grip, top, rough });
const SURF = { asphalt: S('asphalt', 1, 30, 0.05), yard: S('asphalt', 1, 14, 0.08), shoulder: S('gravel', 0.85, 22, 0.3), gravel: S('gravel', 0.75, 18, 0.4),
  dirt: S('dirt', 0.55, 10, 0.8) };
const inBox = (b: Box2, x: number, z: number) => x >= b.minX && x <= b.maxX && z >= b.minZ && z <= b.maxZ;
const box = (minX: number, maxX: number, minZ: number, maxZ: number): Box2 => ({ minX, maxX, minZ, maxZ });
const v3 = (x: number, z: number, o = ORIGINS.saro) => new THREE.Vector3(x + o.x, o.y, z + o.z);
const wrap = (a: number) => Math.atan2(Math.sin(a), Math.cos(a));

// ---------- SARO: the pad, the ramp, the service road, the highway ----------
// Site coordinates (SARO's origin is the world's). The highway runs north-south along
// x = -24 (8 m wide, a southbound lane west of the centre, northbound east); the service
// road comes down the ramp south of the pad, bends west and meets the highway at z 24.5.
export const SARO_HWY = { x: -24, half: 4, z0: -200, z1: 640 };
const SARO_ROAD_FROM = SARO_FROM - SARO_IN_ROAD.z;   // site z where the road's own stretch begins (corridors.ts)
const SARO_RAMP = { minX: 6.6, maxX: 13.0, top: 12.4, foot: 16.6 };
export const SARO_PAD = box(6.6, 13.0, 3.4, 12.4);
// where the truck may go: the pad, the ramp, the service road and its turning area, the
// junction and the highway; the rest of the site is walls, railings and other people's ground
const SARO_WAY: Box2[] = [
  SARO_PAD, box(6.6, 13.0, 12.4, 16.8), box(6.8, 12.8, 16.0, 21.5), box(5.3, 14.3, 15.9, 20.1),
  box(1.0, 12.8, 18.5, 27.6), box(-21.5, 6.5, 21.4, 27.6), box(-29.5, -17.0, 17.0, 34.0),
  box(SARO_HWY.x - 6.2, SARO_HWY.x + 4.2, SARO_HWY.z0, SARO_HWY.z1),
  // south of the motel, between the road's fences (as in the road area)
  box(SARO_HWY.x - HWY.fence + 0.6, SARO_HWY.x + HWY.fence - 0.6, 80, SARO_HWY.z1),
];
const onSaroRoad = (x: number, z: number) => {
  if (z > 22 && z < 27 && x > -20 && x < 5.8) return true;                    // the straight run west
  if (x > 7.3 && x < 12.3 && z > 15.9 && z < 20.5) return true;               // down from the ramp
  const d = Math.hypot(x - 5.8, z - 20.5);
  return x >= 5.8 && z >= 20.5 && d > 1.5 && d < 6.5;                          // the bend
};

export class SaroDrive implements TripArea {
  obstacles: Obstacle[];
  endZone = box(0, 0, 0, 0);    // arrivals are the trip's business (World.ts), not the controller's
  park = SARO_PAD;
  routes: TripArea['routes'];
  headlights?: { set: FloodSet; slots: number[] };
  /** colliders: SARO's, without the truck's own; floorAt: the site's ground (Grounds.ts);
   *  road: the posts, poles and rocks of the road's stretch south of the site (road-local). */
  constructor(colliders: Collider[], private floorAt: (x: number, z: number) => number, road: Circle[] = []) {
    // only what stands along the way matters to the truck
    const near = (c: Collider) => SARO_WAY.some((w) => c.maxX > w.minX - 4 && c.minX < w.maxX + 4 && c.maxZ > w.minZ - 4 && c.minZ < w.maxZ + 4);
    this.obstacles = colliders.filter(near).map((c) => ({ kind: 'box' as const, minX: c.minX, maxX: c.maxX, minZ: c.minZ, maxZ: c.maxZ }) as Obstacle);
    for (const c of road) this.obstacles.push({ ...c, x: c.x - SARO_IN_ROAD.x, z: c.z - SARO_IN_ROAD.z });
    // arcs of 6 m radius (the truck turns no tighter than about 5.3 m) round the posts at the
    // foot of the ramp and at the junction: centre, from and to angle (0 is +x, 90 is +z)
    const arc = (cx: number, cz: number, a0: number, a1: number): [number, number][] => {
      const pts: [number, number][] = [];
      for (let i = 0; i <= 9; i++) { const a = (a0 + (a1 - a0) * i / 9) * Math.PI / 180; pts.push([cx + Math.cos(a) * 6, cz + Math.sin(a) * 6]); }
      return pts;
    };
    const ramp: [number, number][] = [[9.0, 9], [9.4, 11.5], [10.2, 14], [11.0, 16.2], [11.3, 17.8]];
    const out: [number, number][] = [...ramp, ...arc(5.3, 18.5, 0, 90), [1, 24.5], [-6, 24.5], [-13, 24.5], ...arc(-19.8, 30.5, -90, -180).slice(0, -1),
      [-25.8, 31.5], [-25.8, 38], [-25.8, 46]];
    for (let z = 60; z <= SARO_HWY.z1 - 20; z += 20) out.push([-25.8, z]);
    const back: [number, number][] = [];
    for (let z = SARO_HWY.z1 - 20; z >= 46; z -= 20) back.push([-22.2, z]);
    back.push([-22.2, 38], [-22.2, 31.5], ...arc(-16.2, 30.5, 180, 270), [-10, 24.5], [-3, 24.5], [1, 24.5], ...arc(5.3, 18.5, 90, 0),
      ...[...ramp].reverse().slice(1, -1), [9.2, 10.5], [9.0, 8.4]);
    this.routes = { road: out.map(([x, z]) => v3(x, z)), park: back.map(([x, z]) => v3(x, z)) };
  }
  surface(x: number, z: number) {
    const ax = Math.abs(x - SARO_HWY.x);
    if (z > SARO_ROAD_FROM) return ax <= HWY.paved ? SURF.asphalt : ax <= HWY.shoulder ? SURF.shoulder : SURF.dirt;
    if (ax <= SARO_HWY.half + 0.6) return SURF.asphalt;
    if (inBox(SARO_PAD, x, z) || (x >= SARO_RAMP.minX && x <= SARO_RAMP.maxX && z >= SARO_RAMP.top && z <= SARO_RAMP.foot) || onSaroRoad(x, z)) return SURF.yard;
    return SURF.dirt;
  }
  height(x: number, z: number) {
    if (inBox(SARO_PAD, x, z) || (x >= SARO_RAMP.minX && x <= SARO_RAMP.maxX && z >= SARO_RAMP.top && z <= SARO_RAMP.foot)) return this.floorAt(x, z);
    // south of the site's own highway, the road's land (corridors.ts)
    if (z > SARO_ROAD_FROM) return surfaceY(x + SARO_IN_ROAD.x, z + SARO_IN_ROAD.z);
    if (Math.abs(x - SARO_HWY.x) <= SARO_HWY.half + 2) return -0.58;
    return -0.6;
  }
  blocked(x: number, z: number) { return !SARO_WAY.some((w) => inBox(w, x, z)); }
}

// ---------- STATION 01: the last of the track, and the pad by the gate ----------
const sq = (x: number) => x * x;
function trackAt(pts: [number, number][], x: number, z: number) {
  let best = Infinity, s = 0, acc = 0;
  for (let i = 0; i < pts.length - 1; i++) {
    const [ax, az] = pts[i], [bx, bz] = pts[i + 1], dx = bx - ax, dz = bz - az, L = Math.hypot(dx, dz);
    const t = Math.min(1, Math.max(0, ((x - ax) * dx + (z - az) * dz) / (L * L)));
    const d = sq(ax + dx * t - x) + sq(az + dz * t - z);
    if (d < best) { best = d; s = acc + L * t; }
    acc += L;
  }
  return { d: Math.sqrt(best), s };
}
export class StationDrive implements TripArea {
  obstacles: Obstacle[];
  endZone = box(0, 0, 0, 0);
  park: Box2;
  routes: TripArea['routes'];
  headlights?: { set: FloodSet; slots: number[] };
  private o = ORIGINS.station01;
  /** colliders: the station's, without the truck's own; ground: its height (Station01.groundAt);
   *  road: the big rocks by the road's stretch of track (road-local, corridors.ts). */
  constructor(colliders: Collider[], private ground: (x: number, z: number) => number, road: Circle[] = []) {
    const o = this.o;
    this.park = box(STATION_PAD.minX + o.x, STATION_PAD.maxX + o.x, STATION_PAD.minZ + o.z, STATION_PAD.maxZ + o.z);
    this.obstacles = colliders.filter((c) => c.maxZ > o.z + 20 || (c.maxX > o.x - 30 && c.minX < o.x - 8))
      .map((c) => ({ kind: 'box' as const, minX: c.minX, maxX: c.maxX, minZ: c.minZ, maxZ: c.maxZ }) as Obstacle);
    for (const c of road) { const [x, z] = roadToStation(c.x, c.z); this.obstacles.push({ ...c, x: x + o.x, z: z + o.z }); }
    const out: [number, number][] = [[-19.25, 36.5], ...STATION_TRACK.slice(1)];
    const back = [...STATION_TRACK].reverse().filter(([, z]) => z > 40);
    back.push([-19.3, 39], [-19.25, 33]);
    this.routes = { road: out.map(([x, z]) => v3(x, z, o)), park: back.map(([x, z]) => v3(x, z, o)) };
  }
  surface(x: number, z: number) {
    const lx = x - this.o.x, lz = z - this.o.z;
    if (inBox(STATION_PAD, lx, lz) || trackAt(STATION_TRACK, lx, lz).d <= 2.6) return SURF.gravel;
    return SURF.dirt;
  }
  // on the road's stretch (corridors.ts) the road's own surface, as the station draws it
  height(x: number, z: number) {
    const lx = x - this.o.x, lz = z - this.o.z;
    const [rx, rz] = stationToRoad(lx, lz);
    if (inRect(STATION_PATCH, rx, rz) || (trackNearest(rx, rz).d < 4 && lz > STATION_PAD.maxZ)) return this.o.y + surfaceY(rx, rz) + STATION_DY - stationBend(rx, rz);
    return this.ground(x, z);
  }
  // the truck stays out of the compound and near the track
  blocked(x: number, z: number) {
    const lx = x - this.o.x, lz = z - this.o.z;
    if (lz < 26.2) return true;
    return !inBox(STATION_PAD, lx, lz) && trackAt(STATION_TRACK, lx, lz).d > 14;
  }
}

// ---------- the hand-overs ----------
// SARO and the road share the highway; the station's terms are the road's turned a quarter
// round (world/geo.ts). The truck changes from the road to the station 330 m out from the
// gate (the road's track at arc length 330 of 605), and back again 8 m farther out; both
// areas draw the same track there (corridors.ts, from arc length 200).
export const HANDOFF = 330;
const QUARTER = STATION_TURN;
const toStation = roadToStation, toRoad = stationToRoad;

/** The way out the truck has just taken from an area, if any: where it goes on in the next. */
export function exitFrom(leg: LegId, x: number, z: number, heading: number): Exit | null {
  const o = ORIGINS[leg], lx = x - o.x, lz = z - o.z;
  if (leg === 'saro') {
    if (lz > 200 && Math.abs(lx - SARO_HWY.x) < 14) return { to: 'road', x: lx + SARO_IN_ROAD.x + ORIGINS.road.x, z: lz + SARO_IN_ROAD.z + ORIGINS.road.z, heading };
    return null;
  }
  if (leg === 'road') {
    if (lz < -110 && Math.abs(lx) < 14) return { to: 'saro', x: lx - SARO_IN_ROAD.x, z: lz - SARO_IN_ROAD.z, heading };
    const t = trackNearest(lx, lz);
    if (t.d < 14 && t.s >= HANDOFF) {
      const [sx, sz] = toStation(lx, lz);
      return { to: 'station01', x: sx + ORIGINS.station01.x, z: sz + ORIGINS.station01.z, heading: wrap(heading + QUARTER) };
    }
    return null;
  }
  if (leg === 'station01') {
    if (lz < 150) return null;
    const [rx, rz] = toRoad(lx, lz), t = trackNearest(rx, rz);
    if (t.d < 14 && t.s < HANDOFF - 8) return { to: 'road', x: rx + ORIGINS.road.x, z: rz + ORIGINS.road.z, heading: wrap(heading - QUARTER) };
    return null;
  }
  return null;
}

// The autopilot's way on the road (world), towards the next area: the southbound lane and
// the survey track out, the track and the northbound lane back.
const roadWays: Partial<Record<LegId, THREE.Vector3[]>> = {};
export function roadRoute(next: LegId): THREE.Vector3[] {
  const hit = roadWays[next];
  if (hit) return hit;
  const trk = track.pts.filter((p, i) => i % 2 === 0 && p.x < -6).map((p) => [p.x, p.y] as [number, number]);
  const pts: [number, number][] = [];
  if (next === 'station01') {
    for (let z = -140; z <= 510; z += 20) pts.push([-1.8, z]);
    pts.push([-2.6, 513], [-4.2, 518.5], ...trk);
  } else if (next === 'diner') {
    // south past the track, then right onto the diner's lot, south of the rig and the sign
    for (let z = -140; z <= DINER.z - 20; z += 20) pts.push([-1.8, z]);
    // (left facing the diner's door, clear of where the player gets out: diner x 11, z 6)
    pts.push([-2.0, DINER.z - 10], [-2.6, DINER.z - 4], [-3.8, DINER.z], [-5.6, DINER.z + 3.6], [-7.5, DINER.z + 5.4], [-9.0, DINER.z + 6]);
  } else if (next === 'saro') {
    pts.push(...[...trk].reverse(), [-4, 519.6], [-0.6, 517.4], [1.6, 511]);
    for (let z = 495; z >= -160; z -= 20) pts.push([1.8, z]);
  }
  return (roadWays[next] = pts.map(([x, z]) => v3(x, z, ORIGINS.road)));
}

/** The leg a place is in: the diner stands in the road area (World.ts), the others are their own. */
export const legOf = (p: LegId): LegId => p === 'diner' ? 'road' : p;
/** The legs a trip from one place to another passes through, in order. */
export function legsBetween(from: LegId, to: LegId): LegId[] {
  const chain: LegId[] = ['saro', 'road', 'station01'];
  const a = chain.indexOf(legOf(from)), b = chain.indexOf(legOf(to));
  if (a < 0 || b < 0) return [from, to];
  return a <= b ? chain.slice(a, b + 1) : chain.slice(b, a + 1).reverse();
}
