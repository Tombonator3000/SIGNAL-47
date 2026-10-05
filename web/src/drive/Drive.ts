import * as THREE from 'three';
import type { FloodSet } from '../world/kit';
import { WHEELBASE, type Truck } from './Truck';

/*
  DRIVING: how the pieces fit together
  ------------------------------------
  Truck (Truck.ts)          the SARO 07 pickup. `group` is the exterior, `cab` the interior.
                            It knows nothing about roads: it shows lights, dash and wheels.
  DriveController (here)    arcade physics (kinematic bicycle), collisions, the first-person
                            camera at truck.driverEye. Each update() it places truck.group,
                            calls truck.update(), setSteer(), setBrake() and setDriving(true).
  RoadArea (RoadArea.ts)    the drive itself, built around a world origin. It answers
                            surface(x, z), height(x, z), obstacles and endZone (the DriveArea
                            interface below), and owns the flood set `roadFlood`.
  EngineSound (engineSound.ts)  Web Audio engine, tyres, cattle guard rattle and stall.

  Wiring in the game loop (World.ts already does most of this):
    const road = new RoadArea(ROAD_ORIGIN);            // adds its own hemisphere light and moon
    const truck = new Truck();                         // new Truck({ flood: set }) lights the body too
    const drive = new DriveController(camera, truck, road);
    // the controller hands road.headlights (roadFlood, slots 0 to 2) to the truck:
    // truck.headlightFloods(roadFlood, [0, 1, 2]) needs no call of its own
    drive.place(road.start.pos, road.start.heading);
    truck.setHeadlights(true); truck.setLightLevel(1);
    drive.onArrive = () => ...;                        // once, in road.endZone below 6 m/s
    drive.onBump = (speed) => audio.play('thudSoft', { gain: Math.min(1, speed / 8) });
    road.onCattleGuard = (speed) => engine.rattle(speed);
    per frame, instead of player.update():
      const m = input.move();                          // W/S or the left stick: throttle and brake
      drive.update(dt, { steer: m.x, throttle: -m.z }, input.consumeLook()); // A/D: steer
      road.update(dt, t, drive.pos);                   // reflector glints, sign shine, cattle guard
      engine.set(drive.rpm, drive.load); engine.tyres(drive.surface.kind, Math.abs(drive.speed));
      truck.setDash({ mph: drive.mph, rpm: drive.rpm, clock, fuel });   // 10 Hz is plenty
    on arrival: truck.setDriving(false) shows the whole truck again (it also happens by itself
    on the next frame once the camera is outside the cab). A camera put outside while
    drive.update() keeps running needs truck.setDriving(false) after each update.
    road.route lists world waypoints of the way (lane, then track), for hints or tests.

  Input: throttle 1 accelerates, -1 brakes and, once stopped, reverses (up to 5 m/s); positive
  steer turns right. Look: the mouse or the right-hand touch drag turns the head (yaw within
  110 degrees of straight ahead, pitch clamped); driving forward eases the view back to the road.
  Camera: drive.update() writes camera.position and camera.quaternion; do not call
  player.update() in the same frame. A little vibration follows engine load and the surface.
  Floods: roadFlood has 8 slots. 0 to 2 are the headlight pools (near, middle, far), moved by
  the truck every frame; 3 is the ranch yard light, 4 the lamp at the station gate.
  Stall (later, the drive to Roswell): drive.engine = false lets the truck coast with brakes and
  steering, rpm falls to 0; engine.stall() winds the sound down; truck.setLightLevel() blinks.
*/

export type SurfaceKind = 'asphalt' | 'gravel' | 'dirt';
/** grip 0..1 scales acceleration, braking and cornering; top is the top speed in m/s;
 *  rough 0..1 drives bumps and vibration (optional, a default follows the kind). */
