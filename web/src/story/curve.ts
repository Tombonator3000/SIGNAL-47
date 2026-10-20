import type { SeenEntry, SeenLog } from './seen';

// One pressure for the whole night (FORSLAG.md, 20 October, item 1; HENDELSER.md, section 4: build
// up, peak, breathe out). Before this, the night's things ran on timers of their own (the calls,
// the glimpses, the cutout, the night's events, the time away) and the calls and the cutout had no
// pacing at all, so they became wallpaper. Now they ask this one curve before they start.
//
//   plain   from the moment chapter one lets the player out (02:16), ten minutes of plain night:
//           the crickets, the wind, and nothing else.
//   build   the signs pile up, slowly, then quicker: the crickets thin out one by one (Ambience.thin),
//           the flashlight stutters, the dash lights blink, the truck's radio slides under its own
//           static, a call far off out in the dark (core/calls.ts). The cardboard alien turns up
//           somewhere out of the view (core/cutouts.ts), in the build only. From half way, one of
//           the night's events (story/nightEvents.ts, the town's, the walk to the motel).
//   peak    one of the big ones: a glimpse at the edge of the view, the warning (the time away, the
//           dishes in the field), the steps on the archive roof, or a 47-second event (the silence,
//           the 1947 bulletin, the payphone, the motel's television), at most one of those extra
//           ones in a chapter. Earl's reel from 1981 (core/reelTape.ts) is the player's own choice
//           and not the curve's, but once heard it lands the peak like the others: the rest follows.
//           What the player actually got moves the curve (story/seen.ts): a
//           glimpse counts once it was looked at; one that stood there unseen does not, and the peak
//           goes on. Out where nothing can reach the player (indoors) the peak waits for them.
//   rest    8 to 12 minutes in which nothing can happen: no calls, no cutout, no glimpses, no events.
//           The crickets come back. Then the next build.
//
// Each cycle is a little stronger than the one before it, towards 05:29: a shorter build, the
// signs closer together, the night thinner. The curve counts the player's play time, not the
// night's clock (the time away and the drives move the clock on). Nothing in the prologue, in a
// cinematic, on a call, in Nora's room, inside the diner in chapter five (the map), or from chapter
// six, when THE EVENT has the night. One thing at a time stays as it was: while one of the night's
// things is going, nothing else may start (main.ts, `oneAtATime`).
//
// Two of the 47-second events belong to a place and wait there for the player: the motel office's
// television (story/motelTv.ts; the truck on the lot) and the payphone's first call in Caliche
// (story/townEvents.ts; 02:40 to 04:45). Indoors the build goes at a third of its pace, and in a
// rest nothing may come, so a visit to the office or the gas station missed them more often than
// not (PR #123's critic). Where one of them is waiting now, the curve hurries: what is left of the
// build goes in at most HURRY seconds, indoors too, and the peak keeps itself for that event (no
// glimpse, no call, no other event takes it; the peak does not ebb while the player stays). It
// still never comes in a rest, never on top of another thing, and never as a second extra 47 in a
// chapter. The place shows the hurry: the test card slips, the payphone's bell tinks in its box.
// A rest is most of the night, though (8 to 12 minutes of every cycle), and a visit falls in one
// more often than not: so where the player stays on (LINGER) by a waiting 47, the rest keeps only
// its first three minutes, the breath after a peak (as long as the rest after one that never came,
// EBB), and lets go into the hurried build. The rest is shorter there; nothing comes in it.
//
// Chapter one has a 47 of its own that is the story's (story/saroDark.ts): SARO goes dark at 02:29,
// forty seconds after the relay. It is not the curve's to time and comes whatever its phase (never on
// top of another thing: one at a time), but it is the chapter's one 47 all the same: until it has come
// or its window (02:29 to 02:40) is past, no other 47 may take chapter one (CurveIn.keep), and when it
// comes it lands the peak like the other 47s, so the rest follows it.
//
// The face on the film (core/filmFace.ts) is not held back: it is on the print the player takes.
// When it is looked at large, that is the peak. The second call of the payphone after room 6 is the
// story's and not the curve's either. Neither are the things found as they were left (the deputy's
// car, the herd facing 026, the cutout of event 6 on the shoulder north of Caliche).

