import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { Doors } from '../world/Doors';
import type { Room6 } from '../world/Room6';
import type { CourtSite } from '../world/Crossing';
import type { Chapter1 } from './Chapter1';
import type { Chapter2 } from './Chapter2';
import type { Chapter3, AreaId } from './Chapter3';
import * as P from '../ui/Panels';
import type { BoardCard } from '../ui/Board';
import { RuleBook, type BoardHook, type BookNote, type BookRule } from './boardRules';
import { clockText } from './time';
import { artUrl } from '../core/art';
import { WalkieSound } from '../core/walkie';

// Chapter four, "Room 6" (K4 in the design bible). N. Vega is waiting in room 6 at Sierra
// Motor Court, 650 m down the highway south of SARO (Tom, 15 and 16 October: further from the
// stations). The player drives there in the truck, or walks: out of the fire exit at the end
// of the south corridor and down the highway's shoulder (world/Crossing.ts). Then the evidence.
//   P10  Do tonight's prints carry the same mark as 1947? The two B-12 prints and the
//        original record on the table, and which source the player removed for the control.
//   P11  Why did she rewrite the record? Three parts in any order, each with something
//        she puts on the table: the amended record (her signed correction, E12), T. Vega
//        (his letter, E13) and the cable (his last field card, E11). Then the motive.
//   P12  Is the return line a live conversation? The phone in the room is on the old
//        field line. She asks it a new question and gets the 1947 fragment back.
// P10 is answered in the talk, with the evidence on her table. P11 and P12 are worked out on
// the case board from what she puts on the table (SPILLDESIGN.md: answers are given with
// evidence; the three sentences with one right one are gone).
// The texts follow the bible's story material (A5 to A7, V3 to V5). New in the web
// version: at the end she says where C crossed the highway, which leads to the road south.

export type Stage4 = 'to-motel' | 'room' | 'leave' | 'complete';
type Topic = 'record' | 'tomas' | 'cable';
type Paper = 'card' | 'letter' | 'correction';

export interface Ch4State {
  v: 1;
  stage: Stage4;
  exitOpen: boolean;
  arrived: boolean; greeted: boolean;
  placed: string[];
  shown: boolean;            // the evidence is on the table; her question about the control follows
  p10: boolean; wrong10: number;
  topics: Record<Topic, boolean>;
  tone: 'mild' | 'critical' | null;
  p11: boolean; wrong11: number;
  line: boolean; p12: boolean; wrong12: number;
  told: boolean;
  read: Record<Paper, boolean>;
  seen: { photo: boolean; window: boolean };
  /** Nora has called the truck on the CB (saves from before 16 October have none). */
  cb?: boolean;
  links?: string[];          // this chapter's threads on the case board, 'a|b' sorted
}

// What chapter four needs from the places (main.ts and world/World.ts).
export interface Motel {
  area(): AreaId;
  room(): Room6 | null;
  court(): CourtSite;
  goIn(): void;
  goOut(): void;
  brick(): THREE.Object3D | null;
  /** The truck to the motel's lot (World.driveToMotel). */
  driveTo(): void;
  /** Seconds the truck has been driven on this trip, or null when nobody is driving it. */
  driven(): number | null;
  /** A call on the truck's CB (core/cb.ts); false if it could not be made now. */
  cb(lines: { text: string; who: 'woman'; by: string }[], onEnd: () => void): boolean;
}

export interface Chapter4Host {
  clock: number;
  gt: number;
  notes: string[];
  docs: DocSpec[];
  cinematic: boolean;
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
}

export interface Chapter4Deps {
  ui: UI; audio: AudioSys; player: Player; inter: Interaction;
  doors: Doors;
  ch1: Chapter1; ch2: Chapter2; ch3: Chapter3;
  motel: Motel;
  save: () => void;
  milestone: () => void;
  board: BoardHook;
}

const fresh = (): Ch4State => ({
  v: 1, stage: 'to-motel', exitOpen: false, arrived: false, greeted: false,
  placed: [], shown: false, p10: false, wrong10: 0,
  topics: { record: false, tomas: false, cable: false }, tone: null, p11: false, wrong11: 0,
  line: false, p12: false, wrong12: 0, told: false,
  read: { card: false, letter: false, correction: false }, seen: { photo: false, window: false },
});

// ---------- papers (design bible story material, A5 to A7) ----------
const CARD = 'T. VEGA / LAST OBSERVATION\n\nOpening the lamp circuit removes the source, but not the retained reference. Masking the closing sight line prevents an organised return. These are different results.\n\nThe final reading must be taken from outside the marked reference surface. A completed observation may leave its own reference behind. Do not connect a fresh observer to a run merely to finish an old one.';
const LETTER = 'Nora,\n\nI left the spare key in the mug because you said I would lose it in the sand. You were right about the reference moving. I checked it twice before I wrote it down.\n\nWhen we finish, I owe you breakfast and an apology for calling your first plate a mistake.\n\nT.';
const CORRECTION = 'CORRECTION TO THE AMENDED FIELD RECORD\n\nI removed the closing reference from the service copy. I attributed the additional mark to development because I believed the complete arrangement would be repeated if the original finding remained on the record.\n\nThe interruption was deliberate. Tomás Vega was present in the active reference when I broke the connection. He was not accounted for after the interruption. I cannot replace that fact with an instrument fault.\n\nNORA VEGA / 1986';

const SUPPORTED = {
  p10: 'P10 SUPPORTED / Tonight\'s B-12 control print and the 1947 plate carry the same feature: a reference that stays on the film after its local source is taken away. N. Vega knows it from her own plate in 1947.',
  p11: 'P11 SUPPORTED / N. Vega removed C from the service copy and blamed development because she believed the complete record would let someone repeat the arrangement. She cut the cable herself while T. Vega stood inside the active reference. The record does not say what became of him.',
  p12: 'P12 SUPPORTED / The return line repeats T. Vega\'s 1947 correction word for word. A new question gets the same fragment back, not an answer. It is a retained reference, not a conversation.',
};

const N = 'N. VEGA', YOU = 'YOU', LINE = 'THE LINE';
type Line = P.TalkLine;
const n = (text: string): Line => ({ who: N, text });
const you = (text: string): Line => ({ who: YOU, text });
const aside = (text: string): Line => ({ who: '', text });

