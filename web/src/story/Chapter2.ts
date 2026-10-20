import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import { PHONE_CALLS } from '../core/phoneScripts';
import type { ControlRoom, Collider } from '../world/ControlRoom';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { RecordsAnnex } from '../world/Annex';
import type { Doors } from '../world/Doors';
import * as P from '../ui/Panels';
import type { BoardCard } from '../ui/Board';
import { RuleBook, type BoardHook, type BookNote, type BookRule } from './boardRules';
import * as D from './drawings';
import { clockText } from './time';

// Chapter two, "The Amended Record" (K2 in the design bible, "Den strøkne protokollen").
// Ward reads the B-12 report and holds the morning series (her line V2 from the design
// bible). The records room behind the south corridor holds three collections: the 1947
// field records (E07 and the archive sleeve E08), the service copies (E06) and the
// maintenance lineage card for B-12. P04 and P05 follow the Unity dossier (WorldCase22)
// word for word: what changed in 1947, and which field site the reference leads to.
// The field access sheet in the matching sleeve names the key holder, N. Vega at Sierra
// Motor Court. Her two lines on the phone are new text, written for this port.

export type Stage2 = 'ward-call' | 'records' | 'call-nora' | 'complete';
type Finding = 'author-guilt' | 'omitted-c' | 'development-only';
type Marker = 'triangle-bar' | 'triangle' | 'three-bars';

export interface Ch2State {
  v: 1;
  stage: Stage2;
  answered: boolean; door: boolean; entered: boolean;
  read: Record<P.SourceKey, boolean>;
  p04: boolean; p05: boolean;
  finding: string; destination: string; surveyId: string; marker: string;
  wrong04: number; wrong05: number;
  called: boolean;
  links?: string[];         // this chapter's threads on the case board, 'a|b' sorted, in the order made
  /** Ward called back and asked for the array on the schedule (story/arrayBack.ts; older saves
   *  do not have it) */
  arrayAsk?: boolean;
}

export interface Chapter2Host {
  clock: number;
  gt: number;
  notes: string[];
  docs: DocSpec[];
  cinematic: boolean;
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
}

export interface Chapter2Deps {
  ui: UI; audio: AudioSys; room: ControlRoom; annex: RecordsAnnex; player: Player; inter: Interaction;
  colliders: Collider[];
  doors: Doors;
  save: () => void;
  board: BoardHook;
}

const fresh = (): Ch2State => ({
  v: 1, stage: 'ward-call', answered: false, door: false, entered: false,
  read: { original: false, amended: false, lineage: false, index: false },
  p04: false, p05: false, finding: '', destination: '', surveyId: '', marker: '',
  wrong04: 0, wrong05: 0, called: false, links: [],
});

// ---------- the sources (Unity WorldCase22 text) ----------
const ORIGINAL = 'FIELD SURVEY RECORD / STATION 01 / 1947\n\nThe reference remained visible in the plate after the lamp circuit was opened. A second plate was exposed with the local lamp isolated. The retained mark did not follow the new position of the physical vane.\n\nThe arrangement used three references: A, the fixed survey point; B, the optical comparison vane; and C, the closing sight line. The run was interrupted before the final observation was entered. Preserve the original plates and the complete arrangement drawing.\n\nN. VEGA / FIELD TECHNICIAN';
const AMENDED = 'FIELD SURVEY RECORD / STATION 01 / 1947\nAMENDED COPY\n\nThe additional mark is attributed to a fault in plate development. The lamp and comparison vane are sufficient to describe the test. The incomplete closing sight line has been omitted from the service copy.\n\nNo repeat observation is required. Retain the amended copy with the routine reference records.\n\nN. VEGA / FIELD TECHNICIAN';
const LINEAGE = 'SARO / REFERENCE LINEAGE / B-12\n\nThe B-12 comparison vane retains the fixed-point survey identified as STATION 01. Fixed-point register mark: outlined triangle with a short horizontal bar beneath it. The archive sleeve contains the dated plate and fixed-point sheet used for this index. Match the survey identifier and fixed-point mark before selecting the field destination.\n\nSITE REGISTER: STATION 01 / OLD SURVEY STATION. The matching archive sleeve contains the field access sheet.';
const INDEX = 'PLATE / 1947 / STATION 01\n\nFixed-point mark: outlined triangle with a short horizontal bar beneath it.\nArrangement: A - B - C.\nService index: B-12.\nField destination: OLD SURVEY STATION.\n\nRETURN CHECK: Match both STATION 01 and the fixed-point mark. A similar triangle without the bar is a general elevation symbol and is not a matching survey reference.';
const ACCESS = 'FIELD ACCESS / OLD SURVEY STATION\nSURVEY: STATION 01\n\nACCESS ROAD UNSURFACED. GATE LOCKED.\n\nKEY HOLDER:  N. VEGA\n             SIERRA MOTOR COURT\n             (505) 555-0119\n\nDO NOT DISTURB THE FIXED-POINT MARKS.';
const PHONE = '5055550119';
// Ward calls back once, a few seconds after her first call (FORSLAG.md, 20 October, item 12): the
// array. These two lines have no recording (core/phoneScripts.ts has the 24 there are), so they are
// captions on the line, as every unrecorded call is (AudioSys.phoneCall gives null).
const WARD_ARRAY = [
  { by: 'WARD', text: 'Reyes. One more thing.' },
  { by: 'WARD', text: 'Put the array back on the schedule and tell me it stays there.' },
];
/** How long the line rings for her before it stops, and how long after that it rings again (with the
 *  player back in the control room). */
