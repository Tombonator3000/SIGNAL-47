import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, addFlood } from '../world/kit';
import { rng } from '../core/textures';
import { GATE, track, trackHalf, trackNearest, surfaceY, hash } from './roadTerrain';
import { SIGNS, horizonGlowTex, uvInto } from './roadTextures';
import { type Build, roadFlood, colored, instances, place, circle, wall } from './roadProps';

// Places off the road: the station gate at the end of the track, a ranch far to the east,
// the desert plants and rocks, the mesas on the horizon and SARO's lights behind the start.

// ---------- STATION 01: chain-link compound, closed gate, a dim lamp, a hut and a mast ----------
// Only what is seen from the gate at night. The station itself is its own area (World.ts).
export function station(b: Build) {
  const s = b.statics, m = b.m;
  const X0 = GATE.x, X1 = GATE.x - 40, Z0 = GATE.z - 20, Z1 = GATE.z + 20, y = (x: number, z: number) => surfaceY(x, z);
  const fence = (ax: number, az: number, bx: number, bz: number) => {
    const len = Math.hypot(bx - ax, bz - az), g = new THREE.PlaneGeometry(len, 2.1);
    const uv = g.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * len / 0.7, uv.getY(i) * 3);
    const p = new THREE.Mesh(g, m.fence);
    p.position.set((ax + bx) / 2, y((ax + bx) / 2, (az + bz) / 2) + 1.05, (az + bz) / 2);
    p.rotation.y = Math.atan2(-(bz - az), bx - ax);
    s.add(p);
    for (let i = 0; i <= Math.ceil(len / 3); i++) {
      const t = i / Math.ceil(len / 3), px = ax + (bx - ax) * t, pz = az + (bz - az) * t;
      cyl(s, 0.035, 0.035, 2.3, m.steel, px, y(px, pz) + 1.1, pz, 6);
    }
    rod(s, new THREE.Vector3(ax, y(ax, az) + 2.12, az), new THREE.Vector3(bx, y(bx, bz) + 2.12, bz), 0.025, m.steel, 5);
  };
  fence(X0, Z0, X0, GATE.z - GATE.half); fence(X0, GATE.z + GATE.half, X0, Z1);
  fence(X0, Z1, X1, Z1); fence(X1, Z1, X1, Z0); fence(X1, Z0, X0, Z0);
  // the double gate: pipe frames with chain link, a chain where the leaves meet
  const gy = y(X0, GATE.z);
  for (const d of [-1, 1]) {
    const za = GATE.z + d * GATE.half, zb = GATE.z + d * 0.05;
    cyl(s, 0.07, 0.07, 2.5, m.steel, X0, y(X0, za) + 1.2, za, 8);
    for (const h of [0.15, 1.95]) rod(s, new THREE.Vector3(X0, gy + h, za), new THREE.Vector3(X0, gy + h, zb), 0.03, m.steel, 6);
    rod(s, new THREE.Vector3(X0, gy + 0.15, zb), new THREE.Vector3(X0, gy + 1.95, zb), 0.03, m.steel, 6);
    const leaf = new THREE.Mesh(new THREE.PlaneGeometry(GATE.half - 0.1, 1.8), m.fence);
    leaf.position.set(X0, gy + 1.05, (za + zb) / 2); leaf.rotation.y = Math.PI / 2;
    const uv = leaf.geometry.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * 7, uv.getY(i) * 2.6);
    s.add(leaf);
  }
  box(s, 0.06, 0.25, 0.12, m.dark, X0 + 0.04, gy + 1.0, GATE.z);
  const plate = new THREE.Mesh(uvInto(new THREE.PlaneGeometry(0.6, 0.3), SIGNS.gate), m.sign);
  plate.position.set(X0 + 0.04, gy + 1.45, GATE.z + 1.6); plate.rotation.y = Math.PI / 2; s.add(plate);
  wall(b, X0 - 0.2, X0 + 0.2, Z0, Z1);
  wall(b, X1, X0, Z0 - 0.2, Z0 + 0.2); wall(b, X1, X0, Z1 - 0.2, Z1 + 0.2);
  // a bare bulb on a wooden pole by the gate: flood slot 4
  const lx = X0 + 1.2, lz = GATE.z - GATE.half - 1.0, ly = y(lx, lz);
  cyl(s, 0.09, 0.11, 4.4, m.wood, lx, ly + 2.1, lz, 7);
  box(s, 0.5, 0.06, 0.06, m.wood, lx - 0.2, ly + 4.0, lz);
  box(s, 0.16, 0.1, 0.16, m.steel, lx - 0.42, ly + 3.92, lz);
  addFlood(lx - 0.42, ly + 3.6, lz, 9, 0xffc27a, roadFlood);
  b.glow.add(lx - 0.42, ly + 3.82, lz, 0.5, 0xffd49a); b.glow.add(lx - 0.42, ly + 3.82, lz, 3.2, 0x40301c);
  circle(b, lx, lz, 0.2);
  // the 1947 hut and the survey mast behind the fence
  const hx = X0 - 24, hz = GATE.z + 6, hy = y(hx, hz);
  box(s, 6, 3, 4.4, m.concrete, hx, hy + 1.3, hz);
  box(s, 6.6, 0.25, 5, m.concrete, hx, hy + 2.9, hz);
  box(s, 0.05, 2.0, 0.95, m.dark, hx + 3.02, hy + 1.0, hz - 0.8);
  box(s, 0.05, 0.6, 0.9, m.dark, hx + 3.02, hy + 1.8, hz + 1.1);
  const mx = X0 - 12, mz = GATE.z - 10, my = y(mx, mz);
  const legs = [0, 1, 2].map((k) => new THREE.Vector2(Math.cos(k * 2.094) * 1.1, Math.sin(k * 2.094) * 1.1));
  for (let k = 0; k < 3; k++) {
    const a = legs[k], c = legs[(k + 1) % 3];
    rod(s, new THREE.Vector3(mx + a.x, my, mz + a.y), new THREE.Vector3(mx + a.x * 0.25, my + 16, mz + a.y * 0.25), 0.05, m.steel, 4);
    for (let h = 0; h < 16; h += 2.6) {
      const f0 = 1 - h / 16 * 0.75, f1 = 1 - (h + 2.6) / 16 * 0.75;
      rod(s, new THREE.Vector3(mx + a.x * f0, my + h, mz + a.y * f0), new THREE.Vector3(mx + c.x * f1, my + h + 2.6, mz + c.y * f1), 0.02, m.steel, 3);
    }
  }
}

