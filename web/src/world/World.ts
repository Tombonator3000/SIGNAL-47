import * as THREE from 'three';
import type { Player, Zone } from '../player/Player';
import type { Collider } from './ControlRoom';
import type { AreaId, FieldSite } from '../story/Chapter3';
import type { TruckPose } from '../story/state';
import type { Station01 } from './Station01';
import type { RoadArea } from '../drive/RoadArea';
import { TruckTrouble } from '../drive/trouble';
import type { DriveController } from '../drive/Drive';
import type { Truck } from '../drive/Truck';
import type { EngineSound } from '../drive/engineSound';
import type { Room6 } from './Room6';
import type { Diner } from './Diner';
import type { Town } from './Town';
import type { OldRoad } from '../drive/OldRoad';
import type { Herd } from '../drive/herd';
import type { DriveInput, DriveArea } from '../drive/Drive';
import type { LegId, Exit } from '../drive/legs';
import { Crossing, SHOULDER, WEST, type CourtSite } from './Crossing';
import { Grounds } from './Grounds';
import { Field } from './Field';
import { MotelFront } from './MotelFront';
import { flood as siteFlood, addFlood, followNear, type FloodSet } from './kit';
import { loadArtFor, loadArtSome, artTexture, DINER_ART, OLDROAD_ART, TOWN_ART } from '../core/art';
import { STATION_TURN, SARO_IN_ROAD, MOTEL_AT, MOTEL_RECT, MOTEL_GLOWS, inRect } from './geo';
import { FENCE_Z } from './arrayLayout';
import { horizonHaze } from './horizon';
import { DINER, TOWN } from '../drive/roadShape';
import { firstTime } from '../core/hints';
import { getQuality } from '../core/quality';
import { surfaceY } from '../drive/roadTerrain';
import { LightCones, GroundHaze, sodiumGlows } from './airlight';
import { followFlora, floraTest, type Flora } from './flora';
import { saroFlora, stationFlora, postsOf } from './groundCover';
import { NightLife, lifeLamps } from './nightlife';
// The places of the night. SARO is built at the start; the road and STATION 01 are
// loaded the first time they are needed (their code is in separate files that the
// browser fetches then) and kept afterwards. Each area lies far from the others in the
// same scene, beyond the camera's far plane, so only one is ever drawn, and the player
// walks inside that area's zones.
//
//   SARO        around the origin       control room, yard, records, the truck pad
//   road        around (8000, 0, 0)     the drive south to the survey track
//   STATION 01  around (0, 0, 8000)     the survey station, chapter three
//   room 6      around (0, 0, -8000)    Nora's room at Sierra Motor Court, chapter four
//   diner       in the road area        Mesa Diner on the highway, 2 km south (Diner.ts, by Codex), «All Night»,
//                                       and Caliche's main street south of it (Town.ts)
//   roswell     on the road's map       the old Roswell road at dawn, chapter six (drive/OldRoad.ts): mile 0
//                                       is across the highway from the diner, and C 13 km on
//
// Sierra Motor Court is part of SARO's area, 650 m down the highway (MotelFront.ts, geo.ts
// MOTEL_AT; Tom, 15 and 16 October: further from the stations). The walk there from the fire
// exit, along the highway's shoulder, is SARO's too (Crossing.ts); the truck can be left on
// its lot.

export const ROAD_ORIGIN = new THREE.Vector3(8000, 0, 0);
export const STATION_ORIGIN = new THREE.Vector3(0, 0, 8000);
export const ROOM6_ORIGIN = new THREE.Vector3(0, 0, -8000);
// Mesa Diner stands in the road area, beside the highway 2 km south of the start (roadShape.ts)
export const DINER_ORIGIN = new THREE.Vector3(8000 + DINER.x, DINER.y + 0.1, DINER.z);
// The night areas see the camera's far plane at 5 km and thick haze; the old road at dawn
// sees farther (the camera goes up at the end).
const FAR = 5000, FAR_OLDROAD = 12000, FOG = 0.0021, FOG_OLDROAD = 0.00055;
/** The places the truck can be left on (a lot with a park box): SARO's pad, the motel's lot,
 *  the gravel outside STATION 01's gate, the diner's lot. */
const PLACES: LegId[] = ['saro', 'motel', 'station01', 'diner'];
/** The places with a truck of their own (the motel's is SARO's: the same area). */
const TRUCKS: LegId[] = ['saro', 'station01', 'diner'];
const UP = new THREE.Vector3(0, 1, 0);
/** The area a place is in (the motel's lot is SARO's). */
const areaOf = (l: LegId): AreaId => l === 'motel' ? 'saro' : l;
const inBox = (b: { minX: number; maxX: number; minZ: number; maxZ: number } | null, p: THREE.Vector3) =>
  !!b && p.x >= b.minX && p.x <= b.maxX && p.z >= b.minZ && p.z <= b.maxZ;
/** The truck's headlights on the diner and the diner's neon on the road (Diner.shine, Diner.neon),
 *  to look about as strong as in their own sets: the diner's lamps fall off about ten times faster
 *  (falloff 0.14 against 0.012), and a lamp's strength sets how far it reaches. */
const HEAD_ON_DINER = 2.5, NEON_REACH = 2, NEON_ON_ROAD = 0.3;
// the motel's lamps fall off about four times faster than SARO's (0.055 against 0.012)
const HEAD_ON_MOTEL = 1.6;
// Caliche's lamps fall off a little faster than the road's (0.03 against 0.012); its nearest street
// lamps are lamps in the road's set too, so their pools lie on the highway (Town.lampsTo)
const HEAD_ON_TOWN = 1.8, TOWN_ON_ROAD = 0.8;
/** The light in the air (airlight.ts): how much the cones under the lamps add, and the haze on the
 *  ground, at the dead of night (both die away towards dawn). Kept low: memory.md, the floods
 *  that turned the night into day. */
const AIR_CONES = 0.065, AIR_HAZE = 1;
type HazeLamp = { p: THREE.Vector3; r: number; col: THREE.Color; k: number };
type Box = { minX: number; maxX: number; minZ: number; maxZ: number };
/** Whether (x, z) is in one of the boxes (or within m of it): what stands in a tumbleweed's way. */
const inBoxes = (boxes: readonly Box[], x: number, z: number, m = 0.4) => boxes.some((b) => x > b.minX - m && x < b.maxX + m && z > b.minZ - m && z < b.maxZ + m);
/** SARO's land out past the site, as Exterior.ground draws it, in the road's terms (the far lights on it). */
const saroLand = (mx: number, mz: number) => {
  const x = mx - SARO_IN_ROAD.x, z = mz - SARO_IN_ROAD.z, far = Math.hypot(x, z);
  return far > 300 ? -2 + Math.sin(x * 0.004 + z * 0.003) * 6 * Math.min(1, (far - 300) / 600) : -0.62;
};

export interface WorldDeps {
  scene: THREE.Scene;
  camera: THREE.PerspectiveCamera;
  player: Player;
  saro: { groups: THREE.Object3D[]; zones: Zone[]; colliders: Collider[]; truck: { x: number; z: number; heading: number }; truckCol: Collider; truckProxy: THREE.Object3D };
  applyQuality: () => void;
  audio: () => { ctx: AudioContext | null; sfx: AudioNode };
  thud: (gain: number) => void;
  // story hooks: the fade, a locked moment, and game time
  fade: (on: boolean, text?: string) => void;
  hold: (on: boolean) => void;
  after: (s: number, fn: () => void) => void;
  toast: (text: string, secs?: number) => void;
  clock: () => string;
  skipClock: (sec: number) => void;
  touch: () => boolean;
  /** The dawn now (0..1) and the colour of the haze, from the sky. */
  sky: () => { dawn: number; fog: THREE.Color; sunDir: THREE.Vector3; sun: number };
  /** The game's clock, seconds since midnight (Caliche keeps the night's hours, Town.update). */
  time: () => number;
}

type Modules = {
  Station01: typeof import('./Station01');
  RoadArea: typeof import('../drive/RoadArea');
  Drive: typeof import('../drive/Drive');
  Truck: typeof import('../drive/Truck');
  engine: typeof import('../drive/engineSound');
  Room6: typeof import('./Room6');
  Diner: typeof import('./Diner');
  Town: typeof import('./Town');
  OldRoad: typeof import('../drive/OldRoad');
  legs: typeof import('../drive/legs');
};

export class World {
  area: AreaId = 'saro';
  site: (Station01 & FieldSite) | null = null;
  driving = false;
  onArriveStation?: () => void;
  onArriveSaro?: () => void;
  /** The player has got into the truck (a trip has begun): the flashlight on its seat (main.ts). */
  onGetIn?: () => void;
  /** The truck has been left on a place (any trip, the chapters' and the player's own). */
  onParked?: (place: LegId) => void;
  /** What the truck where it stands offers (main.ts asks the chapters, Tom 15 October: the
   *  truck any time): the chapter's drive, or one of the player's own. */
  truckChoice: (place: LegId) => { label: string; use: () => void } = (place) => ({ label: 'Drive', use: () => this.roam(place) });
  onStationLoaded?: (site: FieldSite) => void;
  onRoom6Loaded?: (room: Room6) => void;
  onEnterRoom6?: () => void;
  onLeaveRoom6?: () => void;
  room6: Room6 | null = null;
  diner: Diner | null = null;
  onDinerLoaded?: (diner: Diner) => void;
  /** Caliche, the town south of the diner (Town.ts): built with the diner, walked in its area. */
  town: Town | null = null;
  onTownLoaded?: (town: Town) => void;
  /** The diner's and the town's ground together, walked in the diner's area. */
  private dinerWalk: { zones: Zone[]; colliders: Collider[] } | null = null;
  /** Caliche's nearest street lamps as lamps in the road's set (Town.lampsTo). */
  private townSlots: number[] = [];
  /** The highway has been built (the barricades on the old road get a closer look, main.ts). */
  onRoadLoaded?: (road: RoadArea) => void;
  /** The diner's collider round its parked truck (it moves with the truck). */
  private dinerCol: Collider | null = null;
  /** The diner's neon as lamps in the road's set and the old road's (Diner.neon). */
  private neonSlots = { road: [] as number[], old: [] as number[] };
  /** An invisible box round the truck on the diner's lot, for interaction (chapter five). */
  dinerTruckProxy: THREE.Object3D | null = null;
  onDinerTruck?: (proxy: THREE.Object3D) => void;
  /** The old Roswell road (chapter six), its truck and its controller, once loaded. */
  oldRoad: OldRoad | null = null;
  oldTruck: Truck | null = null;
  oldDrive: DriveController | null = null;
  /** The cattle in the pasture south of the motel, one herd for SARO's stretch of the highway and
   *  one for the road's (drive/herd.ts), as they are built; story/pasture.ts sets both alike. */
  herds: Herd[] = [];
  /** SARO's stretch of the highway, for the headlights in the delineators (initSaro). */
  private saroGlints: { glints: (slots: readonly number[]) => void; trackArt: () => void } | null = null;
  /** Chapter six takes over: the pedals and wheel (input), or the camera (the pull-out at the
   *  end, while the truck stands). Cleared when the drive stops. */
  oldControl: { input?: (dt: number) => DriveInput; camera?: (dt: number) => void } | null = null;
  /** The way over the road and the motel's front (the old backdrop until MotelFront.ts). */
  crossing: Crossing;
  grounds: Grounds;
  /** The antenna field north of the array fence, walked through its gate (Field.ts). */
  field: Field;
  /** The brush, the grass and the stones round SARO, out in the array and down the highway
   *  (flora.ts, groundCover.ts); the road, the station and the old road have their own. */
  saroFlora: Flora;
  /** Tests: the ground cover shown now (tools/env_ground.py). */
  readonly flora = floraTest;
  motel: MotelFront;
  court: CourtSite;
  saroTruck: Truck | null = null;
  /** Each copy of the truck as it is built (main.ts: the Blender pilot gives it the model). */
  onTruck?: (t: Truck) => void;
  /** Tests steer the truck through this instead of the keys (tools/chapter3.py): fixed
   *  input, or a function asked every step (an autopilot along road.route). */
  testInput: { steer: number; throttle: number } | (() => { steer: number; throttle: number }) | null = null;

