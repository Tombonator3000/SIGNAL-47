import * as THREE from 'three';
import { floodlit, flood } from './kit';
import { artTexture } from '../core/art';
import type { GlowPoints } from './glow';
import houseUrl from '../assets/models/saro_house.glb?url';
import truckUrl from '../assets/models/saro_truck.glb?url';
import type { Truck, TruckModel } from '../drive/Truck';

// The Blender pilot (Tom, 19 October): SARO's house modelled from the game's own boxes in Blender,
// its occlusion, dirt and worn edges and the light of its lamps baked on the graphics card
// (tools/blender/build.py saro), in place of the boxes the game builds. Off unless the address has
// ?blender (or a test sets S47.blender); only then are the .glb and three's GLTFLoader fetched
// (their own files in the Pages build; inside the single file they are part of it). The
// code-built shell is hidden, never taken away: its colliders, zones, doors, proxies, lamps,
// stains and lettering are the game's as before, and the rays still hit the hidden boxes (the
// model itself takes no rays).
// Step two: SARO 07, the service truck, the same way (tools/blender/build.py truck): every copy
// of it (on SARO's pad and the motel's lot, on the road and the diner's lot, at STATION 01, on
// the old road) gets the modelled body, cab and wheel, in its own area's floods (Truck.setModel).
// The final pass (19 October): the house's lamps baked (their cones and shadows; the fake pools
// kept at half on the model, so the stains on the walls still match it), the wing's lit rooms
// behind deep reveals, the skirt of shade on the ground round the plinths, the fine detail of the
// concrete tiling on the first UV set; the truck's glass and chrome given the sky, its cab seen
// through the glass, its shade on the ground.

export const blenderWanted = (() => { try { return new URLSearchParams(location.search).has('blender'); } catch { return false; } })();

// ---------- what the materials share ----------
/** A world position and normal of the model's own (for what does not take the floods). */
function worldVaryings(sh: THREE.WebGLProgramParametersWithUniforms) {
  sh.vertexShader = sh.vertexShader
    .replace('#include <common>', '#include <common>\nvarying vec3 vS47W;\nvarying vec3 vS47N;')
    .replace('#include <begin_vertex>', `#include <begin_vertex>
      vS47W = ( modelMatrix * vec4( transformed, 1.0 ) ).xyz;
      vS47N = normalize( mat3( modelMatrix ) * objectNormal );`);
  sh.fragmentShader = sh.fragmentShader.replace('#include <common>', `#include <common>
    varying vec3 vS47W; varying vec3 vS47N;`).replace('void main() {', `
    // the sky as a reflection sees it: the zenith dark, the horizon lighter (the fog's colour,
    // the sky's own at the horizon, by night and at dawn), the ground under it lit by the lamps
    // round the thing (lamp: their fake light where it stands)
    vec3 s47Sky( vec3 r, vec3 lamp ) {
      #ifdef USE_FOG
        vec3 hor = fogColor * 2.4 + 0.004;
      #else
        vec3 hor = vec3( 0.02, 0.025, 0.04 );
      #endif
      vec3 zen = hor * 0.3, gnd = hor * 0.4 + lamp * 0.45;
      hor += lamp * 0.9;
      return r.y > 0.0 ? mix( hor, zen, smoothstep( 0.0, 0.45, r.y ) ) : mix( hor, gnd, smoothstep( 0.0, 0.12, -r.y ) );
    }
    void main() {`);
}

/** A reflection of the sky with Fresnel (glass f0 0.04, chrome much more), k its strength.
 *  Added to the emissive light: a pane is not a blob of the moon any more but the sky's gradient. */
