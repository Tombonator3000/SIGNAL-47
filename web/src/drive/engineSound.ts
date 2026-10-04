import type { SurfaceKind } from './Drive';

// The truck's V8, heard from the cab, made only from Web Audio nodes (no samples).
// Two detuned sawtooths at the firing rate and half of it, a little square for rasp, and
// band-passed rumble noise go through a resonant low-pass that opens with rpm and load,
// then through a fixed "cab" low-pass. Tyre and wind noise have their own small bus.
// Levels sit under the wind ambience (AudioSys: wind 0.34 outside, motors 0.16).
// Route `dest` to the game's sfx bus so the volume setting and pause apply.

const MASTER = 0.33;

export class EngineSound {
  private out: GainNode;
  private nodes: AudioScheduledSourceNode[] = [];
  private eng: { bus: GainNode; lp: BiquadFilterNode; osc: OscillatorNode[]; lfo: OscillatorNode; noise: GainNode; band: BiquadFilterNode } | null = null;
  private tyre: { roll: GainNode; crunch: GainNode; shake: GainNode; rumble: GainNode; wind: GainNode } | null = null;
  private white: AudioBuffer; private brown: AudioBuffer;
  private running = false;
  private stalled = false;
  private level = 0.1;

  constructor(private ctx: AudioContext, dest: AudioNode) {
    this.out = ctx.createGain(); this.out.gain.value = 0; this.out.connect(dest);
    this.white = this.noise(false); this.brown = this.noise(true);
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

  private build() {
    const c = this.ctx;
    const cab = this.filter('lowpass', 1100, 0.5);
    cab.connect(this.out);
    const bus = this.gain(0), lp = this.filter('lowpass', 300, 3);
    lp.connect(bus); bus.connect(cab);
    const osc: OscillatorNode[] = [];
    const gains: GainNode[] = [];
    for (const [type, g] of [['sawtooth', 0.5], ['sawtooth', 0.38], ['square', 0.06]] as const) {
      const o = c.createOscillator(); o.type = type;
      const og = this.gain(g); o.connect(og); og.connect(lp);
      osc.push(o); gains.push(og);
    }
    // the lope: the firing note's loudness wobbles at a quarter of its rate
    const lfo = c.createOscillator(); lfo.frequency.value = 10;
    const depth = this.gain(0.2); lfo.connect(depth); depth.connect(gains[0].gain);
    const n = this.src(this.brown), band = this.filter('bandpass', 120, 0.8), noise = this.gain(0.08);
    n.connect(band); band.connect(noise); noise.connect(bus);
    this.eng = { bus, lp, osc, lfo, noise, band };
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
    this.nodes.push(...osc, lfo, n, shakeSrc);
    for (const s of this.nodes) s.start();
  }

  /** Engine on (idle) and the tyre layers ready. */
  start() {
    if (!this.eng) this.build();
    this.running = true; this.stalled = false;
    const t = this.ctx.currentTime;
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(MASTER, t, 0.25);
    this.set(650, 0);
  }

  /** Fade everything out and free the nodes. */
  stop() {
    if (!this.eng) return;
    const t = this.ctx.currentTime;
    this.running = false;
    this.out.gain.cancelScheduledValues(t);
    this.out.gain.setTargetAtTime(0, t, 0.15);
    for (const s of this.nodes) s.stop(t + 1);
    this.nodes = []; this.eng = null; this.tyre = null;
  }

  /** Every frame: engine speed in rpm, load 0..1 (the throttle). */
  set(rpm: number, load: number) {
    const e = this.eng;
    if (!e || !this.running || this.stalled) return;
    const t = this.ctx.currentTime;
    const f = Math.max(rpm, 300) / 60 * 4; // a V8 fires four times a turn
    const l = Math.min(1, Math.max(0, load));
    e.osc[0].frequency.setTargetAtTime(f, t, 0.05);
    e.osc[1].frequency.setTargetAtTime(f * 0.5 * 1.007, t, 0.05);
    e.osc[2].frequency.setTargetAtTime(f * 2 * 0.996, t, 0.05);
    e.lfo.frequency.setTargetAtTime(f * 0.25, t, 0.05);
    e.lp.frequency.setTargetAtTime(150 + f * 3 + l * 650, t, 0.08);
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
    g.cancelScheduledValues(t); g.setValueAtTime(L, t);
    [[0.08, 1.25], [0.3, 0.45], [0.42, 0.85], [0.72, 0.25], [0.84, 0.45], [1.3, 0]].forEach(([dt, k]) => g.linearRampToValueAtTime(L * k, t + dt));
    e.noise.gain.setTargetAtTime(0, t + 0.2, 0.2);
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
