import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { noMerge, floodlit, type FloodSet } from './kit';
import { rng } from '../core/textures';

// The wear of the night's buildings and what stands about them (Tom, 19 October: «more realistic
// surroundings with more detail»; the concept art of Caliche, the motel and STATION 01): every
// wall's grime mottled and darker to its foot and its corners (what reads from across a lot),
// water stains under the copings and the sills, rust under anything iron, the desert's dirt
// splashed up the foot of every wall and drifted against it, dry grass in it, damp blotches,
// patched and flaking stucco where render fails; on the roofs the swamp coolers and the vent
// stacks; on the lots the drums, the cans, the pallets, the tyres and the people's cars, each in
// its own shade on the ground; porch lights and the pools they throw.
//
// One Dressing per area (a material belongs to one set of fake floodlights, kit.ts), and it costs
// that area four draw calls on High: the props (merged, one palette picture), the small props, the
// stains (quads over the walls, one atlas) and the light pools (additive quads). On Low only the
// big props stay: the small ones, the stains and the pools are left out (core/quality.ts,
// userData.highOnly). The props are painted from a palette of swatches, so every colour of every
// car and drum on a lot is the same material, and they carry the shade they stand in as vertex
// colours (an art director's review, 19 October: on High the cars were flat bright boxes pasted
// on the lot; on Ultra the real lamps' shadows had them right).
//
// Nothing here is a collider: the areas add their own round what the player could walk into
// (the builders give back the footprint).

/** Where a decal faces: out of a wall towards +x, -x, +z or -z, or up off the ground. */
export type Face = '+x' | '-x' | '+z' | '-z';
/** A wall face: its plane (x for ±x, z for ±z), the run along it (a0 to a1, in x or z) and its
 *  height (y0 the foot, y1 the top, both world-ish in the area's terms). */
export type Wall = { face: Face; at: number; a0: number; a1: number; y0: number; y1: number };
/** An opening in a wall (a window or a door): along the wall, and from y0 to y1. */
export type Opening = { a0: number; a1: number; y0: number; y1: number };
export type Foot = { minX: number; maxX: number; minZ: number; maxZ: number };

// ---------- the stains: one atlas, drawn once ----------
const AW = 512, AH = 1024;
type Rect = readonly [number, number, number, number];
const REG = {
  streak: [[0, 0, 64, 256], [64, 0, 64, 256], [128, 0, 64, 256], [192, 0, 64, 256]] as Rect[],
  rust: [[256, 0, 64, 256], [320, 0, 64, 256]] as Rect[],
  sill: [[384, 0, 64, 256]] as Rect[],
  soot: [[448, 0, 64, 256]] as Rect[],
  band: [[0, 256, 512, 64]] as Rect[],
  chips: [[0, 320, 512, 64]] as Rect[],
  damp: [[0, 384, 128, 128]] as Rect[],
  patch: [[128, 384, 128, 128]] as Rect[],
  crack: [[256, 384, 128, 128]] as Rect[],
  peel: [[384, 384, 128, 128]] as Rect[],
  // under a parapet or a coping, tileable along it: dark at the top, a ragged lower edge
  coping: [[0, 512, 512, 64]] as Rect[],
  // on the ground at the foot of a wall, tileable along it: the wall's side at the top
  drift: [[0, 576, 512, 64]] as Rect[],
  // long runs of water, a few drips together, no band at the top (the coping's is under them)
  runs: [[0, 640, 64, 256], [64, 640, 64, 256], [128, 640, 64, 256]] as Rect[],
  // the whole wall's grime, tileable along it: darker towards the foot, mottled
  grime: [[192, 640, 256, 128], [192, 768, 256, 128]] as Rect[],
  // an outside corner's grime: dark at the picture's left edge (flipped for the other end)
  edge: [[448, 640, 64, 256]] as Rect[],
  // the soft shade under something standing on the ground
  contact: [[0, 896, 128, 128]] as Rect[],
  // hands at a door's latch side, the grime round a frame
  smudge: [[128, 896, 64, 128]] as Rect[],
};
export type Stain = keyof typeof REG;

