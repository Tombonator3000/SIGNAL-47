import * as THREE from 'three';
import { M, box, cyl, plane, addFlood, floodlit, mergeStatic, noMerge } from './kit';
import { Dish, DishArray } from './Dish';
import { GlowPoints } from './glow';
import * as T from '../core/textures';

// World layout (metres): control room centred at the origin, windows face north (-Z).
// The road runs north-south west of SARO; Sierra Motor Court sits on the far side of it.
// S-03 is the central antenna at the end of the east service walk (chapter one).
const DISH_LAYOUT: [string, number, number, number][] = [
  ['S-01', -48, -66, 0.85], ['S-02', -16, -52, 0.85], ['S-03', 9.5, -42, 0.9], ['S-04', 6, -88, 0.85],
  ['S-05', 44, -96, 0.85], ['S-06', -30, -116, 0.85], ['S-07', -80, -104, 0.85], ['S-08', 20, -146, 0.85],
  ['S-09', 66, -156, 0.85], ['S-10', -58, -168, 0.85], ['S-11', -10, -200, 0.85], ['S-12', 100, -120, 0.85],
  ['S-13', -120, -150, 0.85], ['S-14', 40, -230, 0.85], ['S-15', -96, -230, 0.85],
];
// SARO has 27 antennas (story bible, chapter X and the LISTEN ending). The far twelve use the light model.
const DISH_LAYOUT_FAR: [string, number, number, number][] = [
  ['S-16', 130, -210, 0.85], ['S-17', -150, -270, 0.85], ['S-18', 80, -290, 0.85], ['S-19', -40, -300, 0.85],
  ['S-20', 170, -260, 0.85], ['S-21', -200, -210, 0.85], ['S-22', 10, -360, 0.85], ['S-23', 120, -350, 0.85],
  ['S-24', -120, -340, 0.85], ['S-25', -230, -300, 0.85], ['S-26', 210, -330, 0.85], ['S-27', -60, -420, 0.85],
];

export class Exterior {
  group = new THREE.Group();
  dishes: Dish[] = [];
  dishArray!: DishArray;
  // Lamp glows, poles and fence posts are batched: one point cloud and one merged group.
  glow = new GlowPoints();
  private statics = new THREE.Group();

  constructor() {
    this.ground();
    this.mountains();
    this.array();
    this.road();
    this.building();
    this.motel();
    this.fence();
    mergeStatic(this.statics);
    this.group.add(this.statics, this.glow.build());
  }

