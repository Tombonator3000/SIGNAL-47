import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import { clockText } from './time';

// The morning holds its breath (FORSLAG.md, 20 October, small change 5; Tom: "Spenning og skummelt
// slår faste regler"). Chapter five, Mesa Diner, at 05:13 or the first moment after it that the
// player is inside and nothing else of the night's is going (main.ts, oneAtATime): everything stops
// at once. The cooler behind the counter cuts out, the radio stops in the middle of a word, the
// birds outside stop, and the room's hum goes with them (and no thunder from the dry storms over
// the mesa: main.ts). Ten seconds of nothing. Then the
// waitress, not looking up: "There. Some mornings about now." Then it comes back one thing at a
// time: the cooler, the radio where the programme has got to, a bird. Once in a night, kept with
// the case (`morning:held`); not after 05:20, when the chapter's drive is due.
//
// "Some mornings", not "every morning": she has watched the lights over the mesa every few years
// since she was eleven (story/Chapter5.ts). Her line has no recording: it is text, as the night's
// other new lines are. It is the plain morning's, as the yard holding its breath in chapter one is
// (core/ambience.ts, breath), not one of the night's events: the curve is held in the diner in
// chapter five and does not count it, but it keeps the one-at-a-time rule both ways (it waits for
// anything else, and nothing starts while it holds). It bends HENDELSER.md section 4, "nothing
// happens in the diner during the map", for these thirty seconds; the map itself is never open
// while it starts.
//
// The cooler is new with it: a reach-in cooler behind the counter, a compressor that runs all the
// time (it was the room tone alone before). It is heard in the diner in every chapter.

export const MORNING_KEY = 'morning:held';
/** 05:13: when it may begin, and 05:20, after which it does not (the chapter's drive). */
export const MORNING_FROM = 5 * 3600 + 13 * 60, MORNING_TO = 5 * 3600 + 20 * 60;
export const MORNING_LINE = 'There. Some mornings about now.';
/** What happens when, in seconds from the moment it stops. */
export const MORNING_CUES = { caption: 2.4, line: 10.6, cooler: 15.2, radio: 20.5, bird: 25.4, end: 29.5 };

// ---------- the cooler behind the counter ----------
/** A compressor running: mains hum and its harmonics with a rattle in it, low and close. */
export class DinerCooler {
  private n: { out: GainNode; f0: OscillatorNode[]; lp: BiquadFilterNode; stop: () => void } | null = null;
  private pos = new THREE.Vector3();
  /** Running now (the compressor is on), and heard (the diner is here). */
  running = true;
  heard = false;
  level = 0.09;
  constructor(private audio: AudioSys) {}