function skyReflection(m: THREE.Material, f0: number, k: number, tag: string, lampK = 1) {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    worldVaryings(sh);
    const lamp = sh.fragmentShader.includes('vec3 fAcc') ? `fAcc * uFloodScale * ${lampK.toFixed(3)}` : 'vec3( 0.0 )';
    sh.fragmentShader = sh.fragmentShader.replace('#include <lights_fragment_begin>', `{
        vec3 sV = normalize( vS47W - cameraPosition ), sN = normalize( vS47N );
        if ( dot( sN, sV ) > 0.0 ) sN = -sN;
        float sF = ${f0.toFixed(3)} + ${(1 - f0).toFixed(3)} * pow( 1.0 - clamp( dot( sN, -sV ), 0.0, 1.0 ), 5.0 );
        totalEmissiveRadiance += s47Sky( reflect( sV, sN ), ${lamp} ) * sF * ${k.toFixed(3)};
      }
      #include <lights_fragment_begin>`);
  };
  m.customProgramCacheKey = () => key.call(m) + 'sky' + tag;
}

/** A shade laid on what is under it (the skirt round the house, the truck's shade): the map's
 *  red multiplies the picture, fading to nothing in the fog. */
function multiply(map: THREE.Texture, k = 1) {
  const m = new THREE.MeshBasicMaterial({ map, transparent: true, depthWrite: false, blending: THREE.MultiplyBlending, premultipliedAlpha: true,
    toneMapped: false, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 });
  m.onBeforeCompile = (sh) => {
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <map_fragment>', `#ifdef USE_MAP
          diffuseColor = vec4( vec3( mix( 1.0, texture2D( map, vMapUv ).r, ${k.toFixed(3)} ) ), 1.0 );
        #endif`)
      .replace('#include <fog_fragment>', `#ifdef USE_FOG
          #ifdef FOG_EXP2
            float s47Fog = 1.0 - exp( - fogDensity * fogDensity * vFogDepth * vFogDepth );
          #else
            float s47Fog = smoothstep( fogNear, fogFar, vFogDepth );
          #endif
          gl_FragColor.rgb = mix( gl_FragColor.rgb, vec3( 1.0 ), s47Fog );
        #endif`);
  };
  m.customProgramCacheKey = () => 'multiply' + k;
  return m;
}

// ---------- the house ----------
/** The concrete's fine detail, tiling on the first UV set (the art/concrete picture's, 2.5 m to
 *  a repeat): R the cast walls' form panels (1.575 m by the 1.2 m lifts: the joints, four tie
 *  holes), G the wing's painted block (two blocks by two courses, half bond). */
let detailTex: THREE.Texture | null = null;
function detail() {
  if (detailTex) return detailTex;
  const S = 256, c = document.createElement('canvas');
  c.width = c.height = S;
  const g = c.getContext('2d')!, img = g.createImageData(S, S);
  const line = (d: number, w: number) => Math.min(1, Math.max(0, w + 0.5 - d));
  for (let y = 0; y < S; y++) for (let x = 0; x < S; x++) {
    const u = (x + 0.5) / S, v = (y + 0.5) / S;
    // form panels: the joints on the tile's edges, the ties a quarter in
    let r = Math.max(line(Math.min(u, 1 - u) * S, 0.7), line(Math.min(v, 1 - v) * S, 0.6));
    for (const tu of [0.25, 0.75]) for (const tv of [0.25, 0.75]) r = Math.max(r, line(Math.hypot((u - tu) * S, (v - tv) * S * 1.575 / 1.2), 0.9) * 0.9);
    // block: the bed joints at 0 and a half, the heads half a block over every other course
    const ku = v < 0.5 ? u : (u + 0.25) % 1, hd = Math.min(ku % 0.5, 0.5 - (ku % 0.5)) * S;
    const b = Math.max(line(Math.min(v % 0.5, 0.5 - (v % 0.5)) * S, 1.1), line(hd, 0.6) * 0.85);
    const i = (y * S + x) * 4;
    img.data[i] = r * 255; img.data[i + 1] = b * 255; img.data[i + 2] = 0; img.data[i + 3] = 255;
  }
  g.putImageData(img, 0, 0);
  const t = new THREE.CanvasTexture(c);
  t.wrapS = t.wrapT = THREE.RepeatWrapping; t.colorSpace = THREE.NoColorSpace; t.anisotropy = 4;
  return (detailTex = t);
}

