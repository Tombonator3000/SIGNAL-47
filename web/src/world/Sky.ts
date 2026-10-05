import * as THREE from 'three';
import { rng } from '../core/textures';
import { artImage } from '../core/art';

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
  private pixelRatio = { value: 1 };
  private flashT = 0;
  private nextFlash = 9;
  onThunder?: (delay: number, strength: number) => void;

  constructor() {
    const dome = new THREE.Mesh(
      new THREE.SphereGeometry(3000, 48, 24),
      new THREE.ShaderMaterial({
        uniforms: { ...this.uniforms, uPanorama: { value: this.milkyWay() } },
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
            // The source is a full-sphere equirectangular map, horizon at v=0.5, with its
            // painted stars taken out (milkyWay()); the stars are the points of makeStars().
            // sRGB textures are decoded to linear values by WebGL's sampler.
            vec2 skyUV = vec2(atan(d.z, d.x) / 6.2831853 + 0.5, asin(clamp(d.y, -1.0, 1.0)) / 3.1415927 + 0.5);
            vec3 painted = texture2D(uPanorama, skyUV).rgb;
            col = mix(col, painted * 2.0, 0.72 * smoothstep(-0.03, 0.14, h));
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

  // The painted sky (round 1) is a 4K panorama shown at 2K. Seen at full screen its stars
  // came out as soft blobs several pixels wide (Tom: too big, low resolution). Here the
  // painted stars are taken out and only the Milky Way is kept: a grey opening (the darkest
  // value within two pixels, then the brightest within one) removes anything smaller than
  // about four pixels and leaves the band, its glow and its dust lanes. Only the sky half is
  // filtered. The band's brightness is kept for makeStars(), so the sharp stars thicken
  // along it as the real ones do.
  private band: { w: number; h: number; lum: Float32Array } | null = null;
  /** How long taking the painted stars out took (tools/sky.py reports it). */
  filterMs = 0;
  private milkyWay() {
    const t0 = performance.now();
    const W = 2048, H = 1024, ROWS = 540;
    const c = document.createElement('canvas'); c.width = W; c.height = H;
    const g = c.getContext('2d', { willReadFrequently: true })!;
    g.drawImage(artImage('sky'), 0, 0, W, H);
    const img = g.getImageData(0, 0, W, ROWS), px = img.data;
    // The opening runs on brightness alone (a third of the work of doing each colour);
    // each pixel is then darkened by what the opening took away, so the band keeps its colour.
    const N = W * ROWS;
    const lum = new Uint8Array(N), a = new Uint8Array(N), b = new Uint8Array(N);
    for (let i = 0, o = 0; i < N; i++, o += 4) lum[i] = (px[o] * 77 + px[o + 1] * 150 + px[o + 2] * 29) >> 8;
    // a min or max filter of radius r along x (wrapping round: it is a panorama) or along y
    const pass = (src: Uint8Array, dst: Uint8Array, r: number, alongX: boolean, min: boolean) => {
      for (let y = 0; y < ROWS; y++) {
        const row = y * W;
        for (let x = 0; x < W; x++) {
          let v = src[row + x];
          for (let k = 1; k <= r; k++) {
            let p: number, q: number;
            if (alongX) { p = src[row + (x + k < W ? x + k : x + k - W)]; q = src[row + (x - k >= 0 ? x - k : x - k + W)]; }
            else { p = src[(y + k < ROWS ? y + k : ROWS - 1) * W + x]; q = src[(y - k >= 0 ? y - k : 0) * W + x]; }
            if (min) { if (p < v) v = p; if (q < v) v = q; } else { if (p > v) v = p; if (q > v) v = q; }
          }
          dst[row + x] = v;
        }
      }
    };
    pass(lum, a, 2, true, true); pass(a, b, 2, false, true);
    pass(b, a, 1, true, false); pass(a, b, 1, false, false);
    for (let i = 0, o = 0; i < N; i++, o += 4) {
      if (b[i] >= lum[i]) continue;
      const k = b[i] / Math.max(1, lum[i]);
      px[o] *= k; px[o + 1] *= k; px[o + 2] *= k;
    }
    g.putImageData(img, 0, 0);
    // the band's brightness at a quarter size, for the stars
    const bw = W / 4, bh = ROWS / 4, band = new Float32Array(bw * bh);
    let max = 0;
    for (let y = 0; y < bh; y++) for (let x = 0; x < bw; x++) {
      const l = b[y * 4 * W + x * 4];
      band[y * bw + x] = l; if (l > max) max = l;
    }
    for (let i = 0; i < band.length; i++) band[i] /= max || 1;
    this.band = { w: bw, h: bh, lum: band };
    this.filterMs = performance.now() - t0;
    const tex = new THREE.CanvasTexture(c);
    tex.name = 'art/sky (stars removed)';
    tex.colorSpace = THREE.SRGBColorSpace;
    tex.wrapS = THREE.RepeatWrapping;
    tex.generateMipmaps = false; tex.minFilter = THREE.LinearFilter;
    return tex;
  }
  /** How bright the Milky Way is in direction d (0 to 1), from the filtered panorama. */
  private bandAt(d: THREE.Vector3) {
    const b = this.band;
    if (!b) return Math.exp(-(d.dot(BAND_N) ** 2) / 0.022);
    const u = Math.atan2(d.z, d.x) / (Math.PI * 2) + 0.5, v = Math.asin(THREE.MathUtils.clamp(d.y, -1, 1)) / Math.PI + 0.5;
    // the canvas has the zenith at the top; v = 1 is the zenith
    const x = Math.min(b.w - 1, Math.floor(u * b.w)), y = Math.floor((1 - v) * 1024 / 4);
    return y >= 0 && y < b.h ? b.lum[y * b.w + x] : 0;
  }

  // Sharp stars: most of them a single pixel and told apart by brightness, not size, as in
  // a real desert sky; only the few brightest are larger, with a tight core. Their number
  // follows the Milky Way. The point size follows the renderer's pixel ratio (update()).
  private makeStars() {
    const r = rng(47);
    const N = 14000;
    const pos = new Float32Array(N * 3), size = new Float32Array(N), col = new Float32Array(N * 3), ph = new Float32Array(N);
    const v = new THREE.Vector3();
    for (let i = 0; i < N; i++) {
      for (;;) {
        v.set(r() * 2 - 1, r() * 2 - 1, r() * 2 - 1);
        const l = v.length();
        if (l < 0.05 || l > 1) continue;
        v.divideScalar(l);
        if (v.y < -0.02) continue;
        // four times as many stars in the brightest part of the band as in the open sky
        if (r() * 1.0 > 0.25 + 0.75 * Math.min(1, this.bandAt(v) * 1.6)) continue;
        break;
      }
      pos.set([v.x * 2800, v.y * 2800, v.z * 2800], i * 3);
      // brightness: many faint stars, some medium, and a few dozen bright ones a little larger
      const q = r();
      const big = q > 0.996 ? 2.8 : q > 0.982 ? 2.0 : 1.0;
      const br = 0.28 + 0.7 * Math.pow(r(), 3) + (big > 1 ? 0.5 + (big - 2) * 0.6 : 0);
      size[i] = big;
      const t = r();
      const c = t < 0.12 ? [1, 0.83, 0.68] : t < 0.3 ? [0.8, 0.88, 1] : [1, 0.98, 0.94];
      col.set([c[0] * br, c[1] * br, c[2] * br], i * 3);
      ph[i] = r() * 100;
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('aSize', new THREE.BufferAttribute(size, 1));
    g.setAttribute('aCol', new THREE.BufferAttribute(col, 3));
    g.setAttribute('aPh', new THREE.BufferAttribute(ph, 1));
    const mat = new THREE.ShaderMaterial({
      uniforms: { uTime: this.uniforms.uTime, uPR: this.pixelRatio },
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false,
      vertexShader: /* glsl */`
        attribute float aSize; attribute vec3 aCol; attribute float aPh;
        uniform float uTime, uPR; varying vec3 vCol; varying float vSharp;
        void main() {
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          gl_Position = projectionMatrix * mv;
          float up = normalize(position).y;
          // stars twinkle more low down, where the light goes through more air
          float amp = mix(0.3, 0.08, smoothstep(0.0, 0.35, up));
          float tw = 1.0 - amp * (0.5 + 0.5 * sin(uTime * (1.7 + fract(aPh) * 3.0) + aPh));
          float horizon = smoothstep(0.0, 0.12, up);
          vCol = aCol * tw * (0.3 + 0.7 * horizon);
          gl_PointSize = max(1.0, aSize * uPR);
          vSharp = aSize > 1.2 ? 1.0 : 0.0;
          gl_Position.z = gl_Position.w * 0.99999;
        }`,
      fragmentShader: /* glsl */`
        varying vec3 vCol; varying float vSharp;
        void main() {
          vec2 c = gl_PointCoord - 0.5;
          // a single pixel is the star; larger ones get a tight core, as bright as a
          // small star in the middle so the light spread over more pixels is not lost
          float a = vSharp > 0.5 ? exp(-dot(c, c) * 9.0) * 2.4 : 1.0;
          gl_FragColor = vec4(vCol * a, a);
        }`,
    });
    const pts = new THREE.Points(g, mat);
    pts.frustumCulled = false;
    pts.renderOrder = -9;
    return pts;
  }

  update(dt: number, t: number, follow: THREE.Vector3, pixelRatio = 1) {
    this.group.position.copy(follow);
    this.pixelRatio.value = pixelRatio;
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
