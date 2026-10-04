import * as THREE from 'three';
import { artImage, artTexture } from './art';

// Authored art supplies surfaces and posters. Instrument text, evidence marks and
// road markings stay deterministic canvas graphics for legibility and story accuracy.

type Draw = (g: CanvasRenderingContext2D, w: number, h: number) => void;

export function canvasTex(w: number, h: number, draw: Draw, repeat?: [number, number]): THREE.CanvasTexture {
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  const g = c.getContext('2d')!;
  draw(g, w, h);
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace;
  t.anisotropy = 4;
  if (repeat) { t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(repeat[0], repeat[1]); }
  return t;
}

// Small deterministic RNG so the room looks the same every run.
export function rng(seed: number) {
  let s = seed >>> 0;
  return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
}

function speckle(g: CanvasRenderingContext2D, w: number, h: number, n: number, alpha: number, seed: number) {
  const r = rng(seed);
  for (let i = 0; i < n; i++) {
    const v = r() < 0.5 ? 0 : 255;
    g.fillStyle = `rgba(${v},${v},${v},${alpha * r()})`;
    g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2);
  }
}

// The blank enamel sign (lab/sign_blank.png) at any card size: the corners keep their
// rivets and rust, the edges and the middle stretch.
function enamel(g: CanvasRenderingContext2D, w: number, h: number) {
  const im = artImage('sign'), inset = 40;
  const d = inset * Math.min(w / im.width, h / im.height);
  const sx = [0, inset, im.width - inset, im.width], sy = [0, inset, im.height - inset, im.height];
  const dx = [0, d, w - d, w], dy = [0, d, h - d, h];
  for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++)
    g.drawImage(im, sx[i], sy[j], sx[i + 1] - sx[i], sy[j + 1] - sy[j], dx[i], dy[j], dx[i + 1] - dx[i], dy[j + 1] - dy[j]);
}

// Paper (lab/tex_paper_card.jpg) filling a card or sheet, like CSS background-size: cover.
function paper(g: CanvasRenderingContext2D, w: number, h: number) {
  const im = artImage('paper'), k = Math.max(w / im.width, h / im.height);
  g.drawImage(im, (w - im.width * k) / 2, (h - im.height * k) / 2, im.width * k, im.height * k);
}

export function hexFloor() { return artTexture('floor', [7, 5.2]); }

export function ceilingTiles() { return artTexture('ceiling', [10, 8]); }

export function wallPaint() { return artTexture('wall', [4, 2]); }

export function concrete(_seed = 1, rep: [number, number] = [4, 2]) { return artTexture('concrete', rep); }

export function posterListen() { return artTexture('listen'); }

export function posterSaro() { return artTexture('saro'); }

export function dishIcon(g: CanvasRenderingContext2D, x: number, y: number, s: number, col: string) {
  g.save(); g.translate(x, y); g.strokeStyle = col; g.fillStyle = col; g.lineWidth = Math.max(2, s / 18);
  g.rotate(-0.5);
  g.beginPath(); g.ellipse(0, 0, s, s * 0.32, 0, Math.PI, 0, true); g.closePath(); g.stroke();
  g.beginPath(); g.moveTo(-s * 0.6, -s * 0.1); g.lineTo(0, -s * 0.85); g.lineTo(s * 0.6, -s * 0.1); g.stroke();
  g.rotate(0.5);
  g.beginPath(); g.moveTo(-s * 0.2, s * 0.3); g.lineTo(-s * 0.35, s * 1.1); g.moveTo(s * 0.2, s * 0.3); g.lineTo(s * 0.35, s * 1.1); g.stroke();
  g.restore();
}

export function mapNM() { return artTexture('map'); }

