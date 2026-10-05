import * as THREE from 'three';

// The desert outside, low and never the same for long (Tom, 5 October: the sounds outside
// should vary a little, quietly, so it is not the same sound all the time).
//
//   wind      the recording, its level and pitch wandering, under a synthesised wind whose
//             colour wanders too, with gusts now and then and calm spells
//   crickets  the recording, faint, and a few single crickets around the listener, each
//             with its own pitch, place and pace. They chirp slower as the night cools
//             (about 13 °C at midnight, 6 °C at dawn) and fall silent towards dawn.
//   events    far off and rare: an owl, a dog at a ranch, a gust in the scrub (recordings);
//             a poorwill, coyotes, a freight train's horn on the line to Roswell, a truck on
//             the highway, and birds when the sky greys (made here).
//   mood      how much is going on: it wanders every minute or two, and now and then the
//             night goes almost quiet for a while.
//
// Everything goes through one outdoor level and a low-pass: indoors the walls take the top
// off and most of the level, in the cab the engine covers it. Nothing here answers the player.

type Space = 'room' | 'yard' | 'lab';
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const pick = <T>(a: T[]) => a[Math.floor(Math.random() * a.length)];

/** A value that wanders: now and then it picks a new target and an AudioParam drifts there. */
class Drift {
  next = 0; value: number;
  constructor(private param: AudioParam, public lo: number, public hi: number, private every: [number, number], private tau: [number, number]) {
    this.value = rnd(lo, hi); param.value = this.value;
  }
  step(dt: number, now: number, scale = 1) {
    this.next -= dt;
    if (this.next > 0) return;
    this.next = rnd(...this.every);
    this.value = rnd(this.lo, this.hi) * scale;
    this.param.setTargetAtTime(this.value, now, rnd(...this.tau));
  }
}

/** One cricket in the grass: a pure tone in short pulses, three or four to a chirp. */
class Cricket {
  osc: OscillatorNode; env: GainNode; vol: GainNode; pan: PannerNode;
  next: number; pulses = 3 + Math.floor(Math.random() * 2); pace = rnd(0.85, 1.15);
  pos = new THREE.Vector3(); level = rnd(0.5, 1); dying = false; gone = false;
  constructor(private ctx: AudioContext, dest: AudioNode, at: THREE.Vector3) {
    this.osc = ctx.createOscillator(); this.osc.frequency.value = rnd(4200, 5200);
    this.env = ctx.createGain(); this.env.gain.value = 0;
    this.vol = ctx.createGain(); this.vol.gain.value = 0;
    this.pan = ctx.createPanner();
    Object.assign(this.pan, { panningModel: 'equalpower', distanceModel: 'inverse', refDistance: 6, rolloffFactor: 1, maxDistance: 80 });
    const a = Math.random() * Math.PI * 2, d = rnd(5, 22);
    this.pos.set(at.x + Math.cos(a) * d, at.y, at.z + Math.sin(a) * d);
    if ((this.pan as any).positionX) { this.pan.positionX.value = this.pos.x; this.pan.positionY.value = this.pos.y; this.pan.positionZ.value = this.pos.z; }
    else (this.pan as any).setPosition(this.pos.x, this.pos.y, this.pos.z);
    this.osc.connect(this.env); this.env.connect(this.vol); this.vol.connect(this.pan); this.pan.connect(dest);
    this.osc.start();
    this.next = ctx.currentTime + rnd(0.2, 2);
    this.vol.gain.setTargetAtTime(0.022 * this.level, ctx.currentTime, rnd(3, 8));
  }
  /** Chirps for the next half second, at `rate` chirps a second. */
  schedule(rate: number) {
    const now = this.ctx.currentTime, g = this.env.gain;
    if (this.next < now) this.next = now + 0.05;
    while (!this.dying && this.next < now + 0.5) {
      for (let k = 0; k < this.pulses; k++) {
        const t = this.next + k * 0.034;
        g.setValueAtTime(0, t); g.linearRampToValueAtTime(1, t + 0.005); g.setValueAtTime(1, t + 0.015); g.linearRampToValueAtTime(0, t + 0.022);
      }
      // now and then a pause, as if it had heard something
      this.next += Math.random() < 0.04 ? rnd(2, 6) : (1 / Math.max(0.15, rate * this.pace)) * rnd(0.92, 1.08);
    }
  }
  fade(secs: number) {
    this.dying = true;
    const now = this.ctx.currentTime;
    this.vol.gain.setTargetAtTime(0, now, secs / 3);
    setTimeout(() => { try { this.osc.stop(); } catch { /* stopped */ } this.pan.disconnect(); this.gone = true; }, secs * 1000 + 500);
  }
}