/** The baked mask (the glTF occlusion texture, three's aoMap on the second UV set): R occlusion,
 *  G dirt, B worn edges. The occlusion darkens the sky's fill (three's own aomap) and the fake
 *  floods (kit.floodlit); here the dirt darkens and warms the colour and the worn edges lighten
 *  it; the detail on the first set (detail()) darkens the joints of what is concrete (its
 *  vertex colour is warm; the metal's is grey) on the walls. */
function baked<T extends THREE.MeshStandardMaterial>(m: T, dirt: number, wear: number, wing: boolean) {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  const scale = wing ? 'vec2( 2.5 / 0.8128, 2.5 / 0.4064 )' : 'vec2( 2.5 / 1.575, 2.5 / 1.2 )';
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.uniforms.uDetail = { value: detail() };
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', '#include <common>\nuniform sampler2D uDetail;')
      .replace('#include <map_fragment>', `#include <map_fragment>
      #ifdef USE_AOMAP
        vec3 bakedMask = texture2D( aoMap, vAoMapUv ).rgb;
        diffuseColor.rgb *= mix( vec3( 1.0 ), vec3( 0.46, 0.4, 0.33 ), bakedMask.g * ${dirt.toFixed(3)} );
        diffuseColor.rgb = mix( diffuseColor.rgb, min( diffuseColor.rgb * 1.3 + 0.05, vec3( 1.0 ) ), bakedMask.b * ${wear.toFixed(3)} );
      #endif
      #if defined( USE_MAP ) && defined( USE_COLOR )
        {
          vec2 dUv = vec2( vMapUv.x, 1.0 - vMapUv.y ) * ${scale};
          float dOn = smoothstep( 0.03, 0.08, vColor.r - vColor.b ) * ( 1.0 - step( 0.35, abs( vFNormal.y ) ) ) * step( 0.02, vFWorld.y ) * step( vFWorld.y, 3.56 );
          diffuseColor.rgb *= 1.0 - texture2D( uDetail, dUv ).${wing ? 'g' : 'r'} * dOn * ${wing ? '0.28' : '0.4'};
        }
      #endif`);
  };
  m.customProgramCacheKey = () => key.call(m) + `baked${dirt}/${wear}/${wing}`;
  return m;
}

type Lamp = { name: string; flood: number[]; w: number; ch: number; at: number[]; fits: number[][] };
type LampInfo = { lamps: Lamp[]; max: number; fake: number; skirt: number };

/** The flood of the site's set that stands where a baked lamp's does (-1: none). */
const floodAt = (p: number[]) => {
  for (let i = 0; i < flood.count; i++) {
    const q = flood.pos[i];
    if (Math.hypot(q.x - p[0], q.y - p[1], q.z - p[2]) < 0.05) return i;
  }
  return -1;
};

/** The lamps' light baked in Cycles (the glTF emissive texture, here the material's own map on
 *  the lightmap's set): in each channel the lamps that do not light the same walls, a texel
 *  taking the strength of its channel's nearest lamp as the game has it now (setFlood; Ultra
 *  takes the nearest for real lights with shadows: then its baked light goes, as its fake does),
 *  in that lamp's colour, on the surface's colour, at the set's scale. The lamps' fake pools stay
 *  on the model at `fake` of their strength (kit.floodlit's weights): the stains on the walls
 *  take the fake ones whole, and must not shine against a wall gone darker than they are. */
