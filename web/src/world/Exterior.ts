import * as THREE from 'three';
import { M, box, cyl, plane, addFlood, floodlit, mergeStatic, noMerge } from './kit';
import { Dish, DishArray } from './Dish';
import { GlowPoints } from './glow';
import { desertGround } from './flora';
import { sharedHorizon } from './horizon';
import { SARO_IN_ROAD, SARO_PATCH, MOTEL_AT, MOTEL_GLOWS, inRect, outside } from './geo';
import { DISH_LAYOUT, DISH_LAYOUT_FAR, ARRAY_FLAT, GATE, FENCE_Z, LAMPS } from './arrayLayout';
import { hLow, smooth } from '../drive/roadShape';
import { highwayTex } from '../drive/roadTextures';
import { wet, HWY_LANES } from '../drive/roadSurface';
import * as T from '../core/textures';
import { artTexture } from '../core/art';

/** Height of SARO's own ground at a site point, as its rings are drawn (Exterior.ground): flat at
 *  -0.62 within 300 m and over the array, rolling further out, and south along the highway the
 *  road's land, kept out of the way under the road's own piece of it (geo.ts, SARO_PATCH). */
export function saroLand(x: number, z: number) {
  const far = Math.hypot(x, z);
  let y = far > 300 ? -2 + Math.sin(x * 0.004 + z * 0.003) * 6 * Math.min(1, (far - 300) / 600) : -0.62;
  // the array is graded flat; the land's roll comes back over 40 m past it
  const e = outside(ARRAY_FLAT, x, z);
  if (e < 40) y = -0.62 + (y + 0.62) * smooth(0, 40, e);
  const rx = x + SARO_IN_ROAD.x, rz = z + SARO_IN_ROAD.z;
  const w = smooth(40, 100, z) * (1 - smooth(0, 160, outside(SARO_PATCH, rx, rz)));
  if (w > 0) y += (hLow(rx, rz) - (inRect(SARO_PATCH, rx, rz, -2) ? 0.7 : 0.05) - y) * w;
  return y;
}

// World layout (metres): control room centred at the origin, windows face north (-Z).
// The road runs north-south west of SARO; Sierra Motor Court is 650 m down it (geo.ts, MOTEL_AT).
// The 27 antennas stand north of the array fence (arrayLayout.ts); its gravel tracks, the gate and
// the drive cabinets are Field.ts.

