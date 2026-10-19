import * as THREE from 'three';
import { rng } from '../core/textures';
import { getQuality, type Quality } from '../core/quality';

// The painted horizon (ART_BRIEF.md, round 16; Tom, 19 October: "a painted backdrop of the
// horizon and the mesas, since the player never comes near them"). One band all the way round,
// on a ring in the sky group (Sky.ts): it goes where the camera goes and turns with the sky at
// STATION 01 (World.skyYaw), so it is never reached and never moves against the land, like the
// stars. It is drawn as far as anything can be (behind every mesh in the scene, the near mesas of
// horizon.ts included) and only over the sky dome, in one draw call: the night picture and the
// dawn picture in one shader, crossfaded with the sky's own dawn (uDawn), the haze of the area
// laid over its foot, and the warm glow of SARO and Caliche over the skyline where those places
// are from the camera (Sky.uGlow, set here from World.skyPlaces). The sky above the land is
// transparent: the stars, the Milky Way and the dawn's colours are the sky's.
//
// The pictures (runtime/horizon_night.webp, horizon_dawn.webp, 4096 x 128: rows 576 to 832 of the
// 8192 x 1024 originals in sky/ at half size) are 360 degrees wide, north at u = 0 and east at
// u = 0.25, and cover the elevations EL_BOT to EL_TOP at the same scale both ways. As they are on
// High (a phone), drawn up to 8192 on Ultra (a computer: a smoother skyline on a big screen) and
// down to 2048 on Low. Without them a simpler band is painted here in code (paintBand).

/** Elevations (degrees) at the top and the bottom of the band's pictures; the horizon is at 0. */
export const EL_TOP = 8.4375, EL_BOT = -2.8125;
/** How far out the ring is drawn (inside the sky dome, 3000 m); its depth is the far plane's. */
const R = 2500;
/** Places whose lights glow on the sky (Sky.uGlow). */
export const GLOWS = 3;
/** A lit place on the map: world position, how far across its lights are, how bright (World.skyPlaces). */
export interface SkyPlace { x: number; z: number; r: number; k: number }

const VS = /* glsl */`
  varying vec3 vDir; varying float vU;
  void main() {
    vDir = position; vU = uv.x;
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    // as far as anything can be: everything in the scene hides it, and it hides only the sky
    gl_Position.z = gl_Position.w * 0.999999;
  }`;
const FS = /* glsl */`
  uniform sampler2D uNight, uDay; uniform float uDawn, uGain, uRim, uFootK, uTopK;
  uniform vec3 uFoot, uSunDir; uniform vec4 uGlow[${GLOWS}];
  varying vec3 vDir; varying float vU;
  vec3 toOut(vec3 c) {
    #if defined( TONE_MAPPING )
      c = toneMapping(c);
    #endif
    return linearToOutputTexel(vec4(c, 1.0)).rgb;
  }
  void main() {
    vec3 d = normalize(vDir);
    float el = degrees(asin(clamp(d.y, -1.0, 1.0)));
    float v = (el - ${EL_BOT.toFixed(4)}) / ${(EL_TOP - EL_BOT).toFixed(4)};
    vec2 uv = vec2(vU, clamp(v, 0.0, 1.0));
    // the pictures carry their alpha premultiplied (the same mask in both)
    vec4 c = mix(texture2D(uNight, uv), texture2D(uDay, uv), smoothstep(0.05, 0.75, uDawn));
    float a = v < 0.0 ? 1.0 : c.a;
    vec3 col = c.rgb / max(c.a, 1e-4) * uGain;
    // the haze at its foot: the colour of the area's own far land (its fog), towards the sun on the
    // old road at dawn (OldRoad.ts rimHaze), all of it under the pictures, less up the cliffs
    vec3 h = normalize(vec3(d.x, 0.0, d.z) + vec3(1e-5, 0.0, 0.0));
    float east = 0.5 + 0.5 * dot(h, normalize(vec3(uSunDir.x, 0.0, uSunDir.z) + vec3(1e-5, 0.0, 0.0)));
    vec3 rim = mix(vec3(0.20, 0.23, 0.30), vec3(0.62, 0.42, 0.26), pow(east, 3.0)) + vec3(0.55, 0.30, 0.12) * pow(east, 8.0) * 0.8;
    vec3 foot = mix(uFoot, mix(uFoot, rim * 0.92, smoothstep(0.0, 1.0, uDawn)), uRim);
    float haze = el < 0.0 ? mix(uFootK, 1.0, smoothstep(0.0, -0.6, el)) : mix(uFootK, uTopK, smoothstep(0.0, 2.5, el));
    col = mix(col, foot, haze);
    // the glow of the places' lights in the air in front of the far land (the sky's own is in Sky.ts)
    float g = 0.0;
    for (int i = 0; i < ${GLOWS}; i++) {
      vec4 G = uGlow[i];
      float t = acos(clamp(dot(h.xz, G.xy), -1.0, 1.0)) / G.w;
      g += G.z * exp(-t * t) * exp(-max(el, 0.0) / 2.2);
    }
    col += vec3(0.30, 0.15, 0.06) * g * 0.12;
    gl_FragColor = vec4(toOut(col) * a, a);
  }`;

