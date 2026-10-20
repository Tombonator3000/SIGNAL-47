import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import type { AudioSys } from './Audio';
import { hash } from '../drive/roadShape';
import { SARO_IN_ROAD, MOTEL_AT, MOTEL_RECT } from '../world/geo';
import { artImage, artLoaded, artTexture, loadArtSome, ALIEN_ART, type ArtId } from './art';

// The walk down the highway to Sierra Motor Court, 650 m on the shoulder at night (Tom's
// improvements, 16 October: make the way between the places a place). The first, small step
// of the event director in HENDELSER.md, on foot only, in SARO's terms:
//
//   the line      the telephone line along the east side hums as the player passes a pole
//   the car       two headlights far down the highway in the south, coming north, slowly;
//                 they stop a long way off, and go out. Nothing arrives. No sound.
//   the silence   once in a case: the crickets, the wind and the player's own footsteps stop
//                 for 47 seconds (HENDELSER.md, event 3). In the notebook after.
//   the roof      sometimes, coming down towards the motel: someone small on its roof against
//                 the sky, only while not looked at (more than 25 degrees from the middle of
//                 the view, and more than 40 m off). Looked at, it was not there.
//
// Nothing chases and nothing answers (HENDELSER.md, the rules). Nothing in the prologue, and
// nothing once THE EVENT has the night (main.ts decides when it may run).

const POLE_X = 15 - SARO_IN_ROAD.x;                  // the telephone line, road x 15 (roadProps.poles)
const poleZ = (k: number) => { const z0 = -1900 + 45 * k; return z0 + (hash(z0, 1) - 0.5) * 4 - SARO_IN_ROAD.z; };
const CAR_LANE = 1.8 - SARO_IN_ROAD.x;               // the northbound lane
const ROOF = new THREE.Vector3(-46.5 + MOTEL_AT.x, MOTEL_AT.y + 3.01, 31 + MOTEL_AT.z);
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export interface RoadsideIn {
  /** On foot outside on the highway between SARO and the motel, at night, in play. */
  ok: boolean;
  pos: THREE.Vector3;
  cam: THREE.Camera;
  /** Whether the silence has happened in this case (it is in the notebook). */
  silenceDone: boolean;
  hush: (secs: number, level: number) => void;
  flicker: (k: number) => void;
  /** The silence is over: write it down. */
  silenceOver: () => void;
}

export class Roadside {
  /** The 47 seconds of silence are on: no footsteps, no music, nothing out in the dark. */
  silent = false;
  /** What happened last, for the developer menu and the tests. */
  last: { what: 'car' | 'silence' | 'roof'; at: number } | null = null;
  private t = 0;
  private onFoot = 0;                 // seconds on the shoulder this time
  private walked = 0;                 // metres down it this time
  private lastZ = 0;
  private silenceT = 0;
  private hum: { g: GainNode; pan: PannerNode; stop: () => void } | null = null;
  private car: { pts: THREE.Points; mat: THREE.PointsMaterial; z: number; state: 'come' | 'stop' | 'out'; t: number } | null = null;
  private carNext = rnd(25, 60);
  private carAgain = 0;               // seconds until the car may come again
  private sil: Silhouette;
  private figure: THREE.Group;
  private roof: 'none' | 'up' | 'gone' = 'none';
  private roofT = 0;

  constructor(private audio: AudioSys, scene: THREE.Scene) {
    // listening, its head a little to one side, against the sky (the dark red of the motel's sign on its edge)
    this.sil = new Silhouette('roadside:figure', 0x2c0e0b, 'turned');
    this.figure = this.sil.group;
    this.figure.position.copy(ROOF);
    this.figure.rotation.y = Math.PI / 2;   // facing the highway
    this.figure.visible = false;
    scene.add(this.figure);
  }

  update(dt: number, o: RoadsideIn) {
    this.t += dt;
    const p = o.pos;
    if (!o.ok) {
      this.onFoot = 0; this.walked = 0;
      if (this.roof === 'up') this.hideRoof();
      if (this.roof === 'gone' && p.z < MOTEL_RECT.minZ - 200) this.roof = 'none';
    } else {
      this.sil.wake();
      this.onFoot += dt;
      if (p.z > this.lastZ) this.walked += p.z - this.lastZ;
    }
    this.lastZ = p.z;
    this.lineHum(dt, o);
    this.carStep(dt, o);
    this.silenceStep(dt, o);
    this.roofStep(dt, o);
  }

