import { RuleBook, pair, type BookNote, type BookRule } from './boardRules';
import type { CaseBoard } from './caseBoard';
import { clockText } from './time';

// A thread you do not remember (FORSLAG.md, 20 October, small change 8; Tom: "Spenning og skummelt
// slår faste regler"). After each time away (story/abduction.ts; taken in the truck too,
// story/truckTaken.ts, which wakes the same way), the case board has one more red thread than the
// player tied: the same yarn, between two cards they have, and on it a scrap of index card with one
// word in pencil, in their own hand: "same". The waking already has "a line in the notebook in my
// own hand. I do not remember writing it" (abduction.ts); this is the board's.
//
// Which two: one end is the exposure the player did not make (the photograph from above, `awayN`),
// the other the first of these the board has, never used before tonight and never a pin a question
// still open rests on (it would read as a hint), never a question card:
//   his last field card, "Do not connect a fresh observer to a run merely to finish an old one."
//   her correction, "Tomás Vega was present in the active reference when I broke the connection."
//   S-03's motor bus, "Encoder: 026 degrees."
//   the film that kept two, FRAME 01's and FRAME 02's second pale mark
//   the printout, "-39 LY"
//   T. Vega's field book
//   the time away before this one (its photograph)
//   and, before any of them have pins, the printout or the frames as cards
// One per time away (at most three in a night). Kept with the case (`same:N`, `same:seen`). The first
// time the board is laid out with one on it, the board opens on it and says, in Reyes's words, "I
// don't remember drawing that.", and the notebook has it.

export const SAME_WORD = 'same';
export const SAME_SAY = 'I don\'t remember drawing that.';
const KEY = 'same:', SEEN = 'same:seen';
/** The other end, in the order it is looked for. */
export const SAME_ENDS = ['e11.fresh', 'e12.cut', 's03log.heading', 'frame02.extra', 'frame01.extra', 'rx.dist', 'fb', '@away', 'printout', 'frame02', 'frame01'];

export interface SameDeps {
  seen: Set<string>;
  board: () => CaseBoard;
  /** The night's clock. */
  clock: () => number;
  note: (text: string) => void;
  saw: () => void;
  save: () => void;
}

export class SameThread {
  /** The threads tonight: the time away it came with, and its two pins. */
  threads: { n: number; a: string; b: string }[] = [];
  /** The times away seen so far (the next one gets its thread). */
  private upTo = 0;
  private readonly rules: BookRule[] = [];
  private readonly notes: Record<string, BookNote> = {};
  private readonly links: string[] = [];
  readonly book = new RuleBook({
    key: 'same', owns: () => false,
    notes: this.notes, rules: this.rules, questions: [],
    links: () => this.links,
    wrong: () => {}, save: () => this.d.save(), cards: () => [],
  });
  constructor(private d: SameDeps) {}

  /** Noticed on the board. */
  get noticed() { return this.d.seen.has(SEEN); }

  /** Every step: the count of times away tonight (story/abduction.ts, data.count). A new one, once
   *  its photograph is on the board, gets its thread. */
  update(count: number) {
    while (this.upTo < count) {
      this.upTo++;
      if (this.d.seen.has(`${KEY}${this.upTo}`) || [...this.d.seen].some((k) => k.startsWith(`${KEY}${this.upTo}=`))) continue;
      this.tie(this.upTo);
    }
  }

  /** Ties the thread for the time away n, if the board has two ends for it (else none, for good). */
  private tie(n: number) {
    const board = this.d.board(), pins = board.pins();
    const has = (p: string) => pins.some((x) => x.pin === p);
    const used = new Set(this.threads.flatMap((t) => [t.a, t.b]));
    const a = has(`away${n}`) ? `away${n}` : null;
    let b: string | null = null;
    for (const want of SAME_ENDS) {
      const p = want === '@away' ? [...Array(n - 1).keys()].map((i) => `away${n - 1 - i}`).find((x) => has(x) && !used.has(x)) ?? null : want;
      if (!p || !has(p) || used.has(p) || p === a || board.rests(p)) continue;
      b = p; break;
    }
    if (!a || !b) { this.d.seen.add(`${KEY}${n}`); this.d.save(); return; }
    this.d.seen.add(`${KEY}${n}=${a}|${b}`);
    this.add(n, a, b);
    this.d.save();
  }
  private add(n: number, a: string, b: string) {
    const id = `same${n}`;
    this.notes[id] = { text: SAME_WORD, hand: true };
    this.rules.push({ a, b, note: id, text: 'Those two are already connected.' });
    this.links.push(pair(a, b));
    this.threads.push({ n, a, b });
  }

  /** The board has been laid out: the first time with a thread on it, it opens on the thread. */
  opened(view: { frame: (ids: string[]) => boolean; say: (t: string) => void; el: HTMLElement }) {
    if (this.noticed || !this.threads.length) return;
    const t = this.threads[0];
    if (!view.frame([t.a, t.b, `same${t.n}`])) return;
    this.d.seen.add(SEEN);
    setTimeout(() => { if (view.el.isConnected) view.say(SAME_SAY); }, 1100);
    const cards = this.d.board().pins();
    const name = (p: string) => cards.find((c) => c.pin === p)?.card ?? p;
    this.d.note(`${clockText(this.d.clock(), false)}. On the case board, a thread I did not tie: from the photograph I did not take to ${this.endName(name(t.b))}. On it, in pencil, in my own hand: "${SAME_WORD}". ${SAME_SAY}`);
    this.d.saw();
    this.d.save();
  }
  private endName(card: string) {
    if (card.startsWith('away')) return 'the one from the time before';
    return ({ e11: 'his last field card', e12: 'her correction', s03log: 'S-03\'s motor bus, 026', frame01: 'FRAME 01', frame02: 'FRAME 02', rx: 'the printout, -39 LY', printout: 'the printout, -39 LY', fb: 'T. Vega\'s field book' } as Record<string, string>)[card] ?? 'another card';
  }

  /** A night begun or a case opened: the threads as the case has them; the times away it has had
   *  have had theirs (an older case's are not tied now). */
  reset(count: number) {
    this.threads = []; this.rules.length = 0; this.links.length = 0;
    for (const k of Object.keys(this.notes)) delete this.notes[k];
    for (const k of this.d.seen) {
      const m = /^same:(\d+)=([^|]+)\|(.+)$/.exec(k);
      if (m) this.add(+m[1], m[2], m[3]);
    }
    this.threads.sort((x, y) => x.n - y.n);
    this.upTo = count;
  }
}
