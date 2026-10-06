import * as THREE from 'three';
import { floodSets, type FloodSet } from '../world/kit';
import { loadArtFor, ULTRA_ART, ULTRA_MAPS, ultraArtReady, dataTextureLike, type ArtId } from './art';

// The PC tier (Settings: Ultra). Phones keep High and Low as they are.
//
// Lamps: the game lights its places with fake floodlights (world/kit.ts), cheap pools of
// light without shadows. Here the three lamps nearest the player become real spotlights
// that cast soft shadows: the dishes, the fence, the truck and the furniture throw long
// shadows across the sodium pools. The fake lamp is switched off while its real one burns,
// and comes back when the player walks away. Its brightness is matched to the fake pool
// right under the lamp. The desk lamp in the control room casts shadows too.
//
// Post (core/vhs.ts runs it): ambient occlusion (GTAO) for contact shadows under desks and
// in corners, and real bloom before the tape look.
//
// Shadow maps need every mesh marked as caster and receiver; that is done once when the
// tier is switched on, and again for areas built later (World calls mark()).

const POOL = 3;
const SHADOW_HZ = 12;   // shadow maps are redrawn this often, not every frame (they cost a scene pass each)
const OFF = -1.2345e-5;   // what a lamp handed to a real light holds (the shader skips w <= 0)

interface Lamp { set: FloodSet; i: number; want: number }

export class Ultra {
  on = false;
  private spots: THREE.SpotLight[] = [];
  private held: (Lamp | null)[] = [];
  private t = 0;
  private extra: (THREE.PointLight | THREE.SpotLight)[] = [];   // lights that cast shadows only in this tier

  constructor(private renderer: THREE.WebGLRenderer, private scene: THREE.Scene) {
    for (let k = 0; k < POOL; k++) {
      const s = new THREE.SpotLight(0xffa24a, 0, 24, 1.2, 0.85, 2);
      s.castShadow = true;
      s.shadow.mapSize.set(1024, 1024);
      s.shadow.bias = -0.0004;
      s.shadow.normalBias = 0.03;
      s.shadow.camera.near = 0.3;
      s.shadow.autoUpdate = false;
      this.park(s);
      this.spots.push(s);
      this.held.push(null);
    }
  }

  /** Lights that should cast shadows in this tier (the desk lamp in the control room). */
  addShadowLight(l: THREE.PointLight | THREE.SpotLight, mapSize = 512) {
    l.shadow.mapSize.set(mapSize, mapSize);
    l.shadow.bias = -0.0006;
    l.shadow.normalBias = 0.02;
    if ((l as THREE.PointLight).isPointLight) l.shadow.camera.near = 0.05;
    l.shadow.autoUpdate = false;
    this.extra.push(l);
    l.castShadow = this.on;
  }

  set(on: boolean) {
    if (on === this.on) return;
    this.on = on;
    const r = this.renderer;
    r.shadowMap.enabled = on;
    r.shadowMap.type = THREE.PCFSoftShadowMap;
    for (const s of this.spots) {
      if (on && !s.parent) { this.scene.add(s, s.target); }
      if (!on && s.parent) { s.removeFromParent(); s.target.removeFromParent(); }
    }
    for (const l of this.extra) l.castShadow = on;
    if (!on) for (let k = 0; k < POOL; k++) this.release(k);
    if (!on) this.unmap();
    this.mark(this.scene);
    // every lit material needs its program rebuilt with or without shadow maps
    this.scene.traverse((o) => {
      const m = (o as THREE.Mesh).material;
      if (!m) return;
      for (const x of Array.isArray(m) ? m : [m]) x.needsUpdate = true;
    });
  }

  /** Marks meshes under root as shadow casters and receivers (opaque, lit ones only). */
  mark(root: THREE.Object3D) {
    const on = this.on;
    root.traverse((o) => {
      const m = o as THREE.Mesh;
      if (!m.isMesh || o.userData.noShadow) return;
      const mat = Array.isArray(m.material) ? m.material[0] : m.material;
      const lit = !!mat && ((mat as THREE.MeshStandardMaterial).isMeshStandardMaterial || (mat as THREE.MeshLambertMaterial).isMeshLambertMaterial);
      const solid = lit && !mat.transparent && mat.visible !== false;
      m.castShadow = on && solid;
      m.receiveShadow = on && lit;
    });
    if (on) this.map(root);
  }

