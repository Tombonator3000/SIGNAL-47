import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { DocSpec } from '../ui/UI';
import { clockText } from './time';

// The time away (HENDELSER.md, section 8; Tom, 16 October: yes to all four questions). On foot,
// alone, outdoors on one of the lines, at night: the pressure rises, and at the top of it they
// take a recording of you, the way you take a picture of an antenna. Nobody in the game says so.
//
//   the warning  20 to 47 s: the crickets stop, then the player's own footsteps, the flashlight
//                stutters and dies, a deep pressure tone. Get off the line or in among people
//                before the light comes, and it passes.
//   the light    white, from above, from nothing to be seen.
//   the taking   the picture freezes with a tracking fault, the sound is cut, a few single
//                frames (the light; the grass from above with your shadow in it; a hand with
//                fingers too long at the edge; yourself lying in the grass from above), black.
//   lost time    17 to 47 minutes, never past the next fixed point of the story.
//   back         on your back in the grass, the stars; the view lifts. 100 to 400 m further
//                along, in a ring of flattened grass with frost on it; the flashlight beside
//                you, on. The truck's engine has died where it stood and wants the key.
//   traces       the lost time, one exposure too many on the field camera (yourself from
//                above), a line in your own hand you do not remember writing, a nosebleed that
//                stopped by itself. And afterwards, where you were taken, someone in a SARO
//                jacket stands quite still, the reflective band catching the light, for the rest
//                of the night: the recording (R5). Gone if you walk up to it.
//
// At least 40 minutes of the night between two, at most three in a night; the director sees to
// one before chapter five. The setting turns them off (on by default). main.ts decides when it
// may run (chapters one to four, at night, no panel, no cinematic) and where the lines are.

export type Line = 'A' | 'B' | 'C';
export interface AbductIn {
  ok: boolean;
  /** The setting (on by default). */
  enabled: boolean;
  /** On foot, outdoors, alone, on a line: which. Null when not. */
  line: Line | null;
  /** How the line runs here (a unit vector on the ground), to come back further along it. */
  along: THREE.Vector3;
  clock: number;
  /** The clock by which the story's next fixed point comes; the lost time stops short of it. */
  nextFixed: number;
  /** Chapter three or four: the one before chapter five is due. */
  late: boolean;
  calm: boolean;
}
export interface AbductDeps {
  audio: AudioSys;
  player: Player;
  scene: THREE.Scene;
  overlay: HTMLElement;
  /** A frame of the scene seen by this camera, drawn at once (main.ts renders it). */
  shoot: (cam: THREE.PerspectiveCamera) => HTMLCanvasElement | null;
  ground: (x: number, z: number) => number;
  /** Ground a player can stand on (the area's zones, minus the boxes in the way). */
  standable: (x: number, z: number) => boolean;
  /** Off the asphalt, onto the verge, where the place has one (main.ts). */
  settle?: (p: THREE.Vector3) => void;
  flashlight: { have: boolean; on: boolean; flicker: (k: number) => void };
  hush: (secs: number, level: number) => void;
  glitch: (k: number) => void;
  skipClock: (sec: number) => void;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  /** The field camera: whether the player has it, and a picture on its roll. */
  camera: () => boolean;
  file: (doc: DocSpec) => void;
  keyStall: () => void;
  save: () => void;
}
/** What is kept with the case. */
export interface AbductSave { count: number; last: number; spots: { scene: string; take: [number, number, number]; back: [number, number, number]; clock: number; pic?: string; doc?: DocSpec }[] }

type Stage = 'calm' | 'warning' | 'light' | 'taken' | 'black' | 'back';
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const MAX = 3, GAP = 40 * 60, MIN_LOST = 17 * 60;

export class Abduction {
  stage: Stage = 'calm';
  /** The footsteps and the night's sounds are gone (main.ts mutes the steps). */
  silent = false;
  /** 0..1, how near the top (the developer menu, the tests). */
  pressure = 0;
  data: AbductSave = { count: 0, last: -1e9, spots: [] };
  /** The scene the player is in, as main.ts names it (rings and recordings belong to one). */
  sceneId = 'saro';
  private st = 0;
  private warnLen = 30;
  private top = 1;
  private lost = 0;
  private from = 0;
  private take = new THREE.Vector3();
  private frames: HTMLCanvasElement[] = [];
  private canvas: HTMLCanvasElement;
  private tone: { stop: () => void; gain: GainNode } | null = null;
  private rings: THREE.Mesh[] = [];
  private records: { g: THREE.Group; scene: string }[] = [];
  private lying: THREE.Group;
  private hadLight = false;

