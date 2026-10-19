import * as THREE from 'three';
import { artImage } from '../core/art';
import { getQuality } from '../core/quality';
import { rng } from '../core/textures';
import { DISH_LAYOUT, DISH_LAYOUT_FAR, ROWS, SPINE_X, FENCE_Z, HWY } from './arrayLayout';

// The things that make a room lived in: papers, mugs with rings under them, an ashtray, binders,
// cables along the walls, notices and posters, a coat on a hook, the worn floor by the doors.
// A room asks for them through a Clutter and gets back two meshes, however many things it asks
// for:
//  - the things: one material on one picture of small printed things drawn here in code (THINGS),
//    each part coloured per corner, so a hundred of them cost one draw call (a wall clock's hands
//    too: their corners are moved in it when the second changes);
//  - the dirt: worn floor, scuffs, stains, cobwebs, see-through, on a second picture (DIRT), and
//    with it the light and shade the lamps alone do not give: soft shadows where things meet the
//    floor, the dark along the walls and in the corners, pools of lamplight, the shine on a counter.
// Low draws only the things that make the room (the first part of the merged geometry, the parts
// not marked fine) and of the dirt only the shade (world/dressing.ts says which is which).
// Both pictures are made once, the first time a room is dressed (after loadArt and the fonts), and
// shared by every room; a room that is disposed never disposes them.

type Draw = (g: CanvasRenderingContext2D, w: number, h: number) => void;
type Rect = { x: number; y: number; w: number; h: number };

