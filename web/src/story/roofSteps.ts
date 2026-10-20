import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Interaction } from '../core/Interaction';
import type { Corner } from '../world/roofFrost';
import { clockText } from './time';

// Light steps on the records room roof (FORSLAG.md, 20 October, "Valg for Tom", the extra choice).
// Tom: "Spenning og skummelt slår faste regler". This one breaks two rules of HENDELSER.md on
// purpose, once in the night: they make no sound, not even steps; and nothing in the world acts
// because the player is there. Here something walks over the player's head, and it knows where.
//
//   the steps   chapter two, once, while the player is in the records room at a quiet moment
//               (nothing open on the table, no call, the tape in the control room not playing, a
//               paper read, standing still a moment): light steps, closer together than anyone
//               walks, come across the annex roof from over the corridor's east end and stop right
//               over the player (up onto the roof unit when the player is at the table, under it).
//               While they come they go where the player goes, and the room's hum sinks a little.
//   the beat    nothing. A thin trickle of dust from the ceiling just in front of the player's face;
//               the room's hum drops out and the tired tube stops flickering; the lights stay on.
//               10.5 s of it.
//   once more   six or seven, quicker, further off: from where it now stands to the lip of the roof
//               at a corner (the south-west, or the south-east when that is the nearer), and gone.
//               The hum comes back two and a half seconds later and the tired tube stutters.
//   after       frost on the roof unit over the records room, and a crust of it on the lip at that
//               corner: seen from outside, with a closer look (the zoom), and a note in the player's
//               words, and another the first time the frost is looked at.
//
// The steps are heard through the slab: the bought ones (sfx/roof_steps.mp3, eight hits of a knife
// going through into a board: a dull thump and a dry crunch) or, without the private folder,
// Kenney's gravel steps (public, CC0) played quicker; low-passed, a thump in the slab under each,
// placed on the roof with the HRTF panner (headphones hear them above), quieter and duller the
// further off. A step on the roof unit over the table (where a player at the table has it stop)
// is on its sheet-metal top, 0.8 m higher, and the metal gives a short dull ring. Outside or in the control room the slab's filter is off and the walls do it.
// Kept in the night's seen set (main.ts `townSeen`, saved as `town`): `roof:steps` when they come,
// `roof:sw` / `roof:se` for the corner (the frost), `roof:frost` once the frost has been looked at.
// Captioned in time with them (the game captions its sounds as toasts): once while they come, when
// they stop, when they go. Under each that comes over the room a few motes of dust shake out of the
// ceiling, so the steps can be seen crossing it with the sound down (a phone, at night).

export interface RoofIn {
  /** Chapter two at the records stage or ready to call the key holder, nothing else of the night's
   *  going (main.ts): the steps may start. */
  ok: boolean;
  /** No paper or board open, no cinematic, no call on the line: quiet enough to hear a roof. */
  quiet: boolean;
  /** A call or a cinematic: what is left of the steps waits for it. */
  hold: boolean;
  /** In the records room. */
  inRoom: boolean;
  /** Indoors (the annex or the control room): heard through the slab. */
  muffled: boolean;
  /** The listener's feet. */
  pos: THREE.Vector3;
  clock: number;
  /** A record has been read (the player has settled into the room). */
  read: boolean;
  /** The key holder is next (P05 recorded): the room's last quiet moment. */
  lastQuiet: boolean;
  /** Still chapter two (if the chapter ends while they are on, what is left of them goes). */
  chapter: boolean;
}

export interface RoofDeps {
  audio: AudioSys;
  seen: Set<string>;
  note: (text: string) => void;
  toast: (text: string, secs: number) => void;
  /** The frost on the roof unit and the lip (Annex.setRoofFrost). */
  frost: (corner: Corner | null) => void;
  /** Dust from the ceiling where it stops (Annex.sift, a little in front of the player's face). */
  dust: (x: number, z: number) => void;
  /** A few motes shaken loose under one step that comes over the rooms (Annex.sift, small). */
  puff: (x: number, z: number, n: number) => void;
  /** The tubes in the records room (Annex.holdTubes). */
  tubes: (mode: 'steady' | 'stutter' | null) => void;
  /** Where the tired tube hangs, for the tick when the hum comes back. */
  tubeAt: THREE.Vector3;
  inter: Interaction;
  /** The hit areas for a closer look (Annex.frost.hit). */
  hit: { unit: THREE.Object3D; sw: THREE.Object3D; se: THREE.Object3D };
  /** The player is outside in SARO's grounds (a closer look at the roof makes sense only there). */
  outside: () => boolean;
  /** The night's clock. */
  clock: () => number;
}

