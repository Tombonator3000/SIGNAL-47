import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { mergeStatic, noMerge } from './kit';
import { parse } from './blenderModels';

// Bought models (Vintage Liminal Props Pack, Superhive, Standard License: Tom, 19 October), cut,
// baked and fitted in Blender (the private repo's tools/build_models.py and models/MANIFEST.md).
// Their licence does not allow the files in this public repo, so they live in the private repo
// SIGNAL-47-assets, cloned to src/assets/licensed (as the bought sounds, core/Audio.ts). Without
// that folder the glob is empty, hasModel() says no everywhere, and every area builds exactly what
// it built before. With it, an area builds what a model stands in for into a holder of its own
// (ModelSwap.holder: merged by itself, not with the area), loads the .glb once the game is up
// (startModels), puts the model in the same place at the same size, lit by the area's own lamps
// (its fake floods, kit.floodlit, or the control room's real lights), and hides the holder. The
// colliders, the proxies the rays aim at, the look texts and the hints stay where they were: the
// models replace the look, not the game (they take no rays). ?nomodels in the address builds the
// game without them (as without the folder), and S47.models.set(false) shows the code-built props
// again in place of the loaded models (tools/licensed_models.py: the pictures with and without).

const FILES = import.meta.glob<string>('../assets/licensed/models/*.glb', { eager: true, query: '?url', import: 'default' });
const URLS: Record<string, string> = {};
for (const [path, url] of Object.entries(FILES)) URLS[/([^/]+)\.glb$/.exec(path)![1]] = url;
const wanted = (() => { try { return !new URLSearchParams(location.search).has('nomodels'); } catch { return true; } })();

/** This model is in the build (and not turned off with ?nomodels): build for it. */
export const hasModel = (name: string) => wanted && name in URLS;

/** A place for a thing: where it stands, how it is turned (about y), how large (one factor or x, y, z). */
export type At = { x: number; y: number; z: number; ry?: number; s?: number | [number, number, number]; rx?: number; rz?: number };
const _q = new THREE.Quaternion(), _e = new THREE.Euler(), _v = new THREE.Vector3(), _s = new THREE.Vector3();
export function matrixAt(a: At, m = new THREE.Matrix4()) {
  const s = a.s ?? 1;
  _s.set(...(typeof s === 'number' ? [s, s, s] as [number, number, number] : s));
  _q.setFromEuler(_e.set(a.rx ?? 0, a.ry ?? 0, a.rz ?? 0, 'YXZ'));
  return m.compose(_v.set(a.x, a.y, a.z), _q, _s);
}

