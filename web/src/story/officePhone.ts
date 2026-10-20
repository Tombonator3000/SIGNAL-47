import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { MotelFront } from '../world/MotelFront';
import type { BoardCard } from '../ui/Board';
import { PHONE_CALLS } from '../core/phoneScripts';
import { RuleBook, pair, type BookNote, type BookRule } from './boardRules';
import { clockText } from './time';

// The crank telephone in the office of Sierra Motor Court plays the player back (FORSLAG.md,
// 20 October, item 7). The oak wall set on the office's north wall is the far end of the old
// field line from the hut at STATION 01; Nora said so on the hut's telephone in chapter three
// ("This line still rings in the motel office"). In chapter four, before room 6 is over, the
// first time the player comes to the office on foot (in it, onto the motel's court, which every way
// to room 6 crosses, or within earshot of the office's open door), it rings: its own
// bell, two small gongs and a clapper, in uneven bursts, the way a magneto rings when somebody
// turns the handle at the other end. Nobody is at the other end.
//
// Picked up, nobody speaks. Down a narrow line, with the carrier of the receiver the player left on
// in the hut under it, comes what the hut's telephone registered of the player out there, in order
// and compressed: steps on the hut's boards (the same recordings the player's own feet made on
// them), the hut door on its hinges, the steps going out; then, not far off, a shutter, and another,
// as far apart as FRAME 03 and FRAME 04 were taken (their real capture clocks, chapter three's
// s.f3 and s.f4; a long wait is drawn in, see shutterGap); the hut's bell right at the mouthpiece,
// (the same ring the player heard out there); steps under it, across the boards right up to the
// telephone; the handset lifted, very faint from its earpiece
// Nora's first two lines ("So you went out there." "This line still rings in the motel office...",
// her recordings when the game has both, else the captions), someone listening, and the handset put
// down. An imprint, as R5 has it: it gives back what was registered
// and answers nothing; only, now the player is on it, as T. Vega's correction is. It prepares P12
// in room 6, and 05:29:47.
//
// A night without the two frames (the developer menu's chapter four has none) gets what the player
// did at the hut: the steps, the door, the call. One thing at a time: it rings only when nothing
// else of the night's is going (main.ts, oneAtATime), and nothing starts while it rings or is off
// the hook, nor for a few seconds after. The night's curve does not gate it (it is the story's, like
// the payphone's second call), but once it rings it lands the curve's peak (story/curve.ts, FELT). Kept
// apart from the office's television (story/motelTv.ts): main.ts holds the set while this goes.
// Blocked as the player comes, it waits while the other thing goes and rings when nothing has gone
// for a while (AFTER) if the player is still there; missed or hung up on, it rings again the next
// time the player comes to the office before room 6 is over. Heard to the end it is written down
// once, kept with the town's part of the case ('ev:office', 'office@<clock>') and on the case
// board, with threads to the two frames.

export type OfficePhoneState = 'idle' | 'ringing' | 'line';

export interface OfficePhoneIn {
  /** In play, no panel, no cinematic, and nothing else of the night's going (one at a time). */
  ok: boolean;
  /** Chapter four, before room 6 is over. */
  open: boolean;
  /** On foot in the office, on the motel's court, or near the office's door. */
  near: boolean;
  /** On foot in SARO's area within earshot of the office (the bell rings on while they are). */
  here: boolean;
  /** The player's feet. */
  pos: THREE.Vector3;
  clock: number;
}
export interface OfficePhoneDeps {
  audio: AudioSys;
  motel: MotelFront;
  /** The town's part of the case (main.ts, `town`). */
  seen: Set<string>;
  notes: () => string[];
  note: (text: string) => void;
  toast: (text: string, secs?: number, clock?: () => number) => void;
  /** FRAME 03 and FRAME 04 as chapter three has them. */
  frames: () => { f3?: { clock: number } | null; f4?: { clock: number } | null };
  save: () => void;
  /** The case board, drawn again. */
  refresh: () => void;
}

/** Seconds after coming to the office before it rings; and, when something else of the night's was
 *  going as the player came (or the case was just opened there), seconds of nothing going first. */
const DELAY = 1.4, AFTER = 8;
/** Seconds it rings for if nobody picks it up (five turns of the pattern: from the far side of the
 *  lot to the telephone is some fifteen seconds' walk). */
