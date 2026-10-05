// Drawings that belong to documents in chapter two, made on a canvas and handed to the
// page as images. They follow the paper test in Docs/Menu14 (archive-reference.svg):
// A is the fixed survey point (an outlined triangle with a short bar under it), B the
// comparison vane, C the closing sight line. The amended copy simply has no C. Nothing
// is circled or marked: the change is found by comparing the two.

const PAPER = '#efe8d6', INK = '#22372e';

// The paper, with a little age on it.
function paperBg(g: CanvasRenderingContext2D, w: number, h: number) {
  g.fillStyle = PAPER; g.fillRect(0, 0, w, h);
  let s = w * 31 + h;
  const r = () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
  for (let i = 0; i < 900; i++) { g.fillStyle = `rgba(90,70,40,${r() * 0.05})`; g.fillRect(r() * w, r() * h, 1 + r() * 2, 1 + r() * 2); }
}

function sheet(w: number, h: number, draw: (g: CanvasRenderingContext2D) => void) {
  const c = document.createElement('canvas');
  c.width = w; c.height = h;
  const g = c.getContext('2d')!;
  paperBg(g, w, h);
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

// E09A, the field transit record (chapter three): a plan of the 1947 set-up at STATION 01,
// north up. Points are in metres from the transit (x east, z south), taken from the built
// station so the paper and the ground agree. A sits in its footing on the transit sight
// line; B is the comparison marker to the west; C, the closing line, runs off the sheet.
export function transitRecord(o: { a: [number, number]; b: [number, number]; c: [number, number] }) {
  return sheet(960, 620, (g) => {
    g.font = '600 26px Oswald, sans-serif';
    g.fillText('FIELD TRANSIT / STATION 01 / E09A', 40, 56);
    g.font = '19px "Special Elite", serif';
    g.fillText('Plan, 1947. Fixed points set in concrete footings.', 40, 92);
    g.globalAlpha = 0.5; g.lineWidth = 1.5;
    g.beginPath(); g.moveTo(40, 112); g.lineTo(920, 112); g.stroke();
    g.globalAlpha = 1; g.lineWidth = 3;
    // fit A and B (C is a direction) into the sheet around the transit
    const reach = Math.max(4, ...[o.a, o.b].map(([x, z]) => Math.hypot(x, z)));
    const k = 200 / reach, cx = 480, cy = 360;
    const P = ([x, z]: [number, number]) => [cx + x * k, cy + z * k] as const;
    const [ax, ay] = P(o.a), [bx, by] = P(o.b);
    // transit: a circle with cross hairs on the pier
    g.beginPath(); g.arc(cx, cy, 16, 0, Math.PI * 2); g.stroke();
    g.beginPath(); g.moveTo(cx - 24, cy); g.lineTo(cx + 24, cy); g.moveTo(cx, cy - 24); g.lineTo(cx, cy + 24); g.stroke();
    // the transit sight line through A, dashed past it
    g.setLineDash([10, 8]); g.lineWidth = 2;
    const dl = Math.hypot(ax - cx, ay - cy) || 1;
    g.beginPath(); g.moveTo(cx, cy); g.lineTo(cx + (ax - cx) / dl * (dl + 70), cy + (ay - cy) / dl * (dl + 70)); g.stroke();
    // C: the closing sight line, off the sheet
    const cl = Math.hypot(o.c[0], o.c[1]) || 1;
    const ex = cx + o.c[0] / cl * 400, ey = cy + o.c[1] / cl * 400;
    g.setLineDash([]); g.lineWidth = 3;
    g.save(); g.beginPath(); g.rect(40, 120, 880, 470); g.clip();
    g.beginPath(); g.moveTo(cx, cy); g.lineTo(ex, ey); g.stroke();
    g.restore();
    // B, the comparison marker
    g.beginPath(); g.moveTo(cx, cy); g.lineTo(bx, by); g.stroke();
    g.beginPath(); g.moveTo(bx, by - 30); g.lineTo(bx, by + 30); g.stroke();
    // A in its square footing
    g.lineWidth = 2; g.strokeRect(ax - 34, ay - 34, 68, 68); g.lineWidth = 3;
    mark(g, ax, ay - 2, 40);
    g.font = '30px "Special Elite", serif';
    g.fillText('A', ax + 42, ay + 10); g.fillText('B', bx - 12, by + 62);
    const lx = Math.min(890, Math.max(60, cx + o.c[0] / cl * 300)), ly = Math.min(570, Math.max(150, cy + o.c[1] / cl * 300));
    g.fillText('C', lx + 6, ly - 14);
    g.font = '17px "Special Elite", serif';
    g.fillText('TRANSIT', cx + 26, cy + 40);
    g.fillText('footing', ax - 34, ay + 56);
    // north arrow
    g.beginPath(); g.moveTo(880, 210); g.lineTo(880, 150); g.lineTo(870, 170); g.moveTo(880, 150); g.lineTo(890, 170); g.stroke();
    g.font = '22px "Special Elite", serif'; g.fillText('N', 872, 236);
    g.font = '17px "Special Elite", serif';
    g.fillText('FIELD COPY / KEEP WITH THE TRANSIT', 40, 600);
  });
}

// ---------- All Night: SARO 07's service map and the tracing of E09A ----------
// The map is north up, 1200 x 800, not to any survey scale. The old Roswell road leaves
// the highway at the diner; its mileposts count from there. C runs due east from the
// transit at STATION 01 (as the stakes do at the built station), so on the map it is the
// horizontal through STATION 01. The road is laid so that line meets it 8.15 miles from
// the junction, just past the eight-mile post, where a survey bolt sits in the shoulder.
type Pt = [number, number];
const ROAD: Pt[] = [[200, 560], [330, 585], [470, 600], [610, 600], [740, 575], [850, 520], [930, 440], [985, 360], [1030, 270], [1080, 180], [1140, 110], [1190, 70]];
const STATION: Pt = [470, 360];
const CROSS_MILES = 8.15;
// distance along the road to a point on it, and the point at a distance along it
function along(pts: Pt[]) {
  const cum = [0];
  for (let i = 1; i < pts.length; i++) cum.push(cum[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
  const at = (d: number): Pt => {
    for (let i = 1; i < pts.length; i++) if (d <= cum[i]) {
      const k = (d - cum[i - 1]) / (cum[i] - cum[i - 1]);
      return [pts[i - 1][0] + (pts[i][0] - pts[i - 1][0]) * k, pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * k];
    }
    return pts[pts.length - 1];
  };
  return { cum, at, total: cum[cum.length - 1] };
}
// where the line east from STATION 01 meets the road, as a distance along it
function crossing() {
  const { cum } = along(ROAD);
  for (let i = 1; i < ROAD.length; i++) {
    const [ax, ay] = ROAD[i - 1], [bx, by] = ROAD[i];
    if ((ay - STATION[1]) * (by - STATION[1]) <= 0 && ay !== by) {
      const k = (STATION[1] - ay) / (by - ay), x = ax + (bx - ax) * k;
      if (x > STATION[0]) return cum[i - 1] + Math.hypot(bx - ax, by - ay) * k;
    }
  }
  return cum[cum.length - 1];
}
export const SERVICE_MAP = (() => {
  const a = along(ROAD), cross = crossing(), perMile = cross / CROSS_MILES;
  // the bolts: the one where C crosses, and three others along the shoulder
  const bolts = [3.3, 6.0, CROSS_MILES, 10.4].map((mi) => ({ mi, at: a.at(mi * perMile) }));
  return { w: 1200, h: 800, station: STATION, road: ROAD, perMile, miles: a.total / perMile, bolts, post: (mi: number) => a.at(mi * perMile), snap: 18 };
})();

function drawServiceMap(g: CanvasRenderingContext2D) {
  const M = SERVICE_MAP;
  g.fillStyle = INK; g.strokeStyle = INK; g.lineCap = 'round'; g.lineJoin = 'round';
  g.font = '600 30px Oswald, sans-serif'; g.fillText('SARO 07 / SERVICE MAP', 40, 54);
  g.font = '19px "Special Elite", serif'; g.fillText('Access and old survey roads. Mileposts from the junction at the diner. BM: survey bolt in the shoulder.', 40, 86, 1120);
  // north arrow
  g.lineWidth = 3;
  g.beginPath(); g.moveTo(1130, 200); g.lineTo(1130, 140); g.lineTo(1120, 160); g.moveTo(1130, 140); g.lineTo(1140, 160); g.stroke();
  g.font = '24px "Special Elite", serif'; g.fillText('N', 1121, 228);
  // the highway, north to south
  g.lineWidth = 9; g.strokeStyle = '#3d4a44';
  g.beginPath(); g.moveTo(200, 110); g.lineTo(200, 780); g.stroke();
  g.lineWidth = 2; g.strokeStyle = PAPER; g.setLineDash([14, 12]);
  g.beginPath(); g.moveTo(200, 110); g.lineTo(200, 780); g.stroke();
  g.setLineDash([]); g.strokeStyle = INK;
  g.save(); g.translate(176, 760); g.rotate(-Math.PI / 2); g.font = '18px "Special Elite", serif'; g.fillText('HIGHWAY', 0, 0); g.restore();
  // SARO and its array, Sierra Motor Court across the road
  g.lineWidth = 2.5;
  for (let i = 0; i < 9; i++) { g.beginPath(); g.arc(300 + (i % 3) * 30, 128 + Math.floor(i / 3) * 22, 7, 0, Math.PI * 2); g.stroke(); }
  g.strokeRect(286, 200, 52, 32);
  g.font = '600 22px Oswald, sans-serif'; g.fillText('SARO', 348, 224);
  g.strokeRect(96, 196, 66, 24);
  g.font = '17px "Special Elite", serif'; g.fillText('SIERRA MOTOR CT', 40, 248);
  // the diner at the junction
  g.fillRect(150, 548, 34, 22);
  g.font = '600 20px Oswald, sans-serif'; g.fillText('MESA DINER', 40, 600);
  // the old Roswell road
  g.lineWidth = 6; g.strokeStyle = '#4a5550';
  g.beginPath(); M.road.forEach(([x, y], i) => (i ? g.lineTo(x, y) : g.moveTo(x, y))); g.stroke();
  g.strokeStyle = INK;
  g.font = '18px "Special Elite", serif';
  g.save(); g.translate(560, 632); g.fillText('OLD ROAD TO ROSWELL', 0, 0); g.restore();
  // mileposts: a tick across the road and the number
  g.lineWidth = 2;
  for (let mi = 1; mi <= Math.floor(M.miles); mi++) {
    const [x, y] = M.post(mi), [x2, y2] = M.post(mi + 0.02);
    const nx = -(y2 - y), ny = x2 - x, l = Math.hypot(nx, ny) || 1;
    g.beginPath(); g.moveTo(x - nx / l * 9, y - ny / l * 9); g.lineTo(x + nx / l * 9, y + ny / l * 9); g.stroke();
    g.font = '600 19px Oswald, sans-serif';
    g.fillText(String(mi), x + nx / l * 16 - 5, y + ny / l * 16 + 7);
  }
  // survey bolts on the shoulder: a small triangle with a dot
  for (const b of M.bolts) {
    const [x, y] = b.at;
    g.beginPath(); g.moveTo(x - 9, y - 13); g.lineTo(x + 9, y - 13); g.lineTo(x, y - 27); g.closePath(); g.stroke();
    g.beginPath(); g.arc(x, y - 18, 2.2, 0, Math.PI * 2); g.fill();
    g.font = '16px "Special Elite", serif'; g.fillText('BM', x - 30, y - 16);
  }
  // STATION 01 north of the road, with its fence
  const [sx, sy] = M.station;
  g.lineWidth = 2; g.setLineDash([5, 5]); g.strokeRect(sx - 30, sy - 24, 60, 48); g.setLineDash([]);
  g.lineWidth = 3; mark(g, sx, sy + 2, 22);
  g.font = '600 20px Oswald, sans-serif'; g.fillText('STATION 01', sx - 52, sy - 34);
  g.font = '16px "Special Elite", serif'; g.fillText('old survey station, 1947', sx - 82, sy + 50);
  g.font = '16px "Special Elite", serif';
  g.fillText('SARO 07 / KEEP IN THE DOOR POCKET', 40, 780);
}

/** E14, the service map as a picture for the case file. */
export function serviceMap() {
  return sheet(SERVICE_MAP.w, SERVICE_MAP.h, drawServiceMap);
}
/** The service map on a canvas, for the overlay at the diner (ui/MapOverlay.ts). */
export function serviceMapCanvas() {
  const c = document.createElement('canvas'); c.width = SERVICE_MAP.w; c.height = SERVICE_MAP.h;
  const g = c.getContext('2d')!;
  paperBg(g, c.width, c.height);
  drawServiceMap(g);
  return c;
}
/** E09A traced onto onion-skin: the transit at (x, y), C running east to the edge of the
 *  sheet, A and B close by, and the north arrow. Drawn over the service map. */
export function drawTracing(g: CanvasRenderingContext2D, x: number, y: number) {
  const W = 620, H = 300, left = x - 70, top = y - H / 2;
  g.save();
  g.fillStyle = 'rgba(214,226,232,.38)'; g.strokeStyle = 'rgba(40,70,110,.55)'; g.lineWidth = 1.5;
  g.fillRect(left, top, W, H); g.strokeRect(left, top, W, H);
  g.strokeStyle = '#1d3f7a'; g.fillStyle = '#1d3f7a'; g.lineCap = 'round';
  // the transit
  g.lineWidth = 2.5;
  g.beginPath(); g.arc(x, y, 12, 0, Math.PI * 2); g.stroke();
  g.beginPath(); g.moveTo(x - 18, y); g.lineTo(x + 18, y); g.moveTo(x, y - 18); g.lineTo(x, y + 18); g.stroke();
  // C, the closing line, east to the edge of the sheet
  g.lineWidth = 3;
  g.beginPath(); g.moveTo(x + 12, y); g.lineTo(left + W - 8, y); g.stroke();
  g.font = '24px "Special Elite", serif'; g.fillText('C', left + W - 34, y - 12);
  // A to the north on the sight line, B to the west
  g.lineWidth = 2; g.setLineDash([6, 5]);
  g.beginPath(); g.moveTo(x, y - 12); g.lineTo(x, y - 60); g.stroke(); g.setLineDash([]);
  g.font = '19px "Special Elite", serif'; g.fillText('A', x + 8, y - 52); g.fillText('B', x - 58, y + 6);
  g.beginPath(); g.moveTo(x - 12, y); g.lineTo(x - 44, y); g.stroke();
  // north arrow and the note
  g.beginPath(); g.moveTo(left + W - 30, top + 74); g.lineTo(left + W - 30, top + 30); g.lineTo(left + W - 37, top + 44); g.moveTo(left + W - 30, top + 30); g.lineTo(left + W - 23, top + 44); g.stroke();
  g.font = '15px "Special Elite", serif';
  g.fillText('E09A, traced. Transit on the station.', left + 12, top + H - 14);
  g.restore();
}
