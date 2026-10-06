import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { box, cyl, plane } from '../world/kit';
import type { Obstacle } from './Drive';

/** The caller owns road placement, terrain, light slots and fence integration.
 * Offsets are positive on the right; every returned coordinate is road-local. */
export interface LandmarkContext {
  beside(s: number, o: number): { x: number; z: number; p: { tx: number; tz: number } };
  heightAt(x: number, z: number): number;
  sAt(mi: number): number;
  rnd(x: number, z: number): number;
  std(params: THREE.MeshStandardMaterialParameters, fall?: number): THREE.MeshStandardMaterial;
}

/** One quiet 1986 landmark. No interaction, event, traffic or light-slot mutation.
 * Two opaque meshes: vertex-coloured solids/track and the code-written sign. */
export function buildLandmarks(c: LandmarkContext): {
  group: THREE.Group;
  obstacles: Obstacle[];
  fenceGaps: { s0: number; s1: number; side: -1 | 1 }[];
  glints: THREE.Vector3[];
} {
  const s = c.sAt(3.2), gate = c.beside(s, -17);
  const group = new THREE.Group();
  group.name = 'oldroad/Kessler';
  // Keep vertex precision near the gate, while the group remains road-local.
  group.position.set(gate.x, 0, gate.z);
  const solids = new THREE.Group();
  group.add(solids);
  const mat = c.std({ vertexColors: true, roughness: 0.94, side: THREE.DoubleSide });
  mat.name = 'oldroad/Kessler-solids';
  const colours = new Map<THREE.Mesh, THREE.Color>();
  const paint = (m: THREE.Mesh, color: number) => { colours.set(m, new THREE.Color(color)); return m; };
  const timber = 0x786950, worn = 0x97866a, iron = 0x4f5352, rust = 0x6f4734;
  const block = (w: number, h: number, d: number, color: number, x: number, y: number, z: number) =>
    paint(box(solids, w, h, d, mat, x - gate.x, y, z - gate.z), color);
  const tube = (rt: number, rb: number, h: number, color: number, x: number, y: number, z: number, seg = 8) =>
    paint(cyl(solids, rt, rb, h, mat, x - gate.x, y, z - gate.z, seg), color);
  const beam = (a: THREE.Vector3, b: THREE.Vector3, r: number, color: number) => {
    const m = tube(r, r, a.distanceTo(b), color, (a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2, 5);
    m.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), b.clone().sub(a).normalize());
    return m;
  };
  const gy = (x: number, z: number) => c.heightAt(x, z);
  const at = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
  const obstacles: Obstacle[] = [];

  // The opening is along the fence's tangent, not across the road's asphalt.
  const ends = [-3.4, 3.4].map(d => c.beside(s + d, -17));
  const base = Math.max(...ends.map(p => gy(p.x, p.z)));
  ends.forEach(p => {
    tube(0.19, 0.24, 5.15, timber, p.x, base + 2.42, p.z, 7);
    tube(0.25, 0.25, 0.18, iron, p.x, base + 0.38, p.z, 7);
    tube(0.21, 0.21, 0.13, iron, p.x, base + 4.67, p.z, 7);
    obstacles.push({ kind: 'circle', x: p.x, z: p.z, r: 0.35 });
  });
  beam(at(ends[0].x, base + 4.85, ends[0].z), at(ends[1].x, base + 4.85, ends[1].z), 0.23, worn);
  const tangent = new THREE.Vector3(gate.p.tx, 0, gate.p.tz).normalize();
  const signYaw = Math.atan2(-tangent.z, tangent.x);
  block(5.65, 0.85, 0.16, timber, gate.x, base + 3.91, gate.z).rotation.y = signYaw;
  for (const d of [-2.3, 2.3]) {
    const x = gate.x + tangent.x * d, z = gate.z + tangent.z * d;
    beam(at(x, base + 4.76, z), at(x, base + 4.34, z), 0.018, iron);
  }

  // Exact lettering only, generated locally; no external bitmap or new art route.
  const canvas = document.createElement('canvas');
  canvas.width = 512; canvas.height = 128;
  const ctx = canvas.getContext('2d')!;
  ctx.fillStyle = '#786950'; ctx.fillRect(0, 0, 512, 128);
  ctx.strokeStyle = '#625540'; ctx.lineWidth = 2;
  for (let i = 0; i < 14; i++) {
    const y = 4 + i * 9; ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(512, y + 2); ctx.stroke();
  }
  ctx.fillStyle = '#ddd1ad'; ctx.font = 'bold 84px Georgia, serif';
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText('KESSLER', 256, 69, 478);
  const signMap = new THREE.CanvasTexture(canvas);
  signMap.colorSpace = THREE.SRGBColorSpace;
  signMap.name = 'oldroad/Kessler-lettering (code, 512x128)';
  const signMat = c.std({ map: signMap, roughness: 1 });
  signMat.name = 'oldroad/Kessler-lettering';
  // Two correctly oriented fronts keep the name readable from either direction.
  const signParts: THREE.BufferGeometry[] = [];
  for (const side of [-1, 1]) {
    const m = plane(group, 5.57, 0.77, signMat, 0, base + 3.91, 0, signYaw + (side < 0 ? Math.PI : 0));
    m.position.x += Math.sin(signYaw) * 0.086 * side;
    m.position.z += Math.cos(signYaw) * 0.086 * side;
    m.updateMatrix();
    const g = m.geometry.clone().applyMatrix4(m.matrix).toNonIndexed();
    signParts.push(g); m.removeFromParent(); m.geometry.dispose();
  }
  const signGeo = mergeGeometries(signParts)!;
  signParts.forEach(g => g.dispose());
  const sign = new THREE.Mesh(signGeo, signMat);
  sign.name = 'oldroad/Kessler-name'; group.add(sign);

  // Gravel follows the supplied ground, including beyond the road ribbon.
  const trackPositions: number[] = [], trackColours: number[] = [];
  const triangle = (points: [number, number][], color: number, lift = [0.055, 0.055, 0.055]) => {
    const col = new THREE.Color(color);
    for (const [i, [x, z]] of points.entries()) {
      trackPositions.push(x - gate.x, gy(x, z) + lift[i], z - gate.z);
      trackColours.push(col.r, col.g, col.b);
    }
  };
  const strip = (a: { x: number; z: number }, b: { x: number; z: number }, half: number, color: number) => {
    const dx = b.x - a.x, dz = b.z - a.z, len = Math.hypot(dx, dz);
    const ox = -dz / len * half, oz = dx / len * half;
    const p0: [number, number] = [a.x - ox, a.z - oz], p1: [number, number] = [a.x + ox, a.z + oz];
    const p2: [number, number] = [b.x - ox, b.z - oz], p3: [number, number] = [b.x + ox, b.z + oz];
    triangle([p0, p1, p2], color); triangle([p1, p3, p2], color);
  };
  const verge = c.beside(s, -5.3);
  const entrySteps = Math.ceil(Math.hypot(gate.x - verge.x, gate.z - verge.z) / 1.5);
  for (let i = 0; i < entrySteps; i++) {
    const p = (k: number) => ({ x: verge.x + (gate.x - verge.x) * k / entrySteps, z: verge.z + (gate.z - verge.z) * k / entrySteps });
    strip(p(i), p(i + 1), 2.7, 0x9b8971);
  }
  const end = { x: gate.x, z: gate.z - 300 };
  // The road's far plain is coarser/lower than heightAt outside its 260 m ribbon.
  // Bury earth shoulders below it, so this graded track cannot float above it.
  const shoulder = (x: number, z: number, nx: number, nz: number, side: number) => {
    const a: [number, number] = [x + side * 2.55, z], b: [number, number] = [nx + side * 2.55, nz];
    const d: [number, number] = [x + side * 4.35, z], e: [number, number] = [nx + side * 4.35, nz];
    if (side < 0) {
      triangle([a, b, d], 0x83705b, [0.055, 0.055, -2.5]);
      triangle([d, b, e], 0x83705b, [-2.5, 0.055, -2.5]);
    } else {
      triangle([a, d, b], 0x83705b, [0.055, -2.5, 0.055]);
      triangle([d, e, b], 0x83705b, [-2.5, -2.5, 0.055]);
    }
  };
  for (let i = 0; i < 200; i++) {
    const z = gate.z - i * 1.5, next = z - 1.5;
    // Subtle bends and dark wheel tracks, not a repeating painted ribbon.
    const x = gate.x + Math.sin(i / 35) * 1.4;
    const nx = gate.x + Math.sin((i + 1) / 35) * 1.4;
    const gravelColor = c.rnd(x, z) > 0.5 ? 0x9b8971 : 0x95836c;
    // Adjacent bands, not coplanar overlays: no flickering wheel marks at distance.
    for (const [offset, half, wheel] of [[-2.09, 0.46, 0], [-1.25, 0.38, 1], [0, 0.87, 0], [1.25, 0.38, 1], [2.09, 0.46, 0]]) {
      strip({ x: x + offset, z }, { x: nx + offset, z: next }, half, wheel ? 0x81725f : gravelColor);
    }
    for (const side of [-1, 1]) shoulder(x, z, nx, next, side);
    // Sparse coloured gravel chips break the broad bands without another sampler.
    for (let k = 0; k < 6; k++) {
      const px = x + (c.rnd(i * 7 + k, 13) - 0.5) * 5.05;
      const pz = z - c.rnd(31, i * 7 + k) * 1.5;
      const size = 0.012 + c.rnd(i, k + 72) * 0.026;
      triangle([[px - size, pz], [px + size, pz], [px, pz - size * 1.6]], k % 3 ? 0xa08b70 : 0x6c5e4c, [0.075, 0.075, 0.075]);
    }
  }
  const lastX = gate.x + Math.sin(200 / 35) * 1.4;
  const cap: [number, number][] = [[lastX - 2.55, end.z], [lastX + 2.55, end.z], [lastX - 4.35, end.z - 1.8], [lastX + 4.35, end.z - 1.8]];
  triangle([cap[0], cap[1], cap[2]], 0x83705b, [0.055, 0.055, -2.5]);
  triangle([cap[1], cap[3], cap[2]], 0x83705b, [0.055, -2.5, -2.5]);
  // Ferist across the northbound entry; upper faces remain just above terrain.
  block(5.8, 0.12, 2.35, 0x272b29, gate.x, gy(gate.x, gate.z) + 0.025, gate.z);
  for (let i = 0; i < 12; i++) {
    const z = gate.z - 1.07 + i * 0.195;
    block(5.6, 0.08, 0.075, iron, gate.x, gy(gate.x, z) + 0.14, z);
  }
  for (const d of [-2.9, 2.9]) block(0.15, 0.16, 2.55, rust, gate.x + d, gy(gate.x + d, gate.z) + 0.08, gate.z);
  const gravel = new THREE.BufferGeometry();
  gravel.setAttribute('position', new THREE.Float32BufferAttribute(trackPositions, 3));
  gravel.setAttribute('color', new THREE.Float32BufferAttribute(trackColours, 3));
  gravel.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(trackPositions.length / 3 * 2), 2));
  gravel.computeVertexNormals();

  // Galvanised windpump, braced tapering tower, fan and tail. Static, no new ticking work.
  const wx = end.x + 13, wz = end.z - 1, wy = gy(wx, wz);
  const corners = [-1, 1].flatMap(x => [-1, 1].map(z => ({ x, z })));
  for (const q of corners) {
    beam(at(wx + q.x * 2.0, wy - 2, wz + q.z * 2.0), at(wx + q.x * 0.48, wy + 14, wz + q.z * 0.48), 0.065, iron);
  }
  for (let y = 2; y <= 12; y += 2) {
    const w0 = 2 - y / 14 * 1.52, w1 = 2 - (y + 2) / 14 * 1.52;
    for (const side of [-1, 1]) {
      beam(at(wx - w0, wy + y, wz + side * w0), at(wx + w1, wy + y + 2, wz + side * w1), 0.025, iron);
      beam(at(wx + side * w0, wy + y, wz - w0), at(wx + side * w1, wy + y + 2, wz + w1), 0.025, iron);
    }
  }
  const hubY = wy + 15.2;
  tube(0.14, 0.16, 1.7, iron, wx, wy + 14.7, wz);
  const hub = tube(0.23, 0.23, 0.7, worn, wx, hubY, wz, 10); hub.rotation.x = Math.PI / 2;
  for (let i = 0; i < 18; i++) {
    const a = i * Math.PI * 2 / 18;
    const blade = block(0.62, 1.6, 0.045, i % 3 ? 0xabb0a6 : 0x7f837a,
      wx + Math.sin(a) * 2.25, hubY + Math.cos(a) * 2.25, wz - 0.4);
    blade.rotation.z = -a; blade.rotation.y = 0.2;
    beam(at(wx, hubY, wz - 0.37), at(wx + Math.sin(a) * 1.9, hubY + Math.cos(a) * 1.9, wz - 0.37), 0.018, iron);
  }
  beam(at(wx, hubY, wz), at(wx + 3.8, hubY, wz + 0.5), 0.05, iron);
  block(1.25, 1.1, 0.045, 0x92988d, wx + 3.5, hubY + 0.18, wz + 0.5);
  obstacles.push({ kind: 'circle', x: wx, z: wz, r: 2.9 });

  const tx = end.x + 8, tz = end.z + 4, ty = gy(tx, tz);
  tube(2.67, 2.67, 2.6, 0x646357, tx, ty - 1.18, tz, 16);
  tube(2.6, 2.65, 3.1, 0x8e9386, tx, ty + 1.55, tz, 16);
  tube(2.67, 2.67, 0.11, iron, tx, ty + 0.15, tz, 16);
  tube(2.66, 2.66, 0.11, iron, tx, ty + 3.03, tz, 16);
  tube(0.1, 0.1, 3.35, rust, tx - 2.5, ty + 1.67, tz);
  obstacles.push({ kind: 'circle', x: tx, z: tz, r: 2.85 });

  // Unlit farmhouse with one exterior yard lamp. No glowing windows or occupants.
  const hx = end.x - 15, hz = end.z - 6, hy = gy(hx, hz);
  block(12, 1.8, 9, 0x625d50, hx, hy - 0.78, hz);
  block(12, 3.15, 9, 0x5e5748, hx, hy + 1.55, hz);
  block(12.7, 0.22, 9.7, 0x3b3e39, hx, hy + 3.12, hz);
  const roof = new THREE.BufferGeometry();
  const rp = [
    -6.5,3.2,-5, 0,5.2,-5, -6.5,3.2,5, 0,5.2,-5, 0,5.2,5, -6.5,3.2,5,
    0,5.2,-5, 6.5,3.2,-5, 0,5.2,5, 6.5,3.2,-5, 6.5,3.2,5, 0,5.2,5,
    -6.5,3.2,-5, 6.5,3.2,-5, 0,5.2,-5, -6.5,3.2,5, 0,5.2,5, 6.5,3.2,5,
  ];
  // Outside faces point up/out, including the two gables.
  for (let i = 0; i < rp.length; i += 9) for (let j = 0; j < 3; j++) {
    const b = rp[i + 3 + j]; rp[i + 3 + j] = rp[i + 6 + j]; rp[i + 6 + j] = b;
  }
  roof.setAttribute('position', new THREE.Float32BufferAttribute(rp, 3));
  roof.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(rp.length / 3 * 2), 2));
  roof.computeVertexNormals();
  const roofMesh = new THREE.Mesh(roof, mat);
  roofMesh.position.set(hx - gate.x, hy, hz - gate.z); solids.add(roofMesh); paint(roofMesh, 0x43463f);
  for (const x of [-3.7, 3.7]) {
    block(1.45, 1.42, 0.07, 0x131e22, hx + x, hy + 1.85, hz + 4.55);
    block(1.58, 0.1, 0.13, worn, hx + x, hy + 1.16, hz + 4.62);
    block(0.06, 1.38, 0.12, timber, hx + x, hy + 1.85, hz + 4.64);
  }
  block(1.1, 2.3, 0.09, 0x3c392f, hx, hy + 1.2, hz + 4.57);
  block(6.5, 0.2, 2.25, 0x514b3d, hx, hy + 0.18, hz + 5.55);
  block(6.6, 0.12, 2.5, 0x464941, hx, hy + 2.65, hz + 5.55).rotation.x = 0.05;
  for (const d of [-3, 3]) tube(0.065, 0.065, 4.7, timber, hx + d, hy + 0.35, hz + 6.4, 5);
  const lx = hx + 2.5, lz = hz + 6;
  block(0.3, 0.17, 0.28, iron, lx, hy + 2.48, lz);
  const glints = [at(lx, hy + 2.36, lz)];
  obstacles.push({ kind: 'box', minX: hx - 6.6, maxX: hx + 6.6, minZ: hz - 5, maxZ: hz + 6.65 });

  // kit.mergeStatic intentionally skips vertex colours. Bake all transforms and keep
  // the colour attribute here so gravel, wood and metal share one opaque draw call.
  const parts: THREE.BufferGeometry[] = [gravel];
  for (const [m, color] of colours) {
    m.updateMatrix();
    const source = m.geometry.index ? m.geometry.toNonIndexed() : m.geometry.clone();
    source.applyMatrix4(m.matrix);
    const n = source.getAttribute('position').count, rgb = new Float32Array(n * 3);
    for (let i = 0; i < n; i++) { rgb[i * 3] = color.r; rgb[i * 3 + 1] = color.g; rgb[i * 3 + 2] = color.b; }
    source.setAttribute('color', new THREE.BufferAttribute(rgb, 3));
    for (const name of Object.keys(source.attributes)) if (!['position', 'normal', 'uv', 'color'].includes(name)) source.deleteAttribute(name);
    parts.push(source); m.geometry.dispose();
  }
  const merged = mergeGeometries(parts)!;
  parts.forEach(g => g.dispose());
  solids.removeFromParent();
  const mesh = new THREE.Mesh(merged, mat); mesh.name = 'oldroad/Kessler-static'; group.add(mesh);
  return { group, obstacles, fenceGaps: [{ s0: s - 3.85, s1: s + 3.85, side: -1 }], glints };
}
