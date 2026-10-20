import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Interaction } from '../core/Interaction';
import type { DocSpec } from '../ui/UI';
import type { RecordsAnnex } from '../world/Annex';
import type { Collider } from '../world/ControlRoom';
import type { BoardCard } from '../ui/Board';
import type { ArrayTip } from './archiveDishes';
import type { Reel047, ReelCue } from './reel047';
import type { Stage2 } from './Chapter2';
import { RuleBook, pair, type BookNote, type BookRule } from './boardRules';
import { facing } from './seen';
import { clockText, pad } from './time';
import { artUrl } from '../core/art';
import { printout } from './Prologue';

// Chapter two gets one peak in the archive: the phone from the future, a second time (FORSLAG.md,
// 20 October, item 5 with Valg for Tom 1, which takes its place; Tom: "Spenning og skummelt slår
// faste regler", so the scarier one is built, with what of item 5 makes it stronger).
//
//   the build     the light steps on the roof (story/roofSteps.ts) come first; this never comes while
//                 they go, nor within BUILD seconds after them (one thing at a time, kept apart).
//   the call      in the records room, at its last quiet moment (P05 recorded, the key holder next;
//                 or while still reading, a while after the steps): the call recorder on the
//                 switchboard puts down a line by itself, IN TRK1 EXT 201, and the wall phone rings,
//                 and through the wall the empty control room's, on the same line. Or, if the player
//                 goes to the wall phone first to call the key holder: no dial tone; the line is
//                 already open. Either way, on it: this room, before it happens. The room's hum and
//                 the tired tube ticking, a thump through concrete, a drawer rolling out on its
//                 runners and slamming, something heavy falling, paper. Then the line is dead.
//   the 47        from the line's death, as at 02:14 (the prologue's grammar, Prologue.startCountdown):
//                 the corner clock in seconds; at once K3 through the wall (it trips before, not after:
//                 bank 3 is out, and there are 47 seconds to put it back); at :16 the beep; at :31 the
//                 call recorder prints again by itself, the night's first call once more, now with
//                 where it came from: EXT 201, the control room, to EXT 201 (R5: an imprint of it);
//                 at :40 the tube that has flickered all night goes out.
//   :47           it happens. A thump up through the floor: every drive in the array starts at once
//                 (story/archiveDishes.ts), and they all tip down from 32 to the horizon along 026.
//                 The drawer from the phone rolls out of the wall behind the STATION 01 box and throws
//                 the box on the floor. The printer in the control room runs by itself: with bank 3
//                 back in, the same solve as at 02:14, its old time on it (ns.solveAt); without, a
//                 blank sheet. The console log has the solve again at its old time.
//   after         the drawer stands out of the wall with reel 047 in it, NOT FOR CATALOGUE (the tape
//                 catalogue on the terminal goes 001-046, 048-1311). On the signal processor's deck
//                 (story/reel047.ts): the pulse group, a crank, a cut, 47 s of nothing, a relay, and
//                 -32 LY on the display. When the relay goes on the tape, the array goes back up to
//                 026 / 32, all at once ("reference motion observed", E10 will say of 02:17:47). Not
//                 played before the player leaves SARO, the dishes go back up unseen.
//
// The prologue's call stays the first; the note says this one has been heard before. Once in the
// night, chapter two's one 47-second event: the curve keeps the chapter's 47 for it (main.ts, keep)
// and it lands the peak (story/curve.ts, FELT 'archive'). No save from the ring until the dishes are
// down; a save after has the drawer out, the box on the floor and the dishes where they are, at rest.
// Kept in the town's part of the case (`arc:` keys). No voices: captions and code-made or recorded
// sounds (the bought ones through Audio.or, the game's own without them).

export type ArcPhase = 'idle' | 'ringing' | 'line' | 'count' | 'moment' | 'done';
type Where = 'records' | 'annex' | 'room' | 'out' | 'away';

export interface ArcIn {
  /** Chapter two at the records stage or ready for the key holder, not stood aside (tests). */
  ok: boolean;
  stage: Stage2 | null;
  /** A record has been read. */
  read: boolean;
  /** Nothing open, no cinematic, no call on a line. */
  quiet: boolean;
  /** Nothing else of the night's is going (one at a time). */
  free: boolean;
  /** The steps on the roof are going / have been. */
  roofBusy: boolean; roofDone: boolean;
  where: Where;
  /** A paper or the notebook open, a cinematic: the 47 holds, as the clock does. */
  hold: boolean;
  clock: number;
  cam: THREE.Camera;
  /** In SARO's area. */
  saro: boolean;
  /** Somewhere the array could be seen from: out at SARO, or in the control room (its windows). */
  view: boolean;
  /** Bank 3 is in (powered, K3 closed). */
  bankIn: boolean;
  /** Chapter two has called the key holder (the recorder puts the call down). */
  called: boolean;
}

export interface ArcDeps {
  audio: AudioSys;
  annex: RecordsAnnex;
  inter: Interaction;
  seen: Set<string>;
  dishes: ArrayTip;
  deck: Reel047;
  /** SARO's colliders (the box on the floor is in the way). */
  colliders: Collider[];
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  save: () => void;
  /** A line in the console log at a clock (Prologue.consoleLog). */
  log: (text: string, at: number) => void;
  /** K3 goes (Prologue.relayClack): true if it tripped, false if it had nothing left to trip. */
  relay: () => boolean;
  at: { rack: () => THREE.Vector3; desk: () => THREE.Vector3; printer: () => THREE.Vector3; recPhone: () => THREE.Vector3; tube: THREE.Vector3 };
  cinematic: (on: boolean) => void;
  shake: (k: number) => void;
  /** Settings, Flashing images: slow fades. */
  calm: () => boolean;
  file: (doc: DocSpec) => void;
  open: (doc: DocSpec, onClose: () => void) => void;
  /** The night's solve (ns.solveAt), the prologue's call (from the console log), Ward's call (notes). */
  solveAt: () => number | null;
  firstCall: () => number | null;
  wardCall: () => number | null;
  /** The control room's printer: the paper feeding out (k 0..1) with the solve or blank, or none. */
  paper: (kind: 'same' | 'blank' | null, k: number) => void;
  /** The case board, drawn again. */
  refresh: () => void;
  /** The signal processor can play (power, a tape locked tonight). */
  deckReady: () => string | null;
}

/** Seconds after the steps on the roof are over before the phone can ring (they are the build). */
const BUILD = 12;
/** Still reading: this long after the steps and settled in the room before it rings. */
const BUILD_READING = 50;
const QUIET = 2.5, RING_MAX = 40, TAIL = 30;
const COUNT = 47;
/** The tipping is over by then, whatever the dishes do (seconds from :47). */
const MOMENT_MAX = 32;
/** The call, from the pick-up (the lift adds LIFT before it). */
const CALL = { tube: 0.35, thump: 2.6, drawer: 3.1, roll: 0.55, fall: 4.05, hum: 4.6, end: 6.3 };
const LIFT = 1.7;
/** The box thrown off the shelf: pushed, tipping over the edge, falling, on the floor. */
const FALL = { push: 0.12, tip: 0.42, land: 0.86, settle: 1.0 };
const SHELF = { edge: -5.555, y: 1.135, z: 8.6, x0: -5.62 };
/** Where it ends: on its side on the floor in front of the shelf, its label to the room (sideways). */
const FLOOR = { x: -5.33, y: 0.17, z: 8.5, yaw: 0.34 };
const Q_FLOOR = new THREE.Quaternion().setFromEuler(new THREE.Euler(Math.PI / 2, FLOOR.yaw, 0, 'YXZ'));
const Q_TIP = new THREE.Quaternion().setFromEuler(new THREE.Euler(0, 0, -0.95));

