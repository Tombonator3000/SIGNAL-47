import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { facing } from './seen';
import { glowSprite } from '../core/textures';

// The recording (FORSLAG.md, 20 October, item 9 B; HENDELSER.md, section 8: "they take a recording
// of you, and every recording stays"). Where the player was taken, someone in a SARO jacket stands
// for the rest of the night (story/abduction.ts, record). The game keeps the player's last 47
// seconds there, as they were lived: where they went, every quarter of a second, and which way
// they looked (AbductSave.spots[].path). Once, later, when the player is back within sight of the
// place and the night is building or at its peak, the one in the jacket walks them: from where the
// player stood when the crickets stopped, along the same way, stopping where they stopped, running
// the same stumbling run, turning to look where they looked, and standing still at the end for the
// three seconds the light took. Then it is not there, and it starts over where it began. R5 on the
// player's own body: it repeats what was registered and does not answer. Seen from a distance only
// (it is not there within 45 m, as the one standing is not), silent, and now and then the tape it
// is on drops a few frames and catches up.
//
// One at a time (main.ts, oneAtATime): it waits for anything else of the night's, and nothing
// starts while it walks. The night's curve (story/curve.ts) lets it come in a build from half way
// or at a peak, never in a rest; where one waits for the player within sight, the curve hurries
// for it the way it does for the motel's television. Written in the night's log (story/seen.ts)
// as 'walk 1' (the first time away's), and it lands the peak once it is looked at: looked at when it
// can be made out, its band lit by the flashlight or the headlights, or within 60 m (a dark shape on
// dark ground at 150 m has not been seen, however the eyes are turned). Far off, what carries is the
// tape: in a beam it gives the light back as a glint a few pixels across on any screen, which moves
// as the one wearing it moves; zoomed, it is someone in a SARO jacket. Kept with the case (the spot's
// `walked`): it never walks again, after Continue either; the one standing stays. Only a walk the
// player went away from before ever making it out (drove on, say) comes again, once, another time
// they are back within sight of the place.

/** The path is sampled this often (seconds), x, z and the yaw of the view each time. */
export const PATH_DT = 0.25;
/** It walks no nearer than this (as the one standing: story/abduction.ts) and comes when the player
 *  is between these from the place (metres). */
export const NEAR = 45, FROM_D = 55, TO_D = 180, GIVE_UP = 450;
/** At the end, standing still as long as the light took; then not there for a moment. */
const STAND = 3, GONE = 1.6;
/** It can be made out (counts as looked at): the band lit by a beam from near the eye this much,
 *  or this near (metres), against whatever is behind it. */
const CLEAR_LIT = 0.25, CLEAR_D = 60;
/** A walk the player went away from before ever making it out comes again, this many times in all. */
const TRIES = 2;
/** Less than this much ground covered in the 47 s, and it would only stand: it does not walk. */
const MOVED = 5;

const _lit = new THREE.Color(), _at = new THREE.Vector3(), _to = new THREE.Vector3(), _eye = new THREE.Vector3();
let _glint: THREE.Texture | null = null;
/** Far off, the band's glint keeps about the same few pixels on any screen, as tape in a beam does
 *  (this much of its size per metre away and per unit of the view's half height; at the usual view
 *  of 70 degrees, its own size at 28 m), and no less than its own size near. Zoomed, it stays the
 *  size it was, and the one wearing it comes up round it. */
const GLINT_FAR = 1 / (28 * Math.tan(THREE.MathUtils.degToRad(35)));
/** Someone in a SARO jacket with the reflective band across it, jointed at the hips and shoulders
 *  so it can walk (the same boxes and colours as the one standing has always had). Built facing -z,
 *  the way the player faces at yaw 0, from the feet. */
