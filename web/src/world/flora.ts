import * as THREE from 'three';
import { floodlit, type FloodSet } from './kit';
import { getQuality } from '../core/quality';
import { wind, windAt } from './wind';

// The desert ground (Tom, 19 October: «more realistic environments with more detail»): dry
// bunchgrass with its seed stalks, sagebrush, creosote, last year's dead weeds standing in it, a
// few yucca and cholla, stones with the pebbles round them, and paler and darker stains in the
// dirt. Every outdoor area has one patch of it (world/groundCover.ts says where for each), built
// in the patch's own terms.
//
// Cost: five instanced meshes, five draw calls with all of them in view, whatever the size of the
// area: one per kind, but the grass and the sage share one, and the creosote and the dead stalks
// another (PAIRS: cards on the one picture, the second kind's copies after the first's in the
// mesh, their corners in attributes of their own). The land is cut into 8 m cells; a cell's
// plants are drawn from its own random numbers the first time the camera comes near it, so two
// areas that draw the same land (the stretches of road in drive/corridors.ts) draw the very same
// plants. Only the cells near the camera are in the meshes: they are copied in again each time
// the camera has moved 8 m (before the picture is drawn, followFlora), and each kind fades out
// with distance in its shader, the small ones first (they shrink into the ground; nothing is
// transparent but the stains). Low draws fewer and nearer, and no stains.
//
// Light: the area's fake floods (kit.floodlit) and the scene's lights, so the sodium lamps, the
// headlights and the flashlight light it like the ground under it; the sky and the moon reach
// into a tuft less than onto the open dirt (darkest at the root), so out of the pools of light
// the cover is as dark as the ground, and a lamp behind the blades shows through their edges.
// The grass, the brush and the stalks sway in the wind (wind.ts, one shared uniform). Nothing
// here is a collider: the player and the truck go through it, so it keeps off the roads, the
// tracks, the lots and the floors, and where the player walks only the low kinds grow
// (FloraSpec.walks); in the seams at the edge of a way only tiny grass and pebbles
// (FloraSpec.seam), and the gravel spills a little out onto the dirt (FloraSpec.spill).
//
// Where it grows: in drifts a few metres across with bare dirt between (clump), the grass and the
// stalks mostly round a shrub (nurse), big tufts where it is thick and small ones in the open, a
// dark litter under some of the big ones. The creosote and the yucca go on thinly far out (far),
// and past the cover's reach the desert's own ground has the brush's dark spots (desertGround).

export type FloraKind = 'grass' | 'sage' | 'creosote' | 'stalks' | 'succulent' | 'stones' | 'stain';
export type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
export interface FloraSpec {
  /** For the tests (S47.world.flora). */
  name: string;
  /** The same seed and the same cover give the same plants in two patches. */
  seed: number;
  set: FloodSet;
  falloff?: number;
  /** The ground's height at a point (the patch's terms), where the plants stand. */
  ground: (x: number, z: number) => number;
  /** How thick the cover is: 0 none (a road, a track, a lot, a floor), 1 the open desert, up to
   *  about 2.5 along fences, ditches and walls. */
  cover: (x: number, z: number) => number;
  /** Things that stand (colliders): nothing inside, thick round their feet. */
  blocks?: Rect[];
  /** Where the player walks: only the low kinds, and small. */
  walks?: Rect[];
  /** More or less of a kind here (1 as everywhere). */
  mix?: Partial<Record<FloraKind, number>>;
  /** Tests: the name of a way the story uses at a point (a track, a road, a lot), or null. */
  ways?: (x: number, z: number) => string | null;
  /** In the seam at the edge of a way, where cover is 0 (0.2 to 0.6 m inside a track's, a lot's,
   *  a curb's edge, a sidewalk's joints, never a wheel track or the middle of a walk): how much
   *  tiny grass and how many pebbles grow there (0 none, 1 as many as on open ground). */
  seam?: (x: number, z: number) => number;
  /** The gravel spilt onto the dirt just off a way: more pebbles, and small (1 as everywhere). */
  spill?: (x: number, z: number) => number;
}

/** One kind: how many per square metre in the open desert, how much more cover can add, how far
 *  it is drawn, how much of it Low keeps, how much of it grows where the player walks and how big
 *  at most there, the sizes, and the size of its patches. cap: instances in the mesh at most.
 *  clump: the size of its drifts (metres; none: even); nurse: the share of it that stands round a
 *  shrub of its cell; seam: how much of it grows in a way's seams; far: past this distance only
 *  this share of it is drawn (a sparse ring out to its reach, not on Low). */
type Kind = { id: FloraKind; rate: number; most: number; reach: number; low: number; walk: number; walkMax: number;
  size: [number, number]; patch: number; cap: number; clump?: number; nurse?: number; seam?: number; far?: [number, number] };
const KINDS: Kind[] = [
  { id: 'grass', rate: 0.46, most: 3.8, reach: 65, low: 0.45, walk: 0.85, walkMax: 1.0, size: [0.3, 1.5], patch: 26, cap: 9000, clump: 3.5, nurse: 0.55, seam: 3.6 },
  { id: 'sage', rate: 0.028, most: 2.2, reach: 110, low: 0.7, walk: 0.35, walkMax: 0.5, size: [0.45, 1.35], patch: 38, cap: 1600 },
  { id: 'creosote', rate: 0.006, most: 1.4, reach: 240, low: 0.8, walk: 0, walkMax: 0, size: [0.7, 1.4], patch: 60, cap: 600, far: [110, 0.14] },
  { id: 'stalks', rate: 0.012, most: 3.4, reach: 70, low: 0.4, walk: 0.5, walkMax: 0.75, size: [0.6, 1.3], patch: 30, cap: 1500, clump: 4, nurse: 0.65 },
  // yucca and cholla in one mesh (a mirrored copy is a cholla: succulentShape)
  { id: 'succulent', rate: 0.0027, most: 1.1, reach: 260, low: 1, walk: 0, walkMax: 0, size: [0.62, 1.28], patch: 60, cap: 520, far: [130, 0.14] },
  { id: 'stones', rate: 0.07, most: 1.6, reach: 36, low: 0.35, walk: 0.8, walkMax: 0.9, size: [0.3, 2.1], patch: 18, cap: 3000, seam: 1.4 },
  // the stains in the dirt, and the litter under a big tuft or a shrub (ranked last: near only)
  { id: 'stain', rate: 0.0045, most: 1.2, reach: 90, low: 0, walk: 1, walkMax: 1.4, size: [0.9, 2.6], patch: 1, cap: 1100, far: [42, 0.5] },
];
const CLUMP_K = 4.5;     // the drifts' noise raised to 2.5 averages about 1/4.5 (as thick as before, gathered)
const NURSE: Partial<Record<FloraKind, FloraKind[]>> = { grass: ['sage', 'creosote'], stalks: ['sage', 'creosote'] };
const C = 8;             // cell size, metres
const STEP = 8;          // the meshes are filled again when the camera has moved this far,
const TURN = 0.26;       // or turned this far (radians): only what is in front of it is drawn,
const CONE = 0.48;       // out to this much past the sides of the picture,
const NEAR = 16;         // and all round it this close
const SYNC = 30;         // cells this near are made when they are needed; further out ahead of time,
const AHEAD_MS = 1.2;    // this long in a picture at the most
const JUMP = 40;         // moved further than this between two pictures: everything at once
const LOW_REACH = 0.7;   // Low draws this much as far
const MAX_R = Math.max(...KINDS.map((k) => k.reach)) + STEP;
/** The cells round the camera's, nearest first. */
const OFFSETS: [number, number, number][] = (() => {
  const n = Math.ceil(MAX_R / C) + 2, out: [number, number, number][] = [];
  for (let dz = -n; dz <= n; dz++) for (let dx = -n; dx <= n; dx++) out.push([dx, dz, Math.hypot(dx, dz) * C]);
  return out.sort((a, b) => a[2] - b[2]);
})();
const FRONT = new Int8Array(OFFSETS.length);