const BACK_RING = 24, BACK_AGAIN = 40;

const SUPPORTED_04 = 'SUPPORTED / E07 records A, B and C and a retained mark with the lamp isolated. E06 omits C, the closing sight line, and calls the mark a development fault. The report was changed; motive remains unknown.';
const SUPPORTED_05 = 'SUPPORTED / B-12 retains STATION 01. The archive sleeve matches its ID and triangle-with-bar fixed-point mark. OLD SURVEY STATION is the supported destination. Field access prepared. The matching sleeve also holds the field access sheet.';

// Design bible section 08 (three levels) and the facilitator hints from the P04/P05 paper
// test, plus a fourth that names the threads, held back until the player has seen the
// third and still missed twice (SPILLDESIGN.md: hints never spell the pair too early).
const HINTS_04 = ['What is different between the two versions?', 'Which field is in the original and missing from the service copy? Compare the arrangement, not only the conclusion.', 'Put the original plan beside the amended copy and find the connection that is missing.',
  'Thread the three references in E07 to the omitted sight line in E06, and the retained mark in E07 to the development fault in E06.'];
const HINTS_05 = ['Where does today\'s B-12 reference come from?', 'Find both the site ID and the reference mark in the lineage card and in the sleeve.', 'Match STATION 01 and the triangle with a bar, then choose the place the sleeve names.',
  'Thread STATION 01 on the B-12 card to STATION 01 on the sleeve, the two triangles with the bar to each other, and the two place names.'];

// ---------- the evidence board ----------
// The lines that matter on each record. A thread between two lines that say something
// about each other holds; what they show together goes into its question by itself (P04:
// the omitted C and the explained-away mark; P05: the survey, the mark and the place).
// Some threads hold without answering anything: a note on the board, or a thin thread.
// Threads that contradict get the Unity replies and count as wrong, as before. The rules
// are worked by story/boardRules.ts; the board is the night's case board (story/caseBoard.ts).
const NOTES: Record<string, BookNote> = {
  'n.c': { text: 'C, the closing sight line, is in the original. The service copy leaves it out.', q: 'P04' },
  'n.mark': { text: 'The original kept the mark with the lamp isolated. The copy calls it a development fault.', q: 'P04' },
  'n.sig': { text: 'The same name signs both versions. It says who changed the record, not why.' },
  'n.id': { text: 'Same survey: STATION 01 on the B-12 card and on the 1947 sleeve.', q: 'P05' },
  'n.mark2': { text: 'Same fixed-point mark: the outlined triangle with the bar.', q: 'P05' },
  'n.dest': { text: 'Both name the same place: OLD SURVEY STATION.', q: 'P05' },
};
const ELEVATION = 'A triangle without the bar is only an elevation symbol. The matching fixed-point mark has a short bar beneath the outlined triangle.';
// -39 against a year from the archive holds, as a thin thread with nothing added: the
// player can do the sum or not (HISTORIE.md: -39 LY is 39 years, and nobody says so).
const LY = '-39 on tonight\'s printout. 1947 on the record.';
const DEVELOPMENT = 'Compare the arrangement, not just the amended explanation. Which reference appears in the original and is omitted from the service copy?';
const RULES: BookRule[] = [
  { a: 'e07.abc', b: 'e06.omit', note: 'n.c', text: NOTES['n.c'].text },
  { a: 'e07.mark', b: 'e06.fault', note: 'n.mark', text: NOTES['n.mark'].text },
  { a: 'e07.sig', b: 'e06.sig', note: 'n.sig', text: NOTES['n.sig'].text },
  { a: 'b12.id', b: 'e08.id', note: 'n.id', text: NOTES['n.id'].text },
  { a: 'b12.mark', b: 'e08.mark', note: 'n.mark2', text: NOTES['n.mark2'].text },
  { a: 'b12.site', b: 'e08.dest', note: 'n.dest', text: NOTES['n.dest'].text },
  { a: 'e07.head', b: 'e08.id', text: 'The sleeve belongs with the 1947 record: same survey, same year.' },
  { a: 'e07.head', b: 'b12.id', text: 'Today\'s B-12 vane keeps the survey from the 1947 record.' },
  { a: 'e06.fault', b: 'e07.abc', text: DEVELOPMENT, wrong: 'P04' },
  { a: 'e06.fault', b: 'e07.head', text: DEVELOPMENT, wrong: 'P04' },
  { a: 'e06.fault', b: 'e07.sig', text: 'The shared signature identifies the report. It does not establish the author\'s motive or responsibility. Which reference changes between the two versions?', wrong: 'P04' },
  { a: 'b12.mark', b: 'e08.elev', text: ELEVATION, wrong: 'P05' },
  { a: 'rx.dist', b: 'e08.id', text: LY },
  { a: 'rx.dist', b: 'e07.head', text: LY },
];
const questionOf = (a: string, b: string) => [a, b].some((x) => /^(b12|e08|rx)\./.test(x)) ? 'P05' : 'P04';

