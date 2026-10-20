import type { TruckTrouble } from '../drive/trouble';
import type { Receiver } from '../core/radio';
import { offset, distTo, LINES, type MapPt } from './lines';
import { clockText } from './time';

// Four, and not yet seven: the truck on B (FORSLAG.md, 20 October, item 8). A ladder towards 05:29
// in the truck, for those who drive everywhere: the signs in the car (HENDELSER.md, section 5) as a
// count that goes up, on the one line the truck can cross. B crosses the highway at SARO's turn-off,
// 23 m south of the site (story/lines.ts, CROSSINGS); the truck is on it every time it turns out of
// the service road onto the highway or back, and when it runs down the highway past the turn-off.
// Nowhere else: south of the motel B is far off to the west, and the old road is C's.
//
//   four    from chapter two, the first time the truck goes over B: the radio drops to a bare
//           carrier for a second (switched off, it gives one anyway), the engine misses, and every
//           light in the truck goes out four times, evenly, a relay ticking in the dash with each.
//   seven   once room 6 is over, the next time over B: the engine dies, OIL and VOLTS, the radio on
//           its carrier. The key (Use, as after the time away): the first turn does not catch, the
//           second does, and then every light goes out seven times. Most come out of room 6 to the
//           truck on the motel's lot and drive south, and that way never goes over B again: from
//           the motel's lot (B's ground, 400 m off the line, as the office's television is: the
//           town's marks on B, story/lines.ts) it comes as the truck pulls out onto the highway.
//   compass the ball compass on the instrument hood (drive/Truck.ts) reads 026 over B, from chapter
//           two: the card swings round to it and stays while the truck turns, and back after; it
//           reads 026 the whole time the truck is in the ladder. On the old road (chapter six) it
//           slides towards 026 from mile 6, and is on it before the line, wherever the road turns.
//
// The blinks count up, four and then seven, so the dash blinking four and seven at 05:29
// (story/Chapter6.ts) is the third time and the whole of it; the headlights' colours there are not
// used here. The key this teaches is the one the last mile takes away (FORSLAG.md, item 6): the
// stall's `tries` (drive/trouble.ts) can be Infinity.
//
// One thing at a time: these are the story's, on a place, as the office's telephone is
// (story/officePhone.ts): the night's curve does not time them, but they never come on top of another
// thing (main.ts, oneAtATime: then the crossing is missed and the next one tries again), and nothing
// starts while they run or for TAIL seconds after. Four is a strong sign: it takes the build's event
// (story/curve.ts reads it from the log), but not the peak; it is three seconds and asks nothing of the
// player. Seven is a peak (FELT): it stops the truck, takes the key twice and holds the player still,
// so the rest after it gives the restart and the count room, and nothing of the night lands on it.
// No saving while it runs (main.ts, saveBlock). Kept in the town's part of the case ('truckB:4',
// 'truckB:7', and a mark on B on the notebook's map); the seen log gets them through the marks.

export type TruckOnBPhase = 'idle' | 'four' | 'stall' | 'seven' | 'tail';

export interface TruckOnBIn {
  /** In play, no panel, no cinematic, and nothing else of the night's going (one at a time). */
  ok: boolean;
  /** Driving on a trip, the story's or the player's own (not a free drive, not chapter six's road). */
  trip: boolean;
  /** The driven truck on the map (road-local metres), or null where the area has no place on it. */
  at: MapPt | null;
  /** The truck's speed, m/s. */
  speed: number;
  /** Chapters two to five: B has the compass, and the ladder may begin. */
  open: boolean;
  /** Room 6 is over: the next rung is seven. */
  after6: boolean;
  clock: number;
  /** On chapter six's old road before the line: miles from the diner; else null. */
  oldMi: number | null;
  /** The trip (World.tripNow): where it began (the place the truck was left at) and the metres
   *  driven so far; null off a trip. */
  from: string | null;
  metres: number;
  /** Settings, Flashing images: slow fades (the blinks dip softly instead of going out). */
  calm: boolean;
}
export interface TruckOnBDeps {
  trouble: TruckTrouble;
  /** The truck's radio (main.ts, carRadio). */
  radio: Receiver;
  /** The town's part of the case (main.ts, townSeen). */
  seen: Set<string>;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  /** A relay in the dash, with each blink. */
  tick: () => void;
  save: () => void;
}

/** Within this far of B (metres, the truck's middle) the truck is over it; it must be this far off
 *  again before the next time counts. The turn out of the service road passes 3.7 to 5 m from it. */
const ON = 6, REARM = 16;
/** Over B the compass reads 026 within NEAR metres, and lets go by FAR. */
const NEAR = 5, FAR = 16;
/** Seconds nothing else of the night's may start after a rung (one at a time). */
const TAIL = 8;
/** From the motel's lot: the truck out on the highway (|map x| under OUT_X, the asphalt), this many
 *  metres into the drive and at speed; past GIVE_UP metres (something else was going all the way
 *  out of B's ground) not on this drive. */
