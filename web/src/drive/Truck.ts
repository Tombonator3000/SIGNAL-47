import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, plane, mergeStatic, noMerge, floodlit, floodSet, type FloodSet } from '../world/kit';
import { GlowPoints } from '../world/glow';
import { PAINT, DASH, DIALS, paintAtlas, dashAtlas, type Rect } from './truckTextures';

// SARO 07, the observatory's service truck: a 1980s full-size single-cab pickup, white with
// faded paint, a long bed with a toolbox and a coil of survey cable. 5.3 x 2.0 x 1.8 m.
// Origin on the ground at the centre of the truck, forward is -Z, the driver sits left (-X).
//
//   group   everything. Inside it:
//   body    stays visible while driving: hood, fenders, grille, bed, lamps, mirrors, wheels.
//   shell   roof, doors, glass and pillars. Hidden while driving, since it would sit in
//           front of the driver's eye. The hood stays: it gives the road its speed.
//   cab     the interior. Shown only while driving.
// Static parts are merged per material: about 14 draw calls outside, 16 while driving.

export const WHEELBASE = 3.3;
const FRONT_AXLE = -1.8, REAR_AXLE = FRONT_AXLE + WHEELBASE, WHEEL_R = 0.37, TRACK = 0.84;
// The fake headlight floods (kit.floodlit): truck-local position and intensity. Three
// pools in a row make a beam that widens and fades down the road.
const FLOODS = [
  { p: new THREE.Vector3(0, 0.9, -7.8), w: 5.5 },
  { p: new THREE.Vector3(0, 1.2, -14.6), w: 10 },
  { p: new THREE.Vector3(0, 1.8, -28.6), w: 14 },
];
const HEAD = new THREE.Color(1.0, 0.9, 0.72), TAIL = new THREE.Color(1, 0.05, 0.03), AMBER = new THREE.Color(1, 0.55, 0.12);
const FACES = ['+x', '-x', '+y', '-y', '+z', '-z'] as const;
type Face = typeof FACES[number];

// Box faces map into regions of the paint atlas: sides get dust, tops stay plain.
function uvBox(m: THREE.Mesh, side: Rect, top: Rect, special: Partial<Record<Face, Rect>> = {}) {
  const uv = m.geometry.attributes.uv as THREE.BufferAttribute;
  for (let i = 0; i < uv.count; i++) {
    const f = Math.floor(i / 4); // BoxGeometry: four vertices per face, in FACES order
    const r = special[FACES[f]] ?? (f === 2 || f === 3 ? top : side);
    uv.setXY(i, r[0] + uv.getX(i) * (r[2] - r[0]), r[1] + uv.getY(i) * (r[3] - r[1]));
  }
  return m;
}
function uvPlane(m: THREE.Mesh, r: Rect) { return uvBox(m, r, r); }
const _q = new THREE.Quaternion(), _e = new THREE.Euler(), _m = new THREE.Matrix4(), _p = new THREE.Vector3();
const _s = new THREE.Vector3(), _z = new THREE.Vector3(0, 0, 1), _c = new THREE.Color();
// an unlit lens: its glass colour when off, plus the lamp colour times k
const lens = (m: THREE.MeshBasicMaterial, off: number, c: THREE.Color, k: number) => m.color.setHex(off).add(_c.copy(c).multiplyScalar(k));

// One wheel for all four instances: tyre, white steel rim with five holes (so the spin
// shows), axis along X. Colours are per vertex so it costs one draw call.
function wheelGeometry() {
  const tint = (g: THREE.BufferGeometry, v: number) => {
    g.setAttribute('color', new THREE.Float32BufferAttribute(new Array(g.attributes.position.count * 3).fill(v), 3));
    return g;
  };
  const parts = [tint(new THREE.CylinderGeometry(WHEEL_R, WHEEL_R, 0.24, 16), 0.035), tint(new THREE.CylinderGeometry(0.2, 0.2, 0.246, 14), 0.55),
    tint(new THREE.CylinderGeometry(0.055, 0.055, 0.252, 8), 0.7)];
  for (let k = 0; k < 5; k++) {
    const a = k / 5 * Math.PI * 2;
    parts.push(tint(new THREE.BoxGeometry(0.05, 0.25, 0.07).rotateY(-a).translate(Math.cos(a) * 0.13, 0, Math.sin(a) * 0.13), 0.03));
  }
  return mergeGeometries(parts)!.rotateZ(Math.PI / 2);
}