const WHERE: Record<P.SourceKey, string> = {
  original: 'The 1947 field records are in a box on the west shelves, marked STATION 01.',
  amended: 'The service copies are in a binder on the bookcase by the south wall.',
  lineage: 'The maintenance lineage cards are in the card index on the east wall.',
  index: 'The archive sleeve is in the same box as the 1947 field record.',
};


const inRoom = (room: ControlRoom, p: THREE.Vector3) => { const b = room.bounds; return p.x >= b.minX && p.x <= b.maxX && p.z >= b.minZ && p.z <= b.maxZ; };

export class Chapter2 {
  active = false;
  started = false;          // part of the saved case once begun
  ringing = false;
  s: Ch2State = fresh();
  onEnd?: () => void;
  /** Nothing else of the night's going (main.ts, oneAtATime): Ward's second call waits for it. */
  quiet: (() => boolean) | null = null;
  /** The array has been put back on the schedule at least once tonight (story/arrayBack.ts). */
  arrayTried: (() => boolean) | null = null;
  /** The ringing is Ward calling back about the array; seconds until she does (-1: not to come),
   *  and how long it has rung. */
  ringBack = false;
  private backT = -1;
  private backRing = 0;
  private images: { original?: string; amended?: string; index?: string; icons?: Record<Marker, string> } = {};
  /** P04 and P05 on the case board: the records' lines, the questions and the rules. */
  readonly book: RuleBook;

  constructor(private g: Chapter2Host, private d: Chapter2Deps) {
    this.interactables();
    this.book = new RuleBook({
      key: 'ch2',
      owns: (n) => /^(e07|e06|b12|e08|rx)\./.test(n) || n === 'P04' || n === 'P05',
      notes: NOTES, rules: RULES,
      questions: [
        { id: 'P04', stamp: 'P04', text: 'What changed in the 1947 record after the run?', slots: 2, hints: HINTS_04, x: 14, y: 452, w: 272,
          open: () => this.started, done: () => this.s.p04 ? SUPPORTED_04 : null, record: () => this.compare('omitted-c') },
        { id: 'P05', stamp: 'P05', text: 'Where does the B-12 reference lead in the field?', slots: 3, hints: HINTS_05, x: 898, y: 14, w: 266,
          open: () => this.started, done: () => this.s.p05 ? SUPPORTED_05 : null, record: () => this.route('old-survey-station', 'STATION 01', 'triangle-bar') },
      ],
      links: () => (this.s.links ??= []),
      wrong: (q) => { if (q === 'P04') this.s.wrong04++; else this.s.wrong05++; },
      save: () => this.save(),
      cards: () => this.boardCards(),
      questionOf,
      empty: 'Those two lines do not say anything about each other.',
      // the first board kept threads to the question cards and its notes
      legacy: (k) => /(^|\|)(P0|n\.)/.test(k),
    });
  }

  // ---------- lifecycle ----------
  begin(saved: Ch2State | null) {
    this.reset();
    this.active = true; this.started = true;
    this.s = saved ? { ...fresh(), ...saved, read: { ...fresh().read, ...saved.read } } : fresh();
    this.applyWorld();
    if (this.s.stage === 'ward-call') this.ring();
    // a case kept between her two calls: she calls back once the player is in the control room
    else if (this.s.answered && !this.s.arrayAsk) this.backT = 6;
    this.d.save();
  }

  reset() {
    const { audio } = this.d;
    this.active = false; this.started = false; this.ringing = false;
    this.ringBack = false; this.backT = -1; this.backRing = 0;
    this.s = fresh();
    if (audio.ctx) audio.stop('ring2');
  }

