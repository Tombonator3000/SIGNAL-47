import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, addFlood } from '../world/kit';
import { rng } from '../core/textures';
import { track, trackHalf, trackNearest, surfaceY, hash, DINER } from './roadTerrain';
import { stationToRoad } from '../world/geo';
import { STATION_FENCE, STATION_GATE, STATION_HUT, STATION_SHED, STATION_POLE } from '../world/stationLayout';
import { horizonGlowTex } from './roadTextures';
import { type Build, type Keep, roadFlood, colored, instances, place, circle, wall } from './roadProps';

// Places off the road: the station gate at the end of the track, a ranch far to the east,
// the desert plants and rocks, the mesas on the horizon and SARO's lights behind the start.

// ---------- STATION 01 from the road: its fence, gate, hut, shed and yard light ----------
// Only what is seen from far off at night. The station itself is its own area (World.ts),
// and the truck goes into it 330 m out (legs.ts); this far view is built from the station's
// own plan (stationLayout.ts), placed as it lies on the map (geo.ts), so nothing moves when
// the real station takes over.
export function station(b: Build) {
  const s = b.statics, m = b.m, y = (x: number, z: number) => surfaceY(x, z);
  const P = (x: number, z: number) => stationToRoad(x, z);
  // a station box (x0..x1, z0..z1, h high) as a road box
  const sbox = (x0: number, x1: number, z0: number, z1: number, h: number, mat: THREE.Material, lift = 0) => {
    const [ax, az] = P(x0, z0), [bx, bz] = P(x1, z1), cx = (ax + bx) / 2, cz = (az + bz) / 2;
    return box(s, Math.abs(bx - ax), h, Math.abs(bz - az), mat, cx, y(cx, cz) + lift + h / 2, cz);
  };
  const fence = (sx0: number, sz0: number, sx1: number, sz1: number) => {
    const [ax, az] = P(sx0, sz0), [bx, bz] = P(sx1, sz1);
    const len = Math.hypot(bx - ax, bz - az), g = new THREE.PlaneGeometry(len, 2.1);
    const uv = g.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * len / 0.7, uv.getY(i) * 3);
    const p = new THREE.Mesh(g, m.fence);
    p.position.set((ax + bx) / 2, y((ax + bx) / 2, (az + bz) / 2) + 1.05, (az + bz) / 2);
    p.rotation.y = Math.atan2(-(bz - az), bx - ax);
    s.add(p);
    for (let i = 0; i <= Math.ceil(len / 3); i++) {
      const t = i / Math.ceil(len / 3), px = ax + (bx - ax) * t, pz = az + (bz - az) * t;
      cyl(s, 0.035, 0.035, 2.3, m.steel, px, y(px, pz) + 1.1, pz, 6);
    }
    rod(s, new THREE.Vector3(ax, y(ax, az) + 2.12, az), new THREE.Vector3(bx, y(bx, bz) + 2.12, bz), 0.025, m.steel, 5);
    wall(b, Math.min(ax, bx) - 0.2, Math.max(ax, bx) + 0.2, Math.min(az, bz) - 0.2, Math.max(az, bz) + 0.2);
  };
  const F = STATION_FENCE, Gt = STATION_GATE, WALK = -23.0;
  fence(F.x0, F.z1, WALK, F.z1); fence(Gt.e, F.z1, F.x1, F.z1);          // the south side: the walk gate stands open
  fence(F.x1, F.z1, F.x1, F.z0); fence(F.x1, F.z0, F.x0, F.z0); fence(F.x0, F.z0, F.x0, F.z1);
  // the vehicle gate, chained shut
  const [gx0, gz0] = P(Gt.w, F.z1), [gx1, gz1] = P(Gt.e, F.z1), gy = y(gx0, (gz0 + gz1) / 2);
  for (const [za, zb] of [[gz0, (gz0 + gz1) / 2], [(gz0 + gz1) / 2, gz1]]) {
    cyl(s, 0.07, 0.07, 2.5, m.steel, gx0, gy + 1.2, za, 8);
    for (const h of [0.15, 1.95]) rod(s, new THREE.Vector3(gx0, gy + h, za), new THREE.Vector3(gx0, gy + h, zb), 0.03, m.steel, 6);
    const leaf = new THREE.Mesh(new THREE.PlaneGeometry(Math.abs(zb - za) - 0.1, 1.8), m.fence);
    leaf.position.set(gx0, gy + 1.05, (za + zb) / 2); leaf.rotation.y = Math.PI / 2;
    const uv = leaf.geometry.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, uv.getX(i) * 3.4, uv.getY(i) * 2.6);
    s.add(leaf);
  }
  wall(b, gx0 - 0.2, gx0 + 0.2, Math.min(gz0, gz1), Math.max(gz0, gz1));
  // the hut (stucco, a flat roof), the generator shed, the pole of the yard light
  const H = STATION_HUT, S = STATION_SHED;
  sbox(H.x0, H.x1, H.z0, H.z1, H.h, m.concrete);
  sbox(H.x0 - 0.3, H.x1 + 0.3, H.z0 - 0.3, H.z1 + 0.3, 0.2, m.concrete, H.h);
  sbox(S.x0, S.x1, S.z0, S.z1, S.h, m.wood);
  const [px, pz] = P(STATION_POLE.x, STATION_POLE.z), py = y(px, pz);
  cyl(s, 0.1, 0.13, 6.6, m.wood, px, py + 3.1, pz, 7);
  // the yard light (flood slot 4) and the lit windows and door lamp of the hut
  const [lx, lz] = P(STATION_POLE.x - 1.05, STATION_POLE.z - 0.3), ly = py + 6.1;
  addFlood(lx, ly - 0.3, lz, 9, 0xffc27a, roadFlood);
  b.glow.add(lx, ly + 0.05, lz, 1.6, 0xffc07a);
  for (const [sx, sy, sz, size, c] of [[-11.8, 1.4, 4.78, 1.1, 0x6a4018], [-8.72, 1.4, 2.8, 1.1, 0x6a4018], [-9.95, 2.4, 5.0, 0.75, 0xffc27a]] as const) {
    const [wx, wz] = P(sx, sz);
    b.glow.add(wx, y(wx, wz) + sy, wz, size, c);
  }
  circle(b, px, pz, 0.25);
}

