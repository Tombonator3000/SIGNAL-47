import * as THREE from 'three';
import { mergeStatic, addFlood, floodlit } from '../world/kit';
import { GlowPoints } from '../world/glow';
import type { Box2, DriveArea, Obstacle, Surface } from './Drive';
import { roadFlood, HEADLIGHT_SLOTS, headlightGlints, roadMaterials, type Build, type Glint, poles, fences, delineators, signs, cattleGuard, wireMesh, roadClosed, ranchRoad, culverts, highwayWear } from './roadProps';
import { wet, wearMesh, OLD_LANES } from './roadSurface';
import { station, ranch, plants, saroLights, motelFar, trackRoute, groundWithTufts } from './roadPlaces';
import { sharedHorizon } from '../world/horizon';
import { HWY, TRK, GUARD, START, END, ENDZONE, OLD, DINER, trackNearest, trackHalf, oldNearest, surfaceY, groundGeometry, highwayGeometry, trackGeometry, vergeGeometry, oldStubGeometry } from './roadTerrain';
import { oldAsphaltTex } from './oldRoadTextures';
import { Herd } from './herd';
import { roadFlora, postsOf } from '../world/groundCover';
import type { Flora } from '../world/flora';

export { roadFlood, HEADLIGHT_SLOTS };

// The drive from SARO to STATION 01, about 1.1 km at night: 520 m south on the state
// highway, then 600 m west on the graded survey track to the closed station gate.
// Everything is built in local coordinates inside `group` (placed at `origin`), so vertex
// data stays small far from the world origin. Whatever is handed out in world terms
// (start, end, endZone, obstacles, surface and height queries, floods) has the origin added.
//
// Draw calls: about 22 for the area itself (ground and verges 1, highway 1, track 1, the ranch's
// road 1, merged statics ~8, instanced poles, posts, delineators, bushes, rocks and yucca, wires 1,
// mesas 1, haze 1, all lamp glows 1, the wear on the highway 1 but not on Low: roadSurface.ts),
// and the ground cover 7 near the camera (world/flora.ts); measured 29 to 38 per frame with the
// truck and the sky before the ground cover.
// Triangles: about 220 k, mostly the desert grid (56 k), bushes (52 k), the dry grass merged into
// the ground (29 k, a third of it on Low: roadPlaces.tufts), posts and poles, and the ground cover
// round the camera.

const S = (kind: Surface['kind'], grip: number, top: number, rough: number): Surface => ({ kind, grip, top, rough });
const SURF = {
  asphalt: S('asphalt', 1, 30, 0.05), shoulder: S('gravel', 0.85, 22, 0.3), gravel: S('gravel', 0.75, 20, 0.4),
  guard: S('gravel', 0.8, 20, 1), verge: S('dirt', 0.6, 12, 0.6), dirt: S('dirt', 0.55, 10, 0.8),
};

export class RoadArea implements DriveArea {
  group = new THREE.Group();
  /** World pose to place the truck at: southbound lane, SARO and the motel behind (just past
   *  where the truck comes into this area from SARO's, legs.ts), the survey track ahead. */
  start: { pos: THREE.Vector3; heading: number };
  /** World pose of the truck parked at the station gate. */
  end: { pos: THREE.Vector3; heading: number };
  /** World box in front of the gate; DriveController fires onArrive in it below 6 m/s. */
  endZone: Box2;
  /** Posts, poles, fences, rocks, the gate and the ends of the highway, in world terms. */
  obstacles: Obstacle[] = [];
  /** World waypoints of the way to the gate: the southbound lane, then the track. */
  route: THREE.Vector3[];
  /** The headlights light this flood set, in these slots (DriveController hands them to the truck). */
  headlights = { set: roadFlood, slots: HEADLIGHT_SLOTS };
  /** Once per crossing of the cattle guard, with the truck's speed in m/s (play a rattle). */
  onCattleGuard?: (speed: number) => void;
  /** The cattle in the pasture south of the motel (drive/herd.ts; story/pasture.ts sets them). */
  readonly herd: Herd;
  /** The brush, the grass and the stones on the road's land (world/flora.ts); the diner's and the
   *  town's walls and walks are added when they are built (World.ensureDiner). */
  readonly flora: Flora;

  private origin: THREE.Vector3;
  private night: { hemi: THREE.HemisphereLight; moon: THREE.DirectionalLight };
  /** Places on the road with ground of their own (the diner's gravel lot), world boxes and heights. */
  private lots: { box: Box2; surface: Surface; y: number }[] = [];
  private glow = new GlowPoints();
  private oldAsphalt: THREE.MeshStandardMaterial;
  private glints: Glint[];
  private last = new THREE.Vector3();
  private haveLast = false;
  private onGuard = false;

