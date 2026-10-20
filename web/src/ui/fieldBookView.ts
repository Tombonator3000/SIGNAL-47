import type { UI } from './UI';

// T. Vega's third field book and the envelope for plate 4 (story/fieldBook.ts), each in the
// document view's own frame: the thing on the left, what it says on the right. The book is
// turned a page at a time (the buttons, the arrow keys, or a tap on the page), so its last page
// comes by itself. The envelope is held up to the lamp with a button, and its picture changes.
// What is written on them is drawn in code (AGENTS.md: text that must be right).

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export interface BookPage { hand: string; typed: string; last?: boolean }
export interface BookView { el: HTMLElement; page: () => number; go: (n: number) => void }

/** The field book, open at `start`. `onPage` hears each page turned to; `onClose`, the last one seen. */
export function fieldBookView(ui: UI, o: { title: string; pages: BookPage[]; start?: number; onPage?: (n: number) => void; onClose: (last: number) => void }): BookView {
  const el = document.createElement('div');
  el.className = 'docview fbook';
  el.innerHTML = `
    <div class="page-wrap"><div class="page fb-page" data-page><div class="hand" data-hand></div><span class="fb-no" data-no></span></div></div>
    <div class="text">
      <h3>${esc(o.title)}</h3>
      <p class="fb-typed" data-typed></p>
      <p class="fb-where" data-where></p>
      <div class="controls fb-nav">
        <button data-a="prev" aria-label="Previous page">&lsaquo; Back</button>
        <button data-a="next" aria-label="Next page">Next page &rsaquo;</button>
        <button data-a="back"><kbd>E</kbd>Put it back</button>
      </div>
    </div>`;
  const pageEl = el.querySelector('[data-page]') as HTMLElement;
  const hand = el.querySelector('[data-hand]') as HTMLElement;
  const typed = el.querySelector('[data-typed]') as HTMLElement;
  const where = el.querySelector('[data-where]') as HTMLElement;
  const no = el.querySelector('[data-no]') as HTMLElement;
  const prev = el.querySelector('[data-a=prev]') as HTMLButtonElement;
  const next = el.querySelector('[data-a=next]') as HTMLButtonElement;
  const side = el.querySelector('.text') as HTMLElement;
  let at = -1;
  const go = (n: number) => {
    n = Math.max(0, Math.min(o.pages.length - 1, n));
    if (n === at) return;
    const first = at < 0;
    at = n;
    const p = o.pages[n];
    hand.textContent = p.hand;
    typed.textContent = p.typed;
    pageEl.classList.toggle('last', !!p.last);
    pageEl.classList.toggle('cover', n === 0);
    no.textContent = n ? String(n) : '';
    where.textContent = `Page ${n + 1} of ${o.pages.length}`;
    prev.disabled = n === 0;
    next.disabled = n === o.pages.length - 1;
    // the page turned: a new sheet under the hand, and its sound
    if (!first) {
      ui.onPaper?.();
      pageEl.classList.remove('turn'); void pageEl.offsetWidth; pageEl.classList.add('turn');
    }
    side.querySelector(':scope > .read-aloud')?.remove();
    ui.readingControls(side, [p.typed], 'Read document', 'document');
    o.onPage?.(n);
  };
  el.querySelector('[data-a=back]')!.addEventListener('click', () => ui.close());
  prev.addEventListener('click', () => go(at - 1));
  next.addEventListener('click', () => go(at + 1));
  // a tap on the page turns it (the right half on, the left half back)
  pageEl.addEventListener('click', (e) => {
    const r = pageEl.getBoundingClientRect();
    go(e.clientX < r.left + r.width * 0.3 ? at - 1 : at + 1);
  });
  el.addEventListener('click', (e) => { if (e.target === el) ui.close(); });
  const key = (e: KeyboardEvent) => {
    if (!el.isConnected) return;
    if (['ArrowRight', 'KeyD', 'PageDown', 'Space'].includes(e.code)) { e.preventDefault(); go(at + 1); }
    else if (['ArrowLeft', 'KeyA', 'PageUp'].includes(e.code)) { e.preventDefault(); go(at - 1); }
  };
  addEventListener('keydown', key);
  ui.onPaper?.();
  ui.open(el, () => { removeEventListener('keydown', key); o.onClose(at); });
  go(o.start ?? 0);
  return { el, page: () => at, go };
}

