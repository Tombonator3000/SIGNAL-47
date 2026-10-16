import * as THREE from 'three';
import type { AudioSys } from './Audio';

// The radio stations of the night and the radios that play them (Tom, 16 October: the truck's
// radio should work, and the national anthem should play on the radios).
//
//   1420  Caliche's own little station, the Night Line: a host, callers about the lights,
//         country tunes. Signs off at 04:00 with the national anthem, then the bare carrier,
//         then nothing.
//   1230  Roswell. Off the air until 05:00, when it signs on with the anthem and goes into
//         the morning: cattle prices, the weather, the Masters, the comet (the diner's radio
//         and the old road's radio in chapters five and six are this station).
//   1090  A station a long way off, coming and going in the static (skywave). Gone with the
//         daylight.
//
// The programme runs in real time while the game runs, so a tune or a caller goes on at the
// speed it is heard; only the fixed points (the sign-off, the sign-on) follow the night's
// clock, which runs faster on a drive. A station makes sound only while a radio listens.
//
// The voices are not words but a muted horn that talks, "wah wah", like the grown-ups in
// Peanuts (Tom, 16 October: a little cartoonish, but it works in the surreal night); what they
// say is in the caption. The tunes are made here, plucked strings and a steel guitar in a few
// bars, so they need no licence. The anthem is the United States Air Force Band's choral
// recording (public domain, THIRD_PARTY_NOTICES.md), streamed from public/radio/ when a
// station plays it; the single-file build has no files beside it, so there it is the carrier.

export type StationId = 'caliche' | 'roswell' | 'far';
/** The order the truck's radio steps through (R, or Radio on a phone). */
export const DIAL: StationId[] = ['caliche', 'roswell', 'far'];
export const FREQ: Record<StationId, number> = { caliche: 1420, roswell: 1230, far: 1090 };
export const SIGN_OFF = 4 * 3600;   // the Night Line signs off
export const SIGN_ON = 5 * 3600;    // Roswell signs on
export const ANTHEM_SECS = 74.8;
export const streamOn = import.meta.env.MODE !== 'single';

export type Voice = 'host' | 'woman' | 'man' | 'roswell';
export const PITCH: Record<Voice, number> = { host: 116, woman: 212, man: 138, roswell: 126 };
/** A number from text, the same every time (a line mumbles the same way each time it is said). */
function seed(text: string) { let h = 2166136261; for (let i = 0; i < text.length; i++) h = Math.imul(h ^ text.charCodeAt(i), 16777619); return h >>> 0; }
function rng(n: number) { return () => { n = (n + 0x6d2b79f5) >>> 0; let t = Math.imul(n ^ (n >>> 15), 1 | n); t ^= t + Math.imul(t ^ (t >>> 7), 61 | t); return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }; }
/** The syllables of a line said in `secs`: when each starts, how long it lasts, its pitch (times
 *  the voice's own). A word gets one to four; the pitch wanders, the first of a word is a
 *  little higher, a sentence falls at its end and a question rises. */
export function syllables(text: string, secs: number) {
  const r = rng(seed(text)), words = text.split(/\s+/).filter(Boolean);
  const per = words.map((w) => THREE.MathUtils.clamp(Math.round(w.replace(/[^a-z]/gi, '').length / 3), 1, 4));
  const pauses = words.filter((w) => /[,.;:!?]$/.test(w)).length;
  const total = per.reduce((a, b) => a + b, 0);
  const d = Math.max(0.12, (secs - pauses * 0.16) / total);
  const out: { at: number; d: number; f: number }[] = [];
  let at = 0, f = 1;
  words.forEach((w, i) => {
    for (let k = 0; k < per[i]; k++) {
      f = THREE.MathUtils.clamp(f + (r() - 0.5) * 0.22, 0.86, 1.22);
      const end = i === words.length - 1 && k === per[i] - 1;
      const pitch = end ? (/\?$/.test(w) ? 1.25 : 0.8) : f * (k === 0 ? 1.07 : 1);
      out.push({ at, d: d * (0.7 + r() * 0.25), f: pitch });
      at += d;
    }
    if (/[,.;:!?]$/.test(w)) at += 0.16;
  });
  return out;
}
interface Line { text: string | ((clock: number) => string); who?: Voice; by?: string }
type TuneId = 'waltz' | 'ballad' | 'twostep';
type Seg =
  | { kind: 'talk'; lines: Line[] }
  | { kind: 'music'; tune: TuneId; len: number }
  | { kind: 'anthem' }
  | { kind: 'carrier'; len: number }
  | { kind: 'dead' };

