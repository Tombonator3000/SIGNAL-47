import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import { Town } from '../world/Town';
import { clockText } from './time';
import { PLACES, lineOfMark } from './lines';

// What happens in Caliche once in a night (HENDELSER.md, section 6; the first of the event
// director, in the town's terms). Each repeats something that was recorded, on line A, which runs
// down the main street at a slant, through the payphone (story/lines.ts), and none of it happens
// because the player is there; the director only waits until the player is near. They are kept in
// the case with the town (main.ts, `town`), and none comes again, also after loading.
//
//   1  the payphone   rings at the gas station. On the line, the control room printer, line by
//                     line, for 47 seconds; a 1947 nickel in the coin return after. After room 6
//                     (chapter 5 on) it rings once more: an engine coughing and dying, nothing,
//                     then relay clicks, four and seven. THE EVENT, before it happens.
//   6  the cutout     the souvenir shop's cardboard alien on the highway's shoulder north of
//                     town, in the headlights. Once passed it is back in the shop's window, with
//                     dew on it.
//   8  the lamps      the street lamps go out one by one down the street, the dogs barking in
//                     the same order, and come back on in the same order.
//  12  the car        from 03:12, while the deputy is out, his car stands on the highway's
//                     shoulder north of town, engine running, the door open, nobody in it. Once
//                     passed it is gone; at 04:30 he is at his desk and says he drove the loop.
//
// Event 5, the attendant's sentence, is in story/caliche.ts (it is said when he is spoken to).
// At least ten minutes of the night between two, nothing in the prologue, nothing once THE EVENT
// has the night (main.ts decides when it may run).

export interface TownEventsIn {
  /** In play, no panel open, not in a cinematic, at night, and the story allows it. */
  ok: boolean;
  /** In the diner's area (the street is part of it), not in the diner. */
  inTown: boolean;
  /** After room 6: the second call. */
  later: boolean;
  cam: THREE.Camera;
  /** The game's clock, seconds since midnight. */
  clock: number;
  /** Seconds running, the same as the town's (Town.update): the lamp wave is timed by it. */
  t: number;
  /** The night's curve (story/curve.ts) lets the lamps go now (an event) and the payphone ring the
   *  first time (47 s on the line); left out, they may. The second call, after room 6, is the story's. */
  may?: boolean;
  mayLong?: boolean;
}
export interface TownEventsDeps {
  audio: AudioSys;
  toast: (text: string, secs?: number) => void;
  note: (text: string) => void;
  /** The town's part of the case (also the marks for the notebook's map). */
  seen: Set<string>;
  save: () => void;
}

// (which line each kind of mark is on, A or B, is where its place is: story/lines.ts, lineOfMark;
// the lights along the highway, the herd facing 026 and the motel's television are B's)
const GAP = 10 * 60;              // seconds of the night between two events
const RING = 30;                  // how long the payphone rings if nobody answers
const LINE = 47;                  // seconds of the printer on the line
const PHONE = new THREE.Vector3(PLACES.phone[0], 1.4, PLACES.phone[1]);   // the payphone, road-local (Town.gasStation; on A)

export class TownEvents {
  /** What is going on now, for the developer menu and the tests. */
  now: 'none' | 'ringing' | 'line' | 'lamps' = 'none';
  private town: Town | null = null;
  private t = 0;
  private near = 0;               // seconds the player has been near the payphone
  private street = 0;             // seconds the player has been out on the street
  private ringT = 0;
  private lineT = 0;
  private call: 1 | 2 = 1;
  private waveT = 0;
  private dogs: number[] = [];
  private notBefore = 0;          // the clock: no ringing again before this
  private cutoutSeen = false;
  private carSeen = false;
  private lineCheck = 0;
  private recordings: ReturnType<AudioSys['play']>[] = [];
  private lineSpeaker: AudioNode | null = null;
  private lineBand: AudioNode | null = null;

  constructor(private d: TownEventsDeps) {}

  bind(town: Town) {
    this.town = town;
    town.onAnswer = () => this.answer();
    town.setCutout(!this.d.seen.has('ev:cutout'));
  }
  /** After a load the town's part of the case is new: the cutout is where that case had it. */
  reset() {
    this.stopAll();
    this.carSeen = this.d.seen.has('ev:car');
    this.town?.setCarOut(false);
    this.cutoutSeen = this.d.seen.has('ev:cutout');
    this.town?.setCutout(!this.cutoutSeen);
  }

  private last() {
    let t = -1e9;
    for (const k of this.d.seen) if (k.startsWith('mark:')) t = Math.max(t, Number(k.slice(k.indexOf('@') + 1)));
    return t;
  }
  private free(clock: number) { return this.now === 'none' && clock - this.last() >= GAP; }
  private mark(kind: string, clock: number) { this.d.seen.add(`mark:${kind}@${Math.round(clock)}`); }
  private pos(local: THREE.Vector3) { return this.town!.group.localToWorld(local.clone()); }

