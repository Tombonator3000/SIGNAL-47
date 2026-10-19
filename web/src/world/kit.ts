import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { artTexture } from '../core/art';

// ---------- Exterior floodlights ----------
// Real three.js point lights cost every material a loop per light. Outside we fake
// sodium floodlights with a tiny shader add-on that only exterior materials pay for.
// A material belongs to one set of fake lights. The big outdoor set lights the site;
// small sets light a single interior (the photo lab) without paying for the site.
/** skip: lamps never handed to a real light in the PC tier (core/ultra.ts), because they move
 *  (the truck's headlights) or copy another set's lamps (World: the diner and the road).
 *  head: the slots that hold the driven truck's three headlight pools (headSlots), -1 for none. */
export type FloodSet = { n: number; pos: THREE.Vector4[]; col: THREE.Color[]; count: number; scale: { value: number }; key: string; skip: Set<number>; head: { value: THREE.Vector3 } };
// Every set made, so the PC tier can swap the nearest lamps for real lights with
// shadows (core/ultra.ts) without importing the areas that own them.
export const floodSets: FloodSet[] = [];
export function floodSet(n: number, key: string, scale = 0.07): FloodSet {
  const set: FloodSet = {
    n, key, count: 0, scale: { value: scale }, skip: new Set(), head: { value: new THREE.Vector3(-1, -1, -1) },
    pos: Array.from({ length: n }, () => new THREE.Vector4(0, -999, 0, 0)),
    col: Array.from({ length: n }, () => new THREE.Color(1, 0.6, 0.25)),
  };
  floodSets.push(set);
  return set;
}
export const FLOOD_N = 25;   // three of them for the truck's headlights when it is driven at SARO
export const flood = floodSet(FLOOD_N, 'site');
// The south corridor and the records room (chapter two). The control room's corridor
// door uses it too, so the open door is lit from the corridor side.
export const annexFlood = floodSet(6, 'annex', 0.2);

// Returns the index, so a lamp can be dimmed or switched later with setFlood().
export function addFlood(x: number, y: number, z: number, intensity: number, color: THREE.ColorRepresentation = 0xff9a45, set = flood) {
  if (set.count >= set.n) { console.warn('flood set full', set.key); return -1; }
  set.pos[set.count].set(x, y, z, intensity);
  set.col[set.count].set(color);
  return set.count++;
}
export function setFlood(i: number, intensity: number, set = flood) { if (i >= 0) set.pos[i].w = intensity; }

// ---------- the headlights' pattern ----------
// The driven truck's headlights are three fake pools in a row down the road (Truck.headlightFloods,
// FLOODS), in the floods of the area it drives in, and copied into the diner's, the town's and
// the motel's sets (their shine()). Round pools light the road like a lamp on a pole, behind and
// beside the truck as well as ahead, with no edge. Those pools are drawn through the pattern of
// two sealed-beam lamps of the early eighties (Tom, 19 October, light cookies): a wide, flat
// spread, a brighter band just under the horizon (the low beam's hot spot, a little to the right),
// less above it (a wall in front shows the cut), the road close in front a good deal less, and
// faint, uneven streaks from the lens's flutes fanning out on the road under the cut. Nothing
// behind the lamps. The lamps' place and aim come from the
// first and the last pool (where they stand in the truck: Truck.ts FLOODS), so a copy in another
// set is drawn the same without being told more. Every tier: Ultra never hands these pools to a
// real light (they move). Low takes one lamp in the middle for the two, and only the broad streaks;
// a point past the reach of the first and the last pool pays nothing.
/** The sealed beams on (1) or the round pools as before (0), every set at once (tools/cookies.py). */
export const headCookie = { value: 1 };
/** These slots of a set hold the truck's three headlight pools (nearest first): drawn through the
 *  sealed beams' pattern, and never handed to a real light (core/ultra.ts). */
