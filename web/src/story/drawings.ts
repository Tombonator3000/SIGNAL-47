// Drawings that belong to documents in chapter two, made on a canvas and handed to the
// page as images. They follow the paper test in Docs/Menu14 (archive-reference.svg):
// A is the fixed survey point (an outlined triangle with a short bar under it), B the
// comparison vane, C the closing sight line. The amended copy simply has no C. Nothing
// is circled or marked: the change is found by comparing the two.

const PAPER = '#efe8d6', INK = '#22372e';

function sheet(w: number, h: number, draw: (g: CanvasRenderingContext2D) => void) {
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  const g = c.getContext('2d')!;
  g.fillStyle = PAPER; g.fillRect(0, 0, w, h);
  // a little age on the paper
  let s = w * 31 + h;
  const r = () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
  for (let i = 0; i < 900; i++) { g.fillStyle = `rgba(90,70,40,${r() * 0.05})`; g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2); }
  g.strokeStyle = INK; g.fillStyle = INK; g.lineWidth = 3; g.lineCap = 'round'; g.lineJoin = 'round';
  draw(g);
  return c.toDataURL('image/png');
}

// The fixed-point mark. `bar` false is the plain elevation symbol that does not match.
export function mark(g: CanvasRenderingContext2D, cx: number, cy: number, size: number, bar = true) {
  g.beginPath();
  g.moveTo(cx, cy - size * 0.58); g.lineTo(cx - size * 0.55, cy + size * 0.4); g.lineTo(cx + size * 0.55, cy + size * 0.4); g.closePath();
  g.stroke();
  if (bar) { g.beginPath(); g.moveTo(cx - size * 0.4, cy + size * 0.62); g.lineTo(cx + size * 0.4, cy + size * 0.62); g.stroke(); }
}

// ARRANGEMENT drawing attached to the 1947 record. full = original (A, B, C).
export function arrangement(full: boolean) {
  return sheet(960, 420, (g) => {
    g.font = '600 26px Oswald, sans-serif';
    g.fillText(full ? 'ARRANGEMENT / STATION 01 / 1947' : 'ARRANGEMENT / STATION 01 / 1947 / SERVICE COPY', 40, 56);
    g.font = '19px "Special Elite", serif';
    g.fillText(full ? 'A: fixed survey point     B: comparison vane     C: closing sight line' : 'A: fixed survey point     B: comparison vane', 40, 92);
    g.globalAlpha = 0.5; g.lineWidth = 1.5;
    g.beginPath(); g.moveTo(40, 112); g.lineTo(920, 112); g.stroke();
    g.globalAlpha = 1; g.lineWidth = 3;
    mark(g, 150, 250, 64);
    g.beginPath(); g.moveTo(196, 250); g.lineTo(470, 250); g.stroke();      // A to B
    g.beginPath(); g.moveTo(500, 218); g.lineTo(500, 282); g.stroke();      // B, the vane
    if (full) {
      g.beginPath(); g.moveTo(506, 250); g.lineTo(790, 250); g.stroke();    // the closing sight line
      g.beginPath(); g.arc(820, 250, 28, 0, Math.PI * 2); g.stroke();       // C
    }
    g.font = '28px "Special Elite", serif';
    g.fillText('A', 140, 340); g.fillText('B', 490, 340); if (full) g.fillText('C', 810, 340);
    g.font = '17px "Special Elite", serif';
    g.fillText('N. VEGA', 790, 396);
  });
}

// The index diagram from the archive sleeve: the matching fixed-point mark.
export function sleeveMark() {
  return sheet(420, 420, (g) => {
    g.font = '600 24px Oswald, sans-serif';
    g.fillText('FIXED-POINT SHEET', 34, 54);
    g.font = '18px "Special Elite", serif';
    g.fillText('INDEX DIAGRAM', 34, 84);
    g.lineWidth = 4;
    mark(g, 210, 220, 120);
    g.font = '24px "Special Elite", serif';
    g.fillText('A  -  B  -  C', 132, 340);
    g.font = '18px "Special Elite", serif';
    g.fillText('STATION 01 / B-12', 128, 376);
  });
}

// Small icons for the destination choices.
export function markIcon(kind: 'triangle-bar' | 'triangle' | 'three-bars') {
  return sheet(120, 120, (g) => {
    g.lineWidth = 4;
    if (kind === 'three-bars') { for (let i = 0; i < 3; i++) { g.beginPath(); g.moveTo(26, 38 + i * 22); g.lineTo(94, 38 + i * 22); g.stroke(); } }
    else mark(g, 60, 58, 70, kind === 'triangle-bar');
  });
}

// The case map: SARO first, the old survey station once P05 is recorded, the motel once
// its number has been called. Dashed lines are travel, not walkable ground. No scale.
export function caseMap(o: { station: boolean; motel: boolean }) {
  return sheet(900, 640, (g) => {
    g.font = '600 26px Oswald, sans-serif';
    g.fillText('CASE MAP / SARO AND FIELD SITES', 40, 54);
    g.font = '17px "Special Elite", serif';
    g.fillText('Sketch from the site register. Not to scale.', 40, 84);
    // north arrow
    g.lineWidth = 3;
    g.beginPath(); g.moveTo(830, 150); g.lineTo(830, 100); g.lineTo(820, 118); g.moveTo(830, 100); g.lineTo(840, 118); g.stroke();
    g.font = '22px "Special Elite", serif'; g.fillText('N', 822, 176);
    // SARO: the array and the buildings
    const sx = 520, sy = 330;
    for (let i = 0; i < 9; i++) { g.beginPath(); g.arc(sx - 90 + (i % 3) * 45, sy - 120 + Math.floor(i / 3) * 34, 9, 0, Math.PI * 2); g.stroke(); }
    g.strokeRect(sx - 70, sy, 70, 46);
    g.strokeRect(sx + 20, sy - 14, 46, 40);
    g.font = '600 22px Oswald, sans-serif'; g.fillText('SARO', sx - 70, sy + 82);
    g.font = '16px "Special Elite", serif'; g.fillText('control room, yard, records', sx - 70, sy + 106);
    g.setLineDash([12, 10]); g.lineWidth = 2.5;
    if (o.station) {
      const tx = 170, ty = 470;
      g.beginPath(); g.moveTo(sx - 80, sy + 40); g.bezierCurveTo(400, 420, 300, 470, tx + 40, ty); g.stroke();
      g.setLineDash([]); g.lineWidth = 3;
      mark(g, tx, ty, 46);
      g.font = '600 20px Oswald, sans-serif'; g.fillText('OLD SURVEY STATION', tx - 70, ty + 62);
      g.font = '16px "Special Elite", serif'; g.fillText('STATION 01 / 1947', tx - 70, ty + 86);
      g.setLineDash([12, 10]); g.lineWidth = 2.5;
    }
    if (o.motel) {
      const mx = 160, my = 220;
      g.beginPath(); g.moveTo(sx - 80, sy - 10); g.bezierCurveTo(400, 260, 300, 220, mx + 70, my); g.stroke();
      g.setLineDash([]); g.lineWidth = 3;
      g.strokeRect(mx - 50, my - 22, 110, 40);
      g.font = '600 20px Oswald, sans-serif'; g.fillText('SIERRA MOTOR COURT', mx - 60, my - 40);
      g.font = '16px "Special Elite", serif'; g.fillText('key holder: N. Vega', mx - 50, my + 46);
    }
    g.setLineDash([]);
  });
}
