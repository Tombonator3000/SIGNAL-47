import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { GlowPoints, glowScale } from './glow';
import { floodlit, type FloodSet } from './kit';
import { canvasTex, rng } from '../core/textures';
import { getQuality, materialFor } from '../core/quality';

// Life in the night (Tom, 19 October: more realistic places, with more in them). The night is
// alive without being busy: moths round the sodium lamps and the porch lights (and a few resting
// on the wall by a porch light), bats at the lamps the player walks under in the early night,
// dust in the flashlight's beam and in the headlights on the survey track, lights far out on the
// land (a car on a county road now and then, ranch and trailer windows that go out and come on, a
// siding's lamps), tumbleweeds caught where the wind left them, and now and then one across the
// highway or the lot in a gust. None of it is the story's: nothing comes near, nothing stops to
// look, nothing glows in the sky (core/glimpses.ts, story/nightEvents.ts and story/townEvents.ts
// have the night's strange things, and while one of them is on there is no car and no tumbleweed).
//
// Cheap on purpose, a phone runs High: all the moths and bats of an area are one point cloud that
// moves in its vertex shader (no work per frame but a few numbers), the dust is one cloud for the
// flashlight and one for the headlights, drawn only while the light is on, the far lights are one
// GlowPoints per area, a tumbleweed is six crossed planes, drawn only while one rolls, and those
// lying about are one mesh per lamp set. Low keeps two thirds of the moths, half the dust and
// half the lying tumbleweeds, and no bats. Nothing here is a collider or a light, and nothing can
// be hit by a ray (the crosshair, the ground under the feet). It has its own random numbers, so
// the rest of the night goes the same with it or without it.
//
// Every area's lamps are read from its own glows (world/glow.ts), as airlight.ts reads its cones,
// so a lamp that goes out (Caliche's at dawn and in event 8) takes its moths with it, and the
// areas' own files need not know about any of this.

/** The life's own random numbers (never Math.random: the rest of the night draws from that). */
const rand = rng((Date.now() % 2147483647) ^ 0x4747);
const rnd = (a: number, b: number) => a + rand() * (b - a);

/** A lamp the insects find: where (in the moth cloud's terms), its colour, how lit it is now, how
 *  high over the ground; a porch light has the wall it is on (normal, and places to rest on it). */
export interface LifeLamp { p: THREE.Vector3; col: THREE.Color; k: () => number; big: boolean; h: number; wall?: { n: THREE.Vector3; rest: THREE.Vector3[] } }
export type LifeArea = 'saro' | 'road' | 'station01';

/** The lamps among some groups' glows: warm or white, bright, the size of a lamp's own glow (not
 *  its halo, not an indicator or a lamp indoors, not a red beacon or a coloured sign), in `into`'s
 *  terms. ground: the ground under a world point. */
