import * as THREE from 'three';
import type { Dish, DishArray } from '../world/Dish';
import type { AudioSys } from '../core/Audio';
import { facing } from './seen';
import { clockText } from './time';

// Put the array back. It goes back (FORSLAG.md, 20 October, item 12; Tom: "Spenning og skummelt
// slår faste regler"). The night watchman fails at the real job, with his own hands (R1). Since
// 02:15 every dish has stood at az 026, el 32, with no command (canon). In chapter two Ward calls
// back: "Put the array back on the schedule and tell me it stays there." (story/Chapter2.ts). At the
// operations terminal on the west desk (story/nightshift.ts) ARRAY RESUME puts it back:
//
//   slew     the contactors go one after another out across the field, the console's motor hum of
//            02:15 (AudioSys.startMotors), and all 27 turn to the night's schedule, az 042, el 48
//            (story/field.ts, SCHED; the status screen's SCHEDULE line). It obeys. The brakes set,
//            the hum stops, ON SCHEDULE in the console log, and on the status screen a count.
//   hold     47 seconds on the schedule. Nothing happens, except that the red lamps on the feeds,
//            each blinking in its own time, slide into one blink across the whole field; towards
//            the end the crickets stop.
//   signal   at 47 every light out in the field goes out at once, the lamps on the posts, their
//            floods and the red lamps on the feeds; then four flashes of the red lamps, all of them
//            together, a gap, and seven (the pulse group of 02:14, slowed so it can be counted),
//            each flaring round its dish and lighting the near ones red. In the control room the
//            status console beeps with them; out of doors each comes with the signal's deep thump.
//            Then the field's lamps are back.
//   back     and all 27 turn back to 026 / 32 at the pace of 02:15, in perfect step and without a
//            sound: no contactor, no motor, nothing on the drive bus (DRIVE CURRENT 0.0 A in the
//            log), the red lamps steady, not blinking. Home, every brake at once.
//
// Every time the player tries. Each try is kept with Night Shift's part of the case
// (NightShiftState.array): the console log has it, S-03's motor bus has it in its own log (the S-03
// history, Chapter1.docS03), the notebook has it, and in chapter five Ward has the count
// (Chapter5.ts). Before Ward has asked (chapter one, chapter two before her second call) the array is
// held for the supervisor.
//
// One thing at a time (main.ts, oneAtATime): it does not start while one of the night's things is
// going (the console says ARRAY LINK BUSY), nor while the dishes move by themselves (DRIVES IN
// MOTION), and while it runs nothing else starts. The night's curve lands its peak on it
// (story/curve.ts, FELT) and does not start its rest over when it comes in one (AGAIN). If the
// warning of the time away comes anyway (the developer menu), the array leaves the schedule for it:
// the dishes the warning takes (story/dishesFollow.ts) are the warning's, the rest turn back to 026 at
// once. Saves wait for it (main.ts, saveBlock); a case kept in the middle of it comes back with the
// array at 026, the try counted and the log written (restore). Like everything in the night it waits
// while a panel is open (main.ts gives it no time then). Main.ts calls update() every frame of play,
// before the array is drawn.