type Phase = 'idle' | 'come' | 'still' | 'off' | 'after' | 'done';
interface Step { at: number; x: number; z: number; slot: number; gain: number; burst: 1 | 2 }

// the annex's roof deck (Annex.ts: the slab 2.8 to 3.2 over x -6.3 to 2.3, z 4.8 to 10.8; the control
// room's taller wall along its north side), and where the steps come from and go to
const ROOF = { x0: -6.1, x1: 2.1, z0: 5.0, z1: 10.6, y: 3.22 };
const FROM = new THREE.Vector2(1.9, 5.3);                  // over the corridor's east end, by the restrooms
const CORNER: Record<Corner, THREE.Vector2> = { sw: new THREE.Vector2(-6.12, 10.62), se: new THREE.Vector2(2.12, 10.62) };
const STRIDE = 0.3, SETTLE = 0.8, BEAT = 10.5, AHEAD = 0.12;
const SLOTS = 8, SLOT = 0.3, SLOT_LEN = 0.28;               // sfx/roof_steps.mp3: slot i from i * 0.3 s
const KENNEY = ['stepGravel0', 'stepGravel1', 'stepGravel2', 'stepGravel3', 'stepGravel4'];
/** Levels, measured in the game (tools/horror_roof.py, K-weighted): right overhead some 6 dB over the
 *  player's own step on the records room's vinyl and 5 over the room's hum (which is as loud as that
 *  step, and low; held 8 dB down while they come, so they stand 13 over it), the first ones across
 *  the roof some 11 dB under the last, coming up out of the hum. */
const LEVEL = { lic: 0.245, kenney: 0.217, thump: 0.079, ring: 0.1 };
// under the deck: the corridor and the records room (Annex.ts), where dust can come down through the view
const ROOMS = { x0: -6.0, x1: -1.0, z0: 4.85, z1: 10.5 };
// the roof unit over the records room (Annex.ts), its top
const UNIT = { x0: -5.0, x1: -3.4, z0: 8.05, z1: 9.15, y: 4.06 };

const UNIT_PLAIN = 'The unit on the records room roof, its louvres furred with dust.';
const UNIT_FROST = 'The unit on the records room roof is white with frost, thickest along the top edge and down the side over the table. Nothing else up there has any. It is not cold enough for frost tonight.';
const LIP_FROST = 'A crust of frost on the lip of the roof at this corner. Nowhere else along the edge.';
// the captions keep time with what is heard (the game captions its sounds as toasts; on a phone
// with the sound down they are the steps): while they come, when they stop, when they go
const CAPTION_COME = 'Footsteps on the roof. Light and quick, closer together than anyone walks.';
const CAPTION_STOP = 'They stop, right over you.';
const CAPTION_2 = 'Again, further off, out to the edge of the roof. Then nothing.';

