import { rng } from '../core/textures';
import { artLoaded, artImage, PHOTO_1947, type ArtId } from '../core/art';

// The single frames of FLASH in THE EVENT (KAPITLER.md): an eye, a hand with too long
// fingers on the glass, a face that could be a Grey (three frames), the light over the mesa
// in 1947, Tomás or a young man who could be him, the stars, the green terminal with the
// file, the 1947 photograph. Drawn here in code; Codex's round 11 (ART_BRIEF.md) will
// replace the eye, the hand, the face and the man. All 640 x 400, grainy and dark.

export type Frame = { id: string; canvas: HTMLCanvasElement; ms: number };
const W = 640, H = 400;

function make(seed: number, draw: (g: CanvasRenderingContext2D, r: () => number) => void) {
  const c = document.createElement('canvas'); c.width = W; c.height = H;
  const g = c.getContext('2d')!, r = rng(seed);
  g.fillStyle = '#000'; g.fillRect(0, 0, W, H);
  draw(g, r);
  // film grain, a cold cast and a heavy vignette, like a frame from a tape that should not exist
  const img = g.getImageData(0, 0, W, H), d = img.data;
  for (let i = 0; i < d.length; i += 4) {
    const n = (r() - 0.5) * 46, l = (d[i] + d[i + 1] + d[i + 2]) / 3;
    d[i] = Math.max(0, Math.min(255, l * 0.86 + n)); d[i + 1] = Math.max(0, Math.min(255, l * 1.0 + n)); d[i + 2] = Math.max(0, Math.min(255, l * 0.95 + n));
  }
  g.putImageData(img, 0, 0);
  const v = g.createRadialGradient(W / 2, H / 2, H * 0.25, W / 2, H / 2, W * 0.62);
  v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,.92)');
  g.fillStyle = v; g.fillRect(0, 0, W, H);
  for (let y = 0; y < H; y += 3) { g.fillStyle = 'rgba(0,0,0,.18)'; g.fillRect(0, y, W, 1); }
  return c;
}

// Codex's frame (round 11) when it has loaded, drawn over the whole frame; k varies it a
// little (the face is shown three times).
function art(id: ArtId, seed: number, k = 0) {
  return make(seed, (g) => {
    const im = artImage(id), s = 1 + k * 0.05;
    g.filter = k ? `blur(${k * 1.5}px)` : 'none';
    g.drawImage(im, (W - W * s) / 2 + k * 9, (H - H * s) / 2 + k * 5, W * s, H * s);
    g.filter = 'none';
  });
}

function eye() {
  if (artLoaded('flashEye')) return art('flashEye', 1);
  return make(1, (g) => {
    g.save(); g.translate(W / 2, H / 2);
    const lid = (k: number) => { g.beginPath(); g.moveTo(-260, 0); g.quadraticCurveTo(0, -150 * k, 260, 0); g.quadraticCurveTo(0, 150 * k, -260, 0); };
    lid(1); g.fillStyle = '#7a776e'; g.fill();
    g.clip();
    const iris = g.createRadialGradient(0, 0, 20, 0, 0, 120);
    iris.addColorStop(0, '#0a0a0a'); iris.addColorStop(0.62, '#0e0e0e'); iris.addColorStop(0.7, '#4a4f48'); iris.addColorStop(1, '#22231f');
    g.fillStyle = iris; g.beginPath(); g.arc(0, 0, 120, 0, 7); g.fill();
    // the pupil is too big, and too glossy
    g.fillStyle = '#020202'; g.beginPath(); g.arc(0, 0, 82, 0, 7); g.fill();
    g.fillStyle = 'rgba(255,255,255,.9)'; g.beginPath(); g.arc(-28, -30, 9, 0, 7); g.fill();
    g.fillStyle = 'rgba(255,255,255,.35)'; g.beginPath(); g.arc(24, 22, 4, 0, 7); g.fill();
    g.restore();
    g.strokeStyle = 'rgba(20,20,20,.9)'; g.lineWidth = 10; g.save(); g.translate(W / 2, H / 2); lid(1); g.stroke(); g.restore();
  });
}