export function headSlots(set: FloodSet, slots: readonly number[]) {
  set.head.value.set(slots[0] ?? -1, slots[1] ?? -1, slots[2] ?? -1);
  for (const i of slots) if (i >= 0) set.skip.add(i);
}
const HEAD_GLSL = `
  // one lamp at d from the point (lamp to point), F its aim, R right, U up; s which of the two
  float fHeadLamp(vec3 d, vec3 F, vec3 R, vec3 U, float s) {
    float z = dot(d, F);
    if (z <= 0.05) return 0.0;
    float u = dot(d, R) / z, v = dot(d, U) / z;
    // wide and flat across, further to the right (the shoulder) than to the left (the oncoming
    // lane), ragged at its sides
    float uu = u - 0.05 + 0.025 * sin(v * 41.0 + s * 2.0);
    float side = 1.0 - smoothstep(uu < 0.0 ? 0.3 : 0.4, uu < 0.0 ? 0.72 : 0.95, abs(uu));
    // some light above the horizon (a sealed beam has no sharp cut), the bright band just under
    // it, and the road close in front a good deal less
    float up = mix(1.0, 0.16, smoothstep(-0.016, 0.07, v));
    float bv = (v + 0.028) / 0.024, bu = abs(u - 0.1);
    float band = 1.0 + 1.3 * exp(-bv * bv) * (1.0 - smoothstep(0.05, 0.5, bu));
    float low = 1.0 - 0.45 * smoothstep(0.04, 0.22, -v);
    // the flutes of the lens: faint, uneven streaks fanning out from the lamp, in the hot band
    // and under the cut, nothing above it (on a wall in front they stay low and short instead of
    // striping it to the roof like ribbed cladding: the art director, 19 October); the road far
    // ahead is where they show. Frequencies that never repeat together, and a little of the
    // vertical angle in them (Low: the broad ones)
    float fa = (0.05 + 0.15 * exp(-bv * bv)) * (1.0 - smoothstep(-0.03, 0.005, v));
    #ifdef LAMBERT
      float fl = 1.0 + fa * sin(u * 29.7 + v * 7.3 + 1.7) * sin(u * 11.83 - 1.0 + v * 3.1);
    #else
      float fl = 1.0 + fa * (sin(u * 29.7 + v * 7.3 + s * 1.7) * sin(u * 11.83 - s + v * 3.1) + 0.4 * sin(u * 71.9 - v * 13.0 + s * 3.1));
    #endif
    return side * up * band * low * fl;
  }
  // the pattern of the truck's two lamps at this point, from its first and last pool
  float fHead(vec3 p0, vec3 p2) {
    vec3 e = p2 - p0;
    // (the last pool stands 0.9 m higher than the first, 20.8 m on: Truck.ts FLOODS)
    vec3 F = normalize(vec3(e.x, e.y - 0.9, e.z));
    vec3 R = normalize(cross(F, vec3(0.0, 1.0, 0.0)));
    vec3 U = cross(R, F);
    // the lamps 5.16 m behind the first pool and 5 cm under it, 0.657 m either side of the middle
    vec3 d = vFWorld - (p0 - F * 5.16 - U * 0.05);
    #ifdef LAMBERT
      return fHeadLamp(d, F, R, U, 0.0);
    #else
      return 0.5 * (fHeadLamp(d + R * 0.657, F, R, U, -1.0) + fHeadLamp(d - R * 0.657, F, R, U, 1.0));
    #endif
  }`;

/** weights: some lamps of the set at a part of their strength on this material (world/blenderModels.ts:
 *  the lamps whose light the Blender house has baked in keep only a little of their fake pool there). */
