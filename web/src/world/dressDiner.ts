import type { FloodSet } from './kit';
import { Dressing, type Foot, type Opening } from './weathering';

// Mesa Diner from its lot (weathering.ts): the panelled walls stained under the roof's edge and
// splashed from the gravel, the cooler and the kitchen's hood on the roof, a news box and a
// bench on the walk, the drinks machine glowing at its south end, a barrel for the trash by
// the door; the cook's car on the south side and a pickup on the north, out of the lot where
// the truck is left. In the diner's own metres (Diner.ts): the floor and the walk at 0, the
// land round it from `gy` (the road's, World.ts).
export function dressDiner(set: FloodSet, col: (f: Foot) => void, gy: (x: number, z: number) => number) {
  const d = new Dressing(set, 0.14, 1947);
  // the front (x 2): the long windows from 0.85 to 2.57, the door, the pier beside it
  const front: Opening[] = [{ a0: -9, a1: -0.6, y0: 0.85, y1: 2.57 }, { a0: -0.6, a1: 0.6, y0: 0, y1: 2.24 }, { a0: 2.15, a1: 9, y0: 0.85, y1: 2.57 }];
  d.wall({ face: '+x', at: 2.0, a0: -9, a1: 9, y0: 0, y1: 3.2 }, { openings: front, streaks: 0.8, patches: 0.4, k: 0.8 });
  d.wall({ face: '-z', at: -9.0, a0: -8, a1: 2, y0: -0.14, y1: 3.2 }, { streaks: 1.3, patches: 1.2, k: 0.9 });
  d.wall({ face: '+z', at: 9.0, a0: -8, a1: 2, y0: -0.14, y1: 3.2 }, { streaks: 1.3, patches: 1.2, k: 0.9 });
  d.wall({ face: '-x', at: -8.0, a0: -9, a1: 9, y0: -0.14, y1: 3.2 }, { streaks: 1.1, k: 0.9 });
  // grease run down the back wall under the kitchen's hood
  d.stain('streak', -8.0, 2.4, -2.4, '-x', 1.1, 1.6, 1, 2, 0x8a7050);

  // ---------- the roof (its slab's top at 3.4) ----------
  d.swampCooler(-5.0, 3.4, 5.2, -Math.PI / 2, 1.05);
  // the kitchen's exhaust over the hatch: a hood on a curb, a cap
  d.box(d.core, 0.9, 0.35, 0.9, 'galvDark', -6.8, 3.58, -2.4); d.box(d.core, 1.1, 0.06, 1.1, 'galv', -6.8, 3.98, -2.4);
  d.cyl(d.core, 0.22, 0.22, 0.3, 'galv', -6.8, 3.84, -2.4, 10);
  d.vent(-7.1, 3.4, 6.6, { h: 0.5 }); d.vent(-6.2, 3.4, -7.1, { h: 0.6, r: 0.05 }); d.vent(-2.5, 3.4, -5.0, { turbine: true, h: 0.3, r: 0.1 });

  // ---------- the walk (x 2 to 3.6) ----------
  col(d.newsBox(3.25, 0, 1.45, Math.PI / 2, 'newsBlue'));
  col(d.bench(2.42, 0, 5.0, Math.PI / 2, 1.8));
  col(d.sodaMachine(2.56, 0, 8.3, Math.PI / 2));
  col(d.drum(3.3, 0, -1.45, 'drumOlive', { open: true }));
  // ---------- beside the house, off the lot ----------
  const yS = gy(-3.4, 11.7), yN = gy(-3.0, -11.6);
  col(d.sedan(-3.4, yS, 11.7, -Math.PI / 2, 'paleBlue', { dusty: true }));
  col(d.pickup(-3.0, yN, -11.6, Math.PI / 2 + 0.05, 'gold', { two: 'white' }));
  // the kitchen's cans and a grease drum at the south-west corner
  col(d.trashCan(-7.2, gy(-7.2, 9.75), 9.75, { k: 'galv' })); col(d.trashCan(-6.5, gy(-6.5, 9.8), 9.8, { lidOff: true }));
  col(d.drum(-8.6, gy(-8.6, 8.2), 8.2, 'rust'));
  return d;
}