// ---------- random numbers ----------
function hash4(a: number, b: number, c: number, d: number) {
  let h = 2166136261 ^ Math.imul(a | 0, 0x27d4eb2d);
  h = Math.imul(h ^ (b | 0), 16777619); h = Math.imul(h ^ (c | 0), 0x165667b1); h = Math.imul(h ^ (d | 0), 0x9e3779b1);
  h ^= h >>> 15; h = Math.imul(h, 0x85ebca6b); h ^= h >>> 13;
  return h >>> 0;
}
function mulberry(seed: number) {
  let s = seed >>> 0;
  return () => { s = (s + 0x6d2b79f5) >>> 0; let t = Math.imul(s ^ (s >>> 15), 1 | s); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
}
/** Smooth value noise, 0..1, on a lattice of `size` metres. */
function vnoise(x: number, z: number, size: number, seed: number) {
  const u = x / size, v = z / size, i = Math.floor(u), j = Math.floor(v), fu = u - i, fv = v - j;
  const h = (a: number, b: number) => hash4(a, b, seed, 77) / 4294967296;
  const su = fu * fu * (3 - 2 * fu), sv = fv * fv * (3 - 2 * fv);
  const a = h(i, j), b = h(i + 1, j), c = h(i, j + 1), d = h(i + 1, j + 1);
  return a + (b - a) * su + (c - a) * sv + (a - b - c + d) * su * sv;
}

/** A way's edge made ragged (groundCover.ts): 0..1, smooth over a couple of metres. */
export const ragged = (x: number, z: number) => vnoise(x, z, 2.3, 4111);

// ---------- shapes ----------
// Each kind is one small mesh built here, its base at y 0, about a metre across at scale 1, with
// a colour per corner (written in sRGB, as a painter picks it; multiplied by the copy's own) and
// aSway, how much a corner moves in the wind (0 at the root, 1 at a grass tip). The grass and the
// shrubs and the dead stalks are a few crossed cards with a picture drawn here (floraAtlas):
// dozens of blades or hundreds of leaves for four to eight triangles. The yucca, the cholla and the
// stones are made of few triangles: icosahedra and octahedra, three- and five-sided stems, blades
// of two triangles.
const _lin = new THREE.Color();
class Shape {
  pos: number[] = []; nor: number[] = []; col: number[] = []; alpha: number[] = []; sway: number[] = []; uv: number[] = []; idx: number[] = [];
  /** Which of two plants in one mesh the corners made now belong to (aPart; succulentShape). */
  part = 0; parts: number[] = [];
  v(x: number, y: number, z: number, n: THREE.Vector3, c: readonly number[], s: number, a = 1, u = 0, w = 0) {
    _lin.setRGB(c[0], c[1], c[2], THREE.SRGBColorSpace);
    this.pos.push(x, y, z); this.nor.push(n.x, n.y, n.z); this.col.push(_lin.r, _lin.g, _lin.b); this.alpha.push(a); this.sway.push(s); this.uv.push(u, w);
    this.parts.push(this.part);
    return this.pos.length / 3 - 1;
  }
  t(a: number, b: number, c: number) { this.idx.push(a, b, c); }
  /** n upright cards round the root showing part r of the atlas ([u0, v0, u1, v1]): w0 wide at
   *  the foot, w1 at the top, h high, each tilted out to one side or the other by tilt (the top
   *  moved this share of h); the foot darker (shade). */
  cards(n: number, w0: number, w1: number, h: number, r: readonly number[], swayTop: number, shade: number, turn = 0, tilt = 0.08) {
    for (let i = 0; i < n; i++) {
      const a = turn + i * Math.PI / n, dx = Math.cos(a), dz = Math.sin(a), lx = -dz, lz = dx;
      const lean = (i % 2 ? 1 : -1) * tilt * h;
      const foot = [shade, shade * 0.98, shade * 0.95], top = [1, 1, 1];
      const p = this.v(-dx * w0 / 2, -0.03, -dz * w0 / 2, UP, foot, 0, 1, r[0], r[1]);
      const q = this.v(dx * w0 / 2, -0.03, dz * w0 / 2, UP, foot, 0, 1, r[2], r[1]);
      const s = this.v(dx * w1 / 2 + lx * lean, h, dz * w1 / 2 + lz * lean, UP, top, swayTop, 1, r[2], r[3]);
      const t = this.v(-dx * w1 / 2 + lx * lean, h, -dz * w1 / 2 + lz * lean, UP, top, swayTop, 1, r[0], r[3]);
      this.t(p, q, s); this.t(p, s, t);
    }
  }
  /** An icosahedron flattened on top and bottom (a weathered stone), its corners moved in or out. */
  rock(at: THREE.Vector3, scale: THREE.Vector3, r: () => number, jit: number, col: (y: number) => readonly number[]) {
    const g = new THREE.IcosahedronGeometry(1, 0), p = g.attributes.position as THREE.BufferAttribute, map = new Map<string, number>(), n = new THREE.Vector3();
    const face: number[] = [];
    for (let i = 0; i < p.count; i++) {
      const key = `${p.getX(i).toFixed(3)},${p.getY(i).toFixed(3)},${p.getZ(i).toFixed(3)}`;
      let k = map.get(key);
      if (k === undefined) {
        const j = 1 - jit + r() * jit * 2, y = Math.max(-0.55, Math.min(0.62, p.getY(i)));
        n.set(p.getX(i), p.getY(i), p.getZ(i)).normalize();
        const py = at.y + y * scale.y * j;
        k = this.v(at.x + p.getX(i) * scale.x * j, py, at.z + p.getZ(i) * scale.z * j, n, col(py), 0);
        map.set(key, k);
      }
      face.push(k);
      // (the faces round the bottom are in the dirt)
      if (face.length === 3) { if (Math.max(p.getY(i), p.getY(i - 1), p.getY(i - 2)) > -0.4) this.idx.push(...face); face.length = 0; }
    }
    g.dispose();
  }
  /** An octahedron, its corners moved in or out, its normals from its centre. */
  lump(at: THREE.Vector3, scale: THREE.Vector3, r: () => number, jit: number, col: (y: number) => readonly number[], sway: (y: number) => number) {
    const n = new THREE.Vector3(), k: number[] = [];
    for (const [x, y, z] of [[1, 0, 0], [-1, 0, 0], [0, 1, 0], [0, -1, 0], [0, 0, 1], [0, 0, -1]]) {
      const j = 1 - jit + r() * jit * 2, px = at.x + x * scale.x * j, py = at.y + y * scale.y * j, pz = at.z + z * scale.z * j;
      k.push(this.v(px, py, pz, n.set(x, y, z), col(py), sway(py)));
    }
    const [px, nx, py, ny, pz, nz] = k;
    for (const [a, b, c] of [[px, py, pz], [pz, py, nx], [nx, py, nz], [nz, py, px], [pz, ny, px], [nx, ny, pz], [nz, ny, nx], [px, ny, nz]]) this.t(a, b, c);
  }
  /** A pebble half in the dirt: a low four-sided mound (four triangles; the rest is buried). */
  pebble(at: THREE.Vector3, rad: number, flat: number, r: () => number, col: readonly number[]) {
    const n = new THREE.Vector3(), a0 = r() * Math.PI, k: number[] = [];
    for (let i = 0; i < 4; i++) {
      const a = a0 + i * Math.PI / 2, j = 0.75 + r() * 0.5;
      k.push(this.v(at.x + Math.cos(a) * rad * j, at.y, at.z + Math.sin(a) * rad * j * 0.8, n.set(Math.cos(a), 0.35, Math.sin(a)).normalize(), col, 0));
    }
    const top = this.v(at.x + (r() - 0.5) * rad * 0.5, at.y + rad * flat, at.z + (r() - 0.5) * rad * 0.4, UP, col, 0);
    for (let i = 0; i < 4; i++) this.t(k[(i + 1) % 4], k[i], top);
  }
  /** A small bead in the air (a pod, a fruit): a three-sided double pyramid, six triangles. */
  bead(at: THREE.Vector3, rad: number, tall: number, r: () => number, col: readonly number[], sway: number) {
    const n = new THREE.Vector3(), a0 = r() * Math.PI, k: number[] = [];
    for (let i = 0; i < 3; i++) {
      const a = a0 + i * Math.PI * 2 / 3;
      k.push(this.v(at.x + Math.cos(a) * rad, at.y, at.z + Math.sin(a) * rad, n.set(Math.cos(a), 0, Math.sin(a)), col, sway));
    }
    const top = this.v(at.x, at.y + tall, at.z, UP, col, sway), bot = this.v(at.x, at.y - tall, at.z, n.set(0, -1, 0), col, sway);
    for (let i = 0; i < 3; i++) { this.t(k[(i + 1) % 3], k[i], top); this.t(k[i], k[(i + 1) % 3], bot); }
  }
  /** A thin prism from a to b (stems, trunks, stalks), radius ra at a and rb at b. */
  stem(a: THREE.Vector3, b: THREE.Vector3, ra: number, rb: number, sides: number, col: readonly number[], sa: number, sb: number, col2 = col) {
    const d = new THREE.Vector3().subVectors(b, a).normalize();
    const u = new THREE.Vector3(1, 0, 0).cross(d); if (u.lengthSq() < 1e-4) u.set(0, 0, 1).cross(d);
    u.normalize(); const w = new THREE.Vector3().crossVectors(d, u);
    const base = this.pos.length / 3, n = new THREE.Vector3();
    for (let i = 0; i < sides; i++) {
      const t = i / sides * Math.PI * 2;
      n.copy(u).multiplyScalar(Math.cos(t)).addScaledVector(w, Math.sin(t));
      this.v(a.x + n.x * ra, a.y + n.y * ra, a.z + n.z * ra, n, col, sa);
      this.v(b.x + n.x * rb, b.y + n.y * rb, b.z + n.z * rb, n, col2, sb);
    }
    for (let i = 0; i < sides; i++) {
      const p = base + i * 2, q = base + ((i + 1) % sides) * 2;
      this.t(p, q, p + 1); this.t(q, q + 1, p + 1);
    }
  }
  /** A blade from a root corner: widest a little under half way, bending over to its tip; two
   *  triangles. Its normals lean up, so both sides take the light as the ground does. */
  blade(root: number, at: THREE.Vector3, dir: THREE.Vector3, len: number, wid: number, bend: number, c1: readonly number[], c2: readonly number[], s1 = 0.35) {
    const out = new THREE.Vector3(dir.x, 0, dir.z); if (out.lengthSq() < 1e-6) out.set(1, 0, 0); out.normalize();
    const side = new THREE.Vector3(-out.z, 0, out.x);
    const mid = at.clone().addScaledVector(dir, len * 0.42).addScaledVector(out, bend * 0.2 * len);
    const tip = at.clone().addScaledVector(dir, len).addScaledVector(out, bend * len); tip.y -= bend * bend * len * 0.45;
    const n = new THREE.Vector3(0, 1, 0).addScaledVector(out, 0.3).normalize();
    const a = this.v(mid.x - side.x * wid, mid.y, mid.z - side.z * wid, n, c1, s1);
    const b = this.v(mid.x + side.x * wid, mid.y, mid.z + side.z * wid, n, c1, s1);
    const e = this.v(tip.x, tip.y, tip.z, n, c2, 1);
    this.t(root, a, b); this.t(a, e, b);
  }
  build() {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('normal', new THREE.Float32BufferAttribute(this.nor, 3));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(this.uv, 2));
    if (this.alpha.some((a) => a !== 1)) {
      const c4: number[] = [];
      for (let i = 0; i < this.alpha.length; i++) c4.push(this.col[i * 3], this.col[i * 3 + 1], this.col[i * 3 + 2], this.alpha[i]);
      g.setAttribute('color', new THREE.Float32BufferAttribute(c4, 4));
    } else g.setAttribute('color', new THREE.Float32BufferAttribute(this.col, 3));
    g.setAttribute('aSway', new THREE.Float32BufferAttribute(this.sway, 1));
    if (this.parts.some((k) => k !== 0)) g.setAttribute('aPart', new THREE.Float32BufferAttribute(this.parts, 1));
    g.setIndex(this.idx);
    g.computeBoundingSphere();
    return g;
  }
}
const V = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
const UP = V(0, 1, 0);
const mixc = (a: readonly number[], b: readonly number[], t: number) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
const polar = (a: number, el: number) => V(Math.cos(a) * Math.sin(el), Math.cos(el), Math.sin(a) * Math.sin(el));