function lampLight(m: THREE.MeshStandardMaterial, map: THREE.Texture, info: LampInfo, falloff: number) {
  const idx = info.lamps.map((l) => floodAt(l.flood));
  info.lamps.forEach((l, i) => { if (idx[i] < 0) console.warn('the Blender house: no lamp of the site at', l.flood, `(${l.name})`); });
  const weights: Record<number, number> = {};
  idx.forEach((i) => { if (i >= 0) weights[i] = info.fake; });
  floodlit(m, falloff, flood, weights);
  const f = (v: number) => v.toFixed(3);
  const chans = [0, 1, 2].map((c) => {
    const ls = info.lamps.map((l, i) => ({ l, i: idx[i] })).filter((x) => x.l.ch === c && x.i >= 0);
    if (!ls.length) return '';
    return `bd = 1e9; fc = vec3( 0.0 );
      ${ls.map(({ l, i }) => `d = distance( vFWorld, vec3( ${f(l.at[0])}, ${f(l.at[1])}, ${f(l.at[2])} ) ); if ( d < bd ) { bd = d; fc = uFloodCol[${i}] * ( max( uFloodPos[${i}].w, 0.0 ) / ${f(l.w)} ); }`).join('\n      ')}
      lAcc += lL.${'rgb'[c]} * fc;`;
  }).join('\n      ');
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  map.colorSpace = THREE.NoColorSpace; map.needsUpdate = true;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.uniforms.uLampMap = { value: map };
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', '#include <common>\nuniform sampler2D uLampMap;')
      .replace('#include <lights_fragment_begin>', `#ifdef USE_AOMAP
        {
          vec3 lT = texture2D( uLampMap, vAoMapUv ).rgb, lL = lT * lT * ${f(info.max)}, lAcc = vec3( 0.0 ), fc;
          float bd, d;
          ${chans}
          totalEmissiveRadiance += lAcc * diffuseColor.rgb * ( uFloodScale / 0.07 );
        }
      #endif
      #include <lights_fragment_begin>`);
  };
  m.customProgramCacheKey = () => key.call(m) + 'lamps';
}

/** The materials as the game wants them: the concrete picture tiling on the first UV set (and its
 *  normal map on Ultra, core/ultra.ts, which knows art/concrete), the lamps of SARO's site set at
 *  the falloffs the boxes had (M.concrete's 0.012, the wing's 0.03) with the baked ones' light,
 *  the mask, the rooms behind the lit panes, the skirt's shade. */
