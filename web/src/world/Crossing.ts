import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood } from './kit';
import { GlowPoints } from './glow';
import * as T from '../core/textures';
import { artTexture } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// Chapter four: the way from SARO across the road to Sierra Motor Court. Part of SARO's
// scene. The corridor's fire exit (Annex.ts) opens onto a concrete step; a ramp goes
// down to the ground, a worn path runs west to the highway, and on the far side is the
// motel. The ground out here is 0.6 m below SARO's floors, so this file also gives the
// player the height of the ground (floorAt).
//
//   x -7.6 to -6.3     the step outside the fire exit (floor level)
//   x -10.6 to -7.6    the ramp down (z 5.05 to 6.35)
//   x -42.3 to -10     the west lot, the highway (x -28 to -20) and the motel's front
//
// Until Codex's MotelFront.ts is in, the motel is the old backdrop in Exterior.ts, and
// this file marks room 6 on it: a number, a lit window, a lamp and a door to use. The
// rest of the game only sees the CourtSite below, which MotelFront will provide too.

export type Anchor = { x: number; z: number; yaw: number };
export interface CourtSite {
  proxies: Record<string, THREE.Object3D>;   // room6Door; with MotelFront also the office and the rest
  objs: Record<string, THREE.Object3D>;
  anchors: { entry: Anchor; room6Outside: Anchor; fromRoom6: Anchor; officeInside?: Anchor };
  setRoom6Light?(on: boolean): void;
}

const GROUND = -0.6;
const RAMP = { x0: -10.6, x1: -7.6, z0: 5.05, z1: 6.35 };
// room 6 on the old backdrop: the sixth painted door from the north end of the facade
const R6 = { x: -42.5, z: 49.4 };

export class Crossing {
  group = new THREE.Group();
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  colliders: Collider[] = [];
  glow = new GlowPoints();
  court: CourtSite;
  brick!: THREE.Object3D;
  private window!: THREE.MeshBasicMaterial;

  constructor() {
    const st = new THREE.Group();
    this.group.add(st);
    this.step(st);
    this.path(st);
    this.court = this.room6(st);
    mergeStatic(st);
    this.group.add(this.glow.build());

    this.addZone('stoop', -7.75, -6.0, 4.75, 6.65);
    this.addZone('ramp', -10.9, -7.0, RAMP.z0, RAMP.z1);
    this.addZone('west', -42.3, -10.0, -4, 64);
    // the road lamps, the canopy posts, the parked car and the sign poles
    const post = (x: number, z: number, r = 0.25) => this.col(x - r, x + r, z - r, z + r);
    post(-18.5, 36); post(-18.5, 2);
    for (let z = 26; z <= 66; z += 5) post(-40.4, z, 0.2);
    post(-31.5, 30, 0.32); post(-29.3, 30, 0.32);
    this.col(-38.6, -36.4, 31.6, 36.4);
  }

  /** Height of the ground under the player in SARO's scene (0 on SARO's floors). */
  floorAt = (x: number, z: number) => {
    if (x > RAMP.x1) return 0;
    if (x >= RAMP.x0 && z > RAMP.z0 - 0.1 && z < RAMP.z1 + 0.1) return GROUND * (RAMP.x1 - x) / (RAMP.x1 - RAMP.x0);
    return GROUND;
  };
  /** Out on the west side (for the sound of the space and the place name of a save). */
  outside(p: THREE.Vector3) { return p.x < -6.3 && p.x > -43 && p.z > -5 && p.z < 65; }
  atMotel(p: THREE.Vector3) { return p.x < -28.5; }

  private addZone(id: string, minX: number, maxX: number, minZ: number, maxZ: number) {
    const z: Zone = { id, minX, maxX, minZ, maxZ, enabled: true };
    this.zones.push(z); this.zone[id] = z;
  }
  private col(minX: number, maxX: number, minZ: number, maxZ: number) { this.colliders.push({ minX, maxX, minZ, maxZ }); }

