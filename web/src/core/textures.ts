import * as THREE from 'three';

// All textures in the prologue are painted on canvas at startup.
// When ChatGPT delivers real texture files, swap them in here (same function names).

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

export function hexFloor() {
  return canvasTex(512, 512, (g, w, h) => {
    g.fillStyle = '#3a2d22'; g.fillRect(0, 0, w, h);
    const r = rng(7);
    const s = 32; // hex radius in px
    const dx = s * Math.sqrt(3), dy = s * 1.5;
    for (let row = -1; row < h / dy + 1; row++) {
      for (let col = -1; col < w / dx + 1; col++) {
        const cx = col * dx + (row % 2 ? dx / 2 : 0), cy = row * dy;
        const t = 0.85 + r() * 0.2;
        g.fillStyle = `rgb(${Math.round(150 * t)},${Math.round(118 * t)},${Math.round(88 * t)})`;
        g.beginPath();
        for (let k = 0; k < 6; k++) {
          const a = Math.PI / 6 + k * Math.PI / 3;
          const px = cx + Math.cos(a) * (s - 2), py = cy + Math.sin(a) * (s - 2);
          k ? g.lineTo(px, py) : g.moveTo(px, py);
        }
        g.closePath(); g.fill();
      }
    }
    speckle(g, w, h, 5000, 0.12, 3);
  }, [7, 5.2]);
}

export function ceilingTiles() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#b9b3a3'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 4000, 0.25, 11);
    g.strokeStyle = '#6d685d'; g.lineWidth = 4;
    g.strokeRect(0, 0, w, h);
  }, [10, 8]);
}

export function wallPaint() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#7f8a8a'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 3000, 0.08, 5);
  }, [4, 2]);
}

export function concrete(seed = 1, rep: [number, number] = [4, 2]) {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#8f877a'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 6000, 0.18, seed);
    g.strokeStyle = 'rgba(60,55,48,.35)'; g.lineWidth = 2;
    g.beginPath(); g.moveTo(0, h * 0.5); g.lineTo(w, h * 0.5); g.stroke();
  }, rep);
}

export function posterListen() {
  return canvasTex(256, 384, (g, w, h) => {
    g.fillStyle = '#16202b'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#c9b48a';
    g.font = '500 34px Oswald';
    ['LISTEN', 'RECORD', 'ANALYZE', 'UNDERSTAND'].forEach((t, i) => g.fillText(t, 24, 70 + i * 44));
    dishIcon(g, w * 0.5, h * 0.78, 42, '#c9b48a');
    g.font = '15px VT323'; g.fillText('SARO  /  EST. 1972', 24, h - 18);
  });
}

export function posterSaro() {
  return canvasTex(256, 384, (g, w, h) => {
    g.fillStyle = '#121a24'; g.fillRect(0, 0, w, h);
    dishIcon(g, w * 0.55, h * 0.32, 70, '#d8cfb9');
    g.fillStyle = '#d8cfb9'; g.font = '500 30px Oswald';
    ['SOUTHWEST', 'ASTRONOMICAL', 'RESEARCH', 'OBSERVATORY'].forEach((t, i) => g.fillText(t, 22, 230 + i * 36));
  });
}

export function dishIcon(g: CanvasRenderingContext2D, x: number, y: number, s: number, col: string) {
  g.save(); g.translate(x, y); g.strokeStyle = col; g.fillStyle = col; g.lineWidth = Math.max(2, s / 18);
  g.rotate(-0.5);
  g.beginPath(); g.ellipse(0, 0, s, s * 0.32, 0, Math.PI, 0, true); g.closePath(); g.stroke();
  g.beginPath(); g.moveTo(-s * 0.6, -s * 0.1); g.lineTo(0, -s * 0.85); g.lineTo(s * 0.6, -s * 0.1); g.stroke();
  g.rotate(0.5);
  g.beginPath(); g.moveTo(-s * 0.2, s * 0.3); g.lineTo(-s * 0.35, s * 1.1); g.moveTo(s * 0.2, s * 0.3); g.lineTo(s * 0.35, s * 1.1); g.stroke();
  g.restore();
}

