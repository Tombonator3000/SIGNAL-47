import * as THREE from 'three';
import { rng } from './textures';

// Light cookies (Tom, 19 October: «Lys-cookies først: lommelykta, frontlysene og vinduene i
// Caliche»): the picture a light throws. The flashlight's is worked out in the shaders, the
// windows' is drawn here in code, pixel by pixel, once.
//
//   the flashlight      the beam of a cheap flashlight of the eighties, in the shaders' own loop
//                       over the spot lights (core/Flashlight.ts): a warm hot core a little off
//                       the middle with the soft dark spot of the bulb's filament in it, in a
//                       broad flood about as bright as the old disc was, with two faint broken
//                       arcs from the stamped reflector in its outer half, a soft ragged edge with
//                       a touch of orange outside and of blue inside it, and a dim corona of spill
//                       round it. The flashlight is a real light on every tier, so Low, High and
//                       Ultra all throw it.
//   windowCookies()     the windows' light on the ground (world/windowLight.ts): one cell for
//                       each kind of window, the panes, the bars, the blinds and the curtains
//                       seen from the room's lamp, soft at their edges.
//
// The truck's headlights have no picture: their pattern is a few lines in the floods' shader
// (world/kit.ts, HEAD_GLSL), since their pools are fake floods on every tier.
//
// All in linear values: what the pattern gives is what multiplies the light.

const smooth = (a: number, b: number, x: number) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };
const gauss = (x: number, w: number) => Math.exp(-(x / w) * (x / w));

/** Pixels of a canvas texture from a function of (x, y) in 0..1, top left first: [r, g, b]. */
function pixels(w: number, h: number, f: (x: number, y: number) => [number, number, number]) {
  const c = document.createElement('canvas'); c.width = w; c.height = h;
  const g = c.getContext('2d')!, im = g.createImageData(w, h), d = im.data;
  for (let j = 0; j < h; j++) for (let i = 0; i < w; i++) {
    const [r, gg, b] = f((i + 0.5) / w, (j + 0.5) / h), k = (j * w + i) * 4;
    d[k] = Math.round(Math.min(1, Math.max(0, r)) * 255);
    d[k + 1] = Math.round(Math.min(1, Math.max(0, gg)) * 255);
    d[k + 2] = Math.round(Math.min(1, Math.max(0, b)) * 255);
    d[k + 3] = 255;
  }
  g.putImageData(im, 0, 0);
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.NoColorSpace;
  t.wrapS = t.wrapT = THREE.ClampToEdgeWrapping;
  return t;
}

// ---------- the flashlight ----------
// Not a picture but a few lines in three.js's own loop over the spot lights, for every lit material
// on every tier (the flashlight is the same real light on Low, High and Ultra). A SpotLight.map
// would do the same, but it costs every lit shader a varying, a matrix per vertex and a lookup in
// every pixel, lit by the flashlight or not: about 7 percent of a frame by SARO on Low on the
// graphics card (tools/cookies.py --cost, 19 October). This runs only where the beam falls
// (three.js has the light there already: directLight.visible), and costs nothing anywhere else.
// The flashlight is told from the other spot lights (Ultra's three) by its linear decay, which only
// it has (memory.md); Flashlight.cookie(false) moves it off 1 by a hair to show the old disc.
/** Where the beam ends, as a part of the cone (FLASH_ANGLE, tan to tan); the corona fills the outer
 *  15 percent. (0.73 left the bright beam at four fifths of the old disc's width: the art
 *  director, 19 October.) */
export const FLASH_BEAM = 0.85;
/** The cone (radians, the spot light's angle: the old disc's, so the shadow's camera on the PC
 *  tier sees no more than before). */
export const FLASH_ANGLE = 0.4;
/** How much the light is raised with its cookie: the flood is 0.55 of it (0.9 of the old disc's
 *  middle), the core about 1 (1.55 of the old middle). Over the old disc's pool it throws about
 *  1.1 times the old disc's light (tools/cookies.py measures it on the screen: at least 0.9). The
 *  first cookie's flood was 0.36: half the old light, a flashlight with its batteries dying (the
 *  art director, 19 October); the hot core, not a dim flood, keeps the close range from blowing out. */
