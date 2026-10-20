import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import { DecoderPlayer } from '../core/decoder';
import { crankBell } from './officePhone';

// Reel 047, 10 August 1979, on the signal processor's deck in the control room (story/archivePhone.ts;
// FORSLAG.md, 20 October, item 5 and Valg for Tom 1). The commissioning series ran one reel a night
// from 25 June 1979 (story/nightshift.ts, reelDate): 047 is the night SARO's solver first came out
// negative, -32, the one that is not in the exceptions binder and not in the tape catalogue
// (SHOW TAPES: 001-046, 048-1311). On it, in order:
//
//   the pulse group   four and seven, every 5.2 s, raw, as tape B has it tonight (core/decoder.ts)
//   a crank           a field telephone rung by its handle: the crank telephone's bell (the motel
//                     office's own, story/officePhone.ts), a little way off, two turns
//   a cut             metal going through metal, a snap: a cable through cutters
//   nothing           for 47 seconds, not even the hiss: the tape has nothing on it at all. The deck
//                     runs on and its reels go round; the night outside goes quiet with it
//   a relay           close and loud, on the tape: the way K3 trips on the rack beside the deck
//   the range         the processor reads the data track's last solve off the end of the tape:
//                     SOURCE DISTANCE -32 LY, on its display
//
// None of it is explained. It pays off later: FRAME 04 (the cable at STATION 01, cut), E10 (02:17:00
// the carrier ceased, 02:17:47 the relay and the reference moving), N. Vega's correction (she cut it
// with Tomás in the reference). -32 is 1979 less 1947; nobody says so (HISTORIE.md).
//
// Driven on the game's own time (update(dt)), not the AudioContext's, so a test can run it fast; each
// sound starts when its moment comes. The deck's motor, the hiss and the 4/7 are continuous and are
// gated on their own nodes. Without sound it runs the same, silent, with its captions.

export type ReelCue =
  | { at: number; kind: 'thread' | 'start' | 'pulse' | 'crank' | 'cut' | 'hole' | 'relay' | 'back' | 'range' | 'leader' | 'end' };

/** When each thing comes, seconds from play(). The hole is exactly 47 s. */
export const REEL_PLAN: ReelCue[] = [
  { at: 0, kind: 'thread' },
  { at: 2.2, kind: 'start' },
  { at: 3.4, kind: 'pulse' },
  { at: 14.2, kind: 'crank' },
  { at: 17.6, kind: 'cut' },
  { at: 18.6, kind: 'hole' },
  { at: 18.6 + 47, kind: 'relay' },
  { at: 18.6 + 47 + 0.9, kind: 'back' },
  { at: 18.6 + 47 + 1.4, kind: 'range' },
  { at: 18.6 + 47 + 3.2, kind: 'leader' },
  { at: 18.6 + 47 + 5.2, kind: 'end' },
];
export const HOLE = { from: 18.6, len: 47 };
const PULSE_END = 13.8;

export interface Reel047Deps {
  audio: AudioSys;
  /** Where the deck is (the signal processor's reels), its two reels, and its display. */
  deck: () => THREE.Vector3;
  reels: THREE.Object3D[];
  screen: { canvas: HTMLCanvasElement; ctx: CanvasRenderingContext2D; tex: THREE.Texture } | undefined;
  /** The night outside quiet (secs, level, tau, back), as Ambience.hush. */
  hush: (secs: number, level: number, tau: number, back: number) => void;
}

export class Reel047 {
  /** Playing now (one thing at a time: main.ts holds the night and the music for it). */
  playing = false;
  /** Seconds into it. */
  t = 0;
  /** The cues as they came (tests). */
  heard: string[] = [];
  /** It reached the end (not stopped before). */
  finished = false;
  /** What is on the tape now, 0 (nothing at all: the 47 s, not even the hiss) to 1 (tests). */
  gate = 0;
  private k = 0;
  private spin = 0;
  private scrT = 0;
  private dec: DecoderPlayer | null = null;
  private nodes: AudioNode[] = [];
  private srcs: AudioScheduledSourceNode[] = [];
  private tape: GainNode | null = null;
  private motor: { o: OscillatorNode; g: GainNode } | null = null;
  private cb: ((c: ReelCue) => void) | null = null;