// ---------- a ranch 400 m east: house, barn, windmill, tank, and its yard light (slot 3) ----------
export function ranch(b: Build) {
  const s = b.statics, m = b.m, y = (x: number, z: number) => surfaceY(x, z);
  const X = 440, Z = 196;
  const shed = (x: number, z: number, w: number, d: number, h: number, wallM: THREE.Material) => {
    const g = new THREE.Group(); g.position.set(x, y(x, z), z); g.rotation.y = 0.12;
    box(g, w, h, d, wallM, 0, h / 2 - 0.2, 0);
    for (const sgn of [-1, 1]) { const r = box(g, w + 0.6, 0.12, d / 2 + 0.6, m.steel, 0, h + 0.55, sgn * d / 4.6); r.rotation.x = sgn * 0.42; }
    s.add(g);
  };
  shed(X - 22, Z - 4, 12, 8, 3, m.paint);
  shed(X + 6, Z - 26, 16, 11, 5.5, m.wood);
  cyl(s, 3, 3, 2.4, m.steel, X + 22, y(X + 22, Z + 4) + 1.0, Z + 4, 14);
  const wx = X + 16, wz = Z + 18, wy = y(wx, wz);
  for (const [dx, dz] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) rod(s, new THREE.Vector3(wx + dx * 1.1, wy, wz + dz * 1.1), new THREE.Vector3(wx + dx * 0.2, wy + 9, wz + dz * 0.2), 0.05, m.steel, 4);
  const wheel = cyl(s, 1.3, 1.3, 0.06, m.steel, wx, wy + 9.3, wz - 0.4, 14); wheel.rotation.x = Math.PI / 2;
  box(s, 0.06, 0.6, 1.6, m.steel, wx, wy + 9.3, wz + 0.8);
  // yard light: mercury vapour, a cold blue-white
  const lx = X - 8, lz = Z + 8, ly = y(lx, lz);
  cyl(s, 0.12, 0.15, 8.2, m.wood, lx, ly + 3.9, lz, 7);
  box(s, 0.9, 0.08, 0.08, m.steel, lx + 0.45, ly + 7.9, lz);
  addFlood(lx + 0.9, ly + 7.4, lz, 22, 0xc8e4ff, roadFlood);
  b.glow.add(lx + 0.9, ly + 7.75, lz, 1.3, 0xdcefff); b.glow.add(lx + 0.9, ly + 7.75, lz, 9, 0x1c2a36);
  b.glow.add(X - 22 + 6.05, y(X - 16, Z - 4) + 1.4, Z - 3, 0.9, 0xffb866); // a lit window
}