function hand() {
  if (artLoaded('flashHand')) return art('flashHand', 2);
  return make(2, (g, r) => {
    // dawn behind a fogged windshield, the hand dark against it
    const sky = g.createLinearGradient(0, 0, 0, H);
    sky.addColorStop(0, '#3a4048'); sky.addColorStop(0.7, '#b4a68e'); sky.addColorStop(1, '#5a5048');
    g.fillStyle = sky; g.fillRect(0, 0, W, H);
    for (let i = 0; i < 220; i++) { g.fillStyle = `rgba(230,230,225,${0.04 + r() * 0.06})`; g.beginPath(); g.arc(r() * W, r() * H, 6 + r() * 30, 0, 7); g.fill(); }
    g.fillStyle = '#0d0d0f';
    g.save(); g.translate(W / 2 + 10, H * 0.78); g.rotate(-0.08);
    g.beginPath(); g.ellipse(0, 0, 70, 58, 0, 0, 7); g.fill();                          // the palm
    const fingers: [number, number, number][] = [[-52, 230, -0.32], [-18, 290, -0.1], [18, 300, 0.08], [52, 250, 0.3]];
    for (const [x, len, a] of fingers) {
      g.save(); g.translate(x, -30); g.rotate(a);
      g.beginPath(); g.roundRect(-9, -len, 18, len, 9); g.fill();
      g.beginPath(); g.ellipse(0, -len, 13, 16, 0, 0, 7); g.fill();                 // pads pressed flat on the glass
      g.restore();
    }
    g.restore();
    // where it touches, the fog is wiped
    g.globalCompositeOperation = 'lighter';
    for (let i = 0; i < 40; i++) { g.fillStyle = 'rgba(40,40,44,.08)'; g.fillRect(W / 2 - 140 + r() * 280, H * 0.1 + r() * H * 0.6, 2, 30 + r() * 60); }
    g.globalCompositeOperation = 'source-over';
  });
}

function face(k: number) {
  if (artLoaded('flashFace')) return art('flashFace', 3 + k, k);
  return make(3 + k, (g, r) => {
    g.save(); g.translate(W / 2 + (k - 1) * 14, H / 2 + 20 + (k - 1) * 6);
    g.filter = `blur(${3 + k * 2}px)`;
    const sk = g.createRadialGradient(-30, -60, 10, 0, -20, 190);
    sk.addColorStop(0, '#8d8b84'); sk.addColorStop(0.6, '#3c3b37'); sk.addColorStop(1, '#0c0c0c');
    g.fillStyle = sk;
    g.beginPath(); g.moveTo(0, -200); g.bezierCurveTo(150, -200, 160, -20, 60, 90); g.quadraticCurveTo(0, 150, -60, 90); g.bezierCurveTo(-160, -20, -150, -200, 0, -200); g.fill();
    g.fillStyle = '#000';
    for (const s of [-1, 1]) { g.save(); g.translate(s * 58, -40); g.rotate(s * 0.45); g.beginPath(); g.ellipse(0, 0, 48, 24, 0, 0, 7); g.fill(); g.restore(); }
    g.restore();
    g.filter = 'none';
    // most of it is lost in the dark and the motion
    g.fillStyle = `rgba(0,0,0,${0.35 + r() * 0.2})`; g.fillRect(0, 0, W * (0.25 + k * 0.08), H);
  });
}

function fromPhoto(seed: number, rect: [number, number, number, number], fallback: (g: CanvasRenderingContext2D, r: () => number) => void, blur = 0) {
  return make(seed, (g, r) => {
    if (artLoaded('photo1947')) {
      const im = artImage('photo1947');
      const sx = im.width / 2400, sy = im.height / 1600;
      g.filter = blur ? `blur(${blur}px) contrast(1.3)` : 'contrast(1.25)';
      g.drawImage(im, rect[0] * sx, rect[1] * sy, rect[2] * sx, rect[3] * sy, 0, 0, W, H);
      g.filter = 'none';
    } else fallback(g, r);
  });
}