// The evidence that can go on the table for P10.
const CHIPS: { id: string; label: string }[] = [
  { id: 'frame01', label: 'PHOTO 01 / S-03 print' },
  { id: 'frame02', label: 'PHOTO 02 / B-12 control' },
  { id: 'e07', label: 'E07 / The 1947 original' },
  { id: 'e06', label: 'E06 / The amended copy' },
  { id: 'frame03', label: 'PHOTO 03 / Fixed point A' },
  { id: 'frame04', label: 'PHOTO 04 / Cable break' },
];

type Mode = 'menu' | 'table' | 'control' | 'tone';

// ---------- the case board ----------
// What her papers and the return line show together with what was found before tonight
// (story/boardRules.ts). Her papers are on the board once she has put them on the table.
const NOTES: Record<string, BookNote> = {
  'n.why': { text: 'She left C out on purpose. She believed the complete record would let someone connect the arrangement again.', q: 'P11' },
  'n.cut4': { text: 'The omission and the cut had the same purpose. She broke the connection herself, with Tomás inside the reference.', q: 'P11' },
  'n.line': { text: 'The line gives back the 1947 correction, word for word, to a new question. It repeats; it does not answer.', q: 'P12' },
  'n.key': { text: 'Her question had an answer: his letter says the key was in the mug. The line did not give it.' },
};
const NOT_HIM = 'His letter says he checked the reference twice. His readings were not the problem.';
const RULES: BookRule[] = [
  { a: 'e12.why', b: 'P04', note: 'n.why', text: NOTES['n.why'].text },
  { a: 'e12.cut', b: 'P08', note: 'n.cut4', text: NOTES['n.cut4'].text },
  { a: 'e12.dev', b: 'e06.fault', text: 'Her own correction says the mark was not a development fault. She wrote that it was one.', wrong: 'P11' },
  { a: 'e13.twice', b: 'e12.why', text: NOT_HIM, wrong: 'P11' },
  { a: 'e13.twice', b: 'P04', text: NOT_HIM, wrong: 'P11' },
  { a: 'r6.line', b: 'e10.carrier', note: 'n.line', text: NOTES['n.line'].text },
  { a: 'r6.line', b: 'hut.rx', note: 'n.line', text: NOTES['n.line'].text },
  { a: 'r6.line', b: 'e13.key', note: 'n.key', text: NOTES['n.key'].text },
  { a: 'r6.line', b: 'e13.twice', text: 'He did not answer the question. It is the same fragment as in the 1947 timing log.', wrong: 'P12' },
  // threads that hold without answering anything
  { a: 'e11.mask', b: 'e06.omit', text: 'Tomás wrote it first: masking the closing sight line prevents an organised return. Then she left C out of the copy.' },
  { a: 'e12.cut', b: 'frame04', text: 'The cut in FRAME 04 is hers.' },
];
const HINTS_11 = ['Why would anyone leave one line out of a record?', 'Her correction says why she removed C. What else did she do that night, for the same reason?', 'Tie her reason in E12 to what changed in the record, P04, and the deliberate interruption in E12 to the cut cable, P08.'];
const HINTS_12 = ['What did the line say, and where have you heard it before?', 'Compare it with the 1947 timing log, or with the receiver in the hut.', 'Tie the return line to the recorded voice in E10.'];

export class Chapter4 {
  active = false;
  started = false;
  s: Ch4State = fresh();
  onEnd?: () => void;
  private lines: Line[] = [];
  private mode: Mode = 'menu';
  private reply: { ok: boolean; text: string } | null = null;
  private panel: { refresh: () => void } | null = null;
  private roomBound = false;
  private courtBound = false;
  private headYaw = 0;
  /** P10 to P12 on the case board: her papers, the return line, the questions and the rules. */
  readonly book: RuleBook;
  /** The crank telephone in the office (story/officePhone.ts, set by main.ts): its label and its use
   *  while it has something of its own (ringing, off the hook); use() false leaves it the prop. */
  officePhone?: { label: () => string | null; use: () => boolean };

  constructor(private g: Chapter4Host, private d: Chapter4Deps) {
    const s = () => this.s;
    this.book = new RuleBook({
      key: 'ch4',
      owns: (n) => /^(e11|e12|e13)\./.test(n) || ['r6.line', 'P10', 'P11', 'P12'].includes(n),
      notes: NOTES, rules: RULES,
      questions: [
        { id: 'P10', stamp: 'P10', text: 'Do tonight\'s prints carry the same mark as 1947?', slots: 0, x: 900, y: 14, w: 266,
          hints: ['She asked for three things on her table.', 'The two prints from B-12 and the original 1947 record, the one with her name on it.', 'Put PHOTO 01, PHOTO 02 and E07 on her table, then tell her which source you took away.'],
          elsewhere: 'Answered at her table in room 6: put the prints and the 1947 original in front of her.',
          open: () => this.started && s().arrived, done: () => s().p10 ? SUPPORTED.p10 : null },
        { id: 'P11', stamp: 'P11', text: 'Why did N. Vega rewrite the 1947 finding?', slots: 2, hints: HINTS_11, x: 610, y: 300, w: 270,
          open: () => this.started && s().p10, done: () => s().p11 ? SUPPORTED.p11 : null, record: () => this.recordP11() },
        { id: 'P12', stamp: 'P12', text: 'What does the return line show?', slots: 1, hints: HINTS_12, x: 900, y: 300, w: 266,
          open: () => this.started && (s().line || s().p12), done: () => s().p12 ? SUPPORTED.p12 : null, record: () => this.recordP12() },
      ],
      links: () => (this.s.links ??= []),
      wrong: (q) => { if (q === 'P11') this.s.wrong11++; else if (q === 'P12') this.s.wrong12++; },
      save: () => this.save(),
      cards: () => this.boardCards(),
      // P10 is made in the talk: the board draws what it rests on
      auto: () => s().p10 ? [{ a: 'P10', b: 'frame02' }, { a: 'P10', b: 'e07.mark' }] : [],
    });
  }

