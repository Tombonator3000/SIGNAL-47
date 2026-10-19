import * as THREE from 'three';
import { rng } from '../core/textures';
import { fromStation, stationLand, stationToRoad } from './geo';

// The near skyline of the night, one for the whole map (geo.ts): the mesas near STATION 01.
// Every area that the truck is driven through draws them, each in its own terms, so they stay
// where they are when the truck goes from one area into the next. Unlit and unfogged, very
// dark: silhouettes against the night sky. Behind them, all round, the painted horizon
// (backdrop.ts, round 16) has the far mesas, buttes and ridge that two rings of mesas here and
// a ring of ridges round the station used to be (19 October).

// ---------- the haze of the dawn on them ----------
// Unfogged, the far mesas stay black when the land in front of them goes into the grey of the
// dawn haze, and the far land shows as a pale band under them. At dawn they take some of the
// haze too (World.update sets it from the sky each frame; none at night).
// As much as the scene's own fog would give at that distance (density), times k.
export const horizonHaze = { color: { value: new THREE.Color() }, k: { value: 0 }, density: { value: 0.0021 } };
export function hazed<M extends THREE.Material>(m: M): M {
  const prev = m.onBeforeCompile;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.uniforms.uHazeCol = horizonHaze.color; sh.uniforms.uHazeK = horizonHaze.k; sh.uniforms.uHazeDen = horizonHaze.density;
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nvarying float vHzD;')
      .replace('#include <project_vertex>', '#include <project_vertex>\n  vHzD = -mvPosition.z;');
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nuniform vec3 uHazeCol; uniform float uHazeK; uniform float uHazeDen; varying float vHzD;')
      .replace('#include <opaque_fragment>', '#include <opaque_fragment>\n  gl_FragColor.rgb = mix(gl_FragColor.rgb, uHazeCol, uHazeK * (1.0 - exp(-uHazeDen * uHazeDen * vHzD * vHzD)));');
  };
  m.customProgramCacheKey = () => 'horizon-haze';
  return m;
}