let grime: THREE.CanvasTexture | null = null;
function grimeAtlas() {
  if (grime) return grime;
  const c = document.createElement('canvas'); c.width = AW; c.height = AH;
  const g = c.getContext('2d')!;
  const r = rng(1986);
  const rgba = (rgb: [number, number, number], a: number) => `rgba(${rgb[0]},${rgb[1]},${rgb[2]},${a.toFixed(3)})`;
  // the rust is an ochre brown: multiplied over the sodium-lit stucco a redder one turns crimson
  const STAIN: [number, number, number] = [44, 34, 25], RUST: [number, number, number] = [140, 84, 40], CORE: [number, number, number] = [92, 56, 28], DIRT: [number, number, number] = [86, 62, 42];
  const inRegion = (rect: Rect, draw: () => void) => { g.save(); g.beginPath(); g.rect(...rect); g.clip(); draw(); g.restore(); };
  // fade a tall region in at its left and right edges, so a stripe has no hard sides
  const sides = (rect: Rect, k = 0.22) => {
    const [x, y, w, h] = rect;
    g.save(); g.globalCompositeOperation = 'destination-in';
    const gr = g.createLinearGradient(x, 0, x + w, 0);
    gr.addColorStop(0, 'rgba(0,0,0,0)'); gr.addColorStop(k, 'rgba(0,0,0,1)'); gr.addColorStop(1 - k, 'rgba(0,0,0,1)'); gr.addColorStop(1, 'rgba(0,0,0,0)');
    g.fillStyle = gr; g.fillRect(x, y, w, h); g.restore();
  };
  // a drip: a vertical run from y0, `len` long, fading out, sometimes with a bead at its end
  const drip = (x: number, y0: number, len: number, w: number, col: [number, number, number], a: number) => {
    const gr = g.createLinearGradient(0, y0, 0, y0 + len);
    gr.addColorStop(0, rgba(col, a)); gr.addColorStop(0.7, rgba(col, a * 0.55)); gr.addColorStop(1, rgba(col, 0));
    g.fillStyle = gr; g.fillRect(x - w / 2, y0, w, len);
  };
  // a soft round blot (rx by ry), wrapped round a tileable region's sides when `wrap` is its width
  const blot = (x: number, y: number, rx: number, ry: number, col: [number, number, number], a: number, wrap = 0) => {
    for (const o of wrap ? [-wrap, 0, wrap] : [0]) {
      g.save(); g.translate(x + o, y); g.scale(rx, ry);
      const b = g.createRadialGradient(0, 0, 0, 0, 0, 1);
      b.addColorStop(0, rgba(col, a)); b.addColorStop(1, rgba(col, 0));
      g.fillStyle = b; g.fillRect(-1, -1, 2, 2); g.restore();
    }
  };
  // water stains under a roof edge or a coping: a dark band along the top, runs below it
  REG.streak.forEach((rect, v) => inRegion(rect, () => {
    const [x, y, w, h] = rect;
    const top = g.createLinearGradient(0, y, 0, y + 64 + v * 10);
    top.addColorStop(0, rgba(STAIN, 0.72)); top.addColorStop(0.4, rgba(STAIN, 0.3)); top.addColorStop(1, rgba(STAIN, 0));
    g.fillStyle = top; g.fillRect(x, y, w, 70 + v * 10);
    for (let i = 0; i < 10 + v * 2; i++) drip(x + 6 + r() * (w - 12), y + r() * 18, 50 + r() * (h - 60) * (0.4 + 0.6 * r()), 3 + r() * 8, STAIN, 0.2 + r() * 0.26);
    for (let i = 0; i < 3; i++) drip(x + 10 + r() * (w - 20), y, 120 + r() * 120, 10 + r() * 14, STAIN, 0.14);
    sides(rect);
  }));
  // rust bleeding from a bracket or a bolt: a small dark core, then a fan of thin brown runs
  // spreading as they go down, so it reads as a stain under the fixture and not as a line
  REG.rust.forEach((rect, v) => inRegion(rect, () => {
    const [x, y, w, h] = rect, cx = x + w / 2;
    blot(cx, y + 10, 9, 8, CORE, 0.55);
    g.beginPath(); g.moveTo(cx - 7, y + 8); g.lineTo(cx + 7, y + 8); g.lineTo(x + w - 3, y + h); g.lineTo(x + 3, y + h); g.closePath();
    const fan = g.createLinearGradient(0, y + 8, 0, y + h);
    fan.addColorStop(0, rgba(RUST, 0.26)); fan.addColorStop(0.45, rgba(RUST, 0.1)); fan.addColorStop(1, rgba(RUST, 0));
    g.fillStyle = fan; g.fill();
    for (let i = 0; i < 9 + v * 3; i++) {
      const dx = (r() - 0.5) * 2, len = (60 + r() * (h - 70)) * (1 - Math.abs(dx) * 0.55);
      drip(cx + dx * (w / 2 - 6) * (0.4 + 0.6 * r()), y + 6 + r() * 12, len, 1.5 + r() * 4, i % 3 ? RUST : CORE, 0.14 + r() * 0.22);
    }
    sides(rect, 0.12);
  }));
  // under a window sill: a short dark run the width of the sill
  REG.sill.forEach((rect) => inRegion(rect, () => {
    const [x, y, w, h] = rect;
    const top = g.createLinearGradient(0, y, 0, y + h);
    top.addColorStop(0, rgba(STAIN, 0.62)); top.addColorStop(0.35, rgba(STAIN, 0.26)); top.addColorStop(1, rgba(STAIN, 0));
    g.fillStyle = top; g.fillRect(x, y, w, h);
    for (let i = 0; i < 12; i++) drip(x + 4 + r() * (w - 8), y + 4, 40 + r() * (h - 50), 1.5 + r() * 4, STAIN, 0.15 + r() * 0.2);
    sides(rect, 0.12);
  }));
  // soot over an exhaust or a kitchen vent: dark at the foot, spreading up
  REG.soot.forEach((rect) => inRegion(rect, () => {
    const [x, y, w, h] = rect, cx = x + w / 2;
    for (let i = 0; i < 26; i++) {
      const k = i / 26, rad = 6 + k * 34, cy = y + h - 12 - k * (h - 30);
      const b = g.createRadialGradient(cx + (r() - 0.5) * k * 20, cy, 1, cx, cy, rad);
      b.addColorStop(0, rgba([18, 16, 14], 0.16 * (1 - k * 0.8))); b.addColorStop(1, rgba([18, 16, 14], 0));
      g.fillStyle = b; g.fillRect(x, y, w, h);
    }
  }));
  // the dirt splashed up the foot of a wall, tileable along it: a dark edge, the splash above
  {
    const [x, y, w, h] = REG.band[0];
    inRegion(REG.band[0], () => {
      const gr = g.createLinearGradient(0, y + h, 0, y);
      gr.addColorStop(0, rgba(DIRT, 0.72)); gr.addColorStop(0.3, rgba(DIRT, 0.42)); gr.addColorStop(0.75, rgba(DIRT, 0.08)); gr.addColorStop(1, rgba(DIRT, 0));
      g.fillStyle = gr; g.fillRect(x, y, w, h);
      // a ragged top made of whole periods, so the band tiles
      for (let px = 0; px < w; px += 2) {
        const t = px / w * Math.PI * 2, top = h * (0.4 + 0.12 * Math.sin(t * 3) + 0.08 * Math.sin(t * 7 + 1.3) + 0.05 * Math.sin(t * 17 + 2.1));
        g.fillStyle = rgba(DIRT, 0.14); g.fillRect(x + px, y + h - top, 2, top);
      }
      for (let i = 0; i < 900; i++) {
        const sx = r() * w, sy = h - Math.pow(r(), 1.8) * h, s = 0.6 + r() * 1.8;
        g.fillStyle = rgba(r() < 0.8 ? DIRT : [60, 44, 30], 0.18 + r() * 0.35);
        for (const o of [-w, 0, w]) g.fillRect(x + sx + o, y + sy, s, s);
      }
    });
  }
  // paint chipped off at the foot of a wall, and the pale undercoat and dark render under it
  {
    const [x, y, w, h] = REG.chips[0];
    inRegion(REG.chips[0], () => {
      for (let i = 0; i < 300; i++) {
        const sx = r() * w, sy = h - Math.pow(r(), 2.2) * h, s = 0.8 + r() * 3.2, pale = r() < 0.3;
        g.fillStyle = pale ? rgba([214, 204, 184], 0.12 + r() * 0.2) : rgba([58, 46, 34], 0.25 + r() * 0.35);
        for (const o of [-w, 0, w]) { g.beginPath(); g.ellipse(x + sx + o, y + sy, s * (0.6 + r()), s * 0.6, r() * 3, 0, Math.PI * 2); g.fill(); }
      }
    });
  }
  // a damp blotch with its tide mark
  inRegion(REG.damp[0], () => {
    const [x, y, w] = REG.damp[0], cx = x + w / 2;
    for (let i = 0; i < 18; i++) {
      const rad = 10 + r() * 22, ox = (r() - 0.5) * 60, oy = 30 + r() * 70;
      g.save(); g.translate(cx + ox, y + oy); g.scale(0.7, 1.6);
      const b = g.createRadialGradient(0, 0, rad * 0.2, 0, 0, rad);
      b.addColorStop(0, rgba(STAIN, 0.04)); b.addColorStop(0.8, rgba(STAIN, 0.08)); b.addColorStop(1, rgba(STAIN, 0));
      g.fillStyle = b; g.fillRect(-rad, -rad, rad * 2, rad * 2); g.restore();
    }
  });
  // a trowelled patch of newer render: a rough oblong a little greyer and cooler than the wall,
  // its edge standing proud (a dark line along the bottom and one side), the float's arcs in it.
  // Low contrast: a paler one read as a second lamp on a floodlit wall.
  inRegion(REG.patch[0], () => {
    const [x, y, w, h] = REG.patch[0];
    const pts: [number, number][] = [[x + 14 + r() * 8, y + 22 + r() * 10], [x + w - 16 - r() * 10, y + 16 + r() * 10], [x + w - 12 - r() * 8, y + h - 20 - r() * 10], [x + 18 + r() * 10, y + h - 14 - r() * 8]];
    const rim: [number, number][] = [];
    pts.forEach(([px, py], i) => {
      const [qx, qy] = pts[(i + 1) % 4];
      for (let s = 0; s < 1; s += 0.125) rim.push([px + (qx - px) * s + (r() - 0.5) * 3, py + (qy - py) * s + (r() - 0.5) * 3]);
    });
    const outline = () => { g.beginPath(); rim.forEach(([px, py]) => g.lineTo(px, py)); g.closePath(); };
    outline(); g.fillStyle = rgba([118, 118, 114], 0.3); g.fill();
    g.strokeStyle = rgba([40, 34, 28], 0.22); g.lineWidth = 2; g.stroke();
    g.save(); outline(); g.clip();
    for (let i = 0; i < 9; i++) {
      const sx = x + 20 + r() * (w - 40), sy = y + 24 + r() * (h - 48);
      g.strokeStyle = r() < 0.5 ? rgba([150, 150, 144], 0.1) : rgba([60, 56, 50], 0.1); g.lineWidth = 2 + r() * 3;
      g.beginPath(); g.arc(sx, sy, 12 + r() * 14, 3.4 + r() * 0.4, 5.4 + r() * 0.6); g.stroke();
    }
    g.restore();
  });
  // cracks running out from a corner
  inRegion(REG.crack[0], () => {
    const [x, y] = REG.crack[0];
    g.strokeStyle = rgba([30, 24, 18], 0.5); g.lineCap = 'round';
    const branch = (px: number, py: number, a: number, len: number, lw: number) => {
      if (len < 6 || lw < 0.4) return;
      g.lineWidth = lw; g.beginPath(); g.moveTo(px, py);
      let qx = px, qy = py;
      for (let s = 0; s < len; s += 6) { a += (r() - 0.5) * 0.35; qx += Math.cos(a) * 6; qy += Math.sin(a) * 6; g.lineTo(qx, qy); }
      g.stroke();
      branch(qx, qy, a + (r() - 0.5) * 1.4, len * 0.55, lw * 0.7);
      if (r() < 0.6) branch(qx, qy, a + (r() < 0.5 ? 0.8 : -0.8), len * 0.4, lw * 0.6);
    };
    branch(x + 10, y + 8, 1.0, 90, 1.6); branch(x + 40, y + 6, 1.4, 60, 1.2);
  });
  // render flaking off: a few clusters of small flakes, the dark scratch coat in each, the
  // faintest rim (a bright one read as stickers)
  inRegion(REG.peel[0], () => {
    const [x, y, w, h] = REG.peel[0];
    for (let c = 0; c < 4; c++) {
      const cx = x + 22 + r() * (w - 44), cy = y + 22 + r() * (h - 44);
      for (let f = 0; f < 9; f++) {
        const fx = cx + (r() - 0.5) * 34, fy = cy + (r() - 0.5) * 26, rr = 2 + r() * (c ? 4.5 : 7), pts: [number, number][] = [];
        for (let i = 0; i < 7; i++) { const a = i / 7 * Math.PI * 2 + r() * 0.5, q = rr * (0.55 + r() * 0.6); pts.push([fx + Math.cos(a) * q * 1.3, fy + Math.sin(a) * q]); }
        g.beginPath(); pts.forEach(([px, py]) => g.lineTo(px, py)); g.closePath();
        g.fillStyle = rgba([84, 70, 54], 0.28 + r() * 0.18); g.fill();
        g.strokeStyle = rgba([228, 218, 198], 0.12); g.lineWidth = 1.2; g.stroke();
      }
    }
  });
  // under the coping: a soft dark band, a ragged lower edge of whole periods (so it tiles), the
  // first few centimetres of the runs in it
  {
    const [x, y, w, h] = REG.coping[0];
    inRegion(REG.coping[0], () => {
      for (let px = 0; px < w; px += 2) {
        const t = px / w * Math.PI * 2, d = h * (0.45 + 0.14 * Math.sin(t * 2 + 0.4) + 0.1 * Math.sin(t * 5 + 2.2) + 0.06 * Math.sin(t * 11 + 1.1));
        const gr = g.createLinearGradient(0, y, 0, y + d);
        gr.addColorStop(0, rgba(STAIN, 0.62)); gr.addColorStop(0.35, rgba(STAIN, 0.3)); gr.addColorStop(1, rgba(STAIN, 0));
        g.fillStyle = gr; g.fillRect(x + px, y, 2, d);
      }
      for (let i = 0; i < 40; i++) {
        const sx = r() * w, len = 10 + r() * 44, ww = 2 + r() * 6, a = 0.08 + r() * 0.16;
        for (const o of [-w, 0, w]) drip(x + sx + o, y + 4, len, ww, STAIN, a);
      }
    });
  }
  // the dirt and the sand drifted against the foot of a wall: dark where it meets the wall,
  // thinning out over a ragged edge, grains in it (tileable along the wall)
  {
    const [x, y, w, h] = REG.drift[0];
    inRegion(REG.drift[0], () => {
      for (let px = 0; px < w; px += 2) {
        const t = px / w * Math.PI * 2, d = h * (0.72 + 0.14 * Math.sin(t * 3 + 0.7) + 0.08 * Math.sin(t * 8 + 2.9) + 0.05 * Math.sin(t * 19));
        // (darker than the wall stains: the ground round the buildings is dark already, and a
        // drift no darker than it does not show)
        const gr = g.createLinearGradient(0, y, 0, y + d);
        gr.addColorStop(0, rgba([22, 16, 11], 0.8)); gr.addColorStop(0.45, rgba([34, 25, 17], 0.5)); gr.addColorStop(1, rgba([34, 25, 17], 0));
        g.fillStyle = gr; g.fillRect(x + px, y, 2, d);
      }
      for (let i = 0; i < 1400; i++) {
        const sx = r() * w, sy = Math.pow(r(), 1.7) * h, s = 0.6 + r() * 2.2, pale = r() < 0.2;
        g.fillStyle = pale ? rgba([150, 130, 100], 0.06 + r() * 0.12) : rgba(r() < 0.6 ? [30, 22, 15] : [46, 34, 24], 0.2 + r() * 0.35);
        for (const o of [-w, 0, w]) g.fillRect(x + sx + o, y + sy, s, s);
      }
    });
  }
  // long runs: a few drips from the top, of different lengths, a faint wash round them
  REG.runs.forEach((rect, v) => inRegion(rect, () => {
    const [x, y, w, h] = rect;
    drip(x + w / 2 + (r() - 0.5) * 10, y, h * (0.6 + r() * 0.35), 22 + r() * 12, STAIN, 0.1);
    for (let i = 0; i < 3 + v; i++) drip(x + 10 + r() * (w - 20), y + r() * 14, h * (0.3 + r() * 0.65), 3 + r() * 7, STAIN, 0.2 + r() * 0.2);
    for (let i = 0; i < 4; i++) drip(x + 8 + r() * (w - 16), y + r() * 30, h * (0.1 + r() * 0.25), 1.5 + r() * 3, STAIN, 0.18);
    sides(rect, 0.18);
  }));
  // the whole wall's grime: darker to the foot, mottled in big soft blots; tileable along it
  REG.grime.forEach((rect, v) => inRegion(rect, () => {
    const [x, y, w, h] = rect;
    const gr = g.createLinearGradient(0, y + h, 0, y);
    gr.addColorStop(0, rgba(STAIN, 0.56)); gr.addColorStop(0.3, rgba(STAIN, 0.3)); gr.addColorStop(0.7, rgba(STAIN, 0.12)); gr.addColorStop(1, rgba(STAIN, 0.06));
    g.fillStyle = gr; g.fillRect(x, y, w, h);
    for (let i = 0; i < 52 + v * 10; i++) {
      const low = r() < 0.55, by = low ? h - Math.pow(r(), 1.5) * h * 0.55 : r() * h;
      blot(x + r() * w, y + by, 12 + r() * 34, 8 + r() * 18, STAIN, 0.05 + r() * (low ? 0.14 : 0.09), w);
    }
    // the faint washes the rain has left down it
    for (let i = 0; i < 9; i++) { const sx = r() * w, len = h * (0.3 + r() * 0.6); for (const o of [-w, 0, w]) drip(x + sx + o, y, len, 6 + r() * 14, STAIN, 0.05 + r() * 0.06); }
  }));
  // an outside corner: dark at the left, out to nothing over the picture's width, most low down
  inRegion(REG.edge[0], () => {
    const [x, y, w, h] = REG.edge[0];
    g.save(); g.beginPath(); g.rect(x, y, w, h); g.clip();
    const gr = g.createLinearGradient(x, 0, x + w, 0);
    gr.addColorStop(0, rgba(STAIN, 0.42)); gr.addColorStop(0.35, rgba(STAIN, 0.16)); gr.addColorStop(1, rgba(STAIN, 0));
    g.fillStyle = gr; g.fillRect(x, y, w, h);
    g.globalCompositeOperation = 'destination-in';
    const fade = g.createLinearGradient(0, y, 0, y + h);
    fade.addColorStop(0, 'rgba(0,0,0,0.45)'); fade.addColorStop(0.6, 'rgba(0,0,0,0.8)'); fade.addColorStop(1, 'rgba(0,0,0,1)');
    g.fillStyle = fade; g.fillRect(x, y, w, h);
    g.restore();
    for (let i = 0; i < 14; i++) blot(x + Math.pow(r(), 2) * w * 0.6, y + r() * h, 6 + r() * 12, 10 + r() * 24, STAIN, 0.06 + r() * 0.08);
  });
  // the shade under a car or a drum: a soft dark oval
  {
    const [x, y, w, h] = REG.contact[0];
    const b = g.createRadialGradient(x + w / 2, y + h / 2, 0, x + w / 2, y + h / 2, w / 2 - 2);
    b.addColorStop(0, 'rgba(14,11,8,0.9)'); b.addColorStop(0.45, 'rgba(14,11,8,0.72)'); b.addColorStop(0.75, 'rgba(14,11,8,0.3)'); b.addColorStop(1, 'rgba(14,11,8,0)');
    g.fillStyle = b; g.fillRect(x, y, w, h);
  }
  // hands and weather at a door's frame: a soft tall smudge
  inRegion(REG.smudge[0], () => {
    const [x, y, w, h] = REG.smudge[0];
    for (let i = 0; i < 16; i++) blot(x + w / 2 + (r() - 0.5) * 16, y + 20 + r() * (h - 40), 8 + r() * 10, 12 + r() * 20, STAIN, 0.07 + r() * 0.07);
  });
  grime = new THREE.CanvasTexture(c);
  grime.colorSpace = THREE.SRGBColorSpace; grime.anisotropy = 4;
  return grime;
}