// ---------- creosote, rocks and yucca, thick where the headlights pass ----------
export function plants(b: Build) {
  const r = rng(77);
  const bush: THREE.Matrix4[] = [], rock: THREE.Matrix4[] = [], yucca: THREE.Matrix4[] = [];
  const tints: THREE.Color[] = [];
  const free = (x: number, z: number) => {
    if (Math.abs(x) < 9.5) return false;
    const t = trackNearest(x, z);
    if (t.d < trackHalf(t.s) + 1.8) return false;
    if (x > -672 && x < -598 && z > 500 && z < 566) return false;   // station
    if (x > 395 && x < 485 && z > 150 && z < 235) return false;     // ranch yard
    return !(x > -26 && x < -14 && z > 510 && z < 530);             // cattle guard
  };
  for (let n = 0; n < 26000 && bush.length < 1600; n++) {
    const x = -820 + r() * 1300, z = -420 + r() * 1600;
    const d = Math.min(z > -320 && z < 1150 ? Math.abs(x) : 1e9, trackNearest(x, z).d);
    if (r() > (d < 45 ? 1 : d < 160 ? 0.32 : 0.1) || !free(x, z)) continue;
    const k = r(), y = surfaceY(x, z);
    if (k < 0.72) {
      const sc = 0.45 + r() * 1.0;
      bush.push(place(x, y - 0.08 * sc, z, r() * 6, sc, 0, sc * (0.7 + r() * 0.5)));
      tints.push(new THREE.Color().setHSL(0.15 + r() * 0.06, 0.25 + r() * 0.15, 0.22 + r() * 0.12));
    } else if (k < 0.96) {
      const sc = r() < 0.85 ? 0.15 + r() * 0.45 : 0.6 + r() * 0.8;
      rock.push(place(x, y - 0.12 * sc, z, r() * 6, sc, (r() - 0.5) * 0.5));
      if (sc > 0.7 && d < 40) circle(b, x, z, 0.4 * sc);
    } else yucca.push(place(x, y - 0.05, z, r() * 6, 0.8 + r() * 0.5));
  }
  const lobe = (rad: number, x: number, y: number, z: number) => new THREE.IcosahedronGeometry(rad, 0).scale(1, 0.6, 1).translate(x, y, z);
  const bushes = instances(mergeGeometries([lobe(0.6, 0, 0.25, 0), lobe(0.42, 0.38, 0.2, 0.22), lobe(0.36, -0.3, 0.18, -0.25)])!, b.m.bush, bush);
  tints.forEach((c, i) => bushes.setColorAt(i, c));
  b.root.add(bushes);
  b.root.add(instances(new THREE.DodecahedronGeometry(0.5, 0).scale(1, 0.62, 0.85).translate(0, 0.2, 0), b.m.rock, rock));
  const leaf = [0.28, 0.36, 0.24], dry = [0.42, 0.34, 0.24];
  const parts = [colored(new THREE.CylinderGeometry(0.07, 0.09, 0.35, 5).translate(0, 0.17, 0), dry),
    colored(new THREE.CylinderGeometry(0.012, 0.02, 1.7, 4).translate(0, 1.15, 0), dry)];
  for (let i = 0; i < 12; i++) {
    const a = i / 12 * Math.PI * 2, tilt = 0.35 + (i % 3) * 0.3;
    parts.push(colored(new THREE.ConeGeometry(0.035, 0.75, 3).translate(0, 0.37, 0).rotateZ(tilt).rotateY(a).translate(0, 0.32, 0), leaf));
  }
  b.root.add(instances(mergeGeometries(parts)!, b.m.vc, yucca));
}

