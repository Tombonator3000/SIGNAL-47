import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, addFlood, floodSet, setFlood, flood, faced } from './kit';
import { lampPost, railing, clipboard } from './props';
import { GlowPoints } from './glow';
import * as T from '../core/textures';
import { artTexture } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';

// Chapter one location, east of the control room (the array is north, -Z).
// A raised concrete walk with handrails leads north from the east door, past the
// S-03 motor bus cabinet, to the B-12 optical reference at the fence. The sight line
// runs S-03 apron -> B-12 vane -> S-03 antenna. The photo lab is a small block east
// of the walk.
export const YARD = {
  apron: { minX: 8.0, maxX: 12.6, minZ: -15.2, maxZ: -8.6 },     // where FRAME 01 must be taken
  sightMark: new THREE.Vector3(10.0, 0, -15.0),                    // FRAME 02 is taken near this
  vane: new THREE.Vector3(9.9, 2.35, -18.35),                      // centre of the B-12 vane board
  echoOffset: 0.45,                                                // film-only stripe, beside the vane
  // the service truck's pad below the east landing (chapter three drives out from here);
  // the truck faces south, towards the ramp, with the driver's door on the open side
  truck: { x: 8.3, z: 8.0, heading: Math.PI },
};

const VANE_W = 0.34, VANE_H = 1.36;

export class ServiceYard {
  group = new THREE.Group();
  zones: Zone[] = [];
  colliders: Collider[] = [];
  objs: Record<string, THREE.Object3D> = {};
  occluders: THREE.Object3D[] = [];
  glow = new GlowPoints();
  labFlood = floodSet(5, 'lab', 0.22);
  vane!: THREE.Group;          // yaw 0 faces the apron
  echo!: THREE.Mesh;           // visible only while a photograph is taken
  vaneStripe!: THREE.Mesh;     // the ivory stripe, used for marking in prints
  labDoorHinge!: THREE.Group;
  wetPrint!: THREE.Mesh;
  dryPrints: THREE.Mesh[] = [];
  bus = T.screenCanvas(256, 160);
  workLamp = -1;
  private workGlow = -1;
  workLampBase = 7;
  safelight = -1;
  zone: Record<string, Zone> = {};

  constructor() {
    const st = new THREE.Group();  // static exterior, merged
    const lab = new THREE.Group(); // static lab interior, merged
    this.group.add(st, lab);
    this.walk(st);
    this.s03(st);
    this.b12(st);
    this.labBlock(st, lab);
    this.signs(st);
    this.details(st);
    mergeStatic(st);
    mergeStatic(lab);
    this.group.add(this.glow.build());
  }

