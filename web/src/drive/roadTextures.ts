import * as THREE from 'three';
import { canvasTex, rng } from '../core/textures';
import { artImage } from '../core/art';
import type { Rect } from './truckTextures';

// Canvas textures for the drive. The art supplies asphalt, desert and enamel; every word
// and every road marking is drawn here in code.

const PX = 512 / 12; // highway texture: 12 m across (gravel shoulders included)

// Two-lane state highway, 24 m per repeat: double yellow centre line, white edge lines,
// paved shoulders, gravel beyond, tar snakes over old cracks, worn paint.
export function highwayTex() {
  const paint = document.createElement('canvas'); paint.width = 512; paint.height = 1024;
  const p = paint.getContext('2d')!;
  const r = rng(64);
  const line = (m: number, w: number, col: string) => { p.fillStyle = col; p.fillRect(256 + m * PX - w * PX / 2, 0, w * PX, 1024); };
  line(-3.6, 0.12, '#d8d6cc'); line(3.6, 0.12, '#d8d6cc');
  line(-0.1, 0.1, '#d0a032'); line(0.1, 0.1, '#d0a032');
  p.globalCompositeOperation = 'destination-out';
  for (let i = 0; i < 2600; i++) { p.fillStyle = `rgba(0,0,0,${0.3 + r() * 0.7})`; p.fillRect(r() * 512, r() * 1024, 1 + r() * 3, 1 + r() * 5); }
  for (let i = 0; i < 14; i++) { p.fillStyle = 'rgba(0,0,0,.6)'; p.fillRect(0, r() * 1024, 512, 4 + r() * 20); }
  return canvasTex(512, 1024, (g, w, h) => {
    const asphalt = artImage('asphalt'), desert = artImage('desert');
    for (let y = 0; y < h; y += 256) for (let x = 0; x < w; x += 256) g.drawImage(asphalt, x, y, 256, 256);
    g.fillStyle = 'rgba(40,40,44,.25)'; g.fillRect(0, 0, w, h);
    // oil down the middle of each lane, old patches
    for (const m of [-1.8, 1.8]) {
      const gr = g.createLinearGradient(256 + (m - 0.6) * PX, 0, 256 + (m + 0.6) * PX, 0);
      gr.addColorStop(0, 'rgba(0,0,0,0)'); gr.addColorStop(0.5, 'rgba(10,10,10,.22)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
      g.fillStyle = gr; g.fillRect(256 + (m - 0.6) * PX, 0, 1.2 * PX, h);
    }
    for (let i = 0; i < 5; i++) { g.fillStyle = `rgba(${r() < 0.5 ? '20,20,22' : '120,118,112'},.18)`; g.fillRect(60 + r() * 330, r() * h, 40 + r() * 90, 30 + r() * 120); }
    // gravel shoulders with a ragged edge
    for (const x0 of [0, w - 1.2 * PX]) {
      g.drawImage(desert, 0, 0, 256, 1024, x0, 0, 1.2 * PX, h);
      g.fillStyle = 'rgba(70,64,56,.35)'; g.fillRect(x0, 0, 1.2 * PX, h);
    }
    for (let i = 0; i < 900; i++) {
      const left = r() < 0.5, x = left ? 1.2 * PX + r() * 6 - 3 : w - 1.2 * PX + r() * 6 - 3;
      g.fillStyle = `rgba(${r() < 0.5 ? '60,58,55' : '140,128,110'},.8)`; g.fillRect(x, r() * h, 2 + r() * 3, 2 + r() * 3);
    }
    // tar snakes: crack sealer in black wandering lines
    g.strokeStyle = 'rgba(8,8,8,.75)'; g.lineCap = 'round';
    for (let i = 0; i < 9; i++) {
      let x = 60 + r() * 390, y = r() * h;
      g.lineWidth = 3 + r() * 3; g.beginPath(); g.moveTo(x, y);
      for (let k = 0; k < 12; k++) { x += (r() - 0.5) * 50; y += (r() - 0.3) * 40; g.lineTo(Math.min(w - 55, Math.max(55, x)), y); }
      g.stroke();
    }
    g.drawImage(paint, 0, 0);
  });
}

// Graded caliche gravel: pale, two darker wheel tracks, a dry crown between them, and
// edges that fade into the plain desert. 6 m across, 12 m per repeat.
export function gravelTex() {
  return canvasTex(256, 512, (g, w, h) => {
    const desert = artImage('desert'), r = rng(12);
    g.drawImage(desert, 0, 0, w, 256); g.drawImage(desert, 0, 256, w, 256);
    const mid = document.createElement('canvas'); mid.width = w; mid.height = h;
    const m = mid.getContext('2d')!;
    m.drawImage(desert, 0, 0, w, 256); m.drawImage(desert, 0, 256, w, 256);
    m.fillStyle = 'rgba(205,196,178,.45)'; m.fillRect(0, 0, w, h);
    for (const u of [0.36, 0.64]) {
      const gr = m.createLinearGradient((u - 0.05) * w, 0, (u + 0.05) * w, 0);
      gr.addColorStop(0, 'rgba(90,80,66,0)'); gr.addColorStop(0.5, 'rgba(90,80,66,.38)'); gr.addColorStop(1, 'rgba(90,80,66,0)');
      m.fillStyle = gr; m.fillRect((u - 0.05) * w, 0, 0.1 * w, h);
    }
    for (let i = 0; i < 4200; i++) {
      const v = r();
      m.fillStyle = v < 0.45 ? 'rgba(70,64,55,.7)' : v < 0.9 ? 'rgba(232,226,210,.65)' : 'rgba(150,110,80,.7)';
      m.fillRect(r() * w, r() * h, 1 + r() * 2.5, 1 + r() * 2.5);
    }
    m.strokeStyle = 'rgba(150,130,80,.55)'; m.lineWidth = 1;
    for (let i = 0; i < 160; i++) {
      const x = w / 2 + (r() - 0.5) * 30, y = r() * h;
      m.beginPath(); m.moveTo(x, y); m.lineTo(x + (r() - 0.5) * 8, y - 4 - r() * 8); m.stroke();
    }
    // fade the gravel out over the soft edges (0.7 m of 6 m each side)
    m.globalCompositeOperation = 'destination-in';
    const fade = m.createLinearGradient(0, 0, w, 0);
    fade.addColorStop(0, 'rgba(0,0,0,0)'); fade.addColorStop(0.13, 'rgba(0,0,0,1)'); fade.addColorStop(0.87, 'rgba(0,0,0,1)'); fade.addColorStop(1, 'rgba(0,0,0,0)');
    m.fillStyle = fade; m.fillRect(0, 0, w, h);
    g.drawImage(mid, 0, 0);
  }, [1, 1]);
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
    text(g, 'STATION 01', 256, 92, 96, '#1f2326', 440, 600);
    g.fillStyle = '#1f2326'; g.fillRect(70, 150, 372, 5);
    text(g, 'U.S. SURVEY', 256, 196, 50, '#1f2326', 400);
    text(g, 'NO THROUGH ROAD', 256, 262, 44, '#8f2a1e', 420);
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
