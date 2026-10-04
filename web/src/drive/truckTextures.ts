import * as THREE from 'three';
import { canvasTex, rng, screenCanvas } from '../core/textures';
import { artImage } from '../core/art';

// Canvas textures for the SARO service truck. Every word on the truck is drawn here in
// code (the fleet markings, the gauges, the clock), so it is always spelt right.

// A rectangle in a texture atlas: [u0, v0, u1, v1]. Canvas y runs down, UV v runs up.
export type Rect = [number, number, number, number];

// The paint atlas (1024 x 512): faded white paint, the same paint with road dust at the
// bottom, both doors with the fleet markings, and the tailgate.
export const PAINT: Record<'plain' | 'dusty' | 'doorL' | 'doorR' | 'tailgate', Rect> = {
  plain: [0, 0.5, 0.25, 1],
  dusty: [0.25, 0.5, 0.5, 1],
  doorL: [0.5, 0.5, 1, 1],
  doorR: [0.5, 0, 1, 0.5],
  tailgate: [0, 0, 0.5, 0.5],
};

const INK = '#1d3a48'; // the logo's own blue-green, as cut vinyl

// Chalky white paint after twenty New Mexico summers: soft blotches and small chips.
function paint(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, seed: number) {
  g.fillStyle = '#e4e0d5'; g.fillRect(x, y, w, h);
  const r = rng(seed);
  for (let i = 0; i < 60; i++) {
    const cx = x + r() * w, cy = y + r() * h, rad = 8 + r() * 40;
    const gr = g.createRadialGradient(cx, cy, 0, cx, cy, rad);
    const warm = r() < 0.5;
    gr.addColorStop(0, warm ? 'rgba(196,182,150,.18)' : 'rgba(250,250,246,.22)');
    gr.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = gr; g.fillRect(cx - rad, cy - rad, rad * 2, rad * 2);
  }
  for (let i = 0; i < 70; i++) {
    g.fillStyle = r() < 0.7 ? 'rgba(120,110,95,.35)' : 'rgba(110,62,30,.4)';
    g.fillRect(x + r() * w, y + r() * h, 1 + r() * 2, 1 + r() * 2);
  }
}

// Road dust thrown up by the wheels, heaviest at the bottom edge.
function dust(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, depth = 0.4) {
  const gr = g.createLinearGradient(0, y + h, 0, y + h * (1 - depth));
  gr.addColorStop(0, 'rgba(150,122,88,.75)'); gr.addColorStop(0.45, 'rgba(165,140,105,.3)'); gr.addColorStop(1, 'rgba(170,150,120,0)');
  g.fillStyle = gr; g.fillRect(x, y + h * (1 - depth), w, h * depth);
}

// Scratches through vinyl lettering: dots of paint colour over the ink.
function wear(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, seed: number) {
  const r = rng(seed);
  g.fillStyle = 'rgba(228,224,213,.85)';
  for (let i = 0; i < 260; i++) g.fillRect(x + r() * w, y + r() * h, 1 + r() * 2.5, 1 + r() * 1.5);
}

// One door. The face is 1.57 x 0.70 m on a 512 x 256 region, so everything drawn here is
// narrowed by `sx` to keep the logo round on the truck. frontLeft: the front of the truck
// is on the viewer's left (the driver's door).
function door(g: CanvasRenderingContext2D, x: number, y: number, frontLeft: boolean, seed: number) {
  const w = 512, h = 256, sx = 1 / 1.12;
  paint(g, x, y, w, h, seed);
  // panel gaps: the door's front and rear edges, and the cab corner behind it
  g.fillStyle = 'rgba(60,58,52,.8)';
  const front = frontLeft ? 6 : w - 8, rear = frontLeft ? w * 0.86 : w * 0.14;
  g.fillRect(x + front, y, 2, h); g.fillRect(x + rear, y, 3, h);
  g.fillRect(x, y + 4, w, 2);
  // the markings sit in the middle of the door itself
  const mid = frontLeft ? x + w * 0.44 : x + w * 0.58;
  g.save();
  g.translate(mid, y); g.scale(sx, 1);
  g.globalAlpha = 0.9;
  g.drawImage(artImage('logo'), -205, 40, 120, 120);
  g.fillStyle = INK; g.textBaseline = 'alphabetic'; g.textAlign = 'left';
  g.font = '600 92px Oswald'; g.fillText('SARO', -75, 128, 240);
  g.font = '500 17px Oswald';
  g.fillText('SOUTHWEST ASTRONOMICAL', -72, 154, 236);
  g.fillText('RESEARCH OBSERVATORY', -72, 174, 236);
  g.font = '600 26px Oswald'; g.fillText('SARO 07', -205, 200, 140);
  g.restore();
  wear(g, mid - 200, y + 36, 400, 170, seed + 1);
  dust(g, x, y, w, h, 0.42);
}

export function paintAtlas() {
  return canvasTex(1024, 512, (g) => {
    paint(g, 0, 0, 256, 256, 11);
    paint(g, 256, 0, 256, 256, 12); dust(g, 256, 0, 256, 256, 0.45);
    door(g, 512, 0, true, 21);
    door(g, 512, 256, false, 22);
    // tailgate (1.86 x 0.56 m): fleet number low on the right, and a dusty bottom
    paint(g, 0, 256, 512, 256, 13);
    g.save(); g.translate(400, 256); g.scale(0.6, 1);
    g.fillStyle = INK; g.font = '600 46px Oswald'; g.textAlign = 'center'; g.fillText('SARO 07', 0, 170);
    g.restore();
    wear(g, 330, 380, 140, 60, 14);
    dust(g, 0, 256, 512, 256, 0.5);
  });
}

