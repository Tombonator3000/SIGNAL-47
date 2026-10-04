import * as THREE from 'three';
import { rng } from '../core/textures';
import { artTexture } from '../core/art';

// Band of the Milky Way runs diagonally across the north view, as in the concept art.
const A = new THREE.Vector3(-0.62, 0.22, -0.75).normalize();
const B = new THREE.Vector3(0.5, 0.78, -0.36).normalize();
const BAND_N = new THREE.Vector3().crossVectors(A, B).normalize();
const CORE = new THREE.Vector3(-0.25, 0.42, -0.87).normalize();

export class Sky {
  group = new THREE.Group();
  uniforms = {
    uTime: { value: 0 },
    uFlash: { value: 0 },
    uFlashDir: { value: new THREE.Vector3(1, 0.05, -0.3).normalize() },
    uBandN: { value: BAND_N },
    uCore: { value: CORE },
  };
  // Halley's comet, April 1986: low in the south-southwest, a smudge with a short tail.
  // After its closest approach on the 11th it stood in the south around midnight and set
  // a little after three (HISTORIE.md, KAPITLER.md). It cannot have been in a 1947 sky.
  comet: THREE.Mesh;
  private flashT = 0;
  private nextFlash = 9;
  onThunder?: (delay: number, strength: number) => void;

  constructor() {
    const dome = new THREE.Mesh(
      new THREE.SphereGeometry(3000, 48, 24),
      new THREE.ShaderMaterial({
        uniforms: { ...this.uniforms, uPanorama: { value: artTexture('sky') } },
        side: THREE.BackSide, depthWrite: false, fog: false,
        vertexShader: /* glsl */`
          varying vec3 vDir;
          void main() {
            vDir = normalize(position);
            vec4 p = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
            gl_Position = p.xyww;
          }`,
        fragmentShader: /* glsl */`
          uniform float uTime, uFlash; uniform vec3 uFlashDir, uBandN, uCore;
          uniform sampler2D uPanorama;
          varying vec3 vDir;
          float hash(vec3 p) { p = fract(p * 0.3183099 + 0.1); p *= 17.0; return fract(p.x * p.y * p.z * (p.x + p.y + p.z)); }
          float noise(vec3 x) {
            vec3 i = floor(x), f = fract(x); f = f * f * (3.0 - 2.0 * f);
            return mix(mix(mix(hash(i), hash(i + vec3(1,0,0)), f.x), mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
                       mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x), mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
          }
          float fbm(vec3 p) { float a = 0.5, s = 0.0; for (int i = 0; i < 5; i++) { s += a * noise(p); p *= 2.07; a *= 0.5; } return s; }
          void main() {
            vec3 d = normalize(vDir);
            float h = d.y;
            vec3 col = mix(vec3(0.030, 0.034, 0.060), vec3(0.006, 0.009, 0.024), smoothstep(-0.02, 0.55, h));
            // warm glow low on the horizon (site lights, distant towns)
            col += vec3(0.30, 0.15, 0.06) * exp(-max(h, 0.0) * 22.0) * 0.30;
            // Milky Way
            float b = dot(d, uBandN);
            float band = exp(-b * b / 0.022);
            float core = exp(-b * b / 0.0045);
            float along = dot(d, uCore);
            float bulge = 0.35 + 1.4 * smoothstep(0.2, 1.0, along);
            float c1 = fbm(d * 3.5 + 2.0);
            float c2 = fbm(d * 11.0 - 4.0);
            float dust = smoothstep(0.48, 0.72, fbm(d * 6.0 + vec3(7.0, 1.0, 3.0)));
            vec3 mwCol = mix(vec3(0.30, 0.27, 0.42), vec3(0.62, 0.46, 0.40), c2 * bulge * 0.6);
            float lum = (band * c1 * 0.65 + core * c2 * 0.9) * bulge;
            lum *= 1.0 - dust * core * 0.9;
            col += mwCol * lum * 0.20 * smoothstep(-0.02, 0.18, h);
            // The source is a full-sphere equirectangular map, horizon at v=0.5.
            // sRGB textures are decoded to linear values by WebGL's sampler.
            vec2 skyUV = vec2(atan(d.z, d.x) / 6.2831853 + 0.5, asin(clamp(d.y, -1.0, 1.0)) / 3.1415927 + 0.5);
            vec3 painted = texture2D(uPanorama, skyUV).rgb;
            col = mix(col, painted * 1.7, 0.72 * smoothstep(-0.03, 0.14, h));
            // distant lightning inside a storm on the horizon
            float fa = max(dot(d, uFlashDir), 0.0);
            col += vec3(0.55, 0.6, 0.85) * uFlash * pow(fa, 18.0) * smoothstep(0.35, 0.0, h) * 1.4;
            col += vec3(0.25, 0.28, 0.4) * uFlash * pow(fa, 4.0) * 0.12;
            if (h < 0.0) col *= 0.5;
            gl_FragColor = vec4(col, 1.0);
            #include <tonemapping_fragment>
            #include <colorspace_fragment>
          }`,
      }),
    );
    dome.userData.noAO = true;   // no surface for the occlusion pass (core/vhs.ts)
    dome.renderOrder = -10;
    dome.frustumCulled = false;
    this.group.add(dome);
    this.group.add(this.makeStars());
    this.comet = this.makeComet();
    this.group.add(this.comet);
  }

