import type { SurfaceKind } from './Drive';
import idleUrl from '../assets/audio/engine_idle.wav';
import driveUrl from '../assets/audio/engine_drive.wav';
import startUrl from '../assets/audio/engine_start.mp3';
import offUrl from '../assets/audio/engine_off.mp3';

// The truck's V8, heard from the cab. Tom, 18 October: a new and better engine sound. It is
// now two recordings (THIRD_PARTY_NOTICES.md): a carburetted V8 at idle, and an old car or
// truck at a steady cruise from inside; each loops, is pitched up and down with the rpm, and
// the two cross over between 900 and 1500 rpm. A little band-passed rumble opens with the
// throttle, and all of it goes through a low-pass that opens with load, then a fixed "cab"
// low-pass. Getting in, the engine starts with its own recording; arriving, it shuts off.
// Until the recordings are decoded (a moment, the first time), the old engine made of Web
// Audio nodes stands in: two detuned sawtooths at the firing rate and half of it, and a little
// square for rasp. Tyre and wind noise have their own small bus.
// Levels sit under the wind ambience (AudioSys: wind 0.34 outside, motors 0.16).
// Route `dest` to the game's sfx bus so the volume setting and pause apply.

const MASTER = 0.33;
/** The rpm each recording was made at (measured from their spectra, then by ear in the game). */
const IDLE_RPM = 600, DRIVE_RPM = 1800;
/** The recordings' level against the old engine's (they are quieter as files). */
const REC = 3.7;

type Clip = 'idle' | 'drive' | 'start' | 'off';
const URLS: Record<Clip, string> = { idle: idleUrl, drive: driveUrl, start: startUrl, off: offUrl };
const clips: Partial<Record<Clip, AudioBuffer>> = {};
let loading: Promise<void> | null = null;
function fromDataUrl(url: string) {
  const bin = atob(url.slice(url.indexOf(',') + 1)), out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out.buffer;
}
function loadClips(ctx: BaseAudioContext) {
  return loading ??= Promise.all((Object.keys(URLS) as Clip[]).map(async (k) => {
    try {
      const u = URLS[k], data = u.startsWith('data:') ? fromDataUrl(u) : await (await fetch(u)).arrayBuffer();
      clips[k] = await ctx.decodeAudioData(data);
    } catch (err) { console.warn('engine sound load failed', k, err); }
  })).then(() => {});
}
const smooth = (x: number, a: number, b: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };

export class EngineSound {
  private out: GainNode;
  private nodes: AudioScheduledSourceNode[] = [];
  private eng: { bus: GainNode; lp: BiquadFilterNode; osc: OscillatorNode[]; lfo: OscillatorNode; noise: GainNode; band: BiquadFilterNode;
    rec: { idle: AudioBufferSourceNode; drive: AudioBufferSourceNode; gi: GainNode; gd: GainNode } | null; own: AudioScheduledSourceNode[] } | null = null;
  /** Whether the recordings are playing (the tests). */
  get recorded() { return !!this.eng?.rec; }
  /** Running or stalled; false when start() would turn the key (the key's own sound). */
  get on() { return !!this.eng; }
  private cab: BiquadFilterNode | null = null;
  private rpm = 650; private load = 0;
  private tyre: { roll: GainNode; crunch: GainNode; shake: GainNode; rumble: GainNode; wind: GainNode } | null = null;
  private white: AudioBuffer; private brown: AudioBuffer;
  private running = false;
  private stalled = false;
  private level = 0.1;

  constructor(private ctx: AudioContext, dest: AudioNode) {
    this.out = ctx.createGain(); this.out.gain.value = 0; this.out.connect(dest);
    this.white = this.noise(false); this.brown = this.noise(true);
    // the recordings: once they are in, a running engine changes over to them
    void loadClips(ctx).then(() => { if (this.eng && !this.eng.rec && clips.idle && clips.drive) { this.engineNodes(); if (this.running && !this.stalled) this.set(this.rpm, this.load); } });
  }

