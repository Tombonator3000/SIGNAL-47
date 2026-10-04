import * as THREE from 'three';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { GTAOPass } from 'three/examples/jsm/postprocessing/GTAOPass.js';
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass.js';
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass.js';
import { OutputPass } from 'three/examples/jsm/postprocessing/OutputPass.js';

// The picture as if it came off a VHS tape recorded in 1986, the look of late-night
// television and the camcorder footage on "Unsolved Mysteries". One full-screen pass after
// the scene: the scene is drawn into a half-float target at a few hundred lines (the
// tape's resolution, which also saves the GPU work on large screens), then tone mapped
// here, graded (lifted blacks, green-blue shadows, warm lamps, less saturation), with the
// colour smeared sideways the way tape smears chroma, a little red and blue fringing
// towards the edges, a soft glow around bright lamps, faint scanlines, fine noise, a slow
// tracking band and, in the heavy setting, the ragged head-switching line at the bottom.
//
// The tuning follows SPILLDESIGN.md: by default only the grade, the glow, the fringe and the
// grain; the jitter, the tracking band and the head switching belong to the heavy setting
// (and to `glitch`), and never move when the player asks for reduced motion.
// 'off' draws straight to the screen as before. The field camera draws its photographs
// on its own (FieldCamera.expose), so prints never get the tape look.

export type Picture = 'off' | 'vhs' | 'heavy';

const LINES: Record<Exclude<Picture, 'off'>, number> = { vhs: 720, heavy: 480 };

const frag = /* glsl */ `
uniform sampler2D tScene;
uniform vec2 uSize;        // target size in pixels
uniform float uTime;
uniform float uHeavy;      // 0 vhs, 1 heavy
uniform float uGlitch;     // 0..1, extra tracking trouble (the signal, the walkie-talkie)
uniform float uGlowK;      // the tape's own glow; the PC tier has real bloom before it
uniform float uMotion;     // 0 when the player asked for reduced motion
varying vec2 vUv;

// a hash without sin(), which loses precision on large inputs and draws lines in the noise
float hash(vec2 p) { vec3 q = fract(vec3(p.xyx) * 0.1031); q += dot(q, q.yzx + 33.33); return fract((q.x + q.y) * q.z); }

vec3 scene(vec2 uv) {
  vec3 c = texture2D(tScene, uv).rgb;
  return linearToOutputTexel(vec4(toneMapping(c), 1.0)).rgb;
}
// RGB -> YIQ and back: tape keeps luma sharp and smears the colour
const mat3 toYIQ = mat3(0.299, 0.596, 0.211, 0.587, -0.274, -0.523, 0.114, -0.322, 0.312);
const mat3 toRGB = mat3(1.0, 1.0, 1.0, 0.956, -0.272, -1.106, 0.621, -0.647, 1.703);

void main() {
  vec2 uv = vUv;
  float px = 1.0 / uSize.x;
  float line = floor(uv.y * uSize.y);
  float t = uTime;

  // tracking: a soft band that drifts up the screen, lines in it slide sideways
  float band = fract(t * 0.045 + 0.3);
  float trouble = max(uHeavy * 0.7, uGlitch) * uMotion;
  float inBand = smoothstep(0.035, 0.0, abs(uv.y - band)) * trouble;
  float wobble = sin(uv.y * 220.0 + t * 7.0) * 0.18 + (hash(vec2(line, floor(t * 24.0) * 7.0)) - 0.5) * 0.6;
  float shift = (wobble * (0.15 + uHeavy) * uMotion + inBand * 9.0 + uGlitch * uMotion * 6.0 * (hash(vec2(line, floor(t * 30.0) + 500.0)) - 0.5)) * px;
  // head switching: the last few lines of the frame tear sideways (heavy only)
  float head = uHeavy * uMotion * smoothstep(0.022, 0.0, uv.y);
  shift += head * (14.0 + 10.0 * hash(vec2(line, floor(t * 60.0)))) * px;
  uv.x += shift;

  // luma sharp, chroma from a wider sideways average, slightly late (to the right)
  vec2 r = uv - 0.5;
  float fringe = dot(r, r) * (0.0024 + 0.0024 * uHeavy);   // about 1.5 px at the corners, 3 px heavy
  vec3 yiq = toYIQ * scene(uv);
  vec3 cAvg = vec3(0.0);
  for (int i = -3; i <= 3; i++) cAvg += toYIQ * scene(uv + vec2((float(i) * 1.6 + 1.5) * px * (1.0 + uHeavy), 0.0));
  cAvg /= 7.0;
  yiq.yz = cAvg.yz;
  vec3 col = toRGB * yiq;
  // red and blue part at the edges
  col.r = mix(col.r, scene(uv + r * fringe).r, 0.6);
  col.b = mix(col.b, scene(uv - r * fringe).b, 0.6);

  // glow around bright things (lamps, the monitor, the sign)
  vec3 glow = vec3(0.0);
  for (int i = 0; i < 8; i++) {
    float a = float(i) * 0.785398;
    vec3 s = scene(uv + vec2(cos(a), sin(a)) * px * (5.0 + 3.0 * uHeavy) * vec2(1.0, uSize.x / uSize.y));
    glow += max(s - 0.8, 0.0);
  }
  col += glow * (0.25 + 0.1 * uHeavy) * 0.5 * uGlowK;

  // grade: less colour, lifted blacks, green-blue shadows and warm highlights
  float l = dot(col, vec3(0.299, 0.587, 0.114));
  col = mix(vec3(l), col, 0.88 - 0.12 * uHeavy);
  col = mix(col, col * vec3(0.86, 1.02, 1.04), smoothstep(0.45, 0.0, l));
  col = mix(col, col * vec3(1.06, 1.0, 0.9), smoothstep(0.5, 1.0, l));
  col = col * (0.95 - 0.04 * uHeavy) + vec3(0.026, 0.031, 0.033) * (1.0 + uHeavy);   // black sits near 0.03

  // faint scanlines, noise, the band's snow
  col *= 1.0 - (0.02 + 0.05 * uHeavy) * (0.5 + 0.5 * cos(uv.y * uSize.y * 3.14159));
  float n = hash(floor(uv * uSize) + vec2(floor(t * 24.0) * 17.0, floor(t * 24.0) * 31.0));
  col += (n - 0.5) * (0.06 + 0.06 * uHeavy) * 0.5;
  col += inBand * (n - 0.35) * 0.25;
  col += head * (n - 0.3) * 0.5;

  gl_FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}`;

