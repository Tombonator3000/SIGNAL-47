import * as THREE from 'three';
import { Clutter, v3 } from './clutter';

// What has gathered in the rooms over the years (world/clutter.ts makes it): papers and mugs
// on the desks, an ashtray, cables along the walls and under the desks, notices and posters, a
// coat on a hook, a drawer left open, the worn floor by the doors and the dirt in the corners.
// One function per room, in the room's own terms (local metres where its group is moved), called
// from the room's constructor. Nothing here can be used, and nothing stands where the chapters
// walk or aim: what stands on a floor is inside a collider the room already has, or gets a small
// one against a wall or a piece of furniture, so it never closes a way or makes a pocket.
// fine: left out on Low (the small things that only add to a room that is already there).

export type Dress = {
  add: (o: THREE.Object3D) => void;
  mat: (p: THREE.MeshStandardMaterialParameters) => THREE.MeshStandardMaterial;
  col: (minX: number, maxX: number, minZ: number, maxZ: number) => void;
};
const C = {
  paper: 0xe8e0c8, manila: 0xc9ad78, card: 0xb89a6a, cable: 0x1a1b1c, grey: 0x8a8c88, steel: 0x8a8f92, dark: 0x2a2c2e,
  navy: 0x2f4a6e, maroon: 0x6e2f2a, olive: 0x3f5b3a, blind: 0xd8d4c8, white: 0xece6da,
  // the dirt's colours
  grime: 0x2e241a, heel: 0x141210, coffee: 0x3a2412, water: 0x6a5232, dust: 0x6a5a48, sand: 0xc8b088, web: 0xd8d4c8,
};
const PI = Math.PI;
function done(c: Clutter, d: Dress) { for (const m of c.build()) d.add(m); }
// pencils standing in a cup
function pencilCup(c: Clutter, x: number, y: number, z: number, color = 0x2a3a4a) {
  c.cyl(0.036, 0.032, 0.1, x, y + 0.05, z, color, { seg: 10 });
  c.cyl(0.033, 0.033, 0.003, x, y + 0.1, z, 0x101112, { seg: 10 });
  [[0.012, 0.004, 0xd9a520], [-0.01, 0.012, 0xa82a22], [0.002, -0.014, 0xd9a520]].forEach(([dx, dz, col]) =>
    c.rod(v3(x + dx, y + 0.02, z + dz), v3(x + dx * 2.2, y + 0.175, z + dz * 2.2), 0.0035, col, { fine: true, seg: 6 }));
}
// a cobweb on the ceiling in a corner: sx, sz say where the corner is from (x, z)
function web(c: Clutter, x: number, y: number, z: number, sx: number, sz: number, size = 0.42) {
  const ry = sx > 0 ? (sz < 0 ? PI : PI / 2) : (sz < 0 ? -PI / 2 : 0);
  c.dirt(size, size, x, y, z, 'web', C.web, { rx: PI / 2, ry });
}
// dirt along the foot of a wall: (x, z) on the wall's face, ry faces the room
function skirt(c: Clutter, x: number, z: number, ry: number, w: number, color = C.dust) {
  c.dirt(w, 0.07, x, 0.035, z, 'dust', color, { ry });
}
const rings = (c: Clutter, x: number, y: number, z: number, ry = 0) => c.floorDirt(0.15, 0.075, x, z, 'rings', C.coffee, ry, y + 0.0015);