// ---------- the clock in words, for the time checks ----------
const ONES = ['zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten', 'eleven', 'twelve',
  'thirteen', 'fourteen', 'fifteen', 'sixteen', 'seventeen', 'eighteen', 'nineteen'];
const TENS = ['', '', 'twenty', 'thirty', 'forty', 'fifty'];
export function sayTime(clock: number) {
  const s = ((clock % 86400) + 86400) % 86400;
  const h = Math.floor(s / 3600) % 12 || 12, m = Math.floor(s / 60) % 60;
  if (m === 0) return `${ONES[h]} o'clock`;
  if (m < 10) return `${ONES[h]}-oh-${ONES[m]}`;
  const mm = m < 20 ? ONES[m] : TENS[Math.floor(m / 10)] + (m % 10 ? '-' + ONES[m % 10] : '');
  return `${ONES[h]} ${mm}`;
}
const cap = (s: string) => s[0].toUpperCase() + s.slice(1);

// ---------- what the stations say ----------
const talk = (...lines: Line[]): Seg => ({ kind: 'talk', lines });
const music = (tune: TuneId, len: number): Seg => ({ kind: 'music', tune, len });
const H = (text: Line['text']): Line => ({ text, who: 'host' });

const NIGHT_LINE: Seg[] = [
  talk(H('This is the Night Line on fourteen-twenty, Caliche. We are on till four.'), H((c) => `It's ${sayTime(c)} in Caliche.`)),
  music('ballad', 46),
  talk(H('Cold one out there. Thirty-eight on the bank sign, and the wind has laid down.'), H('Phones are open. If you are up anyway, you might as well talk to somebody.')),
  music('twostep', 42),
  talk({ text: 'Earl, it\'s Dolores out on the Hondo road. The lights are back over the mesa. Same as last fall.', who: 'woman', by: 'a caller' },
    H('Dolores, I believe you. Now turn the porch light off and go to bed.')),
  music('waltz', 38),
  talk(H('This one goes out to the night crew up at the observatory. Somebody has to listen to the sky.')),
  music('ballad', 46),
  talk({ text: 'Anybody else lose the TV a little after two? Just a hum, most of a minute. Then it came back.', who: 'man', by: 'a caller' },
    H('We lost the transmitter about then. Needle went to nothing for most of a minute. Ray, if you are listening, that one is yours.')),
  music('twostep', 42),
  talk(H('Jack Nicklaus, forty-six years old, took the Masters this afternoon. Thirty on the back nine. My uncle says he called it. My uncle calls everything.'),
    H((c) => `${cap(sayTime(c))}. Night Line, fourteen-twenty.`)),
  music('waltz', 38),
  talk(H('Halley\'s comet is low in the south this week. Get out past the last streetlight and give your eyes ten minutes.')),
  music('ballad', 46),
  talk({ text: 'My dog won\'t come out from under the trailer. He\'s been under there since one.', who: 'man', by: 'a caller' }, H('Smart dog.')),
  music('twostep', 42),
];
const SIGN_OFF_SEQ: Seg[] = [
  talk(H('That\'s the Night Line. Thank you for staying up with us.'), H('This is fourteen-twenty, Caliche, New Mexico, signing off until six. Good night.')),
  { kind: 'anthem' },
  { kind: 'carrier', len: 75 },
];
const R = (text: Line['text']): Line => ({ text, who: 'roswell' });
const MORNING: Seg[] = [
  talk(R('Cattle at the sale barn steady, a dollar higher on calves.'), R((c) => `${cap(sayTime(c))} in Roswell.`)),
  music('twostep', 40),
  talk(R('Clear today, high near seventy-four in Roswell, light winds out of the southwest.')),
  music('ballad', 44),
  talk(R('Jack Nicklaus won the Masters yesterday. Forty-six years old, thirty on the back nine. Nobody saw it coming.')),
  music('waltz', 36),
  talk(R('Road crews on the highway north of town all week, so give yourself a few extra minutes.'), R('If you are out on the county roads this early, watch the low spots. There\'s deer.')),
  music('twostep', 40),
  talk(R('And if you are up at this hour, Halley\'s comet is low in the south around midnight this week.'), R((c) => `${cap(sayTime(c))}.`)),
  music('ballad', 44),
];
const SIGN_ON_SEQ: Seg[] = [
  { kind: 'carrier', len: 6 },
  talk(R('Good morning. This is twelve-thirty, Roswell, beginning our broadcast day.')),
  { kind: 'anthem' },
];
const FAR: Seg[] = [music('waltz', 76)];

