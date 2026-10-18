import * as THREE from 'three';
import { PHOTO_W, PHOTO_H, PHOTO_FOV, type FieldCamera } from './FieldCamera';
import { Silhouette } from './roadside';
import { clockText } from '../story/time';

// The free camera and the album (todo.md, after They Are Here; SPILLDESIGN.md, point 6). Once the
// field camera is had, the shutter takes a picture of anything, anywhere, away from the case's
// own subjects (Chapter1.check, `near`): the case frames keep their numbers and their sheet film,
// the free frames come out of the camera's instant back, unnumbered, 36 in a night. An exposure
// that was not the player's (story/abduction.ts) is one of the 36 too.
//
// A free frame is the scene rendered once more from the camera, 640 x 400 (320 x 200 where the
// saves have no IndexedDB and every save carries its pictures inline), with the case prints'
// film look. It is kept with the case and shown in the notebook's album (ui/album.ts) with a
// caption in the player's hand: the one or two most prominent of the things main.ts names
// (story/albumSubjects.ts) that are in the frame with a clear line of sight to them, the place
// and the clock. Things that should not be there get a line of their own. And now and then,
// outdoors at night and more often towards morning, the film keeps someone at the edge of the
// frame whom the eye did not see, with a faint pale smear round them: "Nobody was there when I
// took it." At most twice in a night, never in the first three frames. A glimpse the frame
// catches (core/glimpses.ts) gets the same smear and the same line.

export const ROLL = 36;
/** Seconds of the night for an instant print to come up in the album. */
const DEVELOP = 90;
export const NOBODY = 'Nobody was there when I took it.';

export interface AlbumShot { id: string; url: string; clock: number; place: string; text: string; odd?: boolean }
/** What is kept with the case (GameState.album). */
export interface AlbumSave { shots: AlbumShot[]; ghosts: number }
/** The album as the notebook shows it (ui/album.ts). */
export interface AlbumView { roll: number; left: number; shots: { id: string; url: string; text: string; meta: string; dev: number; odd: boolean }[] }

/** Something that can be named under a photograph. */
export interface Subject {
  /** As it is written under the photograph: 'the antennas', 'N. Vega'. */
  name: string;
  /** Where it is (world terms): one point, or several for a group of things (the array). */
  at: THREE.Vector3[];
  /** Its size in metres (default 1), or its angular size in degrees (the sky). */
  r?: number;
  ang?: number;
  /** The farthest it is named from (default 120 m), and how much it counts (default 1). */
  max?: number;
  w?: number;
  /** Things that count as one: only the most prominent of them is named ('the antennas', 'S-03'). */
  group?: string;
  /** A line of its own, always written when it is in the photograph (the things that should not be there). */
  odd?: string;
  /** Its height in metres, for a figure that must show on the print to be written about: under
   *  4 of the print's 400 lines it is a speck, and not named. */
  h?: number;
  /** In the sky: the line of sight is checked only so far out. */
  sky?: boolean;
}

export interface AlbumDeps {
  fcam: FieldCamera;
  scene: THREE.Scene;
  view: { restore: () => void; draw: () => void };
  /** What can be named in a photograph here and now. */
  subjects: () => Subject[];
  place: () => string;
  clock: () => number;
  /** Exposures on the roll that were not the player's. */
  extra: () => number;
  /** Before the render (the antennas culled for the photo camera). */
  before: () => void;
  /** The ground's height where the film may keep someone, or null (not here, not now). */
  ghostGround: () => ((x: number, z: number) => number) | null;
  /** The saves have no IndexedDB: smaller pictures. */
  small: () => boolean;
  /** Not in the way of a line of sight (the sky). */
  skip: () => THREE.Object3D[];
  /** The shutter's sound and flash, a word on the roll, the save. */
  done: (shot: AlbumShot, first: boolean, left: number) => void;
}

const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const MIN_SCORE = 0.05;
type Seen = { s: Subject; p: THREE.Vector3; score: number };

