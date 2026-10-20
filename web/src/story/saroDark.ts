import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import { Silhouette } from '../core/roadside';
import type { SaroPower } from '../world/saroPower';
import { facing } from './seen';
import { clockText, pad } from './time';
import { telexHondo } from './nightshift';

// 02:29: SARO goes dark, and Hondo writes (FORSLAG.md, 20 October, item 4; KAPITLER.md, The Second
// Exposure, new 1). Chapter one, once, while the player is out in the lab or the yard and the control
// room stands empty:
//
//   the relay    K3 clacks on the rack in the empty control room, heard through the wall. The player
//                has heard it twice before: by itself at the skip, forty seconds before the lights
//                dipped (Prologue.relayFirst), and seven seconds before the impact at 02:13. The
//                corner clock goes over to seconds (UI.clockSeconds), as it does for the 47-second
//                warning. If K3 is already open since the impact (most nights: it is a lever on the
//                rack and nothing asks for it in chapter one), it has nothing left to trip and trips
//                anyway: the same clack, and the log writes it down a second time (R5: a recording
//                of the trip, not the relay).
//   :31          everything at SARO sags for a moment, as the lights did at 23:52.
//   :40          every light that is SARO's goes out (world/saroPower.ts): the photo lab's safelights,
//                the DARKROOM lamp, the yard's and the array's sodium lamps, the wall packs, the
//                control room, the annex, and the red lamps on all 27 antennas. The hum of the
//                place dies with them (the room tone, the S-03 cabinet, the safelight's ballast), and
//                a breath later the crickets and the wind too. Your own footsteps are left. The eyes
//                need a while: the starlight comes up slowly. The highway's street lamps west of the
//                site and the motel's sign down the road stay lit: it is only SARO.
//   the dark     47 seconds. A few seconds into it, at the far edge of SARO's grounds (out on the
//                windows side towards the highway, whose lamps still light the ground beyond: where
//                the player can see it from, out of the middle of the view), a small figure stands
//                quite still: the outline from the glimpses (core/roadside.ts), black against the
//                light it stands in front of. Tom, 20 October: "Spenning og skummelt slår faste
//                regler". It bends two rules of HENDELSER.md, section 4, here only: it stands nearer
//                than 40 m (some 11 to 20), and it does not go when it is looked at. It goes with a closer look (the zoom), the flashlight on it, walking up
//                to it, and with the light.
//   the light    the power comes back: the tubes, the safelights and the screens at once, the sodium
//                lamps dim and warming up over half a minute; the hum, the crickets all at once.
//                The figure is not there. K3 stays open.
//   the telex    a few seconds on, the teleprinter in the control room starts (made here: a motor and
//                strikes), and Hondo Valley's message is on the roll, with the line in the console
//                log (KAPITLER.md: "TELEX RECEIVED  HONDO VALLEY"). Nothing on the screen says what it
//                says: whoever goes in finds it (Prologue.telexRoll; the first reading is noted).
//
// A night that did not go dark (chapter one done before 02:29, or the player away from SARO until
// 02:40) still has Hondo Valley's message on the roll, printed at 02:29 with nobody there, as
// KAPITLER.md first had it (main.ts): Ward reads it all the same ("I read the telex. All of it.").
//
// One thing at a time (main.ts, oneAtATime): it does not begin while anything else of the night's is
// going, and nothing else begins while it runs; it waits for whatever is going, until 02:40. It is
// the story's 47, not the curve's to time, but it is chapter one's one 47-second event all the same
// (story/curve.ts keeps the chapter's 47 for it until it has come or its time is past, and it lands
// the peak when it comes). Written in the night's log (story/seen.ts: the event 'blackout', and the
// figure as a scare when it stands) and kept with the case (NightShiftState.hondo, the console log).
// A paper, the notebook or the board open holds it where it is, as it holds the clock. The wet bench,
// the B-12 control and the camera's case frames wait for the light (Chapter1.dark): nothing is lost
// in the dark, and the print in the fixer comes up as it would have.