/** How long a line takes to say. */
export const secsOf = (text: string) => THREE.MathUtils.clamp(1.1 + text.split(/\s+/).length * 0.32, 2.4, 7.5);
const GAP = 0.7;
const lineText = (l: Line, clock: number) => (typeof l.text === 'function' ? l.text(clock) : l.text);
/** The length of a segment; a talk's is fixed when it starts, with the clock then. */
function segLen(s: Seg, clock: number) {
  if (s.kind === 'talk') return s.lines.reduce((a, l) => a + secsOf(lineText(l, clock)) + GAP, 0.4);
  if (s.kind === 'music' || s.kind === 'carrier') return s.len;
  if (s.kind === 'anthem') return ANTHEM_SECS;
  return Infinity;
}

// ---------- the tunes, made once ----------
const SR = 22050;
const hz = (n: number) => 440 * Math.pow(2, (n - 69) / 12);
/** A plucked string (Karplus-Strong): a burst of noise in a delay line that loses its top. */
function pluck(d: Float32Array, f: number, t0: number, dur: number, amp: number, soft = 0.5) {
  const N = Math.max(2, Math.round(SR / f)), buf = new Float32Array(N);
  let last = 0;
  for (let i = 0; i < N; i++) { const w = Math.random() * 2 - 1; last = last * soft + w * (1 - soft); buf[i] = last * 2; }
  const i0 = Math.floor(t0 * SR), n = Math.floor(dur * SR);
  let k = 0;
  for (let i = 0; i < n && i0 + i < d.length; i++) {
    const a = buf[k], b = buf[(k + 1) % N];
    buf[k] = (a + b) * 0.4985;
    k = (k + 1) % N;
    d[i0 + i] += amp * a * Math.min(1, (n - i) / (SR * 0.03));
  }
}
/** A steel guitar note: slides up into the pitch, a slow vibrato, a long fade. */
function steel(d: Float32Array, f: number, t0: number, dur: number, amp: number) {
  const i0 = Math.floor(t0 * SR), n = Math.floor(dur * SR);
  let ph = 0;
  for (let i = 0; i < n && i0 + i < d.length; i++) {
    const t = i / SR;
    const slide = 1 - 0.055 * Math.exp(-t * 18);
    const vib = 1 + 0.006 * Math.sin(2 * Math.PI * 5.2 * t) * Math.min(1, t * 2);
    ph += 2 * Math.PI * f * slide * vib / SR;
    const env = Math.min(1, t / 0.04) * Math.exp(-t * 1.1) * Math.min(1, (n - i) / (SR * 0.08));
    d[i0 + i] += amp * env * (Math.sin(ph) + 0.35 * Math.sin(2 * ph) + 0.12 * Math.sin(3 * ph));
  }
}
type Bar = { root: number; chord: number[]; mel: (number | 0)[] };
const G = [55, 59, 62], C = [60, 64, 67], D = [62, 66, 69], G7 = [55, 59, 65], Em = [52, 55, 59];
function makeTune(id: TuneId): Float32Array {
  // a few bars in G, the chords on the off-beats, a bass under them, a tune over them
  const plan: { bpm: number; beats: number; bars: Bar[] } = id === 'waltz' ? {
    bpm: 100, beats: 3, bars: [
      { root: 43, chord: G, mel: [71, 74, 71] }, { root: 48, chord: C, mel: [72, 71, 69] }, { root: 50, chord: D, mel: [69, 66, 62] }, { root: 43, chord: G, mel: [67, 0, 0] },
      { root: 43, chord: G, mel: [74, 76, 74] }, { root: 48, chord: C, mel: [72, 0, 76] }, { root: 50, chord: D, mel: [74, 72, 69] }, { root: 43, chord: G, mel: [67, 0, 0] },
    ],
  } : id === 'ballad' ? {
    bpm: 74, beats: 4, bars: [
      { root: 43, chord: G, mel: [67, 0, 71, 74] }, { root: 43, chord: G7, mel: [77, 0, 74, 0] }, { root: 48, chord: C, mel: [76, 0, 72, 0] }, { root: 43, chord: G, mel: [71, 0, 0, 0] },
      { root: 40, chord: Em, mel: [71, 0, 67, 71] }, { root: 48, chord: C, mel: [72, 0, 76, 0] }, { root: 50, chord: D, mel: [74, 0, 69, 0] }, { root: 43, chord: G, mel: [67, 0, 0, 0] },
    ],
  } : {
    bpm: 116, beats: 4, bars: [
      { root: 43, chord: G, mel: [67, 71, 74, 71] }, { root: 48, chord: C, mel: [72, 0, 69, 0] }, { root: 43, chord: G, mel: [71, 67, 71, 74] }, { root: 50, chord: D, mel: [69, 0, 0, 0] },
      { root: 43, chord: G, mel: [74, 76, 74, 71] }, { root: 48, chord: C, mel: [72, 0, 76, 0] }, { root: 50, chord: D, mel: [74, 72, 69, 66] }, { root: 43, chord: G, mel: [67, 0, 0, 0] },
    ],
  };
  const beat = 60 / plan.bpm, len = plan.bars.length * plan.beats * beat;
  const d = new Float32Array(Math.ceil((len + 0.2) * SR));
  plan.bars.forEach((b, i) => {
    const t0 = i * plan.beats * beat;
    for (let k = 0; k < plan.beats; k++) {
      const t = t0 + k * beat;
      if (k === 0 || (plan.beats === 4 && k === 2)) pluck(d, hz(b.root + (k === 2 ? 7 : 0)), t, beat * 1.6, 0.36, 0.3);   // the bass: root, then the fifth
      else for (const [j, n] of b.chord.entries()) pluck(d, hz(n), t + j * 0.012, beat * 0.9, 0.11, 0.6);             // the strum
      const m = b.mel[k];
      if (m) {
        let hold = 1; while (k + hold < plan.beats && !b.mel[k + hold]) hold++;
        steel(d, hz(m), t, beat * (hold + 0.6), 0.17);
      }
    }
  });
  // even it out: the loudest point at 0.6
  let peak = 0; for (const v of d) peak = Math.max(peak, Math.abs(v));
  if (peak > 0) for (let i = 0; i < d.length; i++) d[i] *= 0.6 / peak;
  return d.subarray(0, Math.floor(len * SR));
}
const tuneBufs = new Map<TuneId, AudioBuffer>();
function tuneBuffer(ctx: BaseAudioContext, id: TuneId) {
  let b = tuneBufs.get(id);
  if (!b) { const d = makeTune(id); b = ctx.createBuffer(1, d.length, SR); b.getChannelData(0).set(d); tuneBufs.set(id, b); }
  return b;
}