export const FLASH_GAIN = 1.6;
/** The decay that marks the flashlight in the shaders (the cookie on), and the old disc's. */
export const FLASH_DECAY = 1, FLASH_PLAIN = 1.0001;
const FLASH_GLSL = /* glsl */ `
#if NUM_SPOT_LIGHTS > 0
  float s47g(float x, float w) { x /= w; return exp(-x * x); }
  // the flashlight's beam at a point (view space): p across the cone, 1 at its edge
  vec3 s47FlashCookie(const in SpotLight L, const in vec3 P) {
    vec3 d = normalize(P - L.position), A = -L.direction;
    // the light is held to the eye's right and below it, aimed where the eye looks: its up is the
    // view's up, so the picture turns with the head and not round the beam
    vec3 X = normalize(cross(A, vec3(0.0, 1.0, 0.0))), Y = cross(X, A);
    float t = sqrt(max(1.0 - L.coneCos * L.coneCos, 1e-6)) / L.coneCos;
    vec2 p = vec2(dot(d, X), dot(d, Y)) / (max(dot(d, A), 1e-3) * t);
    const float B = ${FLASH_BEAM.toFixed(3)};
    // the beam's own middle a little off the cone's, the core further, the filament with it
    vec2 q = p - vec2(0.012, -0.01);
    float r = length(q), th = atan(q.y, q.x);
    // the ragged edge: the dents of the stamped reflector round the rim, and a touch out of round
    float e = B * (1.0 + 0.012 * sin(2.0 * th + 0.7) + 0.009 * sin(7.0 * th + 2.1) + 0.007 * sin(13.0 * th + 4.4) + 0.005 * sin(29.0 * th + 1.3));
    // the flood of the reflector to its soft edge, a touch further for red and less far for blue
    vec3 f = 0.55 * (1.0 - vec3(smoothstep(e * 0.848, e * 1.048, r), smoothstep(e * 0.84, e * 1.04, r), smoothstep(e * 0.829, e * 1.029, r)));
    // the stamped reflector's flaws: two faint bright arcs in the outer half, broken round about
    // and a touch out of round (not even circles: those read as a target on smooth glass)
    float re = length(q * vec2(1.035, 0.965));
    float a1 = 0.3 + 0.7 * smoothstep(-0.2, 0.7, 0.6 * sin(3.0 * th + 0.9) + 0.4 * sin(7.0 * th + 2.6));
    float a2 = 0.3 + 0.7 * smoothstep(-0.2, 0.7, 0.6 * sin(4.0 * th + 4.1) + 0.4 * sin(9.0 * th + 0.3));
    float ring = 0.055 * (a1 * s47g(re - 0.6 * B, 0.035 * B) + 0.8 * a2 * s47g(re - 0.79 * B, 0.03 * B));
    // the hot core, where most of the light is, with the filament's soft dark spot in it (a coil:
    // wider than high), and the beam a little uneven round about
    vec2 c = p - vec2(0.045, -0.03), fl = (p - vec2(0.034, -0.02)) / vec2(0.12 * B, 0.075 * B);
    float k = (0.5 * s47g(length(c), 0.3 * B) + ring) * (1.0 + 0.03 * sin(5.0 * th + 1.9) * smoothstep(0.15 * B, 0.5 * B, r));
    vec3 v = (f + k) * (1.0 - 0.3 * exp(-dot(fl, fl)));
    // the corona of spill round the beam, out to the cone's edge
    v += 0.055 * (1.0 + 0.2 * sin(3.0 * th + 1.1)) * smoothstep(0.5 * B, e, r) * (1.0 - smoothstep(0.8, 0.98, length(p)));
    // a touch warmer than the bulb's own colour
    return v * vec3(1.0, 0.98, 0.93);
  }
#endif
`;
const HOOK = 'getSpotLightInfo( spotLight, geometryPosition, directLight );';
// before anything is compiled (this module is loaded with core/Flashlight.ts, before the first frame)
if (!THREE.ShaderChunk.lights_fragment_begin.includes(HOOK)) console.warn('cookies: three.js has moved the spot lights\' line; the flashlight is a plain disc');
else if (!THREE.ShaderChunk.lights_pars_begin.includes('s47FlashCookie')) {
  THREE.ShaderChunk.lights_pars_begin += FLASH_GLSL;
  THREE.ShaderChunk.lights_fragment_begin = THREE.ShaderChunk.lights_fragment_begin.replace(HOOK,
    `${HOOK}\n\t\tif ( directLight.visible && spotLight.decay == ${FLASH_DECAY.toFixed(1)} ) directLight.color *= s47FlashCookie( spotLight, geometryPosition );`);
}