const RING_MAX = 36;
/** Within this far of the telephone the player is in the office with it (the toast says which). */
const BY_IT = 6;
/** How far from the telephone the cord lets the handset go (metres, on the floor). */
const CORD = 2.8;
/** Seconds nothing else of the night's may start after it (one at a time). */
const TAIL = 10;
/** The gap between the two shutters: as it was up to REAL seconds; a longer one drawn in towards CAP. */
const REAL = 8, CAP = 14, MIN_GAP = 0.7, DRAW = 90;
/** Nora's first two lines on the hut's telephone, and how long they last without her recording. The
 *  second is about this very telephone ("This line still rings in the motel office."), heard in the
 *  motel office, with its handset at the player's ear. */
const NORA_FIRST = PHONE_CALLS.fieldNora[0].text, NORA_NEXT = PHONE_CALLS.fieldNora[1].text;
const VOICE_SECS = 1.4, VOICE2_SECS = 4.4;
/** The office's bell: one turn of the pattern, seconds, and the bursts in it (the handle turned). */
const BELL_LOOP = 7.2;
const BURSTS: [number, number][] = [[0, 1.4], [1.95, 2.6], [3.1, 4.4]];

/** How far apart the two shutters come on the line, from how far apart FRAME 03 and FRAME 04 were
 *  taken (seconds of the night's clock): the same up to REAL seconds; beyond that drawn in, the
 *  longer the wait the longer still, but never past CAP (a wait of minutes at the board, or a time
 *  away between them, is not a minute of carrier). */
export function shutterGap(dt: number): number {
  const d = Math.abs(dt);
  if (!Number.isFinite(d)) return REAL;
  if (d <= REAL) return Math.max(MIN_GAP, d);
  return REAL + (CAP - REAL) * (1 - Math.exp(-(d - REAL) / DRAW));
}

export type LineCueKind = 'open' | 'board' | 'creak' | 'door' | 'out' | 'shutter' | 'ring' | 'lift' | 'voice' | 'voice2' | 'down' | 'end';
export interface LineCue { at: number; kind: LineCueKind; gain?: number; dur?: number }
export interface LineCap { at: number; text: string; secs: number }
export interface LinePlan {
  cues: LineCue[]; caps: LineCap[];
  /** Seconds between the frames as they were taken, and between the shutters on the line (null without the frames). */
  delta: number | null; gap: number | null;
  /** When the two shutters come, seconds from the pick-up (none without the frames). */
  shutters: number[];
  end: number;
}

