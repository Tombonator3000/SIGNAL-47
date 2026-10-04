import * as THREE from 'three';
import { glowScale } from '../world/glow';

// The SARO field camera. Raising it narrows the view to a 16:10 frame; what is inside
// the frame is exactly what the photograph records. An exposure re-renders the scene
// from the same viewpoint at 960x600 (a long exposure, brighter than the eye sees),
// with any film-only objects switched on for that one render, then gets a film look,
// a paper border and a handwritten caption.

export const PHOTO_W = 960, PHOTO_H = 600;
const BORDER = 28, CAPTION = 92;
export const PRINT = { w: PHOTO_W + BORDER * 2, h: PHOTO_H + BORDER + CAPTION, x: BORDER, y: BORDER };
export const PHOTO_FOV = 44.6; // vertical, degrees: a standard lens

export interface Photo {
  id: 'frame01' | 'frame02';
  subject: string;
  method: 'baseline' | 'passive' | 'active';
  clock: number;                       // game clock (seconds) when exposed
  url: string;                         // the print as a JPEG data URL
  targets: Record<string, [number, number]>; // features in photo UV, origin top left
}

export class FieldCamera {
  have = false;
  raised = false;
  photoCam = new THREE.PerspectiveCamera(PHOTO_FOV, PHOTO_W / PHOTO_H, 0.05, 5000);
  private savedFov = 70;

  constructor(private renderer: THREE.WebGLRenderer, private scene: THREE.Scene, private cam: THREE.PerspectiveCamera) {}

  // The viewfinder frame on screen, in CSS pixels. The overlay is drawn from the same numbers.
  frameRect() {
    const W = innerWidth, H = innerHeight;
    const w = Math.min(W * 0.92, H * 0.84 * 1.6);
    const h = w / 1.6;
    return { x: (W - w) / 2, y: (H - h) / 2, w, h, frac: h / H };
  }

  raise(on: boolean) {
    if (on === this.raised) return;
    this.raised = on;
    if (on) this.savedFov = this.cam.fov;
    else { this.cam.fov = this.savedFov; this.cam.updateProjectionMatrix(); }
  }

  // Called every frame while raised: the screen camera gets the fov that makes the
  // frame show exactly the photo's field of view, and the photo camera follows it.
  sync() {
    const { frac } = this.frameRect();
    if (this.raised) {
      const fov = 2 * Math.atan(Math.tan(PHOTO_FOV * Math.PI / 360) / frac) * 180 / Math.PI;
      if (Math.abs(this.cam.fov - fov) > 0.01) { this.cam.fov = fov; this.cam.updateProjectionMatrix(); }
    }
    this.cam.updateMatrixWorld();
    this.photoCam.position.copy(this.cam.position);
    this.photoCam.quaternion.copy(this.cam.quaternion);
    this.photoCam.updateMatrixWorld();
  }

  // Where a world point lands in the photo: u, v from the top left, and whether it is in front.
  project(p: THREE.Vector3) {
    const v = p.clone().project(this.photoCam);
    const local = p.clone().applyMatrix4(this.photoCam.matrixWorldInverse);
    return { u: (v.x + 1) / 2, v: (1 - v.y) / 2, front: local.z < 0 };
  }

  inFrame(p: THREE.Vector3, x0: number, x1: number, y0: number, y1: number) {
    const r = this.project(p);
    // limits are given like Unity viewport coordinates, y from the bottom
    const yb = 1 - r.v;
    return r.front && r.u >= x0 && r.u <= x1 && yb >= y0 && yb <= y1;
  }

  // Is anything in `occluders` between the camera and p?
  clearLine(p: THREE.Vector3, occluders: THREE.Object3D[]) {
    const o = this.photoCam.position;
    const d = p.clone().sub(o);
    const len = d.length();
    const ray = new THREE.Raycaster(o.clone(), d.normalize(), 0.1, len - 0.3);
    return ray.intersectObjects(occluders, true).length === 0;
  }

  // Renders the photograph. `before`/`after` switch film-only objects on and off;
  // `restore` puts the screen back (size, pixel ratio) and is followed by a normal frame.
  expose(o: { before: () => void; after: () => void; restore: () => void; caption: string; time: string; seed: number }) {
    const r = this.renderer;
    const exp = r.toneMappingExposure, glow = glowScale.value;
    o.before();
    r.setPixelRatio(1);
    r.setSize(PHOTO_W, PHOTO_H, false);
    r.toneMappingExposure = exp * 1.9;                   // long exposure: the film sees more than the eye
    glowScale.value = PHOTO_H / (2 * Math.tan(PHOTO_FOV * Math.PI / 360));
    r.render(this.scene, this.photoCam);
    const raw = document.createElement('canvas');
    raw.width = PHOTO_W; raw.height = PHOTO_H;
    raw.getContext('2d')!.drawImage(r.domElement, 0, 0, PHOTO_W, PHOTO_H);
    r.toneMappingExposure = exp;
    glowScale.value = glow;
    o.after();
    o.restore();
    return makePrint(raw, o.caption, o.time, o.seed);
  }
}

// Film look: lifted blacks, warm cast, grain, vignette. Then the paper print around it.
function makePrint(raw: HTMLCanvasElement, caption: string, time: string, seed: number) {
  const g = raw.getContext('2d')!;
  const img = g.getImageData(0, 0, PHOTO_W, PHOTO_H);
  const d = img.data;
  let s = seed >>> 0;
  const rnd = () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
  for (let y = 0; y < PHOTO_H; y++) {
    for (let x = 0; x < PHOTO_W; x++) {
      const i = (y * PHOTO_W + x) * 4;
      const dx = (x / PHOTO_W - 0.5) * 1.6, dy = y / PHOTO_H - 0.5;
      const vig = 1 - Math.min(0.55, (dx * dx + dy * dy) * 0.9);
      const n = (rnd() - 0.5) * 22;
      // a little less colour than the screen: night film under sodium light
      const lum = d[i] * 0.3 + d[i + 1] * 0.59 + d[i + 2] * 0.11;
      const r = d[i] * 0.78 + lum * 0.22, gg = d[i + 1] * 0.78 + lum * 0.22, b = d[i + 2] * 0.78 + lum * 0.22;
      d[i] = Math.min(255, (10 + r * 1.04) * vig + n);
      d[i + 1] = Math.min(255, (9 + gg * 0.99) * vig + n);
      d[i + 2] = Math.min(255, (12 + b * 0.9) * vig + n);
    }
  }
  g.putImageData(img, 0, 0);
  const p = document.createElement('canvas');
  p.width = PRINT.w; p.height = PRINT.h;
  const q = p.getContext('2d')!;
  q.fillStyle = '#ebe5d3'; q.fillRect(0, 0, PRINT.w, PRINT.h);
  q.drawImage(raw, PRINT.x, PRINT.y);
  q.fillStyle = 'rgba(40,40,40,.25)'; q.fillRect(PRINT.x, PRINT.y + PHOTO_H, PHOTO_W, 2);
  q.fillStyle = '#26324f'; q.font = '46px "Reenie Beanie", cursive';
  q.fillText(caption, PRINT.x + 6, PRINT.y + PHOTO_H + 60);
  q.fillStyle = '#2d2b27'; q.font = '30px "Special Elite", serif'; q.textAlign = 'right';
  q.fillText(time, PRINT.w - PRINT.x - 6, PRINT.y + PHOTO_H + 58);
  return p;
}
