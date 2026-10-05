import * as THREE from 'three';
import { smooth, hash } from './roadTerrain';

// The old Roswell road, chapter six (KAPITLER.md, Roswell Road). Laid out as on SARO's
// service map (E14, story/drawings.ts): from the diner east-north-east, then a long bend
// to the north-east. C runs due east from STATION 01 and crosses the road 8.15 miles from
// the diner, just past the eight-mile post. Local metres, origin where C crosses the road;
// x is east, north is -z. C is the line z = 0.
//
// The road is a path of points every 4 m, s = 0 at the crossing, negative behind it.
// Distances along the road are real: a mile is 1609 m.

export const MILE = 1609.34;
export const CROSS_MI = 8.15;
/** Miles from the diner at a distance s along the road. */
export const miAt = (s: number) => CROSS_MI + s / MILE;
export const sAt = (mi: number) => (mi - CROSS_MI) * MILE;

/** The heading of the road in degrees from north (clockwise), by miles from the diner. */
export function bearing(mi: number) {
  const b = (a: number, c: number, m0: number, m1: number) => a + (c - a) * smooth(m0, m1, mi);
  if (mi < 6.75) return b(63, 45, 6.25, 6.75);
  if (mi < 7.75) return b(45, 35, 7.25, 7.75);
  return b(35, 27, 8.5, 8.9);
}

export const S0 = sAt(5.9) - 330;   // the road is built from a little behind the start
export const S1 = 1650;             // to well past the crossing
export const STEP = 4;
export const HALF = 3.2, SHOULDER = 4.4, FENCE = 17, POWER = -28;

export type RoadPt = { x: number; z: number; tx: number; tz: number; s: number; y: number };

// ---------- the land: long low swells, hills rising far out ----------
export function terrain(x: number, z: number) {
  const r = Math.hypot(x + 800, z - 800);
  const hills = smooth(2600, 5200, r) * (22 + 14 * Math.sin(Math.atan2(z, x) * 4 + 0.8));
  return 2.2 * Math.sin(x * 0.0021 + 0.4) * Math.cos(z * 0.0017 + 1.1) + 1.4 * Math.sin((x - z) * 0.0011 + 2.0)
    + 0.6 * Math.sin(x * 0.0071 - z * 0.0043) + hills;
}
const detail = (x: number, z: number) => 0.18 * Math.sin(x * 0.13 + 1.7) * Math.sin(z * 0.11 + 0.3) + 0.09 * Math.sin(x * 0.31 - z * 0.23 + 0.9);

// ---------- the path ----------
export const path: RoadPt[] = (() => {
  const fwd: RoadPt[] = [], back: RoadPt[] = [];
  const at = (s: number, x: number, z: number) => {
    const a = bearing(miAt(s)) * Math.PI / 180;
    return { x, z, tx: Math.sin(a), tz: -Math.cos(a), s, y: 0 };
  };
  let p = at(0, 0, 0);
  fwd.push(p);
  for (let s = STEP; s <= S1; s += STEP) {
    const a = bearing(miAt(s - STEP / 2)) * Math.PI / 180;
    p = at(s, p.x + Math.sin(a) * STEP, p.z - Math.cos(a) * STEP); fwd.push(p);
  }
  p = fwd[0];
  for (let s = -STEP; s >= S0; s -= STEP) {
    const a = bearing(miAt(s + STEP / 2)) * Math.PI / 180;
    p = at(s, p.x - Math.sin(a) * STEP, p.z + Math.cos(a) * STEP); back.push(p);
  }
  const all = [...back.reverse(), ...fwd];
  // the road is graded: its height is the land under it, smoothed over about 120 m
  const raw = all.map((q) => terrain(q.x, q.z));
  const W = 15;
  all.forEach((q, i) => {
    let sum = 0, n = 0;
    for (let k = -W; k <= W; k++) { const j = i + k; if (j >= 0 && j < raw.length) { sum += raw[j]; n++; } }
    q.y = sum / n;
  });
  return all;
})();
export const I0 = Math.round(-S0 / STEP);   // index of the crossing in path

/** The point of the road at a distance s, interpolated. */
export function roadAt(s: number): RoadPt {
  const f = THREE.MathUtils.clamp((s - S0) / STEP, 0, path.length - 1.001), i = Math.floor(f), k = f - i;
  const a = path[i], b = path[i + 1];
  return { x: a.x + (b.x - a.x) * k, z: a.z + (b.z - a.z) * k, tx: a.tx + (b.tx - a.tx) * k, tz: a.tz + (b.tz - a.tz) * k, s, y: a.y + (b.y - a.y) * k };
}
/** A point beside the road: o metres to the right of travel (negative is left). */
export function beside(s: number, o: number) {
  const p = roadAt(s);
  return { x: p.x - p.tz * o, z: p.z + p.tx * o, p };
}

