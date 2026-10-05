import * as THREE from 'three';
import type { UI, DocSpec } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { ControlRoom, Crt } from '../world/ControlRoom';
import type { Exterior } from '../world/Exterior';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import { Rx, PROFILES, drawSpectrum } from './Signal';
import { RxConsole } from '../ui/RxConsole';
import { M } from '../world/kit';
import { materialFor } from '../core/quality';
import { Chapter1, type CaseState } from './Chapter1';
import { Chapter2, type Ch2State } from './Chapter2';
import { Chapter3, type Ch3State, type Travel } from './Chapter3';
import { Chapter4, type Ch4State, type Motel } from './Chapter4';
import type { Doors } from '../world/Doors';
import { DecoderDesk } from './Decoder';
import type { RecordsAnnex } from '../world/Annex';
import type { ServiceYard } from '../world/ServiceYard';
import type { FieldCamera } from '../core/FieldCamera';
import type { Collider } from '../world/ControlRoom';
import { openTerminal } from '../ui/Terminal';
import { halleyPosterTex } from '../core/textures';
import { BINDER, SERVICE, TELEX_EVENING, telex, interferenceCard, halleyPoster, OpsTerminal, type LogLine } from './nightshift';

// The prologue, "Night Shift". Order of beats follows the Unity PrologueDirector,
// with the story beats from the latest ChatGPT outline layered on top.
const ORDER = ['intro', 'shift', 'survey', 'skip', 'residual', 'locked', 'solving', 'printing', 'printed',
  'ringing', 'call', 'countdown', 'event', 'turning', 'end', 'ch1', 'ch2', 'ch3', 'ch4'] as const;
export type Phase = typeof ORDER[number];

const START_CLOCK = 23 * 3600 + 41 * 60;        // 23:41:00
const SKIP_CLOCK = 2 * 3600 + 13 * 60 + 41;     // 02:13:41, residual shows at 02:13:47
const EVENT_AZ = 26, EVENT_EL = 32;
const PARK_AZ = 18, SURVEY_EL = 48;

import { pad, clockText } from './time';
export { clockText };

const LOG: DocSpec = {
  id: 'log', title: "Dale's shift log", kind: 'hand', stamp: 'NIGHT SHIFT',
  page: `23:20  handover

Kid,
1. RX bank 3 tripped again. Big red lever on the rack by the printer.
2. Calibrate on 1419.900 first. Card is taped to the console.
3. Spike near 1420.1 is the highway relay. Notch it, log it, don't chase it.
4. Then let the survey sweep run. Nothing scheduled till 06:00. Babysit it.

Coffee is fresh-ish.
Storm out past the Magdalenas. Should stay there.

Don't break anything.
        D.`,
  transcript: `Torn out of the night shift logbook. Dale writes like he is already halfway to his car.

"Kid. One: RX bank 3 tripped again. Big red lever on the rack by the printer. Two: calibrate on 1419.900 first. Card is taped to the console. Three: the spike near 1420.1 is the highway relay. Notch it, log it, don't chase it. Four: then let the survey sweep run. Nothing scheduled till 06:00. Babysit it."

"Coffee is fresh-ish. Storm out past the Magdalenas. Should stay there. Don't break anything. D."`,
};

// Under Dale's log: what the night is for. Ward's note is her opening line from the design bible (V1).
const WORK_ORDER: DocSpec = {
  id: 'workorder', title: 'Night work order', kind: 'typed', stamp: 'WORK ORDER',
  page: `SARO / OPERATIONS
NIGHT WORK ORDER   04/13/86

OPERATOR:  REYES
ON CALL:   DR. E. WARD, SHIFT SUPERVISOR

1. RESTORE RX BANK 3.
   CALIBRATE ON 1419.900 MHZ.
2. RUN THE SURVEY SWEEP.
   LOG ANYTHING OFF SCHEDULE.
3. MORNING SERIES 06:00.
   IT DOES NOT RUN UNTIL EVERY
   ANOMALY TONIGHT IS DOCUMENTED
   AND SIGNED.`,
  transcript: `A typed work order with your name on it, initialled in the corner by the shift supervisor.

Along the bottom, in Ward's handwriting: "Before the morning series, I need a report I can sign. Start with calibration. Keep the paper if it gives you anything you cannot account for. E.W."`,
};

function printout(clock: number): DocSpec {
  const lines = [
    'SARO RX/DSP   DIRECTION SOLVE',
    '------------------------------',
    `DATE    04/14/86   ${clockText(clock)}`,
    'FREQ    1420.405 MHz',
    'BW      12 kHz',
    'PATTERN PULSE GROUP 4 / 7',
    'RA      05h 17m 32s',
    'DEC    -05  23\' 14"',
    'S/N     4.71',
    'SOURCE  UNKNOWN',
    '',
    'SOURCE DISTANCE:  -39 LY',
    '',
    '** CHECK SOLVE INPUTS **',
  ];
  return {
    id: 'printout', title: 'Direction solve printout', kind: 'printout', page: lines.join('\n'),
    transcript: `Fan-fold paper, still warm from the print head.

Everything above the last lines reads like an ordinary solve: the hydrogen line, a pulse pattern, a position in Orion.

The distance field is negative. Minus thirty-nine light years. The solver has no way of printing a negative distance. It was never written to.`,
  };
}

// What Night Shift leaves behind besides the notes: which of the room's papers were read,
// whether relay K3 is still open, and tonight's lines in the console log (story/nightshift.ts).
export interface NightShiftState { papers: string[]; k3Open: boolean; log: LogLine[]; solveAt: number | null; jacket?: boolean }
export interface SavedCase { s: CaseState; ch2?: Ch2State; ch3?: Ch3State; ch4?: Ch4State; ns?: NightShiftState; notes: string[]; clock: number }
export interface PrologueDeps {
  ui: UI; audio: AudioSys; room: ControlRoom; ext: Exterior; player: Player; inter: Interaction;
  yard: ServiceYard; fcam: FieldCamera; colliders: Collider[]; annex: RecordsAnnex;
  view: { restore: () => void; draw: () => void };
  saveCase: (c: SavedCase | null) => void;
  loadCase: () => SavedCase | null;
  isTouch: () => boolean;
  travel: Travel;
  motel: Motel;
  doors: Doors;
  milestone: () => void;
}

export class Prologue {
  phase: Phase = 'intro';
  rx = new Rx();
  rxc: RxConsole;
  gt = 0;                      // game time, stops while paused or reading
  clock = START_CLOCK;
  cinematic = false;           // locks movement and interaction
  logRead = false;
  coffee: 'none' | 'poured' | 'drunk' = 'none';
  powered = false;
  answered = false;
  notes: string[] = [];
  docs: DocSpec[] = [];
  onFinish?: () => void;
  onCheckpoint?: (name: string) => void;
  ch1: Chapter1;
  ch2: Chapter2;
  ch3: Chapter3;
  ch4: Chapter4;
  decoder: DecoderDesk;
  eventClock = 2 * 3600 + 15 * 60 + 12; // when the dishes turned; the S-03 log is stamped with it
  ns: NightShiftState = { papers: [], k3Open: false, log: [], solveAt: null, jacket: false };
  radioOn = false;
  private radioHeard = false;
  private lineDeadClock = 0;
  private opsLines: string[] = [];
  private opsDirty = true;
  private opsT = 0;
  private halleyImg: string | undefined;
  readonly ops = new OpsTerminal(() => ({ clock: this.clock, solveAt: this.ns.solveAt, log: this.ns.log, future: false }));

  private timers: { at: number; fn: () => void; tag?: string }[] = [];
  private bootT = -1;
  private solveT = -1;
  private printT = -1;
  private lineDeadT = -1;
  private alarmOn = false;
  private skipQueued = false;
  private flicker = 0;
  private crtGlitch = 0;
  private mugFall: { t: number; v: THREE.Vector3; spin: number; stage: 0 | 1 } | null = null;
  private shards: { m: THREE.Mesh; v: THREE.Vector3; w: THREE.Vector3; rest: boolean }[] = [];
  private spill: THREE.Mesh | null = null;
  private spillT = 0;
  private lookAssist = 0;
  private ledT = 0;
  private scan: HTMLCanvasElement;
  private base: { ceiling: number[]; lamps: number[] };
  private d: PrologueDeps;