function mesaLight() {
  return fromPhoto(7, [700, 200, 400, 250], (g) => {
    g.fillStyle = '#101214'; g.fillRect(0, H * 0.7, W, H * 0.3);
    const l = g.createRadialGradient(W / 2, H * 0.4, 4, W / 2, H * 0.4, 120);
    l.addColorStop(0, 'rgba(255,255,255,1)'); l.addColorStop(0.2, 'rgba(220,220,210,.5)'); l.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = l; g.fillRect(0, 0, W, H);
  });
}
function man() {
  if (artLoaded('flashMan')) return art('flashMan', 8);
  // the surveyor with the rod, far right in the 1947 picture, enlarged until it falls apart
  return fromPhoto(8, [2030, 930, 220, 138], (g) => {
    g.fillStyle = '#2a2a28';
    g.beginPath(); g.ellipse(W / 2, 120, 30, 36, 0, 0, 7); g.fill();
    g.fillRect(W / 2 - 60, 160, 120, 240);
    g.fillRect(W / 2 - 70, 82, 140, 14);
  }, 3);
}
function stars() {
  return make(9, (g, r) => {
    for (let i = 0; i < 900; i++) {
      const b = Math.pow(r(), 4);
      g.fillStyle = `rgba(255,255,255,${0.25 + b * 0.75})`;
      g.fillRect(r() * W, r() * H, b > 0.6 ? 2 : 1, b > 0.6 ? 2 : 1);
    }
    const band = g.createLinearGradient(0, H, W, 0);
    band.addColorStop(0.3, 'rgba(0,0,0,0)'); band.addColorStop(0.5, 'rgba(180,170,200,.18)'); band.addColorStop(0.7, 'rgba(0,0,0,0)');
    g.fillStyle = band; g.fillRect(0, 0, W, H);
  });
}
function terminal() {
  return make(10, (g) => {
    g.fillStyle = '#031006'; g.fillRect(40, 30, W - 80, H - 60);
    g.fillStyle = '#5bff8a'; g.font = '22px VT323, monospace';
    // as the operations terminal shows it (story/nightshift.ts, DIR SURVEY.RAW)
    const row = (n: string, b: number, d: string) => `${n.padEnd(20)}${String(b).padStart(6)}   ${d}`;
    const lines = ['$ DIR SURVEY.RAW', '', 'Directory DUA1:[SURVEY.RAW]', '', row('RUN860412_2338.DAT', 2214, '12-APR-1986 23:38'),
      row('RUN860413_2341.DAT', 2271, '13-APR-1986 23:41'), row('RUN860414_0529.DAT', 0, '14-APR-1986 05:29'), '', '$ _'];
    lines.forEach((l, i) => g.fillText(l, 64, 74 + i * 30));
    const glow = g.createRadialGradient(W / 2, H / 2, 40, W / 2, H / 2, 320);
    glow.addColorStop(0, 'rgba(60,255,120,.08)'); glow.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = glow; g.fillRect(0, 0, W, H);
  });
}
function photo() {
  const [x, y, w, h] = PHOTO_1947.A;
  return fromPhoto(11, [x, y, w, h], (g) => {
    g.fillStyle = '#1a1a18'; g.fillRect(0, 0, W, H);
    g.fillStyle = '#0a0a0a'; g.fillRect(0, H * 0.72, W, H * 0.28);
    g.fillStyle = 'rgba(240,240,230,.8)'; g.beginPath(); g.arc(W * 0.45, H * 0.3, 14, 0, 7); g.fill();
  });
}

/** The frames in the order they are shown, with how long each stays (milliseconds). */
export function flashFrames(): Frame[] {
  const f0 = face(0), f1 = face(1), f2 = face(2);
  return [
    { id: 'eye', canvas: eye(), ms: 90 },
    { id: 'hand', canvas: hand(), ms: 110 },
    { id: 'face1', canvas: f0, ms: 70 }, { id: 'face2', canvas: f1, ms: 70 }, { id: 'face3', canvas: f2, ms: 70 },
    { id: 'light', canvas: mesaLight(), ms: 90 },
    { id: 'man', canvas: man(), ms: 110 },
    { id: 'stars', canvas: stars(), ms: 80 },
    { id: 'terminal', canvas: terminal(), ms: 100 },
    { id: '1947', canvas: photo(), ms: 120 },
  ];
}