function dress(root: THREE.Object3D) {
  root.traverse((o) => {
    const mesh = o as THREE.Mesh;
    if (!mesh.isMesh) return;
    mesh.raycast = () => {};
    const m = mesh.material as THREE.MeshStandardMaterial;
    if (m.name === 'house' || m.name === 'wing') {
      const info = m.userData as LampInfo, lamp = m.emissiveMap;
      m.map = artTexture('concrete', [1, 1]);
      m.emissiveMap = null; m.emissive.set(0);
      if (lamp && info.lamps) lampLight(m, lamp, info, m.name === 'house' ? 0.012 : 0.03);
      else floodlit(m, m.name === 'house' ? 0.012 : 0.03, flood);
      // (the dirt a notch under the first pass's: the code-built neighbours carry none)
      baked(m, 0.6, 0.4, m.name === 'wing');
    } else if (m.name === 'glass_dark') {
      // dark rooms: no blob of the moon (rough), the sky's gradient instead, the lamps a little
      m.vertexColors = false; m.color.set(0x040506); m.roughness = 1; m.metalness = 0;
      floodlit(m, 0.03, flood);
      skyReflection(m, 0.04, 0.55, 'pane', 0.25);
    } else if (m.name === 'glass_lit') {
      // the rooms behind (rooms.png: the ceiling's light down the back wall, what stands against
      // it), deep amber at a level the tone mapping keeps orange; not lit by the floods, a faint
      // sky on the glass
      m.vertexColors = false; m.color.set(0x0a0806); m.roughness = 1; m.metalness = 0;
      m.emissive.set(0xff8a3c); m.emissiveIntensity = 1.25;
      skyReflection(m, 0.04, 0.4, 'lit');
    } else if (m.name === 'skirt') {
      const sk = multiply(m.aoMap!, 1);
      sk.name = 'skirt';
      mesh.material = sk; mesh.renderOrder = 1;
      return;
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
 *  rubber), dulls the paint; worn edges show the primer through the paint, pits in the chrome,
 *  scuffs on the rubber and the rims. The chrome keeps its dust to the lower lip and its shine:
 *  little colour of its own, the sky and the lamps round it in it (skyReflection). */
const WEAR: Record<string, string> = {
  paint: 'mix( diffuseColor.rgb, vec3( 0.16, 0.12, 0.09 ), bakedMask.b * 0.55 )',
  chrome: 'diffuseColor.rgb * ( 1.0 - bakedMask.b * 0.3 )',
  dark: 'mix( diffuseColor.rgb, vec3( 0.075 ), bakedMask.b * 0.6 )',
  wheel: 'mix( diffuseColor.rgb, min( diffuseColor.rgb, vec3( 0.18, 0.15, 0.12 ) ), bakedMask.b * 0.7 )',
};
function truckBaked(m: THREE.MeshStandardMaterial, kind: string) {
  // (little colour of its own, for the floods to light flat: the shine is the reflection's, bright
  // where a face looks at the lit yard round it, dark where it looks up at the night)
  if (kind === 'chrome') { m.color.set(0x5e6265); m.roughness = 0.2; m.metalness = 0.85; skyReflection(m, 0.5, 0.85, 'chrome', 0.55); }
  if (kind === 'glass') {
    // a faint sky on the glass and the cab through it (the bench, its headrests, the wheel: in the
    // dark trim's vertex colours, truck.py)
    m.opacity = 0.58; m.roughness = 0.08;
    skyReflection(m, 0.04, 2.2, 'glass');
  }
  // the dark trim and, in its vertex colours, what shows of the cab through the glass (the
  // vinyl's colour, the trim its share of it: truck.py)
  if (kind === 'dark') m.color.set(0x5a4c3e);
  if (!m.aoMap || !WEAR[kind]) { m.needsUpdate = true; return; }
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey;
  // (the chrome takes its dust low down only, under 0.5 m: the bumper's lower lip, not its face)
  const dirt = kind === 'chrome' ? '0.75 * ( 1.0 - smoothstep( 0.42, 0.5, vFWorldLocalY ) )' : '0.75', rough = kind === 'chrome' ? 0.3 : 0.7;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying float vFWorldLocalY;')
      .replace('#include <begin_vertex>', '#include <begin_vertex>\nvFWorldLocalY = transformed.y;');
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', '#include <common>\nvarying float vFWorldLocalY;')
      .replace('#include <color_fragment>', `#include <color_fragment>
        #ifdef USE_AOMAP
          vec3 bakedMask = texture2D( aoMap, vAoMapUv ).rgb;
          diffuseColor.rgb = mix( diffuseColor.rgb, max( diffuseColor.rgb * vec3( 0.62, 0.5, 0.37 ), vec3( 0.15, 0.12, 0.09 ) ), bakedMask.g * ${dirt} );
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
  /** The lamps' glows on the walls the model lights itself (their size with the model, without),
   *  and the blinds the dressing lets down in the lit windows (moved back into the reveals). */
  private glowsAt: { g: GlowPoints; i: number; size: number; small: number }[] = [];
  private blinds: { attr: THREE.BufferAttribute; i: number[]; dz: number[]; back: boolean } | null = null;

  /** replaced: the code-built meshes the model stands in for (ControlRoom.outside,
   *  RecordsAnnex.outside, Exterior.house). prepare: the graphics level for what was added late.
   *  glows: the glows of the lamps on the house's walls (Exterior's, the annex's). props: the
   *  dressing's props (dressSaro.ts), where the lit windows' blinds are. */
  constructor(parent: THREE.Object3D, private replaced: THREE.Object3D[], private prepare: (root: THREE.Object3D) => void,
    private glows: GlowPoints[] = [], private props: () => THREE.Mesh | null = () => null) {
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
    this.fit(shown);
  }

  /** The glows and the blinds as the model wants them, or back as the code had them. */
  private fit(shown: boolean) {
    const touched = new Set<THREE.BufferAttribute>();
    for (const e of this.glowsAt) {
      const a = e.g.points.geometry.getAttribute('aSize') as THREE.BufferAttribute;
      a.setX(e.i, shown ? e.small : e.size); touched.add(a);
    }
    touched.forEach((a) => { a.needsUpdate = true; });
    const b = this.blinds;
    if (b && b.back !== shown) {
      b.i.forEach((i, k) => b.attr.setZ(i, b.attr.getZ(i) + (shown ? b.dz[k] : -b.dz[k])));
      b.back = shown; b.attr.needsUpdate = true;
    }
  }

  private async load() {
    const gltf = await parse(houseUrl);
    let info: LampInfo | null = null;
    gltf.scene.traverse((o) => { const m = (o as THREE.Mesh).material as THREE.Material | undefined; if (m && m.name === 'house') info = m.userData as LampInfo; });
    dress(gltf.scene);
    this.model = gltf.scene;
    this.group.add(gltf.scene);
    this.prepare(this.group);
    // the glows of the fittings whose light is baked: a lamp's own small glow, not a disc on the
    // wall (the light on the wall is the bake's now)
    const fits = (info as LampInfo | null)?.lamps.flatMap((l) => l.fits) ?? [];
    for (const g of this.glows) {
      const pos = g.points.geometry.getAttribute('position') as THREE.BufferAttribute, size = g.points.geometry.getAttribute('aSize') as THREE.BufferAttribute;
      if (!pos || !size) continue;
      for (let i = 0; i < pos.count; i++) {
        const near = fits.some((f) => Math.hypot(pos.getX(i) - f[0], pos.getY(i) - f[1], pos.getZ(i) - f[2]) < 0.5);
        if (near && size.getX(i) > 0.5) this.glowsAt.push({ g, i, size: size.getX(i), small: Math.min(size.getX(i), 0.45) });
      }
    }
    // the blinds in the wing's lit windows (weathering.window: slats just proud of the wall, at
    // z +-5.5): back into the 22 cm reveals, in front of the glass
    const props = this.props();
    if (props) {
      const a = props.geometry.getAttribute('position') as THREE.BufferAttribute, i: number[] = [], dz: number[] = [];
      for (let k = 0; k < a.count; k++) {
        const x = a.getX(k), y = a.getY(k), z = a.getZ(k);
        if (x < 20.6 || x > 38.6 || Math.abs(Math.abs(z) - 5.5) > 0.015 || y < 0.6 || y > 2.33) continue;
        const w = Math.floor((x - 20.6) / 2.25), a0 = 20.6 + 2.25 * w + 0.4, a1 = a0 + 1.45;
        if (x < a0 + 0.004 || x > a1 - 0.004) continue;
        i.push(k); dz.push(-Math.sign(z) * 0.19);
      }
      if (i.length) this.blinds = { attr: a, i, dz, back: false };
    }
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
    let wheel: TruckModel['wheel'] | null = null, shadow: TruckModel['shadow'] = null;
    gltf.scene.traverse((o) => {
      const mesh = o as THREE.Mesh;
      if (!mesh.isMesh) return;
      const m = mesh.material as THREE.MeshStandardMaterial, g = mesh.geometry;
      // (the meshes stand at the truck's origin, unturned: their nodes carry no transform)
      if (m.name === 'wheel') { wheel = { geometry: g, aoMap: m.aoMap }; return; }
      if (m.name === 'shadow') {
        if (m.map) { m.map.colorSpace = THREE.NoColorSpace; m.map.needsUpdate = true; }
        const sm = multiply(m.map!, 1); sm.name = 'truck shadow';
        shadow = { geometry: g, material: sm };
        return;
      }
      const [group, kind] = m.name.split('_') as [TruckModel['parts'][number]['group'], TruckModel['parts'][number]['kind']];
      parts.push({ group, kind, geometry: g, aoMap: m.aoMap });
    });
    if (!wheel || parts.length < 7) throw new Error(`the truck model is not whole (${parts.length} parts${wheel ? '' : ', no wheel'})`);
    // the shade is culled with the body (its quad reaches past the truck: at the picture's edge it
    // would cost a draw call for a sliver of shade when the truck is out of it)
    const body = parts.find((q) => q.group === 'body' && q.kind === 'paint');
    if (shadow && body) { body.geometry.computeBoundingSphere(); (shadow as NonNullable<TruckModel['shadow']>).geometry.boundingSphere = body.geometry.boundingSphere!.clone(); }
    this.model = { parts, wheel, shadow, dress: truckBaked };
  }
}