export class Ambience {
  private out: GainNode;
  private lp: BiquadFilterNode;
  private windRec: { src: AudioBufferSourceNode; gain: GainNode; level: Drift; rate: number; rateNext: number } | null = null;
  private wind: { gain: GainNode; bp: BiquadFilterNode; level: Drift; colour: Drift };
  private bugRec: { src: AudioBufferSourceNode; gain: GainNode; level: Drift } | null = null;
  private crickets: Cricket[] = [];
  private t = { gust: rnd(15, 40), life: rnd(20, 50), poorwill: rnd(60, 180), coyote: rnd(150, 420), train: rnd(240, 900), truck: rnd(120, 400), birds: rnd(10, 40), mood: rnd(60, 150), bugs: 0 };
  private mood = 0.7;
  private space: Space = 'room';

  constructor(private ctx: AudioContext, dest: AudioNode, private buf: Record<string, AudioBuffer>, private noise: AudioBuffer, private brown: AudioBuffer) {
    this.out = ctx.createGain(); this.out.gain.value = 0;
    this.lp = ctx.createBiquadFilter(); this.lp.type = 'lowpass'; this.lp.frequency.value = 900;
    this.out.connect(this.lp); this.lp.connect(dest);
    const now = ctx.currentTime;
    // the wind as recorded
    if (buf.wind) {
      const src = ctx.createBufferSource(); src.buffer = buf.wind; src.loop = true;
      const gain = ctx.createGain(); src.connect(gain); gain.connect(this.out); src.start(now, rnd(0, buf.wind.duration));
      this.windRec = { src, gain, level: new Drift(gain.gain, 0.14, 0.4, [8, 26], [3, 7]), rate: 1, rateNext: 0 };
    }
    // a wind made of noise under it, its colour wandering
    const n = ctx.createBufferSource(); n.buffer = brown; n.loop = true;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 0.7; bp.frequency.value = 420;
    const wg = ctx.createGain(); n.connect(bp); bp.connect(wg); wg.connect(this.out); n.start(now, rnd(0, 2));
    this.wind = { gain: wg, bp, level: new Drift(wg.gain, 0.02, 0.16, [6, 20], [2.5, 6]), colour: new Drift(bp.frequency, 260, 720, [5, 16], [2, 5]) };
    // the crickets as recorded: faint, under the single ones
    if (buf.crickets) {
      const src = ctx.createBufferSource(); src.buffer = buf.crickets; src.loop = true;
      const gain = ctx.createGain(); src.connect(gain); gain.connect(this.out); src.start(now, rnd(0, buf.crickets.duration));
      this.bugRec = { src, gain, level: new Drift(gain.gain, 0.02, 0.07, [10, 30], [3, 8]) };
    }
  }

  /** Where the listener is: outside, indoors (muffled) or in the cab (under the engine). */
  setSpace(space: Space) {
    this.space = space;
    const t = this.ctx.currentTime;
    const [level, cut] = space === 'yard' ? [1, 9000] : space === 'room' ? [0.3, 700] : [0.1, 500];
    this.out.gain.setTargetAtTime(level, t, 0.8);
    this.lp.frequency.setTargetAtTime(cut, t, 0.5);
  }

