import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit } from './kit';
import * as T from '../core/textures';
import { artTexture } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { RAMP, DRIVE, GROUND } from './Crossing';

// SARO's grounds (Tom, 4 October late evening: «SARO must look more real, with a footpath
// and a road out from the ramp with the truck; walk round SARO, the ramp and the truck,
// round the house»). The ground all round the buildings at desert level, 0.6 m below the
// floors, and what a working site has on it:
//
//   - the service road from the foot of the truck ramp, round a bend and west to the
//     highway, with the site sign, a stop sign and the radio quiet zone sign
//   - a worn footpath from the fire exit round the records annex to the foot of the ramp
//   - steps from the east walk down to the north side, so the house can be walked round:
//     east door, truck pad, ramp, path, fire exit, the windows side, steps, walk
//   - a ranch fence on the south and east lines, the power line from the highway with its
//     drop to the annex, the standby generator, a propane tank, a dumpster, the cable
//     trench to the array
//
// Walking: the player stands inside the union of zones (player/Player.ts). The ground
// zones here end at the edges of the raised slabs and plinths, so the only ways up and
// down are the ramps and the steps, whose zones overlap both levels. floorAt() gives the
// height under the player for all of SARO's scene, the walk over the road included.
//
//   floor level (y 0)    the buildings, the east walk, the landing, the truck pad, the
//                        motel's lot
//   ground (y -0.6)      everything else inside the site, and the open ground west of it

type Rect = { minX: number; maxX: number; minZ: number; maxZ: number };
type Slope = Rect & { axis: 'x' | 'z'; low: number; high: number };   // y GROUND at low, 0 at high

const r = (minX: number, maxX: number, minZ: number, maxZ: number): Rect => ({ minX, maxX, minZ, maxZ });
const within = (b: Rect, x: number, z: number) => x >= b.minX && x <= b.maxX && z >= b.minZ && z <= b.maxZ;

// Everything at floor level that the ground zones run up against.
const RAISED: Rect[] = [
  r(-6.7, 6.7, -5.9, 5.9),      // the control room's plinth
  r(-6.6, 2.6, 5.8, 11.1),      // the records annex
  r(-7.6, -6.3, 4.75, 6.65),    // the step outside the fire exit
  r(6.3, 11.0, -0.6, 3.4),      // the landing outside the east door
  r(8.0, 11.0, -19.6, -0.6),    // the east walk
  r(11.0, 12.6, -13.4, -8.6),   // S-03's apron
  r(11.0, 12.6, -1.9, -0.6),    // the path to the photo lab
  r(6.6, 13.0, 3.4, 12.4),      // the truck pad
  r(12.6, 19.2, -6.0, 1.0),     // the photo lab
  r(20.2, 39.0, -5.9, 5.9),     // the east wing
  r(-200, DRIVE.x0, -60, 120),  // Sierra Motor Court's lot and walk (MotelFront.ts)
];
// The truck ramp: its top is level with the pad, its foot on the ground.
export const TRUCK_RAMP = { minX: 6.6, maxX: 13.0, top: 12.4, foot: 16.6 };
// The steps from the east walk down to the north side.
export const STEPS = { x0: 7.1, x1: 8.0, z0: -12.6, z1: -11.4 };
const SLOPES: Slope[] = [
  { ...r(RAMP.x0, RAMP.x1, RAMP.z0 - 0.1, RAMP.z1 + 0.1), axis: 'x', low: RAMP.x0, high: RAMP.x1 },   // the fire exit's ramp
  { ...r(DRIVE.x0, DRIVE.x1, DRIVE.z0, DRIVE.z1), axis: 'x', low: DRIVE.x1, high: DRIVE.x0 },         // the motel's driveway
  { ...r(TRUCK_RAMP.minX, TRUCK_RAMP.maxX, TRUCK_RAMP.top, TRUCK_RAMP.foot), axis: 'z', low: TRUCK_RAMP.foot, high: TRUCK_RAMP.top },
  { ...r(STEPS.x0 - 0.1, STEPS.x1, STEPS.z0, STEPS.z1), axis: 'x', low: STEPS.x0 - 0.1, high: STEPS.x1 },
];

