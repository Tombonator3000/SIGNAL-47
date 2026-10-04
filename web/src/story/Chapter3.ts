import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { FieldCamera, Photo } from '../core/FieldCamera';
import type { Chapter1 } from './Chapter1';
import * as P from '../ui/Panels';
import * as D from './drawings';
import { clockText } from './time';

// Chapter three, "The Survey Station" (K3 in the design bible). Ported from the Unity
// field pass (Station26): P06, which fixed point moved, with FRAME 03; P07, the local
// lamp null test; P08, the deliberate cable cut, with FRAME 04; P09, the 1947 timing log
// against tonight's receiver. P06 and P07 go in either order, P08 needs both and its own
// photograph, P09 needs P08. Field texts and replies are the Unity ones.
// New in the web version: the drive out in the SARO service truck, Nora's call on the
// hut's field telephone after P09, and the drive back to develop both frames at the
// SARO wet bench (Unity did the same with a travel folio).

export type Stage3 = 'to-truck' | 'station' | 'call' | 'return' | 'develop' | 'complete';
export type AreaId = 'saro' | 'road' | 'station01';

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
const HINTS: Record<P.FieldPage, string[]> = {
  marker: ['Which fixed point is no longer where E09A puts it?', 'E09A puts A in the concrete footing on the transit sight line. Is the footing still occupied?', 'The footing is empty, and the post with the triangle and bar stands off the line. Record A.'],
  lamp: ['A null test changes one local condition and looks again.', 'The field lamp has a tin hood. Cover it, then observe.', 'Stand by the lamp, cover it, and choose OBSERVE NULL TEST.'],
  cable: ['Look at the ends of the cable, not at the reference.', 'Old cable frays and corrodes. These faces are clean and opposite each other.', 'Inspect the cut faces, expose FRAME 04 up close, and record the deliberate cut.'],
  timing: ['Read the 1947 log in the hut and listen to the receiver.', 'The carrier ceased at 02:17:00. What was observed 47 seconds later?', 'The correction leads up to the relay impact at 02:17:47.'],
};

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

  constructor(private g: Chapter3Host, private d: Chapter3Deps) {}

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
      case 'to-truck': return area === 'road' ? 'STATION 01 // SOUTH ON THE HIGHWAY, THEN THE SURVEY TRACK' : 'SERVICE YARD // TAKE THE SARO TRUCK TO STATION 01';
      case 'station':
        if (!s.p06 && !s.p07) return 'STATION 01 // DOCUMENT THE MOVED MARKER OR RUN A LOCAL LAMP NULL TEST';
        if (!s.p06) return 'STATION 01 // EXPOSE FRAME 03 AND RECORD WHICH FIXED MARKER MOVED';
        if (!s.p07) return 'STATION 01 // COVER THE LOCAL LAMP AND OBSERVE A NORMAL NULL TEST';
        if (!s.p08) return 'STATION 01 // INSPECT THE CABLE BREAK AND EXPOSE FRAME 04';
        return 'STATION 01 // MATCH THE 1947 AND 1986 TIMING RECORDS';
      case 'call': return this.ringing ? 'STATION 01 // THE FIELD TELEPHONE IS RINGING' : 'STATION 01 // FIELD RECORD COMPLETE';
      case 'return': return area === 'road' ? 'SARO // BACK UP THE HIGHWAY' : 'STATION 01 // DRIVE BACK TO SARO WITH THE FIELD ROLL';
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
    if (s.p09) t.push({ text: 'Answer the field telephone', done: s.called });
    if (s.called) t.push(
      { text: 'Drive back to SARO', done: s.stage === 'develop' || s.stage === 'complete' },
      { text: 'Develop FRAMES 03 and 04 in the photo lab', done: s.dev >= 3 },
    );
    return t;
  }

  // ---------- the truck at SARO ----------
  bindSaroTruck(object: THREE.Object3D) {
    const { inter, ui } = this.d;
    inter.add({ id: 'truck', object, range: 3.2,
      label: () => this.active && this.s.stage === 'to-truck' ? 'Drive to STATION 01' : 'Service truck',
      use: () => {
        if (!this.active) { ui.toast('SARO 07, the service truck. The keys are in it, like always.', 3.4); return; }
        if (this.s.stage === 'to-truck') { this.d.travel.driveOut(); return; }
        ui.toast(this.s.stage === 'develop' ? 'The field roll goes to the wet bench first.' : 'You have been out once tonight. The truck can stay where it is.', 3.4);
      } });
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
      add('s1transit', 'transit', () => this.s.readE09a ? 'Field record / transit' : 'Read the transit record (E09A)', () => this.useTransit(), 2.8);
      add('s1footing', 'footing', () => 'Concrete footing', () => this.look('footing'));
      add('s1markerA', 'markerA', () => 'Marker post', () => this.look('markerA'), 3.2);
      add('s1markerB', 'markerB', () => 'Comparison marker', () => this.look('markerB'), 3.2);
      add('s1stakes', 'stakesC', () => 'Survey stakes', () => this.look('stakes'), 3.5);
      add('s1diagram', 'diagram', () => 'Framed diagram', () => this.look('diagram'));
      add('s1lamp', 'lamp', () => this.s.lampCovered ? 'Field lamp (covered)' : 'Field lamp', () => this.openField('lamp'));
      add('s1cable', 'cable', () => this.s.cableInspected ? 'Cable break' : 'Inspect the cable', () => this.useCable(), 2.4);
      add('s1hutDoor', 'hutDoor', () => 'Field hut', () => this.look('hut'));
      add('s1receiver', 'receiver', () => this.s.heard ? 'Receiver' : 'Switch on the receiver', () => this.useReceiver());
      add('s1timing', 'timingLog', () => this.s.readE10 ? 'Field record / timing' : 'Read the timing log (E10)', () => this.useTimingLog());
      add('s1phone', 'fieldPhone', () => this.ringing ? 'Answer the field telephone' : 'Field telephone', () => this.usePhone());
      add('s1gate', 'gate', () => 'Vehicle gate', () => ui.toast('Chained and padlocked. N. Vega holds the key. The walk gate beside it is open.', 3.6));
      add('s1truck', 'truckSpot', () => this.s.stage === 'return' ? 'Drive back to SARO' : 'Service truck', () => this.useStationTruck(), 3.2);
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

  private useTransit() {
    const s = this.s;
    if (!s.readE09a) {
      s.readE09a = true;
      this.file(this.docTransit());
      this.g.note('E09A, the transit record: A sat in the concrete footing on the transit sight line. B is the fixed comparison marker to the west. C is the closing sight line.');
      this.d.ui.toast('E09A filed in the case.', 2.4);
      this.save();
    }
    this.openField('marker');
  }

  private useCable() {
    if (!this.s.cableInspected) this.inspect();
    this.openField('cable');
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
    this.g.after(4.0, () => audio.radioVoice(4.6));
    cap(4.0, 'Under the carrier, a man\'s voice, far away: "Reference west. No. East. Hold the last reading."', 5.2);
    this.g.after(9.4, () => {
      this.g.cinematic = false;
      this.s.heard = true;
      this.g.note('The receiver in the hut, on the carrier: a man\'s voice, far away. "Reference west. No. East. Hold the last reading." Then only the carrier.');
      ui.toast('Then only the carrier.', 2.6);
      this.panel?.refresh();
      this.save();
    });
  }

  private useTimingLog() {
    const s = this.s;
    if (!s.readE10) {
      s.readE10 = true;
      this.file(this.docTiming());
      this.g.note('E10, the 1947 timing log: 02:17:00 carrier ceased, a recorded voice. 02:17:47 relay impact and reference motion. Final observation not entered.');
      this.d.ui.toast('E10 filed in the case.', 2.4);
      this.save();
    }
    this.openField('timing');
  }

  private useStationTruck() {
    const s = this.s;
    if (s.stage === 'return') { this.d.travel.driveBack(); return; }
    this.d.ui.toast(!s.p08 ? 'Not yet. N. Vega asked you to look at the cable first.' : s.stage === 'call' ? 'The field telephone is ringing in the hut.' : 'Not yet. The field record is not finished.', 3.4);
  }

  // ---------- the field record (P06 to P09) ----------
  private openField(page: P.FieldPage) {
    const s = this.s;
    this.panel = P.fieldRecord(this.d.ui, {
      page,
      status: () => this.s.p09 ? 'P06 TO P09 RECORDED / Field timing linked / Return to SARO' : this.objective(),
      transit: { have: s.readE09a, text: TRANSIT, image: this.transitImage(), where: 'The transit record is behind glass in a steel frame on the west face of the transit pier.' },
      timing: () => ({ have: this.s.readE10, text: this.s.heard ? `${TIMING_1947}\n\n${TIMING_1986}` : TIMING_1947, where: 'The 1947 ledger is on the workbench in the hut.' }),
      state: () => ({ covered: this.s.lampCovered, observed: this.s.lampObserved, inspected: this.s.cableInspected, frame04: !!this.s.f4, p06: this.s.p06, p07: this.s.p07, p08: this.s.p08, p09: this.s.p09 }),
      supported: SUPPORTED,
      onMarker: (c) => this.submitMarker(c),
      onLamp: (cover) => this.setLamp(cover),
      onObserve: () => this.observeLamp(),
      onInspect: () => this.inspect(),
      onCable: (c) => this.submitCable(c),
      onTiming: (c) => this.submitTiming(c),
      hints: HINTS,
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
    audio.callLine(18.4);
    const cap = (t: number, text: string, secs: number) => this.g.after(t, () => ui.toast(text, secs));
    cap(0.3, 'N. VEGA: "So you went out there."', 2.6);
    cap(3.0, 'N. VEGA: "This line still rings in the motel office. It has since 1947. Nobody ever took it down."', 4.6);
    cap(7.8, 'N. VEGA: "You have seen the cable now. It did not break by itself."', 3.8);
    cap(11.8, 'N. VEGA: "Room 6. Bring the original record and your film, developed. Then I will tell you the rest."', 5.0);
    this.g.after(17.0, () => {
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

  private check03(): { text: string; ok: boolean } {
    const { fcam } = this.d;
    const site = this.d.travel.site()!;
    if (!this.s.readE09a) return { text: 'READ THE TRANSIT RECORD E09A FIRST', ok: false };
    const t = this.target('marker');
    const d = this.dist(t);
    if (d > 16) return { text: 'MOVE CLOSER TO THE TRANSIT SIGHT LINE', ok: false };
    if (d < 2.2) return { text: 'STEP BACK // THE FOOTING AND THE POST IN ONE FRAME', ok: false };
    const f = site.proxies.footing ? this.pos(site.proxies.footing) : t;
    const m = site.proxies.markerA ? this.pos(site.proxies.markerA) : t;
    if (!fcam.inFrame(t, 0.18, 0.82, 0.15, 0.85) || !fcam.inFrame(f, 0.04, 0.96, 0.06, 0.94) || !fcam.inFrame(m, 0.04, 0.96, 0.06, 0.94))
      return { text: 'FRAME THE EMPTY FOOTING AND THE MARKED POST TOGETHER', ok: false };
    if (site.occluders && !fcam.clearLine(t, site.occluders)) return { text: 'VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT', ok: false };
    return { text: 'FOOTING AND MARKED POST IN FRAME // READY', ok: true };
  }
  private check04(): { text: string; ok: boolean } {
    const { fcam } = this.d;
    if (!this.s.cableInspected) return { text: 'INSPECT THE CABLE CUT FIRST', ok: false };
    const t = this.target('cable');
    if (this.dist(t) > 3.2) return { text: 'MOVE CLOSER TO THE CABLE BREAK', ok: false };
    if (!fcam.inFrame(t, 0.22, 0.78, 0.18, 0.82)) return { text: 'FRAME THE CUT CABLE ENDS', ok: false };
    return { text: 'CUT FACES IN FRAME // READY', ok: true };
  }

  // Why the shutter would (not) fire right now. Chapter one asks this while it runs.
  check(): { text: string; ok: boolean } {
    const s = this.s;
    if (this.d.travel.area() !== 'station01' || !this.d.travel.site()) {
      return { text: s.f3 && s.f4 ? 'FIELD ROLL EXPOSED // DEVELOP IT AT THE WET BENCH' : 'THE FIELD ROLL IS FOR STATION 01', ok: false };
    }
    if (s.f3 && s.f4) return { text: 'FRAMES 03 AND 04 EXPOSED // DEVELOP AT SARO', ok: false };
    const c4 = s.f4 ? null : this.check04();
    if (c4?.ok) return c4;
    const c3 = s.f3 ? null : this.check03();
    if (c3?.ok) return c3;
    if (c4 && (!c3 || this.dist(this.target('cable')) < 6)) return c4;
    return c3!;
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
        : 'The tank is empty.\nThe field roll for STATION 01 is still in the camera.' });
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

  endingLines() {
    return [
      'THE SURVEY STATION',
      'STATION 01 // P06 TO P09 RECORDED',
      'Fixed point A was moved off its footing. The field lamp is only a local source. The cable was cut on purpose, with clean, opposed faces.',
      'The 1947 log and tonight\'s receiver carry the same correction in the same voice: T. Vega. The relay impact came 47 seconds after the carrier ceased.',
      'N. Vega is waiting in room 6 at Sierra Motor Court with the rest. NEXT: ROOM 6. This part of the night is still being built.',
    ];
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
    if (this.d.travel.area() === 'station01' && !this.s.arrived) this.arrivedStation();
    this.d.ui.objective(this.objective());
  }
}
