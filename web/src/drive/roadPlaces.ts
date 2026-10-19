import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, rod, addFlood } from '../world/kit';
import { rng } from '../core/textures';
import { track, trackHalf, trackNearest, oldNearest, surfaceY, hash, DINER, TOWN, TRK, MOTEL } from './roadTerrain';
import { stationToRoad, MOTEL_AT, MOTEL_GLOWS, SARO_IN_ROAD } from '../world/geo';
import { artTexture } from '../core/art';
import { STATION_FENCE, STATION_GATE, STATION_HUT, STATION_SHED, STATION_POLE } from '../world/stationLayout';
import { horizonGlowTex } from './roadTextures';
import { type Build, type Keep, roadFlood, colored, instances, place, circle, wall, onRanchRoad, ranchZ } from './roadProps';

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

// ---------- rocks, thick where the headlights pass ----------
// The brush, the grass and the small stones are world/flora.ts (groundCover.roadFlora); these are
// the big rocks the truck can hit. (The draws of the brush that stood here are kept, so the rocks
// are where they were.)
export function plants(b: Build, keep: Keep = () => true) {
  const r = rng(77);
  const rock: THREE.Matrix4[] = [];
  const free = (x: number, z: number) => {
    if (Math.abs(x) < 9.5) return false;
    const t = trackNearest(x, z);
    if (t.d < trackHalf(t.s) + 1.8) return false;
    if (x > -672 && x < -598 && z > 500 && z < 566) return false;   // station
    if (x > 395 && x < 485 && z > 150 && z < 235) return false;     // ranch yard
    if (x > DINER.x - 30 && x < -3 && z > DINER.z - 45 && z < DINER.z + 45) return false;   // the diner
    if (x > TOWN.west - 8 && x < TOWN.east + 8 && z > DINER.z && z < TOWN.z1 + 20) return false;   // Caliche (world/Town.ts)
    if (x > MX - 30 && x < -3 && z > MZ - 4 && z < MZ + 76) return false;                  // the motel
    if (oldNearest(x, z).d < 7) return false;                                                 // the old road
    return !(x > -26 && x < -14 && z > 510 && z < 530);             // cattle guard
  };
  let made = 0;
  const add = (rr: () => number, x: number, z: number, d: number) => {
    // the same random numbers are drawn whether or not the plant is kept, so a stretch
    // drawn in another area has the very same plants
    // (none on the ranch's road, which came after them: dropped, not moved, so the rest stay put)
    const k = rr(), y = surfaceY(x, z), here = keep(x, z) && !onRanchRoad(x, z);
    if (k < 0.72) {
      for (let i = 0; i < 6; i++) rr();
      made++;
    } else if (k < 0.96) {
      const sc = rr() < 0.85 ? 0.15 + rr() * 0.45 : 0.6 + rr() * 0.8;
      const m = place(x, y - 0.12 * sc, z, rr() * 6, sc, (rr() - 0.5) * 0.5);
      if (here) { rock.push(m); if (sc > 0.7 && d < 40) circle(b, x, z, 0.4 * sc); }
    } else { rr(); rr(); }
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
  b.root.add(instances(new THREE.DodecahedronGeometry(0.5, 0).scale(1, 0.62, 0.85).translate(0, 0.2, 0), b.m.rock, rock));
}

// ---------- dry grass: tufts on the highway's shoulders and along the tracks ----------
/** Bunch grass, dry since the autumn, where the graders and the wheels leave it: on the gravel
 *  shoulders and out to the fence line along the highway, on the crown between the ruts of the
 *  survey track and the ranch's road and along their edges, lit warm by the lamps and the
 *  headlights. A few blades in a fan to a tuft, in the desert's own material and its uv under
 *  them, so they are merged into the ground and cost no draw call (RoadArea.ts, corridors.ts);
 *  straw coloured, darker at the foot. No collider: the truck goes through it. Every third tuft
 *  comes first: `rest` is how many indices follow those, which Low leaves off (userData.lowRange,
 *  core/quality.ts). The same random numbers whether or not a tuft is kept (keep), so a stretch
 *  drawn in another area has the very same grass. */
export function tufts(keep: Keep = () => true) {
  const r = rng(79), pos: number[] = [], nor: number[] = [], uv: number[] = [], col: number[] = [];
  const few: number[] = [], rest: number[] = [];
  let n = 0;
  const tuft = (x: number, z: number, s: number, lift = 0) => {
    const blades = 5 + Math.floor(r() * 4), a0 = r() * 7, tone = 0.8 + r() * 0.45, dry = r();
    const b = Array.from({ length: blades }, () => [r(), r(), r()]);
    if (!keep(x, z)) return;
    const y = surfaceY(x, z) + lift - 0.015, list = n++ % 3 ? rest : few;
    // straw, some of it greyer with age; the foot in the shade of the rest
    const tip = [1.32 * tone, (1.16 - dry * 0.1) * tone, (0.68 + dry * 0.14) * tone], foot = tip.map((c) => c * 0.45);
    b.forEach(([u, v, w], i) => {
      const a = a0 + i / blades * Math.PI * 2 + (u - 0.5) * 0.8, lean = (0.08 + v * 0.18) * s, h = (0.24 + w * 0.3) * s, bw = 0.055 * s;
      const ca = Math.cos(a), sa = Math.sin(a), k = pos.length / 3;
      pos.push(x - sa * bw, y, z + ca * bw, x + sa * bw, y, z - ca * bw, x + ca * lean, y + h, z + sa * lean);
      for (let j = 0; j < 3; j++) { nor.push(0, 1, 0); uv.push(x / 5, z / 5); col.push(...(j < 2 ? foot : tip)); }
      list.push(k, k + 1, k + 2, k + 1, k, k + 2);   // both faces
    });
  };
  // the shoulders and the verge out to about 9.5 m, both sides, sparse; not where the motel, the
  // tracks, the diner, Caliche and the old road come off it
  const shoulder = (x: number, z: number) => {
    if (z > DINER.z - 45 && z < TOWN.z1 + 20) return false;
    if (x < 0 && ((z > Math.min(MZ - 6, MOTEL.z - 48) && z < Math.max(MZ + 78, MOTEL.z + 48)) || (z > 500 && z < 542))) return false;
    if (x > 0 && Math.abs(z - ranchZ(x)) < 4.5) return false;
    return oldNearest(x, z).d > 7;
  };
  for (const sd of [-1, 1]) for (let z = -260; z < 2700; z += 2 + r() * 5) {
    const x = sd * (6.15 + r() * r() * 3.3), s = 0.8 + r() * 0.7, twin = r() < 0.3;
    if (!shoulder(x, z)) continue;
    tuft(x, z, s);
    if (twin) tuft(x + sd * 0.35, z + 0.4, s * 0.7);
  }
  // the survey track: on the crown between the ruts (not at its mouth, where the trucks turn
  // over it) and along its edges
  const P = track.pts, S = track.s;
  for (let i = 0; i < P.length - 1; i++) {
    const a = P[i], c = P[i + 1], l = a.distanceTo(c), tx = (c.x - a.x) / l, tz = (c.y - a.y) / l, e = trackHalf(S[i]) + TRK.soft;
    for (let t = 0; t < l; t += 1.2 + r() * 2.6) {
      const x0 = a.x + tx * t, z0 = a.y + tz * t, o = (r() - 0.5) * 0.36, side = r() < 0.5 ? -1 : 1, oe = side * (e - 0.25 + r() * r() * 1.6), edge = r() < 0.6;
      if (S[i] > 24) tuft(x0 - tz * o, z0 + tx * o, 0.6 + r() * 0.4);
      if (edge) tuft(x0 - tz * oe, z0 + tx * oe, 0.8 + r() * 0.7);
    }
  }
  // the ranch's two-track: its crown and its edges (on its picture, 3.5 cm over the land)
  for (let x = 12; x < 390; x += 1.4 + r() * 3) {
    const o = (r() - 0.5) * 0.34, side = r() < 0.5 ? -1 : 1, oe = side * (2.35 + r() * r() * 1.6), edge = r() < 0.6;
    if (Math.abs(x - 21) < 1.2) continue;   // the gate
    tuft(x, ranchZ(x) + o, 0.6 + r() * 0.4, 0.035);
    if (edge) tuft(x, ranchZ(x) + oe, 0.8 + r() * 0.7, Math.abs(oe) < 2.5 ? 0.035 : 0);
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(nor, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
  g.setIndex([...few, ...rest]);
  return { geo: g, rest: rest.length, count: n };
}
/** The ground (parts) with the tufts merged in at its end: Low told to leave most of them off, and
 *  a ray (the tools' and the game's checks of what is in the way) stopped by the land only, never
 *  by a blade of grass (a ray keeps to the draw range, three.js Mesh). */
export function groundWithTufts(parts: THREE.BufferGeometry[], mat: THREE.Material, name: string, keep: Keep = () => true) {
  const t = tufts(keep), geo = mergeGeometries(t.count ? [...parts, t.geo] : parts)!;
  const all = geo.index!.count, land = all - (t.count ? t.geo.index!.count : 0);
  const mesh = new THREE.Mesh(geo, mat);
  mesh.name = name; mesh.userData.lowRange = all - (t.count ? t.rest : 0);
  mesh.raycast = (rc, out) => {
    const { start, count } = geo.drawRange;
    geo.setDrawRange(0, land); THREE.Mesh.prototype.raycast.call(mesh, rc, out); geo.setDrawRange(start, count);
  };
  return mesh;
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
  // Sierra Motor Court, 650 m down the highway now (geo.ts)
  for (const [x, y, z, size, c] of MOTEL_GLOWS) b.glow.add(x + MX, y + MOTEL_AT.y, z + MZ, size, c);
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
// ---------- Sierra Motor Court from the road ----------
// Its real buildings are SARO's (world/MotelFront.ts, World.ts): the truck is in SARO's area
// whenever it is near them (drive/legs.ts hands it over 60 m south of the motel). From the road
// area they are only seen from farther off, coming north: their bulk, the canopy, the sign,
// and the lamps (saroLights). Built in the numbers the court was made in, moved as it is.
/** Where the motel's own numbers start, road-local (its x and z offsets; y is MOTEL_AT.y). */
const MX = MOTEL_AT.x + SARO_IN_ROAD.x, MZ = MOTEL_AT.z + SARO_IN_ROAD.z;
export function motelFar(b: Build) {
  const g = new THREE.Group(), m = b.m;
  g.position.set(MX, MOTEL_AT.y, MZ);
  box(g, 4.2, 2.8, 48, m.concrete, -47.9, 1.4, 46);
  box(g, 3.5, 2.8, 42.1, m.concrete, -43.95, 1.4, 48.95);
  box(g, 4.8, 2.8, 5.8, m.concrete, -43.4, 1.4, 24.9);
  box(g, 9.15, 0.22, 48.2, m.concrete, -45.5, 2.9, 46);
  box(g, 3.15, 0.18, 49, m.concrete, -39.75, 2.84, 46.1);
  box(g, 9.4, 0.7, 48.4, m.concrete, -45.5, -0.36, 46);
  for (const z of [22, 33, 43, 53, 63, 69]) box(g, 0.12, 2.74, 0.12, m.steel, -38.8, 1.37, z);
  for (const x of [-31.5, -29.3]) cyl(g, 0.22, 0.22, 9, m.steel, x, 3.9, 30, 8);
  for (const z of [18, 47]) cyl(g, 0.1, 0.14, 6.6, m.steel, -28.4, 3.3, z, 7);
  b.statics.add(g);
  // the sign, as at SARO (Exterior.ts): one picture both ways round
  for (const back of [false, true]) {
    const map = back ? artTexture('sierra').clone() : artTexture('sierra');
    if (back) { map.wrapS = THREE.RepeatWrapping; map.repeat.x = -1; map.offset.x = 1; map.needsUpdate = true; }
    const face = new THREE.Mesh(new THREE.PlaneGeometry(5.2, 7.8), new THREE.MeshBasicMaterial({ map, transparent: true, alphaTest: 0.02, depthWrite: false, side: back ? THREE.BackSide : THREE.FrontSide, toneMapped: false }));
    face.position.set(-30.4 + MX, 5.8 + MOTEL_AT.y, 30.2 + MZ); face.rotation.y = Math.PI / 2 - 0.5;
    face.userData.noMerge = true;
    b.root.add(face);
  }
}

// The survey track's route as points, for path following (preview and tests) and maps.
export const trackRoute = () => track.pts.filter((_, i) => i % 4 === 0).map((p) => new THREE.Vector2(p.x, p.y));
