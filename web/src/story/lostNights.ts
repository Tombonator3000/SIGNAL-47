import type { DocSpec } from '../ui/UI';
import type { BoardCard } from '../ui/Board';
import type { TalkLine } from '../ui/Panels';
import type { ReelTape } from '../core/reelTape';
import { RuleBook, type BookNote, type BookQuestion, type BookRule } from './boardRules';
import { BINDER } from './nightshift';
import { clockText } from './time';

// The nights SARO printed a negative range are the nights Caliche lost time (FORSLAG.md, 20 October,
// item 11, and small tweak 4). The exceptions binder over the printer has three printouts from
// before tonight: 09/22/81 -34, 03/02/83 -36, 10/19/85 -38. The deputy's green pins are people who
// lost track of the time, three of them in town, on the same nights (story/caliche.ts): the boy who
// called the radio in September 1981, the man whose wife sits out at the laundromat, March 1983, and
// the attendant, October 19 last year. Nobody says so. On Night Shift's patch of the case board the
// binder lies with a pin on each printout and the green pins with a pin on each person, and a thread
// between a printout and its person holds: "The night SARO printed -38, the attendant lost two hours
// on the old road." With all three nights tied, a question comes onto the cork that nothing answers:
// "-39 tonight. Who loses time tonight?" (After a time away, the answer is under it in the player's
// own hand, the minutes they lost, which they do not remember writing: the abduction's trace.)
//
// The 1981 night is also Earl's reel, the one call of twelve years he has never played (caliche.ts).
// Once the board has given the player the date, they can give it to him at the studio door before
// the station signs off at four, and he plays it (core/reelTape.ts): the boy says what the boy said on
// the Night Line tonight, word for word, and where Earl answered him tonight the tape has nothing on
// it, not even hiss, for exactly as long. "I answered him tonight. He wasn't answering me." A living
// man answered a recording (R5: the imprint repeats and cannot answer).
//
// October 1985 has two more things: the carbon of the tow ticket, which the attendant gives the player
// when asked (out 1:45, in 4:25, 2.7 hours for a forty minute job; the remarks: the driver said his
// engine quit at 11:58, and his watch said so too), and the driver's watch, which has said two minutes
// to twelve ever since. He is at the counter in the Mesa Diner (story/Chapter5.ts). The binder says
// 23:58:31.
//
// The threads are kept with Night Shift's part of the case (NightShiftState.ln); what the player has
// found with the town's ('ask:attendant:tow', 'ln:watch', 'ln:reel'; 'ln:85', 'ln:83', 'ln:81' once a
// night is tied, 'ln:q39' once the question is up).

export interface LostNightsDeps {
  /** The town's part of the case. */
  seen: Set<string>;
  /** Where Night Shift keeps the threads (NightShiftState.ln). */
  links: () => string[];
  /** Chapter two has begun (the book is on the board from then, once there is something for it). */
  open: () => boolean;
  /** The exceptions binder has been read (it is in the journal). */
  binder: () => boolean;
  notes: () => string[];
  note: (s: string) => void;
  clock: () => number;
  file: (doc: DocSpec) => void;
  save: () => void;
  refresh: () => void;
  tape: ReelTape;
  /** Nothing else of the night's is going on now (one thing at a time). */
  quiet: () => boolean;
  /** The other voices stop before the reel (the narrators, a call, the CB). */
  hushVoices: () => void;
  /** The boy has called the Night Line tonight (core/radio.ts, Radio.boyCalled). */
  boyCalled: () => void;
}

// ---------- what the town has (story/caliche.ts) ----------
const K = {
  pins: 'ask:deputy:green', old: 'ask:attendant:old', tow: 'ask:attendant:tow', wife: 'ask:radio_woman:husband',
  tape: 'ask:host:tape', boy: 'ev:boy', watch: 'ln:watch', reel: 'ln:reel', watchAsked: 'ln:watch:asked', towShown: 'ln:tow:shown',
} as const;

const aside = (text: string): TalkLine => ({ who: '', text });
const you = (text: string): TalkLine => ({ who: 'YOU', text });
const EA = (text: string): TalkLine => ({ who: 'EARL', text });
const D = (text: string): TalkLine => ({ who: 'THE DRIVER', text });
const REEL = 'REEL, 9/22/81';