// ---------- the control room ----------
export function dressControlRoom(d: Dress) {
  const c = new Clutter('control', d.mat), T = 0.78;   // the desks' tops
  // the console desk under the windows: Dale's mug, the ashtray, a calculator, a spare reel
  c.mug(-1.55, T, -3.68, 0xc9d2d6, { ry: 2.2 });
  rings(c, -1.4, T, -3.58, 0.3); rings(c, 1.75, T, -3.56, -0.4);
  c.ashtray(-3.15, T, -3.72, 4);
  c.calculator(1.55, T, -3.66, 0.25);
  c.pencil(-0.55, T, -3.62, 0.4); c.pencil(0.82, T, -3.7, -1.1); c.pencil(1.3, T, -3.84, 0.2, 0xa82a22);
  c.reel(3.18, T, -3.66, 0.12);
  // somebody's cardigan left over the back of the middle chair
  c.slab(0.5, 0.52, 0.2, 0.12, 0.1, 0.07, 0.98, -2.68, 0x5a4a3a, { ry: 0.08, rx: -0.12 });
  c.slab(0.11, 0.1, 0.42, 0.08, 0.07, 0.31, 0.74, -2.74, 0x4e4032, { ry: 0.08, rz: 0.08 });
  c.slab(0.44, 0.46, 0.3, 0.05, 0.05, 0.08, 0.74, -2.62, 0x54463a, { ry: 0.08, rx: -0.14 });
  c.binder(-3.1, T, -4.12, C.navy, { flat: true, ry: 0.05 }); c.binder(-3.12, T + 0.06, -4.1, C.maroon, { flat: true, ry: -0.1 });
  // a printout run off the front of the desk and onto the floor
  c.sheet(0.25, 0.32, -0.62, T + 0.002, -3.62, 'greenbar', 0);
  c.plane(0.25, 0.42, -0.62, T - 0.21, -3.442, 'greenbar', { rx: -0.06, fine: true });
  c.papers(-0.6, 0, -3.24, 0.25, 0.3, 'greenbar', { ry: 0.3, n: 3, fine: true });
  // under it: each screen's cable down to a power strip in its knee space
  for (const x of [-2.15, 0, 2.15]) {
    c.box(0.3, 0.04, 0.06, x + 0.5, 0.02, -4.12, 0xd8d0b8, { fine: true });
    c.cable([v3(x + 0.1, 0.72, -4.27), v3(x + 0.15, 0.3, -4.29), v3(x + 0.22, 0.012, -4.17), v3(x + 0.36, 0.012, -4.1)], 0.007, C.cable);
  }
  // the bundle from the end of the desk along the wall to RX bank 3
  c.cable([v3(3.3, 0.6, -4.3), v3(3.42, 0.15, -4.33), v3(3.7, 0.012, -4.33), v3(4.9, 0.012, -4.32), v3(5.3, 0.012, -3.7), v3(5.32, 0.012, -2.95), v3(5.32, 0.25, -2.79)], 0.011, C.cable);
  // a cable taped across the floor from the west desk to the console
  c.cable([v3(-5.16, 0.32, 1.98), v3(-5.1, 0.008, 1.88), v3(-4.55, 0.008, 0.9), v3(-4.35, 0.008, -1.6), v3(-3.75, 0.008, -3.25), v3(-3.47, 0.06, -3.55), v3(-3.46, 0.5, -3.7)], 0.008, 0x55585a, { fine: false });
  for (const [x, z, ry] of [[-4.68, 1.06, 0.5], [-4.43, -0.4, 0.08], [-4.0, -2.45, 0.3]]) c.box(0.08, 0.002, 0.05, x, 0.01, z, 0x7a7c78, { ry, fine: true });
  // the blinds drawn up in their heads, and one let half down in the east bay, a little crooked
  for (let k = 0; k < 6; k++) {
    c.box(1.68, 0.07, 0.05, -4.5 + k * 1.8, 2.66, -4.42, C.blind);
    c.box(1.68, 0.015, 0.045, -4.5 + k * 1.8, 2.618, -4.42, 0xb8b4a8, { fine: true });
  }
  for (let i = 0; i < 15; i++) c.box(1.66, 0.004, 0.045, 4.5, 2.58 - i * 0.05, -4.42, C.blind, { rx: 0.35, rz: i * 0.0016, fine: true });
  c.box(1.68, 0.02, 0.05, 4.5, 1.84, -4.42, 0xb8b4a8, { rz: 0.024 });
  for (const dx of [-0.6, 0.6]) c.rod(v3(4.5 + dx, 2.63, -4.396), v3(4.5 + dx, 1.85 + dx * 0.02, -4.396), 0.0016, 0xc8c4b8, { fine: true });
  // on the sills: a cactus in the west bay, binoculars in the east one
  c.cyl(0.05, 0.04, 0.08, -4.6, 1.02, -4.32, 0x9a5a36, { seg: 10 });
  c.cyl(0.026, 0.03, 0.14, -4.6, 1.13, -4.32, 0x4a6a3a, { seg: 8 });
  c.cyl(0.014, 0.016, 0.06, -4.565, 1.15, -4.32, 0x4a6a3a, { rz: -0.9, seg: 6, fine: true });
  for (const dx of [-0.035, 0.035]) c.cyl(0.028, 0.03, 0.11, 4.6 + dx, 1.008, -4.3, 0x1e1f20, { rx: PI / 2, seg: 10 });
  c.box(0.05, 0.02, 0.05, 4.6, 1.02, -4.3, 0x3a3c3e);
  // the conduit from the top of the rack along the walls and down to the console's end
  c.rod(v3(5.93, 1.96, -2.25), v3(5.93, 2.88, -2.25), 0.018, C.grey);
  c.rod(v3(5.93, 2.88, -2.25), v3(5.93, 2.88, -4.44), 0.018, C.grey);
  c.rod(v3(5.93, 2.88, -4.44), v3(3.6, 2.88, -4.44), 0.018, C.grey);
  c.rod(v3(3.6, 2.88, -4.44), v3(3.6, 0.99, -4.41), 0.016, C.grey);
  c.box(0.12, 0.12, 0.08, 5.93, 2.88, -4.42, C.grey);
  for (const z of [-3.0, -3.8]) c.box(0.05, 0.05, 0.04, 5.95, 2.88, z, C.grey, { fine: true });
  // the coffee corner: the can, the sugar, paper cups, the fund
  const K = 0.92;
  c.cyl(0.065, 0.065, 0.15, 4.6, K + 0.075, 3.68, 0xffffff, { side: 'can', seg: 14 });
  c.cyl(0.066, 0.066, 0.01, 4.6, K + 0.155, 3.68, C.steel, { seg: 14 });
  c.cyl(0.045, 0.045, 0.1, 4.62, K + 0.05, 4.02, 0xd8e0e0, { seg: 10 });
  c.cyl(0.047, 0.047, 0.02, 4.62, K + 0.11, 4.02, 0xb8261e, { seg: 10 });
  c.cyl(0.042, 0.032, 0.16, 4.14, K + 0.08, 3.98, 0xf2f0ea, { seg: 10 });
  c.floorDirt(0.2, 0.12, 4.5, 3.74, 'splash', C.coffee, 0.3, K + 0.0015);
  c.notice(4.38, 1.42, 4.498, PI, 0.14, 0.095, 'fund', { tilt: 0.06 });
  c.dirt(0.12, 0.3, 4.32, 0.7, 3.586, 'drip', C.coffee, { ry: PI });
  c.floorDirt(0.5, 0.4, 4.3, 3.3, 'splash', 0x2a1a0e, 0.4);
  // the supervisor desk: pencils, a stapler, the rings under the mug, a bin under it
  pencilCup(c, 1.86, T, 2.22);
  c.box(0.15, 0.04, 0.045, 1.94, T + 0.02, 2.86, C.dark, { ry: 0.3 });
  rings(c, 2.46, T, 2.24, 0.5);
  c.bin(1.98, 2.36);
  c.plate(3.62, 0.3, 4.48, PI, 'outlet');
  c.cable([v3(3.36, T + 0.01, 2.95), v3(3.42, 0.5, 3.0), v3(3.46, 0.012, 3.18), v3(3.58, 0.012, 4.2), v3(3.62, 0.012, 4.43), v3(3.62, 0.27, 4.475)], 0.006, 0x1e1f20);
  // the filing cabinets: the top drawer nearest the desk left open, a box of logs on top
  const dy = 1.13;
  c.box(0.33, 0.012, 0.46, -5.27, dy - 0.12, 0, 0x4e5654);
  for (const s of [-1, 1]) c.box(0.33, 0.2, 0.012, -5.27, dy - 0.03, s * 0.225, 0x4e5654);
  c.box(0.02, 0.29, 0.48, -5.095, dy, 0, 0x6e7a76);
  c.box(0.03, 0.025, 0.1, -5.075, dy + 0.06, 0, C.steel);
  c.box(0.3, 0.17, 0.42, -5.27, dy - 0.03, 0, C.manila, { faces: [null, null, 'folders'] });
  c.box(0.03, 0.26, 0.44, -5.41, dy, 0, 0x0c0d0c);   // the dark slot it came out of (over the old handle)
  d.col(-5.43, -5.08, -0.26, 0.26);
  c.box(0.34, 0.24, 0.3, -5.72, 1.44, -0.62, C.card);
  c.plane(0.15, 0.075, -5.546, 1.45, -0.62, 'boxlabel', { ry: PI / 2 });
  c.papers(-5.72, 1.56, -0.6, 0.21, 0.28, 'memoCoffee', { ry: 0.3, n: 2 });
  // the fan on the cabinets gets its grille and motor
  { const a = PI / 2 + 0.4, nx = Math.sin(a), nz = Math.cos(a);
    c.disc(0.125, -5.65, 1.62, 0, 'fan', { ry: a });
    c.cyl(0.045, 0.045, 0.09, -5.65 - nx * 0.05, 1.62, -nz * 0.05, 0x3a3c3e, { rx: PI / 2, ry: a, seg: 10 }); }
  // the west desk: an ashtray of somebody's evening, a green mug
  c.ashtray(-5.34, T, 2.88, 5);
  c.mug(-5.3, T, 1.78, 0x3e6a4a, { ry: 1.0, full: true });
  rings(c, -5.36, T, 1.97, 1.2);
  // the bulletin board: the roster and a postcard over the old papers
  c.notice(1.66, 1.75, 4.465, PI, 0.2, 0.25, 'roster', { pins: true, tilt: -0.04 });
  c.notice(0.74, 1.4, 4.461, PI, 0.15, 0.1, 'postcard', { pins: true, tilt: 0.1 });
  // by the corridor door: the switch, the numbers; by the service door: the switch
  c.plate(-1.72, 1.25, 4.498, PI); c.dirt(0.16, 0.2, -1.72, 1.25, 4.495, 'smudge', C.grime, { ry: PI });
  c.notice(-1.25, 1.6, 4.498, PI, 0.2, 0.25, 'numbers');
  c.plate(5.998, 1.25, 0.95, -PI / 2); c.dirt(0.16, 0.2, 5.995, 1.25, 0.95, 'smudge', C.grime, { ry: -PI / 2 });
  // an extinguisher on its bracket by the corridor door, boxes of paper under the tables
  c.cyl(0.08, 0.08, 0.44, -3.2, 0.52, 4.39, 0xa3241b, { seg: 12 });
  c.cyl(0.03, 0.05, 0.07, -3.2, 0.775, 4.39, 0x1e1f20, { seg: 8 });
  c.rod(v3(-3.17, 0.78, 4.42), v3(-3.12, 0.4, 4.36), 0.008, 0x1e1f20, { fine: true });
  c.box(0.06, 0.08, 0.1, -3.2, 0.6, 4.47, C.steel);
  c.notice(-3.2, 1.12, 4.498, PI, 0.14, 0.07, 'fireExt', { tape: false });
  c.box(0.38, 0.26, 0.3, 5.62, 0.13, 0.0, C.card, { ry: 0.1 });
  c.box(0.36, 0.28, 0.32, -5.62, 0.14, 2.85, C.card, { ry: -0.08 });
  // telex paper on the floor in front of the teleprinter
  c.sheet(0.24, 0.34, -4.2, 0.003, 3.74, 'white', 0.2, { color: 0xece4c8, fine: true });
  c.cyl(0.03, 0.03, 0.22, -4.15, 0.03, 3.62, 0xece4c8, { rz: PI / 2, ry: 0.4, seg: 8, fine: true });
  // the ceiling tiles where the roof leaked
  c.dirt(0.6, 0.5, -1.2, 2.996, 0.6, 'stain', C.water, { rx: PI / 2 });
  c.dirt(0.4, 0.35, 4.6, 2.996, -3.7, 'stain', C.water, { rx: PI / 2, ry: 0.7 });
  // the floor where the night shifts walk: the doors, the chairs, the rack, the coffee
  c.floorDirt(1.3, 1.0, -2.4, 3.95, 'wear', 0x140e08, 0);
  c.floorDirt(1.2, 1.4, 5.4, 1.6, 'wear', 0x140e08, PI / 2);
  for (const x of [-2.15, 0, 2.15]) c.floorDirt(1.0, 0.8, x, -3.0, 'wear', 0xc8b8a0, x * 0.4);   // the casters wear the polish off
  c.floorDirt(1.6, 0.7, 4.85, -1.3, 'wear', C.grime, PI / 2);
  c.floorDirt(0.9, 0.7, 3.7, 3.4, 'wear', C.grime, 0.4);
  c.floorDirt(0.9, 0.7, 2.45, 3.4, 'wear', 0xc8b8a0, 0.2);
  for (const [x, z, ry] of [[1.2, 3.75, 0.3], [-1.6, 3.6, -0.5], [3.9, 0.9, 1.2], [-0.8, -2.4, 0.1]]) c.floorDirt(0.6, 0.15, x, z, 'scuff', C.heel, ry);
  skirt(c, -0.6, 4.478, PI, 2.6); skirt(c, 5.978, 3.3, -PI / 2, 2.2); skirt(c, -5.978, 0.95, PI / 2, 1.2);
  c.dirt(0.7, 0.18, -1.6, 0.42, 4.477, 'scuff', 0x1a1612, { ry: PI });
  c.dirt(0.6, 0.16, 5.977, 0.36, 2.75, 'scuff', 0x1a1612, { ry: -PI / 2 });
  done(c, d);
}