export type CurvePhase = 'off' | 'plain' | 'build' | 'peak' | 'rest';
/** What the night's things ask about: a small sign (the flashlight, the dash, the radio), a call
 *  out in the dark, the cutout (and its one night as a reflector post), a night or town event, a
 *  47-second event, a glimpse, the time away, a scare that comes once (the steps on the roof). */
export type CurveKind = 'sign' | 'call' | 'cutout' | 'post' | 'event' | 'long' | 'glimpse' | 'abduct' | 'scare';
/** What is kept with the case. */
export interface CurveSave { phase: CurvePhase; t: number; len: number; cycle: number; long: string[]; ebb?: boolean }

export interface CurveIn {
  /** The story's part of the night: 'ch1' to 'ch5' run the curve; the prologue and chapter six do not. */
  chapter: string;
  /** Held still, and nothing may start: a cinematic, a panel or the notebook open, a call on the line,
   *  Nora's room, inside the diner in chapter five. */
  held: boolean;
  /** Out where the night can reach the player: on foot outdoors, or driving. */
  exposed: boolean;
  /** One of the night's things is going: nothing else may start (one at a time). */
  busy: boolean;
  /** The peak's own thing is going (the time away, the steps on the roof, the silence, the quiet
   *  after the face): the rest after it counts from when it is over. */
  peaking: boolean;
  /** The night's clock, seconds since midnight. */
  clock: number;
  /** One of the 47-second events that belong to a place is waiting there now (the motel office's
   *  television, the payphone's first call): the player is where it happens and it could come but
   *  for the curve. The build hurries to the peak, and the peak is kept for it. */
  waiting?: boolean;
  /** A 47 of the story's may still come in this chapter (story/saroDark.ts, chapter one): the
   *  chapter's one extra 47 is kept for it. */
  keep?: boolean;
}

const PLAIN = 600;                                  // ten minutes of plain night after the prologue
const REST: [number, number] = [480, 720];          // after a peak: 8 to 12 minutes
const EBB: [number, number] = [180, 240];           // after a peak that never came to anything
const SHORT = 150;                                  // a night put back from a case without a curve
const BUILD: [number, number] = [390, 170];         // the build's length at 02:16 and at 05:29 (play seconds)
const PEAK: [number, number] = [300, 240];          // how long the peak waits, out in the night, for something to land
const HURRY = 40;                                   // a waiting 47 here: the rest of the build in at most this (play seconds)
const LINGER = 5;                                   // and staying by it this long, the rest lets go after its first EBB[0] s
const NIGHT0 = 2 * 3600 + 16 * 60, NIGHT1 = 5 * 3600 + 29 * 60;
const CHAPTERS = ['ch1', 'ch2', 'ch3', 'ch4', 'ch5'];
/** How far into the build each kind may start (glimpses and the time away only at the peak). */
const FROM: Record<CurveKind, number> = { sign: 0.12, call: 0.3, cutout: 0.12, post: 0.2, event: 0.45, long: 0.6, scare: 0.5, glimpse: 9, abduct: 9 };
/** The cutout keeps out of the end of the build: never there at the peak's moment. */
const CUTOUT_UNTIL = 0.78;
/** The peaks that are heard or felt (they count when they come), and those that count only once
 *  they were seen (story/seen.ts, looking). */
const FELT: Record<string, string[]> = { abduction: ['warning'], roof: ['steps'], scare: ['silence'], event: ['bulletin', 'phone', 'phone2', 'tv', 'lamps', 'reel', 'blackout'] };
const SEEN: Record<string, (what: string) => boolean> = { glimpse: (w) => w !== 'on film', face: () => true, event: (w) => w === 'lights' };
/** The 47-second ones (the per-chapter limit). */
const LONG = new Set(['scare:silence', 'event:bulletin', 'event:phone', 'event:tv', 'event:blackout']);

const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const lerp = (a: number, b: number, k: number) => a + (b - a) * k;
const clamp01 = (v: number) => Math.min(1, Math.max(0, v));
const smooth = (a: number, b: number, v: number) => { const k = clamp01((v - a) / (b - a)); return k * k * (3 - 2 * k); };

