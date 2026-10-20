import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, floodlit, floodSet, type FloodSet } from '../world/kit';
import type { GlowPoints } from '../world/glow';
import { desertGround } from '../world/flora';
import { chainlink } from '../core/textures';
import { artTexture } from '../core/art';
import type { Obstacle } from './Drive';
import { HWY, GUARD, PASTURE, DRIVE_N, DRIVE_S, DINER, MOTEL, TOWN, GROUND_TINT, GRAVEL_TINT, surfaceY, hash, smooth, edgeColour, ranchZ, RANCH_TO, onRanchRoad } from './roadTerrain';
import { highwayTex, gravelTex, signAtlas, SIGNS, uvInto } from './roadTextures';
import { wet, HWY_LANES, type Wear } from './roadSurface';
import { rng } from '../core/textures';
import { MILE } from './roadShape';
import type { Rect } from './truckTextures';

// The fake floods of the drive. Slots 0 to 2 belong to the truck's headlights; the road
// area fills them with dark placeholders first, so the ranch light gets 3 and the gate 4.
export const roadFlood = floodSet(11, 'road', 0.1);
export const HEADLIGHT_SLOTS = [0, 1, 2];

// Retroreflective sheeting: road signs shine back at the headlights from far away. The
// beam comes from the truck (RoadArea.update reads it from the headlight floods).
export type RetroBeam = { pos: { value: THREE.Vector4 }; dir: { value: THREE.Vector3 } };
export const retroBeam: RetroBeam = { pos: { value: new THREE.Vector4(0, -999, 0, 0) }, dir: { value: new THREE.Vector3(0, 0, -1) } };
export type Glint = { i: number; p: THREE.Vector3 };
const _src = new THREE.Vector3(), _dir = new THREE.Vector3();
/** Every frame on a road: the beam from the truck's headlights (three flood slots in a set, the
 *  first and last pools) into retroBeam, and the delineators' reflectors (glow points, world
 *  positions) bright inside it, facing the truck. RoadArea and SARO's stretch (World.ts). */
export function headlightGlints(set: FloodSet, slots: readonly number[], glow: GlowPoints, glints: readonly Glint[], beam: RetroBeam = retroBeam) {
  const f0 = set.pos[slots[0]], f2 = set.pos[slots[2]];
  const on = f0.w > 0 ? set.col[slots[0]].r : 0;
  _dir.set(f2.x - f0.x, 0, f2.z - f0.z);
  if (_dir.lengthSq() > 1) _dir.normalize(); else _dir.set(0, 0, -1);
  // the lamps sit about 4 m behind the first pool, 0.9 m up
  const src = _src.set(f0.x - _dir.x * 4, f0.y, f0.z - _dir.z * 4);
  beam.pos.value.set(src.x, src.y, src.z, on * 0.55);
  beam.dir.value.copy(_dir);
  const col = glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
  for (const g of glints) {
    const dx = g.p.x - src.x, dy = g.p.y - src.y, dz = g.p.z - src.z, d = Math.hypot(dx, dy, dz);
    // retroreflectors face along the road: bright inside the beam, facing the truck
    const k = on * smooth(0.8, 0.97, (dx * _dir.x + dz * _dir.z) / d) * smooth(0.4, 0.85, Math.abs(dz) / d) * smooth(2, 7, d) * 1.4 / (1 + (d / 110) ** 2);
    col.setXYZ(g.i, k, k * 0.95, k * 0.84);
  }
  col.needsUpdate = true;
}
/** A sign material that shines back at the headlights, lit by a flood set (the old road has its own). */
export function retro<T extends THREE.MeshStandardMaterial>(m: T, falloff: number, set: FloodSet = roadFlood, beam: RetroBeam = retroBeam): T {
  floodlit(m, falloff, set);
  const flood = m.onBeforeCompile;
  m.onBeforeCompile = (sh, r) => {
    flood.call(m, sh, r);
    sh.uniforms.uRetroPos = beam.pos; sh.uniforms.uRetroDir = beam.dir;
    sh.fragmentShader = sh.fragmentShader
      .replace('uniform float uFloodScale;', 'uniform float uFloodScale; uniform vec4 uRetroPos; uniform vec3 uRetroDir;')
      .replace('totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;', `totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;
        vec3 rL = uRetroPos.xyz - vFWorld; float rD = length(rL); rL /= max(rD, 1e-3);
        float rFace = max(dot(normalize(vFNormal), rL), 0.0);
        float rBeam = smoothstep(0.74, 0.97, dot(-rL, uRetroDir));
        totalEmissiveRadiance += diffuseColor.rgb * uRetroPos.w * rFace * rBeam / (1.0 + rD * rD * 0.00012);`);
  };
  m.customProgramCacheKey = () => 'flood' + set.key + falloff + 'retro';
  return m;
}

