import * as THREE from 'three';
import { windowCookies, windowUv, type WindowStyle } from '../core/cookies';
import type { FloodSet } from './kit';

// A lit window's light on the ground in front of it (Tom, 19 October, light cookies; the concept
// art of Caliche and the payphone): the panes, the bars, the blinds or the curtain laid out on
// the sidewalk the way the room's lamp throws them through the glass, near and sharp at the wall,
// longer, wider and softer further out, and off with the window.
//
// One mesh for all the windows of an area, one draw call, added to what is under it (additive,
// no depth written), on every tier. Each window is a little grid: its points on the glass seen
// from the room's lamp, carried on to where that ray meets the ground. Each point keeps where it
// was on the glass as (u q, v q, q), q how far past the glass the ray went, so the picture lies
// true on the ground between the points (a projection, not a stretch); the further out, the
// blurrier the bars (a lamp is not a point: the picture's coarser levels). How bright: the
// window's own lamp now (a flood slot, read every frame: a television flickers, the studio dims
// after four, the sheriff's goes out with him), the light falling off with the distance from the
// lamp, and held back where the area's other floods already light the ground (a street lamp's
// pool), so it shows where the street is dark, more when the lamps go out (event 8).

const VS = /* glsl */ `
attribute vec3 aUvq;
attribute vec3 aCol;
varying vec3 vUvq;
varying vec3 vCol;
#include <common>
#include <fog_pars_vertex>
void main() {
  vUvq = aUvq; vCol = aCol;
  vec4 mvPosition = modelViewMatrix * vec4(position, 1.0);
  // drawn a little nearer the eye than it lies: the ground under it is a mesh of its own, coarser
  // or finer than this one, and a few centimetres off it here and there (a decal's usual way)
  mvPosition.xyz *= 1.0 - min(0.12 / max(length(mvPosition.xyz), 0.5), 0.2);
  gl_Position = projectionMatrix * mvPosition;
  #include <fog_vertex>
}`;
const FS = /* glsl */ `
uniform sampler2D uMap;
uniform vec2 uMapSize;
varying vec3 vUvq;
varying vec3 vCol;
#include <common>
#include <fog_pars_fragment>
void main() {
  vec2 uv = vUvq.xy / vUvq.z;
  // the level the picture is read at: what the screen needs, or blurrier the further out the ray
  // went past the glass (the room's lamp is a tube or a shade, not a point: the shadows of the
  // bars soften with the distance)
  vec2 px = uv * uMapSize, dx = dFdx(px), dy = dFdy(px);
  float lod = max(0.5 * log2(max(max(dot(dx, dx), dot(dy, dy)), 1e-8)), 1.0 + 2.0 * log2(max(vUvq.z, 1.0)));
  vec3 c = textureLod(uMap, uv, lod).rgb * vCol;
  #ifdef USE_FOG
    // light added through the haze fades with it (the haze's own colour is not added twice)
    #ifdef FOG_EXP2
      c *= exp(-fogDensity * fogDensity * vFogDepth * vFogDepth);
    #else
      c *= 1.0 - smoothstep(fogNear, fogFar, vFogDepth);
    #endif
  #endif
  gl_FragColor = vec4(c, 1.0);
  #include <tonemapping_fragment>
  #include <colorspace_fragment>
}`;

export interface WindowSpec {
  /** For the levels (the panel or slot it belongs to; several windows may share one). */
  id: string;
  style: WindowStyle;
  /** The middle of the glass, the way out of it (level), and its width and height (local terms). */
  c: THREE.Vector3; out: THREE.Vector3; w: number; h: number;
  /** The room's lamp: how far behind the glass, how high over the floor, and how far to the right
   *  of the window's middle (seen from outside). */
  lamp: { depth: number; y: number; side?: number };
  floor: number;
  color: THREE.ColorRepresentation;
  /** How bright, before the lamp's own level. */
  k: number;
  /** The flood slot of the window's lamp (its strength is the window's level), or none (always on). */
  slot?: number;
  /** How far out the light is laid at most (metres from the glass). */
  reach?: number;
  /** The part of its picture this window shows, across (a long band of windows is laid as a few
   *  parts, each from the lamp behind it). */
  part?: [number, number];
  /** The ground's own colour under it (the light it adds is the light on that ground). */
  tint?: (x: number, z: number) => THREE.Color;
}
type Win = WindowSpec & { at: number; n: number; fall: number[]; level: number; w0: number; col: THREE.Color };

const GRID_U = 6, GRID_V = 5;
const _a = new THREE.Vector3(), _w = new THREE.Vector3(), _p = new THREE.Vector3(), _l = new THREE.Vector3(), _up = new THREE.Vector3(0, 1, 0);
const OFF = 1e-3;   // a lamp core/ultra.ts holds for a real light is a tiny negative: it still burns