export function clockFace() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#efeadc'; g.beginPath(); g.arc(128, 128, 124, 0, 7); g.fill();
    g.strokeStyle = '#222'; g.lineWidth = 6; g.stroke();
    g.fillStyle = '#222'; g.font = '30px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
    for (let i = 1; i <= 12; i++) {
      const a = i / 12 * Math.PI * 2 - Math.PI / 2;
      g.fillText(String(i), 128 + Math.cos(a) * 96, 128 + Math.sin(a) * 96);
    }
    for (let i = 0; i < 60; i++) {
      const a = i / 60 * Math.PI * 2;
      g.fillRect(128 + Math.cos(a) * 114 - 1, 128 + Math.sin(a) * 114 - 1, i % 5 ? 2 : 4, i % 5 ? 2 : 4);
    }
  });
}

export function keyboard() {
  return canvasTex(256, 96, (g, w, h) => {
    g.fillStyle = '#9d947c'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#d6ccb0';
    for (let r = 0; r < 5; r++) for (let c = 0; c < 15; c++) {
      if (r === 4 && c > 3 && c < 11) continue;
      g.fillRect(6 + c * 16.5, 6 + r * 17.5, 14, 15);
    }
    g.fillRect(6 + 4 * 16.5, 6 + 4 * 17.5, 16.5 * 7 - 2, 15);
  });
}

export function mugLogo() {
  return canvasTex(256, 128, (g, w, h) => {
    g.fillStyle = '#ede8dc'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#2a2f38';
    g.drawImage(artImage('logo'), 26, 12, 76, 76);
    g.font = '500 30px Oswald'; g.textAlign = 'center'; g.fillText('SARO', 64, 108);
  });
}

export function logbookCover() {
  return canvasTex(256, 320, (g, w, h) => {
    g.fillStyle = '#1b2433'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 5000, 0.18, 31);
    g.fillStyle = '#c9c1a8'; g.font = '500 36px Oswald'; g.textAlign = 'center';
    g.fillText('NIGHT SHIFT', w / 2, 70);
    g.fillRect(40, 84, w - 80, 2);
    g.font = '18px VT323'; g.fillText('SARO CONTROL / LOG 14', w / 2, 110);
  });
}

export function rackFront() {
  return canvasTex(256, 512, (g, w, h) => {
    g.fillStyle = '#22272b'; g.fillRect(0, 0, w, h);
    const units = [60, 40, 80, 40, 60, 100, 40];
    let y = 16;
    units.forEach((u, i) => {
      g.fillStyle = i % 2 ? '#3a4146' : '#30363a'; g.fillRect(12, y, w - 24, u - 6);
      g.fillStyle = '#596166';
      for (let k = 0; k < 6; k++) g.fillRect(24 + k * 34, y + 10, 18, 6);
      y += u;
    });
    g.fillStyle = '#d9d1b8'; g.fillRect(40, 440, 176, 34);
    g.fillStyle = '#1b1b1b'; g.font = '30px VT323'; g.textAlign = 'center'; g.fillText('RX BANK 3', 128, 466);
  });
}

export function signTex(text: string, sub = '') {
  return canvasTex(256, 96, (g, w, h) => {
    enamel(g, w, h);
    g.fillStyle = '#1f2326'; g.textAlign = 'center';
    g.font = '500 32px Oswald'; g.fillText(text, w / 2, sub ? 46 : 60);
    if (sub) { g.font = '18px Oswald'; g.fillText(sub, w / 2, 76); }
  });
}

export function deskPapers(seed = 1) {
  return canvasTex(128, 160, (g, w, h) => {
    paper(g, w, h);
    const r = rng(seed);
    g.fillStyle = 'rgba(40,50,80,.55)';
    for (let y = 18; y < h - 10; y += 9) g.fillRect(12, y, 30 + r() * 80, 2);
  });
}

