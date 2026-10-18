import * as THREE from 'three';
import { artLoaded, artTexture, loadArtSome } from './art';
import { eachVariant } from './quality';

// The souvenir shop's cardboard alien, anywhere (Tom, 18 October: it should keep turning up in
// different places, quite at random, all through the game). One cutout, about 1.5 m, on a
// stick. It is put somewhere the player can walk, 6 to 40 m off and out of the view, so it is
// there the next time they turn round: in a corridor, on the pad, behind the diner. Driving,
// it stands on the shoulder ahead of the truck. Once it has been seen and the player has looked
// away, it is gone, and a while later it is somewhere else. It never moves while it is looked at.
// main.ts decides when it may (not in the prologue, a cinematic or the time away).

const rnd = (a: number, b: number) => a + Math.random() * (b - a);

/** The cutout's picture drawn in code, for when ChatGPT's cutout_alien.png (ART_BRIEF.md,
 *  round 14) has not come. Town.ts uses it for event 6 as well. */
export function cutoutTexture() {
  const c = document.createElement('canvas'); c.width = 128; c.height = 256;
  const g = c.getContext('2d')!;
  g.fillStyle = '#8f9a8c';
  g.beginPath(); g.ellipse(64, 58, 44, 54, 0, 0, 7); g.fill();
  g.beginPath(); g.moveTo(52, 104); g.lineTo(76, 104); g.lineTo(84, 150); g.lineTo(80, 240); g.lineTo(70, 240); g.lineTo(64, 172);
  g.lineTo(58, 240); g.lineTo(48, 240); g.lineTo(44, 150); g.closePath(); g.fill();
  g.lineWidth = 7; g.strokeStyle = '#8f9a8c'; g.lineCap = 'round';
  g.beginPath(); g.moveTo(46, 118); g.lineTo(24, 186); g.moveTo(82, 118); g.lineTo(104, 186); g.stroke();
  g.fillStyle = '#0b0b0b';
  g.beginPath(); g.ellipse(44, 62, 15, 8, 0.55, 0, 7); g.ellipse(84, 62, 15, 8, -0.55, 0, 7); g.fill();
  g.fillStyle = 'rgba(60,50,40,.35)'; g.fillRect(0, 196, 128, 4);
  const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace;
  return tex;
}

export interface CutoutsIn {
  ok: boolean;
  cam: THREE.PerspectiveCamera;
  clock: number;
  /** On foot: where the player is, whether a point can be walked, the floor's height there,
   *  and whether a point is inside something (a desk, a wall's collider). */
  foot: { pos: THREE.Vector3; walkable: (x: number, z: number) => boolean; floor: (x: number, z: number) => number; blocked: (x: number, z: number) => boolean } | null;
  /** Driving a road it can stand beside: the truck, its heading, and the ground. */
  drive: { pos: THREE.Vector3; dir: THREE.Vector3; ground: (x: number, z: number) => number } | null;
}

export class Cutouts {
  /** Where it stands now ('foot' or 'shoulder'), for the developer menu and the tests. */
  up: 'foot' | 'shoulder' | null = null;
  /** How many times it has been seen tonight. */
  seen = 0;
  private fig = new THREE.Group();
  private wait = rnd(40, 120);
  private looked = 0;
  private wasSeen = false;
  private away = 0;
  private age = 0;
  private mat: THREE.MeshStandardMaterial;
  private asked = false;

  constructor(scene: THREE.Scene, private note: (first: boolean) => void) {
    // a little of its own paint shows even in the dark, so it reads as cardboard and not as them
    const tex = artLoaded('cutout') ? artTexture('cutout') : cutoutTexture();
    const mat = this.mat = new THREE.MeshStandardMaterial({ map: tex, emissiveMap: tex, emissive: 0xffffff, emissiveIntensity: 0.1, alphaTest: 0.5, side: THREE.DoubleSide, roughness: 1 });
    const card = new THREE.Mesh(new THREE.PlaneGeometry(0.76, 1.52), mat);
    card.position.y = 0.76; card.rotation.y = Math.PI;
    const stick = new THREE.Mesh(new THREE.BoxGeometry(0.05, 0.6, 0.05), new THREE.MeshStandardMaterial({ color: 0x6b5a44, roughness: 1 }));
    stick.position.set(0, 0.3, 0.03);
    this.fig.add(card, stick);
    this.fig.name = 'cutout:wander';
    this.fig.visible = false;
    scene.add(this.fig);
  }