// ---------- a station ----------
interface StationCfg {
  /** The programme it loops while on the air. */
  loop: Seg[];
  /** A fixed point on the clock: a sequence that runs from then, and what comes after it. */
  fixed?: { at: number; seq: Seg[]; then: 'dead' | 'loop' };
  /** Off the air before its fixed point (Roswell before five). */
  offBefore?: boolean;
}
const CFG: Record<StationId, StationCfg> = {
  caliche: { loop: NIGHT_LINE, fixed: { at: SIGN_OFF, seq: SIGN_OFF_SEQ, then: 'dead' } },
  roswell: { loop: MORNING, fixed: { at: SIGN_ON, seq: SIGN_ON_SEQ, then: 'loop' }, offBefore: true },
  far: { loop: FAR },
};

export class Station {
  /** What is on now, and how far into it (seconds). */
  queue: Seg[] = [{ kind: 'dead' }];
  t = 0;
  private len = Infinity;
  private loopAt = 0;
  private after: 'dead' | 'loop' = 'loop';
  private prev: number | null = null;
  private said = -1;
  /** The clock when the segment began (a talk's time checks say that time). */
  private sc = 0;
  /** The bus every radio tuned here listens to (made once the sound is up). */
  bus: GainNode | null = null;
  private n: { voice: GainNode; horn: OscillatorNode; mute: BiquadFilterNode; car: GainNode; mus: GainNode; music: AudioBufferSourceNode | null; anthem: { el: HTMLAudioElement; src: MediaElementAudioSourceNode; g: GainNode } | null; stop: () => void } | null = null;
  /** How many radios are listening (the sound only runs while some are). */
  listeners = 0;
  /** Whether the anthem file could not be played (the carrier stands in). */
  anthemFailed = !streamOn;
  /** Each line as it starts: (text, who said it). */
  onLine?: (text: string, by: string | undefined) => void;

