// The road area's land in closed form: no tables and no three.js, so the places beside the
// road (SARO's highway south of the site, the last of the survey track at STATION 01) can
// follow the same ground without loading the road. Road-local metres, as in roadTerrain.ts.

export const smooth = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
export const hash = (x: number, z: number) => { const s = Math.sin(x * 12.9898 + z * 78.233) * 43758.5453; return s - Math.floor(s); };

/** Where the drive used to start: the southbound lane, SARO 300 m behind. */
export const START = { x: -1.8, z: 0, heading: Math.PI };
/** The truck parked at the station gate, facing it. */
export const END = { x: -603, z: 532, heading: Math.PI / 2 };

const base = (x: number, z: number) => 1.1 * Math.sin(x * 0.0061 + 0.7) * Math.cos(z * 0.0047 + 0.3)
  + 0.7 * Math.sin((x + z) * 0.0032 + 1.9) + 0.4 * Math.sin(z * 0.011 - x * 0.004 + 0.4);
const B0 = base(START.x, START.z);
export const washAt = (x: number, z: number) => { const w = x + 330 + 0.25 * (z - 500); return Math.exp(-w * w / 520) * smooth(-150, 60, z); };
function raw(x: number, z: number) {
  const dx = x + 250, dz = z - 300, a = Math.atan2(dz, dx);
  const hills = smooth(1300, 2400, Math.hypot(dx, dz)) * (12 + 9 * Math.sin(a * 5 + 1.3) + 5 * Math.sin(a * 13));
  return base(x, z) - B0 - 1.3 * washAt(x, z) + hills;
}
/** Mesa Diner stands beside the highway 2 km south of the start (world/Diner.ts, placed by
 *  World.ts): its own origin is here, its x 20 the highway's centre, and the land round its
 *  lot is flat at DINER.y. */
// (x and z sit on whole 0.15 m steps from where the diner stood before, so tools/reachcheck.py
// samples it as it always has)
export const DINER = { x: -20.05, z: 1999.95, flatX: -11.05, y: raw(-11.05, 1999.95) };
/** The graded land the roads follow: long swells, a dry wash across the track, hills far out,
 *  and flat round the diner. */
export function hLow(x: number, z: number) {
  const r = raw(x, z), d = Math.hypot(x - DINER.flatX, z - DINER.z);
  return d > 110 ? r : r + (DINER.y - r) * (1 - smooth(45, 110, d));
}

// ---------- the old Roswell road (drive/oldRoadLayout.ts) ----------
export const MILE = 1609.34;
/** Where C crosses the old road, in miles from the diner. */
export const CROSS_MI = 8.15;
/** The old road's heading in degrees from north (clockwise), by miles from the diner, where it
 *  leaves the highway across the road from Mesa Diner (as on SARO's service map, E14). */
export function oldRoadBearing(mi: number) {
  const b = (a: number, c: number, m0: number, m1: number) => a + (c - a) * smooth(m0, m1, mi);
  if (mi < 5.6) {
    // off the highway due east, round into the east-north-east, then long easy bends
    const lead = 63 + 27 * (1 - smooth(0.02, 0.3, mi)), m = mi - 0.3;
    return lead + (8 * Math.sin(m * 2.3) + 4 * Math.sin(m * 5.1 + 1)) * smooth(0.3, 1.0, mi) * (1 - smooth(4.6, 5.5, mi));
  }
  if (mi < 6.75) return b(63, 45, 6.25, 6.75);
  if (mi < 7.75) return b(45, 35, 7.25, 7.75);
  return b(35, 27, 8.5, 8.9);
}
