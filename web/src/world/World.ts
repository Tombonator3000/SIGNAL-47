import * as THREE from 'three';
import type { Player, Zone } from '../player/Player';
import type { Collider } from './ControlRoom';
import type { AreaId, FieldSite } from '../story/Chapter3';
import type { Station01 } from './Station01';
import type { RoadArea } from '../drive/RoadArea';
import type { DriveController } from '../drive/Drive';
import type { Truck } from '../drive/Truck';
import type { EngineSound } from '../drive/engineSound';
import type { Room6 } from './Room6';
import type { Diner } from './Diner';
import type { OldRoad } from '../drive/OldRoad';
import type { DriveInput, DriveArea } from '../drive/Drive';
import type { LegId, Exit } from '../drive/legs';
import { Crossing, type CourtSite } from './Crossing';
import { Grounds } from './Grounds';
import { MotelFront } from './MotelFront';
import { flood as siteFlood, addFlood, type FloodSet } from './kit';
import { loadArtFor, artTexture, DINER_ART, OLDROAD_ART } from '../core/art';
import { STATION_TURN } from './geo';

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
//   diner       around (-8000, 0, 0)    Mesa Diner on the Roswell road (Diner.ts, by Codex), «All Night»
//   roswell     around (0, 0, -24000)   the old Roswell road at dawn, chapter six (drive/OldRoad.ts)
//
// The walk from SARO's fire exit over the highway to the motel is part of SARO (Crossing.ts).

export const ROAD_ORIGIN = new THREE.Vector3(8000, 0, 0);
export const STATION_ORIGIN = new THREE.Vector3(0, 0, 8000);
export const ROOM6_ORIGIN = new THREE.Vector3(0, 0, -8000);
export const DINER_ORIGIN = new THREE.Vector3(-8000, 0, 0);
export const OLDROAD_ORIGIN = new THREE.Vector3(0, 0, -24000);
// The night areas see the camera's far plane at 5 km and thick haze; the old road at dawn
// sees farther (the camera goes up at the end).
const FAR = 5000, FAR_OLDROAD = 12000, FOG = 0.0021, FOG_OLDROAD = 0.00055;

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
}

type Modules = {
  Station01: typeof import('./Station01');
  RoadArea: typeof import('../drive/RoadArea');
  Drive: typeof import('../drive/Drive');
  Truck: typeof import('../drive/Truck');
  engine: typeof import('../drive/engineSound');
  Room6: typeof import('./Room6');
  Diner: typeof import('./Diner');
  OldRoad: typeof import('../drive/OldRoad');
  legs: typeof import('../drive/legs');
};

export class World {
  area: AreaId = 'saro';
  site: (Station01 & FieldSite) | null = null;
  driving = false;
  onArriveStation?: () => void;
  onArriveSaro?: () => void;
  onStationLoaded?: (site: FieldSite) => void;
  onRoom6Loaded?: (room: Room6) => void;
  onEnterRoom6?: () => void;
  onLeaveRoom6?: () => void;
  room6: Room6 | null = null;
  diner: Diner | null = null;
  onDinerLoaded?: (diner: Diner) => void;
  private dinerTruck: Truck | null = null;
  /** The diner's collider round its parked truck (it moves with the truck). */
  private dinerCol: Collider | null = null;
  /** An invisible box round the truck on the diner's lot, for interaction (chapter five). */
  dinerTruckProxy: THREE.Object3D | null = null;
  onDinerTruck?: (proxy: THREE.Object3D) => void;
  /** The old Roswell road (chapter six), its truck and its controller, once loaded. */
  oldRoad: OldRoad | null = null;
  oldTruck: Truck | null = null;
  oldDrive: DriveController | null = null;
  /** Chapter six takes over: the pedals and wheel (input), or the camera (the pull-out at the
   *  end, while the truck stands). Cleared when the drive stops. */
  oldControl: { input?: (dt: number) => DriveInput; camera?: (dt: number) => void } | null = null;
  /** The way over the road and the motel's front (the old backdrop until MotelFront.ts). */
  crossing: Crossing;
  grounds: Grounds;
  motel: MotelFront;
  court: CourtSite;
  saroTruck: Truck | null = null;
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
  private legs: Partial<Record<LegId, { area: DriveArea & { park?: { minX: number; maxX: number; minZ: number; maxZ: number } | null; routes?: Partial<Record<string, THREE.Vector3[]>> }; drive: DriveController; truck: Truck }>> = {};
  private trip: { from: LegId; to: LegId | null; rate: number; onArrive?: () => void; still: number; home: LegId | null; hint?: string } | null = null;