export class Walker {
  readonly group = new THREE.Group();
  private body = new THREE.Group();
  private legs: THREE.Group[] = [];
  private arms: THREE.Group[] = [];
  private head: THREE.Mesh;
  private band: THREE.MeshBasicMaterial;
  private coat: THREE.MeshBasicMaterial;
  private dark: THREE.MeshBasicMaterial;
  /** The glint of the tape in a light from near the eye: across the chest and round each sleeve. */
  private glints: THREE.Sprite[] = [];
  private glintMat: THREE.SpriteMaterial;
  /** The band's boxes the glints sit on: the chest's, then the sleeves'. */
  private tapes: THREE.Mesh[] = [];
  private lit = 0;
  /** How much light from near the eye is on the band now (0 to 1). */
  get shone() { return this.lit; }
  constructor(name: string) {
    const coat = this.coat = new THREE.MeshBasicMaterial({ color: 0x2a3036 }), dark = this.dark = new THREE.MeshBasicMaterial({ color: 0x15171a });
    const band = this.band = new THREE.MeshBasicMaterial({ color: 0xd8dccc }), skin = new THREE.MeshBasicMaterial({ color: 0x6e5a4c });
    const box = (p: THREE.Object3D, w: number, h: number, d: number, m: THREE.Material, x: number, y: number, z: number) => {
      const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), m); b.position.set(x, y, z); p.add(b); return b;
    };
    for (const sd of [-1, 1]) {
      const hip = new THREE.Group(); hip.position.set(sd * 0.11, 0.84, 0); this.body.add(hip);
      box(hip, 0.2, 0.84, 0.2, dark, 0, -0.42, 0);
      this.legs.push(hip);
      const sh = new THREE.Group(); sh.position.set(sd * 0.32, 1.42, 0); this.body.add(sh);
      box(sh, 0.12, 0.6, 0.14, coat, 0, -0.3, 0);
      // (and round each sleeve, as the field jackets have it: it swings with the arm)
      this.tapes.push(box(sh, 0.13, 0.05, 0.15, band, 0, -0.24, 0));
      this.arms.push(sh);
    }
    box(this.body, 0.5, 0.62, 0.28, coat, 0, 1.14, 0); this.tapes.unshift(box(this.body, 0.52, 0.08, 0.3, band, 0, 1.2, 0));
    this.head = new THREE.Mesh(new THREE.SphereGeometry(0.13, 10, 8), skin); this.head.position.y = 1.6; this.body.add(this.head);
    // the tape gives a beam back as a glint that carries much further than the jacket can be made
    // out: across the chest, and a small one at each sleeve that swings with the arm
    this.glintMat = new THREE.SpriteMaterial({ map: _glint ??= glowSprite(), color: 0xfff1da, blending: THREE.AdditiveBlending,
      transparent: true, depthWrite: false, fog: false, opacity: 0 });
    // (each is put on the side of its tape that faces the eye, so the jacket does not hide it)
    for (let i = 0; i < 3; i++) { const s = new THREE.Sprite(this.glintMat); s.position.set(0, i ? 1.18 : 1.2, 0); s.visible = false; s.renderOrder = 3; this.group.add(s); this.glints.push(s); }
    this.group.add(this.body);
    this.group.name = name;
    this.group.traverse((x) => { x.frustumCulled = false; });
  }
  /** The reflective band in a light from near the eye (the flashlight, the headlights): it gives the
   *  light back the way it came, as the tape does, 0 to 1. `cam`: the eye the light is beside (the
   *  glint keeps a few pixels across however far off it is). */
  shine(k: number, cam?: THREE.Camera) {
    if (Math.abs(k - this.lit) >= 0.02 || (k === 0 && this.lit !== 0)) {
      this.lit = k;
      this.band.color.setRGB(0.69 + 2.2 * k, 0.72 + 2.2 * k, 0.6 + 1.9 * k);
      // (and the jacket and the jeans a little out of the dark in the beam, enough to be someone)
      this.coat.color.setHex(0x2a3036).lerp(_lit.setHex(0x737a82), k);
      this.dark.color.setHex(0x15171a).lerp(_lit.setHex(0x464a52), k);
      this.glintMat.opacity = 0.9 * Math.min(1, 1.15 * k) ** 1.5;
    }
    const on = this.lit > 0.04;
    for (const g of this.glints) g.visible = on;
    if (!on) return;
    const eye = cam ? cam.getWorldPosition(_eye) : null;
    const half = cam && (cam as THREE.PerspectiveCamera).isPerspectiveCamera ? Math.tan(THREE.MathUtils.degToRad((cam as THREE.PerspectiveCamera).fov / 2)) : 0.7;
    const s = Math.max(1, (eye ? eye.distanceTo(this.group.position) : 40) * half * GLINT_FAR);
    this.glints[0].scale.set(0.7 * s, 0.16 * s, 1);
    this.glints[1].scale.setScalar(0.18 * s); this.glints[2].scale.setScalar(0.18 * s);
    if (!eye) return;
    this.group.updateMatrixWorld(true);
    this.tapes.forEach((t, i) => {
      t.getWorldPosition(_at);
      _to.copy(eye).sub(_at).setY(0).normalize();
      this.group.worldToLocal(_at.addScaledVector(_to, 0.3));
      this.glints[i].position.copy(_at);
    });
  }
  /** Standing still, as it has always stood. */
  stand() { this.pose(0, 0, 0, 0, 0); }
  /** The one standing as one piece of geometry, so it costs one draw (story/theThird.ts): the same
   *  boxes and colours, standing still, facing -z from the feet. Each corner has its colour
   *  (`color`, linear), `aBand` 1 on the tape (the chest and the sleeves), and `aShade`, how much a
   *  light from behind it (+z) falls there: the back full, the shoulders and the sides some, the
   *  front none. From behind the head is hair, not skin. */
  static baked(): THREE.BufferGeometry {
    const w = new Walker('baked');
    w.stand();
    // (standing as a person stands, not a post: the feet a little apart, the arms off the sides, so
    // it is someone and not a box at 9 m)
    w.legs[0].rotation.z = -0.05; w.legs[1].rotation.z = 0.05;
    w.arms[0].rotation.z = -0.1; w.arms[1].rotation.z = 0.1;
    // and the head a little down and forward, as if it looked at the ground in front of it
    w.head.position.set(0, 1.575, -0.045);
    w.group.updateMatrixWorld(true);
    const hair = new THREE.Color(0x221b15), _n = new THREE.Vector3(), _p = new THREE.Vector3();
    const parts: THREE.BufferGeometry[] = [];
    w.body.traverse((o) => {
      const m = o as THREE.Mesh;
      if (!m.isMesh) return;
      const tape = w.tapes.includes(m), head = m === w.head;
      const src = (m.geometry.index ? m.geometry.toNonIndexed() : m.geometry.clone());
      const pos = src.getAttribute('position') as THREE.BufferAttribute, nor = src.getAttribute('normal') as THREE.BufferAttribute;
      const n = pos.count, col = new Float32Array(n * 3), band = new Float32Array(n), shade = new Float32Array(n);
      const base = (m.material as THREE.MeshBasicMaterial).color;
      for (let i = 0; i < n; i++) {
        _p.fromBufferAttribute(pos, i); _n.fromBufferAttribute(nor, i);
        const c = head && (_p.z > -0.045 || _p.y > 0.075) ? hair : base;
        col[i * 3] = c.r; col[i * 3 + 1] = c.g; col[i * 3 + 2] = c.b;
        band[i] = tape ? 1 : 0;
        shade[i] = Math.min(1, 0.22 + 0.78 * Math.max(0, _n.z) + 0.3 * Math.max(0, _n.y) + 0.12 * Math.abs(_n.x));
      }
      src.setAttribute('color', new THREE.BufferAttribute(col, 3));
      src.setAttribute('aBand', new THREE.BufferAttribute(band, 1));
      src.setAttribute('aShade', new THREE.BufferAttribute(shade, 1));
      src.deleteAttribute('uv');
      src.applyMatrix4(m.matrixWorld);
      parts.push(src);
    });
    const geo = mergeGeometries(parts)!;
    parts.forEach((p) => p.dispose());
    // (the walker it was baked from is not kept)
    w.group.traverse((o) => { const m = o as THREE.Mesh; if (m.isMesh) m.geometry.dispose(); });
    for (const m of [w.band, w.coat, w.dark, w.glintMat]) m.dispose();
    (w.head.material as THREE.Material).dispose();
    geo.computeBoundingSphere();
    return geo;
  }
  /** How it is carried: the stride (-1..1 through a step), how wide (0 standing, 1 a full run's),
   *  how far it leans forward and to the side (radians), and how high the body is lifted (metres). */
  pose(swing: number, wide: number, lean: number, roll: number, lift: number) {
    const leg = wide * 0.62, arm = wide * 0.5;
    this.legs[0].rotation.x = swing * leg; this.legs[1].rotation.x = -swing * leg;
    this.arms[0].rotation.x = -swing * arm; this.arms[1].rotation.x = swing * arm;
    // (a hand out a little from the side when it runs)
    this.arms[0].rotation.z = -0.08 * wide; this.arms[1].rotation.z = 0.08 * wide;
    this.body.rotation.set(-lean, 0, roll);
    this.body.position.y = lift;
  }
}