// The service road: down the ramp's line to the south, a bend to the west, then straight
// to the highway's east edge (x -20). Its line, and its width.
const ROAD_W = 5.0;
const BEND = { cx: 5.8, cz: 20.5, r: 4.0 };
const ROAD_Z = BEND.cz + BEND.r;   // 24.5, the straight run to the highway
function roadLine() {
  const pts: THREE.Vector2[] = [new THREE.Vector2(9.8, 16.2), new THREE.Vector2(9.8, BEND.cz)];
  for (let i = 1; i <= 10; i++) {
    const a = (i / 10) * Math.PI / 2;
    pts.push(new THREE.Vector2(BEND.cx + Math.cos(a) * BEND.r, BEND.cz + Math.sin(a) * BEND.r));
  }
  for (const x of [0, -8, -16, -20.4]) pts.push(new THREE.Vector2(x, ROAD_Z));
  return pts;
}
// The worn path: from the foot of the fire exit's ramp, round the annex, to the foot of
// the truck ramp where it meets the road.
const PATH = [[-10.5, 5.7], [-9.8, 7.9], [-9.2, 10.4], [-8.1, 12.4], [-6.1, 13.5], [-2.0, 14.0], [2.0, 14.3], [4.6, 15.0], [6.0, 16.4], [6.9, 18.0]]
  .map(([x, z]) => new THREE.Vector2(x, z));

export class Grounds {
  group = new THREE.Group();
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  colliders: Collider[] = [];
  private ground: Rect[] = [];

  constructor() {
    const st = new THREE.Group();
    this.group.add(st);
    this.walkable();
    this.road(st);
    this.path(st);
    this.steps(st);
    this.fence(st);
    this.power(st);
    this.signs(st);
    this.things(st);
    mergeStatic(st);
  }

  // ---------- where the player can go ----------
  private walkable() {
    // ground zones (y GROUND); each ends at a raised edge, and they overlap one another by
    // more than the player's width wherever they meet
    this.addZone('gNorth', -10.0, 6.7, -19.55, -5.95);    // the windows side, up to the array fence
    this.addZone('gPocket', 2.65, 6.55, 5.95, 20.0);      // between the control room, the annex and the pad
    this.addZone('gSouthW', -27.7, 6.55, 11.15, 34.0);    // south of the annex
    this.addZone('gSouth', -27.7, 46.0, 16.65, 34.0);     // south of the ramp, the road and the fence line
    this.addZone('gEast', 13.05, 20.15, 1.05, 34.0);      // east of the truck pad
    this.addZone('gWingS', 13.05, 46.0, 5.95, 34.0);      // south of the east wing
    this.addZone('gGap', 19.25, 20.15, -19.55, 2.0);      // the gap between the photo lab and the wing
    this.addZone('gNE', 12.65, 46.0, -19.55, -6.05);      // north of the lab and the wing
    this.addZone('gWingE', 39.05, 46.0, -19.55, 34.0);    // east of the wing, along the fence
    // the ways between the levels
    this.addZone('gRamp', TRUCK_RAMP.minX + 0.3, TRUCK_RAMP.maxX - 0.3, TRUCK_RAMP.top - 0.9, TRUCK_RAMP.foot + 0.9);
    this.addZone('gSteps', 5.6, 9.0, STEPS.z0, STEPS.z1);
    // also ground: the open ground west of SARO (Crossing.ts walks it), and the narrow strip
    // between the control room and the walk (the condenser stands in it; the steps cross it)
    this.ground.push(r(-27.7, -6.75, -19.55, 64), r(6.7, 8.0, -19.6, -0.6));
  }

  /** Height of the ground under the player anywhere in SARO's scene (0 on the floors). */
  floorAt = (x: number, z: number) => {
    for (const s of SLOPES) {
      if (!within(s, x, z)) continue;
      const v = s.axis === 'x' ? x : z;
      const k = THREE.MathUtils.clamp((v - s.low) / (s.high - s.low), 0, 1);
      return GROUND * (1 - k);
    }
    for (const b of RAISED) if (within(b, x, z)) return 0;
    for (const b of this.ground) if (within(b, x, z)) return GROUND;
    return 0;
  };