  private applyWorld() {
    const s = this.s;
    if (s.door) this.d.doors.set('south', true, true); // a case from before the doors were free
    if (s.read.original) this.file(this.docOriginal());
    if (s.read.index) this.file(this.docIndex());
    if (s.read.amended) this.file(this.docAmended());
    if (s.read.lineage) this.file(this.docLineage());
    if (s.p04) this.file(this.docFinding04());
    if (s.p05) { this.file(this.docFinding05()); this.file(this.docAccess()); this.file(this.docMap()); }
  }

  private file(doc: DocSpec) {
    const i = this.g.docs.findIndex((x) => x.id === doc.id);
    if (i >= 0) { this.g.docs[i] = doc; return false; }
    this.g.docs.push(doc);
    return true;
  }
  private save() { this.d.save(); }
  private pos(o: THREE.Object3D) { return o.getWorldPosition(new THREE.Vector3()); }
  private time() { return clockText(this.g.clock, false); }

  // ---------- objective and notebook ----------
  objective(): string {
    const s = this.s;
    switch (s.stage) {
      case 'ward-call': return 'CONTROL ROOM // THE SUPERVISOR LINE IS RINGING';
      case 'records':
        if (!s.entered) return 'SARO RECORDS // SOUTH CORRIDOR: FIND WHERE THE B-12 REFERENCE CAME FROM';
        if (!s.p04) return 'SARO RECORDS // COMPARE THE ORIGINAL AND AMENDED FIELD RECORDS';
        return 'SARO RECORDS // MATCH THE B-12 LINEAGE TO THE ARCHIVE INDEX';
      case 'call-nora': return 'STATION 01 // CALL THE KEY HOLDER FROM THE RECORDS ROOM PHONE';
      case 'complete': return 'STATION 01 // FIELD ACCESS PREPARED';
    }
  }

  tasks() {
    const s = this.s;
    const t = [
      { text: 'Answer the supervisor\'s call', done: s.answered },
      { text: 'Find the reference record (south corridor)', done: s.entered },
    ];
    if (s.entered) t.push(
      { text: 'Read the 1947 field record (E07)', done: s.read.original },
      { text: 'Read the amended service copy (E06)', done: s.read.amended },
      { text: 'Read the B-12 reference lineage card', done: s.read.lineage },
      { text: 'Read the archive sleeve index (E08)', done: s.read.index },
      { text: 'Record what changed in 1947 (P04)', done: s.p04 },
      { text: 'Record a supported field destination (P05)', done: s.p05 },
    );
    if (s.p05) t.push({ text: 'Call the key holder for STATION 01', done: s.called });
    if (s.arrayAsk) t.push({ text: 'Put the array back on the schedule (operations terminal)', done: this.arrayTried?.() ?? false });
    return t;
  }

  // ---------- Ward ----------
  private ring() {
    const { audio, room, ui } = this.d;
    this.ringing = true;
    audio.loop('ring2', 'phoneRing', { gain: 1.0, at: this.pos(room.objs.phone) });
    this.g.after(1.0, () => ui.toast('The phone again. The supervisor\'s line.', 3));
  }
  phoneLabel() { return this.ringing ? 'Answer the phone' : this.s.called || this.s.answered ? 'Telephone' : null; }
  usePhone() {
    const { audio, ui, room } = this.d;
    if (!this.ringing) { ui.toast(this.s.answered ? 'Ward said what she had to say. The records room is down the south corridor.' : 'Internal line and one outside line.'); return; }
    if (this.ringBack) { this.answerBack(); return; }
    this.ringing = false;
    audio.stop('ring2', 0.02);
    room.handset.visible = false;
    this.g.cinematic = true;
    const spoken = audio.phoneCall(PHONE_CALLS.ward);
    if (spoken) {
      for (const cue of spoken.cues) this.g.after(cue.at, () => ui.toast(`${cue.by}: "${cue.text}"`, Math.max(2, cue.duration)));
    } else {
      audio.callLine(10.2);
      const cap = (at: number, text: string, secs: number) => this.g.after(at, () => ui.toast(text, secs));
      cap(0.3, 'WARD: "That is enough to hold the morning series. It is not enough to tell us why it happened."', 4.6);
      cap(5.0, 'WARD: "Check the reference record. If the instrument inherited the fault, the record may tell us from where."', 5.0);
    }
    this.g.after(spoken ? spoken.duration + 0.1 : 10.3, () => {
      room.handset.visible = true;
      this.g.cinematic = false;
      this.s.answered = true;
      this.s.stage = 'records';
      this.g.note(`${this.time()}. Dr. Ward read the B-12 report and is holding the morning series. Check the reference record: where did the B-12 reference come from?`);
      ui.toast('Ward hangs up. The records room is down the south corridor.', 3.6);
      if (!this.s.arrayAsk) this.backT = 5;
      this.save();
    });
  }
  // ---------- Ward again: the array (story/arrayBack.ts) ----------
  private callBack() {
    const { audio, room, ui } = this.d;
    this.ringing = true; this.ringBack = true; this.backRing = 0; this.backT = -1;
    audio.loop('ring2', 'phoneRing', { gain: 1.0, at: this.pos(room.objs.phone) });
    ui.toast('The supervisor\'s line again.', 3);
  }
  private answerBack() {
    const { audio, ui, room } = this.d;
    this.ringing = false; this.ringBack = false;
    audio.stop('ring2', 0.02);
    room.handset.visible = false;
    this.g.cinematic = true;
    const spoken = audio.phoneCall(WARD_ARRAY);
    if (spoken) {
      for (const cue of spoken.cues) this.g.after(cue.at, () => ui.toast(`${cue.by}: "${cue.text}"`, Math.max(2, cue.duration)));
    } else {
      audio.callLine(7.6);
      this.g.after(0.3, () => ui.toast(`WARD: "${WARD_ARRAY[0].text}"`, 2.4));
      this.g.after(2.8, () => ui.toast(`WARD: "${WARD_ARRAY[1].text}"`, 4.6));
    }
    this.g.after(spoken ? spoken.duration + 0.1 : 7.7, () => {
      room.handset.visible = true;
      this.g.cinematic = false;
      this.s.arrayAsk = true;
      this.g.note(`${this.time()}. Ward called back. "Put the array back on the schedule and tell me it stays there." The operations terminal on the west desk: ARRAY RESUME.`);
      ui.toast('She hangs up. The operations terminal is on the west desk.', 3.4);
      this.save();
    });
  }

