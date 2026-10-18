import * as THREE from 'three';

// Light in the air (todo.md, the PC list: "Lys i lufta"): faint cones of sodium light under the
// lamps, and a low haze lying on the open ground at night. High and Ultra only; Low has neither.
// Both are cheap on purpose (a phone runs High): all the cones of an area are one mesh, drawn
// additively without writing depth, and the haze is two big flat layers (one on a phone) that
// follow the camera, one draw call together. Nothing here is a light: the cones and the haze
// only add a little colour where the air would catch it, fade with the scene's own fog, keep
// out of the player's eyes up close and die away towards dawn. The floodlights once turned the
// night into day (memory.md); these stay well under them.
//
// The lamps are read from an area's glow points (world/glow.ts): the sodium heads high on their
// poles, by colour, size and height, so a lamp that goes out (Caliche's at dawn) takes its cone
// with it, and the areas' own files need not know about any of this.

export interface AirLamp { x: number; y: number; z: number; ground: number; r: number; col: THREE.Color; k: () => number }

/** Sodium heads among an area's glows: warm, high, the size of a lamp's own glow. */
export function sodiumGlows(root: THREE.Object3D, ok: (x: number, y: number, z: number, size: number, c: THREE.Color) => boolean, ground: (x: number, z: number) => number, reach: (h: number) => number) {
  const out: AirLamp[] = [];
  root.traverse((o) => {
    const pts = o as THREE.Points;
    if (!pts.isPoints) return;
    const pos = pts.geometry.getAttribute('position'), col = pts.geometry.getAttribute('aCol'), size = pts.geometry.getAttribute('aSize');
    if (!pos || !col || !size) return;
    for (let i = 0; i < pos.count; i++) {
      const c = new THREE.Color(col.getX(i), col.getY(i), col.getZ(i)), x = pos.getX(i), y = pos.getY(i), z = pos.getZ(i);
      // sodium (the colours are linear: 0xffb45a is 1, 0.46, 0.10), high on a pole
      if (!(c.r > 0.85 && c.g > 0.3 && c.g < 0.6 && c.b < 0.25) || !ok(x, y, z, size.getX(i), c)) continue;
      const gy = ground(x, z), k0 = c.r;
      // lit as long as its glow is (Town.ts darkens a lamp's glow when it goes off)
      out.push({ x, y: y - 0.15, z, ground: gy, r: reach(y - gy), col: c.clone().multiplyScalar(1 / k0), k: () => col.getX(i) / k0 });
    }
  });
  return out;
}

const CONE_VS = /* glsl */`
  attribute float aV; attribute float aLamp;
  uniform float uK[LAMPS];
  varying float vV; varying vec3 vCol; varying vec3 vN; varying vec3 vView; varying float vFogDepth;
  attribute vec3 aCol;
  void main() {
    vec4 wp = modelMatrix * vec4(position, 1.0);
    vec4 mv = viewMatrix * wp;
    gl_Position = projectionMatrix * mv;
    vV = aV;
    vCol = aCol * uK[int(aLamp + 0.5)];
    vN = normalize(mat3(modelMatrix) * normal);
    vView = cameraPosition - wp.xyz;
    vFogDepth = -mv.z;
  }`;
const CONE_FS = /* glsl */`
  uniform float uGain;
  varying float vV; varying vec3 vCol; varying vec3 vN; varying vec3 vView; varying float vFogDepth;
  #ifdef USE_FOG
    uniform float fogDensity;
  #endif
  void main() {
    float dist = length(vView);
    // soft at the cone's edge (seen side on, the air is thin), strongest just under the lamp,
    // gone before the ground (no line where the cone meets it), and not right in the eyes
    float face = abs(dot(normalize(vN), vView / max(dist, 1e-3)));
    float along = smoothstep(0.0, 0.1, vV) * pow(1.0 - vV, 1.8);
    float near = smoothstep(0.8, 6.0, dist);
    float a = face * face * along * near * uGain;
    #ifdef USE_FOG
      a *= exp(-fogDensity * fogDensity * vFogDepth * vFogDepth);
    #endif
    gl_FragColor = vec4(vCol * a, 1.0);
    #include <tonemapping_fragment>
    #include <colorspace_fragment>
  }`;

/** The cones under an area's lamps: one mesh, a child of the area's group (it hides with it).
 *  Lamps in the group's own terms. */