// ---------- drawing helpers ----------
const OSW = (px: number, weight = 500) => `${weight} ${px}px Oswald, "Arial Narrow", sans-serif`;
const TYPE = (px: number) => `${px}px "Special Elite", "Courier New", monospace`;
const HAND = (px: number) => `${px}px "Reenie Beanie", cursive`;
const MONO = (px: number) => `${px}px VT323, monospace`;
function paperBg(g: CanvasRenderingContext2D, w: number, h: number, tone = 0) {
  const im = artImage('paper'), k = Math.max(w / im.width, h / im.height);
  g.drawImage(im, (w - im.width * k) / 2, (h - im.height * k) / 2, im.width * k, im.height * k);
  if (tone) { g.fillStyle = `rgba(150,120,70,${tone})`; g.fillRect(0, 0, w, h); }
}
function text(g: CanvasRenderingContext2D, s: string, x: number, y: number, font: string, col: string, maxW: number, align: CanvasTextAlign = 'left') {
  g.font = font; g.fillStyle = col; g.textAlign = align; g.textBaseline = 'alphabetic';
  g.fillText(s, x, y, maxW);
}
// typed lines down a sheet, and grey bars where the words are too small to read anyway
function typed(g: CanvasRenderingContext2D, lines: string[], x: number, y: number, size: number, lh: number, maxW: number, col = '#2a2b2e') {
  lines.forEach((s, i) => text(g, s, x, y + i * lh, TYPE(size), col, maxW));
}
function bars(g: CanvasRenderingContext2D, x: number, y: number, w: number, n: number, lh: number, seed: number, col = 'rgba(40,44,60,.45)') {
  const r = rng(seed); g.fillStyle = col;
  for (let i = 0; i < n; i++) g.fillRect(x, y + i * lh, w * (0.45 + r() * 0.55), Math.max(1, lh * 0.28));
}
function speckle(g: CanvasRenderingContext2D, w: number, h: number, n: number, col: string, seed: number) {
  const r = rng(seed); g.fillStyle = col;
  for (let i = 0; i < n; i++) g.fillRect(r() * w, r() * h, 1 + r() * 1.5, 1 + r() * 1.5);
}
// a yucca: a rosette of blades and a dry stalk
function yucca(g: CanvasRenderingContext2D, x: number, y: number, s: number, col = '#3f4a32') {
  g.strokeStyle = col; g.lineWidth = Math.max(1, s * 0.09); g.lineCap = 'round';
  for (let k = 0; k < 7; k++) { const a = -Math.PI / 2 + (k - 3) * 0.33; g.beginPath(); g.moveTo(x, y); g.lineTo(x + Math.cos(a) * s, y + Math.sin(a) * s); g.stroke(); }
  g.beginPath(); g.moveTo(x, y); g.lineTo(x + s * 0.15, y - s * 2.1); g.stroke();
}
// A photograph: the scene drawn on a canvas of its own, greyed and toned (sepia by default), then
// put into the picture at (x, y). (Its own canvas, read once: never a readback of the atlas.)
function photo(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, draw: Draw, k: [number, number, number] = [200, 175, 130], base: [number, number, number] = [40, 28, 18]) {
  const cv = document.createElement('canvas'); cv.width = w; cv.height = h;
  const p = cv.getContext('2d', { willReadFrequently: true })!;
  draw(p, w, h);
  const d = p.getImageData(0, 0, w, h);
  for (let i = 0; i < d.data.length; i += 4) {
    const v = (d.data[i] * 0.3 + d.data[i + 1] * 0.59 + d.data[i + 2] * 0.11) / 255;
    d.data[i] = base[0] + v * k[0]; d.data[i + 1] = base[1] + v * k[1]; d.data[i + 2] = base[2] + v * k[2];
  }
  p.putImageData(d, 0, 0);
  g.drawImage(cv, x, y);
}
// A painting of the mesas, different for every seed: how many buttes, where and how high, the
// horizon, the range behind them, a dusk or a day sky.
function mesa(g: CanvasRenderingContext2D, w: number, h: number, seed: number, dusk = true) {
  const r = rng(seed), hor = h * (0.56 + r() * 0.14);
  const skies = dusk ? [['#3d4a78', '#c9785a', '#f0b26a'], ['#2c3460', '#a8506a', '#f2a05a'], ['#4a5a80', '#d89a6a', '#f4d08a']] : [['#6f97c4', '#a8c0d4', '#d9d2b8'], ['#5a86b8', '#9ab8cc', '#e8d8b0']];
  const s = skies[Math.floor(r() * skies.length)], sky = g.createLinearGradient(0, 0, 0, hor);
  sky.addColorStop(0, s[0]); sky.addColorStop(0.6, s[1]); sky.addColorStop(1, s[2]);
  g.fillStyle = sky; g.fillRect(0, 0, w, h);
  // the far range, low and hazy
  g.fillStyle = dusk ? 'rgba(110,70,84,.6)' : 'rgba(120,130,156,.5)';
  g.beginPath(); g.moveTo(0, hor);
  for (let x = 0; x <= w + 1; x += w / 10) g.lineTo(x, hor - h * (0.02 + r() * 0.06));
  g.lineTo(w, hor); g.fill();
  g.fillStyle = dusk ? '#a8653f' : '#c08a58'; g.fillRect(0, hor, w, h - hor);
  // the buttes: flat tops, steep sides, a skirt of fallen rock, the face to the light
  const n = 1 + Math.floor(r() * 3);
  for (let i = 0; i < n; i++) {
    const cx = w * (0.14 + (i + r()) / n * 0.72), tw = w * (0.05 + r() * 0.14), bh = h * (0.1 + r() * 0.24), sk = tw * (0.5 + r() * 0.7), top = hor - bh;
    g.fillStyle = dusk ? ['#5a3a35', '#6a4034', '#4e3236'][i] : ['#8a5a3c', '#9a6a44', '#7a5038'][i];
    g.beginPath(); g.moveTo(cx - tw - sk, hor + 1); g.lineTo(cx - tw, top + bh * 0.35); g.lineTo(cx - tw * 0.85, top); g.lineTo(cx + tw * 0.9, top + r() * 3);
    g.lineTo(cx + tw, top + bh * 0.3); g.lineTo(cx + tw + sk, hor + 1); g.fill();
    g.fillStyle = dusk ? 'rgba(240,150,90,.3)' : 'rgba(255,220,170,.22)';
    g.beginPath(); g.moveTo(cx - tw * 0.85, top); g.lineTo(cx - tw * 0.25, top); g.lineTo(cx - tw * 0.4, hor); g.lineTo(cx - tw - sk * 0.6, hor); g.lineTo(cx - tw, top + bh * 0.35); g.fill();
  }
  // brush strokes and sage, a yucca or two in front
  for (let i = 0; i < 90; i++) {
    g.fillStyle = r() < 0.5 ? 'rgba(80,90,60,.7)' : 'rgba(150,100,60,.5)';
    g.fillRect(r() * w, hor + 2 + r() * (h - hor), 3 + r() * 6, 1 + r() * 2);
  }
  for (let i = 0, k = 1 + Math.floor(r() * 2); i < k; i++) yucca(g, w * (0.1 + r() * 0.8), h * (0.84 + r() * 0.1), h * (0.06 + r() * 0.04));
}
// A windmill over a corral and a tank at evening, a ranch painted by somebody's aunt.
function ranch(g: CanvasRenderingContext2D, w: number, h: number, seed: number) {
  const r = rng(seed), hor = h * 0.64;
  const sky = g.createLinearGradient(0, 0, 0, hor); sky.addColorStop(0, '#56658c'); sky.addColorStop(0.6, '#d89a72'); sky.addColorStop(1, '#f2d396');
  g.fillStyle = sky; g.fillRect(0, 0, w, h);
  g.fillStyle = 'rgba(96,74,90,.65)'; g.beginPath(); g.moveTo(0, hor); g.lineTo(w * 0.1, hor - h * 0.06); g.lineTo(w * 0.3, hor - h * 0.07); g.lineTo(w * 0.36, hor - h * 0.02); g.lineTo(w, hor - h * 0.03); g.lineTo(w, hor); g.fill();
  g.fillStyle = '#b88a58'; g.fillRect(0, hor, w, h - hor);
  for (let i = 0; i < 70; i++) { g.fillStyle = r() < 0.5 ? 'rgba(90,96,60,.6)' : 'rgba(150,104,62,.5)'; g.fillRect(r() * w, hor + r() * (h - hor), 3 + r() * 5, 1 + r() * 2); }
  // the windmill: a lattice tower, the wheel of blades, the vane
  const wx = w * 0.68, top = h * 0.2, foot = h * 0.82, s = h * 0.1;
  g.strokeStyle = '#3a3230'; g.lineWidth = Math.max(1, w / 160);
  g.beginPath(); g.moveTo(wx - s * 0.6, foot); g.lineTo(wx - s * 0.12, top + s); g.moveTo(wx + s * 0.6, foot); g.lineTo(wx + s * 0.12, top + s); g.stroke();
  for (let k = 1; k < 5; k++) { const y = top + s + (foot - top - s) * k / 5, hw = s * (0.12 + 0.48 * k / 5); g.beginPath(); g.moveTo(wx - hw, y); g.lineTo(wx + hw, y); g.stroke(); }
  for (let k = 0; k < 14; k++) { const a = k / 14 * Math.PI * 2; g.beginPath(); g.moveTo(wx, top + s * 0.6); g.lineTo(wx + Math.cos(a) * s * 0.75, top + s * 0.6 + Math.sin(a) * s * 0.75); g.stroke(); }
  g.beginPath(); g.moveTo(wx, top + s * 0.6); g.lineTo(wx + s * 1.1, top + s * 0.7); g.lineTo(wx + s * 1.1, top + s * 0.35); g.closePath(); g.stroke();
  // the tank at its foot, the corral off to the left, two cows
  g.fillStyle = '#6a7478'; g.fillRect(wx + s * 0.5, foot - s * 0.35, s * 1.2, s * 0.35); g.fillStyle = '#9ab0b8'; g.fillRect(wx + s * 0.5, foot - s * 0.35, s * 1.2, s * 0.06);
  g.strokeStyle = '#4a3a2c'; g.lineWidth = Math.max(1, w / 220);
  for (let k = 0; k < 9; k++) { const x = w * (0.06 + k * 0.05), y = h * (0.86 - k * 0.012); g.beginPath(); g.moveTo(x, y); g.lineTo(x, y - h * 0.07); g.stroke(); }
  for (const dy of [0.03, 0.055]) { g.beginPath(); g.moveTo(w * 0.06, h * (0.86 - dy)); g.lineTo(w * 0.46, h * (0.764 - dy)); g.stroke(); }
  g.fillStyle = '#3a2a22';
  for (const [cx, cy] of [[0.24, 0.84], [0.33, 0.88]]) { g.fillRect(w * cx, h * cy, w * 0.045, h * 0.03); g.fillRect(w * cx - w * 0.012, h * cy - h * 0.005, w * 0.015, h * 0.02); for (const lx of [0.004, 0.036]) g.fillRect(w * (cx + lx), h * (cy + 0.03), w * 0.005, h * 0.02); }
}
// The array from the air, black and white, as the survey flight saw it: the highway, the spine
// track and its rows, the antennas with their shadows, the building at the foot of it.
function aerial(g: CanvasRenderingContext2D, w: number, h: number, zoom = 1) {
  g.fillStyle = '#8a8a86'; g.fillRect(0, 0, w, h);
  speckle(g, w, h, Math.floor(w * h * 0.25), 'rgba(40,40,40,.18)', 81); speckle(g, w, h, Math.floor(w * h * 0.12), 'rgba(230,230,230,.15)', 83);
  const X = (x: number) => w / 2 + x / 480 * w * zoom, Z = (z: number) => h * 0.92 + z / 470 * h * zoom;
  g.fillStyle = '#4e4e4c'; g.fillRect(X(HWY.x0), 0, Math.max(2, X(HWY.x1) - X(HWY.x0)), h);
  g.fillStyle = '#b8b8b2';
  g.fillRect(X(SPINE_X) - 1, Z(-440), 2, Z(FENCE_Z) - Z(-440));
  for (const z of ROWS) g.fillRect(X(-240), Z(z) - 0.75, X(240) - X(-240), 1.5);
  for (const [, x, z, k] of [...DISH_LAYOUT, ...DISH_LAYOUT_FAR]) {
    const rr = Math.max(1.6, 4.5 * zoom * k * w / 256);
    g.fillStyle = 'rgba(20,20,20,.7)'; g.beginPath(); g.ellipse(X(x) + rr * 0.9, Z(z) + rr * 0.5, rr, rr * 0.7, 0.3, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#ecece6'; g.beginPath(); g.arc(X(x), Z(z), rr, 0, Math.PI * 2); g.fill();
  }
  g.fillStyle = '#d8d8d2'; g.fillRect(X(-8), Z(4), X(8) - X(-8), Math.max(3, Z(10) - Z(4)));
  g.strokeStyle = 'rgba(30,30,30,.5)'; g.lineWidth = 1; g.beginPath(); g.moveTo(0, Z(FENCE_Z)); g.lineTo(w, Z(FENCE_Z)); g.stroke();
}
function calendarGrid(g: CanvasRenderingContext2D, x0: number, y0: number, w: number, first: number, days: number, mark: number, col = '#2b2a27') {
  const cw = w / 7;
  g.font = OSW(9); g.fillStyle = col; g.textAlign = 'center';
  'SMTWTFS'.split('').forEach((d, i) => g.fillText(d, x0 + cw * (i + 0.5), y0));
  g.font = TYPE(10);
  for (let d = 1; d <= days; d++) {
    const cell = d - 1 + first, x = x0 + cw * ((cell % 7) + 0.5), y = y0 + 14 + Math.floor(cell / 7) * 15;
    g.fillStyle = col; g.fillText(String(d), x, y, cw);
    if (d < mark) { g.strokeStyle = 'rgba(120,30,25,.7)'; g.lineWidth = 1; g.beginPath(); g.moveTo(x - 5, y - 8); g.lineTo(x + 5, y + 1); g.stroke(); }
  }
}

// ---------- the things ----------
const memo = (title: string, lines: string[], seed: number, tone = 0): Draw => (g, w, h) => {
  paperBg(g, w, h, tone);
  text(g, title, w / 2, 20, OSW(14, 600), '#26282c', w - 16, 'center');
  g.fillStyle = '#26282c'; g.fillRect(10, 25, w - 20, 1);
  typed(g, lines, 10, 40, 9, 11, w - 20);
  bars(g, 10, 44 + lines.length * 11, w - 24, Math.floor((h - 60 - lines.length * 11) / 9), 9, seed);
};
const poster = (bg: string, fg: string, title: string[], small: string[], icon: Draw): Draw => (g, w, h) => {
  g.fillStyle = bg; g.fillRect(0, 0, w, h);
  speckle(g, w, h, 300, 'rgba(255,255,255,.06)', w + h);
  title.forEach((s, i) => text(g, s, w / 2, 30 + i * 24, OSW(24, 700), fg, w - 12, 'center'));
  g.save(); g.translate(w / 2, 34 + title.length * 24 + 32); icon(g, w, h); g.restore();
  small.forEach((s, i) => text(g, s, w / 2, h - 14 - (small.length - 1 - i) * 12, OSW(10, 500), fg, w - 14, 'center'));
};
const card = (lines: [string, number, string?][], bg = '#efe6cf', border = '#6a4b32'): Draw => (g, w, h) => {
  g.fillStyle = bg; g.fillRect(0, 0, w, h);
  g.strokeStyle = border; g.lineWidth = 2; g.strokeRect(3, 3, w - 6, h - 6);
  let y = 0; const total = lines.reduce((n, [, s]) => n + s * 1.25, 0);
  y = (h - total) / 2;
  for (const [s, size, col] of lines) { y += size * 1.1; text(g, s, w / 2, y, OSW(size, 600), col ?? '#2b2a27', w - 12, 'center'); y += size * 0.15; }
};

const THINGS = {
  // plain white: every solid part samples it, its colour comes from the corners
  white: { w: 128, h: 128, draw: (g, w, h) => { g.fillStyle = '#fff'; g.fillRect(0, 0, w, h); } },
  memoWard: { w: 128, h: 160, draw: memo('MEMORANDUM', ['TO: ALL SHIFTS', 'FROM: E. WARD', 'RE: THE LOG', '', 'Every RFI event goes', 'in the log, with the', 'time. No exceptions.'], 3) },
  memoCoffee: { w: 128, h: 160, draw: memo('NOTICE', ['The coffee maker', 'is not a heater.', 'No food or drink', 'on the racks.', '', '   -- Facilities'], 5, 0.08) },
  memoTapes: { w: 128, h: 160, draw: (g, w, h) => {
    paperBg(g, w, h);
    text(g, 'TAPE INVENTORY', w / 2, 18, OSW(13, 600), '#26282c', w - 12, 'center');
    g.strokeStyle = 'rgba(40,44,60,.5)'; g.lineWidth = 1;
    for (let i = 0; i < 11; i++) { g.beginPath(); g.moveTo(8, 28 + i * 12); g.lineTo(w - 8, 28 + i * 12); g.stroke(); }
    g.beginPath(); g.moveTo(44, 24); g.lineTo(44, 148); g.stroke();
    for (let i = 0; i < 10; i++) { text(g, `0${61 + i}`, 12, 38 + i * 12, TYPE(9), '#2a2b2e', 30); text(g, ['RX3', 'RX1', 'CAL', 'RX3', 'RFI', 'RX2', 'RX3', 'CAL', 'RX1', 'RX3'][i] + '  ' + ['OK', 'OK', 'OK', 'REUSE', 'OK', 'OK', 'OUT', 'OK', 'OK', ''][i], 50, 38 + i * 12, TYPE(9), '#2a2b2e', w - 58); }
  } },
  roster: { w: 128, h: 160, draw: (g, w, h) => {
    paperBg(g, w, h);
    text(g, 'SHIFT ROSTER  APRIL', w / 2, 18, OSW(12, 600), '#26282c', w - 12, 'center');
    const rows = [['11 FRI', 'KAHN', 'DALE', 'REYES'], ['12 SAT', 'ORTIZ', 'DALE', 'REYES'], ['13 SUN', 'KAHN', 'DALE', 'REYES'], ['14 MON', 'ORTIZ', 'KAHN', 'DALE'], ['15 TUE', 'KAHN', 'ORTIZ', 'DALE']];
    text(g, 'DAY      EVE      NIGHT', 40, 34, TYPE(8), '#55575c', w - 44);
    rows.forEach((r, i) => { text(g, r[0], 6, 50 + i * 16, TYPE(8), '#2a2b2e', 32); text(g, r[1], 40, 50 + i * 16, TYPE(8), '#2a2b2e', 28); text(g, r[2], 70, 50 + i * 16, TYPE(8), '#2a2b2e', 26); text(g, r[3], 98, 50 + i * 16, TYPE(8), '#2a2b2e', 28); });
    g.fillStyle = 'rgba(200,180,40,.35)'; g.fillRect(4, 75, w - 8, 12);   // tonight, gone over with a highlighter
  } },
  greenbar: { w: 128, h: 192, draw: (g, w, h) => {
    g.fillStyle = '#eef0e2'; g.fillRect(0, 0, w, h);
    for (let y = 0; y < h; y += 24) { g.fillStyle = '#cfe2c6'; g.fillRect(0, y, w, 12); }
    g.fillStyle = '#d8d8cc'; for (let y = 6; y < h; y += 12) { g.fillRect(2, y, 3, 3); g.fillRect(w - 5, y, 3, 3); }
    const r = rng(11);
    for (let i = 0; i < 15; i++) text(g, `RX3 ${String(1419.9 + r() * 0.6).slice(0, 8)} ${['OK', 'OK', 'RFI', 'OK', 'LOW'][i % 5]}  ${String(Math.floor(r() * 99)).padStart(2, '0')}`, 9, 10 + i * 12, MONO(11), '#3a3d48', w - 18);
  } },
  quiet: { w: 128, h: 176, draw: poster('#22314f', '#f0ead8', ['RADIO', 'QUIET ZONE'], ['NO TRANSMITTERS, CB RADIOS', 'OR MICROWAVE OVENS', 'PAST THIS POINT'], (g) => {
    g.strokeStyle = '#f0ead8'; g.lineWidth = 3;
    g.beginPath(); g.ellipse(0, 0, 22, 8, -0.4, Math.PI, 0, true); g.stroke();
    g.beginPath(); g.moveTo(-6, 4); g.lineTo(0, 26); g.lineTo(6, 4); g.stroke();
    g.strokeStyle = '#d0503a'; g.lineWidth = 4; g.beginPath(); g.arc(0, 4, 30, 0, Math.PI * 2); g.moveTo(-21, -17); g.lineTo(21, 25); g.stroke();
  }) },
  safety: { w: 128, h: 176, draw: poster('#e0b636', '#1e1f20', ['THINK', 'SAFETY'], ['REPORT EVERY INJURY', 'TO THE DUTY SUPERVISOR'], (g) => {
    g.fillStyle = '#1e1f20'; g.beginPath(); g.arc(0, 8, 26, Math.PI, 0); g.fill(); g.fillRect(-34, 6, 68, 7);
    g.fillStyle = '#e0b636'; g.fillRect(-3, -16, 6, 22);
  }) },
  energy: { w: 128, h: 176, draw: poster('#ece6d6', '#8c2a20', ['SAVE', 'ENERGY'], ['LIGHTS OFF', 'WHEN YOU LEAVE'], (g) => {
    g.strokeStyle = '#2a2b2e'; g.lineWidth = 3; g.beginPath(); g.arc(0, 0, 18, Math.PI * 0.8, Math.PI * 2.2); g.stroke();
    g.fillStyle = '#2a2b2e'; g.fillRect(-9, 14, 18, 12);
    g.strokeStyle = '#8c2a20'; for (let i = 0; i < 7; i++) { const a = -Math.PI + i * Math.PI / 6; g.beginPath(); g.moveTo(Math.cos(a) * 24, Math.sin(a) * 24); g.lineTo(Math.cos(a) * 32, Math.sin(a) * 32); g.stroke(); }
  }) },
  postcard: { w: 96, h: 64, draw: (g, w, h) => {
    const s = g.createLinearGradient(0, 0, 0, h); s.addColorStop(0, '#f2a25a'); s.addColorStop(0.6, '#e25e4a'); s.addColorStop(0.61, '#2c6f8a'); s.addColorStop(1, '#174c63');
    g.fillStyle = s; g.fillRect(0, 0, w, h); g.fillStyle = '#ffe2a0'; g.beginPath(); g.arc(w * 0.6, h * 0.6, 9, Math.PI, 0); g.fill();
    g.fillStyle = '#e9d8a8'; g.fillRect(0, h * 0.82, w, h * 0.18);
    text(g, 'GREETINGS FROM THE GULF', w / 2, 13, OSW(10, 700), '#fff8e0', w - 6, 'center');
    g.strokeStyle = '#fff'; g.lineWidth = 3; g.strokeRect(1.5, 1.5, w - 3, h - 3);
  } },
  calendar: { w: 128, h: 192, draw: (g, w, h) => {
    g.fillStyle = '#f2ecdc'; g.fillRect(0, 0, w, h);
    g.save(); g.beginPath(); g.rect(6, 6, w - 12, 62); g.clip(); g.translate(6, 6); ranch(g, w - 12, 62, 7); g.restore();
    text(g, 'CALICHE FEED & SUPPLY', w / 2, 80, OSW(9, 600), '#7a2e22', w - 10, 'center');
    text(g, 'APRIL 1986', w / 2, 96, OSW(13, 700), '#2b2a27', w - 10, 'center');
    calendarGrid(g, 6, 110, w - 12, 2, 30, 13);
  } },
  calendarOld: { w: 128, h: 192, draw: (g, w, h) => {
    g.fillStyle = '#e8dcbc'; g.fillRect(0, 0, w, h);
    g.save(); g.globalAlpha = 0.55; g.beginPath(); g.rect(6, 6, w - 12, 62); g.clip(); g.translate(6, 6); mesa(g, w - 12, 62, 21, true); g.restore();
    text(g, 'CHAVES COUNTY CO-OP', w / 2, 80, OSW(9, 600), '#8a6a4a', w - 10, 'center');
    text(g, 'AUGUST 1983', w / 2, 96, OSW(13, 700), '#6a5a44', w - 10, 'center');
    calendarGrid(g, 6, 110, w - 12, 1, 31, 0, '#6a5a44');
    g.fillStyle = 'rgba(170,130,70,.25)'; g.fillRect(0, 0, w, h);   // a sun-faded page nobody turned
  } },
  // the pictures on the walls: the motel office's mesas at dusk, room 6's butte in the day, the
  // diner's ranch, a still life of desert flowers (seeded, so no two rooms hang the same one)
  painting: { w: 256, h: 160, draw: (g, w, h) => { mesa(g, w, h, 3); } },
  paintingBed: { w: 256, h: 160, draw: (g, w, h) => { mesa(g, w, h, 14, false); } },
  ranch: { w: 256, h: 160, draw: (g, w, h) => { ranch(g, w, h, 5); } },
  flowers: { w: 128, h: 176, draw: (g, w, h) => {
    const bg = g.createLinearGradient(0, 0, 0, h); bg.addColorStop(0, '#3a2e2a'); bg.addColorStop(1, '#5a4434'); g.fillStyle = bg; g.fillRect(0, 0, w, h);
    g.fillStyle = '#7a5a3a'; g.fillRect(0, h * 0.8, w, h * 0.2); g.fillStyle = 'rgba(255,230,180,.15)'; g.fillRect(0, h * 0.8, w, 2);
    const r = rng(91), cx = w * 0.5, base = h * 0.8;
    // stems out of a clay jar, red paintbrush and yellow marigolds on them
    g.strokeStyle = '#5a6a3a'; g.lineWidth = 1.5;
    const tips: [number, number][] = [];
    for (let i = 0; i < 9; i++) { const tx = cx + (r() - 0.5) * w * 0.7, ty = h * (0.12 + r() * 0.3); g.beginPath(); g.moveTo(cx + (r() - 0.5) * 10, base - h * 0.2); g.quadraticCurveTo(cx + (tx - cx) * 0.3, ty + h * 0.2, tx, ty); g.stroke(); tips.push([tx, ty]); }
    tips.forEach(([x, y], i) => {
      if (i % 3 === 2) { g.fillStyle = '#e8c040'; for (let k = 0; k < 8; k++) { const a = k / 8 * Math.PI * 2; g.beginPath(); g.ellipse(x + Math.cos(a) * 4, y + Math.sin(a) * 4, 3, 1.6, a, 0, Math.PI * 2); g.fill(); } g.fillStyle = '#8a5a20'; g.beginPath(); g.arc(x, y, 2, 0, Math.PI * 2); g.fill(); }
      else { for (let k = 0; k < 6; k++) { g.fillStyle = k % 2 ? '#d84a2a' : '#f0703a'; g.beginPath(); g.ellipse(x + (r() - 0.5) * 4, y + k * 3, 3, 2.2, 0, 0, Math.PI * 2); g.fill(); } }
    });
    g.fillStyle = '#a8603a'; g.beginPath(); g.moveTo(cx - 18, base - h * 0.22); g.quadraticCurveTo(cx - 30, base - h * 0.08, cx - 16, base); g.lineTo(cx + 16, base); g.quadraticCurveTo(cx + 30, base - h * 0.08, cx + 18, base - h * 0.22); g.fill();
    g.fillStyle = 'rgba(255,220,170,.25)'; g.fillRect(cx - 12, base - h * 0.18, 5, h * 0.14);
    g.strokeStyle = '#6a3a22'; g.lineWidth = 2; g.beginPath(); g.moveTo(cx - 20, base - h * 0.13); g.lineTo(cx + 20, base - h * 0.13); g.stroke();
  } },
  // old photographs in the diner: the crew that built the place, a man by his truck
  sepiaGroup: { w: 128, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#f0ead8'; g.fillRect(0, 0, w, h);
    const b = 7, iw = w - b * 2, ih = h - b * 2, r = rng(93);
    photo(g, b, b, iw, ih, (g) => {
    g.fillStyle = '#c8c4b8'; g.fillRect(0, 0, iw, ih); g.fillStyle = '#6a625a'; g.fillRect(0, ih * 0.22, iw, ih * 0.5);
    for (let x = 0; x < iw; x += 6) { g.fillStyle = 'rgba(30,26,22,.35)'; g.fillRect(x, ih * 0.22, 1, ih * 0.5); }
    g.fillStyle = '#9a948a'; g.fillRect(0, ih * 0.72, iw, ih * 0.28);
    for (let i = 0; i < 7; i++) {
      const x = iw * (0.1 + i * 0.13) + (r() - 0.5) * 3, hh = ih * (0.42 + r() * 0.1), y = ih * 0.84;
      g.fillStyle = i % 3 ? '#2a2622' : '#4a4440'; g.fillRect(x - 5, y - hh, 10, hh);
      g.fillStyle = '#b8b0a4'; g.beginPath(); g.arc(x, y - hh - 4, 4, 0, Math.PI * 2); g.fill();
      if (i % 2 === 0) { g.fillStyle = '#1e1c1a'; g.fillRect(x - 7, y - hh - 8, 14, 2); g.fillRect(x - 4, y - hh - 12, 8, 4); }
    }
    });
    const v = g.createRadialGradient(w / 2, h / 2, ih * 0.3, w / 2, h / 2, w * 0.6); v.addColorStop(0, 'rgba(60,40,20,0)'); v.addColorStop(1, 'rgba(60,40,20,.45)');
    g.fillStyle = v; g.fillRect(b, b, iw, ih);
  } },
  sepiaTruck: { w: 128, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#f0ead8'; g.fillRect(0, 0, w, h);
    const b = 7, iw = w - b * 2, ih = h - b * 2;
    photo(g, b, b, iw, ih, (g) => {
    const sky = g.createLinearGradient(0, 0, 0, ih * 0.6); sky.addColorStop(0, '#d8d4cc'); sky.addColorStop(1, '#f0ece4'); g.fillStyle = sky; g.fillRect(0, 0, iw, ih);
    g.fillStyle = '#8a847a'; g.beginPath(); g.moveTo(0, ih * 0.58); g.lineTo(iw * 0.3, ih * 0.52); g.lineTo(iw * 0.36, ih * 0.56); g.lineTo(iw, ih * 0.55); g.lineTo(iw, ih); g.lineTo(0, ih); g.fill();
    g.fillStyle = '#a8a298'; g.fillRect(0, ih * 0.62, iw, ih * 0.38);
    // a pickup of the forties side on: the cab, the hood, the fenders, the bed
    const tx = iw * 0.18, ty = ih * 0.8;
    g.fillStyle = '#2a2622'; g.fillRect(tx, ty - 22, 62, 12); g.fillRect(tx + 30, ty - 34, 20, 14); g.fillRect(tx + 50, ty - 24, 26, 12);
    g.beginPath(); g.ellipse(tx + 14, ty - 8, 12, 9, 0, Math.PI, 0); g.ellipse(tx + 62, ty - 8, 12, 9, 0, Math.PI, 0); g.fill();
    g.fillStyle = '#0e0c0a'; for (const x of [tx + 14, tx + 62]) { g.beginPath(); g.arc(x, ty - 4, 7, 0, Math.PI * 2); g.fill(); }
    g.fillStyle = '#c8c2b8'; g.fillRect(tx + 33, ty - 31, 12, 8);
    // and the man beside it, hat and shirtsleeves
    const mx = iw * 0.8; g.fillStyle = '#3a3430'; g.fillRect(mx - 4, ty - 36, 8, 22); g.fillStyle = '#2a2622'; g.fillRect(mx - 3, ty - 14, 3, 14); g.fillRect(mx + 1, ty - 14, 3, 14);
    g.fillStyle = '#b0a898'; g.beginPath(); g.arc(mx, ty - 40, 4, 0, Math.PI * 2); g.fill(); g.fillStyle = '#1e1c1a'; g.fillRect(mx - 7, ty - 44, 14, 2); g.fillRect(mx - 4, ty - 48, 8, 4);
    });
    const v = g.createRadialGradient(w / 2, h / 2, ih * 0.3, w / 2, h / 2, w * 0.6); v.addColorStop(0, 'rgba(60,40,20,0)'); v.addColorStop(1, 'rgba(60,40,20,.5)');
    g.fillStyle = v; g.fillRect(b, b, iw, ih);
  } },
  // the survey flight's picture of the array, framed in the corridor
  aerial: { w: 192, h: 128, draw: (g, w, h) => {
    g.fillStyle = '#f2f0ea'; g.fillRect(0, 0, w, h);
    photo(g, 8, 8, w - 16, h - 30, (g, w, h) => aerial(g, w, h), [215, 215, 210], [18, 18, 18]);
    text(g, 'SARO ARRAY  FROM 9,000 FT', w / 2, h - 13, TYPE(10), '#2a2b2e', w - 16, 'center');
  } },
  // a paperback somebody left in the room: a cowboy story with a red sky
  paperback: { w: 64, h: 96, draw: (g, w, h) => {
    const s = g.createLinearGradient(0, 0, 0, h); s.addColorStop(0, '#2a1a2a'); s.addColorStop(0.45, '#c8402a'); s.addColorStop(0.75, '#f0a040'); s.addColorStop(1, '#3a2018');
    g.fillStyle = s; g.fillRect(0, 0, w, h);
    g.fillStyle = '#f4e8c0'; g.beginPath(); g.arc(w * 0.7, h * 0.36, 7, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#1a1010'; g.beginPath(); g.moveTo(0, h * 0.82); g.lineTo(w * 0.3, h * 0.66); g.lineTo(w * 0.5, h * 0.68); g.lineTo(w * 0.62, h * 0.6); g.lineTo(w, h * 0.7); g.lineTo(w, h); g.lineTo(0, h); g.fill();
    g.fillRect(w * 0.4, h * 0.5, 4, 14); g.fillRect(w * 0.4 - 4, h * 0.5 - 2, 12, 2); g.fillRect(w * 0.4 - 1, h * 0.5 - 6, 6, 4);
    g.fillStyle = '#1a1010'; g.fillRect(0, 0, w, 22);
    text(g, 'HIGH DESERT', w / 2, 13, OSW(11, 700), '#f2c040', w - 6, 'center'); text(g, 'J. T. CALDER', w / 2, 20, OSW(6, 500), '#e8e0c8', w - 8, 'center');
    g.fillStyle = 'rgba(255,255,255,.18)'; g.fillRect(2, 0, 2, h);
  } },
  // a blind's slats, drawn up: the front of the stack (the corners colour it)
  slats: { w: 64, h: 32, draw: (g, w, h) => {
    for (let i = 0; i < 5; i++) {
      const y = i * h / 5, s = g.createLinearGradient(0, y, 0, y + h / 5); s.addColorStop(0, '#ffffff'); s.addColorStop(0.7, '#d8d6d0'); s.addColorStop(1, '#8a8882');
      g.fillStyle = s; g.fillRect(0, y, w, h / 5); g.fillStyle = '#5a5852'; g.fillRect(0, y + h / 5 - 1, w, 1);
    }
    g.fillStyle = 'rgba(60,58,52,.5)'; g.fillRect(w * 0.2, 0, 1, h); g.fillRect(w * 0.8, 0, 1, h);   // the ladder strings
  } },
  // ribbed knit, for a cardigan (the corners colour it)
  knit: { w: 64, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f2f0ea'; g.fillRect(0, 0, w, h);
    for (let x = 0; x < w; x += 2) { g.fillStyle = 'rgba(90,78,60,.14)'; g.fillRect(x, 0, 1, h); }
    speckle(g, w, h, 160, 'rgba(90,78,60,.1)', 101);
    g.fillStyle = 'rgba(90,78,60,.12)'; g.fillRect(0, h - 9, w, 9);   // the ribbed band at the hem
  } },
  // a striped hand towel, worn thin
  towel: { w: 64, h: 128, draw: (g, w, h) => {
    g.fillStyle = '#5c544a'; g.fillRect(0, 0, w, h); speckle(g, w, h, 500, 'rgba(255,255,255,.08)', 97); speckle(g, w, h, 400, 'rgba(0,0,0,.12)', 99);
    for (const y of [h - 30, h - 20]) { g.fillStyle = '#d8ccb0'; g.fillRect(0, y, w, 5); }
    g.fillStyle = '#8a3a2a'; g.fillRect(0, h - 25, w, 2);
    g.fillStyle = '#4a443c'; for (let x = 1; x < w; x += 3) g.fillRect(x, h - 6, 1, 6);   // the fringe
  } },
  clock: { w: 128, h: 128, draw: (g, w, h) => {
    g.fillStyle = '#2b3033'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#efe9d8'; g.beginPath(); g.arc(64, 64, 62, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#1c1d1f';
    for (let i = 0; i < 60; i++) { const a = i / 60 * Math.PI * 2, l = i % 5 ? 4 : 9, wd = i % 5 ? 1 : 3; g.save(); g.translate(64, 64); g.rotate(a); g.fillRect(-wd / 2, -58, wd, l); g.restore(); }
    g.font = OSW(16, 600); g.textAlign = 'center'; g.textBaseline = 'middle';
    [['12', 64, 24], ['3', 106, 64], ['6', 64, 105], ['9', 22, 64]].forEach(([s, x, y]) => g.fillText(s as string, x as number, y as number));
    text(g, 'QUARTZ', 64, 88, OSW(7, 500), '#55575c', 40, 'center');
  } },
  calc: { w: 64, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#2a2c2e'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#8f9a7c'; g.fillRect(6, 6, w - 12, 18); text(g, '1420.405', w - 9, 21, MONO(16), '#22261c', w - 14, 'right');
    for (let r = 0; r < 5; r++) for (let c = 0; c < 4; c++) { g.fillStyle = c === 3 ? '#b8743a' : r === 0 ? '#6e7276' : '#c9c6bc'; g.fillRect(7 + c * 13, 32 + r * 12, 10, 9); }
  } },
  adder: { w: 96, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#cfc6ae'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#f2efe4'; g.fillRect(28, 0, 40, 26); bars(g, 32, 4, 32, 5, 4, 2, 'rgba(60,60,70,.6)');
    g.fillStyle = '#3c3e40'; g.fillRect(8, 30, w - 16, 3);
    for (let r = 0; r < 4; r++) for (let c = 0; c < 5; c++) { g.fillStyle = c === 4 ? (r < 2 ? '#a4342a' : '#2a2c2e') : '#e8e4d8'; g.beginPath(); g.arc(18 + c * 15, 44 + r * 13, 5, 0, Math.PI * 2); g.fill(); }
  } },
  books: { w: 256, h: 64, draw: (g, w, h) => {
    const r = rng(17), cols = ['#5a2a26', '#25324a', '#3c4a2e', '#8a6a3a', '#6a3a4a', '#2e2e32', '#7a5a2a', '#44505a'];
    for (let x = 0; x < w;) { const bw = 10 + Math.floor(r() * 14); g.fillStyle = cols[Math.floor(r() * cols.length)]; g.fillRect(x, 0, bw - 1, h);
      g.fillStyle = 'rgba(230,210,160,.55)'; g.fillRect(x + 2, 8, bw - 5, 2); g.fillRect(x + 2, h - 10, bw - 5, 2); g.fillRect(x + bw / 2 - 1, 16, 2, 22 + r() * 10); x += bw; }
  } },
  binders: { w: 256, h: 64, draw: (g, w, h) => {
    const cols = ['#2f4a6e', '#6e2f2a', '#3f5b3a', '#2b2b30', '#7a6a3a', '#2f4a6e', '#5a5e62', '#6e2f2a'];
    cols.forEach((c, i) => { g.fillStyle = c; g.fillRect(i * 32, 0, 31, h); g.fillStyle = '#e8e2cc'; g.fillRect(i * 32 + 7, 10, 17, 26);
      text(g, String(78 + i), i * 32 + 15.5, 27, TYPE(10), '#2a2b2e', 16, 'center'); g.fillStyle = '#9a9a92'; g.beginPath(); g.arc(i * 32 + 15.5, 50, 4, 0, Math.PI * 2); g.fill(); });
  } },
  folders: { w: 128, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#3a3c36'; g.fillRect(0, 0, w, h);
    const r = rng(23);
    for (let i = 0; i < 14; i++) { const x = i * 9; g.fillStyle = i % 4 === 3 ? '#d9cfae' : '#cdb27a'; g.fillRect(x, 18 + r() * 6, 8, h); g.fillRect(x + (i % 3) * 2, 8 + r() * 6, 6, 14); }
  } },
  fan: { w: 128, h: 128, draw: (g) => {
    g.fillStyle = '#2a2c2c'; g.beginPath(); g.arc(64, 64, 63, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#4a5a58'; for (let i = 0; i < 4; i++) { g.save(); g.translate(64, 64); g.rotate(i * Math.PI / 2 + 0.3); g.beginPath(); g.ellipse(0, -30, 15, 28, 0.35, 0, Math.PI * 2); g.fill(); g.restore(); }
    g.strokeStyle = '#b8bcb8'; g.lineWidth = 2;
    for (const r of [60, 46, 32, 18]) { g.beginPath(); g.arc(64, 64, r, 0, Math.PI * 2); g.stroke(); }
    for (let i = 0; i < 16; i++) { const a = i / 16 * Math.PI * 2; g.beginPath(); g.moveTo(64 + Math.cos(a) * 12, 64 + Math.sin(a) * 12); g.lineTo(64 + Math.cos(a) * 61, 64 + Math.sin(a) * 61); g.stroke(); }
    g.fillStyle = '#c9ccc8'; g.beginPath(); g.arc(64, 64, 11, 0, Math.PI * 2); g.fill();
  } },
  pack: { w: 48, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f1ece0'; g.fillRect(0, 0, w, h); g.fillStyle = '#b3261e';
    g.beginPath(); g.moveTo(0, 0); g.lineTo(w, 0); g.lineTo(w, h * 0.38); g.lineTo(w / 2, h * 0.5); g.lineTo(0, h * 0.38); g.fill();
    g.fillStyle = '#c9a24a'; g.beginPath(); g.arc(w / 2, h * 0.7, 6, 0, Math.PI * 2); g.fill();
  } },
  matches: { w: 48, h: 32, draw: (g, w, h) => { g.fillStyle = '#1e2224'; g.fillRect(0, 0, w, h); g.fillStyle = '#c8a050'; g.fillRect(0, h - 6, w, 6); text(g, 'MATCHES', w / 2, 17, OSW(9, 600), '#e8e0c8', w - 4, 'center'); } },
  can: { w: 128, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#9a2a22'; g.fillRect(0, 0, w, h); g.fillStyle = '#d8d2c0'; g.fillRect(0, 30, w, 34);
    text(g, 'COFFEE', w / 2, 56, OSW(26, 700), '#3a1a12', w - 8, 'center'); text(g, 'GROUND  2 LB', w / 2, 82, OSW(10, 500), '#f0e6d0', w - 8, 'center');
  } },
  reel: { w: 128, h: 128, draw: (g) => {
    g.fillStyle = '#3a2a1c'; g.fillRect(0, 0, 128, 128);
    g.fillStyle = '#b8bcbe'; g.beginPath(); g.arc(64, 64, 63, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#4a3422'; for (let i = 0; i < 3; i++) { g.beginPath(); g.arc(64, 64, 52, i * 2.094 + 0.35, i * 2.094 + 1.55); g.arc(64, 64, 22, i * 2.094 + 1.55, i * 2.094 + 0.35, true); g.fill(); }
    g.fillStyle = '#7c8286'; g.beginPath(); g.arc(64, 64, 9, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#e8e2cc'; g.fillRect(52, 30, 24, 10); text(g, '063', 64, 38.5, TYPE(8), '#2a2b2e', 22, 'center');
  } },
  ashtray: { w: 64, h: 64, draw: (g) => {
    g.fillStyle = '#5a3a20'; g.fillRect(0, 0, 64, 64);
    g.fillStyle = '#8a5a2e'; g.beginPath(); g.arc(32, 32, 31, 0, Math.PI * 2); g.fill();
    g.fillStyle = '#4a4440'; g.beginPath(); g.arc(32, 32, 22, 0, Math.PI * 2); g.fill();
    speckle(g, 64, 64, 120, 'rgba(160,150,140,.6)', 5);
    g.fillStyle = '#2a2420'; g.beginPath(); g.arc(28, 36, 6, 0, Math.PI * 2); g.fill();
    for (const a of [0, 2.1, 4.2]) { g.fillStyle = '#5a4a3c'; g.save(); g.translate(32 + Math.cos(a) * 25, 32 + Math.sin(a) * 25); g.rotate(a); g.fillRect(-4, -2, 8, 4); g.restore(); }
  } },
  checkout: { w: 128, h: 64, draw: card([['CHECK OUT TIME', 12], ['11 A.M.', 18, '#7a2e22'], ['THANK YOU', 9]]) },
  rates: { w: 128, h: 96, draw: card([['ROOM RATES', 13], ['SINGLE   19.95', 10], ['DOUBLE   24.95', 10], ['WEEKLY RATES - ASK', 8], ['NO CHECKS', 9, '#7a2e22']]) },
  nopets: { w: 128, h: 48, draw: card([['NO PETS', 20, '#7a2e22']], '#f0e8d0', '#7a2e22') },
  pie: { w: 128, h: 160, draw: (g, w, h) => {
    g.fillStyle = '#5a4430'; g.fillRect(0, 0, w, h); g.fillStyle = '#1d2422'; g.fillRect(6, 6, w - 12, h - 12);
    speckle(g, w, h, 400, 'rgba(255,255,255,.05)', 3);
    text(g, 'PIE', w / 2, 42, HAND(40), '#ece8dc', w - 16, 'center');
    ['cherry', 'apple', 'pecan', '.95 a slice'].forEach((s, i) => text(g, s, w / 2, 76 + i * 22, HAND(26), i === 3 ? '#f0c080' : '#ece8dc', w - 16, 'center'));
  } },
  help: { w: 96, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#c8a874'; g.fillRect(0, 0, w, h);
    text(g, 'HELP WANTED', w / 2, 24, OSW(15, 700), '#8a2a20', w - 8, 'center'); text(g, 'night cook', w / 2, 46, HAND(22), '#2a2420', w - 8, 'center');
  } },
  tent: { w: 64, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#f0e6cc'; g.fillRect(0, 0, w, h); g.fillStyle = '#4e7a5a'; g.fillRect(0, 0, w, 20);
    text(g, 'TRY OUR', w / 2, 15, OSW(11, 600), '#f0e6cc', w - 6, 'center');
    text(g, 'GREEN', w / 2, 44, OSW(16, 700), '#3a5a40', w - 6, 'center'); text(g, 'CHILE', w / 2, 62, OSW(16, 700), '#3a5a40', w - 6, 'center');
    text(g, 'STEW 2.95', w / 2, 84, OSW(10, 500), '#2b2a27', w - 6, 'center');
  } },
  exposure: { w: 128, h: 160, draw: (g, w, h) => {
    paperBg(g, w, h);
    text(g, 'EXPOSURE LOG', w / 2, 16, OSW(12, 600), '#26282c', w - 12, 'center');
    g.strokeStyle = 'rgba(40,44,60,.45)'; g.lineWidth = 1;
    for (let i = 0; i < 12; i++) { g.beginPath(); g.moveTo(6, 24 + i * 11); g.lineTo(w - 6, 24 + i * 11); g.stroke(); }
    const r = rng(41);
    for (let i = 0; i < 8; i++) text(g, `f/${[8, 11, 8, 5.6][i % 4]}  ${4 + Math.floor(r() * 20)}s  gr ${2 + (i % 3)}`, 10, 33 + i * 11, HAND(14), '#24304a', w - 16);
  } },
  grades: { w: 128, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#f2f0ea'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 6; i++) { const v = 230 - i * 38; g.fillStyle = `rgb(${v},${v},${v})`; g.fillRect(8 + i * 19, 14, 18, 56); text(g, String(4 + i * 4), 17 + i * 19, 84, HAND(16), '#24304a', 18, 'center'); }
  } },
  printA: { w: 96, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f4f2ec'; g.fillRect(0, 0, w, h);
    const s = g.createLinearGradient(0, 4, 0, h - 4); s.addColorStop(0, '#2a2a2a'); s.addColorStop(1, '#8a8a8a');
    g.fillStyle = s; g.fillRect(4, 4, w - 8, h - 8);
    g.fillStyle = '#d8d8d8'; g.beginPath(); g.ellipse(w * 0.5, h * 0.42, 22, 8, -0.3, Math.PI, 0, true); g.fill(); g.fillRect(w * 0.5 - 2, h * 0.42, 4, 18);
    g.fillStyle = '#444'; g.fillRect(4, h * 0.75, w - 8, h * 0.25 - 4);
  } },
  // a test print of part of the array from the air
  printB: { w: 96, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f4f2ec'; g.fillRect(0, 0, w, h);
    photo(g, 4, 4, w - 8, h - 8, (g, w, h) => { g.translate(-w * 0.35, -h * 0.9); aerial(g, w * 1.7, h * 1.9); }, [235, 235, 235], [8, 8, 8]);
  } },
  genlog: { w: 128, h: 160, draw: (g, w, h) => {
    paperBg(g, w, h, 0.12);
    text(g, 'GENERATOR LOG', w / 2, 16, OSW(12, 600), '#26282c', w - 12, 'center');
    text(g, 'DATE    HRS   OIL', 8, 30, TYPE(8), '#55575c', w - 12);
    const r = rng(53);
    for (let i = 0; i < 9; i++) text(g, `${1 + (i * 3) % 12}/${4 + (i % 20)}   ${1100 + i * 37 + Math.floor(r() * 20)}   ${i % 3 ? 'ok' : 'added'}`, 8, 44 + i * 12, HAND(14), '#24304a', w - 14);
  } },
  sketch: { w: 160, h: 128, draw: (g, w, h) => {
    g.fillStyle = '#efeadb'; g.fillRect(0, 0, w, h);
    g.strokeStyle = 'rgba(80,130,160,.35)'; g.lineWidth = 1;
    for (let x = 0; x < w; x += 8) { g.beginPath(); g.moveTo(x, 0); g.lineTo(x, h); g.stroke(); }
    for (let y = 0; y < h; y += 8) { g.beginPath(); g.moveTo(0, y); g.lineTo(w, y); g.stroke(); }
    g.strokeStyle = 'rgba(50,50,55,.8)'; g.lineWidth = 1.5;
    g.strokeRect(24, 64, 26, 20); g.beginPath(); g.arc(104, 44, 6, 0, Math.PI * 2); g.stroke();
    g.setLineDash([4, 3]); g.beginPath(); g.moveTo(104, 44); g.lineTo(150, 44); g.moveTo(50, 74); g.lineTo(104, 44); g.stroke(); g.setLineDash([]);
    text(g, 'N', 140, 22, HAND(18), '#333', 14); g.beginPath(); g.moveTo(144, 26); g.lineTo(144, 10); g.stroke();
    text(g, 'hut', 26, 98, HAND(14), '#333', 30); text(g, 'pier', 92, 62, HAND(14), '#333', 30);
  } },
  news: { w: 128, h: 160, draw: (g, w, h) => {
    g.fillStyle = '#e4dfd0'; g.fillRect(0, 0, w, h);
    text(g, 'PECOS VALLEY SENTINEL', w / 2, 14, OSW(11, 700), '#202022', w - 8, 'center');
    g.fillStyle = '#202022'; g.fillRect(6, 18, w - 12, 1);
    text(g, 'COUNTY OKS ROAD BOND', w / 2, 32, OSW(12, 700), '#202022', w - 8, 'center');
    g.fillStyle = '#9a9890'; g.fillRect(8, 40, 54, 40);
    for (let c = 0; c < 3; c++) bars(g, 8 + c * 39, c === 0 ? 86 : 42, 34, c === 0 ? 14 : 22, 5, 60 + c, 'rgba(40,40,44,.55)');
  } },
  numbers: { w: 128, h: 160, draw: (g, w, h) => {
    paperBg(g, w, h);
    g.fillStyle = '#8c2a20'; g.fillRect(6, 6, w - 12, 22); text(g, 'EMERGENCY NUMBERS', w / 2, 22, OSW(12, 700), '#f2ead8', w - 16, 'center');
    [['FIRE', '555-0141'], ['SHERIFF', '555-0122'], ['DOCTOR', '555-0173'], ['ROSWELL HOSP.', '555-0190'], ['DUTY SUPERVISOR', 'EXT 21'], ['POWER CO.', '555-0108']]
      .forEach(([a, b], i) => { text(g, a, 10, 48 + i * 18, TYPE(9), '#2a2b2e', 70); text(g, b, w - 10, 48 + i * 18, TYPE(9), '#2a2b2e', 50, 'right'); });
  } },
  fund: { w: 96, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f4f0e2'; g.fillRect(0, 0, w, h); g.fillStyle = 'rgba(200,60,60,.5)'; g.fillRect(0, 12, w, 1);
    g.fillStyle = 'rgba(80,120,170,.35)'; for (let y = 24; y < h; y += 10) g.fillRect(0, y, w, 1);
    text(g, 'COFFEE FUND', w / 2, 10, OSW(9, 600), '#2a2b2e', w - 6, 'center');
    text(g, '25c a cup', w / 2, 32, HAND(18), '#24304a', w - 8, 'center'); text(g, 'pay up, Dale', w / 2, 52, HAND(18), '#24304a', w - 8, 'center');
  } },
  fuses: { w: 96, h: 48, draw: (g, w, h) => {
    g.fillStyle = '#e2d6b0'; g.fillRect(0, 0, w, h); speckle(g, w, h, 80, 'rgba(120,100,60,.2)', 9);
    text(g, 'spare fuses', w / 2, 20, HAND(20), '#1f2a44', w - 8, 'center'); text(g, 'drawer 2', w / 2, 40, HAND(20), '#1f2a44', w - 8, 'center');
  } },
  fire: { w: 96, h: 128, draw: (g, w, h) => {
    g.fillStyle = '#f2ecdc'; g.fillRect(0, 0, w, h); g.fillStyle = '#a82a20'; g.fillRect(0, 0, w, 22);
    text(g, 'IN CASE OF FIRE', w / 2, 15, OSW(11, 700), '#fff', w - 6, 'center');
    g.strokeStyle = '#3a3a3a'; g.lineWidth = 1.5; g.strokeRect(12, 30, 72, 46); g.strokeRect(12, 30, 30, 22);
    g.fillStyle = '#c03020'; g.beginPath(); g.arc(27, 41, 4, 0, Math.PI * 2); g.fill();
    g.strokeStyle = '#2a8a3a'; g.lineWidth = 2; g.beginPath(); g.moveTo(27, 41); g.lineTo(27, 66); g.lineTo(80, 66); g.lineTo(74, 61); g.moveTo(80, 66); g.lineTo(74, 71); g.stroke();
    bars(g, 10, 86, 76, 5, 8, 71);
  } },
  // the labels on the archive boxes: typed, or written on in a hurry
  boxlabel: { w: 96, h: 48, draw: (g, w, h) => { paperBg(g, w, h); typed(g, ['RX LOGS', '1979 - 1980'], 8, 20, 11, 16, w - 16); } },
  boxlabelB: { w: 96, h: 48, draw: (g, w, h) => { paperBg(g, w, h, 0.1); text(g, 'RFI reports', 8, 22, HAND(22), '#1f2a44', w - 14); text(g, '81 - 82', 30, 40, HAND(20), '#1f2a44', w - 34); } },
  boxlabelC: { w: 96, h: 48, draw: (g, w, h) => { paperBg(g, w, h, 0.05); typed(g, ['MAINTENANCE', '1977 - 78'], 8, 20, 11, 16, w - 16); g.fillStyle = 'rgba(160,40,30,.7)'; g.fillRect(6, 27, w - 12, 2); } },
  boxlabelD: { w: 96, h: 48, draw: (g, w, h) => { paperBg(g, w, h, 0.18); text(g, 'DUPLICATES', w / 2, 22, HAND(22), '#2a2420', w - 12, 'center'); text(g, 'do not toss', w / 2, 40, HAND(18), '#8a2a20', w - 16, 'center'); } },
  plate: { w: 64, h: 64, draw: (g) => {
    g.fillStyle = '#ece8de'; g.fillRect(0, 0, 64, 64); g.strokeStyle = '#c8c2b4'; g.lineWidth = 2; g.beginPath(); g.arc(32, 32, 22, 0, Math.PI * 2); g.stroke();
    g.strokeStyle = '#4e7a5a'; g.lineWidth = 1.5; g.beginPath(); g.arc(32, 32, 29, 0, Math.PI * 2); g.stroke();
    speckle(g, 64, 64, 20, 'rgba(150,100,50,.6)', 13); g.fillStyle = 'rgba(160,40,30,.5)'; g.fillRect(24, 30, 9, 3);
  } },
  pieTop: { w: 64, h: 64, draw: (g) => {
    g.fillStyle = '#7a1a18'; g.fillRect(0, 0, 64, 64);
    g.fillStyle = '#d8a058'; for (let i = 0; i < 6; i++) { g.fillRect(i * 11 + 2, 0, 5, 64); g.fillRect(0, i * 11 + 2, 64, 5); }
    g.strokeStyle = '#c08040'; g.lineWidth = 6; g.beginPath(); g.arc(32, 32, 29, 0, Math.PI * 2); g.stroke();
  } },
  burner: { w: 64, h: 64, draw: (g) => {
    g.fillStyle = '#1e1f20'; g.fillRect(0, 0, 64, 64); g.strokeStyle = '#6a6c6e'; g.lineWidth = 3;
    g.beginPath(); for (let a = 0; a < Math.PI * 8; a += 0.2) { const r = 4 + a * 1.05; g.lineTo(32 + Math.cos(a) * r, 32 + Math.sin(a) * r); } g.stroke();
  } },
  switchPlate: { w: 32, h: 48, draw: (g, w, h) => { g.fillStyle = '#e4dcc8'; g.fillRect(0, 0, w, h); g.fillStyle = '#2a2a28'; g.fillRect(13, 16, 6, 16); g.fillStyle = '#b8b0a0'; g.fillRect(15, 5, 2, 2); g.fillRect(15, h - 7, 2, 2); } },
  outlet: { w: 32, h: 48, draw: (g, w, h) => { g.fillStyle = '#e4dcc8'; g.fillRect(0, 0, w, h); g.fillStyle = '#3a3a36'; for (const y of [10, 28]) { g.fillRect(10, y, 3, 7); g.fillRect(19, y, 3, 7); g.fillRect(15, y + 8, 2, 3); } } },
  vent: { w: 96, h: 64, draw: (g, w, h) => { g.fillStyle = '#8a8c88'; g.fillRect(0, 0, w, h); g.fillStyle = '#2a2c2c'; for (let y = 6; y < h - 4; y += 7) g.fillRect(6, y, w - 12, 4); } },
  fireExt: { w: 96, h: 48, draw: card([['FIRE', 14, '#f2ead8'], ['EXTINGUISHER', 9, '#f2ead8']], '#8c2a20', '#f2ead8') },
  ringBell: { w: 96, h: 56, draw: card([['PLEASE RING', 12], ['FOR SERVICE', 12]], '#efe6cf', '#7a2e22') },
  // the motel's brochures: real places, nobody's brand
  brochureA: { w: 64, h: 96, draw: (g, w, h) => {
    g.fillStyle = '#1e2a3a'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#c8a878'; g.beginPath(); g.moveTo(0, 70); g.quadraticCurveTo(w / 2, 30, w, 70); g.lineTo(w, h); g.lineTo(0, h); g.fill();
    g.fillStyle = '#e8d8b0'; for (let i = 0; i < 6; i++) g.fillRect(8 + i * 9, 34 + (i % 2) * 6, 3, 14 - (i % 3) * 3);
    text(g, 'THE CAVERNS', w / 2, 16, OSW(10, 700), '#f0e6d0', w - 6, 'center'); text(g, '150 MI SOUTH', w / 2, 28, OSW(8, 500), '#c8b898', w - 6, 'center');
  } },
  brochureB: { w: 64, h: 96, draw: (g, w, h) => {
    const s = g.createLinearGradient(0, 0, 0, h); s.addColorStop(0, '#6f97c4'); s.addColorStop(0.55, '#d8e4ec'); s.addColorStop(0.56, '#f4f2ec'); s.addColorStop(1, '#e2ded4');
    g.fillStyle = s; g.fillRect(0, 0, w, h); g.fillStyle = 'rgba(180,170,150,.5)'; g.beginPath(); g.moveTo(0, 70); g.quadraticCurveTo(w * 0.4, 56, w, 74); g.lineTo(w, 80); g.quadraticCurveTo(w * 0.5, 64, 0, 78); g.fill();
    text(g, 'WHITE SANDS', w / 2, 16, OSW(10, 700), '#1e2a3a', w - 6, 'center'); text(g, 'SEE THE DUNES', w / 2, 28, OSW(8, 500), '#2a3a4a', w - 6, 'center');
  } },
} satisfies Record<string, { w: number; h: number; draw: Draw }>;
export type Thing = keyof typeof THINGS;

