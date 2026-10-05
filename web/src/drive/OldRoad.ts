import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { floodSet, addFlood, floodlit, mergeStatic, box, cyl } from '../world/kit';
import { GlowPoints } from '../world/glow';
import { artTexture } from '../core/art';
import { rng } from '../core/textures';
import type { Box2, DriveArea, Obstacle, Surface } from './Drive';
import { retro, colored, instances, place, type RetroBeam } from './roadProps';
import { smooth } from './roadTerrain';
import {
  path, HALF, SHOULDER, FENCE, POWER, S0, S1, START, POSTS, BOLTS, C_LINE, BACK_WALL, SUN_AZ, SARO_FROM, I0,
  terrain, heightAt, nearest, profile, roadAt, beside, rnd, type RoadPt,
} from './oldRoadLayout';
import { oldAsphaltTex, milepostAtlas, POST_NUMBERS, postRect, boltTex, witnessTex, STAKE } from './oldRoadTextures';

// The old Roswell road at dawn, chapter six (KAPITLER.md, Roswell Road). The drive runs from
// just before the six-mile post to C, just past the eight-mile post, about 3.6 km, with the
// road continuing both ways into the haze for the camera at the end. Built in local metres
// inside `group` (placed at `origin`); world terms (start, obstacles, heights) have the
// origin added, as in RoadArea.
//
// Draw calls: about 20 (ground 2, asphalt 1, merged statics ~5, instanced posts, poles,
// stakes, bushes, rocks and yucca, wires 1, far mesas 1, far SARO 1, glows 1).

export const oldFlood = floodSet(6, 'oldroad', 0.1);
export const OLD_HEADLIGHTS = [0, 1, 2];
const beam: RetroBeam = { pos: { value: new THREE.Vector4(0, -999, 0, 0) }, dir: { value: new THREE.Vector3(0, 0, -1) } };

