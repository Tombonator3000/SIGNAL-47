import * as THREE from 'three';
import { STATION_TRACK, STATION_TRUCK, STATION_GROUND, STATION_PAD, STATION_FENCE, STATION_GATE, STATION_HUT, STATION_SHED, STATION_POLE } from './stationLayout';
import { stationMesas, roadMesas, STATION_FAR_LIGHTS } from './horizon';
import { fromRoad, stationToRoad, stationLand, outside, inRect, STATION_PATCH } from './geo';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood, floodSet, setFlood, type FloodSet } from './kit';
import { GlowPoints, glowScale } from './glow';
import { canvasTex, rng } from '../core/textures';
import { artTexture, artImage } from '../core/art';
import { eachVariant } from '../core/quality';
import { mark } from '../story/drawings';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// Chapter three location: STATION 01, the survey station from 1947, about 04:00.
// Built in its own coordinates (metres): x east, z south, north is -Z as at SARO. The
// player's feet are always at y = 0: the hut floor and its plinth are at 0, the desert
// floor lies at G = -0.15. The constructor puts the group at `origin`; everything handed
// out in world coordinates (zones, colliders, anchors, flood positions) includes it.
//
//   fence     x -30 to 30, z -24 to 26. Closed vehicle gate on the south side at
//             x -21.6 to -16.9, a walk-through beside it at x -23.0 to -21.6.
//   arrival   gravel pad outside the gate. The service truck parks facing the gate.
//   hut       inside x -13 to -9, z 1.5 to 4.5. Door in the south wall, x -10.45 to -9.45.
//   transit   concrete pier at (3, -4). The telescope is set on the empty footing of A.
//   A         footing 11 m north-east of the transit on the sight line; the post now
//             stands 3 m off the line, to the south-east of it.
//   B         comparison vane 12 m west-north-west of the transit, still in its footing.
//   C         old stakes east from the transit along z -4, out past the fence.
//   cable     from the junction box beside the door, in loose loops, to the pier. Cut
//             2.7 m south of the pier: two clean ends a hand's width apart.

// ---------- fake lights ----------
// Outside: one set for the whole station. Slots 0 and 1 are kept free for the parked
// truck's headlights (chapter code). The station uses 2 to 6, so 7 to 9 are free too.
export const fieldFlood = floodSet(10, 'field', 0.085);
// Inside the hut: its own small set, so the bulb does not shine through the walls and the
// work lights outside do not light the room. Slot 4 is free.
export const hutFlood = floodSet(5, 'hut', 0.2);

const G = STATION_GROUND;
// the station's night light, and the road's (RoadArea.ts) turned into the station's terms (world/geo.ts)
const NIGHT = { sky: 0x2c3b5e, ground: 0x1b140e, hemi: 1.2, moon: 0xa4b6e0, moonI: 1.5, moonAt: new THREE.Vector3(-50, 70, 55) };
const ROAD_NIGHT = { sky: 0x22304f, ground: 0x2a1b10, hemi: 0.9, moon: 0x8ea4d8, moonI: 0.45, moonAt: new THREE.Vector3(60, 80, -40) };
const _c = new THREE.Color();
const FENCE = STATION_FENCE;
const GATE = STATION_GATE;
const GAP_W = -23.0;
const TRUCK = STATION_TRUCK;
const ARRIVE = { x: -21.4, z: 30.2 };
const HUT = STATION_HUT;
const DOOR = { x0: -10.5, x1: -9.4, h: 2.0 };       // opening in the south wall; jambs inside it
const WIN_S = { x0: -12.3, x1: -11.3 };             // south window
const WIN_E = { z0: 2.3, z1: 3.3 };                 // east window, towards the transit
const SILL = 0.95, HEAD = 1.85;
const T = new THREE.Vector3(3, 0, -4);              // transit pier
const PIER = { bottom: G - 0.1, top: 1.05, half0: 0.45, half1: 0.26 };
const SIGHT = new THREE.Vector3(1, 0, -1).normalize();
const OFF = new THREE.Vector3(1, 0, 1).normalize();
const FOOT_A = T.clone().addScaledVector(SIGHT, 11);
const POST_A = T.clone().addScaledVector(SIGHT, 10).addScaledVector(OFF, 3);
const POST_B = new THREE.Vector3(-7.4, 0, -10.0);
const STAKES = Array.from({ length: 13 }, (_, i) => 8 + i * 4);   // x of the C stakes
const LAMP = new THREE.Vector3(1.6, 0, -2.6);
const CABLE_R = 0.025;
const CUT = new THREE.Vector3(3.2, G + CABLE_R, -1.3);
const CUT_DIR = new THREE.Vector3(0.05, 0, -1).normalize();      // along the cable, towards the pier
const POLE = new THREE.Vector3(STATION_POLE.x, 0, STATION_POLE.z);
const SHED = STATION_SHED;
const EXHAUST = new THREE.Vector3(-16.85, 2.95, -0.95);
const WALL_LAMP = new THREE.Vector3(-9.95, 2.42, 5.0);   // the bulb
const BASE = { wall: 6.0, field: 9.0, yard: 12.0, spill: 3.0, shed: 2.4, ceil: 5.0, desk: 2.6, dial: 1.1, door: 1.4 };
const HOOD_OPEN = 2.1;

// The worn footpath loop and the tyre ruts (painted into the ground map, kept clear of
// rocks and plants).
const PATHS: [number, number][][] = [
  [[-22.3, 28.5], [-22.3, 24.6], [-19.8, 19.5], [-15.8, 12.8], [-11.8, 7.8], [-9.95, 5.7]],
  [[-9.95, 5.7], [-6.8, 4.2], [-3.2, 2.2], [0.4, -0.6], [2.0, -2.0], [2.2, -3.4], [3.0, -5.0], [6.2, -7.0], [9.4, -9.0]],
  [[9.4, -9.0], [13.6, -6.6], [16.2, -4.8], [14.0, 0.5], [7.0, 7.5], [-2.5, 12.8], [-11.5, 16.8], [-17.6, 20.4]],
  [[0.4, -0.6], [-1.6, -4.4], [-4.8, -8.0], [-6.9, -9.4]],
];
const RUTS: [number, number][][] = [
  [[-19.3, 60], [-19.4, 48], [-19.25, 38], [-19.25, 30], [-19.25, 26.0], [-18.6, 21.0], [-16.6, 16.0], [-14.0, 12.0], [-11.8, 9.0]],
  [[-14.0, 12.0], [-11.4, 10.6], [-9.0, 10.0], [-7.6, 9.4]],
  [[-19.3, 37], [-21.2, 37.6], [-23.8, 36.8], [-25.2, 34.2], [-24.6, 31.0]],
];

const smooth = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
function segDist(px: number, pz: number, ax: number, az: number, bx: number, bz: number) {
  const dx = bx - ax, dz = bz - az, l2 = dx * dx + dz * dz;
  const t = l2 > 0 ? Math.min(1, Math.max(0, ((px - ax) * dx + (pz - az) * dz) / l2)) : 0;
  return Math.hypot(px - ax - dx * t, pz - az - dz * t);
}
function polyDist(px: number, pz: number, pts: [number, number][]) {
  let d = Infinity;
  for (let i = 0; i < pts.length - 1; i++) d = Math.min(d, segDist(px, pz, pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1]));
  return d;
}
const yawTo = (px: number, pz: number, tx: number, tz: number) => Math.atan2(-(tx - px), -(tz - pz));
const pitchTo = (px: number, pz: number, tx: number, ty: number, tz: number) => Math.atan2(ty - 1.62, Math.hypot(tx - px, tz - pz));

// ---------- terrain ----------
// Flat around the station and along the road, rolling further out. The grid is fine near
// the station and coarse towards the horizon. surface() returns the height of the drawn
// triangles, so rocks and plants sit on what is on screen.
class Terrain {
  xs = Terrain.axis();
  zs = Terrain.axis();
  h: Float32Array;
  constructor() {
    const nx = this.xs.length, nz = this.zs.length;
    this.h = new Float32Array(nx * nz);
    for (let j = 0; j < nz; j++) for (let i = 0; i < nx; i++) this.h[j * nx + i] = Terrain.height(this.xs[i], this.zs[j]);
  }
  private static axis() {
    const v = [0];
    let x = 0, step = 4;
    while (x < 2600) { if (x >= 60) step *= 1.2; x += step; v.push(Math.round(x * 100) / 100); }
    return [...v.slice(1).reverse().map((a) => -a), ...v];
  }
  static height(x: number, z: number) {
    const own = Terrain.own(x, z);
    // round the last of the survey track the ground is the road's land (world/geo.ts), and
    // under the road's own piece of it (drive/corridors.ts) it keeps out of the way; by the
    // gate that land is the station's flat ground, and the pad itself stays as it is
    const [rx, rz] = stationToRoad(x, z);
    const w = (1 - smooth(60, 360, outside(STATION_PATCH, rx, rz))) * smooth(0, 8, outside(STATION_PAD, x, z));
    if (w <= 0) return own;
    const land = stationLand(rx, rz) - (inRect(STATION_PATCH, rx, rz, -2) ? 0.7 : 0);
    return own + (land - own) * w;
  }
  private static own(x: number, z: number) {
    const k = smooth(70, 260, Math.hypot(x, z - 8)) * smooth(10, 40, polyDist(x, z, STATION_TRACK));
    if (k <= 0) return G;
    const n = Math.sin(x * 0.0043 + z * 0.0031) * 9 + Math.sin(x * 0.011 + 1.3) * Math.cos(z * 0.009 - 0.4) * 5
      + Math.sin(x * 0.027 - z * 0.019 + 2.0) * 2.2;
    return G + k * (n + 3);
  }
  private static find(a: number[], v: number) {
    let lo = 0, hi = a.length - 2;
    while (lo < hi) { const m = (lo + hi + 1) >> 1; if (a[m] <= v) lo = m; else hi = m - 1; }
    return lo;
  }
  surface(x: number, z: number) {
    const i = Terrain.find(this.xs, x), j = Terrain.find(this.zs, z), nx = this.xs.length;
    const fx = Math.min(1, Math.max(0, (x - this.xs[i]) / (this.xs[i + 1] - this.xs[i])));
    const fz = Math.min(1, Math.max(0, (z - this.zs[j]) / (this.zs[j + 1] - this.zs[j])));
    const ha = this.h[j * nx + i], hb = this.h[(j + 1) * nx + i], hc = this.h[(j + 1) * nx + i + 1], hd = this.h[j * nx + i + 1];
    // cell corners a (i, j), b (i, j+1), c (i+1, j+1), d (i+1, j); triangles abd and bcd
    if (fx + fz <= 1) return ha + (hd - ha) * fx + (hb - ha) * fz;
    return hc + (hb - hc) * (1 - fx) + (hd - hc) * (1 - fz);
  }
  geometry() {
    const nx = this.xs.length, nz = this.zs.length;
    const pos = new Float32Array(nx * nz * 3), uv = new Float32Array(nx * nz * 2), col = new Float32Array(nx * nz * 3);
    const r = rng(5);
    for (let j = 0; j < nz; j++) for (let i = 0; i < nx; i++) {
      const k = j * nx + i, x = this.xs[i], z = this.zs[j];
      pos.set([x, this.h[k], z], k * 3);
      uv.set([x, z], k * 2);
      const n = 0.84 + 0.1 * Math.sin(x * 0.07 + 0.5) * Math.cos(z * 0.05) + 0.07 * Math.sin(x * 0.013 - z * 0.017) + (r() - 0.5) * 0.05;
      col.set([n, n * 0.97, n * 0.93], k * 3);
    }
    const idx: number[] = [];
    for (let j = 0; j < nz - 1; j++) for (let i = 0; i < nx - 1; i++) {
      const a = j * nx + i, b = (j + 1) * nx + i, c = (j + 1) * nx + i + 1, d = j * nx + i + 1;
      idx.push(a, b, d, b, c, d);
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
    g.setAttribute('color', new THREE.BufferAttribute(col, 3));
    g.setIndex(idx);
    g.computeVertexNormals();
    return g;
  }
}

// ---------- helpers ----------
// Box-projected UVs in metres from the mesh's placement in its parent, so a wall built
// from several pieces runs one texture across the joints. Each texture sets its own tile
// size with `repeat`.
function metreUV<M extends THREE.Mesh>(m: M, topAlongX = false): M {
  m.updateMatrix();
  const g = m.geometry;
  const p = g.attributes.position as THREE.BufferAttribute, n = g.attributes.normal as THREE.BufferAttribute;
  const uv = g.attributes.uv as THREE.BufferAttribute;
  const nm = new THREE.Matrix3().getNormalMatrix(m.matrix);
  const v = new THREE.Vector3(), nn = new THREE.Vector3();
  for (let i = 0; i < p.count; i++) {
    v.fromBufferAttribute(p, i).applyMatrix4(m.matrix);
    nn.fromBufferAttribute(n, i).applyMatrix3(nm);
    const ax = Math.abs(nn.x), ay = Math.abs(nn.y), az = Math.abs(nn.z);
    if (ay > ax && ay > az) { if (topAlongX) uv.setXY(i, v.z, v.x); else uv.setXY(i, v.x, v.z); }
    else if (ax > az) uv.setXY(i, nn.x > 0 ? -v.z : v.z, v.y);
    else uv.setXY(i, nn.z > 0 ? v.x : -v.x, v.y);
  }
  uv.needsUpdate = true;
  return m;
}

// Flat facets: every triangle gets its own normal (works the same on Low quality, where
// materials lose flatShading).
function faceted(g: THREE.BufferGeometry) {
  const f = g.index ? g.toNonIndexed() : g;
  f.computeVertexNormals();
  return f;
}

// Moves the vertices of a polyhedron in or out, the same amount for every copy of a corner.
function jitter(g: THREE.BufferGeometry, seed: number, lo: number, hi: number) {
  const p = g.attributes.position as THREE.BufferAttribute, r = rng(seed), seen = new Map<string, number>();
  for (let i = 0; i < p.count; i++) {
    const key = `${p.getX(i).toFixed(3)},${p.getY(i).toFixed(3)},${p.getZ(i).toFixed(3)}`;
    if (!seen.has(key)) seen.set(key, lo + r() * (hi - lo));
    const k = seen.get(key)!;
    p.setXYZ(i, p.getX(i) * k, p.getY(i) * k, p.getZ(i) * k);
  }
  return g;
}

// The painted ground map (paths, ruts, soft shade under things) is multiplied into the
// colour of the ground and the gravel. Grey 128 leaves the colour as it is.
const OV = { x0: -64, z0: -54, size: 128, px: 1024 };
function groundMap<T extends THREE.MeshStandardMaterial>(m: T, tex: THREE.Texture): T {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.uniforms.uGround = { value: tex };
    sh.uniforms.uGroundRect = { value: new THREE.Vector3(OV.x0, OV.z0, 1 / OV.size) };
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec2 vGroundUv;\nuniform vec3 uGroundRect;')
      .replace('#include <begin_vertex>', '#include <begin_vertex>\n  vGroundUv = (transformed.xz - uGroundRect.xy) * uGroundRect.z;');
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', '#include <common>\nvarying vec2 vGroundUv;\nuniform sampler2D uGround;')
      .replace('#include <map_fragment>', '#include <map_fragment>\n  diffuseColor.rgb *= texture2D(uGround, vGroundUv).rgb * 2.0;');
  };
  m.customProgramCacheKey = () => key.call(m) + 'ground';
  return m;
}

// The blank enamel sign (lab/sign_blank.png) at any size, corners kept (as in textures.ts).
function enamel(g: CanvasRenderingContext2D, w: number, h: number) {
  const im = artImage('sign'), inset = 40;
  const d = inset * Math.min(w / im.width, h / im.height);
  const sx = [0, inset, im.width - inset, im.width], sy = [0, inset, im.height - inset, im.height];
  const dx = [0, d, w - d, w], dy = [0, d, h - d, h];
  for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++)
    g.drawImage(im, sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j], dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]);
}
function paperFill(g: CanvasRenderingContext2D, w: number, h: number) {
  const im = artImage('paper'), k = Math.max(w / im.width, h / im.height);
  g.drawImage(im, (w - im.width * k) / 2, (h - im.height * k) / 2, im.width * k, im.height * k);
}
function blot(g: CanvasRenderingContext2D, x: number, y: number, r: number, rgba: string) {
  const gr = g.createRadialGradient(x, y, 0, x, y, r);
  gr.addColorStop(0, rgba); gr.addColorStop(1, rgba.replace(/[\d.]+\)$/, '0)'));
  g.fillStyle = gr; g.fillRect(x - r, y - r, r * 2, r * 2);
}
function crack(g: CanvasRenderingContext2D, r: () => number, x: number, y: number, len: number, rgba: string, wdt = 1) {
  g.strokeStyle = rgba; g.lineWidth = wdt; g.beginPath(); g.moveTo(x, y);
  let a = r() * Math.PI * 2;
  for (let i = 0; i < len; i++) { a += (r() - 0.5) * 1.1; x += Math.cos(a) * 4; y += Math.sin(a) * 4; g.lineTo(x, y); }
  g.stroke();
}

// ---------- canvas surfaces ----------
// Lime-washed plaster for the inside of the hut: soft, uneven, a few hairline cracks.
function plasterTex() {
  return canvasTex(512, 512, (g, w, h) => {
    g.fillStyle = '#dcd5c4'; g.fillRect(0, 0, w, h);
    const r = rng(37);
    for (let i = 0; i < 60; i++) blot(g, r() * w, r() * h, 30 + r() * 90, r() < 0.5 ? 'rgba(255,252,240,.18)' : 'rgba(150,132,104,.12)');
    for (let i = 0; i < 90; i++) {
      const x = r() * w, y = r() * h, l = 20 + r() * 60;
      g.strokeStyle = `rgba(${r() < 0.5 ? '255,250,238' : '120,104,80'},${0.05 + r() * 0.06})`; g.lineWidth = 3 + r() * 6;
      g.beginPath(); g.moveTo(x, y); g.quadraticCurveTo(x + (r() - 0.5) * 30, y + l / 2, x + (r() - 0.5) * 20, y + l); g.stroke();
    }
    for (let i = 0; i < 2600; i++) { g.fillStyle = `rgba(90,76,56,${r() * 0.12})`; g.fillRect(r() * w, r() * h, 1 + r(), 1 + r()); }
    for (let i = 0; i < 6; i++) crack(g, r, r() * w, r() * h, 6 + r() * 14, 'rgba(90,74,56,.35)');
  }, [1 / 2.4, 1 / 2.4]);
}

