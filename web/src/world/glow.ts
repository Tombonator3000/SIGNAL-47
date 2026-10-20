import * as THREE from 'three';
import { glowSprite } from '../core/textures';

// Many glowing lamps in one draw call. Each point is a soft additive disc that keeps
// its size in metres, like a sprite, and can blink with its own phase.
// pxScale must follow the drawing buffer height and camera fov (see main.ts resize).
export const glowScale = { value: 400 };

export class GlowPoints {
  points: THREE.Points;
  private pos: number[] = [];
  private col: number[] = [];
  private size: number[] = [];
  private blink: number[] = [];
  private uTime = { value: 0 };
  /** All of them held at one brightness instead of their blink (the array's lamps, story/arrayBack.ts). */
  private uHold = { value: 0 };
  private uLevel = { value: 1 };

  constructor(map: THREE.Texture = glowSprite()) {
    const mat = new THREE.ShaderMaterial({
      uniforms: { uMap: { value: map }, uTime: this.uTime, uPx: glowScale, uHold: this.uHold, uLevel: this.uLevel },
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false,
      vertexShader: /* glsl */`
        attribute vec3 aCol; attribute float aSize; attribute vec2 aBlink;
        uniform float uTime, uPx, uHold, uLevel; varying vec3 vCol;
        void main() {
          vec4 mv = modelViewMatrix * vec4(position, 1.0);
          gl_Position = projectionMatrix * mv;
          float b = aBlink.x > 0.0 ? 0.55 + 0.45 * max(0.0, sin(uTime * aBlink.x + aBlink.y)) : 1.0;
          vCol = aCol * mix(b, uLevel, uHold);
          gl_PointSize = aSize * uPx / max(-mv.z, 0.1);
        }`,
      // same result as an additive SpriteMaterial: colour times texture, weighted by alpha
      fragmentShader: /* glsl */`
        uniform sampler2D uMap; varying vec3 vCol;
        void main() {
          vec4 t = texture2D(uMap, gl_PointCoord);
          gl_FragColor = vec4(vCol * t.rgb, t.a);
          #include <tonemapping_fragment>
          #include <colorspace_fragment>
        }`,
    });
    this.points = new THREE.Points(new THREE.BufferGeometry(), mat);
    this.points.frustumCulled = false;
    this.points.renderOrder = 2;
  }

  // size is the glow diameter in metres; blinkRate 0 means steady
  add(x: number, y: number, z: number, size: number, color: THREE.ColorRepresentation = 0xffffff, blinkRate = 0, phase = 0) {
    const c = new THREE.Color(color);
    this.pos.push(x, y, z); this.col.push(c.r, c.g, c.b); this.size.push(size); this.blink.push(blinkRate, phase);
    return this.size.length - 1;
  }

  build() {
    const g = this.points.geometry;
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('aCol', new THREE.Float32BufferAttribute(this.col, 3));
    g.setAttribute('aSize', new THREE.Float32BufferAttribute(this.size, 1));
    g.setAttribute('aBlink', new THREE.Float32BufferAttribute(this.blink, 2));
    return this.points;
  }

  setPosition(i: number, p: THREE.Vector3) {
    const a = this.points.geometry.getAttribute('position') as THREE.BufferAttribute;
    a.setXYZ(i, p.x, p.y, p.z); a.needsUpdate = true;
  }

  /** Where in its blink a point is (radians); its rate stays. */
  setPhase(i: number, phase: number) {
    const a = this.points.geometry.getAttribute('aBlink') as THREE.BufferAttribute | undefined;
    if (!a || a.getY(i) === phase) return;
    a.setY(i, phase); a.needsUpdate = true;
  }

  /** Hold every point at one brightness (0 dark, 1 its full colour, more for a flash), or let
   *  them blink again (null). */
  hold(level: number | null) { this.uHold.value = level === null ? 0 : 1; if (level !== null) this.uLevel.value = level; }
  get held() { return this.uHold.value ? this.uLevel.value : null; }

  update(t: number) { this.uTime.value = t; }
}