  constructor(private d: AbductDeps) {
    const css = document.createElement('style');
    css.textContent = '.abduct-light::after{content:"";position:fixed;inset:0;pointer-events:none;z-index:39;opacity:var(--abduct,0);background:radial-gradient(ellipse at 50% -10%,#fff 0%,#eef4f6 45%,rgba(220,232,236,.85) 100%)}';
    document.head.appendChild(css);
    this.canvas = document.createElement('canvas'); this.canvas.width = 640; this.canvas.height = 400;
    this.canvas.className = 'abduct-frame';
    Object.assign(this.canvas.style, { position: 'fixed', inset: '0', width: '100%', height: '100%', objectFit: 'cover', pointerEvents: 'none', zIndex: '40', display: 'none', background: '#000' });
    d.overlay.appendChild(this.canvas);
    this.lying = jacketFigure(true); this.lying.visible = false; d.scene.add(this.lying);
  }

  get busy() { return this.stage !== 'calm'; }
  /** Holds the player still (the taking and the waking). */
  get holds() { return this.stage === 'light' || this.stage === 'taken' || this.stage === 'black' || this.stage === 'back'; }

  update(dt: number, s: AbductIn) {
    this.st += dt;
    switch (this.stage) {
      case 'calm': return this.calm(dt, s);
      case 'warning': return this.warning(dt, s);
      case 'light': return this.light(s);
      case 'taken': return this.taken(s);
      case 'black': if (this.st > 2) this.wake(s); return;
      case 'back': return this.back(dt);
    }
  }

  // ---------- the pressure ----------
  private calm(dt: number, s: AbductIn) {
    this.recordsStep();
    const room = s.nextFixed - s.clock >= MIN_LOST + 60;
    const can = s.ok && s.enabled && !!s.line && this.data.count < MAX && s.clock - this.data.last >= GAP && room;
    if (!can) { this.pressure = Math.max(0, this.pressure - dt / 90); return; }
    // about four to eight minutes alone on a line, sooner towards morning, much sooner when the
    // one before chapter five is due and has not come
    const day = ((s.clock % 86400) + 86400) % 86400, k = THREE.MathUtils.clamp((day - 2.3 * 3600) / (2.5 * 3600), 0, 1);
    this.pressure += dt / 360 * (0.7 + 0.8 * k) * (s.late && this.data.count === 0 ? 2.5 : 1);
    if (this.pressure >= this.top) this.begin(s);
  }
  /** Start the warning now (the developer menu, the tests). */
  begin(s: AbductIn) {
    this.stage = 'warning'; this.st = 0; this.warnLen = rnd(20, 47); this.silent = false;
    this.hadLight = this.d.flashlight.on;
    this.take.copy(this.d.player.pos);
    this.d.hush(this.warnLen + 12, 0);
    void s;
  }
  private warning(_dt: number, s: AbductIn) {
    const t = this.st, w = this.warnLen;
    // got away: off the line, inside, among people, before the light
    if (!s.ok || !s.line || !s.enabled) { this.passOver(); return; }
    if (t > w * 0.3) this.silent = true;
    if (t > w * 0.45) this.d.flashlight.flicker(THREE.MathUtils.clamp((t - w * 0.45) / (w * 0.4), 0, 1));
    if (t > w * 0.85 && this.d.flashlight.on) this.d.flashlight.on = false;
    if (t > w * 0.5 && !this.tone) this.toneOn();
    if (this.tone) this.tone.gain.gain.value = 0.05 + 0.25 * THREE.MathUtils.clamp((t - w * 0.5) / (w * 0.5), 0, 1);
    if (t >= w) { this.stage = 'light'; this.st = 0; this.take.copy(this.d.player.pos); this.from = s.clock; }
  }
  private passOver() {
    this.stage = 'calm'; this.st = 0; this.silent = false; this.pressure = 0.35; this.top = rnd(0.85, 1.15);
    this.toneOff(); this.d.flashlight.flicker(0); this.d.hush(0.1, 1);
    if (this.hadLight && this.d.flashlight.have) this.d.flashlight.on = true;
  }

