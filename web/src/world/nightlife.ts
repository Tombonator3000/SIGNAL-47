import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { GlowPoints, glowScale } from './glow';
import { floodlit, type FloodSet } from './kit';
import { canvasTex, rng } from '../core/textures';
import { getQuality } from '../core/quality';

// Life in the night (Tom, 19 October: more realistic places, with more in them). The night is
// alive without being busy: moths round the sodium lamps and the porch lights, bats near the big
// lamps in the early night, dust in the flashlight's beam and in the headlights on the survey
// track, lights far out on the land (a car on a county road now and then, a ranch window that
// goes out, a kitchen that lights up before dawn), and now and then a tumbleweed across the
// highway or the lot in a gust. None of it is the story's: nothing comes near, nothing stops to
// look, nothing glows in the sky (core/glimpses.ts, story/nightEvents.ts and story/townEvents.ts
// have the night's strange things, and while one of them is on there is no car and no tumbleweed).
//
// Cheap on purpose, a phone runs High: all the moths and bats of an area are one point cloud that
// moves in its vertex shader (no work per frame but a few numbers), the dust is one cloud for the
// flashlight and one for the headlights, drawn only while the light is on, the far lights are one
// GlowPoints per area, and a tumbleweed is three crossed planes, drawn only while one rolls. Low
// draws a third of the moths, half the dust and no bats. Nothing here is a collider or a light,
// and nothing can be hit by a ray (the crosshair, the ground under the feet).
//
// Every area's lamps are read from its own glows (world/glow.ts), as airlight.ts reads its cones,
// so a lamp that goes out (Caliche's at dawn and in event 8) takes its moths with it, and the
// areas' own files need not know about any of this.

/** A lamp the insects find: where (in the moth cloud's terms), its colour, how lit it is now. */
export interface LifeLamp { p: THREE.Vector3; col: THREE.Color; k: () => number; big: boolean }
export type LifeArea = 'saro' | 'road' | 'station01';

/** The lamps among some groups' glows: warm or white, bright, the size of a lamp's own glow (not
 *  its halo, not an indicator, not a red beacon or a coloured sign), in `into`'s terms. */
export function lifeLamps(roots: THREE.Object3D[], into: THREE.Object3D, ok: (p: THREE.Vector3, size: number) => boolean): LifeLamp[] {
  const out: LifeLamp[] = [];
  into.updateWorldMatrix(true, false);
  const inv = new THREE.Matrix4().copy(into.matrixWorld).invert(), m = new THREE.Matrix4(), w = new THREE.Vector3();
  for (const root of roots) {
    root.updateWorldMatrix(true, true);
    root.traverse((o) => {
      const pts = o as THREE.Points;
      if (!pts.isPoints) return;
      const pos = pts.geometry.getAttribute('position'), col = pts.geometry.getAttribute('aCol'), size = pts.geometry.getAttribute('aSize');
      if (!pos || !col || !size) return;
      m.multiplyMatrices(inv, pts.matrixWorld);
      for (let i = 0; i < pos.count; i++) {
        const r = col.getX(i), g = col.getY(i), b = col.getZ(i), s = size.getX(i);
        if (r < 0.75 || g < 0.3 || b > g || s < 0.3 || s > 6) continue;
        const p = new THREE.Vector3(pos.getX(i), pos.getY(i), pos.getZ(i)).applyMatrix4(m);
        w.copy(p).applyMatrix4(into.matrixWorld);
        if (!ok(w, s) || out.some((L) => L.p.distanceToSquared(p) < 0.6)) continue;
        // lit as long as its glow is (Town.ts darkens a lamp's glow when it goes off)
        out.push({ p, col: new THREE.Color(r, g, b).multiplyScalar(1 / r), k: () => col.getX(i) / r, big: s >= 2 });
      }
    });
  }
  return out;
}

/** How much is about at this hour (clock: seconds after midnight; dawn 0..1 from the sky). The
 *  night cools from about 13 °C at midnight to 6 at dawn (core/ambience.ts has the crickets on the
 *  same curve): fewer moths as it cools. The bats hunt the lamps in the early night, are gone by
 *  half past three, and a few come back to them before dawn. */
export function activity(clock: number, dawn: number) {
  let h = (((clock % 86400) + 86400) % 86400) / 3600;
  if (h > 12) h -= 24;
  const temp = 16 - 1.2 * (h + 3), day = THREE.MathUtils.smoothstep(dawn, 0.15, 0.45);
  const moth = THREE.MathUtils.clamp((temp - 4.5) / 5, 0.15, 1) * (1 - day);
  const back = THREE.MathUtils.smoothstep(h, 4.4, 4.7) * (1 - THREE.MathUtils.smoothstep(h, 5.0, 5.3));
  const bat = Math.max(1 - THREE.MathUtils.smoothstep(h, 2.9, 3.6), 0.6 * back) * (1 - THREE.MathUtils.smoothstep(dawn, 0.2, 0.5));
  return { moth, bat, hour: h };
}