  // ---------- the corridor door ----------
  // The doors are free (world/Doors.ts); the case remembers that this one was opened.
  doorChanged(id: string, open: boolean) {
    if (!this.active || id !== 'south' || !open || this.s.door) return;
    this.s.door = true;
    this.save();
  }


  // ---------- the records room ----------
  private interactables() {
    const { inter, ui, annex } = this.d;
    const o = annex.objs;
    const on = () => this.active;
    inter.add({ id: 'fieldBox', object: o.fieldBox, label: () => on() ? 'STATION 01 field records, 1947' : null, use: () => this.takeFieldBox() });
    inter.add({ id: 'binder', object: o.binder, label: () => on() ? 'Service copies, reference records' : null, use: () => this.takeBinder() });
    inter.add({ id: 'lineage', object: o.lineage, label: () => on() ? 'Maintenance card index' : null, use: () => this.takeLineage() });
    inter.add({ id: 'workTable', object: o.workTable, label: () => on() ? (this.anyRead() ? 'Lay out the records' : 'Work table') : null, use: () => this.openTable() });
    inter.add({ id: 'recPhone', object: o.recPhone, label: () => on() ? (this.s.stage === 'call-nora' ? 'Call the key holder' : 'Wall phone') : null, use: () => this.useRecPhone() });
    inter.add({ id: 'vending', object: o.vending, label: () => on() ? 'Vending machine' : null, use: () => ui.toast('Cola, orange soda, peanut butter crackers. Somebody\'s dime is stuck in the coin return.') });
    inter.add({ id: 'officeDoor', object: o.officeDoor, label: () => on() ? 'Operations office' : null, use: () => ui.toast('Locked. Dale took the key home.') });
    inter.add({ id: 'restroomDoor', object: o.restroomDoor, label: () => on() ? 'Restrooms' : null, use: () => ui.toast('Not now.') });
  }
  private anyRead() { const r = this.s.read; return r.original || r.amended || r.lineage || r.index; }

  private takeFieldBox() {
    const { ui } = this.d;
    const first = !this.s.read.original;
    this.s.read.original = true;
    this.file(this.docOriginal());
    ui.document(this.docOriginal(), () => {
      if (!this.s.read.index) {
        this.s.read.index = true;
        this.file(this.docIndex());
        ui.toast('The same box holds the archive sleeve for the 1947 plate: an index card and a fixed-point sheet. Filed in the case.', 4.4);
      }
      if (first) this.g.note(`${this.time()}. The STATION 01 box: the original 1947 field record, signed N. Vega, and the archive sleeve for the plate.`);
      this.save();
    });
  }
  private takeBinder() {
    const { ui } = this.d;
    const first = !this.s.read.amended;
    this.s.read.amended = true;
    this.file(this.docAmended());
    ui.document(this.docAmended(), () => {
      if (first) this.g.note(`${this.time()}. The service copy of the same 1947 record. Same title, same signature. It calls the mark a development fault.`);
      this.save();
    });
  }
  private takeLineage() {
    const { ui } = this.d;
    const first = !this.s.read.lineage;
    this.s.read.lineage = true;
    this.file(this.docLineage());
    ui.document(this.docLineage(), () => {
      if (first) this.g.note(`${this.time()}. Maintenance card for B-12: the vane keeps an older survey, STATION 01.`);
      this.save();
    });
  }

