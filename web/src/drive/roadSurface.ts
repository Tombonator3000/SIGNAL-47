import * as THREE from 'three';
import { canvasTex, dataTwin, rng } from '../core/textures';
import { artImage } from '../core/art';
import { floodlit, type FloodSet } from '../world/kit';

// What makes the paved places of the night read as real (Tom, 19 October: «more realistic
// environments with more detail»; the concept art of Caliche and the payphone): the old asphalt
// and concrete hold the sodium lamps and the headlights where they are damp, matte where they are
// dry, and they are worn, with oil where cars stand, rubber where they turn, patches and cracks
// sealed with tar.
//
//   wet()        a fake reflection of the floods (kit.floodlit) on a surface: for each lamp a
//                highlight round the mirror point between it and the eye, held tight across the
//                view and long along it (a column down the road towards the eye, as on wet
//                asphalt), strongest at grazing angles, glossier on the broad dark shapes of the
//                picture (tar, patches, oil) than the light (paint, gravel), cut by the open
//                cracks and broken into glints near by; only where it is damp: patches with hard
//                edges, more in the low places of a road, and wet spots (a hosed apron). In the
//                floods' own loop, so it costs one pow per lamp and pixel; High and Ultra only
//                (the Lambert copies of Low compile without it).
//   wearMesh()   decals from one shared atlas (oil, drips, rubber, cracks, patches, tar snakes,
//                damp, grit, tyre tracks in gravel, a ditch, a puddle), every one in an area one
//                mesh and one draw call, lit by the area's floods and wet; hidden on Low
//                (userData.lowOff, core/quality.ts).
//   lotAsphaltTex() the old asphalt of the lots and the service road, 8 m to a picture, with its
//                hairline and block cracks and the tar that seals them, shared by SARO and the motel.

// ---------- the sheen ----------
// The floods' loop in kit.floodlit, where the reflection of each lamp is added.
const FLOOD_LINE = 'fAcc += uFloodCol[i] * (uFloodPos[i].w * ndl * win * win / (1.0 + d2 * uFloodFall));';
const FLOOD_N = 'vec3 fN = normalize(vFNormal);';
const FLOOD_OUT = 'totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;';
const WET_GLSL = `
  float wSq(float x) { return x * x; }
  // slow damp patches, a few metres across (world metres), ragged at the edge
  float wetNoise(vec2 p) {
    float n = sin(p.x * 0.29 + sin(p.y * 0.21) * 1.8) * sin(p.y * 0.25 + sin(p.x * 0.17 + 1.3) * 2.1);
    n += 0.5 * sin(p.x * 0.83 - p.y * 0.61 + 2.0) * sin(p.y * 0.97 + 0.4);
    return n + 0.14 * sin(p.x * 3.1 + p.y * 2.3) * sin(p.y * 2.9 - p.x * 1.7);
  }
  // where the water lies and how much it gives back (x wet or dry, y how strong): the patches, with
  // hard edges, more of them in the low places of a road (u across it: along its paved edges and
  // in the polished wheel paths, fewer on the crown), and the spots that are wetter still
  vec2 wetAt(vec2 p, float u) {
    float n = wetNoise(p);
    if (uWetLane.x > 0.0) {
      float m = abs(u - 0.5) * uWetLane.x;
      n += 0.6 * exp(-wSq((m - uWetLane.y) / 0.45)) + 0.32 * (exp(-wSq((m - uWetLane.z) / 0.3)) + exp(-wSq((m - uWetLane.w) / 0.3))) - 0.3;
    }
    float w = uWetMode.x > 0.5 ? 1.0 : smoothstep(uWetMode.w, uWetMode.w + 0.14, n), k = uWet.x;
    for (int i = 0; i < 4; i++) {
      vec4 s = uWetSpot[i];
      if (s.z <= 0.0) continue;
      float e = 1.0 - smoothstep(0.92, 1.0, length((p - s.xy) / s.zw) + n * 0.07);
      w = max(w, e); k = mix(k, uWet.w, e);
    }
    return vec2(w, k);
  }
  // Fresnel (grazing views most), the gloss of the broad shapes (tar and oil shine, paint and
  // gravel hardly), and the damp
  float wetness(vec3 n, vec3 v, float lum, vec2 p, float u) {
    float fres = 0.05 + 0.95 * pow(1.0 - clamp(dot(n, v), 0.0, 1.0), 5.0);
    vec2 w = wetAt(p, u);
    return fres * clamp(1.2 - lum * uWet.z, 0.0, 1.0) * w.x * w.y;
  }`;