  // ---------- the light, the taking ----------
  private light(s: AbductIn) {
    const k = Math.min(1, this.st / 3);
    this.d.overlay.style.setProperty('--abduct', String(k));
    this.d.overlay.classList.add('abduct-light');
    if (this.st >= 3) {
      this.stage = 'taken'; this.st = 0;
      this.toneOff();
      this.d.glitch(1);
      this.frames = this.makeFrames();
      this.d.audio.suspend(true);
      const room = s.nextFixed - s.clock - 60;
      this.lost = Math.round(Math.min(rnd(17, 47) * 60, room));
    }
  }
  private taken(s: AbductIn) {
    const per = s.calm ? 1.4 : 0.95, i = Math.floor(this.st / per), k = this.st - i * per;
    this.d.overlay.classList.remove('abduct-light');
    this.d.glitch(Math.max(0, 1 - this.st * 2));
    if (i >= this.frames.length) { this.show(null); this.stage = 'black'; this.st = 0; return; }
    if (s.calm) this.show(this.frames[i], 0.35 * Math.min(1, k / 0.4, (per - k) / 0.4));
    else this.show(k < per - 0.12 ? this.frames[i] : null, 1);
  }
  private show(c: HTMLCanvasElement | null, alpha = 1) {
    const cv = this.canvas, g = cv.getContext('2d')!;
    g.fillStyle = '#000'; g.fillRect(0, 0, cv.width, cv.height);
    if (c) { g.globalAlpha = alpha; g.drawImage(c, 0, 0, cv.width, cv.height); g.globalAlpha = 1; }
    cv.style.display = 'block';
  }

  // ---------- back ----------
  private wake(s: AbductIn) {
    const p = this.d.player, d = this.d;
    d.skipClock(this.lost);
    const t1 = this.from + this.lost;
    // further along the line, on ground that can be stood on
    const back = this.further(this.take, s.along);
    p.place(back.x, back.z, p.yaw + rnd(-2, 2));
    p.pitch = 1.3; p.eye = 0.35;
    back.y = d.ground(back.x, back.z); this.take.y = d.ground(this.take.x, this.take.z);
    this.ring(back);
    if (d.flashlight.have) d.flashlight.on = true;
    d.flashlight.flicker(0);
    d.audio.suspend(false);
    d.hush(6, 0.2);
    this.canvas.style.display = 'none';
    this.silent = false;
    this.data.count++; this.data.last = t1;
    const spot: AbductSave['spots'][number] = { scene: this.sceneId, take: [this.take.x, this.take.y, this.take.z], back: [back.x, back.y, back.z], clock: this.from };
    this.data.spots.push(spot);
    this.record(this.take, this.sceneId);
    d.keyStall();
    // the traces
    const a = clockText(this.from, false), b = clockText(t1, false);
    d.note(`${a} to ${b}. Nothing.`);
    d.note(`${b}. A line in the notebook in my own hand. I do not remember writing it: ${Math.random() < 0.5 ? '026' : clockText(s.nextFixed, false)}.`);
    d.note(`${b}. A nosebleed. It had stopped by itself.`);
    if (d.camera()) {
      const pic = this.fromAbove(back, true);
      if (pic) {
        spot.doc = { id: `away${this.data.count}`, kind: 'photo', image: pic.toDataURL('image/jpeg', 0.8), page: '', title: `PHOTO / Not exposed by me (${a})`,
          transcript: `One more exposure on the roll than there should be, with the counter at ${b}. Taken from above, from higher than anyone could stand: someone lying on their back in the grass in a SARO jacket, the reflective band across the chest. Me.` };
        d.file(spot.doc);
      }
    }
    d.save();
    this.stage = 'back'; this.st = 0;
  }
  private back(dt: number) {
    const p = this.d.player, k = Math.min(1, this.st / 6);
    p.eye = 0.35 + (1.62 - 0.35) * THREE.MathUtils.smoothstep(k, 0.3, 1);
    p.pitch += (-0.06 - p.pitch) * Math.min(1, dt * (k > 0.4 ? 1.4 : 0.2));
    if (this.st > 2.5 && this.st - dt <= 2.5) this.d.toast('On your back in the grass. Stars. The flashlight is lying beside you, on. The grass round you is flat, and white with frost.', 6);
    if (k >= 1) { p.eye = 1.62; this.stage = 'calm'; this.st = 0; this.pressure = 0; this.top = rnd(0.85, 1.15); }
  }
  private further(from: THREE.Vector3, along: THREE.Vector3) {
    const dir = along.clone().setY(0).normalize();
    for (const dist of [rnd(220, 400), rnd(100, 220), 100, 60, 30]) for (const sgn of Math.random() < 0.5 ? [1, -1] : [-1, 1]) {
      const q = from.clone().addScaledVector(dir, dist * sgn);
      this.d.settle?.(q);
      if (this.d.standable(q.x, q.z)) return q;
    }
    return from.clone();
  }