/** The envelope on the office counter: its face, and a button to hold it up to the lamp. */
export function envelopeView(ui: UI, o: {
  title: string; front: string; lit: string; text: string; litText: string;
  onHold: () => void; onClose: () => void;
}) {
  const el = document.createElement('div');
  el.className = 'docview fenv';
  el.innerHTML = `
    <div class="page-wrap"><div class="page photo fenv-pic"><img alt="" data-front><img alt="" data-lit class="fenv-lit"></div></div>
    <div class="text">
      <h3>${esc(o.title)}</h3>
      <div data-say></div>
      <div class="controls"><button data-a="lamp">Hold it up to the lamp</button><button data-a="back"><kbd>E</kbd>Put it back</button></div>
    </div>`;
  const front = el.querySelector('[data-front]') as HTMLImageElement, lit = el.querySelector('[data-lit]') as HTMLImageElement;
  front.src = o.front; lit.src = o.lit;
  const say = el.querySelector('[data-say]') as HTMLElement;
  const lamp = el.querySelector('[data-a=lamp]') as HTMLButtonElement;
  const side = el.querySelector('.text') as HTMLElement;
  const show = (up: boolean) => {
    el.classList.toggle('up', up);
    front.alt = up ? '' : o.title;
    lit.alt = up ? `${o.title}, held up to the lamp` : '';
    say.innerHTML = (up ? o.litText : o.text).split('\n\n').map((p) => `<p>${esc(p)}</p>`).join('');
    lamp.textContent = up ? 'Lower it' : 'Hold it up to the lamp';
    side.querySelector(':scope > .read-aloud')?.remove();
    ui.readingControls(side, (up ? o.litText : o.text).split(/\n\s*\n/).filter((p) => p.trim()), 'Read description', 'description');
  };
  let up = false;
  lamp.addEventListener('click', () => {
    up = !up;
    ui.onPaper?.();
    show(up);
    if (up) o.onHold();
  });
  el.querySelector('[data-a=back]')!.addEventListener('click', () => ui.close());
  el.addEventListener('click', (e) => { if (e.target === el) ui.close(); });
  ui.onPaper?.();
  ui.open(el, o.onClose);
  show(false);
  return { el, hold: () => { if (!up) lamp.click(); } };
}