// Crushed gravel over sand: the desert picture under a few thousand stones (drawn wrapped
// so the tile has no seams).
function gravelTex() {
  return canvasTex(512, 512, (g, w, h) => {
    g.drawImage(artImage('desert'), 0, 0, w, h);
    g.fillStyle = 'rgba(126,120,110,.42)'; g.fillRect(0, 0, w, h);
    const r = rng(77);
    for (let i = 0; i < 2800; i++) {
      const x = r() * w, y = r() * h, s = 1.6 + r() * r() * 7.5, e = 0.55 + r() * 0.45, a = r() * Math.PI;
      const v = 86 + r() * 120, warm = r() * 30;
      for (const ox of [-w, 0, w]) for (const oy of [-h, 0, h]) {
        const px = x + ox, py = y + oy;
        if (px < -12 || px > w + 12 || py < -12 || py > h + 12) continue;
        g.fillStyle = 'rgba(18,14,10,.4)'; g.beginPath(); g.ellipse(px + 1.2, py + 1.4, s, s * e, a, 0, 7); g.fill();
        g.fillStyle = `rgb(${v + warm | 0},${v + warm * 0.55 | 0},${v | 0})`; g.beginPath(); g.ellipse(px, py, s, s * e, a, 0, 7); g.fill();
        g.fillStyle = 'rgba(255,250,240,.18)'; g.beginPath(); g.ellipse(px - s * 0.3, py - s * 0.3, s * 0.4, s * e * 0.35, a, 0, 7); g.fill();
      }
    }
  }, [1 / 1.6, 1 / 1.6]);
}

// Weathered wood with the grain along the length (posts, stakes, frames, furniture).
function woodTex(seed: number, base: string, tile: [number, number]) {
  return canvasTex(256, 512, (g, w, h) => {
    g.fillStyle = base; g.fillRect(0, 0, w, h);
    const r = rng(seed);
    for (let i = 0; i < 120; i++) {
      const x = r() * w, y = r() * h, len = 80 + r() * 380, light = r() < 0.45;
      g.strokeStyle = light ? `rgba(255,246,228,${0.03 + r() * 0.06})` : `rgba(20,14,8,${0.05 + r() * 0.1})`;
      g.lineWidth = 0.6 + r() * 2;
      for (const oy of [0, -h]) {
        g.beginPath(); g.moveTo(x, y + oy);
        g.bezierCurveTo(x + (r() - 0.5) * 8, y + oy + len * 0.33, x + (r() - 0.5) * 8, y + oy + len * 0.66, x + (r() - 0.5) * 5, y + oy + len);
        g.stroke();
      }
    }
    for (let i = 0; i < 7; i++) {
      const x = r() * w, y = r() * h;
      g.fillStyle = 'rgba(30,20,12,.5)'; g.beginPath(); g.ellipse(x, y, 3 + r() * 4, 6 + r() * 8, 0, 0, 7); g.fill();
    }
    for (let i = 0; i < 10; i++) { const x = r() * w; g.fillStyle = 'rgba(12,8,4,.55)'; g.fillRect(x, r() * h, 1.2, 30 + r() * 120); }
  }, tile);
}

// Galvanised corrugated sheet with rust runs, for the generator shed. One tile is the
// wall height, so the rust gathers at the bottom.
function corrugatedTex() {
  return canvasTex(256, 256, (g, w, h) => {
    for (let x = 0; x < w; x++) {
      const v = 128 + Math.sin(x / w * Math.PI * 2 * 12) * 40;
      g.fillStyle = `rgb(${v | 0},${v + 3 | 0},${v + 6 | 0})`; g.fillRect(x, 0, 1, h);
    }
    const r = rng(53);
    for (let i = 0; i < 22; i++) {
      const x = r() * w, y = r() * h * 0.7, l = 30 + r() * 150;
      const s = g.createLinearGradient(0, y, 0, y + l);
      s.addColorStop(0, 'rgba(126,62,24,.5)'); s.addColorStop(1, 'rgba(126,62,24,0)');
      g.fillStyle = s; g.fillRect(x, y, 2 + r() * 7, l);
    }
    const foot = g.createLinearGradient(0, h, 0, h - 60);
    foot.addColorStop(0, 'rgba(110,52,20,.75)'); foot.addColorStop(1, 'rgba(110,52,20,0)');
    g.fillStyle = foot; g.fillRect(0, h - 60, w, 60);
  }, [1 / 0.9, 1 / 2.05]);
}

// The end of a cut multi-core cable: black jacket, jute filler, four insulated conductors
// and a small centre core, the copper bright where the cutter went through.
function cutFaceTex() {
  return canvasTex(128, 128, (g, w) => {
    const c = w / 2;
    const disc = (x: number, y: number, rr: number, f: string | CanvasGradient) => { g.fillStyle = f; g.beginPath(); g.arc(x, y, rr, 0, 7); g.fill(); };
    g.fillStyle = '#0c0c0c'; g.fillRect(0, 0, w, w);
    disc(c, c, 63, '#1c1b1a');
    disc(c, c, 52, '#5a4e3e');
    const ins = ['#8e2b1f', '#d9d0bd', '#2f5b3a', '#26231f'];
    for (let k = 0; k < 4; k++) {
      const a = k * Math.PI / 2 + Math.PI / 4, x = c + Math.cos(a) * 24, y = c + Math.sin(a) * 24;
      disc(x, y, 19, ins[k]);
      const cu = g.createRadialGradient(x - 3, y - 3, 1, x, y, 11);
      cu.addColorStop(0, '#ffd8a8'); cu.addColorStop(0.55, '#e0904e'); cu.addColorStop(1, '#9a5226');
      disc(x, y, 11, cu);
    }
    disc(c, c, 8, '#2a2622');
    disc(c, c, 4, '#d18a52');
  });
}

// ---------- the sign atlas ----------
// Every painted sign and plate outside shares one texture, so they cost one draw call.
const ATLAS = 1024;
const REG = {
  station: [0, 0, 512, 160], gate: [512, 0, 512, 256], markA: [0, 168, 256, 256], disc: [264, 168, 240, 240],
  vane: [520, 264, 120, 480], jbox: [648, 264, 192, 120], gen: [848, 264, 176, 96], tag: [848, 368, 176, 88],
  e09a: [0, 432, 384, 512],
} as const;
type Region = keyof typeof REG;
function regionUV(g: THREE.BufferGeometry, id: Region) {
  const [x, y, w, h] = REG[id];
  const uv = g.attributes.uv as THREE.BufferAttribute;
  const u0 = x / ATLAS, u1 = (x + w) / ATLAS, v0 = 1 - (y + h) / ATLAS, v1 = 1 - y / ATLAS;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, u0 + uv.getX(i) * (u1 - u0), v0 + uv.getY(i) * (v1 - v0));
  return g;
}
function signAtlas() {
  const INK = '#1f2326';
  const draw: Record<Region, (g: CanvasRenderingContext2D, w: number, h: number) => void> = {
    station: (g, w, h) => {
      enamel(g, w, h);
      g.fillStyle = INK; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '500 92px Oswald'; g.fillText('STATION 01', w / 2, h / 2 + 4, w * 0.86);
    },
    gate: (g, w, h) => {
      enamel(g, w, h);
      g.fillStyle = INK; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '600 50px Oswald'; g.fillText('SARO', w / 2, 52, w * 0.8);
      g.font = '500 34px Oswald'; g.fillText('SURVEY STATION 01', w / 2, 104, w * 0.84);
      g.fillStyle = '#8c2418'; g.font = '500 28px Oswald'; g.fillText('AUTHORIZED PERSONNEL ONLY', w / 2, 152, w * 0.84);
      g.fillStyle = INK; g.font = '22px Oswald'; g.fillText('KEEP GATE CLOSED', w / 2, 196, w * 0.8);
    },
    markA: (g, w, h) => {
      enamel(g, w, h);
      g.strokeStyle = '#141414'; g.lineWidth = 15; g.lineJoin = 'miter'; g.lineCap = 'butt';
      mark(g, w / 2, h / 2 - 8, 150);
      const r = rng(61);
      for (let i = 0; i < 26; i++) { g.fillStyle = `rgba(120,64,28,${0.2 + r() * 0.4})`; g.fillRect(r() < 0.5 ? r() * 18 : w - r() * 18, r() * h, 2 + r() * 5, 2 + r() * 9); }
    },
    disc: (g, w, h) => {
      g.fillStyle = '#2c241a'; g.fillRect(0, 0, w, h);
      const c = w / 2, rr = w / 2 - 4;
      const br = g.createRadialGradient(c - 30, c - 30, 10, c, c, rr);
      br.addColorStop(0, '#c9a35e'); br.addColorStop(0.7, '#8e6d36'); br.addColorStop(1, '#5d4522');
      g.fillStyle = br; g.beginPath(); g.arc(c, c, rr, 0, 7); g.fill();
      g.strokeStyle = 'rgba(50,34,14,.8)'; g.lineWidth = 3; g.beginPath(); g.arc(c, c, rr - 26, 0, 7); g.stroke();
      g.fillStyle = 'rgba(46,30,12,.9)'; g.font = '600 19px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
      const text = 'STATION 01  *  FIXED POINT  *  1947  *  ';
      for (let i = 0; i < text.length; i++) {
        const a = i / text.length * Math.PI * 2 - Math.PI / 2;
        g.save(); g.translate(c + Math.cos(a) * (rr - 13), c + Math.sin(a) * (rr - 13)); g.rotate(a + Math.PI / 2); g.fillText(text[i], 0, 0); g.restore();
      }
      g.strokeStyle = 'rgba(46,30,12,.95)'; g.lineWidth = 7; mark(g, c, c - 4, 92);
      g.fillStyle = 'rgba(30,20,8,.9)'; g.beginPath(); g.arc(c, c - 2, 4, 0, 7); g.fill();
      const r = rng(67);
      for (let i = 0; i < 30; i++) blot(g, c + (r() - 0.5) * rr * 1.6, c + (r() - 0.5) * rr * 1.6, 8 + r() * 20, 'rgba(40,70,52,.18)');
    },
    vane: (g, w, h) => {
      enamel(g, w, h);
      g.fillStyle = '#121212'; g.fillRect(w / 2 - 17, 22, 34, h - 44);
      g.fillStyle = '#3a3a36'; for (const y of [12, h - 12]) { g.beginPath(); g.arc(w / 2, y, 4, 0, 7); g.fill(); }
    },
    jbox: (g, w, h) => {
      g.drawImage(artImage('cabinet'), 0, 0, w, h);
      g.fillStyle = '#ece5cf'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '500 36px Oswald'; g.fillText('FIELD LINE', w / 2, h * 0.36, w * 0.86);
      g.font = '26px Oswald'; g.fillText('J-1   STATION 01', w / 2, h * 0.72, w * 0.86);
    },
    gen: (g, w, h) => {
      g.fillStyle = '#e7dcc0'; g.fillRect(0, 0, w, h);
      g.strokeStyle = '#8c2418'; g.lineWidth = 5; g.strokeRect(5, 5, w - 10, h - 10);
      g.fillStyle = '#8c2418'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '600 34px Oswald'; g.fillText('GENERATOR', w / 2, h * 0.38, w * 0.84);
      g.font = '22px Oswald'; g.fillText('NO SMOKING', w / 2, h * 0.74, w * 0.84);
    },
    tag: (g, w, h) => {
      g.fillStyle = '#d6a51c'; g.fillRect(0, 0, w, h);
      g.fillStyle = '#16120a'; g.textAlign = 'center'; g.textBaseline = 'middle';
      g.font = '600 34px Oswald'; g.fillText('DANGER', w / 2, h * 0.36, w * 0.86);
      g.font = '22px Oswald'; g.fillText('HIGH VOLTAGE', w / 2, h * 0.74, w * 0.86);
    },
    e09a: (g, w, h) => {
      paperFill(g, w, h);
      g.fillStyle = 'rgba(120,96,50,.18)'; g.fillRect(0, 0, w, h);
      g.fillStyle = '#26262a'; g.textAlign = 'left'; g.textBaseline = 'alphabetic';
      g.font = '600 26px Oswald'; g.fillText('FIELD TRANSIT RECORD', 26, 46, w - 52);
      g.font = '20px "Special Elite"'; g.fillText('STATION 01  /  E09A  /  1947', 26, 76, w - 52);
      g.fillRect(26, 88, w - 52, 2);
      g.strokeStyle = '#22372e'; g.lineWidth = 3; g.lineCap = 'round';
      mark(g, 70, 150, 46);
      g.beginPath(); g.moveTo(102, 150); g.lineTo(200, 150); g.stroke();
      g.beginPath(); g.moveTo(212, 124); g.lineTo(212, 176); g.stroke();
      g.beginPath(); g.moveTo(218, 150); g.lineTo(300, 150); g.stroke();
      g.beginPath(); g.arc(320, 150, 16, 0, 7); g.stroke();
      g.font = '20px "Special Elite"'; g.fillStyle = '#22372e';
      g.fillText('A', 62, 206); g.fillText('B', 206, 206); g.fillText('C', 313, 206);
      g.fillStyle = '#26262a'; g.font = '17px "Special Elite"';
      ['A  FIXED POINT. FOOTING ON', '   THE TRANSIT SIGHT LINE.', 'B  COMPARISON VANE, WEST.', 'C  CLOSING SIGHT LINE, EAST.', '', 'SET ON A BEFORE EACH RUN.', 'DO NOT DISTURB THE FOOTINGS.']
        .forEach((l, i) => g.fillText(l, 26, 252 + i * 26, w - 52));
      g.fillStyle = '#26324f'; g.font = '34px "Reenie Beanie"'; g.fillText('N. Vega', 238, 476);
      const r = rng(71);
      for (let i = 0; i < 18; i++) blot(g, r() * w, r() * h, 10 + r() * 40, 'rgba(110,80,40,.12)');
    },
  };
  return canvasTex(ATLAS, ATLAS, (g) => {
    g.fillStyle = '#2b2b2b'; g.fillRect(0, 0, ATLAS, ATLAS);
    for (const id of Object.keys(REG) as Region[]) {
      const [x, y, w, h] = REG[id];
      g.save(); g.translate(x, y); g.beginPath(); g.rect(0, 0, w, h); g.clip();
      draw[id](g, w, h);
      g.restore();
    }
  });
}

// The 1947 arrangement drawing (the full original, A, B and C), as it hangs framed in the
// hut. Same layout as arrangement(true) in story/drawings.ts, drawn straight onto a canvas.
function arrangementTex() {
  return canvasTex(960, 420, (g, w, h) => {
    g.fillStyle = '#e9dfc4'; g.fillRect(0, 0, w, h);
    const r = rng(960 * 31 + 420);
    for (let i = 0; i < 1400; i++) { g.fillStyle = `rgba(90,70,40,${r() * 0.07})`; g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2); }
    for (let i = 0; i < 9; i++) blot(g, r() * w, r() * h, 30 + r() * 90, 'rgba(150,110,50,.12)');
    const INK = '#22372e';
    g.strokeStyle = INK; g.fillStyle = INK; g.lineWidth = 3; g.lineCap = 'round'; g.lineJoin = 'round';
    g.font = '600 26px Oswald, sans-serif';
    g.fillText('ARRANGEMENT / STATION 01 / 1947', 40, 56);
    g.font = '19px "Special Elite", serif';
    g.fillText('A: fixed survey point     B: comparison vane     C: closing sight line', 40, 92);
    g.globalAlpha = 0.5; g.lineWidth = 1.5;
    g.beginPath(); g.moveTo(40, 112); g.lineTo(920, 112); g.stroke();
    g.globalAlpha = 1; g.lineWidth = 3;
    mark(g, 150, 250, 64);
    g.beginPath(); g.moveTo(196, 250); g.lineTo(470, 250); g.stroke();
    g.beginPath(); g.moveTo(500, 218); g.lineTo(500, 282); g.stroke();
    g.beginPath(); g.moveTo(506, 250); g.lineTo(790, 250); g.stroke();
    g.beginPath(); g.arc(820, 250, 28, 0, Math.PI * 2); g.stroke();
    g.font = '28px "Special Elite", serif';
    g.fillText('A', 140, 340); g.fillText('B', 490, 340); g.fillText('C', 810, 340);
    g.font = '17px "Special Elite", serif';
    g.fillText('N. VEGA', 790, 396);
  });
}

// The timing log open on the bench: two ruled pages, typed headings, entries in pencil
// and ink. The entries follow the field timing record (E10) from the Unity version.
function ledgerTex() {
  return canvasTex(1024, 720, (g, w, h) => {
    paperFill(g, w, h);
    g.fillStyle = 'rgba(150,120,70,.16)'; g.fillRect(0, 0, w, h);
    const gut = g.createLinearGradient(w / 2 - 60, 0, w / 2 + 60, 0);
    gut.addColorStop(0, 'rgba(60,40,20,0)'); gut.addColorStop(0.5, 'rgba(60,40,20,.45)'); gut.addColorStop(1, 'rgba(60,40,20,0)');
    g.fillStyle = gut; g.fillRect(w / 2 - 60, 0, 120, h);
    for (const x0 of [0, w / 2]) {
      g.strokeStyle = 'rgba(70,100,150,.28)'; g.lineWidth = 1.5;
      for (let y = 120; y < h - 30; y += 38) { g.beginPath(); g.moveTo(x0 + 30, y); g.lineTo(x0 + w / 2 - 30, y); g.stroke(); }
      g.strokeStyle = 'rgba(170,50,40,.35)';
      g.beginPath(); g.moveTo(x0 + 140, 70); g.lineTo(x0 + 140, h - 30); g.stroke();
    }
    g.fillStyle = '#2a2a2e'; g.font = '600 30px Oswald';
    g.fillText('STATION 01   TIMING LOG', 44, 62, w / 2 - 80);
    g.fillText('1947', w / 2 + 44, 62);
    g.font = '19px "Special Elite"';
    g.fillText('TIME', 44, 104); g.fillText('OBSERVATION', 158, 104);
    g.fillText('TIME', w / 2 + 44, 104); g.fillText('OBSERVATION', w / 2 + 158, 104);
    const ink = (lines: [string, string][], x0: number, col: string) => {
      g.fillStyle = col; g.font = '38px "Reenie Beanie"';
      lines.forEach(([t, o], i) => { g.fillText(t, x0 + 36, 152 + i * 38, 100); g.fillText(o, x0 + 152, 152 + i * 38, w / 2 - 190); });
    };
    ink([['01:40', 'transit set on A. N.V.'], ['01:52', 'B vane checked. T.V.'], ['02:05', 'carrier on, steady'], ['02:11', 'lamp on, reference lit'], ['02:14', 'reference holding'], ['02:16', 'T. at the reference']], 0, '#2b3348');
    ink([['02:17:00', 'carrier ceased'], ['', 'voice on the line:'], ['', '"Reference west. No. East.'], ['', 'Hold the last reading."'], ['', '(T. Vega)'], ['02:17:47', 'relay impact, reference motion'], ['', 'final observation:']], w / 2, '#1d2233');
    g.strokeStyle = 'rgba(30,34,51,.6)'; g.lineWidth = 2;
    g.beginPath(); g.moveTo(w / 2 + 160, 152 + 7 * 38 + 4); g.lineTo(w / 2 + 420, 152 + 7 * 38 + 4); g.stroke();
    const r = rng(83);
    for (let i = 0; i < 14; i++) blot(g, r() * w, r() * h, 16 + r() * 50, 'rgba(120,90,40,.1)');
  });
}