// ---------- the pictures, at the size the device and the graphics level allow ----------
// A phone (and Low anywhere) gets them at most 512 texels a side, the small props' 256 (and the
// pictures that only glow): about 18 MB on the graphics card instead of 55 (tools/licensed_models.py
// checks it on Low). A computer on High or Ultra gets them as built. Each is decoded and made smaller off the main thread (createImageBitmap)
// when its file has loaded, and made again from the full picture when the level changes.
/** Small props: a few hundred pixels on a phone's screen at the closest. */
const SMALL = new Set(['archive_box', 'lamp_orange', 'lamp_marble', 'car_battery', 'folding_chair', 'nightstand', 'dining_chair', 'weathered_chair']);
type Img = CanvasImageSource & { width: number; height: number };
type Pic = { t: THREE.Texture; small: boolean; full: Img; size: number; fitting?: Promise<void> };
const pics: Pic[] = [];
const fit = { touch: false, low: false, keepFull: true };
const capOf = (p: Pic) => fit.touch || fit.low ? (p.small ? 256 : 512) : Infinity;
const nextTask = () => new Promise<void>((r) => setTimeout(r, 0));
/** The picture at `size` a side: an ImageBitmap where the browser can make one that size, else a canvas. */
async function smaller(img: Img, size: number): Promise<Img> {
  try {
    const b = await createImageBitmap(img, { resizeWidth: size, resizeHeight: size, resizeQuality: 'high' });
    if (b.width === size && b.height === size) return b;
    b.close();
  } catch { /* (a browser without the options: the canvas) */ }
  const c = document.createElement('canvas'); c.width = c.height = size;
  const g = c.getContext('2d')!; g.imageSmoothingQuality = 'high'; g.drawImage(img, 0, 0, size, size);
  return c;
}
/** A picture to the size its cap wants now (the full one, or one made smaller from it). */
function fitPic(p: Pic): Promise<void> {
  const want = Math.min(p.full.width, capOf(p));
  if (want === p.size) return p.fitting ?? Promise.resolve();
  p.size = want;
  const go = async () => {
    const img = want >= p.full.width ? p.full : await smaller(p.full, want);
    if (p.size !== want) return;   // (the level changed again meanwhile)
    const old = p.t.image as Img;
    p.t.image = img; p.t.needsUpdate = true;
    if (old !== p.full && old !== img && typeof ImageBitmap !== 'undefined' && old instanceof ImageBitmap) old.close();
  };
  return p.fitting = go();
}
/** The pictures of a loaded file: decoded now (off the main thread, not at their first use), and
 *  made the size the device and the level allow. Emissive maps that are only that glow: small. */
async function takePictures(name: string, scene: THREE.Object3D) {
  const maps = new Set<THREE.Texture>(), glows = new Set<THREE.Texture>();
  scene.traverse((o) => {
    const m = (o as THREE.Mesh).material as THREE.MeshStandardMaterial | undefined;
    if (m?.map) maps.add(m.map);
    if (m?.emissiveMap) glows.add(m.emissiveMap);
  });
  const all = [...new Set([...maps, ...glows])].filter((t) => t.image);
  await Promise.all(all.map((t) => (t.image as HTMLImageElement).decode?.().catch(() => {})));
  const mine = all.map((t) => ({ t, small: SMALL.has(name) || !maps.has(t), full: t.image as Img, size: (t.image as Img).width }));
  // (a phone keeps no full picture: it never draws one)
  if (!fit.keepFull) for (const p of mine) if (capOf(p) < p.full.width) { const img = await smaller(p.full, capOf(p)); p.t.image = img; p.size = img.width; p.full = img; }
  pics.push(...mine);
  await Promise.all(mine.map(fitPic));
}

// ---------- loading ----------
type Loaded = { scene: THREE.Object3D; ms: number; bytes: number };
const files = new Map<string, Promise<Loaded>>();
const stats: Record<string, { ms: number; bytes: number }> = {};
function load(name: string) {
  let p = files.get(name);
  if (!p) {
    const t0 = performance.now(), url = URLS[name];
    p = parse(url).then(async (g) => {
      await takePictures(name, g.scene);
      const ms = performance.now() - t0;
      // (the single file has the bytes as base64 in the address: three quarters of its length)
      const bytes = url.startsWith('data:') ? Math.round((url.length - url.indexOf(',') - 1) * 0.75) : 0;
      stats[name] = { ms, bytes };
      g.scene.updateMatrixWorld(true);
      return { scene: g.scene, ms, bytes };
    });
    files.set(name, p);
  }
  return p;
}

/** What main.ts gives the models: the graphics level and Ultra's shadows for what is added late,
 *  the pictures up to the graphics card and the programs compiled before the models are shown. */