// ---------- the south corridor and the records room ----------
export function dressAnnex(d: Dress) {
  const c = new Clutter('annex', d.mat), g = 0x3a3024;
  // the corridor: worn down the middle, more at the doors
  c.floorDirt(7.4, 0.8, -2.1, 5.72, 'wear', g, 0);
  c.floorDirt(1.0, 0.7, -2.4, 5.2, 'wear', g, 0); c.floorDirt(0.9, 0.6, -3.9, 6.3, 'wear', g, 0);
  c.floorDirt(0.8, 0.8, -5.6, 5.7, 'wear', g, PI / 2); c.floorDirt(0.8, 0.5, 0.45, 6.32, 'wear', g, 0);
  for (const [x, z, ry] of [[-1.0, 5.55, 0.2], [0.7, 5.95, -0.3], [-4.6, 5.45, 0.5], [-3.2, 6.1, 2.8], [1.3, 5.6, 1.4]]) c.floorDirt(0.6, 0.15, x, z, 'scuff', C.heel, ry);
  skirt(c, -2.0, 4.828, 0, 7.6); skirt(c, -2.0, 6.573, PI, 7.6);
  // a coat and a hard hat on hooks along the north wall
  c.coat(0.8, 1.75, 4.805, 0, 0x5a4a36);
  c.box(0.05, 0.06, 0.01, 1.3, 1.77, 4.81, 0x6c7074);
  c.rod(v3(1.3, 1.75, 4.815), v3(1.3, 1.72, 4.87), 0.006, 0x6c7074);
  c.dome(0.12, 1.3, 1.6, 4.86, 0xe8e2cc, { rx: PI / 2 - 0.25 });          // a hard hat hung by its brim, crown out
  c.cyl(0.155, 0.155, 0.012, 1.3, 1.6, 4.86, 0xd8d2bc, { rx: PI / 2 - 0.25, seg: 14 });
  c.box(0.12, 0.012, 0.05, 1.3, 1.75, 4.82, 0xd8d2bc, { rx: -0.25, fine: true });
  // posters, the fire card and the alarm, the switch at the control room door, a clock
  c.notice(-4.4, 1.55, 4.805, 0, 0.4, 0.55, 'quiet', { tilt: 0.01 });
  c.notice(-2.3, 1.45, 6.598, PI, 0.36, 0.5, 'safety', { tilt: -0.015 });
  c.notice(-5.25, 1.45, 4.805, 0, 0.14, 0.19, 'fire');
  c.box(0.1, 0.14, 0.05, -5.62, 1.35, 4.83, 0xb8261e); c.box(0.06, 0.03, 0.01, -5.62, 1.37, 4.858, 0xf0e8d8);
  c.plate(-3.08, 1.25, 4.805, 0); c.dirt(0.16, 0.2, -3.08, 1.25, 4.808, 'smudge', C.grime, {});
  c.clock(-1.15, 2.3, 6.598, PI, 0.14);
  // the mop bucket in the corner by the restrooms
  const bx = 1.76, bz = 5.02;
  c.cyl(0.165, 0.145, 0.3, bx, 0.15, bz, 0xc8a030, { seg: 12 });
  c.cyl(0.15, 0.15, 0.004, bx, 0.298, bz, 0x5a5a4a, { seg: 12 });
  c.box(0.24, 0.1, 0.12, bx - 0.02, 0.36, bz + 0.06, 0x8a7a2a);
  c.cyl(0.07, 0.09, 0.12, bx + 0.02, 0.3, bz - 0.03, 0xc8c0a8, { seg: 8 });
  c.rod(v3(bx + 0.02, 0.3, bz - 0.03), v3(1.96, 1.42, 4.85), 0.013, 0x9a7a52);
  for (const [dx, dz] of [[-0.11, -0.11], [0.11, -0.11], [-0.11, 0.11], [0.11, 0.11]]) c.cyl(0.025, 0.025, 0.02, bx + dx, 0.02, bz + dz, 0x1e1f20, { rz: PI / 2, seg: 6, fine: true });
  d.col(1.56, 2.0, 4.8, 5.21);
  // a conduit along the top of the north wall, and the ceiling where it leaked
  c.rod(v3(-5.95, 2.66, 4.84), v3(1.95, 2.66, 4.84), 0.016, C.grey);
  for (let x = -5.0; x < 2; x += 1.6) c.box(0.04, 0.05, 0.05, x, 2.66, 4.83, C.grey, { fine: true });
  c.box(0.12, 0.12, 0.06, -0.6, 2.66, 4.84, C.grey);
  c.dirt(0.45, 0.38, 1.15, 2.795, 6.0, 'stain', C.water, { rx: PI / 2 });
  // a ceiling tile pushed up for the wiring and left hanging off its edge, the cables showing
  c.box(0.7, 0.004, 0.6, -5.0, 2.797, 5.7, 0x0a0b0c);
  c.box(0.7, 0.014, 0.6, -5.0, 2.79 - 0.3 * Math.sin(0.6), 5.4 + 0.3 * Math.cos(0.6), 0xb9b3a4, { rx: 0.6 });
  c.cable([v3(-5.12, 2.83, 5.82), v3(-5.16, 2.62, 5.86), v3(-5.06, 2.47, 5.92)], 0.008, C.cable);
  c.cable([v3(-4.86, 2.83, 5.78), v3(-4.84, 2.66, 5.74), v3(-4.9, 2.56, 5.8)], 0.006, 0x8a7a3a);

  // the records room: a drawer of the middle cabinet left open, folders in it
  const fx = -4.46, fy = 0.52;
  c.box(0.44, 0.27, 0.02, fx, fy, 9.52, 0x55705e);
  c.box(0.12, 0.025, 0.03, fx, fy + 0.08, 9.5, C.steel);
  for (const s of [-1, 1]) c.box(0.012, 0.2, 0.34, fx + s * 0.205, fy - 0.03, 9.69, 0x3d5446);
  c.box(0.42, 0.012, 0.34, fx, fy - 0.12, 9.69, 0x3d5446);
  c.box(0.38, 0.17, 0.3, fx, fy - 0.03, 9.7, C.manila, { faces: [null, null, 'folders'] });
  c.box(0.22, 0.3, 0.004, fx + 0.08, fy + 0.16, 9.72, C.manila, { rx: 0.25 });
  c.box(0.42, 0.25, 0.04, fx, fy, 9.84, 0x0c0d0c);   // the dark slot it came out of
  d.col(-4.7, -4.22, 9.48, 9.86);
  // boxes, a binder and a fan on the cabinets
  c.box(0.36, 0.25, 0.32, -5.0, 1.445, 10.2, C.card);
  c.plane(0.16, 0.08, -5.0, 1.45, 10.038, 'boxlabel', { ry: PI });
  c.box(0.36, 0.25, 0.32, -4.48, 1.445, 10.22, 0xa88a5a, { ry: -0.06 });
  c.binder(-3.95, 1.32, 10.15, C.olive, { flat: true, ry: 0.1 });
  c.fan(-3.95, 1.38, 10.15, PI + 0.2);
  // a corkboard by the door, with notices pinned on it
  c.box(1.0, 0.7, 0.025, -1.75, 1.55, 6.8125, 0x8a6a45);
  for (const [w, h, x, y] of [[1.04, 0.025, -1.75, 1.912], [1.04, 0.025, -1.75, 1.188], [0.025, 0.7, -2.262, 1.55], [0.025, 0.7, -1.238, 1.55]]) c.box(w, h, 0.03, x, y, 6.815, 0x5a3a24, { fine: true });
  c.notice(-2.02, 1.62, 6.825, 0, 0.2, 0.25, 'memoWard', { pins: true, tilt: -0.03 });
  c.notice(-1.68, 1.5, 6.825, 0, 0.18, 0.22, 'news', { pins: true, tilt: 0.07 });
  c.notice(-1.4, 1.68, 6.825, 0, 0.15, 0.19, 'memoTapes', { pins: true, tilt: -0.05 });
  c.plate(-4.62, 1.25, 6.8, 0); c.dirt(0.16, 0.2, -4.62, 1.25, 6.803, 'smudge', C.grime, {});
  // the work table: pencils, a ledger, a mug and its rings, a glass, a bin under it
  const W = 0.77;
  pencilCup(c, -4.35, W, 8.32, 0x3d5446);
  c.box(0.24, 0.035, 0.32, -3.15, W + 0.0175, 8.85, 0x2e4a38, { ry: 0.15 });
  c.mug(-2.88, W, 8.62, 0xe8e2d4, { ry: 2.6 });
  rings(c, -2.98, W, 8.78, 0.4);
  c.cyl(0.04, 0.04, 0.008, -3.6, W + 0.004, 8.95, 0x9ab0b8, { seg: 14 });
  c.rod(v3(-3.56, W + 0.005, 8.98), v3(-3.46, W + 0.005, 9.02), 0.008, 0x1e1f20, { fine: true });
  c.bin(-4.25, 8.88);
  c.plate(-1.002, 0.3, 8.7, -PI / 2, 'outlet');
  c.cable([v3(-3.07, W + 0.01, 8.32), v3(-2.8, W + 0.005, 8.22), v3(-2.77, 0.4, 8.2), v3(-2.7, 0.012, 8.3), v3(-1.9, 0.012, 8.65), v3(-1.06, 0.012, 8.7), v3(-1.01, 0.27, 8.7)], 0.006, 0x1e1f20);
  // the floor, a dropped index card, the dust and a web in the corner, the leak
  c.floorDirt(1.0, 0.9, -3.9, 7.2, 'wear', g, 0);
  c.floorDirt(1.4, 0.8, -3.0, 7.8, 'wear', g, 0.6);
  c.floorDirt(0.8, 0.7, -1.7, 9.2, 'wear', g, 0);
  c.floorDirt(0.9, 0.6, -5.15, 8.6, 'wear', g, PI / 2);
  c.sheet(0.125, 0.075, -2.15, 0.003, 7.95, 'white', 0.7, { color: 0xe8e0c8 });
  skirt(c, -2.4, 10.478, PI, 2.0); skirt(c, -1.002, 9.6, -PI / 2, 1.6);
  web(c, -5.8, 2.795, 10.3, -1, 1);
  c.dirt(0.55, 0.45, -5.2, 2.796, 9.95, 'stain', C.water, { rx: PI / 2, ry: 0.4 });
  c.dirt(0.2, 0.5, -5.62, 2.35, 10.496, 'drip', C.water, { ry: PI });
  done(c, d);
}

