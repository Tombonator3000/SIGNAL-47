import * as THREE from 'three';
import phoneRing from '../assets/audio/phone_ring.mp3';
import printer from '../assets/audio/printer.mp3';
import ceramic from '../assets/audio/ceramic.mp3';
import wind from '../assets/audio/wind.mp3';
import titleMusic from '../assets/audio/title_music.mp3';
import step0 from '../assets/audio/step0.mp3';
import step1 from '../assets/audio/step1.mp3';
import step2 from '../assets/audio/step2.mp3';
import click from '../assets/audio/click.mp3';
import sw from '../assets/audio/switch.mp3';
import thudSoft from '../assets/audio/thud_soft.mp3';

const SOURCES: Record<string, string> = { phoneRing, printer, ceramic, wind, titleMusic, step0, step1, step2, click, switch: sw, thudSoft };

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

  play(name: string, o: { gain?: number; rate?: number; at?: THREE.Vector3; when?: number; dest?: AudioNode; loop?: boolean } = {}) {
    const ctx = this.ctx; const b = this.buf[name];
    if (!ctx || !b) return null;
    const src = ctx.createBufferSource(); src.buffer = b; src.playbackRate.value = o.rate ?? 1; src.loop = !!o.loop;
    const g = ctx.createGain(); g.gain.value = o.gain ?? 1;
    src.connect(g);
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

  thunder(strength = 0.6) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const n = this.noiseSrc(true, false); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 160;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.5 * strength, t + 0.6); g.gain.exponentialRampToValueAtTime(0.001, t + 5.5);
    n.connect(lp); lp.connect(g); g.connect(this.amb); n.start(t); n.stop(t + 6);
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

  // Where the listener is. Out in the yard the wind takes over from the room tone;
  // the photo lab is small and closed, with its own ventilation hum.
  setSpace(space: 'room' | 'yard' | 'lab') {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const [tone, wind] = space === 'room' ? [0.5, 0.14] : space === 'lab' ? [0.3, 0.05] : [0.1, 0.34];
    this.roomTone?.gain.setTargetAtTime(tone, t, 0.7);
    this.loops.get('wind')?.gain.gain.setTargetAtTime(wind, t, 0.7);
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