// ---------- a ranch 400 m east: house, barn, windmill, tank, and its yard light (slot 3) ----------
export function ranch(b: Build) {
  const s = b.statics, m = b.m, y = (x: number, z: number) => surfaceY(x, z);
  const X = 440, Z = 196;
  const shed = (x: number, z: number, w: number, d: number, h: number, wallM: THREE.Material) => {
    const g = new THREE.Group(); g.position.set(x, y(x, z), z); g.rotation.y = 0.12;
    box(g, w, h, d, wallM, 0, h / 2 - 0.2, 0);
    for (const sgn of [-1, 1]) { const r = box(g, w + 0.6, 0.12, d / 2 + 0.6, m.steel, 0, h + 0.55, sgn * d / 4.6); r.rotation.x = sgn * 0.42; }
    s.add(g);
  };
  shed(X - 22, Z - 4, 12, 8, 3, m.paint);
  shed(X + 6, Z - 26, 16, 11, 5.5, m.wood);
  cyl(s, 3, 3, 2.4, m.steel, X + 22, y(X + 22, Z + 4) + 1.0, Z + 4, 14);
  const wx = X + 16, wz = Z + 18, wy = y(wx, wz);
  for (const [dx, dz] of [[-1, -1], [1, -1], [1, 1], [-1, 1]]) rod(s, new THREE.Vector3(wx + dx * 1.1, wy, wz + dz * 1.1), new THREE.Vector3(wx + dx * 0.2, wy + 9, wz + dz * 0.2), 0.05, m.steel, 4);
  const wheel = cyl(s, 1.3, 1.3, 0.06, m.steel, wx, wy + 9.3, wz - 0.4, 14); wheel.rotation.x = Math.PI / 2;
  box(s, 0.06, 0.6, 1.6, m.steel, wx, wy + 9.3, wz + 0.8);
  // yard light: mercury vapour, a cold blue-white
  const lx = X - 8, lz = Z + 8, ly = y(lx, lz);
  cyl(s, 0.12, 0.15, 8.2, m.wood, lx, ly + 3.9, lz, 7);
  box(s, 0.9, 0.08, 0.08, m.steel, lx + 0.45, ly + 7.9, lz);
  addFlood(lx + 0.9, ly + 7.4, lz, 22, 0xc8e4ff, roadFlood);
  ranchLights(b);
}
/** The ranch's yard light and a lit window, as glows: seen from far off (also from SARO and
 *  from the station, corridors.ts, since glows are not fogged). */