  constructor(d: PrologueDeps) {
    this.d = d;
    this.rxc = new RxConsole(d.ui, this.rx);
    this.rxc.onClick = () => d.audio.play('click', { gain: 0.5 });
    this.rxc.onAzimuth = (az) => this.pointDishes(az, SURVEY_EL, 9);
    this.rxc.onAction = () => this.stagePassed();
    this.base = { ceiling: d.room.lights.ceiling.map((l) => l.intensity), lamps: d.room.lights.lamps.map((l) => l.intensity) };
    // scanline overlay, drawn once and stamped on every CRT frame
    this.scan = document.createElement('canvas');
    this.scan.width = 512; this.scan.height = 384;
    const g = this.scan.getContext('2d')!;
    for (let y = 0; y < 384; y += 3) { g.fillStyle = 'rgba(0,0,0,.22)'; g.fillRect(0, y, 512, 1); }
    const v = g.createRadialGradient(256, 192, 120, 256, 192, 330);
    v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,.55)');
    g.fillStyle = v; g.fillRect(0, 0, 512, 384);
    this.interactables();
    // the signal processor holds the tape once the anomaly is locked (02:14 and after)
    this.decoder = new DecoderDesk({ ui: d.ui, audio: d.audio, room: d.room, inter: d.inter,
      locked: () => this.rx.stage >= 3, recorded: () => '02:14', busy: () => this.cinematic, power: () => this.powered && !this.ns.k3Open });
    this.ch1 = new Chapter1(this, {
      ui: d.ui, audio: d.audio, room: d.room, ext: d.ext, player: d.player, inter: d.inter,
      yard: d.yard, fcam: d.fcam, colliders: d.colliders, view: d.view, isTouch: d.isTouch,
      save: (c) => d.saveCase(c ? this.composeCase(c) : null), doors: d.doors,
    });
    this.ch2 = new Chapter2(this, {
      ui: d.ui, audio: d.audio, room: d.room, annex: d.annex, player: d.player, inter: d.inter, colliders: d.colliders, doors: d.doors,
      save: () => d.saveCase(this.composeCase(this.ch1.s)),
    });
    this.ch3 = new Chapter3(this, {
      ui: d.ui, audio: d.audio, player: d.player, inter: d.inter, fcam: d.fcam, ch1: this.ch1, view: d.view, travel: d.travel,
      save: () => d.saveCase(this.composeCase(this.ch1.s)),
      milestone: () => { this.onCheckpoint?.('chapter3'); },
    });
    this.ch4 = new Chapter4(this, {
      ui: d.ui, audio: d.audio, player: d.player, inter: d.inter, doors: d.doors,
      ch1: this.ch1, ch2: this.ch2, ch3: this.ch3, motel: d.motel,
      save: () => d.saveCase(this.composeCase(this.ch1.s)),
      milestone: () => { this.onCheckpoint?.('chapter4'); },
    });
    this.reset();
  }

  at(p: Phase) { return ORDER.indexOf(this.phase) >= ORDER.indexOf(p); }

  // The doors are free at any time (world/Doors.ts); the chapters that care remember them.
  doorChanged(id: string, open: boolean) {
    this.ch1.doorChanged(id, open);
    this.ch2.doorChanged(id, open);
    this.ch4.doorChanged(id, open);
  }

  // ---------- saving ----------
  // Where a save resumes. The prologue only has safe points at the start of the night and
  // at 02:13; inside a chapter the chapter's own state is the resume point.
  checkpointName(): string {
    if (this.phase === 'ch4') return 'chapter4';
    if (this.phase === 'ch3') return 'chapter3';
    if (this.phase === 'ch2') return 'chapter2';
    if (this.phase === 'ch1') return 'chapter1';
    return this.at('residual') ? 'residual' : 'start';
  }
  /** Why the game cannot be saved right now, or null when it can. */
  saveBlock(): string | null {
    if (!this.at('residual')) return 'The night can be saved from 02:13, once the survey has been running for a while.';
    if (this.cinematic) return 'Not right now. Let the moment finish first.';
    if (this.at('ringing') && !this.at('ch1')) return 'Not during the call and what follows it. The game saves again when the night moves on.';
    if (this.phase === 'ch3') return this.ch3.saveBlock();
    if (this.phase === 'ch4') return this.ch4.saveBlock();
    return null;
  }
  /** The case as it stands, for a save. Null in the prologue, which keeps no case. */
  snapshotCase(): SavedCase | null { return this.at('ch1') ? this.composeCase(this.ch1.s) : null; }
  chapterTitle(): string {
    if (this.phase === 'ch4') return 'Chapter 4: Room 6';
    if (this.phase === 'ch3') return 'Chapter 3: The Survey Station';
    if (this.phase === 'ch2') return 'Chapter 2: The Amended Record';
    if (this.phase === 'ch1') return 'Chapter 1: The Second Exposure';
    return 'Prologue: Night Shift';
  }
  // One saved case for every chapter: a later chapter's state rides along once it has begun.
  private composeCase(s: CaseState): SavedCase {
    return { s, ch2: this.ch2?.started ? this.ch2.s : undefined, ch3: this.ch3?.started ? this.ch3.s : undefined, ch4: this.ch4?.started ? this.ch4.s : undefined,
      ns: { papers: [...this.ns.papers], k3Open: this.ns.k3Open, log: this.ns.log.map((l) => [l[0], l[1]] as LogLine), solveAt: this.ns.solveAt, jacket: !!this.ns.jacket }, notes: [...this.notes], clock: this.clock };
  }
  private setPhase(p: Phase) { this.phase = p; }

  // ---------- timers on game time ----------
  after(s: number, fn: () => void, tag?: string) { this.timers.push({ at: this.gt + s, fn, tag }); }
  cancel(tag: string) { this.timers = this.timers.filter((t) => t.tag !== tag); }

  // ---------- notebook ----------
  tasks() {
    if (this.phase === 'ch1') return this.ch1.tasks();
    if (this.phase === 'ch2') return this.ch2.tasks();
    if (this.phase === 'ch3') return this.ch3.tasks();
    if (this.phase === 'ch4') return this.ch4.tasks();
    const t: { text: string; done: boolean }[] = [{ text: "Read Dale's shift log", done: this.logRead }];
    if (this.logRead || this.at('residual')) {
      t.push({ text: 'Power up RX bank 3', done: this.powered });
      t.push({ text: 'Calibrate on 1419.900 MHz', done: this.rx.stage >= 1 });
      t.push({ text: 'Notch the highway relay near 1420.1', done: this.rx.stage >= 2 });
      t.push({ text: 'Babysit the survey until 06:00', done: this.at('residual') });
      t.push({ text: 'Coffee (optional)', done: this.coffee !== 'none' });
    }
    if (this.at('residual')) t.push({ text: 'Find the residual near 1420.4', done: this.rx.stage >= 3 });
    if (this.rx.stage >= 3) t.push({ text: 'Run a direction solve on the tracking terminal', done: this.rx.solved });
    if (this.rx.solved) t.push({ text: 'Tear off the printout', done: this.at('ringing') });
    if (this.at('ringing')) t.push({ text: 'Answer the phone', done: this.answered });
    return t;
  }
  note(s: string) { if (!this.notes.includes(s)) this.notes.push(s); }
  openNotebook() {
    const ui = this.d.ui;
    ui.notebook(this.tasks(), this.notes, this.docs, (doc) => ui.document(doc, () => {}), this.phase === 'ch1' ? 'Chapter one' : this.phase === 'ch2' ? 'Chapter two' : this.phase === 'ch3' ? 'Chapter three' : this.phase === 'ch4' ? 'Chapter four' : 'Tonight',
      // the evidence board can be laid out from the journal once a record from the archive is in it
      this.ch2.started && Object.values(this.ch2.s.read).some(Boolean) ? () => this.ch2.openBoard() : undefined);
  }

  // ---------- world helpers ----------
  private pointDishes(az: number, el: number, slew: number) {
    for (const dish of this.d.ext.dishes) dish.point(az, el, slew);
  }
  private worldPos(o: THREE.Object3D) { return o.getWorldPosition(new THREE.Vector3()); }

  private setLeds(on: boolean) {
    const room = this.d.room;
    for (let i = 0; i < room.ledCount; i++) room.setLed(i, on ? room.ledState(i) : 'off');
  }

  // ---------- interactables ----------
  private interactables() {
    const { room, ui, audio, inter } = this.d;
    const o = room.objs;
    const toast = (s: string) => ui.toast(s);

    inter.add({ id: 'logbook', object: o.logbook, label: () => this.logRead ? 'Read the shift log' : "Read Dale's shift log",
      use: () => ui.document(LOG, () => this.logClosed()) });

    inter.add({ id: 'workOrder', object: o.workOrder, label: () => 'Read the work order',
      use: () => ui.document(WORK_ORDER, () => this.workOrderClosed()) });

    inter.add({ id: 'coffeePot', object: o.coffeePot, label: () => this.at('event') ? 'Coffee machine' : this.coffee === 'none' ? 'Fill your mug' : 'Coffee machine',
      use: () => {
        if (this.coffee !== 'none' || this.at('event')) { toast('The hot plate ticks. It has been on since the day shift.'); return; }
        audio.pour(this.worldPos(o.coffeePot));
        this.coffee = 'poured';
        this.after(1.5, () => { room.coffee.visible = true; room.coffee.position.y = 0.088; toast('You fill the SARO mug. Fresh-ish, like he said.'); });
      } });

    inter.add({ id: 'mug', object: o.mug, label: () => !room.mug.visible ? null : this.coffee === 'poured' ? 'Drink coffee' : 'SARO mug',
      use: () => {
        if (this.coffee === 'poured') { this.coffee = 'drunk'; room.coffee.position.y = 0.05; audio.play('thudSoft', { gain: 0.25, at: this.worldPos(o.mug) }); toast('Burnt. It helps.'); }
        else if (this.coffee === 'drunk') toast('Half a cup left. Still burnt.');
        else toast('Empty. The coffee machine is in the corner.');
      } });

    inter.add({ id: 'rack', object: o.rack, label: () => this.ns.k3Open ? 'Reset RX bank 3' : this.powered ? 'RX bank 3' : 'Power up RX bank 3',
      use: () => {
        if (this.ns.k3Open) { this.resetK3(); return; }
        if (this.powered) { toast('Bank 3 online. Sixteen green, two amber. Amber is normal, according to Dale.'); return; }
        this.powerUp(false);
      } });

    inter.add({ id: 'crtCenter', object: o.crtCenter, range: 2.6,
      label: () => this.cinematic ? null : !this.powered ? 'Receiver console' : this.alarmOn ? 'Check the spectrum' : 'Use receiver console',
      use: () => {
        if (!this.powered) { toast('Dead screen. RX bank 3 is still off.'); audio.play('click', { gain: 0.4 }); return; }
        if (this.ns.k3Open) { toast('Dead screen. Bank 3 has tripped again. The lever is on the rack.'); audio.play('click', { gain: 0.4 }); return; }
        this.alarmOn = false;
        audio.play('click', { gain: 0.5 });
        this.rxc.show(() => this.consoleClosed());
      } });

    inter.add({ id: 'crtLeft', object: o.crtLeft, range: 2.6, label: () => 'Status terminal',
      use: () => {
        if (!this.powered) { toast('No power.'); return; }
        if (this.at('event')) toast('S-01 to S-27: no control command recorded.');
        else if (this.rx.stage >= 2) toast(`Array on survey schedule. Az ${pad(this.d.ext.dishes[0].curAz, 3)}, el ${SURVEY_EL}. Nothing logged since midnight.`);
        else toast('Array parked. Receiver needs calibrating before the survey can start.');
      } });

    inter.add({ id: 'crtRight', object: o.crtRight, range: 2.6,
      label: () => this.phase === 'locked' ? 'Run direction solve' : this.phase === 'solving' ? null : 'Tracking terminal',
      use: () => {
        if (!this.powered) { toast('No power.'); return; }
        if (this.phase === 'locked') { this.startSolve(); return; }
        if (this.rx.stage < 3) toast('Direction solve needs a locked source. Nothing worth solving.');
        else if (this.at('countdown')) toast('It is counting from when the line went dead.');
        else toast('Solve finished. The printer has the rest.');
      } });

    inter.add({ id: 'printer', object: o.printer,
      label: () => this.phase === 'printed' ? 'Tear off the printout' : this.phase === 'printing' ? null : 'Printer',
      use: () => {
        if (this.phase === 'printed') this.tearPrintout();
        else toast('Dot matrix printer. A box of fan-fold paper under the table.');
      } });

    inter.add({ id: 'phone', object: o.phone,
      label: () => this.phase === 'ch2' ? this.ch2.phoneLabel() : this.ringing ? 'Answer the phone' : this.phase === 'call' ? null : 'Telephone',
      use: () => {
        if (this.phase === 'ch2') { this.ch2.usePhone(); return; }
        if (this.phase === 'ch3' || this.phase === 'ch4') { toast(this.phase === 'ch3' ? 'The supervisor line is quiet. Ward wants a field record, not a phone call.' : 'The supervisor line is quiet. N. Vega is waiting across the road.'); return; }
        if (this.ringing) { this.answer(); return; }
        if (this.answered) toast('Dead line. Not even a dial tone.');
        else toast('Internal line and one outside line. Nobody calls out here after midnight.');
      } });

    inter.add({ id: 'clock', object: o.clock, range: 3.5, label: () => 'Wall clock', use: () => toast(`${clockText(this.clock, false)}. ${this.at('residual') ? 'Where did the night go?' : 'Six hours and change to go.'}`) });
    inter.add({ id: 'map', object: o.map, range: 2.8, label: () => 'Map of New Mexico',
      use: () => toast('SARO is the red X on the plains. Somebody drew a ring around Roswell and a question mark.') });

    // ---------- what Night Shift adds (KAPITLER.md) ----------
    const paper = (id: string, object: THREE.Object3D, label: string, doc: () => DocSpec, range = 2.4) =>
      inter.add({ id, object, range, label: () => this.cinematic ? null : label, use: () => { const d = doc(); ui.document(d, () => this.filePaper(d)); } });
    paper('excBinder', o.binder, 'Exceptions binder', () => BINDER);   // not 'binder': that is the archive's service copies
    paper('serviceRecord', o.serviceRecord, 'Service record', () => SERVICE);
    paper('telex', o.telex, 'Telex roll', () => telex(TELEX_EVENING));
    paper('rfiCard', o.rfiCard, "Dale's card", () => interferenceCard(this.rx.stage >= 3));
    paper('halley', o.halley, "Halley's comet poster", () => halleyPoster(this.halleyImage()), 2.8);
    inter.add({ id: 'opsTerminal', object: o.opsTerminal, range: 2.4, label: () => this.cinematic ? null : 'Operations terminal', use: () => this.openOps() });
    inter.add({ id: 'jacket', object: o.jacket, range: 2.4, label: () => this.cinematic || this.ns.jacket ? null : 'Your jacket',
      use: () => ui.toast('SARO issue, with a reflective band all the way round. Too warm for in here.', 3.2) });
    inter.add({ id: 'radio', object: o.radio, range: 2.6, label: () => this.radioOn ? 'Turn the radio off' : 'Turn the radio on', use: () => this.toggleRadio() });
  }

  private filePaper(doc: DocSpec) {
    if (!this.ns.papers.includes(doc.id)) this.ns.papers.push(doc.id);
    const i = this.docs.findIndex((x) => x.id === doc.id);
    if (i < 0) this.docs.push(doc); else this.docs[i] = doc;
  }
  private paperById(id: string): DocSpec | null {
    return ({ binder: BINDER, service: SERVICE, telex: telex(TELEX_EVENING), rfiCard: interferenceCard(this.rx.stage >= 3), halley: halleyPoster(this.halleyImage()) } as Record<string, DocSpec>)[id] ?? null;
  }
  // the poster as a picture for the document view; kept once it is drawn on Codex's picture
  private halleyImage() {
    if (this.halleyImg) return this.halleyImg;
    const t = halleyPosterTex();
    const url = (t.image as HTMLCanvasElement).toDataURL('image/jpeg', 0.9);
    if (t.userData.fromArt) this.halleyImg = url;
    t.dispose();
    return url;
  }

  // the console log: what the operations terminal shows under SHOW LOG
  private log(text: string) { this.ns.log.push([Math.round(this.clock), text]); this.opsDirty = true; }

  private openOps() {
    const { ui, audio } = this.d;
    audio.play('click', { gain: 0.4 });
    openTerminal(ui, {
      header: () => this.ops.header(),
      run: (c) => { const r = this.ops.run(c); this.opsLines = ['$ ' + c.toUpperCase(), ...r.split('\n')].slice(-9); this.opsDirty = true; return r; },
      buttons: () => this.ops.buttons(),
      touch: this.d.isTouch(),
      onCommand: () => audio.beep(1400, 0.02, 0.02),
    });
  }

  private toggleRadio() {
    const { audio, room, ui } = this.d;
    this.radioOn = !this.radioOn;
    audio.play('click', { gain: 0.35, rate: 1.3 });
    audio.amRadio(this.radioOn ? this.worldPos(room.objs.radio).setY(1.45) : null);
    if (this.radioOn && !this.radioHeard) { this.radioHeard = true; ui.toast('A station a long way off, coming and going in the static.', 3); }
    if (this.radioOn && (this.at('countdown') && !this.at('turning'))) audio.amRadioJam(true);
  }

  // Relay K3 drops RX bank 3 out, seven seconds before the impact (KAPITLER.md: the
  // service record says it trips before the power goes, not after).
  private tripK3() {
    const { room, audio } = this.d;
    if (!this.powered || this.ns.k3Open) return;
    this.ns.k3Open = true;
    this.log('RX BANK 3  K3 OPEN');
    const at = this.worldPos(room.objs.rack);
    audio.play('switch', { gain: 1.0, rate: 0.62, at });
    audio.play('thudSoft', { gain: 0.45, rate: 1.4, at, when: 0.03 });
    this.decoder.stop();
    this.applyK3(true);
  }
  private applyK3(open: boolean) {
    const { room, audio } = this.d;
    this.rx.powered = !open;
    room.lever.rotation.z = open ? -0.6 : 0.6;
    this.setLeds(!open);
    room.crtCenter.setPowered(!open);
    room.lights.crt.intensity = open ? 0.45 : 0.9;
    if (open && audio.ctx) { audio.setCarrier(0, 0, 0); audio.signalOff(); }
  }
  private resetK3() {
    const { room, audio, ui } = this.d;
    if (this.cinematic) return;
    this.ns.k3Open = false;
    this.log('RX BANK 3  K3 CLOSED');
    const at = this.worldPos(room.objs.rack);
    audio.play('switch', { gain: 0.9, at });
    audio.play('thudSoft', { gain: 0.5, at, when: 0.12 });
    this.applyK3(false);
    room.lever.rotation.z = -0.6;
    let k = 0;
    const anim = () => { k++; room.lever.rotation.z = -0.6 + Math.min(1, k / 8) * 1.2; if (k < 8) requestAnimationFrame(anim); };
    anim();
    ui.toast('Bank 3 back on. Sixteen green, two amber.', 2.6);
  }

  // ---------- beats ----------
  private workOrderClosed() {
    if (this.docs.includes(WORK_ORDER)) return;
    this.docs.push(WORK_ORDER);
    this.note('Work order: Reyes on nights, Dr. Ward on call. The morning series at 06:00 waits for a report she can sign.');
  }

  private logClosed() {
    if (this.logRead) return;
    this.logRead = true;
    this.docs.push(LOG);
    if (this.phase === 'intro') this.setPhase('shift');
    this.after(0.6, () => this.d.ui.toast(this.powered ? 'Bank 3 is already up. Calibration next.' : 'First the rack. Bank 3 is on the east wall, by the printer.'));
  }

  powerUp(instant: boolean) {
    const { room, audio } = this.d;
    this.powered = true;
    this.rx.powered = true;
    if (!instant) this.log('RX BANK 3  K3 CLOSED');
    room.lever.rotation.z = 0.6;
    audio.startCarrier();
    if (instant) {
      this.setLeds(true);
      [room.crtLeft, room.crtCenter, room.crtRight].forEach((c) => c.setPowered(true));
      room.lights.crt.intensity = 0.9;
      this.bootT = -10;
      return;
    }
    const at = this.worldPos(room.objs.rack);
    audio.play('switch', { gain: 0.9, at });
    audio.play('thudSoft', { gain: 0.5, at, when: 0.12 });
    room.lever.rotation.z = -0.6;
    let k = 0;
    const animLever = () => { k++; room.lever.rotation.z = -0.6 + Math.min(1, k / 8) * 1.2; if (k < 8) requestAnimationFrame(animLever); };
    animLever();
    for (let i = 0; i < room.ledCount; i++) this.after(0.3 + i * 0.07, () => { room.setLed(i, room.ledState(i)); if (i % 6 === 0) audio.beep(900 + i * 20, 0.03, 0.03); });
    this.after(1.6, () => {
      [room.crtLeft, room.crtCenter, room.crtRight].forEach((c) => c.setPowered(true));
      room.lights.crt.intensity = 0.9;
      this.bootT = this.gt;
      audio.beep(1500, 0.06, 0.05);
      this.d.ui.toast('RX bank 3 hums awake. The console screens warm up.');
    });
    if (!this.logRead) this.after(4.5, () => this.d.ui.toast("Dale's shift log is still on the desk."));
  }

  private stagePassed() {
    const { ui, audio } = this.d;
    const s = this.rx.stage;
    this.note(PROFILES[Math.min(s, 2)].note);
    this.rx.advance();
    this.pointDishes(this.rx.azimuth, SURVEY_EL, 6);
    audio.beep(1320, 0.07, 0.07); audio.beep(1760, 0.09, 0.07, 0.1);
    if (s === 0) ui.toast('Calibrated. Next, the highway relay near 1420.1.');
    else if (s === 1) { this.setPhase('survey'); ui.toast('Notched. The survey sweep takes over from here.'); }
    else if (s === 2) {
      this.setPhase('locked');
      this.note('02:14. Pulse group: four, a gap, seven. Repeats every 5.2 seconds. Not the relay. Not anything in the schedule.');
      ui.toast('Four pulses. A gap. Seven pulses. Again.', 4);
      this.after(4.5, () => { if (this.phase === 'locked') ui.toast('The tracking terminal can run a direction solve.'); });
    }
  }

  private consoleClosed() {
    if (this.rx.stage === 2 && this.phase === 'survey' && !this.skipQueued) {
      this.skipQueued = true;
      this.after(2.5, () => this.d.ui.toast('Nothing to do now but watch a sweep that takes all night.', 3.5));
      this.after(7.5, () => this.timeSkip(), 'skip');
    }
  }

  private timeSkip() {
    const { ui, player } = this.d;
    this.setPhase('skip');
    this.cinematic = true;
    ui.fade(true, '02:13');
    // Game-time timers, so a pause holds the skip and reset() cancels it.
    this.after(3.6, () => {
      ui.close();
      this.clock = SKIP_CLOCK;
      this.coffee = this.coffee === 'none' ? 'none' : 'drunk';
      player.place(0.05, -2.2, 0);
      this.enterResidual();
      ui.fade(false);
      this.after(0.9, () => { this.cinematic = false; });
    });
  }

  private enterResidual() {
    this.setPhase('residual');
    this.onCheckpoint?.('residual');
    this.after(6, () => {
      this.rxc.residualVisible = true;
      this.rx.status = 'UNLOGGED RESIDUAL  /  1420.4 MHz';
      this.alarmOn = true;
      this.note('02:13:47. Unlogged residual on the spectrum near 1420.4. Survey schedule says nothing should be there.');
      this.d.ui.toast('02:13:47. The console is beeping.', 3.5);
      this.alarm();
    });
  }

  private alarm() {
    if (!this.alarmOn) return;
    this.d.audio.beep(1980, 0.09, 0.06); this.d.audio.beep(1980, 0.09, 0.06, 0.18);
    this.after(3.2, () => this.alarm(), 'alarm');
  }

  private startSolve() {
    const { audio } = this.d;
    this.setPhase('solving');
    this.solveT = this.gt;
    for (let i = 0; i < 14; i++) audio.beep(600 + (i % 4) * 180, 0.025, 0.035, i * 0.28);
    this.after(4.2, () => {
      this.rx.solved = true;
      this.ns.solveAt = Math.round(this.clock);
      this.log('SOLVE  RANGE NEGATIVE');
      this.note('Direction solve: RA 05h 17m, Dec -05. Somewhere in Orion.');
      audio.beep(1500, 0.12, 0.06);
      this.startPrint();
    });
  }

  private startPrint() {
    const { room, audio } = this.d;
    this.setPhase('printing');
    this.printT = this.gt;
    room.printerPaper.visible = true;
    room.printerPaper.scale.y = 0.001;
    audio.loop('printer', 'printer', { gain: 0.8, at: this.worldPos(room.objs.printer) });
    this.after(5.4, () => {
      audio.stop('printer', 0.04);
      this.setPhase('printed');
      this.d.ui.toast('The printer stops.');
    });
  }

  private tearPrintout() {
    const { ui, audio, room } = this.d;
    audio.play('thudSoft', { gain: 0.35, rate: 1.6, at: this.worldPos(room.objs.printer) });
    room.printerPaper.visible = false;
    const doc = printout(this.clock);
    this.docs.push(doc);
    this.setPhase('ringing');
    ui.document(doc, () => {
      this.note('Source distance on the solve: -39 light years. A negative distance. The solver cannot produce that.');
      this.after(3.0, () => this.ring());
    });
    // the bell starts a few seconds after the printout is put down
    this.ringPending = true;
  }
  private ringPending = false;
  private ringing = false;

  private ring() {
    if (!this.ringPending) return;
    this.ringPending = false;
    this.ringing = true;
    const { audio, ui, room } = this.d;
    audio.loop('ring', 'phoneRing', { gain: 1.0, at: this.worldPos(room.objs.phone) });
    ui.toast('The phone. At this hour.');
  }

  private answer() {
    const { audio, ui, room } = this.d;
    this.ringing = false;
    audio.stop('ring', 0.02);
    audio.play('click', { gain: 0.7 });
    this.answered = true;
    this.setPhase('call');
    this.cinematic = true;
    room.handset.visible = false;
    const dur = audio.futureCall();
    const cap = (s: number, text: string, secs = 2.2) => this.after(s, () => ui.toast(text, secs));
    cap(0.15, 'Static. Then a room. A printer running somewhere in it.', 2.4);
    cap(2.0, 'A deep thud. Somebody catches their breath.', 2.0);
    cap(2.6, 'Something ceramic breaks.', 1.8);
    this.after(dur + 0.15, () => {
      ui.toast('Click. The line is dead.', 3);
      room.handset.visible = true;
      audio.play('thudSoft', { gain: 0.5, at: this.worldPos(room.objs.phone) });
      this.cinematic = false;
      this.note(`${clockText(this.clock, false)}. Outside line. No voice. Room tone, a printer, a heavy thud, a gasp, something ceramic breaking. Then nothing.`);
      this.startCountdown();
    });
  }

  private startCountdown() {
    const { audio, room } = this.d;
    this.setPhase('countdown');
    this.lineDeadT = this.gt;
    this.lineDeadClock = this.clock;
    this.log('STATION CLOCK  WWVB REFERENCE LOST');
    if (this.radioOn) audio.amRadioJam(true);
    this.after(40, () => this.tripK3());
    this.after(16, () => audio.beep(52, 2.8, 0.09, 0, 'sine'));
    this.after(31, () => audio.play('printer', { gain: 0.35, rate: 1.35, at: this.worldPos(room.objs.printer) }));
    this.after(47, () => this.event());
  }

  private event() {
    const { audio, player, ui } = this.d;
    this.setPhase('event');
    this.eventClock = this.clock;
    this.log('LINE POWER  DIP 1.2 S');
    this.log('STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47');
    if (this.radioOn) this.after(3.5, () => audio.amRadioJam(false));
    audio.boom(1.25);
    audio.gasp(undefined, 0.3);
    player.shake = 1.25;
    this.flicker = 1.8;
    this.crtGlitch = 0.5;
    this.dropMug();
    this.after(1.4, () => ui.toast('The floor is still shaking.', 2.4));
    this.after(3.0, () => this.turnDishes());
  }

  private dropMug() {
    const { room } = this.d;
    if (!room.mug.visible) return;
    this.mugFall = { t: 0, v: new THREE.Vector3(0, 0, -0.5), spin: 6 + Math.random() * 3, stage: 0 };
  }

  private shatter(p: THREE.Vector3) {
    const { room, audio } = this.d;
    room.mug.visible = false;
    audio.play('ceramic', { gain: 1.0, at: p.clone().setY(0.2) });
    const geos = [new THREE.TetrahedronGeometry(0.028), new THREE.BoxGeometry(0.04, 0.008, 0.03), new THREE.TetrahedronGeometry(0.02)];
    for (let i = 0; i < 11; i++) {
      const m = new THREE.Mesh(geos[i % 3], M.ceramic);
      m.position.copy(p).add(new THREE.Vector3((Math.random() - 0.5) * 0.06, 0.03, (Math.random() - 0.5) * 0.06));
      const a = Math.random() * Math.PI * 2, sp = 0.5 + Math.random() * 1.3;
      room.group.add(m);
      this.shards.push({ m, v: new THREE.Vector3(Math.cos(a) * sp, 0.6 + Math.random() * 1.2, Math.sin(a) * sp), w: new THREE.Vector3(Math.random() * 20, Math.random() * 20, Math.random() * 20), rest: false });
    }
    if (this.coffee !== 'none') {
      this.spill = new THREE.Mesh(new THREE.CircleGeometry(0.28, 20), new THREE.MeshStandardMaterial({ color: 0x170c05, roughness: 0.08, transparent: true, opacity: 0.85 }));
      this.spill.rotation.x = -Math.PI / 2;
      this.spill.position.set(p.x, 0.004, p.z);
      this.spill.scale.setScalar(0.1);
      this.spillT = 0;
      room.group.add(this.spill);
    }
  }

  private turnDishes() {
    const { audio, ui } = this.d;
    this.setPhase('turning');
    this.cinematic = true;
    this.lookAssist = 1;
    audio.startMotors();
    this.log('ARRAY  S-01 TO S-27 MOTION  NO COMMAND');
    this.pointDishes(EVENT_AZ, EVENT_EL, 2.2);
    ui.toast('Outside, every dish in the array is turning.', 3.5);
    this.after(4.5, () => ui.toast('Nobody sent a command.', 3));
  }

  private arrived() {
    const { audio } = this.d;
    this.setPhase('end');
    audio.stopMotors();
    audio.beep(1760, 0.18, 0.08); audio.beep(1760, 0.18, 0.08, 0.3); audio.beep(2350, 0.4, 0.08, 0.6);
    this.note('Every dish moved to az 026, el 32. Console shows no control command.');
    this.note('All dishes left their scheduled track and aligned together.');
    this.after(3.2, () => this.finish());
  }

  private finish() {
    const { ui, audio } = this.d;
    ui.fade(true, '');
    audio.setCarrier(0, 0, 0); audio.signalOff();
    this.after(1.6, () => { this.cinematic = true; this.onFinish?.(); });
  }

  // ---------- chapter one ----------
  // From the end of the prologue (or a checkpoint): the night continues in the yard.
  beginChapter1(saved: SavedCase | null) {
    const { player, ui } = this.d;
    this.ch2.reset(); // a new or restored chapter one; chapter two begins after it
    this.setPhase('ch1');
    this.cinematic = false;
    this.lookAssist = 0;
    this.timers = []; // nothing from the prologue may fire inside the chapter
    if (saved) { this.notes = [...saved.notes]; this.clock = saved.clock; }
    else player.place(3.4, 1.4, -Math.PI / 2);
    ui.fade(false);
    this.ch1.begin(saved?.s ?? null, saved?.s.eventClock ?? this.eventClock);
    this.onCheckpoint?.('chapter1');
  }

  // ---------- chapter two ----------
  // After the B-12 report: back at the desk, the supervisor's line rings.
  beginChapter2(saved: SavedCase | null) {
    const { player, ui } = this.d;
    this.setPhase('ch2');
    this.cinematic = false;
    this.lookAssist = 0;
    this.timers = [];
    if (saved?.ch2) { this.notes = [...saved.notes]; this.clock = saved.clock; }
    else {
      this.clock = Math.max(this.clock, this.eventClock + 40 * 60);
      player.place(3.35, 1.45, 2.75); // at the supervisor desk, facing the phone
    }
    ui.fade(false);
    this.ch2.begin(saved?.ch2 ?? null);
    this.onCheckpoint?.('chapter2');
  }

  // ---------- chapter three ----------
  // After the call to the key holder: from the records room out to the service truck.
  beginChapter3(saved: SavedCase | null) {
    const { ui } = this.d;
    this.setPhase('ch3');
    this.cinematic = false;
    this.lookAssist = 0;
    this.timers = [];
    if (saved?.ch3) { this.notes = [...saved.notes]; this.clock = saved.clock; }
    else this.clock = Math.max(this.clock, 3 * 3600 + 40 * 60);
    ui.fade(false);
    this.ch3.begin(saved?.ch3 ?? null);
    this.onCheckpoint?.('chapter3');
  }

  // ---------- chapter four ----------
  // After the field prints: across the road to room 6 at Sierra Motor Court.
  beginChapter4(saved: SavedCase | null) {
    const { ui } = this.d;
    this.ch3.active = false; // its field roll is filed; the truck and the station stay as they are
    this.setPhase('ch4');
    this.cinematic = false;
    this.lookAssist = 0;
    this.timers = [];
    if (saved?.ch4) { this.notes = [...saved.notes]; this.clock = saved.clock; }
    else this.clock = Math.max(this.clock, 4 * 3600 + 35 * 60);
    ui.fade(false);
    this.ch4.begin(saved?.ch4 ?? null);
    this.onCheckpoint?.('chapter4');
  }

  // ---------- per frame ----------
  update(dt: number, t: number, paused: boolean) {
    const { room, audio, player, ext } = this.d;
    if (!paused) {
      this.gt += dt;
      this.clock += dt;
      const due = this.timers.filter((x) => x.at <= this.gt);
      if (due.length) {
        this.timers = this.timers.filter((x) => x.at > this.gt);
        due.sort((a, b) => a.at - b.at).forEach((x) => x.fn());
      }
    }
    room.setClock(this.clock);
    this.d.ui.clock(clockText(this.clock, false));

    // printer paper feeding out
    if (this.phase === 'printing') room.printerPaper.scale.y = Math.min(1, Math.max(0.001, (this.gt - this.printT) / 5.2));

    // phone handset rattle while ringing
    room.handset.rotation.z = this.ringing || this.ch2.ringing ? Math.sin(t * 70) * 0.05 * (Math.sin(t * 2.2) > -0.2 ? 1 : 0) : 0;

    // receiver tone
    if (this.powered && audio.carrier && !this.ns.k3Open) {
      const open = this.rxc.open;
      const rx = this.rx;
      if (this.at('end')) { audio.setCarrier(0, 0, 0); audio.signalOff(); }
      else if (rx.stage >= 3 || (rx.stage === 2 && this.rxc.residualVisible)) {
        // the anomaly (core/signalVoice.ts): static that turns into the pulsing signal as the
        // receiver closes on 1420.405, then the locked signal; louder with the console open
        const c = rx.stage >= 3 ? 1 : rx.quality(PROFILES[2]);
        audio.setCarrier(0, 0, 0);
        audio.signalFrame(Math.max(0.02, c), open ? 0.9 : 0.3, t);
      } else {
        const q = this.rxc.surveyOnly ? 0.05 : rx.quality();
        audio.setCarrier(open ? 0.06 * q * q : 0, (q - 1) * 0.03, open ? 0.03 * (1 - q) + 0.008 : 0.003);
      }
    }
    this.rxc.update(t);

    // rack LEDs flicker a little when live
    if (this.powered && !this.ns.k3Open) {
      this.ledT -= dt;
      if (this.ledT <= 0) {
        this.ledT = 0.18;
        const i = Math.floor(Math.random() * room.ledCount);
        room.setLed(i, Math.random() < 0.3 ? 'off' : room.ledState(i));
      }
    }

    // light flicker after the boom
    if (this.flicker > 0) {
      this.flicker -= dt;
      const on = this.flicker <= 0 || Math.random() > 0.45;
      room.lights.ceiling.forEach((l, i) => { l.intensity = on ? this.base.ceiling[i] * (0.6 + Math.random() * 0.5) : 0.2; });
      room.tubes.forEach((tb, i) => { tb.material = on || i === 2 ? M.emissiveTube : materialFor(M.emissiveTubeOff); });
      if (this.flicker <= 0) {
        room.lights.ceiling.forEach((l, i) => { l.intensity = this.base.ceiling[i] * (i === 1 ? 0.55 : 1); });
        room.tubes.forEach((tb, i) => { tb.material = i === 3 ? materialFor(M.emissiveTubeOff) : M.emissiveTube; });
      }
    }
    if (this.crtGlitch > 0) this.crtGlitch -= dt;

    // mug walks off the desk edge, falls, shatters
    if (this.mugFall) {
      const f = this.mugFall, mug = room.mug;
      f.t += dt;
      if (f.stage === 0) {
        mug.position.z = room.mugHome.z - Math.min(1, f.t / 0.55) * 0.27 + (Math.random() - 0.5) * 0.006;
        mug.position.x = room.mugHome.x + (Math.random() - 0.5) * 0.006;
        mug.rotation.y += (Math.random() - 0.5) * 0.15;
        if (f.t >= 0.55) f.stage = 1;
      } else {
        f.v.y -= 9.8 * dt;
        mug.position.addScaledVector(f.v, dt);
        mug.rotation.x -= f.spin * dt;
        if (mug.position.y <= 0.03) {
          const p = mug.position.clone(); p.y = 0;
          this.mugFall = null;
          this.shatter(p);
        }
      }
    }
    for (const s of this.shards) {
      if (s.rest) continue;
      s.v.y -= 9.8 * dt;
      s.m.position.addScaledVector(s.v, dt);
      s.m.rotation.x += s.w.x * dt; s.m.rotation.y += s.w.y * dt; s.m.rotation.z += s.w.z * dt;
      if (s.m.position.y < 0.008) {
        s.m.position.y = 0.008;
        if (Math.abs(s.v.y) < 0.4) { s.rest = true; continue; }
        s.v.y = -s.v.y * 0.3; s.v.x *= 0.5; s.v.z *= 0.5; s.w.multiplyScalar(0.4);
      }
    }
    if (this.spill && this.spillT < 1) { this.spillT = Math.min(1, this.spillT + dt * 0.7); this.spill.scale.setScalar(0.1 + 0.9 * Math.sqrt(this.spillT)); }

    // the first time out in the yard the jacket comes off its hook (it is the silhouette in
    // the photograph after the credits, HISTORIE.md)
    if (!this.ns.jacket && this.at('ch1') && !paused && player.pos.x > 6.3) {
      this.ns.jacket = true;
      room.objs.jacket.visible = false;
      this.d.ui.toast('Cold out. You take your jacket.', 2.6);
    }

    // dishes arrive
    if (this.phase === 'turning' && !ext.dishes.some((x) => x.moving)) this.arrived();
    if (this.phase === 'ch1' || this.phase === 'ch2' || this.phase === 'ch3' || this.phase === 'ch4') this.ch1.update(paused ? 0 : dt, t, this.phase === 'ch1');
    if (this.phase === 'ch2') this.ch2.update(paused ? 0 : dt, t);
    if (this.phase === 'ch3') this.ch3.update(paused ? 0 : dt, t);
    if (this.phase === 'ch4') this.ch4.update(paused ? 0 : dt, t);
    this.decoder.update(dt);

    // gentle camera assist so the player sees the array turn
    if (this.lookAssist > 0 && (this.phase === 'turning' || this.phase === 'end')) {
      const target = new THREE.Vector3(-6, 9, -70);
      const dx = target.x - player.pos.x, dz = target.z - player.pos.z;
      const yaw = Math.atan2(-dx, -dz);
      const pitch = Math.atan2(target.y - player.eye, Math.hypot(dx, dz));
      let dy = yaw - player.yaw;
      dy = Math.atan2(Math.sin(dy), Math.cos(dy));
      const k = 1 - Math.exp(-dt * 1.4);
      player.yaw += dy * k;
      player.pitch += (pitch - player.pitch) * k;
    }
  }

  // ---------- CRT screens (called ~12 times a second) ----------
  drawCrts(t: number) {
    const { room } = this.d;
    this.drawOps(t);
    if (!this.powered) return;
    const glitch = this.crtGlitch > 0 && Math.random() < 0.6;
    const booting = this.bootT >= 0 && this.gt - this.bootT < 2.6;
    for (const [crt, fn] of [[room.crtLeft, this.drawStatus], [room.crtCenter, this.drawSpectrumCrt], [room.crtRight, this.drawTracking]] as [Crt, (g: CanvasRenderingContext2D, t: number) => void][]) {
      if (crt === room.crtCenter && this.ns.k3Open) continue;
      const g = crt.ctx;
      if (glitch) { g.fillStyle = '#000'; g.fillRect(0, 0, 512, 384); }
      else if (booting) this.drawBoot(g, this.gt - this.bootT);
      else {
        fn.call(this, g, t);
        if (this.at('end') && Math.floor(t * 2) % 2 === 0) this.banner(g, 'SIGNAL ACQUIRED');
      }
      g.drawImage(this.scan, 0, 0);
      crt.commit();
    }
  }

  // The operations terminal on the west desk: its header, the last command and a cursor.
  private drawOps(t: number) {
    const crt = this.d.room.crtOps;
    const blink = Math.floor(t * 2) % 2;
    if (!this.opsDirty && blink === this.opsT) return;
    this.opsDirty = false; this.opsT = blink;
    const g = crt.ctx;
    this.clear(g);
    const head = this.ops.header().split('\n');
    this.txt(g, head[0].replace(/\s+/g, ' '), 14, 30, 22);
    this.txt(g, head[1], 14, 56, 22);
    const lines = this.opsLines.slice(-10);
    lines.forEach((l, i) => this.txt(g, l.slice(0, 40), 14, 92 + i * 26, 22));
    this.txt(g, '$ ' + (blink ? '_' : ''), 14, 92 + lines.length * 26, 22);
    g.drawImage(this.scan, 0, 0);
    crt.commit();
  }

  private txt(g: CanvasRenderingContext2D, s: string, x: number, y: number, size = 24, col = '#8cffa4') {
    g.font = `${size}px VT323, monospace`; g.fillStyle = col; g.fillText(s, x, y);
  }
  private clear(g: CanvasRenderingContext2D) { g.fillStyle = '#021006'; g.fillRect(0, 0, 512, 384); }
  private banner(g: CanvasRenderingContext2D, s: string) {
    g.fillStyle = 'rgba(2,16,6,.88)'; g.fillRect(0, 150, 512, 84);
    g.strokeStyle = '#8cffa4'; g.lineWidth = 2; g.strokeRect(14, 158, 484, 68);
    g.font = '52px VT323, monospace'; g.fillStyle = '#c8ffd2'; g.textAlign = 'center'; g.fillText(s, 256, 208); g.textAlign = 'left';
  }

  private drawBoot(g: CanvasRenderingContext2D, e: number) {
    this.clear(g);
    const lines = ['SARO RX/DSP  REV 2.3', '(C) 1983 SOUTHWEST ASTRONOMICAL', 'MEMORY TEST ...... 512K OK', 'RX BANK 3 ........ ONLINE', 'ARRAY LINK ....... 15/15', 'LOADING SCHEDULE'];
    const n = Math.min(lines.length, Math.floor(e * 3));
    for (let i = 0; i < n; i++) this.txt(g, lines[i], 22, 50 + i * 34);
    if (Math.floor(e * 4) % 2) this.txt(g, '_', 22, 50 + n * 34);
  }

  private drawStatus(g: CanvasRenderingContext2D) {
    this.clear(g);
    const ev = this.at('event');
    this.txt(g, 'SARO  ARRAY STATUS', 18, 34, 28);
    this.txt(g, clockText(this.clock), 384, 34, 26);
    const mode = ev ? 'NO SCHEDULE ENTRY' : this.rx.stage >= 2 ? 'SURVEY  H-LINE' : this.rx.stage === 1 ? 'INTERFERENCE CHECK' : 'CALIBRATION';
    this.txt(g, `RX BANK 3   ONLINE`, 18, 68, 22);
    this.txt(g, `MODE        ${mode}`, 18, 92, 22, ev ? '#ffd27a' : '#8cffa4');
    this.txt(g, `SCHEDULE    AZ 042  EL 48  UNTIL 06:00`, 18, 116, 22);
    g.fillStyle = 'rgba(140,255,164,.35)'; g.fillRect(18, 128, 476, 1);
    const ds = this.d.ext.dishes;
    ds.forEach((d, i) => {
      const col = i < 8 ? 0 : 1, row = i % 8;
      const mv = d.moving ? '>' : ' ';
      this.txt(g, `${d.id} ${pad(d.curAz, 3)}.${Math.floor((d.curAz % 1) * 10)} ${pad(d.curEl)}.${Math.floor((d.curEl % 1) * 10)}${mv}`, 18 + col * 250, 156 + row * 26, 22, d.moving && ev ? '#ffd27a' : '#8cffa4');
    });
    if (ev && Math.floor(this.gt * 2) % 2 === 0) this.txt(g, 'NO CONTROL COMMAND RECORDED', 18, 372, 24, '#ffd27a');
  }

  private drawSpectrumCrt(g: CanvasRenderingContext2D, t: number) {
    drawSpectrum(g, 512, 384, this.rx, t, { residualVisible: this.rxc.residualVisible });
    this.txt(g, 'SPECTRUM  1419.5 - 1420.7 MHz', 14, 28, 24);
    this.txt(g, `${this.rx.frequency.toFixed(3)} MHz`, 360, 28, 24, '#ffd27a');
    const warn = this.rx.status.startsWith('UNLOGGED');
    if (!warn || Math.floor(t * 2.5) % 2 === 0) this.txt(g, this.rx.status.slice(0, 40), 14, 370, 22, warn ? '#ffd27a' : '#b9ffc7');
  }

  private drawTracking(g: CanvasRenderingContext2D, t: number) {
    this.clear(g);
    const cx = 150, cy = 200, r = 128;
    g.strokeStyle = 'rgba(140,255,164,.5)'; g.lineWidth = 1.5;
    for (const k of [1, 0.66, 0.33]) { g.beginPath(); g.arc(cx, cy, r * k, 0, Math.PI * 2); g.stroke(); }
    g.beginPath(); g.moveTo(cx - r, cy); g.lineTo(cx + r, cy); g.moveTo(cx, cy - r); g.lineTo(cx, cy + r); g.stroke();
    const d0 = this.d.ext.dishes[0];
    const a = d0.curAz * Math.PI / 180, rr = r * (1 - d0.curEl / 90);
    g.strokeStyle = '#8cffa4'; g.lineWidth = 2;
    g.beginPath(); g.moveTo(cx, cy); g.lineTo(cx + Math.sin(a) * r, cy - Math.cos(a) * r); g.stroke();
    g.fillStyle = '#c8ffd2'; g.beginPath(); g.arc(cx + Math.sin(a) * rr, cy - Math.cos(a) * rr, 5, 0, Math.PI * 2); g.fill();
    this.txt(g, 'N', cx - 5, cy - r - 8, 20);
    this.txt(g, 'TRACKING', 300, 40, 28);
    this.txt(g, `AZ ${pad(d0.curAz, 3)}.${Math.floor((d0.curAz % 1) * 10)}`, 300, 80, 30);
    this.txt(g, `EL ${pad(d0.curEl)}.${Math.floor((d0.curEl % 1) * 10)}`, 300, 112, 30);
    const x = 300;
    if (this.phase === 'solving') {
      const p = Math.min(1, (this.gt - this.solveT) / 4.2);
      this.txt(g, 'DIRECTION SOLVE', x, 160, 22);
      g.strokeStyle = '#8cffa4'; g.strokeRect(x, 172, 190, 16); g.fillStyle = '#8cffa4'; g.fillRect(x + 2, 174, 186 * p, 12);
      for (let i = 0; i < 5; i++) this.txt(g, `ITER ${pad(Math.floor(p * 40) + i, 3)}  ${(Math.random() * 9).toFixed(3)}`, x, 216 + i * 24, 20, 'rgba(140,255,164,.7)');
    } else if (this.rx.solved) {
      this.txt(g, 'RA  05h 17m 32s', x, 160, 22);
      this.txt(g, 'DEC -05  23\' 14"', x, 184, 22);
      if (this.at('countdown')) {
        const e = this.at('event') ? 47 : Math.min(47, this.gt - this.lineDeadT);
        this.txt(g, 'LINE DROP', x, 232, 22, '#ffd27a');
        this.txt(g, `T+ 00:00:${pad(e)}`, x, 264, 30, '#ffd27a');
      } else this.txt(g, 'DIST  -39 LY  ??', x, 232, 22, '#ffd27a');
    } else if (this.rx.stage >= 3) {
      if (Math.floor(t * 2) % 2 === 0) this.txt(g, 'SOURCE LOCKED', x, 160, 22, '#ffd27a');
      this.txt(g, 'SOLVE: READY', x, 188, 22);
    } else this.txt(g, this.rx.stage >= 2 ? 'SURVEY TRACK' : 'PARKED', x, 160, 22);
    if (this.at('event')) this.txt(g, 'NO COMMAND', x, 330, 26, '#ffd27a');
  }

  // ---------- lifecycle ----------
  reset() {
    const { room, audio, ext, ui } = this.d;
    this.timers = []; this.gt = 0; this.clock = START_CLOCK;
    this.phase = 'intro'; this.cinematic = false;
    this.logRead = false; this.coffee = 'none'; this.powered = false; this.answered = false;
    this.notes = []; this.docs = [];
    this.bootT = this.solveT = this.printT = this.lineDeadT = -1;
    this.ns = { papers: [], k3Open: false, log: [], solveAt: null, jacket: false };
    room.objs.jacket.visible = true;
    this.opsLines = []; this.opsDirty = true;
    if (this.radioOn && audio.ctx) audio.amRadio(null);
    this.radioOn = false;
    this.alarmOn = false; this.skipQueued = false; this.ringPending = false; this.ringing = false;
    this.flicker = 0; this.crtGlitch = 0; this.mugFall = null; this.lookAssist = 0;
    Object.assign(this.rx, new Rx());
    this.rxc.residualVisible = false;
    ui.close(true);
    for (const c of [room.crtLeft, room.crtCenter, room.crtRight]) { c.setPowered(false); c.ctx.fillStyle = '#020a04'; c.ctx.fillRect(0, 0, 512, 384); c.commit(); }
    room.lights.crt.intensity = 0;
    room.lights.ceiling.forEach((l, i) => { l.intensity = this.base.ceiling[i]; });
    room.tubes.forEach((tb) => { tb.material = M.emissiveTube; });
    this.setLeds(false);
    room.lever.rotation.z = -0.6;
    room.printerPaper.visible = false; room.printerPaper.scale.y = 0.001;
    room.handset.visible = true; room.handset.rotation.set(0, 0, 0);
    room.mug.visible = true; room.mug.position.copy(room.mugHome); room.mug.rotation.set(0, 0, 0);
    room.coffee.visible = false;
    for (const s of this.shards) { room.group.remove(s.m); }
    this.shards = [];
    if (this.spill) { room.group.remove(this.spill); this.spill.geometry.dispose(); this.spill = null; }
    for (const dish of ext.dishes) dish.snap(PARK_AZ, SURVEY_EL);
    if (audio.ctx) { audio.stop('ring'); audio.stop('printer'); audio.setCarrier(0, 0, 0); audio.signalOff(); audio.stopMotors(); }
    this.ch1?.reset();
    this.ch2?.reset();
    this.ch3?.reset();
    this.ch4?.reset();
    this.d.doors.closeAll();
    this.decoder?.stop();
  }

  start(checkpoint?: string) {
    this.reset();
    const { ui, player, room } = this.d;
    player.place(room.spawn.x, room.spawn.z, room.spawn.yaw);
    if (checkpoint) { this.jump(checkpoint); return; }
    this.after(1.2, () => ui.toast('23:41. Dale left twenty minutes ago.', 3.4));
    this.after(5.0, () => { if (!this.logRead) ui.toast('His shift log is on the desk.', 3.4); });
  }

  // Jump straight to a beat. Used by Continue and for testing (window.S47.jump).
  jump(target: string) {
    const { player, room, ui } = this.d;
    if (target === 'chapter1') { this.jumpChapter1(); return; }
    if (target === 'chapter2') { this.jumpChapter2(); return; }
    if (target === 'chapter3') { this.jumpChapter3(); return; }
    if (target === 'chapter4') { this.jumpChapter4(); return; }
    const steps = ['shift', 'residual', 'locked', 'printed', 'countdown', 'event'];
    const idx = steps.indexOf(target);
    if (idx < 0) return;
    this.logRead = true; this.docs = [LOG]; this.phase = 'shift';
    if (idx >= 1) {
      this.powerUp(true);
      this.ns.log.push([23 * 3600 + 44 * 60, 'RX BANK 3  K3 CLOSED']);
      this.rx.advance(); this.rx.advance();
      this.note(PROFILES[0].note); this.note(PROFILES[1].note);
      this.clock = SKIP_CLOCK;
      this.pointDishes(this.rx.azimuth, SURVEY_EL, 99);
      player.place(0.05, -2.2, 0);
      this.enterResidual();
      ui.toast('02:13. The survey has been running for hours.', 3.4);
    }
    if (idx >= 2) {
      this.cancel('alarm');
      this.timers = [];
      this.rxc.residualVisible = true;
      this.rx.advance();
      this.pointDishes(this.rx.azimuth, SURVEY_EL, 99);
      this.clock = SKIP_CLOCK + 40;
      this.phase = 'locked';
    }
    if (idx >= 3) {
      this.rx.solved = true;
      room.printerPaper.visible = true; room.printerPaper.scale.y = 1;
      this.phase = 'printed';
      player.place(4.6, -0.4, Math.PI / 2);
    }
    if (idx >= 4) {
      room.printerPaper.visible = false;
      this.docs.push(printout(this.clock));
      this.answered = true;
      player.place(2.4, 1.6, Math.PI);
      this.startCountdown();
      // testing shortcut: the full 47 seconds only run in a real playthrough
      if (idx === 4) { this.timers = this.timers.filter((x) => x.at - this.gt < 40); this.after(4, () => this.event()); }
    }
    if (idx >= 5) { this.timers = []; this.event(); }
  }

  // Everything the prologue leaves behind, then chapter one (fresh or from the saved case).
  private jumpChapter1() {
    const saved = this.d.loadCase();
    this.afterPrologue(saved);
    this.beginChapter1(saved);
  }

  // Chapter two from a checkpoint, or for testing straight after a finished chapter one
  // (without photographs) when there is no saved case.
  private jumpChapter2() {
    const saved = this.d.loadCase();
    this.afterPrologue(saved);
    const done: SavedCase = saved?.s.filed ? saved : {
      s: { v: 1, stage: 'complete', eventClock: this.eventClock, camera: true, door: true, labDoor: true, log: true, returned: true,
        dev1: 3, dev2: 3, ref: true, hyp: 'light', method: 'passive', observed: true, concluded: true, filed: true,
        wrongRef: 0, wrongHyp: 0, wrongConcl: 0, rejected: 0 },
      notes: [...this.notes], clock: this.eventClock + 35 * 60,
    };
    this.beginChapter1(done);
    this.beginChapter2(done.ch2 ? done : null);
  }

  // Chapter three from a checkpoint, or for testing straight after chapter two (without
  // photographs) when there is no saved case.
  private jumpChapter3() {
    const saved = this.d.loadCase();
    this.afterPrologue(saved);
    const ch2 = { v: 1 as const, stage: 'complete' as const, answered: true, door: true, entered: true,
      read: { original: true, amended: true, lineage: true, index: true }, p04: true, p05: true,
      finding: 'omitted-c', destination: 'old-survey-station', surveyId: 'STATION 01', marker: 'triangle-bar',
      wrong04: 0, wrong05: 0, called: true };
    const done: SavedCase = saved?.ch2?.called ? saved : {
      s: { v: 1, stage: 'complete', eventClock: this.eventClock, camera: true, door: true, labDoor: true, log: true, returned: true,
        dev1: 3, dev2: 3, ref: true, hyp: 'light', method: 'passive', observed: true, concluded: true, filed: true,
        wrongRef: 0, wrongHyp: 0, wrongConcl: 0, rejected: 0 },
      ch2, notes: [...this.notes], clock: this.eventClock + 80 * 60,
    };
    this.beginChapter1(done);
    this.beginChapter2(done);
    this.beginChapter3(done.ch3 ? done : null);
  }

  // Chapter four from a checkpoint, or for testing straight after chapter three (without
  // photographs) when there is no saved case.
  private jumpChapter4() {
    const saved = this.d.loadCase();
    this.afterPrologue(saved);
    const ch2 = { v: 1 as const, stage: 'complete' as const, answered: true, door: true, entered: true,
      read: { original: true, amended: true, lineage: true, index: true }, p04: true, p05: true,
      finding: 'omitted-c', destination: 'old-survey-station', surveyId: 'STATION 01', marker: 'triangle-bar',
      wrong04: 0, wrong05: 0, called: true };
    const ch3: Ch3State = { v: 1, stage: 'complete', arrived: true, readE09a: true, readE10: true, heard: true,
      seen: { footing: true, markerA: true, markerB: true, stakes: true, diagram: true, hut: true },
      lampCovered: false, lampObserved: true, cableInspected: true,
      p06: true, p07: true, p08: true, p09: true, wrong06: 0, wrong08: 0, wrong09: 0, rejected: 0, called: true, dev: 3 };
    const done: SavedCase = saved?.ch3?.stage === 'complete' ? saved : {
      s: { v: 1, stage: 'complete', eventClock: this.eventClock, camera: true, door: true, labDoor: true, log: true, returned: true,
        dev1: 3, dev2: 3, ref: true, hyp: 'light', method: 'passive', observed: true, concluded: true, filed: true,
        wrongRef: 0, wrongHyp: 0, wrongConcl: 0, rejected: 0 },
      ch2, ch3, notes: [...this.notes], clock: 4 * 3600 + 32 * 60,
    };
    this.beginChapter1(done);
    this.beginChapter2(done);
    this.beginChapter3(done);
    this.beginChapter4(done.ch4 ? done : null);
  }

  // Everything the prologue leaves behind.
  private afterPrologue(saved: SavedCase | null) {
    const { room, ext } = this.d;
    this.logRead = true; this.answered = true;
    this.powerUp(true);
    this.rx.advance(); this.rx.advance(); this.rx.advance();
    this.rx.solved = true;
    this.rxc.residualVisible = true;
    this.clock = saved?.clock ?? this.eventClock + 70;
    this.docs = [LOG, printout(this.eventClock - 60)];
    this.notes = [];
    for (const n of [PROFILES[0].note, PROFILES[1].note,
      '02:13:47. Unlogged residual on the spectrum near 1420.4. Survey schedule says nothing should be there.',
      '02:14. Pulse group: four, a gap, seven. Repeats every 5.2 seconds. Not the relay. Not anything in the schedule.',
      'Direction solve: RA 05h 17m, Dec -05. Somewhere in Orion.',
      'Source distance on the solve: -39 light years. A negative distance. The solver cannot produce that.',
      'Outside line. No voice. Room tone, a printer, a heavy thud, a gasp, something ceramic breaking. Then nothing.',
      'Every dish moved to az 026, el 32. Console shows no control command.',
      'All dishes left their scheduled track and aligned together.']) this.note(n);
    room.mug.visible = false;
    room.printerPaper.visible = false;
    for (const dish of ext.dishes) dish.snap(EVENT_AZ, EVENT_EL);
    // Night Shift's papers, relay K3 and the console log: as saved, or as the night left them
    // (a save from before 4 October has none of it; then K3 was reset after the impact)
    const e = saved?.s?.eventClock ?? this.eventClock;
    this.ns = saved?.ns
      ? { papers: [...saved.ns.papers], k3Open: saved.ns.k3Open, log: saved.ns.log.map((l) => [l[0], l[1]] as LogLine), solveAt: saved.ns.solveAt, jacket: !!saved.ns.jacket }
      : { papers: [], k3Open: false, solveAt: e - 60, log: [[23 * 3600 + 44 * 60, 'RX BANK 3  K3 CLOSED'], [e - 60, 'SOLVE  RANGE NEGATIVE'],
        [e - 47, 'STATION CLOCK  WWVB REFERENCE LOST'], [e - 7, 'RX BANK 3  K3 OPEN'], [e, 'LINE POWER  DIP 1.2 S'],
        [e, 'STATION CLOCK  WWVB REFERENCE RESTORED  HOLDOVER 00:00:47'], [e + 3, 'ARRAY  S-01 TO S-27 MOTION  NO COMMAND'], [e + 70, 'RX BANK 3  K3 CLOSED']] };
    for (const id of this.ns.papers) { const d = this.paperById(id); if (d) this.docs.push(d); }
    this.applyK3(this.ns.k3Open);
    room.objs.jacket.visible = !this.ns.jacket;
    this.opsDirty = true;
    this.phase = 'end';
  }
}