  update(dt: number, s: TownEventsIn) {
    this.t = s.t;
    const town = this.town;
    if (!town) return;
    const day = ((s.clock % 86400) + 86400) % 86400;
    const cam = s.cam.getWorldPosition(new THREE.Vector3());
    const local = town.group.worldToLocal(cam.clone());

    // three marks on A: the line drawn in (the map draws it, ui/areaMap.ts)
    if ((this.lineCheck += dt) > 1 && !this.d.seen.has('lineA')) {
      this.lineCheck = 0;
      if (townMarks(this.d.seen).filter((m) => lineOfMark(m.kind) === 'A').length >= 3) {
        this.d.seen.add('lineA');
        this.d.note(`${clockText(s.clock, false)}. Three on one line. Drawn in on the map: through Caliche, and on to the north-west.`);
        this.d.save();
      }
    }

    // ---------- 6: the cutout ----------
    if (town.cutoutOut) {
      const p = town.cutoutAt, dist = Math.hypot(local.x - p.x, local.z - p.z);
      if (!this.cutoutSeen && s.ok && dist < 45) {
        const fwd = s.cam.getWorldDirection(new THREE.Vector3());
        const to = this.pos(p.clone().setY(p.y + .6)).sub(cam).normalize();
        if (fwd.dot(to) > 0.85) {
          this.cutoutSeen = true;
          this.d.seen.add('ev:cutout'); this.mark('cutout', s.clock);
          this.d.note(`${clockText(s.clock, false)}. The highway north of Caliche: the cardboard alien from the souvenir shop, standing on the shoulder. Kids, probably.`);
          this.d.save();
        }
      }
      // once passed and well behind, it is gone from the road
      if (this.cutoutSeen && dist > 150) town.setCutout(false);
    }

    // ---------- 12: the deputy's car ----------
    {
      const p = town.carShoulder, dist = Math.hypot(local.x - p.x, local.z - p.z);
      if (!town.carOut && !this.carSeen && !town.deputyIn && day >= 3 * 3600 + 12 * 60 && day < 4 * 3600 + 30 * 60 && dist > 200) town.setCarOut(true);
      if (town.carOut && !this.carSeen && s.ok && dist < 25) {
        this.carSeen = true;
        this.d.seen.add('ev:car'); this.mark('car', s.clock);
        this.d.note(`${clockText(s.clock, false)}. The deputy's car on the shoulder of the highway north of Caliche: engine running, the door open, nobody in it.`);
        this.d.save();
      }
      if (town.carOut && this.carSeen && dist > 150) town.setCarOut(false);
    }

    // ---------- 1: the payphone ----------
    const phone = this.pos(PHONE), dPhone = cam.distanceTo(phone);
    if (this.now === 'ringing') {
      this.ringT += dt;
      if (this.ringT > RING || !s.inTown || !s.ok) { this.stopRing(); this.notBefore = s.clock + 4 * 60; }
    } else if (this.now === 'line') {
      // Leaving halfway through is not evidence of a complete recording. It can ring on a
      // later visit, but no nickel, event flag or full-length observation is awarded yet.
      if (dPhone > 6 || !s.inTown || !s.ok) {
        this.stopAll(); this.notBefore = s.clock + 4 * 60;
      } else if ((this.lineT += dt) >= LINE) this.hangUp(s.clock);
    }
    this.near = s.ok && s.inTown && dPhone < 45 ? this.near + dt : 0;
    const call = !this.d.seen.has('ev:phone1') ? 1 : s.later && !this.d.seen.has('ev:phone2') ? 2 : 0;
    // The return call follows Room 6, at about 05:00. The original 04:45 cutoff made it
    // unreachable in the real chapter journey. Leave enough room before the 05:20 departure.
    const phoneUntil = call === 2 ? 5 * 3600 + 19 * 60 : 4 * 3600 + 45 * 60;
    if (call && this.near > 20 && s.clock >= this.notBefore && day >= 2 * 3600 + 40 * 60 && day < phoneUntil && this.free(s.clock) && (call === 2 || s.mayLong !== false)) {
      this.call = call; this.now = 'ringing'; this.ringT = 0; town.ringing = true;
      this.d.audio.loop('town-ring', 'phoneRing', { at: phone, gain: 0.8 });
    }

    // ---------- 8: the lamps ----------
    this.street = s.ok && s.inTown ? this.street + dt : 0;
    if (this.now === 'lamps') {
      const w = this.t - this.waveT;
      // the dogs, by the lamps, in the same order
      town.waveOrder.forEach((at, k) => {
        if (!this.dogs.includes(k) && w >= k * Town.WAVE_STEP) {
          this.dogs.push(k);
          this.d.audio.play(k % 2 ? 'dog1' : 'dog0', { at: at.clone().setX(at.x + 30 + 10 * (k % 3)), gain: 0.55, rate: 0.92 + 0.05 * (k % 3) });
        }
      });
      if (w > Town.WAVE_HOLD + town.waveOrder.length * Town.WAVE_STEP + 1) {
        this.now = 'none';
        this.d.seen.add('ev:lamps'); this.mark('lamps', s.clock);
        this.d.note(`${clockText(s.clock, false)}. Main Street, Caliche: the street lights out one by one, south to north, and the dogs barking in the same order. Back on in the same order.`);
        this.d.save();
      }
    } else if (!this.d.seen.has('ev:lamps') && this.street > 40 && day >= 3 * 3600 + 5 * 60 && day < 4 * 3600 + 55 * 60 && this.free(s.clock) && s.may !== false) {
      this.now = 'lamps'; this.waveT = this.t; this.dogs = [];
      town.lampWave(this.t);
    }
  }