  // Her papers on the board, once she has put them on the table, and the return line once
  // it has been heard. Nothing of hers lies there before she gives it.
  private boardCards(): BoardCard[] {
    if (!this.started) return [];
    const s = this.s;
    const cards: BoardCard[] = [
      { id: 'e12', doc: 'e12', stamp: 'E12', title: 'HER CORRECTION', have: s.topics.record, full: CORRECTION, x: 14, y: 14, w: 300, lines: [
        { id: 'e12.why', text: 'I removed the closing reference from the service copy because I believed the complete arrangement would be repeated if the original finding remained on the record.' },
        { id: 'e12.dev', text: 'I attributed the additional mark to development.' },
        { id: 'e12.cut', text: 'The interruption was deliberate. Tomás Vega was present in the active reference when I broke the connection.' }] },
      { id: 'e13', doc: 'e13', stamp: 'E13', title: 'A LETTER FROM T. VEGA', have: s.topics.tomas, full: LETTER, x: 330, y: 14, w: 266, lines: [
        { id: 'e13.key', text: 'I left the spare key in the mug because you said I would lose it in the sand.' },
        { id: 'e13.twice', text: 'You were right about the reference moving. I checked it twice before I wrote it down.' }] },
      { id: 'e11', doc: 'e11', stamp: 'E11', title: 'HIS LAST FIELD CARD', have: s.topics.cable, full: CARD, x: 330, y: 330, w: 266, lines: [
        { id: 'e11.mask', text: 'Masking the closing sight line prevents an organised return.' },
        { id: 'e11.fresh', text: 'Do not connect a fresh observer to a run merely to finish an old one.' }] },
      { id: 'r6.line', stamp: 'THE LINE', title: 'ROOM 6, TONIGHT', kind: 'thing', have: s.line, x: 610, y: 14, w: 270,
        text: 'She asked it: "Tomás. Where did you leave the spare key?" The line said: "Reference west. No. East. Hold the last reading."' },
    ];
    return cards.filter((c) => c.have);
  }

  // ---------- lifecycle ----------
  begin(saved: Ch4State | null) {
    this.reset();
    this.active = true; this.started = true;
    this.s = saved ? { ...fresh(), ...saved, topics: { ...fresh().topics, ...saved.topics }, read: { ...fresh().read, ...saved.read }, seen: { ...fresh().seen, ...saved.seen } } : fresh();
    // the field roll is developed and filed; the camera has nothing left to do tonight
    this.d.ch1.field = {
      frameNo: () => 'FRAME 05',
      check: () => ({ text: 'THE FIELD ROLL IS DEVELOPED AND FILED', ok: false }),
      expose: () => {},
      rejected: () => {},
    };
    this.bindCourt();
    this.applyWorld();
    if (!saved) {
      this.g.note(`${this.time()}. N. Vega is in room 6 at Sierra Motor Court, 650 metres down the highway south of SARO, on the right. She wants the original record and the prints. The truck is on its pad; on foot it is the fire exit at the west end of the south corridor, and the shoulder.`);
      this.d.ui.toast('Room 6 is at Sierra Motor Court, down the highway south. Take the truck, or walk the shoulder from the fire exit.', 4.6);
    }
    this.d.save();
  }

  reset() {
    this.active = false; this.started = false;
    this.s = fresh(); this.lines = []; this.mode = 'menu'; this.reply = null; this.panel = null;
    const brick = this.d.motel.brick(); if (brick) brick.visible = false;
    this.d.motel.court().setRoom6Light?.(true);
    this.setLeaf(0);
    const room = this.d.motel.room();
    if (room) { for (const k of ['fieldCard', 'letter', 'correction']) { const o = room.objs[k]; if (o) o.visible = false; } room.setLamp(true); }
  }

  // Puts the world in the state of this.s (after begin, and when room 6 has loaded).
  private applyWorld() {
    const s = this.s;
    if (s.exitOpen) this.d.doors.set('exit', true, true); // a case from before the doors were saved
    const brick = this.d.motel.brick(); if (brick) brick.visible = this.d.doors.isOpen('exit');
    this.setLeaf(s.stage === 'complete' ? 0 : 0.35);   // her door stands ajar while she waits
    const room = this.d.motel.room();
    if (room) {
      const o = room.objs;
      if (o.correction) o.correction.visible = s.topics.record;
      if (o.letter) o.letter.visible = s.topics.tomas;
      if (o.fieldCard) o.fieldCard.visible = s.topics.cable;
    }
    if (s.topics.record) this.file(this.docCorrection());
    if (s.topics.tomas) this.file(this.docLetter());
    if (s.topics.cable) this.file(this.docCard());
    if (s.p10) this.file(this.docFinding('p10'));
    if (s.p11) this.file(this.docFinding('p11'));
    if (s.p12) this.file(this.docFinding('p12'));
  }

  saveBlock(): string | null { return null; }

  private save() { this.d.save(); }
  private time() { return clockText(this.g.clock, false); }
  private file(doc: DocSpec) {
    const i = this.g.docs.findIndex((x) => x.id === doc.id);
    if (i >= 0) { this.g.docs[i] = doc; return false; }
    this.g.docs.push(doc);
    return true;
  }
  private pos(o: THREE.Object3D) { return o.getWorldPosition(new THREE.Vector3()); }
  private inRoom() { return this.d.motel.area() === 'room6'; }

  // ---------- objective and notebook ----------
  objective(): string {
    const s = this.s;
    const room = this.inRoom();
    switch (s.stage) {
      case 'to-motel':
        if (!this.d.doors.isOpen('exit') && this.d.motel.area() === 'saro') return 'SIERRA MOTOR COURT // ROOM 6, DOWN THE HIGHWAY SOUTH. THE TRUCK, OR THE FIRE EXIT AT THE END OF THE SOUTH CORRIDOR';
        return 'SIERRA MOTOR COURT // DOWN THE HIGHWAY TO ROOM 6, THE LIT SIGN ON THE RIGHT';
      case 'room':
        if (!room) return 'SIERRA MOTOR COURT // N. VEGA IS STILL WAITING IN ROOM 6';
        if (!s.greeted) return 'ROOM 6 // N. VEGA IS WAITING AT THE TABLE';
        if (!s.p10) return 'ROOM 6 // PUT THE PRINTS AND THE 1947 RECORD ON THE TABLE (P10)';
        if (!s.p11 || !s.p12) {
          const left = (['record', 'tomas', 'cable'] as Topic[]).filter((t) => !s.topics[t]).length;
          if (left) return `ROOM 6 // ASK HER ABOUT THE RECORD, T. VEGA AND THE CABLE (${3 - left} OF 3)${s.p12 ? '' : '. AND THE PHONE'}`;
          if (!s.p11) return 'ROOM 6 // ON THE CASE BOARD: WHY SHE REWROTE THE FINDING (P11)';
          return s.line ? 'ROOM 6 // ON THE CASE BOARD: WHAT THE RETURN LINE SHOWS (P12)' : 'ROOM 6 // ASK ABOUT THE PHONE ON THE NIGHTSTAND (P12)';
        }
        return 'ROOM 6 // ASK HER WHERE C WENT';
      case 'leave': return room ? 'ROOM 6 // THE ROAD SOUTH IS NEXT. LEAVE ROOM 6' : 'SIERRA MOTOR COURT // THE ROAD SOUTH IS NEXT';
      case 'complete': return 'ALL NIGHT // NEXT PART NOT YET PLAYABLE';
    }
  }