// ---------- moths and bats round an area's lamps ----------
const MOTH_VS = /* glsl */`
  attribute vec4 aSeed; attribute float aLamp; attribute float aKind; attribute vec3 aCol;
  uniform float uTime, uPx, uMoth, uBat;
  uniform float uK[LAMPS];
  varying vec3 vCol; varying float vA; varying float vKind; varying float vFlap;
  void main() {
    float k = uK[int(aLamp + 0.5)];
    vec3 c = position, p;
    float size, a, lit;
    if (aKind < 0.5) {
      // a moth: loops round the lamp and a little under it, never still, darting now and then
      float t = uTime * (0.7 + 0.6 * aSeed.x);
      float r = 0.18 + 0.75 * aSeed.y * (0.65 + 0.35 * sin(t * 0.9 + aSeed.z * 6.28));
      float a1 = t * (2.0 + aSeed.z * 1.8) + aSeed.w * 6.28;
      p = c + vec3(cos(a1) * r, -0.3 + 0.4 * sin(t * 1.7 + aSeed.x * 9.0) + 0.12 * sin(t * 5.3 + aSeed.y * 7.0), sin(a1 * 1.31 + aSeed.y * 3.0) * r);
      p += 0.07 * vec3(sin(t * 11.0 + aSeed.w * 30.0), sin(t * 13.0 + aSeed.z * 20.0), cos(t * 12.0 + aSeed.x * 25.0));
      float d = length(p - c);
      lit = 0.2 + 1.0 / (1.0 + d * d * 3.0);
      size = 0.05;
      // fewer as the night cools: each moth has its own threshold
      a = k * step(aSeed.x, uMoth);
      vFlap = 0.45 + 0.55 * abs(sin(uTime * (31.0 + 17.0 * aSeed.y) + aSeed.w * 6.28));
    } else {
      // a bat: wide quick loops through the lamp's light and out of it, hunting the moths;
      // each comes for a while and is off into the dark for a while
      float t = uTime * (0.85 + 0.3 * aSeed.x) + aSeed.w * 40.0;
      float R = 2.5 + 4.5 * (0.5 + 0.5 * sin(t * 0.23 + aSeed.y * 6.28));
      float a1 = t * (6.0 / R) * (aSeed.z < 0.5 ? 1.0 : -1.0);
      p = c + vec3(cos(a1) * R + 1.2 * sin(t * 2.3 + aSeed.y * 9.0), -1.4 + 1.3 * sin(t * 0.9 + aSeed.z * 5.0) + 0.4 * sin(t * 3.1), sin(a1 * 0.83 + aSeed.x * 4.0) * R);
      float d = length(p - c);
      lit = 1.0 / (1.0 + d * d * 0.07);
      size = 0.3;
      a = uBat * smoothstep(-0.3, 0.2, sin(t * 0.045 + aSeed.y * 6.28));
      vFlap = sin(uTime * (13.0 + 3.0 * aSeed.x) + aSeed.w * 6.28);
      lit *= 0.25 + 0.75 * k;
    }
    vec4 mv = modelViewMatrix * vec4(p, 1.0);
    gl_Position = projectionMatrix * mv;
    float dist = -mv.z;
    float px = size * uPx / max(dist, 0.1);
    // under a pixel: kept at one, fainter instead; gone well before they would only flicker
    gl_PointSize = clamp(px, 1.0, 64.0);
    vA = a * min(px, 1.0) * (1.0 - smoothstep(32.0, 55.0, dist)) * smoothstep(0.5, 1.5, dist);
    if (vA < 0.002) gl_Position = vec4(2.0, 2.0, 2.0, 1.0);   // (not flying now, or too far: no pixels)
    // (a moth close under the lamp is all but white in it)
    vCol = mix(aCol, vec3(1.0, 0.8, 0.55), 0.2 * (1.0 - aKind)) * lit;
    vKind = aKind;
  }`;
const MOTH_FS = /* glsl */`
  uniform float uGain;
  varying vec3 vCol; varying float vA; varying float vKind; varying float vFlap;
  void main() {
    vec2 q = gl_PointCoord - 0.5;
    float a; vec3 col;
    if (vKind < 0.5) {
      // a fleck of light, brighter as a wing turns to the lamp
      a = smoothstep(0.5, 0.12, length(q)) * vA;
      col = vCol * vFlap * uGain;
    } else {
      // a bat from below: a small body and two wings, their tips going up and down, the
      // trailing edges scalloped between the fingers; dark, warm where the lamp catches it
      q.y = -q.y;
      float x = abs(q.x) * 2.1;
      float top = vFlap * 0.28 * x + 0.05 - 0.03 * x;
      float bot = top - (0.15 * (1.0 - x) + 0.025) + 0.035 * abs(sin(x * 9.4));
      float wing = step(bot, q.y) * step(q.y, top) * step(x, 1.0);
      float body = step(length(q / vec2(0.075, 0.17)), 1.0);
      a = max(wing, body) * vA;
      col = vCol * vec3(0.1, 0.075, 0.055) + 0.0015;
    }
    if (a < 0.01) discard;
    gl_FragColor = vec4(col, a);
    #include <tonemapping_fragment>
    #include <colorspace_fragment>
  }`;

