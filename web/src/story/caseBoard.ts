import type { DocSpec, UI } from '../ui/UI';
import { board, type BoardCard, type BoardRegion } from '../ui/Board';
import { RuleBook, SAME, EMPTY } from './boardRules';

// The one case board of the night (ui/Board.ts draws it). Every chapter that has begun
// brings its book (story/boardRules.ts): its records with their lines, its questions and
// its rules. Every other paper and photograph in the journal lies on the board as a plain
// card, in the patch of cork of the chapter it belongs to, so the board gathers the whole
// journal. Where the player put the cards is kept with the case (`SavedCase.board`).

/** The board's part of the saved case: where the cards lie (board coordinates). */
export interface BoardSave { pos: Record<string, [number, number]> }

// Six patches of cork, one per part of the night, in reading order: three across, two down.
const RW = 1180, RH = 760, GAP = 60, M = 40, LABEL = 40;
const REGIONS = [
  ['ns', 'NIGHT SHIFT'], ['ch1', '1  THE SECOND EXPOSURE'], ['ch2', '2  THE AMENDED RECORD'],
  ['ch3', '3  THE SURVEY STATION'], ['ch4', '4  ROOM 6'], ['ch5', '5  ALL NIGHT'],
] as const;
export type RegionId = typeof REGIONS[number][0];
const rect = (i: number) => ({ x: M + (i % 3) * (RW + GAP), y: M + LABEL + Math.floor(i / 3) * (RH + GAP + LABEL), w: RW, h: RH });
// The board grows with the night: as wide as the patches reached so far, the second row
// from chapter three on, so there is no bare cork for what has not happened yet.
const sizeAt = (now: number) => {
  const cols = Math.min(3, now + 1), rows = now >= 3 ? 2 : 1;
  return { w: 2 * M + cols * RW + (cols - 1) * GAP, h: 2 * M + rows * (RH + LABEL) + (rows - 1) * GAP };
};

// Which part of the night a paper belongs to when no chapter lays it out itself. Anything
// not named here goes to the part of the night it first turns up in.
const GROUP: Record<string, RegionId> = {
  log: 'ns', workorder: 'ns', printout: 'ns', binder: 'ns', service: 'ns', rfiCard: 'ns', halley: 'ns', telex: 'ns',
  s03log: 'ch1', refsheet: 'ch1', frame01: 'ch1', frame02: 'ch1', observation: 'ch1', finding: 'ch1',
  access: 'ch2', casemap: 'ch2', s1diagram: 'ch3', e14: 'ch5', e15: 'ch5',
};

export interface CaseBoardHost {
  ui: UI;
  docs: () => DocSpec[];
  books: () => RuleBook[];          // the chapters that have begun, in the order of the night
  region: () => RegionId;           // the part of the night the player is in
  layout: () => BoardSave;
  save: () => void;                 // the case, after cards were moved
}

// A paper or photograph from the journal, as a card: the stamp, a title, the picture if it
// has one, a line about what it is, and its whole text behind TEXT.
function docCard(d: DocSpec, region: string): BoardCard {
  // the stamp on the paper if it is short, else the part of the title before the slash
  const parts = d.title.split(' / ');
  const short = d.stamp && d.stamp.length <= 10 ? d.stamp : parts.length > 1 ? parts[0] : '';
  const title = (parts.length > 1 ? parts.slice(1).join(' / ') : d.title).toUpperCase();
  const first = (d.transcript ?? '').split('\n')[0].replace(/\s+/g, ' ').trim();
  const line = first.length > 120 ? first.slice(0, 117).replace(/[\s.,;:]+\S*$/, '') + '...' : first;
  const kind = d.kind === 'photo' ? 'photo' : d.kind === 'map' ? 'map' : d.id === 'finding' ? 'finding' : 'paper';
  const page = d.kind === 'photo' || d.kind === 'map' ? '' : (d.sheets ? d.sheets.map((s) => s.page).join('\n\n') : d.page ?? '').replace(/^~ /gm, '');
  return {
    id: d.id, doc: d.id, stamp: short, title: title === short ? '' : title, kind, have: true, region,
    image: d.image, text: line, readingText: (d.transcript ?? '').split(/\n\s*\n/)[0], full: [page, d.transcript].filter(Boolean).join('\n\n'),
  };
}