  /** On the service road (for footsteps and the place name). */
  onRoad(p: THREE.Vector3) {
    if (p.z > ROAD_Z - ROAD_W / 2 && p.z < ROAD_Z + ROAD_W / 2 && p.x < BEND.cx && p.x > -20) return true;
    if (p.x > 9.8 - ROAD_W / 2 && p.x < 9.8 + ROAD_W / 2 && p.z > TRUCK_RAMP.foot && p.z < BEND.cz) return true;
    const d = Math.hypot(p.x - BEND.cx, p.z - BEND.cz);
    return p.x >= BEND.cx && p.z >= BEND.cz && d > BEND.r - ROAD_W / 2 && d < BEND.r + ROAD_W / 2;
  }
  /** Out on SARO's ground, below the floors. */
  onGround(p: THREE.Vector3) { return this.floorAt(p.x, p.z) < -0.3; }

  private addZone(id: string, minX: number, maxX: number, minZ: number, maxZ: number) {
    const z: Zone = { id, minX, maxX, minZ, maxZ, enabled: true };
    this.zones.push(z); this.zone[id] = z;
    if (id !== 'gRamp' && id !== 'gSteps') this.ground.push(z);
  }
  private col(minX: number, maxX: number, minZ: number, maxZ: number) { this.colliders.push({ minX, maxX, minZ, maxZ }); }

