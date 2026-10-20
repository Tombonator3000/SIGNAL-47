// A face on the film before 05:29 (FORSLAG.md, 20 October, "Valg for Tom" 5; Tom: "Spenning og
// skummelt slår faste regler"). Once in the night, on the first free frame taken out of doors in
// chapter four (core/album.ts decides), the print keeps something the eye did not see: at the
// edge of the frame and a foot from the lens, out of focus, the side of a pale head and one
// black eye with the flash in it. It is never in the scene, only on the print.
//
// It is drawn here, in two dimensions, on the frame as the renderer gave it and before the
// film look (core/FieldCamera.ts), so the grain, the warm cast and the vignette go over it as
// over everything else on the print. A thing that close to a lens focused on the night is a
// soft shape: the head is drawn sharp on a layer of its own, then thrown out of focus with its
// edges (a box blur three times over, which is near enough a gaussian and does not need the
// canvas filter some phones lack). It is lit the way an on-camera flash lights what is nearest:
// brightest where it faces the lens, falling off fast over the curve of the skull and towards
// the back, the edges dark. What is blown out spills a little light over its own edge
// (halation), and the flash shows in the eye as one soft hot disc, the way a highlight goes out
// of focus. Its shape is the face in FLASH (story/flashFrames.ts and Codex's flash_face.jpg):
// the big skull, the narrow chin, a black almond eye slanting up towards the temple. Only part
// of it, turned towards the lens.
//
// Everything is laid out on a 640 x 400 frame and scaled to the print's own size (640, or 320
// where the saves have no IndexedDB). The same seed always gives the same picture.

const DW = 640, DH = 400;

function lcg(seed: number) {
  let s = seed >>> 0 || 1;
  return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
}

/** The head's outline in the 640 x 400 frame, the head at the right edge, turned towards the lens:
 *  the front of the face is the line nearest the middle of the frame, the skull runs off the top
 *  and the right, the chin off the bottom. */
function headPath(g: CanvasRenderingContext2D) {
  g.beginPath();
  g.moveTo(500, -80);
  g.bezierCurveTo(452, -30, 424, 46, 428, 112);       // the dome of the skull, bulging forward
  g.bezierCurveTo(430, 146, 444, 160, 452, 178);      // the brow
  g.bezierCurveTo(458, 192, 452, 204, 456, 222);      // a dip where the eye sits in
  g.bezierCurveTo(462, 250, 474, 262, 486, 290);      // the cheekbone
  g.bezierCurveTo(500, 320, 520, 340, 546, 360);      // the cheek, falling away to the chin
  g.bezierCurveTo(566, 377, 582, 390, 590, 396);      // the narrow chin
  g.bezierCurveTo(616, 388, 656, 364, 688, 334);      // the jaw, back up towards where an ear would be
  g.lineTo(780, 300); g.lineTo(780, -80);
  g.closePath();
}

/** The eye in its own frame: the inner corner at -L/2, the outer at +L/2, y down. The lower lid is
 *  fuller at the inner side; the outer corner is drawn out to a point. */
function eyePath(g: CanvasRenderingContext2D, L: number, Hh: number) {
  g.beginPath();
  g.moveTo(-L / 2, Hh * 0.08);
  g.bezierCurveTo(-L * 0.34, -Hh * 0.8, L * 0.2, -Hh * 0.7, L / 2, -Hh * 0.04);
  g.bezierCurveTo(L * 0.28, Hh * 0.52, -L * 0.18, Hh * 0.9, -L / 2, Hh * 0.08);
  g.closePath();
}

// where the eye is, its size and its slant, in the 640 x 400 frame
const EYE = { x: 524, y: 174, L: 120, H: 48, tilt: -0.44 };
// it leans in from the edge, the top of the head towards the middle of the frame
const LEAN = { x: 640, y: 400, a: -0.1 };
const lean = (g: CanvasRenderingContext2D) => { g.translate(LEAN.x, LEAN.y); g.rotate(LEAN.a); g.translate(-LEAN.x, -LEAN.y); };

/** A soft oval of colour `rgb` fading out from `a` at its middle. */
function blob(g: CanvasRenderingContext2D, x: number, y: number, rx: number, ry: number, rot: number, rgb: string, a: number) {
  g.save(); g.translate(x, y); g.rotate(rot); g.scale(1, ry / rx);
  const d = g.createRadialGradient(0, 0, 0, 0, 0, rx);
  d.addColorStop(0, `rgba(${rgb},${a})`); d.addColorStop(1, `rgba(${rgb},0)`);
  g.fillStyle = d; g.beginPath(); g.arc(0, 0, rx, 0, 7); g.fill(); g.restore();
}

