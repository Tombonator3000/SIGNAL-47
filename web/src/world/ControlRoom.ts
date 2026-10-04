import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit, faced, annexFlood } from './kit';
import * as T from '../core/textures';
import { fieldCameraModel } from './props';

export type Collider = { minX: number; maxX: number; minZ: number; maxZ: number };

export class Crt {
  canvas = document.createElement('canvas');
  ctx: CanvasRenderingContext2D;
  tex: THREE.CanvasTexture;
  mat: THREE.MeshBasicMaterial;
  group = new THREE.Group();
  powered = false;
  constructor() {
    this.canvas.width = 512; this.canvas.height = 384;
    this.ctx = this.canvas.getContext('2d')!;
    this.ctx.fillStyle = '#020a04'; this.ctx.fillRect(0, 0, 512, 384);
    this.tex = new THREE.CanvasTexture(this.canvas);
    this.tex.colorSpace = THREE.SRGBColorSpace;
    this.mat = new THREE.MeshBasicMaterial({ map: this.tex, color: 0x0d1410, toneMapped: false });
  }
  setPowered(on: boolean) { this.powered = on; this.mat.color.set(on ? 0xffffff : 0x0d1410); }
  commit() { this.tex.needsUpdate = true; }
}

const blobMat = new THREE.MeshBasicMaterial({ map: T.blobShadow(), transparent: true, depthWrite: false });
// Soft contact shadows. They are collected in one group and merged into a single mesh.
function blob(parent: THREE.Object3D, x: number, z: number, w: number, d: number, y = 0.003) {
  return plane(parent, w, d, blobMat, x, y, z, 0, -Math.PI / 2);
}

// Rack status lights: one instanced mesh, coloured per light.
export type LedState = 'off' | 'green' | 'amber';
const LED_COL: Record<LedState, THREE.Color> = { off: new THREE.Color(0x141619), green: new THREE.Color(0x45ff7a), amber: new THREE.Color(0xffb040) };

export class ControlRoom {
  group = new THREE.Group();
  colliders: Collider[] = [];
  crtLeft = new Crt(); crtCenter = new Crt(); crtRight = new Crt();
  crtOps = new Crt();   // the operations terminal on the west desk (story/nightshift.ts)
  lights: { ceiling: THREE.PointLight[]; lamps: THREE.PointLight[]; crt: THREE.PointLight; hemi: THREE.HemisphereLight; moon: THREE.DirectionalLight };
  tubes: THREE.Mesh[] = [];
  objs: Record<string, THREE.Object3D> = {};
  mug!: THREE.Group; coffee!: THREE.Mesh; mugHome = new THREE.Vector3();
  printerPaper!: THREE.Mesh;
  clockHands!: { h: THREE.Object3D; m: THREE.Object3D; s: THREE.Object3D };
  leds!: THREE.InstancedMesh;
  ledCount = 18;
  private blobs = new THREE.Group();
  lever!: THREE.Object3D;
  handset!: THREE.Object3D;
  doorHinge!: THREE.Object3D;
  southDoorHinge!: THREE.Object3D;
  doorLight!: THREE.Mesh;
  fieldCamera!: THREE.Group;
  dspScreen!: { canvas: HTMLCanvasElement; ctx: CanvasRenderingContext2D; tex: THREE.CanvasTexture };
  reels: THREE.Object3D[] = [];
  spawn = { x: 1.55, z: 3.7, yaw: -0.22 };
  bounds = { minX: -5.75, maxX: 5.75, minZ: -4.22, maxZ: 4.22 };

  constructor() {
    const st = new THREE.Group(); // static, gets merged
    this.group.add(st);
    this.shell(st);
    this.consoleDesk(st);
    this.supervisorDesk(st);
    this.eastWall(st);
    this.westWall(st);
    this.southWall(st);
    this.nightShift(st);
    mergeStatic(st);
    this.group.add(this.blobs);
    mergeStatic(this.blobs);

    const hemi = new THREE.HemisphereLight(0x22304f, 0x2a1b10, 0.9);
    const moon = new THREE.DirectionalLight(0x8ea4d8, 0.45);
    moon.position.set(-40, 80, -60);
    const ceiling = [new THREE.PointLight(0xffe2b8, 9, 13, 1.6), new THREE.PointLight(0xffe2b8, 7, 13, 1.6)];
    ceiling[0].position.set(-1.8, 2.75, -1.6); ceiling[1].position.set(3.2, 2.75, 1.8);
    const lamps = [new THREE.PointLight(0xffa85a, 3.2, 5, 1.8), new THREE.PointLight(0xffa85a, 2.4, 5, 1.8)];
    lamps[0].position.set(3.38, 1.18, 2.62); lamps[1].position.set(-5.3, 1.18, 2.95);
    const crt = new THREE.PointLight(0x5cff8a, 0.0, 3.6, 2);
    crt.position.set(0, 1.15, -3.3);
    [hemi, moon, ...ceiling, ...lamps, crt].forEach((l) => this.group.add(l));
    this.lights = { ceiling, lamps, crt, hemi, moon };
  }

  private col(minX: number, maxX: number, minZ: number, maxZ: number) { this.colliders.push({ minX, maxX, minZ, maxZ }); }