export class Backdrop {
  readonly mesh: THREE.Mesh;
  readonly uniforms: {
    uNight: { value: THREE.Texture | null }; uDay: { value: THREE.Texture | null }; uDawn: { value: number }; uSunDir: { value: THREE.Vector3 };
    uGlow: { value: THREE.Vector4[] }; uGain: { value: number }; uRim: { value: number }; uFootK: { value: number }; uTopK: { value: number }; uFoot: { value: THREE.Color };
  };
  /** Tests: keep it hidden (the same picture without it). */
  hold = false;
  private q: Quality | null = null;
  private seam = { at: 0, around: 0 };
  private width = 0;
  constructor(
    sky: { group: THREE.Group; uniforms: { uDawn: { value: number }; uSunDir: { value: THREE.Vector3 }; uGlow: { value: THREE.Vector4[] } } },
    /** The painted pictures (night, dawn), or null for the band painted in code. */
    private readonly art: { night: CanvasImageSource & { width: number; height: number }; dawn: CanvasImageSource & { width: number; height: number } } | null,
  ) {
    this.uniforms = {
      uNight: { value: null }, uDay: { value: null }, uDawn: sky.uniforms.uDawn, uSunDir: sky.uniforms.uSunDir, uGlow: sky.uniforms.uGlow,
      uGain: { value: 1 }, uRim: { value: 0 }, uFootK: { value: 0.5 }, uTopK: { value: 0.06 }, uFoot: { value: new THREE.Color() },
    };
    const mat = new THREE.ShaderMaterial({
      uniforms: this.uniforms, vertexShader: VS, fragmentShader: FS,
      transparent: true, premultipliedAlpha: true, depthWrite: false, fog: false, side: THREE.BackSide,
    });
    this.mesh = new THREE.Mesh(ring(), mat);
    this.mesh.name = 'horizon:band';
    // after the stars (-9), the comet and the shooting stars, which it hides behind the land;
    // before everything else that is see-through
    this.mesh.renderOrder = -6;
    this.mesh.frustumCulled = false;
    this.mesh.userData.noAO = true;       // no surface for the occlusion pass (core/vhs.ts)
    this.mesh.raycast = () => {};
    sky.group.add(this.mesh);
    this.build(getQuality());
  }

  /** The pictures at the width the graphics level takes: 8192 on Ultra (a computer), 4096 on High
   *  (a phone), 2048 on Low. */
  private build(q: Quality) {
    this.q = q;
    const w = q === 'low' ? 2048 : q === 'high' ? 4096 : 8192, h = w / 32;
    if (w === this.width) return;
    this.width = w;
    const old = [this.uniforms.uNight.value, this.uniforms.uDay.value];
    const night = this.canvas(w, h, 'night'), dawn = this.canvas(w, h, 'dawn');
    this.seam = seamOf(night);
    this.uniforms.uNight.value = texture(night, 'backdrop/night');
    this.uniforms.uDay.value = texture(dawn, 'backdrop/dawn');
    for (const t of old) t?.dispose();
  }
  private canvas(w: number, h: number, which: 'night' | 'dawn') {
    if (!this.art) return paintBand(w, h, which === 'dawn');
    const c = document.createElement('canvas'); c.width = w; c.height = h;
    const g = c.getContext('2d', { willReadFrequently: true })!;
    g.imageSmoothingQuality = 'high';
    g.drawImage(this.art[which], 0, 0, w, h);
    return c;
  }

