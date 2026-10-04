import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { M, box, cyl, rod } from './kit';
import { GlowPoints } from './glow';

const DEG = Math.PI / 180;
const R = 9, F = 7.2;                       // reflector radius and focal length (dish-local metres)
const APEX_Y = 1.1 + F;
const BEACON = new THREE.Vector3(0, APEX_Y + 0.9, 0);

// Pointing state of one antenna. Azimuth: degrees clockwise from north (north is -Z).
// Drawing is done for all antennas at once by DishArray.
export class Dish {
  curAz: number; curEl: number;
  tgtAz: number; tgtEl: number;
  slew = 6; // degrees per second
  phase = Math.random() * 6;

  // lod 0: full model. lod 1: far dish, no back truss, fewer segments, metal base.
  constructor(public id: string, public x: number, public z: number, public scale = 1, az0 = 42, el0 = 48, public lod: 0 | 1 = 0) {
    this.curAz = this.tgtAz = az0; this.curEl = this.tgtEl = el0;
  }

  point(az: number, el: number, slew?: number) { this.tgtAz = az; this.tgtEl = el; if (slew) this.slew = slew; }
  snap(az: number, el: number) { this.curAz = this.tgtAz = az; this.curEl = this.tgtEl = el; }
  get moving() { return Math.abs(this.curAz - this.tgtAz) > 0.05 || Math.abs(this.curEl - this.tgtEl) > 0.05; }

  update(dt: number) {
    const step = this.slew * dt;
    const dA = this.tgtAz - this.curAz, dE = this.tgtEl - this.curEl;
    this.curAz += Math.sign(dA) * Math.min(Math.abs(dA), step);
    this.curEl += Math.sign(dE) * Math.min(Math.abs(dE), step * 0.6);
  }

  // World transforms of the three moving levels.
  matrices(root: THREE.Matrix4, az: THREE.Matrix4, el: THREE.Matrix4) {
    root.compose(_p.set(this.x, 0, this.z), _q.identity(), _s.setScalar(this.scale));
    az.copy(root).multiply(_m.makeTranslation(0, 7.8, 0)).multiply(_m2.makeRotationY(-this.curAz * DEG));
    el.copy(az).multiply(_m.makeTranslation(0, 4.6, 0)).multiply(_m2.makeRotationX(-(90 - this.curEl) * DEG));
  }
}
const _p = new THREE.Vector3(), _q = new THREE.Quaternion(), _s = new THREE.Vector3();
const _m = new THREE.Matrix4(), _m2 = new THREE.Matrix4();

// The model, built once per level of detail and merged into one geometry per material.
function rootPart(g: THREE.Group, far: boolean) {
  box(g, 7, 0.8, 7, far ? M.dishMetal : M.concrete, 0, 0.4, 0);
  cyl(g, 1.45, 2.05, 7, M.dishMetal, 0, 4.3, 0, 10);
  if (!far) box(g, 1.2, 2.2, 0.15, M.dishMetal, 0, 2.0, 2.02).rotation.x = -0.08;
}
function azPart(g: THREE.Group, far: boolean) {
  cyl(g, 2.2, 2.2, 0.6, M.dishMetal, 0, 0.3, 0, 14);
  box(g, 3.6, 1.6, 2.6, M.dishMetal, 0, 1.3, 0.4);
  box(g, 0.55, 4.4, 1.5, M.dishMetal, -1.9, 3.0, 0);
  box(g, 0.55, 4.4, 1.5, M.dishMetal, 1.9, 3.0, 0);
  if (!far) box(g, 1.8, 1.2, 1.4, M.dishMetal, 0, 1.6, 1.9);
}
function elPart(g: THREE.Group, far: boolean) {
  const pts: THREE.Vector2[] = [];
  for (let i = 0; i <= 14; i++) { const r = 0.9 + (R - 0.9) * (i / 14); pts.push(new THREE.Vector2(r, (r * r) / (4 * F))); }
  const dish = new THREE.Mesh(new THREE.LatheGeometry(pts, far ? 20 : 40), M.dishWhite);
  dish.position.y = 1.1;
  g.add(dish);
  cyl(g, 1.6, 1.2, 1.4, M.dishMetal, 0, 0.6, 0, 12);
  cyl(g, 0.35, 0.35, 4.4, M.dishMetal, 0, 0, 0, 8).rotation.z = Math.PI / 2;
  // back truss: radial ribs and a lattice cone
  for (let k = 0; k < (far ? 0 : 16); k++) {
    const a = (k / 16) * Math.PI * 2;
    const rim = new THREE.Vector3(Math.cos(a) * R * 0.98, 1.1 + (R * R) / (4 * F) - 0.25, Math.sin(a) * R * 0.98);
    const hub = new THREE.Vector3(Math.cos(a) * 1.4, 0.2, Math.sin(a) * 1.4);
    rod(g, hub, rim, 0.09, M.dishMetal, 4);
    const low = new THREE.Vector3(Math.cos(a) * R * 0.55, 1.1 + (R * R * 0.3) / (4 * F) - 0.9, Math.sin(a) * R * 0.55);
    if (k % 2 === 0) rod(g, hub, low, 0.07, M.dishMetal, 4);
  }
  // feed legs to the focal point
  const apex = new THREE.Vector3(0, APEX_Y, 0);
  for (let k = 0; k < 4; k++) {
    const a = (k / 4) * Math.PI * 2 + Math.PI / 4;
    const r0 = R * 0.62;
    rod(g, new THREE.Vector3(Math.cos(a) * r0, 1.1 + (r0 * r0) / (4 * F) + 0.05, Math.sin(a) * r0), apex, 0.11, M.dishMetal, 5);
  }
  cyl(g, 0.55, 0.85, 1.1, M.dishMetal, 0, apex.y, 0, 10);
  cyl(g, 1.1, 0.2, 0.35, M.dishWhite, 0, apex.y - 0.75, 0, 16);
}
// Merge one level into a single indexed geometry per material. Indexed keeps the
// vertex count (and vertex shader work) at about a third of the flattened version.
function baked(build: (g: THREE.Group, far: boolean) => void, far: boolean) {
  const g = new THREE.Group();
  build(g, far);
  g.updateMatrixWorld(true);
  const buckets = new Map<THREE.Material, THREE.BufferGeometry[]>();
  g.traverse((o) => {
    const m = o as THREE.Mesh;
    if (!m.isMesh) return;
    const geo = m.geometry.clone().applyMatrix4(m.matrixWorld);
    for (const k of Object.keys(geo.attributes)) if (!['position', 'normal', 'uv'].includes(k)) geo.deleteAttribute(k);
    if (!geo.index) geo.setIndex([...Array(geo.attributes.position.count).keys()]);
    const mat = m.material as THREE.Material;
    if (!buckets.has(mat)) buckets.set(mat, []);
    buckets.get(mat)!.push(geo);
  });
  return [...buckets].map(([mat, geos]) => new THREE.Mesh(mergeGeometries(geos, false)!, mat));
}