export class Truck {
  group = new THREE.Group();
  body = new THREE.Group();
  shell = new THREE.Group();
  cab = new THREE.Group();
  /** The driver's eye, truck-local (left seat). */
  driverEye = new THREE.Vector3(-0.42, 1.48, 0.2);
  driving = false;
  private m: ReturnType<Truck['materials']>;
  private wheels: THREE.InstancedMesh;
  private wheelSpin = new THREE.Group();
  private needles!: THREE.InstancedMesh;
  private glow = new GlowPoints();
  private gi = { head: [] as number[], park: [] as number[], tail: [] as number[] };
  private level = 1; private headOn = false; private brakeOn = false;
  private headColor = new THREE.Color().copy(HEAD); private dashLevel: number | null = null;
  private spin = 0; private wheelAngle = 0;
  private flood: { set: FloodSet; slots: number[] } | null = null;
  private dash = dashAtlas();
  private clockShown = ''; private clockWanted = ''; private clockAt = -1;
  private gauge = { speed: 0, fuel: 0.6, temp: 0.15, tSpeed: 0, tFuel: 0.6, tTemp: 0.5 };
  private eyeW = new THREE.Vector3(); private camW = new THREE.Vector3();
  // The instruments' own light on the wheel and the dash: a one-lamp flood set of its own.
  private cabFlood = floodSet(1, 'cab', 0.5);

  /** flood: light the exterior with a set of fake floods (leave out the headlight slots). */
  constructor(o: { flood?: FloodSet } = {}) {
    this.m = this.materials(o.flood);
    this.group.name = 'SARO 07';
    this.group.add(this.body, this.shell, this.cab);
    this.buildBody(); this.buildShell(); this.buildCab();
    mergeStatic(this.body); mergeStatic(this.shell); mergeStatic(this.cab);
    this.wheels = new THREE.InstancedMesh(wheelGeometry(), this.m.wheel, 4);
    this.wheels.frustumCulled = false; // the instances move; the truck is small anyway
    // Whoever puts the camera outside the cab gets the whole truck back on the next frame,
    // even if nobody calls setDriving(false) (a cut to the parked truck, for example).
    this.wheels.onBeforeRender = (_r, _s, cam) => this.autoView(cam);
    this.body.add(this.wheels);
    for (const s of [-1, 1]) {
      this.gi.head.push(this.glow.add(s * 0.7, 0.87, -2.6, 0.75, 0));
      this.gi.park.push(this.glow.add(s * 0.7, 0.71, -2.6, 0.24, 0));
      this.gi.tail.push(this.glow.add(s * 0.93, 0.95, 2.6, 0.45, 0));
    }
    this.body.add(this.glow.build());
    this.setDriving(false);
    this.update(0, 0);
    this.applyLights();
  }

  private materials(flood?: FloodSet) {
    const lit = <T extends THREE.MeshStandardMaterial>(m: T) => flood ? floodlit(m, 0.02, flood) : m;
    const std = (o: THREE.MeshStandardMaterialParameters) => lit(new THREE.MeshStandardMaterial(o));
    const basic = (c: number) => new THREE.MeshBasicMaterial({ color: c });
    const cab = <T extends THREE.MeshStandardMaterial>(m: T) => floodlit(m, 16, this.cabFlood);
    return {
      paint: std({ map: paintAtlas(), roughness: 0.55, metalness: 0.05 }),
      chrome: std({ color: 0xc4c7c9, roughness: 0.3, metalness: 0.55 }),
      trim: std({ color: 0x1a1b1d, roughness: 0.75 }),
      glass: std({ color: 0x0a0e13, roughness: 0.12, metalness: 0.4 }),
      wheel: std({ vertexColors: true, roughness: 0.85 }),
      head: basic(0x2c2c2a), park: basic(0x3a2a10), tail: basic(0x2a0606), beacon: basic(0x4a3010),
      // the interior is lit by the instruments only (the headlights do not shine into the cab)
      dash: cab(new THREE.MeshStandardMaterial({ color: 0x343537, roughness: 0.7 })),
      wheelRim: cab(new THREE.MeshStandardMaterial({ color: 0x2c2926, roughness: 0.5 })),
      vinyl: cab(new THREE.MeshStandardMaterial({ color: 0x6a5240, roughness: 0.6 })),
      liner: cab(new THREE.MeshStandardMaterial({ color: 0x77736a, roughness: 1 })),
      mirror: basic(0x131b28),
      gauge: new THREE.MeshBasicMaterial({ map: this.dash.tex, toneMapped: false }),
      needle: new THREE.MeshBasicMaterial({ color: 0xff6a30, toneMapped: false }),
    };
  }