/** A path as kept with the case: x, z and yaw every PATH_DT seconds. */
export type Path = number[];
const lerpAngle = (a: number, b: number, k: number) => a + Math.atan2(Math.sin(b - a), Math.cos(b - a)) * k;
/** How long a path runs (seconds). */
export const pathLen = (p: Path) => Math.max(0, (Math.floor(p.length / 3) - 1) * PATH_DT);
/** Where on the path at t seconds (smoothed between the samples), and the yaw of the view then. */
export function pathAt(p: Path, t: number, out: THREE.Vector3) {
  const n = Math.floor(p.length / 3);
  if (n < 1) { out.set(0, 0, 0); return 0; }
  const f = THREE.MathUtils.clamp(t / PATH_DT, 0, n - 1), i = Math.min(n - 2, Math.floor(f)), k = n > 1 ? f - i : 0;
  const at = (j: number, c: number) => p[Math.max(0, Math.min(n - 1, j)) * 3 + c];
  // Catmull-Rom through the quarter seconds, so a turn is a turn and not a corner
  const cr = (c: number) => {
    const p0 = at(i - 1, c), p1 = at(i, c), p2 = at(i + 1, c), p3 = at(i + 2, c), k2 = k * k, k3 = k2 * k;
    return 0.5 * (2 * p1 + (-p0 + p2) * k + (2 * p0 - 5 * p1 + 4 * p2 - p3) * k2 + (-p0 + 3 * p1 - 3 * p2 + p3) * k3);
  };
  out.set(cr(0), 0, cr(1));
  return lerpAngle(at(i, 2), at(i + 1, 2), k);
}
/** How much ground the path covers (metres). */
export function pathMoved(p: Path) {
  let d = 0;
  for (let i = 3; i + 2 < p.length; i += 3) d += Math.hypot(p[i] - p[i - 3], p[i + 1] - p[i - 2]);
  return d;
}

