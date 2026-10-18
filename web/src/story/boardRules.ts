import type { BoardCard, BoardQuestion, BoardNote, BoardThread } from '../ui/Board';

// The rules of the case board, one book per chapter. A chapter writes down, as data, what
// it puts on the board (cards, and the lines on them that matter), its questions, and which
// threads hold: a rule ties two pins, and what they show together is a note. A note that
// belongs to a question goes into its slots by itself; when they are all filled, RECORD
// registers the finding with the chapter's own function (the one its old panel called), so
// the story moves on exactly as before. A rule marked `wrong` contradicts: the thread falls
// slack, the reply says why, and it counts against the question like a wrong answer did.
// Two of the chapter's pins without a rule say nothing about each other; that counts for
// nothing, except that it brings the fourth hint closer. After SPILLDESIGN.md, "Bevisbordet".

export type Result = { ok: boolean; text: string };
export interface BookNote { text: string; q?: string }
export interface BookRule {
  a: string; b: string; note?: string; text: string; wrong?: string;
  /** Alternative working hypotheses: only the last thread in this group stays. */
  group?: string;
  /** Whether this rule restores a recorded finding from a save without threads. */
  restore?: () => boolean;
}
export interface BookQuestion {
  id: string; stamp: string; text: string;
  doc?: string;                         // the journal finding this question stands for
  /** how many notes it needs; 0 for a finding made somewhere else (at the lamp, in a talk) */
  slots: number;
  hints: string[];
  open: () => boolean;
  done: () => string | null;
  record?: () => Result;
  /** what a finding made somewhere else says while it is still open */
  elsewhere?: string;
  x: number; y: number; w?: number;
}
export interface BookSpec {
  key: string;                          // the board region: 'ch2', 'ch3'...
  owns: (node: string) => boolean;      // the pins this chapter put on the board
  notes: Record<string, BookNote>;
  rules: BookRule[];
  questions: BookQuestion[];
  links: () => string[];                // where the chapter keeps its threads ('a|b', sorted)
  wrong: (q: string, rule: BookRule) => Result | void;  // the chapter can supply its original reply
  save: () => void;
  cards: () => BoardCard[];             // the chapter's records, with their lines
  /** threads the story itself draws: what a finding made elsewhere rests on */
  auto?: () => { a: string; b: string }[];
  /** which question a miss belongs to, for the hints (default: the first open one) */
  questionOf?: (a: string, b: string) => string;
  empty?: string;                       // the reply for two of its pins that say nothing
  /** links kept by an older version of the board, to be dropped */
  legacy?: (k: string) => boolean;
}

/** What a chapter needs of the board: to lay it out, and to draw it again when its state
 *  changes while it is open. `place` names the table it is laid out on. */
export interface BoardHook { open(place?: string, region?: string): void; refresh(): void }

export const pair = (a: string, b: string) => [a, b].sort().join('|');
export const SAME = 'Both lines are on the same record. Compare it with another one.';
export const EMPTY = 'Those two do not say anything about each other.';

export class RuleBook {
  private hintSeen: Record<string, number> = {};
  private missSince: Record<string, number> = {};
  constructor(readonly o: BookSpec) {}

  get key() { return this.o.key; }
  ruleOf(k: string) { return this.o.rules.find((r) => pair(r.a, r.b) === k); }
  question(id: string) { return this.o.questions.find((q) => q.id === id); }
  private questionOf(a: string, b: string) {
    if (this.o.questionOf) return this.o.questionOf(a, b);
    const r = this.ruleOf(pair(a, b));
    const q = r?.wrong ?? (r?.note ? this.o.notes[r.note]?.q : undefined);
    return q ?? this.o.questions.find((x) => x.open() && !x.done())?.id ?? '';
  }
  private miss(a: string, b: string, r: Result) {
    const q = this.questionOf(a, b);
    if (q) this.missSince[q] = (this.missSince[q] ?? 0) + 1;
    return r;
  }