const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export class RoofSteps {
  phase: Phase = 'idle';
  /** Every step as it was played (tests): when (the event's own clock), where, how far from the
   *  listener's feet across the roof, and which burst. */
  log: { t: number; x: number; z: number; d: number; burst: 1 | 2 }[] = [];
  corner: Corner | null = null;
  /** The bought steps were there (or Kenney's stood in). */
  bought = false;
  private t = 0;
  private room = 0; private quiet = 0; private still = 0;
  private last = new THREE.Vector3(1e9, 0, 0);
  private queue: Step[] = [];
  private cues: { at: number; fn: () => void }[] = [];
  private walk = { pos: new THREE.Vector2(), n: 0, next: 0, settle: 0, hesitated: false, planned: false };
  /** The first caption has been shown (while they come). */
  private told = false;
  private target = new THREE.Vector2();
  private muffled = true;
  private sources = new Set<AudioScheduledSourceNode>();

  constructor(private d: RoofDeps) {
    d.inter.add({ id: 'look:roofUnit', object: d.hit.unit, label: () => null, use: () => {}, look: true, examineRange: 32,
      examine: () => !d.outside() ? null : !this.corner ? UNIT_PLAIN : (this.frostSeen(), UNIT_FROST) });
    for (const c of ['sw', 'se'] as const) d.inter.add({ id: `look:roofLip${c.toUpperCase()}`, object: d.hit[c], label: () => null, use: () => {}, look: true, examineRange: 26,
      examine: () => d.outside() && this.corner === c ? (this.frostSeen(), LIP_FROST) : null });
  }

  /** Where they make for and stop: the roof right over the player (SARO's terms; story/seen.ts). */
  get overhead() { return this.overPt.set(this.target.x, ROOF.y, this.target.y); }
  private overPt = new THREE.Vector3();
  /** Something of the steps is going on: the beat included (the music and the night's other events wait). */
  get busy() { return this.phase !== 'idle' && this.phase !== 'done'; }
  private humBack = 0;

  /** A new night, a save opened: the frost as the seen set has it, nothing going. */
  reset() {
    this.cancel();
    const s = this.d.seen;
    this.corner = s.has('roof:sw') ? 'sw' : s.has('roof:se') ? 'se' : null;
    this.phase = s.has('roof:steps') ? 'done' : 'idle';
    // saved while they were still coming (a paper filed mid-way autosaves): they will not come
    // again, so what they left is up there all the same
    if (this.phase === 'done' && !this.corner) { this.corner = 'sw'; s.add('roof:sw'); }
    this.d.frost(this.corner);
    this.room = 0; this.quiet = 0; this.still = 0; this.last.set(1e9, 0, 0);
  }
  private cancel() {
    for (const s of this.sources) { try { s.stop(); } catch { /* not started or done */ } }
    this.sources.clear();
    this.queue = []; this.cues = [];
    this.d.audio.holdRoomTone(1, 0.05);
    this.d.tubes(null);
  }

  /** Can they start now (the waiting is over)? */
  private due(s: RoofIn) {
    if (!s.ok || !s.inRoom || !s.quiet) return false;
    // settled in, a paper read and put back, standing still: the first quiet moment
    if (s.read && this.room >= 20 && this.quiet >= 4 && this.still >= 1.2) return true;
    // the board done and the key holder next: the room's last quiet moment, before the call
    if (s.lastQuiet && this.quiet >= 3.5) return true;
    // never standing still: still once, a while in
    return this.room >= 75 && this.quiet >= 6;
  }

  update(dt: number, s: RoofIn) {
    const moved = this.last.x > 1e8 ? 0 : Math.hypot(s.pos.x - this.last.x, s.pos.z - this.last.z);
    this.last.copy(s.pos);
    if (this.phase === 'idle') {
      // the time settled in the room counts only from when the steps could come (chapter two's records)
      if (s.ok && s.inRoom && s.quiet) { this.room += dt; this.quiet += dt; } else this.quiet = 0;
      if (moved > 0.004) this.still = 0; else this.still += dt;
      if (this.due(s)) this.start(s);
      return;
    }
    if (!this.busy) return;
    if (!s.chapter) { this.cancel(); this.phase = 'done'; return; }
    this.muffled = s.muffled;
    if (s.hold) return;             // a call or a cinematic: the rest waits for it
    this.t += dt;
    this.target.copy(this.roofOver(s.pos));
    this.plan();
    // play what falls due in the next moment, at its exact time
    const ctx = this.d.audio.ctx;
    while (this.queue.length && this.queue[0].at <= this.t + AHEAD) {
      const st = this.queue.shift()!;
      const d = Math.hypot(st.x - s.pos.x, st.z - s.pos.z);
      this.log.push({ t: st.at, x: st.x, z: st.z, d, burst: st.burst });
      if (ctx) this.play(st, Math.max(0, st.at - this.t));
      // over the rooms and near enough to see: a few motes shaken out of the ceiling under it
      // (the last of the first ones and the trickle where it stops are cued in stopped())
      if (s.inRoom && st.x > ROOMS.x0 && st.x < ROOMS.x1 && st.z > ROOMS.z0 && st.z < ROOMS.z1 && d < 4.5) {
        const n = st.burst === 1 ? (d < 1.6 ? 7 : 4) : 3;
        this.cue(st.at, () => this.d.puff(st.x, st.z, n));
      }
      // the caption keeps time with them: once they are near enough to be plain, while they come
      if (st.burst === 1 && !this.told && (d < 3.2 || this.walk.n >= 9)) {
        this.told = true;
        this.cue(st.at + 0.15, () => this.d.toast(CAPTION_COME, 4.2));
      }
    }
    while (this.cues.length && this.cues[0].at <= this.t) this.cues.shift()!.fn();
  }

  /** Now, whatever the waiting (the developer menu, the tests). False when it cannot be. */
  now(s: RoofIn) {
    if (this.phase !== 'idle' || !s.inRoom) return false;
    this.start(s);
    return true;
  }

  private start(s: RoofIn) {
    this.phase = 'come'; this.t = 0; this.log = []; this.queue = []; this.cues = []; this.told = false;
    this.d.seen.add('roof:steps');           // never twice, even if the night is left in the middle
    this.bought = !!this.d.audio.lic('roof_steps');
    this.clockAt = this.d.clock();
    this.muffled = s.muffled;
    this.target.copy(this.roofOver(s.pos));
    // the hum sinks a little as they come (as when you stop to listen), and goes when they stop
    this.d.audio.holdRoomTone(0.4, 0.6);
    const w = this.walk;
    w.pos.copy(FROM); w.n = 0; w.next = 0.05; w.settle = 0; w.hesitated = false; w.planned = false;
  }

  /** The roof right over a point (clamped to the deck); within half a metre of the roof unit, up on
   *  the unit (a player at the work table stands just off its edge: it steps up onto it, over them). */
  private roofOver(p: THREE.Vector3) {
    const x = THREE.MathUtils.clamp(p.x, ROOF.x0, ROOF.x1), z = THREE.MathUtils.clamp(p.z, ROOF.z0, ROOF.z1), m = 0.12;
    const ux = THREE.MathUtils.clamp(x, UNIT.x0 + m, UNIT.x1 - m), uz = THREE.MathUtils.clamp(z, UNIT.z0 + m, UNIT.z1 - m);
    return Math.hypot(x - ux, z - uz) < 0.5 + m ? new THREE.Vector2(ux, uz) : new THREE.Vector2(x, z);
  }

  // ---------- the steps, a little ahead of when they are heard ----------
  private plan() {
    const w = this.walk;
    while (this.phase === 'come' && !w.planned && w.next <= this.t + AHEAD + 0.05) {
      // coming: a short stride and quick, a little longer every other step (two feet), each foot a
      // little to its side; one hesitation early on; then three slower ones that close the gap
      const to = this.target, d = w.pos.distanceTo(to);
      const dir = to.clone().sub(w.pos).normalize(), side = new THREE.Vector2(-dir.y, dir.x);
      let gap: number, at: THREE.Vector2;
      if (w.settle === 0 && d > SETTLE && w.n < 40) {
        w.pos.addScaledVector(dir, Math.min(STRIDE, d - SETTLE * 0.6));
        at = w.pos.clone().addScaledVector(side, w.n % 2 ? 0.07 : -0.07);
        gap = (w.n % 2 ? 0.205 : 0.155) + rnd(-0.012, 0.012);
        if (w.n === 4 && !w.hesitated) { w.hesitated = true; gap += 0.62; }
      } else {
        w.settle++;
        const k = [0.45, 0.75, 1][w.settle - 1];
        w.pos.lerp(to, k);
        at = w.settle === 3 ? to.clone() : w.pos.clone().addScaledVector(side, w.settle % 2 ? 0.05 : -0.05);
        gap = [0.27, 0.36, 0][w.settle - 1];
      }
      const near = 1 - Math.min(1, w.pos.distanceTo(to) / 6);
      this.queue.push({ at: w.next, x: at.x, z: at.y, slot: this.slot(), gain: (0.92 + 0.08 * near) * rnd(0.85, 1.05) * (w.settle === 3 ? 1.12 : 1), burst: 1 });
      w.n++;
      if (w.settle === 3) { w.planned = true; this.stopped(w.next, to); break; }
      w.next += gap;
    }
  }
  private slotLast = -1;
  private slot() {
    const n = this.bought ? SLOTS : KENNEY.length;
    let i = Math.floor(Math.random() * n);
    if (i === this.slotLast) i = (i + 1 + Math.floor(Math.random() * (n - 1))) % n;
    return (this.slotLast = i);
  }

  /** Right over the player: the beat, then once more further off, then the hum back. */
  private stopped(at: number, where: THREE.Vector2) {
    this.cue(at, () => { this.phase = 'still'; });
    // the corner it goes off at: the south-west, unless that is right over the player
    const c: Corner = where.distanceTo(CORNER.sw) >= 3.2 ? 'sw' : 'se';
    this.corner = c;
    this.d.seen.add(`roof:${c}`);
    this.cue(at + 0.02, () => this.d.frost(c));   // up there now; nobody outside to see it come
    this.cue(at + 0.05, () => this.d.dust(where.x, where.y));
    this.cue(at + 0.9, () => { this.d.audio.holdRoomTone(0, 0.09); this.d.tubes('steady'); this.d.audio.night?.hush(BEAT + 4, 0); });
    this.cue(at + 0.45, () => this.d.toast(CAPTION_STOP, 4.5));
    // once more, further off: from where it stands now (it moved without a sound) to the lip
    const C = CORNER[c], away = C.clone().sub(where), len = away.length(), dir = away.clone().normalize();
    const b0 = where.clone().addScaledVector(dir, THREE.MathUtils.clamp(len - 1.8, 1.6, len - 0.6));
    const n = Math.max(5, Math.min(8, Math.round(b0.distanceTo(C) / 0.27) + 1));
    let t2 = at + BEAT;
    for (let i = 0; i < n; i++) {
      const k = n === 1 ? 1 : i / (n - 1), p = b0.clone().lerp(C, k), side = new THREE.Vector2(-dir.y, dir.x);
      if (i < n - 1) p.addScaledVector(side, i % 2 ? 0.06 : -0.06);
      this.queue.push({ at: t2, x: p.x, z: p.y, slot: this.slot(), gain: 0.62 * rnd(0.85, 1.05) * (1 - 0.25 * k), burst: 2 });
      t2 += Math.max(0.12, (i % 2 ? 0.175 : 0.15) - i * 0.006) + rnd(-0.01, 0.01);
    }
    const gone = t2;
    this.cue(at + BEAT - 0.01, () => { this.phase = 'off'; });
    this.cue(gone + 0.6, () => this.d.toast(CAPTION_2, 4));
    this.humBack = gone + 2.5;
    this.cue(gone + 0.05, () => { this.phase = 'after'; });
    this.cue(this.humBack, () => {
      this.d.audio.holdRoomTone(1, 0.35);
      this.d.tubes('stutter');
      this.d.audio.play('click', { gain: 0.14, rate: 0.62, at: this.d.tubeAt });
    });
    this.cue(this.humBack + 0.4, () => {
      this.phase = 'done';
      this.d.note(`${clockText(this.clockAt, false)}. Records room. Steps on the roof, light and quick, closer together than anyone walks. They came across and stopped right over me, and stayed there. The tubes kept burning and the hum went out. Then a few more, further off, out to the edge of the roof, and nothing.`);
    });
  }
  private clockAt = 0;
  private cue(at: number, fn: () => void) { this.cues.push({ at, fn }); this.cues.sort((a, b) => a.at - b.at); }

  private frostSeen() {
    const s = this.d.seen;
    if (s.has('roof:frost')) return;
    s.add('roof:frost');
    const where = this.corner === 'se' ? 'south-east' : 'south-west';
    this.d.note(`${clockText(this.d.clock(), false)}. Frost on the unit on the records room roof, white along the top, and a crust of it on the lip of the roof at the ${where} corner. Nothing else out here has any.`);
  }

  // ---------- one step, through the slab ----------
  private play(st: Step, when: number) {
    const a = this.d.audio, ctx = a.ctx!, t0 = ctx.currentTime + when;
    const lic = a.lic('roof_steps'), buf = lic ?? a.buf[KENNEY[st.slot % KENNEY.length]];
    if (!buf) return;
    // on the roof unit's sheet-metal top (over the table) a step is higher up, and the metal rings
    const onUnit = st.x > UNIT.x0 && st.x < UNIT.x1 && st.z > UNIT.z0 && st.z < UNIT.z1, y = onUnit ? UNIT.y : ROOF.y;
    const pan = ctx.createPanner();
    pan.panningModel = 'HRTF'; pan.distanceModel = 'inverse'; pan.refDistance = 1.2; pan.rolloffFactor = 0.7; pan.maxDistance = 60;
    if (pan.positionX) { pan.positionX.value = st.x; pan.positionY.value = y; pan.positionZ.value = st.z; }
    else (pan as any).setPosition(st.x, y, st.z);
    pan.connect(a.sfx);
    const src = ctx.createBufferSource(); src.buffer = buf;
    // small and quick: the bought hits a little up in pitch, Kenney's a good deal (they are a man's)
    src.playbackRate.value = (lic ? 1.12 : 1.42) * rnd(0.95, 1.06);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.Q.value = 0.6;
    const far = Math.min(1, Math.hypot(st.x - this.target.x, st.z - this.target.y) / 6);
    lp.frequency.value = this.muffled ? 2400 - 900 * far : 5200;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 70;
    const g = ctx.createGain(); g.gain.value = st.gain * (lic ? LEVEL.lic : LEVEL.kenney) * (this.muffled ? 1 : 0.7);
    src.connect(hp); hp.connect(lp); lp.connect(g); g.connect(pan);
    if (lic) src.start(t0, st.slot * SLOT, SLOT_LEN); else src.start(t0);
    this.track(src);
    if (onUnit) this.ring(ctx, pan, t0 + 0.004, st.gain);
    // the slab taking it: a short low thump under each step, felt more than heard
    if (this.muffled) {
      const o = ctx.createOscillator(); o.type = 'sine';   // (on the unit too: its curb stands on the slab)
      o.frequency.setValueAtTime(rnd(64, 78), t0); o.frequency.exponentialRampToValueAtTime(46, t0 + 0.09);
      const og = ctx.createGain(); og.gain.setValueAtTime(0, t0);
      og.gain.linearRampToValueAtTime(LEVEL.thump * st.gain, t0 + 0.004); og.gain.exponentialRampToValueAtTime(0.0008, t0 + 0.11);
      o.connect(og); og.connect(pan); o.start(t0); o.stop(t0 + 0.13);
      this.track(o);
    }
  }
  /** A light foot on thin sheet metal: a dull, short ring (three damped partials, the panel's own),
   *  through the same panner. */
  private ring(ctx: AudioContext, out: AudioNode, t0: number, k: number) {
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = this.muffled ? 1100 : 3000;
    lp.connect(out);
    const base = rnd(176, 196);
    for (const [m, a, dec] of [[1, 1, 0.16], [1.73, 0.55, 0.11], [2.91, 0.3, 0.07]] as const) {
      const o = ctx.createOscillator(); o.frequency.value = base * m * rnd(0.99, 1.01);
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t0);
      g.gain.linearRampToValueAtTime(LEVEL.ring * a * k, t0 + 0.003); g.gain.exponentialRampToValueAtTime(0.0005, t0 + dec);
      o.connect(g); g.connect(lp); o.start(t0); o.stop(t0 + dec + 0.02);
      this.track(o);
    }
  }
  private track(n: AudioScheduledSourceNode) {
    this.sources.add(n);
    n.onended = () => { this.sources.delete(n); try { n.disconnect(); } catch { /* gone */ } };
  }
}