  // ---------- what is left ----------
  private ring(at: THREE.Vector3) {
    const geo = new THREE.RingGeometry(1.3, 2.7, 40, 1); geo.rotateX(-Math.PI / 2);
    const m = new THREE.Mesh(geo, new THREE.MeshBasicMaterial({ color: 0xc8d4dc, transparent: true, opacity: 0.2, depthWrite: false }));
    m.position.set(at.x, at.y + 0.03, at.z); m.name = 'abduct:ring';
    this.d.scene.add(m); this.rings.push(m);
  }
  private record(at: THREE.Vector3, scene: string) {
    const g = jacketFigure(false); g.name = 'abduct:record';
    g.position.copy(at); g.rotation.y = rnd(0, Math.PI * 2);
    g.visible = false; this.d.scene.add(g); this.records.push({ g, scene });
  }
  /** The recordings stand where they were taken; walk up to one and it was not there. */
  private recordsStep() {
    const c = this.d.player.pos;
    for (const r of this.records) r.g.visible = r.scene === this.sceneId && Math.hypot(r.g.position.x - c.x, r.g.position.z - c.z) > 45;
  }
  /** Yourself lying in the grass, from above (the frame and the extra exposure). */
  private fromAbove(at: THREE.Vector3, me: boolean) {
    const y = this.d.ground(at.x, at.z);
    if (me) { this.lying.position.set(at.x, y + 0.12, at.z); this.lying.rotation.y = rnd(0, Math.PI * 2); this.lying.visible = true; }
    const cam = new THREE.PerspectiveCamera(40, 1.6, 0.5, 400);
    cam.position.set(at.x + 0.3, y + (me ? 7.5 : 5.5), at.z + 0.2); cam.lookAt(at.x, y, at.z);
    cam.updateMatrixWorld(true);
    // their light, from straight above: the only time the ground here is seen like this
    const L = new THREE.PointLight(0xeef4ff, 140, 30, 1.6); L.position.set(at.x, y + 9, at.z);
    this.d.scene.add(L); L.updateMatrixWorld(true);
    const c = this.d.shoot(cam);
    L.removeFromParent(); L.dispose();
    this.lying.visible = false;
    return c ? grain(c, 7, me ? 0.9 : 1.3) : null;
  }
  private makeFrames() {
    const out: HTMLCanvasElement[] = [frameLight(3)];
    const grass = this.fromAbove(this.d.player.pos, false);
    if (grass) out.push(grass);
    out.push(frameHand(11));
    const me = this.fromAbove(this.d.player.pos, true);
    if (me) out.push(me);
    return out;
  }

  // ---------- the tone ----------
  private toneOn() {
    const ctx = this.d.audio.ctx;
    if (!ctx) return;
    const g = ctx.createGain(); g.gain.value = 0;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 160;
    const o = [37, 52.5, 74].map((f) => { const x = ctx.createOscillator(); x.frequency.value = f; x.connect(lp); x.start(); return x; });
    lp.connect(g); g.connect(this.d.audio.sfx);
    this.tone = { gain: g, stop: () => { for (const x of o) { try { x.stop(); } catch { /* stopped */ } } g.disconnect(); } };
  }
  private toneOff() { this.tone?.stop(); this.tone = null; }

