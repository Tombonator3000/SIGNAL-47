import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood } from './kit';
import { artTexture } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { courtFlood } from './MotelFront';

// Chapter four: the way from SARO across the road to Sierra Motor Court. Part of SARO's
// scene. The corridor's fire exit (Annex.ts) opens onto a concrete step; a ramp goes
// down to the ground, a worn path runs west to the highway, and on the far side a short
// driveway ramp climbs to the motel's lot (world/MotelFront.ts, by Codex), which is level
// with SARO's floors. The ground out here is 0.6 m lower, so this file also gives the
// player the height of the ground (floorAt).
//
//   x -7.6 to -6.3      the step outside the fire exit (floor level)
//   x -10.6 to -7.6     the ramp down (z 5.05 to 6.35)
//   x -27.7 to -10      the open ground and the highway (x -28 to -20), 0.6 m down
//   x -27.7 to -26.2    the driveway ramp up to the lot (z 2 to 10)
//   x below -27.7       the motel's lot, walk and office (MotelFront's own zones)

export type Anchor = { x: number; z: number; yaw: number };
export interface CourtSite {
  proxies: Record<string, THREE.Object3D>;   // room6Door; with MotelFront also the office and the rest
  objs: Record<string, THREE.Object3D>;
  anchors: { entry: Anchor; room6Outside: Anchor; fromRoom6: Anchor; officeInside?: Anchor };
  setRoom6Light?(on: boolean): void;
}

const GROUND = -0.6;
const RAMP = { x0: -10.6, x1: -7.6, z0: 5.05, z1: 6.35 };
const DRIVE = { x0: -27.7, x1: -26.2, z0: 2, z1: 10 };   // the driveway ramp up to the lot

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
    this.driveway(st);
    mergeStatic(st);

    this.addZone('stoop', -7.75, -6.0, 4.75, 6.65);
    this.addZone('ramp', -10.9, -7.0, RAMP.z0, RAMP.z1);
    // the open ground between SARO and the lot (the lot's edge is a 0.6 m step: the
    // driveway ramp is the way up), and the ramp itself, overlapping both
    this.addZone('west', -27.7, -10.0, -4, 64);
    this.addZone('driveway', -28.6, -25.9, DRIVE.z0 + 0.5, DRIVE.z1 - 0.5);
    // the road lamps
    const post = (x: number, z: number, r = 0.25) => this.col(x - r, x + r, z - r, z + r);
    post(-18.5, 36); post(-18.5, 2);
  }

  /** Height of the ground under the player in SARO's scene (0 on SARO's floors). */
  floorAt = (x: number, z: number) => {
    if (x > RAMP.x1) return 0;
    if (x <= DRIVE.x0) return 0;                                   // the motel's lot
    if (x < DRIVE.x1 && z > DRIVE.z0 && z < DRIVE.z1) return GROUND * (x - DRIVE.x0) / (DRIVE.x1 - DRIVE.x0);
    if (x >= RAMP.x0 && z > RAMP.z0 - 0.1 && z < RAMP.z1 + 0.1) return GROUND * (RAMP.x1 - x) / (RAMP.x1 - RAMP.x0);
    return GROUND;
  };
  /** Out on the west side (for the sound of the space and the place name of a save). */
  outside(p: THREE.Vector3) { return p.x < -6.3 && p.x > -41.2 && p.z > -5 && p.z < 71; }
  atMotel(p: THREE.Vector3) { return p.x < -27.7; }

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

  // ---------- the driveway ramp up to the motel's lot ----------
  private driveway(st: THREE.Group) {
    const shape = new THREE.Shape();
    shape.moveTo(DRIVE.x1, GROUND + 0.02); shape.lineTo(DRIVE.x0, 0); shape.lineTo(DRIVE.x0, GROUND - 0.02); shape.closePath();
    // the lot's own asphalt under the lot's own lights, so the ramp reads as part of it
    const asphalt = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('asphalt', [0.2, 0.2]), color: 0xe2d6c2, roughness: 0.95 }), 0.055, courtFlood);
    const geo = new THREE.ExtrudeGeometry(shape, { depth: DRIVE.z1 - DRIVE.z0, bevelEnabled: false });
    const uv = geo.attributes.uv, pos = geo.attributes.position;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, pos.getZ(i), pos.getX(i) + pos.getY(i));   // metres, as the lot
    const wedge = new THREE.Mesh(geo, asphalt);
    wedge.position.z = DRIVE.z0;
    st.add(wedge);
  }
}