  // ---------- trips: the truck driven from one place to another, the whole way ----------
  /** Where the truck stands when nobody drives it (null while it is out on a trip). */
  truckAt: LegId | null = 'saro';
  /** The area being driven in on a trip, or null. */
  leg: LegId | null = null;
  /** Drive by itself along the way at about this speed in m/s (tests, the developer menu), or null. */
  autopilot: number | null = null;
  /** Trouble with the truck on a trip: misfires, a dead engine, flickering lights (drive/trouble.ts). */
  trouble = new TruckTrouble();
  /** The next time the truck is got into, its engine is dead and wants the key (story/abduction.ts). */
  keyStall = false;
  /** Whether random trouble may come now (main.ts: the night, no cinematic). */
  troubleOk?: () => boolean;
  private legs: Partial<Record<LegId, { area: DriveArea & { park?: { minX: number; maxX: number; minZ: number; maxZ: number } | null; routes?: Partial<Record<string, THREE.Vector3[]>> }; drive: DriveController; truck: Truck }>> = {};
  private trip: { from: LegId; to: LegId | null; perMetre: number; onArrive?: () => void; still: number; home: LegId | null; hint?: string;
    free: boolean; start: THREE.Vector3; left: boolean; secs: number; metres: number; n: number } | null = null;
  private tripNo = 0;
  /** Seconds the truck has been driven on this trip (the story's or the player's own), or null. */
  get tripSecs() { return this.driving && this.trip && !this.trip.free ? this.trip.secs : null; }
  /** The trip being driven, for the radio's programme on the way (story/tripRadio.ts): which
   *  trip (n), from and to (to null on a drive of the player's own), the metres driven, and where
   *  it is bound: its place, or on the player's own drive the way the truck is going by now (null
   *  while it could still go either way, drive/legs.ts boundOn). Null when not on a trip, on a
   *  free drive (the developer menu) and on chapter six's road. */
  get tripNow(): { n: number; from: LegId; to: LegId | null; bound: LegId | null; metres: number } | null {
    const trip = this.trip, id = this.leg, L = id ? this.legs[id] : null, legs = this.mods.legs;
    if (!this.driving || !trip || trip.free || !id || id === 'roswell' || !L || !legs) return null;
    const D = L.drive;
    return { n: trip.n, from: trip.from, to: trip.to, metres: trip.metres,
      bound: trip.to ?? legs.boundOn(id, D.pos.x, D.pos.z, D.heading, D.reversing) };
  }

  private mods: Partial<Modules> = {};
  private road: RoadArea | null = null;
  /** The places the truck any time can reach, as the story stands (main.ts, every frame): the
   *  walk gap at STATION 01 open from chapter three, the old road closed until chapter five
   *  sends the truck out on it. */
  setStoryPlaces(o: { stationOpen: boolean; oldRoadOpen: boolean }) {
    this.site?.setWalkGate(o.stationOpen);
    this.road?.setRoadClosed(!o.oldRoadOpen);
  }
  private drive: DriveController | null = null;
  private truck: Truck | null = null;          // the one that drives (lit by the road's floods)
  private parked: Truck | null = null;         // the same truck standing at the station (the station's floods)
  private engine: EngineSound | null = null;
  private dashT = 0;
  private busy = false;

  constructor(private d: WorldDeps) {
    // the walk over the road and SARO's grounds belong to SARO: built now, shown and walked with SARO
    const c = new Crossing();
    this.crossing = c;
    const g = new Grounds();
    this.grounds = g;
    // Sierra Motor Court from the outside, and its office (MotelFront.ts, by Codex)
    const m = new MotelFront();
    this.motel = m;
    this.court = m;
    const f = new Field();
    this.field = f;
    for (const part of [c, g, m, f]) {
      d.scene.add(part.group);
      d.saro.groups.push(part.group);
      d.saro.zones.push(...part.zones);
      d.saro.colliders.push(...part.colliders);
    }
    d.player.floor = g.floorAt;
    // the ground cover, kept off SARO's floors, roads, tracks and things, and low where it is walked
    this.saroFlora = saroFlora(siteFlood, d.saro.colliders, d.saro.zones);
    d.scene.add(this.saroFlora.group);
    d.saro.groups.push(this.saroFlora.group);
    followFlora(d.scene);
    followNear(d.scene);   // and the detail drawn only near (kit.nearOnly: the dressing, the wear, the rooms' clutter)
    // the light in the air: cones under SARO's sodium lamps (the array, the highway north of the
    // site and the motel's lot, read from the outdoor glows), and the haze on the ground
    const ext = d.saro.groups[0], ground = (_x: number, z: number) => z > 600 ? MOTEL_AT.y : -0.62;
    const lamps = sodiumGlows(ext, (x, y, z, size) => size >= 2.4 && size <= 5.5 && y - ground(x, z) > 4 && Math.abs(x) < 300 && z > -260 && z < 760,
      ground, (h) => h * 0.62);
    this.air.cones.push(new LightCones(lamps, ext));
    this.air.lamps.saro = lamps.map((L) => ({ p: new THREE.Vector3(L.x, L.y, L.z), r: 7, col: L.col, k: 1 }));
    // and the motel's sign, red and cyan, in the haze round it
    for (const [x, y, z, , c] of MOTEL_GLOWS.slice(0, 2)) this.air.lamps.saro.push({ p: new THREE.Vector3(x + MOTEL_AT.x, y + MOTEL_AT.y, z + MOTEL_AT.z), r: 9, col: new THREE.Color(c), k: 1 });
    const coarse = (() => { try { return matchMedia('(pointer: coarse)').matches; } catch { return false; } })();
    this.air.haze = new GroundHaze(d.scene, coarse ? [0.3] : [0.2, 0.5]);   // one layer on a phone
    // the life in the night (nightlife.ts): moths round the lamps of the site, its doors and the
    // motel's (not those indoors: the control room and the annex), bats at the big ones, lights far
    // out on the land, a tumbleweed now and then, and the dust in the beams
    this.life = new NightLife(d.scene);
    const S = SARO_IN_ROAD;
    // (the fences stop a rolling tumbleweed too: the array's chain-link, and the ranch fence on
    // the south and east lines, Grounds.fence)
    const fenced = (x: number, z: number) => (Math.abs(z - FENCE_Z) < 0.35 && x > -80 && x < 60 && !(x > -28.6 && x < -19.4))
      || (Math.abs(z - 34.5) < 0.35 && x > -18.6 && x < 46.5) || (Math.abs(x - 46.5) < 0.35 && z > -19.9 && z < 34.5);
    this.life.addArea('saro', { flood: siteFlood, ground: g.floorAt, blocked: (x, z) => inBoxes(d.saro.colliders, x, z) || fenced(x, z),
      onHighway: (x) => Math.abs(x + S.x) < 5,
      far: { toWorld: (x, z) => new THREE.Vector3(x - S.x, 0, z - S.z), toMap: (x, z) => ({ x: x + S.x, z: z + S.z }), ground: saroLand, centre: S } });
    // tumbleweeds where the wind (from the south-south-west) left them, in the lamps' light (out of
    // it they would be as dark as the ground): against the south face of the array's chain-link,
    // the poles and posts by the highway, the wing's wall between its doors, the control room's
    // west wall, down between the east walk and the lab; at the motel under the walk's posts,
    // against the pool's fence and the end of the block (lit by its own lamps). Off the doors,
    // the gate, the walk to B-12 and the paths; the first half are those Low keeps.
    this.life.addWeeds('saro', null, [[27.0, -19.64, 1.1], [-16.8, 28.95, 0.9], [25.5, 6.26, 0.9], [12.3, -5.0, 0.8], [-8.6, -19.66, 0.95], [-7.0, -0.5, 0.8],
      [4.2, -19.68, 1.05], [27.75, -19.72, 0.7], [15.5, -19.7, 0.8], [33.5, 6.3, 0.75], [-18.5, 36.48, 0.8], [-14.5, 22.95, 0.7]]);
    this.life.addWeeds('saro', m.flood, [[-33.2, 712.0, 1.0], [-41.25, 719.35, 1.0], [-47.0, 720.36, 1.1], [-41.25, 693.35, 0.85],
      [-35.6, 711.98, 0.8], [-37.36, 711.85, 0.7], [-41.25, 678.35, 0.8], [-45.3, 720.3, 0.8]], (x, z) => inRect(MOTEL_RECT, x, z, -12));
    this.life.addMoths('saro', lifeLamps([ext, d.saro.groups[2], c.group, g.group, m.group, f.group], ext, (w) => ground(w.x, w.z),
      (p, _s, h) => Math.abs(p.x) < 400 && p.z > -500 && p.z < 800 && h > 1.5), ext,
      // the bats hunt where the player walks: the highway lamp by the site's sign, the lamps along
      // the east walk, the motel's lot
      { bats: [[new THREE.Vector3(-20.1, 7.1, 36), 3], [new THREE.Vector3(10.3, 4.1, 1.6), 2], [new THREE.Vector3(-32.05, 4.1, 668), 1]],
        walls: d.scene });
  }
  /** The life in the night: moths, bats, far lights, tumbleweeds, the dust in the beams (nightlife.ts). */
  readonly life: NightLife;
  /** The flashlight's beam, for its dust (main.ts). */
  lifeBeam: () => THREE.SpotLight | null = () => null;
  /** One of the night's strange things has it (or a cinematic, or the title): no far car, no tumbleweed (main.ts). */
  lifeQuiet: () => boolean = () => false;
  /** The light in the air: each area's cones, the haze, and the lamps that tint it per area. */
  private air: { cones: LightCones[]; haze: GroundHaze | null; lamps: { saro: HazeLamp[]; road: HazeLamp[] }; town: LightCones | null } =
    { cones: [], haze: null, lamps: { saro: [], road: [] }, town: null };
  /** Whether the camera is out in the open (main.ts: not inside a building; the cab counts). */
  airOut: () => boolean = () => true;
  /** Tests: the night without the light in the air, to compare. */
  airOff = false;