export class NightCurve {
  phase: CurvePhase = 'off';
  /** Seconds into the phase, and how long it runs (the build's length; the peak's wait). */
  t = 0;
  len = 0;
  /** 0..1: how far up this cycle is (the build rises to 1, the peak holds it, the rest lets it go). */
  level = 0;
  /** How many peaks have come tonight, and how many builds have begun (since the case was opened). */
  cycle = 0;
  builds = 0;
  /** 0..1 over the night, 02:16 to 05:29: each cycle a little stronger. */
  strength = 0;
  /** The chapters that have had their one extra 47-second event. */
  readonly long = new Set<string>();
  /** The last peak that landed: what, at what clock (the tests, the ?debug overlay). */
  landed: { kind: string; what: string; clock: number } | null = null;
  /** The last rest came after a peak that never landed (shorter). */
  ebb = false;
  /** The curve stands aside: everything as on its own timer before it, and no signs of its own
   *  (the tests of a single thing, the developer menu). */
  free = false;
  /** A small sign is due now (main.ts gives one where it can: the flashlight, the dash, the radio). */
  sign = false;
  /** A 47-second event is waiting where the player is (CurveIn.waiting), and the curve hurries
   *  for it: the build to the peak, and the peak kept for it. */
  pulled = false;
  /** 0..1 while the curve hurries in the build (the place shows it: the test card slips, the
   *  payphone's bell tinks); 0 otherwise. */
  get unrest() { return this.pulled && this.phase === 'build' ? this.level : 0; }
  /** How long the player has stayed by the waiting 47 (play seconds; LINGER). */
  private pullT = 0;
  /** How many of each started in this cycle (the calls, the cutout, the events). */
  readonly took = { call: 0, cutout: 0, event: 0 };
  private chapter = '';
  /** The chapter's 47 is kept for the story's own (CurveIn.keep). */
  private keep = false;
  private held = true;
  private busy = false;
  private signT = 0;
  private read = 0;
  private pending: SeenEntry[] = [];
  private resume: 'plain' | 'short' = 'plain';
  private restored: CurveSave | null = null;

  constructor(private log: SeenLog) {}

  /** May a thing of this kind start now? */
  may(kind: CurveKind): boolean {
    if (this.free) return true;
    if (this.held || this.busy) return false;
    if (kind === 'long' && (this.long.has(this.chapter) || this.keep)) return false;
    // the peak this is hurrying to is kept for the 47 that waits here: nothing else takes it
    if (this.pulled && kind !== 'long' && kind !== 'sign') return false;
    const s = this.strength;
    if (kind === 'call' && this.took.call >= (s > 0.45 ? 2 : 1)) return false;
    if (kind === 'event' && this.took.event >= (s > 0.5 ? 2 : 1)) return false;
    if (this.phase === 'build') {
      if (this.level < FROM[kind]) return false;
      if (kind === 'cutout') return this.level < CUTOUT_UNTIL && this.took.cutout < (s > 0.6 ? 2 : 1);
      if (kind === 'post') return this.level < 0.85;
      return true;
    }
    if (this.phase === 'peak') return kind !== 'cutout' && kind !== 'post';
    return false;
  }
  /** May the cutout that is out and not yet found stay there? Only while the build is on: never at
   *  the peak's moment, never in the quiet after it, and not while something else is going. */
  get cutoutKeep() {
    if (this.free) return true;
    return this.phase === 'build' && this.level < CUTOUT_UNTIL && !this.busy;
  }
  /** How much sooner a thing that waits on a timer comes now (1: its own pace). */
  pace(kind: 'call' | 'glimpse' | 'event' | 'cutout'): number {
    if (this.free) return 1;
    if (this.phase === 'peak') return kind === 'glimpse' ? 3.5 : 1.5;
    if (this.phase === 'build') return kind === 'cutout' ? 3 : 1 + 1.2 * this.level;
    return 1;
  }
  /** How thin the crickets are, 0..1 (Ambience.thin): thinner as the build goes on, thinnest at the
   *  peak and towards morning, all back in the rest. */
  get thin() {
    if (this.free) return 0;
    if (this.phase === 'build') return smooth(0.08, 0.85, this.level) * lerp(0.6, 0.9, this.strength);
    if (this.phase === 'peak') return lerp(0.7, 0.95, this.strength);
    return 0;
  }
  /** Seconds left of the phase (the overlay). */
  get left() { return Math.max(0, Math.round(this.len - this.t)); }

