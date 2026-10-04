import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { artTexture } from '../core/art';

// ---------- Exterior floodlights ----------
// Real three.js point lights cost every material a loop per light. Outside we fake
// sodium floodlights with a tiny shader add-on that only exterior materials pay for.
// A material belongs to one set of fake lights. The big outdoor set lights the site;
// small sets light a single interior (the photo lab) without paying for the site.
export type FloodSet = { n: number; pos: THREE.Vector4[]; col: THREE.Color[]; count: number; scale: { value: number }; key: string };
export function floodSet(n: number, key: string, scale = 0.07): FloodSet {
  return {
    n, key, count: 0, scale: { value: scale },
    pos: Array.from({ length: n }, () => new THREE.Vector4(0, -999, 0, 0)),
    col: Array.from({ length: n }, () => new THREE.Color(1, 0.6, 0.25)),
  };
}
export const FLOOD_N = 20;
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

export function floodlit<T extends THREE.MeshStandardMaterial>(m: T, falloff = 0.012, set = flood): T {
  const FLOOD_N = set.n;
  m.onBeforeCompile = (sh) => {
    sh.uniforms.uFloodPos = { value: set.pos };
    sh.uniforms.uFloodCol = { value: set.col };
    sh.uniforms.uFloodFall = { value: falloff };
    sh.uniforms.uFloodScale = set.scale;
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
        uniform float uFloodFall; uniform float uFloodScale;`)
      .replace('#include <emissivemap_fragment>', `#include <emissivemap_fragment>
        vec3 fAcc = vec3(0.0);
        vec3 fN = normalize(vFNormal);
        for (int i = 0; i < ${FLOOD_N}; i++) {
          if (uFloodPos[i].w <= 0.0) continue;
          vec3 L = uFloodPos[i].xyz - vFWorld;
          float d2 = dot(L, L);
          L *= inversesqrt(max(d2, 1e-4));
          float ndl = abs(dot(fN, L)) * 0.75 + 0.25;
          // windowed falloff: each lamp lights a pool, not the whole desert
          float win = clamp(1.0 - sqrt(d2) / (uFloodPos[i].w * 1.6 + 8.0), 0.0, 1.0);
          fAcc += uFloodCol[i] * (uFloodPos[i].w * ndl * win * win / (1.0 + d2 * uFloodFall));
        }
        totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;`);
  };
  m.customProgramCacheKey = () => 'flood' + set.key + falloff;
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