export function ranchLights(b: Build) {
  const y = (x: number, z: number) => surfaceY(x, z), X = 440, Z = 196, lx = X - 8, lz = Z + 8, ly = y(lx, lz);
  b.glow.add(lx + 0.9, ly + 7.75, lz, 1.3, 0xdcefff); b.glow.add(lx + 0.9, ly + 7.75, lz, 9, 0x1c2a36);
  b.glow.add(X - 22 + 6.05, y(X - 16, Z - 4) + 1.4, Z - 3, 0.9, 0xffb866); // a lit window
}

// ---------- creosote, rocks and yucca, thick where the headlights pass ----------
export function plants(b: Build, keep: Keep = () => true) {
  const r = rng(77);
  const bush: THREE.Matrix4[] = [], rock: THREE.Matrix4[] = [], yucca: THREE.Matrix4[] = [];
  const tints: THREE.Color[] = [];
  const free = (x: number, z: number) => {
    if (Math.abs(x) < 9.5) return false;
    const t = trackNearest(x, z);
    if (t.d < trackHalf(t.s) + 1.8) return false;
    if (x > -672 && x < -598 && z > 500 && z < 566) return false;   // station
    if (x > 395 && x < 485 && z > 150 && z < 235) return false;     // ranch yard
    if (x > DINER.x - 30 && x < -3 && z > DINER.z - 45 && z < DINER.z + 45) return false;   // the diner
    return !(x > -26 && x < -14 && z > 510 && z < 530);             // cattle guard
  };
  let made = 0;
  const add = (rr: () => number, x: number, z: number, d: number) => {
    // the same random numbers are drawn whether or not the plant is kept, so a stretch
    // drawn in another area has the very same plants
    const k = rr(), y = surfaceY(x, z), here = keep(x, z);
    if (k < 0.72) {
      const sc = 0.45 + rr() * 1.0;
      const m = place(x, y - 0.08 * sc, z, rr() * 6, sc, 0, sc * (0.7 + rr() * 0.5));
      const c = new THREE.Color().setHSL(0.15 + rr() * 0.06, 0.22 + rr() * 0.15, 0.27 + rr() * 0.13);
      made++;
      if (here) { bush.push(m); tints.push(c); }
    } else if (k < 0.96) {
      const sc = rr() < 0.85 ? 0.15 + rr() * 0.45 : 0.6 + rr() * 0.8;
      const m = place(x, y - 0.12 * sc, z, rr() * 6, sc, (rr() - 0.5) * 0.5);
      if (here) { rock.push(m); if (sc > 0.7 && d < 40) circle(b, x, z, 0.4 * sc); }
    } else { const m = place(x, y - 0.05, z, rr() * 6, 0.8 + rr() * 0.5); if (here) yucca.push(m); }
  };
  // half of the candidates fall beside the way (where the headlights pass), half anywhere
  const P = track.pts;
  for (let n = 0; n < 30000 && made < 1300; n++) {
    let x: number, z: number;
    const near = r() < 0.5, side = r() < 0.5 ? -1 : 1, off = r() * r();
    if (near && r() < 0.45) { x = side * (9.5 + off * 55); z = -150 + r() * 1100; }
    else if (near) {
      const i = Math.floor(r() * (P.length - 1)), a = P[i], c = P[i + 1], l = a.distanceTo(c);
      x = a.x - side * (c.y - a.y) / l * (3.6 + off * 45); z = a.y + side * (c.x - a.x) / l * (3.6 + off * 45);
    } else { x = -820 + r() * 1300; z = -420 + r() * 1600; }
    const d = Math.min(z > -320 && z < 1150 ? Math.abs(x) : 1e9, trackNearest(x, z).d);
    if ((!near && r() > (d < 45 ? 1 : d < 160 ? 0.3 : 0.1)) || !free(x, z)) continue;
    add(r, x, z, d);
  }
  // the highway on south to the diner (2 km) and past it: beside the way only, a sequence of
  // its own, so the stretch to the station keeps its plants
  const r2 = rng(78);
  for (let n = 0, more = 0; n < 12000 && more < 900; n++) {
    const side = r2() < 0.5 ? -1 : 1, off = r2() * r2();
    const x = side * (9.5 + off * 60), z = 950 + r2() * 1950;
    if (!free(x, z)) continue;
    const was = made;
    add(r2, x, z, Math.abs(x));
    more += made - was;
  }
  const lobe = (rad: number, x: number, y: number, z: number) => new THREE.IcosahedronGeometry(rad, 0).scale(1, 0.6, 1).translate(x, y, z);
  const bushes = instances(mergeGeometries([lobe(0.6, 0, 0.25, 0), lobe(0.42, 0.4, 0.2, 0.18)])!, b.m.bush, bush);
  tints.forEach((c, i) => bushes.setColorAt(i, c));
  b.root.add(bushes);
  b.root.add(instances(new THREE.DodecahedronGeometry(0.5, 0).scale(1, 0.62, 0.85).translate(0, 0.2, 0), b.m.rock, rock));
  const leaf = [0.28, 0.36, 0.24], dry = [0.42, 0.34, 0.24];
  const parts = [colored(new THREE.CylinderGeometry(0.07, 0.09, 0.35, 5).translate(0, 0.17, 0), dry),
    colored(new THREE.CylinderGeometry(0.012, 0.02, 1.7, 4).translate(0, 1.15, 0), dry)];
  for (let i = 0; i < 12; i++) {
    const a = i / 12 * Math.PI * 2, tilt = 0.35 + (i % 3) * 0.3;
    parts.push(colored(new THREE.ConeGeometry(0.035, 0.75, 3).translate(0, 0.37, 0).rotateZ(tilt).rotateY(a).translate(0, 0.32, 0), leaf));
  }
  b.root.add(instances(mergeGeometries(parts)!, b.m.vc, yucca));
}

