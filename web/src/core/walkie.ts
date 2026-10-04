// Tomás' field radio from 1947, on the table in room 6 (KAPITLER.md, Room 6). It has had no
// battery since 1947 and it plays anyway, the same every time, like any reference: hiss, an
// engine far off that coughs twice and dies, the hiss gone, relay clicks in four and seven,
// somebody breathing close to the microphone, and the hiss again. It is THE EVENT on the
// Roswell road, an hour before it happens; nothing says so. Press to talk and the radio goes
// quiet (it is sending), let go and the clicks come back, four and seven. No voice (R5).
//
// Everything is made here and sent through a small speaker: high and low cut, a little
// clipping. The node graph is built on start and torn down on stop.

export class WalkieSound {
  private out: GainNode | null = null;
  private hiss: GainNode | null = null;
  private srcs: AudioScheduledSourceNode[] = [];
  private speaker: AudioNode | null = null;

  constructor(private ctx: AudioContext, private dest: AudioNode, private noise: AudioBuffer) {}

  get active() { return !!this.out; }

  private build() {
    const ctx = this.ctx;
    const out = ctx.createGain(); out.gain.value = 0.9;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 420;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 2900;
    const sh = ctx.createWaveShaper();
    const curve = new Float32Array(256);
    for (let i = 0; i < 256; i++) { const x = i / 127.5 - 1; curve[i] = Math.tanh(x * 2.2) / Math.tanh(2.2); }
    sh.curve = curve;
    hp.connect(lp); lp.connect(sh); sh.connect(out); out.connect(this.dest);
    this.out = out; this.speaker = hp;
    // the hiss runs all the time the radio is on; the sequence opens and shuts it
    const n = this.noiseSrc();
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 2000; bp.Q.value = 0.6;
    const hiss = ctx.createGain(); hiss.gain.value = 0;
    n.connect(bp); bp.connect(hiss); hiss.connect(hp);
    n.start();
    this.hiss = hiss;
  }
  private noiseSrc() {
    const s = this.ctx.createBufferSource(); s.buffer = this.noise; s.loop = true; this.srcs.push(s); return s;
  }

  /** Switch on: the whole sequence. Returns its length in seconds; the hiss stays after it. */
  start(): number {
    this.stop();
    this.build();
    const ctx = this.ctx, t0 = ctx.currentTime, h = this.hiss!.gain, sp = this.speaker!;
    h.setValueAtTime(0, t0); h.linearRampToValueAtTime(0.22, t0 + 2.6);
    h.setValueAtTime(0.22, t0 + 6.95); h.linearRampToValueAtTime(0, t0 + 7.1);   // gone with the engine
    h.setValueAtTime(0, t0 + 16.4); h.linearRampToValueAtTime(0.13, t0 + 17.2);

    // the engine: a low saw chopped at the firing rate, through a muffled exhaust
    const osc = ctx.createOscillator(); osc.type = 'sawtooth';
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 520;
    const fire = ctx.createOscillator(); fire.type = 'square'; fire.frequency.value = 17;
    const fg = ctx.createGain(); fg.gain.value = 0.45; fire.connect(fg);
    const chop = ctx.createGain(); chop.gain.value = 0.55; fg.connect(chop.gain);
    const eng = ctx.createGain(); eng.gain.value = 0;
    osc.connect(lp); lp.connect(chop); chop.connect(eng); eng.connect(sp);
    const f = osc.frequency, g = eng.gain, ff = fire.frequency;
    f.setValueAtTime(34, t0); g.setValueAtTime(0, t0 + 3.0); g.linearRampToValueAtTime(0.42, t0 + 3.7);
    for (const c of [5.0, 5.9]) {                  // two coughs
      g.setValueAtTime(0.42, t0 + c); g.linearRampToValueAtTime(0.04, t0 + c + 0.08); g.linearRampToValueAtTime(0.42, t0 + c + 0.3);
      f.setValueAtTime(34, t0 + c); f.linearRampToValueAtTime(24, t0 + c + 0.12); f.linearRampToValueAtTime(34, t0 + c + 0.35);
      ff.setValueAtTime(17, t0 + c); ff.linearRampToValueAtTime(11, t0 + c + 0.12); ff.linearRampToValueAtTime(17, t0 + c + 0.35);
    }
    f.setValueAtTime(34, t0 + 6.4); f.linearRampToValueAtTime(15, t0 + 7.0);  // and it dies
    ff.setValueAtTime(17, t0 + 6.4); ff.linearRampToValueAtTime(6, t0 + 7.0);
    g.setValueAtTime(0.42, t0 + 6.4); g.linearRampToValueAtTime(0, t0 + 7.05);
    osc.start(t0); fire.start(t0); osc.stop(t0 + 7.2); fire.stop(t0 + 7.2);
    this.srcs.push(osc, fire);

    // two seconds of nothing, then the relay: four, a gap, seven
    this.clicks(t0 + 9.0);
    // somebody breathing, close to the microphone, twice
    this.breath(t0 + 13.7); this.breath(t0 + 15.1);
    return 17.2;
  }