/** One of the times away, as the walk sees it (story/abduction.ts, recording). */
export interface Rec { n: number; scene: string; take: THREE.Vector3; path: Path | undefined; walked: number | undefined; clock: number; fig: Walker;
  /** Kept with the case: it has walked (the night's clock then), or not after all (undefined). */
  markWalked: (clock: number | undefined) => void }
export interface RecordingDeps {
  /** The recordings there are (one for each time away). */
  recs: () => Rec[];
  /** The walk has the one standing for this time away (held) or gives it back (the one standing
   *  comes back where it stands, once that place is out of the view). */
  hold: (n: number, held: boolean) => void;
  ground: (x: number, z: number) => number;
  camera: THREE.Camera;
  /** Write it down: the notebook, the case. */
  note: (text: string) => void;
  clockText: (c: number) => string;
  save: () => void;
  /** How much a light from near the eye falls on a point (the band gives it back), 0 to 1. */
  shine?: (at: THREE.Vector3) => number;
}
export interface RecordingIn {
  /** Chapters three to five, playing, no panel, no cinematic. */
  ok: boolean;
  /** The night's curve lets a scare come, and nothing else of the night's is going. */
  may: boolean;
  /** The area the player is in (rings and recordings belong to one), and where they are. */
  scene: string;
  pos: THREE.Vector3;
  /** Out where it could be seen: on foot outdoors, or in the truck (not inside a building). */
  out: boolean;
  clock: number;
}
type Phase = 'idle' | 'walk' | 'stand' | 'gone';

export class RecordingWalk {
  phase: Phase = 'idle';
  /** Which time away is walking (1 to 3), or 0. */
  n = 0;
  /** How it ended the last time: 'near' (walked up to it), 'left' (the player went away) or 'done'. */
  ended: string | null = null;
  /** How many times it has started over, and the seconds it was looked at. */
  loops = 0;
  looked = 0;
  /** A recording that could walk is within sight of the player now (the curve hurries for it). */
  waiting = false;
  private rec: Rec | null = null;
  private t = 0;
  private st = 0;
  private shown = 0;
  private hitch = 0;
  private nextHitch = 8;
  private phaseStep = 0;
  private yaw = 0;
  private prev = new THREE.Vector3();
  private v = new THREE.Vector3();
  private w = new THREE.Vector3();
  private head = new THREE.Vector3();
  private eye = new THREE.Vector3();
  private noted = false;
  private sawRestart = false;
  /** It can be made out from where the player is now (lit, or near enough). */
  private clear = false;
  /** How many times each time away has begun to walk tonight (n). */
  private tries = new Map<number, number>();