  private pb(p: THREE.Object3D, w: number, h: number, d: number, x: number, y: number, z: number, special: Partial<Record<Face, Rect>> = {}) {
    return uvBox(box(p, w, h, d, this.m.paint, x, y, z), PAINT.dusty, PAINT.plain, special);
  }

  private buildBody() {
    const B = this.body, m = this.m;
    // front: bumper, grille with three bars, sealed-beam headlamps, parking lamps
    box(B, 1.96, 0.18, 0.13, m.chrome, 0, 0.46, -2.585);
    box(B, 1.7, 0.12, 0.1, m.trim, 0, 0.32, -2.5);
    box(B, 1.84, 0.44, 0.06, m.trim, 0, 0.8, -2.5);
    for (const y of [0.58, 1.02]) box(B, 1.86, 0.03, 0.07, m.chrome, 0, y, -2.51);
    for (const y of [0.7, 0.8, 0.9]) box(B, 1.0, 0.024, 0.03, m.chrome, 0, y, -2.535);
    box(B, 0.024, 0.32, 0.03, m.chrome, 0, 0.8, -2.535);
    for (const s of [-1, 1]) {
      box(B, 0.3, 0.2, 0.03, m.chrome, s * 0.7, 0.85, -2.53);
      box(B, 0.25, 0.15, 0.02, m.head, s * 0.7, 0.85, -2.548);
      box(B, 0.25, 0.05, 0.02, m.park, s * 0.7, 0.69, -2.548);
      box(B, 0.12, 0.18, 0.3, m.chrome, s * 0.92, 0.46, -2.47);
    }
    // hood, a little higher at the back, and the cowl under the windshield
    const hood = this.pb(B, 1.88, 0.06, 1.68, 0, 1.06, -1.69); hood.rotation.x = -0.03;
    this.pb(B, 1.88, 0.06, 0.06, 0, 1.03, -2.53);
    this.pb(B, 1.88, 0.1, 0.14, 0, 1.12, -0.88);
    box(B, 1.56, 0.5, 1.6, m.trim, 0, 0.75, -1.7); // closes the engine bay
    for (const x of [-0.35, 0.25]) box(B, 0.55, 0.015, 0.03, m.trim, x, 1.178, -0.8).rotation.z = 0.06;
    // fenders around square wheel openings, the bed sides the same at the back
    for (const s of [-1, 1]) {
      this.pb(B, 0.22, 0.5, 0.26, s * 0.89, 0.8, -2.37);
      this.pb(B, 0.22, 0.22, 0.88, s * 0.89, 0.94, -1.8);
      this.pb(B, 0.22, 0.53, 0.51, s * 0.89, 0.785, -1.105);
      box(B, 0.04, 0.42, 0.88, m.trim, s * 0.79, 0.62, -1.8);
      this.pb(B, 0.1, 0.68, 0.28, s * 0.95, 0.86, 0.92);
      this.pb(B, 0.1, 0.37, 0.88, s * 0.95, 1.015, 1.5);
      this.pb(B, 0.1, 0.68, 0.61, s * 0.95, 0.86, 2.245);
      uvBox(box(B, 0.14, 0.03, 1.77, m.paint, s * 0.95, 1.215, 1.665), PAINT.plain, PAINT.plain);
      this.pb(B, 0.26, 0.2, 0.86, s * 0.77, 0.85, 1.5);              // wheel wells in the bed
      box(B, 0.1, 0.3, 0.03, m.tail, s * 0.93, 0.95, 2.565);
      // mirrors on short chrome arms, the glass facing back
      box(B, 0.17, 0.025, 0.025, m.chrome, s * 1.06, 1.25, -0.72);
      box(B, 0.05, 0.24, 0.16, m.trim, s * 1.15, 1.34, -0.72);
      box(B, 0.042, 0.21, 0.01, m.mirror, s * 1.15, 1.34, -0.637);
    }
    // bed floor, front wall, tailgate with the fleet number, rear step bumper
    uvBox(box(B, 1.76, 0.05, 1.72, m.paint, 0, 0.73, 1.66), PAINT.plain, PAINT.plain);
    this.pb(B, 1.96, 0.5, 0.06, 0, 0.95, 0.81);
    this.pb(B, 1.86, 0.56, 0.06, 0, 0.9, 2.53, { '+z': PAINT.tailgate });
    box(B, 1.96, 0.17, 0.12, m.chrome, 0, 0.45, 2.61);
    box(B, 0.1, 0.06, 0.14, m.trim, 0, 0.4, 2.7);
    // chassis and axles, so nothing shows through under the body
    box(B, 1.2, 0.22, 4.9, m.trim, 0, 0.4, 0);
    rod(B, new THREE.Vector3(-0.75, WHEEL_R, FRONT_AXLE), new THREE.Vector3(0.75, WHEEL_R, FRONT_AXLE), 0.05, m.trim, 6);
    rod(B, new THREE.Vector3(-0.75, WHEEL_R, REAR_AXLE), new THREE.Vector3(0.75, WHEEL_R, REAR_AXLE), 0.06, m.trim, 6);
    box(B, 0.3, 0.26, 0.3, m.trim, 0.1, WHEEL_R, REAR_AXLE);
    // cross-bed toolbox on the rails, a coil of cable, the radio whip on the bed corner
    box(B, 1.92, 0.27, 0.48, m.chrome, 0, 1.365, 1.08);
    box(B, 1.93, 0.012, 0.49, m.trim, 0, 1.44, 1.08);
    for (const x of [-0.6, 0.6]) box(B, 0.08, 0.05, 0.02, m.trim, x, 1.4, 0.835);
    for (const [r, y] of [[0.27, 0.8], [0.24, 0.86], [0.21, 0.91]]) {
      const t = new THREE.Mesh(new THREE.TorusGeometry(r, 0.04, 5, 18), m.trim);
      t.rotation.x = Math.PI / 2; t.position.set(0.32, y, 2.05); B.add(t);
    }
    cyl(B, 0.03, 0.04, 0.06, m.trim, 0.93, 1.25, 0.86, 6);
    rod(B, new THREE.Vector3(0.93, 1.27, 0.86), new THREE.Vector3(0.93, 2.6, 0.96), 0.006, m.chrome, 4);
  }

