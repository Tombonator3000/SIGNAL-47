import * as THREE from 'three';
import { SignalVoice, pulseAt } from './signalVoice';

// The signal processor on the control room rack: it plays back the tape of the locked
// anomaly, either as it came in (RAW, slowed down if you like) or harmonized, where every
// pulse of the 4/7 group becomes a note with a fifth and an octave over it, the first of
// each group gets a bass note, a slow pad holds underneath and a long reverb carries it.
// It sounds close to music without being any. The player sets pitch, time and space.
// All synthesis. The sound comes from the rack, so it keeps playing as you walk around.

export interface DecoderParams { mode: 'raw' | 'harmonized'; pitch: number; stretch: number; space: number }

// the 4-group rises through a minor seventh chord, the 7-group falls back down
const DEG_A = [0, 3, 7, 10];
const DEG_B = [12, 10, 7, 5, 3, 2, 0];
const F0 = 220;

export class DecoderPlayer {
  playing = false;
  params: DecoderParams = { mode: 'harmonized', pitch: 0, stretch: 2, space: 0.6 };
  /** Which pulse sounds now: 0 to 3 the 4-group, 4 to 10 the 7-group, -1 between. */
  current = -1;
  private ctx: AudioContext;
  private out: GainNode;
  private dry: GainNode;
  private wet: GainNode;
  private verb: ConvolverNode;
  private raw: SignalVoice | null = null;
  private pad: { gain: GainNode; oscs: OscillatorNode[]; filter: BiquadFilterNode } | null = null;
  private t0 = 0;            // audio time of the start of the current cycle
  private next = 0;          // index of the next pulse to schedule within the cycle (0..10)
  private cycle = 0;
  private started = 0;
  private marks: { at: number; i: number }[] = [];

  constructor(ctx: AudioContext, dest: AudioNode, at: THREE.Vector3, private noise: AudioBuffer) {
    this.ctx = ctx;
    const pan = ctx.createPanner();
    pan.panningModel = 'equalpower'; pan.distanceModel = 'inverse'; pan.refDistance = 1.6; pan.rolloffFactor = 1.1; pan.maxDistance = 40;
    if ((pan as any).positionX) { pan.positionX.value = at.x; pan.positionY.value = at.y; pan.positionZ.value = at.z; } else (pan as any).setPosition(at.x, at.y, at.z);
    pan.connect(dest);
    this.out = ctx.createGain(); this.out.gain.value = 0; this.out.connect(pan);
    this.dry = ctx.createGain(); this.dry.connect(this.out);
    this.wet = ctx.createGain(); this.verb = ctx.createConvolver(); this.verb.buffer = impulse(ctx, 3.6);
    this.wet.connect(this.verb); this.verb.connect(this.out);
  }

  start(p?: Partial<DecoderParams>) {
    if (p) Object.assign(this.params, p);
    this.stopVoices();
    const now = this.ctx.currentTime;
    this.playing = true;
    this.t0 = now + 0.15; this.next = 0; this.cycle = 0; this.started = now; this.marks = [];
    this.out.gain.cancelScheduledValues(now);
    this.out.gain.setTargetAtTime(0.9, now, 0.2);
    this.applySpace();
    if (this.params.mode === 'raw') {
      this.raw = new SignalVoice(this.ctx, this.dry, this.noise);
      this.raw.frame(1, 0.9, 0);
    } else this.startPad();
  }
  set(p: Partial<DecoderParams>) {
    const mode = this.params.mode;
    Object.assign(this.params, p);
    if (!this.playing) return;
    if (p.mode && p.mode !== mode) { this.start(); return; }
    this.applySpace();
    if (this.pad) this.tunePad();
  }
  stop() {
    if (!this.playing) return;
    this.playing = false;
    this.current = -1;
    this.out.gain.setTargetAtTime(0, this.ctx.currentTime, 0.25);
    const raw = this.raw, pad = this.pad;
    this.raw = null; this.pad = null;
    setTimeout(() => { raw?.stop(); if (pad) for (const o of pad.oscs) { try { o.stop(); } catch { /* stopped */ } } }, 1200);
  }

  private stopVoices() {
    this.raw?.stop(); this.raw = null;
    if (this.pad) { const pad = this.pad; this.pad = null; pad.gain.gain.setTargetAtTime(0, this.ctx.currentTime, 0.1); setTimeout(() => pad.oscs.forEach((o) => { try { o.stop(); } catch { /* stopped */ } }), 600); }
  }
  private applySpace() {
    const now = this.ctx.currentTime, s = this.params.space;
    this.wet.gain.setTargetAtTime(0.15 + 1.1 * s, now, 0.1);
    this.dry.gain.setTargetAtTime(1 - 0.45 * s, now, 0.1);
  }

