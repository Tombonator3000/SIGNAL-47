import * as THREE from 'three';
import { flood } from './kit';
import { Dressing, type Foot, type Opening, type Wall } from './weathering';
import { GROUND as G } from './Crossing';
import { facadeLit } from '../core/textures';

// SARO's house from the outside (weathering.ts; the concept art ch1_service_yard and the
// peripheral glimpse between the antennas): the control room, the records annex, the photo lab
// and the east wing weathered as a 1960s site in the desert is, their roofs cluttered with what
// keeps them cool and dry, a service entrance on the wing, and the staff's corner of the yard:
// two cars nose in to the wing, drums, a cable reel, pallets and tyres behind it, the cans by
// the dumpster. In the site's terms (Exterior.ts, Grounds.ts): the floors at 0, the ground at
// G -0.6. Built with the grounds (Grounds.ts adds the colliders through `col`).
export function dressSaro(col: (f: Foot) => void) {
  // the props take the lamps as the wing's walls do (Exterior.ts, 0.03), not as the open yard
  const d = new Dressing(flood, 0.012, 47, 0.03);
  const V = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
  // the ground the brush keeps off: the fire exit and its path, the east walk and the steps,
  // the gap between the lab and the wing, the props, the way between the cars and the wing
  const clear = (x: number, z: number) => (x < -5.8 && z > 4.0 && z < 7.4) || (x > 6.5 && x < 13 && z > -13 && z < 2)
    || (x > 19.1 && x < 20.7 && z > -6.2 && z < 1.2) || (x > 20.7 && x < 22.9 && z > 5.9 && z < 8.0) || (x > 23.0 && x < 34.0 && z > 5.9 && z < 7.3)
    || (x > 32.8 && x < 38.0 && z > -7.9 && z < -5.9) || (x > 17.8 && x < 19.3 && z > 0.9 && z < 1.8);

  // ---------- the walls ----------
  // the walls on the plinths stand back from its edge (0.3 to 0.4 m; the north one 1.1 m): the
  // dust lies on that ledge and the brush grows at the plinth's foot below
  const ledge = { drift: 0.22 };
  // the control room: the long window in the north wall in seven bays (its mullions every 1.8 m),
  // the east door, and the parapet's coping at 3.6
  const bays: Opening[] = Array.from({ length: 6 }, (_, k) => ({ a0: -5.4 + k * 1.8 + 0.05, a1: -5.4 + (k + 1) * 1.8 - 0.05, y0: 0.95, y1: 2.75 }));
  d.wall({ face: '-z', at: -4.8, a0: -6.3, a1: 6.3, y0: 0, y1: 3.6 }, { openings: bays, streaks: 0.8, drips: [-5.95, 0.9, 5.95], drift: 0.5 });
  d.wall({ face: '-x', at: -6.3, a0: -4.8, a1: 4.8, y0: 0, y1: 3.6 }, { ...ledge, patches: 1.4, drips: [-3.0, 2.0] });
  d.wall({ face: '+x', at: 6.3, a0: -4.8, a1: 4.8, y0: 0, y1: 3.6 }, { ...ledge, openings: [{ a0: 1.1, a1: 2.1, y0: 0, y1: 2.12 }], drips: [-2.5] });
  d.wall({ face: '+z', at: 4.8, a0: 2.3, a1: 6.3, y0: 0, y1: 3.6 }, ledge);
  // the records annex: the wall pack and the meter on its south face, the fire exit in its west
  d.wall({ face: '+z', at: 10.8, a0: -6.3, a1: 2.3, y0: 0, y1: 3.2 }, { ...ledge, patches: 1.5, drips: [-4.4, -2.0] });
  d.wall({ face: '-x', at: -6.3, a0: 4.8, a1: 10.8, y0: 0, y1: 3.2 }, { ...ledge, openings: [{ a0: 5.22, a1: 6.18, y0: 0, y1: 2.12 }] });
  d.wall({ face: '+x', at: 2.3, a0: 4.8, a1: 10.8, y0: 0, y1: 3.2 }, ledge);
  // the east wing: eight windows a side in its facade picture (core/textures.ts facadeWindows);
  // its water comes down at the scuppers, the coping's joints and under the coolers
  const wins: Opening[] = Array.from({ length: 8 }, (_, i) => ({ a0: 20.6 + 2.25 * i + 0.4, a1: 20.6 + 2.25 * i + 1.85, y0: 0.58, y1: 2.34 }));
  const south: Wall = { face: '+z', at: 5.5, a0: 20.6, a1: 38.6, y0: 0, y1: 3.6 }, north: Wall = { face: '-z', at: -5.5, a0: 20.6, a1: 38.6, y0: 0, y1: 3.6 };
  d.wall(south, { ...ledge, openings: wins, streaks: 0.6, patches: 0.8, drips: [20.75, 31.85, 38.4, 29.6, 33.8], grime: 0.9 });
  d.wall(north, { ...ledge, openings: wins, streaks: 0.6, drips: [20.8, 25.0, 29.6, 34.1, 38.4], grime: 0.9 });
  d.wall({ face: '-x', at: 20.6, a0: -5.5, a1: 5.5, y0: 0, y1: 3.6 }, { ...ledge, drips: [-2.0] });
  d.wall({ face: '+x', at: 38.6, a0: -5.5, a1: 5.5, y0: 0, y1: 3.6 }, { ...ledge, patches: 1.6, drips: [2.2] });
  // its windows stand out of the picture: sills, frames, and the blinds let down in the lit ones
  const litS = facadeLit(8, 1, 3), litN = facadeLit(8, 1, 8);
  wins.forEach((op, i) => {
    d.window(south, op, { sill: 'concrete', frame: 'galvDark', blind: litS[i] ? 0.25 + ((i * 0.37) % 0.8) : 0 });
    // (the north face's picture runs the other way: its first window is the east end's)
    d.window(north, op, { sill: 'concrete', frame: 'galvDark', blind: litN[7 - i] ? 0.3 + ((i * 0.53) % 0.7) : 0 });
  });
  // the photo lab, on its own plinth flush with its walls; its door in the west wall
  d.wall({ face: '+z', at: 1.0, a0: 12.6, a1: 19.2, y0: G, y1: 3.0 }, { drift: 0.5, tufts: 3, clear, drips: [17.6] });
  d.wall({ face: '+x', at: 19.2, a0: -6.0, a1: 1.0, y0: G, y1: 3.0 }, { drift: 0.35 });
  d.wall({ face: '-z', at: -6.0, a0: 12.6, a1: 19.2, y0: G, y1: 3.0 }, { drift: 0.5, tufts: 4, clear });
  d.wall({ face: '-x', at: 12.6, a0: -6.0, a1: 1.0, y0: 0, y1: 3.0 }, { openings: [{ a0: -1.7, a1: -0.7, y0: 0, y1: 2.1 }], drift: 0.3 });
  // the plinths where they stand out of the ground: the dirt, the drift and the brush at them
  const plinth = { band: 0.4, chips: 0.25, streaks: 0, patches: 0, grime: 0.4, drift: 0.75, tufts: 3.5, clear };
  d.wall({ face: '-z', at: -5.9, a0: -6.7, a1: 6.7, y0: G, y1: 0 }, plinth);
  d.wall({ face: '-x', at: -6.7, a0: -5.9, a1: 4.75, y0: G, y1: 0 }, plinth);
  d.wall({ face: '+x', at: 6.7, a0: -5.9, a1: -0.6, y0: G, y1: 0 }, { ...plinth, tufts: 0 });
  d.wall({ face: '+z', at: 11.1, a0: -6.6, a1: 2.6, y0: G, y1: 0 }, plinth);
  d.wall({ face: '-x', at: -6.6, a0: 6.65, a1: 11.1, y0: G, y1: 0 }, plinth);
  d.wall({ face: '+z', at: 5.9, a0: 20.2, a1: 39.0, y0: G, y1: 0 }, { ...plinth, tufts: 2.5 });
  d.wall({ face: '-z', at: -5.9, a0: 20.2, a1: 39.0, y0: G, y1: 0 }, { ...plinth, tufts: 4.5 });
  d.wall({ face: '+x', at: 39.0, a0: -5.9, a1: 5.9, y0: G, y1: 0 }, { ...plinth, tufts: 4 });
  // rust under the wall packs (Exterior.building, Annex.shell), the fire exit's lamp and the meter
  d.rust(-6.3, 2.47, 2, '-x', 1.3); d.rust(-6.3, 2.47, -3, '-x', 1.1);
  d.rust(20.9, 2.67, 5.5, '+z', 1.5); d.rust(30.4, 2.67, 5.5, '+z', 1.2);
  d.rust(-2.0, 2.27, 10.8, '+z', 1.4); d.rust(-6.3, 2.2, 5.7, '-x', 0.9, 0.35); d.rust(1.6, 1.28, 10.8, '+z', 0.9);
  // and at the feet of the control room's downpipes (Grounds.things), the damp they leave
  for (const [x, z, f] of [[-6.36, -4.8, '-z'], [6.36, -4.8, '-z'], [-6.3, 10.86, '-x']] as const) d.stain('damp', x, 0.4, z, f, 0.8, 0.7, 0.7);

  // ---------- the roofs ----------
  // the control room (its parapet's top at 3.6; the condenser and the small box are Exterior's)
  d.swampCooler(-1.2, 3.6, 2.2, 0);
  d.vent(4.9, 3.6, 3.5, { h: 0.55 }); d.vent(-4.6, 3.6, -3.2, { turbine: true, h: 0.35, r: 0.1 }); d.vent(0.8, 3.6, -3.7, { h: 0.4, r: 0.04 });
  d.box(d.core, 0.9, 0.3, 0.9, 'galvDark', 4.6, 3.75, -3.3);   // the roof hatch
  // the weather mast at the north-west corner: the cups and the vane for the site's log
  {
    const x = -5.6, z = -4.1, y = 3.6;
    d.cyl(d.core, 0.035, 0.045, 3.4, 'galv', x, y + 1.7, z, 6);
    d.box(d.core, 0.9, 0.04, 0.04, 'galv', x, y + 3.3, z);
    d.cyl(d.core, 0.012, 0.012, 0.3, 'galv', x - 0.42, y + 3.45, z, 4);
    for (let i = 0; i < 3; i++) { const a = i * 2.094; d.rod(d.core, V(x - 0.42, y + 3.58, z), V(x - 0.42 + Math.cos(a) * 0.16, y + 3.58, z + Math.sin(a) * 0.16), 0.008, 'galv', 3); d.cyl(d.core, 0.04, 0.04, 0.05, 'galvDark', x - 0.42 + Math.cos(a) * 0.17, y + 3.58, z + Math.sin(a) * 0.17, 6, Math.PI / 2); }
    d.box(d.core, 0.04, 0.12, 0.42, 'galvDark', x + 0.42, y + 3.48, z + 0.1);
    for (const a of [0.7, 2.8, 4.9]) d.rod(d.core, V(x, y + 2.6, z), V(x + Math.cos(a) * 1.1, y + 0.02, z + Math.sin(a) * 1.1), 0.005, 'galvDark', 3);
  }
  // the annex (3.2; its roof unit is Annex.ts's)
  d.vent(0.6, 3.2, 9.9, { h: 0.5 }); d.vent(-1.6, 3.2, 6.0, { turbine: true, h: 0.3, r: 0.09 });
  // the east wing (3.6): a coping round its edge, two coolers, the stacks, a hatch
  for (const z of [5.52, -5.52]) d.box(d.core, 18.12, 0.12, 0.08, 'galvDark', 29.6, 3.62, z);
  for (const x of [20.58, 38.62]) d.box(d.core, 0.08, 0.12, 11.12, 'galvDark', x, 3.62, 0);
  d.swampCooler(25.0, 3.6, -1.6, Math.PI, 1.1); d.swampCooler(33.8, 3.6, 0.9, 0, 1.1);
  d.vent(22.2, 3.6, 3.2); d.vent(28.4, 3.6, 3.9, { h: 0.75, r: 0.05 }); d.vent(31.2, 3.6, -3.4, { turbine: true, h: 0.4, r: 0.11 }); d.vent(37.0, 3.6, -2.2, { h: 0.5 });
  d.box(d.core, 1.0, 0.4, 1.0, 'galvDark', 36.2, 3.8, 3.4);
  // the lab (3.0): the darkroom's light-tight exhaust and a stack
  d.box(d.core, 0.7, 0.42, 0.7, 'galvDark', 17.6, 3.21, -4.5); d.box(d.core, 0.8, 0.06, 0.8, 'galv', 17.6, 3.45, -4.5);
  d.vent(14.2, 3.0, -1.2, { h: 0.45 });

  // ---------- on the walls ----------
  // the wing's service entrance on its east end: meter, disconnect, conduit up to the roof
  d.meter(38.6, 1.5, 3.4, '+x', 1.9); d.rust(38.6, 1.25, 3.4, '+x', 0.9);
  d.conduit(V(38.66, 0.25, 2.2), V(38.66, 3.55, 2.2));
  // scuppers and downspouts on the wing's long sides, clear of its windows and of the lettering
  // on its south face (Exterior.ts: 22.4 to 30.4): at the corner and between the fifth and the
  // sixth window; painted dark, as the wall (the plinth stands 0.4 m out: the splash blocks stay on it)
  for (const x of [20.75, 31.85, 38.4]) d.downspout(x, 3.48, 0, 5.5, '+z', 'galvDark', 0.22);
  for (const x of [20.8, 29.6, 38.4]) d.downspout(x, 3.48, 0, -5.5, '-z', 'galvDark', 0.22);
  // a hose on its hook by the lab's spigot
  d.cyl(d.fine, 0.02, 0.02, 0.14, 'galvDark', 18.4, 0.55, 1.07, 6, Math.PI / 2);
  d.hose(18.6, 1.0, 1.08, { wall: '+z' });
  // conduit from the lab along to the control room's pocket (the lab's power)
  d.conduit(V(12.66, 2.7, 0.6), V(12.66, 2.7, -5.6));

  // ---------- the staff's corner south of the wing ----------
  // Reyes's car and the site's old pickup, nose in to the wing, the curbs in front of them
  for (const x of [27.0, 31.0]) { d.box(d.core, 1.8, 0.13, 0.2, 'concrete', x, G + 0.065, 6.45); d.contact(x, G, 6.5, 2.1, 0.5, 0.5); }
  col(d.sedan(27.0, G, 9.6, Math.PI, 'brown', { dusty: true }));
  col(d.pickup(31.0, G, 9.9, Math.PI + 0.04, 'white', { tools: true }));
  // drums at the wing's west end
  col(d.drum(21.3, G, 6.75, 'rust')); col(d.drum(21.95, G, 6.6, 'drumBlue')); col(d.drum(22.3, G, 7.5, 'rust', { lying: true, yaw: 0.3 }));
  // behind the wing: a cable reel on its rim, pallets, tyres
  {
    const x = 35.5, z = -7.0, r = 0.65, g = d.piece(x, G, z, 0, false);
    for (const dx of [-0.38, 0.38]) d.cyl(g, r, r, 0.06, 'woodNew', dx, r, 0, 14, 0, Math.PI / 2);
    d.cyl(g, 0.32, 0.32, 0.7, 'woodOld', 0, r, 0, 10, 0, Math.PI / 2);
    d.cyl(g, 0.42, 0.42, 0.66, 'black', 0, r, 0, 12, 0, Math.PI / 2);   // the cable left on it
    d.contact(x, G, z, 1.2, 1.6, 0.7);
    col({ minX: x - 0.42, maxX: x + 0.42, minZ: z - r, maxZ: z + r });
  }
  col(d.pallet(33.6, G, -6.75, 3, { yaw: 0, fine: false }));
  col(d.tyres(37.4, G, -6.6, 4, { fine: false }));
  // the cans north of the dumpster, clear of the path between it and the propane tank
  col(d.trashCan(0.0, G, 17.9, { k: 'galvDark' })); col(d.trashCan(0.75, G, 17.95, { lidOff: true, k: 'galvDark' }));
  return d;
}