const S = (kind: Surface['kind'], grip: number, top: number, rough: number): Surface => ({ kind, grip, top, rough });
const SURF = { asphalt: S('asphalt', 1, 30, 0.08), shoulder: S('gravel', 0.85, 22, 0.3), verge: S('dirt', 0.6, 12, 0.6), dirt: S('dirt', 0.55, 10, 0.8) };
const _v = new THREE.Vector3(), _d = new THREE.Vector3(), _c = new THREE.Color();

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
  /** World waypoints along the right lane, start to past C (the tests' autopilot, and the coast to a stop). */
  route: THREE.Vector3[] = [];
  /** Set when the truck has run into the invisible end behind the start (the chapter says why). */
  hitBack = false;

  private origin: THREE.Vector3;
  private glow = new GlowPoints();
  private glints: { i: number; p: THREE.Vector3; k: number }[] = [];
  private hemi: THREE.HemisphereLight;
  private sun: THREE.DirectionalLight;
  private far: THREE.Mesh;
  private farMat: THREE.MeshBasicMaterial;
  private saroMat: THREE.MeshBasicMaterial;

  constructor(origin: THREE.Vector3) {
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
      ground: std({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe6d6c0, roughness: 1 }),
      asphalt: std({ map: oldAsphaltTex(), roughness: 0.7, ...top }, 0.01),
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
    const ground = new THREE.Mesh(mergeGeometries([ribbon(rows, left, gy, true, 0), ribbon(rows, out, gy, true, 0)])!, m.ground);
    ground.name = 'desert';
    const plainMesh = new THREE.Mesh(plain, m.ground);
    // the road: asphalt and shoulders in one picture, 8.8 m across, 24 m per repeat
    const road = new THREE.Mesh(ribbon(path, [-4.4, -3.2, -1.6, 0, 1.6, 3.2, 4.4], (p, o) => p.y + profile(o), false, 1 / 24, 8.8), m.asphalt);
    this.group.add(plainMesh, ground, road);

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

    // ---------- the right-of-way fences along the driven part, the power line on the left ----------
    const tpost: THREE.Matrix4[] = [], wpost: THREE.Matrix4[] = [];
    for (const side of [-1, 1]) {
      let p0: THREE.Vector3 | null = null, k = 0;
      for (let s = START.s - 420; s <= 700; s += 5, k++) {
        const b = beside(s, side * FENCE), y = heightAt(b.x, b.z);
        (k % 10 === 0 ? wpost : tpost).push(place(b.x, y, b.z, rnd(s, side) * 3, 1, (rnd(b.x, s) - 0.5) * 0.06));
        const q = new THREE.Vector3(b.x, y, b.z);
        if (p0) for (const h of [0.55, 0.85, 1.15]) sag(p0.clone().setY(p0.y + h), q.clone().setY(y + h), 0.04, 1);
        p0 = q;
      }
    }
    this.group.add(instances(colored(new THREE.BoxGeometry(0.05, 1.45, 0.05).translate(0, 0.6, 0), [0.16, 0.2, 0.16]), m.vc, tpost));
    this.group.add(instances(colored(new THREE.CylinderGeometry(0.07, 0.085, 1.7, 6).translate(0, 0.7, 0), [0.38, 0.33, 0.27]), m.vc, wpost));
    {
      const wood = [0.3, 0.24, 0.17];
      const parts = [colored(new THREE.CylinderGeometry(0.11, 0.15, 9.4, 6).translate(0, 4.5, 0), wood), colored(new THREE.BoxGeometry(1.8, 0.1, 0.1).translate(0, 8.4, 0), wood)];
      const list: THREE.Matrix4[] = [];
      let tops: THREE.Vector3[] | null = null;
      for (let s = S0 + 20; s < S1 - 20; s += 70) {
        const b = beside(s, POWER), y = heightAt(b.x, b.z), mtx = place(b.x, y, b.z, Math.atan2(-b.p.tx, -b.p.tz) + Math.PI / 2 + (rnd(s, 2) - 0.5) * 0.08, 1, (rnd(2, s) - 0.5) * 0.03);
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
      for (let s = START.s + 400; s < -300; s += 90) for (const side of [-1, 1]) {
        if (rnd(s, side) < 0.35) continue;
        const b = beside(s + side * 20, side * 5.6), y = heightAt(b.x, b.z);
        list.push(place(b.x, y, b.z, 0, 1, (rnd(b.x, b.z) - 0.5) * 0.08));
        glint(new THREE.Vector3(b.x, y + 1.05, b.z), 0.5);
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
      const pos: number[] = [], col: number[] = [], N = 540, R = 4300;
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
        for (const k of [0, 1, 2, 0, 2, 3]) { pos.push(...q[k]); col.push(...(k < 2 ? [0.55, 0.55, 0.6] : [0.42, 0.4, 0.44])); }
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
      this.farMat = new THREE.MeshBasicMaterial({ vertexColors: true, fog: false, side: THREE.DoubleSide });
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
      // the diner, far behind the start: its sign and lot lights as a warm smudge in the west
      const back = roadAt(S0), dx = -back.tx, dz = -back.tz;
      for (const [k, sz2, c] of [[2600, 60, 0xffb070], [2620, 26, 0xff6a50]] as const) this.glow.add(back.x + dx * k, back.y + 6, back.z + dz * k, sz2, c);
    }

    mergeStatic(statics);
    this.group.add(statics, this.glow.build());

    // ---------- world terms ----------
    const w = (x: number, z: number) => new THREE.Vector3(origin.x + x, origin.y + heightAt(x, z), origin.z + z);
    this.start = { pos: w(START.x, START.z), heading: START.heading };
    for (let s = START.s; s <= 300; s += 20) { const b = beside(s, 1.6); this.route.push(w(b.x, b.z)); }
  }

  /** Where a world point is: distance along the road (s), signed offset (o) and miles from the diner. */
  where(x: number, z: number) {
    const n = nearest(x - this.origin.x, z - this.origin.z);
    return { s: n.s, o: n.o, d: n.d, mi: 8.15 + n.s / 1609.34 };
  }
  /** World height of the road where C crosses it (the far mesas stand on it). */
  get crossY() { return this.origin.y + path[I0].y; }
  /** World point in the right lane (o metres right of the centreline) at a distance s along the road. */
  lane(s: number, out = new THREE.Vector3(), o = 1.6) {
    const b = beside(s, o);
    return out.set(b.x + this.origin.x, this.origin.y + heightAt(b.x, b.z), b.z + this.origin.z);
  }
  /** Signed distance of a world point past C (positive once over the line, going north-east). */
  pastC(x: number, z: number) { return -(z - this.origin.z - C_LINE.z); }
  /** The local frame: world position of a local point. */
  toWorld(x: number, y: number, z: number, out = new THREE.Vector3()) { return out.set(x + this.origin.x, y + this.origin.y, z + this.origin.z); }

  surface(x: number, z: number): Surface {
    const n = nearest(x - this.origin.x, z - this.origin.z), a = Math.abs(n.o);
    if (n.d === Infinity) return SURF.dirt;
    if (a <= HALF + 0.2) return SURF.asphalt;
    if (a <= SHOULDER + 0.6) return SURF.shoulder;
    return a <= 10 ? SURF.verge : SURF.dirt;
  }
  height(x: number, z: number) { return this.origin.y + heightAt(x - this.origin.x, z - this.origin.z); }
  /** The truck's centre may not leave the right of way, nor go back past the end behind the start. */
  blocked(x: number, z: number) {
    const n = nearest(x - this.origin.x, z - this.origin.z);
    if (n.d === Infinity || Math.abs(n.o) > FENCE - 2.5) return true;
    if (n.s < BACK_WALL || n.s > S1 - 300) { if (n.s < BACK_WALL) this.hitBack = true; return true; }
    return false;
  }

  /** Every frame while the area is shown: the dawn light (0..1), glints in the headlights, and
   *  the far horizon kept round the camera. fog is the colour of the haze now. */
  update(_dt: number, t: number, cam: THREE.Vector3, dawn: number, fog: THREE.Color) {
    this.glow.update(t);
    this.hemi.intensity = 0.2 + 0.75 * dawn;
    this.hemi.color.setRGB(0.13, 0.19, 0.31).lerp(_c.setRGB(0.56, 0.64, 0.8), dawn);
    this.hemi.groundColor.setRGB(0.16, 0.11, 0.06).lerp(_c.setRGB(0.29, 0.23, 0.17), dawn);
    this.sun.intensity = 0.55 * smooth(0.85, 1.0, dawn);
    this.far.position.set(cam.x - this.origin.x, 0, cam.z - this.origin.z);
    this.farMat.color.copy(fog).multiplyScalar(0.55 + 0.25 * dawn);
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