const DAY = (c: number) => `04/14 ${clockText(c, false)}`;

// ---------- the board ----------
const NOTES: Record<string, BookNote> = {
  'n.r47.cut': { text: 'A cable through cutters, on a reel from 1979. The cable at STATION 01 was cut clean, on purpose.' },
  'n.r47.gap': { text: 'The 1947 log: the carrier ceased at 02:17:00, and 47 seconds later the relay, and the reference moved. Reel 047 has the same 47 seconds, and the same relay.' },
  'n.r47.her': { text: 'She broke the connection herself in 1947, with Tomás in the reference. Reel 047 has the cut on it, thirty-two years later.' },
  'n.r47.crank': { text: 'The telephone in the hut is rung by its handle. On reel 047 somebody rings one.' },
};
const RULES: BookRule[] = [
  { a: 'r47.cut', b: 'frame04', note: 'n.r47.cut', text: NOTES['n.r47.cut'].text },
  { a: 'r47.gap', b: 'e10.relay', note: 'n.r47.gap', text: NOTES['n.r47.gap'].text },
  { a: 'r47.gap', b: 'e10.carrier', note: 'n.r47.gap', text: NOTES['n.r47.gap'].text },
  { a: 'r47.cut', b: 'e12.cut', note: 'n.r47.her', text: NOTES['n.r47.her'].text },
  { a: 'r47.crank', b: 'ol.answer', note: 'n.r47.crank', text: NOTES['n.r47.crank'].text },
  // (thin: the player can do the sum or not)
  { a: 'r47.range', b: 'rx.dist', text: '-32 on reel 047, 10 August 1979. -39 on tonight\'s printout.' },
];
const LINK = 'a47:';

export class ArchivePhone {
  phase: ArcPhase = 'idle';
  /** Stand aside: never comes (the tests of other things; the chapter goes as it did before it). */
  off = false;
  /** Seconds into the phase (the 47's own seconds in 'count'). */
  t = 0;
  /** How it began: the phone rang and was answered, or the handset was lifted to dial. */
  how: 'ring' | 'lift' | null = null;
  /** Where the player was at :47. */
  at47: Where | null = null;
  /** The captions given since it began (tests). */
  shown: string[] = [];
  /** The beats as they came, with the 47's second they came at (tests). */
  beats: { what: string; t: number }[] = [];
  /** The corner clock with seconds while the 47 counts (main.ts, UI.clockSeconds). */
  clockShow: { hm: string; ss: string } | null = null;
  readonly book: RuleBook;
  private q = 0;               // quiet in the room, seconds
  private since = -1;          // seconds since the steps on the roof were over (-1: not yet)
  private inRoom = 0;          // seconds in the room since the steps
  private wasRoof = false;
  private rings = 0;
  private lamp = 0;
  private ringHandles: { src: AudioScheduledSourceNode; gain: GainNode }[] = [];
  private lineNodes: AudioNode[] = [];
  private lineSrcs: AudioScheduledSourceNode[] = [];
  private cues: { at: number; fn: () => void }[] = [];
  private deadAt = 0;          // the night's clock when the line went dead
  private momentAt = 0;        // and at :47
  private tail = 0;
  private boxCol: Collider = { minX: -5.55, maxX: -5.1, minZ: 8.45, maxZ: 8.82 };
  private fallT = -1;
  private printT = -1;
  private printKind: 'same' | 'blank' = 'blank';
  private links: string[] = [];
  private watchUp = false;

  constructor(private d: ArcDeps) {
    const a = d.annex.arc;
    d.inter.add({ id: 'arc:drawer', object: a.hit.drawer, range: 2.6,
      label: () => !this.drawerOut ? null : this.has('arc:reel') ? 'Fire drawer' : 'Take reel 047',
      use: () => this.useDrawer(),
      examine: () => !this.drawerOut ? null : this.has('arc:reel') ? 'The fire drawer, out of the wall on its rusted runners. Empty now.'
        : 'A steel fire drawer let into the wall behind the shelf, out on its runners. Rust on the runners. In the tray, a reel of tape in a grey box: 047.' });
    d.inter.add({ id: 'arc:recorder', object: a.hit.recorder, range: 2.4,
      label: () => this.shownOk ? 'Call recorder' : null,
      use: () => this.readStrip(),
      examine: () => this.shownOk ? 'The call recorder on its bracket under the switchboard: a strip printer that puts down a line for every call on SARO\'s lines.' : null });
    d.inter.add({ id: 'arc:ksu', object: a.hit.ksu, label: () => null, use: () => {}, look: true, examineRange: 4,
      examine: () => this.shownOk ? 'SARO\'s key service unit: lamps for the outside line and the supervisor line, and a typed card. EXT 201 CONTROL ROOM, EXT 214 RECORDS, EXT 220 OPERATIONS, EXT 230 PHOTO LAB.' : null });
    this.book = new RuleBook({
      key: 'ch2',
      owns: (n) => n.startsWith('r47.') || n === 'r47',
      notes: NOTES, rules: RULES, questions: [],
      links: () => this.links,
      wrong: () => {},
      save: () => { this.keepLinks(); this.d.save(); },
      cards: () => this.cards(),
      empty: 'Those two do not say anything about each other.',
    });
  }

  // ---------- what main.ts asks ----------
  private has(k: string) { return this.d.seen.has(k); }
  /** Not had yet tonight: chapter two's 47 is kept for it (story/curve.ts, keep). */
  get pending() { return !this.off && !this.has('arc:call'); }
  /** Going: nothing else of the night's starts (one at a time), no save, the music holds. */
  get busy() { return this.phase === 'ringing' || this.phase === 'line' || this.phase === 'count' || this.phase === 'moment' || this.d.deck.playing; }
  /** The peak's own thing (the curve's rest counts from when it is over). */
  get peaking() { return this.busy; }
  /** No save now: the 47 is a moment, and the dishes are moving or the reel is on. */
  get racing() { return this.busy || this.d.dishes.moving; }
  /** A few seconds after it, nothing else starts either (the steps on the roof). */
  get resting() { return this.tail > 0; }
  /** The drawer stands out of the wall. */
  get drawerOut() { return this.has('arc:done'); }
  /** The records room's things have something to say (the annex is shown in chapter two on). */
  private shownOk = false;
  /** The reel is in the kit (taken, not yet heard / heard). */
  get reel(): 'none' | 'kit' | 'heard' { return this.has('arc:heard') ? 'heard' : this.has('arc:reel') ? 'kit' : 'none'; }