// ---------- the dirt (white on clear: the corners colour it) ----------
const blot = (g: CanvasRenderingContext2D, x: number, y: number, rx: number, ry: number, a: number) => {
  const gr = g.createRadialGradient(x, y, 0, x, y, Math.max(rx, ry));
  gr.addColorStop(0, `rgba(255,255,255,${a})`); gr.addColorStop(1, 'rgba(255,255,255,0)');
  g.save(); g.translate(x, y); g.scale(rx / Math.max(rx, ry), ry / Math.max(rx, ry)); g.translate(-x, -y);
  g.fillStyle = gr; g.beginPath(); g.arc(x, y, Math.max(rx, ry), 0, Math.PI * 2); g.fill(); g.restore();
};
// a smooth round falloff, for shadows and pools of light
const soft = (g: CanvasRenderingContext2D, x: number, y: number, rad: number) => {
  const gr = g.createRadialGradient(x, y, 0, x, y, rad);
  for (const [k, a] of [[0, 1], [0.25, 0.84], [0.5, 0.46], [0.75, 0.14], [1, 0]]) gr.addColorStop(k, `rgba(255,255,255,${a})`);
  g.fillStyle = gr; g.fillRect(x - rad, y - rad, rad * 2, rad * 2);
};
const DIRT = {
  // the floor where everybody walks: a soft patch, darker in the middle, a few short streaks
  // (a patch is never stretched much past 2 m: a corridor gets several 'lane' patches instead)
  wear: { w: 256, h: 128, draw: (g, w, h) => {
    blot(g, w / 2, h / 2, w * 0.48, h * 0.42, 0.5); blot(g, w * 0.45, h * 0.5, w * 0.3, h * 0.25, 0.35);
    const r = rng(3); g.strokeStyle = 'rgba(255,255,255,.09)'; g.lineCap = 'round';
    for (let i = 0; i < 40; i++) { g.lineWidth = 1 + r() * 2; const y = h * (0.25 + r() * 0.5), x = w * (0.18 + r() * 0.55); g.beginPath(); g.moveTo(x, y); g.lineTo(x + 8 + r() * 22, y + (r() - 0.5) * 5); g.stroke(); }
  } },
  // the same wear without strokes: a soft mottled patch, for overlapping down a corridor or an aisle
  lane: { w: 128, h: 128, draw: (g, w, h) => {
    const r = rng(61);
    blot(g, w / 2, h / 2, w * 0.46, h * 0.4, 0.32);
    for (let i = 0; i < 46; i++) { const a = r() * Math.PI * 2, d = Math.sqrt(r()) * 0.7; blot(g, w / 2 + Math.cos(a) * d * w * 0.38, h / 2 + Math.sin(a) * d * h * 0.32, 5 + r() * 14, 4 + r() * 9, 0.05 + r() * 0.1); }
    g.fillStyle = 'rgba(255,255,255,.14)';
    for (let i = 0; i < 260; i++) { const a = r() * Math.PI * 2, d = Math.sqrt(r()) * 0.85; g.fillRect(w / 2 + Math.cos(a) * d * w * 0.42, h / 2 + Math.sin(a) * d * h * 0.36, 1, 1); }
  } },
  // the ring the casters wear round a chair that is turned all night: uneven, with short arcs
  casters: { w: 128, h: 128, draw: (g) => {
    const r = rng(67); g.lineCap = 'round';
    for (let i = 0; i < 90; i++) { const a = r() * Math.PI * 2, d = 36 + (r() - 0.5) * 20; blot(g, 64 + Math.cos(a) * d, 64 + Math.sin(a) * d, 4 + r() * 8, 4 + r() * 8, 0.06 + r() * 0.09); }
    for (let i = 0; i < 16; i++) { const a0 = r() * Math.PI * 2, d = 26 + r() * 24; g.strokeStyle = `rgba(255,255,255,${0.08 + r() * 0.14})`; g.lineWidth = 1 + r() * 1.5; g.beginPath(); g.arc(64, 64, d, a0, a0 + 0.3 + r() * 0.8); g.stroke(); }
  } },
  // a soft round shadow where a thing meets the floor; coloured light, a pool of lamplight
  shadow: { w: 64, h: 64, draw: (g) => soft(g, 32, 32, 31) },
  // a soft band, strongest at its bottom edge: at the foot of a wall, round a cabinet's base,
  // up a corner, along the ceiling (turned so that edge is against the wall)
  edge: { w: 32, h: 64, draw: (g, w, h) => {
    const gr = g.createLinearGradient(0, h, 0, 0);
    for (const [k, a] of [[0, 1], [0.2, 0.8], [0.45, 0.46], [0.72, 0.17], [1, 0]]) gr.addColorStop(k, `rgba(255,255,255,${a})`);
    g.fillStyle = gr; g.fillRect(0, 0, w, h);
  } },
  // the same band fading out at both ends, round the foot of a cabinet or a bed
  band: { w: 64, h: 64, draw: (g, w, h) => {
    for (let x = 0; x < w; x++) {
      const t = Math.min(1, Math.min(x + 0.5, w - x - 0.5) / (w * 0.22)), e = t * t * (3 - 2 * t), gr = g.createLinearGradient(0, h, 0, 0);
      for (const [k, a] of [[0, 1], [0.2, 0.8], [0.45, 0.46], [0.72, 0.17], [1, 0]]) gr.addColorStop(k, `rgba(255,255,255,${a * e})`);
      g.fillStyle = gr; g.fillRect(x, 0, 1, h);
    }
  } },
  // the shine of a tube light along a polished top: a long soft band, fading at its ends
  streak: { w: 64, h: 32, draw: (g, w, h) => {
    for (let x = 0; x < w; x++) {
      const e = Math.pow(Math.sin(Math.PI * (x + 0.5) / w), 1.5), gr = g.createLinearGradient(0, 0, 0, h);
      gr.addColorStop(0, 'rgba(255,255,255,0)'); gr.addColorStop(0.5, `rgba(255,255,255,${e})`); gr.addColorStop(1, 'rgba(255,255,255,0)');
      g.fillStyle = gr; g.fillRect(x, 0, 1, h);
    }
  } },
  // shoe marks low on a counter's front: soft smudges, a few short grey strokes
  kick: { w: 128, h: 32, draw: (g, w, h) => {
    const r = rng(71); g.lineCap = 'round';
    for (let i = 0; i < 8; i++) {
      const x = 14 + r() * (w - 28), y = 10 + r() * (h - 20);
      blot(g, x, y, 6 + r() * 10, 3 + r() * 3, 0.14 + r() * 0.18);
      if (r() < 0.6) { g.strokeStyle = `rgba(255,255,255,${0.18 + r() * 0.22})`; g.lineWidth = 1 + r(); g.beginPath(); g.moveTo(x - 6, y + (r() - 0.5) * 3); g.quadraticCurveTo(x, y - 1 - r() * 3, x + 4 + r() * 7, y + (r() - 0.5) * 3); g.stroke(); }
    }
  } },
  scuff: { w: 256, h: 64, draw: (g, w, h) => {
    const r = rng(7); g.lineCap = 'round';
    for (let i = 0; i < 26; i++) { g.strokeStyle = `rgba(255,255,255,${0.25 + r() * 0.5})`; g.lineWidth = 1 + r() * 3; const x = r() * (w - 40), y = 8 + r() * (h - 16);
      g.beginPath(); g.moveTo(x, y); g.quadraticCurveTo(x + 10 + r() * 20, y + (r() - 0.5) * 8, x + 15 + r() * 30, y + (r() - 0.5) * 6); g.stroke(); }
  } },
  // a water stain: a blotch with darker tide lines round it
  stain: { w: 128, h: 128, draw: (g, w, h) => {
    blot(g, 64, 64, 56, 50, 0.35);
    const r = rng(9); g.strokeStyle = 'rgba(255,255,255,.55)';
    for (const k of [0.95, 0.75, 0.55]) { g.lineWidth = 1.5; g.beginPath(); for (let a = 0; a <= Math.PI * 2 + 0.01; a += 0.2) { const rr = 56 * k * (0.86 + r() * 0.14); g.lineTo(64 + Math.cos(a) * rr, 64 + Math.sin(a) * rr * 0.9); } g.stroke(); }
  } },
  drip: { w: 64, h: 128, draw: (g, w, h) => {
    const r = rng(13);
    for (let i = 0; i < 7; i++) { const x = 8 + r() * (w - 16), len = h * (0.4 + r() * 0.6), gr = g.createLinearGradient(0, 0, 0, len);
      gr.addColorStop(0, 'rgba(255,255,255,.7)'); gr.addColorStop(1, 'rgba(255,255,255,0)'); g.fillStyle = gr; g.fillRect(x, 0, 2 + r() * 4, len); }
    blot(g, w / 2, 6, w * 0.45, 8, 0.6);
  } },
  rings: { w: 128, h: 64, draw: (g) => {
    for (const [x, y, a] of [[30, 30, 0.6], [44, 38, 0.4], [96, 28, 0.5]]) { g.strokeStyle = `rgba(255,255,255,${a})`; g.lineWidth = 3; g.beginPath(); g.arc(x, y, 20, 0.3, Math.PI * 1.85); g.stroke(); g.lineWidth = 1; g.beginPath(); g.arc(x, y, 17, 0, Math.PI * 2); g.stroke(); }
  } },
  smudge: { w: 64, h: 64, draw: (g) => { blot(g, 32, 32, 28, 22, 0.5); const r = rng(19); for (let i = 0; i < 6; i++) blot(g, 20 + r() * 24, 20 + r() * 24, 5 + r() * 6, 5 + r() * 6, 0.35); } },
  // dirt gathered along a skirting or in a corner: strongest at the bottom edge
  dust: { w: 256, h: 32, draw: (g, w, h) => {
    const gr = g.createLinearGradient(0, h, 0, 0); gr.addColorStop(0, 'rgba(255,255,255,.7)'); gr.addColorStop(1, 'rgba(255,255,255,0)');
    g.fillStyle = gr; g.fillRect(0, 0, w, h); const r = rng(29); g.fillStyle = 'rgba(255,255,255,.4)'; for (let i = 0; i < 200; i++) g.fillRect(r() * w, h - r() * r() * h, 1, 1);
  } },
  web: { w: 128, h: 128, draw: (g) => {
    g.strokeStyle = 'rgba(255,255,255,.55)'; g.lineWidth = 1;
    const spokes = [0.02, 0.3, 0.6, 0.9, 1.2, 1.5];
    for (const a of spokes) { g.beginPath(); g.moveTo(0, 0); g.lineTo(Math.cos(a) * 128, Math.sin(a) * 128); g.stroke(); }
    for (let r = 14; r < 120; r += 13) { g.beginPath(); spokes.forEach((a, i) => { const k = r * (1 + (i % 2) * 0.06); i ? g.lineTo(Math.cos(a) * k, Math.sin(a) * k) : g.moveTo(Math.cos(a) * k, Math.sin(a) * k); }); g.stroke(); }
  } },
  boots: { w: 128, h: 64, draw: (g) => {
    const sole = (x: number, y: number, a: number) => { g.save(); g.translate(x, y); g.rotate(a); blot(g, 0, -6, 6, 9, 0.55); blot(g, 0, 9, 5, 5, 0.5); g.restore(); };
    sole(20, 24, 1.5); sole(44, 40, 1.6); sole(70, 22, 1.5); sole(96, 40, 1.55); sole(118, 24, 1.5);
  } },
  crack: { w: 128, h: 128, draw: (g) => {
    const r = rng(37); g.strokeStyle = 'rgba(255,255,255,.75)'; g.lineWidth = 1.2;
    const go = (x: number, y: number, a: number, n: number) => { g.beginPath(); g.moveTo(x, y); for (let i = 0; i < n; i++) { a += (r() - 0.5) * 0.9; x += Math.cos(a) * 6; y += Math.sin(a) * 6; g.lineTo(x, y); if (r() < 0.12 && n > 6) { g.stroke(); go(x, y, a + (r() - 0.5) * 2, Math.floor(n / 2)); g.beginPath(); g.moveTo(x, y); } } g.stroke(); };
    go(6, 10, 0.9, 20);
  } },
  burn: { w: 32, h: 32, draw: (g) => { blot(g, 16, 16, 14, 12, 0.6); blot(g, 16, 16, 5, 4, 0.95); } },
  splash: { w: 128, h: 128, draw: (g) => { const r = rng(43); for (let i = 0; i < 26; i++) blot(g, 10 + r() * 108, 10 + r() * 108, 3 + r() * 14, 3 + r() * 10, 0.25 + r() * 0.35); } },
  // pencil on the wall by a payphone
  scrawl: { w: 128, h: 64, draw: (g) => {
    text(g, 'TAXI 555-0186', 6, 24, HAND(20), 'rgba(255,255,255,.85)', 116); text(g, 'call Ray', 20, 48, HAND(22), 'rgba(255,255,255,.75)', 100);
    g.strokeStyle = 'rgba(255,255,255,.6)'; g.lineWidth = 1.5; g.strokeRect(2, 4, 120, 26);
  } },
} satisfies Record<string, { w: number; h: number; draw: Draw }>;
export type Dirt = keyof typeof DIRT;