// The night work order on the supervisor desk: typed form, Ward's initials, a red stamp.
export function workOrderSheet() {
  const typed = (g: CanvasRenderingContext2D) => {
    g.fillStyle = '#26262a'; g.font = '15px "Special Elite", serif';
    const lines = ['SARO / OPERATIONS', 'NIGHT WORK ORDER  04/13/86', '', 'OPERATOR:  REYES', 'ON CALL:   DR. E. WARD', '', '1. RESTORE RX BANK 3.', '   CALIBRATE 1419.900', '2. RUN THE SURVEY SWEEP.', '3. MORNING SERIES 06:00.', '   NOT BEFORE EVERY', '   ANOMALY IS SIGNED.'];
    lines.forEach((l, i) => g.fillText(l, 18, 34 + i * 19));
    g.strokeStyle = 'rgba(38,50,79,.8)'; g.lineWidth = 1.5;
    g.beginPath(); g.moveTo(20, 300); g.bezierCurveTo(60, 290, 90, 312, 130, 298); g.stroke();
    g.fillStyle = '#26324f'; g.font = '24px "Reenie Beanie", cursive'; g.fillText('keep the paper  E.W.', 22, 328);
    g.save(); g.translate(196, 70); g.rotate(-0.25);
    g.strokeStyle = 'rgba(170,40,30,.7)'; g.lineWidth = 2.5; g.strokeRect(-40, -14, 80, 28);
    g.fillStyle = 'rgba(170,40,30,.75)'; g.font = '600 13px Oswald'; g.textAlign = 'center'; g.fillText('WORK ORDER', 0, 5);
    g.restore();
  };
  return canvasTex(256, 352, (g, w, h) => {
    paper(g, w, h);
    typed(g);
  });
}

// Chapter two: the south corridor and the records room.
export function vinylTiles() {
  return canvasTex(256, 256, (g, w, h) => {
    const r = rng(41);
    for (let y = 0; y < 4; y++) for (let x = 0; x < 4; x++) {
      const v = 150 + Math.floor(r() * 18);
      g.fillStyle = `rgb(${v},${v - 6},${v - 18})`; g.fillRect(x * 64, y * 64, 64, 64);
    }
    speckle(g, w, h, 3000, 0.12, 42);
    g.strokeStyle = 'rgba(60,50,35,.35)'; g.lineWidth = 2;
    for (let i = 0; i <= 4; i++) { g.beginPath(); g.moveTo(i * 64, 0); g.lineTo(i * 64, h); g.stroke(); g.beginPath(); g.moveTo(0, i * 64); g.lineTo(w, i * 64); g.stroke(); }
  }, [5, 5]);
}

// A drinks and snacks machine, lit from inside. Plain colours, no brands.
export function vendingFront() {
  return canvasTex(256, 512, (g, w, h) => {
    g.fillStyle = '#20262c'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#dfe8ef'; g.fillRect(14, 14, 172, 380);
    const cols = ['#b8322a', '#2f6db0', '#e0b13a', '#3f8a4c', '#c56a2b', '#7b4fa3'];
    const r = rng(7);
    for (let row = 0; row < 6; row++) {
      g.fillStyle = 'rgba(40,50,60,.5)'; g.fillRect(14, 70 + row * 56, 172, 3);
      for (let i = 0; i < 5; i++) { g.fillStyle = cols[Math.floor(r() * cols.length)]; g.fillRect(22 + i * 33, 30 + row * 56, 24, 38); }
    }
    g.fillStyle = '#c9c2a8'; g.fillRect(196, 40, 46, 120);
    g.fillStyle = '#10161a'; g.fillRect(206, 56, 26, 8); g.fillRect(212, 90, 14, 40);
    g.fillStyle = '#1a1f24'; g.fillRect(30, 410, 150, 60);
    g.fillStyle = '#e8e2cf'; g.font = '600 22px Oswald'; g.textAlign = 'center'; g.fillText('COLD DRINKS', 100, 500 - 2);
  });
}

