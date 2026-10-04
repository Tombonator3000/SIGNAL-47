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
import { Crossing, type CourtSite } from './Crossing';
import { MotelFront } from './MotelFront';
import { flood as siteFlood } from './kit';
import { loadArtFor, artTexture, DINER_ART } from '../core/art';

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
//
// The walk from SARO's fire exit over the highway to the motel is part of SARO (Crossing.ts).

export const ROAD_ORIGIN = new THREE.Vector3(8000, 0, 0);
export const STATION_ORIGIN = new THREE.Vector3(0, 0, 8000);
export const ROOM6_ORIGIN = new THREE.Vector3(0, 0, -8000);
export const DINER_ORIGIN = new THREE.Vector3(-8000, 0, 0);

export interface WorldDeps {
  scene: THREE.Scene;
  camera: THREE.PerspectiveCamera;
  player: Player;
  saro: { groups: THREE.Object3D[]; zones: Zone[]; colliders: Collider[]; truck: { x: number; z: number; heading: number } };
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
}

type Modules = {
  Station01: typeof import('./Station01');
  RoadArea: typeof import('../drive/RoadArea');
  Drive: typeof import('../drive/Drive');
  Truck: typeof import('../drive/Truck');
  engine: typeof import('../drive/engineSound');
  Room6: typeof import('./Room6');
  Diner: typeof import('./Diner');
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
  /** The way over the road and the motel's front (the old backdrop until MotelFront.ts). */
  crossing: Crossing;
  motel: MotelFront;
  court: CourtSite;
  saroTruck: Truck | null = null;
  /** Tests steer the truck through this instead of the keys (tools/chapter3.py): fixed
   *  input, or a function asked every step (an autopilot along road.route). */
  testInput: { steer: number; throttle: number } | (() => { steer: number; throttle: number }) | null = null;

  private mods: Partial<Modules> = {};
  private road: RoadArea | null = null;
  private drive: DriveController | null = null;
  private truck: Truck | null = null;          // the one that drives (lit by the road's floods)
  private parked: Truck | null = null;         // the same truck standing at the station (the station's floods)
  private engine: EngineSound | null = null;
  private dashT = 0;
  private busy = false;

  constructor(private d: WorldDeps) {
    // the walk over the road belongs to SARO: built now, shown and walked with SARO
    const c = new Crossing();
    this.crossing = c;
    // Sierra Motor Court from the outside, and its office (MotelFront.ts, by Codex)
    const m = new MotelFront();
    this.motel = m;
    this.court = m;
    for (const part of [c, m]) {
      d.scene.add(part.group);
      d.saro.groups.push(part.group);
      d.saro.zones.push(...part.zones);
      d.saro.colliders.push(...part.colliders);
    }
    d.player.floor = c.floorAt;
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
  }

  private async load<K extends keyof Modules>(k: K): Promise<Modules[K]> {
    if (!this.mods[k]) {
      const m = k === 'Station01' ? await import('./Station01')
        : k === 'RoadArea' ? await import('../drive/RoadArea')
          : k === 'Drive' ? await import('../drive/Drive')
            : k === 'Truck' ? await import('../drive/Truck')
              : k === 'Room6' ? await import('./Room6')
                : k === 'Diner' ? await import('./Diner')
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
      const [{ Station01, fieldFlood }, { Truck }] = await Promise.all([this.load('Station01'), this.load('Truck'),
        loadArtFor(['stucco', 'oldConcrete', 'weatheredWood', 'floorboards'])]);
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
      this.d.applyQuality();
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
      this.diner = d;
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
    if (this.saroTruck) this.saroTruck.group.visible = area === 'saro';
    if (this.site) this.site.group.visible = area === 'station01';
    if (this.road) this.road.group.visible = area === 'road';
    if (this.truck) this.truck.group.visible = area === 'road';
    if (this.parked) this.parked.group.visible = area === 'station01';
    if (this.room6) this.room6.group.visible = area === 'room6';
    if (this.diner) this.diner.group.visible = area === 'diner';
    if (this.dinerTruck) this.dinerTruck.group.visible = area === 'diner';
    player.floor = area === 'saro' ? this.crossing.floorAt : null;
    if (area === 'saro') { player.zones = saro.zones; player.colliders = saro.colliders; }
    if (area === 'station01' && this.site) { player.zones = this.site.zones; player.colliders = this.site.colliders; }
    if (area === 'room6' && this.room6) { player.zones = this.room6.zones; player.colliders = this.room6.colliders; }
    if (area === 'diner' && this.diner) { player.zones = this.diner.zones; player.colliders = this.diner.colliders; }
  }
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
  /** From the truck at SARO to the station gate. */
  driveOut() {
    if (this.busy) return;
    this.busy = true;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, '');
    // the fade takes a moment; loading may take longer on a slow connection
    Promise.all([this.ensureRoad(), this.ensureStation(), new Promise((r) => setTimeout(r, 700))]).then(() => {
      this.enter('road');
      const road = this.road!, drive = this.drive!, truck = this.truck!;
      drive.place(road.start.pos, road.start.heading);
      drive.onArrive = () => this.arrive();
      truck.setDriving(true);
      truck.setHeadlights(true);
      truck.setLightLevel(1);
      this.startEngine();
      this.driving = true;
      this.busy = false;
      hold(false);
      fade(false);
      toast(this.d.touch() ? 'Left stick: throttle, brake and steering.' : 'W and S: throttle and brake. A and D: steer.', 4.5);
      this.d.after(5, () => { if (this.driving) toast('South on the highway. STATION 01 is off the old survey track.', 4); });
    }).catch((e: Error) => {
      console.error(e);
      this.busy = false;
      hold(false);
      fade(false);
      toast('The road could not be loaded. Check your connection and try the truck again.', 5);
    });
  }

  // The drive reaches the gate: park, and walk in.
  private arrive() {
    if (!this.driving) return;
    const { fade, hold } = this.d;
    this.driving = false;
    this.engine?.stop();
    hold(true);
    fade(true, '');
    this.d.after(1.0, () => {
      this.truck!.setHeadlights(false);
      this.truck!.setDriving(false);
      this.enter('station01');
      this.placeAtStation();
      this.onArriveStation?.();
      hold(false);
      fade(false);
    });
  }

  /** From the truck at the station back to SARO. The way back is not driven again. */
  driveBack() {
    if (this.busy) return;
    const { fade, hold, toast } = this.d;
    hold(true);
    fade(true, 'BACK TO SARO');
    this.d.after(2.2, () => {
      this.d.skipClock(9 * 60);
      this.enter('saro');
      const p = this.d.saro.truck;
      this.d.player.place(p.x + 2.0, p.z - 3.6, 0);
      this.onArriveSaro?.();
      hold(false);
      fade(false);
      toast(`${this.d.clock()}. The truck is back on its pad.`, 3);
    });
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

  /** Stop driving without arriving (a save is opened, or the title). */
  stopDriving() {
    this.driving = false;
    this.engine?.stop();
  }

  // ---------- per frame ----------
  update(dt: number, t: number, input: { steer: number; throttle: number } | null, look: { x: number; y: number }) {
    if (this.area === 'station01') this.site?.update(dt, t);
    if (this.area === 'room6') this.room6?.update(dt, t);
    if (this.area === 'diner') this.diner?.update(dt, t);
    if (this.area === 'saro') this.motel.update(dt, t);
    if (this.area === 'road' && this.road && this.drive && this.truck) {
      if (this.testInput) input = typeof this.testInput === 'function' ? this.testInput() : this.testInput;
      if (this.driving) this.drive.update(dt, input ?? { steer: 0, throttle: 0 }, look);
      this.road.update(dt, t, this.drive.pos);
      if (this.engine) { this.engine.set(this.drive.rpm, this.drive.load); this.engine.tyres(this.drive.surface.kind, Math.abs(this.drive.speed)); }
      this.dashT -= dt;
      if (this.dashT <= 0) { this.dashT = 0.1; this.truck.setDash({ mph: this.drive.mph, rpm: this.drive.rpm, clock: this.d.clock(), fuel: 0.62 }); }
    }
  }
}