/** Every area's window lights (the test turns them all off and on). */
export const windowLights: WindowLights[] = [];
export function setWindowCookies(on: boolean) { for (const w of windowLights) w.mesh.visible = on; }

export class WindowLights {
  readonly mesh: THREE.Mesh;
  private wins: Win[] = [];
  private pos: number[] = [];
  private uvq: number[] = [];
  private dark: number[] = [];
  private tints: number[] = [];
  private lit = '';
  private lastW: number[] = [];
  private readonly mat: THREE.ShaderMaterial;

  /** set: the area's floods (in world terms; the group the mesh is put in sits at `origin`);
   *  ground: the height to lay the light at (the mesh's terms); own: the set's slots that are
   *  lamps in the windows' rooms (not counted as light already on the ground; the truck's
   *  headlights never are); fall: the area's floods' fall-off (kit.floodlit's). */
  constructor(readonly name: string, private set: FloodSet, private ground: (x: number, z: number) => number, private origin = new THREE.Vector3(), private own: number[] = [], private fall = 0.03) {
    this.mat = new THREE.ShaderMaterial({
      uniforms: THREE.UniformsUtils.merge([THREE.UniformsLib.fog, { uMap: { value: null }, uMapSize: { value: new THREE.Vector2() } }]),
      vertexShader: VS, fragmentShader: FS, fog: true,
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide,
      polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
    });
    const map = windowCookies();
    this.mat.uniforms.uMap.value = map;
    this.mat.uniforms.uMapSize.value.set(map.image.width, map.image.height);
    this.mat.name = name + '/windowLight';
    this.mesh = new THREE.Mesh(new THREE.BufferGeometry(), this.mat);
    this.mesh.name = name + ':window light';
    this.mesh.userData.noMerge = true; this.mesh.userData.noShadow = true;
    this.mesh.renderOrder = 1;
    windowLights.push(this);
  }

  /** A window's light; call build() after the last. */
  add(s: WindowSpec) {
    const a = _a.copy(_up).cross(s.out).normalize();   // across, to the right seen from outside
    const lamp = new THREE.Vector3().copy(s.c).addScaledVector(s.out, -s.lamp.depth).addScaledVector(a, s.lamp.side ?? 0);
    lamp.y = s.floor + s.lamp.y;
    const reach = s.reach ?? 3.6;
    // how far up the glass the light still comes down within reach (at the window's middle)
    const hit = (u: number, v: number, o: THREE.Vector3) => {
      _w.copy(s.c).addScaledVector(a, (u - 0.5) * s.w); _w.y += (v - 0.5) * s.h;
      _l.subVectors(_w, lamp);
      if (_l.y > -1e-3) return -1;
      let t = 1;
      for (let k = 0; k < 4; k++) {
        const g = this.ground(lamp.x + _l.x * t, lamp.z + _l.z * t) + 0.02;
        t = (g - lamp.y) / _l.y;
      }
      o.copy(lamp).addScaledVector(_l, t);
      return t;
    };
    let v1 = 1;
    const out = (v: number) => { const t = hit(0.5, v, _p); return t < 0 ? Infinity : _p.clone().sub(s.c).dot(s.out); };
    if (out(1) > reach) { let lo = 0, hi = 1; for (let k = 0; k < 20; k++) { const m = (lo + hi) / 2; if (out(m) > reach) hi = m; else lo = m; } v1 = lo; }
    if (v1 < 0.08) return;
    const at = this.pos.length / 3, fall: number[] = [];
    // the light at the foot of the window, for the fall-off (the irradiance there is 1)
    let ref = 0;
    for (let j = 0; j <= GRID_V; j++) for (let i = 0; i <= GRID_U; i++) {
      const u = i / GRID_U, v = (j / GRID_V) * v1, t = hit(u, v, _p);
      this.pos.push(_p.x, _p.y, _p.z);
      const tc = s.tint?.(_p.x, _p.z);
      this.tints.push(tc?.r ?? 1, tc?.g ?? 1, tc?.b ?? 1);
      const [ua, va] = windowUv(s.style, s.part ? s.part[0] + u * (s.part[1] - s.part[0]) : u, v);
      this.uvq.push(ua * t, va * t, t);
      // irradiance from a lamp: the cosine on the ground over the distance squared; and the far
      // end, where the light is cut by the reach rather than the window's head, fades out
      const d = _p.distanceTo(lamp), e = (lamp.y - _p.y) / d / (d * d);
      if (j === 0 && i === GRID_U / 2) ref = e;
      fall.push(e * (v1 < 1 ? 1 - THREE.MathUtils.smoothstep(j / GRID_V, 0.35, 1) : 1));
    }
    for (let k = 0; k < fall.length; k++) fall[k] = Math.min(1.25, fall[k] / ref);
    const w: Win = { ...s, at, n: (GRID_U + 1) * (GRID_V + 1), fall, level: s.slot === undefined ? 1 : 0, w0: s.slot !== undefined ? Math.max(1e-3, this.set.pos[s.slot].w) : 1, col: new THREE.Color(s.color) };
    this.wins.push(w);
  }