export type ModelHost = {
  prepare: (root: THREE.Object3D) => void;
  /** A phone (the pictures capped whatever the level). */
  touch: boolean;
  low: boolean;
  upload: (t: THREE.Texture) => void;
  /** Compiles the materials of what `root` holds for the lights now in the picture. */
  compile: (root: THREE.Object3D) => Promise<unknown>;
};
let started = false;
let host: ModelHost = { prepare: () => {}, touch: false, low: false, upload: () => {}, compile: async () => {} };
const waiting: (() => void)[] = [];
/** The game is up (main.ts): load what the areas built so far want, and later areas' at once. */
export function startModels(h: ModelHost) {
  host = h;
  fit.touch = h.touch; fit.low = h.low; fit.keepFull = !h.touch;
  if (started) return;
  started = true;
  for (const go of waiting.splice(0)) go();
}
/** The graphics level changed (main.ts applyQuality): the pictures to its size, the programs for
 *  its materials. */
export function modelsQuality(low: boolean) {
  fit.low = low;
  for (const p of pics) void fitPic(p);
  warmShown();
}
/** The areas' own groups (under the scene) of what is shown now: the models in them are compiled
 *  for the lights as they are now (World.enter, after an area is shown: cheap once compiled). */
export function warmShown() {
  for (const s of swaps) if (s.loaded && s.models.length && s.models.every((m) => areaShown(m))) void s.compile();
}
function areaShown(o: THREE.Object3D) {
  let p: THREE.Object3D | null = o;
  while (p && p.parent && !(p.parent as THREE.Scene).isScene) p = p.parent;
  return !!p?.parent && p.visible;
}
/** These areas' models loaded (or failed), but no longer than `ms`: World waits for them behind
 *  its fade, so the drive does not stop for their build once the picture is back. */
export function modelsSettled(list: (ModelSwap | undefined)[], ms = 6000) {
  const ps = list.filter((s): s is ModelSwap => !!s && s.wanted.length > 0 && started).map((s) => s.ready);
  if (!ps.length) return Promise.resolve();
  return Promise.race([Promise.all(ps).then(() => {}), new Promise<void>((r) => setTimeout(r, ms))]);
}

// ---------- the models' parts, lit for a place ----------
/** One primitive of a model: its geometry (the nodes carry no transform) and its material. */
export type Part = { geometry: THREE.BufferGeometry; material: THREE.MeshStandardMaterial };
export class Lib {
  private mats = new Map<string, THREE.MeshStandardMaterial>();
  constructor(private scenes: Record<string, THREE.Object3D>, private lit: (m: THREE.MeshStandardMaterial) => THREE.MeshStandardMaterial) {}

