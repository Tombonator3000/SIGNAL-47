import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, floodlit, floodSet } from '../world/kit';
import type { GlowPoints } from '../world/glow';
import { chainlink } from '../core/textures';
import { artTexture } from '../core/art';
import type { Obstacle } from './Drive';
import { HWY, GUARD, PASTURE, DRIVE_N, DRIVE_S, surfaceY, hash } from './roadTerrain';
import { highwayTex, gravelTex, signAtlas, SIGNS, uvInto } from './roadTextures';
import type { Rect } from './truckTextures';

// The fake floods of the drive. Slots 0 to 2 belong to the truck's headlights; the road
// area fills them with dark placeholders first, so the ranch light gets 3 and the gate 4.
export const roadFlood = floodSet(8, 'road', 0.1);
export const HEADLIGHT_SLOTS = [0, 1, 2];

// Retroreflective sheeting: road signs shine back at the headlights from far away. The
// beam comes from the truck (RoadArea.update reads it from the headlight floods).
export const retroBeam = { pos: { value: new THREE.Vector4(0, -999, 0, 0) }, dir: { value: new THREE.Vector3(0, 0, -1) } };
function retro<T extends THREE.MeshStandardMaterial>(m: T, falloff: number): T {
  floodlit(m, falloff, roadFlood);
  const flood = m.onBeforeCompile;
  m.onBeforeCompile = (sh, r) => {
    flood.call(m, sh, r);
    sh.uniforms.uRetroPos = retroBeam.pos; sh.uniforms.uRetroDir = retroBeam.dir;
    sh.fragmentShader = sh.fragmentShader
      .replace('uniform float uFloodScale;', 'uniform float uFloodScale; uniform vec4 uRetroPos; uniform vec3 uRetroDir;')
      .replace('totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;', `totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;
        vec3 rL = uRetroPos.xyz - vFWorld; float rD = length(rL); rL /= max(rD, 1e-3);
        float rFace = max(dot(normalize(vFNormal), rL), 0.0);
        float rBeam = smoothstep(0.74, 0.97, dot(-rL, uRetroDir));
        totalEmissiveRadiance += diffuseColor.rgb * uRetroPos.w * rFace * rBeam / (1.0 + rD * rD * 0.00012);`);
  };
  m.customProgramCacheKey = () => 'flood' + roadFlood.key + falloff + 'retro';
  return m;
}

export function roadMaterials() {
  const std = (o: THREE.MeshStandardMaterialParameters, fall = 0.012) => floodlit(new THREE.MeshStandardMaterial(o), fall, roadFlood);
  // road surfaces lie a few centimetres over the verges: pull them forward in depth
  const top = { polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -2 };
  const link = chainlink(); link.repeat.set(1, 1);
  return {
    ground: std({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe8dcc8, roughness: 1 }),
    asphalt: std({ map: highwayTex(), roughness: 0.62, ...top }, 0.01),
    gravel: std({ map: gravelTex(), color: 0xf2ece0, roughness: 0.95, ...top }, 0.01),
    steel: std({ color: 0x8b9094, roughness: 0.45, metalness: 0.4 }),
    wood: std({ color: 0x6e5d4b, roughness: 0.92 }),
    concrete: std({ map: artTexture('concrete'), color: 0xbab2a2, roughness: 0.95 }),
    dark: std({ color: 0x0b0b0c, roughness: 0.9, ...top }),
    paint: std({ color: 0x8a6a4e, roughness: 0.9 }),
    sign: retro(new THREE.MeshStandardMaterial({ map: signAtlas(), roughness: 0.55 }), 0.004),
    fence: std({ map: link, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x9a9a92, roughness: 0.6 }),
    vc: std({ vertexColors: true, roughness: 0.9 }),
    bush: std({ color: 0x4b4a2f, roughness: 1, flatShading: true }),
    rock: std({ color: 0x847a6c, roughness: 0.95, flatShading: true }),
    wire: std({ color: 0x55585a, roughness: 0.6, metalness: 0.3 }),
  };
}
export type RoadMats = ReturnType<typeof roadMaterials>;
export type Build = { root: THREE.Group; statics: THREE.Group; glow: GlowPoints; m: RoadMats; obstacles: Obstacle[]; wires: number[] };

