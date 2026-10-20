import * as THREE from 'three';
import type { AudioSys } from './Audio';
import { RadioVoice, type RadioClip, type CallerVoice } from './radioVoice';

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
// clock, which runs faster on a drive. A station makes sound only while a radio listens. On a
// drive, the trip's own lines (story/tripRadio.ts) are said in between, in the station's voices
// (Station.slot).
//
// The optional recorded host replaces only Night Line's who='host' lines. The other voices
// are a muted horn that talks, "wah wah", like the grown-ups in
// Peanuts (Tom, 16 October: a little cartoonish, but it works in the surreal night); what they
// say is in the caption. The tunes are Kevin MacLeod's (incompetech.com, CC BY 4.0,
// THIRD_PARTY_NOTICES.md), streamed from public/radio/: a part of a song each time, a different
// part the next (Tom, 18 October: the few bars made here, round and round, were too irritating).
// The single-file build has no files beside it; there the bars made here stand in. The anthem is the United States Air Force Band's choral
// recording (public domain, THIRD_PARTY_NOTICES.md), streamed from public/radio/ when a
// station plays it; the single-file build has no files beside it, so there it is the carrier.

export type StationId = 'caliche' | 'roswell' | 'far';
/** The order the truck's radio steps through (R, or Radio on a phone). */
export const DIAL: StationId[] = ['caliche', 'roswell', 'far'];
export const FREQ: Record<StationId, number> = { caliche: 1420, roswell: 1230, far: 1090 };
export const SIGN_OFF = 4 * 3600;   // the Night Line signs off
export const SIGN_ON = 5 * 3600;    // Roswell signs on
export const ANTHEM_SECS = 74.8;
/** The recorded bulletin on the frequency (HENDELSER.md, event 4). */
export const ARCHIVE_SECS = 47;
export const streamOn = import.meta.env.MODE !== 'single';

export type Voice = 'host' | 'woman' | 'man' | 'roswell' | 'boy' | 'news' | 'old';
export const PITCH: Record<Voice, number> = { host: 116, woman: 212, man: 138, roswell: 126, boy: 262, news: 150, old: 104 };
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
/** A line said on a station: the words (or the words at this clock, for the time checks), the
 *  voice, and who said it for the caption (none: the station's own host). */
export interface Line { text: string | ((clock: number) => string); who?: Voice; by?: string }
type TuneId = 'waltz' | 'ballad' | 'twostep' | 'far';
/** The recordings (public/radio/), mono and narrow like AM, and how long each is (s). */
const TRACK: Record<TuneId, { file: string; dur: number }> = {
  ballad: { file: 'drankin_song', dur: 246.4 },
  twostep: { file: 'bama_country', dur: 211.8 },
  waltz: { file: 'cattails', dur: 159.0 },
  far: { file: 'anamalie', dur: 208.2 },
};
type Seg =
  | { kind: 'talk'; lines: Line[]; slotted?: boolean; duration?: number }
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
  music('ballad', 78),
  talk(H('Cold one out there. Thirty-eight on the bank sign, and the wind has laid down.'), H('Phones are open. If you are up anyway, you might as well talk to somebody.')),
  music('twostep', 70),
  talk({ text: 'Earl, it\'s Dolores out on the Hondo road. The lights are back over the mesa. Same as last fall.', who: 'woman', by: 'a caller' },
    H('Dolores, I believe you. Now turn the porch light off and go to bed.')),
  music('waltz', 64),
  talk(H('This one goes out to the night crew up at the observatory. Somebody has to listen to the sky.')),
  music('ballad', 78),
  talk({ text: 'Anybody else lose the TV a little after two? Just a hum, most of a minute. Then it came back.', who: 'man', by: 'a caller' },
    H('We lost the transmitter about then. Needle went to nothing for most of a minute. Ray, if you are listening, that one is yours.')),
  music('twostep', 70),
  talk(H('Jack Nicklaus, forty-six years old, took the Masters this afternoon. Thirty on the back nine. My uncle says he called it. My uncle calls everything.'),
    H((c) => `${cap(sayTime(c))}. Night Line, fourteen-twenty.`)),
  music('waltz', 64),
  talk(H('Halley\'s comet is low in the south this week. Get out past the last streetlight and give your eyes ten minutes.')),
  music('ballad', 78),
  talk({ text: 'My dog won\'t come out from under the trailer. He\'s been under there since one.', who: 'man', by: 'a caller' }, H('Smart dog.')),
  music('twostep', 70),
];
// HENDELSER.md, event 9: once in a night, from 02:26 on, a boy calls the Night Line about a star
// with a tail low in the south and a light over the mesa, from his father's ranch, in July. The
// comet in the 1947 photograph. Afterwards the host says the line was dead when he picked it up.
export const BOY = 'a boy';
const BOY_CALL: Seg[] = [
  talk({ text: 'Hello? Is this the radio? There\'s a star with a tail on it, real low in the south. A little tail.', who: 'boy', by: BOY },
    H('That\'s Halley\'s comet, son. It\'s there all week. You\'re up late.'),
    { text: 'I\'m calling from my dad\'s place, out on the ranch. There\'s a light over the mesa too. It isn\'t moving.', who: 'boy', by: BOY },
    H('Is your dad up?'),
    { text: 'He\'s out with the men. It\'s hot out. It\'s July.', who: 'boy', by: BOY }),
  music('waltz', 24),
  talk(H('Folks, I want to say something. That line was dead when I picked it up. No dial tone, nothing at all.'),
    H('I don\'t know who I was talking to.')),
];
export const BOY_AT = 2 * 3600 + 26 * 60;
/** The boy's call as it went out tonight, line by line, with how long each took on the air: the
 *  recorded voices' own lengths where the station had them, else the time the mumble took, and
 *  the station's gap after each (story/lostNights.ts: Earl's reel from 1981 has the same words, and
 *  nothing at all where the host answered him tonight). */
