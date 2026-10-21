import { SERVICE_MAP, mark } from '../story/drawings';
import { CROSSINGS } from '../story/lines';

// The map in the notebook (Tom, 16 October: a map of the place). The player's own sketch in
// pencil on a notebook page, not the service map: SARO and the array east of the highway,
// Sierra Motor Court 650 m down it on the west side, the track west to STATION 01 once the
// player knows of it, Mesa Diner at the junction, and the old road to Roswell going off east.
// Drawn the way the places really lie (Tom's improvements, 16 October: the service map has
// STATION 01 east of the highway, and its puzzle at the diner needs it so; this page does not),
// and not to scale. A cross marks where the player is. It shows no survey bolts, and no line C
// until the maps at the diner have found it (ui/MapOverlay.ts, P13 in chapter five).
//
// The lines meet (FORSLAG.md, 20 October, small change 7): with A and B drawn in and C found, the
// three are drawn on past where the player knows them, faint, to the edges of the page. They cross
// in one point and nowhere else, the transit at STATION 01 (they are its sight lines, HENDELSER.md
// section 3), and that point has a small circle round it and no word.

export type MapSpot = 'saro' | 'yard' | 'motel' | 'office' | 'highway' | 'diner' | 'track' | 'station01' | 'oldroad';
export interface MapView {
  /** Where the player is: a place and, on a road, how far along it (the highway: metres south
   *  of SARO; the track: 0..1 from the highway; the old road: the mile). */
  at: { spot: MapSpot; t?: number; m?: number; mi?: number } | null;
  /** Places the player knows of. */
  station: boolean;
  oldRoad: boolean;
  /** What happened where (story/townEvents.ts): a small circle with the time by it. Three on
   *  line A and the line is drawn in, through the town and on towards STATION 01. */
  marks?: { kind: string; clock: number; line?: 'A' | 'B' | 'C'; at?: MapView['at'] }[];
  /** C has been found (P13, at the diner): drawn from just past the bolt on the old road to STATION 01. */
  lineC?: boolean;
}

/** A and B are drawn in (three marks on each) and C is found: all three are drawn on past where
 *  they are known, and where they cross is circled. */
export function linesMeet(v: MapView) {
  const marks = v.marks ?? [], n = (l: 'A' | 'B') => marks.filter((m) => (m.line ?? 'A') === l).length;
  return !!v.lineC && !!v.oldRoad && !!v.station && n('A') >= 3 && n('B') >= 3;
}

const PAPER = '#f1ead8', PENCIL = '#3b3b3f', RED = '#a8281e';
const W = 1200, H = 800;
/** The highway's line on the page. */
const HX = 420;
const SARO: [number, number] = [532, 202], MOTEL: [number, number] = [371, 261], DINER: [number, number] = [387, 609];
const MOUTH: [number, number] = [HX - 12, 360], STATION: [number, number] = [170, 372];
/** Down the highway, metres south of SARO to the page: SARO, the motel, the track, the diner. */
const HWY_Y: [number, number][] = [[0, SARO[1] + 14], [690, MOTEL[1]], [815, MOUTH[1]], [2300, DINER[1]]];
function hwyY(m: number) {
  for (let i = 1; i < HWY_Y.length; i++) {
    const [m0, y0] = HWY_Y[i - 1], [m1, y1] = HWY_Y[i];
    if (m <= m1 || i === HWY_Y.length - 1) return y0 + (y1 - y0) * Math.min(1, Math.max(0, (m - m0) / (m1 - m0)));
  }
  return HWY_Y[0][1];
}
/** The old road as the service map has it (east from the junction), moved onto this page. */
const onPage = ([x, y]: readonly number[]): [number, number] => [HX + (x - 200) * 0.75, y + 50];

/** Where a spot is on the page. */
export function spotXY(at: NonNullable<MapView['at']>): [number, number] {
  switch (at.spot) {
    case 'saro': return SARO;
    case 'yard': return [SARO[0] + 22, SARO[1] - 30];
    case 'motel': return MOTEL;
    // the motel's office, at its north end: just off the corner of its box, so a mark there and
    // its time keep clear of the box, its name, the highway's marks and the cross for the player
    case 'office': return [MOTEL[0] - 23, MOTEL[1] - 25];
    case 'highway': return [HX, hwyY(at.m ?? 0)];
    case 'diner': return DINER;
    case 'track': { const t = Math.min(1, Math.max(0, at.t ?? 0)); return [MOUTH[0] + (STATION[0] + 26 - MOUTH[0]) * t, MOUTH[1] + (STATION[1] - MOUTH[1]) * t]; }
    case 'station01': return STATION;
    case 'oldroad': return onPage(SERVICE_MAP.post(Math.min(SERVICE_MAP.miles, Math.max(0, at.mi ?? 0))));
  }
}