  tasks() {
    const s = this.s;
    const t = [{ text: 'Go to room 6 at Sierra Motor Court', done: s.arrived }];
    if (s.arrived) t.push(
      { text: 'Put the prints and the 1947 record on the table (P10)', done: s.p10 },
      { text: 'Ask about the amended record', done: s.topics.record },
      { text: 'Ask who T. Vega was', done: s.topics.tomas },
      { text: 'Show her the cable break', done: s.topics.cable },
      { text: 'Work out why she rewrote the finding, on the case board (P11)', done: s.p11 },
      { text: 'Ask about the phone on the nightstand (P12)', done: s.p12 },
    );
    if (s.p10 && s.p11 && s.p12) t.push({ text: 'Ask where C went', done: s.told });
    return t;
  }

  // The door of room 6 on the motel's front (MotelFront's hinged leaf): 0 shut, 1 open.
  private setLeaf(k: number) {
    const leaf = this.d.motel.court().objs.room6DoorLeaf;
    if (leaf) leaf.rotation.y = k * Math.PI / 2;
  }

  // ---------- the truck ----------
  /** This chapter's drive (main.ts asks it, World.truckChoice): down the highway to the motel,
   *  until the player has been to room 6, from wherever the truck stands but the motel's lot. */
  truckTrip(at: string | null): { label: string; use: () => void } | null {
    if (!this.active || this.s.stage !== 'to-motel' || at === 'motel') return null;
    return { label: 'Drive to Sierra Motor Court', use: () => this.d.motel.driveTo() };
  }

  // ---------- the fire exit ----------
  // The doors are free (world/Doors.ts). Opened in this chapter, a brick props the fire exit.
  doorChanged(id: string, open: boolean) {
    if (id !== 'exit') return;
    const brick = this.d.motel.brick();
    if (brick) brick.visible = open;
    if (!this.active || !open || this.s.exitOpen) return;
    this.s.exitOpen = true;
    this.d.ui.toast('The alarm on the push bar was disconnected years ago. A brick on the step holds the door.', 4.2);
    this.save();
  }

  // The motel's front: room 6 (and, with MotelFront.ts, the office).
  private bindCourt() {
    if (this.courtBound) return;
    this.courtBound = true;
    const { inter, ui } = this.d;
    const court = this.d.motel.court();
    const px = court.proxies;
    const on = () => this.d.motel.area() === 'saro';
    if (px.room6Door) inter.add({ id: 'room6Door', object: px.room6Door, range: 2.6,
      label: () => on() ? (this.active && this.s.stage !== 'complete' ? 'Room 6' : 'Room 6, Sierra Motor Court') : null,
      use: () => {
        if (!this.active) { ui.toast('Room 6. There is a light behind the curtains.', 3); return; }
        if (this.s.stage === 'complete') { ui.toast('The light is still on. She said what she had to say.', 3.2); return; }
        this.setLeaf(1);
        this.d.motel.goIn();
      } });
    const extra = (id: string, label: string, text: string) => {
      if (px[id]) inter.add({ id: 'court:' + id, object: px[id], label: () => on() ? label : null, use: () => ui.toast(text, 3.6) });
    };
    extra('officeDoor', 'Motel office', 'The office of Sierra Motor Court. The night bell is on the counter.');
    extra('register', 'Guest register', 'A register in her handwriting. Room 6: N. Vega, every night since the spring.');
    extra('keyBoard', 'Key board', 'Ten hooks with brass tags. The hook for 6 is empty.');
    // the crank telephone: while it rings or is off the hook it is story/officePhone.ts's (main.ts)
    if (px.officePhone) inter.add({ id: 'court:officePhone', object: px.officePhone,
      label: () => on() ? this.officePhone?.label() ?? 'Field telephone' : null,
      use: () => { if (this.officePhone?.use()) return; ui.toast('An oak wall telephone with a crank. The cable runs out through the back wall and on to STATION 01.', 3.6); } });
    // (the envelope on the counter is the field book's, any time of the night: story/fieldBook.ts)
    if (px.message) inter.add({ id: 'court:message', object: px.message, label: () => on() ? 'A note on the counter' : null,
      use: () => {
        ui.toast('In pencil, on the back of a bill: "Room 6. The door is open. N."', 4);
        if (this.active && !this.g.notes.some((x) => x.includes('the back of a bill'))) this.g.note('A note on the office counter, in pencil on the back of a bill: "Room 6. The door is open. N."');
      } });
    extra('otherDoors', 'Motel room', 'Dark. Nobody else is staying tonight.');
    extra('iceMachine', 'Ice machine', 'It hums. Nobody has opened it in a while.');
    extra('car', 'Parked car', 'A dusty sedan with New Mexico plates. Her car, by the look of the survey stakes on the back seat.');
    extra('pool', 'Pool', 'Empty, with a fence round it and leaves in the deep end.');
  }

  // ---------- room 6 ----------
  // Room 6 loads the first time the player goes in; its interactables are added then.
  bindRoom(room: Room6) {
    if (!this.roomBound) {
      this.roomBound = true;
      const { inter, ui } = this.d;
      const px = room.proxies;
      const on = () => this.active && this.inRoom();
      const add = (id: string, key: string, label: () => string | null, use: () => void, range = 2.4) => {
        if (px[key]) inter.add({ id: 'r6:' + id, object: px[key], range, label: () => on() ? label() : null, use });
      };
      add('nora', 'nora', () => 'Talk to N. Vega', () => this.openTalk(), 2.8);
      add('table', 'table', () => 'The table', () => this.openTalk(), 2.4);
      add('card', 'fieldCard', () => this.s.topics.cable ? 'E11 / T. Vega\'s field card' : null, () => this.read('card'));
      add('letter', 'letter', () => this.s.topics.tomas ? 'E13 / The letter' : null, () => this.read('letter'));
      add('correction', 'correction', () => this.s.topics.record ? 'E12 / Her correction' : null, () => this.read('correction'));
      add('photo', 'photo', () => 'Framed snapshot', () => this.look('photo'));
      add('window', 'window', () => 'Window', () => this.look('window'));
      add('phone', 'phone', () => 'Phone on the nightstand', () => this.usePhone());
      add('tv', 'tv', () => room.tvOn ? 'Turn the television off' : 'Turn the television on', () => this.useTv());
      add('bed', 'bed', () => 'Bed', () => ui.toast('Made, and not slept in.', 2.6));
      add('lamp', 'lamp', () => 'Table lamp', () => { room.setLamp(!room.lampOn); this.d.audio.play(this.d.audio.or('small_switch', 'switch'), { gain: 0.3, rate: 1.4 }); });
      add('bathroom', 'bathroom', () => 'Bathroom door', () => ui.toast('Shut. A tap drips behind it.', 2.6));
      add('walkie', 'walkie', () => this.walkieLabel(), () => this.useWalkie());
      add('door', 'door', () => this.s.stage === 'leave' ? 'Leave room 6' : 'Door to the walk', () => this.useDoor(), 2.6);
    }
    if (this.active) this.applyWorld();
  }