  /** Every frame: the area's haze at the foot (fog: the colour of its far land; rim: the old road's
   *  dawn haze towards the sun), and the places that glow, in world terms, seen from cam; yaw is
   *  how the sky is turned in this area (World.skyYaw). */
  update(fog: THREE.Color, dawnHaze: number, rim: boolean, places: SkyPlace[], cam: THREE.Vector3, yaw: number) {
    if (getQuality() !== this.q) this.build(getQuality());
    this.mesh.visible = !this.hold;
    const u = this.uniforms, dawn = u.uDawn.value;
    u.uFoot.value.copy(fog);
    u.uRim.value = rim ? 1 : 0;
    // as the far mesas of horizon.ts had it: dark against the night sky, a little of the haze on their
    // feet, more of it as the day comes (dawnHaze, main.ts; the near mesas of horizon.ts take less)
    u.uFootK.value = 0.3 + 0.45 * dawnHaze;
    u.uTopK.value = 0.18 * dawnHaze;
    // the glows: brightest a few hundred metres out, gone right inside the place (its own lamps
    // are all round then) and far off; wider the nearer it is; the night's only
    const night = 1 - THREE.MathUtils.smoothstep(dawn, 0.15, 0.6), c = Math.cos(-yaw), s = Math.sin(-yaw);
    for (let i = 0; i < GLOWS; i++) {
      const G = u.uGlow.value[i], P = places[i];
      if (!P || night <= 0) { G.set(1, 0, 0, 1); continue; }
      const dx = P.x - cam.x, dz = P.z - cam.z, dist = Math.hypot(dx, dz) || 1;
      // into the sky group's own terms (it is turned by yaw)
      const lx = (dx * c + dz * s) / dist, lz = (-dx * s + dz * c) / dist;
      const k = P.k * night * THREE.MathUtils.smoothstep(dist, P.r * 0.6, P.r * 2.2) / (1 + (dist / 2600) ** 2);
      G.set(lx, lz, k, Math.max(0.06, Math.atan(P.r / dist) * 1.3));
    }
  }

  /** What the band is drawn from (tests): its width, painted or in code, and how well its ends meet. */
  info() {
    return { width: this.width, art: !!this.art, source: this.art ? 'painted (round 16)' : 'painted in code', seam: this.seam.at, seamAround: this.seam.around };
  }
}

/** The ring: 256 columns round, the pictures' rows, and a skirt under them down to the nadir (the haze
 *  where the land of an area ends short of the horizon, as from above at the end of chapter six). */
