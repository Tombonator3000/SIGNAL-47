import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import { PHONE_CALLS } from '../core/phoneScripts';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { FieldCamera, Photo } from '../core/FieldCamera';
import type { Chapter1, ShotCheck } from './Chapter1';
import * as P from '../ui/Panels';
import type { BoardCard } from '../ui/Board';
import { RuleBook, type BoardHook, type BookNote, type BookRule } from './boardRules';
import * as D from './drawings';
import { clockText } from './time';
import type { FbSave } from './fieldBook';

// Chapter three, "The Survey Station" (K3 in the design bible). Ported from the Unity
// field pass (Station26): P06, which fixed point moved, with FRAME 03; P07, the local
// lamp null test; P08, the deliberate cable cut, with FRAME 04; P09, the 1947 timing log
// against tonight's receiver. P06 and P07 go in either order, P08 needs both and its own
// photograph, P09 needs P08. Field texts and replies are the Unity ones. P07 is made at the
// lamp; P06, P08 and P09 are made on the case board, by tying the records and the frames
// together (SPILLDESIGN.md: "Kapittel 3 på bordet"), where a tabbed panel used to offer three
// sentences each.
// New in the web version: the drive out in the SARO service truck, Nora's call on the
// hut's field telephone after P09, and the drive back to develop both frames at the
// SARO wet bench (Unity did the same with a travel folio).

export type Stage3 = 'to-truck' | 'station' | 'call' | 'return' | 'develop' | 'complete';
export type AreaId = 'saro' | 'road' | 'station01' | 'room6' | 'diner' | 'roswell';

export interface Ch3State {
  v: 1;
  stage: Stage3;
  arrived: boolean;
  readE09a: boolean; readE10: boolean; heard: boolean;
  seen: { footing: boolean; markerA: boolean; markerB: boolean; stakes: boolean; diagram: boolean; hut: boolean };
  lampCovered: boolean; lampObserved: boolean; cableInspected: boolean;
  f3?: Photo; f4?: Photo;
  p06: boolean; p07: boolean; p08: boolean; p09: boolean;
  wrong06: number; wrong08: number; wrong09: number; rejected: number;
  called: boolean;
  dev: number;            // the field roll: 0 sealed, 1 developed, 2 print in fixer, 3 collected
  links?: string[];       // this chapter's threads on the case board, 'a|b' sorted
  /** T. Vega's field book from the drawer in the hut, once read (story/fieldBook.ts). */
  fb?: FbSave;
}

// What chapter three needs from STATION 01 (world/Station01.ts).
export interface FieldSite {
  proxies: Record<string, THREE.Object3D>;
  anchors: {
    arrive: { x: number; z: number; yaw: number };
    markerTarget: THREE.Vector3;
    cableTarget: THREE.Vector3;
  };
  occluders?: THREE.Object3D[];
  objs?: Record<string, THREE.Object3D>;   // fieldPhoneHandset rattles while the bell rings
  setLampCovered(on: boolean): void;
  setReceiver(on: boolean): void;
  setFlash?(p: THREE.Vector3 | null, w?: number): void;   // the camera flash, for one exposure
}

// Travel between SARO and the station (main.ts: the areas, the truck and the drive).
export interface Travel {
  area(): AreaId;
  site(): FieldSite | null;
  driving(): boolean;
  driveOut(): void;
  driveBack(): void;
  /** What the truck offers where it stands: this chapter's drive, a later one's, or the
   *  player's own (main.ts, World.truckChoice). */
  truckChoice(place: 'saro' | 'station01' | 'diner'): { label: string; use: () => void };
}

export interface Chapter3Host {
  clock: number;
  gt: number;
  notes: string[];
  docs: DocSpec[];
  cinematic: boolean;
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
  cancel(tag: string): void;
}

export interface Chapter3Deps {
  ui: UI; audio: AudioSys; player: Player; inter: Interaction; fcam: FieldCamera;
  ch1: Chapter1;
  view: { restore: () => void; draw: () => void };
  travel: Travel;
  save: () => void;
  milestone: () => void;   // a new area or a recorded finding starts a new autosave generation
  board: BoardHook;
}

const fresh = (): Ch3State => ({
  v: 1, stage: 'to-truck', arrived: false, readE09a: false, readE10: false, heard: false,
  seen: { footing: false, markerA: false, markerB: false, stakes: false, diagram: false, hut: false },
  lampCovered: false, lampObserved: false, cableInspected: false,
  p06: false, p07: false, p08: false, p09: false,
  wrong06: 0, wrong08: 0, wrong09: 0, rejected: 0, called: false, dev: 0,
});

// ---------- the sources and replies (Unity Station26 text) ----------
const TRANSIT = 'FIELD TRANSIT / STATION 01 / E09A\n\n1947 / A: triangle with bar; mounted in the concrete footing on the transit sight line. B: fixed comparison marker to the west. C: closing sight line, shown in the original arrangement.\n\nCompare the empty footing, present markers and this diagram. Photograph both the footing and the marked post before recording which position changed.';
const TIMING_1947 = 'FIELD TIMING RECORD / STATION 01 / E10\n\n02:17:00 / Carrier ceased. Recorded voice: ‘Reference west. No. East. Hold the last reading.’\n02:17:47 / Local relay impact and reference motion observed.\nFinal observation / Not entered.';
const TIMING_1986 = 'NEW RECEIVER OBSERVATION / 1986\nThe received fragment repeats the same correction, ‘west’ to ‘east,’ in the same place. The original field record identifies the speaker as T. Vega.';

const SUPPORTED = {
  marker: 'P06 SUPPORTED / The fixed survey marker A has moved relative to the transit record. The original footing and fixed B marker establish the comparison; this record does not identify who moved A.',
  lamp: 'NULL TEST RECORDED / The local source is screened. This control result does not complete the cable reconstruction.',
  cable: 'P08 SUPPORTED / The cable was deliberately cut. The null test separates the local lamp from the retained reference.',
  timing: 'P09 SUPPORTED / The historic relay observation followed carrier loss by 47 seconds. The 1986 fragment repeats T. Vega\'s same correction and adds no final observation.',
};

// Three levels, as in chapters one and two: a question, a pointer, the answer.
type Page = 'marker' | 'lamp' | 'cable' | 'timing';
const HINTS: Record<Page, string[]> = {
  marker: ['Which fixed point is no longer where E09A puts it?', 'E09A puts A in the concrete footing on the transit sight line. Is the footing still occupied?', 'The footing is empty, and the post with the triangle and bar stands off the line. Tie A on E09A to FRAME 03.'],
  lamp: ['A null test changes one local condition and looks again.', 'The field lamp has a tin hood. Cover it, then observe.', 'Stand by the lamp, cover it, and choose OBSERVE NULL TEST.'],
  cable: ['Look at the ends of the cable, not at the reference.', 'Old cable frays and corrodes. These faces are clean and opposite each other. And the lamp, covered, changed nothing.', 'Tie FRAME 04, the cut, to the null test at the lamp, P07.'],
  timing: ['Read the 1947 log in the hut and listen to the receiver.', 'The carrier ceased at 02:17:00. What was observed 47 seconds later?', 'Tie the receiver to the line in E10 with the same words.'],
};