  private view(cam: THREE.PerspectiveCamera, at: THREE.Vector3) {
    const c = cam.getWorldPosition(new THREE.Vector3());
    const mid = at.clone().setY(at.y + 0.8), dist = mid.distanceTo(c);
    const ndc = mid.clone().project(cam);
    return { dist, inView: ndc.z < 1 && Math.abs(ndc.x) < 1.05 && Math.abs(ndc.y) < 1.05 };
  }

  update(dt: number, s: CutoutsIn) {
    if (this.up) {
      this.age += dt;
      const v = this.view(s.cam, this.fig.position);
      // on foot: it stays only on ground the player can still reach (a new area, it is gone)
      const lost = !s.ok || (this.up === 'foot' ? !s.foot || !s.foot.walkable(this.fig.position.x, this.fig.position.z) : !s.drive);
      if (v.inView && v.dist < (this.up === 'shoulder' ? 90 : 32)) {
        this.looked += dt; this.away = 0;
        if (this.looked > 0.6 && !this.wasSeen) { this.wasSeen = true; this.seen++; this.note(this.seen === 1); }
      } else this.away += dt;
      // seen and looked away from, left behind, or never found: gone, when nobody is looking
      const done = (this.wasSeen && this.away > 2.5) || (this.up === 'shoulder' && v.dist > 260 && this.age > 8) || this.age > 300;
      if (lost || (done && !v.inView)) this.hide(rnd(70, 200));
      return;
    }
    if (!s.ok) return;
    // the painted cutout (round 14) is fetched once the night may show it, and put on the card
    if (!this.asked) { this.asked = true; void loadArtSome(['cutout']).then(() => this.paint()); }
    this.wait -= dt;
    if (this.wait > 0) return;
    if (!this.place(s)) this.wait = rnd(4, 10);   // nowhere out of sight just now: soon
  }

  /** Put it somewhere out of the view (the developer menu and the tests call it too). */
  place(s: CutoutsIn) {
    const cam = s.cam, cp = cam.getWorldPosition(new THREE.Vector3());
    let at: THREE.Vector3 | null = null, kind: 'foot' | 'shoulder' = 'foot';
    if (s.drive) {
      // on the shoulder, a good way ahead: the headlights will find it
      const d = s.drive.dir.clone().setY(0).normalize(), side = new THREE.Vector3(-d.z, 0, d.x);
      const p = s.drive.pos.clone().addScaledVector(d, rnd(140, 220)).addScaledVector(side, (Math.random() < 0.5 ? -1 : 1) * rnd(6.5, 8));
      p.y = s.drive.ground(p.x, p.z);
      at = p; kind = 'shoulder';
    } else if (s.foot) {
      const f = s.foot;
      for (let i = 0; i < 60 && !at; i++) {
        const a = Math.random() * Math.PI * 2, r = rnd(6, 40);
        const x = f.pos.x + Math.sin(a) * r, z = f.pos.z + Math.cos(a) * r;
        if (!f.walkable(x, z) || f.blocked(x, z)) continue;
        const p = new THREE.Vector3(x, f.floor(x, z), z);
        if (this.view(cam, p).inView) continue;
        at = p;
      }
    }
    if (!at) return false;
    this.fig.position.copy(at);
    this.fig.rotation.y = Math.atan2(cp.x - at.x, cp.z - at.z) + rnd(-0.35, 0.35);   // more or less facing the player
    this.fig.visible = true; this.up = kind;
    this.looked = 0; this.away = 0; this.age = 0; this.wasSeen = false;
    return true;
  }
  private paint() {
    if (!artLoaded('cutout') || this.mat.map?.name === 'art/cutout') return;
    const old = this.mat.map, tex = artTexture('cutout');
    // on Low the card wears a cheaper copy of the material (core/quality.ts): both get it
    eachVariant(this.mat, (v) => { const m = v as THREE.MeshStandardMaterial; m.map = tex; m.emissiveMap = tex; m.needsUpdate = true; });
    old?.dispose();
  }
  get position() { return this.fig.position; }
  private hide(wait: number) { this.fig.visible = false; this.up = null; this.wait = wait; }
  /** A new night, the title, a save opened. */
  reset() { this.hide(rnd(40, 120)); this.seen = 0; }
}
