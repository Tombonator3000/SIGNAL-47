import * as THREE from 'three';
import type { TruckTrouble } from '../drive/trouble';
import type { Receiver } from '../core/radio';
import type { AudioSys } from '../core/Audio';
import { offset, type MapPt } from './lines';
import { BEAT, WARN } from './abduction';
import { clockText } from './time';

// Taken in the truck (FORSLAG.md, 20 October, item 10; HENDELSER.md, section 8: "alone, outdoors or
// in the car"). Until now the time away came only on foot. B crosses the highway at SARO's turn-off
// (story/lines.ts, CROSSINGS: 23 m south of the site); A crosses it in Caliche's main street, among
// people and lamps, and C crosses the old road, which is chapter six's: so here, on B, southbound
// (the way the proposal has it, and the only way there is 300 m of highway further on).
//
// When the night's curve is at its peak (story/curve.ts, may('abduct')), the time away could come
// (story/abduction.ts, mayBegin: Missing time on, at most three, 40 minutes since the last, room for
// the lost time) and nothing else of the night's is going (one at a time), the truck going south
// over B gets the warning in the cab: the same 47 seconds as on foot (PR #125), the corner clock in
// amber seconds, and the truck has the beats:
//
//   :00  the radio slides off its station under the static and comes out as the bare carrier (as it
//        is on or off, PR #131's carrier); the engine dies under the player and the headlights sink
//        onto the battery, OIL and VOLTS; the truck slows to a stop as if something had hold of it,
//        brakes or none. "Turn the key."
//   :16  the beep, and the engine's note and the tyres drop out: no sound of the truck at all, as
//        the footsteps go on foot. The starter turns in silence after this.
//   :31  the pressure; the headlights and the dash go down with the battery.
//   :40  every light in the truck out, the radio with them, with the click K3 made (the flashlight's
//        beat on foot). The key does nothing now: not even the starter.
//   :41  three knocks on the roof of the cab, slow, from above (T. Vega's July 5, 1947, and the
//        night's own knocks, story/nightEvents.ts: if the night still owes them, these are them).
//   :47  the light from above, through the windshield, and the taking (story/abduction.ts).
//
// The way out is the truck's: it is the truck they have hold of.
//   the key  (Use, E, or the Use button on a phone): the first turn turns it over and does not catch,
//            the second nearly; the third catches, if it is begun before the beep at :16. A quick and
//            persistent hand has the engine going in five or six seconds; after :16 it never will.
//            The second warning in the truck in a night, once the key has got it away, it never
//            catches at all ("Nearly.", then "It does not catch this time."): out is the way then.
//   out      stopped, Get out (Q, or the Get out button): out of the truck and RUN metres away from
//            it before the light, any way (back up the shoulder towards SARO is the nearest light).
//            Got back into, the truck is as dead as it was.
//
// Taken, the player wakes the time away later (abduction.ts: 17 to 47 minutes, never past the story's
// next fixed point) at the wheel 300 m further on, in the southbound lane with the nose to the north,
// the wrong way: the engine off, every light off, the radio on, on 1090 (the far station, core/
// radio.ts), frost on the windshield and on the asphalt round the truck. One of them is on the bonnet
// as the head comes up (abduction.ts, OVER_CAB). Then the instruments come on, and the odometer reads
// 31.4 miles more than it did at SARO (World.odometer; drive/truckTextures.ts drawOdo). Out of the
// truck at the light, not far enough, the player wakes on their back on the verge beside it,
// the truck put down there all the same.
//
// The truck on B (story/truckOnB.ts) has the same crossing. A rung of its ladder that is due takes the
// pass (the first time over B from chapter two, the next time after room 6), and this never comes on
// the same pass: the four first, then this on a later pass; after room 6 the seven first. One at a
// time otherwise (main.ts, oneAtATime): it never starts on top of anything, and nothing starts while
// it runs or for TAIL seconds after. It is the curve's peak (the warning lands it, story/seen.ts),
// and the rest follows. Chapters one to four, at night; not with Missing time turned off (and turned
// off in the middle, the truck lets go). No saving from the crossing to the waking (main.ts,
// saveBlock: abduction.racing); after it, the truck is kept where it was put down (World.moveDriven).
// At most twice in a night (a warning got away from may come once more, at a later peak), and taken
// once. Kept in the town's part of the case ('tt:try@', 'tt:taken@', 'tt:key@', 'tt:ran@') and with
// the times away (AbductSave.spots[].cab). No new voices: toasts and the notebook.