/** The moths (and bats) of one area: one point cloud, a child of the group the lamps are in. */
export class LampMoths {
  readonly points: THREE.Points;
  private readonly mat: THREE.ShaderMaterial;
  private readonly k: number[];
  /** Points in the buffer: the third of the moths Low draws first, then the rest, then the bats. */
  readonly n: { low: number; moths: number; bats: number };
  constructor(readonly lamps: LifeLamp[], parent: THREE.Object3D, bats: number, seed: number) {
    const r = rng(seed), L = Math.max(1, lamps.length);
    const pos: number[] = [], sd: number[] = [], li: number[] = [], kind: number[] = [], col: number[] = [];
    const put = (i: number, b: number) => {
      const p = lamps[i].p, c = lamps[i].col;
      pos.push(p.x, p.y - (b ? 0 : 0.12), p.z); sd.push(r(), r(), r(), r()); li.push(i); kind.push(b); col.push(c.r, c.g, c.b);
    };
    // a big lamp has about a dozen, a porch light half that, at the dead of night
    const per = lamps.map((x) => x.big ? 12 : 7);
    let low = 0, moths = 0;
    lamps.forEach((_, i) => { for (let j = 0; j < Math.ceil(per[i] / 3); j++) { put(i, 0); low++; } });
    lamps.forEach((_, i) => { for (let j = Math.ceil(per[i] / 3); j < per[i]; j++) put(i, 0); });
    moths = pos.length / 3;
    // the bats at the big lamps, spread along them
    const big = lamps.map((x, i) => x.big ? i : -1).filter((i) => i >= 0);
    const nb = big.length ? bats : 0;
    for (let b = 0; b < nb; b++) put(big[Math.floor((b + 0.5) / nb * big.length)], 1);
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setAttribute('aSeed', new THREE.Float32BufferAttribute(sd, 4));
    g.setAttribute('aLamp', new THREE.Float32BufferAttribute(li, 1));
    g.setAttribute('aKind', new THREE.Float32BufferAttribute(kind, 1));
    g.setAttribute('aCol', new THREE.Float32BufferAttribute(col, 3));
    this.n = { low, moths, bats: nb };
    this.k = new Array(L).fill(1);
    this.mat = new THREE.ShaderMaterial({
      uniforms: { uTime: { value: 0 }, uPx: glowScale, uMoth: { value: 1 }, uBat: { value: 0 }, uGain: { value: 2.4 }, uK: { value: this.k } },
      defines: { LAMPS: L }, vertexShader: MOTH_VS, fragmentShader: MOTH_FS,
      transparent: true, depthWrite: false, fog: false,
    });
    this.points = new THREE.Points(g, this.mat);
    this.points.name = 'life:moths';
    this.points.frustumCulled = false;
    this.points.renderOrder = 3;   // over the lamps' own glows (2): a bat crossing one darkens it
    this.points.raycast = () => {};
    this.points.visible = false;
    parent.add(this.points);
  }
  /** Points drawn now (moths, bats), for the tests. */
  drawn = { moths: 0, bats: 0 };
  /** Round all the lamps in world terms, 60 m out: farther off nothing of them shows (the shader
   *  fades them out by 55 m), so they are not drawn at all (Caliche from the survey track). */
  private reach: THREE.Box3 | null = null;
  update(t: number, on: boolean, low: boolean, act: { moth: number; bat: number }, cam: THREE.Vector3) {
    const P = this.points;
    if (!this.reach) {
      P.updateWorldMatrix(true, false);
      this.reach = new THREE.Box3().setFromPoints(this.lamps.map((L) => L.p.clone().applyMatrix4(P.matrixWorld))).expandByScalar(60);
    }
    P.visible = on && act.moth > 0.01 && this.lamps.length > 0 && this.reach.containsPoint(cam);
    if (!P.visible) { this.drawn = { moths: 0, bats: 0 }; return; }
    const bats = !low && act.bat > 0.01 ? this.n.bats : 0;
    const moths = low ? this.n.low : this.n.moths;
    // the bats are the last points: left out of the range when they are not flying
    P.geometry.setDrawRange(0, bats ? this.n.moths + bats : moths);
    this.drawn = { moths, bats };
    const u = this.mat.uniforms;
    u.uTime.value = t; u.uMoth.value = act.moth; u.uBat.value = act.bat;
    this.lamps.forEach((L, i) => { this.k[i] = THREE.MathUtils.clamp(L.k(), 0, 1); });
  }
}

// ---------- dust in a beam ----------
const MOTE_VS = /* glsl */`
  attribute vec4 aSeed;
  uniform float uTime, uPx, uSize, uDensity, uWobble, uFall, uPuff, uPuffSize, uPuffA, uCut;
  uniform vec3 uCenter, uBox, uDrift;   // (uDrift: how far the air has carried them, wrapped by the box)
  uniform vec3 uBeamPos[2]; uniform vec3 uBeamDir[2]; uniform float uBeamK[2]; uniform vec2 uCos;
  varying float vA; varying float vPuff;
  void main() {
    // each mote has its place in the world, drifting; the box round the beam (always the same
    // size) wraps them, so they stay where they are as the light moves through them
    vec3 P = position * uBox + uDrift
      + uWobble * vec3(sin(uTime * 0.7 + aSeed.x * 40.0), sin(uTime * 0.5 + aSeed.y * 30.0), cos(uTime * 0.6 + aSeed.z * 35.0));
    vec3 base = uCenter - 0.5 * uBox;
    vec3 l = mod(P - base, uBox);
    vec3 wp = base + l;
    vec3 e = min(l, uBox - l) / (0.16 * uBox);
    float edge = clamp(min(min(e.x, e.y), e.z), 0.0, 1.0);
    float light = 0.0;
    for (int i = 0; i < 2; i++) {
      vec3 v = wp - uBeamPos[i];
      float L = length(v);
      // (headlights on low beam: nothing much above uCut over the lamps)
      light += uBeamK[i] * smoothstep(uCos.x, uCos.y, dot(v, uBeamDir[i]) / max(L, 1e-3)) / (1.0 + L * L * uFall) * (1.0 - smoothstep(0.0, uCut, v.y));
    }
    vec4 mv = viewMatrix * vec4(wp, 1.0);
    gl_Position = projectionMatrix * mv;
    float dist = -mv.z;
    // a few of them (when there is dust about) are soft puffs of it a metre or more across
    float puff = step(uPuff, aSeed.w);
    float px = mix(uSize * (0.6 + 0.8 * aSeed.w), uPuffSize * (0.6 + 0.8 * fract(aSeed.w * 7.0)), puff) * uPx / max(dist, 0.05);
    gl_PointSize = clamp(px, 1.0, mix(16.0, 64.0, puff));
    vPuff = puff;
    // brightest looking down the beam (the light scatters forward), glinting as each one turns
    float fwd = 0.3 + 0.7 * pow(max(dot(normalize(wp - cameraPosition), uBeamDir[0]), 0.0), 3.0);
    float glint = 0.55 + 0.45 * sin(uTime * (2.0 + 5.0 * aSeed.y) + aSeed.z * 6.28);
    vA = step(aSeed.x, uDensity) * edge * light * fwd * mix(glint, uPuffA, puff) * min(px, 1.0) * smoothstep(mix(0.3, 2.0, puff), mix(0.7, 5.0, puff), dist);
    // out of the light: not drawn at all (outside the view, so it costs no pixels)
    if (vA < 0.002) gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
  }`;
const MOTE_FS = /* glsl */`
  uniform vec3 uCol;
  varying float vA; varying float vPuff;
  void main() {
    float d = length(gl_PointCoord - 0.5), s = smoothstep(0.5, 0.0, d);
    float a = mix(smoothstep(0.5, 0.1, d), s * s, vPuff) * vA;
    if (a < 0.004) discard;
    gl_FragColor = vec4(uCol, a);
    #include <tonemapping_fragment>
    #include <colorspace_fragment>
  }`;