// ---------- the case board ----------
// What the station's records and frames show together (story/boardRules.ts). The replies to
// threads that contradict are the Unity ones where Unity had that wrong answer.
const NOTES: Record<string, BookNote> = {
  'n.a': { text: 'A is gone from its footing on the transit sight line. The post with its mark, the triangle and bar, stands off the line.', q: 'P06' },
  'n.cut': { text: 'Two clean faces, opposite each other, and a lamp that changed nothing when it was covered. The cable was cut on purpose.', q: 'P08' },
  'n.same': { text: 'Tonight\'s fragment is T. Vega\'s correction from the 1947 log, word for word. The relay struck 47 seconds after it, at 02:17:47.', q: 'P09' },
};
const NOT_THE_LAMP = 'The lamp and the cable do not identify the moved fixed point. Match the outlined triangle and short bar, then record A.';
const NOT_WHO = 'The faces show that the cable was cut on purpose. They do not show who cut it, or why.';
const RULES: BookRule[] = [
  { a: 'e09a.a', b: 'frame03', note: 'n.a', text: NOTES['n.a'].text },
  { a: 'e09a.b', b: 'frame03', text: 'B is still in its footing west of the transit. The post in FRAME 03 carries the triangle with the bar.', wrong: 'P06' },
  { a: 'e09a.c', b: 'frame03', text: 'C is a sight line, not a marker; the stakes still run east. Which reference had the triangle with the bar?', wrong: 'P06' },
  { a: 'P07', b: 'frame03', text: NOT_THE_LAMP, wrong: 'P06' },
  { a: 'e09a.a', b: 'frame04', text: NOT_THE_LAMP, wrong: 'P06' },
  { a: 'frame04', b: 'P07', note: 'n.cut', text: NOTES['n.cut'].text },
  { a: 'frame04', b: 'e07.sig', text: NOT_WHO, wrong: 'P08' },
  { a: 'frame04', b: 'e06.sig', text: NOT_WHO, wrong: 'P08' },
  { a: 'frame04', b: 'e07.head', text: 'The cut faces are opposed and clean. A cable from 1947 can wear through, but age and weather do not leave a break like that.', wrong: 'P08' },
  { a: 'hut.rx', b: 'e10.carrier', note: 'n.same', text: NOTES['n.same'].text },
  { a: 'hut.rx', b: 'e10.relay', text: 'The voice is not on that line. The matched reading arrives at the 47-second relay impact: 02:17:47. Do not infer a new event from the clock alone.', wrong: 'P09' },
  { a: 'hut.rx', b: 'e10.final', text: 'The fragment is the correction, not a final observation. It adds nothing after it, and the log leaves that line empty.', wrong: 'P09' },
  // threads that hold without answering anything
  { a: 'e09a.c', b: 'e07.abc', text: 'The same C: the closing sight line, on the transit plan and in the 1947 record.' },
  { a: 's1diagram', b: 'e06.omit', text: 'The framed copy in the hut still has C. Only the service copy left it out.' },
];

const TANK_TIME = 2.2, FIX_TIME = 1.5;
const RING = 'ring3';

export class Chapter3 {
  active = false;
  started = false;
  ringing = false;
  s: Ch3State = fresh();
  onEnd?: () => void;
  private bound = false;
  private busy: string | null = null;
  private panel: { refresh: () => void } | null = null;
  private images: { transit?: string } = {};
  private callT = -1;
  private radioOn = false;
  /** P06 to P09 on the case board: the station's records and frames, the questions, the rules. */
  readonly book: RuleBook;

  constructor(private g: Chapter3Host, private d: Chapter3Deps) {
    const s = () => this.s;
    this.book = new RuleBook({
      key: 'ch3',
      owns: (n) => /^(e09a|e10)\./.test(n) || ['frame03', 'frame04', 'hut.rx', 'P06', 'P07', 'P08', 'P09'].includes(n),
      notes: NOTES, rules: RULES,
      questions: [
        { id: 'P06', stamp: 'P06', text: 'Which fixed point is no longer where E09A puts it?', slots: 1, hints: HINTS.marker, x: 14, y: 470, w: 290,
          open: () => this.started, done: () => s().p06 ? SUPPORTED.marker : null, record: () => this.submitMarker('a') },
        { id: 'P07', stamp: 'P07', text: 'Does the local lamp make the mark?', slots: 0, hints: HINTS.lamp, x: 600, y: 14, w: 270,
          elsewhere: 'Made at the field lamp, not here: cover it, then observe a null test.',
          open: () => this.started, done: () => s().p07 ? SUPPORTED.lamp : null },
        { id: 'P08', stamp: 'P08', text: 'What does the cable break show?', slots: 1, hints: HINTS.cable, x: 600, y: 222, w: 270,
          open: () => this.started, done: () => s().p08 ? SUPPORTED.cable : null, record: () => this.submitCable('deliberate-cut') },
        { id: 'P09', stamp: 'P09', text: 'Match the receiver fragment to the 1947 log. What does T. Vega\'s correction lead up to?', slots: 1, hints: HINTS.timing, x: 600, y: 444, w: 270,
          open: () => this.started, done: () => s().p09 ? SUPPORTED.timing : null, record: () => this.submitTiming('02:17:47') },
      ],
      links: () => (this.s.links ??= []),
      wrong: (q) => { const x = this.s; if (q === 'P06') x.wrong06++; else if (q === 'P08') x.wrong08++; else if (q === 'P09') x.wrong09++; },
      save: () => this.save(),
      cards: () => this.boardCards(),
    });
  }