export type TruckTakenPhase = 'idle' | 'warning' | 'taken' | 'wake' | 'tail';
/** How a warning in the truck ended: the key, out and away, Missing time turned off, or taken. */
export type TruckTakenEnd = 'key' | 'ran' | 'off' | 'taken';

export interface TruckTakenIn {
  /** In play, no panel, no cinematic, chapters one to four, at night, and nothing else of the
   *  night's going (one at a time), no trouble with the truck, no call on the line. */
  ok: boolean;
  /** The night's curve is at its peak and lets the time away come (and is not standing aside). */
  may: boolean;
  /** The time away may come as far as it goes itself (abduction.ts, mayBegin). */
  allowed: boolean;
  /** Missing time on. */
  enabled: boolean;
  /** Driving on a trip (the story's or the player's own) in SARO's area. */
  trip: boolean;
  /** The driven truck on the map (road-local metres), or null. */
  at: MapPt | null;
  speed: number;
  /** How far the truck faces south (its forward's z on the map, -1 to 1). */
  south: number;
  /** A rung of the truck on B is due on a pass over B now (story/truckOnB.ts, due). */
  due: boolean;
  /** The time away: its stage, and the warning's seconds (null outside it). */
  stage: string;
  warnT: number | null;
  /** At the wheel now (else on foot), and how far from the truck the player is (m), and how far
   *  north of it (m, on the map). */
  inCab: boolean;
  away: number;
  north: number;
  clock: number;
  /** Whether the time away has someone over the player on waking now (abduction.ts, overPhase). */
  over: boolean;
}
export interface TruckTakenDeps {
  trouble: TruckTrouble;
  /** The truck's radio (main.ts, carRadio). */
  radio: Receiver;
  audio: AudioSys;
  /** The town's part of the case (main.ts, townSeen). */
  seen: Set<string>;
  /** The warning begins in the cab (abduction.beginCab). */
  begin: () => void;
  /** Something holds the truck back (World.holdTruck), its sound (World.hushTruck). */
  hold: (k: number) => void;
  hush: (on: boolean) => void;
  /** The truck's roof in the world (the knocks), or null. */
  roof: () => THREE.Vector3 | null;
  /** The truck in SARO's area now: where (world, which is SARO's site) and its heading, or null. */
  truck: () => { x: number; z: number; h: number } | null;
  /** Put it down there: being driven (World.moveDriven), or standing (World.moveParked). */
  moveDriven: (x: number, z: number, h: number) => boolean;
  moveParked: (x: number, z: number, h: number) => boolean;
  /** The headlights of the truck being driven, on or off; the frost on its windshield. */
  headlights: (on: boolean) => void;
  frost: (k: number) => void;
  /** The odometer (World.odometer: miles since the shift began), what it read when the truck last
   *  set out from SARO, and miles nobody drove put on it. */
  odometer: () => number;
  odoAtSaro: () => number | null;
  addMiles: (miles: number) => void;
  /** The next time the truck is got into, its engine is dead and wants the key. */
  keyStall: () => void;
  ground: (x: number, z: number) => number;
  toast: (text: string, secs?: number) => void;
  note: (text: string) => void;
  save: () => void;
}

/** Within this far of B (m, the truck's middle) the truck is over it; this far off again before
 *  the next pass counts (as story/truckOnB.ts). */
const ON = 6, REARM = 16;
/** Out of the truck and this far from it (m), before the light: free. */
export const RUN = 30;
/** The truck slows to a stop under it (m/s²). */
const HELD = 3.2;
/** The third turn catches, if begun before the beep. */
const CATCH = 3;
/** Put down this far on from where it was taken (m), the nose the other way, in the southbound lane
 *  of SARO's highway (site x; drive/legs.ts SARO_HWY: x -24, the southbound lane west of the
 *  middle), and never onto the motel's lot. */
const ON_FROM = 300, LANE_X = -25.8, LAST_Z = 600;
/** The miles on the odometer when the player wakes, over what it read at SARO. */
export const MILES = 31.4;
/** A warning in the truck comes at most this many times in a night. */
const TRIES = 2;
/** Seconds nothing else of the night's starts after it. */
const TAIL = 8;
/** The knocks on the roof: when, and how far apart (as story/nightEvents.ts has them). */
const KNOCK_AT = 41.2, KNOCKS = [0, 0.95, 1.9];