export type Surface = { kind: SurfaceKind; grip: number; top: number; rough?: number };
export type Box2 = { minX: number; maxX: number; minZ: number; maxZ: number };
export type Obstacle = ({ kind: 'circle'; x: number; z: number; r: number }) | ({ kind: 'box' } & Box2);
export interface DriveArea {
  surface(x: number, z: number): Surface;
  /** World height of the ground; flat ground at y = 0 if left out. */
  height?(x: number, z: number): number;
  obstacles: Obstacle[];
  endZone: Box2;
  /** true where the truck's centre may not go (a curved right of way; the old road). */
  blocked?(x: number, z: number): boolean;
  /** Flood slots for the headlight pools; the controller hands them to the truck. */
  headlights?: { set: FloodSet; slots: number[] };
}
export type DriveInput = { steer: number; throttle: number };

const MAX_STEER = 32 * Math.PI / 180;
const LF = 1.8, LR = WHEELBASE - LF;          // centre of the truck to the front and rear axles
const HALF_W = 1.0, HALF_L = 2.65, TRACK = 0.84;
const ACCEL = 3.5, BRAKE = 8, REVERSE = 5, LAT = 7.5;
const RATIOS = [2.48, 1.48, 1.0], FINAL = 3.08, IDLE = 650;
const ROUGH: Record<SurfaceKind, number> = { asphalt: 0.05, gravel: 0.4, dirt: 0.8 };
const YAW_MAX = 110 * Math.PI / 180, PITCH0 = -0.08;
const _q = new THREE.Quaternion(), _q2 = new THREE.Quaternion(), _e = new THREE.Euler(), _v = new THREE.Vector3();

export class DriveController {
  /** World position of the truck's centre on the ground. */
  pos = new THREE.Vector3();
  /** Radians, like rotation.y: 0 faces -Z (north), PI/2 faces -X. */
  heading = 0;
  /** m/s along the heading, negative in reverse. */
  speed = 0;
  mph = 0;
  rpm = IDLE;
  gear = 1;
  /** 0..1, how hard the engine works (throttle); for the engine sound. */
  load = 0;
  /** The surface under the truck now. */
  surface: Surface = { kind: 'asphalt', grip: 1, top: 30 };
  /** false: the engine has died. The truck coasts, brakes and steers, and rpm falls to 0. */
  engine = true;
  onArrive?: () => void;
  onBump?: (speed: number) => void;
  /** In reverse gear (S held at a stop). */
  get reversing() { return this.reverse; }

  private steer = 0;
  private reverse = false;
  private shiftHold = 0;
  private yaw = 0; private pitch = PITCH0; private idle = 0;
  private arrived = false;
  private bumpT = 0;
  private t = 0;
  private accel = 0;
  private yawRate = 0;
  private body = { y: 0, vy: 0, pitch: 0, vp: 0, roll: 0, vr: 0 };

  constructor(public camera: THREE.PerspectiveCamera, public truck: Truck, public area: DriveArea) {
    if (area.headlights) truck.headlightFloods(area.headlights.set, area.headlights.slots);
  }

  /** Put the truck at a world position and heading, standing still, view straight ahead. */
  place(pos: THREE.Vector3, heading: number) {
    this.pos.set(pos.x, 0, pos.z);
    this.heading = heading;
    this.speed = 0; this.steer = 0; this.reverse = false; this.gear = 1; this.accel = 0; this.yawRate = 0;
    this.yaw = 0; this.pitch = PITCH0; this.arrived = false; this.engine = true;
    const g = this.ground();
    Object.assign(this.body, { y: g.y, vy: 0, pitch: g.pitch, vp: 0, roll: g.roll, vr: 0 });
    this.surface = this.area.surface(this.pos.x, this.pos.z);
    this.truck.setDriving(true);
    this.pose(0, 0);
  }

  /** Go on from another controller's truck in another area (the hand-over between two
   *  areas on a drive): this truck at (x, z, heading), moving and steering as that one was. */
  adopt(o: DriveController, x: number, z: number, heading: number) {
    this.place(new THREE.Vector3(x, 0, z), heading);
    this.speed = o.speed; this.steer = o.steer; this.reverse = o.reverse; this.gear = o.gear; this.rpm = o.rpm; this.load = o.load;
    this.accel = o.accel; this.yawRate = o.yawRate; this.yaw = o.yaw; this.pitch = o.pitch; this.idle = o.idle; this.t = o.t; this.engine = o.engine;
    this.body.vy = o.body.vy; this.body.vp = o.body.vp; this.body.vr = o.body.vr;
    this.mph = o.mph;
    this.pose(0, 0);
  }