  // ---------- Room shell ----------
  private shell(st: THREE.Group) {
    const wallTex = T.wallPaint();
    const wall = new THREE.MeshStandardMaterial({ color: 0xb3c2c4, map: wallTex, roughness: 0.92 });
    const wallLow = new THREE.MeshStandardMaterial({ color: 0x566b6e, map: wallTex, roughness: 0.9 });
    const ext = M.concrete;
    const floorMat = new THREE.MeshStandardMaterial({ map: T.hexFloor(), roughness: 0.55, metalness: 0.0, color: 0xdfd3bf });
    const ceilMat = new THREE.MeshStandardMaterial({ map: T.ceilingTiles(), roughness: 1, color: 0xe2dfd3 });
    const floor = plane(this.group, 12, 9, floorMat, 0, 0, 0, 0, -Math.PI / 2);
    noMerge(floor);
    plane(st, 12, 9, ceilMat, 0, 3.0, 0, 0, Math.PI / 2);

    // box faces: [+x, -x, +y, -y, +z, -z]
    const north = (w: number, h: number, x: number, y: number) => faced(st, w, h, 0.3, ext, wall, x, y, -4.65, '+z');
    north(12.6, 0.95, 0, 0.475);
    north(12.6, 0.45, 0, 2.975);
    north(0.9, 1.8, -5.85, 1.85);
    north(0.9, 1.8, 5.85, 1.85);
    // south wall, with the corridor door opening (x -2.9 to -1.9)
    faced(st, 3.4, 3.2, 0.3, ext, wall, -4.6, 1.6, 4.65, '-z');
    faced(st, 8.2, 3.2, 0.3, ext, wall, 2.2, 1.6, 4.65, '-z');
    faced(st, 1.0, 1.08, 0.3, ext, wall, -2.4, 2.66, 4.65, '-z');
    faced(st, 0.3, 3.2, 9.6, ext, wall, -6.15, 1.6, 0, '+x'); // west
    // east wall, with the opening for the service yard door (z 1.1 to 2.1)
    faced(st, 0.3, 3.2, 5.9, ext, wall, 6.15, 1.6, -1.85, '-x');
    faced(st, 0.3, 3.2, 2.7, ext, wall, 6.15, 1.6, 3.45, '-x');
    faced(st, 0.3, 1.08, 1.0, ext, wall, 6.15, 2.66, 1.6, '-x');
    // dado band and skirting
    box(st, 3.04, 0.95, 0.02, wallLow, -4.47, 0.475, 4.49);
    box(st, 7.84, 0.95, 0.02, wallLow, 2.07, 0.475, 4.49);
    box(st, 0.02, 0.95, 8.98, wallLow, -5.99, 0.475, 0);
    box(st, 0.02, 0.95, 5.59, wallLow, 5.99, 0.475, -1.695);
    box(st, 0.02, 0.95, 2.39, wallLow, 5.99, 0.475, 3.295);
    box(st, 3.04, 0.04, 0.04, M.frame, -4.47, 0.97, 4.47);
    box(st, 7.84, 0.04, 0.04, M.frame, 2.07, 0.97, 4.47);
    box(st, 0.04, 0.04, 8.98, M.frame, -5.97, 0.97, 0);
    box(st, 0.04, 0.04, 5.59, M.frame, 5.97, 0.97, -1.695);
    box(st, 0.04, 0.04, 2.39, M.frame, 5.97, 0.97, 3.295);
    // window frame: sill, head, mullions
    box(st, 10.9, 0.06, 0.42, M.frame, 0, 0.95, -4.38);
    box(st, 10.9, 0.1, 0.2, M.frame, 0, 2.75, -4.55);
    for (let k = 0; k <= 6; k++) box(st, 0.09, 1.8, 0.22, M.frame, -5.4 + k * 1.8, 1.85, -4.55);
    const glass = plane(this.group, 10.8, 1.8, new THREE.MeshBasicMaterial({ color: 0x9fbccc, transparent: true, opacity: 0.05, depthWrite: false }), 0, 1.85, -4.56);
    noMerge(glass);
    // ceiling fixtures
    const fx = [[-3.5, -1.8, 1], [0, -1.8, 1], [3.5, -1.8, 0], [-3.5, 1.8, 0], [0, 1.8, 1], [3.5, 1.8, 1]];
    for (const [x, z, on] of fx) {
      box(st, 1.3, 0.06, 0.36, M.steel, x, 2.97, z);
      const tube = box(this.group, 1.2, 0.03, 0.26, on ? M.emissiveTube : M.emissiveTubeOff, x, 2.935, z);
      noMerge(tube);
      if (on) this.tubes.push(tube);
    }
  }

  // ---------- Console desk under the window ----------
  private consoleDesk(st: THREE.Group) {
    box(st, 6.9, 0.05, 0.9, M.deskTop, 0, 0.755, -3.9);
    box(st, 6.9, 0.62, 0.04, M.deskBody, 0, 0.42, -4.3);
    for (const x of [-3.2, -1.1, 1.1, 3.2]) {
      box(st, 0.46, 0.7, 0.78, M.deskBody, x, 0.38, -3.92);
      for (let i = 0; i < 3; i++) box(st, 0.4, 0.015, 0.01, M.steel, x, 0.2 + i * 0.22, -3.525);
    }
    this.col(-3.5, 3.5, -4.5, -3.42);
    const crts: [Crt, number, string][] = [[this.crtLeft, -2.15, 'crtLeft'], [this.crtCenter, 0, 'crtCenter'], [this.crtRight, 2.15, 'crtRight']];
    for (const [crt, x, id] of crts) {
      const g = crt.group;
      g.position.set(x, 0.78, -4.0);
      box(g, 0.3, 0.04, 0.26, M.beigeDark, 0, 0.02, 0);
      box(g, 0.5, 0.42, 0.36, M.beige, 0, 0.26, 0);
      box(g, 0.38, 0.32, 0.26, M.beige, 0, 0.25, -0.28);
      box(g, 0.43, 0.33, 0.01, M.darkPlastic, 0, 0.27, 0.181);
      const screen = plane(g, 0.37, 0.28, crt.mat, 0, 0.27, 0.187);
      noMerge(screen);
      box(g, 0.06, 0.012, 0.01, M.ledGreen, 0.19, 0.075, 0.181);
      // keyboard
      const kb = new THREE.Group();
      kb.position.set(0, 0, 0.42);
      box(kb, 0.46, 0.03, 0.17, M.beige, 0, 0.015, 0);
      plane(kb, 0.44, 0.15, new THREE.MeshStandardMaterial({ map: T.keyboard(), roughness: 0.6 }), 0, 0.031, 0, 0, -Math.PI / 2);
      g.add(kb);
      mergeStatic(g);
      this.group.add(g);
      this.objs[id] = g;
      blob(this.blobs, x, -2.95, 0.9, 0.9);
      this.chair(x + (x === 0 ? 0.05 : -0.08), -2.95, x === 0 ? 0.08 : -0.15);
    }
    // clutter on the console desk
    box(st, 0.22, 0.02, 0.3, M.paper, -1.05, 0.79, -3.75).rotation.y = 0.2;
    box(st, 0.22, 0.02, 0.3, M.paper, 1.15, 0.79, -3.8).rotation.y = -0.15;
    cyl(st, 0.04, 0.035, 0.1, M.beigeDark, -2.75, 0.83, -3.65, 10);
    for (let i = 0; i < 4; i++) rod(st, new THREE.Vector3(-2.75 + (i - 1.5) * 0.01, 0.82, -3.65), new THREE.Vector3(-2.75 + (i - 1.5) * 0.025, 0.95, -3.64 + i * 0.008), 0.004, M.darkPlastic, 4);
    // receiver/radio units at the right end
    box(st, 0.42, 0.18, 0.3, M.darkPlastic, 2.95, 0.87, -4.1);
    box(st, 0.36, 0.08, 0.005, M.ledAmber, 2.95, 0.9, -3.948);
    box(st, 0.4, 0.15, 0.28, M.beigeDark, 2.95, 1.035, -4.12);
    // trash bin
    cyl(st, 0.17, 0.14, 0.36, M.darkPlastic, -3.75, 0.18, -3.7, 12);
    for (let i = 0; i < 3; i++) { const p = new THREE.Mesh(new THREE.IcosahedronGeometry(0.05, 0), M.paper); p.position.set(-3.75 + (i - 1) * 0.06, 0.34, -3.7 + (i % 2) * 0.05); st.add(p); }
    this.col(-3.95, -3.55, -3.9, -3.5);
  }

