// A face on the film before 05:29 (FORSLAG.md, 20 October, "Valg for Tom" 5; Tom: "Spenning og
// skummelt slår faste regler"). Once in the night, on the first free frame taken out of doors in
// chapter four (core/album.ts decides), the print keeps something the eye did not see: at the
// edge of the frame and a foot from the lens, out of focus, the side of a pale head and one
// black eye with the flash in it. It is never in the scene, only on the print.
//
// It is drawn here, on the frame as the renderer gave it and before the film look
// (core/FieldCamera.ts), so the grain, the warm cast and the vignette go over it as over
// everything else on the print. It is not painted as a picture of a face but lit as a thing a
// foot from an on-camera flash: the head is a surface (two ellipsoids, the big skull and the
// narrow face, with a socket for the eye, a brow over it and a cheekbone under it), and every
// point of it gets the flash by how squarely it faces the lens and by the square of its
// distance. So it is near white where it is nearest and turned to the lens, it falls off fast
// over the curve of the skull, and its far edges are dark, a mass with no stars in it. A thing
// that close to a lens focused on the night is a soft shape: the layer is thrown out of focus
// (a box blur three times over, near enough a gaussian, without the canvas filter some phones
// lack). What the flash blew out spills a little light over its edge, and the flash shows in
// the eye as one soft disc, the way a highlight goes out of focus.
//
// It is the face in FLASH (story/flashFrames.ts and Codex's flash_face.jpg), so the end can be
// recognised: the big skull, the narrow chin, a black teardrop eye slanting up to the temple
// with the light high on its inner side. Only part of it: the head is turned to the lens, its
// middle is at the very edge of the print and the other eye is past it, the lower face goes
// into the dark.
//
// Laid out on a 640 x 400 frame (the head at the right edge; -1 mirrors it to the left), worked
// out on a layer of half that, and scaled to the print's own size (640, or 320 where the saves
// have no IndexedDB). The same seed always gives the same picture.

const DW = 640, DH = 400;
/** The layer the head is worked out on: half the frame, which the blur hides. */
const LW = 320, LH = 200, STEP = DW / LW;

function lcg(seed: number) {
  let s = seed >>> 0 || 1;
  return () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
}

/** Smooth value noise, 0 to 1, repeating every 64 units. */
function noise(seed: number) {
  const r = lcg(seed), t = new Float32Array(64 * 64);
  for (let i = 0; i < t.length; i++) t[i] = r();
  return (x: number, y: number) => {
    const xi = Math.floor(x), yi = Math.floor(y), fx = x - xi, fy = y - yi;
    const x0 = xi & 63, x1 = (xi + 1) & 63, y0 = (yi & 63) * 64, y1 = ((yi + 1) & 63) * 64;
    const a = t[y0 + x0], b = t[y0 + x1], c = t[y1 + x0], d = t[y1 + x1];
    const sx = fx * fx * (3 - 2 * fx), sy = fy * fy * (3 - 2 * fy);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
  };
}

const clamp01 = (v: number) => v < 0 ? 0 : v > 1 ? 1 : v;
const smooth = (a: number, b: number, v: number) => { const t = clamp01((v - a) / (b - a)); return t * t * (3 - 2 * t); };

// The head, upright in its own frame, at the right edge: its middle (MID) is just past the edge,
// so what is on the print is the near side of the face, the temple and the skull. The skull is
// big and the face under it narrow, running to a small chin; the lower face goes back from the
// lens, so the flash leaves it.
const MID = 684;
const SKULL = { y: 70, rx: 206, ry: 214, rz: 190 };
const FACE = { y: 236, rx: 156, ry: 172, rz: 150, z: -10, taper: 0.55 };
/** Below the eye the face goes back from the lens, so much per unit down. */
const RECEDE = { y: 200, k: 0.7 };
// It leans in from the edge, the top of the head towards the middle of the frame, as if to see.
const LEAN = { x: 620, y: 430, a: -0.13 };
// The eye: its middle, its length and height, and its slant (the outer end, towards the temple
// and the middle of the frame, up). The socket it sits in, the brow over it and the cheekbone under.
const EYE = { x: 594, y: 182, L: 126, H: 58, tilt: 0.4 };
const SOCKET = 12, BROW = 14, CHEEK = 14;
/** The flash, a foot from the face, which is turned to it. */
const CAM = { x: 640, y: 150, z: -320 };
/** How much flash reaches the nearest skin, and the skin (a little cold before the print's warm cast). */
const FLASH = 3.2, SKIN = [0.74, 0.81, 0.83];