  private buildShell() {
    const S = this.shell, m = this.m;
    // cab sides carry the doors and their markings, on the outer face only
    this.pb(S, 0.1, 0.65, 1.57, -0.95, 0.845, -0.065, { '-x': PAINT.doorL });
    this.pb(S, 0.1, 0.65, 1.57, 0.95, 0.845, -0.065, { '+x': PAINT.doorR });
    box(S, 1.8, 0.06, 1.57, m.trim, 0, 0.55, -0.065);
    for (const s of [-1, 1]) {
      box(S, 0.02, 0.03, 0.12, m.chrome, s * 1.005, 1.1, 0.42);
      this.pb(S, 0.08, 0.72, 0.07, s * 0.9, 1.465, -0.63).rotation.x = 0.572;   // A-pillar
      this.pb(S, 0.08, 0.58, 0.1, s * 0.93, 1.455, 0.67);                          // B-pillar
      box(S, 0.02, 0.02, 1.18, m.chrome, s * 0.935, 1.745, 0.13);                  // drip rail
      // side glass: a quad that follows the slope of the windshield at the front
      const g = new THREE.PlaneGeometry(1, 1);
      const p = g.attributes.position as THREE.BufferAttribute;
      [[-0.44, 1.735], [0.62, 1.735], [-0.84, 1.165], [0.62, 1.165]].forEach(([z, y], i) => p.setXYZ(i, s * 0.935, y, z));
      if (s > 0) g.index!.array.reverse(); // as laid out it faces -X: turn it outwards on the right
      g.computeVertexNormals();
      S.add(new THREE.Mesh(g, m.glass));
    }
    const ws = box(S, 1.78, 0.72, 0.012, m.glass, 0, 1.465, -0.63); ws.rotation.x = 0.572;
    this.pb(S, 1.88, 0.06, 1.2, 0, 1.77, 0.13);                                   // roof
    this.pb(S, 1.92, 1.2, 0.06, 0, 1.17, 0.73);                                   // back of the cab
    box(S, 1.28, 0.4, 0.012, m.glass, 0, 1.47, 0.765);
    // amber beacon on the roof, switched off
    box(S, 0.22, 0.05, 0.22, m.trim, 0, 1.825, 0.25);
    cyl(S, 0.075, 0.09, 0.14, m.beacon, 0, 1.92, 0.25, 10);
  }