// ---------- the pictures on the cards ----------
// One picture of 512 for the four, each in its quarter, drawn here: the grass's blades and seed
// stalks, the sage's twigs and its grey leaves in a mound, the creosote's dark stems in a vase
// with small leaves along them, last year's dead weeds. Its smaller sizes are made here too, with the cover thickened at
// each step, so a bush seen from far off keeps its body instead of fading through.
const AT = 512, Q = AT / 2;
/** The atlas quarters as [u0, v0, u1, v1] (v up, the picture's top is v 1). */
const REGION = { grass: [0, 0.5, 0.5, 1], sage: [0.5, 0.5, 1, 1], creosote: [0, 0, 0.5, 0.5], stalks: [0.5, 0, 1, 0.5] } as const;
let atlas: THREE.Texture | null = null;
function floraAtlas() {
  if (atlas) return atlas;
  const c = document.createElement('canvas'); c.width = c.height = AT;
  const g = c.getContext('2d')!, r = mulberry(97);
  const rgb = (k: readonly number[]) => `rgb(${Math.round(k[0] * 255)},${Math.round(k[1] * 255)},${Math.round(k[2] * 255)})`;
  // a tapering stroke from (ax, ay) by way of (cx, cy) to (bx, by), w0 wide at its start
  const stroke = (ax: number, ay: number, cx: number, cy: number, bx: number, by: number, w0: number, c0: string, c1: string) => {
    const grad = g.createLinearGradient(ax, ay, bx, by); grad.addColorStop(0, c0); grad.addColorStop(1, c1);
    g.fillStyle = grad; g.beginPath();
    const n = 8, L: [number, number][] = [], R: [number, number][] = [];
    for (let i = 0; i <= n; i++) {
      const t = i / n, x = (1 - t) * (1 - t) * ax + 2 * (1 - t) * t * cx + t * t * bx, y = (1 - t) * (1 - t) * ay + 2 * (1 - t) * t * cy + t * t * by;
      const dx = 2 * (1 - t) * (cx - ax) + 2 * t * (bx - cx), dy = 2 * (1 - t) * (cy - ay) + 2 * t * (by - cy), l = Math.hypot(dx, dy) || 1, w = w0 * (1 - t * 0.92) / 2;
      L.push([x - dy / l * w, y + dx / l * w]); R.push([x + dy / l * w, y - dx / l * w]);
    }
    g.moveTo(...L[0]); for (const p of L) g.lineTo(...p); for (const p of R.reverse()) g.lineTo(...p); g.closePath(); g.fill();
  };
  // each in its own quarter (nothing runs over into the next, also in the small sizes)
  const clip = (x0: number, y0: number) => { g.restore(); g.save(); g.beginPath(); g.rect(x0 + 3, y0 + 3, Q - 6, Q - 6); g.clip(); };
  g.save();
  // the grass: a dense dark foot of short blades and old leaves, blades from a crown a third of
  // the picture wide fanning out over it, nearly half of them bent over at the tip, and seed
  // stalks over them. Straw by day; at night no paler than the dirt in the moon (floraMaterial).
  {
    const x0 = 0, y0 = 0, cx = x0 + Q / 2, by = y0 + Q - 3;
    clip(x0, y0);
    for (let i = 0; i < 70; i++) {
      const bx = cx + (r() - 0.5) * Q * 0.44, a = (bx - cx) / Q * 3 + (r() - 0.5) * 1.6, len = Q * (0.1 + r() * 0.2);
      const tx = bx + Math.sin(a) * len, ty = by - Math.cos(a) * len;
      stroke(bx, by, (bx + tx) / 2, (by + ty) / 2 - 3, tx, ty, 3 + r() * 3.5, rgb(mixc([0.1, 0.085, 0.06], [0.16, 0.13, 0.09], r())), rgb(mixc([0.22, 0.18, 0.12], [0.32, 0.27, 0.19], r())));
    }
    for (let i = 0; i < 74; i++) {
      const u = r() - 0.5, bx = cx + u * Q * 0.35, a = u * 1.5 + (r() - 0.5) * 1.5, len = Q * (0.4 + r() * 0.52);
      const droop = r() < 0.45, s = Math.sin(a);
      // straight up and out, or up and over: the tip comes down outside the crown
      const tx = bx + s * len * (droop ? 1.2 : 1), ty = by - Math.cos(a) * len * (droop ? 0.5 + r() * 0.2 : 0.78 + r() * 0.22);
      const mx = bx + s * len * (droop ? 0.45 : 0.25), my = by - len * (droop ? 0.92 : 0.55);
      const base = mixc([0.15, 0.125, 0.08], [0.24, 0.2, 0.13], r()), tip = mixc([0.66, 0.59, 0.42], [0.78, 0.7, 0.52], r());
      stroke(bx, by, mx, my, tx + s * len * 0.12, ty, 2.5 + r() * 3.5, rgb(base), rgb(tip));
    }
    for (let i = 0; i < 4; i++) {
      const bx = cx + (r() - 0.5) * Q * 0.22, tx = bx + (r() - 0.5) * Q * 0.6, ty = y0 + Q * (0.04 + r() * 0.16);
      stroke(bx, by, (bx + tx) / 2, (by + ty) / 2, tx, ty, 2.2, rgb([0.36, 0.32, 0.24]), rgb([0.52, 0.47, 0.36]));
      g.fillStyle = rgb([0.54, 0.49, 0.38]);
      for (let k = 0; k < 6; k++) { g.beginPath(); g.ellipse(tx + (r() - 0.5) * 2.5, ty + k * 3, 1.3, 2.2, 0, 0, Math.PI * 2); g.fill(); }
    }
  }
  // the sage: grey twigs from the root, five ragged lobes of grey-olive leaves over them with
  // gaps between, darker underneath, and bare twig tips standing out of them
  {
    const x0 = Q, y0 = 0, cx = x0 + Q / 2, by = y0 + Q - 3, rx = Q * 0.46, ry = Q * 0.78;
    clip(x0, y0);
    const lobes: { x: number; w: number; h: number }[] = [];
    for (let i = 0, n = 5; i < n; i++) {
      const x = cx + rx * (-0.78 + 1.56 * (i + 0.2 + r() * 0.6) / n);
      lobes.push({ x, w: rx * (0.24 + r() * 0.16), h: ry * (0.98 - 0.85 * ((x - cx) / rx) ** 2) * (0.72 + r() * 0.32) });
    }
    const dome = (x: number) => Math.max(0, ...lobes.map((l) => l.h * Math.sqrt(Math.max(0, 1 - ((x - l.x) / l.w) ** 2))));
    for (let i = 0; i < 30; i++) {
      const l = lobes[Math.floor(r() * lobes.length)], tx = l.x + (r() - 0.5) * l.w * 1.6, ty = by - dome(tx) * (0.45 + r() * 0.5);
      stroke(cx + (r() - 0.5) * 18, by, (cx + tx) / 2, (by + ty) / 2 + 10, tx, ty, 3 + r() * 2.5, rgb([0.18, 0.16, 0.13]), rgb([0.34, 0.31, 0.26]));
    }
    for (let i = 0; i < 900; i++) {
      const l = lobes[Math.floor(r() * lobes.length)], x = l.x + (r() - 0.5) * l.w * 2, top = dome(x), y = by - top * Math.pow(r(), 0.55);
      if (top <= 0 || y > by - 6) continue;
      const h = (by - y) / ry, t = r();
      const k = mixc(mixc([0.19, 0.19, 0.15], [0.27, 0.27, 0.21], t), mixc([0.41, 0.41, 0.33], [0.51, 0.5, 0.41], t), Math.min(1, h * 1.3));
      g.fillStyle = rgb(k); g.beginPath(); g.ellipse(x, y, 2.2 + r() * 3, 1.6 + r() * 2.2, r() * Math.PI, 0, Math.PI * 2); g.fill();
    }
    // bare twig tips out of the lobes and between them
    for (let i = 0; i < 14; i++) {
      const tx = cx + (r() - 0.5) * rx * 1.8, ty = by - dome(tx) - 6 - r() * 26;
      stroke(cx + (r() - 0.5) * 30, by - 16 - r() * 30, (cx + tx) / 2, (by + ty) / 2, tx, ty, 1.5 + r(), rgb([0.26, 0.23, 0.19]), rgb([0.44, 0.4, 0.34]));
    }
  }
  // the creosote: dark stems in a vase, small olive leaves clustered along their upper parts
  {
    const x0 = 0, y0 = Q, cx = x0 + Q / 2, by = y0 + Q - 3;
    clip(x0, y0);
    for (let i = 0; i < 13; i++) {
      const u = r() - 0.5, tx = cx + u * Q * 0.92, ty = y0 + Q * (0.04 + Math.abs(u) * 0.55 + r() * 0.2), bx = cx + (r() - 0.5) * 16;
      const mx = (bx + tx) / 2 + (r() - 0.5) * 20, my = (by + ty) / 2;
      stroke(bx, by, mx, my, tx, ty, 4 + r() * 2, rgb([0.2, 0.18, 0.15]), rgb([0.3, 0.27, 0.22]));
      for (let k = 0; k < 45; k++) {
        const t = 0.35 + Math.pow(r(), 0.7) * 0.68, x = (1 - t) * (1 - t) * bx + 2 * (1 - t) * t * mx + t * t * tx, y = (1 - t) * (1 - t) * by + 2 * (1 - t) * t * my + t * t * ty;
        const s = r(), kc = mixc([0.16, 0.17, 0.1], [0.3, 0.31, 0.18], s);
        g.fillStyle = rgb(kc); g.beginPath(); g.ellipse(x + (r() - 0.5) * 16, y + (r() - 0.5) * 12, 1.6 + r() * 2.4, 1.2 + r() * 1.6, r() * Math.PI, 0, Math.PI * 2); g.fill();
      }
    }
  }
  // dead stalks: last year's weeds, grey and bare, a few side twigs and a dry head on each
  {
    const x0 = Q, y0 = Q, cx = x0 + Q / 2, by = y0 + Q - 3;
    clip(x0, y0);
    for (let i = 0; i < 11; i++) {
      const bx = cx + (r() - 0.5) * Q * 0.3, tx = bx + (r() - 0.5) * Q * 0.55, ty = y0 + Q * (0.04 + r() * 0.35);
      const mx = (bx + tx) / 2 + (r() - 0.5) * 14, my = (by + ty) / 2;
      const c0 = rgb([0.22, 0.2, 0.17]), c1 = rgb(mixc([0.34, 0.31, 0.27], [0.45, 0.41, 0.36], r()));
      stroke(bx, by, mx, my, tx, ty, 3 + r() * 1.5, c0, c1);
      for (let k = 0; k < 4; k++) {
        const t = 0.45 + r() * 0.45, x = (1 - t) * (1 - t) * bx + 2 * (1 - t) * t * mx + t * t * tx, y = (1 - t) * (1 - t) * by + 2 * (1 - t) * t * my + t * t * ty;
        const sx = x + (r() < 0.5 ? -1 : 1) * (10 + r() * 18), sy = y - 8 - r() * 16;
        stroke(x, y, (x + sx) / 2, (y + sy) / 2 + 2, sx, sy, 1.7, c1, c1);
      }
      g.fillStyle = rgb([0.36, 0.3, 0.24]);
      for (let k = 0; k < 6; k++) { g.beginPath(); g.ellipse(tx + (r() - 0.5) * 8, ty + (r() - 0.3) * 8, 2 + r() * 2, 1.5 + r() * 1.5, r() * Math.PI, 0, Math.PI * 2); g.fill(); }
    }
  }
  g.restore();
  // the smaller sizes, each a little more covered than a plain average, so far bushes keep their body
  const levels: HTMLCanvasElement[] = [c];
  for (let w = AT >> 1, prev = c; w >= 1; w >>= 1) {
    const m = document.createElement('canvas'); m.width = m.height = w;
    const mg = m.getContext('2d')!; mg.drawImage(prev, 0, 0, w, w);
    const d = mg.getImageData(0, 0, w, w);
    for (let i = 3; i < d.data.length; i += 4) d.data[i] = Math.min(255, d.data[i] * 1.45);
    mg.putImageData(d, 0, 0);
    levels.push(m); prev = m;
  }
  const t = new THREE.CanvasTexture(c);
  t.mipmaps = levels; t.generateMipmaps = false; t.minFilter = THREE.LinearMipmapLinearFilter;
  t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 4;
  return atlas = t;
}