  constructor(private d: Reel047Deps) {}

  /** Thread it and play it; `cue` gets each thing on it as it comes. */
  play(cue: (c: ReelCue) => void) {
    this.stop();
    this.playing = true; this.finished = false; this.t = 0; this.k = 0; this.heard = []; this.spin = 0; this.gate = 0;
    this.cb = cue;
    this.build();
    this.step(0);
  }
  /** Stopped where it is (a new night, the title): not heard to the end. */
  stop() {
    if (!this.playing && !this.tape) return;
    this.playing = false; this.cb = null;
    this.release(true);
  }

  update(dt: number) {
    if (!this.playing) return;
    this.t += dt;
    this.step(dt);
  }

  private step(dt: number) {
    const a = this.d.audio, ctx = a.ctx;
    while (this.k < REEL_PLAN.length && REEL_PLAN[this.k].at <= this.t) {
      const c = REEL_PLAN[this.k++];
      this.heard.push(c.kind);
      if (ctx && this.tape) this.sound(c, ctx);
      this.cb?.(c);
      if (c.kind === 'end') { this.playing = false; this.finished = true; this.release(false); this.draw(); return; }
    }
    if (this.dec) { this.dec.update(); if (this.t >= PULSE_END) { this.dec.stop(); this.dec = null; } }
    // the reels go round from the start to the end, through the hole too
    if (this.t >= 2.2) {
      this.spin += dt * 2.4;
      this.d.reels.forEach((r, i) => { r.rotation.x = this.spin * (i ? 0.8 : 1.15); });
    }
    if ((this.scrT -= dt) <= 0) { this.scrT = 1 / 8; this.draw(); }
  }