  private build() {
    const a = this.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise) return null;
    const out = ctx.createGain(); out.gain.value = 0;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 420;
    out.connect(lp); const pan = a.spatial(this.pos, 1.6); if (pan) lp.connect(pan);
    const f0: OscillatorNode[] = [];
    for (const [mul, g, type] of [[1, 0.55, 'sine'], [2, 0.3, 'sine'], [3, 0.08, 'square'], [5, 0.03, 'sawtooth']] as const) {
      const o = ctx.createOscillator(); o.type = type; o.frequency.value = 58 * mul;
      const og = ctx.createGain(); og.gain.value = g; o.connect(og); og.connect(out); o.start(); f0.push(o);
    }
    // the rattle of the compressor in its housing: a band of noise, shaken a few times a second
    const src = ctx.createBufferSource(); src.buffer = noise; src.loop = true;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 160; bp.Q.value = 2.5;
    const rg = ctx.createGain(); rg.gain.value = 0.22;
    const lfo = ctx.createOscillator(); lfo.frequency.value = 7.3; const lg = ctx.createGain(); lg.gain.value = 0.12;
    lfo.connect(lg); lg.connect(rg.gain);
    src.connect(bp); bp.connect(rg); rg.connect(out); src.start(); lfo.start();
    return { out, f0, lp, stop: () => { for (const s of [...f0, src, lfo]) { try { s.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }

  /** Every frame: where it stands (null: the diner is not here), and whether the listener is inside. */
  update(at: THREE.Vector3 | null, inside: boolean) {
    const ctx = this.audio.ctx;
    if (!ctx) return;
    if (!at) {
      if (this.n) { const n = this.n; n.out.gain.setTargetAtTime(0, ctx.currentTime, 0.2); setTimeout(() => n.stop(), 1500); this.n = null; }
      this.heard = false;
      return;
    }
    this.pos.copy(at);
    if (!this.n) { this.n = this.build(); if (!this.n) return; }
    this.heard = true;
    const n = this.n, now = ctx.currentTime;
    const want = this.running ? this.level * (inside ? 1 : 0.3) : 0;
    n.out.gain.setTargetAtTime(want, now, this.running ? 0.25 : 0.02);
    n.lp.frequency.setTargetAtTime(inside ? 420 : 170, now, 0.3);
  }
  /** The compressor cuts out: a knock in the housing, the motor running down in a fraction of a second. */
  off() {
    const ctx = this.audio.ctx, n = this.n;
    this.running = false;
    if (!ctx || !n) return;
    const now = ctx.currentTime;
    this.audio.play('thudSoft', { gain: 0.32, rate: 1.75, at: this.pos });
    for (const [i, o] of n.f0.entries()) { const f = 58 * [1, 2, 3, 5][i]; o.frequency.cancelScheduledValues(now); o.frequency.setValueAtTime(f, now); o.frequency.exponentialRampToValueAtTime(f * 0.55, now + 0.35); }
    n.out.gain.cancelScheduledValues(now); n.out.gain.setValueAtTime(n.out.gain.value, now); n.out.gain.setTargetAtTime(0, now + 0.06, 0.07);
  }
  /** And starts again: the knock, the motor pulling up to speed. */
  on() {
    const ctx = this.audio.ctx, n = this.n;
    this.running = true;
    if (!ctx || !n) return;
    const now = ctx.currentTime;
    this.audio.play('thudSoft', { gain: 0.3, rate: 1.55, at: this.pos });
    for (const [i, o] of n.f0.entries()) { const f = 58 * [1, 2, 3, 5][i]; o.frequency.cancelScheduledValues(now); o.frequency.setValueAtTime(f * 0.45, now); o.frequency.exponentialRampToValueAtTime(f, now + 0.9); }
    n.out.gain.cancelScheduledValues(now); n.out.gain.setValueAtTime(0, now); n.out.gain.linearRampToValueAtTime(this.level * 1.25, now + 0.5); n.out.gain.setTargetAtTime(this.level, now + 0.9, 0.6);
  }
}

// ---------- the morning holds its breath ----------
export interface MorningIn {
  /** In the diner in chapter five, inside, on foot, playing. */
  here: boolean;
  /** The diner is the area (to let go at once when the player drives off in the middle of it). */
  area: boolean;
  /** Nothing else of the night's is going, no call on the line, no cinematic. */
  free: boolean;
  /** A panel is open (a talk, the maps on the table): it waits. */
  panel: boolean;
  clock: number;
}
export interface MorningDeps {
  audio: AudioSys;
  cooler: DinerCooler;
  seen: Set<string>;
  /** The diner's radio is playing now, near enough to be heard. */
  radioHeard: () => boolean;
  /** The radio's power (0: dead air; 1: the programme where it has got to). */
  radio: (power: number) => void;
  toast: (text: string, secs: number) => void;
  note: (text: string) => void;
  saw: () => void;
  save: () => void;
}

export class MorningHolds {
  phase: 'idle' | 'held' | 'done' = 'idle';
  t = 0;
  private cue = 0;
  /** The radio was playing when it stopped (the caption says so). */
  private radioWas = false;
  /** When it began (the night's clock), for the note. */
  at: number | null = null;
  constructor(private d: MorningDeps) {}

  /** Holding now: nothing else may start (main.ts, oneAtATime), and the music keeps out. */
  get busy() { return this.phase === 'held'; }
  /** Had tonight. */
  get had() { return this.d.seen.has(MORNING_KEY); }

  update(dt: number, s: MorningIn) {
    if (this.phase === 'idle') {
      if (this.had) { this.phase = 'done'; return; }
      const day = ((s.clock % 86400) + 86400) % 86400;
      if (s.here && s.free && !s.panel && day >= MORNING_FROM && day < MORNING_TO) this.begin(s.clock);
      return;
    }
    if (this.phase !== 'held') return;
    // driven off, or gone to another area, in the middle of it: everything as it was, at once
    if (!s.area) { this.letGo(true); return; }
    this.t += dt;
    const C = MORNING_CUES, d = this.d;
    const step = (at: number, fn: () => void) => { if (this.t >= at && this.cue < at) fn(); };
    step(C.caption, () => d.toast(this.radioWas ? 'The cooler behind the counter cuts out. The radio stops in the middle of a word. Outside, the birds stop.' : 'The cooler behind the counter cuts out. Outside, the birds stop. All of it at once.', 4.2));
    step(C.line, () => {
      d.toast(`THE WAITRESS: "${MORNING_LINE}"`, 4);
      d.note(`${clockText(this.at ?? s.clock, false)}. Mesa Diner. Everything stopped at once: the cooler behind the counter, ${this.radioWas ? 'the radio in the middle of a word, ' : ''}the birds outside, the hum of the room. Ten seconds of nothing. The waitress did not look up: "${MORNING_LINE}"`);
      d.save();
    });
    step(C.cooler, () => { d.cooler.on(); d.audio.holdRoomTone(1, 0.5); });
    step(C.radio, () => d.radio(1));
    step(C.bird, () => d.audio.night?.back(0.3));
    this.cue = this.t;
    if (this.t >= C.end) this.letGo(false);
  }

  /** Now, wherever the clock is (the developer menu, the tests): false when it cannot. */
  now(s: MorningIn) {
    if (this.phase === 'held' || !s.area) return false;
    this.d.seen.delete(MORNING_KEY);
    this.begin(s.clock);
    return true;
  }

  private begin(clock: number) {
    const d = this.d;
    this.phase = 'held'; this.t = 0; this.cue = 0; this.at = clock;
    this.radioWas = d.radioHeard();
    d.seen.add(MORNING_KEY);
    d.cooler.off();
    d.audio.holdRoomTone(0, 0.02);
    d.audio.holdThunder(0);
    d.audio.night?.still();
    d.radio(0);
    d.saw();
    d.save();
  }
  /** Everything back: at the end, one thing after another as it came; cut short, all at once. */
  private letGo(cut: boolean) {
    const d = this.d;
    if (cut || !d.cooler.running) d.cooler.on();
    d.audio.holdRoomTone(1, cut ? 0.2 : 0.5);
    d.radio(1);
    if (cut || d.audio.night?.isStill) d.audio.night?.back(cut ? 4 : 0.3);
    d.audio.holdThunder(1);
    this.phase = 'done';
  }

  /** A night begun or a case opened: as the case has it, every sound as it should be. */
  reset() {
    if (this.phase === 'held') this.letGo(true);
    this.phase = this.had ? 'done' : 'idle'; this.t = 0; this.cue = 0; this.at = null;
  }
}