  // ---------- per frame ----------
  update(dt: number, s: ArcIn) {
    this.shownOk = s.stage !== null || this.has('arc:done') || this.has('arc:call');
    this.d.dishes.muffled = s.where !== 'out';
    if (this.tail > 0) this.tail = Math.max(0, this.tail - dt);
    // the steps on the roof are the build: count from when they are over
    if (s.roofBusy) this.since = -1;
    else if (this.wasRoof || (s.roofDone && this.since < 0)) { this.since = 0; this.inRoom = 0; }
    this.wasRoof = s.roofBusy;
    if (this.since >= 0) this.since += dt;
    // (the cues run on the event's own time: they hold with the 47, a paper open over it)
    if (!(this.phase === 'count' && s.hold)) this.cueT += dt;
    while (this.cues.length && this.cues[0].at <= this.cueT) this.cues.shift()!.fn();
    switch (this.phase) {
      case 'idle': this.idle(dt, s); break;
      case 'ringing': this.ringing(dt, s); break;
      case 'line': this.t += dt; if (this.t >= this.lineEnd) this.dead(s); break;
      case 'count': if (!s.hold) { this.t += dt; this.count(s); } break;
      case 'moment': this.t += dt; this.moment(dt, s); break;
      case 'done': this.found(s); break;
    }
    this.fallStep(dt);
    this.printStep(dt);
    this.arrayStep(dt, s);
    if (this.d.deck.playing) this.d.deck.update(dt);
  }

  /** Not there when it came: the first time back in the records room after it, what is left. */
  private found(s: ArcIn) {
    if (s.where !== 'records' || this.has('arc:found') || !this.has('arc:done') || this.phase === 'moment') return;
    this.d.seen.add('arc:found');
    this.say('The STATION 01 box is on the floor, its papers out of it. Behind where it stood, a steel drawer has come out of the wall.', 4.6);
    this.d.note(`${clockText(s.clock, false)}. The records room. The STATION 01 box is on the floor with its papers out of it. Behind where it stood on the shelf a steel drawer has come out of the wall: the drawer from the phone.`);
    this.d.save();
  }

  private idle(dt: number, s: ArcIn) {
    if (!this.pending || !s.ok || s.where !== 'records') { this.q = 0; return; }
    if (s.quiet && s.free && !s.roofBusy) this.q += dt; else this.q = 0;
    if (this.since >= 0) this.inRoom += dt;
    if (this.rings >= 2 || this.since < 0) return;
    // the room's last quiet moment (the key holder next), or still reading, a while after the steps
    const due = s.stage === 'call-nora' ? this.since >= BUILD && this.q >= QUIET
      : s.stage === 'records' && s.read && this.since >= BUILD_READING && this.inRoom >= BUILD_READING * 0.6 && this.q >= 4;
    if (due && (this.rings === 0 || this.since >= 90)) this.ring(s);
  }

  /** The wall phone (and the control room's): what main.ts and story/Chapter2.ts ask first. */
  phoneLabel(): string | null { return this.phase === 'ringing' ? 'Answer the phone' : null; }
  /** Used on a phone: answer it, or lift the wall phone to dial and find the line open. False: the
   *  chapter's own use of it. */
  phoneUse(where: 'room' | 'desk', s: ArcIn): boolean {
    if (this.phase === 'ringing') { this.answer(where); return true; }
    if (where !== 'room') return false;
    if (this.phase === 'line') return true;
    if (this.phase === 'count' || this.phase === 'moment') { this.say('The line is dead. Not even a dial tone.', 3); return true; }
    if (!this.pending || !s.ok) return false;
    // something on the roof: the line is dead (and the steps go on, not held for a call)
    if (s.roofBusy) { this.say('You lift the handset. The line is dead. Not even a dial tone.', 3.4); this.d.audio.play('click', { gain: 0.45, at: this.d.at.recPhone() }); return true; }
    // to call the key holder (or before): no dial tone, the line is already open
    if (s.stage === 'call-nora' || (s.stage === 'records' && s.roofDone && s.read)) { this.lift(s); return true; }
    return false;
  }

  // ---------- the ring ----------
  private ring(s: ArcIn) {
    this.phase = 'ringing'; this.t = 0; this.rings++; this.shown = [];
    this.printIn(s.clock);
    const a = this.d.audio, ctx = a.ctx;
    this.cue(1.1, () => {
      if (this.phase !== 'ringing' || !ctx) return;
      const room = a.play('phoneRing', { loop: true, gain: 1.0, at: this.d.at.recPhone() });
      // through the wall, the empty control room's, on the same line
      const desk = a.play('phoneRing', { loop: true, gain: 0.32, lp: 1300, at: this.d.at.desk(), rate: 0.985 });
      for (const h of [room, desk]) if (h) this.ringHandles.push(h);
    });
    this.cue(1.6, () => { if (this.phase === 'ringing') this.say('The wall phone. And through the wall, the control room\'s, on the same line.', 3.8); });
    this.say('By the wall phone, the call recorder prints a line. Nobody is on the phones.', 3.6);
  }
  private ringing(dt: number, s: ArcIn) {
    this.t += dt;
    this.lamp += dt;
    this.d.annex.arc.lamp('trk1', Math.floor(this.lamp * 2.4) % 2 ? 1 : 0.15);
    if (this.t > RING_MAX || (s.where !== 'records' && s.where !== 'annex' && s.where !== 'room')) {
      this.stopRing();
      this.phase = 'idle'; this.since = 0.001; this.q = 0;
      this.d.annex.arc.lamp('trk1', 0);
      if (s.where === 'records' || s.where === 'annex' || s.where === 'room') this.say('The ringing stops.', 2.4);
    }
  }
  private stopRing() {
    const ctx = this.d.audio.ctx;
    for (const h of this.ringHandles) { if (!ctx) continue; h.gain.gain.setTargetAtTime(0, ctx.currentTime, 0.02); try { h.src.stop(ctx.currentTime + 0.15); } catch { /* stopped */ } }
    this.ringHandles = [];
  }