/** Dust (or a few insects) where up to two spot beams cross a box that follows them. */
export class Motes {
  readonly points: THREE.Points;
  readonly mat: THREE.ShaderMaterial;
  readonly n: number;
  /** How far the air has carried the motes (world metres, kept within the box). */
  private readonly carried = new THREE.Vector3();
  constructor(scene: THREE.Scene, n: number, name: string, seed: number, box: THREE.Vector3, bottom = false) {
    const r = rng(seed), pos = new Float32Array(n * 3), sd = new Float32Array(n * 4);
    for (let i = 0; i < n; i++) {
      // (the headlights' dust lies thicker near the ground)
      pos.set([r(), bottom ? r() * r() : r(), r()], i * 3);
      sd.set([r(), r(), r(), r()], i * 4);
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('aSeed', new THREE.BufferAttribute(sd, 4));
    this.n = n;
    this.mat = new THREE.ShaderMaterial({
      uniforms: {
        uTime: { value: 0 }, uPx: glowScale, uSize: { value: 0.01 }, uDensity: { value: 1 }, uWobble: { value: 0.05 }, uFall: { value: 0.1 },
        uPuff: { value: 2 }, uPuffSize: { value: 1.4 }, uPuffA: { value: 0.05 }, uCut: { value: 1000 },
        uCenter: { value: new THREE.Vector3() }, uBox: { value: box.clone() }, uDrift: { value: new THREE.Vector3() },
        uBeamPos: { value: [new THREE.Vector3(), new THREE.Vector3()] }, uBeamDir: { value: [new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 0, -1)] },
        uBeamK: { value: [0, 0] }, uCos: { value: new THREE.Vector2(0.92, 0.98) }, uCol: { value: new THREE.Color(1, 0.9, 0.75) },
      },
      vertexShader: MOTE_VS, fragmentShader: MOTE_FS,
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false,
    });
    this.points = new THREE.Points(g, this.mat);
    this.points.name = name;
    this.points.frustumCulled = false;
    this.points.matrixAutoUpdate = false;
    this.points.renderOrder = 3;
    this.points.raycast = () => {};
    this.points.visible = false;
    scene.add(this.points);
  }
  get u() { return this.mat.uniforms; }
  /** The air carries them on at this speed (m/s): changing it changes how they move, not where they are. */
  drift(dt: number, v: THREE.Vector3) {
    const b = this.u.uBox.value as THREE.Vector3, c = this.carried.addScaledVector(v, dt);
    c.set(((c.x % b.x) + b.x) % b.x, ((c.y % b.y) + b.y) % b.y, ((c.z % b.z) + b.z) % b.z);
    (this.u.uDrift.value as THREE.Vector3).copy(c);
  }
}

// ---------- far lights out on the land ----------
// In the road's terms (the map, geo.ts): SARO sees them less (24, -300), the road area plus its
// origin. A county road runs north and south 1.4 km east of the highway, past a ranch; a car comes
// along it now and then, its lights crawling across the dark, and goes on. Ranch and trailer
// windows: one goes out in the small hours, one comes on before dawn (a rancher up early), one is
// lit for a while at half past three. Each area shows only those over its own drawn land.
const COUNTY = (z: number) => 1450 + 70 * Math.sin(z / 900 + 0.4);
const WINDOWS: { x: number; z: number; y: number; size: number; col: number; on: [number, number] }[] = [
  { x: 1478, z: 1652, y: 1.4, size: 3.2, col: 0xffc27a, on: [4.58, 9] },      // the ranch's kitchen, from 04:35
  { x: 1492, z: 1700, y: 5.5, size: 4.5, col: 0xbcd4ff, on: [-6, 9] },        // its yard light (mercury), all night
  { x: -1650, z: 1480, y: 1.4, size: 3, col: 0xffb868, on: [-6, 2.87] },      // a house west of Caliche, out at 02:52
  { x: 1420, z: -820, y: 1.4, size: 3, col: 0xffc888, on: [3.33, 3.75] },     // north-east of SARO, lit 03:20 to 03:45
  { x: -1300, z: 250, y: 1.4, size: 2.8, col: 0xffb060, on: [-6, 3.08] },     // a trailer west of SARO, out at 03:05
];
const HEAD = new THREE.Color(1, 0.9, 0.72), TAIL = new THREE.Color(1, 0.05, 0.03), POOL = new THREE.Color(0.11, 0.085, 0.05);

export class FarLights {
  readonly glow = new GlowPoints();
  private readonly win: { i: number; w: typeof WINDOWS[number]; lit: number }[] = [];
  private readonly car: number[];
  constructor(scene: THREE.Scene, private readonly toWorld: (x: number, z: number) => THREE.Vector3, ground: (x: number, z: number) => number, centre: { x: number; z: number }) {
    for (const w of WINDOWS) {
      if (Math.hypot(w.x - centre.x, w.z - centre.z) > 1800) continue;
      const p = toWorld(w.x, w.z);
      this.win.push({ i: this.glow.add(p.x, ground(w.x, w.z) + w.y, p.z, w.size, w.col), w, lit: 1 });
    }
    this.car = [this.glow.add(0, -999, 0, 8, 0), this.glow.add(0, -999, 0, 5, 0), this.glow.add(0, -999, 0, 46, 0)];
    this.ground = ground;
    scene.add(this.glow.build());
    this.glow.points.name = 'life:far';
    this.glow.points.raycast = () => {};
    this.glow.points.visible = false;
  }
  private readonly ground: (x: number, z: number) => number;
  private readonly _p = new THREE.Vector3();
  /** The car's lamps are put away (no car on the road). */
  private gone = false;
  private set(i: number, c: THREE.Color, k: number) {
    const a = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    a.setXYZ(i, c.r * k, c.g * k, c.b * k); a.needsUpdate = true;
  }
  /** car: where on the county road (map z, heading +1 south or -1 north, how visible), or null. */
  update(t: number, on: boolean, hour: number, night: number, cam: THREE.Vector3, car: { z: number; dir: number; k: number } | null) {
    const P = this.glow.points;
    P.visible = on && night > 0.01;
    if (!P.visible) return;
    this.glow.update(t);
    for (const W of this.win) {
      const want = (hour >= W.w.on[0] && hour < W.w.on[1] ? 1 : 0) * night;
      if (Math.abs(want - W.lit) < 0.004) continue;
      W.lit = want;
      this.set(W.i, _c.set(W.w.col), want);
    }
    if (!car) { if (!this.gone) for (const i of this.car) this.glow.setPosition(i, _off); this.gone = true; return; }
    this.gone = false;
    const z = car.z, x = COUNTY(z), dx = COUNTY(z + car.dir) - x, h = Math.hypot(dx, car.dir);
    const fx = dx / h, fz = car.dir / h, p = this.toWorld(x, z), y = this.ground(x, z);
    // the lamps as seen from here: headlights coming, tail lights going, a little of both side on
    const tx = cam.x - p.x, tz = cam.z - p.z, d = Math.hypot(tx, tz) || 1, face = (fx * tx + fz * tz) / d;
    const far = 1 - THREE.MathUtils.smoothstep(d, 1700, 2100);
    const [hi, ti, pi] = this.car;
    this.glow.setPosition(hi, this._p.set(p.x, y + 0.9, p.z));
    this.glow.setPosition(ti, this._p.set(p.x - fx * 4, y + 0.9, p.z - fz * 4));
    this.glow.setPosition(pi, this._p.set(p.x + fx * 30, y + 0.6, p.z + fz * 30));
    const k = car.k * far * night;
    this.set(hi, HEAD, k * (0.18 + 0.82 * THREE.MathUtils.smoothstep(face, -0.35, 0.9)));
    this.set(ti, TAIL, k * 0.7 * THREE.MathUtils.smoothstep(-face, -0.2, 0.9));
    this.set(pi, POOL, k);
  }
}
const _off = new THREE.Vector3(0, -999, 0), _c = new THREE.Color();