/** Three box blurs, rows then columns, on premultiplied RGBA inside [x0, x1) x [y0, y1). */
function blur(img: ImageData, rad: number, x0: number, y0: number, x1: number, y1: number) {
  const W = img.width, d = img.data, w = x1 - x0, h = y1 - y0;
  if (rad < 1 || w <= 0 || h <= 0) return;
  const buf = new Float32Array(w * h * 4);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = ((y + y0) * W + x + x0) * 4, j = (y * w + x) * 4, a = d[i + 3] / 255;
    buf[j] = d[i] * a; buf[j + 1] = d[i + 1] * a; buf[j + 2] = d[i + 2] * a; buf[j + 3] = d[i + 3];
  }
  const tmp = new Float32Array(Math.max(w, h) * 4);
  const pass = (len: number, count: number, at: (n: number, k: number) => number) => {
    const r = Math.min(rad, len - 1), inv = 1 / (2 * r + 1);
    for (let n = 0; n < count; n++) {
      for (let c = 0; c < 4; c++) {
        // edges clamped: the layer is transparent there anyway, or the head runs on off the frame
        let sum = buf[at(n, 0) + c] * (r + 1);
        for (let k = 1; k <= r; k++) sum += buf[at(n, Math.min(len - 1, k)) + c];
        for (let k = 0; k < len; k++) {
          tmp[k * 4 + c] = sum * inv;
          sum += buf[at(n, Math.min(len - 1, k + r + 1)) + c] - buf[at(n, Math.max(0, k - r)) + c];
        }
      }
      for (let k = 0; k < len; k++) { const o = at(n, k); buf[o] = tmp[k * 4]; buf[o + 1] = tmp[k * 4 + 1]; buf[o + 2] = tmp[k * 4 + 2]; buf[o + 3] = tmp[k * 4 + 3]; }
    }
  };
  for (let p = 0; p < 3; p++) {
    pass(w, h, (row, k) => (row * w + k) * 4);
    pass(h, w, (col, k) => (k * w + col) * 4);
  }
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = ((y + y0) * W + x + x0) * 4, j = (y * w + x) * 4, a = buf[j + 3];
    const k = a > 0.5 ? 255 / a : 0;
    d[i] = Math.min(255, buf[j] * k); d[i + 1] = Math.min(255, buf[j + 1] * k); d[i + 2] = Math.min(255, buf[j + 2] * k); d[i + 3] = a;
  }
}

/** Puts the face on a free frame, in place: `raw` is the frame as rendered, before the film look.
 *  `side` 1 has it at the right edge, -1 at the left. */