// ---------- the photo lab ----------
export function dressLab(d: Dress) {
  const c = new Clutter('lab', d.mat), g = 0x2a2018;
  // jugs of chemistry on the floor in the corner past the bench
  c.box(0.2, 0.3, 0.14, 17.62, 0.15, -5.6, 0x5a3a22); c.cyl(0.03, 0.03, 0.04, 17.66, 0.32, -5.6, 0x1e1f20, { seg: 8 });
  c.box(0.18, 0.28, 0.13, 17.95, 0.14, -5.62, 0xd8d4c8, { ry: 0.2 }); c.cyl(0.028, 0.028, 0.04, 17.98, 0.3, -5.62, 0x2a4a8a, { seg: 8 });
  c.box(0.2, 0.3, 0.14, 18.55, 0.15, -5.58, 0x3a3c3e, { ry: -0.15 }); c.cyl(0.03, 0.03, 0.04, 18.58, 0.32, -5.58, 0xb8261e, { seg: 8 });
  c.box(0.2, 0.024, 0.14, 17.78, 0.32, -5.42, 0xd8d4c8, { ry: 0.5, rz: 1.45, fine: true });
  d.col(17.35, 19.0, -5.85, -5.15);
  // old test prints left on the line
  for (const [x, t] of [[13.6, 'printA'], [14.15, 'printB']] as const) {
    c.plane(0.24, 0.16, x, 1.99, -2.995, t);
    c.plane(0.24, 0.16, x, 1.99, -3.005, 'white', { ry: PI, color: 0xe8e4da });
    c.box(0.02, 0.05, 0.012, x, 2.12, -3.0, C.steel, { fine: true });
  }
  // bottles and boxes of film on the shelves by the door
  for (const [y, z, col, h] of [[0.515, -3.6, 0x5a3a22, 0.22], [0.515, -3.35, 0xd8d4c8, 0.26], [1.115, -3.55, 0x5a3a22, 0.2], [1.115, -3.3, 0x2a4a3a, 0.22], [1.715, -3.5, 0xd8d4c8, 0.24]] as const)
    c.bottle(13.0, y, z, 0.045, h, col);
  for (const z of [-3.85, -3.75]) c.box(0.1, 0.07, 0.07, 13.0, 1.75, z, 0xc8782a, { ry: 0.1 });
  // the wet bench: a towel over its edge, tongs and a thermometer in the trays, a box of paper
  c.box(0.3, 0.012, 0.16, 16.62, 0.946, -5.2, 0x7a8a8a);
  c.plane(0.3, 0.42, 16.62, 0.73, -5.094, 'white', { color: 0x7a8a8a });
  c.rod(v3(14.05, 0.99, -5.5), v3(14.2, 1.06, -5.25), 0.006, 0xc89a5a, { fine: true });
  c.rod(v3(15.38, 0.99, -5.52), v3(15.52, 1.06, -5.27), 0.006, 0xc89a5a, { fine: true });
  c.rod(v3(14.75, 0.98, -5.45), v3(14.85, 1.2, -5.55), 0.004, 0xe8e0c8, { fine: true });
  c.box(0.28, 0.07, 0.22, 13.62, 0.975, -5.32, 0x2a2c30);
  c.box(0.2, 0.002, 0.08, 13.62, 1.011, -5.2, 0xf2f0ea, { fine: true });
  c.floorDirt(1.4, 0.22, 14.8, -5.18, 'splash', 0xd8d0c0, 0, 0.9415);
  // a rubber mat in front of it, and what the trays have splashed
  c.box(2.4, 0.012, 0.55, 14.9, 0.006, -4.83, 0x1c1d1e);
  c.floorDirt(0.9, 0.7, 14.7, -4.5, 'splash', 0x2a1e12, 0.3, 0.014);
  c.dirt(0.8, 0.4, 14.7, 1.15, -5.796, 'splash', 0x3a2a1a);
  // the cables: the enlarger to its timer and the wall, the safelight to the ceiling
  c.plate(16.6, 1.15, -5.798, 0, 'outlet');
  c.cable([v3(17.08, 1.72, -5.5), v3(17.05, 1.3, -5.62), v3(17.02, 1.0, -5.66), v3(16.9, 0.985, -5.68), v3(16.85, 1.03, -5.58)], 0.005, C.cable);
  c.cable([v3(16.8, 1.0, -5.6), v3(16.7, 1.0, -5.75), v3(16.62, 1.12, -5.79)], 0.005, C.cable);
  c.rod(v3(15.0, 2.53, -5.68), v3(15.0, 2.75, -5.77), 0.004, C.cable, { fine: true });
  // notes by the enlarger, prints taped up over the archive bench
  c.notice(17.62, 1.55, -5.798, 0, 0.2, 0.25, 'exposure', { tilt: 0.02 });
  c.notice(17.95, 1.82, -5.798, 0, 0.17, 0.125, 'grades', { tilt: -0.06 });
  c.notice(18.998, 1.38, -2.25, -PI / 2, 0.15, 0.1, 'printA', { tilt: 0.05 });
  c.notice(18.998, 1.36, -3.8, -PI / 2, 0.15, 0.1, 'printB', { tilt: -0.04 });
  // the records desk: a stool under it, a mug, envelopes of prints, a pencil
  const sx = 16.42, sz = 0.44;
  c.cyl(0.16, 0.16, 0.035, sx, 0.6, sz, 0x3a3c3e, { seg: 12 });
  for (let i = 0; i < 3; i++) { const a = i * 2.094 + 0.3; c.rod(v3(sx + Math.cos(a) * 0.1, 0.59, sz + Math.sin(a) * 0.1), v3(sx + Math.cos(a) * 0.19, 0, sz + Math.sin(a) * 0.19), 0.012, C.steel); }
  c.torus(0.15, 0.007, sx, 0.25, sz, C.steel, { rx: PI / 2, fine: true });
  c.mug(16.15, 0.79, 0.52, 0xd8d0c0, { ry: 0.5 });
  rings(c, 16.3, 0.79, 0.38, 0.2);
  c.box(0.26, 0.04, 0.18, 18.0, 0.81, 0.52, C.manila, { ry: 0.1 });
  c.pencil(17.62, 0.79, 0.5, 0.6);
  // the floor by the door and on the way to the bench, a web in the corner
  c.floorDirt(1.0, 0.8, 13.3, -1.2, 'wear', g, PI / 2);
  c.floorDirt(1.4, 0.8, 15.4, -1.8, 'wear', g, 0.8);
  web(c, 13.0, 2.75, -5.6, -1, -1);
  done(c, d);
}