export type DarkPhase = 'idle' | 'relay' | 'dark' | 'back' | 'done';
/** Where the player is: the photo lab, out in the yard and SARO's grounds, the control room, or away. */
export type DarkWhere = 'lab' | 'yard' | 'room' | 'away';

const T0 = 2 * 3600 + 29 * 60;              // 02:29, the telex's minute (KAPITLER.md)
const T1 = 2 * 3600 + 40 * 60;              // after this it does not come
const YARD_FROM = T0 + 150, ROOM_FROM = T0 + 420;   // the lab first; the yard from 02:31:30; the room from 02:36
const RELAY = 40, SAG = 31, DARK = 47;
const FIG_FROM = 6.5, FIG_ANY = 20;         // the figure: out of the middle of the view from 6.5 s, anywhere seen from 20 s
const TELEX_AFTER = 2.6, CPS = 10.5;        // the teleprinter: seconds after the light, characters a second
const SODIUM = 28;                          // the sodium lamps, from dim to full after the light
const STRIKE = 0.45;                        // the tubes and safelights stutter as they strike
const ADAPT = { low: 0.32, high: 1.3, secs: 14 };   // the starlight as the eyes open (the sky light's share)

/** Where it can stand: the far edges of the yard, in SARO's terms (feet on the ground there). */
export interface DarkSpot { id: string; x: number; z: number; where: string }
// (both out on the grounds on the windows side, towards the highway, whose lamps are not SARO's and
// still light the ground beyond: seen from the walk north of the lab, from B-12, and through the
// control room's windows, what stands there is black against that light)
export const DARK_SPOTS: DarkSpot[] = [
  { id: 'grounds', x: -6.5, z: -12.5, where: 'out on the grounds by the windows, at the edge of the highway\'s light' },
  { id: 'fence', x: -9.0, z: -17.5, where: 'out by the fence towards the highway, at the edge of its light' },
];

// What the eyes can see through at SARO: the buildings' footprints (xz), each with the openings it has
// (a door, when it is open; the control room's windows on the north side).
interface Box { minX: number; maxX: number; minZ: number; maxZ: number; holes: { side: 'n' | 's' | 'e' | 'w'; a: number; b: number; door?: string }[] }
const BUILDINGS: Box[] = [
  { minX: -6.3, maxX: 6.3, minZ: -4.75, maxZ: 4.75, holes: [{ side: 'n', a: -5.4, b: 1.8 }, { side: 'e', a: 1.1, b: 2.1, door: 'east' }] },   // the control room
  { minX: -6.6, maxX: 2.6, minZ: 4.75, maxZ: 11.1, holes: [] },                                                                              // the annex
  { minX: 12.6, maxX: 19.2, minZ: -6.0, maxZ: 1.0, holes: [{ side: 'w', a: -1.7, b: -0.7, door: 'lab' }] },                                  // the photo lab
  { minX: 20.2, maxX: 39.0, minZ: -5.9, maxZ: 5.9, holes: [] },                                                                              // the east wing
];
type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
/** Whether the line from a to b (xz) gets out of and past SARO's buildings (and `more` in the way:
 *  the truck on its pad). */