  /** A thing has started (main.ts sees it): the calls, the cutout and the events have a limit in a cycle. */
  started(kind: keyof NightCurve['took']) { this.took[kind]++; }

  update(dt: number, s: CurveIn) {
    this.chapter = s.chapter;
    this.held = s.held; this.busy = s.busy; this.keep = !!s.keep;
    this.sign = false;
    // (not for a chapter that has had its extra 47, or keeps it for the story's: then there is
    // nothing here to hurry for)
    this.pulled = !this.free && !!s.waiting && !this.long.has(s.chapter) && !this.keep;
    if (!CHAPTERS.includes(s.chapter)) { if (this.phase !== 'off') this.stop(); return; }
    if (this.phase === 'off') this.begin();
    this.strength = clamp01((((s.clock % 86400) + 86400) % 86400 - NIGHT0) / (NIGHT1 - NIGHT0));
    this.readLog(s.clock);
    if (s.held) return;
    this.step(dt, s.exposed, s.busy, s.peaking);
  }

  private step(dt: number, exposed: boolean, busy: boolean, peaking: boolean) {
    this.pullT = this.pulled ? this.pullT + dt : 0;
    switch (this.phase) {
      case 'plain':
        if ((this.t += dt) >= this.len) this.to('build');
        break;
      case 'build': {
        // indoors the night goes on building, but slowly: it is out there it is felt; where a 47
        // waits for the player, what is left of it goes in at most HURRY seconds, indoors too
        const pace = exposed ? 1 : 0.35;
        this.t += dt * (this.pulled ? Math.max(pace, this.len / HURRY) : pace);
        this.level = clamp01(this.t / this.len);
        this.signs(dt, exposed, busy);
        if (this.level >= 1) this.to('peak');
        break;
      }
      case 'peak':
        this.level = 1;
        this.signs(dt, exposed, busy);
        // the peak waits while the player is somewhere it cannot reach them, or while something runs,
        // or while the 47 it is kept for waits for them here
        if (exposed && !busy && !this.pulled && (this.t += dt) >= this.len) { this.ebb = true; this.to('rest'); }
        break;
      case 'rest':
        this.level = Math.max(0, this.level - dt / 20);
        // (staying by a 47 that waits there: only the breath after the peak, then the hurried build)
        if (!peaking && ((this.t += dt) >= this.len || (this.pulled && this.pullT >= LINGER && this.t >= EBB[0]))) this.to('build');
        break;
    }
  }
  /** The small signs: now and then, closer together as the build goes on and towards morning.
   *  Not while one of the night's things is going (one at a time): a blink of the dash or the radio
   *  under its static would land on top of it (the 1947 bulletin would not be heard through). */
  private signs(dt: number, exposed: boolean, busy: boolean) {
    if (!exposed || busy || this.level < FROM.sign) return;
    if ((this.signT -= dt) > 0) return;
    this.sign = true;
    this.signT = rnd(28, 60) / (0.55 + this.level) * (1 - 0.35 * this.strength);
  }

  private to(phase: CurvePhase, len?: number) {
    this.phase = phase; this.t = 0;
    const s = this.strength;
    if (phase === 'plain') { this.len = len ?? PLAIN; this.level = 0; }
    else if (phase === 'build') {
      this.len = len ?? lerp(BUILD[0], BUILD[1], s) * rnd(0.85, 1.15);
      this.level = 0; this.ebb = false; this.builds++;
      this.took.call = this.took.cutout = this.took.event = 0;
      this.signT = rnd(20, 45);
    } else if (phase === 'peak') { this.len = len ?? lerp(PEAK[0], PEAK[1], s); this.level = 1; }
    else if (phase === 'rest') {
      // 8 to 12 minutes, the shorter towards morning; a peak that never landed lets go for less
      this.len = len ?? (this.ebb ? rnd(...EBB) : Math.min(REST[1], Math.max(REST[0], lerp(REST[1], REST[0], s) + rnd(-40, 40))));
    }
  }