export function lifeLamps(roots: THREE.Object3D[], into: THREE.Object3D, ground: (w: THREE.Vector3) => number, ok: (w: THREE.Vector3, size: number, h: number) => boolean): LifeLamp[] {
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
        if (r < 0.75 || g < 0.3 || b > g || s < 0.38 || s > 6) continue;
        const p = new THREE.Vector3(pos.getX(i), pos.getY(i), pos.getZ(i)).applyMatrix4(m);
        w.copy(p).applyMatrix4(into.matrixWorld);
        const h = w.y - ground(w);
        if (!ok(w, s, h) || out.some((L) => L.p.distanceToSquared(p) < 0.6)) continue;
        // lit as long as its glow is (Town.ts darkens a lamp's glow when it goes off)
        out.push({ p, col: new THREE.Color(r, g, b).multiplyScalar(1 / r), k: () => col.getX(i) / r, big: s >= 2, h });
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
// Four kinds of point in one cloud: 0 a moth flying round the lamp, 1 a bat, 2 a moth resting on
// the wall by a porch light, 3 a moth that keeps close in front of the lens (a dark flicker
// against it). A flying moth is drawn as a short streak along its way on the screen over the last
// thirtieth of a second, as the eye and a camera see one, and never thinner than one and a half
// pixels (a phone's screen and the adaptive resolution would lose them under a pixel).
const MOTH_VS = /* glsl */`
  attribute vec4 aSeed; attribute float aLamp; attribute float aKind; attribute vec3 aCol; attribute vec4 aWall;
  uniform float uTime, uPx, uMoth, uBat, uShut;
  uniform float uK[LAMPS];
  varying vec3 vCol; varying float vA; varying float vKind; varying float vFlap; varying vec2 vDir; varying vec3 vS;
  // where a moth is round its lamp (the lens, at position) at time t
  vec3 mothAt(float t) {
    float porch = aWall.w, wide = step(0.86, aSeed.y) * step(aKind, 2.5);
    float tt = t * (0.8 + 0.5 * aSeed.x);
    // most of a swarm keeps within half a metre of the lens, a few go out on wider loops
    float r0 = mix(0.12 + 0.38 * aSeed.y * aSeed.y, 0.6 + 0.8 * aSeed.z, wide) * (aKind > 2.5 ? 0.4 : 1.0);
    float r = r0 * (0.75 + 0.25 * sin(tt * 1.3 + aSeed.z * 6.28));
    // (the tighter the loop, the quicker round it)
    float a1 = tt * (4.0 + 4.0 * aSeed.z) / (0.5 + 2.5 * r0) * (aSeed.w < 0.5 ? 1.0 : -1.0) + aSeed.w * 6.28;
    // under a pole lamp's head; at or above the bottom edge of a porch light
    float y = mix(-0.16 + 0.14 * sin(tt * 2.1 + aSeed.x * 9.0), 0.1 + 0.12 * sin(tt * 2.3 + aSeed.x * 9.0), porch) * (1.0 + 2.0 * wide);
    vec3 o = vec3(cos(a1) * r, y + 0.06 * sin(tt * 7.3 + aSeed.y * 7.0), sin(a1 * 1.17 + aSeed.y * 3.0) * r);
    // and the dart and jink of a moth at a light
    o += 0.04 * vec3(sin(tt * 17.0 + aSeed.w * 30.0), sin(tt * 19.0 + aSeed.z * 20.0), cos(tt * 15.0 + aSeed.x * 25.0));
    // a wall behind the light: a loop that would go into it comes back out in front
    float on = dot(o, aWall.xyz);
    o += aWall.xyz * (abs(on) - on + 0.05 * porch);
    return position + o;
  }
  vec2 screen(vec4 mv) { return mv.xy / max(-mv.z, 0.05) * uPx; }
  void main() {
    float k = uK[int(aLamp + 0.5)];
    vec3 c = position, p = c;
    float size, a, px, len = 0.0;
    vec2 dir = vec2(1.0, 0.0);
    vec4 mv;
    vS = vec3(0.0);
    if (aKind < 0.5 || aKind > 2.5) {
      p = mothAt(uTime);
      mv = modelViewMatrix * vec4(p, 1.0);
      vec2 d = screen(mv) - screen(modelViewMatrix * vec4(mothAt(uTime - uShut), 1.0));
      len = length(d);
      if (len > 1e-3) dir = d / len;
      float dl = length(p - c);
      float lit = 0.25 + 1.0 / (1.0 + dl * dl * 3.0);
      // between the eye and the lens it is a silhouette against the light
      vec3 lv = (modelViewMatrix * vec4(c, 1.0)).xyz;
      float perp = length(mv.xy / -mv.z - lv.xy / -lv.z) * -lv.z;
      float R = aKind > 2.5 ? 0.3 : 0.16;
      float dark = (1.0 - smoothstep(R * 0.5, R, perp)) * smoothstep(0.0, 0.06, mv.z - lv.z);
      vFlap = 0.5 + 0.5 * abs(sin(uTime * (31.0 + 17.0 * aSeed.y) + aSeed.w * 6.28));
      vCol = mix(mix(aCol, vec3(1.0, 0.86, 0.66), 0.15) * lit * vFlap, vec3(0.03, 0.021, 0.013), dark);
      size = 0.055;
      // fewer as the night cools: each moth has its own threshold
      a = k * step(aSeed.x, uMoth) * mix(1.0, 0.75, dark);
      vKind = 0.0;
    } else if (aKind < 1.5) {
      // a bat: wide quick loops through the lamp's light and out of it, hunting the moths;
      // each comes for a while and is off into the dark for a while (aWall.w: the lamp's height)
      float t = uTime * (0.85 + 0.3 * aSeed.x) + aSeed.w * 40.0;
      float R = (2.5 + 4.5 * (0.5 + 0.5 * sin(t * 0.23 + aSeed.y * 6.28))) * aWall.w;
      float a1 = t * (6.0 / R) * (aSeed.z < 0.5 ? 1.0 : -1.0);
      p = c + vec3(cos(a1) * R + 1.2 * sin(t * 2.3 + aSeed.y * 9.0), (-1.4 + 1.3 * sin(t * 0.9 + aSeed.z * 5.0) + 0.4 * sin(t * 3.1)) * aWall.w, sin(a1 * 0.83 + aSeed.x * 4.0) * R);
      mv = modelViewMatrix * vec4(p, 1.0);
      float d = length(p - c);
      vCol = aCol / (1.0 + d * d * 0.07) * (0.25 + 0.75 * k);
      size = 0.3;
      a = uBat * smoothstep(-0.3, 0.2, sin(t * 0.045 + aSeed.y * 6.28));
      vFlap = sin(uTime * (13.0 + 3.0 * aSeed.x) + aSeed.w * 6.28);
      vKind = 1.0;
    } else {
      // resting on the wall by the light, wings flat; now and then one twitches, shifts a little
      // and settles again
      float per = 14.0 + 22.0 * aSeed.y, ph = uTime / per + aSeed.z, n = floor(ph);
      p = c + vec3(0.0, 0.018 * (fract(sin(n * 12.9898 + aSeed.w * 78.2) * 43758.5) - 0.5), 0.0);
      mv = modelViewMatrix * vec4(p, 1.0);
      vFlap = 1.0 + 0.6 * step(fract(ph), 0.012) * abs(sin(uTime * 40.0));
      vCol = aCol * (0.05 + 0.1 * k) + 0.003;
      size = 0.042;
      a = 0.92;
      vKind = 2.0;
      dir = vec2(cos(aSeed.x * 6.28), sin(aSeed.x * 6.28));
    }
    gl_Position = projectionMatrix * mv;
    float dist = -mv.z;
    px = size * uPx / max(dist, 0.1);
    float w = max(px, 1.5);
    if (vKind < 0.5) {
      // the streak: from where it was a thirtieth of a second ago to where it is
      float L = clamp(len, 2.5, 6.0);
      gl_PointSize = L + w + 1.0;
      vS = vec3(gl_PointSize, L, 0.5 * w);
      // the point's middle is half way along it
      gl_Position.xy -= dir * (0.5 * L) / uPx * vec2(projectionMatrix[0][0], projectionMatrix[1][1]) * gl_Position.w;
    } else if (vKind < 1.5) {
      gl_PointSize = clamp(px, 1.0, 64.0);
    } else {
      gl_PointSize = w * 1.4 + 1.0;
      vS = vec3(gl_PointSize, 0.0, 0.5 * w);
    }
    vDir = dir;
    // never thinner than a pixel and a half; fainter when really smaller; gone well before they would only flicker
    vA = a * clamp(px / 0.9, 0.45, 1.0) * (1.0 - smoothstep(26.0, 48.0, dist)) * smoothstep(0.4, 1.0, dist);
    if (vKind > 0.5 && vKind < 1.5) vA = a * min(px, 1.0) * (1.0 - smoothstep(32.0, 55.0, dist)) * smoothstep(0.5, 1.5, dist);
    if (vA < 0.002) gl_Position = vec4(2.0, 2.0, 2.0, 1.0);   // (not flying now, or too far: no pixels)
  }`;
const MOTH_FS = /* glsl */`
  uniform float uGain;
  varying vec3 vCol; varying float vA; varying float vKind; varying float vFlap; varying vec2 vDir; varying vec3 vS;
  void main() {
    vec2 q = gl_PointCoord - 0.5;
    float a; vec3 col;
    if (vKind < 0.5) {
      // a streak with round ends, brighter at its head (where the moth is now)
      vec2 s = vec2(q.x, -q.y) * vS.x;
      float h = clamp(dot(s, vDir), -0.5 * vS.y, 0.5 * vS.y);
      float e = length(s - vDir * h);
      a = (1.0 - smoothstep(vS.z - 0.5, vS.z + 0.5, e)) * (0.55 + 0.45 * (h / max(vS.y, 1e-3) + 0.5)) * vA;
      col = vCol * uGain;
    } else if (vKind < 1.5) {
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
    } else {
      // a moth at rest: a small dark delta, the wings opening a little when it twitches
      vec2 s = vec2(q.x, -q.y) * vS.x;
      vec2 u = vec2(dot(s, vDir), dot(s, vec2(-vDir.y, vDir.x))) / max(vS.z * 2.0, 1e-3);
      float wing = step(-0.55, u.x) * step(u.x, 0.5) * step(abs(u.y), (0.55 - u.x) * 0.55 * vFlap);
      a = max(wing, 1.0 - smoothstep(0.35, 0.6, length(u))) * vA;
      col = vCol;
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
  /** Points in the buffer: what Low draws first (two thirds of each lamp's moths and those at rest), then the rest, then the bats. */
  readonly n: { low: number; moths: number; bats: number; resting: number };
  /** bats: how many hunt at each lamp (by index). */
  constructor(readonly lamps: LifeLamp[], parent: THREE.Object3D, bats: number[], seed: number) {
    const r = rng(seed), L = Math.max(1, lamps.length);
    const pos: number[] = [], sd: number[] = [], li: number[] = [], kind: number[] = [], col: number[] = [], wall: number[] = [];
    const put = (i: number, b: number, at?: THREE.Vector3) => {
      const x = lamps[i], p = at ?? x.p, c = x.col, n = x.wall?.n;
      pos.push(p.x, p.y, p.z); sd.push(r(), r(), r(), r()); li.push(i); kind.push(b); col.push(c.r, c.g, c.b);
      // a moth: the wall behind its light and whether it is a porch light; a bat: how wide it hunts (a low lamp, closer in)
      if (b === 1) wall.push(0, 0, 0, THREE.MathUtils.clamp(x.h / 7.5, 0.55, 1));
      else wall.push(n?.x ?? 0, n?.y ?? 0, n?.z ?? 0, n ? 1 : 0);
    };
    // no two lamps alike: one swarmed, one all but empty (0.3 to 1.8 times a dozen at a pole
    // lamp, eight at a porch light), at the dead of night
    const per = lamps.map((x) => Math.max(1, Math.round((x.wall ? 8 : 12) * (0.3 + 1.5 * Math.pow(r(), 1.6)))));
    let resting = 0;
    lamps.forEach((x, i) => {
      for (let j = 0; j < Math.ceil(per[i] * 2 / 3); j++) put(i, 0);
      put(i, 3);
      for (const q of x.wall?.rest ?? []) { put(i, 2, q); resting++; }
    });
    const low = pos.length / 3;
    lamps.forEach((_, i) => { for (let j = Math.ceil(per[i] * 2 / 3); j < per[i]; j++) put(i, 0); put(i, 3); });
    const moths = pos.length / 3;
    bats.forEach((nb, i) => { for (let b = 0; b < nb; b++) put(i, 1); });
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setAttribute('aSeed', new THREE.Float32BufferAttribute(sd, 4));
    g.setAttribute('aLamp', new THREE.Float32BufferAttribute(li, 1));
    g.setAttribute('aKind', new THREE.Float32BufferAttribute(kind, 1));
    g.setAttribute('aCol', new THREE.Float32BufferAttribute(col, 3));
    g.setAttribute('aWall', new THREE.Float32BufferAttribute(wall, 4));
    this.n = { low, moths, bats: pos.length / 3 - moths, resting };
    this.k = new Array(L).fill(1);
    this.mat = new THREE.ShaderMaterial({
      // (a gain over one and the tonemapping turns the sodium's amber white, like the stars behind)
      uniforms: { uTime: { value: 0 }, uPx: glowScale, uMoth: { value: 1 }, uBat: { value: 0 }, uGain: { value: 1.2 }, uShut: { value: 1 / 30 }, uK: { value: this.k } },
      defines: { LAMPS: L }, vertexShader: MOTH_VS, fragmentShader: MOTH_FS,
      transparent: true, depthWrite: false, fog: false,
    });
    this.points = new THREE.Points(g, this.mat);
    this.points.name = 'life:moths';
    this.points.frustumCulled = false;
    this.points.renderOrder = 3;   // over the lamps' own glows (2): a bat or a moth crossing one darkens it
    this.points.raycast = () => {};
    this.points.visible = false;
    parent.add(this.points);
  }
  /** Points drawn now (moths, bats), for the tests. */
  drawn = { moths: 0, bats: 0 };
  /** Round all the lamps in world terms, 60 m out: farther off nothing of them shows (the shader
   *  fades them out by 48 m), so they are not drawn at all (Caliche from the survey track). */
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

/** The triangles of the scene's meshes within r of a point (world terms), once per porch light:
 *  the rays that find its wall then test these few and not every triangle of a merged block. */
function trisNear(scene: THREE.Object3D, p: THREE.Vector3, r: number) {
  const out: THREE.Triangle[] = [], box = new THREE.Box3(p.clone().subScalar(r), p.clone().addScalar(r)), local = new THREE.Box3(), inv = new THREE.Matrix4();
  const a = new THREE.Vector3(), b = new THREE.Vector3(), c = new THREE.Vector3(), tb = new THREE.Box3();
  scene.traverse((o) => {
    const m = o as THREE.Mesh;
    if (!m.isMesh || (m as THREE.InstancedMesh).isInstancedMesh || m.name.startsWith('life') || !m.geometry.getAttribute('position')) return;
    const g = m.geometry;
    if (!g.boundingSphere) g.computeBoundingSphere();
    _sph.copy(g.boundingSphere!).applyMatrix4(m.matrixWorld);
    if (_sph.distanceToPoint(p) > r) return;
    local.copy(box).applyMatrix4(inv.copy(m.matrixWorld).invert());
    const pos = g.getAttribute('position'), idx = g.index, n = idx ? idx.count : pos.count;
    for (let i = 0; i + 2 < n; i += 3) {
      const i0 = idx ? idx.getX(i) : i, i1 = idx ? idx.getX(i + 1) : i + 1, i2 = idx ? idx.getX(i + 2) : i + 2;
      a.fromBufferAttribute(pos, i0); b.fromBufferAttribute(pos, i1); c.fromBufferAttribute(pos, i2);
      if (!local.intersectsBox(tb.setFromPoints([a, b, c]))) continue;
      out.push(new THREE.Triangle(a.clone().applyMatrix4(m.matrixWorld), b.clone().applyMatrix4(m.matrixWorld), c.clone().applyMatrix4(m.matrixWorld)));
    }
  });
  return out;
}
/** The nearest of some triangles along a ray within far, with the face's normal turned to the ray. */
function hitTris(tris: THREE.Triangle[], o: THREE.Vector3, d: THREE.Vector3, far: number) {
  _ray.set(o, d);
  let best: { t: number; n: THREE.Vector3; at: THREE.Vector3 } | null = null;
  for (const T of tris) {
    if (!_ray.intersectTriangle(T.a, T.b, T.c, false, _hit)) continue;
    const t = _hit.distanceTo(o);
    if (t > far || (best && t >= best.t)) continue;
    const n = T.getNormal(new THREE.Vector3());
    best = { t, n: n.dot(d) > 0 ? n.negate() : n, at: _hit.clone() };
  }
  return best;
}
/** The wall a porch light is on (world terms), found with short rays round the fixture: the side
 *  of it where most of nine of them meet an upright face within 0.8 m (a wall, not a post or a
 *  narrow board the lamp hangs by). Then two to four places round the fixture where a moth can
 *  sit, each on the wall itself (or the door's frame), none on the fixture. */
function wallOf(p: THREE.Vector3, scene: THREE.Object3D, r: () => number) {
  const tris = trisNear(scene, p, 1.4);
  if (!tris.length) return null;
  const at = (n: THREE.Vector3, u: number, v: number) => _o.copy(p).addScaledVector(_side.set(-n.z, 0, n.x), u).setY(p.y + v);
  let best: { n: THREE.Vector3; d: number[] } | null = null;
  for (const [x, z] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
    const n = new THREE.Vector3(-x, 0, -z), d: number[] = [];
    for (const u of [-0.45, 0, 0.45]) for (const v of [-0.3, 0.15, 0.35]) {
      const h = hitTris(tris, at(n, u, v), _dir.set(x, 0, z), 0.8);
      if (h && Math.abs(h.n.y) < 0.25) d.push(h.t);
    }
    if (d.length >= 4 && (!best || d.length > best.d.length)) best = { n, d };
  }
  if (!best) return null;
  const n = best.n, D = best.d.sort((x, y) => x - y)[best.d.length >> 1];
  const rest: THREE.Vector3[] = [], want = 2 + Math.floor(r() * 3);
  for (let k = 0; k < 10 && rest.length < want; k++) {
    // beside and above the fixture more than under it (the door is under it)
    const u = (r() < 0.5 ? -1 : 1) * (0.16 + 0.3 * r()), v = -0.12 + 0.5 * r();
    if (Math.abs(u) < 0.2 && v < 0.12) continue;
    const h = hitTris(tris, at(n, u, v), _dir.copy(n).negate(), D + 0.3);
    if (!h || Math.abs(h.t - D) > 0.2 || h.n.dot(n) < 0.9 || rest.some((o) => o.distanceTo(h.at) < 0.1)) continue;
    rest.push(h.at.addScaledVector(n, 0.022));
  }
  return { n, rest };
}
const _ray = new THREE.Ray(), _hit = new THREE.Vector3(), _sph = new THREE.Sphere(), _o = new THREE.Vector3(), _side = new THREE.Vector3(), _dir = new THREE.Vector3();

// ---------- dust in a beam ----------
const MOTE_VS = /* glsl */`
  attribute vec4 aSeed;
  uniform float uTime, uPx, uSize, uDensity, uWobble, uFall, uPuff, uPuffSize, uPuffA, uHead, uNear;
  uniform vec3 uCenter, uBox, uDrift;   // (uDrift: how far the air has carried them, wrapped by the box)
  uniform vec3 uBeamPos[2]; uniform vec3 uBeamDir[2]; uniform float uBeamK[2]; uniform vec2 uCos;
  varying float vA; varying float vPuff; varying float vSoft;
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
    // a few of them (when there is dust about) are soft puffs of it a metre or more across
    float puff = step(uPuff, aSeed.w);
    float light = 0.0, L0 = 0.0;
    for (int i = 0; i < 2; i++) {
      vec3 v = wp - uBeamPos[i];
      float L = length(v);
      float b = uBeamK[i] * smoothstep(uCos.x, uCos.y, dot(v, uBeamDir[i]) / max(L, 1e-3)) / (1.0 + L * L * uFall);
      // headlights on low beam: the top of the beam comes down with the distance
      b *= mix(1.0, 1.0 - smoothstep(-0.12, 0.1, v.y - (0.15 - 0.012 * L)), uHead);
      light += b;
      if (i == 0) L0 = L;
    }
    // the headlights' puffs only in the near part of the beam, thinning out along it
    light *= mix(1.0, smoothstep(2.5, 4.0, L0) * (1.0 - smoothstep(8.0, 14.0, L0)), puff * uHead);
    vec4 mv = viewMatrix * vec4(wp, 1.0);
    gl_Position = projectionMatrix * mv;
    float dist = -mv.z;
    float px = mix(uSize * (0.6 + 0.8 * aSeed.w), uPuffSize * (0.6 + 0.8 * fract(aSeed.w * 7.0)), puff) * uPx / max(dist, 0.05);
    gl_PointSize = clamp(px, 1.0, mix(16.0, 64.0, puff));
    vPuff = puff;
    // a speck close to the eye is out of focus: a faint soft disc with the same light in it
    float blur = puff > 0.5 ? 1.0 : min(1.0, 16.0 / max(px * px, 16.0));
    vSoft = puff > 0.5 ? 1.0 : smoothstep(4.0, 9.0, px);
    // specks are brightest looking down the beam (the light scatters forward), glinting as each one turns
    float fwd = mix(0.3 + 0.7 * pow(max(dot(normalize(wp - cameraPosition), uBeamDir[0]), 0.0), 3.0), 1.0, puff);
    float glint = 0.55 + 0.45 * sin(uTime * (2.0 + 5.0 * aSeed.y) + aSeed.z * 6.28);
    vA = step(aSeed.x, uDensity) * edge * light * fwd * mix(glint, uPuffA, puff) * min(px, 1.0) * blur
      * smoothstep(mix(uNear, 2.0, puff), mix(2.0 * uNear, 5.0, puff), dist);
    // out of the light: not drawn at all (outside the view, so it costs no pixels)
    if (vA < 0.002) gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
  }`;
const MOTE_FS = /* glsl */`
  uniform vec3 uCol;
  varying float vA; varying float vPuff; varying float vSoft;
  void main() {
    float d = length(gl_PointCoord - 0.5), s = smoothstep(0.5, 0.0, d);
    float a = mix(mix(smoothstep(0.5, 0.1, d), s, vSoft), s * s, vPuff) * vA;
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
        uPuff: { value: 2 }, uPuffSize: { value: 1.4 }, uPuffA: { value: 0.05 }, uHead: { value: 0 }, uNear: { value: 0.8 },
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
// origin. A county road runs north and south 1.4 km east of the highway, beyond the swells; a car
// comes along it now and then, its lights crawling across the dark, and goes on. Ranch, farm and
// trailer windows: some go out in the small hours, some come on before dawn (a rancher up early),
// one is lit for a while at half past three, a trailer's is the blue of a television until it is
// switched off. A siding on the railroad to Roswell has a string of lamps far off, north of SARO
// and north-east of Caliche. Each area shows those over its own drawn land, where the land does not
// hide them (east of SARO a ridge does, and the land rising east of Caliche). The farther off, the
// dimmer and redder, and the more the air near the ground makes them twinkle.
const COUNTY = (z: number) => 1450 + 70 * Math.sin(z / 900 + 0.4);
type Far = { x: number; z: number; y: number; size: number; col: number; on: [number, number]; k?: number; tv?: boolean };
const WINDOWS: Far[] = [
  { x: 835, z: 1118, y: 1.4, size: 3.2, col: 0xffc27a, on: [4.58, 9] },      // a ranch north-east of Caliche: its kitchen, from 04:35
  { x: 850, z: 1100, y: 5.5, size: 4.5, col: 0xbcd4ff, on: [-6, 9] },        // its yard light (mercury), all night
  { x: -1000, z: 1270, y: 1.4, size: 3, col: 0xffb868, on: [-6, 2.87] },     // a house north-west of Caliche, out at 02:52
  { x: -1220, z: 1080, y: 1.3, size: 2.6, col: 0xffb466, on: [-6, 9] },      // a farmhouse past it: its porch light,
  { x: -1208, z: 1070, y: 1.4, size: 2.3, col: 0xffc690, on: [4.83, 9] },    // and its kitchen from ten to five
  { x: -470, z: -1170, y: 1.4, size: 3, col: 0xffc888, on: [3.33, 3.75] },   // north of SARO, lit 03:20 to 03:45
  { x: -1300, z: 250, y: 1.4, size: 2.8, col: 0x9ab4ff, on: [-6, 3.08], tv: true },   // a trailer west of SARO: the television, off at 03:05
  { x: -742, z: -943, y: 6.5, size: 4.2, col: 0xc4dcff, on: [-6, 9] },       // a ranch north-west of SARO: its yard light,
  { x: -760, z: -930, y: 1.4, size: 2.6, col: 0xffbe78, on: [-6, 3.6] },     // a window out at 03:36,
  { x: -748, z: -918, y: 1.4, size: 2.2, col: 0xffc888, on: [-6, 9], k: 0.6 },   // and a dim one all night
  // the siding's lamps
  ...[[-330, -1530], [-260, -1545], [-190, -1556], [-120, -1566], [-50, -1574]].map(([x, z]): Far => ({ x, z, y: 4, size: 2.6, col: 0xffa850, on: [-6, 9], k: 0.75 })),
  ...[0, 1, 2, 3, 4, 5].map((i): Far => ({ x: 900 + i * 48, z: 1000 - i * 36, y: 4, size: 2.6, col: 0xffa850, on: [-6, 9], k: 0.7 })),
];
const HEAD = new THREE.Color(1, 0.9, 0.72), TAIL = new THREE.Color(1, 0.05, 0.03), POOL = new THREE.Color(0.11, 0.085, 0.05);

export class FarLights {
  readonly glow = new GlowPoints();
  private readonly win: { i: number; w: Far; ph: number; p: THREE.Vector3 }[] = [];
  private readonly car: number[];
  constructor(scene: THREE.Scene, private readonly toWorld: (x: number, z: number) => THREE.Vector3, ground: (x: number, z: number) => number, centre: { x: number; z: number }) {
    for (const w of WINDOWS) {
      if (Math.hypot(w.x - centre.x, w.z - centre.z) > 1800) continue;
      const p = toWorld(w.x, w.z).setY(ground(w.x, w.z) + w.y);
      this.win.push({ i: this.glow.add(p.x, p.y, p.z, w.size, w.col), w, ph: rand() * 60, p });
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
  /** A glow's size: its own in metres, but never under three and a half pixels across (its
   *  bright core a pixel or two: so far off, and on a phone's screen, it is a point of light and
   *  not lost under a pixel). */
  private size(i: number, m: number, d: number) {
    const a = this.glow.points.geometry.getAttribute('aSize') as THREE.BufferAttribute, s = Math.max(m, 3.5 * d / glowScale.value);
    if (Math.abs(a.getX(i) - s) > 0.05 * s) { a.setX(i, s); a.needsUpdate = true; }
  }
  /** A light this far off: dimmer and redder (the air near the ground), twinkling a little more. */
  private far(i: number, c: THREE.Color, k: number, d: number, t: number, ph: number) {
    const f = THREE.MathUtils.smoothstep(d, 900, 1900), amp = 0.04 + 0.16 * THREE.MathUtils.smoothstep(d, 300, 1800);
    const tw = 1 + amp * (0.6 * Math.sin(t * 1.9 + ph) * Math.sin(t * 0.73 + ph * 1.7) + 0.4 * Math.sin(t * 3.1 + ph * 2.3));
    this.set(i, _c.setRGB(c.r, c.g * (1 - 0.12 * f), c.b * (1 - 0.3 * f)), k * (1 - 0.35 * f) * tw);
  }
  /** car: where on the county road (map z, heading +1 south or -1 north, how visible), or null. */
  update(t: number, on: boolean, hour: number, night: number, cam: THREE.Vector3, car: { z: number; dir: number; k: number } | null) {
    const P = this.glow.points;
    P.visible = on && night > 0.01;
    if (!P.visible) return;
    this.glow.update(t);
    for (const W of this.win) {
      const w = W.w, lit = (hour >= w.on[0] && hour < w.on[1] ? 1 : 0) * night * (w.k ?? 1);
      const d = Math.hypot(W.p.x - cam.x, W.p.z - cam.z);
      // (a television: its light jumps with the pictures, and is greyer between them)
      const tv = w.tv ? 0.7 + 0.3 * Math.sin(t * 2.3 + W.ph) * Math.sin(t * 5.1) + 0.15 * Math.sign(Math.sin(t * 0.37 + W.ph)) : 1;
      this.far(W.i, _c2.set(w.col), lit * tv, d, t, W.ph);
      this.size(W.i, w.size, d);
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
    this.far(hi, HEAD, k * (0.18 + 0.82 * THREE.MathUtils.smoothstep(face, -0.35, 0.9)), d, t, 3.1);
    this.far(ti, TAIL, k * 0.7 * THREE.MathUtils.smoothstep(-face, -0.2, 0.9), d, t, 7.7);
    this.size(hi, 8, d); this.size(ti, 5, d);
    this.set(pi, POOL, k);
  }
}
const _off = new THREE.Vector3(0, -999, 0), _c = new THREE.Color(), _c2 = new THREE.Color();

// ---------- tumbleweeds ----------
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
const WEED_R = 0.36;
/** The ball: six crossed planes (the three axes and three between them, so it never turns to a flat
 *  cross as it rolls), a little flattened, as a tumbleweed is. */
function weedBall() {
  const planes: THREE.BufferGeometry[] = [];
  for (const [rx, ry, rz] of [[0, 0, 0], [Math.PI / 2, 0, 0], [0, Math.PI / 2, 0], [-Math.PI / 4, 0, 0], [0, Math.PI / 4, 0], [0, Math.PI / 2, Math.PI / 4]]) {
    const p = new THREE.PlaneGeometry(2 * WEED_R, 2 * WEED_R);
    p.rotateX(rx); p.rotateY(ry); p.rotateZ(rz); p.scale(1.08, 0.86, 1);
    planes.push(p);
  }
  return mergeGeometries(planes)!;
}
/** The dry stems, lit only by the scene: the floods of a lamp set, the flashlight, the sky (no glow
 *  of their own: a pale thing that shone in the dark would be too like the night's strange ones).
 *  See-through, so far off it is a tangle and not a solid ball (alpha-tested lace turns into a disc
 *  in the smaller mipmaps); a grey tan, so out of the lamps' pools it is as dark as the ground. */
function weedMat(set: FloodSet) {
  const m = floodlit(new THREE.MeshStandardMaterial({ map: tumbleTex(), transparent: true, depthWrite: false, alphaTest: 0.03, side: THREE.DoubleSide,
    color: 0xb5a68a, roughness: 1 }), 0.012, set);
  m.forceSinglePass = true;   // (three draws a see-through double-sided mesh twice, back faces first; lace needs no such care)
  return m;
}
/** A random turn from the life's own numbers (Quaternion.random draws on Math.random). */
const turn = (q: THREE.Quaternion, r: () => number) => q.setFromEuler(_e.set(r() * 6.283, r() * 6.283, r() * 6.283));
const _e = new THREE.Euler();

/** Tumbleweeds caught where the wind left them (x, z in world terms; s: size), one mesh for a lamp
 *  set; Low draws the first half. Seen through, no collider, never hit by a ray. */
export class RestingWeeds {
  readonly mesh: THREE.Mesh;
  private readonly low: number;
  constructor(scene: THREE.Scene, mat: THREE.Material, spots: [number, number, number][], ground: (x: number, z: number) => number, name: string, seed: number) {
    const r = rng(seed), ball = weedBall(), parts: THREE.BufferGeometry[] = [], q = new THREE.Quaternion(), m = new THREE.Matrix4();
    for (const [x, z, s] of spots) {
      // squat, a little sunk where it lies, turned any way
      const g = ball.clone();
      m.compose(_v.set(x, ground(x, z) + WEED_R * s * 0.66, z), turn(q, r), _sc.set(s, s * (0.78 + 0.12 * r()), s));
      g.applyMatrix4(m);
      parts.push(g);
    }
    const all = mergeGeometries(parts)!;
    this.low = (ball.index!.count) * Math.ceil(spots.length / 2);
    this.mesh = new THREE.Mesh(all, mat);
    this.mesh.name = name;
    this.mesh.raycast = () => {};
    this.mesh.visible = false;
    scene.add(this.mesh);
  }
  /** Drawn in its area within 150 m (farther off they are less than a pixel: the motel's from the site). */
  show(on: boolean, low: boolean, cam: THREE.Vector3) {
    const b = this.mesh.geometry.boundingSphere ?? (this.mesh.geometry.computeBoundingSphere(), this.mesh.geometry.boundingSphere!);
    this.mesh.visible = on && cam.distanceTo(b.center) < b.radius + 150;
    this.mesh.geometry.setDrawRange(0, low ? this.low : Infinity);
  }
}
const _sc = new THREE.Vector3();

/** One tumbleweed: rolls along a path on the ground in a gust, hops now and then, and is gone. */
export class Tumbleweed {
  readonly mesh: THREE.Mesh;
  on = false;
  /** The ground under it (world x, z). */
  ground: (x: number, z: number) => number = () => 0;
  /** Which lamp set lights it where it starts (the first that holds the place, or the area's own). */
  floods: { mat: THREE.Material; where: (x: number, z: number) => boolean }[] = [];
  /** Its own material, lit by the area's lamp set. */
  readonly base: THREE.Material;
  private readonly from = new THREE.Vector3();
  private readonly dir = new THREE.Vector3();
  private s = 0;
  private len = 0;
  private top = 5;
  private hop = { h: 0, phase: 0, next: 3 };
  /** How far it has turned over (for the tests). */
  spin = 0;
  private readonly axis = new THREE.Vector3();
  private readonly R = WEED_R;
  /** Seconds it lies still against what stopped it, before it is gone (unseen). */
  private rest = 0;
  constructor(scene: THREE.Scene, set: FloodSet, name: string) {
    this.base = weedMat(set);
    this.mesh = new THREE.Mesh(weedBall(), this.base);
    this.mesh.name = name;
    this.mesh.raycast = () => {};   // never in the way of the crosshair or the ground under the feet
    this.mesh.visible = false;
    scene.add(this.mesh);
  }
  /** The material for a place: lit by the lamp set that lights it. */
  matAt(x: number, z: number) { return this.floods.find((f) => f.where(x, z))?.mat ?? this.base; }
  get at() { return this.mesh.position; }
  /** Roll from a point along a direction (world, on the ground) for len metres, at up to top m/s. */
  start(from: THREE.Vector3, dir: THREE.Vector3, len: number, top: number) {
    this.from.copy(from); this.dir.copy(dir).setY(0).normalize();
    this.s = 0; this.len = len; this.top = top; this.rest = 0; this.on = true; this.spin = 0;
    this.axis.set(0, 1, 0).cross(this.dir).normalize();
    this.hop = { h: 0, phase: 0, next: rnd(2, 6) };
    turn(this.mesh.quaternion, rand);
    this.mesh.material = materialFor(this.matAt(from.x, from.z));
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
    if ((this.hop.next -= ds) <= 0) { this.hop = { h: rnd(0.15, 0.6), phase: 0, next: rnd(2, 8) }; }
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
 *  terms (they all lie north up): towards the north-north-east. They fetch up on the south and
 *  west faces of what stands in the way. */
const WIND = new THREE.Vector3(0.45, 0, -0.89).normalize();

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
  /** Tumbleweeds lying about, one mesh per lamp set that lights them. */
  resting: RestingWeeds[];
  /** In the way of a tumbleweed (world x, z): a wall, a fence, a parked truck. */
  blocked: (x: number, z: number) => boolean;
  /** The road's terms of a world point here (the far car), or null for none. */
  toMap: ((x: number, z: number) => { x: number; z: number }) | null;
  /** Whether the highway runs under this world point (a tumbleweed across it in the headlights). */
  onHighway: (x: number, z: number) => boolean;
  /** Its lamps in world terms (a tumbleweed on foot comes where one of them lights it). */
  lampsW: THREE.Vector3[];
  ground: (x: number, z: number) => number;
  /** How long finding the porch lights' walls took, when the area was built (the tests). */
  wallMs?: number;
}

export class NightLife {
  /** Tests: the night without its life, to compare. */
  off = false;
  private readonly areas: Partial<Record<LifeArea, AreaLife>> = {};
  private readonly flash: Motes;
  private readonly head: Motes;
  private act = { moth: 1, bat: 0, hour: 2 };
  /** The headlights' dust: fine specks raised as the truck goes (by its speed), and the cloud it
   *  trails catching up with it as it slows and stops (thick, soft, then settling). */
  private specks = 0;
  private cloud = 0;
  private lastSpeed = 0;
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
    // (the flashlight's box holds the near part of its beam whichever way it points, where the
    // dust shows; the headlights' the first thirty metres of theirs)
    this.flash = new Motes(scene, 900, 'life:flashDust', 61, new THREE.Vector3(3.4, 2.6, 3.4));
    this.head = new Motes(scene, 3000, 'life:headDust', 62, new THREE.Vector3(34, 4.6, 34), true);
  }

  /** An area's far lights (if it sees the county road), its tumbleweed, and what stands in a tumbleweed's way. */
  addArea(id: LifeArea, o: { flood: FloodSet; ground: (x: number, z: number) => number; blocked: (x: number, z: number) => boolean;
    onHighway?: (x: number, z: number) => boolean;
    far?: { toWorld: (x: number, z: number) => THREE.Vector3; toMap: (x: number, z: number) => { x: number; z: number }; ground: (x: number, z: number) => number; centre: { x: number; z: number } } }) {
    const far = o.far ? new FarLights(this.scene, o.far.toWorld, o.far.ground, o.far.centre) : null;
    const weed = new Tumbleweed(this.scene, o.flood, `life:tumbleweed:${id}`);
    weed.ground = o.ground;
    this.areas[id] = { moths: null, far, weed, resting: [], blocked: o.blocked, toMap: o.far?.toMap ?? null, onHighway: o.onHighway ?? (() => false), lampsW: [], ground: o.ground };
  }
  /** Tumbleweeds lying where the wind left them in an area (world x, z, size), lit by a lamp set
   *  (the area's own when null); with `where`, a rolling one there is lit by that set too. */
  addWeeds(id: LifeArea, set: FloodSet | null, spots: [number, number, number][], where?: (x: number, z: number) => boolean) {
    const A = this.areas[id];
    if (!A?.weed) return;
    const mat = set ? weedMat(set) : A.weed.base;
    if (set && where) A.weed.floods.push({ mat, where });
    if (spots.length) A.resting.push(new RestingWeeds(this.scene, mat, spots, A.ground, `life:weeds:${id}:${A.resting.length}`, id.length * 7 + A.resting.length));
  }
  /** An area's moths and bats: its lamps (lifeLamps), in parent's terms. bats: [world place, how
   *  many] for the lamps the player walks under (the nearest lamp to each); walls: what a porch
   *  light's wall is found in. */
  addMoths(id: LifeArea, lamps: LifeLamp[], parent: THREE.Object3D, o: { bats: [THREE.Vector3, number][]; walls?: THREE.Object3D }) {
    const A = this.areas[id];
    if (!A || A.moths || !lamps.length) return;
    parent.updateWorldMatrix(true, false);
    const W = lamps.map((L) => parent.localToWorld(L.p.clone())), inv = new THREE.Matrix4().copy(parent.matrixWorld).invert();
    const r = rng(id.length * 131 + lamps.length);
    // (a porch light is one at a door's height; a lamp higher up is on a pole)
    const t0 = performance.now();
    if (o.walls) lamps.forEach((L, i) => {
      if (L.h > 3.6) return;
      const w = wallOf(W[i], o.walls!, r);
      if (w) L.wall = { n: w.n.transformDirection(inv), rest: w.rest.map((q) => q.applyMatrix4(inv)) };
    });
    const bats = lamps.map(() => 0);
    for (const [p, n] of o.bats) {
      let best = -1;
      W.forEach((w, i) => { if (w.distanceTo(p) < 30 && (best < 0 || w.distanceTo(p) < W[best].distanceTo(p))) best = i; });
      if (best >= 0) bats[best] += n;
    }
    A.wallMs = performance.now() - t0;
    A.moths = new LampMoths(lamps, parent, bats, id.length * 31 + lamps.length);
    A.lampsW = W.filter((_, i) => lamps[i].big);
  }

  update(dt: number, t: number, o: LifeIn) {
    this.t = t; this.last = o;
    const low = getQuality() === 'low', on = !this.off;
    const here: LifeArea | null = o.area === 'saro' ? 'saro' : o.area === 'road' || o.area === 'diner' ? 'road' : o.area === 'station01' ? 'station01' : null;
    if (here !== this.here) { for (const A of Object.values(this.areas)) A.weed?.stop(); this.here = here; }
    this.act = activity(o.clock, o.dawn);
    const night = 1 - THREE.MathUtils.smoothstep(o.dawn, 0.12, 0.4);
    // the moths and bats of the area shown, out in the open; its tumbleweeds lying about
    for (const [id, A] of Object.entries(this.areas) as [LifeArea, AreaLife][]) {
      A.moths?.update(t, on && id === here && o.out, low, this.act, o.cam.position);
      for (const R of A.resting) R.show(on && id === here, low, o.cam.position);
    }
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
    const m = A.toMap(c.x, c.z), dir = rand() < 0.5 ? 1 : -1;
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
    const F = this.flash, u = F.u, L = o.flash, k = L ? L.intensity / (L.userData.full ?? 40) : 0;
    F.points.visible = on && k > 0.01;
    if (F.points.visible && L) {
      const cam = o.cam.position, f = o.cam.getWorldDirection(this._f);
      u.uTime.value = this.t;
      // a box round the near part of the beam, where the dust shows (a speck right at the lens is
      // out of focus, and fades in from 0.8 m)
      u.uCenter.value.copy(cam).addScaledVector(f, 1.6);
      u.uBeamPos.value[0].copy(L.position);
      u.uBeamDir.value[0].subVectors(L.target.position, L.position).normalize();
      u.uBeamK.value[0] = k; u.uBeamK.value[1] = 0;
      // (the flashlight's beam inside its cookie's corona, core/Flashlight.ts)
      const cone = (L.userData.cone as [number, number] | undefined) ?? [L.angle, L.angle * (1 - L.penumbra)];
      u.uCos.value.set(Math.cos(cone[0]), Math.cos(cone[1]));
      u.uFall.value = 0.12; u.uSize.value = 0.02; u.uHead.value = 0; u.uNear.value = 0.8;
      // out in the open a breeze carries it (and a few gnats); indoors it hangs in the air
      if (o.out) { F.drift(dt, _v.copy(WIND).multiplyScalar(0.35)); u.uWobble.value = 0.05; u.uDensity.value = 0.75; }
      else { F.drift(dt, _v.set(0.01, 0.006, -0.004)); u.uWobble.value = 0.09; u.uDensity.value = 1; }
      (u.uCol.value as THREE.Color).setRGB(1, 0.9, 0.75).multiplyScalar(1.6);
      F.points.geometry.setDrawRange(0, low ? F.n >> 1 : F.n);
    }
    // the truck's headlights: on gravel and dirt fine specks of the dust it raises, a few at a
    // steady speed; when it slows and stops the cloud it trails catches up with it and hangs in
    // the near part of the beam, low over the road, and settles. On the highway now and then a
    // bug in the beam.
    const T = o.truck, H = this.head, h = H.u;
    const loose = !!T && T.kind !== 'asphalt', sp = T ? Math.abs(T.speed) : 0;
    const want = loose ? THREE.MathUtils.clamp(sp / 10, 0, 1) : 0;
    this.specks += (want - this.specks) * Math.min(1, dt * (want > this.specks ? 0.8 : 0.3));
    const slowing = dt > 0 && T ? Math.max(0, this.lastSpeed - sp) : 0;
    this.lastSpeed = sp;
    this.cloud = THREE.MathUtils.clamp(this.cloud + (loose ? slowing * 0.11 * Math.min(1, this.specks * 1.5 + 0.3) : 0) - dt * 0.09 * (0.4 + this.cloud), 0, 1);
    if (!T) this.cloud = 0;
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
      // (the pool on the road is about its width a dozen metres out: a cone of 12 degrees)
      h.uCos.value.set(Math.cos(0.21), Math.cos(0.09)); h.uHead.value = 1; h.uNear.value = 0.8;
      h.uFall.value = 0.004;
      const c = this.cloud, s = this.specks;
      h.uDensity.value = Math.max(0.04, 0.22 * s + 0.7 * c);
      h.uSize.value = 0.03;
      h.uPuff.value = 1 - 0.16 * c; h.uPuffSize.value = 2.0; h.uPuffA.value = 0.13;
      h.uWobble.value = 0.12 + 0.2 * c;
      H.drift(dt, _v.copy(WIND).multiplyScalar(0.6).addScaledVector(fw, 1.2 * c).setY(0.1 * c));
      (h.uCol.value as THREE.Color).setRGB(1, 0.9, 0.72).multiplyScalar(1 - 0.3 * c);
      H.points.geometry.setDrawRange(0, low ? H.n >> 1 : H.n);
    }
  }

  // ---------- the tests and the developer menu ----------
  lampK(id: LifeArea) { return this.areas[id]?.moths?.lamps.map((L) => THREE.MathUtils.clamp(L.k(), 0, 1)) ?? []; }
  tumbleMesh() { return (this.weedArea && this.areas[this.weedArea]?.weed?.mesh) || this.areas.saro?.weed?.mesh || null; }
  stats() {
    const a = (id: LifeArea) => {
      const A = this.areas[id], M = A?.moths;
      return { lamps: M?.lamps.length ?? 0, porch: M?.lamps.filter((L) => L.wall).length ?? 0, moths: M?.n.moths ?? 0, low: M?.n.low ?? 0, resting: M?.n.resting ?? 0,
        bats: M?.n.bats ?? 0, drawn: M?.drawn.moths ?? 0, batsDrawn: M?.drawn.bats ?? 0, visible: !!M?.points.visible,
        weeds: A?.resting.reduce((n, R) => n + (R.mesh.visible ? 1 : 0), 0) ?? 0, wallMs: Math.round(A?.wallMs ?? 0) };
    };
    const W = this.tumbleMesh(), A = this.weedArea ? this.areas[this.weedArea] : undefined, weed = A?.weed;
    const c = this.car, toMap = this.here ? this.areas[this.here]?.toMap : null, cam = this.last?.cam.position;
    let carDist = 0;
    if (c && toMap && cam) { const m = toMap(cam.x, cam.z); carDist = Math.hypot(COUNTY(c.z) - m.x, c.z - m.z); }
    return {
      saro: a('saro'), road: a('road'), station01: a('station01'), activity: this.act,
      motes: { flash: this.flash.points.visible, flashN: this.flash.points.geometry.drawRange.count, flashMax: this.flash.n, head: this.head.points.visible, specks: this.specks, cloud: this.cloud },
      far: { car: !!c, carAt: c ? [COUNTY(c.z), c.z] : [0, 0], carDist, lights: this.here ? this.areas[this.here]?.far?.['win'].length ?? 0 : 0 },
      tumble: { on: !!weed?.on, at: W ? [W.position.x, W.position.y, W.position.z] : [0, 0, 0], ground: W && weed ? weed.ground(W.position.x, W.position.z) : 0, spin: weed?.spin ?? 0 },
    };
  }
}
const _up = new THREE.Vector3(0, 1, 0), _v = new THREE.Vector3();