// ---------- the two pictures: packed in shelves, drawn once ----------
type Sheet = { w: number; h: number; rects: Record<string, Rect>; tex: THREE.CanvasTexture | null };
function pack(cells: Record<string, { w: number; h: number }>, w: number, h: number): Record<string, Rect> {
  const pad = 4, out: Record<string, Rect> = {};
  const order = Object.keys(cells).sort((a, b) => cells[b].h - cells[a].h);
  let x = 0, y = 0, row = 0;
  for (const k of order) {
    const c = cells[k];
    if (x + c.w > w) { x = 0; y += row + pad; row = 0; }
    out[k] = { x, y, w: c.w, h: c.h };
    x += c.w + pad; row = Math.max(row, c.h);
  }
  if (y + row > h) console.warn('clutter atlas too small', w, h);
  return out;
}
const things: Sheet = { w: 1024, h: 1280, rects: pack(THINGS, 1024, 1280), tex: null };
const dirt: Sheet = { w: 512, h: 640, rects: pack(DIRT, 512, 640), tex: null };
function sheet(s: Sheet, cells: Record<string, { draw: Draw }>, clear: boolean) {
  if (s.tex) return s.tex;
  const c = document.createElement('canvas'); c.width = s.w; c.height = s.h;
  const g = c.getContext('2d')!;
  if (!clear) { g.fillStyle = '#fff'; g.fillRect(0, 0, s.w, s.h); }
  for (const [k, r] of Object.entries(s.rects)) {
    g.save(); g.beginPath(); g.rect(r.x, r.y, r.w, r.h); g.clip(); g.translate(r.x, r.y);
    cells[k].draw(g, r.w, r.h); g.restore();
  }
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 4; t.name = clear ? 'clutter/dirt' : 'clutter/things';
  s.tex = t;
  return t;
}
/** The things' picture (shared, never disposed). */
export function thingsAtlas() { return sheet(things, THINGS, false); }
/** The dirt's picture (shared, never disposed). */
export function dirtAtlas() { return sheet(dirt, DIRT, true); }