  /** What the night did since the last look (story/seen.ts): a peak the player got lands the curve. */
  private readLog(clock: number) {
    const all = this.log.entries, fresh = Math.min(all.length, this.log.added - this.read);
    this.read = this.log.added;
    for (const e of fresh > 0 ? all.slice(-fresh) : []) {
      if (e.kind === 'call') { this.took.call++; continue; }
      if (FELT[e.kind]?.includes(e.what)) this.land(e, clock);
      else if ((e.kind === 'event' && !SEEN.event(e.what)) || (e.kind === 'scare' && e.what === 'car lights')) this.took.event++;
      else if (SEEN[e.kind]?.(e.what)) { if (e.looking) this.land(e, clock); else this.pending.push(e); }
    }
    // a glimpse counts once it was looked at; if its watch ends unseen it never does
    if (this.pending.length) this.pending = this.pending.filter((e) => {
      if (e.looking) { this.land(e, clock); return false; }
      return e.kind === 'face' || this.log.watching(e);
    });
  }
  private land(e: SeenEntry, clock: number) {
    if (LONG.has(`${e.kind}:${e.what}`) && this.chapter) this.long.add(this.chapter);
    this.landed = { kind: e.kind, what: e.what, clock: Math.round(clock) };
    this.cycle++;
    this.ebb = false;
    this.to('rest');
  }

  /** The free night begins (chapter one after the prologue), or a night put back goes on. */
  private begin() {
    const r = this.restored; this.restored = null;
    this.read = this.log.added; this.pending = [];
    if (r) {
      this.to(r.phase === 'off' ? 'plain' : r.phase, r.len > 0 ? r.len : undefined);
      this.t = Math.min(r.t, this.len); this.ebb = !!r.ebb;
      if (this.phase === 'build') this.level = clamp01(this.t / this.len);
      if (this.phase === 'peak') this.level = 1;
      return;
    }
    if (this.resume === 'short') this.to('rest', SHORT);
    else this.to('plain');
  }
  private stop() { this.phase = 'off'; this.t = 0; this.len = 0; this.level = 0; this.pending = []; this.pulled = false; this.pullT = 0; }

  /** What is kept with the case. */
  get data(): CurveSave {
    return { phase: this.phase, t: Math.round(this.t), len: Math.round(this.len), cycle: this.cycle, long: [...this.long], ebb: this.ebb || undefined };
  }
  /** A save opened (its curve, or none: an older case goes on after a short quiet), or a new night
   *  (null: the plain ten minutes when chapter one begins). A save from before the curve began (the
   *  prologue, from 02:13) has it 'off': its night still gets the plain ten minutes. */
  restore(save: CurveSave | null | undefined, started: boolean) {
    this.stop(); this.cycle = 0; this.long.clear(); this.landed = null; this.ebb = false;
    this.took.call = this.took.cutout = this.took.event = 0;
    this.restored = null; this.resume = started ? 'short' : 'plain';
    if (save && typeof save === 'object' && ['off', 'plain', 'build', 'peak', 'rest'].includes(save.phase)) {
      this.resume = 'plain';
      this.cycle = Number.isFinite(save.cycle) ? save.cycle : 0;
      for (const c of Array.isArray(save.long) ? save.long : []) if (CHAPTERS.includes(c)) this.long.add(c);
      if (save.phase !== 'off') this.restored = { ...save, t: Number.isFinite(save.t) ? save.t : 0, len: Number.isFinite(save.len) ? save.len : 0 };
    }
    this.read = this.log.added;
  }

  // ---------- the developer menu and the tests ----------
  /** Go to a phase now (in the build: at this level). */
  force(phase: Exclude<CurvePhase, 'off'>, level = 0) {
    if (this.phase === 'off' && !CHAPTERS.includes(this.chapter)) return false;
    this.to(phase);
    if (phase === 'build') { this.t = clamp01(level) * this.len; this.level = clamp01(level); }
    return true;
  }
  /** Let this much of the night's play time go by at once, out in the night with nothing going. */
  advance(secs: number) {
    for (let left = secs; left > 0; left -= 1) this.step(Math.min(1, left), true, false, false);
    this.sign = false;
  }
  /** One line for the ?debug overlay. */
  text() {
    if (this.phase === 'off') return `curve off${this.free ? ' (free)' : ''}`;
    const l = [...this.long];
    return `curve ${this.phase} ${this.level.toFixed(2)} ${this.left}s c${this.cycle} s${this.strength.toFixed(2)}${this.pulled ? ' pull' : ''}${l.length ? ' 47:' + l.join(',') : ''}${this.landed ? ' last ' + this.landed.what.slice(0, 10) : ''}${this.free ? ' free' : ''}`;
  }
}
