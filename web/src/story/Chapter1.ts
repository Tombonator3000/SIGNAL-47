import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { ControlRoom } from '../world/ControlRoom';
import type { Exterior } from '../world/Exterior';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { Collider } from '../world/ControlRoom';
import { FieldCamera, type Photo } from '../core/FieldCamera';
import { ServiceYard, YARD } from '../world/ServiceYard';
import { eachVariant } from '../core/quality';
import * as P from '../ui/Panels';
import { clockText } from './time';

// Chapter one, "The Second Exposure". Ported from the Unity chapter (Chapter09 and
// Visual10 passes): S-03 motor log, field camera, photo lab, B-12 control test with a
// passive or active method, two exposures and a bounded local report.
// Web adaptations: the array lies north of the yard, the photo lab sits east of the
// walk, the three-bar marker is called R-07 (S-07 is an antenna here), and the motor
// bus header reads SARO ARRAY.

export type Stage = 'collect-camera' | 'motor-log' | 'first-exposure' | 'develop-first' | 'interpret-first'
  | 'choose-control' | 'observe-control' | 'second-exposure' | 'develop-second' | 'compare-exposures' | 'file-report' | 'complete';
type Method = 'passive' | 'active';
type Hyp = 'light' | 'drift';

export interface CaseState {
  v: 1;
  stage: Stage;
  eventClock: number;
  camera: boolean; door: boolean; labDoor: boolean; log: boolean; returned: boolean;
  f1?: Photo; f2?: Photo;
  dev1: number; dev2: number;          // 0 sealed, 2 print in fixer, 3 collected
  mark1?: [number, number]; ref: boolean; hyp: Hyp | null;
  method: Method | null; observed: boolean;
  mark2?: [number, number]; concluded: boolean; filed: boolean;
  wrongRef: number; wrongHyp: number; wrongConcl: number; rejected: number;
}

export interface ChapterHost {
  clock: number;
  gt: number;
  notes: string[];
  docs: DocSpec[];
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
  cancel(tag: string): void;
}

export interface Chapter1Deps {
  ui: UI; audio: AudioSys; room: ControlRoom; ext: Exterior; player: Player; inter: Interaction;
  yard: ServiceYard; fcam: FieldCamera; colliders: Collider[];
  view: { restore: () => void; draw: () => void };
  save: (s: CaseState | null) => void;
  isTouch: () => boolean;
}

const fresh = (eventClock: number): CaseState => ({
  v: 1, stage: 'collect-camera', eventClock, camera: false, door: false, labDoor: false, log: false, returned: false,
  dev1: 0, dev2: 0, ref: false, hyp: null, method: null, observed: false, concluded: false, filed: false,
  wrongRef: 0, wrongHyp: 0, wrongConcl: 0, rejected: 0,
});

const MARK_R1 = 0.095, MARK_R2 = 0.115; // how close a click must be to a reference, in print UV
const VANE_TIME = 1.8, DOOR_TIME = 0.75, TANK_TIME = 2.2, FIX_TIME = 1.5;

export class Chapter1 {
  active = false;
  s: CaseState = fresh(0);
  onEnd?: (method: Method) => void;
  private images: Record<string, THREE.Texture> = {};
  private anim: { vane?: { from: number; to: number; t0: number }; door?: number; labDoor?: number; fix?: number } = {};
  private busy: string | null = null;     // a timed step at the wet bench
  private settled = false;
  private status = { text: '', ok: false };
  private busT = 0;
  private doorCol: Collider = { minX: 6.06, maxX: 7.0, minZ: 1.08, maxZ: 1.18 };
  private labDoorCol: Collider = { minX: 12.8, maxX: 13.75, minZ: -0.78, maxZ: -0.68 };

  constructor(private g: ChapterHost, private d: Chapter1Deps) {
    this.interactables();
  }

  // ---------- lifecycle ----------
  begin(saved: CaseState | null, eventClock: number) {
    this.reset();
    this.active = true;
    this.s = saved ? { ...fresh(eventClock), ...saved } : fresh(eventClock);
    this.applyWorld();
    // the motor bus cabinet hums; so does the safelight ballast in the lab
    this.d.audio.hum('s03', new THREE.Vector3(12.3, 1.0, -11.0), 0.07, 60);
    this.d.audio.hum('safelight', new THREE.Vector3(15.0, 2.4, -5.6), 0.025, 120);
    if (!saved) {
      this.g.note('The array left its track. Check the local motor controller in the east service yard.');
      this.d.ui.toast('Service access released. East door.', 3.6);
      this.save();
    }
  }

  reset() {
    const { yard, room, fcam, ui } = this.d;
    this.active = false;
    this.d.audio.stopHums();
    this.s = fresh(0);
    this.busy = null; this.settled = false; this.anim = {};
    fcam.raise(false); fcam.have = false;
    ui.viewfinder(false); ui.cameraButton(false); ui.objective(null);
    this.showCamera(true);
    room.setDoor(0); room.setDoorLock(false);
    yard.labDoorHinge.rotation.y = 0;
    yard.setVane(0); yard.setWorkLamp(1);
    yard.wetPrint.visible = false;
    yard.dryPrints.forEach((p) => { p.visible = false; });
    yard.zone.eastDoor.enabled = false; yard.zone.labDoor.enabled = false;
    this.setCollider(this.doorCol, false); this.setCollider(this.labDoorCol, false);
  }

