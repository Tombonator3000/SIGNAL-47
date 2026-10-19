import type { FloodSet } from './kit';
import { Dressing, type Foot } from './weathering';
import { STATION_HUT as HUT, STATION_SHED as SHED, STATION_GROUND as G } from './stationLayout';

// STATION 01 from 1947 (weathering.ts; the concept art station01_field and the cut cable): the
// hut's render forty years in the desert, stained down from the roof slab, blotched, patched and
// peeled, its foot dirty from the yard, the dirt drifted at its plinth and the grass in it, rust
// run down from the meter, the junction box and the lamp's bracket; the generator shed's tin
// rusted along its seams; a pallet against the shed, crates by it, a water drum by the hut, two
// jerrycans by the shed door. In the station's own
// metres (Station01.ts): the hut's plinth at 0, the yard at G. `gy`: the ground as drawn.
export function dressStation(set: FloodSet, col: (f: Foot) => void, gy: (x: number, z: number) => number) {
  const d = new Dressing(set, 0.03, 1);
  d.ground = gy;
  const { x0, x1, z0, z1, t, h } = HUT;
  // the hut stands 0.3 m in from its plinth's edge: the dust lies on that ledge
  const heavy = { streaks: 1.6, patches: 2.6, peel: 0.55, band: 0.6, chips: 0.6, grime: 1.1, drift: 0.2, k: 1.05 };
  d.wall({ face: '+z', at: z1 + t, a0: x0 - t, a1: x1 + t, y0: 0, y1: h }, { ...heavy, openings: [{ a0: -12.3, a1: -11.3, y0: 0.95, y1: 1.85 }, { a0: -10.5, a1: -9.4, y0: 0, y1: 2.0 }], drips: [-12.8, -8.95] });
  d.wall({ face: '+x', at: x1 + t, a0: z0 - t, a1: z1 + t, y0: 0, y1: h }, { ...heavy, openings: [{ a0: 2.3, a1: 3.3, y0: 0.95, y1: 1.85 }], drips: [4.25] });
  d.wall({ face: '-z', at: z0 - t, a0: x0 - t, a1: x1 + t, y0: 0, y1: h }, { ...heavy, drips: [-11.0] });
  d.wall({ face: '-x', at: x0 - t, a0: z0 - t, a1: z1 + t, y0: 0, y1: h }, heavy);
  // the plinth (x -13.5 to -8.5, z 1 to 5) out of the yard, the dirt drifted at it and the
  // grass in it; none on the step at the door, nor where the drum and the cable stand
  const clear = (x: number, z: number) => (x > -11.0 && x < -8.8 && z > 4.6) || (x > -14.3 && x < -13.3 && z > 1.7 && z < 2.8) || (x > -9.4 && z > 4.7);
  const plinth = { band: 0.25, chips: 0.2, streaks: 0, patches: 0, grime: 0.5, drift: 0.55, tufts: 6, clear };
  d.wall({ face: '+z', at: 5.0, a0: -13.5, a1: -8.5, y0: G - 0.05, y1: 0 }, plinth);
  d.wall({ face: '+x', at: -8.5, a0: 1.0, a1: 5.0, y0: G - 0.05, y1: 0 }, plinth);
  d.wall({ face: '-z', at: 1.0, a0: -13.5, a1: -8.5, y0: G - 0.05, y1: 0 }, plinth);
  d.wall({ face: '-x', at: -13.5, a0: 1.0, a1: 5.0, y0: G - 0.05, y1: 0 }, plinth);
  // rust from the meter on the east wall, the junction box and the lamp's bracket by the door
  d.rust(x1 + t, 1.39, 4.25, '+x', 1.0); d.rust(-9.1, 1.09, z1 + t, '+z', 0.9, 0.4); d.rust(-9.95, 2.46, z1 + t, '+z', 0.7, 0.3, 0.6);
  // the shed's tin: rust along it, the dirt and the grass at its foot (not at its door, the
  // pallet or the crates)
  const shedClear = (x: number, z: number) => (x > -15.25 && z > -1.0 && z < 0.2) || (x > -17.0 && x < -16.0 && z > 0.8 && z < 1.7) || (x < -17.3 && z > 0.4 && z < 1.4)
    || (x > -15.2 && z > 0.5 && z < 1.4) || (z < -1.3 && x > -18.4 && x < -15.4);
  // (the shed stands on a concrete pad 0.15 m wider than it: the grass grows beyond it)
  const tin = { streaks: 0, patches: 0, band: 0.5, chips: 0, grime: 0.5, drift: 0, tufts: 5, tuftOut: 0.2, clear: shedClear, k: 0.9 };
  for (const [f, at, a0, a1] of [['-x', SHED.x0, SHED.z0, SHED.z1], ['+x', SHED.x1, SHED.z0, SHED.z1], ['-z', SHED.z0, SHED.x0, SHED.x1], ['+z', SHED.z1, SHED.x0, SHED.x1]] as const) {
    d.wall({ face: f, at, a0, a1, y0: G, y1: G + SHED.h }, tin);
    for (let a = a0 + 0.35; a < a1 - 0.2; a += 0.55 + ((a * 7.3) % 0.4)) {
      const [x, z] = f === '-x' || f === '+x' ? [at, a] : [a, at];
      d.rust(x, G + SHED.h - 0.02, z, f, 0.7 + ((a * 3.1) % 0.9), 0.22, 0.65);
    }
  }

  // ---------- what stands about ----------
  // a pallet leaning on the shed's north wall, crates at its north-west corner
  col(d.pallet(-16.2, G, -1.62, 1, { lean: true, yaw: Math.PI, fine: false }));
  col(d.crate(-17.75, G, -1.85, 0.7, { yaw: 0.15, fine: false })); d.crate(-17.72, G + 0.56, -1.86, 0.5, { yaw: 0.55 });
  // a drum for water by the hut's west wall, two jerrycans by the shed's door
  col(d.drum(-13.82, gy(-13.82, 2.25), 2.25, 'drumBlue'));
  for (const [x, z, k, yaw] of [[-14.82, 0.78, 'drumRed', 0.2], [-14.78, 1.12, 'drumOlive', -0.15]] as const) {
    const g = d.piece(x, G, z, yaw, true);
    d.box(g, 0.17, 0.42, 0.34, k, 0, 0.21, 0); d.contact(x, G, z, 0.35, 0.5, 0.55, yaw); d.box(g, 0.05, 0.04, 0.14, 'black', 0, 0.44, -0.06); d.box(g, 0.04, 0.05, 0.05, 'galvDark', 0, 0.44, 0.12);
    col({ minX: x - 0.14, maxX: x + 0.14, minZ: z - 0.2, maxZ: z + 0.2 });
  }
  return d;
}
