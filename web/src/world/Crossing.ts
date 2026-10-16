import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood } from './kit';
import { artTexture } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { MOTEL_RECT } from './geo';

// Chapter four: the way from SARO down the highway to Sierra Motor Court. Part of SARO's
// scene. The corridor's fire exit (Annex.ts) opens onto a concrete step; a ramp goes down
// to the ground, and a worn path runs west to the highway. From there the highway goes south,
// 650 m on its shoulder to the motel's lot (world/MotelFront.ts, by Codex, moved there: Tom,
// 15 and 16 October, further from the stations). The ground at SARO is 0.6 m below the floors;
// south of the site it is the road's land (Grounds.floorAt has the heights).
//
//   x -7.6 to -6.3      the step outside the fire exit (floor level)
//   x -10.6 to -7.6     the ramp down (z 5.05 to 6.35)
//   x -33.5 to -6.75    the open ground and the highway (x -28 to -20), 0.6 m down
//   x -33.5 to -15      the highway and its shoulders south to the motel (z 30 to 725)

export type Anchor = { x: number; z: number; yaw: number };
export interface CourtSite {
  proxies: Record<string, THREE.Object3D>;   // room6Door; with MotelFront also the office and the rest
  objs: Record<string, THREE.Object3D>;
  anchors: { entry: Anchor; room6Outside: Anchor; fromRoom6: Anchor; officeInside?: Anchor };
  setRoom6Light?(on: boolean): void;
}

export const GROUND = -0.6;
export const RAMP = { x0: -10.6, x1: -7.6, z0: 5.05, z1: 6.35 };
/** The open ground west of SARO, and the way down the highway to the motel (site). */
export const WEST = { x0: -33.5, x1: -6.75, z0: -19.55, z1: 64 };
export const SHOULDER = { x0: -33.5, x1: -15, z0: 30, z1: MOTEL_RECT.maxZ + 4 };

export class Crossing {
  group = new THREE.Group();
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  colliders: Collider[] = [];
  brick!: THREE.Object3D;

  constructor() {
    const st = new THREE.Group();
    this.group.add(st);
    this.step(st);
    this.path(st);
    mergeStatic(st);

    this.addZone('stoop', -7.75, -6.0, 4.75, 6.65);
    this.addZone('ramp', -10.9, -7.0, RAMP.z0, RAMP.z1);
    // the open ground west of SARO and the highway, and the ramp itself, overlapping both. It
    // runs up to the control room's plinth and north to the array fence; the rest of SARO's
    // ground is in Grounds.ts. The ramp's rails and the step's open sides are walls from below.
    this.addZone('west', WEST.x0, WEST.x1, WEST.z0, WEST.z1);
    this.col(RAMP.x0 + 0.05, -6.6, 4.7, RAMP.z0 - 0.05);
    this.col(RAMP.x0 + 0.05, -6.6, RAMP.z1 + 0.05, 6.7);
    // south down the highway to the motel: the road, its shoulders and a little of the verge
    // either side (no fences here; the right-of-way fence is 21 m out). It overlaps the
    // motel's lot (MotelFront's own zones) at the south end.
    this.addZone('shoulder', SHOULDER.x0, SHOULDER.x1, SHOULDER.z0, SHOULDER.z1);
    // the road lamps
    const post = (x: number, z: number, r = 0.25) => this.col(x - r, x + r, z - r, z + r);
    post(-18.5, 36); post(-18.5, 2);
  }

  /** Height of the ground by the fire exit's step and ramp (the rails here use it). The
   *  player's floor in SARO's scene is Grounds.floorAt, which includes the ramp. */
  floorAt = (x: number, z: number) => {
    if (x > RAMP.x1) return 0;
    if (x >= RAMP.x0 && z > RAMP.z0 - 0.1 && z < RAMP.z1 + 0.1) return GROUND * (RAMP.x1 - x) / (RAMP.x1 - RAMP.x0);
    return GROUND;
  };
  /** Out on the west side, the highway or the motel's front (for the sound of the space and
   *  the place name of a save). */
  outside(p: THREE.Vector3) { return p.x < -6.3 && p.x > MOTEL_RECT.minX && p.z > -5 && p.z < SHOULDER.z1; }
  atMotel(p: THREE.Vector3) { return p.x >= MOTEL_RECT.minX && p.x <= MOTEL_RECT.maxX + 1 && p.z >= MOTEL_RECT.minZ - 4 && p.z <= MOTEL_RECT.maxZ + 4; }
  /** Out on the highway south of SARO, on the way to the motel. */
  onHighway(p: THREE.Vector3) { return p.z > WEST.z1 - 20 && p.x > SHOULDER.x0 && p.x < SHOULDER.x1 && !this.atMotel(p); }

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
}