export function mapNM() {
  return canvasTex(512, 400, (g, w, h) => {
    g.fillStyle = '#d9cfb3'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 4000, 0.1, 21);
    // Rough state outline (New Mexico is almost a box with the bootheel in the south-west).
    g.strokeStyle = '#4a4436'; g.lineWidth = 3;
    g.beginPath();
    g.moveTo(70, 60); g.lineTo(440, 60); g.lineTo(440, 330); g.lineTo(200, 330); g.lineTo(200, 350); g.lineTo(110, 350); g.lineTo(110, 370); g.lineTo(70, 370); g.closePath(); g.stroke();
    g.strokeStyle = '#9b3b2c'; g.lineWidth = 2;
    const roads = [[[70, 160], [440, 175]], [[230, 60], [240, 330]], [[120, 230], [440, 250]], [[300, 175], [370, 300]]];
    roads.forEach(([a, b]) => { g.beginPath(); g.moveTo(a[0], a[1]); g.lineTo(b[0], b[1]); g.stroke(); });
    g.fillStyle = '#2d2a22'; g.font = '500 30px Oswald'; g.fillText('NEW MEXICO', 160, 44);
    g.font = '16px Special Elite';
    const city = (x: number, y: number, n: string) => { g.beginPath(); g.arc(x, y, 4, 0, 7); g.fill(); g.fillText(n, x + 8, y + 5); };
    city(236, 168, 'Albuquerque'); city(255, 120, 'Santa Fe'); city(372, 292, 'Roswell'); city(212, 318, 'Las Cruces');
    g.strokeStyle = '#b3261e'; g.lineWidth = 3;
    g.beginPath(); g.moveTo(150, 228); g.lineTo(166, 244); g.moveTo(166, 228); g.lineTo(150, 244); g.stroke();
    g.fillStyle = '#b3261e'; g.font = '22px Reenie Beanie'; g.fillText('SARO', 128, 268);
  });
}

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
    dishIcon(g, 64, 50, 18, '#2a2f38');
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
    g.fillStyle = '#e9e3d1'; g.fillRect(0, 0, w, h);
    g.strokeStyle = '#3a3a3a'; g.lineWidth = 4; g.strokeRect(4, 4, w - 8, h - 8);
    g.fillStyle = '#1f2326'; g.textAlign = 'center';
    g.font = '500 32px Oswald'; g.fillText(text, w / 2, sub ? 46 : 60);
    if (sub) { g.font = '18px Oswald'; g.fillText(sub, w / 2, 76); }
  });
}

export function deskPapers(seed = 1) {
  return canvasTex(128, 160, (g, w, h) => {
    g.fillStyle = '#e8e1cd'; g.fillRect(0, 0, w, h);
    const r = rng(seed);
    g.fillStyle = 'rgba(40,50,80,.55)';
    for (let y = 18; y < h - 10; y += 9) g.fillRect(12, y, 30 + r() * 80, 2);
  });
}

// The night work order on the supervisor desk: typed form, Ward's initials, a red stamp.
export function workOrderSheet() {
  return canvasTex(256, 352, (g, w, h) => {
    g.fillStyle = '#ece5d1'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 900, 0.05, 23);
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
  return canvasTex(64, 256, (g, w, h) => {
    g.fillStyle = '#25262a'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 1500, 0.2, 9);
    g.fillStyle = '#c99a2e'; g.fillRect(29, 0, 3, 140); g.fillRect(34, 0, 3, 140);
    g.fillStyle = '#9a9a92'; g.fillRect(2, 0, 2, h); g.fillRect(w - 4, 0, 2, h);
  }, [1, 60]);
}

// ---------- Chapter one: service yard and photo lab ----------