  // ---------- loading ----------
  // The truck is part of SARO from the start (it stands on the pad all night). It is
  // small; the road and the station are not, so they load on demand.
  // A material belongs to one set of fake floodlights, so each area has its own copy of
  // the truck: one on the SARO pad, one on the road, one parked at the station.
  async initSaro() {
    const { Truck } = await import('../drive/Truck');
    const t = new Truck({ flood: siteFlood });
    this.onTruck?.(t);
    const p = this.d.saro.truck;
    t.group.position.set(p.x, 0, p.z);
    t.group.rotation.y = p.heading;
    this.d.saro.groups[0].parent!.add(t.group);
    this.saroTruck = t;
    this.d.applyQuality();
    // the highway south of the site is the road's own stretch (drive/corridors.ts): the truck
    // goes from SARO onto the road without anything changing in front of it
    const { saroCorridor } = await import('../drive/corridors');
    const c = saroCorridor(siteFlood);
    this.d.saro.groups[0].parent!.add(c.group);
    c.group.visible = this.area === 'saro';
    this.d.saro.groups.push(c.group);
    this.saroRoad = c.obstacles;
    this.saroFlora.add({ blocks: postsOf(c.obstacles) });   // the posts and poles of the stretch, as the road's patch has them
    this.saroGlints = c;
    this.herds.push(c.herd);
    // the signs and big rocks beside the walk down the highway are in the way on foot too
    for (const o of c.obstacles) {
      const x = o.x - SARO_IN_ROAD.x, z = o.z - SARO_IN_ROAD.z;
      if (x > SHOULDER.x0 - 1 && x < SHOULDER.x1 + 1 && z > SHOULDER.z0 && z < SHOULDER.z1) this.d.saro.colliders.push({ minX: x - o.r, maxX: x + o.r, minZ: z - o.r, maxZ: z + o.r });
    }
    this.d.applyQuality();
  }
  private saroRoad: { kind: 'circle'; x: number; z: number; r: number }[] = [];
  /** Three flood slots for the headlight pools in an area's own set of floods. */
  private headlightSlots(set: FloodSet) {
    const slots = [0, 1, 2].map(() => addFlood(0, -999, 0, 0, 0xfff0d8, set)).filter((i) => i >= 0);
    return slots.length ? { set, slots } : undefined;
  }

  private async load<K extends keyof Modules>(k: K): Promise<Modules[K]> {
    if (!this.mods[k]) {
      const m = k === 'Station01' ? await import('./Station01')
        : k === 'RoadArea' ? await import('../drive/RoadArea')
          : k === 'Drive' ? await import('../drive/Drive')
            : k === 'Truck' ? await import('../drive/Truck')
              : k === 'Room6' ? await import('./Room6')
                : k === 'Diner' ? await import('./Diner')
                  : k === 'Town' ? await import('./Town')
                  : k === 'OldRoad' ? await import('../drive/OldRoad')
                    : k === 'legs' ? await import('../drive/legs')
                      : await import('../drive/engineSound');
      (this.mods as Record<string, unknown>)[k] = m;
    }
    return this.mods[k] as Modules[K];
  }

  // Each area is built once, even when two callers ask for it at the same time (a drive
  // needs the road and the station together). A failed load can be tried again.
  private pending: Record<string, Promise<unknown> | undefined> = {};
  private once<T>(key: string, build: () => Promise<T>): Promise<T> {
    const p = this.pending[key] ?? (this.pending[key] = build().catch((e) => { this.pending[key] = undefined; throw e; }));
    return p as Promise<T>;
  }

  private ensureStation() {
    return this.once('station', async () => {
      // the station's own pictures (render, old concrete, weathered wood, floor boards)
      // come with it, so the start of the game does not wait for them
      const [{ Station01, fieldFlood }, { Truck }, { DriveController }, { StationDrive }, { stationCorridor }] = await Promise.all([this.load('Station01'), this.load('Truck'),
        this.load('Drive'), this.load('legs'), import('../drive/corridors'), loadArtFor(['stucco', 'oldConcrete', 'weatheredWood', 'floorboards', 'gravelTrack'])]);
      const s = new Station01(STATION_ORIGIN.clone()) as Station01 & FieldSite;
      s.group.visible = false;
      s.colliders.push(s.truckCollider); // the truck the player came in stands on the pad
      this.d.scene.add(s.group);
      // the truck the player came in, parked outside the gate with its lights off
      const t = new Truck({ flood: fieldFlood });
      this.onTruck?.(t);
      const a = s.anchors.truck;
      t.group.position.set(a.x, 0, a.z);
      t.group.rotation.y = a.heading;
      t.group.visible = false;
      this.d.scene.add(t.group);
      this.parked = t;
      this.site = s;
      // the last of the survey track is the road's own (drive/corridors.ts), and the truck can
      // be driven from here and back to SARO on it (drive/legs.ts)
      const road = stationCorridor(fieldFlood);
      s.group.add(road.group);
      s.group.add(stationFlora(fieldFlood, s, STATION_ORIGIN, postsOf(road.obstacles)).group);
      const area = new StationDrive(s.colliders.filter((c) => c !== s.truckCollider), (x, z) => s.groundAt(x, z), road.obstacles);
      area.headlights = this.headlightSlots(fieldFlood);
      const drive = new DriveController(this.d.camera, t, area);
      drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      this.legs.station01 = { area, drive, truck: t };
      this.parkAt('station01');
      this.life.addArea('station01', { flood: fieldFlood, ground: (x, z) => s.groundAt(x, z), blocked: (x, z) => inBoxes(s.colliders, x, z) });
      // tumbleweeds against the hut and the yard light's pole (in its light), the shed, and the
      // outside of the south and west fences (clear of the gate and the pad)
      const O = STATION_ORIGIN;
      this.life.addWeeds('station01', null, ([[-12.4, 5.0, 0.8], [-6.6, 7.45, 0.7], [-13.5, 2.6, 0.8], [-16.4, 0.86, 0.7],
        [2.0, 26.36, 1.0], [-30.36, 8.0, 0.9], [14.0, 26.36, 0.9], [5.0, -23.65, 1.0]] as [number, number, number][]).map(([x, z, k]) => [x + O.x, z + O.z, k]));
      this.life.addMoths('station01', lifeLamps([s.group], s.group, (w) => s.groundAt(w.x, w.z), (p, _s, h) => h > 1.5 && Math.hypot(p.x - O.x, p.z - O.z) < 90), s.group,
        { bats: [[new THREE.Vector3(-7.65, 6, 8006.7), 2]], walls: this.d.scene });
      this.d.applyQuality();
      this.onStationLoaded?.(s);
      return s;
    });
  }

  private ensureRoad() {
    return this.once('road', async () => {
      const [{ RoadArea, roadFlood }, { DriveController }, { Truck }] = await Promise.all([this.load('RoadArea'), this.load('Drive'), this.load('Truck'), this.load('engine'),
        loadArtFor(['gravelTrack'])]); // the survey track's gravel comes with the road
      const truck = new Truck({ flood: roadFlood });
      this.onTruck?.(truck);
      truck.headlightFloods(roadFlood, [0, 1, 2]);
      truck.group.visible = false;
      this.d.scene.add(truck.group);
      this.truck = truck;
      const road = new RoadArea(ROAD_ORIGIN.clone());
      road.group.visible = false;
      this.d.scene.add(road.group);
      this.road = road;
      this.herds.push(road.herd);
      this.drive = new DriveController(this.d.camera, truck, road);
      this.drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      road.onCattleGuard = (speed) => this.engine?.rattle(speed);
      this.legs.road = { area: road, drive: this.drive, truck };
      this.life.addArea('road', { flood: road.headlights.set, ground: (x, z) => surfaceY(x - ROAD_ORIGIN.x, z - ROAD_ORIGIN.z) + ROAD_ORIGIN.y,
        blocked: (x, z) => road.obstacles.some((o) => o.kind === 'circle' ? Math.hypot(x - o.x, z - o.z) < o.r + 0.4 : inBoxes([o], x, z)) || (!!this.dinerWalk && inBoxes(this.dinerWalk.colliders, x, z)),
        onHighway: (x) => Math.abs(x - ROAD_ORIGIN.x) < 5,
        far: { toWorld: (x, z) => new THREE.Vector3(x + ROAD_ORIGIN.x, 0, z + ROAD_ORIGIN.z), toMap: (x, z) => ({ x: x - ROAD_ORIGIN.x, z: z - ROAD_ORIGIN.z }),
          ground: (x, z) => surfaceY(x, z) + ROAD_ORIGIN.y, centre: { x: 0, z: 1600 } } });
      this.d.applyQuality();   // (the tumbleweed's material too)
      this.onRoadLoaded?.(road);
    });
  }