  private buildCab() {
    const C = this.cab, m = this.m;
    // Seen from the eye (1.48 m): the windshield's lower edge is 17 degrees down, the
    // instrument binnacle just under it, the top of the wheel rim at the bottom of the view.
    box(C, 1.76, 0.34, 0.42, m.dash, 0, 0.93, -0.66);                // dashboard
    box(C, 1.78, 0.04, 0.46, m.dash, 0, 1.12, -0.66);                // pad
    box(C, 0.6, 0.15, 0.2, m.dash, -0.42, 1.175, -0.5);              // binnacle, kept under the cowl line
    box(C, 0.62, 0.025, 0.09, m.dash, -0.42, 1.262, -0.405);         // its visor
    rod(C, new THREE.Vector3(-0.42, 0.86, -0.46), new THREE.Vector3(-0.42, 0.97, -0.24), 0.035, m.dash, 8);
    rod(C, new THREE.Vector3(-0.33, 0.97, -0.3), new THREE.Vector3(-0.17, 0.93, -0.24), 0.008, m.chrome, 5); // column shifter
    box(C, 1.74, 0.03, 1.5, m.dash, 0, 0.6, -0.1);                   // floor
    box(C, 1.72, 0.03, 1.18, m.liner, 0, 1.745, 0.13);               // roof liner
    box(C, 1.7, 0.07, 0.1, m.liner, 0, 1.72, -0.47);
    for (const x of [-0.42, 0.42]) box(C, 0.4, 0.012, 0.18, m.liner, x, 1.718, -0.36);
    // rear-view mirror: a dark plane
    rod(C, new THREE.Vector3(0, 1.74, -0.43), new THREE.Vector3(0, 1.68, -0.41), 0.008, m.dash, 4);
    box(C, 0.23, 0.07, 0.025, m.dash, 0, 1.655, -0.405);
    plane(C, 0.21, 0.054, m.mirror, 0, 1.655, -0.391);
    for (const s of [-1, 1]) {
      box(C, 0.08, 0.72, 0.07, m.dash, s * 0.83, 1.465, -0.62).rotation.x = 0.572; // A-pillar
      box(C, 0.06, 0.61, 1.4, m.vinyl, s * 0.87, 0.875, -0.1);                    // door panel
      box(C, 0.1, 0.04, 1.42, m.dash, s * 0.865, 1.19, -0.1);
      box(C, 0.08, 0.06, 0.32, m.dash, s * 0.82, 0.96, 0.2);
      box(C, 0.02, 0.03, 0.1, m.chrome, s * 0.835, 1.04, -0.2);
      box(C, 0.08, 0.58, 0.1, m.liner, s * 0.88, 1.455, 0.66);
      box(C, 0.24, 0.42, 0.04, m.liner, s * 0.75, 1.48, 0.7);          // beside the rear window
    }
    box(C, 1.74, 0.7, 0.04, m.vinyl, 0, 0.92, 0.7);
    box(C, 1.74, 0.08, 0.04, m.liner, 0, 1.73, 0.7);
    // bench seat
    box(C, 1.66, 0.14, 0.5, m.vinyl, 0, 0.75, 0.3);
    box(C, 1.66, 0.58, 0.12, m.vinyl, 0, 1.11, 0.6).rotation.x = 0.12;
    // instruments: cluster and radio share one canvas; needles are three instances
    const cluster = new THREE.Group();
    cluster.position.set(-0.42, 1.165, -0.378); cluster.rotation.x = -0.15;
    uvPlane(plane(cluster, 0.56, 0.175, m.gauge, 0, 0, 0), DASH.cluster);
    C.add(cluster);
    uvPlane(plane(C, 0.34, 0.064, m.gauge, 0.12, 1.055, -0.428), DASH.stack);
    const ng = new THREE.BoxGeometry(0.0035, 1, 0.002).translate(0, 0.42, 0);
    this.needles = new THREE.InstancedMesh(ng, m.needle, 3);
    this.needles.frustumCulled = false;
    noMerge(this.needles); cluster.add(this.needles);
    // steering wheel: tilted towards the driver, turns about its own axis
    const pivot = new THREE.Group();
    pivot.position.set(-0.42, 1.0, -0.2); pivot.rotation.x = -0.5;
    const parts: THREE.BufferGeometry[] = [new THREE.TorusGeometry(0.19, 0.016, 6, 28), new THREE.CylinderGeometry(0.05, 0.055, 0.045, 10).rotateX(Math.PI / 2)];
    for (const s of [-1, 1]) parts.push(new THREE.BoxGeometry(0.15, 0.03, 0.012).translate(s * 0.105, -0.02, 0));
    this.wheelSpin.add(new THREE.Mesh(mergeGeometries(parts)!, m.wheelRim));
    pivot.add(this.wheelSpin);
    noMerge(pivot); C.add(pivot);
    // the instruments' glow, in front of the binnacle
    this.cabFlood.pos[0].set(0, -999, 0, 0); this.cabFlood.count = 1;
  }