const STEP = 0.56;
/** What comes down the line, seconds from the pick-up. */
export function planLine(f3?: number | null, f4?: number | null, voice = VOICE_SECS, voice2 = VOICE2_SECS): LinePlan {
  const cues: LineCue[] = [], caps: LineCap[] = [];
  const cue = (at: number, kind: LineCueKind, o: { gain?: number; dur?: number } = {}) => cues.push({ at, kind, ...o });
  const cap = (at: number, text: string, secs: number) => caps.push({ at, text, secs });
  cue(0, 'open');
  cap(0.5, 'Nobody on the line. A carrier, very weak, under the hiss.', 3.6);
  // in the hut: across the boards from the telephone to the door, and out
  const first = 2.2;
  for (let i = 0; i < 6; i++) cue(first + i * STEP, 'board', { gain: 0.72 - i * 0.06 });
  cue(first + 1.9, 'creak');
  cap(first + 0.25, 'Boards creak out there. Someone is walking on a wooden floor.', 3.4);
  const door = first + 6 * STEP + 0.2;
  cue(door, 'door');
  cue(door + 0.55, 'out', { gain: 0.36 }); cue(door + 1.15, 'out', { gain: 0.26 });
  cap(door + 0.1, 'A door moves on its hinges. The steps go out.', 3.2);
  let t = door + 2.3;
  const shutters: number[] = [];
  let delta: number | null = null, gap: number | null = null;
  if (Number.isFinite(f3) && Number.isFinite(f4)) {
    delta = Math.abs((f4 as number) - (f3 as number));
    gap = shutterGap(delta);
    shutters.push(t, t + gap);
    cue(t, 'shutter'); cap(t, 'Outside, not far off: a shutter.', 2.8);
    cue(t + gap, 'shutter'); cap(t + gap, 'Another.', 2.4);
    t += gap + 1.8;
  } else t += 2.6;   // nothing out there but the carrier, a while
  // the telephone in the hut rings; under the second ring steps cross the boards right up to it,
  // and it is lifted (the bell is cut short). (Only the boards: the player may have been in the hut
  // when it rang out there, at the receiver, or come in from the station; either way they crossed
  // the boards to it.)
  const ring = t;
  cue(ring, 'ring', { dur: 1.75 });
  cap(ring, 'A bell, right at the mouthpiece: the telephone in the hut is ringing.', 3.6);
  const ring2 = ring + 3.0, inAt = ring + 2.4;
  for (let i = 0; i < 4; i++) cue(inAt + i * STEP, 'board', { gain: 0.5 + i * 0.09 });
  cap(ring2, 'It rings again. Steps cross the boards, right up to it.', 3.4);
  const lift = inAt + 4 * STEP + 0.35;
  // (seconds of the recording: it is played at 0.82, as the hut's bell was)
  cue(ring2, 'ring', { dur: Math.min(1.75, (lift - ring2) * 0.82) });
  cue(lift, 'lift'); cap(lift, 'Someone picks it up.', 2.4);
  const v = lift + 0.9;
  cue(v, 'voice');
  cap(v, `Very faint, from the earpiece out there, a woman: "${NORA_FIRST}"`, Math.max(3, voice + 1.6));
  // her next line, fainter still: about this telephone, heard on it, in the motel office
  const v2 = v + voice + 0.7;
  cue(v2, 'voice2');
  cap(v2, `She goes on: "${NORA_NEXT}"`, Math.max(3.6, voice2 + 1.2));
  const listen = v2 + voice2 + 0.8;
  cap(listen, 'Nobody answers her. Someone is listening.', 3.2);
  const down = listen + 2.8;
  cue(down, 'down'); cap(down, 'The handset goes down out there. Only the carrier.', 3.2);
  const end = down + 2.4;
  cue(end, 'end');
  cues.sort((a, b) => a.at - b.at);
  return { cues, caps, delta, gap, shutters, end };
}

// ---------- the board ----------
const NOTES: Record<string, BookNote> = {
  'n.ol.frames': { text: 'The two shutters on the office line are my two frames at STATION 01, as far apart as I took them.' },
  'n.ol.rx': { text: 'The carrier under the steps is the receiver I left on in the hut.' },
  'n.ol.room6': { text: 'Room 6 is on the same line. Asked about the key, it gives back his correction from 1947. Nobody asked it about me, and it gave me back.' },
};
const RULES: BookRule[] = [
  { a: 'ol.shutters', b: 'frame03', note: 'n.ol.frames', text: NOTES['n.ol.frames'].text },
  { a: 'ol.shutters', b: 'frame04', note: 'n.ol.frames', text: NOTES['n.ol.frames'].text },
  { a: 'ol.boards', b: 'hut.rx', note: 'n.ol.rx', text: NOTES['n.ol.rx'].text },
  { a: 'ol.answer', b: 'r6.line', note: 'n.ol.room6', text: NOTES['n.ol.room6'].text },
];
const LINK = 'ol:';   // a thread on the board, kept in the town's part of the case

/** The office's own bell, made once: two small brass gongs a little apart in pitch and a clapper
 *  between them, struck in turn as fast as the generator at the other end is turned (it comes up to
 *  speed and slows with each turn of the handle), each gong ringing on in its partials. */