  // ---------- the call ----------
  private lineEnd = CALL.end + 0.1;
  private answer(where: 'room' | 'desk') {
    this.stopRing();
    this.how = 'ring';
    this.d.audio.play('click', { gain: 0.6, at: where === 'room' ? this.d.at.recPhone() : this.d.at.desk() });
    this.call(0);
  }
  private lift(s: ArcIn) {
    this.how = 'lift'; this.rings = Math.max(this.rings, 1); this.shown = [];
    this.d.audio.play('click', { gain: 0.5, at: this.d.at.recPhone() });
    this.printIn(s.clock);
    this.say('You lift the handset to dial. No dial tone. The line is already open.', 3.2);
    this.call(LIFT);
  }
  private call(lead: number) {
    this.phase = 'line'; this.t = 0; this.lineEnd = lead + CALL.end + 0.1;
    this.d.seen.add('arc:call');
    if (this.how === 'lift') this.d.seen.add('arc:lift');
    this.d.annex.arc.lamp('trk1', 1);
    this.d.cinematic(true);
    this.lineSound(lead);
    const cap = (at: number, text: string, secs: number) => this.cue(at, () => this.say(text, secs));
    cap(lead + 0.3, 'Static. Then a room, very quiet. A fluorescent tube ticks somewhere in it.', 2.6);
    cap(lead + CALL.thump, 'A deep thump, through concrete.', 1.6);
    cap(lead + CALL.drawer + 0.05, 'A drawer rolls out on its runners, all the way, and slams.', 1.9);
    cap(lead + CALL.fall + 0.05, 'Something heavy falls. Paper, all over the floor.', 2.4);
  }
  /** What the line has: the records room before it happens. */
  private lineSound(lead: number) {
    const a = this.d.audio, ctx = a.ctx, noise = a.noiseBuffer();
    if (!ctx || !noise) return;
    const line = a.phoneLine(), t0 = ctx.currentTime + lead, end = t0 + CALL.end;
    this.lineNodes.push(...line.nodes);
    const src = (b: AudioBuffer, gain: number, f?: { type: BiquadFilterType; freq: number }) => {
      const s = ctx.createBufferSource(); s.buffer = b; s.loop = true;
      const g = ctx.createGain(); g.gain.value = gain;
      if (f) { const bq = ctx.createBiquadFilter(); bq.type = f.type; bq.frequency.value = f.freq; s.connect(bq); bq.connect(g); this.lineNodes.push(bq); } else s.connect(g);
      g.connect(line.input); s.start(t0 - lead); s.stop(end + 0.05);
      this.lineSrcs.push(s); this.lineNodes.push(g);
      return g;
    };
    // the line's own hiss from the lift; the room under it once it opens
    src(noise, 0.04);
    const room = src(noise, 0, { type: 'lowpass', freq: 520 });
    room.gain.setValueAtTime(0, t0); room.gain.linearRampToValueAtTime(0.32, t0 + CALL.tube + 0.3);
    // the tube's ballast and its ticks, the way the tired one goes
    const hum = ctx.createOscillator(); hum.frequency.value = 240;
    const hg = ctx.createGain(); hg.gain.setValueAtTime(0, t0); hg.gain.linearRampToValueAtTime(0.012, t0 + 0.6);
    hum.connect(hg); hg.connect(line.input); hum.start(t0); hum.stop(end);
    this.lineSrcs.push(hum); this.lineNodes.push(hg);
    for (const w of [0.85, 1.7, 1.95, 3.0, 4.9, 5.5]) a.play('click', { gain: 0.09, rate: 2.5, when: lead + w, dest: line.input });
    // a thump through concrete: every brake in the array at once, heard down through the floor
    a.boom(0.55, line.input, lead + CALL.thump);
    a.play(a.or('door_heavy', 'doorMetal'), { gain: 0.45, rate: 0.42, lp: 900, when: lead + CALL.thump + 0.02, dest: line.input });
    // the drawer, and the box off the shelf
    drawerRoll(a, line.input, lead + CALL.drawer, 1, CALL.roll);
    boxFall(a, line.input, lead + CALL.fall, 1);
    // under it, something low begins to turn: the gears of the drives
    const gear = ctx.createOscillator(); gear.type = 'square'; gear.frequency.setValueAtTime(180, t0 + CALL.hum); gear.frequency.linearRampToValueAtTime(236, end);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 236; bp.Q.value = 3;
    const gg = ctx.createGain(); gg.gain.setValueAtTime(0, t0 + CALL.hum); gg.gain.linearRampToValueAtTime(0.06, end - 0.05); gg.gain.linearRampToValueAtTime(0, end);
    gear.connect(bp); bp.connect(gg); gg.connect(line.input); gear.start(t0 + CALL.hum); gear.stop(end + 0.02);
    this.lineSrcs.push(gear); this.lineNodes.push(bp, gg);
    // and the click: the line is dead
    a.play('click', { gain: 0.8, when: lead + CALL.end });
  }
  private releaseLine(fade = 0.05) {
    const ctx = this.d.audio.ctx, srcs = this.lineSrcs, nodes = this.lineNodes;
    this.lineSrcs = []; this.lineNodes = [];
    setTimeout(() => {
      for (const s of srcs) { try { s.stop(); } catch { /* stopped */ } try { s.disconnect(); } catch { /* gone */ } }
      for (const n of nodes) { try { n.disconnect(); } catch { /* gone */ } }
    }, ctx ? fade * 1000 + 400 : 0);
  }

  /** Click. The line is dead: and the 47 from now. */
  private dead(s: ArcIn) {
    this.releaseLine();
    this.d.cinematic(false);
    this.d.annex.arc.lamp('trk1', 0);
    this.phase = 'count'; this.t = 0; this.beats = []; this.beats47.clear();
    this.deadAt = Math.round(s.clock);
    this.d.seen.add(`arc@${this.deadAt}`);
    this.d.audio.play(this.d.audio.or('handset_down', 'thudSoft'), { gain: 0.5, at: this.d.at.recPhone() });
    this.say('Click. The line is dead.', 2.6);
    const first = this.d.firstCall();
    this.cue(2.4, () => this.say(`Like the call at ${first !== null ? clockText(first, false) : '02:14'}. Only this time the room was this one.`, 4.2));
    this.d.note(`${clockText(this.deadAt, false)}. The wall phone in the records room. No voice. This room, very quiet, with the tube ticking: a thump through concrete, a drawer rolling out on its runners and slamming, something heavy falling, paper. Then nothing. I have heard a call like this once before tonight, ${first !== null ? `at ${clockText(first, false)}, ` : ''}in the control room. Forty-seven seconds after that one the floor shook.`);
    this.d.log('STATION CLOCK  WWVB REFERENCE LOST', this.deadAt);
    // K3 at once: it trips before, not after (bank 3 out, and 47 seconds to put it back)
    const tripped = this.d.relay();
    this.d.audio.relayClack(this.d.at.rack(), s.where !== 'room');
    this.cue(0.5, () => this.say(s.where === 'room' ? 'On the rack, K3 trips.'
      : tripped ? 'Through the wall, in the empty control room: the relay. Bank 3 is out again.' : 'Through the wall, in the empty control room: the relay, with nothing left to trip.', 3.8));
    this.beat('relay');
    this.clock();
  }

  private count(s: ArcIn) {
    const t = this.t;
    this.clock();
    if (t >= 16 && !this.hasBeat('beep')) { this.beat('beep'); this.d.audio.beep(52, 2.8, 0.09, 0, 'sine'); this.d.audio.beep(1046, 0.05, 0.035, 0, 'sine'); }
    if (t >= 31 && !this.hasBeat('print')) { this.beat('print'); this.printAgain(s); }
    if (t >= 40 && !this.hasBeat('tube')) {
      this.beat('tube');
      this.d.annex.holdTubes('out');
      this.d.audio.play('click', { gain: 0.18, rate: 0.55, at: this.d.at.tube });
      this.d.audio.holdRoomTone(0.45, 0.3);
      if (s.where === 'records') this.say('The tube that has flickered all night goes out.', 3.2);
    }
    if (t >= COUNT) this.at47now(s);
  }
  private beats47 = new Set<string>();
  private beat(what: string) { this.beats.push({ what, t: Math.round(this.t * 10) / 10 }); this.beats47.add(what); }
  private hasBeat(what: string) { return this.beats47.has(what); }
  private clock() {
    const ss = Math.min(COUNT, Math.floor(this.t));
    this.clockShow = { hm: clockText(this.deadAt, false), ss: pad(ss) };
  }