// The same facts as the pencil labels, plus the cross and event marks that are
// actually drawn below. Fixed phrases can be recorded without inventing a new
// map, reading hidden survey geometry, or generating every possible clock value.
const MAP_READING = [
  'A pencil sketch of the area round SARO, from memory.',
  'The highway.',
  'SARO. 27 dishes, azimuth 026.',
  'Sierra Motor Court. Nora, room 6.',
  'Mesa Diner. Open all night.',
  'Caliche, population 412.',
];
const MAP_POSITION: Record<MapSpot, string> = {
  saro: 'The red cross marks me at SARO.',
  yard: 'The red cross marks me in the service yard.',
  motel: 'The red cross marks me at Sierra Motor Court.',
  office: 'The red cross marks me at the motel office.',
  highway: 'The red cross marks me on the highway.',
  diner: 'The red cross marks me at Mesa Diner.',
  track: 'The red cross marks me on the track to STATION 01.',
  station01: 'The red cross marks me at STATION 01.',
  oldroad: 'The red cross marks me on the old road to Roswell.',
};

/** A spoken description of this notebook sketch, using its current visible state. */
export function areaMapDescription(v: MapView): string[] {
  const text = [...MAP_READING];
  if (v.station) text.push('STATION 01. 1947.');
  text.push(v.oldRoad ? 'The old road to Roswell.' : 'The old road is closed.');
  const marks = v.marks ?? [];
  if (marks.length) text.push('Small circles mark events and their times.');
  if (marks.some((m) => m.kind === 'away')) text.push('A ring marks time away.');
  for (const line of ['A', 'B'] as const) {
    if (marks.filter((m) => (m.line ?? 'A') === line).length >= 3) text.push(line === 'A' ? 'Line A.' : 'Line B.');
  }
  if (v.at) text.push(MAP_POSITION[v.at.spot]);
  return text;
}