function crankBell(ctx: BaseAudioContext): AudioBuffer {
  const sr = 16000, len = Math.round(sr * BELL_LOOP);   // (the top partial is near 6.1 kHz)
  const buf = ctx.createBuffer(1, len, sr), out = buf.getChannelData(0);
  // [ratio, level, seconds to fall to 1/e]: a small hemispherical gong
  const PARTS: [number, number, number][] = [[1, 1, 0.5], [2.04, 0.5, 0.32], [2.83, 0.38, 0.24], [4.16, 0.22, 0.15], [5.43, 0.13, 0.1]];
  const gongs = [1047, 1123].map((f0) => PARTS.map(([k, a, tau]) => {
    const w = 2 * Math.PI * f0 * k / sr, r = Math.exp(-1 / (tau * sr));
    return { c1: 2 * r * Math.cos(w), c2: -r * r, a: a * Math.sin(w), y1: 0, y2: 0 };
  }));
  // where the clapper strikes, gong by gong (a little uneven: a hand on a crank)
  const hits = new Float32Array(len * 2);
  let side = 0, seed = 7;
  const rnd = () => { seed = (seed * 16807) % 2147483647; return seed / 2147483647; };
  for (const [a, b] of BURSTS) {
    for (let t = a; t < b;) {
      const k = (t - a) / (b - a), f = 14 + 6 * Math.sin(Math.PI * Math.min(1, k * 1.15));
      const i = Math.round(t * sr);
      if (i < len) hits[i * 2 + side] = 0.75 + 0.25 * rnd();
      side ^= 1;
      t += 1 / (2 * f);
    }
  }
  let peak = 0;
  for (let i = 0; i < len; i++) {
    let y = 0;
    for (let g = 0; g < 2; g++) {
      const x = hits[i * 2 + g];
      for (const p of gongs[g]) {
        const v = p.c1 * p.y1 + p.c2 * p.y2 + x * p.a;
        p.y2 = p.y1; p.y1 = v; y += v;
      }
      // the clapper's own tick on the metal
      if (x) for (let j = 0; j < 24 && i + j < len; j++) out[i + j] += (rnd() * 2 - 1) * 0.35 * x * (1 - j / 24);
    }
    out[i] += y;
    peak = Math.max(peak, Math.abs(out[i]));
  }
  if (peak > 0) for (let i = 0; i < len; i++) out[i] *= 0.85 / peak;
  return buf;
}
let bellBuf: AudioBuffer | null = null;

export class OfficePhone {
  state: OfficePhoneState = 'idle';
  /** How the last time went, for the seen log and the tests: heard to the end, hung up on or walked
   *  away from, or let ring. */
  result: 'heard' | 'away' | 'unanswered' | null = null;
  /** What the last call had on it, and when it began (the line's clock), for the tests. */
  plan: LinePlan | null = null;
  startedAt = 0;
  /** Whether Nora's recording played on it (the tests), and the captions given so far. */
  voiced = false;
  shown: string[] = [];
  /** Rung tonight (since the case was opened): how many times. */
  rings = 0;
  readonly book: RuleBook;
  private t = 0;
  private was = false;
  /** Come to the office (and not rung since): it rings once nothing else has been going for DELAY,
   *  or for AFTER when something was (`waited`); walked away, it waits for the next time. */
  private armed = false;
  private waited = false;
  private calm = 0;
  /** The first step after a night was opened (Continue in the office: not at once). */
  private opened = true;
  private pos: THREE.Vector3 | null = null;
  private ringT = 0;
  private tail = 0;
  private capAt = 0;
  private ctx: AudioContext | null = null;
  private bus: GainNode | null = null;
  private loops: AudioScheduledSourceNode[] = [];
  private nodes: AudioNode[] = [];
  private bell: { src: AudioBufferSourceNode; gain: GainNode } | null = null;
  private links: string[] = [];
  private v = new THREE.Vector3();

  constructor(private d: OfficePhoneDeps) {
    this.book = new RuleBook({
      key: 'ch4',
      owns: (n) => n.startsWith('ol.'),
      notes: NOTES, rules: RULES, questions: [],
      links: () => this.links,
      wrong: () => {},
      save: () => { this.keepLinks(); this.d.save(); },
      cards: () => this.cards(),
      empty: 'Those two do not say anything about each other.',
    });
  }

  /** Ringing, on the line, or just put down: nothing else of the night's starts (one at a time). */
  get busy() { return this.state !== 'idle' || this.tail > 0; }
  /** Heard to the end tonight. */
  get heard() { return this.d.seen.has('ev:office'); }
  /** The line's own card on the board, once heard. */
  books() { return this.heard ? [this.book] : []; }

  /** The line's clock: the sound's own while there is sound (a pause suspends it and holds the call),
   *  else the page's. Chosen when the call is picked up, and kept for it. */
  private lineTime() { return this.ctx ? this.ctx.currentTime : performance.now() / 1000; }
  private phoneAt() {
    const p = this.d.motel.proxies.officePhone;
    return p ? p.getWorldPosition(this.v) : this.v.set(0, -999, 0);
  }
  private handset() { return this.d.motel.objs.officePhoneHandset ?? null; }

