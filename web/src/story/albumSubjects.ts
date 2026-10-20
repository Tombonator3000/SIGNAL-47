import * as THREE from 'three';
import type { Subject } from '../core/album';
import { NOBODY } from '../core/album';
import type { World } from '../world/World';
import type { Exterior } from '../world/Exterior';
import type { ControlRoom } from '../world/ControlRoom';
import type { ServiceYard } from '../world/ServiceYard';
import type { RecordsAnnex } from '../world/Annex';
import type { Sky } from '../world/Sky';
import type { Cutouts } from '../core/cutouts';
import { MOTEL_AT, MOTEL_GLOWS } from '../world/geo';
import { NINTH_PRINT } from './ninthCow';

// What the album can name under a free frame (core/album.ts), where the player is: the known
// things of each area by where they stand, the people of the night, the sky, and the things the
// player may not have noticed: the cardboard alien wherever it has been put (core/cutouts.ts),
// one of them at the edge of the view (core/glimpses.ts), the someone in a SARO jacket standing
// where the time was lost (story/abduction.ts), and the ring of frost there; and the ninth cow lying
// at STATION 01 (story/ninthCow.ts).

export interface SubjectDeps {
  world: World; ext: Exterior; room: ControlRoom; yard: ServiceYard; annex: RecordsAnnex; sky: Sky;
  scene: THREE.Scene; camera: THREE.PerspectiveCamera; cutouts: Cutouts;
  /** Out under the sky (not in a room, the lab, the diner or the hut). */
  outdoors: () => boolean;
  /** Night: the dawn has hardly begun. */
  night: () => boolean;
  /** Seconds since midnight. */
  clock: () => number;
}

const CARDBOARD = 'The cardboard alien. Who keeps moving it?';
const JACKET = 'Someone in a SARO jacket, standing out there. I was the only one on shift.';
// [object key, name, size m, farthest m, weight, things that count as one]
type Thing = [string, string, number, number, number, string?];
const ROOM: Thing[] = [
  ['rack', 'the receivers', 0.9, 11, 1], ['phone', 'the phone', 0.2, 6, 1], ['mug', 'my coffee', 0.12, 3, 0.8],
  ['coffeePot', 'the coffee pot', 0.2, 5, 0.8], ['printer', 'the printer', 0.4, 8, 0.8], ['clock', 'the clock', 0.25, 10, 1],
  ['opsTerminal', 'the tracking terminal', 0.4, 8, 1], ['halley', 'the Halley poster', 0.6, 10, 1.1], ['decoder', 'the signal processor', 0.5, 8, 1],
  ['map', 'the map on the wall', 0.8, 10, 0.9], ['telex', 'the telex', 0.4, 8, 0.8], ['radio', 'the radio', 0.25, 6, 0.8],
];
const ANNEX: Thing[] = [['vending', 'the vending machine', 0.8, 12, 1], ['fieldBoxModel', 'the field box', 0.4, 6, 1], ['recPhoneModel', 'the phone in the records room', 0.2, 6, 1]];
const YARD_THINGS: Thing[] = [['motorBus', 'the S-03 motor cabinet', 0.8, 25, 1], ['vane', 'B-12', 0.7, 45, 1.2], ['wetBench', 'the wet bench', 1.2, 8, 1], ['r07', 'R-07', 0.4, 25, 1]];
const STATION: Thing[] = [
  ['hutDoor', 'the field hut', 3, 260, 1.3], ['transit', 'the transit', 0.6, 30, 1.1], ['markerA', 'fixed point A', 0.4, 60, 1.3],
  ['markerB', 'fixed point B', 0.4, 60, 1.2], ['stakesC', 'the stakes along C', 3, 80, 1.2], ['cable', 'the cut cable', 0.3, 12, 1.5],
  ['lamp', 'the field lamp', 0.5, 60, 0.9], ['gate', 'the gate', 2.4, 120, 0.9], ['receiver', 'the receiver', 0.4, 6, 1.1],
  ['fieldPhone', 'the field telephone', 0.3, 6, 1], ['diagram', 'the diagram on the wall', 0.4, 5, 0.9],
];
const ROOM6: Thing[] = [['tv', 'the television', 0.5, 8, 1], ['walkie', 'the walkie-talkie', 0.2, 4, 1.1], ['photo', 'the photograph by the bed', 0.2, 4, 1], ['bed', 'the bed', 1, 8, 0.6], ['phone', 'the phone', 0.2, 5, 0.8]];
const DINER: Thing[] = [
  ['sign', 'the Mesa Diner sign', 3, 900, 2.5, 'diner'], ['door', 'Mesa Diner', 7, 500, 1.4, 'diner'], ['waitress', 'the waitress', 0.4, 15, 2.5],
  ['driver', 'the driver', 0.4, 15, 2.5], ['counter', 'the counter', 2, 12, 0.7], ['jukebox', 'the jukebox', 0.6, 12, 1],
  ['booth', 'the booth', 0.9, 10, 0.7], ['clipping', 'the clipping on the wall', 0.4, 5, 1.1], ['payphone', 'the payphone', 0.3, 8, 0.9],
  ['coffee', 'the coffee', 0.2, 3, 0.8], ['menu', 'the menu', 0.6, 6, 0.8], ['rig', 'the rig on the lot', 6, 200, 1.2],
];

