import * as THREE from 'three';
import type { BoardCard } from '../ui/Board';
import { RuleBook, pair, type BookNote, type BookRule } from './boardRules';
import { clockText } from './time';

// No stars where the antennas point (FORSLAG.md, 20 October, "Valg for Tom" 6; Tom: "Spenning og
// skummelt slår faste regler"). From 02:15, when every dish has gone to az 026, el 32 with nobody
// driving it, a round patch of the sky there has no stars in it (world/Sky.ts draws it, STAR_HOLE:
// the air goes on through it, only what is beyond the air is gone, a shooting star with it). It is
// held to the dishes, not to the stars, it never grows, and it is still there at dawn until the
// stars go; the last picture of the pull-out at 05:29 has it too (story/Chapter6.ts). This bends
// SPILLDESIGN.md's "the world is honest" (R1) in one place, in the sky, and nowhere on the ground.
//
// A found thing, not an event: the night's curve does not hold it (story/curve.ts), and nothing
// happens when it is found. But it keeps the one-at-a-time rule (main.ts oneAtATime): it is not
// noticed while another of the night's things is going.
//   - Noticed, once in the night: on foot, out under the sky, at night, after it has come in, with
//     the middle of the view within NOTICE.deg of it for NOTICE.secs (a closer look, the zoom, for
//     NOTICE.zoomSecs) and nothing solid in the way. Reyes says it in a toast and writes it in the
//     notebook; the night's log has it (story/seen.ts: 'scare', 'no stars'); a card on the board.
//   - A free frame with it in it (core/album.ts, story/albumSubjects.ts) is captioned with
//     HOLE_PRINT and keeps where it is on the print (AlbumShot.targets.starhole); the card on the
//     board shows the first such print. Taken before it was noticed, the frame notices it.
//   - A closer look at it (the zoom) says what there is to see (examine).
// Kept with the town's part of the case: 'sky:hole', 'sky:hole@<clock>', 'sky:hole/<chapter>' (the
// patch of cork it goes on), 'sky:hole:print@<shot id>', the board's threads as 'sh:<a|b>'. None of
// it comes again after a load.

/** The look that notices it: degrees off the middle of the view, seconds (and with the zoom). */
export const NOTICE = { deg: 4, secs: 3, zoomDeg: 2.5, zoomSecs: 1.2 } as const;
/** Under a free frame with it in it (core/album.ts). */
export const HOLE_PRINT = 'No stars where the dishes point.';
export const HOLE_TOAST = 'There are no stars where the dishes point.';
const HOLE_LOOK = 'No stars in it. Not one, and the sky behind it is the same colour as the sky round it.';
const KEY = 'sky:hole', LINK = 'sh:';
const CHAPTERS = ['ch1', 'ch2', 'ch3', 'ch4', 'ch5'];

export interface StarHoleDeps {
  seen: Set<string>;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  clock: () => number;
  chapter: () => string;
  /** Write it in the night's log (story/seen.ts): seen, where the player is. */
  saw: () => void;
  /** The prints of the night (core/album.ts): the url of one by its id. */
  print: (id: string) => string | null;
  save: () => void;
  refresh: () => void;
}
export interface StarHoleIn {
  /** In play, no panel open, not in a cinematic, on foot. */
  ok: boolean;
  /** Nothing else of the night's is going (main.ts oneAtATime), no call on the line. */
  free: boolean;
  /** Out under the sky. */
  outdoors: boolean;
  /** The dawn has hardly begun. */
  night: boolean;
  /** How much of it there is now (Sky.hole, 0 to 1). */
  hole: number;
  /** Where it is from here, in world terms (the sky's direction turned with the area's sky). */
  dir: THREE.Vector3;
  cam: THREE.Camera;
  /** The closer look (the zoom) is most of the way in. */
  zoom: boolean;
  /** Nothing solid between the eye and the sky along dir. */
  clear: () => boolean;
}

// ---------- the board ----------
const NOTES: Record<string, BookNote> = {
  'n.hole.s03': { text: 'S-03 has read 026 since the turn, with no command. All twenty-seven are pointed at the one place in the sky with no stars in it.' },
  'n.hole.cows': { text: 'The herd stands facing 026 with their heads up. The way the dishes point.' },
};
const RULES: BookRule[] = [
  { a: 'hole.sky', b: 's03log.heading', note: 'n.hole.s03', text: NOTES['n.hole.s03'].text },
  // tonight's herd, from T. Vega's field book when it is on the board (story/fieldBook.ts)
  { a: 'hole.sky', b: 'tn.cows', note: 'n.hole.cows', text: NOTES['n.hole.cows'].text },
];

const _f = new THREE.Vector3();

export class StarHole {
  readonly book: RuleBook;
  /** Seconds the eyes have been on it (the tests). */
  look = 0;
  /** Tests that are about other things: never noticed. */
  off = false;
  private links: string[] = [];

  constructor(private d: StarHoleDeps) {
    const self = this;
    this.book = new RuleBook({
      // (its card and its notes lie in the patch of cork of the part of the night it was noticed in)
      get key() { return self.region; },
      owns: (n) => n === 'hole' || n.startsWith('hole.'),
      notes: NOTES, rules: RULES, questions: [],
      links: () => this.links,
      wrong: () => {},
      save: () => { this.keepLinks(); this.d.save(); },
      cards: () => this.cards(),
      empty: 'Not that one. What else tonight is on 026?',
    });
  }

