import * as THREE from 'three';

// Two graphics levels.
// high: MeshStandardMaterial (PBR, specular highlights from the lamps).
// low:  MeshLambertMaterial copies (much cheaper per pixel) and a lower pixel ratio cap.
// The swap is reversible and cached, so toggling in Settings costs nothing after the first time.

export type Quality = 'high' | 'low';

const toLow = new WeakMap<THREE.Material, THREE.Material>();
const toHigh = new WeakMap<THREE.Material, THREE.Material>();

function lambertOf(m: THREE.MeshStandardMaterial): THREE.MeshLambertMaterial {
  const l = new THREE.MeshLambertMaterial({
    color: m.color,
    map: m.map,
    emissive: m.emissive,
    emissiveMap: m.emissiveMap,
    emissiveIntensity: m.emissiveIntensity,
    transparent: m.transparent,
    opacity: m.opacity,
    side: m.side,
    vertexColors: m.vertexColors,
    alphaTest: m.alphaTest,
    alphaMap: m.alphaMap,
    depthWrite: m.depthWrite,
    fog: m.fog,
    polygonOffset: m.polygonOffset,
    polygonOffsetFactor: m.polygonOffsetFactor,
    polygonOffsetUnits: m.polygonOffsetUnits,
  });
  l.name = m.name;
  l.toneMapped = m.toneMapped;
  // keep the fake sodium floodlights (kit.floodlit) on the cheap material too
  l.onBeforeCompile = m.onBeforeCompile;
  l.customProgramCacheKey = m.customProgramCacheKey;
  return l;
}

function swap(m: THREE.Material, q: Quality): THREE.Material {
  if (q === 'low') {
    if (!(m as THREE.MeshStandardMaterial).isMeshStandardMaterial) return m;
    let l = toLow.get(m);
    if (!l) { l = lambertOf(m as THREE.MeshStandardMaterial); toLow.set(m, l); toHigh.set(l, m); }
    return l;
  }
  return toHigh.get(m) ?? m;
}

let current: Quality = 'high';
export function getQuality() { return current; }

// Applies to everything currently in the scene. Code that assigns a standard
// material later (LED flicker, tube flicker) should call materialFor() so the
// swap still holds.
export function setQuality(scene: THREE.Object3D, q: Quality) {
  current = q;
  scene.traverse((o) => {
    const mesh = o as THREE.Mesh;
    if (!mesh.isMesh || !mesh.material) return;
    mesh.material = Array.isArray(mesh.material) ? mesh.material.map((m) => swap(m, q)) : swap(mesh.material, q);
  });
}

export function materialFor<T extends THREE.Material>(m: T): THREE.Material {
  return swap(m, current);
}