/** Bunchgrass: three crossed cards of blades and seed stalks, a fountain from a crown half a metre
 *  across, the cards tilted out 25 degrees so the foot reads full from the eyes and the cab. */
function grassShape() {
  const s = new Shape();
  s.cards(3, 0.5, 0.95, 0.5, REGION.grass, 1, 0.7, 0, Math.tan(0.44));
  return s.build();
}

/** Sagebrush: three crossed cards of a low ragged grey mound, about as wide at the foot as over it. */
function sageShape() {
  const s = new Shape();
  s.cards(3, 0.9, 1.08, 0.78, REGION.sage, 0.4, 0.8, 0.3, 0.12);
  return s.build();
}

/** Creosote: four crossed cards of thin dark stems in a vase with small olive leaves along the
 *  top: open, taller than the sage, the desert seen through it. */
function creosoteShape() {
  const s = new Shape();
  s.cards(4, 0.5, 1.35, 1.6, REGION.creosote, 0.8, 0.85, 0.2);
  return s.build();
}

/** Dead stalks: two crossed cards of last year's bare weeds, standing over the grass. */
function stalksShape() {
  const s = new Shape();
  s.cards(2, 0.3, 0.75, 1.1, REGION.stalks, 0.9, 0.85, 0.7);
  return s.build();
}

/** A yucca and a cholla in one mesh, each copy showing one of them: the shader keeps the yucca's
 *  corners (aPart 0) in a copy as it is made and the cholla's (aPart 1) in one turned inside out
 *  (mirrored, which a plant does not show): one draw call for the two. */
function succulentShape() {
  const s = new Shape();
  yucca(s);
  s.part = 1;
  cholla(s);
  return s.build();
}

/** Soaptree yucca: a short trunk, a round ball of stiff narrow leaves (the lower ones out level
 *  and drooping, the tips paler), a skirt of dead ones under it, and last year's flower stalk
 *  standing over it, thick enough to show against the stars, branched at the top with its
 *  empty pods. */
function yucca(s: Shape) {
  const r = mulberry(41);
  const trunk = [0.3, 0.25, 0.19], dead = [0.46, 0.4, 0.31];
  s.stem(V(0, 0, 0), V(0.02, 0.42, 0), 0.1, 0.08, 5, trunk, 0, 0.05);
  const c = V(0.02, 0.45, 0), crown = s.v(c.x, c.y, c.z, UP, [0.26, 0.28, 0.2], 0.05);
  for (let i = 0, N = 30; i < N; i++) {
    // evenly over the ball, from straight up to 1.9 rad down from it
    const a = i * 2.4 + r() * 0.4, el = Math.acos(0.995 - (i + r()) / N * 1.32), t = r();
    s.blade(crown, c, polar(a, el), 0.48 + r() * 0.34, 0.026, el > 1.3 ? 0.08 + (el - 1.3) * 0.2 : 0.03,
      mixc([0.32, 0.36, 0.25], [0.4, 0.44, 0.31], t), mixc([0.62, 0.62, 0.47], [0.72, 0.7, 0.55], t), 0.12);
  }
  const skirt = s.v(0.02, 0.4, 0, UP, trunk, 0.02);
  for (let i = 0; i < 9; i++) {
    const a = i * 0.7 + r() * 0.5;
    s.blade(skirt, V(0.02, 0.4, 0), polar(a, 2.05 + r() * 0.45), 0.36 + r() * 0.12, 0.028, 0, dead, mixc(dead, [0.56, 0.5, 0.4], r()), 0.05);
  }
  const top = V(0.08, 1.95, 0.04), stalk = [0.42, 0.37, 0.3], pod = [0.38, 0.32, 0.25];
  s.stem(c, top, 0.032, 0.016, 4, stalk, 0.1, 0.7);
  // the panicle: side branches up the top of the stalk, pods along them
  for (let i = 0; i < 6; i++) {
    const t = 0.6 + i * 0.065, at = c.clone().lerp(top, t), a = i * 2.1 + r() * 0.6;
    const end = at.clone().add(V(Math.cos(a) * (0.16 + r() * 0.1) * (1.2 - t), 0.14 + r() * 0.08, Math.sin(a) * (0.16 + r() * 0.1) * (1.2 - t)));
    s.stem(at, end, 0.014, 0.008, 3, stalk, 0.5 + t * 0.2, 0.75 + t * 0.2);
    for (let k = i % 2; k < 2; k++) s.bead(at.clone().lerp(end, 0.55 + k * 0.4), 0.03, 0.045, r, pod, 0.7 + t * 0.2);
  }
}

/** Cane cholla: a woody trunk and jointed arms bending up, a few dry yellow fruit on them. */
function cholla(s: Shape) {
  const r = mulberry(53);
  const skin = [0.27, 0.3, 0.21], tip = [0.36, 0.38, 0.28];
  const base = V(0, 0.55, 0);
  s.stem(V(0, 0, 0), base, 0.07, 0.06, 5, [0.36, 0.33, 0.27], 0, 0.04);
  for (let i = 0; i < 5; i++) {
    const a = i * 1.3 + r() * 0.5;
    let p = base.clone().add(V(0, -0.1 + r() * 0.15, 0)), dir = V(Math.cos(a) * 0.8, 0.45, Math.sin(a) * 0.8).normalize();
    for (let k = 0; k < 2; k++) {
      const q = p.clone().addScaledVector(dir, 0.3 + r() * 0.15);
      s.stem(p, q, 0.045, 0.04, 4, k ? tip : skin, 0.04, 0.08);
      p = q; dir = dir.add(V(0, 0.8, 0)).normalize();
    }
    if (r() < 0.6) s.bead(p.clone().add(V(0, 0.03, 0)), 0.035, 0.045, r, [0.62, 0.53, 0.3], 0.08);
  }
}

/** A stone a third sunk in the dirt with pebbles round it (small copies are gravel, big ones a
 *  rock with its rubble): dark with the desert's varnish, red-brown and grey, a pale caliche one
 *  among the pebbles now and then. */
function stonesShape() {
  const s = new Shape(), r = mulberry(67);
  const rock = (y: number) => mixc([0.2, 0.16, 0.12], [0.35, 0.28, 0.22], Math.min(1, Math.max(0, (y + 0.02) / 0.14)));
  s.rock(V(0, 0, 0), V(0.22, 0.15, 0.17), r, 0.22, rock);
  s.rock(V(0.07, 0, -0.06), V(0.13, 0.1, 0.16), r, 0.22, rock);
  for (let i = 0; i < 14; i++) {
    const a = r() * Math.PI * 2, d = 0.2 + Math.pow(r(), 0.7) * 0.65, rad = 0.03 + r() * r() * 0.065, tone = r();
    const c = tone < 0.5 ? [0.28, 0.22, 0.16] : tone < 0.85 ? [0.34, 0.3, 0.25] : [0.56, 0.52, 0.45];
    // (their foot just over the rock's: the copy is sunk by a fortieth of its height, make())
    s.pebble(V(Math.cos(a) * d, 0.02, Math.sin(a) * d), rad, 0.32 + r() * 0.25, r, c);
  }
  return s.build();
}

/** A stain in the dirt: a ragged disc, its colour strongest in the middle and gone at the rim. */
function stainShape() {
  const s = new Shape(), r = mulberry(79), N = 12, white = [1, 1, 1];
  const mid = s.v(0, 0, 0, UP, white, 0, 0.85);
  const inner: number[] = [], rim: number[] = [];
  for (let i = 0; i < N; i++) {
    const a = i / N * Math.PI * 2, k = 0.7 + r() * 0.3;
    inner.push(s.v(Math.cos(a) * 0.5 * k, 0, Math.sin(a) * 0.5 * k, UP, white, 0, 0.62));
    rim.push(s.v(Math.cos(a) * k, 0, Math.sin(a) * k, UP, white, 0, 0));
  }
  for (let i = 0; i < N; i++) {
    const j = (i + 1) % N;
    s.t(mid, inner[j], inner[i]);
    s.t(inner[i], inner[j], rim[i]); s.t(inner[j], rim[j], rim[i]);
  }
  return s.build();
}

// ---------- materials ----------
// The sky and the moon in a tuft (floraMaterial, shade): the directional lights' share of the
// light (the moon, the dawn's sun) and the sky's are taken down by fSky, from the root to the
// top; the flashlight, the headlights and the lamps (spot and point lights, and the floods) are
// not. And a card's blades face the flashlight and the headlights more than the ground does (its
// normals are the ground's, up): for the spot lights its normal turns towards the eye by fFace.
const SKY_IN = THREE.ShaderChunk.lights_fragment_begin
  .replace('#if ( NUM_SPOT_LIGHTS > 0 ) && defined( RE_Direct )', 'vec3 fN0 = geometryNormal;\ngeometryNormal = normalize(mix(geometryNormal, geometryViewDir, fFace));\n#if ( NUM_SPOT_LIGHTS > 0 ) && defined( RE_Direct )')
  .replace('#if ( NUM_DIR_LIGHTS > 0 ) && defined( RE_Direct )', 'geometryNormal = fN0;\nvec3 fDir0 = reflectedLight.directDiffuse;\n#if ( NUM_DIR_LIGHTS > 0 ) && defined( RE_Direct )')
  .replace('#if ( NUM_RECT_AREA_LIGHTS > 0 ) && defined( RE_Direct_RectArea )', 'reflectedLight.directDiffuse = fDir0 + (reflectedLight.directDiffuse - fDir0) * fSky;\n#if ( NUM_RECT_AREA_LIGHTS > 0 ) && defined( RE_Direct_RectArea )');