/** The road's materials, lit by the road's floods; a stretch of the road drawn in another
 *  area (drive/corridors.ts) takes that area's floods. Without the track's gravel picture
 *  (it comes with the road, art.ts), gravel is a plain colour. */
export { ranchZ, onRanchRoad };

export function roadMaterials(set: FloodSet = roadFlood, gravelArt = true) {
  const std = (o: THREE.MeshStandardMaterialParameters, fall = 0.012) => floodlit(new THREE.MeshStandardMaterial(o), fall, set);
  // road surfaces lie a few centimetres over the verges: pull them forward in depth
  const top = { polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -2 };
  const link = chainlink(); link.repeat.set(1, 1);
  const mats = {
    ground: std({ map: artTexture('desert', [1, 1]), vertexColors: true, color: GROUND_TINT, roughness: 1 }),
    // mostly dry: the damp along its edges and in the wheel paths (roadSurface.ts)
    asphalt: wet(std({ map: highwayTex(), roughness: 0.62, ...top }, 0.01), 0.5, 100, 9, { lanes: HWY_LANES, dry: -0.15 }),
    // (its edges coloured to the desert's, roadTerrain.edgeColour)
    gravel: gravelArt ? std({ map: gravelTex(), color: GRAVEL_TINT, vertexColors: true, roughness: 0.95, ...top }, 0.01) : std({ color: 0x8a8070, vertexColors: true, roughness: 0.95, ...top }, 0.01),
    steel: std({ color: 0x8b9094, roughness: 0.45, metalness: 0.4 }),
    wood: std({ color: 0x6e5d4b, roughness: 0.92 }),
    concrete: std({ map: artTexture('concrete'), color: 0xbab2a2, roughness: 0.95 }),
    dark: std({ color: 0x0b0b0c, roughness: 0.9, ...top }),
    paint: std({ color: 0x8a6a4e, roughness: 0.9 }),
    sign: retro(new THREE.MeshStandardMaterial({ map: signAtlas(), roughness: 0.55 }), 0.004, set),
    fence: std({ map: link, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x9a9a92, roughness: 0.6 }),
    vc: std({ vertexColors: true, roughness: 0.9 }),
    rock: std({ color: 0x847a6c, roughness: 0.95, flatShading: true }),
    wire: std({ color: 0x55585a, roughness: 0.6, metalness: 0.3 }),
  };
  desertGround(mats.ground);   // the brush's dark spots past the ground cover's reach (world/flora.ts)
  return mats;
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
/** keep (here and below): only what stands at road-local (x, z) where it says yes (a
 *  stretch of the road drawn in another area). */
export type Keep = (x: number, z: number) => boolean;
export function poles(b: Build, keep: Keep = () => true) {
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
      if (!keep(x, z)) { prev = null; continue; }
      const m = place(x, surfaceY(x, z), z, ry + (hash(x, z) - 0.5) * 0.06, 1, (hash(z, x) - 0.5) * 0.03);
      list.push(m);
      const tops = INS.map((ix) => new THREE.Vector3(ix, 8.9, 0).applyMatrix4(m));
      if (prev) tops.forEach((t, k) => span(b, prev![k], t, 0.45, 8));
      prev = tops;
      if (inDrive(x, z)) circle(b, x, z, 0.3);
    }
  };
  const main: [number, number][] = [];
  for (let z = -1900; z <= 3400; z += 45) main.push([15, z + (hash(z, 1) - 0.5) * 4]);
  line(main, 0);
  const branch: [number, number][] = [];
  for (let x = 75; x <= 435; x += 60) branch.push([x, 190 + (x - 15) * 0.012]);
  line(branch, Math.PI / 2);
  b.root.add(instances(mergeGeometries(parts)!, b.m.vc, list));
}

