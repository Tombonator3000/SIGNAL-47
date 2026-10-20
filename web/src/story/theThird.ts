import * as THREE from 'three';
import { Walker } from './recording';

// The third one before the credits (FORSLAG.md, 20 October, Valg for Tom 4, a and b; Tom: "Spenning
// og skummelt slår faste regler"). HISTORIE.md has three at the survey equipment in the whole 1947
// photograph after the credits: Nora, Tomás and a third, blurred, with Reyes's silhouette, the SARO
// jacket with the reflective band across the back. Until now that photograph was the only time the
// game showed it, and the empty seat in the pull-out was never pointed at. Tom bends that rule here,
// twice, and the ending stays the one ending:
//
//   FRAME 03  At STATION 01 in chapter three the field camera takes the empty footing and the marked
//             post. When the shutter goes, out in the dark past the fence on C's ground, someone in a
//             SARO jacket stands with their back to the camera, and the band gives the flash back: a
//             short pale line at chest height on something darker than the dark, in the print and
//             nowhere else (R4: it is in the picture from the moment it is exposed, and it never
//             changes). Out on the C stakes' own line when the frame has them; A, which the frame
//             looks along, leaves the transit 45 degrees from C, so most frames do not, and then it
//             stands on C's ground (story/lines.ts: the station's, within 80 m of the transit) as near
//             the stakes as the picture allows.
//             Small and faint: most will not see it until they look closely at the print.
//   05:29:10  On the old road, when the headlights wake in the wrong colours, someone in a SARO jacket
//             stands 9 m in front of the truck with their back to it, looking east along C. The band
//             across the back takes the headlights' colours as they change. It does not move. It is
//             there through the radio playing the night back, and it is gone in the FLASH; the camera
//             then goes out to where it stood and looks back at the empty cab.
//
// Both are the same figure as the one standing where the player was taken and walking their 47
// seconds (story/recording.ts, Walker), baked into one draw.

const _v = new THREE.Vector3(), _w = new THREE.Vector3(), _r = new THREE.Vector3(), _c = new THREE.Color();

/** Someone in a SARO jacket, as one mesh: lit by what the caller says (a light from behind it, the
 *  way it faces away from the viewer), the band given back in the light's own colour. Built facing
 *  -z from the feet, like the player at yaw 0. */