// April 1986. The night of the 13th runs into the 14th (the printout is dated 04/14/86).
export function calendarApril1986() {
  return canvasTex(256, 320, (g, w) => {
    g.fillStyle = '#f2ecdc'; g.fillRect(0, 0, w, 320);
    g.fillStyle = '#7a2e22'; g.fillRect(0, 0, w, 70);
    g.fillStyle = '#f2ecdc'; g.font = '600 34px Oswald'; g.textAlign = 'center'; g.fillText('APRIL 1986', w / 2, 48);
    g.fillStyle = '#2b2a27'; g.font = '15px Oswald';
    'SMTWTFS'.split('').forEach((d, i) => g.fillText(d, 22 + i * 35, 96));
    g.font = '18px "Special Elite", serif';
    for (let d = 1; d <= 30; d++) {
      const cell = d + 1; // the 1st is a Tuesday
      const x = 22 + (cell % 7) * 35, y = 128 + Math.floor(cell / 7) * 38;
      g.fillText(String(d), x, y);
      if (d === 13) { g.strokeStyle = '#26324f'; g.lineWidth = 2; g.beginPath(); g.arc(x, y - 6, 15, 0, Math.PI * 2); g.stroke(); }
    }
  });
}

export function greenbarPaper(lines: string[]) {
  return canvasTex(512, 768, (g, w, h) => {
    g.fillStyle = '#f1efe6'; g.fillRect(0, 0, w, h);
    g.fillStyle = 'rgba(150,200,160,.45)';
    for (let y = 0; y < h; y += 64) g.fillRect(0, y, w, 32);
    g.fillStyle = 'rgba(0,0,0,.45)';
    for (let y = 16; y < h; y += 32) { g.beginPath(); g.arc(14, y, 5, 0, 7); g.fill(); g.beginPath(); g.arc(w - 14, y, 5, 0, 7); g.fill(); }
    g.fillStyle = '#2b2b30'; g.font = '30px VT323';
    lines.forEach((l, i) => g.fillText(l, 36, 52 + i * 32));
  });
}

export function facadeWindows(cols: number, rows: number, seed = 3) {
  return canvasTex(512, 256, (g, w, h) => {
    g.fillStyle = '#7b7266'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 3000, 0.15, seed);
    const r = rng(seed);
    const cw = w / cols, rh = h / rows;
    for (let i = 0; i < cols; i++) for (let j = 0; j < rows; j++) {
      const lit = r() < 0.55;
      g.fillStyle = lit ? `rgb(${230 + r() * 25},${160 + r() * 40},${80 + r() * 30})` : '#1d2128';
      g.fillRect(i * cw + cw * 0.18, j * rh + rh * 0.3, cw * 0.64, rh * 0.42);
    }
  });
}

export function saroLettering() {
  return canvasTex(512, 160, (g, w, h) => {
    g.clearRect(0, 0, w, h);
    g.fillStyle = '#e8dfca'; g.font = '500 92px Oswald'; g.fillText('SARO', 12, 100);
    dishIcon(g, 300, 58, 30, '#e8dfca');
    g.font = '22px Oswald'; g.fillText('SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY', 14, 140);
  });
}

export function neonSierra() {
  return canvasTex(512, 256, (g, w, h) => {
    g.fillStyle = '#1a0d0c'; g.fillRect(0, 0, w, h);
    g.shadowColor = '#ff3b2a'; g.shadowBlur = 24; g.fillStyle = '#ff6b55';
    g.font = 'italic 700 120px Georgia, serif'; g.textAlign = 'center';
    g.fillText('Sierra', w / 2, 150);
    g.shadowBlur = 0;
  });
}

export function neonMotorCourt() {
  return canvasTex(512, 128, (g, w, h) => {
    g.fillStyle = '#0b1716'; g.fillRect(0, 0, w, h);
    g.shadowColor = '#41f0d8'; g.shadowBlur = 18; g.fillStyle = '#9ffff0';
    g.font = '500 70px Oswald'; g.textAlign = 'center'; g.fillText('MOTOR COURT', w / 2, 90);
  });
}