export function clearAt(a: THREE.Vector3, b: THREE.Vector3, open: (door: string) => boolean, more: readonly Rect[] = []) {
  const dx = b.x - a.x, dz = b.z - a.z;
  for (const r of [...BUILDINGS, ...more.map((m) => ({ ...m, holes: [] as Box['holes'] }))]) {
    // the segment's part inside the box (Liang-Barsky)
    let t0 = 0, t1 = 1, side0: Box['holes'][0]['side'] | null = null, side1: Box['holes'][0]['side'] | null = null;
    const clip = (p: number, q: number, enter: Box['holes'][0]['side'], leave: Box['holes'][0]['side']) => {
      if (Math.abs(p) < 1e-9) return q >= 0;
      const t = q / p;
      if (p < 0) { if (t > t1) return false; if (t > t0) { t0 = t; side0 = enter; } }
      else { if (t < t0) return false; if (t < t1) { t1 = t; side1 = leave; } }
      return true;
    };
    if (!clip(-dx, a.x - r.minX, 'w', 'w') || !clip(dx, r.maxX - a.x, 'e', 'e') || !clip(-dz, a.z - r.minZ, 'n', 'n') || !clip(dz, r.maxZ - a.z, 's', 's')) continue;
    if (t1 <= t0) continue;
    // from inside (the eye in the room or the lab): out through an opening, or not at all
    const through = (t: number, side: Box['holes'][0]['side'] | null) => {
      if (!side) return false;
      const x = a.x + dx * t, z = a.z + dz * t, along = side === 'n' || side === 's' ? x : z;
      return r.holes.some((h) => h.side === side && along >= h.a && along <= h.b && (!h.door || open(h.door)));
    };
    const fromIn = t0 <= 0 && side0 === null, toIn = t1 >= 1 && side1 === null;
    if (fromIn && !toIn) { if (!through(t1, side1)) return false; continue; }
    if (!fromIn && toIn) { if (!through(t0, side0)) return false; continue; }
    if (fromIn && toIn) continue;
    return false;   // straight across a building
  }
  return true;
}

export interface DarkIn {
  /** Chapter one at SARO, on foot, its report not yet filed, nothing else of the night's going (one
   *  at a time), the telex not yet had: it may begin. */
  ok: boolean;
  /** A paper, the notebook or the board open, a cinematic, a call: what is going waits where it is. */
  hold: boolean;
  where: DarkWhere;
  clock: number;
  cam: THREE.PerspectiveCamera;
  /** 0..1, the closer look (the zoom). */
  zoom: number;
  /** The flashlight is on. */
  torch: boolean;
  /** Whether a door is open (Doors.isOpen). */
  open: (door: string) => boolean;
  /** What else stands in the way of the eyes (the truck, where it is parked at SARO). */
  blocks?: readonly Rect[];
}

export interface DarkDeps {
  audio: AudioSys;
  power: SaroPower;
  scene: THREE.Scene;
  /** The ground's height in SARO's scene. */
  ground: (x: number, z: number) => number;
  /** The rack and the teleprinter, in the world. */
  rack: () => THREE.Vector3;
  teleprinter: () => THREE.Vector3;
  /** K3 goes (Prologue.relayClack): true if it tripped, false if it had nothing left to trip. */
  relay: () => boolean;
  /** A line in the console log (Prologue.consoleLog), at a clock. */
  log: (text: string, at: number) => void;
  /** Hondo Valley's message is on the roll now (Prologue.hondoTelex). */
  telex: (filed: number) => void;
  /** The control room's radio and screens without power, and back (Prologue.powerOut). */
  powerOut: (out: boolean) => void;
  /** The night outside: quiet (secs, level, tau, back), as Ambience.hush. */
  hush: (secs: number, level: number, tau: number, back: number) => void;
  toast: (text: string, secs: number) => void;
  note: (text: string) => void;
}

export class SaroDark {
  phase: DarkPhase = 'idle';
  /** Seconds into the phase. */
  t = 0;
  /** When K3 went (the night's clock): the telex is filed at its minute. */
  at = 0;
  /** K3 was already open and tripped anyway. */
  wasOpen = false;
  /** Where the player was when it began. */
  from: DarkWhere = 'away';
  /** The figure: where it stood (DARK_SPOTS id), whether it was looked at, how it went. */
  figure: { id: string; looked: boolean; gone: string | null } | null = null;
  /** The starlight's share as the eyes open (main.ts multiplies the sky light by it). */
  adapt = 1;
  /** The corner clock with seconds while the relay counts (main.ts, UI.clockSeconds). */
  clockShow: { hm: string; ss: string } | null = null;
  private sil: Silhouette;
  private fig: THREE.Group;
  private head = new THREE.Vector3();
  private eye = new THREE.Vector3();
  private sagged = false;
  private chars: { t: number; cr: boolean }[] = [];
  private printed = 0;
  private motor = false;
  private noted = false;
  private sodiumT = -1;
  private sagT = -1;
  private nowClock = 0;