// Plain label: dark text on a light card, one line per entry. Size is in canvas pixels.
export function labelCard(lines: string[], o: { w?: number; h?: number; bg?: string; fg?: string; font?: string; size?: number; border?: boolean; align?: CanvasTextAlign } = {}) {
  const w = o.w ?? 256, h = o.h ?? 160;
  return canvasTex(w, h, (g) => {
    g.fillStyle = o.bg ?? '#e6dfca'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, Math.floor(w * h / 60), 0.08, lines.length + w);
    if (o.border !== false) { g.strokeStyle = o.fg ?? '#25292c'; g.lineWidth = Math.max(2, w / 90); g.strokeRect(w * 0.03, h * 0.04, w * 0.94, h * 0.92); }
    const size = o.size ?? Math.min(34, Math.floor((h * 0.8) / Math.max(1, lines.length) * 0.78));
    g.fillStyle = o.fg ?? '#1f2326'; g.textAlign = o.align ?? 'center'; g.textBaseline = 'middle';
    g.font = `500 ${size}px ${o.font ?? 'Oswald'}`;
    const x = (o.align ?? 'center') === 'center' ? w / 2 : w * 0.1;
    const top = h / 2 - ((lines.length - 1) * size * 1.2) / 2;
    lines.forEach((l, i) => g.fillText(l, x, top + i * size * 1.2));
  });
}

export function cameraCard() {
  return labelCard(['FIELD CAMERA', 'C  /  VIEWFINDER', 'SPACE  /  SHUTTER'], { w: 256, h: 160, font: 'VT323', size: 30 });
}

// SARO enamel sign on the yard fence (reference image: service yard at S-03)
export function yardSign() {
  return canvasTex(512, 256, (g, w, h) => {
    g.fillStyle = '#d9d4c4'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 4000, 0.14, 61);
    g.strokeStyle = '#2b2f33'; g.lineWidth = 6; g.strokeRect(10, 10, w - 20, h - 20);
    g.fillStyle = '#23272b'; g.font = '500 70px Oswald'; g.fillText('SARO', 36, 92);
    dishIcon(g, 270, 62, 22, '#23272b');
    g.font = '18px Oswald'; g.fillText('SOUTHWEST ASTRONOMICAL', 36, 122); g.fillText('RESEARCH OBSERVATORY', 36, 144);
    g.fillRect(36, 158, w - 72, 3);
    g.font = '500 44px Oswald'; g.fillText('SERVICE YARD', 36, 206);
    g.font = '20px Oswald'; g.fillText('AUTHORIZED PERSONNEL ONLY', 36, 236);
    // rust at the bolts
    for (const [x, y] of [[22, 22], [w - 22, 22], [22, h - 22], [w - 22, h - 22]]) { g.fillStyle = 'rgba(120,60,20,.55)'; g.beginPath(); g.arc(x, y, 7, 0, 7); g.fill(); }
  });
}