  // The set is on when you come in: snow, and its blue light jumping on the walls. Nora
  // keeps it on for the noise. It reacts to nothing (R1); it is just a television at 4 am.
  private tvSeen = false;
  private tvTouched = false;   // until the player switches it, the set is on whenever they come in
  private useTv() {
    const room = this.d.motel.room(); if (!room) return;
    this.tvTouched = true;
    room.setTv(!room.tvOn);
    this.d.audio.play(this.d.audio.or('small_switch', 'switch'), { gain: 0.35, rate: 0.8 });
    this.tvSound();
    if (room.tvOn && !this.tvSeen) { this.tvSeen = true; this.d.ui.toast('Snow on every channel. The stations signed off at one.', 3.2); }
    else if (!room.tvOn) this.noraToast('N. VEGA: "Leave it. The quiet is worse."', 3);
  }
  private tvSound() {
    const room = this.d.motel.room();
    const tv = room?.objs.tv;
    this.d.audio.tvHiss(room && tv && room.tvOn && this.inRoom() ? tv.getWorldPosition(new THREE.Vector3()) : null);
  }

  // Called by World when the player comes in through the door.
  enteredRoom() {
    const s = this.s;
    if (!this.active) return;
    const room = this.d.motel.room();
    if (room && !this.tvTouched) room.setTv(true);
    this.tvSound();
    if (!s.arrived) {
      s.arrived = true;
      if (s.stage === 'to-motel') s.stage = 'room';
      this.g.note(`${this.time()}. Room 6. N. Vega in the chair by the window, a shoebox of papers on the table. She has been waiting a long time; she does not say so.`);
      this.d.ui.toast('N. Vega is sitting by the window.', 3.4);
      this.d.milestone();
    }
  }
  // ---------- Tomás' field radio (KAPITLER.md, Room 6; core/walkie.ts) ----------
  // It plays the road an hour before it happens: an engine dying, the hiss gone, relay
  // clicks in four and seven, someone breathing. Nobody says so. It has no battery.
  private walkie: WalkieSound | null = null;
  private walkieState: 'off' | 'playing' | 'on' | 'sending' = 'off';
  private walkiePressed = false;     // pressed to talk since it was switched on
  private walkieHeard = false;       // Nora's lines come once each
  private walkieSent = false;
  private walkieLabel() {
    if (this.walkieState === 'playing' || this.walkieState === 'sending') return null;
    if (this.walkieState === 'on') return this.walkiePressed ? 'Turn the field radio off' : 'Press to talk';
    return 'Field radio, 1947';
  }
  private useWalkie() {
    const { audio, ui } = this.d;
    const room = this.d.motel.room(), o = room?.objs.walkie;
    if (!o || !audio.ctx) { ui.toast('The switch is stiff. Nothing.', 2.4); return; }
    if (this.walkieState === 'on' && !this.walkiePressed) { this.walkiePress(); return; }
    if (this.walkieState === 'on') { this.walkieOff(); audio.play('click', { gain: 0.35, rate: 0.8, at: o.getWorldPosition(new THREE.Vector3()) }); return; }
    const at = o.getWorldPosition(new THREE.Vector3()).setY(0.95);
    const dest = audio.spatial(at, 0.7), noise = audio.noiseBuffer();
    if (!dest || !noise) return;
    audio.play('click', { gain: 0.45, rate: 0.7, at });
    this.walkie?.stop();
    this.walkie = new WalkieSound(audio.ctx, dest, noise);
    this.walkieState = 'playing'; this.walkiePressed = false;
    if (!this.walkieHeard) ui.toast('The switch is stiff. There is no battery in it.', 3);
    const secs = this.walkie.start();
    this.g.after(secs, () => {
      if (this.walkieState !== 'playing') return;
      this.walkieState = 'on';
      if (!this.walkieHeard) {
        this.walkieHeard = true;
        this.noraToast('N. VEGA: "I took the battery out in 1947."', 3.4);
        this.g.note('Tomás\' field radio, on her table. No battery. An engine far off, coughing, then stopping. Silence. Clicks, four and seven. Someone breathing.');
      }
    });
  }
  private walkiePress() {
    const { audio, ui } = this.d;
    if (!this.walkie) return;
    this.walkieState = 'sending'; this.walkiePressed = true;
    audio.play('click', { gain: 0.5, rate: 1.2 });
    const secs = this.walkie.pressToTalk();
    this.g.after(secs, () => {
      if (this.walkieState !== 'sending') return;
      this.walkieState = 'on';
      if (!this.walkieSent) {
        this.walkieSent = true;
        this.noraToast('N. VEGA: "He pressed it too. Twice."', 3.2);
        this.g.note('I pressed to talk once. Four clicks and seven came back.');
      }
    });
  }
  private walkieOff() {
    this.walkie?.stop();
    this.walkieState = 'off'; this.walkiePressed = false;
  }

  // Called by World when the player has stepped out of room 6.
  leftRoom() {
    this.d.audio.tvHiss(null);
    this.walkieOff();
    if (!this.active) return;
    this.setLeaf(this.s.stage === 'leave' ? 0 : 0.35);
    if (this.s.stage === 'leave') this.finish();
  }

  private useDoor() {
    const s = this.s;
    if (s.stage !== 'leave' && s.arrived && !s.told) this.noraToast('N. VEGA: "Go if you need to. I will be here."', 3.2);
    this.d.motel.goOut();
  }

  private look(what: 'photo' | 'window') {
    const { ui } = this.d;
    const first = !this.s.seen[what];
    this.s.seen[what] = true;
    if (what === 'photo') {
      ui.toast('Two people at a transit on a concrete pier, squinting into the sun. On the back, in pencil: N. and T., STATION 01, June 1947.', 5);
      if (first) this.g.note('A framed snapshot on the dresser: N. and T. Vega at the transit at STATION 01, June 1947.');
    } else ui.toast('Through the gap in the curtains: the Sierra sign, red and cyan, and far up the road the lights of SARO.', 4);
    if (first) this.save();
  }

