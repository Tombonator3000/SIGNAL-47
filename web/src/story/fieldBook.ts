import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { BoardCard } from '../ui/Board';
import { RuleBook, type BookNote, type BookRule } from './boardRules';
import { clockText } from './time';
import { fieldBookView, envelopeView, envelopeFront, envelopeLit, type BookPage } from '../ui/fieldBookView';

// T. Vega's third field book, in the drawer under the bench in the hut at STATION 01, and the
// envelope for plate 4 on the counter of the motel office (FORSLAG.md, 20 October, item 3 and
// small tweak 3). Tomás and Nora were the survey crew in 1947 (HISTORIE.md: he stood on C with
// the last reading when she cut the cable, and was not there afterwards). This is his book from
// June and July that year: the survey, and then, in July, the things that happened to them on
// the roads round the station. They are the things that happen tonight (HENDELSER.md, events 2,
// 7, 8, 12 and 14), in the same words, and where the player has already seen one, at the same
// minute: 1947 took in this night (HISTORIE.md, "Avtrykket går begge veier over 39 år"). The
// book is read once and stays as it was read (R4). What it has that the player has not seen yet,
// comes on the way back (the herd turned for the truck, the knocks on the cab's roof, the lights
// along B northbound too), each in its own director, one thing at a time as always.
// The last page: "Three on plate 4. There were two of us." The envelope from the photo shop in
// Roswell came back for him after he was gone; nobody opened it. Held up to the lamp: the
// transit, and three small figures by it, the third not as sharp.
// On the case board each thing in the book has its card from tonight, tied to the key the event
// already keeps (ev:, ne:, mark:); a thread between the two holds: "He wrote it in July 1947. I
// saw it tonight." Kept with chapter three's part of the case (Ch3State.fb) and, for the
// envelope, with the town's ('env:lamp').

export type FbEvent = 'lights' | 'cows' | 'cows2' | 'knocks' | 'lamps' | 'car';
const EVENTS: FbEvent[] = ['lights', 'cows', 'cows2', 'knocks', 'lamps', 'car'];
/** What the book keeps once it has been read: when, the minutes it has (the ones the player had
 *  seen by then), which of the rest have been seen since, and the threads on the board. */
export interface FbSave { at: number; times: Partial<Record<FbEvent, string>>; had: FbEvent[]; told: FbEvent[]; links: string[]; page?: number }
/** Seconds into a drive after the book before the knocks may come (the track out of the
 *  station, between the gate and the highway, on the way back). */
export const KNOCK_AFTER = 20;
/** Seconds with nothing else of the night's going on before them (the night director keeps its
 *  own events minutes apart; the book's knocks do not come on the heels of one of them). */
export const KNOCK_QUIET = 30;

export interface FieldBookDeps {
  ui: UI; audio: AudioSys;
  /** The town's part of the case (the night's events keep their keys there). */
  seen: Set<string>;
  notes: () => string[];
  note: (s: string) => void;
  toast: (text: string, secs?: number) => void;
  clock: () => number;
  /** Game time: fn after s seconds of it (paused with the game). */
  after: (s: number, fn: () => void) => void;
  state: { get(): FbSave | undefined; set(v: FbSave): void };
  file: (doc: DocSpec) => void;
  save: () => void;
  refresh: () => void;
  /** The drawer in the hut, out or in (Station01.setDrawer), if the station is built. */
  drawer: (open: boolean, now?: boolean) => void;
  /** The book has been read: what it has not seen yet may come on the way back (main.ts). */
  foretell: () => void;
}