// ---------- the windows ----------
/** The kinds of window (one cell of the atlas each), and the soft glow of a whole lit window on
 *  the ground at the foot of its wall ('glow': u across, v out from the wall). */
export type WindowStyle = 'panes' | 'shop' | 'bars' | 'blinds' | 'curtain' | 'drawn' | 'tv' | 'band' | 'glow';
const STYLES: WindowStyle[] = ['panes', 'shop', 'bars', 'blinds', 'curtain', 'drawn', 'tv', 'band', 'glow'];
// (a wide margin: the light is laid a little past the sill, into it, where the lamp's size blurs
// the sill's edge; world/windowLight.ts)
const CELL = 128, COLS = 3, ROWS = 3, MARGIN = 16;
/** A window's picture in the atlas: how many pixels across it is (and up). */
export const WINDOW_PX = CELL - 2 * MARGIN;
/** Where a point of a window (u across, v up from the sill, 0..1) is in the atlas. */
export function windowUv(style: WindowStyle, u: number, v: number): [number, number] {
  const i = STYLES.indexOf(style), cx = (i % COLS) * CELL, cy = Math.floor(i / COLS) * CELL, s = WINDOW_PX;
  // (the canvas's top row is the texture's top: the sill at the bottom of the cell)
  return [(cx + MARGIN + u * s) / (CELL * COLS), 1 - (cy + MARGIN + (1 - v) * s) / (CELL * ROWS)];
}
let atlas: THREE.CanvasTexture | null = null;
/** What each kind of window lets through, seen from its room's lamp: 1 in the open glass. The
 *  bars are a little wider than the real ones (the art director, 19 October: the mullions have to
 *  live through the first metre on the ground, where the lamp's size starts to blur them). */
