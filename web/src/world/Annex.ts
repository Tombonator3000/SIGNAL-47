import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood, annexFlood, setFlood, faced } from './kit';
import { GlowPoints } from './glow';
import * as T from '../core/textures';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// Chapter two location: the south corridor behind the control room and the SARO records
// room off it. A small extension of the main building, as the design bible asks for.
//
//   z 4.8 to 6.6   corridor, x -6 to 2. The control room door is at x -2.9 to -1.9.
//   z 6.8 to 10.5  records room, x -6 to -1. Its doorway is at x -4.4 to -3.4.
//                  East of it (x -1 to 2): restrooms and the operations office, closed.
//
// The rooms are lit only by their own fake lamps (flood set "annex"), like the photo lab.
export class RecordsAnnex {
  group = new THREE.Group();
  // Everything inside. The extension has no windows, so this is only drawn while the
  // player is in it or looking in through the open corridor door (see main.ts).
  interior = new THREE.Group();
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  colliders: Collider[] = [];
  objs: Record<string, THREE.Object3D> = {};
  glow = new GlowPoints();
  flood = annexFlood;
  private flicker = -1;
  private flickerTube!: THREE.Mesh;
  private tubeOn = new THREE.MeshBasicMaterial({ color: 0xeef3ff });
  private tubeDim = new THREE.MeshBasicMaterial({ color: 0x6d7378 });

  constructor() {
    const st = new THREE.Group();
    this.group.add(st, this.interior);
    this.shell(st);
    this.corridor(st);
    this.records(st);
    mergeStatic(st);
    // after merging, only the outside surfaces stay with the shell
    const outside = new Set<THREE.Material>([M.concrete, M.steel, M.darkPlastic]);
    for (const c of [...st.children]) {
      const m = c as THREE.Mesh;
      if (m.isMesh && !Array.isArray(m.material) && outside.has(m.material)) continue;
      this.interior.add(c);
    }
    this.group.add(this.glow.build());
  }