export function marquee() {
  return canvasTex(256, 192, (g, w, h) => {
    g.fillStyle = '#efe7cf'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#1b1b1b'; g.font = '500 34px Oswald'; g.textAlign = 'center';
    g.fillText('CLEAN ROOMS', w / 2, 50); g.fillText('CABLE TV', w / 2, 96);
    g.fillStyle = '#c0281c'; g.fillRect(16, 118, w - 32, 56);
    g.shadowColor = '#ff3020'; g.shadowBlur = 14; g.fillStyle = '#ffd2c8'; g.fillText('VACANCY', w / 2, 160);
  });
}

export function motelFacade() {
  return canvasTex(1024, 128, (g, w, h) => {
    g.fillStyle = '#b48c63'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 10; i++) {
      const x = i * 102 + 12;
      g.fillStyle = i % 3 === 1 ? '#7a2b22' : '#8c3a2c'; g.fillRect(x, 30, 26, 92);
      g.fillStyle = Math.random() < 0.6 ? '#ffcf86' : '#2a2620'; g.fillRect(x + 40, 44, 46, 40);
      g.fillStyle = '#ffe9b0'; g.fillRect(x + 12, 14, 4, 6);
    }
  });
}

export function glowSprite(inner = 'rgba(255,255,255,1)', outer = 'rgba(255,255,255,0)') {
  const t = canvasTex(64, 64, (g, w, h) => {
    const gr = g.createRadialGradient(32, 32, 0, 32, 32, 32);
    gr.addColorStop(0, inner); gr.addColorStop(0.25, inner.replace(/[\d.]+\)$/, '0.55)')); gr.addColorStop(1, outer);
    g.fillStyle = gr; g.fillRect(0, 0, w, h);
  });
  return t;
}