export function windowCookies() {
  if (atlas) return atlas;
  const r = rng(47);
  const grime = Array.from({ length: 6 }, () => ({ a: r() * 9 + 3, b: r() * 9 + 3, p: r() * 6.28 }));
  // the glass's dust and smears: a little uneven
  const dirt = (u: number, v: number) => { let n = 0; for (const g of grime) n += Math.sin(u * g.a + v * g.b + g.p); return 0.93 + 0.012 * n; };
  // open between a frame of width f round it, softly: the jambs' edges soft (the wall's depth
  // round the glass), the sill's softer still (a deep sill, and the lamp is not a point); the
  // light on the ground ends nowhere as a hard line
  const frame = (u: number, v: number, f = 0.035, s = 0.025) => smooth(f * 0.5, f + s * 2.5, u) * smooth(f * 0.5, f + s * 2.5, 1 - u) * smooth(0, f + s * 5, v) * smooth(f, f + s * 2, 1 - v);
  // a dark bar at x of width w (all in the window's part), soft by s; k how dark
  const bar = (x: number, at: number, w: number, s = 0.012, k = 1) => 1 - k * (1 - smooth(w / 2, w / 2 + s, Math.abs(x - at)));
  const folds = (u: number, n: number, seed: number) => 0.5 + 0.5 * Math.sin(u * n * 6.283 + Math.sin(u * 7.3 + seed) * 1.6) * (0.7 + 0.3 * Math.sin(u * 23 + seed * 2));
  const cells: Record<WindowStyle, (u: number, v: number) => number> = {
    // four panes: a cross of a mullion and a transom
    panes: (u, v) => frame(u, v) * bar(u, 0.5, 0.05) * bar(v, 0.56, 0.045),
    // the gas station's office: a wide shop window in four lights over a rail, and the counter
    // with the till and the jars on it in the way of the lower part
    shop: (u, v) => {
      let k = frame(u, v, 0.02) * bar(u, 0.25, 0.02) * bar(u, 0.5, 0.026) * bar(u, 0.75, 0.02) * bar(v, 0.1, 0.04);
      const top = 0.3 + 0.07 * smooth(0.55, 0.6, u) * (1 - smooth(0.72, 0.77, u)) + 0.04 * smooth(0.2, 0.24, u) * (1 - smooth(0.33, 0.37, u));
      return k * (1 - 0.6 * (1 - smooth(top - 0.04, top + 0.04, v)));
    },
    // the sheriff's: two lights and six bars outside them, the desk low in the room
    bars: (u, v) => {
      let k = frame(u, v, 0.025) * bar(u, 0.5, 0.035);
      for (let i = 0; i < 6; i++) k *= bar(u, (0.3 + 0.6 * i) / 3.6, 0.02, 0.008, 0.85);
      return k * (1 - 0.35 * (1 - smooth(0.2, 0.3, v)) * smooth(0.25, 0.3, u) * (1 - smooth(0.7, 0.75, u)));
    },
    // the studio: one big light, the venetian blind let down over the top third with its slats
    // half open, and the console low in the room
    blinds: (u, v) => {
      let k = frame(u, v, 0.025);
      const slat = 0.5 + 0.5 * Math.cos((v / 0.075) * 6.283);
      k *= 1 - smooth(0.62, 0.66, v) * (0.75 - 0.6 * smooth(0.3, 0.9, slat));
      return k * (1 - 0.45 * (1 - smooth(0.24, 0.32, v)) * smooth(0.18, 0.24, u) * (1 - smooth(0.78, 0.84, u)));
    },
    // the kitchen: a sheer curtain drawn across, its folds
    curtain: (u, v) => frame(u, v, 0.03) * (0.38 + 0.4 * folds(u, 9, 1.3)) * (0.9 + 0.1 * v),
    // the motel's rooms: two lights and the curtains drawn, thick, light at the gap between them
    drawn: (u, v) => {
      const f = frame(u, v, 0.03) * bar(u, 0.5, 0.04);
      const gap = gauss(u - 0.47, 0.014) + 0.6 * gauss(u - 0.07, 0.016) + 0.6 * gauss(u - 0.93, 0.016);
      return f * Math.min(1, 0.22 + 0.22 * folds(u, 7, 0.4) + 0.75 * gap);
    },
    // a television's room: four panes, the light even and soft
    tv: (u, v) => frame(u, v, 0.05, 0.05) * bar(u, 0.5, 0.05, 0.03) * bar(v, 0.5, 0.045, 0.03),
    // SARO's control room: a band of six lights, a blind down in the first, the consoles' screens
    // standing in the way of the lowest part of three
    band: (u, v) => {
      let k = frame(u, v, 0.006, 0.006);
      for (let i = 1; i < 6; i++) k *= bar(u, i / 6, 0.016, 0.006);
      const blind = 0.5 + 0.5 * Math.cos((v / 0.075) * 6.283);
      k *= 1 - (1 - smooth(0.16, 0.165, u)) * (0.8 - 0.55 * smooth(0.4, 0.9, blind));
      for (const c of [0.36, 0.5, 0.64]) k *= 1 - 0.7 * (1 - smooth(0.035, 0.05, Math.abs(u - c))) * (1 - smooth(0.2, 0.26, v));
      return k;
    },
    // the whole lit window's soft light at the foot of its wall, wider than the glass, out to
    // about a metre and a half (u across, v out from the wall)
    glow: (u, v) => smooth(0, 0.3, u) * smooth(0, 0.3, 1 - u) * smooth(0, 0.06, v) * Math.pow(1 - Math.min(1, v), 1.7),
  };
  atlas = pixels(CELL * COLS, CELL * ROWS, (x, y) => {
    const i = Math.floor(x * COLS) + COLS * Math.floor(y * ROWS), s = WINDOW_PX;
    const u = ((x * COLS * CELL) % CELL - MARGIN) / s, v = 1 - ((y * ROWS * CELL) % CELL - MARGIN) / s;
    if (u < 0 || u > 1 || v < 0 || v > 1) return [0, 0, 0];
    const k = cells[STYLES[i]](u, v) * (STYLES[i] === 'glow' ? 1 : dirt(u, v));
    return [k, k, k];
  });
  atlas.name = 'cookie/windows';
  atlas.generateMipmaps = true;
  atlas.minFilter = THREE.LinearMipmapLinearFilter;
  return atlas;
}
