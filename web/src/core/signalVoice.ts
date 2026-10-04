// The anomaly near 1420.405 MHz as you hear it while tuning in, after the signal in
// "Contact" (1997): the hiss of the array gives way to a pulsing, structured interference,
// then heavy low thumps like machinery cycling, with a metallic whine that rises and falls
// over them. The thumps keep this game's own rhythm, the 4/7 pulse group every 5.2 s
// (Signal.ts), not the film's prime numbers. All synthesis, no samples.
//
// clarity 0 is plain static; 1 is the locked signal. The receiver drives it every frame.

// 4 pulses, a gap, 7 pulses, every 5.2 s; each pulse 0.11 s in a 0.22 s slot (as Rx.pulse).
export function pulseAt(t: number) {
  const c = ((t % 5.2) + 5.2) % 5.2;
  const count = c < 1.6 ? 4 : 7;
  const local = c < 1.6 ? c : c - 2.1;
  if (local < 0) return { on: false, group: false };
  const group = local < count * 0.22;
  return { on: group && (local % 0.22) < 0.11, group };
}

export class SignalVoice {
  private out: GainNode;
  private stat: GainNode;
  private statGate: GainNode;
  private whine: GainNode;
  private whineBand: BiquadFilterNode;
  private nodes: AudioScheduledSourceNode[] = [];
  private last = false;
  private clarity = 0;
  private shaper: WaveShaperNode;
  private tremDepth: GainNode;
  private thumpBus: GainNode;
  private nb: AudioBuffer | null = null;

  constructor(private ctx: AudioContext, dest: AudioNode, noise: AudioBuffer) {
    const c = ctx;
    this.out = c.createGain(); this.out.gain.value = 0; this.out.connect(dest);
    // a soft limiter so the thumps can be heavy without clipping the mix
    const comp = c.createDynamicsCompressor(); comp.threshold.value = -16; comp.ratio.value = 5; comp.attack.value = 0.003; comp.release.value = 0.2;
    comp.connect(this.out);
    this.shaper = c.createWaveShaper(); this.shaper.curve = drive(2.6); this.shaper.oversample = '2x';

    // the array's hiss, which a slow tremolo turns into a pulsing interference as clarity rises
    const n = c.createBufferSource(); n.buffer = noise; n.loop = true;
    const bp = c.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1700; bp.Q.value = 0.55;
    this.statGate = c.createGain(); this.statGate.gain.value = 1;
    this.stat = c.createGain(); this.stat.gain.value = 0;
    n.connect(bp); bp.connect(this.statGate); this.statGate.connect(this.stat); this.stat.connect(comp);
    const trem = c.createOscillator(); trem.frequency.value = 4.55;
    this.tremDepth = c.createGain(); this.tremDepth.gain.value = 0; trem.connect(this.tremDepth); this.tremDepth.connect(this.statGate.gain);

    // the whine: two detuned buzzing oscillators and a comb-filtered noise, through a
    // resonant band that wanders up and down, ring-modulated for the metal in it
    this.whine = c.createGain(); this.whine.gain.value = 0;
    this.whineBand = c.createBiquadFilter(); this.whineBand.type = 'bandpass'; this.whineBand.frequency.value = 1900; this.whineBand.Q.value = 7;
    const ring = c.createGain(); ring.gain.value = 0.55;
    const ringOsc = c.createOscillator(); ringOsc.frequency.value = 37; const ringDepth = c.createGain(); ringDepth.gain.value = 0.45; ringOsc.connect(ringDepth); ringDepth.connect(ring.gain);
    const o1 = c.createOscillator(); o1.type = 'sawtooth'; o1.frequency.value = 1460;
    const o2 = c.createOscillator(); o2.type = 'square'; o2.frequency.value = 1473;
    const og = c.createGain(); og.gain.value = 0.18; o1.connect(og); o2.connect(og);
    const pitch = c.createOscillator(); pitch.frequency.value = 0.21; const pd = c.createGain(); pd.gain.value = 260;
    pitch.connect(pd); pd.connect(o1.frequency); pd.connect(o2.frequency);
    const sweep = c.createOscillator(); sweep.frequency.value = 0.13; const sd = c.createGain(); sd.gain.value = 650;
    sweep.connect(sd); sd.connect(this.whineBand.frequency);
    // metal scraped slowly: noise in a short comb whose length drifts
    const sn = c.createBufferSource(); sn.buffer = noise; sn.loop = true; sn.playbackRate.value = 0.37;
    const comb = c.createDelay(0.05); comb.delayTime.value = 0.0027;
    const fb = c.createGain(); fb.gain.value = 0.86;
    const cd = c.createOscillator(); cd.frequency.value = 0.09; const cdd = c.createGain(); cdd.gain.value = 0.0009; cd.connect(cdd); cdd.connect(comb.delayTime);
    const sg = c.createGain(); sg.gain.value = 0.22;
    sn.connect(comb); comb.connect(fb); fb.connect(comb); comb.connect(sg);
    og.connect(this.whineBand); sg.connect(this.whineBand);
    this.whineBand.connect(ring); ring.connect(this.whine); this.whine.connect(comp);

    // the thumps go through the drive, then the limiter
    this.thumpBus = c.createGain(); this.thumpBus.gain.value = 1;
    this.thumpBus.connect(this.shaper); this.shaper.connect(comp);

    for (const s of [n, trem, ringOsc, o1, o2, pitch, sweep, sn, cd]) { s.start(); this.nodes.push(s); }
  }
  /** Called every frame: how clear the signal is (0..1), how loud overall, and the time. */
  frame(clarity: number, level: number, t: number) {
    const c = this.ctx, now = c.currentTime;
    this.clarity = clarity;
    const p = pulseAt(t);
    this.out.gain.setTargetAtTime(level, now, 0.12);
    // static: plain at first, then chopped into a pulse as the structure comes through
    this.stat.gain.setTargetAtTime(0.05 * (1 - 0.65 * clarity), now, 0.1);
    this.tremDepth.gain.setTargetAtTime(0.75 * clarity, now, 0.2);
    // the whine grows with clarity and swells inside a pulse group
    this.whine.gain.setTargetAtTime(0.05 * Math.pow(clarity, 1.6) * (p.group ? 1 : 0.55), now, 0.08);
    if (p.on && !this.last && clarity > 0.12) this.thump(0.85 * clarity * clarity);
    this.last = p.on;
  }