// ---------- where tonight keeps each thing ----------
const KEY: Record<FbEvent, string> = { lights: 'ev:lights', cows: 'ev:cows1', cows2: 'ev:cows', knocks: 'ne:knocks1', lamps: 'ev:lamps', car: 'ev:car' };
const MARK: Partial<Record<FbEvent, string>> = { lights: 'lights', cows: 'cows', lamps: 'lamps', car: 'car' };
/** A few words of the player's own note for each (story/lineLights.ts, pasture.ts, nightEvents.ts, townEvents.ts). */
const NOTE: Record<FbEvent, string> = {
  lights: 'Then they turned off along B and went out', cows: 'Nine head', cows2: 'One missing. A ring of frost',
  knocks: 'Three knocks on the roof of the cab', lamps: 'the street lights out one by one', car: 'The deputy\'s car on the shoulder',
};
const DATE: Record<FbEvent, string> = { lights: 'July 1', cows: 'July 3', cows2: 'July 4', knocks: 'July 5', lamps: 'July 6', car: 'July 7' };
/** What he wrote that the player will know again, when it comes after the book (a toast). */
const QUOTE: Record<FbEvent, string> = {
  lights: 'Then they turned off along B and went out.', cows: 'Took a bearing on them: 026. Nine head.', cows2: 'Eight.',
  knocks: 'Three knocks on the roof of the car, slow, from above.', lamps: 'Back on in the same order.', car: 'Nobody in it.',
};
const WHAT: Record<FbEvent, string> = {
  lights: 'three lights along B', cows: 'the herd on 026, nine head', cows2: 'the herd, eight, and the ring of frost',
  knocks: 'three knocks on the roof of the car', lamps: 'the lamps on Main Street and the dogs, in the same order', car: 'the county car with nobody in it',
};

export const FB_TITLE = 'T. VEGA / FIELD BOOK 3 / JUNE-JULY 47';
const LAST = 'Three on plate 4.\nThere were two of us.';

/** The pages, with the minutes the player had seen by the time the book was read. */
export function fbPages(times: Partial<Record<FbEvent, string>>): BookPage[] {
  const t = (e: FbEvent) => times[e] ? `${times[e]}. ` : '';
  const pages = [
    'T. VEGA\nFIELD BOOK 3\nJUNE - JULY 47\n\nSTA 01.  A, B, C.\n\nIf found, Sierra Motor Court,\nCaliche, N. Mex.',
    'Jun 9. Footing for A poured on the transit line. Backsight to B. N. on the rod.\n\nJun 14. C staked out east, forty stakes. Cable laid from the hut to the pier.\n\nJun 20. N.\'s first plate has a mark on it that should not be there. Told her it was a mistake.',
    `Jul 1. ${t('lights')}The highway north of the motel. Three lights, low, west of the road, keeping pace with the car. Then they turned off along B and went out.\n\nN. says ranch trucks. Ranch trucks have two lights.`,
    `Jul 3. ${t('cows')}The pasture south of the motel. The whole herd standing the same way. Took a bearing on them: 026. Nine head. Not one of them moved, with the headlights right on them.\n\nJul 4. ${t('cows2')}Eight. Where the ninth stood the grass is flat in a ring, white with frost. In July.`,
    `Jul 5. ${t('knocks')}Coming back up from Caliche, N. asleep against the door. Three knocks on the roof of the car, slow, from above. Nothing on the road behind.\n\nI did not stop. I did not wake her.`,
    `Jul 6. ${t('lamps')}Caliche. The lamps on Main Street out one by one, south to north, and the dogs barking in the same order. Back on in the same order.\n\nAs if something had walked up the street.`,
    `Jul 7. ${t('car')}The county car on the shoulder north of Caliche. Engine running, the door open, his hat on the seat. Nobody in it.\n\nWe waited. Nobody came back for it.`,
    'Jul --. Plate 4, off the camera on the pier. Held it up to the lamp in the hut.\n\nSent it in to Roswell for a print, care of the motel.\n\nN. will not look at it.',
    LAST,
  ];
  // the transcription: the same words, the lines run together within a paragraph (not on the cover
  // and the last page, which are set as they stand)
  return pages.map((p, i) => ({ hand: p, typed: i === 0 || i === pages.length - 1 ? p : p.replace(/(?<!\n)\n(?!\n)/g, ' '), last: i === pages.length - 1 }));
}