export class Album {
  data: AlbumSave = { shots: [], ghosts: 0 };
  /** Tests: the next frame keeps the figure, where there is ground for it. */
  forceGhost = false;
  /** Tests: what the last caption was made from, how bright the frame was, how long it took. */
  last: { names: string[]; odd: string[]; mean: number; ms: number; lookMs: number; ghost: boolean } | null = null;
  private sil: Silhouette;
  private ghost: THREE.Group;
  private halos: THREE.Mesh[] = [];
  private rc = new THREE.Raycaster();

  constructor(private d: AlbumDeps) {
    // the same figure as the glimpses (core/glimpses.ts), with a little more of a rim: the film sees more than the eye
    this.sil = new Silhouette('album:ghost', 0x221a14);
    this.ghost = this.sil.group;
    this.ghost.visible = false;
    d.scene.add(this.ghost);
    // and round a figure the film keeps a faint pale smear, the way a long exposure keeps a light:
    // black on the black of the night it would not be seen on the print at all
    const tex = haloTexture();
    for (let i = 0; i < 2; i++) {
      const m = new THREE.Mesh(new THREE.PlaneGeometry(1.5, 2.3), new THREE.MeshBasicMaterial({ map: tex, color: 0xb4bcc4, transparent: true, opacity: 0.3, blending: THREE.AdditiveBlending, depthWrite: false }));
      m.name = 'album:halo'; m.visible = false; m.frustumCulled = false;
      d.scene.add(m); this.halos.push(m);
    }
  }

  /** The smears behind the figures in this frame (the glimpse, the one on the film only), and the
   *  rest far under the ground: drawn in every free frame all the same. */
  private placeHalos(figs: THREE.Vector3[]) {
    const cam = this.d.fcam.photoCam;
    this.halos.forEach((h, i) => {
      const f = figs[i];
      if (f) h.position.copy(f).setY(f.y + 0.62).addScaledVector(f.clone().sub(cam.position).setY(0).normalize(), 0.35);
      else h.position.set(cam.position.x, -9000, cam.position.z);
      h.quaternion.copy(cam.quaternion);
    });
  }

  get shots() { return this.data.shots; }
  /** Free frames left on the roll tonight. */
  get left() { return Math.max(0, ROLL - this.data.shots.length - this.d.extra()); }

  /** Take a free frame now (Chapter1.shutter, when the case has no frame to take here). */
  expose() {
    const d = this.d, fcam = d.fcam, t0 = performance.now();
    this.sil.wake();
    fcam.sync();
    const clock = d.clock(), place = d.place(), n = this.data.shots.length;
    // someone at the edge of the frame, on the film only
    const day = ((clock % 86400) + 86400) % 86400, k = THREE.MathUtils.clamp((day - 2.5 * 3600) / (2.5 * 3600), 0, 1);
    const ground = this.data.ghosts < 2 && (this.forceGhost || (n >= 3 && Math.random() < 0.05 + 0.12 * k)) ? d.ghostGround() : null;
    this.forceGhost = false;
    const ghostAt = ground ? this.placeGhost(ground) : null;
    const list = d.subjects();
    if (ghostAt) list.push({ name: 'someone on the film only', at: [ghostAt.clone().setY(ghostAt.y + 0.7)], r: 0.45, max: 60, odd: NOBODY, h: 1.25 });
    const seen = this.inFrame(list), t1 = performance.now();
    const small = d.small();
    // the figure and the smears are drawn in every free frame, far under the ground when they are
    // not in this one, so the frame that keeps one costs no more than any other (no stutter to give it away)
    if (!ghostAt) this.ghost.position.set(fcam.photoCam.position.x, -9000, fcam.photoCam.position.z);
    const glimpse = seen.some((x) => x.s.odd === NOBODY && x.s.name === 'someone') ? d.scene.getObjectByName('glimpse') : null;
    this.placeHalos([ghostAt, glimpse?.visible ? glimpse.position : null].filter((p): p is THREE.Vector3 => !!p));
    const { canvas, mean } = fcam.snap({
      before: () => { d.before(); this.ghost.visible = true; this.halos.forEach((h) => { h.visible = true; }); },
      after: () => { this.ghost.visible = false; this.halos.forEach((h) => { h.visible = false; }); },
      restore: d.view.restore, seed: Math.floor(clock * 1000) + n * 7919, w: small ? 320 : 640,
    });
    d.view.draw();
    const url = canvas.toDataURL('image/jpeg', small ? 0.62 : 0.74);
    const odd = [...new Set(seen.filter((x) => x.s.odd).map((x) => x.s.odd!))];
    const names = seen.filter((x) => !x.s.odd).map((x) => x.s.name);
    if (ghostAt && odd.includes(NOBODY)) this.data.ghosts++;
    // "S-03 and B-12.", "Halley's comet, low in the south, and the truck."
    const main = names.length ? names.join(names[0].includes(',') ? ', and ' : ' and ') + '.' : mean < 15 && !odd.length ? 'Too dark. Nothing came out.' : '';
    const text = [main.charAt(0).toUpperCase() + main.slice(1), ...odd].filter(Boolean).join(' ');
    const shot: AlbumShot = { id: `free${n + 1}@${Math.round(clock)}`, url, clock, place, text };
    if (odd.length) shot.odd = true;
    this.data.shots.push(shot);
    this.last = { names, odd, mean, ms: performance.now() - t0, lookMs: t1 - t0, ghost: !!ghostAt };
    d.done(shot, n === 0, this.left);
    return shot;
  }