export class ThirdFigure {
  readonly mesh: THREE.Mesh;
  /** The light on it from behind (the headlights, the flash), the light all round (the dawn), the
   *  band's colour, and how much of it is there (1 all; less, and it is let go by a dither). */
  private u = { lit: { value: new THREE.Color(0, 0, 0) }, amb: { value: new THREE.Color(0, 0, 0) }, band: { value: new THREE.Color(0, 0, 0) }, fade: { value: 1 } };
  constructor(name: string) {
    const mat = new THREE.MeshBasicMaterial({ vertexColors: true });
    mat.onBeforeCompile = (sh) => {
      sh.uniforms.uLit = this.u.lit; sh.uniforms.uAmb = this.u.amb; sh.uniforms.uBand = this.u.band; sh.uniforms.uFade = this.u.fade;
      sh.vertexShader = sh.vertexShader
        .replace('#include <common>', '#include <common>\nattribute float aBand;\nattribute float aShade;\nvarying float vBand;\nvarying float vShade;')
        .replace('#include <begin_vertex>', '#include <begin_vertex>\nvBand = aBand; vShade = aShade;');
      sh.fragmentShader = sh.fragmentShader
        .replace('#include <common>', '#include <common>\nuniform vec3 uLit, uAmb, uBand;\nuniform float uFade;\nvarying float vBand;\nvarying float vShade;')
        .replace('#include <clipping_planes_fragment>', `#include <clipping_planes_fragment>
          // let go without blending: an ordered dither over the screen
          if (uFade < 0.999) {
            vec2 q = floor(gl_FragCoord.xy);
            vec2 h = floor(q * 0.5);
            float b = fract(dot(h, vec2(0.5, h.y * 0.75))) * 0.25 + fract(dot(q, vec2(0.5, q.y * 0.75)));
            if (b >= uFade) discard;
          }`)
        .replace('#include <color_fragment>', `#include <color_fragment>
          // the jacket and the jeans in the light from behind them; the tape gives the light back
          diffuseColor.rgb = mix(diffuseColor.rgb * (uAmb + uLit * vShade), uBand * (0.3 + 0.7 * vShade), vBand);`);
    };
    mat.customProgramCacheKey = () => 'thirdFigure';
    this.mesh = new THREE.Mesh(Walker.baked(), mat);
    this.mesh.name = name;
    this.mesh.visible = false;
    this.mesh.frustumCulled = false;
  }
  /** Where it stands (the feet) and which way it faces (yaw: facing -sin, -cos). */
  place(at: THREE.Vector3, yaw: number) { this.mesh.position.copy(at); this.mesh.rotation.set(0, yaw, 0); this.mesh.updateMatrixWorld(); }
  /** lit: the light on its back, amb: the light all round, band: the tape (linear colours). */
  light(lit: THREE.Color, amb: THREE.Color, band: THREE.Color) { this.u.lit.value.copy(lit); this.u.amb.value.copy(amb); this.u.band.value.copy(band); }
  /** 1 all there, 0 gone (in between it is let go by a dither, never blended). */
  set fade(k: number) { this.u.fade.value = THREE.MathUtils.clamp(k, 0, 1); }
  get fade() { return this.u.fade.value; }
  /** The band's colour now (the tests). */
  get bandColor() { return this.u.band.value; }
  /** World points along the band across the back and at the two sleeves, a little towards `eye` so
   *  the jacket never hides a glow put there: five along the chest, then the left and right sleeve. */
  bandPoints(eye: THREE.Vector3, out: THREE.Vector3[] = []) {
    const m = this.mesh;
    m.updateMatrixWorld();
    const local: [number, number][] = [[-0.2, 1.2], [-0.1, 1.2], [0, 1.2], [0.1, 1.2], [0.2, 1.2], [-0.32, 1.18], [0.32, 1.18]];
    local.forEach(([x, y], i) => {
      const p = (out[i] ??= new THREE.Vector3()).set(x, y, 0.16).applyMatrix4(m.matrixWorld);
      _v.copy(eye).sub(p).setY(0).normalize();
      p.addScaledVector(_v, 0.14);
    });
    out.length = local.length;
    return out;
  }
  /** Where its head is (world), for the log and the tests. */
  head(out = new THREE.Vector3()) { return out.set(0, 1.6, 0).applyMatrix4(this.mesh.matrixWorld); }
  dispose() { this.mesh.removeFromParent(); this.mesh.geometry.dispose(); (this.mesh.material as THREE.Material).dispose(); }
}

// ---------- FRAME 03 ----------
/** Where it stands for FRAME 03: how far from the transit (metres; STATION 01's ground within 80 m
 *  of it is C's, story/lines.ts C_GROUND), and the way C's stakes run from it, in degrees north of
 *  them it may stand to be in the picture at all (A, which the frame looks along, leaves the
 *  transit 45 degrees north of C). */
export const FILM = { from: 58, to: 76, step: 3, northMax: 60, southMax: 8, onC: 7 } as const;
export interface FilmIn {
  cam: THREE.PerspectiveCamera;
  /** The transit's pier (world) and the way C's stakes run from it (world, flat). */
  transit: THREE.Vector3; cDir: THREE.Vector3;
  ground: (x: number, z: number) => number;
  /** Nothing in the way between two world points (the hut, the shed, the footing, the land). */
  clear: (from: THREE.Vector3, to: THREE.Vector3) => boolean;
}
export interface FilmShot {
  /** Where the band is in the photograph (u, v from the top left), and whether it stood on the C
   *  stakes' own line (else on C's ground beside it). */
  u: number; v: number; onC: boolean;
  /** Switch it into the scene for the exposure, and out again. */
  before: () => void; after: () => void;
  /** Draw the light the tape gives back onto the raw frame, before the film goes over it. */
  post: (raw: HTMLCanvasElement) => void;
}

/** The third one on FRAME 03: the spot in the frame nearest C's stakes (60 to 76 m out from the
 *  transit, past the fence), or null when none of C's ground is in the picture clear of everything. */
export class FilmThird {
  private fig: ThirdFigure | null = null;
  constructor(private scene: THREE.Scene) {}