// a picture's corner in the atlas, as uv: [u0, v0, u1, v1] (v up)
function uvRect(s: Sheet, k: string, inset = 0): [number, number, number, number] {
  const r = s.rects[k];
  return [(r.x + inset) / s.w, 1 - (r.y + r.h - inset) / s.h, (r.x + r.w - inset) / s.w, 1 - (r.y + inset) / s.h];
}
const WHITE = () => uvRect(things, 'white', 40);

// The dirt's material lets a mark shine of itself by its corners' glow (a pool of lamplight under a
// lamp that lights nothing else, the shine along a counter), on top of what the room's lamps do
// (kit.floodlit or the real lights): its own colour, times the glow, as emissive light.
function shines(m: THREE.MeshStandardMaterial) {
  const before = m.onBeforeCompile, key = m.customProgramCacheKey.bind(m);
  m.onBeforeCompile = (sh, r) => {
    before.call(m, sh, r);
    sh.vertexShader = sh.vertexShader.replace('#include <common>', '#include <common>\nattribute float glow;\nvarying float vGlow;')
      .replace('#include <begin_vertex>', '#include <begin_vertex>\nvGlow = glow;');
    sh.fragmentShader = sh.fragmentShader.replace('#include <common>', '#include <common>\nvarying float vGlow;')
      .replace('#include <emissivemap_fragment>', '#include <emissivemap_fragment>\ntotalEmissiveRadiance += diffuseColor.rgb * vGlow;');
  };
  m.customProgramCacheKey = () => key() + '+glow';
  return m;
}