  constructor(origin: THREE.Vector3) {
    this.origin = origin.clone();
    this.group.name = 'road';
    this.group.position.copy(origin);
    // A module-level set: start it over, so a second RoadArea (after dispose) fits too.
    roadFlood.count = 0;
    for (const p of roadFlood.pos) p.set(0, -999, 0, 0);
    for (const _ of HEADLIGHT_SLOTS) addFlood(0, -999, 0, 0, 0xfff0d8, roadFlood);
    // SARO's lights stay behind when the player leaves: the night sky light of ControlRoom
    const hemi = new THREE.HemisphereLight(0x22304f, 0x2a1b10, 0.9);
    const moon = new THREE.DirectionalLight(0x8ea4d8, 0.45);
    moon.position.set(-40, 80, -60);
    this.group.add(hemi, moon, moon.target);
    this.night = { hemi, moon };

    const m = roadMaterials();
    // Caliche's main street wet along its west edge where the water ran off: the gas station's
    // hosed apron (Town.ts), the diner's lot and the laundromat
    const spots = m.asphalt.userData.wetSpots as THREE.Vector4[];
    spots[0].set(origin.x - 4.4, origin.z + 2055, 1.5, 10); spots[1].set(origin.x - 3.9, origin.z + DINER.z + 2, 1.7, 9);
    spots[2].set(origin.x - 4.3, origin.z + 2131.5, 1.2, 6);
    const b: Build = { root: this.group, statics: new THREE.Group(), glow: this.glow, m, obstacles: [], wires: [] };
    // the desert, its verges and the dry grass on them (roadPlaces.tufts; Low draws a third of it)
    const ground = groundWithTufts([groundGeometry(), ...vergeGeometry()], m.ground, 'desert');
    // the old Roswell road going off east across from the diner (OldRoad.ts has the rest of it)
    this.oldAsphalt = wet(floodlit(new THREE.MeshStandardMaterial({ map: oldAsphaltTex(), roughness: 0.7, polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -2 }), 0.01, roadFlood), 0.5, 100, 6, { lanes: OLD_LANES });
    const trk = new THREE.Mesh(trackGeometry(), m.gravel);
    trk.name = 'surveyTrack';
    this.group.add(ground, new THREE.Mesh(highwayGeometry(), m.asphalt), trk, new THREE.Mesh(oldStubGeometry(), this.oldAsphalt));
    ranch(b);    // flood slot 3
    station(b);  // flood slot 4
    // the builders placed their lamps in local terms; floods live in world terms
    for (let i = HEADLIGHT_SLOTS.length; i < roadFlood.count; i++) roadFlood.pos[i].x += origin.x, roadFlood.pos[i].y += origin.y, roadFlood.pos[i].z += origin.z;
    poles(b); fences(b); signs(b); cattleGuard(b); plants(b); ranchRoad(b); culverts(b);
    this.group.add(wearMesh(highwayWear(), surfaceY, roadFlood));   // the highway's wear (roadSurface.ts)
    motelFar(b);   // Sierra Motor Court from the road (its real buildings are SARO's, World.ts)
    this.herd = new Herd(b, roadFlood, 'road');   // the pasture south of it (HENDELSER.md, event 2)
    this.closed = roadClosed(b);
    this.glints = delineators(b).map((g) => ({ i: g.i, p: g.p.clone().add(origin) }));
    mergeStatic(b.statics);
    this.flora = roadFlora(roadFlood, postsOf(b.obstacles));
    saroLights(b);
    this.group.add(b.statics, wireMesh(b), sharedHorizon('road', this.glow), this.glow.build(), this.flora.group);

    const w = (x: number, z: number) => new THREE.Vector3(origin.x + x, origin.y + surfaceY(x, z), origin.z + z);
    this.obstacles = b.obstacles.map((o) => o.kind === 'circle' ? { ...o, x: o.x + origin.x, z: o.z + origin.z }
      : { ...o, minX: o.minX + origin.x, maxX: o.maxX + origin.x, minZ: o.minZ + origin.z, maxZ: o.maxZ + origin.z });
    this.obstacles.push(this.herd.obstacle(origin));   // the cow that goes, while it is there
    this.start = { pos: w(START.x, 495), heading: START.heading };
    this.end = { pos: w(END.x, END.z), heading: END.heading };
    this.endZone = { minX: ENDZONE.minX + origin.x, maxX: ENDZONE.maxX + origin.x, minZ: ENDZONE.minZ + origin.z, maxZ: ENDZONE.maxZ + origin.z };
    this.route = [];
    for (let z = START.z; z <= 510; z += 10) this.route.push(w(START.x, z));
    this.route.push(w(-2.6, 513), w(-4.2, 518.5));          // swing into the mouth of the track
    for (const p of trackRoute()) if (p.x < -6) this.route.push(w(p.x, p.y));
  }