  /** The node of a model by its mesh's name (a Mesh, or a Group of one Mesh per material). */
  node(file: string, name: string) {
    const o = this.scenes[file]?.getObjectByName(name);
    if (!o) throw new Error(`${file}.glb has no ${name}`);
    return o;
  }
  /** What the model's builder wrote in a mesh's extras (screens, drawers, shelves). */
  extras(file: string, name: string) { return this.node(file, name).userData as Record<string, any>; }
  /** The primitives of a mesh, one per material. */
  parts(file: string, name: string): Part[] {
    const out: Part[] = [];
    this.node(file, name).traverse((o) => {
      const m = o as THREE.Mesh;
      if (m.isMesh) out.push({ geometry: m.geometry, material: m.material as THREE.MeshStandardMaterial });
    });
    return out;
  }
  /** A model's material as the place lights it (one copy per place, shared by everything in it;
   *  own: a copy of its own, for a lamp the story switches). vc: with the geometry's colours. */
  material(src: THREE.MeshStandardMaterial, own = false, vc = false) {
    const key = src.uuid + (vc ? ':vc' : '');
    let m = own ? undefined : this.mats.get(key);
    if (m) return m;
    m = src.clone();
    m.vertexColors = vc;
    if (m.map) m.map.anisotropy = 4;
    m = this.lit(m);
    if (!own) this.mats.set(key, m);
    return m;
  }
  /** A model as a mesh (or a group of one per material) at a place. */
  mesh(file: string, name: string, at?: At, opt: { own?: boolean } = {}): THREE.Object3D {
    const parts = this.parts(file, name);
    const mk = (p: Part) => { const m = new THREE.Mesh(p.geometry, this.material(p.material, opt.own)); m.name = `lic:${name}`; m.userData.model = file; return m; };
    let o: THREE.Object3D;
    if (parts.length === 1) o = mk(parts[0]);
    else { o = new THREE.Group(); o.name = `lic:${name}`; for (const p of parts) o.add(mk(p)); }
    if (at) { matrixAt(at, o.matrix); o.matrix.decompose(o.position, o.quaternion, o.scale); }
    return o;
  }
  /** Several placed copies of a model's meshes (one material) merged into one mesh: one draw call.
   *  tint: a colour per copy, laid on where the attribute `paint` of the mesh is 1 (or everywhere
   *  without it), as the geometry's colours; wear: how much of it is left at a point of the mesh
   *  (in its own terms, 0 worn off to 1), times that. */
  merged(file: string, items: { name: string; at: At; tint?: THREE.ColorRepresentation; paint?: string; wear?: (p: THREE.Vector3) => number }[]) {
    const geos: THREE.BufferGeometry[] = [];
    let mat: THREE.MeshStandardMaterial | null = null;
    const vc = items.some((i) => i.tint !== undefined), c = new THREE.Color();
    for (const it of items) {
      const [p] = this.parts(file, it.name);
      mat ??= p.material;
      const g = p.geometry.clone();
      // (quantized attributes come out as plain floats when the geometry is transformed and merged)
      for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'uv', ...(it.paint ? [it.paint] : [])].includes(k)) g.deleteAttribute(k);
      for (const k of ['position', 'normal', 'uv']) {
        const a = g.getAttribute(k) as THREE.BufferAttribute;
        if (a && (a.normalized || !(a.array instanceof Float32Array))) {
          const f = new Float32Array(a.count * a.itemSize);
          for (let i = 0; i < a.count; i++) for (let j = 0; j < a.itemSize; j++) f[i * a.itemSize + j] = a.getComponent(i, j);
          g.setAttribute(k, new THREE.BufferAttribute(f, a.itemSize));
        }
      }
      const local = it.wear ? (g.getAttribute('position') as THREE.BufferAttribute).clone() : null;
      const mx = matrixAt(it.at);
      g.applyMatrix4(mx);
      // (a mirrored copy: its triangles wound the other way, so their fronts stay outside)
      if (mx.determinant() < 0 && g.index) {
        const ix = g.index;
        for (let i = 0; i < ix.count; i += 3) { const a = ix.getX(i + 1); ix.setX(i + 1, ix.getX(i + 2)); ix.setX(i + 2, a); }
      }
      if (vc) {
        const n = g.getAttribute('position').count, col = new Float32Array(n * 3), paint = it.paint ? g.getAttribute(it.paint) : null;
        c.set(it.tint ?? 0xffffff);   // (linear: three reads a hex colour as sRGB)
        const v = new THREE.Vector3();
        for (let i = 0; i < n; i++) {
          const k = (paint ? paint.getX(i) : 1) * (local ? it.wear!(v.fromBufferAttribute(local, i)) : 1);
          col[i * 3] = 1 + (c.r - 1) * k; col[i * 3 + 1] = 1 + (c.g - 1) * k; col[i * 3 + 2] = 1 + (c.b - 1) * k;
        }
        g.setAttribute('color', new THREE.BufferAttribute(col, 3));
      }
      if (it.paint) g.deleteAttribute(it.paint);
      geos.push(g);
    }
    const g = mergeGeometries(geos, false);
    if (!g) throw new Error(`${file}: the meshes do not merge`);
    g.computeBoundingSphere();
    const m = new THREE.Mesh(g, this.material(mat!, false, vc));
    m.name = `lic:${file}`; m.userData.model = file;
    m.matrixAutoUpdate = false;
    return m;
  }
  /** Copies of one mesh (one material) drawn as one InstancedMesh, each at its place and, with
   *  colours, in its own tone (the colour multiplies the model's picture; above 1 brightens). */
  instanced(file: string, name: string, at: At[], colors?: THREE.Color[]) {
    const [p] = this.parts(file, name);
    const im = new THREE.InstancedMesh(p.geometry, this.material(p.material), at.length);
    const m = new THREE.Matrix4();
    at.forEach((a, i) => { im.setMatrixAt(i, matrixAt(a, m)); if (colors) im.setColorAt(i, colors[i]); });
    im.instanceMatrix.needsUpdate = true;
    if (im.instanceColor) im.instanceColor.needsUpdate = true;
    im.computeBoundingSphere();
    im.name = `lic:${name}`; im.userData.model = file;
    return im;
  }
  /** Colours that turn a mesh's picture to a tone of the code's (the picture's mean colour to
   *  `c`, times k): for the instances' colours, or a material's colour. */
  tones(file: string, name: string) {
    const mean = this.meanColor(file, name);
    return (c?: number, k = 1) => {
      if (c === undefined) return new THREE.Color(k, k, k);
      const t = new THREE.Color(c);   // (linear already)
      return t.setRGB(t.r / Math.max(mean.r, 0.02) * k, t.g / Math.max(mean.g, 0.02) * k, t.b / Math.max(mean.b, 0.02) * k);
    };
  }
  /** The mean colour of a material's picture (linear), for tones relative to it. */
  meanColor(file: string, name: string) {
    const img = this.parts(file, name)[0].material.map?.image as CanvasImageSource & { width: number; height: number } | undefined;
    const c = new THREE.Color(1, 1, 1);
    if (!img) return c;
    try {
      const cv = document.createElement('canvas'); cv.width = cv.height = 64;
      const g = cv.getContext('2d', { willReadFrequently: true })!;
      g.imageSmoothingQuality = 'high';
      g.drawImage(img, 0, 0, 64, 64);
      const d = g.getImageData(0, 0, 64, 64).data;
      let r = 0, gg = 0, b = 0;
      for (let i = 0; i < d.length; i += 4) {
        r += new THREE.Color().setRGB(d[i] / 255, 0, 0, THREE.SRGBColorSpace).r;
        gg += new THREE.Color().setRGB(0, d[i + 1] / 255, 0, THREE.SRGBColorSpace).g;
        b += new THREE.Color().setRGB(0, 0, d[i + 2] / 255, THREE.SRGBColorSpace).b;
      }
      const n = d.length / 4;
      c.setRGB(r / n, gg / n, b / n);
    } catch { /* (a picture the canvas may not read: no tone) */ }
    return c;
  }
}