const WET_LAMP = `
        {
          float wD = sqrt(d2), wWin = clamp(1.0 - wD / (uFloodPos[i].w * 3.0 + 22.0), 0.0, 1.0);
          // the half vector with its part across the view held 2.5 times tighter than its part
          // along it: each lamp a long column (the little lift keeps it from ever being nothing:
          // a NaN would bloom, memory.md); the tighter, the brighter its peak, as Blinn's own
          vec3 wH = normalize(L + wV + vec3(0.0, 1e-4, 0.0));
          float hn = max(dot(wH, fN), 0.0), ht = dot(wH, wT), hb = dot(wH, wB);
          float wS = pow(hn * inversesqrt(max(hn * hn + ht * ht * 6.25 + hb * hb * 0.5, 1e-6)), uWet.y) * (uWet.y + 8.0) / 48.0 * step(0.0, dot(fN, L));
          // not the truck's own headlights: on a damp road their bounce goes off ahead, away from the eye
          wS *= smoothstep(2.0, 6.0, distance(uFloodPos[i].xyz, cameraPosition));
          wAcc += uFloodCol[i] * (uFloodPos[i].w * wS * wWin * wWin / (1.0 + d2 * uFloodFall * 0.25));
        }`;
const WET_OUT = `
        // the gloss from the broad shapes of the picture (a blurred look at it: tar, patches,
        // oil, the polished wheel paths), not from every dark pore; and the open cracks, much
        // darker than round them, cut dark lines through the streak
        float wLum = dot(diffuseColor.rgb, vec3(0.3, 0.59, 0.11)), wCut = 1.0, wU = 0.5;
        #ifdef USE_MAP
          float wFine = dot(texture2D(map, vMapUv).rgb, vec3(0.3, 0.59, 0.11));
          float wR = wFine / max(dot(texture2D(map, vMapUv, uWetMode.y).rgb, vec3(0.3, 0.59, 0.11)), 1e-4);
          wLum /= max(wR, 0.05); wCut = smoothstep(0.03, 0.15, wR); wU = vMapUv.x;
        #endif
        // near by the streak breaks into glints, the stones of the aggregate (2 cm, world)
        vec2 wC = mod(floor(vFWorld.xz * 50.0), 1024.0);
        vec3 wP = fract(vec3(wC.xyx) * 0.1031); wP += dot(wP, wP.yzx + 33.33);
        float wG = mix(1.0, fract((wP.x + wP.y) * wP.z) > 0.84 ? 2.2 : 0.77, uWetMode.z * (1.0 - smoothstep(5.0, 18.0, distance(cameraPosition, vFWorld))));
        totalEmissiveRadiance += wAcc * wetness(fN, wV, wLum, vFWorld.xz, wU) * wCut * wG * uFloodScale;`;
/** How a surface is damp (wet()): `lanes` for a road's picture (u across it): its width in metres,
 *  the paved half-width and the inner and outer wheel paths, where the wet gathers; `spots`, up to
 *  four ellipses in world terms (x, z, rx, rz) wetter still, at `spotK`; `even`, damp all over
 *  (the decals, whose own darkness says where); `blur`, the mip bias of the broad look at the
 *  picture the gloss is read from (0: the texel itself); `glints`, how much it breaks into glints;
 *  `dry`, how much of it is dry (the level of the patches' noise, about -1.5 to 1.5, under which). */