if (!SKY_IN.includes('fDir0 +') || !SKY_IN.includes('fN0;')) console.warn('flora: three.js lights chunk changed, the cover takes the sky and the moon as the ground does');
/** Card materials made before the first picture: they take alpha to coverage once the renderer
 *  is known to have MSAA (followFlora); the ones made after, when they are made. */
const cardMats: THREE.Material[] = [];
let msaa: boolean | null = null;

/** A flora material: floodlit by the patch's set, swaying in the wind, faded with distance.
 *  reach is this kind's (Low draws it nearer). shade: a plant (the sky and the moon in it less,
 *  darker to the root); back: thin blades, a lamp behind them shows through their edges. pair:
 *  the mesh holds a second kind's copies from `split` on (pairShape: its corners in aPosB, aUvB,
 *  aColB and aSwayB), faded by its own reach. */
function floraMaterial(o: THREE.MeshStandardMaterialParameters, set: FloodSet, falloff: number, reach: { value: number }, parts = false, shade = false, back = false,
  pair: { reach: { value: number }; split: { value: number } } | null = null) {
  const m = floodlit(new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 1, ...o }), falloff, set);
  const flood = m.onBeforeCompile;
  const twoSided = o.side === THREE.DoubleSide && !o.flatShading;
  m.onBeforeCompile = (sh, r) => {
    flood.call(m, sh, r);
    sh.uniforms.uWindTime = wind.time; sh.uniforms.uWindAmp = wind.strength; sh.uniforms.uWindDir = wind.dir; sh.uniforms.uReach = reach;
    if (pair) { sh.uniforms.uReachB = pair.reach; sh.uniforms.uSplit = pair.split; }
    // a pair: the second kind's copies (from uSplit on) take its corners, its colours and its reach
    const B = pair ? '\nattribute vec3 aPosB; attribute vec2 aUvB; attribute vec3 aColB; attribute float aSwayB;\nuniform float uReachB; uniform int uSplit;' : '';
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', `#include <common>\nattribute float aSway;${parts ? '\nattribute float aPart;' : ''}${B}\nuniform float uWindTime; uniform float uWindAmp; uniform float uReach; uniform vec2 uWindDir;\nvarying float vFSway;`)
      .replace('void main() {', pair ? 'void main() {\n  bool fB = gl_InstanceID >= uSplit;' : 'void main() {')
      .replace('#include <uv_vertex>', pair ? THREE.ShaderChunk.uv_vertex.replace(/\bMAP_UV\b/g, '(fB ? aUvB : MAP_UV)') : '#include <uv_vertex>')
      .replace('#include <color_vertex>', pair ? THREE.ShaderChunk.color_vertex.replace('vColor *= color;', 'vColor *= fB ? aColB : color;') : '#include <color_vertex>')
      .replace('#include <begin_vertex>', `#include <begin_vertex>
        ${pair ? 'if (fB) transformed = aPosB;' : ''}
        float fSw = ${pair ? 'fB ? aSwayB : aSway' : 'aSway'};
        vFSway = fSw;
        #ifdef USE_INSTANCING
        {
          // shrink into the ground with distance (the small ones much nearer: far off a tiny tuft
          // in a light is a speck), and sway at the top
          float is2 = dot(instanceMatrix[0].xyz, instanceMatrix[0].xyz), isc = sqrt(is2);
          vec3 iw = (modelMatrix * vec4(instanceMatrix[3].xyz, 1.0)).xyz;
          float reach = ${pair ? '(fB ? uReachB : uReach)' : 'uReach'} * clamp(0.05 + 0.75 * isc, 0.3, 1.0);
          float fade = 1.0 - smoothstep(reach * 0.72, reach, distance(iw.xz, cameraPosition.xz));
          ${parts ? '// one of the two plants: the other folds away\n          fade *= step(abs(aPart - step(determinant(mat3(instanceMatrix)), 0.0)), 0.5);' : ''}
          float ph = uWindTime * 1.9 + dot(iw.xz, vec2(0.21, 0.17));
          float gust = sin(ph) * 0.55 + sin(ph * 2.7 + iw.x * 0.5) * 0.25 + 0.35;
          vec3 wl = (vec3(uWindDir.x, 0.0, uWindDir.y) * mat3(instanceMatrix)) / max(is2, 1e-4);
          transformed = transformed * fade + wl * (fSw * uWindAmp * gust * isc * fade);
        }
        #endif`);
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nvarying float vFSway;');
    // both sides of a blade take the light as its top side does (the normals lean up)
    if (twoSided) sh.fragmentShader = sh.fragmentShader.replace('#include <normal_fragment_begin>', '#include <normal_fragment_begin>\n#ifdef DOUBLE_SIDED\nnormal = normalize( vNormal );\n#endif');
    // the sky and the moon in it: a third at the root, three quarters at the tips
    if (shade) sh.fragmentShader = sh.fragmentShader
      .replace('#include <lights_fragment_begin>', `float fSky = mix(0.35, 0.75, min(1.0, vFSway * 1.6)), fFace = ${back ? 'mix(0.35, 0.9, vFSway)' : '0.0'};\n${SKY_IN}`)
      .replace('#include <lights_fragment_end>', '#include <lights_fragment_end>\nreflectedLight.indirectDiffuse *= fSky;');
    // a lamp behind the blades (the camera looking towards it through them) shows through their
    // edges, the tips most; still in the lamp's own pool (the floods' windowed falloff)
    if (back) sh.fragmentShader = sh.fragmentShader
      .replace('vec3 fN = normalize(vFNormal);', 'vec3 fN = normalize(vFNormal);\n        vec3 fV = normalize(vFWorld - cameraPosition); float fBk = 0.6 * (0.3 + 0.7 * vFSway);')
      .replace('float ndl = abs(dot(fN, L)) * 0.75 + 0.25;', 'float fb = max(dot(fV, L), 0.0);\n          float ndl = abs(dot(fN, L)) * 0.75 + 0.25 + fBk * fb * fb * fb;');
  };
  m.customProgramCacheKey = () => 'flora' + set.key + falloff + (twoSided ? '2' : '') + (parts ? 'p' : '') + (shade ? 's' : '') + (back ? 'b' : '') + (pair ? 'B' : '');
  if (o.alphaTest) { if (msaa === null) cardMats.push(m); else m.alphaToCoverage = msaa; }
  return m;
}

// ---------- the desert's own ground ----------
// The cover stops at its reach (60 to 110 m; a few creosote and yucca on to 250): past it the
// ground has the dark spots of the brush itself, coming in from about 40 m, a third darker,
// brush-sized, filtered to their average where a pixel holds several (no shimmer far off), in
// thicker and thinner stands; and
// the night's haze on the far ground is darker and greyer than the sky's, so the land ends under
// the horizon in a dark band (main.ts sets the fog's colour: at night it is itself the darker haze
// now, NIGHT_FOG, the colour of the painted horizon's foot, so here it is only greyer; at dawn it
// is the sky's again).
// High has two sizes of spot, Low one. On the desert ground's materials only: never a road or a lot.
const DESERT_VS = /* glsl */`
  vDgW = (modelMatrix * vec4(transformed, 1.0)).xyz;`;
const DESERT_FS = /* glsl */`
  float dgH(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }
  float dgN(vec2 p) {
    vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    return mix(mix(dgH(i), dgH(i + vec2(1.0, 0.0)), f.x), mix(dgH(i + vec2(0.0, 1.0)), dgH(i + vec2(1.0, 1.0)), f.x), f.y);
  }
  // the haze on the far land: at night greyer than the sky's (the fog is the darker night haze
  // already, main.ts), at dawn the sky's
  vec3 dgFog(vec3 c) {
    vec3 night = mix(vec3(dot(c, vec3(0.3, 0.45, 0.25))), c, 0.3);
    return mix(night, c, smoothstep(0.24, 0.42, c.r + c.g + c.b));
  }`;
const DESERT_MAP = /* glsl */`
  {
    vec2 dq = vDgW.xz * 0.42;
    float dgK = smoothstep(30.0, 90.0, distance(vDgW.xz, cameraPosition.xz)), dgW = length(fwidth(dq));
    if (dgK > 0.0) {
      #ifdef LAMBERT
      float dn = dgN(dq), dgP = 0.5;
      #else
      // and the thicker and thinner stands of it, twenty metres across
      float dn = dgN(dq) * 0.62 + dgN(dq * 2.9 + 17.0) * 0.38, dgP = dgN(vDgW.xz * 0.045 + 3.1);
      #endif
      float spot = mix(smoothstep(0.5, 0.66, dn), 0.3, smoothstep(0.3, 0.9, dgW));
      diffuseColor.rgb *= 1.0 - dgK * (0.1 + 0.12 * dgP + 0.3 * spot);
    }
  }`;
export function desertGround<T extends THREE.Material>(m: T): T {
  if (m.userData.desertGround) return m;
  m.userData.desertGround = true;
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey.bind(m);
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nvarying vec3 vDgW;')
      .replace('#include <project_vertex>', `#include <project_vertex>${DESERT_VS}`);
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', `#include <common>\nvarying vec3 vDgW;${DESERT_FS}`)
      .replace('#include <map_fragment>', `#include <map_fragment>${DESERT_MAP}`)
      // (an area that has its own haze on the land, the old road's at dawn, keeps it)
      .replace('#include <fog_fragment>', THREE.ShaderChunk.fog_fragment.replace('fogColor', 'dgFog(fogColor)'));
  };
  m.customProgramCacheKey = () => key() + '-desert';
  return m;
}

// The shapes are made once and shared by every patch.
let shapes: Record<FloraKind, THREE.BufferGeometry> | null = null;
function getShapes() {
  return shapes ??= { grass: grassShape(), sage: sageShape(), creosote: creosoteShape(), stalks: stalksShape(), succulent: succulentShape(), stones: stonesShape(), stain: stainShape() };
}
/** The kinds that share a mesh (one draw call for the two): cards on the one picture, the second
 *  with no more cards than the first (its triangles are the first's, the ones it has not are folded
 *  to a point). The rest have one each. */
const PAIRS: [FloraKind, FloraKind][] = [['grass', 'sage'], ['creosote', 'stalks']];
const pairShapes: Partial<Record<FloraKind, THREE.BufferGeometry>> = {};
/** a's mesh with b's corners in it too (aPosB, aUvB, aColB, aSwayB; the copies past the split
 *  take them, floraMaterial). */