export class LightCones {
  readonly mesh: THREE.Mesh;
  private readonly k: number[];
  private readonly mat: THREE.ShaderMaterial;
  constructor(readonly lamps: AirLamp[], parent: THREE.Object3D) {
    const n = Math.max(1, lamps.length);
    const geos: THREE.BufferGeometry[] = [];
    lamps.forEach((L, i) => {
      const h = Math.max(1, L.y - L.ground);
      const g = new THREE.CylinderGeometry(0.4, L.r, h, 18, 1, true);
      g.translate(L.x, L.y - h / 2, L.z);
      const p = g.getAttribute('position'), cnt = p.count;
      const v = new Float32Array(cnt), id = new Float32Array(cnt).fill(i), c = new Float32Array(cnt * 3);
      // the air scatters it paler than the lamp's own orange
      const k = L.col.clone().lerp(PALE, 0.45);
      for (let j = 0; j < cnt; j++) { v[j] = THREE.MathUtils.clamp((L.y - p.getY(j)) / h, 0, 1); c.set([k.r, k.g, k.b], j * 3); }
      g.setAttribute('aV', new THREE.BufferAttribute(v, 1));
      g.setAttribute('aLamp', new THREE.BufferAttribute(id, 1));
      g.setAttribute('aCol', new THREE.BufferAttribute(c, 3));
      g.deleteAttribute('uv');
      geos.push(g);
    });
    const geo = geos.length ? mergeAll(geos) : new THREE.BufferGeometry();
    this.k = new Array(n).fill(0);
    this.mat = new THREE.ShaderMaterial({
      uniforms: { ...THREE.UniformsUtils.clone(THREE.UniformsLib.fog), uK: { value: this.k }, uGain: { value: 0 } },
      defines: { LAMPS: n }, vertexShader: CONE_VS, fragmentShader: CONE_FS,
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide, fog: true,
    });
    this.mesh = new THREE.Mesh(geo, this.mat);
    this.mesh.name = 'air:cones';
    this.mesh.renderOrder = 1;
    this.mesh.raycast = () => {};   // only light: never hit (the interaction, the ground under the feet in the tests)
    parent.add(this.mesh);
    this.mesh.visible = false;
  }
  update(on: boolean, gain: number) {
    this.mesh.visible = on && gain > 0.002 && this.lamps.length > 0;
    if (!this.mesh.visible) return;
    this.mat.uniforms.uGain.value = gain;
    this.lamps.forEach((L, i) => { this.k[i] = THREE.MathUtils.clamp(L.k(), 0, 1); });
  }
}

function mergeAll(geos: THREE.BufferGeometry[]) {
  // the cones share their attributes: indices offset by each one's vertices
  const keys = ['position', 'normal', 'aV', 'aLamp', 'aCol'];
  let verts = 0, idx = 0;
  for (const g of geos) { verts += g.getAttribute('position').count; idx += g.index!.count; }
  const out = new THREE.BufferGeometry(), index = new Uint32Array(idx);
  const arrays = keys.map((k) => new Float32Array(verts * geos[0].getAttribute(k).itemSize));
  let v0 = 0, i0 = 0;
  for (const g of geos) {
    keys.forEach((k, a) => arrays[a].set(g.getAttribute(k).array as Float32Array, v0 * g.getAttribute(k).itemSize));
    const ix = g.index!;
    for (let j = 0; j < ix.count; j++) index[i0 + j] = ix.getX(j) + v0;
    v0 += g.getAttribute('position').count; i0 += ix.count;
  }
  keys.forEach((k, a) => out.setAttribute(k, new THREE.BufferAttribute(arrays[a], geos[0].getAttribute(k).itemSize)));
  out.setIndex(new THREE.BufferAttribute(index, 1));
  out.computeBoundingSphere();
  return out;
}

const PALE = new THREE.Color(1, 0.72, 0.42);

// ---------- the haze on the ground ----------
const HAZE_LAMPS = 4;
const HAZE_VS = /* glsl */`
  attribute float aLayer;
  varying vec3 vW; varying float vLayer; varying float vFogDepth;
  void main() {
    vec4 wp = modelMatrix * vec4(position, 1.0);
    vec4 mv = viewMatrix * wp;
    gl_Position = projectionMatrix * mv;
    vW = wp.xyz; vLayer = aLayer; vFogDepth = -mv.z;
  }`;
const HAZE_FS = /* glsl */`
  uniform vec3 uCol; uniform float uK; uniform float uTime; uniform float uR;
  uniform vec4 uLamp[${HAZE_LAMPS}]; uniform vec3 uLampCol[${HAZE_LAMPS}];
  varying vec3 vW; varying float vLayer; varying float vFogDepth;
  #ifdef USE_FOG
    uniform float fogDensity;
  #endif
  void main() {
    vec3 d = vW - cameraPosition;
    float dist = length(d);
    // a thin layer seen from above: little straight down, more the flatter it is looked through
    float sinA = abs(d.y) / max(dist, 1e-3);
    float a = 1.0 - exp(-0.012 / max(sinA, 0.03));
    // it drifts in slow patches, and fades out well before the edge of its sheet
    vec2 q = vW.xz * 0.045 + vec2(uTime * 0.011, uTime * -0.007) + vLayer * 3.1;
    float n = 0.62 + 0.22 * sin(q.x * 1.7 + sin(q.y * 1.3)) * sin(q.y * 1.1 + 0.6 * sin(q.x * 0.9)) + 0.16 * sin(q.x * 3.1 - q.y * 2.3);
    // and not round the player's feet, where it would only grey the ground
    a *= n * (1.0 - smoothstep(uR * 0.55, uR, length(d.xz))) * smoothstep(5.0, 30.0, dist);
    vec3 col = uCol;
    // under a lamp the haze takes its colour
    float lit = 0.0;
    for (int i = 0; i < ${HAZE_LAMPS}; i++) {
      vec2 l = vW.xz - uLamp[i].xz;
      float w = exp(-dot(l, l) / max(uLamp[i].w, 1e-3));
      col += uLampCol[i] * w; lit += w * dot(uLampCol[i], vec3(0.33));
    }
    a = a * uK * (1.0 + 2.5 * min(lit, 1.0));
    #ifdef USE_FOG
      a *= exp(-fogDensity * fogDensity * vFogDepth * vFogDepth * 0.5);
    #endif
    gl_FragColor = vec4(col, clamp(a, 0.0, 0.12));
    #include <tonemapping_fragment>
    #include <colorspace_fragment>
  }`;

