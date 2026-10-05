import * as THREE from 'three';
import { M, box, cyl, plane, addFlood, floodlit, mergeStatic, noMerge } from './kit';
import { Dish, DishArray } from './Dish';
import { GlowPoints } from './glow';
import { sharedHorizon } from './horizon';
import { SARO_IN_ROAD, SARO_PATCH, inRect, outside } from './geo';
import { hLow, smooth } from '../drive/roadShape';
import * as T from '../core/textures';
import { artTexture } from '../core/art';

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
    this.array();
    this.road();
    this.building();
    this.motel();
    this.fence();
    mergeStatic(this.statics);
    // the skyline of the whole map (horizon.ts): the road's far mesas, and those near STATION 01
    this.group.add(sharedHorizon('saro', this.glow));
    this.group.add(this.statics, this.glow.build());
  }

  private ground() {
    // Rings that grow outwards (each about 7 per cent wider than the last), so every cell is
    // roughly square: near SARO they are a few metres across, at the horizon a hundred. The
    // ground within 300 m is flat at -0.62. (A plain circle has only a centre and a rim, and
    // its triangles sloped out to the rolling rim and rose over the highway; evenly spaced
    // rings left long slivers at the centre that leaked through the control room floor as a
    // dotted line on some screens.)
    const R0 = 0.5, R1 = 1900;
    const geo = new THREE.RingGeometry(R0, R1, 96, 120);
    {
      const p = geo.attributes.position as THREE.BufferAttribute;
      for (let i = 0; i < p.count; i++) {
        const x = p.getX(i), y = p.getY(i), r = Math.hypot(x, y);
        const k = R0 * Math.pow(R1 / R0, (r - R0) / (R1 - R0)) / r;
        p.setXY(i, x * k, y * k);
      }
    }
    geo.rotateX(-Math.PI / 2);
    const pos = geo.attributes.position as THREE.BufferAttribute;
    const col = new Float32Array(pos.count * 3);
    const r = T.rng(5);
    for (let i = 0; i < pos.count; i++) {
      const x = pos.getX(i), z = pos.getZ(i);
      const n = 0.75 + 0.25 * Math.sin(x * 0.013) * Math.cos(z * 0.011) + r() * 0.12;
      col.set([n, n, n], i * 3);
      const far = Math.hypot(x, z);
      let y = far > 300 ? -2 + Math.sin(x * 0.004 + z * 0.003) * 6 * Math.min(1, (far - 300) / 600) : -0.62;
      // south along the highway the ground is the road's land (drive/roadShape.ts), and under
      // the road's own piece of ground (geo.ts, SARO_PATCH) it keeps out of the way
      const rx = x + SARO_IN_ROAD.x, rz = z + SARO_IN_ROAD.z;
      const w = smooth(40, 100, z) * (1 - smooth(0, 160, outside(SARO_PATCH, rx, rz)));
      if (w > 0) y += (hLow(rx, rz) - (inRect(SARO_PATCH, rx, rz, -2) ? 0.7 : 0.05) - y) * w;
      pos.setY(i, y);
    }
    geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
    geo.computeVertexNormals();
    // World-space metres avoid a radial UV singularity at the centre of the circle.
    const uv = geo.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < pos.count; i++) uv.setXY(i, pos.getX(i) / 5, pos.getZ(i) / 5);
    const ground = new THREE.Mesh(geo, floodlit(new THREE.MeshStandardMaterial({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe8dcc8, roughness: 1 })));
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
      if (x > -9 && x < 40 && z > -8 && z < 13) continue; // buildings and the records annex
      if (x > 4 && x < 21 && z > -21 && z < 2) continue;  // service yard and photo lab
      if (x > -17 && x < -6 && z > 3 && z < 8.5) continue;  // the path from the fire exit
      if (x > -50 && x < -28 && z > 0 && z < 70) continue;  // the motel's lot and front
      if (x > -21 && x < 15 && z > 11 && z < 29) continue;  // the service road, the path and the yard behind the annex (Grounds.ts)
      if (x > -11 && x < -6 && z > -20 && z < 12) continue;  // the west side of the house
      if (x > -11 && x < 8 && z > -20 && z < -5) continue;  // the windows side
      const k = 0.4 + r() * 1.1;
      p.set(x, -0.55, z); s.set(k, k * (0.6 + r() * 0.6), k); q.setFromAxisAngle(new THREE.Vector3(0, 1, 0), r() * 6);
      scrub.setMatrixAt(n++, m.compose(p, q, s));
    }
    this.group.add(scrub);
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
    // the site's own highway ends at z 36; south of it the road's own stretch takes over
    // (drive/corridors.ts), the same road the truck goes on in the road area
    const tex = T.roadTex();
    tex.repeat.set(1, 32);
    const asphalt = floodlit(new THREE.MeshStandardMaterial({ map: tex, roughness: 0.55 }), 0.02);
    const r = plane(this.group, 8, 256, asphalt, -24, -0.58, -92, 0, -Math.PI / 2);
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

  // Sierra Motor Court itself is world/MotelFront.ts (Codex, chapter four); only its road
  // sign stands here. The sign is double-faced: the back is the same picture, not mirrored.
  private motel() {
    const g = new THREE.Group();
    cyl(g, 0.22, 0.22, 9, M.pole, -31.5, 3.9, 30, 8);
    cyl(g, 0.22, 0.22, 9, M.pole, -29.3, 3.9, 30, 8);
    const face = (back: boolean) => {
      const map = back ? artTexture('sierra').clone() : artTexture('sierra');
      if (back) { map.wrapS = THREE.RepeatWrapping; map.repeat.x = -1; map.offset.x = 1; map.needsUpdate = true; }
      const m = new THREE.Mesh(new THREE.PlaneGeometry(5.2, 7.8), new THREE.MeshBasicMaterial({ map, transparent: true, alphaTest: 0.02, depthWrite: false, side: back ? THREE.BackSide : THREE.FrontSide, toneMapped: false }));
      m.name = back ? 'Sierra Motor Court (back)' : 'Sierra Motor Court';
      m.position.set(-30.4, 5.8, 30.2); m.rotation.y = Math.PI / 2 - 0.5;
      noMerge(m); g.add(m);
    };
    face(false); face(true);
    addFlood(-29, 6, 32, 18, 0xff4a35); addFlood(-29, 4, 32, 6, 0x50f0d8);
    mergeStatic(g);
    this.group.add(g);
  }

  private fence() {
    const tex = T.chainlink();
    const mat = new THREE.MeshBasicMaterial({ map: tex, transparent: true, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x6d6a62 });
    // two runs with the highway between them (it goes on north through the array)
    for (const [x0, x1] of [[-80, -28.6], [-19.4, 60]]) {
      const f = plane(this.group, x1 - x0, 2.4, mat.clone(), (x0 + x1) / 2, 0.6, -20);
      (f.material as THREE.MeshBasicMaterial).map = tex.clone();
      (f.material as THREE.MeshBasicMaterial).map!.repeat.x = tex.repeat.x * (x1 - x0) / 140;
      (f.material as THREE.MeshBasicMaterial).map!.needsUpdate = true;
      noMerge(f);
    }
    for (let x = -80; x <= 60; x += 7) if (x < -28.6 || x > -19.4) cyl(this.statics, 0.05, 0.05, 2.6, M.pole, x, 0.6, -20, 5);
    for (const x of [-28.6, -19.4]) cyl(this.statics, 0.06, 0.06, 2.7, M.pole, x, 0.6, -20, 5);
  }

  update(dt: number, t: number) {
    this.dishArray.update(dt, t);
    this.glow.update(t);
  }
}
