import type { FloodSet } from './kit';
import { Dressing, type Face, type Foot } from './weathering';
import { rng } from '../core/textures';

// Caliche's houses and shops from the street (weathering.ts; the concept art of the main street,
// the payphone and the laundromat): every wall grimy to its foot, stained down from its parapet
// and splashed up its footing, the dirt drifted against it and the brush in it, about one in
// three worn hard; the swamp coolers and the stacks on the roofs, the antennas on the houses, a
// meter and a downspout or two, each building's in its own places; at the gas station the tyres, the drums and the drinks machine
// by the office, at the laundromat a machine and a can, at the feed store the pallets and the
// crates; the cars in the driveways, porch lights on at two houses. In the town's road-local
// metres (Town.ts); `built` is every building Town.building() made.
type Built = { x0: number; x1: number; z0: number; z1: number; floor: number; top: number; fx: number; s: 1 | -1 };
export type TownDress = {
  set: FloodSet; fall: number; built: Built[];
  /** A collider (truck: the truck's obstacle too). */
  col: (f: Foot, truck?: boolean) => void;
  gy: (x: number, z: number) => number;
  glow: (x: number, y: number, z: number, size: number, color: number) => void;
};

export function dressTown(t: TownDress) {
  const d = new Dressing(t.set, t.fall, 412);
  const { gy, col } = t;
  // on the concrete laid over the land (Town.ground and the gas station's lot), or on the land
  const DRAPES = [[-17.6, -14.2, 2016, 2040], [-17.6, -14.2, 2090, 2250], [16.6, 20.2, 2024, 2200], [-17.6, -5.6, 2040, 2090]];
  const pave = (x: number, z: number) => gy(x, z) + (DRAPES.some(([x0, x1, z0, z1]) => x >= x0 && x <= x1 && z >= z0 && z <= z1) ? 0.055 : 0);
  // a face another building stands against (the gas station's office and its bay)
  const shared = (b: Built, f: Face) => t.built.some((q) => q !== b && (f === '+z' ? q.z0 === b.z1 : f === '-z' ? q.z1 === b.z0 : f === '+x' ? q.x0 === b.x1 : q.x1 === b.x0)
    && (f === '+z' || f === '-z' ? q.x0 < b.x1 && q.x1 > b.x0 : q.z0 < b.z1 && q.z1 > b.z0));
  d.ground = pave;
  const onPave = (x: number, z: number) => DRAPES.some(([x0, x1, z0, z1]) => x >= x0 - 0.3 && x <= x1 + 0.3 && z >= z0 - 0.3 && z <= z1 + 0.3);
  // each building its own wear and its own fixtures, the same every night: about one in three
  // worn hard, the rest lightly (the concept's street: no two fronts alike)
  const rr = rng(1947);
  const pick = <T,>(xs: T[]) => xs[Math.floor(rr() * xs.length)];
  let n = 0;
  for (const b of t.built) {
    const h = b.top - b.floor, house = Math.abs(h - 3.0) < 0.01, tin = h > 5 && h < 5.3, church = h > 5.3 && h < 5.5;
    const cx = (b.x0 + b.x1) / 2, cz = (b.z0 + b.z1) / 2, back = b.s > 0 ? b.x0 : b.x1, side: Face = b.s > 0 ? '+x' : '-x', backFace: Face = b.s > 0 ? '-x' : '+x';
    const heavy = rr() < 0.36, w = b.x1 - b.x0, dz = b.z1 - b.z0;
    // the walls from the floor to the parapet's top (Town.building's lip); the front less, it is painted
    const top = b.top + (tin ? 0.2 : h > 6 ? 0.5 : house || church ? 0.3 : 0.35);
    // the fixtures first, so the water runs where they are: the meter on one side (near the
    // front, the middle or the back), one or two downspouts at the corners, the cooler somewhere
    // on the back half of the roof
    const sides = (['-z', '+z'] as Face[]).filter((f) => !shared(b, f));
    const mf = sides.length ? pick(sides) : null, mx = pick([b.fx - b.s * 1.6, cx + b.s * rr() * 1.5, back + b.s * 1.4]);
    const corners: [Face, number][] = [];
    for (const f of sides) corners.push([f, back + b.s * 0.3], [f, b.fx - b.s * 0.35]);
    if (!shared(b, backFace)) corners.push([backFace, b.z0 + 0.3], [backFace, b.z1 - 0.3]);
    const spouts: [Face, number][] = [];
    for (let k = 0; k < (rr() < 0.4 ? 2 : 1) && corners.length; k++) {
      const c = corners.splice(Math.floor(rr() * corners.length), 1)[0];
      if (!(c[0] === mf && Math.abs(c[1] - mx) < 1.2)) spouts.push(c);
    }
    const cool = !tin && !church ? { x: cx + (back - cx) * (0.15 + rr() * 0.5), z: cz + (rr() - 0.5) * dz * 0.6 } : null;
    const faces: [Face, number, number, number][] = [['-x', b.x0, b.z0, b.z1], ['+x', b.x1, b.z0, b.z1], ['-z', b.z0, b.x0, b.x1], ['+z', b.z1, b.x0, b.x1]];
    for (const [f, at, a0, a1] of faces) {
      if (shared(b, f)) continue;
      const front = f === side, along = f === '-z' || f === '+z';
      const drips = spouts.filter((q) => q[0] === f).map((q) => q[1]);
      // the cooler's bleed water runs to the nearest edge
      if (cool && along && Math.abs((f === '-z' ? b.z0 : b.z1) - cool.z) < dz * 0.4) drips.push(cool.x);
      d.wall({ face: f, at, a0, a1, y0: b.floor, y1: top }, {
        streaks: (heavy ? 1.1 : 0.4) * (front ? 0.6 : 1), drips, patches: tin ? 0.2 : (heavy ? 1.4 : 0.5) * (front ? 0.5 : 1), peel: house ? (heavy ? 0.45 : 0.25) : 0.15,
        chips: tin ? 0 : heavy ? 0.5 : 0.3, band: heavy ? 0.5 : 0.35, grime: heavy ? 1.2 : 0.7, k: (tin ? 0.7 : heavy ? 1.0 : 0.8),
        drift: front ? 0.35 : 0.55, tufts: front ? 0 : heavy ? 4.5 : 2.5, clear: onPave,
      });
      // the footing, 3 cm proud of the wall, splashed up from where the ground meets it
      const len = a1 - a0, m = Math.max(1, Math.round(len / 2.4)), s = len / m;
      for (let i = 0; i < m; i++) {
        const a = a0 + s * (i + 0.5), [x, z] = f === '-x' || f === '+x' ? [at + (f === '+x' ? 0.03 : -0.03), a] : [a, at + (f === '+z' ? 0.03 : -0.03)];
        const g = Math.min(b.floor - 0.1, gy(x, z));
        d.stain('band', x, (g + b.floor) / 2 + 0.02, z, f, s + 0.01, b.floor - g + 0.08, 0.8);
      }
    }
    // the roof: a cooler on all but the tin shed and the church, the stacks, an antenna on a house
    const ry = b.top + 0.04;
    if (cool) {
      d.swampCooler(cool.x, ry, cool.z, pick([Math.PI / 2, -Math.PI / 2, 0, Math.PI]), house ? 0.85 : 1);
      if (dz > 18) d.swampCooler(cx + (b.fx - cx) * 0.2, ry, cz + dz * (cool.z > cz ? -0.28 : 0.28), b.s > 0 ? Math.PI / 2 : -Math.PI / 2);
    }
    d.vent(cx + w * (rr() * 0.5 - 0.1), ry, cz - dz * (0.15 + rr() * 0.2), { h: 0.4 + rr() * 0.25, r: 0.05 });
    if (tin) for (const k of [-0.3, 0, 0.3]) d.vent(cx, ry, cz + dz * k, { turbine: true, h: 0.35, r: 0.12 });
    else if (!house) d.vent(cx - w * 0.2, ry, cz + dz * (0.2 + rr() * 0.15), { turbine: rr() < 0.5, h: 0.3, r: 0.08 });
    if (house) d.antenna(cx + (back - cx) * (0.3 + rr() * 0.4), ry, cz + dz * (rr() - 0.5) * 0.6, 3.6 + (n % 3) * 0.5, n * 0.7);
    // the meter and its conduit up to the parapet, the rust run from it; the downspouts
    if (mf && !church) {
      const mz = mf === '-z' ? b.z0 : b.z1;
      d.meter(mx, b.floor + 1.5, mz, mf, top - b.floor - 1.75, true); d.rust(mx, b.floor + 1.25, mz, mf, 0.8, 0.42, 0.6);
    }
    for (const [f, a] of spouts) {
      if (f === '-z' || f === '+z') { const sz = f === '-z' ? b.z0 : b.z1; d.downspout(a, top - 0.15, Math.min(b.floor, gy(a, sz)), sz, f, 'galvDark'); }
      else d.downspout(back, top - 0.15, Math.min(b.floor, gy(back, a)), a, f, 'galvDark');
    }
    n++;
  }

  // ---------- the gas station (office x -27 to -17, the bay to 2088; its front to the street) ----------
  col(d.tyres(-16.9, pave(-16.9, 2086.4), 2086.4, 5, { lean: true, fine: false }));
  col(d.drum(-16.75, pave(-16.75, 2078.65), 2078.65, 'rust'));
  col(d.drum(-16.6, pave(-16.6, 2079.4), 2079.4, 'drumOlive', { open: true }));
  col(d.sodaMachine(-16.6, pave(-16.6, 2076.5), 2076.5, Math.PI / 2));
  col(d.newsBox(-16.55, pave(-16.55, 2065.6), 2065.6, Math.PI / 2, 'yellow'));
  col(d.drum(-10.25, pave(-10.25, 2046.0), 2046.0, 'drumBlue', { open: true }));   // the island's trash barrel
  d.stainGround('damp', -10.25, pave(-10.25, 2052.4), 2052.4, 1.6, 2.2, 0.6); d.stainGround('damp', -10.25, pave(-10.25, 2057.6), 2057.6, 1.6, 2.2, 0.6);
  d.stainGround('damp', -15.6, pave(-15.6, 2083), 2083, 2.6, 3.2, 0.55);   // oil in front of the bay
  // ---------- the laundromat (x -25 to -15, z 2126 to 2140): a machine by the bench, a can by the door ----------
  col(d.sodaMachine(-14.6, pave(-14.6, 2134.6), 2134.6, Math.PI / 2));
  col(d.trashCan(-14.7, pave(-14.7, 2139.05), 2139.05, { k: 'galvDark' }));
  // its dryers vent out of the north wall, the lint and the damp round each hood
  {
    const b = t.built.find((q) => q.x0 === -25 && q.z0 === 2126);
    if (b) for (const x of [-23.2, -21.6, -20.0]) {
      d.box(d.core, 0.26, 0.2, 0.18, 'galv', x, b.floor + 2.3, 2125.91);
      d.stain('soot', x, b.floor + 2.75, 2126, '-z', 0.55, 0.9, 0.55); d.stain('damp', x, b.floor + 1.9, 2126, '-z', 0.7, 0.6, 0.6);
    }
  }
  // ---------- the feed store (x -31 to -15, its dock at 2196 to 2206) ----------
  col(d.pallet(-13.5, pave(-13.5, 2192.5), 2192.5, 4, { yaw: 0.06, fine: false }));
  {
    const y = pave(-13.6, 2207.4);
    col(d.crate(-13.6, y, 2207.4, 0.8, { yaw: 0.1, fine: false })); d.crate(-13.6, y + 0.64, 2207.4, 0.6, { yaw: 0.5 });
    col(d.crate(-12.85, pave(-12.85, 2207.6), 2207.6, 0.6, { yaw: -0.2, fine: false }));
  }
  col(d.drum(-14.4, pave(-14.4, 2215.0), 2215.0, 'rust')); col(d.drum(-13.75, pave(-13.75, 2215.25), 2215.25, 'drumBlue', { lying: true, yaw: 0.2 }));
  // the bulb over its door throws a little on the wall and the dock
  {
    const b = t.built.find((q) => q.x0 === -31 && q.z0 === 2190);
    if (b) { d.pool(b.fx, b.floor + 3.4, 2201.5, '+x', 3.4, 2.8, 0xffc070, 0.3); d.pool(b.fx + 1.3, b.floor + 0.95, 2201.5, 'up', 2.4, 3.2, 0xffc070, 0.2); }
  }
  // ---------- the east side, set back past the sidewalk (x 19.5): nobody walks there ----------
  // Earl's car beside the studio, crates and a drum behind the souvenir shop
  col(d.sedan(24.6, gy(24.6, 2027.5), 2027.5, Math.PI / 2 + 0.04, 'teal', { dusty: true }), true);
  col(d.crate(21.6, gy(21.6, 2071.3), 2071.3, 0.7), false); col(d.crate(22.5, gy(22.5, 2071.6), 2071.6, 0.55, { yaw: 0.6 }), false);
  col(d.drum(23.3, gy(23.3, 2071.2), 2071.2, 'rust'), false);
  // the houses: the cars in the driveways, a can by the corner, a hose, the porch lights
  col(d.pickup(26.5, gy(26.5, 2140.3), 2140.3, -Math.PI / 2, 'green', { two: 'white' }), true);
  col(d.sedan(27.2, gy(27.2, 2177.6), 2177.6, -Math.PI / 2 + 0.05, 'beige', { wagon: true }), true);
  col(d.pickup(-23.5, gy(-23.5, 2233.8), 2233.8, Math.PI / 2, 'maroon'), true);
  for (const [x, z] of [[23.4, 2137.3], [24.4, 2190.75], [-18.4, 2246.6]]) col(d.trashCan(x, gy(x, z), z, { lidOff: z > 2200 }), false);
  d.hose(23.2, gy(23.2, 2129.2), 2129.2);
  for (const b of t.built.filter((q) => Math.abs(q.top - q.floor - 3.0) < 0.01)) {
    const wz = b.z0 === 2128 ? 2136.4 : b.z0 === 2180 ? 2181.6 : b.z0 === 2236 ? 2244.4 : null;
    if (wz === null) continue;   // the cream house's porch light is the smoker's (Town.people)
    const f: Face = b.s > 0 ? '+x' : '-x', x = b.fx, lz = wz + (b.z0 === 2236 ? 0 : 0.8);
    // (the house with the kitchen lit has its glow already: only the fixture and its light)
    d.porchLight(x, b.floor + (b.z0 === 2236 ? 2.45 : 2.25), lz, f, gy(x + b.s * 1.2, lz), b.z0 === 2236 ? undefined : (px, py, pz) => { t.glow(px, py, pz, 0.42, 0xffd890); t.glow(px, py, pz, 2.6, 0x2a1a0a); });
  }
  return d;
}
