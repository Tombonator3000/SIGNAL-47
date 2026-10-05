import type { UI } from './UI';
import { panelShell, panelSay } from './Panels';
import { SERVICE_MAP, serviceMapCanvas, drawTracing } from '../story/drawings';

// All Night's map puzzle (KAPITLER.md, P13): SARO 07's service map on the booth table and
// E09A traced on onion-skin over it. The tracing is dragged with the mouse or a finger; it
// settles on STATION 01 when its transit comes within a few millimetres of the station.
// Then C runs east across the map, and the player reads off where it meets the old road.

type Result = { ok: boolean; text: string };
const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export interface MapOverlayOpts {
  at: [number, number];                       // where the tracing's transit lies on the map now
  solved: string | null;                      // the recorded finding, once there is one
  question: string;
  choices: { id: string; label: string }[];
  onMove: (x: number, y: number, aligned: boolean) => void;
  onChoose: (id: string) => Result;
  onClose?: () => void;
}

export function mapOverlay(ui: UI, o: MapOverlayOpts) {
  const M = SERVICE_MAP;
  const el = panelShell(ui, 'THE BOOTH / SERVICE MAP AND E09A', `
    <p class="lp-sub" data-status></p>
    <div class="mapov"><canvas data-map width="${M.w}" height="${M.h}"></canvas></div>
    <div data-ask ${o.solved ? 'hidden' : ''}>
      <p class="lp-q">${esc(o.question)}</p>
      <div class="lp-col">${o.choices.map((c) => `<button class="lp-btn" data-c="${esc(c.id)}">${esc(c.label)}</button>`).join('')}</div>
    </div>
    <p class="lp-text${o.solved ? ' good' : ''}" data-say>${o.solved ? esc(o.solved) : ''}</p>`, o.onClose);
  const canvas = el.querySelector('[data-map]') as HTMLCanvasElement;
  const g = canvas.getContext('2d')!;
  const base = serviceMapCanvas();
  const status = el.querySelector('[data-status]') as HTMLElement;
  let [x, y] = o.at;
  const aligned = () => Math.hypot(x - M.station[0], y - M.station[1]) < 0.5;
  const draw = () => {
    g.drawImage(base, 0, 0);
    drawTracing(g, x, y);
    status.textContent = aligned()
      ? 'The tracing\'s transit sits on STATION 01, north to north. C runs east across the map.'
      : 'Drag the tracing until the transit lies on STATION 01.';
  };
  // map coordinates of a pointer
  const toMap = (e: PointerEvent) => {
    const r = canvas.getBoundingClientRect();
    return [(e.clientX - r.left) * M.w / r.width, (e.clientY - r.top) * M.h / r.height] as const;
  };
  let grab: [number, number] | null = null;
  canvas.addEventListener('pointerdown', (e) => {
    const [px, py] = toMap(e);
    // the tracing sheet is 620 by 300 with the transit 70 in from its left edge
    if (px < x - 70 || px > x + 550 || py < y - 150 || py > y + 150) return;
    grab = [px - x, py - y];
    canvas.setPointerCapture(e.pointerId);
    e.preventDefault();
  });
  canvas.addEventListener('pointermove', (e) => {
    if (!grab) return;
    const [px, py] = toMap(e);
    x = Math.min(M.w - 40, Math.max(40, px - grab[0]));
    y = Math.min(M.h - 40, Math.max(40, py - grab[1]));
    draw();
  });
  const drop = () => {
    if (!grab) return;
    grab = null;
    if (Math.hypot(x - M.station[0], y - M.station[1]) < M.snap) { [x, y] = M.station; }
    draw();
    o.onMove(x, y, aligned());
  };
  canvas.addEventListener('pointerup', drop);
  canvas.addEventListener('pointercancel', drop);
  el.querySelectorAll<HTMLButtonElement>('[data-c]').forEach((b) => b.addEventListener('click', () => {
    const r = o.onChoose(b.dataset.c!);
    panelSay(el, '[data-say]', r);
    if (r.ok) (el.querySelector('[data-ask]') as HTMLElement).hidden = true;
  }));
  draw();
  return el;
}
