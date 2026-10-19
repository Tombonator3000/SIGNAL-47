import * as THREE from 'three';
import { floodlit, flood } from './kit';
import { artTexture } from '../core/art';
import houseUrl from '../assets/models/saro_house.glb?url';
import truckUrl from '../assets/models/saro_truck.glb?url';
import type { Truck, TruckModel } from '../drive/Truck';

// The Blender pilot (Tom, 19 October): SARO's house modelled from the game's own boxes in Blender,
// its occlusion, dirt and worn edges baked on the graphics card (tools/blender/build.py saro), in
// place of the boxes the game builds. Off unless the address has ?blender (or a test sets
// S47.blender); only then are the .glb and three's GLTFLoader fetched (their own files in the
// Pages build; inside the single file they are part of it). The code-built shell is hidden, never
// taken away: its colliders, zones, doors, proxies, lamps, stains and lettering are the game's as
// before, and the rays still hit the hidden boxes (the model itself takes no rays).
// Step two: SARO 07, the service truck, the same way (tools/blender/build.py truck): every copy
// of it (on SARO's pad and the motel's lot, on the road and the diner's lot, at STATION 01, on
// the old road) gets the modelled body, cab and wheel, in its own area's floods (Truck.setModel).

export const blenderWanted = (() => { try { return new URLSearchParams(location.search).has('blender'); } catch { return false; } })();

/** The baked mask (the glTF occlusion texture, three's aoMap on the second UV set): R occlusion,
 *  G dirt, B worn edges. The occlusion darkens the sky's fill (three's own aomap) and the fake
 *  floods (kit.floodlit); here the dirt darkens and warms the colour and the worn edges lighten it. */
function baked<T extends THREE.MeshStandardMaterial>(m: T, dirt: number, wear: number): T {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.fragmentShader = sh.fragmentShader.replace('#include <map_fragment>', `#include <map_fragment>
      #ifdef USE_AOMAP
        vec3 bakedMask = texture2D( aoMap, vAoMapUv ).rgb;
        diffuseColor.rgb *= mix( vec3( 1.0 ), vec3( 0.46, 0.4, 0.33 ), bakedMask.g * ${dirt.toFixed(3)} );
        diffuseColor.rgb = mix( diffuseColor.rgb, min( diffuseColor.rgb * 1.3 + 0.05, vec3( 1.0 ) ), bakedMask.b * ${wear.toFixed(3)} );
      #endif`);
  };
  m.customProgramCacheKey = () => key.call(m) + `baked${dirt}/${wear}`;
  return m;
}

/** A lit pane's glow falls off from the window's head (2.34) to its sill (0.58), and differs a
 *  little from room to room (kit.floodlit's world position). */
function roomLight(m: THREE.MeshStandardMaterial) {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.fragmentShader = sh.fragmentShader.replace('#include <emissivemap_fragment>', `#include <emissivemap_fragment>
      totalEmissiveRadiance *= mix( 0.45, 1.15, smoothstep( 0.6, 2.3, vFWorld.y ) ) * ( 0.85 + 0.15 * sin( vFWorld.x * 1.7 + vFWorld.z ) );`);
  };
  m.customProgramCacheKey = () => key.call(m) + 'room';
}

/** The materials as the game wants them: the concrete picture tiling on the first UV set (and its
 *  normal map on Ultra, core/ultra.ts, which knows art/concrete), the lamps of SARO's site set at
 *  the falloffs the boxes had (M.concrete's 0.012, the wing's 0.03), the mask, and the panes. */
function dress(root: THREE.Object3D) {
  root.traverse((o) => {
    const mesh = o as THREE.Mesh;
    if (!mesh.isMesh) return;
    mesh.raycast = () => {};
    const m = mesh.material as THREE.MeshStandardMaterial;
    if (m.name === 'house' || m.name === 'wing') {
      m.map = artTexture('concrete', [1, 1]);
      floodlit(m, m.name === 'house' ? 0.012 : 0.03, flood);
      baked(m, 0.75, 0.45);
    } else if (m.name === 'glass_dark') {
      m.vertexColors = false; m.color.set(0x101418); m.roughness = 0.4; m.metalness = 0;
      floodlit(m, 0.03, flood);
    } else if (m.name === 'glass_lit') {
      // the rooms behind, lit from their ceilings, through the blinds the dressing lets down
      // (dressSaro.ts): brighter at the head of the window than down by the desks
      m.vertexColors = false; m.color.set(0x2a2219); m.roughness = 0.4;
      m.emissive.set(0xd98a48); m.emissiveIntensity = 0.75;
      floodlit(m, 0.03, flood);
      roomLight(m);
    }
    m.needsUpdate = true;
  });
}

/** The .glb's bytes: an address in the Pages build, a data URI in the single file (decoded here,
 *  so a strict content policy without connect-src does not stop it). */
async function bytes(url: string) {
  if (url.startsWith('data:')) {
    const s = atob(url.slice(url.indexOf(',') + 1)), b = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) b[i] = s.charCodeAt(i);
    return b.buffer;
  }
  const r = await fetch(url);
  if (!r.ok) throw new Error(`${url}: ${r.status}`);
  return r.arrayBuffer();
}

/** The baked mask on the truck (R occlusion, G dirt, B worn edges), after the vertex colours:
 *  the road's dust warms and darkens what is light and greys what is dark (the tyres, the
 *  rubber), dulls the paint and the chrome; worn edges show the primer through the paint, pits
 *  in the chrome, scuffs on the rubber and the rims. */
