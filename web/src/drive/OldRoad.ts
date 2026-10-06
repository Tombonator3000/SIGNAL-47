import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { floodSet, addFlood, floodlit, mergeStatic, box, cyl } from '../world/kit';
import { GlowPoints } from '../world/glow';
import { artTexture, artLoaded } from '../core/art';
import { rng } from '../core/textures';
import type { Box2, DriveArea, Obstacle, Surface } from './Drive';
import { retro, colored, instances, place, type RetroBeam } from './roadProps';
import { smooth, hash, HWY, DINER, highwayGeometry } from './roadTerrain';
import { highwayTex } from './roadTextures';
import {
  path, HALF, SHOULDER, FENCE, POWER, S0, S1, START, POSTS, BOLTS, C_LINE, SUN_AZ, SARO_FROM, I0, J,
  terrain, heightAt, nearest, profile, beside, rnd, type RoadPt,
} from './oldRoadLayout';
import { oldAsphaltTex, milepostAtlas, POST_NUMBERS, postRect, boltTex, witnessTex, STAKE } from './oldRoadTextures';

// The old Roswell road at dawn, chapter six (KAPITLER.md, Roswell Road). The drive runs from
// the state highway across from Mesa Diner (mile 0) to C, just past the eight-mile post,
// 13.1 km, with the road going on into the haze for the camera at the end. The area lies on
// the road area's map at the diner (oldRoadLayout.ts): the diner itself is the road area's
// and is shown with this one, and this area draws the highway there and the land round it
// as the road area has them. Built in local metres inside `group` (placed at `origin`);
// world terms (start, obstacles, heights) have the origin added, as in RoadArea.
//
// Draw calls: about 24 (ground 3, asphalt 2, merged statics ~5, instanced posts, poles,
// stakes, bushes, rocks and yucca, wires 1, far mesas 1, far SARO 1, glows 1).

export const oldFlood = floodSet(6, 'oldroad', 0.1);
export const OLD_HEADLIGHTS = [0, 1, 2];
const beam: RetroBeam = { pos: { value: new THREE.Vector4(0, -999, 0, 0) }, dir: { value: new THREE.Vector3(0, 0, -1) } };

const S = (kind: Surface['kind'], grip: number, top: number, rough: number): Surface => ({ kind, grip, top, rough });
const SURF = { asphalt: S('asphalt', 1, 30, 0.08), shoulder: S('gravel', 0.85, 22, 0.3), verge: S('dirt', 0.6, 12, 0.6), dirt: S('dirt', 0.55, 10, 0.8) };
const _v = new THREE.Vector3(), _d = new THREE.Vector3(), _c = new THREE.Color();
/** The highway at the diner, in road-local z: drawn from here to here, and open to the truck
 *  this far either way from mile 0 (past that, the chapter says the bolt is ahead). */
const HWY_DRAWN = [DINER.z - 4000, HWY.z1] as const, HWY_OPEN = 260;
/** Road-local z of a local point, and local z of a road-local one (x: local = road-local + J.x). */
const toRoadZ = (z: number) => z - J.z + DINER.z, fromRoadZ = (rz: number) => rz + J.z - DINER.z;

// The haze at the horizon, the way the sky draws it (world/Sky.ts): blue-grey away from the
// sun, warm towards it. The far ground and the hills take this colour instead of the scene's
// one fog colour, so the land goes into the sky without a grey band between them.
const RIM = { sun: { value: new THREE.Vector3(0, 0, -1) }, dawn: { value: 0 }, up: { value: 0 } };
const RIM_GLSL = `
  uniform vec3 uRimSun; uniform float uRimDawn;
  vec3 rimHaze(vec3 dir, vec3 night) {
    vec3 h = normalize(vec3(dir.x, 0.0, dir.z) + vec3(1e-5, 0.0, 0.0));
    float east = 0.5 + 0.5 * dot(h, normalize(vec3(uRimSun.x, 0.0, uRimSun.z) + vec3(1e-5, 0.0, 0.0)));
    vec3 rim = mix(vec3(0.20, 0.23, 0.30), vec3(0.62, 0.42, 0.26), pow(east, 3.0)) + vec3(0.55, 0.30, 0.12) * pow(east, 8.0) * 0.8;
    return mix(night, rim * 0.92, smoothstep(0.0, 1.0, uRimDawn));
  }`;
// The mean of round 12's desert in linear light: the picture changes the colour of the ground
// from far off, but not its brightness on the whole.
const MACRO_MEAN = '0.3766, 0.2722, 0.1874';
/** Fog towards the haze of the horizon; with macro, the desert from above laid over the near
 *  tile from about 25 m out (round 12, 1 m to a pixel, 2 km to a repeat). */