  // ---------- round 9 (Codex): normal and roughness maps, Ultra only ----------
  // A material whose colour map is one of the 19 pictures gets that picture's normal and
  // roughness maps laid the same way; a road canvas made from them carries a twin that is
  // drawn the same way (textures.ts dataTwin). The roughness in the map is absolute, so the
  // material's own factor goes to 1 while it is on. Off (High, Low), every material gets
  // back exactly what it had. The maps load the first time Ultra is switched on; until they
  // are there (or if they cannot load), Ultra runs without them.
  private saved = new Map<THREE.MeshStandardMaterial, { normalMap: THREE.Texture | null; roughnessMap: THREE.Texture | null; roughness: number }>();
  private loading = false;
  get mapped() { return this.saved.size; }
  private map(root: THREE.Object3D) {
    if (!ultraArtReady()) {
      if (this.loading) return;
      this.loading = true;
      loadArtFor(ULTRA_ART).then(() => { this.loading = false; if (this.on) this.map(this.scene); })
        .catch((e) => { this.loading = false; console.info('Ultra runs without the round 9 maps:', e?.message ?? e); });
      return;
    }
    root.traverse((o) => {
      const m = (o as THREE.Mesh).material;
      if (m) for (const x of Array.isArray(m) ? m : [m]) this.mapOne(x);
    });
  }
  private mapOne(mat: THREE.Material) {
    const m = mat as THREE.MeshStandardMaterial;
    if (!m.isMeshStandardMaterial || !m.map || this.saved.has(m) || m.normalMap || m.roughnessMap) return;
    let normal: THREE.Texture, roughness: THREE.Texture;
    const twin = m.map.userData?.dataTwin as ((like: THREE.Texture) => { normal: THREE.Texture; roughness: THREE.Texture }) | undefined;
    if (twin) ({ normal, roughness } = twin(m.map));
    else {
      const pair = (ULTRA_MAPS as Record<string, [ArtId, ArtId]>)[m.map.name.startsWith('art/') ? m.map.name.slice(4) : ''];
      if (!pair) return;
      normal = dataTextureLike(pair[0], m.map); roughness = dataTextureLike(pair[1], m.map);
    }
    this.saved.set(m, { normalMap: m.normalMap, roughnessMap: m.roughnessMap, roughness: m.roughness });
    m.normalMap = normal; m.roughnessMap = roughness; m.roughness = 1;
    m.needsUpdate = true;
  }
  private unmap() {
    for (const [m, o] of this.saved) { m.normalMap = o.normalMap; m.roughnessMap = o.roughnessMap; m.roughness = o.roughness; m.needsUpdate = true; }
    this.saved.clear();
  }

  // Hand the lamp back to its fake light.
  private release(k: number) {
    const h = this.held[k];
    if (h) {
      const p = h.set.pos[h.i];
      if (p.w === OFF) p.w = h.want;
    }
    this.held[k] = null;
    this.park(this.spots[k]);
  }
  // An unused light stays in the scene (switching lights on and off would rebuild every
  // shader), dark and far under the ground where its shadow map has nothing to draw.
  private park(s: THREE.SpotLight) {
    s.intensity = 0;
    s.shadow.needsUpdate = true;   // once, so it holds nothing; then it costs nothing
    s.position.set(0, -500, 0);
    s.target.position.set(0, -600, 0.01);
    s.target.updateMatrixWorld();
  }

  private shadowT = 0;
  /** `keys`: the flood sets that may hand over lamps where the player is (main.ts picks them:
   *  the yard outside, the records room in the annex, none in the control room).
   *  `extras`: whether the lights added with addShadowLight are where the player is. */
  update(dt: number, at: THREE.Vector3, keys: readonly string[] | null = null, extras = true) {
    if (!this.on) return;
    this.shadowT -= dt;
    if (this.shadowT <= 0) {
      this.shadowT = 1 / SHADOW_HZ;
      for (let k = 0; k < POOL; k++) if (this.held[k]) this.spots[k].shadow.needsUpdate = true;
      if (extras) for (const l of this.extra) if (l.visible && l.intensity > 0) l.shadow.needsUpdate = true;
    }
    // what the game wants from each held lamp: it may have dimmed or switched it meanwhile
    for (const h of this.held) if (h) { const w = h.set.pos[h.i].w; if (w !== OFF) h.want = w; }
    this.t -= dt;
    if (this.t <= 0) {
      this.t = 0.25;
      // the three nearest overhead lamps within reach
      const near: { set: FloodSet; i: number; d: number }[] = [];
      for (const set of floodSets) {
        if (set.key === 'cab' || (keys && !keys.includes(set.key))) continue;
        for (let i = 0; i < set.count; i++) {
          if (set.skip.has(i)) continue;
          const p = set.pos[i];
          const w = p.w === OFF ? (this.held.find((h) => h && h.set === set && h.i === i)?.want ?? 0) : p.w;
          // overhead lamps only; the wide area floods over the array stay fake (a spot would
          // shrink their pools to a few metres)
          if (w < 2.5 || w > 20 || p.y < 1.8) continue;
          const d = Math.hypot(p.x - at.x, p.z - at.z);
          if (d < 40) near.push({ set, i, d });
        }
      }
      near.sort((a, b) => a.d - b.d);
      const pick = near.slice(0, POOL);
      // keep what is still picked, release the rest, then fill the free slots
      for (let k = 0; k < POOL; k++) {
        const h = this.held[k];
        if (h && !pick.some((q) => q.set === h.set && q.i === h.i)) this.release(k);
      }
      for (const q of pick) {
        if (this.held.some((h) => h && h.set === q.set && h.i === q.i)) continue;
        const k = this.held.indexOf(null);
        if (k < 0) break;
        const p = q.set.pos[q.i];
        this.held[k] = { set: q.set, i: q.i, want: p.w };
        p.w = OFF;
        const s = this.spots[k];
        s.position.set(p.x, p.y, p.z);
        s.target.position.set(p.x, 0, p.z + 0.01);
        s.target.updateMatrixWorld();
        s.color.copy(q.set.col[q.i]);
        s.shadow.needsUpdate = true;
      }
    }
    // brightness: match the fake pool right under the lamp (kit.ts: w * win^2 / (1 + d^2 * 0.012) * scale)
    for (let k = 0; k < POOL; k++) {
      const h = this.held[k], s = this.spots[k];
      if (!h) continue;
      const p = h.set.pos[h.i], y = Math.max(1, p.y), reach = h.want * 1.6 + 8;
      const win = Math.max(0, 1 - y / reach);
      s.intensity = Math.PI * y * y * h.want * win * win / (1 + y * y * 0.012) * h.set.scale.value * 1.6;
      s.distance = reach;
    }
  }
}