const OUT_X = 4.5, OUT_M = 10, GIVE_UP = 220;
/** The old road: the compass leaves the road's heading at mile 6 and is on 026 by 7.6. */
const OLD_FROM = 6.0, OLD_TO = 7.6;
export const BEARING = 26;

const smooth = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };

export class TruckOnB {
  phase: TruckOnBPhase = 'idle';
  /** Seconds into the phase. */
  t = 0;
  /** Crossings of B that came while something else was going (the tests): missed, tried again. */
  missed = 0;
  /** Crossings of B counted (the tests). */
  crossings = 0;
  /** The blinks of the rung that is running, as the dash showed them (the tests). */
  blinked = 0;
  /** The truck's distance from B now (m), or null off the map (the tests, ?debug). */
  dist: number | null = null;
  /** The compass's pull now: towards 026, by k (Truck.compassPull), or null. */
  pull: { deg: number; k: number } | null = null;
  /** Stands aside: no rung begins (the older tests of other things that drive out of SARO; the
   *  compass still reads 026 over B). */
  off = false;
  private armed = false;
  private radioWasOn = false;
  private failed = false;
  /** Where a rung's script is (0 at its start), and when the stall caught. */
  private step = 0;
  private caughtAt = 0;
  /** The clock the rung began at (the note's time). */
  private at = 0;
  private toldOld = false;
  /** Where the seven came (the note, the mark): over B at the turn-off, or pulling out of the motel. */
  private where: 'turnoff' | 'motel' = 'turnoff';
  /** The motel's lot this drive: tried while something else was going (counted missed once). */
  private motelMissed = false;
  /** Slow fades, as the rung began. */
  private calm = false;

  constructor(private d: TruckOnBDeps) {}

  /** Which rungs the night has had: four, seven. */
  get four() { return this.d.seen.has('truckB:4'); }
  get seven() { return this.d.seen.has('truckB:7'); }
  /** Something of it is going (one at a time: main.ts, oneAtATime). */
  get busy() { return this.phase !== 'idle'; }
  /** A rung is running: no saving (main.ts, saveBlock). */
  get racing() { return this.phase === 'four' || this.phase === 'stall' || this.phase === 'seven'; }
  /** The stall and its seven: the peak's own thing (the curve's rest counts from when it is over). */
  get peaking() { return this.phase === 'stall' || this.phase === 'seven'; }

  update(dt: number, s: TruckOnBIn) {
    const off = s.at ? Math.abs(offset('B', s.at[0], s.at[1])) : null;
    this.dist = off;
    if (off === null || off > REARM || !s.trip) this.armed = off === null || off > REARM;
    // ---------- a rung that is running ----------
    if (this.racing && !s.trip) this.abandon(s.clock);
    if (this.phase !== 'idle') { this.t += dt; this.run(s); }
    else if (s.trip && s.open && this.armed && off !== null && off < ON && Math.abs(s.speed) > 1.5) {
      // ---------- over B ----------
      this.armed = false;
      this.crossings++;
      const rung = this.seven || this.off ? null : s.after6 ? 'stall' : this.four ? null : 'four';
      if (rung) {
        if (!s.ok || this.d.trouble.kind) this.missed++;
        else this.begin(rung, s, 'turnoff');
      }
    } else if (s.trip && s.open && s.after6 && !this.seven && !this.off && this.outOfMotel(s)) {
      // ---------- after room 6, out of the motel's lot onto the highway ----------
      if (!s.ok || this.d.trouble.kind) { if (!this.motelMissed) { this.motelMissed = true; this.missed++; } }
      else this.begin('stall', s, 'motel');
    }
    if (!s.trip) this.motelMissed = false;
    // ---------- the compass ----------
    let k = 0;
    if (this.phase === 'four' || this.phase === 'stall' || this.phase === 'seven' || (this.phase === 'tail' && this.t < 2.5)) k = 1;
    else if (s.oldMi !== null) k = smooth(OLD_FROM, OLD_TO, s.oldMi);
    else if (s.open && off !== null) k = 1 - smooth(NEAR, FAR, off);
    this.pull = k > 0 ? { deg: BEARING, k } : null;
    if (s.oldMi !== null && k > 0.97 && !this.toldOld && s.ok) {
      this.toldOld = true;
      this.d.toast('The compass on the dash has come round to 026. The road has not.', 4.4);
    }
  }

  /** A drive from the motel's lot, out on the highway and still on B's ground (the seven's other way in). */
  private outOfMotel(s: TruckOnBIn) {
    return s.from === 'motel' && !!s.at && s.metres > OUT_M && s.metres < GIVE_UP && Math.abs(s.at[0]) < OUT_X
      && Math.abs(s.speed) > 3 && distTo('B', s.at[0], s.at[1]) <= LINES.B.reach;
  }