// ---------- swapping the code's props for the models ----------
const swaps: ModelSwap[] = [];
/** The pictures of these models' materials (as the graphics level has them now). */
function texturesOf(objs: THREE.Object3D[]) {
  const out = new Set<THREE.Texture>();
  for (const o of objs) o.traverse((x) => {
    const ms = (x as THREE.Mesh).material;
    for (const m of (Array.isArray(ms) ? ms : ms ? [ms] : []) as THREE.MeshStandardMaterial[]) for (const t of [m.map, m.emissiveMap]) if (t?.image) out.add(t);
  });
  return [...out];
}
let shown = true;

/** What the models do in one area: the holders of what they stand in for, what they add, and
 *  what else to change while they are shown. lit: the area's lighting for a material. */
export class ModelSwap {
  readonly holders: THREE.Object3D[] = [];
  readonly models: THREE.Object3D[] = [];
  /** Places the code's props leave for the models (tools/licensed_models.py checks them). */
  readonly places: { model: string; box: THREE.Box3 }[] = [];
  /** Data the area's dressing gives for the models (the archive boxes on the shelves). */
  readonly spots: Record<string, { at: At; tone?: number; lid?: At | null }[]> = {};
  private fits: ((on: boolean) => void)[] = [];
  private wants: string[] = [];
  loaded = false;
  failed = '';
  /** Milliseconds from the start of the load to the models in the picture. */
  ms = 0;
  /** Loaded, built, their pictures on the graphics card (and compiled, if the area is shown), or
   *  failed: World waits for it behind the fade (modelsSettled). */
  readonly ready: Promise<void>;
  private settle!: () => void;
  /** When (performance.now()) its files were in, its models built, their pictures up, compiled. */
  readonly steps: Record<string, number> = {};