// ---------- the wall clocks ----------
// The control room's clock is driven by the night's clock every frame (ControlRoom.setClock);
// it passes the time on here, and every other clock in the night shows the same.
let wallSecs = 0;
export function setWallClock(seconds: number) { wallSecs = seconds; }

// ---------- building a room's things ----------
export type At = { rx?: number; ry?: number; rz?: number; fine?: boolean };
/** A mark of dirt or light: a, how strong (0 to 1); low, drawn on Low too (the shadows); glow, how
 *  much it shines of itself (a pool of lamplight, a shine: lit by its lamp, not by the room). */
export type Mark = At & { a?: number; low?: boolean; glow?: number };
type Part = { pos: number[]; nor: number[]; uv: number[]; col: number[]; rgba?: boolean; glow?: number[] };
const _m = new THREE.Matrix4(), _q = new THREE.Quaternion(), _e = new THREE.Euler(), _p = new THREE.Vector3(), _one = new THREE.Vector3(1, 1, 1), _c = new THREE.Color();
const _up = new THREE.Vector3(0, 1, 0);
export const v3 = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);

export class Clutter {
  private base: Part = { pos: [], nor: [], uv: [], col: [] };
  private fine: Part = { pos: [], nor: [], uv: [], col: [] };
  // the dirt mesh: first the shade (contact shadows, the dark along the walls and in the
  // corners; Low draws only this), then the rest (wear, stains, light). Colour and strength per corner.
  private ground: Part = { pos: [], nor: [], uv: [], col: [], rgba: true, glow: [] };
  private grime: Part = { pos: [], nor: [], uv: [], col: [], rgba: true, glow: [] };
  private clocks: { x: number; y: number; z: number; ry: number; r: number }[] = [];
  private shade = rng(5);

