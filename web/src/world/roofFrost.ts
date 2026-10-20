import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { rng } from '../core/textures';
import { floodlit } from './kit';

// Frost on the records annex's roof after the steps (story/roofSteps.ts): on the roof unit over the
// records room (Annex.ts builds it: x -5.0 to -3.4, y 3.2 to 4.0, z 8.05 to 9.15, a plate on top to
// 4.04), thick along its top edges and furred down its sides, most on the east side, over the table;
// and a crust on the lip of the roof (3.2) at the corner where the second steps went off it. Seen
// from the ground only the unit's upper part shows over the edge and the slab's face at the
// corners, so that is where it is. One picture for all of it (the feathers over the top 160 rows,
// a crust under them), drawn once; see-through, not cut out, so it does not crumble away when the
// picture gets small at 20 m.

export type Corner = 'sw' | 'se';
export interface RoofFrost {
  group: THREE.Group;
  /** The frost on the unit, and on the lip at each corner (only one is ever shown). */
  unit: THREE.Mesh;
  lips: Record<Corner, THREE.Mesh>;
  /** Hit areas for a closer look (invisible): the unit, the two corners. */
  hit: { unit: THREE.Mesh; sw: THREE.Mesh; se: THREE.Mesh };
}

const V_FEATHER = 1 - 160 / 256;   // the feathers run from v 1 (their root, at the top edge) down to here
const V_CRUST = 1 - 170 / 256;     // the crust is v 0 to here

function picture() {
  const c = document.createElement('canvas'); c.width = c.height = 256;
  const g = c.getContext('2d')!, r = rng(4747);
  g.clearRect(0, 0, 256, 256);
  g.lineCap = 'round';
  // the coat: thick and white from the edge down to a ragged line that wanders across (two slow
  // waves and a little noise), so from the ground it reads as a white top on the unit and not as
  // hair; under it the needles of rime growing down out of it, with short side branches, thinner
  // and shorter further down
  const edge: number[] = [];
  const ph = [r() * 6.3, r() * 6.3];
  for (let x = 0; x < 256; x++) edge.push(46 + 16 * Math.sin(x / 256 * 6.283 * 2 + ph[0]) + 9 * Math.sin(x / 256 * 6.283 * 5 + ph[1]) + (r() - 0.5) * 8);
  for (let x = 0; x < 256; x++) {
    const e = edge[x];
    for (let y = 0; y < e; y += 2) {
      const k = y / e, al = 0.97 - 0.25 * k * k - r() * 0.12;
      const l = 228 + r() * 27 | 0;
      g.fillStyle = `rgba(${l},${Math.min(255, l + 4)},255,${al.toFixed(3)})`;
      g.fillRect(x, y, 1, 2);
    }
  }
  for (let i = 0; i < 700; i++) {
    const x0 = r() * 256, y0 = edge[x0 | 0] - 4 - r() * 10, L = 4 + r() ** 1.8 * 90, a = (r() - 0.5) * 0.6;
    const x1 = x0 + Math.sin(a) * L, y1 = Math.min(158, y0 + Math.cos(a) * L);
    const al = 0.5 + r() * 0.45 * (1 - L / 110);
    g.strokeStyle = `rgba(240,246,255,${al.toFixed(3)})`; g.lineWidth = 1 + r() * 1.6;
    g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke();
    // side branches every few pixels, shorter towards the tip
    g.lineWidth = 0.9;
    for (let s = 3 + r() * 3; s < L - 2; s += 3 + r() * 5) {
      const k = s / L, bx = x0 + Math.sin(a) * s, by = y0 + Math.cos(a) * s, bl = (2 + r() * 5) * (1 - k * 0.7);
      for (const side of [-1, 1]) {
        if (r() < 0.2) continue;
        const ba = a + side * (0.7 + r() * 0.4);
        g.beginPath(); g.moveTo(bx, by); g.lineTo(bx + Math.sin(ba) * bl, by + Math.cos(ba) * bl * 0.8); g.stroke();
      }
    }
  }
  // grains that catch the light, all over the coat and a few below it
  for (let i = 0; i < 1400; i++) {
    const x = r() * 256, y = r() < 0.8 ? r() * edge[x | 0] : edge[x | 0] + r() * 60;
    g.fillStyle = `rgba(255,255,255,${(0.5 + r() * 0.5).toFixed(3)})`;
    g.fillRect(x, y, 1 + r() * 1.5, 1 + r() * 1.5);
  }
  // thinner as it goes down (the ragged end is where the needles stop)
  g.globalCompositeOperation = 'destination-in';
  const fade = g.createLinearGradient(0, 0, 0, 160);
  fade.addColorStop(0, 'rgba(0,0,0,1)'); fade.addColorStop(0.4, 'rgba(0,0,0,0.95)'); fade.addColorStop(1, 'rgba(0,0,0,0.3)');
  g.fillStyle = fade; g.fillRect(0, 0, 256, 160);
  g.globalCompositeOperation = 'source-over';
  // the crust: grains packed close, a few bare spots
  for (let i = 0; i < 2600; i++) {
    const x = r() * 256, y = 172 + r() * 84, s = 1 + r() * 3.2;
    g.fillStyle = `rgba(${236 + r() * 19 | 0},${242 + r() * 13 | 0},255,${(0.55 + r() * 0.45).toFixed(3)})`;
    g.beginPath(); g.arc(x, y, s, 0, Math.PI * 2); g.fill();
  }
  const tex = new THREE.CanvasTexture(c);
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 4;
  return tex;
}

