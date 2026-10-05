import * as THREE from 'three';
import { canvasTex, rng, screenCanvas } from '../core/textures';
import { artImage } from '../core/art';

// Canvas textures for the SARO service truck. Every word on the truck is drawn here in
// code (the fleet markings, the gauges, the clock), so it is always spelt right.

// A rectangle in a texture atlas: [u0, v0, u1, v1]. Canvas y runs down, UV v runs up.
export type Rect = [number, number, number, number];

const INK = '#1d3a48'; // the logo's own blue-green, as cut vinyl

// ---------- paint ----------
// Chalky white paint after twenty New Mexico summers: soft blotches, small chips. Tiled
// over the body at about a metre per repeat; the road dust low on the sides comes from
// the vertices (Truck.ts), so this stays the same everywhere.
export function paintTex() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#e6e2d8'; g.fillRect(0, 0, w, h);
    const r = rng(11);
    for (let i = 0; i < 40; i++) {
      const cx = r() * w, cy = r() * h, rad = 10 + r() * 46;
      for (const [ox, oy] of [[0, 0], [w, 0], [-w, 0], [0, h], [0, -h]]) {   // wrap, so the tile has no seams
        const gr = g.createRadialGradient(cx + ox, cy + oy, 0, cx + ox, cy + oy, rad);
        gr.addColorStop(0, r() < 0.5 ? 'rgba(196,184,154,.07)' : 'rgba(252,252,248,.08)');
        gr.addColorStop(1, 'rgba(0,0,0,0)');
        g.fillStyle = gr; g.fillRect(cx + ox - rad, cy + oy - rad, rad * 2, rad * 2);
      }
    }
    for (let i = 0; i < 50; i++) {
      g.fillStyle = r() < 0.75 ? 'rgba(120,110,95,.25)' : 'rgba(112,64,30,.3)';
      g.fillRect(r() * w, r() * h, 1 + r() * 1.6, 1 + r() * 1.6);
    }
  }, [1, 1]);
}

// ---------- decals: everything printed, painted or stamped on the outside ----------
// One 1024 x 1024 atlas with alpha. Door and tailgate markings, panel gaps and handles'
// shadows on the doors, the grille with its four square headlamp bezels, the licence plates.
const AW = 1024, AH = 1024;
const px = (x0: number, y0: number, x1: number, y1: number): Rect => [x0 / AW, 1 - y1 / AH, x1 / AW, 1 - y0 / AH];
export const DECAL = {
  doorL: px(0, 0, 512, 256), doorR: px(512, 0, 1024, 256),
  tailgate: px(0, 256, 512, 384), grille: px(512, 256, 1024, 384),
  plateF: px(0, 384, 256, 512), plateR: px(256, 384, 512, 512),
};

// Scratches through vinyl lettering: dots of paint colour over the ink.
function wear(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, seed: number) {
  const r = rng(seed);
  g.fillStyle = 'rgba(230,226,216,.9)';
  for (let i = 0; i < 240; i++) g.fillRect(x + r() * w, y + r() * h, 1 + r() * 2.5, 1 + r() * 1.5);
}

// One door, 1.22 x 0.62 m on 512 x 256: the gaps round it, the handle's shadow, the
// markings. frontLeft: the front of the truck is on the viewer's left (the driver's door).
function door(g: CanvasRenderingContext2D, x: number, y: number, frontLeft: boolean, seed: number) {
  const w = 512, h = 256, sx = 0.9;
  g.save(); g.beginPath(); g.rect(x, y, w, h); g.clip();
  g.strokeStyle = 'rgba(48,46,42,.85)'; g.lineWidth = 3;
  g.beginPath(); g.roundRect(x + 4, y - 20, w - 8, h + 14, 14); g.stroke();             // the gap round the door
  g.fillStyle = 'rgba(40,38,34,.35)';
  const hx = frontLeft ? x + w - 92 : x + 30;                                              // under the handle
  g.beginPath(); g.ellipse(hx + 31, y + 44, 34, 11, 0, 0, 7); g.fill();
  g.fillStyle = 'rgba(40,38,34,.7)'; g.beginPath(); g.arc(frontLeft ? hx - 10 : hx + 72, y + 44, 5, 0, 7); g.fill();   // the lock
  const mid = frontLeft ? x + w * 0.47 : x + w * 0.53;
  g.translate(mid, y); g.scale(sx, 1);
  g.globalAlpha = 0.92;
  g.drawImage(artImage('logo'), -205, 64, 120, 120);
  g.fillStyle = INK; g.textBaseline = 'alphabetic'; g.textAlign = 'left';
  g.font = '600 92px Oswald'; g.fillText('SARO', -75, 152, 240);
  g.font = '500 17px Oswald';
  g.fillText('SOUTHWEST ASTRONOMICAL', -72, 178, 236);
  g.fillText('RESEARCH OBSERVATORY', -72, 198, 236);
  g.font = '600 26px Oswald'; g.fillText('SARO 07', -205, 224, 140);
  g.restore();
  wear(g, mid - 190, y + 60, 380, 170, seed + 1);
}

