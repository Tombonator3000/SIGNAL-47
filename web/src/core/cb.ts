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
/** The speaker's clipping: narrow and overdriven. */
const SHAPE = (() => { const c = new Float32Array(256); for (let i = 0; i < 256; i++) { const x = i / 127.5 - 1; c[i] = Math.tanh(x * 2.4) / Math.tanh(2.4); } return c; })();

/** When the CB opens by itself on the old road (story/Chapter6.ts; FORSLAG.md, 20 October, punkt 6),
 *  seconds from the squelch opening: nobody calls and nobody speaks. On the channel, small and a long
 *  way off, an engine idling like the truck's own (its idle recording, a little slower and a long way
 *  down; the field radio's made engine until that is in), which coughs twice and dies, as the engine on
 *  Tomás' radio does (core/walkie.ts). The hiss goes with it and the channel stays open on nothing for
 *  three seconds; then the squelch tail, and shut. No relay clicks: four and seven are the key's, at
 *  05:29 on the line, and the car radio's carrier is still the first sound of THE EVENT. */
export const CB_ENGINE = { in: 0.5, up: 1.4, cough: [5.6, 6.9], die: 8.0, dead: 9.3, tail: 12.3, end: 12.8 } as const;
/** The engine on the channel's level (a few dB over the cab at 55 mph: tools/f_last_mile.py measures it),
 *  how much slower it runs than the truck's own idle, the recording's make-up, and the hiss under it. */
const CB_LEVEL = 0.6, CB_RATE = 0.9, CB_REC = 3.2, HISS = 0.035;

/** The CB_ENGINE sequence from `t0` (AudioContext time) into `dest`, on any context (the tests render it
 *  offline): the CB's own speaker (band-limited, a little clipping), its squelch, the engine. Returns
 *  stop(), which silences it at once. */