  /** The case board, laid out on the archive table once a record is on it. */
  openBoard() { this.d.board.open(); }

  private openTable() {
    const { ui } = this.d;
    if (!this.anyRead()) { ui.toast('Nothing on the table yet. The records are on the shelves, the bookcase and in the card index.', 4); return; }
    this.d.board.open('SARO ARCHIVE', 'ch2');
  }

  // The five records on the board, with the lines that matter on each. Each picture on its
  // own: reading E07, E06 or E08 on the shelf already drew that one, and the mark icons were
  // then never drawn, which left P05 an empty page.
  private boardCards(): BoardCard[] {
    const s = this.s, im = this.images;
    if (!this.started) return [];
    im.original ??= D.arrangement(true);
    im.amended ??= D.arrangement(false);
    im.index ??= D.sleeveMark();
    im.icons ??= { 'triangle-bar': D.markIcon('triangle-bar'), triangle: D.markIcon('triangle'), 'three-bars': D.markIcon('three-bars') };
    return [
      { id: 'e07', doc: 'e07', stamp: 'E07', title: 'ORIGINAL FIELD RECORD', have: s.read.original, where: WHERE.original, image: im.original, full: ORIGINAL, x: 14, y: 14, w: 272, lines: [
        { id: 'e07.head', text: 'FIELD SURVEY RECORD / STATION 01 / 1947' },
        { id: 'e07.mark', text: 'The retained mark did not follow the new position of the physical vane.' },
        { id: 'e07.abc', text: 'Three references: A, the fixed survey point; B, the optical comparison vane; C, the closing sight line.' },
        { id: 'e07.sig', text: 'N. VEGA / FIELD TECHNICIAN' }] },
      { id: 'e06', doc: 'e06', stamp: 'E06', title: 'AMENDED SERVICE COPY', have: s.read.amended, where: WHERE.amended, image: im.amended, full: AMENDED, x: 298, y: 14, w: 272, lines: [
        { id: 'e06.fault', text: 'The additional mark is attributed to a fault in plate development.' },
        { id: 'e06.omit', text: 'The incomplete closing sight line has been omitted from the service copy.' },
        { id: 'e06.repeat', text: 'No repeat observation is required.' },
        { id: 'e06.sig', text: 'N. VEGA / FIELD TECHNICIAN' }] },
      { id: 'rx', doc: 'printout', stamp: 'RX', title: 'PRINTOUT, 02:14', have: true, x: 898, y: 640, w: 266, lines: [
        { id: 'rx.dist', text: 'SOURCE DISTANCE:  -39 LY' }] },
      { id: 'b12', doc: 'b12lineage', stamp: 'B-12', title: 'LINEAGE CARD', have: s.read.lineage, where: WHERE.lineage, full: LINEAGE, x: 600, y: 14, w: 278, lines: [
        { id: 'b12.id', text: 'The B-12 vane retains the fixed-point survey identified as STATION 01.' },
        { id: 'b12.mark', text: 'Register mark: outlined triangle with a short bar beneath it.', icon: im.icons['triangle-bar'] },
        { id: 'b12.site', text: 'SITE REGISTER: STATION 01 / OLD SURVEY STATION.' }] },
      { id: 'e08', doc: 'e08', stamp: 'E08', title: 'ARCHIVE SLEEVE', have: s.read.index, where: WHERE.index, image: im.index, full: INDEX, x: 600, y: 274, w: 278, lines: [
        { id: 'e08.id', text: 'PLATE / 1947 / STATION 01' },
        { id: 'e08.mark', text: 'Fixed-point mark: outlined triangle with a short bar beneath it.', icon: im.icons['triangle-bar'] },
        { id: 'e08.dest', text: 'Field destination: OLD SURVEY STATION.' },
        { id: 'e08.elev', text: 'A similar triangle without the bar is a general elevation symbol.', icon: im.icons.triangle }] },
    ];
  }

  // P04: the Unity rules and replies.
  private compare(finding: Finding): { ok: boolean; text: string } {
    const s = this.s;
    if (!s.read.original || !s.read.amended) return { ok: false, text: 'Read both the original protocol and amended copy before recording a comparison.' };
    if (finding !== 'omitted-c') {
      s.wrong04++; this.save();
      return { ok: false, text: finding === 'author-guilt'
        ? 'The shared signature identifies the report. It does not establish the author\'s motive or responsibility. Which reference changes between the two versions?'
        : 'Compare the arrangement, not just the amended explanation. Which reference appears in the original and is omitted from the service copy?' };
    }
    if (!s.p04) {
      s.p04 = true; s.finding = finding;
      this.file(this.docFinding04());
      this.g.note(`${this.time()}. P04: the 1947 record was changed after the run. The service copy leaves out C, the closing sight line, and calls the retained mark a development fault. That is a changed report, not a motive.`);
      this.d.audio.play('click', { gain: 0.5 });
      this.save();
    }
    return { ok: true, text: SUPPORTED_04 };
  }