  // A flat band along a line, `width` wide, `y` up, with UVs in metres (u across, v along).
  private ribbon(line: THREE.Vector2[], width: number, y: number, mat: THREE.Material) {
    const pos: number[] = [], uv: number[] = [], idx: number[] = [];
    let along = 0;
    for (let i = 0; i < line.length; i++) {
      const a = line[Math.max(0, i - 1)], b = line[Math.min(line.length - 1, i + 1)];
      const t = new THREE.Vector2().subVectors(b, a).normalize();
      const n = new THREE.Vector2(-t.y, t.x).multiplyScalar(width / 2);
      if (i > 0) along += line[i].distanceTo(line[i - 1]);
      pos.push(line[i].x + n.x, y, line[i].y + n.y, line[i].x - n.x, y, line[i].y - n.y);
      uv.push(0, along, width, along);
      if (i > 0) { const k = i * 2; idx.push(k - 2, k - 1, k, k - 1, k + 1, k); }
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
    g.setIndex(idx);
    g.computeVertexNormals();
    // the band faces up whichever way the line turns
    if (g.attributes.normal.getY(0) < 0) { g.setIndex(idx.map((_, i) => idx[i - (i % 3) + 2 - (i % 3)])); g.computeVertexNormals(); }
    const m = new THREE.Mesh(g, mat);
    return m;
  }

  // ---------- the service road ----------
  private road(st: THREE.Group) {
    const line = roadLine();
    // an old, sun-bleached asphalt (5 m to a picture, as the motel's driveway): the picture
    // is fresh wet asphalt, so it is lifted to a dry grey
    const asphalt = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('asphalt', [0.2, 0.2]), color: new THREE.Color().setRGB(1.9, 1.8, 1.62), roughness: 0.95 }), 0.012);
    const shoulder = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('desert', [0.25, 0.25]), color: 0xd8c9ae, roughness: 1 }), 0.012);
    st.add(this.ribbon(line, ROAD_W + 1.6, GROUND + 0.004, shoulder));
    st.add(this.ribbon(line, ROAD_W, GROUND + 0.012, asphalt));
    // gravel where the truck turns at the foot of the ramp
    plane(st, 9.0, 4.2, shoulder, 9.8, GROUND + 0.002, 18.0, 0, -Math.PI / 2);
    // the stop line at the highway, worn
    const paint = floodlit(new THREE.MeshStandardMaterial({ color: 0xcfc8b6, roughness: 0.9, transparent: true, opacity: 0.75 }), 0.012);
    plane(st, 0.4, ROAD_W / 2 - 0.3, paint, -18.6, GROUND + 0.016, ROAD_Z - ROAD_W / 4 - 0.1, 0, -Math.PI / 2);
  }

  // ---------- the worn footpath ----------
  private path(st: THREE.Group) {
    const earth = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('desert', [0.25, 0.25]), color: 0xc9b9a0, roughness: 1 }), 0.012);
    st.add(this.ribbon(PATH, 1.1, GROUND + 0.007, earth));
  }

  // ---------- steps from the east walk down to the north side ----------
  private steps(st: THREE.Group) {
    const c = M.concrete;
    const w = STEPS.z1 - STEPS.z0, zc = (STEPS.z0 + STEPS.z1) / 2;
    // three treads below the walk's level, each 0.3 m deep and 0.155 m down
    for (let k = 1; k <= 3; k++) {
      const top = -0.155 * k, x1 = STEPS.x1 - 0.3 * (k - 1), x0 = x1 - 0.3;
      box(st, x1 - x0, top - GROUND, w, c, (x0 + x1) / 2, (top + GROUND) / 2, zc);
    }
    // pipe handrails down both sides
    for (const z of [STEPS.z0 + 0.05, STEPS.z1 - 0.05]) {
      cyl(st, 0.025, 0.025, 0.95, M.pole, STEPS.x0 - 0.05, GROUND + 0.47, z, 6);
      rod(st, new THREE.Vector3(STEPS.x0 - 0.05, GROUND + 0.92, z), new THREE.Vector3(STEPS.x1, 1.0, z), 0.022, M.pole, 6);
    }
  }

  // ---------- the ranch fence on the south and east lines ----------
  private fence(st: THREE.Group) {
    const wood = floodlit(new THREE.MeshStandardMaterial({ color: 0x5a4a3a, roughness: 1 }), 0.012);
    const line: [number, number][] = [[-18.6, 34.5], [46.5, 34.5], [46.5, -19.9]];
    const wires: number[] = [];
    for (let s = 0; s < line.length - 1; s++) {
      const [ax, az] = line[s], [bx, bz] = line[s + 1];
      const len = Math.hypot(bx - ax, bz - az), n = Math.round(len / 4);
      for (let i = 0; i <= n; i++) {
        const x = ax + (bx - ax) * i / n, z = az + (bz - az) * i / n;
        const lean = Math.sin(i * 12.9898 + s * 78.233) * 0.04;   // old posts lean a little
        const p = cyl(st, 0.06, 0.07, 1.35, wood, x, GROUND + 0.6, z, 5);
        p.rotation.z = lean; p.rotation.x = lean * 0.6;
      }
      for (const h of [0.45, 0.75, 1.05]) wires.push(ax, GROUND + h, az, bx, GROUND + h, bz);
    }
    this.group.add(this.lines(wires, 0x2a2724));
    // tumbleweeds caught against the wire
    const weed = floodlit(new THREE.MeshStandardMaterial({ color: 0x6a5a40, roughness: 1, flatShading: true }), 0.012);
    for (const [x, z, s] of [[3.0, 34.1, 0.45], [17.5, 34.2, 0.6], [30.0, 34.1, 0.4], [46.1, 22.0, 0.5], [46.1, 8.5, 0.35]]) {
      const m = new THREE.Mesh(new THREE.IcosahedronGeometry(s, 1), weed);
      m.position.set(x, GROUND + s * 0.8, z); m.scale.y = 0.8;
      st.add(m);
    }
  }

  // Thin lines (wires): one draw call for all of them.
  private lines(v: number[], color: number) {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(v, 3));
    const l = new THREE.LineSegments(g, new THREE.LineBasicMaterial({ color }));
    noMerge(l);
    return l;
  }
  // A sagging wire from a to b in short straight pieces.
  private wire(out: number[], a: THREE.Vector3, b: THREE.Vector3, sagPerM = 0.018) {
    const n = 12, sag = a.distanceTo(b) * sagPerM;
    let prev = a.clone();
    for (let i = 1; i <= n; i++) {
      const t = i / n;
      const p = a.clone().lerp(b, t);
      p.y -= Math.sin(t * Math.PI) * sag;
      out.push(prev.x, prev.y, prev.z, p.x, p.y, p.z);
      prev = p;
    }
  }

  // ---------- the power line ----------
  // Three wires along the highway; a branch east along the road to a pole with the
  // transformer, and the service drop from there to the meter on the annex's south wall.
  private power(st: THREE.Group) {
    const wood = floodlit(new THREE.MeshStandardMaterial({ color: 0x4e4234, roughness: 1 }), 0.012);
    const grey = floodlit(new THREE.MeshStandardMaterial({ color: 0x6b6e6c, roughness: 0.6, metalness: 0.4 }), 0.012);
    const H = 9.0;
    const wires: number[] = [];
    // a pole with its crossarm square to `dir`; returns the three wire points
    const pole = (x: number, z: number, dir: number) => {
      cyl(st, 0.13, 0.16, H + 0.6, wood, x, GROUND + (H + 0.6) / 2 - 0.3, z, 7);
      const arm = box(st, 2.2, 0.1, 0.1, wood, x, GROUND + H - 0.5, z);
      arm.rotation.y = dir;
      const across = new THREE.Vector3(Math.cos(dir), 0, -Math.sin(dir));
      const pts: THREE.Vector3[] = [];
      for (const k of [-0.95, 0, 0.95]) {
        const p = new THREE.Vector3(x, GROUND + H - 0.32, z).addScaledVector(across, k);
        cyl(st, 0.04, 0.05, 0.16, M.ceramic, p.x, p.y - 0.08, p.z, 6);
        pts.push(p);
      }
      this.col(x - 0.25, x + 0.25, z - 0.25, z + 0.25);
      return pts;
    };
    const span = (a: THREE.Vector3[], b: THREE.Vector3[]) => a.forEach((p, i) => this.wire(wires, p, b[i]));
    // along the highway (the arm across the road's line)
    const hwy = [-16.8, 28.5, 69.0].map((z) => pole(-16.8, z, 0));
    const north = pole(-16.8, -14.0, 0);
    span(north, hwy[0]); span(hwy[0], hwy[1]); span(hwy[1], hwy[2]);
    // off past the ends of the site: the line goes on
    span(north, north.map((p) => p.clone().add(new THREE.Vector3(0, 0.3, -60))));
    span(hwy[2], hwy[2].map((p) => p.clone().add(new THREE.Vector3(0, 0.3, 60))));
    // the branch to SARO
    const b1 = pole(-3.0, 28.5, Math.PI / 2);
    const b2 = pole(3.6, 18.6, Math.PI / 2 + 0.9);
    span(hwy[0], b1); span(b1, b2);
    // the transformer on the last pole and the service drop to the annex
    cyl(st, 0.32, 0.32, 1.0, grey, 3.6 - 0.36, GROUND + H - 2.1, 18.6, 10);
    cyl(st, 0.34, 0.34, 0.06, grey, 3.6 - 0.36, GROUND + H - 1.58, 18.6, 10);
    const head = new THREE.Vector3(1.6, 3.55, 10.86);
    for (const dz of [-0.06, 0, 0.06]) this.wire(wires, new THREE.Vector3(3.6 - 0.36, GROUND + H - 1.7, 18.6 + dz * 3), head.clone().add(new THREE.Vector3(0, 0, dz)), 0.03);
    // down guy at the end pole
    wires.push(3.6, GROUND + H - 0.6, 18.6, 6.0, GROUND, 15.5);
    // weatherhead, conduit and the meter on the wall
    rod(st, new THREE.Vector3(1.6, 3.6, 10.86), new THREE.Vector3(1.6, 1.75, 10.86), 0.03, grey, 6);
    box(st, 0.3, 0.42, 0.14, grey, 1.6, 1.5, 10.88);
    cyl(st, 0.1, 0.1, 0.06, M.glassDark, 1.6, 1.55, 10.96, 12).rotation.x = Math.PI / 2;
    this.group.add(this.lines(wires, 0x101215));
  }

  // ---------- signs ----------
  private signs(st: THREE.Group) {
    const lit = (map: THREE.Texture) => floodlit(new THREE.MeshStandardMaterial({ map, roughness: 0.8 }), 0.012);
    // the site sign at the highway, facing the traffic: a low block wall with a panel
    const sx = -14.5, sz = 20.8;
    box(st, 0.45, 1.25, 3.4, M.concrete, sx, GROUND + 0.62, sz);
    const face = lit(siteSign());
    const sign = plane(st, 3.1, 1.0, face, sx - 0.23, GROUND + 0.72, sz, -Math.PI / 2);
    sign.name = 'siteSign';
    plane(st, 3.1, 1.0, face, sx + 0.23, GROUND + 0.72, sz, Math.PI / 2);   // the same on the side facing the site
    this.col(sx - 0.3, sx + 0.3, sz - 1.75, sz + 1.75);
    // radio quiet zone, on two posts on the other side of the road
    const qx = -12.5, qz = 28.3;
    for (const dz of [-0.55, 0.55]) cyl(st, 0.04, 0.04, 2.0, M.pole, qx, GROUND + 1.0, qz + dz, 6);
    plane(st, 1.3, 0.85, lit(T.labelCard(['RADIO QUIET ZONE', 'NO TRANSMITTERS', 'BEYOND THIS SIGN', 'CB OR TWO-WAY RADIOS'], { w: 256, h: 168, size: 22, bg: '#ece6d6', fg: '#8a1a14' })), qx - 0.05, GROUND + 1.55, qz, -Math.PI / 2);
    this.col(qx - 0.2, qx + 0.2, qz - 0.7, qz + 0.7);
    // stop sign where the road meets the highway, facing traffic leaving SARO
    const tx = -19.0, tz = ROAD_Z - ROAD_W / 2 - 0.9;
    cyl(st, 0.035, 0.035, 2.2, M.pole, tx, GROUND + 1.1, tz, 6);
    const stop = plane(st, 0.76, 0.76, floodlit(new THREE.MeshStandardMaterial({ map: stopSign(), transparent: true, alphaTest: 0.5, roughness: 0.6 }), 0.012), tx + 0.05, GROUND + 2.0, tz, Math.PI / 2);
    noMerge(stop); st.add(stop);
    this.col(tx - 0.15, tx + 0.15, tz - 0.15, tz + 0.15);
  }

  // ---------- what stands about ----------
  private things(st: THREE.Group) {
    const G = GROUND;
    const lit = (o: THREE.MeshStandardMaterialParameters) => floodlit(new THREE.MeshStandardMaterial(o), 0.012);
    const beige = lit({ color: 0xb9ae92, roughness: 0.7, metalness: 0.3 });
    const dark = lit({ color: 0x2a2c2e, roughness: 0.6, metalness: 0.4 });
    const green = lit({ color: 0x2f4a36, roughness: 0.8, metalness: 0.3 });
    const white = lit({ color: 0xd9d6cc, roughness: 0.5, metalness: 0.2 });
    const yellow = lit({ color: 0xc89a2a, roughness: 0.6 });

    // standby generator in its housing, on a pad in the pocket behind the control room
    const gx = 4.3, gz = 8.4;
    box(st, 1.5, 0.12, 3.0, M.concrete, gx, G + 0.06, gz);
    box(st, 1.1, 1.35, 2.6, beige, gx, G + 0.8, gz);
    for (let k = 0; k < 7; k++) box(st, 0.02, 0.06, 1.0, dark, gx + 0.56, G + 0.55 + k * 0.1, gz + 0.5);   // louvres
    cyl(st, 0.06, 0.06, 0.7, dark, gx - 0.2, G + 1.8, gz - 0.9, 8);                                         // exhaust
    box(st, 0.25, 0.35, 0.18, dark, gx + 0.6, G + 1.05, gz - 0.8);                                          // control box
    plane(st, 0.4, 0.16, lit({ map: T.labelCard(['STANDBY', 'GENERATOR'], { w: 128, h: 52, size: 17 }), roughness: 0.8 }), gx + 0.555, G + 1.32, gz - 0.2, Math.PI / 2);
    this.col(gx - 0.6, gx + 0.6, gz - 1.35, gz + 1.35);

    // propane tank on its saddles south of the annex, with a bollard each end
    const px = -3.6, pz = 17.6;
    const tank = cyl(st, 0.5, 0.5, 2.2, white, px, G + 0.75, pz, 14); tank.rotation.z = Math.PI / 2;
    for (const dx of [-1.1, 1.1]) { const cap = new THREE.Mesh(new THREE.SphereGeometry(0.5, 14, 8), white); cap.position.set(px + dx, G + 0.75, pz); cap.scale.x = 0.45; st.add(cap); }
    for (const dx of [-0.7, 0.7]) box(st, 0.25, 0.3, 0.8, M.concrete, px + dx, G + 0.15, pz);
    for (const dx of [-1.9, 1.9]) cyl(st, 0.08, 0.08, 1.0, yellow, px + dx, G + 0.5, pz + 0.75, 8);
    this.col(px - 1.7, px + 1.7, pz - 0.6, pz + 0.6);

    // a dumpster by the road
    const dx = 0.4, dz = 19.2;
    box(st, 1.9, 1.1, 1.2, green, dx, G + 0.65, dz);
    const lid = box(st, 1.95, 0.06, 1.3, dark, dx, G + 1.24, dz + 0.05); lid.rotation.x = -0.08;
    for (const ox of [-0.8, 0.8]) for (const oz of [-0.45, 0.45]) cyl(st, 0.05, 0.05, 0.1, dark, dx + ox, G + 0.05, dz + oz, 6);
    this.col(dx - 1.0, dx + 1.0, dz - 0.65, dz + 0.7);

    // bollards at the foot of the truck ramp
    for (const x of [TRUCK_RAMP.minX - 0.3, TRUCK_RAMP.maxX + 0.3]) {
      cyl(st, 0.1, 0.1, 1.0, yellow, x, G + 0.5, TRUCK_RAMP.foot + 0.3, 8);
      this.col(x - 0.15, x + 0.15, TRUCK_RAMP.foot + 0.15, TRUCK_RAMP.foot + 0.45);
    }

    // the cable trench from the control room to the array: concrete covers, flush with the ground
    for (let z = -6.4; z > -19.4; z -= 1.05) box(st, 0.7, 0.06, 1.0, M.concrete, -1.0, G + 0.01, z);

    // downpipes at the corners of the control room and the annex
    for (const [x, z] of [[-6.36, -4.86], [6.36, -4.86], [-6.36, 4.6], [-6.36, 10.86], [2.36, 10.86]]) {
      box(st, 0.09, 3.6, 0.09, M.pole, x, 1.75, z);
      box(st, 0.09, 0.09, 0.3, M.pole, x, 0.05, z + (z > 0 ? 0.15 : -0.15));
    }
  }
}