// ---------- the board ----------
const NOTES: Record<string, BookNote> = {
  'n.fb.lights': { text: 'Three lights along B. He wrote it in July 1947. I saw it tonight.' },
  'n.fb.cows': { text: 'The herd on 026. He wrote it in July 1947. I saw it tonight.' },
  'n.fb.knocks': { text: 'Three knocks on the roof. He wrote it in July 1947. I saw it tonight.' },
  'n.fb.lamps': { text: 'The lamps and the dogs, in the same order. He wrote it in July 1947. I saw it tonight.' },
  'n.fb.car': { text: 'The car on the shoulder, nobody in it. He wrote it in July 1947. I saw it tonight.' },
  'n.fb.plate': { text: 'Plate 4, against the lamp: three by the tripod. He wrote that there were two of them.' },
};
const RULES: BookRule[] = [
  ...(['lights', 'cows', 'knocks', 'lamps', 'car'] as const).map((e) => ({ a: `fb.${e}`, b: `tn.${e}`, note: `n.fb.${e}`, text: NOTES[`n.fb.${e}`].text })),
  { a: 'fb.plate', b: 'env4', note: 'n.fb.plate', text: NOTES['n.fb.plate'].text },
];
type Card = 'lights' | 'cows' | 'knocks' | 'lamps' | 'car';
const TONIGHT: Record<Card, { title: string; fallback: string }> = {
  lights: { title: 'THREE LIGHTS ALONG B', fallback: 'Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out.' },
  cows: { title: 'THE HERD ON 026', fallback: 'The whole herd standing facing the same way, on 026. Not one of them moved, with the headlights right on them.' },
  knocks: { title: 'KNOCKS ON THE ROOF', fallback: 'Driving. Three knocks on the roof of the cab, slow, from above. Nothing on the road behind.' },
  lamps: { title: 'THE LAMPS IN CALICHE', fallback: 'Main Street, Caliche: the street lights out one by one, south to north, and the dogs barking in the same order. Back on in the same order.' },
  car: { title: 'THE EMPTY COUNTY CAR', fallback: 'The deputy\'s car on the shoulder of the highway north of Caliche: engine running, the door open, nobody in it.' },
};
// where they lie on chapter three's patch of cork, under the station's records
const AT: Record<string, [number, number]> = { fb: [14, 790], 'tn.lights': [370, 790], 'tn.cows': [580, 790], 'tn.knocks': [790, 790], 'tn.lamps': [370, 1020], 'tn.car': [580, 1020] };
/** A note as a card's line: the start of it, the whole behind TEXT (as caseBoard.docCard does). */
const short = (t: string) => t.length > 118 ? t.slice(0, 115).replace(/[\s.,;:]+\S*$/, '') + '...' : t;

const ENV_TITLE = 'PLATE 4 / An envelope for T. Vega, 1947';
const ENV_TEXT = 'An envelope addressed to T. Vega, care of Sierra Motor Court. Postmarked July 1947, never opened.\n\nA photo shop\'s envelope from Roswell, sealed with wax. In pencil along the bottom, in the shop\'s hand: Plate 4. 1 print. 35c.';
const ENV_LIT = 'Held up in front of the lamp on the counter, the paper goes the colour of weak tea. The flaps show through, and a print inside, a little smaller than the envelope.\n\nOn the print, small and dark against something lighter: a tripod with the transit on it, and three people standing by it. Two close together. The third a step off, and not as sharp.';

export class FieldBook {
  /** P06 to P09 have the station's records; this has the book and what it shares with tonight. */
  readonly book: RuleBook;
  /** Seen since the book was read, and written down in a moment (after the event's own note). */
  private pending = new Map<FbEvent, number>();
  private driveT = 0;
  private calm = 0;
  private knocked = false;
  /** The drawer is sliding out and the book about to open (a second Use waits for it). */
  private opening = false;
  private frontArt: string | null = null;
  /** Opened, and not read to the end yet: the minutes it had when it was first opened. */
  private draft: Partial<Record<FbEvent, string>> | null = null;
  private litArt: string | null = null;