// The grille, 1.80 x 0.40 m on 512 x 128: four square sealed-beam bezels at the ends,
// black egg-crate between them with three bright bars, the parking lamps' frames under.
function grille(g: CanvasRenderingContext2D, x: number, y: number) {
  const w = 512, h = 128;
  g.fillStyle = '#0d0e0f'; g.fillRect(x, y, w, h);
  const chrome = (cx: number, cy: number, cw: number, ch: number) => {
    const gr = g.createLinearGradient(0, cy, 0, cy + ch);
    gr.addColorStop(0, '#f2f3f1'); gr.addColorStop(0.45, '#9a9ea0'); gr.addColorStop(0.55, '#5f6366'); gr.addColorStop(1, '#d9dbd8');
    g.fillStyle = gr; g.fillRect(cx, cy, cw, ch);
  };
  // the egg-crate between the lamps
  g.strokeStyle = '#2a2c2e'; g.lineWidth = 3;
  for (let i = 0; i <= 14; i++) { const gx = x + 128 + i * (256 / 14); g.beginPath(); g.moveTo(gx, y + 10); g.lineTo(gx, y + 118); g.stroke(); }
  for (let j = 0; j <= 4; j++) { const gy = y + 10 + j * 27; g.beginPath(); g.moveTo(x + 128, gy); g.lineTo(x + 384, gy); g.stroke(); }
  for (const gy of [8, 60, 112]) chrome(x + 120, y + gy, 272, 6);
  chrome(x + 252, y + 8, 8, 110);
  // the lamp bezels (the lenses are their own meshes, lit by the lamps)
  for (const bx of [10, 384]) {
    chrome(x + bx, y + 4, 118, 120);
    g.fillStyle = '#16181a'; g.fillRect(x + bx + 8, y + 10, 102, 50); g.fillRect(x + bx + 8, y + 66, 102, 50);
  }
  g.strokeStyle = 'rgba(255,255,255,.35)'; g.lineWidth = 1; g.strokeRect(x + 1, y + 1, w - 2, h - 2);
}

// New Mexico plates, 1986: yellow with red letters, LAND OF ENCHANTMENT along the bottom.
function plate(g: CanvasRenderingContext2D, x: number, y: number, text: string, seed: number) {
  const w = 256, h = 128;
  g.fillStyle = '#e8c34a'; g.beginPath(); g.roundRect(x + 4, y + 4, w - 8, h - 8, 10); g.fill();
  g.strokeStyle = '#a5281e'; g.lineWidth = 4; g.beginPath(); g.roundRect(x + 10, y + 10, w - 20, h - 20, 7); g.stroke();
  g.fillStyle = '#a5281e'; g.textAlign = 'center'; g.textBaseline = 'middle';
  g.font = '600 18px Oswald'; g.fillText('NEW MEXICO', x + w / 2, y + 26);
  g.font = '600 56px Oswald'; g.fillText(text, x + w / 2, y + 68, 210);
  g.font = '500 13px Oswald'; g.fillText('LAND OF ENCHANTMENT', x + w / 2, y + 105);
  const r = rng(seed);
  for (let i = 0; i < 160; i++) { g.fillStyle = r() < 0.6 ? 'rgba(120,90,50,.35)' : 'rgba(255,240,200,.3)'; g.fillRect(x + 6 + r() * (w - 12), y + 6 + r() * (h - 12), 1 + r() * 2, 1 + r() * 2); }
  g.fillStyle = '#6d6a62'; for (const bx of [40, 216]) { g.beginPath(); g.arc(x + bx, y + 64, 5, 0, 7); g.fill(); }
}

export function decalAtlas() {
  return canvasTex(AW, AH, (g) => {
    door(g, 0, 0, true, 21);
    door(g, 512, 0, false, 22);
    // the tailgate, 1.84 x 0.46 m: the fleet number low on the right, scuffs by the latch
    g.save(); g.translate(400, 256); g.scale(0.62, 1);
    g.fillStyle = INK; g.font = '600 46px Oswald'; g.textAlign = 'center'; g.textBaseline = 'alphabetic'; g.fillText('SARO 07', 0, 100);
    g.restore();
    wear(g, 330, 300, 140, 60, 14);
    g.fillStyle = 'rgba(40,38,34,.55)'; g.fillRect(226, 268, 60, 12);
    grille(g, 512, 256);
    plate(g, 0, 384, 'SGV 147', 31);
    plate(g, 256, 384, 'SGV 147', 32);
  });
}