export class CaseBoard {
  view: ReturnType<typeof board> | null = null;
  private seen: Record<string, RegionId> = {};   // where a paper nobody claims first turned up
  private cardPins = new Map<string, string>();  // older whole-card references now landing on a line pin

  constructor(private h: CaseBoardHost) {}

  private books() { return this.h.books(); }
  private cards(): BoardCard[] {
    const out: BoardCard[] = [];
    for (const b of this.books()) for (const c of b.cards()) out.push({ ...c, region: c.region ?? b.key });
    const claimed = new Set(out.flatMap((c) => [c.id, c.doc ?? '']));
    for (const b of this.books()) for (const q of b.o.questions) if (q.doc) claimed.add(q.doc);
    for (const d of this.h.docs()) {
      // a finding is its question's card; a paper a chapter lays out itself is on its card
      if (/^p\d\d$/.test(d.id) || claimed.has(d.id)) continue;
      const region = GROUP[d.id] ?? (this.seen[d.id] ??= this.h.region());
      out.push(docCard(d, region));
    }
    this.cardPins.clear();
    for (const c of out) if (c.have && c.lines?.length) this.cardPins.set(c.id, c.lines[0].id);
    return out;
  }
  private cardOf(node: string, cards: BoardCard[]) {
    return cards.find((c) => c.id === node || c.lines?.some((l) => l.id === node))?.id ?? node;
  }

  /** Opens the board, at the part of the night `region` (the one the player is in when not
   *  given). `place` names where it is laid out, when that is a table in the world. */
  open(place?: string, region?: RegionId) {
    const ui = this.h.ui;
    for (const b of this.books()) b.backfill();
    const save = this.h.layout();
    let moved = false;
    const now = Math.max(0, REGIONS.findIndex((r) => r[0] === this.h.region()));
    this.view = board(ui, {
      title: place ? `CASE BOARD / ${place}` : 'CASE BOARD',
      size: sizeAt(now),
      status: () => {
        const qs = this.books().flatMap((b) => b.questions());
        const open = qs.filter((q) => !q.done).map((q) => q.stamp), done = qs.filter((q) => q.done).length;
        return `${open.length ? `OPEN: ${open.join(', ')}` : 'NOTHING OPEN'}${done ? `  /  RECORDED: ${done}` : ''}`;
      },
      // every patch of cork has its place; the ones the night has not reached have no label
      regions: (): BoardRegion[] => REGIONS.map(([id, label], i) => ({ id, label: i <= now ? label : '', ...rect(i) })),
      focus: region ?? this.h.region(),
      cards: () => this.cards(),
      questions: () => this.books().flatMap((b) => b.questions().map((q) => ({ ...q, region: b.key }))),
      notes: () => this.books().flatMap((b) => b.notes().map((n) => ({ ...n, region: b.key }))),
      // P10, for example, still points at the control print as a whole. A chapter can
      // give that print line pins without losing the older chapter's automatic thread.
      threads: () => this.books().flatMap((b) => b.threads()).map((t) => ({
        ...t, a: this.cardPins.get(t.a) ?? t.a, b: this.cardPins.get(t.b) ?? t.b,
      })),
      connect: (a, b) => {
        if (a === b) return { ok: false, text: 'Pull the thread to another piece of evidence.' };
        const cards = this.cards();
        const same = this.cardOf(a, cards) === this.cardOf(b, cards);
        // the chapter whose rule it is answers; the latest chapter first, since its rules
        // reach back to the earlier records
        for (const book of [...this.books()].reverse()) {
          const r = book.connect(a, b, same);
          if (r) return r;
        }
        return { ok: false, text: same ? SAME : EMPTY };
      },
      record: (q) => {
        for (const b of this.books()) { const r = b.record(q); if (r) return r; }
        return { ok: false, text: 'Nothing to record.' };
      },
      hint: (q) => { for (const b of this.books()) { const h = b.hint(q); if (h) return h; } return ''; },
      layout: {
        get: (id) => save.pos[id],
        set: (id, x, y) => { save.pos[id] = [Math.round(x), Math.round(y)]; moved = true; },
      },
      onClose: () => { this.view = null; if (moved) this.h.save(); },
    });
  }

  /** Draws the board again if it is open (a chapter's state changed under it). */
  refresh() { if (this.view?.el.isConnected) this.view.render(); }
}
