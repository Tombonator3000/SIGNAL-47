import * as THREE from 'three';
import { canvasTex, dataTwin, rng } from '../core/textures';
import { artImage } from '../core/art';
import { floodlit, type FloodSet } from '../world/kit';

// What makes the paved places of the night read as real (Tom, 19 October: «more realistic
// environments with more detail»; the concept art of Caliche and the payphone): the old asphalt
// and concrete hold the sodium lamps and the headlights in a damp sheen, and they are worn,
// with oil where cars stand, rubber where they turn, patches and cracks sealed with tar.
//
//   wet()        a fake reflection of the floods (kit.floodlit) on a surface: for each lamp a
//                Blinn highlight round the mirror point between it and the eye, strongest at
//                grazing angles, on the dark of the texture (tar, oil) more than the light
//                (paint, gravel), in slow damp patches. In the floods' own loop, so it costs
//                one pow per lamp and pixel; High and Ultra only (the Lambert copies of Low
//                compile without it).
//   wearMesh()   decals from one shared atlas (oil, drips, rubber, cracks, a patch, tar snakes,
//                a damp patch, grit), every one in an area one mesh and one draw call, lit by the
//                area's floods and wet; hidden on Low (userData.lowOff, core/quality.ts).
//   lotAsphalt() the old asphalt of the lots and the service road, 8 m to a picture, with its
//                cracks and patches, shared by SARO and the motel.

// ---------- the sheen ----------
// The floods' loop in kit.floodlit, where the reflection of each lamp is added.
const FLOOD_LINE = 'fAcc += uFloodCol[i] * (uFloodPos[i].w * ndl * win * win / (1.0 + d2 * uFloodFall));';
const FLOOD_OUT = 'totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;';
const WET_GLSL = `
  // slow damp patches, a few metres across (world metres), where it is glossier
  float wetPatch(vec2 p) {
    float n = sin(p.x * 0.29 + sin(p.y * 0.21) * 1.8) * sin(p.y * 0.25 + sin(p.x * 0.17 + 1.3) * 2.1);
    n += 0.5 * sin(p.x * 0.83 - p.y * 0.61 + 2.0) * sin(p.y * 0.97 + 0.4);
    return smoothstep(-0.45, 0.75, n);
  }
  // how much of a lamp's reflection a point gives back: Fresnel (grazing views most), the dark
  // of the picture (tar and oil shine, paint and gravel hardly), and the damp patches
  float wetness(vec3 n, vec3 v, vec3 albedo, vec3 w) {
    float fres = 0.05 + 0.95 * pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 5.0);
    float gloss = clamp(1.2 - dot(albedo, vec3(0.3, 0.59, 0.11)) * uWet.z, 0.0, 1.0);
    return fres * gloss * mix(0.3, 1.0, wetPatch(w.xz));
  }`;
const WET_LAMP = `
        {
          float wD = sqrt(d2), wWin = clamp(1.0 - wD / (uFloodPos[i].w * 3.0 + 22.0), 0.0, 1.0);
          // (the little lift keeps the half vector from ever being nothing: a NaN would bloom, memory.md)
          float wS = pow(max(dot(fN, normalize(L + wV + vec3(0.0, 1e-4, 0.0))), 0.0), uWet.y) * step(0.0, dot(fN, L));
          wAcc += uFloodCol[i] * (uFloodPos[i].w * wS * wWin * wWin / (1.0 + d2 * uFloodFall * 0.25));
        }`;
/** A floodlit material with a damp sheen: k how strong, shine the Blinn exponent (higher is
 *  tighter and wetter), dark how fast lighter parts of the picture lose it (linear luminance). */
export function wet<T extends THREE.MeshStandardMaterial>(m: T, k = 1, shine = 40, dark = 5): T {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey.bind(m);
  const u = { value: new THREE.Vector3(k, shine, dark) };
  m.userData.wet = u;   // (tools/env_roads.py turns it off to see what it adds)
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    if (sh.fragmentShader.includes('#define LAMBERT')) return;   // Low
    if (!sh.fragmentShader.includes(FLOOD_LINE)) { console.warn('wet: not a floodlit material', m.name); return; }
    sh.uniforms.uWet = u;
    sh.fragmentShader = sh.fragmentShader
      .replace('uniform float uFloodScale;', `uniform float uFloodScale; uniform vec3 uWet;${WET_GLSL}`)
      .replace('vec3 fAcc = vec3(0.0);', 'vec3 fAcc = vec3(0.0); vec3 wAcc = vec3(0.0); vec3 wV = normalize(cameraPosition - vFWorld);')
      .replace(FLOOD_LINE, FLOOD_LINE + WET_LAMP)
      .replace(FLOOD_OUT, `${FLOOD_OUT}
        totalEmissiveRadiance += wAcc * wetness(fN, wV, diffuseColor.rgb, vFWorld) * uWet.x * uFloodScale;`);
  };
  m.customProgramCacheKey = () => key() + 'wet';
  return m;
}