export function boyCallTimes(radio: Radio): { text: string; boy: boolean; secs: number; gap: number }[] {
  const call = BOY_CALL[0] as Extract<Seg, { kind: 'talk' }>;
  return call.lines.map((line) => {
    const text = lineText(line, 0), boy = line.who === 'boy';
    const clips = boy ? (radio.callerVoice ? radio.callerBank.resolve(text, 0, 'boy') : null) : radio.hostVoice ? radio.hostBank.resolve(text, 0) : null;
    return { text, boy, secs: clips ? clips.reduce((sum, clip) => sum + clip.duration, 0) + (clips.length - 1) * FRAGMENT_GAP : secsOf(text), gap: GAP };
  });
}
const SIGN_OFF_SEQ: Seg[] = [
  talk(H('That\'s the Night Line. Thank you for staying up with us.'), H('This is fourteen-twenty, Caliche, New Mexico, signing off until six. Good night.')),
  { kind: 'anthem' },
  { kind: 'carrier', len: 75 },
];
const R = (text: Line['text']): Line => ({ text, who: 'roswell' });
const MORNING: Seg[] = [
  talk(R('Cattle at the sale barn steady, a dollar higher on calves.'), R((c) => `${cap(sayTime(c))} in Roswell.`)),
  music('twostep', 70),
  talk(R('Clear today, high near seventy-four in Roswell, light winds out of the southwest.')),
  music('ballad', 78),
  talk(R('Jack Nicklaus won the Masters yesterday. Forty-six years old, thirty on the back nine. Nobody saw it coming.')),
  music('waltz', 64),
  talk(R('Road crews on the highway north of town all week, so give yourself a few extra minutes.'), R('If you are out on the county roads this early, watch the low spots. There\'s deer.')),
  music('twostep', 70),
  talk(R('And if you are up at this hour, Halley\'s comet is low in the south around midnight this week.'), R((c) => `${cap(sayTime(c))}.`)),
  music('ballad', 78),
];
const SIGN_ON_SEQ: Seg[] = [
  { kind: 'carrier', len: 6 },
  talk(R('Good morning. This is twelve-thirty, Roswell, beginning our broadcast day.')),
  { kind: 'anthem' },
];
const FAR: Seg[] = [music('far', 96)];

/** How long a line takes to say. */
export const secsOf = (text: string) => THREE.MathUtils.clamp(1.1 + text.split(/\s+/).length * 0.32, 2.4, 7.5);
const GAP = 0.7;
const FRAGMENT_GAP = 0.035;
interface SpokenLine { line: Line; text: string; secs: number; at: number; clips: RadioClip[] | null; bank?: 'host' | 'caller'; buffers?: AudioBuffer[] }
export interface ArchiveProgress {
  id: number; state: 'loading' | 'playing' | 'complete' | 'cancelled'; elapsed: number; duration: number;
}
const CALLER_VOICES = new Set<string>(['woman', 'man', 'old', 'boy', 'news']);
/** Seconds of its own programme a station has after anything else (a trip's lines, the boy, the
 *  bulletin), before it takes the next. */