function pairShape(a: FloraKind, b: FloraKind) {
  if (pairShapes[a]) return pairShapes[a]!;
  const sh = getShapes(), A = sh[a], Bg = sh[b], n = A.attributes.position.count, nb = Bg.attributes.position.count;
  const ia = A.index!.array, ib = Bg.index!.array;
  if (nb > n || ib.length > ia.length || Array.from(ib).some((v, i) => v !== ia[i]) || A.attributes.color.itemSize !== 3 || Bg.attributes.color.itemSize !== 3) throw new Error(`flora: ${b} does not fit in ${a}'s mesh`);
  const g = new THREE.BufferGeometry();
  for (const k of ['position', 'normal', 'uv', 'color', 'aSway']) g.setAttribute(k, A.attributes[k]);
  g.setIndex(A.index);
  // b's corners; the cards it has not are one point under the ground (no triangle to draw)
  const pad = (k: string, size: number, fill: number[]) => {
    const src = Bg.attributes[k].array as Float32Array, out = new Float32Array(n * size);
    out.set(src.subarray(0, nb * size));
    for (let i = nb; i < n; i++) out.set(fill, i * size);
    return new THREE.Float32BufferAttribute(out, size);
  };
  g.setAttribute('aPosB', pad('position', 3, [0, -1, 0]));
  g.setAttribute('aUvB', pad('uv', 2, [0, 0]));
  g.setAttribute('aColB', pad('color', 3, [0, 0, 0]));
  g.setAttribute('aSwayB', pad('aSway', 1, [0]));
  g.boundingSphere = (A.boundingSphere!.radius >= Bg.boundingSphere!.radius ? A : Bg).boundingSphere!.clone();
  return pairShapes[a] = g;
}

// ---------- the colours of the copies ----------
// Tints over the shape's own colours (sRGB, as the shapes): straw, rust and bleached grass, warm
// grey sage, the odd dead grey shrub; the stains are the dirt itself: a darker litter under brush,
// pale caliche, a red sand, a grey crust. Nothing cool: at night the moon makes it blue enough.
const lin = (c: readonly number[]) => { _lin.setRGB(c[0], c[1], c[2], THREE.SRGBColorSpace); return [_lin.r, _lin.g, _lin.b]; };
const PAL: Record<FloraKind, number[][]> = Object.fromEntries(Object.entries({
  grass: [[1, 0.94, 0.8], [0.94, 0.9, 0.8], [1, 0.85, 0.72], [1.02, 0.98, 0.9], [0.8, 0.74, 0.66]],
  sage: [[1, 0.98, 0.9], [0.94, 0.93, 0.84], [1, 0.95, 0.88], [0.9, 0.84, 0.76]],
  creosote: [[0.98, 1, 0.9], [0.9, 0.92, 0.82], [1.04, 1, 0.86]],
  stalks: [[1, 1, 1], [0.9, 0.88, 0.85], [1.04, 0.97, 0.9]],
  succulent: [[0.98, 1, 0.94], [0.92, 0.94, 0.86], [1, 0.97, 0.9]],
  stones: [[1, 0.98, 0.95], [0.9, 0.86, 0.82], [1.04, 0.94, 0.86], [0.86, 0.84, 0.82]],
  stain: [[0.34, 0.28, 0.22], [0.7, 0.66, 0.58], [0.6, 0.45, 0.34], [0.5, 0.48, 0.44], [0.28, 0.24, 0.19]],
}).map(([k, list]) => [k, list.map(lin)])) as Record<FloraKind, number[][]>;
/** Broom snakeweed, the commonest little shrub of the grassland: a sage copy, smaller, yellow-green. */
const SNAKEWEED = lin([0.98, 1.04, 0.66]), SNAKEWEED_SHARE = 0.32;
/** The litter under a big tuft or a shrub (the stains): old leaves and shade, dark. */
const LITTER = [[0.22, 0.18, 0.14], [0.27, 0.22, 0.17]].map(lin), LITTER_SHARE = 0.25;
/** A pale caliche stone now and then among the dark ones. */
const CALICHE = lin([1.55, 1.48, 1.32]);

// ---------- the patch ----------
type KindCell = { m: Float32Array; c: Float32Array; rank: Float32Array; seam: Uint8Array | null; n: number };
/** A kind in its mesh: its copies are at..at+n in it now (a pair's second kind after the first's).
 *  seam: which of them grow in a way's seam (the tests). */
type Layer = { k: Kind; mesh: THREE.InstancedMesh; reach: { value: number }; seam: Uint8Array; at: number; n: number };
/** A mesh and the kinds in it, in order; split: where the second kind's copies start (a pair),
 *  reachB its reach (the material's second). */
type Draw = { mesh: THREE.InstancedMesh; layers: Layer[]; split: { value: number }; reachB: { value: number } };
const live: Flora[] = [];
const _p = new THREE.Vector3(), _q = new THREE.Quaternion(), _q2 = new THREE.Quaternion(), _e = new THREE.Euler(), _s = new THREE.Vector3(), _n = new THREE.Vector3(), _mx = new THREE.Matrix4();
const _up = new THREE.Vector3(0, 1, 0);
const inside = (b: Rect, x: number, z: number, m = 0) => x >= b.minX - m && x <= b.maxX + m && z >= b.minZ - m && z <= b.maxZ + m;

export class Flora {
  readonly group = new THREE.Group();
  readonly layers: Layer[] = [];
  private readonly draws: Draw[] = [];
  private cells = new Map<number, (KindCell | undefined)[]>();
  private at = new THREE.Vector2(Infinity, Infinity);
  private q = '';
  private dirty = true;
  /** The things standing and the walks, by 16 m squares (each in every square it reaches into). */
  private bucket = new Map<number, { b: Rect[]; w: Rect[] }>();

  constructor(readonly spec: FloraSpec) {
    this.group.name = 'flora:' + spec.name;
    const sh = getShapes(), fall = spec.falloff ?? 0.012;
    for (const k of KINDS) {
      // (the second of a pair goes into the first's mesh, faded by the material's second reach)
      const pairOf = PAIRS.find((p) => p[1] === k.id);
      if (pairOf) {
        const D = this.draws.find((d) => d.layers[0].k.id === pairOf[0])!;
        const L: Layer = { k, mesh: D.mesh, reach: D.reachB, seam: new Uint8Array(k.cap), at: 0, n: 0 };
        D.layers.push(L); this.layers.push(L);
        continue;
      }
      const reach = { value: k.reach };
      const pair = PAIRS.find((p) => p[0] === k.id), second = pair ? KINDS.find((q) => q.id === pair[1])! : null;
      const split = { value: 0 }, reachB = { value: second ? second.reach : 0 };
      const card = k.id === 'grass' || k.id === 'sage' || k.id === 'creosote' || k.id === 'stalks';
      const o: THREE.MeshStandardMaterialParameters = card ? { side: THREE.DoubleSide, map: floraAtlas(), alphaTest: 0.42 }
        : k.id === 'succulent' ? { side: THREE.DoubleSide }
          : k.id === 'stain' ? { transparent: true, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }
            : { flatShading: true };
      const plant = card || k.id === 'succulent';
      const mat = floraMaterial(o, spec.set, fall, reach, k.id === 'succulent', plant, card, pair ? { reach: reachB, split } : null);
      const mesh = new THREE.InstancedMesh(pair ? pairShape(pair[0], pair[1]) : sh[k.id], mat, k.cap + (second ? second.cap : 0));
      mesh.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
      mesh.setColorAt(0, new THREE.Color(1, 1, 1));
      mesh.instanceColor!.setUsage(THREE.DynamicDrawUsage);
      mesh.count = 0; mesh.visible = false;
      mesh.boundingSphere = new THREE.Sphere(new THREE.Vector3(), 1);
      mesh.name = 'flora-' + (pair ? pair.join('+') : k.id);
      mesh.userData.noShadow = true;   // core/ultra.ts: too many and too thin to cast, and the cards
      mesh.userData.noAO = true;       // would be solid quads in the occlusion pass (core/vhs.ts)
      mesh.userData.noMerge = true;
      mesh.raycast = () => {};
      if (k.id === 'stain') mesh.renderOrder = -1;
      this.group.add(mesh);
      const L: Layer = { k, mesh, reach, seam: new Uint8Array(k.cap), at: 0, n: 0 };
      this.layers.push(L);
      this.draws.push({ mesh, layers: [L], split, reachB });
    }
    this.add({ blocks: spec.blocks, walks: spec.walks });
    live.push(this);
  }

  /** More things that stand, or more ground walked (an area built later: the diner's, the
   *  town's): the cells round them are drawn again. */
  add(o: { blocks?: Rect[]; walks?: Rect[] }) {
    const put = (r: Rect, w: boolean) => {
      const m = 1.4;
      for (let i = Math.floor((r.minX - m) / 16); i <= Math.floor((r.maxX + m) / 16); i++) for (let j = Math.floor((r.minZ - m) / 16); j <= Math.floor((r.maxZ + m) / 16); j++) {
        const key = i * 65536 + j;
        let e = this.bucket.get(key); if (!e) this.bucket.set(key, e = { b: [], w: [] });
        (w ? e.w : e.b).push(r);
      }
      // the cells it touches are made again
      for (let i = Math.floor((r.minX - m) / C); i <= Math.floor((r.maxX + m) / C); i++) for (let j = Math.floor((r.minZ - m) / C); j <= Math.floor((r.maxZ + m) / C); j++) this.cells.delete(this.key(i, j));
    };
    for (const r of o.blocks ?? []) put(r, false);
    for (const r of o.walks ?? []) put(r, true);
    this.dirty = true;
  }

  private key(i: number, j: number) { return (i + 40000) * 80000 + (j + 40000); }

  /** The cover at a point for a kind, with the things standing and the walks, and its drifts
   *  (noise: the patch's and the clump's, made first by noiseAt); walked says the player goes
   *  there, seamed that it is in a way's seam, spilt that it is in the gravel spilt off a way. */
  private walked = false;
  private seamed = false;
  private spilt = false;
  private noiseAt(x: number, z: number, k: Kind) {
    let n = k.patch > 1 ? 0.3 + 1.4 * vnoise(x, z, k.patch, this.spec.seed + k.patch) : 1;
    // drifts a few metres across with bare dirt between (raised to a power: most of it thin, some thick)
    if (k.clump) n *= CLUMP_K * Math.pow(vnoise(x, z, k.clump, this.spec.seed + 911), 2.5);
    return n * (this.spec.mix?.[k.id] ?? 1);
  }
  private coverAt(x: number, z: number, k: Kind, noise: number) {
    this.walked = false; this.seamed = false; this.spilt = false;
    let d = this.spec.cover(x, z);
    if (d <= 0) {
      // on a way: only in its seams, tiny grass and pebbles
      if (!k.seam || !this.spec.seam) return 0;
      d = this.spec.seam(x, z) * k.seam;
      if (d <= 0) return 0;
      this.seamed = true;
    }
    const e = this.bucket.get(Math.floor(x / 16) * 65536 + Math.floor(z / 16));
    if (e) {
      let near = false;
      for (const b of e.b) { if (inside(b, x, z, 0.12)) return 0; if (!near && inside(b, x, z, 1.3)) near = true; }
      if (near && !this.seamed) d *= 1.7;
      for (const w of e.w) if (inside(w, x, z)) { this.walked = true; break; }
    }
    if (k.id === 'stones' && !this.seamed && this.spec.spill) { const s = this.spec.spill(x, z); if (s > 1) { d *= s; this.spilt = true; } }
    return d * noise;
  }