// ---------- barbed wire fences: steel T-posts, a wooden post every tenth, four strands ----------
// The fence on the west side stops at the motel's and the diner's ground (roadShape.ts, MOTEL
// and DINER), and the one on the east is open where the old Roswell road goes off across from
// the diner (roadTerrain.oldStub). Both are open along Caliche's main street (TOWN, world/Town.ts),
// where walls further out keep the truck in the town.
const DINER_GAP: [number, number] = [DINER.z - 40, TOWN.z1], OLD_GAP: [number, number] = [DINER.z - 18, DINER.z + 18];
const EAST_GAP: [number, number] = [OLD_GAP[0], TOWN.z1];
export const MOTEL_GAP: [number, number] = [MOTEL.z - 46, MOTEL.z + 46];
/** The ranch's road (ranchRoad): road-local z of its middle at x, and where its gate stands in the fence. */
const RANCH_GAP: [number, number] = [ranchZ(HWY.fence) - 3.4, ranchZ(HWY.fence) + 3.4];
export function fences(b: Build, keep: Keep = () => true) {
  const tpost: THREE.Matrix4[] = [], wpost: THREE.Matrix4[] = [];
  const line = (ax: number, az: number, bx: number, bz: number, ...gaps: [number, number][]) => {
    const len = Math.hypot(bx - ax, bz - az), n = Math.ceil(len / 4.5);
    let prev: THREE.Vector3 | null = null;
    for (let i = 0; i <= n; i++) {
      const x = ax + (bx - ax) * i / n, z = az + (bz - az) * i / n;
      if (gaps.some((g) => z > g[0] && z < g[1]) || !keep(x, z)) { prev = null; continue; }
      const y = surfaceY(x, z), lean = (hash(x * 3, z) - 0.5) * 0.06;
      (i % 10 === 0 || i === n ? wpost : tpost).push(place(x, y, z, hash(z, x) * 3, 1, lean));
      const p = new THREE.Vector3(x, y, z);
      if (prev) for (const h of [0.45, 0.7, 0.95, 1.2]) span(b, prev.clone().setY(prev.y + h), p.clone().setY(y + h), 0.04, 1);
      prev = p;
    }
  };
  line(HWY.fence, -1000, HWY.fence, 2900, EAST_GAP, RANCH_GAP);
  line(-HWY.fence, -1000, -HWY.fence, 2900, MOTEL_GAP, [GUARD.z0 - 0.6, GUARD.z1 + 0.6], DINER_GAP);
  line(-HWY.fence - 0.3, PASTURE.north, PASTURE.west, PASTURE.north);
  line(-HWY.fence - 0.3, PASTURE.south, PASTURE.west, PASTURE.south);
  line(PASTURE.west, PASTURE.north, PASTURE.west, PASTURE.south);
  b.root.add(instances(colored(new THREE.BoxGeometry(0.05, 1.45, 0.05).translate(0, 0.6, 0), [0.16, 0.2, 0.16]), b.m.vc, tpost));
  b.root.add(instances(colored(new THREE.CylinderGeometry(0.07, 0.085, 1.7, 6).translate(0, 0.7, 0), [0.36, 0.32, 0.27]), b.m.vc, wpost));
  // the fences hold the truck, and invisible ends close the highway north and south
  const t = 0.15, F = HWY.fence;
  wall(b, F - t, F + t, DRIVE_N, OLD_GAP[0]); wall(b, F - t, F + t, TOWN.z1, DRIVE_S);
  wall(b, 30 - t, 30 + t, OLD_GAP[0], OLD_GAP[1]);   // the old road is for chapter six (OldRoad.ts)
  // round the town's east side: from the old road's south edge out, down, and back to the fence
  wall(b, 30 - t, TOWN.east, OLD_GAP[1] - t, OLD_GAP[1] + t); wall(b, TOWN.east - t, TOWN.east + t, OLD_GAP[1], TOWN.z1);
  wall(b, F - t, TOWN.east, TOWN.z1 - t, TOWN.z1 + t);
  wall(b, -F - t, -F + t, DRIVE_N, MOTEL_GAP[0]); wall(b, -F - t, -F + t, MOTEL_GAP[1], GUARD.z0 - 0.6);
  wall(b, -F - t, -F + t, GUARD.z1 + 0.6, DINER_GAP[0]); wall(b, -F - t, -F + t, DINER_GAP[1], DRIVE_S);
  // round the town's west side, behind its buildings (the diner's lot is open to the desert as before)
  wall(b, TOWN.west - t, TOWN.west + t, DINER.z + 45, TOWN.z1); wall(b, TOWN.west, -F, TOWN.z1 - t, TOWN.z1 + t);
  wall(b, TOWN.west, -F, DINER.z + 45 - t, DINER.z + 45 + t);
  wall(b, PASTURE.west, -F, PASTURE.north - t, PASTURE.north + t);
  wall(b, PASTURE.west, -F, PASTURE.south - t, PASTURE.south + t);
  wall(b, PASTURE.west - t, PASTURE.west + t, PASTURE.north, PASTURE.south);
  wall(b, -F, F, DRIVE_N - 1, DRIVE_N); wall(b, -F, F, DRIVE_S, DRIVE_S + 1);
}