  // ---------- the step and the ramp outside the fire exit ----------
  private step(st: THREE.Group) {
    const c = M.concrete, rail = M.pole;
    // the step, level with the corridor floor, its sides going down to the ground
    box(st, 1.3, 0.62, 1.9, c, -6.95, -0.31, 5.7);
    // the ramp: a wedge from the step down to the ground, 3 m long
    const shape = new THREE.Shape();
    shape.moveTo(RAMP.x1, 0); shape.lineTo(RAMP.x0, GROUND - 0.02); shape.lineTo(RAMP.x1, GROUND - 0.02); shape.closePath();
    const wedge = new THREE.Mesh(new THREE.ExtrudeGeometry(shape, { depth: RAMP.z1 - RAMP.z0, bevelEnabled: false }), c);
    wedge.position.z = RAMP.z0;
    st.add(wedge);
    // pipe rails along the ramp and the open sides of the step
    const at = (x: number) => (x > RAMP.x1 ? 0 : this.floorAt(x, 5.7));
    for (const z of [RAMP.z0 - 0.05, RAMP.z1 + 0.05]) {
      for (const x of [RAMP.x0 + 0.1, -9.1, RAMP.x1, -6.45]) cyl(st, 0.025, 0.025, 0.9, rail, x, at(x) + 0.45, z, 5);
      rod(st, new THREE.Vector3(RAMP.x0 + 0.1, at(RAMP.x0 + 0.1) + 0.9, z), new THREE.Vector3(RAMP.x1, 0.9, z), 0.025, rail);
      rod(st, new THREE.Vector3(RAMP.x1, 0.9, z), new THREE.Vector3(-6.45, 0.9, z), 0.025, rail);
    }
    // the brick that props the fire exit open (shown once it is open)
    const brick = box(this.group, 0.2, 0.06, 0.1, floodlit(new THREE.MeshStandardMaterial({ color: 0x8a4a34, roughness: 0.95 }), 0.012), -6.62, 0.03, 5.3);
    noMerge(brick);
    brick.visible = false;
    this.brick = brick;
    // a light over the door: one more slot in SARO's flood set
    addFlood(-6.9, 2.1, 5.7, 4.5, 0xffc47a);
  }

  // ---------- the worn path to the highway ----------
  private path(st: THREE.Group) {
    const earth = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('desert', [9.4 / 4, 1.6 / 4]), color: 0xc9b9a0, roughness: 1 }), 0.012);
    plane(st, 9.4, 1.6, earth, -15.3, GROUND - 0.008, 5.7, 0, -Math.PI / 2);
  }

  // ---------- room 6 on the old motel backdrop (until MotelFront.ts) ----------
  private room6(st: THREE.Group): CourtSite {
    const x = R6.x + 0.03;
    // the number on the door, a lamp over it, and the lit window to the south of it
    const num = plane(this.group, 0.18, 0.18, new THREE.MeshBasicMaterial({ map: T.labelCard(['6'], { w: 64, h: 64, size: 50, bg: '#d8c69a', fg: '#3a2616', border: false }), toneMapped: false }), x, 1.45, R6.z, Math.PI / 2);
    noMerge(num);
    box(st, 0.18, 0.12, 0.18, M.steel, R6.x + 0.09, 2.0, R6.z + 0.75);
    this.glow.add(R6.x + 0.12, 1.93, R6.z + 0.75, 0.9, 0xffc47a);
    this.window = new THREE.MeshBasicMaterial({ color: 0xffc27a, toneMapped: false });
    const win = plane(this.group, 1.95, 1.0, this.window, x, 1.0, 47.25, Math.PI / 2);
    noMerge(win);
    // curtains drawn across the light
    const curtain = new THREE.MeshBasicMaterial({ color: 0x7a4a2a, transparent: true, opacity: 0.55, toneMapped: false });
    for (const dz of [-0.62, 0.62]) { const c = plane(this.group, 0.7, 1.0, curtain, x + 0.005, 1.0, 47.25 + dz, Math.PI / 2); noMerge(c); }
    // what the player uses: the door
    const proxy = box(this.group, 0.4, 2.2, 1.2, new THREE.MeshBasicMaterial({ visible: false }), R6.x + 0.25, 0.55, R6.z);
    proxy.name = 'room6Door';
    noMerge(proxy);
    return {
      proxies: { room6Door: proxy },
      objs: {},
      anchors: {
        entry: { x: -27.0, z: 6.0, yaw: Math.atan2(-27.0 - R6.x, 6.0 - R6.z) },   // facing room 6
        room6Outside: { x: R6.x + 1.0, z: R6.z, yaw: Math.PI / 2 },
        fromRoom6: { x: R6.x + 1.2, z: R6.z, yaw: -Math.PI / 2 },
      },
      setRoom6Light: (on) => { this.window.color.set(on ? 0xffc27a : 0x2a2018); },
    };
  }
}