  constructor(readonly id: StationId, private radio: Radio) {}
  private get cfg() { return CFG[this.id]; }
  get seg() { return this.queue[0]; }
  get onAir() { return this.seg.kind !== 'dead'; }
  /** How strong it comes in, 0..1 (1420 is in town; the far one comes and goes). */
  strength(clock: number, time: number) {
    if (!this.onAir) return 0;
    if (this.id === 'caliche') return 1;
    if (this.id === 'roswell') return 0.8;
    const day = 1 - THREE.MathUtils.smoothstep(((clock % 86400) + 86400) % 86400, SIGN_ON + 300, SIGN_ON + 1800);
    const fade = 0.5 + 0.3 * Math.sin(time * 0.37) + 0.15 * Math.sin(time * 1.13 + 1);
    return THREE.MathUtils.clamp(fade, 0.08, 0.9) * day;
  }

  /** Back to the start of a night at this clock (a new night, a save opened). */
  reset(clock: number) { this.prev = null; this.fixed(clock); }

  private start(segs: Seg[], clock: number, skip = 0) {
    this.queue = segs.slice();
    this.t = 0; this.said = -1; this.sc = clock;
    this.len = segLen(this.seg, clock);
    this.forward(skip, clock, true);
    this.sync();
  }
  private refill() {
    const L = this.cfg.loop;
    if (this.after === 'dead') return [{ kind: 'dead' } as Seg];
    const s = L[this.loopAt % L.length]; this.loopAt++;
    return [s];
  }
  /** The clock's fixed points: a new night, or crossing the sign-off or sign-on. */
  private fixed(clock: number) {
    const f = this.cfg.fixed, prev = this.prev;
    this.prev = clock;
    if (prev !== null && clock >= prev - 120) {
      if (f && prev < f.at && clock >= f.at) this.cross(clock - f.at, clock);
      return;
    }
    // a new night or a jump back: where the station is at this clock
    this.loopAt = Math.floor(Math.random() * this.cfg.loop.length);
    if (!f || clock < f.at) {
      this.after = 'loop';
      if (f && this.cfg.offBefore) this.start([{ kind: 'dead' }], clock);
      else this.start(this.refill(), clock, Math.random() * 20);
    } else this.cross(clock - f.at, clock);
  }
  private cross(since: number, clock: number) {
    const f = this.cfg.fixed!;
    this.after = f.then;
    const total = f.seq.reduce((a, s) => a + segLen(s, clock), 0);
    if (since > total + 30) this.start(this.refill(), clock, f.then === 'loop' ? Math.random() * 20 : 0);
    else this.start(f.seq, clock, since);
  }
  /** On by `dt` seconds of programme. quiet: no lines said (skipping ahead). */
  private forward(dt: number, clock: number, quiet = false) {
    this.t += dt;
    let changed = false;
    while (this.t >= this.len) {
      this.t -= this.len;
      this.queue.shift();
      if (!this.queue.length) this.queue.push(...this.refill());
      this.len = segLen(this.seg, clock);
      this.said = -1; this.sc = clock;
      changed = true;
    }
    // the lines of a talk, each once as it starts
    const s = this.seg;
    if (s.kind === 'talk') {
      let at = 0.2, i = 0;
      for (; i < s.lines.length; i++) { const e = at + secsOf(lineText(s.lines[i], this.sc)); if (this.t < e) break; at = e + GAP; }
      if (i < s.lines.length && this.t >= at && i > this.said) {
        this.said = i;
        if (!quiet) this.onLine?.(lineText(s.lines[i], this.sc), s.lines[i].by);
      }
    }
    return changed;
  }

  update(dt: number, clock: number) {
    this.fixed(clock);
    if (this.forward(dt, clock)) this.sync();
  }

