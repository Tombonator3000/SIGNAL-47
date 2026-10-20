import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Station01 } from '../world/Station01';
import { BEAT } from './abduction';

// At STATION 01 everyone goes (FORSLAG.md, 20 October, item 9 A; Tom: "Spenning og skummelt slår
// faste regler"). Until now "at least one time away before chapter five" was only a push on the
// pressure (story/abduction.ts, `late`). Now a player who has not been taken by the time N. Vega
// has called the field telephone in the hut gets the warning on the way out from it, the 47
// seconds of it, and this one cannot be got away from. The two ways out there are closed, both
// in the world and both fairly: a step or two out of the door, the hut door swings shut behind
// the player by itself, latches, and the light inside goes out (it does not give again until it is
// over); and the truck by the gate, whose keys are always in it, has none in it. Run, and the run
// is real: through the walk gate to the truck, back to the hut, out among the stakes. It goes on
// counting. At :40, with the flashlight, the lamps outside go out too, the yard light on its pole,
// the bulb over the door, the shed's: seven seconds of the dark, and the light.
//
// On waking: the door stands open into the room again as it has for years, the light is on, the
// lamps outside are on, and the truck's keys are lying in the frost by the player's hand.
//
// Only with Missing time on (off, nothing of it). Not if the player has been taken already
// tonight. The lost time needs its 17 minutes before 04:35 (chapter three's fixed point): a call
// that ends after about 04:16 is let go, and the night's own director still has chapter four's
// push (story/abduction.ts). One at a time: it waits for anything else of the night's to be over.
// It is the story's and not the night's curve's (as SARO dark at 02:29 is): it comes whatever the
// curve's phase is, and as the warning it lands the peak (story/curve.ts). Kept with the case
// (AbductSave.station): never twice; a save from before it brings it back.

/** The door shuts this long after the warning begins, in this long; the light inside goes out after it. */
const DOOR_AT = 0.35, DOOR_T = 1.15, LIGHT_AT = 2.0;
/** The player is this far out of the hut door before it begins (metres). */
export const OUT_OF_DOOR = 2.4;
/** The lost time's 17 minutes, a minute's room, and the warning's 47 s and the light's 3 s before
 *  chapter three's fixed point (04:35): the last clock it may begin at. */
export const LAST = 4 * 3600 + 35 * 60 - (17 * 60 + 60 + 47 + 3);

export interface StationTakenDeps {
  site: () => Station01 | null;
  audio: AudioSys;
  toast: (text: string, secs?: number) => void;
  note: (text: string) => void;
  clockText: (c: number) => string;
}
export interface StationIn {
  /** The time away's stage and how far into the warning (seconds; null outside it). */
  stage: string;
  warnT: number | null;
  clock: number;
}

export class StationTaken {
  /** idle: nothing (or over); shut: the warning there is under way, the door shut, the keys gone. */
  state: 'idle' | 'shut' = 'idle';
  /** The keys are not in the truck (the truck at the station says so). */
  keysGone = false;
  /** How it went the last time (the tests): 'taken' or 'passed'. */
  last: 'taken' | 'passed' | null = null;
  private t = 0;
  private doorK = 0;
  private lit = true;
  private lamps = true;
  /** Taken: the keys and the note wait until the player is up again. */
  private pending = false;
  private triedTruck = 0;
  private triedDoor = 0;
  private from = 0;
  private wasStage = 'calm';
  private v = new THREE.Vector3();

  constructor(private d: StationTakenDeps) {}

  /** The hut door is shut (the hut says so when it is tried). */
  get hutShut() { return this.state === 'shut' && this.doorK > 0.8; }

  /** The forced warning has begun here (main.ts has started it on the time away). */
  begin(clock: number) {
    this.state = 'shut'; this.t = 0; this.doorK = 0; this.lit = true; this.lamps = true; this.keysGone = true;
    this.triedTruck = 0; this.triedDoor = 0; this.from = clock; this.wasStage = 'warning'; this.last = null;
  }

