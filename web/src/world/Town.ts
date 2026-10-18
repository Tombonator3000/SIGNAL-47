import * as THREE from 'three';
import { box, cyl, plane, rod, noMerge, mergeStatic, floodSet, floodlit, addFlood, setFlood, type FloodSet } from './kit';
import { GlowPoints } from './glow';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import type { Surface } from '../core/Audio';
import { surfaceY, hash, DINER, TOWN } from '../drive/roadTerrain';
import { SIGN_OFF } from '../core/radio';

// Caliche, New Mexico, pop. 412 (HENDELSER.md, section 9; Tom, 16 October). The highway is its
// main street, from Mesa Diner, the northernmost house, about 300 m south: the gas station, the
// sheriff's substation, the laundromat, the Plaza (closed) and the feed store on the west side;
// 1420 AM, the souvenir shop, the church and a few houses on the east. A light hangs over the
// crossing where the old Roswell road goes off, blinking yellow.
//
// It stands in the road area like the diner (World.ts): built in road-local metres (x east from
// the highway's centre line, z south), its group at the road's origin, on the road's own ground
// (roadTerrain.surfaceY). Nothing here is entered: the street, the sidewalks and the lots are
// walked, the windows are looked into. Every static part is merged by material (about twenty draw
// calls), every lit window, sign and lamp lens is one mesh of quads coloured per vertex (one draw
// call, switched by the night's hours), every glow one GlowPoints.
//
// The night's hours (update): the deputy at his desk until 02:45 and again from 04:30, his car in
// front; the Night Line on the air until four; a television on in two houses; the Sentinel's
// bundle at the gas station from 04:45; from 05:05 the street lamps go off one by one, each on
// its own photocell.

// sixteen lamps of its own (the last a porch light, 17 October), three for the driven truck's headlights (shine)
export const townFlood = floodSet(20, 'town', 0.1);
let floodOwner: Town | null = null;
const FALL = 0.03;
const SODIUM = 0xffa552;
const AW = 2048, AH = 1024;
type Rect = readonly [number, number, number, number];
// the sign atlas, drawn once (drawAtlas): one picture for every sign and every window
const R = {
  caliche: [0, 0, 512, 256], gas: [512, 0, 512, 256], am: [1024, 0, 512, 256], sheriff: [1536, 0, 512, 128],
  souvenir: [0, 256, 512, 256], marquee: [512, 256, 1024, 128], board: [512, 384, 512, 128], feed: [1024, 384, 512, 128],
  laundro: [1536, 256, 512, 128], open24: [1536, 384, 512, 128],
  inGas: [0, 512, 512, 256], inSheriff: [512, 512, 512, 256], inStudio: [1024, 512, 512, 256], inLaundro: [1536, 512, 512, 256],
  inTv: [0, 768, 256, 256], inWarm: [256, 768, 256, 256], poster: [512, 768, 256, 256], display: [768, 768, 256, 256],
  paper: [1024, 768, 256, 256], pump: [1280, 768, 128, 128], phone: [1280, 896, 256, 64], closed: [1280, 960, 128, 64],
  white: [1416, 776, 48, 48], lens: [1472, 768, 64, 64], blade: [1536, 768, 96, 256], onAir: [1648, 776, 112, 48],
} as const satisfies Record<string, Rect>;
type PanelId = 'lamps' | 'blink' | 'canopy' | 'gasSign' | 'gasWin' | 'sheriffWin' | 'laundro' | 'laundroWin' | 'studio' | 'onAir' | 'amSign'
  | 'display' | 'tv1' | 'tv2' | 'warm' | 'pumps' | string;
/** Where the street lamps stand (road-local z, west side) and which goes off when at dawn. */
const LAMPS = [2030, 2092, 2122, 2150, 2184, 2222, 2262];
const LAMP_X = -7, ARM_X = -3.9;
/** What the town is called in its parts, for the place shown in the corner (main.ts). */
const PLACES: { name: string; x: number; z: number }[] = [
  { name: 'Caliche, the gas station', x: -12, z: 2064 }, { name: "Caliche, the sheriff's substation", x: -14, z: 2106 },
  { name: 'Caliche, the laundromat', x: -13, z: 2133 }, { name: 'Caliche, the Plaza', x: -13, z: 2162 },
  { name: 'Caliche, the feed store', x: -12, z: 2202 }, { name: 'Caliche, 1420 AM', x: 17, z: 2036 },
  { name: 'Caliche, the souvenir shop', x: 17, z: 2064 }, { name: 'Caliche, the church', x: 17, z: 2094 },
];

type Anchor = { x: number; z: number; yaw: number };
type Built = { x0: number; x1: number; z0: number; z1: number; floor: number; top: number; fx: number; s: 1 | -1 };
type Panel = { at: number[]; c: THREE.Color; k: number };
type Box = Collider & { kind: 'box' };
const V = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);

// Position-based UVs, in local metres (repeat scales belong to the sampler), as Diner.ts.
function metreUV(m: THREE.Mesh) {
  m.updateMatrix();
  const p = m.geometry.attributes.position, n = m.geometry.attributes.normal, uv = m.geometry.attributes.uv;
  const v = new THREE.Vector3(), q = new THREE.Vector3(), nm = new THREE.Matrix3().getNormalMatrix(m.matrix);
  for (let i = 0; i < p.count; i++) {
    v.fromBufferAttribute(p, i).applyMatrix4(m.matrix); q.fromBufferAttribute(n, i).applyMatrix3(nm);
    if (Math.abs(q.y) > .5) uv.setXY(i, v.x, v.z); else if (Math.abs(q.x) > .5) uv.setXY(i, -v.z, v.y); else uv.setXY(i, v.x, v.y);
  }
  return m;
}

export class Town {
  readonly group = new THREE.Group();
  readonly zones: Zone[] = [];
  /** For the player (world terms). */
  readonly colliders: Collider[] = [];
  /** For the truck (world terms; the same objects, so the deputy's car comes and goes in both). */
  readonly obstacles: Box[] = [];
  readonly proxies: Record<string, THREE.Object3D> = {};
  readonly flood = townFlood;
  readonly anchors: Record<'street' | 'gas' | 'church', Anchor>;
  /** The deputy is at his desk (his car out front, the window lit). */
  deputyIn = true;
  /** The Sentinel's bundle lies by the gas station door. */
  paperIn = false;
  /** The woman with the radio on the laundromat's bench, and the souvenir man smoking on his stoop. */
  womanIn = true;
  smokerIn = true;
  private drawing = false;
  /** Event 1 (story/townEvents.ts): the payphone ringing, and who answers it. */
  ringing = false;
  onAnswer: (() => void) | null = null;
  /** Event 6: the souvenir shop's cardboard alien on the highway's shoulder north of town. */
  cutoutOut = false;
  readonly cutoutAt = V(-6.4, 0, 1450);
  private readonly cutout = new THREE.Group();
  /** Event 12: the deputy's car on the highway's shoulder north of town, engine running, the
   *  door open, nobody in it (while he is out on patrol). */
  carOut = false;
  readonly carShoulder = V(6.9, 0, 1690);
  private readonly carDoor = new THREE.Group();
  /** Event 8: the street lamps out one by one down the street and back on (running time). */
  private waveT0 = -1e9;
  private readonly origin: THREE.Vector3;
  private readonly st = new THREE.Group();
  private readonly car = new THREE.Group();
  private readonly paper = new THREE.Group();
  private readonly woman = new THREE.Group();
  private readonly smoker = new THREE.Group();
  private readonly materials = new Set<THREE.Material>();
  private readonly hit = new THREE.MeshBasicMaterial({ visible: false });
  private readonly glow = new GlowPoints();
  private readonly atlas: THREE.CanvasTexture;
  private m!: ReturnType<Town['makeMaterials']>;
  private carCol!: Box;
  private readonly slots: Record<string, number> = {};
  private readonly head: number[] = [];
  private readonly lamps: { z: number; slot: number; glow: number[]; quad: number; off: number; on: boolean }[] = [];
  private readonly blinkGlow: number[] = [];
  private blinkOn = true;
  // the lit quads: positions, atlas coordinates, colours, by panel
  private readonly qPos: number[] = [];
  private readonly qUv: number[] = [];
  private readonly qCol: number[] = [];
  private readonly panels = new Map<PanelId, Panel>();
  private qMesh!: THREE.Mesh;
  private dirty = false;

  constructor(origin: THREE.Vector3) {
    this.origin = origin.clone();
    this.group.name = 'Caliche';
    this.group.position.copy(origin);
    this.group.add(this.st, this.car, this.paper, this.woman, this.smoker, this.cutout);
    noMerge(this.car); noMerge(this.paper); noMerge(this.woman); noMerge(this.smoker); noMerge(this.cutout);
    floodOwner = this; townFlood.count = 0;
    for (const p of townFlood.pos) p.set(0, -999, 0, 0);
    this.materials.add(this.hit);
    const c = document.createElement('canvas'); c.width = AW; c.height = AH;
    drawAtlas(c.getContext('2d')!);
    this.atlas = new THREE.CanvasTexture(c);
    this.atlas.colorSpace = THREE.SRGBColorSpace; this.atlas.anisotropy = 4;
    this.m = this.makeMaterials();
    for (const [k, mat] of Object.entries(this.m)) mat.name = 'town/' + k;

    this.townSigns(); this.crossing(); this.streetLamps(); this.ground();
    this.gasStation(); this.sheriff(); this.laundromat(); this.cinema(); this.feedStore();
    this.station1420(); this.souvenirs(); this.church(); this.houses(); this.poles();
    this.patrolCar(); this.bundle(); this.people(); this.cardboard();
    for (let i = 0; i < 3; i++) { const h = addFlood(0, -999, 0, 0, 0xfff0d8, townFlood); townFlood.skip.add(h); this.head.push(h); }

    // the street and the sidewalks are walked, from the diner's lot to the last house; and the
    // crossing in front of the diner, to the verge across the highway (the old road's barricades
    // are seen from there). Each overlaps the next, and the diner's lot.
    this.zone('crossing', DINER.x + 16, 16.4, 1978, 2016.5);
    this.zone('main', -17.6, 19.5, 2015, 2256);
    const a = (x: number, z: number, yaw: number): Anchor => ({ x: x + origin.x, z: z + origin.z, yaw });
    this.anchors = { street: a(-1.5, 2040, Math.PI), gas: a(-12.5, 2070, Math.PI / 2), church: a(16.5, 2095, -Math.PI / 2) };

    mergeStatic(this.st); mergeStatic(this.car); mergeStatic(this.paper); mergeStatic(this.woman); mergeStatic(this.smoker);
    this.qMesh = this.quadMesh();
    this.group.add(this.qMesh, this.glow.build());
    this.setDeputy(true); this.setPaper(false); this.setCutout(false);
    this.group.updateMatrixWorld(true);
  }