/** The carbon of the tow ticket, from the attendant's spike by the register (caliche.ts). */
export const TOW_SLIP: DocSpec = {
  id: 'towslip', title: 'Tow ticket / Caliche Service, No. 0419', kind: 'typed', stamp: 'TOW',
  page: `CALICHE SERVICE
GAS . DIESEL . TOWING
CALICHE, N. MEX.          No. 0419

DATE
~ 10-20-85
CALLED IN               BY
~ 1:40 AM               ---
OUT
~ 1:45 AM
VEHICLE
~ Tractor & reefer trailer
FROM
~ Old Roswell Rd, past 8 mi post
TO
~ Mesa Diner lot
REMARKS
~ Driver in cab. Says engine quit 11:58.
~ His watch says so too.
IN
~ 4:25 AM
HOURS
~ 2.7

~ 40 min job. 16 mi. Where's the other 2 hrs?`,
  transcript: `A yellow carbon off the spike of tow tickets by the register at the gas station, filled in in ballpoint. A tractor and a refrigerated trailer, towed in off the old road from just past the eight-mile post, to the diner's lot. Out at a quarter to two, in at twenty-five past four: 2.7 hours, for a forty minute job.

The line for who called it in is blank. In the remarks: the driver was in the cab, and said his engine quit at 11:58. His watch said so too.

Under it, in the attendant's pencil, a sum that does not come out.`,
};

/** Reyes's notebook copy of the reel, once it has been heard to the end. */
function reelPaper(heard: boolean): DocSpec {
  const page = `1420 AM / THE REEL FROM 9/22/81

Pencil on the box, in Earl's hand:
9/22/81. OFF AIR.

ON THE REEL, a boy, over a telephone line:
"Hello? Is this the radio? There's a star with a tail on it, real low in the south. A little tail."
Then nothing. Not hiss: nothing.
"I'm calling from my dad's place, out on the ranch. There's a light over the mesa too. It isn't moving."
Nothing again, shorter.
"He's out with the men. It's hot out. It's July."
A click on the line.

${heard ? 'Word for word what I heard the boy say on the Night Line tonight.' : 'Earl says it is word for word what the boy said on the Night Line tonight.'} Where it goes quiet is where Earl answered him tonight, and for as long.

EARL:
"I answered him tonight."
"He wasn't answering me."
"Station was off that night in eighty-one. There was nobody on the air to answer him."
"He left room for me anyway."`;
  return { id: 'witness-reel', title: 'Witness / Earl\'s reel from 9/22/81', kind: 'hand', stamp: 'REYES',
    page: 'REYES / NOTEBOOK COPY\n\n' + page,
    transcript: 'Copied into my notebook at the studio door. What is on the reel, and what Earl said.\n\n' + page };
}

// ---------- the board ----------
/** Night Shift's patch of cork has the evening's papers along the top; the binder, the green pins,
 *  what the three in Caliche said and the question lie under them. What turns up later of October
 *  1985 and of the reel (the ticket, the watch, the reel, tonight's boy) goes on the same patch, in
 *  the first place there that has room (and stays there). */
const AT: Record<string, [number, number, number]> = {
  'ln.binder': [14, 372, 282], 'ln.pins': [308, 372, 282], 'ln.said': [602, 372, 290], 'ln.q39': [904, 372, 262],
};
const at = (id: string) => ({ x: AT[id][0], y: AT[id][1], w: AT[id][2] });
// the October 1985 night: the hours (the binder, the green pin, the attendant's own words, the
// ticket's times) and the minute (the binder, the ticket's remarks, the watch)
const HOURS = ['ln.b85', 'ln.g85', 'ln.w85', 'ln.th'], MINUTE = ['ln.b85', 'ln.tq', 'ln.watch'];
const N85 = [...new Set([...HOURS, ...MINUTE])], N83 = ['ln.b83', 'ln.g83', 'ln.w83'], N81 = ['ln.b81', 'ln.g81', 'ln.w81', 'ln.reel'];
/** Two pins on the same card (the board says so itself). */
const CARD: Record<string, string> = {
  'ln.b81': 'b', 'ln.b83': 'b', 'ln.b85': 'b', 'ln.g81': 'g', 'ln.g83': 'g', 'ln.g85': 'g', 'ln.w81': 'w', 'ln.w83': 'w', 'ln.w85': 'w', 'ln.tq': 't', 'ln.th': 't',
};
/** The notes that tie a night of the binder's to Caliche (the others: two things in Caliche). */
const NIGHT: Record<string, '85' | '83' | '81'> = { n85: '85', n85m: '85', n83: '83', n81: '81' };
const Q39 = '-39 tonight. Who loses time tonight?';
/** What gives one of the three nights a date in Caliche (story/caliche.ts). */
const DATES = [K.pins, K.old, K.wife, K.tape];