  // ---------- the line ----------
  private lineHum(_dt: number, o: RoadsideIn) {
    const ctx = this.audio.ctx;
    if (!ctx) return;
    if (!this.hum) {
      if (!o.ok) return;
      const g = ctx.createGain(); g.gain.value = 0;
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 420;
      const am = ctx.createGain(); am.gain.value = 0.75;
      const lfo = ctx.createOscillator(); lfo.frequency.value = 0.11;
      const lg = ctx.createGain(); lg.gain.value = 0.25; lfo.connect(lg); lg.connect(am.gain);
      const oscs = ([[60, 'sine', 0.5], [120, 'sine', 0.35], [180, 'triangle', 0.12], [240.6, 'sine', 0.06]] as const).map(([f, type, v]) => {
        const s = ctx.createOscillator(); s.type = type; s.frequency.value = f;
        const sg = ctx.createGain(); sg.gain.value = v; s.connect(sg); sg.connect(lp); s.start(); return s;
      });
      lfo.start();
      const pan = this.audio.spatial(new THREE.Vector3(POLE_X, 8.6, 0), 2.0) as PannerNode;
      lp.connect(am); am.connect(g); g.connect(pan);
      this.hum = { g, pan, stop: () => { for (const s of [...oscs, lfo]) { try { s.stop(); } catch { /* stopped */ } } g.disconnect(); pan.disconnect(); } };
    }
    // at the nearest pole, up by the insulators
    const k = Math.round((o.pos.z + SARO_IN_ROAD.z + 1900) / 45), z = poleZ(k), now = ctx.currentTime;
    const P = this.hum.pan;
    if (P.positionZ) { P.positionX.setTargetAtTime(POLE_X, now, 0.1); P.positionY.setTargetAtTime(8.6, now, 0.1); P.positionZ.setTargetAtTime(z, now, 0.1); }
    else (P as unknown as { setPosition: (x: number, y: number, z: number) => void }).setPosition(POLE_X, 8.6, z);
    const on = o.ok && !this.silent && o.pos.z > 100;
    this.hum.g.gain.setTargetAtTime(on ? 0.05 : 0, now, on ? 1.2 : 0.3);
  }