  update(dt: number, s: StationIn) {
    const was = this.wasStage;
    this.wasStage = s.stage;
    // up again: the keys by the hand, and it written down
    if (this.pending && s.stage === 'calm' && was === 'back') {
      this.pending = false;
      this.d.toast('The truck\'s keys are lying in the frost by your hand.', 4);
      this.d.note(`${this.d.clockText(this.from)}. STATION 01, on the way out from the hut after N. Vega's call. The hut door swung shut behind me by itself, and the light inside went out. The keys were not in the truck. There was nowhere to go. When I came to, the keys were in the frost beside me, and the door stood open again with the light on.`);
    }
    if (this.state !== 'shut') return;
    const site = this.d.site();
    // the warning passed after all (Missing time turned off on the way, the night left): put back
    if (s.stage === 'calm' && was === 'warning') { this.putBack(); this.last = 'passed'; return; }
    // taken: while it is black, the door open again and the lights on (the keys will be by the hand)
    if (s.stage === 'black' || s.stage === 'back') { this.putBack(); this.last = 'taken'; this.pending = true; return; }
    if (s.stage !== 'warning' && s.stage !== 'light') return;
    this.t += dt;
    if (!site) return;
    // the door swings shut behind the player, latches; the light inside goes out
    if (this.t >= DOOR_AT && this.doorK < 1) {
      if (this.doorK === 0) {
        const at = this.doorAt(site);
        this.d.audio.play(Math.random() < 0.5 ? 'doorCreak0' : 'doorCreak1', { gain: 0.2, rate: 0.82, at });
        this.d.audio.play(this.d.audio.or('door_light', 'doorClose'), { gain: 0.55, when: DOOR_T * 0.82, at });
        this.d.audio.play(this.d.audio.or('door_latch', 'switch'), { gain: 0.4, rate: 0.9, when: DOOR_T * 0.82 + 0.12, at });
      }
      this.doorK = Math.min(1, this.doorK + dt / DOOR_T);
      site.setHutDoor(this.doorK);
      if (this.doorK >= 1) this.d.toast('Behind you, the hut door swings shut by itself, and latches.', 3.2);
    }
    if (this.lit && this.t >= DOOR_AT + DOOR_T + LIGHT_AT - 1.15) {
      this.lit = false;
      site.setHutLight(false);
      this.d.audio.play('switch', { gain: 0.18, rate: 0.7, at: this.doorAt(site) });
    }
    // :40, with the flashlight: the lamps outside, all of them
    if (this.lamps && (s.warnT ?? 0) >= BEAT.dark) { this.lamps = false; site.setOutLamps(false); }
  }

  /** The truck by the gate while the keys are gone. */
  tryTruck() {
    this.triedTruck++;
    this.d.audio.play('doorCreak1', { gain: 0.14, rate: 1.25 });
    this.d.toast(this.triedTruck === 1 ? 'The keys are not in it. They are always in it.' : 'No keys. Not in the ignition, not on the seat, not on the floor.', 3.6);
  }
  /** The hut door while it is shut. */
  tryDoor() {
    this.triedDoor++;
    const site = this.d.site();
    this.d.audio.play(this.d.audio.or('door_latch', 'switch'), { gain: 0.5, rate: 0.75, at: site ? this.doorAt(site) : undefined });
    this.d.toast(this.triedDoor === 1 ? 'The latch gives. The door does not. It has not been locked in years.' : 'It does not move. Not even in the frame.', 3.4);
  }
  /** How the truck at the station is described while the keys are gone (null: as always). */
  truckExamine() { return this.keysGone ? 'SARO 07, by the gate where you left it. The ignition is empty.' : null; }

  private doorAt(site: Station01) {
    const p = site.proxies.hutDoor;
    return p ? p.getWorldPosition(this.v) : undefined;
  }
  /** The station as it always is: the door open into the room, the lights on, the keys in the truck. */
  private putBack() {
    const site = this.d.site();
    if (site) { site.setHutDoor(0); site.setHutLight(true); site.setOutLamps(true); }
    this.state = 'idle'; this.keysGone = false; this.doorK = 0; this.lit = true; this.lamps = true;
  }
  /** A night put back or left in the middle: the station as it always is. */
  reset() { if (this.state === 'shut') this.putBack(); this.state = 'idle'; this.wasStage = 'calm'; this.last = null; this.pending = false; }
}