const vert = /* glsl */ `varying vec2 vUv; void main() { vUv = uv; gl_Position = vec4(position.xy, 0.0, 1.0); }`;

// Ambient occlusion that leaves out what has no surface: the sky dome, see-through things
// and the invisible boxes the player aims at (they would leave dark halos otherwise).
class SolidAO extends GTAOPass {
  overrideVisibility() {
    super.overrideVisibility();
    this.scene.traverse((o) => {
      if (!o.visible) return;
      const m = o as THREE.Mesh;
      if (!m.isMesh) return;
      const mat = (Array.isArray(m.material) ? m.material[0] : m.material) as THREE.Material | undefined;
      if (o.userData.noAO || !mat || mat.transparent || mat.visible === false) o.visible = false;
    });
  }
}

export class Vhs {
  picture: Picture = 'vhs';
  glitch = 0;
  private rt: THREE.WebGLRenderTarget | null = null;
  private quad: THREE.Mesh;
  private mat: THREE.ShaderMaterial;
  private cam = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
  private scene2 = new THREE.Scene();
  private size = new THREE.Vector2();
  readonly supported: boolean;

  constructor(private renderer: THREE.WebGLRenderer, private samples = 4) {
    const gl = renderer.getContext();
    this.supported = renderer.capabilities.isWebGL2 && !!(gl.getExtension('EXT_color_buffer_float') || gl.getExtension('EXT_color_buffer_half_float'));
    this.mat = new THREE.ShaderMaterial({
      vertexShader: vert, fragmentShader: frag, depthTest: false, depthWrite: false,
      uniforms: { tScene: { value: null }, uSize: { value: new THREE.Vector2(1, 1) }, uTime: { value: 0 }, uHeavy: { value: 0 }, uGlitch: { value: 0 }, uMotion: { value: 1 }, uGlowK: { value: 1 } },
    });
    this.quad = new THREE.Mesh(new THREE.PlaneGeometry(2, 2), this.mat);
    this.quad.frustumCulled = false;
    this.scene2.add(this.quad);
  }
  get on() { return this.picture !== 'off' && this.supported; }