// ---------- the motel office (in the terms it was built in, world/MotelFront.ts) ----------
export function dressMotelOffice(d: Dress) {
  const c = new Clutter('office', d.mat), K = 1.02, g = 0x4a3c2c;   // the counter's top
  // a painting of the mesas, the calendar from the feed store, the rules by the door, a clock
  c.frame(-43.6, 1.72, 27.6, PI, 0.9, 0.56, 'painting', 0x6a4a2a);
  c.notice(-44.15, 1.55, 22.2, 0, 0.2, 0.3, 'calendar', { pins: true });
  c.notice(-41.2, 1.62, 22.72, -PI / 2, 0.24, 0.12, 'checkout');
  c.notice(-41.2, 1.3, 22.72, -PI / 2, 0.2, 0.15, 'rates', { tilt: 0.02 });
  c.notice(-41.2, 1.08, 22.8, -PI / 2, 0.16, 0.06, 'nopets', { tilt: -0.03 });
  c.clock(-41.2, 2.5, 24.1, -PI / 2, 0.13);
  // behind the counter: an adding machine, pencils, the ashtray, the paper, a mug
  c.box(0.2, 0.07, 0.24, -44.45, K + 0.035, 24.95, 0xcfc6ae, { ry: 0.2, faces: [null, null, 'adder'] });
  pencilCup(c, -44.55, K, 24.32);
  c.ashtray(-44.52, K, 25.72, 3);
  c.papers(-44.42, K, 26.0, 0.24, 0.32, 'news', { ry: 0.25 });
  c.mug(-44.62, K, 25.35, 0x8a2a22, { full: true, ry: 2.5 });
  c.plane(0.1, 0.06, -43.7, K + 0.028, 26.38, 'ringBell', { ry: PI / 2, rx: -0.25 });
  rings(c, -44.5, K, 25.47, 0.7);
  // brochures at the public end of the counter
  c.box(0.08, 0.03, 0.22, -43.75, K + 0.015, 23.72, 0x3a2a1c);
  c.box(0.012, 0.15, 0.09, -43.74, K + 0.09, 23.67, 0xffffff, { rz: 0.12, faces: ['brochureA'] });
  c.box(0.012, 0.15, 0.09, -43.73, K + 0.09, 23.77, 0xffffff, { rz: 0.1, faces: ['brochureB'] });
  // a fan on the shelves by the television, the clerk's cardigan by the key board
  c.fan(-45.33, 1.5, 27.28, PI / 2 + 0.3);
  c.coat(-45.0, 1.75, 22.2, 0, 0x3e4a5a);
  // the blind drawn up in the window, the mat inside the door
  c.box(0.06, 0.1, 2.0, -41.27, 2.04, 26.2, C.blind);
  c.box(0.05, 0.02, 2.0, -41.27, 1.98, 26.2, 0xb8b2a0, { fine: true });
  c.rod(v3(-41.3, 1.98, 25.4), v3(-41.3, 1.4, 25.4), 0.0015, 0xc8c4b8, { fine: true });
  c.cyl(0.008, 0.012, 0.04, -41.3, 1.38, 25.4, 0xc8c4b8, { fine: true, seg: 6 });
  c.box(0.62, 0.012, 0.95, -41.9, 0.006, 24.1, 0x4a3a2a);
  // the lamp's cord down behind the counter to the wall
  c.plate(-45.58, 0.3, 25.9, PI / 2, 'outlet');
  c.cable([v3(-44.42, K + 0.01, 26.38), v3(-44.75, K + 0.005, 26.42), v3(-44.8, 0.6, 26.5), v3(-44.85, 0.012, 26.6), v3(-45.5, 0.012, 26.1), v3(-45.57, 0.012, 25.92), v3(-45.575, 0.27, 25.9)], 0.006, 0x1e1f20);
  // the floor from the door to the counter, scuffs behind the chair, a stain under the window, the leak
  c.floorDirt(1.6, 0.8, -42.5, 24.1, 'wear', g, 0);
  c.floorDirt(0.9, 1.6, -43.2, 25.3, 'wear', g, 0);
  c.dirt(0.6, 0.16, -42.45, 0.45, 27.584, 'scuff', 0x1a1612, { ry: PI });
  c.dirt(0.45, 0.4, -41.202, 0.42, 26.6, 'stain', C.water, { ry: -PI / 2 });
  c.dirt(0.6, 0.5, -44.6, 2.785, 23.2, 'stain', C.water, { rx: PI / 2 });
  web(c, -45.4, 2.785, 22.4, -1, -1);
  done(c, d);
}