export class TruckTaken {
  phase: TruckTakenPhase = 'idle';
  /** How the last one ended (the tests, the log). */
  ended: TruckTakenEnd | null = null;
  /** The truck has let go of the player in this warning (abduction.ts: it passes). */
  free: 'key' | 'ran' | null = null;
  /** Stands aside: never begins (the older tests that drive out of SARO). */
  off = false;
  /** The next pass over B southbound begins it, the curve or not (the tests, the developer menu). */
  force = false;
  /** The truck's distance from B now (m), or null (the tests). */
  dist: number | null = null;
  /** Passes over B southbound that were the ladder's (the tests). */
  ladderPasses = 0;
  /** The turns of the key in this warning, and the clock it began at. */
  turns = 0;
  /** This warning's key never catches (the night's second in the truck, after the key got away once). */
  keyProof = false;
  private at = 0;
  private armed = false;
  private pass: 'ladder' | 'free' | null = null;
  private t = 0;
  private knocked = false;
  private radioWas = false;
  private toldSilent = false;
  private toldDead = false;
  private gotOut = false;
  private wasOut = false;
  private north = 0;
  private plan: { x: number; z: number } | null = null;
  /** The knocks were the night's own (it still owed them): ne:knocks1 is set when the waking is over. */
  private knocksOwed = false;
  private overSeen = false;
  private frostK = 0;
  private dashK = 1;
  private wokeHere = false;
  /** Where it was taken: at the wheel, or out of the truck. */
  private takenIn = true;
  /** Woken at the wheel: the headlights come on when the engine does. */
  private lightsOwed = false;

  constructor(private d: TruckTakenDeps) {}

  /** Taken in the truck tonight. */
  get taken() { return [...this.d.seen].some((k) => k.startsWith('tt:taken@')); }
  /** Warnings in the truck tonight. */
  get tries() { let n = 0; for (const k of this.d.seen) if (k.startsWith('tt:try@')) n++; return n; }
  /** Something of it is going (one at a time: main.ts, oneAtATime). */
  get busy() { return this.phase !== 'idle'; }
  /** In the 47 seconds, the engine dead: the key and Get out are offered (main.ts). */
  get warning() { return this.phase === 'warning' && !this.free; }
  /** The truck is coming onto B on a trip or on it, and it could come on this pass (main.ts: N. Vega's
   *  CB call in chapter four, nine seconds out from the pad, would otherwise begin just as the truck
   *  reaches the turn-off and keep the crossing from the night: it waits the second or two). */
  get ahead() { return this.phase === 'idle' && this.could && this.armed && this.pass !== 'ladder' && this.dist !== null && this.dist < REARM; }
  private could = false;

  update(dt: number, s: TruckTakenIn) {
    const d = this.d;
    // after the waking, whatever comes next: the frost goes off the glass slowly, quicker once the
    // engine runs and the heater with it; the headlights come on with the engine
    if (this.frostK > 0) { this.frostK = Math.max(0, this.frostK - dt / (d.trouble.kind === 'stall' || !s.inCab ? 240 : 80)); d.frost(this.frostK); }
    if (this.lightsOwed && this.phase !== 'wake') {
      if (!s.inCab) this.lightsOwed = false;
      else if (d.trouble.kind !== 'stall') { this.lightsOwed = false; d.trouble.dim = 1; d.headlights(true); }
    }
    const off = s.at ? Math.abs(offset('B', s.at[0], s.at[1])) : null;
    this.dist = off;
    if (off === null || off > REARM || !s.trip) { this.armed = true; this.pass = null; }
    this.could = s.trip && !this.off && !this.taken && this.tries < TRIES && s.enabled && s.allowed && (s.may || this.force) && !s.due;
    switch (this.phase) {
      case 'idle': {
        if (!s.trip || off === null || off >= ON || !this.armed) return;
        // the pass is decided as the truck comes onto B: a rung of the ladder that is due has it
        if (this.pass === null) { this.pass = s.due ? 'ladder' : 'free'; if (s.due) this.ladderPasses++; }
        if (this.pass !== 'free' || this.off || this.taken || this.tries >= TRIES) return;
        if (s.south < 0.5 || Math.abs(s.speed) < 1.5) return;
        if (!s.ok || !s.enabled || !s.allowed || !(s.may || this.force)) return;
        this.armed = false; this.force = false;
        this.start(s);
        return;
      }
      case 'warning': return this.warningStep(dt, s);
      case 'taken': return this.takenStep();
      case 'wake': return this.wakeStep(dt, s);
      case 'tail': if ((this.t += dt) >= TAIL) this.phase = 'idle'; return;
    }
  }