  private clicks(at: number) {
    const ctx = this.ctx, sp = this.speaker; if (!sp) return;
    const times: number[] = [];
    for (let i = 0; i < 4; i++) times.push(at + i * 0.32);
    for (let i = 0; i < 7; i++) times.push(at + 4 * 0.32 + 0.9 + i * 0.32);
    for (const t of times) {
      const n = ctx.createBufferSource(); n.buffer = this.noise;
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 2600; bp.Q.value = 1.4;
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.9, t + 0.002); g.gain.exponentialRampToValueAtTime(0.001, t + 0.03);
      n.connect(bp); bp.connect(g); g.connect(sp);
      n.start(t, Math.random()); n.stop(t + 0.05);
      // and the thump of the armature behind it
      const o = ctx.createOscillator(); o.frequency.setValueAtTime(150, t); o.frequency.exponentialRampToValueAtTime(60, t + 0.04);
      const og = ctx.createGain(); og.gain.setValueAtTime(0.5, t); og.gain.exponentialRampToValueAtTime(0.001, t + 0.05);
      o.connect(og); og.connect(sp); o.start(t); o.stop(t + 0.06);
    }
  }

  private breath(at: number) {
    const ctx = this.ctx, sp = this.speaker; if (!sp) return;
    const n = ctx.createBufferSource(); n.buffer = this.noise;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 0.9;
    bp.frequency.setValueAtTime(650, at); bp.frequency.linearRampToValueAtTime(900, at + 0.8); bp.frequency.linearRampToValueAtTime(560, at + 1.3);
    const g = ctx.createGain(), v = g.gain;
    v.setValueAtTime(0, at); v.linearRampToValueAtTime(0.32, at + 0.55);   // in
    v.linearRampToValueAtTime(0.06, at + 0.75); v.linearRampToValueAtTime(0.26, at + 0.95); // out
    v.linearRampToValueAtTime(0, at + 1.3);
    n.connect(bp); bp.connect(g); g.connect(sp);
    n.start(at, Math.random()); n.stop(at + 1.4);
  }

  /** Press to talk: quiet while sending, then four and seven come back. Returns its length. */
  pressToTalk(): number {
    if (!this.out || !this.hiss) return 0;
    const t0 = this.ctx.currentTime, h = this.hiss.gain;
    h.cancelScheduledValues(t0); h.setValueAtTime(h.value, t0); h.linearRampToValueAtTime(0, t0 + 0.05);
    this.clicks(t0 + 2.3);
    h.setValueAtTime(0, t0 + 7.0); h.linearRampToValueAtTime(0.13, t0 + 7.6);
    return 7.6;
  }

  stop() {
    const out = this.out; if (!out) return;
    const t = this.ctx.currentTime;
    out.gain.setTargetAtTime(0, t, 0.03);
    const srcs = this.srcs; this.srcs = [];
    setTimeout(() => { for (const s of srcs) { try { s.stop(); } catch { /* stopped */ } } out.disconnect(); }, 300);
    this.out = null; this.hiss = null; this.speaker = null;
  }
}