// ---------- the light pools: a soft disc, a soft panel, and the words on a drinks machine ----------
const LW = 256, LH = 64;
const LREG = { disc: [0, 0, 64, 64] as Rect, panel: [64, 0, 64, 64] as Rect, drinks: [128, 0, 128, 32] as Rect, ice: [128, 32, 128, 32] as Rect };
export type Pool = keyof typeof LREG;
let lightTex: THREE.CanvasTexture | null = null;
function poolAtlas() {
  if (lightTex) return lightTex;
  const c = document.createElement('canvas'); c.width = LW; c.height = LH;
  const g = c.getContext('2d')!;
  const d = g.createRadialGradient(32, 32, 0, 32, 32, 31);
  d.addColorStop(0, 'rgba(255,255,255,1)'); d.addColorStop(0.35, 'rgba(255,255,255,.45)'); d.addColorStop(0.7, 'rgba(255,255,255,.12)'); d.addColorStop(1, 'rgba(255,255,255,0)');
  g.fillStyle = d; g.fillRect(0, 0, 64, 64);
  // the panel: bright inside, soft for a few pixels at its edges
  for (let i = 0; i < 8; i++) { g.fillStyle = `rgba(255,255,255,${(0.14 + i * 0.03).toFixed(2)})`; g.fillRect(64 + i, i, 64 - i * 2, 64 - i * 2); }
  g.fillStyle = '#fff'; g.textAlign = 'center'; g.textBaseline = 'middle';
  g.font = '600 22px Oswald, sans-serif'; g.fillText('COLD DRINKS', 192, 16, 120);
  g.font = '600 24px Oswald, sans-serif'; g.fillText('ICE', 192, 48, 120);
  lightTex = new THREE.CanvasTexture(c);
  lightTex.colorSpace = THREE.SRGBColorSpace;
  return lightTex;
}

// ---------- the palette the props are painted from ----------
// Dirtied: a lot's cars and cans have stood in the dust for years, and a clean white is the
// brightest thing in a sodium-lit scene (the light falls on it as it falls on the walls).
const PAL = {
  rust: 0x7a3a22, rustDark: 0x4f2a1a, drumBlue: 0x2f4f6e, drumOlive: 0x56603a, drumRed: 0x8a2a1e, galv: 0x7f817c, galvDark: 0x60625e,
  rubber: 0x1c1b1a, tyre: 0x262524, woodNew: 0x8c6e4a, woodOld: 0x6a5642, pallet: 0x7c6650, cardboard: 0x97795a, tarp: 0x2e5a7a,
  beige: 0x9b8b6a, tan: 0x7c6a4c, brown: 0x5a3a24, paleBlue: 0x6c808c, white: 0xaea898, green: 0x2e4436, maroon: 0x6a2a26, gold: 0x8a6c38, teal: 0x3e6a66,
  chrome: 0x8f9292, glass: 0x10161c, tail: 0x7a1410, lens: 0xb2aa90, cooler: 0x8c8676, louvre: 0x4e4b44, hose: 0x2f5a2a,
  sodaRed: 0x9a2420, sodaWhite: 0xc2bcae, newsBlue: 0x2a4a7a, bench: 0x3a5a40, canGreen: 0x2c3a2e, black: 0x111213,
  pipe: 0x5e615f, conduit: 0x777974, cream: 0xb4ab96, trim: 0x4a3a2c, yellow: 0xa88426, concrete: 0x86817a, grey: 0x3a3c3e,
  brick: 0x7a4632, bulb: 0xf2e2c0, sand: 0xa8937a, steel: 0x6c7377, red: 0xa8382c, orange: 0xb8661e,
  // the desert's dust on a car's lower body and bumpers; the dark of its wheel wells
  dust: 0x76695a, well: 0x0e0d0c,
  // the dry brush at the foot of the walls (the scrub's colour, Exterior.ts) and its grass
  brush: 0x4b4a2f, brushDry: 0x6a6242, grassDry: 0x8c7a50,
  // a venetian blind seen against a lit room; the motel's green trim
  blind: 0x3a2e24, trimGreen: 0x3c5249,
} as const;
export type Paint = keyof typeof PAL;
const PAL_KEYS = Object.keys(PAL) as Paint[];
const CELLS = 8, CELL = 8;   // an 8 x 8 grid of 8-pixel swatches
let palTex: THREE.CanvasTexture | null = null;
function palette() {
  if (palTex) return palTex;
  const c = document.createElement('canvas'); c.width = c.height = CELLS * CELL;
  const g = c.getContext('2d')!;
  PAL_KEYS.forEach((k, i) => { g.fillStyle = '#' + PAL[k].toString(16).padStart(6, '0'); g.fillRect((i % CELLS) * CELL, Math.floor(i / CELLS) * CELL, CELL, CELL); });
  palTex = new THREE.CanvasTexture(c);
  palTex.colorSpace = THREE.SRGBColorSpace;
  // every part samples the middle of one swatch: no filtering between them, no mipmaps to bleed
  palTex.magFilter = palTex.minFilter = THREE.NearestFilter; palTex.generateMipmaps = false;
  return palTex;
}
function swatch(k: Paint): [number, number] {
  const i = PAL_KEYS.indexOf(k);
  return [((i % CELLS) + 0.5) / CELLS, 1 - (Math.floor(i / CELLS) + 0.5) / CELLS];
}
/** Paint a part: every vertex on the middle of one swatch. */
export function paint<T extends THREE.Mesh>(m: T, k: Paint): T {
  const uv = m.geometry.attributes.uv as THREE.BufferAttribute, [u, v] = swatch(k);
  for (let i = 0; i < uv.count; i++) uv.setXY(i, u, v);
  return m;
}

/** Leave a group out on Low (core/quality.ts reads it on every switch). */
export function highOnly<T extends THREE.Object3D>(o: T): T { o.userData.highOnly = true; return o; }

const V = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z);
const _up = new THREE.Vector3(0, 1, 0);
const NORMAL: Record<Face, THREE.Vector3> = { '+x': V(1, 0, 0), '-x': V(-1, 0, 0), '+z': V(0, 0, 1), '-z': V(0, 0, -1) };
// the way to the viewer's right, looking at the face
const RIGHT: Record<Face, THREE.Vector3> = { '+x': V(0, 0, -1), '-x': V(0, 0, 1), '+z': V(1, 0, 0), '-z': V(-1, 0, 0) };

/** How a wall is worn (Dressing.wall); the amounts are multiples of the usual. */
export type Wear = {
  openings?: Opening[];
  /** the dirt splashed up the foot and the paint chipped there: their heights */
  band?: number; chips?: number;
  /** the long runs down from the top edge (how many), the dark band under the coping (its height) */
  streaks?: number; coping?: number;
  /** along the wall, where water comes down for certain: scuppers, coping joints, roof units */
  drips?: number[];
  /** damp, trowelled patches and cracks (how many), and how many of them peel */
  patches?: number; peel?: number;
  /** the whole wall's grime, darker to its foot and its corners (its strength) */
  grime?: number;
  /** on the ground: how far the dirt drifts out from the foot (0 on a ledge too narrow for it)
   *  and the tufts of dry brush against it (per 10 m) */
  drift?: number; tufts?: number;
  /** how far out from the wall the brush starts (beyond a footing's pad) */
  tuftOut?: number;
  /** the ground at the foot (the dressing's own, or y0) */
  ground?: number | ((x: number, z: number) => number);
  /** where nothing may lie on the ground: a path, a step */
  clear?: (x: number, z: number) => boolean;
  k?: number; tint?: THREE.ColorRepresentation;
};
type Buf = { pos: number[]; uv: number[]; col: number[]; nrm?: number[] };

/** One area's dressing: build into it, then build() once and add `group` to the area. */
export class Dressing {
  readonly group = new THREE.Group();
  /** The big props: kept on Low. */
  readonly core = new THREE.Group();
  /** The small props: High only. */
  readonly fine = new THREE.Group();
  readonly mat: THREE.MeshStandardMaterial;
  private readonly stainMat: THREE.MeshStandardMaterial;
  private readonly poolMat: THREE.MeshBasicMaterial;
  private readonly d: Buf = { pos: [], nrm: [], uv: [], col: [] };
  private readonly p: Buf = { pos: [], uv: [], col: [] };
  private readonly r: () => number;
  /** The land under the props and the walls where it is not flat (the area's), or null. */
  ground: ((x: number, z: number) => number) | null = null;
  /** For tests: how much went in. */
  readonly stats = { stains: 0, pools: 0, props: 0 };