  // The station's records and the two field frames, as they lie on the board. A frame is
  // on the roll, sealed, until it is developed at SARO; what is in it is known all the same.
  private boardCards(): BoardCard[] {
    if (!this.started) return [];
    const s = this.s, dev = s.dev >= 3;
    const frame = (f: Photo | undefined, id: 'frame03' | 'frame04', title: string, what: string, where: string, x: number, y: number): BoardCard => ({
      id, doc: id, stamp: id === 'frame03' ? 'FRAME 03' : 'FRAME 04', title: dev ? title : 'ON THE FIELD ROLL', kind: dev ? 'photo' : 'sealed',
      have: !!f, where, image: dev ? f?.url : undefined, text: dev ? what : `${what} Sealed; it is developed at SARO.`, x, y, w: 250,
    });
    return [
      { id: 'e09a', doc: 'e09a', stamp: 'E09A', title: 'TRANSIT RECORD', have: s.readE09a, where: 'Behind glass in a steel frame on the west face of the transit pier.',
        image: this.transitImage(), full: TRANSIT, x: 14, y: 14, w: 290, lines: [
          { id: 'e09a.a', text: 'A: triangle with bar, mounted in the concrete footing on the transit sight line.' },
          { id: 'e09a.b', text: 'B: fixed comparison marker to the west.' },
          { id: 'e09a.c', text: 'C: closing sight line, shown in the original arrangement.' }] },
      frame(s.f3, 'frame03', 'FIXED POINT A', 'The empty footing on the transit sight line, and the post with the triangle and bar standing off it.',
        'Expose it from near the transit: the empty footing and the marked post in one frame.', 320, 14),
      frame(s.f4, 'frame04', 'CABLE BREAK', 'The cable south of the transit pier, cut: two clean faces, opposite each other.',
        'Inspect the cable break, then expose a near frame of the cut.', 320, 300),
      { id: 'e10', doc: 'e10', stamp: 'E10', title: 'TIMING RECORD, 1947', have: s.readE10, where: 'The 1947 ledger is on the workbench in the hut.',
        full: s.heard ? `${TIMING_1947}\n\n${TIMING_1986}` : TIMING_1947, x: 890, y: 14, w: 276, lines: [
          { id: 'e10.carrier', text: '02:17:00 / Carrier ceased. Recorded voice: \u2018Reference west. No. East. Hold the last reading.\u2019' },
          { id: 'e10.relay', text: '02:17:47 / Local relay impact and reference motion observed.' },
          { id: 'e10.final', text: 'Final observation / Not entered.' }] },
      { id: 'hut.rx', stamp: 'RECEIVER', title: 'IN THE HUT, TONIGHT', kind: 'thing', have: s.heard, where: 'The receiver on the hut workbench. Switch it on and listen.',
        text: 'A carrier, very weak, and under it a man\'s voice, far away: "Reference west. No. East. Hold the last reading." Then only the carrier.', x: 890, y: 330, w: 276 },
    ];
  }

  // ---------- lifecycle ----------
  begin(saved: Ch3State | null) {
    this.reset();
    this.active = true; this.started = true;
    this.s = saved ? { ...fresh(), ...saved, seen: { ...fresh().seen, ...saved.seen } } : fresh();
    // chapter one's camera and wet bench answer to this chapter now
    this.d.ch1.field = {
      frameNo: () => this.frameNo(),
      check: () => this.check(),
      expose: () => this.expose(),
      rejected: () => { this.s.rejected++; },
    };
    this.d.ch1.lab = () => this.useBench();
    this.applyWorld();
    if (!saved) {
      this.g.note(`${this.time()}. N. Vega wants me at the cut cable at STATION 01 before I bring her anything. The service truck is on the pad below the east landing. The keys are always in it.`);
      this.d.ui.toast('The service truck is parked on the pad below the east landing.', 4);
    }
    this.d.save();
  }

  reset() {
    const { audio, ch1 } = this.d;
    this.active = false; this.started = false; this.ringing = false;
    this.s = fresh(); this.busy = null; this.panel = null; this.callT = -1; this.radioOn = false;
    ch1.field = null; ch1.lab = null;
    if (audio.ctx) { audio.stop(RING); audio.radio(null); }
    const site = this.d.travel.site();
    if (site) {
      site.setLampCovered(false); site.setReceiver(false);
      const hs = site.objs?.fieldPhoneHandset;
      if (hs) { hs.visible = true; hs.rotation.z = 0; }
    }
  }

  // Puts the world in the state of this.s, after begin or once the station has loaded.
  private applyWorld() {
    const s = this.s;
    const site = this.d.travel.site();
    if (site) { site.setLampCovered(s.lampCovered); site.setReceiver(s.heard); }
    if (s.readE09a) this.file(this.docTransit());
    if (s.readE10) this.file(this.docTiming());
    if (s.p06) this.file(this.docFinding('p06'));
    if (s.p07) this.file(this.docFinding('p07'));
    if (s.p08) this.file(this.docFinding('p08'));
    if (s.p09) this.file(this.docFinding('p09'));
    if (s.dev >= 3 && s.f3 && s.f4) {
      this.d.ch1.hangPrint(2, s.f3); this.d.ch1.hangPrint(3, s.f4);
      this.file(this.docPhoto(s.f3)); this.file(this.docPhoto(s.f4));
    }
    if (s.stage === 'call' && !s.called) this.callT = this.g.gt + 2.5; // it rings again
  }

  // ---------- saving ----------
  saveBlock(): string | null {
    if (this.d.travel.driving()) return 'Not while driving. The game saves when you get there.';
    if (this.busy === 'tank') return 'Not while the film is in the tank.';
    return null;
  }

  private save() { this.d.save(); }
  private time() { return clockText(this.g.clock, false); }
  private file(doc: DocSpec) {
    const i = this.g.docs.findIndex((x) => x.id === doc.id);
    if (i >= 0) { this.g.docs[i] = doc; return false; }
    this.g.docs.push(doc);
    return true;
  }
  private pos(o: THREE.Object3D) { return o.getWorldPosition(new THREE.Vector3()); }
  private dist(p: THREE.Vector3) { const q = this.d.player.pos; return Math.hypot(p.x - q.x, p.z - q.z); }

  // ---------- objective and notebook ----------
  objective(): string {
    const s = this.s;
    const area = this.d.travel.area();
    switch (s.stage) {
      case 'to-truck':
        if (!this.d.travel.driving()) return area === 'diner' ? 'STATION 01 // BACK IN THE TRUCK, THEN RIGHT ONTO THE SURVEY TRACK' : 'SERVICE YARD // TAKE THE SARO TRUCK TO STATION 01';
        // one line the whole way: a change of line where the truck goes from one area into
        // the next would give the change away (World.ts)
        return 'STATION 01 // SOUTH ON THE HIGHWAY, RIGHT ONTO THE SURVEY TRACK TO THE GATE';
      case 'station':
        if (!s.p06 && !s.p07) return 'STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST';
        if (!s.p06) return s.f3 ? 'STATION 01 // ON THE CASE BOARD: WHICH FIXED MARKER MOVED (P06)' : 'STATION 01 // EXPOSE FRAME 03 AND RECORD WHICH FIXED MARKER MOVED';
        if (!s.p07) return 'STATION 01 // COVER THE LOCAL LAMP AND OBSERVE A NORMAL NULL TEST';
        if (!s.p08) return s.f4 ? 'STATION 01 // ON THE CASE BOARD: WHAT THE CABLE BREAK SHOWS (P08)' : 'STATION 01 // INSPECT THE CABLE BREAK AND EXPOSE FRAME 04';
        return 'STATION 01 // MATCH THE 1947 AND 1986 TIMING RECORDS ON THE CASE BOARD';
      case 'call': return this.ringing ? 'STATION 01 // THE FIELD TELEPHONE IS RINGING' : 'STATION 01 // FIELD RECORD COMPLETE';
      case 'return':
        if (!this.d.travel.driving()) return 'STATION 01 // DRIVE BACK TO SARO WITH THE FIELD ROLL';
        return 'SARO // THE TRACK TO THE HIGHWAY, NORTH, UP THE RAMP TO THE TRUCK PAD';
      case 'develop':
        if (s.dev === 0) return 'SARO PHOTOLAB // DEVELOP FRAMES 03 AND 04 AT THE WET BENCH';
        if (s.dev === 1) return 'SARO PHOTOLAB // TRANSFER THE FIELD PRINTS TO THE FIXER';
        return 'SARO PHOTOLAB // COLLECT THE FIELD PRINTS';
      case 'complete': return 'ROOM 6 // SIERRA MOTOR COURT / NEXT AREA NOT YET PLAYABLE';
    }
  }