  // ---------- the case ----------
  restore(v: AbductSave | null | undefined) {
    this.clear();
    this.data = v ? { count: v.count, last: v.last, spots: [...v.spots] } : { count: 0, last: -1e9, spots: [] };
    for (const sp of this.data.spots) {
      this.ring(new THREE.Vector3(...sp.back));
      this.record(new THREE.Vector3(...sp.take), sp.scene);
    }
  }
  /** The exposures that were not the player's, back among the papers (after the case is opened). */
  refile() { for (const sp of this.data.spots) if (sp.doc) this.d.file(sp.doc); }
  private clear() {
    this.toneOff(); this.silent = false; this.stage = 'calm'; this.st = 0; this.pressure = 0; this.top = rnd(0.85, 1.15);
    this.canvas.style.display = 'none'; this.d.overlay.classList.remove('abduct-light');
    for (const m of this.rings) { m.removeFromParent(); m.geometry.dispose(); (m.material as THREE.Material).dispose(); }
    for (const r of this.records) r.g.removeFromParent();
    this.rings = []; this.records = [];
  }
}

// ---------- the figures and the frames ----------
/** Someone in a SARO jacket, the reflective band across it: standing, or lying on their back. */
function jacketFigure(lying: boolean) {
  const g = new THREE.Group(), body = new THREE.Group();
  const coat = new THREE.MeshBasicMaterial({ color: 0x2a3036 }), dark = new THREE.MeshBasicMaterial({ color: 0x15171a });
  const band = new THREE.MeshBasicMaterial({ color: 0xd8dccc }), skin = new THREE.MeshBasicMaterial({ color: 0x6e5a4c });
  const box = (w: number, h: number, d: number, m: THREE.Material, x: number, y: number, z: number) => { const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), m); b.position.set(x, y, z); body.add(b); };
  box(.2, .82, .2, dark, -.11, .41, 0); box(.2, .82, .2, dark, .11, .41, 0);
  box(.5, .62, .28, coat, 0, 1.14, 0); box(.52, .07, .3, band, 0, 1.2, 0);
  box(.12, .6, .14, coat, -.32, 1.12, 0); box(.12, .6, .14, coat, .32, 1.12, 0);
  const head = new THREE.Mesh(new THREE.SphereGeometry(.13, 10, 8), skin); head.position.y = 1.6; body.add(head);
  if (lying) { body.rotation.x = -Math.PI / 2; body.position.y = .14; }
  g.add(body);
  g.traverse((x) => { x.frustumCulled = false; });
  return g;
}
function grain(src: HTMLCanvasElement, seed: number, dark = 1) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  g.filter = `grayscale(0.7) contrast(1.3) brightness(${dark})`; g.drawImage(src, 0, 0, 640, 400); g.filter = 'none';
  let s = seed;
  const r = () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
  for (let i = 0; i < 9000; i++) { g.fillStyle = `rgba(255,255,255,${r() * 0.08})`; g.fillRect(r() * 640, r() * 400, 1, 1); }
  const v = g.createRadialGradient(320, 200, 90, 320, 200, 400); v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,.9)');
  g.fillStyle = v; g.fillRect(0, 0, 640, 400);
  for (let y = 0; y < 400; y += 3) { g.fillStyle = 'rgba(0,0,0,.18)'; g.fillRect(0, y, 640, 1); }
  return c;
}
function frameLight(seed: number) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  const l = g.createRadialGradient(320, 40, 10, 320, 160, 420); l.addColorStop(0, '#ffffff'); l.addColorStop(0.35, '#d8e0e4'); l.addColorStop(1, '#101214');
  g.fillStyle = l; g.fillRect(0, 0, 640, 400);
  return grain(c, seed, 1);
}
function frameHand(seed: number) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  const l = g.createRadialGradient(260, 120, 20, 300, 200, 420); l.addColorStop(0, '#e8eef0'); l.addColorStop(1, '#202426');
  g.fillStyle = l; g.fillRect(0, 0, 640, 400);
  // at the right-hand edge, against the light: a hand with fingers too long
  g.fillStyle = '#060607'; g.strokeStyle = '#060607'; g.lineCap = 'round';
  g.beginPath(); g.ellipse(640, 250, 70, 56, -0.3, 0, 7); g.fill();
  for (const [a, len] of [[-2.2, 190], [-2.5, 230], [-2.8, 240], [-3.05, 210]] as const) {
    g.lineWidth = 9; g.beginPath(); g.moveTo(600, 240); g.lineTo(600 + Math.cos(a) * len, 240 + Math.sin(a) * len * 0.8); g.stroke();
  }
  return grain(c, seed, 1);
}