  private col(minX: number, maxX: number, minZ: number, maxZ: number) { this.colliders.push({ minX, maxX, minZ, maxZ }); }
  // Invisible box that interaction rays can hit (small or merged objects are hard to aim at).
  private proxy(name: string, w: number, h: number, d: number, x: number, y: number, z: number) {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), new THREE.MeshBasicMaterial({ visible: false }));
    m.position.set(x, y, z); m.name = name;
    this.group.add(m);
    this.objs[name] = m;
    return m;
  }
  private addZone(id: string, minX: number, maxX: number, minZ: number, maxZ: number, enabled = true) {
    const z: Zone = { id, minX, maxX, minZ, maxZ, enabled };
    this.zones.push(z); this.zone[id] = z;
    return z;
  }

  // ---------- the walk ----------
  private walk(st: THREE.Group) {
    const slab = floodlit(new THREE.MeshStandardMaterial({ map: T.concrete(12, [2, 6]), roughness: 0.92, color: 0xb3aa98 }));
    const slabBox = (x0: number, x1: number, z0: number, z1: number) => box(st, x1 - x0, 0.66, z1 - z0, slab, (x0 + x1) / 2, -0.33, (z0 + z1) / 2);
    slabBox(6.3, 11.0, -0.6, 3.4);     // landing outside the east door
    slabBox(8.0, 11.0, -19.6, -0.6);   // the walk to the fence
    slabBox(11.0, 12.6, -13.4, -8.6);  // S-03 cabinet apron
    slabBox(11.0, 12.6, -1.9, -0.6);   // path to the lab door
    slabBox(6.6, 13.0, 3.4, 12.4);     // the truck pad below the landing
    // the ramp the truck uses, down to the desert on the south side
    const ramp = box(st, 6.4, 0.3, 4.4, slab, 9.8, -0.42, 14.4);
    ramp.rotation.x = Math.atan2(0.62, 4.4);

    // Walkable areas. Where two areas join they overlap by more than the player's
    // diameter, because the player must fit inside one of them at every step.
    this.addZone('landing', 6.3, 11.0, -0.6, 3.4);
    this.addZone('walk', 8.0, 11.0, -19.55, 0.5);
    this.addZone('apron', 10.2, 12.6, -13.4, -8.6);
    this.addZone('labPath', 10.2, 12.7, -1.9, -0.5);
    this.addZone('eastDoor', 5.0, 7.1, 1.12, 2.08, false);
    // down from the landing through the gap in its railing, to the truck
    this.addZone('truckPad', 6.6, 13.0, 2.7, 12.4);
    const tk = YARD.truck;
    this.col(tk.x - 1.1, tk.x + 1.1, tk.z - 2.75, tk.z + 2.75);
    this.col(6.3, 9.4, 3.35, 3.45);   // the railing that is left west of the gap
    this.col(10.95, 11.05, 2.6, 3.45); // the landing's east railing, where the pad overlaps it
    this.col(6.72, 6.98, 11.47, 11.73); // the pad's lamp post
    this.proxy('truck', 2.1, 1.9, 5.3, tk.x, 0.95, tk.z);

    const v = (x: number, z: number) => new THREE.Vector3(x, 0, z);
    railing(st, v(8.0, -0.6), v(8.0, -19.6));
    railing(st, v(6.3, -0.6), v(8.0, -0.6));
    railing(st, v(6.3, 3.4), v(9.4, 3.4));      // the gap east of here leads down to the truck pad
    railing(st, v(13.0, 3.4), v(13.0, 12.4));
    railing(st, v(11.0, 3.4), v(11.0, -0.6));
    railing(st, v(11.0, -1.9), v(11.0, -8.6));
    railing(st, v(11.0, -8.6), v(12.6, -8.6));
    railing(st, v(12.6, -8.6), v(12.6, -13.4));
    railing(st, v(12.6, -13.4), v(11.0, -13.4));
    railing(st, v(11.0, -13.4), v(11.0, -19.6));
    railing(st, v(8.0, -19.6), v(11.0, -19.6));

    // three warm lamp posts along the walk, shadowless like the Unity yard
    for (const [x, z, ry] of [[11.25, 1.6, Math.PI], [11.25, -6.0, Math.PI], [7.75, -9.2, 0], [6.85, 11.6, 0]]) {
      const lp = lampPost(4.4, 0.95);
      lp.group.position.set(x, 0, z); lp.group.rotation.y = ry;
      st.add(lp.group);
      const head = lp.head.clone().applyAxisAngle(new THREE.Vector3(0, 1, 0), ry).add(lp.group.position);
      addFlood(head.x, head.y, head.z, 9, 0xffa24a);
      this.glow.add(head.x, head.y - 0.05, head.z, 1.5, 0xffb760);
    }
    // markings: S-03 apron and the B-12 sight line, painted on the concrete
    const apronMark = plane(st, 2.2, 1.1, new THREE.MeshBasicMaterial({ map: T.floorStencil(['S-03 APRON']), transparent: true, depthWrite: false, color: 0x8f7f55 }), 9.5, 0.006, -11.6, 0, -Math.PI / 2);
    apronMark.rotation.z = Math.PI / 2;
    const sight = plane(st, 1.6, 0.8, new THREE.MeshBasicMaterial({ map: T.floorStencil(['B-12', 'SIGHT LINE']), transparent: true, depthWrite: false, color: 0x8f7f55 }), YARD.sightMark.x, 0.006, YARD.sightMark.z, 0, -Math.PI / 2);
    noMerge(sight); this.group.add(sight);
  }

  // ---------- S-03 motor bus cabinet ----------
  private s03(st: THREE.Group) {
    const cab = new THREE.Group();
    cab.position.set(12.3, 0, -11.0);
    cab.rotation.y = -Math.PI / 2; // door faces west, towards the walk
    const steel = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('cabinet'), color: 0xa6b3af, roughness: 0.55, metalness: 0.45 }));
    const face = floodlit(new THREE.MeshStandardMaterial({ map: T.cabinetFace('S-03', ['MOTOR BUS', 'SARO ARRAY']), roughness: 0.6, metalness: 0.3 }));
    faced(cab, 0.82, 1.62, 0.46, steel, face, 0, 0.81, 0, '+z');
    box(cab, 0.9, 0.08, 0.52, steel, 0, 1.66, 0);
    box(cab, 0.9, 0.1, 0.52, M.concrete, 0, 0.05, 0);
    // display behind the inspection glass (drawn by the chapter code)
    const scr = plane(cab, 0.42, 0.26, new THREE.MeshBasicMaterial({ map: this.bus.tex, toneMapped: false }), 0, 1.055, 0.232);
    noMerge(scr); cab.add(scr);
    // conduit into the ground and the clipboard shelf
    cyl(cab, 0.04, 0.04, 0.5, M.steel, 0.3, 0.25, -0.3, 6);
    box(cab, 0.3, 0.02, 0.16, steel, -0.24, 0.47, 0.31);
    const log = clipboard(T.labelCard(['ANTENNA LOG', 'S-03', '', '02:15  026', '      ???'], { w: 128, h: 170, font: 'Special Elite', size: 16, border: false }), (m) => floodlit(m));
    log.position.set(-0.24, 0.48, 0.31); log.rotation.x = -0.5;
    cab.add(log);
    const proc = plane(cab, 0.24, 0.34, floodlit(new THREE.MeshStandardMaterial({ map: artTexture('procedure'), roughness: 0.9 })), 0.42, 1.0, 0.05, Math.PI / 2);
    noMerge(proc); cab.add(proc);
    mergeStatic(cab);
    this.group.add(cab);
    this.objs.motorBus = cab;
    this.occluders.push(cab);
    this.col(11.98, 12.6, -11.5, -10.5);
  }

  // ---------- B-12 optical reference ----------
  private b12(st: THREE.Group) {
    // tripod and mast
    const base = new THREE.Vector3(YARD.vane.x, 0, YARD.vane.z);
    for (let k = 0; k < 3; k++) {
      const a = k / 3 * Math.PI * 2 + 0.3;
      rod(st, base.clone().add(new THREE.Vector3(Math.cos(a) * 0.45, 0, Math.sin(a) * 0.45)), base.clone().setY(1.25), 0.02, M.steel, 5);
      box(st, 0.14, 0.02, 0.14, M.darkPlastic, base.x + Math.cos(a) * 0.45, 0.01, base.z + Math.sin(a) * 0.45);
    }
    cyl(st, 0.03, 0.03, 0.5, M.steel, base.x, 1.45, base.z, 6);
    // the fixed bracket that carries the station number
    box(st, 0.2, 0.12, 0.03, M.darkPlastic, base.x, 1.58, base.z + 0.03);
    const plate = plane(st, 0.16, 0.09, new THREE.MeshBasicMaterial({ map: T.labelCard(['B-12'], { w: 128, h: 72, bg: '#e8e1cb', size: 40 }), color: 0x9a9483 }), base.x, 1.58, base.z + 0.046);
    plate.name = 'b12plate';
    // the movable vane
    this.vane = new THREE.Group();
    this.vane.position.copy(YARD.vane);
    const board = floodlit(new THREE.MeshStandardMaterial({ map: T.vaneFace(), roughness: 0.6 }), 0.02);
    const back = floodlit(new THREE.MeshStandardMaterial({ color: 0x1b1e20, roughness: 0.7 }));
    faced(this.vane, VANE_W, VANE_H, 0.03, back, board, 0, 0, 0, '+z');
    mergeStatic(this.vane);
    this.vaneStripe = new THREE.Mesh(new THREE.PlaneGeometry(VANE_W * 0.3, VANE_H * 0.9), new THREE.MeshBasicMaterial({ visible: false }));
    this.vaneStripe.position.set(-VANE_W * 0.13, 0, 0.02);
    this.vane.add(this.vaneStripe);
    this.group.add(this.vane);
    this.objs.vane = this.vane;
    // the film-only echo: same stripe, rest alignment, 0.45 m to the side
    this.echo = new THREE.Mesh(new THREE.PlaneGeometry(VANE_W * 0.9, VANE_H * 0.98), new THREE.MeshBasicMaterial({ map: T.echoFace(), transparent: true, depthWrite: false, color: 0xd6cfb4, toneMapped: false }));
    this.echo.position.set(YARD.vane.x + YARD.echoOffset, YARD.vane.y, YARD.vane.z + 0.012);
    this.echo.visible = false;
    this.group.add(this.echo);
    this.col(base.x - 0.5, base.x + 0.5, base.z - 0.5, base.z + 0.5);

    // work lamp on its own pole, aimed at the vane
    const lp = new THREE.Vector3(base.x - 0.9, 0, base.z + 0.45);
    cyl(st, 0.03, 0.03, 2.9, M.pole, lp.x, 1.45, lp.z, 6);
    rod(st, lp.clone().setY(2.9), new THREE.Vector3(lp.x + 0.35, 2.95, lp.z + 0.15), 0.02, M.pole, 5);
    const head = box(st, 0.22, 0.14, 0.18, M.pole, lp.x + 0.4, 2.9, lp.z + 0.17);
    head.rotation.set(0.3, -0.6, -0.5);
    this.workLamp = addFlood(lp.x + 0.45, 2.75, lp.z + 0.1, this.workLampBase, 0xffb46a);
    this.workGlow = this.glow.add(lp.x + 0.42, 2.82, lp.z + 0.2, 0.7, 0xffc985);
    this.col(lp.x - 0.1, lp.x + 0.1, lp.z - 0.1, lp.z + 0.1);

    // B-12 local control cabinet on the west side of the walk
    const cab = new THREE.Group();
    cab.position.set(8.35, 0, -16.9);
    cab.rotation.y = Math.PI / 2; // faces east, onto the walk
    const steel = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('cabinet'), color: 0xa6b3af, roughness: 0.55, metalness: 0.45 }));
    const face = floodlit(new THREE.MeshStandardMaterial({ map: T.cabinetFace('B-12', ['SHIELD / DRIVE', 'REFERENCE 042'], 9), roughness: 0.6, metalness: 0.3 }));
    faced(cab, 0.6, 1.25, 0.36, steel, face, 0, 0.625, 0, '+z');
    box(cab, 0.66, 0.06, 0.4, steel, 0, 1.28, 0);
    mergeStatic(cab);
    this.group.add(cab);
    this.objs.b12Control = cab;
    this.occluders.push(cab);
    this.col(8.05, 8.6, -17.25, -16.55);
  }

  setVane(yawDeg: number) { this.vane.rotation.y = yawDeg * Math.PI / 180; }
  setWorkLamp(level: number) {
    setFlood(this.workLamp, this.workLampBase * level, flood);
    const col = (this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute);
    const c = new THREE.Color(0xffc985).multiplyScalar(Math.min(1.4, level));
    col.setXYZ(this.workGlow, c.r, c.g, c.b); col.needsUpdate = true;
  }

  // ---------- photo lab ----------
  private labBlock(st: THREE.Group, lab: THREE.Group) {
    const ext = M.concrete;
    const lf = this.labFlood;
    const FALL = 0.22;
    const lit = <T extends THREE.MeshStandardMaterial>(m: T) => floodlit(m, FALL, lf);
    const wall = lit(new THREE.MeshStandardMaterial({ color: 0xb4b8ad, map: T.wallPaint(), roughness: 0.92 }));
    const ceil = lit(new THREE.MeshStandardMaterial({ color: 0x5a5852, roughness: 1 }));
    const floor = lit(new THREE.MeshStandardMaterial({ color: 0x9f988b, map: T.concrete(31, [3, 3]), roughness: 0.75 }));
    const bench = lit(new THREE.MeshStandardMaterial({ color: 0x4a4e50, roughness: 0.5 }));
    const steelL = lit(new THREE.MeshStandardMaterial({ color: 0x8a8f92, roughness: 0.35, metalness: 0.7 }));
    const paperL = lit(new THREE.MeshStandardMaterial({ color: 0xd8d0b8, roughness: 0.9 }));
    const tray = lit(new THREE.MeshStandardMaterial({ color: 0x9a3a2c, roughness: 0.4 }));
    const plastic = lit(new THREE.MeshStandardMaterial({ color: 0x1b1d1f, roughness: 0.6 }));
    const card = (lines: string[], o: Parameters<typeof T.labelCard>[1]) => lit(new THREE.MeshStandardMaterial({ map: T.labelCard(lines, o), roughness: 0.9 }));

    // shell: exterior faces concrete, interior faces painted block. Door on the west wall.
    const X0 = 12.6, X1 = 19.2, Z0 = -6.0, Z1 = 1.0, H = 3.0;
    box(st, X1 - X0, 0.66, Z1 - Z0, M.concrete, (X0 + X1) / 2, -0.33, (Z0 + Z1) / 2);
    faced(lab, 0.2, H, -1.7 - Z0, ext, wall, X0 + 0.1, H / 2, (Z0 - 1.7) / 2, '+x');
    faced(lab, 0.2, H, Z1 + 0.7, ext, wall, X0 + 0.1, H / 2, (Z1 - 0.7) / 2, '+x');
    faced(lab, 0.2, H - 2.1, 1.0, ext, wall, X0 + 0.1, 2.1 + (H - 2.1) / 2, -1.2, '+x');
    faced(lab, 0.2, H, Z1 - Z0, ext, wall, X1 - 0.1, H / 2, (Z0 + Z1) / 2, '-x');
    faced(lab, X1 - X0, H, 0.2, ext, wall, (X0 + X1) / 2, H / 2, Z0 + 0.1, '+z');
    faced(lab, X1 - X0, H, 0.2, ext, wall, (X0 + X1) / 2, H / 2, Z1 - 0.1, '-z');
    faced(lab, X1 - X0 + 0.3, 0.25, Z1 - Z0 + 0.3, ext, ceil, (X0 + X1) / 2, H - 0.12, (Z0 + Z1) / 2, '-y');
    plane(lab, X1 - X0 - 0.4, Z1 - Z0 - 0.4, floor, (X0 + X1) / 2, 0.002, (Z0 + Z1) / 2, 0, -Math.PI / 2);
    this.addZone('labDoor', 12.0, 13.6, -1.7, -0.7, false);
    this.addZone('lab', 12.8, 19.0, -5.8, 0.8);

    // door: frame and a panel hinged on the south jamb, swings into the lab
    box(st, 0.24, 0.07, 1.06, M.frame, X0 + 0.1, 2.135, -1.2);
    this.labDoorHinge = new THREE.Group();
    this.labDoorHinge.position.set(X0 + 0.2, 0, -0.73);
    box(this.labDoorHinge, 0.045, 2.06, 0.93, new THREE.MeshStandardMaterial({ color: 0x3e4447, roughness: 0.55, metalness: 0.35 }), 0, 1.03, -0.465);
    box(this.labDoorHinge, 0.06, 0.04, 0.12, M.steel, -0.04, 1.0, -0.82);
    mergeStatic(this.labDoorHinge);
    this.group.add(this.labDoorHinge);
    const sign = plane(st, 0.78, 0.36, new THREE.MeshStandardMaterial({ map: T.labelCard(['PHOTOGRAPHIC LAB', 'PROCESS / EXAMINE / FILE'], { w: 256, h: 120, size: 26 }), roughness: 0.8 }), X0 - 0.01, 2.45, -1.2, -Math.PI / 2);
    sign.name = 'labSign';
    box(st, 0.3, 0.2, 0.2, M.steel, X0 - 0.12, 2.75, -2.0);
    addFlood(X0 - 0.4, 2.6, -2.0, 6, 0xffbf72);
    this.glow.add(X0 - 0.2, 2.65, -2.0, 1.2, 0xffc47a);
    this.objs.labDoor = this.labDoorHinge;

    // wet bench along the north wall: sink, three trays, tank, bottles, safelight above
    box(lab, 4.2, 0.08, 0.7, bench, 15.2, 0.9, -5.45);
    box(lab, 4.2, 0.82, 0.62, plastic, 15.2, 0.43, -5.45);
    this.col(13.0, 17.4, -5.85, -5.0);
    const trays = [['DEV', 14.0], ['STOP', 14.7], ['FIX', 15.4]] as const;
    for (const [, x] of trays) { box(lab, 0.5, 0.06, 0.4, tray, x, 0.97, -5.4); box(lab, 0.44, 0.01, 0.34, plastic, x, 0.995, -5.4); }
    cyl(lab, 0.08, 0.08, 0.2, plastic, 16.3, 1.04, -5.5, 14);   // developing tank
    cyl(lab, 0.085, 0.085, 0.03, steelL, 16.3, 1.15, -5.5, 14);
    box(lab, 0.16, 0.12, 0.06, plastic, 16.85, 1.0, -5.55);     // timer
    for (let i = 0; i < 4; i++) cyl(lab, 0.05, 0.05, 0.22, i % 2 ? plastic : paperL, 13.4 + i * 0.13, 1.05, -5.7, 10);
    const proc = plane(lab, 0.5, 0.36, card(['PROCESSING', '', '1  LOAD', '2  TRANSFER', '3  COLLECT'], { w: 200, h: 150, font: 'VT323', size: 24, surface: 'paper' }), 16.3, 1.6, -5.79);
    proc.name = 'processCard';
    plane(lab, 0.7, 0.24, card(['01 / WET PROCESS', 'DEVELOP  -  STOP  -  FIX'], { w: 256, h: 88, size: 26 }), 14.7, 1.85, -5.79);
    plane(lab, 0.62, 0.24, card(['SAFELIGHT AREA', 'KEEP EXPOSED FILM CLOSED'], { w: 256, h: 96, size: 22, bg: '#3b0d0a', fg: '#ff9a84' }), 13.5, 2.2, -5.79);
    for (const [name, x] of trays) plane(lab, 0.18, 0.05, card([name], { w: 96, h: 28, font: 'VT323', size: 24, surface: 'paper' }), x, 0.97, -5.195);
    // safelight
    box(lab, 0.34, 0.16, 0.18, plastic, 15.0, 2.45, -5.65);
    box(lab, 0.3, 0.12, 0.01, new THREE.MeshBasicMaterial({ color: 0xff2a14 }), 15.0, 2.43, -5.555);
    this.safelight = addFlood(15.0, 2.2, -5.2, 9, 0xff2616, lf);
    this.glow.add(15.0, 2.43, -5.5, 0.9, 0xff3a1f);
    // a second safelight in the ceiling, so the room reads without white light
    box(lab, 0.26, 0.1, 0.26, plastic, 15.9, 2.7, -2.4);
    box(lab, 0.22, 0.01, 0.22, new THREE.MeshBasicMaterial({ color: 0xff2a14 }), 15.9, 2.645, -2.4);
    addFlood(15.9, 2.35, -2.4, 6, 0xff2a18, lf);
    this.glow.add(15.9, 2.62, -2.4, 0.7, 0xff3a1f);
    // the print in the fixer tray (shows the real photograph while it develops)
    this.wetPrint = new THREE.Mesh(new THREE.PlaneGeometry(0.4, 0.25), lit(new THREE.MeshStandardMaterial({ color: 0x000000, roughness: 0.4 })));
    this.wetPrint.rotation.x = -Math.PI / 2; this.wetPrint.position.set(15.4, 1.003, -5.4);
    this.wetPrint.visible = false;
    this.group.add(this.wetPrint);
    this.proxy('wetBench', 3.4, 0.55, 0.75, 15.1, 1.15, -5.45);

    // enlarger on a stand at the east end of the bench
    box(lab, 0.5, 0.04, 0.5, bench, 17.0, 0.96, -5.4);
    cyl(lab, 0.03, 0.03, 0.9, steelL, 17.0, 1.4, -5.6, 8);
    box(lab, 0.22, 0.26, 0.24, plastic, 17.0, 1.75, -5.47);
    cyl(lab, 0.05, 0.06, 0.12, plastic, 17.0, 1.56, -5.45, 12);

    // drying line across the room, with clips; developed prints hang here (frames 01 to 04)
    rod(lab, new THREE.Vector3(13.0, 2.15, -3.0), new THREE.Vector3(18.9, 2.15, -3.0), 0.004, M.darkPlastic, 4);
    for (let i = 0; i < 4; i++) {
      const p = new THREE.Mesh(new THREE.PlaneGeometry(0.36, 0.24), lit(new THREE.MeshStandardMaterial({ color: 0xffffff, roughness: 0.6, side: THREE.DoubleSide })));
      p.position.set(15.0 + i * 0.6, 1.99, -3.0);
      p.visible = false;
      this.group.add(p);
      this.dryPrints.push(p);
      box(lab, 0.02, 0.05, 0.01, M.steel, 15.0 + i * 0.6, 2.13, -3.0);
    }

    // archive bench along the east wall, with the reference sheet and a task lamp
    box(lab, 0.8, 0.06, 3.2, bench, 18.6, 0.86, -3.0);
    box(lab, 0.06, 0.82, 3.0, plastic, 18.95, 0.43, -3.0);
    this.col(18.15, 19.05, -4.65, -1.35);
    const sheet = plane(lab, 0.3, 0.4, card(['INSTALLATION SHEET 11-86', '', 'B-12  ONE STRIPE', 'R-07  THREE BARS', '', 'SIGHTLINE:', 'S-03 APRON > B-12 >', 'CENTRAL ANTENNA'], { w: 200, h: 260, font: 'Special Elite', size: 15, surface: 'paper' }), 18.55, 0.895, -3.2, -Math.PI / 2, -Math.PI / 2);
    sheet.name = 'referenceSheet';
    const loupe = cyl(lab, 0.04, 0.05, 0.05, plastic, 18.5, 0.92, -2.6, 12);
    loupe.name = 'loupe';
    plane(lab, 0.7, 0.26, card(['02 / EXAMINE', 'REFERENCE / PHOTOGRAPHS'], { w: 256, h: 96, size: 24 }), 18.99, 1.7, -3.0, -Math.PI / 2);
    const tl = new THREE.Group(); tl.position.set(18.75, 0.89, -3.9);
    cyl(tl, 0.07, 0.08, 0.03, plastic, 0, 0.015, 0, 12);
    rod(tl, new THREE.Vector3(0, 0.03, 0), new THREE.Vector3(-0.12, 0.42, 0.1), 0.008, steelL, 5);
    const shade = cyl(tl, 0.03, 0.1, 0.12, plastic, -0.18, 0.42, 0.14, 12); shade.rotation.z = 0.7;
    lab.add(tl);
    addFlood(18.45, 1.3, -3.5, 5, 0xffb064, lf);
    this.glow.add(18.6, 1.3, -3.75, 0.35, 0xffc98a);
    this.proxy('archive', 0.8, 0.5, 3.2, 18.6, 1.1, -3.0);

    // records desk along the south wall, with the night report clipboard
    box(lab, 2.6, 0.06, 0.7, bench, 17.0, 0.76, 0.42);
    box(lab, 0.06, 0.73, 0.66, plastic, 15.73, 0.37, 0.42);
    box(lab, 0.06, 0.73, 0.66, plastic, 18.27, 0.37, 0.42);
    this.col(15.65, 18.35, 0.05, 0.85);
    const report = clipboard(T.labelCard(['SARO / NIGHT REPORT', '', 'OBSERVATION', 'METHOD', 'CONCLUSION'], { w: 160, h: 210, font: 'Special Elite', size: 15, border: false }), lit);
    report.position.set(17.2, 0.79, 0.35); report.rotation.y = 0.15;
    this.group.add(report);
    this.objs.records = report;
    addFlood(17.3, 1.4, 0.1, 3.5, 0xffd2a0, lf);
    plane(lab, 1.0, 0.75, lit(new THREE.MeshStandardMaterial({ map: T.fieldMap(), roughness: 0.85 })), 17.0, 1.75, 0.79, Math.PI);
    // shelves with film and chemistry by the door
    for (const y of [0.5, 1.1, 1.7]) box(lab, 0.35, 0.03, 2.4, bench, 13.0, y, -4.3);
    box(lab, 0.03, 1.8, 2.4, plastic, 12.83, 0.9, -4.3);
    for (let i = 0; i < 9; i++) box(lab, 0.12, 0.16, 0.1, i % 3 ? plastic : paperL, 13.0, 0.6 + Math.floor(i / 3) * 0.6, -5.2 + (i % 3) * 0.6);
    this.col(12.8, 13.25, -5.55, -3.05);
    this.occluders.push(lab);
  }

  // ---------- signs on the walk ----------
  private signs(st: THREE.Group) {
    const yard = plane(st, 1.0, 0.5, new THREE.MeshStandardMaterial({ map: T.yardSign(), roughness: 0.7 }), 8.05, 0.9, -3.4, Math.PI / 2);
    yard.name = 'yardSign';
    const route = plane(st, 0.6, 0.34, floodlit(new THREE.MeshStandardMaterial({ map: T.labelCard(['EAST WALK', 'B-12: NORTH OF S-03'], { w: 200, h: 112, size: 22 }), roughness: 0.8 })), 10.98, 0.85, -2.6, -Math.PI / 2);
    route.name = 'routeSign';
    const ref = plane(st, 0.5, 0.3, floodlit(new THREE.MeshStandardMaterial({ map: T.labelCard(['B-12', 'OPTICAL REFERENCE'], { w: 180, h: 108, size: 24 }), roughness: 0.8 })), 8.05, 0.85, -15.6, Math.PI / 2);
    ref.name = 'b12Sign';
    // R-07: three horizontal bars on a post by the west fence, off the sight line
    cyl(st, 0.03, 0.03, 1.6, M.pole, 6.6, 0.18, -19.4, 6);
    plane(st, 0.42, 0.42, floodlit(new THREE.MeshStandardMaterial({ map: T.barsBoard(), roughness: 0.6 })), 6.6, 1.15, -19.36, 0.5);
    this.proxy('r07', 0.5, 0.5, 0.12, 6.6, 1.15, -19.36);
  }

  // ---------- lived-in details along the walk ----------
  // Nothing here is interactive. It is what a service yard collects over the years.
  private details(st: THREE.Group) {
    const G = -0.62; // desert floor beside the raised walk
    const lit = (o: THREE.MeshStandardMaterialParameters) => floodlit(new THREE.MeshStandardMaterial(o));
    const unit = lit({ color: 0x9a9480, roughness: 0.7, metalness: 0.3 });
    const dark = lit({ color: 0x222426, roughness: 0.6, metalness: 0.4 });
    const grey = lit({ color: 0x6c7270, roughness: 0.5, metalness: 0.5 });
    const copper = lit({ color: 0x8a5a3a, roughness: 0.45, metalness: 0.6 });
    const red = lit({ color: 0xa3241b, roughness: 0.45, metalness: 0.2 });
    const card = (lines: string[], o: Parameters<typeof T.labelCard>[1]) => lit({ map: T.labelCard(lines, o), roughness: 0.85 });
    const v = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);

    // air conditioning condenser for the control room, on its own pad below the walk
    box(st, 1.2, 0.1, 1.2, M.concrete, 7.15, G + 0.05, -2.6);
    box(st, 0.95, 0.85, 0.95, unit, 7.15, G + 0.525, -2.6);
    cyl(st, 0.38, 0.38, 0.02, dark, 7.15, G + 0.96, -2.6, 18);
    cyl(st, 0.07, 0.07, 0.05, unit, 7.15, G + 0.98, -2.6, 8);
    for (const a of [0, Math.PI / 2]) {
      const d = new THREE.Vector3(Math.cos(a), 0, Math.sin(a)).multiplyScalar(0.37);
      rod(st, v(7.15 - d.x, G + 0.975, -2.6 - d.z), v(7.15 + d.x, G + 0.975, -2.6 + d.z), 0.008, grey, 4);
    }
    box(st, 0.02, 0.62, 0.78, dark, 7.63, G + 0.5, -2.6);           // louvre side towards the walk
    rod(st, v(6.67, G + 0.3, -2.8), v(6.32, G + 0.3, -2.8), 0.018, copper, 6);
    rod(st, v(6.67, G + 0.42, -2.72), v(6.32, G + 0.42, -2.72), 0.012, copper, 6);
    rod(st, v(6.32, G + 0.3, -2.8), v(6.32, 1.4, -2.8), 0.018, copper, 6);
    rod(st, v(6.32, G + 0.42, -2.72), v(6.32, 1.4, -2.72), 0.012, copper, 6);
    box(st, 0.08, 0.14, 0.2, grey, 6.34, 1.45, -2.76);

    // conduit along the east wall to a disconnect box by the landing, and a vent louvre
    rod(st, v(6.34, 2.85, -4.5), v(6.34, 2.85, 3.7), 0.025, grey, 6);
    rod(st, v(6.34, 2.85, 3.7), v(6.34, 1.75, 3.7), 0.025, grey, 6);
    rod(st, v(6.34, 1.15, 3.7), v(6.34, 0, 3.7), 0.025, grey, 6);
    for (let z = -4.2; z < 3.6; z += 1.3) box(st, 0.05, 0.06, 0.04, grey, 6.33, 2.85, z);
    box(st, 0.2, 0.55, 0.4, grey, 6.4, 1.45, 3.7);
    box(st, 0.04, 0.12, 0.05, dark, 6.52, 1.5, 3.86);                 // handle
    plane(st, 0.3, 0.12, card(['DISC. 480 V', 'YARD LIGHTING'], { w: 160, h: 64, size: 18 }), 6.505, 1.62, 3.66, Math.PI / 2);
    box(st, 0.04, 0.5, 0.7, dark, 6.32, 2.35, -1.4);
    for (let k = 0; k < 6; k++) box(st, 0.05, 0.02, 0.66, grey, 6.34, 2.14 + k * 0.08, -1.4);

    // fire extinguisher on a bracket south of the door
    cyl(st, 0.08, 0.08, 0.46, red, 6.42, 0.98, 2.55, 12);
    cyl(st, 0.03, 0.05, 0.08, dark, 6.42, 1.25, 2.55, 8);
    box(st, 0.04, 0.06, 0.2, grey, 6.33, 1.1, 2.55);
    plane(st, 0.22, 0.12, card(['FIRE', 'EXTINGUISHER'], { w: 128, h: 64, size: 18, bg: '#7a1610', fg: '#f3e6d0' }), 6.31, 1.48, 2.55, Math.PI / 2);

    // a folding chair and a coffee can by the wall: someone sits out here on breaks
    const chair = new THREE.Group();
    chair.position.set(6.78, 0, 3.0); chair.rotation.y = 0.5;
    box(chair, 0.42, 0.03, 0.4, grey, 0, 0.45, 0);
    const back = box(chair, 0.42, 0.3, 0.03, grey, 0, 0.75, -0.2); back.rotation.x = -0.12;
    rod(chair, v(-0.19, 0, 0.2), v(-0.19, 0.9, -0.24), 0.012, dark, 4);
    rod(chair, v(0.19, 0, 0.2), v(0.19, 0.9, -0.24), 0.012, dark, 4);
    rod(chair, v(-0.19, 0, -0.22), v(-0.19, 0.45, 0.16), 0.012, dark, 4);
    rod(chair, v(0.19, 0, -0.22), v(0.19, 0.45, 0.16), 0.012, dark, 4);
    st.add(chair);
    this.col(6.45, 7.12, 2.7, 3.3);
    cyl(st, 0.065, 0.065, 0.14, copper, 7.25, 0.07, 3.15, 10);
    cyl(st, 0.06, 0.06, 0.005, dark, 7.25, 0.142, 3.15, 10);

    // silver recovery drum outside the photo lab: fixer is not poured down the drain
    cyl(st, 0.29, 0.29, 0.88, lit({ color: 0x2f4f5c, roughness: 0.55, metalness: 0.4 }), 11.95, G + 0.44, -4.2, 16);
    for (const y of [0.3, 0.6]) cyl(st, 0.3, 0.3, 0.025, dark, 11.95, G + y, -4.2, 16);
    plane(st, 0.34, 0.22, card(['FIXER WASTE', 'SILVER RECOVERY', 'DO NOT DUMP'], { w: 160, h: 104, size: 17 }), 11.655, G + 0.55, -4.2, -Math.PI / 2);
    rod(st, v(12.25, G + 0.82, -4.2), v(12.58, 0.9, -4.2), 0.015, dark, 5);   // hose from the lab

    // the darkroom warning lamp beside the lab door (lit: the safelight is on)
    box(st, 0.1, 0.14, 0.14, dark, 12.55, 2.3, -0.42);
    box(st, 0.01, 0.1, 0.1, new THREE.MeshBasicMaterial({ color: 0xff2a14 }), 12.495, 2.3, -0.42);
    this.glow.add(12.47, 2.3, -0.42, 0.5, 0xff3a1f);
    plane(st, 0.26, 0.1, card(['DARKROOM', 'IN USE'], { w: 128, h: 50, size: 18, bg: '#3b0d0a', fg: '#ff9a84' }), 12.585, 2.1, -0.42, -Math.PI / 2);

    // checker plate over the cable trench in front of S-03
    box(st, 0.98, 0.014, 0.42, lit({ color: 0x77746c, roughness: 0.5, metalness: 0.6, map: T.concrete(77, [4, 1]) }), 11.5, 0.007, -11.0);
  }

  // Drawn on the motor bus inspection glass.
  drawBus(az: number, commands: number, warn: boolean, t: number) {
    const g = this.bus.ctx, w = 256, h = 160;
    g.fillStyle = '#0b0d08'; g.fillRect(0, 0, w, h);
    g.font = '22px VT323, monospace'; g.fillStyle = '#ffb052';
    g.fillText('MOTOR BUS S-03', 12, 26);
    g.font = '18px VT323, monospace'; g.fillText('AZIMUTH', 12, 58);
    g.font = '52px VT323, monospace'; g.fillText(`${String(Math.round(az)).padStart(3, '0')}°`, 12, 104);
    g.font = '18px VT323, monospace';
    g.fillText(`SCHED 042   CMD ${commands}`, 120, 58);
    if (!warn || Math.floor(t * 2) % 2 === 0) { g.fillStyle = '#ffd27a'; g.fillText('NO CONTROL COMMAND', 12, 134); g.fillText('RECORDED', 12, 152); }
    this.bus.tex.needsUpdate = true;
  }

  update(t: number) { this.glow.update(t); }
}