  constructor(private d: DarkDeps) {
    // black against the night, with the faintest cold edge of the starlight on it
    this.sil = new Silhouette('saroDark:figure', 0x1d2433, 'stand');
    this.fig = this.sil.group;
    this.fig.visible = false;
    d.scene.add(this.fig);
  }

  /** It is going (one at a time: nothing else starts, main.ts). */
  get busy() { return this.phase === 'relay' || this.phase === 'dark' || (this.phase === 'back' && this.printed < this.chars.length); }
  /** The 47 seconds of the dark (saves wait, and the night's other things). */
  get dark() { return this.phase === 'dark'; }
  /** Where the figure's head is while it stands (story/seen.ts), or null. */
  get figureAt() { return this.fig.visible ? this.head : null; }
  /** Whether the figure is one of round 14's outlines (the tests). */
  get painted() { return this.sil.painted; }
  /** Its time: 02:29 to 02:40 (main.ts keeps the chapter's 47 for it until then). */
  static window(clock: number) { const c = ((clock % 86400) + 86400) % 86400; return c >= T0 && c < T1; }
  static pending(clock: number) { return (((clock % 86400) + 86400) % 86400) < T1; }
  /** The telex's own minute (KAPITLER.md): where the night never went dark, it printed then. */
  static readonly FILED = T0;

  update(dt: number, s: DarkIn) {
    this.nowClock = s.clock;
    this.sodiumStep(dt);
    this.adaptStep(dt);
    if (this.phase === 'idle') {
      if (s.ok && !s.hold && this.due(s)) this.begin(s);
      return;
    }
    if (this.phase === 'done') return;
    if (s.hold && this.phase !== 'back') return;
    this.t += dt;
    if (this.phase === 'relay') this.relayStep(s);
    else if (this.phase === 'dark') this.darkStep(s);
    else this.backStep();
  }

  private due(s: DarkIn) {
    const c = ((s.clock % 86400) + 86400) % 86400;
    if (c < T0 || c >= T1) return false;
    return s.where === 'lab' || (s.where === 'yard' && c >= YARD_FROM) || (s.where === 'room' && c >= ROOM_FROM);
  }

  /** Now (the developer menu, the tests): from where the player is, whatever the clock. */
  now(s: DarkIn) {
    if (this.phase !== 'idle' || s.where === 'away') return false;
    this.begin(s);
    return true;
  }

  private begin(s: DarkIn) {
    const { audio, rack } = this.d;
    this.phase = 'relay'; this.t = 0; this.sagged = false; this.noted = false;
    this.at = Math.round(s.clock); this.from = s.where; this.figure = null;
    this.wasOpen = !this.d.relay();
    audio.relayClack(rack(), s.where !== 'room');
    this.d.toast(s.where === 'room' ? 'On the rack, K3 trips.' : 'Through the wall, in the empty control room: the relay.', 3.6);
    this.clock();
  }

  private relayStep(s: DarkIn) {
    const { power, audio } = this.d;
    this.clock();
    // :31, a sag: everything dims for a moment and the hum with it
    if (!this.sagged && this.t >= SAG) {
      this.sagged = true; this.sagT = this.t;
      power.cut(); power.set(0.35, 0.5);
      audio.humLevel(0.3, 0.03);
    }
    if (this.sagT >= 0 && this.t >= this.sagT + 0.34) { this.sagT = -1; power.restore(); audio.humLevel(1, 0.15); }
    if (this.t >= RELAY) this.goDark(s);
  }