// ---------- room 6 (local metres, world/Room6.ts) ----------
export function dressRoom6(d: Dress) {
  const c = new Clutter('room6', d.mat), g = 0x2a2016, T6 = 0.81;   // the table's top
  c.frame(1.12, 1.78, -3.0, 0, 0.85, 0.53, 'painting', 0x4a3020);
  c.frame(-2.25, 1.78, -1.15, PI / 2, 0.3, 0.41, 'paintingTall', 0x4a3020);
  // her suitcase stood on end against the wall past the foot of the bed (turned along the wall,
  // so the way between the bed and the table stays open: reachcheck)
  const sx = 2.06, sz = -0.07;
  c.box(0.2, 0.44, 0.6, sx, 0.22, sz, 0x6a4a32);
  for (const dz of [-0.29, 0.29]) for (const y of [0.02, 0.42]) c.box(0.21, 0.04, 0.04, sx, y, sz + dz, 0x3a2a1c, { fine: true });
  c.box(0.03, 0.025, 0.14, sx, 0.47, sz, 0x2a1e14);
  for (const dz of [-0.06, 0.06]) c.box(0.02, 0.03, 0.015, sx, 0.45, sz + dz, 0x2a1e14, { fine: true });
  c.box(0.205, 0.02, 0.6, sx, 0.3, sz, 0x4a3422, { fine: true });
  d.col(1.94, 2.25, -0.38, 0.25);
  // her raincoat on a hook by the door
  c.coat(-2.25, 1.72, 1.0, PI / 2, 0x7a6a50, { long: true });
  // the table: cigarettes and matches, the butts in the ashtray, her coffee and its rings
  c.box(0.055, 0.022, 0.085, 0.43, T6 + 0.011, 1.84, 0xffffff, { ry: 0.3, faces: [null, null, 'pack'] });
  c.box(0.04, 0.006, 0.025, 0.53, T6 + 0.003, 1.79, 0xffffff, { ry: -0.4, faces: [null, null, 'matches'] });
  for (const [dx, dz, ry] of [[0.02, 0.01, 0.4], [-0.025, 0.015, 2.1], [0.005, -0.03, 1.2]]) c.cyl(0.0045, 0.0045, 0.03, 0.59 + dx, 0.852, 1.59 + dz, 0xd8b078, { rz: PI / 2 - 0.15, ry, seg: 5, fine: true });
  c.mug(1.27, T6, 1.84, 0xd8d0b8, { full: true, ry: 2.8 });
  rings(c, 1.2, T6, 1.68, -0.3);
  // the nightstand: pills and a glass; a paperback on the bed, shoes under its edge
  c.bottle(-0.47, 0.585, -2.7, 0.018, 0.075, 0xc87a2a, 0xf0ece0);
  c.cyl(0.03, 0.026, 0.09, -0.07, 0.63, -2.7, 0xb8c8cc, { seg: 10 });
  c.box(0.13, 0.02, 0.19, 1.55, 0.635, -1.05, 0xd8c08a, { ry: 0.5, faces: [null, null, 'paintingTall'] });
  c.box(0.125, 0.012, 0.18, 1.55, 0.629, -1.05, 0xe8e0c8, { ry: 0.5, fine: true });
  c.box(0.09, 0.07, 0.25, 0.2, 0.035, -1.0, 0x2a1e18, { ry: 0.15 });
  c.box(0.09, 0.07, 0.25, 0.22, 0.035, -1.27, 0x2a1e18, { ry: -0.1 });
  // the motel's cards: check out on the dresser, the fire card and the switch by the door
  c.plane(0.12, 0.06, -1.79, 0.905, -1.12, 'checkout', { ry: PI / 2, rx: -0.2 });
  c.plate(-0.6, 1.25, 2.998, PI); c.dirt(0.16, 0.2, -0.6, 1.25, 2.995, 'smudge', C.grime, { ry: PI });
  c.notice(-0.5, 1.58, 2.998, PI, 0.13, 0.17, 'fire');
  // the carpet where people walk, a coffee stain, two cigarette burns; the walls and the ceiling
  c.floorDirt(1.2, 0.9, -1.4, 2.4, 'wear', g, PI / 2);
  c.floorDirt(1.4, 0.8, -0.9, 1.3, 'wear', g, 0.4);
  c.floorDirt(1.2, 0.7, -0.35, -0.6, 'wear', g, 1.3);
  c.floorDirt(0.35, 0.3, 0.3, 0.75, 'stain', C.coffee, 0.3);
  c.floorDirt(0.04, 0.04, 0.9, 0.42, 'burn', 0x1a1210); c.floorDirt(0.035, 0.035, 1.45, 2.42, 'burn', 0x1a1210, 0.6);
  c.dirt(0.14, 0.32, 1.52, 0.62, 2.996, 'drip', C.water, { ry: PI });
  c.dirt(0.6, 0.15, -0.55, 0.32, 2.996, 'scuff', 0x1a1612, { ry: PI });
  c.dirt(0.55, 0.45, 1.8, 2.596, -2.5, 'stain', C.water, { rx: PI / 2 });
  web(c, 2.05, 2.595, -2.8, 1, -1, 0.36);
  done(c, d);
}