// ---------- near STATION 01: mesas, like the concept art ----------
// A little lighter on the cliffs that face the moon, darker on the slopes and the tops.
// Station-local terms; ground: the height under them (the station's land, which out there is
// the road's, geo.ts).
export function stationMesas(ground: (x: number, z: number) => number) {
  const pos: number[] = [], col: number[] = [];
  const R0 = rng(5);
  const tri = (a: THREE.Vector3, b: THREE.Vector3, c: THREE.Vector3, ca: number[], cb: number[], cc: number[]) => {
    pos.push(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z); col.push(...ca, ...cb, ...cc);
  };
  const moon = new THREE.Vector3(-0.6, 0, 0.8).normalize();
  const quad = (a: THREE.Vector3, b: THREE.Vector3, c: THREE.Vector3, d: THREE.Vector3, lo: number[], hi: number[], from: THREE.Vector3) => {
    const n = new THREE.Vector3().subVectors(b, a).cross(new THREE.Vector3().subVectors(d, a)).normalize();
    const mid = a.clone().add(b).add(c).add(d).multiplyScalar(0.25);
    if (n.dot(mid.sub(from)) < 0) n.negate();
    const k = (0.7 + 0.75 * Math.max(0, n.dot(moon))) * (0.85 + R0() * 0.3);
    const L = lo.map((v) => v * k), H = hi.map((v) => v * k);
    tri(a, b, c, L, L, H); tri(a, c, d, L, H, H);
  };
  const mesa = (bearing: number, dist: number, len: number, depth: number, height: number, seed: number, tiers: number) => {
    const b = bearing * Math.PI / 180, r = rng(seed);
    const cx = Math.sin(b) * dist, cz = -Math.cos(b) * dist;
    const tx = Math.cos(b), tz = Math.sin(b), rx = Math.sin(b), rz = -Math.cos(b);
    const centre = new THREE.Vector3(cx, height * 0.5, cz);
    const N = 34;
    const outline: [number, number][] = [];
    for (let i = 0; i < N; i++) {
      const a = i / N * Math.PI * 2;
      let k = 0.88 + r() * 0.22;
      if (r() < 0.14) k *= 0.74;   // a side canyon
      outline.push([Math.cos(a) * len / 2 * k, Math.sin(a) * depth / 2 * k]);
    }
    const ring = (scale: number, yAt: (x: number, z: number, i: number) => number, shift = 0) => outline.map(([u, v], i) => {
      const x = cx + (u * scale + shift) * tx + v * scale * rx, z = cz + (u * scale + shift) * tz + v * scale * rz;
      return new THREE.Vector3(x, yAt(x, z, i), z);
    });
    const rim0 = outline.map(() => height * (0.97 + r() * 0.04));
    const foot = ring(1.32, (x, z) => ground(x, z) - 6);
    const shoulder = ring(1.09, (x, z) => Math.max(ground(x, z) + 4, height * 0.36));
    const ledge = ring(1.03, () => height * 0.66);
    const ledgeIn = ring(1.0, () => height * 0.67);
    const rim = ring(0.97, (_x, _z, i) => rim0[i]);
    const talus = [0.009, 0.0085, 0.011], cliffLo = [0.011, 0.01, 0.013], cliffHi = [0.026, 0.021, 0.022], cap = [0.0075, 0.0075, 0.0105];
    for (let i = 0; i < N; i++) {
      const j = (i + 1) % N;
      quad(foot[i], foot[j], shoulder[j], shoulder[i], talus, talus, centre);
      quad(shoulder[i], shoulder[j], ledge[j], ledge[i], cliffLo, cliffHi.map((v) => v * 0.75), centre);
      quad(ledge[i], ledge[j], ledgeIn[j], ledgeIn[i], cap, cap, centre);
      quad(ledgeIn[i], ledgeIn[j], rim[j], rim[i], cliffLo, cliffHi, centre);
    }
    let top = rim, topY = height;
    if (tiers > 1) {
      const shift = (r() - 0.5) * len * 0.2, h2 = height * (1.22 + r() * 0.12);
      const base2 = ring(0.6, () => height, shift), rim2 = ring(0.54, () => h2, shift);
      const c0 = new THREE.Vector3(cx, height, cz);
      for (let i = 0; i < N; i++) { const j = (i + 1) % N; tri(c0, rim[j], rim[i], cap, cap, cap); }
      for (let i = 0; i < N; i++) { const j = (i + 1) % N; quad(base2[i], base2[j], rim2[j], rim2[i], cliffLo, cliffHi, centre); }
      top = rim2; topY = h2;
    }
    const c1 = top.reduce((sum, v) => sum.add(v), new THREE.Vector3()).multiplyScalar(1 / N).setY(topY);
    for (let i = 0; i < N; i++) { const j = (i + 1) % N; tri(c1, top[j], top[i], cap, cap, cap); }
  };
  mesa(28, 1100, 950, 320, 118, 3, 2);
  mesa(62, 1500, 520, 260, 82, 5, 1);
  mesa(84, 900, 150, 120, 66, 7, 1);
  mesa(-38, 1450, 700, 300, 86, 9, 2);
  // (turned from -95 so its east end stops short of the highway at Caliche, 180 m west of the town)
  mesa(-85, 1750, 900, 300, 48, 11, 1);
  mesa(150, 2000, 700, 300, 40, 13, 1);
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  return g;
}
/** A relay tower's red light to the south-east, in front of the painted horizon's mesas there, ranch lights west (station-local). */
export const STATION_FAR_LIGHTS: { x: number; y: number; z: number; size: number; color: number; blink?: number; phase?: number }[] = [
  { x: 1500, y: 52, z: 1650, size: 14, color: 0xff3020, blink: 1.4, phase: 0.3 }, { x: -2100, y: 22, z: 600, size: 9, color: 0xffc070 },
  { x: -1900, y: 18, z: 1100, size: 7, color: 0xffb060 },
];

/** The near skyline as SARO or the road sees it, and its far lights (for the area's glow). */
export function sharedHorizon(area: 'saro' | 'road', glow: { add: (x: number, y: number, z: number, size: number, color: number, blink?: number, phase?: number) => unknown }) {
  const group = new THREE.Group();
  group.name = 'horizon';
  // on the station's land out there (the road's own, geo.ts), so the very same mesas as the station's
  const st = new THREE.Mesh(stationMesas((x, z) => stationLand(...stationToRoad(x, z))), hazed(new THREE.MeshBasicMaterial({ vertexColors: true, fog: false, side: THREE.DoubleSide })));
  st.name = 'mesas:near';
  st.frustumCulled = false;
  fromStation(st, area);
  st.updateMatrix();
  group.add(st);
  const v = new THREE.Vector3();
  for (const l of STATION_FAR_LIGHTS) { v.set(l.x, l.y, l.z).applyMatrix4(st.matrix); glow.add(v.x, v.y, v.z, l.size, l.color, l.blink, l.phase); }
  return group;
}