  /** Every frame while a night runs. clock: the night's clock in seconds (it may run past 24 h). */
  update(dt: number, at: THREE.Vector3, clock: number) {
    const ctx = this.ctx, now = ctx.currentTime, t = this.t;
    const hour = (((clock % 86400) + 86400) % 86400) / 3600;
    const sinceDusk = (hour - 21 + 24) % 24;                       // 21:00 is 0
    const temp = 16 - 1.2 * sinceDusk;                              // about 13 °C at midnight, 6 at dawn
    const dawn = THREE.MathUtils.clamp((hour - 4.9) / 0.6, 0, 1) * (hour < 12 ? 1 : 0);
    // ---------- how much is going on ----------
    if ((t.mood -= dt) <= 0) {
      t.mood = rnd(60, 180);
      this.mood = Math.random() < 0.15 ? 0.08 : rnd(0.35, 1);       // now and then the night goes quiet
    }
    const m = this.mood;
    // ---------- wind ----------
    if (this.windRec) {
      this.windRec.level.step(dt, now, 0.4 + 0.6 * m);
      if ((this.windRec.rateNext -= dt) <= 0) {
        this.windRec.rateNext = rnd(10, 30);
        this.windRec.src.playbackRate.setTargetAtTime(rnd(0.88, 1.06), now, 6);
      }
    }
    this.wind.level.step(dt, now, 0.3 + 0.7 * m);
    this.wind.colour.step(dt, now);
    if ((t.gust -= dt) <= 0) { t.gust = rnd(14, 55) / (0.4 + m); this.gust(); }
    // ---------- crickets: fewer and slower as it cools, gone by dawn ----------
    const active = THREE.MathUtils.clamp((temp - 6.3) / 5, 0, 1) * (1 - dawn);
    const rate = Math.max(0.2, (temp - 4) / 7.5);
    this.bugRec?.level.step(dt, now, active * (0.5 + 0.5 * m));
    if ((t.bugs -= dt) <= 0) {
      t.bugs = rnd(4, 12);
      const want = Math.round(active * (1 + 4 * m));
      const live = this.crickets.filter((c) => !c.dying);
      for (const c of live) if (c.pos.distanceTo(at) > 32) c.fade(4);  // left behind
      const near = live.filter((c) => c.pos.distanceTo(at) <= 32);
      if (near.length < want) this.crickets.push(new Cricket(ctx, this.out, at));
      else if (near.length > want && near.length) pick(near).fade(rnd(4, 12));
      this.crickets = this.crickets.filter((c) => !c.gone);
    }
    for (const c of this.crickets) c.schedule(rate);
    // ---------- far off ----------
    if (this.space === 'lab') return;                                // the engine covers it
    const busy = 0.3 + m;
    if ((t.life -= dt) <= 0) {
      t.life = rnd(25, 70) / busy;
      const r = Math.random();
      const name = r < 0.35 ? 'owl' : r < 0.6 ? pick(['dog0', 'dog1']) : 'gust';
      if (!(name === 'owl' && dawn > 0.6)) this.far(name, at, name === 'gust' ? 0.3 : name === 'owl' ? 0.26 : 0.18, name === 'owl' ? rnd(0.9, 1.05) : rnd(0.95, 1.05));
    }
    if ((t.poorwill -= dt) <= 0) {
      // they call most in the grey before dawn
      const keen = hour >= 3.5 && hour < 5.3 ? 1 : 0.35;
      t.poorwill = rnd(90, 260) / (keen * busy);
      if (temp > 5 && dawn < 0.9) this.poorwill(at);
    }
    if ((t.coyote -= dt) <= 0) {
      t.coyote = rnd(240, 600) / busy;
      if (m > 0.2) this.coyotes(at);
    }
    if ((t.train -= dt) <= 0) { t.train = rnd(700, 1500); this.train(at); }
    if ((t.truck -= dt) <= 0) { t.truck = rnd(200, 520) / busy; this.truck(); }
    if (dawn > 0 && (t.birds -= dt) <= 0) {
      t.birds = rnd(12, 40) / (0.3 + dawn);
      if (Math.random() < 0.55) this.meadowlark(at); else this.lark(at);
    }
  }