  // ---------- the sound ----------
  /** Make the bus (once), so radios can be tuned here before anything plays. */
  ensureBus() {
    const ctx = this.radio.audio.ctx;
    if (!ctx || this.bus) return this.bus;
    this.bus = ctx.createGain();
    return this.bus;
  }
  listen(on: boolean) {
    this.listeners += on ? 1 : -1;
    if (this.listeners > 0 && !this.n) this.wake();
    else if (this.listeners <= 0 && this.n) this.sleep();
  }
  private wake() {
    const ctx = this.radio.audio.ctx, bus = this.ensureBus();
    if (!ctx || !bus) return;
    // a voice: a buzzing horn behind a mute that opens and closes on each syllable ("wah"), with
    // a little vibrato
    const horn = ctx.createOscillator(); horn.type = 'sawtooth'; horn.frequency.value = 120;
    const vib = ctx.createOscillator(); vib.frequency.value = 5.3;
    const vd = ctx.createGain(); vd.gain.value = 2.2; vib.connect(vd); vd.connect(horn.frequency);
    const mute = ctx.createBiquadFilter(); mute.type = 'lowpass'; mute.frequency.value = 400; mute.Q.value = 6.5;
    const voice = ctx.createGain(); voice.gain.value = 0;
    horn.connect(mute); mute.connect(voice); voice.connect(bus);
    // the bare carrier, as the game's other radios have it
    const osc = ctx.createOscillator(); osc.frequency.value = 760;
    const car = ctx.createGain(); car.gain.value = 0; osc.connect(car); car.connect(bus);
    const mus = ctx.createGain(); mus.gain.value = 1; mus.connect(bus);
    for (const x of [horn, vib, osc]) x.start();
    this.n = { voice, horn, mute, car, mus, music: null, anthem: null, stop: () => { for (const x of [horn, vib, osc]) { try { x.stop(); } catch { /* stopped */ } } voice.disconnect(); car.disconnect(); mus.disconnect(); } };
    this.sync();
  }
  private sleep() {
    const n = this.n; if (!n) return;
    this.n = null;
    try { n.music?.stop(); } catch { /* stopped */ }
    this.stopAnthem(n);
    n.stop();
  }
  /** Set the sound to what is on now (a new segment, or a radio just tuned in). */
  private sync() {
    const n = this.n, ctx = this.radio.audio.ctx;
    if (!n || !ctx) return;
    const now = ctx.currentTime, s = this.seg, t = this.t;
    // the tune
    if (n.music) { try { n.music.stop(); } catch { /* stopped */ } n.music.disconnect(); n.music = null; }
    if (s.kind === 'music') {
      const b = tuneBuffer(ctx, s.tune);
      const src = ctx.createBufferSource(); src.buffer = b; src.loop = true;
      src.connect(n.mus); src.start(now, t % b.duration);
      n.music = src;
    }
    // the voices of a talk, each line from where it is: every syllable a "wah" of the horn
    const fq = n.horn.frequency, mf = n.mute.frequency, g = n.voice.gain;
    for (const p of [fq, mf, g]) p.cancelScheduledValues(now);
    g.setValueAtTime(0, now);
    if (s.kind === 'talk') {
      let at = 0.2;
      for (const l of s.lines) {
        const text = lineText(l, this.sc), secs = secsOf(text), base = PITCH[l.who ?? 'host'];
        if (at + secs > t) for (const y of syllables(text, secs)) {
          if (at + y.at < t) continue;
          const ts = now + at + y.at - t, f = base * y.f;
          fq.setValueAtTime(f, ts); fq.linearRampToValueAtTime(f * 0.94, ts + y.d);
          mf.setValueAtTime(330, ts); mf.linearRampToValueAtTime(1350, ts + y.d * 0.3); mf.linearRampToValueAtTime(430, ts + y.d);
          g.setValueAtTime(0, ts); g.linearRampToValueAtTime(0.42, ts + 0.025); g.setValueAtTime(0.42, ts + y.d * 0.7); g.linearRampToValueAtTime(0, ts + y.d);
        }
        at += secs + GAP;
      }
    }
    // the anthem, or the carrier
    if (s.kind === 'anthem') this.playAnthem(n); else this.stopAnthem(n);
    const carrier = s.kind === 'carrier' || (s.kind === 'anthem' && this.anthemFailed);
    n.car.gain.setTargetAtTime(carrier ? 0.03 : 0, now, 0.05);   // quiet: a dead carrier mostly hushes the static
  }
  private playAnthem(n: NonNullable<Station['n']>) {
    if (n.anthem || this.anthemFailed) return;
    const ctx = this.radio.audio.ctx!;
    const el = new Audio();
    el.preload = 'auto';
    el.src = new URL('radio/anthem.mp3', document.baseURI).href;
    let src: MediaElementAudioSourceNode;
    try { src = ctx.createMediaElementSource(el); } catch { this.anthemFailed = true; return; }
    const g = ctx.createGain(); g.gain.value = 0.9;
    src.connect(g); g.connect(n.mus);
    n.anthem = { el, src, g };
    const fail = () => { if (n.anthem?.el !== el) return; this.anthemFailed = true; this.stopAnthem(n); this.sync(); };
    el.addEventListener('error', fail);
    // tuned in partway: start where the station is by the time the file is there
    el.addEventListener('loadedmetadata', () => { const at = this.seg.kind === 'anthem' ? this.t : 0; if (at > 0.5 && at < el.duration) el.currentTime = at; });
    el.play().catch(fail);
  }
  private stopAnthem(n: NonNullable<Station['n']>) {
    const a = n.anthem; if (!a) return;
    n.anthem = null;
    a.el.pause(); a.src.disconnect(); a.g.disconnect();
    a.el.removeAttribute('src'); a.el.load();
  }
  /** For the tests: the anthem's element, while it plays. */
  get anthemEl() { return this.n?.anthem?.el ?? null; }
  get awake() { return !!this.n; }
}

