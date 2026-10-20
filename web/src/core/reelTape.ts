import type { AudioSys } from './Audio';
import { boyCallTimes, type Radio } from './radio';

// Earl's reel from 9/22/81, on the studio monitor at 1420 AM (story/lostNights.ts; FORSLAG.md, 20
// October, item 11). A call taken off the telephone line on a night the station was off the air:
// the line's hiss and hum, and the boy who called the Night Line tonight, saying what he said
// tonight, word for word (the same recordings: core/radio.ts, the caller bank). Where Earl answered
// him tonight there is nothing on the tape at all, not even the hiss, for exactly as long as Earl
// talked (tonight's call as it went out: radio.boyCallTimes). The deck's motor and the reels go on
// through the holes; it is the tape that has nothing on it. At the end, a click on the line, as if a
// receiver were put down at the other end, and Earl stops the deck. All of it on the AudioContext's
// clock, and the lines for the conversation panel on the same clock, so a pause holds both.
// With the recorded callers turned off (or a clip that will not load), the boy is the caption alone,
// and the tape and its holes are the same.

export type TapeCue =
  | { at: number; kind: 'deck' | 'start' | 'click' | 'end' }
  | { at: number; kind: 'boy'; text: string; secs: number }
  | { at: number; kind: 'hole'; secs: number; i: number };

/** Seconds from play() before the tape runs: Earl goes in and threads it on the deck. */
export const TAPE_BEFORE = 4.6;
/** Hiss on the tape before the boy's first word. */
const LEAD = 1.5;

export class ReelTape {
  /** The reel is playing (one thing at a time: main.ts holds the music and the night for it). */
  busy = false;
  private gen = 0;
  private timer: number | null = null;
  private abort: AbortController | null = null;
  private srcs: AudioScheduledSourceNode[] = [];
  private nodes: AudioNode[] = [];
  private duckedAt: number | null = null;
  /** What the last play had on it, for the tests: whether the boy's own voice played, the clock it
   *  started on (the AudioContext's, or performance.now() in seconds without sound), its cues. */
  voiced = false;
  startedAt = 0;
  cues: TapeCue[] = [];

  constructor(private audio: AudioSys, private radio: Radio) {}

  /** When each thing on the reel comes, in seconds from play(). */
  plan(): TapeCue[] {
    const cues: TapeCue[] = [{ at: TAPE_BEFORE - 0.55, kind: 'deck' }, { at: TAPE_BEFORE, kind: 'start' }];
    let at = TAPE_BEFORE + LEAD, hole = 0;
    for (const l of boyCallTimes(this.radio)) {
      if (l.boy) cues.push({ at, kind: 'boy', text: l.text, secs: l.secs });
      else cues.push({ at, kind: 'hole', secs: l.secs, i: hole++ });
      at += l.secs + l.gap;
    }
    // after the last word: the line a moment more, the click at the other end, and the deck stopped
    cues.push({ at: at + 0.5, kind: 'click' }, { at: at + 2.4, kind: 'end' });
    return cues;
  }

