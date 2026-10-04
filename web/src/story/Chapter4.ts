import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { Collider } from '../world/ControlRoom';
import type { RecordsAnnex } from '../world/Annex';
import type { Room6 } from '../world/Room6';
import type { CourtSite } from '../world/Crossing';
import type { Chapter1 } from './Chapter1';
import type { Chapter2 } from './Chapter2';
import type { Chapter3, AreaId } from './Chapter3';
import * as P from '../ui/Panels';
import { clockText } from './time';

// Chapter four, "Room 6" (K4 in the design bible). N. Vega is waiting in room 6 at Sierra
// Motor Court, across the highway from SARO. The player walks there from the fire exit at
// the end of the south corridor (world/Crossing.ts) and shows her the evidence.
//   P10  Do tonight's prints carry the same mark as 1947? The two B-12 prints and the
//        original record on the table, and which source the player removed for the control.
//   P11  Why did she rewrite the record? Three parts in any order, each with something
//        she puts on the table: the amended record (her signed correction, E12), T. Vega
//        (his letter, E13) and the cable (his last field card, E11). Then the motive.
//   P12  Is the return line a live conversation? The phone in the room is on the old
//        field line. She asks it a new question and gets the 1947 fragment back.
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
}

// What chapter four needs from the places (main.ts and world/World.ts).
export interface Motel {
  area(): AreaId;
  room(): Room6 | null;
  court(): CourtSite;
  goIn(): void;
  goOut(): void;
  brick(): THREE.Object3D | null;
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
  annex: RecordsAnnex; colliders: Collider[];
  ch1: Chapter1; ch2: Chapter2; ch3: Chapter3;
  motel: Motel;
  save: () => void;
  milestone: () => void;
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

type Mode = 'menu' | 'table' | 'control' | 'tone' | 'p11' | 'p12';

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
  private exitT = -1;
  private headYaw = 0;

