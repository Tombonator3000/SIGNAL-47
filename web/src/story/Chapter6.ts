import * as THREE from 'three';
import type { UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { World } from '../world/World';
import type { DriveInput } from '../drive/Drive';
import { Ending } from '../ui/Ending';
import { flashFrames, type Frame } from './flashFrames';
import { artLoaded, artImage, loadArtFor } from '../core/art';
import { clockText } from './time';

// Chapter six, "Roswell Road" and THE EVENT (KAPITLER.md). From the diner's lot out onto the
// old road at 05:26, east-north-east and then north-east into the dawn, past the six-, seven-
// and eight-mile posts to where C crosses, just past the eight. On the line the night takes
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
const START = 5 * 3600 + 26 * 60 + 20;   // the cut puts the truck on the road at 05:26:20
const EVENT = 47, PULL = 34;
const sm = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
const dir = (deg: number) => new THREE.Vector3(Math.sin(deg * Math.PI / 180), 0, -Math.cos(deg * Math.PI / 180));
const UP = new THREE.Vector3(0, 1, 0);
const HEADS = [new THREE.Color(0.12, 0.25, 1.0), new THREE.Color(1.0, 0.55, 0.12), new THREE.Color(0.2, 1.0, 0.35), new THREE.Color(1.0, 0.08, 0.05)];
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
  private frames: Frame[] | null = null;
  private still = false;
  private calm = false;
  private idle = 0;
  private backT = 0;
  private from = { pos: new THREE.Vector3(), q: new THREE.Quaternion() };
  private truckAt = { pos: new THREE.Vector3(), heading: 0 };

  constructor(private g: Chapter6Host, private d: Chapter6Deps) {}

  // ---------- lifecycle ----------
  /** From the diner's lot: the cut onto the old road (the case was saved in the diner first). */
  begin() {
    this.reset();
    this.active = true; this.started = true;
    this.stage = 'drive';
    void loadArtFor(['photo1947']).catch(() => {});
    this.d.audio.amRadio(null);   // the diner's radio stays at the diner
    this.d.world.driveOldRoad(() => {
      this.g.clock = START;
      this.d.audio.cabRadio('talk');
      this.g.note(`${this.time()}. The old road, out past the six-mile post. The sky is coming up grey in the east, and the road goes off into it. C crosses just past the eight.`);
      const radio = (at: number, text: string) => this.g.after(at, () => { if (this.stage === 'drive') this.d.ui.toast(text, 5); });
      radio(7, 'The radio, low: "...cattle at the sale barn steady, a dollar higher on calves..."');
      radio(34, 'The radio: "...clear today, high near seventy-four in Roswell, light winds..."');
      radio(78, 'The radio: "...and Halley\'s comet is still low in the south after midnight this week, if you can get away from the town lights."');
    });
    // the frames of FLASH are drawn now, so the moment itself does not wait for them
    this.g.after(3, () => { if (this.active && !this.frames) this.frames = flashFrames(); });
  }

  reset() {
    this.active = false; this.started = false; this.stage = 'drive';
    this.et = 0; this.pt = 0; this.done.clear(); this.idle = 0; this.backT = 0;
    this.skyLift = null; this.sunElev = null;
    this.ending?.dispose(); this.ending = null;
    if (this.d.world.oldControl) this.d.world.oldControl = null;
    this.d.fog(null);
    this.d.audio.cabRadio(null);
    this.d.audio.silence(false);
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
    // the clock never shows 05:29 before the line: it slows to a crawl in the last 20 seconds
    const left = T - this.g.clock;
    if (left < 20) this.g.clock -= dt * (1 - Math.max(0.02, left / 20));
    this.g.clock = Math.min(this.g.clock, T - 0.05);
    // fronts of the truck over C: the night begins
    const h = drv.heading, fx = drv.pos.x - Math.sin(h) * 2.3, fz = drv.pos.z - Math.cos(h) * 2.3;
    if (road.pastC(fx, fz) >= 0) { this.startEvent(); return; }
    if (road.hitBack) { road.hitBack = false; if (this.backT <= 0) { this.d.ui.toast('Ward can wait. The bolt is ahead.', 3.4); this.backT = 6; } }
    this.backT -= dt;
    this.idle = Math.abs(drv.speed) < 0.5 ? this.idle + dt : 0;
    if (this.idle > 40 && !this.done.has('nudge')) { this.done.add('nudge'); this.d.ui.toast('The line is just ahead, past the eight-mile post.', 4); }
  }

  // ---------- THE EVENT ----------
  private startEvent() {
    const w = this.d.world;
    this.stage = 'event';
    this.et = 0;
    this.g.clock = T;
    this.calm = this.d.settings().calmFlash;
    if (!this.frames) this.frames = flashFrames();
    this.ending = new Ending();
    // the truck coasts to a stop in its lane; the player keeps only their eyes
    w.oldControl = { input: (dt) => this.coast(dt) };
    this.d.ui.objective(null);
    this.d.ui.clearToasts();
  }

  /** Steer along the right lane and brake gently: the truck rolls out and stands. */
  private coast(_dt: number): DriveInput {
    const w = this.d.world, drv = w.oldDrive!, road = w.oldRoad!;
    const tg = road.lane(road.where(drv.pos.x, drv.pos.z).s + 14, _w);
    let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading;
    err = Math.atan2(Math.sin(err), Math.cos(err));
    return { steer: THREE.MathUtils.clamp(-err * 2.2, -1, 1), throttle: this.et > 0.8 && drv.speed > 0.05 ? -0.42 : 0 };
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
    // + 10 s: the headlights wake: deep blue, then amber, green and red, slowly
    if (e >= 10 && e < 34) {
      const t = (e - 10) / 2.2, i = Math.min(HEADS.length - 1, Math.floor(t)), k = sm(0.55, 1, t - i);
      _c.copy(HEADS[i]).lerp(HEADS[Math.min(HEADS.length - 1, i + 1)], i < HEADS.length - 1 ? k : 0);
      truck.setHeadColor(_c.multiplyScalar(1.6));
      truck.setLightLevel(0.16 + 0.84 * sm(10, 11.2, e));
    }
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
      truck.setHeadColor(null);
    });
    if (e >= 34) truck.setLightLevel(0.2 + 0.12 * (0.5 + 0.5 * Math.sin((e - 34) * 2.1)));
    if (e >= EVENT) this.startPullout();
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
    // the sun's upper edge touches the top of the mesa (as seen from here) at 05:30:00, then rises
    const mesaTop = Math.atan2(road.crossY + 120 - this.d.camera.position.y, 4300) * 180 / Math.PI;
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