  /** Interior on, view-blocking shell off. DriveController calls this while it drives. */
  setDriving(on: boolean) {
    this.driving = on;
    this.cab.visible = on;
    this.shell.visible = !on;
  }
  private autoView(cam: THREE.Camera) {
    if (!this.driving) return;
    this.eyeW.copy(this.driverEye).applyMatrix4(this.group.matrixWorld);
    if (this.camW.setFromMatrixPosition(cam.matrixWorld).distanceTo(this.eyeW) > 0.9) this.setDriving(false);
  }

  /** Dashboard values. Needles ease in update(); the clock canvas redraws at most 10 times a second. */
  setDash(o: { mph: number; rpm: number; clock: string; fuel: number }) {
    this.gauge.tSpeed = THREE.MathUtils.clamp(o.mph / 85, 0, 1);
    this.gauge.tFuel = THREE.MathUtils.clamp(o.fuel, 0, 1);
    this.gauge.tTemp = o.rpm > 100 ? 0.5 + Math.min(0.06, o.rpm / 60000) : 0.15;
    this.clockWanted = o.clock;
    this.flushClock();
  }
  private flushClock() {
    const now = performance.now();
    if (this.clockWanted === this.clockShown || now - this.clockAt < 100) return;
    this.clockShown = this.clockWanted; this.clockAt = now;
    this.dash.drawClock(this.clockShown);
  }
  /** -1 (full left) to 1 (full right). Turns the steering wheel. */
  setSteer(v: number) { this.wheelSpin.rotation.z = -THREE.MathUtils.clamp(v, -1, 1) * 2.4; }
  setHeadlights(on: boolean) { this.headOn = on; this.applyLights(); }
  /** 0..1: headlights, tail lights and dash light together (0 is dark). */
  setLightLevel(level: number) { this.level = THREE.MathUtils.clamp(level, 0, 1); this.applyLights(); }
  setBrake(on: boolean) { if (on !== this.brakeOn) { this.brakeOn = on; this.applyLights(); } }
  /** The headlights' colour (white when null): the lamps, their glow and the pools on the ground. */
  setHeadColor(c: THREE.Color | null) { this.headColor.copy(c ?? HEAD); this.applyLights(); }
  /** The instrument lights on their own, 0..1 (null: they follow setLightLevel). */
  setDashLevel(k: number | null) { this.dashLevel = k; this.applyLights(); }
  /** Use these slots of a flood set as headlight pools on the ground (moved in update()). */
  headlightFloods(set: FloodSet, slots: number[]) { this.flood = { set, slots: slots.slice(0, FLOODS.length) }; this.placeFloods(); }