  tasks() {
    const s = this.s;
    const t = [{ text: 'Take the service truck to STATION 01', done: s.arrived }];
    if (s.arrived) t.push(
      { text: 'Read the transit record (E09A)', done: s.readE09a },
      { text: 'Expose FRAME 03: the empty footing and the marked post', done: !!s.f3 },
      { text: 'Record which fixed marker moved (P06)', done: s.p06 },
      { text: 'Cover the field lamp and observe the null test (P07)', done: s.p07 },
      { text: 'Inspect the cable cut and expose FRAME 04', done: s.cableInspected && !!s.f4 },
      { text: 'Record what the cable break shows (P08)', done: s.p08 },
      { text: 'Listen to the receiver in the hut', done: s.heard },
      { text: 'Read the 1947 timing log (E10)', done: s.readE10 },
      { text: 'Match the two timing records (P09)', done: s.p09 },
    );
    // once the player has been in at the bench: the drawer under it, which nothing asks for
    if (s.heard || s.readE10) t.push({ text: 'The drawer under the hut bench (optional)', done: !!s.fb });
    if (s.p09) t.push({ text: 'Answer the field telephone', done: s.called });
    if (s.called) t.push(
      { text: 'Drive back to SARO', done: s.stage === 'develop' || s.stage === 'complete' },
      { text: 'Develop FRAMES 03 and 04 in the photo lab', done: s.dev >= 3 },
    );
    return t;
  }

  // ---------- the truck ----------
  /** This chapter's drive from wherever the truck stands: out to STATION 01, and back to SARO
   *  with the field roll (main.ts asks it first, World.truckChoice). */
  truckTrip(_place: 'saro' | 'station01' | 'diner'): { label: string; use: () => void } | null {
    if (!this.active) return null;
    if (this.s.stage === 'to-truck') return { label: 'Drive to STATION 01', use: () => this.d.travel.driveOut() };
    if (this.s.stage === 'return') return { label: 'Drive back to SARO', use: () => this.d.travel.driveBack() };
    return null;
  }
  bindSaroTruck(object: THREE.Object3D) {
    const { inter } = this.d;
    inter.add({ id: 'truck', object, range: 3.2,
      label: () => this.d.travel.truckChoice('saro').label,
      use: () => this.d.travel.truckChoice('saro').use(),
      examine: () => 'SARO 07: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.' });
  }

  // Called by main when the drive reaches the station gate (and when a save there is restored).
  arrivedStation() {
    const s = this.s;
    if (s.arrived) return;
    s.arrived = true;
    if (s.stage === 'to-truck') s.stage = 'station';
    this.g.note(`${this.time()}. STATION 01. A stucco hut, a transit on a concrete pier, marker posts, and a cable on the ground. The vehicle gate is chained; the walk gate beside it is open.`);
    this.d.ui.toast('STATION 01. The transit stands on its pier inside the fence.', 4);
    this.d.milestone();
  }
  // Called by main when the drive back ends at SARO.
  arrivedSaro() {
    const s = this.s;
    if (s.stage !== 'return') return;
    s.stage = 'develop';
    this.g.note(`${this.time()}. Back at SARO with the field roll: FRAME 03 and FRAME 04, still sealed. The photo lab is east of the walk.`);
    this.d.ui.toast('Back at SARO. The film goes to the wet bench in the photo lab.', 4);
    this.d.milestone();
  }

  // ---------- the station ----------
  // The station loads on demand; its interactables are added once, the first time.
  bindSite(site: FieldSite) {
    if (!this.bound) {
      this.bound = true;
      const { inter, ui } = this.d;
      const px = site.proxies;
      const on = () => this.active && this.d.travel.area() === 'station01';
      const add = (id: string, key: string, label: () => string | null, use: () => void, range = 2.6) => {
        if (px[key]) inter.add({ id, object: px[key], range, label: () => on() ? label() : null, use });
      };
      add('s1transit', 'transit', () => this.s.readE09a ? 'Lay out the field record' : 'Read the transit record (E09A)', () => this.useTransit(), 2.8);
      add('s1footing', 'footing', () => 'Concrete footing', () => this.look('footing'));
      add('s1markerA', 'markerA', () => 'Marker post', () => this.look('markerA'), 3.2);
      add('s1markerB', 'markerB', () => 'Comparison marker', () => this.look('markerB'), 3.2);
      add('s1stakes', 'stakesC', () => 'Survey stakes', () => this.look('stakes'), 3.5);
      add('s1diagram', 'diagram', () => 'Framed diagram', () => this.look('diagram'));
      add('s1lamp', 'lamp', () => this.s.lampCovered ? 'Field lamp (covered)' : 'Field lamp', () => this.openLamp());
      add('s1cable', 'cable', () => this.s.cableInspected ? 'Cable break' : 'Inspect the cable', () => this.useCable(), 2.4);
      add('s1hutDoor', 'hutDoor', () => 'Field hut', () => this.look('hut'));
      add('s1receiver', 'receiver', () => this.s.heard ? 'Receiver' : 'Switch on the receiver', () => this.useReceiver());
      add('s1timing', 'timingLog', () => this.s.readE10 ? 'Lay out the field record' : 'Read the timing log (E10)', () => this.useTimingLog());
      add('s1phone', 'fieldPhone', () => this.ringing ? 'Answer the field telephone' : 'Field telephone', () => this.usePhone());
      add('s1gate', 'gate', () => 'Vehicle gate', () => ui.toast('Chained and padlocked. N. Vega holds the key. The walk gate beside it is open.', 3.6));
      // the truck outside the gate, any time it stands there (with the truck somewhere else,
      // World.parkAt has put its handle out of the way: nothing to use)
      if (px.truckSpot) inter.add({ id: 's1truck', object: px.truckSpot, range: 3.2,
        label: () => this.d.travel.area() === 'station01' && px.truckSpot.position.y > -100 ? this.d.travel.truckChoice('station01').label : null,
        use: () => this.d.travel.truckChoice('station01').use(),
        examine: () => this.d.travel.area() === 'station01' && px.truckSpot.position.y > -100 ? 'SARO 07: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.' : null });
    }
    if (this.active) this.applyWorld();
  }