  // ---------- materials ----------
  private mat(p: THREE.MeshStandardMaterialParameters, falloff = FALL) {
    const m = floodlit(new THREE.MeshStandardMaterial(p), falloff, townFlood); this.materials.add(m); return m;
  }
  private tex(w: number, h: number, draw: (g: CanvasRenderingContext2D) => void, repeat: [number, number]) {
    const c = document.createElement('canvas'); c.width = w; c.height = h; draw(c.getContext('2d')!);
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 4;
    t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(...repeat); return t;
  }
  private makeMaterials() {
    const speck = (g: CanvasRenderingContext2D, base: string, a: string, b: string, n: number, seed: number) => {
      g.fillStyle = base; g.fillRect(0, 0, 128, 128); let s = seed;
      for (let i = 0; i < n; i++) { s = (s * 1664525 + 1013904223) >>> 0; const x = s % 128; s = (s * 1664525 + 1013904223) >>> 0; g.fillStyle = i % 3 ? a : b; g.fillRect(x, s % 128, 1 + (i % 2), 1); }
    };
    // plaster with a trowelled grain, every 2 m; concrete block coursing; corrugated tin; pavement
    const plaster = this.tex(128, 128, (g) => { speck(g, '#bcbcbc', '#c8c8c8', '#a9a9a9', 2400, 412); g.fillStyle = 'rgba(0,0,0,.05)'; for (let y = 0; y < 128; y += 9) g.fillRect(0, y + (y % 4), 128, 3); }, [0.5, 0.5]);
    const block = this.tex(128, 128, (g) => {
      speck(g, '#b8b6b0', '#c4c2bb', '#a3a19b', 1800, 1420); g.fillStyle = '#8d8a84';
      for (let y = 0; y < 128; y += 32) { g.fillRect(0, y, 128, 2); for (let x = (y / 32) % 2 ? 32 : 0; x < 128; x += 64) g.fillRect(x, y, 2, 32); }
    }, [0.25, 0.5]);
    const tin = this.tex(64, 64, (g) => { for (let x = 0; x < 64; x++) { const k = 150 + 60 * Math.sin(x / 64 * Math.PI * 8); g.fillStyle = `rgb(${k},${k + 4},${k + 8})`; g.fillRect(x, 0, 1, 64); } g.fillStyle = 'rgba(90,60,40,.25)'; g.fillRect(0, 50, 64, 14); }, [1, 0.5]);
    const pave = this.tex(128, 128, (g) => { speck(g, '#9a958b', '#a7a296', '#827d74', 3000, 47); g.fillStyle = '#6f6b64'; g.fillRect(0, 0, 128, 1); g.fillRect(0, 0, 1, 128); }, [0.5, 0.5]);
    return {
      cream: this.mat({ color: 0xdcc9a6, map: plaster, roughness: .95 }),
      tan: this.mat({ color: 0xb98f68, map: plaster, roughness: 1 }),
      pink: this.mat({ color: 0xcfaa98, map: plaster, roughness: .95 }),
      white: this.mat({ color: 0xe6e2d8, map: plaster, roughness: .9 }),
      block: this.mat({ color: 0xc9c5bd, map: block, roughness: .95 }),
      tin: this.mat({ color: 0xa2a6aa, map: tin, roughness: .55, metalness: .35 }),
      roof: this.mat({ color: 0x3b3633, roughness: 1 }),
      trim: this.mat({ color: 0x5e4835, roughness: .8 }),
      metal: this.mat({ color: 0x9aa0a4, roughness: .45, metalness: .6 }),
      dark: this.mat({ color: 0x1e2225, roughness: .7 }),
      glass: this.mat({ color: 0x1b242c, roughness: .12, metalness: .35 }),
      concrete: this.mat({ color: 0xa29d93, map: pave, roughness: 1 }),
      pave: this.mat({ color: 0x9c978d, map: pave, roughness: 1, polygonOffset: true, polygonOffsetFactor: -1, polygonOffsetUnits: -2 }),
      red: this.mat({ color: 0xa8382c, roughness: .7 }),
      sign: this.mat({ map: this.atlas, roughness: .85 }),
      paint: this.mat({ color: 0xdedcd4, roughness: .35, metalness: .3 }),
      brown: this.mat({ color: 0x6e5236, roughness: .5, metalness: .2 }),
      wood: this.mat({ color: 0x6b4c33, roughness: .95 }),
      skin: this.mat({ color: 0xb8977b, roughness: 1, flatShading: true }),
      hair: this.mat({ color: 0x8a8580, roughness: 1, flatShading: true }),
      coat: this.mat({ color: 0x4a5560, roughness: 1, flatShading: true }),
      cloth: this.mat({ color: 0x5c4a3e, roughness: 1, flatShading: true }),
    };
  }

