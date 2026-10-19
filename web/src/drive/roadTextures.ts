import * as THREE from 'three';
import { canvasTex, rng } from '../core/textures';
import { artImage } from '../core/art';
import { dataTwin } from '../core/textures';
import type { Rect } from './truckTextures';

// Canvas textures for the drive. The art supplies asphalt, desert and enamel; every word
// and every road marking is drawn here in code.

const PX = 512 / 12; // highway texture: 12 m across (gravel shoulders included), as many pixels a metre along
const HT = 2048;      // 48 m to a repeat

/** Three times, a repeat apart along v: what crosses the top or the bottom of the picture goes on
 *  at the other end, so the highway has no seam every 48 m. */
const wrapV = (g: CanvasRenderingContext2D, fn: () => void) => { for (const dy of [-HT, 0, HT]) { g.save(); g.translate(0, dy); fn(); g.restore(); } };
/** A crack: a random walk from (x0, y0) to (x1, y1) in steps of about `step` pixels. */
function crackPath(g: CanvasRenderingContext2D, r: () => number, x0: number, y0: number, x1: number, y1: number, step: number, j: number) {
  const n = Math.max(2, Math.round(Math.hypot(x1 - x0, y1 - y0) / step));
  const nx = -(y1 - y0), ny = x1 - x0, L = Math.hypot(nx, ny);
  let o = 0;
  g.beginPath(); g.moveTo(x0, y0);
  for (let i = 1; i <= n; i++) {
    const t = i / n; o = o * 0.7 + (r() - 0.5) * j;
    g.lineTo(x0 + (x1 - x0) * t + nx / L * o, y0 + (y1 - y0) * t + ny / L * o);
  }
}
/** Polygon cracks in an ellipse (alligator cracking in a wheel path). */
function alligator(g: CanvasRenderingContext2D, r: () => number, cx: number, cy: number, rx: number, ry: number, cell: number) {
  const nx = Math.ceil(rx * 2 / cell) + 1, ny = Math.ceil(ry * 2 / cell) + 1, pts: [number, number][] = [];
  for (let j = 0; j < ny; j++) for (let i = 0; i < nx; i++) pts.push([cx - rx + i * cell + (r() - 0.5) * cell * 0.8, cy - ry + j * cell + (r() - 0.5) * cell * 0.8]);
  const inside = ([x, y]: [number, number]) => ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1 - r() * 0.3;
  for (let j = 0; j < ny; j++) for (let i = 0; i < nx; i++) {
    const a = pts[j * nx + i];
    if (!inside(a)) continue;
    for (const [di, dj] of [[1, 0], [0, 1]]) {
      if (i + di >= nx || j + dj >= ny || r() < 0.15) continue;
      const b = pts[(j + dj) * nx + i + di];
      if (!inside(b)) continue;
      g.lineWidth = 0.8 + r() * 1.2; g.beginPath(); g.moveTo(a[0], a[1]); g.lineTo(b[0], b[1]); g.stroke();
    }
  }
}