  constructor(readonly name: string, readonly lit: (m: THREE.MeshStandardMaterial) => THREE.MeshStandardMaterial = (m) => m) {
    swaps.push(this);
    this.ready = new Promise<void>((r) => { this.settle = r; });
  }

  /** Where the code builds what `model` stands in for: a group of its own in `parent` (merged by
   *  itself, hidden once the model is in), or `parent` itself when the model is not in the build. */
  holder(parent: THREE.Object3D, model: string): THREE.Object3D {
    if (!hasModel(model)) return parent;
    const g = noMerge(new THREE.Group());
    g.name = `lic:${this.name}:${model}`;
    g.userData.licensed = model;
    parent.add(g);
    this.holders.push(g);
    this.want(model);
    return g;
  }
  /** Something the code built that a model stands in for (shown again without the model). */
  adopt(o: THREE.Object3D, model: string) {
    if (!hasModel(model)) return;
    // (a near-only group's own `visible` is the camera's, set before every picture: kit.followNear.
    // Hidden, it would be shown again at once; what it holds is hidden instead)
    if (o.userData.nearOnly) o = o.children[0];
    o.userData.licensed = model;
    this.holders.push(o);
    this.want(model);
  }
  /** Changed while the models are shown, and back when they are not. */
  fit(f: (on: boolean) => void) { this.fits.push(f); }
  /** Something of the code's that goes with a model (a label on its face): shown with it. */
  withModel(o: THREE.Object3D, model: string) {
    if (!hasModel(model)) return;
    o.visible = false;
    o.traverse((x) => { x.userData.model ??= model; });
    this.models.push(o);
    this.want(model);
  }
  /** A place the dressing gives a model. */
  spot(model: string, s: { at: At; tone?: number; lid?: At | null }) { (this.spots[model] ??= []).push(s); this.want(model); }
  /** A model that stands somewhere new (nothing of the code's in its place). */
  want(model: string) { if (hasModel(model) && !this.wants.includes(model)) this.wants.push(model); }
  get wanted() { return this.wants; }

  /** Once the area is built: merge the holders, and when the game is up load the models and
   *  build them (build adds them with add()). */
  place(build: (L: Lib, add: (o: THREE.Object3D, parent: THREE.Object3D) => THREE.Object3D) => void) {
    if (!this.wants.length) { this.settle(); return; }
    for (const h of this.holders) {
      if (!(h as THREE.Mesh).isMesh && h.userData.noMerge) mergeStatic(h);
      // the box each leaves, in the area's own terms (the holder's parent chain as built)
      h.updateWorldMatrix(true, true);
      const box = new THREE.Box3().setFromObject(h);
      if (!box.isEmpty()) this.places.push({ model: h.userData.licensed, box });
    }
    const go = async () => {
      const t0 = performance.now();
      try {
        const got = await Promise.all(this.wants.map(load));
        this.steps.loaded = performance.now();
        const scenes: Record<string, THREE.Object3D> = {};
        this.wants.forEach((n, i) => { scenes[n] = got[i].scene; });
        build(new Lib(scenes, this.lit), (o, parent) => {
          o.traverse((x) => { (x as THREE.Mesh).raycast = () => {}; });
          o.visible = false;
          parent.add(o);
          this.models.push(o);
          return o;
        });
        for (const o of this.models) host.prepare(o);
        this.steps.built = performance.now();
        // their pictures up to the graphics card one at a time, a task each (not all in the first
        // picture they are in), and their programs compiled for the lights of their area if it is
        // shown now (else when it is: warmShown, World.enter)
        for (const t of texturesOf(this.models)) { host.upload(t); await nextTask(); }
        this.steps.uploaded = performance.now();
        this.loaded = true;
        if (this.models.every(areaShown)) { await this.compile(); this.steps.compiled = performance.now(); }
        this.ms = performance.now() - t0;
      } catch (e) {
        this.loaded = false;
        this.failed = String((e as Error)?.message ?? e);
        console.warn(`the bought models (${this.name}) could not load:`, this.failed);
      }
      this.apply();
      this.settle();
    };
    if (started) void go(); else waiting.push(() => void go());
  }

