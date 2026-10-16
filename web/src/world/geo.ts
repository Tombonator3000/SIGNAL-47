import * as THREE from 'three';
import { END, hLow, smooth, MOTEL } from '../drive/roadShape';
import { STATION_TRUCK, STATION_GROUND } from './stationLayout';

// How the places of the night lie on one map. Each area is built in its own terms far from
// the others in the scene (World.ts); the road's terms (drive/roadTerrain.ts) are the map:
//
//   SARO        road-local = site + (24, 0, -300): the site's highway is the road's
//   STATION 01  the road's parking spot by the gate (END) is the station's (STATION_TRUCK),
//               turned a quarter round: the survey track comes in from the south there
//
// Where the truck goes from one area into the next (drive/legs.ts), both areas draw the same
// stretch of road around the change, built once from the road's own builders and placed
// into the other area with fromRoad() (drive/corridors.ts). The far mesas are shared the
// same way (horizon.ts), so the skyline does not change either.

type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
export const SARO_IN_ROAD = { x: 24, z: -300 };
/** Sierra Motor Court in SARO's terms (Tom, 15 and 16 October: further from the stations). It
 *  was built across the road from SARO (world/MotelFront.ts keeps those numbers); it stands
 *  this far from there now: 650 m south down the highway and 2.5 m further west (clear of the
 *  road's wider shoulder), its lot 8 cm above the flat land round it (roadShape.MOTEL). */
export const MOTEL_AT = { x: -2.5, y: MOTEL.y + 0.08, z: 650 };
/** The motel's lot, walk and buildings (site). */
export const MOTEL_RECT: Rect = { minX: -50 + MOTEL_AT.x, maxX: -27.7 + MOTEL_AT.x, minZ: 1.5 + MOTEL_AT.z, maxZ: 70.8 + MOTEL_AT.z };
/** The motel's lamps as glows for far off (in the numbers it was built in, before MOTEL_AT):
 *  the sign's red and cyan, the lamp over each room door, the office window, the lot's two
 *  sodium lamps. Exterior.ts (from SARO), roadPlaces.saroLights (from the road and STATION 01). */
export const MOTEL_GLOWS: [number, number, number, number, number][] = [
  [-30.4, 5.8, 30.2, 12, 0xff4a35], [-29, 4, 32, 5, 0x50f0d8], [-40.95, 1.4, 26.2, 2.2, 0xffb866],
  ...[30, 35, 40, 45, 50, 55, 60, 65].map((z) => [-40.8, 2.15, z, 2.2, 0xffc070] as [number, number, number, number, number]),
  [-29.55, 6.2, 18, 5, 0xffa050], [-29.55, 6.2, 47, 5, 0xffa050],
];
/** The station's terms are the road's turned by this (rotation.y), about END. */
export const STATION_TURN = STATION_TRUCK.heading - END.heading;
export const roadToStation = (x: number, z: number): [number, number] => [STATION_TRUCK.x - (z - END.z), STATION_TRUCK.z + (x - END.x)];
export const stationToRoad = (x: number, z: number): [number, number] => [END.x + (z - STATION_TRUCK.z), END.z - (x - STATION_TRUCK.x)];
/** Station height = road height + this: the road's land at END is the station's flat ground. */
export const STATION_DY = STATION_GROUND - hLow(END.x, END.z);

/** The pieces of the road's ground both areas draw, road-local: beside the highway south of
 *  SARO, and round the survey track's last 400 m up to the station's pad. */
export const SARO_PATCH: Rect = { minX: -120, maxX: 120, minZ: -200, maxZ: 600 };
export const STATION_PATCH: Rect = { minX: -592, maxX: -190, minZ: 430, maxZ: 610 };
export const inRect = (r: Rect, x: number, z: number, m = 0) => x >= r.minX + m && x <= r.maxX - m && z >= r.minZ + m && z <= r.maxZ - m;
/** How far (x, z) lies outside a rectangle; 0 inside. */
export const outside = (r: Rect, x: number, z: number) => Math.hypot(Math.max(r.minX - x, 0, x - r.maxX), Math.max(r.minZ - z, 0, z - r.maxZ));
/** The road's land rises a little away from the gate; the station's ground round the hut and
 *  the pad is flat. In the station's copy of the road the land is brought down to it over the
 *  last 40 m of the track, and only there (road-local x, z; metres to take off). */
export const stationBend = (x: number, z: number) =>
  (hLow(x, z) - hLow(END.x, END.z)) * (1 - smooth(-600, -560, x)) * (1 - smooth(60, 160, Math.hypot(x - END.x, z - END.z)));
/** The road's land at a road-local point, in the station's terms (bent as above). */
export const stationLand = (x: number, z: number) => hLow(x, z) + STATION_DY - stationBend(x, z);

/** Place an object built in the road's terms into SARO's or the station's (local) terms. */
export function fromRoad(o: THREE.Object3D, area: 'saro' | 'station01') {
  if (area === 'saro') { o.position.set(-SARO_IN_ROAD.x, 0, -SARO_IN_ROAD.z); o.rotation.y = 0; return o; }
  const [x, z] = roadToStation(0, 0);
  o.position.set(x, STATION_DY, z); o.rotation.y = STATION_TURN;
  return o;
}
/** Place an object built in the station's (local) terms into the road's or SARO's terms. */
export function fromStation(o: THREE.Object3D, area: 'road' | 'saro') {
  const [x, z] = stationToRoad(0, 0), sx = area === 'saro' ? -SARO_IN_ROAD.x : 0, sz = area === 'saro' ? -SARO_IN_ROAD.z : 0;
  o.position.set(x + sx, -STATION_DY, z + sz); o.rotation.y = -STATION_TURN;
  return o;
}