// ---------- the cab: small tiled textures for its materials ----------
/** Moulded dash plastic: dark brown-black with a fine leather grain. */
export function dashGrainTex() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#8a7660'; g.fillRect(0, 0, w, h);
    const r = rng(5);
    for (let i = 0; i < 9000; i++) { const v = r() < 0.5 ? 40 : 120; g.fillStyle = `rgba(${v},${v - 4},${v - 8},${0.08 + r() * 0.12})`; g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r()); }
  }, [1, 1]);
}
/** Pleated vinyl for the bench and the door cards: channels across the u direction. */
export function vinylTex() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#b39878'; g.fillRect(0, 0, w, h);
    const r = rng(8);
    for (let i = 0; i < 5000; i++) { const v = r() < 0.5 ? 80 : 200; g.fillStyle = `rgba(${v},${v * 0.86},${v * 0.7},${0.06 + r() * 0.08})`; g.fillRect(r() * w, r() * h, 1.5, 1.5); }
    for (let k = 0; k < 4; k++) {   // four pleats per tile: a soft crown and a stitched seam
      const x = k * 64, gr = g.createLinearGradient(x, 0, x + 64, 0);
      gr.addColorStop(0, 'rgba(40,28,18,.55)'); gr.addColorStop(0.12, 'rgba(255,240,220,.10)'); gr.addColorStop(0.5, 'rgba(255,240,220,.16)'); gr.addColorStop(0.88, 'rgba(255,240,220,.06)'); gr.addColorStop(1, 'rgba(40,28,18,.55)');
      g.fillStyle = gr; g.fillRect(x, 0, 64, h);
      g.fillStyle = 'rgba(60,44,30,.6)'; for (let y = 0; y < h; y += 8) g.fillRect(x + 3, y, 1.5, 4);
    }
  }, [1, 1]);
}
/** The headliner: pale fibreboard with rows of small holes. */
export function linerTex() {
  return canvasTex(128, 128, (g, w, h) => {
    g.fillStyle = '#c9c2b2'; g.fillRect(0, 0, w, h);
    g.fillStyle = 'rgba(70,64,54,.28)';
    for (let y = 4; y < h; y += 8) for (let x = (y / 8) % 2 ? 4 : 0; x < w; x += 8) g.fillRect(x, y, 2, 2);
  }, [1, 1]);
}
/** Ribbed rubber floor mat. */
export function matTex() {
  return canvasTex(128, 128, (g, w, h) => {
    g.fillStyle = '#4a4744'; g.fillRect(0, 0, w, h);
    for (let x = 0; x < w; x += 8) { g.fillStyle = 'rgba(0,0,0,.45)'; g.fillRect(x, 0, 3, h); g.fillStyle = 'rgba(255,255,255,.07)'; g.fillRect(x + 4, 0, 1, h); }
    const r = rng(9); g.fillStyle = 'rgba(150,120,90,.35)';
    for (let i = 0; i < 300; i++) g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2);     // desert grit
  }, [1, 1]);
}