// ---------- the envelope, drawn ----------
const W = 900, H = 600;
const EX = 150, EY = 120, EW = 600, EH = 360;   // the envelope on the picture
let rnd = 1;
const r = () => { rnd = (rnd * 16807) % 2147483647; return (rnd - 1) / 2147483646; };
function canvas() { const c = document.createElement('canvas'); c.width = W; c.height = H; return c; }
function grain(g: CanvasRenderingContext2D, x: number, y: number, w: number, h: number, n: number, dark: string, light: string) {
  for (let i = 0; i < n; i++) {
    g.fillStyle = r() < 0.5 ? dark : light;
    const a = r() * Math.PI, l = 2 + r() * 9;
    g.save(); g.translate(x + r() * w, y + r() * h); g.rotate(a); g.fillRect(-l / 2, 0, l, 0.8 + r() * 0.8); g.restore();
  }
}
// what is written on the face of it, the same on both pictures (ink in front of the light)
function face(g: CanvasRenderingContext2D, lit: boolean) {
  const ink = lit ? 'rgba(28,20,14,.92)' : 'rgba(30,26,34,.9)';
  // the shop's printed corner
  g.fillStyle = lit ? 'rgba(40,28,18,.8)' : 'rgba(70,50,40,.85)';
  g.font = '600 15px Oswald, "Arial Narrow", sans-serif';
  g.fillText('PRINTS  .  ENLARGEMENTS  .  FINISHING', EX + 26, EY + 34);
  g.font = '13px "Special Elite", "Courier New", serif';
  g.fillText('ROSWELL, N. MEX.', EX + 26, EY + 54);
  // the stamp: three cents, purple, with its teeth
  const sx = EX + EW - 92, sy = EY + 22;
  g.fillStyle = lit ? 'rgba(30,18,30,.95)' : '#6b4a7c';
  for (let i = 0; i < 9; i++) for (const [x, y] of [[sx + 4 + i * 7, sy], [sx + 4 + i * 7, sy + 82]]) { g.beginPath(); g.arc(x, y, 2.4, 0, Math.PI * 2); g.fill(); }
  g.fillRect(sx, sy + 2, 66, 78);
  if (!lit) {
    g.fillStyle = '#e9dcc6'; g.fillRect(sx + 6, sy + 8, 54, 66);
    g.fillStyle = '#6b4a7c'; g.fillRect(sx + 9, sy + 11, 48, 60);
    g.fillStyle = '#efe4d2'; g.font = '600 24px Oswald, sans-serif'; g.fillText('3c', sx + 19, sy + 50);
    g.font = '9px Oswald, sans-serif'; g.fillText('U.S. POSTAGE', sx + 11, sy + 66);
  }
  // the postmark over it: a ring with the town and the month, the day under a smudge
  const px = sx - 70, py = sy + 46;
  g.strokeStyle = lit ? 'rgba(25,18,14,.8)' : 'rgba(40,40,60,.6)'; g.lineWidth = 2;
  g.beginPath(); g.arc(px, py, 34, 0, Math.PI * 2); g.stroke();
  g.fillStyle = g.strokeStyle; g.font = '11px "Special Elite", serif';
  g.fillText('ROSWELL', px - 26, py - 12); g.fillText('JUL', px - 11, py + 4); g.fillText('1947', px - 14, py + 18); g.fillText('N.MEX.', px - 19, py + 30);
  g.fillStyle = lit ? 'rgba(25,18,14,.55)' : 'rgba(40,40,60,.35)';
  g.beginPath(); g.ellipse(px + 14, py + 4, 9, 6, 0.4, 0, Math.PI * 2); g.fill();
  for (let i = 0; i < 5; i++) {
    g.beginPath(); g.moveTo(px + 36, py - 18 + i * 9);
    for (let x = 0; x <= 110; x += 10) g.lineTo(px + 36 + x, py - 18 + i * 9 + Math.sin(x / 9) * 3);
    g.stroke();
  }
  // the address, typed
  g.fillStyle = ink; g.font = '25px "Special Elite", "Courier New", serif';
  const ax = EX + 210, ay = EY + 190;
  g.fillText('T. Vega', ax, ay);
  g.fillText('c/o Sierra Motor Court', ax, ay + 34);
  g.fillText('Caliche, N. Mex.', ax, ay + 68);
  // the counter's pencil, along the bottom
  g.fillStyle = lit ? 'rgba(35,30,28,.85)' : 'rgba(70,70,78,.85)';
  g.font = '34px "Reenie Beanie", "Bradley Hand", cursive';
  g.save(); g.translate(EX + 34, EY + EH - 30); g.rotate(-0.025); g.fillText('Plate 4.   1 print.   35c.', 0, 0); g.restore();
}