// ---------- a radio ----------
export interface ReceiverOpts { vol: number; ref?: number }
export class Receiver {
  on = false;
  station: StationId = 'caliche';
  /** Where it plays from (the truck's dash, the diner's counter), or null: not heard. */
  pos: THREE.Vector3 | null = null;
  /** The power it gets, 0..1 (the truck's trouble: it drops out while the starter turns). */
  power = 1;
  /** Seconds of tuning noise left after the dial is turned. */
  private sweep = 0;
  /** Interference (Tom, 16 October: static now and then, at random, with the wah wah): every
   *  5 to 20 seconds the static swells over the station for half a second to three, crackling,
   *  and the station sinks under it, then comes back. */
  private fuzz = { next: 12 + Math.random() * 20, left: 0, len: 1, depth: 0 };
  /** How far under the static the station is now, 0..1 (the tests). */
  interference = 0;
  private n: { sig: GainNode; stat: GainNode; out: GainNode; pan: PannerNode; bus: GainNode | null; stop: () => void } | null = null;
  private listening: StationId | null = null;

  constructor(private radio: Radio, private o: ReceiverOpts) {}

  set(on: boolean, station: StationId = this.station) {
    if (station !== this.station) { this.station = station; this.sweep = 0.5; }
    if (on && !this.on) this.sweep = Math.max(this.sweep, 0.25);
    this.on = on;
  }
  /** The next station on the dial, and after the last, off. Returns the station or null. */
  next(): StationId | null {
    if (!this.on) { this.set(true, this.station); return this.station; }
    const i = DIAL.indexOf(this.station);
    if (i === DIAL.length - 1) { this.set(false, DIAL[0]); return null; }
    this.set(true, DIAL[i + 1]);
    return this.station;
  }
  /** Playing and somewhere it can be heard. */
  get live() { return this.on && !!this.pos; }

  private build() {
    const a = this.radio.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise) return null;
    // the station and the static, through a small speaker: a narrow band, a little overdriven
    const sig = ctx.createGain(); sig.gain.value = 0;
    const n = ctx.createBufferSource(); n.buffer = noise; n.loop = true;
    const stat = ctx.createGain(); stat.gain.value = 0; n.connect(stat);
    const mix = ctx.createGain(); sig.connect(mix); stat.connect(mix);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1250; bp.Q.value = 0.7;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3400;
    const sh = ctx.createWaveShaper();
    const curve = new Float32Array(256); for (let i = 0; i < 256; i++) { const x = i / 127.5 - 1; curve[i] = Math.tanh(x * 1.6) / Math.tanh(1.6); }
    sh.curve = curve;
    const out = ctx.createGain(); out.gain.value = 0;
    mix.connect(bp); bp.connect(lp); lp.connect(sh); sh.connect(out);
    const pan = a.spatial(new THREE.Vector3(), this.o.ref ?? 1.2, a.radioBus) as PannerNode;
    out.connect(pan);
    n.start();
    return { sig, stat, out, pan, bus: null, stop: () => { try { n.stop(); } catch { /* stopped */ } out.disconnect(); } };
  }

  update(dt: number) {
    const ctx = this.radio.audio.ctx;
    if (!ctx) return;
    if (!this.n) { if (!this.on) return; this.n = this.build(); if (!this.n) return; }
    const n = this.n, now = ctx.currentTime;
    // listen to the station it is tuned to, while it plays
    const want = this.live ? this.station : null;
    if (want !== this.listening) {
      if (this.listening) { this.radio.stations[this.listening].listen(false); if (n.bus) { try { n.bus.disconnect(n.sig); } catch { /* not connected */ } } n.bus = null; }
      this.listening = want;
      if (want) {
        const st = this.radio.stations[want];
        const bus = st.ensureBus();
        if (bus) { bus.connect(n.sig); n.bus = bus; }
        st.listen(true);
      }
    }
    this.sweep = Math.max(0, this.sweep - dt);
    const st = this.radio.stations[this.station];
    const s = this.live ? st.strength(this.radio.clock, this.radio.time) : 0;
    const carrier = st.seg.kind === 'carrier' || st.seg.kind === 'anthem';
    const tuning = this.sweep > 0 ? 1 : 0;
    // now and then the static comes up over it, crackling, and goes again. Tom, 16 October:
    // the sound dropped out all the time (it came every 5 to 20 s and took up to four fifths of
    // the station for up to 3 s); now every 20 to 60 s, shorter, and the station stays under it
    const z = this.fuzz;
    if (this.live && s > 0) {
      if (z.left > 0) z.left = Math.max(0, z.left - dt);
      else if ((z.next -= dt) <= 0) { z.len = z.left = 0.4 + Math.random() * 1.2; z.depth = 0.3 + Math.random() * 0.4; z.next = 20 + Math.random() * 40; }
    } else z.left = 0;
    const k = z.left > 0 ? z.depth * Math.sqrt(Math.sin(Math.PI * (1 - z.left / z.len))) : 0;
    this.interference = k;
    const crackle = k > 0 ? 0.5 + Math.random() * 1.1 : 1;
    n.sig.gain.setTargetAtTime(this.live ? s * (1 - tuning) * (1 - 0.5 * k) * this.power : 0, now, 0.05);
    n.stat.gain.setTargetAtTime(this.live ? (0.03 + (carrier ? 0.02 : 0.36 * (1 - s)) + tuning * 0.45 + 0.42 * k * crackle) * this.power : 0, now, k > 0 ? 0.015 : 0.04);
    n.out.gain.setTargetAtTime(this.live ? this.o.vol : 0, now, this.live ? 0.08 : 0.15);
    if (this.pos && Number.isFinite(this.pos.x + this.pos.y + this.pos.z)) {
      const p = n.pan as PannerNode & { positionX?: AudioParam };
      if (p.positionX) { p.positionX.value = this.pos.x; p.positionY.value = this.pos.y; p.positionZ.value = this.pos.z; }
      else (p as unknown as { setPosition: (x: number, y: number, z: number) => void }).setPosition(this.pos.x, this.pos.y, this.pos.z);
    }
  }
  /** Off, and let the station go (a new night). */
  reset() {
    this.on = false; this.sweep = 0;
    if (this.listening) { this.radio.stations[this.listening].listen(false); this.listening = null; }
    if (this.n) { if (this.n.bus) { try { this.n.bus.disconnect(this.n.sig); } catch { /* not connected */ } } this.n.stop(); this.n = null; }
  }
}