  // ---------- :47 ----------
  private at47now(s: ArcIn) {
    const a = this.d.audio;
    this.phase = 'moment'; this.t = 0;
    this.momentAt = Math.round(s.clock);
    this.at47 = s.where;
    this.clockShow = { hm: clockText(this.deadAt, false), ss: '47' };
    this.d.seen.add('arc:done');
    // a thump up through the floor, and the building takes it
    a.boom(0.7);
    a.holdRoomTone(1, 0.2);
    this.d.shake(this.d.calm() ? 0.15 : 0.42);
    if (!this.d.calm()) this.d.annex.holdTubes('stutter'); else this.d.annex.holdTubes(null);
    for (const [x, z] of [[-3.0, 7.6], [-4.4, 9.4], [-5.2, 8.2]]) this.d.annex.sift(x, z, 26, 0.5, 1.4);
    // every drive at once, down to the horizon along 026
    const tipped = this.d.dishes.tip();
    if (tipped) { this.d.seen.add('arc:tip'); this.d.log('ARRAY  S-01 TO S-27 MOTION  NO COMMAND', this.momentAt); }
    this.d.log('STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47', this.momentAt);
    // the drawer from the phone, and the box off the shelf
    const da = this.d.annex.arc.drawerAt;
    const dest = a.spatial(da, 1.6);
    if (dest) { drawerRoll(a, dest, 0.12, 1.3, 0.5); }
    this.fallT = 0;
    // the printer in the control room, by itself
    this.printKind = s.bankIn ? 'same' : 'blank';
    this.d.seen.add(s.bankIn ? 'arc:in' : 'arc:out');
    const solve = this.d.solveAt();
    if (s.bankIn && solve !== null) this.d.log('SOLVE  RANGE NEGATIVE', solve);
    else if (!s.bankIn) this.d.log('PRINTER  FORM FEED', this.momentAt);
    this.cue(0.9, () => this.printStart());
    // what is said depends on where the player is
    const w = s.where;
    if (w === 'records') {
      this.cue(0.25, () => this.say('A thump, up through the floor. Every drive in the array has started at once.', 3.4));
      this.cue(1.5, () => this.say('Behind the STATION 01 box a drawer has rolled out of the wall. The box is on the floor.', 4.4));
    } else if (w === 'room') {
      this.cue(0.25, () => this.say('The floor shakes. Out in the array every drive starts at once, and the dishes come down.', 4));
      this.cue(1.5, () => this.say('The printer starts by itself. From the records room, a slam, and something heavy falling.', 4.4));
    } else if (w === 'annex') {
      this.cue(0.25, () => this.say('A thump, up through the floor. Every drive in the array has started at once.', 3.4));
      this.cue(1.5, () => this.say('In the records room, a slam, and something heavy falling.', 3.6));
    } else if (w === 'out') {
      this.cue(0.25, () => this.say('Every drive in the array starts at once. The dishes are coming down.', 3.8));
    }
    this.d.save();
  }
  private moment(_dt: number, s: ArcIn) {
    if (this.t >= 2.6 && this.clockShow) this.clockShow = null;
    if (this.t >= 2.6 && this.d.annex.tubeMode === 'out') this.d.annex.holdTubes(null);
    // over when the dishes are down (or never tipped), and the box and the paper have settled
    if ((this.d.dishes.down || !this.d.dishes.holds || this.t >= MOMENT_MAX) && this.t >= 3) this.finish(s);
  }
  private finish(s: ArcIn) {
    this.phase = 'done'; this.t = 0; this.tail = TAIL; this.clockShow = null;
    if (this.d.annex.tubeMode) this.d.annex.holdTubes(null);
    const hm = clockText(this.momentAt, false);
    const w = this.at47;
    if (w === 'records') {
      this.d.seen.add('arc:found');
      this.d.note(`${hm}. Forty-seven seconds after the line went dead, a thump came up through the floor and every drive in the array started at once. A drawer I never knew was there rolled out of the wall behind the STATION 01 box, the drawer from the phone, and threw the box on the floor.`);
    } else if (w === 'annex' || w === 'room') {
      this.d.note(`${hm}. Forty-seven seconds after the line went dead, the floor shook and every drive in the array started at once. In the records room something slammed and fell.`);
    } else if (w === 'out') {
      this.d.note(`${hm}. Forty-seven seconds after the line went dead, every drive in the array started at once, and they all came down out of 32 together.`);
    }
    if (this.d.dishes.down) this.d.log('ARRAY  27 DRIVES  AZ 026 EL 02  NO COMMAND', Math.round(s.clock));
    this.d.save();
  }

  // ---------- the box, the drawer ----------
  private fallStep(dt: number) {
    if (this.fallT < 0) return;
    const ft = (this.fallT += dt), A = this.d.annex, fb = A.objs.fieldBoxModel;
    const drawerK = THREE.MathUtils.clamp((ft - 0.12) / 0.5, 0, 1);
    A.arc.setDrawer(drawerK * drawerK * (3 - 2 * drawerK));
    // pushed off the shelf by the drawer's front as it comes, then over the edge and down
    const t = ft - 0.12 - 0.38;   // from when the drawer's front reaches the box
    if (t < 0) return;
    if (t < FALL.push) {
      fb.position.x = SHELF.x0 + 0.11 * (t / FALL.push);
    } else if (t < FALL.tip) {
      const k = (t - FALL.push) / (FALL.tip - FALL.push);
      this.tipPose(fb, -0.95 * k * k);
    } else if (t < FALL.land) {
      // off the edge: it turns over on its way down and lands on its side
      const k = (t - FALL.tip) / (FALL.land - FALL.tip);
      const from = this.tipAt(-0.95);
      fb.position.set(THREE.MathUtils.lerp(from.x, FLOOR.x, k), THREE.MathUtils.lerp(from.y, FLOOR.y, k * k), THREE.MathUtils.lerp(SHELF.z, FLOOR.z, k));
      fb.quaternion.slerpQuaternions(Q_TIP, Q_FLOOR, Math.min(1, k * 1.08));
    } else {
      if (this.fallT >= 0 && !A.arc.papers.visible) {
        A.arc.papers.visible = true;
        const dest = this.d.audio.spatial(new THREE.Vector3(FLOOR.x, 0.2, FLOOR.z), 1.6);
        if (dest) boxFall(this.d.audio, dest, 0, 1.3);
        for (const [x, z] of [[-5.1, 8.6], [-4.9, 8.8]]) A.sift(x, z, 10, 0.3, 0.4);
      }
      const k = Math.min(1, (t - FALL.land) / (FALL.settle - FALL.land));
      this.floorPose(fb, 0.08 * (1 - k));
      if (k >= 1) { this.fallT = -1; this.boxDown(); }
    }
  }
  /** The box tipping about the shelf's front edge, by angle a (radians, negative: forward). */
  private tipPose(fb: THREE.Object3D, a: number) {
    const p = this.tipAt(a);
    fb.position.set(p.x, p.y, SHELF.z); fb.rotation.order = 'YXZ'; fb.rotation.set(0, 0, a);
  }
  private tipAt(a: number) {
    // the box's origin is its bottom centre; it pivots on the edge, 0.045 behind its centre once pushed
    const off = (SHELF.x0 + 0.11) - SHELF.edge;
    return { x: SHELF.edge + off * Math.cos(a), y: SHELF.y + off * Math.sin(a) };
  }
  private floorPose(fb: THREE.Object3D, lift = 0) {
    fb.rotation.order = 'YXZ';
    fb.position.set(FLOOR.x, FLOOR.y + lift, FLOOR.z); fb.rotation.set(Math.PI / 2 - lift * 1.5, FLOOR.yaw, 0);
  }
  /** On the floor for good: its hit area there, and in the way. */
  private boxDown() {
    const A = this.d.annex;
    this.floorPose(A.objs.fieldBoxModel);
    A.objs.fieldBox.position.set(FLOOR.x, 0.2, FLOOR.z + 0.12);
    if (!this.d.colliders.includes(this.boxCol)) this.d.colliders.push(this.boxCol);
    A.arc.papers.visible = true;
  }
  private boxBack() {
    const A = this.d.annex, fb = A.objs.fieldBoxModel;
    fb.position.set(SHELF.x0, SHELF.y, SHELF.z); fb.rotation.order = 'XYZ'; fb.rotation.set(0, 0, 0);
    A.objs.fieldBox.position.set(-5.55, 1.28, 8.6);
    const i = this.d.colliders.indexOf(this.boxCol); if (i >= 0) this.d.colliders.splice(i, 1);
    A.arc.papers.visible = false;
  }