  /** mat makes the room's material (its lamps: kit.floodlit with its flood set, or plain). */
  constructor(readonly name: string, private mat: (p: THREE.MeshStandardMaterialParameters) => THREE.MeshStandardMaterial) {}

  // Puts a geometry into a part: uvs into the atlas (groups[i] -> rects[i], the rest white),
  // turned (Euler YXZ, as kit.plane) and moved, coloured per corner with a little variation.
  private put(geo: THREE.BufferGeometry, color: THREE.ColorRepresentation, x: number, y: number, z: number, o: At, rects: ([number, number, number, number] | null)[] = [], to?: Part, vary = 0.06, alpha = 1, glow = 0) {
    const uv = geo.attributes.uv as THREE.BufferAttribute;
    const map = (i: number, r: [number, number, number, number]) => uv.setXY(i, r[0] + uv.getX(i) * (r[2] - r[0]), r[1] + uv.getY(i) * (r[3] - r[1]));
    const done = new Uint8Array(uv.count);
    const idx = geo.index;
    geo.groups.forEach((gr, gi) => {
      const r = rects[gi]; if (!r) return;
      for (let k = gr.start; k < gr.start + gr.count; k++) { const i = idx ? idx.getX(k) : k; if (!done[i]) { done[i] = 1; map(i, r); } }
    });
    if (!geo.groups.length && rects[0]) for (let i = 0; i < uv.count; i++) { done[i] = 1; map(i, rects[0]); }
    const w = WHITE();
    for (let i = 0; i < uv.count; i++) if (!done[i]) uv.setXY(i, (w[0] + w[2]) / 2 + (uv.getX(i) - 0.5) * (w[2] - w[0]), (w[1] + w[3]) / 2 + (uv.getY(i) - 0.5) * (w[3] - w[1]));
    _e.set(o.rx ?? 0, o.ry ?? 0, o.rz ?? 0, 'YXZ');
    geo.applyMatrix4(_m.compose(_p.set(x, y, z), _q.setFromEuler(_e), _one));
    const g = geo.index ? geo.toNonIndexed() : geo;
    const part = to ?? (o.fine ? this.fine : this.base);
    const k = 1 - vary + this.shade() * vary * 1.4;
    _c.set(color).multiplyScalar(k);
    const P = g.attributes.position, N = g.attributes.normal, U = g.attributes.uv;
    for (let i = 0; i < P.count; i++) {
      part.pos.push(P.getX(i), P.getY(i), P.getZ(i)); part.nor.push(N.getX(i), N.getY(i), N.getZ(i));
      part.uv.push(U.getX(i), U.getY(i)); part.col.push(_c.r, _c.g, _c.b);
      if (part.rgba) part.col.push(alpha);
      part.glow?.push(glow);
    }
    geo.dispose(); if (g !== geo) g.dispose();
  }