  private look(what: 'footing' | 'markerA' | 'markerB' | 'stakes' | 'diagram' | 'hut') {
    const { ui } = this.d;
    const s = this.s;
    const first = !s.seen[what];
    s.seen[what] = true;
    switch (what) {
      case 'footing':
        ui.toast('A square concrete footing on the transit sight line. Four rusted anchor bolts, and nothing on them.', 4);
        if (first) this.g.note('The footing on the transit sight line is empty: four rusted anchor bolts in a square of paler concrete.');
        break;
      case 'markerA':
        ui.toast('A marker post with an outlined triangle and a short bar beneath it. It stands a few metres off the sight line.', 4.2);
        if (first) this.g.note('A post carrying the outlined triangle with a short bar stands a few metres off the transit sight line, away from the empty footing.');
        break;
      case 'markerB':
        ui.toast('B, the comparison marker west of the transit: a vertical vane plate on a post, still set in its footing.', 4);
        if (first) this.g.note('B, the comparison marker west of the transit, is still in its footing.');
        break;
      case 'stakes':
        ui.toast('Old wooden survey stakes run east into the dark, out past the fence. C, the closing sight line.', 4);
        if (first) this.g.note('Weathered survey stakes run east past the fence: C, the closing sight line, as it was laid out in 1947.');
        break;
      case 'diagram':
        ui.document(this.docDiagram(), () => {});
        this.file(this.docDiagram());
        if (first) this.g.note('A framed copy of the 1947 arrangement hangs in the hut: A, B and C, signed N. Vega.');
        break;
      case 'hut':
        ui.toast('The field hut. Workbench, receiver, ledger, a telephone on the wall. The door has not been locked in years.', 4);
        break;
    }
    if (first) this.save();
  }

  private transitImage() {
    if (!this.images.transit) {
      const site = this.d.travel.site();
      if (!site) return D.transitRecord({ a: [0, -8], b: [-9, 0], c: [12, 0] }); // drawn again once the station is built
      const rel = (k: string, fallback: [number, number]): [number, number] => {
        const t = site?.proxies.transit, o = site?.proxies[k];
        if (!t || !o) return fallback;
        const a = this.pos(t), b = this.pos(o);
        return [b.x - a.x, b.z - a.z];
      };
      const c = rel('stakesC', [12, 0]);
      this.images.transit = D.transitRecord({ a: rel('footing', [0, -8]), b: rel('markerB', [-9, 0]), c });
    }
    return this.images.transit;
  }

  // Read once, the transit record is on the board with the rest; after that the transit is
  // where the field record is laid out.
  private useTransit() {
    const s = this.s;
    if (s.readE09a) { this.d.board.open('STATION 01', 'ch3'); return; }
    s.readE09a = true;
    this.file(this.docTransit());
    this.g.note('E09A, the transit record: A sat in the concrete footing on the transit sight line. B is the fixed comparison marker to the west. C is the closing sight line.');
    this.save();
    this.d.ui.document(this.docTransit(), () => this.d.ui.toast('E09A is filed, and pinned on the case board with the station\'s other records.', 3.6));
  }

  private useCable() {
    if (!this.s.cableInspected) { const r = this.inspect(); this.d.ui.toast(r.ok ? 'Two clean faces, opposite each other, copper bright where the sheath ends. Take a near frame of them.' : r.text, 4); return; }
    this.d.ui.toast(this.s.f4 ? 'The cut cable. FRAME 04 has it, close up.' : 'Two clean faces, opposite each other. Take a near frame of the cut.', 3.4);
  }

  private useReceiver() {
    const { ui, audio } = this.d;
    const site = this.d.travel.site();
    const at = site?.proxies.receiver ? this.pos(site.proxies.receiver) : undefined;
    if (this.s.heard) { ui.toast('The carrier, steady under the static. The voice does not come back.', 3.4); return; }
    site?.setReceiver(true);
    if (at) { audio.radio(at); this.radioOn = true; }
    audio.play('click', { gain: 0.5, at });
    this.g.cinematic = true;
    const cap = (t: number, text: string, secs: number) => this.g.after(t, () => ui.toast(text, secs));
    cap(0.4, 'The dial warms to amber. A carrier comes up out of the static, very weak.', 3.4);
    const recorded = audio.phoneCall([{ by: 'THE LINE', text: 'Reference west. No. East. Hold the last reading.' }], { lead: 4.0 });
    if (!recorded) this.g.after(4.0, () => audio.radioVoice(4.6));
    cap(4.0, 'Under the carrier, a man\'s voice, far away: "Reference west. No. East. Hold the last reading."', recorded?.cues[0].duration ?? 5.2);
    this.g.after(Math.max(9.4, recorded?.duration ?? 0), () => {
      this.g.cinematic = false;
      this.s.heard = true;
      this.g.note('The receiver in the hut, on the carrier: a man\'s voice, far away. "Reference west. No. East. Hold the last reading." Then only the carrier.');
      ui.toast('Then only the carrier.', 2.6);
      if (this.s.readE10) this.file(this.docTiming());   // the 1986 line goes under the 1947 one
      this.d.board.refresh();
      this.save();
    });
  }

  private useTimingLog() {
    const s = this.s;
    if (s.readE10) { this.d.board.open('STATION 01', 'ch3'); return; }
    s.readE10 = true;
    this.file(this.docTiming());
    this.g.note('E10, the 1947 timing log: 02:17:00 carrier ceased, a recorded voice. 02:17:47 relay impact and reference motion. Final observation not entered.');
    this.save();
    this.d.ui.document(this.docTiming(), () => this.d.ui.toast('E10 is filed, and pinned on the case board.', 3));
  }

  // ---------- P07 at the field lamp ----------
  private openLamp() {
    this.panel = P.nullTest(this.d.ui, {
      status: () => this.s.p09 ? 'P06 TO P09 RECORDED / Field timing linked / Return to SARO' : this.objective(),
      state: () => ({ covered: this.s.lampCovered, observed: this.s.lampObserved, p07: this.s.p07 }),
      supported: SUPPORTED.lamp,
      onLamp: (cover) => this.setLamp(cover),
      onObserve: () => this.observeLamp(),
      hints: HINTS.lamp,
    });
  }

  private near(key: string, r: number) {
    const o = this.d.travel.site()?.proxies[key];
    return !!o && this.dist(this.pos(o)) <= r;
  }