  /** Begin now, the truck where it is (the developer menu; the pass over B calls it itself). */
  start(s: { clock: number }) {
    const d = this.d;
    this.phase = 'warning'; this.t = 0; this.at = s.clock; this.free = null; this.ended = null;
    this.turns = 0; this.knocked = false; this.toldSilent = false; this.toldDead = false; this.gotOut = false; this.wasOut = false; this.north = 0;
    this.failedTold.clear();
    this.wokeHere = false; this.overSeen = false; this.takenIn = true;
    this.knocksOwed = !d.seen.has('ne:knocks1');
    d.seen.add(`tt:try@${Math.round(s.clock)}`);
    const tr = d.truck();
    this.plan = tr ? { x: LANE_X, z: Math.min(LAST_Z, tr.z + ON_FROM) } : null;
    // the radio slides off its station under the static first (it is on), then the bare carrier
    this.radioWas = d.radio.on;
    if (!d.radio.drift(1.1, 1)) d.radio.carrier(0.5);
    // (the second time tonight, after the key got the truck away once, it never catches: they have a
    // better hold of it, and the way out is out of it)
    this.keyProof = [...d.seen].some((k) => k.startsWith('tt:key@'));
    d.trouble.start('stall', { key: true, tries: this.keyProof ? Infinity : CATCH });
    d.hold(HELD);
    d.begin();
    d.toast(this.radioWas ? 'The radio slides off the station into a bare carrier. The engine dies.' : 'The radio is off. A carrier comes out of it anyway. The engine dies.', 3.4);
  }

  /** Where the truck will be put down (world, on the ground), from where it stands when the light
   *  comes (abduction.ts asks then, and turns the dishes to it): 300 m on from there. */
  planned(): THREE.Vector3 | null {
    const tr = this.d.truck();
    if (tr) this.plan = { x: LANE_X, z: Math.min(LAST_Z, tr.z + ON_FROM) };
    const p = this.plan;
    return p ? new THREE.Vector3(p.x, this.d.ground(p.x, p.z), p.z) : null;
  }

  private warningStep(dt: number, s: TruckTakenIn) {
    const d = this.d, tr = d.trouble, t = s.warnT ?? this.t;
    this.t += dt;
    // it has passed (abduction.ts saw the truck let go, or Missing time turned off), or come
    if (s.stage === 'calm') { this.release(); this.end(this.free ?? 'off', s); return; }
    if (s.stage !== 'warning') { this.phase = 'taken'; this.takenIn = s.inCab; return; }
    if (this.free) return;
    if (s.inCab) {
      // (got back into it: as dead as it was, and three turns again before the beep)
      const can = t < BEAT.steps && !this.keyProof ? CATCH : Infinity;
      if (tr.kind !== 'stall') tr.start('stall', { key: true, tries: can, dead: true });
      if (this.wasOut) { this.wasOut = false; tr.catchOn(can); this.turns = tr.turns; this.failedTold.clear(); }
      // the third turn catches if it was begun before the beep; after it, never
      if (t >= BEAT.steps && tr.turns < CATCH) tr.catchOn(Infinity);
      d.hold(HELD);
      // the electrics go down with the battery from the pressure, and out at :40, the radio with them
      tr.dim = t < BEAT.tone ? 1 : t < BEAT.dark ? 1 - 0.55 * (t - BEAT.tone) / (BEAT.dark - BEAT.tone) : 0;
      if (tr.turns > this.turns) {
        this.turns = tr.turns;
        if (t >= BEAT.steps && !this.toldSilent) { this.toldSilent = true; d.toast('The starter turns. There is no sound at all.', 3); }
      }
      if (tr.stage === 'pause' && this.turns === 1 && !this.failedTold.has(1)) { this.failedTold.add(1); d.toast('It turns over and will not catch.', 2.6); }
      if (tr.stage === 'pause' && this.turns === 2 && !this.failedTold.has(2) && t < BEAT.steps) { this.failedTold.add(2); d.toast('It turns over. Nearly.', 2.4); }
      if (tr.stage === 'pause' && this.turns === 3 && !this.failedTold.has(3) && t < BEAT.steps && this.keyProof) { this.failedTold.add(3); d.toast('The third turn. It does not catch this time.', 2.8); }
      // it caught: they let go
      if (tr.stage === 'catching') { this.free = 'key'; this.release(); return; }
    } else {
      // (out of it: the engine is dead for whoever gets back in)
      if (!this.gotOut) d.keyStall();
      this.gotOut = true; this.wasOut = true;
      this.north = Math.max(this.north, s.north);
      if (s.away >= RUN) { this.free = 'ran'; this.release(); return; }
    }
    // the radio: the bare carrier from the start to the dark (and none after)
    if (t >= 0.9 && t < BEAT.dark) d.radio.carrier(0.5);
    if (t >= BEAT.dark) d.radio.carrier(0);
    // :16, the truck's own sound gone (the engine's note, the tyres, the starter)
    d.hush(t >= BEAT.steps);
    // :41, three knocks on the roof of the cab
    if (t >= KNOCK_AT && !this.knocked) {
      this.knocked = true;
      const at = d.roof();
      for (const w of KNOCKS) d.audio.play('thudSoft', { gain: 1.1, rate: 0.62, lp: 900, when: w, ...(at && !s.inCab ? { at } : {}) });
    }
  }
  private failedTold = new Set<number>();