  // P05: the Unity rules and replies, checked in the same order.
  private route(destination: string, surveyId: string, marker: Marker): { ok: boolean; text: string } {
    const s = this.s;
    if (!s.p04) return { ok: false, text: 'Record what changed in the two field records before preparing the field destination.' };
    if (!s.read.lineage || !s.read.index) return { ok: false, text: 'Read both the B-12 lineage card and archive sleeve index. A place name alone does not establish the connection.' };
    const wrong = (text: string) => { s.wrong05++; this.save(); return { ok: false, text }; };
    if (surveyId !== 'STATION 01') return wrong('Match the survey ID in both sources. -39 LY is not a survey ID.');
    if (marker !== 'triangle-bar') return wrong('The matching fixed-point mark has a short bar beneath the outlined triangle. A triangle without the bar is only an elevation symbol.');
    if (destination !== 'old-survey-station') return wrong('The B-12 service lineage and matching archive sleeve identify OLD SURVEY STATION. The documents do not establish another destination.');
    if (!s.p05) {
      s.p05 = true; s.destination = destination; s.surveyId = surveyId; s.marker = marker;
      s.stage = 'call-nora';
      this.file(this.docFinding05()); this.file(this.docAccess()); this.file(this.docMap());
      this.g.note(`${this.time()}. P05: B-12 keeps the STATION 01 survey. The sleeve matches the ID and the triangle with a bar. Destination: OLD SURVEY STATION.`);
      this.g.note('Field access sheet: key holder N. VEGA, SIERRA MOTOR COURT, (505) 555-0119. The same name signs both versions of the 1947 record.');
      this.d.audio.play('click', { gain: 0.5 });
      this.d.ui.toast('Field access sheet filed. The key holder has a phone number.', 4);
      this.save();
    }
    return { ok: true, text: SUPPORTED_05 };
  }

  // ---------- N. Vega ----------
  private useRecPhone() {
    const { ui, audio, annex } = this.d;
    const s = this.s;
    if (s.stage !== 'call-nora') {
      ui.toast(s.called ? 'You could call back. You would get the same answer.' : 'Wall phone. Internal line and one outside line.');
      return;
    }
    this.g.cinematic = true;
    audio.play('click', { gain: 0.5, at: this.pos(annex.objs.recPhone) });
    ui.toast('(505) 555-0119.', 2.4);
    const dial = audio.dial(PHONE);
    const answered = audio.ringback(2, dial + 0.5) + 0.2;
    const finish = () => {
      this.g.cinematic = false;
      s.called = true; s.stage = 'complete';
      // (the chapter ends: Ward does not call back about the array now; the console still has it)
      if (this.ringBack) { this.ringing = false; this.ringBack = false; this.d.audio.stop('ring2', 0.3); }
      this.backT = -1;
      this.file(this.docMap());
      this.g.note(`${this.time()}. Called the key holder. N. Vega, Sierra Motor Court. She wants me at the cut cable at STATION 01 before I come to her, and she wants the originals, not copies.`);
      ui.toast('She hangs up before you can ask anything else.', 3.4);
      this.save();
      this.g.after(3.6, () => this.onEnd?.());
    };
    const beginCall = () => {
      const spoken = audio.phoneCall(PHONE_CALLS.nora);
      if (spoken) {
        for (const cue of spoken.cues) this.g.after(cue.at, () => ui.toast(cue.aside ? cue.text : `${cue.by}: "${cue.text}"`, Math.max(2, cue.duration)));
        this.g.after(spoken.duration + 0.1, finish);
        return;
      }
      audio.callLine(13.6);
      const cap = (at: number, text: string, secs: number) => this.g.after(at, () => ui.toast(text, secs));
      cap(0.4, 'N. VEGA: "Sierra Motor Court."', 2.4);
      cap(2.8, 'You give your name and SARO, and read her the survey number from the sleeve.', 3.4);
      cap(6.2, 'N. VEGA: "Station One. Nobody has asked me for that key in a long time."', 3.6);
      cap(9.6, 'N. VEGA: "Don\'t send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals."', 4.4);
      this.g.after(13.8, finish);
    };
    if (audio.kinskiCalls || audio.characterVoices) this.g.after(answered, beginCall);
    else {
      const talk = 13.6;
      this.g.after(answered, () => audio.callLine(talk));
      const cap = (at: number, text: string, secs: number) => this.g.after(answered + at, () => ui.toast(text, secs));
      cap(0.4, 'N. VEGA: "Sierra Motor Court."', 2.4);
      cap(2.8, 'You give your name and SARO, and read her the survey number from the sleeve.', 3.4);
      cap(6.2, 'N. VEGA: "Station One. Nobody has asked me for that key in a long time."', 3.6);
      cap(9.6, 'N. VEGA: "Don\'t send me copies. Go out to the station first and look at the cable, where it was cut. Then bring me the originals."', 4.4);
      this.g.after(answered + talk + 0.2, finish);
    }
  }