export function filmFace(raw: HTMLCanvasElement, seed: number, side: 1 | -1 = 1) {
  const W = raw.width, H = raw.height, s = W / DW;
  const r = lcg(seed * 2654435761);
  const layer = document.createElement('canvas'); layer.width = W; layer.height = H;
  const g = layer.getContext('2d', { willReadFrequently: true })!;
  g.save();
  if (side < 0) { g.translate(W, 0); g.scale(-1, 1); }
  g.scale(s, H / DH);
  lean(g);

  // ---- the head, lit by the flash ----
  g.save();
  headPath(g); g.clip();
  // at a grazing angle the flash hardly lights it: that is the colour of the skin in shadow
  // (a little cold before the print's warm cast goes over it: on the print it comes out grey)
  g.fillStyle = 'rgb(50,56,57)'; g.fillRect(400, -80, 400, 520);
  // where it faces the lens it is nearly white: the temple and the dome over the eye
  const hot = g.createRadialGradient(504, 134, 6, 514, 150, 200);
  hot.addColorStop(0, 'rgb(234,246,246)'); hot.addColorStop(0.2, 'rgb(214,228,228)');
  hot.addColorStop(0.46, 'rgb(158,170,170)'); hot.addColorStop(0.76, 'rgb(90,98,99)'); hot.addColorStop(1, 'rgb(50,56,57)');
  g.fillStyle = hot; g.fillRect(400, -80, 400, 520);
  // the cheekbone under the eye catches it too
  const cheek = g.createRadialGradient(496, 258, 4, 496, 258, 66);
  cheek.addColorStop(0, 'rgba(240,252,252,.26)'); cheek.addColorStop(1, 'rgba(240,252,252,0)');
  g.fillStyle = cheek; g.fillRect(400, 180, 200, 170);
  // further from the flash, darker: the back of the skull, and down towards the jaw
  const back = g.createLinearGradient(590, 0, 700, 0);
  back.addColorStop(0, 'rgba(8,10,10,0)'); back.addColorStop(1, 'rgba(8,10,10,.6)');
  g.fillStyle = back; g.fillRect(400, -80, 400, 520);
  const low = g.createLinearGradient(0, 214, 0, 384);
  low.addColorStop(0, 'rgba(6,8,8,0)'); low.addColorStop(0.55, 'rgba(6,8,8,.5)'); low.addColorStop(1, 'rgba(6,8,8,.86)');
  g.fillStyle = low; g.fillRect(400, -80, 400, 520);
  // and the dome curving away over the top
  const top = g.createLinearGradient(0, 70, 0, -60);
  top.addColorStop(0, 'rgba(8,10,10,0)'); top.addColorStop(1, 'rgba(8,10,10,.58)');
  g.fillStyle = top; g.fillRect(400, -80, 400, 520);
  // the bone under the skin: the temple sunk in behind the eye, the hollow under the cheekbone
  const dent = (x: number, y: number, rx: number, ry: number, a: number, rot = 0) => blob(g, x, y, rx, ry, rot, '14,16,16', a);
  dent(604, 150, 46, 78, 0.3, -0.2);
  dent(560, 318, 56, 30, 0.26, -0.6);
  dent(EYE.x - 6, EYE.y - 34, 70, 22, 0.16, EYE.tilt);   // a soft fold over the eye
  // the skin: a faint mottling, almost lost when it goes out of focus
  for (let i = 0; i < 260; i++) {
    const x = 420 + r() * 260, y = -20 + r() * 420, rr = 3 + r() * 14, v = r() < 0.5 ? 0 : 255;
    g.fillStyle = `rgba(${v},${v},${v},${0.025 + r() * 0.04})`;
    g.beginPath(); g.ellipse(x, y, rr, rr * (0.5 + r() * 0.5), r() * 3, 0, 7); g.fill();
  }
  // round the edge the surface turns away from the lens and the light goes off it
  headPath(g);
  for (let i = 0; i < 6; i++) { g.strokeStyle = 'rgba(14,16,16,.17)'; g.lineWidth = 10 + i * 14; g.stroke(); }
  // skin, not plaster: a broad soft sheen of the flash along the brow and on the cheekbone
  blob(g, EYE.x - 12, EYE.y - 52, 64, 13, EYE.tilt - 0.1, '255,255,255', 0.2);
  blob(g, EYE.x - 30, EYE.y + 74, 30, 10, -0.5, '255,255,255', 0.16);
  // a shallow hollow round the eye
  g.save();
  g.translate(EYE.x, EYE.y); g.rotate(EYE.tilt);
  const hol = g.createRadialGradient(0, 4, EYE.L * 0.3, 0, 4, EYE.L * 0.82);
  hol.addColorStop(0, 'rgba(30,32,32,.34)'); hol.addColorStop(1, 'rgba(30,32,32,0)');
  g.fillStyle = hol; g.scale(1, 0.62); g.beginPath(); g.arc(0, 0, EYE.L * 0.82, 0, 7); g.fill();
  g.restore();
  g.restore();
  // the thin neck, behind the jaw and out of the light
  g.save();
  g.beginPath(); g.moveTo(642, 352); g.bezierCurveTo(660, 376, 664, 396, 662, 440); g.lineTo(780, 440); g.lineTo(780, 300); g.lineTo(688, 334); g.closePath();
  g.fillStyle = 'rgb(22,25,25)'; g.fill();
  g.restore();

  // ---- the eye: black, wet, too big ----
  g.save();
  g.translate(EYE.x, EYE.y); g.rotate(EYE.tilt);
  // the lid's edge, darker where it folds in
  eyePath(g, EYE.L + 10, EYE.H + 9); g.fillStyle = 'rgba(40,42,42,.7)'; g.fill();
  eyePath(g, EYE.L, EYE.H); g.fillStyle = 'rgb(5,6,7)'; g.fill();
  g.clip();
  // the dull sheen of the night on its upper curve
  const sheen = g.createRadialGradient(-EYE.L * 0.12, -EYE.H * 0.5, 2, -EYE.L * 0.05, -EYE.H * 0.3, EYE.L * 0.5);
  sheen.addColorStop(0, 'rgba(120,130,134,.30)'); sheen.addColorStop(1, 'rgba(120,130,134,0)');
  g.fillStyle = sheen; g.fillRect(-EYE.L, -EYE.H, EYE.L * 2, EYE.H * 2);
  g.restore();
  g.save();
  g.translate(EYE.x, EYE.y); g.rotate(EYE.tilt);
  // and the wet line of the lower lid
  g.beginPath(); g.moveTo(-EYE.L * 0.44, EYE.H * 0.22); g.bezierCurveTo(-EYE.L * 0.16, EYE.H * 0.76, EYE.L * 0.2, EYE.H * 0.46, EYE.L * 0.44, EYE.H * 0.06);
  g.strokeStyle = 'rgba(214,220,214,.38)'; g.lineWidth = 2.4; g.stroke();
  g.restore();
  g.restore();

  // ---- out of focus: it is far too close for the lens ----
  const x0 = side > 0 ? Math.floor(340 * s) : 0, x1 = side > 0 ? W : Math.ceil(W - 340 * s);
  const img = g.getImageData(x0, 0, x1 - x0, H);
  blur(img, Math.max(1, Math.round(6 * s)), 0, 0, x1 - x0, H);
  g.putImageData(img, x0, 0);

  // ---- onto the print ----
  const p = raw.getContext('2d')!;
  // a little smear down the frame: it was moving when the shutter went
  p.globalAlpha = 0.2; p.drawImage(layer, 0, 4 * s);
  p.globalAlpha = 1; p.drawImage(layer, 0, 0);
  // what the flash blew out spills light over its own edge
  const q = Math.max(1, Math.round(4 * s)), gw = Math.ceil(W / q), gh = Math.ceil(H / q);
  const glow = document.createElement('canvas'); glow.width = gw; glow.height = gh;
  const gg = glow.getContext('2d', { willReadFrequently: true })!;
  gg.drawImage(layer, 0, 0, gw, gh);
  const gi = gg.getImageData(0, 0, gw, gh), gd = gi.data;
  for (let i = 0; i < gd.length; i += 4) {
    const l = (gd[i] * 0.3 + gd[i + 1] * 0.59 + gd[i + 2] * 0.11) * gd[i + 3] / 255;
    const k = Math.max(0, (l - 168) / 87);
    gd[i] = 236; gd[i + 1] = 240; gd[i + 2] = 234; gd[i + 3] = Math.round(255 * k * k);
  }
  blur(gi, 5, 0, 0, gw, gh);
  gg.putImageData(gi, 0, 0);
  p.globalCompositeOperation = 'screen'; p.globalAlpha = 0.55;
  p.drawImage(glow, 0, 0, W, H);
  p.globalCompositeOperation = 'source-over'; p.globalAlpha = 1;

  // ---- the flash in the eye: one soft hot disc, the way a highlight goes out of focus ----
  p.save();
  if (side < 0) { p.translate(W, 0); p.scale(-1, 1); }
  p.scale(s, H / DH);
  lean(p);
  const c = Math.cos(EYE.tilt), sn = Math.sin(EYE.tilt);
  const at = (ex: number, ey: number) => [EYE.x + ex * c - ey * sn, EYE.y + ex * sn + ey * c] as const;
  const [hx, hy] = at(-EYE.L * 0.2, -EYE.H * 0.22);
  const hl = p.createRadialGradient(hx, hy, 0, hx, hy, 7);
  hl.addColorStop(0, 'rgba(255,255,255,.95)'); hl.addColorStop(0.45, 'rgba(240,250,250,.7)'); hl.addColorStop(1, 'rgba(240,250,250,0)');
  p.fillStyle = hl; p.beginPath(); p.arc(hx, hy, 7, 0, 7); p.fill();
  p.restore();
}

/** Where on the frame the eye is (u, v from the top left), for the tests. */
export function filmFaceEye(side: 1 | -1 = 1) {
  const u = EYE.x / DW;
  return { u: side > 0 ? u : 1 - u, v: EYE.y / DH };
}
