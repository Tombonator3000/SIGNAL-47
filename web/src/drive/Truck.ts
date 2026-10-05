import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, plane, noMerge, floodlit, floodSet, type FloodSet } from '../world/kit';
import { GlowPoints } from '../world/glow';
import {
  DASH, DIALS, DECAL, CAB, dashAtlas, paintTex, decalAtlas, dashGrainTex, vinylTex, linerTex, matTex, cabAtlas, windshieldTex, type Rect,
} from './truckTextures';

// SARO 07, the observatory's service truck: a full-size single-cab pickup of the early
// eighties, white with faded paint, square headlamps, a long bed with a toolbox and a coil
// of survey cable. 5.5 x 2.0 x 1.8 m. Origin on the ground at the centre of the truck,
// forward is -Z, the driver sits left (-X).
//
//   group   everything. Inside it:
//   body    stays visible while driving: hood and fenders, grille, bumpers, bed, mirrors,
//           wheels, the frame underneath.
//   shell   the cab from outside: doors, pillars, roof, glass. Hidden while driving, since
//           it would sit in front of the driver's eye. The hood stays: it gives the road its speed.
//   cab     the interior. Shown only while driving.
//
// The panels are side profiles extruded across the truck with rounded edges (`side`), so
// the wheel arches are round and the corners soft. Tiled textures are projected on after
// the parts are placed (`merge`); the road dust low on the sides is in the vertex colours.
// Static parts are merged per material: about 17 draw calls outside, 27 while driving.

export const WHEELBASE = 3.3;
const FRONT_AXLE = -1.8, REAR_AXLE = FRONT_AXLE + WHEELBASE, WHEEL_R = 0.37, TRACK = 0.84;
const WHEELS: [number, number][] = [[-TRACK, FRONT_AXLE], [TRACK, FRONT_AXLE], [-TRACK, REAR_AXLE], [TRACK, REAR_AXLE]];
// The fake headlight floods (kit.floodlit): truck-local position and intensity. Three
// pools in a row make a beam that widens and fades down the road.
const FLOODS = [
  { p: new THREE.Vector3(0, 0.9, -7.8), w: 5.5 },
  { p: new THREE.Vector3(0, 1.2, -14.6), w: 10 },
  { p: new THREE.Vector3(0, 1.8, -28.6), w: 14 },
];
const HEAD = new THREE.Color(1.0, 0.9, 0.72), TAIL = new THREE.Color(1, 0.05, 0.03), AMBER = new THREE.Color(1, 0.55, 0.12);
const _q = new THREE.Quaternion(), _e = new THREE.Euler(), _m = new THREE.Matrix4(), _p = new THREE.Vector3();
const _s = new THREE.Vector3(), _z = new THREE.Vector3(0, 0, 1), _c = new THREE.Color();
const _a = new THREE.Vector3(), _b = new THREE.Vector3(), _n = new THREE.Vector3();
// an unlit lens: its glass colour when off, plus the lamp colour times k
const lens = (m: THREE.MeshBasicMaterial, off: number, c: THREE.Color, k: number) => m.color.setHex(off).add(_c.copy(c).multiplyScalar(k));

// ---------- shapes ----------
type P2 = [number, number];
/** A closed outline from points; [u, y] pairs, or a function that draws on the shape. */
function outline(pts: P2[]) { const s = new THREE.Shape(); s.moveTo(pts[0][0], pts[0][1]); for (const p of pts.slice(1)) s.lineTo(p[0], p[1]); return s; }
function hole(pts: P2[]) { const h = new THREE.Path(); h.moveTo(pts[0][0], pts[0][1]); for (const p of pts.slice(1)) h.lineTo(p[0], p[1]); return h; }
/** A rounded rectangle in the shape's own plane. */
function rrect(x0: number, y0: number, x1: number, y1: number, r: number) {
  const s = new THREE.Shape();
  s.moveTo(x0 + r, y0); s.lineTo(x1 - r, y0); s.quadraticCurveTo(x1, y0, x1, y0 + r);
  s.lineTo(x1, y1 - r); s.quadraticCurveTo(x1, y1, x1 - r, y1); s.lineTo(x0 + r, y1);
  s.quadraticCurveTo(x0, y1, x0, y1 - r); s.lineTo(x0, y0 + r); s.quadraticCurveTo(x0, y0, x0 + r, y0);
  return s;
}
function extrude(shape: THREE.Shape, depth: number, bevel: number) {
  const b = Math.min(bevel, depth / 2 - 0.002);
  const g = new THREE.ExtrudeGeometry(shape, {
    depth: Math.max(0.002, depth - 2 * Math.max(0, b)), bevelEnabled: b > 0, bevelThickness: b, bevelSize: b, bevelOffset: -b,
    bevelSegments: 2, curveSegments: 10,
  });
  g.translate(0, 0, -depth / 2 + Math.max(0, b));
  return g;
}
/** A side profile (u = -z forward, y up) extruded across the truck, centred on x, w wide. */
function side(shape: THREE.Shape, w: number, x = 0, bevel = 0.02) {
  return extrude(shape, w, bevel).rotateY(Math.PI / 2).translate(x, 0, 0);
}
/** A face shape (x, y) extruded along z, centred on z. */
function face(shape: THREE.Shape, d: number, z = 0, bevel = 0.01) { return extrude(shape, d, bevel).translate(0, 0, z); }
/** A plan shape (x, -z) extruded up, centred on y. */
function slabY(shape: THREE.Shape, t: number, y = 0, bevel = 0.006) { return extrude(shape, t, bevel).rotateX(-Math.PI / 2).translate(0, y, 0); }
/** An arch over a wheel: the solid part of a profile above a circle, for the inner wheel houses. */
function arch(cu: number, cy: number, r0: number, r1: number, y0: number) {
  const s = new THREE.Shape();
  const a0 = Math.asin(Math.min(1, (y0 - cy) / r1)), b0 = Math.asin(Math.min(1, (y0 - cy) / r0));
  s.absarc(cu, cy, r1, a0, Math.PI - a0, false);
  s.absarc(cu, cy, r0, Math.PI - b0, b0, true);
  return s;
}