// Two-lane state highway, 48 m per repeat, forty years of New Mexico on it: double yellow centre
// line and white edge lines, worn and cut by the cracks; paved shoulders, gravel beyond, the
// edge of the asphalt broken up into it. Cracks across it every few metres where the cold opens
// it in winter, most of them sealed with tar long ago, the paving joint along the middle sealed
// in stretches, alligator cracking in the wheel paths, three patches of blacker asphalt with
// their edges sealed, the wheel paths polished darker and the oil between them. The tar, the
// patches and the oil are what the damp sheen catches (roadSurface.wet: the dark of the picture).
// One picture for the road, SARO's stretch of it and the old road's area (cached).
let hwy: THREE.CanvasTexture | null = null;
export function highwayTex() {
  if (hwy) return hwy;
  const W = 512, X = (m: number) => W / 2 + m * PX;
  const layer = () => { const c = document.createElement('canvas'); c.width = W; c.height = HT; return [c, c.getContext('2d')!] as const; };
  const r = rng(64);
  // the paint, worn: specks and longer gaps where it has gone
  const [paint, p] = layer();
  const line = (m: number, w: number, col: string) => { p.fillStyle = col; p.fillRect(X(m) - w * PX / 2, 0, w * PX, HT); };
  line(-3.6, 0.12, '#d8d6cc'); line(3.6, 0.12, '#d8d6cc');
  line(-0.1, 0.1, '#d0a032'); line(0.1, 0.1, '#d0a032');
  p.globalCompositeOperation = 'destination-out';
  for (let i = 0; i < 5200; i++) { p.fillStyle = `rgba(0,0,0,${0.3 + r() * 0.7})`; p.fillRect(r() * W, r() * HT, 1 + r() * 3, 1 + r() * 5); }
  for (let i = 0; i < 40; i++) {   // gone in stretches, a line at a time
    const m = [-3.6, -0.1, 0.1, 3.6][Math.floor(r() * 4)];
    p.fillStyle = `rgba(0,0,0,${0.3 + r() * 0.5})`; p.fillRect(X(m) - 4, r() * HT, 8, (0.2 + r() * 1.4) * PX);
  }
  // the tar: sealed cracks across, the joint along the middle, the patches' edges (smoother in
  // Ultra's twin). Sealer is a narrow core in the crack and a wider, thinner band squeegeed over it.
  const [tar, t] = layer();
  t.lineCap = 'round'; t.lineJoin = 'round';
  const seal = (w: number) => {
    t.strokeStyle = 'rgba(8,8,8,.32)'; t.lineWidth = w + 4; t.stroke();
    t.strokeStyle = 'rgba(4,4,4,.62)'; t.lineWidth = w; t.stroke();
  };
  // the open cracks, drawn over the paint (they cut it)
  const [open, o] = layer();
  o.strokeStyle = 'rgba(4,4,4,.85)'; o.lineCap = 'round'; o.lineJoin = 'round';
  for (let y = r() * 4 * PX; y < HT; y += (4 + r() * 5) * PX) {
    const sealed = r() < 0.6, one = r() < 0.25, left = r() < 0.5;
    const a = one && !left ? 0.1 : -4.7, b = one && left ? -0.1 : 4.7, y1 = y + (r() - 0.5) * 0.8 * PX, seed = r() * 1e6, w = sealed ? 2 + r() * 2 : 1.2 + r() * 0.8;
    wrapV(sealed ? t : o, () => {
      crackPath(sealed ? t : o, rng(seed), X(a), y, X(b), y1, 6, 12);
      if (sealed) seal(w); else { o.lineWidth = w; o.stroke(); }
    });
  }
  // fine cracks branching off here and there, open
  for (let i = 0; i < 70; i++) {
    const x = X(-4.6 + r() * 9.2), y = r() * HT, a = r() * Math.PI * 2, len = (0.3 + r() * 1.2) * PX, seed = r() * 1e6;
    wrapV(o, () => { o.lineWidth = 0.9; crackPath(o, rng(seed), x, y, x + Math.cos(a) * len, y + Math.sin(a) * len, 5, 5); o.stroke(); });
  }
  // the paving joint along the middle, just off the centre line: sealed in stretches, open between
  for (let y = 0; y < HT;) {
    const len = (3 + r() * 9) * PX, sealed = r() < 0.6, seed = r() * 1e6, x = X(0.26 + (r() - 0.5) * 0.08);
    wrapV(sealed ? t : o, () => {
      const rr = rng(seed);
      crackPath(sealed ? t : o, rr, x, y, x + (rr() - 0.5) * 4, y + len, 8, 4);
      if (sealed) seal(2 + rr() * 1.5); else { o.lineWidth = 1.4; o.stroke(); }
    });
    y += len + r() * 2 * PX;
  }
  // an edge crack a little in from each paved edge, in stretches
  for (const s of [-1, 1]) for (let y = 0; y < HT;) {
    const len = (2 + r() * 8) * PX, seed = r() * 1e6, x = X(s * (4.4 + r() * 0.15));
    if (r() < 0.6) wrapV(o, () => { const rr = rng(seed); o.lineWidth = 1.2 + rr(); crackPath(o, rr, x, y, x, y + len, 8, 5); o.stroke(); });
    y += len + r() * 6 * PX;
  }
  // three patches: a lane, a wheel path, a short one by the edge line; skewed off the lanes, a
  // corner chipped, one newer and blacker, one sun-faded lighter than the road round it; the seam
  // a fine line of tar, and a crack from the road carried on across the first
  const patches = ([[0.4, 3.3, 3.5 + r() * 2.5, false], [-2.6 - r() * 0.4, -1.4, 2.6 + r() * 2, true], [2.9, 4.6, 1.6 + r() * 1.4, false]] as const).map(([m0, m1, len, old]) => {
    const y = r() * HT, l = len * PX, sk = (r() - 0.5) * 0.5 * PX, dx = (r() - 0.5) * 0.2 * PX, c = 0.1 + r() * 0.12;
    const q: [number, number][] = [[X(m0), y], [X(m1), y + sk], [X(m1) + dx, y + sk + l], [X(m0) + dx, y + l]];
    // the chipped corner (the second): cut off by a few centimetres
    const pts: [number, number][] = [q[0], [q[1][0] + (q[0][0] - q[1][0]) * c, q[1][1] + (q[0][1] - q[1][1]) * c], [q[1][0] + (q[2][0] - q[1][0]) * c * 0.5, q[1][1] + (q[2][1] - q[1][1]) * c * 0.5], q[2], q[3]];
    return { pts, old, y, l, m0, m1 };
  });
  const patchPath = (g: CanvasRenderingContext2D, pts: [number, number][]) => { g.beginPath(); pts.forEach(([x, y], i) => (i ? g.lineTo(x, y) : g.moveTo(x, y))); g.closePath(); };
  for (const q of patches) wrapV(t, () => { patchPath(t, q.pts); t.strokeStyle = 'rgba(6,6,6,.25)'; t.lineWidth = 1; t.stroke(); });
  { const q = patches[0], y = q.y + q.l * 0.45, y1 = y + (r() - 0.5) * 6, seed = r() * 1e6; wrapV(o, () => { o.lineWidth = 1.2; crackPath(o, rng(seed), X(q.m0 - 0.6), y, X(q.m1 + 0.4), y1, 6, 8); o.stroke(); }); }

  const tex = canvasTex(W, HT, (g, w, h) => {
    const asphalt = artImage('asphalt'), desert = artImage('desert');
    for (let y = 0; y < h; y += 256) for (let x = 0; x < w; x += 256) g.drawImage(asphalt, x, y, 256, 256);
    // the picture again at another size over it, so its 6 m repeat does not show down the road
    g.globalAlpha = 0.35;
    for (let y = -150; y < h; y += 341) for (let x = -90; x < w; x += 341) g.drawImage(asphalt, x, y, 341, 341);
    g.globalAlpha = 1;
    g.fillStyle = 'rgba(40,40,44,.25)'; g.fillRect(0, 0, w, h);
    // slow patches: bleached by the sun, darker where it is newer or damper
    for (let i = 0; i < 30; i++) {
      const x = X(-4.5 + r() * 9), y = r() * h, rx = (1.2 + r() * 3.5) * PX, ry = rx * (1 + r() * 1.5), light = r() < 0.55;
      wrapV(g, () => {
        g.save(); g.translate(x, y); g.scale(1, ry / rx);
        const gr = g.createRadialGradient(0, 0, 0, 0, 0, rx);
        gr.addColorStop(0, light ? 'rgba(150,146,138,.08)' : 'rgba(0,0,0,.1)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
        g.fillStyle = gr; g.beginPath(); g.arc(0, 0, rx, 0, 7); g.fill(); g.restore();
      });
    }
    // the wheel paths polished darker, the oil down the middle of each lane
    const band = (m: number, half: number, a: number) => {
      const gr = g.createLinearGradient(X(m - half), 0, X(m + half), 0);
      gr.addColorStop(0, 'rgba(0,0,0,0)'); gr.addColorStop(0.5, `rgba(10,10,10,${a})`); gr.addColorStop(1, 'rgba(0,0,0,0)');
      g.fillStyle = gr; g.fillRect(X(m - half), 0, 2 * half * PX, h);
    };
    for (const m of [-1.8, 1.8]) { band(m - 0.85, 0.3, 0.12); band(m + 0.85, 0.3, 0.12); band(m, 0.6, 0.22); }
    // the patches: newer asphalt, blacker and finer, or old and bleached by the sun
    for (const q of patches) wrapV(g, () => {
      g.save(); patchPath(g, q.pts); g.clip();
      g.drawImage(asphalt, X(q.m0) - 40, q.y - 60, 300, 300);
      g.fillStyle = q.old ? 'rgba(160,154,144,.16)' : 'rgba(12,12,14,.3)'; g.fillRect(0, q.y - PX, w, q.l + 2 * PX);
      if (q.old) { const rr = rng(q.y); for (let k = 0; k < 900; k++) { g.fillStyle = 'rgba(170,164,152,.35)'; g.fillRect(X(q.m0) + rr() * (q.m1 - q.m0) * PX, q.y - PX + rr() * (q.l + 2 * PX), 1.2, 1.2); } }
      g.restore();
    });
    // two spray patches of chip seal: lighter stones in tar
    for (let i = 0; i < 2; i++) {
      const cx = X(-3 + r() * 6), cy = r() * h, rx = (0.8 + r()) * PX, ry = (1.5 + r() * 2) * PX;
      wrapV(g, () => {
        g.fillStyle = 'rgba(18,18,20,.28)'; g.beginPath(); g.ellipse(cx, cy, rx, ry, 0, 0, 7); g.fill();
        const rr = rng(i + 9);
        for (let k = 0; k < 500; k++) { const a = rr() * 7, d = Math.sqrt(rr()); g.fillStyle = `rgba(${140 + rr() * 40},${134 + rr() * 36},${120 + rr() * 30},.5)`; g.fillRect(cx + Math.cos(a) * d * rx, cy + Math.sin(a) * d * ry, 1.5, 1.5); }
      });
    }
    // alligator cracking in the wheel paths, mostly the outer ones
    g.strokeStyle = 'rgba(6,6,6,.58)'; g.lineCap = 'round';
    for (let i = 0; i < 5; i++) {
      const m = (r() < 0.5 ? -1 : 1) * (r() < 0.7 ? 2.65 : 0.95), cy = r() * h, ry = (1.2 + r() * 2.6) * PX, seed = r() * 1e6;
      wrapV(g, () => alligator(g, rng(seed), X(m), cy, 0.55 * PX, ry, 11));
    }
    g.drawImage(paint, 0, 0);
    g.drawImage(open, 0, 0);
    g.globalAlpha = 0.18; g.drawImage(open, 1, 1); g.globalAlpha = 1;   // a darker lip where the edges have spalled
    g.drawImage(tar, 0, 0);
    // gravel shoulders, and the edge of the asphalt broken up into them: a smoothed random walk in
    // steps of 2 to 3 px (back to where it began at the end of the picture, so it has no seam),
    // rounded bites out of it here and there, a dark lip where the asphalt has spalled, and its
    // crumbs loose in the gravel
    for (const s of [-1, 1]) {
      const e = s < 0 ? 1.2 * PX : w - 1.2 * PX, x0 = s < 0 ? 0 : w - 1.6 * PX, out = s < 0 ? 0 : w;
      const walk: [number, number][] = [];
      for (let y = 0, o = 0, v = 0; y <= h; y += 2 + r()) {
        v = v * 0.8 + (r() - 0.5) * 0.9; o = Math.max(-2, Math.min(0.14 * PX, o * 0.985 + v));
        walk.push([e - s * o * Math.min(1, (h - y) / 80), y]);
      }
      walk.push([e, h]);
      const bites: [number, number, number][] = [];
      for (let y = 30 + r() * 60; y < h - 30; y += 30 + r() * 150) bites.push([e - s * r() * 3, y, 4 + r() * 8]);
      const gravel = () => {
        g.drawImage(desert, 0, 0, 256, 1024, x0, 0, 1.6 * PX, h / 2); g.drawImage(desert, 0, 0, 256, 1024, x0, h / 2, 1.6 * PX, h / 2);
        g.fillStyle = 'rgba(70,64,56,.35)'; g.fillRect(0, 0, w, h);
      };
      g.strokeStyle = 'rgba(10,10,10,.38)'; g.lineWidth = 2; g.lineJoin = 'round';
      g.beginPath(); walk.forEach(([x, y], i) => (i ? g.lineTo(x - s * 1.4, y) : g.moveTo(x - s * 1.4, y))); g.stroke();
      g.beginPath(); for (const [bx, by, br] of bites) { g.moveTo(bx + br + 1.4, by); g.arc(bx, by, br + 1.4, 0, Math.PI * 2); } g.stroke();
      g.save(); g.beginPath(); g.moveTo(out, 0); walk.forEach(([x, y]) => g.lineTo(x, y)); g.lineTo(out, h); g.closePath(); g.clip(); gravel(); g.restore();
      g.save(); g.beginPath(); for (const [bx, by, br] of bites) { g.moveTo(bx + br, by); g.arc(bx, by, br, 0, Math.PI * 2); } g.clip(); gravel(); g.restore();
      for (let i = 0; i < 900; i++) {
        const d = r() * r() * 0.45 * PX + 1, y = r() * h, sz = 1.2 + r() * 2.6, crumb = r() < 0.45;
        g.fillStyle = crumb ? `rgba(${30 + r() * 18},${30 + r() * 18},${32 + r() * 18},.75)` : `rgba(${r() < 0.5 ? '64,60,56' : '146,134,114'},.7)`;
        g.fillRect(e + s * d - sz / 2, y, sz, sz * (0.7 + r() * 0.6));
      }
    }
  }, [1, 1]); // wraps: v runs along the whole highway
  // Ultra (core/ultra.ts): the same layout from the round 9 maps, the paint and the tar flat and smoother
  dataTwin(tex, W, HT, (g, w, h, k) => {
    const asphalt = artImage(k === 'n' ? 'asphaltN' : 'asphaltR'), desert = artImage(k === 'n' ? 'desertN' : 'desertR');
    for (let y = 0; y < h; y += 256) for (let x = 0; x < w; x += 256) g.drawImage(asphalt, x, y, 256, 256);
    for (const x0 of [0, w - 1.2 * PX]) { g.drawImage(desert, 0, 0, 256, 1024, x0, 0, 1.2 * PX, h / 2); g.drawImage(desert, 0, 0, 256, 1024, x0, h / 2, 1.2 * PX, h / 2); }
    for (const [src, rough] of [[paint, 'rgb(140,140,140)'], [tar, 'rgb(64,64,64)']] as const) {
      const [flat, f] = layer();
      f.drawImage(src, 0, 0); f.globalCompositeOperation = 'source-in';
      f.fillStyle = k === 'n' ? 'rgb(128,128,255)' : rough; f.fillRect(0, 0, w, h);
      g.drawImage(flat, 0, 0);
    }
  });
  return (hwy = tex);
}

// Graded caliche gravel: Codex's track with two wheel ruts (round 6, 4 m across, ruts along
// the picture's height), laid twice per 8 m repeat on the desert, with edges that fade into
// the plain desert. The track ribbon is 6 m across (soft edges included), so the picture
// spans the 4.6 m of firm track. World loads the picture (loadArtFor) before the road.
export function gravelTex() {
  const draw = (g: CanvasRenderingContext2D, w: number, h: number, desert: HTMLImageElement, track: HTMLImageElement, marks: boolean) => {
    g.drawImage(desert, 0, 0, w, 512); g.drawImage(desert, 0, 512, w, 512);
    const mid = document.createElement('canvas'); mid.width = w; mid.height = h;
    const m = mid.getContext('2d')!;
    m.drawImage(desert, 0, 0, w, 512); m.drawImage(desert, 0, 512, w, 512);
    const x0 = Math.round(w * 0.11), x1 = Math.round(w * 0.89);
    m.drawImage(track, x0, 0, x1 - x0, 512); m.drawImage(track, x0, 512, x1 - x0, 512);
    if (marks) {
      // the last trucks out to the station: tread in the ruts (a block every 7 cm, dusted over in
      // places), a second, narrower set wandering in and out of them, and dried mud where the
      // water stood in the low places, cracked as it dried
      const r = rng(1947), P = w / 6;
      for (const f of [0.32, 0.68]) {
        const cx = x0 + (x1 - x0) * f;
        for (const [off, tw, a] of [[0, 0.21, 0.13], [0.11, 0.17, 0.07]] as const) {
          let dust = 0;
          for (let y = 0; y < h; y += 0.07 * P) {
            if (r() < 0.03) dust = 6 + r() * 30;
            if (dust > 0) { dust--; continue; }
            const x = cx + (off + Math.sin(y / h * Math.PI * 2 * (1 + f)) * 0.04) * P * (f < 0.5 ? -1 : 1);
            m.fillStyle = `rgba(42,34,26,${a * (0.6 + r() * 0.6)})`;
            m.fillRect(x - tw * P / 2, y, tw * P * (0.4 + r() * 0.15), 0.025 * P);
            m.fillRect(x + tw * P * 0.05, y + 0.035 * P, tw * P * (0.4 + r() * 0.15), 0.025 * P);
          }
        }
        for (let i = 0; i < 2; i++) {
          const y = r() * h, ry = (0.4 + r() * 0.7) * P, rx = 0.26 * P;
          for (const dy of [-h, 0, h]) {
            m.fillStyle = 'rgba(70,56,42,.3)'; m.beginPath(); m.ellipse(cx, y + dy, rx, ry, 0, 0, 7); m.fill();
            m.strokeStyle = 'rgba(40,30,22,.45)'; m.lineWidth = 1;
            for (let k = 0; k < 14; k++) { const a = r() * 7, b = r() * 7; m.beginPath(); m.moveTo(cx + Math.cos(a) * rx * 0.8, y + dy + Math.sin(a) * ry * 0.8); m.lineTo(cx + Math.cos(b) * rx * 0.3, y + dy + Math.sin(b) * ry * 0.3); m.stroke(); }
          }
        }
      }
    }
    // fade the gravel out over the soft edges and a little more (the outer fifth each side; the
    // track's vertex colours go over to the desert's there too, roadTerrain.edgeColour)
    m.globalCompositeOperation = 'destination-in';
    const fade = m.createLinearGradient(0, 0, w, 0);
    fade.addColorStop(0, 'rgba(0,0,0,0)'); fade.addColorStop(0.08, 'rgba(0,0,0,.25)'); fade.addColorStop(0.2, 'rgba(0,0,0,1)');
    fade.addColorStop(0.8, 'rgba(0,0,0,1)'); fade.addColorStop(0.92, 'rgba(0,0,0,.25)'); fade.addColorStop(1, 'rgba(0,0,0,0)');
    m.fillStyle = fade; m.fillRect(0, 0, w, h);
    g.drawImage(mid, 0, 0);
  };
  const tex = canvasTex(512, 1024, (g, w, h) => draw(g, w, h, artImage('desert'), artImage('gravelTrack'), true), [1, 1]);
  dataTwin(tex, 512, 1024, (g, w, h, k) => draw(g, w, h, artImage(k === 'n' ? 'desertN' : 'desertR'), artImage(k === 'n' ? 'gravelTrackN' : 'gravelTrackR'), false));
  return tex;
}

// ---------- signs: all faces in one atlas, one draw call ----------
const AW = 1024, AH = 1024;
const rect = (x: number, y: number, w: number, h: number): Rect => [x / AW, 1 - (y + h) / AH, (x + w) / AW, 1 - y / AH];
export const SIGNS = {
  station: rect(0, 0, 512, 320),
  speed: rect(512, 0, 256, 320),
  mile: rect(768, 0, 128, 256),
  roswell: rect(0, 320, 640, 240),
  gate: rect(640, 320, 384, 192),
  closed: rect(0, 560, 512, 192),
  stripes: rect(512, 560, 512, 128),
  mile111: rect(896, 0, 128, 256),
  marker: rect(0, 760, 80, 240),
  mailbox: rect(96, 760, 256, 64),
};

// The blank enamel sign (art 'sign') stretched to any card, corners kept (as textures.ts does).
function enamel(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number) {
  const im = artImage('sign'), inset = 40, d = inset * Math.min(w / im.width, h / im.height);
  const sx = [0, inset, im.width - inset, im.width], sy = [0, inset, im.height - inset, im.height];
  const dx = [x, x + d, x + w - d, x + w], dy = [y, y + d, y + h - d, y + h];
  for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++) g.drawImage(im, sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j], dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]);
}
// Forty years of sun and the odd rifle: rust streaks, chips, a bullet hole or two.
function weather(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, seed: number, holes: number) {
  const r = rng(seed);
  for (let i = 0; i < 7; i++) {
    const sx = x + r() * w, sy = y + r() * h * 0.6, len = 20 + r() * 60;
    const gr = g.createLinearGradient(0, sy, 0, sy + len);
    gr.addColorStop(0, 'rgba(120,60,25,.4)'); gr.addColorStop(1, 'rgba(120,60,25,0)');
    g.fillStyle = gr; g.fillRect(sx, sy, 2 + r() * 4, len);
  }
  for (let i = 0; i < 260; i++) { g.fillStyle = r() < 0.6 ? 'rgba(90,70,50,.35)' : 'rgba(250,248,240,.3)'; g.fillRect(x + r() * w, y + r() * h, 1 + r() * 2, 1 + r() * 2); }
  for (let i = 0; i < holes; i++) {
    const hx = x + w * (0.15 + r() * 0.7), hy = y + h * (0.2 + r() * 0.6);
    g.fillStyle = 'rgba(110,55,20,.45)'; g.beginPath(); g.arc(hx, hy, 9, 0, 7); g.fill();
    g.fillStyle = '#c9c4b4'; g.beginPath(); g.arc(hx, hy, 5, 0, 7); g.fill();
    g.fillStyle = '#121212'; g.beginPath(); g.arc(hx, hy, 3.2, 0, 7); g.fill();
  }
}
function text(g: CanvasRenderingContext2D, s: string, x: number, y: number, size: number, col: string, maxW: number, weight = 500) {
  g.fillStyle = col; g.font = `${weight} ${size}px Oswald`; g.textAlign = 'center'; g.textBaseline = 'middle';
  g.fillText(s, x, y, maxW);
}
function guide(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, seed: number) {
  g.fillStyle = '#1d5a3b'; g.fillRect(x, y, w, h);
  g.strokeStyle = '#e9eee6'; g.lineWidth = Math.max(4, w / 60);
  const i = g.lineWidth * 1.5;
  g.beginPath(); g.roundRect(x + i, y + i, w - 2 * i, h - 2 * i, i * 1.5); g.stroke();
  weather(g, x, y, w, h, seed, 0);
}