function ring() {
  const N = 256, rows = [EL_TOP, 6, 3, 1.5, 0.5, 0, -0.5, -1.5, EL_BOT, -8, -25, -60, -89.5];
  const pos: number[] = [], uv: number[] = [], idx: number[] = [];
  for (const el of rows) {
    const e = el * Math.PI / 180;
    for (let i = 0; i <= N; i++) {
      const az = i / N * Math.PI * 2;
      pos.push(Math.sin(az) * Math.cos(e) * R, Math.sin(e) * R, -Math.cos(az) * Math.cos(e) * R);
      uv.push(i / N, 0);
    }
  }
  for (let r = 0; r < rows.length - 1; r++) for (let i = 0; i < N; i++) {
    const a = r * (N + 1) + i, b = a + N + 1;
    idx.push(a, a + 1, b, a + 1, b + 1, b);
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setIndex(idx);
  return g;
}

function texture(c: HTMLCanvasElement, name: string) {
  const t = new THREE.CanvasTexture(c);
  t.name = name;
  t.colorSpace = THREE.SRGBColorSpace;
  t.wrapS = THREE.RepeatWrapping;
  t.premultiplyAlpha = true;            // no dark or pale fringe along the skyline
  t.anisotropy = 4;
  return t;
}

/** How well the picture's two ends meet: the mean step across the join (last column to the
 *  first), against the mean step between neighbouring columns elsewhere (0 to 255). */
function seamOf(c: HTMLCanvasElement) {
  const w = c.width, h = c.height, px = c.getContext('2d', { willReadFrequently: true })!.getImageData(0, 0, w, h).data;
  const step = (a: number, b: number) => { let s = 0; for (let y = 0; y < h; y++) for (let k = 0; k < 4; k++) s += Math.abs(px[(y * w + a) * 4 + k] - px[(y * w + b) * 4 + k]); return s / (h * 4); };
  let around = 0;
  const xs = [97, 611, 1201, 1530, 1999].map((x) => Math.floor(x / 2048 * w));
  for (const x of xs) around += step(x, x + 1);
  return { at: step(w - 1, 0), around: around / xs.length };
}

// ---------- the band painted in code, when the pictures are not there ----------
// After the concept art of the night (concept/night_20261018/): flat high desert to the edge, a low
// far ridge all round, and mesas and buttes with caprock, cliff bands and talus, laid out as the
// order (ART_BRIEF.md, round 16) has them: low to the north and south where the highway runs, open
// in the east-north-east where the sun comes up, the highest wall in the west.
// [azimuth, half width, top (degrees), layer (1 near, 2 far)]
const MESAS: [number, number, number, number][] = [
  [26, 8, 1.9, 1], [44, 6, 2.4, 2], [58, 3.5, 1.5, 1],
  [108, 3, 1.3, 2], [130, 1.6, 4.0, 1], [146, 6, 2.2, 1], [158, 2.5, 1.1, 2],
  [212, 11, 2.0, 1], [214, 5, 2.9, 1], [238, 5, 2.1, 2],
  [258, 13, 3.6, 1], [264, 5, 4.6, 1], [284, 9, 3.2, 1], [300, 4, 2.5, 2], [276, 2.2, 5.0, 1],
  [322, 6, 1.6, 2], [337, 3.5, 1.1, 1],
];
function mesaAt(az: number, layer: number) {
  let h = 0;
  for (const [a, w, top, l] of MESAS) {
    if (l !== layer) continue;
    const d = Math.abs(((az - a + 540) % 360) - 180);
    if (d > w + top * 3 + 8) continue;
    const cliff = Math.max(0.15, top * 0.1), talus = w * 0.3 + top * 1.6;
    let y: number;
    if (d <= w) y = top * (0.985 + 0.015 * Math.sin(az * 2.3 + a));
    else if (d <= w + cliff) y = top * (1 - 0.4 * (d - w) / cliff) - (d - w > cliff * 0.5 ? top * 0.04 : 0);
    else y = top * 0.6 * Math.pow(Math.max(0, 1 - (d - w - cliff) / talus), 1.5);
    h = Math.max(h, y);
  }
  return h;
}
function ridgeAt(az: number) {
  const r = az * Math.PI / 180;
  let h = 0.55 + 0.3 * Math.sin(r * 3 + 1) + 0.22 * Math.sin(r * 7 + 2) + 0.12 * Math.sin(r * 17);
  // low where the highway runs north and south and where the sun comes up
  const low = (c: number, w: number) => 1 - 0.75 * Math.exp(-((((az - c + 540) % 360 - 180) / w) ** 2));
  h *= low(0, 12) * low(180, 12) * low(80, 14);
  return Math.max(0.12, h);
}
type RGB = [number, number, number];
const mixC = (a: RGB, b: RGB, k: number): RGB => [a[0] + (b[0] - a[0]) * k, a[1] + (b[1] - a[1]) * k, a[2] + (b[2] - a[2]) * k];
export function paintBand(w: number, h: number, dawn: boolean) {
  const c = document.createElement('canvas'); c.width = w; c.height = h;
  const g = c.getContext('2d', { willReadFrequently: true })!, img = g.createImageData(w, h), px = img.data, r = rng(16);
  const perDeg = h / (EL_TOP - EL_BOT);
  const noise = new Float32Array(w * h).map(() => r());
  for (let x = 0; x < w; x++) {
    const az = x / w * 360;
    const near = mesaAt(az, 1), far = mesaAt(az, 2), ridge = ridgeAt(az);
    // at dawn: backlit in the east (the sun at 79 degrees), the cliffs in the west catching it
    const e = Math.pow(0.5 + 0.5 * Math.cos((az - 79) * Math.PI / 180), 2);
    const P = dawn ? {
      plain: mixC([62, 58, 66], [30, 24, 30], e), near: mixC([74, 62, 72], [22, 15, 22], e), far: mixC([96, 86, 100], [38, 27, 37], e),
      ridge: mixC([112, 108, 126], [98, 64, 64], e), rim: mixC([150, 120, 128], [210, 128, 74], e),
    } : { plain: [13, 15, 22] as RGB, near: [8, 9, 14] as RGB, far: [13, 15, 23] as RGB, ridge: [21, 25, 37] as RGB, rim: [40, 48, 64] as RGB };
    for (let y = 0; y < h; y++) {
      const el = EL_TOP - (y + 0.5) / perDeg, i = (y * w + x) * 4, n = noise[y * w + x];
      let col: RGB | null = null, top = 0;
      if (el < 0) col = mixC(P.plain, [P.plain[0] * 0.75, P.plain[1] * 0.75, P.plain[2] * 0.75], Math.min(1, -el / 2.5));
      else if (el < near) { col = P.near; top = near; }
      else if (el < far) { col = P.far; top = far; }
      else if (el < ridge) { col = P.ridge; top = ridge; }
      if (!col) { px[i + 3] = 0; continue; }
      // a cool edge of starlight (warm against the dawn) along the top, grain and cliff bands below it
      if (top > 0 && top - el < 1.3 / perDeg) col = mixC(col, P.rim, 0.55);
      const band = top > 0 ? 0.9 + 0.1 * Math.sin(el * 9 + x * 0.002) : 1;
      const k = band * (0.92 + 0.16 * n);
      px[i] = col[0] * k; px[i + 1] = col[1] * k; px[i + 2] = col[2] * k; px[i + 3] = 255;
    }
  }
  // a few far ranch lights out on the plain at night, a pixel or two each
  if (!dawn) for (const az of [33, 97, 151, 203, 247, 318, 343]) {
    const x = Math.floor(az / 360 * w), y = Math.floor((EL_TOP + 0.25 + r() * 0.3) * perDeg);
    const i = (y * w + x) * 4;
    px[i] = 200; px[i + 1] = 150; px[i + 2] = 96;
  }
  g.putImageData(img, 0, 0);
  return c;
}