// Grey steel cabinet door with stencil and a small inspection window
export function cabinetFace(title: string, sub: string[], seed = 3) {
  return canvasTex(256, 384, (g, w, h) => {
    g.fillStyle = '#5b6463'; g.fillRect(0, 0, w, h);
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

// Faded paint stencil on the concrete walk
export function floorStencil(lines: string[], w = 512, h = 256) {
  const t = canvasTex(w, h, (g) => {
    g.clearRect(0, 0, w, h);
    g.fillStyle = 'rgba(232,206,120,.82)'; g.strokeStyle = 'rgba(232,206,120,.82)'; g.lineWidth = 8;
    g.strokeRect(14, 14, w - 28, h - 28);
    g.font = `500 ${Math.floor(h / (lines.length + 1.2))}px Oswald`; g.textAlign = 'center'; g.textBaseline = 'middle';
    lines.forEach((l, i) => g.fillText(l, w / 2, h / 2 + (i - (lines.length - 1) / 2) * h / (lines.length + 0.8)));
    // wear
    const r = rng(lines[0].length * 13);
    g.globalCompositeOperation = 'destination-out';
    for (let i = 0; i < 900; i++) { g.fillStyle = `rgba(0,0,0,${r() * 0.8})`; g.fillRect(r() * w, r() * h, 1 + r() * 7, 1 + r() * 3); }
  });
  return t;
}

// B-12 reference vane: dark board, one ivory stripe, scale ticks (as in the Unity photographs)
export function vaneFace() {
  return canvasTex(128, 512, (g, w, h) => {
    g.fillStyle = '#16191b'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#efe6cd'; g.fillRect(w * 0.22, h * 0.05, w * 0.3, h * 0.9);
    g.fillStyle = '#c9c0a4';
    for (let i = 0; i < 12; i++) g.fillRect(w * 0.66, h * 0.08 + i * h * 0.075, w * 0.18, 4);
    g.strokeStyle = '#3a3f42'; g.lineWidth = 6; g.strokeRect(3, 3, w - 6, h - 6);
  });
}
export function echoFace() {
  return canvasTex(128, 512, (g, w, h) => {
    g.clearRect(0, 0, w, h);
    const gr = g.createLinearGradient(w * 0.15, 0, w * 0.6, 0);
    gr.addColorStop(0, 'rgba(240,236,214,0)'); gr.addColorStop(0.25, 'rgba(240,236,214,.75)'); gr.addColorStop(0.75, 'rgba(240,236,214,.75)'); gr.addColorStop(1, 'rgba(240,236,214,0)');
    g.fillStyle = gr; g.fillRect(w * 0.15, h * 0.05, w * 0.45, h * 0.9);
  });
}
// R-07: the other survey marker, three horizontal bars
export function barsBoard() {
  return canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#16191b'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#efe6cd'; for (let i = 0; i < 3; i++) g.fillRect(w * 0.12, h * (0.18 + i * 0.25), w * 0.76, h * 0.12);
    g.fillStyle = '#c9c0a4'; g.font = '24px Oswald'; g.textAlign = 'center'; g.fillText('R-07', w / 2, h - 10);
  });
}

// Field map on the photo lab wall: control room, yard walk, S-03 motor bus, B-12, the lab
export function fieldMap() {
  return canvasTex(512, 384, (g, w, h) => {
    g.fillStyle = '#ddd4ba'; g.fillRect(0, 0, w, h);
    speckle(g, w, h, 5000, 0.1, 77);
    g.fillStyle = '#2a2a26'; g.font = '500 30px Oswald'; g.fillText('SARO / FIELD MAP', 24, 40);
    g.font = '15px Special Elite'; g.fillText('SERVICE YARD, EAST WALK', 24, 62);
    g.strokeStyle = '#3c3a33'; g.lineWidth = 3;
    // control room (west), walk going north (up), lab east of the walk
    g.strokeRect(60, 250, 130, 100); g.fillText('CONTROL ROOM', 66, 300);
    g.strokeRect(220, 90, 46, 270); g.fillText('EAST WALK', 274, 340);
    g.strokeRect(300, 230, 130, 90); g.fillText('PHOTO LAB', 320, 280);
    g.fillStyle = '#9b3b2c';
    g.beginPath(); g.arc(258, 190, 7, 0, 7); g.fill(); g.fillText('S-03 / MOTOR BUS', 300, 195);
    g.beginPath(); g.arc(240, 110, 7, 0, 7); g.fill(); g.fillText('B-12 / OPTICAL', 300, 112);
    g.strokeStyle = '#9b3b2c'; g.setLineDash([6, 6]); g.beginPath(); g.moveTo(243, 200); g.lineTo(236, 30); g.stroke(); g.setLineDash([]);
    g.fillText('TO S-03 (ANTENNA)', 140, 26);
    g.fillStyle = '#2a2a26'; g.font = '22px Reenie Beanie'; g.fillText('fence', 120, 84);
    g.strokeStyle = '#2a2a26'; g.beginPath(); g.moveTo(20, 80); g.lineTo(w - 20, 80); g.stroke();
    g.font = '16px Oswald'; g.fillText('N', w - 40, 40); g.beginPath(); g.moveTo(w - 34, 48); g.lineTo(w - 34, 90); g.stroke();
  });
}

// Motor bus display behind the inspection glass. Drawn by the chapter code.
export function screenCanvas(w = 256, h = 160) {
  const c = document.createElement('canvas'); c.width = w; c.height = h;
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace;
  return { canvas: c, ctx: c.getContext('2d')!, tex: t };
}