  private useDrawer() {
    if (!this.drawerOut) return;
    if (this.has('arc:reel')) { this.say('The fire drawer. Empty now.', 2.4); return; }
    this.d.seen.add('arc:reel');
    this.d.annex.arc.reel.visible = false;
    this.d.audio.play('paper0', { gain: 0.3, rate: 1.4, at: this.d.annex.arc.drawerAt });
    this.d.file(reelDoc());
    this.say('Reel 047, 10-AUG-79. NOT FOR CATALOGUE, in pencil. It fits the deck on the signal processor.', 4.4);
    this.d.note(`${clockText(this.nowClock, false)}. The drawer behind the STATION 01 box: a steel fire drawer let into the wall, shut so long its runners have rusted. In it, alone, a reel of tape. 047, 10-AUG-79. In pencil: NOT FOR CATALOGUE.`);
    this.d.save();
  }
  private nowClock = 0;

  // ---------- the call recorder ----------
  /** The recorder puts down the call coming in, at its clock: IN TRK1 EXT 201. */
  private printIn(clock: number) {
    this.d.seen.add(`arcin@${Math.round(clock)}`);
    this.chatter(() => this.strip());
  }
  /** At :31, by itself: the night's first call again, and where it came from. */
  private printAgain(s: ArcIn) {
    this.d.seen.add('arc:31');
    this.chatter(() => this.strip());
    if (s.where === 'records') this.say('The call recorder prints another line by itself. Nobody has called.', 3.4);
  }
  /** The recorder's print head, a line's worth (the dot matrix of the control room, small and quick). */
  private chatter(then: () => void) {
    const a = this.d.audio, at = this.d.annex.arc.recorderAt;
    a.play('printer', { gain: 0.26, rate: 1.75, at, dur: 1.05 });
    a.play('click', { gain: 0.14, rate: 1.9, at, when: 1.08 });
    this.cue(1.1, then);
  }
  /** The strip's lines, as the night has them (the newest at the bottom). */
  lines(): string[] {
    const out = ['CALL RECORD  SARO  KSU-12', '- - - - - - - - - - - - - -'];
    const first = this.d.firstCall(), ward = this.d.wardCall();
    const rec: [number, string][] = [];
    if (first !== null) rec.push([first, `${DAY(first)}  IN   TRK1  EXT 201  0:04`]);
    if (ward !== null) rec.push([ward, `${DAY(ward)}  IN   TRK2  EXT 201  0:11`]);
    for (const k of this.d.seen) {
      const m = /^arcin@(-?\d+)$/.exec(k);
      if (m) rec.push([+m[1], `${DAY(+m[1])}  IN   TRK1  EXT 201`]);
      const n = /^arcnora@(-?\d+)$/.exec(k);
      if (n) rec.push([+n[1], `${DAY(+n[1])}  OUT  TRK1  EXT 214  555 0119  0:14`]);
    }
    const night = (v: number) => { const c = ((v % 86400) + 86400) % 86400; return c >= 12 * 3600 ? c - 86400 : c; };
    rec.sort((x, y) => night(x[0]) - night(y[0]));
    // (the one it put down by itself at :31 goes in where it was printed: after the call that came at the ring)
    const again = this.has('arc:31') && first !== null ? `${DAY(first)}  IN   EXT 201  FROM EXT 201` : null;
    const ring = [...this.d.seen].map((k) => /^arcin@(-?\d+)$/.exec(k)).filter(Boolean).map((m) => +m![1]).sort((x, y) => night(x) - night(y)).pop();
    for (const [c, l] of rec) {
      out.push(l);
      if (again && ring !== undefined && c === ring) out.push(again);
    }
    return out;
  }
  private strip() { this.d.annex.arc.setStrip(this.lines()); }
  /** The strip as a paper, read off the recorder. */
  stripDoc(): DocSpec {
    const lines = this.lines(), again = this.has('arc:31');
    return {
      id: 'callstrip', title: 'Call recorder strip', kind: 'typed', stamp: 'CALL RECORD',
      page: lines.join('\n'),
      sheets: [{ page: lines.join('\n'), paper: artUrl('fanfold1986'), strip: true, term: true }],
      transcript: `The paper strip from the call recorder under the switchboard in the records room. It puts down a line for every call on SARO's lines: the date and the time, in or out, the line, the extension. The card on the switchboard: EXT 201 is the control room, EXT 214 the records room.${
        [...this.d.seen].some((k) => k.startsWith('arcin@')) ? '\n\nThe line IN TRK1 EXT 201 came out before the phone rang. Nobody was on the phones.' : ''}${
        again ? '\n\nThe line under it was printed by itself while that line was dead, a quarter of a minute before the floor shook: the night\'s first call again, the one that played the control room. This time the recorder has where it came from. EXT 201, to EXT 201.' : ''}`,
    };
  }
  private readStrip() {
    const doc = this.stripDoc();
    this.d.file(doc);
    this.d.open(doc, () => {
      if (this.has('arc:31') && !this.has('arc:strip')) {
        this.d.seen.add('arc:strip');
        const first = this.d.firstCall();
        this.d.note(`${clockText(this.nowClock, false)}. The call recorder's strip. The call at ${first !== null ? clockText(first, false) : '02:14'} is on it twice: once as it came in on the outside line, and once more, printed by itself tonight, from EXT 201 to EXT 201. The control room called itself.`);
        this.d.save();
      }
    });
  }