  // ---------- helpers ----------
  /** A panner some way off in a random direction (or a given bearing), heard at about `gain`. */
  private distant(at: THREE.Vector3, d: number, gain: number, cut: number, bearing?: number) {
    const ctx = this.ctx;
    const a = bearing ?? Math.random() * Math.PI * 2;
    const p = ctx.createPanner();
    Object.assign(p, { panningModel: 'equalpower', distanceModel: 'inverse', refDistance: d, rolloffFactor: 1, maxDistance: d * 4 });
    const x = at.x + Math.cos(a) * d, y = at.y + 2, z = at.z + Math.sin(a) * d;
    if ((p as any).positionX) { p.positionX.value = x; p.positionY.value = y; p.positionZ.value = z; } else (p as any).setPosition(x, y, z);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = cut;
    const g = ctx.createGain(); g.gain.value = gain;
    g.connect(lp); lp.connect(p); p.connect(this.out);
    setTimeout(() => p.disconnect(), 40000);
    return g;
  }
  /** A short echo off the mesas, for things a long way away. */
  private echo(into: AudioNode, delay = 0.22, fb = 0.35) {
    const ctx = this.ctx;
    const d = ctx.createDelay(1); d.delayTime.value = delay;
    const f = ctx.createGain(); f.gain.value = fb;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 900;
    const inp = ctx.createGain();
    inp.connect(into); inp.connect(d); d.connect(lp); lp.connect(f); f.connect(d); lp.connect(into);
    setTimeout(() => { inp.disconnect(); d.disconnect(); }, 30000);
    return inp;
  }
  /** A recording far off in a random direction: panned, quiet and dull with distance. */
  far(name: string, at: THREE.Vector3, gain: number, rate = 1) {
    const b = this.buf[name]; if (!b) return;
    const d = rnd(30, 60);
    const g = this.distant(at, d, gain, 2600 - d * 20);
    const src = this.ctx.createBufferSource(); src.buffer = b; src.playbackRate.value = rate;
    src.connect(g); src.start();
  }
  /** A whistled note: a sine (and a little of its octave) gliding from f0 to f1. */
  private note(dest: AudioNode, t: number, len: number, f0: number, f1: number, peak: number, harm = 0.12) {
    const ctx = this.ctx;
    const o = ctx.createOscillator(); o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(f1, t + len);
    const o2 = ctx.createOscillator(); o2.frequency.setValueAtTime(f0 * 2, t); o2.frequency.exponentialRampToValueAtTime(f1 * 2, t + len);
    const h = ctx.createGain(); h.gain.value = harm;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(peak, t + Math.min(0.03, len * 0.3));
    g.gain.setValueAtTime(peak, t + len * 0.7); g.gain.linearRampToValueAtTime(0, t + len);
    o.connect(g); o2.connect(h); h.connect(g); g.connect(dest);
    o.start(t); o2.start(t); o.stop(t + len + 0.02); o2.stop(t + len + 0.02);
  }

  // ---------- the wind gusts ----------
  private gust() {
    const ctx = this.ctx, now = ctx.currentTime, w = this.wind;
    const up = rnd(1.2, 3), hold = rnd(0.5, 2.5), down = rnd(3, 7), peak = rnd(0.12, 0.3);
    w.gain.gain.cancelScheduledValues(now); w.gain.gain.setValueAtTime(w.gain.gain.value, now);
    w.gain.gain.linearRampToValueAtTime(peak, now + up); w.gain.gain.setValueAtTime(peak, now + up + hold);
    w.gain.gain.setTargetAtTime(w.level.value, now + up + hold, down / 3);
    w.bp.frequency.cancelScheduledValues(now); w.bp.frequency.setValueAtTime(w.bp.frequency.value, now);
    w.bp.frequency.linearRampToValueAtTime(rnd(800, 1300), now + up);
    w.bp.frequency.setTargetAtTime(w.colour.value, now + up + hold, down / 3);
    w.level.next = up + hold + down; w.colour.next = up + hold + down;
  }