export type WetOpts = { lanes?: [number, number, number, number]; spots?: [number, number, number, number][]; spotK?: number; even?: boolean; blur?: number; glints?: number; dry?: number };
/** A highway's picture (roadTextures.highwayTex: 12 m across, paved to 4.8 m, the wheel paths
 *  either side of 1.8 m) and the old road's (8.8 m across, paved to 3.2 m). */
export const HWY_LANES: [number, number, number, number] = [12, 4.5, 0.95, 2.65];
export const OLD_LANES: [number, number, number, number] = [8.8, 2.95, 0.85, 2.35];
/** A floodlit material with a damp sheen: k how strong where it is damp, shine the exponent
 *  (higher is tighter and wetter), dark how fast lighter parts of the picture lose it (linear
 *  luminance). Its uniforms are in userData.wet (k, shine, dark, spotK) and userData.wetSpots. */
export function wet<T extends THREE.MeshStandardMaterial>(m: T, k = 1, shine = 100, dark = 5, o: WetOpts = {}): T {
  const prev = m.onBeforeCompile, key = m.customProgramCacheKey.bind(m);
  const u = { value: new THREE.Vector4(k, shine, dark, o.spotK ?? 1.4) };
  const lane = { value: new THREE.Vector4(...(o.lanes ?? [0, 0, 0, 0])) };
  const mode = { value: new THREE.Vector4(o.even ? 1 : 0, o.blur ?? 4, o.glints ?? 1, o.dry ?? 0.36) };
  // (full length from the start, neutral where unused: a uniform list must have its values before the first draw)
  const spots = { value: [0, 1, 2, 3].map((i) => new THREE.Vector4(...(o.spots?.[i] ?? [0, 0, 0, 0]))) };
  m.userData.wet = u;   // (tools/env_roads.py turns it off to see what it adds)
  m.userData.wetSpots = spots.value; m.userData.wetMode = mode.value; m.userData.wetLane = lane.value;
  m.onBeforeCompile = (sh, r) => {
    prev.call(m, sh, r);
    if (sh.fragmentShader.includes('#define LAMBERT')) return;   // Low
    if (!sh.fragmentShader.includes(FLOOD_LINE)) { console.warn('wet: not a floodlit material', m.name); return; }
    Object.assign(sh.uniforms, { uWet: u, uWetLane: lane, uWetMode: mode, uWetSpot: spots });
    sh.fragmentShader = sh.fragmentShader
      .replace('uniform float uFloodScale;', `uniform float uFloodScale; uniform vec4 uWet; uniform vec4 uWetLane; uniform vec4 uWetMode; uniform vec4 uWetSpot[4];${WET_GLSL}`)
      .replace('vec3 fAcc = vec3(0.0);', 'vec3 fAcc = vec3(0.0); vec3 wAcc = vec3(0.0); vec3 wV = normalize(cameraPosition - vFWorld);')
      // across the view and along it, on the surface
      .replace(FLOOD_N, `${FLOOD_N} vec3 wT = cross(fN, wV); wT *= inversesqrt(max(dot(wT, wT), 1e-8)); vec3 wB = cross(wT, fN);`)
      .replace(FLOOD_LINE, FLOOD_LINE + WET_LAMP)
      .replace(FLOOD_OUT, FLOOD_OUT + WET_OUT);
  };
  m.customProgramCacheKey = () => key() + 'wet';
  return m;
}

// ---------- the wear atlas: twelve cells of 256 px ----------
export type WearKind = 'oil' | 'drips' | 'rubber' | 'cracks' | 'patch' | 'seal' | 'damp' | 'grit' | 'tread' | 'fade' | 'swale' | 'puddle';
const CELLS: WearKind[] = ['oil', 'drips', 'rubber', 'cracks', 'patch', 'seal', 'damp', 'grit', 'tread', 'fade', 'swale', 'puddle'];
const C = 256, COLS = 4, ROWS = 3;
let atlas: THREE.CanvasTexture | null = null;
/** The decals' picture, drawn once and shared by every area (a disposed copy is uploaded again
 *  by three.js the next time it is drawn). */