  // Puts the world in the state described by this.s (after begin or a restore).
  private applyWorld() {
    const { room, yard, fcam, ui } = this.d;
    const s = this.s;
    room.setDoorLock(true);
    this.showCamera(!s.camera);
    fcam.have = s.camera;
    ui.cameraButton(s.camera && this.touch);
    if (s.door) { room.setDoor(1); yard.zone.eastDoor.enabled = true; this.setCollider(this.doorCol, true); }
    if (s.labDoor) { yard.labDoorHinge.rotation.y = -Math.PI / 2; yard.zone.labDoor.enabled = true; this.setCollider(this.labDoorCol, true); }
    if (s.method) {
      yard.setVane(s.method === 'passive' ? -60 : 45);
      yard.setWorkLamp(s.method === 'passive' ? 0 : 1.35);
      this.settled = true;
    }
    if (s.f1 && s.dev1 >= 3) this.hangPrint(0, s.f1);
    if (s.f2 && s.dev2 >= 3) this.hangPrint(1, s.f2);
    // documents filed so far
    if (s.log) this.file(this.docS03());
    if (s.f1 && s.dev1 >= 3) this.file(this.docPhoto(s.f1));
    if (s.ref) this.file(this.docSheet());
    if (s.observed) this.file(this.docObservation());
    if (s.f2 && s.dev2 >= 3) this.file(this.docPhoto(s.f2));
    if (s.concluded) this.file(this.docFinding());
  }

  serialize() { return this.s; }
  // Hidden objects still catch raycasts in three.js, so the collected camera leaves layer 0.
  private showCamera(on: boolean) {
    const cam = this.d.room.fieldCamera;
    cam.visible = on;
    cam.traverse((o) => o.layers.set(on ? 0 : 31));
  }
  private save() { this.d.save(this.s); }
  private get touch() { return this.d.isTouch(); }

  private setCollider(c: Collider, on: boolean) {
    const list = this.d.colliders;
    const i = list.indexOf(c);
    if (on && i < 0) list.push(c);
    if (!on && i >= 0) list.splice(i, 1);
  }

  private file(doc: DocSpec) {
    const i = this.g.docs.findIndex((x) => x.id === doc.id);
    if (i >= 0) { this.g.docs[i] = doc; return false; }
    this.g.docs.push(doc);
    return true;
  }

  private stageTo(st: Stage) { this.s.stage = st; this.save(); }

  // ---------- objective line and notebook ----------
  objective(): string {
    const s = this.s;
    const lab = 'PHOTO LAB';
    switch (s.stage) {
      case 'collect-camera': return 'FIELD KIT // COLLECT THE CAMERA BESIDE THE EAST DOOR';
      case 'motor-log': return 'S-03 // READ THE MOTOR CABINET IN THE EAST SERVICE YARD';
      case 'first-exposure': return this.touch ? 'S-03 APRON // CAMERA, THEN SHUTTER: EXPOSE THE CENTRAL ANTENNA' : 'S-03 APRON // C: CAMERA / SPACE: EXPOSE THE CENTRAL ANTENNA';
      case 'develop-first':
      case 'develop-second': {
        const dev = s.stage === 'develop-first' ? s.dev1 : s.dev2;
        if (this.busy === 'tank') return `${lab} // THE FILM IS IN THE TANK; FINISH THE PROCESS AT THE BENCH`;
        if (this.busy === 'fix') return `${lab} // THE PRINT IS IN THE FIXER`;
        if (dev === 0) return s.stage === 'develop-first' ? `SEALED FILM // ${lab} EAST OF THE WALK / PROCESS AT THE WET BENCH` : `CONTROL FILM // ${lab} / PROCESS AT THE WET BENCH`;
        if (dev === 1) return `${lab} // TRANSFER THE CONTACT PRINT TO THE FIXER`;
        return `${lab} // COLLECT THE PROCESSED PRINT`;
      }
      case 'interpret-first':
        if (!s.mark1) return 'FRAME 01 // INSPECT THE PRINT AND MARK A COMPARABLE DETAIL';
        if (!s.ref) return `${lab} ARCHIVE // MATCH THE MARKED DETAIL TO THE FACILITY REFERENCE SHEET`;
        return 'REFERENCE SHEET // CHOOSE A TESTABLE EXPLANATION';
      case 'choose-control': return 'B-12 // FOLLOW THE EAST WALK NORTH TO THE LOCAL REFERENCE CONTROL';
      case 'observe-control': return this.settled ? 'B-12 // NOTE THE DIRECT OBSERVATION AFTER THE VANE SETTLES' : 'B-12 // WAIT FOR THE REFERENCE VANE TO SETTLE';
      case 'second-exposure': return this.touch ? 'B-12 // FROM THE SIGHT LINE MARK: CAMERA, SHUTTER' : 'B-12 // C: CAMERA / SPACE: RECORD THE CONTROLLED REFERENCE';
      case 'compare-exposures': return 'TWO PRINTS // COMPARE THE SAME REFERENCE AND RECORD WHAT THE TEST SUPPORTS';
      case 'file-report': return `${lab} // FILE THE TWO-EXPOSURE REPORT AT THE RECORDS DESK`;
      case 'complete': return 'LOCAL CASE FILED // THE SECOND EXPOSURE';
    }
  }