export function engineOnChannel(ctx: BaseAudioContext, dest: AudioNode, noise: AudioBuffer, idle: AudioBuffer | null, t0: number) {
  const E = CB_ENGINE, srcs: AudioScheduledSourceNode[] = [];
  // the speaker: as for a call, with a little more of the low middle left in for an engine
  const out = ctx.createGain(); out.gain.value = 0.85;
  const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 300; hp.Q.value = 0.7;
  const pk = ctx.createBiquadFilter(); pk.type = 'peaking'; pk.frequency.value = 1500; pk.Q.value = 1.1; pk.gain.value = 5;
  const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 2900;
  const sh = ctx.createWaveShaper(); sh.curve = SHAPE;
  out.connect(hp); hp.connect(pk); pk.connect(lp); lp.connect(sh); sh.connect(dest);
  // the squelch: a burst as it opens, the hiss low under the engine and gone with it (an open carrier
  // with nothing on it), and the tail as the carrier drops
  const n = ctx.createBufferSource(); n.buffer = noise; n.loop = true; srcs.push(n);
  const nh = ctx.createBiquadFilter(); nh.type = 'highpass'; nh.frequency.value = 700;
  const sq = ctx.createGain(); sq.gain.value = 0;
  n.connect(nh); nh.connect(sq); sq.connect(out);
  const s = sq.gain;
  s.setValueAtTime(0, t0); s.linearRampToValueAtTime(0.55, t0 + 0.02); s.setTargetAtTime(HISS, t0 + 0.16, 0.06);
  s.setValueAtTime(HISS, t0 + E.die + 0.3); s.linearRampToValueAtTime(0, t0 + E.dead);
  s.setValueAtTime(0, t0 + E.tail); s.linearRampToValueAtTime(0.5, t0 + E.tail + 0.03); s.setTargetAtTime(0, t0 + E.tail + 0.2, 0.03);
  n.start(t0); n.stop(t0 + E.end + 0.2);
  // the engine
  const eng = ctx.createGain(); eng.gain.value = 0; eng.connect(out);
  const g = eng.gain, L = CB_LEVEL;
  g.setValueAtTime(0, t0 + E.in); g.linearRampToValueAtTime(L, t0 + E.up);
  for (const c of E.cough) {
    g.setValueAtTime(L, t0 + c); g.linearRampToValueAtTime(L * 0.08, t0 + c + 0.08); g.linearRampToValueAtTime(L * 1.1, t0 + c + 0.24); g.linearRampToValueAtTime(L, t0 + c + 0.4);
  }
  // it dies as the truck's does (drive/engineSound.ts, stall): chugs, and a wind-down
  g.setValueAtTime(L, t0 + E.die);
  for (const [dt, k] of [[0.08, 1.25], [0.3, 0.45], [0.42, 0.85], [0.72, 0.25], [0.84, 0.45], [1.3, 0]] as const) g.linearRampToValueAtTime(L * k, t0 + E.die + dt);
  const stopAt = t0 + E.dead + 0.1;
  if (idle) {
    const src = ctx.createBufferSource(); src.buffer = idle; src.loop = true; srcs.push(src);
    const rg = ctx.createGain(); rg.gain.value = CB_REC; src.connect(rg); rg.connect(eng);
    const r = src.playbackRate;
    r.setValueAtTime(CB_RATE, t0);
    for (const c of E.cough) { r.setValueAtTime(CB_RATE, t0 + c); r.linearRampToValueAtTime(CB_RATE * 0.72, t0 + c + 0.12); r.linearRampToValueAtTime(CB_RATE, t0 + c + 0.38); }
    r.setValueAtTime(CB_RATE, t0 + E.die); r.exponentialRampToValueAtTime(0.22, t0 + E.die + 1.25);
    src.start(t0 + E.in, (t0 * 3.7) % Math.max(0.1, idle.duration - 0.1)); src.stop(stopAt);
  } else {
    // (the field radio's engine: a low saw beating at the firing rate, through a muffled exhaust; the beat
    // is round here, so on the channel's band it is a lope and not a train of clicks)
    const osc = ctx.createOscillator(); osc.type = 'sawtooth';
    const elp = ctx.createBiquadFilter(); elp.type = 'lowpass'; elp.frequency.value = 520;
    const elp2 = ctx.createBiquadFilter(); elp2.type = 'lowpass'; elp2.frequency.value = 520; elp.connect(elp2);
    const fire = ctx.createOscillator(); fire.type = 'sine'; fire.frequency.value = 17;
    const fg = ctx.createGain(); fg.gain.value = 0.45; fire.connect(fg);
    const chop = ctx.createGain(); chop.gain.value = 0.55; fg.connect(chop.gain);
    const lift = ctx.createGain(); lift.gain.value = 2.2;
    osc.connect(elp); elp2.connect(lift); lift.connect(chop); chop.connect(eng);
    const f = osc.frequency, ff = fire.frequency;
    f.setValueAtTime(31, t0); ff.setValueAtTime(15.5, t0);
    for (const c of E.cough) {
      f.setValueAtTime(31, t0 + c); f.linearRampToValueAtTime(22, t0 + c + 0.12); f.linearRampToValueAtTime(31, t0 + c + 0.38);
      ff.setValueAtTime(15.5, t0 + c); ff.linearRampToValueAtTime(10, t0 + c + 0.12); ff.linearRampToValueAtTime(15.5, t0 + c + 0.38);
    }
    f.setValueAtTime(31, t0 + E.die); f.exponentialRampToValueAtTime(8, t0 + E.die + 1.25);
    ff.setValueAtTime(15.5, t0 + E.die); ff.exponentialRampToValueAtTime(4, t0 + E.die + 1.25);
    for (const o of [osc, fire]) { o.start(t0 + E.in); o.stop(stopAt); srcs.push(o); }
  }
  // a dull pop in the exhaust with each cough
  for (const c of E.cough) {
    const p = ctx.createBufferSource(); p.buffer = noise; srcs.push(p);
    const pb = ctx.createBiquadFilter(); pb.type = 'bandpass'; pb.frequency.value = 420; pb.Q.value = 1.4;
    const pg = ctx.createGain(); pg.gain.setValueAtTime(0, t0 + c + 0.02); pg.gain.linearRampToValueAtTime(0.5, t0 + c + 0.03); pg.gain.exponentialRampToValueAtTime(0.001, t0 + c + 0.18);
    p.connect(pb); pb.connect(pg); pg.connect(eng);
    p.start(t0 + c + 0.02, (c * 0.37) % Math.max(0.1, noise.duration - 0.3)); p.stop(t0 + c + 0.22);
  }
  return {
    stop() {
      for (const x of srcs) { try { x.stop(); } catch { /* stopped */ } }
      out.disconnect();
    },
  };
}