  private submitMarker(c: 'a' | 'b' | 'lamp') {
    const s = this.s;
    if (!s.readE09a) return { ok: false, text: 'READ E09A / TRANSIT RECORD BEFORE RECORDING THE MARKER FINDING.' };
    if (!s.f3) return { ok: false, text: 'FRAME 03 FIRST / Expose the empty footing and the marked post together, then record which position changed.' };
    if (c !== 'a') { s.wrong06++; this.save(); return { ok: false, text: 'The lamp and the cable do not identify the moved fixed point. Match the outlined triangle and short bar, then record A.' }; }
    if (!s.p06) {
      s.p06 = true;
      this.file(this.docFinding('p06'));
      this.g.note('P06: fixed point A has moved relative to the transit record. B has not. The record does not say who moved A.');
      this.d.audio.play('click', { gain: 0.5 });
      this.d.milestone();
    }
    return { ok: true, text: SUPPORTED.marker };
  }

  private setLamp(cover: boolean) {
    const s = this.s;
    s.lampCovered = cover;
    this.d.travel.site()?.setLampCovered(cover);
    const o = this.d.travel.site()?.proxies.lamp;
    this.d.audio.play('switch', { gain: 0.35, rate: 1.3, at: o ? this.pos(o) : undefined });
    this.d.ui.toast(cover ? 'LOCAL LAMP COVERED // NULL TEST READY' : 'LOCAL LAMP EXPOSED', 2.6);
    this.save();
  }

  private observeLamp() {
    const s = this.s;
    if (!this.near('lamp', 4)) return { ok: false, text: 'MOVE TO THE LOCAL LAMP BEFORE OBSERVING ITS NULL TEST.' };
    if (!s.lampCovered) return { ok: false, text: 'COVER THE LOCAL LAMP BEFORE OBSERVING A NULL TEST.' };
    s.lampObserved = true;
    if (!s.p07) {
      s.p07 = true;
      this.file(this.docFinding('p07'));
      this.g.note('P07: with the field lamp covered, the local null test is normal. The lamp is a local source, nothing more.');
      this.d.ui.toast('Null test recorded. Uncover the lamp again when you need its light.', 4);
      this.d.board.refresh();
      this.d.milestone();
    }
    return { ok: true, text: SUPPORTED.lamp };
  }

  private inspect() {
    const s = this.s;
    if (!this.near('cable', 3)) return { ok: false, text: 'MOVE TO THE CABLE CUT BEFORE INSPECTING ITS FACES.' };
    if (!s.cableInspected) {
      s.cableInspected = true;
      this.g.note('The cable runs from the junction box on the hut to the transit pier. South of the pier it is cut: two clean faces, opposite each other, copper bright where the sheath ends.');
      this.save();
    }
    return { ok: true, text: 'CABLE INSPECTED / Follow the sheath to the opposed cut faces and take the near frame.' };
  }

  private submitCable(c: 'deliberate-cut' | 'weathering' | 'author-guilt') {
    const s = this.s;
    if (!s.p06 || !s.p07) return { ok: false, text: 'P08 REQUIRES BOTH THE MARKER FINDING AND THE LOCAL LAMP NULL TEST.' };
    if (!s.cableInspected || !s.f4) return { ok: false, text: 'FRAME 04 FIRST / Inspect the cut faces and expose a near frame of the break.' };
    if (c !== 'deliberate-cut') {
      s.wrong08++; this.save();
      return { ok: false, text: c === 'author-guilt'
        ? 'The faces show that the cable was cut on purpose. They do not show who cut it, or why.'
        : 'The cut faces are opposed and clean. A lamp fault or ordinary drift does not account for that physical break.' };
    }
    if (!s.p08) {
      s.p08 = true;
      this.file(this.docFinding('p08'));
      this.g.note('P08: the cable was cut on purpose. A cut shows how, not who or why.');
      this.d.audio.play('click', { gain: 0.5 });
      this.d.milestone();
    }
    return { ok: true, text: SUPPORTED.cable };
  }

  private submitTiming(c: '02:17:47' | '02:17:00' | '47-minutes') {
    const s = this.s;
    if (!s.p08) return { ok: false, text: 'RECORD THE MARKER AND CABLE FINDINGS BEFORE MATCHING THE TIMING LOG.' };
    if (!s.readE10) return { ok: false, text: 'READ E10 / HISTORICAL TIMING RECORD BEFORE SUBMITTING A TIME.' };
    if (!s.heard) return { ok: false, text: 'LISTEN TO THE HUT RECEIVER BEFORE MATCHING THE TWO RECORDS.' };
    if (c !== '02:17:47') { s.wrong09++; this.save(); return { ok: false, text: 'The matched reading arrives at the 47-second relay impact: 02:17:47. Do not infer a new event from the clock alone.' }; }
    if (!s.p09) {
      s.p09 = true;
      s.stage = 'call';
      this.file(this.docFinding('p09'));
      this.g.note('P09: the carrier ceased at 02:17:00 and the relay impact came 47 seconds later, at 02:17:47. Tonight\'s fragment repeats T. Vega\'s correction and adds nothing after it.');
      this.d.audio.play('click', { gain: 0.5 });
      this.callT = this.g.gt + 4.0;
      this.d.milestone();
    }
    return { ok: true, text: SUPPORTED.timing };
  }

  // ---------- Nora on the field telephone ----------
  private ring() {
    const site = this.d.travel.site();
    const o = site?.proxies.fieldPhone;
    if (!o || this.d.travel.area() !== 'station01') { this.callT = this.g.gt + 3; return; }
    this.ringing = true;
    this.d.audio.loop(RING, 'phoneRing', { gain: 0.9, rate: 0.82, at: this.pos(o) });
    this.g.after(0.8, () => this.d.ui.toast('The field telephone on the hut wall is ringing.', 3.4));
  }

  private usePhone() {
    const { ui, audio } = this.d;
    if (!this.ringing) {
      ui.toast(this.s.called ? 'The line is open, and quiet. She has said what she wanted to say.' : 'A field telephone with a crank, on a line that leaves the hut through the wall. Nobody answers it.', 3.6);
      return;
    }
    this.ringing = false;
    audio.stop(RING, 0.02);
    const handset = this.d.travel.site()?.objs?.fieldPhoneHandset;
    if (handset) handset.visible = false;
    this.g.cinematic = true;
    const spoken = audio.phoneCall(PHONE_CALLS.fieldNora);
    if (spoken) {
      for (const cue of spoken.cues) this.g.after(cue.at, () => ui.toast(`${cue.by}: "${cue.text}"`, Math.max(2, cue.duration)));
    } else {
      audio.callLine(18.4);
      const cap = (t: number, text: string, secs: number) => this.g.after(t, () => ui.toast(text, secs));
      cap(0.3, 'N. VEGA: "So you went out there."', 2.6);
      cap(3.0, 'N. VEGA: "This line still rings in the motel office. It has since 1947. Nobody ever took it down."', 4.6);
      cap(7.8, 'N. VEGA: "You have seen the cable now. It did not break by itself."', 3.8);
      cap(11.8, 'N. VEGA: "Room 6. Bring the original record and your film, developed. Then I will tell you the rest."', 5.0);
    }
    this.g.after(spoken ? spoken.duration + 0.1 : 17.0, () => {
      this.g.cinematic = false;
      if (handset) handset.visible = true;
      const s = this.s;
      s.called = true; s.stage = 'return';
      this.g.note(`${this.time()}. N. Vega called the field telephone. The line still rings in the motel office. She wants the original record and my film, developed, in room 6.`);
      ui.toast('The line goes quiet. The truck is by the gate.', 3.4);
      this.d.milestone();
    });
  }