  tasks() {
    const s = this.s;
    const all: { text: string; done: boolean }[] = [
      { text: 'Collect the field camera by the east door', done: s.camera },
      { text: 'Read the S-03 motor bus in the service yard', done: s.log },
      { text: 'Expose frame 01 from the S-03 apron', done: !!s.f1 },
      { text: 'Develop frame 01 in the photo lab', done: s.dev1 >= 3 },
      { text: 'Mark a detail you can check at the site', done: !!s.mark1 },
      { text: 'Match it on the reference sheet', done: s.ref },
      { text: 'Choose an explanation you can test', done: !!s.hyp },
      { text: 'Run a control at B-12', done: !!s.method },
      { text: 'Note what you see at B-12', done: s.observed },
      { text: 'Expose frame 02 from the sight line', done: !!s.f2 },
      { text: 'Develop frame 02', done: s.dev2 >= 3 },
      { text: 'Compare the two exposures', done: s.concluded },
      { text: 'File the local report', done: s.filed },
    ];
    const next = all.findIndex((t) => !t.done);
    return next < 0 ? all : all.slice(0, next + 1);
  }

  // ---------- interactables ----------
  private interactables() {
    const { room, yard, inter, ui, audio } = this.d;
    const toast = (t: string, secs?: number) => ui.toast(t, secs);
    const pos = (o: THREE.Object3D) => o.getWorldPosition(new THREE.Vector3());

    inter.add({ id: 'fieldCamera', object: room.objs.fieldCamera, range: 2.6,
      label: () => !this.active || this.s.camera ? null : 'Collect field camera',
      use: () => {
        this.s.camera = true;
        this.showCamera(false);
        this.d.fcam.have = true;
        ui.cameraButton(this.touch);
        audio.play('click', { gain: 0.6 });
        this.g.note('Collected the field camera. Expose the antenna profile from S-03, then process the film in the photo lab.');
        toast(this.touch ? 'Field camera. The Camera button raises it.' : 'Field camera. C to raise, Space to expose.', 3.6);
        if (this.s.stage === 'collect-camera') this.stageTo(this.s.log ? 'first-exposure' : 'motor-log'); else this.save();
      } });

    inter.add({ id: 'motorBus', object: yard.objs.motorBus, range: 2.4,
      label: () => !this.active ? null : this.s.log ? 'Read motor bus S-03' : 'Check motor bus S-03',
      use: () => {
        audio.play('click', { gain: 0.5, at: pos(yard.objs.motorBus) });
        const doc = this.docS03();
        ui.document(doc, () => {
          if (this.s.log) return;
          this.s.log = true;
          this.file(doc);
          this.g.note('Filed: S-03 / No command recorded.');
          this.g.note('S-03 registered a physical heading change without a motor command.');
          if (this.s.stage === 'motor-log') this.stageTo('first-exposure');
          else if (this.s.stage === 'collect-camera') { this.save(); toast('The field camera is still on the shelf by the east door.'); }
          else this.save();
        });
      } });

    inter.add({ id: 'labDoor', object: yard.objs.labDoor, range: 2.4,
      label: () => !this.active ? null : this.s.labDoor ? null : 'Open lab door',
      use: () => {
        this.s.labDoor = true;
        this.anim.labDoor = this.g.gt;
        yard.zone.labDoor.enabled = true;
        this.setCollider(this.labDoorCol, true);
        audio.play('switch', { gain: 0.5, at: pos(yard.objs.labDoor) });
        audio.play('thudSoft', { gain: 0.35, when: 0.6, at: pos(yard.objs.labDoor) });
        this.save();
      } });

    inter.add({ id: 'wetBench', object: yard.objs.wetBench, range: 2.6,
      label: () => {
        if (!this.active) return null;
        const f = this.devFrame();
        if (!f) return 'Wet bench';
        if (this.busy === 'tank') return 'Check processing tank';
        if (this.busy === 'fix') return null;
        const dev = f.which === 1 ? this.s.dev1 : this.s.dev2;
        return dev === 0 ? 'Process sealed film' : dev === 1 ? 'Transfer print to fixer' : 'Collect processed print';
      },
      use: () => this.useWetBench() });

    inter.add({ id: 'archive', object: yard.objs.archive, range: 2.4,
      label: () => {
        if (!this.active) return null;
        const s = this.s;
        if (s.stage === 'interpret-first') return !s.mark1 ? 'Examine contact print' : 'Reference file';
        if (s.stage === 'compare-exposures') return 'Compare contact prints';
        if (s.f1 && s.dev1 >= 3) return 'Reference file';
        return 'Archive bench';
      },
      use: () => this.useArchive() });

    inter.add({ id: 'b12Control', object: yard.objs.b12Control, range: 2.4,
      label: () => {
        if (!this.active) return null;
        const s = this.s;
        if (s.stage === 'observe-control') return this.settled ? 'Note the direct observation' : null;
        if (s.method) return 'B-12 / review control settings';
        return 'B-12 / local reference control';
      },
      use: () => this.useB12() });

    inter.add({ id: 'vane', object: yard.objs.vane, range: 3.2,
      label: () => this.active ? 'B-12 reference vane' : null,
      use: () => toast(this.s.method === 'passive' ? 'The vane is folded into its shield stop. One stripe. The lamp is dark.'
        : this.s.method === 'active' ? 'The vane stands at 042 degrees. One ivory stripe. One fixed bracket.'
          : 'One ivory stripe on a movable vane. The fixed bracket carries the station number.', 4) });

    inter.add({ id: 'records', object: yard.objs.records, range: 2.4,
      label: () => !this.active ? null : this.s.stage === 'file-report' ? 'File local investigation' : this.s.filed ? 'Read filed chapter report' : 'Night report',
      use: () => {
        const s = this.s;
        if (s.stage === 'file-report' || s.filed) P.localReport(ui, { text: this.reportText(), filed: s.filed, onFile: () => this.fileReport() });
        else toast('SARO night report: observation, method, conclusion. Nothing to file yet.');
      } });

    inter.add({ id: 'r07', object: yard.objs.r07, range: 3.5, label: () => this.active ? 'R-07 marker' : null,
      use: () => toast('R-07. Three horizontal bars on the west fence. Not on the sight line from the apron.', 4) });
  }

