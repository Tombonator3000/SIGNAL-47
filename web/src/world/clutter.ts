import * as THREE from 'three';
import { artImage } from '../core/art';
import { getQuality } from '../core/quality';
import { rng } from '../core/textures';

// The things that make a room lived in: papers, mugs with rings under them, an ashtray, binders,
// cables along the walls, notices and posters, a coat on a hook, the worn floor by the doors.
// A room asks for them through a Clutter and gets back two meshes, however many things it asks
// for (three with a wall clock):
//  - the things: one material on one picture of small printed things drawn here in code (THINGS),
//    each part coloured per corner, so a hundred of them cost one draw call;
//  - the dirt: worn floor, scuffs, stains, cobwebs, see-through, on a second picture (DIRT).
// Low draws only the things that make the room (the first part of the merged geometry, the parts
// not marked fine) and none of the dirt (world/dressing.ts says which is which).
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
function mesa(g: CanvasRenderingContext2D, w: number, h: number, seed: number, dusk = true) {
  const sky = g.createLinearGradient(0, 0, 0, h * 0.7);
  if (dusk) { sky.addColorStop(0, '#3d4a78'); sky.addColorStop(0.55, '#c9785a'); sky.addColorStop(1, '#f0b26a'); }
  else { sky.addColorStop(0, '#6f97c4'); sky.addColorStop(1, '#d9d2b8'); }
  g.fillStyle = sky; g.fillRect(0, 0, w, h);
  const r = rng(seed);
  g.fillStyle = dusk ? '#5a3a35' : '#8a5a3c';
  g.beginPath(); g.moveTo(0, h * 0.62); g.lineTo(w * 0.12, h * 0.6); g.lineTo(w * 0.2, h * 0.38); g.lineTo(w * 0.46, h * 0.36); g.lineTo(w * 0.52, h * 0.58);
  g.lineTo(w * 0.7, h * 0.56); g.lineTo(w * 0.76, h * 0.44); g.lineTo(w * 0.84, h * 0.45); g.lineTo(w * 0.88, h * 0.6); g.lineTo(w, h * 0.62); g.lineTo(w, h); g.lineTo(0, h); g.fill();
  g.fillStyle = dusk ? '#a8653f' : '#c08a58'; g.fillRect(0, h * 0.66, w, h * 0.34);
  // brush strokes and sage
  for (let i = 0; i < 90; i++) {
    g.fillStyle = r() < 0.5 ? 'rgba(80,90,60,.7)' : 'rgba(150,100,60,.5)';
    g.fillRect(r() * w, h * 0.66 + r() * h * 0.34, 3 + r() * 6, 1 + r() * 2);
  }
  g.fillStyle = '#3f4a32';
  for (let i = 0; i < 3; i++) { const x = w * (0.15 + r() * 0.7), y = h * (0.78 + r() * 0.12); g.fillRect(x, y - h * 0.12, w * 0.012, h * 0.12); g.fillRect(x - w * 0.02, y - h * 0.08, w * 0.02, w * 0.01); g.fillRect(x + w * 0.01, y - h * 0.1, w * 0.02, w * 0.01); }
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
    g.save(); g.beginPath(); g.rect(6, 6, w - 12, 62); g.clip(); g.translate(6, 6); mesa(g, w - 12, 62, 7, false); g.restore();
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
  painting: { w: 256, h: 160, draw: (g, w, h) => { mesa(g, w, h, 3); } },
  paintingTall: { w: 128, h: 176, draw: (g, w, h) => { mesa(g, w, h, 9, false); } },
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
  printB: { w: 96, h: 64, draw: (g, w, h) => {
    g.fillStyle = '#f4f2ec'; g.fillRect(0, 0, w, h);
    g.save(); g.beginPath(); g.rect(4, 4, w - 8, h - 8); g.clip(); g.translate(4, 4); mesa(g, w - 8, h - 8, 31, false); g.restore();
    const d = g.getImageData(4, 4, w - 8, h - 8);
    for (let i = 0; i < d.data.length; i += 4) { const v = d.data[i] * 0.3 + d.data[i + 1] * 0.59 + d.data[i + 2] * 0.11; d.data[i] = d.data[i + 1] = d.data[i + 2] = v; }
    g.putImageData(d, 4, 4);
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
  boxlabel: { w: 96, h: 48, draw: (g, w, h) => { paperBg(g, w, h); typed(g, ['RX LOGS', '1979 - 1980'], 8, 20, 11, 16, w - 16); } },
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
const DIRT = {
  // the floor where everybody walks: a soft patch, darker in the middle, with streaks along it
  wear: { w: 256, h: 128, draw: (g, w, h) => {
    blot(g, w / 2, h / 2, w * 0.48, h * 0.42, 0.5); blot(g, w * 0.45, h * 0.5, w * 0.3, h * 0.25, 0.35);
    const r = rng(3); g.strokeStyle = 'rgba(255,255,255,.18)';
    for (let i = 0; i < 60; i++) { g.lineWidth = 1 + r() * 2; const y = h * (0.2 + r() * 0.6), x = w * (0.1 + r() * 0.6); g.beginPath(); g.moveTo(x, y); g.lineTo(x + 20 + r() * 50, y + (r() - 0.5) * 6); g.stroke(); }
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
type Sheet = { size: number; rects: Record<string, Rect>; tex: THREE.CanvasTexture | null };
function pack(cells: Record<string, { w: number; h: number }>, size: number): Record<string, Rect> {
  const pad = 4, out: Record<string, Rect> = {};
  const order = Object.keys(cells).sort((a, b) => cells[b].h - cells[a].h);
  let x = 0, y = 0, row = 0;
  for (const k of order) {
    const c = cells[k];
    if (x + c.w > size) { x = 0; y += row + pad; row = 0; }
    out[k] = { x, y, w: c.w, h: c.h };
    x += c.w + pad; row = Math.max(row, c.h);
  }
  if (y + row > size) console.warn('clutter atlas too small', size);
  return out;
}
const things: Sheet = { size: 1024, rects: pack(THINGS, 1024), tex: null };
const dirt: Sheet = { size: 512, rects: pack(DIRT, 512), tex: null };
function sheet(s: Sheet, cells: Record<string, { draw: Draw }>, clear: boolean) {
  if (s.tex) return s.tex;
  const c = document.createElement('canvas'); c.width = c.height = s.size;
  const g = c.getContext('2d')!;
  if (!clear) { g.fillStyle = '#fff'; g.fillRect(0, 0, s.size, s.size); }
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
  const r = s.rects[k], S = s.size;
  return [(r.x + inset) / S, 1 - (r.y + r.h - inset) / S, (r.x + r.w - inset) / S, 1 - (r.y + inset) / S];
}
const WHITE = () => uvRect(things, 'white', 40);

// ---------- the wall clocks ----------
// The control room's clock is driven by the night's clock every frame (ControlRoom.setClock);
// it passes the time on here, and every other clock in the night shows the same.
let wallSecs = 0;
export function setWallClock(seconds: number) { wallSecs = seconds; }

// ---------- building a room's things ----------
export type At = { rx?: number; ry?: number; rz?: number; fine?: boolean };
type Part = { pos: number[]; nor: number[]; uv: number[]; col: number[] };
const _m = new THREE.Matrix4(), _q = new THREE.Quaternion(), _e = new THREE.Euler(), _p = new THREE.Vector3(), _one = new THREE.Vector3(1, 1, 1), _c = new THREE.Color();
const _up = new THREE.Vector3(0, 1, 0);
export const v3 = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);

export class Clutter {
  private base: Part = { pos: [], nor: [], uv: [], col: [] };
  private fine: Part = { pos: [], nor: [], uv: [], col: [] };
  private grime: Part = { pos: [], nor: [], uv: [], col: [] };
  private clocks: { x: number; y: number; z: number; ry: number; r: number }[] = [];
  private shade = rng(5);

  /** mat makes the room's material (its lamps: kit.floodlit with its flood set, or plain). */
  constructor(readonly name: string, private mat: (p: THREE.MeshStandardMaterialParameters) => THREE.MeshStandardMaterial) {}

  // Puts a geometry into a part: uvs into the atlas (groups[i] -> rects[i], the rest white),
  // turned (Euler YXZ, as kit.plane) and moved, coloured per corner with a little variation.
  private put(geo: THREE.BufferGeometry, color: THREE.ColorRepresentation, x: number, y: number, z: number, o: At, rects: ([number, number, number, number] | null)[] = [], to?: Part, vary = 0.06) {
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
  /** A box narrower or thinner at the top than at the bottom (a coat's body, a sleeve). */
  slab(wTop: number, wBot: number, h: number, dTop: number, dBot: number, x: number, y: number, z: number, color: THREE.ColorRepresentation, o: At = {}) {
    const g = new THREE.BoxGeometry(1, 1, 1), p = g.attributes.position as THREE.BufferAttribute;
    for (let i = 0; i < p.count; i++) { const top = p.getY(i) > 0; p.setXYZ(i, p.getX(i) * (top ? wTop : wBot), p.getY(i) * h, p.getZ(i) * (top ? dTop : dBot)); }
    g.computeVertexNormals();
    this.put(g, color, x, y, z, o);
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
  /** Dirt: a see-through picture facing +z, turned by o (never drawn on Low). */
  dirt(w: number, h: number, x: number, y: number, z: number, d: Dirt, color: THREE.ColorRepresentation, o: At = {}) {
    this.put(new THREE.PlaneGeometry(w, h), color, x, y, z, o, [uvRect(dirt, d, 1)], this.grime, 0.1);
  }
  /** Dirt on the floor (5 mm up), turned ry about the vertical. */
  floorDirt(w: number, d: number, x: number, z: number, t: Dirt, color: THREE.ColorRepresentation, ry = 0, y = 0.005) {
    this.dirt(w, d, x, y, z, t, color, { rx: -Math.PI / 2, ry });
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
   *  ry faces the room. The shoulders fall away from the hook, the sleeves hang at the sides. */
  coat(x: number, y: number, z: number, ry: number, color: THREE.ColorRepresentation, o: { long?: boolean } = {}) {
    const s = Math.sin(ry), c = Math.cos(ry), L = o.long ? 0.92 : 0.64;
    const at = (u: number, v: number, n: number): [number, number, number] => [x + c * u + s * n, y + v, z - s * u + c * n];
    const dark = new THREE.Color(color).multiplyScalar(0.72), side = new THREE.Color(color).multiplyScalar(0.88);
    this.box(0.05, 0.06, 0.01, ...at(0, 0.02, 0.005), 0x6c7074, { ry });                    // the hook's plate
    this.rod(v3(...at(0, 0.0, 0.01)), v3(...at(0, -0.02, 0.07)), 0.006, 0x6c7074);
    this.slab(0.12, 0.4, 0.16, 0.06, 0.11, ...at(0, -0.09, 0.07), color, { ry });             // the shoulders
    this.slab(0.37, 0.47, L, 0.11, 0.1, ...at(0, -0.17 - L / 2, 0.072), color, { ry, rx: 0.03 });
    for (const k of [-1, 1]) {
      this.slab(0.12, 0.1, 0.56, 0.1, 0.085, ...at(k * 0.215, -0.45, 0.1), side, { ry, rz: -k * 0.04 });
      this.box(0.1, 0.05, 0.088, ...at(k * 0.225, -0.74, 0.1), dark, { ry, rz: -k * 0.04, fine: true });   // the cuffs
      this.box(0.12, 0.022, 0.014, ...at(k * 0.12, -0.17 - L * 0.62, 0.132), dark, { ry, fine: true });   // the pocket flaps
    }
    for (const k of [-1, 1]) this.box(0.1, 0.08, 0.03, ...at(k * 0.045, -0.05, 0.12), dark, { ry, rz: k * 0.55 });   // the collar, turned down
    this.box(0.012, L * 0.92, 0.006, ...at(0.012, -0.2 - L * 0.46, 0.128), dark, { ry, fine: true });   // where it buttons
    for (let i = 0; i < 4; i++) this.box(0.016, 0.016, 0.008, ...at(0.03, -0.24 - i * L * 0.2, 0.131), 0x2a2420, { ry, fine: true });
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
  /** The room's meshes: its things (one call), its dirt (one more) and a mesh per clock's hands. */
  build(): THREE.Mesh[] {
    const out: THREE.Mesh[] = [];
    const geo = (parts: Part[]) => {
      const g = new THREE.BufferGeometry(), cat = (k: keyof Part) => parts.flatMap((p) => p[k]);
      g.setAttribute('position', new THREE.Float32BufferAttribute(cat('pos'), 3));
      g.setAttribute('normal', new THREE.Float32BufferAttribute(cat('nor'), 3));
      g.setAttribute('uv', new THREE.Float32BufferAttribute(cat('uv'), 2));
      g.setAttribute('color', new THREE.Float32BufferAttribute(cat('col'), 3));
      g.computeBoundingSphere();
      return g;
    };
    const solidMat = this.mat({ map: thingsAtlas(), vertexColors: true, roughness: 0.78 });
    const solidGeo = geo([this.base, this.fine]), first = this.base.pos.length / 3;
    const solid = new THREE.Mesh(solidGeo, solidMat);
    solid.name = 'clutter:' + this.name; solid.userData.noMerge = true;
    out.push(solid);
    let grime: THREE.Mesh | null = null;
    if (this.grime.pos.length) {
      const m = this.mat({ map: dirtAtlas(), vertexColors: true, roughness: 0.95, transparent: true, depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 });
      grime = new THREE.Mesh(geo([this.grime]), m);
      grime.name = 'clutter:' + this.name + ':dirt'; grime.userData.noMerge = true; grime.userData.noShadow = true;
      out.push(grime);
    }
    // Low draws the first part only, and no dirt (the dirt changes on the next frame: its own
    // onBeforeRender would never run again once it is hidden)
    solid.onBeforeRender = () => {
      const low = getQuality() === 'low';
      solidGeo.drawRange.count = low ? first : Infinity;
      if (grime) grime.visible = !low;
    };
    for (const k of this.clocks) out.push(this.hands(k, solidMat));
    return out;
  }

  // Three hands as quads in one small mesh, moved when the second changes.
  private hands(k: { x: number; y: number; z: number; ry: number; r: number }, mat: THREE.Material) {
    const shapes = [[0.022, k.r * 0.55, 0.002, 0x1c1d1f], [0.014, k.r * 0.82, 0.004, 0x1c1d1f], [0.004, k.r * 0.9, 0.006, 0xb3261e]];
    const n = shapes.length * 6, pos = new Float32Array(n * 3), col = new Float32Array(n * 3), uv = new Float32Array(n * 2), nor = new Float32Array(n * 3);
    const w = WHITE();
    for (let i = 0; i < n; i++) { nor[i * 3 + 2] = 1; uv[i * 2] = (w[0] + w[2]) / 2; uv[i * 2 + 1] = (w[1] + w[3]) / 2; _c.set(shapes[Math.floor(i / 6)][3]); col.set([_c.r, _c.g, _c.b], i * 3); }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
    g.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
    g.setAttribute('color', new THREE.BufferAttribute(col, 3));
    g.boundingSphere = new THREE.Sphere(new THREE.Vector3(), k.r);
    const mesh = new THREE.Mesh(g, mat);
    mesh.name = 'clutter:' + this.name + ':clock'; mesh.userData.noMerge = true;
    mesh.position.set(k.x, k.y, k.z); mesh.rotation.y = k.ry;
    let shown = -1;
    mesh.onBeforeRender = () => {
      const s = Math.floor(wallSecs);
      if (s === shown) return;
      shown = s;
      const ang = [-(s / 43200) * Math.PI * 2, -(s / 3600) * Math.PI * 2, -((s % 60) / 60) * Math.PI * 2];
      shapes.forEach(([wd, len, dz], j) => {
        const c = Math.cos(ang[j]), sn = Math.sin(ang[j]), tail = -len * 0.18;
        const q = [[-wd / 2, tail], [wd / 2, tail], [wd / 2, len], [-wd / 2, tail], [wd / 2, len], [-wd / 2, len]];
        q.forEach(([u, v], i) => pos.set([u * c - v * sn, u * sn + v * c, dz as number], (j * 6 + i) * 3));
      });
      (g.attributes.position as THREE.BufferAttribute).needsUpdate = true;
    };
    return mesh;
  }
}