const mesh = (parent: THREE.Object3D, g: THREE.BufferGeometry, mat: THREE.Material, tint?: number) => {
  const m = new THREE.Mesh(g, mat);
  if (tint !== undefined) m.userData.tint = tint;
  parent.add(m); return m;
};
/** A plane showing one rectangle of an atlas. */
function decal(parent: THREE.Object3D, w: number, h: number, mat: THREE.Material, r: Rect, x: number, y: number, z: number, ry = 0, rx = 0) {
  const m = plane(parent, w, h, mat, x, y, z, ry, rx);
  const uv = m.geometry.attributes.uv as THREE.BufferAttribute;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, r[0] + uv.getX(i) * (r[2] - r[0]), r[1] + uv.getY(i) * (r[3] - r[1]));
  return m;
}

// ---------- merging, with projected texture coordinates and dust ----------
// Materials say what they want in userData: boxUV (metres per texture repeat, projected
// on the face's main axis), dust (the paint's vertex colours), tinted (a colour per part).
function boxUV(g: THREE.BufferGeometry, scale: number) {
  const p = g.attributes.position as THREE.BufferAttribute;
  const uv = new Float32Array(p.count * 2);
  for (let i = 0; i < p.count; i += 3) {
    _a.fromBufferAttribute(p, i + 1).sub(_p.fromBufferAttribute(p, i));
    _b.fromBufferAttribute(p, i + 2).sub(_p);
    _n.crossVectors(_a, _b);
    const ax = Math.abs(_n.x), ay = Math.abs(_n.y), az = Math.abs(_n.z);
    for (let k = 0; k < 3; k++) {
      _p.fromBufferAttribute(p, i + k);
      const [u, v] = ax >= ay && ax >= az ? [_p.z, _p.y] : ay >= az ? [_p.x, _p.z] : [_p.x, _p.y];
      uv[(i + k) * 2] = u / scale; uv[(i + k) * 2 + 1] = v / scale;
    }
  }
  g.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
}
const DUST = new THREE.Color(0.66, 0.55, 0.42), UNDER = 0.16;
function dustColors(g: THREE.BufferGeometry) {
  const p = g.attributes.position as THREE.BufferAttribute, nrm = g.attributes.normal as THREE.BufferAttribute;
  const col = new Float32Array(p.count * 3);
  for (let i = 0; i < p.count; i++) {
    const y = p.getY(i), ny = nrm.getY(i);
    const k = THREE.MathUtils.smoothstep(y, 0.45, 0.82);
    _c.copy(DUST).lerp(new THREE.Color(1, 1, 1), k);
    if (ny < -0.45 && y < 1.0) _c.setScalar(UNDER);          // undersides and the inside of the wheel arches
    col[i * 3] = _c.r; col[i * 3 + 1] = _c.g; col[i * 3 + 2] = _c.b;
  }
  g.setAttribute('color', new THREE.BufferAttribute(col, 3));
}
function merge(root: THREE.Object3D) {
  root.updateMatrixWorld(true);
  const inv = new THREE.Matrix4().copy(root.matrixWorld).invert();
  const buckets = new Map<THREE.Material, THREE.BufferGeometry[]>();
  const victims: THREE.Mesh[] = [];
  const walk = (o: THREE.Object3D) => {
    for (const c of [...o.children]) {
      if (c.userData.noMerge) continue;
      const m = c as THREE.Mesh;
      if (m.isMesh && !(m as any).isInstancedMesh && !Array.isArray(m.material)) {
        const mat = m.material as THREE.Material, ud = mat.userData;
        const g = m.geometry.index ? m.geometry.toNonIndexed() : m.geometry.clone();
        for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'uv'].includes(k)) g.deleteAttribute(k);
        g.applyMatrix4(new THREE.Matrix4().multiplyMatrices(inv, m.matrixWorld));
        if (ud.boxUV) boxUV(g, ud.boxUV);
        if (!g.attributes.uv) g.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(g.attributes.position.count * 2), 2));
        if (ud.dust) dustColors(g);
        if (ud.tinted) {
          _c.set(m.userData.tint ?? 0xffffff);
          const n = g.attributes.position.count, col = new Float32Array(n * 3);
          for (let i = 0; i < n; i++) { col[i * 3] = _c.r; col[i * 3 + 1] = _c.g; col[i * 3 + 2] = _c.b; }
          g.setAttribute('color', new THREE.BufferAttribute(col, 3));
        }
        if (!buckets.has(mat)) buckets.set(mat, []);
        buckets.get(mat)!.push(g);
        victims.push(m);
      }
      walk(c);
    }
  };
  walk(root);
  for (const v of victims) { v.parent?.remove(v); v.geometry.dispose(); }
  for (const [mat, geos] of buckets) {
    const merged = mergeGeometries(geos, false);
    if (!merged) continue;
    const m = new THREE.Mesh(merged, mat);
    m.matrixAutoUpdate = false;
    root.add(m);
  }
}

