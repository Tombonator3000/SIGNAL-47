import * as THREE from 'three';

// Layout of the drive in metres, local to RoadArea.group (whose origin is the start).
// North is -Z. The state highway runs north-south along x = 0, and the truck starts in the
// southbound lane, heading south (+Z). SARO lies about 300 m north, behind the start.
// About 520 m south the graded survey track leaves to the west (the right), crosses the
// right-of-way fence on a cattle guard and runs about 600 m to the STATION 01 gate.

export const HWY = { paved: 4.8, shoulder: 6, flat: 16.5, margin: 18, fence: 21, z0: -2000, z1: 2200 };
export const TRK = { half: 2.3, soft: 0.7, flat: 11.5, margin: 13 };
export const GUARD = { x0: -22.2, x1: -19.8, z0: 517.6, z1: 522.4 };
export const START = { x: -1.8, z: 0, heading: Math.PI };
export const GATE = { x: -612, z: 532, half: 5 };                  // closed double gate, station fence
export const END = { x: -603, z: 532, heading: Math.PI / 2 };       // parked, facing the gate
export const ENDZONE = { minX: -609, maxX: -585, minZ: 524, maxZ: 540 };
export const PASTURE = { west: -760, north: 300, south: 720 };
export const DRIVE_N = -120, DRIVE_S = 900;                          // invisible ends of the highway

export const smooth = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
export const hash = (x: number, z: number) => { const s = Math.sin(x * 12.9898 + z * 78.233) * 43758.5453; return s - Math.floor(s); };

// ---------- the survey track: a smooth curve through hand-placed points, every 3 m ----------
const KEYS = [[-5, 520], [-24, 520], [-70, 518], [-150, 507], [-240, 496], [-330, 497], [-420, 508], [-500, 522], [-560, 530], [-606, 532]];
export const track = (() => {
  const curve = new THREE.CatmullRomCurve3(KEYS.map(([x, z]) => new THREE.Vector3(x, 0, z)), false, 'centripetal');
  const pts = curve.getSpacedPoints(Math.ceil(curve.getLength() / 3)).map((p) => new THREE.Vector2(p.x, p.z));
  const s = [0];
  for (let i = 1; i < pts.length; i++) s.push(s[i - 1] + pts[i].distanceTo(pts[i - 1]));
  return { pts, s, length: s[s.length - 1] };
})();
// The mouth by the highway is wider, so the turn can be made at a crawl.
export const trackHalf = (s: number) => TRK.half + 2.6 * (1 - smooth(0, 15, s));

const near = { d: Infinity, s: 0 };
/** Distance to the track centreline and the arc length there. Reuses one result object. */
export function trackNearest(x: number, z: number) {
  near.d = Infinity; near.s = 0;
  if (x > 40 || x < -650 || z < 450 || z > 580) return near;
  const p = track.pts;
  for (let i = 0; i < p.length - 1; i++) {
    const ax = p[i].x, az = p[i].y, dx = p[i + 1].x - ax, dz = p[i + 1].y - az;
    const L2 = dx * dx + dz * dz;
    const t = Math.min(1, Math.max(0, ((x - ax) * dx + (z - az) * dz) / L2));
    const ex = ax + dx * t - x, ez = az + dz * t - z, d = ex * ex + ez * ez;
    if (d < near.d) { near.d = d; near.s = track.s[i] + Math.sqrt(L2) * t; }
  }
  near.d = Math.sqrt(near.d);
  return near;
}

// ---------- heights ----------
const base = (x: number, z: number) => 1.1 * Math.sin(x * 0.0061 + 0.7) * Math.cos(z * 0.0047 + 0.3)
  + 0.7 * Math.sin((x + z) * 0.0032 + 1.9) + 0.4 * Math.sin(z * 0.011 - x * 0.004 + 0.4);
