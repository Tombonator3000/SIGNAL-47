import * as THREE from 'three';
import type { Dish, DishArray } from '../world/Dish';
import type { AudioSys } from '../core/Audio';
import { onField, nearestAntenna } from '../world/arrayLayout';
import { clockText } from './time';

// The dishes come down and follow you (FORSLAG.md, 20 October, Valg for Tom 2, the full version;
// Tom: "Spenning og skummelt slår faste regler"). Since 02:15 every dish in the array has stood
// at az 026, el 32, with no command. Once in the night, when the warning of the time away
// (story/abduction.ts) comes while the player is out in the antenna field or close to it:
//
//   the start    a few seconds into the silence, a contactor and a brake out in the array, then the
//                drive motor of 02:15, near. The nearest dish lowers out of 026 and turns until it
//                points at the player; the next two after it, a second or so apart, each on the
//                player within some eight seconds however far round it has to come.
//   following    they track the player's eyes, the nearest fast enough to keep up. With the
//                footsteps gone, walking is heard only as their motors; standing still, they stop,
//                one brake after another, and it is quiet but for the warning's own pressure. The
//                one right over the player cannot come down far enough (its final limit, el -15)
//                and pushes against it now and then. Further out, later and slower (one bed of
//                sound, not placed); far out, late or not at all. The red lamps of those that have
//                been on the player slide into one blink.
//   escape       off the line, inside, in the truck: they stay on the place a while, then go back to
//                026 at the pace of 02:15.
//   taken        when the light comes they turn away from the player all at once, to the grass
//                where the player will wake (chosen then, Abduction.backAt). On waking they are
//                pointing down at it, and the view lifts from the stars towards the nearest; a
//                few seconds later they let go and go back.
//   the record   the console log has each drive that moved with no command, the final limit, and
//                the array back at 026 (SHOW LOG); the notebook has what was seen. Kept with the
//                case (AbductSave.dishes): it does not come again, not after Continue either.
//
// Later chapters see the array at 026: every dish ends exactly there (Dish.snap), and a save made
// while they are out comes back with them home and the log line written. Cheap on a phone: the
// array's matrices change only for the dishes that move (DishArray.refresh), three motor voices
// at most, one bed for the rest. Main.ts calls update() every frame of play, before the array is
// drawn; the Missing time setting turns the warning, and so this, off.

export const HOME = { az: 26, el: 32 };
/** As low as a drive goes: below its own final limit already, and not low enough for someone under it. */
export const EL_MIN = -15;
/** The elevation axle's height at scale 1 (Dish.matrices): where the boresight turns about. */
const AXLE = 12.4;
const DEG = 180 / Math.PI;
type Stage = 'calm' | 'warning' | 'light' | 'taken' | 'black' | 'back';

export interface DishesSave { clock: number; ids: string[]; back: boolean }
export interface FollowIn {
  stage: Stage;
  warning: { t: number; len: number } | null;
  /** In SARO's area, where the array is; on foot there. */
  saro: boolean;
  onFoot: boolean;
  /** The player's eyes and which way they look. */
  eye: THREE.Vector3;
  view: THREE.Vector3;
  clock: number;
  /** Where the player will wake, once the light has come. */
  backAt: THREE.Vector3 | null;
  /** The calm flashes setting: the brakes a little softer. */
  calm: boolean;
  /** The clock the player woke at, after the time away (its notes have it). */
  woke: number;
}
export interface FollowDeps {
  dishes: Dish[];
  array: DishArray;
  audio: AudioSys;
  /** Kept with the case (the abduction's save). */
  store: { get: () => DishesSave | undefined; set: (v: DishesSave) => void };
  ground: (x: number, z: number) => number;
  toast: (text: string, secs?: number) => void;
  note: (text: string) => void;
  /** A line in the console log at its own time (Prologue.consoleLog). */
  log: (text: string, at?: number) => void;
  save: () => void;
  /** Turn the player's head (waking on the back: towards the one that watches). */
  face: (yaw: number) => void;
}

/** Out in the antenna field, or close enough to one of its dishes. */
export function nearArray(x: number, z: number) { return onField(x, z, 3) || nearestAntenna(x, z).d < 45; }

// how they move: the nearest unmistakable, further out later and slower, far out late
const RATE = [{ az: 13, el: 7, acc: 7 }, { az: 6, el: 3.5, acc: 3 }, { az: 3.2, el: 2, acc: 1.4 }];
const AWAY = 1.5;                               // when the light comes, all at once and faster
const BACK = { az: 2.2, el: 1.3, acc: 0.6 };    // home again at the pace of 02:15 (Prologue.turnDishes)
const NEAR = 55, MID = 120, FAR = 260;           // metres from the player
const NEAR_MAX = 3;                              // the unmistakable ones, at most (one more as the player goes among them)
const HOLD = 5.5, WATCH = 7;                     // seconds they stay on the place before they go back