// ---------- instrument cluster ----------
// One 512 x 256 canvas: the cluster (speedometer, fuel, temperature) in the top 160 rows,
// the radio and the clock below. Only the clock is ever redrawn, and only when it changes.
export const DASH = { cluster: [0, 0.375, 1, 1] as Rect, stack: [0, 0, 1, 0.375] as Rect };
// Dial geometry in canvas pixels, shared with the needle placement in Truck.ts.
export const DIALS = {
  speed: { x: 256, y: 88, r: 70, from: -120, to: 120 },
  fuel: { x: 88, y: 98, r: 40, from: -60, to: 60 },
  temp: { x: 424, y: 98, r: 40, from: -60, to: 60 },
};
const MARK = '#d6eadc';

function dial(g: CanvasRenderingContext2D, d: { x: number; y: number; r: number; from: number; to: number }, labels: [number, string][], ticks: number, name: string) {
  const at = (f: number, rad: number) => {
    const a = (d.from + (d.to - d.from) * f) * Math.PI / 180;
    return [d.x + Math.sin(a) * rad, d.y - Math.cos(a) * rad];
  };
  g.strokeStyle = MARK; g.fillStyle = MARK; g.lineCap = 'butt';
  for (let i = 0; i <= ticks; i++) {
    const major = i % 2 === 0;
    const [x0, y0] = at(i / ticks, d.r), [x1, y1] = at(i / ticks, d.r - (major ? 9 : 5));
    g.lineWidth = major ? 2.4 : 1.3;
    g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke();
  }
  g.font = `500 ${d.r > 50 ? 15 : 13}px Oswald`; g.textAlign = 'center'; g.textBaseline = 'middle';
  for (const [f, s] of labels) { const [x, y] = at(f, d.r - (d.r > 50 ? 21 : 16)); g.fillText(s, x, y); }
  g.font = '500 11px Oswald'; g.fillText(name, d.x, d.y + (d.r > 50 ? 26 : 16));
}

export function dashAtlas() {
  const s = screenCanvas(512, 256);
  const g = s.ctx;
  g.fillStyle = '#060707'; g.fillRect(0, 0, 512, 160);
  g.shadowColor = 'rgba(160,255,200,.45)'; g.shadowBlur = 3;
  const sp = DIALS.speed;
  dial(g, sp, Array.from({ length: 9 }, (_, i) => [i * 10 / 85, String(i * 10)] as [number, string]), 34, 'MPH');
  dial(g, DIALS.fuel, [[0, 'E'], [0.5, '1/2'], [1, 'F']], 4, 'FUEL');
  dial(g, DIALS.temp, [[0, 'C'], [1, 'H']], 4, 'TEMP');
  g.shadowBlur = 0;
  // odometer and the dark warning lamps
  g.fillStyle = '#000'; g.fillRect(sp.x - 34, sp.y + 34, 68, 18);
  g.strokeStyle = 'rgba(214,234,220,.5)'; g.lineWidth = 1; g.strokeRect(sp.x - 34, sp.y + 34, 68, 18);
  g.fillStyle = MARK; g.font = '17px VT323'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillText('084217', sp.x, sp.y + 44);
  g.font = '500 10px Oswald';
  [['BRAKE', 170, '#4a1712'], ['OIL', 342, '#4a1712'], ['VOLTS', 170, '#4a3512'], ['BELTS', 342, '#4a3512']].forEach(([t, x, c], i) => {
    g.fillStyle = c as string; g.fillRect((x as number) - 20, i < 2 ? 128 : 144, 40, 12);
    g.fillStyle = '#0b0b0b'; g.fillText(t as string, x as number, i < 2 ? 134 : 150);
  });
  g.fillStyle = '#123d22';
  for (const [x, dir] of [[196, -1], [316, 1]]) { g.beginPath(); g.moveTo(x + dir * 12, 14); g.lineTo(x, 6); g.lineTo(x, 22); g.closePath(); g.fill(); }
  // radio faceplate: AM and FM dial behind glass, two knobs
  g.fillStyle = '#16181b'; g.fillRect(0, 160, 512, 96);
  g.fillStyle = '#2b2e32'; g.fillRect(0, 160, 512, 4);
  g.fillStyle = '#0e1210'; g.fillRect(70, 176, 260, 64);
  g.fillStyle = 'rgba(214,234,220,.75)'; g.font = '12px VT323'; g.textAlign = 'center';
  ['88', '92', '96', '100', '104', '108'].forEach((t, i) => g.fillText(t, 92 + i * 44, 192));
  ['54', '60', '70', '80', '100', '120', '160'].forEach((t, i) => g.fillText(t, 88 + i * 37, 226));
  g.fillText('FM', 80, 208); g.fillText('AM', 318, 208);
  g.fillStyle = '#c4361e'; g.fillRect(214, 180, 3, 56);
  g.fillStyle = '#3a3d42';
  for (const x of [36, 357]) { g.beginPath(); g.arc(x, 208, 20, 0, 7); g.fill(); }
  const drawClock = (text: string) => {
    g.fillStyle = '#020403'; g.fillRect(384, 178, 116, 60);
    g.shadowColor = 'rgba(120,255,210,.8)'; g.shadowBlur = 6;
    g.fillStyle = '#86f2cf'; g.font = '46px VT323'; g.textAlign = 'center'; g.textBaseline = 'middle';
    g.fillText(text, 442, 210, 108);
    g.shadowBlur = 0;
    s.tex.needsUpdate = true;
  };
  drawClock('');
  s.tex.anisotropy = 4;
  s.tex.minFilter = THREE.LinearMipmapLinearFilter;
  return { tex: s.tex, drawClock };
}
