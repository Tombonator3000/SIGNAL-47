import * as THREE from 'three';
import { END, hLow, smooth } from '../drive/roadShape';
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

export const SARO_IN_ROAD = { x: 24, z: -300 };
/** The station's terms are the road's turned by this (rotation.y), about END. */
export const STATION_TURN = STATION_TRUCK.heading - END.heading;
export const roadToStation = (x: number, z: number): [number, number] => [STATION_TRUCK.x - (z - END.z), STATION_TRUCK.z + (x - END.x)];
export const stationToRoad = (x: number, z: number): [number, number] => [END.x + (z - STATION_TRUCK.z), END.z - (x - STATION_TRUCK.x)];
/** Station height = road height + this: the road's land at END is the station's flat ground. */
export const STATION_DY = STATION_GROUND - hLow(END.x, END.z);

/** The pieces of the road's ground both areas draw, road-local: beside the highway south of
 *  SARO, and round the survey track's last 400 m up to the station's pad. */
type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
export const SARO_PATCH: Rect = { minX: -120, maxX: 120, minZ: -200, maxZ: 600 };
export const STATION_PATCH: Rect = { minX: -592, maxX: -190, minZ: 430, maxZ: 610 };
export const inRect = (r: Rect, x: number, z: number, m = 0) => x >= r.minX + m && x <= r.maxX - m && z >= r.minZ + m && z <= r.maxZ - m;
/** How far (x, z) lies outside a rectangle; 0 inside. */
export const outside = (r: Rect, x: number, z: number) => Math.hypot(Math.max(r.minX - x, 0, x - r.maxX), Math.max(r.minZ - z, 0, z - r.maxZ));
/** The road's land rises a little away from the gate; the station's ground round the hut and
 *  the pad is flat. In the station's copy of the road the land is brought down to it over the
 *  last 40 m (road-local x, z; metres to take off). */
export const stationBend = (x: number, z: number) => (hLow(x, z) - hLow(END.x, END.z)) * (1 - smooth(-600, -560, x));
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