  /** fall: the flood falloff of the walls the stains lie on; propFall the props' own (the
   *  walls they stand against, where those take the lamps more steeply). */
  constructor(set: FloodSet, fall: number, seed = 1, propFall = fall) {
    this.r = rng(seed * 7919 + 13);
    // the props carry the shade they stand in as vertex colours (build(): darker to their feet)
    this.mat = floodlit(new THREE.MeshStandardMaterial({ map: palette(), vertexColors: true, roughness: 0.8, metalness: 0.05 }), propFall, set);
    this.mat.name = `dress/${set.key}/props`;
    // the stains take the wall's light: lit by the same lamps as what they lie on
    this.stainMat = floodlit(new THREE.MeshStandardMaterial({ map: grimeAtlas(), vertexColors: true, transparent: true, depthWrite: false, roughness: 1,
      polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }), fall, set);
    this.stainMat.name = `dress/${set.key}/stains`;
    this.poolMat = new THREE.MeshBasicMaterial({ map: poolAtlas(), vertexColors: true, transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, fog: false,
      polygonOffset: true, polygonOffsetFactor: -3, polygonOffsetUnits: -6 });
    this.poolMat.name = `dress/${set.key}/pools`;
    this.group.add(this.core, highOnly(this.fine));
    noMerge(this.group);
  }
  private rand(a: number, b: number) { return a + (b - a) * this.r(); }
  /** Its three materials, for an area that disposes its own (the pictures are shared: never disposed). */
  get materials(): THREE.Material[] { return [this.mat, this.stainMat, this.poolMat]; }

  // ---------- stains ----------
  /** A stain on a wall: centred at (x, y, z) on the face, w wide and h tall; k its strength
   *  (alpha), tint a colour it is multiplied by. */
  stain(kind: Stain, x: number, y: number, z: number, face: Face, w: number, h: number, k = 1, variant = -1, tint: THREE.ColorRepresentation = 0xffffff) {
    const regs = REG[kind], rect = regs[variant >= 0 ? variant % regs.length : Math.floor(this.r() * regs.length)];
    const n = NORMAL[face], rt = RIGHT[face], c = V(x, y, z).addScaledVector(n, 0.012);
    this.quad(this.d, c, rt.clone().multiplyScalar(w / 2), V(0, h / 2, 0), rect, AW, AH, new THREE.Color(tint), k, n);
    this.stats.stains++;
  }
  /** A stain lying on the ground (an oil drip under a car, a damp patch by a downspout); its
   *  corners follow the dressing's ground where it has one. */
  stainGround(kind: Stain, x: number, y: number, z: number, w: number, d: number, k = 1, rot = 0) {
    const regs = REG[kind], rect = regs[Math.floor(this.r() * regs.length)];
    const a = V(Math.cos(rot) * w / 2, 0, -Math.sin(rot) * w / 2), b = V(-Math.sin(rot) * d / 2, 0, -Math.cos(rot) * d / 2);
    const pts = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(([sa, sb]) => {
      const p = V(x, y + 0.012, z).addScaledVector(a, sa).addScaledVector(b, sb);
      if (this.ground) p.y += this.ground(p.x, p.z) - this.ground(x, z);
      return p;
    });
    this.quad4(this.d, pts, rect, [[0, 0], [1, 0], [1, 1], [0, 1]], new THREE.Color(0xffffff), k, _up);
    this.stats.stains++;
  }
  /** The soft shade under something standing on the ground, w across and d along (turned by rot). */
  contact(x: number, y: number, z: number, w: number, d: number, k = 0.7, rot = 0) { this.stainGround('contact', x, y, z, w, d, k, rot); }

  /** The wear of a whole wall: its grime darker to the foot and the corners, the dirt splashed
   *  up the foot and the paint chipped there, a dark band under the top edge and the long runs
   *  where water comes down (scuppers, corners, roof units; bare between), stains under the
   *  sills and nothing over the glass, patches and peeling where render fails (the foot, the
   *  corners, the corners of the openings), and on the ground the drifted dirt and the brush. */
  wall(wl: Wall, o: Wear = {}) {
    const len = wl.a1 - wl.a0, hgt = wl.y1 - wl.y0, k = o.k ?? 1, ops = o.openings ?? [];
    const at = (a: number, y: number): [number, number, number] => wl.face === '+x' || wl.face === '-x' ? [wl.at, y, a] : [a, y, wl.at];
    // the runs of wall between the doors (a door's foot is the wall's: nothing across it)
    const doors = ops.filter((op) => op.y0 <= wl.y0 + 0.05).sort((p, q) => p.a0 - q.a0);
    const runs: [number, number][] = [];
    let from = wl.a0;
    for (const op of doors) { if (op.a0 > from + 0.05) runs.push([from, op.a0]); from = Math.max(from, op.a1); }
    if (wl.a1 > from + 0.05) runs.push([from, wl.a1]);
    // a band tiled along the wall's runs (or its whole length), from y0 up h, round the openings
    const band = (kind: Stain, y0: number, h: number, kk: number, spans = runs, tile = 2.6) => {
      for (const [r0, r1] of spans) {
        const rl = r1 - r0, n = Math.max(1, Math.round(rl / tile)), s = rl / n;
        for (let i = 0; i < n; i++) this.cover(wl, REG[kind][0], r0 + s * i, r0 + s * (i + 1), y0, y0 + h, kk, o.tint, ops);
      }
    };

    // the whole wall's grime, in long tiles, and its outside corners (High: this is what reads
    // from 20 or 40 m away, where the stains are too small to see)
    const gk = (o.grime ?? 0.8) * k;
    if (gk > 0 && hgt > 0.3) {
      const n = Math.max(1, Math.round(len / 5.5)), s = len / n, v = Math.floor(this.r() * REG.grime.length);
      for (let i = 0; i < n; i++) this.cover(wl, REG.grime[(v + i) % REG.grime.length], wl.a0 + s * i, wl.a0 + s * (i + 1), wl.y0, wl.y1, gk, o.tint, ops);
      const cw = Math.min(1.1, len * 0.2), sign = RIGHT[wl.face].x + RIGHT[wl.face].z;
      // the edge picture is dark at its left: the wall's left end as it is seen gets it as drawn
      this.cover(wl, REG.edge[0], wl.a0, wl.a0 + cw, wl.y0, wl.y1, gk, o.tint, ops, sign < 0);
      this.cover(wl, REG.edge[0], wl.a1 - cw, wl.a1, wl.y0, wl.y1, gk, o.tint, ops, sign > 0);
    }
    if ((o.band ?? 0.55) > 0) band('band', wl.y0, o.band ?? 0.55, 0.85 * k);
    if ((o.chips ?? 0.4) > 0) band('chips', wl.y0, o.chips ?? 0.4, 0.8 * k);

    // under the top edge: one band along it, then the long runs where water comes down
    if ((o.streaks ?? 1) > 0) {
      const ch = Math.min(o.coping ?? 0.5, hgt * 0.3);
      if (ch > 0.05) band('coping', wl.y1 - ch, ch, 0.8 * k, [[wl.a0, wl.a1]], 3.2);
      const run = (a: number, w: number, kk: number, long: number) => {
        if (a - w / 2 < wl.a0 || a + w / 2 > wl.a1) return;
        let h = Math.min(hgt - 0.1, hgt * long);
        // water stops at a window's head (its sill stain is below it) and is cut by a door
        for (const op of ops) if (a + w * 0.3 > op.a0 && a - w * 0.3 < op.a1 && op.y1 < wl.y1) h = Math.min(h, wl.y1 - op.y1 - 0.04);
        if (h < 0.3) return;
        this.stain('runs', ...at(a, wl.y1 - h / 2 - 0.01), wl.face, w, h, kk, -1, o.tint);
      };
      for (const a of o.drips ?? []) { run(a, this.rand(0.55, 0.95), this.rand(0.8, 1) * k, this.rand(0.6, 0.9)); run(a + this.rand(-0.35, 0.35), this.rand(0.3, 0.5), 0.6 * k, this.rand(0.4, 0.7)); }
      if (this.r() < 0.75) run(wl.a0 + this.rand(0.3, 0.55), this.rand(0.45, 0.8), this.rand(0.6, 0.95) * k, this.rand(0.45, 0.85));
      if (this.r() < 0.75) run(wl.a1 - this.rand(0.3, 0.55), this.rand(0.45, 0.8), this.rand(0.6, 0.95) * k, this.rand(0.45, 0.85));
      const nRun = Math.round((o.streaks ?? 1) * len / 7);
      for (let i = 0; i < nRun; i++) run(this.rand(wl.a0 + 0.6, wl.a1 - 0.6), this.rand(0.35, 0.7), this.rand(0.4, 0.8) * k, this.rand(0.25, 0.65));
    }
    for (const op of ops) {
      if (op.y0 < wl.y0 + 0.3) continue;   // a door: its sill is the ground
      const h = Math.min(op.y0 - wl.y0 - 0.05, this.rand(0.5, 0.95));
      this.stain('sill', ...at((op.a0 + op.a1) / 2 + this.rand(-0.08, 0.08), op.y0 - h / 2 - 0.02), wl.face, (op.a1 - op.a0) * this.rand(0.75, 1.05), h, 0.85 * k, 0, o.tint);
    }

    // patches and peeling where render fails: at the foot, the outside corners, the corners of
    // the openings, under the sills and the scuppers; never on a wall too short for one
    const nPatch = Math.round((o.patches ?? 1) * len / 5);
    if (hgt >= 1.2) for (let i = 0; i < nPatch; i++) {
      const s = Math.min(this.rand(0.35, 1.0), hgt - 0.15), w = s * 1.2;
      const wins = ops.filter((op) => op.y0 > wl.y0 + 0.3);
      let a: number, y: number;
      const where = this.r();
      if (where < 0.3) { a = this.rand(wl.a0 + w, wl.a1 - w); y = wl.y0 + s / 2 + this.rand(0.04, 0.3); }
      else if (where < 0.55) { a = this.r() < 0.5 ? wl.a0 + w / 2 + this.rand(0.04, 0.3) : wl.a1 - w / 2 - this.rand(0.04, 0.3); y = wl.y0 + this.rand(s / 2 + 0.05, Math.max(s / 2 + 0.1, hgt * 0.7)); }
      else if (where < 0.85 && ops.length) {
        const op = ops[Math.floor(this.r() * ops.length)], left = this.r() < 0.5;
        a = left ? op.a0 - w / 2 - 0.03 : op.a1 + w / 2 + 0.03;
        y = op.y0 > wl.y0 + 0.3 && this.r() < 0.5 ? op.y0 + this.rand(-0.1, 0.2) : Math.min(op.y1 + this.rand(-0.2, 0.1), wl.y1 - s / 2 - 0.1);
      } else if (wins.length) { const op = wins[Math.floor(this.r() * wins.length)]; a = (op.a0 + op.a1) / 2 + this.rand(-0.3, 0.3); y = op.y0 - s / 2 - this.rand(0.08, 0.3); }
      else if (o.drips?.length) { a = o.drips[Math.floor(this.r() * o.drips.length)] + this.rand(-0.4, 0.4); y = wl.y0 + this.rand(s / 2 + 0.1, hgt * 0.5); }
      else continue;
      // the whole of it on the wall, and clear of the glass
      if (a - w / 2 < wl.a0 + 0.02 || a + w / 2 > wl.a1 - 0.02) continue;
      y = Math.min(Math.max(y, wl.y0 + s / 2 + 0.03), wl.y1 - s / 2 - 0.08);
      if (!ops.every((op) => a + w / 2 < op.a0 || a - w / 2 > op.a1 || y + s / 2 < op.y0 || y - s / 2 > op.y1)) continue;
      const kind: Stain = this.r() < (o.peel ?? 0.2) ? 'peel' : (['damp', 'patch', 'damp', 'patch', 'crack'] as Stain[])[Math.floor(this.r() * 5)];
      this.stain(kind, ...at(a, y), wl.face, w, s, this.rand(0.5, 0.95) * k, 0, o.tint);
    }

    // on the ground: the dirt and the sand drifted against the foot, deeper at the corners, and
    // the brush that grows in it (High, the small props); none across a door or on a path
    const gy = o.ground ?? this.ground ?? wl.y0, yAt = (x: number, z: number) => (typeof gy === 'number' ? gy : gy(x, z)) + 0.015;
    const n = NORMAL[wl.face], depth = o.drift ?? 0.45;
    const pt = (a: number, out: number) => { const [x, , z] = at(a, 0), p = V(x + n.x * out, 0, z + n.z * out); p.y = yAt(p.x, p.z); return p; };
    if (depth > 0) for (const [r0, r1] of runs) {
      const rl = r1 - r0, m = Math.max(1, Math.round(rl / 2.6)), s = rl / m;
      for (let i = 0; i < m; i++) {
        const b0 = r0 + s * i, b1 = b0 + s, d0 = depth * (b0 - wl.a0 < 0.8 ? 1.6 : 1), d1 = depth * (wl.a1 - b1 < 0.8 ? 1.6 : 1);
        if (o.clear && (o.clear(...xz(pt(b0, d0))) || o.clear(...xz(pt(b1, d1))))) continue;
        this.groundStrip(wl.face, pt(b0, 0), pt(b1, 0), pt(b1, d1), pt(b0, d0), REG.drift[0], 0.85 * k);
      }
    }
    const nTuft = (o.tufts ?? 0) * len / 10;
    if (nTuft > 0) {
      const spots: number[] = [];
      for (let i = 0; i < nTuft; i++) spots.push(this.rand(wl.a0 + 0.3, wl.a1 - 0.3));
      // the corners gather more of it
      for (const end of [wl.a0, wl.a1]) if (this.r() < 0.75) for (let j = 0; j < 2 + Math.floor(this.r() * 2); j++) spots.push(end + (end === wl.a0 ? 1 : -1) * this.rand(0.1, 0.7));
      // each spot a clump of one to three, the biggest nearest the wall
      for (const s0 of spots) for (let j = 0, m = 1 + Math.floor(this.r() * 2.6); j < m; j++) {
        const a = s0 + this.rand(-0.3, 0.3) * j;
        if (a < wl.a0 - 0.2 || a > wl.a1 + 0.2 || doors.some((op) => a > op.a0 - 0.6 && a < op.a1 + 0.6)) continue;
        const lo = o.tuftOut ?? 0.1, p = pt(a, lo + this.rand(0, 0.15) + j * this.rand(0.1, 0.25));
        if (o.clear?.(p.x, p.z)) continue;
        this.tuft(p.x, p.y - 0.015, p.z, this.rand(0.7, 1.35) * (1 - j * 0.2));
      }
    }
  }
  /** Rust run down a wall from a fixture at (x, y, z): it fans out into a stain below it. */
  rust(x: number, y: number, z: number, face: Face, len = 1.2, w = 0.45, k = 0.75) { this.stain('rust', x, y - len / 2, z, face, w, len, k); }