  // ---------- the interaction (story/Chapter4.ts asks first) ----------
  label(): string | null {
    if (this.state === 'ringing') return 'Answer the field telephone';
    if (this.state === 'line') return 'Hang up';
    return null;
  }
  /** Use on the telephone: answer it, hang it up, or (heard) lift it and hear the carrier. False:
   *  nothing of this one's to do, the chapter's own words for it. */
  use(): boolean {
    if (this.state === 'ringing') { this.answer(); return true; }
    if (this.state === 'line') { this.hangUp('You put the handset back on its hook.'); return true; }
    if (this.heard) { this.listenAgain(); return true; }
    return false;
  }

  // ---------- per frame ----------
  update(dt: number, s: OfficePhoneIn) {
    this.t += dt;
    this.pos = s.pos;
    if (this.tail > 0) this.tail = Math.max(0, this.tail - dt);
    const entered = s.near && !this.was;
    this.was = s.near;
    const opened = this.opened; this.opened = false;
    switch (this.state) {
      case 'idle':
        if (!s.open || this.heard) { this.armed = false; break; }
        // the bell is made while the player is still on the way (a moment's work, not at the ring)
        if (s.here && !bellBuf && this.d.audio.ctx) bellBuf = crankBell(this.d.audio.ctx);
        if (entered) { this.armed = true; this.waited = opened; this.calm = 0; }
        if (!s.near) { this.armed = false; break; }
        if (!this.armed) break;
        // never on top of something else of the night's: it waits while that goes, and a while after
        if (!s.ok) { this.calm = 0; this.waited = true; break; }
        this.calm += dt;
        if (this.calm >= (this.waited ? AFTER : DELAY)) { this.armed = false; this.ring(); }
        break;
      case 'ringing': {
        this.ringT += dt;
        const h = this.handset();
        const ph = this.ringT % BELL_LOOP, on = BURSTS.some(([a, b]) => ph >= a && ph < b + 0.15);
        if (h) h.rotation.z = on ? Math.sin(this.t * 70) * 0.05 : 0;
        if (this.ringT > RING_MAX || !s.open || !s.here) this.unanswered(s.here);
        break;
      }
      case 'line': {
        const p = this.phoneAt();
        if (Math.hypot(s.pos.x - p.x, s.pos.z - p.z) > CORD) { this.hangUp('The cord will not reach. The handset goes back on its hook.'); break; }
        this.lineStep(s.clock);
        break;
      }
    }
  }

  // ---------- the bell ----------
  private ring() {
    const a = this.d.audio, ctx = a.ctx;
    this.state = 'ringing'; this.ringT = 0; this.result = null; this.rings++;
    if (ctx) {
      bellBuf ??= crankBell(ctx);
      // (from the two gongs over the mouthpiece; a magneto bell is loud, made to be heard out of
      // doors: across the lot through the office's open door)
      const at = this.phoneAt().clone(); at.y += 0.2;
      const dest = a.spatial(at, 2.5);
      if (dest) {
        const src = ctx.createBufferSource(); src.buffer = bellBuf; src.loop = true;
        const gain = ctx.createGain(); gain.gain.value = 0.45; src.connect(gain); gain.connect(dest);
        src.start();
        this.bell = { src, gain };
      }
    }
    const p = this.phoneAt(), q = this.pos, by = !q || Math.hypot(q.x - p.x, q.z - p.z) <= BY_IT;
    this.d.toast(by ? 'The crank telephone in the office is ringing. At the other end of the line, someone is turning the handle.'
      : 'In the motel office, a telephone is ringing: a bell in bursts, the way a crank telephone rings when someone at the other end turns the handle.', 4.6);
  }
  private stopBell() {
    const b = this.bell, ctx = this.d.audio.ctx;
    this.bell = null;
    const h = this.handset(); if (h) h.rotation.z = 0;
    if (!b || !ctx) return;
    b.gain.gain.setTargetAtTime(0, ctx.currentTime, 0.03);
    try { b.src.stop(ctx.currentTime + 0.25); } catch { /* stopped */ }
  }
  private unanswered(here: boolean) {
    this.stopBell();
    this.state = 'idle'; this.result = 'unanswered'; this.tail = 4;
    if (here) this.d.toast('The ringing stops.', 2.4);
  }