  shot(o: FilmIn): FilmShot | null {
    const cam = o.cam;
    cam.updateMatrixWorld();
    const eye = cam.getWorldPosition(new THREE.Vector3());
    const north = new THREE.Vector3(o.cDir.z, 0, -o.cDir.x);   // C's stakes run east: north is to their left
    let best: { at: THREE.Vector3; u: number; v: number; deg: number } | null = null;
    // nearest C's stakes first, then out to the north (and a little to the south) of them
    const degs: number[] = [0];
    for (let d = 2; d <= FILM.northMax; d += 2) { degs.push(d); if (d <= FILM.southMax) degs.push(-d); }
    for (const deg of degs) {
      const a = THREE.MathUtils.degToRad(deg);
      for (let r = FILM.from; r <= FILM.to && !best; r += FILM.step) {
        _w.copy(o.cDir).multiplyScalar(Math.cos(a)).addScaledVector(north, Math.sin(a)).multiplyScalar(r).add(o.transit);
        const at = new THREE.Vector3(_w.x, o.ground(_w.x, _w.z), _w.z);
        const band = at.clone().setY(at.y + 1.2), top = at.clone().setY(at.y + 1.7);
        const p = band.clone().project(cam), q = top.clone().project(cam);
        const u = (p.x + 1) / 2, v = (1 - p.y) / 2;
        const inView = p.z < 1 && q.z < 1 && u > 0.07 && u < 0.93 && v > 0.08 && v < 0.92 && (1 - q.y) / 2 > 0.03;
        if (!inView || !o.clear(eye, band)) continue;
        best = { at, u, v, deg };
      }
      if (best) break;
    }
    if (!best) return null;
    const b = best;
    // back to the camera, looking out the way C runs
    const yaw = Math.atan2(-o.cDir.x, -o.cDir.z);
    const face = Math.atan2(-(b.at.x - eye.x), -(b.at.z - eye.z));
    // (turned towards C's way by up to 35 degrees, never side on to the camera)
    let d = yaw - face; d = Math.atan2(Math.sin(d), Math.cos(d));
    const turn = face + THREE.MathUtils.clamp(d, -0.6, 0.6);
    return {
      u: b.u, v: b.v, onC: Math.abs(b.deg) <= FILM.onC,
      before: () => {
        const f = this.fig ??= new ThirdFigure('third:film');
        f.place(b.at, turn);
        // the flash does not reach it: a shape against the sky at most; the tape alone comes back
        f.light(_c.setRGB(0, 0, 0), new THREE.Color(0.006, 0.007, 0.008), new THREE.Color(0.55, 0.56, 0.52));
        f.fade = 1;
        f.mesh.visible = true;
        this.scene.add(f.mesh);
      },
      after: () => { if (this.fig) { this.fig.mesh.visible = false; this.fig.mesh.removeFromParent(); } },
      post: (raw) => this.glow(raw, cam, b.at, turn),
    };
  }

  /** The tape in the flash, on the raw frame: across the back and the sleeves, which at this
   *  distance run together into one short line, with the halo film gives a hard light. Faint, and a
   *  few pixels across at most. */
  private glow(raw: HTMLCanvasElement, cam: THREE.PerspectiveCamera, at: THREE.Vector3, yaw: number) {
    const g = raw.getContext('2d')!, W = raw.width, H = raw.height;
    const side = _r.set(Math.cos(yaw), 0, -Math.sin(yaw));
    const px = (p: THREE.Vector3) => { const q = p.clone().project(cam); return [(q.x + 1) / 2 * W, (1 - q.y) / 2 * H] as const; };
    const c = at.clone().setY(at.y + 1.2);
    const [cx, cy] = px(c);
    const [lx] = px(c.clone().addScaledVector(side, -0.4)), [rx] = px(c.clone().addScaledVector(side, 0.4));
    const [, ty] = px(c.clone().setY(c.y + 0.05)), [, by] = px(c.clone().setY(c.y - 0.05));
    const half = Math.max(3.6, Math.abs(rx - lx) / 2 + 0.6), tall = Math.max(1.0, Math.abs(by - ty) / 2 + 0.35);
    g.save();
    g.globalCompositeOperation = 'lighter';
    const streak = (x: number, y: number, hw: number, hh: number, a: number) => {
      g.save(); g.translate(x, y); g.scale(hw, hh);
      const gr = g.createRadialGradient(0, 0, 0, 0, 0, 1);
      gr.addColorStop(0, `rgba(226,228,216,${a})`); gr.addColorStop(0.5, `rgba(206,210,200,${a * 0.6})`); gr.addColorStop(1, 'rgba(180,186,180,0)');
      g.fillStyle = gr; g.beginPath(); g.arc(0, 0, 1, 0, Math.PI * 2); g.fill(); g.restore();
    };
    // the halo first, wide and low, then the line, brightest across the back
    streak(cx, cy, half * 1.6, tall * 4.2, 0.22);
    streak(cx, cy, half * 1.08, tall * 1.5, 0.5);
    streak(cx, cy, half * 0.62, tall, 0.55);
    g.restore();
  }
}