  // East door: called by the prologue's door interactable once the chapter is active.
  doorLabel() { return this.s.door ? null : 'Open service door'; }
  useDoor() {
    if (this.s.door) return;
    const { room, yard, audio } = this.d;
    this.s.door = true;
    this.anim.door = this.g.gt;
    yard.zone.eastDoor.enabled = true;
    this.setCollider(this.doorCol, true);
    const at = room.objs.doorEast.getWorldPosition(new THREE.Vector3());
    audio.play('switch', { gain: 0.6, at });
    audio.play('thudSoft', { gain: 0.4, when: 0.65, at });
    this.save();
  }

  // ---------- wet bench ----------
  private devFrame(): { which: 1 | 2; photo: Photo } | null {
    const s = this.s;
    if (s.stage === 'develop-first' && s.f1) return { which: 1, photo: s.f1 };
    if (s.stage === 'develop-second' && s.f2) return { which: 2, photo: s.f2 };
    return null;
  }

  private useWetBench() {
    const { ui, yard, audio } = this.d;
    const f = this.devFrame();
    if (!f) {
      P.wetBench(ui, { frame: '', step: 0, empty: 'The tank is empty.\nExpose a frame from the S-03 apron before loading film. The archive bench holds the local reference plan.', onAct: () => {} });
      return;
    }
    if (this.busy === 'tank') { ui.toast('The tank is sealed. Give it a moment.'); return; }
    if (this.busy) return;
    const dev = f.which === 1 ? this.s.dev1 : this.s.dev2;
    const label = f.which === 1 ? 'FRAME 01 / S-03 ARRAY PROFILE' : 'FRAME 02 / B-12 CONTROL';
    const step = (dev === 0 ? 0 : dev === 1 ? 2 : 3) as 0 | 2 | 3;
    P.wetBench(ui, { frame: label, step, onAct: () => {
      const at = yard.wetPrint.getWorldPosition(new THREE.Vector3());
      if (dev === 0) {
        this.busy = 'tank';
        audio.pour(at);
        ui.toast('Sealed tank loaded. The exposed film is safe from room light.', 3);
        this.g.after(TANK_TIME, () => {
          this.busy = null;
          this.setDev(f.which, 1);
          audio.beep(1500, 0.12, 0.05);
          ui.toast('Contact print ready. Transfer it to the fixer.', 3);
        }, 'lab');
      } else if (dev === 1) {
        this.busy = 'fix';
        audio.pour(at);
        this.showWetPrint(f.photo);
        this.anim.fix = this.g.gt;
        ui.toast('Print in the fixer. The image is becoming stable.', 3);
        this.g.after(FIX_TIME, () => {
          this.busy = null;
          this.setDev(f.which, 2);
          audio.beep(1500, 0.12, 0.05);
          ui.toast('Print fixed. Collect it at the bench.', 3);
        }, 'lab');
      } else this.collect(f.which, f.photo);
    } });
  }

  private setDev(which: 1 | 2, v: number) { if (which === 1) this.s.dev1 = v; else this.s.dev2 = v; this.save(); }

  private collect(which: 1 | 2, photo: Photo) {
    const { yard, audio, ui } = this.d;
    yard.wetPrint.visible = false;
    this.hangPrint(which - 1, photo);
    audio.play('thudSoft', { gain: 0.3, rate: 1.5 });
    if (which === 1) {
      this.s.dev1 = 3;
      this.file(this.docPhoto(photo));
      this.g.note('Filed: PHOTO 01 / S-03 contact print.');
      this.g.note('Processed frame 01. A contact print is ready for close inspection; the facility sheet is at the archive bench.');
      this.stageTo('interpret-first');
      this.openPrint1();
    } else {
      this.s.dev2 = 3;
      this.file(this.docPhoto(photo));
      this.g.note('Filed: PHOTO 02 / B-12 control print.');
      this.g.note('Processed the control print. Both original exposures are available for comparison.');
      this.stageTo('compare-exposures');
      ui.toast('Both prints are ready. Compare them at the archive bench.', 3.5);
      this.openCompare();
    }
  }

  private texOf(photo: Photo) {
    let t = this.images[photo.id];
    if (!t) {
      const img = new Image();
      t = new THREE.Texture(img);
      t.colorSpace = THREE.SRGBColorSpace;
      img.onload = () => { t.needsUpdate = true; };
      img.src = photo.url;
      this.images[photo.id] = t;
    }
    return t;
  }
  private setMap(mesh: THREE.Mesh, tex: THREE.Texture, color: number) {
    eachVariant(mesh.material as THREE.Material, (m) => {
      const mm = m as THREE.MeshStandardMaterial;
      if (mm.map !== tex) { mm.map = tex; mm.needsUpdate = true; }
      mm.color.setScalar(color);
    });
  }
  private showWetPrint(photo: Photo) {
    const p = this.d.yard.wetPrint;
    this.setMap(p, this.texOf(photo), 0);
    p.visible = true;
  }
  private hangPrint(i: number, photo: Photo) {
    const p = this.d.yard.dryPrints[i];
    this.setMap(p, this.texOf(photo), 1);
    p.visible = true;
  }