interface Rig {
  d: Dish; i: number; tier: 0 | 1 | 2;
  /** When it starts, in seconds of the event; started. */
  at: number; on: boolean; onAt: number;
  az: number; el: number; vAz: number; vEl: number;
  /** Where it points: the player's eyes while it follows, then the place. */
  aim: THREE.Vector3;
  locked: boolean; ever: boolean;
  /** Running (its motor heard, a brake each way), and how long it has been nearly still. */
  run: boolean; still: number; clunkT: number;
  /** Against the final limit: seconds to the next push, and the push. */
  strain: number; push: number; limit: boolean;
  /** How fast it may go (the near ones faster the further they have to come round). */
  rate: { az: number; el: number; acc: number };
  /** Its red lamp's place in its blink. */
  ph: number;
}

const wrap180 = (a: number) => ((a + 540) % 360) - 180;
const norm = (a: number) => ((a % 360) + 360) % 360;
const wrapPi = (a: number) => Math.atan2(Math.sin(a), Math.cos(a));
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const WORDS = ['no', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten', 'eleven', 'twelve'];
const word = (n: number) => WORDS[n] ?? String(n);
const list = (ids: string[]) => ids.length <= 1 ? ids.join('') : `${ids.slice(0, -1).join(', ')} and ${ids[ids.length - 1]}`;

/** Where a dish has to point to look at p (degrees), and how far it is on the ground. */
function aimAt(d: Dish, p: THREE.Vector3) {
  const dx = p.x - d.x, dz = p.z - d.z, dy = p.y - AXLE * d.scale, h = Math.hypot(dx, dz);
  return { az: norm(Math.atan2(dx, -dz) * DEG), el: Math.atan2(dy, Math.max(0.5, h)) * DEG, h };
}
/** How fast a dish may go: the near ones the faster the further round they have to come to the
 *  player (on them within about eight seconds of starting), the others at their tier's pace. */
function rateFor(d: Dish, tier: 0 | 1 | 2, eye: THREE.Vector3, from = d.curAz) {
  const swing = Math.abs(wrap180(aimAt(d, eye).az - from)), k = RATE[tier];
  // the near ones on the player within about eight seconds of starting; further out within about
  // twelve, far out about eighteen: the field goes quiet again when the player stands still
  const az = tier === 0 ? THREE.MathUtils.clamp(swing / 6, k.az - 2, 24) : tier === 1 ? THREE.MathUtils.clamp(swing / 10, 4, 11) : THREE.MathUtils.clamp(swing / 16, 2.2, 6);
  return { az, el: k.el, acc: Math.max(k.acc, az * (tier === 0 ? 0.7 : 0.5)) };
}
/** One axis of a drive: up to vmax, accelerating and braking at acc, stopping on the target. */
function axis(v: number, err: number, vmax: number, acc: number, dt: number) {
  const want = Math.sign(err) * Math.min(vmax, Math.sqrt(2 * acc * Math.abs(err)) * 0.92);
  // it gathers speed at acc, a heavy thing; it brakes harder than that
  const faster = Math.abs(want) > Math.abs(v) && want * v >= 0, lim = (faster ? 1 : 1.8) * acc * dt;
  return v + THREE.MathUtils.clamp(want - v, -lim, lim);
}

export class DishesFollow {
  mode: 'idle' | 'follow' | 'hold' | 'away' | 'watch' | 'home' = 'idle';
  /** Seconds since it began (tests). */
  t = 0;
  private rigs = new Map<string, Rig>();
  private skip = new Set<string>();
  private clock0 = 0;
  private len = 30;
  private holdT = 0;
  private chooseT = 0;
  private common: number | null = null;
  private said = { start: false, lock: false, move: false };
  /** Where the player stood when it began, and whether they went on from there. */
  private from = new THREE.Vector3();
  private moved = false;
  private lockAt = new THREE.Vector3();
  private firstFar = -1;
  private sound: Drives;

  constructor(private d: FollowDeps) { this.sound = new Drives(d.audio); }

  /** Seconds into the warning before the first one starts. */
  static firstAt(len: number) { return Math.max(2.5, 0.12 * len); }
  /** The dishes that are part of it now (tests), with how they stand. */
  get state() {
    return [...this.rigs.values()].map((r) => ({ id: r.d.id, tier: r.tier, on: r.on, onAt: r.onAt, locked: r.locked, ever: r.ever, run: r.run, az: r.az, el: r.el,
      aim: [r.aim.x, r.aim.y, r.aim.z], push: r.push > 0 }));
  }
  get voices() { return this.sound.live; }

  update(dt: number, s: FollowIn) {
    if (this.mode === 'idle') {
      const w = s.warning;
      if (s.stage === 'warning' && w && !this.d.store.get() && s.saro && s.onFoot && nearArray(s.eye.x, s.eye.z)
        && w.t >= DishesFollow.firstAt(w.len) && w.t < 0.8 * w.len) this.begin(s);
      return;
    }
    // out of SARO's area: nobody there to see them go back
    if (!s.saro) { this.home(s.clock); return; }
    this.t += dt;
    switch (this.mode) {
      case 'follow':
        if (s.stage === 'light') this.away(s);
        else if (s.stage !== 'warning') this.hold(s);
        else this.follow(dt, s);
        break;
      case 'hold': if ((this.holdT -= dt) <= 0) this.goHome(s); break;
      case 'away': if (s.stage === 'back') this.woke(s); else if (s.stage === 'calm') this.goHome(s); break;
      case 'watch': if ((this.holdT -= dt) <= 0) this.goHome(s); break;
      case 'home': break;
    }
    if ((this.mode as string) === 'idle') return;   // ended above
    this.step(dt, s);
    if (this.mode === 'home' && [...this.rigs.values()].every((r) => !r.on || (Math.abs(wrap180(r.az - HOME.az)) < 0.05 && Math.abs(r.el - HOME.el) < 0.05 && Math.abs(r.vAz) + Math.abs(r.vEl) < 0.05))) this.end(s.clock);
  }

  // ---------- the event ----------
  private begin(s: FollowIn) {
    this.mode = 'follow'; this.t = 0; this.clock0 = s.clock; this.len = s.warning?.len ?? 30;
    this.rigs.clear(); this.skip.clear(); this.common = null; this.firstFar = -1; this.chooseT = 1;
    this.said = { start: false, lock: false, move: false };
    this.from.copy(s.eye); this.moved = false;
    this.choose(s.eye);
    this.d.store.set({ clock: Math.round(s.clock), ids: [], back: false });
    this.sound.open();
    this.d.save();
  }
  /** Which dishes take part, and when each starts: by how far each is from the player now. */
  private choose(eye: THREE.Vector3) {
    const by = this.d.dishes.map((d, i) => ({ d, i, h: Math.hypot(d.x - eye.x, d.z - eye.z) })).sort((a, b) => a.h - b.h);
    const DELAY = [0, 1.8, 3.2];
    let near = 0, mid = 0;
    by.forEach(({ d, i, h }, rank) => {
      // the nearest is always one of the near ones, even off in a corner of the field
      let tier: 0 | 1 | 2 | -1 = (h < NEAR || (rank === 0 && h < 110)) && near < NEAR_MAX ? 0 : h < MID ? 1 : h < FAR ? 2 : -1;
      if (tier === 2 && Math.random() > 0.45) tier = -1;   // far out, most never do
      if (tier === -1) { this.skip.add(d.id); return; }
      const at = tier === 0 ? DELAY[near] : tier === 1 ? Math.max(4, 0.22 * this.len) + mid * 0.8 + rnd(0, 1.2) : Math.max(8, 0.42 * this.len) + rnd(0, 0.3 * this.len);
      if (tier === 0) near++; else if (tier === 1) mid++;
      this.rigs.set(d.id, this.rig(d, i, tier, this.t + at, eye));
    });
  }
  /** As the player goes among them: a dish come near is a near one, and soon; one that never was
   *  starts when the player comes within reach of it. */
  private promote(eye: THREE.Vector3) {
    const near = () => [...this.rigs.values()].filter((r) => r.tier === 0).length;
    this.d.dishes.forEach((d, i) => {
      const h = Math.hypot(d.x - eye.x, d.z - eye.z), r = this.rigs.get(d.id);
      if (r) {
        if (r.tier > 0 && h < NEAR - 15 && near() < NEAR_MAX + 1) { r.tier = 0; r.rate = rateFor(d, 0, eye, r.az); if (!r.on) r.at = Math.min(r.at, this.t + 0.6); }
        else if (r.tier === 2 && h < MID - 10) { r.tier = 1; r.rate = rateFor(d, 1, eye); if (!r.on) r.at = Math.min(r.at, this.t + 1.5); }
      } else if (h < MID - 10) {
        this.skip.delete(d.id);
        const n = h < NEAR - 15 && near() < NEAR_MAX + 1;
        this.rigs.set(d.id, this.rig(d, i, n ? 0 : 1, this.t + (n ? 0.6 : 1 + rnd(0, 1.5)), eye));
      }
    });
  }
  private rig(d: Dish, i: number, tier: 0 | 1 | 2, at: number, eye: THREE.Vector3): Rig {
    return { d, i, tier, at, on: false, onAt: -1, az: norm(d.curAz), el: d.curEl, vAz: 0, vEl: 0, aim: eye.clone(), locked: false, ever: false,
      run: false, still: 0, clunkT: -9, strain: rnd(1.5, 3), push: 0, limit: false, ph: d.phase, rate: rateFor(d, tier, eye) };
  }
  private follow(dt: number, s: FollowIn) {
    if ((this.chooseT -= dt) <= 0) { this.chooseT = 1; this.promote(s.eye); }
    if (!this.moved && Math.hypot(s.eye.x - this.from.x, s.eye.z - this.from.z) > 3) this.moved = true;
    for (const r of this.rigs.values()) {
      if (!r.on && this.t >= r.at) this.start(r, s);
      if (r.on) r.aim.copy(s.eye);
    }
    // the third thing: once one is on the player, a step away and it comes after
    if (this.said.lock && !this.said.move && Math.hypot(s.eye.x - this.lockAt.x, s.eye.z - this.lockAt.z) > 2.5
      && [...this.rigs.values()].some((r) => r.tier === 0 && r.run)) {
      this.said.move = true;
      this.d.toast('When I move, they move.', 3.2);
    }
  }
  private start(r: Rig, s: FollowIn) {
    r.on = true; r.onAt = this.t;
    if (r.tier === 0) this.d.log(`DRIVE ${r.d.id}  MOTION  NO COMMAND`, s.clock);
    else if (this.firstFar < 0) this.firstFar = s.clock;
    if (r.tier === 0 && !this.said.start) {
      this.said.start = true;
      this.d.toast(`${where(s.eye, s.view, r.d)}, out in the array, a drive motor starts up. Nobody is at the console.`, 4.2);
    }
  }
  /** Got away: they stay on the place a while. */
  private hold(s: FollowIn) {
    this.mode = 'hold'; this.holdT = HOLD;
    this.record(' When I got away from them they stayed on the place a while, and then went back to 026 by themselves, slowly.');
  }
  /** The light: away from the player all at once, to the grass where the player will wake. */
  private away(s: FollowIn) {
    this.mode = 'away';
    const at = s.backAt ? new THREE.Vector3(s.backAt.x, this.d.ground(s.backAt.x, s.backAt.z) + 0.35, s.backAt.z) : s.eye.clone();
    let n = 0;
    for (const r of this.rigs.values()) {
      if (!r.on && r.tier === 2) continue;
      if (!r.on) { r.on = true; r.onAt = this.t; }
      r.aim.copy(at); r.locked = false; n++;
      // every brake at once, near enough to hear
      if (r.tier === 0) { r.run = true; r.still = 0; r.clunkT = this.t; this.sound.clunk(r.d, true, s.calm, rnd(0, 0.18)); }
    }
    this.record(' When the light came they all turned away from me at once, out along the line, to the dark.');
    this.d.log(`ARRAY  ${n} DRIVES  MOTION  NO COMMAND`, s.clock);
  }
  /** Woken: they are pointing down at the place, and have been since the light. */
  private woke(s: FollowIn) {
    this.mode = 'watch'; this.holdT = WATCH;
    this.sound.hush();   // the sound comes back with the waking: not their motors from before
    const ids: string[] = [];
    let watcher: { r: Rig; h: number } | null = null;
    for (const r of this.rigs.values()) {
      if (!r.on) continue;
      r.aim.copy(s.eye);
      const a = aimAt(r.d, s.eye);
      r.az = a.az; r.el = Math.max(EL_MIN, a.el); r.vAz = r.vEl = 0; r.run = false; r.still = 0; r.locked = true;
      this.write(r, 0);
      if (a.h < 140) ids.push(r.d.id);
      if (a.h < 80 && (!watcher || a.h < watcher.h)) watcher = { r, h: a.h };
    }
    // the view lifts from the stars towards the nearest of them, a little to one side
    if (watcher) this.d.face(Math.atan2(-(watcher.r.d.x - s.eye.x), -(watcher.r.d.z - s.eye.z)) + rnd(-0.22, 0.22));
    const who = ids.length > 3 ? `${ids.slice(0, 3).join(', ')} and the others` : list(ids);
    if (ids.length) this.d.note(`${clockText(s.woke, false)}. When I woke, ${who} ${ids.length > 1 ? 'were' : 'was'} pointing down at the grass where I lay. ${ids.length > 1 ? 'They' : 'It'} had turned to it when the light came, before I was there.`);
  }
  private goHome(s: FollowIn) {
    this.mode = 'home';
    for (const r of this.rigs.values()) {
      r.locked = false;
      if (r.on && r.tier === 0 && !r.run && this.t - r.clunkT > 1) { r.run = true; r.still = 0; r.clunkT = this.t; this.sound.clunk(r.d, true, s.calm, rnd(0.1, 0.9)); }
    }
  }
  /** Gone from SARO's area: home at once (end() puts each one there). */
  private home(clock: number) { this.end(clock); }
  private end(clock: number) {
    const on = [...this.rigs.values()].filter((r) => r.on).sort((a, b) => a.onAt - b.onAt);
    for (const r of this.rigs.values()) this.release(r);
    this.d.log(`ARRAY  ${on.length} DRIVES  AZ 026 EL 32  NO COMMAND`, clock);
    this.d.store.set({ clock: Math.round(this.clock0), ids: on.map((r) => r.d.id), back: true });
    this.sound.close();
    this.rigs.clear(); this.mode = 'idle';
    this.d.save();
  }
  private release(r: Rig) {
    r.d.snap(HOME.az, HOME.el); r.d.rogue = false; r.d.spin = 0;
    this.d.array.beaconPhase(r.i, r.d.phase);
  }
  /** What the notebook keeps, once the warning is over (and the log's line for the ones further out). */
  private record(after: string) {
    const on = [...this.rigs.values()].filter((r) => r.on).sort((a, b) => a.onAt - b.onAt);
    const near = on.filter((r) => r.tier === 0).map((r) => r.d.id), more = on.length - near.length;
    const far = on.filter((r) => r.tier > 0).map((r) => r.d.id);
    if (far.length && this.firstFar >= 0) this.d.log(`ARRAY  ${far.slice(0, 5).join(' ')}${far.length > 5 ? ` +${far.length - 5}` : ''}  MOTION  NO COMMAND`, this.firstFar);
    if (!near.length) return;
    const one = near.length === 1, saw = on.some((r) => r.ever);
    const after1 = !more ? '' : more > 12 ? `, and most of the array further out after ${one ? 'it' : 'them'}` : `, and ${word(more)} more further out after ${one ? 'it' : 'them'}`;
    const they = one && !more ? 'it' : 'they';
    this.d.note(`${clockText(this.clock0, false)}. Out in the array, with the crickets gone. ${list(near)} came down out of 026 by ${one ? 'itself' : 'themselves'} and ${saw ? 'turned to me' : 'began to turn towards me'}${after1}. ${this.moved ? `When I moved, ${they} followed.` : `I stood still, and ${they} stayed on me.`} Nobody was at the console.${after}`);
    const v = this.d.store.get();
    if (v) this.d.store.set({ ...v, ids: on.map((r) => r.d.id) });
  }

  // ---------- the drives ----------
  private step(dt: number, s: FollowIn) {
    const home = this.mode === 'home', away = this.mode === 'away';
    // the one over the player: the only one that pushes against its limit
    let over: Rig | null = null, oh = Infinity;
    for (const r of this.rigs.values()) { const h = Math.hypot(r.d.x - s.eye.x, r.d.z - s.eye.z); if (r.on && h < oh) { oh = h; over = r; } }
    for (const r of this.rigs.values()) {
      if (!r.on) continue;
      const k = home ? BACK : r.rate, m = away ? AWAY : 1;
      let tAz = HOME.az, want = HOME.el;
      if (!home) { const a = aimAt(r.d, r.aim); tAz = a.az; want = a.el; }
      const tEl = THREE.MathUtils.clamp(want, EL_MIN, 88);
      const eAz = wrap180(tAz - r.az), eEl = tEl - r.el;
      r.vAz = axis(r.vAz, eAz, k.az * m, k.acc * m, dt);
      r.vEl = axis(r.vEl, eEl, k.el * m, k.acc * m * 0.7, dt);
      // the last step onto the target lands on it and stops there (exactly home at the end), so it
      // does not hunt to and fro about it
      if (Math.abs(eAz) <= Math.abs(r.vAz) * dt + 0.02 && Math.abs(r.vAz) < 4) { r.az = tAz; r.vAz = 0; } else r.az = norm(r.az + r.vAz * dt);
      if (Math.abs(eEl) <= Math.abs(r.vEl) * dt + 0.02 && Math.abs(r.vEl) < 4) { r.el = tEl; r.vEl = 0; } else r.el += r.vEl * dt;
      const was = r.locked;
      r.locked = !home && Math.abs(wrap180(tAz - r.az)) < 3 && Math.abs(tEl - r.el) < 3;
      if (r.locked) r.ever = true;
      if (r.locked && !was && r.tier === 0 && this.mode === 'follow' && !this.said.lock) {
        this.said.lock = true; this.lockAt.copy(s.eye);
        this.d.toast(`${r.d.id} has come down out of 026. It is pointing at me.`, 4);
      }
      // down on its final limit with the player lower still: the console has it; and the one right
      // over the player pushes against it now and then, a groan in the quiet
      let off = 0;
      const atLimit = this.mode === 'follow' && want < EL_MIN - 0.5 && r.el <= EL_MIN + 0.3 && Math.abs(eAz) < 5;
      if (atLimit && r.tier === 0 && !r.limit) { r.limit = true; this.d.log(`DRIVE ${r.d.id}  EL FINAL LIMIT  NO COMMAND`, s.clock); }
      if (atLimit && r === over && want < EL_MIN - 5 && !r.run) {
        if (r.push > 0) { r.push -= dt; off = -0.5 * Math.sin(Math.PI * Math.max(0, 1 - r.push / 0.8)); }
        else if ((r.strain -= dt) <= 0) { r.push = 0.8; r.strain = rnd(3.5, 6); }
      } else r.push = 0;
      // running: its motor, and a brake each way (the near ones)
      const sp = Math.abs(r.vAz) + Math.abs(r.vEl);
      if (!r.run && sp > 0.7) { r.run = true; r.still = 0; if (r.tier === 0 && this.t - r.clunkT > 1.2) { r.clunkT = this.t; this.sound.clunk(r.d, true, s.calm); } }
      else if (r.run && sp < 0.1) { if ((r.still += dt) > 0.45) { r.run = false; if (r.tier === 0 && this.t - r.clunkT > 1.2) { r.clunkT = this.t; this.sound.clunk(r.d, false, s.calm); } } }
      else if (r.run) r.still = 0;
      // the lamps of those on the player blink as one; afterwards each its own way again
      if (this.mode === 'follow' && r.locked) { if (this.common === null) this.common = r.ph; }
      const tgt = !home && r.ever && this.common !== null ? this.common : r.d.phase;
      r.ph += THREE.MathUtils.clamp(wrapPi(tgt - r.ph), -0.9 * dt, 0.9 * dt);
      this.d.array.beaconPhase(r.i, r.ph);
      this.write(r, off);
    }
    this.sound.update(dt, this.rigs, s.eye, this.mode === 'follow');
  }
  private write(r: Rig, off: number) {
    const d = r.d;
    d.curAz = d.tgtAz = r.az; d.curEl = d.tgtEl = r.el + off;
    d.rogue = true; d.spin = Math.abs(r.vAz) + Math.abs(r.vEl) + (off ? 0.5 : 0);
  }

  // ---------- the case ----------
  /** Everything stopped, the dishes home and their own again (a new night, the title). */
  reset() {
    this.sound.close();
    for (const r of this.rigs.values()) this.release(r);
    this.rigs.clear(); this.mode = 'idle'; this.t = 0;
  }
  /** After a case is opened (the abduction's save restored, the log with it): a save made while
   *  they were out has them home, and the line that says so. */
  restore() {
    this.reset();
    const v = this.d.store.get();
    if (v && !v.back) {
      this.d.log(`ARRAY  ${v.ids.length ? `${v.ids.length} DRIVES  ` : ''}AZ 026 EL 32  NO COMMAND`, v.clock + 90);
      this.d.store.set({ ...v, back: true });
    }
  }
}

/** Where a dish is from the player's view, in words. */
function where(eye: THREE.Vector3, view: THREE.Vector3, d: Dish) {
  const fx = view.x, fz = view.z, dx = d.x - eye.x, dz = d.z - eye.z;
  const f = Math.hypot(fx, fz) || 1, h = Math.hypot(dx, dz) || 1;
  const cos = (fx * dx + fz * dz) / (f * h), side = fx * dz - fz * dx;
  return cos > 0.64 ? 'Ahead of me' : cos < -0.5 ? 'Behind me' : side > 0 ? 'To my right' : 'To my left';
}

// ---------- the sound of the drives ----------
// The motor of 02:15 (AudioSys.startMotors: a sawtooth low down, through a low-pass), placed at the
// dish and sped up and slowed with it, with the whine of its gears over it; a contactor and a
// brake each way (the bought switch and heavy door from the private repo, pitched down, or the
// game's own when they are not there). Three voices for the nearest that move; the rest of the
// array that moves is one bed, not placed, as it was heard from the control room at 02:15.
class Voice {
  rig: Rig | null = null;
  private g: GainNode; private pan: PannerNode; private saw: OscillatorNode; private gear: OscillatorNode;
  private lp: BiquadFilterNode; private bp: BiquadFilterNode; private lfo: OscillatorNode;
  level = 0;
  constructor(private ctx: AudioContext, out: AudioNode) {
    this.saw = ctx.createOscillator(); this.saw.type = 'sawtooth'; this.saw.frequency.value = 34;
    this.lp = ctx.createBiquadFilter(); this.lp.type = 'lowpass'; this.lp.frequency.value = 300; this.lp.Q.value = 0.8;
    this.gear = ctx.createOscillator(); this.gear.type = 'square'; this.gear.frequency.value = 156;
    this.bp = ctx.createBiquadFilter(); this.bp.type = 'bandpass'; this.bp.frequency.value = 156; this.bp.Q.value = 7;
    const gg = ctx.createGain(); gg.gain.value = 0.2;
    this.lfo = ctx.createOscillator(); this.lfo.frequency.value = 5.3;
    const depth = ctx.createGain(); depth.gain.value = 3;
    this.g = ctx.createGain(); this.g.gain.value = 0;
    const p = ctx.createPanner();
    p.panningModel = 'equalpower'; p.distanceModel = 'inverse'; p.refDistance = 7; p.rolloffFactor = 1; p.maxDistance = 400;
    this.pan = p;
    this.saw.connect(this.lp); this.lp.connect(this.g);
    this.gear.connect(this.bp); this.bp.connect(gg); gg.connect(this.g);
    this.lfo.connect(depth); depth.connect(this.gear.frequency);
    this.g.connect(p); p.connect(out);
    for (const o of [this.saw, this.gear, this.lfo]) o.start();
  }
  set(r: Rig | null, t: number) {
    this.rig = r;
    const now = this.ctx.currentTime;
    if (!r) { this.level = 0; this.g.gain.setTargetAtTime(0, now, 0.25); return; }
    const d = r.d, y = 8 * d.scale, p = this.pan as any;
    if (p.positionX) { p.positionX.value = d.x; p.positionY.value = y; p.positionZ.value = d.z; } else p.setPosition(d.x, y, d.z);
    const k = THREE.MathUtils.clamp((Math.abs(r.vAz) + Math.abs(r.vEl)) / (r.rate.az * 0.75), 0, 1);
    const strain = r.push > 0;
    const f = strain ? 24 : 30 + 30 * k;
    this.saw.frequency.setTargetAtTime(f, now, 0.18);
    this.gear.frequency.setTargetAtTime(f * 4.6, now, 0.18); this.bp.frequency.setTargetAtTime(f * 4.6, now, 0.18);
    this.lp.frequency.setTargetAtTime(strain ? 520 : 220 + 700 * k, now, 0.2);
    // pushing against the limit: low, louder, juddering
    // a drive that runs is heard even when it barely turns (the hum of it; the pitch goes with the speed)
    this.level = strain ? 0.34 * (0.55 + 0.45 * Math.sign(Math.sin(t * Math.PI * 2 * 7))) : 0.46 * (0.45 + 0.55 * Math.pow(k, 0.7)) * (r.tier === 0 ? 1 : 0.7);
    this.g.gain.setTargetAtTime(this.level, now, strain ? 0.012 : 0.12);
  }
  mute(t: number) { this.rig = null; this.level = 0; this.g.gain.cancelScheduledValues(t); this.g.gain.setValueAtTime(0, t); }
  stop() {
    const now = this.ctx.currentTime;
    this.g.gain.cancelScheduledValues(now); this.g.gain.setTargetAtTime(0, now, 0.15);
    for (const o of [this.saw, this.gear, this.lfo]) { try { o.stop(now + 1); } catch { /* stopped */ } }
    setTimeout(() => this.g.disconnect(), 1200);
  }
}

class Drives {
  private voices: Voice[] = [];
  private bed: { osc: OscillatorNode; g: GainNode } | null = null;
  private pick = 0;
  private tick = 0;
  private time = 0;
  constructor(private audio: AudioSys) {}
  /** The voices that sound now (tests): the dish each is on and its level. */
  get live() { return this.voices.filter((v) => v.rig && v.level > 0.01).map((v) => ({ id: v.rig!.d.id, level: v.level })); }

  open() {
    const ctx = this.audio.ctx;
    if (!ctx || this.voices.length) return;
    for (let i = 0; i < 3; i++) this.voices.push(new Voice(ctx, this.audio.sfx));
    // the rest of the array, as at 02:15
    const osc = ctx.createOscillator(); osc.type = 'sawtooth'; osc.frequency.value = 58;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 240;
    const g = ctx.createGain(); g.gain.value = 0;
    osc.connect(lp); lp.connect(g); g.connect(this.audio.amb); osc.start();
    this.bed = { osc, g };
  }
  /** Silent at once (the sound back after the time away). */
  hush() {
    const t = this.audio.ctx?.currentTime ?? 0;
    for (const v of this.voices) v.mute(t);
    if (this.bed) { this.bed.g.gain.cancelScheduledValues(t); this.bed.g.gain.setValueAtTime(0, t); }
  }
  close() {
    for (const v of this.voices) v.stop();
    this.voices = [];
    if (this.bed && this.audio.ctx) {
      const t = this.audio.ctx.currentTime, b = this.bed;
      b.g.gain.cancelScheduledValues(t); b.g.gain.setTargetAtTime(0, t, 0.4); b.osc.frequency.setTargetAtTime(30, t, 0.8);
      try { b.osc.stop(t + 3); } catch { /* stopped */ }
    }
    this.bed = null;
  }
  update(dt: number, rigs: Map<string, Rig>, eye: THREE.Vector3, nearOnly: boolean) {
    this.time += dt;
    if (!this.voices.length) { this.open(); if (!this.voices.length) return; }
    const moving = [...rigs.values()].filter((r) => r.on && (r.run || r.push > 0));
    if ((this.pick -= dt) <= 0) {
      this.pick = 0.3;
      // while they follow, the voices are the near ones' only (standing still, quiet near by); going
      // home, the nearest that move; the rest is the bed
      const near = moving.filter((r) => !nearOnly || r.tier === 0).map((r) => ({ r, h: Math.hypot(r.d.x - eye.x, r.d.z - eye.z) })).sort((a, b) => a.h - b.h).slice(0, this.voices.length).map((x) => x.r);
      // keep a voice on its dish while that one is still among the nearest
      const free = this.voices.filter((v) => !v.rig || !near.includes(v.rig));
      for (const r of near) if (!this.voices.some((v) => v.rig === r)) { const v = free.shift(); if (v) v.rig = r; }
      for (const v of free) v.rig = null;
    }
    // the parameters twenty times a second is enough (and keeps the automation short)
    if ((this.tick -= dt) > 0) return;
    this.tick = 0.05;
    for (const v of this.voices) v.set(v.rig && moving.includes(v.rig) ? v.rig : null, this.time);
    if (this.bed && this.audio.ctx) {
      const rest = moving.filter((r) => !this.voices.some((v) => v.rig === r)).reduce((a, r) => a + Math.abs(r.vAz) + Math.abs(r.vEl), 0);
      const now = this.audio.ctx.currentTime;
      // further off than the near ones: lower than the whole array was from the control room at 02:15
      this.bed.g.gain.setTargetAtTime(0.07 * Math.min(1, rest / 12), now, 0.6);
      this.bed.osc.frequency.setTargetAtTime(46 + 12 * Math.min(1, rest / 12), now, 1.2);
    }
  }
  /** A contactor and the brake: letting go (start) or setting (stop), at the dish's foot. */
  clunk(d: Dish, start: boolean, calm: boolean, when = 0) {
    const a = this.audio, sp = a.spatial(new THREE.Vector3(d.x, 6 * d.scale, d.z), 6);
    if (!sp) return;
    const k = calm ? 0.6 : 1, sw = a.or('rack_switch', 'switch'), brake = a.or('door_heavy', 'doorMetal');
    if (start) {
      a.play(sw, { dest: sp, gain: 0.5 * k, rate: 0.62, when });
      a.play(brake, { dest: sp, gain: 0.4 * k, rate: 0.5, lp: 1300, when: when + 0.09 });
    } else {
      a.play(brake, { dest: sp, gain: 0.34 * k, rate: 0.45, lp: 900, when });
      a.play(sw, { dest: sp, gain: 0.36 * k, rate: 0.55, when: when + 0.06 });
    }
  }
}