  constructor(private d: RecordingDeps) {}

  get busy() { return this.phase !== 'idle'; }
  /** Where it is now, at the head, while it is there and can be made out (the log watches this:
   *  a dark shape on dark ground at 150 m has not been seen, however the eyes are turned). */
  get at(): THREE.Vector3 | null { return this.rec && this.rec.fig.group.visible && this.clear ? this.head : null; }
  /** Where it is, at the head, whether or not it can be made out (null while it is not walking). */
  get spot(): THREE.Vector3 | null { return this.rec ? this.head : null; }
  /** The one that would walk if it could, from where the player is now. */
  candidate(s: RecordingIn): Rec | null {
    for (const r of this.d.recs()) {
      if (r.walked !== undefined || !r.path || r.scene !== s.scene || r.path.length < 36 || pathMoved(r.path) < MOVED) continue;
      const dist = Math.hypot(r.take.x - s.pos.x, r.take.z - s.pos.z);
      if (dist >= FROM_D && dist <= TO_D) return r;
    }
    return null;
  }

  update(dt: number, s: RecordingIn) {
    if (this.phase === 'idle') {
      const c = s.ok && s.out ? this.candidate(s) : null;
      this.waiting = !!c;
      if (!c || !s.may) return;
      // it comes where the player is not looking: they turn, and someone is walking there
      pathAt(c.path!, 0, this.v);
      this.v.y = this.d.ground(this.v.x, this.v.z) + 1.2;
      if (facing(this.d.camera, this.v, 600, 40) || Math.hypot(this.v.x - s.pos.x, this.v.z - s.pos.z) <= NEAR) return;
      this.begin(c, s.clock);
      return;
    }
    this.waiting = false;
    const r = this.rec!;
    // (held while a cinematic or the like has the player; it does not end for it)
    if (!s.ok && s.scene === r.scene) return;
    if (s.scene !== r.scene || Math.hypot(r.take.x - s.pos.x, r.take.z - s.pos.z) > GIVE_UP) { this.end('left'); return; }
    const len = pathLen(r.path!);
    if (this.phase === 'walk') {
      this.t += dt;
      // now and then the tape it is on drops a few frames, and catches up
      if (this.hitch > 0) this.hitch -= dt;
      else if ((this.nextHitch -= dt) <= 0) { this.hitch = 0.12 + Math.random() * 0.18; this.nextHitch = 6 + Math.random() * 5; }
      if (this.hitch <= 0) this.shown = this.t;
      if (this.t >= len) { this.phase = 'stand'; this.st = 0; this.shown = len; }
    } else if (this.phase === 'stand') {
      if ((this.st += dt) >= STAND) { this.phase = 'gone'; this.st = 0; }
    } else if ((this.st += dt) >= GONE) {
      // and it starts over, where the player was when the crickets stopped
      this.loops++;
      if (this.looked > 0.5) this.sawRestart = true;
      // twice through; a third time if it was never looked at
      if (this.loops >= (this.looked > 0.5 ? 2 : 3)) { this.end('done'); return; }
      this.phase = 'walk'; this.t = 0; this.shown = 0; this.st = 0;
      pathAt(r.path!, 0, this.prev);
    }
    this.place(dt, r);
    const g = r.fig.group;
    const near = Math.hypot(g.position.x - s.pos.x, g.position.z - s.pos.z) <= NEAR;
    // walked up to: it was not there
    if (near && this.phase !== 'gone') { this.end('near'); return; }
    g.visible = this.phase !== 'gone';
    if (g.visible && this.clear && facing(this.d.camera, this.head, 320, 18)) this.looked += dt;
  }