  private read(which: Paper) {
    const doc = which === 'card' ? this.docCard() : which === 'letter' ? this.docLetter() : this.docCorrection();
    if (!this.s.read[which]) { this.s.read[which] = true; this.save(); }
    this.d.ui.document(doc, () => {});
  }

  private usePhone() {
    const s = this.s;
    if (!s.p10) { this.d.ui.toast('A beige phone. The cord runs to a wall jack marked OFFICE.', 3.2); return; }
    if (s.p12) { this.d.ui.toast('The handset is back on the cradle. The carrier is still there, if you listen for it.', 3.2); return; }
    this.openTalk();
    this.choose('line');
  }

  // ---------- the conversation ----------
  private noraToast(text: string, seconds: number) {
    this.d.ui.toast(text, seconds); this.d.ui.speakNora(text);
  }

  private openTalk() {
    const s = this.s;
    if (this.walkieState !== 'off') this.walkieOff();   // it keeps quiet while she talks
    if (!s.greeted) {
      s.greeted = true;
      this.mode = 'table';
      this.lines = [
        n('So you came down the road.'),
        n('Do not sit yet. Put them on the table first. The two prints from B-12, and the record from 1947. The one with my name on it.'),
      ];
      this.save();
    } else if (this.mode === 'menu' && !this.lines.length) {
      this.lines = [aside(s.told ? 'She is looking out at the road.' : 'She waits for you to go on.')];
    }
    this.reply = null;
    this.panel = P.talk(this.d.ui, {
      title: 'ROOM 6 / N. VEGA',
      character: 'bacall',
      view: () => this.view(),
      onChip: (id) => this.toggleChip(id),
      onTable: () => this.showEvidence(),
      onChoice: (id) => this.choose(id),
      onRead: (id) => this.read(id as Paper),
      onClose: () => { this.d.audio.stopPhoneCalls(); this.panel = null; },
    });
  }

  private have(id: string) {
    const { ch1, ch2, ch3 } = this.d;
    switch (id) {
      case 'frame01': case 'frame02': return ch1.s.filed;
      case 'e07': return ch2.s.read.original;
      case 'e06': return ch2.s.read.amended;
      case 'frame03': case 'frame04': return ch3.s.dev >= 3;
    }
    return false;
  }
  private image(id: string) {
    const { ch1, ch3 } = this.d;
    return ({ frame01: ch1.s.f1?.url, frame02: ch1.s.f2?.url, frame03: ch3.s.f3?.url, frame04: ch3.s.f4?.url } as Record<string, string | undefined>)[id];
  }

  private view(): P.TalkView {
    const s = this.s;
    const v: P.TalkView = { status: this.objective(), lines: this.lines, choices: [], reply: this.reply };
    v.given = [];
    if (s.topics.record) v.given.push({ id: 'correction', label: 'E12 / Her correction' });
    if (s.topics.tomas) v.given.push({ id: 'letter', label: 'E13 / The letter' });
    if (s.topics.cable) v.given.push({ id: 'card', label: 'E11 / The field card' });
    switch (this.mode) {
      case 'table':
        v.table = {
          prompt: 'Put the evidence on the table.',
          chips: CHIPS.map((c) => ({ ...c, image: this.image(c.id), have: this.have(c.id), on: s.placed.includes(c.id) })),
          submit: 'SHOW HER',
        };
        break;
      case 'control':
        v.question = 'Which source did you take away before the control exposure?';
        v.choices = [
          { id: 'lamp', label: 'The work lamp. I shielded it, and the vane motor was isolated.' },
          { id: 'none', label: 'None. I drove the vane to 042 and left the lamp on.' },
          { id: 'receiver', label: 'The receiver bank. I switched it off.' },
        ];
        break;
      case 'tone':
        v.question = 'How do you ask her?';
        v.choices = [
          { id: 'mild', label: '"Why did you change it?"' },
          { id: 'critical', label: '"You falsified the record."' },
        ];
        break;
      case 'menu':
        if (!s.topics.record) v.choices.push({ id: 'record', label: 'Ask about the amended record' });
        if (!s.topics.tomas) v.choices.push({ id: 'tomas', label: 'Ask who T. Vega was' });
        if (!s.topics.cable) v.choices.push({ id: 'cable', label: 'Show her PHOTO 04, the cable break', disabled: !this.have('frame04') });
        if (!s.line) v.choices.push({ id: 'line', label: 'Ask about the phone on the nightstand' });
        {
          // P11 once all three parts are on the table, P12 once the line has been heard
          const open = [...(s.topics.record && s.topics.tomas && s.topics.cable && !s.p11 ? ['P11'] : []), ...(s.line && !s.p12 ? ['P12'] : [])];
          if (open.length) v.choices.push({ id: 'board', label: `Lay it out on the case board (${open.join(', ')})` });
        }
        if (s.p10 && s.p11 && s.p12 && !s.told) v.choices.push({ id: 'end', label: 'Ask where C went' });
        v.choices.push({ id: 'bye', label: s.told ? 'Leave her be' : 'Step back from the table' });
        break;
    }
    return v;
  }

  private toggleChip(id: string) {
    if (!this.have(id)) return;
    const p = this.s.placed;
    const i = p.indexOf(id);
    if (i >= 0) p.splice(i, 1); else p.push(id);
    this.reply = null;
  }

  private showEvidence() {
    const s = this.s;
    const p = s.placed;
    const prints = p.includes('frame01') && p.includes('frame02');
    if (!prints || !p.includes('e07')) {
      s.wrong10++;
      this.reply = { ok: false, text: !prints ? 'N. VEGA: "Both prints from B-12. The first one, and the control."' : 'N. VEGA: "And the original. The one from the field box, not the service copy."' };
      this.save();
      return;
    }
    s.shown = true;
    this.mode = 'control';
    this.reply = null;
    this.lines = [
      aside('She lays the two prints side by side and puts the 1947 record under them.'),
      ...(p.includes('e06') ? [n('The amended copy you can keep. I know what it says. I wrote it.')] : []),
      n('I know that second line.'),
      n('Tell me which source you took away before you made the control.'),
    ];
    this.save();
  }

