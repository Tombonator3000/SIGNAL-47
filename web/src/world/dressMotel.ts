import * as THREE from 'three';
import type { FloodSet } from './kit';
import { Dressing, type Foot, type Opening } from './weathering';

// Sierra Motor Court from its lot (weathering.ts; the concept art sierra_court and the office's
// window): the rooms' painted front stained under its sills and splashed at its foot, rust from
// the window units, the office's north end weathered where the lot sees it, swamp coolers and
// vent stacks along the roof, an antenna over the office; a drinks machine glowing by the ice
// machine, a news box and a can on the walk, a pickup and a sedan nose in to the rooms (the
// pool and the way past it to room 6 left clear). In the numbers the court was built in
// (MotelFront.ts); its group is moved with the rest. The walk and the lot are at y 0.
export function dressMotel(set: FloodSet, col: (f: Foot) => void) {
  const d = new Dressing(set, 0.055, 6);
  // the rooms' front (x -41.045) from the office to the south end; each room's window at z+2.03
  const rooms = Array.from({ length: 8 }, (_, i) => 25 + (i + 1) * 5);
  const wins: Opening[] = rooms.map((z) => ({ a0: z + 1.33, a1: z + 2.73, y0: 0.78, y1: 1.98 }));
  const doors: Opening[] = rooms.map((z) => ({ a0: z - 0.53, a1: z + 0.53, y0: 0, y1: 2.13 }));
  // under the walk's roof: less run down from the top, the foot splashed from the lot
  d.wall({ face: '+x', at: -41.045, a0: 28.35, a1: 70.0, y0: 0, y1: 2.8 }, { openings: [...wins, ...doors], streaks: 0.45, band: 0.45, chips: 0.4, patches: 0.9, k: 0.85 });
  // the office's front and its window
  d.wall({ face: '+x', at: -41.0, a0: 22.0, a1: 27.8, y0: 0, y1: 2.8 }, { openings: [{ a0: 23.4, a1: 24.8, y0: 0, y1: 2.2 }, { a0: 25.18, a1: 27.22, y0: 0.71, y1: 2.07 }], streaks: 0.5, k: 0.85 });
  // the north end, out in the weather: the office's side and the rear block's
  d.wall({ face: '-z', at: 21.89, a0: -50.0, a1: -41.0, y0: -0.3, y1: 3.0 }, { streaks: 1.4, patches: 1.6, peel: 0.3 });
  // the fascia of the walk's roof, stained where it drips
  for (let z = 22.5; z < 70; z += 2.3 + (z % 3) * 0.4) d.stain('streak', -38.17, 2.8, z, '+x', 0.9, 0.24, 0.8);
  // rust under each window unit, and its drip on the walk
  for (const z of rooms) {
    d.rust(-40.9, 0.2, z + 2.03, '+x', 0.22, 0.5, 0.75);
    d.stainGround('damp', -40.6, 0, z + 2.03, 0.8, 0.9, 0.55);
  }

  // ---------- the roof (its slab's top at 3.01, from x -50 to -40.9) ----------
  for (const [z, x] of [[32.5, -45.6], [42.5, -45.2], [52.5, -45.8], [62.5, -45.4]] as const) d.swampCooler(x, 3.01, z, -Math.PI / 2, 0.95);
  d.swampCooler(-44.2, 3.01, 24.6, -Math.PI / 2, 0.85);
  for (const z of rooms) d.vent(-43.1, 3.01, z + 1.1, { h: 0.42, r: 0.045 });
  d.vent(-48.6, 3.01, 37.5, { turbine: true, h: 0.3, r: 0.1 }); d.vent(-48.4, 3.01, 57.5, { turbine: true, h: 0.3, r: 0.1 });
  d.antenna(-42.4, 3.01, 23.0, 3.4, 0.4);

  // ---------- on the walk ----------
  // the drinks machine by the ice machine, its front to the lot
  col(d.sodaMachine(-40.6, 0, 29.02, Math.PI / 2));
  // a news box at the walk's edge outside the office, a can at the south end, a chair or two
  col(d.newsBox(-39.05, 0, 26.6, Math.PI / 2, 'yellow'));
  col(d.trashCan(-40.75, 0, 69.75, { k: 'galvDark' }));
  for (const [z, yaw] of [[41.1, 1.9], [61.0, 1.3]] as const) col(chair(d, -40.55, z, yaw));

  // ---------- the lot ----------
  // nose in to the rooms between the stripes (z 41 to 46 and 61 to 66), clear of the pool's fence
  // and of the way along the front of the lot to room 6 (x -37.5: tools/motel.py)
  col(d.pickup(-33.9, 0, 43.5, -Math.PI / 2, 'white', { tools: true }));
  col(d.sedan(-34.6, 0, 63.6, -Math.PI / 2 + 0.03, 'maroon'));
  return d;
}

// A metal lawn chair (the shell seat on a tube frame), as outside the rooms in the concept art.
function chair(d: Dressing, x: number, z: number, yaw: number): Foot {
  const g = new THREE.Group(); g.position.set(x, 0, z); g.rotation.y = yaw; d.fine.add(g);
  d.box(g, 0.5, 0.04, 0.48, 'red', 0, 0.42, 0);
  d.box(g, 0.5, 0.42, 0.04, 'red', 0, 0.66, -0.24, -0.15);
  for (const sx of [-0.23, 0.23]) { d.box(g, 0.03, 0.42, 0.03, 'chrome', sx, 0.21, 0.2); d.box(g, 0.03, 0.03, 0.5, 'chrome', sx, 0.02, 0); d.box(g, 0.03, 0.42, 0.03, 'chrome', sx, 0.21, -0.22); }
  return { minX: x - 0.3, maxX: x + 0.3, minZ: z - 0.3, maxZ: z + 0.3 };
}