// ---------- nearest point on the road, through a coarse grid of path segments ----------
const CELL = 100;
const cells = new Map<number, number[]>();
const key = (cx: number, cz: number) => (cx + 1000) * 4000 + (cz + 1000);
for (let i = 0; i < path.length - 1; i++) {
  const a = path[i], b = path[i + 1];
  for (const q of [a, b]) {
    const k = key(Math.floor(q.x / CELL), Math.floor(q.z / CELL));
    const l = cells.get(k); if (l) { if (l[l.length - 1] !== i) l.push(i); } else cells.set(k, [i]);
  }
}
const near = { d: Infinity, s: 0, o: 0, y: 0 };
/** Distance to the road's centreline (o is signed, right of travel positive), s and grade there.
 *  Far from the road (more than about 150 m) d is Infinity. Reuses one result object. */
export function nearest(x: number, z: number) {
  near.d = Infinity; near.s = 0; near.o = 0; near.y = 0;
  const cx = Math.floor(x / CELL), cz = Math.floor(z / CELL);
  let best = Infinity;
  for (let dx = -1; dx <= 1; dx++) for (let dz = -1; dz <= 1; dz++) {
    const l = cells.get(key(cx + dx, cz + dz)); if (!l) continue;
    for (const i of l) {
      const a = path[i], b = path[i + 1], ex = b.x - a.x, ez = b.z - a.z, L2 = ex * ex + ez * ez;
      const t = Math.min(1, Math.max(0, ((x - a.x) * ex + (z - a.z) * ez) / L2));
      const px = a.x + ex * t - x, pz = a.z + ez * t - z, d2 = px * px + pz * pz;
      if (d2 < best) {
        best = d2;
        near.s = a.s + STEP * t; near.y = a.y + (b.y - a.y) * t;
        near.o = (x - (a.x + ex * t)) * -a.tz + (z - (a.z + ez * t)) * a.tx;
      }
    }
  }
  if (best < Infinity) near.d = Math.sqrt(best);
  return near;
}

// ---------- heights ----------
/** The road's cross-section: crown, shoulders, a shallow ditch, then up to the land. */
export function profile(o: number) {
  const a = Math.abs(o);
  if (a <= HALF) return 0.1 + 0.04 * (1 - (a / HALF) ** 2);
  if (a <= SHOULDER) return 0.1 - 0.06 * (a - HALF) / (SHOULDER - HALF);
  if (a <= 7) return 0.04 - 0.2 * smooth(SHOULDER, 7, a);
  if (a <= 10) return -0.16 - 0.3 * smooth(7, 9, a) + 0.3 * smooth(9, 10, a);
  return -0.16;
}
/** World-local height of the ground at (x, z): the road and its verges, blending into the land. */
export function heightAt(x: number, z: number) {
  const n = nearest(x, z), land = terrain(x, z);
  if (n.d === Infinity) return land + detail(x, z);
  const w = smooth(16, 70, n.d);
  const road = n.y + profile(n.d);
  return road + (land + detail(x, z) * smooth(10, 20, n.d) - road) * w;
}

// ---------- places along the road ----------
/** Mile posts on the right, facing the traffic. */
export const POSTS = [5, 6, 7, 8, 9].map((mi) => ({ mi, s: sAt(mi), o: 5.4 }));
/** Survey bolts in the right shoulder: by the six-mile post (south of C), and on C. */
export const BOLTS = [{ mi: 6.0, s: sAt(6.0) + 9, o: 4.9, c: false }, { mi: CROSS_MI, s: 0, o: 4.9, c: true }];
/** The truck at the start: the right lane at 5.9 miles, the engine running. */
export const START = (() => {
  const s = sAt(5.9), b = beside(s, 1.6);
  return { x: b.x, z: b.z, heading: -Math.atan2(b.p.tx, -b.p.tz), s };
})();
/** C, the old survey line: due east through the crossing, from STATION 01's side to the dawn. */
export const C_LINE = { x0: -620, x1: 940, z: 0 };
/** Where the truck may not go back past: about 250 m behind the start. */
export const BACK_WALL = sAt(5.9) - 250;
/** Mesa on the east-north-east horizon where the sun comes up (azimuth, degrees). */
export const SUN_AZ = 79;
/** SARO from the crossing: west-north-west, far (bearing degrees, metres). */
export const SARO_FROM = { bearing: 284, dist: 3600 };

export const rnd = (x: number, z: number) => hash(x, z);