  /** The programs of the models' materials, compiled for the lights in the picture now (in the
   *  background where the browser can: compileAsync). One root for all of them, so the scene's
   *  lights are gathered once. */
  compile() {
    const objs = this.models;
    const root = { traverse: (f: (o: THREE.Object3D) => void) => { for (const o of objs) o.traverse(f); }, traverseVisible: () => {} };
    return host.compile(root as unknown as THREE.Object3D).catch((e) => console.warn(`the bought models (${this.name}): no warm-up`, e));
  }

  /** The models in the picture (loaded, and not turned off), or the code's props. */
  get on() { return shown && this.loaded; }
  apply() {
    const on = this.on;
    for (const h of this.holders) h.visible = !on;
    for (const m of this.models) m.visible = on;
    for (const f of this.fits) f(on);
  }
}

/** Tests: the models, or the code's props in their place (S47.models). */
export const licensedModels = {
  inBuild: Object.keys(URLS).length > 0,
  wanted,
  files: Object.keys(URLS).sort(),
  swaps,
  get shown() { return shown; },
  set(on: boolean) { shown = on; for (const s of swaps) s.apply(); },
  /** The pictures at the size the level wants (after modelsQuality). */
  fitted: () => Promise.all(pics.map((p) => p.fitting)).then(() => true),
  /** Every area's state, the files' load times and sizes, and the textures' sizes. */
  status() {
    const tex = new Map<string, { w: number; h: number }>();
    // (the models' own meshes: not the labels that go with them, withModel)
    for (const s of swaps) for (const o of s.models) o.traverse((x) => {
      if (!x.name.startsWith('lic:')) return;
      const m = (x as THREE.Mesh).material as THREE.MeshStandardMaterial | undefined;
      for (const t of [m?.map, m?.emissiveMap]) if (t?.image) tex.set(t.uuid, { w: t.image.width, h: t.image.height });
    });
    let tris = 0;
    for (const s of swaps) for (const o of s.models) o.traverse((x) => {
      const m = x as THREE.Mesh;
      if (!m.isMesh || !x.name.startsWith('lic:')) return;
      const n = (m.geometry.index ? m.geometry.index.count : m.geometry.attributes.position.count) / 3;
      tris += n * ((m as THREE.InstancedMesh).isInstancedMesh ? (m as THREE.InstancedMesh).count : 1);
    });
    return {
      swaps: swaps.map((s) => ({ name: s.name, wants: s.wanted, loaded: s.loaded, failed: s.failed, ms: Math.round(s.ms), steps: s.steps, models: s.models.length, holders: s.holders.length,
        places: s.places.map((p) => ({ model: p.model, min: p.box.min.toArray(), max: p.box.max.toArray() })) })),
      files: stats,
      textures: [...tex.values()],
      /** The pictures capped (a phone, or Low): 512 a side, the small props' 256. */
      capped: fit.touch || fit.low,
      // the pictures on the graphics card: four bytes a texel and a third more for the mipmaps
      texBytes: [...tex.values()].reduce((n, t) => n + t.w * t.h * 4 * 4 / 3, 0),
      tris,
    };
  },
};
