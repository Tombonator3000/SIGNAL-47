import * as THREE from 'three';
import type { Dish, DishArray } from '../world/Dish';
import type { AudioSys } from '../core/Audio';

// The array tips down to the horizon along 026, all 27 at once, and later goes back up to 32
// (story/archivePhone.ts: the archive's phone, chapter two; FORSLAG.md, 20 October, Valg for Tom 1).
// Kept small and apart from the rest on purpose: the dishes have one owner at a time.
//
// Who has the dishes. Since 02:15 they stand at az 026, el 32 with nobody driving them (the
// prologue snaps them there). A story thing that moves them marks each one `rogue` (world/Dish.ts:
// "driven by nothing on the console") for as long as it has them, and gives them back exactly at
// 026 / 32 with `rogue` off: the dishes that follow the player in a warning (story/dishesFollow.ts)
// do it so, and this does too. So this takes the array only when no dish is rogue (nothing else has
// them), and while it has them `holds` is true: anything else that would drive them (the console's
// schedule, ARRAY RESUME, the warning's dishes) leaves them alone until it is false again. If the
// array is on the schedule when the tipping comes, it leaves the schedule: they go from wherever
// they are to 026 and down.
//
//   tip()      every drive at once: a ragged clunk of 27 brakes letting go, the motors of 02:15,
//              and every dish down from where it is to el 2 on 026, together, in some twenty
//              seconds; the brakes again all at once when they get there, and the motors stop.
//              The red lamps blink as one while they are down.
//   raise()    the same the other way: up to 32, then given back (`rogue` off, exactly 026 / 32).
//   home()     at once, unseen (nobody at SARO to see it): back at 026 / 32 and given back.
//   restore()  a night put back: tipped (as they were saved, at rest), or not.
//
// The sound is one bed for the array (not placed, as it was heard from the control room at 02:15),
// darker and quieter indoors, and the brakes at the nearest few dishes. Cheap on a phone: the
// dishes move by their own Dish.point()/update(); the array refreshes only the ones that move.

export const TIP = { az: 26, el: 2 };
export const HOME = { az: 26, el: 32 };
/** How fast (Dish.slew, degrees a second in azimuth; elevation goes at 0.6 of it): 30 degrees of
 *  elevation in about 22 seconds, the pace of the drives at 02:15. */
const SLEW = 2.3;

export interface ArrayTipDeps {
  dishes: Dish[];
  array: DishArray;
  audio: AudioSys;
}

type Mode = 'idle' | 'tipping' | 'down' | 'raising';

export class ArrayTip {
  mode: Mode = 'idle';
  /** Indoors (the annex, the control room): the motors are heard through the walls (main.ts). */
  muffled = true;
  private bed: { osc: OscillatorNode[]; lp: BiquadFilterNode; g: GainNode; whine: GainNode } | null = null;
  private common = 0;
  constructor(private d: ArrayTipDeps) {}

  /** This has the dishes (tipping, down, or on the way up): nothing else may drive them. */
  get holds() { return this.mode !== 'idle'; }
  /** They are moving now (the motors run; no save meanwhile). */
  get moving() { return this.mode === 'tipping' || this.mode === 'raising'; }
  /** Every dish where the tipping leaves them, at rest. */
  get down() { return this.mode === 'down'; }
  /** Something else has them now (a dish is rogue and this does not hold them). */
  get taken() { return !this.holds && this.d.dishes.some((x) => x.rogue); }

  /** Every drive at once, down to the horizon along 026. False if something else has them. */
  tip(): boolean {
    if (this.holds || this.taken) return false;
    for (const x of this.d.dishes) { x.rogue = true; x.point(TIP.az, TIP.el, SLEW); }
    this.common = this.d.dishes[0]?.phase ?? 0;
    this.mode = 'tipping';
    this.start();
    return true;
  }
  /** Back up to 32 on 026, all at once, and given back when they are there. */
  raise(): boolean {
    if (this.mode !== 'down') return false;
    for (const x of this.d.dishes) x.point(HOME.az, HOME.el, SLEW);
    this.mode = 'raising';
    this.start();
    return true;
  }
  /** At once, nobody there to see it: home, and given back. */
  home() {
    if (!this.holds) return;
    this.release();
    this.quiet(0.05);
  }
  /** A night put back: tipped and at rest, or not this one's at all. */
  restore(tipped: boolean) {
    this.quiet(0.01);
    if (this.holds) this.release();
    if (!tipped) return;
    for (const x of this.d.dishes) { x.rogue = true; x.snap(TIP.az, TIP.el); }
    this.common = this.d.dishes[0]?.phase ?? 0;
    this.mode = 'down';
    this.beacons(true);
  }
  /** A new night, the title: whatever this had, back as the night left it (026 / 32), quiet. */
  reset() { this.quiet(0.01); if (this.holds) this.release(); }