  private noise(brown: boolean) {
    const len = this.ctx.sampleRate * 2, b = this.ctx.createBuffer(1, len, this.ctx.sampleRate), d = b.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) {
      const w = Math.random() * 2 - 1;
      if (brown) { last = (last + 0.02 * w) / 1.02; d[i] = last * 3.5; } else d[i] = w;
    }
    return b;
  }
  private src(buf: AudioBuffer, loop = true) {
    const s = this.ctx.createBufferSource(); s.buffer = buf; s.loop = loop; return s;
  }
  private filter(type: BiquadFilterType, f: number, q = 0.7) {
    const b = this.ctx.createBiquadFilter(); b.type = type; b.frequency.value = f; b.Q.value = q; return b;
  }
  private gain(v: number) { const g = this.ctx.createGain(); g.gain.value = v; return g; }

  /** The engine itself: the recordings if they are in, else the oscillators. Called again
   *  when the recordings arrive while it runs. */
  private engineNodes() {
    const c = this.ctx, old = this.eng, t = c.currentTime;
    const bus = this.gain(old ? old.bus.gain.value : 0), lp = this.filter('lowpass', 300, 3);
    lp.connect(bus); bus.connect(this.cab!);
    const own: AudioScheduledSourceNode[] = [];
    const osc: OscillatorNode[] = [];
    const lfo = c.createOscillator(); lfo.frequency.value = 10;
    let rec: NonNullable<EngineSound['eng']>['rec'] = null;
    if (clips.idle && clips.drive) {
      // each loop starts somewhere in itself, so the two do not beat together the same way each time
      const loop = (b: AudioBuffer, g: GainNode) => { const s = this.src(b); s.connect(g); g.connect(lp); s.start(t, Math.random() * b.duration); own.push(s); return s; };
      const gi = this.gain(REC), gd = this.gain(0);
      rec = { idle: loop(clips.idle, gi), drive: loop(clips.drive, gd), gi, gd };
      lp.Q.value = 0.7;
    } else {
      const gains: GainNode[] = [];
      for (const [type, g] of [['sawtooth', 0.5], ['sawtooth', 0.38], ['square', 0.06]] as const) {
        const o = c.createOscillator(); o.type = type;
        const og = this.gain(g); o.connect(og); og.connect(lp);
        osc.push(o); gains.push(og);
      }
      // the lope: the firing note's loudness wobbles at a quarter of its rate
      const depth = this.gain(0.2); lfo.connect(depth); depth.connect(gains[0].gain);
      own.push(...osc);
    }
    own.push(lfo);
    const n = this.src(this.brown), band = this.filter('bandpass', 120, 0.8), noise = this.gain(rec ? 0.03 : 0.08);
    n.connect(band); band.connect(noise); noise.connect(bus);
    own.push(n);
    for (const s of own) if (!rec || (s !== rec.idle && s !== rec.drive)) s.start(t);
    // the old engine goes as the new one comes
    if (old) { old.bus.gain.setTargetAtTime(0, t, 0.08); for (const s of old.own) s.stop(t + 0.5); this.nodes = this.nodes.filter((x) => !old.own.includes(x)); }
    this.nodes.push(...own);
    this.eng = { bus, lp, osc, lfo, noise, band, rec, own };
    if (old) bus.gain.setTargetAtTime(this.level, t, 0.1);
  }

  private build() {
    const cab = this.filter('lowpass', 1100, 0.5);
    cab.connect(this.out);
    this.cab = cab;
    this.engineNodes();
    // tyres and wind: less muffled than the engine
    const tbus = this.filter('lowpass', 3800, 0.5); tbus.connect(this.out);
    const layer = (buf: AudioBuffer, f: BiquadFilterNode) => { const s = this.src(buf), g = this.gain(0); s.connect(f); f.connect(g); g.connect(tbus); this.nodes.push(s); return g; };
    const roll = layer(this.brown, this.filter('lowpass', 280));
    const crunch = layer(this.white, this.filter('bandpass', 1900, 0.6));
    const rumble = layer(this.brown, this.filter('lowpass', 120));
    const wind = layer(this.white, this.filter('bandpass', 650, 0.4));
    // gravel crackles: slow brown noise shakes the crunch layer's gain, as deep as the layer is loud
    const shakeSrc = this.src(this.brown), shake = this.gain(0), slp = this.filter('lowpass', 35);
    shakeSrc.connect(slp); slp.connect(shake); shake.connect(crunch.gain);
    this.tyre = { roll, crunch, shake, rumble, wind };
    for (const s of [...this.nodes.filter((x) => !this.eng!.own.includes(x)), shakeSrc]) s.start();
    this.nodes.push(shakeSrc);
  }
  /** A recording played once, through the cab. */
  private once(k: Clip, gain: number) {
    const b = clips[k]; if (!b || !this.cab) return 0;
    const s = this.src(b, false), g = this.gain(gain); s.connect(g); g.connect(this.cab); s.start(); return b.duration;
  }

  /** Engine on (idle) and the tyre layers ready. */
  start() {
    const fresh = !this.eng;
    if (fresh) this.build();
    this.running = true; this.stalled = false;
    const t = this.ctx.currentTime;
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(MASTER, t, 0.25);
    this.set(650, 0);
    // the key: it turns over and catches, and the idle comes up under it
    if (fresh && this.once('start', 1.1) && this.eng) { const g = this.eng.bus.gain; g.cancelScheduledValues(t); g.setValueAtTime(0, t); g.setValueAtTime(0, t + 1.3); g.linearRampToValueAtTime(this.level, t + 2.6); }
  }

  /** Fade everything out and free the nodes. */
  stop() {
    if (!this.eng) return;
    const t = this.ctx.currentTime;
    // switched off, not dead already: the shut-off, then quiet
    const off = this.running && !this.stalled ? this.once('off', 1.0) : 0;
    this.running = false;
    this.eng.bus.gain.setTargetAtTime(0, t, 0.06);
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(0, t + off, 0.15);
    for (const s of this.nodes) s.stop(t + off + 1);
    this.nodes = []; this.eng = null; this.tyre = null;
  }

  /** Every frame: engine speed in rpm, load 0..1 (the throttle). */
  set(rpm: number, load: number) {
    const e = this.eng;
    if (!e || !this.running || this.stalled) return;
    const t = this.ctx.currentTime;
    this.rpm = rpm; this.load = load;
    const f = Math.max(rpm, 300) / 60 * 4; // a V8 fires four times a turn
    const l = Math.min(1, Math.max(0, load));
    if (e.rec) {
      const r = Math.max(rpm, 300), x = smooth(r, 900, 1500);
      e.rec.idle.playbackRate.setTargetAtTime(Math.min(1.9, Math.max(0.6, r / IDLE_RPM)), t, 0.05);
      e.rec.drive.playbackRate.setTargetAtTime(Math.min(2.1, Math.max(0.55, r / DRIVE_RPM)), t, 0.05);
      e.rec.gi.gain.setTargetAtTime(REC * Math.cos(x * Math.PI / 2), t, 0.08);
      e.rec.gd.gain.setTargetAtTime(REC * Math.sin(x * Math.PI / 2) * (0.85 + 0.3 * l), t, 0.08);
      e.lp.frequency.setTargetAtTime(700 + Math.min(1, r / 4000) * 900 + l * 1200, t, 0.08);
    } else {
      e.osc[0].frequency.setTargetAtTime(f, t, 0.05);
      e.osc[1].frequency.setTargetAtTime(f * 0.5 * 1.007, t, 0.05);
      e.osc[2].frequency.setTargetAtTime(f * 2 * 0.996, t, 0.05);
      e.lfo.frequency.setTargetAtTime(f * 0.25, t, 0.05);
      e.lp.frequency.setTargetAtTime(150 + f * 3 + l * 650, t, 0.08);
    }
    e.band.frequency.setTargetAtTime(f * 2.5, t, 0.08);
    e.noise.gain.setTargetAtTime(0.05 + 0.16 * l, t, 0.1);
    this.level = 0.1 + 0.07 * l + 0.04 * Math.min(1, rpm / 3500);
    e.bus.gain.setTargetAtTime(this.level, t, 0.1);
  }

  /** The engine dies: two or three chugs and a wind-down, then silence. start() runs it again. */
  stall() {
    const e = this.eng;
    if (!e || this.stalled) return;
    this.stalled = true;
    const t = this.ctx.currentTime, g = e.bus.gain, L = this.level;
    for (const o of e.osc) {
      o.frequency.cancelScheduledValues(t);
      o.frequency.setValueAtTime(o.frequency.value, t);
      o.frequency.exponentialRampToValueAtTime(Math.max(4, o.frequency.value * 0.12), t + 1.2);
    }
    // the recordings wind down like a tape slowing
    if (e.rec) for (const s of [e.rec.idle, e.rec.drive]) { const p = s.playbackRate; p.cancelScheduledValues(t); p.setValueAtTime(p.value, t); p.exponentialRampToValueAtTime(0.25, t + 1.2); }
    g.cancelScheduledValues(t); g.setValueAtTime(L, t);
    [[0.08, 1.25], [0.3, 0.45], [0.42, 0.85], [0.72, 0.25], [0.84, 0.45], [1.3, 0]].forEach(([dt, k]) => g.linearRampToValueAtTime(L * k, t + dt));
    e.noise.gain.setTargetAtTime(0, t + 0.2, 0.2);
  }

  /** A misfire (drive/trouble.ts): the note drops out for a moment, with a dull pop in the
   *  exhaust. k 0..1 is how hard. */
  cough(k = 1) {
    const e = this.eng;
    if (!e || this.stalled) return;
    const t = this.ctx.currentTime, g = e.bus.gain, L = this.level;
    g.cancelScheduledValues(t); g.setValueAtTime(L, t);
    g.linearRampToValueAtTime(L * (1 - 0.75 * k), t + 0.025);
    g.linearRampToValueAtTime(L * 1.15, t + 0.11);
    g.linearRampToValueAtTime(L, t + 0.2);
    const n = this.src(this.brown, false), bp = this.filter('bandpass', 160 + Math.random() * 90, 1.4), ng = this.gain(0);
    ng.gain.setValueAtTime(0, t + 0.02); ng.gain.linearRampToValueAtTime(0.5 * k, t + 0.03); ng.gain.exponentialRampToValueAtTime(0.001, t + 0.16);
    n.connect(bp); bp.connect(ng); ng.connect(this.out); n.start(t + 0.02); n.stop(t + 0.2);
  }

  /** The starter turning the engine over for `secs` (it is dead): a whirring motor that
   *  labours on each compression, about four and a half a second. */
  crank(secs = 1.2) {
    const c = this.ctx;
    if (!this.eng) this.build();
    const t = c.currentTime;
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(MASTER, t, 0.05);
    const o = c.createOscillator(); o.type = 'sawtooth';
    o.frequency.setValueAtTime(115, t); o.frequency.linearRampToValueAtTime(135, t + secs);
    const lp = this.filter('lowpass', 900, 1.2), env = this.gain(0), am = this.gain(0.5);
    // the strokes: the motor's level swings at 4.5 Hz
    const lfo = c.createOscillator(); lfo.frequency.value = 4.5;
    const depth = this.gain(0.35); lfo.connect(depth); depth.connect(am.gain);
    o.connect(lp); lp.connect(am); am.connect(env); env.connect(this.out);
    env.gain.setValueAtTime(0, t); env.gain.linearRampToValueAtTime(0.32, t + 0.05);
    env.gain.setValueAtTime(0.32, t + secs - 0.08); env.gain.linearRampToValueAtTime(0, t + secs);
    // the bendix gear's clatter at the start
    const n = this.src(this.white, false), bp = this.filter('bandpass', 2400, 2), ng = this.gain(0);
    ng.gain.setValueAtTime(0.12, t); ng.gain.exponentialRampToValueAtTime(0.001, t + 0.09);
    n.connect(bp); bp.connect(ng); ng.connect(this.out);
    for (const s of [o, lfo]) { s.start(t); s.stop(t + secs + 0.05); }
    n.start(t); n.stop(t + 0.1);
  }

  /** It catches after a stall: the engine note comes back with a little surge. */
  restart() {
    const e = this.eng;
    if (!e) { this.start(); return; }
    this.stalled = false; this.running = true;
    const t = this.ctx.currentTime;
    for (const o of e.osc) o.frequency.cancelScheduledValues(t);
    if (e.rec) for (const s of [e.rec.idle, e.rec.drive]) s.playbackRate.cancelScheduledValues(t);
    e.bus.gain.cancelScheduledValues(t);
    e.bus.gain.setValueAtTime(0, t);
    e.bus.gain.linearRampToValueAtTime(this.level * 1.6, t + 0.15);
    e.bus.gain.setTargetAtTime(this.level, t + 0.3, 0.3);
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(MASTER, t, 0.05);
    this.set(1400, 0.4);
  }

  /** Every frame: rolling noise for the surface under the truck and the speed in m/s. */
  tyres(surface: SurfaceKind, speed: number) {
    const y = this.tyre;
    if (!y || !this.running) return;
    const t = this.ctx.currentTime, v = Math.abs(speed);
    const k = Math.min(1, v / 30), slow = Math.min(1, v / 9);
    y.roll.gain.setTargetAtTime((surface === 'asphalt' ? 0.1 : 0.05) * k * k, t, 0.15);
    const crunch = surface === 'gravel' ? 0.1 * slow : surface === 'dirt' ? 0.04 * slow : 0;
    y.crunch.gain.setTargetAtTime(crunch, t, 0.15);
    y.shake.gain.setTargetAtTime(crunch * 2.5, t, 0.15);
    y.rumble.gain.setTargetAtTime(surface === 'dirt' ? 0.3 * slow : surface === 'gravel' ? 0.1 * slow : 0, t, 0.15);
    y.wind.gain.setTargetAtTime(0.045 * k * k, t, 0.3);
  }

  /** A cattle guard: both axles over a dozen steel pipes. Speed in m/s. */
  rattle(speed: number) {
    const c = this.ctx, v = Math.max(1.5, Math.abs(speed)), t0 = c.currentTime + 0.02;
    const g = 0.06 + 0.22 * Math.min(1, v / 10);
    for (const axle of [0, 3.3 / v]) {
      for (let i = 0; i < 12; i++) {
        const t = t0 + axle + i * 0.2 / v + Math.random() * 0.006;
        const s = this.src(this.white, false), bp = this.filter('bandpass', 700 + Math.random() * 700, 5), env = this.gain(0);
        env.gain.setValueAtTime(0, t); env.gain.linearRampToValueAtTime(g, t + 0.002); env.gain.exponentialRampToValueAtTime(0.001, t + 0.07);
        s.connect(bp); bp.connect(env); env.connect(this.out);
        s.start(t); s.stop(t + 0.08);
      }
      const o = c.createOscillator(), og = this.gain(0), t = t0 + axle;
      o.frequency.value = 62;
      og.gain.setValueAtTime(0, t); og.gain.linearRampToValueAtTime(g * 1.5, t + 0.01); og.gain.exponentialRampToValueAtTime(0.001, t + 0.25);
      o.connect(og); og.connect(this.out); o.start(t); o.stop(t + 0.3);
    }
  }

  /** A knock against a post or a fence, strength from the impact speed in m/s. */
  thump(speed: number) {
    const c = this.ctx, t = c.currentTime, g = Math.min(1, 0.2 + speed / 8);
    const o = c.createOscillator(), og = this.gain(0);
    o.frequency.setValueAtTime(90, t); o.frequency.exponentialRampToValueAtTime(40, t + 0.3);
    og.gain.setValueAtTime(0, t); og.gain.linearRampToValueAtTime(0.5 * g, t + 0.01); og.gain.exponentialRampToValueAtTime(0.001, t + 0.4);
    o.connect(og); og.connect(this.out); o.start(t); o.stop(t + 0.45);
    const n = this.src(this.brown, false), lp = this.filter('lowpass', 700), ng = this.gain(0);
    ng.gain.setValueAtTime(0, t); ng.gain.linearRampToValueAtTime(0.8 * g, t + 0.005); ng.gain.exponentialRampToValueAtTime(0.001, t + 0.3);
    n.connect(lp); lp.connect(ng); ng.connect(this.out); n.start(t); n.stop(t + 0.35);
  }
}