// ---------- helpers ----------
export function colored(g: THREE.BufferGeometry, c: number[]) {
  const n = g.attributes.position.count, a = new Float32Array(n * 3);
  for (let i = 0; i < n; i++) a.set(c, i * 3);
  g.setAttribute('color', new THREE.BufferAttribute(a, 3));
  return g;
}
export function instances(geo: THREE.BufferGeometry, mat: THREE.Material, list: THREE.Matrix4[]) {
  const m = new THREE.InstancedMesh(geo, mat, list.length);
  list.forEach((x, i) => m.setMatrixAt(i, x));
  m.computeBoundingSphere();
  return m;
}
const _q = new THREE.Quaternion(), _e = new THREE.Euler();
export const place = (x: number, y: number, z: number, ry = 0, s = 1, lean = 0, sy = s) =>
  new THREE.Matrix4().compose(new THREE.Vector3(x, y, z), _q.setFromEuler(_e.set(lean, ry, lean * 0.6, 'YXZ')), new THREE.Vector3(s, sy, s));
export const circle = (b: Build, x: number, z: number, r: number) => b.obstacles.push({ kind: 'circle', x, z, r });
export const wall = (b: Build, minX: number, maxX: number, minZ: number, maxZ: number) => b.obstacles.push({ kind: 'box', minX, maxX, minZ, maxZ });
// a hanging wire as line segments with a little sag
function span(b: Build, a: THREE.Vector3, c: THREE.Vector3, sag: number, n: number) {
  for (let i = 0; i < n; i++) {
    const t0 = i / n, t1 = (i + 1) / n;
    for (const t of [t0, t1]) b.wires.push(a.x + (c.x - a.x) * t, a.y + (c.y - a.y) * t - sag * 4 * t * (1 - t), a.z + (c.z - a.z) * t);
  }
}
export function wireMesh(b: Build) {
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(b.wires, 3));
  // a standard material needs normals, also on lines: straight up, lit from the sky
  g.setAttribute('normal', new THREE.Float32BufferAttribute(b.wires.map((_, i) => (i % 3 === 1 ? 1 : 0)), 3));
  const l = new THREE.LineSegments(g, b.m.wire);
  l.frustumCulled = false;
  return l;
}
const inDrive = (x: number, z: number) => Math.abs(x) < HWY.fence && z > DRIVE_N - 5 && z < DRIVE_S + 5;

// ---------- telephone line along the east side, and a branch to the ranch ----------
export function poles(b: Build) {
  const wood = [0.3, 0.24, 0.17], glassC = [0.32, 0.48, 0.4];
  const parts = [colored(new THREE.CylinderGeometry(0.11, 0.15, 9.8, 6).translate(0, 4.7, 0), wood),
    colored(new THREE.BoxGeometry(2.4, 0.1, 0.1).translate(0, 8.7, 0), wood)];
  for (const s of [-1, 1]) parts.push(colored(new THREE.BoxGeometry(0.05, 0.75, 0.05).rotateZ(s * 0.75).translate(s * 0.26, 8.4, 0), wood));
  const INS = [-1.05, -0.6, 0.6, 1.05];
  for (const x of INS) parts.push(colored(new THREE.CylinderGeometry(0.035, 0.05, 0.13, 6).translate(x, 8.82, 0), glassC));
  const list: THREE.Matrix4[] = [];
  const line = (pts: [number, number][], ry: number) => {
    let prev: THREE.Vector3[] | null = null;
    for (const [x, z] of pts) {
      const m = place(x, surfaceY(x, z), z, ry + (hash(x, z) - 0.5) * 0.06, 1, (hash(z, x) - 0.5) * 0.03);
      list.push(m);
      const tops = INS.map((ix) => new THREE.Vector3(ix, 8.9, 0).applyMatrix4(m));
      if (prev) tops.forEach((t, k) => span(b, prev![k], t, 0.45, 8));
      prev = tops;
      if (inDrive(x, z)) circle(b, x, z, 0.3);
    }
  };
  const main: [number, number][] = [];
  for (let z = -1900; z <= 2100; z += 45) main.push([15, z + (hash(z, 1) - 0.5) * 4]);
  line(main, 0);
  const branch: [number, number][] = [];
  for (let x = 75; x <= 435; x += 60) branch.push([x, 190 + (x - 15) * 0.012]);
  line(branch, Math.PI / 2);
  b.root.add(instances(mergeGeometries(parts)!, b.m.vc, list));
}