// ---------- a tumbleweed ----------
let weedTex: THREE.Texture | null = null;
/** A ball of dry twigs, drawn once: thin arcs that cross and leave gaps between them (it is seen
 *  through), a few spikes past the ragged edge, the stems darker towards the core. */
function tumbleTex() {
  if (weedTex) return weedTex;
  weedTex = canvasTex(128, 128, (g, w, h) => {
    const r = rng(47);
    g.clearRect(0, 0, w, h);
    g.lineCap = 'round';
    for (let i = 0; i < 70; i++) {
      // each arc within a radius of its own, so the outline is ragged and not a circle
      const a = r() * Math.PI * 2, d = 40 * Math.sqrt(r()), rad = 10 + r() * 30, lim = 48 + r() * 14;
      const cx = 64 + Math.cos(a) * d, cy = 64 + Math.sin(a) * d;
      const v = Math.floor(150 + 105 * Math.min(1, d / 40));
      g.save(); g.beginPath(); g.arc(64, 64, lim, 0, Math.PI * 2); g.clip();
      g.strokeStyle = `rgb(${v},${v - 6},${v - 16})`; g.lineWidth = 1 + r() * 1.1;
      g.beginPath(); g.arc(cx, cy, rad, r() * 6.28, r() * 6.28 + 0.5 + r() * 1.4); g.stroke();
      g.restore();
    }
    for (let i = 0; i < 26; i++) {
      const a = r() * Math.PI * 2, d0 = 18 + r() * 26, d1 = d0 + 10 + r() * 16;
      g.strokeStyle = 'rgb(232,226,212)'; g.lineWidth = 0.9 + r() * 0.7;
      g.beginPath(); g.moveTo(64 + Math.cos(a) * d0, 64 + Math.sin(a) * d0); g.lineTo(64 + Math.cos(a + 0.15) * d1, 64 + Math.sin(a + 0.15) * d1); g.stroke();
    }
  });
  return weedTex;
}

/** One tumbleweed: rolls along a path on the ground in a gust, hops now and then, and is gone. */
export class Tumbleweed {
  readonly mesh: THREE.Mesh;
  on = false;
  /** The ground under it (world x, z). */
  ground: (x: number, z: number) => number = () => 0;
  private readonly from = new THREE.Vector3();
  private readonly dir = new THREE.Vector3();
  private s = 0;
  private len = 0;
  private top = 5;
  private hop = { h: 0, phase: 0, next: 3 };
  /** How far it has turned over (for the tests). */
  spin = 0;
  private readonly axis = new THREE.Vector3();
  private readonly R = 0.36;
  /** Seconds it lies still against what stopped it, before it is gone (unseen). */
  private rest = 0;
  constructor(scene: THREE.Scene, set: FloodSet, name: string) {
    const R = this.R, planes: THREE.BufferGeometry[] = [];
    // three crossed planes, a little flattened, as a tumbleweed is
    for (const [rx, ry] of [[0, 0], [Math.PI / 2, 0], [0, Math.PI / 2]]) {
      const p = new THREE.PlaneGeometry(2 * R, 2 * R);
      p.rotateX(rx); p.rotateY(ry); p.scale(1.08, 0.86, 1);
      planes.push(p);
    }
    // see-through, so far off it is a pale tangle and not a solid ball (alpha-tested lace turns into
    // a disc in the smaller mipmaps); pale, as dry stems are in the lamps and the starlight
    const mat = floodlit(new THREE.MeshStandardMaterial({ map: tumbleTex(), transparent: true, depthWrite: false, alphaTest: 0.03, side: THREE.DoubleSide,
      color: 0xf2e4c6, emissive: 0x0d0b08, roughness: 1 }), 0.012, set);
    this.mesh = new THREE.Mesh(mergeGeometries(planes)!, mat);
    this.mesh.name = name;
    this.mesh.raycast = () => {};   // never in the way of the crosshair or the ground under the feet
    this.mesh.visible = false;
    scene.add(this.mesh);
  }
  get at() { return this.mesh.position; }
  /** Roll from a point along a direction (world, on the ground) for len metres, at up to top m/s. */
  start(from: THREE.Vector3, dir: THREE.Vector3, len: number, top: number) {
    this.from.copy(from); this.dir.copy(dir).setY(0).normalize();
    this.s = 0; this.len = len; this.top = top; this.rest = 0; this.on = true; this.spin = 0;
    this.axis.set(0, 1, 0).cross(this.dir).normalize();
    this.hop = { h: 0, phase: 0, next: 2 + Math.random() * 4 };
    this.mesh.quaternion.random();
    this.mesh.visible = true;
    this.place();
  }
  stop() { this.on = false; this.mesh.visible = false; }
  /** seen: whether the camera looks its way (it is not taken away while looked at). */
  update(dt: number, seen: boolean) {
    if (!this.on) return;
    if (this.s >= this.len) {
      // it has come to rest (against a fence, a wall, or the gust has died); gone once unseen
      this.rest += dt;
      if (this.rest > 25 && !seen) this.stop();
      return;
    }
    // the gust: up to speed, rolling, and dying away at the end of the way
    const u = this.s / this.len, v = this.top * Math.max(0.12, Math.pow(Math.sin(Math.PI * Math.min(0.999, u * 0.92 + 0.04)), 0.6));
    const ds = v * dt;
    this.s += ds;
    // a hop over a stone now and then
    if ((this.hop.next -= ds) <= 0) { this.hop = { h: 0.15 + Math.random() * 0.45, phase: 0, next: 2 + Math.random() * 6 }; }
    if (this.hop.h > 0) { this.hop.phase += ds / (0.9 + this.hop.h * 2.5); if (this.hop.phase >= 1) this.hop.h = 0; }
    this.mesh.quaternion.premultiply(_q.setFromAxisAngle(this.axis, ds / this.R));
    this.spin += ds / this.R;
    this.place();
  }
  private place() {
    const p = this.mesh.position.copy(this.from).addScaledVector(this.dir, this.s);
    // a wobble across the way, as the wind veers
    p.addScaledVector(this.axis, 0.6 * Math.sin(this.s * 0.21) * Math.min(1, this.s / 6));
    const hop = this.hop.h > 0 ? this.hop.h * Math.sin(Math.PI * this.hop.phase) : 0;
    p.y = this.ground(p.x, p.z) + this.R * 0.85 + hop;
  }
}
const _q = new THREE.Quaternion();