  // ---------- the field camera ----------
  private frameNo() {
    const s = this.s;
    if (s.f3 && s.f4) return 'FRAME 05';
    if (!s.f4 && (s.f3 || this.dist(this.target('cable')) < 6)) return 'FRAME 04';
    return 'FRAME 03';
  }
  private target(which: 'marker' | 'cable') {
    const a = this.d.travel.site()?.anchors;
    return a ? (which === 'marker' ? a.markerTarget : a.cableTarget) : new THREE.Vector3(0, -999, 0);
  }

  // `near` (chapter one's ShotCheck): within the transit sight line or at the cable break, a
  // miss is a refused case frame and never a free one (core/album.ts)
  private check03(): ShotCheck {
    const { fcam } = this.d;
    const site = this.d.travel.site()!;
    const t = this.target('marker');
    const d = this.dist(t), near = d <= 16;
    if (!this.s.readE09a) return { text: 'READ THE TRANSIT RECORD E09A FIRST', ok: false, near };
    if (d > 16) return { text: 'MOVE CLOSER TO THE TRANSIT SIGHT LINE', ok: false };
    if (d < 2.2) return { text: 'STEP BACK // THE FOOTING AND THE POST IN ONE FRAME', ok: false, near };
    const f = site.proxies.footing ? this.pos(site.proxies.footing) : t;
    const m = site.proxies.markerA ? this.pos(site.proxies.markerA) : t;
    if (!fcam.inFrame(t, 0.18, 0.82, 0.15, 0.85) || !fcam.inFrame(f, 0.04, 0.96, 0.06, 0.94) || !fcam.inFrame(m, 0.04, 0.96, 0.06, 0.94))
      return { text: 'FRAME THE EMPTY FOOTING AND THE MARKED POST TOGETHER', ok: false, near };
    if (site.occluders && !fcam.clearLine(t, site.occluders)) return { text: 'VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT', ok: false, near };
    return { text: 'FOOTING AND MARKED POST IN FRAME // READY', ok: true };
  }
  private check04(): ShotCheck {
    const { fcam } = this.d;
    const t = this.target('cable'), near = this.dist(t) <= 3.2;
    if (!this.s.cableInspected) return { text: 'INSPECT THE CABLE CUT FIRST', ok: false, near };
    if (!near) return { text: 'MOVE CLOSER TO THE CABLE BREAK', ok: false };
    if (!fcam.inFrame(t, 0.22, 0.78, 0.18, 0.82)) return { text: 'FRAME THE CUT CABLE ENDS', ok: false, near };
    return { text: 'CUT FACES IN FRAME // READY', ok: true };
  }

  // Why the shutter would (not) fire right now. Chapter one asks this while it runs.
  check(): ShotCheck {
    const s = this.s;
    if (this.d.travel.area() !== 'station01' || !this.d.travel.site()) {
      return { text: s.f3 && s.f4 ? 'FIELD ROLL EXPOSED // DEVELOP IT AT THE WET BENCH' : 'THE FIELD ROLL IS FOR STATION 01', ok: false };
    }
    if (s.f3 && s.f4) return { text: 'FRAMES 03 AND 04 EXPOSED // DEVELOP AT SARO', ok: false };
    const c4 = s.f4 ? null : this.check04();
    if (c4?.ok) return c4;
    const c3 = s.f3 ? null : this.check03();
    if (c3?.ok) return c3;
    const near = !!(c4?.near || c3?.near);
    if (c4 && (!c3 || this.dist(this.target('cable')) < 6)) return { ...c4, near };
    return { ...c3!, near };
  }

  private expose() {
    const { fcam, ui, audio } = this.d;
    const site = this.d.travel.site()!;
    const cable = !this.s.f4 && this.check04().ok;
    const clock = this.g.clock;
    fcam.sync();
    const targets: Record<string, [number, number]> = {};
    const keys = cable ? ['cable'] : ['footing', 'markerA'];
    for (const k of keys) {
      const o = site.proxies[k];
      const p = k === 'cable' ? this.target('cable') : o ? this.pos(o) : null;
      if (!p) continue;
      const r = fcam.project(p); if (r.front) targets[k] = [r.u, r.v];
    }
    // out here at night the camera needs its flash; nearer subjects get less of it
    const subject = cable ? this.target('cable') : this.target('marker');
    const flash = Math.min(18, Math.max(3, this.dist(subject) * 2.4));
    const canvas = fcam.expose({
      before: () => site.setFlash?.(fcam.photoCam.position, flash),
      after: () => site.setFlash?.(null),
      restore: () => this.d.view.restore(),
      caption: cable ? 'STATION 01 // CABLE BREAK' : 'STATION 01 // FIXED POINT A',
      time: clockText(clock),
      seed: Math.floor(clock * 1000),
    });
    this.d.view.draw();
    const photo: Photo = {
      id: cable ? 'frame04' : 'frame03', subject: cable ? 'STATION 01 / cable break' : 'STATION 01 / fixed point A',
      method: 'field', clock, url: canvas.toDataURL('image/jpeg', 0.86), targets,
    };
    audio.shutter();
    ui.shutterFlash();
    if (cable) {
      this.s.f4 = photo;
      this.g.note('Frame 04 exposed: the cable break, close up. Sealed film; it is developed at SARO.');
      ui.toast('Frame 04 exposed. The roll is developed at SARO.', 3.5);
    } else {
      this.s.f3 = photo;
      this.g.note('Frame 03 exposed: the empty footing and the marked post in one frame. Sealed film; it is developed at SARO.');
      ui.toast('Frame 03 exposed. The roll is developed at SARO.', 3.5);
    }
    this.panel?.refresh();
    this.d.board.refresh();
    this.save();
  }