/** Two (or one) flat layers of haze a little above the open ground round the camera. */
export class GroundHaze {
  readonly mesh: THREE.Mesh;
  private readonly mat: THREE.ShaderMaterial;
  private base = NaN;
  private readonly lampPos = Array.from({ length: HAZE_LAMPS }, () => new THREE.Vector4(0, -999, 0, 1));
  private readonly lampCol = Array.from({ length: HAZE_LAMPS }, () => new THREE.Color(0, 0, 0));
  static readonly R = 320;
  constructor(scene: THREE.Scene, layers: number[]) {
    const geo = new THREE.BufferGeometry(), R = GroundHaze.R;
    const pos: number[] = [], layer: number[] = [], index: number[] = [];
    layers.forEach((h, i) => {
      const b = i * 4;
      pos.push(-R, h, -R, R, h, -R, R, h, R, -R, h, R);
      layer.push(i, i, i, i);
      index.push(b, b + 2, b + 1, b, b + 3, b + 2);
    });
    geo.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    geo.setAttribute('aLayer', new THREE.Float32BufferAttribute(layer, 1));
    geo.setIndex(index);
    this.mat = new THREE.ShaderMaterial({
      uniforms: { ...THREE.UniformsUtils.clone(THREE.UniformsLib.fog), uCol: { value: new THREE.Color() }, uK: { value: 0 }, uTime: { value: 0 }, uR: { value: R },
        uLamp: { value: this.lampPos }, uLampCol: { value: this.lampCol } },
      vertexShader: HAZE_VS, fragmentShader: HAZE_FS,
      transparent: true, depthWrite: false, side: THREE.DoubleSide, fog: true,
    });
    this.mesh = new THREE.Mesh(geo, this.mat);
    this.mesh.name = 'air:haze';
    this.mesh.renderOrder = 1;
    this.mesh.raycast = () => {};
    this.mesh.frustumCulled = false;
    this.mesh.visible = false;
    scene.add(this.mesh);
  }
  /** ground: the open land's height under the camera, or null for none here (indoors, another
   *  area, day); lamps: those that colour it, in world terms. */
  update(dt: number, t: number, cam: THREE.Vector3, ground: number | null, k: number, fog: THREE.Color, lamps: { p: THREE.Vector3; r: number; col: THREE.Color; k: number }[]) {
    this.mesh.visible = ground !== null && k > 0.002;
    if (ground === null) { this.base = NaN; return; }
    if (!this.mesh.visible) return;
    // the layers follow the land under the camera, smoothly, so a step up does not lift the haze at once
    this.base = Number.isFinite(this.base) ? this.base + (ground - this.base) * Math.min(1, dt * 1.5) : ground;
    this.mesh.position.set(cam.x, this.base, cam.z);
    const u = this.mat.uniforms;
    // the haze is the night's own colour, a little paler than the fog it sits in
    (u.uCol.value as THREE.Color).copy(fog).multiplyScalar(1.7).add(_lift);
    u.uK.value = k; u.uTime.value = t;
    // the nearest few lamps tint it
    _near.length = 0;
    for (const L of lamps) if (L.k > 0.02) _near.push({ L, d: (L.p.x - cam.x) ** 2 + (L.p.z - cam.z) ** 2 });
    _near.sort((a, b) => a.d - b.d);
    for (let i = 0; i < HAZE_LAMPS; i++) {
      const e = _near[i];
      if (!e) { this.lampPos[i].set(0, -999, 0, 1); this.lampCol[i].setRGB(0, 0, 0); continue; }
      this.lampPos[i].set(e.L.p.x, e.L.p.y, e.L.p.z, e.L.r * e.L.r);
      this.lampCol[i].copy(e.L.col).multiplyScalar(0.16 * e.L.k);
    }
  }
}
const _lift = new THREE.Color(0.012, 0.014, 0.02);
const _near: { L: { p: THREE.Vector3; r: number; col: THREE.Color; k: number }; d: number }[] = [];