export class LostNights {
  readonly book: RuleBook;
  /** The question has just come onto the cork with the thread just tied (its reply says so). */
  private justAsked = false;

  constructor(private d: LostNightsDeps) {
    const self = this;
    const seen = d.seen;
    const notes: Record<string, BookNote> = {
      n85: { text: 'The night SARO printed -38, the attendant lost two hours on the old road.' },
      n85m: { text: 'SARO printed -38 at 23:58:31. On the old road a rig quit at 11:58, and the driver\'s watch stopped with it.' },
      n85x: { text: 'October 19, 1985: a rig quit on the old road at two minutes to midnight, and the man who towed it in lost two hours.' },
      n85h: { text: 'The attendant\'s two hours last October: out at a quarter to two, back at twenty-five past four, for a forty minute job.' },
      n85w: { text: '11:58 on the ticket and on his watch. It stopped with the engine, and it never went again.' },
      n83h: { text: 'The deputy\'s pin and the woman on the bench: her husband, March 1983. Forty-seven minutes.' },
      n81h: { text: 'The same boy, the same September: the call Earl kept on reel and never played back.' },
      n83: { get text() { return seen.has(K.wife)
        ? 'The night SARO printed -36, a man in Caliche went out to his cattle and was gone forty-seven minutes.'
        : 'The night SARO printed -36, a man in Caliche lost track of the time.'; } },
      n81: { get text() { return seen.has(K.tape)
        ? 'The night SARO printed -34, a boy called the radio in Caliche, and the station was off the air.'
        : 'The night SARO printed -34, a boy called the radio in Caliche.'; } },
      nreel: { text: 'The boy on the Night Line tonight is on Earl\'s reel from September 1981, word for word. Earl answered a recording.' },
    };
    this.book = new RuleBook({
      key: 'ns',
      owns: (n) => /^ln\./.test(n),
      notes,
      get rules() { return self.rules(); },
      questions: [this.question()],
      links: () => d.links(),
      wrong: () => {},
      save: () => { this.sync(); d.save(); },
      cards: () => this.cards(),
      empty: 'Not the same night. Look at the dates.',
    });
  }

  /** The book's part of the board, from chapter two on: once the binder has been read, or once
   *  somebody in Caliche has given a date (then the binder lies there as a card not yet on the table:
   *  the dates are in it). */
  books() { return this.d.open() && (this.d.binder() || DATES.some((k) => this.d.seen.has(k))) ? [this.book] : []; }

  private rules(): BookRule[] {
    const out: BookRule[] = [];
    const self = this;
    const tie = (a: string, b: string, note: string) => out.push({ a, b, note, get text() { return self.say(note); } });
    const each = (list: string[], f: (a: string, b: string) => void) => {
      for (let i = 0; i < list.length; i++) for (let j = i + 1; j < list.length; j++) {
        const a = list[i], b = list[j];
        if (!CARD[a] || CARD[a] !== CARD[b]) f(a, b);
      }
    };
    // with the binder, SARO's night; without it, only that two things in Caliche are the same night
    each(N85, (a, b) => {
      const other = a === 'ln.b85' ? b : b === 'ln.b85' ? a : null;
      tie(a, b, other ? (HOURS.includes(other) ? 'n85' : 'n85m')
        : HOURS.includes(a) && HOURS.includes(b) ? 'n85h' : MINUTE.includes(a) && MINUTE.includes(b) ? 'n85w' : 'n85x');
    });
    each(N83, (a, b) => tie(a, b, a === 'ln.b83' || b === 'ln.b83' ? 'n83' : 'n83h'));
    each(N81, (a, b) => tie(a, b, a === 'ln.b81' || b === 'ln.b81' ? 'n81' : 'n81h'));
    tie('ln.reel', 'ln.boy', 'nreel');
    // tonight's boy and the boy of 1981, before the reel has been heard: the thread holds, thin
    for (const b of ['ln.g81', 'ln.w81']) out.push({ a: 'ln.boy', b, text: 'A boy on the radio, tonight and in September 1981. Earl kept the one from 1981. He has never played it.' });
    return out;
  }
  private say(note: string) {
    const t = this.book.o.notes[note].text;
    if (!this.justAsked) return t;
    this.justAsked = false;
    return `${t} And a question with it: ${Q39}`;
  }
  /** Which of the three nights are tied on the board. */
  private nights() {
    const out = new Set<string>();
    for (const k of this.d.links()) { const n = this.book.ruleOf(k)?.note; const night = n ? NIGHT[n] : null; if (night) out.add(night); }
    return out;
  }
  private trio() { const n = this.nights(); return n.has('85') && n.has('83') && n.has('81'); }
  /** After each thread (the board draws itself again after it): what it found goes with the town's
   *  part of the case (Earl needs the date), and with the three nights tied, the question and a line
   *  in the notebook, once. */
  private sync() {
    for (const n of this.nights()) this.d.seen.add('ln:' + n);
    if (this.trio() && !this.d.seen.has('ln:q39')) {
      this.d.seen.add('ln:q39');
      this.justAsked = true;
      this.d.note(`${clockText(this.d.clock(), false)}. The case board. Three nights SARO printed a negative range, somebody in Caliche lost time: September 1981, a boy on the radio; March 1983, a man out with his cattle; October 1985, the attendant on the old road. Tonight SARO printed -39.`);
    }
  }