  // ---------- documents ----------
  private docOriginal(): DocSpec {
    return { id: 'e07', title: 'E07 / Field survey record, 1947', kind: 'typed', stamp: 'E07', page: ORIGINAL, image: this.images.original ?? (this.images.original = D.arrangement(true)),
      transcript: 'The original field protocol from the STATION 01 box, signed N. Vega. The arrangement drawing is clipped to it: A, the fixed survey point; B, the comparison vane; C, the closing sight line.' };
  }
  private docAmended(): DocSpec {
    return { id: 'e06', title: 'E06 / Amended service copy, 1947', kind: 'typed', stamp: 'E06', page: AMENDED, image: this.images.amended ?? (this.images.amended = D.arrangement(false)),
      transcript: 'The service copy of the same record, filed with the routine reference records. Same title, same signature. Its own arrangement drawing is clipped to the back.' };
  }
  private docLineage(): DocSpec {
    return { id: 'b12lineage', title: 'B-12 / Reference lineage card', kind: 'typed', stamp: 'B-12', page: LINEAGE,
      transcript: 'A card from the maintenance index. It ties today\'s B-12 vane to an older survey, and names the place in the site register.' };
  }
  private docIndex(): DocSpec {
    return { id: 'e08', title: 'E08 / Archive sleeve index', kind: 'typed', stamp: 'E08', page: INDEX, image: this.images.index ?? (this.images.index = D.sleeveMark()),
      transcript: 'From the same box: the archive sleeve for the 1947 plate, with its index card and the fixed-point sheet. This is the index, not a photograph you took.' };
  }
  private docFinding04(): DocSpec {
    return { id: 'p04', title: 'P04 / The amended record', kind: 'typed', stamp: 'P04', page: SUPPORTED_04,
      transcript: 'Your finding at the records table. It establishes that the report was changed. It does not establish why, or who cut anything.' };
  }
  private docFinding05(): DocSpec {
    return { id: 'p05', title: 'P05 / Field destination', kind: 'typed', stamp: 'P05', page: SUPPORTED_05,
      transcript: 'Your second finding: the reference in today\'s B-12 leads to the old survey station, by survey ID and fixed-point mark together.' };
  }
  private docAccess(): DocSpec {
    return { id: 'access', title: 'STATION 01 / Field access sheet', kind: 'typed', stamp: 'ACCESS', page: ACCESS,
      transcript: 'Folded into the matching sleeve. The key holder for the old station is listed by initial and surname, at the motel down the road: N. Vega. The same name signs both versions of the 1947 record.' };
  }
  private docMap(): DocSpec {
    return { id: 'casemap', title: 'Case map / SARO and field sites', kind: 'map', page: '', image: D.caseMap({ station: this.s.p05, motel: this.s.called }),
      transcript: this.s.called
        ? 'Sketched from the site register. SARO, the old survey station along a dashed road, and now the motel where the key holder answered. Not to scale.'
        : 'Sketched from the site register. SARO, and the old survey station along a dashed road. Not to scale.' };
  }

  // ---------- per frame ----------
  update(dt: number, _t: number) {
    if (!this.active) return;
    const { room, ui, player, annex } = this.d;
    const z = annex.zone.records, p = player.pos;
    if (!this.s.entered && p.x >= z.minX && p.x <= z.maxX && p.z >= z.minZ && p.z <= z.maxZ) {
      this.s.entered = true;
      this.g.note(`${this.time()}. The records room. Steel shelves, three filing cabinets, a card index for maintenance. A box marked STATION 01 sticks out of the west shelves.`);
      ui.toast('SARO records room.', 2.4);
      this.save();
    }
    // Ward calls back about the array a few seconds after her first call, with the player in the
    // control room and nothing else going; let ring, it stops, and she tries again later
    if (this.ringBack && this.ringing && (this.backRing += dt) > BACK_RING) {
      this.ringing = false; this.ringBack = false; this.backT = BACK_AGAIN;
      this.d.audio.stop('ring2', 0.3);
      if (inRoom(room, p)) ui.toast('The ringing stops.', 2.4);
    } else if (!this.ringing && this.backT >= 0 && !this.s.arrayAsk && !this.g.cinematic && this.s.stage !== 'complete') {
      if (this.backT > 0) this.backT = Math.max(0, this.backT - dt);
      else if (inRoom(room, p) && (this.quiet?.() ?? true)) this.callBack();
    }
    ui.objective(this.objective());
  }
}