  // ---------- the car far down the highway ----------
  private carStep(dt: number, o: RoadsideIn) {
    this.carAgain = Math.max(0, this.carAgain - dt);
    if (!this.car && o.ok && !this.silent && this.carAgain <= 0 && this.onFoot > this.carNext && o.pos.z > 150 && o.pos.z < MOTEL_RECT.minZ - 20) this.carNow(o.pos);
    const c = this.car;
    if (!c) return;
    c.t += dt;
    if (c.state === 'come') {
      c.z -= 11 * dt;
      if (c.t > 48) { c.state = 'stop'; c.t = 0; }
    } else if (c.state === 'stop') {
      if (c.t > 7) { c.state = 'out'; c.t = 0; }
    } else {
      c.mat.opacity = Math.max(0, 1 - c.t / 0.5);
      if (c.t > 0.6) { c.pts.removeFromParent(); c.pts.geometry.dispose(); c.mat.dispose(); this.car = null; return; }
    }
    if (c.state !== 'out') c.mat.opacity = 0.85 + 0.15 * Math.sin(this.t * 9.1) * Math.sin(this.t * 2.3);   // the air between
    const a = c.pts.geometry.getAttribute('position') as THREE.BufferAttribute;
    a.setZ(0, c.z); a.setZ(1, c.z); a.needsUpdate = true;
  }
  /** Headlights far down the highway (the developer menu, the tests): from 2.6 km south of here. */
  carNow(from: THREE.Vector3) {
    if (this.car) return;
    const z = Math.max(from.z, 0) + 2600, y = MOTEL_AT.y + 0.8;
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute([CAR_LANE - 0.75, y, z, CAR_LANE + 0.75, y, z], 3));
    const mat = new THREE.PointsMaterial({ color: 0xfff1d6, size: 5, sizeAttenuation: false, transparent: true, opacity: 0.9, depthWrite: false, fog: false, blending: THREE.AdditiveBlending });
    const pts = new THREE.Points(g, mat);
    pts.frustumCulled = false; pts.name = 'roadside:car';
    this.figure.parent?.add(pts);
    this.car = { pts, mat, z, state: 'come', t: 0 };
    this.carAgain = 15 * 60; this.carNext = rnd(25, 60);
    this.last = { what: 'car', at: this.t };
  }
  get carState() { return this.car ? { state: this.car.state, z: this.car.z } : null; }
  /** Where the car's lights are (SARO's terms) while they are there (story/seen.ts), or null. */
  get carAt() { return this.car && this.car.state !== 'out' ? this.carPt.set(CAR_LANE, MOTEL_AT.y + 0.8, this.car.z) : null; }
  private carPt = new THREE.Vector3();
  /** Where the one on the motel's roof stands (its head) while it is there, or null. */
  get roofAt() { return this.roof === 'up' ? this.roofPt.copy(ROOF).setY(ROOF.y + 0.6) : null; }
  private roofPt = new THREE.Vector3();

  // ---------- the silence ----------
  private silenceStep(dt: number, o: RoadsideIn) {
    if (this.silent) {
      this.silenceT -= dt;
      if (this.silenceT <= 0) { this.silent = false; o.silenceOver(); }
      return;
    }
    if (o.ok && !o.silenceDone && !this.car && this.walked > 100 && o.pos.z > 260 && o.pos.z < MOTEL_RECT.minZ - 90) this.silenceNow(o);
  }
  /** 47 seconds with no sound (the developer menu, the tests). */
  silenceNow(o: Pick<RoadsideIn, 'hush' | 'flicker'>) {
    if (this.silent) return;
    this.silent = true; this.silenceT = 47;
    o.hush(47, 0);
    o.flicker(0.4);
    this.last = { what: 'silence', at: this.t };
  }

  // ---------- someone on the motel's roof ----------
  private roofStep(dt: number, o: RoadsideIn) {
    const p = o.pos, d = p.distanceTo(ROOF);
    // coming down towards the motel: the first time the player looks away from it, and not
    // every time, someone is there (only ever where it is not being looked at)
    if (this.roof === 'none' && o.ok && !this.silent && p.z > MOTEL_RECT.minZ - 170 && p.z < MOTEL_RECT.minZ - 45 && this.offCentre(o.cam) > 32) {
      if (Math.random() < 0.6) this.roofNow(); else this.roof = 'gone';
    }
    if (this.roof !== 'up') return;
    this.roofT += dt;
    // looked at, too near, or long enough: it was not there
    if (this.offCentre(o.cam) < 25 || d < 40 || this.roofT > 75 || !o.ok) this.hideRoof();
  }
  /** Someone on the roof (the developer menu, the tests). */
  roofNow() { this.roof = 'up'; this.roofT = 0; this.figure.visible = true; this.last = { what: 'roof', at: this.t }; }
  get roofUp() { return this.roof === 'up'; }
  private hideRoof() { this.roof = 'gone'; this.figure.visible = false; }
  /** Degrees between the middle of the view and the figure. */
  private offCentre(cam: THREE.Camera) {
    const f = new THREE.Vector3(); cam.getWorldDirection(f);
    const to = ROOF.clone().setY(ROOF.y + 0.6).sub(cam.getWorldPosition(new THREE.Vector3())).normalize();
    return THREE.MathUtils.radToDeg(Math.acos(THREE.MathUtils.clamp(f.dot(to), -1, 1)));
  }

  /** A new night, the title, a save opened. */
  reset() {
    this.silent = false; this.silenceT = 0; this.onFoot = 0; this.walked = 0; this.carAgain = 0; this.carNext = rnd(25, 60);
    if (this.car) { this.car.pts.removeFromParent(); this.car.pts.geometry.dispose(); this.car.mat.dispose(); this.car = null; }
    this.hideRoof(); this.roof = 'none';
    if (this.hum) { this.hum.stop(); this.hum = null; }
  }
}