  // ---------- prints and the archive bench ----------
  private openPrint1() {
    const s = this.s;
    if (!s.f1) return;
    P.printView(this.d.ui, {
      title: 'CONTACT PRINT 01', url: s.f1.url, mark: s.mark1,
      intro: s.mark1 ? 'DETAIL MARKED // Now identify this manufactured reference using the facility sheet at the archive bench.' : 'CLICK A DETAIL IN THE PRINT. Find something that can also be checked directly at the facility. Zoom and move the crop if necessary.',
      hints: [
        'Look for a manufactured reference feature, not the shape of the antenna itself.',
        'The nearer equipment below the central dish carries a stripe. Its mounting appears in the facility reference sheet.',
        'The B-12 assembly is in the lower foreground, in front of the dish. Mark its cream stripe and compare the number of stripes with the sheet.',
      ],
      onMark: (u, v) => {
        if (!this.near(s.f1!, u, v, MARK_R1)) return { ok: false, text: 'No comparable reference feature at that point. Try a different manufactured detail, or request a hint.' };
        if (!s.mark1) { s.mark1 = [u, v]; this.save(); }
        return { ok: true, text: 'DETAIL MARKED // Now identify this manufactured reference using the facility sheet at the archive bench.' };
      },
    });
  }

  private near(photo: Photo, u: number, v: number, r: number) {
    return Object.values(photo.targets).some(([tu, tv]) => Math.hypot(tu - u, tv - v) <= r);
  }

  private useArchive() {
    const s = this.s;
    if (s.stage === 'interpret-first' && !s.mark1) { this.openPrint1(); return; }
    if (s.stage === 'compare-exposures') { this.openCompare(); return; }
    if (s.f1 && s.dev1 >= 3) { this.openReference(); return; }
    this.d.ui.toast('Archive bench: the facility reference sheet, a loupe and an empty print tray.', 3.5);
  }

  private openReference() {
    const s = this.s, { ui } = this.d;
    if (this.file(this.docSheet())) this.g.note('Filed: B-12 / Reference installation sheet.');
    P.referenceFile(ui, {
      sheet: [
        'INSTALLATION SHEET 11-86 / LOCAL SURVEY REFERENCES',
        'B-12: one ivory stripe on a movable reference vane. The fixed bracket carries the station number. A local lamp and an isolated reference drive allow two different checks.',
        'R-07: three horizontal bars, on the west fence. It does not lie between the S-03 service apron and the central antenna.',
        'SIGHTLINE FROM SERVICE APRON: S-03 APRON > B-12 REFERENCE > CENTRAL ANTENNA',
        'A contact print can preserve an optical error as well as a physical object. Make a direct observation and change one known condition before trusting a cause.',
      ],
      refDone: s.ref, hyp: s.hyp,
      onRef: (c) => {
        if (!s.mark1) return { ok: false, text: 'Mark the detail on CONTACT PRINT 01 first.' };
        if (c === 'R-07') {
          s.wrongRef++;
          this.g.note('Rejected R-07 as the photographed reference: its three-bar pattern does not match the marked foreground detail.');
          this.save();
          return { ok: false, text: 'R-07 has THREE HORIZONTAL BARS. Compare that silhouette with the vertical feature you marked. The sheet does not place R-07 on this sightline.' };
        }
        if (!s.ref) {
          s.ref = true;
          this.g.note('Matched the marked foreground reference to B-12 using the installation sheet and the S-03 sightline.');
          this.save();
        }
        return { ok: true, text: 'B-12 selected. Its installation sheet specifies ONE ivory stripe on a movable vane. Choose a cause you can test.' };
      },
      onHyp: (c) => {
        if (s.hyp) return { ok: true, text: 'A working hypothesis is on file. The field test is at B-12, north of the S-03 apron.' };
        if (c === 'command') {
          s.wrongHyp++;
          this.g.note('Hypothesis rejected: a second motor command contradicts the S-03 zero-command record.');
          this.save();
          return { ok: false, text: 'The S-03 log records ZERO commands during the first movement. A second command does not explain this record. Test light or the local reference instead.' };
        }
        s.hyp = c;
        this.g.note(c === 'light' ? 'Working hypothesis: an extra stripe caused by reflected work light.' : 'Working hypothesis: a mismatch caused by the local reference or encoder.');
        this.stageTo('choose-control');
        ui.toast('Field test available. B-12, north of the S-03 apron.', 4);
        return { ok: true, text: c === 'light'
          ? 'Working hypothesis: reflected work light. At B-12, shielding the lamp changes the light while leaving the motor isolated.'
          : 'Working hypothesis: reference/encoder drift. At B-12, a commanded move provides a known reference state.' };
      },
    });
  }

