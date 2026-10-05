import * as THREE from 'three';
import { SignalVoice } from './signalVoice';
import phoneRing from '../assets/audio/phone_ring.mp3';
import printer from '../assets/audio/printer.mp3';
import ceramic from '../assets/audio/ceramic.mp3';
import wind from '../assets/audio/wind.mp3';
import titleMusic from '../assets/audio/title_music.mp3';
// The night outside, steps by surface, doors and paper: CC0 recordings from Freesound, by
// way of Tom's own projects (morbidium, Loincloth-Legends). See THIRD_PARTY_NOTICES.md.
import crickets from '../assets/audio/crickets.mp3';
import owl from '../assets/audio/owl.mp3';
import dog0 from '../assets/audio/dog0.mp3';
import dog1 from '../assets/audio/dog1.mp3';
import gust from '../assets/audio/gust.mp3';
import stepConcrete0 from '../assets/audio/step_concrete0.mp3';
import stepConcrete1 from '../assets/audio/step_concrete1.mp3';
import stepConcrete2 from '../assets/audio/step_concrete2.mp3';
import stepWood0 from '../assets/audio/step_wood0.mp3';
import stepWood1 from '../assets/audio/step_wood1.mp3';
import stepDirt from '../assets/audio/step_dirt.mp3';
import doorCreak0 from '../assets/audio/door_creak0.mp3';
import doorCreak1 from '../assets/audio/door_creak1.mp3';
import doorMetal from '../assets/audio/door_metal.mp3';
import doorClose from '../assets/audio/door_close.mp3';
import paper0 from '../assets/audio/paper0.mp3';
import paper1 from '../assets/audio/paper1.mp3';
import click from '../assets/audio/click.mp3';
import sw from '../assets/audio/switch.mp3';
import thudSoft from '../assets/audio/thud_soft.mp3';

const SOURCES: Record<string, string> = {
  phoneRing, printer, ceramic, wind, titleMusic, click, switch: sw, thudSoft,
  crickets, owl, dog0, dog1, gust, stepConcrete0, stepConcrete1, stepConcrete2, stepWood0, stepWood1, stepDirt,
  doorCreak0, doorCreak1, doorMetal, doorClose, paper0, paper1,
};

// What the player walks on, and how a step on it sounds: which recordings, how loud, the
// pitch, a low-pass for soft floors, and grit on top for dirt and gravel.
export type Surface = 'tile' | 'concrete' | 'dirt' | 'wood' | 'carpet';
const STEPS: Record<Surface, { names: string[]; gain: number; rate: number; lp?: number; grit?: number }> = {
  tile: { names: ['stepConcrete0', 'stepConcrete1', 'stepConcrete2'], gain: 0.34, rate: 1.12 },
  concrete: { names: ['stepConcrete0', 'stepConcrete1', 'stepConcrete2'], gain: 0.42, rate: 0.94 },
  dirt: { names: ['stepDirt'], gain: 0.42, rate: 0.72, grit: 0.05 },
  wood: { names: ['stepWood0', 'stepWood1'], gain: 0.4, rate: 0.95 },
  carpet: { names: ['stepWood0', 'stepWood1'], gain: 0.2, rate: 0.9, lp: 900 },
};

function decodeDataUrl(url: string): ArrayBuffer {
  const b64 = url.slice(url.indexOf(',') + 1);
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out.buffer;
}

export class AudioSys {
  ctx: AudioContext | null = null;
  master!: GainNode; sfx!: GainNode; music!: GainNode; amb!: GainNode;
  buf: Record<string, AudioBuffer> = {};
  private noise!: AudioBuffer; private brown!: AudioBuffer;
  volume = 0.8;
  ready = false;
  private loops = new Map<string, { src: AudioBufferSourceNode; gain: GainNode }>();
  carrier: { osc: OscillatorNode; gain: GainNode; stat: GainNode } | null = null;
  signal: SignalVoice | null = null;   // the anomaly as it comes in (core/signalVoice.ts)
  roomTone: GainNode | null = null;
  motor: { gain: GainNode; osc: OscillatorNode } | null = null;

  private loading: Promise<void> | null = null;

  // Decode every sound while the title screen is up, so Start does not wait on it.
  // An OfflineAudioContext needs no user gesture and logs no autoplay warning. Its
  // buffers play fine in the real context that unlock() creates later.
  preload() {
    if (this.loading) return this.loading;
    const O = window.OfflineAudioContext || (window as any).webkitOfflineAudioContext;
    let dec: BaseAudioContext;
    try { dec = new O(1, 1, 48000); } catch { return null; }
    this.loading = this.decodeAll(dec);
    return this.loading;
  }

  private async decodeAll(ctx: BaseAudioContext) {
    await Promise.all(Object.entries(SOURCES).map(async ([k, url]) => {
      if (this.buf[k]) return;
      try {
        const data = url.startsWith('data:') ? decodeDataUrl(url) : await (await fetch(url)).arrayBuffer();
        this.buf[k] = await ctx.decodeAudioData(data);
      } catch (err) { console.warn('audio load failed', k, err); }
    }));
  }