  /** What a frame taken now would name, without taking it (the tests). */
  peek() {
    this.d.fcam.sync();
    const seen = this.inFrame(this.d.subjects());
    return { names: seen.filter((x) => !x.s.odd).map((x) => x.s.name), odd: seen.filter((x) => x.s.odd).map((x) => x.s.odd!) };
  }

  /** A save opened, or a new night. */
  restore(v: AlbumSave | null | undefined) {
    this.data = v ? { shots: [...(v.shots ?? [])], ghosts: v.ghosts ?? 0 } : { shots: [], ghosts: 0 };
    this.ghost.visible = false;
  }

  /** For the notebook: each print, how far it has come up by now. */
  view(clock: number): AlbumView {
    return {
      roll: ROLL, left: this.left,
      shots: this.data.shots.map((s) => ({ id: s.id, url: s.url, text: s.text, meta: `${s.place}, ${clockText(s.clock, false)}`,
        dev: THREE.MathUtils.clamp((clock - s.clock) / DEVELOP, 0, 1), odd: !!s.odd })),
    };
  }

  // ---------- what is in the frame ----------
  // Every point of every subject in the photograph and near enough is scored by how big it looks
  // and how near the middle it is; then, most prominent first, a line of sight is looked for to
  // at most three of its points. Two things are named at most; the odd ones are always looked for.
  private inFrame(list: Subject[]): Seen[] {
    const fcam = this.d.fcam, c = fcam.photoCam.position;
    const by: Seen[][] = [];
    for (const s of list) {
      const pts: Seen[] = [];
      for (const p of s.at) {
        const dist = p.distanceTo(c);
        if (dist > (s.max ?? 120) || dist < 0.25) continue;
        if (s.h && s.h / dist * 400 / (2 * Math.tan(PHOTO_FOV * Math.PI / 360)) < 4) continue;
        const q = fcam.project(p);
        if (!q.front || q.u < 0.01 || q.u > 0.99 || q.v < 0.01 || q.v > 0.99) continue;
        const ang = s.ang ?? THREE.MathUtils.radToDeg(Math.atan((s.r ?? 1) / dist));
        const off = Math.min(1, Math.hypot((q.u - 0.5) * PHOTO_W / PHOTO_H, q.v - 0.5) / 0.9);
        pts.push({ s, p, score: (s.w ?? 1) * Math.min(1, ang / 8) * (1 - 0.5 * off) });
      }
      if (pts.length) by.push(pts.sort((a, b) => b.score - a.score));
    }
    by.sort((a, b) => b[0].score - a[0].score);
    const out: Seen[] = [], groups = new Set<string>(), names = new Set<string>();
    const place = this.d.place().toLowerCase();
    let rays = 0;
    for (const pts of by) {
      const s = pts[0].s;
      if (!s.odd) {
        if (out.filter((x) => !x.s.odd).length >= 2 || rays >= 12 || pts[0].score < MIN_SCORE) continue;
        if ((s.group && groups.has(s.group)) || names.has(s.name) || place.includes(s.name.toLowerCase())) continue;
      }
      for (const cd of pts.slice(0, 3)) {
        rays++;
        if (this.blocked(c, cd.p, s.r ?? 1, s.sky ? 1500 : Infinity)) continue;
        out.push(cd);
        if (s.group) groups.add(s.group);
        names.add(s.name);
        break;
      }
    }
    return out;
  }