export class CB {
  /** A call is going on. */
  busy = false;
  /** While the engine is on the channel, where it comes out (the tests measure it there). */
  out: AudioNode | null = null;
  private stopAll: (() => void) | null = null;
  constructor(private audio: AudioSys) {}

  /** Plays a call from the truck at `at`, or in the cab with the player (null). onLine is
   *  called as each line begins (for the caption), onEnd when the squelch has closed. Returns
   *  how long it takes, in seconds (0 if there is no sound yet or a call is going on). */
  call(lines: CBLine[], at: THREE.Vector3 | null, onLine: (l: CBLine, duration?: number) => void, onEnd?: () => void) {
    const a = this.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise || this.busy || !lines.length) return 0;
    a.stopPhoneCalls(); a.onPhoneCall?.();
    this.busy = true;
    const selected = lines.map((line) => line.by === 'N. VEGA' && line.who === 'woman' ? a.characterBuffer('bacall', line.text) : undefined);
    const recorded = selected.every((buffer) => buffer && Number.isFinite(buffer.duration) && buffer.duration > 0);
    const sources: AudioBufferSourceNode[] = [];
    const t0 = ctx.currentTime + 0.05;
    // the speaker: narrow, overdriven, from the dash
    const out = ctx.createGain(); out.gain.value = 0.85;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1500; bp.Q.value = 1.1;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 2900;
    const sh = ctx.createWaveShaper(); sh.curve = SHAPE;
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
    const durations: number[] = [];
    for (const [i, l] of lines.entries()) {
      const secs = recorded ? selected[i]!.duration : secsOf(l.text), base = PITCH[l.who];
      durations.push(secs);
      starts.push(at0);
      if (recorded) {
        const source = ctx.createBufferSource(); source.buffer = selected[i]!; source.connect(out);
        source.start(t0 + at0); sources.push(source); at0 += secs + GAP; continue;
      }
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
    // Silent source-end markers use the same pausable clock as the speaker.
    // No wall timer can caption the next sentence or finish the call during Pause.
    const marker = (seconds: number, callback: () => void) => {
      const source = ctx.createBufferSource(); source.buffer = ctx.createBuffer(1, 1, ctx.sampleRate);
      source.connect(out); source.onended = () => { source.disconnect(); callback(); };
      sources.push(source); source.start(t0 + seconds);
    };
    lines.forEach((l, i) => marker(starts[i], () => onLine(l, durations[i])));
    const stop = () => {
      for (const source of sources) { source.onended = null; try { source.stop(); } catch { /* stopped */ } source.disconnect(); }
      for (const x of [horn, vib, n]) { try { x.stop(); } catch { /* stopped */ } }
      out.disconnect(); if (pan !== a.radioBus) pan.disconnect();
      this.busy = false; this.stopAll = null;
    };
    this.stopAll = stop;
    marker(end + 0.8, () => { stop(); onEnd?.(); });
    return end + 0.5;
  }

  /** The channel opens by itself with an engine on it (CB_ENGINE, engineOnChannel): in the cab, a little
   *  to the passenger side where the set hangs, into `dest` (story/Chapter6.ts: the game's own sounds,
   *  so the radios' level does not take it away). idle: the truck's idle recording, or null. Busy until
   *  stop() or the end. Returns how long it takes (0 if there is no sound yet or a call is going on). */
  engine(idle: AudioBuffer | null, dest: AudioNode) {
    const a = this.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise || this.busy) return 0;
    this.busy = true;
    const pan = ctx.createStereoPanner(); pan.pan.value = 0.35; pan.connect(dest);
    const t0 = ctx.currentTime + 0.05, h = engineOnChannel(ctx, pan, noise, idle, t0);
    // (and gone by itself at the end, on the same clock, if nothing has stopped it first)
    const end = ctx.createBufferSource(); end.buffer = ctx.createBuffer(1, 1, ctx.sampleRate); end.connect(pan);
    const stop = () => { end.onended = null; try { end.stop(); } catch { /* stopped */ } h.stop(); pan.disconnect(); this.busy = false; this.stopAll = null; this.out = null; };
    end.onended = () => { if (this.stopAll === stop) stop(); };
    end.start(t0 + CB_ENGINE.end + 0.4);
    this.stopAll = stop; this.out = pan;
    return CB_ENGINE.end;
  }

  /** At once: a new night, the title, a save opened. */
  stop() { this.stopAll?.(); }
}