  // A slow pad under the harmonized playback: two detuned saws through a low filter.
  private startPad() {
    const c = this.ctx;
    const gain = c.createGain(); gain.gain.value = 0;
    const filter = c.createBiquadFilter(); filter.type = 'lowpass'; filter.frequency.value = 650; filter.Q.value = 0.7;
    filter.connect(gain); gain.connect(this.dry); gain.connect(this.wet);
    const oscs = [0, 7, -5].map((cents) => { const o = c.createOscillator(); o.type = 'sawtooth'; o.detune.value = cents; o.connect(filter); o.start(); return o; });
    const lfo = c.createOscillator(); lfo.frequency.value = 0.07; const ld = c.createGain(); ld.gain.value = 260; lfo.connect(ld); ld.connect(filter.frequency); lfo.start(); oscs.push(lfo);
    this.pad = { gain, oscs, filter };
    this.tunePad();
  }
  private tunePad() {
    const f = F0 / 2 * Math.pow(2, this.params.pitch / 12);
    const now = this.ctx.currentTime;
    this.pad!.oscs.slice(0, 3).forEach((o, i) => o.frequency.setTargetAtTime(i === 2 ? f * 1.5 : f, now, 0.3));
  }

  // One note of the harmonized playback: a soft bell (sine, octave and a slightly sharp
  // twelfth), the harmonizer's fifth and octave a few milliseconds late, a bass under the
  // first note of a group.
  private note(at: number, deg: number, len: number, first: boolean) {
    const c = this.ctx;
    const f = F0 * Math.pow(2, (deg + this.params.pitch) / 12);
    const voice = (freq: number, gain: number, delay: number, dur: number) => {
      const t = at + delay;
      const env = c.createGain(); env.gain.setValueAtTime(0, t);
      env.gain.linearRampToValueAtTime(gain, t + Math.min(0.04, 0.01 * this.params.stretch));
      env.gain.exponentialRampToValueAtTime(0.0008, t + dur);
      env.connect(this.dry); env.connect(this.wet);
      for (const [mul, g, type] of [[1, 1, 'sine'], [2, 0.28, 'triangle'], [3.004, 0.07, 'sine']] as const) {
        const o = c.createOscillator(); o.type = type; o.frequency.value = freq * mul;
        const og = c.createGain(); og.gain.value = g; o.connect(og); og.connect(env);
        o.start(t); o.stop(t + dur + 0.05);
      }
    };
    const dur = 0.35 + len * 2.2;
    voice(f, 0.11, 0, dur);
    voice(f * Math.pow(2, 7 / 12), 0.045, 0.012, dur * 0.9);
    voice(f * 2, 0.03, 0.019, dur * 0.8);
    if (first) {
      const t = at, o = c.createOscillator(); o.frequency.value = F0 / 4 * Math.pow(2, this.params.pitch / 12);
      const g = c.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.16, t + 0.02); g.gain.exponentialRampToValueAtTime(0.0008, t + 1.2 * this.params.stretch);
      o.connect(g); g.connect(this.dry); g.connect(this.wet); o.start(t); o.stop(t + 1.3 * this.params.stretch);
    }
    if (this.pad) {
      const g = this.pad.gain.gain;
      g.setTargetAtTime(first ? 0.05 : 0.035, at, 0.4 * this.params.stretch);
    }
  }

  /** Every frame: schedule what is due, and say which pulse is sounding (for the displays). */
  update() {
    if (!this.playing) return;
    const c = this.ctx, now = c.currentTime, k = this.params.stretch;
    if (this.raw) {
      // the tape, slowed down: the same signal on a slower clock
      this.raw.frame(1, 0.9, (now - this.started) / k);
      const p = pulseAt((now - this.started) / k);
      this.current = p.group ? 0 : -1;
      return;
    }
    const period = 5.2 * k;
    while (true) {
      const i = this.next;
      const local = i < 4 ? i * 0.22 : 2.1 + (i - 4) * 0.22;
      const at = this.t0 + local * k;
      if (at > now + 0.25) break;
      if (at >= now - 0.05) {
        this.note(at, i < 4 ? DEG_A[i] : DEG_B[i - 4], 0.11 * k, i === 0 || i === 4);
        this.marks.push({ at, i });
      }
      this.next++;
      if (this.next > 10) { this.next = 0; this.cycle++; this.t0 += period; }
    }
    while (this.marks.length > 1 && this.marks[1].at <= now) this.marks.shift();
    const m = this.marks[0];
    this.current = m && m.at <= now && now - m.at < 0.22 * k ? m.i : -1;
  }
}

// A reverb tail: decaying noise in both channels, a little darker at the end.
function impulse(ctx: AudioContext, secs: number) {
  const len = Math.floor(ctx.sampleRate * secs);
  const b = ctx.createBuffer(2, len, ctx.sampleRate);
  for (let ch = 0; ch < 2; ch++) {
    const d = b.getChannelData(ch);
    let lp = 0;
    for (let i = 0; i < len; i++) {
      const x = i / len;
      lp += ((Math.random() * 2 - 1) - lp) * (0.9 - 0.6 * x);
      d[i] = lp * Math.pow(1 - x, 2.4);
    }
  }
  return b;
}