  /** A place beside the road (the diner): its walls and posts hold the truck, its lot is gravel
   *  with its top at world height y. */
  addPlace(obstacles: Box2[], lot: Box2, y: number) {
    this.obstacles.push(...obstacles.map((b) => ({ kind: 'box' as const, ...b })));
    this.lots.push({ box: lot, surface: SURF.gravel, y });
  }
  /** More things beside the road that hold the truck (Caliche's buildings and posts, world/Town.ts),
   *  kept as they are given, so one that moves (the deputy's car) moves here too. */
  addObstacles(list: Obstacle[]) { this.obstacles.push(...list); }
  private lotAt(x: number, z: number) {
    for (const l of this.lots) if (x >= l.box.minX && x <= l.box.maxX && z >= l.box.minZ && z <= l.box.maxZ) return l;
    return null;
  }
  /** The old road's asphalt again, once its own picture is in (round 11, loaded with the diner). */
  refreshOldRoad() { this.oldAsphalt.map?.dispose(); this.oldAsphalt.map = oldAsphaltTex(); this.oldAsphalt.needsUpdate = true; }
  /** The barricades across the old road (closed until chapter five sends the truck out on it). */
  private closed: { group: THREE.Group; lamps: number[] };
  private closedOn = true;
  /** The barricades, for a closer look from the diner's lot (main.ts). */
  get closedSign(): THREE.Object3D { return this.closed.group; }
  get roadClosed() { return this.closedOn; }
  setRoadClosed(on: boolean) {
    if (on === this.closedOn) return;
    this.closedOn = on;
    this.closed.group.visible = on;
    // taken away, they are out of the way of the crosshair too (layer 31)
    this.closed.group.traverse((o) => o.layers.set(on ? 0 : 31));
    const col = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    const c = new THREE.Color(on ? 0xffa020 : 0x000000);   // as roadClosed gave them (GlowPoints.add)
    for (const i of this.closed.lamps) col.setXYZ(i, c.r, c.g, c.b);
    col.needsUpdate = true;
  }
  /** How much of the road's own night light is on (the diner has its own, at dawn, World.ts). */
  setNight(k: number) { this.night.hemi.intensity = 0.9 * k; this.night.moon.intensity = 0.45 * k; }

  /** What the ground is like at a world point: asphalt, gravel (track, shoulder, cattle guard) or dirt. */
  surface(x: number, z: number): Surface {
    const lot = this.lotAt(x, z);
    if (lot) return lot.surface;
    const lx = x - this.origin.x, lz = z - this.origin.z, ax = Math.abs(lx);
    if (lx > GUARD.x0 && lx < GUARD.x1 && lz > GUARD.z0 && lz < GUARD.z1) return SURF.guard;
    if (ax <= HWY.paved) return SURF.asphalt;
    if (ax <= HWY.shoulder) return SURF.shoulder;
    const od = oldNearest(lx, lz).d;
    if (od <= OLD.half + 0.2) return SURF.asphalt;
    if (od <= OLD.shoulder + 0.6) return SURF.shoulder;
    const t = trackNearest(lx, lz);
    if (t.d <= trackHalf(t.s)) return SURF.gravel;
    return ax <= HWY.margin || t.d <= TRK.margin ? SURF.verge : SURF.dirt;
  }

  /** World height of the surface at a world point. */
  height(x: number, z: number) { return this.lotAt(x, z)?.y ?? this.origin.y + surfaceY(x - this.origin.x, z - this.origin.z); }

  /** Every frame while the area is shown. Reads the headlight beam from flood slots 0 to 2. */
  update(dt: number, t: number, truckPos: THREE.Vector3) {
    this.glow.update(t);
    headlightGlints(roadFlood, HEADLIGHT_SLOTS, this.glow, this.glints);
    // the cattle guard rattles while the front axle is on it, from either side
    const lx = truckPos.x - this.origin.x, lz = truckPos.z - this.origin.z;
    const on2 = lx > GUARD.x0 - 1.8 && lx < GUARD.x1 + 1.8 && lz > GUARD.z0 - 1.5 && lz < GUARD.z1 + 1.5;
    if (on2 && !this.onGuard) this.onCattleGuard?.(this.haveLast && dt > 0 ? truckPos.distanceTo(this.last) / dt : 0);
    this.onGuard = on2;
    this.last.copy(truckPos); this.haveLast = true;
  }

  /** Frees the geometry, materials and canvas textures built here (shared artwork stays). */
  dispose() {
    const mats = new Set<THREE.Material>();
    this.group.traverse((o) => {
      const mesh = o as THREE.Mesh;
      if (mesh.geometry) mesh.geometry.dispose();
      if (mesh.material) (Array.isArray(mesh.material) ? mesh.material : [mesh.material]).forEach((x) => mats.add(x));
      (o as THREE.DirectionalLight).dispose?.();
    });
    for (const mat of mats) {
      for (const tex of Object.values(mat).filter((v): v is THREE.Texture => v instanceof THREE.Texture)) if (!tex.name.startsWith('art/')) tex.dispose();
      const u = (mat as THREE.ShaderMaterial).uniforms;
      if (u?.uMap?.value instanceof THREE.Texture) u.uMap.value.dispose();
      mat.dispose();
    }
    for (const p of roadFlood.pos) p.w = 0;
    roadFlood.count = 0;
    this.group.removeFromParent();
  }
}