  get noticed() { return this.d.seen.has(KEY); }
  /** When it was noticed (seconds since midnight), or null. */
  get when() { for (const k of this.d.seen) if (k.startsWith(`${KEY}@`)) return Number(k.slice(KEY.length + 1)); return null; }
  /** The first free frame that has it, by its id, or null. */
  get printId() { for (const k of this.d.seen) if (k.startsWith(`${KEY}:print@`)) return k.slice(KEY.length + 7); return null; }
  private get region() {
    for (const k of this.d.seen) if (k.startsWith(`${KEY}/`)) { const c = k.slice(KEY.length + 1); if (CHAPTERS.includes(c)) return c; }
    return 'ch1';
  }
  /** Its card on the board, once noticed. */
  books() { return this.noticed ? [this.book] : []; }

  /** Each frame: the eyes on it, long enough, notice it. */
  update(dt: number, s: StarHoleIn) {
    if (this.noticed || this.off || !s.ok || !s.free || !s.outdoors || !s.night || s.hole < 0.9) { this.look = 0; return; }
    const off = THREE.MathUtils.radToDeg(s.cam.getWorldDirection(_f).angleTo(s.dir));
    const zoom = s.zoom && off < NOTICE.zoomDeg;
    // (a moment's slip off it does not start the look over)
    this.look = zoom || off < NOTICE.deg ? this.look + dt * (zoom ? NOTICE.secs / NOTICE.zoomSecs : 1) : Math.max(0, this.look - dt * 2);
    if (this.look < NOTICE.secs) return;
    if (!s.clear()) { this.look = NOTICE.secs * 0.5; return; }
    this.notice(false);
  }

  /** What a closer look (the zoom) at it says, or null when it is not looked at. */
  examine(s: StarHoleIn): string | null {
    if (!s.outdoors || !s.night || s.hole < 0.5) return null;
    const off = THREE.MathUtils.radToDeg(s.cam.getWorldDirection(_f).angleTo(s.dir));
    return off < NOTICE.zoomDeg && s.clear() ? HOLE_LOOK : null;
  }

  /** A free frame was taken with it in it (core/album.ts, AlbumShot.targets.starhole): kept for its
   *  card, and it is noticed now if nothing else of the night's is going (free). */
  photographed(shotId: string, free: boolean) {
    if (!this.printId) this.d.seen.add(`${KEY}:print@${shotId}`);
    if (!this.noticed && !this.off && free) this.notice(true);
    else { this.d.refresh(); this.d.save(); }
  }

  private notice(viaPrint: boolean) {
    const seen = this.d.seen, clock = this.d.clock(), t = clockText(clock, false);
    seen.add(KEY);
    seen.add(`${KEY}@${Math.round(clock)}`);
    const ch = this.d.chapter();
    if (CHAPTERS.includes(ch)) seen.add(`${KEY}/${ch}`);
    this.look = 0;
    this.d.toast(viaPrint ? `In the viewfinder, up where the dishes point: no stars.` : HOLE_TOAST, 5);
    this.d.note(`${t}. There are no stars where the dishes point. Az 026, el 32: a round patch about eight degrees across, nearly a fist at arm's length, with nothing in it. Not a cloud. The sky is the same colour all the way through it. Only the stars are gone, as if something were standing in front of them.`);
    this.d.saw();
    this.d.refresh();
    this.d.save();
  }

  // ---------- the board ----------
  private cards(): BoardCard[] {
    if (!this.noticed) return [];
    const when = this.when, id = this.printId, url = id ? this.d.print(id) : null;
    return [{
      id: 'hole', stamp: 'AZ 026 EL 32', title: 'NO STARS AT 026', kind: url ? 'photo' : 'thing', have: true, w: url ? 260 : 300,
      image: url ?? undefined,
      text: `${when !== null ? `${clockText(when, false)}, looking` : 'Looking'} up along the dishes: a round patch of sky with no stars in it.`,
      lines: [{ id: 'hole.sky', text: 'Az 026, el 32. Eight degrees across, no stars, the sky the same colour through it.' }],
    }];
  }
  private keepLinks() {
    for (const k of [...this.d.seen]) if (k.startsWith(LINK)) this.d.seen.delete(k);
    for (const k of this.links) this.d.seen.add(LINK + k);
  }

  // ---------- a night opened ----------
  /** A new night or a case opened: as the case has it; its threads. */
  reset() {
    this.look = 0;
    this.links = [...this.d.seen].filter((k) => k.startsWith(LINK)).map((k) => k.slice(LINK.length)).filter((k) => RULES.some((r) => pair(r.a, r.b) === k));
  }
  /** The developer menu, the tests: not noticed tonight (again). */
  forget() {
    for (const k of [...this.d.seen]) if (k === KEY || k.startsWith(`${KEY}@`) || k.startsWith(`${KEY}/`) || k.startsWith(`${KEY}:`) || k.startsWith(LINK)) this.d.seen.delete(k);
    this.reset();
  }
}