  // ---------- the sound ----------
  /** The tape's chain: what is on the tape comes through the deck's head and the processor's little
   *  monitor, placed at the deck; the motor and the transport are the deck's own, not on the tape. */
  private build() {
    const a = this.d.audio, ctx = a.ctx;
    if (!ctx || ctx.state === 'closed' || !a.noiseBuffer()) return;
    const at = this.d.deck();
    const out = a.spatial(at, 1.8);
    if (!out) return;
    const mon = ctx.createBiquadFilter(); mon.type = 'peaking'; mon.frequency.value = 1700; mon.Q.value = 0.8; mon.gain.value = 4; mon.connect(out);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 5200; lp.connect(mon);
    const tape = ctx.createGain(); tape.gain.value = 0; tape.connect(lp);
    this.tape = tape; this.nodes.push(out, mon, lp, tape);
    // the hiss on the tape, from the start
    const n = ctx.createBufferSource(); n.buffer = a.noiseBuffer()!; n.loop = true;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 1800;
    const ng = ctx.createGain(); ng.gain.value = 0.045;
    n.connect(hp); hp.connect(ng); ng.connect(tape); n.start();
    this.srcs.push(n); this.nodes.push(hp, ng);
  }
  private sound(c: ReelCue, ctx: AudioContext) {
    const a = this.d.audio, tape = this.tape!, now = ctx.currentTime, at = this.d.deck();
    const deck = (name: string, gain: number, rate: number, when = 0) => { const s = a.play(name, { gain, rate, at, when }); if (s) this.srcs.push(s.src); };
    const onTape = (name: string, gain: number, rate: number, when = 0, lp?: number) => { const s = a.play(name, { gain, rate, when, dest: tape, lp }); if (s) this.srcs.push(s.src); };
    switch (c.kind) {
      case 'thread':
        // the reel seated on its spindle, the leader through the guides
        deck('click', 0.4, 0.8); deck('click', 0.3, 1.1, 0.5); deck('paper1', 0.18, 2.2, 0.9); deck('click', 0.35, 0.9, 1.6);
        break;
      case 'start': {
        deck(a.or('small_switch', 'switch'), 0.45, 0.8);
        this.gate = 1;
        tape.gain.setTargetAtTime(1, now, 0.05);
        // the motor and the capstan
        const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = 22;
        const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 170;
        const g = ctx.createGain(); g.gain.value = 0; o.connect(lp); lp.connect(g);
        const sp = a.spatial(at, 1.4); if (sp) g.connect(sp);
        o.start(); o.frequency.setTargetAtTime(38, now, 0.15); g.gain.setTargetAtTime(0.028, now, 0.12);
        this.motor = { o, g }; this.srcs.push(o); this.nodes.push(lp, g);
        if (sp) this.nodes.push(sp);
        break;
      }
      case 'pulse': {
        // the pulse group as it came in, raw: tape B's own sound, through the deck's monitor
        const noise = a.noiseBuffer();
        if (!noise) break;
        // (its own panner at the deck, straight to the effects: not through the tape's)
        this.dec = new DecoderPlayer(ctx, a.sfx, at.clone().add(new THREE.Vector3(-0.3, 1.1, 0)), noise);
        this.dec.start({ mode: 'raw', stretch: 1, pitch: 0, space: 0.2 });
        break;
      }
      case 'crank': {
        // a field telephone rung by its handle, a little way off: two turns of the crank bell
        const b = crankBell(ctx), s = ctx.createBufferSource(); s.buffer = b;
        const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1300; bp.Q.value = 0.7;
        const g = ctx.createGain(); g.gain.value = 0.3;
        s.connect(bp); bp.connect(g); g.connect(tape); s.start(now, 0, 3.0);
        // the handle's gears under it
        const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.setValueAtTime(70, now);
        o.frequency.linearRampToValueAtTime(110, now + 1.2); o.frequency.linearRampToValueAtTime(80, now + 2.6);
        const og = ctx.createGain(); og.gain.setValueAtTime(0, now); og.gain.linearRampToValueAtTime(0.03, now + 0.2); og.gain.setValueAtTime(0.03, now + 2.4); og.gain.linearRampToValueAtTime(0, now + 2.8);
        const olp = ctx.createBiquadFilter(); olp.type = 'lowpass'; olp.frequency.value = 900;
        o.connect(olp); olp.connect(og); og.connect(tape); o.start(now); o.stop(now + 2.9);
        this.srcs.push(s, o); this.nodes.push(bp, g, og, olp);
        break;
      }
      case 'cut': {
        // two blades closing on copper: a click, a short bright crunch, a ring
        onTape('click', 0.7, 0.55);
        const n = ctx.createBufferSource(); n.buffer = a.noiseBuffer()!;
        const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 3800; bp.Q.value = 1.4;
        const g = ctx.createGain(); g.gain.setValueAtTime(0, now + 0.03); g.gain.linearRampToValueAtTime(0.5, now + 0.035); g.gain.exponentialRampToValueAtTime(0.001, now + 0.12);
        n.connect(bp); bp.connect(g); g.connect(tape); n.start(now + 0.03); n.stop(now + 0.15);
        const r = ctx.createOscillator(); r.frequency.value = 2950;
        const rg = ctx.createGain(); rg.gain.setValueAtTime(0, now + 0.04); rg.gain.linearRampToValueAtTime(0.06, now + 0.045); rg.gain.exponentialRampToValueAtTime(0.0005, now + 0.5);
        r.connect(rg); rg.connect(tape); r.start(now + 0.04); r.stop(now + 0.55);
        onTape('thudSoft', 0.3, 2.1, 0.05);
        this.srcs.push(n, r); this.nodes.push(bp, g, rg);
        break;
      }
      case 'hole':
        // nothing on the tape at all, for 47 seconds: the hiss too (the deck runs on); the night
        // outside goes with it, and comes back after
        this.gate = 0;
        tape.gain.cancelScheduledValues(now); tape.gain.setValueAtTime(tape.gain.value, now); tape.gain.setTargetAtTime(0, now, 0.006);
        this.d.hush(HOLE.len, 0.04, 0.4, 0.2);
        break;
      case 'relay':
        this.gate = 1;
        tape.gain.cancelScheduledValues(now); tape.gain.setValueAtTime(0, now); tape.gain.linearRampToValueAtTime(1, now + 0.004);
        onTape(a.or('rack_switch', 'switch'), 1.0, 0.62);
        onTape('thudSoft', 0.5, 1.4, 0.03);
        this.d.hush(4, 0.3, 0.05, 2.2);
        break;
      case 'back':
        break;
      case 'leader':
        // the end of the tape: the leader through the guides, then flapping on the take-up reel
        this.gate = 0;
        tape.gain.setTargetAtTime(0, now, 0.05);
        for (let i = 0; i < 6; i++) deck('click', 0.22 - i * 0.025, 1.6, i * (0.16 + i * 0.05));
        break;
      case 'end':
        deck(a.or('small_switch', 'switch'), 0.4, 0.72);
        break;
      case 'range':
        break;
    }
  }
  private release(now: boolean) {
    const ctx = this.d.audio.ctx;
    this.dec?.stop(); this.dec = null;
    if (this.motor && ctx) { const m = this.motor; m.g.gain.setTargetAtTime(0, ctx.currentTime, now ? 0.05 : 0.3); m.o.frequency.setTargetAtTime(12, ctx.currentTime, 0.3); }
    this.motor = null;
    if (this.tape && ctx) this.tape.gain.setTargetAtTime(0, ctx.currentTime, 0.03);
    this.tape = null;
    const srcs = this.srcs, nodes = this.nodes; this.srcs = []; this.nodes = [];
    setTimeout(() => {
      for (const s of srcs) { try { s.stop(); } catch { /* stopped */ } try { s.disconnect(); } catch { /* gone */ } }
      for (const n of nodes) { try { n.disconnect(); } catch { /* gone */ } }
    }, now ? 120 : 1800);
  }