  // ---------- shapes ----------
  /** A box; faces may carry a picture: [+x, -x, +y, -y, +z, -z]. */
  box(w: number, h: number, d: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At & { faces?: (Thing | null)[] } = {}) {
    this.put(new THREE.BoxGeometry(w, h, d), color, x, y, z, o, (o.faces ?? []).map((t) => t ? uvRect(things, t, 1) : null));
  }
  /** A flat picture facing +z, turned by o. */
  plane(w: number, h: number, x: number, y: number, z: number, t: Thing, o: At & { color?: THREE.ColorRepresentation } = {}) {
    this.put(new THREE.PlaneGeometry(w, h), o.color ?? 0xffffff, x, y, z, o, [uvRect(things, t, 1)]);
  }
  /** A sheet lying on a surface at height y, turned ry about the vertical. */
  sheet(w: number, d: number, x: number, y: number, z: number, t: Thing, ry = 0, o: At & { color?: THREE.ColorRepresentation } = {}) {
    this.plane(w, d, x, y, z, t, { ...o, rx: -Math.PI / 2, ry });
  }
  /** A cylinder standing at (x, y, z) (its middle); the top may carry a picture. */
  cyl(rt: number, rb: number, h: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At & { seg?: number; top?: Thing; side?: Thing } = {}) {
    this.put(new THREE.CylinderGeometry(rt, rb, h, o.seg ?? 10), color, x, y, z, o, [o.side ? uvRect(things, o.side, 1) : null, o.top ? uvRect(things, o.top, 1) : null, null]);
  }
  /** A round picture facing +z (a clock face, a fan's grille). */
  disc(r: number, x: number, y: number, z: number, t: Thing, o: At & { seg?: number; color?: THREE.ColorRepresentation } = {}) {
    this.put(new THREE.CircleGeometry(r, o.seg ?? 20), o.color ?? 0xffffff, x, y, z, o, [uvRect(things, t, 1)]);
  }
  /** A thin rod from a to b. */
  rod(a: THREE.Vector3, b: THREE.Vector3, r: number, color: THREE.ColorRepresentation, o: { fine?: boolean; seg?: number } = {}) {
    const d = new THREE.Vector3().subVectors(b, a), len = d.length();
    const g = new THREE.CylinderGeometry(r, r, len, o.seg ?? 5, 1, true);
    g.applyQuaternion(_q.setFromUnitVectors(_up, d.normalize()));
    this.put(g, color, (a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2, { fine: o.fine });
  }
  /** A cable through the points (a soft curve). */
  cable(pts: THREE.Vector3[], r: number, color: THREE.ColorRepresentation, o: { fine?: boolean } = {}) {
    const curve = new THREE.CatmullRomCurve3(pts, false, 'catmullrom', 0.2);
    const len = curve.getLength();
    this.put(new THREE.TubeGeometry(curve, Math.max(4, Math.ceil(len / 0.08)), r, 4, false), color, 0, 0, 0, { fine: o.fine ?? true });
  }
  /** A box narrower or thinner at the top than at the bottom (a coat's body, a sleeve); faces as box. */
  slab(wTop: number, wBot: number, h: number, dTop: number, dBot: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At & { faces?: (Thing | null)[] } = {}) {
    const g = new THREE.BoxGeometry(1, 1, 1), p = g.attributes.position as THREE.BufferAttribute;
    for (let i = 0; i < p.count; i++) { const top = p.getY(i) > 0; p.setXYZ(i, p.getX(i) * (top ? wTop : wBot), p.getY(i) * h, p.getZ(i) * (top ? dTop : dBot)); }
    g.computeVertexNormals();
    this.put(g, color, x, y, z, o, (o.faces ?? []).map((t) => t ? uvRect(things, t, 1) : null));
  }
  /** Half a sphere, its round side up before turning (a hard hat). */
  dome(r: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At = {}) {
    this.put(new THREE.SphereGeometry(r, 12, 5, 0, Math.PI * 2, 0, Math.PI / 2), color, x, y, z, o);
  }
  torus(r: number, tube: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At & { arc?: number } = {}) {
    this.put(new THREE.TorusGeometry(r, tube, 4, 10, o.arc ?? Math.PI * 2), color, x, y, z, o);
  }
  ball(r: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At = {}) {
    this.put(new THREE.IcosahedronGeometry(r, 0), color, x, y, z, o);
  }
  /** Dirt: a see-through picture facing +z, turned by o (on Low only if o.low). */
  dirt(w: number, h: number, x: number, y: number, z: number, d: Dirt, color: THREE.ColorRepresentation, o: Mark = {}) {
    this.put(new THREE.PlaneGeometry(w, h), color, x, y, z, o, [uvRect(dirt, d, 1)], o.low ? this.ground : this.grime, 0.1, o.a ?? 1, o.glow ?? 0);
  }
  /** Dirt on the floor (5 mm up), turned ry about the vertical. */
  floorDirt(w: number, d: number, x: number, z: number, t: Dirt, color: THREE.ColorRepresentation, ry = 0, y = 0.005, o: Mark = {}) {
    this.dirt(w, d, x, y, z, t, color, { ...o, rx: -Math.PI / 2, ry });
  }

  // ---------- things ----------
  mug(x: number, y: number, z: number, color: THREE.ColorRepresentation = 0xe8e2d4, o: { ry?: number; full?: boolean; fine?: boolean } = {}) {
    const f = { fine: o.fine };
    this.cyl(0.04, 0.036, 0.095, x, y + 0.0475, z, color, { ...f, seg: 12 });
    this.cyl(0.036, 0.036, 0.004, x, y + 0.094, z, o.full ? 0x2a160a : 0x5a4030, { ...f, seg: 12 });
    const a = o.ry ?? 0;
    this.torus(0.024, 0.0065, x + Math.cos(a) * 0.044, y + 0.05, z - Math.sin(a) * 0.044, color, { ...f, ry: a, rz: -Math.PI / 2, arc: Math.PI });
  }
  ashtray(x: number, y: number, z: number, butts = 3, color: THREE.ColorRepresentation = 0x8a5a2e) {
    this.cyl(0.065, 0.07, 0.024, x, y + 0.012, z, color, { seg: 12, top: 'ashtray' });
    const r = rng(Math.floor(x * 97 + z * 31));
    for (let i = 0; i < butts; i++) {
      const a = r() * Math.PI * 2, d = 0.012 + r() * 0.03, ry = r() * Math.PI;
      const bx = x + Math.cos(a) * d, bz = z + Math.sin(a) * d;
      this.cyl(0.0045, 0.0045, 0.03, bx, y + 0.03, bz, 0xd8b078, { fine: true, rz: Math.PI / 2 - 0.2, ry, seg: 5 });
    }
  }
  pencil(x: number, y: number, z: number, ry: number, color: THREE.ColorRepresentation = 0xd9a520) {
    this.cyl(0.0038, 0.0038, 0.17, x, y + 0.004, z, color, { fine: true, rz: Math.PI / 2, ry, seg: 6 });
  }
  /** A stack of n sheets; the top one shows t. */
  papers(x: number, y: number, z: number, w: number, d: number, t: Thing, o: { ry?: number; n?: number; fine?: boolean } = {}) {
    const n = o.n ?? 1, r = rng(Math.floor(x * 53 + z * 71));
    for (let i = 0; i < n - 1; i++) this.box(w, 0.002, d, x + (r() - 0.5) * 0.02, y + 0.001 + i * 0.0025, z + (r() - 0.5) * 0.02, 0xe8e0c8, { ry: (o.ry ?? 0) + (r() - 0.5) * 0.12, fine: o.fine });
    this.sheet(w, d, x, y + 0.0035 + (n - 1) * 0.0025, z, t, o.ry ?? 0, { fine: o.fine });
  }
  /** A binder standing on its edge (spine towards +x before turning), or lying flat. */
  binder(x: number, y: number, z: number, color: THREE.ColorRepresentation, o: { ry?: number; flat?: boolean; fine?: boolean } = {}) {
    if (o.flat) this.box(0.3, 0.06, 0.27, x, y + 0.03, z, color, { ry: o.ry, fine: o.fine });
    else this.box(0.06, 0.3, 0.27, x, y + 0.15, z, color, { ry: o.ry, fine: o.fine });
  }
  calculator(x: number, y: number, z: number, ry = 0) {
    this.box(0.08, 0.016, 0.12, x, y + 0.008, z, 0x2a2c2e, { ry, faces: [null, null, 'calc'] });
  }
  /** A spare tape reel lying flat. */
  reel(x: number, y: number, z: number, r = 0.13) {
    this.cyl(r, r, 0.014, x, y + 0.007, z, 0xffffff, { seg: 18, top: 'reel' });
  }
  /** A waste basket with paper balls on top (the dark inside is its top). */
  bin(x: number, z: number, y = 0, color: THREE.ColorRepresentation = 0x3a3c3e) {
    this.cyl(0.15, 0.12, 0.32, x, y + 0.16, z, color, { seg: 12 });
    this.cyl(0.14, 0.14, 0.004, x, y + 0.318, z, 0x101112, { seg: 12 });
    const r = rng(Math.floor(x * 13 + z * 7));
    for (let i = 0; i < 3; i++) this.ball(0.045, x + (r() - 0.5) * 0.12, y + 0.33, z + (r() - 0.5) * 0.12, 0xe6dfcb, { fine: true, ry: r() * 3 });
  }
  /** A picture in a frame on a wall: (x, y, z) is the front of the wall, ry faces the room. */
  frame(x: number, y: number, z: number, ry: number, w: number, h: number, t: Thing, color: THREE.ColorRepresentation = 0x5a3a24, o: { fine?: boolean } = {}) {
    const s = Math.sin(ry), c = Math.cos(ry), b = 0.03, d = 0.025;
    const at = (u: number, v: number, n: number): [number, number, number] => [x + c * u + s * n, y + v, z - s * u + c * n];
    this.box(w + b * 2, b, d, ...at(0, h / 2 + b / 2, d / 2), color, { ry, fine: o.fine });
    this.box(w + b * 2, b, d, ...at(0, -h / 2 - b / 2, d / 2), color, { ry, fine: o.fine });
    this.box(b, h, d, ...at(-w / 2 - b / 2, 0, d / 2), color, { ry, fine: o.fine });
    this.box(b, h, d, ...at(w / 2 + b / 2, 0, d / 2), color, { ry, fine: o.fine });
    this.plane(w, h, ...at(0, 0, 0.006), t, { ry, fine: o.fine });
  }
  /** A notice taped or pinned to a wall: (x, y, z) at the wall's face, ry faces the room. */
  notice(x: number, y: number, z: number, ry: number, w: number, h: number, t: Thing, o: { tilt?: number; tape?: boolean; pins?: boolean } = {}) {
    const s = Math.sin(ry), c = Math.cos(ry);
    this.plane(w, h, x + s * 0.004, y, z + c * 0.004, t, { ry, rz: o.tilt ?? 0 });
    const tilt = o.tilt ?? 0;
    for (const [u, v] of [[-w / 2 + 0.01, h / 2 - 0.01], [w / 2 - 0.01, h / 2 - 0.01]]) {
      const uu = u * Math.cos(tilt) - v * Math.sin(tilt), vv = u * Math.sin(tilt) + v * Math.cos(tilt);
      if (o.pins) this.cyl(0.006, 0.006, 0.01, x + c * uu + s * 0.009, y + vv, z - s * uu + c * 0.009, 0xc03028, { rx: Math.PI / 2, ry, fine: true, seg: 6 });
      else if (o.tape !== false) this.box(0.035, 0.016, 0.001, x + c * uu + s * 0.0055, y + vv, z - s * uu + c * 0.0055, 0xe0d8b0, { ry, rz: tilt + 0.6, fine: true });
    }
  }
  /** A wall clock: (x, y, z) at the wall's face, ry faces the room. Its hands keep the night's time. */
  clock(x: number, y: number, z: number, ry: number, r = 0.15, rim: THREE.ColorRepresentation = 0x2b3033) {
    const s = Math.sin(ry), c = Math.cos(ry);
    this.cyl(r + 0.012, r + 0.012, 0.035, x + s * 0.0175, y, z + c * 0.0175, rim, { rx: Math.PI / 2, ry, seg: 20 });
    this.disc(r, x + s * 0.036, y, z + c * 0.036, 'clock', { ry, seg: 24 });
    this.clocks.push({ x: x + s * 0.038, y, z: z + c * 0.038, ry, r });
  }
  /** A coat hanging by its loop from a hook: (x, y, z) at the wall's face, the hook at height y,
   *  ry faces the room. The shoulders fall away from the hook, the hem stands off the wall, one
   *  sleeve hangs straight and the other a little forward. knit: a cardigan (ribbed, no collar). */
  coat(x: number, y: number, z: number, ry: number, color: THREE.ColorRepresentation, o: { long?: boolean; knit?: boolean } = {}) {
    const s = Math.sin(ry), c = Math.cos(ry), L = o.long ? 0.92 : 0.64, out = 0.055;   // out: how far the hem stands off
    const at = (u: number, v: number, n: number): [number, number, number] => [x + c * u + s * n, y + v, z - s * u + c * n];
    const dark = new THREE.Color(color).multiplyScalar(0.72), side = new THREE.Color(color).multiplyScalar(0.88);
    const f = o.knit ? { faces: [null, null, null, null, 'knit', null] as (Thing | null)[] } : {};
    const tilt = -Math.atan2(out, L);   // the body leans out from the shoulders to the hem
    this.box(0.05, 0.06, 0.01, ...at(0, 0.02, 0.005), 0x6c7074, { ry });                    // the hook's plate
    this.rod(v3(...at(0, 0.0, 0.01)), v3(...at(0, -0.02, 0.07)), 0.006, 0x6c7074);
    this.slab(0.12, 0.4, 0.16, 0.06, 0.11, ...at(0, -0.09, 0.07), color, { ry, ...f });             // the shoulders
    this.slab(0.37, 0.48, L, 0.11, 0.1, ...at(0, -0.17 - L / 2, 0.075 + out / 2), color, { ry, rx: tilt, ...f });
    this.slab(0.48, 0.5, 0.05, 0.1, 0.1, ...at(0, -0.17 - L + 0.025, 0.077 + out), o.knit ? dark : side, { ry, rx: tilt });   // the hem, turned up or ribbed
    for (const k of [-1, 1]) {
      // the left sleeve hangs straight down the side, the right one forward and in a little
      const fw = k > 0 ? 0.035 : 0, rz = k > 0 ? 0.08 : 0.03;
      this.slab(0.12, 0.1, 0.56, 0.1, 0.085, ...at(k * (0.215 - fw * 0.4), -0.45, 0.1 + fw), side, { ry, rz: -k * rz, rx: -fw, ...f });
      this.box(0.1, 0.05, 0.088, ...at(k * (0.235 - fw * 0.9 - rz * 0.25), -0.74, 0.1 + fw * 1.8), dark, { ry, rz: -k * rz, rx: -fw, fine: true });   // the cuffs
      if (!o.knit) this.box(0.12, 0.022, 0.014, ...at(k * 0.12, -0.17 - L * 0.62, 0.137 + out * 0.62), dark, { ry, rx: tilt, fine: true });   // the pocket flaps
    }
    if (!o.knit) for (const k of [-1, 1]) this.box(0.1, 0.08, 0.03, ...at(k * 0.045, -0.05, 0.12), dark, { ry, rz: k * 0.55 });   // the collar, turned down
    this.box(0.012, L * 0.92, 0.006, ...at(0.012, -0.2 - L * 0.46, 0.13 + out * 0.5), dark, { ry, rx: tilt, fine: true });   // where it buttons
    for (let i = 0; i < 4; i++) this.box(0.016, 0.016, 0.008, ...at(0.03, -0.24 - i * L * 0.2, 0.133 + out * (0.1 + i * 0.2)), 0x2a2420, { ry, fine: true });
  }
  /** A desk fan facing ry. */
  fan(x: number, y: number, z: number, ry: number, color: THREE.ColorRepresentation = 0x4e5a58) {
    const s = Math.sin(ry), c = Math.cos(ry);
    this.cyl(0.08, 0.09, 0.03, x, y + 0.015, z, color, { seg: 12 });
    this.rod(v3(x, y + 0.03, z), v3(x, y + 0.22, z), 0.012, 0x9a9e9c);
    this.cyl(0.05, 0.05, 0.11, x - s * 0.03, y + 0.26, z - c * 0.03, color, { rx: Math.PI / 2, ry, seg: 10 });
    this.disc(0.13, x + s * 0.045, y + 0.26, z + c * 0.045, 'fan', { ry, seg: 20 });
    this.torus(0.13, 0.006, x + s * 0.044, y + 0.26, z + c * 0.044, 0xb8bcb8, { ry, fine: true });
  }
  /** An electric kettle. */
  kettle(x: number, y: number, z: number, ry = 0, color: THREE.ColorRepresentation = 0xb8b6ae) {
    const s = Math.sin(ry), c = Math.cos(ry);
    this.cyl(0.065, 0.085, 0.16, x, y + 0.08, z, color, { seg: 12 });
    this.cyl(0.05, 0.06, 0.02, x, y + 0.17, z, color, { seg: 12 });
    this.cyl(0.012, 0.012, 0.02, x, y + 0.185, z, 0x1e1f20, { seg: 6 });
    this.rod(v3(x + c * 0.07, y + 0.07, z - s * 0.07), v3(x + c * 0.13, y + 0.15, z - s * 0.13), 0.012, color);
    this.torus(0.055, 0.01, x - c * 0.065, y + 0.11, z + s * 0.065, 0x1e1f20, { ry, rz: Math.PI / 2, arc: Math.PI });
  }
  /** A light switch or an outlet plate on a wall (fine). */
  plate(x: number, y: number, z: number, ry: number, t: 'switchPlate' | 'outlet' = 'switchPlate') {
    this.box(0.07, 0.115, 0.006, x + Math.sin(ry) * 0.003, y, z + Math.cos(ry) * 0.003, 0xe4dcc8, { ry, faces: [null, null, null, null, t], fine: true });
  }
  /** A bottle: body, shoulder, cap. */
  bottle(x: number, y: number, z: number, r: number, h: number, color: THREE.ColorRepresentation, cap: THREE.ColorRepresentation = 0x1e1f20, o: { fine?: boolean } = {}) {
    this.cyl(r, r, h * 0.72, x, y + h * 0.36, z, color, { seg: 10, fine: o.fine });
    this.cyl(r * 0.35, r, h * 0.18, x, y + h * 0.81, z, color, { seg: 10, fine: o.fine });
    this.cyl(r * 0.38, r * 0.38, h * 0.1, x, y + h * 0.95, z, cap, { seg: 8, fine: o.fine });
  }

  // ---------- the meshes ----------
  /** The room's meshes: its things with the clocks' hands (one call) and its dirt (one more). */
  build(): THREE.Mesh[] {
    const out: THREE.Mesh[] = [];
    const geo = (parts: Part[]) => {
      const g = new THREE.BufferGeometry(), cat = (k: 'pos' | 'nor' | 'uv' | 'col') => parts.flatMap((p) => p[k]);
      g.setAttribute('position', new THREE.Float32BufferAttribute(cat('pos'), 3));
      g.setAttribute('normal', new THREE.Float32BufferAttribute(cat('nor'), 3));
      g.setAttribute('uv', new THREE.Float32BufferAttribute(cat('uv'), 2));
      g.setAttribute('color', new THREE.Float32BufferAttribute(cat('col'), parts[0].rgba ? 4 : 3));
      if (parts[0].glow) g.setAttribute('glow', new THREE.Float32BufferAttribute(parts.flatMap((p) => p.glow!), 1));
      g.computeBoundingSphere();
      return g;
    };
    const solidMat = this.mat({ map: thingsAtlas(), vertexColors: true, roughness: 0.78 });
    // the hands after the things Low keeps and before the fine ones: Low draws them too
    const hands = this.clocks.map((k) => this.hands(k)), at = this.base.pos.length / 3;
    const solidGeo = geo([this.base, ...hands.map((h) => h.part), this.fine]), first = at + hands.length * HAND_N;
    const solid = new THREE.Mesh(solidGeo, solidMat);
    solid.name = 'clutter:' + this.name; solid.userData.noMerge = true;
    out.push(solid);
    let grime: THREE.Mesh | null = null;
    const shade = this.ground.pos.length / 3;
    if (this.ground.pos.length || this.grime.pos.length) {
      const m = shines(this.mat({ map: dirtAtlas(), vertexColors: true, roughness: 0.95, transparent: true, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }));
      grime = new THREE.Mesh(geo([this.ground, this.grime]), m);
      grime.name = 'clutter:' + this.name + ':dirt'; grime.userData.noMerge = true; grime.userData.noShadow = true;
      out.push(grime);
    }
    // Low draws the first part of the things only, and of the dirt only the shade (set here: the
    // dirt's own onBeforeRender would never run again once it is hidden)
    let shown = -1;
    solid.onBeforeRender = () => {
      const low = getQuality() === 'low';
      solidGeo.drawRange.count = low ? first : Infinity;
      if (grime) { grime.geometry.drawRange.count = low ? shade : Infinity; grime.visible = !low || shade > 0; }
      // the clocks' hands, when the second has changed (only their corners are sent again)
      const sec = Math.floor(wallSecs);
      if (!hands.length || sec === shown) return;
      shown = sec;
      const p = solidGeo.attributes.position as THREE.BufferAttribute;
      hands.forEach((h, i) => h.set(p.array as Float32Array, (at + i * HAND_N) * 3, sec));
      p.clearUpdateRanges(); p.addUpdateRange(at * 3, hands.length * HAND_N * 3); p.needsUpdate = true;
    };
    return out;
  }

  // A clock's three hands as quads (HAND_N corners) in the things' mesh, in the room's terms: the
  // part to put in it, and how to write their corners for a time.
  private hands(k: { x: number; y: number; z: number; ry: number; r: number }) {
    const shapes = [[0.022, k.r * 0.55, 0.002, 0x1c1d1f], [0.014, k.r * 0.82, 0.004, 0x1c1d1f], [0.004, k.r * 0.9, 0.006, 0xb3261e]];
    const cy = Math.cos(k.ry), sy = Math.sin(k.ry), w = WHITE();
    const part: Part = { pos: [], nor: [], uv: [], col: [] };
    const set = (out: Float32Array | number[], o: number, s: number) => {
      const ang = [-(s / 43200) * Math.PI * 2, -(s / 3600) * Math.PI * 2, -((s % 60) / 60) * Math.PI * 2];
      shapes.forEach(([wd, len, dz], j) => {
        const c = Math.cos(ang[j]), sn = Math.sin(ang[j]), tail = -len * 0.18;
        const q = [[-wd / 2, tail], [wd / 2, tail], [wd / 2, len], [-wd / 2, tail], [wd / 2, len], [-wd / 2, len]];
        q.forEach(([u, v], i) => {
          // on the clock's face (x, y, dz), turned by ry about y and moved to it
          const x = u * c - v * sn, y = u * sn + v * c, z = dz as number, at = o + (j * 6 + i) * 3;
          out[at] = k.x + x * cy + z * sy; out[at + 1] = k.y + y; out[at + 2] = k.z - x * sy + z * cy;
        });
      });
    };
    for (let i = 0; i < HAND_N; i++) {
      part.pos.push(0, 0, 0); part.nor.push(sy, 0, cy); part.uv.push((w[0] + w[2]) / 2, (w[1] + w[3]) / 2);
      _c.set(shapes[Math.floor(i / 6)][3]); part.col.push(_c.r, _c.g, _c.b);
    }
    set(part.pos, 0, Math.floor(wallSecs));
    return { part, set };
  }
}
/** The corners of a clock's hands: three quads of two triangles. */
const HAND_N = 18;