// ---------- the night's life, every frame ----------
/** The way the wind blows the tumbleweeds and drifts the dust out in the open, in every area's
 *  terms (they all lie north up): towards the north-north-east. */
const WIND = new THREE.Vector3(0.45, 0, -0.89).normalize();
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export interface LifeIn {
  area: string;
  cam: THREE.PerspectiveCamera;
  /** The game's clock (seconds after midnight) and the dawn (0..1). */
  clock: number;
  dawn: number;
  /** The camera is out in the open (not inside a building; the cab counts as out). */
  out: boolean;
  /** One of the night's strange things has it now (no car, no tumbleweed), or a cinematic. */
  quiet: boolean;
  /** On foot in play (a tumbleweed now and then). */
  foot: boolean;
  flash: THREE.SpotLight | null;
  /** The truck being driven here: its group, its speed (m/s), the ground under it, its beams. */
  truck: { group: THREE.Object3D; speed: number; kind: string; beam: number } | null;
}
interface AreaLife {
  moths: LampMoths | null;
  far: FarLights | null;
  weed: Tumbleweed | null;
  /** In the way of a tumbleweed (world x, z): a wall, a fence, a parked truck. */
  blocked: (x: number, z: number) => boolean;
  /** The road's terms of a world point here (the far car), or null for none. */
  toMap: ((x: number, z: number) => { x: number; z: number }) | null;
  /** Whether the highway runs under this world point (a tumbleweed across it in the headlights). */
  onHighway: (x: number, z: number) => boolean;
  /** Its lamps in world terms (a tumbleweed on foot comes where one of them lights it). */
  lampsW: THREE.Vector3[];
}

export class NightLife {
  /** Tests: the night without its life, to compare. */
  off = false;
  private readonly areas: Partial<Record<LifeArea, AreaLife>> = {};
  private readonly flash: Motes;
  private readonly head: Motes;
  private act = { moth: 1, bat: 0, hour: 2 };
  private dust = 0;
  private weedT = rnd(70, 160);
  private carT = rnd(30, 90);
  private car: { z: number; dir: number; end: number; speed: number } | null = null;
  private weedArea: LifeArea | null = null;
  private here: LifeArea | null = null;
  private last: LifeIn | null = null;
  private t = 0;
  private readonly _v = new THREE.Vector3();
  private readonly _f = new THREE.Vector3();

  constructor(private readonly scene: THREE.Scene) {
    // (the boxes hold the near part of a beam whichever way it points)
    this.flash = new Motes(scene, 900, 'life:flashDust', 61, new THREE.Vector3(5.2, 3.6, 5.2));
    this.head = new Motes(scene, 3000, 'life:headDust', 62, new THREE.Vector3(34, 4.6, 34), true);
  }

  /** An area's far lights (if it sees the county road), its tumbleweed, and what stands in a tumbleweed's way. */
  addArea(id: LifeArea, o: { flood: FloodSet; ground: (x: number, z: number) => number; blocked: (x: number, z: number) => boolean;
    onHighway?: (x: number, z: number) => boolean;
    far?: { toWorld: (x: number, z: number) => THREE.Vector3; toMap: (x: number, z: number) => { x: number; z: number }; ground: (x: number, z: number) => number; centre: { x: number; z: number } } }) {
    const far = o.far ? new FarLights(this.scene, o.far.toWorld, o.far.ground, o.far.centre) : null;
    const weed = new Tumbleweed(this.scene, o.flood, `life:tumbleweed:${id}`);
    weed.ground = o.ground;
    this.areas[id] = { moths: null, far, weed, blocked: o.blocked, toMap: o.far?.toMap ?? null, onHighway: o.onHighway ?? (() => false), lampsW: [] };
  }
  /** An area's moths and bats: its lamps (lifeLamps), in parent's terms. */
  addMoths(id: LifeArea, lamps: LifeLamp[], parent: THREE.Object3D, bats: number) {
    const A = this.areas[id];
    if (!A || A.moths || !lamps.length) return;
    A.moths = new LampMoths(lamps, parent, bats, id.length * 31 + lamps.length);
    parent.updateWorldMatrix(true, false);
    A.lampsW = lamps.filter((L) => L.big).map((L) => parent.localToWorld(L.p.clone()));
  }