  // ---------- made here ----------
  /** The common poorwill: "poor-will", a soft low note and a rising one, three to eight times. */
  private poorwill(at: THREE.Vector3) {
    const g = this.distant(at, rnd(40, 110), 1, 3200), t0 = this.ctx.currentTime + 0.1;
    const n = 3 + Math.floor(Math.random() * 6), f = rnd(0.95, 1.06);
    for (let i = 0; i < n; i++) {
      const t = t0 + i * rnd(1.05, 1.3);
      this.note(g, t, 0.11, 1900 * f, 1820 * f, 0.03);
      this.note(g, t + 0.17, 0.2, 1980 * f, 2650 * f, 0.04);
    }
  }
  /** Coyotes a long way off: a few voices, yips and a howl or two, with the mesas answering. */
  private coyotes(at: THREE.Vector3) {
    const ctx = this.ctx, d = rnd(250, 700);
    const out = this.echo(this.distant(at, d, 1, 1700), rnd(0.18, 0.3), 0.32);
    const t0 = ctx.currentTime + 0.2, voices = 2 + Math.floor(Math.random() * 3);
    for (let v = 0; v < voices; v++) {
      const o = ctx.createOscillator(); o.type = 'sawtooth';
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = rnd(900, 1300); bp.Q.value = 1.4;
      const g = ctx.createGain(); g.gain.value = 0;
      o.connect(bp); bp.connect(g); g.connect(out);
      const vib = ctx.createOscillator(); vib.frequency.value = rnd(5, 6.5); const vg = ctx.createGain(); vg.gain.value = 0;
      vib.connect(vg); vg.connect(o.frequency); vib.start(t0); o.start(t0);
      let t = t0 + rnd(0, 1.2), end = t0 + rnd(4, 7);
      const k = rnd(0.9, 1.15);
      while (t < end) {
        if (Math.random() < 0.3) {   // a howl
          const len = rnd(0.9, 1.7);
          o.frequency.setValueAtTime(430 * k, t); o.frequency.exponentialRampToValueAtTime(880 * k, t + 0.3);
          o.frequency.setValueAtTime(880 * k, t + len - 0.45); o.frequency.exponentialRampToValueAtTime(520 * k, t + len);
          vg.gain.setValueAtTime(0, t + 0.3); vg.gain.linearRampToValueAtTime(22, t + 0.5); vg.gain.setValueAtTime(22, t + len - 0.4); vg.gain.linearRampToValueAtTime(0, t + len);
          g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.022, t + 0.25); g.gain.setValueAtTime(0.022, t + len - 0.3); g.gain.linearRampToValueAtTime(0, t + len);
          t += len + rnd(0.1, 0.5);
        } else {                     // a run of yips
          for (let y = 0, n = 2 + Math.floor(Math.random() * 4); y < n; y++) {
            const len = rnd(0.06, 0.11);
            o.frequency.setValueAtTime(700 * k, t); o.frequency.exponentialRampToValueAtTime(1250 * k, t + len * 0.5); o.frequency.exponentialRampToValueAtTime(820 * k, t + len);
            g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.02, t + 0.012); g.gain.linearRampToValueAtTime(0, t + len);
            t += len + rnd(0.05, 0.16);
          }
          t += rnd(0.1, 0.4);
        }
      }
      o.stop(end + 2); vib.stop(end + 2);
    }
  }
  /** A freight train's horn on the line to Roswell, miles off: long, long, short, long. */
  private train(at: THREE.Vector3) {
    const ctx = this.ctx;
    const out = this.echo(this.distant(at, 1500, 1, 700, rnd(0, Math.PI * 2)), 0.32, 0.45);
    let t = ctx.currentTime + 0.3;
    for (const len of [1.8, 1.8, 0.7, 2.8]) {
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.012, t + 0.18); g.gain.setValueAtTime(0.012, t + len - 0.3); g.gain.linearRampToValueAtTime(0, t + len);
      g.connect(out);
      for (const f of [311, 370, 494]) {
        const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = f * rnd(0.995, 1.005);
        o.connect(g); o.start(t); o.stop(t + len + 0.05);
      }
      t += len + (len < 1 ? 0.35 : 0.55);
    }
    // and the rumble of the train itself, rising and going
    const n = ctx.createBufferSource(); n.buffer = this.brown; n.loop = true;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 110;
    const g = ctx.createGain(); const t0 = ctx.currentTime;
    g.gain.setValueAtTime(0, t0); g.gain.linearRampToValueAtTime(0.05, t0 + 8); g.gain.linearRampToValueAtTime(0, t0 + 24);
    n.connect(lp); lp.connect(g); g.connect(out); n.start(t0); n.stop(t0 + 25);
  }
  /** A truck on the highway: tyres and a diesel coming up out of nothing and going away. */
  private truck() {
    const ctx = this.ctx, t0 = ctx.currentTime, len = rnd(14, 22), mid = t0 + len * 0.5;
    const pan = ctx.createStereoPanner(); const dir = Math.random() < 0.5 ? -1 : 1;
    pan.pan.setValueAtTime(-0.9 * dir, t0); pan.pan.linearRampToValueAtTime(0.9 * dir, t0 + len);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 900;
    pan.connect(lp); lp.connect(this.out);
    const env = (g: GainNode, peak: number) => { g.gain.setValueAtTime(0, t0); g.gain.linearRampToValueAtTime(peak, mid); g.gain.linearRampToValueAtTime(0, t0 + len); g.connect(pan); };
    const n = ctx.createBufferSource(); n.buffer = this.noise; n.loop = true;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 700; bp.Q.value = 0.6;
    const ng = ctx.createGain(); n.connect(bp); bp.connect(ng); env(ng, 0.018);
    const o = ctx.createOscillator(); o.type = 'sawtooth';
    o.frequency.setValueAtTime(78, t0); o.frequency.setValueAtTime(78, mid - 1); o.frequency.linearRampToValueAtTime(66, mid + 1);  // the Doppler drop
    const ol = ctx.createBiquadFilter(); ol.type = 'lowpass'; ol.frequency.value = 240;
    const og = ctx.createGain(); o.connect(ol); ol.connect(og); env(og, 0.025);
    n.start(t0); o.start(t0); n.stop(t0 + len + 0.1); o.stop(t0 + len + 0.1);
    setTimeout(() => pan.disconnect(), (len + 1) * 1000);
  }
  /** A meadowlark when the sky greys: a few clear slurred whistles, then a quick jumble. */
  private meadowlark(at: THREE.Vector3) {
    const g = this.distant(at, rnd(40, 90), 1, 5000);
    let t = this.ctx.currentTime + 0.1; const f = rnd(0.93, 1.07);
    for (const [len, a, b] of [[0.28, 2650, 2400], [0.22, 2950, 2700], [0.12, 2150, 2350]] as const) { this.note(g, t, len, a * f, b * f, 0.022, 0.2); t += len + 0.04; }
    for (let i = 0, n = 4 + Math.floor(Math.random() * 4); i < n; i++) { const a = rnd(1900, 3300) * f; this.note(g, t, 0.06, a, a * rnd(0.85, 1.15), 0.016, 0.25); t += 0.075; }
  }
  /** Horned larks: high, quick, tinkling notes from the open ground. */
  private lark(at: THREE.Vector3) {
    const g = this.distant(at, rnd(30, 70), 1, 7000);
    let t = this.ctx.currentTime + 0.1;
    for (let i = 0, n = 10 + Math.floor(Math.random() * 12); i < n; i++) { const a = rnd(3600, 6000); this.note(g, t, rnd(0.03, 0.05), a, a * rnd(0.9, 1.12), 0.009, 0.05); t += rnd(0.05, 0.13); }
  }
}