  /** Plays the reel. `cue` gets each thing on it as it comes; `done(true)` once the deck has been
   *  stopped at the end, `done(false)` if it was stopped before (stop()). */
  play(o: { cue: (c: TapeCue) => void; done: (complete: boolean) => void }) {
    this.stop();
    const gen = ++this.gen;
    this.busy = true; this.voiced = false;
    const cues = this.plan();
    this.onDone = (complete) => { if (gen === this.gen || !complete) o.done(complete); };
    const ctx = this.audio.ctx && this.audio.ctx.state === 'running' ? this.audio.ctx : null;
    const now = () => ctx ? ctx.currentTime : performance.now() / 1000;
    const t0 = now();
    this.startedAt = t0; this.cues = cues;
    if (ctx) {
      // the boy's recordings first (they are small; the deck is still being threaded meanwhile)
      const bank = this.radio.callerBank;
      const clips = this.radio.callerVoice ? cues.flatMap((c) => c.kind === 'boy' ? bank.resolve(c.text, 0, 'boy') ?? [] : []) : [];
      const boys = cues.filter((c) => c.kind === 'boy').length;
      this.abort = new AbortController();
      const loaded = clips.length === boys ? bank.load(clips, ctx, this.abort.signal).catch(() => null) : Promise.resolve(null);
      void loaded.then((buffers) => {
        if (gen !== this.gen) return;
        // (a voice that comes too late to start with its caption is left out: the caption stands)
        const late = now() - t0 > TAPE_BEFORE + LEAD - 0.3;
        const voice = buffers && !late ? cues.filter((c) => c.kind === 'boy').map((_, i) => buffers.get(clips[i].id) ?? null) : null;
        this.voiced = !!voice && voice.every(Boolean);
        this.build(ctx, t0, cues, this.voiced ? voice as AudioBuffer[] : null);
      });
    }
    let k = 0;
    const step = () => {
      if (gen !== this.gen) return;
      const t = now() - t0;
      while (k < cues.length && cues[k].at <= t) {
        const c = cues[k++];
        // in a hole the night outside goes with it, all but the deck; then back to as it was
        if (c.kind === 'hole' && ctx) {
          const end = cues[cues.length - 1].at;
          this.audio.night?.hush(c.secs, 0.03, 0.015, 0.2);
          window.setTimeout(() => { if (gen === this.gen) this.audio.night?.hush(Math.max(0.5, end - c.at - c.secs + 2), 0.22, 0.05, 2.5); }, c.secs * 1000);
        }
        o.cue(c);
        if (c.kind === 'end') { this.release(); this.busy = false; this.onDone = null; o.done(true); return; }
      }
      this.timer = window.setTimeout(step, 40);
    };
    step();
  }
  private onDone: ((complete: boolean) => void) | null = null;

  /** Stops the reel where it is (the conversation closed): not heard to the end. */
  stop() {
    const was = this.busy;
    ++this.gen;
    if (this.timer !== null) { clearTimeout(this.timer); this.timer = null; }
    this.abort?.abort(); this.abort = null;
    this.release(true);
    this.busy = false;
    const done = this.onDone; this.onDone = null;
    if (was) done?.(false);
  }