// ---------- barbed wire fences: steel T-posts, a wooden post every tenth, four strands ----------
export function fences(b: Build) {
  const tpost: THREE.Matrix4[] = [], wpost: THREE.Matrix4[] = [];
  const line = (ax: number, az: number, bx: number, bz: number, gap?: [number, number]) => {
    const len = Math.hypot(bx - ax, bz - az), n = Math.ceil(len / 4.5);
    let prev: THREE.Vector3 | null = null;
    for (let i = 0; i <= n; i++) {
      const x = ax + (bx - ax) * i / n, z = az + (bz - az) * i / n;
      if (gap && z > gap[0] && z < gap[1]) { prev = null; continue; }
      const y = surfaceY(x, z), lean = (hash(x * 3, z) - 0.5) * 0.06;
      (i % 10 === 0 || i === n ? wpost : tpost).push(place(x, y, z, hash(z, x) * 3, 1, lean));
      const p = new THREE.Vector3(x, y, z);
      if (prev) for (const h of [0.45, 0.7, 0.95, 1.2]) span(b, prev.clone().setY(prev.y + h), p.clone().setY(y + h), 0.04, 1);
      prev = p;
    }
  };
  line(HWY.fence, -1000, HWY.fence, 1700);
  line(-HWY.fence, -1000, -HWY.fence, 1700, [GUARD.z0 - 0.6, GUARD.z1 + 0.6]);
  line(-HWY.fence - 0.3, PASTURE.north, PASTURE.west, PASTURE.north);
  line(-HWY.fence - 0.3, PASTURE.south, PASTURE.west, PASTURE.south);
  line(PASTURE.west, PASTURE.north, PASTURE.west, PASTURE.south);
  b.root.add(instances(colored(new THREE.BoxGeometry(0.05, 1.45, 0.05).translate(0, 0.6, 0), [0.16, 0.2, 0.16]), b.m.vc, tpost));
  b.root.add(instances(colored(new THREE.CylinderGeometry(0.07, 0.085, 1.7, 6).translate(0, 0.7, 0), [0.36, 0.32, 0.27]), b.m.vc, wpost));
  // the fences hold the truck, and invisible ends close the highway north and south
  const t = 0.15, F = HWY.fence;
  wall(b, F - t, F + t, DRIVE_N, DRIVE_S);
  wall(b, -F - t, -F + t, DRIVE_N, GUARD.z0 - 0.6); wall(b, -F - t, -F + t, GUARD.z1 + 0.6, DRIVE_S);
  wall(b, PASTURE.west, -F, PASTURE.north - t, PASTURE.north + t);
  wall(b, PASTURE.west, -F, PASTURE.south - t, PASTURE.south + t);
  wall(b, PASTURE.west - t, PASTURE.west + t, PASTURE.north, PASTURE.south);
  wall(b, -F, F, DRIVE_N - 1, DRIVE_N); wall(b, -F, F, DRIVE_S, DRIVE_S + 1);
}

// ---------- delineator posts: the reflectors glint in the headlights (RoadArea.update) ----------
export function delineators(b: Build) {
  const list: THREE.Matrix4[] = [], glints: { i: number; p: THREE.Vector3 }[] = [];
  for (const side of [-1, 1]) for (let z = -1000 + (side > 0 ? 40 : 0); z <= 1600; z += 80) {
    if (side < 0 && z > 470 && z < 570) continue; // the mouth of the survey track
    const x = side * 7.2, y = surfaceY(x, z);
    list.push(place(x, y, z, 0, 1, (hash(x, z) - 0.5) * 0.05));
    const p = new THREE.Vector3(x, y + 1.05, z);
    glints.push({ i: b.glow.add(p.x, p.y, p.z, 0.5, 0), p });
  }
  const geo = mergeGeometries([colored(new THREE.BoxGeometry(0.07, 1.3, 0.025).translate(0, 0.55, 0), [0.62, 0.63, 0.62]),
    colored(new THREE.BoxGeometry(0.075, 0.16, 0.035).translate(0, 1.05, 0), [0.85, 0.85, 0.8])])!;
  b.root.add(instances(geo, b.m.vc, list));
  return glints;
}