/** Upright-head coordinates of a point on the frame (undoes the lean). */
function unlean(x: number, y: number): [number, number] {
  const c = Math.cos(LEAN.a), s = Math.sin(LEAN.a), dx = x - LEAN.x, dy = y - LEAN.y;
  return [LEAN.x + dx * c + dy * s, LEAN.y - dx * s + dy * c];
}
/** Where a point of the head (upright) is on the frame. */
function lean(x: number, y: number): [number, number] {
  const c = Math.cos(LEAN.a), s = Math.sin(LEAN.a), dx = x - LEAN.x, dy = y - LEAN.y;
  return [LEAN.x + dx * c - dy * s, LEAN.y + dx * s + dy * c];
}

/** The head's depth at an upright point (further from the lens is larger), and how much of it is there. */
function depth(hx: number, hy: number, wr: (x: number, y: number) => number): [number, number] {
  const u1 = (hx - MID) / SKULL.rx, v1 = (hy - SKULL.y) / SKULL.ry, t1 = 1 - u1 * u1 - v1 * v1;
  const k = clamp01((hy - FACE.y) / FACE.ry), rxf = FACE.rx * (1 - FACE.taper * k * k);
  const u2 = (hx - MID) / rxf, v2 = (hy - FACE.y) / FACE.ry, t2 = 1 - u2 * u2 - v2 * v2;
  const cov = smooth(0, 0.035, Math.max(t1, t2));
  if (cov <= 0) return [0, 0];
  const z1 = t1 > 0 ? SKULL.rz - SKULL.rz * Math.sqrt(t1) : 1e4;
  const z2 = t2 > 0 ? FACE.rz + FACE.z - FACE.rz * Math.sqrt(t2) : 1e4;
  // the skull and the face run into each other without a seam
  const h = clamp01(0.5 + 0.5 * (z2 - z1) / 30);
  let z = z2 + (z1 - z2) * h - 30 * h * (1 - h);
  z += Math.max(0, hy - RECEDE.y) * RECEDE.k;
  // the eye's socket, the brow over it, the cheekbone under it, a few soft folds round it
  const c = Math.cos(EYE.tilt), s = Math.sin(EYE.tilt), ex = hx - EYE.x, ey = hy - EYE.y;
  const lu = ex * c + ey * s, lv = -ex * s + ey * c;
  const r = Math.hypot(lu / (EYE.L * 0.64), lv / (EYE.H * 1.2));
  z += SOCKET * (1 - smooth(0.42, 1.3, r));
  z -= BROW * Math.exp(-(((lv + EYE.H * 1.05) / (EYE.H * 0.5)) ** 2) - (lu / (EYE.L * 0.62)) ** 2);
  z -= CHEEK * Math.exp(-(((lv - EYE.H * 1.25) / (EYE.H * 0.6)) ** 2) - ((lu + EYE.L * 0.08) / (EYE.L * 0.55)) ** 2);
  if (r > 0.9 && r < 1.9) z += 1.6 * Math.sin((r - 0.9) * 19) * (1 - smooth(1.2, 1.9, r));
  // and the skin is not quite smooth
  z += (wr(hx / 38, hy / 38) - 0.5) * 5;
  return [z, cov];
}

/** The eye in its own frame: the outer end (towards the temple) at -L/2 drawn out to a point, the
 *  inner end at +L/2 full and round, y down. */
function eyePath(g: CanvasRenderingContext2D, L: number, H: number) {
  g.beginPath();
  g.moveTo(-L / 2, 0);
  g.bezierCurveTo(-L * 0.22, -H * 0.64, L * 0.2, -H * 0.8, L * 0.42, -H * 0.4);
  g.bezierCurveTo(L * 0.56, -H * 0.16, L * 0.55, H * 0.32, L * 0.33, H * 0.45);
  g.bezierCurveTo(L * 0.06, H * 0.6, -L * 0.26, H * 0.34, -L / 2, 0);
  g.closePath();
}