  // ---------- the sound ----------
  private build(ctx: AudioContext, t0: number, cues: TapeCue[], voice: AudioBuffer[] | null) {
    const a = this.audio, at = (s: number) => t0 + s, add = <T extends AudioNode>(n: T) => { this.nodes.push(n); return n; };
    const end = cues.find((c) => c.kind === 'end')!.at, start = TAPE_BEFORE;
    // the radios and the night held down while it plays (the monitor is not loud)
    this.duckedAt = ctx.currentTime;
    a.radioBus?.gain.setTargetAtTime(a.radioLevel * 0.12, ctx.currentTime, 0.4);
    a.night?.hush(end + 2, 0.22, 0.6, 2.5);
    // the studio's monitor: what is on the tape comes through the band of the telephone line it was
    // taken off, and a small speaker's honk
    const out = add(ctx.createGain()); out.gain.value = 0.85; out.connect(a.sfx);
    const mon = add(ctx.createBiquadFilter()); mon.type = 'peaking'; mon.frequency.value = 1500; mon.Q.value = 0.9; mon.gain.value = 5; mon.connect(out);
    const lp = add(ctx.createBiquadFilter()); lp.type = 'lowpass'; lp.frequency.value = 3300; lp.connect(mon);
    const hp = add(ctx.createBiquadFilter()); hp.type = 'highpass'; hp.frequency.value = 300; hp.connect(lp);
    // the tape: everything on it goes through here, and where Earl answered tonight it is nothing
    const tape = add(ctx.createGain()); tape.gain.value = 0; tape.connect(hp);
    const g = tape.gain;
    g.setValueAtTime(0, at(start)); g.linearRampToValueAtTime(1, at(start + 0.25));
    for (const c of cues) {
      if (c.kind !== 'hole') continue;
      g.setTargetAtTime(0, at(c.at), 0.006);
      g.setTargetAtTime(1, at(c.at + c.secs), 0.006);
    }
    g.setTargetAtTime(0, at(end - 0.05), 0.02);
    const noise = a.noiseBuffer();
    if (noise) {
      const n = ctx.createBufferSource(); n.buffer = noise; n.loop = true;
      const ng = add(ctx.createGain()); ng.gain.value = 0.06; n.connect(ng); ng.connect(tape);
      n.start(at(start)); n.stop(at(end + 0.2)); this.srcs.push(n);
    }
    // the line's hum, faint, on the tape too
    const hum = ctx.createOscillator(); hum.frequency.value = 60;
    const hg = add(ctx.createGain()); hg.gain.value = 0.01; hum.connect(hg); hg.connect(tape);
    hum.start(at(start)); hum.stop(at(end + 0.2)); this.srcs.push(hum);
    // the boy, with the little waver an old deck gives a voice
    if (voice) {
      const vg = add(ctx.createGain()); vg.gain.value = 1.15; vg.connect(tape);
      let i = 0;
      for (const c of cues) {
        if (c.kind !== 'boy') continue;
        const src = ctx.createBufferSource(); src.buffer = voice[i++];
        const wow = ctx.createOscillator(); wow.frequency.value = 0.55;
        const wg = add(ctx.createGain()); wg.gain.value = 0.004; wow.connect(wg); wg.connect(src.playbackRate);
        src.connect(vg); src.start(at(c.at)); wow.start(at(c.at)); wow.stop(at(c.at + c.secs + 0.5));
        this.srcs.push(src, wow);
      }
    }
    // the click on the line, on the tape
    const click = cues.find((c) => c.kind === 'click')!;
    const cl = a.play('click', { gain: 0.55, rate: 0.8, when: Math.max(0, at(click.at) - ctx.currentTime), dest: tape });
    if (cl) { this.srcs.push(cl.src); this.nodes.push(cl.gain); }
    // the deck itself, in the studio, not on the tape: the transport key, the motor and the reels
    // going round (they go on through the holes), and the key again at the end
    const sw = a.or('small_switch', 'switch');
    for (const [w, gain, rate] of [[start - 0.55, 0.42, 0.8], [end - 0.1, 0.38, 0.72]] as const) {
      const s = a.play(sw, { gain, rate, when: Math.max(0, at(w) - ctx.currentTime) });
      if (s) { this.srcs.push(s.src); this.nodes.push(s.gain); }
    }
    const motor = ctx.createOscillator(); motor.type = 'sawtooth'; motor.frequency.value = 22;
    const ml = add(ctx.createBiquadFilter()); ml.type = 'lowpass'; ml.frequency.value = 170;
    const mg = add(ctx.createGain()); mg.gain.value = 0;
    motor.connect(ml); ml.connect(mg); mg.connect(a.sfx);
    mg.gain.setValueAtTime(0, at(start - 0.5)); mg.gain.linearRampToValueAtTime(0.022, at(start - 0.1));
    motor.frequency.setValueAtTime(22, at(start - 0.5)); motor.frequency.linearRampToValueAtTime(38, at(start - 0.1));
    mg.gain.setTargetAtTime(0, at(end), 0.25); motor.frequency.setTargetAtTime(12, at(end), 0.3);
    motor.start(at(start - 0.5)); motor.stop(at(end + 1.6)); this.srcs.push(motor);
    // the reels' flanges, a soft tick each time round
    for (let t = start + 0.4; t < end; t += 1.35) {
      const r = a.play('click', { gain: 0.03, rate: 2.4, lp: 1800, when: Math.max(0, at(t) - ctx.currentTime) });
      if (r) { this.srcs.push(r.src); this.nodes.push(r.gain); }
    }
  }
  /** Lets go of the sound; `now`: at once (stopped early), else the tails ring out by themselves. */
  private release(now = false) {
    const ctx = this.audio.ctx;
    if (ctx && this.duckedAt !== null) {
      this.audio.radioBus?.gain.setTargetAtTime(this.audio.radioLevel, ctx.currentTime, 0.6);
      if (now) this.audio.night?.hush(0.1, 1, 0.3, 1.5);
    }
    this.duckedAt = null;
    const srcs = this.srcs, nodes = this.nodes;
    this.srcs = []; this.nodes = [];
    const go = () => {
      for (const s of srcs) { s.onended = null; try { s.stop(); } catch { /* stopped */ } s.disconnect(); }
      for (const n of nodes) n.disconnect();
    };
    if (now || !ctx) go(); else setTimeout(go, 2000);
  }
}