// The cab's printed parts in one 512 x 512 atlas: the heater panel, the vents' louvres,
// the defroster slots, the rear-view mirror's picture, the clipboard on the seat, the horn pad.
const CW = 512, CH = 512;
const cpx = (x0: number, y0: number, x1: number, y1: number): Rect => [x0 / CW, 1 - y1 / CH, x1 / CW, 1 - y0 / CH];
export const CAB = {
  heater: cpx(0, 0, 256, 64), louver: cpx(256, 0, 384, 64), mirror: cpx(384, 0, 512, 64),
  defrost: cpx(0, 64, 512, 96), clip: cpx(0, 96, 192, 352), horn: cpx(192, 96, 320, 224),
  glove: cpx(320, 96, 512, 224),
};
export function cabAtlas() {
  return canvasTex(CW, CH, (g) => {
    // heater and air: three levers in slots, the words over them
    g.fillStyle = '#2a2826'; g.fillRect(0, 0, 256, 64);
    g.strokeStyle = 'rgba(255,255,255,.18)'; g.strokeRect(2, 2, 252, 60);
    g.fillStyle = '#cfd6cf'; g.font = '500 10px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
    [['OFF  VENT  HEAT  DEF', 70], ['COLD        WARM', 186]].forEach(([t, x]) => g.fillText(t as string, x as number, 14));
    g.fillStyle = '#0b0b0b'; g.fillRect(14, 30, 112, 6); g.fillRect(130, 30, 112, 6);
    g.fillStyle = '#9a9c9a'; g.fillRect(52, 24, 10, 18); g.fillRect(178, 24, 10, 18);
    g.fillStyle = '#cfd6cf'; g.fillText('FAN  LO  MED  HI', 128, 54);
    // louvres
    g.fillStyle = '#121212'; g.fillRect(256, 0, 128, 64);
    for (let y = 4; y < 64; y += 8) { g.fillStyle = '#3a3a3a'; g.fillRect(258, y, 124, 3); }
    // the mirror: the back window, dark, the bed and the road behind going grey
    const mg = g.createLinearGradient(0, 0, 0, 64);
    mg.addColorStop(0, '#1d2533'); mg.addColorStop(0.55, '#2c3340'); mg.addColorStop(1, '#141414');
    g.fillStyle = mg; g.fillRect(384, 0, 128, 64);
    g.fillStyle = '#07080a'; g.fillRect(384, 0, 128, 10); g.fillRect(384, 0, 14, 64); g.fillRect(498, 0, 14, 64); g.fillRect(384, 44, 128, 20);
    g.fillStyle = 'rgba(200,210,230,.08)'; g.fillRect(398, 10, 100, 34);
    // defroster slots along the top of the dash
    g.fillStyle = '#1b1a19'; g.fillRect(0, 64, 512, 32);
    g.fillStyle = '#050505'; for (let x = 8; x < 504; x += 12) g.fillRect(x, 70, 7, 20);
    // the clipboard on the seat: a work order under the clip
    g.fillStyle = '#7a5a36'; g.fillRect(0, 96, 192, 256);
    g.fillStyle = '#ece6d6'; g.fillRect(12, 124, 168, 220);
    g.fillStyle = '#9da0a3'; g.fillRect(56, 100, 80, 26);
    g.fillStyle = '#26303a'; g.font = '600 13px Oswald'; g.textAlign = 'left'; g.fillText('SARO WORK ORDER', 22, 146);
    g.font = '12px "Special Elite", monospace';
    ['VEH  SARO 07', 'MILES OUT  84,217', 'FUEL  1/2', 'DRIVER', '', 'NOTES'].forEach((t, i) => g.fillText(t, 22, 172 + i * 20));
    g.strokeStyle = 'rgba(40,50,60,.35)'; for (let i = 0; i < 4; i++) { g.beginPath(); g.moveTo(22, 296 + i * 12); g.lineTo(170, 296 + i * 12); g.stroke(); }
    g.strokeStyle = 'rgba(30,40,110,.8)'; g.lineWidth = 1.4; g.beginPath(); g.moveTo(80, 234); g.bezierCurveTo(96, 222, 104, 246, 122, 230); g.stroke();
    // the horn pad: black grain, a small round badge with no name on it
    g.fillStyle = '#151515'; g.fillRect(192, 96, 128, 128);
    const hg = g.createRadialGradient(256, 160, 2, 256, 160, 22); hg.addColorStop(0, '#d8dad6'); hg.addColorStop(1, '#55595c');
    g.fillStyle = hg; g.beginPath(); g.arc(256, 160, 18, 0, 7); g.fill();
    g.fillStyle = '#151515'; g.beginPath(); g.arc(256, 160, 11, 0, 7); g.fill();
    // the glovebox door: a seam, a chrome button, a scuff
    g.fillStyle = '#7d6a56'; g.fillRect(320, 96, 192, 128);
    g.strokeStyle = 'rgba(0,0,0,.6)'; g.lineWidth = 2; g.strokeRect(324, 100, 184, 120);
    g.fillStyle = '#b7bab8'; g.fillRect(400, 104, 32, 10);
    g.fillStyle = 'rgba(255,255,255,.08)'; g.fillRect(340, 170, 60, 6);
  });
}

/** The windshield from inside: a green shade band along the top, road film low down, the
 *  two clean arcs the wipers leave. Drawn with alpha over the view. */
export function windshieldTex() {
  return canvasTex(512, 256, (g, w, h) => {
    const band = g.createLinearGradient(0, 0, 0, h * 0.16);
    band.addColorStop(0, 'rgba(40,110,90,.2)'); band.addColorStop(0.6, 'rgba(40,110,90,.09)'); band.addColorStop(1, 'rgba(40,110,90,0)');
    g.fillStyle = band; g.fillRect(0, 0, w, h * 0.16);
    const film = g.createLinearGradient(0, h, 0, h * 0.6);
    film.addColorStop(0, 'rgba(170,150,120,0)'); film.addColorStop(0.15, 'rgba(170,150,120,.06)'); film.addColorStop(1, 'rgba(170,150,120,0)');
    g.fillStyle = film; g.fillRect(0, h * 0.6, w, h * 0.4);
    const r = rng(3);
    for (let i = 0; i < 120; i++) { g.fillStyle = `rgba(190,180,160,${0.03 + r() * 0.05})`; g.fillRect(r() * w, h * 0.35 + r() * h * 0.6, 1, 1); }
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
  g.font = '500 11px Oswald'; g.fillText('P   R   N   D   2   1', sp.x, 152);
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