// ---------- signs: faces from the atlas, steel backs and posts merged ----------
function sign(b: Build, x: number, z: number, ry: number, w: number, h: number, bottom: number, r: Rect, posts: number[], post: THREE.Material, solid = true) {
  const g = new THREE.Group();
  const y0 = surfaceY(x, z);
  g.position.set(x, y0, z); g.rotation.y = ry;
  const face = new THREE.Mesh(uvInto(new THREE.PlaneGeometry(w, h), r), b.m.sign);
  face.position.set(0, bottom + h / 2, 0.012); g.add(face);
  box(g, w, h, 0.02, b.m.steel, 0, bottom + h / 2, 0);
  for (const px of posts) cyl(g, 0.045, 0.045, bottom + h + 0.2, post, px, (bottom + h) / 2 - 0.1, -0.03, 6);
  b.statics.add(g);
  if (solid) for (const px of posts) circle(b, x + Math.cos(ry) * px, z - Math.sin(ry) * px, 0.15);
}
export function signs(b: Build) {
  sign(b, -7.6, 60, Math.PI, 0.76, 0.95, 1.5, SIGNS.speed, [0], b.m.steel);
  sign(b, -7.4, 210, Math.PI, 0.3, 0.6, 1.0, SIGNS.mile, [0], b.m.steel, false);
  sign(b, -9.2, 330, Math.PI, 2.6, 0.98, 1.8, SIGNS.roswell, [-0.9, 0.9], b.m.steel);
  // the turn-off: old enamel on two wooden posts, angled to the traffic from the north
  sign(b, -12.5, 511.5, 2.6, 1.8, 1.13, 1.0, SIGNS.station, [-0.65, 0.65], b.m.wood);
}

// ---------- the cattle guard in the right-of-way fence, with heavy gate posts ----------
export function cattleGuard(b: Build) {
  const { x0, x1, z0, z1 } = GUARD, cx = (x0 + x1) / 2, cz = (z0 + z1) / 2, y = surfaceY(cx, cz);
  const s = b.statics, m = b.m;
  box(s, x1 - x0, 0.06, z1 - z0, m.dark, cx, y - 0.01, cz);                      // the pit under the rails
  for (let x = x0 + 0.1; x < x1; x += 0.2) rod(s, new THREE.Vector3(x, y + 0.06, z0), new THREE.Vector3(x, y + 0.06, z1), 0.05, m.steel, 6);
  for (const z of [z0 - 0.15, z1 + 0.15]) box(s, x1 - x0 + 0.4, 0.3, 0.3, m.concrete, cx, y + 0.05, z);
  for (const [z, d] of [[z0 - 0.3, -1], [z1 + 0.3, 1]] as const) {
    // wing rails along both sides of the guard, then the big posts in the fence line
    for (const h of [0.55, 0.95]) rod(s, new THREE.Vector3(x0 - 0.1, y + h, z), new THREE.Vector3(x1 + 0.1, y + h, z), 0.035, m.steel, 6);
    for (const x of [x0 - 0.1, x1 + 0.1]) cyl(s, 0.04, 0.04, 1.05, m.steel, x, y + 0.5, z, 6);
    wall(b, x0 - 0.2, x1 + 0.2, Math.min(z, z + d * 0.25), Math.max(z, z + d * 0.25));
    const pz = z + d * 0.35, px = -HWY.fence;
    cyl(s, 0.13, 0.15, 2.2, m.wood, px, surfaceY(px, pz) + 0.9, pz, 7);
    cyl(s, 0.1, 0.12, 1.9, m.wood, px, surfaceY(px, pz + d * 2.6) + 0.75, pz + d * 2.6, 7);
    rod(s, new THREE.Vector3(px, y + 1.25, pz), new THREE.Vector3(px, y + 1.25, pz + d * 2.6), 0.05, m.wood, 5);
    rod(s, new THREE.Vector3(px, y + 0.2, pz), new THREE.Vector3(px, y + 1.4, pz + d * 2.6), 0.012, m.wire, 4);
    circle(b, px, pz, 0.2);
  }
}