  private question(): BookQuestion {
    const self = this;
    return {
      id: 'ln.q39', stamp: '-39', text: Q39, slots: 0,
      hints: ['Nothing on the board answers it.', 'Every night SARO printed a negative range, somebody in Caliche lost time. Tonight it printed -39.', 'Keep an eye on the clock tonight.'],
      open: () => this.trio(), done: () => null,
      get elsewhere() { return self.answer(); },
      ...at('ln.q39'),
    };
  }
  /** Under the question, once the player has been away: the minutes, in their own hand. */
  private answer() {
    const away = this.d.notes().map((n) => /^(\d\d:\d\d) to (\d\d:\d\d)\. Nothing\.$/.exec(n)).filter(Boolean).map((m) => `${m![1]} to ${m![2]}`);
    if (!away.length) return 'Nobody has an answer for it. Not yet.';
    const list = away.length === 1 ? away[0] : `${away.slice(0, -1).join(', ')} and ${away[away.length - 1]}`;
    return `Under it, in my own hand:\n~ ${list}\nI do not remember writing it.`;
  }

  private cards(): BoardCard[] {
    const s = this.d.seen, out: BoardCard[] = [];
    if (!this.d.binder()) out.push({ id: 'ln.binder', stamp: 'EXCEPTIONS', title: 'SOLVER EXCEPTIONS BINDER', have: false, ...at('ln.binder'),
      where: 'On the shelf over the printer in the control room. The old printouts in it have dates.' });
    else out.push({ id: 'ln.binder', doc: 'binder', stamp: 'EXCEPTIONS', title: 'SOLVER EXCEPTIONS BINDER', have: true, ...at('ln.binder'),
      text: 'Three printouts from before tonight, each stamped INPUT ERROR, and an older sheet.',
      full: [BINDER.page.replace(/^~ /gm, ''), BINDER.transcript].join('\n\n'),
      lines: [
        { id: 'ln.b81', text: '09/22/81  01:52:07  -34 LY. In pencil: clock?' },
        { id: 'ln.b83', text: '03/02/83  03:10:44  -36 LY. In pencil: baseline file reloaded.' },
        { id: 'ln.b85', text: '10/19/85  23:58:31  -38 LY. In pencil: third time. Ask Systems.' },
      ] });
    const pins = { id: 'ln.pins', stamp: 'GREEN PINS', title: 'THE DEPUTY\'S MAP, CALICHE', kind: 'thing' as const, ...at('ln.pins') };
    if (s.has(K.pins)) out.push({ ...pins, have: true,
      text: '"Green is people who lost track of the time." Three of them in town.',
      lines: [
        { id: 'ln.g81', text: 'A kid who called the radio station in September of \'81. Family moved away after.' },
        { id: 'ln.g83', text: 'A man whose wife sits out at the laundromat nights. March of \'83.' },
        { id: 'ln.g85', text: 'The fella at the gas station. October nineteenth, last year. Went out to tow a rig and lost two hours.' },
      ] });
    else if (s.has('pins') || s.has('met:deputy')) out.push({ ...pins, have: false, where: 'Green pins on the deputy\'s map. He knows what each one is.' });
    // what the three said themselves, a line each as they said it
    const said = [
      s.has(K.tape) && { id: 'ln.w81', text: 'Earl, 1420 AM: "September of \'81, a boy called in. I\'ve never played it back. The station was off."' },
      s.has(K.wife) && { id: 'ln.w83', text: 'The woman on the bench: "He went out to look at the cattle one night. March, three years ago. Forty-seven minutes."' },
      s.has(K.old) && { id: 'ln.w85', text: 'The attendant: "Towed a rig in off it once. Last October. Left here a quarter to two. Got back twenty-five past four."' },
    ].filter(Boolean) as { id: string; text: string }[];
    if (said.length) out.push({ id: 'ln.said', stamp: 'CALICHE', title: 'WHAT THEY TOLD ME', kind: 'thing', have: true, ...at('ln.said'),
      text: 'In their own words, from my notebook.', lines: said });
    if (s.has(K.tow)) out.push({ id: 'ln.tow', doc: 'towslip', stamp: 'TOW', title: 'CALICHE SERVICE, No. 0419', have: true, w: 262,
      text: '10-20-85. A tractor and reefer, off the old road past the 8 mi post. Called in by: blank.',
      full: [TOW_SLIP.page.replace(/^~ /gm, ''), TOW_SLIP.transcript].join('\n\n'),
      lines: [
        { id: 'ln.tq', text: 'Driver in cab. Says engine quit 11:58. His watch says so too.' },
        { id: 'ln.th', text: 'Out 1:45 AM. In 4:25 AM. Hours 2.7. In pencil: 40 min job.' },
      ] });
    if (s.has(K.watch)) out.push({ id: 'ln.watch', stamp: 'MESA DINER', title: 'THE DRIVER\'S WATCH', kind: 'thing', have: true, w: 220,
      text: s.has(K.watchAsked) ? 'Two minutes to twelve. It stopped the minute his rig quit on the old road last October, and it will not go.'
        : 'Two minutes to twelve, on the wrist of the driver at the counter. It is not running.' });
    if (s.has(K.reel)) out.push({ id: 'ln.reel', doc: 'witness-reel', stamp: 'REEL', title: 'EARL\'S REEL, 9/22/81', kind: 'thing', have: true, w: 240,
      text: 'The boy, word for word as tonight. Where Earl answered him tonight, nothing on the tape. Not even hiss.' });
    if (s.has(K.boy)) {
      const n = this.d.notes().find((x) => x.includes('A boy calling about a star with a tail'));
      out.push({ id: 'ln.boy', stamp: 'TONIGHT', title: 'THE BOY ON THE NIGHT LINE', kind: 'thing', have: true, w: 230,
        text: n ?? 'Night Line, 1420. A boy calling about a star with a tail, low in the south. He said it was July.' });
    } else if (s.has(K.reel)) out.push({ id: 'ln.boy', stamp: 'TONIGHT', title: 'THE BOY ON THE NIGHT LINE', kind: 'thing', have: false, w: 230,
      where: 'Earl had him on the Night Line tonight. I did not hear it.' });
    return out;
  }
  // ---------- Earl's reel (caliche.ts calls these from the studio door) ----------
  /** The date can be given to Earl now: the board has the 1981 night, and nothing else is on. */
  reelReady() { return this.d.seen.has('ln:81') && !this.d.seen.has(K.reel) && !this.d.tape.busy && this.d.quiet(); }
  get reelBusy() { return this.d.tape.busy; }
  /** The reel, as lines under the conversation: the tape as it plays, then a page of Earl's. */
  playReel(o: { lines: (l: TalkLine[]) => void; done: (complete: boolean) => void }) {
    this.d.hushVoices();
    let page: TalkLine[] = [];
    const put = (l: TalkLine) => { page = [...page, l]; o.lines(page); };
    this.d.tape.play({
      cue: (c) => {
        if (c.kind === 'start') { page = []; put(aside('Hiss from the monitor. An open telephone line, a long way off.')); }
        else if (c.kind === 'boy') put({ who: REEL, text: c.text });
        else if (c.kind === 'hole') put(aside(c.i === 0 ? 'Then nothing. Not hiss: nothing at all, and the reels going round.' : 'Nothing again, shorter. Earl\'s lips are moving.'));
        else if (c.kind === 'click') put(aside('The hiss again, and a click on the line, as if a receiver were put down at the other end.'));
      },
      done: (complete) => {
        if (!complete) { o.done(false); return; }
        const heard = this.d.seen.has(K.boy);
        this.played(heard);
        o.lines([
          aside('He stops the deck. He does not rewind it.'),
          ...(heard ? [aside('Word for word what you heard on the Night Line tonight.')] : [EA('That\'s what he said tonight. On the air. Every word.')]),
          EA('Where it goes quiet, that\'s me. That\'s how long I talked to him. Both times.'),
          EA('I answered him tonight.'),
          EA('He wasn\'t answering me.'),
          EA('Station was off that night in eighty-one. There was nobody on the air to answer him.'),
          EA('He left room for me anyway.'),
          aside('He puts the reel back in its box. On the spine, in pencil, in his own hand: 9/22/81. OFF AIR.'),
        ]);
        o.done(true);
      },
    });
  }
  stopReel() { this.d.tape.stop(); }
  private played(heard: boolean) {
    const s = this.d.seen;
    s.add(K.reel);
    this.d.boyCalled();
    this.d.file(reelPaper(heard));
    this.d.note(`${clockText(this.d.clock(), false)}. 1420 AM. Earl played me the reel he never played: 9/22/81, the night SARO printed -34, with the station off the air. The boy on it says what ${heard ? 'I heard the boy say' : 'Earl says the boy said'} on the Night Line tonight, word for word. Twice the tape has nothing on it, not even hiss, for as long as Earl answered him tonight. Earl: "I answered him tonight. He wasn't answering me."`);
    this.d.refresh();
    this.d.save();
  }