// ---------- the airwaves ----------
export class Radio {
  readonly stations: Record<StationId, Station>;
  readonly receivers: Receiver[] = [];
  /** The night's clock as of the last update. */
  clock = 0;
  /** Seconds the radio has run (the far station's fading). */
  time = 0;
  /** A line said on a station: (station, text, who said it). */
  onLine?: (station: StationId, text: string, by: string | undefined) => void;

  constructor(readonly audio: AudioSys) {
    this.stations = { caliche: new Station('caliche', this), roswell: new Station('roswell', this), far: new Station('far', this) };
    for (const id of DIAL) this.stations[id].onLine = (text, by) => this.onLine?.(id, text, by);
  }
  receiver(o: ReceiverOpts) { const r = new Receiver(this, o); this.receivers.push(r); return r; }
  /** A new night, or a save opened at this clock: every radio off, every station where it is. */
  reset(clock: number) {
    this.clock = clock;
    for (const r of this.receivers) r.reset();
    for (const id of DIAL) this.stations[id].reset(clock);
  }
  update(dt: number, clock: number) {
    this.clock = clock; this.time += dt;
    for (const id of DIAL) this.stations[id].update(dt, clock);
    for (const r of this.receivers) r.update(dt);
  }
  /** What each station has on now (the tests, the developer menu). */
  state() {
    return Object.fromEntries(DIAL.map((id) => {
      const s = this.stations[id];
      return [id, { seg: s.seg.kind, t: s.t, awake: s.awake, listeners: s.listeners, strength: s.strength(this.clock, this.time),
        anthem: s.anthemEl ? { time: s.anthemEl.currentTime, paused: s.anthemEl.paused, failed: s.anthemFailed } : null, anthemFailed: s.anthemFailed }];
    }));
  }
}

/** Where a frequency is on the truck's AM scale (dash canvas pixels, truckTextures.ts). */
export function dialX(freq: number) {
  const P: [number, number][] = [[540, 88], [600, 125], [700, 162], [800, 199], [1000, 236], [1200, 273], [1600, 310]];
  for (let i = 1; i < P.length; i++) if (freq <= P[i][0]) { const [f0, x0] = P[i - 1], [f1, x1] = P[i]; return x0 + (x1 - x0) * (freq - f0) / (f1 - f0); }
  return P[P.length - 1][1];
}