  build() {
    const g = this.mesh.geometry, idx: number[] = [];
    for (const w of this.wins) for (let j = 0; j < GRID_V; j++) for (let i = 0; i < GRID_U; i++) {
      const a = w.at + j * (GRID_U + 1) + i, b = a + 1, c = a + GRID_U + 1, d = c + 1;
      idx.push(a, b, d, a, d, c);
    }
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('aUvq', new THREE.Float32BufferAttribute(this.uvq, 3));
    g.setAttribute('aCol', new THREE.Float32BufferAttribute(new Float32Array(this.pos.length), 3));
    g.setIndex(idx);
    g.computeBoundingSphere();
    this.dark = new Array(this.pos.length / 3).fill(1);
    this.update();
    return this.mesh;
  }

  /** How bright a window without a flood slot of its own is now (0 dark, 1 as made). */
  setLevel(id: string, k: number) { for (const w of this.wins) if (w.id === id && w.slot === undefined && w.level !== k) { w.level = k; this.again = true; } }
  private again = false;

  /** How many windows lay their light here. */
  get windows() { return this.wins.length; }
  /** The windows by id now, 0 (dark) to about 1 (as made): tools/cookies.py. */
  levels() { const o: Record<string, number> = {}; for (const w of this.wins) o[w.id] = Math.max(o[w.id] ?? 0, w.level); return o; }
  /** How much the street's own light holds the windows' back, on average (1 none): tools/cookies.py. */
  darkness() { return this.dark.reduce((a, b) => a + b, 0) / Math.max(1, this.dark.length); }

  /** Every frame the area is shown: the windows' lamps now, and the street's light where it changed. */
  update() {
    const set = this.set;
    let changed = this.again;
    this.again = false;
    for (const w of this.wins) {
      if (w.slot === undefined) continue;
      const p = set.pos[w.slot].w, lv = p > 0 ? p / w.w0 : p > -OFF && p < 0 ? w.level : 0;
      if (lv !== w.level) { w.level = lv; changed = true; }
    }
    // which of the area's other lamps burn: the light already on the ground where the windows' fall
    let sig = '';
    for (let i = 0; i < set.count; i++) if (!this.own.includes(i) && !set.skip.has(i)) sig += set.pos[i].w > 0 || (set.pos[i].w < 0 && set.pos[i].w > -OFF) ? '1' : '0';
    if (sig !== this.lit) { this.lit = sig; this.street(); changed = true; }
    if (!changed) return;
    const col = this.mesh.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    if (!col) return;
    const T = this.tints;
    for (const w of this.wins) for (let k = 0; k < w.n; k++) {
      const i = w.at + k, f = w.fall[k] * w.k * w.level * this.dark[i];
      col.setXYZ(i, w.col.r * f * T[i * 3], w.col.g * f * T[i * 3 + 1], w.col.b * f * T[i * 3 + 2]);
    }
    col.needsUpdate = true;
  }

  // the light of the area's own lamps on the ground at each point (kit.floodlit's pools without
  // the angle), and what is left for the window's: about two thirds under a street lamp
  private street() {
    const set = this.set, o = this.origin, n = this.pos.length / 3;
    for (let k = 0; k < n; k++) {
      const x = this.pos[k * 3] + o.x, y = this.pos[k * 3 + 1] + o.y, z = this.pos[k * 3 + 2] + o.z;
      let e = 0;
      for (let i = 0; i < set.count; i++) {
        if (this.own.includes(i) || set.skip.has(i)) continue;
        // (a lamp handed to a real light in Ultra, core/ultra.ts, burns as it did before)
        const p = set.pos[i], w = p.w > 0 ? (this.lastW[i] = p.w) : p.w < 0 && p.w > -OFF ? this.lastW[i] ?? 0 : 0;
        if (w <= 0) continue;
        const d = Math.hypot(p.x - x, p.y - y, p.z - z), win = Math.max(0, 1 - d / (w * 1.6 + 8));
        const c = set.col[i];
        e += w * win * win / (1 + d * d * this.fall) * (c.r * 0.3 + c.g * 0.59 + c.b * 0.11);
      }
      this.dark[k] = 1 / (1 + 3 * e * set.scale.value);
    }
  }

  dispose() {
    this.mesh.geometry.dispose(); this.mat.dispose();
    const i = windowLights.indexOf(this); if (i >= 0) windowLights.splice(i, 1);
  }
}