  // ---------- the driver at the counter in the Mesa Diner (Chapter5.ts) ----------
  private seeWatch() {
    if (this.d.seen.has(K.watch)) return;
    this.d.seen.add(K.watch);
    this.d.note(`${clockText(this.d.clock(), false)}. Mesa Diner. The driver at the counter: his wristwatch says two minutes to twelve. It is not running.`);
    this.d.refresh();
    this.d.save();
  }
  /** A closer look at him, any time the diner is there. */
  driverExamine() {
    this.seeWatch();
    return 'A driver over two eggs and a thermos, his cap pushed back. His wristwatch says two minutes to twelve.';
  }
  /** When he is spoken to (chapter five): the watch, the first time. */
  driverOpen(): TalkLine[] {
    if (this.d.seen.has(K.watch)) return [];
    this.seeWatch();
    return [aside('He checks his watch, the way people do. It says two minutes to twelve.')];
  }
  driverChoices(october: boolean): { id: string; label: string }[] {
    const s = this.d.seen, out: { id: string; label: string }[] = [];
    if (s.has(K.watch) && !s.has(K.watchAsked)) out.push({ id: 'ln:watch', label: 'Ask about his watch' });
    if (s.has(K.tow) && october && !s.has(K.towShown)) out.push({ id: 'ln:tow', label: 'Show him the tow ticket from the gas station' });
    return out;
  }
  driverChoose(id: string): TalkLine[] | null {
    const s = this.d.seen, t = clockText(this.d.clock(), false);
    if (id === 'ln:watch') {
      s.add(K.watchAsked);
      this.d.note(`${t}. The driver's watch has said 11:58 since last October. It stopped the minute his rig quit on the old road, and it will not go again. A man in Roswell found nothing wrong with it. He still wears it.`);
      this.d.refresh(); this.d.save();
      return [
        you('Your watch has stopped.'),
        D('Since October.'),
        aside('He turns his wrist and looks at it anyway, the way you look at a watch that works.'),
        D('Quit out there the same minute she did. Two minutes to midnight.'),
        D('Wound it, shook it, took it to a man in Roswell. Nothing wrong with it, he says. It just doesn\'t go.'),
        D('So I wear it.'),
      ];
    }
    if (id === 'ln:tow') {
      s.add(K.towShown);
      this.d.note(`${t}. Mesa Diner. I showed the driver the tow ticket: his rig, towed in off the old road that night, to this lot. He remembers sitting there a minute, and the engine coming back by itself. He does not remember a tow.`);
      this.d.refresh(); this.d.save();
      return [
        you('The gas station in Caliche towed a rig in off the old road last October. A tractor and a reefer.'),
        aside('You put the yellow carbon down by his plate. He reads it without picking it up.'),
        D('That\'s my rig.'),
        D('Nobody towed me. She came back on by herself, and I drove her in.'),
        you('It says you were in the cab. It says he had you on this lot by four.'),
        aside('He looks at his watch for a long time.'),
        D('I sat there a minute. Then she came back.'),
        aside('He pushes the carbon back across the counter and picks up his fork. He does not eat.'),
      ];
    }
    return null;
  }

  /** After a case is opened: what the reel left, back in Papers (the ticket comes back with the
   *  town's witness papers, caliche.ts). */
  restore() {
    if (this.d.seen.has(K.reel)) this.d.file(reelPaper(this.d.seen.has(K.boy)));
  }
}