  /** The key in the cab (main.ts, Use): after :40 there is nothing in the battery to turn it with. */
  useKey(warnT: number | null): boolean {
    if (this.phase !== 'warning' || this.free || warnT === null || warnT < BEAT.dark) return false;
    this.d.audio.play('click', { gain: 0.35, rate: 0.55 });
    if (!this.toldDead) { this.toldDead = true; this.d.toast('The key turns. Nothing. Not even the starter.', 3); }
    return true;
  }

  /** The truck lets go: its engine, its lights, its sound and the radio its own again. */
  private release() {
    const d = this.d;
    d.hold(0); d.hush(false); d.trouble.dim = 1; d.radio.carrier(0);
    if (d.trouble.kind === 'stall' && d.trouble.stage !== 'catching') d.trouble.catchOn(1);
    this.failedTold.clear();
  }

  private takenStep() {
    const d = this.d;
    // the light and the taking: the truck dark and still (the abduction has the rest)
    d.hold(HELD); d.trouble.dim = 0; d.radio.carrier(0);
  }

  // ---------- the waking (abduction.ts, AbductDeps.cab.wake) ----------
  /** The truck put down 300 m on, the nose north; the player at its wheel (inCab) or not. Where it
   *  stands now (world, on the ground), or null. */
  wake(_lost: number, inCab: boolean): THREE.Vector3 | null {
    const d = this.d, p = this.plan ?? (() => { const tr = d.truck(); return tr ? { x: LANE_X, z: Math.min(LAST_Z, tr.z + ON_FROM) } : null; })();
    this.phase = 'wake'; this.t = 0; this.takenIn = inCab; this.wokeHere = true;
    d.hold(0); d.hush(false);
    if (!p) return null;
    const moved = inCab ? d.moveDriven(p.x, p.z, 0) : d.moveParked(p.x, p.z, 0);
    if (!moved) return null;
    if (inCab) {
      // the engine off and quiet, every light off: the key starts it, first time
      d.trouble.start('stall', { key: true, tries: 1, dead: true });
      d.trouble.dim = 0; this.dashK = 0;
      d.headlights(false);
    } else { d.trouble.dim = 1; d.keyStall(); }
    this.lightsOwed = inCab;
    // the radio on, on 1090: the far station, coming and going in its static
    d.radio.set(true, 'far'); d.radio.carrier(0);
    // the odometer: 31.4 miles more than it read when the truck set out from SARO
    const from = d.odoAtSaro() ?? d.odometer(), more = from + MILES - d.odometer();
    if (more > 0) d.addMiles(more);
    this.frostK = 1; d.frost(1);
    return new THREE.Vector3(p.x, d.ground(p.x, p.z), p.z);
  }
  /** The notes of it (after the time away's own three). */
  woke(_a: string, b: string, inCab: boolean) {
    // (the clock the warning came at, as the corner clock had it, not the light's)
    const d = this.d, k = this.knocksOwed, a = clockText(this.at, false);
    const knocks = k ? 'Then three knocks on the roof of the cab, slow, from above' : 'Then the three knocks on the roof of the cab again, slow, from above';
    d.note(inCab
      ? `${a}. The turn-off to SARO, on B. The radio slid over to a bare carrier, the engine died under me and the headlights sank. ${this.turns ? 'The key would not catch.' : 'I never turned the key.'} At forty seconds every light in the truck went out. ${knocks}, and the light.`
      : `${a}. The turn-off to SARO, on B. The engine died under me and the radio went to a bare carrier${this.turns ? ', and the key would not catch' : ''}. I got out, and did not get far enough. ${knocks}, behind me, and the light.`);
    d.note(inCab
      ? `${b}. At the wheel, three hundred metres further on, in the southbound lane with the nose to the north. The engine off, every light off. The radio on, on 1090. I never have it on 1090.`
      : `${b}. On my back on the verge. The truck beside me, three hundred metres on from where I left it, the nose to the north. Its radio on, on 1090.`);
    d.note(`${b}. The odometer reads ${MILES} miles more than it did at SARO.`);
    d.seen.add(`tt:taken@${Math.round(this.at)}`);
  }
  backText() { return 'At the wheel. The truck stands in the southbound lane, facing north. The engine is off. The radio is on, on 1090.'; }