  private answer() {
    if (this.now !== 'ringing' || !this.town) return;
    this.stopRing();
    this.now = 'line'; this.lineT = 0;
    const at = this.pos(PHONE), a = this.d.audio;
    if (this.call === 1) {
      a.loop('town-line', 'printer', { at, gain: 0.35 });
      this.d.toast('No dial tone. On the line, a printer: the control room printer, line by line, the way it printed at 02:13.', 6);
    } else {
      // the line still open after it: a hush in the handset's band (bought sound; else the wind)
      a.loop('town-line', a.or('line_hush', 'gust'), { at, gain: 0.12 });
      // far off on the line, an engine: it turns over and catches, again, and dies (the truck's
      // own start and shut-off, small in the handset's band)
      const ear = a.spatial(at, 1.2);
      if (ear) {
        const line = a.phoneLine(); line.gain.disconnect(); line.gain.connect(ear); line.gain.gain.value = 0.4;
        this.lineSpeaker = ear; this.lineBand = line.gain;
        this.recordings.push(
          a.play('engineStart', { dest: line.input, when: 0.4, from: 0.7, dur: 1.5 }),
          a.play('engineStart', { dest: line.input, when: 1.9, from: 0.75, dur: 1.1, rate: 0.93 }),
          a.play('engineOff', { dest: line.input, when: 2.9, rate: 0.9 }),
        );
      }
      // relay clicks, four and seven, after the silence
      for (let i = 0; i < 11; i++) this.recordings.push(a.play('click', { at, gain: 0.5, when: 14 + i * 0.32 + (i >= 4 ? 0.9 : 0), lp: 2200 }));
      this.d.toast('Far off, an engine. It coughs, coughs again and stops. Then nothing at all.', 6);
    }
  }
  private hangUp(clock: number) {
    this.stopRecording();
    this.d.audio.stop('town-line', 0.2);
    this.now = 'none';
    this.town!.ringing = false;
    const t = clockText(clock, false);
    if (this.call === 1) {
      this.d.seen.add('ev:phone1'); this.d.seen.add('nickel'); this.mark('phone', clock);
      this.d.toast('The line clicks and goes dead. Something drops into the coin return: a nickel, dated 1947, bright as new.', 5.5);
      this.d.note(`${t}. Payphone, Main Street, Caliche: 47 s of the control room printer on the line. A 1947 nickel in the coin return.`);
    } else {
      this.d.seen.add('ev:phone2'); this.mark('phone2', clock);
      this.d.toast('Clicks on the line: four, and seven. Then the dial tone, as if nothing had been there.', 5);
      this.d.note(`${t}. Payphone, Main Street, Caliche, again: an engine coughing and dying on the line, then nothing, then relay clicks, four and seven.`);
    }
    this.d.save();
  }
  private stopRing() {
    this.d.audio.stop('town-ring', 0.05);
    if (this.town) this.town.ringing = false;
    if (this.now === 'ringing') this.now = 'none';
  }
  private stopAll() {
    this.stopRing(); this.d.audio.stop('town-line', 0.05);
    this.stopRecording();
    this.now = 'none'; this.near = 0; this.street = 0; this.notBefore = 0;
  }
  private stopRecording() {
    for (const h of this.recordings) if (h) { try { h.src.stop(); } catch {} h.gain.disconnect(); }
    this.recordings = [];
    this.lineBand?.disconnect(); this.lineBand = null;
    this.lineSpeaker?.disconnect(); this.lineSpeaker = null;
  }
}

/** The marks the town's events have left, for the notebook's map: what, and when. */
export function townMarks(seen: Set<string>) {
  const out: { kind: string; clock: number }[] = [];
  for (const k of seen) if (k.startsWith('mark:')) { const [kind, c] = k.slice(5).split('@'); out.push({ kind, clock: Number(c) }); }
  return out.sort((a, b) => a.clock - b.clock);
}