const CALM = 10;
/** A record plays at least this long before the host comes in over it, and fades in this long. */
const RECORD_MIN = 5, RECORD_FADE = 1.6;
/** No trip's lines in the last minutes before a fixed point (the sign-off at four). */
const BEFORE_FIXED = 180;
/** Lines a trip's programme has put on a station (Station.slot). */
export interface Slotted { readonly kind: 'talk'; readonly lines: Line[]; readonly slotted?: boolean }
const lineText = (l: Line, clock: number) => (typeof l.text === 'function' ? l.text(clock) : l.text);
const talkLen = (lines: Line[], clock: number) => lines.reduce((a, l) => a + secsOf(lineText(l, clock)) + GAP, 0.4);
/** A fixed recording keeps its full duration, including the gaps, in sound and captions. */
const talkScale = (s: Extract<Seg, { kind: 'talk' }>, clock: number) => s.duration ? s.duration / talkLen(s.lines, clock) : 1;
/** The length of a segment; a talk's is fixed when it starts, with the clock then. */
function segLen(s: Seg, clock: number) {
  if (s.kind === 'talk') return s.duration ?? talkLen(s.lines, clock);
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
  const plan: { bpm: number; beats: number; bars: Bar[] } = id === 'waltz' || id === 'far' ? {
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
  private n: { voice: GainNode; horn: OscillatorNode; mute: BiquadFilterNode; car: GainNode; mus: GainNode; music: AudioBufferSourceNode | null; anthem: { el: HTMLAudioElement; src: MediaElementAudioSourceNode; g: GainNode } | null;
    track: { el: HTMLAudioElement; src: MediaElementAudioSourceNode; g: GainNode } | null; stop: () => void } | null = null;
  /** Whether the recordings could not be played (the bars made here stand in). */
  trackFailed = !streamOn;
  /** Counts the segments, so each song is heard from a different place each time. */
  private segNo = 0;
  /** The segment the song could not start in (the bars made here stand in for it). */
  private trackSkip = -1;
  /** How many radios are listening (the sound only runs while some are). */
  listeners = 0;
  /** Whether the anthem file could not be played (the carrier stands in). */
  anthemFailed = !streamOn;
  /** Each line as it starts: (text, who said it). */
  onLine?: (text: string, by: string | undefined, duration?: number) => void;
  /** Seconds the station has been in its own programme (since the last of anything else). */
  private calm = CALM;
  /** The record on now is being faded down for the host (slot). */
  private fading = false;
  /** Event 4 holds the programme at its exact syllable or place in the record. It is not a
   *  queued programme item: a clock jump, sign-off or reset discards the interruption. */
  private interrupted: { queue: Seg[]; t: number; len: number; said: number; sc: number;
    segNo: number; trackSkip: number; fading: boolean; spoken: SpokenLine[];
    hostReady: boolean; hostVoice: boolean; callerVoice: boolean } | null = null;
  private archive: ArchiveProgress | null = null;
  private archiveId = 0;
  private spoken: SpokenLine[] = [];
  private hostSources: AudioBufferSourceNode[] = [];
  private hostPending: AbortController | null = null;
  private hostGeneration = 0;
  private hostReady = false;
  /** Station position is tied to the pausable sound clock while recorded talk is heard. */
  private audioAt: number | null = null;

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
  reset(clock: number) { this.prev = null; this.calm = CALM; this.fixed(clock); }
  /** In its own programme now, with nothing else waiting behind it (not the boy's call, the
   *  bulletin, the sign-off or the sign-on, or a trip's lines), and has been for a little while. */
  get free() { return this.onAir && this.queue.length === 1 && this.cfg.loop.includes(this.seg) && this.calm >= CALM; }
  /** Lines of the station's own, said next (a trip's programme, story/tripRadio.ts): after the
   *  talk that is on, or over the end of the record, faded down, and then the next record (the
   *  talk that was next waits for the next time round). Only while the station is free,
   *  and not in the last minutes before its fixed point, so nothing else is talked over. After
   *  them, the programme goes on where it was. Returns them as they wait on the air (where,
   *  unslot), or null if they were not taken. */
  slot(lines: Line[], clock: number): Slotted | null {
    if (!this.free) return null;
    const f = this.cfg.fixed, day = ((clock % 86400) + 86400) % 86400;
    if (f && this.after === 'loop' && day < f.at && day > f.at - BEFORE_FIXED) return null;
    const s = this.seg;
    if (s.kind === 'music') {
      if (this.t < RECORD_MIN) return null;
      if (this.t + RECORD_FADE < this.len) {
        this.len = this.t + RECORD_FADE; this.fading = true;
        const n = this.n, ctx = this.radio.audio.ctx;
        if (n && ctx) { const g = n.mus.gain, now = ctx.currentTime; g.cancelScheduledValues(now); g.setValueAtTime(g.value, now); g.linearRampToValueAtTime(0, now + RECORD_FADE); }
      }
      const L = this.cfg.loop;
      if (L[this.loopAt % L.length].kind === 'talk') this.loopAt++;
    } else if (s.kind !== 'talk') return null;
    const seg: Seg & Slotted = { kind: 'talk', lines, slotted: true };
    this.queue.splice(1, 0, seg);
    return seg;
  }
  /** Lines taken by slot(): being said now, still waiting, or gone (the sign-off came first). */
  where(x: Slotted): 'on' | 'waiting' | null {
    const i = this.queue.indexOf(x as Seg);
    return i === 0 ? 'on' : i > 0 ? 'waiting' : null;
  }
  /** Lines taken by slot() and not begun yet, taken out again (the trip is over before them).
   *  Returns whether they were. */
  unslot(x: Slotted) {
    const i = this.queue.indexOf(x as Seg);
    if (i < 1) return false;
    this.queue.splice(i, 1);
    return true;
  }
  /** Event 4: a 47-second recording on the frequency, then the interrupted programme from the
   *  same point. Never replaces the boy, a trip's lines, or a fixed point. Returns its duration,
   *  or 0 when the frequency is not free or too close to its sign-off. */
  cutIn(lines: { text: string; who: Voice; by: string }[], clock: number) {
    if (!this.free || this.interrupted || this.hostPending || !lines.length) return 0;
    const f = this.cfg.fixed, day = ((clock % 86400) + 86400) % 86400;
    if (f && this.after === 'loop' && day >= f.at - BEFORE_FIXED && day < f.at) return 0;
    const previous = { queue: this.queue.slice(), t: this.t, len: this.len, said: this.said, sc: this.sc,
      segNo: this.segNo, trackSkip: this.trackSkip, fading: this.fading,
      spoken: this.spoken, hostReady: this.hostReady, hostVoice: this.radio.hostVoice, callerVoice: this.radio.callerVoice };
    const seg: Seg = { kind: 'talk', lines, duration: ARCHIVE_SECS };
    this.start([seg], clock);
    this.interrupted = previous;
    this.archive = { id: ++this.archiveId, state: this.hostPending ? 'loading' : 'playing', elapsed: 0, duration: ARCHIVE_SECS };
    return ARCHIVE_SECS;
  }

  private lines(s: Seg, clock: number): SpokenLine[] {
    if (s.kind !== 'talk') return [];
    const scale = talkScale(s, clock);
    return s.lines.map((line) => {
      const text = lineText(line, clock);
      const bank = this.id !== 'caliche' ? undefined : !s.duration && this.radio.hostVoice && line.who === 'host' ? 'host'
        : this.radio.callerVoice && line.who && CALLER_VOICES.has(line.who) ? 'caller' : undefined;
      const clips = bank === 'host' ? this.radio.hostBank.resolve(text, clock)
        : bank === 'caller' ? this.radio.callerBank.resolve(text, clock, line.who as CallerVoice) : null;
      return { line, text, clips, bank, at: 0, secs: clips ? clips.reduce((sum, clip) => sum + clip.duration, 0) + (clips.length - 1) * FRAGMENT_GAP : secsOf(text) * scale };
    });
  }
  /** Fixed archives distribute spare time as gaps. Recorded words are never time-stretched. */
  private layout() {
    const s = this.seg;
    let at = 0.2, gap = GAP;
    if (s.kind === 'talk' && s.duration) {
      const recorded = this.spoken.every((line) => !!line.buffers);
      const speech = this.spoken.reduce((sum, line) => sum + line.secs, 0);
      if (recorded && speech + 0.4 <= s.duration) {
        gap = (s.duration - 0.4 - speech) / Math.max(1, this.spoken.length - 1);
      } else {
        // Missing/failed/overlong archives retain every word in the original newsreader.
        const scale = talkScale(s, this.sc);
        at *= scale; gap *= scale;
        for (const line of this.spoken) {
          line.secs = secsOf(line.text) * scale;
          if (recorded) { line.clips = null; line.buffers = undefined; }
        }
      }
    }
    for (const line of this.spoken) { line.at = at; at += line.secs + gap; }
  }
  private lengthOf(s: Seg, clock: number) {
    return s.kind === 'talk' ? s.duration ?? this.lines(s, clock).reduce((sum, line) => sum + line.secs + GAP, 0.4) : segLen(s, clock);
  }
  private plan(clock: number) {
    this.cancelHost();
    this.spoken = this.lines(this.seg, clock);
    // An archive is atomic: an unavailable sentence must not leave a partial recording.
    if (this.seg.kind === 'talk' && this.seg.duration && this.spoken.some((line) => !line.clips)) {
      for (const line of this.spoken) line.clips = null;
    }
    this.layout();
    this.hostReady = !this.spoken.some((line) => line.clips);
    this.len = this.seg.kind === 'talk' ? this.seg.duration ?? this.spoken.reduce((sum, line) => sum + line.secs + GAP, 0.4) : segLen(this.seg, clock);
  }
  /** Keep the same line and relative point when manifest lengths become decoded lengths. */
  private mapPosition(before: number[], after: number[]) {
    let oldAt = 0.2, newAt = 0.2;
    for (let i = 0; i < before.length; i++) {
      if (this.t < oldAt) return newAt - (oldAt - this.t);
      if (this.t < oldAt + before[i]) return newAt + (this.t - oldAt) / before[i] * after[i];
      if (this.t < oldAt + before[i] + GAP) return newAt + after[i] + (this.t - oldAt - before[i]);
      oldAt += before[i] + GAP; newAt += after[i] + GAP;
    }
    return newAt + this.t - oldAt;
  }
  /** Change the voice in place; no stale fetch or future source can survive the option. */
  revoice(kind: 'host' | 'caller' = 'host') {
    if (this.seg.kind !== 'talk') return;
    if (!this.seg.lines.some((line) => kind === 'host' ? line.who === 'host' : !!line.who && CALLER_VOICES.has(line.who))) return;
    // Switching between the natural and procedural archive layouts mid-sentence can seek
    // across words. The 47-second broadcast continues, but that mixed listen is no finding.
    if (kind === 'caller' && this.seg.duration && this.t > 0) this.cancelArchive();
    const before = this.spoken.map((line) => line.secs);
    this.plan(this.sc);
    if (!this.seg.duration) this.t = this.mapPosition(before, this.spoken.map((line) => line.secs));
    this.sync();
  }

  private start(segs: Seg[], clock: number, skip = 0) {
    this.cancelArchive();
    this.interrupted = null;
    this.queue = segs.slice();
    this.t = 0; this.said = -1; this.sc = clock; this.segNo++; this.fading = false;
    this.plan(clock);
    this.forward(skip, clock, true);
    this.sync();
  }
  private refill(clock = this.sc) {
    const L = this.cfg.loop;
    if (this.after === 'dead') return [{ kind: 'dead' } as Seg];
    const day = ((clock % 86400) + 86400) % 86400;
    if (this.id === 'caliche' && !this.radio.boyCalled && day >= BOY_AT && day < SIGN_OFF - 300) { this.radio.boyCalled = true; return BOY_CALL.slice(); }
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
      else this.start(this.refill(clock), clock, Math.random() * 20);
    } else this.cross(clock - f.at, clock);
  }
  private cross(since: number, clock: number) {
    const f = this.cfg.fixed!;
    this.after = f.then;
    const total = f.seq.reduce((a, s) => a + this.lengthOf(s, clock), 0);
    if (since > total + 30) this.start(this.refill(clock), clock, f.then === 'loop' ? Math.random() * 20 : 0);
    else this.start(f.seq, clock, since);
  }
  /** On by `dt` seconds of programme. quiet: no lines said (skipping ahead). */
  private forward(dt: number, clock: number, quiet = false) {
    this.t += dt;
    if (this.interrupted && this.archive && this.archive.state !== 'cancelled') {
      this.archive.elapsed = Math.min(this.len, this.t);
    }
    let changed = false;
    while (this.t >= this.len) {
      this.t -= this.len;
      if (this.interrupted) {
        if (this.archive && this.archive.state !== 'cancelled') { this.archive.state = 'complete'; this.archive.elapsed = ARCHIVE_SECS; }
        const previous = this.interrupted;
        this.interrupted = null;
        this.cancelHost();
        // The held audio has not played during this frame's archive overrun. Resume its
        // exact sample, then sync() rebases the sound clock to the actual restart time.
        this.queue = previous.queue; this.t = previous.t; this.len = previous.len;
        this.said = previous.said; this.sc = previous.sc; this.segNo = previous.segNo;
        this.trackSkip = previous.trackSkip; this.calm = 0; this.fading = previous.fading;
        this.spoken = previous.spoken; this.hostReady = previous.hostReady;
        // The option can change while the bulletin is on. Apply the ordinary voice mapping to
        // the held sentence then; otherwise keep the exact decoded plan and buffer offsets.
        if (this.seg.kind === 'talk' && (
          previous.hostVoice !== this.radio.hostVoice && this.seg.lines.some((line) => line.who === 'host') ||
          previous.callerVoice !== this.radio.callerVoice && this.seg.lines.some((line) => !!line.who && CALLER_VOICES.has(line.who))
        )) {
          const before = this.spoken.map((line) => line.secs);
          this.plan(this.sc);
          this.t = this.mapPosition(before, this.spoken.map((line) => line.secs));
        }
        changed = true;
        continue;
      }
      this.queue.shift();
      if (!this.queue.length) this.queue.push(...this.refill(clock));
      this.said = -1; this.sc = clock; this.segNo++; this.fading = false;
      this.plan(clock);
      changed = true;
    }
    // the lines of a talk, each once as it starts
    const s = this.seg;
    if (s.kind === 'talk' && (quiet || !this.n || this.hostReady)) {
      let i = 0;
      for (; i < this.spoken.length; i++) { const line = this.spoken[i]; if (this.t < line.at + line.secs) break; }
      if (i < s.lines.length && this.t >= this.spoken[i].at && i > this.said) {
        this.said = i;
        const line = this.spoken[i];
        if (!quiet) this.onLine?.(line.text, line.line.by, line.buffers || s.duration ? line.secs : undefined);
      }
    }
    return changed;
  }

  update(dt: number, clock: number) {
    this.fixed(clock);
    const ctx = this.radio.audio.ctx;
    // No programme or caption runs ahead of a recording still being prepared, or of a
    // suspended AudioContext. On a slow rendering frame the recording remains authoritative.
    const advance = this.hostPending ? 0 : this.audioAt !== null && ctx ? Math.max(0, ctx.currentTime - this.audioAt - this.t) : dt;
    // Until the held source is restarted, even a slow boundary frame is archive time.
    const archiveTime = this.interrupted ? advance : 0;
    if (!this.hostPending && this.forward(advance, clock)) this.sync();
    this.calm = this.cfg.loop.includes(this.seg) ? this.calm + advance - archiveTime : 0;
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
    this.n = { voice, horn, mute, car, mus, music: null, anthem: null, track: null, stop: () => { for (const x of [horn, vib, osc]) { try { x.stop(); } catch { /* stopped */ } } voice.disconnect(); car.disconnect(); mus.disconnect(); } };
    this.sync();
    this.announceRecordedLine();
  }
  private sleep() {
    const n = this.n; if (!n) return;
    this.n = null;
    this.cancelArchive();
    this.cancelHost();
    try { n.music?.stop(); } catch { /* stopped */ }
    this.stopAnthem(n);
    this.stopTrack(n);
    n.stop();
  }
  /** Set the sound to what is on now (a new segment, or a radio just tuned in). */
  private sync() {
    const n = this.n, ctx = this.radio.audio.ctx;
    if (!n || !ctx) return;
    const now = ctx.currentTime, s = this.seg, t = this.t;
    this.stopHostSources();
    this.audioAt = null;
    // the music's level: up, or going down for the host (slot)
    const mg = n.mus.gain;
    mg.cancelScheduledValues(now); mg.setValueAtTime(1, now);
    if (this.fading) mg.linearRampToValueAtTime(0, now + Math.max(0.05, this.len - t));
    // the tune
    if (n.music) { try { n.music.stop(); } catch { /* stopped */ } n.music.disconnect(); n.music = null; }
    const bars = this.trackFailed || this.trackSkip === this.segNo;
    if (s.kind === 'music' && !bars) this.playTrack(n, s.tune, s.len);
    else this.stopTrack(n);
    if (s.kind === 'music' && bars) {
      const b = tuneBuffer(ctx, s.tune);
      const src = ctx.createBufferSource(); src.buffer = b; src.loop = true;
      src.connect(n.mus); src.start(now, t % b.duration);
      n.music = src;
    }
    // the voices of a talk, each line from where it is: every syllable a "wah" of the horn
    const fq = n.horn.frequency, mf = n.mute.frequency, g = n.voice.gain;
    for (const p of [fq, mf, g]) p.cancelScheduledValues(now);
    g.setValueAtTime(0, now);
    if (s.kind === 'talk' && !this.hostReady) this.prepareHost();
    if (s.kind === 'talk' && this.hostReady) {
      if (s.duration || this.spoken.some((line) => line.buffers)) this.audioAt = now - t;
      if (this.interrupted && this.archive && this.archive.state !== 'cancelled') this.archive.state = 'playing';
      for (const line of this.spoken) {
        const { text, secs, at } = line, base = PITCH[line.line.who ?? 'host'];
        if (line.buffers) {
          let partAt = at;
          for (const buffer of line.buffers) {
            if (partAt + buffer.duration > t) {
              const source = ctx.createBufferSource(); source.buffer = buffer;
              // The station bus then passes through each receiver's existing AM filter,
              // tuning, interference, position and AudioSys.radioBus volume control.
              source.connect(this.bus!);
              source.onended = () => { source.disconnect(); const i = this.hostSources.indexOf(source); if (i >= 0) this.hostSources.splice(i, 1); };
              this.hostSources.push(source);
              source.start(now + Math.max(0, partAt - t), Math.max(0, t - partAt));
            }
            partAt += buffer.duration + FRAGMENT_GAP;
          }
        } else if (at + secs > t) for (const y of syllables(text, secs)) {
          if (at + y.at < t) continue;
          const ts = now + at + y.at - t, f = base * y.f;
          fq.setValueAtTime(f, ts); fq.linearRampToValueAtTime(f * 0.94, ts + y.d);
          mf.setValueAtTime(330, ts); mf.linearRampToValueAtTime(1350, ts + y.d * 0.3); mf.linearRampToValueAtTime(430, ts + y.d);
          g.setValueAtTime(0, ts); g.linearRampToValueAtTime(0.42, ts + 0.025); g.setValueAtTime(0.42, ts + y.d * 0.7); g.linearRampToValueAtTime(0, ts + y.d);
        }
      }
    }
    // the anthem, or the carrier
    if (s.kind === 'anthem') this.playAnthem(n); else this.stopAnthem(n);
    const carrier = s.kind === 'carrier' || (s.kind === 'anthem' && this.anthemFailed);
    n.car.gain.setTargetAtTime(carrier ? 0.03 : 0, now, 0.05);   // quiet: a dead carrier mostly hushes the static
  }

  private stopHostSources() {
    for (const source of this.hostSources) {
      source.onended = null;
      try { source.stop(); } catch { /* already ended */ }
      source.disconnect();
    }
    this.hostSources = [];
  }
  private cancelHost() {
    this.hostGeneration++;
    this.hostPending?.abort(); this.hostPending = null;
    this.stopHostSources(); this.audioAt = null;
  }
  private cancelArchive() {
    if (this.archive && (this.archive.state === 'loading' || this.archive.state === 'playing')) this.archive.state = 'cancelled';
  }
  /** Read-only episode progress remains available after completion or cancellation. */
  get archiveStatus(): ArchiveProgress | null { return this.archive ? { ...this.archive } : null; }
  /** A first receiver joining an already decoded recording needs its current caption too. */
  private announceRecordedLine() {
    if (!this.n || !this.hostReady) return;
    for (let i = 0; i < this.spoken.length; i++) {
      const line = this.spoken[i];
      if (line.buffers && this.t >= line.at && this.t < line.at + line.secs) {
        this.said = i - 1;
        this.forward(0, this.radio.clock);
        return;
      }
    }
  }
  private prepareHost() {
    const ctx = this.radio.audio.ctx;
    if (!ctx || !this.n || this.hostPending) return;
    const controller = this.hostPending = new AbortController(), generation = ++this.hostGeneration;
    const clips = (bank: 'host' | 'caller') => this.spoken.flatMap((line) => line.bank === bank ? line.clips ?? [] : []);
    if (this.interrupted && this.archive && this.archive.state !== 'cancelled') this.archive.state = 'loading';
    void Promise.all([
      this.radio.hostBank.load(clips('host'), ctx, controller.signal),
      this.radio.callerBank.load(clips('caller'), ctx, controller.signal),
    ]).then(([hostBuffers, callerBuffers]) => {
      if (controller.signal.aborted || generation !== this.hostGeneration || !this.n) return;
      const before = this.spoken.map((line) => line.secs);
      for (const line of this.spoken) if (line.clips) {
        const buffers = line.bank === 'caller' ? callerBuffers : hostBuffers;
        line.buffers = line.clips.map((clip) => {
          const buffer = buffers.get(clip.id);
          if (!buffer) throw new Error(`Incomplete radio recording: ${clip.id}`);
          return buffer;
        });
        line.secs = line.buffers.reduce((sum, buffer) => sum + buffer.duration, 0) + (line.buffers.length - 1) * FRAGMENT_GAP;
      }
      this.layout();
      if (this.seg.kind === 'talk' && !this.seg.duration) this.t = this.mapPosition(before, this.spoken.map((line) => line.secs));
      this.len = this.seg.kind === 'talk' && this.seg.duration ? this.seg.duration : this.spoken.reduce((sum, line) => sum + line.secs + GAP, 0.4);
      this.hostReady = true; this.hostPending = null;
      this.sync();
      this.announceRecordedLine();
      this.forward(0, this.radio.clock);
    }).catch(() => {
      if (controller.signal.aborted || generation !== this.hostGeneration || !this.n) return;
      controller.abort();
      // A failed request retains a complete, word-matched legacy line. No partial recording
      // and no retry loop can hold the programme forever.
      const before = this.spoken.map((line) => line.secs);
      for (const line of this.spoken) { line.clips = null; line.buffers = undefined; line.secs = secsOf(line.text); }
      this.layout();
      if (this.seg.kind === 'talk' && !this.seg.duration) this.t = this.mapPosition(before, this.spoken.map((line) => line.secs));
      this.len = this.seg.kind === 'talk' && this.seg.duration ? this.seg.duration : this.spoken.reduce((sum, line) => sum + line.secs + GAP, 0.4);
      this.hostReady = true; this.hostPending = null; this.sync();
      this.announceRecordedLine();
    });
  }
  /** Current recorded-host state for diagnostics, without creating a second playback path. */
  private voiceStatus(bank: 'host' | 'caller') {
    const lines = this.spoken.filter((line) => bank === 'host' ? line.line.who === 'host' : !!line.line.who && CALLER_VOICES.has(line.line.who));
    const current = lines.find((line) => this.t >= line.at && this.t < line.at + line.secs);
    return {
      loading: !!this.hostPending,
      playing: !!current?.buffers && this.awake && this.radio.audio.ctx?.state === 'running',
      text: current?.text ?? null, duration: current?.secs ?? null,
      clips: lines.flatMap((line) => line.clips?.map((clip, i) => ({ id: clip.id, text: clip.text, voice: clip.voice,
        duration: line.buffers?.[i]?.duration ?? clip.duration })) ?? []),
      bufferCount: (bank === 'host' ? this.radio.hostBank : this.radio.callerBank).bufferCount,
    };
  }
  get hostStatus() { return this.voiceStatus('host'); }
  get callerStatus() { return this.voiceStatus('caller'); }
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
    el.play().catch((e: DOMException) => { if (e?.name !== 'AbortError') fail(); });
  }
  /** A part of a song for the length of the segment, faded in and out like a station joining it. */
  private playTrack(n: NonNullable<Station['n']>, tune: TuneId, len: number) {
    const ctx = this.radio.audio.ctx!, tr = TRACK[tune];
    let a = n.track;
    const url = new URL(`radio/${tr.file}.mp3`, document.baseURI).href;
    if (!a) {
      const el = new Audio(); el.preload = 'auto';
      let src: MediaElementAudioSourceNode;
      try { src = ctx.createMediaElementSource(el); } catch { this.trackFailed = true; return; }   // sync() goes on with the bars made here
      const g = ctx.createGain(); g.gain.value = 0;
      src.connect(g); g.connect(n.mus);
      a = n.track = { el, src, g };
      el.addEventListener('error', () => { if (n.track?.el !== el || !el.getAttribute('src') || !el.error || el.error.code === MediaError.MEDIA_ERR_ABORTED) return; this.trackFailed = true; this.stopTrack(n); this.sync(); });
    }
    const el = a.el, room = Math.max(1, tr.dur - len - 2);
    const from = (this.segNo * 61.7) % room, seg = this.segNo;
    if (el.src !== url) el.src = url;
    // where the song is by now (the file may take a moment to come)
    const seek = () => { if (this.segNo === seg) try { el.currentTime = from + this.t; } catch { /* not yet */ } };
    if (el.readyState >= 1) seek(); else el.addEventListener('loadedmetadata', seek, { once: true });
    // A play() cut short by the next segment's pause (a slow phone: the song was still loading
    // when the talk came) is not a failure. Tom, 18 October: that put the old bars back for the
    // rest of the night. Not allowed to play just now: the bars made here for this segment only.
    el.play().catch((e: DOMException) => {
      if (e?.name === 'AbortError' || this.segNo !== seg || n.track?.el !== el) return;
      this.trackSkip = seg; this.stopTrack(n); this.sync();
    });
    const now = ctx.currentTime, left = len - this.t, g = a.g.gain;
    g.cancelScheduledValues(now);
    g.setValueAtTime(0, now);
    g.linearRampToValueAtTime(0.85, now + Math.max(0.3, 1.5 - this.t));
    if (left > 2) { g.setValueAtTime(0.85, now + left - 2); g.linearRampToValueAtTime(0, now + left); }
  }
  private stopTrack(n: NonNullable<Station['n']>) {
    const a = n.track; if (!a) return;
    a.el.pause(); a.g.gain.cancelScheduledValues(0); a.g.gain.value = 0;
    if (!this.n) { n.track = null; a.src.disconnect(); a.g.disconnect(); a.el.removeAttribute('src'); a.el.load(); }
  }
  /** For the tests: the song's element, while it plays. */
  get trackEl() { return this.n?.track && !this.n.track.el.paused ? this.n.track.el : null; }
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
  /** Heard from outside a closed cab, 0..1: muffled and faint (Tom, 18 October). */
  muffle = 0;
  /** Seconds of tuning noise left after the dial is turned. */
  private sweep = 0;
  /** Interference (Tom, 16 October: static now and then, at random, with the wah wah): every
   *  5 to 20 seconds the static swells over the station for half a second to three, crackling,
   *  and the station sinks under it, then comes back. */
  private fuzz = { next: 12 + Math.random() * 20, left: 0, len: 1, depth: 0 };
  /** The dial was turned or the set switched on: a crack of static with the sweep (bought sound). */
  private dial = false;
  /** How far under the static the station is now, 0..1 (the tests). */
  interference = 0;
  /** Seconds left of a bare carrier (carrier()): the station gone from under it, on or off. */
  private carry = 0;
  /** The bought crackles played so far (the tests). */
  cracks = 0;
  private n: { sig: GainNode; stat: GainNode; mix: GainNode; car: GainNode; out: GainNode; cab: BiquadFilterNode; pan: PannerNode; bus: GainNode | null; stop: () => void } | null = null;
  private listening: StationId | null = null;

  constructor(private radio: Radio, private o: ReceiverOpts) {}

  set(on: boolean, station: StationId = this.station) {
    if (station !== this.station) { this.station = station; this.sweep = 0.5; this.dial = true; }
    if (on && !this.on) { this.sweep = Math.max(this.sweep, 0.25); this.dial = true; }
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
  /** Giving a bare carrier now (carrier()), somewhere it can be heard: the dial lamp is lit for it
   *  even with the set switched off. */
  get carrying() { return this.carry > 0 && !!this.pos; }
  /** The station drops out from under a bare carrier for `secs` (story/truckOnB.ts: the truck on B,
   *  as 05:29 has it, story/Chapter6.ts), whatever it was playing; switched off, the speaker gives
   *  the carrier all the same. carrier(0) lets the station come back now. */
  carrier(secs: number) { this.carry = secs > 0 ? Math.max(this.carry, secs) : 0; }

  private build() {
    const a = this.radio.audio, ctx = a.ctx, n = a.staticSrc();
    if (!ctx || !n) return null;
    // the station and the static, through a small speaker: a narrow band, a little overdriven
    const sig = ctx.createGain(); sig.gain.value = 0;
    const stat = ctx.createGain(); stat.gain.value = 0; n.out.connect(stat);
    const mix = ctx.createGain(); sig.connect(mix); stat.connect(mix);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1250; bp.Q.value = 0.7;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3400;
    const sh = ctx.createWaveShaper();
    const curve = new Float32Array(256); for (let i = 0; i < 256; i++) { const x = i / 127.5 - 1; curve[i] = Math.tanh(x * 1.6) / Math.tanh(1.6); }
    sh.curve = curve;
    // the bare carrier (carrier()): the 760 Hz the game's other radios have for it, into the same speaker
    const osc = ctx.createOscillator(); osc.frequency.value = 760;
    const car = ctx.createGain(); car.gain.value = 0; osc.connect(car); car.connect(mix); osc.start();
    const out = ctx.createGain(); out.gain.value = 0;
    // the cab's doors and glass between the speaker and whoever stands outside
    const cab = ctx.createBiquadFilter(); cab.type = 'lowpass'; cab.frequency.value = 20000; cab.Q.value = 0.5;
    mix.connect(bp); bp.connect(lp); lp.connect(sh); sh.connect(cab); cab.connect(out);
    const pan = a.spatial(new THREE.Vector3(), this.o.ref ?? 1.2, a.radioBus) as PannerNode;
    out.connect(pan);
    return { sig, stat, mix, car, out, cab, pan, bus: null, stop: () => { for (const x of [n.src, osc]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }

  update(dt: number) {
    const ctx = this.radio.audio.ctx;
    if (!ctx) return;
    this.carry = Math.max(0, this.carry - dt);
    if (!this.n) { if (!this.on && !this.carrying) return; this.n = this.build(); if (!this.n) return; }
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
      else if ((z.next -= dt) <= 0) { z.len = z.left = 0.4 + Math.random() * 1.2; z.depth = 0.3 + Math.random() * 0.4; z.next = 20 + Math.random() * 40; this.crack(z.len, z.depth); }
    } else z.left = 0;
    if (this.dial && this.live) this.crack(0, 0.6);
    this.dial = false;
    const k = z.left > 0 ? z.depth * Math.sqrt(Math.sin(Math.PI * (1 - z.left / z.len))) : 0;
    this.interference = k;
    const crackle = k > 0 ? 0.5 + Math.random() * 1.1 : 1;
    // a bare carrier: the station gone at once, a steady tone and a breath of static under it
    const bare = this.carrying, sounds = this.live || bare;
    n.sig.gain.setTargetAtTime(this.live && !bare ? s * (1 - tuning) * (1 - 0.5 * k) * this.power : 0, now, bare ? 0.008 : 0.05);
    n.stat.gain.setTargetAtTime(bare ? 0.035 * this.power : this.live ? (0.03 + (carrier ? 0.02 : 0.36 * (1 - s)) + tuning * 0.45 + 0.42 * k * crackle) * this.power : 0, now, bare ? 0.008 : k > 0 ? 0.015 : 0.04);
    n.car.gain.setTargetAtTime(bare ? 0.11 * this.power : 0, now, 0.008);
    const m = THREE.MathUtils.clamp(this.muffle, 0, 1);
    n.out.gain.setTargetAtTime(sounds ? this.o.vol * (1 - 0.82 * m) : 0, now, sounds ? (bare ? 0.01 : 0.08) : 0.15);
    n.cab.frequency.setTargetAtTime(20000 * Math.pow(420 / 20000, m), now, 0.1);
    if (this.pos && Number.isFinite(this.pos.x + this.pos.y + this.pos.z)) {
      const p = n.pan as PannerNode & { positionX?: AudioParam };
      if (p.positionX) { p.positionX.value = this.pos.x; p.positionY.value = this.pos.y; p.positionZ.value = this.pos.z; }
      else (p as unknown as { setPosition: (x: number, y: number, z: number) => void }).setPosition(this.pos.x, this.pos.y, this.pos.z);
    }
  }
  /** A crash of static over the station, recorded (crunches, which the band-pass makes into
   *  cracks): the long one stretched over an interference of `len` seconds, the short one for a
   *  short one or the dial (len 0). Only with the bought sounds; the static's own swell stays. */
  private crack(len: number, k: number) {
    const a = this.radio.audio, name = len > 0.8 ? a.or('radio_crackle_long') : a.or('radio_crackle_short'), b = a.buf[name];
    if (!b || !this.n) return;
    a.play(name, { dest: this.n.mix, gain: k * this.power, rate: len ? THREE.MathUtils.clamp(b.duration / len, 0.75, 1.5) : 1 });
    this.cracks++;
  }
  /** The station slides away under its own static for a few seconds and comes back (story/curve.ts:
   *  one of the signs in its build), as if the set had been knocked off the station. */
  drift(secs = 3.5, depth = 1) {
    if (!this.live) return false;
    const z = this.fuzz;
    z.len = z.left = secs; z.depth = depth; z.next = Math.max(z.next, 20);
    this.crack(secs, depth);
    return true;
  }
  /** Off, and let the station go (a new night). */
  reset() {
    this.on = false; this.sweep = 0; this.dial = false; this.carry = 0;
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
  onLine?: (station: StationId, text: string, by: string | undefined, duration?: number) => void;
  /** The boy has called the Night Line tonight (event 9; main.ts keeps it with the case). */
  boyCalled = false;
  readonly hostBank = new RadioVoice();
  readonly callerBank = new RadioVoice('caller');
  private recordedHost = false;
  private recordedCallers = false;
  get hostVoice() { return this.recordedHost; }
  get callerVoice() { return this.recordedCallers; }
  setHostVoice(on: boolean) {
    if (this.recordedHost === on) return;
    this.recordedHost = on;
    this.stations.caliche.revoice();
  }
  setCallerVoice(on: boolean) {
    if (this.recordedCallers === on) return;
    this.recordedCallers = on;
    this.stations.caliche.revoice('caller');
  }

  constructor(readonly audio: AudioSys) {
    this.stations = { caliche: new Station('caliche', this), roswell: new Station('roswell', this), far: new Station('far', this) };
    for (const id of DIAL) this.stations[id].onLine = (text, by, duration) => this.onLine?.(id, text, by, duration);
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
      return [id, { seg: s.seg.kind, t: s.t, free: s.free, slotted: s.seg.kind === 'talk' && !!s.seg.slotted, awake: s.awake, listeners: s.listeners, strength: s.strength(this.clock, this.time),
        anthem: s.anthemEl ? { time: s.anthemEl.currentTime, paused: s.anthemEl.paused, failed: s.anthemFailed } : null, anthemFailed: s.anthemFailed,
        host: s.hostStatus, callers: s.callerStatus, archive: s.archiveStatus }];
    }));
  }
}

/** Where a frequency is on the truck's AM scale (dash canvas pixels, truckTextures.ts). */
export function dialX(freq: number) {
  const P: [number, number][] = [[540, 88], [600, 125], [700, 162], [800, 199], [1000, 236], [1200, 273], [1600, 310]];
  for (let i = 1; i < P.length; i++) if (freq <= P[i][0]) { const [f0, x0] = P[i - 1], [f1, x1] = P[i]; return x0 + (x1 - x0) * (freq - f0) / (f1 - f0); }
  return P[P.length - 1][1];
}