  // ---------- the PC tier (core/ultra.ts): occlusion and bloom, then the tape ----------
  ultra = false;
  private post: { composer: EffectComposer; ao: SolidAO; bloom: UnrealBloomPass; tape: ShaderPass; out: OutputPass; w: number; h: number; scene: THREE.Scene } | null = null;
  private renderUltra(scene: THREE.Scene, camera: THREE.Camera, t: number) {
    const r = this.renderer;
    r.getDrawingBufferSize(this.size);
    const h = this.on ? this.lines(this.size.y) : Math.min(this.size.y, 1440), w = Math.round(this.size.x * h / this.size.y);
    let p = this.post;
    if (!p || p.scene !== scene) {
      this.disposePost();
      const rt = new THREE.WebGLRenderTarget(w, h, { type: THREE.HalfFloatType, samples: 4 });
      const composer = new EffectComposer(r, rt);
      composer.setPixelRatio(1);
      composer.addPass(new RenderPass(scene, camera));
      const ao = new SolidAO(scene, camera, w, h);
      ao.updateGtaoMaterial({ radius: 0.7, distanceExponent: 1.5, thickness: 1.5, scale: 1.0, samples: 16, distanceFallOff: 1, screenSpaceRadius: false });
      ao.updatePdMaterial({ lumaPhi: 10, depthPhi: 2, normalPhi: 3, radius: 6, rings: 2, samples: 16 });
      ao.blendIntensity = 0.85;
      composer.addPass(ao);
      const bloom = new UnrealBloomPass(new THREE.Vector2(w >> 1, h >> 1), 0.45, 0.5, 0.95);
      composer.addPass(bloom);
      const tape = new ShaderPass(this.mat, 'tScene');
      composer.addPass(tape);
      const out = new OutputPass();
      composer.addPass(out);
      p = this.post = { composer, ao, bloom, tape, out, w: -1, h: -1, scene };
    }
    if (p.w !== w || p.h !== h) { p.composer.setSize(w, h); p.w = w; p.h = h; }
    p.tape.enabled = this.on;
    p.out.enabled = !this.on;
    const u = this.mat.uniforms;
    u.uSize.value.set(w, h);
    u.uTime.value = t;
    u.uHeavy.value = this.picture === 'heavy' ? 1 : 0;
    u.uGlitch.value = this.glitch;
    u.uMotion.value = matchMedia('(prefers-reduced-motion: reduce)').matches ? 0 : 1;
    u.uGlowK.value = 0.35;
    p.composer.render();
  }
  private disposePost() {
    const p = this.post; if (!p) return;
    p.composer.dispose(); p.ao.dispose(); p.bloom.dispose();
    this.post = null;
  }
  /** The height in pixels the scene is drawn at (the glow points need it). */
  lines(bufferH: number) { return this.on ? Math.min(bufferH, LINES[this.picture as 'vhs']) : bufferH; }

  /** Draw the scene through the tape, or straight to the screen when off. */
  render(scene: THREE.Scene, camera: THREE.Camera, t: number) {
    const r = this.renderer;
    if (this.ultra && this.supported) { this.renderUltra(scene, camera, t); return; }
    this.mat.uniforms.uGlowK.value = 1;
    if (!this.on) { r.render(scene, camera); return; }
    r.getDrawingBufferSize(this.size);
    const h = this.lines(this.size.y), w = Math.round(this.size.x * h / this.size.y);
    if (!this.rt || this.rt.width !== w || this.rt.height !== h) {
      this.rt?.dispose();
      this.rt = new THREE.WebGLRenderTarget(w, h, { type: THREE.HalfFloatType, samples: this.samples, colorSpace: THREE.LinearSRGBColorSpace });
    }
    r.setRenderTarget(this.rt);
    r.render(scene, camera);
    r.setRenderTarget(null);
    const u = this.mat.uniforms;
    u.tScene.value = this.rt.texture;
    u.uSize.value.set(w, h);
    u.uTime.value = t;
    u.uHeavy.value = this.picture === 'heavy' ? 1 : 0;
    u.uGlitch.value = this.glitch;
    u.uMotion.value = matchMedia('(prefers-reduced-motion: reduce)').matches ? 0 : 1;
    r.render(this.scene2, this.cam);
  }
  setSamples(n: number) { if (n !== this.samples) { this.samples = n; this.rt?.dispose(); this.rt = null; } }
  dispose() { this.rt?.dispose(); this.disposePost(); this.mat.dispose(); this.quad.geometry.dispose(); }
}