  private makeComet() {
    const c = document.createElement('canvas'); c.width = 64; c.height = 192;
    const g = c.getContext('2d')!;
    const tail = g.createLinearGradient(0, 170, 0, 0);
    tail.addColorStop(0, 'rgba(200,220,255,.55)'); tail.addColorStop(1, 'rgba(200,220,255,0)');
    g.fillStyle = tail; g.beginPath(); g.moveTo(28, 170); g.lineTo(14, 0); g.lineTo(50, 0); g.lineTo(36, 170); g.fill();
    const head = g.createRadialGradient(32, 168, 0, 32, 168, 16);
    head.addColorStop(0, 'rgba(255,255,255,1)'); head.addColorStop(0.35, 'rgba(220,232,255,.55)'); head.addColorStop(1, 'rgba(200,220,255,0)');
    g.fillStyle = head; g.fillRect(0, 140, 64, 52);
    const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace;
    const m = new THREE.Mesh(new THREE.PlaneGeometry(16, 48), new THREE.MeshBasicMaterial({ map: tex, transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false, toneMapped: false, opacity: 0.8 }));
    // az 200 (north is -Z, east +X), 7 degrees up, 2600 m out; the tail leans up and west
    const az = 200 * Math.PI / 180, el = 7 * Math.PI / 180, R = 2600;
    m.position.set(Math.sin(az) * Math.cos(el) * R, Math.sin(el) * R + 14, -Math.cos(az) * Math.cos(el) * R);
    m.lookAt(0, m.position.y, 0);
    m.rotateZ(0.5);
    m.userData.noAO = true;
    m.renderOrder = -8;
    m.frustumCulled = false;
    return m;
  }
  /** The comet is up from the evening until it sets a little after three in the morning. */
  setCometClock(clock: number) {
    const s = ((clock % 86400) + 86400) % 86400;
    this.comet.visible = s > 20 * 3600 || s < 3 * 3600 + 5 * 60;
  }

  private makeStars() {
    const r = rng(47);
    const N = 5200;
    const pos = new Float32Array(N * 3), size = new Float32Array(N), col = new Float32Array(N * 3), ph = new Float32Array(N);
    const v = new THREE.Vector3();
    for (let i = 0; i < N; i++) {
      // a third of the stars cluster towards the galactic band
      for (;;) {
        v.set(r() * 2 - 1, r() * 2 - 1, r() * 2 - 1);
        const l = v.length();
        if (l < 0.05 || l > 1) continue;
        v.divideScalar(l);
        if (v.y < -0.02) continue;
        if (i % 3 === 0) { const b = v.dot(BAND_N); if (Math.abs(b) > 0.12 + r() * 0.1) continue; }
        break;
      }
      pos.set([v.x * 2800, v.y * 2800, v.z * 2800], i * 3);
      const m = Math.pow(r(), 6);
      size[i] = 1.0 + m * 4.5;
      const t = r();
      const c = t < 0.15 ? [1, 0.8, 0.65] : t < 0.3 ? [0.75, 0.85, 1] : [1, 0.97, 0.92];
      const br = 0.35 + m * 1.4;
      col.set([c[0] * br, c[1] * br, c[2] * br], i * 3);
      ph[i] = r() * 100;
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('aSize', new THREE.BufferAttribute(size, 1));
    g.setAttribute('aCol', new THREE.BufferAttribute(col, 3));
    g.setAttribute('aPh', new THREE.BufferAttribute(ph, 1));
    const mat = new THREE.ShaderMaterial({
      uniforms: { uTime: this.uniforms.uTime, uPR: { value: Math.min(window.devicePixelRatio, 2) } },
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false,
      vertexShader: /* glsl */`
        attribute float aSize; attribute vec3 aCol; attribute float aPh;
        uniform float uTime, uPR; varying vec3 vCol;
        void main() {
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          gl_Position = projectionMatrix * mv;
          float tw = 0.75 + 0.25 * sin(uTime * (1.3 + fract(aPh) * 2.5) + aPh);
          float horizon = smoothstep(0.0, 0.12, normalize(position).y);
          vCol = aCol * tw * (0.35 + 0.65 * horizon);
          gl_PointSize = aSize * uPR;
          gl_Position.z = gl_Position.w * 0.99999;
        }`,
      fragmentShader: /* glsl */`
        varying vec3 vCol;
        void main() {
          vec2 c = gl_PointCoord - 0.5;
          float a = smoothstep(0.5, 0.0, length(c));
          gl_FragColor = vec4(vCol * a, a);
        }`,
    });
    const pts = new THREE.Points(g, mat);
    pts.frustumCulled = false;
    pts.renderOrder = -9;
    return pts;
  }

  update(dt: number, t: number, follow: THREE.Vector3) {
    this.group.position.copy(follow);
    this.uniforms.uTime.value = t;
    this.nextFlash -= dt;
    if (this.nextFlash <= 0) {
      this.flashT = 1;
      this.nextFlash = 14 + Math.random() * 26;
      const a = -0.2 + Math.random() * 1.6;
      this.uniforms.uFlashDir.value.set(Math.sin(a), 0.04, -Math.cos(a)).normalize();
      this.onThunder?.(3 + Math.random() * 5, 0.4 + Math.random() * 0.6);
    }
    if (this.flashT > 0) {
      this.flashT = Math.max(0, this.flashT - dt * 2.2);
      const flick = this.flashT > 0.55 ? (Math.sin(t * 90) > 0 ? 1 : 0.3) : this.flashT;
      this.uniforms.uFlash.value = flick * this.flashT;
    } else this.uniforms.uFlash.value = 0;
  }
}