  /** Every frame while a night runs. `done`: they have just arrived (down, or home). */
  update(_dt: number): 'down' | 'home' | null {
    if (this.bed && this.d.audio.ctx) {
      const now = this.d.audio.ctx.currentTime;
      this.bed.lp.frequency.setTargetAtTime(this.muffled ? 260 : 820, now, 0.4);
      this.bed.whine.gain.setTargetAtTime(this.muffled ? 0.0 : 0.05, now, 0.4);
    }
    if (!this.moving) return null;
    if (this.mode === 'tipping') this.beacons(true);
    if (this.d.dishes.some((x) => x.moving)) return null;
    // all there: every brake at once, and the motors stop
    this.brakes(false);
    this.quiet(0.5);
    if (this.mode === 'tipping') { this.mode = 'down'; return 'down'; }
    this.release();
    return 'home';
  }

  // ---------- the dishes ----------
  private release() {
    this.d.dishes.forEach((x, i) => { x.snap(HOME.az, HOME.el); x.rogue = false; x.spin = 0; x.slew = 6; this.d.array.beaconPhase(i, x.phase); });
    this.mode = 'idle';
  }
  /** The red lamps of the tipped array blink as one. */
  private beacons(on: boolean) {
    this.d.dishes.forEach((x, i) => this.d.array.beaconPhase(i, on ? this.common : x.phase));
  }

  // ---------- the sound ----------
  /** The nearest dishes to SARO's buildings, where the brakes are heard from (the first rows). */
  private near(n = 4) {
    return [...this.d.dishes].sort((a, b) => Math.hypot(a.x, a.z) - Math.hypot(b.x, b.z)).slice(0, n);
  }
  /** Every brake letting go (or setting) within a few hundredths of a second: one ragged clunk. */
  private brakes(start: boolean) {
    const a = this.d.audio; if (!a.ctx) return;
    const sw = a.or('rack_switch', 'switch'), brake = a.or('door_heavy', 'doorMetal');
    this.near().forEach((x, i) => {
      const sp = a.spatial(new THREE.Vector3(x.x, 6 * x.scale, x.z), 9);
      if (!sp) return;
      const w = i * 0.013 + Math.random() * 0.02, k = this.muffled ? 0.7 : 1;
      a.play(brake, { dest: sp, gain: (start ? 0.55 : 0.45) * k, rate: start ? 0.48 : 0.42, lp: this.muffled ? 700 : 1300, when: w });
      a.play(sw, { dest: sp, gain: 0.4 * k, rate: 0.55, lp: this.muffled ? 900 : undefined, when: w + 0.06 });
    });
    // the rest of the array, further off, as one dull knock
    a.play('thudSoft', { gain: this.muffled ? 0.5 : 0.35, rate: 0.4, lp: 600, when: 0.05 });
  }
  private start() {
    this.brakes(true);
    const a = this.d.audio, ctx = a.ctx;
    if (!ctx) return;
    if (!this.bed) {
      // 27 drives on one bus, a little apart in speed: the beat between them is the array
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = this.muffled ? 260 : 820; lp.Q.value = 0.7;
      const g = ctx.createGain(); g.gain.value = 0;
      const whine = ctx.createGain(); whine.gain.value = 0;
      lp.connect(g); g.connect(a.amb); whine.connect(g);
      const osc = [46, 51.5, 57, 61.8].map((f, i) => {
        const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = f * 0.6;
        const og = ctx.createGain(); og.gain.value = [0.5, 0.35, 0.3, 0.22][i]; o.connect(og); og.connect(lp); o.start();
        o.frequency.setTargetAtTime(f, ctx.currentTime, 1.4);
        return o;
      });
      // the gears, heard outdoors only
      const gear = ctx.createOscillator(); gear.type = 'square'; gear.frequency.value = 236;
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 236; bp.Q.value = 6;
      gear.connect(bp); bp.connect(whine); gear.start(); osc.push(gear);
      this.bed = { osc, lp, g, whine };
    }
    this.bed.g.gain.setTargetAtTime(0.16, ctx.currentTime, 0.9);
  }
  private quiet(tau: number) {
    const b = this.bed, ctx = this.d.audio.ctx; this.bed = null;
    if (!b || !ctx) return;
    const t = ctx.currentTime;
    b.g.gain.cancelScheduledValues(t); b.g.gain.setTargetAtTime(0, t, tau);
    for (const o of b.osc) { o.frequency.setTargetAtTime(o.frequency.value * 0.5, t, tau * 1.6); try { o.stop(t + tau * 8 + 0.1); } catch { /* stopped */ } }
    setTimeout(() => { try { b.g.disconnect(); } catch { /* gone */ } }, (tau * 8 + 0.5) * 1000);
  }
  /** Whether the motors are heard now (tests). */
  get humming() { return !!this.bed; }
}
