import * as THREE from 'three';
import type { LineId } from './lines';

// What the night did, and whether the player saw it (FORSLAG.md, 20 October, "Hull": nobody
// measured what happened and was seen). Every scare, event, glimpse, time away and call is written
// here when it happens: the game's clock, where the player was, which line it was on (story/lines.ts)
// and whether the player was looking. Kept with the case (main.ts, GameState.seen), shown on the
// ?debug overlay, S47.seen for the tests. It changes nothing in the night; the next step's pacing
// reads it.
//
// Looking: the camera faced the thing, within an angle of the middle of the view, on the screen
// (a phone held upright sees less to the sides) and within a range, at any moment while it was
// there (a watch). For a glimpse that is the angle at which it goes (25 degrees); for a sound out
// in the dark, whether the player turned to it; for what is only heard or only on film, null, or
// what the thing itself counts as seen (the face: the print opened large; the one over you on
// waking: the eyes went up to it).

export type SeenKind = 'glimpse' | 'call' | 'scare' | 'event' | 'abduction' | 'over' | 'dishes' | 'face' | 'roof';
export interface SeenEntry {
  kind: SeenKind;
  /** Which: a glimpse's place, a sound's name, an event's id ('phone', 'cows', 'steps', ...). */
  what: string;
  /** The game's clock (seconds since midnight, whole). */
  clock: number;
  /** Where the player was (the save card's name for it), and the area. */
  place: string;
  area: string;
  /** Where the thing was, on the map (road-local metres, whole), when it had a place there. */
  at?: [number, number];
  line: LineId | null;
  /** The player looked at it while it was there; null when there was nothing to look at. */
  looking: boolean | null;
  /** How it ended, where that is part of it (the time away: 'away' or 'taken'). */
  end?: string;
}
/** Watching a thing for a while after it is written down: where it is now, for how long at most,
 *  from how far and within how many degrees of the middle of the view. Without `until`, a null
 *  place means it has gone and the watch ends; with it, nothing to look at just now (the watch ends
 *  when `until` says so). */
export interface SeenWatch { pos: () => THREE.Vector3 | null; secs: number; range?: number; deg?: number; until?: () => boolean }

const _c = new THREE.Vector3(), _f = new THREE.Vector3(), _t = new THREE.Vector3(), _n = new THREE.Vector3();
/** The camera faces a point: within deg of the middle of the view, on the screen, within range. */
export function facing(cam: THREE.Camera, at: THREE.Vector3, range = 300, deg = 30) {
  cam.getWorldPosition(_c); cam.getWorldDirection(_f);
  _t.copy(at).sub(_c);
  const d = _t.length();
  if (d > range) return false;
  if (d < 0.05) return true;
  if (_f.angleTo(_t) > THREE.MathUtils.degToRad(deg)) return false;
  _n.copy(at).project(cam);
  return _n.z < 1 && Math.abs(_n.x) <= 1 && Math.abs(_n.y) <= 1;
}

const hhmm = (c: number) => { const d = ((c % 86400) + 86400) % 86400; return `${String(Math.floor(d / 3600)).padStart(2, '0')}:${String(Math.floor(d % 3600 / 60)).padStart(2, '0')}`; };

export class SeenLog {
  /** At most this many are kept (the oldest go first): a night has a few dozen. */
  static MAX = 300;
  /** The night so far, oldest first (the same array for the whole game: S47.seen). */
  readonly entries: SeenEntry[] = [];
  private watches: { e: SeenEntry; w: SeenWatch; t: number }[] = [];

  add(e: SeenEntry, w?: SeenWatch | null) {
    this.entries.push(e);
    if (this.entries.length > SeenLog.MAX) this.entries.splice(0, this.entries.length - SeenLog.MAX);
    if (w) this.watches.push({ e, w, t: 0 });
    return e;
  }
  /** The last one written of a kind (and which), or null. */
  last(kind: SeenKind, what?: string) {
    for (let i = this.entries.length - 1; i >= 0; i--) { const e = this.entries[i]; if (e.kind === kind && (what === undefined || e.what === what)) return e; }
    return null;
  }
  /** Is a thing still being watched (until its watch ends)? */
  watching(e: SeenEntry) { return this.watches.some((x) => x.e === e); }
  stop(e: SeenEntry) { this.watches = this.watches.filter((x) => x.e !== e); }

  /** Each frame, before the night's things move or go: the camera on what is being watched. */
  update(dt: number, cam: THREE.Camera) {
    if (!this.watches.length) return;
    this.watches = this.watches.filter((x) => {
      const p = x.w.pos();
      if (x.w.until ? x.w.until() : !p) return false;
      if (p && !x.e.looking && facing(cam, p, x.w.range, x.w.deg)) x.e.looking = true;
      return (x.t += dt) < x.w.secs;
    });
  }
  /** A save opened (its night as it was), or a new night (nothing). */
  restore(list?: SeenEntry[] | null) {
    this.watches = [];
    this.entries.length = 0;
    for (const e of list ?? []) { const c: SeenEntry = { ...e }; if (e.at) c.at = [e.at[0], e.at[1]]; else delete c.at; this.entries.push(c); }
  }
  /** What is kept with the case. */
  get data(): SeenEntry[] { return this.entries.map((e) => ({ ...e })); }
  /** The last few for the ?debug overlay, one short line each (fits a phone held upright). */
  text(n = 8) {
    const all = this.entries, out = [`seen ${all.length}${all.length > n ? `, last ${n}` : ''}`];
    for (const e of all.slice(-n)) out.push(`${hhmm(e.clock)} ${e.kind.padEnd(9)} ${e.what.slice(0, 12).padEnd(12)} ${e.line ?? '-'} ${e.looking === null ? '-' : e.looking ? 'looked' : 'missed'}${e.end ? ' ' + e.end : ''}`);
    return out.join('\n');
  }
}