  // ---------- the line ----------
  private answer() {
    const a = this.d.audio;
    this.stopBell();
    const at = this.phoneAt().clone();
    const h = this.handset(); if (h) h.visible = false;
    a.play('click', { at, gain: 0.55 });
    a.play('thudSoft', { at, gain: 0.16, rate: 1.7, when: 0.05 });
    const fr = this.d.frames();
    // her recordings, both or neither (never half of what she said out there spoken)
    let voice = a.lineBuffer('N. VEGA', NORA_FIRST), voice2 = a.lineBuffer('N. VEGA', NORA_NEXT);
    if (!voice || !voice2) voice = voice2 = undefined;
    const plan = planLine(fr.f3?.clock, fr.f4?.clock, voice?.duration ?? VOICE_SECS, voice2?.duration ?? VOICE2_SECS);
    this.plan = plan; this.shown = []; this.capAt = 0; this.voiced = false;
    this.state = 'line';
    this.schedule(plan, voice, voice2);
    this.startedAt = this.lineTime();
  }

  /** Every sound of the call put on the line's clock at once: a pause holds it all (the context is
   *  suspended), and a frame that comes late cannot pull the steps apart. */
  private schedule(plan: LinePlan, voice: AudioBuffer | undefined, voice2: AudioBuffer | undefined) {
    const a = this.d.audio, ctx = a.ctx;
    this.ctx = ctx && ctx.state === 'running' && a.noiseBuffer() ? ctx : null;
    if (!this.ctx) return;
    const c = this.ctx;
    const line = a.phoneLine();
    const bus = c.createGain(); bus.gain.value = 1; bus.connect(line.input);
    // what is a few metres from the hut's telephone, and out of its door: duller
    const far = c.createBiquadFilter(); far.type = 'lowpass'; far.frequency.value = 2000; far.connect(bus);
    const outside = c.createGain(); outside.gain.value = 1.1;
    const ol = c.createBiquadFilter(); ol.type = 'lowpass'; ol.frequency.value = 3000; outside.connect(ol); ol.connect(bus);
    this.bus = bus; this.nodes = [bus, far, outside, ol, ...line.nodes];
    const t0 = c.currentTime;
    // the line itself: its hiss, and the bought hush of an open line (the payphone has it too)
    const hiss = c.createBufferSource(); hiss.buffer = a.noiseBuffer()!; hiss.loop = true;
    const hg = c.createGain(); hg.gain.value = 0.015; hiss.connect(hg); hg.connect(bus); hiss.start(t0);
    this.loops.push(hiss); this.nodes.push(hg);
    const hush = a.lic('line_hush');
    if (hush) {
      const h = c.createBufferSource(); h.buffer = hush; h.loop = true;
      const g = c.createGain(); g.gain.value = 0.09; h.connect(g); g.connect(bus); h.start(t0, Math.random() * hush.duration);
      this.loops.push(h); this.nodes.push(g);
    }
    // the receiver left on in the hut, across the room: its carrier, coming and going (Audio.radio)
    const car = c.createGain(); car.gain.value = 0;
    car.gain.setValueAtTime(0, t0); car.gain.linearRampToValueAtTime(0.03, t0 + 1.4);
    const bp = c.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1100; bp.Q.value = 0.6;
    car.connect(bp); bp.connect(bus);
    const osc = c.createOscillator(); osc.frequency.value = 760;
    const og = c.createGain(); og.gain.value = 0.6; osc.connect(og); og.connect(car);
    const lfo = c.createOscillator(); lfo.frequency.value = 0.21;
    const lg = c.createGain(); lg.gain.value = 0.35; lfo.connect(lg); lg.connect(og.gain);
    osc.start(t0); lfo.start(t0);
    this.loops.push(osc, lfo); this.nodes.push(car, bp, og, lg);
    const st = a.staticSrc();
    if (st) { const sg = c.createGain(); sg.gain.value = 0.25; st.out.connect(sg); sg.connect(car); this.loops.push(st.src); this.nodes.push(sg); }
    const pick = (kind: string) => `${kind}${Math.floor(Math.random() * 5)}`;
    for (const q of plan.cues) {
      const when = q.at;
      switch (q.kind) {
        case 'open': a.play('click', { gain: 0.5, when, dest: bus }); break;
        case 'board': a.play(pick('stepWood'), { gain: q.gain, rate: 0.95 + Math.random() * 0.1, lp: 3000, when, dest: bus }); break;
        case 'creak': a.play('doorCreak0', { gain: 0.16, rate: 2.3, when, dest: far }); break;
        case 'door': a.play('doorCreak1', { gain: 0.35, rate: 0.9, when, dest: far }); break;
        case 'out': a.play(pick('stepGravel'), { gain: q.gain, rate: 0.88, lp: 2400, when, dest: far }); break;
        case 'shutter': a.shutter({ dest: outside, when }); break;
        case 'ring': a.play('phoneRing', { gain: 0.5, rate: 0.82, from: 0, dur: q.dur, when, dest: bus }); break;
        case 'lift':
          a.play('click', { gain: 0.6, when, dest: bus });
          a.play('thudSoft', { gain: 0.3, rate: 1.9, when: when + 0.05, dest: bus });
          break;
        case 'voice': case 'voice2': {
          const buf = q.kind === 'voice' ? voice : voice2;
          if (buf) {
            // her voice in the earpiece out there, as the mouthpiece beside it heard it: small and thin
            const s = c.createBufferSource(); s.buffer = buf;
            const vb = c.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = 1400; vb.Q.value = 0.9;
            const vg = c.createGain(); vg.gain.value = q.kind === 'voice' ? 0.36 : 0.3;
            s.connect(vb); vb.connect(vg); vg.connect(bus); s.start(t0 + when);
            this.loops.push(s); this.nodes.push(vb, vg);
            this.voiced = true;
          }
          break;
        }
        case 'down':
          a.play(a.or('handset_down', 'thudSoft'), { gain: 0.42, rate: a.lic('handset_down') ? 1 : 1.5, when, dest: bus });
          a.play('click', { gain: 0.55, when: when + 0.07, dest: bus });
          break;
        case 'end': break;
      }
    }
  }