  private goDark(s: DarkIn) {
    const { power, audio, rack } = this.d;
    this.phase = 'dark'; this.t = 0; this.sagT = -1;
    power.cut();
    const at = rack();
    audio.contactor(at, s.where === 'room' ? 1 : 1.3);
    audio.powerDown(at);
    audio.humLevel(0, 0.05);
    audio.holdRoomTone(0, 0.05);
    this.d.powerOut(true);
    this.d.log('LINE POWER  FAIL', Math.round(s.clock));
    // a breath later the night outside goes quiet too, and stays so until the light
    this.hushAt = 1.4;
    this.adapt = ADAPT.low; this.adaptT = 0;
    this.d.toast(s.where === 'lab' ? 'The safelight dies, and the hum with it. Black.' : s.where === 'room' ? 'Every light in the room goes out at once.' : 'Every lamp at SARO goes out at once. The red lamps on the dishes too.', 3.6);
    this.clockShow = { hm: this.clockShow?.hm ?? clockText(this.at, false), ss: '40' };
  }
  private hushAt = -1;
  private adaptT = -1;

  private darkStep(s: DarkIn) {
    if (this.hushAt >= 0 && this.t >= this.hushAt) { this.hushAt = -1; this.d.hush(600, 0, 0.9, 0.2); }
    s.cam.getWorldPosition(this.eye);
    if (this.fig.visible) this.watch(s);
    else if (!this.figure && this.t >= FIG_FROM) this.place(s);
    if (this.t >= DARK) this.goBack();
  }

  /** It stands where the player can see it from here, out of the middle of the view (from FIG_ANY,
   *  anywhere they can see it). */
  private place(s: DarkIn) {
    const f = s.cam.getWorldDirection(new THREE.Vector3());
    let best: { spot: DarkSpot; p: THREE.Vector3; off: number } | null = null;
    for (const spot of DARK_SPOTS) {
      const p = new THREE.Vector3(spot.x, this.d.ground(spot.x, spot.z), spot.z);
      const head = p.clone().setY(p.y + this.sil.headY);
      const dist = Math.hypot(head.x - this.eye.x, head.z - this.eye.z);
      if (dist < 11 || dist > 60 || !clearAt(this.eye, head, s.open, s.blocks)) continue;
      const off = THREE.MathUtils.radToDeg(f.angleTo(head.clone().sub(this.eye)));
      if (off < 38 && this.t < FIG_ANY) continue;
      if (!best || off > best.off) best = { spot, p, off };
    }
    if (!best) return;
    this.fig.position.copy(best.p);
    this.fig.rotation.y = Math.atan2(this.eye.x - best.p.x, this.eye.z - best.p.z);
    this.sil.wake(); this.sil.pose('stand'); this.sil.rim(0x1d2433);
    this.head.copy(best.p).setY(best.p.y + this.sil.headY);
    this.fig.visible = true;
    this.figure = { id: best.spot.id, looked: false, gone: null };
  }
  /** Looked at, it stays. A closer look, the flashlight on it, or walking up to it, and it was not there. */
  private watch(s: DarkIn) {
    const fg = this.figure!, dist = this.eye.distanceTo(this.head);
    if (facing(s.cam, this.head, 90, 22)) fg.looked = true;
    let gone: string | null = null;
    if (s.zoom > 0.25 && facing(s.cam, this.head, 120, 14)) gone = 'zoom';
    else if (s.torch && dist < 70 && facing(s.cam, this.head, 70, 40)) gone = 'flashlight';
    else if (dist < 9) gone = 'near';
    if (gone) { fg.gone = gone; this.fig.visible = false; }
  }

