import * as THREE from 'three';
import type { FloodSet } from './kit';
import { Dressing, type Foot, type Opening, type Wall } from './weathering';

// Sierra Motor Court from its lot (weathering.ts; the concept art sierra_court and the office's
// window): the rooms' painted front grimy under the walk's roof and round the doors, stained
// under its sills and splashed at its foot, rust from the window units and their drips on the
// walk, the office's north end weathered where the lot sees it, swamp coolers and vent stacks
// along the roof, an antenna over the office; a drinks machine glowing by the ice machine, a news
// box and a can on the walk, two webbed lawn chairs, a pickup and a sedan nose in to the rooms
// (the pool and the way past it to room 6 left clear). In the numbers the court was built in
// (MotelFront.ts); its group is moved with the rest. The walk and the lot are at y 0.
export function dressMotel(set: FloodSet, col: (f: Foot) => void) {
  const d = new Dressing(set, 0.055, 6);
  // the rooms' front (x -41.045) from the office to the south end; each room's window at z+2.03
  const rooms = Array.from({ length: 8 }, (_, i) => 25 + (i + 1) * 5);
  const wins: Opening[] = rooms.map((z) => ({ a0: z + 1.33, a1: z + 2.73, y0: 0.78, y1: 1.98 }));
  const doors: Opening[] = rooms.map((z) => ({ a0: z - 0.53, a1: z + 0.53, y0: 0, y1: 2.13 }));
  // under the walk's roof: less run down from the top, but its shade dark under the roof and
  // the foot splashed from the lot; the scuppers' water comes down at the posts
  const front: Wall = { face: '+x', at: -41.045, a0: 28.35, a1: 70.0, y0: 0, y1: 2.8 };
  d.wall(front, { openings: [...wins, ...doors], streaks: 0.3, coping: 0.7, band: 0.45, chips: 0.4, patches: 0.9, grime: 1.0, drift: 0.3, k: 0.9 });
  // the office's front and its window
  d.wall({ face: '+x', at: -41.0, a0: 22.0, a1: 27.8, y0: 0, y1: 2.8 }, { openings: [{ a0: 23.4, a1: 24.8, y0: 0, y1: 2.2 }, { a0: 25.18, a1: 27.22, y0: 0.71, y1: 2.07 }], streaks: 0.3, coping: 0.6, grime: 0.7, drift: 0.3, k: 0.85 });
  // the north end, out in the weather: the office's side and the rear block's; the brush at it
  // (its land lies a little under the walk, on the footing's edge)
  d.wall({ face: '-z', at: 21.89, a0: -50.0, a1: -41.0, y0: -0.3, y1: 3.0 }, { streaks: 1.2, patches: 1.6, peel: 0.3, drips: [-48.6, -44.2], grime: 1.0, drift: 0.6, tufts: 5, ground: -0.065 });
  // the fascia of the walk's roof, stained where it drips
  for (let z = 22.5; z < 70; z += 2.3 + (z % 3) * 0.4) d.stain('streak', -38.17, 2.8, z, '+x', 0.9, 0.24, 0.8);
  // round each door: hands at the latch side, the grime of the frame
  for (const z of rooms) {
    d.stain('smudge', -41.045, 1.05, z - 0.68, '+x', 0.22, 1.0, 0.55);
    d.stain('smudge', -41.045, 1.15, z + 0.66, '+x', 0.2, 1.6, 0.3);
  }
  // rust under each window unit, and its drip on the walk: a dark wet line under its edge
  for (const z of rooms) {
    d.rust(-40.9, 0.2, z + 2.03, '+x', 0.22, 0.55, 0.6);
    d.stainGround('damp', -40.6, 0, z + 2.03, 0.8, 0.9, 0.55);
    d.stainGround('contact', -40.72, 0, z + 2.03, 0.32, 0.9, 0.5);
  }
  // the rooms' windows: a frame round the glass (the sills are MotelFront's)
  for (const op of wins) d.window(front, op, { frame: 'trimGreen', depth: 0.018 });

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
  col(d.pickup(-33.9, 0, 43.5, -Math.PI / 2, 'tan', { tools: true }));
  col(d.sedan(-34.6, 0, 63.6, -Math.PI / 2 + 0.03, 'maroon'));
  return d;
}

// A folding lawn chair as outside the rooms in the concept art: an aluminium tube frame, the
// webbing woven over the seat and the back in two colours, flat arms.
function chair(d: Dressing, x: number, z: number, yaw: number): Foot {
  const g = d.piece(x, 0, z, yaw, true), V = (a: number, b: number, c: number) => new THREE.Vector3(a, b, c), t = 0.011;
  // the back leans back at this slope; its rails run on up from the seat's rear
  const back = (y: number) => -0.21 - (y - 0.36) * 0.28;
  for (const sx of [-0.24, 0.24]) {
    d.rod(g, V(sx, 0, 0.2), V(sx, 0.6, 0.2), t, 'chrome');               // the front leg, up to the arm
    d.rod(g, V(sx, 0, -0.24), V(sx, 0.36, -0.21), t, 'chrome');          // the back leg
    d.rod(g, V(sx, 0.36, 0.22), V(sx, 0.36, -0.21), t, 'chrome');        // the seat's side
    d.rod(g, V(sx, 0.36, -0.21), V(sx, 0.86, back(0.86)), t, 'chrome');  // the back's side
    d.box(g, 0.045, 0.014, 0.42, 'chrome', sx, 0.607, 0.0);              // the flat arm
    d.rod(g, V(sx, 0.6, -0.2), V(sx, 0.6, back(0.6)), t, 'chrome');
  }
  for (const [y, zz] of [[0.36, 0.22], [0.86, back(0.86)], [0.04, 0.2], [0.04, -0.24]]) d.rod(g, V(-0.24, y, zz), V(0.24, y, zz), t, 'chrome');
  // the webbing: straps front to back on the seat, across on the back
  for (let i = 0; i < 6; i++) d.box(g, 0.06, 0.006, 0.43, i % 2 ? 'cream' : 'red', -0.2 + i * 0.08, 0.364, 0.005);
  for (let i = 0; i < 5; i++) { const y = 0.42 + i * 0.095; d.box(g, 0.48, 0.06, 0.006, i % 2 ? 'red' : 'cream', 0, y, back(y), -0.27); }
  d.contact(x, 0, z, 0.75, 0.75, 0.45, yaw);
  return { minX: x - 0.3, maxX: x + 0.3, minZ: z - 0.3, maxZ: z + 0.3 };
}
