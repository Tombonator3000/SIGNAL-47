import * as THREE from 'three';
import { windowCookies, windowUv, WINDOW_PX, type WindowStyle } from '../core/cookies';
import type { FloodSet } from './kit';

// A lit window's light on the ground in front of it (Tom, 19 October, light cookies; the concept
// art of Caliche and the payphone): the panes, the bars, the blinds or the curtain laid out on
// the sidewalk the way the room's lamp throws them through the glass, sharp near the wall,
// longer, wider and softer further out, fading across the curb, and off with the window. Under
// the window a faint, broad glow of the whole lit glass on the ground at the foot of the wall
// ties the light to its house. Always dimmer than the room seen through the glass (the art
// director, 19 October: light on the ground brighter than its room reads as a projector).
//
// One mesh for all the windows of an area, one draw call, added to what is under it (additive,
// no depth written), on every tier. Each window is a little grid: its points on the glass seen
// from the room's lamp, carried on to where that ray meets the ground, from a little under the
// sill (where the lamp's size blurs the sill's edge) to the head or the reach. Each point keeps
// where it was on the glass as (u q, v q, q), q how far past the glass the ray went (1 on the
// glass), so the picture lies true on the ground between the points (a projection, not a
// stretch). The blur is the lamp's size seen from the ground through the glass: size (q - 1) / q,
// none at the glass, a bare bulb sharp, a tube soft along its length. How bright: the window's
// own lamp now (a flood slot, read every frame: a television flickers, the studio dims after four,
// the sheriff's goes out with him), the light falling off with the distance from the lamp and
// faded out towards the reach, and held back where the area's other floods already light the
// ground (a street lamp's pool), so it shows where the street is dark, more when the lamps go out
// (event 8).