// The site sign: name and the line every visitor reads.
function siteSign() {
  return T.canvasTex(512, 168, (g, w, h) => {
    g.fillStyle = '#2a3a46'; g.fillRect(0, 0, w, h);
    g.strokeStyle = '#d9cfb6'; g.lineWidth = 4; g.strokeRect(8, 8, w - 16, h - 16);
    g.fillStyle = '#e8dfca'; g.textAlign = 'center';
    g.font = '500 64px Oswald'; g.fillText('SARO', w / 2, 74);
    g.font = '500 21px Oswald'; g.fillText('SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY', w / 2, 106);
    g.font = '18px Oswald'; g.fillStyle = '#c9bea4'; g.fillText('AUTHORIZED VEHICLES ONLY  ·  NO VISITORS AFTER DARK', w / 2, 142);
  });
}

// A stop sign, the octagon on a clear background.
function stopSign() {
  return T.canvasTex(128, 128, (g, w) => {
    g.clearRect(0, 0, w, w);
    const oct = (rad: number) => {
      g.beginPath();
      for (let i = 0; i < 8; i++) { const a = Math.PI / 8 + i * Math.PI / 4; g.lineTo(w / 2 + Math.cos(a) * rad, w / 2 + Math.sin(a) * rad); }
      g.closePath();
    };
    g.fillStyle = '#efe9dc'; oct(62); g.fill();
    g.fillStyle = '#a3241b'; oct(57); g.fill();
    g.fillStyle = '#efe9dc'; g.textAlign = 'center'; g.font = '600 38px Oswald'; g.fillText('STOP', w / 2, w / 2 + 14);
  });
}