  private mods: Partial<Modules> = {};
  private road: RoadArea | null = null;
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
    for (const part of [c, g, m]) {
      d.scene.add(part.group);
      d.saro.groups.push(part.group);
      d.saro.zones.push(...part.zones);
      d.saro.colliders.push(...part.colliders);
    }
    d.player.floor = g.floorAt;
  }

  // ---------- loading ----------
  // The truck is part of SARO from the start (it stands on the pad all night). It is
  // small; the road and the station are not, so they load on demand.
  // A material belongs to one set of fake floodlights, so each area has its own copy of
  // the truck: one on the SARO pad, one on the road, one parked at the station.
  async initSaro() {
    const { Truck } = await import('../drive/Truck');
    const t = new Truck({ flood: siteFlood });
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
      const area = new StationDrive(s.colliders.filter((c) => c !== s.truckCollider), (x, z) => s.groundAt(x, z), road.obstacles);
      area.headlights = this.headlightSlots(fieldFlood);
      const drive = new DriveController(this.d.camera, t, area);
      drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      this.legs.station01 = { area, drive, truck: t };
      this.parkAt('station01');
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
      truck.headlightFloods(roadFlood, [0, 1, 2]);
      truck.group.visible = false;
      this.d.scene.add(truck.group);
      this.truck = truck;
      const road = new RoadArea(ROAD_ORIGIN.clone());
      road.group.visible = false;
      this.d.scene.add(road.group);
      this.road = road;
      this.drive = new DriveController(this.d.camera, truck, road);
      this.drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      road.onCattleGuard = (speed) => this.engine?.rattle(speed);
      this.legs.road = { area: road, drive: this.drive, truck };
      this.d.applyQuality();
    });
  }

  // The old road at dawn, its own truck lit by its own floods, and a controller for it
  private ensureOldRoad() {
    return this.once('oldroad', async () => {
      const [{ OldRoad, oldFlood, OLD_HEADLIGHTS }, { DriveController }, { Truck }] = await Promise.all([this.load('OldRoad'), this.load('Drive'), this.load('Truck'), this.load('engine'),
        loadArtFor(OLDROAD_ART)]); // round 11: the road's own pictures come with it
      const truck = new Truck({ flood: oldFlood });
      truck.headlightFloods(oldFlood, OLD_HEADLIGHTS);
      truck.group.visible = false;
      this.d.scene.add(truck.group);
      const road = new OldRoad(OLDROAD_ORIGIN.clone());
      road.group.visible = false;
      this.d.scene.add(road.group);
      const drive = new DriveController(this.d.camera, truck, road);
      drive.onBump = (speed) => this.d.thud(Math.min(1, speed / 8));
      this.oldRoad = road; this.oldTruck = truck; this.oldDrive = drive;
      this.legs.roswell = { area: road, drive, truck };
      this.d.applyQuality();
      return road;
    });
  }
  /** The engine sound, for the chapter that stops it. */
  get engineSound() { return this.engine; }

  /** From the diner's lot out onto the old road (chapter six): a cut with the road's name,
   *  then the truck in the right lane just before the six-mile post, the engine running.
   *  ready() runs behind the black, before the picture comes back. */
  driveOldRoad(ready: () => void) {
    if (this.busy) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, 'THE OLD ROAD');
    const loaded = this.ensureOldRoad();
    this.d.after(2.4, () => {
      void loaded.then(() => {
        this.enter('roswell');
        const road = this.oldRoad!, drive = this.oldDrive!, truck = this.oldTruck!;
        road.hitBack = false;
        drive.place(road.start.pos, road.start.heading);
        truck.setDriving(true);
        truck.setHeadlights(true);
        truck.setLightLevel(1);
        truck.setHeadColor(null);
        truck.setDashLevel(null);
        this.oldControl = null;
        this.truckAt = null; this.leg = 'roswell';
        this.parkAt('diner');
        this.startEngine();
        this.driving = true;
        ready();
        toast(this.d.touch() ? 'Left stick: throttle, brake and steering.' : 'W and S: throttle and brake. A and D: steer.', 4.5);
      }).catch((e) => {
        console.error(e);
        toast('The old road could not be loaded. Check your connection and try the truck again.', 5);
      }).finally(() => { this.busy = false; hold(false); fade(false); });
    });
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

  // The diner, its pictures (round 6 and 8) and SARO's truck parked on its lot
  private ensureDiner() {
    return this.once('diner', async () => {
      const [{ Diner, dinerFlood }, { Truck }] = await Promise.all([this.load('Diner'), this.load('Truck'), loadArtFor(DINER_ART)]);
      const d = new Diner(DINER_ORIGIN.clone());
      d.setSurfaceArt(artTexture('dinerBooth', [2, 2]), artTexture('dinerWall', [0.5, 0.5]));
      d.group.visible = false;
      this.d.scene.add(d.group);
      const t = new Truck({ flood: dinerFlood });
      const a = d.anchors.truckPark;
      t.group.position.set(a.x, 0, a.z);
      t.group.rotation.y = a.yaw;
      t.group.visible = false;
      this.d.scene.add(t.group);
      this.dinerTruck = t;
      this.dinerCol = d.colliders.find((c) => Math.abs(c.minX - (DINER_ORIGIN.x + 7.85)) < 0.01 && Math.abs(c.minZ - (DINER_ORIGIN.z + 3.2)) < 0.01) ?? null;
      const px = new THREE.Mesh(new THREE.BoxGeometry(2.2, 1.9, 5.4), new THREE.MeshBasicMaterial({ visible: false }));
      px.position.set(a.x, 0.95, a.z); px.rotation.y = a.yaw; px.name = 'dinerTruck';
      t.group.parent!.add(px);
      this.dinerTruckProxy = px;
      this.onDinerTruck?.(px);
      this.diner = d;
      this.parkAt('diner');
      this.d.applyQuality();
      this.onDinerLoaded?.(d);
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
    if (this.saroTruck) this.saroTruck.group.visible = here('saro');
    if (this.site) this.site.group.visible = area === 'station01';
    if (this.road) this.road.group.visible = area === 'road';
    if (this.truck) this.truck.group.visible = here('road');
    if (this.parked) this.parked.group.visible = here('station01');
    if (this.room6) this.room6.group.visible = area === 'room6';
    if (this.diner) this.diner.group.visible = area === 'diner';
    if (this.dinerTruck) this.dinerTruck.group.visible = here('diner');
    if (this.oldRoad) this.oldRoad.group.visible = area === 'roswell';
    if (this.oldTruck) this.oldTruck.group.visible = area === 'roswell';
    const cam = this.d.camera, far = area === 'roswell' ? FAR_OLDROAD : FAR;
    if (cam.far !== far) { cam.far = far; cam.updateProjectionMatrix(); }
    const fog = this.d.scene.fog as THREE.FogExp2 | null;
    if (fog) fog.density = area === 'roswell' ? FOG_OLDROAD : FOG;
    player.floor = area === 'saro' ? this.grounds.floorAt : null;
    if (area === 'saro') { player.zones = saro.zones; player.colliders = saro.colliders; }
    if (area === 'station01' && this.site) { player.zones = this.site.zones; player.colliders = this.site.colliders; }
    if (area === 'room6' && this.room6) { player.zones = this.room6.zones; player.colliders = this.room6.colliders; }
    if (area === 'diner' && this.diner) { player.zones = this.diner.zones; player.colliders = this.diner.colliders; }
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
    const go = () => this.startTrip(onTheRoad ? 'road' : 'saro', 'station01', { at: onTheRoad ? this.road!.start : undefined,
      hint: 'Leave the truck on the gravel outside the gate: stop there.', onArrive: () => this.onArriveStation?.() });
    if (onTheRoad) void this.ensureRoad().then(go); else go();
  }

  /** From the truck on SARO's pad to the diner where the old road leaves the highway
   *  (chapter five). Not driven: a cut with the place's name, nine minutes on the clock. */
  driveToDiner(onArrive: () => void) {
    if (this.busy) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, 'MESA DINER');
    const ready = this.ensureDiner();
    this.d.after(2.2, () => {
      void ready.then(() => {
        this.d.skipClock(9 * 60);
        this.truckAt = 'diner';
        for (const id of ['saro', 'diner'] as LegId[]) this.parkAt(id);
        this.enter('diner');
        this.placeAtDiner('arrive');
        onArrive();
      }).catch((e) => {
        console.error(e);
        toast('The diner could not be loaded. Check your connection and try the truck again.', 5);
      }).finally(() => { this.busy = false; hold(false); fade(false); });
    });
  }

  /** From the truck at the station back to SARO, the whole way, onto the pad. */
  driveBack() {
    this.startTrip('station01', 'saro', { hint: 'The truck goes back on its pad by the service yard: up the ramp, and stop there.',
      onArrive: () => { this.onArriveSaro?.(); this.d.toast(`${this.d.clock()}. The truck is back on its pad.`, 3); } });
  }

  // ---------- trips ----------
  // The truck is driven from one place to another the whole way (drive/legs.ts). Each area it
  // passes through is a leg with its own copy of the truck and its own controller; where two
  // areas meet on a road the truck is handed from one to the next between two frames.
  private trucksHome: { t: Truck; x: number; y: number; z: number; h: number }[] = [];

  /** SARO's own ground to drive on: the pad, the ramp, the service road, the highway. */
  private ensureSaroLeg() {
    return this.once('saroLeg', async () => {
      const [{ DriveController }, { SaroDrive }] = await Promise.all([this.load('Drive'), this.load('legs'), this.load('engine')]);
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
  private truckOf(id: LegId) { return id === 'saro' ? this.saroTruck : id === 'station01' ? this.parked : id === 'diner' ? this.dinerTruck : null; }

  /** The parked truck's collider and its handle for "use" go where the truck stands, or out
   *  of the way while it is gone. */
  private parkAt(id: LegId) {
    const t = this.truckOf(id);
    if (!t) return;
    const here = this.truckAt === id;
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
      proxy.position.set(at.x, here ? 0.95 : -500, at.z);
      proxy.rotation.y = h;
    }
  }

  /** Get in and drive from where the truck stands to another place, the whole way.
   *  to null: drive anywhere; at: start there, not where the truck stands; free: the trucks
   *  stay where they are and go back to their places afterwards (the developer menu);
   *  hint: said once on coming into the area where the truck is to be left. */
  startTrip(from: LegId, to: LegId | null, o: { rate?: number; onArrive?: () => void; onStart?: () => void; at?: { pos: THREE.Vector3; heading: number };
    legs?: LegId[]; free?: boolean; hint?: string } = {}) {
    if (this.busy || this.driving) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, '');
    this.load('legs').then(({ legsBetween }) => {
      const need = o.legs ?? (to ? legsBetween(from, to) : [from]);
      return Promise.all([...need.map((l) => this.ensureLeg(l)), this.load('engine'), new Promise((r) => setTimeout(r, 400))]);
    }).then(() => {
      const L = this.legs[from]!;
      const g = L.truck.group;
      // where every parked truck stands now, so a trip that is given up puts them back
      this.trucksHome = (['saro', 'station01', 'diner'] as LegId[]).map((id) => this.truckOf(id)).filter((t): t is Truck => !!t)
        .map((t) => ({ t, x: t.group.position.x, y: t.group.position.y, z: t.group.position.z, h: t.group.rotation.y }));
      this.trip = { from, to, rate: o.rate ?? 1, onArrive: o.onArrive, still: 0, home: this.truckAt, hint: o.hint };
      this.leg = from;
      if (!o.free) { this.truckAt = null; for (const id of ['saro', 'station01', 'diner'] as LegId[]) this.parkAt(id); }
      L.drive.place(o.at?.pos ?? g.position.clone(), o.at?.heading ?? g.rotation.y);
      this.lightsOn(L.truck);
      this.enter(from);
      this.startEngine();
      this.driving = true;
      Object.assign(this.auto, { rev: false, stall: 0, back: 0, stuck: 0 });
      if (o.onStart) o.onStart();
      else if (from === to && o.hint) toast(o.hint, 5);
      else toast(this.d.touch() ? 'Left stick: throttle, brake and steering.' : 'W and S: throttle and brake (S at a stop: reverse). A and D: steer.', 4.5);
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
    const id = this.leg!, L = this.legs[id]!, D = L.drive, legs = this.mods.legs!;
    const ex = legs.exitFrom(id, D.pos.x, D.pos.z, D.heading);
    if (ex && this.legs[ex.to]) { this.transfer(ex); return; }
    const trip = this.trip;
    if (!trip) return;
    if (trip.rate !== 1) this.d.skipClock(dt * (trip.rate - 1));
    const P = (L.area as { park?: { minX: number; maxX: number; minZ: number; maxZ: number } | null }).park;
    // where to leave the truck, said once it is in sight
    if (trip.hint && trip.to === id && P && Math.hypot(D.pos.x - (P.minX + P.maxX) / 2, D.pos.z - (P.minZ + P.maxZ) / 2) < 110) {
      this.d.toast(trip.hint, 5); trip.hint = undefined;
    }
    if (trip.to === id && P && D.pos.x >= P.minX && D.pos.x <= P.maxX && D.pos.z >= P.minZ && D.pos.z <= P.maxZ && Math.abs(D.speed) < 0.35) {
      trip.still += dt;
      if (trip.still > 0.7) this.park();
    } else trip.still = 0;
  }
  // Into the next area: its truck goes on from here, as fast and as turned as this one.
  private transfer(ex: Exit) {
    const from = this.legs[this.leg!]!, to = this.legs[ex.to]!;
    this.leg = ex.to;
    to.drive.adopt(from.drive, ex.x, ex.z, ex.heading);
    to.truck.copyCab(from.truck);
    this.dashT = 0;   // the clock on its dash is set this frame
    this.lightsOn(to.truck);
    from.truck.setDriving(false);
    this.enter(ex.to);

  }
  // Stopped where the truck is left: the engine off, and the player gets out by the door.
  private park() {
    const id = this.leg!, L = this.legs[id]!, trip = this.trip;
    this.driving = false;
    this.engine?.stop();
    L.truck.setDriving(false);
    L.truck.setHeadlights(false);
    this.truckAt = id; this.leg = null; this.trip = null; this.autopilot = null;
    this.parkAt(id);
    const { fade, hold } = this.d;
    hold(true);
    fade(true, '');
    this.d.after(0.4, () => {
      this.enter(id);
      this.getOut(L.truck);
      hold(false);
      fade(false);
      trip?.onArrive?.();
    });
  }
  private getOut(t: Truck) {
    const g = t.group, h = g.rotation.y, p = this.d.player, up = new THREE.Vector3(0, 1, 0);
    for (const side of [-1, 1, -1.4, 1.4]) {
      const v = new THREE.Vector3(side * 1.75, 0, -0.3).applyAxisAngle(up, h).add(g.position);
      if (p.walkable(v.x, v.z)) { p.place(v.x, v.z, h); return; }
    }
    const v = new THREE.Vector3(-1.75, 0, -0.3).applyAxisAngle(up, h).add(g.position);
    p.place(v.x, v.z, h);
  }

  // The autopilot (tests, the developer menu): along the way to the next area, or to where
  // the truck is left. Pure pursuit, slowed in time for the bends ahead; when the way lies
  // behind, a turn in three points (forward on full lock, back on the other lock when it can
  // go no further, and so on until it faces the way).
  private auto = { rev: false, stall: 0, back: 0, stuck: 0 };
  /** dt: the step (the stall timer of the turn); 0 to only look. */
  private autoInput(dt = 0): DriveInput {
    const id = this.leg!, L = this.legs[id]!, D = L.drive, trip = this.trip, legs = this.mods.legs!;
    const next = !trip?.to ? null : trip.to === id ? 'park' : legs.legsBetween(id, trip.to)[1];
    const route = next ? (id === 'road' ? legs.roadRoute(next as LegId) : (L.area as { routes?: Partial<Record<string, THREE.Vector3[]>> }).routes?.[next]) : null;
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
    this.driving = false;
    this.oldControl = null;
    this.engine?.stop();
    if (id) { this.legs[id]?.truck.setDriving(false); this.legs[id]?.truck.setHeadlights(false); }
    if (trip) {
      for (const h of this.trucksHome) { h.t.group.position.set(h.x, h.y, h.z); h.t.group.rotation.y = h.h; h.t.setDriving(false); }
      this.truckAt = trip.home;
    }
    this.leg = null; this.trip = null; this.autopilot = null;
    for (const p of ['saro', 'station01', 'diner'] as LegId[]) this.parkAt(p);
    this.free = null; this.freeBack = null;
  }

  /** The truck stands where the story has left it (a new night or a restored save): on
   *  SARO's pad, outside STATION 01's gate or on the diner's lot. */
  placeTruck(at: LegId) {
    this.stopDriving();
    this.truckAt = at;
    const put = (t: Truck | null, x: number, z: number, h: number) => { if (t) { t.group.position.set(x, 0, z); t.group.rotation.y = h; t.setDriving(false); } };
    const p = this.d.saro.truck;
    put(this.saroTruck, p.x, p.z, p.heading);
    const a = this.site?.anchors.truck;
    if (a) put(this.parked, a.x, a.z, a.heading);
    const b = this.diner?.anchors.truckPark;
    if (b) put(this.dinerTruck, b.x, b.z, b.yaw);
    for (const id of ['saro', 'station01', 'diner'] as LegId[]) this.parkAt(id);
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
    if (this.area === 'diner') this.diner?.update(dt, t);
    if (this.area === 'saro') this.motel.update(dt, t);
    if (this.area === 'roswell') this.oldRoad?.update(dt, t, this.d.camera.position, this.d.sky());
    if (this.area === 'road' && this.road && this.drive) this.road.update(dt, t, this.drive.pos);
    this.drivingStep(dt, input, look);
    // out on the track in the truck the station's moonlight gives way to the road's
    // (Station01.nightBlend); after the step, so it is right in the frame the truck comes in
    if (this.area === 'station01' && this.site) {
      const L = this.leg === 'station01' ? this.legs.station01 : null;
      const d = L ? Math.hypot(L.drive.pos.x - STATION_ORIGIN.x, L.drive.pos.z - STATION_ORIGIN.z - 8) : 0;
      this.site.nightBlend(1 - THREE.MathUtils.smoothstep(d, 110, 290));
    }
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
    if (this.engine) { this.engine.set(D.rpm, D.load); this.engine.tyres(D.surface.kind, Math.abs(D.speed)); }
    this.dashT -= dt;
    if (this.dashT <= 0) { this.dashT = 0.1; N.truck.setDash({ mph: D.mph, rpm: D.rpm, clock: this.d.clock(), fuel: id === 'roswell' ? 0.55 : 0.62 }); }
  }
}