export const HOME = { az: 26, el: 32 };
export const SCHED = { az: 42, el: 48 };
/** Seconds on the schedule before it goes. */
export const HOLD = 47;
const ARM = 0.7;                                       // from the command to the first contactor
const STAGGER = 1.2;                                   // the contactors out across the field
const RATE = { az: 2.2, el: 1.32, accAz: 1.0, accEl: 0.7 };   // 02:15's pace (Prologue.turnDishes, slew 2.2)
const SYNC: [number, number] = [6, 34];                // the lamps slide into one blink, seconds into the hold
const HUSH = 37;                                       // and the crickets stop
// the four and the seven: dark first, the flashes, a gap, dark again before they turn
const DARK0 = 1.6, SLOT = 0.58, ON = 0.3, GAP = 0.9, TAIL = 1.0;
const SEVEN = DARK0 + 4 * SLOT + GAP;
/** How long the lamps' four and seven take, from the 47th second to the turn. */
export const SIGNAL = SEVEN + 7 * SLOT + TAIL;
const SETTLE = 2.6;                                    // home: the crickets back, then it is over
// a flash: brighter, and flared to a halo wider than the dish, so that from behind it (from SARO's
// side, where the reflectors hide the lamps on their feeds) the rim stands out red against it
const FLASH = 1.9, FLARE = 1.5, STEADY = 1.0;
const WORDS = ['no', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten', 'eleven', 'twelve'];

/** How many times, in words: once, twice, three times. */
export function times(n: number) { return n === 1 ? 'once' : n === 2 ? 'twice' : `${WORDS[n] ?? n} times`; }
const ORD = ['', 'first', 'second', 'third', 'fourth', 'fifth', 'sixth', 'seventh', 'eighth', 'ninth', 'tenth'];
const ordinal = (n: number) => ORD[n] ?? `${n}th`;

export type ArrayStage = 'idle' | 'armed' | 'slew' | 'hold' | 'signal' | 'back' | 'settle';
/** Why the console will not do it now: before 02:15 (nothing to resume), held for the supervisor,
 *  the dishes moving by themselves or tipped down by the archive's phone, or one of the night's
 *  things going (one at a time). */
export type ArrayGate = 'early' | 'release' | 'moving' | 'tipped' | 'busy' | null;

/** One try, by the night's clock (whole seconds): the command, on the schedule, leaving it (47 s
 *  after: the lamps' four and seven), turning back, home at 026. */
export interface ArrayTry { cmd: number; on?: number; lost?: number; turn?: number; home?: number }
/** What is kept with the case (NightShiftState.array). */
export interface ArraySave {
  runs: ArrayTry[];
  /** A try going on as the case was kept, as far as it had come (restore finishes it). */
  run?: ArrayTry;
}
const KEYS = ['cmd', 'on', 'lost', 'turn', 'home'] as const;
function copyTry(r: unknown): ArrayTry | null {
  const t = r as ArrayTry | null;
  if (!t || typeof t !== 'object' || !Number.isFinite(t.cmd)) return null;
  const out: ArrayTry = { cmd: t.cmd };
  for (const k of KEYS) { const x = t[k]; if (x !== undefined && Number.isFinite(x)) out[k] = x; }
  return out;
}
/** A copy of a kept one, or undefined for anything that is not one. */
export function copyArraySave(v: unknown): ArraySave | undefined {
  const a = v as ArraySave | undefined;
  if (!a || typeof a !== 'object' || !Array.isArray(a.runs)) return undefined;
  const out: ArraySave = { runs: a.runs.map(copyTry).filter((r): r is ArrayTry => !!r) };
  const run = copyTry(a.run);
  if (run) out.run = run;
  return out;
}

export interface ArrayIn {
  /** In SARO's area, where the array is. */
  saro: boolean;
  /** In the control room (the status console, the windows), and out of doors at SARO. */
  room: boolean;
  outside: boolean;
  /** In the truck at SARO: the field through the windscreen (nothing heard of it over the engine). */
  cab: boolean;
  cam: THREE.Camera;
  eye: THREE.Vector3;
  /** The warning of the time away has begun (story/abduction.ts): the array leaves the schedule. */
  warning: boolean;
  /** The dishes the warning's dishes have (story/dishesFollow.ts), while they have any. */
  taken: Set<string> | null;
  /** The dishes that follow are not going (their lamps are this one's again). */
  followIdle: boolean;
  /** The calm flashes setting: the lamps' four and seven softer. */
  calm: boolean;
}
export interface ArrayDeps {
  dishes: Dish[];
  array: DishArray;
  audio: AudioSys;
  /** Kept with Night Shift's part of the case. */
  store: { get: () => ArraySave | undefined; set: (v: ArraySave) => void };
  clock: () => number;
  /** A line in the console log at its own time (Prologue.consoleLog). */
  log: (text: string, at?: number) => void;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  save: () => void;
  /** The crickets (Ambience.hush). */
  hush: (secs: number, level: number, tau?: number, back?: number) => void;
  /** S-03's motor bus has more in its log (Chapter1 files it again). */
  s03: () => void;
  /** The floods out among the near dishes (Exterior.fieldLights): as built, out (0), or red. */
  field: (k: number | 'on', red?: boolean) => void;
}

interface Rig {
  d: Dish; i: number;
  az: number; el: number; vAz: number; vEl: number;
  /** When its contactor goes (seconds into the slew); started; one of the few heard close by. */
  at: number; on: boolean; loud: boolean;
  /** Still this one's (the warning's dishes have not taken it). */
  mine: boolean;
}

const wrap180 = (a: number) => ((a + 540) % 360) - 180;
const wrapPi = (a: number) => Math.atan2(Math.sin(a), Math.cos(a));
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
/** One axis of a drive: up to vmax, gathering speed at acc, braking harder, stopping on the target. */
function axis(v: number, err: number, vmax: number, acc: number, dt: number) {
  const want = Math.sign(err) * Math.min(vmax, Math.sqrt(2 * acc * Math.abs(err)) * 0.92);
  const faster = Math.abs(want) > Math.abs(v) && want * v >= 0, lim = (faster ? 1 : 1.8) * acc * dt;
  return v + THREE.MathUtils.clamp(want - v, -lim, lim);
}
/** Where the lamps are in their four and seven, seconds after the 47th: on, and which flash. */
export function lampsAt(ts: number): { on: boolean; n: number } {
  if (ts < DARK0) return { on: false, n: -1 };
  const a = (ts - DARK0) / SLOT;
  if (a < 4) return { on: (ts - DARK0) % SLOT < ON, n: Math.floor(a) };
  if (ts < SEVEN) return { on: false, n: 3 };
  const b = (ts - SEVEN) / SLOT;
  if (b < 7) return { on: (ts - SEVEN) % SLOT < ON, n: 4 + Math.floor(b) };
  return { on: false, n: 10 };
}

const _v = new THREE.Vector3();

export class ArrayBack {
  stage: ArrayStage = 'idle';
  /** Seconds into the stage. */
  t = 0;
  /** What the player got of this try (and the last): the lamps seen, the console's beeps heard,
   *  the dishes seen turning back. */
  got = { lamps: false, beeps: false, back: false };
  /** The lamps' flashes, as they come (tests): how many came, and whether they are lit now. */
  flashes = 0;
  lit = false;
  private rigs: Rig[] = [];
  /** This try's clocks. */
  private run: ArrayTry = { cmd: 0 };
  /** Each lamp's place in its blink (the array's lamps, by index), and the one they slide to. */
  private ph: number[] = [];
  private common = 0;
  private hushed = false;
  private cut = false;
  private drifting = false;
  private motors = false;
  private said = { sync: false, flash: false, turn: false };

  constructor(private d: ArrayDeps) {}

  get busy() { return this.stage !== 'idle'; }
  /** How many times it was put back and went back. */
  get count() { return this.d.store.get()?.runs.length ?? 0; }
  /** Every try kept (tests, the S-03 history). */
  get runs() { return this.d.store.get()?.runs.map((r) => ({ ...r })) ?? []; }
  /** Where to look to see it: the middle of the array's near rows, a lamp's height. */
  get centre() { return _v.set(-12, 14, -120); }

  /** The status screen in the control room (Prologue.drawStatus), while a try runs: its mode, its
   *  bottom line, and whether they are amber. */
  status(): { mode: string; line: string | null; warn: boolean } | null {
    switch (this.stage) {
      case 'armed': case 'slew': return { mode: 'SLEWING TO SCHEDULE', line: 'S-01 TO S-27  CMD  AZ 042 EL 48', warn: false };
      case 'hold': return { mode: 'ON SCHEDULE', line: `ON SCHEDULE  00:00:${String(Math.min(HOLD, Math.floor(this.t))).padStart(2, '0')}`, warn: false };
      case 'signal': return { mode: 'ON SCHEDULE', line: `ON SCHEDULE  00:00:${HOLD}`, warn: true };
      case 'back': return { mode: 'NO SCHEDULE ENTRY', line: 'MOTION  NO CONTROL COMMAND', warn: true };
      case 'settle': return { mode: 'NO SCHEDULE ENTRY', line: 'AZ 026 EL 32  NO CONTROL COMMAND', warn: true };
      default: return null;
    }
  }
  /** What the status terminal says when used during a try (Prologue's crtLeft). */
  statusText(): string | null {
    switch (this.stage) {
      case 'armed': case 'slew': return 'S-01 to S-27: slewing to the schedule. Az 042, el 48.';
      case 'hold': case 'signal': return `S-01 to S-27 on the schedule. Az 042, el 48, for ${Math.min(HOLD, Math.floor(this.t))} seconds.`;
      case 'back': case 'settle': return 'S-01 to S-27: moving. No control command. No drive current.';
      default: return null;
    }
  }

  // ---------- the console (story/nightshift.ts) ----------
  /** The operations terminal's answer to one of the array's commands, or null when it is not one. */
  command(input: string, gate: ArrayGate): string | null {
    const c = input.trim().toUpperCase().replace(/\s+/g, ' ');
    if (!/^(ARRAY RESUME( SCHEDULE| ALL)?|ARRAY RESUME\/SCHEDULE|RESUME ARRAY|RESUME SCHEDULE)$/.test(c)) return null;
    if (this.busy) return '%ARRAY-W-INPROG, schedule resume in progress';
    if (gate === 'early') return 'ARRAY ON SURVEY TRACK.\nNOTHING TO RESUME.';
    if (gate === 'release') return '%ARRAY-E-HELD, array held, no schedule entry\n-ARRAY-I-SUPV, release by supervisor only';
    if (gate === 'moving') return '%ARRAY-E-DRVMOTION, drives in motion\n-ARRAY-I-NOCMD, no command';
    // (tipped down on 026 by the archive's phone, story/archiveDishes.ts: they do not answer)
    if (gate === 'tipped') return '%ARRAY-E-DRVNORESP, drives not responding\n-ARRAY-I-POS, S-01 TO S-27  AZ 026  EL 02\n-ARRAY-I-NOCMD, no command';
    if (gate === 'busy') return '%ARRAY-W-LINKBUSY, array link busy\n-ARRAY-I-RETRY, retry';
    const last = this.d.store.get()?.runs.slice(-1)[0];
    this.begin();
    const head = last?.lost !== undefined ? `%ARRAY-W-LEFTSCHED, left schedule ${clockText(last.lost)}\n-ARRAY-I-NOCMD, no command\n` : '';
    return `${head}ARRAY RESUME\nSCHEDULE  RUN860413_2341\nPOINTING  AZ 042  EL 48\nDRIVES    S-01 TO S-27  ENABLED`;
  }

  /** S-03's own log since 02:15 (Chapter1.docS03): each command, on the schedule, and the turn back
   *  with no command. The last four tries. */
  history(): { cmds: number; text: string } | null {
    const r = this.d.store.get()?.runs ?? [];
    if (!r.length) return null;
    const rows: string[] = [];
    for (const t of r.slice(-4)) {
      rows.push(`${clockText(t.cmd)}  CMD RESUME   AZ 042 EL 48`);
      if (t.on !== undefined) rows.push(`${clockText(t.on)}  ON SCHEDULE  AZ 042 EL 48`);
      const back = t.home ?? t.turn ?? t.lost;
      if (back !== undefined) rows.push(`${clockText(back)}  NO COMMAND   AZ 026 EL 32`);
    }
    return { cmds: r.length, text: `SINCE ${clockText(r[0].cmd, false)}:\n${rows.join('\n')}` };
  }

  // ---------- a try ----------
  private begin() {
    this.stage = 'armed'; this.t = 0; this.cut = false; this.hushed = false; this.motors = false;
    this.got = { lamps: false, beeps: false, back: false }; this.flashes = 0; this.lit = false;
    this.said = { sync: false, flash: false, turn: false };
    const c = Math.round(this.d.clock());
    this.run = { cmd: c };
    this.d.log('ARRAY RESUME  RUN860413_2341  OPERATOR REYES', c);
    // out along the cable trench from the control room: the near rows first, the far ones last
    const far = Math.max(...this.d.dishes.map((d) => Math.hypot(d.x, d.z)));
    this.rigs = this.d.dishes.map((d, i) => ({ d, i, az: d.curAz, el: d.curEl, vAz: 0, vEl: 0,
      at: ARM + STAGGER * Math.hypot(d.x, d.z) / far + rnd(0, 0.12), on: false, loud: false, mine: true }));
    this.lamps();
    this.keep();
  }
  /** The lamps' places, the first time they are needed. */
  private lamps() { if (this.ph.length !== this.d.dishes.length) this.ph = this.d.dishes.map((d) => d.phase); }
  private keep() {
    const v = this.d.store.get() ?? { runs: [] };
    this.d.store.set({ runs: v.runs, run: { ...this.run } });
  }

  update(dt: number, s: ArrayIn) {
    if (this.stage === 'idle') { this.drift(dt, s); return; }
    // away from SARO: nobody there to see it go back
    if (!s.saro) { this.away(); return; }
    // the warning of the time away: the array leaves the schedule for it
    if (s.warning && !this.cut && this.stage !== 'settle') this.leave();
    this.t += dt;
    switch (this.stage) {
      case 'armed': if (this.t >= ARM) { this.stage = 'slew'; this.t = ARM; this.loud(s.eye); } break;
      case 'slew': this.slew(dt, s); break;
      case 'hold': this.hold(dt, s); break;
      case 'signal': this.signal(s); break;
      case 'back': this.back(dt, s); break;
      case 'settle': if (this.t >= SETTLE) this.end(true); break;
    }
  }

  /** The few heard close by: the nearest to the player now. */
  private loud(eye: THREE.Vector3) {
    [...this.rigs].sort((a, b) => Math.hypot(a.d.x - eye.x, a.d.z - eye.z) - Math.hypot(b.d.x - eye.x, b.d.z - eye.z))
      .slice(0, 5).forEach((r) => { if (Math.hypot(r.d.x - eye.x, r.d.z - eye.z) < 160) r.loud = true; });
  }
  private slew(dt: number, s: ArrayIn) {
    let all = true;
    for (const r of this.rigs) {
      if (!r.mine) continue;
      if (this.t < r.at) { all = false; continue; }
      if (!r.on) {
        r.on = true;
        if (!this.motors) {
          this.motors = true; this.d.audio.startMotors();
          if (this.count === 0) {
            if (s.outside) this.d.toast('Across the field the drives start up, one after another.', 3.2);
            else if (s.room) this.d.toast('Through the windows the dishes are turning, all of them, to the schedule.', 3.4);
          }
        }
        if (r.loud) this.clunk(r.d, true, s.calm);
      }
      if (!this.drive(r, SCHED, dt)) all = false;
    }
    if (all) this.onSchedule(s);
  }
  /** Turn a dish towards a pointing at the drives' pace; true when it is on it, stopped. */
  private drive(r: Rig, to: { az: number; el: number }, dt: number) {
    const eAz = wrap180(to.az - r.az), eEl = to.el - r.el;
    r.vAz = axis(r.vAz, eAz, RATE.az, RATE.accAz, dt);
    r.vEl = axis(r.vEl, eEl, RATE.el, RATE.accEl, dt);
    if (Math.abs(eAz) <= Math.abs(r.vAz) * dt + 0.02 && Math.abs(r.vAz) < 1.2) { r.az = to.az; r.vAz = 0; } else r.az += r.vAz * dt;
    if (Math.abs(eEl) <= Math.abs(r.vEl) * dt + 0.02 && Math.abs(r.vEl) < 1.2) { r.el = to.el; r.vEl = 0; } else r.el += r.vEl * dt;
    r.d.snap(r.az, r.el);
    r.d.spin = Math.abs(r.vAz) + Math.abs(r.vEl);
    return r.az === to.az && r.el === to.el && !r.vAz && !r.vEl;
  }
  private onSchedule(s: ArrayIn) {
    this.stage = 'hold'; this.t = 0;
    this.run.on = Math.round(this.d.clock());
    this.d.log('ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48', this.run.on);
    if (this.motors) { this.motors = false; this.d.audio.stopMotors(); }
    for (const r of this.rigs) { r.d.spin = 0; if (r.loud && r.mine) this.clunk(r.d, false, s.calm, rnd(0, 0.5)); }
    // the one they slide to: the nearest lamp's
    this.lamps();
    const near = this.rigs.filter((r) => r.loud)[0] ?? this.rigs[0];
    this.common = this.ph[near.i];
    if (this.count === 0) this.d.toast('All twenty-seven on the schedule. Az 042, el 48.', 3.4);
    this.keep();
  }
  private hold(dt: number, s: ArrayIn) {
    // the red lamps on the feeds, each its own way, slide into one blink across the field
    if (this.t > SYNC[0]) {
      const k = Math.PI / (SYNC[1] - SYNC[0]);
      for (const r of this.rigs) {
        if (!r.mine) continue;
        this.ph[r.i] += THREE.MathUtils.clamp(wrapPi(this.common - this.ph[r.i]), -k * dt, k * dt);
        this.d.array.beaconPhase(r.i, this.ph[r.i]);
      }
    }
    if (!this.said.sync && this.t > 20 && this.t < HOLD - 6 && this.sees(s)) {
      this.said.sync = true;
      if (this.count < 2) this.d.toast('The red lamps on the feeds are blinking together now. All of them.', 3.6);
    }
    if (!this.hushed && this.t >= HUSH) { this.hushed = true; this.d.hush(180, 0.02, 1.4, 2.5); }
    if (this.t >= HOLD) {
      this.stage = 'signal'; this.t = 0; this.lit = false;
      this.d.array.holdBeacons(s.calm ? 0.22 : 0);
      // and the floods out among them, with every lamp on the posts
      this.d.field(s.calm ? 0.35 : 0);
      // (the console has the lamps as it has the signal: the binder's printouts say the same)
      // (stamped 47 s after ON SCHEDULE, which is when it is: the frame may be a little after)
      this.run.lost = (this.run.on ?? Math.round(this.d.clock() - HOLD)) + HOLD;
      this.d.log('ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7', this.run.lost);
      this.keep();
    }
  }
  private signal(s: ArrayIn) {
    const l = lampsAt(this.t), dark = s.calm ? 0.22 : 0;
    if (l.on !== this.lit) {
      this.lit = l.on;
      this.d.array.holdBeacons(l.on ? (s.calm ? 1.3 : FLASH) : dark, l.on ? (s.calm ? 0.6 : FLARE) : 0);
      // the near dishes lit red by their own lamps (not with calm flashes: the floods stay low)
      if (!s.calm) this.d.field(l.on ? 0.42 : 0, l.on);
      if (l.on) {
        this.flashes++;
        // the status console in the control room, with them; out of doors, with each flash, the
        // deep thump the signal made in the receiver at 02:14 (core/signalVoice.ts), low and far
        if (s.room) { this.d.audio.beep(this.flashes <= 4 ? 880 : 988, 0.16, 0.05, 0, 'square'); this.got.beeps = true; }
        else if (s.outside) this.thump(s.calm ? 0.12 : 0.26);
      }
    }
    if (this.sees(s)) this.got.lamps = true;
    if (this.t >= SIGNAL) this.turn(s);
  }
  /** After the seventh: back to 026, together and without a sound. */
  private turn(s: ArrayIn) {
    this.stage = 'back'; this.t = 0; this.lit = false;
    const c = Math.round(this.d.clock());
    this.run.turn = c;
    this.d.log('ARRAY  S-01 TO S-27  MOTION  NO COMMAND', c);
    this.d.log('ARRAY  DRIVE CURRENT 0.0 A', c);
    this.d.array.holdBeacons(STEADY);
    this.d.field('on');
    for (const r of this.rigs) if (r.mine) { r.d.back = true; r.vAz = r.vEl = 0; }
    const again = this.count > 0;
    if (this.got.lamps) this.d.toast(again ? 'Four. Then seven.' : 'The lights in the field went out. Every red lamp in the array, all at once: four flashes. Then seven.', 4);
    else if (s.room) this.d.toast('The status console beeps. Four. Then seven.', 3.2);
    else if (s.outside) this.d.toast('Out in the array the red lamps are flashing, all of them. Four. Then seven.', 3.4);
    this.keep();
  }
  private back(dt: number, s: ArrayIn) {
    let all = true;
    for (const r of this.rigs) {
      if (!r.mine) continue;
      // (the warning's dishes have it now: it is theirs from where it is)
      if (s.taken?.has(r.d.id)) { r.mine = false; r.d.back = false; r.d.spin = 0; continue; }
      if (!this.drive(r, HOME, dt)) all = false;
    }
    if (this.t > 0.6 && this.sees(s)) this.got.back = true;
    if (!this.said.turn && this.t > 1.6) {
      this.said.turn = true;
      if (this.cut) { /* the warning has the night: nothing said */ }
      // (watched from the control room's windows as well: the console's hum went with them out to
      // the schedule, and there is none now)
      // (from the truck nothing is heard of them over the engine either way)
      else if (this.got.back) this.d.toast(this.count ? (s.cab ? 'Back to 026.' : 'Back to 026. Not a sound.') : `They are turning back. All twenty-seven at once${s.cab ? '' : ', and not a sound'}.`, 3.6);
      else if (s.room) this.d.toast(this.count ? 'On the status screen they are moving back. No command.' : 'On the status screen all twenty-seven are moving back. No command. No drive current.', 3.6);
    }
    if (all) this.home(s);
  }
  private home(s: ArrayIn) {
    const mine = this.rigs.filter((r) => r.mine);
    for (const r of mine) { r.d.snap(HOME.az, HOME.el); r.d.back = false; r.d.spin = 0; }
    this.run.home = Math.round(this.d.clock());
    this.d.log(`ARRAY  ${mine.length} DRIVES  AZ 026 EL 32  NO COMMAND`, this.run.home);
    if (!this.cut) {
      this.d.array.holdBeacons(null);
      for (const r of mine) { this.ph[r.i] = this.common; this.d.array.beaconPhase(r.i, this.common); }
      // every brake at once, out among them; in the control room the console's chime of 02:15
      const near = mine.filter((r) => r.loud)[0];
      if (near && s.outside) {
        const a = this.d.audio, sp = a.spatial(new THREE.Vector3(near.d.x, 6 * near.d.scale, near.d.z), 9);
        if (sp) for (const [rate, gain, when] of [[0.42, 0.5, 0], [0.47, 0.38, 0.012], [0.38, 0.32, 0.03]]) a.play(a.or('door_heavy', 'doorMetal'), { dest: sp, gain: gain * (s.calm ? 0.6 : 1), rate, lp: 900, when });
      }
      if (s.room) { const a = this.d.audio; a.beep(1760, 0.18, 0.06); a.beep(1760, 0.18, 0.06, 0.3); a.beep(2350, 0.4, 0.06, 0.6); }
      if (this.got.back) this.d.toast('Az 026, el 32. All twenty-seven. Nobody sent a command.', 3.6);
      else if (s.outside) this.d.toast('Out in the array, every brake at once.', 3);
      else if (s.room) this.d.toast('The console chimes, as it did at 02:15. Az 026, el 32, all twenty-seven.', 3.6);
    }
    this.stage = 'settle'; this.t = 0;
    this.keep();
  }
  /** It is over: kept, written down, and S-03's log has it. */
  private end(seen: boolean) {
    if (this.hushed && !this.cut) this.d.hush(0, 1, 0.4, 2.5);
    this.hushed = false;
    if (this.motors) { this.motors = false; this.d.audio.stopMotors(); }
    const v = this.d.store.get() ?? { runs: [] };
    const run = { ...this.run };
    this.d.store.set({ runs: [...v.runs, run] });
    const n = v.runs.length + 1, at = clockText(run.cmd, false);
    if (this.cut) this.d.note(`${at}. ARRAY RESUME. The array went to the schedule, and when the warning came it left it by itself. 026 again, el 32. Nobody sent a command.`);
    else if (!seen || !(this.got.lamps || this.got.beeps || this.got.back)) this.d.note(`${at}. ARRAY RESUME. The console log has the array on the schedule for 47 seconds, then back on 026 by itself. Nobody sent a command.`);
    else if (n === 1) {
      const saw = this.got.lamps ? 'every red lamp in the array blinked four times, then seven, all together' : this.got.beeps ? 'the status console beeped four times, then seven' : 'the lamps out in the array blinked four, then seven';
      this.d.note(`${at}. ARRAY RESUME. All twenty-seven went to the schedule, az 042, el 48, and stayed there 47 seconds. Then ${saw}, and they turned back to 026 by themselves, all at once. No motors. No drive current. Nobody sent a command.`);
    } else if (n === 2) this.d.note(`${at}. ARRAY RESUME again. 042 for 47 seconds, four and seven on the lamps, and back to 026 by themselves. Twice now.`);
    else this.d.note(`${at}. The ${ordinal(n)} time. The same: 47 seconds on the schedule, four, seven, 026.`);
    this.rigs = []; this.stage = 'idle'; this.t = 0; this.cut = false;
    this.drifting = true;
    this.d.s03();
    this.d.save();
  }
  /** The warning of the time away came: the dishes it takes are its; the rest turn back at once,
   *  with no four and seven (the warning has the night). */
  private leave() {
    this.cut = true;
    this.d.array.holdBeacons(null);
    this.d.field('on');
    if (this.motors) { this.motors = false; this.d.audio.stopMotors(); }
    if (this.stage === 'settle') return;
    const c = Math.round(this.d.clock());
    this.run.on ??= c;
    this.run.lost ??= c;
    if (this.run.turn === undefined) { this.run.turn = c; this.d.log('ARRAY  S-01 TO S-27  MOTION  NO COMMAND', c); }
    if (this.stage !== 'back') for (const r of this.rigs) if (r.mine) { r.d.back = true; r.vAz = r.vEl = 0; r.on = true; }
    this.stage = 'back'; this.t = 0; this.said.turn = true;
    // (the crickets are the warning's now; this lets go of nothing when it ends)
    this.hushed = false;
    this.keep();
  }
  /** Out of SARO's area in the middle of it: home at once, unseen, and the log has the rest. */
  private away() {
    const c = Math.round(this.d.clock());
    const r = this.run;
    if (r.on === undefined) { r.on = c; this.d.log('ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48', c); }
    if (r.lost === undefined) { r.lost = Math.max(c, r.on + HOLD); this.d.log('ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7', r.lost); }
    if (r.turn === undefined) { r.turn = Math.max(c, r.lost + Math.round(SIGNAL)); this.d.log('ARRAY  S-01 TO S-27  MOTION  NO COMMAND', r.turn); this.d.log('ARRAY  DRIVE CURRENT 0.0 A', r.turn); }
    for (const x of this.rigs) if (x.mine) { x.d.snap(HOME.az, HOME.el); x.d.back = false; x.d.spin = 0; }
    if (r.home === undefined) { r.home = Math.max(c, r.turn + 13); this.d.log(`ARRAY  ${this.rigs.filter((x) => x.mine).length} DRIVES  AZ 026 EL 32  NO COMMAND`, r.home); }
    this.d.array.holdBeacons(null);
    this.d.field('on');
    this.end(false);
  }

  /** Idle: the lamps that went into one blink go back to their own, slowly (not while the dishes
   *  that follow have some of them). */
  private drift(dt: number, s: ArrayIn) {
    if (!this.drifting) return;
    if (!s.followIdle) { this.drifting = false; this.lamps(); this.d.dishes.forEach((d, i) => { this.ph[i] = d.phase; }); return; }
    let left = false;
    this.d.dishes.forEach((d, i) => {
      const e = wrapPi(d.phase - this.ph[i]);
      if (Math.abs(e) > 0.004) { left = true; this.ph[i] += THREE.MathUtils.clamp(e, -0.035 * dt, 0.035 * dt); this.d.array.beaconPhase(i, this.ph[i]); }
    });
    if (!left) this.drifting = false;
  }
  /** Where the field can be seen from (out of doors, the control room's windows, the truck's
   *  windscreen at SARO), and some of it on the screen. */
  private sees(s: ArrayIn) { return (s.outside || s.room || s.cab) && this.inView(s.cam); }
  /** Some of the array's lamps on the screen, near enough. */
  private inView(cam: THREE.Camera) {
    if (facing(cam, this.centre, 700, 55)) return true;
    for (const d of this.d.dishes) if (facing(cam, _v.set(d.x, 16 * d.scale, d.z), 500, 45)) return true;
    return false;
  }
  /** One pulse of the four and seven, heard out in the open: a sine falling from 118 to 41 Hz and
   *  an octave over it, as the signal's pulse is drawn (SignalVoice.thump), without its clank. */
  private thump(level: number) {
    const ctx = this.d.audio.ctx; if (!ctx) return;
    const t = ctx.currentTime + 0.005, env = ctx.createGain();
    env.gain.setValueAtTime(0, t); env.gain.linearRampToValueAtTime(level, t + 0.008); env.gain.exponentialRampToValueAtTime(0.001, t + 0.42);
    env.connect(this.d.audio.sfx);
    for (const [f0, f1, g] of [[118, 41, 1], [236, 82, 0.3]] as const) {
      const o = ctx.createOscillator(); o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(f1, t + 0.18);
      const og = ctx.createGain(); og.gain.value = g; o.connect(og); og.connect(env);
      o.start(t); o.stop(t + 0.45);
    }
    setTimeout(() => env.disconnect(), 900);
  }
  /** A contactor and the brake at a dish's foot: letting go (start) or setting (stop). */
  private clunk(d: Dish, start: boolean, calm: boolean, when = 0) {
    const a = this.d.audio, sp = a.spatial(new THREE.Vector3(d.x, 6 * d.scale, d.z), 6);
    if (!sp) return;
    const k = calm ? 0.6 : 1, sw = a.or('rack_switch', 'switch'), brake = a.or('door_heavy', 'doorMetal');
    if (start) { a.play(sw, { dest: sp, gain: 0.45 * k, rate: 0.62, when }); a.play(brake, { dest: sp, gain: 0.34 * k, rate: 0.5, lp: 1300, when: when + 0.09 }); }
    else { a.play(brake, { dest: sp, gain: 0.3 * k, rate: 0.45, lp: 900, when }); a.play(sw, { dest: sp, gain: 0.32 * k, rate: 0.55, when: when + 0.06 }); }
  }

  // ---------- the case ----------
  /** Everything stopped (a new night, the title, a jump): nothing of a try runs on. The dishes are
   *  put where the chapter wants them by the chapter (Prologue.afterPrologue). */
  reset() {
    if (this.motors) this.d.audio.stopMotors();
    if (this.hushed && !this.cut) this.d.hush(0, 1, 0.3, 1);
    this.motors = this.hushed = this.cut = this.drifting = false;
    this.d.array.holdBeacons(null);
    this.d.field('on');
    for (const r of this.rigs) { r.d.back = false; r.d.spin = 0; }
    this.rigs = []; this.stage = 'idle'; this.t = 0; this.lit = false;
    // (the lamps each in its own blink again, at once)
    if (this.ph.length) this.d.dishes.forEach((d, i) => this.d.array.beaconPhase(i, d.phase));
    this.ph = [];
  }
  /** After a case is opened (Night Shift's part back, the dishes at 026): a try kept in the middle
   *  of it is over, counted, and the log has the rest. */
  restore() {
    this.reset();
    const v = this.d.store.get();
    if (!v?.run) return;
    // (kept part way: the log has what came before the save; this writes the rest at its own times)
    const r = v.run, on = r.on ?? r.cmd + 14, lost = r.lost ?? on + HOLD, turn = r.turn ?? lost + Math.round(SIGNAL), home = r.home ?? turn + 13;
    if (r.on === undefined) this.d.log('ARRAY  S-01 TO S-27  ON SCHEDULE  AZ 042 EL 48', on);
    if (r.lost === undefined) this.d.log('ARRAY  BEACONS S-01 TO S-27  PULSE GROUP 4 / 7', lost);
    if (r.turn === undefined) { this.d.log('ARRAY  S-01 TO S-27  MOTION  NO COMMAND', turn); this.d.log('ARRAY  DRIVE CURRENT 0.0 A', turn); }
    if (r.home === undefined) this.d.log('ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND', home);
    this.d.store.set({ runs: [...v.runs, { cmd: r.cmd, on, lost, turn, home }] });
    for (const d of this.d.dishes) { d.snap(HOME.az, HOME.el); d.back = false; d.spin = 0; }
    this.d.note(`${clockText(r.cmd, false)}. ARRAY RESUME. The console log has the array on the schedule for 47 seconds, then back on 026 by itself. Nobody sent a command.`);
    this.d.s03();
  }
}