/** The envelope lying on the office counter, seen from above in the office's light. */
export function envelopeFront(): string {
  rnd = 4711;
  const c = canvas(), g = c.getContext('2d')!;
  // the counter: dark varnished wood
  const wood = g.createLinearGradient(0, 0, 0, H);
  wood.addColorStop(0, '#2a1a10'); wood.addColorStop(1, '#170e08');
  g.fillStyle = wood; g.fillRect(0, 0, W, H);
  for (let i = 0; i < 70; i++) { g.strokeStyle = `rgba(${60 + r() * 40},${35 + r() * 20},${18 + r() * 10},.25)`; g.lineWidth = 1 + r() * 2; const y = r() * H; g.beginPath(); g.moveTo(0, y); g.bezierCurveTo(300, y + r() * 20 - 10, 600, y + r() * 20 - 10, W, y + r() * 16 - 8); g.stroke(); }
  // the lamp's pool on it, from the right
  const pool = g.createRadialGradient(W * 0.78, H * 0.18, 30, W * 0.7, H * 0.3, 620);
  pool.addColorStop(0, 'rgba(255,200,130,.32)'); pool.addColorStop(1, 'rgba(255,200,130,0)');
  g.fillStyle = pool; g.fillRect(0, 0, W, H);
  g.save(); g.translate(W / 2, H / 2); g.rotate(-0.035); g.translate(-W / 2, -H / 2);
  g.fillStyle = 'rgba(0,0,0,.45)'; g.fillRect(EX + 10, EY + 12, EW, EH);
  const paper = g.createLinearGradient(EX, EY, EX + EW, EY + EH);
  paper.addColorStop(0, '#d6c193'); paper.addColorStop(0.6, '#c9b080'); paper.addColorStop(1, '#b39a6c');
  g.fillStyle = paper; g.fillRect(EX, EY, EW, EH);
  grain(g, EX, EY, EW, EH, 2600, 'rgba(110,85,50,.14)', 'rgba(255,245,220,.12)');
  // 39 years on a counter: the edges gone darker, a ring where a cup stood once
  const edge = g.createRadialGradient(EX + EW / 2, EY + EH / 2, EH * 0.4, EX + EW / 2, EY + EH / 2, EW * 0.62);
  edge.addColorStop(0, 'rgba(90,60,30,0)'); edge.addColorStop(1, 'rgba(90,60,30,.35)');
  g.fillStyle = edge; g.fillRect(EX, EY, EW, EH);
  g.strokeStyle = 'rgba(110,80,45,.22)'; g.lineWidth = 5; g.beginPath(); g.arc(EX + EW - 130, EY + EH - 90, 46, 0.3, 5.6); g.stroke();
  face(g, false);
  g.restore();
  return c.toDataURL('image/jpeg', 0.86);
}

/** The same envelope held up in front of the counter lamp: the paper lit through, the flaps
 *  behind it, and the print inside as a shadow. On the print, small and dark against something
 *  lighter: the transit on its tripod, and three people standing by it, the third softer. */
