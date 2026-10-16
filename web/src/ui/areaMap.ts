import { SERVICE_MAP, mark } from '../story/drawings';

// The map in the notebook (Tom, 16 October: a map of the place). The player's own sketch in
// pencil on a notebook page, not the service map: SARO and the array, the highway, Sierra
// Motor Court, Mesa Diner at the junction, the old road to Roswell going off east, and the
// track to STATION 01 once the player knows of it. Laid out like the service map (the same
// places in the same spots, drawings.ts), and not to scale. A cross marks where the player is.
// It shows no survey bolts and no line C: the map at the diner is for that (ui/MapOverlay.ts).

export type MapSpot = 'saro' | 'yard' | 'motel' | 'highway' | 'diner' | 'track' | 'station01' | 'oldroad';
export interface MapView {
  /** Where the player is: a place and, on a road, how far along it (0..1, or the mile). */
  at: { spot: MapSpot; t?: number; mi?: number } | null;
  /** Places the player knows of. */
  station: boolean;
  oldRoad: boolean;
}

const PAPER = '#f1ead8', PENCIL = '#3b3b3f', RED = '#a8281e';
const W = 1200, H = 800;
const SARO: [number, number] = [312, 200], MOTEL: [number, number] = [118, 208], DINER: [number, number] = [167, 559];
const TRACK0: [number, number] = [200, 300];

/** Where a spot is on the page. */
export function spotXY(at: NonNullable<MapView['at']>): [number, number] {
  const M = SERVICE_MAP;
  switch (at.spot) {
    case 'saro': return SARO;
    case 'yard': return [SARO[0] + 22, SARO[1] - 30];
    case 'motel': return MOTEL;
    case 'highway': { const t = Math.min(1, Math.max(0, at.t ?? 0)); return [200, SARO[1] + 16 + (DINER[1] - SARO[1] - 16) * t]; }
    case 'diner': return DINER;
    case 'track': { const t = Math.min(1, Math.max(0, at.t ?? 0)); return [TRACK0[0] + (M.station[0] - 30 - TRACK0[0]) * t, TRACK0[1] + (M.station[1] - TRACK0[1]) * t]; }
    case 'station01': return M.station;
    case 'oldroad': { const p = M.post(Math.min(M.miles, Math.max(0, at.mi ?? 0))); return [p[0], p[1]]; }
  }
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
  const text = (s: string, x: number, y: number, size = 24) => { g.fillStyle = PENCIL; g.font = `${size}px "Reenie Beanie", "Special Elite", cursive`; g.fillText(s, x, y); };
  g.globalAlpha = 0.9;
  text('the area round SARO, from memory', 96, 50, 32);
  // north
  line([[1120, 190], [1120, 120]]); line([[1110, 140], [1120, 120], [1130, 140]]); text('N', 1112, 220, 30);
  // the highway, two lines
  line([[188, 96], [188, 780]], 1.8); line([[212, 96], [212, 780]], 1.8);
  text('highway', 120, 470, 24);
  // SARO, its array and the yard
  for (let i = 0; i < 9; i++) { const x = 300 + (i % 3) * 30, y = 118 + Math.floor(i / 3) * 22; g.beginPath(); g.lineWidth = 2; g.arc(x + r() * 2, y + r() * 2, 7, 0, Math.PI * 2); g.stroke(); }
  text('27 dishes, az 026', 400, 140, 22);
  line([[286, 186], [338, 186], [338, 218], [286, 218], [286, 186]]);
  text('SARO', 348, 214, 30);
  // the motel across the road
  line([[88, 196], [150, 196], [150, 218], [88, 218], [88, 196]]);
  text('Sierra Motor Ct', 76, 250, 24); text('(Nora, rm 6)', 84, 276, 20);
  // the diner at the junction
  line([[150, 548], [184, 548], [184, 570], [150, 570], [150, 548]]);
  text('Mesa Diner', 66, 604, 28); text('open all night', 70, 630, 20);
  // the old road east
  if (v.oldRoad) {
    line(SERVICE_MAP.road.map(([x, y]) => [x, y] as [number, number]), 2.6);
    text('old road to Roswell', 560, 640, 26);
    for (const mi of [2, 4, 6, 8, 10]) {
      const [x, y] = SERVICE_MAP.post(mi);
      g.fillStyle = PENCIL; g.beginPath(); g.arc(x, y, 3, 0, Math.PI * 2); g.fill();
      text(String(mi), x + 8, y + 22, 20);
    }
  } else {
    line([[212, 562], [270, 580]], 2.2);
    text('old road (closed)', 230, 610, 20);
  }
  // the track to STATION 01
  if (v.station) {
    const [sx, sy] = SERVICE_MAP.station;
    g.setLineDash([10, 9]); line([TRACK0, [sx - 30, sy]], 2); g.setLineDash([]);
    g.lineWidth = 2.4; g.strokeStyle = PENCIL; mark(g, sx, sy, 22);
    text('STATION 01', sx - 54, sy - 30, 26); text('1947', sx - 18, sy + 42, 20);
  }
  // where the player is
  if (v.at) {
    const [x, y] = spotXY(v.at);
    g.globalAlpha = 1; g.strokeStyle = RED; g.lineWidth = 4;
    g.beginPath(); g.moveTo(x - 11, y - 11); g.lineTo(x + 11, y + 11); g.moveTo(x + 11, y - 11); g.lineTo(x - 11, y + 11); g.stroke();
    g.fillStyle = RED; g.font = '24px "Reenie Beanie", "Special Elite", cursive'; g.fillText('me', x + 14, y - 10);
  }
  return c.toDataURL('image/png');
}