export function floodlit<T extends THREE.MeshStandardMaterial>(m: T, falloff = 0.012, set = flood, weights?: Record<number, number>): T {
  const FLOOD_N = set.n;
  const ws = weights ? Object.entries(weights).filter(([i]) => +i >= 0) : [];
  const weigh = ws.length ? `float fWt = 1.0;\n${ws.map(([i, w]) => `          if (i == ${+i}) fWt = ${(+w).toFixed(3)};`).join('\n')}\n` : '';
  m.onBeforeCompile = (sh) => {
    sh.uniforms.uFloodPos = { value: set.pos };
    sh.uniforms.uFloodCol = { value: set.col };
    sh.uniforms.uFloodFall = { value: falloff };
    sh.uniforms.uFloodScale = set.scale;
    sh.uniforms.uFloodHead = set.head;
    sh.uniforms.uHeadCookie = headCookie;
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec3 vFWorld;\nvarying vec3 vFNormal;')
      .replace('#include <begin_vertex>', `#include <begin_vertex>
        vec4 fw = vec4(transformed, 1.0);
        #ifdef USE_INSTANCING
          fw = instanceMatrix * fw;
        #endif
        fw = modelMatrix * fw;
        vFWorld = fw.xyz;
        vec3 fn = objectNormal;
        #ifdef USE_INSTANCING
          fn = mat3(instanceMatrix) * fn;
        #endif
        vFNormal = normalize(mat3(modelMatrix) * fn);`);
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', `#include <common>
        varying vec3 vFWorld; varying vec3 vFNormal;
        uniform vec4 uFloodPos[${FLOOD_N}]; uniform vec3 uFloodCol[${FLOOD_N}];
        uniform float uFloodFall; uniform float uFloodScale;
        uniform vec3 uFloodHead; uniform float uHeadCookie;${HEAD_GLSL}`)
      .replace('#include <emissivemap_fragment>', `#include <emissivemap_fragment>
        vec3 fAcc = vec3(0.0);
        vec3 fN = normalize(vFNormal);
        int fHa = int(uFloodHead.x), fHb = int(uFloodHead.y), fHc = int(uFloodHead.z);
        float fHk = 1.0;
        if (fHa >= 0 && fHc >= 0 && uHeadCookie > 0.0 && uFloodPos[fHa].w > 0.0) {
          // only within the reach of the first or the last pool (the far land pays nothing)
          vec4 fH0 = uFloodPos[fHa], fH2 = uFloodPos[fHc];
          vec3 fE0 = vFWorld - fH0.xyz, fE2 = vFWorld - fH2.xyz;
          float fR0 = fH0.w * 1.6 + 8.0, fR2 = fH2.w * 1.6 + 8.0;
          if (dot(fE0, fE0) < fR0 * fR0 || dot(fE2, fE2) < fR2 * fR2)
            fHk = mix(1.0, sqrt(fHead(fH0.xyz, fH2.xyz)), uHeadCookie);
        }
        for (int i = 0; i < ${FLOOD_N}; i++) {
          if (uFloodPos[i].w <= 0.0) continue;
          vec3 L = uFloodPos[i].xyz - vFWorld;
          float d2 = dot(L, L);
          L *= inversesqrt(max(d2, 1e-4));
          float ndl = abs(dot(fN, L)) * 0.75 + 0.25;
          // windowed falloff: each lamp lights a pool, not the whole desert
          float win = clamp(1.0 - sqrt(d2) / (uFloodPos[i].w * 1.6 + 8.0), 0.0, 1.0);
          if (i == fHa || i == fHb || i == fHc) win *= fHk;   // (squared below: the headlights' pattern once)
          ${weigh}fAcc += uFloodCol[i] * (uFloodPos[i].w * ndl * win * win / (1.0 + d2 * uFloodFall))${ws.length ? ' * fWt' : ''};
        }
        #ifdef USE_AOMAP
          // a baked occlusion (world/blenderModels.ts) darkens the lamps' pools in the corners too
          fAcc *= mix( 1.0, texture2D( aoMap, vAoMapUv ).r, 0.6 );
        #endif
        totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;`);
  };
  m.customProgramCacheKey = () => 'flood' + set.key + falloff + (ws.length ? JSON.stringify(ws) : '');
  return m;
}

// ---------- Materials ----------
const std = (o: THREE.MeshStandardMaterialParameters) => new THREE.MeshStandardMaterial(o);
export const M = {
  deskTop: std({ color: 0x55605f, roughness: 0.5 }),
  deskBody: std({ color: 0x3d4749, roughness: 0.55, metalness: 0.35 }),
  steel: std({ color: 0x7c8387, roughness: 0.4, metalness: 0.7 }),
  beige: std({ color: 0xcfc3a3, roughness: 0.55 }),
  beigeDark: std({ color: 0xa69a7c, roughness: 0.6 }),
  darkPlastic: std({ color: 0x17191c, roughness: 0.6 }),
  chair: std({ color: 0x24272d, roughness: 0.95 }),
  cabinet: std({ color: 0x59615f, roughness: 0.5, metalness: 0.4 }),
  paper: std({ color: 0xe6dfcb, roughness: 0.9 }),
  glassDark: std({ color: 0x08100b, roughness: 0.15, metalness: 0.1 }),
  plant: std({ color: 0x3e5a2c, roughness: 0.9 }),
  pot: std({ color: 0x7a4a2e, roughness: 0.9 }),
  frame: std({ color: 0x2b3033, roughness: 0.5, metalness: 0.5 }),
  emissiveTube: new THREE.MeshBasicMaterial({ color: 0xfff1d6 }),
  emissiveTubeOff: new THREE.MeshStandardMaterial({ color: 0x8c8a82, roughness: 0.4 }),
  lampShade: std({ color: 0x1e2224, roughness: 0.45, metalness: 0.5, side: THREE.DoubleSide }),
  lampBulb: new THREE.MeshBasicMaterial({ color: 0xffd9a0 }),
  ceramic: std({ color: 0xece7db, roughness: 0.35 }),
  coffee: std({ color: 0x1c0f07, roughness: 0.15 }),
  ledRed: new THREE.MeshBasicMaterial({ color: 0xff3b2a }),
  ledGreen: new THREE.MeshBasicMaterial({ color: 0x45ff7a }),
  ledAmber: new THREE.MeshBasicMaterial({ color: 0xffb040 }),
  black: new THREE.MeshBasicMaterial({ color: 0x050505 }),
  // exterior, floodlit
  concrete: floodlit(std({ color: 0x8d8576, roughness: 0.95 })),
  dishWhite: floodlit(std({ color: 0xd8d2c4, roughness: 0.6, metalness: 0.15, side: THREE.DoubleSide })),
  dishMetal: floodlit(std({ color: 0xa8a39a, roughness: 0.55, metalness: 0.3 })),
  pole: floodlit(std({ color: 0x4a4a48, roughness: 0.7, metalness: 0.4 })),
};

// Called after artwork loading, before world construction and quality swapping.
export function initArtMaterials() {
  M.deskTop.map = artTexture('desk', [2, 1]); M.deskTop.color.set(0xbcc5c1);
  M.cabinet.map = artTexture('cabinet'); M.cabinet.color.set(0xa6b3af);
  M.concrete.map = artTexture('concrete'); M.concrete.color.set(0xd1c8b8);
}

// ---------- Mesh helpers ----------
export function box(parent: THREE.Object3D, w: number, h: number, d: number, mat: THREE.Material | THREE.Material[], x = 0, y = 0, z = 0) {
  const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), mat);
  m.position.set(x, y, z);
  parent.add(m);
  return m;
}
export function cyl(parent: THREE.Object3D, rt: number, rb: number, h: number, mat: THREE.Material, x = 0, y = 0, z = 0, seg = 12) {
  const m = new THREE.Mesh(new THREE.CylinderGeometry(rt, rb, h, seg), mat);
  m.position.set(x, y, z);
  parent.add(m);
  return m;
}
export function plane(parent: THREE.Object3D, w: number, h: number, mat: THREE.Material, x = 0, y = 0, z = 0, ry = 0, rx = 0) {
  const m = new THREE.Mesh(new THREE.PlaneGeometry(w, h), mat);
  m.position.set(x, y, z); m.rotation.set(rx, ry, 0, 'YXZ');
  parent.add(m);
  return m;
}
// Thin rod between two points (cables, truss members, feed legs).
const _up = new THREE.Vector3(0, 1, 0);
export function rod(parent: THREE.Object3D, a: THREE.Vector3, b: THREE.Vector3, r: number, mat: THREE.Material, seg = 5) {
  const d = new THREE.Vector3().subVectors(b, a);
  const len = d.length();
  const m = new THREE.Mesh(new THREE.CylinderGeometry(r, r, len, seg), mat);
  m.position.copy(a).addScaledVector(d, 0.5);
  m.quaternion.setFromUnitVectors(_up, d.normalize());
  parent.add(m);
  return m;
}
export function noMerge(o: THREE.Object3D) { o.userData.noMerge = true; return o; }

// A box in `outer` with one face covered by a plane in `inner` (a painted room side,
// a printed cabinet door). Same look as a six-material box, but both parts merge with
// other meshes of the same material, where a multi-material box costs six draw calls.
export type Side = '+x' | '-x' | '+y' | '-y' | '+z' | '-z';
export function faced(parent: THREE.Object3D, w: number, h: number, d: number, outer: THREE.Material, inner: THREE.Material, x: number, y: number, z: number, side: Side) {
  const b = box(parent, w, h, d, outer, x, y, z);
  const e = 0.002;
  let p: THREE.Mesh;
  switch (side) {
    case '+x': p = plane(parent, d, h, inner, x + w / 2 + e, y, z, Math.PI / 2); break;
    case '-x': p = plane(parent, d, h, inner, x - w / 2 - e, y, z, -Math.PI / 2); break;
    case '+z': p = plane(parent, w, h, inner, x, y, z + d / 2 + e, 0); break;
    case '-z': p = plane(parent, w, h, inner, x, y, z - d / 2 - e, Math.PI); break;
    case '+y': p = plane(parent, w, d, inner, x, y + h / 2 + e, z, 0, -Math.PI / 2); break;
    case '-y': p = plane(parent, w, d, inner, x, y - h / 2 - e, z, 0, Math.PI / 2); break;
  }
  return { box: b, face: p };
}

// ---------- Batching ----------
// Merge all static meshes under `root` into one mesh per material. Big win on mobile.
export function mergeStatic(root: THREE.Object3D) {
  root.updateMatrixWorld(true);
  const inv = new THREE.Matrix4().copy(root.matrixWorld).invert();
  const buckets = new Map<THREE.Material, THREE.BufferGeometry[]>();
  const victims: THREE.Mesh[] = [];
  const walk = (o: THREE.Object3D) => {
    for (const c of [...o.children]) {
      if (c.userData.noMerge) continue;
      const mesh = c as THREE.Mesh;
      if (mesh.isMesh && !(mesh as any).isInstancedMesh && !Array.isArray(mesh.material) && !(mesh.geometry.attributes.color)) {
        const g = mesh.geometry.index ? mesh.geometry.toNonIndexed() : mesh.geometry.clone();
        for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'uv'].includes(k)) g.deleteAttribute(k);
        if (!g.attributes.uv) g.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(g.attributes.position.count * 2), 2));
        g.applyMatrix4(new THREE.Matrix4().multiplyMatrices(inv, mesh.matrixWorld));
        const mat = mesh.material as THREE.Material;
        if (!buckets.has(mat)) buckets.set(mat, []);
        buckets.get(mat)!.push(g);
        victims.push(mesh);
      }
      walk(c);
    }
  };
  walk(root);
  for (const v of victims) { v.parent?.remove(v); v.geometry.dispose(); }
  for (const [mat, geos] of buckets) {
    const merged = mergeGeometries(geos, false);
    if (!merged) continue;
    const m = new THREE.Mesh(merged, mat);
    m.matrixAutoUpdate = false;
    root.add(m);
  }
}

// ---------- Detail drawn only near ----------
// What only adds to a place seen from close by (the props and the stains of a lot, the wear on
// its asphalt, the clutter of a room seen through its window) costs its draw calls from across
// the land too: from SARO the motel's are a few pixels 650 m off, and from the control room's
// desk the lab's are behind two walls. A group made here is drawn only while the camera
// is within `near` metres of what is in it (its box, taken the first time it is in the scene).
// The group's own `visible` is this one's: quality (core/quality.ts) and the areas switch what
// is inside it, never the group. followNear runs it before every picture.
const nearOnes: { g: THREE.Group; near: number; box: THREE.Box3 | null }[] = [];
export function nearOnly(o: THREE.Object3D, near: number) {
  const g = noMerge(new THREE.Group()) as THREE.Group;
  g.name = (o.name || 'detail') + ':near';
  g.userData.nearOnly = true;
  g.add(o);
  nearOnes.push({ g, near, box: null });
  return g;
}
const _eye = new THREE.Vector3();
const inScene = (o: THREE.Object3D) => { let p: THREE.Object3D | null = o; while (p && !(p as THREE.Scene).isScene) p = p.parent; return !!p; };
/** Before every picture of the scene, the near-only groups shown or not for the camera drawing
 *  it (the world matrices are up to date then). Once, for the one scene. */
export function followNear(scene: THREE.Scene) {
  if (scene.userData.near) return;
  scene.userData.near = true;
  const prev = scene.onBeforeRender;
  scene.onBeforeRender = function (this: THREE.Scene, ...args: Parameters<THREE.Object3D['onBeforeRender']>) {
    prev.apply(this, args);
    const eye = args[2].getWorldPosition(_eye);
    for (const e of nearOnes) {
      if (!e.box) { if (!inScene(e.g)) continue; e.box = new THREE.Box3().setFromObject(e.g); }
      e.g.visible = e.box.distanceToPoint(eye) < e.near;
    }
  };
}