  private lineStep(clock: number) {
    const plan = this.plan; if (!plan) return;
    const e = this.lineTime() - this.startedAt;
    while (this.capAt < plan.caps.length && plan.caps[this.capAt].at <= e) {
      const c = plan.caps[this.capAt++];
      this.shown.push(c.text);
      this.d.toast(c.text, c.secs, () => this.lineTime());
    }
    if (e >= plan.end) this.finish(clock);
  }

  private stopLine(fade = 0.04) {
    const c = this.ctx, bus = this.bus, loops = this.loops, nodes = this.nodes;
    this.bus = null; this.loops = []; this.nodes = [];
    if (!c) return;
    const t = c.currentTime;
    bus?.gain.setTargetAtTime(0, t, fade);
    for (const s of loops) { try { s.stop(t + fade * 8); } catch { /* stopped */ } }
    setTimeout(() => { for (const n of nodes) { try { n.disconnect(); } catch { /* gone */ } } }, Math.ceil(fade * 8000) + 120);
  }
  private putBack() {
    const h = this.handset(); if (h) { h.visible = true; h.rotation.z = 0; }
    this.d.audio.play('click', { at: this.phoneAt().clone(), gain: 0.5 });
  }

  /** Put down before the end (Use, or walked off the end of the cord): as if it had not been heard;
   *  it rings again the next time the player comes to the office before room 6 is over. */
  hangUp(text?: string) {
    if (this.state !== 'line') return;
    this.stopLine();
    this.putBack();
    this.state = 'idle'; this.result = 'away'; this.tail = TAIL;
    if (text) this.d.toast(text, 3);
  }

  private finish(clock: number) {
    const plan = this.plan!;
    this.stopLine(0.3);
    this.putBack();
    this.state = 'idle'; this.result = 'heard'; this.tail = TAIL;
    const seen = this.d.seen;
    seen.add('ev:office'); seen.add(`office@${Math.round(clock)}`);
    this.d.toast('You put the handset back on its hook.', 3.2);
    this.d.note(this.noteText(clock, plan));
    this.d.refresh();
    this.d.save();
  }