  constructor(private d: FieldBookDeps) {
    this.book = new RuleBook({
      key: 'ch3',
      owns: (n) => /^(fb|tn)\./.test(n),
      notes: NOTES, rules: RULES, questions: [],
      links: () => this.s?.links ?? [],
      wrong: () => {},
      save: () => this.d.save(),
      cards: () => this.cards(),
      empty: 'Not the same thing. Find his line for what you saw.',
    });
  }

  private get s() { return this.d.state.get(); }
  get read() { return !!this.s; }
  /** The book's part of the board, once it has been read. */
  books() { return this.read ? [this.book] : []; }

  // ---------- what tonight has ----------
  private has(e: FbEvent) { return this.d.seen.has(KEY[e]); }
  private noteOf(e: FbEvent) { const n = this.d.notes(); for (let i = n.length - 1; i >= 0; i--) if (n[i].includes(NOTE[e])) return n[i]; return null; }
  /** The minute the player had it: the map's mark, or the note's clock. */
  private timeOf(e: FbEvent): string | null {
    const m = MARK[e];
    if (m) for (const k of this.d.seen) if (k.startsWith(`mark:${m}@`)) return clockText(Number(k.slice(k.indexOf('@') + 1)), false);
    const n = this.noteOf(e)?.match(/^(\d\d:\d\d)\./);
    return n ? n[1] : null;
  }

  // ---------- the drawer and the book ----------
  drawerLabel() { return this.read || this.draft ? 'T. Vega\'s field book' : 'Open the drawer'; }
  drawerExamine() {
    return this.read || this.draft ? 'Pencils, a dry rubber band, and a field book in green cloth: T. VEGA, FIELD BOOK 3.'
      : 'A shallow drawer under the bench top, below the lamp. The pull is worn bright.';
  }
  useDrawer() {
    if (this.opening) return;
    if (this.read || this.draft) { this.openBook(this.s?.page ?? 0); return; }
    this.opening = true;
    this.d.drawer(true);
    this.d.audio.play(this.d.audio.or('door_latch', 'switch'), { gain: 0.22, rate: 0.7 });
    this.d.audio.play('thudSoft', { gain: 0.18, rate: 1.7, when: 0.25 });
    this.d.toast('The drawer sticks, then comes. Two pencils, a dry rubber band, and a field book in green cloth.', 3.6);
    this.d.after(1.1, () => this.openBook(0));
  }
  /** The book, open at a page. It is read once the last page has been turned to and it is put
   *  back; until then it keeps the minutes it had when it was first opened (it does not change). */
  openBook(start = this.s?.page ?? 0) {
    this.opening = false;
    const times = this.s?.times ?? (this.draft ??= this.timesNow());
    const pages = fbPages(times);
    let end = false;
    fieldBookView(this.d.ui, {
      title: 'T. Vega / Field book 3', pages, start,
      onPage: (n) => { const s = this.s; if (s) s.page = n; if (n === pages.length - 1) end = true; },
      onClose: (last) => {
        if (this.read) { this.s!.page = last; this.d.save(); }
        else if (end) this.firstRead(times, last);
      },
    });
  }
  private timesNow() {
    const out: Partial<Record<FbEvent, string>> = {};
    for (const e of EVENTS) if (this.has(e)) { const t = this.timeOf(e); if (t) out[e] = t; }
    return out;
  }
  private firstRead(times: Partial<Record<FbEvent, string>>, last: number) {
    const s: FbSave = { at: this.d.clock(), times, had: EVENTS.filter((e) => this.has(e)), told: [], links: [], page: last };
    this.d.state.set(s);
    this.draft = null;
    this.d.file(this.doc());
    const seenNow = EVENTS.filter((e) => this.has(e));
    const list = seenNow.filter((e) => e !== 'cows2' || !seenNow.includes('cows')).map((e) => e === 'cows' && seenNow.includes('cows2') ? 'the herd on 026, nine and then eight' : WHAT[e]);
    const and = (l: string[]) => l.length <= 1 ? l.join('') : `${l.slice(0, -1).join(', ')} and ${l[l.length - 1]}`;
    const more = EVENTS.some((e) => !this.has(e));
    let n = `${clockText(this.d.clock(), false)}. The hut at STATION 01, the drawer under the bench: T. Vega's field book, June and July 1947.`;
    if (list.length) n += ` In July he wrote down ${and(list)}. I saw ${list.length > 1 ? 'them' : 'it'} tonight, and the times he has are mine.`;
    if (more) n += list.length ? ' There is more in it that I have not seen. Not yet.' : ' I have not seen any of it. Not yet.';
    n += ' The last page: "Three on plate 4. There were two of us."';
    if (this.d.seen.has('env:lamp')) n += ' The envelope in the motel office has Plate 4 on it.';
    this.d.note(n);
    this.d.toast(list.length ? 'In July 1947, he wrote down tonight.' : 'July 1947. None of it has happened tonight. Not yet.', 4);
    this.d.foretell();
    this.d.refresh();
    this.d.save();
  }
  /** The book as a paper in the notebook (opened in its own view, main.ts). */
  doc(): DocSpec {
    const pages = fbPages(this.s?.times ?? {});
    return { id: 'fieldbook', title: 'FIELD BOOK 3 / T. Vega, June and July 1947', kind: 'hand', stamp: 'T. VEGA',
      page: pages.map((p) => p.hand).join('\n\n'),
      transcript: 'From the drawer under the bench in the hut at STATION 01. Pencil, in a field book bound in green cloth. The survey in June; in July, other things.' };
  }