  // ---------- the wet bench at SARO ----------
  private useBench(): boolean {
    if (!this.active) return false;
    const { ui, audio, ch1 } = this.d;
    const s = this.s;
    if (s.stage !== 'develop' || !s.f3 || !s.f4) {
      P.wetBench(ui, { frame: '', step: 0, onAct: () => {}, empty: s.dev >= 3
        ? 'The tank is rinsed and empty.\nFrames 03 and 04 hang on the drying line and are filed in the case.'
        // Tom, 18 October: "how do you fill the tank?" Say where the film comes from.
        : !s.f3 || !s.f4
          ? 'The tank is empty. Nothing to develop yet.\nFRAMES 03 and 04 are taken at STATION 01: drive out in the truck on the pad, photograph the site, and bring the roll back here.'
          : 'The tank is empty.\nThe field roll for STATION 01 is still in the camera. Bring it back to SARO in the truck.' });
      return true;
    }
    if (this.busy === 'tank') { ui.toast('The tank is sealed. Give it a moment.'); return true; }
    if (this.busy) return true;
    const step = (s.dev === 0 ? 0 : s.dev === 1 ? 2 : 3) as 0 | 2 | 3;
    P.wetBench(ui, { frame: 'FRAMES 03 + 04 / STATION 01 FIELD ROLL', step, onAct: () => {
      if (s.dev === 0) {
        this.busy = 'tank';
        audio.pour();
        ui.toast('Field roll in the sealed tank.', 3);
        this.g.after(TANK_TIME, () => {
          this.busy = null; s.dev = 1; this.save();
          audio.beep(1500, 0.12, 0.05);
          ui.toast('Contact prints ready. Transfer them to the fixer.', 3);
        }, 'lab3');
      } else if (s.dev === 1) {
        this.busy = 'fix';
        audio.pour();
        ch1.fixPrint(s.f3!);
        ui.toast('Prints in the fixer. The images are becoming stable.', 3);
        this.g.after(FIX_TIME, () => {
          this.busy = null; s.dev = 2; this.save();
          audio.beep(1500, 0.12, 0.05);
          ui.toast('Prints fixed. Collect them at the bench.', 3);
        }, 'lab3');
      } else this.collect();
    } });
    return true;
  }

  private collect() {
    const { ch1, audio } = this.d;
    const s = this.s;
    ch1.clearWetPrint();
    ch1.hangPrint(2, s.f3!); ch1.hangPrint(3, s.f4!);
    audio.play('thudSoft', { gain: 0.3, rate: 1.5 });
    s.dev = 3;
    this.file(this.docPhoto(s.f3!)); this.file(this.docPhoto(s.f4!));
    this.g.note('Filed: PHOTO 03, the empty footing and the moved marker A. PHOTO 04, the cable break.');
    this.save();
    P.fieldPrints(this.d.ui, {
      prints: [{ url: s.f3!.url, caption: 'FRAME 03 / FIXED POINT A' }, { url: s.f4!.url, caption: 'FRAME 04 / CABLE BREAK' }],
      text: 'The footing without its marker, and the marker standing where nobody recorded it. The cable, cut clean. Both prints go into the case with the original record, for room 6.',
      onClose: () => this.finish(),
    });
  }

  private finish() {
    const s = this.s;
    if (s.stage === 'complete') return;
    s.stage = 'complete';
    this.g.note('FIELD RECORD FILED: STATION 01, P06 to P09, with frames 03 and 04. Next: room 6 at Sierra Motor Court, with the original 1947 record and the prints.');
    this.d.milestone();
    this.g.after(0.8, () => this.onEnd?.());
  }


  // ---------- documents ----------
  private docTransit(): DocSpec {
    return { id: 'e09a', title: 'E09A / Field transit record', kind: 'typed', stamp: 'E09A', page: TRANSIT, image: this.transitImage(),
      transcript: 'From the steel frame on the transit pier. A plan of the 1947 set-up: A in its footing on the transit sight line, B to the west, C running off the sheet.' };
  }
  private docTiming(): DocSpec {
    return { id: 'e10', title: 'E10 / Field timing record, 1947', kind: 'typed', stamp: 'E10', page: this.s.heard ? `${TIMING_1947}\n\n${TIMING_1986}` : TIMING_1947,
      transcript: 'The ledger on the hut workbench. Times to the second, in pencil. The last line was never filled in.' };
  }
  private docDiagram(): DocSpec {
    return { id: 's1diagram', title: 'STATION 01 / Arrangement on the hut wall', kind: 'typed', stamp: '1947', page: 'A: fixed survey point. B: comparison vane. C: closing sight line.\n\nThe framed copy of the original arrangement, signed N. Vega.', image: D.arrangement(true),
      transcript: 'Behind dusty glass in the hut: the same arrangement as the original 1947 record, with all three references, A, B and C.' };
  }
  private docFinding(k: 'p06' | 'p07' | 'p08' | 'p09'): DocSpec {
    const map = {
      p06: { title: 'P06 / The moved fixed point', page: SUPPORTED.marker, transcript: 'Your finding at the transit. A is not where E09A puts it. Who moved it is not in the record.' },
      p07: { title: 'P07 / Local lamp null test', page: SUPPORTED.lamp, transcript: 'An ordinary control: the field lamp covered, the local condition normal.' },
      p08: { title: 'P08 / The cable break', page: SUPPORTED.cable, transcript: 'The cable from the hut to the pier was cut on purpose. The cut does not say by whom.' },
      p09: { title: 'P09 / Two timing records', page: SUPPORTED.timing, transcript: 'Carrier lost at 02:17:00, relay impact at 02:17:47. The voice tonight is the same correction as in 1947.' },
    }[k];
    return { id: k, kind: 'typed', stamp: k.toUpperCase(), ...map };
  }
  private docPhoto(p: Photo): DocSpec {
    const a = p.id === 'frame03';
    return {
      id: p.id, kind: 'photo', image: p.url, page: '',
      title: a ? 'PHOTO 03 / Fixed point A' : 'PHOTO 04 / Cable break',
      transcript: a
        ? `Exposed at STATION 01 at ${clockText(p.clock)}. The empty footing on the transit sight line, and the marker post with the triangle and bar standing off it.`
        : `Exposed at STATION 01 at ${clockText(p.clock)}. The cable south of the transit pier, cut clean, the two faces opposite each other.`,
    };
  }

  // ---------- per frame ----------
  update(_dt: number, t: number) {
    if (!this.active) return;
    const handset = this.d.travel.site()?.objs?.fieldPhoneHandset;
    if (handset) handset.rotation.z = this.ringing ? Math.sin(t * 70) * 0.05 * (Math.sin(t * 2.2) > -0.2 ? 1 : 0) : 0;
    // the receiver keeps its carrier once it is on, but only out at the station
    const want = this.s.heard && this.d.travel.area() === 'station01';
    if (want !== this.radioOn) {
      this.radioOn = want;
      const o = this.d.travel.site()?.proxies.receiver;
      this.d.audio.radio(want && o ? this.pos(o) : null);
    }
    if (this.callT >= 0 && this.g.gt >= this.callT && !this.g.cinematic && !this.d.ui.modal) { this.callT = -1; this.ring(); }
    if (this.d.travel.area() === 'station01' && !this.d.travel.driving() && !this.s.arrived) this.arrivedStation();
    this.d.ui.objective(this.objective());
  }
}