/** The page, as an image. */
export function areaMap(v: MapView) {
  const c = document.createElement('canvas'); c.width = W; c.height = H;
  const g = c.getContext('2d')!;
  // a notebook page: faint ruled lines and a margin
  g.fillStyle = PAPER; g.fillRect(0, 0, W, H);
  g.strokeStyle = 'rgba(70,110,160,.16)'; g.lineWidth = 1.5;
  for (let y = 60; y < H; y += 34) { g.beginPath(); g.moveTo(0, y); g.lineTo(W, y); g.stroke(); }
  g.strokeStyle = 'rgba(190,60,50,.25)'; g.beginPath(); g.moveTo(70, 0); g.lineTo(70, H); g.stroke();
  // pencil: a little uneven
  let seed = 7;
  const r = () => { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296 - 0.5; };
  const line = (pts: [number, number][], w = 2.2) => {
    g.lineWidth = w; g.strokeStyle = PENCIL; g.lineCap = 'round'; g.lineJoin = 'round';
    for (let pass = 0; pass < 2; pass++) {
      g.beginPath();
      pts.forEach(([x, y], i) => { const jx = x + r() * 1.6, jy = y + r() * 1.6; if (i) g.lineTo(jx, jy); else g.moveTo(jx, jy); });
      g.stroke();
    }
  };
  const box = (x0: number, y0: number, x1: number, y1: number) => line([[x0, y0], [x1, y0], [x1, y1], [x0, y1], [x0, y0]]);
  const text = (s: string, x: number, y: number, size = 24) => { g.fillStyle = PENCIL; g.font = `${size}px "Reenie Beanie", "Special Elite", cursive`; g.fillText(s, x, y); };
  g.globalAlpha = 0.9;
  text('the area round SARO, from memory', 96, 50, 32);
  // north
  line([[1120, 740], [1120, 670]]); line([[1110, 690], [1120, 670], [1130, 690]]); text('N', 1135, 700, 30);
  // the highway, two lines
  line([[HX - 12, 96], [HX - 12, 780]], 1.8); line([[HX + 12, 96], [HX + 12, 780]], 1.8);
  text('highway', HX - 116, 490, 24);
  // SARO east of it, its array to the north, and the service road out to the highway
  for (let i = 0; i < 9; i++) { const x = 520 + (i % 3) * 30, y = 118 + Math.floor(i / 3) * 22; g.beginPath(); g.lineWidth = 2; g.arc(x + r() * 2, y + r() * 2, 7, 0, Math.PI * 2); g.stroke(); }
  text('27 dishes, az 026', 620, 140, 22);
  box(506, 186, 558, 218);
  text('SARO', 568, 214, 30);
  line([[HX + 12, 210], [506, 206]], 1.4);
  // the motel down the highway, on the west side
  box(340, 250, 402, 272);
  text('Sierra Motor Ct', 186, 262, 24); text('(Nora, rm 6)', 214, 288, 20);
  // the diner at the junction, on the west side too
  box(370, 598, 404, 620);
  text('Mesa Diner', 226, 652, 28); text('open all night', 236, 678, 20);
  // Caliche down the street from it, both sides of the highway (world/Town.ts)
  for (let i = 0; i < 3; i++) { box(392, 628 + i * 15, 408, 638 + i * 15); box(HX + 10, 632 + i * 15, HX + 24, 641 + i * 15); }
  text('Caliche, pop. 412', 226, 708, 22);
  // the old road east
  if (v.oldRoad) {
    line(SERVICE_MAP.road.map(onPage), 2.6);
    text('old road to Roswell', 690, 690, 26);
    for (const mi of [2, 4, 6, 8, 10]) {
      const [x, y] = onPage(SERVICE_MAP.post(mi));
      g.fillStyle = PENCIL; g.beginPath(); g.arc(x, y, 3, 0, Math.PI * 2); g.fill();
      text(String(mi), x + 8, y + 22, 20);
    }
  } else {
    line([[HX + 12, 612], [478, 630]], 2.2);
    text('old road (closed)', 450, 664, 20);
  }
  // the track west to STATION 01
  if (v.station) {
    g.setLineDash([10, 9]); line([MOUTH, [STATION[0] + 26, STATION[1]]], 2); g.setLineDash([]);
    g.lineWidth = 2.4; g.strokeStyle = PENCIL; mark(g, STATION[0], STATION[1], 22);
    text('STATION 01', STATION[0] - 66, STATION[1] - 30, 26); text('1947', STATION[0] - 18, STATION[1] + 42, 20);
  }
  // what happened where, with the time; three on one line and the line is drawn through them
  const marks = v.marks ?? [];
  const onLine = (l: 'A' | 'B') => marks.filter((m) => (m.line ?? 'A') === l).length;
  const across = (from: [number, number], to: [number, number], k0: number, k1: number, label: string) => {
    const [ax, ay] = from, [bx, by] = to;
    g.setLineDash([3, 7]); line([[ax + (bx - ax) * k0, ay + (by - ay) * k0], [ax + (bx - ax) * k1, ay + (by - ay) * k1]], 1.6); g.setLineDash([]);
    text(label, ax + (bx - ax) * k1 - 28, ay + (by - ay) * k1 - 4, 28);
  };
  // where the lines are on the ground (story/lines.ts), as this page has the places: A from beyond
  // the town, through it, to STATION 01 and a little past; B out of the dishes, across the highway
  // at SARO's turn-off (CROSSINGS.B, 23 m south of the site), on south-west to STATION 01 and past;
  // C, once found at the diner, from just past the bolt on the old road back to STATION 01
  const fromA: [number, number] = [DINER[0], DINER[1] + 30], fromB: [number, number] = [HX, hwyY(CROSSINGS.B.highwayM)];
  const bolt = SERVICE_MAP.bolts.find((b) => Math.abs(b.mi - CROSSINGS.C.oldRoad) < 0.01)?.at ?? SERVICE_MAP.post(CROSSINGS.C.oldRoad);
  const fromC = onPage(bolt);
  if (onLine('A') >= 3) across(fromA, STATION, -0.35, 1.12, 'A');
  if (onLine('B') >= 3) across(fromB, STATION, -0.5, 1.12, 'B');
  const cOn = !!v.lineC && v.oldRoad && v.station;
  if (cOn) {
    // (its letter at the old road, where it was found: by the station the page has A's and B's)
    across(fromC, STATION, -0.06, 1.12, '');
    const [bx, by] = fromC;
    text('C', bx + 24, by - 12, 28);
    // the bolt where it crosses, as the maps at the diner have it: a small cross on the road
    line([[bx - 6, by - 6], [bx + 6, by + 6]], 1.6); line([[bx + 6, by - 6], [bx - 6, by + 6]], 1.6);
  }
  // drawn on past what is known, all three: faint, to the edges of the page, and where they cross
  // a small circle; nothing written by it
  if (linesMeet(v)) {
    const page = { x0: 74, x1: W - 8, y0: 64, y1: H - 8 };
    // how far along from..STATION the line leaves the page, beyond STATION (k > 1) or before `from` (k < 0)
    const edgeK = (from: [number, number], dir: 1 | -1) => {
      const dx = STATION[0] - from[0], dy = STATION[1] - from[1];
      let k = dir > 0 ? 1e9 : -1e9;
      for (const [d, p, lo, hi] of [[dx, from[0], page.x0, page.x1], [dy, from[1], page.y0, page.y1]] as const) {
        if (Math.abs(d) < 1e-6) continue;
        for (const e of [lo, hi]) { const t = (e - p) / d; if (dir > 0 ? t > 1 && t < k : t < 0 && t > k) k = t; }
      }
      return k;
    };
    g.save(); g.globalAlpha = 0.42;
    for (const [from, k0, k1] of [[fromA, -0.35, 1.12], [fromB, -0.5, 1.12], [fromC, -0.06, 1.12]] as const) {
      const [ax, ay] = from, [bx, by] = STATION, at = (k: number): [number, number] => [ax + (bx - ax) * k, ay + (by - ay) * k];
      g.setLineDash([2, 12]);
      line([at(k1), at(edgeK(from, 1))], 1.3);
      line([at(k0), at(Math.max(edgeK(from, -1), k0 - 0.6))], 1.3);
      g.setLineDash([]);
    }
    g.restore();
    g.lineWidth = 1.8; g.strokeStyle = PENCIL;
    for (let pass = 0; pass < 2; pass++) { g.beginPath(); g.arc(STATION[0] + r() * 1.2, STATION[1] + r() * 1.2, 31, 0, Math.PI * 2); g.stroke(); }
  }
  const hhmm = (c: number) => { const d = ((c % 86400) + 86400) % 86400; return `${String(Math.floor(d / 3600)).padStart(2, '0')}:${String(Math.floor(d % 3600 / 60)).padStart(2, '0')}`; };
  const inTown: string[] = [];
  let k = 0;
  // (two at the same spot, the truck on B at the turn-off twice: the second's time under the first's)
  const drawn: [number, number][] = [];
  for (const m of marks) {
    const [x, y] = m.at ? spotXY(m.at) : [HX - 4 + (k % 3) * 14, 640 + Math.floor(k / 3) * 14];
    if (!m.at) k++;
    g.lineWidth = 2; g.strokeStyle = PENCIL; g.beginPath(); g.arc(x, y, 6, 0, Math.PI * 2); g.stroke();
    // a time away: a ring round it
    if (m.kind === 'away') { g.beginPath(); g.arc(x, y, 13, 0, Math.PI * 2); g.stroke(); }
    const under = drawn.filter(([dx, dy]) => Math.hypot(dx - x, dy - y) < 12).length;
    // (beside the motel's box, the truck's seven pulling out of its lot: the time east of the highway)
    const ty = y + 6 + under * 20, tx = ty > 246 && ty < 292 && x - 64 < 404 && x - 18 > 338 ? x + 16 : x - 64;
    if (m.at) { text(hhmm(m.clock), tx, ty, 20); drawn.push([x, y]); } else inTown.push(hhmm(m.clock));
  }
  // the times in town, in a row under its name
  if (inTown.length) text(inTown.join(', '), 226, 734, 20);
  // where the player is
  if (v.at) {
    const [x, y] = spotXY(v.at);
    g.globalAlpha = 1; g.strokeStyle = RED; g.lineWidth = 4;
    g.beginPath(); g.moveTo(x - 11, y - 11); g.lineTo(x + 11, y + 11); g.moveTo(x + 11, y - 11); g.lineTo(x - 11, y + 11); g.stroke();
    g.fillStyle = RED; g.font = '24px "Reenie Beanie", "Special Elite", cursive'; g.fillText('me', x + 14, y - 10);
  }
  return c.toDataURL('image/png');
}
