import * as THREE from 'three';
import { M, box, cyl, rod, mergeStatic } from './kit';

// Small reusable models. Each returns a group whose parts are already merged per material.

// SARO field camera: a 1980s rangefinder with a strap. About 14 cm wide.
export function fieldCameraModel() {
  const g = new THREE.Group();
  const body = new THREE.MeshStandardMaterial({ color: 0x1a1c1e, roughness: 0.55, metalness: 0.3 });
  const chrome = new THREE.MeshStandardMaterial({ color: 0xb9bcbf, roughness: 0.25, metalness: 0.9 });
  box(g, 0.14, 0.075, 0.045, body, 0, 0.0375, 0);
  box(g, 0.142, 0.022, 0.047, chrome, 0, 0.0755, 0);
  box(g, 0.03, 0.012, 0.02, chrome, -0.045, 0.092, 0);       // rewind knob plate
  cyl(g, 0.009, 0.009, 0.01, chrome, 0.05, 0.09, 0, 10);     // shutter button
  const lens = cyl(g, 0.022, 0.024, 0.04, body, 0.012, 0.038, 0.04, 16);
  lens.rotation.x = Math.PI / 2;
  const ring = cyl(g, 0.024, 0.024, 0.006, chrome, 0.012, 0.038, 0.058, 16);
  ring.rotation.x = Math.PI / 2;
  const glass = cyl(g, 0.017, 0.017, 0.002, new THREE.MeshStandardMaterial({ color: 0x0b1424, roughness: 0.05, metalness: 0.6 }), 0.012, 0.038, 0.061, 16);
  glass.rotation.x = Math.PI / 2;
  box(g, 0.022, 0.014, 0.003, chrome, -0.045, 0.06, 0.024);  // rangefinder windows
  box(g, 0.012, 0.012, 0.003, chrome, 0.05, 0.06, 0.024);
  // strap lying on the shelf
  const strap = new THREE.MeshStandardMaterial({ color: 0x2a2320, roughness: 0.9 });
  const pts: THREE.Vector3[] = [];
  for (let i = 0; i <= 24; i++) { const a = i / 24 * Math.PI; pts.push(new THREE.Vector3(Math.cos(a) * 0.09, 0.005, -0.03 - Math.sin(a) * 0.1)); }
  g.add(new THREE.Mesh(new THREE.TubeGeometry(new THREE.CatmullRomCurve3(pts), 24, 0.004, 4), strap));
  mergeStatic(g);
  return g;
}

// Street-style lamp post with a sodium head. Returns the group and the head position.
export function lampPost(h = 4.2, arm = 0.9) {
  const g = new THREE.Group();
  cyl(g, 0.07, 0.09, h, M.pole, 0, h / 2, 0, 8);
  box(g, arm, 0.08, 0.1, M.pole, arm / 2, h - 0.05, 0);
  box(g, 0.42, 0.12, 0.26, M.pole, arm, h - 0.12, 0);
  box(g, 0.36, 0.02, 0.2, M.lampBulb, arm, h - 0.19, 0);
  mergeStatic(g);
  return { group: g, head: new THREE.Vector3(arm, h - 0.25, 0) };
}

// Pipe railing along a line, posts every ~2 m, two rails.
export function railing(parent: THREE.Object3D, a: THREE.Vector3, b: THREE.Vector3, h = 1.05) {
  const d = b.clone().sub(a);
  const n = Math.max(1, Math.round(d.length() / 2));
  for (let i = 0; i <= n; i++) {
    const p = a.clone().addScaledVector(d, i / n);
    cyl(parent, 0.025, 0.025, h, M.steel, p.x, p.y + h / 2, p.z, 6);
  }
  for (const y of [h, h * 0.55]) rod(parent, a.clone().setY(a.y + y), b.clone().setY(b.y + y), 0.022, M.steel, 6);
}

// Clipboard with a printed sheet (texture on the paper).
// `lit` lets the caller hook the materials into a flood set (outside or in the lab).
export function clipboard(sheet: THREE.Texture, lit: (m: THREE.MeshStandardMaterial) => THREE.MeshStandardMaterial = (m) => m) {
  const g = new THREE.Group();
  box(g, 0.24, 0.008, 0.33, lit(new THREE.MeshStandardMaterial({ color: 0x6d4b2c, roughness: 0.8 })), 0, 0.004, 0);
  const p = new THREE.Mesh(new THREE.PlaneGeometry(0.21, 0.28), lit(new THREE.MeshStandardMaterial({ map: sheet, roughness: 0.9 })));
  p.rotation.x = -Math.PI / 2; p.position.set(0, 0.0095, 0.015); g.add(p);
  box(g, 0.1, 0.015, 0.03, M.steel, 0, 0.012, -0.15);
  mergeStatic(g);
  return g;
}