export class Exterior {
  group = new THREE.Group();
  dishes: Dish[] = [];
  dishArray!: DishArray;
  // Lamp glows, poles and fence posts are batched: one point cloud and one merged group.
  glow = new GlowPoints();
  /** The east wing and the concrete of the control room's roof and the plinths, as built here
   *  (world/blenderModels.ts puts the modelled house in their place when it is shown). */
  house: THREE.Mesh[] = [];
  /** Unlit surfaces SARO's lamps light (the array fence, the lettering on the wing): dimmed with
   *  the power (world/saroPower.ts). */
  readonly paint: THREE.MeshBasicMaterial[] = [];
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
    // ground within 300 m is flat at -0.62, and so is the array out past its last antennas
    // (arrayLayout.ARRAY_FLAT, walked: Field.ts). (A plain circle has only a centre and a rim, and
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
      pos.setY(i, saroLand(x, z));
    }
    geo.setAttribute('color', new THREE.BufferAttribute(col, 3));
    geo.computeVertexNormals();
    // World-space metres avoid a radial UV singularity at the centre of the circle.
    const uv = geo.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < pos.count; i++) uv.setXY(i, pos.getX(i) / 5, pos.getZ(i) / 5);
    const ground = new THREE.Mesh(geo, desertGround(floodlit(new THREE.MeshStandardMaterial({ map: artTexture('desert', [1, 1]), vertexColors: true, color: 0xe8dcc8, roughness: 1 }))));
    this.group.add(ground);
    // the brush, the grass and the stones on it are world/flora.ts (World.ts, groundCover.ts);
    // past their reach the ground has the brush's dark spots (flora.desertGround)
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
    // the first five carry the floods; one of the others stood in the middle of the highway (-24, -106)
    LAMPS.forEach(([x, z], i) => lamp(x, z, i < 5 ? 7 : 6));
  }

  private road() {
    // the site's own highway ends at z 36; south of it the road's own stretch takes over
    // (drive/corridors.ts), the same road the truck goes on in the road area. North it goes on
    // through the array and out past its last antennas, on the array's flat ground (z -480).
    // It is the road's own asphalt (drive/roadTextures.ts), its middle 8 m, in step with the
    // stretch south of z 36 (road-local z -264, 48 m to a repeat), with the same damp sheen.
    const asphalt = wet(floodlit(new THREE.MeshStandardMaterial({ map: highwayTex(), roughness: 0.55 }), 0.02), 0.5, 100, 9, { lanes: HWY_LANES, dry: -0.15 });
    const r = plane(this.group, 8, 516, asphalt, -24, -0.58, -222, 0, -Math.PI / 2);
    const uv = r.geometry.attributes.uv as THREE.BufferAttribute;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, (10 - 8 * uv.getX(i)) / 12, (-264 - uv.getY(i) * 516) / 48);
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
    wing.name = 'house:wing';
    // lettering on the south face of the wing
    const letters = new THREE.Mesh(new THREE.PlaneGeometry(8, 2.5), new THREE.MeshBasicMaterial({ map: T.saroLettering(), transparent: true, toneMapped: false, color: 0xd9cfb6 }));
    letters.position.set(26.4, 2.0, 5.53);
    this.paint.push(letters.material);
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
    const roof = g.children.find((c) => (c as THREE.Mesh).material === M.concrete) as THREE.Mesh;
    roof.name = 'house:roof';
    this.house = [wing, roof];
    this.group.add(g);
  }

  // Sierra Motor Court itself is world/MotelFront.ts (Codex, chapter four); only its road
  // sign stands here, where the court stands now (MOTEL_AT: built at the old spot, then moved).
  // The sign is double-faced: the back is the same picture, not mirrored.
  private motel() {
    const g = new THREE.Group();
    g.position.set(MOTEL_AT.x, MOTEL_AT.y, MOTEL_AT.z);
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
    const A = MOTEL_AT;
    addFlood(-29 + A.x, 6 + A.y, 32 + A.z, 18, 0xff4a35); addFlood(-29 + A.x, 4 + A.y, 32 + A.z, 6, 0x50f0d8);
    // its lights from far off: the sign is a landmark on the way south
    for (const [x, y, z, size, c] of MOTEL_GLOWS) this.glow.add(x + A.x, y + A.y, z + A.z, size, c);
    mergeStatic(g);
    this.group.add(g);
  }

  private fence() {
    const tex = T.chainlink();
    const mat = new THREE.MeshBasicMaterial({ map: tex, transparent: true, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x6d6a62 });
    // three runs: the highway between the first two (it goes on north through the array), and
    // the walk gate into the array between the last two (its posts and leaf are Field.ts)
    for (const [x0, x1] of [[-80, -28.6], [-19.4, GATE.x0], [GATE.x1, 60]]) {
      const f = plane(this.group, x1 - x0, 2.4, mat.clone(), (x0 + x1) / 2, 0.6, FENCE_Z);
      (f.material as THREE.MeshBasicMaterial).map = tex.clone();
      (f.material as THREE.MeshBasicMaterial).map!.repeat.x = tex.repeat.x * (x1 - x0) / 140;
      (f.material as THREE.MeshBasicMaterial).map!.needsUpdate = true;
      noMerge(f);
      this.paint.push(f.material as THREE.MeshBasicMaterial);
    }
    const gap = (x: number) => (x > -28.6 && x < -19.4) || (x > GATE.x0 - 0.8 && x < GATE.x1 + 0.8);
    for (let x = -80; x <= 60; x += 7) if (!gap(x)) cyl(this.statics, 0.05, 0.05, 2.6, M.pole, x, 0.6, FENCE_Z, 5);
    for (const x of [-28.6, -19.4]) cyl(this.statics, 0.06, 0.06, 2.7, M.pole, x, 0.6, FENCE_Z, 5);
  }

  update(dt: number, t: number) {
    this.dishArray.update(dt, t);
    this.glow.update(t);
  }
}