const B0 = base(START.x, START.z);
const washAt = (x: number, z: number) => { const w = x + 330 + 0.25 * (z - 500); return Math.exp(-w * w / 520) * smooth(-150, 60, z); };
/** The graded land the roads follow: long swells, a dry wash across the track, hills far out. */
export function hLow(x: number, z: number) {
  const dx = x + 250, dz = z - 300, a = Math.atan2(dz, dx);
  const hills = smooth(1300, 2400, Math.hypot(dx, dz)) * (12 + 9 * Math.sin(a * 5 + 1.3) + 5 * Math.sin(a * 13));
  return base(x, z) - B0 - 1.3 * washAt(x, z) + hills;
}
const detail = (x: number, z: number) => 0.16 * Math.sin(x * 0.13 + 1.7) * Math.sin(z * 0.11 + 0.3) + 0.08 * Math.sin(x * 0.31 - z * 0.23 + 0.9);
/** Height of the desert grid at a vertex: small bumps away from the roads, sunk under the road strips. */
function gridH(x: number, z: number) {
  const hd = Math.abs(x), td = trackNearest(x, z).d;
  let h = hLow(x, z) + detail(x, z) * smooth(0, 15, Math.min(hd - HWY.margin, td - TRK.margin));
  if (hd < HWY.flat || td < TRK.flat) h -= 0.45; // hidden under the road strips: no z-fighting
  return h;
}
/** Colour of the desert at a point: slow patches, grain, pale sand in the wash. */
export function tint(x: number, z: number, out: number[], k = 0) {
  const n = 0.8 + 0.12 * Math.sin(x * 0.013) * Math.cos(z * 0.011) + 0.16 * (hash(Math.round(x), Math.round(z)) - 0.5);
  const sand = 0.22 * washAt(x, z);
  out[k] = n + sand; out[k + 1] = n * 0.98 + sand * 0.9; out[k + 2] = n * 0.95 + sand * 0.75;
}

// ---------- the desert grid: 10 m cells over the drive, wider cells out to the horizon ----------
function axis(c0: number, c1: number, step: number, lo: number, hi: number) {
  const mid: number[] = [], left: number[] = [], right: number[] = [];
  for (let v = c0; v <= c1 + 1e-6; v += step) mid.push(v);
  for (let s = step, v = c0; v > lo;) { s = Math.min(120, s * 1.25); v = Math.max(lo, v - s); left.push(v); }
  for (let s = step, v = c1; v < hi;) { s = Math.min(120, s * 1.25); v = Math.min(hi, v + s); right.push(v); }
  return [...left.reverse(), ...mid, ...right];
}
export const GX = axis(-800, 60, 10, -3000, 2300), GZ = axis(-480, 1040, 10, -2700, 3300);
export const gridHeights = new Float32Array(GX.length * GZ.length);
for (let j = 0; j < GZ.length; j++) for (let i = 0; i < GX.length; i++) gridHeights[j * GX.length + i] = gridH(GX[i], GZ[j]);

const cell = (a: number[], v: number) => { let lo = 0, hi = a.length - 2; while (lo < hi) { const m = (lo + hi + 1) >> 1; if (a[m] <= v) lo = m; else hi = m - 1; } return lo; };
/** The grid surface exactly as drawn (same triangle split as groundGeometry). */
export function gridY(x: number, z: number) {
  const i = cell(GX, x), j = cell(GZ, z), n = GX.length, H = gridHeights;
  const u = Math.min(1, Math.max(0, (x - GX[i]) / (GX[i + 1] - GX[i]))), v = Math.min(1, Math.max(0, (z - GZ[j]) / (GZ[j + 1] - GZ[j])));
  const a = H[j * n + i], b = H[j * n + i + 1], c = H[(j + 1) * n + i], d = H[(j + 1) * n + i + 1];
  return u + v <= 1 ? a + (b - a) * u + (c - a) * v : d + (c - d) * (1 - u) + (b - d) * (1 - v);
}
const hwyLift = (ax: number) => ax <= HWY.paved ? 0.06 + 0.06 * (1 - (ax / HWY.paved) ** 2) : 0.06 * (HWY.shoulder - ax) / (HWY.shoulder - HWY.paved);
const trkLift = (d: number, w: number) => d <= w ? 0.03 + 0.02 * (1 - (d / w) ** 2) : 0.03 * (w + TRK.soft - d) / TRK.soft;
const dropY = (d: number, flat: number, edge: number) => d <= flat ? 0 : -0.6 * (d - flat) / (edge - flat);
/** Height of whatever is on top at (x, z): road, track, verge or desert. */
export function surfaceY(x: number, z: number) {
  const ax = Math.abs(x);
  if (ax <= HWY.shoulder) return hLow(x, z) + hwyLift(ax);
  const t = trackNearest(x, z), w = trackHalf(t.s);
  if (t.d <= w + TRK.soft) return hLow(x, z) + trkLift(t.d, w);
  let y = gridY(x, z);
  if (ax <= HWY.margin) y = Math.max(y, hLow(x, z) + dropY(ax, HWY.flat, HWY.margin));
  if (t.d <= TRK.margin) y = Math.max(y, hLow(x, z) + dropY(t.d, TRK.flat, TRK.margin));
  return y;
}