export function envelopeLit(): string {
  rnd = 4711;
  const c = canvas(), g = c.getContext('2d')!;
  g.fillStyle = '#080604'; g.fillRect(0, 0, W, H);
  // the lamp behind it, a little right of the middle
  const lx = EX + EW * 0.6, ly = EY + EH * 0.42;
  const halo = g.createRadialGradient(lx, ly, 20, lx, ly, 560);
  halo.addColorStop(0, 'rgba(255,190,110,.55)'); halo.addColorStop(0.4, 'rgba(200,120,50,.18)'); halo.addColorStop(1, 'rgba(0,0,0,0)');
  g.fillStyle = halo; g.fillRect(0, 0, W, H);
  g.save(); g.translate(W / 2, H / 2); g.rotate(0.02); g.translate(-W / 2, -H / 2);
  // the paper, the light coming through it
  const through = g.createRadialGradient(lx, ly, 10, lx, ly, EW * 0.75);
  through.addColorStop(0, '#fff1c8'); through.addColorStop(0.35, '#f0c983'); through.addColorStop(0.8, '#b97b3c'); through.addColorStop(1, '#7a4a22');
  g.fillStyle = through; g.fillRect(EX, EY, EW, EH);
  grain(g, EX, EY, EW, EH, 3600, 'rgba(120,70,30,.16)', 'rgba(255,240,200,.1)');
  // the flaps on the back, where the paper is double: darker, meeting under the middle
  g.fillStyle = 'rgba(110,60,20,.22)';
  g.beginPath(); g.moveTo(EX, EY); g.lineTo(EX + EW * 0.5, EY + EH * 0.62); g.lineTo(EX + EW, EY); g.closePath(); g.fill();
  g.beginPath(); g.moveTo(EX, EY + EH); g.lineTo(EX + EW * 0.44, EY + EH * 0.5); g.lineTo(EX + EW * 0.56, EY + EH * 0.5); g.lineTo(EX + EW, EY + EH); g.closePath(); g.fill();
  // the print inside: a smaller sheet, a little askew, as a shadow
  const PX = EX + 92, PY = EY + 64, PW = 400, PH = 262;
  g.save(); g.translate(PX + PW / 2, PY + PH / 2); g.rotate(0.022); g.translate(-PW / 2, -PH / 2);
  g.filter = 'blur(2.2px)';
  g.fillStyle = 'rgba(70,32,10,.42)'; g.fillRect(0, 0, PW, PH);
  // what is light on the print lets the lamp through: the sky over a dark line of ground
  const sky = g.createLinearGradient(0, 0, 0, PH);
  sky.addColorStop(0, 'rgba(255,220,160,.26)'); sky.addColorStop(0.36, 'rgba(255,215,150,.38)'); sky.addColorStop(0.43, 'rgba(60,25,8,.0)');
  g.fillStyle = sky; g.fillRect(6, 6, PW - 12, PH * 0.43);
  g.fillStyle = 'rgba(55,22,6,.5)'; g.fillRect(0, PH * 0.43, PW, PH * 0.57);
  // the transit on its three legs, on the line of the ground (above where the address is typed)
  const GROUND = PH * 0.43, tx = 66, ty = GROUND - 44;
  g.strokeStyle = 'rgba(40,14,2,.85)'; g.lineWidth = 3;
  for (const dx of [-20, 2, 19]) { g.beginPath(); g.moveTo(tx, ty + 6); g.lineTo(tx + dx, GROUND + 2); g.stroke(); }
  g.fillStyle = 'rgba(40,14,2,.85)'; g.fillRect(tx - 8, ty - 8, 16, 12); g.fillRect(tx - 2, ty - 14, 4, 7);
  // three standing by it: two together, and one a step off and not as sharp
  const fig = (x: number, h: number, blur: number, a: number) => {
    g.filter = `blur(${blur}px)`;
    g.fillStyle = `rgba(36,12,2,${a})`;
    const top = GROUND + 2 - h;
    g.beginPath(); g.ellipse(x, top + h * 0.09, h * 0.075, h * 0.09, 0, 0, Math.PI * 2); g.fill();       // the head
    g.beginPath(); g.moveTo(x - h * 0.13, top + h * 0.2); g.lineTo(x + h * 0.13, top + h * 0.2);           // shoulders, body
    g.lineTo(x + h * 0.1, top + h * 0.6); g.lineTo(x + h * 0.06, top + h); g.lineTo(x - h * 0.06, top + h); g.lineTo(x - h * 0.1, top + h * 0.6); g.closePath(); g.fill();
  };
  fig(112, 66, 2.2, 0.88);
  fig(138, 60, 2.2, 0.86);
  fig(196, 64, 4.6, 0.6);
  g.filter = 'none';
  g.restore();
  // the wax seal on the back, where the flaps meet
  g.fillStyle = 'rgba(40,10,4,.7)'; g.beginPath(); g.arc(EX + EW * 0.5, EY + EH * 0.6, 15, 0, Math.PI * 2); g.fill();
  // the edges of the paper, darker where it curls away from the light
  g.strokeStyle = 'rgba(80,40,14,.6)'; g.lineWidth = 3; g.strokeRect(EX, EY, EW, EH);
  face(g, true);
  // the thumbs holding it up at the two bottom corners, dark against the light
  g.fillStyle = '#100a07'; g.lineCap = 'round';
  for (const [x0, y0, x1, y1] of [[EX + 58, EY + EH + 34, EX + 70, EY + EH - 14], [EX + EW - 56, EY + EH + 34, EX + EW - 68, EY + EH - 12]] as const) {
    g.strokeStyle = '#100a07'; g.lineWidth = 44; g.beginPath(); g.moveTo(x0, y0); g.lineTo(x1, y1); g.stroke();
    // the light round the edge of the thumb, through the paper
    g.strokeStyle = 'rgba(255,170,90,.18)'; g.lineWidth = 2; g.beginPath(); g.arc(x1, y1, 23, Math.PI * 1.05, Math.PI * 1.95); g.stroke();
  }
  g.restore();
  return c.toDataURL('image/jpeg', 0.88);
}