  private openCompare() {
    const s = this.s;
    if (!s.f1 || !s.f2) return;
    P.twoExposures(this.d.ui, {
      url1: s.f1.url, url2: s.f2.url, mark2: s.mark2, concluded: s.concluded,
      hints: [
        'Mark the same assembly in the right-hand print. What moved when the control changed? What did not?',
        'Look at the ivory reference stripe and the second pale mark beside it. The second mark keeps the original alignment while the physical vane turns.',
      ],
      onMark: (u, v) => {
        if (!this.near(s.f2!, u, v, MARK_R2)) return { ok: false, text: 'No comparable reference feature at that point. Mark the same assembly you marked in FRAME 01.' };
        if (!s.mark2) { s.mark2 = [u, v]; this.save(); }
        return { ok: true, text: 'Reference marked in FRAME 02. Now record what the two prints support.' };
      },
      onConclude: (c) => {
        if (s.concluded) return { ok: true, text: 'Finding recorded. File both prints at the records desk.' };
        if (!s.mark2) return { ok: false, text: 'Mark the reference detail in FRAME 02 before recording a conclusion.' };
        if (c === 'lamp') {
          s.wrongConcl++; this.save();
          return { ok: false, text: s.method === 'passive'
            ? 'The lamp was shielded for frame 02. Its extra mark cannot be explained by that lamp remaining on.'
            : 'The vane moved to a known angle while the second mark kept its old alignment. A lamp reflection alone does not account for the fixed second reference.' };
        }
        if (c === 'commands') {
          s.wrongConcl++; this.save();
          return { ok: false, text: 'The first S-03 movement had no command. The control used a different local reference. These records do not support two antenna commands.' };
        }
        s.concluded = true;
        this.file(this.docFinding());
        this.g.note('Filed: Two exposures / Local finding.');
        this.g.note(`The second exposure reproduces the extra reference stripe under a controlled condition. The local test rules out ${s.method === 'passive' ? 'the work lamp' : 'simple reference/encoder drift'} as its source.`);
        this.d.audio.signature(0.45);
        this.stageTo('file-report');
        return { ok: true, text: 'FINDING RECORDED // The physical reference changed. The film retains a second reference. File both prints at the records desk.' };
      },
    });
  }

  // ---------- B-12 ----------
  private useB12() {
    const s = this.s, { ui, audio, yard } = this.d;
    if (s.stage === 'observe-control') {
      if (!this.settled) return;
      s.observed = true;
      this.file(this.docObservation());
      this.g.note('Filed: B-12 / Direct control observation.');
      this.g.note(s.method === 'passive'
        ? 'Direct control observation: one physical ivory stripe. The work lamp is shielded and motor remains isolated.'
        : 'Direct control observation: one physical ivory stripe. The local reference responds to the 042-degree command.');
      ui.document(this.docObservation(), () => {});
      this.stageTo('second-exposure');
      return;
    }
    if (s.method) { P.b12Control(ui, { method: s.method, onChoose: () => {}, review: this.observationText() }); return; }
    if (s.stage !== 'choose-control') {
      ui.toast('The local control is available, but a useful test needs a question. Examine the print and the reference sheet in the photo lab first.', 4.5);
      return;
    }
    P.b12Control(ui, { method: null, onChoose: (m) => {
      s.method = m;
      const target = m === 'passive' ? -60 : 45;
      this.anim.vane = { from: 0, to: target, t0: this.g.gt };
      this.settled = false;
      audio.servo(VANE_TIME, yard.vane.getWorldPosition(new THREE.Vector3()));
      if (m === 'passive') { yard.setWorkLamp(0); ui.toast('Lamp shield engaged. Motor isolated.', 3); }
      else { yard.setWorkLamp(1.35); ui.toast('Reference drive commanding 042 degrees.', 3); }
      this.stageTo('observe-control');
      this.g.after(VANE_TIME, () => {
        this.settled = true;
        ui.toast('B-12 settled. Note the direct observation at the control.', 3.5);
      }, 'vane');
    } });
  }

  // ---------- camera ----------
  toggleCamera() {
    const { fcam, ui } = this.d;
    if (!this.active || !fcam.have) return;
    if (ui.modal) return;
    fcam.raise(!fcam.raised);
    this.d.audio.play('click', { gain: 0.35, rate: fcam.raised ? 1.2 : 0.9 });
    ui.cameraButton(this.touch, fcam.raised);
    if (!fcam.raised) ui.viewfinder(false);
  }

  private frameNo() { return this.s.f2 ? 'FRAME 03' : this.s.f1 ? 'FRAME 02' : 'FRAME 01'; }

  // Why the shutter would (not) fire right now. Mirrors the Unity rules.
  private check(): { text: string; ok: boolean } {
    const s = this.s, { fcam, yard, ext } = this.d;
    const p = this.d.player.pos;
    if (!s.log) return { text: 'READ THE S-03 MOTOR LOG FIRST', ok: false };
    if (!s.f1) {
      const a = YARD.apron;
      if (p.x < a.minX || p.x > a.maxX || p.z < a.minZ || p.z > a.maxZ) return { text: 'MOVE TO THE S-03 SERVICE APRON', ok: false };
      const dish = ext.dishes.find((d) => d.id === 'S-03')!;
      const c = ext.dishArray.feedPosition(dish).lerp(new THREE.Vector3(dish.x, 12.4 * dish.scale, dish.z), 0.5);
      if (!fcam.inFrame(c, 0.36, 0.64, 0.30, 0.65)) return { text: 'ALIGN THE CENTRAL ANTENNA IN THE FRAME', ok: false };
      if (!fcam.clearLine(c, yard.occluders)) return { text: 'VIEW OBSTRUCTED // FIND A CLEAR LINE OF SIGHT', ok: false };
      return { text: 'REFERENCE IN FRAME // READY', ok: true };
    }
    if (s.f2) return { text: 'CONTROL FRAME EXPOSED // RETURN TO THE PHOTO LAB', ok: false };
    if (s.dev1 < 3) return { text: 'UNPROCESSED FILM // PHOTO LAB', ok: false };
    if (!s.hyp) return { text: 'REVIEW FRAME 01 AND THE B-12 REFERENCE SHEET', ok: false };
    if (!s.method || !this.settled || !s.observed) return { text: s.method && !this.settled ? 'WAIT FOR THE REFERENCE VANE TO SETTLE' : 'NOTE THE B-12 OBSERVATION FIRST', ok: false };
    if (Math.hypot(p.x - YARD.sightMark.x, p.z - YARD.sightMark.z) > 3.5) return { text: 'MOVE TO THE B-12 SIGHT LINE MARK', ok: false };
    const v = yard.vane.getWorldPosition(new THREE.Vector3());
    if (!fcam.inFrame(v, 0.22, 0.78, 0.23, 0.78)) return { text: 'FRAME THE B-12 REFERENCE VANE AND ITS FIXED BRACKET', ok: false };
    if (!fcam.clearLine(v, yard.occluders)) return { text: 'REFERENCE OBSTRUCTED // FIND A CLEAR LINE OF SIGHT', ok: false };
    return { text: 'REFERENCE IN FRAME // READY', ok: true };
  }

