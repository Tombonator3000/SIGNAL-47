import * as THREE from 'three';
import { M, box, cyl, rod, mergeStatic, noMerge } from './kit';
import { glowSprite } from '../core/textures';

const beaconTex = glowSprite('rgba(255,60,40,1)', 'rgba(255,30,20,0)');
const DEG = Math.PI / 180;

// Azimuth: degrees clockwise from north (north is -Z in world space).
export class Dish {
  root = new THREE.Group();
  az = new THREE.Group();
  el = new THREE.Group();
  beacon: THREE.Sprite;
  curAz: number; curEl: number;
  tgtAz: number; tgtEl: number;
  slew = 6; // degrees per second
  phase = Math.random() * 6;

  // lod 0: full model. lod 1: far dish, no back truss, fewer segments, metal base.
  // The red beacon is kept on both, it is what you actually see at night.
  constructor(public id: string, x: number, z: number, scale = 1, az0 = 42, el0 = 48, lod: 0 | 1 = 0) {
    const s = scale;
    const far = lod === 1;
    this.root.position.set(x, 0, z);
    this.root.scale.setScalar(s);
    // pedestal
    box(this.root, 7, 0.8, 7, far ? M.dishMetal : M.concrete, 0, 0.4, 0);
    cyl(this.root, 1.45, 2.05, 7, M.dishMetal, 0, 4.3, 0, 10);
    if (!far) box(this.root, 1.2, 2.2, 0.15, M.dishMetal, 0, 2.0, 2.02).rotation.x = -0.08;
    mergeStatic(this.root);
    // turntable and yoke
    this.az.position.y = 7.8;
    noMerge(this.az);
    this.root.add(this.az);
    cyl(this.az, 2.2, 2.2, 0.6, M.dishMetal, 0, 0.3, 0, 14);
    box(this.az, 3.6, 1.6, 2.6, M.dishMetal, 0, 1.3, 0.4);
    box(this.az, 0.55, 4.4, 1.5, M.dishMetal, -1.9, 3.0, 0);
    box(this.az, 0.55, 4.4, 1.5, M.dishMetal, 1.9, 3.0, 0);
    if (!far) box(this.az, 1.8, 1.2, 1.4, M.dishMetal, 0, 1.6, 1.9);
    // elevation assembly
    this.el.position.y = 4.6;
    noMerge(this.el);
    this.az.add(this.el);
    const R = 9, f = 7.2;
    const pts: THREE.Vector2[] = [];
    for (let i = 0; i <= 14; i++) { const r = 0.9 + (R - 0.9) * (i / 14); pts.push(new THREE.Vector2(r, (r * r) / (4 * f))); }
    const dish = new THREE.Mesh(new THREE.LatheGeometry(pts, far ? 20 : 40), M.dishWhite);
    dish.position.y = 1.1;
    this.el.add(dish);
    cyl(this.el, 1.6, 1.2, 1.4, M.dishMetal, 0, 0.6, 0, 12);
    cyl(this.el, 0.35, 0.35, 4.4, M.dishMetal, 0, 0, 0, 8).rotation.z = Math.PI / 2;
    // back truss: radial ribs and a lattice cone
    for (let k = 0; k < (far ? 0 : 16); k++) {
      const a = (k / 16) * Math.PI * 2;
      const rim = new THREE.Vector3(Math.cos(a) * R * 0.98, 1.1 + (R * R) / (4 * f) - 0.25, Math.sin(a) * R * 0.98);
      const hub = new THREE.Vector3(Math.cos(a) * 1.4, 0.2, Math.sin(a) * 1.4);
      rod(this.el, hub, rim, 0.09, M.dishMetal, 4);
      const low = new THREE.Vector3(Math.cos(a) * R * 0.55, 1.1 + (R * R * 0.3) / (4 * f) - 0.9, Math.sin(a) * R * 0.55);
      if (k % 2 === 0) rod(this.el, hub, low, 0.07, M.dishMetal, 4);
    }
    // feed legs to the focal point
    const apex = new THREE.Vector3(0, 1.1 + f, 0);
    for (let k = 0; k < 4; k++) {
      const a = (k / 4) * Math.PI * 2 + Math.PI / 4;
      const r0 = R * 0.62;
      rod(this.el, new THREE.Vector3(Math.cos(a) * r0, 1.1 + (r0 * r0) / (4 * f) + 0.05, Math.sin(a) * r0), apex, 0.11, M.dishMetal, 5);
    }
    cyl(this.el, 0.55, 0.85, 1.1, M.dishMetal, 0, apex.y, 0, 10);
    cyl(this.el, 1.1, 0.2, 0.35, M.dishWhite, 0, apex.y - 0.75, 0, 16);
    mergeStatic(this.el);
    mergeStatic(this.az);
    // red aircraft beacon on the feed
    this.beacon = new THREE.Sprite(new THREE.SpriteMaterial({ map: beaconTex, color: 0xffffff, transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false }));
    this.beacon.scale.setScalar(3.2);
    this.beacon.position.copy(apex).add(new THREE.Vector3(0, 0.9, 0));
    noMerge(this.beacon);
    this.el.add(this.beacon);

    this.curAz = this.tgtAz = az0; this.curEl = this.tgtEl = el0;
    this.apply();
  }

  point(az: number, el: number, slew?: number) { this.tgtAz = az; this.tgtEl = el; if (slew) this.slew = slew; }
  snap(az: number, el: number) { this.curAz = this.tgtAz = az; this.curEl = this.tgtEl = el; this.apply(); }
  get moving() { return Math.abs(this.curAz - this.tgtAz) > 0.05 || Math.abs(this.curEl - this.tgtEl) > 0.05; }

  update(dt: number, t: number) {
    const step = this.slew * dt;
    const dA = this.tgtAz - this.curAz, dE = this.tgtEl - this.curEl;
    this.curAz += Math.sign(dA) * Math.min(Math.abs(dA), step);
    this.curEl += Math.sign(dE) * Math.min(Math.abs(dE), step * 0.6);
    this.apply();
    const blink = 0.55 + 0.45 * Math.max(0, Math.sin(t * 1.6 + this.phase));
    (this.beacon.material as THREE.SpriteMaterial).opacity = blink;
  }

  private apply() {
    this.az.rotation.y = -this.curAz * DEG;
    this.el.rotation.x = -(90 - this.curEl) * DEG;
  }
}