  // ---------- helpers (road-local metres; colliders, zones and lamps in world terms) ----------
  private gy(x: number, z: number) { return surfaceY(x, z); }
  private col(x0: number, x1: number, z0: number, z1: number, truck = true) {
    const o = this.origin, c: Box = { kind: 'box', minX: x0 + o.x, maxX: x1 + o.x, minZ: z0 + o.z, maxZ: z1 + o.z };
    this.colliders.push(c); if (truck) this.obstacles.push(c);
    return c;
  }
  private post(x: number, z: number, r = 0.2, truck = true) { return this.col(x - r, x + r, z - r, z + r, truck); }
  private zone(id: string, x0: number, x1: number, z0: number, z1: number) {
    const o = this.origin; this.zones.push({ id, enabled: true, minX: x0 + o.x, maxX: x1 + o.x, minZ: z0 + o.z, maxZ: z1 + o.z });
  }
  private light(id: string, x: number, y: number, z: number, w: number, c: THREE.ColorRepresentation) {
    const o = this.origin; this.slots[id] = addFlood(x + o.x, y + o.y, z + o.z, w, c, townFlood); return this.slots[id];
  }
  private proxy(id: string, w: number, h: number, d: number, x: number, y: number, z: number) {
    const g = new THREE.Group(); g.name = 'town:' + id; g.position.set(x, y, z); noMerge(g);
    const hit = box(g, w, h, d, this.hit); hit.layers.set(31); hit.name = 'town-hit:' + id;
    // the crosshair's ray looks at layer 0: the group hands it to its hidden box (Diner.proxy)
    g.raycast = (ray, hits) => hit.raycast(ray, hits);
    this.group.add(g); this.proxies[id] = g; return g;
  }
  /** A plane showing a region of the atlas (lit by the lamps like a painted board). */
  private board(parent: THREE.Object3D, r: Rect, w: number, h: number, x: number, y: number, z: number, ry: number, rx = 0) {
    const f = plane(parent, w, h, this.m.sign, x, y, z, ry, rx), uv = f.geometry.attributes.uv;
    for (let i = 0; i < uv.count; i++) uv.setXY(i, (r[0] + uv.getX(i) * r[2]) / AW, 1 - (r[1] + (1 - uv.getY(i)) * r[3]) / AH);
    return f;
  }
  /** A lit quad of the atlas (a window, a sign, a lens): centre c, half-vectors a (to the
   *  viewer's right) and b (up). k: how bright to begin with. */
  private quad(id: PanelId, r: Rect, c: THREE.Vector3, a: THREE.Vector3, b: THREE.Vector3, color: THREE.ColorRepresentation = 0xffffff, k = 1) {
    const at = this.qPos.length / 3, col = new THREE.Color(color);
    const u0 = r[0] / AW, u1 = (r[0] + r[2]) / AW, v0 = 1 - (r[1] + r[3]) / AH, v1 = 1 - r[1] / AH;
    for (const [sa, sb, u, v] of [[-1, -1, u0, v0], [1, -1, u1, v0], [1, 1, u1, v1], [-1, 1, u0, v1]]) {
      this.qPos.push(c.x + a.x * sa + b.x * sb, c.y + a.y * sa + b.y * sb, c.z + a.z * sa + b.z * sb);
      this.qUv.push(u, v); this.qCol.push(col.r * k, col.g * k, col.b * k);
    }
    let p = this.panels.get(id);
    if (!p) { p = { at: [], c: col, k }; this.panels.set(id, p); }
    p.at.push(at);
    return at;
  }
  /** A lit quad on a building's front (facing out of it, s: +1 east, -1 west). */
  private frontQuad(b: Built, id: PanelId, r: Rect, z: number, y: number, w: number, h: number, color: THREE.ColorRepresentation = 0xffffff, k = 1) {
    return this.quad(id, r, V(b.fx + b.s * 0.035, b.floor + y, z), V(0, 0, -b.s * w / 2), V(0, h / 2, 0), color, k);
  }
  private quadMesh() {
    const g = new THREE.BufferGeometry(), n = this.qPos.length / 3, idx: number[] = [];
    for (let i = 0; i < n; i += 4) idx.push(i, i + 1, i + 2, i, i + 2, i + 3);
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.qPos, 3));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(this.qUv, 2));
    g.setAttribute('color', new THREE.Float32BufferAttribute(this.qCol, 3));
    g.setIndex(idx);
    const m = new THREE.MeshBasicMaterial({ map: this.atlas, vertexColors: true, side: THREE.DoubleSide, fog: true });
    m.name = 'town/lit'; this.materials.add(m);
    const mesh = new THREE.Mesh(g, m); mesh.name = 'town-lit'; mesh.frustumCulled = false; noMerge(mesh);
    return mesh;
  }
  /** How bright a panel is now (0 off, 1 as made). */
  private setPanel(id: PanelId, k: number, only?: number) {
    const p = this.panels.get(id); if (!p) return;
    const a = this.qMesh?.geometry.getAttribute('color') as THREE.BufferAttribute | undefined;
    for (const at of p.at) {
      if (only !== undefined && at !== only) continue;
      for (let i = 0; i < 4; i++) {
        if (a) a.setXYZ(at + i, p.c.r * k, p.c.g * k, p.c.b * k);
        else { const j = (at + i) * 3; this.qCol[j] = p.c.r * k; this.qCol[j + 1] = p.c.g * k; this.qCol[j + 2] = p.c.b * k; }
      }
    }
    this.dirty = true;
  }
  private glowColor(i: number, c: THREE.ColorRepresentation) {
    const a = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute | undefined;
    if (!a) return; const k = new THREE.Color(c); a.setXYZ(i, k.r, k.g, k.b); a.needsUpdate = true;
  }
  /** Ground draped with pavement (lift metres above the land, which it follows). */
  private drape(x0: number, x1: number, z0: number, z1: number, lift = 0.05, mat = this.m.pave, step = 2) {
    const nx = Math.max(1, Math.ceil((x1 - x0) / step)), nz = Math.max(1, Math.ceil((z1 - z0) / step));
    const pos: number[] = [], uv: number[] = [], idx: number[] = [];
    for (let j = 0; j <= nz; j++) for (let i = 0; i <= nx; i++) {
      const x = x0 + (x1 - x0) * i / nx, z = z0 + (z1 - z0) * j / nz;
      pos.push(x, this.gy(x, z) + lift, z); uv.push(x, z);
    }
    for (let j = 0; j < nz; j++) for (let i = 0; i < nx; i++) { const a = j * (nx + 1) + i, c = a + nx + 1; idx.push(a, c, a + 1, a + 1, c, c + 1); }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3)); g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
    g.setIndex(idx); g.computeVertexNormals();
    this.st.add(new THREE.Mesh(g, mat));
  }
  /** A plain building on its own footing: the floor a little above the highest ground under it,
   *  the walls down below the lowest, a dark roof and a lip round it. s: the way its front faces. */
  private building(x0: number, x1: number, z0: number, z1: number, h: number, mat: THREE.Material, s: 1 | -1, lip = 0.35): Built {
    let lo = Infinity, hi = -Infinity;
    for (const x of [x0, (x0 + x1) / 2, x1]) for (const z of [z0, (z0 + z1) / 2, z1]) { const y = this.gy(x, z); lo = Math.min(lo, y); hi = Math.max(hi, y); }
    const floor = hi + 0.15, base = lo - 0.5, top = floor + h, cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
    metreUV(box(this.st, x1 - x0, top - base, z1 - z0, mat, cx, (base + top) / 2, cz));
    box(this.st, x1 - x0 - 0.4, 0.06, z1 - z0 - 0.4, this.m.roof, cx, top - 0.02 + 0.03, cz);
    metreUV(box(this.st, x1 - x0 + 0.08, lip, 0.25, mat, cx, top + lip / 2, z0 + 0.125));
    metreUV(box(this.st, x1 - x0 + 0.08, lip, 0.25, mat, cx, top + lip / 2, z1 - 0.125));
    metreUV(box(this.st, 0.25, lip, z1 - z0, mat, x0 + 0.125, top + lip / 2, cz));
    metreUV(box(this.st, 0.25, lip, z1 - z0, mat, x1 - 0.125, top + lip / 2, cz));
    // a concrete band where the wall meets the ground
    box(this.st, x1 - x0 + 0.06, floor - base, z1 - z0 + 0.06, this.m.concrete, cx, (base + floor) / 2, cz);
    this.col(x0, x1, z0, z1);
    return { x0, x1, z0, z1, floor, top, fx: s > 0 ? x1 : x0, s };
  }
  /** A window or door frame on a front: four bars round w x h at (z, y). */
  private frame(b: Built, z: number, y: number, w: number, h: number, mat = this.m.metal, t = 0.07) {
    const x = b.fx + b.s * 0.04, fy = b.floor + y;
    box(this.st, 0.08, t, w + t * 2, mat, x, fy + h / 2 + t / 2, z); box(this.st, 0.08, t, w + t * 2, mat, x, fy - h / 2 - t / 2, z);
    box(this.st, 0.08, h, t, mat, x, fy, z - w / 2 - t / 2); box(this.st, 0.08, h, t, mat, x, fy, z + w / 2 + t / 2);
  }
  /** A dark pane, or a door, flat on a front. */
  private pane(b: Built, z: number, y: number, w: number, h: number, mat = this.m.glass) {
    return plane(this.st, w, h, mat, b.fx + b.s * 0.025, b.floor + y, z, b.s * Math.PI / 2);
  }
  /** A step up to a door from the ground in front. */
  private stoop(b: Built, z: number, w: number) {
    const x = b.fx + b.s * 0.45, g = this.gy(x, z);
    box(this.st, 0.9, b.floor - g + 0.3, w, this.m.concrete, x, (g - 0.3 + b.floor) / 2, z);
    this.col(Math.min(b.fx, x + b.s * .45), Math.max(b.fx, x + b.s * .45), z - w / 2, z + w / 2, false);
  }

  // ---------- the town's signs at either end ----------
  private townSigns() {
    for (const [x, z, n] of [[-8.6, 1880, -1], [8.6, 2335, 1]] as const) {
      const y = this.gy(x, z);
      for (const dx of [-1.4, 1.4]) { box(this.st, .1, 3.2, .1, this.m.metal, x + dx, y + 1.6, z); this.post(x + dx, z, .15); }
      box(this.st, 3.7, 1.9, .06, this.m.dark, x, y + 2.15, z);
      this.board(this.st, R.caliche, 3.6, 1.8, x, y + 2.15, z + n * .035, n > 0 ? 0 : Math.PI);
    }
  }

  // ---------- the crossing: a light on a span wire over the highway, blinking yellow ----------
  private crossing() {
    const m = this.m, w0 = V(-8.6, 0, 2019), w1 = V(9.6, 0, 1981);
    for (const p of [w0, w1]) { p.y = this.gy(p.x, p.z); cyl(this.st, .13, .17, 8, m.wood, p.x, p.y + 4, p.z, 7); this.post(p.x, p.z, .25); }
    const top = (p: THREE.Vector3) => V(p.x, p.y + 7.6, p.z), mid = top(w0).lerp(top(w1), 0.5);
    mid.y -= 0.35;   // the wire sags
    rod(this.st, top(w0), mid, .012, m.dark, 4); rod(this.st, mid, top(w1), .012, m.dark, 4);
    const hy = mid.y - 0.85;
    rod(this.st, mid, V(mid.x, hy + 0.55, mid.z), .02, m.dark, 4);
    box(this.st, .42, 1.1, .42, m.roof, mid.x, hy, mid.z);
    // one lens on each side, yellow; they blink together (update)
    for (const [nx, nz] of [[0, -1], [0, 1], [1, 0], [-1, 0]]) {
      const c = V(mid.x + nx * .215, hy + .22, mid.z + nz * .215);
      this.quad('blink', R.lens, c, V(nz * .14, 0, -nx * .14), V(0, .14, 0), 0xffc020, 1.2);
      box(this.st, nx ? .04 : .34, .06, nz ? .04 : .34, m.dark, c.x + nx * .03, c.y + .16, c.z + nz * .03);
    }
    this.light('blink', mid.x, hy - .2, mid.z, 5, 0xffb420);
    this.blinkGlow.push(this.glow.add(mid.x, hy + .22, mid.z, 1.6, 0xffb420), this.glow.add(mid.x, hy + .22, mid.z, 7, 0x2a1a04));
  }

  // ---------- cobra-head street lamps down the west side; they go off one by one at dawn ----------
  private streetLamps() {
    const m = this.m;
    LAMPS.forEach((z, i) => {
      const y = this.gy(LAMP_X, z), hy = y + 7.7;
      cyl(this.st, .09, .14, 7.9, m.metal, LAMP_X, y + 3.95, z, 7);
      rod(this.st, V(LAMP_X, hy - .3, z), V(ARM_X + .5, hy, z), .05, m.metal, 5);
      box(this.st, .95, .2, .42, m.metal, ARM_X, hy - .02, z);
      const quad = this.quad('lamps', R.lens, V(ARM_X, hy - .125, z), V(.36, 0, 0), V(0, 0, .15), 0xffb860, 1.3);
      this.post(LAMP_X, z, .2);
      const slot = addFlood(ARM_X + this.origin.x, hy - .4 + this.origin.y, z + this.origin.z, 13, SODIUM, townFlood);
      const glow = [this.glow.add(ARM_X, hy - .2, z, 2.2, 0xffb060), this.glow.add(ARM_X, hy - .6, z, 13, 0x2c1806)];
      // photocells: the first at 05:05, the last near half past, in no order
      const off = 5 * 3600 + 5 * 60 + [0, 9, 3, 14, 6, 21, 11][i] * 60 + Math.floor(hash(z, 7) * 50);
      this.lamps.push({ z, slot, glow, quad, off, on: true });
    });
  }

  // ---------- sidewalks and lots: concrete laid on the land ----------
  private ground() {
    this.drape(-17.6, -14.2, 2016, 2040);
    this.drape(-17.6, -14.2, 2090, 2250);
    this.drape(16.6, 20.2, 2024, 2200);
  }

  // ---------- the gas station: a canopy over two pumps, the office with its window lit all night ----------
  private gasStation() {
    const m = this.m;
    this.drape(-17.6, -5.6, 2040, 2090, 0.055);
    const office = this.building(-27, -17, 2066, 2078, 3.6, m.cream, 1);
    const bay = this.building(-27, -17.4, 2078, 2088, 4.3, m.cream, 1);
    // office front: the door with the poster in it, the window with the counter behind
    this.pane(office, 2067.2, 1.12, 1.1, 2.2, m.glass); this.frame(office, 2067.2, 1.12, 1.1, 2.2); this.stoop(office, 2067.2, 1.6);
    this.board(this.st, R.poster, .46, .46, office.fx + .045, office.floor + 1.35, 2067.2, Math.PI / 2);
    this.frontQuad(office, 'gasWin', R.inGas, 2073, 1.55, 4.6, 1.75, 0xffe2b8, .85); this.frame(office, 2073, 1.55, 4.6, 1.75);
    box(this.st, .5, .08, 9.4, m.red, office.fx + .25, office.floor + 3.45, 2072);   // the red band along the top
    // the service bay: a roll-up door, closed
    plane(this.st, 3.6, 3.1, m.tin, bay.fx + .03, bay.floor + 1.55, 2083, Math.PI / 2); this.frame(bay, 2083, 1.55, 3.6, 3.1, m.trim, .12);
    this.stoop(bay, 2083, 3.6);
    // the canopy over the pump island, lit underneath
    const iy = this.gy(-10.25, 2055), cy = iy + 4.5;
    box(this.st, 1.5, .2, 16, m.concrete, -10.25, iy + .1, 2055); this.col(-11, -9.5, 2047, 2063);
    box(this.st, 7.6, .45, 18.4, m.white, -10.25, cy + .22, 2055);
    box(this.st, 7.7, .32, 18.5, m.red, -10.25, cy + .2, 2055);
    for (const z of [2049, 2061]) { box(this.st, .3, cy - iy, .3, m.white, -10.25, (iy + cy) / 2, z); }
    for (const z of [2052.4, 2057.6]) {
      box(this.st, .55, 1.55, .8, m.paint, -10.25, iy + .2 + .78, z); box(this.st, .6, .12, .85, m.red, -10.25, iy + .2 + 1.6, z);
      for (const s of [-1, 1]) this.quad('pumps', R.pump, V(-10.25 + s * .28, iy + 1.25, z), V(0, 0, -s * .3), V(0, .3, 0), 0xffffff, .55);
      rod(this.st, V(-9.95, iy + 1.1, z + .38), V(-9.75, iy + .45, z + .5), .02, m.dark, 4);
    }
    this.proxy('pumps', 1.6, 1.8, 7, -10.25, iy + 1.1, 2055);
    this.quad('canopy', R.white, V(-10.25, cy - .01, 2052), V(.5, 0, 0), V(0, 0, 1.6), 0xf4f1e0, 1);
    this.quad('canopy', R.white, V(-10.25, cy - .01, 2058), V(.5, 0, 0), V(0, 0, 1.6), 0xf4f1e0, 1);
    this.light('canopy', -10.25, cy - .5, 2055, 11, 0xf0eedf);
    this.light('gasWin', -15.6, office.floor + 1.6, 2073, 4, 0xffcf96);
    this.glow.add(-10.25, cy - .1, 2052, 1.6, 0xeceadc); this.glow.add(-10.25, cy - .1, 2058, 1.6, 0xeceadc);
    this.glow.add(-10.25, cy - 1, 2055, 16, 0x1a1a14);
    this.glow.add(office.fx + .4, office.floor + 1.5, 2073, 4, 0x2a1c0e);
    // the price sign on its pole, square to the highway
    const py = this.gy(-15.2, 2042);
    cyl(this.st, .14, .16, 7.2, m.metal, -15.2, py + 3.6, 2042, 8); this.post(-15.2, 2042, .25);
    box(this.st, 3.2, 1.7, .3, m.dark, -15.2, py + 6.5, 2042);
    for (const n of [-1, 1]) this.quad('gasSign', R.gas, V(-15.2, py + 6.5, 2042 + n * .16), V(n * 1.5, 0, 0), V(0, .78, 0), 0xffffff, .95);
    this.glow.add(-15.2, py + 6.5, 2042, 6, 0x2a1810);
    // the payphone on a post at the corner of the office
    const ty = this.gy(-15.6, 2064.6);
    box(this.st, .12, 2.3, .12, m.metal, -15.95, ty + 1.15, 2064.6);
    box(this.st, .45, .65, .6, m.metal, -15.6, ty + 1.45, 2064.6); box(this.st, .05, .5, .48, m.dark, -15.36, ty + 1.45, 2064.6);
    this.board(this.st, R.phone, .7, .18, -15.33, ty + 2.02, 2064.6, Math.PI / 2);
    box(this.st, .08, .24, .07, m.dark, -15.3, ty + 1.55, 2064.42);
    this.col(-16.1, -15.3, 2064.2, 2065);
    this.proxy('payphone', .7, 1.2, .8, -15.5, ty + 1.4, 2064.6);
    this.proxy('gasDoor', .3, 2.2, 1.3, office.fx + .1, office.floor + 1.1, 2067.2);
    this.proxy('gasWindow', .3, 1.8, 4.6, office.fx + .1, office.floor + 1.55, 2073);
    this.proxy('gasSign', .6, 2, 3.4, -15.2, py + 1.2, 2042);
  }

  // ---------- the sheriff's substation: the map with the pins, seen through the window ----------
  private sheriff() {
    const m = this.m, b = this.building(-26, -16, 2100, 2112, 3.5, m.tan, 1);
    this.pane(b, 2101.6, 1.1, 1.0, 2.15, m.trim); this.frame(b, 2101.6, 1.1, 1.0, 2.15, m.dark); this.stoop(b, 2101.6, 1.5);
    this.frontQuad(b, 'sheriffWin', R.inSheriff, 2107.3, 1.6, 3.6, 1.6, 0xfff0d8, .85); this.frame(b, 2107.3, 1.6, 3.6, 1.6, m.trim);
    for (let i = 0; i < 6; i++) box(this.st, .03, 1.6, .03, m.dark, b.fx + .07, b.floor + 1.6, 2105.8 + i * .6);   // the bars
    this.board(this.st, R.sheriff, 3.4, .85, b.fx + .04, b.floor + 2.95, 2105.4, Math.PI / 2);
    this.light('sheriffWin', -15.4, b.floor + 1.7, 2107.3, 4.5, 0xfff0d0);
    this.slots.sheriffGlow = this.glow.add(b.fx + .4, b.floor + 1.6, 2107.3, 4, 0x2a2618);
    // the flagpole, the flag taken in for the night
    const fy = this.gy(-14.6, 2099.2);
    cyl(this.st, .05, .07, 7, m.metal, -14.6, fy + 3.5, 2099.2, 6); this.post(-14.6, 2099.2, .12);
    this.proxy('sheriffWindow', .3, 1.7, 3.8, b.fx + .1, b.floor + 1.6, 2107.3);
    this.proxy('sheriffDoor', .3, 2.2, 1.2, b.fx + .1, b.floor + 1.1, 2101.6);
  }

  // ---------- the laundromat: one tube on all night, and a bench outside ----------
  private laundromat() {
    const m = this.m, b = this.building(-25, -15, 2126, 2140, 3.4, m.block, 1);
    for (const z of [2128.9, 2132.3]) { this.frontQuad(b, 'laundroWin', R.inLaundro, z, 1.45, 3.1, 1.7, 0xe8f2ff, .8); this.frame(b, z, 1.45, 3.1, 1.7); }
    this.frontQuad({ ...b, fx: b.fx + .012 }, 'laundroWin', R.open24, 2131.9, .85, .9, .22, 0xffffff, 1);
    this.pane(b, 2137.4, 1.1, 1.1, 2.2, m.glass); this.frame(b, 2137.4, 1.1, 1.1, 2.2); this.stoop(b, 2137.4, 1.6);
    this.frontQuad({ ...b, fx: b.fx + .28 }, 'laundro', R.laundro, 2131, 2.85, 4.2, 1.05, 0xffffff, .95);
    box(this.st, .3, 1.2, 4.6, m.dark, b.fx + .12, b.floor + 2.85, 2131);
    this.light('laundro', -14.6, b.floor + 2.2, 2131, 6, 0xdcecff);
    this.glow.add(b.fx + .3, b.floor + 1.5, 2130.6, 5, 0x18202a);
    // the bench under the window
    const x = b.fx + .55, y = this.gy(x, 2132);
    for (const z of [2131, 2133.2]) box(this.st, .45, .42, .06, m.metal, x, y + .21, z);
    for (let i = 0; i < 3; i++) box(this.st, .12, .04, 2.6, m.wood, x - .14 + i * .14, y + .44, 2132.1);
    box(this.st, .04, .32, 2.6, m.wood, x - .24, y + .66, 2132.1);
    this.col(x - .3, x + .28, 2130.75, 2133.45, false);
    this.proxy('bench', .6, .8, 2.7, x, y + .45, 2132.1);
    this.proxy('laundromat', .3, 1.9, 6.8, b.fx + .1, b.floor + 1.45, 2130.6);
  }

  // ---------- the Plaza: a picture house, closed, its marquee dark ----------
  private cinema() {
    const m = this.m, b = this.building(-29, -15, 2152, 2172, 6.2, m.pink, 1, .5);
    // a stepped top over the front, with the name on a blade sign
    metreUV(box(this.st, .6, 1.4, 8, m.pink, b.fx - .3, b.top + .7, 2162));
    const fy = b.floor, mx = b.fx + 1.25;
    box(this.st, 2.5, 1.15, 9.8, m.dark, mx, fy + 3.55, 2162);
    box(this.st, 2.56, .14, 9.86, m.metal, mx, fy + 4.18, 2162); box(this.st, 2.56, .1, 9.86, m.metal, mx, fy + 2.96, 2162);
    this.board(this.st, R.marquee, 9.6, 1.0, mx + 1.27, fy + 3.55, 2162, Math.PI / 2);
    for (const n of [-1, 1]) this.board(this.st, R.white, 2.4, 1.0, mx, fy + 3.55, 2162 + n * 4.92, n > 0 ? 0 : Math.PI);
    for (let i = 0; i < 12; i++) box(this.st, .1, .06, .1, m.white, mx + 1.2, fy + 2.9, 2157.4 + i * .84);   // the bulbs, out
    box(this.st, .9, 3.4, .22, m.dark, b.fx + .45, fy + 6.1, 2162);
    for (const n of [-1, 1]) this.board(this.st, R.blade, .8, 3.2, b.fx + .48, fy + 6.1, 2162 + n * .115, n > 0 ? 0 : Math.PI);
    // under the marquee: the doors, dark, and a box office with its blind down
    for (const z of [2159.4, 2161, 2164.6]) { this.pane(b, z, 1.15, 1.3, 2.3, m.glass); this.frame(b, z, 1.15, 1.3, 2.3); }
    box(this.st, .9, 2.4, 1.4, m.trim, b.fx + .45, fy + 1.2, 2162.8); this.col(b.fx, b.fx + .9, 2162.1, 2163.5);
    this.board(this.st, R.closed, .5, .25, b.fx + .91, fy + 1.4, 2162.8, Math.PI / 2);
    this.proxy('cinema', .5, 2.6, 9.6, b.fx + .3, fy + 1.4, 2162);
    this.proxy('marquee', 2.6, 1.2, 9.8, mx, fy + 3.55, 2162);
  }

  // ---------- the feed store: a tin shed with a loading dock ----------
  private feedStore() {
    const m = this.m, b = this.building(-31, -15, 2190, 2214, 5.2, m.tin, 1, .2);
    this.board(this.st, R.feed, 6.4, 1.6, b.fx + .04, b.floor + 4.1, 2202, Math.PI / 2);
    plane(this.st, 5.2, 3.6, m.trim, b.fx + .03, b.floor + 1.8, 2201.5, Math.PI / 2); this.frame(b, 2201.5, 1.8, 5.2, 3.6, m.trim, .15);
    // the dock, sacks on a pallet, a bulb over the door
    const dy = this.gy(-13.7, 2201);
    box(this.st, 2.6, b.floor + .95 - dy + .3, 10, m.concrete, b.fx + 1.3, (dy - .3 + b.floor + .95) / 2, 2201);
    this.col(b.fx, b.fx + 2.6, 2196, 2206);
    box(this.st, 1.2, .14, 1.2, m.wood, b.fx + 1.4, b.floor + 1.02, 2204.4);
    for (let i = 0; i < 5; i++) box(this.st, .55, .22, .85, m.tan, b.fx + 1.1 + (i % 2) * .58, b.floor + 1.2 + Math.floor(i / 2) * .23, 2204.1 + (i % 2) * .1);
    box(this.st, .3, .12, .3, m.dark, b.fx + .2, b.floor + 4.0, 2201.5);
    this.glow.add(b.fx + .25, b.floor + 3.85, 2201.5, .5, 0xffd08a); this.glow.add(b.fx + .5, b.floor + 3.6, 2201.5, 4, 0x24180a);
    this.proxy('feed', 2.8, 1.6, 10, b.fx + 1.3, b.floor + 1.2, 2201);
  }

  // ---------- 1420 AM, the Night Line: a block house, the studio behind glass, the mast ----------
  private station1420() {
    const m = this.m, b = this.building(20, 30, 2030, 2042, 3.3, m.block, -1);
    this.pane(b, 2031.4, 1.1, 1.0, 2.15, m.trim); this.frame(b, 2031.4, 1.1, 1.0, 2.15, m.dark); this.stoop(b, 2031.4, 1.4);
    this.frontQuad(b, 'studio', R.inStudio, 2037.2, 1.55, 3.7, 1.65, 0xffe6c4, .8); this.frame(b, 2037.2, 1.55, 3.7, 1.65);
    this.frontQuad({ ...b, fx: b.fx - .012 }, 'onAir', R.onAir, 2038.4, 2.1, .5, .22, 0xffffff, 1.1);
    // the sign on the roof's edge, lit
    for (const z of [2033.6, 2038.6]) box(this.st, .08, 1.3, .08, m.metal, b.fx + .3, b.top + .65, z);
    box(this.st, .12, 1.7, 5.4, m.dark, b.fx + .3, b.top + 1.05, 2036.1);
    this.frontQuad({ ...b, fx: b.fx + .24, floor: b.top }, 'amSign', R.am, 2036.1, 1.05, 3.3, 1.6, 0xffffff, .95);
    this.light('studio', 19.4, b.floor + 1.6, 2037.2, 4, 0xffd8a8);
    this.glow.add(b.fx - .4, b.floor + 1.6, 2037.2, 4, 0x2a1c10); this.glow.add(b.fx - .3, b.top + 1.05, 2036.1, 6, 0x221812);
    // the mast: three legs and their bracing, guyed three ways, a red lamp at the top and halfway
    const mx = 38, mz = 2036, my = this.gy(mx, mz), H = 36, r = .32;
    const legs = [0, 1, 2].map((i) => V(mx + Math.cos(i * 2.094) * r, 0, mz + Math.sin(i * 2.094) * r));
    for (const l of legs) rod(this.st, V(l.x, my, l.z), V(l.x, my + H, l.z), .035, m.red, 4);
    for (let y = 1.5; y < H; y += 1.5) for (let i = 0; i < 3; i++) { const a = legs[i], c = legs[(i + 1) % 3]; rod(this.st, V(a.x, my + y - 1.5, a.z), V(c.x, my + y, c.z), .012, m.metal, 3); }
    box(this.st, 1.2, .3, 1.2, m.concrete, mx, my + .05, mz);
    for (let i = 0; i < 3; i++) {
      const a = i * 2.094 + .5, ax = mx + Math.cos(a) * 20, az = mz + Math.sin(a) * 20, ay = this.gy(ax, az);
      for (const y of [H * .45, H * .85]) rod(this.st, V(mx, my + y, mz), V(ax, ay + .3, az), .008, m.dark, 3);
      box(this.st, .5, .4, .5, m.concrete, ax, ay + .1, az);
    }
    this.col(mx - .8, mx + .8, mz - .8, mz + .8);
    for (const y of [H + .2, H * .5]) { this.glow.add(mx, my + y, mz, 1.4, 0xff2010, 1.3, 0); this.glow.add(mx, my + y, mz, 6, 0x300402, 1.3, 0); }
    this.proxy('studio', .3, 1.8, 3.9, b.fx - .1, b.floor + 1.55, 2037.2);
    this.proxy('amDoor', .3, 2.2, 1.2, b.fx - .1, b.floor + 1.1, 2031.4);
  }

  // ---------- the souvenir shop: a hand-painted sign, a cardboard alien in the window ----------
  private souvenirs() {
    const m = this.m, b = this.building(20.5, 29.5, 2058, 2070, 3.7, m.tan, -1);
    // a portal on peeled posts along the front, the sign standing on its roof
    const py = b.floor + 2.75;
    box(this.st, 2.2, .18, 12.4, m.wood, b.fx - 1.1, py, 2064);
    for (let i = 0; i < 9; i++) cyl(this.st, .07, .07, 2.3, m.wood, b.fx - 1.1, py - .05, 2058.2 + i * 1.45, 6).rotation.set(0, 0, Math.PI / 2);
    for (const z of [2058.4, 2064, 2069.6]) { const y = this.gy(18.6, z); cyl(this.st, .1, .12, py - y, m.wood, 18.6, (y + py) / 2, z, 7); this.post(18.6, z, .15); }
    for (const z of [2063, 2065.2]) box(this.st, .1, 1.2, .1, m.wood, 18.8, py + .6, z);
    this.board(this.st, R.souvenir, 2.6, 1.3, 18.7, py + .85, 2064.1, -Math.PI / 2);
    this.frontQuad(b, 'display', R.display, 2062.3, 1.45, 2.2, 1.7, 0xffe0b0, .35); this.frame(b, 2062.3, 1.45, 2.2, 1.7, m.trim);
    this.pane(b, 2066.8, 1.1, 1.0, 2.15, m.trim); this.frame(b, 2066.8, 1.1, 1.0, 2.15, m.wood); this.stoop(b, 2066.8, 1.4);
    this.board(this.st, R.closed, .42, .2, b.fx - .045, b.floor + 1.5, 2066.8, -Math.PI / 2);
    this.glow.add(b.fx - .3, b.floor + 1.4, 2062.3, 2.4, 0x1a120a);
    this.proxy('souvenir', .3, 1.8, 2.4, b.fx - .1, b.floor + 1.45, 2062.3);
    this.proxy('souvenirSign', .3, 1.4, 2.8, 18.7, py + .85, 2064.1);
    this.proxy('souvenirDoor', .3, 2.2, 1.2, b.fx - .1, b.floor + 1.1, 2066.8);
  }

  // ---------- the church: white, a bell gable over the door, the letter board by the street ----------
  private church() {
    const m = this.m, b = this.building(21, 35, 2091, 2100, 5.4, m.white, -1, .3);
    // the bell gable: a wall over the front with an opening, a bell, a cross
    const gx = b.fx + .2;
    metreUV(box(this.st, .4, 2.8, 4.2, m.white, gx, b.top + 1.4, 2095.5));
    box(this.st, .42, 1.3, 1.2, m.roof, gx, b.top + 1.35, 2095.5);
    cyl(this.st, .22, .32, .5, m.brown, gx, b.top + 1.25, 2095.5, 10);
    box(this.st, .1, 1, .1, m.wood, gx, b.top + 3.3, 2095.5); box(this.st, .1, .1, .6, m.wood, gx, b.top + 3.5, 2095.5);
    this.pane(b, 2095.5, 1.45, 2.2, 2.9, m.wood); this.frame(b, 2095.5, 1.45, 2.2, 2.9, m.trim, .12); this.stoop(b, 2095.5, 3);
    box(this.st, .1, 2.9, .04, m.dark, b.fx - .05, b.floor + 1.45, 2095.5);
    for (const z of [2092.6, 2098.4]) { this.pane(b, z, 3.2, .55, 1.5); this.frame(b, z, 3.2, .55, 1.5, m.trim); }
    // the letter board on two posts, a bulb over it
    const x = 18.9, y = this.gy(x, 2087.3);
    for (const z of [2086, 2088.6]) { box(this.st, .12, 1.95, .12, m.wood, x, y + .97, z); this.post(x, z, .12); }
    box(this.st, .14, .95, 2.75, m.wood, x, y + 1.4, 2087.3);
    this.board(this.st, R.board, 2.5, .78, x - .075, y + 1.4, 2087.3, -Math.PI / 2);
    box(this.st, .3, .05, .05, m.metal, x - .15, y + 1.95, 2087.3); box(this.st, .1, .1, .1, m.dark, x - .3, y + 1.92, 2087.3);
    this.light('board', x - .6, y + 2.2, 2087.3, 3, 0xffdca0);
    this.glow.add(x - .3, y + 1.88, 2087.3, .35, 0xffe0a0); this.glow.add(x - .4, y + 1.6, 2087.3, 3, 0x1e160a);
    this.proxy('letters', .3, 1, 2.8, x - .1, y + 1.4, 2087.3);
    this.proxy('churchDoor', .3, 3, 2.4, b.fx - .1, b.floor + 1.45, 2095.5);
  }

  // ---------- houses, set back on the east side; a television on in two of them ----------
  private houses() {
    const m = this.m;
    const house = (x0: number, x1: number, z0: number, z1: number, mat: THREE.Material, s: 1 | -1, win: PanelId | null, wz: number) => {
      const b = this.building(x0, x1, z0, z1, 3.0, mat, s, .3);
      const dz = wz > (z0 + z1) / 2 ? z0 + 1.6 : z1 - 1.6;
      this.pane(b, dz, 1.05, .95, 2.1, m.trim); this.frame(b, dz, 1.05, .95, 2.1, m.wood, .08); this.stoop(b, dz, 1.3);
      if (win) this.frontQuad(b, win, win === 'warm' ? R.inWarm : R.inTv, wz, 1.5, 1.5, 1.15, win === 'warm' ? 0xffd8a8 : 0xc8d8ff, .7);
      else this.pane(b, wz, 1.5, 1.5, 1.15);
      this.frame(b, wz, 1.5, 1.5, 1.15, m.wood, .08);
      // the mailbox at the street
      const mx = s < 0 ? 18.7 : -17.3, my = this.gy(mx, wz);
      box(this.st, .08, 1.05, .08, m.wood, mx, my + .52, wz); box(this.st, .45, .22, .25, m.metal, mx, my + 1.1, wz);
      this.post(mx, wz, .12, false);
      return b;
    };
    const h1 = house(24, 33, 2128, 2138, m.tan, -1, 'tv1', 2131.2);
    house(24.5, 32, 2152, 2161, m.cream, -1, null, 2155.2);
    const h3 = house(25, 34, 2180, 2190, m.tan, -1, 'tv2', 2186.6);
    const hw = house(-28, -19, 2236, 2246, m.cream, 1, 'warm', 2239.2);
    this.light('tv1', h1.fx - .5, h1.floor + 1.5, 2131.2, 3, 0x9db6ff);
    this.light('tv2', h3.fx - .5, h3.floor + 1.5, 2186.6, 3, 0x9db6ff);
    this.slots.tv1Glow = this.glow.add(h1.fx - .3, h1.floor + 1.5, 2131.2, 2.4, 0x101828);
    this.glow.add(h3.fx - .3, h3.floor + 1.5, 2186.6, 2.4, 0x101828);
    this.glow.add(hw.fx + .3, hw.floor + 1.5, 2239.2, 2.2, 0x1e140a);
    // a porch light at the house with the lit kitchen
    this.glow.add(hw.fx + .3, hw.floor + 2.4, 2244.4, .45, 0xffd890);
    this.proxy('tv1', .3, 1.3, 1.7, h1.fx - .1, h1.floor + 1.5, 2131.2);
    this.proxy('tv2', .3, 1.3, 1.7, h3.fx - .1, h3.floor + 1.5, 2186.6);
    this.proxy('kitchen', .3, 1.3, 1.7, hw.fx + .1, hw.floor + 1.5, 2239.2);
  }

  // ---------- the telephone line along the east side (the road draws the poles, roadProps.poles) ----------
  private poles() {
    for (let z = -1900; z <= 3400; z += 45) {
      const pz = z + (hash(z, 1) - 0.5) * 4;
      if (pz > 1976 && pz < 2258) this.post(15, pz, .22);
    }
  }

  // ---------- the deputy's car, out front while he is in ----------
  private patrolCar() {
    const g = this.car, m = this.m, x = -12.3, z = 2106.5, y = this.gy(x, z);
    g.position.set(x, y, z);
    box(g, 1.9, .62, 5.3, m.paint, 0, .62, 0); box(g, 1.94, .3, 2.2, m.brown, 0, .62, .2);
    box(g, 1.62, .5, 2.5, m.paint, 0, 1.17, .35); box(g, 1.64, .4, 2.3, m.glass, 0, 1.15, .35);
    box(g, 1.25, .13, .3, m.dark, 0, 1.49, .2); box(g, .5, .1, .28, m.red, -.3, 1.5, .2);
    for (const [wx, wz] of [[-.86, -1.6], [.86, -1.6], [-.86, 1.65], [.86, 1.65]]) {
      const w = cyl(g, .34, .34, .24, m.dark, wx, .34, wz, 10); w.rotation.z = Math.PI / 2;
    }
    box(g, 1.92, .12, .12, m.metal, 0, .5, -2.68); box(g, 1.92, .12, .12, m.metal, 0, .5, 2.68);
    this.carCol = this.col(x - 1, x + 1, z - 2.7, z + 2.7);
    this.proxy('patrolCar', 2, 1.6, 5.4, x, y + .8, z);
    // the driver's door, open (shown only out on the shoulder; outside the merge)
    const door = this.carDoor; door.position.set(-.95, .35, -.55); door.rotation.y = -1.0; noMerge(door);
    box(door, .06, .6, 1.1, m.paint, 0, .32, .55); box(door, .05, .38, .9, m.glass, 0, .82, .6);
    door.visible = false; g.add(door);
    this.carShoulder.y = this.gy(this.carShoulder.x, this.carShoulder.z);
  }

  // ---------- the Sentinel's bundle, dropped at the gas station at a quarter to five ----------
  private bundle() {
    const g = this.paper, x = -16.35, z = 2069.4, y = this.gy(x, z) + .055;
    g.position.set(x, y, z);
    box(g, .3, .22, .42, this.m.white, 0, .11, 0);
    this.board(g, R.paper, .3, .42, 0, .222, 0, 0, -Math.PI / 2);
    box(g, .31, .012, .02, this.m.trim, 0, .2, -.1); box(g, .31, .012, .02, this.m.trim, 0, .2, .1);
    this.proxy('paper', .5, .5, .6, x, y + .2, z);
  }

  // ---------- the people out at night: the woman on the bench, the souvenir man on his stoop ----------
  private people() {
    const m = this.m;
    const head = (g: THREE.Group, y: number, hair: THREE.Material, bun = false) => {
      const f = new THREE.Mesh(new THREE.IcosahedronGeometry(.15, 1), m.skin); f.scale.set(.87, 1.15, .88); f.position.y = y; g.add(f);
      const h = new THREE.Mesh(new THREE.IcosahedronGeometry(.154, 1), hair); h.scale.set(.98, .7, .9); h.position.set(0, y + .065, .04); g.add(h);
      if (bun) { const b = new THREE.Mesh(new THREE.IcosahedronGeometry(.075, 0), hair); b.position.set(0, y + .02, .15); g.add(b); }
    };
    // she sits at the north end of the bench, facing the street, a transistor radio in her lap
    // (the model faces -z: turned to face east)
    {
      const g = this.woman, x = -14.45 + .02, z = 2132.75, y = this.gy(x, z) + .44;
      g.position.set(x, y, z); g.rotation.y = -Math.PI / 2;
      box(g, .38, .5, .26, m.coat, 0, .27, .02); cyl(g, .055, .06, .09, m.skin, 0, .56, 0, 7);
      for (const sx of [-.1, .1]) {
        rod(g, V(sx, .03, 0), V(sx, .03, -.4), .075, m.coat, 6); rod(g, V(sx, .02, -.4), V(sx, -.38, -.42), .05, m.cloth, 6);
        box(g, .12, .07, .2, m.dark, sx, -.41, -.46);
      }
      for (const sd of [-1, 1]) { rod(g, V(sd * .19, .46, 0), V(sd * .2, .2, -.12), .058, m.coat, 6); rod(g, V(sd * .2, .2, -.12), V(sd * .07, .1, -.3), .042, m.skin, 6); }
      box(g, .2, .12, .06, m.dark, 0, .12, -.33); box(g, .14, .07, .008, m.metal, 0, .13, -.364);
      head(g, .73, m.hair, true);
      this.present(this.proxy('woman', .7, 1.2, 1.1, x + .1, y + .2, z), () => this.womanIn);
    }
    // he stands on the stoop of the cream house, east side, looking at the street, smoking
    {
      const g = this.smoker, x = 24.15, z = 2159.4, y = this.gy(x, z) + .3;
      g.position.set(x, y, z); g.rotation.y = Math.PI / 2;
      for (const sx of [-.1, .1]) { rod(g, V(sx, .88, 0), V(sx, .1, .02), .07, m.cloth, 6); box(g, .13, .08, .25, m.dark, sx, .04, -.05); }
      box(g, .42, .58, .26, m.coat, 0, 1.17, 0); cyl(g, .058, .062, .1, m.skin, 0, 1.5, 0, 7);
      rod(g, V(-.22, 1.42, 0), V(-.25, 1.0, .02), .06, m.coat, 6); rod(g, V(-.25, 1.0, .02), V(-.24, .82, -.02), .045, m.skin, 6);
      rod(g, V(.22, 1.42, 0), V(.24, 1.2, -.18), .06, m.coat, 6); rod(g, V(.24, 1.2, -.18), V(.06, 1.55, -.2), .045, m.skin, 6);
      head(g, 1.68, m.hair);
      box(g, .34, .05, .34, m.cloth, 0, 1.84, 0); box(g, .22, .1, .22, m.cloth, 0, 1.9, 0);
      // his porch light, on while he is out (the town's last flood slot)
      this.light('porch', 24.5 - .5, this.gy(24.5, z) + 2.5, z, 3.2, 0xffd49a);
      this.slots.porchGlow = this.glow.add(24.5 - .12, this.gy(24.5, z) + 2.55, z, .4, 0xffd890);
      // the cigarette, glowing brighter when he draws on it (update)
      this.slots.ember = this.glow.add(x - .2, y + 1.56, z - .04, .07, 0xff6a20);
      this.present(this.proxy('smoker', .7, 1.9, .8, x, y + .95, z), () => this.smokerIn);
    }
  }
  // ---------- the cardboard alien, out on the shoulder (event 6): drawn in code until ChatGPT's
  // cutout_alien.png comes (ART_BRIEF.md, round 14) ----------
  private cardboard() {
    const c = document.createElement('canvas'); c.width = 128; c.height = 256;
    const g = c.getContext('2d')!;
    g.fillStyle = '#8f9a8c';
    g.beginPath(); g.ellipse(64, 58, 44, 54, 0, 0, 7); g.fill();
    g.beginPath(); g.moveTo(52, 104); g.lineTo(76, 104); g.lineTo(84, 150); g.lineTo(80, 240); g.lineTo(70, 240); g.lineTo(64, 172);
    g.lineTo(58, 240); g.lineTo(48, 240); g.lineTo(44, 150); g.closePath(); g.fill();
    g.lineWidth = 7; g.strokeStyle = '#8f9a8c'; g.lineCap = 'round';
    g.beginPath(); g.moveTo(46, 118); g.lineTo(24, 186); g.moveTo(82, 118); g.lineTo(104, 186); g.stroke();
    g.fillStyle = '#0b0b0b';
    g.beginPath(); g.ellipse(44, 62, 15, 8, 0.55, 0, 7); g.ellipse(84, 62, 15, 8, -0.55, 0, 7); g.fill();
    g.fillStyle = 'rgba(60,50,40,.35)'; g.fillRect(0, 196, 128, 4);
    const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace;
    const mat = this.mat({ map: tex, transparent: false, alphaTest: 0.5, side: THREE.DoubleSide, roughness: 1 });
    // the shape is the canvas's painted part: the rest is clear and cut away (alphaTest)
    const p = this.cutoutAt, y = this.gy(p.x, p.z);
    p.y = y;
    this.cutout.position.set(p.x, y, p.z);
    plane(this.cutout, .62, 1.24, mat, 0, .62, 0, Math.PI);
    box(this.cutout, .05, .5, .05, this.m.wood, 0, .25, .03);
  }
  setCutout(on: boolean) { this.cutoutOut = on; this.cutout.visible = on; }
  /** Event 8: the lamps go out one by one, south to north, and come back on in the same order. */
  lampWave(t: number) { this.waveT0 = t; }
  /** The lamps in the order the wave takes them, in world terms (the dogs bark by them). */
  get waveOrder() { return [...this.lamps].sort((a, b) => b.z - a.z).map((L) => V(ARM_X + this.origin.x, this.origin.y, L.z + this.origin.z)); }
  static readonly WAVE_STEP = 1.3;
  static readonly WAVE_HOLD = 24;
  /** A person's proxy takes the crosshair only while they are there (the bench behind her). */
  private present(g: THREE.Object3D, here: () => boolean) {
    const ray = g.raycast.bind(g);
    g.raycast = (r, hits) => { if (here()) ray(r, hits); };
  }
  /** Who is out now (update): she goes in at ten to five, he a little after four. */
  setPeople(woman: boolean, smoker: boolean) {
    this.womanIn = woman; this.woman.visible = woman;
    this.smokerIn = smoker; this.smoker.visible = smoker;
    if (!smoker) this.glowColor(this.slots.ember, 0);
    this.glowColor(this.slots.porchGlow, smoker ? 0xffd890 : 0);
    if (floodOwner === this) setFlood(this.slots.porch, smoker ? 3.2 : 0, townFlood);
  }

  // ---------- the night ----------
  /** The deputy at his desk: the window lit, his car out front (it holds the truck and the player). */
  setDeputy(on: boolean) {
    this.deputyIn = on;
    if (on) this.carOut = false;
    this.placeCar();
    this.setPanel('sheriffWin', on ? 1 : 0.07);
    if (floodOwner === this) setFlood(this.slots.sheriffWin, on ? 4.5 : 0, townFlood);
    this.glowColor(this.slots.sheriffGlow, on ? 0x2a2618 : 0);
  }
  /** Event 12: the car out on the shoulder (only while he is out), or not. */
  setCarOut(on: boolean) { this.carOut = on && !this.deputyIn; this.placeCar(); }
  /** The car where it is: at the office while he is in, on the shoulder for event 12, else gone. */
  private placeCar() {
    const o = this.origin, out = this.carOut, here = this.deputyIn || out;
    const x = out ? this.carShoulder.x : -12.3, z = out ? this.carShoulder.z : 2106.5, y = this.gy(x, z);
    this.car.visible = here; this.carDoor.visible = out;
    this.car.position.set(x, y, z); this.car.rotation.y = out ? 0.04 : 0; this.car.updateMatrixWorld(true);
    this.proxies.patrolCar.position.set(x, y + .8, z); this.proxies.patrolCar.updateMatrixWorld(true);
    Object.assign(this.carCol, here ? { minX: x - 1 + o.x, maxX: x + 1 + o.x, minZ: z - 2.7 + o.z, maxZ: z + 2.7 + o.z } : { minX: 1e9, maxX: 1e9, minZ: 1e9, maxZ: 1e9 });
  }
  setPaper(on: boolean) { this.paperIn = on; this.paper.visible = on; }
  /** The night now: clock, seconds since midnight; t, seconds running (for the blinking). */
  update(t: number, clock: number) {
    if (floodOwner !== this) return;
    const day = ((clock % 86400) + 86400) % 86400;
    const deputy = day < 2 * 3600 + 45 * 60 || day >= 4 * 3600 + 30 * 60;
    if (deputy !== this.deputyIn) this.setDeputy(deputy);
    const paper = day >= 4 * 3600 + 45 * 60 && day < 12 * 3600;
    if (paper !== this.paperIn) this.setPaper(paper);
    const woman = day < 4 * 3600 + 50 * 60, smoker = day < 4 * 3600 + 10 * 60;
    if (woman !== this.womanIn || smoker !== this.smokerIn) this.setPeople(woman, smoker);
    const draw = smoker && (t % 9) < 1.4;
    if (smoker && draw !== this.drawing) { this.drawing = draw; this.glowColor(this.slots.ember, draw ? 0xff6a20 : 0x5a2408); }
    for (const L of this.lamps) {
      const k = LAMPS.length - 1 - LAMPS.indexOf(L.z), w = t - this.waveT0 - k * Town.WAVE_STEP;
      const on = day < L.off && !(w >= 0 && w < Town.WAVE_HOLD);
      if (on === L.on) continue;
      L.on = on;
      setFlood(L.slot, on ? 13 : 0, townFlood);
      this.glowColor(L.glow[0], on ? 0xffb060 : 0); this.glowColor(L.glow[1], on ? 0x2c1806 : 0);
      this.setPanel('lamps', on ? 1 : 0.05, L.quad);
    }
    // the Night Line until four, then the carrier and an empty studio
    const air = day < SIGN_OFF;
    this.setPanel('onAir', air ? 1 : 0); this.setPanel('studio', air ? 1 : 0.3);
    setFlood(this.slots.studio, air ? 4 : 1.2, townFlood);
    // the blinker: on a little over half a second, off a little under
    const blink = (t % 1.1) < 0.6;
    if (blink !== this.blinkOn) {
      this.blinkOn = blink;
      this.setPanel('blink', blink ? 1 : 0.06);
      setFlood(this.slots.blink, blink ? 5 : 0, townFlood);
      this.glowColor(this.blinkGlow[0], blink ? 0xffb420 : 0); this.glowColor(this.blinkGlow[1], blink ? 0x2a1a04 : 0);
    }
    // the televisions: one goes off at twenty to four, the other stays on to the test pattern
    const flick = (ph: number) => { const n = hash(Math.floor(t * 8 + ph), ph); return 0.55 + 0.3 * n + 0.15 * Math.sin(t * 11 + ph); };
    const tv1 = day < 3 * 3600 + 40 * 60 ? flick(1) : 0;
    this.setPanel('tv1', tv1 ? tv1 : 0.05); setFlood(this.slots.tv1, tv1 * 3, townFlood);
    this.glowColor(this.slots.tv1Glow, tv1 ? 0x101828 : 0);
    const tv2 = flick(7); this.setPanel('tv2', tv2); setFlood(this.slots.tv2, tv2 * 3, townFlood);
    // the laundromat's tube hums, and now and then it stutters
    const st = hash(Math.floor(t * 9), 3) > 0.985 ? 0.45 : 0.97 + 0.03 * Math.sin(t * 31);
    this.setPanel('laundroWin', st); setFlood(this.slots.laundro, 6 * st, townFlood);
    if (this.dirty) { (this.qMesh.geometry.getAttribute('color') as THREE.BufferAttribute).needsUpdate = true; this.dirty = false; }
    this.glow.update(t);
  }

  /** The driven truck's headlights on the town: lamps of another set copied into the town's
   *  three, their colour k times as strong (the town's light falls off faster). src null: none. */
  shine(src: FloodSet | null, slots: readonly number[], k: number) {
    if (floodOwner !== this) return;
    this.head.forEach((h, j) => {
      const i = slots[j], p = townFlood.pos[h];
      if (!src || i === undefined || i < 0) { p.w = 0; return; }
      p.copy(src.pos[i]); townFlood.col[h].copy(src.col[i]).multiplyScalar(k);
    });
  }
  /** The street lamps and the canopy nearest the camera, as lamps in another set (out: the
   *  road's), so their pools lie on the highway and the land too. k scales the colour. */
  private near: number[] = [];
  lampsTo(out: FloodSet, slots: readonly number[], at: THREE.Vector3, k: number) {
    if (floodOwner !== this) return;
    const cand = [...this.lamps.map((l) => l.slot), this.slots.canopy, this.slots.blink];
    const d = (i: number) => { const p = townFlood.pos[i]; return (p.x - at.x) ** 2 + (p.z - at.z) ** 2; };
    this.near = cand.filter((i) => townFlood.pos[i].w > 0).sort((a, b) => d(a) - d(b));
    slots.forEach((o, j) => {
      const i = this.near[j];
      if (o === undefined || o < 0) return;
      if (i === undefined) { out.pos[o].w = 0; return; }
      out.pos[o].copy(townFlood.pos[i]); out.col[o].copy(townFlood.col[i]).multiplyScalar(k);
    });
  }

  // ---------- for the player ----------
  /** Whether a world point is on Caliche's street (or its crossing). */
  inTown(x: number, z: number) {
    return this.zones.some((q) => x >= q.minX && x <= q.maxX && z >= q.minZ && z <= q.maxZ);
  }
  /** Where in town a world point is, for the place shown in the corner. */
  placeAt(x: number, z: number) {
    const lx = x - this.origin.x, lz = z - this.origin.z;
    let best = 'Caliche', d = 64;
    for (const p of PLACES) { const e = (p.x - lx) ** 2 + (p.z - lz) ** 2; if (e < d) { d = e; best = p.name; } }
    return lz < 2016 ? 'Caliche, the crossing' : best;
  }
  /** What is underfoot: concrete on the sidewalks, the lots and the highway, dirt between. */
  surfaceAt(x: number, z: number): Surface {
    const lx = x - this.origin.x, lz = z - this.origin.z;
    if (Math.abs(lx) < 6) return 'concrete';
    if (lx > -17.6 && lx < -5.6 && lz > 2040 && lz < 2090) return 'concrete';
    if ((lx > -17.6 && lx < -14.2) || (lx > 16.6 && lx < 20.2 && lz < 2200)) return 'concrete';
    return 'dirt';
  }
  /** The height to walk at (world): the diner's lot and floor are flat (Diner.ts), with an easy
   *  step down round them; elsewhere the road's own ground. */
  floorAt = (x: number, z: number) => {
    const o = this.origin, lx = x - o.x, lz = z - o.z;
    let g = o.y + surfaceY(lx, lz);
    // where the old road comes off the highway its crown stands 14 cm over the shoulder: on foot
    // it is walked up over three metres
    if (lx > 5 && lx < 8 && Math.abs(lz - DINER.z) < 6) { const a = surfaceY(5, lz), b = surfaceY(8, lz); g = o.y + a + (b - a) * (lx - 5) / 3; }
    const dx = Math.max(DINER.x - 8.2 - lx, 0, lx - (DINER.x + 16.7)), dz = Math.max(DINER.z - 16.2 - lz, 0, lz - (DINER.z + 16.2));
    const d = Math.hypot(dx, dz), y = DINER.y + 0.1 + o.y;
    return d >= 1.2 ? g : y + (g - y) * THREE.MathUtils.smoothstep(d, 0, 1.2);
  };
  /** The atlas and its quads, for the developer's eye (tools/town.py). */
  get stats() { return { quads: this.qPos.length / 12, lamps: townFlood.count, colliders: this.colliders.length, glows: this.glow.points.geometry.getAttribute('position')?.count ?? 0 }; }
}