  update(dt: number, input: DriveInput, look: { x: number; y: number }) {
    this.t += dt;
    const throttle = THREE.MathUtils.clamp(input.throttle, -1, 1);
    // ---------- steering: the wheel turns at a limited rate, less lock at speed ----------
    const want = THREE.MathUtils.clamp(input.steer, -1, 1);
    const rate = Math.abs(want) > Math.abs(this.steer) && Math.sign(want) === Math.sign(this.steer || want) ? 2.2 : 3.6;
    this.steer += THREE.MathUtils.clamp(want - this.steer, -rate * dt, rate * dt);
    // ---------- longitudinal ----------
    const s = this.surface = this.area.surface(this.pos.x, this.pos.z);
    const v = this.speed, dir = this.reverse ? -1 : 1;
    const go = throttle * dir;                 // > 0: pull in the selected direction (D or R)
    const pulling = this.engine && go > 0;
    const braking = this.engine ? go < 0 : throttle < 0;
    let a = 0;
    if (pulling) {
      // the pull fades towards 1.19 x top, where it just matches drag at `top`
      const top = (this.reverse ? REVERSE : s.top) * 1.19;
      a = dir * ACCEL * go * (0.55 + 0.45 * s.grip) * Math.max(0, 1 - (Math.abs(v) / top) ** 2);
    } else if (braking) {
      a = -Math.sign(v) * BRAKE * Math.abs(throttle) * (0.5 + 0.5 * s.grip);
    }
    // a stopped truck held on the brake for a moment changes direction (D to R and back)
    if (this.engine && braking && Math.abs(v) < 0.3) {
      this.shiftHold += dt;
      if (this.shiftHold > 0.3) { this.reverse = !this.reverse; this.shiftHold = 0; }
    } else this.shiftHold = 0;
    // rolling resistance (more on loose ground), air, engine braking, soft ground above its top speed
    const rough = s.rough ?? ROUGH[s.kind];
    let drag = 0.18 + 0.5 * rough + 0.0009 * v * v + (!pulling && this.engine ? 0.25 : 0);
    if (Math.abs(v) > s.top) drag += (Math.abs(v) - s.top) * 0.45;
    a -= Math.sign(v) * drag;
    this.accel = a;
    // ---------- move in small steps, so nothing is skipped at speed ----------
    const n = Math.min(8, Math.max(1, Math.ceil(Math.abs(v) * dt / 0.25)));
    const h = dt / n;
    for (let i = 0; i < n; i++) {
      const before = this.speed;
      this.speed += a * h;
      // brakes and drag stop the truck; they never push it the other way
      if (!pulling && before !== 0 && Math.sign(this.speed) !== Math.sign(before)) { this.speed = 0; a = 0; }
      const vmax = Math.max(1, Math.abs(this.speed));
      const lock = Math.min(MAX_STEER, Math.atan(WHEELBASE * LAT * (0.45 + 0.55 * s.grip) / (vmax * vmax)));
      const beta = Math.atan(LR / WHEELBASE * Math.tan(this.steer * lock));
      const ox = this.pos.x, oz = this.pos.z, oh = this.heading;
      this.yawRate = -this.speed / LR * Math.sin(beta);
      this.heading += this.yawRate * h;
      // the centre moves a little to the inside of the heading (the slip angle beta)
      this.pos.x -= Math.sin(this.heading - beta) * this.speed * h;
      this.pos.z -= Math.cos(this.heading - beta) * this.speed * h;
      if (this.hit()) {
        const impact = Math.abs(this.speed);
        this.pos.x = ox; this.pos.z = oz; this.heading = oh;
        this.speed = -Math.sign(this.speed) * Math.min(1.2, impact * 0.15);
        if (impact > 0.8 && this.bumpT <= 0) { this.bumpT = 0.6; this.onBump?.(impact); }
        break;
      }
    }
    this.bumpT -= dt;
    this.mph = Math.abs(this.speed) * 2.23694;
    // ---------- a three-speed automatic, for the sound and the dash ----------
    const fwd = Math.max(0, this.speed);
    const pull = pulling ? go : 0;
    const up = [4.5 + 8 * pull, 10 + 12 * pull];
    if (this.reverse) this.gear = 1;
    else if (this.gear < 3 && fwd > up[this.gear - 1]) this.gear++;
    else if (this.gear > 1 && fwd < up[this.gear - 2] - 3.5) this.gear--;
    const wheelRpm = Math.abs(this.speed) * 60 / (2 * Math.PI * 0.37);
    const coupled = wheelRpm * (this.reverse ? 2.08 : RATIOS[this.gear - 1]) * FINAL;
    const slip = (IDLE + pull * 1500) * Math.max(0, 1 - Math.abs(this.speed) / 7);
    const target = this.engine ? Math.max(IDLE, coupled, slip) : 0;
    this.rpm += (target - this.rpm) * (1 - Math.exp(-dt * (this.engine ? 6 : 1.6)));
    this.load = this.engine ? pull : 0;
    // ---------- arrival ----------
    const z = this.area.endZone, p = this.pos;
    if (!this.arrived && p.x >= z.minX && p.x <= z.maxX && p.z >= z.minZ && p.z <= z.maxZ && Math.abs(this.speed) < 6) {
      this.arrived = true;
      this.onArrive?.();
    }
    // ---------- look: free head within limits, eased back to the road when driving on ----------
    if (look.x || look.y) this.idle = 0; else this.idle += dt;
    this.yaw = THREE.MathUtils.clamp(this.yaw - look.x, -YAW_MAX, YAW_MAX);
    this.pitch = THREE.MathUtils.clamp(this.pitch - look.y, -0.9, 0.6);
    if (this.speed > 2 && this.idle > 0.8) {
      const k = 1 - Math.exp(-dt * 1.4 * Math.min(1, this.speed / 8));
      this.yaw += (0 - this.yaw) * k; this.pitch += (PITCH0 - this.pitch) * k;
    }
    this.truck.setBrake(braking);
    this.pose(dt, this.steer * Math.min(MAX_STEER, Math.atan(WHEELBASE * LAT / Math.max(1, this.speed * this.speed))));
  }