  // ---------- the envelope in the motel office ----------
  envelopeLabel() { return this.d.seen.has('env:lamp') ? 'Envelope, plate 4' : 'Sealed envelope'; }
  useEnvelope() {
    this.frontArt ??= envelopeFront();
    this.litArt ??= envelopeLit();
    envelopeView(this.d.ui, {
      title: ENV_TITLE, front: this.frontArt, lit: this.litArt, text: ENV_TEXT, litText: this.envLit(),
      onHold: () => this.held(),
      onClose: () => {},
    });
  }
  private envLit() { return this.read ? `${ENV_LIT}\n\nThere were two of them. He wrote it on the last page of his field book.` : ENV_LIT; }
  private held() {
    if (this.d.seen.has('env:lamp')) return;
    this.d.seen.add('env:lamp');
    this.d.file(this.envelopeDoc());
    this.d.note(`${clockText(this.d.clock(), false)}. Sierra Motor Court, the office. On the counter, an envelope for T. Vega that nobody has opened since 1947: "Plate 4. 1 print. 35c." Held up to the lamp: a print inside, and on it a tripod and three small figures standing by it.${this.read ? ' He wrote that there were two of them.' : ''}`);
    this.d.refresh();
    this.d.save();
  }
  envelopeDoc(): DocSpec {
    this.litArt ??= envelopeLit();
    return { id: 'env4', title: ENV_TITLE, kind: 'typed', stamp: 'PLATE 4',
      page: 'PRINTS . ENLARGEMENTS . FINISHING\nROSWELL, N. MEX.\n\nT. Vega\nc/o Sierra Motor Court\nCaliche, N. Mex.\n\n~ Plate 4. 1 print. 35c.',
      image: this.litArt, transcript: `${ENV_LIT}\n\nThe envelope is still sealed. It stays on the counter.` };
  }