  /** Lay `rect` over the wall from a0 to a1 and y0 to y1 (the whole picture), in pieces round
   *  the openings so that nothing lies over the glass; `flip` mirrors it. */
  private cover(wl: Wall, rect: Rect, a0: number, a1: number, y0: number, y1: number, k: number, tint: THREE.ColorRepresentation | undefined, ops: Opening[], flip = false) {
    const cuts = [a0, a1, ...ops.flatMap((op) => [op.a0, op.a1]).filter((a) => a > a0 && a < a1)].sort((p, q) => p - q);
    for (let i = 0; i + 1 < cuts.length; i++) {
      const c0 = cuts[i], c1 = cuts[i + 1], mid = (c0 + c1) / 2;
      if (c1 - c0 < 0.01) continue;
      const holes = ops.filter((op) => op.a0 <= mid && op.a1 >= mid && op.y1 > y0 && op.y0 < y1).sort((p, q) => p.y0 - q.y0);
      let y = y0;
      for (const h of holes) { if (h.y0 > y + 0.01) this.wallPiece(wl, rect, a0, a1, y0, y1, c0, c1, y, Math.min(h.y0, y1), k, tint, flip); y = Math.max(y, h.y1); }
      if (y1 > y + 0.01) this.wallPiece(wl, rect, a0, a1, y0, y1, c0, c1, y, y1, k, tint, flip);
    }
  }
  /** The part [b0,b1] x [z0,z1] of a picture laid over the wall from a0 to a1 and y0 to y1. */
  private wallPiece(wl: Wall, rect: Rect, a0: number, a1: number, y0: number, y1: number, b0: number, b1: number, z0: number, z1: number, k: number, tint: THREE.ColorRepresentation | undefined, flip: boolean) {
    const n = NORMAL[wl.face], sign = RIGHT[wl.face].x + RIGHT[wl.face].z;
    const P = (a: number, y: number) => wl.face === '+x' || wl.face === '-x' ? V(wl.at + n.x * 0.012, y, a) : V(a, y, wl.at + n.z * 0.012);
    // the picture's left edge is the viewer's left
    const fu = (a: number) => { let f = (a - a0) / (a1 - a0); if (sign < 0) f = 1 - f; return flip ? 1 - f : f; };
    const fv = (y: number) => (y - y0) / (y1 - y0);
    const [l, r] = sign > 0 ? [b0, b1] : [b1, b0];
    this.quad4(this.d, [P(l, z0), P(r, z0), P(r, z1), P(l, z1)], rect, [[fu(l), fv(z0)], [fu(r), fv(z0)], [fu(r), fv(z1)], [fu(l), fv(z1)]], new THREE.Color(tint ?? 0xffffff), k, n);
    this.stats.stains++;
  }
  /** A strip on the ground along a wall's foot: from a and b at the wall to c and d out from it. */
  private groundStrip(face: Face, a: THREE.Vector3, b: THREE.Vector3, c: THREE.Vector3, d: THREE.Vector3, rect: Rect, k: number) {
    // seen from above with the wall at the top, the face's right is the picture's right
    const [l0, r0, r1, l1] = RIGHT[face].x + RIGHT[face].z > 0 ? [a, b, c, d] : [b, a, d, c];
    this.quad4(this.d, [l1, r1, r0, l0], rect, [[0, 0], [1, 0], [1, 1], [0, 1]], new THREE.Color(0xffffff), k, _up);
    this.stats.stains++;
  }

  // ---------- light pools (additive) ----------
  /** A pool of light on a wall or on the ground: centre, size, colour (its brightness is the
   *  colour's), facing a wall's way or 'up'. */
  pool(x: number, y: number, z: number, face: Face | 'up', w: number, h: number, color: THREE.ColorRepresentation, k = 1, kind: Pool = 'disc') {
    const c = new THREE.Color(color).multiplyScalar(k);
    if (face === 'up') this.quad(this.p, V(x, y + 0.02, z), V(w / 2, 0, 0), V(0, 0, -h / 2), LREG[kind], LW, LH, c, 1);
    else this.quad(this.p, V(x, y, z).addScaledVector(NORMAL[face], 0.02), RIGHT[face].clone().multiplyScalar(w / 2), V(0, h / 2, 0), LREG[kind], LW, LH, c, 1);
    this.stats.pools++;
  }

  private quad(b: Buf, c: THREE.Vector3, a: THREE.Vector3, up: THREE.Vector3, rect: Rect, w: number, h: number, col: THREE.Color, alpha: number, n?: THREE.Vector3) {
    const pts = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(([sa, sb]) => c.clone().addScaledVector(a, sa).addScaledVector(up, sb));
    this.quad4(b, pts, rect, [[0, 0], [1, 0], [1, 1], [0, 1]], col, alpha, n, w, h);
  }
  /** Four corners (anticlockwise, seen from the front), each with its place in `rect` (0..1). */
  private quad4(b: Buf, pts: THREE.Vector3[], rect: Rect, f: number[][], col: THREE.Color, alpha: number, n?: THREE.Vector3, w = AW, h = AH) {
    // half a pixel in from the region's edges, so a neighbour never bleeds into it
    const u = (t: number) => (rect[0] + 0.5 + t * (rect[2] - 1)) / w, v = (t: number) => 1 - (rect[1] + 0.5 + (1 - t) * (rect[3] - 1)) / h;
    pts.forEach((p, i) => {
      b.pos.push(p.x, p.y, p.z);
      b.uv.push(u(f[i][0]), v(f[i][1]));
      if (b.nrm && n) { b.nrm.push(n.x, n.y, n.z); b.col.push(col.r, col.g, col.b, alpha); } else b.col.push(col.r, col.g, col.b);
    });
  }
  private mesh(b: Buf, mat: THREE.Material, rgba: boolean) {
    const g = new THREE.BufferGeometry(), idx: number[] = [];
    for (let i = 0; i < b.pos.length / 3; i += 4) idx.push(i, i + 1, i + 2, i, i + 2, i + 3);
    g.setAttribute('position', new THREE.Float32BufferAttribute(b.pos, 3));
    g.setAttribute('uv', new THREE.Float32BufferAttribute(b.uv, 2));
    g.setAttribute('color', new THREE.Float32BufferAttribute(b.col, rgba ? 4 : 3));
    if (b.nrm) g.setAttribute('normal', new THREE.Float32BufferAttribute(b.nrm, 3));
    g.setIndex(idx);
    const m = new THREE.Mesh(g, mat);
    // Ultra: neither casts (core/ultra.ts: transparent, or unlit); the stains take the shadows
    // of what they lie on, or they would stand out lit in them
    return highOnly(noMerge(m));
  }