// The receiver's slide-rule dial (top) and its signal meter (bottom left), lit from behind.
function dialTex() {
  return canvasTex(512, 256, (g, w) => {
    const bg = g.createLinearGradient(0, 0, 0, 128);
    bg.addColorStop(0, '#5b3a10'); bg.addColorStop(0.5, '#a8701e'); bg.addColorStop(1, '#5b3a10');
    g.fillStyle = bg; g.fillRect(0, 0, w, 128);
    g.strokeStyle = '#2a1806'; g.fillStyle = '#2a1806'; g.lineWidth = 2;
    g.font = '600 15px Oswald'; g.textAlign = 'center';
    const band = (y: number, label: string, marks: string[]) => {
      g.beginPath(); g.moveTo(40, y); g.lineTo(w - 20, y); g.stroke();
      marks.forEach((m, i) => {
        const x = 60 + i * (w - 100) / (marks.length - 1);
        g.beginPath(); g.moveTo(x, y - 7); g.lineTo(x, y + 2); g.stroke();
        g.fillText(m, x, y - 11);
      });
      for (let i = 0; i < 40; i++) { const x = 60 + i * (w - 100) / 39; g.beginPath(); g.moveTo(x, y); g.lineTo(x, y - 3); g.stroke(); }
      g.textAlign = 'left'; g.fillText(label, 8, y + 4); g.textAlign = 'center';
    };
    band(40, 'KC', ['550', '700', '900', '1100', '1300', '1600']);
    band(82, 'MC', ['2', '3', '4', '6', '8', '12', '18']);
    g.font = '500 13px Oswald'; g.fillText('FIELD RECEIVER   BAND B', w / 2, 116);
    g.fillStyle = '#c8281c'; g.fillRect(318, 10, 3, 104);
    // signal meter
    g.fillStyle = '#e8d6a8'; g.fillRect(0, 128, 128, 128);
    g.strokeStyle = '#2a1806'; g.lineWidth = 2;
    g.beginPath(); g.arc(64, 236, 84, Math.PI * 1.22, Math.PI * 1.78); g.stroke();
    for (let i = 0; i <= 10; i++) { const a = Math.PI * (1.22 + i * 0.056); g.beginPath(); g.moveTo(64 + Math.cos(a) * 84, 236 + Math.sin(a) * 84); g.lineTo(64 + Math.cos(a) * 74, 236 + Math.sin(a) * 74); g.stroke(); }
    g.fillStyle = '#2a1806'; g.font = '600 14px Oswald'; g.fillText('SIGNAL', 64, 214);
    g.strokeStyle = '#1a1006'; g.lineWidth = 2.5;
    g.beginPath(); g.moveTo(64, 236); g.lineTo(64 + Math.cos(Math.PI * 1.62) * 80, 236 + Math.sin(Math.PI * 1.62) * 80); g.stroke();
  });
}

// ---------- the painted ground map ----------
type Shade = { x: number; z: number; r: number; k: number };
function groundMapTex(shades: Shade[]) {
  const ppm = OV.px / OV.size;
  const P = (x: number, z: number) => [(x - OV.x0) * ppm, (z - OV.z0) * ppm] as const;
  const t = canvasTex(OV.px, OV.px, (g, w, h) => {
    g.fillStyle = 'rgb(128,128,128)'; g.fillRect(0, 0, w, h);
    const r = rng(101);
    // soft light and dark drifts break up the repeat of the sand picture
    for (let i = 0; i < 320; i++) blot(g, r() * w, r() * h, 12 + r() * 64, r() < 0.5 ? `rgba(156,154,150,${0.2 + r() * 0.3})` : `rgba(104,102,100,${0.2 + r() * 0.3})`);
    // lines go through a centripetal spline, so corners become curves
    const curve = (pts: [number, number][]) => {
      if (pts.length < 3) return pts;
      const c = new THREE.CatmullRomCurve3(pts.map(([x, z]) => new THREE.Vector3(x, 0, z)), false, 'centripetal');
      return c.getSpacedPoints(Math.ceil(c.getLength() / 0.5)).map((v) => [v.x, v.z] as [number, number]);
    };
    const line = (pts: [number, number][], wd: number, style: string) => {
      g.strokeStyle = style; g.lineWidth = wd * ppm; g.lineCap = 'round'; g.lineJoin = 'round';
      g.beginPath(); curve(pts).forEach(([x, z], i) => { const [px, py] = P(x, z); if (i) g.lineTo(px, py); else g.moveTo(px, py); }); g.stroke();
    };
    // footpaths: trodden lighter, a soft edge first
    for (const p of PATHS) { line(p, 1.9, 'rgba(160,156,150,.28)'); line(p, 0.95, 'rgba(172,168,160,.5)'); }
    // tyre ruts: two tracks 1.65 m apart, offset sideways from the centre line
    for (const raw of RUTS) {
      const p = curve(raw);
      for (const s of [-0.82, 0.82]) {
        const off: [number, number][] = p.map(([x, z], i) => {
          const a = p[Math.max(0, i - 1)], b = p[Math.min(p.length - 1, i + 1)];
          const dx = b[0] - a[0], dz = b[1] - a[1], l = Math.hypot(dx, dz) || 1;
          return [x - dz / l * s, z + dx / l * s];
        });
        line(off, 0.6, 'rgba(104,102,100,.22)');
        line(off, 0.28, 'rgba(82,80,78,.38)');
      }
    }
    // trodden ground at the transit, the lamp and the door
    for (const [x, z, rr] of [[T.x - 0.4, T.z + 1.2, 2.6], [-9.95, 5.9, 1.4], [GATE.w - 0.7, FENCE.z1, 1.6]]) {
      const [px, py] = P(x, z); blot(g, px, py, rr * ppm, 'rgba(170,166,158,.45)');
    }
    // dug and refilled where the A post stands now
    { const [px, py] = P(POST_A.x, POST_A.z); blot(g, px, py, 0.9 * ppm, 'rgba(84,80,74,.55)'); }
    // soft shade under things
    for (const s of shades) { const [px, py] = P(s.x, s.z); blot(g, px, py, Math.max(1.5, s.r * ppm), `rgba(40,38,36,${s.k})`); }
    // fade to neutral towards the edge
    const band = 48;
    for (const [x0, y0, x1, y1, rx, ry, rw, rh] of [[0, 0, 0, band, 0, 0, w, band], [0, h, 0, h - band, 0, h - band, w, band], [0, 0, band, 0, 0, 0, band, h], [w, 0, w - band, 0, w - band, 0, band, h]]) {
      const gr = g.createLinearGradient(x0, y0, x1, y1);
      gr.addColorStop(0, 'rgb(128,128,128)'); gr.addColorStop(1, 'rgba(128,128,128,0)');
      g.fillStyle = gr; g.fillRect(rx, ry, rw, rh);
    }
  });
  t.flipY = false;
  t.wrapS = t.wrapT = THREE.ClampToEdgeWrapping;
  return t;
}