  // Heights under the four wheels give the body its height, pitch and roll.
  private ground() {
    const h = this.area.height;
    if (!h) return { y: 0, pitch: 0, roll: 0, at: [0, 0, 0, 0] };
    const sx = -Math.sin(this.heading), sz = -Math.cos(this.heading); // forward
    const rx = Math.cos(this.heading), rz = -Math.sin(this.heading);  // right
    const at = (f: number, r: number) => h.call(this.area, this.pos.x + sx * f + rx * r, this.pos.z + sz * f + rz * r);
    const fl = at(LF, -TRACK), fr = at(LF, TRACK), rl = at(-LR, -TRACK), rr = at(-LR, TRACK);
    const front = (fl + fr) / 2, rear = (rl + rr) / 2;
    return { y: rear + (front - rear) * LR / WHEELBASE, pitch: Math.atan2(front - rear, WHEELBASE), roll: Math.atan2((fr + rr - fl - rl) / 2, 2 * TRACK), at: [fl, fr, rl, rr] };
  }

  // Overlap of the truck's footprint (an oriented rectangle) with any obstacle.
  private hit() {
    const c = Math.cos(this.heading), sn = Math.sin(this.heading), x = this.pos.x, z = this.pos.z;
    if (this.area.blocked?.(x, z)) return true;
    for (const o of this.area.obstacles) {
      if (o.kind === 'circle') {
        const dx = o.x - x, dz = o.z - z;
        if (dx * dx + dz * dz > (o.r + 2.9) ** 2) continue;
        const lx = dx * c - dz * sn, lz = dx * sn + dz * c;
        const qx = lx - THREE.MathUtils.clamp(lx, -HALF_W, HALF_W), qz = lz - THREE.MathUtils.clamp(lz, -HALF_L, HALF_L);
        if (qx * qx + qz * qz < o.r * o.r) return true;
      } else {
        const bx = (o.minX + o.maxX) / 2, bz = (o.minZ + o.maxZ) / 2, hx = (o.maxX - o.minX) / 2, hz = (o.maxZ - o.minZ) / 2;
        const ac = Math.abs(c), as = Math.abs(sn), dx = bx - x, dz = bz - z;
        if (Math.abs(dx) > hx + HALF_W * ac + HALF_L * as) continue;
        if (Math.abs(dz) > hz + HALF_W * as + HALF_L * ac) continue;
        if (Math.abs(dx * c - dz * sn) > HALF_W + hx * ac + hz * as) continue;
        if (Math.abs(dx * sn + dz * c) > HALF_L + hx * as + hz * ac) continue;
        return true;
      }
    }
    return false;
  }

