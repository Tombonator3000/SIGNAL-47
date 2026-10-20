import type { DriveController } from './Drive';
import type { Truck } from './Truck';
import type { EngineSound } from './engineSound';

// Trouble with the truck (Tom, 16 October): an engine that misfires or dies on the road, and
// instrument lights and headlights that flicker or go out. Made for the story's events later
// and for strange things that happen at random on a night drive. World.ts steps it every frame
// of a trip with the truck that is being driven, and it only touches the truck while something
// is wrong; afterwards the lights, the instruments and the engine are given back as they were.
//
//   sputter  the engine misfires for a second or two: the pull comes and goes, it coughs.
//   stall    the engine dies: the truck coasts, OIL and VOLTS light up, the headlights sag on
//            the battery. Then the key: the starter turns it over (the lights dip with it), it
//            may take two tries, and it catches. hold: it stays dead until release(); key: the
//            player turns the key (Use, World.useKey) instead of the driver doing it.
//   flicker  a bad contact: dash and headlights stutter for a moment.
//   blink    the same contact, once: the lights all but out for a blink, and a stutter as they
//            come back (story/curve.ts: a sign in its build; the shape at the end of the
//            headlights that is a post, core/cutouts.ts).
//   dashOut  the instrument lights go dark; the fuel and temperature needles fall back (they
//            are electric), the speedometer goes on (it runs off a cable).
//
// Random trouble is off until the story turns it on (`random`); the developer menu can too.

export type TroubleKind = 'sputter' | 'stall' | 'flicker' | 'blink' | 'dashOut';
export interface TroubleOpts { len?: number; hold?: boolean; key?: boolean }

type Stage = 'dying' | 'dead' | 'pause' | 'cranking' | 'catching';
interface Active {
  kind: TroubleKind; t: number; len: number;
  // the stall
  stage?: Stage; st: number; hold: boolean; key: boolean; tries: number; failFirst: boolean;
  /** The stage whose sound has been started (once per stage). */
  heard?: Stage;
  // the patterns: misfires and flickers, a value held until its time runs out
  beat: number; v: number;
}