// ---------- the diner (local metres, world/Diner.ts) ----------
export function dressDiner(d: Dress) {
  const c = new Clutter('diner', d.mat), g = 0x2a2420, K = 1.0, B = 0.803;   // the counter's top, the booths' tables
  // the back bar against the partition south of the hatch: a low cabinet, shelves of cups,
  // plates and glasses, the shake mixer, a pie, the register
  c.box(0.42, 0.9, 2.6, -5.48, 0.45, 2.1, 0x7b9a89);
  c.box(0.46, 0.04, 2.64, -5.48, 0.92, 2.1, 0xb8b0a0);
  for (const y of [1.45, 1.85]) {
    c.box(0.22, 0.03, 2.4, -5.59, y, 2.1, 0x5d422e);
    for (const z of [1.0, 3.2]) c.box(0.2, 0.08, 0.03, -5.6, y - 0.055, z, 0x3a2a1c, { fine: true });
  }
  for (let i = 0; i < 5; i++) for (let k = 0; k < 3; k++) c.cyl(0.04, 0.034, 0.075, -5.6, 1.4 + 0.1 + k * 0.074, 1.2 + i * 0.22, 0xe8e2d4, { seg: 10, fine: k > 0 });
  c.cyl(0.11, 0.1, 0.12, -5.6, 1.525, 2.6 + 0.4, 0xece8de, { seg: 14 });
  for (let i = 0; i < 6; i++) c.cyl(0.03, 0.035, 0.11, -5.6, 1.92, 1.1 + i * 0.3, 0xb8c8cc, { seg: 10 });
  c.box(0.12, 0.04, 0.16, -5.45, 0.96, 1.2, C.steel);
  c.rod(v3(-5.5, 0.98, 1.2), v3(-5.5, 1.32, 1.2), 0.02, C.steel);
  c.box(0.1, 0.1, 0.12, -5.46, 1.3, 1.2, 0x7b9a89);
  c.cyl(0.04, 0.035, 0.16, -5.42, 1.12, 1.2, 0xc8ccd0, { seg: 10 });
  c.cyl(0.03, 0.05, 0.12, -5.45, 1.0, 2.0, C.steel, { seg: 10 });
  c.cyl(0.15, 0.15, 0.01, -5.45, 1.065, 2.0, 0xece8de, { seg: 16 });
  c.cyl(0.12, 0.13, 0.04, -5.45, 1.09, 2.0, 0xc08040, { seg: 16, top: 'pieTop' });
  c.box(0.34, 0.2, 0.3, -5.45, 1.04, 3.0, C.steel);
  c.box(0.24, 0.05, 0.26, -5.43, 1.17, 3.0, 0xcfc6ae, { rz: -0.35, faces: [null, null, 'adder'] });
  c.box(0.08, 0.1, 0.22, -5.55, 1.26, 3.0, 0x2a2c2e);
  // the counter: sugar, salt and pepper, ketchup, ashtrays, a paper; the driver's pie and coffee
  for (const z of [-2.7, 2.6]) {
    c.cyl(0.035, 0.035, 0.1, -4.42, K + 0.05, z, 0xc8d4d8, { seg: 10 }); c.cyl(0.036, 0.02, 0.04, -4.42, K + 0.12, z, C.steel, { seg: 10 });
    c.cyl(0.018, 0.02, 0.07, -4.36, K + 0.035, z + 0.09, 0xece8de, { seg: 8 }); c.cyl(0.018, 0.02, 0.07, -4.36, K + 0.035, z + 0.13, 0x8a8478, { seg: 8 });
    c.bottle(-4.32, K, z - 0.12, 0.028, 0.18, 0xa8221a, 0xe8e0d0);
  }
  c.ashtray(-4.35, K, -0.55, 2); c.ashtray(-4.35, K, 3.2, 4);
  c.papers(-4.42, K, -3.6, 0.22, 0.3, 'news', { ry: 0.2, n: 2 });
  c.cyl(0.11, 0.1, 0.012, -4.42, K + 0.006, 1.6, 0xffffff, { top: 'plate', seg: 16 });
  c.box(0.09, 0.04, 0.08, -4.45, K + 0.032, 1.58, 0xc8883a, { ry: 0.6 });
  c.box(0.015, 0.004, 0.14, -4.33, K + 0.014, 1.62, 0xb8bcbe, { ry: 0.3, fine: true });
  c.mug(-4.4, K, 1.88, 0xe8e2d4, { full: true, ry: 3.6 });
  // a denim jacket left on the stool beside the driver
  c.box(0.42, 0.05, 0.4, -3.4, 0.815, 3.2, 0x3a4a6a, { ry: 0.3 });
  c.box(0.12, 0.42, 0.04, -3.18, 0.62, 3.32, 0x3a4a6a, { ry: 0.3, rz: 0.12 });
  // the booths' tables: salt and pepper, sugar, a card; leftovers on two of them
  for (const z of [-6.1, -3.05, 3.05, 6.1]) {
    c.cyl(0.018, 0.02, 0.07, 1.2, B + 0.035, z - 0.05, 0xece8de, { seg: 8 }); c.cyl(0.018, 0.02, 0.07, 1.2, B + 0.035, z + 0.05, 0x8a8478, { seg: 8 });
    c.cyl(0.035, 0.035, 0.1, 1.32, B + 0.05, z + 0.12, 0xc8d4d8, { seg: 10 }); c.cyl(0.036, 0.02, 0.04, 1.32, B + 0.12, z + 0.12, C.steel, { seg: 10, fine: true });
    c.plane(0.06, 0.09, 1.065, B + 0.043, z - 0.2, 'tent', { ry: -PI / 2, rx: -0.25 });
    c.plane(0.06, 0.09, 1.095, B + 0.043, z - 0.2, 'tent', { ry: PI / 2, rx: -0.25 });
  }
  c.cyl(0.11, 0.1, 0.012, 0.45, B + 0.006, -3.0, 0xffffff, { top: 'plate', seg: 16 });
  c.mug(0.75, B, -3.25, 0xe8e2d4, { ry: 1 });
  c.ball(0.03, 0.4, B + 0.025, -2.85, 0xf0ece4, { fine: true });
  rings(c, 0.85, B, -2.95, 0.5);
  c.papers(0.55, B, 6.1, 0.3, 0.4, 'news', { ry: 0.4, n: 2 });
  c.ashtray(0.25, B, 5.92, 3);
  // the walls: old photographs, a painting, the pie board, help wanted, the calendar, a clock,
  // and a number written by the payphone
  c.frame(-1.1, 2.35, -8.8, 0, 0.48, 0.32, 'printB', 0x2a2420);
  c.frame(-3.6, 1.9, -8.8, 0, 0.8, 0.5, 'painting', 0x5a3a24);
  c.frame(0.9, 1.9, -8.8, 0, 0.4, 0.27, 'printA', 0x2a2420);
  c.clock(-2.4, 2.65, -8.8, 0, 0.2);
  c.frame(-5.7, 1.7, -4.6, PI / 2, 0.3, 0.38, 'pie', 0x5a4430);
  c.notice(-5.7, 1.62, -0.75, PI / 2, 0.16, 0.11, 'help', { tilt: 0.05 });
  c.notice(-3.65, 1.62, 8.8, PI, 0.2, 0.3, 'calendar', { pins: true });
  c.dirt(0.16, 0.08, 0.83, 1.7, 8.798, 'scrawl', 0x2a2a3a, { ry: PI });
  // two ceiling fans over the aisle, standing still
  for (const z of [-1.6, 4.6]) {
    const x = -0.45;
    c.rod(v3(x, 3.2, z), v3(x, 2.98, z), 0.014, 0x3a3c3e);
    c.cyl(0.11, 0.09, 0.1, x, 2.93, z, 0x3a3c3e, { seg: 12 });
    for (let i = 0; i < 4; i++) { const a = i * PI / 2 + 0.4; c.box(0.56, 0.012, 0.13, x + Math.cos(a) * 0.36, 2.89, z - Math.sin(a) * 0.36, 0x5d422e, { ry: a, rx: 0.12 }); }
    c.cyl(0.07, 0.07, 0.05, x, 2.86, z, 0xd8c890, { seg: 10, fine: true });
  }
  // the floor in front of the stools and at the door, kicks on the counter's front, dust, a web
  c.floorDirt(10.2, 1.0, -3.15, 0, 'wear', g, PI / 2);
  c.floorDirt(1.4, 1.1, 1.0, 0, 'wear', g, 0);
  c.floorDirt(14, 0.9, -1.2, 0, 'wear', 0x3a3028, PI / 2);
  for (let i = 0; i < 7; i++) c.floorDirt(0.5, 0.14, -3.05, -4.8 + i * 1.6, 'scuff', C.heel, 1.3 + i * 0.4);
  for (const z of [-3.9, -0.4, 2.2, 4.4]) c.dirt(0.8, 0.2, -4.207, 0.28, z, 'scuff', C.heel, { ry: PI / 2 });
  skirt(c, -2.0, -8.798, 0, 7.2); skirt(c, -2.0, 8.798, PI, 3.0);
  web(c, -5.5, 3.195, -8.6, -1, -1);
  done(c, d);
}