  private chair(x: number, z: number, yaw: number, parent: THREE.Object3D = this.group) {
    const g = new THREE.Group();
    g.position.set(x, 0, z); g.rotation.y = yaw;
    for (let k = 0; k < 5; k++) { const a = (k / 5) * Math.PI * 2; const s = box(g, 0.3, 0.03, 0.04, M.steel, Math.cos(a) * 0.15, 0.06, Math.sin(a) * 0.15); s.rotation.y = -a; }
    cyl(g, 0.025, 0.025, 0.38, M.steel, 0, 0.27, 0, 6);
    box(g, 0.5, 0.08, 0.48, M.chair, 0, 0.48, 0);
    box(g, 0.46, 0.5, 0.07, M.chair, 0, 0.8, 0.25).rotation.x = -0.12;
    box(g, 0.04, 0.2, 0.04, M.steel, 0, 0.56, 0.24);
    mergeStatic(g);
    parent.add(g);
    this.col(x - 0.28, x + 0.28, z - 0.28, z + 0.28);
    return g;
  }

  // ---------- Supervisor desk (the player's spot) ----------
  private supervisorDesk(st: THREE.Group) {
    box(st, 1.8, 0.05, 0.9, M.deskTop, 2.6, 0.755, 2.55);
    box(st, 0.46, 0.7, 0.84, M.deskBody, 3.22, 0.38, 2.55);
    box(st, 1.8, 0.55, 0.03, M.deskBody, 2.6, 0.45, 2.98);
    box(st, 0.04, 0.73, 0.84, M.deskBody, 1.72, 0.37, 2.55);
    this.col(1.68, 3.52, 2.08, 3.02);
    blob(this.blobs, 2.6, 2.55, 2.4, 1.4);
    this.chair(2.45, 3.3, 0.2);

    // NIGHT SHIFT logbook
    const coverMat = new THREE.MeshStandardMaterial({ map: T.logbookCover(), roughness: 0.85 });
    const bookSide = new THREE.MeshStandardMaterial({ color: 0x1b2433, roughness: 0.9 });
    const log = new THREE.Group();
    log.position.set(2.2, 0.796, 2.5);
    log.rotation.y = 0.18;
    faced(log, 0.24, 0.032, 0.31, bookSide, coverMat, 0, 0, 0, '+y');
    plane(log, 0.23, 0.026, M.paper, 0, 0, 0.156);
    this.group.add(log);
    this.objs.logbook = log;
    // the night work order, and a pen on it. The invisible box makes the thin sheet easy to aim at.
    const wo = new THREE.Group();
    wo.position.set(2.62, 0.782, 2.72); wo.rotation.y = -0.1;
    plane(wo, 0.21, 0.29, new THREE.MeshStandardMaterial({ map: T.workOrderSheet(), roughness: 0.9 }), 0, 0, 0, 0, -Math.PI / 2);
    const woHit = new THREE.Mesh(new THREE.BoxGeometry(0.24, 0.04, 0.31), new THREE.MeshBasicMaterial({ visible: false }));
    noMerge(woHit); wo.add(woHit);
    mergeStatic(wo);
    this.group.add(wo);
    this.objs.workOrder = wo;
    rod(st, new THREE.Vector3(2.55, 0.787, 2.62), new THREE.Vector3(2.68, 0.787, 2.58), 0.005, M.darkPlastic, 5);

    // telephone
    const ph = new THREE.Group();
    ph.position.set(3.02, 0.78, 2.36); ph.rotation.y = -0.35;
    box(ph, 0.2, 0.06, 0.22, M.beige, 0, 0.03, 0);
    const keys = box(ph, 0.12, 0.012, 0.1, M.beigeDark, 0, 0.066, 0.04);
    keys.rotation.x = -0.25;
    for (let r = 0; r < 4; r++) for (let c = 0; c < 3; c++) box(ph, 0.022, 0.01, 0.016, M.darkPlastic, (c - 1) * 0.034, 0.072 + r * 0.005, 0.075 - r * 0.022);
    const hs = new THREE.Group();
    hs.position.set(0, 0.085, -0.045);
    box(hs, 0.2, 0.035, 0.045, M.beige, 0, 0, 0);
    box(hs, 0.055, 0.05, 0.06, M.beige, -0.085, -0.015, 0);
    box(hs, 0.055, 0.05, 0.06, M.beige, 0.085, -0.015, 0);
    ph.add(hs);
    noMerge(hs);
    this.handset = hs;
    const pts: THREE.Vector3[] = [];
    for (let i = 0; i <= 80; i++) { const t = i / 80; const a = t * Math.PI * 28; pts.push(new THREE.Vector3(-0.1 - t * 0.1 + Math.cos(a) * 0.012, 0.03 + Math.sin(t * Math.PI) * -0.02 + Math.sin(a) * 0.012, -0.03 + t * 0.08)); }
    const cord = new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 160, 0.004, 4), M.beigeDark);
    ph.add(cord);
    mergeStatic(ph);
    mergeStatic(hs);
    this.group.add(ph);
    this.objs.phone = ph;

    // SARO mug
    const mug = new THREE.Group();
    mug.position.set(2.66, 0.78, 2.32);
    const mugMat = new THREE.MeshStandardMaterial({ map: T.mugLogo(), roughness: 0.35 });
    const body = new THREE.Mesh(new THREE.CylinderGeometry(0.042, 0.04, 0.1, 20, 1, true), [mugMat] as any);
    body.material = mugMat;
    body.position.y = 0.05; body.rotation.y = Math.PI * 0.6;
    mug.add(body);
    const inner = new THREE.Mesh(new THREE.CylinderGeometry(0.038, 0.036, 0.098, 20, 1, true), new THREE.MeshStandardMaterial({ color: 0xd9d3c6, side: THREE.BackSide, roughness: 0.4 }));
    inner.position.y = 0.051; mug.add(inner);
    cyl(mug, 0.04, 0.04, 0.006, M.ceramic, 0, 0.003, 0, 20);
    const handle = new THREE.Mesh(new THREE.TorusGeometry(0.026, 0.0075, 6, 12, Math.PI), M.ceramic);
    handle.position.set(0.045, 0.05, 0); handle.rotation.z = -Math.PI / 2;
    mug.add(handle);
    this.coffee = cyl(mug, 0.037, 0.037, 0.004, M.coffee, 0, 0.02, 0, 20);
    this.coffee.visible = false;
    noMerge(this.coffee);
    mergeStatic(mug);
    this.group.add(mug);
    this.mug = mug; this.mugHome.copy(mug.position);
    this.objs.mug = mug;

    // desk lamp
    const lamp = new THREE.Group();
    lamp.position.set(3.4, 0.78, 2.84);
    cyl(lamp, 0.08, 0.09, 0.03, M.lampShade, 0, 0.015, 0, 16);
    rod(lamp, new THREE.Vector3(0, 0.03, 0), new THREE.Vector3(-0.05, 0.36, -0.06), 0.008, M.steel);
    rod(lamp, new THREE.Vector3(-0.05, 0.36, -0.06), new THREE.Vector3(-0.02, 0.44, -0.24), 0.008, M.steel);
    const shade = cyl(lamp, 0.03, 0.11, 0.13, M.lampShade, -0.02, 0.41, -0.27, 16);
    (shade.geometry as THREE.CylinderGeometry).dispose();
    shade.geometry = new THREE.CylinderGeometry(0.03, 0.11, 0.13, 16, 1, true);
    shade.rotation.x = 0.55;
    const bulb = new THREE.Mesh(new THREE.SphereGeometry(0.03, 10, 8), M.lampBulb);
    bulb.position.set(-0.02, 0.39, -0.3);
    lamp.add(bulb);
    mergeStatic(lamp);
    this.group.add(lamp);

    // coffee corner
    box(st, 0.62, 0.92, 0.52, M.cabinet, 4.38, 0.46, 3.85);
    this.col(4.05, 4.71, 3.55, 4.5);
    const cm = new THREE.Group();
    cm.position.set(4.38, 0.92, 3.9);
    box(cm, 0.24, 0.34, 0.22, M.darkPlastic, 0, 0.17, 0.02);
    box(cm, 0.24, 0.05, 0.3, M.darkPlastic, 0, 0.36, -0.02);
    box(cm, 0.2, 0.012, 0.16, M.steel, 0, 0.012, -0.1);
    const carafe = cyl(cm, 0.065, 0.07, 0.15, new THREE.MeshStandardMaterial({ color: 0x223040, roughness: 0.1, transparent: true, opacity: 0.55 }), 0, 0.09, -0.11, 14);
    noMerge(carafe);
    const brew = cyl(cm, 0.06, 0.066, 0.07, M.coffee, 0, 0.05, -0.11, 14);
    noMerge(brew);
    box(cm, 0.03, 0.015, 0.01, M.ledRed, 0.07, 0.03, 0.135);
    carafe.userData.noMerge = false; brew.userData.noMerge = false;
    mergeStatic(cm);
    this.group.add(cm);
    this.objs.coffeePot = cm;
    const cups = new THREE.Group();
    cups.position.set(4.18, 0.92, 3.7);
    cyl(cups, 0.04, 0.038, 0.1, M.ceramic, 0, 0.05, 0, 14);
    cyl(cups, 0.04, 0.038, 0.1, M.ceramic, 0.09, 0.05, 0.04, 14);
    st.add(cups);
  }

  // ---------- East wall: rack, printer, clock, door ----------
  private eastWall(st: THREE.Group) {
    // RX bank 3 rack
    const rackTex = new THREE.MeshStandardMaterial({ map: T.rackFront(), roughness: 0.6 });
    faced(st, 0.62, 1.95, 0.62, M.cabinet, rackTex, 5.66, 0.975, -2.4, '-x');
    this.col(5.3, 6, -2.75, -2.05);
    blob(this.blobs, 5.5, -2.4, 1.0, 1.0);
    this.leds = new THREE.InstancedMesh(new THREE.BoxGeometry(0.005, 0.016, 0.016), new THREE.MeshBasicMaterial({ color: 0xffffff }), this.ledCount);
    const lm = new THREE.Matrix4();
    for (let i = 0; i < this.ledCount; i++) {
      this.leds.setMatrixAt(i, lm.makeTranslation(5.347, 1.55 - Math.floor(i / 6) * 0.22, -2.62 + (i % 6) * 0.068));
      this.leds.setColorAt(i, LED_COL.off);
    }
    noMerge(this.leds);
    this.group.add(this.leds);
    // power lever
    const lev = new THREE.Group();
    lev.position.set(5.34, 1.05, -2.24);
    box(lev, 0.03, 0.16, 0.12, M.darkPlastic, 0, 0, 0);
    const arm = new THREE.Group();
    arm.position.set(-0.03, 0, 0);
    box(arm, 0.025, 0.14, 0.025, M.steel, -0.02, 0.06, 0);
    box(arm, 0.05, 0.035, 0.06, new THREE.MeshStandardMaterial({ color: 0xb3261e, roughness: 0.5 }), -0.03, 0.13, 0);
    arm.rotation.z = -0.6;
    lev.add(arm);
    this.lever = arm;
    this.group.add(lev);
    this.objs.rack = lev;

    // the signal processor: a short rack between RX bank 3 and the printer, with the
    // processor unit (its display is drawn while it plays) and a reel-to-reel deck on top
    const dsp = new THREE.Group();
    dsp.position.set(5.72, 0, -1.55);
    box(dsp, 0.5, 0.95, 0.62, M.cabinet, 0, 0.475, 0);
    const front = T.canvasTex(512, 160, (g, w, h) => {
      g.fillStyle = '#26292b'; g.fillRect(0, 0, w, h);
      g.fillStyle = '#3a3e40'; g.fillRect(6, 6, w - 12, h - 12);
      g.fillStyle = '#d9d2bf'; g.font = '600 22px Oswald'; g.textBaseline = 'top';
      g.fillText('SIGNAL PROCESSOR', 22, 18);
      g.font = '15px Oswald'; g.fillStyle = '#9fa4a0'; g.fillText('DSP-4   HARMONIC / TIME', 22, 46);
      g.fillStyle = '#0b0f0d'; g.fillRect(260, 20, 220, 64); // the display goes here
      for (let i = 0; i < 6; i++) { g.fillStyle = i === 0 ? '#b3261e' : '#c9c1a8'; g.fillRect(24 + i * 38, 104, 26, 18); }
      g.fillStyle = '#9fa4a0'; g.font = '12px Oswald';
      ['PLAY', 'RAW', 'HARM', 'PITCH', 'TIME', 'SPACE'].forEach((l, i) => g.fillText(l, 24 + i * 38, 128));
    });
    plane(dsp, 0.6, 0.19, new THREE.MeshStandardMaterial({ map: front, roughness: 0.55 }), -0.252, 0.83, 0, -Math.PI / 2);
    const scr = document.createElement('canvas'); scr.width = 256; scr.height = 72;
    this.dspScreen = { canvas: scr, ctx: scr.getContext('2d')!, tex: new THREE.CanvasTexture(scr) };
    this.dspScreen.tex.colorSpace = THREE.SRGBColorSpace;
    const screen = plane(dsp, 0.255, 0.075, new THREE.MeshBasicMaterial({ map: this.dspScreen.tex, toneMapped: false }), -0.254, 0.848, -0.077, -Math.PI / 2);
    noMerge(screen);
    cyl(dsp, 0.035, 0.035, 0.03, M.darkPlastic, -0.262, 0.78, -0.2, 16).rotation.z = Math.PI / 2;
    // the deck: two reels on the front, a head block between them
    box(dsp, 0.3, 0.52, 0.6, M.beigeDark, 0.08, 1.21, 0);
    box(dsp, 0.02, 0.48, 0.56, M.darkPlastic, -0.075, 1.21, 0);
    box(dsp, 0.04, 0.06, 0.12, M.steel, -0.09, 1.05, 0);
    this.reels = [-0.15, 0.15].map((z) => {
      const reel = new THREE.Group();
      reel.position.set(-0.1, 1.29, z);
      const disc = cyl(reel, 0.115, 0.115, 0.012, M.steel, 0, 0, 0, 20); disc.rotation.z = Math.PI / 2;
      const tape = cyl(reel, 0.07, 0.07, 0.016, new THREE.MeshStandardMaterial({ color: 0x3a2a1c, roughness: 0.6 }), -0.004, 0, 0, 18); tape.rotation.z = Math.PI / 2;
      for (let k = 0; k < 3; k++) box(reel, 0.016, 0.02, 0.17, M.darkPlastic, -0.01, 0, 0).rotation.x = k * Math.PI / 3;
      noMerge(reel); dsp.add(reel);
      return reel;
    });
    this.col(5.4, 6, -1.92, -1.18);
    mergeStatic(dsp);
    this.group.add(dsp);
    this.objs.decoder = dsp;

    // printer table and dot matrix printer
    box(st, 0.72, 0.04, 1.15, M.deskTop, 5.6, 0.72, -0.4);
    for (const [dx, dz] of [[-0.3, -0.5], [0.3, -0.5], [-0.3, 0.5], [0.3, 0.5]]) cyl(st, 0.02, 0.02, 0.7, M.steel, 5.6 + dx, 0.35, -0.4 + dz, 6);
    box(st, 0.4, 0.22, 0.5, M.paper, 5.6, 0.12, -0.4); // fan-fold stack under the table
    this.col(5.2, 6, -1.0, 0.2);
    const pr = new THREE.Group();
    pr.position.set(5.62, 0.74, -0.4);
    pr.rotation.y = -Math.PI / 2; // front faces west
    box(pr, 0.52, 0.14, 0.36, M.beige, 0, 0.07, 0);
    box(pr, 0.5, 0.04, 0.12, M.beigeDark, 0, 0.15, -0.1);
    box(pr, 0.42, 0.012, 0.01, M.darkPlastic, 0, 0.11, 0.181);
    box(pr, 0.03, 0.01, 0.01, M.ledGreen, 0.2, 0.06, 0.181);
    this.group.add(pr);
    const lines = ['SARO RX/DSP   DIRECTION SOLVE', '------------------------------', 'FREQ    1420.405 MHz', 'PATTERN PULSE GROUP 4 / 7', 'RA      05h 17m 32s', 'DEC    -05  23\' 14"', 'S/N     4.71', 'SOURCE  UNKNOWN', '', 'SOURCE DISTANCE:  -39 LY', '', '** CHECK SOLVE INPUTS **'];
    const paperGeo = new THREE.PlaneGeometry(0.4, 0.6);
    paperGeo.translate(0, -0.3, 0);
    const paper = new THREE.Mesh(paperGeo, new THREE.MeshStandardMaterial({ map: T.greenbarPaper(lines), roughness: 0.9, side: THREE.DoubleSide }));
    paper.position.set(0, 0.17, -0.09);
    paper.rotation.x = Math.PI - 0.35;
    paper.scale.y = 0.001;
    paper.visible = false;
    pr.add(paper);
    noMerge(paper);
    mergeStatic(pr);
    this.printerPaper = paper;
    this.objs.printer = pr;

    // wall clock
    const clk = new THREE.Group();
    clk.position.set(5.97, 2.35, -3.55); clk.rotation.y = -Math.PI / 2;
    const face = new THREE.Mesh(new THREE.CircleGeometry(0.17, 32), new THREE.MeshStandardMaterial({ map: T.clockFace(), roughness: 0.5 }));
    clk.add(face);
    const ring = new THREE.Mesh(new THREE.TorusGeometry(0.175, 0.012, 6, 32), M.frame);
    clk.add(ring);
    const hand = (len: number, w: number, z: number, mat: THREE.Material) => { const p = new THREE.Group(); p.position.z = z; box(p, w, len, 0.004, mat, 0, len / 2 - 0.015, 0); noMerge(p); clk.add(p); return p; };
    this.clockHands = { h: hand(0.09, 0.012, 0.006, M.darkPlastic), m: hand(0.13, 0.008, 0.009, M.darkPlastic), s: hand(0.14, 0.003, 0.012, M.ledRed) };
    mergeStatic(clk);
    this.group.add(clk);
    this.objs.clock = clk;

    // east door to the service yard: frame, a hinged panel that swings out, lock light
    const door = new THREE.Group();
    door.position.set(6.0, 0, 1.6);
    box(door, 0.34, 0.07, 1.06, M.frame, 0.15, 2.155, 0);
    box(door, 0.34, 2.12, 0.05, M.frame, 0.15, 1.06, -0.505);
    box(door, 0.34, 2.12, 0.05, M.frame, 0.15, 1.06, 0.505);
    const hinge = new THREE.Group();
    hinge.position.set(0.06, 0, -0.47); // hinge on the north jamb
    const panelMat = new THREE.MeshStandardMaterial({ color: 0x5a6466, roughness: 0.5, metalness: 0.4 });
    box(hinge, 0.045, 2.06, 0.93, panelMat, 0, 1.03, 0.47);
    box(hinge, 0.07, 0.04, 0.14, M.steel, -0.045, 1.0, 0.82); // handle, room side
    box(hinge, 0.07, 0.04, 0.14, M.steel, 0.045, 1.0, 0.82);  // handle, yard side
    box(hinge, 0.02, 0.3, 0.01, M.frame, 0.026, 1.6, 0.47);
    mergeStatic(hinge);
    noMerge(hinge);
    door.add(hinge);
    this.doorHinge = hinge;
    this.doorLight = box(door, 0.02, 0.05, 0.05, M.ledRed, -0.03, 1.25, 0.72);
    noMerge(this.doorLight);
    const sign = plane(door, 0.6, 0.22, new THREE.MeshStandardMaterial({ map: T.signTex('SERVICE YARD', 'AUTHORIZED PERSONNEL'), roughness: 0.7 }), -0.025, 2.4, 0, -Math.PI / 2);
    noMerge(sign);
    mergeStatic(door);
    this.group.add(door);
    this.objs.doorEast = door;

    // field camera on a small equipment shelf beside the door
    box(st, 0.36, 0.95, 0.7, M.cabinet, 5.81, 0.475, 0.67);
    box(st, 0.36, 0.02, 0.7, M.deskTop, 5.81, 0.96, 0.67);
    this.col(5.6, 6, 0.3, 1.05);
    const card = plane(st, 0.34, 0.22, new THREE.MeshStandardMaterial({ map: T.cameraCard(), roughness: 0.85 }), 5.985, 1.32, 0.67, -Math.PI / 2);
    card.name = 'cameraCard';
    this.fieldCamera = fieldCameraModel();
    // a larger invisible box around the small camera, so it is easy to aim at
    const camHit = new THREE.Mesh(new THREE.BoxGeometry(0.34, 0.2, 0.34), new THREE.MeshBasicMaterial({ visible: false }));
    camHit.position.y = 0.06;
    this.fieldCamera.add(camHit);
    this.fieldCamera.position.set(5.8, 0.97, 0.66);
    this.fieldCamera.rotation.y = -Math.PI / 2 + 0.35;
    this.group.add(this.fieldCamera);
    this.objs.fieldCamera = this.fieldCamera;

    const poster = plane(st, 0.7, 1.05, new THREE.MeshStandardMaterial({ map: T.posterSaro(), roughness: 0.8 }), 5.98, 1.8, 3.1, -Math.PI / 2);
    poster.name = 'posterSaro';
  }

  // ---------- West wall: map, cabinets, second desk ----------
  private westWall(st: THREE.Group) {
    // The backing front is x=-5.975; the print must sit in front of it.
    const map = plane(this.group, 1.35, 0.9, new THREE.MeshStandardMaterial({ map: T.mapNM(), roughness: 0.85 }), -5.965, 1.75, -3.0, Math.PI / 2);
    noMerge(map);
    this.objs.map = map;
    box(st, 0.03, 0.97, 1.42, M.frame, -5.99, 1.75, -3.0);
    for (const z of [-1.2, -0.6, 0.0]) {
      box(st, 0.55, 1.32, 0.56, M.cabinet, -5.7, 0.66, z);
      for (let i = 0; i < 4; i++) { box(st, 0.005, 0.01, 0.48, M.frame, -5.42, 0.33 + i * 0.32, z); box(st, 0.02, 0.025, 0.1, M.steel, -5.41, 0.43 + i * 0.32, z); }
    }
    this.col(-6, -5.4, -1.5, 0.3);
    blob(this.blobs, -5.6, -0.6, 1.0, 2.0);
    const lp = plane(st, 0.62, 0.92, new THREE.MeshStandardMaterial({ map: T.posterListen(), roughness: 0.8 }), -5.98, 2.12, -0.6, Math.PI / 2);
    lp.name = 'posterListen';
    // fan and plant on cabinets / floor
    cyl(st, 0.07, 0.09, 0.03, M.darkPlastic, -5.7, 1.335, 0.0, 10);
    rod(st, new THREE.Vector3(-5.7, 1.34, 0), new THREE.Vector3(-5.7, 1.55, 0), 0.012, M.steel);
    const fan = new THREE.Mesh(new THREE.TorusGeometry(0.13, 0.008, 4, 24), M.steel);
    fan.position.set(-5.65, 1.62, 0); fan.rotation.y = Math.PI / 2 + 0.4; st.add(fan);
    cyl(st, 0.2, 0.15, 0.36, M.pot, -5.55, 0.18, 3.95, 10);
    for (let i = 0; i < 9; i++) {
      const a = (i / 9) * Math.PI * 2;
      const leaf = new THREE.Mesh(new THREE.ConeGeometry(0.06, 0.7, 4), M.plant);
      leaf.position.set(-5.55 + Math.cos(a) * 0.08, 0.6, 3.95 + Math.sin(a) * 0.08);
      leaf.rotation.set(Math.sin(a) * 0.5, 0, -Math.cos(a) * 0.5);
      st.add(leaf);
    }
    this.col(-6, -5.25, 3.7, 4.5);
    // second workstation against the west wall
    box(st, 0.8, 0.05, 1.5, M.deskTop, -5.58, 0.755, 2.4);
    box(st, 0.74, 0.7, 0.44, M.deskBody, -5.58, 0.38, 1.88);
    box(st, 0.03, 0.72, 1.46, M.deskBody, -5.95, 0.37, 2.4);
    this.col(-6, -5.15, 1.62, 3.18);
    blob(this.blobs, -5.4, 2.4, 1.4, 1.9);
    this.chair(-4.85, 2.55, Math.PI / 2 + 0.3);
    box(st, 0.35, 0.22, 0.3, M.beigeDark, -5.7, 0.89, 1.95);
    box(st, 0.3, 0.02, 0.24, M.paper, -5.45, 0.79, 3.1).rotation.y = 0.3;
    const lamp = new THREE.Group();
    lamp.position.set(-5.72, 0.78, 3.0);
    cyl(lamp, 0.08, 0.09, 0.03, M.lampShade, 0, 0.015, 0, 16);
    rod(lamp, new THREE.Vector3(0, 0.03, 0), new THREE.Vector3(0.05, 0.38, 0), 0.008, M.steel);
    const shade = new THREE.Mesh(new THREE.CylinderGeometry(0.03, 0.11, 0.13, 16, 1, true), M.lampShade);
    shade.position.set(0.15, 0.4, 0); shade.rotation.z = 0.6;
    lamp.add(shade);
    const bulb = new THREE.Mesh(new THREE.SphereGeometry(0.03, 10, 8), M.lampBulb);
    bulb.position.set(0.18, 0.38, 0);
    lamp.add(bulb);
    st.add(lamp);
  }

  // ---------- South wall: corridor door ----------
  // A real door since chapter two: frame in the wall opening, a panel hinged on the east
  // jamb that swings out into the corridor, and the old CORRIDOR sign above it.
  private southWall(st: THREE.Group) {
    const door = new THREE.Group();
    door.position.set(-2.4, 0, 4.65);
    const frameMat = floodlit(new THREE.MeshStandardMaterial({ color: 0x2b3033, roughness: 0.5, metalness: 0.5 }), 0.25, annexFlood);
    box(door, 1.06, 0.07, 0.34, frameMat, 0, 2.155, 0);
    box(door, 0.05, 2.12, 0.34, frameMat, -0.505, 1.06, 0);
    box(door, 0.05, 2.12, 0.34, frameMat, 0.505, 1.06, 0);
    const hinge = new THREE.Group();
    hinge.position.set(0.47, 0, 0.06);
    // lit by the room's lamps on one side and the corridor's on the other
    const panelMat = floodlit(new THREE.MeshStandardMaterial({ color: 0x6b5a45, roughness: 0.7 }), 0.25, annexFlood);
    box(hinge, 0.93, 2.06, 0.045, panelMat, -0.465, 1.03, 0);
    box(hinge, 0.14, 0.04, 0.07, M.steel, -0.82, 1.0, -0.045); // handle, room side
    box(hinge, 0.14, 0.04, 0.07, M.steel, -0.82, 1.0, 0.045);  // handle, corridor side
    mergeStatic(hinge);
    noMerge(hinge);
    door.add(hinge);
    this.southDoorHinge = hinge;
    const sign = plane(door, 0.5, 0.18, new THREE.MeshStandardMaterial({ map: T.signTex('CORRIDOR'), roughness: 0.7 }), 0, 2.4, -0.155, Math.PI);
    noMerge(sign);
    mergeStatic(door);
    this.group.add(door);
    this.objs.doorSouth = door;
    // bulletin board
    box(st, 1.2, 0.8, 0.03, new THREE.MeshStandardMaterial({ color: 0x8a6a45, roughness: 1 }), 1.2, 1.7, 4.48);
    const pr = T.rng(3);
    for (let i = 0; i < 6; i++) {
      const p = plane(st, 0.2, 0.26, new THREE.MeshStandardMaterial({ map: T.deskPapers(i + 9), roughness: 0.9 }), 0.75 + (i % 3) * 0.32, 1.85 - Math.floor(i / 3) * 0.33, 4.46, Math.PI);
      p.rotation.z = (pr() - 0.5) * 0.15;
    }
  }

  // ---------- What Night Shift adds (KAPITLER.md) ----------
  // An invisible box around a small thing, so it is easy to aim at.
  private hit(g: THREE.Object3D, w: number, h: number, d: number, x = 0, y = 0, z = 0) {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), new THREE.MeshBasicMaterial({ visible: false }));
    m.position.set(x, y, z); noMerge(m); g.add(m);
  }
  private nightShift(st: THREE.Group) {
    // the exceptions binder and two others on a shelf above the printer
    box(st, 0.28, 0.025, 0.9, M.deskTop, 5.84, 1.5, -0.4);
    for (const z of [-0.78, -0.02]) box(st, 0.22, 0.12, 0.02, M.steel, 5.9, 1.43, z);
    const shelf = new THREE.Group();
    shelf.position.set(5.84, 1.513, -0.4);
    const spine = new THREE.MeshStandardMaterial({ map: T.binderSpineTex(), roughness: 0.8 });
    const blue = new THREE.MeshStandardMaterial({ color: 0x2f4a6e, roughness: 0.8 });
    faced(shelf, 0.26, 0.3, 0.07, blue, spine, 0, 0.15, -0.12, '-x');
    box(shelf, 0.26, 0.3, 0.07, new THREE.MeshStandardMaterial({ color: 0x6e2f2a, roughness: 0.8 }), 0.0, 0.15, 0.0);
    box(shelf, 0.26, 0.28, 0.06, new THREE.MeshStandardMaterial({ color: 0x3a3d40, roughness: 0.8 }), 0.0, 0.14, 0.1);
    this.hit(shelf, 0.3, 0.36, 0.14, 0, 0.16, -0.12);
    mergeStatic(shelf);
    this.group.add(shelf);
    this.objs.binder = shelf;

    // the service record on a clipboard, hung on the rack under the status lights
    const clip = new THREE.Group();
    clip.position.set(5.335, 0.68, -2.47); clip.rotation.y = -Math.PI / 2;
    plane(clip, 0.2, 0.27, new THREE.MeshStandardMaterial({ map: T.serviceRecordTex(), roughness: 0.85 }), 0, 0, 0.004);
    box(clip, 0.03, 0.02, 0.01, M.steel, 0, 0.15, 0.0);
    this.hit(clip, 0.26, 0.34, 0.08);
    mergeStatic(clip);
    this.group.add(clip);
    this.objs.serviceRecord = clip;

    // the teleprinter in the corner by the corridor door, with its roll of paper
    const tx = new THREE.Group();
    tx.position.set(-4.2, 0, 4.18);
    box(tx, 0.72, 0.72, 0.5, M.cabinet, 0, 0.36, 0);
    box(tx, 0.62, 0.16, 0.42, M.beige, 0, 0.8, -0.02);
    const kb = box(tx, 0.56, 0.05, 0.16, M.beigeDark, 0, 0.75, -0.26); kb.rotation.x = 0.2;
    plane(tx, 0.5, 0.12, new THREE.MeshStandardMaterial({ map: T.keyboard(), roughness: 0.6 }), 0, 0.78, -0.265, 0, -Math.PI / 2 + 0.2);
    cyl(tx, 0.05, 0.05, 0.3, M.paper, 0, 0.9, 0.12, 12).rotation.z = Math.PI / 2;
    const roll = plane(tx, 0.24, 0.42, new THREE.MeshStandardMaterial({ map: T.telexPaperTex(), roughness: 0.9, side: THREE.DoubleSide }), 0, 1.08, 0.02, Math.PI);
    roll.rotation.x = 0.35;
    blob(this.blobs, -4.2, 4.18, 0.95, 0.7);
    mergeStatic(tx);
    this.group.add(tx);
    this.col(-4.6, -3.8, 3.9, 4.5);
    this.objs.telex = tx;

    // the operations terminal on the west desk, facing into the room
    const ops = new THREE.Group();
    ops.position.set(-5.6, 0.78, 2.45); ops.rotation.y = Math.PI / 2;
    box(ops, 0.42, 0.34, 0.34, M.beige, 0, 0.2, -0.04);
    box(ops, 0.34, 0.26, 0.01, M.darkPlastic, 0, 0.21, 0.131);
    const scr = plane(ops, 0.3, 0.22, this.crtOps.mat, 0, 0.21, 0.137);
    noMerge(scr);
    box(ops, 0.44, 0.03, 0.15, M.beige, 0, 0.015, 0.3);
    plane(ops, 0.42, 0.13, new THREE.MeshStandardMaterial({ map: T.keyboard(), roughness: 0.6 }), 0, 0.031, 0.3, 0, -Math.PI / 2);
    mergeStatic(ops);
    this.group.add(ops);
    this.objs.opsTerminal = ops;
    this.crtOps.setPowered(true);

    // the Halley's comet poster on the south wall, behind the supervisor desk
    const hp = new THREE.Group();
    hp.position.set(3.0, 1.78, 4.47); hp.rotation.y = Math.PI;
    plane(hp, 0.5, 0.75, new THREE.MeshStandardMaterial({ map: T.halleyPosterTex(), roughness: 0.8 }), 0, 0, 0);
    this.hit(hp, 0.5, 0.75, 0.04);
    mergeStatic(hp);
    this.group.add(hp);
    this.objs.halley = hp;

    // Dale's card, taped to the console desk beside the receiver keyboard
    const card = new THREE.Group();
    card.position.set(0.42, 0.783, -3.62); card.rotation.y = -0.12;
    plane(card, 0.14, 0.09, new THREE.MeshStandardMaterial({ map: T.rfiCardTex(), roughness: 0.9 }), 0, 0, 0, 0, -Math.PI / 2);
    this.hit(card, 0.2, 0.05, 0.15);
    mergeStatic(card);
    this.group.add(card);
    this.objs.rfiCard = card;

    // your jacket on a hook by the service door: SARO navy, a reflective band all round
    const jk = new THREE.Group();
    jk.position.set(5.93, 0, 2.42);
    const navy = new THREE.MeshStandardMaterial({ color: 0x1f2a44, roughness: 0.85 });
    const band = new THREE.MeshStandardMaterial({ color: 0xc9ccc8, roughness: 0.35, metalness: 0.3 });
    box(jk, 0.04, 0.03, 0.04, M.steel, 0.04, 1.78, 0);
    box(jk, 0.1, 0.62, 0.46, navy, -0.02, 1.4, 0);
    box(jk, 0.11, 0.08, 0.2, navy, -0.02, 1.72, 0);
    for (const z of [-0.26, 0.26]) box(jk, 0.09, 0.56, 0.09, navy, -0.03, 1.36, z);
    box(jk, 0.105, 0.05, 0.465, band, -0.02, 1.26, 0);
    for (const z of [-0.26, 0.26]) box(jk, 0.095, 0.04, 0.095, band, -0.03, 1.2, z);
    this.hit(jk, 0.2, 0.75, 0.64, 0, 1.4, 0);
    mergeStatic(jk);
    this.group.add(jk);
    this.objs.jacket = jk;

    // a small radio on the filing cabinets, left on by the day shift
    const rd = new THREE.Group();
    rd.position.set(-5.68, 1.32, -1.15); rd.rotation.y = Math.PI / 2;
    box(rd, 0.28, 0.15, 0.1, M.darkPlastic, 0, 0.075, 0);
    plane(rd, 0.12, 0.1, new THREE.MeshStandardMaterial({ color: 0x8a8473, roughness: 0.9 }), -0.06, 0.075, 0.051);
    box(rd, 0.09, 0.03, 0.01, M.ledAmber, 0.07, 0.1, 0.052);
    cyl(rd, 0.015, 0.015, 0.02, M.steel, 0.07, 0.05, 0.06, 8).rotation.x = Math.PI / 2;
    rod(rd, new THREE.Vector3(0.12, 0.15, 0), new THREE.Vector3(0.2, 0.42, -0.05), 0.003, M.steel, 4);
    this.hit(rd, 0.32, 0.22, 0.16, 0, 0.09, 0);
    mergeStatic(rd);
    this.group.add(rd);
    this.objs.radio = rd;
  }

  // ---------- runtime ----------
  // 0 closed, 1 open (swung 90 degrees out into the yard)
  setDoor(open01: number) { this.doorHinge.rotation.y = open01 * Math.PI / 2; }
  // 0 closed, 1 open (swung 90 degrees out into the corridor)
  setSouthDoor(open01: number) { this.southDoorHinge.rotation.y = open01 * Math.PI / 2; }
  setDoorLock(unlocked: boolean) { this.doorLight.material = unlocked ? M.ledGreen : M.ledRed; }

  setLed(i: number, state: LedState) {
    this.leds.setColorAt(i, LED_COL[state]);
    this.leds.instanceColor!.needsUpdate = true;
  }
  ledState(i: number): LedState { return i === 7 || i === 15 ? 'amber' : 'green'; }

  setClock(seconds: number) {
    const s = seconds % 60, m = (seconds / 60) % 60, h = (seconds / 3600) % 12;
    this.clockHands.s.rotation.z = -Math.floor(s) / 60 * Math.PI * 2;
    this.clockHands.m.rotation.z = -m / 60 * Math.PI * 2;
    this.clockHands.h.rotation.z = -h / 12 * Math.PI * 2;
  }
}

// keep tree-shaking honest for helpers only used in some builds
export const _unused = floodlit;