  update(dt: number, t: number, o: LifeIn) {
    this.t = t; this.last = o;
    const low = getQuality() === 'low', on = !this.off;
    const here: LifeArea | null = o.area === 'saro' ? 'saro' : o.area === 'road' || o.area === 'diner' ? 'road' : o.area === 'station01' ? 'station01' : null;
    if (here !== this.here) { for (const A of Object.values(this.areas)) A.weed?.stop(); this.here = here; }
    this.act = activity(o.clock, o.dawn);
    const night = 1 - THREE.MathUtils.smoothstep(o.dawn, 0.12, 0.4);
    // the moths and bats of the area shown, out in the open
    for (const [id, A] of Object.entries(this.areas) as [LifeArea, AreaLife][]) A.moths?.update(t, on && id === here && o.out, low, this.act, o.cam.position);
    // the far lights and the car on the county road
    this.carStep(dt, o, on && night > 0.2);
    const cam = o.cam.position;
    for (const [id, A] of Object.entries(this.areas) as [LifeArea, AreaLife][]) {
      if (!A.far) continue;
      const car = this.car && A.toMap ? { z: this.car.z, dir: this.car.dir, k: this.carK() } : null;
      A.far.update(t, on && id === here, this.act.hour, night, cam, car);
    }
    this.weedStep(dt, o, on && night > 0.3, here);
    this.beams(dt, o, on, low);
  }

  // ---------- the car on the county road ----------
  private carStep(dt: number, o: LifeIn, can: boolean) {
    const A = this.here ? this.areas[this.here] : undefined;
    if (this.car) {
      this.car.z += this.car.dir * this.car.speed * dt;
      if ((this.car.end - this.car.z) * this.car.dir <= 0 || !can) this.car = null;
      return;
    }
    if (!can || o.quiet || !A?.toMap) return;
    if ((this.carT -= dt) > 0) return;
    this.carT = rnd(200, 480);
    this.carNow(false);
  }
  /** A car on the county road, coming past the camera's place from one end: from out of sight,
   *  or (near: the tests, the developer menu) already in view. */
  carNow(near = true) {
    const A = this.here ? this.areas[this.here] : undefined, c = this.last?.cam.position;
    if (!A?.toMap || !c) return false;
    const m = A.toMap(c.x, c.z), dir = Math.random() < 0.5 ? 1 : -1;
    this.car = { z: m.z - dir * (near ? 700 : 2200), dir, end: m.z + dir * 2200, speed: rnd(20, 25) };
    return true;
  }
  /** How much of the car shows: in and out of sight behind the swells of the land, faded at its ends. */
  private carK() {
    const c = this.car!, s = c.z * c.dir, n = Math.sin(s * 0.013) * Math.sin(s * 0.0071 + 1.3) + 0.4 * Math.sin(s * 0.031);
    return THREE.MathUtils.smoothstep(n, -0.55, -0.1) * Math.min(1, Math.abs(c.end - c.z) / 300);
  }

  // ---------- a tumbleweed now and then ----------
  private weedStep(dt: number, o: LifeIn, can: boolean, here: LifeArea | null) {
    const cam = o.cam.position;
    for (const [id, A] of Object.entries(this.areas) as [LifeArea, AreaLife][]) {
      const W = A.weed;
      if (!W?.on) continue;
      if (id !== here || this.off) { W.stop(); continue; }
      const d = this._v.subVectors(W.at, cam), seen = d.length() < 80 && d.normalize().dot(o.cam.getWorldDirection(this._f)) > 0.45;
      W.update(dt, seen);
      // the truck runs it down: it goes under
      const T = o.truck;
      if (T && W.on && Math.hypot(W.at.x - T.group.position.x, W.at.z - T.group.position.z) < 3.2) W.stop();
    }
    if (!can || o.quiet || !here || !o.out || (!o.foot && !o.truck)) return;
    if ((this.weedT -= dt) > 0) return;
    this.weedT = this.tumbleNow() ? rnd(180, 420) : rnd(25, 50);
  }
  /** A tumbleweed now, crossing in front of the camera (on foot) or the highway ahead of the
   *  truck, with the wind; false when there is no open ground for one here. On foot it comes only
   *  where it would be seen: under a lamp, or in the flashlight's beam (anywhere: the tests). */
  tumbleNow(anywhere = false) {
    const o = this.last, A = this.here ? this.areas[this.here] : undefined;
    if (!o || !A?.weed) return false;
    const cam = o.cam.position, f = o.cam.getWorldDirection(this._f).setY(0).normalize();
    const T = o.truck;
    for (let k = 0; k < 8; k++) {
      let q: THREE.Vector3, dir: THREE.Vector3, back: number, len: number;
      if (T) {
        // across the road ahead of the truck, so it is over before the truck comes to it
        const h = new THREE.Vector3(0, 0, -1).applyQuaternion(T.group.quaternion).setY(0).normalize();
        q = T.group.position.clone().addScaledVector(h, 30 + Math.max(0, T.speed) * 2.6 + k * 4);
        if (!A.onHighway(q.x, q.z) && T.kind === 'asphalt') continue;
        dir = new THREE.Vector3(-h.z, 0, h.x);
        if (dir.dot(WIND) < 0) dir.negate();
        dir.lerp(WIND, 0.25).normalize();
        back = 13; len = 34;
      } else {
        q = cam.clone().addScaledVector(f, rnd(10, 26)).addScaledVector(new THREE.Vector3(-f.z, 0, f.x), rnd(-7, 7));
        const lamp = A.lampsW.some((L) => Math.hypot(L.x - q.x, L.z - q.z) < 13);
        const torch = !!o.flash && o.flash.intensity > 1 && cam.distanceTo(q) < 15;
        if (!lamp && !torch && !anywhere) continue;
        dir = WIND.clone().applyAxisAngle(_up, rnd(-0.4, 0.4));
        back = 26; len = 55;
      }
      // what stands in its way: it comes from behind nothing, and stops against whatever it meets
      let b = 0, e = 0;
      while (b < back && !A.blocked(q.x - dir.x * (b + 1), q.z - dir.z * (b + 1))) b += 1;
      if (A.blocked(q.x, q.z) || b < back * 0.6) continue;
      while (e < len - back && !A.blocked(q.x + dir.x * (e + 1), q.z + dir.z * (e + 1))) e += 1;
      const from = q.clone().addScaledVector(dir, -b);
      A.weed.start(from, dir, b + e, rnd(4.5, 6.5));
      this.weedArea = this.here;
      return true;
    }
    return false;
  }