  private applyLights() {
    const L = this.level, head = this.headOn ? L : 0, D = this.dashLevel ?? L;
    const tail = L * Math.max(this.headOn ? 0.5 : 0, this.brakeOn ? 1.5 : 0);
    const m = this.m;
    lens(m.head, 0x3e3e3a, this.headColor, 1.6 * head);
    lens(m.park, 0x4a3a18, AMBER, 0.8 * head);
    lens(m.tail, 0x420a08, TAIL, tail);
    m.gauge.color.setScalar(0.03 + 0.85 * D);
    m.needle.color.setRGB(1, 0.42, 0.19).multiplyScalar(0.04 + 0.96 * D);
    const col = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    const put = (ids: number[], c: THREE.Color, k: number) => ids.forEach((i) => col.setXYZ(i, c.r * k, c.g * k, c.b * k));
    put(this.gi.head, this.headColor, 0.9 * head); put(this.gi.park, AMBER, 0.3 * head); put(this.gi.tail, TAIL, 0.4 * tail);
    col.needsUpdate = true;
    this.placeFloods();
  }

  private placeFloods() {
    this.group.updateWorldMatrix(true, false);
    const c = this.cabFlood.pos[0].set(-0.42, 1.12, -0.28, 1).applyMatrix4(this.group.matrixWorld);
    const D = this.dashLevel ?? this.level;
    c.w = D > 0.01 ? 1.6 : 0;
    this.cabFlood.col[0].setRGB(0.6, 0.78, 0.68).multiplyScalar(D);
    if (!this.flood) return;
    const { set, slots } = this.flood;
    const on = this.headOn && this.level > 0.01;
    slots.forEach((i, k) => {
      const f = FLOODS[k];
      const p = set.pos[i].set(f.p.x, f.p.y, f.p.z, 1).applyMatrix4(this.group.matrixWorld);
      p.w = on ? f.w : 0;
      // dim by colour, not by intensity: the pools fade instead of shrinking
      set.col[i].copy(this.headColor).multiplyScalar(this.level);
    });
  }

  /** Once per frame while the truck moves: wheels, needles, headlight pools. */
  update(dt: number, speed: number, wheelAngle = 0) {
    this.spin -= speed / WHEEL_R * dt;
    this.wheelAngle = wheelAngle;
    [[-TRACK, FRONT_AXLE, 1], [TRACK, FRONT_AXLE, 1], [-TRACK, REAR_AXLE, 0], [TRACK, REAR_AXLE, 0]].forEach(([x, z, front], i) => {
      _q.setFromEuler(_e.set(this.spin, front ? -this.wheelAngle : 0, 0, 'YXZ'));
      this.wheels.setMatrixAt(i, _m.compose(_p.set(x, WHEEL_R, z), _q, _s.set(1, 1, 1)));
    });
    this.wheels.instanceMatrix.needsUpdate = true;
    // needles have a little weight to them
    const g = this.gauge, k = 1 - Math.exp(-dt * 6);
    g.speed += (g.tSpeed - g.speed) * k; g.fuel += (g.tFuel - g.fuel) * k * 0.3; g.temp += (g.tTemp - g.temp) * dt * 0.02;
    const W = 0.56, H = 0.175, needle = (i: number, d: typeof DIALS.speed, f: number, len: number) => {
      const a = (d.from + (d.to - d.from) * f) * Math.PI / 180;
      _q.setFromAxisAngle(_z, -a);
      this.needles.setMatrixAt(i, _m.compose(_p.set((d.x / 512 - 0.5) * W, (0.5 - d.y / 160) * H, 0.003), _q, _s.set(1, len, 1)));
    };
    needle(0, DIALS.speed, g.speed, 0.068); needle(1, DIALS.fuel, g.fuel, 0.038); needle(2, DIALS.temp, g.temp, 0.038);
    this.needles.instanceMatrix.needsUpdate = true;
    this.flushClock();
    this.placeFloods();
  }

  /** Frees what the truck created (geometry, materials, canvas textures). */
  dispose() {
    this.group.traverse((o) => { const mesh = o as THREE.Mesh; if (mesh.geometry) mesh.geometry.dispose(); });
    for (const mat of Object.values(this.m)) { (mat as THREE.MeshBasicMaterial).map?.dispose(); mat.dispose(); }
    const gm = this.glow.points.material as THREE.ShaderMaterial;
    gm.uniforms.uMap.value.dispose(); gm.dispose();
    if (this.flood) for (const i of this.flood.slots) this.flood.set.pos[i].w = 0;
  }
}
