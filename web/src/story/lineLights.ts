import * as THREE from 'three';
import { clockText } from './time';
import { LINES, toMap, fromMap, foot, alongFrom, pointAt } from './lines';

// HENDELSER.md, event 7: three lights, low over the desert west of the highway, keep pace with
// the truck for a stretch south of SARO, quite silent, and then turn off along B and go out.
// They follow the line, not the truck. Once in a night, driving south between SARO and the motel
// in the dark (main.ts decides when it may run). In SARO's terms (x east, z south).
//
// B runs 210 to 270 m west of the highway there, from SARO's turn-off south-west to STATION 01
// (story/lines.ts): they come up abreast of the truck over it, and when they leave the truck they
// go back onto it and along it, away south-west towards the station, and out.

/** Along B towards STATION 01 (the transit), in SARO's terms (the same as the map's). */
const TO_STATION = new THREE.Vector3(-LINES.B.dir[0], 0, -LINES.B.dir[1]);
/** How far ahead along B they make for when they turn off (metres). */
const AHEAD = 250;

export interface LineLightsIn {
  ok: boolean;
  /** Driving in SARO's area, and where the truck is. */
  truck: THREE.Vector3 | null;
  clock: number;
  ground: (x: number, z: number) => number;
  /** The night's curve (story/curve.ts) lets them come now (an event); left out, they may. */
  may?: boolean;
}

export class LineLights {
  state: 'none' | 'pace' | 'off' = 'none';
  done = false;
  private pts: THREE.Points;
  private mat: THREE.PointsMaterial;
  private base: THREE.Vector3[] = [];
  private t = 0;
  private lastZ = 0;

  constructor(scene: THREE.Scene, private note: (text: string) => void, private onDone: (clock: number) => void) {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(new Array(9).fill(0), 3));
    this.mat = new THREE.PointsMaterial({ color: 0xfff4e0, size: 4, sizeAttenuation: false, transparent: true, opacity: 0, depthWrite: false, fog: false, blending: THREE.AdditiveBlending });
    this.pts = new THREE.Points(g, this.mat);
    this.pts.frustumCulled = false; this.pts.name = 'lineLights'; this.pts.visible = false;
    scene.add(this.pts);
  }

  update(dt: number, s: LineLightsIn) {
    const tr = s.truck;
    if (this.state === 'none') {
      if (this.done || !s.ok || !tr) { if (tr) this.lastZ = tr.z; return; }
      const south = tr.z - this.lastZ > 0.05; this.lastZ = tr.z;
      if (south && tr.z > 120 && tr.z < 240 && s.may !== false) this.begin(tr, s);
      return;
    }
    this.t += dt;
    if (this.state === 'pace') {
      // abreast of the truck, 200 m off to the west, about twenty metres up
      const dz = tr ? tr.z - this.lastZ : 0; if (tr) this.lastZ = tr.z;
      for (const b of this.base) b.z += dz;
      this.mat.opacity = Math.min(0.95, this.t / 2.5);
      if (this.t > 16 || !tr || !s.ok) { this.state = 'off'; this.t = 0; this.turnOff(); this.note(`${clockText(s.clock, false)}. Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out.`); this.onDone(s.clock); }
    } else {
      for (const b of this.base) b.addScaledVector(this.off, 38 * dt);
      this.mat.opacity = Math.max(0, 0.95 - this.t / 7);
      if (this.t > 7.5) { this.state = 'none'; this.pts.visible = false; return; }
    }
    const a = this.pts.geometry.getAttribute('position') as THREE.BufferAttribute;
    this.base.forEach((b, i) => a.setXYZ(i, b.x, b.y + Math.sin(this.t * 0.7 + i) * 0.4, b.z));
    a.needsUpdate = true;
  }
  /** Which way they go when they leave the truck: onto B and along it towards STATION 01 (from
   *  the middle one: a point of B AHEAD metres on from the nearest, in SARO's terms). */
  private off = TO_STATION.clone();
  private turnOff() {
    const m = this.base[1] ?? this.base[0];
    if (!m) { this.off.copy(TO_STATION); return; }
    const p = toMap('saro', m.x, m.z)!, q = pointAt('B', alongFrom('B', ...foot('B', p[0], p[1])) - AHEAD), [x, z] = fromMap('saro', q[0], q[1])!;
    this.off.set(x - m.x, 0, z - m.z).normalize();
  }
  /** Where they are now (the middle one, in SARO's terms), or null when they are not there. */
  get at() { return this.state === 'none' || !this.base.length ? null : (this.base[1] ?? this.base[0]); }
  private begin(tr: THREE.Vector3, s: LineLightsIn) {
    this.done = true; this.state = 'pace'; this.t = 0; this.lastZ = tr.z;
    this.base = [0, 1, 2].map((i) => { const x = tr.x - 210 - i * 6, z = tr.z + 30 + i * 16; return new THREE.Vector3(x, s.ground(x, z) + 19 + i * 1.5, z); });
    this.pts.visible = true; this.mat.opacity = 0;
    const a = this.pts.geometry.getAttribute('position') as THREE.BufferAttribute;
    this.base.forEach((b, i) => a.setXYZ(i, b.x, b.y, b.z)); a.needsUpdate = true;
  }
  /** A new night or a case opened: whether it has been seen in it. */
  reset(done: boolean) { this.done = done; this.state = 'none'; this.pts.visible = false; }
}