  // ---------- dust in the beams ----------
  private beams(dt: number, o: LifeIn, on: boolean, low: boolean) {
    const F = this.flash, u = F.u, L = o.flash, k = L ? L.intensity / 40 : 0;
    F.points.visible = on && k > 0.01;
    if (F.points.visible && L) {
      const cam = o.cam.position, f = o.cam.getWorldDirection(this._f);
      u.uTime.value = this.t;
      // a box round the near part of the beam, where the dust shows
      u.uCenter.value.copy(cam).addScaledVector(f, 2.4);
      u.uBeamPos.value[0].copy(L.position);
      u.uBeamDir.value[0].subVectors(L.target.position, L.position).normalize();
      u.uBeamK.value[0] = k; u.uBeamK.value[1] = 0;
      u.uCos.value.set(Math.cos(L.angle), Math.cos(L.angle * (1 - L.penumbra)));
      u.uFall.value = 0.12; u.uSize.value = 0.02;
      // out in the open a breeze carries it (and a few gnats); indoors it hangs in the air
      if (o.out) { F.drift(dt, _v.copy(WIND).multiplyScalar(0.35)); u.uWobble.value = 0.05; u.uDensity.value = 0.75; }
      else { F.drift(dt, _v.set(0.01, 0.006, -0.004)); u.uWobble.value = 0.09; u.uDensity.value = 1; }
      (u.uCol.value as THREE.Color).setRGB(1, 0.9, 0.75).multiplyScalar(1.6);
      F.points.geometry.setDrawRange(0, low ? F.n >> 1 : F.n);
    }
    // the truck's headlights: the dust it raises on gravel and dirt hangs in them, and lingers
    // when it slows (the cloud catches up); on the highway now and then a bug in the beam
    const T = o.truck, H = this.head, h = H.u;
    const loose = !!T && T.kind !== 'asphalt';
    const want = loose ? THREE.MathUtils.clamp(Math.abs(T!.speed) / 10, 0, 1) : 0;
    this.dust += (want - this.dust) * Math.min(1, dt * (want > this.dust ? 0.8 : 0.22));
    H.points.visible = on && !!T && T.beam > 0.01;
    if (H.points.visible && T) {
      const g = T.group, fw = this._v.set(0, 0, -1).applyQuaternion(g.quaternion);
      h.uTime.value = this.t;
      h.uCenter.value.copy(g.position).addScaledVector(fw, 15); h.uCenter.value.y = g.position.y + 1.9;
      const down = this._f.set(0, -0.05, -1).normalize().applyQuaternion(g.quaternion);
      [-0.62, 0.62].forEach((x, i) => {
        h.uBeamPos.value[i].set(x, 0.85, -2.45).applyMatrix4(g.matrixWorld);
        h.uBeamDir.value[i].copy(down);
        h.uBeamK.value[i] = T.beam;
      });
      h.uCos.value.set(Math.cos(0.42), Math.cos(0.2)); h.uCut.value = 1.3;
      h.uFall.value = 0.004;
      const d = this.dust;
      h.uDensity.value = Math.max(0.04, d);
      h.uSize.value = 0.03;
      h.uPuff.value = 1 - 0.14 * d; h.uPuffSize.value = 2.2; h.uPuffA.value = 0.12;
      h.uWobble.value = 0.12 + 0.2 * d;
      H.drift(dt, _v.copy(WIND).multiplyScalar(0.6).addScaledVector(fw, 0.9 * d).setY(0.12 * d));
      (h.uCol.value as THREE.Color).setRGB(1, 0.9, 0.72).multiplyScalar(1 - 0.3 * d);
      H.points.geometry.setDrawRange(0, low ? H.n >> 1 : H.n);
    }
  }

  // ---------- the tests and the developer menu ----------
  lampK(id: LifeArea) { return this.areas[id]?.moths?.lamps.map((L) => THREE.MathUtils.clamp(L.k(), 0, 1)) ?? []; }
  tumbleMesh() { return (this.weedArea && this.areas[this.weedArea]?.weed?.mesh) || this.areas.saro?.weed?.mesh || null; }
  stats() {
    const a = (id: LifeArea) => {
      const M = this.areas[id]?.moths;
      return { lamps: M?.lamps.length ?? 0, moths: M?.n.moths ?? 0, bats: M?.n.bats ?? 0, drawn: M?.drawn.moths ?? 0, batsDrawn: M?.drawn.bats ?? 0, visible: !!M?.points.visible };
    };
    const W = this.tumbleMesh(), A = this.weedArea ? this.areas[this.weedArea] : undefined, weed = A?.weed;
    const c = this.car, toMap = this.here ? this.areas[this.here]?.toMap : null, cam = this.last?.cam.position;
    let carDist = 0;
    if (c && toMap && cam) { const m = toMap(cam.x, cam.z); carDist = Math.hypot(COUNTY(c.z) - m.x, c.z - m.z); }
    return {
      saro: a('saro'), road: a('road'), station01: a('station01'), activity: this.act,
      motes: { flash: this.flash.points.visible, flashN: this.flash.points.geometry.drawRange.count, flashMax: this.flash.n, head: this.head.points.visible, dust: this.dust },
      far: { car: !!c, carAt: c ? [COUNTY(c.z), c.z] : [0, 0], carDist },
      tumble: { on: !!weed?.on, at: W ? [W.position.x, W.position.y, W.position.z] : [0, 0, 0], ground: W && weed ? weed.ground(W.position.x, W.position.z) : 0, spin: weed?.spin ?? 0 },
    };
  }
}
const _up = new THREE.Vector3(0, 1, 0), _v = new THREE.Vector3();