  private wakeStep(dt: number, s: TruckTakenIn) {
    const d = this.d;
    this.t += dt;
    // dark while one of them is there; then the instruments come on by themselves, OIL and VOLTS,
    // and the radio on 1090 with them
    if (s.over) this.overSeen = true;
    if (this.takenIn && s.inCab) {
      const want = s.over || (s.stage !== 'calm' && s.stage !== 'back') ? 0 : 1;
      this.dashK += (want - this.dashK) * Math.min(1, dt * 2.5);
      if (d.trouble.kind === 'stall') d.trouble.dim = this.dashK > 0.98 ? 1 : this.dashK;
    }
    if (s.stage === 'calm' && this.t > 2) {
      if (this.knocksOwed && !d.seen.has('ne:knocks1')) d.seen.add('ne:knocks1');
      this.ended = 'taken';
      d.toast(`The odometer reads ${MILES} miles more than it did at SARO.`, 4.6);
      d.save();
      this.phase = 'tail'; this.t = 0;
    }
  }

  // ---------- endings ----------
  private end(how: TruckTakenEnd, s: TruckTakenIn) {
    const d = this.d, a = clockText(this.at, false);
    this.ended = how;
    if (how === 'key') {
      d.seen.add(`tt:key@${Math.round(this.at)}`);
      d.toast('It catches. The lights come up, and the radio.', 3.4);
      d.note(`${a}. The turn-off to SARO, on B. The radio slid over to a bare carrier, the engine died under me and the headlights sank. The crickets had stopped. It caught on the third turn of the key, and it all came back at once: the engine, the lights, the radio.`);
    } else if (how === 'ran') {
      d.seen.add(`tt:ran@${Math.round(this.at)}`);
      d.keyStall();
      // (out and away after the knocks had come: heard, they were the night's, if it still owed them)
      if (this.knocked && this.knocksOwed && !d.seen.has('ne:knocks1')) d.seen.add('ne:knocks1');
      const knocks = this.knocked ? ' Behind me, three knocks on the roof of the cab, slow, from above.' : '';
      // (on the screen too, as the key's way out has its toast: the player is out in the dark with nothing else)
      d.toast('The crickets, all at once. Behind you the truck stands in the road with its lights out.', 4.2);
      d.note(`${a}. The turn-off to SARO, on B. The engine died under me, the radio went to a bare carrier and the headlights sank${this.turns ? ', and the key would not catch' : ''}. I got out and ran${this.north > RUN * 0.6 ? ', back up the shoulder towards SARO' : ''}.${knocks} When I looked back, the truck was standing in the road with its lights out. Then the crickets again.`);
    }
    d.save();
    this.phase = how === 'off' ? 'idle' : 'tail'; this.t = 0;
    void s;
  }

  /** A new night, or a save opened, or the title: nothing running, the truck its own. */
  reset() {
    if (this.phase === 'warning' || this.phase === 'taken') this.release();
    if (this.wokeHere || this.frostK > 0) this.d.frost(0);
    this.lightsOwed = false;
    this.phase = 'idle'; this.t = 0; this.free = null; this.armed = false; this.pass = null; this.dist = null; this.plan = null;
    this.wokeHere = false; this.frostK = 0; this.force = false; this.ladderPasses = 0; this.failedTold.clear();
  }
  /** One line for the ?debug overlay. */
  text() { return `truck taken ${this.phase}${this.taken ? ' had' : ''} tries ${this.tries}${this.dist === null ? '' : ` ${this.dist.toFixed(0)}m`}${this.free ? ' ' + this.free : ''}`; }
}

/** The warning's length (the tests). */
export const TRUCK_WARN = WARN;