  private goBack() {
    const { power, audio, rack } = this.d;
    this.phase = 'back'; this.t = 0;
    if (this.fig.visible) { this.fig.visible = false; if (this.figure) this.figure.gone = 'light'; }
    // the tubes, the safelights and the screens strike at once (a stutter), the sodium lamps dim
    power.set(1, 0.06);
    this.sodiumT = 0;
    const at = rack();
    audio.contactor(at, 0.8);
    audio.powerUp(at);
    audio.humLevel(1, 0.4);
    audio.holdRoomTone(1, 0.35);
    this.d.powerOut(false);
    this.d.hush(0, 1, 0.05, 0.12);   // the crickets all at once
    this.hushAt = -1;
    this.adapt = 1; this.adaptT = -1;
    this.d.log('LINE POWER  RESTORED  00:00:47', Math.round(this.nowClock));
    this.clockShow = null;
    // the teleprinter: the message as it prints, at its own pace, a line at a time
    this.chars = []; this.printed = 0;
    let t = TELEX_AFTER + 0.6;
    for (const line of telexHondo(this.at).split('\n')) {
      for (const ch of line) { if (ch !== ' ') this.chars.push({ t, cr: false }); t += 1 / CPS; }
      this.chars.push({ t, cr: true }); t += 0.32;
    }
    this.d.toast('The lights come back.', 2.6);
  }
  private backStep() {
    const { audio, teleprinter } = this.d;
    if (!this.motor && this.t >= TELEX_AFTER) {
      this.motor = true; audio.telexMotor(teleprinter());
      this.d.toast('In the control room, the telex starts up.', 3.2);
    }
    const at = teleprinter();
    while (this.printed < this.chars.length && this.t >= this.chars[this.printed].t) { audio.typeBar(at, this.chars[this.printed].cr); this.printed++; }
    if (this.motor && this.printed >= this.chars.length) {
      this.motor = false; audio.telexMotor(null);
      this.d.telex(this.at);
      this.finish();
    }
  }
  private finish() {
    this.phase = 'done';
    if (this.noted) return;
    this.noted = true;
    const hm = clockText(this.at, false);
    this.d.note(this.wasOpen
      ? `${hm}. A clack from the rack, in the empty control room: K3, with nothing left to trip. It tripped anyway. Forty seconds later every light at SARO went out, the red lamps on the dishes too. For 47 s. The telex ran when the power came back.`
      : `${hm}. In the empty control room K3 tripped. Forty seconds later every light at SARO went out, the red lamps on the dishes too. For 47 s. The telex ran when the power came back.`);
    const f = this.figure, spot = f ? DARK_SPOTS.find((x) => x.id === f.id) : null;
    if (f && f.looked && spot) this.d.note(`${hm}. In the dark, ${spot.where}: someone small, standing quite still. ${f.gone === 'light' ? 'Not there when the lights came back.' : 'Then not there.'}`);
  }

  // the stutter as the power strikes, and the sodium lamps warming up after
  private sodiumStep(dt: number) {
    if (this.sodiumT < 0) return;
    this.sodiumT += dt;
    const k = Math.min(1, this.sodiumT / SODIUM), e = 0.06 + 0.94 * k * k * (3 - 2 * k);
    if (this.sodiumT < STRIKE) this.d.power.set(Math.random() > 0.45 ? 1 : 0.12, e);
    else { this.d.power.settle(); this.d.power.set(1, e); }
    if (k >= 1) { this.sodiumT = -1; this.d.power.restore(); }
  }
  private adaptStep(dt: number) {
    if (this.adaptT < 0) return;
    this.adaptT += dt;
    const k = Math.min(1, this.adaptT / ADAPT.secs);
    this.adapt = ADAPT.low + (ADAPT.high - ADAPT.low) * k * k * (3 - 2 * k);
  }
  private clock() {
    const ss = Math.min(40, Math.floor(this.t));
    this.clockShow = { hm: clockText(this.at, false), ss: pad(ss) };
  }

  /** Stop at once, the power back as it was (the title, another save, a new night). */
  halt() {
    const { power, audio } = this.d;
    if (this.motor) { this.motor = false; audio.telexMotor(null); }
    if (power.out) power.restore();
    if (this.phase === 'relay' || this.phase === 'dark' || this.phase === 'back') {
      audio.humLevel(1, 0.1); audio.holdRoomTone(1, 0.1); this.d.powerOut(false); this.d.hush(0, 1, 0.05, 0.1);
    }
    this.fig.visible = false;
    this.phase = 'idle'; this.t = 0; this.sodiumT = -1; this.sagT = -1; this.adapt = 1; this.adaptT = -1; this.hushAt = -1;
    this.clockShow = null; this.chars = []; this.printed = 0; this.figure = null;
  }
  /** A night put back: had already (the telex is on the roll), or not yet. */
  restore(had: boolean) { this.halt(); if (had) this.phase = 'done'; }
}