  choose(id: string) {
    this.d.audio.stopPhoneCalls();
    this.d.ui.npcDialogue?.stop();
    this.reply = null;
    switch (this.mode) {
      case 'control': return this.answerControl(id);
      case 'tone': return this.answerTone(id as 'mild' | 'critical');
    }
    switch (id) {
      case 'record': this.mode = 'tone'; this.lines = [you('About the amended copy.'), aside('She does not look away from it.')]; return;
      case 'tomas': return this.topicTomas();
      case 'cable': return this.topicCable();
      case 'line': return this.topicLine();
      case 'board': this.d.board.open('ROOM 6', 'ch4'); return;
      case 'end': return this.ending();
      case 'bye': this.d.ui.close(); return;
    }
  }

  private answerControl(id: string) {
    const s = this.s;
    const method = this.d.ch1.s.method;
    const right = method === 'passive' ? id === 'lamp' : method === 'active' ? id === 'none' : id !== 'receiver';
    if (!right) {
      s.wrong10++;
      this.reply = { ok: false, text: id === 'receiver'
        ? 'N. VEGA: "The receiver has nothing to do with the film. Look at how you made the second print."'
        : method === 'active'
          ? 'N. VEGA: "That is not what your print says. The lamp was on when you took it."'
          : 'N. VEGA: "That is not what your print says. Your lamp was shielded."' };
      this.save();
      return;
    }
    s.p10 = true;
    this.mode = 'menu';
    this.lines = [
      id === 'lamp' ? n('The lamp. That is what I did in 1947. I opened the lamp circuit, and the mark stayed on the plate.') : n('You moved it, and the second line did not move with it. Then it was not the drive.'),
      n('Then you have what I had. A reference that stays on the film after its source is gone.'),
      aside('P10 recorded. She pushes a chair out for you with her foot.'),
    ];
    this.file(this.docFinding('p10'));
    this.g.note('P10: tonight\'s control print and her 1947 plate carry the same mark, a reference that stays on the film after its source is taken away. She recognised it at once.');
    this.d.audio.play('click', { gain: 0.5 });
    this.d.milestone();
  }

  private answerTone(tone: 'mild' | 'critical') {
    const s = this.s;
    s.tone = tone;
    s.topics.record = true;
    this.mode = 'menu';
    this.lines = [
      you(tone === 'mild' ? 'Why did you change it?' : 'You falsified the record.'),
      tone === 'mild'
        ? n('I thought leaving out one line would stop someone connecting it again. I was wrong about what a missing line could hide.')
        : n('Yes. I changed it. I called it a development fault because I could not write what was left after the cable was cut.'),
      n('I wrote that the lamp remained. It was easier than writing that he did.'),
      n('I typed this in the office tonight, before you came. I have not signed it yet.'),
      aside('She signs it and lays it on the table, on top of the amended copy. E12 filed.'),
    ];
    this.give('correction');
    this.g.note(`N. Vega on the amended record: she left out C so that nobody would connect the arrangement again, and blamed development. She signed a correction tonight (E12). ${tone === 'critical' ? 'I put it to her as falsification. She did not argue.' : ''}`.trim());
  }

  private topicTomas() {
    this.s.topics.tomas = true;
    this.lines = [
      you('Who was T. Vega?'),
      n('Tomás. My brother. He was nineteen that summer, the field assistant, and very sure of himself.'),
      n('He has been nineteen for thirty-nine years.'),
      n('He read the last observation into the field line. You heard him tonight, on the receiver in the hut.'),
      n('He wrote me this the day before. I have never shown it to anyone.'),
      aside('She puts a folded letter on the table. E13 filed.'),
    ];
    this.give('letter');
    this.g.note('T. Vega was Tomás, her brother, the field assistant in 1947. Nineteen. His voice is the one on the receiver. She gave me his letter (E13).');
  }

  private topicCable() {
    this.s.topics.cable = true;
    this.lines = [
      aside('You put PHOTO 04 in front of her.'),
      n('Yes. I cut it.'),
      n('The lamp was off. The reference stayed anyway. Tomás was out on C with the last reading, and the arrangement was answering him.'),
      n('I thought if I broke the connection it would stop. It stopped. He was not there afterwards.'),
      n('This was in his coat. He knew more than I did.'),
      aside('She puts a field card on the table. E11 filed.'),
    ];
    this.give('card');
    this.g.note('She cut the cable in 1947, with Tomás standing on C inside the active reference. Afterwards he was not there. She gave me his last field card (E11).');
  }

  private give(which: Paper) {
    const room = this.d.motel.room();
    const key = which === 'card' ? 'fieldCard' : which;
    const o = room?.objs[key];
    if (o) o.visible = true;
    this.file(which === 'card' ? this.docCard() : which === 'letter' ? this.docLetter() : this.docCorrection());
    this.d.audio.play('thudSoft', { gain: 0.18, rate: 1.8 });
    this.d.milestone();
  }

  // P11, recorded on the case board once both notes are on it. Her answer is waiting in the
  // talk when the player turns back to her.
  private recordP11(): { ok: boolean; text: string } {
    const s = this.s;
    if (!(s.topics.record && s.topics.tomas && s.topics.cable)) return { ok: false, text: 'Hear her out first: the amended record, T. Vega and the cable.' };
    if (s.p11) return { ok: true, text: SUPPORTED.p11 };
    s.p11 = true;
    this.mode = 'menu';
    this.lines = [
      you('You thought the whole record would bring it back.'),
      n('Write it like that. Not cleaner than it was.'),
      aside('P11 recorded.'),
    ];
    this.file(this.docFinding('p11'));
    this.g.note('P11: she rewrote the finding because she believed the complete record would let someone repeat the arrangement. She cut the cable herself, with Tomás inside the reference. Nobody knows what became of him.');
    this.d.audio.play('click', { gain: 0.5 });
    this.d.milestone();
    return { ok: true, text: SUPPORTED.p11 };
  }