  /** When the player answered the hut's telephone: the clock on chapter three's note of it. */
  private answeredAt(): string | null {
    for (const n of this.d.notes()) { const m = /^(\d\d:\d\d)\. N\. Vega called the field telephone/.exec(n); if (m) return m[1]; }
    return null;
  }
  private noteText(clock: number, plan: LinePlan) {
    let n = `${clockText(clock, false)}. The crank telephone in the motel office, on the line to the hut at STATION 01. Nobody on it. Someone walking on the boards, the hut door.`;
    if (plan.gap !== null) {
      n += plan.delta !== null && plan.delta <= REAL ? ' A shutter, then another, as far apart as FRAME 03 and FRAME 04.'
        : ' A shutter, a wait, then another: the wait between FRAME 03 and FRAME 04.';
      n += ' I took two frames out there.';
    }
    const at = this.answeredAt();
    n += ` Then the telephone in the hut rang, and someone picked it up and listened to N. Vega. That was me${at ? `, at ${at}` : ''}.`;
    // (what Reyes knows by now: P09 has the hut's receiver giving back T. Vega's correction; that
    // the line itself does, P12, is room 6's to show)
    n += ' The receiver out there gives back T. Vega\'s correction from 1947. This line has kept me.';
    return n;
  }

  /** Heard: lifted again, it has the carrier on it and nothing else. */
  private listenAgain() {
    const a = this.d.audio, ctx = a.ctx, at = this.phoneAt().clone();
    a.play('click', { at, gain: 0.5 });
    if (ctx && a.noiseBuffer()) {
      const line = a.phoneLine(), t = ctx.currentTime;
      const n = ctx.createBufferSource(); n.buffer = a.noiseBuffer()!; const ng = ctx.createGain(); ng.gain.value = 0.02;
      n.connect(ng); ng.connect(line.input); n.start(t); n.stop(t + 2.6);
      const o = ctx.createOscillator(); o.frequency.value = 760; const og = ctx.createGain();
      og.gain.setValueAtTime(0, t); og.gain.linearRampToValueAtTime(0.018, t + 0.6); og.gain.setValueAtTime(0.018, t + 2.2); og.gain.linearRampToValueAtTime(0, t + 2.6);
      o.connect(og); og.connect(line.input); o.start(t); o.stop(t + 2.7);
      a.play('click', { at, gain: 0.45, when: 2.6 });
      setTimeout(() => { for (const x of line.nodes) { try { x.disconnect(); } catch { /* gone */ } } }, 3200);
    }
    this.d.toast('The carrier, very weak, and nothing on it. You put the handset back.', 3.4);
  }

  // ---------- the board ----------
  private when() {
    for (const k of this.d.seen) if (k.startsWith('office@')) return clockText(Number(k.slice(7)), false);
    return null;
  }
  private cards(): BoardCard[] {
    if (!this.heard) return [];
    const fr = this.d.frames(), frames = !!(fr.f3 && fr.f4), t = this.when();
    return [{
      id: 'ol', stamp: 'OFFICE', title: 'THE MOTEL OFFICE, TONIGHT', kind: 'thing', have: true, x: 14, y: 820, w: 300,
      text: `The crank telephone in the office${t ? `, ${t}` : ''}, on the line to the hut at STATION 01. Nobody on it.`,
      lines: [
        { id: 'ol.boards', text: 'Steps on the boards in the hut, and the hut door. A carrier under them.' },
        ...(frames ? [{ id: 'ol.shutters', text: 'Outside, a shutter, then another.' }] : []),
        { id: 'ol.answer', text: 'The telephone in the hut rings. Someone picks it up, and listens.' },
      ],
    }];
  }
  private keepLinks() {
    for (const k of [...this.d.seen]) if (k.startsWith(LINK)) this.d.seen.delete(k);
    for (const k of this.links) this.d.seen.add(LINK + k);
  }

  // ---------- a night opened, the developer menu ----------
  /** A new night or a case opened: quiet, on its hook, the threads the case has. */
  reset() {
    this.stopBell(); this.stopLine();
    const h = this.handset(); if (h) { h.visible = true; h.rotation.z = 0; }
    this.state = 'idle'; this.result = null; this.plan = null; this.armed = false; this.calm = 0; this.opened = true; this.tail = 0; this.was = false; this.rings = 0;
    this.links = [...this.d.seen].filter((k) => k.startsWith(LINK)).map((k) => k.slice(LINK.length)).filter((k) => RULES.some((r) => pair(r.a, r.b) === k));
  }
  /** It rings now (the developer menu, the tests), wherever the player is. */
  ringNow() { if (this.state === 'idle') { this.armed = false; this.ring(); return true; } return false; }
}