  private col(minX: number, maxX: number, minZ: number, maxZ: number) { this.colliders.push({ minX, maxX, minZ, maxZ }); }
  private addZone(id: string, minX: number, maxX: number, minZ: number, maxZ: number, enabled = true) {
    const z: Zone = { id, minX, maxX, minZ, maxZ, enabled };
    this.zones.push(z); this.zone[id] = z;
  }
  // Invisible box that interaction rays can hit.
  private proxy(name: string, w: number, h: number, d: number, x: number, y: number, z: number) {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), new THREE.MeshBasicMaterial({ visible: false }));
    m.position.set(x, y, z); m.name = name;
    this.group.add(m);
    this.objs[name] = m;
    return m;
  }
  private lit<T extends THREE.MeshStandardMaterial>(m: T) { return floodlit(m, 0.25, this.flood); }
  private mat(o: THREE.MeshStandardMaterialParameters) { return this.lit(new THREE.MeshStandardMaterial(o)); }
  private card(lines: string[], o: Parameters<typeof T.labelCard>[1]) { return this.mat({ map: T.labelCard(lines, o), roughness: 0.85 }); }

  // ---------- walls, floor, ceiling and the outside of the extension ----------
  private corrWall!: THREE.MeshStandardMaterial;
  private recWall!: THREE.MeshStandardMaterial;
  private shell(st: THREE.Group) {
    const ext = M.concrete;
    this.corrWall = this.mat({ color: 0xa69d88, map: T.wallPaint(), roughness: 0.92 });
    this.recWall = this.mat({ color: 0x8a9684, map: T.wallPaint(), roughness: 0.92 });
    const cw = this.corrWall, rw = this.recWall;
    const ceil = this.mat({ color: 0xb9b3a4, map: T.ceilingTiles(), roughness: 1 });
    const H = 2.8;
    // outer walls: west (corridor part, records part), south, east
    faced(st, 0.3, H, 1.9, ext, cw, -6.15, H / 2, 5.75, '+x');
    faced(st, 0.3, H, 4.1, ext, rw, -6.15, H / 2, 8.75, '+x');
    faced(st, 8.6, H, 0.3, ext, rw, -2.0, H / 2, 10.65, '-z');
    faced(st, 0.3, H, 6.0, ext, cw, 2.15, H / 2, 7.8, '-x');
    // corridor / records partition, with the records doorway (x -4.4 to -3.4)
    faced(st, 1.6, H, 0.2, rw, cw, -5.2, H / 2, 6.7, '-z');
    faced(st, 1.0, H - 2.1, 0.2, rw, cw, -3.9, 2.1 + (H - 2.1) / 2, 6.7, '-z');
    faced(st, 5.4, H, 0.2, rw, cw, -0.7, H / 2, 6.7, '-z');
    // records / restrooms partition
    box(st, 0.2, H, 3.7, rw, -0.9, H / 2, 8.65);
    // the control room's south wall seen from the corridor: lined like the corridor
    plane(st, 3.1, H, cw, -4.45, H / 2, 4.805);
    plane(st, 3.9, H, cw, 0.05, H / 2, 4.805);
    plane(st, 1.0, H - 2.12, cw, -2.4, 2.12 + (H - 2.12) / 2, 4.805);
    // ceiling slab and roof, plinth
    faced(st, 8.6, 0.4, 6.0, ext, ceil, -2.0, H + 0.2, 7.8, '-y');
    box(st, 9.2, 0.6, 5.3, ext, -2.0, -0.31, 8.45);
    // floors
    const vinyl = this.mat({ color: 0xcfc4ad, map: T.vinylTiles(), roughness: 0.45 });
    const lino = this.mat({ color: 0x8f8a74, map: T.vinylTiles(), roughness: 0.6 });
    plane(st, 8.0, 1.8, vinyl, -2.0, 0.002, 5.7, 0, -Math.PI / 2);
    plane(st, 5.0, 3.7, lino, -3.5, 0.002, 8.65, 0, -Math.PI / 2);
    // skirting
    const skirt = this.mat({ color: 0x3a3a34, roughness: 0.7 });
    box(st, 3.1, 0.1, 0.02, skirt, -4.45, 0.05, 4.82); box(st, 3.9, 0.1, 0.02, skirt, 0.05, 0.05, 4.82);
    box(st, 1.6, 0.1, 0.02, skirt, -5.2, 0.05, 6.585); box(st, 5.4, 0.1, 0.02, skirt, -0.7, 0.05, 6.585);
    // outside: a wall pack on the south face, the fire exit on the west side, a roof unit
    box(st, 0.35, 0.25, 0.25, M.steel, -2.0, 2.4, 10.92);
    this.glow.add(-2.0, 2.3, 11.06, 1.6, 0xffbe6e);
    addFlood(-2.0, 2.2, 11.6, 8);
    box(st, 0.05, 2.1, 1.0, M.steel, -6.33, 1.05, 5.7);
    box(st, 0.3, 0.06, 0.6, M.steel, -6.45, 2.25, 5.7);
    this.glow.add(-6.45, 2.18, 5.7, 0.7, 0xffc47a);
    box(st, 1.6, 0.8, 1.1, M.steel, -4.2, 3.6, 8.6);
    box(st, 1.5, 0.04, 1.0, M.darkPlastic, -4.2, 4.02, 8.6);

    // walkable areas: corridor, records room and the two doorways
    this.addZone('southDoor', -2.88, -1.92, 3.6, 5.8, false);
    this.addZone('corridor', -5.75, 1.75, 5.05, 6.35);
    this.addZone('recordsDoor', -4.38, -3.42, 5.6, 7.7);
    this.addZone('records', -5.75, -1.25, 7.05, 10.25);
  }

  private tube(st: THREE.Group, x: number, z: number, horizontal = true) {
    box(st, horizontal ? 1.3 : 0.36, 0.06, horizontal ? 0.36 : 1.3, M.steel, x, 2.77, z);
    const t = box(this.interior, horizontal ? 1.2 : 0.26, 0.03, horizontal ? 0.26 : 1.2, this.tubeOn, x, 2.735, z);
    noMerge(t);
    return t;
  }

  // ---------- the corridor ----------
  private corridor(st: THREE.Group) {
    const f = this.flood;
    this.tube(st, -3.7, 5.7); this.tube(st, 0.2, 5.7);
    addFlood(-3.7, 2.5, 5.7, 6, 0xe4ecff, f);
    addFlood(0.2, 2.5, 5.7, 6, 0xe4ecff, f);
    const door = this.mat({ color: 0x6b5a45, roughness: 0.7 });
    const frame = this.mat({ color: 0x2b3033, roughness: 0.5, metalness: 0.5 });
    const steel = this.mat({ color: 0x7c8387, roughness: 0.4, metalness: 0.6 });
    const sign = (lines: string[], x: number, y: number, z: number, ry: number, w = 0.5) => plane(st, w, 0.18, this.card(lines, { w: 256, h: 92, size: 30 }), x, y, z, ry);

    // over the control room door, and over the records doorway
    sign(['CONTROL ROOM'], -2.4, 2.4, 4.82, 0, 0.56);
    sign(['RECORDS'], -3.9, 2.4, 6.585, Math.PI, 0.44);
    // operations office (closed) on the south side, restrooms at the east end, fire exit at the west end
    box(st, 0.96, 2.05, 0.04, door, 0.45, 1.03, 6.57);
    box(st, 1.06, 0.07, 0.06, frame, 0.45, 2.1, 6.57);
    box(st, 0.14, 0.04, 0.05, steel, 0.81, 1.0, 6.53);
    sign(['OPERATIONS'], 0.45, 1.62, 6.545, Math.PI, 0.42);
    this.proxy('officeDoor', 1.0, 2.0, 0.2, 0.45, 1.05, 6.5);
    box(st, 0.04, 2.05, 0.96, door, 1.98, 1.03, 5.7);
    box(st, 0.06, 0.07, 1.06, frame, 1.98, 2.1, 5.7);
    box(st, 0.05, 0.04, 0.14, steel, 1.94, 1.0, 5.34);
    sign(['RESTROOMS'], 1.955, 1.62, 5.7, -Math.PI / 2, 0.42);
    this.proxy('restroomDoor', 0.2, 2.0, 1.0, 1.9, 1.05, 5.7);
    box(st, 0.04, 2.05, 0.96, this.mat({ color: 0x5a6466, roughness: 0.5, metalness: 0.4 }), -5.98, 1.03, 5.7);
    box(st, 0.06, 0.05, 0.8, steel, -5.93, 1.0, 5.7); // push bar
    const exit = plane(st, 0.42, 0.16, new THREE.MeshBasicMaterial({ map: T.labelCard(['EXIT'], { w: 160, h: 60, size: 40, bg: '#5a0f0b', fg: '#ff5a45', border: false }), toneMapped: false }), -5.985, 2.32, 5.7, Math.PI / 2);
    noMerge(exit); this.interior.add(exit);
    this.glow.add(-5.9, 2.32, 5.7, 0.6, 0xff3a28);
    this.proxy('exitDoor', 0.2, 2.0, 1.0, -5.9, 1.05, 5.7);

    // vending machine against the south wall, east end
    const vm = new THREE.Group();
    vm.position.set(1.35, 0, 6.2);
    box(vm, 0.8, 1.8, 0.72, this.mat({ color: 0x2a3036, roughness: 0.5, metalness: 0.4 }), 0, 0.9, 0);
    const face = plane(vm, 0.74, 1.48, new THREE.MeshBasicMaterial({ map: T.vendingFront(), toneMapped: false, color: 0xd8dde0 }), 0, 1.0, -0.365, Math.PI);
    noMerge(face);
    mergeStatic(vm);
    this.interior.add(vm);
    this.objs.vending = vm;
    addFlood(1.35, 1.2, 5.6, 2.6, 0xcfe0ff, f);
    this.col(0.93, 1.77, 5.8, 6.6);

    // notice board on the corridor's north wall
    box(st, 1.1, 0.75, 0.03, this.mat({ color: 0x8a6a45, roughness: 1 }), -0.6, 1.6, 4.83);
    const pr = T.rng(17);
    for (let i = 0; i < 5; i++) {
      const p = plane(st, 0.2, 0.26, this.mat({ map: T.deskPapers(i + 21), roughness: 0.9 }), -0.95 + (i % 3) * 0.33, 1.72 - Math.floor(i / 3) * 0.32, 4.85);
      p.rotation.z = (pr() - 0.5) * 0.16;
    }
  }

  // ---------- the records room ----------
  private records(st: THREE.Group) {
    const f = this.flood;
    const green = this.mat({ color: 0x55705e, roughness: 0.5, metalness: 0.45 });
    const greenDark = this.mat({ color: 0x3d5446, roughness: 0.55, metalness: 0.4 });
    const card = this.mat({ color: 0xa3865d, roughness: 0.9 });
    const paper = this.mat({ color: 0xe6dfcb, roughness: 0.9 });
    const wood = this.mat({ color: 0x6e5a42, roughness: 0.6 });
    const desk = this.mat({ color: 0x5b6664, roughness: 0.5 });
    const steel = this.mat({ color: 0x7c8387, roughness: 0.4, metalness: 0.6 });

    this.flickerTube = this.tube(st, -4.6, 8.0);
    this.tube(st, -2.4, 9.4);
    addFlood(-4.6, 2.5, 8.0, 5.5, 0xe4ecff, f);
    this.flicker = addFlood(-2.4, 2.5, 9.4, 5.5, 0xe4ecff, f);
    // the door leaf stands open against the wall inside the room
    const leaf = new THREE.Group();
    leaf.position.set(-3.42, 0, 6.82);
    box(leaf, 0.045, 2.04, 0.93, this.mat({ color: 0x6b5a45, roughness: 0.7 }), 0, 1.02, 0.47);
    box(leaf, 0.06, 0.04, 0.14, steel, -0.04, 1.0, 0.82);
    st.add(leaf);
    this.col(-3.48, -3.36, 6.8, 7.78);
    plane(st, 0.62, 0.22, this.card(['RECORDS ROOM', 'NO SMOKING / NO FOOD'], { w: 256, h: 92, size: 24 }), -2.6, 2.05, 6.81);

    // west wall: steel shelving with archive boxes
    for (const z of [7.15, 8.65, 10.15]) box(st, 0.45, 2.3, 0.04, greenDark, -5.78, 1.15, z);
    for (const y of [0.12, 0.62, 1.12, 1.62, 2.12]) box(st, 0.45, 0.025, 3.0, green, -5.78, y, 8.65);
    const r = T.rng(5);
    for (const y of [0.135, 0.635, 1.135, 1.635]) {
      for (let i = 0; i < 7; i++) {
        const z = 7.4 + i * 0.4;
        if (y === 1.135 && i === 3) continue; // the STATION 01 box sits here, pulled out
        const bw = 0.32 + r() * 0.04, bh = 0.24 + r() * 0.05;
        box(st, 0.38, bh, bw, card, -5.76, y + bh / 2, z);
        plane(st, 0.16, 0.1, paper, -5.565, y + bh * 0.55, z, Math.PI / 2);
      }
    }
    this.col(-6.0, -5.5, 7.1, 10.2);
    // the 1947 field records box, pulled half out of the shelf
    const fb = new THREE.Group();
    fb.position.set(-5.62, 1.135, 8.6);
    box(fb, 0.38, 0.28, 0.34, this.mat({ color: 0x8c6d48, roughness: 0.9 }), 0, 0.14, 0);
    plane(fb, 0.26, 0.17, this.card(['STATION 01', 'FIELD RECORDS', '1947'], { w: 192, h: 128, font: 'Special Elite', size: 22, surface: 'paper' }), 0.191, 0.15, 0, Math.PI / 2);
    mergeStatic(fb);
    this.interior.add(fb);
    this.objs.fieldBoxModel = fb;
    this.proxy('fieldBox', 0.5, 0.4, 0.5, -5.55, 1.28, 8.6);

    // south wall: three four-drawer filing cabinets, then a low bookcase with binders
    for (const x of [-5.0, -4.46, -3.92]) {
      box(st, 0.5, 1.32, 0.62, green, x, 0.66, 10.17);
      for (let k = 0; k < 4; k++) {
        box(st, 0.44, 0.27, 0.02, greenDark, x, 0.2 + k * 0.32, 9.855);
        box(st, 0.12, 0.025, 0.03, steel, x, 0.28 + k * 0.32, 9.84);
        plane(st, 0.09, 0.05, paper, x, 0.33 + k * 0.32, 9.843, Math.PI);
      }
    }
    this.col(-5.27, -3.65, 9.84, 10.5);
    box(st, 1.9, 0.9, 0.36, wood, -2.35, 0.45, 10.3);
    const binderMats = [0x2f4a6e, 0x6e2f2a, 0x3f5b3a, 0x2b2b30].map((c) => this.mat({ color: c, roughness: 0.8 }));
    for (let i = 0; i < 12; i++) box(st, 0.075, 0.3, 0.25, binderMats[i % 4], i < 6 ? -3.2 + i * 0.1 : -1.95 + (i - 6) * 0.1, 1.05, 10.3);
    this.col(-3.32, -1.38, 10.1, 10.5);
    // the service copies binder, taken down and lying on the bookcase
    const bd = new THREE.Group();
    bd.position.set(-2.32, 0.9, 10.24); bd.rotation.y = 0.25;
    box(bd, 0.3, 0.06, 0.34, this.mat({ color: 0x2f4a6e, roughness: 0.8 }), 0, 0.03, 0);
    plane(bd, 0.2, 0.12, this.card(['SERVICE COPIES', 'REFERENCE RECORDS'], { w: 192, h: 112, size: 20, surface: 'paper' }), 0, 0.061, 0, 0, -Math.PI / 2);
    mergeStatic(bd);
    this.interior.add(bd);
    this.proxy('binder', 0.45, 0.25, 0.45, -2.32, 1.0, 10.2);

    // east wall: card index cabinet for maintenance records, wall phone, calendar
    const ci = new THREE.Group();
    ci.position.set(-1.25, 0, 7.95);
    box(ci, 0.5, 1.05, 0.9, greenDark, 0, 0.525, 0);
    for (let row = 0; row < 4; row++) for (let c = 0; c < 3; c++) {
      box(ci, 0.02, 0.2, 0.24, green, -0.255, 0.18 + row * 0.24, -0.29 + c * 0.29);
      plane(ci, 0.07, 0.04, paper, -0.267, 0.24 + row * 0.24, -0.29 + c * 0.29, -Math.PI / 2);
    }
    box(ci, 0.36, 0.11, 0.24, green, -0.33, 0.78, 0.0); // one drawer pulled out
    plane(ci, 0.12, 0.08, paper, -0.36, 0.84, 0.0, 0, -Math.PI / 2 + 0.3);
    plane(ci, 0.5, 0.18, this.card(['MAINTENANCE', 'REFERENCE LINEAGE'], { w: 256, h: 92, size: 22 }), 0.243, 1.3, 0, -Math.PI / 2);
    mergeStatic(ci);
    this.interior.add(ci);
    this.col(-1.8, -1.0, 7.5, 8.4);
    this.proxy('lineage', 0.6, 1.2, 0.95, -1.3, 0.65, 7.95);

    const ph = new THREE.Group();
    ph.position.set(-1.02, 1.42, 9.25); ph.rotation.y = -Math.PI / 2;
    const beige = this.mat({ color: 0xcfc3a3, roughness: 0.55 }), keys = this.mat({ color: 0x2a2c30, roughness: 0.6 });
    box(ph, 0.18, 0.28, 0.07, beige, 0, 0, 0.035);
    box(ph, 0.05, 0.24, 0.06, beige, -0.06, 0.0, 0.1);
    for (let rr = 0; rr < 4; rr++) for (let c = 0; c < 3; c++) box(ph, 0.02, 0.016, 0.01, keys, 0.03 + (c - 1) * 0.03, 0.06 - rr * 0.035, 0.072);
    rod(ph, new THREE.Vector3(-0.06, -0.12, 0.1), new THREE.Vector3(-0.02, -0.32, 0.05), 0.006, keys, 4);
    mergeStatic(ph);
    this.interior.add(ph);
    this.objs.recPhoneModel = ph;
    this.proxy('recPhone', 0.25, 0.4, 0.3, -1.12, 1.42, 9.25);
    plane(st, 0.26, 0.32, this.mat({ map: T.calendarApril1986(), roughness: 0.9 }), -1.005, 1.65, 9.8, -Math.PI / 2);

    // work table in the middle, with a green desk lamp and a blotter
    box(st, 1.7, 0.05, 0.85, desk, -3.4, 0.745, 8.6);
    for (const [x, z] of [[-4.2, 8.22], [-2.6, 8.22], [-4.2, 8.98], [-2.6, 8.98]]) box(st, 0.05, 0.72, 0.05, steel, x, 0.36, z);
    box(st, 0.6, 0.006, 0.42, this.mat({ color: 0x3b4f3f, roughness: 0.9 }), -3.4, 0.773, 8.6);
    for (let i = 0; i < 3; i++) { const p = plane(st, 0.21, 0.29, paper, -3.75 + i * 0.05, 0.777 + i * 0.002, 8.62 - i * 0.03, 0, -Math.PI / 2); p.rotation.z = 0.12 * i - 0.1; }
    const lamp = new THREE.Group();
    lamp.position.set(-2.82, 0.77, 8.35);
    cyl(lamp, 0.08, 0.09, 0.03, this.mat({ color: 0x23262a, roughness: 0.5 }), 0, 0.015, 0, 14);
    rod(lamp, new THREE.Vector3(0, 0.03, 0), new THREE.Vector3(0, 0.32, 0), 0.01, steel, 6);
    const shade = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.05, 0.3, 14, 1, false, 0, Math.PI), this.mat({ color: 0x1f5a3a, roughness: 0.3, metalness: 0.3, side: THREE.DoubleSide }));
    shade.rotation.set(0, 0, Math.PI / 2); shade.position.set(0, 0.36, 0.0);
    lamp.add(shade);
    st.add(lamp);
    addFlood(-2.85, 1.05, 8.4, 3.2, 0xffcf8a, f);
    this.glow.add(-2.82, 1.08, 8.36, 0.3, 0xffd59a);
    this.col(-4.28, -2.52, 8.15, 9.05);
    this.proxy('workTable', 1.7, 0.3, 0.85, -3.4, 0.9, 8.6);
    // a chair pushed in at the table
    const chair = this.mat({ color: 0x3a3f46, roughness: 0.95 });
    box(st, 0.44, 0.05, 0.42, chair, -3.4, 0.46, 9.35);
    box(st, 0.44, 0.42, 0.05, chair, -3.4, 0.72, 9.58);
    for (const [x, z] of [[-3.6, 9.17], [-3.2, 9.17], [-3.6, 9.53], [-3.2, 9.53]]) box(st, 0.03, 0.44, 0.03, steel, x, 0.22, z);
    this.col(-3.65, -3.15, 9.15, 9.62);
    // fire extinguisher by the door
    cyl(st, 0.08, 0.08, 0.46, this.mat({ color: 0xa3241b, roughness: 0.45 }), -4.75, 0.5, 6.95, 12);
    box(st, 0.04, 0.06, 0.2, steel, -4.75, 0.6, 6.83);
  }

  // A tired tube flickers now and then.
  update(t: number) {
    this.glow.update(t);
    const on = !(Math.sin(t * 0.37) > 0.985 && Math.sin(t * 41) > 0);
    this.flickerTube.material = on ? this.tubeOn : this.tubeDim;
    setFlood(this.flicker, on ? 5.5 : 1.8, this.flood);
  }
}
