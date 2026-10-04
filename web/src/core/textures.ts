import * as THREE from 'three';
import { artImage, artTexture, artLoaded, loadArtFor } from './art';

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

// Chapter two: the south corridor and the records room. Quiet 30 cm vinyl tiles, four by
// four on the picture, so one repeat is 1.2 m: give the floor size to keep them square.
// A drinks and snacks machine, lit from inside: Codex's text-free front (round 4), with
// COLD DRINKS written in code where its manifest puts it. No brands.
export function vendingFront() {
  return canvasTex(256, 512, (g, w, h) => {
    g.drawImage(artImage('vending'), 0, 0, w, h);
    g.fillStyle = '#e8e2cf'; g.font = '600 22px Oswald'; g.textAlign = 'center'; g.textBaseline = 'middle';
    g.fillText('COLD DRINKS', 116, 491, 187);
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
      // the moon's phases, printed small in the corner of the day, as calendars had them
      const phase = ({ 1: 'last', 9: 'new', 17: 'first', 24: 'full' } as Record<number, string>)[d];
      if (phase) {
        const mx = x + 12, my = y - 16, r = 4;
        g.fillStyle = '#2b2a27'; g.strokeStyle = '#2b2a27'; g.lineWidth = 1;
        g.beginPath(); g.arc(mx, my, r, 0, Math.PI * 2);
        if (phase === 'new') g.fill(); else g.stroke();
        if (phase === 'first' || phase === 'last') { g.beginPath(); g.arc(mx, my, r, Math.PI / 2, -Math.PI / 2, phase === 'first'); g.fill(); }
      }
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
  const marks = (g: CanvasRenderingContext2D, w: number, h: number, yellow: string, white: string) => {
    g.scale(w / 64, h / 256);
    g.fillStyle = yellow; g.fillRect(29, 0, 3, 140); g.fillRect(34, 0, 3, 140);
    g.fillStyle = white; g.fillRect(2, 0, 2, 256); g.fillRect(60, 0, 2, 256);
  };
  const tex = canvasTex(512, 512, (g, w, h) => {
    g.drawImage(artImage('asphalt'), 0, 0, w, h);
    marks(g, w, h, '#c99a2e', '#9a9a92');
  }, [1, 60]);
  dataTwin(tex, 512, 512, (g, w, h, k) => {
    g.drawImage(artImage(k === 'n' ? 'asphaltN' : 'asphaltR'), 0, 0, w, h);
    const flat = k === 'n' ? 'rgb(128,128,255)' : 'rgb(140,140,140)';
    marks(g, w, h, flat, flat);
  });
  return tex;
}

/**
 * For a canvas made from round 6/7 colour pictures (the roads): a builder for the matching
 * normal and roughness canvases from the round 9 maps, drawn the same way, kept on the
 * texture for core/ultra.ts. Built only when Ultra asks, and only once.
 */
export type TwinDraw = (g: CanvasRenderingContext2D, w: number, h: number, kind: 'n' | 'r') => void;
export function dataTwin(tex: THREE.Texture, w: number, h: number, draw: TwinDraw) {
  let made: { normal: THREE.Texture; roughness: THREE.Texture } | null = null;
  tex.userData.dataTwin = (like: THREE.Texture) => {
    if (!made) {
      const one = (k: 'n' | 'r') => {
        const t = canvasTex(w, h, (g, ww, hh) => draw(g, ww, hh, k));
        t.colorSpace = THREE.NoColorSpace;
        return t;
      };
      made = { normal: one('n'), roughness: one('r') };
    }
    for (const t of [made.normal, made.roughness]) {
      t.wrapS = like.wrapS; t.wrapT = like.wrapT; t.repeat.copy(like.repeat); t.offset.copy(like.offset);
      t.rotation = like.rotation; t.center.copy(like.center); t.flipY = like.flipY;
    }
    return made;
  };
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

// ---------- Night Shift papers (story/nightshift.ts) ----------

// The spring 1986 poster for Halley's comet: a night sky, the comet with its tail, a little
// chart of the southern sky with the comet's places through April, and Ward's note.
// The picture is Codex's (round 10, poster_halley_1986_blank.jpg, no text); the words, the
// chart's marks and the note are drawn here (KAPITLER.md, Night Shift, item 4). The picture
// is a later image: until it has loaded (or if it cannot), a drawn poster stands in, and the
// texture is redrawn when it arrives.
export function halleyPosterTex() {
  const c = document.createElement('canvas'); c.width = 512; c.height = 768;
  const g = c.getContext('2d')!;
  const tex = new THREE.CanvasTexture(c); tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 4;
  const draw = () => {
    g.setTransform(1, 0, 0, 1, 0, 0);
    if (artLoaded('halleyPoster')) halleyOnArt(g); else { g.scale(2, 2); halleyDrawn(g, 256, 384); }
    tex.userData.fromArt = artLoaded('halleyPoster');
    tex.needsUpdate = true;
  };
  draw();
  if (!artLoaded('halleyPoster')) loadArtFor(['halleyPoster']).then(draw).catch(() => { /* the drawn poster stays */ });
  return tex;
}

// On the picture, 512 x 768 (half the picture's size; the zones are in ART_BRIEF.md).
function halleyOnArt(g: CanvasRenderingContext2D) {
  g.drawImage(artImage('halleyPoster'), 0, 0, 512, 768);
  g.textAlign = 'center'; g.textBaseline = 'alphabetic';
  g.fillStyle = '#f2e6c4';
  g.font = '600 60px Oswald'; g.fillText("HALLEY'S COMET", 256, 86, 440);
  g.font = '500 26px Oswald'; g.fillText('APRIL 1986', 256, 124);
  // the chart in the cream box: the horizon is printed at y 620; S, SE and SW under it and
  // the comet's place on four nights, moving west and down through the month
  g.fillStyle = '#2b2a27'; g.strokeStyle = '#2b2a27';
  g.font = '500 13px Oswald'; g.textAlign = 'left'; g.fillText('THE SOUTHERN SKY, APRIL', 46, 524);
  g.textAlign = 'center';
  for (const [x, l] of [[90, 'SW'], [256, 'S'], [422, 'SE']] as [number, string][]) {
    g.fillRect(x - 0.5, 616, 1, 8);
    g.fillText(l, x, 638);
  }
  const pts: [number, number, string][] = [[404, 556, 'APR 5'], [338, 575, '10'], [262, 594, '14'], [186, 606, '20']];
  g.setLineDash([3, 4]); g.lineWidth = 1;
  g.beginPath(); pts.forEach(([x, y], i) => (i ? g.lineTo(x, y) : g.moveTo(x, y))); g.stroke();
  g.setLineDash([]);
  for (const [x, y, d] of pts) {
    g.beginPath(); g.arc(x, y, 4, 0, Math.PI * 2); g.fill();
    g.fillText(d, x, y - 9);
  }
  // the five lines
  g.fillStyle = '#f2e6c4'; g.font = '500 17px Oswald';
  ['CLOSEST TO EARTH APRIL 11', 'LOOK LOW IN THE SOUTH', 'EARLY APRIL: BEFORE DAWN', 'AFTER THE 12TH: AROUND MIDNIGHT', 'GET AWAY FROM TOWN LIGHTS']
    .forEach((l, i) => g.fillText(l, 256, 676 + i * 18.5, 440));
  // Ward's note, taped on in the top left
  g.save(); g.translate(34, 160); g.rotate(-0.06);
  g.fillStyle = 'rgba(0,0,0,.35)'; g.fillRect(3, 4, 150, 104);
  g.fillStyle = '#efe39a'; g.fillRect(0, 0, 150, 104);
  g.fillStyle = 'rgba(255,255,255,.5)'; g.fillRect(52, -8, 46, 16);
  g.fillStyle = '#26324f'; g.font = '17px "Reenie Beanie", cursive'; g.textAlign = 'left';
  ['Public line: comet calls', 'go to the planetarium', 'in town. Not us.', 'Not at 3 a.m.', '            E.W.'].forEach((l, i) => g.fillText(l, 8, 20 + i * 18, 136));
  g.restore();
}

// The stand-in, drawn at 256 x 384.
function halleyDrawn(g: CanvasRenderingContext2D, w: number, h: number) {
  const sky = g.createLinearGradient(0, 0, 0, h);
  sky.addColorStop(0, '#060a1c'); sky.addColorStop(0.65, '#14204a'); sky.addColorStop(1, '#2a2440');
  g.fillStyle = sky; g.fillRect(0, 0, w, h);
  const r = rng(1986);
  for (let i = 0; i < 160; i++) { g.fillStyle = `rgba(255,255,240,${0.3 + r() * 0.7})`; g.fillRect(r() * w, r() * h * 0.7, r() < 0.1 ? 2 : 1, r() < 0.1 ? 2 : 1); }
  // the comet, head low right, tail up to the left
  g.save(); g.translate(176, 150); g.rotate(-0.75);
  const tail = g.createLinearGradient(0, 0, 0, -150);
  tail.addColorStop(0, 'rgba(220,235,255,.85)'); tail.addColorStop(1, 'rgba(220,235,255,0)');
  g.fillStyle = tail; g.beginPath(); g.moveTo(-6, 0); g.lineTo(-26, -150); g.lineTo(26, -150); g.lineTo(6, 0); g.fill();
  const head = g.createRadialGradient(0, 0, 0, 0, 0, 14);
  head.addColorStop(0, 'rgba(255,255,255,1)'); head.addColorStop(1, 'rgba(200,220,255,0)');
  g.fillStyle = head; g.beginPath(); g.arc(0, 0, 14, 0, Math.PI * 2); g.fill();
  g.restore();
  // mesa along the bottom of the picture
  g.fillStyle = '#0b0a12'; g.beginPath(); g.moveTo(0, 236); g.lineTo(40, 228); g.lineTo(70, 214); g.lineTo(150, 212); g.lineTo(176, 226); g.lineTo(w, 232); g.lineTo(w, 250); g.lineTo(0, 250); g.fill();
  g.fillStyle = '#f2e6c4'; g.textAlign = 'center';
  g.font = '600 30px Oswald'; g.fillText("HALLEY'S COMET", w / 2, 44);
  g.font = '500 16px Oswald'; g.fillText('APRIL 1986', w / 2, 66);
  // the chart: horizon line and the comet's places
  g.fillStyle = '#e9dfc4'; g.fillRect(14, 256, w - 28, 92);
  g.strokeStyle = '#2b2a27'; g.lineWidth = 1; g.strokeRect(18, 260, w - 36, 84);
  g.beginPath(); g.moveTo(22, 330); g.lineTo(w - 22, 330); g.stroke();
  g.fillStyle = '#2b2a27'; g.font = '10px Oswald'; g.textAlign = 'left'; g.fillText('S', w / 2 - 3, 342); g.fillText('SE', w - 40, 342); g.fillText('SW', 26, 342);
  const pts: [number, number, string][] = [[196, 300, '5'], [160, 316, '10'], [118, 318, '14'], [76, 304, '20']];
  for (const [x, y, d] of pts) { g.beginPath(); g.arc(x, y, 3, 0, Math.PI * 2); g.fill(); g.fillText(d, x - 4, y - 7); }
  g.font = '500 12px Oswald'; g.textAlign = 'center'; g.fillStyle = '#f2e6c4';
  g.fillText('LOOK LOW IN THE SOUTH', w / 2, 372);
  // Ward's note, taped on
  g.save(); g.translate(30, 92); g.rotate(-0.08);
  g.fillStyle = '#efe39a'; g.fillRect(0, 0, 92, 62);
  g.fillStyle = 'rgba(255,255,255,.45)'; g.fillRect(30, -6, 34, 12);
  g.fillStyle = '#26324f'; g.font = '15px "Reenie Beanie", cursive'; g.textAlign = 'left';
  ['comet calls go', 'to the planetarium', 'not us  E.W.'].forEach((l, i) => g.fillText(l, 5, 18 + i * 17));
  g.restore();
}

// The service record on its clipboard (RX bank 3).
export function serviceRecordTex() {
  return canvasTex(192, 256, (g, w, h) => {
    g.fillStyle = '#7a5a36'; g.fillRect(0, 0, w, h);
    paperRect(g, 12, 22, w - 24, h - 30);
    g.fillStyle = '#9aa0a3'; g.fillRect(w / 2 - 34, 6, 68, 24);
    g.fillStyle = '#26262a'; g.font = '11px "Special Elite", serif'; g.textAlign = 'left';
    ['SARO / MAINTENANCE', 'RX BANK 3', 'SERVICE RECORD', '', '09/23/81 K3 REPLACED', '03/03/83 K3 REPLACED', '10/21/85 K3 REPLACED', '  (3RD)', 'IT TRIPS BEFORE,', 'NOT AFTER.   M.O.'].forEach((l, i) => g.fillText(l, 20, 50 + i * 18));
  });
}

// A strip of telex roll: yellowish paper with upper-case type.
export function telexPaperTex() {
  return canvasTex(128, 256, (g, w, h) => {
    g.fillStyle = '#efe8c8'; g.fillRect(0, 0, w, h);
    g.fillStyle = 'rgba(38,38,42,.85)'; g.font = '9px "Special Elite", serif';
    ['ZCZC WX2250', 'SARO OPS', 'WX ADVISORY 2250', 'TSTMS W OF MAGDALENA', 'MTNS MOVG NE 15 KT.', 'NNNN', '', 'ZCZC NET2300', 'ALL SITES', 'NET STATUS 2300 MST', 'SITE 03 - NORMAL', 'SITE 11 - NORMAL', 'SARO - NORMAL', 'NNNN'].forEach((l, i) => g.fillText(l, 8, 18 + i * 13));
  });
}

// The spine label of the exceptions binder.
export function binderSpineTex() {
  return canvasTex(64, 256, (g, w, h) => {
    g.fillStyle = '#2f4a6e'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#e9e2cf'; g.fillRect(10, 30, w - 20, 150);
    g.save(); g.translate(w / 2 + 6, 105); g.rotate(-Math.PI / 2);
    g.fillStyle = '#1f2326'; g.font = '600 16px Oswald'; g.textAlign = 'center'; g.fillText('EXCEPTIONS', 0, 0);
    g.restore();
    g.fillStyle = '#c9c1a8'; g.beginPath(); g.arc(w / 2, 214, 9, 0, Math.PI * 2); g.fill();
  });
}

// Dale's index card taped to the console.
export function rfiCardTex() {
  return canvasTex(192, 120, (g, w, h) => {
    g.fillStyle = '#f1ead6'; g.fillRect(0, 0, w, h);
    g.strokeStyle = 'rgba(180,70,70,.6)'; g.beginPath(); g.moveTo(0, 22); g.lineTo(w, 22); g.stroke();
    g.strokeStyle = 'rgba(80,120,170,.35)'; for (let y = 38; y < h; y += 14) { g.beginPath(); g.moveTo(0, y); g.lineTo(w, y); g.stroke(); }
    g.fillStyle = '#26324f'; g.font = '15px "Reenie Beanie", cursive';
    ['CAL 1419.900  45-65  34-62', '1420.110 HIGHWAY RELAY. NOTCH', '1420.6 SAT, PASSES ONLY', 'ANYTHING ELSE: LOG IT'].forEach((l, i) => g.fillText(l, 8, 18 + i * 26));
    g.fillStyle = 'rgba(200,170,90,.55)'; g.fillRect(70, -2, 50, 10);
  });
}

function paperRect(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number) {
  const im = artImage('paper');
  g.drawImage(im, 0, 0, im.width, im.height, x, y, w, h);
}