const shown = (o: THREE.Object3D) => { for (let x: THREE.Object3D | null = o; x; x = x.parent) if (!x.visible) return false; return true; };
/** Where a shown object is (world terms), raised by dy; null when it or a parent is hidden. */
const P = (o: THREE.Object3D | undefined | null, dy = 0) => {
  if (!o || !shown(o)) return null;
  const p = o.getWorldPosition(new THREE.Vector3()); p.y += dy;
  return p;
};

export function albumSubjects(d: SubjectDeps): Subject[] {
  const out: Subject[] = [];
  const add = (name: string, at: (THREE.Vector3 | null)[], o: Omit<Subject, 'name' | 'at'> = {}) => {
    const pts = at.filter((p): p is THREE.Vector3 => !!p);
    if (pts.length) out.push({ name, at: pts, ...o });
  };
  const things = (objs: Record<string, THREE.Object3D> | undefined, list: Thing[]) => {
    if (objs) for (const [k, name, r, max, w, group] of list) add(name, [P(objs[k])], { r, max, w, group });
  };
  const { world } = d, cam = d.camera.getWorldPosition(new THREE.Vector3());
  const open = d.outdoors(), night = d.night();

  // anywhere: the truck where it stands
  const tr = world.truckHere();
  if (tr) add('the truck', [P(tr.group, 1.1)], { r: 2.6, max: 140, w: 1.2 });

  switch (world.area) {
    case 'saro': {
      // the array, and the near antennas by their numbers (only the most prominent is named)
      const dishes = d.ext.dishes;
      add('the antennas', dishes.map((x) => new THREE.Vector3(x.x, 9 * x.scale, x.z)), { r: 7, max: 900, w: 1.2, group: 'array' });
      for (const x of dishes) {
        const p = new THREE.Vector3(x.x, 9 * x.scale, x.z);
        if (p.distanceTo(cam) < 60) add(x.id, [p], { r: 6 * x.scale, max: 60, w: 1.25, group: 'array' });
      }
      if (open) {
        const b = d.room.bounds, lab = d.yard.zone.lab;
        add('the control room', [new THREE.Vector3((b.minX + b.maxX) / 2, 2.6, (b.minZ + b.maxZ) / 2)], { r: 6, max: 260 });
        if (lab) add('the photo lab', [new THREE.Vector3((lab.minX + lab.maxX) / 2, 2, (lab.minZ + lab.maxZ) / 2)], { r: 3.5, max: 120 });
      }
      things(d.yard.objs, YARD_THINGS);
      const rm = d.room;
      add('the RX consoles', [P(rm.crtLeft.group, 0.3), P(rm.crtCenter.group, 0.3), P(rm.crtRight.group, 0.3)], { r: 0.45, max: 12, w: 1.3 });
      things(rm.objs, ROOM);
      things(d.annex.objs, ANNEX);
      // Sierra Motor Court down the highway: its sign from all over the site, the rooms nearer
      const [sx, sy, sz] = MOTEL_GLOWS[0];
      add('the Sierra Motor Court sign', [new THREE.Vector3(sx + MOTEL_AT.x, sy + MOTEL_AT.y, sz + MOTEL_AT.z)], { r: 3.5, max: 1600, w: 3, group: 'motel' });
      add('Sierra Motor Court', [new THREE.Vector3(-40.8 + MOTEL_AT.x, 2.2 + MOTEL_AT.y, 47 + MOTEL_AT.z)], { r: 14, max: 500, w: 1.5, group: 'motel' });
      break;
    }
    case 'station01':
      things(world.site?.proxies, STATION);
      // the ninth cow from the pasture, once she lies there (story/ninthCow.ts): a line of her own
      if (world.site?.ninthDown) add('the dead cow', [P(world.site.proxies.ninthBody), P(world.site.proxies.ninthHead)], { r: 1.3, max: 45, w: 3, odd: NINTH_PRINT });
      break;
    case 'room6': {
      const r6 = world.room6;
      if (r6) { add('N. Vega', [P(r6.objs.noraHead)], { r: 0.35, max: 8, w: 2.5 }); things(r6.proxies, ROOM6); }
      break;
    }
    case 'diner': case 'road': {
      things(world.diner?.proxies, DINER);
      const t = world.town;
      if (t) {
        const day = ((d.clock() % 86400) + 86400) % 86400, onAir = day < 4 * 3600 || day > 20 * 3600;
        const town: Thing[] = [
          ['pumps', 'the gas station', 4, 250, 1.2], ['gasWindow', 'the attendant behind the night window', 0.8, 30, 2],
          ['sheriffWindow', t.deputyIn ? 'the deputy at his desk' : "the sheriff's substation", 2, 150, t.deputyIn ? 1.8 : 1.1],
          ['laundromat', 'the laundromat', 3, 150, 1], ['cinema', 'The Plaza', 4, 250, 1.2, 'plaza'], ['marquee', 'the marquee', 2, 250, 1.1, 'plaza'],
          ['studio', onAir ? 'Earl at the microphone' : 'the studio', 1.5, 40, onAir ? 1.8 : 1, '1420'], ['amDoor', '1420 AM', 1.5, 120, 1, '1420'],
          ['souvenir', 'the souvenir shop', 1.5, 120, 1.1], ['letters', 'the church sign', 1.2, 80, 0.9, 'church'], ['churchDoor', 'the church', 3, 250, 1, 'church'],
          ['feed', 'the feed store', 3, 150, 0.9], ['payphone', 'the payphone', 0.3, 12, 0.9], ['tv1', 'a lit window', 0.8, 40, 0.6],
          ['tv2', 'a lit window', 0.8, 40, 0.6], ['kitchen', 'a lit window', 0.8, 40, 0.5],
        ];
        if (t.womanIn) town.push(['woman', 'the woman with the radio', 0.5, 25, 2.2]);
        if (t.smokerIn) town.push(['smoker', 'the man on the stoop', 0.5, 30, 2.2]);
        if (t.paperIn) town.push(['paper', 'the newspapers', 0.3, 8, 0.9]);
        if (t.deputyIn || t.carOut) town.push(['patrolCar', t.carOut ? "the deputy's car with its door open" : 'the patrol car', 2.4, 120, t.carOut ? 2.4 : 1.1]);
        things(t.proxies, town);
        if (t.cutoutOut) add('the cardboard alien on the shoulder', [t.group.position.clone().add(t.cutoutAt).setY(t.group.position.y + t.cutoutAt.y + 0.6)], { r: 0.6, max: 120, w: 2.5 });
      }
      break;
    }
  }

  // the sky at night: Halley's comet until it sets, and the Milky Way looking up
  if (night && open) {
    if (d.sky.comet.visible) add("Halley's comet, low in the south", [P(d.sky.comet)], { ang: 1.4, w: 1.6, max: 1e6, sky: true });
    const f = d.camera.getWorldDirection(new THREE.Vector3());
    if (f.y > 0.45) add('the Milky Way', [cam.clone().addScaledVector(f, 900)], { ang: 18, w: 0.35, max: 1e6, sky: true });
  }

  // what should not be there
  const g = d.scene.getObjectByName('glimpse');
  // (the figures only when they are more than a speck on the print: core/album.ts, Subject.h)
  if (g?.visible) add('someone', [P(g, 0.7), P(g, 1.05), P(g, 0.3)], { r: 0.3, max: 330, odd: NOBODY, h: 1.2 });
  if (d.cutouts.up) add('the cardboard alien', [d.cutouts.position.clone().setY(d.cutouts.position.y + 0.8)], { r: 0.5, max: 70, odd: CARDBOARD, h: 1.5 });
  for (const o of d.scene.children) {
    if (o.name === 'abduct:record' && o.visible) add('someone in a SARO jacket', [P(o, 1.1)], { r: 0.5, max: 400, odd: JACKET, h: 1.75 });
    if (o.name === 'abduct:ring' && o.visible) add('a ring of flattened grass, white with frost', [P(o)], { r: 2.7, max: 50, w: 2 });
  }
  return out;
}