  // The old road at dawn, its own truck lit by its own floods, and a controller for it. It
  // begins at the diner (on the road's map), so the diner and the road come first.
  private ensureOldRoad() {
    return this.once('oldroad', async () => {
      const [{ OldRoad, oldFlood, OLD_HEADLIGHTS }, { DriveController }, { Truck }] = await Promise.all([this.load('OldRoad'), this.load('Drive'), this.load('Truck'), this.load('engine'),
        loadArtFor(OLDROAD_ART), this.ensureDiner()]); // round 11: the road's own pictures come with it
      const truck = new Truck({ flood: oldFlood });
      this.onTruck?.(truck);
      truck.headlightFloods(oldFlood, OLD_HEADLIGHTS);
      truck.group.visible = false;
      this.d.scene.add(truck.group);
      const road = new OldRoad(ROAD_ORIGIN);
      road.group.visible = false;
      this.d.scene.add(road.group);
      this.neonSlots.old = [0, 1].map(() => addFlood(0, -999, 0, 0, 0, oldFlood));
      for (const i of this.neonSlots.old) oldFlood.skip.add(i);
      // the diner's walls, sign and rig, and its lot, as in the road area (ensureDiner)
      const o = DINER_ORIGIN, d = this.diner!;
      road.addPlace(d.colliders.filter((c) => c !== this.dinerCol), { minX: o.x + 3.6, maxX: o.x + 16, minZ: o.z - 16, maxZ: o.z + 16 }, o.y);
      const drive = new DriveController(this.d.camera, truck, road);
      drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      road.flora.add(this.coverOf(road.group.position));
      this.oldRoad = road; this.oldTruck = truck; this.oldDrive = drive;
      this.legs.roswell = { area: road, drive, truck };
      this.d.applyQuality();
      return road;
    });
  }
  /** The engine sound, for the chapter that stops it. */
  get engineSound() { return this.engine; }
  /** The diner's and the town's walls and walks, for the ground cover of an area at `o` (the road's,
   *  the old road's). The truck on the lot moves: not that one. */
  private coverOf(o: THREE.Vector3) {
    const at = (r: { minX: number; maxX: number; minZ: number; maxZ: number }) => ({ minX: r.minX - o.x, maxX: r.maxX - o.x, minZ: r.minZ - o.z, maxZ: r.maxZ - o.z });
    const w = this.dinerWalk!;
    return { blocks: w.colliders.filter((c) => c !== this.dinerCol).map(at), walks: w.zones.map(at) };
  }

  /** From the diner's lot out onto the old road (chapter six), driven the whole way: the truck
   *  where it was left on the lot, now the old road's (its area has the diner too), across the
   *  highway and out to C. ready() runs behind the black, before the picture comes back. */
  driveOldRoad(ready: () => void) {
    void this.ensureDiner().then(() => {
      // (a chapter started from the menu has the truck somewhere else)
      if (this.truckAt !== 'diner') this.placeTruck('diner');
      this.startTrip('diner', 'roswell', { leg: 'roswell', onStart: () => {
        this.oldRoad!.hitBack = false;
        this.oldControl = null;
        ready();
        if (firstTime('drive')) this.d.toast(this.d.touch() ? 'Left stick: throttle, brake and steering.' : 'W and S: throttle and brake (S at a stop: reverse). A and D: steer.', 4.5);
      } });
    }).catch((e: Error) => { console.error(e); this.d.toast('The diner could not be loaded. Check your connection and try the truck again.', 5); });
  }

  /** Load what an area needs before entering it (a restored save, or a drive). */
  async prepare(area: AreaId) {
    if (area === 'station01') await this.ensureStation();
    if (area === 'road') await this.ensureRoad();
    if (area === 'room6') await this.ensureRoom6();
    if (area === 'diner') await this.ensureDiner();
  }

  private ensureRoom6() {
    return this.once('room6', async () => {
      const { Room6 } = await this.load('Room6');
      const r = new Room6(ROOM6_ORIGIN.clone());
      r.group.visible = false;
      this.d.scene.add(r.group);
      this.room6 = r;
      this.d.applyQuality();
      this.onRoom6Loaded?.(r);
      return r;
    });
  }

  // The diner and its pictures (round 6 and 8). It stands in the road area (beside the highway,
  // 2 km south): the road loads with it, and SARO's truck is the road's when it stands here.
  private dinerPark: { minX: number; maxX: number; minZ: number; maxZ: number } | null = null;
  private ensureDiner() {
    return this.once('diner', async () => {
      // Caliche's pictures (round 14) come with it; one that does not is drawn in code
      const [{ Diner, dinerFlood }, { Town, townFlood }] = await Promise.all([this.load('Diner'), this.load('Town'), this.ensureRoad(), loadArtFor([...DINER_ART, 'oldAsphalt']), loadArtSome(TOWN_ART)]);
      this.road!.refreshOldRoad();   // the old road going off across from it, in its own asphalt (round 11)
      const d = new Diner(DINER_ORIGIN.clone(), true);
      d.setSurfaceArt(artTexture('dinerBooth', [2, 2]), artTexture('dinerWall', [0.5, 0.5]));
      d.group.visible = this.area === 'road' || this.area === 'diner';
      this.d.scene.add(d.group);
      const o = DINER_ORIGIN, a = d.anchors.truckPark;
      this.dinerCol = d.colliders.find((c) => Math.abs(c.minX - (o.x + 7.85)) < 0.01 && Math.abs(c.minZ - (o.z + 3.2)) < 0.01) ?? null;
      // its walls, the sign's posts and the rig hold the truck; the lot is gravel, and the truck is left on it
      this.road!.addPlace(d.colliders.filter((c) => c !== this.dinerCol), { minX: o.x + 3.6, maxX: o.x + 16, minZ: o.z - 16, maxZ: o.z + 16 }, o.y);
      this.neonSlots.road = [0, 1].map(() => addFlood(0, -999, 0, 0, 0, this.road!.headlights.set));
      for (const i of this.neonSlots.road) this.road!.headlights.set.skip.add(i);
      this.dinerPark = { minX: o.x + 5, maxX: o.x + 14, minZ: o.z - 4, maxZ: o.z + 14 };
      const px = new THREE.Mesh(new THREE.BoxGeometry(2.2, 1.9, 5.4), new THREE.MeshBasicMaterial({ visible: false }));
      px.position.set(a.x, o.y + 0.95, a.z); px.rotation.y = a.yaw; px.name = 'dinerTruck';
      this.d.scene.add(px);
      this.dinerTruckProxy = px;
      this.onDinerTruck?.(px);
      // Caliche down the highway from it: its buildings and posts hold the truck too
      const town = new Town(ROAD_ORIGIN);
      town.group.visible = d.group.visible;
      this.d.scene.add(town.group);
      this.road!.addObstacles(town.obstacles);
      // the light in the air in the town: cones under its street lamps (they go out at dawn, and
      // in event 8, and their cones with them), and the diner's neon in the haze
      const tl = sodiumGlows(town.group, (x, y, z, size) => Math.abs(size - 2.2) < 0.05 && y - surfaceY(x, z) > 5, (x, z) => surfaceY(x, z), (h) => h * 0.6);
      this.air.town = new LightCones(tl, town.group);
      this.air.cones.push(this.air.town);
      this.air.lamps.road = tl.map((L) => ({ p: new THREE.Vector3(L.x, L.y, L.z).add(ROAD_ORIGIN), r: 7, col: L.col, k: 1 }));
      for (const [x, y, z, c] of [[13, 2.2, -10, 0xff5341], [12.8, 1.8, -10, 0x52ddd7]] as const) this.air.lamps.road.push({ p: new THREE.Vector3(x, y, z).add(DINER_ORIGIN), r: 8, col: new THREE.Color(c), k: 1 });
      this.townSlots = [0, 1, 2].map(() => addFlood(0, -999, 0, 0, 0, this.road!.headlights.set));
      for (const i of this.townSlots) this.road!.headlights.set.skip.add(i);
      this.town = town;
      this.dinerWalk = { zones: [...d.zones, ...town.zones], colliders: [...d.colliders, ...town.colliders] };
      // no ground cover in their walls, thick at their feet, low where they are walked
      this.road!.flora.add(this.coverOf(ROAD_ORIGIN));
      this.diner = d;
      this.parkAt('diner');
      this.d.applyQuality();
      this.onDinerLoaded?.(d);
      // (the tumbleweeds in the town lit by its lamps, and on the diner's lot by the diner's)
      const R = ROAD_ORIGIN, inTown = (x: number, z: number) => x > R.x + TOWN.west && x < R.x + TOWN.east && z > TOWN.z0 && z < TOWN.z1;
      // at the feet of the street lamps' posts, the end of the pump island under the canopy, the
      // mouths of the alleys off the main street, the fronts across it
      this.life.addWeeds('road', townFlood, ([[-7.0, 2122.45, 0.9], [-10.25, 2063.45, 0.8], [-17.0, 2112.4, 0.9], [-7.0, 2184.45, 0.85],
        [-7.0, 2030.45, 0.7], [-17.2, 2140.4, 0.8], [23.65, 2131.5, 0.9], [-7.0, 2150.5, 0.6]] as [number, number, number][]).map(([x, z, k]) => [x + R.x, z, k]), inTown);
      this.life.addWeeds('road', dinerFlood, [], (x, z) => Math.hypot(x - DINER_ORIGIN.x, z - DINER_ORIGIN.z) < 40);
      // moths round Caliche's street lamps, the gas station's canopy and the porch lights; bats at
      // the street lamp by the laundromat, and one at the diner's lot
      this.life.addMoths('road', lifeLamps([town.group, d.group], town.group, (w) => surfaceY(w.x - R.x, w.z - R.z) + R.y, (_p, _s, h) => h > 1.5), town.group,
        { bats: [[new THREE.Vector3(R.x - 3.9, 15, 2122), 3], [new THREE.Vector3(8000.5, 10.8, 2000), 1]], walls: this.d.scene });
      this.onTownLoaded?.(town);
      return d;
    });
  }