  private ground() {
    const geo = new THREE.CircleGeometry(1900, 96, 0, Math.PI * 2);
    geo.rotateX(-Math.PI / 2);
    const pos = geo.attributes.position as THREE.BufferAttribute;
    const col = new Float32Array(pos.count * 3);
    const r = T.rng(5);
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i), z = pos.getZ(i);
      const n = 0.75 + 0.25 * Math.sin(x * 0.013) * Math.cos(z * 0.011) + r() * 0.12;
      col.set([0.30 * n, 0.24 * n, 0.19 * n], i * 3);
      const far = Math.hypot(x, z);
      if (far > 300) pos.setY(i, -2 + Math.sin(x * 0.004 + z * 0.003) * 6 * Math.min(1, (far - 300) / 600));
      else pos.setY(i, -0.62);
    }
    geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
    geo.computeVertexNormals();
    const ground = new THREE.Mesh(geo, floodlit(new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 1 })));
    this.group.add(ground);

    // scrub
    const bush = new THREE.IcosahedronGeometry(0.6, 0);
    bush.scale(1, 0.55, 1);
    const scrub = new THREE.InstancedMesh(bush, floodlit(new THREE.MeshStandardMaterial({ color: 0x4b4a2f, roughness: 1, flatShading: true })), 900);
    const m = new THREE.Matrix4(), q = new THREE.Quaternion(), s = new THREE.Vector3(), p = new THREE.Vector3();
    let n = 0;
    while (n < 900) {
      const x = (r() - 0.5) * 420, z = (r() - 0.5) * 420 - 60;
      if (Math.abs(x + 24) < 7) continue;             // road
      if (x > -9 && x < 40 && z > -8 && z < 9) continue;  // buildings
      if (x > 4 && x < 21 && z > -21 && z < 2) continue;  // service yard and photo lab
      const k = 0.4 + r() * 1.1;
      p.set(x, -0.55, z); s.set(k, k * (0.6 + r() * 0.6), k); q.setFromAxisAngle(new THREE.Vector3(0, 1, 0), r() * 6);
      scrub.setMatrixAt(n++, m.compose(p, q, s));
    }
    this.group.add(scrub);
  }

  private mountains() {
    const ring = (radius: number, peaks: number, hMin: number, hMax: number, color: number, seed: number) => {
      const r = T.rng(seed);
      const N = 220;
      const heights: number[] = [];
      for (let i = 0; i < N; i++) {
        const a = i / N * Math.PI * 2;
        let h = 0;
        for (let k = 1; k <= 4; k++) h += Math.sin(a * peaks * k + seed * k) / k;
        heights.push(hMin + (hMax - hMin) * (0.5 + 0.35 * h + 0.15 * r()));
      }
      const pos: number[] = [];
      for (let i = 0; i < N; i++) {
        const a0 = i / N * Math.PI * 2, a1 = (i + 1) / N * Math.PI * 2;
        const h0 = heights[i], h1 = heights[(i + 1) % N];
        const x0 = Math.cos(a0) * radius, z0 = Math.sin(a0) * radius, x1 = Math.cos(a1) * radius, z1 = Math.sin(a1) * radius;
        pos.push(x0, -5, z0, x1, -5, z1, x1 * 0.97, h1, z1 * 0.97, x0, -5, z0, x1 * 0.97, h1, z1 * 0.97, x0 * 0.97, h0, z0 * 0.97);
      }
      const g = new THREE.BufferGeometry();
      g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
      const mat = new THREE.MeshBasicMaterial({ color, fog: false, side: THREE.DoubleSide });
      this.group.add(new THREE.Mesh(g, mat));
    };
    ring(1500, 3, 30, 150, 0x0d1220, 3);
    ring(1100, 5, 10, 70, 0x0a0d14, 9);
  }

  private array() {
    for (const [id, x, z, s] of DISH_LAYOUT) this.dishes.push(new Dish(id, x, z, s, 42, 48));
    for (const [id, x, z, s] of DISH_LAYOUT_FAR) this.dishes.push(new Dish(id, x, z, s, 42, 48, 1));
    this.dishArray = new DishArray(this.dishes);
    this.group.add(this.dishArray.group);
    // sodium floods near the closest dishes
    addFlood(30, 3, -30, 46); addFlood(-10, 3, -42, 40); addFlood(-40, 3, -56, 34);
    addFlood(12, 3, -78, 30); addFlood(48, 3, -86, 28);
    const lamp = (x: number, z: number, h = 7) => {
      cyl(this.statics, 0.12, 0.16, h, M.pole, x, h / 2 - 0.6, z, 6);
      this.glow.add(x, h - 0.5, z, 2.6, 0xffb45a);
    };
    lamp(30, -30); lamp(-10, -42); lamp(-40, -56); lamp(12, -78); lamp(48, -86);
    [[-70, -94], [-24, -106], [60, -146], [-52, -158], [-4, -190], [94, -110]].forEach(([x, z]) => lamp(x, z, 6));
  }

  private road() {
    const asphalt = floodlit(new THREE.MeshStandardMaterial({ map: T.roadTex(), roughness: 0.55 }), 0.02);
    const r = plane(this.group, 8, 480, asphalt, -24, -0.58, 20, 0, -Math.PI / 2);
    noMerge(r);
    for (let z = 70; z > -200; z -= 34) {
      cyl(this.statics, 0.12, 0.16, 8, M.pole, -18.5, 3.4, z, 6);
      box(this.statics, 1.8, 0.12, 0.2, M.pole, -19.3, 7.3, z);
      this.glow.add(-20.1, 7.1, z, 3, 0xffaf55);
    }
    addFlood(-20, 6, 58, 30); addFlood(-20, 6, 24, 30); addFlood(-20, 6, -10, 28);
  }

  private building() {
    const g = new THREE.Group();
    // East wing of SARO (labs and offices) behind the control room
    const wingMats = [
      new THREE.MeshStandardMaterial({ map: T.concrete(4, [3, 1]), roughness: 0.95 }),
      new THREE.MeshStandardMaterial({ map: T.concrete(4, [3, 1]), roughness: 0.95 }),
      new THREE.MeshStandardMaterial({ color: 0x3a3833, roughness: 1 }),
      new THREE.MeshStandardMaterial({ color: 0x3a3833, roughness: 1 }),
      new THREE.MeshStandardMaterial({ map: T.facadeWindows(8, 1, 3), roughness: 0.9, emissive: 0xffffff, emissiveIntensity: 0.0 }),
      new THREE.MeshStandardMaterial({ map: T.facadeWindows(8, 1, 8), roughness: 0.9 }),
    ];
    wingMats.forEach((m) => floodlit(m as THREE.MeshStandardMaterial, 0.03));
    const wing = box(g, 18, 4.2, 11, wingMats, 29.6, 1.5, 0);
    noMerge(wing);
    // lettering on the south face of the wing
    const letters = new THREE.Mesh(new THREE.PlaneGeometry(8, 2.5), new THREE.MeshBasicMaterial({ map: T.saroLettering(), transparent: true, toneMapped: false, color: 0xd9cfb6 }));
    letters.position.set(26.4, 2.0, 5.53);
    noMerge(letters);
    g.add(letters);
    // roof parapet of the control room
    box(g, 12.6, 0.5, 9.6, M.concrete, 0, 3.35, 0);
    box(g, 0.6, 0.3, 0.6, M.steel, -3, 3.75, 2);
    box(g, 1.4, 0.8, 1.1, M.steel, 2.5, 3.95, -1.5);
    // plinths under the control room and the east wing (the service yard has its own slabs)
    box(g, 13.4, 0.6, 11.8, M.concrete, 0, -0.31, 0);
    box(g, 18.8, 0.6, 11.8, M.concrete, 29.6, -0.31, 0);
    // wall pack lights
    [[-6.4, 2.6, 2], [20.9, 2.8, 5.8], [30.4, 2.8, 5.8], [-6.4, 2.6, -3]].forEach(([x, y, z]) => {
      box(g, 0.35, 0.25, 0.25, M.steel, x, y, z);
      this.glow.add(x, y - 0.1, z + (z > 0 ? 0.25 : 0), 1.6, 0xffbe6e);
    });
    addFlood(-7.5, 2.4, 2, 10); addFlood(24.4, 2.6, 7, 14);
    mergeStatic(g);
    this.group.add(g);
  }

  private motel() {
    const g = new THREE.Group();
    const face = floodlit(new THREE.MeshStandardMaterial({ map: T.motelFacade(), roughness: 0.9 }), 0.05);
    const wall = floodlit(new THREE.MeshStandardMaterial({ color: 0x8f6c4b, roughness: 0.95 }), 0.05);
    const mats = [face, wall, M.concrete, M.concrete, wall, wall];
    const b = box(g, 7, 3.2, 44, mats, -46, 1.0, 46);
    b.rotation.y = 0;
    // the facade texture is mapped to +X (facing the road)
    noMerge(b);
    box(g, 9, 0.3, 45, M.concrete, -44.5, 2.75, 46); // canopy
    for (let z = 26; z <= 66; z += 5) cyl(g, 0.08, 0.08, 3.2, M.pole, -40.4, 1.0, z, 6);
    addFlood(-40, 2.4, 36, 8, 0xffc070); addFlood(-40, 2.4, 56, 8, 0xffc070);
    // parked sedan
    const car = floodlit(new THREE.MeshStandardMaterial({ color: 0x8a8273, roughness: 0.4, metalness: 0.4 }), 0.05);
    box(g, 2.0, 0.7, 4.6, car, -37.5, -0.15, 34);
    box(g, 1.8, 0.55, 2.4, car, -37.5, 0.45, 34.2);
    box(g, 1.82, 0.4, 2.2, M.darkPlastic, -37.5, 0.48, 34.2);
    // sign
    cyl(g, 0.22, 0.22, 9, M.pole, -31.5, 3.9, 30, 8);
    cyl(g, 0.22, 0.22, 9, M.pole, -29.3, 3.9, 30, 8);
    const sierra = new THREE.Mesh(new THREE.PlaneGeometry(4.8, 2.4), new THREE.MeshBasicMaterial({ map: T.neonSierra(), toneMapped: false }));
    sierra.position.set(-30.4, 7.6, 30.2); sierra.rotation.y = Math.PI / 2 - 0.5;
    const mc = new THREE.Mesh(new THREE.PlaneGeometry(4.8, 1.2), new THREE.MeshBasicMaterial({ map: T.neonMotorCourt(), toneMapped: false }));
    mc.position.set(-30.4, 5.8, 30.2); mc.rotation.y = Math.PI / 2 - 0.5;
    const mq = new THREE.Mesh(new THREE.PlaneGeometry(3.2, 2.4), new THREE.MeshBasicMaterial({ map: T.marquee(), toneMapped: false, color: 0xd8d0bc }));
    mq.position.set(-30.4, 3.6, 30.2); mq.rotation.y = Math.PI / 2 - 0.5;
    [sierra, mc, mq].forEach((m) => { noMerge(m); g.add(m); });
    addFlood(-29, 6, 32, 18, 0xff4a35); addFlood(-29, 4, 32, 6, 0x50f0d8);
    mergeStatic(g);
    this.group.add(g);
  }

  private fence() {
    const tex = T.chainlink();
    const mat = new THREE.MeshBasicMaterial({ map: tex, transparent: true, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x6d6a62 });
    const f = plane(this.group, 140, 2.4, mat, -10, 0.6, -20);
    noMerge(f);
    for (let x = -80; x <= 60; x += 7) cyl(this.statics, 0.05, 0.05, 2.6, M.pole, x, 0.6, -20, 5);
  }

  update(dt: number, t: number) {
    this.dishArray.update(dt, t);
    this.glow.update(t);
  }
}