  // ---------- parts ----------
  /** A painted box in `g` (the props are built from these and merged). */
  box(g: THREE.Object3D, w: number, h: number, d: number, k: Paint, x: number, y: number, z: number, rx = 0, ry = 0, rz = 0) {
    const m = paint(new THREE.Mesh(new THREE.BoxGeometry(w, h, d), this.mat), k);
    m.position.set(x, y, z); m.rotation.set(rx, ry, rz); g.add(m); return m;
  }
  cyl(g: THREE.Object3D, rt: number, rb: number, h: number, k: Paint, x: number, y: number, z: number, seg = 10, rx = 0, rz = 0) {
    const m = paint(new THREE.Mesh(new THREE.CylinderGeometry(rt, rb, h, seg), this.mat), k);
    m.position.set(x, y, z); m.rotation.set(rx, 0, rz); g.add(m); return m;
  }
  /** A thin rod from a to b (conduit, pipe, an antenna's element). */
  rod(g: THREE.Object3D, a: THREE.Vector3, b: THREE.Vector3, r: number, k: Paint, seg = 5) {
    const d = new THREE.Vector3().subVectors(b, a), len = d.length();
    const m = paint(new THREE.Mesh(new THREE.CylinderGeometry(r, r, len, seg), this.mat), k);
    m.position.copy(a).addScaledVector(d, 0.5); m.quaternion.setFromUnitVectors(_up, d.normalize()); g.add(m); return m;
  }
  torus(g: THREE.Object3D, r: number, t: number, k: Paint, x: number, y: number, z: number, rs = 6, ts = 12) {
    const m = paint(new THREE.Mesh(new THREE.TorusGeometry(r, t, rs, ts), this.mat), k);
    m.position.set(x, y, z); m.rotation.x = Math.PI / 2; g.add(m); return m;
  }
  /** A prop's own group at (x, y, z) turned by yaw, in core or fine. `ao`: it stands on what is
   *  at its y (the ground, a roof), and darkens to its foot (build(); a number: how dark it is
   *  there, 0.5 if true); a fixture on a wall does not. */
  piece(x: number, y: number, z: number, yaw: number, fine: boolean, ao: boolean | number = true) {
    const g = new THREE.Group(); g.position.set(x, y, z); g.rotation.y = yaw; g.userData.ao = ao;
    (fine ? this.fine : this.core).add(g); this.stats.props++; return g;
  }
  /** The ground a prop stands on, w by d round (x, z) turned by yaw. */
  private foot(x: number, z: number, w: number, d: number, yaw: number, pad = 0): Foot {
    const c = Math.abs(Math.cos(yaw)), s = Math.abs(Math.sin(yaw)), hx = (w * c + d * s) / 2 + pad, hz = (w * s + d * c) / 2 + pad;
    return { minX: x - hx, maxX: x + hx, minZ: z - hz, maxZ: z + hz };
  }