  // ---------- areas ----------
  /** Show one area and walk in it. The area must be prepared. */
  enter(area: AreaId) {
    const { player, saro } = this.d;
    this.area = area;
    for (const g of saro.groups) g.visible = area === 'saro';
    // a truck shows where it stands, or where it is being driven
    const here = (id: LegId) => area === id && (this.truckAt === id || this.leg === id);
    // (on the motel's lot it is SARO's truck too, in SARO's area, as parkAt has it)
    if (this.saroTruck) this.saroTruck.group.visible = here('saro') || (area === 'saro' && this.truckAt === 'motel');
    if (this.site) this.site.group.visible = area === 'station01';
    // the diner stands in the road area: each is seen from the other
    const onRoad = area === 'road' || area === 'diner';
    if (this.road) this.road.group.visible = onRoad;
    if (this.truck) this.truck.group.visible = onRoad && (this.leg === 'road' || this.truckAt === 'diner');
    if (this.parked) this.parked.group.visible = here('station01');
    if (this.room6) this.room6.group.visible = area === 'room6';
    if (this.diner) this.diner.group.visible = onRoad || area === 'roswell';
    if (this.town) this.town.group.visible = onRoad || area === 'roswell';
    if (this.oldRoad) this.oldRoad.group.visible = area === 'roswell';
    if (this.oldTruck) this.oldTruck.group.visible = area === 'roswell';
    const cam = this.d.camera, far = area === 'roswell' ? FAR_OLDROAD : FAR;
    if (cam.far !== far) { cam.far = far; cam.updateProjectionMatrix(); }
    const fog = this.d.scene.fog as THREE.FogExp2 | null;
    if (fog) fog.density = area === 'roswell' ? FOG_OLDROAD : FOG;
    player.floor = area === 'saro' ? this.grounds.floorAt : area === 'diner' ? this.town?.floorAt ?? (() => DINER_ORIGIN.y) : null;
    if (area === 'saro') { player.zones = saro.zones; player.colliders = saro.colliders; }
    if (area === 'station01' && this.site) { player.zones = this.site.zones; player.colliders = this.site.colliders; }
    if (area === 'room6' && this.room6) { player.zones = this.room6.zones; player.colliders = this.room6.colliders; }
    if (area === 'diner' && this.diner) { player.zones = this.dinerWalk?.zones ?? this.diner.zones; player.colliders = this.dinerWalk?.colliders ?? this.diner.colliders; }
  }
  /** How the sky is turned in the current area: the areas lie on one map (geo.ts), and the
   *  station's terms are the road's turned a quarter round, so its sky is turned with them,
   *  or the stars would swing round where the truck comes in. */
  get skyYaw() { return this.area === 'station01' ? STATION_TURN : 0; }
  /** Inside a building of the current area (for the sound of the space). */
  indoors(p: THREE.Vector3) {
    if (this.area === 'room6') return !!this.room6 && p.z < ROOM6_ORIGIN.z + 3.0;   // in the room, not on the walk outside
    if (this.area === 'diner') return !!this.diner && this.diner.zones.some((z) => (z.id === 'inside' || z.id === 'phone') && p.x >= z.minX && p.x <= z.maxX && p.z >= z.minZ && p.z <= z.maxZ);
    if (this.area !== 'station01' || !this.site) return false;
    const b = this.site.hutBounds;
    return p.x >= b.minX && p.x <= b.maxX && p.z >= b.minZ && p.z <= b.maxZ;
  }
  /** Put the player on the walk outside room 6, facing the open door. */
  placeAtRoom6() {
    const a = this.room6!.anchors.arrive;
    this.d.player.place(a.x, a.z, a.yaw);
  }
  /** Put the player outside room 6 on the motel's front, back in SARO's scene. */
  placeOutsideRoom6() {
    const a = this.court.anchors.fromRoom6;
    this.d.player.place(a.x, a.z, a.yaw);
  }

  /** Put the player at one of the diner's anchors (by the truck when they arrive). */
  placeAtDiner(at: 'arrive' | 'outside' | 'inside' | 'stool' | 'phone' = 'arrive') {
    const a = this.diner!.anchors[at];
    this.d.player.place(a.x, a.z, a.yaw);
  }
  /** To the diner (a fade; it loads the first time). The chapter on the Roswell road calls
   *  this when the truck pulls in; until then only the tests and ?debug use it. */
  goDiner(at: 'arrive' | 'outside' | 'inside' | 'stool' | 'phone' = 'arrive') {
    if (this.busy) return Promise.resolve();
    this.busy = true;
    const { fade, hold } = this.d;
    hold(true);
    fade(true, '');
    return Promise.all([this.ensureDiner(), new Promise((r) => setTimeout(r, 500))]).then(() => {
      this.enter('diner');
      this.placeAtDiner(at);
    }).finally(() => { this.busy = false; hold(false); fade(false); });
  }