export function wearAtlas() {
  if (atlas) return atlas;
  atlas = canvasTex(C * COLS, C * ROWS, (g) => {
    g.clearRect(0, 0, C * COLS, C * ROWS);
    CELLS.forEach((k, i) => {
      g.save(); g.translate((i % COLS) * C, Math.floor(i / COLS) * C);
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
/** Fade a cell out at its sides: across (left and right) from `a` in, along (top and bottom) from `b` in. */
const feather = (g: CanvasRenderingContext2D, a: number, b: number) => {
  g.globalCompositeOperation = 'destination-in';
  for (const [x1, y1] of [[C, 0], [0, C]]) {
    const f = g.createLinearGradient(0, 0, x1, y1), e = x1 ? a : b;
    f.addColorStop(0, 'rgba(0,0,0,0)'); f.addColorStop(e, 'rgba(0,0,0,1)'); f.addColorStop(1 - e, 'rgba(0,0,0,1)'); f.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = f; g.fillRect(0, 0, C, C);
  }
  g.globalCompositeOperation = 'source-over';
};
/** The curve two tyres follow through a turn (rubber, tread), at t 0 to 1, `off` pixels to one side. */
const turn = (t: number, off: number): [number, number] => {
  const u = 1 - t, b = (p0: number, p1: number, p2: number, p3: number) => u * u * u * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t * p3;
  return [b(10, 70, 130, 250), b(230 + off * 0.2, 150 + off, 100 + off, 70 + off * 0.9)];
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
      g.lineWidth = 0.7 + r() * 0.9;
      g.beginPath(); g.moveTo(a[0], a[1]); g.quadraticCurveTo((a[0] + b[0]) / 2 + (r() - 0.5) * cell * 0.3, (a[1] + b[1]) / 2 + (r() - 0.5) * cell * 0.3, b[0], b[1]); g.stroke();
    }
  }
}
/** A patch: a skewed quad cut in, one or two corners chipped, of newer asphalt (blacker) or old
 *  (sun-faded, lighter than the road round it), its seam a fine line of tar and a crack carried
 *  across it from the road. */
function patchCell(g: CanvasRenderingContext2D, r: () => number, old: boolean) {
  const q: [number, number][] = [[24, 22], [232, 30], [236, 230], [18, 238]].map(([x, y]) => [x + (r() - 0.5) * 18, y + (r() - 0.5) * 18]);
  const pts: [number, number][] = [];
  q.forEach((a, i) => {
    const b = q[(i + 1) % 4], p = q[(i + 3) % 4];
    // a chipped corner: the corner cut off by a few centimetres, a bite out of it
    if (i === 1 || (i === 3 && r() < 0.6)) { const c = 0.08 + r() * 0.1; pts.push([a[0] + (p[0] - a[0]) * c, a[1] + (p[1] - a[1]) * c], [a[0] + (b[0] - a[0]) * c * 0.6 + (r() - 0.5) * 6, a[1] + (b[1] - a[1]) * c * 0.6 + (r() - 0.5) * 6]); }
    else pts.push(a);
  });
  const path = () => { g.beginPath(); pts.forEach(([x, y], i) => (i ? g.lineTo(x, y) : g.moveTo(x, y))); g.closePath(); };
  path(); g.fillStyle = old ? 'rgba(168,162,150,.17)' : 'rgba(14,14,16,.36)'; g.fill();
  g.save(); path(); g.clip();
  for (let i = 0; i < 1400; i++) {
    const light = r() < (old ? 0.55 : 0.3);
    g.fillStyle = light ? `rgba(${old ? 176 : 92},${old ? 170 : 92},${old ? 160 : 94},.3)` : 'rgba(8,8,8,.3)';
    g.fillRect(r() * C, r() * C, 1 + r() * 1.6, 1 + r() * 1.6);
  }
  // a darker band where the roller left the edge thicker, inside the seam
  g.strokeStyle = old ? 'rgba(40,38,36,.06)' : 'rgba(6,6,6,.12)'; g.lineWidth = 6; path(); g.stroke();
  g.restore();
  // the seam: a single fine line of tar
  g.strokeStyle = 'rgba(6,6,6,.25)'; g.lineWidth = 1; g.lineJoin = 'round'; path(); g.stroke();
  // a crack from the road carried on across it
  g.strokeStyle = 'rgba(5,5,5,.6)'; g.lineWidth = 1; g.lineCap = 'round';
  const y0 = 50 + r() * 150;
  wander(g, r, 0, y0, C, y0 + (r() - 0.5) * 90, 16, 9);
}
const DRAW: Record<WearKind, Pen> = {
  // where a car stands every night: a wide faint stain (the decals are 1.5 to 2.5 m), a darker
  // core of 30 to 40 cm under the sump, a brown rim
  oil: (g, r) => {
    for (let i = 0; i < 12; i++) blob(g, 128 + (r() - 0.5) * 110, 128 + (r() - 0.5) * 90, 44 + r() * 60, 30 + r() * 40, r() * 3, '14,11,8', 0.035 + r() * 0.03);
    for (let i = 0; i < 4; i++) blob(g, 128 + (r() - 0.5) * 22, 126 + (r() - 0.5) * 18, 12 + r() * 12, 9 + r() * 9, r() * 3, '5,4,3', 0.2 + r() * 0.1);
    for (let i = 0; i < 6; i++) blob(g, 128 + (r() - 0.5) * 160, 128 + (r() - 0.5) * 120, 20 + r() * 26, 12 + r() * 14, r() * 3, '40,28,16', 0.05);
  },
  // drops from a sump and a leaking pan, in a line
  drips: (g, r) => {
    for (let i = 0; i < 46; i++) {
      const t = r(), x = 40 + t * 176 + (r() - 0.5) * 30, y = 128 + Math.sin(t * 5) * 14 + (r() - 0.5) * 34;
      blob(g, x, y, 3 + r() * 9, 3 + r() * 7, r() * 3, '8,6,4', 0.25 + r() * 0.3);
    }
  },
  // two tyres' rubber in a curve, where the trucks turn: wide (20 to 25 cm on a 4 m decal) and
  // very faint, feathered, smudged with the dirt they carried, no line to it
  rubber: (g, r) => {
    g.lineCap = 'round';
    for (const off of [-50, 50]) {
      const j = [(r() - 0.5) * 10, (r() - 0.5) * 10];
      for (let k = 0; k < 7; k++) {
        g.strokeStyle = `rgba(16,15,14,${0.005 + r() * 0.003})`; g.lineWidth = 24 - k * 2;
        g.beginPath(); g.moveTo(10, 230 + off * 0.2); g.bezierCurveTo(70, 150 + off + j[0], 130, 100 + off, 250, 70 + off * 0.9 + j[1]); g.stroke();
      }
      for (let i = 0; i < 26; i++) { const [x, y] = turn(r(), off); blob(g, x + (r() - 0.5) * 10, y + (r() - 0.5) * 10, 8 + r() * 14, 5 + r() * 8, r() * 3, '22,20,17', 0.015 + r() * 0.015); }
    }
    feather(g, 0.2, 0.15);
  },
  // alligator cracking in a wheel path, the broken pieces a little darker
  cracks: (g, r) => {
    blob(g, 128, 128, 112, 84, 0, '20,20,20', 0.1);
    alligator(g, r, 128, 128, 110, 84, 19, 'rgba(6,6,6,.7)');
  },
  // a patch of newer, blacker asphalt
  patch: (g, r) => patchCell(g, r, false),
  // crack sealer: tar snakes across and along, wandering, a narrow core and a faint band over it
  seal: (g, r) => {
    g.lineCap = 'round'; g.lineJoin = 'round';
    const snake = (x0: number, y0: number, x1: number, y1: number, w: number) => {
      const seed = r() * 1e6;
      g.strokeStyle = 'rgba(8,8,8,.15)'; g.lineWidth = w + 5; wander(g, rng(seed), x0, y0, x1, y1, 14, 12);
      g.strokeStyle = 'rgba(5,5,5,.55)'; g.lineWidth = w; wander(g, rng(seed), x0, y0, x1, y1, 14, 12);
    };
    for (let k = 0; k < 3; k++) snake(4, 30 + r() * 196, 252, 30 + r() * 196, 1.4 + r() * 1.4);
    snake(60 + r() * 136, 4, 60 + r() * 136, 252, 1.4);
  },
  // where the water stood last: a soft dark patch (wet() makes it shine)
  damp: (g, r) => {
    for (let i = 0; i < 16; i++) blob(g, 128 + (r() - 0.5) * 130, 128 + (r() - 0.5) * 120, 30 + r() * 50, 22 + r() * 40, r() * 3, '6,6,7', 0.1 + r() * 0.1);
  },
  // gravel and dust carried onto the paving from a dirt edge (the bottom of the cell), thinning
  // out at every side
  grit: (g, r) => {
    for (let i = 0; i < 2600; i++) {
      const y = C * (1 - Math.pow(r(), 2.2)), x = r() * C, s = 1 + r() * 2.5;
      g.fillStyle = r() < 0.75 ? `rgba(${150 + r() * 50},${132 + r() * 40},${104 + r() * 30},${0.35 + r() * 0.45})` : 'rgba(40,36,32,.55)';
      g.fillRect(x, y, s, s);
    }
    const f = g.createLinearGradient(0, 0, 0, C);
    f.addColorStop(0, 'rgba(150,130,100,0)'); f.addColorStop(1, 'rgba(150,130,100,.28)');
    g.fillStyle = f; g.fillRect(0, 0, C, C);
    feather(g, 0.25, 0.12);
  },
  // the tracks of the night's trucks pressed into gravel as they swung in: where the tyres went
  // the gravel is packed darker, with the tread across it, and pushed up lighter either side
  tread: (g, r) => {
    for (const off of [-56, 56]) {
      for (let i = 0; i <= 90; i++) {
        const t = i / 90, [x, y] = turn(t, off), [x2, y2] = turn(Math.min(1, t + 0.01), off);
        const a = Math.atan2(y2 - y, x2 - x) + Math.PI / 2, ca = Math.cos(a), sa = Math.sin(a);
        for (const [o, w, col, k] of [[0, 15, '52,44,34', 0.15], [-10, 4, '214,198,168', 0.11], [10, 4, '214,198,168', 0.11]] as const) {
          g.fillStyle = `rgba(${col},${k * (0.7 + r() * 0.5)})`;
          g.save(); g.translate(x + ca * o, y + sa * o); g.rotate(a); g.fillRect(-w / 2, -2.2, w, 4.4); g.restore();
        }
        if (i % 2) { g.fillStyle = 'rgba(30,24,18,.12)'; g.save(); g.translate(x, y); g.rotate(a); g.fillRect(-6, -0.8, 12, 1.6); g.restore(); }   // the tread
      }
    }
    feather(g, 0.18, 0.15);
  },
  // an old patch, sun-faded lighter than the road round it
  fade: (g, r) => patchCell(g, r, true),
  // the shallow ditch along a verge where a culvert takes the water under a track: darker down the
  // middle (along the cell), silt left lighter in streaks by the last storm, small stones
  swale: (g, r) => {
    const gr = g.createLinearGradient(0, 0, C, 0);
    gr.addColorStop(0, 'rgba(28,22,16,0)'); gr.addColorStop(0.28, 'rgba(28,22,16,.26)'); gr.addColorStop(0.5, 'rgba(24,19,14,.42)');
    gr.addColorStop(0.72, 'rgba(28,22,16,.26)'); gr.addColorStop(1, 'rgba(28,22,16,0)');
    g.fillStyle = gr; g.fillRect(0, 0, C, C);
    g.lineCap = 'round';
    for (let i = 0; i < 26; i++) { const x = 96 + r() * 64; g.strokeStyle = `rgba(196,178,146,${0.06 + r() * 0.08})`; g.lineWidth = 2 + r() * 5; wander(g, r, x, r() * 60, x + (r() - 0.5) * 20, 196 + r() * 60, 6, 6); }
    for (let i = 0; i < 300; i++) { const x = 60 + r() * 136; g.fillStyle = r() < 0.5 ? 'rgba(60,52,42,.5)' : 'rgba(170,154,124,.4)'; g.fillRect(x, r() * C, 1 + r() * 2.5, 1 + r() * 2.5); }
    feather(g, 0.05, 0.12);
  },
  // standing water where the apron was hosed: dark, with a crisp edge (wet() makes it shine)
  puddle: (g, r) => {
    for (const [cx, cy, rad] of [[118, 132, 92], [196, 70, 34]]) {
      const n = 28, ph = [r() * 7, r() * 7, r() * 7], pts: [number, number][] = [];
      for (let i = 0; i < n; i++) {
        const a = i / n * Math.PI * 2, d = rad * (0.82 + 0.12 * Math.sin(a * 2 + ph[0]) + 0.07 * Math.sin(a * 5 + ph[1]) + 0.04 * Math.sin(a * 9 + ph[2]));
        pts.push([cx + Math.cos(a) * d, cy + Math.sin(a) * d * 0.8]);
      }
      g.beginPath();
      pts.forEach((p, i) => { const q = pts[(i + 1) % n], mx = (p[0] + q[0]) / 2, my = (p[1] + q[1]) / 2; if (i) g.quadraticCurveTo(p[0], p[1], mx, my); else g.moveTo(mx, my); });
      g.closePath();
      g.fillStyle = 'rgba(7,7,8,.42)'; g.fill();
      g.strokeStyle = 'rgba(5,5,6,.18)'; g.lineWidth = 2.5; g.stroke();
    }
  },
};

// ---------- decals ----------
/** One decal: centre (area-local x, z), size (w across, l along its own z), turned ry, which picture. */
export type Wear = { x: number; z: number; w: number; l: number; ry: number; k: WearKind };
/** The decals as one mesh lying on ground(x, z) + lift (in the same terms as x and z), lit by a
 *  flood set and damp at k (all over: their own darkness says where; about 0.1 on gravel, where
 *  nothing shines). Not merged with statics (noMerge), drawn first of the see-through things (the
 *  lamps' glows and haze go over them), and hidden on Low. */
export function wearMesh(list: Wear[], ground: (x: number, z: number) => number, set: FloodSet, lift = 0.012, fall = 0.012, k = 1.3) {
  const pos: number[] = [], uv: number[] = [], idx: number[] = [];
  for (const d of list) {
    const i = CELLS.indexOf(d.k), u0 = (i % COLS) / COLS, v0 = 1 - (Math.floor(i / COLS) + 1) / ROWS;
    const nx = Math.max(1, Math.ceil(d.w / 1.5)), nz = Math.max(1, Math.ceil(d.l / 1.5)), c = Math.cos(d.ry), s = Math.sin(d.ry);
    const base = pos.length / 3;
    for (let j = 0; j <= nz; j++) for (let q = 0; q <= nx; q++) {
      const a = (q / nx - 0.5) * d.w, b = (j / nz - 0.5) * d.l;
      const x = d.x + a * c + b * s, z = d.z - a * s + b * c;
      pos.push(x, ground(x, z) + lift, z);
      // a small inset keeps the next cell out of the mip levels
      uv.push(u0 + (0.01 + 0.98 * q / nx) / COLS, v0 + (0.01 + 0.98 * j / nz) / ROWS);
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
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }), fall, set), k, 100, 3, { even: true, blur: 0 });
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
/** Old lot asphalt, 8 m to a picture, for surfaces with their uv in metres: the round 9 picture
 *  laid twice each way (512 px to 4 m, about 8 mm a texel, near the 5 mm the lots had with the
 *  picture alone; one picture for High and Low, which share their maps: tools/artcheck.py),
 *  bleached a little, with hairline cracks and a few block cracks and the tar that seals them (the
 *  rest of the cracks and the patches are decals, wearMesh, so the 8 m does not show); the Ultra
 *  twin (normal and roughness) has the tar smoother. */
export function lotAsphaltTex() {
  if (lot) return lot;
  const W = 1024, M = W / 8, P = W / 1024;   // P: pixels of this picture to one of a 1024 one
  const tar = document.createElement('canvas'); tar.width = tar.height = W;
  const t = tar.getContext('2d')!, r = rng(1986);
  const wrap = (fn: (dx: number, dy: number) => void) => { for (const dx of [-W, 0, W]) for (const dy of [-W, 0, W]) fn(dx, dy); };
  t.lineCap = 'round'; t.lineJoin = 'round';
  // block cracking in stretches of 1 to 4 m, along and across, sealed long ago: a narrow core
  // (1.2 to 1.6 cm) and a faint band squeegeed over it (about 5 cm)
  for (let k = 0; k < 5; k++) {
    const across = k % 2 === 0, x0 = r() * W, y0 = r() * W, len = (1 + r() * 3) * M, a = (across ? 0 : Math.PI / 2) + (r() - 0.5) * 0.3, seed = r() * 1e6;
    wrap((dx, dy) => {
      const rr = rng(seed), x = x0 + dx, y = y0 + dy, w = (1.5 + rr() * 0.6) * P;
      t.strokeStyle = 'rgba(8,8,8,.15)'; t.lineWidth = w + 4.5 * P; wander(t, rng(seed), x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 12, 14 * P);
      t.strokeStyle = 'rgba(5,5,5,.55)'; t.lineWidth = w; wander(t, rng(seed), x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 12, 14 * P);
    });
  }
  const draw = (g: CanvasRenderingContext2D, asphalt: HTMLImageElement, k: 'c' | 'n' | 'r') => {
    for (let y = 0; y < W; y += W / 2) for (let x = 0; x < W; x += W / 2) g.drawImage(asphalt, x, y, W / 2, W / 2);
    if (k === 'c') {
      g.fillStyle = 'rgba(150,146,138,.12)'; g.fillRect(0, 0, W, W);
      const rr = rng(47);
      for (let i = 0; i < 18; i++) blob(g, rr() * W, rr() * W, (60 + rr() * 140) * P, (40 + rr() * 100) * P, rr() * 3, rr() < 0.5 ? '0,0,0' : '170,165,155', 0.08);
      for (let i = 0; i < 40; i++) {   // hairline cracks
        const x = rr() * W, y = rr() * W, a = rr() * 7, len = (0.2 + rr() * 0.8) * M;
        g.strokeStyle = 'rgba(6,6,6,.4)'; g.lineWidth = Math.max(0.8, 1.1 * P); wander(g, rr, x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 7, 8 * P);
      }
    }
    const flat = document.createElement('canvas'); flat.width = flat.height = W;
    const f = flat.getContext('2d')!;
    f.drawImage(tar, 0, 0);
    if (k !== 'c') { f.globalCompositeOperation = 'source-in'; f.fillStyle = k === 'n' ? 'rgb(128,128,255)' : 'rgb(70,70,70)'; f.fillRect(0, 0, W, W); }
    g.drawImage(flat, 0, 0);
  };
  const tex = canvasTex(W, W, (g) => draw(g, artImage('asphalt'), 'c'), [1 / 8, 1 / 8]);
  tex.name = 'lotAsphalt';
  dataTwin(tex, W, W, (g, _w, _h, k) => draw(g, artImage(k === 'n' ? 'asphaltN' : 'asphaltR'), k));
  return (lot = tex);
}