  /** A thread from a to b, if it is this chapter's business; null when it is not. */
  connect(a: string, b: string, sameCard: boolean): Result | null {
    const k = pair(a, b);
    const r = this.ruleOf(k);
    if (!r && !(this.o.owns(a) && this.o.owns(b))) return null;
    if (sameCard) return this.miss(a, b, { ok: false, text: SAME });
    const links = this.o.links();
    if (links.includes(k)) return { ok: true, text: 'Those two are already connected.' };
    if (!r) return this.miss(a, b, { ok: false, text: this.o.empty ?? EMPTY });
    if (r.wrong) {
      const reply = this.o.wrong(r.wrong, r);
      this.o.save();
      if (reply?.ok) return reply;
      return this.miss(a, b, reply ?? { ok: false, text: r.text });
    }
    const q = r.note ? this.o.notes[r.note]?.q : undefined;
    if (r.group) {
      const done = q ? this.question(q)?.done() : null;
      if (done) return { ok: true, text: done };
      for (let i = links.length - 1; i >= 0; i--) {
        if (this.ruleOf(links[i])?.group === r.group) links.splice(i, 1);
      }
    }
    links.push(k);
    this.o.save();
    if (!q) return { ok: true, text: r.text };
    const filled = this.filled(q).length, total = this.question(q)?.slots ?? filled;
    return { ok: true, text: filled >= total ? `${r.text} ${q} has what it needs: RECORD it.` : `${r.text} (${q}: ${filled} of ${total})` };
  }

  /** The notes in question q's slots, in the order their threads were tied (one per note). */
  filled(q: string): string[] {
    const ids: string[] = [];
    for (const k of this.o.links()) {
      const n = this.ruleOf(k)?.note;
      if (n && this.o.notes[n]?.q === q && !ids.includes(n)) ids.push(n);
    }
    return ids.map((id) => this.o.notes[id].text);
  }

  threads(): BoardThread[] {
    const out: BoardThread[] = [];
    for (const k of this.o.links()) {
      const r = this.ruleOf(k);
      if (r) out.push({ a: r.a, b: r.b, thin: !r.note });
    }
    for (const t of this.o.auto?.() ?? []) out.push({ ...t, thin: true, auto: true });
    return out;
  }
  /** Notes that answer no question stay on the board, between the pins they came from. */
  notes(): BoardNote[] {
    const out: BoardNote[] = [];
    for (const k of this.o.links()) {
      const r = this.ruleOf(k);
      if (!r?.note || this.o.notes[r.note]?.q || out.some((n) => n.id === r.note)) continue;
      out.push({ id: r.note, text: this.o.notes[r.note].text, from: [r.a, r.b] });
    }
    return out;
  }
  questions(): BoardQuestion[] {
    return this.o.questions.filter((q) => q.open()).map((q) => ({
      id: q.id, stamp: q.stamp, text: q.text, slots: q.slots, filled: this.filled(q.id), done: q.done(),
      elsewhere: q.elsewhere, x: q.x, y: q.y, w: q.w,
    }));
  }

  record(id: string): Result | null {
    const q = this.question(id);
    if (!q) return null;
    if (!q.record) return { ok: false, text: q.elsewhere ?? 'This one is not recorded on the board.' };
    if (this.filled(id).length < q.slots) return { ok: false, text: `${id} needs ${q.slots - this.filled(id).length} more on it first.` };
    return q.record();
  }

  // Three hint levels per question; a fourth, if there is one, names the threads and only
  // comes after the third has been read and two more threads have missed.
  hint(id: string): string | null {
    const q = this.question(id);
    if (!q) return null;
    const list = q.hints;
    const n = this.hintSeen[id] ?? 0;
    if (n >= 3 && (this.missSince[id] ?? 0) < 2) return list[Math.min(2, list.length - 1)];
    this.hintSeen[id] = Math.min(n + 1, list.length);
    if (this.hintSeen[id] === 3) this.missSince[id] = 0;
    return list[Math.min(n, list.length - 1)];
  }

  // Saves from before the board recorded findings without threads (or with the first
  // board's threads): lay the threads that fill each recorded question out again.
  backfill() {
    const links = this.o.links();
    if (this.o.legacy) for (let i = links.length - 1; i >= 0; i--) if (this.o.legacy(links[i])) links.splice(i, 1);
    for (const q of this.o.questions) {
      if (!q.slots || !q.done()) continue;
      const have = new Set(links.map((k) => this.ruleOf(k)?.note).filter(Boolean));
      for (const [id, n] of Object.entries(this.o.notes)) {
        if (n.q !== q.id || have.has(id)) continue;
        const r = this.o.rules.find((x) => x.note === id && !x.wrong && (x.restore?.() ?? true));
        if (r) { links.push(pair(r.a, r.b)); have.add(id); }
      }
    }
  }

  cards() { return this.o.cards(); }
  owns(node: string) { return this.o.owns(node); }
}