  /** Is anything solid and shown between `from` and `to`? Stops short of the thing itself. */
  private blocked(from: THREE.Vector3, to: THREE.Vector3, r: number, cap = Infinity) {
    const dir = to.clone().sub(from), len = Math.min(dir.length(), cap);
    this.rc.set(from, dir.normalize());
    this.rc.near = 0.2; this.rc.far = Math.max(0.25, len - Math.max(0.5, r * 1.1));
    const skip = new Set(this.d.skip()), hits: THREE.Intersection[] = [], rc = this.rc;
    const solid = (m: THREE.Material | THREE.Material[]) => {
      const x = Array.isArray(m) ? m[0] : m;
      return !!x && x.visible !== false && !(x.transparent && x.opacity < 0.5) && x.blending !== THREE.AdditiveBlending;
    };
    // three.js would test hidden objects too (memory.md): only what is shown is in the way
    const walk = (o: THREE.Object3D): boolean => {
      if (!o.visible || skip.has(o)) return false;
      const m = o as THREE.Mesh;
      if (m.isMesh && o.layers.test(rc.layers) && solid(m.material)) { m.raycast(rc, hits); if (hits.length) return true; }
      for (const ch of o.children) if (walk(ch)) return true;
      return false;
    };
    return walk(this.d.scene);
  }

  // ---------- on the film only ----------
  /** A place 14 to 34 m off near the left or right edge of the frame, on the ground, in plain sight of the camera. */
  private placeGhost(ground: (x: number, z: number) => number) {
    const fcam = this.d.fcam, cam = fcam.photoCam, c = cam.position;
    for (let i = 0; i < 30; i++) {
      const side = Math.random() < 0.5 ? -1 : 1;
      const dir = new THREE.Vector3(side * rnd(0.6, 0.86), 0, 0.5).unproject(cam).sub(c).setY(0).normalize();
      const p = c.clone().addScaledVector(dir, rnd(14, 34));
      p.y = ground(p.x, p.z);
      if (!Number.isFinite(p.y) || Math.abs(p.y - (c.y - 1.6)) > 5) continue;
      const feet = fcam.project(p), head = fcam.project(p.clone().setY(p.y + 1.25));
      if (!feet.front || feet.v > 0.97 || head.v < 0.03 || feet.u < 0.03 || feet.u > 0.97) continue;
      if (this.blocked(c, p.clone().setY(p.y + 0.7), 0.4)) continue;
      this.ghost.position.copy(p);
      this.ghost.rotation.y = Math.atan2(c.x - p.x, c.z - p.z);   // facing the camera
      this.ghost.updateMatrixWorld(true);
      return p;
    }
    return null;
  }
}

/** A soft pale oval, brightest a little above its middle (the smear the film keeps round a figure). */
function haloTexture() {
  const c = document.createElement('canvas'); c.width = 64; c.height = 96;
  const g = c.getContext('2d')!;
  const r = g.createRadialGradient(32, 44, 2, 32, 48, 46);
  r.addColorStop(0, 'rgba(255,255,255,.9)'); r.addColorStop(0.45, 'rgba(255,255,255,.35)'); r.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = r; g.fillRect(0, 0, 64, 96);
  const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace;
  return tex;
}

/** The kit's line for the camera (main.ts). */
export const rollText = (left: number) => `${left} of ${ROLL} free frames left`;