export function signAtlas() {
  return canvasTex(AW, AH, (g) => {
    g.fillStyle = '#6a6d70'; g.fillRect(0, 0, AW, AH);
    // STATION 01: the old enamel sign at the turn-off
    enamel(g, 0, 0, 512, 320);
    text(g, 'STATION 01', 256, 84, 92, '#1f2326', 440, 600);
    g.fillStyle = '#1f2326'; g.fillRect(64, 138, 384, 5);
    text(g, 'U.S. SURVEY', 256, 190, 62, '#1f2326', 420);
    text(g, 'NO THROUGH ROAD', 256, 262, 56, '#8f2a1e', 440);
    weather(g, 0, 0, 512, 320, 7, 2);
    // regulatory white
    g.fillStyle = '#ecebe4'; g.fillRect(512, 0, 256, 320);
    g.strokeStyle = '#141414'; g.lineWidth = 8; g.strokeRect(526, 14, 228, 292);
    text(g, 'SPEED', 640, 62, 54, '#141414', 200); text(g, 'LIMIT', 640, 122, 54, '#141414', 200);
    text(g, '55', 640, 222, 140, '#141414', 210, 600);
    weather(g, 512, 0, 256, 320, 8, 0);
    // mile marker
    guide(g, 768, 0, 128, 256, 9);
    text(g, 'MILE', 832, 70, 34, '#e9eee6', 100); text(g, '112', 832, 160, 58, '#e9eee6', 108, 600);
    // distance sign, the first time the name is seen
    guide(g, 0, 320, 640, 240, 10);
    text(g, 'ROSWELL', 250, 440, 104, '#e9eee6', 380); text(g, '64', 540, 440, 104, '#e9eee6', 140);
    weather(g, 0, 320, 640, 240, 11, 1);
    // plate on the station gate
    enamel(g, 640, 320, 384, 192);
    text(g, 'STATION 01', 832, 380, 58, '#1f2326', 330, 600);
    text(g, 'KEEP GATE CLOSED', 832, 452, 36, '#8f2a1e', 330);
    weather(g, 640, 320, 384, 192, 12, 0);
    // the old road, closed until chapter six: a regulatory plate and the barricades' rails
    g.fillStyle = '#ecebe4'; g.fillRect(0, 560, 512, 192);
    g.strokeStyle = '#141414'; g.lineWidth = 8; g.strokeRect(12, 572, 488, 168);
    text(g, 'ROAD CLOSED', 256, 656, 96, '#141414', 450, 600);
    weather(g, 0, 560, 512, 192, 13, 0);
    g.save(); g.beginPath(); g.rect(512, 560, 512, 128); g.clip();
    g.fillStyle = '#f2efe6'; g.fillRect(512, 560, 512, 128);
    g.fillStyle = '#d8661c';
    for (let x = 512 - 128; x < 1024 + 128; x += 96) { g.beginPath(); g.moveTo(x, 688); g.lineTo(x + 48, 688); g.lineTo(x + 176, 560); g.lineTo(x + 128, 560); g.fill(); }
    g.restore();
    weather(g, 512, 560, 512, 128, 14, 0);
    // the next mile marker south, towards Caliche
    guide(g, 896, 0, 128, 256, 15);
    text(g, 'MILE', 960, 70, 34, '#e9eee6', 100); text(g, '111', 960, 160, 58, '#e9eee6', 108, 600);
    // the object marker at a culvert's end: yellow and black down and to the road
    g.save(); g.beginPath(); g.rect(0, 760, 80, 240); g.clip();
    g.fillStyle = '#e2b52c'; g.fillRect(0, 760, 80, 240);
    g.fillStyle = '#161616';
    for (let y = 760 - 80; y < 1000; y += 40) { g.beginPath(); g.moveTo(0, y); g.lineTo(80, y + 50); g.lineTo(80, y + 70); g.lineTo(0, y + 20); g.fill(); }
    g.restore();
    weather(g, 0, 760, 80, 240, 16, 0);
    // the ranch's name painted on its mailbox, by hand
    g.fillStyle = '#8e9192'; g.fillRect(96, 760, 256, 64);
    text(g, 'ALDERETE', 224, 793, 40, '#1d1f22', 230, 600);
    weather(g, 96, 760, 256, 64, 17, 0);
  });
}

// A warm haze on the northern horizon above SARO's floodlights.
export function horizonGlowTex() {
  return canvasTex(256, 128, (g, w, h) => {
    const gr = g.createRadialGradient(w / 2, h, 0, w / 2, h, w / 2);
    gr.addColorStop(0, 'rgba(255,170,90,1)'); gr.addColorStop(0.35, 'rgba(255,130,60,.45)'); gr.addColorStop(1, 'rgba(255,110,40,0)');
    g.save(); g.scale(1, 0.5); g.translate(0, h); g.fillStyle = gr; g.fillRect(0, -h, w, h * 2); g.restore();
  });
}

/** UVs of a plane mapped into one rectangle of an atlas. */
export function uvInto(geo: THREE.BufferGeometry, r: Rect) {
  const uv = geo.attributes.uv as THREE.BufferAttribute;
  for (let i = 0; i < uv.count; i++) uv.setXY(i, r[0] + uv.getX(i) * (r[2] - r[0]), r[1] + uv.getY(i) * (r[3] - r[1]));
  return geo;
}