// ---------- SARO behind the start: its lamps and beacons as they sit on the site ----------
// Positions follow Exterior.ts (site x, z), moved so the site road lies on this highway:
// local = site + (24, 0.6, -300). The array is north of the control room, the motel west.
const DISHES = [[-48, -66], [-16, -52], [9.5, -42], [6, -88], [44, -96], [-30, -116], [-80, -104], [20, -146], [66, -156], [-58, -168], [-10, -200], [100, -120], [-120, -150], [40, -230], [-96, -230],
  [130, -210], [-150, -270], [80, -290], [-40, -300], [170, -260], [-200, -210], [10, -360], [120, -350], [-120, -340], [-230, -300], [210, -330], [-60, -420]];
const LAMPS = [[30, -30], [-10, -42], [-40, -56], [12, -78], [48, -86], [-70, -94], [-24, -106], [60, -146], [-52, -158], [-4, -190], [94, -110]];
export function saroLights(b: Build) {
  const L = (x: number, y: number, z: number, size: number, c: number, blink = 0, ph = 0) => b.glow.add(x + 24, y, z - 300, size, c, blink, ph);
  LAMPS.forEach(([x, z]) => L(x, 6.5, z, 7, 0xffb45a));
  for (let z = 70; z > -200; z -= 34) L(-20.1, 7.1, z, 7, 0xffaf55);
  DISHES.forEach(([x, z], i) => L(x + 3, 16.4, z - 3, 5.5, 0xff3c28, 1.6, hash(i, 3) * 6));
  [[-6.4, 2.6, 2], [20.9, 2.8, 5.8], [30.4, 2.8, 5.8], [-6.4, 2.6, -3]].forEach(([x, y, z]) => L(x, y, z, 4, 0xffbe6e));
  L(-30.4, 5.8, 30.2, 12, 0xff4a35); L(-29, 4, 32, 5, 0x50f0d8);
  for (let z = 26; z <= 66; z += 5) L(-42.6, 2.2, z, 2.6, 0xffc070);
  // far ranches and a town glow to the south east, to fill the night a little
  b.glow.add(1300, 4, 1700, 7, 0xffd0a0); b.glow.add(-1500, 4, 2100, 6, 0xffc890); b.glow.add(900, 4, -1600, 6, 0xd0e4ff);
  // a warm haze over the site, low on the northern horizon
  const haze = new THREE.Mesh(new THREE.PlaneGeometry(1800, 300), new THREE.MeshBasicMaterial({
    map: horizonGlowTex(), color: new THREE.Color(0.42, 0.22, 0.09), transparent: true, blending: THREE.AdditiveBlending, depthWrite: false, fog: false,
  }));
  haze.position.set(24, 110, -1500);
  haze.renderOrder = 1;
  return haze;
}
// The survey track's route as points, for path following (preview and tests) and maps.
export const trackRoute = () => track.pts.filter((_, i) => i % 4 === 0).map((p) => new THREE.Vector2(p.x, p.y));
