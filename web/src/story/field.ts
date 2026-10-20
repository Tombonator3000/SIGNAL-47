import * as THREE from 'three';
import type { Field } from '../world/Field';
import type { Dish } from '../world/Dish';
import type { Interaction } from '../core/Interaction';
import type { AudioSys } from '../core/Audio';
import type { Doors } from '../world/Doors';
import { angleText } from './time';

// What the antenna field says (world/Field.ts): the walk gate in the array fence, a drive cabinet
// at each antenna's foot with a readout behind its glass, and a closer look at each pedestal.
// Nothing here is a finding of the chapters: the cabinets read what the antennas do, which after
// 02:15 is what S-03's motor bus says (canon, memory.md: az 026, el 32, no control command).
// Text inside the game is English.

export interface FieldDeps {
  inter: Interaction;
  doors: Doors;
  audio: AudioSys;
  toast: (text: string, secs?: number) => void;
  /** The antennas as they point now (Exterior.ts). */
  dishes: Dish[];
  /** In SARO's area on foot. */
  here: () => boolean;
}

// where every antenna turned to at 02:15 (Prologue.ts, EVENT_AZ and EVENT_EL), and the night's schedule
const TURNED = { az: 26, el: 32 }, SCHED = { az: 42, el: 48 };
const deg = (v: number, n: number) => angleText(v, n);

// A closer look at the cabinets: the sign is the same on all of them, the wear is not
const WEAR = [
  'The name is stencilled by hand. The paint ran under the last figure and nobody cleaned it off.',
  'A service sticker on the door: INSPECTED 11/85, and initials too smudged to read.',
  'The conduit to the pedestal has split along its length and been wound with black tape.',
  'Two cables come up out of the trench. One has been cut and spliced again, neatly, with new sleeving.',
  'The bolts on the door are new and bright. The hinges are not.',
  'An old wasp nest in the vent louvre, long empty.',
  'The stencil was painted over an older number. Under it a 2 still shows.',
  'A hook for a clipboard on the side. No clipboard.',
  'Sand has got in behind the inspection glass and lies in a little drift along the bottom.',
  'Rust has run from both hinges in two thin brown lines.',
];

export function bindField(field: Field, d: FieldDeps) {
  const dish = (id: string) => d.dishes.find((x) => x.id === id) ?? null;
  const turned = (x: Dish) => Math.abs(x.curAz - TURNED.az) < 0.6 && Math.abs(x.curEl - TURNED.el) < 0.6 && !x.moving;
  const at = (o: THREE.Object3D) => o.getWorldPosition(new THREE.Vector3());

  // the gate: a door like SARO's others (open at any time, saved with them)
  d.doors.add({ id: 'gate', inter: 'fieldGate', name: 'array gate', proxy: field.gateProxy, set: (k) => field.setGate(k),
    zones: [field.zone.gate], leaf: field.gateLeaf, range: 2.6 });
  const gate = d.inter.get('fieldGate');
  if (gate) {
    gate.examine = () => d.here() ? 'The walk gate into the array. A chain hangs from the post with its padlock open; nobody has locked it in years.' : null;
  }

  field.cabinets.forEach((cab, i) => {
    const id = cab.a.id, junction = id === 'S-03';
    d.inter.add({ id: `field:${id}`, object: cab.proxy, range: 2.4,
      label: () => !d.here() ? null : junction ? 'S-03 cable junction' : `Read drive cabinet ${id}`,
      use: () => {
        d.audio.play('click', { gain: 0.5, at: at(cab.proxy) });
        const x = dish(id);
        if (junction || !x) { d.toast('S-03 cable junction. Its readings go to the motor bus cabinet on the east walk.', 3.6); return; }
        field.showReadout(cab, () => {
          const t = turned(x);
          // moving by itself (story/dishesFollow.ts): the encoder runs, and nothing commands it
          return { head: `DRIVE ${id}   SARO ARRAY`,
            rows: [`ENCODER  AZ ${deg(x.curAz, 3)}  EL ${deg(x.curEl, 2)}`, `SCHED    AZ 0${SCHED.az}    EL ${SCHED.el}`, t || x.rogue ? 'CMD RCVD 0' : 'CONTROL  CONSOLE'],
            warn: x.rogue ? (x.moving ? 'SLEWING  NO COMMAND' : 'NO CONTROL COMMAND') : t ? 'NO CONTROL COMMAND' : x.moving ? 'SLEWING' : null };
        });
        d.toast(x.rogue
          ? `${id}. Encoder az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}${x.moving ? ', and running' : ''}. Commands received: 0.`
          : turned(x)
          ? `${id}. Encoder az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}. Scheduled az 0${SCHED.az}, el ${SCHED.el}. Commands received: 0.`
          : `${id}. Encoder az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}, on the console's track.`, 4.6);
      },
      examine: () => !d.here() ? null : junction
        ? 'A cable junction at the foot of S-03. Its drive is read at the motor bus cabinet on the east walk.'
        : `Drive cabinet ${id}. ${WEAR[(i * 7) % WEAR.length]}` });
    // the pedestal: a closer look only
    d.inter.add({ id: `mast:${id}`, object: cab.mast, label: () => null, use: () => {}, examineRange: 30,
      examine: () => {
        const x = dish(id);
        if (!d.here() || !x) return null;
        if (x.rogue) return `${id}. It has come down out of 026: az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}. Nobody is at the console.`;
        return turned(x) ? `${id}. It points where the other twenty-six point: az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}. Only the red lamp on the feed moves.`
          : `${id}. On the survey track with the others: az ${deg(x.curAz, 3)}, el ${deg(x.curEl, 2)}.`;
      } });
  });
}