// ---------- the wheel ----------
// One wheel for all four instances: a lathe-turned tyre with round shoulders, a white steel
// rim with five slots (so the spin shows) and a small chrome cap, a dark disc behind. Axis
// along X, the rim's face towards +X. Colours are per vertex, so it is one draw call.
function wheelGeometry() {
  const tint = (g: THREE.BufferGeometry, v: number | [number, number, number]) => {
    const c = typeof v === 'number' ? [v, v, v] : v, n = g.attributes.position.count;
    const a = new Float32Array(n * 3); for (let i = 0; i < n; i++) a.set(c, i * 3);
    g.setAttribute('color', new THREE.BufferAttribute(a, 3));
    return g.index ? g.toNonIndexed() : g;
  };
  const V = (pts: P2[]) => pts.map(([r, y]) => new THREE.Vector2(r, y));
  const tyre = new THREE.LatheGeometry(V([[0.214, -0.098], [0.25, -0.114], [0.31, -0.122], [0.352, -0.114], [0.368, -0.096], [0.372, -0.06],
    [0.372, 0.06], [0.368, 0.096], [0.352, 0.114], [0.31, 0.122], [0.25, 0.114], [0.214, 0.098]]), 28);
  const cap = new THREE.LatheGeometry(V([[0, -0.128], [0.05, -0.124], [0.085, -0.112], [0.104, -0.096]]), 20);
  const rim = new THREE.LatheGeometry(V([[0.104, -0.096], [0.13, -0.09], [0.17, -0.078], [0.2, -0.084], [0.214, -0.098]]), 24);
  const back = new THREE.LatheGeometry(V([[0.214, 0.09], [0, 0.09]]), 16);
  const parts = [tint(tyre, 0.04), tint(cap, 0.82), tint(rim, [0.78, 0.76, 0.7]), tint(back, 0.03)];
  for (let k = 0; k < 5; k++) {
    const a = k / 5 * Math.PI * 2;
    parts.push(tint(new THREE.BoxGeometry(0.045, 0.012, 0.03).rotateY(-a).translate(Math.cos(a) * 0.15, -0.081, Math.sin(a) * 0.15), 0.02));
  }
  // lathe space: axis Y, the rim on -Y. Turned so the axis is X and the rim faces +X.
  return mergeGeometries(parts.map((g) => { g.deleteAttribute('uv'); return g; }))!.rotateZ(Math.PI / 2);
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
  private keys = new THREE.Group();
  private needles!: THREE.InstancedMesh;
  private glow = new GlowPoints();
  private gi = { head: [] as number[], park: [] as number[], tail: [] as number[] };
  private level = 1; private headOn = false; private brakeOn = false;
  private headColor = new THREE.Color().copy(HEAD); private dashLevel: number | null = null;
  private spin = 0; private wheelAngle = 0;
  private drops = [0, 0, 0, 0];
  private swing = { a: 0, v: 0, last: 0 };
  private flood: { set: FloodSet; slots: number[] } | null = null;
  private dash = dashAtlas();
  private clockShown = ''; private clockWanted = ''; private clockAt = -1;
  private gauge = { speed: 0, fuel: 0.6, temp: 0.15, tSpeed: 0, tFuel: 0.6, tTemp: 0.5 };
  private eyeW = new THREE.Vector3(); private camW = new THREE.Vector3();
  // The cab's own light: the instruments, and a little sky through the glass. A flood set of its own.
  private cabFlood = floodSet(2, 'cab', 0.5);

  /** flood: light the exterior with a set of fake floods (leave out the headlight slots). */
  constructor(o: { flood?: FloodSet } = {}) {
    this.m = this.materials(o.flood);
    this.group.name = 'SARO 07';
    this.group.add(this.body, this.shell, this.cab);
    this.buildBody(); this.buildUnder(); this.buildShell(); this.buildCab();
    merge(this.body); merge(this.shell); merge(this.cab);
    this.wheels = new THREE.InstancedMesh(wheelGeometry(), this.m.wheel, 4);
    this.wheels.frustumCulled = false; // the instances move; the truck is small anyway
    // Whoever puts the camera outside the cab gets the whole truck back on the next frame,
    // even if nobody calls setDriving(false) (a cut to the parked truck, for example).
    this.wheels.onBeforeRender = (_r, _s, cam) => this.autoView(cam);
    this.body.add(this.wheels);
    for (const s of [-1, 1]) {
      for (const y of [0.931, 0.756]) this.gi.head.push(this.glow.add(s * 0.657, y, -2.64, 0.55, 0));
      this.gi.park.push(this.glow.add(s * 0.66, 0.6, -2.64, 0.22, 0));
      this.gi.tail.push(this.glow.add(s * 0.93, 0.98, 2.64, 0.42, 0));
    }
    this.body.add(this.glow.build());
    this.setDriving(false);
    this.update(0, 0);
    this.applyLights();
  }

  private materials(flood?: FloodSet) {
    const lit = <T extends THREE.MeshStandardMaterial>(m: T) => flood ? floodlit(m, 0.02, flood) : m;
    const std = (o: THREE.MeshStandardMaterialParameters, ud: Record<string, unknown> = {}) => { const m = lit(new THREE.MeshStandardMaterial(o)); Object.assign(m.userData, ud); return m; };
    const basic = (c: number) => new THREE.MeshBasicMaterial({ color: c });
    const cab = <T extends THREE.MeshStandardMaterial>(m: T, ud: Record<string, unknown> = {}) => { Object.assign(m.userData, ud); return floodlit(m, 5, this.cabFlood); };
    const atlas = cabAtlas();
    return {
      paint: std({ map: paintTex(), vertexColors: true, roughness: 0.5, metalness: 0.05 }, { boxUV: 1.1, dust: true }),
      chrome: std({ color: 0xc8cbcc, roughness: 0.26, metalness: 0.62 }),
      trim: std({ color: 0x1a1b1d, roughness: 0.75 }),
      under: std({ color: 0x1b1a19, roughness: 1 }),
      glass: std({ color: 0x0b0f14, roughness: 0.1, metalness: 0.45, side: THREE.DoubleSide }),
      decal: std({ map: decalAtlas(), transparent: true, depthWrite: false, roughness: 0.6, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -2 }),
      wheel: std({ vertexColors: true, roughness: 0.82 }),
      head: basic(0x2c2c2a), park: basic(0x3a2a10), tail: basic(0x2a0606), beacon: basic(0x4a3010), rev: basic(0x8a8a86),
      mirror: basic(0x131b28),
      // the cab is lit by its instruments and the sky, not by the headlights
      dash: cab(new THREE.MeshStandardMaterial({ map: dashGrainTex(), color: 0xc8beb2, roughness: 0.75 }), { boxUV: 0.35 }),
      vinyl: cab(new THREE.MeshStandardMaterial({ map: vinylTex(), roughness: 0.55 }), { boxUV: 0.32 }),
      doorCard: cab(new THREE.MeshStandardMaterial({ map: vinylTex(), color: 0x9a8670, roughness: 0.6 }), { boxUV: 0.6 }),
      liner: cab(new THREE.MeshStandardMaterial({ map: linerTex(), roughness: 1 }), { boxUV: 0.12 }),
      mat: cab(new THREE.MeshStandardMaterial({ map: matTex(), roughness: 0.95 }), { boxUV: 0.25 }),
      cabTrim: cab(new THREE.MeshStandardMaterial({ color: 0x232220, roughness: 0.6 })),
      cabChrome: cab(new THREE.MeshStandardMaterial({ color: 0xb8bbbc, roughness: 0.3, metalness: 0.6 })),
      cabDecal: cab(new THREE.MeshStandardMaterial({ map: atlas, roughness: 0.7 })),
      prop: cab(new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.6 }), { tinted: true }),
      wheelRim: cab(new THREE.MeshStandardMaterial({ color: 0x2c2926, roughness: 0.45 })),
      cabMirror: new THREE.MeshBasicMaterial({ map: atlas }),
      tint: new THREE.MeshBasicMaterial({ map: windshieldTex(), transparent: true, depthWrite: false }),
      gauge: new THREE.MeshBasicMaterial({ map: this.dash.tex, toneMapped: false }),
      needle: new THREE.MeshBasicMaterial({ color: 0xff6a30, toneMapped: false }),
    };
  }

  // ---------- outside, always seen ----------
  private buildBody() {
    const B = this.body, m = this.m;
    // the front clip: fenders and hood in one profile, the front wheel arch through it
    const front = new THREE.Shape();
    front.moveTo(0.86, 0.5); front.lineTo(1.348, 0.5);
    front.absarc(1.8, 0.37, 0.47, Math.PI - 0.2795, 0.2795, true);
    front.lineTo(2.44, 0.5); front.quadraticCurveTo(2.6, 0.5, 2.62, 0.62);
    front.lineTo(2.62, 1.04); front.quadraticCurveTo(2.62, 1.1, 2.56, 1.105);
    front.lineTo(1.4, 1.12); front.lineTo(0.86, 1.13); front.lineTo(0.86, 0.5);
    mesh(B, side(front, 1.98, 0, 0.035), m.paint);
    // the hood's edges and its front, as gaps in the paint; the cowl vent and the wipers
    for (const s of [-1, 1]) box(B, 0.008, 0.006, 1.64, m.trim, s * 0.8, 1.123, -1.74).rotation.x = 0.012;
    box(B, 1.6, 0.006, 0.008, m.trim, 0, 1.106, -2.52);
    box(B, 1.7, 0.012, 0.09, m.trim, 0, 1.133, -0.93);
    for (const x of [-0.42, 0.3]) {
      box(B, 0.58, 0.012, 0.014, m.trim, x, 1.15, -0.9).rotation.z = 0.05;
      rod(B, new THREE.Vector3(x + 0.26, 1.135, -0.97), new THREE.Vector3(x - 0.05, 1.15, -0.905), 0.006, m.trim, 4);
    }
    // the inner wheel houses, dark, so nothing shows through the arches
    for (const u of [-FRONT_AXLE]) box(B, 1.4, 0.44, 0.84, m.under, 0, 0.62, -u);
    // grille and the four square headlamps, the parking lamps under them
    decal(B, 1.8, 0.4, m.decal, DECAL.grille, 0, 0.84, -2.623, Math.PI);
    for (const s of [-1, 1]) {
      for (const y of [0.931, 0.756]) box(B, 0.33, 0.135, 0.02, m.head, s * 0.657, y, -2.628);
      box(B, 0.3, 0.05, 0.02, m.park, s * 0.66, 0.6, -2.624);
      box(B, 0.03, 0.045, 0.085, m.park, s * 0.992, 0.84, -2.36);               // side markers
    }
    // the front bumper, the plate, two tow hooks
    mesh(B, side(outline([[2.6, 0.37], [2.74, 0.38], [2.775, 0.43], [2.775, 0.5], [2.74, 0.555], [2.6, 0.565]]), 2.04, 0, 0.03), m.chrome);
    decal(B, 0.3, 0.15, m.decal, DECAL.plateF, 0, 0.465, -2.777, Math.PI);
    for (const s of [-1, 1]) box(B, 0.05, 0.05, 0.1, m.chrome, s * 0.5, 0.33, -2.7);
    // the side moulding: a thin bright strip along the body, front and bed (the cab has its own)
    for (const s of [-1, 1]) {
      box(B, 0.012, 0.022, 1.66, m.chrome, s * 0.996, 0.93, -1.73);
      box(B, 0.012, 0.022, 1.76, m.chrome, s * 0.996, 0.93, 1.71);
    }
    // the bed: sides with the rear arch, caps on the rails, floor and ribs, the tubs inside
    const bedSide = new THREE.Shape();
    bedSide.moveTo(-0.82, 0.55); bedSide.lineTo(-1.066, 0.55);
    bedSide.absarc(-1.5, 0.37, 0.47, 0.3929, Math.PI - 0.3929, false);
    bedSide.lineTo(-2.58, 0.55); bedSide.quadraticCurveTo(-2.62, 0.55, -2.62, 0.6);
    bedSide.lineTo(-2.62, 1.2); bedSide.lineTo(-0.82, 1.2); bedSide.lineTo(-0.82, 0.55);
    for (const s of [-1, 1]) {
      mesh(B, side(bedSide, 0.1, s * 0.94, 0.025), m.paint);
      mesh(B, side(rrect(-2.62, 1.19, -0.82, 1.232, 0.012), 0.16, s * 0.92, 0.015), m.paint);
      mesh(B, side(arch(-1.5, 0.74, 0.001, 0.47, 0.74), 0.26, s * 0.76, 0.02), m.paint);
      box(B, 0.25, 0.32, 0.86, m.under, s * 0.76, 0.6, REAR_AXLE);
      box(B, 0.03, 0.045, 0.085, m.tail, s * 0.992, 0.84, 2.45);                 // rear side markers
      // tail lamps on the corner posts, a white reversing lamp low in each
      box(B, 0.13, 0.36, 0.02, m.chrome, s * 0.93, 0.93, 2.623);
      box(B, 0.11, 0.25, 0.02, m.tail, s * 0.93, 0.98, 2.632);
      box(B, 0.11, 0.07, 0.02, m.rev, s * 0.93, 0.815, 2.632);
    }
    box(B, 1.76, 0.045, 1.74, m.paint, 0, 0.722, 1.69);
    for (let i = -3; i <= 3; i++) box(B, 0.035, 0.014, 1.72, m.paint, i * 0.24, 0.75, 1.69);
    box(B, 1.98, 0.5, 0.06, m.paint, 0, 0.95, 0.83);
    // the tailgate with the fleet number, its handle; the step bumper and the plate
    mesh(B, side(rrect(-2.62, 0.6, -2.565, 1.185, 0.01), 1.84, 0, 0.015), m.paint);
    decal(B, 1.84, 0.46, m.decal, DECAL.tailgate, 0, 0.9, 2.6215);
    box(B, 0.2, 0.05, 0.02, m.chrome, 0, 1.13, 2.622);
    mesh(B, side(outline([[-2.6, 0.38], [-2.79, 0.38], [-2.8, 0.42], [-2.8, 0.53], [-2.79, 0.56], [-2.6, 0.56]]), 2.0, 0, 0.03), m.chrome);
    box(B, 0.9, 0.012, 0.12, m.trim, 0, 0.562, 2.7);
    decal(B, 0.3, 0.15, m.decal, DECAL.plateR, 0, 0.465, 2.802);
    // mud flaps behind the rear wheels
    for (const s of [-1, 1]) box(B, 0.28, 0.3, 0.012, m.trim, s * 0.84, 0.34, 2.02);
    // west coast mirrors on two arms each, the glass facing back
    for (const s of [-1, 1]) {
      for (const y of [1.06, 1.32]) rod(B, new THREE.Vector3(s * 0.99, y, -0.66), new THREE.Vector3(s * 1.21, y + 0.02, -0.64), 0.011, m.chrome, 6);
      mesh(B, side(rrect(0.6, 1.04, 0.69, 1.4, 0.02), 0.15, s * 1.25, 0.012), m.trim);
      box(B, 0.13, 0.33, 0.005, m.mirror, s * 1.25, 1.22, -0.598);
    }
    // a cross-bed toolbox on the rails, a coil of survey cable, the radio whip on the corner
    mesh(B, side(rrect(-1.32, 1.23, -0.84, 1.5, 0.03), 1.94, 0, 0.02), m.chrome);
    box(B, 1.9, 0.006, 0.006, m.trim, 0, 1.47, 0.86);
    for (const x of [-0.6, 0.6]) box(B, 0.08, 0.05, 0.02, m.trim, x, 1.42, 0.835);
    for (const [r, y] of [[0.27, 0.8], [0.24, 0.86], [0.21, 0.91]]) {
      const t = new THREE.Mesh(new THREE.TorusGeometry(r, 0.035, 6, 20), m.trim);
      t.rotation.x = Math.PI / 2; t.position.set(0.32, y, 2.05); B.add(t);
    }
    cyl(B, 0.03, 0.04, 0.06, m.trim, 0.93, 1.26, 0.86, 6);
    rod(B, new THREE.Vector3(0.93, 1.28, 0.86), new THREE.Vector3(0.93, 2.6, 0.96), 0.006, m.chrome, 4);
  }

  // ---------- underneath: frame, axles, springs, tank, exhaust, the spare ----------
  private buildUnder() {
    const B = this.body, u = this.m.under;
    for (const s of [-1, 1]) {
      box(B, 0.08, 0.18, 4.9, u, s * 0.46, 0.43, 0.05);
      box(B, 0.07, 0.05, 1.3, u, s * 0.46, 0.33, REAR_AXLE);                     // leaf springs
    }
    for (const z of [-2.3, -0.7, 0.6, 2.4]) box(B, 0.86, 0.08, 0.08, u, 0, 0.42, z);
    box(B, 0.62, 0.22, 1.7, u, 0, 0.46, -1.5);                                   // engine and gearbox
    box(B, 1.5, 0.07, 0.09, u, 0, WHEEL_R, FRONT_AXLE);
    rod(B, new THREE.Vector3(-0.75, WHEEL_R, REAR_AXLE), new THREE.Vector3(0.75, WHEEL_R, REAR_AXLE), 0.05, u, 8);
    cyl(B, 0.16, 0.16, 0.22, u, 0.06, WHEEL_R, REAR_AXLE, 10).rotation.x = Math.PI / 2;
    rod(B, new THREE.Vector3(0, 0.38, -0.6), new THREE.Vector3(0.06, WHEEL_R, REAR_AXLE - 0.1), 0.04, u, 6);   // driveshaft
    box(B, 0.24, 0.2, 0.5, u, -0.68, 0.48, 1.05);                               // the tank
    rod(B, new THREE.Vector3(0.3, 0.36, -1.0), new THREE.Vector3(0.58, 0.32, 0.1), 0.03, u, 6);
    cyl(B, 0.1, 0.1, 0.62, u, 0.6, 0.33, 0.45, 10).rotation.x = Math.PI / 2;    // muffler
    rod(B, new THREE.Vector3(0.6, 0.33, 0.76), new THREE.Vector3(0.66, 0.3, 2.36), 0.03, u, 6);
    const spare = new THREE.Mesh(new THREE.CylinderGeometry(0.36, 0.36, 0.24, 18), u);
    spare.position.set(0, 0.4, 2.15); B.add(spare);
  }

  // ---------- the cab from outside: hidden while driving ----------
  private buildShell() {
    const S = this.shell, m = this.m;
    // doors and cab corners below the belt, the door markings and handles on them
    mesh(S, side(rrect(-0.76, 0.5, 0.86, 1.13, 0.02), 1.98, 0, 0.035), m.paint);
    for (const s of [-1, 1]) {
      decal(S, 1.22, 0.62, m.decal, s < 0 ? DECAL.doorL : DECAL.doorR, s * 0.992, 0.82, -0.19, s * Math.PI / 2);
      box(S, 0.016, 0.03, 0.14, m.chrome, s * 1.0, 1.045, 0.3);
      box(S, 0.012, 0.022, 1.6, m.chrome, s * 0.996, 0.93, 0.05);
    }
    // the glasshouse: each side a frame round the door window, the roof over both
    const gh = new THREE.Shape();
    gh.moveTo(0.86, 1.12); gh.lineTo(0.45, 1.775); gh.quadraticCurveTo(0.42, 1.8, 0.36, 1.8);
    gh.lineTo(-0.7, 1.8); gh.quadraticCurveTo(-0.76, 1.8, -0.76, 1.74); gh.lineTo(-0.76, 1.12); gh.lineTo(0.86, 1.12);
    gh.holes.push(hole([[0.746, 1.17], [-0.38, 1.17], [-0.38, 1.71], [-0.36, 1.73], [0.396, 1.73]]));
    for (const s of [-1, 1]) mesh(S, side(gh, 0.05, s * 0.925, 0.012), m.paint);
    mesh(S, side(outline([[0.44, 1.765], [-0.75, 1.765], [-0.75, 1.8], [-0.7, 1.815], [0.36, 1.815], [0.43, 1.79]]), 1.86, 0, 0.03), m.paint);
    for (const s of [-1, 1]) rod(S, new THREE.Vector3(s * 0.935, 1.795, -0.42), new THREE.Vector3(s * 0.935, 1.795, 0.74), 0.008, m.chrome, 4);
    // the back of the cab above the belt, with the window
    const back = rrect(-0.93, 1.12, 0.93, 1.78, 0.04);
    back.holes.push(hole([[-0.62, 1.25], [0.62, 1.25], [0.62, 1.68], [-0.62, 1.68]]));
    mesh(S, face(back, 0.05, 0.735, 0.01), m.paint);
    // glass: the windshield in its black rubber, the door windows, the back window
    const ws = box(S, 1.76, 0.705, 0.01, m.glass, 0, 1.45, -0.615); ws.rotation.x = 0.552;
    for (const [y, z] of [[1.755, -0.43], [1.15, -0.8]] as const) box(S, 1.78, 0.025, 0.03, m.trim, 0, y, z).rotation.x = 0.552;
    const pane = new THREE.ShapeGeometry(outline([[0.746, 1.17], [-0.38, 1.17], [-0.38, 1.71], [-0.36, 1.73], [0.396, 1.73]])).rotateY(Math.PI / 2);
    for (const s of [-1, 1]) mesh(S, pane.clone().translate(s * 0.925, 0, 0), m.glass);
    box(S, 1.24, 0.43, 0.01, m.glass, 0, 1.465, 0.735);
    // an amber beacon on the roof, switched off
    box(S, 0.22, 0.05, 0.22, m.trim, 0, 1.84, 0.25);
    cyl(S, 0.075, 0.09, 0.14, m.beacon, 0, 1.935, 0.25, 12);
  }

  // ---------- the cab from inside ----------
  private buildCab() {
    const C = this.cab, m = this.m;
    // Seen from the eye (1.48 m): the windshield's lower edge is 17 degrees down, the
    // instrument brow just under it, the top of the wheel rim at the bottom of the view.
    // The dash: a moulded profile across the cab, the pad rolling over at the back.
    mesh(C, side(outline([[0.86, 0.62], [0.86, 1.12], [0.78, 1.135], [0.46, 1.14], [0.405, 1.125], [0.395, 1.095], [0.42, 1.02],
      [0.47, 0.86], [0.5, 0.805], [0.56, 0.79], [0.72, 0.7], [0.8, 0.62]]), 1.76, 0, 0.025), m.dash);
    // the instrument brow over the cluster
    mesh(C, side(outline([[0.66, 1.13], [0.66, 1.21], [0.6, 1.27], [0.48, 1.292], [0.415, 1.29], [0.4, 1.282], [0.405, 1.27], [0.42, 1.266], [0.42, 1.13]]), 0.66, -0.42, 0.02), m.dash);
    decal(C, 1.5, 0.06, m.cabDecal, CAB.defrost, 0, 1.141, -0.69, 0, -Math.PI / 2);
    // cluster and radio share one canvas; needles are three instances
    const cluster = new THREE.Group();
    cluster.position.set(-0.42, 1.165, -0.392); cluster.rotation.x = -0.15;
    decal(cluster, 0.56, 0.175, m.gauge, DASH.cluster, 0, 0, 0);
    C.add(cluster);
    const ng = new THREE.BoxGeometry(0.0035, 1, 0.002).translate(0, 0.42, 0);
    this.needles = new THREE.InstancedMesh(ng, m.needle, 3);
    this.needles.frustumCulled = false;
    noMerge(this.needles); cluster.add(this.needles);
    // the radio in its bezel, two knobs; the heater panel, the ashtray, the glovebox, the vents
    box(C, 0.4, 0.1, 0.03, m.cabTrim, 0.14, 1.06, -0.41).rotation.x = 0.2;
    decal(C, 0.34, 0.064, m.gauge, DASH.stack, 0.14, 1.06, -0.393, 0, 0.2);
    for (const s of [-1, 1]) rod(C, new THREE.Vector3(0.14 + s * 0.155, 1.06, -0.4), new THREE.Vector3(0.14 + s * 0.155, 1.06, -0.37), 0.012, m.cabChrome, 10);
    decal(C, 0.3, 0.075, m.cabDecal, CAB.heater, 0.14, 0.965, -0.428, 0, 0.3);
    box(C, 0.18, 0.04, 0.03, m.cabChrome, 0.14, 0.89, -0.47).rotation.x = 0.3;
    decal(C, 0.36, 0.13, m.cabDecal, CAB.glove, 0.53, 0.955, -0.437, 0, 0.3);
    for (const x of [-0.79, 0.79]) {
      box(C, 0.16, 0.085, 0.02, m.cabChrome, x, 1.065, -0.412).rotation.x = 0.25;
      decal(C, 0.14, 0.068, m.cabDecal, CAB.louver, x, 1.065, -0.4, 0, 0.25);
    }
    // the steering column, the turn signal stalk, the column shift, the key on its ring
    rod(C, new THREE.Vector3(-0.42, 0.88, -0.5), new THREE.Vector3(-0.42, 0.985, -0.24), 0.036, m.cabTrim, 10);
    box(C, 0.11, 0.085, 0.15, m.cabTrim, -0.42, 0.955, -0.31).rotation.x = -0.45;
    rod(C, new THREE.Vector3(-0.47, 0.985, -0.27), new THREE.Vector3(-0.63, 0.99, -0.26), 0.006, m.cabChrome, 5);
    cyl(C, 0.011, 0.011, 0.03, m.cabTrim, -0.645, 0.99, -0.26, 6).rotation.z = Math.PI / 2;
    rod(C, new THREE.Vector3(-0.37, 0.985, -0.27), new THREE.Vector3(-0.17, 0.945, -0.22), 0.008, m.cabChrome, 6);
    cyl(C, 0.016, 0.016, 0.05, m.cabTrim, -0.15, 0.942, -0.215, 8).rotation.z = Math.PI / 2;
    this.keys.position.set(-0.345, 0.95, -0.33);
    const keyring = [new THREE.TorusGeometry(0.014, 0.0025, 4, 12).translate(0, -0.018, 0),
      new THREE.BoxGeometry(0.012, 0.045, 0.003).rotateZ(0.15).translate(0.004, -0.05, 0),
      new THREE.BoxGeometry(0.014, 0.04, 0.003).rotateZ(-0.2).translate(-0.006, -0.047, 0.002),
      new THREE.BoxGeometry(0.02, 0.028, 0.006).translate(0, -0.045, -0.004)];
    this.keys.add(new THREE.Mesh(mergeGeometries(keyring.map((g) => g.index ? g.toNonIndexed() : g))!, m.cabChrome));
    noMerge(this.keys); C.add(this.keys);
    // the steering wheel: tilted towards the driver, turns about its own axis
    const pivot = new THREE.Group();
    pivot.position.set(-0.42, 1.0, -0.2); pivot.rotation.x = -0.5;
    const parts: THREE.BufferGeometry[] = [new THREE.TorusGeometry(0.195, 0.019, 10, 44), new THREE.CylinderGeometry(0.045, 0.05, 0.05, 12).rotateX(Math.PI / 2)];
    for (const s of [-1, 1]) parts.push(new THREE.BoxGeometry(0.15, 0.036, 0.014).rotateZ(s * -0.32).translate(s * 0.11, -0.03, 0.004));
    parts.push(extrude(rrect(-0.085, -0.06, 0.085, 0.035, 0.025), 0.03, 0.01).translate(0, 0, 0.02));
    this.wheelSpin.add(new THREE.Mesh(mergeGeometries(parts.map((g) => g.index ? g.toNonIndexed() : g))!, m.wheelRim));
    const horn = decal(this.wheelSpin, 0.05, 0.05, m.cabDecal, CAB.horn, 0, -0.012, 0.0365);
    noMerge(horn);
    pivot.add(this.wheelSpin);
    noMerge(pivot); C.add(pivot);
    // pedals
    box(C, 0.14, 0.075, 0.02, m.cabTrim, -0.47, 0.76, -0.56).rotation.x = -0.5;
    rod(C, new THREE.Vector3(-0.47, 0.79, -0.58), new THREE.Vector3(-0.47, 0.98, -0.64), 0.01, m.cabTrim, 4);
    box(C, 0.06, 0.15, 0.015, m.cabTrim, -0.3, 0.7, -0.64).rotation.x = -0.85;
    // floor, mat and the transmission hump; the kick panels
    box(C, 1.74, 0.03, 1.56, m.mat, 0, 0.615, -0.08);
    mesh(C, face(rrect(-0.22, 0.6, 0.22, 0.72, 0.06), 0.9, -0.38, 0.03), m.mat);
    // headliner, header over the windshield, dome light, sun visors, rear-view mirror
    box(C, 1.8, 0.03, 1.18, m.liner, 0, 1.755, 0.15);
    box(C, 1.76, 0.07, 0.1, m.liner, 0, 1.72, -0.44);
    box(C, 0.16, 0.015, 0.08, m.cabDecal, 0, 1.736, 0.28);
    for (const x of [-0.42, 0.42]) mesh(C, slabY(rrect(x - 0.21, 0.3, x + 0.21, 0.47, 0.03), 0.02, 1.71, 0.006), m.liner);
    rod(C, new THREE.Vector3(0, 1.74, -0.43), new THREE.Vector3(0, 1.69, -0.415), 0.008, m.cabTrim, 4);
    mesh(C, face(rrect(-0.125, 1.62, 0.125, 1.69, 0.025), 0.03, -0.41, 0.008), m.cabTrim);
    const rv = decal(C, 0.23, 0.056, m.cabMirror, CAB.mirror, 0, 1.655, -0.393);
    noMerge(rv);
    // the windshield's black rubber and the A-pillars inside; the B-pillars, the corners, the back
    for (const s of [-1, 1]) {
      box(C, 0.06, 0.72, 0.07, m.liner, s * 0.875, 1.45, -0.63).rotation.x = 0.552;
      box(C, 0.02, 0.7, 0.03, m.cabTrim, s * 0.85, 1.45, -0.6).rotation.x = 0.552;
      box(C, 0.07, 0.62, 0.09, m.liner, s * 0.88, 1.43, 0.42);
      box(C, 0.06, 0.62, 0.28, m.liner, s * 0.885, 1.43, 0.6);
      box(C, 0.05, 0.03, 0.82, m.liner, s * 0.89, 1.735, -0.02);            // over the door window
      // the door: card, sill, armrest, crank, handle, lock button
      mesh(C, side(outline([[0.8, 0.62], [-0.4, 0.62], [-0.4, 1.12], [0.74, 1.12], [0.8, 1.06]]), 0.05, s * 0.875, 0.012), m.doorCard);
      box(C, 0.07, 0.02, 1.22, m.dash, s * 0.87, 1.128, -0.2);
      mesh(C, side(rrect(-0.27, 0.9, 0.32, 0.965, 0.03), 0.08, s * 0.83, 0.02), m.doorCard);
      cyl(C, 0.022, 0.022, 0.012, m.cabChrome, s * 0.845, 0.99, 0.02, 10).rotation.z = Math.PI / 2;
      box(C, 0.01, 0.012, 0.09, m.cabChrome, s * 0.838, 0.99, 0.06);
      cyl(C, 0.009, 0.009, 0.04, m.cabTrim, s * 0.82, 0.99, 0.1, 6).rotation.z = Math.PI / 2;
      box(C, 0.01, 0.025, 0.08, m.cabChrome, s * 0.842, 1.02, -0.5);
      cyl(C, 0.006, 0.007, 0.03, m.cabChrome, s * 0.87, 1.15, 0.36, 6);
      // the seat belt from the B-pillar, its buckle
      rod(C, new THREE.Vector3(s * 0.855, 1.63, 0.43), new THREE.Vector3(s * 0.8, 0.83, 0.52), 0.016, m.cabTrim, 4);
    }
    box(C, 0.03, 0.05, 0.04, m.cabChrome, -0.12, 0.85, 0.47);
    box(C, 1.76, 0.58, 0.03, m.doorCard, 0, 0.92, 0.705);
    const bw = rrect(-0.88, 1.2, 0.88, 1.74, 0.03);
    bw.holes.push(hole([[-0.62, 1.25], [0.62, 1.25], [0.62, 1.68], [-0.62, 1.68]]));
    mesh(C, face(bw, 0.03, 0.705, 0.008), m.liner);
    for (const [w, h, x, y] of [[1.26, 0.02, 0, 1.25], [1.26, 0.02, 0, 1.68], [0.02, 0.43, -0.62, 1.465], [0.02, 0.43, 0.62, 1.465], [0.02, 0.43, 0, 1.465]] as const) box(C, w, h, 0.02, m.cabTrim, x, y, 0.71);
    box(C, 0.05, 0.025, 0.02, m.cabChrome, 0, 1.465, 0.705);
    // the bench: cushion and back in pleated vinyl
    mesh(C, side(outline([[0.06, 0.66], [0.07, 0.79], [0.02, 0.845], [-0.4, 0.84], [-0.5, 0.82], [-0.53, 0.76], [-0.53, 0.66]]), 1.7, 0, 0.04), m.vinyl);
    mesh(C, side(outline([[-0.46, 0.8], [-0.44, 0.88], [-0.55, 1.4], [-0.6, 1.44], [-0.66, 1.42], [-0.66, 0.8]]), 1.7, 0, 0.04), m.vinyl);
    // on the passenger side: the clipboard with the work order, a thermos, a flashlight
    const clip = new THREE.Group(); clip.position.set(0.42, 0.85, 0.15); clip.rotation.y = 0.25; C.add(clip);
    mesh(clip, new THREE.BoxGeometry(0.23, 0.012, 0.31), m.prop, 0x7a5a36);
    decal(clip, 0.22, 0.29, m.cabDecal, CAB.clip, 0, 0.007, 0, 0, -Math.PI / 2);
    const thermos = mesh(C, new THREE.CylinderGeometry(0.042, 0.042, 0.3, 12), m.prop, 0x2f5a3a);
    thermos.position.set(0.68, 0.89, 0.32); thermos.rotation.set(0, 0.4, Math.PI / 2);
    const cup = mesh(C, new THREE.CylinderGeometry(0.046, 0.044, 0.07, 12), m.prop, 0x9a9c98);
    cup.position.set(0.53, 0.89, 0.38); cup.rotation.set(0, 0.4, Math.PI / 2);
    const torch = mesh(C, new THREE.CylinderGeometry(0.02, 0.02, 0.32, 10), m.prop, 0x1a1a1c);
    torch.position.set(0.18, 0.865, 0.42); torch.rotation.set(0, -0.3, Math.PI / 2);
    // the glass from inside: a green shade band along the top, road film, the wipers' arcs
    const tint = decal(C, 1.74, 0.7, m.tint, [0, 0, 1, 1], 0, 1.45, -0.6, 0, 0.552);
    tint.renderOrder = 3; noMerge(tint);
    // the instruments' glow and the sky's
    this.cabFlood.pos[0].set(0, -999, 0, 0); this.cabFlood.pos[1].set(0, -999, 0, 0); this.cabFlood.count = 2;
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

  /** Take over another copy's needles and swinging keys (the truck goes on in another area). */
  copyCab(o: Truck) {
    Object.assign(this.gauge, o.gauge);
    Object.assign(this.swing, o.swing);
    this.keys.rotation.copy(o.keys.rotation);
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
  /** How far each wheel hangs below its place on the body (fl, fr, rl, rr), in metres: the
   *  suspension keeps the tyres on the ground while the body moves on its springs. */
  setWheelDrops(d: number[]) { for (let i = 0; i < 4; i++) this.drops[i] = THREE.MathUtils.clamp(d[i] ?? 0, -0.09, 0.09); }

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
    put(this.gi.head, this.headColor, 0.75 * head); put(this.gi.park, AMBER, 0.3 * head); put(this.gi.tail, TAIL, 0.4 * tail);
    col.needsUpdate = true;
    this.placeFloods();
  }

  private placeFloods() {
    this.group.updateWorldMatrix(true, false);
    const D = this.dashLevel ?? this.level;
    const c = this.cabFlood.pos[0].set(-0.42, 1.12, -0.28, 1).applyMatrix4(this.group.matrixWorld);
    c.w = D > 0.01 ? 0.9 : 0;
    this.cabFlood.col[0].setRGB(0.6, 0.78, 0.68).multiplyScalar(D);
    // the sky through the glass: a faint cool light from above the cab
    const k = this.cabFlood.pos[1].set(0.1, 2.3, -0.2, 1).applyMatrix4(this.group.matrixWorld);
    k.w = 0.35;
    this.cabFlood.col[1].setRGB(0.32, 0.36, 0.46);
    if (!this.flood) return;
    const { set, slots } = this.flood;
    const on = this.headOn && this.level > 0.01;
    slots.forEach((i, j) => {
      const f = FLOODS[j];
      const p = set.pos[i].set(f.p.x, f.p.y, f.p.z, 1).applyMatrix4(this.group.matrixWorld);
      p.w = on ? f.w : 0;
      // dim by colour, not by intensity: the pools fade instead of shrinking
      set.col[i].copy(this.headColor).multiplyScalar(this.level);
    });
  }

  /** Once per frame while the truck moves: wheels, needles, keys, headlight pools. */
  update(dt: number, speed: number, wheelAngle = 0) {
    this.spin -= speed / WHEEL_R * dt;
    this.wheelAngle = wheelAngle;
    WHEELS.forEach(([x, z], i) => {
      const front = i < 2, left = x < 0;
      // the left wheels are the same wheel turned round, so they spin the other way
      _q.setFromEuler(_e.set(left ? -this.spin : this.spin, (front ? -this.wheelAngle : 0) + (left ? Math.PI : 0), 0, 'YXZ'));
      this.wheels.setMatrixAt(i, _m.compose(_p.set(x, WHEEL_R + this.drops[i], z), _q, _s.set(1, 1, 1)));
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
    // the keys swing on their ring: pushed by the turns and the road
    if (dt > 0) {
      const sw = this.swing, push = speed * wheelAngle * 0.35 + (speed - sw.last) / dt * 0.004;
      sw.last = speed;
      sw.v += (-30 * sw.a - 2.2 * sw.v - push) * dt;
      sw.a = THREE.MathUtils.clamp(sw.a + sw.v * dt, -0.7, 0.7);
      this.keys.rotation.set(sw.a * 0.4, 0, sw.a);
    }
    this.flushClock();
    this.placeFloods();
  }

  /** Frees what the truck created (geometry, materials, canvas textures). */
  dispose() {
    this.group.traverse((o) => { const mesh = o as THREE.Mesh; if (mesh.geometry) mesh.geometry.dispose(); });
    const maps = new Set<THREE.Texture>();
    for (const mat of Object.values(this.m)) { const t = (mat as THREE.MeshBasicMaterial).map; if (t) maps.add(t); mat.dispose(); }
    maps.forEach((t) => t.dispose());
    const gm = this.glow.points.material as THREE.ShaderMaterial;
    gm.uniforms.uMap.value.dispose(); gm.dispose();
    if (this.flood) for (const i of this.flood.slots) this.flood.set.pos[i].w = 0;
  }
}