// ---------- the hut at STATION 01 (local metres, world/Station01.ts) ----------
export function dressHut(d: Dress) {
  const c = new Clutter('hut', d.mat), B = 0.8, g = 0x2a2016;   // the bench's top
  // a crate in the corner past the bench, a hotplate and the kettle on it, a jerrycan beside it
  c.box(0.5, 0.45, 0.42, -9.32, 0.225, 1.78, 0x8a6a48);
  for (const y of [0.12, 0.32]) c.box(0.505, 0.04, 0.425, -9.32, y, 1.78, 0x6e5034, { fine: true });
  c.box(0.22, 0.06, 0.22, -9.35, 0.48, 1.76, C.dark, { faces: [null, null, 'burner'] });
  c.kettle(-9.35, 0.51, 1.76, 0.6, 0xb8b6ae);
  c.plate(-9.75, 0.35, 1.502, 0, 'outlet');
  c.cable([v3(-9.45, 0.5, 1.68), v3(-9.6, 0.45, 1.6), v3(-9.7, 0.3, 1.53), v3(-9.75, 0.33, 1.508)], 0.005, C.cable);
  c.box(0.17, 0.34, 0.27, -9.17, 0.17, 2.27, 0x8a2a1e);
  c.box(0.05, 0.04, 0.12, -9.17, 0.36, 2.27, 0x6a1e16);
  c.cyl(0.02, 0.02, 0.06, -9.17, 0.37, 2.38, 0x6a1e16, { rx: 0.5, seg: 6 });
  d.col(-9.62, -9.0, 1.5, 2.43);
  // the field coat on a nail by the door
  c.coat(-10.9, 1.75, 4.498, PI, 0x7a6a48, { long: true });
  // an old calendar nobody turned, the generator log, a sketch of the station
  c.notice(-12.998, 1.7, 3.2, PI / 2, 0.2, 0.3, 'calendarOld', { pins: true, tilt: 0.03 });
  c.notice(-10.25, 1.55, 1.502, 0, 0.19, 0.24, 'genlog', { tape: false });
  c.cyl(0.004, 0.004, 0.02, -10.25, 1.665, 1.512, C.steel, { rx: PI / 2, fine: true, seg: 4 });
  c.notice(-12.5, 2.05, 1.502, 0, 0.26, 0.2, 'sketch', { pins: true, tilt: -0.04 });
  // the bench: a thermos, an enamel mug, a coffee can for the butts, field books, a pencil
  c.cyl(0.045, 0.045, 0.28, -10.72, B + 0.14, 1.68, 0x3e5a44, { seg: 10 });
  c.cyl(0.048, 0.048, 0.06, -10.72, B + 0.31, 1.68, 0x9a9e9c, { seg: 10 });
  c.mug(-10.58, B, 1.98, 0xe8ece8, { ry: 2, full: true });
  c.cyl(0.06, 0.06, 0.13, -12.7, B + 0.065, 2.0, 0xffffff, { side: 'can', top: 'ashtray', seg: 12 });
  c.box(0.14, 0.05, 0.2, -12.74, B + 0.025, 1.66, 0x5a4a32, { ry: 0.1 });
  c.box(0.13, 0.04, 0.19, -12.73, B + 0.07, 1.66, 0x3a4a3a, { ry: -0.05 });
  c.pencil(-10.95, B, 2.12, 0.8);
  // under the bench: a battery and a toolbox; a lantern hanging from the beam
  c.box(0.26, 0.17, 0.17, -10.5, 0.085, 1.82, 0x1e1f20);
  for (const dx of [-0.08, 0.08]) c.cyl(0.012, 0.012, 0.02, -10.5 + dx, 0.18, 1.82, dx < 0 ? 0xa82a22 : 0x8a8f92, { seg: 6, fine: true });
  c.box(0.42, 0.15, 0.18, -11.62, 0.075, 1.84, 0x8a2a1e);
  c.box(0.2, 0.02, 0.03, -11.62, 0.16, 1.84, C.dark, { fine: true });
  c.rod(v3(-12.3, 2.48, 3.8), v3(-12.3, 2.38, 3.8), 0.004, C.steel);
  c.torus(0.07, 0.005, -12.3, 2.33, 3.8, C.steel, { arc: PI, fine: true });
  c.cyl(0.04, 0.05, 0.05, -12.3, 2.27, 3.8, 0x2a3a2a, { seg: 10 });
  c.cyl(0.05, 0.06, 0.12, -12.3, 2.18, 3.8, 0x8a9490, { seg: 10 });
  c.cyl(0.07, 0.07, 0.03, -12.3, 2.105, 3.8, 0x2a3a2a, { seg: 10 });
  // the floor: sand in under the door, dusty boot prints to the bench, worn boards in front of it
  c.floorDirt(1.2, 0.5, -9.95, 4.32, 'wear', C.sand, 0, 0.007);
  c.floorDirt(0.6, 0.3, -10.25, 3.85, 'boots', 0xb8a888, 2.29, 0.007);
  c.floorDirt(0.6, 0.3, -10.8, 3.25, 'boots', 0xb8a888, 2.29, 0.007);
  c.floorDirt(0.6, 0.3, -11.45, 2.7, 'boots', 0xb8a888, 2.39, 0.007);
  c.floorDirt(1.8, 0.7, -11.3, 2.55, 'wear', g, 0, 0.007);
  c.floorDirt(0.5, 0.5, -12.4, 2.6, 'wear', g, 0, 0.007);
  // the walls: a stain under the east window, the hand marks by the door; webs in the corners
  c.dirt(0.16, 0.4, -9.002, 0.7, 2.8, 'drip', C.water, { ry: -PI / 2 });
  c.dirt(0.18, 0.22, -10.62, 1.0, 4.496, 'smudge', C.grime, { ry: PI });
  web(c, -9.2, 2.594, 1.7, 1, -1);
  web(c, -12.8, 2.594, 4.3, -1, 1);
  done(c, d);
}