  // ---------- the printer in the control room ----------
  private printStart() {
    const a = this.d.audio, at = this.d.at.printer();
    this.printT = 0;
    if (this.printKind === 'same') a.play('printer', { gain: 0.8, at, dur: 5.4 });
    else a.play('printer', { gain: 0.32, rate: 0.72, at, dur: 1.7 });
  }
  private printStep(dt: number) {
    if (this.printT < 0) return;
    this.printT += dt;
    const len = this.printKind === 'same' ? 5.2 : 1.6;
    this.d.paper(this.printKind, Math.min(1, this.printT / len));
    if (this.printT >= len) this.printT = -1;
  }
  /** The control room's printer: a sheet hangs out of it (main.ts asks for the printer's label). */
  get sheet(): 'same' | 'blank' | null { return this.has('arc:done') && !this.has('arc:torn') ? (this.has('arc:in') ? 'same' : 'blank') : null; }
  printerLabel() { return this.sheet && this.printT < 0 ? 'Tear off the printout' : null; }
  printerUse(): boolean {
    const k = this.sheet;
    if (!k || this.printT >= 0) return false;
    this.d.seen.add('arc:torn');
    this.d.paper(null, 0);
    this.d.audio.play('thudSoft', { gain: 0.35, rate: 1.6, at: this.d.at.printer() });
    const doc = reprintDoc(k, this.d.solveAt(), this.momentAt || this.at47Clock());
    this.d.file(doc);
    this.d.open(doc, () => {
      const at = clockText(this.momentAt || this.at47Clock(), false), solve = this.d.solveAt();
      this.d.note(k === 'same'
        ? `${clockText(this.nowClock, false)}. The printer ran by itself at ${at}, with nobody at the console. It is the solve from ${solve !== null ? clockText(solve) : 'earlier tonight'}, line for line, its time on it too. Nobody ran a solve.`
        : `${clockText(this.nowClock, false)}. The printer ran by itself at ${at} and fed out one sheet with nothing on it. Not even the header. Bank 3 was out.`);
      this.d.save();
    });
    return true;
  }
  private at47Clock() { for (const k of this.d.seen) { const m = /^arc@(-?\d+)$/.exec(k); if (m) return +m[1] + COUNT; } return this.nowClock; }

  // ---------- the array ----------
  private arrayStep(dt: number, s: ArcIn) {
    this.nowClock = s.clock;
    const r = this.d.dishes.update(dt);
    if (r === 'home') {
      this.d.seen.add('arc:up');
      this.d.log('ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND', Math.round(s.clock));
      if (this.watchUp) this.d.seen.add('arc:upseen');
      this.watchUp = false;
    }
    const dishes = this.d.dishes;
    // seen, down: once
    if (dishes.holds && s.view && !this.has('arc:tipseen') && this.facingArray(s.cam)) {
      this.d.seen.add('arc:tipseen');
      this.d.note(`${clockText(s.clock, false)}. Every dish in the array has come down out of 32: all 27 on 026, tipped down to the horizon, as if something stood out there. Their red lamps blink together. Nobody sent a command.`);
    }
    if (dishes.mode === 'raising' && s.view && this.facingArray(s.cam)) this.watchUp = true;
    // nobody at SARO: they go back up unseen
    if (dishes.down && !s.saro) {
      dishes.home();
      this.d.seen.add('arc:up'); this.d.seen.add('arc:unseen');
      this.d.log('ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND', Math.round(s.clock));
      this.d.save();
    }
    // back at SARO, the first look at them after
    if (this.has('arc:unseen') && !this.has('arc:upnote') && s.saro && s.view && !dishes.holds && this.facingArray(s.cam)) {
      this.d.seen.add('arc:upnote');
      this.d.note(`${clockText(s.clock, false)}. The dishes are back up on 026 and 32, every one. I did not see them go.`);
    }
  }
  private nearest = new THREE.Vector3(-1, 9, -74);
  private facingArray(cam: THREE.Camera) {
    // the first rows of the array, from where the player is: any of three points across it
    for (const x of [-40, -1, 40]) { this.nearest.set(x, 9, -80); if (facing(cam, this.nearest, 420, 34)) return true; }
    return false;
  }

  // ---------- reel 047 on the signal processor ----------
  /** What the processor offers while the reel is in the kit (story/Decoder.ts asks first). */
  deckLabel(): string | null {
    if (this.d.deck.playing) return 'Signal processor (reel 047)';
    return this.reel === 'kit' ? 'Thread reel 047 on the deck' : null;
  }
  deckUse(): boolean {
    if (this.d.deck.playing) { this.say('Reel 047 is on the deck. Let it run.', 2.4); return true; }
    if (this.reel !== 'kit') return false;
    const why = this.d.deckReady();
    if (why) { this.say(why, 3.2); return true; }
    this.shown = [];
    let raised = false;
    this.d.deck.play((c: ReelCue) => {
      switch (c.kind) {
        case 'thread': this.say('You thread reel 047 on the deck and start it.', 2.6); break;
        case 'pulse': this.say('Tape hiss. Then the pulse group, four and seven, the way it came in tonight.', 3.8); break;
        case 'crank': this.say('A handle turned, and a bell: somebody ringing a field telephone.', 3.6); break;
        case 'cut': this.say('A snap, metal through metal. Like a cable going through cutters.', 3.4); break;
        case 'hole': this.cue(0.8, () => this.say('Then nothing. Not even the hiss. The reels go on turning.', 4)); break;
        case 'relay':
          this.say('A relay, loud and close, on the tape.', 2.8);
          // and out in the array, all at once, back up to 32 (E10: "relay impact and reference motion observed")
          if (this.d.dishes.raise()) {
            raised = true;
            this.d.log('ARRAY  S-01 TO S-27 MOTION  NO COMMAND', Math.round(this.nowClock));
            this.cue(1.2, () => this.say('Out in the array, every drive starts at once.', 3.2));
          }
          break;
        case 'range': this.say('On the processor\'s display, from the tape\'s data track: SOURCE DISTANCE -32 LY.', 4.4); break;
        case 'end': {
          this.d.seen.add('arc:heard');
          if (raised) this.d.seen.add('arc:raised');
          const hm = clockText(this.nowClock, false);
          this.d.note(`${hm}. Reel 047, 10 August 1979, on the signal processor. The pulse group, four and seven. A crank, like a field telephone being rung by its handle. A cut, metal through metal. Then nothing, not even the hiss, for 47 seconds. Then a relay, close. The data track ends on a solve: -32 LY.${raised ? ' When the relay went on the tape, the array went back up, all 27 together, to 026 and 32.' : ''}`);
          this.d.refresh();
          this.d.save();
          break;
        }
        default: break;
      }
    });
    return true;
  }

  // ---------- the board ----------
  books() { return this.has('arc:heard') ? [this.book] : []; }
  private cards(): BoardCard[] {
    if (!this.has('arc:heard')) return [];
    return [{
      id: 'r47', doc: 'reel047', stamp: 'REEL 047', title: 'COMMISSIONING, 10-AUG-79', kind: 'thing', have: true, w: 300,
      text: 'From the fire drawer behind the STATION 01 box. NOT FOR CATALOGUE.',
      lines: [
        { id: 'r47.pulse', text: 'The pulse group, four and seven, as on tape B tonight.' },
        { id: 'r47.crank', text: 'A handle turned, and a bell: a field telephone being rung.' },
        { id: 'r47.cut', text: 'A snap of metal through metal: a cable through cutters.' },
        { id: 'r47.gap', text: 'Then nothing, not even the hiss, for 47 seconds. Then a relay.' },
        { id: 'r47.range', text: 'On the data track: SOURCE DISTANCE -32 LY.' },
      ],
    }];
  }
  private keepLinks() {
    for (const k of [...this.d.seen]) if (k.startsWith(LINK)) this.d.seen.delete(k);
    for (const k of this.links) this.d.seen.add(LINK + k);
  }

  // ---------- small things ----------
  private say(text: string, secs = 3.2) { this.shown.push(text); this.d.toast(text, secs); }
  private cue(after: number, fn: () => void) { this.cues.push({ at: this.cueT + after, fn }); this.cues.sort((a, b) => a.at - b.at); }
  private cueT = 0;