// ---------- geometry ----------
function attrs(g: THREE.BufferGeometry, pos: number[], uv: number[], col: number[] | null, idx: number[]) {
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(new Array(pos.length).fill(0).map((_, i) => (i % 3 === 1 ? 1 : 0)), 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  if (col) g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  g.setIndex(idx);
  return g;
}
export function groundGeometry() {
  const n = GX.length, pos: number[] = [], uv: number[] = [], col: number[] = [], idx: number[] = [];
  for (let j = 0; j < GZ.length; j++) for (let i = 0; i < n; i++) {
    const x = GX[i], z = GZ[j];
    pos.push(x, gridHeights[j * n + i], z); uv.push(x / 5, z / 5);
    tint(x, z, col, col.length);
  }
  for (let j = 0; j < GZ.length - 1; j++) for (let i = 0; i < n - 1; i++) {
    const a = j * n + i, b = a + 1, c = a + n, d = c + 1;
    idx.push(a, c, b, b, c, d);
  }
  const g = attrs(new THREE.BufferGeometry(), pos, uv, col, idx);
  g.computeVertexNormals();
  return g;
}

// A ribbon along a path: rows along it, columns across at the given offsets (right of the
// direction of travel is positive). Triangles face up.
type Path = { x: number; z: number; tx: number; tz: number; s: number }[];
function ribbon(path: Path, cols: (p: Path[number]) => { o: number; y: number; u: number }[], ground: boolean, vScale: number) {
  const pos: number[] = [], uv: number[] = [], col: number[] | null = ground ? [] : null, idx: number[] = [];
  let w = 0;
  for (const p of path) {
    const cs = cols(p); w = cs.length;
    for (const c of cs) {
      const x = p.x - p.tz * c.o, z = p.z + p.tx * c.o; // right of travel is (-tz, tx)
      pos.push(x, c.y, z);
      if (col) { uv.push(x / 5, z / 5); tint(x, z, col, col.length); } else uv.push(c.u, p.s * vScale);
    }
  }
  for (let r = 0; r < path.length - 1; r++) for (let c = 0; c < w - 1; c++) {
    const a = r * w + c, b = a + w;
    idx.push(a, a + 1, b, a + 1, b + 1, b);
  }
  return attrs(new THREE.BufferGeometry(), pos, uv, col, idx);
}
function highwayPath(step = 8): Path {
  const p: Path = [];
  for (let z = HWY.z0; z <= HWY.z1; z += step) p.push({ x: 0, z, tx: 0, tz: 1, s: z });
  return p;
}
function trackPath(): Path {
  return track.pts.map((q, i) => {
    const a = track.pts[Math.max(0, i - 1)], b = track.pts[Math.min(track.pts.length - 1, i + 1)];
    const l = Math.hypot(b.x - a.x, b.y - a.y);
    return { x: q.x, z: q.y, tx: (b.x - a.x) / l, tz: (b.y - a.y) / l, s: track.s[i] };
  });
}
/** Asphalt with paved and gravel shoulders; u spans 12 m across, the texture repeats every 24 m. */
export function highwayGeometry() {
  return ribbon(highwayPath(), (p) => [-6, -4.8, -2.4, 0, 2.4, 4.8, 6].map((o) => ({ o, y: hLow(-p.tz * o, p.z) + hwyLift(Math.abs(o)), u: (o + 6) / 12 })), false, 1 / 24);
}
/** The gravel track, its soft edges blending into the desert; 12 m per texture repeat. */
export function trackGeometry() {
  return ribbon(trackPath(), (p) => {
    const w = trackHalf(p.s), e = w + TRK.soft;
    return [-e, -w, -w / 2, 0, w / 2, w, e].map((o) => ({ o, y: hLow(p.x - p.tz * o, p.z + p.tx * o) + trkLift(Math.abs(o), w), u: 0.5 + o / (2 * e) }));
  }, false, 1 / 12);
}
/** Verges beside both roads, in the desert's own material: flat, then dipping under the grid. */
export function vergeGeometry() {
  const side = (path: Path, inner: (p: Path[number]) => number, flat: number, edge: number) => [-1, 1].map((sg) => ribbon(path, (p) => {
    const os = [inner(p), (inner(p) + flat) / 2, flat, edge];
    return (sg < 0 ? os.map((o) => -o).reverse() : os).map((o) => ({ o, y: hLow(p.x - p.tz * o, p.z + p.tx * o) + dropY(Math.abs(o), flat, edge), u: 0 }));
  }, true, 0));
  return [...side(highwayPath(), () => HWY.shoulder, HWY.flat, HWY.margin), ...side(trackPath(), (p) => trackHalf(p.s) + TRK.soft, TRK.flat, TRK.margin)];
}