const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export class TruckTrouble {
  /** Trouble at random on drives (the story or the developer menu turns it on). */
  random = false;
  /** About how many seconds of driving between random troubles. */
  every = 300;
  /** Each trouble as it starts (the story, tests). */
  onTrouble?: (kind: TroubleKind) => void;
  /** How many troubles have started (tests). */
  count = 0;
  private a: Active | null = null;
  private next = rnd(0.5, 1.5);
  private touched = false;   // the truck has been changed and must be given back
  private power = 1;

  /** What is wrong now, or null. */
  get kind() { return this.a?.kind ?? null; }
  /** The engine is dead and waits for the player to turn the key (Use). */
  get wantsKey() { const a = this.a; return !!a && a.kind === 'stall' && a.key && !a.hold && (a.stage === 'dead' || a.stage === 'pause'); }
  /** What the radio gets, 0..1 (core/radio.ts): nothing while the starter turns, a stutter with
   *  the lights. On the battery alone it plays on. */
  get radioLevel() { return this.power; }
  /** The engine is dead (or being started). */
  get dead() { const a = this.a; return !!a && a.kind === 'stall' && a.stage !== 'catching'; }

  /** Start a trouble now (the story, the developer menu, tests). A running one is replaced,
   *  except a stall, which only release() or the key ends. */
  start(kind: TroubleKind, o: TroubleOpts = {}) {
    if (this.a?.kind === 'stall' && kind !== 'stall') return false;
    const len = o.len ?? (kind === 'sputter' ? rnd(1.2, 2.6) : kind === 'flicker' ? rnd(0.8, 2.4) : kind === 'blink' ? 0.55 : kind === 'dashOut' ? rnd(2.5, 6) : 0);
    this.a = { kind, t: 0, len, st: 0, hold: !!o.hold, key: !!o.key, tries: 0, failFirst: Math.random() < 0.55, beat: 0, v: 1 };
    if (kind === 'stall') this.a.stage = 'dying';
    this.count++;
    this.onTrouble?.(kind);
    return true;
  }
  /** A held stall may be started again (the story lets go of it). */
  release() { if (this.a?.kind === 'stall') { this.a.hold = false; if (this.a.stage === 'dead') this.a.st = 0; } }
  /** The player turns the key (when wantsKey). */
  turnKey() { const a = this.a; if (!this.wantsKey || !a) return false; a.stage = 'cranking'; a.st = 0; return true; }
  /** Everything as it was: the engine runs, the lights and the instruments are the truck's own. */
  reset(truck?: Truck | null, drive?: DriveController | null) {
    this.a = null; this.power = 1;
    if (drive) { drive.engine = true; drive.power = 1; }
    if (truck && this.touched) { truck.setLightLevel(1); truck.setDashLevel(null); truck.setWarnings({}); truck.setGaugePower(1); }
    this.touched = false;
  }

  /** The truck goes on in another area (World.transfer): the copy left behind gets its own
   *  lights and engine back; the new one takes the trouble on from the next step. */
  handOver(truck: Truck, drive: DriveController) {
    if (!this.touched) return;
    drive.engine = true; drive.power = 1;
    truck.setLightLevel(1); truck.setDashLevel(null); truck.setWarnings({}); truck.setGaugePower(1);
  }

  /** Every frame of a trip, after the drive has moved. random: whether random trouble may
   *  come now (the story's say, the night, a cinematic). */
  step(dt: number, drive: DriveController, truck: Truck, sound: EngineSound | null, mayRandom: boolean) {
    // ---------- the random ones: only at speed, now and then ----------
    if (this.random && mayRandom && !this.a && drive.engine && Math.abs(drive.speed) > 6) {
      this.next -= dt / this.every;
      if (this.next <= 0) {
        this.next = rnd(0.5, 1.5);
        const r = Math.random();
        this.start(r < 0.4 ? 'flicker' : r < 0.7 ? 'sputter' : r < 0.85 ? 'dashOut' : 'stall');
      }
    }
    const a = this.a;
    if (!a) { this.power = 1; if (this.touched) this.reset(truck, drive); return; }
    a.t += dt;
    this.touched = true;
    let light = 1, dash = 1, gauges = 1;
    const warn = { oil: false, volts: false, brake: false };
    // a pattern: hold a value for a short random time, then pick another
    const beat = (lo: number, hi: number, pick: () => number) => { a.beat -= dt; if (a.beat <= 0) { a.beat = rnd(lo, hi); a.v = pick(); } return a.v; };

    if (a.kind === 'sputter') {
      // misfires: the pull drops out in bursts, each with a cough
      const was = a.v;
      const p = beat(0.07, 0.24, () => (Math.random() < 0.55 ? rnd(0.05, 0.3) : 1));
      drive.power = p;
      if (p < 0.5 && was >= 0.5) sound?.cough(0.6 + Math.random() * 0.4);
      dash = p < 0.5 ? 0.85 : 1;
      warn.volts = p < 0.2;
      if (a.t > a.len) { drive.power = 1; this.a = null; }
    } else if (a.kind === 'flicker') {
      const k = beat(0.03, 0.16, () => (Math.random() < 0.5 ? rnd(0, 0.25) : rnd(0.7, 1)));
      light = k; dash = k;
      if (a.t > a.len) this.a = null;
    } else if (a.kind === 'blink') {
      light = a.t < 0.14 ? 0.03 : a.t < 0.42 ? beat(0.03, 0.07, () => (Math.random() < 0.45 ? rnd(0.15, 0.5) : 1)) : 1;
      dash = light;
      if (a.t > a.len) this.a = null;
    } else if (a.kind === 'dashOut') {
      // out at once with a stutter at both ends
      const edge = Math.min(a.t, a.len - a.t);
      dash = edge < 0.25 ? beat(0.03, 0.08, () => (Math.random() < 0.5 ? 0 : 1)) : 0;
      gauges = 0;
      if (a.t > a.len) this.a = null;
    } else this.stall(dt, a, drive, sound, (l, d, w) => { light = l; dash = d; Object.assign(warn, w); });

    this.power = this.a?.kind === 'stall' && this.a.stage === 'cranking' ? 0.04 : this.a?.kind === 'flicker' || this.a?.kind === 'blink' ? light : 1;
    truck.setLightLevel(light);
    truck.setDashLevel(dash);
    truck.setGaugePower(gauges);
    truck.setWarnings(warn);
  }

  private stall(dt: number, a: Active, drive: DriveController, sound: EngineSound | null, out: (light: number, dash: number, warn: { oil?: boolean; volts?: boolean }) => void) {
    a.st += dt;
    const ign = { oil: true, volts: true };   // the key on and the engine stopped: oil pressure and charge lamps
    switch (a.stage) {
      case 'dying':
        if (a.heard !== 'dying') { a.heard = 'dying'; sound?.stall(); }
        drive.engine = false;
        // the alternator stops: the lights sag onto the battery, with a stumble on the way
        out(a.st < 0.6 ? (Math.random() < 0.3 ? 0.4 : 0.8) : 0.72, 0.85, a.st > 0.5 ? ign : {});
        if (a.st > 1.3) { a.stage = 'dead'; a.st = 0; }
        break;
      case 'dead':
        a.heard = 'dead';
        out(0.72, 0.85, ign);
        // the driver turns the key once the truck has slowed (or after a while), unless the story holds it
        if (!a.hold && !a.key && (a.st > 2.4 || (Math.abs(drive.speed) < 2 && a.st > 1.2))) { a.stage = 'cranking'; a.st = 0; }
        break;
      case 'pause':
        out(0.72, 0.85, ign);
        if (!a.key && a.st > 0.9) { a.stage = 'cranking'; a.st = 0; }
        break;
      case 'cranking': {
        if (a.heard !== 'cranking') { a.heard = 'cranking'; sound?.crank(1.2); }
        // the starter pulls the battery down: the lights dip in time with the compression strokes
        const k = 0.5 + 0.5 * Math.sin(a.st * Math.PI * 2 * 4.5);
        out(0.28 + 0.12 * k, 0.4 + 0.15 * k, ign);
        if (a.st > 1.15) {
          a.tries++;
          if (a.hold || (a.failFirst && a.tries === 1)) { a.stage = 'pause'; a.st = 0; a.heard = 'pause'; }
          else { a.stage = 'catching'; a.st = 0; drive.engine = true; drive.power = 0.3; sound?.restart(); }
        }
        break;
      }
      case 'catching':
        // it fires, runs rough for a moment and settles; the lamps go out as the oil pressure comes up
        drive.power = Math.min(1, 0.3 + a.st * 0.7);
        out(Math.min(1, 0.75 + a.st * 0.4), 1, a.st < 0.35 ? ign : {});
        if (a.st > 1.0) { drive.power = 1; this.a = null; }
        break;
    }
  }
}