  // P12: the phone on the nightstand is on the old field line.
  private topicLine() {
    const { audio, ui } = this.d;
    const s = this.s;
    if (s.line) { this.mode = 'menu'; this.lines = [aside('The handset is back on the cradle.')]; return; }
    const room = this.d.motel.room();
    const at = room?.proxies.phone ? this.pos(room.proxies.phone) : undefined;
    this.lines = [
      you('Where does that phone go?'),
      n('To the office. At night the office line is patched through to this room, and the other end of it is the field hut. It has been since 1947.'),
      n('You left the receiver on out there. Listen.'),
      aside('She lifts the handset and holds it between you. A carrier, very faint, under the hiss.'),
      n('Tomás. Where did you leave the spare key?'),
      { who: LINE, text: 'Reference west. No. East. Hold the last reading.' },
      n('That is what he said then. Not an answer to what I asked now.'),
      aside('She puts the handset down.'),
    ];
    s.line = true;
    this.mode = 'menu';
    // Character mode plays Nora and the recorded Price line through the panel's
    // single actor-aware queue. The legacy whole-exchange phone voice remains optional.
    const spoken = audio.characterVoices ? null : audio.phoneCall(this.lines.filter((l) => l.who).map((l) => ({ by: l.who, text: l.text })));
    if (!audio.characterVoices && !spoken && at) {
      audio.play('click', { gain: 0.5, at });
      audio.radio(at, 0.08);
      audio.radioVoice(4.4, 3.6);
      this.g.after(9.5, () => { audio.radio(null); audio.play('click', { gain: 0.45, at }); });
    }
    this.g.note('The phone in room 6 is on the old field line to the hut. She asked it: "Tomás. Where did you leave the spare key?" The line said: "Reference west. No. East. Hold the last reading." The same words as in 1947.');
    ui.toast('The same words as on the receiver in the hut.', 3);
    this.save();
  }

  // P12, recorded on the case board.
  private recordP12(): { ok: boolean; text: string } {
    const s = this.s;
    if (!s.line) return { ok: false, text: 'Ask her about the phone on the nightstand first.' };
    if (s.p12) return { ok: true, text: SUPPORTED.p12 };
    s.p12 = true;
    this.mode = 'menu';
    this.lines = [
      n('No. It is not him. It is what he left.'),
      aside('P12 recorded.'),
    ];
    this.file(this.docFinding('p12'));
    this.g.note('P12: the return line repeats the 1947 correction unchanged. A new question gets the same fragment. It is a retained reference, not a conversation.');
    this.d.audio.play('click', { gain: 0.5 });
    this.d.milestone();
    return { ok: true, text: SUPPORTED.p12 };
  }

  private ending() {
    const s = this.s;
    s.told = true;
    s.stage = 'leave';
    this.lines = [
      you('Where did C go?'),
      n('East from the station, over the fence, and on across the old highway to Roswell. There is a survey bolt in the shoulder where it crosses.'),
      n('Tomás walked out to the closing point with the last reading.'),
      n('If you want to know whether the line still holds, drive the road.'),
      n('Do not stop on the line.'),
    ];
    this.g.note(`${this.time()}. C ran east from STATION 01 and crosses the old highway to Roswell at a survey bolt in the shoulder. Tomás walked out to the closing point with the last reading. "If you want to know whether the line still holds, drive the road. Do not stop on the line."`);
    this.d.milestone();
  }

  private finish() {
    const s = this.s;
    if (s.stage === 'complete') return;
    s.stage = 'complete';
    this.g.note('ROOM 6: P10 to P12 recorded, with her correction (E12), Tomás\' letter (E13) and his last field card (E11). Next: the road south, where C crosses it.');
    this.d.milestone();
    this.g.after(1.2, () => this.onEnd?.());
  }


  // ---------- documents ----------
  private docCard(): DocSpec {
    return { id: 'e11', title: 'E11 / T. Vega, last observation card', kind: 'hand', stamp: 'E11', page: CARD, paper: artUrl('cardField'), paperRatio: 1.6,
      transcript: 'A field card in pencil, from Tomás Vega\'s coat. She has kept it for thirty-nine years.\n\nThe first half separates opening the lamp from masking C. The second half says how the last reading must be taken, and who should not take it.' };
  }
  private docLetter(): DocSpec {
    return { id: 'e13', title: 'E13 / A letter from T. Vega, 1947', kind: 'hand', stamp: 'E13', page: LETTER, paper: artUrl('letterPaper'), paperRatio: 0.75,
      transcript: 'Tomás to his sister, the day before the last observation. No instrument language. The spare key is the key to the field hut.' };
  }
  private docCorrection(): DocSpec {
    return { id: 'e12', title: 'E12 / Correction to the amended record', kind: 'typed', stamp: 'E12', page: CORRECTION,
      transcript: 'Typed on the office machine earlier tonight and signed at the table, in front of you. It is attached to the amended copy. It does not replace it.' };
  }
  private docFinding(k: 'p10' | 'p11' | 'p12'): DocSpec {
    const map = {
      p10: { title: 'P10 / The same mark', transcript: 'Tonight\'s two B-12 prints beside the 1947 original. The control did not remove the second reference, then or now.' },
      p11: { title: 'P11 / Why the record was changed', transcript: 'The omission of C and the cut cable had the same purpose. Neither says what became of Tomás.' },
      p12: { title: 'P12 / The return line', transcript: 'A new question on the old field line gets the 1947 fragment back, unchanged.' },
    }[k];
    return { id: k, kind: 'typed', stamp: k.toUpperCase(), page: SUPPORTED[k], ...map };
  }

  // ---------- per frame ----------
  update(_dt: number, _t: number) {
    if (!this.active) return;
    // on the way down the highway in the truck, Nora calls it on the CB (Tom's improvements,
    // 16 October: the CB did nothing). Once; it is in the notebook after.
    const driven = this.d.motel.driven();
    if (this.s.stage === 'to-motel' && !this.s.cb && driven !== null && driven > 9) {
      const by = 'N. VEGA';
      const ok = this.d.motel.cb([
        { text: 'SARO seven, this is Sierra. You hear me?', who: 'woman', by },
        { text: 'Pull in at the north end, by the office. Not down past the rooms.', who: 'woman', by },
        { text: 'Room six. The door is open. Bring the prints.', who: 'woman', by },
      ], () => {
        this.g.note(`${this.time()}. N. Vega on the truck's CB, on the way down the highway: pull in by the office, room six, the door is open, bring the prints.`);
        this.save();
      });
      if (ok) this.s.cb = true;
    }
    // N. Vega turns her head to whoever is in the room with her
    const room = this.d.motel.room();
    const head = room?.objs.noraHead;
    if (head && this.inRoom() && head.parent) {
      const p = head.parent.worldToLocal(this.d.player.pos.clone().setY(1.2));
      const want = THREE.MathUtils.clamp(Math.atan2(-(p.x - head.position.x), -(p.z - head.position.z)), -0.9, 0.9);
      this.headYaw += (want - this.headYaw) * Math.min(1, _dt * 3);
      head.rotation.y = this.headYaw;
    }
    this.d.ui.objective(this.objective());
  }
}