// ---------- the wear atlas: eight cells of 256 px ----------
export type WearKind = 'oil' | 'drips' | 'rubber' | 'cracks' | 'patch' | 'seal' | 'damp' | 'grit';
const CELLS: WearKind[] = ['oil', 'drips', 'rubber', 'cracks', 'patch', 'seal', 'damp', 'grit'];
const C = 256;
let atlas: THREE.CanvasTexture | null = null;
/** The decals' picture, drawn once and shared by every area (a disposed copy is uploaded again
 *  by three.js the next time it is drawn). */
export function wearAtlas() {
  if (atlas) return atlas;
  atlas = canvasTex(C * 4, C * 2, (g) => {
    g.clearRect(0, 0, C * 4, C * 2);
    CELLS.forEach((k, i) => {
      g.save(); g.translate((i % 4) * C, Math.floor(i / 4) * C);
      g.beginPath(); g.rect(0, 0, C, C); g.clip();
      DRAW[k](g, rng(300 + i * 17));
      g.restore();
    });
  });
  atlas.name = 'wear';
  return atlas;
}
type Pen = (g: CanvasRenderingContext2D, r: () => number) => void;
const blob = (g: CanvasRenderingContext2D, x: number, y: number, rx: number, ry: number, rot: number, col: string, a: number) => {
  g.save(); g.translate(x, y); g.rotate(rot); g.scale(1, ry / rx);
  const gr = g.createRadialGradient(0, 0, 0, 0, 0, rx);
  gr.addColorStop(0, `rgba(${col},${a})`); gr.addColorStop(0.6, `rgba(${col},${a * 0.7})`); gr.addColorStop(1, `rgba(${col},0)`);
  g.fillStyle = gr; g.beginPath(); g.arc(0, 0, rx, 0, 7); g.fill(); g.restore();
};
/** A wandering line from (x0, y0) to (x1, y1) in n steps, `j` pixels of wander. */
const wander = (g: CanvasRenderingContext2D, r: () => number, x0: number, y0: number, x1: number, y1: number, n: number, j: number) => {
  g.beginPath(); g.moveTo(x0, y0);
  let ox = 0, oy = 0;
  for (let i = 1; i <= n; i++) {
    const t = i / n;
    ox = ox * 0.6 + (r() - 0.5) * j; oy = oy * 0.6 + (r() - 0.5) * j;
    g.lineTo(x0 + (x1 - x0) * t + (i < n ? ox : 0), y0 + (y1 - y0) * t + (i < n ? oy : 0));
  }
  g.stroke();
};
/** Polygon cracks in an ellipse (alligator cracking): a jittered grid joined to its neighbours. */
function alligator(g: CanvasRenderingContext2D, r: () => number, cx: number, cy: number, rx: number, ry: number, cell: number, col: string) {
  const nx = Math.ceil(rx * 2 / cell) + 1, ny = Math.ceil(ry * 2 / cell) + 1, pts: [number, number][] = [];
  for (let j = 0; j < ny; j++) for (let i = 0; i < nx; i++) pts.push([cx - rx + i * cell + (r() - 0.5) * cell * 0.8, cy - ry + j * cell + (r() - 0.5) * cell * 0.8]);
  const inside = ([x, y]: [number, number]) => ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1 - r() * 0.25;
  g.strokeStyle = col; g.lineCap = 'round'; g.lineJoin = 'round';
  for (let j = 0; j < ny; j++) for (let i = 0; i < nx; i++) {
    const a = pts[j * nx + i];
    if (!inside(a)) continue;
    for (const [di, dj] of [[1, 0], [0, 1]]) {
      if (i + di >= nx || j + dj >= ny || r() < 0.12) continue;
      const b = pts[(j + dj) * nx + i + di];
      if (!inside(b)) continue;
      g.lineWidth = 0.8 + r() * 1.4;
      g.beginPath(); g.moveTo(a[0], a[1]); g.quadraticCurveTo((a[0] + b[0]) / 2 + (r() - 0.5) * cell * 0.3, (a[1] + b[1]) / 2 + (r() - 0.5) * cell * 0.3, b[0], b[1]); g.stroke();
    }
  }
}
const DRAW: Record<WearKind, Pen> = {
  // where a car stands every night: a dark stain, darker in the middle, a brown rim
  oil: (g, r) => {
    for (let i = 0; i < 14; i++) blob(g, 128 + (r() - 0.5) * 120, 128 + (r() - 0.5) * 90, 22 + r() * 46, 14 + r() * 30, r() * 3, '14,11,8', 0.3 + r() * 0.22);
    for (let i = 0; i < 5; i++) blob(g, 128 + (r() - 0.5) * 50, 128 + (r() - 0.5) * 40, 10 + r() * 22, 8 + r() * 16, r() * 3, '4,3,2', 0.6);
    for (let i = 0; i < 6; i++) blob(g, 128 + (r() - 0.5) * 150, 128 + (r() - 0.5) * 110, 18 + r() * 20, 10 + r() * 12, r() * 3, '40,28,16', 0.12);
  },
  // drops from a sump and a leaking pan, in a line
  drips: (g, r) => {
    for (let i = 0; i < 46; i++) {
      const t = r(), x = 40 + t * 176 + (r() - 0.5) * 30, y = 128 + Math.sin(t * 5) * 14 + (r() - 0.5) * 34;
      blob(g, x, y, 3 + r() * 10, 3 + r() * 8, r() * 3, '8,6,4', 0.35 + r() * 0.35);
    }
  },
  // two tyres' rubber in a curve, where the trucks turn
  rubber: (g, r) => {
    g.lineCap = 'round';
    for (const off of [-46, 46]) for (let k = 0; k < 5; k++) {
      g.strokeStyle = `rgba(10,10,10,${0.06 + r() * 0.07})`; g.lineWidth = 10 + r() * 14;
      g.beginPath(); g.moveTo(10, 230 + off * 0.2); g.bezierCurveTo(70, 150 + off + (r() - 0.5) * 8, 130, 100 + off, 250, 70 + off * 0.9 + (r() - 0.5) * 8); g.stroke();
    }
    g.globalCompositeOperation = 'destination-in';
    const f = g.createLinearGradient(0, 0, C, 0);
    f.addColorStop(0, 'rgba(0,0,0,0)'); f.addColorStop(0.2, 'rgba(0,0,0,1)'); f.addColorStop(0.8, 'rgba(0,0,0,1)'); f.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = f; g.fillRect(0, 0, C, C);
    g.globalCompositeOperation = 'source-over';
  },
  // alligator cracking in a wheel path, the broken pieces a little darker
  cracks: (g, r) => {
    blob(g, 128, 128, 112, 84, 0, '20,20,20', 0.12);
    alligator(g, r, 128, 128, 110, 84, 19, 'rgba(6,6,6,0.85)');
  },
  // a patch of newer, blacker asphalt, its edges sealed with tar
  patch: (g, r) => {
    const pts: [number, number][] = [];
    for (const [x, y] of [[22, 20], [234, 26], [236, 232], [20, 236]] as const) pts.push([x + (r() - 0.5) * 10, y + (r() - 0.5) * 10]);
    g.beginPath(); pts.forEach(([x, y], i) => (i ? g.lineTo(x, y) : g.moveTo(x, y))); g.closePath();
    g.fillStyle = 'rgba(16,16,18,0.5)'; g.fill();
    for (let i = 0; i < 900; i++) { g.fillStyle = r() < 0.5 ? 'rgba(70,70,72,.35)' : 'rgba(8,8,8,.35)'; g.fillRect(30 + r() * 196, 30 + r() * 196, 1 + r() * 2, 1 + r() * 2); }
    g.strokeStyle = 'rgba(5,5,5,0.6)'; g.lineWidth = 3.5; g.lineJoin = 'round';
    for (let k = 0; k < 4; k++) { const a = pts[k], b = pts[(k + 1) % 4]; wander(g, r, a[0], a[1], b[0], b[1], 8, 4); }
  },
  // crack sealer: tar snakes across and along, wandering, a narrow core and a thin band over it
  seal: (g, r) => {
    g.lineCap = 'round'; g.lineJoin = 'round';
    const snake = (x0: number, y0: number, x1: number, y1: number, w: number) => {
      const seed = r() * 1e6;
      g.strokeStyle = 'rgba(8,8,8,.3)'; g.lineWidth = w + 5; wander(g, rng(seed), x0, y0, x1, y1, 14, 12);
      g.strokeStyle = 'rgba(5,5,5,.6)'; g.lineWidth = w; wander(g, rng(seed), x0, y0, x1, y1, 14, 12);
    };
    for (let k = 0; k < 3; k++) snake(4, 30 + r() * 196, 252, 30 + r() * 196, 2 + r() * 2.5);
    snake(60 + r() * 136, 4, 60 + r() * 136, 252, 2);
  },
  // where the water stood last: a soft dark patch (wet() makes it shine)
  damp: (g, r) => {
    for (let i = 0; i < 16; i++) blob(g, 128 + (r() - 0.5) * 130, 128 + (r() - 0.5) * 120, 30 + r() * 50, 22 + r() * 40, r() * 3, '6,6,7', 0.12 + r() * 0.1);
  },
  // gravel and dust carried onto the paving from a dirt edge (the bottom of the cell)
  grit: (g, r) => {
    for (let i = 0; i < 2600; i++) {
      const y = C * (1 - Math.pow(r(), 2.2)), x = r() * C, s = 1 + r() * 2.5;
      g.fillStyle = r() < 0.75 ? `rgba(${150 + r() * 50},${132 + r() * 40},${104 + r() * 30},${0.4 + r() * 0.5})` : 'rgba(40,36,32,.6)';
      g.fillRect(x, y, s, s);
    }
    const f = g.createLinearGradient(0, 0, 0, C);
    f.addColorStop(0, 'rgba(150,130,100,0)'); f.addColorStop(1, 'rgba(150,130,100,.35)');
    g.fillStyle = f; g.fillRect(0, 0, C, C);
  },
};