  // ---------- room 6 ----------
  /** Through the door of room 6 (a short fade; the room loads the first time). */
  goRoom6() {
    if (this.busy) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, '');
    Promise.all([this.ensureRoom6(), new Promise((r) => setTimeout(r, 500))]).then(() => {
      this.enter('room6');
      this.placeAtRoom6();
      this.busy = false;
      this.onEnterRoom6?.();
      hold(false);
      fade(false);
    }).catch((e: Error) => {
      console.error(e);
      this.busy = false;
      hold(false);
      fade(false);
      toast('Room 6 could not be loaded. Check your connection and try the door again.', 5);
    });
  }
  /** Out of room 6 onto the motel's front. */
  leaveRoom6() {
    if (this.busy) return;
    const { fade, hold } = this.d;
    hold(true);
    fade(true, '');
    this.d.after(0.6, () => {
      this.enter('saro');
      this.placeOutsideRoom6();
      this.onLeaveRoom6?.();
      hold(false);
      fade(false);
    });
  }

  /** Put the player at the station's arrival point. */
  placeAtStation() {
    const a = this.site!.anchors.arrive;
    this.d.player.place(a.x, a.z, a.yaw);
  }

  // ---------- the drive ----------
  /** From the truck at SARO to STATION 01: out of the yard, south on the highway, the
   *  survey track to the gate, driven the whole way. Parked on the pad, the player gets out. */
  driveOut(onTheRoad = false) {
    const go = () => this.startTrip(onTheRoad ? 'road' : this.truckAt ?? 'saro', 'station01', { at: onTheRoad ? this.road!.start : undefined,
      hint: 'Leave the truck on the gravel outside the gate: stop there.', onArrive: () => this.onArriveStation?.() });
    if (onTheRoad) void this.ensureRoad().then(go); else go();
  }

  /** Get in where the truck stands and drive anywhere (Tom 15 October: the truck any time).
   *  Every road and the diner are built first, behind the black; the truck is left on any
   *  place it is stopped on once it has been driven off the one it stood on. */
  roam(from: LegId) {
    this.startTrip(from, null, { legs: ['saro', 'road', 'station01'], places: ['diner'], onStart: () => {
      // the controls the first time on this device, where the truck can be left the first drive of one's own
      const say = [firstTime('drive') ? this.d.touch() ? 'Left stick: drive. Radio: the radio. Stop, and Get out takes you out.' : 'W, S, A and D: drive. R: radio. Stop and press E to get out.' : '',
        firstTime('roam') ? 'On the highway, stop at the motel or the diner.' : ''].filter(Boolean).join(' ');
      if (say) this.d.toast(say, 5.5);
    } });
  }

  /** From the truck on SARO's pad to Mesa Diner where the old road leaves the highway
   *  (chapter five), driven the whole way: out of the yard and 2 km south. The night's clock
   *  goes on faster for every metre driven, so the drive takes the story's nine minutes or so
   *  at highway speed, and not much more when it is taken slowly. */
  driveToDiner(onArrive: () => void) {
    this.startTrip(this.truckAt ?? 'saro', 'diner', { perMetre: 0.19, onArrive,
      hint: 'Mesa Diner is the lit sign on the right. Leave the truck on its lot: stop there.' });
  }

  /** From the truck to Sierra Motor Court's lot, 650 m down the highway (chapter four). */
  driveToMotel(onArrive: () => void) {
    this.startTrip(this.truckAt ?? 'saro', 'motel', { onArrive,
      hint: 'Sierra Motor Court is the lit sign on the right. Leave the truck on its lot by the office: stop there.' });
  }

  /** From the truck at the station (or wherever it was left) back to SARO, the whole way, onto the pad. */
  driveBack() {
    this.startTrip(this.truckAt ?? 'station01', 'saro', { hint: 'The truck goes back on its pad by the service yard: up the ramp, and stop there.',
      onArrive: () => { this.onArriveSaro?.(); this.d.toast(`${this.d.clock()}. The truck is back on its pad.`, 3); } });
  }

  // ---------- trips ----------
  // The truck is driven from one place to another the whole way (drive/legs.ts). Each area it
  // passes through is a leg with its own copy of the truck and its own controller; where two
  // areas meet on a road the truck is handed from one to the next between two frames.
  private trucksHome: { t: Truck; x: number; y: number; z: number; h: number; p: number; r: number }[] = [];

  /** SARO's own ground to drive on: the pad, the ramp, the service road, the highway. */
  private ensureSaroLeg() {
    return this.once('saroLeg', async () => {
      const [{ DriveController }, { SaroDrive }] = await Promise.all([this.load('Drive'), this.load('legs'), this.load('engine'), loadArtFor(['gravelTrack'])]);
      this.saroGlints?.trackArt();   // the survey track's mouth in its own gravel, as the road area has it
      const t = this.saroTruck!;
      const area = new SaroDrive(this.d.saro.colliders.filter((c) => c !== this.d.saro.truckCol), this.grounds.floorAt, this.saroRoad);
      area.headlights = this.headlightSlots(siteFlood);
      const drive = new DriveController(this.d.camera, t, area);
      drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      this.legs.saro = { area, drive, truck: t };
    });
  }
  private ensureLeg(id: LegId): Promise<unknown> {
    return id === 'saro' ? this.ensureSaroLeg() : id === 'road' ? this.ensureRoad() : id === 'station01' ? this.ensureStation()
      : id === 'diner' ? this.ensureDiner() : this.ensureOldRoad();
  }
  private truckOf(id: LegId) { return id === 'saro' || id === 'motel' ? this.saroTruck : id === 'station01' ? this.parked : id === 'diner' ? this.truck : null; }

  /** The parked truck's collider and its handle for "use" go where the truck stands, or out
   *  of the way while it is gone. (At the motel it is SARO's truck, collider and handle.) */
  private parkAt(id: LegId) {
    if (id === 'motel') id = 'saro';
    const t = this.truckOf(id);
    if (!t) return;
    const here = this.truckAt === id || (id === 'saro' && this.truckAt === 'motel');
    const col = id === 'saro' ? this.d.saro.truckCol : id === 'station01' ? this.site?.truckCollider : id === 'diner' ? this.dinerCol : null;
    const proxy = id === 'saro' ? this.d.saro.truckProxy : id === 'station01' ? this.site?.proxies.truckSpot : id === 'diner' ? this.dinerTruckProxy : null;
    const g = t.group, h = g.rotation.y, c = Math.abs(Math.cos(h)), sn = Math.abs(Math.sin(h));
    const hx = 1.05 * c + 2.75 * sn, hz = 1.05 * sn + 2.75 * c;
    if (col) Object.assign(col, here ? { minX: g.position.x - hx, maxX: g.position.x + hx, minZ: g.position.z - hz, maxZ: g.position.z + hz }
      : { minX: 1e9, maxX: 1e9, minZ: 1e9, maxZ: 1e9 });
    if (proxy) {
      const parent = proxy.parent;
      parent?.updateMatrixWorld();
      const at = parent ? parent.worldToLocal(g.position.clone()) : g.position.clone();
      proxy.position.set(at.x, here ? at.y + 0.95 : -500, at.z);   // the diner's lot is 4 m up
      proxy.rotation.y = h;
    }
  }

  /** Get in and drive from where the truck stands to another place, the whole way.
   *  to null: drive anywhere; at: start there, not where the truck stands; free: the trucks
   *  stay where they are and go back to their places afterwards (the developer menu);
   *  hint: said once on coming into the area where the truck is to be left; leg: start in this
   *  leg's area, with its truck where the place's truck stands (the old road from the diner). */
  startTrip(from: LegId, to: LegId | null, o: { perMetre?: number; onArrive?: () => void; onStart?: () => void; at?: { pos: THREE.Vector3; heading: number };
    legs?: LegId[]; free?: boolean; hint?: string; leg?: LegId; places?: LegId[] } = {}) {
    if (this.busy || this.driving) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, '');
    this.load('legs').then(({ legsBetween }) => {
      const need = o.legs ?? (to ? legsBetween(from, to) : [from]);
      const places = [from, to, ...(o.places ?? [])].filter((p): p is LegId => p === 'diner').slice(0, 1).map(() => this.ensureDiner());
      return Promise.all([...need.map((l) => this.ensureLeg(l)), ...places, this.load('engine'), new Promise((r) => setTimeout(r, 400))]);
    }).then(() => {
      // a place can stand in another's area (the diner in the road's): the trip starts in that leg
      const leg0 = o.leg ?? this.mods.legs!.legOf(from), L = this.legs[leg0]!;
      const g = (this.truckOf(from) ?? L.truck).group;
      // where every parked truck stands now, so a trip that is given up puts them back
      this.trucksHome = TRUCKS.map((id) => this.truckOf(id)).filter((t): t is Truck => !!t)
        .map((t) => ({ t, x: t.group.position.x, y: t.group.position.y, z: t.group.position.z, h: t.group.rotation.y, p: t.group.rotation.x, r: t.group.rotation.z }));
      this.trip = { from, to, perMetre: o.perMetre ?? 0, onArrive: o.onArrive, still: 0, home: this.truckAt, hint: o.hint,
        free: !!o.free, start: (o.at?.pos ?? g.position).clone(), left: false, secs: 0, metres: 0, n: ++this.tripNo };
      this.leg = leg0;
      if (!o.free) { this.truckAt = null; for (const id of TRUCKS) this.parkAt(id); }
      L.drive.place(o.at?.pos ?? g.position.clone(), o.at?.heading ?? g.rotation.y);
      this.lightsOn(L.truck);
      this.enter(areaOf(leg0));
      this.startEngine();
      this.driving = true;
      // after a time away (story/abduction.ts) the engine has died where it stood: the key
      if (this.keyStall) { this.keyStall = false; this.trouble.start('stall', { key: true }); }
      this.onGetIn?.();
      Object.assign(this.auto, { rev: false, stall: 0, back: 0, stuck: 0 });
      if (o.onStart) o.onStart();
      else if (from === to && o.hint) toast(o.hint, 5);
      // how to drive, the first time on this device (the notebook's Controls tab has it after)
      else if (firstTime('drive')) toast(this.d.touch() ? 'Left stick: throttle, brake and steering. Radio: the radio. Stopped, Get out takes you out.' : 'W and S: throttle and brake (S at a stop: reverse). A and D: steer. R: radio. Stopped, E gets out.', 5);
    }).catch((e: Error) => {
      console.error(e);
      toast('The way could not be loaded. Check your connection and try the truck again.', 5);
    }).finally(() => { this.busy = false; hold(false); fade(false); });
  }
  private lightsOn(t: Truck) {
    t.setDriving(true); t.setHeadlights(true); t.setLightLevel(1); t.setHeadColor(null); t.setDashLevel(null);
  }

  // Every frame of a trip, after the truck has moved: on into the next area, or arrived.
  private tripStep(dt: number) {
    const id = this.leg!, L = this.legs[id]!, D = L.drive, legs = this.mods.legs;
    // (chapter six's old road is not a trip: the ways between areas may never have been loaded)
    if (!legs) return;
    const ex = legs.exitFrom(id, D.pos.x, D.pos.z, D.heading);
    if (ex && this.legs[ex.to]) { this.transfer(ex); return; }
    const trip = this.trip;
    if (!trip) return;
    trip.secs += dt;
    trip.metres += Math.abs(D.speed) * dt;
    if (trip.perMetre) this.d.skipClock(Math.abs(D.speed) * dt * trip.perMetre);
    const here = !!trip.to && legs.legOf(trip.to) === id, P = trip.to ? this.parkBox(trip.to) : null;
    // where to leave the truck, said once it is in sight
    if (trip.hint && here && P && Math.hypot(D.pos.x - (P.minX + P.maxX) / 2, D.pos.z - (P.minZ + P.maxZ) / 2) < 110) {
      this.d.toast(trip.hint, 5); trip.hint = undefined;
    }
    // the truck is left where it stops: on the trip's place, or (once it has been driven off the
    // one it stood on) on any place in this area; a free drive (the developer menu) on none
    if (!trip.left && (id !== legs.legOf(trip.from) || Math.hypot(D.pos.x - trip.start.x, D.pos.z - trip.start.z) > 12)) trip.left = true;
    const stop = trip.free || Math.abs(D.speed) >= 0.35 ? undefined : PLACES.find((p) => (p === trip.to || trip.left) && legs.legOf(p) === id && inBox(this.parkBox(p), D.pos));
    if (stop) {
      trip.still += dt;
      if (trip.still > 0.7) this.park(stop);
    } else trip.still = 0;
  }
  /** Where the truck is left at a place (world box). */
  private parkBox(place: LegId) {
    if (place === 'diner') return this.dinerPark;
    if (place === 'motel') return (this.legs.saro?.area as { motelPark?: { minX: number; maxX: number; minZ: number; maxZ: number } } | undefined)?.motelPark ?? null;
    return (this.legs[place]?.area as { park?: { minX: number; maxX: number; minZ: number; maxZ: number } | null } | undefined)?.park ?? null;
  }
  // Into the next area: its truck goes on from here, as fast and as turned as this one.
  private transfer(ex: Exit) {
    const from = this.legs[this.leg!]!, to = this.legs[ex.to]!;
    this.leg = ex.to;
    to.drive.adopt(from.drive, ex.x, ex.z, ex.heading);
    to.truck.copyCab(from.truck);
    this.trouble.handOver(from.truck, from.drive);   // the copy left behind is itself again
    this.dashT = 0;   // the clock on its dash is set this frame
    this.lightsOn(to.truck);
    from.truck.setDriving(false);
    this.enter(areaOf(ex.to));

  }
  // Stopped where the truck is left: the engine off, and the player gets out by the door.
  private park(place: LegId) {
    const id = this.leg!, L = this.legs[id]!, trip = this.trip;
    this.trouble.reset(L.truck, L.drive);
    this.driving = false;
    this.engine?.stop();
    L.truck.setDriving(false);
    L.truck.setHeadlights(false);
    this.truckAt = place; this.leg = null; this.trip = null; this.autopilot = null;
    this.parkAt(place);
    const { fade, hold } = this.d;
    hold(true);
    fade(true, '');
    this.d.after(0.4, () => {
      this.enter(areaOf(place));
      this.getOut(L.truck);
      hold(false);
      fade(false);
      if (trip?.to === place) trip.onArrive?.();
      this.onParked?.(place);
    });
  }
  private getOut(t: Truck) {
    const g = t.group, h = g.rotation.y, p = this.d.player;
    const v = this.doorSpot(t) ?? new THREE.Vector3(-1.75, 0, -0.3).applyAxisAngle(UP, h).add(g.position);
    p.place(v.x, v.z, h);
  }
  /** Ground to stand on by one of the truck's doors in the area's walkable zones, clear of
   *  everything in the way, or null. */
  private doorSpot(t: Truck) {
    const g = t.group, h = g.rotation.y, p = this.d.player, r = p.radius;
    for (const side of [-1, 1, -1.4, 1.4]) {
      const v = new THREE.Vector3(side * 1.75, 0, -0.3).applyAxisAngle(UP, h).add(g.position);
      if (p.walkable(v.x, v.z) && !p.colliders.some((c) => v.x + r > c.minX && v.x - r < c.maxX && v.z + r > c.minZ && v.z - r < c.maxZ)) return v;
    }
    return null;
  }

  /** The truck the player has, if it is in this area (its radio plays from the dash), or null.
   *  Not on the old road: chapter six has the cab radio there (Audio.cabRadio). */
  truckHere(): Truck | null {
    if (this.area === 'roswell' || this.leg === 'roswell') return null;
    if (this.driving) return this.leg ? this.legs[this.leg]?.truck ?? null : null;
    const at = this.truckAt;
    if ((at === 'saro' || at === 'motel') && this.area === 'saro') return this.saroTruck;
    if (at === 'station01' && this.area === 'station01') return this.parked;
    if (at === 'diner' && (this.area === 'diner' || this.area === 'road')) return this.truck;
    return null;
  }

  // ---------- getting out wherever the truck is stopped (Tom, 16 October) ----------
  /** The place a stop in this leg leaves the truck at (its parked truck is this leg's). */
  private placeOfLeg(id: LegId): LegId | null { return id === 'saro' || id === 'station01' ? id : id === 'road' ? 'diner' : null; }
  /** Whether the player can get out now: stopped, on a trip of the story's or their own, and
   *  with ground by a door (round SARO and STATION 01 anywhere the area can be walked; on the
   *  highway only at the diner and in Caliche, the road itself is not walked). E or Use gets out
   *  (main.ts). */
  canGetOut() {
    const id = this.leg, trip = this.trip;
    if (!this.driving || this.busy || !id || !trip || trip.free || this.trouble.wantsKey) return false;
    const L = this.legs[id], place = this.placeOfLeg(id);
    if (!L || !place || Math.abs(L.drive.speed) > 0.6) return false;
    if (id === 'road') {
      const b = this.dinerPark, q = L.drive.pos;
      if (this.town?.inTown(q.x, q.z)) return true;
      return !!b && q.x > b.minX - 6 && q.x < b.maxX + 6 && q.z > b.minZ - 6 && q.z < b.maxZ + 6;
    }
    return !!this.doorSpot(L.truck);
  }
  /** Out of the truck where it stands; the trip ends as if it had been left on a lot. */
  getOutHere() {
    if (!this.canGetOut()) return false;
    const id = this.leg!, q = this.legs[id]!.drive.pos;
    this.park(id === 'saro' && inBox(this.parkBox('motel'), q) ? 'motel' : this.placeOfLeg(id)!);
    return true;
  }

  // The autopilot (tests, the developer menu): along the way to the next area, or to where
  // the truck is left. Pure pursuit, slowed in time for the bends ahead; when the way lies
  // behind, a turn in three points (forward on full lock, back on the other lock when it can
  // go no further, and so on until it faces the way).
  private auto = { rev: false, stall: 0, back: 0, stuck: 0 };
  /** dt: the step (the stall timer of the turn); 0 to only look. */
  private autoInput(dt = 0): DriveInput {
    const id = this.leg!, L = this.legs[id]!, D = L.drive, trip = this.trip, legs = this.mods.legs!;
    const next = !trip?.to ? null : legs.legOf(trip.to) === id ? 'park' : legs.legsBetween(id, trip.to)[1];
    const roadTo = next === 'park' ? trip!.to! : next as LegId;
    // to the motel's lot: from the north, or from the south once past it
    const key = next === 'park' && trip!.to === 'motel' ? (D.pos.z > MOTEL_RECT.minZ + 20 ? 'motelS' : 'motel') : next;
    const route = key ? (id === 'road' ? legs.roadRoute(roadTo, D.pos.z - ROAD_ORIGIN.z) : (L.area as { routes?: Partial<Record<string, THREE.Vector3[]>> }).routes?.[key]) : null;
    if (!route?.length) return { steer: 0, throttle: D.speed > 0.2 ? -0.6 : 0 };
    let k = 0, best = Infinity;
    for (let i = 0; i < route.length; i++) { const d = route[i].distanceToSquared(D.pos); if (d < best) { best = d; k = i; } }
    const sp = Math.abs(D.speed), look = THREE.MathUtils.clamp(3.5 + sp * 0.7, 4, 16);
    let j = k;
    while (j < route.length - 1 && route[j].distanceTo(D.pos) < look) j++;
    const tg = route[j];
    let err = Math.atan2(-(tg.x - D.pos.x), -(tg.z - D.pos.z)) - D.heading;
    err = Math.atan2(Math.sin(err), Math.cos(err));
    const A = this.auto;
    // ---------- stuck against something: back off a little, then on ----------
    if (A.back > 0) {
      A.back -= dt;
      if (!D.reversing) return { steer: 0, throttle: -1 };
      return { steer: err > 0 ? 0.6 : -0.6, throttle: sp < 1.2 ? -0.6 : 0 };
    }
    // ---------- turning round ----------
    if (Math.abs(err) > 0.9 || A.rev) {
      const want = A.rev ? -1 : 1, moving = D.reversing === A.rev && sp > 0.25;
      A.stall = moving ? 0 : A.stall + dt;
      if (A.rev && (Math.abs(err) < 0.6 || A.stall > 1.2)) { A.rev = false; A.stall = 0; }
      else if (!A.rev && A.stall > 1.2 && Math.abs(err) > 0.9) { A.rev = true; A.stall = 0; }
      const lock = err > 0 ? 1 : -1;
      if (A.rev) return { steer: lock, throttle: D.reversing ? (sp < 1.8 ? -0.7 : 0) : -1 };
      if (D.reversing) return { steer: -lock, throttle: 1 };
      return { steer: -lock, throttle: THREE.MathUtils.clamp((2 * want - D.speed) * 0.6, -1, 0.7) };
    }
    A.stall = 0;
    if (D.reversing) return { steer: 0, throttle: 1 };
    // ---------- along the way: the speed each bend ahead allows, braked to in time ----------
    let v = Math.min(this.autopilot ?? 15, D.surface.top * 0.7);
    let dist = route[k].distanceTo(D.pos);
    for (let i = k; i < route.length - 2 && dist < 90; i++) {
      const p0 = route[i], p1 = route[i + 1], p2 = route[i + 2];
      const h1 = Math.atan2(p1.x - p0.x, p1.z - p0.z), h2 = Math.atan2(p2.x - p1.x, p2.z - p1.z);
      const bend = Math.abs(Math.atan2(Math.sin(h2 - h1), Math.cos(h2 - h1)));
      const curv = bend / Math.max((p1.distanceTo(p0) + p2.distanceTo(p1)) / 2, 0.5);
      const vb = Math.sqrt(2.2 / Math.max(curv, 1e-4));       // about 2.2 m/s² sideways
      v = Math.min(v, Math.sqrt(vb * vb + 2 * 2.5 * dist));   // braking at 2.5 m/s²
      dist += p1.distanceTo(p0);
    }
    const end = next === 'park' ? route[route.length - 1].distanceTo(D.pos) : Infinity;
    v = Math.min(v, Math.max(0, (end - 2.2) * 0.6)) * THREE.MathUtils.clamp(1.15 - Math.abs(err) * 1.4, 0.3, 1);
    if (end < 2.2) v = 0;
    // wanting to go but not going (a post, a kerb): back off for a moment
    A.stuck = v > 0.8 && sp < 0.15 ? A.stuck + dt : 0;
    if (A.stuck > 1.5) { A.stuck = 0; A.back = 1.4; }
    // pure pursuit: the curvature that reaches the aim point, turned into the wheel the truck
    // has at this speed on this ground (the same lock as the physics, Drive.ts)
    const kappa = 2 * Math.sin(err) / look;
    return { steer: D.steerFor(-kappa), throttle: THREE.MathUtils.clamp((v - D.speed) * 0.5, -1, 1) };
  }

  private startEngine() {
    const a = this.d.audio();
    if (!a.ctx) return;
    if (!this.engine) {
      const { EngineSound } = this.mods.engine ?? {} as Modules['engine'];
      if (!EngineSound) { void this.load('engine').then(() => { if (this.driving) this.startEngine(); }); return; }
      this.engine = new EngineSound(a.ctx, a.sfx);
    }
    this.engine.start();
  }

  /** Stop driving without arriving (a save is opened, or the title). A trip that is given up
   *  puts the trucks back where they stood when it began. */
  stopDriving() {
    const id = this.leg, trip = this.trip;
    this.trouble.reset(id ? this.legs[id]?.truck : null, id ? this.legs[id]?.drive : null);
    this.driving = false;
    this.oldControl = null;
    this.engine?.stop();
    if (id) { this.legs[id]?.truck.setDriving(false); this.legs[id]?.truck.setHeadlights(false); }
    if (trip) {
      for (const h of this.trucksHome) { h.t.group.position.set(h.x, h.y, h.z); h.t.group.rotation.set(h.p, h.h, h.r, 'YXZ'); h.t.setDriving(false); }
      this.truckAt = trip.home;
    }
    this.leg = null; this.trip = null; this.autopilot = null;
    for (const p of TRUCKS) this.parkAt(p);
    this.free = null; this.freeBack = null;
  }

  /** Where a save finds the truck (main.ts): the place it was left, exactly how it stands there,
   *  and the area that is in. Out on a trip it is where the truck stood when the trip began: a
   *  trip given up puts it back there (stopDriving), and so does the save when it is opened. */
  truckSave(): { at: LegId; area: AreaId; pose: TruckPose } | null {
    const at = this.trip ? this.trip.home : this.truckAt, t = at ? this.truckOf(at) : null;
    if (!at || !t) return null;
    const home = this.trip ? this.trucksHome.find((h) => h.t === t) : null, g = t.group;
    const pose = home ? { at, x: home.x, y: home.y, z: home.z, h: home.h, p: home.p, r: home.r }
      : { at, x: g.position.x, y: g.position.y, z: g.position.z, h: g.rotation.y, p: g.rotation.x, r: g.rotation.z };
    return { at, area: areaOf(at), pose };
  }

  /** The truck stands where the story has left it (a new night or a restored save): on
   *  SARO's pad, on the motel's lot, outside STATION 01's gate or on the diner's lot; with
   *  a pose from a save, exactly where it was left there (on the grounds, in Caliche's street). */
  placeTruck(at: LegId, pose?: TruckPose | null) {
    this.stopDriving();
    this.truckAt = at;
    const put = (t: Truck | null, x: number, z: number, h: number, y = 0) => { if (t) { t.group.position.set(x, y, z); t.group.rotation.y = h; t.setDriving(false); } };
    const p = this.d.saro.truck;
    // on the motel's lot, nosed in towards the office
    if (at === 'motel') put(this.saroTruck, -35.3 + MOTEL_AT.x, 9.5 + MOTEL_AT.z, Math.PI / 2, MOTEL_AT.y);
    else put(this.saroTruck, p.x, p.z, p.heading);
    const a = this.site?.anchors.truck;
    if (a) put(this.parked, a.x, a.z, a.heading);
    // at the diner it is the road's truck that stands on the lot
    const b = this.diner?.anchors.truckPark;
    if (b && at === 'diner') put(this.truck, b.x, b.z, b.yaw, DINER_ORIGIN.y);
    // a save from 18 October 2026 on knows the very spot: only for the place it is at, and only
    // within reach of that place (the areas lie kilometres apart), or it goes on the place as before
    const t = this.truckOf(at), ok = (v: unknown): v is number => typeof v === 'number' && Number.isFinite(v);
    if (t && pose && pose.at === at && ok(pose.x) && ok(pose.y) && ok(pose.z) && ok(pose.h)
      && Math.hypot(pose.x - t.group.position.x, pose.z - t.group.position.z) < 1500) {
      t.group.position.set(pose.x, pose.y, pose.z);
      t.group.rotation.set(ok(pose.p) ? pose.p : 0, pose.h, ok(pose.r) ? pose.r : 0, 'YXZ');
    }
    for (const id of TRUCKS) this.parkAt(id);
  }

  // ---------- free drive (the developer menu): the truck on a road, any time, no story ----------
  /** Which road the truck is out on in a free drive, or null. */
  free: 'road' | 'oldroad' | null = null;
  private freeBack: { area: AreaId; x: number; z: number; yaw: number; pitch: number } | null = null;
  /** Out on the highway (and on to SARO and STATION 01, the whole way) or on the old Roswell
   *  road, from wherever the player is. Nothing in the story happens on the way, and the
   *  truck is not left anywhere; endFreeDrive() puts everything back. */
  freeDrive(which: 'road' | 'oldroad') {
    if (this.busy) return;
    const { player } = this.d;
    const back = this.freeBack ?? { area: this.area, x: player.pos.x, z: player.pos.z, yaw: player.yaw, pitch: player.pitch };
    if (this.driving) this.stopDriving();
    this.freeBack = back;
    const leg: LegId = which === 'road' ? 'road' : 'roswell';
    const ready = which === 'road' ? this.ensureRoad() : this.ensureOldRoad();
    void ready.then(() => {
      const start = which === 'road' ? this.road!.start : this.oldRoad!.start;
      if (this.oldRoad) this.oldRoad.hitBack = false;
      this.startTrip(leg, null, { at: start, free: true, legs: which === 'road' ? ['saro', 'road', 'station01'] : ['roswell'], onStart: () => {
        this.free = which; this.freeBack = back;
        this.d.toast('Free drive. The developer menu takes you back.', 3.5);
      } });
    }).catch((e: Error) => { console.error(e); this.d.toast('That road could not be loaded.', 4); });
  }
  /** Out of the free drive: back where the player stood before it. */
  endFreeDrive() {
    const back = this.freeBack;
    if (!this.free || !back) return;
    this.stopDriving();
    this.enter(back.area);
    this.d.player.place(back.x, back.z, back.yaw);
    this.d.player.pitch = back.pitch;
  }

  // ---------- per frame ----------
  update(dt: number, t: number, input: { steer: number; throttle: number } | null, look: { x: number; y: number }) {
    if (this.area === 'station01') this.site?.update(dt, t);
    if (this.area === 'room6') this.room6?.update(dt, t);
    if (this.area === 'diner' || this.area === 'road' || this.area === 'roswell') { this.diner?.update(dt, t); this.town?.update(t, this.d.time()); }
    // on the old road the diner and the town are there behind the truck until they are far off in
    // the haze, lit by the road's dawn
    if (this.area === 'roswell' && this.diner) {
      const c = this.d.camera.position, near = Math.hypot(c.x - DINER_ORIGIN.x, c.z - DINER_ORIGIN.z) < 2600;
      this.diner.group.visible = near;
      if (this.town) this.town.group.visible = near;
      this.diner.setNear(0);
    }
    if (this.area === 'saro') { this.motel.update(dt, t); this.field.update(dt, t, this.d.camera.position); }
    // SARO's stretch of the highway: the reflectors and signs in the truck's headlights
    const sl = this.legs.saro?.area.headlights;
    if (this.area === 'saro' && this.leg === 'saro' && sl) this.saroGlints?.glints(sl.slots);
    if (this.area === 'roswell') this.oldRoad?.update(dt, t, this.d.camera.position, this.d.sky());
    if ((this.area === 'road' || this.area === 'diner') && this.road && this.drive) this.road.update(dt, t, this.drive.pos);
    this.drivingStep(dt, input, look);
    // the herd south of the motel in the area shown, in the headlights of the truck driven there
    // (after the step, so the beam is where the truck is in this frame)
    const herdArea = this.area === 'saro' ? 'saro' : this.area === 'road' || this.area === 'diner' ? 'road' : null;
    const lamps = this.driving && this.leg === herdArea ? (herdArea === 'saro' ? this.legs.saro?.area.headlights : this.road?.headlights) : undefined;
    for (const h of this.herds) if (h.area === herdArea) h.update(t, lamps ?? null);
    // the dawn's haze on the far mesas (horizon.ts), as on the land in front of them; none at night
    if (this.area !== 'roswell') {
      const sk = this.d.sky();
      // the same haze as the land's (main.ts darkens it a little at dawn out here)
      const fog = this.d.scene.fog as THREE.FogExp2 | null;
      horizonHaze.color.value.copy(fog?.color ?? sk.fog);
      horizonHaze.k.value = Math.min(0.9, sk.dawn * 3);
      horizonHaze.density.value = (this.d.scene.fog as THREE.FogExp2 | null)?.density ?? FOG;
    }
    // the diner's own sky light near it, the road's night light up the road (both in one scene)
    if ((this.area === 'road' || this.area === 'diner') && this.diner && this.road) {
      const c = this.d.camera.position, w = 1 - THREE.MathUtils.smoothstep(Math.hypot(c.x - DINER_ORIGIN.x, c.z - DINER_ORIGIN.z), 120, 520);
      this.diner.setNear(w);
      this.road.setNight(1 - w);
    }
    // out on the track in the truck the station's moonlight gives way to the road's
    // (Station01.nightBlend); after the step, so it is right in the frame the truck comes in
    if (this.area === 'station01' && this.site) {
      const L = this.leg === 'station01' ? this.legs.station01 : null;
      const d = L ? Math.hypot(L.drive.pos.x - STATION_ORIGIN.x, L.drive.pos.z - STATION_ORIGIN.z - 8) : 0;
      this.site.nightBlend(1 - THREE.MathUtils.smoothstep(d, 110, 290));
    }
    this.airStep(dt, t);
  }
  /** The light in the air (airlight.ts), every frame (also behind the title screen, main.ts):
   *  High and Ultra only, at night, and the haze only out in the open of SARO, the road, the
   *  diner's lot and the town, and STATION 01. */
  airStep(dt: number, t: number) {
    const a = this.air, on = !this.airOff && getQuality() !== 'low', night = 1 - THREE.MathUtils.smoothstep(this.d.sky().dawn, 0.12, 0.5);
    for (const c of a.cones) c.update(on, AIR_CONES * night);
    const c = this.d.camera.position, fog = this.d.scene.fog as THREE.FogExp2 | null;
    let ground: number | null = null, lamps: HazeLamp[] = [];
    if (on && night > 0 && this.airOut()) {
      // SARO's own ground is flat; south of it the land is the road's (Grounds.floorAt), and the
      // slabs and floors at 0 stand above the haze
      if (this.area === 'saro') { ground = c.z > WEST.z1 ? this.grounds.floorAt(c.x, c.z) : -0.62; lamps = a.lamps.saro; }
      else if (this.area === 'road' || this.area === 'diner') {
        ground = surfaceY(c.x - ROAD_ORIGIN.x, c.z - ROAD_ORIGIN.z) + ROAD_ORIGIN.y; lamps = a.lamps.road;
        a.town?.lamps.forEach((L, i) => { if (lamps[i]) lamps[i].k = L.k(); });
      } else if (this.area === 'station01' && this.site) ground = this.site.groundAt(c.x, c.z);
    }
    a.haze?.update(dt, t, c, ground, AIR_HAZE * night, fog?.color ?? new THREE.Color(), lamps);
    // and the life in it
    this.lifeStep(dt, t);
  }
  /** The life in the night (nightlife.ts), every frame with the light in the air (tests call it too). */
  lifeStep(dt: number, t: number) {
    const L = this.driving && this.leg ? this.legs[this.leg] : undefined, D = L?.drive;
    this.life.update(dt, t, { area: this.area, cam: this.d.camera, clock: this.d.time(), dawn: this.d.sky().dawn, out: this.airOut(), quiet: this.lifeQuiet(),
      foot: !this.driving, flash: this.lifeBeam(),
      truck: L && D ? { group: L.truck.group, speed: D.speed, kind: D.surface.kind, beam: L.truck.beam } : null });
  }
  /** Each area lights its own materials from its own lamps: the truck's headlights go to the
   *  diner's, and the diner's neon to the road's and the old road's. main.ts calls it after the
   *  step (the headlights are where the truck is now) and after the PC tier has taken the lamps
   *  it makes real lights of (a neon lamp it holds is off here, and its real light lights all). */
  shareLights() {
    // the truck's headlights on the motel when it is driven at SARO (its lamps are its own set)
    const sh = this.area === 'saro' && this.leg === 'saro' ? this.legs.saro?.area.headlights : undefined;
    this.motel.shine(sh?.set ?? null, sh?.slots ?? [], HEAD_ON_MOTEL);
    if (!this.diner) return;
    const hl = this.area === 'roswell' ? this.oldRoad?.headlights : this.area === 'road' || this.area === 'diner' ? this.road?.headlights : undefined;
    this.diner.shine(hl?.set ?? null, hl?.slots ?? [], HEAD_ON_DINER);
    this.town?.shine(hl?.set ?? null, hl?.slots ?? [], HEAD_ON_TOWN);
    if (this.road && this.town) this.town.lampsTo(this.road.headlights.set, this.townSlots, this.d.camera.position, TOWN_ON_ROAD);
    if (this.road) this.diner.neon(this.road.headlights.set, this.neonSlots.road, NEON_REACH, NEON_ON_ROAD);
    if (this.oldRoad) this.diner.neon(this.oldRoad.headlights.set, this.neonSlots.old, NEON_REACH, NEON_ON_ROAD);
  }
  // the truck being driven: by the keys, by chapter six, by the autopilot or by a test
  private drivingStep(dt: number, input: { steer: number; throttle: number } | null, look: { x: number; y: number }) {
    const id = this.leg, L = id && id === this.area ? this.legs[id] : null;
    if (!L) return;
    const c = id === 'roswell' ? this.oldControl : null;
    if (c?.camera) c.camera(dt);
    else if (this.driving) {
      if (c?.input) input = c.input(dt);
      else if (this.autopilot !== null) input = this.autoInput(dt);
      else if (this.testInput) input = typeof this.testInput === 'function' ? this.testInput() : this.testInput;
      L.drive.update(dt, input ?? { steer: 0, throttle: 0 }, look);
      this.tripStep(dt);
    }
    const N = (this.leg && this.legs[this.leg]) || L, D = N.drive;
    // trouble with the truck, on the story's trips and the player's own (not chapter six's road)
    if (this.driving && this.trip && id !== 'roswell') this.trouble.step(dt, D, N.truck, this.engine, this.troubleOk?.() ?? false);
    if (this.engine) { this.engine.set(D.rpm, D.load); this.engine.tyres(D.surface.kind, Math.abs(D.speed)); }
    this.dashT -= dt;
    if (this.dashT <= 0) { this.dashT = 0.1; N.truck.setDash({ mph: D.mph, rpm: D.rpm, clock: this.d.clock(), fuel: id === 'roswell' ? 0.55 : 0.62 }); }
    // a camera that has left the cab sees the whole truck in this very frame
    this.d.camera.updateMatrixWorld();
    N.truck.viewFrom(this.d.camera);
  }
}
