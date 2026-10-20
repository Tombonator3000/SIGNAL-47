import * as THREE from 'three';
import { clockText } from './time';

// HENDELSER.md, event 7: three lights, low over the desert west of the highway, keep pace with
// the truck for a stretch south of SARO, quite silent, and then turn off along B and go out.
// They follow the line, not the truck. Once in a night, driving south between SARO and the motel
// in the dark (main.ts decides when it may run). In SARO's terms (x east, z south).

const B = new THREE.Vector3(-0.92, 0, -0.38).normalize();   // line B, west-north-west (abduction.ts)

export interface LineLightsIn {
  ok: boolean;
  /** Driving in SARO's area, and where the truck is. */
  truck: THREE.Vector3 | null;
  clock: number;
  ground: (x: number, z: number) => number;
}

export class LineLights {
  state: 'none' | 'pace' | 'off' = 'none';
  done = false;
  /** T. Vega's field book has them (story/fieldBook.ts): not seen yet, they may come northbound too,
   *  on the way back. */
  anyWay = false;
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
      const dz = tr.z - this.lastZ; this.lastZ = tr.z;
      if ((dz > 0.05 || (this.anyWay && dz < -0.05)) && tr.z > 120 && tr.z < 240) this.begin(tr, s, dz > 0 ? 1 : -1);
      return;
    }
    this.t += dt;
    if (this.state === 'pace') {
      // abreast of the truck, 200 m off to the west, about twenty metres up
      const dz = tr ? tr.z - this.lastZ : 0; if (tr) this.lastZ = tr.z;
      for (const b of this.base) b.z += dz;
      this.mat.opacity = Math.min(0.95, this.t / 2.5);
      if (this.t > 16 || !tr || !s.ok) { this.state = 'off'; this.t = 0; this.note(`${clockText(s.clock, false)}. Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out.`); this.onDone(s.clock); }
    } else {
      for (const b of this.base) b.addScaledVector(B, 38 * dt);
      this.mat.opacity = Math.max(0, 0.95 - this.t / 7);
      if (this.t > 7.5) { this.state = 'none'; this.pts.visible = false; return; }
    }
    const a = this.pts.geometry.getAttribute('position') as THREE.BufferAttribute;
    this.base.forEach((b, i) => a.setXYZ(i, b.x, b.y + Math.sin(this.t * 0.7 + i) * 0.4, b.z));
    a.needsUpdate = true;
  }
  /** `ahead`: +1 driving south, -1 north; the three stand a little ahead of abreast either way. */
  private begin(tr: THREE.Vector3, s: LineLightsIn, ahead = 1) {
    this.done = true; this.state = 'pace'; this.t = 0; this.lastZ = tr.z;
    this.base = [0, 1, 2].map((i) => { const x = tr.x - 210 - i * 6, z = tr.z + ahead * (30 + i * 16); return new THREE.Vector3(x, s.ground(x, z) + 19 + i * 1.5, z); });
    this.pts.visible = true; this.mat.opacity = 0;
    const a = this.pts.geometry.getAttribute('position') as THREE.BufferAttribute;
    this.base.forEach((b, i) => a.setXYZ(i, b.x, b.y, b.z)); a.needsUpdate = true;
  }
  /** A new night or a case opened: whether it has been seen in it. */
  reset(done: boolean) { this.done = done; this.state = 'none'; this.pts.visible = false; this.anyWay = false; }
}