  /** Where it is on the path and how it is carried there. */
  private place(dt: number, r: Rec) {
    const g = r.fig.group, p = r.path!;
    const viewYaw = pathAt(p, this.shown, this.v);
    pathAt(p, Math.min(pathLen(p), this.shown + 0.15), this.w);
    const ahead = this.w.clone().sub(this.v);
    pathAt(p, Math.max(0, this.shown - 0.15), this.w);
    const vel = this.v.clone().sub(this.w).add(ahead).divideScalar(0.3);
    const speed = this.phase === 'walk' ? Math.hypot(vel.x, vel.z) : 0;
    // (the player walks at 1.9 m/s and runs at 3: player/Player.ts)
    const run = THREE.MathUtils.smoothstep(speed, 2.0, 2.9);
    // facing the way it goes while it goes, else the way the player was looking then
    const want = speed > 0.45 ? Math.atan2(-vel.x, -vel.z) : viewYaw;
    const turn = Math.min(1, dt * (speed > 0.45 ? 7 : 3.5));
    this.yaw = lerpAngle(this.yaw, want, turn);
    // the stride from the ground covered (frozen with the tape)
    if (this.hitch <= 0) this.phaseStep += Math.hypot(this.v.x - this.prev.x, this.v.z - this.prev.z) / THREE.MathUtils.lerp(1.3, 2.2, run) * Math.PI * 2;
    this.prev.copy(this.v);
    const wide = Math.min(1, speed / 1.1) * THREE.MathUtils.lerp(0.62, 1, run);
    const sw = Math.sin(this.phaseStep);
    // the stumbling run: forward over the feet, and off balance now and then
    const tt = this.shown;
    const lurch = run * Math.max(0, Math.sin(tt * 1.7 + 1.3 * Math.sin(tt * 0.37))) ** 3;
    const lean = 0.03 + 0.17 * run + 0.12 * lurch;
    const roll = run * (0.06 * Math.sin(tt * 2.3 + 2 * Math.sin(tt * 0.71)) + 0.05 * lurch * Math.sign(Math.sin(tt * 0.9)));
    r.fig.pose(sw, wide, lean, roll, Math.abs(Math.cos(this.phaseStep)) * 0.035 * wide);
    if (this.phase !== 'walk') r.fig.stand();
    g.position.set(this.v.x, this.d.ground(this.v.x, this.v.z), this.v.z);
    g.rotation.set(0, this.yaw, 0);
    this.head.set(this.v.x, g.position.y + 1.45, this.v.z);
    this.w.set(this.v.x, g.position.y + 1.2, this.v.z);
    const eye = this.d.camera.getWorldPosition(this.eye);
    r.fig.shine(this.d.shine?.(this.w) ?? 0, this.d.camera);
    this.clear = r.fig.shone >= CLEAR_LIT || Math.hypot(eye.x - this.v.x, eye.z - this.v.z) <= CLEAR_D;
  }

  /** It walks now (the curve and one-at-a-time have said so; the developer menu, the tests). */
  begin(r: Rec, clock: number) {
    this.rec = r; this.n = r.n; this.phase = 'walk'; this.t = 0; this.shown = 0; this.st = 0; this.loops = 0; this.looked = 0;
    this.hitch = 0; this.nextHitch = 5 + Math.random() * 4; this.phaseStep = 0; this.noted = false; this.sawRestart = false; this.ended = null;
    pathAt(r.path!, 0, this.prev);
    this.yaw = r.path![2];
    this.d.hold(r.n, true);
    this.tries.set(r.n, (this.tries.get(r.n) ?? 0) + 1);
    r.walked = Math.round(clock); r.markWalked(r.walked);
    this.place(0, r);
    r.fig.group.visible = true;
    this.d.save();
  }
  private end(why: string) {
    const r = this.rec!;
    // seen walking for a while: written down, in the player's own words
    if (!this.noted && this.looked >= 2.5) {
      this.noted = true;
      this.d.note(`Someone in a SARO jacket, out where I was taken at ${this.d.clockText(r.clock)}. It went the way I went in those 47 seconds: it stopped where I stopped, ran where I ran, looked where I looked, and stood still at the end.${this.sawRestart ? ' Then it was not there, and it was back where I had been when the crickets stopped, doing it again.' : ''}`);
    }
    r.fig.stand();
    this.d.hold(r.n, false);
    // the player went away before ever making it out (drove on, say): it was not seen, and it walks
    // again another time they are back within sight of the place (once more)
    if (why === 'left' && this.looked <= 0 && (this.tries.get(r.n) ?? 0) < TRIES) { r.walked = undefined; r.markWalked(undefined); }
    this.phase = 'idle'; this.ended = why; this.rec = null; this.n = 0; this.clear = false;
    this.d.save();
  }
  /** A night put back or left (nothing of it walks on; the one standing stands). */
  reset() {
    if (this.rec) { this.rec.fig.stand(); this.d.hold(this.rec.n, false); }
    this.phase = 'idle'; this.rec = null; this.n = 0; this.waiting = false; this.ended = null; this.clear = false;
    this.tries.clear();
  }
}