  shutter() {
    const { fcam, ui, audio, yard } = this.d;
    if (!fcam.raised) return;
    const c = this.check();
    if (!c.ok) {
      this.s.rejected++;
      audio.play('click', { gain: 0.3, rate: 0.7 });
      ui.toast(`${c.text}. No film used.`, 2.6);
      return;
    }
    const first = !this.s.f1;
    const id = first ? 'frame01' : 'frame02';
    const clock = this.g.clock;
    const stripe = () => yard.vaneStripe.getWorldPosition(new THREE.Vector3());
    const echoP = () => yard.echo.getWorldPosition(new THREE.Vector3());
    fcam.sync();
    const targets: Record<string, [number, number]> = {};
    for (const [k, p] of [['vane', stripe()], ['echo', echoP()]] as const) { const r = fcam.project(p); if (r.front) targets[k] = [r.u, r.v]; }
    const canvas = fcam.expose({
      before: () => { yard.echo.visible = true; this.d.ext.dishArray.cull(fcam.photoCam); },
      after: () => { yard.echo.visible = false; },
      restore: () => this.d.view.restore(),
      caption: first ? 'S-03 // ARRAY PROFILE' : `B-12 // ${this.s.method === 'passive' ? 'SHIELDED CONTROL' : 'CONTROL 042'}`,
      time: clockText(clock),
      seed: Math.floor(clock * 1000),
    });
    this.d.view.draw();
    const photo: Photo = { id, subject: first ? 'S-03 / array profile' : 'B-12 / controlled reference', method: first ? 'baseline' : this.s.method!, clock, url: canvas.toDataURL('image/jpeg', 0.86), targets };
    audio.shutter();
    ui.shutterFlash();
    if (first) {
      this.s.f1 = photo;
      this.g.note('Frame 01 exposed. Sealed film awaits processing in the photo lab.');
      ui.toast('Frame 01 exposed. Process the film in the photo lab.', 3.5);
      this.stageTo('develop-first');
    } else {
      this.s.f2 = photo;
      this.g.note('Frame 02 exposed. Sealed film awaits processing in the photo lab.');
      this.g.note(`The control exposure preserves the local test condition: ${this.s.method === 'passive' ? 'lamp shielded, motor isolated' : 'reference driven to 042 degrees'}.`);
      ui.toast('Control frame exposed. Return to the photo lab.', 3.5);
      this.stageTo('develop-second');
    }
    this.g.after(0.6, () => { if (fcam.raised) this.toggleCamera(); });
  }

  // ---------- report and ending ----------
  private reportText() {
    const s = this.s;
    const passive = s.method === 'passive';
    return [
      'SARO / LOCAL INCIDENT S-03 + B-12',
      'S-03: encoder 026; scheduled 042; commands 0.\nFrame 01: foreground reference does not match the one-stripe installation sheet.',
      `CONTROL: ${passive ? 'lamp shielded, motor isolated.' : 'local reference commanded to 042; encoder and visible vane agree.'}\nDirect observation: one physical stripe.\nFrame 02: the physical vane changes; a second stripe retains its earlier alignment.`,
      `SUPPORTED: the discrepancy persists in the photographic record under the chosen control. ${passive ? 'The work lamp is not sufficient to explain it.' : 'Simple reference/encoder drift is not sufficient to explain it.'}`,
      'LIMIT: one local test does not establish the cause of the array movement or the signal. Preserve the negatives and the original receiver printout.',
    ].join('\n\n');
  }

  private fileReport() {
    const s = this.s;
    if (s.filed) return;
    s.filed = true;
    this.g.note('LOCAL CASE CLOSED: S-03 and B-12 records filed together. Preserve negatives. Follow-up: why does the original return report a distance of -39 LY?');
    this.d.audio.play('thudSoft', { gain: 0.4 });
    this.stageTo('complete');
    this.g.after(0.8, () => this.onEnd?.(s.method ?? 'passive'));
  }

  endingLines(method: Method) {
    return [
      'THE SECOND EXPOSURE',
      'LOCAL INCIDENT S-03 / B-12 // FILED',
      'The array moved without a command. You changed a known condition at B-12 and preserved the result. The eye saw one reference. The film kept two.',
      `Your ${method === 'passive' ? 'shielded-lamp test rules out the work lamp' : '042-degree reference test rules out simple encoder drift'} as a sufficient explanation. Both negatives, the motor log and your conclusion now belong to the same case.`,
      'One line on the original receiver print remains unexplained: DISTANCE:\u00a0-39\u00a0LY. What does the receiver mean by "behind"?',
    ];
  }