  // ---------- props ----------
  /** A 55-gallon drum, standing (or lying on its side with `lying`). */
  drum(x: number, y: number, z: number, k: Paint = 'rust', o: { lying?: boolean; yaw?: number; fine?: boolean; open?: boolean } = {}) {
    const g = this.piece(x, y, z, o.yaw ?? this.rand(0, 6), !!o.fine);
    const s = o.lying ? new THREE.Group() : g;
    if (o.lying) { s.position.y = 0.29; s.rotation.z = Math.PI / 2; s.position.x = 0; g.add(s); }
    const base = o.lying ? -0.44 : 0;
    this.cyl(s, 0.29, 0.29, 0.88, k, 0, base + 0.44, 0, 12);
    for (const yy of [0.3, 0.6]) this.cyl(s, 0.3, 0.3, 0.03, k === 'rust' ? 'rustDark' : 'galvDark', 0, base + yy, 0, 12);
    this.cyl(s, 0.3, 0.3, 0.03, 'rustDark', 0, base + 0.875, 0, 12);
    if (o.open) this.cyl(s, 0.26, 0.26, 0.02, 'black', 0, base + 0.86, 0, 12);
    else this.cyl(s, 0.04, 0.04, 0.03, 'galv', 0.15, base + 0.895, 0.05, 6);
    if (o.lying) this.contact(x, y, z, 0.8, 1.15, 0.7, o.yaw ?? 0); else this.contact(x, y, z, 0.9, 0.9, 0.7);
    return o.lying ? this.foot(x, z, 0.9, 0.6, o.yaw ?? 0) : this.foot(x, z, 0.6, 0.6, 0);
  }
  /** A galvanised trash can with its lid (or the lid off, beside it). */
  trashCan(x: number, y: number, z: number, o: { k?: Paint; lidOff?: boolean; fine?: boolean } = {}) {
    const g = this.piece(x, y, z, this.rand(0, 6), o.fine ?? true), k = o.k ?? 'galv';
    this.cyl(g, 0.27, 0.23, 0.72, k, 0, 0.36, 0, 12);
    for (const yy of [0.18, 0.5]) this.cyl(g, 0.265, 0.255, 0.025, 'galvDark', 0, yy, 0, 12);
    if (o.lidOff) { const l = this.cyl(g, 0.29, 0.29, 0.04, k, 0.42, 0.13, 0.1, 12, 1.2); l.rotation.y = 0.4; }
    else { this.cyl(g, 0.29, 0.28, 0.05, k, 0, 0.745, 0, 12); this.cyl(g, 0.04, 0.04, 0.05, 'galvDark', 0, 0.79, 0, 6); }
    this.contact(x, y, z, 0.85, 0.85, 0.65);
    return this.foot(x, z, 0.6, 0.6, 0);
  }
  /** A wooden crate, its slats round the edges. */
  crate(x: number, y: number, z: number, s = 0.7, o: { yaw?: number; k?: Paint; fine?: boolean } = {}) {
    const yaw = o.yaw ?? this.rand(-0.3, 0.3), g = this.piece(x, y, z, yaw, o.fine ?? true), k = o.k ?? 'woodNew';
    this.box(g, s, s * 0.8, s, k, 0, s * 0.4, 0);
    for (const yy of [0.04, s * 0.76]) for (const dz of [-1, 1]) this.box(g, s + 0.02, 0.07, 0.03, 'woodOld', 0, yy, dz * (s / 2 + 0.005));
    for (const dx of [-1, 1]) this.box(g, 0.03, s * 0.8, 0.07, 'woodOld', dx * (s / 2 + 0.005), s * 0.4, s / 2 - 0.03);
    this.contact(x, y, z, s * 1.5, s * 1.5, 0.6, yaw);
    return this.foot(x, z, s, s, yaw);
  }
  /** A pallet, flat on the ground; a stack of them with `n`; or stood up on its edge with `lean`,
   *  its top leaning back towards the prop's -z (turn it with yaw onto a wall). */
  pallet(x: number, y: number, z: number, n = 1, o: { yaw?: number; fine?: boolean; lean?: boolean } = {}) {
    const yaw = o.yaw ?? this.rand(-0.2, 0.2), g = this.piece(x, y, z, yaw, o.fine ?? true);
    const one = (p: THREE.Object3D, y0: number) => {
      for (let i = 0; i < 7; i++) this.box(p, 1.2, 0.022, 0.1, 'pallet', 0, y0 + 0.133, -0.45 + i * 0.15);
      for (const dx of [-0.55, 0, 0.55]) this.box(p, 0.1, 0.1, 1.0, 'woodOld', dx, y0 + 0.07, 0);
      for (const dz of [-0.45, 0.45]) this.box(p, 1.2, 0.022, 0.1, 'pallet', 0, y0 + 0.011, dz);
    };
    if (o.lean) { const p = new THREE.Group(), q = new THREE.Group(); p.rotation.x = 1.33; q.position.z = -0.5; p.add(q); g.add(p); one(q, 0); }
    else for (let i = 0; i < n; i++) one(g, i * 0.145);
    this.contact(x, y, z, 1.55, o.lean ? 0.7 : 1.35, o.lean ? 0.5 : 0.7, yaw);
    return this.foot(x, z, 1.2, o.lean ? 0.4 : 1.0, yaw);
  }
  /** Old tyres in a stack (and one leaning on it with `lean`). */
  tyres(x: number, y: number, z: number, n = 4, o: { lean?: boolean; fine?: boolean } = {}) {
    const g = this.piece(x, y, z, 0, o.fine ?? true);
    for (let i = 0; i < n; i++) this.torus(g, 0.29, 0.11, 'tyre', this.rand(-0.04, 0.04), 0.11 + i * 0.205, this.rand(-0.04, 0.04), 6, 14);
    if (o.lean) { const t = this.torus(g, 0.29, 0.11, 'tyre', 0.55, 0.39, 0.05, 6, 14); t.rotation.set(0.2, 1.4, 0); }
    this.contact(x + (o.lean ? 0.25 : 0), y, z, o.lean ? 1.6 : 1.1, 1.1, 0.7);
    return this.foot(x, z, o.lean ? 1.3 : 0.84, 0.84, 0);
  }
  /** A garden hose coiled on the ground (or on a hook on the wall with `wall`). */
  hose(x: number, y: number, z: number, o: { wall?: Face; fine?: boolean } = {}) {
    const g = this.piece(x, y, z, 0, o.fine ?? true, !o.wall);
    for (let i = 0; i < 4; i++) {
      const t = this.torus(g, 0.22 - i * 0.012, 0.018, 'hose', 0, 0.02 + i * 0.03, 0, 4, 16);
      if (o.wall) { t.rotation.set(0, o.wall === '+x' || o.wall === '-x' ? Math.PI / 2 : 0, 0); t.position.set(0, 0.22 - i * 0.01, 0); }
    }
    if (!o.wall) this.contact(x, y, z, 0.6, 0.6, 0.35);
    return this.foot(x, z, 0.5, 0.5, 0);
  }
  /** A newspaper box on its legs, the window in its door. */
  newsBox(x: number, y: number, z: number, yaw: number, k: Paint = 'newsBlue') {
    const g = this.piece(x, y, z, yaw, true);
    for (const dx of [-0.17, 0.17]) for (const dz of [-0.15, 0.15]) this.box(g, 0.03, 0.42, 0.03, 'grey', dx, 0.21, dz);
    this.box(g, 0.44, 0.62, 0.4, k, 0, 0.73, 0);
    this.box(g, 0.34, 0.28, 0.01, 'glass', 0, 0.82, 0.205);
    this.box(g, 0.36, 0.04, 0.02, 'chrome', 0, 0.62, 0.21);
    this.box(g, 0.46, 0.04, 0.42, 'grey', 0, 1.06, 0);
    this.contact(x, y, z, 0.75, 0.7, 0.5, yaw);
    return this.foot(x, z, 0.46, 0.42, yaw);
  }
  /** A bench: slats on two iron ends. */
  bench(x: number, y: number, z: number, yaw: number, len = 1.8, k: Paint = 'bench') {
    const g = this.piece(x, y, z, yaw, false);
    for (const dx of [-len / 2 + 0.15, len / 2 - 0.15]) { this.box(g, 0.05, 0.44, 0.42, 'black', dx, 0.22, 0); this.box(g, 0.05, 0.42, 0.06, 'black', dx, 0.65, -0.2, -0.12); }
    for (let i = 0; i < 3; i++) this.box(g, len, 0.035, 0.11, k, 0, 0.45, -0.14 + i * 0.13);
    for (let i = 0; i < 2; i++) this.box(g, len, 0.1, 0.03, k, 0, 0.62 + i * 0.16, -0.22, -0.12);
    this.contact(x, y, z, len + 0.3, 0.75, 0.5, yaw);
    return this.foot(x, z, len, 0.5, yaw);
  }
  /** A drinks machine: its lit front is a pool panel (call with the area's light pools on). */
  sodaMachine(x: number, y: number, z: number, yaw: number) {
    const g = this.piece(x, y, z, yaw, false);
    this.box(g, 0.92, 1.8, 0.78, 'sodaRed', 0, 0.9, 0);
    this.box(g, 0.94, 0.08, 0.8, 'grey', 0, 1.84, 0);
    this.box(g, 0.6, 1.18, 0.01, 'sodaWhite', -0.08, 1.12, 0.395);
    this.box(g, 0.14, 0.5, 0.02, 'chrome', 0.32, 1.12, 0.4);
    this.box(g, 0.5, 0.14, 0.04, 'black', -0.08, 0.3, 0.4);
    const f = V(Math.sin(yaw), 0, Math.cos(yaw)), rt = V(Math.cos(yaw), 0, -Math.sin(yaw));
    // the panel's light and the words over it, a little in front of the face, then its spill on the ground
    const c = V(x, y + 1.12, z).addScaledVector(f, 0.41).addScaledVector(rt, -0.08);
    this.quad(this.p, c, rt.clone().multiplyScalar(0.3), V(0, 0.59, 0), LREG.panel, LW, LH, new THREE.Color(0xe8eeff).multiplyScalar(0.4), 1);
    const t = V(x, y + 1.62, z).addScaledVector(f, 0.405);
    this.quad(this.p, t, rt.clone().multiplyScalar(0.36), V(0, 0.09, 0), LREG.drinks, LW, LH, new THREE.Color(0xfff6ea).multiplyScalar(0.7), 1);
    const gp = V(x, y + 0.02, z).addScaledVector(f, 1.0);
    this.quad(this.p, gp, rt.clone().multiplyScalar(1.1), f.clone().multiplyScalar(-0.9), LREG.disc, LW, LH, new THREE.Color(0xfff0d8).multiplyScalar(0.22), 1);
    this.stats.pools += 3;
    this.contact(x, y, z, 1.25, 1.1, 0.7, yaw);
    return this.foot(x, z, 0.94, 0.8, yaw);
  }
  /** An evaporative (swamp) cooler on a roof: the cabinet with its louvred pads, the stand, and
   *  the duct turning down into the roof. `face`: the way its duct side faces. */
  swampCooler(x: number, y: number, z: number, yaw: number, s = 1) {
    const g = this.piece(x, y, z, yaw, false);
    g.scale.setScalar(s);
    for (const dx of [-0.42, 0.42]) for (const dz of [-0.42, 0.42]) this.box(g, 0.05, 0.3, 0.05, 'galvDark', dx, 0.15, dz);
    this.box(g, 0.98, 0.9, 0.98, 'cooler', 0, 0.75, 0);
    this.box(g, 1.04, 0.08, 1.04, 'galvDark', 0, 1.24, 0, 0, 0, 0.04);
    // louvres on three sides
    for (const [ax, az] of [[1, 0], [-1, 0], [0, -1]]) for (let i = 0; i < 6; i++) {
      const yy = 0.42 + i * 0.12;
      if (ax) this.box(g, 0.03, 0.04, 0.86, 'louvre', ax * 0.5, yy, 0, 0, 0, ax * 0.5);
      else this.box(g, 0.86, 0.04, 0.03, 'louvre', 0, yy, az * 0.5, az * -0.5);
    }
    // the duct out of the fourth side and down
    this.box(g, 0.56, 0.5, 0.5, 'galv', 0, 0.75, 0.74);
    this.box(g, 0.56, 0.62, 0.5, 'galv', 0, 0.28, 1.0);
    this.box(g, 0.66, 0.05, 0.6, 'galvDark', 0, 0.02, 1.0);
    return g;
  }
  /** A vent stack through a roof with its cap (or a turbine vent with `turbine`). */
  vent(x: number, y: number, z: number, o: { h?: number; r?: number; turbine?: boolean; k?: Paint } = {}) {
    const g = this.piece(x, y, z, 0, false), h = o.h ?? 0.6, r = o.r ?? 0.06, k = o.k ?? 'pipe';
    this.cyl(g, r, r * 1.2, h, k, 0, h / 2, 0, 8);
    this.cyl(g, r * 1.6, r * 1.6, 0.03, 'galvDark', 0, 0.02, 0, 8);
    if (o.turbine) { const t = this.cyl(g, r * 2.6, r * 2.2, r * 3.2, 'galv', 0, h + r * 1.6, 0, 10); t.scale.y = 1; this.cyl(g, r * 1.2, r * 2.6, r * 1.2, 'galvDark', 0, h + r * 3.6, 0, 10); }
    else this.cyl(g, r * 2.2, r * 1.4, r * 1.3, 'galvDark', 0, h + r * 0.9, 0, 8);
    return g;
  }
  /** A television antenna on its mast: a boom with elements, guyed three ways. */
  antenna(x: number, y: number, z: number, h = 3.2, yaw = 0) {
    const g = this.piece(x, y, z, yaw, false);
    this.cyl(g, 0.025, 0.03, h, 'galv', 0, h / 2, 0, 6);
    this.box(g, 0.025, 0.025, 1.8, 'galv', 0, h - 0.12, 0.1);
    for (let i = 0; i < 7; i++) { const w = 1.3 - i * 0.12; this.box(g, w, 0.015, 0.015, 'galv', 0, h - 0.12, -0.7 + i * 0.27); }
    this.box(g, 0.9, 0.015, 0.015, 'galv', 0, h - 0.42, 0.25, 0, 0, 0);
    for (let i = 0; i < 3; i++) { const a = i * 2.094 + 0.5; this.rod(g, V(0, h * 0.7, 0), V(Math.cos(a) * 1.3, 0.02, Math.sin(a) * 1.3), 0.005, 'galvDark', 3); }
    return g;
  }
  /** A meter on the wall (face: which way it looks) with the conduit up to the roof line. */
  meter(x: number, y: number, z: number, face: Face, up = 1.6, fine = false) {
    const yaw = face === '+z' ? 0 : face === '-z' ? Math.PI : face === '+x' ? Math.PI / 2 : -Math.PI / 2;
    const g = this.piece(x, y, z, yaw, fine, false);
    this.box(g, 0.3, 0.42, 0.13, 'galvDark', 0, 0, 0.065);
    this.cyl(g, 0.1, 0.1, 0.07, 'glass', 0, 0.05, 0.16, 12, Math.PI / 2);
    this.cyl(g, 0.105, 0.105, 0.02, 'chrome', 0, 0.05, 0.135, 12, Math.PI / 2);
    this.cyl(g, 0.025, 0.025, up, 'conduit', 0, 0.21 + up / 2, 0.06, 6);
    this.cyl(g, 0.025, 0.025, 0.5, 'conduit', 0, -0.46, 0.06, 6);
    this.box(g, 0.36, 0.5, 0.16, 'galv', 0.42, -0.06, 0.08);   // the disconnect beside it
    this.box(g, 0.03, 0.12, 0.04, 'black', 0.6, 0.02, 0.15);
    return g;
  }
  /** Conduit (or a pipe) along a wall from a to b, with straps every metre or so. */
  conduit(a: THREE.Vector3, b: THREE.Vector3, k: Paint = 'conduit', r = 0.022, fine = true) {
    const g = this.piece(0, 0, 0, 0, fine, false);
    this.rod(g, a, b, r, k, 6);
    const n = Math.floor(a.distanceTo(b) / 1.1), dir = b.clone().sub(a).normalize();
    for (let i = 1; i <= n; i++) { const c = a.clone().lerp(b, i / (n + 1)); this.rod(g, c.clone().addScaledVector(dir, -0.015), c.clone().addScaledVector(dir, 0.015), r * 1.7, 'galvDark', 6); }
    return g;
  }
  /** A downspout from a scupper at the roof edge to the ground, with its elbow and splash block
   *  (`reach` out from the wall: less where a plinth's edge is close). */
  downspout(x: number, yTop: number, yGround: number, z: number, face: Face, k: Paint = 'galv', reach = 0.45) {
    const n = NORMAL[face], g = this.piece(x, yGround, z, 0, false);
    const o = (d: number) => [n.x * d, n.z * d];
    const top = yTop - yGround;
    const [sx, sz] = o(0.12);
    this.box(g, Math.abs(n.x) ? 0.24 : 0.3, 0.2, Math.abs(n.z) ? 0.24 : 0.3, k, sx, top, sz);
    const [px, pz] = o(0.1);
    this.box(g, 0.09, top - 0.3, 0.09, k, px, (top + 0.3) / 2 - 0.1, pz);
    const [ex, ez] = o(Math.min(0.25, reach * 0.6));
    this.box(g, Math.abs(n.x) ? 0.3 : 0.09, 0.09, Math.abs(n.z) ? 0.3 : 0.09, k, ex, 0.18, ez);
    const [bx, bz] = o(reach);
    this.box(g, Math.abs(n.x) ? 0.34 : 0.26, 0.05, Math.abs(n.z) ? 0.34 : 0.26, 'concrete', bx, 0.025, bz);
    this.stain('damp', x + n.x * 0.1, yGround + 0.35, z + n.z * 0.1, face, 0.7, 0.6, 0.7);
    // the splash's wet and the dirt it has washed out round the block (not on a narrow ledge)
    if (reach >= 0.4) this.stainGround('contact', x + n.x * (reach + 0.1), yGround, z + n.z * (reach + 0.1), 0.9, 0.9, 0.45);
    return g;
  }
  /** A gutter along an eave from a to b (y the eave's), its downspouts by the caller. */
  gutter(a: THREE.Vector3, b: THREE.Vector3, k: Paint = 'galv') {
    const g = this.piece(0, 0, 0, 0, false, false);
    this.rod(g, a, b, 0.055, k, 6);
    return g;
  }
  /** A porch light on a wall: the jelly-jar fixture (its glow is the area's, `glow` takes the
   *  bulb's point), the warm wash on the wall round it and the pool on the ground below. */
  porchLight(x: number, y: number, z: number, face: Face, yGround: number, glow?: (x: number, y: number, z: number) => void, k = 1, color: THREE.ColorRepresentation = 0xffc884) {
    const n = NORMAL[face], g = this.piece(x, y, z, 0, false, false);
    this.box(g, Math.abs(n.x) ? 0.04 : 0.14, 0.18, Math.abs(n.z) ? 0.04 : 0.14, 'black', n.x * 0.02, 0, n.z * 0.02);
    this.cyl(g, 0.06, 0.06, 0.14, 'bulb', n.x * 0.1, -0.06, n.z * 0.1, 8);
    this.cyl(g, 0.07, 0.07, 0.03, 'black', n.x * 0.1, 0.03, n.z * 0.1, 8);
    glow?.(x + n.x * 0.1, y - 0.06, z + n.z * 0.1);
    this.pool(x, y - 0.25, z, face, 2.6, 2.4, color, 0.32 * k);
    this.pool(x + n.x * 1.2, yGround, z + n.z * 1.2, 'up', 3.0, 3.0, color, 0.2 * k);
  }
  /** A tuft of dry grass round a low clump (High: the small props); lit as the brush is, not
   *  shaded to its foot as a solid prop is. */
  tuft(x: number, y: number, z: number, s = 1) {
    const g = this.piece(x, y, z, this.rand(0, 6), true, 0.72);
    const c = paint(new THREE.Mesh(new THREE.IcosahedronGeometry(0.14 * s, 0), this.mat), 'brushDry');
    c.scale.set(1, 0.5 + this.r() * 0.3, 0.8 + this.r() * 0.4); c.position.y = 0.02 * s; g.add(c);
    // the blades: each a thin triangle both ways round, leaning out
    const pos: number[] = [], nrm: number[] = [], n = 12 + Math.floor(this.r() * 8);
    for (let i = 0; i < n; i++) {
      const a = i / n * Math.PI * 2 + this.rand(-0.4, 0.4), len = s * this.rand(0.22, 0.55), lean = s * this.rand(0.08, 0.32), bw = 0.035 * s;
      const ca = Math.cos(a), sa = Math.sin(a), r0 = this.rand(0, 0.1) * s, bx = ca * r0, bz = sa * r0;
      const p0 = [bx + sa * bw, 0, bz - ca * bw], p1 = [bx - sa * bw, 0, bz + ca * bw], tip = [bx + ca * lean, len, bz + sa * lean];
      const nn = V(ca * 0.4, 1, sa * 0.4).normalize();
      pos.push(...p0, ...p1, ...tip, ...p1, ...p0, ...tip);
      for (let j = 0; j < 6; j++) nrm.push(nn.x, nn.y, nn.z);
    }
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    geo.setAttribute('normal', new THREE.Float32BufferAttribute(nrm, 3));
    geo.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(pos.length / 3 * 2), 2));
    g.add(paint(new THREE.Mesh(geo, this.mat), this.r() < 0.65 ? 'grassDry' : 'brushDry'));
    this.contact(x, y, z, 0.55 * s, 0.55 * s, 0.4);
    return g;
  }
  /** Depth for a window painted into its wall (the wing's facade picture, the motel's rooms): a
   *  sill standing out under it, a thin frame round the sides and the head, and with `blind` a
   *  venetian blind let down part way behind the lit glass (High). */
  window(wl: Wall, op: Opening, o: { sill?: Paint; frame?: Paint; blind?: number; depth?: number; sillOut?: number } = {}) {
    const n = NORMAL[wl.face], fd = o.depth ?? 0.022, so = o.sillOut ?? 0.05, w = op.a1 - op.a0, ac = (op.a0 + op.a1) / 2, ax = wl.face === '+x' || wl.face === '-x';
    const put = (g: THREE.Object3D, along: number, h: number, out: number, k: Paint, a: number, y: number, off = 0) => {
      const c = (ax ? V(wl.at, y, a) : V(a, y, wl.at)).addScaledVector(n, out / 2 + off);
      this.box(g, ax ? out : along, h, ax ? along : out, k, c.x, c.y, c.z);
    };
    if (o.sill !== undefined || o.frame !== undefined) {
      if (o.sill) put(this.core, w + 0.12, 0.05, so, o.sill, ac, op.y0 - 0.025);
      const f = o.frame ?? 'galvDark';
      for (const a of [op.a0 - 0.0225, op.a1 + 0.0225]) put(this.core, 0.045, op.y1 - op.y0 + 0.045, fd, f, a, (op.y0 + op.y1) / 2 + 0.0225);
      put(this.core, w + 0.09, 0.045, fd, f, ac, op.y1 + 0.0225);
    }
    if (o.blind) {
      const drop = Math.min(o.blind, op.y1 - op.y0 - 0.05), m = Math.max(2, Math.round(drop / 0.07));
      for (let i = 0; i < m; i++) put(this.fine, w - 0.02, 0.018, 0.005, 'blind', ac, op.y1 - 0.03 - i * (drop / m), 0.002);
      put(this.fine, w - 0.02, 0.03, 0.008, 'blind', ac, op.y1 - 0.03 - drop, 0.002);
    }
  }

  // ---------- cars ----------
  /** A full-size 1970s sedan (or a station wagon with `wagon`), no badges. The nose looks
   *  along +z turned by yaw. */
  sedan(x: number, y: number, z: number, yaw: number, k: Paint = 'beige', o: { wagon?: boolean; dusty?: boolean } = {}) {
    const g = this.piece(x, y, z, yaw, false);
    this.box(g, 1.94, 0.56, 5.3, k, 0, 0.62, 0);                       // the body
    this.box(g, 1.95, 0.16, 5.31, 'dust', 0, 0.42, 0);                  // the dust on the rockers and the valances
    this.box(g, 1.96, 0.04, 5.32, 'chrome', 0, 0.66, 0);                // the side trim
    const bumper: Paint = o.dusty ? 'dust' : 'chrome';
    this.box(g, 1.98, 0.14, 0.12, bumper, 0, 0.48, 2.7);                // the bumpers
    this.box(g, 1.98, 0.14, 0.12, bumper, 0, 0.48, -2.7);
    const L = o.wagon ? 3.0 : 2.3, zc = o.wagon ? -0.55 : -0.15;
    this.box(g, 1.72, 0.5, L, k, 0, 1.15, zc);                          // the roof's pillars
    this.box(g, 1.75, 0.38, L - 0.25, 'glass', 0, 1.13, zc);            // the windows all round
    this.box(g, 1.74, 0.07, L - 0.1, k, 0, 1.43, zc);                   // the roof
    this.box(g, 1.5, 0.2, 0.05, 'glass', 0, 0.68, 2.66);                // the grille
    for (const dx of [-0.66, 0.66]) {
      this.box(g, 0.3, 0.16, 0.04, 'lens', dx, 0.68, 2.665);
      this.box(g, 0.34, 0.14, 0.04, 'tail', dx, 0.7, -2.665);
    }
    // the wheel wells, dark round the tyres, then the wheels
    for (const wz of [1.6, -1.6]) this.cyl(g, 0.42, 0.42, 1.965, 'well', 0, 0.34, wz, 10, 0, Math.PI / 2);
    for (const [wx, wz] of [[-0.88, 1.6], [0.88, 1.6], [-0.88, -1.6], [0.88, -1.6]]) {
      this.cyl(g, 0.34, 0.34, 0.24, 'rubber', wx, 0.34, wz, 10, 0, Math.PI / 2);
      this.cyl(g, 0.17, 0.17, 0.25, 'chrome', wx, 0.34, wz, 8, 0, Math.PI / 2);
    }
    this.contact(x, y, z, 2.3, 5.8, 0.8, yaw);
    this.stainGround('damp', x, y, z, 1.6, 2.2, 0.55, yaw);
    return this.foot(x, z, 2.0, 5.5, yaw);
  }
  /** A pickup of about 1978, the bed open (a toolbox in it with `tools`). */
  pickup(x: number, y: number, z: number, yaw: number, k: Paint = 'white', o: { tools?: boolean; two?: Paint } = {}) {
    const g = this.piece(x, y, z, yaw, false), two = o.two ?? k;
    this.box(g, 1.98, 0.62, 2.2, k, 0, 0.78, 1.45);                     // the hood and the front
    this.box(g, 1.96, 0.62, 1.1, k, 0, 0.78, -0.17);                    // the doors under the cab
    this.box(g, 1.9, 0.5, 1.45, two, 0, 1.33, 0.05);                    // the cab
    this.box(g, 1.92, 0.36, 1.2, 'glass', 0, 1.36, 0.05);
    this.box(g, 1.92, 0.06, 1.3, two, 0, 1.6, 0.05);
    this.box(g, 1.98, 0.62, 0.1, k, 0, 0.78, -0.72);                    // the bed: its front, floor, sides, gate
    this.box(g, 1.8, 0.06, 2.3, 'black', 0, 0.55, -1.9);
    for (const dx of [-0.95, 0.95]) this.box(g, 0.08, 0.6, 2.4, k, dx, 0.82, -1.92);
    this.box(g, 1.98, 0.5, 0.08, k, 0, 0.78, -3.1);
    this.box(g, 2.0, 0.16, 5.74, 'dust', 0, 0.55, -0.26);               // the dust along its lower body
    this.box(g, 2.0, 0.16, 0.12, 'dust', 0, 0.48, 2.6);                 // the bumpers
    this.box(g, 2.0, 0.16, 0.12, 'dust', 0, 0.48, -3.18);
    this.box(g, 1.5, 0.24, 0.05, 'chrome', 0, 0.82, 2.56);
    for (const dx of [-0.7, 0.7]) { this.box(g, 0.26, 0.18, 0.04, 'lens', dx, 0.82, 2.585); this.box(g, 0.16, 0.26, 0.04, 'tail', dx * 1.3, 0.85, -3.16); }
    if (o.tools) this.box(g, 1.75, 0.38, 0.5, 'galv', 0, 1.15, -0.95);
    for (const wz of [1.75, -1.85]) this.cyl(g, 0.47, 0.47, 2.005, 'well', 0, 0.38, wz, 10, 0, Math.PI / 2);
    for (const [wx, wz] of [[-0.9, 1.75], [0.9, 1.75], [-0.9, -1.85], [0.9, -1.85]]) {
      this.cyl(g, 0.38, 0.38, 0.27, 'rubber', wx, 0.38, wz, 10, 0, Math.PI / 2);
      this.cyl(g, 0.17, 0.17, 0.28, 'galv', wx, 0.38, wz, 8, 0, Math.PI / 2);
    }
    this.contact(x, y, z, 2.3, 6.1, 0.8, yaw);
    this.stainGround('damp', x, y, z, 1.6, 2.2, 0.5, yaw);
    return this.foot(x, z, 2.05, 5.9, yaw);
  }

  /** Merge the props (with their shade baked in) and make the stain and pool meshes; add `group`
   *  to the area after. */
  build() {
    this.bake(this.core); this.bake(this.fine);
    if (this.d.pos.length) this.group.add(this.mesh(this.d, this.stainMat, true));
    if (this.p.pos.length) this.group.add(this.mesh(this.p, this.poolMat, false));
    return this.group;
  }
  /** One mesh for everything under `root` (all of it the props' material), each vertex coloured
   *  by the shade it stands in: about half at the prop's foot, full from 1.2 m up (a car's
   *  rockers and wheel wells, a drum's foot, the base of a cooler on its roof). */
  private bake(root: THREE.Group) {
    root.updateMatrixWorld(true);
    const inv = root.matrixWorld.clone().invert(), geos: THREE.BufferGeometry[] = [], parts: THREE.Mesh[] = [];
    root.traverse((o) => { if ((o as THREE.Mesh).isMesh) parts.push(o as THREE.Mesh); });
    const w = new THREE.Vector3(), base = new THREE.Vector3();
    for (const m of parts) {
      let p: THREE.Object3D | null = m;
      while (p && p !== root && p.userData.ao === undefined) p = p.parent;
      const ao = !!p && p !== root && p.userData.ao !== false, foot = ao ? (p!.userData.ao === true ? 0.5 : p!.userData.ao as number) : 1;
      if (ao) base.setFromMatrixPosition(p!.matrixWorld);
      const g = m.geometry.index ? m.geometry.toNonIndexed() : m.geometry.clone();
      for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'uv'].includes(k)) g.deleteAttribute(k);
      const pos = g.attributes.position, col = new Float32Array(pos.count * 3).fill(1);
      if (ao) for (let i = 0; i < pos.count; i++) {
        w.fromBufferAttribute(pos, i).applyMatrix4(m.matrixWorld);
        col.fill(foot + (1 - foot) * THREE.MathUtils.clamp((w.y - base.y) / 1.2, 0, 1), i * 3, i * 3 + 3);
      }
      g.setAttribute('color', new THREE.BufferAttribute(col, 3));
      g.applyMatrix4(new THREE.Matrix4().multiplyMatrices(inv, m.matrixWorld));
      geos.push(g); m.geometry.dispose();
    }
    root.clear();
    if (!geos.length) return;
    const merged = new THREE.Mesh(mergeGeometries(geos, false), this.mat);
    merged.matrixAutoUpdate = false;
    root.add(merged);
  }
}
const xz = (p: THREE.Vector3): [number, number] => [p.x, p.z];