function hazed(m: THREE.MeshStandardMaterial, macro: THREE.Texture | null) {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey.bind(m);
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.uniforms.uRimSun = RIM.sun; sh.uniforms.uRimDawn = RIM.dawn;
    if (macro) sh.uniforms.uMacro = { value: macro };
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', `#include <common>\n${RIM_GLSL}${macro ? '\nuniform sampler2D uMacro;' : ''}`)
      .replace('#include <fog_fragment>', `#ifdef USE_FOG
          float fogF = 1.0 - exp(-fogDensity * fogDensity * vFogDepth * vFogDepth);
          gl_FragColor.rgb = mix(gl_FragColor.rgb, rimHaze(normalize(vFWorld - cameraPosition), fogColor), fogF);
        #endif`);
    if (macro) sh.fragmentShader = sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
        {
          float md = length(vFWorld - cameraPosition);
          float mk = 0.35 * smoothstep(25.0, 140.0, md) + 0.65 * smoothstep(140.0, 450.0, md);
          vec3 mc = texture2D(uMacro, vFWorld.xz / 2048.0).rgb / vec3(${MACRO_MEAN});
          diffuseColor.rgb *= mix(vec3(1.0), mc, mk);
        }`);
  };
  m.customProgramCacheKey = () => key() + (macro ? 'rim+macro' : 'rim');
  return m;
}

// the desert's colour at a point: slow patches and grain, a little redder than the highway's
function tint(x: number, z: number, out: number[]) {
  const n = 0.82 + 0.12 * Math.sin(x * 0.011) * Math.cos(z * 0.013) + 0.14 * (rnd(Math.round(x), Math.round(z)) - 0.5);
  out.push(n * 1.02, n * 0.96, n * 0.9);
}
function attrs(pos: number[], uv: number[], col: number[] | null, idx: number[]) {
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  if (col) g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  g.setIndex(idx);
  g.computeVertexNormals();
  return g;
}
// rows along the path, columns across at offsets (right of travel positive)
function ribbon(rows: RoadPt[], cols: number[], y: (p: RoadPt, o: number, x: number, z: number) => number, ground: boolean, vScale: number, uSpan = 1) {
  const pos: number[] = [], uv: number[] = [], col: number[] | null = ground ? [] : null, idx: number[] = [];
  for (const p of rows) for (const o of cols) {
    const x = p.x - p.tz * o, z = p.z + p.tx * o;
    pos.push(x, y(p, o, x, z), z);
    if (col) { uv.push(x / 5, z / 5); tint(x, z, col); } else uv.push((o - cols[0]) / uSpan, p.s * vScale);
  }
  const w = cols.length;
  for (let r = 0; r < rows.length - 1; r++) for (let c = 0; c < w - 1; c++) {
    const a = r * w + c, b = a + w;
    idx.push(a, a + 1, b, a + 1, b + 1, b);
  }
  return attrs(pos, uv, col, idx);
}

export class OldRoad implements DriveArea {
  group = new THREE.Group();
  /** World pose for the truck at the start, in the right lane, facing the dawn. */
  start: { pos: THREE.Vector3; heading: number };
  obstacles: Obstacle[] = [];
  /** Never reached: the chapter ends the drive on C, not in a zone. */
  endZone: Box2 = { minX: 1e9, maxX: 1e9, minZ: 1e9, maxZ: 1e9 };
  headlights = { set: oldFlood, slots: OLD_HEADLIGHTS };
  /** World waypoints from the diner's lot across the highway and along the right lane to past C
   *  (the game's autopilot, World.autoInput, and the coast to a stop). */
  route: THREE.Vector3[] = [];
  routes: { park: THREE.Vector3[] } = { park: this.route };
  /** Set when the truck has run into the end of the open stretch of highway (the chapter says why). */
  hitBack = false;
  /** Places by the road with ground of their own (the diner's gravel lot), world boxes and heights. */
  private lots: { box: Box2; y: number }[] = [];

  private origin: THREE.Vector3;
  private glow = new GlowPoints();
  private glints: { i: number; p: THREE.Vector3; k: number }[] = [];
  private hemi: THREE.HemisphereLight;
  private sun: THREE.DirectionalLight;
  private far: THREE.Mesh;
  private farMat: THREE.ShaderMaterial;
  private saroMat: THREE.MeshBasicMaterial;

  /** roadOrigin: the road area's origin (World.ts); this area's lies so that mile 0 is on the
   *  highway's centreline across from the diner (oldRoadLayout.ts). */
  constructor(roadOrigin: THREE.Vector3) {
    const origin = roadOrigin.clone().add(new THREE.Vector3(-J.x, 0, DINER.z - J.z));
    this.origin = origin.clone();
    this.group.name = 'oldroad';
    this.group.position.copy(origin);
    oldFlood.count = 0;
    for (const p of oldFlood.pos) p.set(0, -999, 0, 0);
    for (const _ of OLD_HEADLIGHTS) addFlood(0, -999, 0, 0, 0xfff0d8, oldFlood);
    // the dawn: a blue sky light over warm ground, and the light of the sun still under the edge
    this.hemi = new THREE.HemisphereLight(0x8fa3c8, 0x4a3a2c, 1.0);
    this.sun = new THREE.DirectionalLight(0xffb070, 0);
    const az = SUN_AZ * Math.PI / 180;
    this.sun.position.set(Math.sin(az) * 100, 9, -Math.cos(az) * 100);
    this.group.add(this.hemi, this.sun, this.sun.target);

    const std = (o: THREE.MeshStandardMaterialParameters, fall = 0.012) => floodlit(new THREE.MeshStandardMaterial(o), fall, oldFlood);
    const top = { polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -2 };
    const m = {
      ground: hazed(std({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe6d6c0, roughness: 1 }), artLoaded('oldMacro') ? artTexture('oldMacro', [1, 1]) : null),
      hwyGround: hazed(std({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe6d6c0, roughness: 1, polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -1 }), artLoaded('oldMacro') ? artTexture('oldMacro', [1, 1]) : null),
      highway: hazed(std({ map: highwayTex(), roughness: 0.62, ...top }, 0.01), null),
      asphalt: hazed(std({ map: oldAsphaltTex(), roughness: 0.7, ...top }, 0.01), null),
      vc: std({ vertexColors: true, roughness: 0.9 }),
      steel: std({ color: 0x6f7a70, roughness: 0.5, metalness: 0.3 }),
      concrete: std({ map: artTexture('concrete'), color: 0xb8b0a0, roughness: 0.95 }),
      post: retro(new THREE.MeshStandardMaterial({ map: milepostAtlas(), roughness: 0.55 }), 0.004, oldFlood, beam),
      witness: retro(new THREE.MeshStandardMaterial({ map: witnessTex(), roughness: 0.6, alphaTest: 0.5, side: THREE.DoubleSide }), 0.006, oldFlood, beam),
      bush: std({ color: 0x4f4c31, roughness: 1, flatShading: true }),
      rock: std({ color: 0x8a7c6a, roughness: 0.95, flatShading: true }),
      wire: std({ color: 0x5a5650, roughness: 0.6, metalness: 0.3 }),
      rust: std({ color: 0x6a3a1e, roughness: 0.8, metalness: 0.2 }),
    };
    const statics = new THREE.Group();

    // ---------- ground: a wide ribbon along the road, and a coarse plain under and around it ----------
    const rows = path.filter((_, i) => i % 2 === 0);
    const out = [4.4, 6, 8, 10, 13, 17, 22, 30, 42, 58, 80, 110, 150, 200, 260];
    const gy = (_p: RoadPt, o: number, x: number, z: number) => Math.abs(o) >= 259 ? terrain(x, z) - 1 : heightAt(x, z);
    const left = out.map((o) => -o).reverse();
    const plain = (() => {
      let x0 = Infinity, x1 = -Infinity, z0 = Infinity, z1 = -Infinity;
      for (const p of path) { x0 = Math.min(x0, p.x); x1 = Math.max(x1, p.x); z0 = Math.min(z0, p.z); z1 = Math.max(z1, p.z); }
      const pos: number[] = [], uv: number[] = [], col: number[] = [], idx: number[] = [], C = 160, pad = 6000;
      const nx = Math.ceil((x1 - x0 + 2 * pad) / C), nz = Math.ceil((z1 - z0 + 2 * pad) / C);
      for (let j = 0; j <= nz; j++) for (let i = 0; i <= nx; i++) {
        const x = x0 - pad + i * C, z = z0 - pad + j * C;
        pos.push(x, terrain(x, z) - 1, z); uv.push(x / 5, z / 5); tint(x, z, col);
      }
      for (let j = 0; j < nz; j++) for (let i = 0; i < nx; i++) { const a = j * (nx + 1) + i, b = a + 1, c = a + nx + 1, d = c + 1; idx.push(a, c, b, b, c, d); }
      return attrs(pos, uv, col, idx);
    })();
    // (by the highway the highway's ground below has it: the old road's starts 120 m out)
    const own = rows.filter((p) => Math.abs(p.x - J.x) > 120);
    const ground = new THREE.Mesh(mergeGeometries([ribbon(own, left, gy, true, 0), ribbon(own, out, gy, true, 0)])!, m.ground);
    ground.name = 'desert';
    const plainMesh = new THREE.Mesh(plain, m.ground);
    // the road: asphalt and shoulders in one picture, 8.8 m across, 24 m per repeat; it begins
    // at the highway's edge
    const road = new THREE.Mesh(ribbon(path.filter((p) => p.s >= S0 + 6), [-4.4, -3.2, -1.6, 0, 1.6, 3.2, 4.4], (p, o) => p.y + profile(o), false, 1 / 24, 8.8), m.asphalt);
    // the highway at the diner: the road area's own strip (roadTerrain), and the ground either
    // side of it, finer where the old road comes off it; pulled forward in depth over the old
    // road's ground where the two lie on each other
    const hrows: RoadPt[] = [];
    for (let rz = HWY_DRAWN[1]; rz >= HWY_DRAWN[0];) {
      hrows.push({ x: J.x, z: fromRoadZ(rz), tx: 0, tz: -1, s: rz, y: 0 });
      const d = Math.abs(rz - DINER.z);
      rz -= d < 40 ? 2 : d < 600 ? 6 : 12;
    }
    const hcols = [6, 8, 10, 13, 16.5, 18, 21, 25, 30, 42, 58, 80, 110, 150, 200, 260];
    // (under the old road where it comes off the highway the ground sinks out of sight, or it
    // shows through the asphalt in patches: drivelook D01)
    const hy = (_p: RoadPt, o: number, x: number, z: number) => Math.abs(o) >= 259 ? terrain(x, z) - 1 : heightAt(x, z) - (nearest(x, z).d <= SHOULDER + 0.6 ? 0.15 : 0);
    const hwyGround = new THREE.Mesh(mergeGeometries([ribbon(hrows, hcols.map((o) => -o).reverse(), hy, true, 0), ribbon(hrows, hcols, hy, true, 0)])!, m.hwyGround);
    const hwy = new THREE.Mesh(highwayGeometry(...HWY_DRAWN).translate(J.x, 0, J.z - DINER.z), m.highway);
    this.group.add(plainMesh, ground, road, hwyGround, hwy);

    // ---------- mile posts on the right, facing the traffic ----------
    const glint = (p: THREE.Vector3, k = 1) => this.glints.push({ i: this.glow.add(p.x, p.y, p.z, 0.45, 0), p: p.clone().add(origin), k });
    for (const pst of POSTS) {
      const b = beside(pst.s, pst.o), y = heightAt(b.x, b.z), ry = Math.atan2(-b.p.tx, -b.p.tz);
      const g = new THREE.Group();
      g.position.set(b.x, y, b.z); g.rotation.y = ry;
      const face = new THREE.Mesh(new THREE.PlaneGeometry(0.3, 0.75), m.post);
      const r = postRect(POST_NUMBERS.indexOf(pst.mi)), uv = face.geometry.attributes.uv as THREE.BufferAttribute;
      for (let i = 0; i < uv.count; i++) uv.setXY(i, r[0] + uv.getX(i) * (r[2] - r[0]), uv.getY(i));
      face.position.set(0, 1.55, 0.015); g.add(face);
      box(g, 0.3, 0.75, 0.02, m.steel, 0, 1.55, 0);
      box(g, 0.05, 2.0, 0.03, m.steel, 0, 0.95, -0.03);
      statics.add(g);
      glint(new THREE.Vector3(b.x, y + 1.55, b.z), 0.8);
      this.obstacles.push({ kind: 'circle', x: b.x + origin.x, z: b.z + origin.z, r: 0.12 });
    }
    // ---------- the survey bolts in the right shoulder, each with a witness post ----------
    for (const bt of BOLTS) {
      // the bolt on C sits exactly on the line z = 0
      let b = beside(bt.s, bt.o);
      if (bt.c) { const k = b.z / (b.p.tz || 1e-6); b = beside(bt.s - k, bt.o); }
      const y = heightAt(b.x, b.z);
      cyl(statics, 0.17, 0.2, 0.36, m.concrete, b.x, y - 0.12, b.z, 10);
      const disk = new THREE.Mesh(new THREE.CircleGeometry(0.15, 24), new THREE.MeshStandardMaterial({ map: boltTex(bt.c ? ['STA 01', 'C', '1947'] : ['BM 6']), roughness: 0.5, metalness: 0.4 }));
      floodlit(disk.material as THREE.MeshStandardMaterial, 0.012, oldFlood);
      disk.rotation.x = -Math.PI / 2; disk.position.set(b.x, y + 0.065, b.z);
      this.group.add(disk);
      const w = beside(bt.c ? bt.s + 1.2 : bt.s, bt.o + 0.8), wy = heightAt(w.x, w.z);
      const post = new THREE.Group();
      post.position.set(w.x, wy, w.z); post.rotation.set(0.04, Math.atan2(-w.p.tx, -w.p.tz), -0.05);
      const f = new THREE.Mesh(new THREE.PlaneGeometry(0.1, 1.25), m.witness);
      f.position.set(0, 0.62, 0.007); post.add(f);
      box(post, 0.06, 1.2, 0.01, m.steel, 0, 0.6, -0.004);
      statics.add(post);
      glint(new THREE.Vector3(w.x, wy + 1.05, w.z), 0.7);
    }

    // ---------- C: old survey stakes in a straight line east-west, some cable between them ----------
    const r = rng(47), stakes: THREE.Matrix4[] = [];
    const wires: number[] = [];
    const sag = (a: THREE.Vector3, c: THREE.Vector3, s: number, n = 6) => {
      for (let i = 0; i < n; i++) for (const t of [i / n, (i + 1) / n]) wires.push(a.x + (c.x - a.x) * t, a.y + (c.y - a.y) * t - s * 4 * t * (1 - t), a.z + (c.z - a.z) * t);
    };
    const rust: number[] = [];
    let prev: THREE.Vector3 | null = null;
    for (let x = C_LINE.x0; x <= C_LINE.x1; x += 18) {
      const xx = x + (r() - 0.5) * 2;
      const n = nearest(xx, C_LINE.z);
      if (n.d < 9.5) { prev = null; continue; }   // not on the road or its shoulders
      const y = heightAt(xx, C_LINE.z), broken = r() < 0.15, hgt = broken ? 0.5 + r() * 0.3 : 1.15 + r() * 0.3;
      const lean = (r() - 0.5) * (broken ? 0.5 : 0.16);
      stakes.push(place(xx, y, C_LINE.z, r() * 3, 1, lean, hgt));
      const topP = new THREE.Vector3(xx, y + hgt * 0.95, C_LINE.z);
      // the cable: rusted through in places, lying on the ground in others
      if (prev && r() < 0.55) {
        const a = prev;
        for (let i = 0; i < 6; i++) for (const t of [i / 6, (i + 1) / 6]) rust.push(a.x + (topP.x - a.x) * t, a.y + (topP.y - a.y) * t - 0.55 * 4 * t * (1 - t), a.z + (topP.z - a.z) * t);
      }
      prev = topP;
    }
    // weathered wood with the faded red paint the survey put on their tops, so they read as a line
    const stakeGeo = mergeGeometries([colored(new THREE.BoxGeometry(0.09, 0.86, 0.09).translate(0, 0.38, 0), [STAKE.r, STAKE.g, STAKE.b]),
      colored(new THREE.BoxGeometry(0.095, 0.16, 0.095).translate(0, 0.88, 0), [0.62, 0.22, 0.16])])!;
    this.group.add(instances(stakeGeo, m.vc, stakes));

    // ---------- the right-of-way fences, both sides all the way, and along the highway ----------
    const tpost: THREE.Matrix4[] = [], wpost: THREE.Matrix4[] = [];
    const fence = (pts: { x: number; z: number; k: number }[]) => {
      let p0: THREE.Vector3 | null = null, n = 0;
      for (const { x, z, k } of pts) {
        if (Number.isNaN(x)) { p0 = null; continue; }
        const y = heightAt(x, z);
        (n++ % 10 === 0 ? wpost : tpost).push(place(x, y, z, rnd(k, x) * 3, 1, (rnd(x, k) - 0.5) * 0.06));
        const q = new THREE.Vector3(x, y, z);
        if (p0) for (const h of [0.55, 0.85, 1.15]) sag(p0.clone().setY(p0.y + h), q.clone().setY(y + h), 0.04, 1);
        p0 = q;
      }
    };
    // (open where a gate stands in it: fenceGaps, filled by the places along the road)
    const fenceGaps: { s0: number; s1: number; side: -1 | 1 }[] = [];
    for (const side of [-1, 1]) {
      const pts: { x: number; z: number; k: number }[] = [];
      for (let s = S0 + 40; s <= 700; s += 5) {
        if (fenceGaps.some((g) => g.side === side && s > g.s0 && s < g.s1)) { pts.push({ x: NaN, z: 0, k: 0 }); continue; }
        const b = beside(s, side * FENCE); pts.push({ x: b.x, z: b.z, k: s * side });
      }
      fence(pts);
    }
    // the highway's: open where the old road comes off it, and at the diner's lot
    for (const side of [-1, 1]) {
      const pts: { x: number; z: number; k: number }[] = [];
      for (let rz = HWY_DRAWN[0]; rz <= HWY_DRAWN[1]; rz += 4.5) {
        const gap = side > 0 ? Math.abs(rz - DINER.z) < 20 : rz > DINER.z - 40 && rz < DINER.z + 45;
        pts.push(gap ? { x: NaN, z: 0, k: 0 } : { x: J.x + side * HWY.fence, z: fromRoadZ(rz), k: rz * side });
      }
      fence(pts);
    }
    this.group.add(instances(colored(new THREE.BoxGeometry(0.05, 1.45, 0.05).translate(0, 0.6, 0), [0.16, 0.2, 0.16]), m.vc, tpost));
    this.group.add(instances(colored(new THREE.CylinderGeometry(0.07, 0.085, 1.7, 6).translate(0, 0.7, 0), [0.38, 0.33, 0.27]), m.vc, wpost));
    {
      const wood = [0.3, 0.24, 0.17];
      const parts = [colored(new THREE.CylinderGeometry(0.11, 0.15, 9.4, 6).translate(0, 4.5, 0), wood), colored(new THREE.BoxGeometry(1.8, 0.1, 0.1).translate(0, 8.4, 0), wood)];
      const list: THREE.Matrix4[] = [];
      let tops: THREE.Vector3[] | null = null;
      for (let s = S0 + 40; s < S1 - 20; s += 70) {
        const b = beside(s, POWER), y = heightAt(b.x, b.z), mtx = place(b.x, y, b.z, Math.atan2(-b.p.tx, -b.p.tz) + Math.PI / 2 + (rnd(s, 2) - 0.5) * 0.08, 1, (rnd(2, s) - 0.5) * 0.03);
        list.push(mtx);
        const t2 = [-0.75, 0.75].map((ix) => new THREE.Vector3(ix, 8.5, 0).applyMatrix4(mtx));
        if (tops) t2.forEach((t, i) => sag(tops![i], t, 0.5, 6));
        tops = t2;
      }
      // the telephone line up the east side of the highway, where the road area has it
      tops = null;
      for (let z = -1900; z <= 3400; z += 45) {
        const rz = z + (hash(z, 1) - 0.5) * 4;
        if (rz < HWY_DRAWN[0] || rz > HWY_DRAWN[1] || Math.abs(rz - DINER.z) < 12) { if (Math.abs(rz - DINER.z) >= 12) tops = null; continue; }
        const x = J.x + 15, zz = fromRoadZ(rz), mtx = place(x, heightAt(x, zz), zz, (hash(15, rz) - 0.5) * 0.06, 1, (hash(rz, 15) - 0.5) * 0.03);
        list.push(mtx);
        const t2 = [-0.75, 0.75].map((ix) => new THREE.Vector3(ix, 8.5, 0).applyMatrix4(mtx));
        if (tops) t2.forEach((t, i) => sag(tops![i], t, 0.5, 6));
        tops = t2;
      }
      this.group.add(instances(mergeGeometries(parts)!, m.vc, list));
    }
    {
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(wires, 3));
      g.setAttribute('normal', new THREE.Float32BufferAttribute(wires.map((_, i) => (i % 3 === 1 ? 1 : 0)), 3));
      const l = new THREE.LineSegments(g, m.wire); l.frustumCulled = false; this.group.add(l);
      const g2 = new THREE.BufferGeometry();
      g2.setAttribute('position', new THREE.Float32BufferAttribute(rust, 3));
      g2.setAttribute('normal', new THREE.Float32BufferAttribute(rust.map((_, i) => (i % 3 === 1 ? 1 : 0)), 3));
      const l2 = new THREE.LineSegments(g2, m.rust); l2.frustumCulled = false; this.group.add(l2);
    }
    // a few delineators where the road bends, their reflectors catching the headlights
    {
      const list: THREE.Matrix4[] = [];
      for (let s = S0 + 60; s < -300; s += 90) for (const side of [-1, 1]) {
        if (rnd(s, side) < 0.35) continue;
        const b = beside(s + side * 20, side * 5.6), y = heightAt(b.x, b.z);
        list.push(place(b.x, y, b.z, 0, 1, (rnd(b.x, b.z) - 0.5) * 0.08));
        glint(new THREE.Vector3(b.x, y + 1.05, b.z), 0.5);
      }
      // and the highway's, where the road area has them (roadProps.delineators)
      for (const side of [-1, 1]) for (let rz = -1000 + (side > 0 ? 40 : 0); rz <= 2900; rz += 80) {
        if (rz < HWY_DRAWN[0] || Math.abs(rz - DINER.z) < 30) continue;
        const x = J.x + side * 7.2, z = fromRoadZ(rz), y = heightAt(x, z);
        list.push(place(x, y, z, 0, 1, (hash(x, rz) - 0.5) * 0.05));
        glint(new THREE.Vector3(x, y + 1.05, z), 0.5);
      }
      const geo = mergeGeometries([colored(new THREE.BoxGeometry(0.07, 1.3, 0.025).translate(0, 0.55, 0), [0.6, 0.6, 0.58]),
        colored(new THREE.BoxGeometry(0.075, 0.16, 0.035).translate(0, 1.05, 0), [0.85, 0.85, 0.8])])!;
      this.group.add(instances(geo, m.vc, list));
    }

    // ---------- creosote, rocks and yucca, thick near the road ----------
    {
      const bush: THREE.Matrix4[] = [], rock: THREE.Matrix4[] = [], yucca: THREE.Matrix4[] = [], tints: THREE.Color[] = [];
      for (let n = 0; n < 9000 && bush.length < 1700; n++) {
        const s = START.s - 300 + r() * (1100 - (START.s - 300)), side = r() < 0.5 ? -1 : 1, off = 7 + Math.pow(r(), 1.6) * 170;
        const b = beside(s, side * off);
        if (Math.abs(off - FENCE) < 1 || Math.abs(off + side * POWER) < 1.5 || Math.abs(b.z - C_LINE.z) < 1.2) continue;
        const k = r(), y = heightAt(b.x, b.z);
        if (k < 0.74) {
          const sc = 0.45 + r() * 1.0;
          bush.push(place(b.x, y - 0.08 * sc, b.z, r() * 6, sc, 0, sc * (0.7 + r() * 0.5)));
          tints.push(new THREE.Color().setHSL(0.13 + r() * 0.07, 0.2 + r() * 0.15, 0.27 + r() * 0.12));
        } else if (k < 0.96) rock.push(place(b.x, y - 0.06, b.z, r() * 6, 0.15 + r() * 0.6, (r() - 0.5) * 0.5));
        else yucca.push(place(b.x, y - 0.05, b.z, r() * 6, 0.8 + r() * 0.5));
      }
      const lobe = (rad: number, x: number, y: number, z: number) => new THREE.IcosahedronGeometry(rad, 0).scale(1, 0.6, 1).translate(x, y, z);
      const bushes = instances(mergeGeometries([lobe(0.6, 0, 0.25, 0), lobe(0.42, 0.4, 0.2, 0.18)])!, m.bush, bush);
      tints.forEach((c, i) => bushes.setColorAt(i, c));
      this.group.add(bushes, instances(new THREE.DodecahedronGeometry(0.5, 0).scale(1, 0.62, 0.85).translate(0, 0.2, 0), m.rock, rock));
      // from the diner out to there, and along the highway: beside the way, a sequence of its own
      // (so the driven part keeps its plants)
      const r2 = rng(48), from = START.s - 300;
      for (let n = 0, made = 0; n < 16000 && made < 2600; n++) {
        const hwy = r2() < 0.18, side = r2() < 0.5 ? -1 : 1, off = 7 + Math.pow(r2(), 1.8) * 120;
        let x: number, z: number;
        if (hwy) { x = J.x + side * (9.5 + off * 0.5); z = fromRoadZ(HWY_DRAWN[0] + r2() * (HWY_DRAWN[1] - HWY_DRAWN[0])); }
        else { const b = beside(S0 + 25 + r2() * (from - S0 - 25), side * off); x = b.x; z = b.z; }
        const rx = x - J.x, rz = toRoadZ(z);
        if (Math.abs(Math.abs(rx) - HWY.fence) < 1 || (rx > -60 && rx < 0 && Math.abs(rz - DINER.z) < 60)) continue;   // the fence, the diner
        const nn = nearest(x, z);
        if (nn.d < 6.5 || Math.abs(Math.abs(nn.o) - FENCE) < 1 || Math.abs(nn.o + POWER) < 1.5) continue;
        if (Math.abs(rx) < 9 && Math.abs(rz - DINER.z) < 6000) continue;   // on the highway
        const k = r2(), y = heightAt(x, z);
        made++;
        if (k < 0.74) {
          const sc = 0.45 + r2() * 1.0;
          bush.push(place(x, y - 0.08 * sc, z, r2() * 6, sc, 0, sc * (0.7 + r2() * 0.5)));
          tints.push(new THREE.Color().setHSL(0.13 + r2() * 0.07, 0.2 + r2() * 0.15, 0.27 + r2() * 0.12));
        } else if (k < 0.96) rock.push(place(x, y - 0.06, z, r2() * 6, 0.15 + r2() * 0.6, (r2() - 0.5) * 0.5));
        else yucca.push(place(x, y - 0.05, z, r2() * 6, 0.8 + r2() * 0.5));
      }
      const leaf = [0.28, 0.36, 0.24], dry = [0.42, 0.34, 0.24];
      const parts = [colored(new THREE.CylinderGeometry(0.07, 0.09, 0.35, 5).translate(0, 0.17, 0), dry), colored(new THREE.CylinderGeometry(0.012, 0.02, 1.7, 4).translate(0, 1.15, 0), dry)];
      for (let i = 0; i < 12; i++) {
        const a = i / 12 * Math.PI * 2, tilt = 0.35 + (i % 3) * 0.3;
        parts.push(colored(new THREE.ConeGeometry(0.035, 0.75, 3).translate(0, 0.37, 0).rotateZ(tilt).rotateY(a).translate(0, 0.32, 0), leaf));
      }
      this.group.add(instances(mergeGeometries(parts)!, m.vc, yucca));
    }

    // ---------- far away: the mesas round the horizon (they follow the camera), SARO, the diner ----------
    {
      const pos: number[] = [], col: number[] = [], lit: number[] = [], N = 540, R = 4300;
      const ROAD_Y = path[I0].y;
      // flat-topped mesas: [azimuth, half width, top]; the one in the east-north-east is where
      // the sun comes up, its top 0.6 degrees over the road (KAPITLER.md: the sun takes its edge at 05:30)
      const MES: [number, number, number][] = [[SUN_AZ + 2, 9, R * Math.tan(0.6 * Math.PI / 180) + 1.5], [28, 7, 70], [128, 10, 55], [205, 14, 95], [252, 6, 60], [318, 11, 110], [352, 5, 48]];
      const h = (deg: number) => {
        let v = 12 + 8 * Math.sin(deg * 0.061 + 1.3) + 5 * Math.sin(deg * 0.17);
        for (const [a, w, t] of MES) {
          const d = Math.abs(((deg - a + 540) % 360) - 180);
          v = Math.max(v, t * (1 - THREE.MathUtils.smoothstep(d, w, w + 1.8)) * (0.97 + 0.03 * Math.sin(deg * 2.7)));
        }
        return v;
      };
      for (let i = 0; i < N; i++) {
        const a0 = i / N * 360, a1 = (i + 1) / N * 360;
        const p = (deg: number, y: number) => [Math.sin(deg * Math.PI / 180) * R, ROAD_Y + y, -Math.cos(deg * Math.PI / 180) * R];
        const q = [p(a0, -60), p(a1, -60), p(a1, h(a1)), p(a0, h(a0))];
        // the sides of the hills that face the sun light up once it is over the edge: the far
        // ones, on the other side of the sky from it
        const litA = Math.max(0, -Math.cos((a0 - SUN_AZ) * Math.PI / 180)), litB = Math.max(0, -Math.cos((a1 - SUN_AZ) * Math.PI / 180));
        for (const k of [0, 1, 2, 0, 2, 3]) { pos.push(...q[k]); col.push(k < 2 ? 0 : 1, 0, 0); lit.push(k === 1 || k === 2 ? litB : litA); }
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
      g.setAttribute('aLit', new THREE.Float32BufferAttribute(lit, 1));
      // the hills: the haze of the horizon behind them, a little darker, darker at the tops
      this.farMat = new THREE.ShaderMaterial({
        uniforms: { uRimSun: RIM.sun, uRimDawn: RIM.dawn, uSunUp: RIM.up, uNight: { value: new THREE.Color() } },
        vertexColors: true, side: THREE.DoubleSide,
        vertexShader: `attribute float aLit; varying vec3 vDir; varying float vTop; varying float vLit;
          void main() { vec4 w = modelMatrix * vec4(position, 1.0); vDir = w.xyz - cameraPosition; vTop = color.r; vLit = aLit;
            gl_Position = projectionMatrix * viewMatrix * w; }`,
        fragmentShader: `${RIM_GLSL}
          uniform float uSunUp; uniform vec3 uNight; varying vec3 vDir; varying float vTop; varying float vLit;
          void main() {
            vec3 col = rimHaze(normalize(vDir), uNight) * mix(0.86, 0.6, vTop) + vec3(0.55, 0.34, 0.18) * uSunUp * vLit * (0.15 + 0.15 * vTop);
            gl_FragColor = vec4(col, 1.0);
            #include <tonemapping_fragment>
            #include <colorspace_fragment>
          }`,
      });
      this.far = new THREE.Mesh(g, this.farMat);
      this.far.frustumCulled = false;
      this.far.renderOrder = -5;
      this.group.add(this.far);
      // SARO: 27 dishes small on the west-north-west horizon, red lamps on top
      const sb = SARO_FROM.bearing * Math.PI / 180, sx = Math.sin(sb) * SARO_FROM.dist, sz = -Math.cos(sb) * SARO_FROM.dist;
      const dishes: THREE.Matrix4[] = [], dr = rng(26);
      const dish = mergeGeometries([new THREE.CylinderGeometry(0.6, 0.9, 11, 5).translate(0, 5.5, 0),
        new THREE.CylinderGeometry(9, 2, 3.5, 12, 1, true).rotateX(-0.45).translate(0, 13, 0)])!;
      for (let i = 0; i < 27; i++) {
        const x = sx + (dr() - 0.5) * 700, z = sz + (dr() - 0.5) * 420, y = terrain(x, z) - 1;
        dishes.push(place(x, y, z, 0.45 + (dr() - 0.5) * 0.1, 1));
        this.glow.add(x, y + 17, z, 24, 0xff3a26, 1.6, dr() * 6);
      }
      for (let i = 0; i < 6; i++) this.glow.add(sx + (dr() - 0.5) * 300, terrain(sx, sz) + 5, sz + 180 + dr() * 60, 18, 0xffb45a);
      this.saroMat = new THREE.MeshBasicMaterial({ color: 0x3a3f4a, fog: false });
      const saro = instances(dish, this.saroMat, dishes);
      saro.frustumCulled = false;
      this.group.add(saro);
    }

    mergeStatic(statics);
    this.group.add(statics, this.glow.build());

    // ---------- world terms ----------
    const w = (x: number, z: number) => new THREE.Vector3(origin.x + x, origin.y + heightAt(x, z), origin.z + z);
    this.start = { pos: w(START.x, START.z), heading: START.heading };
    // out of the diner's lot (diner x 11 to 16, World.ts parks it there), over the highway, into
    // the right lane (to the south of the centreline, the road going east)
    for (const [rx, rz] of [[-4.5, 4], [-1.5, 3], [2.5, 2.4]]) this.route.push(w(J.x + rx, fromRoadZ(DINER.z + rz)));
    for (let s = S0 + 10; s <= 300; s += s < S0 + 200 ? 8 : 20) { const b = beside(s, 1.6); this.route.push(w(b.x, b.z)); }
  }

  /** A place by the road (the diner): its walls and posts hold the truck, its lot is gravel with
   *  its top at world height y (the same as RoadArea.addPlace). */
  addPlace(obstacles: Box2[], lot: Box2, y: number) {
    this.obstacles.push(...obstacles.map((b) => ({ kind: 'box' as const, ...b })));
    this.lots.push({ box: lot, y });
  }
  private lotAt(x: number, z: number) {
    for (const l of this.lots) if (x >= l.box.minX && x <= l.box.maxX && z >= l.box.minZ && z <= l.box.maxZ) return l;
    return null;
  }

  /** Where a world point is: distance along the road (s), signed offset (o) and miles from the
   *  diner. Off the road (on the highway, on the diner's lot) it is mile 0. */
  where(x: number, z: number) {
    const n = nearest(x - this.origin.x, z - this.origin.z), s = n.d === Infinity ? S0 : n.s;
    return { s, o: n.o, d: n.d, mi: 8.15 + s / 1609.34 };
  }
  /** World height of the road where C crosses it (the far mesas stand on it). */
  get crossY() { return this.origin.y + path[I0].y; }
  /** World point in the right lane (o metres right of the centreline) at a distance s along the road. */
  lane(s: number, out = new THREE.Vector3(), o = 1.6) {
    const b = beside(s, o);
    return out.set(b.x + this.origin.x, this.origin.y + heightAt(b.x, b.z), b.z + this.origin.z);
  }
  /** The truck's pose in the right lane at a mile, facing the dawn (the developer menu). */
  poseAt(mi: number) {
    const s = (mi - 8.15) * 1609.34, a = this.lane(s), b = this.lane(s + 4);
    return { pos: a, heading: Math.atan2(-(b.x - a.x), -(b.z - a.z)) };
  }
  /** Signed distance of a world point past C (positive once over the line, going north-east). */
  pastC(x: number, z: number) { return -(z - this.origin.z - C_LINE.z); }
  /** The local frame: world position of a local point. */
  toWorld(x: number, y: number, z: number, out = new THREE.Vector3()) { return out.set(x + this.origin.x, y + this.origin.y, z + this.origin.z); }

  surface(x: number, z: number): Surface {
    if (this.lotAt(x, z)) return SURF.shoulder;
    const lx = x - this.origin.x, lz = z - this.origin.z, ax = Math.abs(lx - J.x);
    if (ax <= HWY.shoulder) return ax <= HWY.paved ? SURF.asphalt : SURF.shoulder;
    const n = nearest(lx, lz), a = Math.abs(n.o);
    if (n.d === Infinity && ax <= HWY.margin) return SURF.verge;
    if (n.d === Infinity) return SURF.dirt;
    if (a <= HALF + 0.2) return SURF.asphalt;
    if (a <= SHOULDER + 0.6) return SURF.shoulder;
    return a <= 10 ? SURF.verge : SURF.dirt;
  }
  height(x: number, z: number) { return this.lotAt(x, z)?.y ?? this.origin.y + heightAt(x - this.origin.x, z - this.origin.z); }
  /** The truck's centre may not leave the right of way: the old road's, or the highway's for a
   *  little way either side of mile 0 (further, and the chapter says the bolt is ahead). */
  blocked(x: number, z: number) {
    const lx = x - this.origin.x, lz = z - this.origin.z;
    if (Math.abs(lx - J.x) < HWY.fence - 2.5) {
      if (Math.abs(toRoadZ(lz) - DINER.z) < HWY_OPEN) return false;
      this.hitBack = true; return true;
    }
    const n = nearest(lx, lz);
    if (n.d === Infinity || Math.abs(n.o) > FENCE - 2.5) return true;
    return n.s > S1 - 300;
  }

  /** Every frame while the area is shown: the dawn light (0..1), glints in the headlights, and
   *  the far horizon kept round the camera. fog is the colour of the haze now. */
  update(_dt: number, t: number, cam: THREE.Vector3, sky: { dawn: number; fog: THREE.Color; sunDir: THREE.Vector3; sun: number }) {
    const { dawn, fog } = sky;
    // the hills light up once the sun is over the edge (its glow comes a few minutes before)
    RIM.sun.value.copy(sky.sunDir); RIM.dawn.value = dawn; RIM.up.value = smooth(-0.002, 0.012, sky.sunDir.y) * sky.sun;
    this.glow.update(t);
    this.hemi.intensity = 0.2 + 0.75 * dawn;
    this.hemi.color.setRGB(0.13, 0.19, 0.31).lerp(_c.setRGB(0.56, 0.64, 0.8), dawn);
    this.hemi.groundColor.setRGB(0.16, 0.11, 0.06).lerp(_c.setRGB(0.29, 0.23, 0.17), dawn);
    this.sun.intensity = 0.55 * smooth(0.85, 1.0, dawn);
    this.far.position.set(cam.x - this.origin.x, 0, cam.z - this.origin.z);
    (this.farMat.uniforms.uNight.value as THREE.Color).copy(fog).multiplyScalar(0.45);
    this.saroMat.color.copy(fog).multiplyScalar(0.7);
    // reflectors: bright inside the headlight beam, facing the truck
    const f0 = oldFlood.pos[OLD_HEADLIGHTS[0]], f2 = oldFlood.pos[OLD_HEADLIGHTS[2]];
    const on = f0.w > 0 ? Math.max(oldFlood.col[OLD_HEADLIGHTS[0]].r, oldFlood.col[OLD_HEADLIGHTS[0]].g, oldFlood.col[OLD_HEADLIGHTS[0]].b) : 0;
    _d.set(f2.x - f0.x, 0, f2.z - f0.z);
    if (_d.lengthSq() > 1) _d.normalize(); else _d.set(0, 0, -1);
    const src = _v.set(f0.x - _d.x * 4, f0.y, f0.z - _d.z * 4);
    beam.pos.value.set(src.x, src.y, src.z, on * 0.55);
    beam.dir.value.copy(_d);
    const col = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    const hc = oldFlood.col[OLD_HEADLIGHTS[0]];
    for (const g of this.glints) {
      const dx = g.p.x - src.x, dy = g.p.y - src.y, dz = g.p.z - src.z, d = Math.hypot(dx, dy, dz);
      const k = (f0.w > 0 ? 1 : 0) * smooth(0.8, 0.97, (dx * _d.x + dz * _d.z) / d) * smooth(2, 7, d) * 1.4 * g.k / (1 + (d / 110) ** 2);
      col.setXYZ(g.i, k * hc.r, k * hc.g, k * hc.b);
    }
    col.needsUpdate = true;
  }

  dispose() {
    const mats = new Set<THREE.Material>();
    this.group.traverse((o) => {
      const mesh = o as THREE.Mesh;
      if (mesh.geometry) mesh.geometry.dispose();
      if (mesh.material) (Array.isArray(mesh.material) ? mesh.material : [mesh.material]).forEach((x) => mats.add(x));
      (o as THREE.DirectionalLight).dispose?.();
    });
    for (const mat of mats) {
      for (const tex of Object.values(mat).filter((v): v is THREE.Texture => v instanceof THREE.Texture)) if (!tex.name.startsWith('art/')) tex.dispose();
      mat.dispose();
    }
    for (const p of oldFlood.pos) p.w = 0;
    oldFlood.count = 0;
    this.group.removeFromParent();
  }
}