export function blobShadow() {
  const t = canvasTex(64, 64, (g, w, h) => {
    const gr = g.createRadialGradient(32, 32, 4, 32, 32, 32);
    gr.addColorStop(0, 'rgba(0,0,0,.55)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = gr; g.fillRect(0, 0, w, h);
  });
  return t;
}

export function chainlink() {
  const t = canvasTex(64, 64, (g, w, h) => {
    g.clearRect(0, 0, w, h);
    g.strokeStyle = 'rgba(190,190,180,1)'; g.lineWidth = 2;
    g.beginPath(); g.moveTo(0, 0); g.lineTo(64, 64); g.moveTo(64, 0); g.lineTo(0, 64); g.stroke();
  }, [60, 3]);
  t.colorSpace = THREE.SRGBColorSpace;
  return t;
}

export function roadTex() {
  return canvasTex(512, 512, (g, w, h) => {
    g.drawImage(artImage('asphalt'), 0, 0, w, h);
    g.scale(w / 64, h / 256);
    g.fillStyle = '#c99a2e'; g.fillRect(29, 0, 3, 140); g.fillRect(34, 0, 3, 140);
    g.fillStyle = '#9a9a92'; g.fillRect(2, 0, 2, 256); g.fillRect(60, 0, 2, 256);
  }, [1, 60]);
}

// ---------- Chapter one: service yard and photo lab ----------

// Label with dark text, one line per entry. Size is in canvas pixels. Signs (the default) are
// blank enamel, cards and sheets (`border: false` or `surface: 'paper'`) are paper, and a
// card with its own `bg` colour (warnings, the darkroom sign) keeps the plain painted look.
export function labelCard(lines: string[], o: { w?: number; h?: number; bg?: string; fg?: string; font?: string; size?: number; border?: boolean; align?: CanvasTextAlign; surface?: 'sign' | 'paper' } = {}) {
  const w = o.w ?? 256, h = o.h ?? 160;
  const surface = o.bg ? 'painted' : o.surface ?? (o.border === false ? 'paper' : 'sign');
  return canvasTex(w, h, (g) => {
    if (surface === 'sign') enamel(g, w, h);
    else if (surface === 'paper') paper(g, w, h);
    else { g.fillStyle = o.bg!; g.fillRect(0, 0, w, h); speckle(g, w, h, Math.floor(w * h / 60), 0.08, lines.length + w); }
    if (surface !== 'sign' && o.border !== false) { g.strokeStyle = o.fg ?? '#25292c'; g.lineWidth = Math.max(2, w / 90); g.strokeRect(w * 0.03, h * 0.04, w * 0.94, h * 0.92); }
    const size = o.size ?? Math.min(34, Math.floor((h * 0.8) / Math.max(1, lines.length) * 0.78));
    g.fillStyle = o.fg ?? '#1f2326'; g.textAlign = o.align ?? 'center'; g.textBaseline = 'middle';
    g.font = `500 ${size}px ${o.font ?? 'Oswald'}`;
    const centred = (o.align ?? 'center') === 'center';
    const x = centred ? w / 2 : w * 0.1;
    const top = h / 2 - ((lines.length - 1) * size * 1.2) / 2;
    // a long line is narrowed to stay inside the border instead of running off the card
    lines.forEach((l, i) => g.fillText(l, x, top + i * size * 1.2, centred ? w * 0.88 : w * 0.82));
  });
}

export function cameraCard() {
  return labelCard(['FIELD CAMERA', 'C  /  VIEWFINDER', 'SPACE  /  SHUTTER'], { w: 256, h: 160, font: 'VT323', size: 30, surface: 'paper' });
}

// SARO enamel sign on the yard fence (reference image: service yard at S-03)
export function yardSign() { return artTexture('yard'); }

// Grey steel cabinet door with stencil and a small inspection window
export function cabinetFace(title: string, sub: string[], seed = 3) {
  return canvasTex(256, 384, (g, w, h) => {
    g.drawImage(artImage('cabinet'), 0, 0, w, h);
    speckle(g, w, h, 5000, 0.18, seed);
    const r = rng(seed + 7);
    for (let i = 0; i < 26; i++) { g.fillStyle = `rgba(110,60,25,${0.15 + r() * 0.3})`; g.fillRect(r() < 0.5 ? r() * 14 : w - r() * 14, r() * h, 2 + r() * 6, 2 + r() * 14); }
    g.strokeStyle = 'rgba(20,24,26,.8)'; g.lineWidth = 3; g.strokeRect(10, 10, w - 20, h - 20);
    g.fillStyle = '#e8e2cf'; g.font = '500 64px Oswald'; g.textAlign = 'center'; g.fillText(title, w / 2, 70);
    // inspection window (a screen can sit behind it), py 92 to 176
    g.fillStyle = '#1c2226'; g.fillRect(40, 92, w - 80, 84);
    g.strokeStyle = 'rgba(200,196,180,.35)'; g.lineWidth = 2; g.strokeRect(36, 88, w - 72, 92);
    g.fillStyle = '#e8e2cf'; g.font = '22px Oswald'; g.textAlign = 'right';
    sub.forEach((l, i) => g.fillText(l, w - 28, 210 + i * 26));
    // warning plate, bottom right
    g.fillStyle = '#d1a91f'; g.fillRect(128, 292, 100, 40);
    g.fillStyle = '#16120a'; g.textAlign = 'center'; g.font = '600 19px Oswald'; g.fillText('DANGER', 178, 310);
    g.font = '15px Oswald'; g.fillText('480 VOLTS', 178, 327);
  });
}

// Faded paint on the concrete walk: the worn frame is yard/floor_paint_frame.png, the words
// are stencilled in code in the same paint and worn on their own canvas, so only they get it.
export function floorStencil(lines: string[], w = 512, h = 256) {
  const words = document.createElement('canvas');
  words.width = w; words.height = h;
  const t = words.getContext('2d')!;
  t.fillStyle = 'rgba(232,206,120,.82)';
  t.font = `500 ${Math.floor(h / (lines.length + 1.2))}px Oswald`; t.textAlign = 'center'; t.textBaseline = 'middle';
  // never wider than the inside of the painted frame
  lines.forEach((l, i) => t.fillText(l, w / 2, h / 2 + (i - (lines.length - 1) / 2) * h / (lines.length + 0.8), w * 0.84));
  const r = rng(lines[0].length * 13);
  t.globalCompositeOperation = 'destination-out';
  for (let i = 0; i < 900; i++) { t.fillStyle = `rgba(0,0,0,${r() * 0.8})`; t.fillRect(r() * w, r() * h, 1 + r() * 7, 1 + r() * 3); }
  return canvasTex(w, h, (g) => { g.drawImage(artImage('frame'), 0, 0, w, h); g.drawImage(words, 0, 0); });
}

// B-12 reference vane: dark board, one ivory stripe, scale ticks (as in the Unity photographs).
// yard/vane_b12.png keeps the stripe at x 0.22 to 0.52 and y 0.05 to 0.95, where the
// stripe test in ServiceYard expects it. It has no words, so it is used as it is.
export function vaneFace() { return artTexture('vane'); }
export function echoFace() {
  return canvasTex(128, 512, (g, w, h) => {
    g.clearRect(0, 0, w, h);
    const gr = g.createLinearGradient(w * 0.15, 0, w * 0.6, 0);
    gr.addColorStop(0, 'rgba(240,236,214,0)'); gr.addColorStop(0.25, 'rgba(240,236,214,.75)'); gr.addColorStop(0.75, 'rgba(240,236,214,.75)'); gr.addColorStop(1, 'rgba(240,236,214,0)');
    g.fillStyle = gr; g.fillRect(w * 0.15, h * 0.05, w * 0.45, h * 0.9);
  });
}
// R-07: the other survey marker, three horizontal bars (yard/board_r07.png). The bars end at
// 0.80 of the height; the name goes in the free strip under them.
export function barsBoard() {
  return canvasTex(512, 512, (g, w, h) => {
    g.drawImage(artImage('bars'), 0, 0, w, h);
    g.fillStyle = '#c9c0a4'; g.font = '500 52px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
    g.fillText('R-07', w / 2, h * 0.9);
  });
}

// Field map on the photo lab wall: control room, yard walk, S-03 motor bus, B-12, the lab.
// lab/map_field_yard.png is the plan without words. Every name is drawn here, at the places
// given in production/PRECISE_GRAPHICS.json (512 x 384 units). The title sits in the empty
// ground west of the walk, clear of the antenna arrow and the north mark.
export function fieldMap() {
  return canvasTex(1024, 768, (g, w, h) => {
    g.drawImage(artImage('fieldMap'), 0, 0, w, h);
    g.scale(w / 512, h / 384);
    const ink = '#2a2a26', red = '#9b3b2c';
    g.fillStyle = ink;
    g.font = '500 26px Oswald'; g.fillText('SARO / FIELD MAP', 24, 128, 186);
    g.font = '14px "Special Elite"'; g.fillText('SERVICE YARD, EAST WALK', 24, 150, 186);
    g.font = '15px "Special Elite"';
    g.fillText('CONTROL ROOM', 66, 300); g.fillText('PHOTO LAB', 320, 280); g.fillText('EAST WALK', 274, 340);
    g.fillStyle = red;
    g.fillText('S-03 / MOTOR BUS', 300, 195); g.fillText('B-12 / OPTICAL', 300, 112); g.fillText('TO S-03 (ANTENNA)', 140, 26);
    g.fillStyle = ink;
    g.font = '22px "Reenie Beanie"'; g.fillText('fence', 120, 70);
    g.font = '16px Oswald'; g.fillText('N', 472, 40);
  });
}

// Motor bus display behind the inspection glass. Drawn by the chapter code.
export function screenCanvas(w = 256, h = 160) {
  const c = document.createElement('canvas'); c.width = w; c.height = h;
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace;
  return { canvas: c, ctx: c.getContext('2d')!, tex: t };
}