/** One quad: corners top-left, top-right, bottom-right, bottom-left, and the feathers' picture from
 *  u0 to u1, its root at the top edge (vb: how far down the picture the quad's bottom reaches). */
function quad(tl: THREE.Vector3, tr: THREE.Vector3, br: THREE.Vector3, bl: THREE.Vector3, u0: number, u1: number, vb = V_FEATHER) {
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute([...tl.toArray(), ...tr.toArray(), ...br.toArray(), ...bl.toArray()], 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute([u0, 1, u1, 1, u1, vb, u0, vb], 2));
  g.setIndex([0, 3, 1, 1, 3, 2]);
  g.computeVertexNormals();
  return g;
}
/** A strip of crust along an edge: a thin box, its picture the crust. */
function crust(w: number, h: number, d: number, x: number, y: number, z: number) {
  const g = new THREE.BoxGeometry(w, h, d);
  const uv = g.getAttribute('uv') as THREE.BufferAttribute;
  for (let i = 0; i < uv.count; i++) uv.setY(i, uv.getY(i) * V_CRUST);
  g.translate(x, y, z);
  return g;
}
const v = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);

export function buildRoofFrost(): RoofFrost {
  const mat = new THREE.MeshStandardMaterial({
    map: picture(), color: 0xf4f6f8, roughness: 0.45, metalness: 0,
    transparent: true, depthWrite: false, alphaTest: 0.02,
    // frost is the palest thing about at night: a little of its own, so the starlight it would
    // catch is there on a screen too (not a glow)
    emissive: 0x8796a8, emissiveIntensity: 0.3,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -2,
  });
  mat.emissiveMap = mat.map;
  mat.name = 'roofFrost';
  // and the yard's sodium lamps on it, as on everything else out there (world/kit.ts)
  floodlit(mat, 0.012);
  const group = new THREE.Group(); group.name = 'roofFrost';
  group.visible = false;

  // ---------- the unit ----------
  const e = 0.006, x0 = -5.0, x1 = -3.4, z0 = 8.05, z1 = 9.15, top = 4.0;
  const parts: THREE.BufferGeometry[] = [
    // north (-z) and south (+z) faces, the upper part; west (-x); east (+x, over the table) furred lower
    quad(v(x1, top, z0 - e), v(x0, top, z0 - e), v(x0, top - 0.55, z0 - e), v(x1, top - 0.55, z0 - e), 0.0, 0.9),
    quad(v(x0, top, z1 + e), v(x1, top, z1 + e), v(x1, top - 0.62, z1 + e), v(x0, top - 0.62, z1 + e), 0.05, 0.95),
    quad(v(x0 - e, top, z0), v(x0 - e, top, z1), v(x0 - e, top - 0.58, z1), v(x0 - e, top - 0.58, z0), 0.3, 0.92),
    quad(v(x1 + e, top, z1), v(x1 + e, top, z0), v(x1 + e, top - 0.78, z0), v(x1 + e, top - 0.78, z1), 0.1, 0.8),
    // the plate on top, crusted all over, and the plate's edge
    crust(1.5, 0.006, 1.0, -4.2, 4.044, 8.6),
    // the rims along the body's top edges, where it is thickest
    crust(1.64, 0.04, 0.035, -4.2, top + 0.005, z0), crust(1.64, 0.04, 0.035, -4.2, top + 0.005, z1),
    crust(0.035, 0.04, 1.14, x0, top + 0.005, 8.6), crust(0.04, 0.05, 1.14, x1, top + 0.005, 8.6),
    crust(1.54, 0.03, 0.03, -4.2, 4.035, 8.1), crust(1.54, 0.03, 0.03, -4.2, 4.035, 9.1),
    crust(0.03, 0.03, 1.04, -4.95, 4.035, 8.6), crust(0.03, 0.03, 1.04, -3.45, 4.035, 8.6),
  ];
  const unit = new THREE.Mesh(mergeGeometries(parts, false)!, mat);
  unit.name = 'roofFrost:unit';
  group.add(unit);

  // ---------- the lip at a corner: the slab's faces (2.8 to 3.2) and its top edge ----------
  const Y = 3.2, H = 0.24, L = 0.8;
  const lip = (corner: Corner) => {
    const cz = 10.8;
    const geos = corner === 'sw'
      ? [quad(v(-6.3, Y, cz + e), v(-6.3 + L, Y, cz + e), v(-6.3 + L, Y - H, cz + e), v(-6.3, Y - H, cz + e), 0.2, 0.55),
         quad(v(-6.3 - e, Y, cz - L), v(-6.3 - e, Y, cz), v(-6.3 - e, Y - H, cz), v(-6.3 - e, Y - H, cz - L), 0.6, 0.95),
         crust(L + 0.03, 0.035, 0.04, -6.3 + L / 2, Y + 0.004, cz), crust(0.04, 0.035, L + 0.03, -6.3, Y + 0.004, cz - L / 2)]
      : [quad(v(2.3 - L, Y, cz + e), v(2.3, Y, cz + e), v(2.3, Y - H, cz + e), v(2.3 - L, Y - H, cz + e), 0.45, 0.8),
         quad(v(2.3 + e, Y, cz), v(2.3 + e, Y, cz - L), v(2.3 + e, Y - H, cz - L), v(2.3 + e, Y - H, cz), 0.05, 0.4),
         crust(L + 0.03, 0.035, 0.04, 2.3 - L / 2, Y + 0.004, cz), crust(0.04, 0.035, L + 0.03, 2.3, Y + 0.004, cz - L / 2)];
    const m = new THREE.Mesh(mergeGeometries(geos, false)!, mat);
    m.name = `roofFrost:${corner}`; m.visible = false;
    group.add(m);
    return m;
  };
  const lips = { sw: lip('sw'), se: lip('se') };

  // ---------- where a closer look finds them ----------
  const none = new THREE.MeshBasicMaterial({ visible: false });
  const area = (name: string, w: number, h: number, d: number, x: number, y: number, z: number) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), none);
    m.position.set(x, y, z); m.name = name;
    return m;
  };
  const hit = {
    unit: area('look:roofUnit', 1.8, 1.0, 1.3, -4.2, 3.6, 8.6),
    // (only the slab's face, under the roof line: from the south-west the unit stands just over it)
    sw: area('look:roofLipSW', 0.95, 0.32, 0.95, -5.9, 3.04, 10.4),
    se: area('look:roofLipSE', 0.95, 0.32, 0.95, 1.9, 3.04, 10.4),
  };
  return { group, unit, lips, hit };
}