  async unlock() {
    if (this.ctx) { if (this.ctx.state !== 'running') await this.ctx.resume(); return; }
    const C = window.AudioContext || (window as any).webkitAudioContext;
    const ctx: AudioContext = new C();
    this.ctx = ctx;
    this.master = ctx.createGain(); this.master.gain.value = this.volume; this.master.connect(ctx.destination);
    this.sfx = ctx.createGain(); this.sfx.connect(this.master);
    this.music = ctx.createGain(); this.music.gain.value = 0.7; this.music.connect(this.master);
    this.amb = ctx.createGain(); this.amb.gain.value = 1; this.amb.connect(this.master);
    this.noise = this.makeNoise(false); this.brown = this.makeNoise(true);
    // A silent blip satisfies iOS's "sound must start inside the gesture" rule.
    const b = ctx.createBufferSource(); b.buffer = ctx.createBuffer(1, 1, 22050); b.connect(ctx.destination); b.start();
    if (this.loading) await this.loading;
    await this.decodeAll(ctx); // anything the preload missed
    this.ready = true;
  }

  setVolume(v: number) { this.volume = v; if (this.master) this.master.gain.value = v; }

  private makeNoise(brown: boolean) {
    const ctx = this.ctx!;
    const len = ctx.sampleRate * 3;
    const b = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = b.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) {
      const w = Math.random() * 2 - 1;
      if (brown) { last = (last + 0.02 * w) / 1.02; d[i] = last * 3.5; } else d[i] = w;
    }
    return b;
  }

  listener(cam: THREE.Camera) {
    const ctx = this.ctx; if (!ctx) return;
    const l = ctx.listener as any;
    const p = cam.position;
    const f = new THREE.Vector3(0, 0, -1).applyQuaternion(cam.quaternion);
    const u = new THREE.Vector3(0, 1, 0).applyQuaternion(cam.quaternion);
    if (l.positionX) {
      const t = ctx.currentTime;
      l.positionX.setTargetAtTime(p.x, t, 0.02); l.positionY.setTargetAtTime(p.y, t, 0.02); l.positionZ.setTargetAtTime(p.z, t, 0.02);
      l.forwardX.setTargetAtTime(f.x, t, 0.02); l.forwardY.setTargetAtTime(f.y, t, 0.02); l.forwardZ.setTargetAtTime(f.z, t, 0.02);
      l.upX.setTargetAtTime(u.x, t, 0.02); l.upY.setTargetAtTime(u.y, t, 0.02); l.upZ.setTargetAtTime(u.z, t, 0.02);
    } else { l.setPosition(p.x, p.y, p.z); l.setOrientation(f.x, f.y, f.z, u.x, u.y, u.z); }
  }

  private panner(pos: THREE.Vector3, ref = 1.2) {
    const p = this.ctx!.createPanner();
    p.panningModel = 'equalpower'; p.distanceModel = 'inverse'; p.refDistance = ref; p.rolloffFactor = 1.2; p.maxDistance = 60;
    if ((p as any).positionX) { p.positionX.value = pos.x; p.positionY.value = pos.y; p.positionZ.value = pos.z; } else (p as any).setPosition(pos.x, pos.y, pos.z);
    p.connect(this.sfx);
    return p;
  }

  play(name: string, o: { gain?: number; rate?: number; at?: THREE.Vector3; when?: number; dest?: AudioNode; loop?: boolean; lp?: number } = {}) {
    const ctx = this.ctx; const b = this.buf[name];
    if (!ctx || !b) return null;
    const src = ctx.createBufferSource(); src.buffer = b; src.playbackRate.value = o.rate ?? 1; src.loop = !!o.loop;
    const g = ctx.createGain(); g.gain.value = o.gain ?? 1;
    if (o.lp) { const f = ctx.createBiquadFilter(); f.type = 'lowpass'; f.frequency.value = o.lp; src.connect(f); f.connect(g); } else src.connect(g);
    g.connect(o.dest ?? (o.at ? this.panner(o.at) : this.sfx));
    src.start(ctx.currentTime + (o.when ?? 0));
    return { src, gain: g };
  }

  loop(key: string, name: string, o: { gain?: number; at?: THREE.Vector3; dest?: AudioNode; rate?: number } = {}) {
    this.stop(key);
    const h = this.play(name, { ...o, loop: true });
    if (h) this.loops.set(key, h);
    return h;
  }
  stop(key: string, fade = 0.05) {
    const h = this.loops.get(key); if (!h || !this.ctx) return;
    const t = this.ctx.currentTime;
    h.gain.gain.setTargetAtTime(0, t, fade);
    h.src.stop(t + fade * 6);
    this.loops.delete(key);
  }

  // ---------- Synth ----------
  private noiseSrc(brown = false, loop = true) {
    const s = this.ctx!.createBufferSource(); s.buffer = brown ? this.brown : this.noise; s.loop = loop; return s;
  }

  startRoomTone() {
    const ctx = this.ctx; if (!ctx || this.roomTone) return;
    const g = ctx.createGain(); g.gain.value = 0.0; g.connect(this.amb);
    const n = this.noiseSrc(true); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 420;
    const ng = ctx.createGain(); ng.gain.value = 0.22; n.connect(lp); lp.connect(ng); ng.connect(g); n.start();
    const hum = ctx.createOscillator(); hum.frequency.value = 60; const hg = ctx.createGain(); hg.gain.value = 0.035; hum.connect(hg); hg.connect(g); hum.start();
    const hum2 = ctx.createOscillator(); hum2.frequency.value = 120; const hg2 = ctx.createGain(); hg2.gain.value = 0.02; hum2.connect(hg2); hg2.connect(g); hum2.start();
    g.gain.setTargetAtTime(0.5, ctx.currentTime, 1.5);
    this.roomTone = g;
  }

  // Receiver tone: a sine whose loudness follows lock quality, plus band noise.
  startCarrier() {
    const ctx = this.ctx; if (!ctx || this.carrier) return;
    const osc = ctx.createOscillator(); osc.type = 'sine'; osc.frequency.value = 760;
    const gain = ctx.createGain(); gain.gain.value = 0;
    osc.connect(gain); gain.connect(this.sfx); osc.start();
    const n = this.noiseSrc(false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1800; bp.Q.value = 0.6;
    const stat = ctx.createGain(); stat.gain.value = 0; n.connect(bp); bp.connect(stat); stat.connect(this.sfx); n.start();
    this.carrier = { osc, gain, stat };
  }
  /** The anomaly near 1420.405 as the receiver hears it: clarity 0 (static) to 1 (locked). */
  signalFrame(clarity: number, level: number, t: number) {
    if (!this.ctx || !this.noise) return;
    if (!this.signal) {
      if (clarity <= 0 || level <= 0) return;
      this.signal = new SignalVoice(this.ctx, this.sfx, this.noise);
    }
    this.signal.frame(clarity, level, t);
  }
  signalOff() { this.signal?.stop(); this.signal = null; }
  /** White noise, for synths elsewhere (core/decoder.ts). Null until unlock(). */
  noiseBuffer(): AudioBuffer | null { return this.ctx ? this.noise : null; }
  /** A point in the world that sounds can be sent to (core/walkie.ts). */
  spatial(at: THREE.Vector3, ref = 1.0): AudioNode | null { return this.ctx ? this.panner(at, ref) : null; }

  // ---------- the radio on the filing cabinets ----------
  // AM at night: a station a long way off that fades in and out of the static (skywave).
  // The tune is made here, a slow three-chord waltz on a tinny little speaker; no
  // recording, so no licence. amRadioJam() drops it into the hiss while the line is dead.
  private am: { out: GainNode; music: GainNode; hiss: GainNode; stop: () => void } | null = null;
  amRadio(at: THREE.Vector3 | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (!at) {
      const r = this.am; if (!r) return;
      this.am = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.08);
      setTimeout(() => r.stop(), 900);
      return;
    }
    if (this.am) return;
    const sr = ctx.sampleRate, secs = 19.2, buf = ctx.createBuffer(1, Math.floor(sr * secs), sr), d = buf.getChannelData(0);
    // G, C, D, G in 3/4 at 100 bpm: bass on one, chord on two and three, a little melody over it
    const hz = (n: number) => 440 * Math.pow(2, (n - 69) / 12);
    const bars: [number, number[], number[]][] = [[43, [59, 62, 67], [71, 74, 71]], [48, [60, 64, 67], [72, 71, 69]], [50, [62, 66, 69], [69, 66, 62]], [43, [59, 62, 67], [67, 0, 0]]];
    const beat = 0.6;
    const tone = (f: number, t0: number, dur: number, amp: number, harm: number) => {
      const i0 = Math.floor(t0 * sr), n = Math.floor(dur * sr);
      for (let i = 0; i < n && i0 + i < d.length; i++) {
        const t = i / sr, env = Math.min(1, t / 0.01) * Math.exp(-t * 3.2);
        const ph = 2 * Math.PI * f * t;
        d[i0 + i] += amp * env * (Math.sin(ph) + harm * Math.sin(2 * ph) + harm * 0.5 * Math.sin(3 * ph));
      }
    };
    for (let rep = 0; rep < 2; rep++) bars.forEach(([bass, chord, mel], b) => {
      const t0 = (rep * 4 + b) * 3 * beat;
      tone(hz(bass), t0, beat * 1.2, 0.22, 0.4);
      for (const k of [1, 2]) for (const n of chord) tone(hz(n), t0 + k * beat, beat * 0.8, 0.06, 0.6);
      mel.forEach((n, k) => { if (n) tone(hz(n + (rep ? 0 : 0)), t0 + k * beat, beat * 1.4, 0.1, 0.3); });
    });
    const out = ctx.createGain(); out.gain.value = 0;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1300; bp.Q.value = 0.7;
    out.connect(bp); bp.connect(this.panner(at, 0.8));
    const src = ctx.createBufferSource(); src.buffer = buf; src.loop = true;
    const music = ctx.createGain(); music.gain.value = 0.8;
    const fade = ctx.createGain(); fade.gain.value = 0.6;
    const lfo = ctx.createOscillator(); lfo.frequency.value = 0.07;
    const lg = ctx.createGain(); lg.gain.value = 0.4; lfo.connect(lg); lg.connect(fade.gain);
    src.connect(music); music.connect(fade); fade.connect(out);
    const n = this.noiseSrc(false); const hiss = ctx.createGain(); hiss.gain.value = 0.18; n.connect(hiss); hiss.connect(out);
    for (const x of [src, lfo, n]) x.start();
    out.gain.setTargetAtTime(0.11, ctx.currentTime, 0.3);
    this.am = { out, music, hiss, stop: () => { for (const x of [src, lfo, n]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }
  amRadioJam(on: boolean) {
    const ctx = this.ctx, r = this.am; if (!ctx || !r) return;
    const t = ctx.currentTime;
    r.music.gain.setTargetAtTime(on ? 0 : 0.8, t, on ? 0.4 : 1.2);
    r.hiss.gain.setTargetAtTime(on ? 0.5 : 0.18, t, 0.4);
  }

  setCarrier(level: number, pitchOffset: number, staticLevel: number) {
    if (!this.carrier || !this.ctx) return;
    const t = this.ctx.currentTime;
    this.carrier.gain.gain.setTargetAtTime(level, t, 0.015);
    this.carrier.osc.frequency.setTargetAtTime(760 * (1 + pitchOffset), t, 0.05);
    this.carrier.stat.gain.setTargetAtTime(staticLevel, t, 0.08);
  }

  beep(freq = 1200, dur = 0.08, gain = 0.12, when = 0, type: OscillatorType = 'square') {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const o = ctx.createOscillator(); o.type = type; o.frequency.value = freq;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(gain, t + 0.005); g.gain.setValueAtTime(gain, t + dur); g.gain.linearRampToValueAtTime(0, t + dur + 0.02);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3000;
    o.connect(lp); lp.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + dur + 0.05);
  }

  boom(strength = 1, dest?: AudioNode, when = 0) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const out = dest ?? this.sfx;
    const o = ctx.createOscillator(); o.type = 'sine';
    o.frequency.setValueAtTime(70, t); o.frequency.exponentialRampToValueAtTime(24, t + 1.6);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.9 * strength, t + 0.02); g.gain.exponentialRampToValueAtTime(0.001, t + 2.6);
    o.connect(g); g.connect(out); o.start(t); o.stop(t + 2.8);
    const n = this.noiseSrc(true, false); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.setValueAtTime(900, t); lp.frequency.exponentialRampToValueAtTime(80, t + 2.2);
    const ng = ctx.createGain(); ng.gain.setValueAtTime(0, t); ng.gain.linearRampToValueAtTime(1.4 * strength, t + 0.015); ng.gain.exponentialRampToValueAtTime(0.001, t + 2.8);
    n.connect(lp); lp.connect(ng); ng.connect(out); n.start(t); n.stop(t + 3);
  }

  // Thunder far away over the mesa: no crack, only rolls that come in slowly, darker and
  // later the further off it is, after the far-thunder layers in Tom's Loincloth-Legends
  // (thunderSyn with `far`), with a low sine swell under them.
  thunder(strength = 0.6) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const roll = (at: number, dur: number, f0: number, f1: number, v: number) => {
      const n = this.noiseSrc(true, false); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass';
      lp.frequency.setValueAtTime(f0, t + at); lp.frequency.exponentialRampToValueAtTime(f1, t + at + dur);
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t + at); g.gain.linearRampToValueAtTime(v, t + at + dur * 0.28); g.gain.exponentialRampToValueAtTime(0.001, t + at + dur);
      n.connect(lp); lp.connect(g); g.connect(this.amb); n.start(t + at); n.stop(t + at + dur + 0.1);
    };
    const s = strength;
    roll(0, 3.4, 360, 55, 0.5 * s);
    roll(0.8 + Math.random() * 0.7, 2.6, 280, 45, 0.38 * s);
    if (Math.random() < 0.7) roll(2.1 + Math.random() * 1.2, 3.2, 220, 38, 0.3 * s);
    const o = ctx.createOscillator(); o.frequency.setValueAtTime(44, t + 0.3); o.frequency.exponentialRampToValueAtTime(30, t + 2.8);
    const og = ctx.createGain(); og.gain.setValueAtTime(0, t + 0.3); og.gain.linearRampToValueAtTime(0.28 * s, t + 1.0); og.gain.exponentialRampToValueAtTime(0.001, t + 3.0);
    o.connect(og); og.connect(this.amb); o.start(t + 0.3); o.stop(t + 3.1);
  }

  gasp(dest?: AudioNode, when = 0) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 1.8;
    bp.frequency.setValueAtTime(900, t); bp.frequency.linearRampToValueAtTime(1600, t + 0.35);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.5, t + 0.06); g.gain.exponentialRampToValueAtTime(0.001, t + 0.45);
    n.connect(bp); bp.connect(g); g.connect(dest ?? this.sfx); n.start(t); n.stop(t + 0.5);
  }

  pour(at?: THREE.Vector3) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 3;
    bp.frequency.setValueAtTime(500, t); bp.frequency.linearRampToValueAtTime(1300, t + 1.6);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.25, t + 0.1); g.gain.setValueAtTime(0.25, t + 1.4); g.gain.linearRampToValueAtTime(0, t + 1.7);
    n.connect(bp); bp.connect(g); g.connect(at ? this.panner(at, 1) : this.sfx); n.start(t); n.stop(t + 1.8);
  }

  // A television with no station: hiss from the set's little speaker, where it stands.
  private tv: { src: AudioBufferSourceNode; gain: GainNode } | null = null;
  tvHiss(at: THREE.Vector3 | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (this.tv) { const old = this.tv; this.tv = null; old.gain.gain.setTargetAtTime(0, ctx.currentTime, 0.05); old.src.stop(ctx.currentTime + 0.3); }
    if (!at) return;
    const n = this.noiseSrc(false, true);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 2600; bp.Q.value = 0.5;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 450;
    const g = ctx.createGain(); g.gain.value = 0;
    g.gain.setTargetAtTime(0.09, ctx.currentTime, 0.15);
    n.connect(hp); hp.connect(bp); bp.connect(g); g.connect(this.panner(at, 0.9));
    n.start();
    this.tv = { src: n, gain: g };
  }

  // The handset sound: band-limited like a 1980s phone line.
  phoneLine() {
    const ctx = this.ctx!;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 320;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3200;
    const g = ctx.createGain(); g.gain.value = 1.0;
    hp.connect(lp); lp.connect(g); g.connect(this.sfx);
    return { input: hp, gain: g };
  }

  futureCall(): number {
    const ctx = this.ctx; if (!ctx) return 4.3;
    const line = this.phoneLine();
    const t = ctx.currentTime;
    const n = this.noiseSrc(false); const sg = ctx.createGain(); sg.gain.value = 0.05; n.connect(sg); sg.connect(line.input); n.start(t); n.stop(t + 4.3);
    const h = this.noiseSrc(true); const hg = ctx.createGain(); hg.gain.value = 0.5; h.connect(hg); hg.connect(line.input); h.start(t); h.stop(t + 4.3);
    this.play('printer', { gain: 0.15, when: 0.4, dest: line.input });
    this.boom(0.8, line.input, 2.0);
    this.gasp(line.input, 2.25);
    this.play('ceramic', { gain: 0.9, when: 2.55, dest: line.input });
    this.play('click', { gain: 0.8, when: 4.3 });
    return 4.3;
  }

  startMotors() {
    const ctx = this.ctx; if (!ctx || this.motor) return;
    const osc = ctx.createOscillator(); osc.type = 'sawtooth'; osc.frequency.value = 46;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 240;
    const gain = ctx.createGain(); gain.gain.value = 0;
    osc.connect(lp); lp.connect(gain); gain.connect(this.amb); osc.start();
    const t = ctx.currentTime;
    gain.gain.setTargetAtTime(0.16, t, 0.8);
    osc.frequency.setTargetAtTime(58, t, 2);
    this.motor = { gain, osc };
  }
  stopMotors() {
    if (!this.motor || !this.ctx) return;
    const t = this.ctx.currentTime;
    this.motor.gain.gain.setTargetAtTime(0, t, 0.6);
    this.motor.osc.frequency.setTargetAtTime(30, t, 0.8);
    this.motor.osc.stop(t + 4);
    this.motor = null;
  }

  // Grit under a boot on the outside concrete: a short burst of filtered noise.
  grit(gain = 0.05) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const n = this.noiseSrc(false, false);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 2600 + Math.random() * 1400; bp.Q.value = 0.9;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(gain, t + 0.01); g.gain.exponentialRampToValueAtTime(0.001, t + 0.12);
    n.connect(bp); bp.connect(g); g.connect(this.sfx); n.start(t); n.stop(t + 0.14);
  }

  // An ordinary phone call on the handset: line hiss for `secs`, a click at each end.
  callLine(secs: number) {
    const ctx = this.ctx; if (!ctx) return;
    const line = this.phoneLine();
    const t = ctx.currentTime;
    const n = this.noiseSrc(false); const g = ctx.createGain(); g.gain.value = 0.035;
    n.connect(g); g.connect(line.input); n.start(t); n.stop(t + secs);
    this.play('click', { gain: 0.6 }); this.play('click', { gain: 0.5, when: secs });
  }

  // Touch-tone dialling. Returns how long it takes.
  dial(number: string) {
    const ctx = this.ctx; if (!ctx) return 1.5;
    const rows = [697, 770, 852, 941], cols = [1209, 1336, 1477], keys = '123456789*0#';
    let when = 0.25;
    for (const ch of number) {
      const k = keys.indexOf(ch);
      if (k < 0) { when += 0.2; continue; }
      const t = ctx.currentTime + when;
      for (const f of [rows[Math.floor(k / 3)], cols[k % 3]]) {
        const o = ctx.createOscillator(); o.frequency.value = f;
        const g = ctx.createGain();
        g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.05, t + 0.01); g.gain.setValueAtTime(0.05, t + 0.09); g.gain.linearRampToValueAtTime(0, t + 0.1);
        o.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + 0.12);
      }
      when += 0.16;
    }
    return when;
  }

  // North American ringback in the earpiece: 440 and 480 Hz together, on for 1.6 s.
  ringback(times: number, start = 0) {
    const ctx = this.ctx; if (!ctx) return start + times * 3.2;
    for (let i = 0; i < times; i++) {
      const t = ctx.currentTime + start + i * 3.2;
      for (const f of [440, 480]) {
        const o = ctx.createOscillator(); o.frequency.value = f;
        const g = ctx.createGain();
        g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.04, t + 0.03); g.gain.setValueAtTime(0.04, t + 1.6); g.gain.linearRampToValueAtTime(0, t + 1.65);
        o.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + 1.7);
      }
    }
    return start + times * 3.2;
  }

  // Positional electrical hum (a transformer cabinet, a lamp ballast): mains and two harmonics.
  private hums = new Map<string, GainNode>();
  hum(key: string, at: THREE.Vector3, gain = 0.05, base = 60) {
    const ctx = this.ctx; if (!ctx || this.hums.has(key)) return;
    const out = ctx.createGain(); out.gain.value = 0;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 900;
    out.connect(lp); lp.connect(this.panner(at, 1.0));
    for (const [mul, g, type] of [[1, 1, 'sine'], [2, 0.5, 'sine'], [3, 0.12, 'square']] as const) {
      const o = ctx.createOscillator(); o.type = type; o.frequency.value = base * mul;
      const og = ctx.createGain(); og.gain.value = g; o.connect(og); og.connect(out); o.start();
    }
    out.gain.setTargetAtTime(gain, ctx.currentTime, 1.0);
    this.hums.set(key, out);
  }
  stopHums() {
    if (!this.ctx) return;
    for (const g of this.hums.values()) g.gain.setTargetAtTime(0, this.ctx.currentTime, 0.3);
    this.hums.clear();
  }

  // The valve receiver in the STATION 01 hut: a weak carrier that fades in and out, with
  // static, from a small speaker. `voice` lays a voice-shaped band of noise under it for
  // a few seconds (the fragment is captioned; there is no recorded actor).
  private radioNodes: { out: GainNode; voice: GainNode; stop: () => void } | null = null;
  radio(at: THREE.Vector3 | null, level = 0.06) {
    const ctx = this.ctx; if (!ctx) return;
    if (!at) {
      const r = this.radioNodes; if (!r) return;
      this.radioNodes = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.15);
      setTimeout(() => r.stop(), 1200);
      return;
    }
    if (this.radioNodes) return;
    const out = ctx.createGain(); out.gain.value = 0;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1100; bp.Q.value = 0.6;
    out.connect(bp); bp.connect(this.panner(at, 1.0));
    const osc = ctx.createOscillator(); osc.frequency.value = 760;
    const og = ctx.createGain(); og.gain.value = 0.45; osc.connect(og); og.connect(out);
    const lfo = ctx.createOscillator(); lfo.frequency.value = 0.21;
    const lg = ctx.createGain(); lg.gain.value = 0.3; lfo.connect(lg); lg.connect(og.gain);
    const n = this.noiseSrc(false); const ng = ctx.createGain(); ng.gain.value = 0.3; n.connect(ng); ng.connect(out);
    // the voice: noise through a speech band, chopped at a syllable rate
    const vn = this.noiseSrc(true); const vb = ctx.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = 650; vb.Q.value = 1.8;
    const voice = ctx.createGain(); voice.gain.value = 0;
    const syl = ctx.createOscillator(); syl.type = 'square'; syl.frequency.value = 4.3;
    const sg = ctx.createGain(); sg.gain.value = 0.5; syl.connect(sg);
    const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
    vn.connect(vb); vb.connect(chop); chop.connect(voice); voice.connect(out);
    for (const x of [osc, lfo, n, vn, syl]) x.start();
    out.gain.setTargetAtTime(level, ctx.currentTime, 0.5);
    this.radioNodes = { out, voice, stop: () => { for (const x of [osc, lfo, n, vn, syl]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }
  radioVoice(secs: number, when = 0) {
    const ctx = this.ctx, r = this.radioNodes; if (!ctx || !r) return;
    const t = ctx.currentTime + when;
    r.voice.gain.setTargetAtTime(2.2, t, 0.08);
    r.voice.gain.setTargetAtTime(0, t + secs, 0.12);
  }

  // The radio in the truck's dash on the old road (chapter six): not placed in the world,
  // a small speaker in front of the driver. 'talk' is the morning programme from Roswell,
  // too low to follow (a speech band chopped at a talking rate, a little music under it);
  // 'carrier' is the bare carrier when the night takes the station; null fades it out.
  // radioIn() is where the fragments of the night are played through the same speaker.
  private cab: { out: GainNode; spk: AudioNode; talk: GainNode; car: GainNode; hiss: GainNode; stop: () => void } | null = null;
  cabRadio(mode: 'talk' | 'carrier' | 'hiss' | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (!mode) {
      const r = this.cab; if (!r) return;
      this.cab = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.05);
      setTimeout(() => r.stop(), 700);
      return;
    }
    if (!this.cab) {
      const out = ctx.createGain(); out.gain.value = 0;
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1250; bp.Q.value = 0.8;
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3400;
      out.connect(bp); bp.connect(lp); lp.connect(this.sfx);
      // talk: noise in a voice band, chopped at a syllable rate, with pauses between phrases
      const vn = this.noiseSrc(true), vb = ctx.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = 700; vb.Q.value = 1.6;
      const syl = ctx.createOscillator(); syl.type = 'square'; syl.frequency.value = 4.6;
      const sg = ctx.createGain(); sg.gain.value = 0.5; syl.connect(sg);
      const phr = ctx.createOscillator(); phr.type = 'square'; phr.frequency.value = 0.31;
      const pg = ctx.createGain(); pg.gain.value = 0.5; phr.connect(pg);
      const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
      const pause = ctx.createGain(); pause.gain.value = 0.5; pg.connect(pause.gain);
      const talk = ctx.createGain(); talk.gain.value = 0;
      vn.connect(vb); vb.connect(chop); chop.connect(pause); pause.connect(talk); talk.connect(out);
      const car = ctx.createGain(); car.gain.value = 0;
      const osc = ctx.createOscillator(); osc.frequency.value = 760; osc.connect(car); car.connect(out);
      const n = this.noiseSrc(false), hiss = ctx.createGain(); hiss.gain.value = 0.25; n.connect(hiss); hiss.connect(out);
      for (const x of [vn, syl, phr, osc, n]) x.start();
      this.cab = { out, spk: bp, talk, car, hiss, stop: () => { for (const x of [vn, syl, phr, osc, n]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
    }
    const r = this.cab, t = ctx.currentTime;
    r.out.gain.setTargetAtTime(0.09, t, 0.2);
    r.talk.gain.setTargetAtTime(mode === 'talk' ? 1.6 : 0, t, mode === 'talk' ? 0.4 : 0.01);
    r.car.gain.setTargetAtTime(mode === 'carrier' ? 0.22 : 0, t, 0.01);
    r.hiss.gain.setTargetAtTime(mode === 'talk' ? 0.22 : mode === 'carrier' ? 0.08 : 0.35, t, 0.02);
  }
  /** The cab speaker's input (null when the radio is off): sounds played here come out of it. */
  radioIn(): AudioNode | null { return this.cab?.spk ?? null; }
  /** A voice on the cab radio, as noise in a speech band (no recorded actor; it is captioned).
   *  formant: lower for a man (about 520 Hz), higher for a woman (about 820 Hz). */
  cabVoice(secs: number, formant: number) {
    const ctx = this.ctx, r = this.cab; if (!ctx || !r) return;
    const t = ctx.currentTime;
    const vn = this.noiseSrc(true, true), vb = ctx.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = formant; vb.Q.value = 2.2;
    const vb2 = ctx.createBiquadFilter(); vb2.type = 'peaking'; vb2.frequency.value = formant * 2.6; vb2.gain.value = 8;
    const syl = ctx.createOscillator(); syl.type = 'triangle'; syl.frequency.value = 3.9 + (formant > 700 ? 0.8 : 0);
    const sg = ctx.createGain(); sg.gain.value = 0.55; syl.connect(sg);
    const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
    const g = ctx.createGain(); g.gain.value = 0;
    vn.connect(vb); vb.connect(vb2); vb2.connect(chop); chop.connect(g); g.connect(r.spk);
    g.gain.setTargetAtTime(3.2, t, 0.05); g.gain.setTargetAtTime(0, t + secs, 0.08);
    vn.start(t); syl.start(t); vn.stop(t + secs + 0.6); syl.stop(t + secs + 0.6);
  }
  /** A cup set down on a saucer: two short bright rings. dest: where it plays (the cab radio). */
  cupDown(dest?: AudioNode | null) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime, out = dest ?? this.sfx;
    for (const [dt, f, a] of [[0, 2380, 0.22], [0.012, 3710, 0.12], [0.09, 2390, 0.07], [0.1, 5120, 0.05]] as const) {
      const o = ctx.createOscillator(); o.frequency.value = f;
      const g = ctx.createGain(); g.gain.value = 0;
      g.gain.setValueAtTime(0, t + dt); g.gain.linearRampToValueAtTime(a, t + dt + 0.002); g.gain.exponentialRampToValueAtTime(0.0005, t + dt + 0.35);
      o.connect(g); g.connect(out); o.start(t + dt); o.stop(t + dt + 0.4);
    }
    const n = this.noiseSrc(false, false), ng = ctx.createGain(), hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 2500;
    ng.gain.setValueAtTime(0.18, t); ng.gain.exponentialRampToValueAtTime(0.0005, t + 0.04);
    n.connect(hp); hp.connect(ng); ng.connect(out); n.start(t); n.stop(t + 0.06);
  }
  /** Total silence (THE EVENT): every sound and the ambience off; music is left alone. */
  silence(on: boolean) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    this.sfx.gain.setTargetAtTime(on ? 0 : 1, t, on ? 0.04 : 0.3);
    this.amb.gain.setTargetAtTime(on ? 0 : 1, t, on ? 0.08 : 0.5);
  }

  // Where the listener is. Out in the yard the wind and the crickets take over from the
  // room tone; the photo lab is small and closed, with its own ventilation hum.
  private space: 'room' | 'yard' | 'lab' = 'room';
  setSpace(space: 'room' | 'yard' | 'lab') {
    this.space = space;
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const [tone, wind, bugs] = space === 'room' ? [0.5, 0.14, 0.02] : space === 'lab' ? [0.3, 0.05, 0] : [0.1, 0.34, 0.16];
    this.roomTone?.gain.setTargetAtTime(tone, t, 0.7);
    this.loops.get('wind')?.gain.gain.setTargetAtTime(wind, t, 0.7);
    this.loops.get('crickets')?.gain.gain.setTargetAtTime(bugs, t, 1.2);
  }

  /** One step on a surface (Player.onStep). */
  step(surface: Surface) {
    const s = STEPS[surface];
    const name = s.names[Math.floor(Math.random() * s.names.length)];
    this.play(name, { gain: s.gain * (0.85 + Math.random() * 0.3), rate: s.rate * (0.94 + Math.random() * 0.12), lp: s.lp });
    if (s.grit) this.grit(s.grit);
  }

  // The desert at night, far from everything: an owl, a dog at a ranch, a gust in the
  // scrub, now and then, from a random direction. Only outside; nothing answers the player.
  private nextLife = 20;
  nightLife(dt: number, at: THREE.Vector3) {
    if (!this.ctx || this.space !== 'yard') return;
    this.nextLife -= dt;
    if (this.nextLife > 0) return;
    this.nextLife = 25 + Math.random() * 45;
    const r = Math.random();
    const name = r < 0.35 ? 'owl' : r < 0.6 ? (Math.random() < 0.5 ? 'dog0' : 'dog1') : 'gust';
    this.far(name, at, name === 'gust' ? 0.35 : name === 'owl' ? 0.3 : 0.22, name === 'owl' ? 0.9 + Math.random() * 0.15 : 1);
  }
  /** A sound far off in a random direction: panned, quiet and dull with distance. */
  far(name: string, at: THREE.Vector3, gain: number, rate = 1) {
    const ctx = this.ctx; const b = this.buf[name];
    if (!ctx || !b) return;
    const a = Math.random() * Math.PI * 2, d = 30 + Math.random() * 30;
    const pos = new THREE.Vector3(at.x + Math.cos(a) * d, at.y + 2, at.z + Math.sin(a) * d);
    const p = ctx.createPanner();
    p.panningModel = 'equalpower'; p.distanceModel = 'inverse'; p.refDistance = 30; p.rolloffFactor = 1; p.maxDistance = 200;
    if ((p as any).positionX) { p.positionX.value = pos.x; p.positionY.value = pos.y; p.positionZ.value = pos.z; } else (p as any).setPosition(pos.x, pos.y, pos.z);
    const src = ctx.createBufferSource(); src.buffer = b; src.playbackRate.value = rate;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 2600 - d * 20;
    const g = ctx.createGain(); g.gain.value = gain;
    src.connect(lp); lp.connect(g); g.connect(p); p.connect(this.amb);
    src.start();
  }

  duckAll(seconds: number) {
    if (!this.ctx) return;
    const t = this.ctx.currentTime;
    this.sfx.gain.setTargetAtTime(0, t, 0.3); this.amb.gain.setTargetAtTime(0, t, 0.6);
    setTimeout(() => { if (!this.ctx) return; const t2 = this.ctx.currentTime; this.sfx.gain.setTargetAtTime(1, t2, 0.2); this.amb.gain.setTargetAtTime(1, t2, 0.5); }, seconds * 1000);
  }

  // Rangefinder shutter: two dry clicks, then the film advance lever.
  shutter() {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const click = (when: number, f: number, g: number) => {
      const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = f; bp.Q.value = 2.5;
      const gn = ctx.createGain(); gn.gain.setValueAtTime(0, t + when); gn.gain.linearRampToValueAtTime(g, t + when + 0.002); gn.gain.exponentialRampToValueAtTime(0.001, t + when + 0.04);
      n.connect(bp); bp.connect(gn); gn.connect(this.sfx); n.start(t + when); n.stop(t + when + 0.06);
    };
    click(0, 3400, 0.8); click(0.05, 2200, 0.55);
    const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.setValueAtTime(150, t + 0.16); o.frequency.linearRampToValueAtTime(95, t + 0.42);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 1100;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t + 0.16); g.gain.linearRampToValueAtTime(0.07, t + 0.19); g.gain.linearRampToValueAtTime(0, t + 0.44);
    o.connect(lp); lp.connect(g); g.connect(this.sfx); o.start(t + 0.16); o.stop(t + 0.46);
  }

  // A small geared motor running for `secs` (the B-12 reference drive).
  servo(secs: number, at?: THREE.Vector3) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.setValueAtTime(210, t); o.frequency.linearRampToValueAtTime(260, t + secs * 0.5); o.frequency.linearRampToValueAtTime(190, t + secs);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 700;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.18, t + 0.08); g.gain.setValueAtTime(0.18, t + secs - 0.1); g.gain.linearRampToValueAtTime(0, t + secs);
    o.connect(lp); lp.connect(g); g.connect(at ? this.panner(at, 1.5) : this.sfx); o.start(t); o.stop(t + secs + 0.05);
    this.play('switch', { gain: 0.5, when: secs, at });
  }

  // Clicks in the 4 / 7 grouping of the signal.
  signature(gain = 0.5) {
    if (!this.ctx) return;
    const times: number[] = [];
    for (let i = 0; i < 4; i++) times.push(i * 0.22);
    for (let i = 0; i < 7; i++) times.push(1.4 + i * 0.22);
    times.forEach((w) => this.play('click', { gain, when: w, rate: 0.9 }));
  }

  suspend(on: boolean) { if (!this.ctx) return; if (on) this.ctx.suspend(); else this.ctx.resume(); }
}