  private cell(i: number, j: number, ki: number): KindCell {
    const key = this.key(i, j);
    let c = this.cells.get(key);
    if (!c) { c = []; this.cells.set(key, c); }
    return c[ki] ??= this.make(i, j, ki);
  }
  /** The shrubs of a cell the grass and the stalks stand round, as [x, z, size, ...]. */
  private nurses(i: number, j: number, kinds: FloraKind[]) {
    const out: number[] = [];
    for (const id of kinds) {
      const c = this.cell(i, j, KINDS.findIndex((k) => k.id === id));
      for (let t = 0; t < c.n; t++) out.push(c.m[t * 16 + 12], c.m[t * 16 + 14], Math.hypot(c.m[t * 16], c.m[t * 16 + 1], c.m[t * 16 + 2]));
    }
    return out;
  }
  // A cell's copies of a kind: drawn the same every time from the cell's own random numbers (the
  // same count of them for every candidate, kept or not), sorted by rank so Low keeps the first.
  private make(i: number, j: number, ki: number): KindCell {
    const k = KINDS[ki], r = mulberry(hash4(i, j, ki, this.spec.seed));
    const want = k.rate * C * C * k.most, n = Math.floor(want) + (r() < want % 1 ? 1 : 0);
    const out: { m: number[]; c: number[]; rank: number; seam: boolean }[] = [];
    const pal = PAL[k.id], g = this.spec.ground;
    const nurse = k.nurse && NURSE[k.id] ? this.nurses(i, j, NURSE[k.id]!) : null, nn = nurse ? nurse.length / 3 : 0;
    const put = (x: number, y: number, z: number, c: readonly number[], rank: number, seam: boolean) => {
      _mx.compose(_p.set(x, y, z), _q, _s);
      out.push({ m: Array.from(_mx.elements), c: [c[0], c[1], c[2]], rank, seam });
    };
    const tint = (cA: number, cB: number, f: number) => {
      const c0 = pal[Math.floor(cA * pal.length) % pal.length], c1 = pal[Math.floor(cB * pal.length) % pal.length];
      return [c0[0] + (c1[0] - c0[0]) * f, c0[1] + (c1[1] - c0[1]) * f, c0[2] + (c1[2] - c0[2]) * f];
    };
    for (let t = 0; t < n; t++) {
      const u = r(), v = r(), a = r(), b = r(), cA = r(), cB = r(), ry = r(), l1 = r(), l2 = r(), rank = r(), n1 = r(), n2 = r(), n3 = r();
      let x = (i + u) * C, z = (j + v) * C;
      // most of the grass and the stalks stand 0.4 to 1.5 m from a shrub of the cell
      if (nn && n1 < k.nurse!) {
        const q = Math.floor(n2 * nn) * 3, an = n3 * Math.PI * 2, dist = nurse![q + 2] * 0.3 + 0.4 + v * 1.1;
        x = nurse![q] + Math.cos(an) * dist; z = nurse![q + 1] + Math.sin(an) * dist;
      }
      const noise = this.noiseAt(x, z, k);
      if (a * k.most >= noise * 4.2) continue;   // (the cover is at most about that much, near a wall in a ditch)
      const d = this.coverAt(x, z, k, noise);
      if (a * k.most >= Math.min(d, k.most)) continue;
      // big where it is thick, small and mostly tiny in the open
      const dens = Math.min(1, d / k.most);
      let s = k.size[0] + Math.pow(b, 2.5 - 1.7 * dens + (k.id === 'stones' ? 1.3 : 0)) * (k.size[1] - k.size[0]);
      if (this.walked) { if (k.walk <= 0 || (cB * 7.31) % 1 > k.walk) continue; s = Math.min(s, k.walkMax); }
      // in a seam tiny grass and pebbles; the gravel spilt off a way small
      if (this.seamed) s = k.id === 'grass' ? 0.25 + b * 0.2 : 0.3 + b * 0.2;
      else if (this.spilt) s = Math.min(s, 0.3 + b * 0.35);
      const y = g(x, z);
      let sy = s, col = tint(cA, cB, l2 * 0.5);
      if (k.id === 'stones' || k.id === 'stain') {
        // flat on the ground's slope; a stain only where the ground is near flat across it
        const h = k.id === 'stain' ? s : 0.4;
        const hx = g(x + h, z) - g(x - h, z), hz = g(x, z + h) - g(x, z - h);
        if (k.id === 'stain' && Math.abs(g(x + h, z) + g(x - h, z) - 2 * y) + Math.abs(g(x, z + h) + g(x, z - h) - 2 * y) > 0.05) continue;
        _n.set(-hx / (2 * h), 1, -hz / (2 * h)).normalize();
        _q.setFromUnitVectors(_up, _n).multiply(_q2.setFromAxisAngle(_up, ry * Math.PI * 2));
        if (k.id === 'stones') {
          _q.multiply(_q2.setFromEuler(_e.set((l1 - 0.5) * 0.4, 0, (l2 - 0.5) * 0.4))); sy = s * (0.7 + l1 * 0.6);
          if (n1 < 0.06) col = col.map((c, q) => c * CALICHE[q]);
        }
        _s.set(s * (k.id === 'stain' ? 0.8 + l2 * 0.5 : 1), sy, s);
        // the stones a third in the dirt; the stains (ranked first: litter comes after them) on it
        put(x, y + (k.id === 'stain' ? 0.02 : -0.025 * sy), z, col, k.id === 'stain' ? rank * 0.5 : rank, this.seamed);
      } else {
        const lean = k.id === 'grass' ? 0.16 : k.id === 'creosote' ? 0.1 : 0.08;
        _q.setFromEuler(_e.set((l1 - 0.5) * lean * 2, ry * Math.PI * 2, (l2 - 0.5) * lean * 2, 'YXZ'));
        sy = s * (0.8 + l1 * 0.45);
        // a third of the sage copies are snakeweed: smaller, rounder, yellow-green
        if (k.id === 'sage' && n2 < SNAKEWEED_SHARE && !nn) { s *= 0.62; sy = s * (0.75 + l1 * 0.3); col = col.map((c, q) => c * SNAKEWEED[q]); }
        // two in five succulents are cholla: their copies are mirrored (succulentShape)
        _s.set(k.id === 'succulent' && (cA * 13.7) % 1 < 0.4 ? -s : s, sy, s);
        put(x, y - 0.035 * s, z, col, rank, this.seamed);
      }
    }
    // the litter under one in four of the big tufts and the shrubs: a small dark stain, ranked
    // after the stains (Kind.far: near the camera only)
    if (k.id === 'stain') for (const [id, big, w] of [['grass', 1.0, 0.42], ['sage', 0.8, 0.52], ['creosote', 0.7, 0.5]] as const) {
      const c = this.cell(i, j, KINDS.findIndex((q) => q.id === id));
      for (let t = 0; t < c.n; t++) {
        const s = Math.hypot(c.m[t * 16], c.m[t * 16 + 1], c.m[t * 16 + 2]), h1 = r(), h2 = r(), h3 = r();
        if (s < big || h1 >= LITTER_SHARE) continue;
        const x = c.m[t * 16 + 12] + (h2 - 0.5) * 0.2 * s, z = c.m[t * 16 + 14] + (h3 - 0.5) * 0.2 * s, y = g(x, z), sz = s * w * (0.85 + h2 * 0.3);
        if (Math.abs(g(x + sz, z) + g(x - sz, z) - 2 * y) + Math.abs(g(x, z + sz) + g(x, z - sz) - 2 * y) > 0.05) continue;
        _n.set(-(g(x + sz, z) - g(x - sz, z)) / (2 * sz), 1, -(g(x, z + sz) - g(x, z - sz)) / (2 * sz)).normalize();
        _q.setFromUnitVectors(_up, _n).multiply(_q2.setFromAxisAngle(_up, h3 * Math.PI * 2));
        _s.set(sz * (0.85 + h1), sz, sz);
        put(x, y + 0.018, z, LITTER[h2 < 0.5 ? 0 : 1], 0.5 + h3 * 0.5, false);
      }
    }
    out.sort((p, q) => p.rank - q.rank);
    const cell: KindCell = { m: new Float32Array(out.length * 16), c: new Float32Array(out.length * 3), rank: new Float32Array(out.length), seam: null, n: out.length };
    if (out.some((o) => o.seam)) cell.seam = new Uint8Array(out.length);
    out.forEach((o, t) => { cell.m.set(o.m, t * 16); cell.c.set(o.c, t * 3); cell.rank[t] = o.rank; if (o.seam) cell.seam![t] = 1; });
    return cell;
  }