  // Body on its springs, then the camera at the driver's eye.
  private pose(dt: number, wheelAngle: number) {
    const g = this.ground(), b = this.body;
    const rough = this.surface.rough ?? ROUGH[this.surface.kind];
    const fast = Math.min(1, Math.abs(this.speed) / 12);
    const t = this.t;
    // bumps: a few unrelated sines, scaled by surface and speed
    const bump = rough * fast * 0.025 * (Math.sin(t * 13.1) * 0.5 + Math.sin(t * 21.7 + 1.1) * 0.3 + Math.sin(t * 34.3 + 2.3) * 0.2);
    const latAcc = -this.speed * this.yawRate; // positive in a right turn: the body leans left
    const tp = g.pitch + THREE.MathUtils.clamp(this.accel * 0.0025, -0.03, 0.015) + bump * 0.6;
    const tr = g.roll + THREE.MathUtils.clamp(latAcc * 0.005, -0.05, 0.05);
    const spring = (x: number, vx: number, target: number) => {
      if (dt <= 0) return [target, 0];
      const k = 140, c = 18;
      const acc = k * (target - x) - c * vx;
      vx += acc * dt; return [x + vx * dt, vx];
    };
    [b.y, b.vy] = spring(b.y, b.vy, g.y + bump);
    [b.pitch, b.vp] = spring(b.pitch, b.vp, tp);
    [b.roll, b.vr] = spring(b.roll, b.vr, tr);
    const tg = this.truck.group;
    tg.position.set(this.pos.x, b.y, this.pos.z);
    tg.rotation.set(b.pitch, this.heading, b.roll, 'YXZ');
    tg.updateMatrixWorld();
    this.truck.setDriving(true);
    this.truck.setSteer(this.steer);
    // the wheels stay on the ground while the body rides its springs: each one hangs as far
    // below its place on the body as the ground is (fl, fr, rl, rr)
    const sr = Math.sin(b.roll) * Math.cos(b.pitch), sp = Math.sin(b.pitch);
    this.truck.setWheelDrops(g.at.map((h, i) => h - (b.y + (i % 2 ? TRACK : -TRACK) * sr - (i < 2 ? -LF : LR) * sp)));
    this.truck.update(dt, this.speed, wheelAngle);
    // camera: the eye, a little engine shake and road vibration
    const eng = this.rpm > 50 ? (0.0004 + 0.0008 * Math.min(1, this.rpm / 3000)) * (0.5 + this.load) : 0;
    const road = (rough * 0.01 + 0.001) * fast;
    _v.copy(this.truck.driverEye);
    _v.y += Math.sin(t * 47.3) * eng + Math.sin(t * 17.9 + 0.7) * road * 0.5;
    _v.x += Math.sin(t * 23.1 + 2.1) * road * 0.3;
    this.camera.position.copy(_v.applyMatrix4(tg.matrixWorld));
    const jitter = (Math.sin(t * 19.3) * 0.6 + Math.sin(t * 29.9 + 1.7) * 0.4) * road * 0.5;
    _q.setFromEuler(_e.set(b.pitch, this.heading, b.roll, 'YXZ'));
    _q2.setFromEuler(_e.set(this.pitch + jitter, this.yaw, 0, 'YXZ'));
    this.camera.quaternion.copy(_q).multiply(_q2);
  }
}