  constructor(private g: Chapter4Host, private d: Chapter4Deps) {}

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
    this.d.ch2.exit = { label: () => this.exitLabel(), use: () => this.useExit() };
    this.bindCourt();
    this.applyWorld();
    if (!saved) {
      this.g.note(`${this.time()}. N. Vega is in room 6 at Sierra Motor Court, across the highway west of SARO. She wants the original record and the prints. The fire exit at the west end of the south corridor is the short way.`);
      this.d.ui.toast('Room 6 is across the road. The fire exit at the end of the south corridor is the short way.', 4.4);
    }
    this.d.save();
  }

  reset() {
    this.active = false; this.started = false;
    this.s = fresh(); this.lines = []; this.mode = 'menu'; this.reply = null; this.panel = null; this.exitT = -1;
    this.d.ch2.exit = null;
    this.d.annex.setExitDoor(0);
    this.setLeaf(false);
    const brick = this.d.motel.brick(); if (brick) brick.visible = false;
    this.d.motel.court().setRoom6Light?.(true);
    const room = this.d.motel.room();
    if (room) { for (const k of ['fieldCard', 'letter', 'correction']) { const o = room.objs[k]; if (o) o.visible = false; } room.setLamp(true); }
  }

  // Puts the world in the state of this.s (after begin, and when room 6 has loaded).
  private applyWorld() {
    const s = this.s;
    this.d.annex.setExitDoor(s.exitOpen ? 1 : 0);
    this.setLeaf(s.exitOpen);
    const brick = this.d.motel.brick(); if (brick) brick.visible = s.exitOpen;
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
        if (!s.exitOpen && this.d.motel.area() === 'saro') return 'SIERRA MOTOR COURT // ROOM 6 IS ACROSS THE ROAD. THE FIRE EXIT IS AT THE END OF THE SOUTH CORRIDOR';
        return 'SIERRA MOTOR COURT // CROSS THE ROAD TO ROOM 6';
      case 'room':
        if (!room) return 'SIERRA MOTOR COURT // N. VEGA IS STILL WAITING IN ROOM 6';
        if (!s.greeted) return 'ROOM 6 // N. VEGA IS WAITING AT THE TABLE';
        if (!s.p10) return 'ROOM 6 // PUT THE PRINTS AND THE 1947 RECORD ON THE TABLE (P10)';
        if (!s.p11 || !s.p12) {
          const left = (['record', 'tomas', 'cable'] as Topic[]).filter((t) => !s.topics[t]).length;
          if (left) return `ROOM 6 // ASK HER ABOUT THE RECORD, T. VEGA AND THE CABLE (${3 - left} OF 3)${s.p12 ? '' : '. AND THE PHONE'}`;
          if (!s.p11) return 'ROOM 6 // RECORD WHY SHE REWROTE THE FINDING (P11)';
          return 'ROOM 6 // ASK ABOUT THE PHONE ON THE NIGHTSTAND (P12)';
        }
        return 'ROOM 6 // ASK HER WHERE C WENT';
      case 'leave': return room ? 'ROOM 6 // THE ROAD SOUTH IS NEXT. LEAVE ROOM 6' : 'SIERRA MOTOR COURT // THE ROAD SOUTH IS NEXT';
      case 'complete': return 'THE ROSWELL ROAD // NEXT AREA NOT YET PLAYABLE';
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
      { text: 'Record why she rewrote the finding (P11)', done: s.p11 },
      { text: 'Ask about the phone on the nightstand (P12)', done: s.p12 },
    );
    if (s.p10 && s.p11 && s.p12) t.push({ text: 'Ask where C went', done: s.told });
    return t;
  }

  // ---------- the fire exit and the road ----------
  private exitLabel() {
    if (!this.active) return null;
    return this.s.exitOpen ? 'Fire exit (propped open)' : 'Open the fire exit';
  }
  private useExit() {
    const { ui, audio, annex } = this.d;
    if (this.s.exitOpen) { ui.toast('Propped open with a brick. The road is past the end of the ramp.', 3.2); return; }
    this.s.exitOpen = true;
    this.exitT = this.g.gt;
    const at = this.pos(annex.objs.exitDoor);
    audio.play('switch', { gain: 0.6, rate: 0.7, at });
    audio.play('thudSoft', { gain: 0.4, when: 0.5, at });
    ui.toast('The alarm on the push bar was disconnected years ago. You prop the door with the brick on the step.', 4.2);
    this.g.after(0.9, () => { const b = this.d.motel.brick(); if (b) b.visible = true; });
    this.setLeaf(true);
    this.save();
  }
  private setLeaf(open: boolean) {
    const cols = this.d.colliders, c = this.d.annex.exitLeafCol;
    const i = cols.indexOf(c);
    if (open && i < 0) cols.push(c);
    if (!open && i >= 0) cols.splice(i, 1);
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
        this.d.motel.goIn();
      } });
    const extra = (id: string, label: string, text: string) => {
      if (px[id]) inter.add({ id: 'court:' + id, object: px[id], label: () => on() ? label : null, use: () => ui.toast(text, 3.6) });
    };
    extra('officeDoor', 'Motel office', 'The office of Sierra Motor Court. The night bell is on the counter.');
    extra('register', 'Guest register', 'A register in her handwriting. Room 6: N. Vega, every night since the spring.');
    extra('keyBoard', 'Key board', 'Ten hooks with brass tags. The hook for 6 is empty.');
    extra('officePhone', 'Field telephone', 'An oak wall telephone with a crank. The cable runs out through the back wall and on to STATION 01.');
    extra('envelope', 'Sealed envelope', 'An envelope addressed to T. Vega, care of Sierra Motor Court. Postmarked July 1947, never opened.');
    extra('message', 'A note on the counter', '"Room 6. The door is open. N."');
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
      add('tv', 'tv', () => 'Television', () => ui.toast('Off. At this hour it would only show a test card.', 3));
      add('bed', 'bed', () => 'Bed', () => ui.toast('Made, and not slept in.', 2.6));
      add('lamp', 'lamp', () => 'Table lamp', () => { room.setLamp(!room.lampOn); this.d.audio.play('switch', { gain: 0.3, rate: 1.4 }); });
      add('bathroom', 'bathroom', () => 'Bathroom door', () => ui.toast('Shut. A tap drips behind it.', 2.6));
      add('door', 'door', () => this.s.stage === 'leave' ? 'Leave room 6' : 'Door to the walk', () => this.useDoor(), 2.6);
    }
    if (this.active) this.applyWorld();
  }

  // Called by World when the player comes in through the door.
  enteredRoom() {
    const s = this.s;
    if (!this.active) return;
    if (!s.arrived) {
      s.arrived = true;
      if (s.stage === 'to-motel') s.stage = 'room';
      this.g.note(`${this.time()}. Room 6. N. Vega in the chair by the window, a shoebox of papers on the table. She has been waiting a long time; she does not say so.`);
      this.d.ui.toast('N. Vega is sitting by the window.', 3.4);
      this.d.milestone();
    }
  }
  // Called by World when the player has stepped out of room 6.
  leftRoom() {
    if (!this.active) return;
    if (this.s.stage === 'leave') this.finish();
  }

  private useDoor() {
    const s = this.s;
    if (s.stage !== 'leave' && s.arrived && !s.told) this.d.ui.toast('N. VEGA: "Go if you need to. I will be here."', 3.2);
    this.d.motel.goOut();
  }

  private look(what: 'photo' | 'window') {
    const { ui } = this.d;
    const first = !this.s.seen[what];
    this.s.seen[what] = true;
    if (what === 'photo') {
      ui.toast('Two people at a transit on a concrete pier, squinting into the sun. On the back, in pencil: N. and T., STATION 01, June 1947.', 5);
      if (first) this.g.note('A framed snapshot on the dresser: N. and T. Vega at the transit at STATION 01, June 1947.');
    } else ui.toast('Through the gap in the curtains: the Sierra sign, red and cyan, and the lights of SARO across the road.', 4);
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
  private openTalk() {
    const s = this.s;
    if (!s.greeted) {
      s.greeted = true;
      this.mode = 'table';
      this.lines = [
        n('So you came across the road.'),
        n('Do not sit yet. Put them on the table first. The two prints from B-12, and the record from 1947. The one with my name on it.'),
      ];
      this.save();
    } else if (this.mode === 'menu' && !this.lines.length) {
      this.lines = [aside(s.told ? 'She is looking out at the road.' : 'She waits for you to go on.')];
    }
    this.reply = null;
    this.panel = P.talk(this.d.ui, {
      title: 'ROOM 6 / N. VEGA',
      view: () => this.view(),
      onChip: (id) => this.toggleChip(id),
      onTable: () => this.showEvidence(),
      onChoice: (id) => this.choose(id),
      onRead: (id) => this.read(id as Paper),
      onClose: () => { this.panel = null; },
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
      case 'p11':
        v.question = 'P11 / Why did N. Vega rewrite the 1947 finding?';
        v.choices = [
          { id: 'repeat', label: 'She believed the complete record would let someone connect the arrangement again.' },
          { id: 'fault', label: 'Her plate had a development fault, and she covered it.' },
          { id: 'protect', label: 'T. Vega\'s readings were wrong, and she protected him.' },
        ];
        break;
      case 'p12':
        v.question = 'P12 / What does the return line show?';
        v.choices = [
          { id: 'retained', label: 'The 1947 correction, repeating unchanged. It does not answer a new question.' },
          { id: 'alive', label: 'T. Vega, answering her now.' },
          { id: 'crosstalk', label: 'Crosstalk on an old field line.' },
        ];
        break;
      case 'menu':
        if (!s.topics.record) v.choices.push({ id: 'record', label: 'Ask about the amended record' });
        if (!s.topics.tomas) v.choices.push({ id: 'tomas', label: 'Ask who T. Vega was' });
        if (!s.topics.cable) v.choices.push({ id: 'cable', label: 'Show her PHOTO 04, the cable break', disabled: !this.have('frame04') });
        if (!s.p12) v.choices.push({ id: 'line', label: 'Ask about the phone on the nightstand' });
        if (s.topics.record && s.topics.tomas && s.topics.cable && !s.p11) v.choices.push({ id: 'why', label: 'Record why she rewrote the finding (P11)' });
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
    this.reply = null;
    switch (this.mode) {
      case 'control': return this.answerControl(id);
      case 'tone': return this.answerTone(id as 'mild' | 'critical');
      case 'p11': return this.answerP11(id);
      case 'p12': return this.answerP12(id);
    }
    switch (id) {
      case 'record': this.mode = 'tone'; this.lines = [you('About the amended copy.'), aside('She does not look away from it.')]; return;
      case 'tomas': return this.topicTomas();
      case 'cable': return this.topicCable();
      case 'line': return this.topicLine();
      case 'why': this.mode = 'p11'; this.lines = [aside('The correction, the letter and the field card lie on the table between you.')]; return;
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

  private answerP11(id: string) {
    const s = this.s;
    if (id !== 'repeat') {
      s.wrong11++;
      this.reply = { ok: false, text: id === 'fault'
        ? 'Her own correction says the mark was not a development fault. She wrote that it was one.'
        : 'His letter says he checked the reference twice. His readings were not the problem.' };
      this.save();
      return;
    }
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
  }

  // P12: the phone on the nightstand is on the old field line.
  private topicLine() {
    const { audio, ui } = this.d;
    const s = this.s;
    if (s.line) { this.mode = 'p12'; this.lines = [aside('The handset is back on the cradle.')]; return; }
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
    this.mode = 'p12';
    if (at) {
      audio.play('click', { gain: 0.5, at });
      audio.radio(at, 0.08);
      audio.radioVoice(4.4, 3.6);
      this.g.after(9.5, () => { audio.radio(null); audio.play('click', { gain: 0.45, at }); });
    }
    this.g.note('The phone in room 6 is on the old field line to the hut. She asked it: "Tomás. Where did you leave the spare key?" The line said: "Reference west. No. East. Hold the last reading." The same words as in 1947.');
    ui.toast('The same words as on the receiver in the hut.', 3);
    this.save();
  }

  private answerP12(id: string) {
    const s = this.s;
    if (id !== 'retained') {
      s.wrong12++;
      this.reply = { ok: false, text: id === 'alive'
        ? (s.read.letter ? 'He did not answer the question. His letter says where the key was: in the mug. The line said nothing about it.' : 'He did not answer her question. It is the same fragment as in the 1947 timing log.')
        : 'Crosstalk does not repeat the same correction in the same place, every time.' };
      this.save();
      return;
    }
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

  endingLines() {
    return [
      'ROOM 6',
      'SIERRA MOTOR COURT // P10 TO P12 RECORDED',
      'Tonight\'s control print carries the same mark as her plate from 1947: a reference that stays on the film after its source is gone.',
      'She left C out of the record and blamed development. She cut the cable herself, with her brother Tomás standing on C. He was not there afterwards. The line still repeats his last correction, and it does not answer.',
      'C crosses the old highway to Roswell. NEXT: THE ROSWELL ROAD. This part of the night is still being built.',
    ];
  }

  // ---------- documents ----------
  private docCard(): DocSpec {
    return { id: 'e11', title: 'E11 / T. Vega, last observation card', kind: 'hand', stamp: 'E11', page: CARD,
      transcript: 'A field card in pencil, from Tomás Vega\'s coat. She has kept it for thirty-nine years.\n\nThe first half separates opening the lamp from masking C. The second half says how the last reading must be taken, and who should not take it.' };
  }
  private docLetter(): DocSpec {
    return { id: 'e13', title: 'E13 / A letter from T. Vega, 1947', kind: 'hand', stamp: 'E13', page: LETTER,
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
    // the fire exit swings out
    if (this.exitT >= 0) {
      const k = Math.min(1, (this.g.gt - this.exitT) / 0.8);
      this.d.annex.setExitDoor(1 - (1 - k) * (1 - k));
      if (k >= 1) this.exitT = -1;
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