// All antennas as a handful of instanced meshes: about ten draw calls instead of
// six per dish. The red aircraft beacons are one point cloud.
export class DishArray {
  group = new THREE.Group();
  private parts: { mesh: THREE.InstancedMesh; level: 0 | 1 | 2; dishes: Dish[] }[] = [];
  private beacons = new GlowPoints();
  private last: number[] = [];
  private mats = [new THREE.Matrix4(), new THREE.Matrix4(), new THREE.Matrix4()];

  constructor(public dishes: Dish[]) {
    for (const lod of [0, 1] as const) {
      const set = dishes.filter((d) => d.lod === lod);
      if (!set.length) continue;
      [rootPart, azPart, elPart].forEach((build, level) => {
        for (const m of baked(build, lod === 1)) {
          const inst = new THREE.InstancedMesh(m.geometry, m.material as THREE.Material, set.length);
          inst.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
          this.group.add(inst);
          this.parts.push({ mesh: inst, level: level as 0 | 1 | 2, dishes: set });
        }
      });
    }
    for (const d of dishes) this.beacons.add(0, -999, 0, 3.2 * d.scale, 0xff3c28, 1.6, d.phase);
    this.group.add(this.beacons.build());
    this.refresh(true);
    // cull() decides visibility per antenna group; the built-in test would be too coarse.
    for (const p of this.parts) p.mesh.frustumCulled = false;
  }

  private refresh(force: boolean) {
    const [root, az, el] = this.mats;
    const changed = new Set<Dish>();
    this.dishes.forEach((d, i) => {
      const key = d.curAz * 1000 + d.curEl;
      if (force || this.last[i] !== key) { this.last[i] = key; changed.add(d); }
    });
    if (!changed.size) return;
    for (const p of this.parts) {
      let dirty = false;
      p.dishes.forEach((d, i) => {
        if (!changed.has(d)) return;
        d.matrices(root, az, el);
        p.mesh.setMatrixAt(i, p.level === 0 ? root : p.level === 1 ? az : el);
        dirty = true;
      });
      if (dirty) p.mesh.instanceMatrix.needsUpdate = true;
    }
    const v = new THREE.Vector3();
    this.dishes.forEach((d, i) => {
      if (!changed.has(d)) return;
      d.matrices(root, az, el);
      this.beacons.setPosition(i, v.copy(BEACON).applyMatrix4(el));
    });
  }

  update(dt: number, t: number) {
    for (const d of this.dishes) d.update(dt);
    this.refresh(false);
    this.beacons.update(t);
  }

  // The instanced meshes span the whole array, so their bounding spheres almost always
  // touch the view. Test each antenna on its own instead and hide a group when none
  // of its antennas is in view (looking at the desk, for example).
  private frustum = new THREE.Frustum();
  private sphere = new THREE.Sphere();
  private pm = new THREE.Matrix4();
  cull(camera: THREE.Camera) {
    camera.updateMatrixWorld();
    this.frustum.setFromProjectionMatrix(this.pm.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse));
    const seen = new Set<Dish>();
    for (const d of this.dishes) {
      this.sphere.center.set(d.x, 9 * d.scale, d.z); this.sphere.radius = 13 * d.scale;
      if (this.frustum.intersectsSphere(this.sphere)) seen.add(d);
    }
    for (const p of this.parts) p.mesh.visible = p.dishes.some((d) => seen.has(d));
  }

  // World position of a dish's feed (for aiming cameras and lights at it).
  feedPosition(d: Dish, out = new THREE.Vector3()) {
    const [root, az, el] = this.mats;
    d.matrices(root, az, el);
    return out.set(0, APEX_Y, 0).applyMatrix4(el);
  }
}