// About 1.2 m: a large head, a thin body, long thin arms. Black, with a faint edge of the light
// behind it. Once round 14's outlines (alien/) are there, it is one of them: a flat picture that
// turns round its own upright to whoever looks (an outline has no sides), dark, the light only along
// its edge, never a face. Until then (or if they do not come), a low figure in code: a shell a
// little larger round it, its inside faces only, gives the edge.
// 'bent' is only ever near, over you on waking (story/abduction.ts): the standing picture with its
// head brought down to you and a little to one side (bentArt below).
export type Pose = 'stand' | 'turned' | 'far' | 'bent';
const POSE_ART: Record<Pose, ArtId> = { stand: 'silhouetteStand', turned: 'silhouetteTurned', far: 'silhouetteFar', bent: 'silhouetteStand' };
let asked: Promise<void> | null = null;
/** The outlines are fetched once, the first time the night may show one of them. */
const askArt = () => (asked ??= loadArtSome(ALIEN_ART));
// The pictures are twice as high as wide, the figure in them 88 per cent of the height with its feet
// 7 per cent above the bottom: a card 1.36 m high stands a figure of 1.2 m on the ground.
const CARD_H = 1.36, CARD_W = 0.68, FEET = 0.074 * CARD_H;
// The light along the pictures' edge is a cold grey; the colour given is what it should be.
const EDGE = new THREE.Color(0x141923);
const _p = new THREE.Vector3(), _c = new THREE.Vector3(), _s = new THREE.Vector3(), _q = new THREE.Quaternion();

/** The middle of the head in the standing pictures, metres above the ground. */
const HEAD_MID = 1.1;
let bent: { tex: THREE.CanvasTexture; head: number } | null | false = null;
/** The standing picture with the head bent: cut off at the neck (the narrowest row under it) and
 *  put back lower, into the shoulders, tipped a little to one side: from below, a head looking down
 *  at you. Made once from the picture itself, so it is the same outline as everywhere else. */
function bentArt() {
  if (bent !== null) return bent || null;
  if (!artLoaded('silhouetteStand')) return null;
  // (a picture the page may not read back, from another origin: then it stands as it is)
  try { bent = makeBent(); } catch { bent = false; }
  return bent || null;
}
function makeBent() {
  const im = artImage('silhouetteStand'), W = im.naturalWidth || im.width, H = im.naturalHeight || im.height;
  const c = document.createElement('canvas'); c.width = W; c.height = H;
  const g = c.getContext('2d', { willReadFrequently: true })!;
  g.drawImage(im, 0, 0, W, H);
  const a = g.getImageData(0, 0, W, H).data;
  const row = (y: number) => { let n = 0, sx = 0; for (let x = 0; x < W; x++) if (a[(y * W + x) * 4 + 3] > 128) { n++; sx += x; } return { n, mid: n ? sx / n : W / 2 }; };
  let top = 0; while (top < H && row(top).n === 0) top++;
  let neck = top + Math.round(H * 0.16), nw = Infinity;
  for (let y = top + Math.round(H * 0.08); y < top + Math.round(H * 0.24); y++) { const r = row(y); if (r.n > 0 && r.n < nw) { nw = r.n; neck = y; } }
  const cx = row(neck).mid, headH = neck - top;
  // down by about a third of the head, into the shoulders; tipped to one side, about 15 degrees
  const drop = Math.round(headH * 0.34), tilt = -0.26;
  g.clearRect(0, 0, W, neck);
  g.save(); g.translate(cx, neck + drop); g.rotate(tilt); g.scale(1.06, 1.06);
  g.drawImage(im, 0, 0, W, neck, -cx, -neck, W, neck);
  g.restore();
  const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace; tex.anisotropy = 4; tex.name = 'art/silhouetteBent';
  // where the head's middle is now, in the card's metres above the feet
  const mid = top + headH / 2 + drop;
  return { tex, head: CARD_H - FEET - (mid / H) * CARD_H };
}

export class Silhouette {
  readonly group = new THREE.Group();
  private body: THREE.Mesh;
  private shell: THREE.Mesh;
  private card: THREE.MeshBasicMaterial | null = null;
  private edge: number;
  private asking = false;