  // ---------- a night opened, the developer menu, the tests ----------
  /** Stop at once, everything as the night has it (a new night, another save, the title). */
  halt() {
    this.stopRing();
    this.releaseLine(0);
    if (this.phase === 'line') this.d.cinematic(false);
    if (this.phase === 'count' || this.phase === 'moment') this.d.audio.holdRoomTone(1, 0.1);
    this.d.deck.stop();
    this.cues = []; this.clockShow = null; this.fallT = -1; this.printT = -1;
    if (this.d.annex.tubeMode) this.d.annex.holdTubes(null);
    this.d.annex.arc.lamp('trk1', 0); this.d.annex.arc.lamp('trk2', 0);
    this.beats47.clear();
    this.phase = 'idle'; this.t = 0; this.how = null; this.rings = 0; this.q = 0; this.since = -1; this.inRoom = 0; this.wasRoof = false; this.tail = 0;
  }
  /** A night put back from the town's part of the case: the drawer, the box, the strip, the paper
   *  in the printer, the dishes, the papers in the journal. A night saved in the middle of it (it
   *  cannot be: no save while it goes) is put back as if it had not begun. */
  restore() {
    this.halt();
    const s = this.d.seen;
    if (s.has('arc:call') && !s.has('arc:done')) { s.delete('arc:call'); s.delete('arc:lift'); for (const k of [...s]) if (/^arc@/.test(k)) s.delete(k); }
    const done = s.has('arc:done');
    this.phase = done ? 'done' : 'idle';
    const A = this.d.annex;
    A.arc.setDrawer(done ? 1 : 0);
    A.arc.reel.visible = !s.has('arc:reel');
    if (done) this.boxDown(); else this.boxBack();
    this.strip();
    // (only a sheet of its own: the prologue has the printer's paper until then)
    if (this.sheet) this.d.paper(this.sheet, 1);
    // the dishes: down and at rest if they were saved down, else as the night left them
    this.d.dishes.restore(s.has('arc:tip') && !s.has('arc:up'));
    this.links = [...s].filter((k) => k.startsWith(LINK)).map((k) => k.slice(LINK.length)).filter((k) => RULES.some((r) => pair(r.a, r.b) === k));
    if (s.has('arc:reel')) this.d.file(reelDoc());
    if (s.has('arc:torn')) this.d.file(reprintDoc(s.has('arc:in') ? 'same' : 'blank', this.d.solveAt(), this.at47Clock()));
    if (s.has('arc:strip')) this.d.file(this.stripDoc());
  }
  /** The recorder puts down the call to the key holder (main.ts, when chapter two has made it). */
  calledOut(clock: number) {
    if ([...this.d.seen].some((k) => k.startsWith('arcnora@'))) return;
    this.d.seen.add(`arcnora@${Math.round(clock)}`);
    this.chatter(() => this.strip());
  }
  /** Now, whatever the waiting (the developer menu, the tests): the phone rings, or the line opens. */
  now(how: 'ring' | 'lift', s: ArcIn) {
    if (this.phase !== 'idle' || !this.pending || s.where !== 'records') return false;
    if (how === 'ring') this.ring(s); else this.lift(s);
    return true;
  }
}

// ---------- the papers ----------
function reelDoc(): DocSpec {
  return {
    id: 'reel047', title: 'Reel 047 / The box label', kind: 'typed', stamp: 'REEL 047',
    page: 'SARO  RAW TAPE\nREEL  047\nDATE  10-AUG-79\nFREQ  1420.405 MHz\nSURVEY / COMMISSIONING\n\n~ NOT FOR CATALOGUE',
    transcript: 'A grey reel box from the fire drawer behind the STATION 01 box. The label is typed like every other reel SARO keeps. The pencil under it is not Dale\'s.\n\nThe tape catalogue on the operations terminal goes from 046 to 048.',
  };
}
function reprintDoc(kind: 'same' | 'blank', solve: number | null, printed: number): DocSpec {
  if (kind === 'blank') return {
    id: 'reprint', title: 'A blank sheet from the printer', kind: 'printout', page: '',
    transcript: `Fan-fold paper from the printer by RX bank 3, torn off below the perforation. The printer ran by itself at ${clockText(printed, false)} and fed it out.\n\nNothing is printed on it. Not even the header.`,
  };
  // (the night's own printout, the same lines, its time with them: Prologue.printout)
  return {
    ...printout(solve ?? 0), id: 'reprint', title: 'Direction solve printout, again',
    transcript: `Fan-fold paper, still warm. The printer ran by itself at ${clockText(printed, false)}, with nobody at the console.\n\nIt is the solve from earlier tonight, line for line, down to the time on it: ${clockText(solve ?? 0)}.`,
  };
}

// ---------- the sounds ----------
/** A steel drawer rolling out on its runners and slamming at the end of its travel. */
function drawerRoll(a: AudioSys, dest: AudioNode, when: number, k = 1, dur = 0.55) {
  const ctx = a.ctx, noise = a.noiseBuffer(); if (!ctx || !noise) return;
  const t = ctx.currentTime + when;
  // the runners: a band of noise rippling with the ball bearings, quicker as it goes
  const n = ctx.createBufferSource(); n.buffer = noise; n.loop = true;
  const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 1.6;
  bp.frequency.setValueAtTime(300, t); bp.frequency.linearRampToValueAtTime(640, t + dur);
  const am = ctx.createGain(); am.gain.value = 0.55;
  const lfo = ctx.createOscillator(); lfo.frequency.setValueAtTime(16, t); lfo.frequency.linearRampToValueAtTime(48, t + dur);
  const lg = ctx.createGain(); lg.gain.value = 0.45; lfo.connect(lg); lg.connect(am.gain);
  const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.32 * k, t + 0.07); g.gain.linearRampToValueAtTime(0.5 * k, t + dur); g.gain.setTargetAtTime(0, t + dur, 0.015);
  n.connect(bp); bp.connect(am); am.connect(g); g.connect(dest);
  n.start(t, Math.random() * 2); n.stop(t + dur + 0.2); lfo.start(t); lfo.stop(t + dur + 0.2);
  setTimeout(() => { for (const x of [n, bp, am, lfo, lg, g]) { try { x.disconnect(); } catch { /* gone */ } } }, (when + dur + 1) * 1000);
  // the slam: steel on steel, and the wall taking it; a rattle after
  a.play(a.or('door_heavy', 'doorMetal'), { dest, gain: 0.75 * k, rate: 1.35, lp: 4200, when: when + dur });
  a.play('thudSoft', { dest, gain: 0.55 * k, rate: 1.1, when: when + dur + 0.005 });
  a.play('click', { dest, gain: 0.3 * k, rate: 0.7, when: when + dur + 0.09 });
}
/** A heavy archive box off a shelf onto the floor, and its papers going. */
function boxFall(a: AudioSys, dest: AudioNode, when: number, k = 1) {
  a.play('thudSoft', { dest, gain: 0.95 * k, rate: 0.6, lp: 2400, when });
  a.play('thudSoft', { dest, gain: 0.4 * k, rate: 0.9, when: when + 0.11 });
  a.play('paper0', { dest, gain: 0.55 * k, rate: 0.85, when: when + 0.04 });
  a.play('paper1', { dest, gain: 0.4 * k, rate: 1.1, when: when + 0.3 });
  a.play('click', { dest, gain: 0.2 * k, rate: 0.6, when: when + 0.18 });
}