// ---------- the atlas: every sign and window picture of the town, drawn once ----------
function drawAtlas(g: CanvasRenderingContext2D) {
  g.fillStyle = '#000'; g.fillRect(0, 0, AW, AH);
  const at = (r: Rect, fn: (w: number, h: number) => void) => { g.save(); g.translate(r[0], r[1]); g.beginPath(); g.rect(0, 0, r[2], r[3]); g.clip(); fn(r[2], r[3]); g.restore(); };
  const text = (s: string, x: number, y: number, font: string, color: string, max?: number, align: CanvasTextAlign = 'center') => {
    g.font = font; g.fillStyle = color; g.textAlign = align; g.textBaseline = 'middle'; g.fillText(s, x, y, max);
  };
  const SANS = 'Arial, Helvetica, sans-serif', SERIF = 'Georgia, "Times New Roman", serif';
  const rnd = (() => { let s = 1986; return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; }; })();

  at(R.caliche, (w, h) => {
    g.fillStyle = '#1e5a3a'; g.fillRect(0, 0, w, h);
    g.strokeStyle = '#eef2ea'; g.lineWidth = 9; g.strokeRect(14, 14, w - 28, h - 28);
    text('CALICHE', w / 2, 100, `bold 112px ${SANS}`, '#eef2ea', w - 70);
    text('POP. 412', w / 2, 186, `bold 58px ${SANS}`, '#eef2ea', w - 120);
  });
  at(R.gas, (w, h) => {
    g.fillStyle = '#f0ebdf'; g.fillRect(0, 0, w, h);
    text('GAS', w / 2, 92, `bold 150px ${SANS}`, '#b2261e', w - 60);
    g.fillStyle = '#1d3557'; g.fillRect(0, 176, w, 80);
    text('DIESEL', w / 2, 217, `bold 56px ${SANS}`, '#f0ebdf', w - 80);
  });
  at(R.am, (w, h) => {
    g.fillStyle = '#18202c'; g.fillRect(0, 0, w, h);
    text('1420', 210, 95, `bold 140px ${SANS}`, '#f4ead2', 330);
    text('AM', 430, 112, `bold 70px ${SANS}`, '#f4ead2', 120);
    g.fillStyle = '#c9862a'; g.fillRect(30, 172, w - 60, 4);
    text('N I G H T   L I N E', w / 2, 214, `bold 44px ${SANS}`, '#e8a640', w - 60);
  });
  at(R.sheriff, (w, h) => {
    g.fillStyle = '#4a3a28'; g.fillRect(0, 0, w, h);
    g.strokeStyle = '#c8a860'; g.lineWidth = 4; g.strokeRect(8, 8, w - 16, h - 16);
    text('COUNTY SHERIFF', w / 2, 48, `bold 54px ${SERIF}`, '#d8b868', w - 40);
    text('SUBSTATION', w / 2, 98, `bold 34px ${SERIF}`, '#d8b868', w - 80);
  });
  at(R.souvenir, (w, h) => {
    // plywood, painted by hand
    g.fillStyle = '#cbb48a'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 40; i++) { g.fillStyle = `rgba(110,80,40,${0.05 + rnd() * 0.08})`; g.fillRect(0, rnd() * h, w, 1 + rnd() * 3); }
    g.save(); g.translate(w / 2, 60); g.rotate(-0.025); text('SAUCER CRASH 1947', 0, 0, `bold 58px ${SERIF}`, '#8a2a1a', w - 40); g.restore();
    // a saucer, painted
    g.fillStyle = '#5a6a6a'; g.beginPath(); g.ellipse(96, 150, 62, 16, 0, 0, 7); g.fill();
    g.fillStyle = '#7e9090'; g.beginPath(); g.ellipse(96, 138, 26, 16, 0, Math.PI, 0); g.fill();
    g.save(); g.translate(330, 140); g.rotate(0.03); text('TOURS', 0, 0, `bold 76px ${SERIF}`, '#2f5a2a', 300); g.restore();
    text('ASK INSIDE', w / 2 + 40, 212, `bold 44px ${SERIF}`, '#3a2a1a', 360);
  });
  at(R.marquee, (w, h) => {
    g.fillStyle = '#ebe5d0'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#b9b19a'; for (const y of [42, 86]) g.fillRect(0, y, w, 2);
    text('C L O S E D', w / 2, 30, `bold 38px ${SANS}`, '#151515', w - 80);
    text('THANK YOU CALICHE', w / 2, 66, `bold 34px ${SANS}`, '#151515', w - 80);
    text('FOR 31 YEARS', w / 2, 106, `bold 30px ${SANS}`, '#151515', w - 200);
    g.fillStyle = '#ebe5d0'; g.fillRect(w / 2 + 120, 92, 26, 28);   // a letter has fallen
  });
  at(R.board, (w, h) => {
    g.fillStyle = '#16181a'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#23272a'; for (let y = 6; y < h; y += 8) g.fillRect(0, y, w, 2);
    text('THE HEAVENS DECLARE', w / 2, 26, `bold 34px ${SANS}`, '#efefea', w - 30);
    text('THE GLORY OF GOD', w / 2, 66, `bold 34px ${SANS}`, '#efefea', w - 30);
    text('PS 19:1', w / 2, 104, `bold 28px ${SANS}`, '#efefea', w - 30);
  });
  at(R.feed, (w, h) => {
    g.fillStyle = '#7a2b22'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 30; i++) { g.fillStyle = `rgba(40,10,5,${rnd() * 0.15})`; g.fillRect(rnd() * w, 0, 2 + rnd() * 6, h); }
    text('FEED & SEED', w / 2, h / 2 + 4, `bold 86px ${SERIF}`, '#efe2c4', w - 40);
  });
  at(R.laundro, (w, h) => {
    g.fillStyle = '#e8eff1'; g.fillRect(0, 0, w, h);
    text('LAUNDROMAT', w / 2, h / 2 + 2, `bold 82px ${SANS}`, '#1c5a86', w - 40);
  });
  at(R.open24, (w, h) => {
    g.fillStyle = '#080808'; g.fillRect(0, 0, w, h);
    g.shadowColor = '#ff3020'; g.shadowBlur = 16; text('OPEN 24 HRS', w / 2, h / 2 + 2, `bold 70px ${SANS}`, '#ff5a42', w - 40); g.shadowBlur = 0;
  });
  // ---------- windows, seen from the street at night ----------
  const wall = (w: number, h: number, top: string, bottom: string) => { const gr = g.createLinearGradient(0, 0, 0, h); gr.addColorStop(0, top); gr.addColorStop(1, bottom); g.fillStyle = gr; g.fillRect(0, 0, w, h); };
  at(R.inGas, (w, h) => {
    wall(w, h, '#e8d4a8', '#9a8460');
    for (const y of [40, 92, 144]) { g.fillStyle = '#6a5038'; g.fillRect(250, y, 240, 6); for (let x = 256; x < 486; x += 18) { g.fillStyle = ['#b33', '#d9b130', '#2b5aa8', '#e8e0d0', '#2a7a3a'][Math.floor(rnd() * 5)]; g.fillRect(x, y - 26, 13, 26); } }
    g.fillStyle = '#efe8d8'; g.fillRect(60, 30, 110, 120); text('APRIL', 115, 46, `bold 18px ${SANS}`, '#a22'); text('1986', 115, 68, `14px ${SANS}`, '#333');
    g.fillStyle = '#c33'; g.fillRect(120, 104, 14, 12);
    g.fillStyle = '#5c4430'; g.fillRect(0, 186, w, 70); g.fillStyle = '#7a5c40'; g.fillRect(0, 180, w, 10);
    g.fillStyle = '#2c2c2c'; g.fillRect(330, 150, 70, 34);
    // the attendant, reading behind the counter
    g.fillStyle = '#2a2420'; g.beginPath(); g.arc(200, 128, 20, 0, 7); g.fill(); g.fillRect(172, 146, 56, 40);
    g.fillStyle = '#d8d0b8'; g.fillRect(178, 160, 46, 24);
  });
  at(R.inSheriff, (w, h) => {
    wall(w, h, '#c4ccb4', '#8a9480');
    // the county map on the wall, and its pins
    g.fillStyle = '#e4d8b4'; g.fillRect(40, 18, 300, 170); g.strokeStyle = '#6a6250'; g.lineWidth = 3; g.strokeRect(40, 18, 300, 170);
    g.strokeStyle = '#8c8470'; g.lineWidth = 2; g.beginPath(); g.moveTo(190, 18); g.lineTo(196, 188); g.moveTo(196, 100); g.lineTo(340, 70); g.moveTo(196, 110); g.lineTo(40, 130); g.stroke();
    g.strokeStyle = '#6a8aa8'; g.beginPath(); g.moveTo(40, 60); g.bezierCurveTo(120, 80, 150, 40, 250, 50); g.stroke();
    const pin = (x: number, y: number, c = '#c81818') => { g.fillStyle = c; g.beginPath(); g.arc(x, y, 4, 0, 7); g.fill(); };
    for (let i = 0; i < 7; i++) pin(196 + i * 20, 100 - i * 4.2);              // along the old road, east
    for (let i = 0; i < 6; i++) pin(176 - i * 21, 112 + i * 3.4);              // and west, towards the station
    for (let i = 0; i < 9; i++) pin(60 + rnd() * 270, 30 + rnd() * 150, ['#c81818', '#1848c8', '#e0c020'][i % 3]);
    // the desk, the lamp, the radio, a cup
    g.fillStyle = '#5a4430'; g.fillRect(0, 200, w, 56);
    g.fillStyle = '#2a2a2a'; g.fillRect(380, 170, 70, 30); g.fillStyle = '#7a8a3a'; g.fillRect(392, 178, 24, 10);
    g.fillStyle = '#c8b030'; g.beginPath(); g.moveTo(450, 120); g.lineTo(490, 120); g.lineTo(470, 98); g.fill();
    g.fillStyle = '#e8e4d8'; g.fillRect(360, 186, 14, 16);
  });
  at(R.inStudio, (w, h) => {
    wall(w, h, '#5a4030', '#2e2018');
    g.fillStyle = '#6a5040'; for (let x = 0; x < w; x += 40) for (let y = 0; y < 150; y += 40) g.fillRect(x + 3, y + 3, 34, 34);
    g.fillStyle = '#ff2a1a'; g.fillRect(36, 20, 96, 34); text('ON AIR', 84, 38, `bold 22px ${SANS}`, '#fff');
    g.fillStyle = '#2a2a2c'; g.fillRect(0, 186, w, 70); g.fillStyle = '#3c3c40'; g.fillRect(140, 168, 260, 26);
    for (let i = 0; i < 14; i++) { g.fillStyle = '#c8c8c8'; g.fillRect(150 + i * 17, 174, 6, 12); }
    g.fillStyle = '#1a1a1a'; g.beginPath(); g.arc(380, 60, 26, 0, 7); g.arc(450, 60, 26, 0, 7); g.fill();   // the reels
    // the host at the microphone, headphones on
    g.fillStyle = '#1e1814'; g.beginPath(); g.arc(250, 112, 24, 0, 7); g.fill(); g.fillRect(214, 134, 72, 40);
    g.strokeStyle = '#111'; g.lineWidth = 6; g.beginPath(); g.arc(250, 110, 28, Math.PI, 0); g.stroke();
    g.strokeStyle = '#555'; g.lineWidth = 4; g.beginPath(); g.moveTo(330, 40); g.lineTo(290, 104); g.stroke(); g.fillStyle = '#333'; g.fillRect(280, 100, 16, 22);
  });
  at(R.inLaundro, (w, h) => {
    wall(w, h, '#eef4f6', '#b8c4c8');
    g.strokeStyle = '#c8d2d6'; g.lineWidth = 1; for (let x = 0; x < w; x += 22) { g.beginPath(); g.moveTo(x, 0); g.lineTo(x, 150); g.stroke(); }
    g.fillStyle = '#ffffff'; g.fillRect(0, 6, w, 8);
    for (let i = 0; i < 6; i++) { const x = 18 + i * 82; g.fillStyle = '#e8ecee'; g.fillRect(x, 120, 72, 110); g.fillStyle = '#9aa4a8'; g.fillRect(x, 120, 72, 4); g.fillStyle = '#2a3036'; g.beginPath(); g.arc(x + 36, 176, 22, 0, 7); g.fill(); g.fillStyle = '#4a5a66'; g.beginPath(); g.arc(x + 36, 176, 15, 0, 7); g.fill(); }
    g.fillStyle = '#fff'; g.fillRect(380, 40, 110, 46); text('NO DYEING', 435, 63, `bold 18px ${SANS}`, '#a22');
    g.fillStyle = '#c8c0b0'; g.fillRect(0, 230, w, 26);
  });
  at(R.inTv, (w, h) => {
    wall(w, h, '#151b2a', '#0a0d16');
    const gr = g.createRadialGradient(170, 120, 10, 170, 120, 130); gr.addColorStop(0, 'rgba(160,190,255,.55)'); gr.addColorStop(1, 'rgba(0,0,0,0)'); g.fillStyle = gr; g.fillRect(0, 0, w, h);
    g.fillStyle = '#3a2c20'; g.fillRect(130, 140, 90, 70); g.fillStyle = '#dce8ff'; g.fillRect(140, 96, 70, 52);
    g.fillStyle = '#0c0c10'; g.beginPath(); g.arc(70, 150, 22, 0, 7); g.fill(); g.fillRect(30, 166, 80, 90);   // someone in the chair
    g.fillStyle = '#3a1418'; g.fillRect(0, 0, 26, h); g.fillRect(w - 26, 0, 26, h);
  });
  at(R.inWarm, (w, h) => {
    wall(w, h, '#e8b070', '#b87838');
    for (let x = 0; x < w; x += 18) { g.fillStyle = `rgba(120,60,20,${0.1 + 0.1 * Math.sin(x)})`; g.fillRect(x, 0, 8, h); }
  });
  at(R.poster, (w, h) => {
    g.fillStyle = '#efe8d6'; g.fillRect(0, 0, w, h);
    text('SEEN', w / 2, 44, `bold 50px ${SANS}`, '#141414', w - 20);
    text('LIGHTS?', w / 2, 94, `bold 50px ${SANS}`, '#141414', w - 20);
    text('CALL THE NIGHT LINE', w / 2, 140, `bold 22px ${SANS}`, '#141414', w - 24);
    text('1420 AM', w / 2, 180, `bold 38px ${SANS}`, '#b2261e', w - 40);
    text('MIDNIGHT TO FOUR', w / 2, 220, `bold 18px ${SANS}`, '#141414', w - 40);
    g.fillStyle = 'rgba(230,220,160,.7)'; g.fillRect(0, 0, 40, 14); g.fillRect(w - 40, 0, 40, 14);
  });
  at(R.display, (w, h) => {
    wall(w, h, '#2a221a', '#140e0a');
    // the cardboard alien, the rubber mask, the bags of foil
    g.fillStyle = '#9aa69a'; g.beginPath(); g.ellipse(90, 74, 40, 50, 0, 0, 7); g.fill(); g.fillRect(74, 116, 32, 120);
    g.fillStyle = '#0a0a0a'; g.beginPath(); g.ellipse(72, 76, 13, 7, 0.5, 0, 7); g.ellipse(108, 76, 13, 7, -0.5, 0, 7); g.fill();
    g.fillStyle = '#4c7a3a'; g.beginPath(); g.ellipse(196, 132, 26, 32, 0, 0, 7); g.fill(); g.fillStyle = '#111'; g.fillRect(182, 124, 8, 6); g.fillRect(202, 124, 8, 6);
    for (let i = 0; i < 3; i++) { g.fillStyle = '#c8ccd0'; g.fillRect(150 + i * 32, 200, 26, 30); g.fillStyle = '#e8ecf0'; g.fillRect(154 + i * 32, 206, 10, 8); }
    g.fillStyle = '#efe6cc'; g.fillRect(138, 176, 104, 18); text('GENUINE DEBRIS $1', 190, 185, `bold 11px ${SANS}`, '#222', 100);
  });
  at(R.paper, (w, h) => {
    g.fillStyle = '#ddd5c2'; g.fillRect(0, 0, w, h);
    text('PECOS VALLEY SENTINEL', w / 2, 24, `bold 21px ${SERIF}`, '#1a1a1a', w - 16);
    g.fillStyle = '#1a1a1a'; g.fillRect(8, 40, w - 16, 2);
    text('MONDAY, APRIL 14, 1986', w / 2, 52, `11px ${SERIF}`, '#1a1a1a', w - 20);
    text('NICKLAUS, 46,', w / 2, 86, `bold 30px ${SERIF}`, '#111', w - 16);
    text('WINS MASTERS', w / 2, 118, `bold 30px ${SERIF}`, '#111', w - 16);
    g.fillStyle = '#7a746a'; for (let c = 0; c < 3; c++) for (let i = 0; i < 12; i++) g.fillRect(10 + c * 82, 142 + i * 9, 70 - (i % 3) * 8, 3);
  });
  at(R.pump, (w, h) => {
    g.fillStyle = '#e8e2d4'; g.fillRect(0, 0, w, h);
    text('REGULAR', w / 2, 18, `bold 16px ${SANS}`, '#b2261e', w - 10);
    g.fillStyle = '#1a1a1a'; g.fillRect(14, 34, w - 28, 34); text('89.9', w / 2, 52, `bold 24px monospace`, '#f0e8c0');
    g.fillStyle = '#1a1a1a'; g.fillRect(14, 78, w - 28, 34); text('0.000', w / 2, 96, `bold 20px monospace`, '#f0e8c0');
  });
  at(R.phone, (w, h) => { g.fillStyle = '#2a4e8e'; g.fillRect(0, 0, w, h); text('TELEPHONE', w / 2, h / 2 + 2, `bold 40px ${SANS}`, '#f4f4f4', w - 20); });
  at(R.closed, (w, h) => { g.fillStyle = '#efe9d8'; g.fillRect(0, 0, w, h); g.strokeStyle = '#a22'; g.lineWidth = 3; g.strokeRect(4, 4, w - 8, h - 8); text('CLOSED', w / 2, h / 2 + 2, `bold 30px ${SANS}`, '#a22', w - 16); });
  at(R.white, (w, h) => { g.fillStyle = '#ffffff'; g.fillRect(0, 0, w, h); });
  at(R.lens, (w, h) => { const gr = g.createRadialGradient(w / 2, h / 2, 2, w / 2, h / 2, w / 2); gr.addColorStop(0, '#ffffff'); gr.addColorStop(0.7, '#f0e0c0'); gr.addColorStop(1, '#8a7a60'); g.fillStyle = gr; g.fillRect(0, 0, w, h); });
  at(R.blade, (w, h) => {
    g.fillStyle = '#3a1e1a'; g.fillRect(0, 0, w, h); g.strokeStyle = '#8a6a4a'; g.lineWidth = 3; g.strokeRect(4, 4, w - 8, h - 8);
    'PLAZA'.split('').forEach((c, i) => text(c, w / 2, 30 + i * 48, `bold 44px ${SANS}`, '#7a5a48'));
  });
  at(R.onAir, (w, h) => { g.fillStyle = '#ff2a1a'; g.fillRect(0, 0, w, h); text('ON AIR', w / 2, h / 2 + 1, `bold 30px ${SANS}`, '#fff', w - 10); });
}