  // ---------- documents ----------
  private docS03(): DocSpec {
    return {
      id: 's03log', kind: 'typed', title: 'S-03 / No command recorded', stamp: 'MOTOR BUS',
      page: `SARO ARRAY / MOTOR BUS S-03\nLOCAL CONTROLLER LOG // ${clockText(this.s.eventClock, false)}\n\nENCODER HEADING:   026 DEGREES\nSCHEDULED HEADING: 042 DEGREES\n\nCOMMANDS RECEIVED: 0\nLOCAL OVERRIDE:    NONE`,
      transcript: 'The antenna moved. The controller did not send a command.\n\nA thin line of condensation runs across the warm inspection glass.',
    };
  }
  private docSheet(): DocSpec {
    return {
      id: 'refsheet', kind: 'typed', title: 'B-12 / Reference installation sheet', stamp: '11-86',
      page: 'SARO / INSTALLATION SHEET 11-86\nLOCAL SURVEY REFERENCES\n\nB-12  ONE IVORY STRIPE\n      MOVABLE VANE\n      FIXED BRACKET: STATION NO.\nR-07  THREE HORIZONTAL BARS\n      WEST FENCE\n\nSIGHTLINE FROM SERVICE APRON:\nS-03 APRON > B-12 > CENTRAL ANTENNA',
      transcript: 'Local survey references: B-12 is the movable single ivory stripe north of the S-03 apron. R-07 uses three horizontal bars. The warm inspection lamp and local reference drive have independent controls.',
    };
  }
  private docPhoto(p: Photo): DocSpec {
    const first = p.id === 'frame01';
    return {
      id: p.id, kind: 'photo', image: p.url, page: '',
      title: first ? 'PHOTO 01 / S-03 contact print' : 'PHOTO 02 / B-12 control print',
      transcript: first
        ? `Exposed from the S-03 apron at ${clockText(p.clock)}. The central antenna, and below it the B-12 reference vane in the lower foreground.\n\nThe installation sheet says B-12 carries one stripe.`
        : `Exposed from the B-12 sight line at ${clockText(p.clock)}, under the ${p.method === 'passive' ? 'shielded, passive' : 'active, 042-degree'} control.`,
    };
  }
  private observationText() {
    return this.s.method === 'passive'
      ? 'B-12 / PASSIVE CONTROL\n\nLamp: shielded, no direct illumination.\nMotor: isolated, no drive command.\nDirect observation: one ivory stripe on the folded reference vane; one fixed bracket.\n\nMake a control exposure from the apron without changing this setup.'
      : 'B-12 / ACTIVE CONTROL\n\nLocal command: reference 042 degrees.\nEncoder: 042 degrees, reference moved and settled.\nDirect observation: one ivory stripe on the turned vane; one fixed bracket.\nWork lamp remains lit.\n\nMake a control exposure without changing this setup.';
  }
  private docObservation(): DocSpec {
    return { id: 'observation', kind: 'typed', title: 'B-12 / Direct control observation', stamp: this.s.method === 'passive' ? 'PASSIVE' : 'ACTIVE', page: this.observationText(), transcript: 'One physical stripe on the vane. Expose frame 02 from the B-12 sight line mark without changing the setup.' };
  }
  private docFinding(): DocSpec {
    return { id: 'finding', kind: 'typed', title: 'Two exposures / Local finding', stamp: 'LOCAL', page: this.reportText(), transcript: 'The eye saw one reference. The film kept two.' };
  }

  // ---------- per frame ----------
  update(dt: number, t: number) {
    if (!this.active) return;
    const { room, yard, fcam, ui, player } = this.d;
    const gt = this.g.gt;
    if (this.anim.door !== undefined) {
      const k = Math.min(1, (gt - this.anim.door) / DOOR_TIME);
      room.setDoor(1 - Math.pow(1 - k, 3));
      if (k >= 1) this.anim.door = undefined;
    }
    if (this.anim.labDoor !== undefined) {
      const k = Math.min(1, (gt - this.anim.labDoor) / DOOR_TIME);
      yard.labDoorHinge.rotation.y = -(1 - Math.pow(1 - k, 3)) * Math.PI / 2;
      if (k >= 1) this.anim.labDoor = undefined;
    }
    if (this.anim.vane) {
      const a = this.anim.vane;
      const k = Math.min(1, (gt - a.t0) / VANE_TIME);
      const e = k < 0.5 ? 2 * k * k : 1 - Math.pow(-2 * k + 2, 2) / 2;
      yard.setVane(a.from + (a.to - a.from) * e);
      if (k >= 1) this.anim.vane = undefined;
    }
    if (this.anim.fix !== undefined) {
      const k = Math.min(1, (gt - this.anim.fix) / FIX_TIME);
      eachVariant(yard.wetPrint.material as THREE.Material, (m) => (m as THREE.MeshStandardMaterial).color.setScalar(k * k));
      if (k >= 1) this.anim.fix = undefined;
    }
    this.busT -= dt;
    if (this.busT <= 0) { this.busT = 0.25; yard.drawBus(26, 0, true, t); }

    // walking back into the control room with the log
    if (!this.s.returned && this.s.log && Math.abs(player.pos.x) < 5.9 && Math.abs(player.pos.z) < 4.4) {
      this.s.returned = true;
      this.g.note('Returned to the control room with the S-03 motor log. The unauthorized alignment is documented.');
      ui.toast('S-03 investigation filed.', 2.6);
      this.save();
    }

    if (fcam.raised) {
      fcam.sync();
      if (!ui.modal) {
        this.status = this.check();
        ui.viewfinder(true, { rect: fcam.frameRect(), frame: this.frameNo(), status: this.status.text, ok: this.status.ok, touch: this.touch });
      }
    }
    ui.objective(this.objective());
  }
}