/** Three box blurs, rows then columns, on premultiplied RGBA inside [x0, x1) x [0, h). */
function blur(img: ImageData, rad: number, x0: number, x1: number) {
  const W = img.width, h = img.height, d = img.data, w = x1 - x0;
  if (rad < 1 || w <= 0 || h <= 0) return;
  const buf = new Float32Array(w * h * 4);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = (y * W + x + x0) * 4, j = (y * w + x) * 4, a = d[i + 3] / 255;
    buf[j] = d[i] * a; buf[j + 1] = d[i + 1] * a; buf[j + 2] = d[i + 2] * a; buf[j + 3] = d[i + 3];
  }
  const tmp = new Float32Array(Math.max(w, h) * 4);
  const pass = (len: number, count: number, at: (n: number, k: number) => number) => {
    const r = Math.min(rad, len - 1), inv = 1 / (2 * r + 1);
    for (let n = 0; n < count; n++) {
      for (let c = 0; c < 4; c++) {
        // edges clamped: the layer is empty there, or the head runs on off the frame
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
    const i = (y * W + x + x0) * 4, j = (y * w + x) * 4, a = buf[j + 3];
    const k = a > 0.5 ? 255 / a : 0;
    d[i] = Math.min(255, buf[j] * k); d[i + 1] = Math.min(255, buf[j + 1] * k); d[i + 2] = Math.min(255, buf[j + 2] * k); d[i + 3] = a;
  }
}

/** The layer's columns the head can reach, and how far out of focus it is (on the layer). */
const X0 = Math.floor(380 / STEP), BLUR = 3;

/** Puts the face on a free frame, in place: `raw` is the frame as rendered, before the film look.
 *  `side` 1 has it at the right edge, -1 at the left (the same head in a mirror: the lens is in
 *  the middle of the frame, so the flash falls on it the same way). */
export function filmFace(raw: HTMLCanvasElement, seed: number, side: 1 | -1 = 1) {
  const W = raw.width, H = raw.height;
  const wr = noise(seed * 2654435761), mot = noise(seed * 40503 + 7);
  const layer = document.createElement('canvas'); layer.width = LW; layer.height = LH;
  const g = layer.getContext('2d', { willReadFrequently: true })!;

  // ---- the head, as deep as it is, then lit by the flash ----
  const zs = new Float32Array(LW * LH), cs = new Float32Array(LW * LH);
  for (let ly = 0; ly < LH; ly++) for (let lx = X0; lx < LW; lx++) {
    const [hx, hy] = unlean((lx + 0.5) * STEP, (ly + 0.5) * STEP);
    const [z, c] = depth(hx, hy, wr);
    zs[ly * LW + lx] = z; cs[ly * LW + lx] = c;
  }
  const img = g.createImageData(LW, LH), d = img.data;
  for (let ly = 0; ly < LH; ly++) for (let lx = X0; lx < LW; lx++) {
    const i = ly * LW + lx, c = cs[i];
    if (c <= 0) continue;
    // past its edge the surface is taken to go on falling away
    const at = (x: number, y: number) => { const j = Math.min(LH - 1, Math.max(0, y)) * LW + Math.min(LW - 1, Math.max(X0, x)); return cs[j] > 0 ? zs[j] : zs[i] + 40; };
    // the surface's slope, so which way it faces
    const gx = (at(lx + 1, ly) - at(lx - 1, ly)) / (2 * STEP), gy = (at(lx, ly + 1) - at(lx, ly - 1)) / (2 * STEP);
    const nl = 1 / Math.hypot(gx, gy, 1), nx = gx * nl, ny = gy * nl, nz = -nl;
    const px = (lx + 0.5) * STEP, py = (ly + 0.5) * STEP, pz = zs[i];
    let vx = CAM.x - px, vy = CAM.y - py, vz = CAM.z - pz;
    const dist = Math.hypot(vx, vy, vz); vx /= dist; vy /= dist; vz /= dist;
    const lam = Math.max(0, nx * vx + ny * vy + nz * vz);
    const fall = (-CAM.z / dist) ** 3;
    // skin, a little mottled, with a broad wet sheen where it faces the flash squarely
    const m = 0.84 + 0.32 * mot(px / 30, py / 30) + 0.14 * (wr(px / 11, py / 11) - 0.5);
    // (a hard terminator: the light goes off it quickly as it turns away)
    const turn = lam ** 1.7, sheen = 0.2 * lam ** 26, lit = FLASH * fall;
    for (let k = 0; k < 3; k++) d[i * 4 + k] = 255 * (1 - Math.exp(-lit * (turn * SKIN[k] * m + sheen) * 1.55));
    d[i * 4 + 3] = 255 * c;
  }
  g.putImageData(img, 0, 0);

  // ---- the eye: black, wet, too big ----
  const [ex, ey] = lean(EYE.x, EYE.y), tilt = EYE.tilt + LEAN.a;
  g.save();
  g.scale(1 / STEP, 1 / STEP);
  g.translate(ex, ey); g.rotate(tilt);
  // where the lids meet it, the skin folds in, dark
  eyePath(g, EYE.L + 8, EYE.H + 8); g.fillStyle = 'rgba(16,18,18,.4)'; g.fill();
  eyePath(g, EYE.L, EYE.H); g.fillStyle = 'rgb(4,5,6)'; g.fill();
  g.save(); g.clip();
  // the curve of it gives back a little of the flash, high on the inner side
  const sh = g.createRadialGradient(EYE.L * 0.1, -EYE.H * 0.26, 2, EYE.L * 0.06, -EYE.H * 0.1, EYE.L * 0.46);
  sh.addColorStop(0, 'rgba(150,160,164,.2)'); sh.addColorStop(0.5, 'rgba(90,98,102,.07)'); sh.addColorStop(1, 'rgba(90,98,102,0)');
  g.fillStyle = sh; g.fillRect(-EYE.L, -EYE.H, EYE.L * 2, EYE.H * 2);
  g.restore();
  // the wet line of the lower lid
  g.beginPath(); g.moveTo(-EYE.L * 0.4, EYE.H * 0.1); g.bezierCurveTo(-EYE.L * 0.16, EYE.H * 0.5, EYE.L * 0.12, EYE.H * 0.6, EYE.L * 0.36, EYE.H * 0.42);
  g.strokeStyle = 'rgba(220,226,222,.42)'; g.lineWidth = 2.6; g.stroke();
  g.restore();

  // ---- out of focus: it is far too close for the lens ----
  const soft = g.getImageData(0, 0, LW, LH);
  blur(soft, BLUR, X0 - 2 * BLUR, LW);
  g.putImageData(soft, 0, 0);

  // ---- onto the print ----
  const p = raw.getContext('2d')!;
  p.save();
  if (side < 0) { p.translate(W, 0); p.scale(-1, 1); }
  p.imageSmoothingEnabled = true; p.imageSmoothingQuality = 'high';
  p.drawImage(layer, 0, 0, W, H);
  // what the flash blew out spills light over its own edge
  const gw = LW / 2, gh = LH / 2;
  const glow = document.createElement('canvas'); glow.width = gw; glow.height = gh;
  const gg = glow.getContext('2d', { willReadFrequently: true })!;
  gg.drawImage(layer, 0, 0, gw, gh);
  const gi = gg.getImageData(0, 0, gw, gh), gd = gi.data;
  for (let i = 0; i < gd.length; i += 4) {
    const l = (gd[i] * 0.3 + gd[i + 1] * 0.59 + gd[i + 2] * 0.11) * gd[i + 3] / 255;
    const k = Math.max(0, (l - 176) / 79);
    gd[i] = 236; gd[i + 1] = 240; gd[i + 2] = 236; gd[i + 3] = Math.round(255 * k * k);
  }
  blur(gi, 4, 0, gw);
  gg.putImageData(gi, 0, 0);
  p.globalCompositeOperation = 'screen'; p.globalAlpha = 0.5;
  p.drawImage(glow, 0, 0, W, H);
  p.globalCompositeOperation = 'source-over'; p.globalAlpha = 1;

  // ---- the flash in the eye, high on the inner side: one soft spot, the way a highlight goes out of focus ----
  p.scale(W / DW, H / DH);
  const c = Math.cos(tilt), sn = Math.sin(tilt);
  const hx = ex + EYE.L * 0.2 * c + EYE.H * 0.3 * sn, hy = ey + EYE.L * 0.2 * sn - EYE.H * 0.3 * c;
  p.translate(hx, hy); p.rotate(tilt * 0.5); p.scale(1.5, 1);
  const hl = p.createRadialGradient(0, 0, 0, 0, 0, 5);
  hl.addColorStop(0, 'rgba(255,255,255,.86)'); hl.addColorStop(0.55, 'rgba(244,250,250,.64)'); hl.addColorStop(1, 'rgba(240,250,250,0)');
  p.fillStyle = hl; p.beginPath(); p.arc(0, 0, 5, 0, 7); p.fill();
  p.restore();
}

/** Where on the frame the eye is (u, v from the top left), for the tests (tools/horror_face.py). */
export function filmFaceEye(side: 1 | -1 = 1) {
  const [x, y] = lean(EYE.x, EYE.y);
  return { u: side > 0 ? x / DW : 1 - x / DW, v: y / DH };
}