const VS = /* glsl */ `
attribute vec3 aUvq;
attribute vec3 aCol;
attribute vec2 aBlur;
varying vec3 vUvq;
varying vec3 vCol;
varying vec2 vBlur;
#include <common>
#include <fog_pars_vertex>
void main() {
  vUvq = aUvq; vCol = aCol; vBlur = aBlur;
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
varying vec2 vBlur;
#include <common>
#include <fog_pars_fragment>
void main() {
  vec2 uv = vUvq.xy / vUvq.z;
  // the lamp's size seen from here through the glass, in the picture's pixels across and up
  // (vBlur: the lamp's size in them): none at the glass, growing out from it
  vec2 b = vBlur * max(vUvq.z - 1.0, 0.0) / vUvq.z;
  float bs = min(b.x, b.y), bl = max(b.x, b.y);
  // the level the picture is read at: what the screen needs, or the lamp's blur (its short way;
  // a tube's long way is three reads along it)
  vec2 px = uv * uMapSize, dx = dFdx(px), dy = dFdy(px);
  float lod = max(0.5 * log2(max(max(dot(dx, dx), dot(dy, dy)), 1e-8)), log2(max(max(bs, 0.33 * bl), 1.0)));
  vec2 e = (b - bs) / (3.0 * uMapSize);
  vec3 c = (textureLod(uMap, uv - e, lod).rgb + textureLod(uMap, uv, lod).rgb + textureLod(uMap, uv + e, lod).rgb) * (vCol / 3.0);
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
  /** How far out the light is laid at most (metres from the glass): it fades out on the way there. */
  reach?: number;
  /** The size of the room's lamp (metres, across the window and up): its blur on the ground. A
   *  bare bulb about 0.1, a shade 0.3, a fluorescent tube its length across (or up, when it runs
   *  into the room). */
  size?: [number, number];
  /** The part of its picture this window shows, across (a long band of windows is laid as a few
   *  parts, each from the lamp behind it). */
  part?: [number, number];
  /** The ground's own colour under it (the light it adds is the light on that ground). */
  tint?: (x: number, z: number) => THREE.Color;
}
type Win = WindowSpec & { at: number; n: number; fall: number[]; level: number; w0: number; col: THREE.Color };

// the grid of a window's light (across, out) and of its glow at the foot of the wall
const GRID_U = 6, GRID_V = 10, GLOW_U = 4, GLOW_V = 4;
/** How far under the sill the light is laid (a part of the window's height): the sill's blur. */
const UNDER = 0.15;
/** The glow of the whole glass at the foot of the wall: how bright against the light through it,
 *  how far out (metres), and how much wider than the glass either side. */
const GLOW = 0.15, GLOW_OUT = 1.6, GLOW_SIDE = 0.35;
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
  private blur: number[] = [];
  private quads: number[] = [];
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

  /** A window's light; call build() after the last (it is laid then, on the ground as it is by then). */
  add(s: WindowSpec) { this.specs.push(s); }
  private specs: WindowSpec[] = [];

  private lay(s: WindowSpec) {
    const a = _a.copy(_up).cross(s.out).normalize();   // across, to the right seen from outside
    const lamp = new THREE.Vector3().copy(s.c).addScaledVector(s.out, -s.lamp.depth).addScaledVector(a, s.lamp.side ?? 0);
    lamp.y = s.floor + s.lamp.y;
    const reach = s.reach ?? 6;
    // where the ray from the lamp through the glass at (u, v) meets the ground, and q there
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
    // how far up the glass the light still comes down within reach (at the window's middle)
    let v1 = 1;
    const out = (v: number) => { const t = hit(0.5, v, _p); return t < 0 ? Infinity : _p.clone().sub(s.c).dot(s.out); };
    if (out(1) > reach) { let lo = 0, hi = 1; for (let k = 0; k < 20; k++) { const m = (lo + hi) / 2; if (out(m) > reach) hi = m; else lo = m; } v1 = lo; }
    if (v1 < 0.08) return;
    const at = this.pos.length / 3, fall: number[] = [];
    // the lamp's size in the picture's pixels (across a part of the picture, a long band's)
    const part = s.part ? s.part[1] - s.part[0] : 1, size = s.size ?? [0.12, 0.12];
    const bu = size[0] * WINDOW_PX * part / s.w, bv = size[1] * WINDOW_PX / s.h;
    const v0 = -UNDER;
    for (let j = 0; j <= GRID_V; j++) for (let i = 0; i <= GRID_U; i++) {
      const u = i / GRID_U, v = v0 + (j / GRID_V) * (v1 - v0), t = hit(u, v, _p);
      this.pos.push(_p.x, _p.y, _p.z);
      const tc = s.tint?.(_p.x, _p.z);
      this.tints.push(tc?.r ?? 1, tc?.g ?? 1, tc?.b ?? 1);
      const [ua, va] = windowUv(s.style, s.part ? s.part[0] + u * (s.part[1] - s.part[0]) : u, v);
      this.uvq.push(ua * t, va * t, t);
      this.blur.push(bu, bv);
      // irradiance from a lamp: the cosine on the ground over the distance squared; faded out
      // from about half the reach on, so the far end ends nowhere in particular (not on a curb)
      const d = _p.distanceTo(lamp), e = (lamp.y - _p.y) / d / (d * d), far = _p.clone().sub(s.c).dot(s.out);
      fall.push(e * (1 - THREE.MathUtils.smoothstep(far, reach * 0.45, reach)));
    }
    // the light where the sill's line comes down at the window's middle, for the fall-off (the
    // irradiance there is 1)
    hit(0.5, 0, _p);
    const d0 = _p.distanceTo(lamp), ref = (lamp.y - _p.y) / d0 / (d0 * d0);
    for (let k = 0; k < fall.length; k++) fall[k] = Math.min(1.25, fall[k] / ref);
    this.quads.push(at, GRID_U, GRID_V);
    // the glow of the whole glass at the foot of the wall, from the wall out
    const g0 = this.pos.length / 3, wall = s.c.clone().addScaledVector(s.out, -0.05);
    for (let j = 0; j <= GLOW_V; j++) for (let i = 0; i <= GLOW_U; i++) {
      const u = i / GLOW_U, v = j / GLOW_V;
      _p.copy(wall).addScaledVector(a, (u - 0.5) * (s.w + 2 * GLOW_SIDE)).addScaledVector(s.out, 0.02 + v * GLOW_OUT);
      _p.y = this.ground(_p.x, _p.z) + 0.02;
      this.pos.push(_p.x, _p.y, _p.z);
      const tc = s.tint?.(_p.x, _p.z);
      this.tints.push(tc?.r ?? 1, tc?.g ?? 1, tc?.b ?? 1);
      const [ua, va] = windowUv('glow', u, v);
      this.uvq.push(ua, va, 1);
      this.blur.push(0, 0);
      fall.push(GLOW);
    }
    this.quads.push(g0, GLOW_U, GLOW_V);
    const w: Win = { ...s, at, n: fall.length, fall, level: s.slot === undefined ? 1 : 0, w0: s.slot !== undefined ? Math.max(1e-3, this.set.pos[s.slot].w) : 1, col: new THREE.Color(s.color) };
    this.wins.push(w);
  }

  build() {
    for (const s of this.specs) this.lay(s);
    this.specs = [];
    const g = this.mesh.geometry, idx: number[] = [], Q = this.quads;
    for (let q = 0; q < Q.length; q += 3) for (let j = 0; j < Q[q + 2]; j++) for (let i = 0; i < Q[q + 1]; i++) {
      const a = Q[q] + j * (Q[q + 1] + 1) + i, b = a + 1, c = a + Q[q + 1] + 1, d = c + 1;
      idx.push(a, b, d, a, d, c);
    }
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('aUvq', new THREE.Float32BufferAttribute(this.uvq, 3));
    g.setAttribute('aBlur', new THREE.Float32BufferAttribute(this.blur, 2));
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
  /** Where a window's glass and its light on the ground are, in world terms (tools/cookies.py
   *  compares how bright they look): the glass's corners, the points of the light's grid, the way out. */
  shapes(id: string) {
    const m = this.mesh.matrixWorld, o: { glass: THREE.Vector3[]; patch: THREE.Vector3[]; c: THREE.Vector3; out: THREE.Vector3 }[] = [];
    for (const w of this.wins) {
      if (w.id !== id) continue;
      const a = new THREE.Vector3().copy(_up).cross(w.out).normalize();
      const glass = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(([i, j]) => w.c.clone().addScaledVector(a, i * w.w / 2).add(new THREE.Vector3(0, j * w.h / 2, 0)).applyMatrix4(m));
      const patch: THREE.Vector3[] = [];
      for (let k = w.at; k < w.at + (GRID_U + 1) * (GRID_V + 1); k++) patch.push(new THREE.Vector3(this.pos[k * 3], this.pos[k * 3 + 1], this.pos[k * 3 + 2]).applyMatrix4(m));
      o.push({ glass, patch, c: w.c.clone().applyMatrix4(m), out: w.out.clone().transformDirection(m) });
    }
    return o;
  }
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
  // the angle), and what is left for the window's: about half under a street lamp (two thirds of
  // it held back before the windows were made dimmer than their rooms: it hid them from the street)
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
      this.dark[k] = 1 / (1 + 1.5 * e * set.scale.value);
    }
  }

  dispose() {
    this.mesh.geometry.dispose(); this.mat.dispose();
    const i = windowLights.indexOf(this); if (i >= 0) windowLights.splice(i, 1);
  }
}