  // ---------- the board's cards ----------
  private cards(): BoardCard[] {
    const s = this.s;
    if (!s) return [];
    const at = (k: Card | 'fb'): { x: number; y: number } => { const p = AT[k === 'fb' ? 'fb' : `tn.${k}`]; return { x: p[0], y: p[1] }; };
    const t = (e: FbEvent) => s.times[e] ? `, ${s.times[e]}` : '';
    const book: BoardCard = { id: 'fb', doc: 'fieldbook', stamp: 'FIELD BOOK 3', title: 'T. VEGA, JUNE AND JULY 1947', have: true, w: 320, ...at('fb'),
      full: fbPages(s.times).map((p) => p.typed).join('\n\n'),
      lines: [
        { id: 'fb.lights', text: `Jul 1${t('lights')}. Three lights, low, west of the road. They turned off along B and went out.` },
        { id: 'fb.cows', text: `Jul 3${t('cows')}. The herd all one way, on 026. Nine head. Jul 4${t('cows2')}. Eight.` },
        { id: 'fb.knocks', text: `Jul 5${t('knocks')}. Three knocks on the roof of the car, slow, from above.` },
        { id: 'fb.lamps', text: `Jul 6${t('lamps')}. The lamps on Main Street out one by one, the dogs with them.` },
        { id: 'fb.car', text: `Jul 7${t('car')}. The county car on the shoulder. Nobody in it.` },
        { id: 'fb.plate', text: 'Three on plate 4. There were two of us.' }] };
    const tonight = (k: Card): BoardCard => {
      const have = k === 'cows' ? this.has('cows') || this.has('cows2') : this.has(k);
      const note = k === 'cows' ? (this.has('cows2') ? this.noteOf('cows2') : this.noteOf('cows')) : this.noteOf(k);
      const text = note ?? TONIGHT[k].fallback;
      return { id: `tn.${k}`, stamp: 'TONIGHT', title: TONIGHT[k].title, kind: 'thing', have, w: 190, ...at(k),
        where: 'In his book. Not yet in mine.', text: short(text), readingText: text, full: text.length > 118 ? text : undefined };
    };
    return [book, ...(['lights', 'cows', 'knocks', 'lamps', 'car'] as Card[]).map(tonight)];
  }

  // ---------- after the book ----------
  /** Each frame: what the book has that comes tonight after it was read is written down (once),
   *  and while driving, the knocks on the roof it has, if the night has not had them yet. */
  update(dt: number, o: { ok: boolean; driving: boolean; knock: () => boolean }) {
    const s = this.s;
    if (!s) { this.driveT = 0; return; }
    for (const e of EVENTS) {
      if ((s.had ?? []).includes(e) || s.times[e] || s.told.includes(e) || !this.has(e)) continue;
      // its own note and toast first; then this one, with nothing open over the game
      const left = (this.pending.get(e) ?? (e === 'cows2' ? 4.5 : 3.0)) - dt;
      if (left > 0 || this.d.ui.modal) { this.pending.set(e, Math.max(0, left)); continue; }
      this.pending.delete(e);
      s.told.push(e);
      this.d.note(`${clockText(this.d.clock(), false)}. It is in T. Vega's field book, under ${DATE[e]}, 1947: ${WHAT[e]}. He wrote it in July 1947. I saw it tonight.`);
      this.d.toast(`T. Vega, ${DATE[e]}, 1947: "${QUOTE[e]}"`, 4.6);
      this.d.refresh();
      this.d.save();
    }
    // the knocks: some way into a drive, once nothing else is going on, and has not been for a while
    if (this.has('knocks') || this.knocked) return;
    this.calm = o.ok ? this.calm + dt : 0;
    if (!o.driving) { this.driveT = 0; return; }
    if (!o.ok) return;
    this.driveT += dt;
    if (this.driveT > KNOCK_AFTER && this.calm > KNOCK_QUIET && o.knock()) this.knocked = true;
  }

  // ---------- a case opened ----------
  /** After a load: the papers back in the notebook, the drawer as it was left. */
  restore() {
    this.pending.clear(); this.driveT = 0; this.calm = 0; this.knocked = false; this.draft = null; this.opening = false;
    if (this.read) this.d.file(this.doc());
    if (this.d.seen.has('env:lamp')) this.d.file(this.envelopeDoc());
    this.d.drawer(this.read, true);
    if (this.read) this.d.foretell();
  }
  /** The drawer as the case has it, once the station has been built (main.ts). */
  drawerNow() { this.d.drawer(this.read || !!this.draft, true); }
}
