import * as THREE from 'three';
import type { UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { World } from '../world/World';
import type { DriveInput } from '../drive/Drive';
import { Ending } from '../ui/Ending';
import { flashFrames, type Frame } from './flashFrames';
import { artLoaded, artImage, loadArtFor, FLASH_ART } from '../core/art';
import { clockText } from './time';
import { SUN_MESA, horizonDip } from '../world/backdrop';
import { ThirdFigure } from './theThird';

// Chapter six, "Roswell Road" and THE EVENT (KAPITLER.md). From the diner's lot across the
// highway onto the old road at 05:20, east-north-east and then north-east into the dawn, past
// the mile posts to where C crosses, just past the eight: 13 km, driven the whole way. On the line the night takes
// the truck for exactly 47 seconds: the radio goes to carrier, the engine dies, the dash
// blinks four and seven, the headlights wake in the wrong colours, the radio plays the night
// back in pieces, FLASH, silence. At 05:29:47 the camera leaves the cab and does not come
// back: out through the windshield, up over the desert, the sun on the mesa at 05:30, the
// stars. Then the title, the credits and the whole photograph from 1947.
//
// Nothing here is saved. The last save is the diner with the truck ready (chapter five),
// so the ending can be seen again.

export type Stage6 = 'drive' | 'event' | 'pullout' | 'end';

export interface Chapter6Host {
  clock: number;
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
}
type OldWorld = Pick<World, 'driveOldRoad' | 'oldRoad' | 'oldDrive' | 'oldTruck' | 'engineSound' | 'oldControl' | 'stopDriving' | 'area'>;
export interface Chapter6Deps {
  ui: UI; audio: AudioSys;
  world: OldWorld;
  camera: THREE.PerspectiveCamera;
  settings: () => { calmFlash: boolean; stillShots: boolean };
  /** The haze over the old road (null: the area's own). */
  fog: (density: number | null) => void;
  /** The 3D part is over, the screen is black: stop the game, roll the credits. */
  finale: () => void;
}

const T = 5 * 3600 + 29 * 60;            // 05:29:00
const LEAVE = 5 * 3600 + 20 * 60;        // the truck leaves the diner's lot at 05:20
// At 55 mph the truck is on C at 05:29:00. The clock goes as it goes, but it is never ahead of
// that pace (driven slower, or stopped, it waits) and never more than a minute behind it.
const PACE = 24.6;
const EVENT = 47, PULL = 34;
/** The morning programme on the AM station, a line at a time as the miles go by (KAPITLER.md). */
const RADIO: [number, string][] = [
  [0.15, 'The radio, low: "...cattle at the sale barn steady, a dollar higher on calves..."'],
  [1.4, 'The radio: "...clear today, high near seventy-four in Roswell, light winds out of the southwest..."'],
  [2.9, 'The radio: "...road crews on the highway north of town all week, so give yourself a few extra minutes..."'],
  [4.3, 'The radio: "...and Halley\'s comet is still low in the south after midnight this week, if you can get away from the town lights."'],
  [5.5, 'The radio: "...if you\'re out on the county roads this early, watch the low spots, there\'s deer..."'],
];
const sm = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
const dir = (deg: number) => new THREE.Vector3(Math.sin(deg * Math.PI / 180), 0, -Math.cos(deg * Math.PI / 180));
const UP = new THREE.Vector3(0, 1, 0);
const HEADS = [new THREE.Color(0.12, 0.25, 1.0), new THREE.Color(1.0, 0.55, 0.12), new THREE.Color(0.2, 1.0, 0.35), new THREE.Color(1.0, 0.08, 0.05)];
/** The third one (story/theThird.ts). Where the night takes the truck, it stands on the road ahead,
 *  where the truck will come to stand `ahead` m short of it (its middle; where the pull-out's first
 *  shot stands, keyframes().front): a dark shape in the grey, the band catching the dead headlights
 *  a little as the truck rolls up to it. When they wake (+10 s) it is someone in a SARO jacket, back
 *  to the cab, and the band across the back takes each colour as it comes (`flares`: the blue up,
 *  amber, green, red), except with slow fades or still shots, where it only follows them. It never
 *  moves. Gone in the FLASH (+32 s): with the first frame, or with slow fades or still shots let go
 *  over `letGo` s of the first fade instead. The truck brakes to stand where it should: `brake` is
 *  the braking the stop is planned on (m/s/s), after `react` s, and never more than `far` m on. */
export const THIRD = { until: 32, ahead: 9, letGo: 1.2, flares: [11.0, 12.2, 14.4, 16.6], brake: 3.2, react: 0.8, near: 3, far: 120 } as const;
const HEAD_WHITE = new THREE.Color(1.0, 0.9, 0.72), THIRD_AMB = new THREE.Color(0.035, 0.038, 0.048);
const _band = new THREE.Color(), _glint = new THREE.Color(), _eye = new THREE.Vector3();
const _c = new THREE.Color(), _v = new THREE.Vector3(), _w = new THREE.Vector3(), _q = new THREE.Quaternion(), _m = new THREE.Matrix4();

export class Chapter6 {
  active = false;
  started = false;
  stage: Stage6 = 'drive';
  /** Read by main.ts every frame: how much of the dawn the sky shows (null: the area decides),
   *  and the sun pinned to an elevation in degrees (null: it follows the clock). */
  skyLift: number | null = null;
  sunElev: number | null = null;
  ending: Ending | null = null;

  private et = 0;          // seconds into THE EVENT
  private pt = 0;          // seconds into the pull-out
  private done = new Set<string>();
  /** The pressure of the time away under THE EVENT (bought sound, AudioSys.pressure). */
  private bed: ReturnType<AudioSys['pressure']> = null;
  private frames: Frame[] | null = null;
  private still = false;
  private calm = false;
  private idle = 0;
  private backT = 0;
  private from = { pos: new THREE.Vector3(), q: new THREE.Quaternion() };
  private truckAt = { pos: new THREE.Vector3(), heading: 0 };
  /** The third one in the headlights (story/theThird.ts), while THE EVENT has it. */
  third: ThirdFigure | null = null;
  private steady = false;
  private bandPts: THREE.Vector3[] = [];
  private headNow = new THREE.Color();
  private _head = new THREE.Vector3();
  /** Where the truck's middle comes to stand (along the road, m), planned when the night takes it. */
  private stopS: number | null = null;
  /** Where its head is while it stands there (the night's log, main.ts; the tests), else null. */
  get thirdAt(): THREE.Vector3 | null { return this.third?.mesh.visible ? this.third.head(this._head) : null; }

  constructor(private g: Chapter6Host, private d: Chapter6Deps) {}

  // ---------- lifecycle ----------
  /** From the diner's lot onto the old road, driven (the case was saved in the diner first). */
  begin() {
    this.reset();
    this.active = true; this.started = true;
    this.stage = 'drive';
    // the photograph and Codex's frames of FLASH; the frames are drawn again once they are in
    void loadArtFor(['photo1947', ...FLASH_ART]).then(() => { if (this.active && this.stage === 'drive') this.frames = flashFrames(); }).catch(() => {});
    this.d.audio.amRadio(null);   // the diner's radio stays at the diner
    this.d.world.driveOldRoad(() => {
      this.g.clock = LEAVE;
      this.d.audio.cabRadio('talk');
      this.g.note(`${this.time()}. Across the highway from the diner the old road goes off east-north-east, into the grey coming up in the east. C crosses it just past the eight-mile post.`);
    });
    // the frames of FLASH are drawn now, so the moment itself does not wait for them
    this.g.after(3, () => { if (this.active && !this.frames) this.frames = flashFrames(); });
  }

  reset() {
    this.active = false; this.started = false; this.stage = 'drive';
    this.et = 0; this.pt = 0; this.done.clear(); this.idle = 0; this.backT = 0; this.stopS = null;
    this.skyLift = null; this.sunElev = null;
    this.ending?.dispose(); this.ending = null;
    if (this.d.world.oldControl) this.d.world.oldControl = null;
    this.d.fog(null);
    this.d.audio.cabRadio(null);
    this.d.audio.silence(false);
    this.bed?.stop(); this.bed = null;
    this.dropThird();
  }
  private dropThird() {
    this.third?.dispose(); this.third = null;
    this.d.world.oldRoad?.thirdGlints(null);
  }

  saveBlock(): string | null { return 'Not on the road.'; }
  private time() { return clockText(this.g.clock, false); }

  objective() {
    if (this.stage === 'drive') return 'THE OLD ROSWELL ROAD // C CROSSES JUST PAST MILE 8';
    return null;
  }
  tasks() {
    return [{ text: 'Drive out to where C crosses the old road, just past mile 8', done: this.stage !== 'drive' }];
  }

  // ---------- the drive ----------
  private drive(dt: number) {
    const w = this.d.world, road = w.oldRoad, drv = w.oldDrive;
    if (!road || !drv || w.area !== 'roswell') return;
    // the clock keeps to the pace of the drive (PACE), and never shows 05:29 before the line:
    // it slows to a crawl in the last 20 seconds
    const at = road.where(drv.pos.x, drv.pos.z), pace = T - Math.max(0, -at.s) / PACE;
    this.g.clock = THREE.MathUtils.clamp(this.g.clock, pace - 60, pace);
    const left = T - this.g.clock;
    if (left < 20) this.g.clock -= dt * (1 - Math.max(0.02, left / 20));
    this.g.clock = Math.min(this.g.clock, T - 0.05);
    // fronts of the truck over C: the night begins
    const h = drv.heading, fx = drv.pos.x - Math.sin(h) * 2.3, fz = drv.pos.z - Math.cos(h) * 2.3;
    if (road.pastC(fx, fz) >= 0) { this.startEvent(); return; }
    if (road.hitBack) { road.hitBack = false; if (this.backT <= 0) { this.d.ui.toast('Ward can wait. The bolt is ahead.', 3.4); this.backT = 6; } }
    this.backT -= dt;
    this.idle = Math.abs(drv.speed) < 0.5 ? this.idle + dt : 0;
    // (a truck put further on by the developer menu skips what it has passed)
    for (const [mi, text] of RADIO) if (at.mi >= mi && !this.done.has(text)) { this.done.add(text); if (at.mi < mi + 0.4) this.d.ui.toast(text, 5); }
    if (this.idle > 40 && at.mi > 7.6 && !this.done.has('nudge')) { this.done.add('nudge'); this.d.ui.toast('The line is just ahead, past the eight-mile post.', 4); }
    if (this.idle > 40 && at.mi < 0.2 && !this.done.has('across')) { this.done.add('across'); this.d.ui.toast('The old road is across the highway, going off to the east.', 4); }
  }

  // ---------- THE EVENT ----------
  private startEvent() {
    const w = this.d.world;
    this.stage = 'event';
    this.et = 0;
    this.g.clock = T;
    this.calm = this.d.settings().calmFlash;
    this.steady = this.calm || this.d.settings().stillShots;
    if (!this.frames) this.frames = flashFrames();
    this.ending = new Ending();
    // where it rolls out and stands: as far as it would, braking gently, from the speed it had
    const drv = w.oldDrive, road = w.oldRoad;
    if (drv && road) {
      const v = Math.max(0, drv.speed);
      this.stopS = road.where(drv.pos.x, drv.pos.z).s + THREE.MathUtils.clamp(v * v / (2 * THIRD.brake) + v * THIRD.react, THIRD.near, THIRD.far);
    }
    this.headNow.copy(HEAD_WHITE);
    // the truck coasts to a stop in its lane; the player keeps only their eyes
    w.oldControl = { input: (dt) => this.coast(dt) };
    this.d.ui.objective(null);
    this.d.ui.clearToasts();
  }

  /** Steer along the right lane and brake gently: the truck rolls out and stands, where it was
   *  planned to (stopS: the third one is standing 9 m on from there). */
  private coast(_dt: number): DriveInput {
    const w = this.d.world, drv = w.oldDrive!, road = w.oldRoad!;
    const s = road.where(drv.pos.x, drv.pos.z).s;
    const tg = road.lane(s + 14, _w);
    let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading;
    err = Math.atan2(Math.sin(err), Math.cos(err));
    let throttle = 0;
    if (this.et > THIRD.react && drv.speed > 0.05) {
      const rem = (this.stopS ?? s + 40) - s, v = drv.speed;
      // the braking it takes to stand there (Drive.ts: 8 m/s/s at full brake on asphalt, a little drag)
      throttle = rem <= 0.3 ? -1 : -THREE.MathUtils.clamp((v * v / (2 * rem) - 0.2) / 7.8, 0.05, 1);
    }
    return { steer: THREE.MathUtils.clamp(-err * 2.2, -1, 1), throttle };
  }

  private once(k: string, fn: () => void) { if (!this.done.has(k)) { this.done.add(k); fn(); } }

  private event(dt: number) {
    const { audio, ui } = this.d, w = this.d.world, truck = w.oldTruck, drv = w.oldDrive;
    if (!truck || !drv) return;
    this.et += dt;
    const e = this.et;
    this.g.clock = T + e;
    // 05:29:00: carrier, the engine coughs and stops, the instruments die, the wind drops
    this.once('cut', () => {
      audio.cabRadio('carrier');
      w.engineSound?.stall();
      drv.engine = false;
      truck.setDashLevel(0);
      truck.setLightLevel(0.16);
      audio.amb.gain.setTargetAtTime(0.25, audio.ctx?.currentTime ?? 0, 0.8);
      this.g.note('05:29. On the line.');
    });
    // + 4 s: the dash lamps blink four times, a pause, seven times
    if (e >= 4 && e < 10.8) {
      const t = e - 4;
      const on = t < 2 ? (t % 0.5) < 0.25 : t >= 3.2 && t < 6.7 ? ((t - 3.2) % 0.5) < 0.25 : false;
      truck.setDashLevel(on ? 1 : 0);
    } else if (e >= 10.8) this.once('dashOff', () => truck.setDashLevel(0));
    // + 2 s: the pressure the time away has, the same, coming up under it until FLASH
    if (e >= 2) this.once('pressure', () => { this.bed = audio.pressure(); });
    if (this.bed && e < 34) this.bed.gain.gain.value = 0.063 * sm(2, 6, e) + 0.187 * sm(6, 32, e);
    // + 10 s: the headlights wake: deep blue, then amber, green and red, slowly
    if (e >= 10 && e < 34) {
      const t = (e - 10) / 2.2, i = Math.min(HEADS.length - 1, Math.floor(t)), k = sm(0.55, 1, t - i);
      _c.copy(HEADS[i]).lerp(HEADS[Math.min(HEADS.length - 1, i + 1)], i < HEADS.length - 1 ? k : 0);
      truck.setHeadColor(_c.multiplyScalar(1.6));
      this.headNow.copy(_c);
      truck.setLightLevel(0.16 + 0.84 * sm(10, 11.2, e));
    }
    // someone in a SARO jacket on the road ahead, back to the truck (story/theThird.ts)
    this.thirdStep(e);
    // + 18 s: the radio comes back with the night in pieces, in the wrong order
    if (e >= 18) this.once('radio', () => audio.cabRadio('hiss'));
    const piece = (at: number, k: string, fn: () => void) => { if (e >= at) this.once(k, fn); };
    piece(18.4, 'printer', () => { const h = audio.play('printer', { gain: 1.4, dest: audio.radioIn() ?? undefined }); h?.src.stop((audio.ctx?.currentTime ?? 0) + 2.4); });
    piece(21.0, 'phone', () => { const h = audio.play('phoneRing', { gain: 1.2, dest: audio.radioIn() ?? undefined }); h?.src.stop((audio.ctx?.currentTime ?? 0) + 2.6); });
    piece(23.8, 'tomas', () => { audio.cabVoice(2.4, 520); ui.toast('On the radio, a man\'s voice, far away: "Reference west. No. East."', 3.2); });
    piece(26.8, 'nora', () => { audio.cabVoice(2.0, 820); ui.toast('A woman\'s voice: "Do not stop on the line."', 3.2); });
    piece(29.6, 'cup', () => audio.cupDown(audio.radioIn()));
    piece(31.4, 'carrier2', () => audio.cabRadio('carrier'));
    // + 32 s: FLASH
    if (e >= 32) this.flash(e - 32);
    // + 34 s: nothing at all
    piece(34, 'silence', () => {
      audio.cabRadio(null);
      w.engineSound?.stop();
      audio.silence(true);
      this.bed?.stop(0.04); this.bed = null;
      truck.setHeadColor(null);
    });
    if (e >= 34) truck.setLightLevel(0.2 + 0.12 * (0.5 + 0.5 * Math.sin((e - 34) * 2.1)));
    if (e >= EVENT) this.startPullout();
  }

  // The third one: on the road where the truck will stand, 9 m on (its back to the cab, looking east
  // along C, this area's line z 0: drive/oldRoadLayout.ts), from the moment the night takes the
  // truck. Lit by the headlights in their colour, less the further off it is; the band gives them
  // back. It does not move. Gone in the FLASH.
  private thirdStep(e: number) {
    const w = this.d.world, truck = w.oldTruck, drv = w.oldDrive, road = w.oldRoad;
    if (!truck || !drv || !road || this.stopS === null) return;
    const letGo = this.steady ? THIRD.letGo : 0;
    if (e >= THIRD.until + letGo) {
      if (this.third?.mesh.visible) { this.third.mesh.visible = false; road.thirdGlints(null); }
      return;
    }
    if (!this.third) {
      const f = this.third = new ThirdFigure('third:road');
      // the truck's middle will stand in the lane at stopS, facing along it
      const a = road.lane(this.stopS, new THREE.Vector3()), b = road.lane(this.stopS + 4, new THREE.Vector3());
      const h = Math.atan2(-(b.x - a.x), -(b.z - a.z));
      const at = a.addScaledVector(_v.set(-Math.sin(h), 0, -Math.cos(h)), THIRD.ahead);
      at.y = road.height(at.x, at.z);
      // east along C, but never more than 40 degrees off the way the truck faces: its back to the cab
      let d = -Math.PI / 2 - h; d = Math.atan2(Math.sin(d), Math.cos(d));
      f.place(at, h + THREE.MathUtils.clamp(d, -0.7, 0.7));
      road.group.attach(f.mesh);
    }
    const f = this.third;
    f.mesh.visible = true;
    f.fade = this.steady ? 1 - sm(THIRD.until, THIRD.until + letGo, e) : 1;
    // the headlights on its back: their colour and level now, less the further off it is (the tape
    // gives a beam back much further than the jacket takes it)
    const eye = _eye.copy(truck.driverEye).applyMatrix4(truck.group.matrixWorld);
    const dist = f.mesh.getWorldPosition(_w).distanceTo(eye);
    const level = truck.beam, near = 1 / (1 + (dist / 12) ** 2), tape = 1 / (1 + (dist / 45) ** 2);
    let flare = 0;
    if (!this.steady) for (const tc of THIRD.flares) flare = Math.max(flare, Math.exp(-(((e - tc) / 0.2) ** 2)));
    f.light(_c.copy(this.headNow).multiplyScalar(level * near * 1.8), THIRD_AMB, _band.copy(this.headNow).multiplyScalar(level * tape * (0.8 + 1.3 * flare)));
    // and the tape's glint in the beam (the road's own glow points: no draw of its own)
    road.thirdGlints(f.bandPoints(eye, this.bandPts), _glint.copy(this.headNow).multiplyScalar(level * tape * (0.24 + 0.6 * flare) * f.fade));
  }

  // The frames, one after the other with black between; or slow, dim fades instead.
  private flash(t: number) {
    const fr = this.frames, end = this.ending;
    if (!fr || !end) return;
    if (this.calm) {
      const per = 0.9, i = Math.floor(t / per), k = t - i * per;
      if (i >= fr.length) { end.flash(null); return; }
      end.flash(fr[i].canvas, 0.35 * Math.min(1, k / 0.35, (per - k) / 0.35));
      return;
    }
    let at = 0;
    for (const f of fr) {
      if (t < at + f.ms / 1000) { end.flash(f.canvas, 1); return; }
      at += f.ms / 1000;
      if (t < at + 0.04) { end.flash(null); return; }
      at += 0.04;
    }
    end.flash(null);
  }

  // ---------- the pull-out ----------
  private startPullout() {
    const w = this.d.world, drv = w.oldDrive!;
    this.dropThird();
    this.stage = 'pullout';
    this.pt = 0;
    this.ending?.flash(null);
    this.still = this.d.settings().stillShots;
    this.from.pos.copy(this.d.camera.position);
    this.from.q.copy(this.d.camera.quaternion);
    this.truckAt.pos.copy(drv.pos).setY(w.oldTruck!.group.position.y);
    this.truckAt.heading = drv.heading;
    w.oldControl = { camera: () => this.shoot() };
    this.g.note('05:29:47.');
  }

  // Where the camera is at a moment of the pull-out: [position, look target].
  private keyframes() {
    const P = this.truckAt.pos, h = this.truckAt.heading;
    const F = new THREE.Vector3(-Math.sin(h), 0, -Math.cos(h));
    const front = P.clone().addScaledVector(F, 9).addScaledVector(UP, 1.9);
    const atTruck = P.clone().addScaledVector(UP, 1.2);
    // high to the east-south-east of the truck, looking west-north-west over it at SARO
    const high = P.clone().addScaledVector(dir(104), 170).addScaledVector(UP, 140);
    const westward = P.clone().addScaledVector(dir(284), 160);
    // west-south-west of it, looking along C into the east-north-east, the sun on the mesa
    const sunSide = P.clone().addScaledVector(dir(259), 120).addScaledVector(UP, 150);
    const sunward = P.clone().addScaledVector(dir(79), 3000).addScaledVector(UP, 60);
    const top = P.clone().addScaledVector(dir(259), 220).addScaledVector(UP, 650);
    const stars = P.clone().addScaledVector(dir(79), 400).addScaledVector(UP, 4000);
    return { front, atTruck, high, westward, sunSide, sunward, top, stars };
  }

  private shoot() {
    const cam = this.d.camera, k = this.keyframes(), t = this.pt;
    if (this.still) {
      // four still shots, each held, with a cut through black between them
      const shots: [THREE.Vector3, THREE.Vector3][] = [[k.front, k.atTruck], [k.high, k.westward], [k.sunSide, k.sunward], [k.top, k.stars]];
      const i = Math.min(3, Math.floor(t / 8.5));
      cam.position.copy(shots[i][0]); cam.lookAt(shots[i][1]);
      const into = t - i * 8.5;
      this.ending?.black(into < 0.5 || (into > 8.0 && i < 3), 350);
    } else if (t < 6.5) {
      const e = sm(0, 6.5, t);
      cam.position.lerpVectors(this.from.pos, k.front, e);
      _m.lookAt(cam.position, k.atTruck, UP); _q.setFromRotationMatrix(_m);
      cam.quaternion.slerpQuaternions(this.from.q, _q, sm(0.5, 5.5, t));
    } else if (t < 13) {
      const e = sm(6.5, 13, t);
      cam.position.lerpVectors(k.front, k.high, e);
      cam.lookAt(_v.lerpVectors(k.atTruck, k.westward, e));
    } else if (t < 21) {
      const e = sm(13, 21, t);
      cam.position.lerpVectors(k.high, k.sunSide, e);
      cam.lookAt(_v.lerpVectors(k.westward, k.sunward, e));
    } else {
      const e = sm(21, PULL, t);
      cam.position.lerpVectors(k.sunSide, k.top, e);
      cam.lookAt(_v.lerpVectors(k.sunward, k.stars, sm(22, PULL, t)));
    }
    cam.updateMatrixWorld();
  }

  private pullout(dt: number) {
    const w = this.d.world, truck = w.oldTruck, road = w.oldRoad;
    this.pt += dt;
    const t = this.pt;
    this.g.clock = T + EVENT + t;
    if (!truck || !road) return;
    // the truck alone on the road, its headlights pulsing faintly
    truck.setLightLevel(0.2 + 0.12 * (0.5 + 0.5 * Math.sin((EVENT - 34 + t) * 2.1)));
    // the higher the camera, the thinner the haze and the darker the sky, until the stars are back
    const hgt = this.d.camera.position.y - road.height(this.d.camera.position.x, this.d.camera.position.z);
    this.d.fog(0.00055 * (1 - sm(10, 420, hgt)) + 0.00006);
    this.skyLift = 1 - 0.85 * sm(180, 640, hgt);
    // the sun's upper edge touches the top of the mesa (as seen from here) at 05:30:00, then rises:
    // the mesa in the painted horizon, which dips under the camera as it goes up (backdrop.ts)
    const mesaTop = SUN_MESA.top - horizonDip(hgt);
    this.sunElev = mesaTop - 0.27 + (this.g.clock - (5 * 3600 + 30 * 60)) * 0.05;
    if (t >= PULL) this.once('black', () => {
      this.ending?.black(true, 1400);
      this.g.after(1.6, () => { this.stage = 'end'; this.d.finale(); });
    });
  }

  /** The whole 1947 photograph for after the credits (null if the picture is missing). */
  photo(): HTMLCanvasElement | null {
    if (!artLoaded('photo1947')) return null;
    const im = artImage('photo1947'), c = document.createElement('canvas');
    c.width = 1200; c.height = 800;
    c.getContext('2d')!.drawImage(im, 0, 0, c.width, c.height);
    return c;
  }

  // ---------- per frame ----------
  update(dt: number, _t: number) {
    if (!this.active) return;
    if (this.stage === 'drive') this.drive(dt);
    else if (this.stage === 'event') this.event(dt);
    else if (this.stage === 'pullout') this.pullout(dt);
    this.d.ui.objective(this.objective());
  }
}
