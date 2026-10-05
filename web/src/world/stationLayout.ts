// STATION 01's layout numbers that the drive needs without loading the station itself
// (drive/legs.ts): the track in from the south and the pad by the gate, station-local.

/** The track in from the south: the pad by the gate first, then out towards the road.
 *  It is the road's survey track (drive/roadTerrain.ts) from 186 m out from the highway to
 *  the gate, in the station's terms (world/geo.ts): the road's parking spot by the gate is
 *  STATION_TRUCK below. The truck is driven from one area into the other on the same gravel
 *  (drive/legs.ts), and the gravel itself is the road's (drive/corridors.ts). */
export const STATION_TRACK: [number, number][] = [[-19.25, 38.5], [-18.7, 46.4], [-18.3, 58.3], [-17.6, 70.3], [-16.5, 82.2], [-15.1, 94.1], [-13.5, 106],
  [-11.8, 117.9], [-10, 129.7], [-8.1, 141.5], [-6.1, 153.3], [-3.9, 165.1], [-1.7, 176.9], [0.5, 188.7], [2.5, 200.5], [4.4, 212.3], [6.2, 224.2],
  [7.9, 236], [9.6, 247.9], [11.2, 259.8], [12.7, 271.7], [14, 283.6], [15.1, 295.5], [15.9, 307.5], [16.6, 319.4], [17.2, 331.4], [17.5, 343.4],
  [17.7, 355.3], [17.6, 367.3], [17.4, 379.3], [16.9, 391.3], [16.2, 403.2], [15.1, 415.2], [13.7, 427.1], [12.2, 439], [11.7, 441.9]];
/** The fence, the vehicle gate in its south side, the hut, the generator shed and the pole of
 *  the yard light: the road's far view of the station is built from these too (roadPlaces.ts). */
export const STATION_FENCE = { x0: -30, x1: 30, z0: -24, z1: 26 };
export const STATION_GATE = { w: -21.6, e: -16.9 };
export const STATION_HUT = { x0: -13, x1: -9, z0: 1.5, z1: 4.5, t: 0.2, h: 2.6 };
export const STATION_SHED = { x0: -17.3, x1: -15.1, z0: -1.3, z1: 0.5, h: 2.05 };
export const STATION_POLE = { x: -6.6, z: 7.0 };
/** The station's flat ground round the hut and the pad (station-local y). */
export const STATION_GROUND = -0.15;
/** The gravel pad outside the gate, where the truck is left. */
export const STATION_PAD = { minX: -27.6, maxX: -12.2, minZ: 26.3, maxZ: 39.8 };
/** The truck as it stands on the pad (the night it first comes here). */
export const STATION_TRUCK = { x: -19.25, z: 31.4, heading: 0 };