  constructor(name: string, edge: number, private want: Pose = 'stand') {
    const parts = [
      new THREE.SphereGeometry(0.2, 10, 8).scale(1, 1.22, 1.05).translate(0, 1.0, 0),
      new THREE.CylinderGeometry(0.035, 0.04, 0.1, 6).translate(0, 0.83, 0),
      new THREE.CylinderGeometry(0.1, 0.13, 0.42, 8).translate(0, 0.6, 0),
      new THREE.CylinderGeometry(0.032, 0.03, 0.42, 5).translate(-0.06, 0.2, 0),
      new THREE.CylinderGeometry(0.032, 0.03, 0.42, 5).translate(0.06, 0.2, 0),
      new THREE.CylinderGeometry(0.024, 0.02, 0.52, 5).rotateZ(0.08).translate(-0.17, 0.52, 0),
      new THREE.CylinderGeometry(0.024, 0.02, 0.52, 5).rotateZ(-0.08).translate(0.17, 0.52, 0),
    ].map((g) => g.index ? g.toNonIndexed() : g);
    for (const g of parts) { g.deleteAttribute('uv'); }
    const body = mergeGeometries(parts)!;
    this.group.name = name;
    this.body = new THREE.Mesh(body, new THREE.MeshBasicMaterial({ color: 0x050506 }));
    this.shell = new THREE.Mesh(body.clone().scale(1.07, 1.03, 1.07), new THREE.MeshBasicMaterial({ color: edge, side: THREE.BackSide }));
    this.group.add(this.body, this.shell);
    this.group.traverse((x) => { x.frustumCulled = false; });
    this.edge = edge;
    if (artLoaded(POSE_ART.stand)) this.useArt();
  }
  /** The night may show it now: the pictures are asked for (once), and taken when they come. */
  wake() {
    if (this.card || this.asking) return;
    this.asking = true;
    void askArt().then(() => this.useArt());
  }
  private useArt() {
    if (this.card || !artLoaded(POSE_ART.stand)) return;   // without them, the figure in code stays
    const mat = new THREE.MeshBasicMaterial({ map: artTexture(POSE_ART.stand), transparent: true, alphaTest: 0.02, depthWrite: false });
    mat.name = 'silhouette';
    const card = new THREE.Mesh(new THREE.PlaneGeometry(CARD_W, CARD_H).translate(0, CARD_H / 2 - FEET, 0), mat);
    card.name = this.group.name + ':card';
    card.frustumCulled = false;
    card.raycast = () => { /* nothing to take hold of */ };
    card.onBeforeRender = (_r, _scene, cam) => {
      card.matrixWorld.decompose(_p, _q, _s);
      _c.setFromMatrixPosition(cam.matrixWorld);
      card.matrixWorld.makeRotationY(Math.atan2(_c.x - _p.x, _c.z - _p.z)).scale(_s).setPosition(_p);
    };
    this.group.add(card);
    this.body.visible = this.shell.visible = false;
    this.card = mat;
    this.pose(this.want); this.rim(this.edge);
  }
  /** Standing, turned half aside as if listening, far off and soft in the haze, or bent to you. */
  pose(p: Pose) {
    this.want = p;
    if (this.card) this.card.map = (p === 'bent' ? bentArt()?.tex : null) ?? artTexture(artLoaded(POSE_ART[p]) ? POSE_ART[p] : POSE_ART.stand);
  }
  /** How high the middle of its head is, in metres, as it stands now (the picture's or the code's). */
  get headY() { return this.card && this.want === 'bent' ? bentArt()?.head ?? HEAD_MID : HEAD_MID; }
  /** The colour of the light along its edge (what is behind it where it stands). */
  rim(hex: number) {
    this.edge = hex;
    (this.shell.material as THREE.MeshBasicMaterial).color.setHex(hex);
    if (this.card) {
      const c = this.card.color.setHex(hex);
      c.setRGB(Math.min(4, c.r / EDGE.r), Math.min(4, c.g / EDGE.g), Math.min(4, c.b / EDGE.b));
    }
  }
  /** Whether it is one of the pictures now (the tests). */
  get painted() { return !!this.card; }
}