// ---------- delineator posts: the reflectors glint in the headlights (RoadArea.update) ----------
/** Where the delineator posts stand along the highway (road-local x, z; the foot at surfaceY):
 *  both shoulders, every 80 m, not across the survey track's mouth, the motel's lot or Caliche.
 *  core/cutouts.ts has the shape at the end of the headlights stand in front of one. */
export function delineatorSpots(): [number, number][] {
  const out: [number, number][] = [];
  for (const side of [-1, 1]) for (let z = -1000 + (side > 0 ? 40 : 0); z <= 2900; z += 80) {
    if (side < 0 && z > 470 && z < 570) continue; // the mouth of the survey track
    if (side < 0 && z > MOTEL.z - 42 && z < MOTEL.z + 42) continue; // the motel's lot
    if (z > DINER.z - 30 && z < TOWN.z1) continue; // the way into the diner's lot, the old road's mouth, Caliche
    out.push([side * 7.2, z]);
  }
  return out;
}
export function delineators(b: Build, keep: Keep = () => true) {
  const list: THREE.Matrix4[] = [], glints: { i: number; p: THREE.Vector3 }[] = [];
  for (const [x, z] of delineatorSpots()) {
    const y = surfaceY(x, z);
    if (!keep(x, z)) continue;
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
export function signs(b: Build, keep: Keep = () => true) {
  if (keep(-7.6, 60)) sign(b, -7.6, 60, Math.PI, 0.76, 0.95, 1.5, SIGNS.speed, [0], b.m.steel);
  // the mile markers, both sides, a mile apart (the next one south stands short of the diner)
  for (const [z, face] of [[210, SIGNS.mile], [210 + MILE, SIGNS.mile111]] as const) {
    if (keep(-7.4, z)) sign(b, -7.4, z, Math.PI, 0.3, 0.6, 1.0, face, [0], b.m.steel, false);
    if (keep(7.4, z + 6)) sign(b, 7.4, z + 6, 0, 0.3, 0.6, 1.0, face, [0], b.m.steel, false);
  }
  if (keep(-9.2, 315)) sign(b, -9.2, 315, Math.PI, 2.6, 0.98, 1.8, SIGNS.roswell, [-0.9, 0.9], b.m.steel);
  // the turn-off: old enamel on two wooden posts, angled to the traffic from the north
  if (keep(-12.5, 511.5)) sign(b, -12.5, 511.5, 2.6, 1.8, 1.13, 1.0, SIGNS.station, [-0.65, 0.65], b.m.wood);
}

// ---------- the old road, closed until chapter six (Tom 15 October) ----------
// Two barricades across the old road between the highway's fence and the end of what this area
// drives (the wall at x 30 in fences), striped orange and white, with a ROAD CLOSED plate and a
// flashing amber lamp on each. Their own group, so RoadArea can take them away when chapter
// five sends the truck out on the old road (OldRoad.ts never has them).
export function roadClosed(b: Build) {
  const g = new THREE.Group(); g.name = 'roadClosed';
  const x = 25.5, z0 = DINER.z, y = surfaceY(x, z0), m = b.m;
  g.position.set(x, y, z0); g.rotation.y = -Math.PI / 2;      // the faces look west, at the highway
  const lamps: number[] = [];
  for (const off of [-1.45, 1.45]) {
    for (const h of [0.45, 0.85, 1.25]) {
      const rail = new THREE.Mesh(uvInto(new THREE.BoxGeometry(2.4, 0.2, 0.03), SIGNS.stripes), m.sign);
      rail.position.set(off, h, 0); g.add(rail);
    }
    for (const lx of [-1.0, 1.0]) for (const lz of [-0.32, 0.32]) {
      rod(g, new THREE.Vector3(off + lx, 0, lz), new THREE.Vector3(off + lx, 1.35, 0), 0.03, m.wood, 5);
    }
    // the blinker on top of the barricade, its housing on the top rail
    lamps.push(b.glow.add(x, y + 1.55, z0 + off, 0.45, 0xffa020, 3.4, off > 0 ? Math.PI : 0));
    cyl(g, 0.07, 0.07, 0.14, m.dark, off, 1.42, 0, 8);
    circle(b, x, z0 + off - 1, 0.3); circle(b, x, z0 + off + 1, 0.3);
  }
  const plate = new THREE.Mesh(uvInto(new THREE.PlaneGeometry(1.25, 0.47), SIGNS.closed), m.sign);
  plate.position.set(0, 1.62, 0.03); g.add(plate);
  box(g, 1.27, 0.49, 0.02, m.steel, 0, 1.62, 0.01);
  for (const px of [-0.5, 0.5]) cyl(g, 0.03, 0.03, 1.9, m.steel, px, 0.95, -0.02, 6);
  b.root.add(g);
  return { group: g, lamps };
}

// ---------- the ranch's road: a two-track east to the ranch, its gate and its mailbox ----------
// Off the highway 200 m south of the start, under the telephone branch to the ranch (poles), to
// the yard (roadPlaces.ranch). A pipe gate, shut, in the right-of-way fence (the fence's wall
// still holds the truck), a culvert under the approach, the mailbox at the highway with the
// ranch's name on it. The track is the survey track's gravel, a little narrower.
/** Whether (x, z) is on the ranch's road or by its mailbox (no plants there: roadPlaces.plants). */
export function ranchRoad(b: Build, keep: Keep = () => true) {
  const s = b.statics, m = b.m, HW = 2.5;
  // the track: rows every 4 m from the shoulder to the yard, on the land (the verge, then the desert)
  // (its edges in the desert's colour, the picture's own fade under them: no hard line to the land)
  const pos: number[] = [], uv: number[] = [], col: number[] = [], idx: number[] = [];
  let rows = 0;
  for (let x = HWY.shoulder; x <= RANCH_TO; x += 4) {
    if (!keep(x, ranchZ(x))) continue;
    const t = (ranchZ(x + 1) - ranchZ(x - 1)) / 2, n = Math.hypot(1, t);
    for (const o of [-HW, -HW / 2, 0, HW / 2, HW]) {
      const px = x - t / n * o, pz = ranchZ(x) + o / n;
      pos.push(px, surfaceY(px, pz) + 0.035, pz); uv.push(0.5 + o / (2 * HW), x / 8);
      edgeColour(px, pz, Math.abs(o) === HW ? 1 : o ? 0.2 : 0, col);
    }
    if (rows++ > 0) for (let c = 0; c < 4; c++) { const a = (rows - 2) * 5 + c, e = a + 5; idx.push(a, e, a + 1, a + 1, e, e + 1); }
  }
  if (rows > 1) {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
    g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
    g.setIndex(idx); g.computeVertexNormals();
    if (g.attributes.normal.getY(0) < 0) { g.setIndex(idx.map((_, i) => idx[i - (i % 3) + 2 - (i % 3)])); g.computeVertexNormals(); }
    const mesh = new THREE.Mesh(g, m.gravel); mesh.name = 'ranchRoad';
    b.root.add(mesh);
  }
  // the gate: a tube gate between heavy posts, shut
  const F = HWY.fence, [g0, g1] = RANCH_GAP;
  if (keep(F, g0)) {
    const y0 = surfaceY(F, g0), y1 = surfaceY(F, g1);
    cyl(s, 0.13, 0.15, 2.1, m.wood, F, y0 + 0.85, g0, 7); cyl(s, 0.13, 0.15, 2.1, m.wood, F, y1 + 0.85, g1, 7);
    for (const h of [0.35, 0.6, 0.85, 1.1, 1.3]) rod(s, new THREE.Vector3(F, y0 + h, g0 + 0.15), new THREE.Vector3(F, y1 + h, g1 - 0.2), 0.024, m.steel, 6);
    rod(s, new THREE.Vector3(F, y0 + 0.35, g0 + 0.15), new THREE.Vector3(F, y1 + 1.3, g1 - 0.2), 0.02, m.steel, 6);
    for (const z of [g0 + 0.15, g1 - 0.2]) rod(s, new THREE.Vector3(F, y0 + 0.3, z), new THREE.Vector3(F, y0 + 1.35, z), 0.028, m.steel, 6);
    circle(b, F, g0, 0.2); circle(b, F, g1, 0.2);
  }
  // the mailbox on its post, opening to the highway, the name on both sides
  const mx = 9.6, mz = ranchZ(mx) - 4.2;
  if (keep(mx, mz)) {
    const my = surfaceY(mx, mz), g = new THREE.Group();
    g.position.set(mx, my, mz); g.rotation.set(0, (hash(mx, mz) - 0.5) * 0.12, 0.03);
    box(g, 0.09, 1.12, 0.09, m.wood, 0, 0.56, 0);
    box(g, 0.26, 0.05, 0.16, m.wood, 0, 1.12, 0);
    box(g, 0.5, 0.15, 0.2, m.steel, -0.05, 1.22, 0);
    const top = cyl(g, 0.1, 0.1, 0.5, m.steel, -0.05, 1.3, 0, 10); top.rotation.z = Math.PI / 2;
    for (const sd of [-1, 1]) {
      const plate = new THREE.Mesh(uvInto(new THREE.PlaneGeometry(0.36, 0.09), SIGNS.mailbox), m.sign);
      plate.position.set(-0.05, 1.22, sd * 0.103); plate.rotation.y = sd > 0 ? 0 : Math.PI; g.add(plate);
    }
    s.add(g);
    circle(b, mx, mz, 0.15);
  }
  culvert(b, keep, ...CULVERTS[1]);
}

// ---------- culverts: a corrugated pipe under an approach, its ends and their markers ----------
/** Under a track crossing the highway's verge at (x, z): the pipe's two ends `half` metres either
 *  side along z, a little concrete headwall behind each, and an object marker beside it facing
 *  the highway (ry). Pipe and headwalls stand down in the ditch along the verge, 0.2 m below the
 *  land (its decal, 'swale', is in highwayWear), so the pipe goes under the track. */
export function culvert(b: Build, keep: Keep, x: number, z: number, half: number, ry: number) {
  const s = b.statics, m = b.m;
  for (const sd of [-1, 1]) {
    const pz = z + sd * half;
    if (!keep(x, pz)) continue;
    const y = surfaceY(x, pz);
    // galvanised long ago, rusted brown now, half silted up
    const pipe = cyl(s, 0.32, 0.32, 0.55, m.paint, x, y - 0.17, pz + sd * 0.1, 12); pipe.rotation.x = Math.PI / 2;
    const hole = new THREE.Mesh(new THREE.CircleGeometry(0.29, 12), m.dark);
    hole.position.set(x, y - 0.17, pz + sd * 0.38); hole.rotation.y = sd > 0 ? 0 : Math.PI; s.add(hole);
    box(s, 1.3, 0.62, 0.2, m.concrete, x, y - 0.1, pz - sd * 0.12);
    const mx = x + (ry < 0 ? -0.9 : 0.9), mzz = pz + sd * 0.25, my = surfaceY(mx, mzz);
    cyl(s, 0.025, 0.025, 1.25, m.steel, mx, my + 0.6, mzz, 5);
    const plate = new THREE.Mesh(uvInto(new THREE.PlaneGeometry(0.2, 0.6), SIGNS.marker), m.sign);
    plate.position.set(mx + (ry < 0 ? -0.03 : 0.03), my + 0.92, mzz); plate.rotation.y = ry; s.add(plate);
    circle(b, x, pz + sd * 0.1, 0.4); circle(b, mx, mzz, 0.1);
  }
}
/** The culverts by the road: under the mouth of the survey track (the ranch's has its own). */
export function culverts(b: Build, keep: Keep = () => true) {
  culvert(b, keep, ...CULVERTS[0]);
}
/** The culverts (x, z, half, ry): under the survey track's mouth, and under the ranch's road (ranchRoad). */
const CULVERTS: [number, number, number, number][] = [[-13.8, 520, 4.4, Math.PI / 2], [13.5, ranchZ(13.5), 3.2, -Math.PI / 2]];

// ---------- wear on the highway: oil, damp patches, cracks, patches and tar (roadSurface.ts) ----------
/** The highway's decals, road-local. The same random sequence whatever keep() keeps, so a stretch
 *  drawn in another area (corridors.ts) has the same ones. More of them along Caliche's main
 *  street, where the cars stand and the water lies at the edges after the street is hosed. */
export function highwayWear(keep: Keep = () => true): Wear[] {
  const r = rng(4711), out: Wear[] = [];
  const add = (x: number, z: number, w: number, l: number, ry: number, k: Wear['k']) => { if (keep(x, z)) out.push({ x, z, w, l, ry, k }); };
  const one = (z: number, town: boolean) => {
    const k = r(), side = r() < 0.5 ? -1 : 1, a = r(), b = r(), c = r(), d = r();
    // the open road mostly dry, damp only by an edge now and then; in town along the gutters
    if (k < (town ? 0.4 : 0.12)) add(side * (town ? 3.6 + a * 1.2 : 4.1 + a * 0.5), z, 1.2 + b * (town ? 2.6 : 1.2), 2.4 + c * 4.5, town ? d * 6.3 : (d - 0.5) * 0.4, 'damp');
    else if (k < 0.42) add(side * (1.8 + (a - 0.5) * 0.4), z, 0.8 + b * 0.5, 2.5 + c * 4, (d - 0.5) * 0.2, 'drips');
    else if (k < 0.64) add(side * (a < 0.6 ? 2.65 : 0.95), z, 1.1 + b * 0.5, 2.5 + c * 3, (d - 0.5) * 0.1, 'cracks');
    // patches of all sizes, newer and blacker or sun-faded lighter, skewed off the lanes
    else if (k < 0.8) add(side * (1.2 + a * 1.8), z, 1.0 + b * 2.2, 1.2 + c * 3, (d - 0.5) * 0.3, (c * 7) % 1 < 0.45 ? 'fade' : 'patch');
    else add((a - 0.5) * 7, z, 3 + b * 4, 3 + c * 3, d * 6.3, 'seal');
  };
  for (let z = HWY.z0 + 40; z < HWY.z1 - 40; z += 10 + r() * 24) one(z, false);
  for (let z = TOWN.z0; z < TOWN.z1; z += 6 + r() * 8) one(z, true);
  // the mouths of the tracks and the lots: gravel carried onto the paved shoulder (short of the
  // edge line), rubber where they turn
  add(-4.3, 520, 6, 1.3, Math.PI / 2, 'grit'); add(-3.4, 516.5, 3, 4.2, 0.5, 'rubber');
  add(4.3, ranchZ(4.3), 5, 1.3, -Math.PI / 2, 'grit');
  add(-4.3, DINER.z - 4, 3.6, 1.3, Math.PI / 2, 'grit'); add(-4.3, DINER.z + 6, 3.6, 1.3, Math.PI / 2, 'grit');
  add(-2.6, DINER.z - 10, 3.4, 5, 2.6, 'rubber'); add(-2.6, DINER.z + 9, 3.4, 5, 0.6, 'rubber');
  add(-1.6, DINER.z - 1, 1.6, 3.4, 0.3, 'drips');
  // the ditches along the verge either side of each culvert
  for (const [x, z, half] of CULVERTS) for (const sd of [-1, 1]) add(x, z + sd * (half + 2.8), 1.9, 6.2, 0, 'swale');
  return out;
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