// ---------- mesas on the horizon: two dark rings of flat tops and cliffs, no fog ----------
const MESAS: [number, number, number][] = [[18, 9, 150], [57, 5, 95], [96, 12, 175], [148, 6, 115], [201, 14, 155], [246, 4, 85], [273, 10, 195], [314, 7, 125], [346, 5, 100]];
export function mesas() {
  const pos: number[] = [], col: number[] = [];
  const ring = (R: number, scale: number, lift: number, c0: number[], c1: number[], seed: number) => {
    const N = 720;
    const h = (deg: number) => {
      let v = 18 + 9 * Math.sin(deg * 0.052 + seed) + 6 * Math.sin(deg * 0.13 + seed * 2);
      for (const [a, w, top] of MESAS) {
        const d = Math.abs(((deg - a - seed * 9 + 540) % 360) - 180);
        v = Math.max(v, top * scale * (1 - THREE.MathUtils.smoothstep(d, w, w + 1.6)) * (0.96 + 0.04 * Math.sin(deg * 3.1)));
      }
      return v + lift;
    };
    for (let i = 0; i < N; i++) {
      const a0 = i / N * 360, a1 = (i + 1) / N * 360;
      const p = (deg: number, y: number) => [-250 + Math.sin(deg * Math.PI / 180) * R, y, 300 - Math.cos(deg * Math.PI / 180) * R];
      const q = [p(a0, -40), p(a1, -40), p(a1, h(a1)), p(a0, h(a0))];
      for (const k of [0, 1, 2, 0, 2, 3]) { pos.push(...q[k]); col.push(...(k < 2 ? c0 : c1)); }
    }
  };
  ring(3900, 0.8, 10, [0.006, 0.008, 0.016], [0.016, 0.019, 0.032], 3);
  ring(3200, 1, 0, [0.003, 0.004, 0.009], [0.009, 0.011, 0.02], 0);
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  const mesh = new THREE.Mesh(g, new THREE.MeshBasicMaterial({ vertexColors: true, fog: false, side: THREE.DoubleSide }));
  mesh.frustumCulled = false;
  return mesh;
}

// ---------- SARO behind the start: its lamps and beacons as they sit on the site ----------
// Positions follow Exterior.ts (site x, z), moved so the site road lies on this highway:
// local = site + (24, 0.6, -300). The array is north of the control room, the motel west.
const DISHES = [[-48, -66], [-16, -52], [9.5, -42], [6, -88], [44, -96], [-30, -116], [-80, -104], [20, -146], [66, -156], [-58, -168], [-10, -200], [100, -120], [-120, -150], [40, -230], [-96, -230],
  [130, -210], [-150, -270], [80, -290], [-40, -300], [170, -260], [-200, -210], [10, -360], [120, -350], [-120, -340], [-230, -300], [210, -330], [-60, -420]];
const LAMPS = [[30, -30], [-10, -42], [-40, -56], [12, -78], [48, -86], [-70, -94], [-24, -106], [60, -146], [-52, -158], [-4, -190], [94, -110]];
export function saroLights(b: Build) {
  const L = (x: number, y: number, z: number, size: number, c: number, blink = 0, ph = 0) => b.glow.add(x + 24, y + 0.6, z - 300, size, c, blink, ph);
  LAMPS.forEach(([x, z]) => L(x, 6.5, z, 7, 0xffb45a));
  for (let z = 70; z > -200; z -= 34) L(-20.1, 7.1, z, 7, 0xffaf55);
  DISHES.forEach(([x, z], i) => L(x + 3, 16.4, z - 3, 5.5, 0xff3c28, 1.6, hash(i, 3) * 6));
  [[-6.4, 2.6, 2], [20.9, 2.8, 5.8], [30.4, 2.8, 5.8], [-6.4, 2.6, -3]].forEach(([x, y, z]) => L(x, y, z, 4, 0xffbe6e));
  L(-30.4, 5.8, 30.2, 12, 0xff4a35); L(-29, 4, 32, 5, 0x50f0d8);
  for (let z = 26; z <= 66; z += 5) L(-42.6, 2.2, z, 2.6, 0xffc070);
  // far ranches and a town glow to the south east, to fill the night a little
  b.glow.add(1300, 4, 1700, 7, 0xffd0a0); b.glow.add(-1500, 4, 2100, 6, 0xffc890); b.glow.add(900, 4, -1600, 6, 0xd0e4ff);
  // a warm haze over the site, low on the northern horizon
  const haze = new THREE.Mesh(new THREE.PlaneGeometry(1800, 300), new THREE.MeshBasicMaterial({
    map: horizonGlowTex(), color: new THREE.Color(0.42, 0.22, 0.09), transparent: true, blending: THREE.AdditiveBlending, depthWrite: false, fog: false,
  }));
  haze.position.set(24, 110, -1500);
  haze.renderOrder = 1;
  return haze;
}
// The survey track's route as points, for path following (preview and tests) and maps.
export const trackRoute = () => track.pts.filter((_, i) => i % 4 === 0).map((p) => new THREE.Vector2(p.x, p.y));