const WEAR: Record<string, string> = {
  paint: 'mix( diffuseColor.rgb, vec3( 0.16, 0.12, 0.09 ), bakedMask.b * 0.55 )',
  chrome: 'diffuseColor.rgb * ( 1.0 - bakedMask.b * 0.45 )',
  dark: 'mix( diffuseColor.rgb, vec3( 0.075 ), bakedMask.b * 0.6 )',
  wheel: 'mix( diffuseColor.rgb, min( diffuseColor.rgb, vec3( 0.18, 0.15, 0.12 ) ), bakedMask.b * 0.7 )',
};
function truckBaked(m: THREE.MeshStandardMaterial, kind: string) {
  if (!m.aoMap || !WEAR[kind]) return;
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  // (the chrome takes less of it: a bumper should still read as chrome under the dust)
  const dirt = kind === 'chrome' ? 0.55 : 0.9, rough = kind === 'chrome' ? 0.35 : 0.7;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <color_fragment>', `#include <color_fragment>
        #ifdef USE_AOMAP
          vec3 bakedMask = texture2D( aoMap, vAoMapUv ).rgb;
          diffuseColor.rgb = mix( diffuseColor.rgb, max( diffuseColor.rgb * vec3( 0.62, 0.5, 0.37 ), vec3( 0.15, 0.12, 0.09 ) ), bakedMask.g * ${dirt.toFixed(2)} );
          diffuseColor.rgb = ${WEAR[kind]};
        #endif`)
      .replace('#include <roughnessmap_fragment>', `#include <roughnessmap_fragment>
        #ifdef USE_AOMAP
          roughnessFactor = mix( roughnessFactor, 0.95, bakedMask.g * ${rough.toFixed(2)} );
        #endif`);
  };
  m.customProgramCacheKey = () => key.call(m) + 'truckBaked' + kind;
  m.needsUpdate = true;
}

/** A .glb parsed with three's GLTFLoader (fetched only now, with the flag). */
async function parse(url: string) {
  const [{ GLTFLoader }, data] = await Promise.all([import('three/examples/jsm/loaders/GLTFLoader.js'), bytes(url)]);
  const loader = new GLTFLoader();
  // the masks through an <img> from a blob, not fetch (the same content policy)
  loader.register((parser) => { parser.textureLoader = new THREE.TextureLoader(parser.options.manager); return { name: 's47_img' }; });
  return loader.parseAsync(data, '');
}

export class BlenderHouse {
  /** In the SARO area's group, so it is shown and hidden with the area. */
  readonly group = new THREE.Group();
  on = false;
  private model: THREE.Object3D | null = null;
  private loading: Promise<void> | null = null;
  failed = '';

  /** replaced: the code-built meshes the model stands in for (ControlRoom.outside,
   *  RecordsAnnex.outside, Exterior.house). prepare: the graphics level for what was added late. */
  constructor(parent: THREE.Object3D, private replaced: THREE.Object3D[], private prepare: (root: THREE.Object3D) => void) {
    this.group.name = 'blender:house';
    this.group.visible = false;
    parent.add(this.group);
  }

  /** Loaded and in the picture. */
  get shown() { return this.on && !!this.model; }

  async set(on: boolean) {
    this.on = on;
    if (on && !this.model) {
      this.loading ??= this.load().catch((e) => { this.failed = String(e?.message ?? e); console.warn('the Blender house could not load:', this.failed); });
      await this.loading;
    }
    const shown = this.shown;
    for (const o of this.replaced) o.visible = !shown;
    this.group.visible = shown;
  }

  private async load() {
    const gltf = await parse(houseUrl);
    dress(gltf.scene);
    this.model = gltf.scene;
    this.group.add(gltf.scene);
    this.prepare(this.group);
  }
}

/** The truck: every copy the world builds is handed in (World.onTruck) and given the model,
 *  or the code's again, when the flag changes. */
export class BlenderTruck {
  on = false;
  failed = '';
  readonly trucks = new Set<Truck>();
  private model: TruckModel | null = null;
  private loading: Promise<void> | null = null;

  /** prepare: the graphics level for what was added late. */
  constructor(private prepare: (root: THREE.Object3D) => void) {}

  /** Loaded and in the picture. */
  get shown() { return this.on && !!this.model; }

  add(t: Truck) {
    this.trucks.add(t);
    if (this.shown) { t.setModel(this.model); this.prepare(t.group); }
  }

  async set(on: boolean) {
    this.on = on;
    if (on && !this.model) {
      this.loading ??= this.load().catch((e) => { this.failed = String(e?.message ?? e); console.warn('the Blender truck could not load:', this.failed); });
      await this.loading;
    }
    for (const t of this.trucks) {
      if (t.modelled === this.shown) continue;
      t.setModel(this.shown ? this.model : null);
      this.prepare(t.group);
    }
  }

  private async load() {
    const gltf = await parse(truckUrl);
    const parts: TruckModel['parts'] = [];
    let wheel: TruckModel['wheel'] | null = null;
    gltf.scene.traverse((o) => {
      const mesh = o as THREE.Mesh;
      if (!mesh.isMesh) return;
      const m = mesh.material as THREE.MeshStandardMaterial, g = mesh.geometry;
      // (the meshes stand at the truck's origin, unturned: their nodes carry no transform)
      if (m.name === 'wheel') { wheel = { geometry: g, aoMap: m.aoMap }; return; }
      const [group, kind] = m.name.split('_') as [TruckModel['parts'][number]['group'], TruckModel['parts'][number]['kind']];
      parts.push({ group, kind, geometry: g, aoMap: m.aoMap });
    });
    if (!wheel || parts.length < 7) throw new Error(`the truck model is not whole (${parts.length} parts${wheel ? '' : ', no wheel'})`);
    this.model = { parts, wheel, dress: truckBaked };
  }
}