  // ---------- the processor's display ----------
  /** What the processor shows: the reel, its counter, and what is on the tape now. */
  get state(): string {
    const t = this.t;
    if (!this.playing && !this.finished) return '';
    if (this.finished || t >= REEL_PLAN.find((c) => c.kind === 'range')!.at) return 'RANGE';
    if (t >= HOLE.from + HOLE.len) return 'RELAY';
    if (t >= HOLE.from) return 'NO SIGNAL';
    if (t >= 3.4 && t < PULSE_END) return 'PULSE 4/7';
    return t < 2.2 ? 'THREADING' : 'PLAY';
  }
  draw() {
    const s = this.d.screen; if (!s) return;
    const g = s.ctx, w = s.canvas.width, h = s.canvas.height;
    g.fillStyle = '#031008'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#8cffa4'; g.font = '20px VT323'; g.textBaseline = 'top';
    g.fillText('REEL 047  10-AUG-79', 8, 4);
    const st = this.state, secs = Math.max(0, Math.floor(this.t - 2.2));
    const counter = `${String(Math.floor(secs / 60)).padStart(2, '0')}:${String(secs % 60).padStart(2, '0')}`;
    if (st === 'RANGE') {
      g.fillStyle = '#ffd27a'; g.fillText('SOURCE DIST  -32 LY', 8, 26);
      g.fillStyle = 'rgba(140,255,164,.6)'; g.fillText(this.finished ? 'END OF TAPE' : `${counter}  DATA TRACK`, 8, 48);
    } else {
      g.fillText(`${counter}  ${st}`, 8, 26);
      // the level on the tape: nothing at all in the hole
      const lvl = st === 'NO SIGNAL' || st === 'THREADING' ? 0 : st === 'PULSE 4/7' ? 0.4 + 0.5 * Math.abs(Math.sin(this.t * 9)) : st === 'RELAY' ? 1 : 0.18 + 0.06 * Math.sin(this.t * 23);
      for (let i = 0; i < 16; i++) { g.fillStyle = i / 16 < lvl ? '#b9ffc7' : 'rgba(110,220,150,.18)'; g.fillRect(10 + i * 14, 52, 10, 12); }
    }
    s.tex.needsUpdate = true;
  }
}