  // One heavy pulse: a sine that falls from 118 to 41 Hz, a second one an octave up for
  // the punch, and a dull clank of filtered noise, all through the drive.
  private thump(strength: number) {
    const c = this.ctx, t = c.currentTime + 0.005;
    const env = c.createGain(); env.gain.setValueAtTime(0, t);
    env.gain.linearRampToValueAtTime(strength, t + 0.006); env.gain.exponentialRampToValueAtTime(0.001, t + 0.34);
    env.connect(this.thumpBus);
    for (const [f0, f1, g] of [[118, 41, 1], [236, 82, 0.35]] as const) {
      const o = c.createOscillator(); o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(f1, t + 0.16);
      const og = c.createGain(); og.gain.value = g; o.connect(og); og.connect(env);
      o.start(t); o.stop(t + 0.36);
    }
    const n = c.createBufferSource(); n.buffer = this.noiseBuf();
    const bp = c.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 310; bp.Q.value = 2.2;
    const ng = c.createGain(); ng.gain.setValueAtTime(0.5 * strength, t); ng.gain.exponentialRampToValueAtTime(0.001, t + 0.09);
    n.connect(bp); bp.connect(ng); ng.connect(this.thumpBus);
    n.start(t); n.stop(t + 0.1);
  }
  private noiseBuf() {
    if (this.nb) return this.nb;
    const c = this.ctx, b = c.createBuffer(1, c.sampleRate * 0.2, c.sampleRate), d = b.getChannelData(0);
    for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
    return (this.nb = b);
  }

  /** Fade out (the receiver is off, the night is over); `stop` frees the nodes. */
  silence() { this.out.gain.setTargetAtTime(0, this.ctx.currentTime, 0.2); this.clarity = 0; }
  stop() {
    this.silence();
    const nodes = this.nodes; this.nodes = [];
    setTimeout(() => { for (const s of nodes) { try { s.stop(); } catch { /* already */ } } this.out.disconnect(); }, 900);
  }
  get on() { return this.clarity > 0; }
  /** How clear it is right now (tests read this). */
  get level() { return this.clarity; }
}

// A soft-clipping curve: heavier `k`, more grit.
function drive(k: number) {
  const n = 1024, curve = new Float32Array(n);
  for (let i = 0; i < n; i++) { const x = i / (n - 1) * 2 - 1; curve[i] = Math.tanh(k * x) / Math.tanh(k); }
  return curve;
}
