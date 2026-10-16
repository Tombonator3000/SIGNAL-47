import * as THREE from 'three';
import type { AudioSys } from './Audio';
import { syllables, PITCH, secsOf, type Voice } from './radio';

// The CB radio under the dash of the truck (Tom's improvements, 16 October: the CB is modelled
// but did nothing). Someone calls the truck: the squelch opens with a burst of hiss, a voice
// mumbles like the grown-ups in Peanuts, as the AM radio's voices do (core/radio.ts, the same
// horn behind a mute that opens on every syllable), each line captioned, and the squelch
// closes with a click of hiss. Through a narrower, harder speaker than the AM set, from the
// dash, at the radios' own level (AudioSys.radioBus).

export interface CBLine { text: string; who: Voice; by: string }
const LEAD = 0.45, GAP = 0.55;

export class CB {
  /** A call is going on. */
  busy = false;
  private stopAll: (() => void) | null = null;
  constructor(private audio: AudioSys) {}

  /** Plays a call from the truck at `at`, or in the cab with the player (null). onLine is
   *  called as each line begins (for the caption), onEnd when the squelch has closed. Returns
   *  how long it takes, in seconds (0 if there is no sound yet or a call is going on). */
  call(lines: CBLine[], at: THREE.Vector3 | null, onLine: (l: CBLine) => void, onEnd?: () => void) {
    const a = this.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise || this.busy || !lines.length) return 0;
    this.busy = true;
    const t0 = ctx.currentTime + 0.05;
    // the speaker: narrow, overdriven, from the dash
    const out = ctx.createGain(); out.gain.value = 0.85;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1500; bp.Q.value = 1.1;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 2900;
    const sh = ctx.createWaveShaper();
    const curve = new Float32Array(256); for (let i = 0; i < 256; i++) { const x = i / 127.5 - 1; curve[i] = Math.tanh(x * 2.4) / Math.tanh(2.4); }
    sh.curve = curve;
    const pan = at ? a.spatial(at.clone().setY(at.y + 1.1), 1.0, a.radioBus) : a.radioBus;
    if (!pan) { this.busy = false; return 0; }
    out.connect(bp); bp.connect(lp); lp.connect(sh); sh.connect(pan);
    // the voice
    const horn = ctx.createOscillator(); horn.type = 'sawtooth';
    const vib = ctx.createOscillator(); vib.frequency.value = 5.8;
    const vd = ctx.createGain(); vd.gain.value = 2.6; vib.connect(vd); vd.connect(horn.frequency);
    const mute = ctx.createBiquadFilter(); mute.type = 'lowpass'; mute.frequency.value = 400; mute.Q.value = 6.5;
    const voice = ctx.createGain(); voice.gain.value = 0;
    horn.connect(mute); mute.connect(voice); voice.connect(out);
    // the squelch: hiss that bursts open, sits low under the voice, and bursts again at the end
    const n = ctx.createBufferSource(); n.buffer = noise; n.loop = true;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 700;
    const sq = ctx.createGain(); sq.gain.value = 0;
    n.connect(hp); hp.connect(sq); sq.connect(out);
    // the lines, one after the other
    const fq = horn.frequency, mf = mute.frequency, g = voice.gain;
    let at0 = LEAD;
    const starts: number[] = [];
    for (const l of lines) {
      const secs = secsOf(l.text), base = PITCH[l.who];
      starts.push(at0);
      fq.setValueAtTime(base, t0 + at0);
      for (const y of syllables(l.text, secs)) {
        const ts = t0 + at0 + y.at, f = base * y.f;
        fq.setValueAtTime(f, ts); fq.linearRampToValueAtTime(f * 0.94, ts + y.d);
        mf.setValueAtTime(330, ts); mf.linearRampToValueAtTime(1300, ts + y.d * 0.3); mf.linearRampToValueAtTime(420, ts + y.d);
        g.setValueAtTime(0, ts); g.linearRampToValueAtTime(0.5, ts + 0.025); g.setValueAtTime(0.5, ts + y.d * 0.7); g.linearRampToValueAtTime(0, ts + y.d);
      }
      at0 += secs + GAP;
    }
    const end = at0;
    const s = sq.gain;
    s.setValueAtTime(0, t0); s.linearRampToValueAtTime(0.55, t0 + 0.02); s.setTargetAtTime(0.035, t0 + 0.16, 0.06);
    s.setValueAtTime(0.035, t0 + end); s.linearRampToValueAtTime(0.5, t0 + end + 0.03); s.setTargetAtTime(0, t0 + end + 0.2, 0.03);
    horn.start(t0); vib.start(t0); n.start(t0);
    const timers: number[] = [];
    lines.forEach((l, i) => timers.push(window.setTimeout(() => onLine(l), (starts[i] + 0.05) * 1000)));
    const stop = () => {
      for (const x of [horn, vib, n]) { try { x.stop(); } catch { /* stopped */ } }
      out.disconnect(); if (pan !== a.radioBus) pan.disconnect();
      for (const id of timers) clearTimeout(id);
      this.busy = false; this.stopAll = null;
    };
    this.stopAll = stop;
    timers.push(window.setTimeout(() => { stop(); onEnd?.(); }, (end + 0.8) * 1000));
    return end + 0.5;
  }

  /** At once: a new night, the title, a save opened. */
  stop() { this.stopAll?.(); }
}