  /** Whether the patch is drawn now (its area shown). */
  get shown() {
    for (let o: THREE.Object3D | null = this.group; o; o = o.parent) { if (!o.visible) return false; if ((o as THREE.Scene).isScene) return true; }
    return false;
  }
  private local = new THREE.Vector3();
  private look = new THREE.Vector3();
  private inv = new THREE.Matrix4();
  /** Which way the meshes were filled for (radians about y, in the patch's terms), and how far to
   *  either side (PI: all round). */
  private yaw = 0;
  private half = Math.PI;
  /** Fill the meshes round the camera, when it has moved or turned far enough (followFlora). */
  follow(cam: THREE.Camera) {
    if (!this.shown) return;
    const p = this.group.worldToLocal(cam.getWorldPosition(this.local));
    // where it looks: what is behind it is not drawn (unless it looks down at its feet)
    const d = cam.getWorldDirection(this.look).transformDirection(this.inv.copy(this.group.matrixWorld).invert());
    const pc = cam as THREE.PerspectiveCamera, yaw = Math.atan2(d.x, d.z);
    const half = Math.hypot(d.x, d.z) < 0.35 ? Math.PI : Math.min(Math.PI, Math.atan(Math.tan(pc.fov * Math.PI / 360) * pc.aspect) + CONE);
    const turned = this.half < Math.PI && (Math.abs(Math.atan2(Math.sin(yaw - this.yaw), Math.cos(yaw - this.yaw))) > TURN || half > this.half + 1e-3);
    const q = getQuality();
    if (!this.dirty && q === this.q && !turned && Math.hypot(p.x - this.at.x, p.z - this.at.y) < STEP) {
      // the cells further out are made a little each picture; the ones a fill had to leave out
      // are put in once they are all made
      if (this.ahead(this.at.x, this.at.y) && this.pending) this.fill(this.at.x, this.at.y, q === 'low');
      return;
    }
    this.yaw = yaw; this.half = half;
    // after a jump (a new area, a place out of a save, the first picture) all is made at once
    this.fill(p.x, p.z, q === 'low', Math.hypot(p.x - this.at.x, p.z - this.at.y) > JUMP);
    this.at.set(p.x, p.z); this.q = q; this.dirty = false;
    this.next = { kind: 0, at: 0 };
  }
  /** Where making cells ahead of need has got to (a kind, and a place in OFFSETS). */
  private next = { kind: 0, at: 0 };
  /** A fill left out cells further off that were not made yet. */
  private pending = false;
  /** Make the cells all round the camera out past each kind's reach, nearest first, for at most
   *  AHEAD_MS in one picture; whether they are all made. Walking on or turning then only copies. */
  private ahead(px: number, pz: number) {
    if (this.next.kind >= KINDS.length) return true;
    const t0 = performance.now(), ci = Math.floor(px / C), cj = Math.floor(pz / C);
    for (; this.next.kind < KINDS.length; this.next.kind++, this.next.at = 0) {
      const ki = this.next.kind, R = KINDS[ki].reach + 3 * STEP;
      for (; this.next.at < OFFSETS.length; this.next.at++) {
        const [dx, dz, dist] = OFFSETS[this.next.at];
        if (dist > R + C) break;
        if (this.cells.get(this.key(ci + dx, cj + dz))?.[ki]) continue;
        this.cell(ci + dx, cj + dz, ki);
        if (performance.now() - t0 > AHEAD_MS) return false;
      }
    }
    return true;
  }
  private fill(px: number, pz: number, low: boolean, all = false) {
    const ci = Math.floor(px / C), cj = Math.floor(pz / C), cr = C * 0.71;
    this.pending = false;
    // a cell is in front when its centre is within the cone, widened by the cell's own size: once
    // for each place in OFFSETS, for all the kinds (-1 not worked out yet, 0 behind, 1 in front)
    const seen = FRONT.fill(-1);
    const front = (o: number, i: number, j: number) => {
      if (this.half >= Math.PI) return true;
      if (seen[o] < 0) {
        const vx = (i + 0.5) * C - px, vz = (j + 0.5) * C - pz, dd = Math.hypot(vx, vz), a = Math.atan2(vx, vz) - this.yaw;
        seen[o] = dd < NEAR + cr || Math.abs(Math.atan2(Math.sin(a), Math.cos(a))) <= this.half + Math.asin(Math.min(1, cr / dd)) ? 1 : 0;
      }
      return seen[o] === 1;
    };
    // (the kinds in KINDS' order: the second of a pair goes on after the first in their mesh)
    let at = 0, R0 = 0;
    this.layers.forEach((L, ki) => {
      const k = L.k, keep = low ? k.low : 1, mesh = L.mesh, far = low ? null : k.far;
      const D = this.draws.find((d) => d.mesh === mesh)!, first = D.layers[0] === L;
      if (first) { at = 0; R0 = 0; }
      // Low: none of the far ring, and nearer
      L.reach.value = low ? (k.far ? k.far[0] : k.reach) * LOW_REACH : k.reach;
      let n = 0;
      L.at = at;
      if (keep > 0) {
        const R = L.reach.value + STEP, M = mesh.instanceMatrix.array as Float32Array, Cc = mesh.instanceColor!.array as Float32Array;
        // after a jump, everything out to the near reach at once (the far ring a picture or two later)
        const now = Math.max(SYNC, all ? (k.far ? k.far[0] : R) : 0);
        L.seam.fill(0);
        for (let o = 0; o < OFFSETS.length; o++) {
          const [dx, dz, dist] = OFFSETS[o];
          if (dist > R + C * 1.5) break;
          const i = ci + dx, j = cj + dz;
          const nx = Math.max(i * C, Math.min(px, (i + 1) * C)) - px, nz = Math.max(j * C, Math.min(pz, (j + 1) * C)) - pz;
          const d2 = nx * nx + nz * nz;
          if (d2 > R * R || !front(o, i, j)) continue;
          // near ones are made now; further off, one not made yet comes in a picture or two later
          const cell = this.cells.get(this.key(i, j))?.[ki] ?? (d2 < now * now ? this.cell(i, j, ki) : null);
          if (!cell) { this.pending = true; continue; }
          // past far[0] only the first share of it by rank, easing down over three cells
          const share = far && dist > far[0] ? 1 + (far[1] - 1) * Math.min(1, (dist - far[0]) / (3 * C)) : 1;
          let m = cell.n;
          if (keep * share < 1) { m = 0; while (m < cell.n && cell.rank[m] < keep * share) m++; }
          m = Math.min(m, k.cap - n);
          if (m <= 0) continue;
          M.set(cell.m.subarray(0, m * 16), (at + n) * 16);
          Cc.set(cell.c.subarray(0, m * 3), (at + n) * 3);
          if (cell.seam) L.seam.set(cell.seam.subarray(0, m), n);
          n += m;
          if (n >= k.cap) break;
        }
        R0 = Math.max(R0, R);
      }
      L.n = n; at += n;
      if (first) D.split.value = n;
      if (L !== D.layers[D.layers.length - 1]) return;
      // the mesh, once its kinds are in
      mesh.instanceMatrix.clearUpdateRanges(); mesh.instanceMatrix.addUpdateRange(0, at * 16); mesh.instanceMatrix.needsUpdate = true;
      mesh.instanceColor!.clearUpdateRanges(); mesh.instanceColor!.addUpdateRange(0, at * 3); mesh.instanceColor!.needsUpdate = true;
      mesh.boundingSphere!.center.set(px, this.spec.ground(px, pz), pz);
      mesh.boundingSphere!.radius = R0 + 10;
      mesh.count = at;
      mesh.visible = at > 0;
    });
    // forget the cells far behind
    if (this.cells.size > 2500) for (const key of [...this.cells.keys()]) {
      const i = Math.floor(key / 80000) - 40000, j = key % 80000 - 40000;
      if (Math.hypot((i + 0.5) * C - px, (j + 0.5) * C - pz) > MAX_R * 1.25) this.cells.delete(key);
    }
  }

  // ---------- tests ----------
  /** The copies in the meshes now, by kind. */
  drawn() { const o: Partial<Record<FloraKind, number>> = {}; for (const L of this.layers) if (L.mesh.visible) o[L.k.id] = L.n; return o; }
  /** Every copy in the meshes now that stands on a way the story uses (spec.ways), as [kind, x, z,
   *  way]; one in a seam only when it is further in than the seam's 0.6 m (no point 0.65 m round
   *  it off the way), or bigger than the seam's kinds grow there. */
  onWays() {
    const out: [FloraKind, number, number, string][] = [], ways = this.spec.ways;
    if (!ways) return out;
    for (const L of this.layers) {
      const a = L.mesh.instanceMatrix.array as Float32Array;
      for (let i = L.at; i < L.at + L.n; i++) {
        const x = a[i * 16 + 12], z = a[i * 16 + 14], w = ways(x, z);
        if (!w) continue;
        if (L.seam[i - L.at] && Math.hypot(a[i * 16], a[i * 16 + 1], a[i * 16 + 2]) <= 0.5) {
          let edge = false;
          for (let t = 0; t < 8 && !edge; t++) edge = !ways(x + Math.cos(t * Math.PI / 4) * 0.65, z + Math.sin(t * Math.PI / 4) * 0.65);
          if (edge) continue;
        }
        out.push([L.k.id, Math.round(x * 10) / 10, Math.round(z * 10) / 10, w]);
      }
    }
    return out;
  }
  /** The copies in the meshes now that grow in a way's seam, and how many of those are on a way. */
  seams() {
    let n = 0, on = 0;
    for (const L of this.layers) {
      const a = L.mesh.instanceMatrix.array as Float32Array;
      for (let i = L.at; i < L.at + L.n; i++) if (L.seam[i - L.at]) { n++; if (this.spec.ways?.(a[i * 16 + 12], a[i * 16 + 14])) on++; }
    }
    return { n, on };
  }
}

/** Every patch follows the camera that draws the scene: before each picture, the meshes of
 *  the patches shown are filled round it, and the wind moves on. Once, for the one scene. */
export function followFlora(scene: THREE.Scene) {
  if (scene.userData.flora) return;
  scene.userData.flora = true;
  const prev = scene.onBeforeRender;
  scene.onBeforeRender = function (this: THREE.Scene, ...args: Parameters<THREE.Object3D['onBeforeRender']>) {
    prev.apply(this, args);
    const cam = args[2];
    // the cards' cut-out edges soften with MSAA (a PC: alpha to coverage); a phone keeps the
    // plain alpha test (core/quality.ts's Low copies never take it)
    if (msaa === null) {
      msaa = !!args[0].getContext().getContextAttributes()?.antialias;
      if (msaa) for (const m of cardMats) { m.alphaToCoverage = true; m.needsUpdate = true; }
      cardMats.length = 0;
    }
    windAt(performance.now() / 1000);
    if ((cam as THREE.PerspectiveCamera).isPerspectiveCamera) for (const f of live) f.follow(cam);
  };
}
/** Tests (S47.world.flora, tools/env_ground.py): the copies drawn now by kind, over the patches
 *  shown; the ones on a way the story uses; the wind. */
export const floraTest = {
  wind,
  patches: live as readonly Flora[],
  drawn() {
    const o: Partial<Record<FloraKind, number>> = {};
    for (const f of live) if (f.shown) for (const [k, n] of Object.entries(f.drawn()) as [FloraKind, number][]) o[k] = (o[k] ?? 0) + n;
    return o;
  },
  onWays() { return live.filter((f) => f.shown).flatMap((f) => f.onWays().map((w) => [f.spec.name, ...w])); },
  seams() { return live.filter((f) => f.shown).reduce((o, f) => { const s = f.seams(); return { n: o.n + s.n, on: o.on + s.on }; }, { n: 0, on: 0 }); },
};