// ---------- plant and rock shapes ----------
function rockGeo(seed: number, kind: 'block' | 'slab') {
  const g = kind === 'block' ? new THREE.IcosahedronGeometry(1, 0) : new THREE.DodecahedronGeometry(1, 0);
  jitter(g, seed, 0.7, 1.25);
  if (kind === 'block') g.scale(1, 0.72, 0.86); else g.scale(1.3, 0.46, 0.85);
  const p = g.attributes.position as THREE.BufferAttribute;
  const floor = kind === 'block' ? -0.32 : -0.18;
  for (let i = 0; i < p.count; i++) if (p.getY(i) < floor) p.setY(i, floor);
  return faceted(g);
}
function creosoteGeo() {
  const parts: THREE.BufferGeometry[] = [];
  const r = rng(31);
  const clumps: THREE.Vector3[] = [];
  for (let i = 0; i < 7; i++) {
    const g = jitter(new THREE.IcosahedronGeometry(0.22, 0), 40 + i, 0.7, 1.3);
    const a = i / 7 * Math.PI * 2 + r() * 0.6, d = i === 0 ? 0.05 : 0.18 + r() * 0.26, y = 0.42 + r() * 0.62;
    g.scale(0.9 + r() * 0.5, 0.75 + r() * 0.45, 0.9 + r() * 0.5);
    g.translate(Math.cos(a) * d, y, Math.sin(a) * d);
    clumps.push(new THREE.Vector3(Math.cos(a) * d, y, Math.sin(a) * d));
    parts.push(g);
  }
  // thin stems fanning out from the root to the clumps
  for (const c of clumps.slice(0, 5)) {
    const len = c.length();
    const st = new THREE.CylinderGeometry(0.008, 0.016, len, 3).toNonIndexed();
    st.translate(0, len / 2, 0);
    st.applyQuaternion(new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 1, 0), c.clone().normalize()));
    parts.push(st);
  }
  return faceted(mergeGeometries(parts.map((p) => { p.deleteAttribute('uv'); return p; }), false)!);
}
function bladeGeo(seed: number, n: number, lenLo: number, lenHi: number, wdt: number, up: [number, number]) {
  const pos: number[] = [], r = rng(seed);
  for (let i = 0; i < n; i++) {
    const a = r() * Math.PI * 2, el = up[0] + r() * (up[1] - up[0]), len = lenLo + r() * (lenHi - lenLo);
    const bx = (r() - 0.5) * 0.06, bz = (r() - 0.5) * 0.06;
    const d = new THREE.Vector3(Math.cos(a) * Math.cos(el), Math.sin(el), Math.sin(a) * Math.cos(el));
    const s = new THREE.Vector3(-Math.sin(a), 0, Math.cos(a)).multiplyScalar(wdt);
    pos.push(bx - s.x, 0.02, bz - s.z, bx + s.x, 0.02, bz + s.z, bx + d.x * len, d.y * len, bz + d.z * len);
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.computeVertexNormals();
  return g;
}

type Thing = { kind: 'block' | 'slab' | 'creo' | 'yucca' | 'grass'; x: number; z: number; s: number; ry: number; tilt: number; tone: number };
type Spot = { x: number; z: number; yaw: number; pitch: number };
export interface StationAnchors {
  arrive: { x: number; z: number; yaw: number };
  truck: { x: number; z: number; heading: number };
  markerTarget: THREE.Vector3;
  cableTarget: THREE.Vector3;
  photoSpots: { marker: Spot; cable: Spot };
  points: Record<string, THREE.Vector3>;
}

export class Station01 {
  group = new THREE.Group();
  // The hut's inside. The chapter may hide it while the player is far from the hut.
  interior = new THREE.Group();
  colliders: Collider[] = [];
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  proxies: Record<string, THREE.Object3D> = {};
  objs: Record<string, THREE.Object3D> = {};
  // Invisible boxes for FieldCamera.clearLine(): the hut, the shed, the pier.
  occluders: THREE.Object3D[] = [];
  hutBounds: Collider;
  // The parked truck's footprint, for the chapter to add when the truck is there.
  truckCollider: Collider;
  anchors: StationAnchors;
  lights: { group: THREE.Group; hemi: THREE.HemisphereLight; moon: THREE.DirectionalLight };
  glow = new GlowPoints();
  lampCovered = false;
  receiverOn = true;
  hutLightOn = true;

  private o: THREE.Vector3;
  private terrain = new Terrain();
  private st = new THREE.Group();     // static outside, merged per material
  private inSt = new THREE.Group();   // static inside, merged per material
  private mats = new Set<THREE.Material>();
  private texs: THREE.Texture[] = [];
  private things: Thing[] = [];
  private shades: Shade[] = [];
  private keep: { x: number; z: number; r: number }[] = [];
  private slot = { wall: -1, field: -1, yard: -1, spill: -1, shed: -1, ceil: -1, desk: -1, dial: -1, door: -1 };
  private gx = { wall: -1, field: -1, yard: -1, shed: -1, winS: -1, winE: -1, ceil: -1, desk: -1, dial: -1 };
  private hood!: THREE.Group;
  private fxTime = { value: 0 };
  private wallK = 1;
  private m!: ReturnType<Station01['materials']>;

  constructor(origin: THREE.Vector3) {
    this.o = origin.clone();
    this.group.position.copy(this.o);
    this.group.name = 'Station01';
    fieldFlood.count = 2;  // 0 and 1 belong to the truck
    hutFlood.count = 0;
    this.m = this.materials();
    this.plan();
    this.group.add(this.st, this.interior);
    this.interior.add(this.inSt);
    this.horizon();
    this.fence();
    this.power();
    this.hut();
    this.hutInside();
    this.shed();
    this.transit();
    this.markers();
    this.lamp();
    this.cable();
    this.ground();
    this.scatter();
    this.fx();
    mergeStatic(this.st);
    mergeStatic(this.inSt);
    this.group.add(this.glow.build());
    this.texs.push((this.glow.points.material as THREE.ShaderMaterial).uniforms.uMap.value);
    this.walkable();
    this.lights = this.nightLights();

    const ox = this.o.x, oz = this.o.z;
    const w = (x: number, y: number, z: number) => new THREE.Vector3(x + ox, y + this.o.y, z + oz);
    this.hutBounds = { minX: HUT.x0 + ox, maxX: HUT.x1 + ox, minZ: HUT.z0 + oz, maxZ: HUT.z1 + oz };
    this.truckCollider = { minX: TRUCK.x - 1.05 + ox, maxX: TRUCK.x + 1.05 + ox, minZ: TRUCK.z - 2.8 + oz, maxZ: TRUCK.z + 2.8 + oz };
    const mid = FOOT_A.clone().lerp(POST_A, 0.5);
    // FRAME 04 is taken looking along the cable from the hut side, so the far end's cut
    // face (copper) is seen nearly head-on across the gap
    const mSpot = { x: 7.45, z: -7.33 }, cSpot = { x: CUT.x - 0.18, z: CUT.z + 1.3 };
    this.anchors = {
      arrive: { x: ARRIVE.x + ox, z: ARRIVE.z + oz, yaw: yawTo(ARRIVE.x, ARRIVE.z, -4, 0) },
      truck: { x: TRUCK.x + ox, z: TRUCK.z + oz, heading: TRUCK.heading },
      markerTarget: w(mid.x, 0.35, mid.z),
      cableTarget: w(CUT.x, CUT.y, CUT.z),
      photoSpots: {
        marker: { x: mSpot.x + ox, z: mSpot.z + oz, yaw: yawTo(mSpot.x, mSpot.z, mid.x, mid.z), pitch: pitchTo(mSpot.x, mSpot.z, mid.x, 0.35, mid.z) },
        cable: { x: cSpot.x + ox, z: cSpot.z + oz, yaw: yawTo(cSpot.x, cSpot.z, CUT.x, CUT.z), pitch: pitchTo(cSpot.x, cSpot.z, CUT.x, CUT.y, CUT.z) },
      },
      points: {
        transit: w(T.x, PIER.top + 0.3, T.z), footing: w(FOOT_A.x, 0, FOOT_A.z), markerA: w(POST_A.x, 1.05, POST_A.z),
        markerB: w(POST_B.x, 1.05, POST_B.z), lamp: w(LAMP.x, 1.86, LAMP.z), hutDoor: w(-9.95, 1.0, 4.6),
        hutInside: w(-11.0, 0, 3.2), receiver: w(-11.85, 0.98, 1.8), fieldPhone: w(-12.9, 1.45, 2.55),
        generator: w(-16.2, 1.0, -0.4), gateInside: w(-22.3, 0, 23.5), stakesC: w(STAKES[0], 0.3, T.z),
      },
    };
  }

  // ---------- materials ----------
  private track<M extends THREE.Material>(m: M): M { this.mats.add(m); return m; }
  private tex<X extends THREE.Texture>(t: X): X { this.texs.push(t); return t; }
  private lit(o: THREE.MeshStandardMaterialParameters, fall = 0.03) { return this.track(floodlit(new THREE.MeshStandardMaterial(o), fall, fieldFlood)); }
  private litIn(o: THREE.MeshStandardMaterialParameters) { return this.track(floodlit(new THREE.MeshStandardMaterial(o), 0.25, hutFlood)); }
  private basic(o: THREE.MeshBasicMaterialParameters) { return this.track(new THREE.MeshBasicMaterial(o)); }

  private materials() {
    const atlas = this.tex(signAtlas());
    // Codex's surfaces (round 5, metre UVs): render 2 x 2 m, concrete 1 x 1 m, weathered
    // wood 0.5 x 1 m with the grain along the length, floor boards 1.2 x 1.2 m. World loads
    // them (loadArtFor) before it builds the station. The hut's furniture keeps its
    // varnished canvas wood.
    const woodIn = this.tex(woodTex(21, '#7a5638', [1 / 0.6, 1 / 1.2]));
    return {
      atlas: this.lit({ map: atlas, roughness: 0.75 }, 0.03),
      stucco: this.lit({ map: artTexture('stucco', [1 / 2, 1 / 2]), roughness: 0.95 }, 0.03),
      roof: this.lit({ map: artTexture('asphalt', [1 / 2, 1 / 2]), color: 0x8a8580, roughness: 0.95 }, 0.03),
      concrete: this.lit({ map: artTexture('oldConcrete', [1, 1]), roughness: 0.95 }, 0.03),
      wood: this.lit({ map: artTexture('weatheredWood', [1 / 0.5, 1 / 1.0]), roughness: 0.95 }, 0.03),
      woodDark: this.lit({ map: artTexture('weatheredWood', [1 / 0.5, 1 / 1.5]), color: 0x7d6a58, roughness: 0.9 }, 0.03),
      paint: this.lit({ color: 0x31463a, roughness: 0.7 }, 0.03),
      steel: this.lit({ map: artTexture('cabinet', [1 / 0.8, 1 / 0.8]), color: 0xb4b9b2, roughness: 0.5, metalness: 0.45 }, 0.03),
      darkMetal: this.lit({ color: 0x2a2826, roughness: 0.55, metalness: 0.5 }, 0.03),
      brass: this.lit({ color: 0x6f5a36, roughness: 0.45, metalness: 0.6 }, 0.03),
      corrugated: this.lit({ map: this.tex(corrugatedTex()), roughness: 0.6, metalness: 0.35 }, 0.03),
      rust: this.lit({ color: 0x7a3a22, roughness: 0.8, metalness: 0.3 }, 0.03),
      rubber: this.lit({ color: 0x151413, roughness: 0.5 }, 0.03),
      cutFace: this.lit({ map: this.tex(cutFaceTex()), roughness: 0.45, metalness: 0.2 }, 0.03),
      fadedPaint: this.lit({ color: 0xded7c4, roughness: 0.85 }, 0.03),
      insulator: this.lit({ color: 0x5f7d6e, roughness: 0.3, metalness: 0.1 }, 0.03),
      galv: this.lit({ color: 0x9a9c98, roughness: 0.45, metalness: 0.6 }, 0.03),
      paneOut: this.basic({ color: 0xffb56b, transparent: true, opacity: 0.5, depthWrite: false }),
      paneIn: this.basic({ color: 0x0a0d14, transparent: true, opacity: 0.35, depthWrite: false }),
      wallBulb: this.basic({ color: 0xfff0d6 }),
      fieldBulb: this.basic({ color: 0xfff2d8 }),
      lampBulb: this.basic({ color: 0xffd9a0 }),
      lens: this.basic({ color: 0xffd28a }),
      shadeOut: this.lit({ color: 0x33403a, roughness: 0.5, metalness: 0.45, side: THREE.DoubleSide }, 0.03),
      // inside
      plaster: this.litIn({ map: this.tex(plasterTex()), roughness: 0.95 }),
      dado: this.litIn({ color: 0x3c4a3c, roughness: 0.8 }),
      boards: this.litIn({ map: artTexture('floorboards', [1 / 1.2, 1 / 1.2]), roughness: 0.85 }),
      ceiling: this.litIn({ map: woodIn, color: 0x9a8a78, roughness: 0.9 }),
      woodIn: this.litIn({ map: woodIn, roughness: 0.75 }),
      blackIn: this.litIn({ color: 0x1c1b1a, roughness: 0.55, metalness: 0.2 }),
      crinkle: this.litIn({ color: 0x2b2c2a, roughness: 0.8, metalness: 0.3 }),
      chrome: this.litIn({ color: 0xb8b6ae, roughness: 0.25, metalness: 0.85 }),
      cabinet: this.litIn({ map: artTexture('cabinet', [1 / 0.9, 1 / 0.9]), color: 0x9fb0a6, roughness: 0.55, metalness: 0.4 }),
      ledger: this.litIn({ map: this.tex(ledgerTex()), roughness: 0.9 }),
      cloth: this.litIn({ color: 0x2c3d33, roughness: 0.9 }),
      diagram: this.litIn({ map: this.tex(arrangementTex()), roughness: 0.85 }),
      paperIn: this.litIn({ color: 0xd9cfb4, roughness: 0.9 }),
      doorIn: this.litIn({ color: 0x31463a, roughness: 0.7 }),
      dial: this.basic({ map: this.tex(dialTex()), toneMapped: false }),
      hutBulb: this.basic({ color: 0xfff0d0 }),
      shadeIn: this.litIn({ color: 0x2f4a3a, roughness: 0.45, metalness: 0.35, side: THREE.DoubleSide }),
    };
  }

  // ---------- layout of things that are not buildings ----------
  // Rocks and plants are placed first, so the ground map can shade under them.
  private plan() {
    const r = rng(2024);
    const keepRect = (x0: number, x1: number, z0: number, z1: number, x: number, z: number, m: number) => x > x0 - m && x < x1 + m && z > z0 - m && z < z1 + m;
    const free = (x: number, z: number, rad: number) => {
      if (keepRect(-13.6, -8.4, 0.9, 5.6, x, z, rad + 0.6)) return false;                      // hut and step
      if (keepRect(SHED.x0, SHED.x1, SHED.z0, SHED.z1 + 1.7, x, z, rad + 0.7)) return false;  // shed and drum
      if (keepRect(-28.5, -11.8, 25.6, 41, x, z, rad + 0.5)) return false;                     // pad and truck
      if (keepRect(-22.0, -5.5, 4.8, 26.5, x, z, rad) && polyDist(x, z, [[-19.25, 26], [-18, 20], [-15, 13.5], [-12.5, 9.5], [-8, 7.5]]) < 4.2 + rad) return false;
      if (Math.hypot(x - T.x, z - T.z) < 3.4 + rad) return false;
      if (Math.hypot(x - LAMP.x, z - LAMP.z) < 1.4 + rad) return false;
      for (const p of [FOOT_A, POST_A, POST_B]) if (Math.hypot(x - p.x, z - p.z) < 1.6 + rad) return false;
      if (Math.hypot(x - POLE.x, z - POLE.z) < 1.2 + rad) return false;
      if (Math.hypot(x - 7.45, z + 7.33) < 1.4 + rad) return false;                              // photo spot for A
      if (x > 6 && x < 60 && Math.abs(z - T.z) < 1.1 + rad) return false;                       // the C stakes
      if (polyDist(x, z, STATION_TRACK) < 3.4 + rad) return false;
      if (inRect(STATION_PATCH, ...stationToRoad(x, z), -(rad + 1))) return false;              // the road's own plants are there
      for (const p of PATHS) if (polyDist(x, z, p) < 1.0 + rad) return false;
      if (polyDist(x, z, this.cablePath2D()) < 0.9 + rad) return false;
      const onFence = (Math.abs(z - FENCE.z1) < 0.8 + rad || Math.abs(z - FENCE.z0) < 0.8 + rad) && x > FENCE.x0 - 1 && x < FENCE.x1 + 1
        || (Math.abs(x - FENCE.x0) < 0.8 + rad || Math.abs(x - FENCE.x1) < 0.8 + rad) && z > FENCE.z0 - 1 && z < FENCE.z1 + 1;
      if (onFence) return false;
      for (const k of this.keep) if (Math.hypot(x - k.x, z - k.z) < (k.r + rad) * 0.85) return false;
      return true;
    };
    const put = (kind: Thing['kind'], x: number, z: number, s: number, rad: number) => {
      this.things.push({ kind, x, z, s, ry: r() * Math.PI * 2, tilt: (r() - 0.5) * 0.3, tone: r() });
      this.keep.push({ x, z, r: rad });
    };
    const inside = (x: number, z: number) => x > FENCE.x0 + 0.5 && x < FENCE.x1 - 0.5 && z > FENCE.z0 + 0.5 && z < FENCE.z1 - 0.5;
    const scatter = (kind: Thing['kind'], n: number, lo: number, hi: number, rad: number, area: 'in' | 'out', reach = 140) => {
      let placed = 0, tries = 0;
      while (placed < n && tries++ < n * 60) {
        const x = area === 'in' ? FENCE.x0 + 1 + r() * 58 : (r() - 0.5) * reach * 2;
        const z = area === 'in' ? FENCE.z0 + 1 + r() * 48 : (r() - 0.5) * reach * 2 + 8;
        if (area === 'out' && (inside(x, z) || Math.hypot(x, z - 8) > reach)) continue;
        const s = lo + Math.pow(r(), 1.6) * (hi - lo);
        if (!free(x, z, s * rad)) continue;
        put(kind, x, z, s, s * rad);
        placed++;
      }
    };
    // the few big things first, then the small ones fill in
    scatter('block', 10, 0.7, 1.6, 0.9, 'out', 60);
    scatter('creo', 18, 0.7, 1.25, 0.65, 'in');
    scatter('yucca', 12, 0.7, 1.15, 0.5, 'in');
    scatter('block', 26, 0.18, 0.62, 0.9, 'in');
    scatter('slab', 30, 0.15, 0.5, 1.1, 'in');
    scatter('grass', 70, 0.7, 1.3, 0.25, 'in');
    scatter('creo', 300, 0.6, 1.4, 0.65, 'out');
    scatter('yucca', 110, 0.6, 1.2, 0.5, 'out');
    scatter('block', 150, 0.2, 1.1, 0.9, 'out');
    scatter('slab', 130, 0.18, 0.9, 1.1, 'out');
    scatter('grass', 360, 0.7, 1.4, 0.25, 'out');
    // rubble round the pier, broken concrete at the empty footing, the cairn at the post
    for (let i = 0; i < 18; i++) {
      const a = r() * Math.PI * 2, d = 0.62 + r() * 0.55, x = T.x + Math.cos(a) * d, z = T.z + Math.sin(a) * d;
      if (polyDist(x, z, this.cablePath2D()) < 0.15) continue;
      this.things.push({ kind: r() < 0.5 ? 'block' : 'slab', x, z, s: 0.06 + r() * 0.1, ry: r() * 6, tilt: (r() - 0.5) * 0.5, tone: 0.2 + r() * 0.3 });
    }
    for (let i = 0; i < 6; i++) {
      const a = r() * Math.PI * 2, d = 0.42 + r() * 0.3;
      this.things.push({ kind: 'slab', x: FOOT_A.x + Math.cos(a) * d, z: FOOT_A.z + Math.sin(a) * d, s: 0.05 + r() * 0.06, ry: r() * 6, tilt: (r() - 0.5) * 0.6, tone: 0.95 });
    }
    for (let i = 0; i < 8; i++) {
      const a = i / 8 * Math.PI * 2 + r() * 0.4, d = 0.13 + r() * 0.08;
      this.things.push({ kind: 'block', x: POST_A.x + Math.cos(a) * d, z: POST_A.z + Math.sin(a) * d, s: 0.09 + r() * 0.07, ry: r() * 6, tilt: (r() - 0.5) * 0.4, tone: r() });
    }
    // shade under all of it, and under the buildings and posts
    for (const t of this.things) {
      const k = t.kind === 'grass' ? 0.25 : t.kind === 'creo' ? 0.5 : t.kind === 'yucca' ? 0.4 : 0.55;
      const rr = t.kind === 'creo' ? t.s * 0.9 : t.kind === 'yucca' ? t.s * 0.6 : t.kind === 'grass' ? t.s * 0.25 : t.s * (t.kind === 'slab' ? 1.3 : 1.05);
      this.shades.push({ x: t.x, z: t.z, r: rr, k });
    }
    for (let x = -13.4; x <= -8.6; x += 0.6) for (const z of [0.85, 5.15]) this.shades.push({ x, z, r: 0.7, k: 0.35 });
    for (let z = 1.0; z <= 5.0; z += 0.6) for (const x of [-13.65, -8.35]) this.shades.push({ x, z, r: 0.7, k: 0.35 });
    for (let x = SHED.x0; x <= SHED.x1; x += 0.5) for (const z of [SHED.z0 - 0.1, SHED.z1 + 0.1]) this.shades.push({ x, z, r: 0.6, k: 0.4 });
    for (const x of STAKES) this.shades.push({ x, z: T.z, r: 0.15, k: 0.5 });
    this.shades.push({ x: CUT.x, z: CUT.z, r: 0.35, k: 0.3 });
    this.shades.push({ x: T.x, z: T.z, r: 1.0, k: 0.6 }, { x: FOOT_A.x, z: FOOT_A.z, r: 0.55, k: 0.5 }, { x: POST_B.x, z: POST_B.z, r: 0.5, k: 0.5 },
      { x: POST_A.x, z: POST_A.z, r: 0.4, k: 0.5 }, { x: LAMP.x, z: LAMP.z, r: 0.6, k: 0.35 }, { x: POLE.x, z: POLE.z, r: 0.45, k: 0.5 });
  }

  // The cable's course on the ground, from the plinth to the pier (for keeping it clear).
  private cablePath2D(): [number, number][] {
    return [[-9.1, 5.0], [-8.2, 5.75], [-7.2, 5.95], [-6.2, 5.7], [-4.7, 5.8], [-3.8, 5.6], [-2.6, 5.0], [-1.4, 4.4], [-0.4, 3.6], [0.6, 2.7],
      [1.4, 1.7], [2.1, 0.9], [2.75, 0.1], [3.05, -0.6], [CUT.x, CUT.z], [3.22, -2.2], [3.15, -3.0], [3.1, -3.5]];
  }

  // ---------- small helpers ----------
  private col(x0: number, x1: number, z0: number, z1: number) {
    this.colliders.push({ minX: x0 + this.o.x, maxX: x1 + this.o.x, minZ: z0 + this.o.z, maxZ: z1 + this.o.z });
  }
  private addZone(id: string, x0: number, x1: number, z0: number, z1: number) {
    const z: Zone = { id, minX: x0 + this.o.x, maxX: x1 + this.o.x, minZ: z0 + this.o.z, maxZ: z1 + this.o.z, enabled: true };
    this.zones.push(z); this.zone[id] = z;
  }
  private hidden?: THREE.MeshBasicMaterial;
  // Invisible box that interaction rays can hit.
  private proxy(id: string, w: number, h: number, d: number, x: number, y: number, z: number, ry = 0) {
    this.hidden ??= this.track(new THREE.MeshBasicMaterial({ visible: false }));
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), this.hidden);
    m.position.set(x, y, z); m.rotation.y = ry; m.name = id;
    this.group.add(m);
    this.proxies[id] = m;
    return m;
  }
  private occluder(w: number, h: number, d: number, x: number, y: number, z: number) {
    this.hidden ??= this.track(new THREE.MeshBasicMaterial({ visible: false }));
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), this.hidden);
    m.position.set(x, y, z);
    this.group.add(m);
    this.occluders.push(m);
  }
  private setGlow(i: number, color: THREE.ColorRepresentation, k = 1) {
    const col = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    const c = new THREE.Color(color).multiplyScalar(k);
    col.setXYZ(i, c.r, c.g, c.b); col.needsUpdate = true;
  }
  private y(x: number, z: number) { return this.terrain.surface(x, z); }
  /** Height of the ground as drawn, at a world point (the truck drives on it). */
  groundAt(x: number, z: number) { return this.o.y + this.terrain.surface(x - this.o.x, z - this.o.z); }

  // ---------- ground, gravel, road ----------
  private ground() {
    const map = this.tex(groundMapTex(this.shades));
    const sand = groundMap(this.lit({ map: artTexture('desert', [1 / 3.5, 1 / 3.5]), vertexColors: true, color: 0xe2d6c2, roughness: 1 }, 0.04), map);
    const ground = new THREE.Mesh(this.terrain.geometry(), sand);
    ground.name = 'stationGround';
    this.group.add(ground);

    // gravel: the pad and the road outside the gate, the track and the yard inside
    const gravel = groundMap(this.lit({ map: this.tex(gravelTex()), color: 0xc4bcae, roughness: 1, side: THREE.DoubleSide, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -2 }, 0.04), map);
    const r = rng(303);
    const shape = (pts: [number, number][], jit: number, keepEdge?: (x: number, z: number) => boolean) => {
      const s = new THREE.Shape(pts.map(([x, z]) => {
        const j = keepEdge?.(x, z) ? 0 : jit;
        return new THREE.Vector2(x + (r() - 0.5) * j, -(z + (r() - 0.5) * j));
      }));
      const g = new THREE.ShapeGeometry(s);
      g.rotateX(-Math.PI / 2);
      const p = g.attributes.position as THREE.BufferAttribute, uv = g.attributes.uv as THREE.BufferAttribute;
      for (let i = 0; i < p.count; i++) { p.setY(i, this.y(p.getX(i), p.getZ(i)) + 0.012); uv.setXY(i, p.getX(i), p.getZ(i)); }
      return g;
    };
    // densify an outline so the jitter makes a ragged edge
    const dense = (pts: [number, number][], step = 1.6) => {
      const out: [number, number][] = [];
      for (let i = 0; i < pts.length; i++) {
        const a = pts[i], b = pts[(i + 1) % pts.length], n = Math.max(1, Math.ceil(Math.hypot(b[0] - a[0], b[1] - a[1]) / step));
        for (let k = 0; k < n; k++) out.push([a[0] + (b[0] - a[0]) * k / n, a[1] + (b[1] - a[1]) * k / n]);
      }
      return out;
    };
    const atFence = (_x: number, z: number) => Math.abs(z - FENCE.z1) < 0.06;
    const parts: THREE.BufferGeometry[] = [
      shape(dense([[-21.7, 26.0], [-16.8, 26.0], [-15.6, 19.5], [-13.0, 13.6], [-6.4, 10.8], [-6.0, 8.6], [-7.6, 5.2], [-13.8, 5.2], [-16.2, 8.6], [-18.6, 13.6], [-20.7, 19.5]]), 0.7, atFence),
      shape(dense([[-27.2, 26.0], [-12.2, 26.0], [-12.6, 33.5], [-15.2, 39.2], [-23.6, 39.8], [-27.6, 34.8]]), 0.8, atFence),
    ];
    // the track itself is the road's (drive/corridors.ts), from the pad out
    const merged = mergeGeometries(parts.map((g) => { const n = g.index ? g.toNonIndexed() : g; n.computeVertexNormals(); return n; }), false)!;
    for (let i = 0; i < (merged.attributes.normal as THREE.BufferAttribute).count; i++) (merged.attributes.normal as THREE.BufferAttribute).setXYZ(i, 0, 1, 0);
    const gm = new THREE.Mesh(merged, gravel);
    gm.name = 'stationGravel';
    this.group.add(gm);
  }

  // Dark mesas and far ridges on the horizon (horizon.ts), and the road's far rings round
  // the drive, placed as they lie on the map (geo.ts): the skyline is the road's too.
  private horizon() {
    const mesh = new THREE.Mesh(stationMesas((x, z) => Terrain.height(x, z)), this.basic({ vertexColors: true, fog: false, side: THREE.DoubleSide }));
    mesh.name = 'mesas';
    this.group.add(mesh, fromRoad(roadMesas(), 'station01'));
    for (const l of STATION_FAR_LIGHTS) this.glow.add(l.x, l.y, l.z, l.size, l.color, l.blink, l.phase);
  }

  // ---------- fence, gate, wires ----------
  private wires: number[] = [];
  private wireCol: number[] = [];
  private wire(a: THREE.Vector3, b: THREE.Vector3, sag = 0, n = 1) {
    const pts: THREE.Vector3[] = [];
    for (let i = 0; i <= n; i++) { const t = i / n; pts.push(a.clone().lerp(b, t).setY(a.y + (b.y - a.y) * t - sag * 4 * t * (1 - t))); }
    for (let i = 0; i < n; i++) this.wirePush(pts[i], pts[i + 1]);
  }
  private wirePush(a: THREE.Vector3, b: THREE.Vector3) {
    this.wires.push(a.x, a.y, a.z, b.x, b.y, b.z);
    for (const p of [a, b]) {
      // light baked from the lamps nearby, so the wire catches the glow
      let k = 0;
      for (const f of this.bakeLights) {
        const d = p.distanceTo(f.p), win = Math.max(0, 1 - d / (f.w * 1.6 + 8));
        k += f.w * win * win / (1 + d * d * 0.05);
      }
      k = Math.min(1, k * 0.05);
      this.wireCol.push(0.011 + k * 0.2, 0.011 + k * 0.13, 0.012 + k * 0.07);
    }
  }
  private bakeLights = [
    { p: WALL_LAMP, w: BASE.wall }, { p: new THREE.Vector3(POLE.x - 1.0, 6.1, POLE.z - 0.3), w: BASE.yard },
    { p: new THREE.Vector3(LAMP.x + 0.2, 1.8, LAMP.z - 0.2), w: BASE.field }, { p: new THREE.Vector3(-15.0, 1.8, -0.4), w: BASE.shed },
  ];

  private fence() {
    const st = this.st, m = this.m;
    const post = (x: number, z: number, h = 1.45, r = 0.055) => {
      const y0 = this.y(x, z) - 0.35;
      metreUV(cyl(st, r * 0.9, r, h + 0.35, m.wood, x, y0 + (h + 0.35) / 2, z, 6));
    };
    const strands = [0.32, 0.62, 0.92, 1.22];
    const run = (ax: number, az: number, bx: number, bz: number, step = 3.6) => {
      const n = Math.max(1, Math.round(Math.hypot(bx - ax, bz - az) / step));
      for (let i = 0; i <= n; i++) post(ax + (bx - ax) * i / n, az + (bz - az) * i / n);
      for (const h of strands) {
        for (let i = 0; i < n; i++) {
          const x0 = ax + (bx - ax) * i / n, z0 = az + (bz - az) * i / n, x1 = ax + (bx - ax) * (i + 1) / n, z1 = az + (bz - az) * (i + 1) / n;
          const a = new THREE.Vector3(x0, this.y(x0, z0) + h, z0), b = new THREE.Vector3(x1, this.y(x1, z1) + h, z1);
          this.wire(a, b, 0.03, 2);
          // barbs every 22 cm: two short crossing pieces
          const d = b.clone().sub(a), len = d.length(); d.normalize();
          const s1 = new THREE.Vector3(-d.z, 0, d.x), up = new THREE.Vector3(0, 1, 0);
          for (let t = 0.11; t < len; t += 0.22) {
            const p = a.clone().addScaledVector(d, t); p.y -= 0.03 * 4 * (t / len) * (1 - t / len);
            const k = (t * 7.1) % 1 * Math.PI;
            const q1 = s1.clone().multiplyScalar(Math.cos(k)).addScaledVector(up, Math.sin(k)).multiplyScalar(0.022);
            const q2 = s1.clone().multiplyScalar(-Math.sin(k)).addScaledVector(up, Math.cos(k)).multiplyScalar(0.022);
            this.wirePush(p.clone().add(q1), p.clone().sub(q1));
            this.wirePush(p.clone().add(q2), p.clone().sub(q2));
          }
        }
      }
    };
    const { x0, x1, z0, z1 } = FENCE;
    run(x0, z0, x1, z0); run(x1, z0, x1, z1); run(x0, z1, x0, z0);
    run(x1, z1, GATE.e, z1); run(GAP_W, z1, x0, z1);
    // corner braces
    for (const [x, z, dx, dz] of [[x0, z0, 1, 0], [x1, z0, -1, 0], [x0, z1, 1, 0], [x1, z1, -1, 0], [x0, z0, 0, 1], [x1, z0, 0, 1], [x0, z1, 0, -1], [x1, z1, 0, -1]]) {
      rod(st, new THREE.Vector3(x, G + 1.25, z), new THREE.Vector3(x + dx * 1.6, G + 0.05, z + dz * 1.6), 0.035, m.wood, 5);
    }
    // gate posts and the walk-through post
    for (const x of [GATE.w, GATE.e]) metreUV(cyl(st, 0.1, 0.12, 2.1, m.wood, x, G + 0.85, z1, 8));
    metreUV(cyl(st, 0.07, 0.08, 1.9, m.wood, GAP_W, G + 0.75, z1, 6));
    // the pipe gate, closed and chained
    const gx0 = GATE.w + 0.12, gx1 = GATE.e - 0.12, gz = z1 + 0.02;
    const gv = (x: number, h: number) => new THREE.Vector3(x, G + h, gz);
    for (const h of [0.22, 0.5, 0.78, 1.06, 1.3]) rod(st, gv(gx0, h), gv(gx1, h), 0.024, m.galv, 6);
    for (const x of [gx0, gx1]) rod(st, gv(x, 0.12), gv(x, 1.36), 0.03, m.galv, 6);
    rod(st, gv(gx0, 1.3), gv(gx0 + 2.2, 0.22), 0.02, m.galv, 6);
    rod(st, gv(gx0 + 2.2, 0.22), gv(gx1, 1.3), 0.02, m.galv, 6);
    for (const h of [0.35, 1.15]) box(st, 0.16, 0.05, 0.08, m.darkMetal, GATE.w + 0.06, G + h, gz);
    for (let i = 0; i < 5; i++) {
      const t = new THREE.Mesh(new THREE.TorusGeometry(0.03, 0.007, 4, 8), m.darkMetal);
      t.position.set(GATE.e - 0.12 + Math.sin(i * 1.3) * 0.05, G + 1.02 - i * 0.045, gz + 0.03 + Math.cos(i * 1.3) * 0.04);
      t.rotation.set(i % 2 ? Math.PI / 2 : 0, i * 0.7, 0);
      st.add(t);
    }
    box(st, 0.06, 0.07, 0.03, m.brass, GATE.e - 0.1, G + 0.8, gz + 0.06);
    const sign = plane(st, 0.8, 0.4, m.atlas, (gx0 + gx1) / 2, G + 0.92, gz + 0.04);
    regionUV(sign.geometry, 'gate');
    const back = plane(st, 0.8, 0.4, m.darkMetal, (gx0 + gx1) / 2, G + 0.92, gz + 0.035, Math.PI);
    back.name = 'gateSignBack';
    this.proxy('gate', 4.7, 1.5, 0.35, (GATE.w + GATE.e) / 2, G + 0.75, z1);
    // the parking spot for the truck, marked only by the gravel and old ruts
    this.proxy('truckSpot', 2.2, 1.9, 5.6, TRUCK.x, 0.95, TRUCK.z, TRUCK.heading);
  }

  // ---------- power line, transformer, yard light ----------
  private power() {
    const st = this.st, m = this.m;
    const poles: [number, number][] = [[POLE.x, POLE.z], [-3.6, 31.5], [2.5, 58], [9, 85], [16, 112], [23, 140]];
    const tops: THREE.Vector3[][] = [];
    poles.forEach(([x, z], i) => {
      const y0 = this.y(x, z), h = i === 0 ? 8.4 : 9.0;
      metreUV(cyl(st, 0.11, 0.14, h + 1.2, m.woodDark, x, y0 + h / 2 - 0.6, z, 7));
      const next = poles[Math.min(poles.length - 1, i + 1)], prev = poles[Math.max(0, i - 1)];
      const ang = Math.atan2(next[0] - prev[0], next[1] - prev[1]);
      const arm = box(st, 1.7, 0.1, 0.1, m.woodDark, x, y0 + h - 0.45, z);
      arm.rotation.y = ang;
      metreUV(arm);
      const ax = Math.cos(ang) * 0.7, az = -Math.sin(ang) * 0.7;
      const ends = [new THREE.Vector3(x + ax, y0 + h - 0.28, z + az), new THREE.Vector3(x - ax, y0 + h - 0.28, z - az), new THREE.Vector3(x, y0 + h + 0.12, z)];
      for (const e of ends) cyl(st, 0.045, 0.06, 0.16, m.insulator, e.x, e.y - 0.06, e.z, 6);
      tops.push(ends);
      if (i === poles.length - 1) {
        // the line goes on into the dark
        const d = new THREE.Vector3(next[0] - prev[0], 0, next[1] - prev[1]).normalize().multiplyScalar(40);
        for (const e of ends) this.wire(e, e.clone().add(d).setY(e.y + 1), 0.6, 6);
      }
    });
    for (let i = 0; i < poles.length - 1; i++) for (let k = 0; k < 3; k++) this.wire(tops[i][k], tops[i + 1][k], 0.55, 8);
    // the yard pole ends the line: a guy wire holds it against the pull, to the north
    this.wire(new THREE.Vector3(POLE.x, G + 7.6, POLE.z), new THREE.Vector3(POLE.x - 0.6, G + 0.05, POLE.z - 3.0), 0, 1);
    box(st, 0.3, 0.12, 0.3, m.darkMetal, POLE.x - 0.6, G + 0.04, POLE.z - 3.0);
    // transformer can, bushings and the drop to the hut
    const tf = new THREE.Vector3(POLE.x + 0.3, G + 6.4, POLE.z);
    cyl(st, 0.24, 0.24, 0.78, m.galv, tf.x, tf.y, tf.z, 12);
    cyl(st, 0.25, 0.25, 0.05, m.darkMetal, tf.x, tf.y + 0.41, tf.z, 12);
    for (const dz of [-0.1, 0.1]) cyl(st, 0.025, 0.035, 0.14, m.insulator, tf.x, tf.y + 0.5, tf.z + dz, 6);
    box(st, 0.08, 0.5, 0.06, m.darkMetal, POLE.x + 0.12, tf.y, POLE.z);
    const tag = plane(st, 0.3, 0.15, m.atlas, POLE.x, G + 2.6, POLE.z + 0.125);
    regionUV(tag.geometry, 'tag');
    for (let k = 0; k < 2; k++) this.wire(new THREE.Vector3(tf.x, tf.y + 0.55, tf.z + (k - 0.5) * 0.2), tops[0][2 - k * 2].clone().setY(tops[0][2 - k * 2].y - 0.1), 0.05, 2);
    const mast = new THREE.Vector3(-9.05, 3.55, 4.45);
    for (let k = 0; k < 3; k++) this.wire(new THREE.Vector3(tf.x - 0.1, tf.y - 0.3 + k * 0.04, tf.z), mast.clone().add(new THREE.Vector3(0, -0.05 + k * 0.03, 0)), 0.32, 8);
    // the yard light: a dusk-to-dawn lamp on an arm, aimed at the hut and the yard
    const lx = POLE.x - 1.05, lz = POLE.z - 0.3, ly = G + 6.1;
    rod(st, new THREE.Vector3(POLE.x, ly + 0.1, POLE.z), new THREE.Vector3(lx, ly + 0.25, lz), 0.03, m.galv, 6);
    rod(st, new THREE.Vector3(POLE.x, ly - 0.4, POLE.z), new THREE.Vector3(lx + 0.4, ly + 0.2, lz + 0.1), 0.02, m.galv, 5);
    const head = box(st, 0.5, 0.16, 0.3, m.galv, lx, ly + 0.2, lz);
    head.rotation.y = Math.atan2(POLE.z - lz, -(POLE.x - lx));
    box(st, 0.4, 0.04, 0.22, m.lens, lx, ly + 0.1, lz);
    this.slot.yard = addFlood(lx + this.o.x, ly + this.o.y, lz + this.o.z, BASE.yard, 0xffb468, fieldFlood);
    this.gx.yard = this.glow.add(lx, ly + 0.05, lz, 1.6, 0xffc07a);
    this.col(POLE.x - 0.2, POLE.x + 0.2, POLE.z - 0.2, POLE.z + 0.2);
  }

  // ---------- the field hut ----------
  // One piece of wall: stucco box outside, plaster (dark paint below 0.9 m) inside.
  private wallPiece(x0: number, x1: number, y0: number, y1: number, z0: number, z1: number, side: 'n' | 's' | 'e' | 'w') {
    metreUV(box(this.st, x1 - x0, y1 - y0, z1 - z0, this.m.stucco, (x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2));
    const a0 = side === 'n' || side === 's' ? Math.max(x0, HUT.x0) : Math.max(z0, HUT.z0);
    const a1 = side === 'n' || side === 's' ? Math.min(x1, HUT.x1) : Math.min(z1, HUT.z1);
    if (a1 - a0 < 0.01) return;
    const face = (b0: number, b1: number, mat: THREE.Material) => {
      if (b1 - b0 < 0.005) return;
      const c = (a0 + a1) / 2, yc = (b0 + b1) / 2, e = 0.002;
      let p: THREE.Mesh;
      if (side === 's') p = plane(this.inSt, a1 - a0, b1 - b0, mat, c, yc, z0 - e, Math.PI);
      else if (side === 'n') p = plane(this.inSt, a1 - a0, b1 - b0, mat, c, yc, z1 + e, 0);
      else if (side === 'w') p = plane(this.inSt, a1 - a0, b1 - b0, mat, x1 + e, yc, c, Math.PI / 2);
      else p = plane(this.inSt, a1 - a0, b1 - b0, mat, x0 - e, yc, c, -Math.PI / 2);
      metreUV(p);
    };
    face(y0, Math.min(y1, 0.9), this.m.dado);
    face(Math.max(y0, 0.9), y1, this.m.plaster);
    if (y0 < 0.9 && y1 > 0.9) {
      // a thin rail on top of the dark paint
      const rw = a1 - a0;
      if (side === 's') box(this.inSt, rw, 0.035, 0.02, this.m.woodIn, (a0 + a1) / 2, 0.9, z0 - 0.01);
      else if (side === 'n') box(this.inSt, rw, 0.035, 0.02, this.m.woodIn, (a0 + a1) / 2, 0.9, z1 + 0.01);
      else if (side === 'w') box(this.inSt, 0.02, 0.035, rw, this.m.woodIn, x1 + 0.01, 0.9, (a0 + a1) / 2);
      else box(this.inSt, 0.02, 0.035, rw, this.m.woodIn, x0 - 0.01, 0.9, (a0 + a1) / 2);
    }
  }

  private hut() {
    const st = this.st, m = this.m, { x0, x1, z0, z1, t, h } = HUT;
    // plinth and the step at the door
    metreUV(box(st, 5.0, 0.25, 4.0, m.concrete, -11, -0.125, 3.0));
    metreUV(box(st, 1.4, 0.18, 0.42, m.concrete, -9.95, -0.16, 5.21));
    // walls
    this.wallPiece(x0 - t, x1 + t, 0, h, z0 - t, z0, 'n');
    this.wallPiece(x0 - t, x0, 0, h, z0, z1, 'w');
    this.wallPiece(x1, x1 + t, 0, h, z0, WIN_E.z0, 'e');
    this.wallPiece(x1, x1 + t, 0, SILL, WIN_E.z0, WIN_E.z1, 'e');
    this.wallPiece(x1, x1 + t, HEAD, h, WIN_E.z0, WIN_E.z1, 'e');
    this.wallPiece(x1, x1 + t, 0, h, WIN_E.z1, z1, 'e');
    this.wallPiece(x0 - t, WIN_S.x0, 0, h, z1, z1 + t, 's');
    this.wallPiece(WIN_S.x0, WIN_S.x1, 0, SILL, z1, z1 + t, 's');
    this.wallPiece(WIN_S.x0, WIN_S.x1, HEAD, h, z1, z1 + t, 's');
    this.wallPiece(WIN_S.x1, DOOR.x0, 0, h, z1, z1 + t, 's');
    this.wallPiece(DOOR.x0, DOOR.x1, DOOR.h, h, z1, z1 + t, 's');
    this.wallPiece(DOOR.x1, x1 + t, 0, h, z1, z1 + t, 's');
    // the jambs sit inside the opening, so the walls stop at their inner faces
    this.col(x0 - t, x1 + t, z0 - t, z0);
    this.col(x0 - t, x0, z0, z1);
    this.col(x1, x1 + t, z0, z1);
    this.col(x0 - t, DOOR.x0 + 0.05, z1, z1 + t);
    this.col(DOOR.x1 - 0.05, x1 + t, z1, z1 + t);
    this.occluder(x1 - x0 + 2 * t, h, z1 - z0 + 2 * t, (x0 + x1) / 2, h / 2, (z0 + z1) / 2);
    // roof slab with roll roofing, a stovepipe, the service mast and the meter
    metreUV(box(st, x1 - x0 + 0.9, 0.16, z1 - z0 + 0.9, m.roof, (x0 + x1) / 2, h + 0.08, (z0 + z1) / 2));
    {
      const fw = x1 - x0 + 0.94, fd = z1 - z0 + 0.94, cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
      metreUV(box(st, fw, 0.07, 0.03, m.woodDark, cx, h + 0.03, cz - fd / 2));
      metreUV(box(st, fw, 0.07, 0.03, m.woodDark, cx, h + 0.03, cz + fd / 2));
      metreUV(box(st, 0.03, 0.07, fd, m.woodDark, cx - fw / 2, h + 0.03, cz));
      metreUV(box(st, 0.03, 0.07, fd, m.woodDark, cx + fw / 2, h + 0.03, cz));
    }
    cyl(st, 0.075, 0.075, 0.75, m.darkMetal, -12.4, h + 0.53, 2.0, 10);
    cyl(st, 0.14, 0.02, 0.1, m.darkMetal, -12.4, h + 0.95, 2.0, 10);
    cyl(st, 0.03, 0.03, 0.95, m.galv, -9.05, h + 0.55, 4.45, 6);
    const cap = cyl(st, 0.05, 0.03, 0.1, m.darkMetal, -9.05, h + 1.02, 4.45, 6); cap.rotation.z = 0.5;
    rod(st, new THREE.Vector3(x1 + t + 0.03, 1.72, 4.25), new THREE.Vector3(x1 + t + 0.03, h - 0.05, 4.25), 0.016, m.galv, 5);
    box(st, 0.1, 0.32, 0.24, m.galv, x1 + t + 0.05, 1.55, 4.25);
    cyl(st, 0.075, 0.075, 0.06, m.paneIn, x1 + t + 0.11, 1.58, 4.25, 12).rotation.z = Math.PI / 2;

    // door frame, casing and threshold (painted green like the door)
    box(st, 0.05, DOOR.h, t + 0.06, m.paint, DOOR.x0 + 0.025, DOOR.h / 2, z1 + t / 2);
    box(st, 0.05, DOOR.h, t + 0.06, m.paint, DOOR.x1 - 0.025, DOOR.h / 2, z1 + t / 2);
    box(st, DOOR.x1 - DOOR.x0 + 0.04, 0.07, t + 0.06, m.paint, (DOOR.x0 + DOOR.x1) / 2, DOOR.h + 0.035, z1 + t / 2);
    for (const x of [DOOR.x0 - 0.04, DOOR.x1 + 0.04]) box(st, 0.08, DOOR.h + 0.08, 0.025, m.paint, x, (DOOR.h + 0.08) / 2, z1 + t + 0.012);
    box(st, DOOR.x1 - DOOR.x0 + 0.16, 0.09, 0.025, m.paint, (DOOR.x0 + DOOR.x1) / 2, DOOR.h + 0.1, z1 + t + 0.012);
    metreUV(box(st, DOOR.x1 - DOOR.x0 - 0.1, 0.025, t + 0.04, m.wood, (DOOR.x0 + DOOR.x1) / 2, 0.0125, z1 + t / 2));
    // windows: frame, cross bars, sill, panes (warm outside while the light is on)
    const win = (cx: number, cz: number, wdt: number, along: 'x' | 'z') => {
      const bx = (w: number, hh: number, d: number, x: number, y: number, z: number, mat: THREE.Material) =>
        along === 'x' ? box(st, w, hh, d, mat, x, y, z) : box(st, d, hh, w, mat, x, y, z);
      const at = (u: number) => along === 'x' ? [cx + u, cz] : [cx, cz + u];
      const half = wdt / 2, ym = (SILL + HEAD) / 2;
      for (const u of [-half + 0.03, half - 0.03]) { const [x, z] = at(u); bx(0.06, HEAD - SILL, 0.1, x, ym, z, m.paint); }
      { const [x, z] = at(0); bx(wdt, 0.06, 0.1, x, HEAD - 0.03, z, m.paint); bx(wdt, 0.06, 0.1, x, SILL + 0.03, z, m.paint); }
      { const [x, z] = at(0); bx(0.03, HEAD - SILL, 0.05, x, ym, z, m.paint); bx(wdt, 0.03, 0.05, x, ym, z, m.paint); }
      // sill board outside
      if (along === 'x') box(st, wdt + 0.14, 0.04, 0.16, m.paint, cx, SILL - 0.02, cz + 0.1);
      else box(st, 0.16, 0.04, wdt + 0.14, m.paint, cx + 0.1, SILL - 0.02, cz);
      const pw = wdt - 0.08, ph = HEAD - SILL - 0.08;
      if (along === 'x') { plane(st, pw, ph, m.paneOut, cx, ym, cz + 0.012); plane(st, pw, ph, m.paneIn, cx, ym, cz - 0.012, Math.PI); }
      else { plane(st, pw, ph, m.paneOut, cx + 0.012, ym, cz, Math.PI / 2); plane(st, pw, ph, m.paneIn, cx - 0.012, ym, cz, -Math.PI / 2); }
    };
    win((WIN_S.x0 + WIN_S.x1) / 2, z1 + t / 2, WIN_S.x1 - WIN_S.x0, 'x');
    win(x1 + t / 2, (WIN_E.z0 + WIN_E.z1) / 2, WIN_E.z1 - WIN_E.z0, 'z');
    this.gx.winS = this.glow.add((WIN_S.x0 + WIN_S.x1) / 2, 1.4, z1 + t + 0.08, 1.1, 0x6a4018);
    this.gx.winE = this.glow.add(x1 + t + 0.08, 1.4, (WIN_E.z0 + WIN_E.z1) / 2, 1.1, 0x6a4018);

    // the wall lamp over the door: a goose-neck arm and an enamel shade
    rod(st, new THREE.Vector3(WALL_LAMP.x, 2.48, z1 + t), new THREE.Vector3(WALL_LAMP.x, 2.57, z1 + t + 0.17), 0.012, m.darkMetal, 5);
    rod(st, new THREE.Vector3(WALL_LAMP.x, 2.57, z1 + t + 0.17), new THREE.Vector3(WALL_LAMP.x, 2.53, WALL_LAMP.z), 0.012, m.darkMetal, 5);
    box(st, 0.1, 0.12, 0.03, m.darkMetal, WALL_LAMP.x, 2.48, z1 + t + 0.015);
    const shade = new THREE.Mesh(new THREE.ConeGeometry(0.17, 0.11, 14, 1, true), m.shadeOut);
    shade.position.set(WALL_LAMP.x, 2.475, WALL_LAMP.z); st.add(shade);
    const bulb = new THREE.Mesh(new THREE.SphereGeometry(0.035, 8, 6), m.wallBulb);
    bulb.position.set(WALL_LAMP.x, 2.42, WALL_LAMP.z); st.add(bulb);
    this.slot.wall = addFlood(WALL_LAMP.x + this.o.x, 2.2 + this.o.y, WALL_LAMP.z + 0.15 + this.o.z, BASE.wall, 0xffb966, fieldFlood);
    this.gx.wall = this.glow.add(WALL_LAMP.x, 2.4, WALL_LAMP.z, 0.75, 0xffc27a);
    this.slot.spill = addFlood(-9.95 + this.o.x, 0.9 + this.o.y, 5.25 + this.o.z, BASE.spill, 0xffb060, fieldFlood);
    // STATION 01 over the door, the junction box beside it
    const sign = plane(st, 0.6, 0.1875, m.atlas, WALL_LAMP.x, 2.262, z1 + t + 0.012);
    regionUV(sign.geometry, 'station');
    box(st, 0.62, 0.2, 0.01, m.darkMetal, WALL_LAMP.x, 2.262, z1 + t + 0.005);
    box(st, 0.26, 0.32, 0.12, m.steel, -9.1, 1.25, z1 + t + 0.06);
    const jl = plane(st, 0.2, 0.125, m.atlas, -9.1, 1.27, z1 + t + 0.121);
    regionUV(jl.geometry, 'jbox');
    cyl(st, 0.025, 0.025, 1.08, m.galv, -9.1, 0.55, z1 + t + 0.04, 6);
    box(st, 0.05, 0.05, 0.1, m.galv, -9.1, 0.04, z1 + t + 0.06);
    this.proxy('hutDoor', 1.2, 2.15, 0.18, (DOOR.x0 + DOOR.x1) / 2, 1.07, z1 + t + 0.06);
  }

  // ---------- inside the hut ----------
  private hutInside() {
    const s = this.inSt, m = this.m, { x0, x1, z0, z1 } = HUT;
    const v = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
    metreUV(plane(s, x1 - x0, z1 - z0, m.boards, (x0 + x1) / 2, 0.004, (z0 + z1) / 2, 0, -Math.PI / 2));
    metreUV(plane(s, x1 - x0, z1 - z0, m.ceiling, (x0 + x1) / 2, HUT.h - 0.003, (z0 + z1) / 2, 0, Math.PI / 2));
    for (const z of [2.2, 3.8]) box(s, x1 - x0, 0.12, 0.08, m.woodIn, (x0 + x1) / 2, HUT.h - 0.06, z);

    // the door leaf, open into the room against the east corner (hinged on the east jamb)
    const leaf = new THREE.Group();
    leaf.position.set(DOOR.x1 - 0.05, 0, z1 - 0.02);
    box(leaf, 0.98, 1.97, 0.04, m.doorIn, -0.49, 0.995, 0);
    for (const y of [0.35, 1.0, 1.65]) box(leaf, 0.9, 0.1, 0.02, m.doorIn, -0.49, y, -0.03);
    box(leaf, 0.04, 0.05, 0.12, m.chrome, -0.9, 1.0, 0.03);
    box(leaf, 0.04, 0.05, 0.12, m.chrome, -0.9, 1.0, -0.05);
    leaf.rotation.y = -1.75;
    mergeStatic(leaf);
    this.interior.add(leaf);
    this.objs.hutDoorLeaf = leaf;
    {
      const ex = DOOR.x1 - 0.05 + Math.cos(Math.PI - 1.75) * 0.98, ez = z1 - 0.02 - Math.sin(Math.PI - 1.75) * 0.98;
      this.col(Math.min(ex, DOOR.x1 - 0.05) - 0.04, Math.max(ex, DOOR.x1 - 0.05) + 0.04, Math.min(ez, z1) - 0.04, z1);
    }

    // workbench along the north wall
    const bx0 = x0 + 0.05, bx1 = x1 - 0.95, bz = z0 + 0.36, top = 0.8;
    metreUV(box(s, bx1 - bx0, 0.05, 0.72, m.woodIn, (bx0 + bx1) / 2, top - 0.025, bz), true);
    for (const x of [bx0 + 0.05, bx1 - 0.05]) for (const z of [z0 + 0.06, z0 + 0.66]) box(s, 0.06, top - 0.05, 0.06, m.woodIn, x, (top - 0.05) / 2, z);
    metreUV(box(s, bx1 - bx0 - 0.1, 0.03, 0.6, m.woodIn, (bx0 + bx1) / 2, 0.22, bz), true);
    box(s, bx1 - bx0, 0.1, 0.03, m.woodIn, (bx0 + bx1) / 2, top - 0.1, z0 + 0.71);
    this.col(bx0, bx1, z0, z0 + 0.74);
    // things on the lower shelf: boxes of spare tubes, a coil of wire
    for (let i = 0; i < 4; i++) box(s, 0.2, 0.14, 0.24, i % 2 ? m.paperIn : m.cloth, bx0 + 0.3 + i * 0.32, 0.31, bz - 0.05);
    const coil = new THREE.Mesh(new THREE.TorusGeometry(0.12, 0.025, 5, 12), m.blackIn);
    coil.position.set(bx1 - 0.35, 0.27, bz); coil.rotation.x = Math.PI / 2; s.add(coil);
    // shelf over the left end of the bench
    metreUV(box(s, 0.9, 0.03, 0.26, m.woodIn, x0 + 0.5, 1.48, z0 + 0.13), true);
    for (const x of [x0 + 0.1, x0 + 0.9]) box(s, 0.03, 0.16, 0.2, m.woodIn, x, 1.4, z0 + 0.1);
    for (let i = 0; i < 5; i++) box(s, 0.12, 0.16 + (i % 3) * 0.04, 0.12, i % 2 ? m.paperIn : m.cloth, x0 + 0.16 + i * 0.16, 1.58 + (i % 3) * 0.02, z0 + 0.13);
    cyl(s, 0.06, 0.06, 0.17, m.chrome, x0 + 0.85, 1.58, z0 + 0.13, 10);

    // the tube receiver with its speaker, headphones and lead to the field line
    const rx = new THREE.Vector3(-11.85, top, z0 + 0.3);
    box(s, 0.52, 0.28, 0.34, m.crinkle, rx.x, rx.y + 0.14, rx.z);
    box(s, 0.54, 0.3, 0.02, m.blackIn, rx.x, rx.y + 0.15, rx.z + 0.175);
    const dial = plane(s, 0.3, 0.075, m.dial, rx.x + 0.06, rx.y + 0.2, rx.z + 0.187);
    { const uv = dial.geometry.attributes.uv as THREE.BufferAttribute; for (let i = 0; i < uv.count; i++) uv.setY(i, 0.5 + uv.getY(i) * 0.5); }
    const meter = plane(s, 0.07, 0.07, m.dial, rx.x - 0.17, rx.y + 0.2, rx.z + 0.187);
    { const uv = meter.geometry.attributes.uv as THREE.BufferAttribute; for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * 0.25, uv.getY(i) * 0.5); }
    for (let i = 0; i < 5; i++) {
      const k = cyl(s, 0.022, 0.024, 0.03, m.blackIn, rx.x - 0.2 + i * 0.1, rx.y + 0.08, rx.z + 0.2, 10);
      k.rotation.x = Math.PI / 2;
      cyl(s, 0.006, 0.006, 0.012, m.chrome, rx.x - 0.2 + i * 0.1, rx.y + 0.08, rx.z + 0.217, 6).rotation.x = Math.PI / 2;
    }
    for (const dx of [-0.24, 0.24]) box(s, 0.02, 0.02, 0.05, m.chrome, rx.x + dx, rx.y + 0.27, rx.z + 0.18);
    box(s, 0.24, 0.24, 0.18, m.crinkle, rx.x - 0.44, rx.y + 0.12, rx.z - 0.02);
    cyl(s, 0.085, 0.085, 0.01, m.blackIn, rx.x - 0.44, rx.y + 0.12, rx.z + 0.072, 14).rotation.x = Math.PI / 2;
    for (let i = 0; i < 4; i++) box(s, 0.15, 0.006, 0.004, m.chrome, rx.x - 0.44, rx.y + 0.07 + i * 0.033, rx.z + 0.078);
    // headphones lying in front of the receiver
    const hp = new THREE.Mesh(new THREE.TorusGeometry(0.075, 0.008, 4, 12, Math.PI), m.blackIn);
    hp.position.set(rx.x + 0.32, rx.y + 0.012, rx.z + 0.25); hp.rotation.x = -Math.PI / 2; s.add(hp);
    for (const dx of [-0.075, 0.075]) cyl(s, 0.032, 0.032, 0.025, m.blackIn, rx.x + 0.32 + dx, rx.y + 0.013, rx.z + 0.25, 10);
    rod(s, v(rx.x + 0.2, rx.y + 0.1, rx.z - 0.17), v(rx.x + 0.25, 1.3, z0 + 0.02), 0.006, m.blackIn, 4);
    rod(s, v(rx.x + 0.25, 1.3, z0 + 0.02), v(x1 - 0.02, 1.3, z0 + 0.02), 0.006, m.blackIn, 4);
    this.slot.dial = addFlood(rx.x + this.o.x, rx.y + 0.25 + this.o.y, rx.z + 0.4 + this.o.z, BASE.dial, 0xffb048, hutFlood);
    this.gx.dial = this.glow.add(rx.x + 0.06, rx.y + 0.2, rx.z + 0.2, 0.22, 0xffb050);
    this.proxy('receiver', 0.62, 0.36, 0.42, rx.x, rx.y + 0.16, rx.z + 0.02);

    // the timing log, open, with a pencil
    const lg = new THREE.Group();
    lg.position.set(-11.05, top + 0.004, z0 + 0.46); lg.rotation.y = 0.08;
    box(lg, 0.47, 0.02, 0.33, m.cloth, 0, 0.01, 0);
    for (const sx of [-1, 1]) {
      const p = plane(lg, 0.22, 0.31, m.ledger, sx * 0.112, 0.026, 0, 0, -Math.PI / 2);
      p.rotation.z = sx * 0.035;
      const uv = p.geometry.attributes.uv as THREE.BufferAttribute;
      for (let i = 0; i < uv.count; i++) uv.setX(i, (sx < 0 ? 0 : 0.5) + uv.getX(i) * 0.5);
    }
    rod(lg, v(0.06, 0.032, 0.1), v(0.2, 0.032, 0.04), 0.004, m.woodIn, 5);
    s.add(lg);
    this.proxy('timingLog', 0.52, 0.18, 0.4, -11.05, top + 0.06, z0 + 0.46);

    // desk lamp: a goose-neck with a green enamel shade, aimed at the log
    const dl = v(-10.35, top, z0 + 0.25);
    cyl(s, 0.07, 0.08, 0.025, m.blackIn, dl.x, dl.y + 0.012, dl.z, 12);
    rod(s, v(dl.x, dl.y + 0.02, dl.z), v(dl.x - 0.05, dl.y + 0.3, dl.z + 0.05), 0.008, m.chrome, 5);
    rod(s, v(dl.x - 0.05, dl.y + 0.3, dl.z + 0.05), v(dl.x - 0.2, dl.y + 0.36, dl.z + 0.12), 0.008, m.chrome, 5);
    const dsh = new THREE.Mesh(new THREE.ConeGeometry(0.08, 0.11, 12, 1, true), m.shadeIn);
    dsh.position.set(dl.x - 0.24, dl.y + 0.33, dl.z + 0.14); dsh.rotation.z = 0.5; s.add(dsh);
    const db = new THREE.Mesh(new THREE.SphereGeometry(0.025, 8, 6), m.hutBulb);
    db.position.set(dl.x - 0.25, dl.y + 0.3, dl.z + 0.14); s.add(db);
    this.slot.desk = addFlood(dl.x - 0.4 + this.o.x, dl.y + 0.2 + this.o.y, dl.z + 0.2 + this.o.z, BASE.desk, 0xffd090, hutFlood);
    this.gx.desk = this.glow.add(dl.x - 0.25, dl.y + 0.29, dl.z + 0.14, 0.28, 0xffd59a);

    // the framed 1947 arrangement above the bench
    const fx = -11.15, fy = 1.78, fz = z0 + 0.025;
    plane(s, 0.6, 0.2625, m.diagram, fx, fy, fz + 0.012);
    box(s, 0.68, 0.33, 0.02, m.blackIn, fx, fy, fz);
    for (const [w, h, x, y] of [[0.7, 0.04, 0, 0.165], [0.7, 0.04, 0, -0.165], [0.04, 0.37, -0.33, 0], [0.04, 0.37, 0.33, 0]]) box(s, w, h, 0.03, m.woodIn, fx + x, fy + y, fz + 0.012);
    this.proxy('diagram', 0.74, 0.4, 0.08, fx, fy, fz + 0.03);

    // the field telephone on the west wall: oak box, two bells, crank, the earpiece on its hook
    const ph = v(x0 + 0.08, 1.45, 2.55);
    box(s, 0.16, 0.44, 0.26, m.woodIn, ph.x, ph.y, ph.z);
    box(s, 0.03, 0.03, 0.3, m.woodIn, ph.x + 0.01, ph.y - 0.235, ph.z);
    box(s, 0.2, 0.025, 0.3, m.woodIn, ph.x + 0.03, ph.y - 0.26, ph.z);
    for (const dz of [-0.06, 0.06]) {
      const bell = new THREE.Mesh(new THREE.SphereGeometry(0.042, 10, 6, 0, Math.PI * 2, 0, Math.PI / 2), m.chrome);
      bell.position.set(ph.x + 0.09, ph.y + 0.17, ph.z + dz); bell.rotation.z = -Math.PI / 2; s.add(bell);
    }
    rod(s, v(ph.x + 0.08, ph.y + 0.02, ph.z), v(ph.x + 0.2, ph.y + 0.04, ph.z), 0.012, m.blackIn, 6);
    const mouth = new THREE.Mesh(new THREE.ConeGeometry(0.035, 0.06, 10, 1, true), m.blackIn);
    mouth.position.set(ph.x + 0.23, ph.y + 0.04, ph.z); mouth.rotation.z = Math.PI / 2; s.add(mouth);
    rod(s, v(ph.x + 0.08, ph.y + 0.0, ph.z - 0.14), v(ph.x + 0.12, ph.y + 0.0, ph.z - 0.19), 0.008, m.chrome, 5);
    rod(s, v(ph.x + 0.12, ph.y + 0.0, ph.z - 0.19), v(ph.x + 0.12, ph.y - 0.08, ph.z - 0.19), 0.008, m.chrome, 5);
    box(s, 0.03, 0.03, 0.04, m.blackIn, ph.x + 0.12, ph.y - 0.09, ph.z - 0.19);
    // the earpiece hangs on the hook: its own group, so the chapter can rattle it
    const hs = new THREE.Group();
    hs.position.set(ph.x + 0.1, ph.y + 0.1, ph.z + 0.15);
    cyl(hs, 0.024, 0.02, 0.13, m.blackIn, 0, -0.07, 0, 10);
    cyl(hs, 0.032, 0.032, 0.02, m.blackIn, 0, -0.14, 0, 10);
    box(hs, 0.03, 0.02, 0.04, m.chrome, 0, 0.0, -0.01);
    mergeStatic(hs);
    this.interior.add(hs);
    this.objs.fieldPhoneHandset = hs;
    rod(s, v(ph.x + 0.1, ph.y - 0.15, ph.z + 0.15), v(ph.x + 0.06, ph.y - 0.3, ph.z + 0.1), 0.005, m.blackIn, 4);
    rod(s, v(ph.x + 0.0, ph.y + 0.22, ph.z), v(ph.x - 0.0, HUT.h - 0.02, ph.z), 0.005, m.blackIn, 4);
    this.col(x0, x0 + 0.24, ph.z - 0.2, ph.z + 0.2);
    this.proxy('fieldPhone', 0.3, 0.6, 0.42, ph.x + 0.08, ph.y, ph.z);

    // a steel cabinet in the south-west corner
    const cb = v(x0 + 0.24, 0, 3.9);
    metreUV(box(s, 0.46, 1.78, 0.9, m.cabinet, cb.x, 0.89, cb.z));
    box(s, 0.01, 1.7, 0.012, m.blackIn, cb.x + 0.232, 0.89, cb.z);
    for (const dz of [-0.06, 0.06]) box(s, 0.03, 0.14, 0.02, m.chrome, cb.x + 0.245, 1.0, cb.z + dz);
    box(s, 0.005, 0.06, 0.12, m.paperIn, cb.x + 0.233, 1.45, cb.z - 0.22);
    this.col(x0, x0 + 0.5, cb.z - 0.47, cb.z + 0.47);
    // a chair pulled out from the bench
    const ch = new THREE.Group();
    ch.position.set(-12.0, 0, 3.05); ch.rotation.y = 0.6;
    box(ch, 0.42, 0.04, 0.4, m.woodIn, 0, 0.45, 0);
    box(ch, 0.42, 0.08, 0.03, m.woodIn, 0, 0.82, -0.19);
    box(ch, 0.42, 0.06, 0.03, m.woodIn, 0, 0.62, -0.19);
    for (const [x, z] of [[-0.18, -0.17], [0.18, -0.17]]) box(ch, 0.035, 0.86, 0.035, m.woodIn, x, 0.43, z);
    for (const [x, z] of [[-0.18, 0.17], [0.18, 0.17]]) box(ch, 0.035, 0.45, 0.035, m.woodIn, x, 0.225, z);
    s.add(ch);
    this.col(-12.15, -11.85, 2.9, 3.2);
    // a bare bulb under a tin shade in the middle of the ceiling
    rod(s, v(-11.0, HUT.h, 3.0), v(-11.0, 2.2, 3.0), 0.004, m.blackIn, 4);
    const cs = new THREE.Mesh(new THREE.ConeGeometry(0.16, 0.08, 12, 1, true), m.shadeIn);
    cs.position.set(-11.0, 2.2, 3.0); s.add(cs);
    const cb2 = new THREE.Mesh(new THREE.SphereGeometry(0.035, 8, 6), m.hutBulb);
    cb2.position.set(-11.0, 2.15, 3.0); s.add(cb2);
    this.slot.ceil = addFlood(-11.0 + this.o.x, 2.0 + this.o.y, 3.0 + this.o.z, BASE.ceil, 0xffd9a0, hutFlood);
    this.gx.ceil = this.glow.add(-11.0, 2.14, 3.0, 0.32, 0xffe0b0);
    // the wall lamp outside reaches in through the door a little
    this.slot.door = addFlood(-9.95 + this.o.x, 1.7 + this.o.y, 4.9 + this.o.z, BASE.door, 0xffb966, hutFlood);
  }

  // ---------- generator shed, fuel drum ----------
  private shed() {
    const st = this.st, m = this.m, { x0, x1, z0, z1, h } = SHED;
    metreUV(box(st, x1 - x0 + 0.3, 0.12, z1 - z0 + 0.3, m.concrete, (x0 + x1) / 2, G + 0.03, (z0 + z1) / 2));
    metreUV(box(st, x1 - x0, h, z1 - z0, m.corrugated, (x0 + x1) / 2, G + h / 2, (z0 + z1) / 2));
    const roof = box(st, x1 - x0 + 0.3, 0.05, z1 - z0 + 0.35, m.corrugated, (x0 + x1) / 2, G + h + 0.07, (z0 + z1) / 2);
    roof.rotation.x = -0.08; metreUV(roof);
    // door on the east side, padlocked; louvres on the south side
    box(st, 0.03, 1.75, 0.86, m.rust, x1 + 0.016, G + 0.9, -0.4);
    box(st, 0.04, 0.05, 0.1, m.darkMetal, x1 + 0.04, G + 1.0, -0.78);
    for (let i = 0; i < 6; i++) box(st, 0.8, 0.03, 0.05, m.darkMetal, (x0 + x1) / 2, G + 0.5 + i * 0.07, z1 + 0.03).rotation.x = 0.5;
    const sign = plane(st, 0.4, 0.22, m.atlas, x1 + 0.034, G + 1.55, 0.25, Math.PI / 2);
    regionUV(sign.geometry, 'gen');
    // exhaust pipe through the roof, with a rain cap
    cyl(st, 0.045, 0.045, 1.1, m.darkMetal, EXHAUST.x, G + h + 0.4, EXHAUST.z, 8);
    cyl(st, 0.1, 0.05, 0.08, m.darkMetal, EXHAUST.x, EXHAUST.y - 0.05, EXHAUST.z, 8);
    // a caged bulb over the door
    box(st, 0.08, 0.08, 0.1, m.darkMetal, x1 + 0.05, G + 1.95, -0.4);
    const sb = new THREE.Mesh(new THREE.SphereGeometry(0.04, 8, 6), m.lampBulb);
    sb.position.set(x1 + 0.12, G + 1.9, -0.4); st.add(sb);
    this.slot.shed = addFlood(x1 + 0.4 + this.o.x, G + 1.75 + this.o.y, -0.4 + this.o.z, BASE.shed, 0xffc078, fieldFlood);
    this.gx.shed = this.glow.add(x1 + 0.12, G + 1.9, -0.4, 0.45, 0xffc888);
    // fuel drum, a second one on its side
    cyl(st, 0.29, 0.29, 0.88, m.rust, -16.5, G + 0.44, 1.25, 14);
    for (const y of [0.3, 0.6]) cyl(st, 0.3, 0.3, 0.025, m.darkMetal, -16.5, G + y, 1.25, 14);
    const d2 = cyl(st, 0.29, 0.29, 0.88, m.rust, -17.9, G + 0.29, 0.9, 14); d2.rotation.set(0, 0.4, Math.PI / 2);
    this.col(x0 - 0.05, x1 + 0.05, z0 - 0.05, z1 + 0.05);
    this.col(-16.82, -16.18, 0.93, 1.57);
    this.col(-18.45, -17.35, 0.45, 1.35);
    this.occluder(x1 - x0, h, z1 - z0, (x0 + x1) / 2, G + h / 2, (z0 + z1) / 2);
  }

  // ---------- the transit pier and the instrument ----------
  private transit() {
    const st = this.st, m = this.m;
    const ph = PIER.top - PIER.bottom;
    const g = new THREE.CylinderGeometry(PIER.half1 * Math.SQRT2, PIER.half0 * Math.SQRT2, ph, 4, 1);
    const pier = new THREE.Mesh(faceted(g), m.concrete);
    pier.position.set(T.x, PIER.bottom + ph / 2, T.z); pier.rotation.y = Math.PI / 4;
    st.add(metreUV(pier));
    cyl(st, 0.13, 0.13, 0.012, m.brass, T.x, PIER.top + 0.006, T.z, 16);
    this.col(T.x - 0.48, T.x + 0.48, T.z - 0.48, T.z + 0.48);
    this.occluder(0.8, ph, 0.8, T.x, PIER.bottom + ph / 2, T.z);

    // the 1947 transit, set on the footing of A
    const tr = new THREE.Group();
    tr.position.set(T.x, PIER.top + 0.012, T.z);
    tr.rotation.y = yawTo(T.x, T.z, FOOT_A.x, FOOT_A.z);
    const b = m.brass, k = m.darkMetal;
    cyl(tr, 0.085, 0.095, 0.02, b, 0, 0.01, 0, 16);
    for (let i = 0; i < 4; i++) { const a = i * Math.PI / 2 + Math.PI / 4; cyl(tr, 0.009, 0.009, 0.05, k, Math.cos(a) * 0.07, 0.045, Math.sin(a) * 0.07, 6); }
    cyl(tr, 0.075, 0.075, 0.015, b, 0, 0.075, 0, 16);
    cyl(tr, 0.03, 0.04, 0.03, b, 0, 0.098, 0, 10);
    cyl(tr, 0.09, 0.09, 0.018, b, 0, 0.12, 0, 20);
    cyl(tr, 0.066, 0.066, 0.03, k, 0, 0.144, 0, 16);
    cyl(tr, 0.06, 0.06, 0.003, m.paneIn, 0, 0.16, 0, 16);
    box(tr, 0.004, 0.003, 0.1, m.galv, 0, 0.162, 0).rotation.y = 0.3;
    for (const x of [-0.065, 0.065]) {
      const sd = box(tr, 0.014, 0.17, 0.06, k, x, 0.24, 0);
      sd.rotation.z = x > 0 ? -0.08 : 0.08;
    }
    cyl(tr, 0.009, 0.009, 0.15, k, 0, 0.31, 0, 8).rotation.z = Math.PI / 2;
    const tel = new THREE.Group();
    tel.position.set(0, 0.31, 0);
    tel.rotation.x = Math.atan2(G + 0.15 - (PIER.top + 0.33), T.distanceTo(FOOT_A));
    cyl(tel, 0.022, 0.022, 0.27, k, 0, 0, 0, 12).rotation.x = Math.PI / 2;
    cyl(tel, 0.027, 0.025, 0.05, k, 0, 0, -0.15, 12).rotation.x = Math.PI / 2;
    cyl(tel, 0.025, 0.025, 0.003, m.paneIn, 0, 0, -0.176, 12).rotation.x = Math.PI / 2;
    cyl(tel, 0.012, 0.015, 0.05, k, 0, 0, 0.16, 10).rotation.x = Math.PI / 2;
    cyl(tel, 0.009, 0.009, 0.09, m.paneIn, 0, -0.035, 0.02, 6).rotation.x = Math.PI / 2;
    cyl(tel, 0.015, 0.015, 0.02, b, 0.03, 0.0, 0.05, 8).rotation.z = Math.PI / 2;
    tr.add(tel);
    const arc = new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.06, 0.006, 16, 1, false, 0, Math.PI), b);
    arc.rotation.z = Math.PI / 2; arc.position.set(0.078, 0.31, 0); tr.add(arc);
    st.add(tr);
    // the record box on the pier's west face: a steel frame with the field card behind glass
    const tilt = Math.atan2(PIER.half0 - PIER.half1, ph);
    const yc = 0.72, half = PIER.half0 - (yc - PIER.bottom) / ph * (PIER.half0 - PIER.half1);
    const fr = box(st, 0.024, 0.32, 0.26, m.steel, T.x - half - 0.012, yc, T.z);
    fr.rotation.z = -tilt;
    const card = plane(st, 0.2, 0.267, m.atlas, T.x - half - 0.026, yc, T.z, -Math.PI / 2, -tilt);
    regionUV(card.geometry, 'e09a');
    this.proxy('transit', 1.0, 1.55, 1.0, T.x, 0.65 + 0.2, T.z);
  }

  // ---------- fixed points A and B, the C stakes ----------
  private markers() {
    const st = this.st, m = this.m;
    // A's footing: empty, the socket open, its brass disc still set in the concrete
    const fa = FOOT_A;
    const foot = box(st, 0.5, 0.26, 0.5, m.concrete, fa.x, G + 0.02, fa.z);
    foot.rotation.y = 0.15; metreUV(foot);
    box(st, 0.12, 0.01, 0.12, m.darkMetal, fa.x, G + 0.152, fa.z).rotation.y = 0.15;
    box(st, 0.14, 0.012, 0.025, m.rust, fa.x, G + 0.153, fa.z - 0.07).rotation.y = 0.15;
    for (const [dx, dz] of [[-0.17, -0.17], [0.17, -0.17], [-0.17, 0.17], [0.17, 0.17]]) cyl(st, 0.012, 0.012, 0.03, m.rust, fa.x + dx, G + 0.16, fa.z + dz, 6);
    const disc = new THREE.Mesh(regionUV(new THREE.CircleGeometry(0.05, 20), 'disc'), m.atlas);
    disc.position.set(fa.x + 0.12, G + 0.152, fa.z + 0.13); disc.rotation.set(-Math.PI / 2, 0, 0.4);
    st.add(disc);
    this.col(fa.x - 0.32, fa.x + 0.32, fa.z - 0.32, fa.z + 0.32);
    this.proxy('footing', 0.75, 0.4, 0.75, fa.x, G + 0.15, fa.z);

    // A's post, now standing off the line in a pile of stones, the plate facing the transit
    const pa = POST_A, ya = yawTo(pa.x, pa.z, T.x, T.z) + Math.PI;
    const postA = new THREE.Group();
    postA.position.set(pa.x, G - 0.3, pa.z); postA.rotation.set(0.03, ya, -0.04, 'YXZ');
    metreUV(box(postA, 0.1, 1.65, 0.1, m.wood, 0, 0.825, 0));
    const pp = plane(postA, 0.32, 0.32, m.atlas, 0, 1.45, 0.057);
    regionUV(pp.geometry, 'markA');
    box(postA, 0.34, 0.34, 0.01, m.darkMetal, 0, 1.45, 0.051);
    st.add(postA);
    this.col(pa.x - 0.22, pa.x + 0.22, pa.z - 0.22, pa.z + 0.22);
    this.proxy('markerA', 0.6, 1.7, 0.6, pa.x, 0.65, pa.z);

    // B: the comparison vane, still in its footing, the vane plate facing the transit
    const pb = POST_B, yb = yawTo(pb.x, pb.z, T.x, T.z) + Math.PI;
    metreUV(box(st, 0.42, 0.22, 0.42, m.concrete, pb.x, G + 0.02, pb.z));
    const bp = box(st, 0.1, 1.55, 0.1, m.wood, pb.x, G + 0.1 + 0.775, pb.z);
    bp.rotation.y = yb; metreUV(bp);
    const vane = new THREE.Group();
    vane.position.set(pb.x, 1.05, pb.z); vane.rotation.y = yb;
    const vp = plane(vane, 0.12, 0.48, m.atlas, 0, 0, 0.058);
    regionUV(vp.geometry, 'vane');
    box(vane, 0.13, 0.5, 0.012, m.darkMetal, 0, 0, 0.05);
    st.add(vane);
    this.col(pb.x - 0.25, pb.x + 0.25, pb.z - 0.25, pb.z + 0.25);
    this.proxy('markerB', 0.6, 1.7, 0.6, pb.x, 0.65, pb.z);

    // C: a row of old stakes east from the transit, out past the fence into the dark
    const r = rng(47);
    STAKES.forEach((x, i) => {
      const z = T.z + (r() - 0.5) * 0.16, gy = this.y(x, z);
      const hgt = i === 5 ? 0.22 : 0.55 + r() * 0.35;
      const s = box(st, 0.06, hgt + 0.25, 0.06, m.wood, x, gy - 0.25 + (hgt + 0.25) / 2, z);
      s.rotation.set((r() - 0.5) * 0.35, r() * 0.6, (r() - 0.5) * 0.35); metreUV(s);
      if (i !== 5) {
        const p = box(st, 0.064, 0.1, 0.064, m.fadedPaint, 0, 0, 0);
        p.position.copy(new THREE.Vector3(0, (hgt + 0.25) / 2 - 0.07, 0).applyEuler(s.rotation).add(s.position));
        p.rotation.copy(s.rotation);
      }
      if (x < FENCE.x1 - 0.5) this.col(x - 0.06, x + 0.06, z - 0.06, z + 0.06);
    });
    // one has fallen and lies in the dirt
    const fallen = box(st, 0.05, 0.05, 0.62, m.wood, 41.3, G + 0.025, T.z + 0.35); fallen.rotation.y = 0.5;
    this.proxy('stakesC', 22, 0.9, 0.9, (STAKES[0] + 29.5) / 2 - 0.3, G + 0.42, T.z);
  }

  // ---------- the field lamp with its hinged tin hood (P07) ----------
  private lamp() {
    const st = this.st, m = this.m, L = LAMP;
    const collar = new THREE.Vector3(L.x, 1.0, L.z);
    for (let i = 0; i < 3; i++) {
      const a = i / 3 * Math.PI * 2 + 0.5;
      rod(st, collar, new THREE.Vector3(L.x + Math.cos(a) * 0.4, G, L.z + Math.sin(a) * 0.4), 0.014, m.darkMetal, 5);
    }
    cyl(st, 0.03, 0.03, 0.06, m.darkMetal, L.x, 1.0, L.z, 8);
    rod(st, collar, new THREE.Vector3(L.x, 1.8, L.z), 0.016, m.galv, 6);
    // battery box at the foot, and its lead up the pole
    box(st, 0.32, 0.22, 0.2, m.paint, L.x - 0.32, G + 0.11, L.z + 0.36);
    box(st, 0.06, 0.04, 0.04, m.darkMetal, L.x - 0.38, G + 0.24, L.z + 0.36);
    rod(st, new THREE.Vector3(L.x - 0.3, G + 0.22, L.z + 0.33), new THREE.Vector3(L.x - 0.05, 0.6, L.z + 0.05), 0.006, m.rubber, 4);
    rod(st, new THREE.Vector3(L.x - 0.05, 0.6, L.z + 0.05), new THREE.Vector3(L.x - 0.02, 1.7, L.z + 0.02), 0.006, m.rubber, 4);
    // the head, aimed at the transit
    const head = new THREE.Group();
    head.position.set(L.x, 1.86, L.z);
    head.lookAt(T.x, PIER.top + 0.3, T.z);
    const shell = new THREE.Mesh(new THREE.CylinderGeometry(0.13, 0.085, 0.2, 14, 1, true), m.shadeOut);
    shell.rotation.x = Math.PI / 2; head.add(shell);
    cyl(head, 0.085, 0.085, 0.02, m.darkMetal, 0, 0, -0.1, 14).rotation.x = Math.PI / 2;
    const glass = cyl(head, 0.125, 0.125, 0.006, m.fieldBulb, 0, 0, 0.098, 16); glass.rotation.x = Math.PI / 2;
    for (const sx of [-1, 1]) rod(head, new THREE.Vector3(sx * 0.14, 0, 0), new THREE.Vector3(sx * 0.02, -0.12, 0), 0.01, m.darkMetal, 4);
    // the hood: a tin can lid on a hinge at the top of the rim
    this.hood = new THREE.Group();
    this.hood.position.set(0, 0.135, 0.105);
    const lid = new THREE.Mesh(new THREE.CylinderGeometry(0.142, 0.142, 0.05, 16, 1, false), m.galv);
    lid.rotation.x = Math.PI / 2; lid.position.set(0, -0.135, 0.022);
    this.hood.add(lid);
    box(this.hood, 0.06, 0.02, 0.02, m.darkMetal, 0, 0, 0);
    box(this.hood, 0.03, 0.04, 0.012, m.darkMetal, 0, -0.27, 0.05);
    this.hood.rotation.x = HOOD_OPEN;
    mergeStatic(this.hood);
    head.add(noMerge(this.hood));
    st.add(head);   // the rest of the head merges with the static outside
    this.objs.lampHead = head;
    this.objs.lampHood = this.hood;
    const fwd = new THREE.Vector3(0, 0, 1).applyQuaternion(head.quaternion);
    const lp = head.position.clone().addScaledVector(fwd, 0.3);
    this.slot.field = addFlood(lp.x + this.o.x, lp.y + this.o.y, lp.z + this.o.z, BASE.field, 0xffc27a, fieldFlood);
    const gp = head.position.clone().addScaledVector(fwd, 0.11);
    this.gx.field = this.glow.add(gp.x, gp.y, gp.z, 0.6, 0xffd29a);
    this.col(L.x - 0.5, L.x + 0.32, L.z - 0.32, L.z + 0.48);
    this.proxy('lamp', 0.75, 2.1, 0.75, L.x, 0.9, L.z);
  }

  // ---------- the cable (P08) ----------
  private cable() {
    const R = CABLE_R, gy = G + R;
    const P = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
    // a loose loop lying on the ground: the cable circles once while going on along `dir`
    const loop = (start: THREE.Vector3, dir: THREE.Vector3, len: number, rad: number) => {
      const out: THREE.Vector3[] = [];
      const side = new THREE.Vector3(-dir.z, 0, dir.x);
      for (let i = 1; i < 28; i++) {
        const s = i / 28, a = s * Math.PI * 2;
        const u = len * s + rad * Math.sin(a), w = rad * (1 - Math.cos(a));
        // the second pass lies over the first where they cross
        const lift = 2.1 * R * Math.exp(-Math.pow((s - 0.84) / 0.06, 2));
        out.push(start.clone().addScaledVector(dir, u).addScaledVector(side, w).setY(gy + lift));
      }
      return out;
    };
    const d1 = new THREE.Vector3(1, 0, 0.12).normalize(), d2 = new THREE.Vector3(0.6, 0, -0.8).normalize();
    const wallFace = HUT.z1 + HUT.t;
    const hutSide = [
      P(-9.1, 0.02, wallFace + 0.06), P(-9.08, R, 4.96), P(-9.03, -0.05, 5.1), P(-8.9, gy, 5.32),
      P(-8.2, gy, 5.75), P(-7.2, gy, 5.95), P(-6.3, gy, 5.72),
      ...loop(P(-6.0, gy, 5.65), d1, 1.3, 0.55),
      P(-4.6, gy, 5.85), P(-3.8, gy, 5.6), P(-2.6, gy, 5.0), P(-1.4, gy, 4.4), P(-0.4, gy, 3.6), P(0.4, gy, 2.9),
      ...loop(P(0.6, gy, 2.7), d2, 1.2, 0.5),
      P(1.42, gy, 1.68), P(2.1, gy, 0.9), P(2.75, gy, 0.1), P(3.05, gy, -0.62),
      CUT.clone().addScaledVector(CUT_DIR, -0.36), CUT.clone().addScaledVector(CUT_DIR, -0.05),
    ];
    const face = (y: number) => T.z + PIER.half0 - (y - PIER.bottom) / (PIER.top - PIER.bottom) * (PIER.half0 - PIER.half1) + R + 0.004;
    const pierSide = [
      CUT.clone().addScaledVector(CUT_DIR, 0.05), CUT.clone().addScaledVector(CUT_DIR, 0.36),
      P(3.22, gy, -2.2), P(3.15, gy, -3.0), P(3.1, gy + 0.01, face(gy) + 0.06),
      P(3.1, 0.12, face(0.12)), P(3.1, 0.55, face(0.55)), P(3.1, 0.98, face(0.98)),
      P(3.1, PIER.top + R, T.z + PIER.half1 - 0.02), P(3.08, PIER.top + R, T.z + 0.15),
    ];
    const tube = (pts: THREE.Vector3[], radial: number) => {
      const curve = new THREE.CatmullRomCurve3(pts, false, 'centripetal');
      const n = Math.ceil(curve.getLength() / 0.06);
      return new THREE.Mesh(new THREE.TubeGeometry(curve, n, R, radial, false), this.m.rubber);
    };
    this.st.add(tube(hutSide, 8), tube(pierSide, 8));
    // the cut faces, a hand's width apart and facing each other
    for (const [p, n] of [[CUT.clone().addScaledVector(CUT_DIR, -0.05), CUT_DIR], [CUT.clone().addScaledVector(CUT_DIR, 0.05), CUT_DIR.clone().negate()]] as const) {
      const f = new THREE.Mesh(new THREE.CircleGeometry(R * 1.01, 16), this.m.cutFace);
      f.position.copy(p).addScaledVector(n, 0.0005);
      f.quaternion.setFromUnitVectors(new THREE.Vector3(0, 0, 1), n);
      this.st.add(f);
    }
    // clips on the pier, the terminal box on its top, the elbow at the junction box
    for (const y of [0.3, 0.75]) box(this.st, 0.07, 0.025, 0.02, this.m.galv, 3.1, y, face(y) + R * 0.6);
    box(this.st, 0.08, 0.05, 0.06, this.m.brass, 3.08, PIER.top + 0.025, T.z + 0.13);
    this.proxy('cable', 0.8, 0.32, 0.8, CUT.x, G + 0.08, CUT.z);
  }

  // ---------- rocks and plants ----------
  private scatter() {
    const kinds: Record<Thing['kind'], { geo: THREE.BufferGeometry; mat: THREE.Material; tones: [THREE.Color, THREE.Color] }> = {
      block: { geo: rockGeo(3, 'block'), mat: this.lit({ color: 0xffffff, roughness: 0.95 }, 0.03), tones: [new THREE.Color(0x8a7a68), new THREE.Color(0x5f5650)] },
      slab: { geo: rockGeo(8, 'slab'), mat: this.lit({ color: 0xffffff, roughness: 0.95 }, 0.03), tones: [new THREE.Color(0x9a8670), new THREE.Color(0x6a5a4c)] },
      creo: { geo: creosoteGeo(), mat: this.lit({ color: 0xffffff, roughness: 1 }, 0.03), tones: [new THREE.Color(0x5a5c40), new THREE.Color(0x45482f)] },
      yucca: { geo: bladeGeo(41, 24, 0.42, 0.82, 0.03, [0.45, 1.3]), mat: this.lit({ color: 0xffffff, roughness: 0.8, side: THREE.DoubleSide }, 0.03), tones: [new THREE.Color(0x5a6a4a), new THREE.Color(0x46523c)] },
      grass: { geo: bladeGeo(57, 9, 0.22, 0.42, 0.012, [0.9, 1.4]), mat: this.lit({ color: 0xffffff, roughness: 1, side: THREE.DoubleSide }, 0.03), tones: [new THREE.Color(0x8a7a52), new THREE.Color(0x6a6048)] },
    };
    const mtx = new THREE.Matrix4(), q = new THREE.Quaternion(), e = new THREE.Euler(), sc = new THREE.Vector3(), p = new THREE.Vector3(), c = new THREE.Color();
    for (const kind of Object.keys(kinds) as Thing['kind'][]) {
      const list = this.things.filter((t) => t.kind === kind);
      if (!list.length) continue;
      const k = kinds[kind];
      const im = new THREE.InstancedMesh(k.geo, k.mat, list.length);
      list.forEach((t, i) => {
        const gy = this.y(t.x, t.z);
        const rock = kind === 'block' || kind === 'slab';
        p.set(t.x, gy + (rock ? t.s * 0.12 : -0.02), t.z);
        e.set(t.tilt, t.ry, rock ? t.tilt * 0.7 : t.tilt * 0.3);
        q.setFromEuler(e);
        sc.set(t.s, t.s * (rock ? 0.85 + t.tone * 0.3 : 0.9 + t.tone * 0.25), t.s * (rock ? 0.9 : 1));
        im.setMatrixAt(i, mtx.compose(p, q, sc));
        c.copy(k.tones[0]).lerp(k.tones[1], t.tone);
        im.setColorAt(i, c);
        const inside = t.x > FENCE.x0 && t.x < FENCE.x1 && t.z > FENCE.z0 && t.z < FENCE.z1;
        const big = rock ? t.s > 0.3 : kind === 'creo' ? t.s > 0.55 : kind === 'yucca' ? t.s > 0.6 : false;
        if (inside && big) { const rr = t.s * (kind === 'slab' ? 0.95 : kind === 'yucca' ? 0.45 : 0.62); this.col(t.x - rr, t.x + rr, t.z - rr, t.z + rr); }
      });
      im.instanceMatrix.needsUpdate = true;
      if (im.instanceColor) im.instanceColor.needsUpdate = true;
      im.computeBoundingSphere();
      im.name = 'scatter-' + kind;
      this.group.add(im);
    }
  }

  // ---------- exhaust from the generator, moths at the wall lamp ----------
  private fx() {
    const pos: number[] = [], seed: number[] = [], kind: number[] = [], r = rng(91);
    for (let i = 0; i < 12; i++) { pos.push(EXHAUST.x, EXHAUST.y, EXHAUST.z); seed.push(i / 12 + r() * 0.03); kind.push(0); }
    for (let i = 0; i < 9; i++) { pos.push(WALL_LAMP.x, WALL_LAMP.y - 0.05, WALL_LAMP.z + 0.12); seed.push(r()); kind.push(1); }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setAttribute('aSeed', new THREE.Float32BufferAttribute(seed, 1));
    g.setAttribute('aKind', new THREE.Float32BufferAttribute(kind, 1));
    const mat = this.track(new THREE.ShaderMaterial({
      uniforms: { uTime: this.fxTime, uPx: glowScale },
      transparent: true, depthWrite: false, fog: false,
      vertexShader: /* glsl */`
        attribute float aSeed; attribute float aKind;
        uniform float uTime, uPx; varying float vA; varying float vKind;
        void main() {
          vec3 p = position; float size;
          if (aKind < 0.5) {
            float life = fract(uTime * 0.26 + aSeed);
            p += vec3(0.4 * life + 0.06 * sin(aSeed * 40.0 + uTime), 1.5 * life, -0.3 * life + 0.05 * cos(aSeed * 31.0 + uTime * 0.7));
            size = 0.14 + 0.8 * life;
            vA = smoothstep(0.0, 0.12, life) * (1.0 - life) * 0.3;
          } else {
            float a = uTime * (2.0 + aSeed * 2.5) + aSeed * 6.283;
            float rad = 0.18 + 0.12 * sin(uTime * 1.3 + aSeed * 11.0);
            p += vec3(cos(a) * rad, 0.09 * sin(uTime * 2.9 + aSeed * 17.0), sin(a * 1.13) * rad * 0.8);
            size = 0.02; vA = 0.9;
          }
          vKind = aKind;
          vec4 mv = modelViewMatrix * vec4(p, 1.0);
          gl_Position = projectionMatrix * mv;
          gl_PointSize = max(1.5, size * uPx / max(-mv.z, 0.1));
        }`,
      fragmentShader: /* glsl */`
        varying float vA; varying float vKind;
        void main() {
          float d = length(gl_PointCoord - 0.5);
          float a = vKind < 0.5 ? smoothstep(0.5, 0.0, d) * vA : smoothstep(0.5, 0.15, d) * vA;
          vec3 col = vKind < 0.5 ? vec3(0.2, 0.19, 0.18) : vec3(1.0, 0.85, 0.6);
          gl_FragColor = vec4(col, a);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }`,
    }));
    const pts = new THREE.Points(g, mat);
    pts.frustumCulled = false;
    pts.renderOrder = 3;
    pts.name = 'stationFx';
    this.group.add(pts);
    this.objs.fx = pts;
    // wires go in here too: they are complete once fence() and power() have run
    const lg = new THREE.BufferGeometry();
    lg.setAttribute('position', new THREE.Float32BufferAttribute(this.wires, 3));
    lg.setAttribute('color', new THREE.Float32BufferAttribute(this.wireCol, 3));
    const lines = new THREE.LineSegments(lg, this.track(new THREE.LineBasicMaterial({ vertexColors: true })));
    lines.name = 'wires';
    this.group.add(lines);
  }

  // ---------- where the player can walk ----------
  // The fenced field is one zone; the hut's walls and the things in it are colliders. The
  // pad outside the gate is a second zone, joined by the walk-through beside the gate.
  // Zones overlap by more than the player's width where they meet.
  private walkable() {
    this.addZone('field', FENCE.x0 + 0.3, FENCE.x1 - 0.3, FENCE.z0 + 0.3, FENCE.z1 - 0.3);
    this.addZone('walkThrough', GAP_W + 0.12, GATE.w - 0.15, FENCE.z1 - 1.4, FENCE.z1 + 1.6);
    this.addZone('arrival', -28.0, -12.5, FENCE.z1 + 0.3, 39.0);
  }

  // A cold sky light and a low moon. SARO keeps its own in the control room group, which
  // the chapter hides while the player is out here.
  private nightLights() {
    const group = new THREE.Group();
    const hemi = new THREE.HemisphereLight(NIGHT.sky, NIGHT.ground, NIGHT.hemi);
    const moon = new THREE.DirectionalLight(NIGHT.moon, NIGHT.moonI);
    moon.position.copy(NIGHT.moonAt);
    moon.target.position.set(0, 0, 0);
    group.add(hemi, moon, moon.target);
    this.group.add(group);
    return { group, hemi, moon };
  }
  /** Out on the track in the truck, the station's bright moonlight gives way to the road's
   *  (RoadArea.ts), so that nothing changes where the truck goes from one area into the
   *  other (drive/legs.ts): 1 at the station, 0 at the change. */
  nightBlend(w: number) {
    const { hemi, moon } = this.lights, a = ROAD_NIGHT, b = NIGHT;
    hemi.color.setHex(a.sky).lerp(_c.setHex(b.sky), w);
    hemi.groundColor.setHex(a.ground).lerp(_c.setHex(b.ground), w);
    hemi.intensity = a.hemi + (b.hemi - a.hemi) * w;
    moon.color.setHex(a.moon).lerp(_c.setHex(b.moon), w);
    moon.intensity = a.moonI + (b.moonI - a.moonI) * w;
    moon.position.copy(a.moonAt).lerp(b.moonAt, w);
  }

  // ---------- switches ----------
  // P07: the tin hood over the field lamp. Covered, the lamp gives no light at all.
  setLampCovered(on: boolean) {
    this.lampCovered = on;
    this.hood.rotation.x = on ? 0 : HOOD_OPEN;
    setFlood(this.slot.field, on ? 0 : BASE.field, fieldFlood);
    this.m.fieldBulb.color.set(on ? 0x151311 : 0xfff2d8);
    this.setGlow(this.gx.field, 0xffd29a, on ? 0 : 1);
  }
  // The field camera's flash for one exposure: a white flood at the camera, in slot 0 (kept
  // for the truck's headlights, which are off while the truck is parked here).
  setFlash(p: THREE.Vector3 | null, w = 16) {
    if (p) { fieldFlood.pos[0].set(p.x, p.y, p.z, w); fieldFlood.col[0].set(0xfff6ec); }
    else fieldFlood.pos[0].w = 0;
  }
  setReceiver(on: boolean) {
    this.receiverOn = on;
    this.m.dial.color.set(on ? 0xffffff : 0x141210);
    setFlood(this.slot.dial, on ? BASE.dial : 0, hutFlood);
    this.setGlow(this.gx.dial, 0xffb050, on ? 1 : 0);
  }
  setHutLight(on: boolean) {
    this.hutLightOn = on;
    this.m.hutBulb.color.set(on ? 0xfff0d0 : 0x22201c);
    setFlood(this.slot.ceil, on ? BASE.ceil : 0, hutFlood);
    setFlood(this.slot.desk, on ? BASE.desk : 0, hutFlood);
    setFlood(this.slot.spill, on ? BASE.spill : 0, fieldFlood);
    this.m.paneOut.color.set(on ? 0xffb56b : 0x0b0e14);
    this.m.paneOut.opacity = on ? 0.5 : 0.7;
    this.setGlow(this.gx.ceil, 0xffe0b0, on ? 1 : 0);
    this.setGlow(this.gx.desk, 0xffd59a, on ? 1 : 0);
    this.setGlow(this.gx.winS, 0x6a4018, on ? 1 : 0);
    this.setGlow(this.gx.winE, 0x6a4018, on ? 1 : 0);
  }

  update(_dt: number, t: number) {
    this.glow.update(t);
    this.fxTime.value = t;
    // the bulb over the door is loose in its socket: now and then it dips
    const dip = Math.sin(t * 0.83) > 0.93 && Math.sin(t * 31 + Math.sin(t * 5)) > -0.2;
    const k = dip ? 0.3 + 0.25 * Math.abs(Math.sin(t * 47)) : 0.95 + 0.05 * Math.sin(t * 13.7) * Math.sin(t * 4.1);
    if (Math.abs(k - this.wallK) > 0.01) {
      this.wallK = k;
      setFlood(this.slot.wall, BASE.wall * k, fieldFlood);
      setFlood(this.slot.door, BASE.door * k, hutFlood);
      this.m.wallBulb.color.setRGB(k, k * 0.94, k * 0.84);
      this.setGlow(this.gx.wall, 0xffc27a, k);
    }
  }

  dispose() {
    this.group.parent?.remove(this.group);
    const geos = new Set<THREE.BufferGeometry>();
    this.group.traverse((o) => {
      const m = o as THREE.Mesh;
      if (m.geometry) geos.add(m.geometry);
      if (m.material) for (const mm of ([] as THREE.Material[]).concat(m.material)) this.mats.add(mm);
      if ((o as THREE.InstancedMesh).isInstancedMesh) (o as THREE.InstancedMesh).dispose();
    });
    geos.forEach((g) => g.dispose());
    this.mats.forEach((m) => eachVariant(m, (v) => v.dispose()));
    this.texs.forEach((t) => t.dispose());
    for (const set of [fieldFlood, hutFlood] as FloodSet[]) for (let i = set === fieldFlood ? 2 : 0; i < set.n; i++) set.pos[i].w = 0;
    fieldFlood.count = 2; hutFlood.count = 0;
  }
}