// ---------- decals ----------
/** One decal: centre (area-local x, z), size (w across, l along its own z), turned ry, which picture. */
export type Wear = { x: number; z: number; w: number; l: number; ry: number; k: WearKind };
/** The decals as one mesh lying on ground(x, z) + lift (in the same terms as x and z), lit by a
 *  flood set. Not merged with statics (noMerge), drawn first of the see-through things (the
 *  lamps' glows and haze go over them), and hidden on Low. */
export function wearMesh(list: Wear[], ground: (x: number, z: number) => number, set: FloodSet, lift = 0.012, fall = 0.012) {
  const pos: number[] = [], uv: number[] = [], idx: number[] = [];
  for (const d of list) {
    const i = CELLS.indexOf(d.k), u0 = (i % 4) / 4, v0 = 1 - (Math.floor(i / 4) + 1) / 2;
    const nx = Math.max(1, Math.ceil(d.w / 1.5)), nz = Math.max(1, Math.ceil(d.l / 1.5)), c = Math.cos(d.ry), s = Math.sin(d.ry);
    const base = pos.length / 3;
    for (let j = 0; j <= nz; j++) for (let q = 0; q <= nx; q++) {
      const a = (q / nx - 0.5) * d.w, b = (j / nz - 0.5) * d.l;
      const x = d.x + a * c + b * s, z = d.z - a * s + b * c;
      pos.push(x, ground(x, z) + lift, z);
      // a small inset keeps the next cell out of the mip levels
      uv.push(u0 + (0.01 + 0.98 * q / nx) / 4, v0 + (0.01 + 0.98 * j / nz) / 2);
    }
    for (let j = 0; j < nz; j++) for (let q = 0; q < nx; q++) {
      const a = base + j * (nx + 1) + q, e = a + nx + 1;
      idx.push(a, e, a + 1, a + 1, e, e + 1);
    }
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(pos.map((_, i) => (i % 3 === 1 ? 1 : 0)), 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setIndex(idx);
  g.computeBoundingSphere();
  const m = wet(floodlit(new THREE.MeshStandardMaterial({ map: wearAtlas(), transparent: true, depthWrite: false, roughness: 0.6,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }), fall, set), 1.4, 60, 3);
  m.name = 'wear';
  const mesh = new THREE.Mesh(g, m);
  mesh.name = 'wear'; mesh.renderOrder = -1;
  mesh.userData.noMerge = true; mesh.userData.lowOff = true;
  return mesh;
}
/** Decals scattered by a seeded random: n of them, kinds weighted, inside a rectangle (local),
 *  each `size` [min, max] metres, kept where ok(x, z) says yes. */
export function scatter(seed: number, n: number, kinds: [WearKind, number][], r0: { x0: number; x1: number; z0: number; z1: number }, size: [number, number], ok: (x: number, z: number) => boolean = () => true, along = 0) {
  const r = rng(seed), out: Wear[] = [], tot = kinds.reduce((a, k) => a + k[1], 0);
  for (let t = 0; t < n * 4 && out.length < n; t++) {
    let p = r() * tot, k = kinds[0][0];
    for (const [kk, w] of kinds) { if ((p -= w) <= 0) { k = kk; break; } }
    const x = r0.x0 + r() * (r0.x1 - r0.x0), z = r0.z0 + r() * (r0.z1 - r0.z0), s = size[0] + r() * (size[1] - size[0]);
    if (!ok(x, z)) continue;
    out.push({ x, z, w: s * (0.7 + r() * 0.5), l: s * (0.8 + r() * 0.6), ry: along + (r() - 0.5) * (along ? 0.5 : 6.3), k });
  }
  return out;
}

// ---------- the lots' asphalt ----------
let lot: THREE.CanvasTexture | null = null;
/** Old lot asphalt, 8 m to a picture (512 px) for surfaces with their uv in metres: the round
 *  9 picture laid twice each way, bleached a little, with hairline cracks and block cracks and
 *  the tar that seals them; the Ultra twin (normal and roughness) has the tar smoother. */
export function lotAsphaltTex() {
  if (lot) return lot;
  const W = 512, M = W / 8;
  const tar = document.createElement('canvas'); tar.width = tar.height = W;
  const t = tar.getContext('2d')!, r = rng(1986);
  const wrap = (fn: (dx: number, dy: number) => void) => { for (const dx of [-W, 0, W]) for (const dy of [-W, 0, W]) fn(dx, dy); };
  t.lineCap = 'round'; t.lineJoin = 'round';
  // block cracking in stretches of 1 to 4 m, along and across, sealed long ago: a narrow core
  // and a thinner band squeegeed over it
  for (let k = 0; k < 9; k++) {
    const across = k % 2 === 0, x0 = r() * W, y0 = r() * W, len = (1 + r() * 3) * M, a = (across ? 0 : Math.PI / 2) + (r() - 0.5) * 0.3, seed = r() * 1e6;
    wrap((dx, dy) => {
      const rr = rng(seed), x = x0 + dx, y = y0 + dy, w = 1.6 + rr() * 1.4;
      t.strokeStyle = 'rgba(8,8,8,.28)'; t.lineWidth = w + 4; wander(t, rng(seed), x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 10, 10);
      t.strokeStyle = 'rgba(5,5,5,.55)'; t.lineWidth = w; wander(t, rng(seed), x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 10, 10);
    });
  }
  // (the patches and the alligator cracking are decals, wearMesh: in the picture they would
  // come round again every 8 m)
  const draw = (g: CanvasRenderingContext2D, asphalt: HTMLImageElement, k: 'c' | 'n' | 'r') => {
    for (let y = 0; y < W; y += W / 2) for (let x = 0; x < W; x += W / 2) g.drawImage(asphalt, x, y, W / 2, W / 2);
    if (k === 'c') {
      g.fillStyle = 'rgba(150,146,138,.12)'; g.fillRect(0, 0, W, W);
      const rr = rng(47);
      for (let i = 0; i < 18; i++) blob(g, rr() * W, rr() * W, 30 + rr() * 70, 20 + rr() * 50, rr() * 3, rr() < 0.5 ? '0,0,0' : '170,165,155', 0.08);
      for (let i = 0; i < 40; i++) {   // hairline cracks
        const x = rr() * W, y = rr() * W, a = rr() * 7, len = (0.2 + rr() * 0.8) * M;
        g.strokeStyle = 'rgba(6,6,6,.4)'; g.lineWidth = 0.8; wander(g, rr, x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 5, 5);
      }
    }
    const flat = document.createElement('canvas'); flat.width = flat.height = W;
    const f = flat.getContext('2d')!;
    f.drawImage(tar, 0, 0);
    if (k !== 'c') { f.globalCompositeOperation = 'source-in'; f.fillStyle = k === 'n' ? 'rgb(128,128,255)' : 'rgb(70,70,70)'; f.fillRect(0, 0, W, W); }
    g.drawImage(flat, 0, 0);
  };
  lot = canvasTex(W, W, (g) => draw(g, artImage('asphalt'), 'c'), [1 / 8, 1 / 8]);
  lot.name = 'lotAsphalt';
  dataTwin(lot, W, W, (g, _w, _h, k) => draw(g, artImage(k === 'n' ? 'asphaltN' : 'asphaltR'), k));
  return lot;
}