  private begin(rung: 'four' | 'stall', s: TruckOnBIn, where: 'turnoff' | 'motel') {
    const d = this.d, clock = s.clock;
    this.phase = rung; this.t = 0; this.blinked = 0; this.failed = false; this.step = 0; this.caughtAt = 0; this.at = clock; this.where = where;
    this.calm = s.calm;
    this.radioWasOn = d.radio.on;
    if (rung === 'four') {
      d.radio.carrier(1.1);
      d.seen.add(`mark:truck4@${Math.round(clock)}`);
      d.toast(this.radioWasOn ? 'The radio drops to a bare carrier. The engine misses.' : 'The radio is off. A carrier comes out of it anyway. The engine misses.', 2.6);
    } else {
      d.trouble.start('stall', { key: true, tries: 2 });
      d.radio.carrier(3);
      d.seen.add(`mark:${where === 'motel' ? 'truck7m' : 'truck7'}@${Math.round(clock)}`);
      d.toast('The engine dies. OIL and VOLTS. The radio goes to a bare carrier.', 3.4);
    }
  }

  private run(s: TruckOnBIn) {
    const d = this.d, tr = d.trouble, t = this.t;
    // a relay ticks in the dash with each blink, as every light goes out
    const n = tr.blinks;
    if (tr.kind === 'blink' && n > this.blinked) { this.blinked = n; d.tick(); }
    switch (this.phase) {
      case 'four':
        // the carrier (begin), the engine missing under it, then the count
        if (t >= 0.12 && this.step === 0) { this.step = 1; tr.start('sputter', { len: 0.85 }); }
        if (t >= 1.15 && this.step === 1) { this.step = 2; tr.start('blink', { count: 4, soft: this.calm }); }
        if (this.step === 2 && tr.kind !== 'blink') this.done('four', s.clock);
        break;
      case 'stall':
        // the carrier holds while it is dead (the starter takes the radio down with the lights), and
        // through the seven after it
        d.radio.carrier(0.5);
        if (tr.stage === 'pause' && tr.turns === 1 && !this.failed) { this.failed = true; d.toast('It turns over and will not catch.', 2.6); }
        // it caught and settled: a breath, then seven
        if (!tr.kind && this.step === 0) { this.step = 1; this.caughtAt = t; }
        if (this.step === 1 && t - this.caughtAt >= 0.9) { this.phase = 'seven'; this.t = 0; this.step = 0; tr.start('blink', { count: 7, soft: this.calm }); }
        break;
      case 'seven':
        d.radio.carrier(0.3);
        if (tr.kind !== 'blink') { d.radio.carrier(0); this.done('seven', s.clock); }
        break;
      case 'tail':
        if (t >= TAIL) this.phase = 'idle';
        break;
    }
  }

  private done(rung: 'four' | 'seven', clock: number) {
    const d = this.d, at = clockText(this.at || clock, false);
    if (rung === 'four') {
      d.seen.add('truckB:4');
      d.toast('Four. Every light in the truck, four times. The compass on the dash reads 026.', 4.6);
      d.note(`${at}. The turn-off to SARO, on B. ${this.radioWasOn ? 'The radio went to a bare carrier' : 'The radio was off and gave a carrier anyway'}, the engine missed, and every light in the truck went out four times. Evenly, like counting. The compass on the dash swung round to 026 and held it.`);
    } else {
      d.seen.add('truckB:7');
      d.toast('Seven.', 3);
      const place = this.where === 'motel' ? 'Out of Sierra Motor Court onto the highway' : `The turn-off to SARO, on B${this.four ? ', again' : ''}`;
      d.note(`${at}. ${place}. The engine died under me, OIL and VOLTS lit, the radio on a carrier. It caught on the second turn of the key. Then every light in the truck went out seven times.${this.four ? ' Four, the last time.' : ''}`);
    }
    d.save();
    this.phase = 'tail'; this.t = 0; this.step = 0;
  }

  /** The drive ended in the middle of a rung (the truck left, the night put away): what was seen of
   *  it stands, so it is not given again. */
  private abandon(clock: number) {
    const rung = this.phase === 'four' ? 'four' : 'seven';
    this.d.radio.carrier(0);
    this.done(rung, clock);
    this.phase = 'idle';
  }

  /** A new night, or a save opened: nothing running (the rungs the night has had are in its keys). */
  reset() {
    this.phase = 'idle'; this.t = 0; this.blinked = 0; this.step = 0;
    this.armed = false; this.pull = null; this.dist = null; this.toldOld = false; this.where = 'turnoff'; this.motelMissed = false;
    this.missed = 0; this.crossings = 0;
  }
  /** One line for the ?debug overlay. */
  text() { return `truck on B ${this.phase}${this.four ? ' 4' : ''}${this.seven ? ' 7' : ''}${this.dist === null ? '' : ` ${this.dist.toFixed(0)}m`}${this.pull ? ` 026 x${this.pull.k.toFixed(2)}` : ''}`; }
}
