import type { UI } from './UI';

// The case board: one cork board for the whole night (Tom, 4 October: the tabs and the three
// ready-made sentences were "knotete og vanskelig å forstå"). Everything in the journal lies
// on it as a card or a picture, each chapter in its own patch of cork. The player drags the
// cards about freely and ties a red thread between two pieces of evidence: pull it from a
// pin to another card, or tap one and then the other. A thread between two things that
// belong together holds and leaves a note; a note that answers a question goes into the
// question's slots by itself, and when they are full RECORD registers the finding. A thread
// that does not hold falls slack with a short reply. Cards with several things on them (the
// 1947 records) have a pin on each line that matters; anything else is one pin.
//
// The board is larger than any screen. Drag the cork (or use the wheel) to look around,
// pinch or ctrl+wheel or the buttons to zoom. Mouse and touch work the same way, so a phone
// in either orientation gets the same board, opened at a readable size. All the rules live
// in the chapters (story/boardRules.ts, story/caseBoard.ts); this file only draws and listens.
//
// The art from ART_BRIEF.md round 15 (assets/art/board/) is used once the files are in the
// repository: the cork, the frame, the pins, the yarn, the index card, tape and a coffee ring.
// Until then the same things are drawn in CSS and SVG.

export interface BoardLine { id: string; text: string; icon?: string }
export interface BoardCard {
  id: string;
  stamp: string; title: string;
  kind?: 'paper' | 'photo' | 'map' | 'sealed' | 'finding' | 'thing';
  have: boolean; where?: string;          // not collected yet: where it is
  image?: string;
  text?: string;                          // a line about what it is
  readingText?: string;                   // the complete description behind a shortened excerpt
  full?: string;                          // the whole text behind TEXT
  lines?: BoardLine[];                    // the lines that matter, each with a pin; none: one pin
  doc?: string;                           // the paper in the journal it stands for
  region?: string;                        // the patch of cork it lies on (the chapter's, by default)
  x?: number; y?: number; w?: number;     // in its region; without: the board finds a free place
}
export interface BoardQuestion {
  id: string; stamp: string; text: string; slots: number;
  filled: string[];                       // what fills the slots, in order
  done: string | null;                    // the supported finding once recorded
  elsewhere?: string;                     // recorded somewhere else: what to do
  region?: string; x: number; y: number; w?: number;
}
export interface BoardNote { id: string; text: string; from: [string, string]; region?: string; /** in pencil, in the player's hand, on a scrap on the thread (story/sameThread.ts) */ hand?: boolean }
export interface BoardThread { a: string; b: string; thin?: boolean; auto?: boolean }
export interface BoardRegion { id: string; label: string; x: number; y: number; w: number; h: number }
type Result = { ok: boolean; text: string };
export interface BoardSpec {
  title: string;
  status: () => string;
  size: { w: number; h: number };
  regions: () => BoardRegion[];           // the patches of cork that have something on them
  focus: string;                          // the region the board opens on
  cards: () => BoardCard[];
  questions: () => BoardQuestion[];
  notes: () => BoardNote[];
  threads: () => BoardThread[];
  connect: (a: string, b: string) => Result;
  record: (q: string) => Result;
  hint: (q: string) => string;
  layout: { get(id: string): [number, number] | undefined; set(id: string, x: number, y: number): void };
  onClose?: () => void;
}

// ---------- the art (round 15), picked up when it is there ----------
const FILES = import.meta.glob<string>('../assets/art/board/*.{jpg,png,webp}', { eager: true, import: 'default' });
const art = (name: string) => FILES[`../assets/art/board/${name}`] ?? null;
/** Which of the board's pictures the build has (the tests). */
export const boardArt = () => Object.keys(FILES).map((k) => k.split('/').pop()!);

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
/** A line that starts with "~ " is pencil, in the hand (as on the papers, UI.document). */
const pencil = (t: string) => esc(t).replace(/^~ (.*)$/gm, '<span class="pencil">$1</span>').replace(/\n/g, '<br>');
const paras = (t: string) => t.split('\n\n').map((p) => `<p>${esc(p).replace(/\n/g, '<br>')}</p>`).join('');
const small = () => innerHeight < 560 || innerWidth < 700;
const coarse = () => matchMedia('(pointer: coarse)').matches;
const clamp = (v: number, a: number, b: number) => Math.min(b, Math.max(a, v));

export function board(ui: UI, o: BoardSpec) {
  const W = o.size.w, H = o.size.h;
  const el = document.createElement('div');
  el.className = 'overlay board-overlay';
  // the round 15 files, as CSS variables the stylesheet uses when they are set
  const vars: [string, string | null][] = [['--board-cork', art('tex_cork.jpg')], ['--board-frame', art('frame_wood.png')],
    ['--pin-red', art('pin_red.png')], ['--pin-yellow', art('pin_yellow.png')], ['--pin-blue', art('pin_blue.png')],
    ['--board-card', art('card_index_blank.png')], ['--board-tape', art('tape_strip.png')], ['--board-stain', art('stain_coffee.png')]];
  for (const [k, v] of vars) if (v) { el.style.setProperty(k, `url("${v}")`); el.classList.add('art-' + k.replace(/^--(board-)?/, '')); }
  const yarn = art('thread_red.png');
  el.innerHTML = `
    <div class="board-wrap" role="dialog" aria-label="${esc(o.title)}">
      <div class="board-head">
        <h3>${esc(o.title)}</h3>
        <p class="board-status" data-status></p>
        <div class="board-zoom" role="group" aria-label="Zoom">
          <button data-z="out" aria-label="Zoom out">-</button><button data-z="all" aria-label="The whole board">ALL</button><button data-z="in" aria-label="Zoom in">+</button>
        </div>
        <button class="lp-close" data-a="close">Close</button>
      </div>
      <p class="board-how"></p>
      <div class="board-view" data-view>
        <div class="board" data-board style="width:${W}px;height:${H}px">
          <div class="board-frame"></div>
          <i class="board-stain" style="left:${Math.round(W * 0.43)}px;top:${Math.round(H * 0.47)}px"></i>
          <div data-labels></div>
          <svg class="board-threads" viewBox="0 0 ${W} ${H}" width="${W}" height="${H}" aria-hidden="true"><g data-threads></g><g data-slack></g><path data-live class="thread live" d=""/></svg>
        </div>
      </div>
      <p class="board-say" data-say aria-live="polite"></p>
    </div>`;
  el.querySelector('[data-a=close]')!.addEventListener('click', () => ui.close());
  const view = el.querySelector('[data-view]') as HTMLElement;
  const bd = el.querySelector('[data-board]') as HTMLElement;
  const labels = el.querySelector('[data-labels]') as HTMLElement;
  const tg = el.querySelector('[data-threads]') as SVGGElement;
  const sg = el.querySelector('[data-slack]') as SVGGElement;   // threads falling slack, apart from the redraws
  const live = el.querySelector('[data-live]') as SVGPathElement;
  const sayEl = el.querySelector('[data-say]') as HTMLElement;
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  const howEl = el.querySelector('.board-how') as HTMLElement;
  const open = new Set<string>();       // cards with their whole text out
  const zs = new Map<string, number>();  // the cards moved last lie on top, also when drawn again
  let selected: string | null = null;
  let top = 10;                          // the card last moved lies on top
  let s = 1, tx = 0, ty = 0;             // the view: screen = t + s * board

  const say = (r: Result | string) => {
    const text = typeof r === 'string' ? r : r.text;
    sayEl.textContent = text;
    sayEl.classList.toggle('bad', typeof r !== 'string' && !r.ok);
    sayEl.classList.toggle('good', typeof r !== 'string' && r.ok);
  };
  const regionOf = (id: string) => o.regions().find((r) => r.id === id);

  // ---------- drawing ----------
  // where a box lies: as the player left it, else where its chapter puts it, else free (null)
  const placeOf = (id: string, region?: string, x?: number, y?: number) => {
    const saved = o.layout.get(id);
    if (saved) return { x: saved[0], y: saved[1] };
    const r = region ? regionOf(region) : undefined;
    if (r && x !== undefined && y !== undefined) return { x: r.x + x, y: r.y + y };
    return null;
  };
  const box = (id: string, region: string | undefined, x: number | undefined, y: number | undefined, w: number) => {
    const p = placeOf(id, region, x, y);
    const z = zs.has(id) ? `;z-index:${zs.get(id)}` : '';
    return p ? `data-id="${id}" style="left:${p.x}px;top:${p.y}px;width:${w}px${z}"` : `data-id="${id}" data-free="${region ?? ''}" style="left:0;top:0;width:${w}px;visibility:hidden"`;
  };
  const cardHtml = (c: BoardCard) => {
    const w = c.w ?? (c.kind === 'photo' || c.kind === 'map' ? 220 : 240);
    if (!c.have) return `<div class="bcard away" data-card="${c.id}" ${box(c.id, c.region, c.x, c.y, w)}>
      <div class="bcard-head"><b>${esc(c.stamp)}</b><span>${esc(c.title)}</span></div>
      <p class="bcard-where">Not on the table yet. ${esc(c.where ?? '')}</p></div>`;
    const plain = !c.lines?.length;
    return `<div class="bcard k-${c.kind ?? 'paper'}${plain ? ' plain' : ''}${open.has(c.id) ? ' open' : ''}" data-card="${c.id}" ${plain ? `data-node="${c.id}"` : ''} ${box(c.id, c.region, c.x, c.y, w)}>
      ${plain ? `<i class="pin" data-pull="${c.id}"></i>` : ''}${c.kind === 'photo' ? '<i class="tape"></i>' : ''}
      <div class="bcard-head"><b>${esc(c.stamp)}</b><span>${esc(c.title)}</span>${c.full ? '<button class="bcard-more" data-more>TEXT</button>' : ''}</div>
      ${c.image ? `<img class="bcard-img" alt="" src="${c.image}" draggable="false">` : ''}
      ${c.text ? `<p class="bcard-sum"${c.readingText ? ` data-reading-text="${esc(c.readingText)}"` : ''}>${esc(c.text)}</p>` : ''}
      <div class="bcard-full">${paras(c.full ?? '')}</div>
      ${(c.lines ?? []).map((l) => `<div class="bline" data-node="${l.id}" data-pull="${l.id}">${l.icon ? `<img alt="" src="${l.icon}" draggable="false">` : ''}<span>${esc(l.text)}</span><i class="pin"></i></div>`).join('')}
    </div>`;
  };
  const questionHtml = (q: BoardQuestion) => {
    const ready = !q.done && q.slots > 0 && q.filled.length >= q.slots;
    const slots = Array.from({ length: q.slots }, (_, i) => `<li class="${q.filled[i] ? 'on' : ''}">${q.filled[i] ? esc(q.filled[i]) : '&nbsp;'}</li>`).join('');
    const count = q.done ? '<span>RECORDED</span>' : q.slots ? `<span class="bq-count">${q.filled.length} / ${q.slots}</span>` : '<span class="bq-count">OPEN</span>';
    return `<div class="bq${q.done ? ' done' : ''}${ready ? ' ready' : ''}" data-q="${q.id}" ${q.done ? `data-node="${q.id}"` : ''} ${box(q.id, q.region, q.x, q.y, q.w ?? 260)}>
      ${q.done ? `<i class="pin pin-blue" data-pull="${q.id}"></i>` : ''}
      <div class="bq-head"><b>${esc(q.stamp)}</b>${count}</div>
      <p class="bq-text">${esc(q.text)}</p>
      ${q.done ? `<p class="bq-done">${esc(q.done)}</p>`
        : q.slots ? `<ol class="bq-slots">${slots}</ol>
          <div class="bq-row">${ready ? `<button class="lp-btn primary bq-record" data-record="${q.id}">RECORD</button>` : ''}<button class="lp-btn bq-hint" data-hint="${q.id}">Hint</button></div>`
          : `<p class="bq-else">${pencil(q.elsewhere ?? '')}</p><div class="bq-row"><button class="lp-btn bq-hint" data-hint="${q.id}">Hint</button></div>`}
    </div>`;
  };

  // the centre of a pin, in board coordinates
  const toBoardXY = (cx: number, cy: number) => { const v = view.getBoundingClientRect(); return { x: (cx - v.left - tx) / s, y: (cy - v.top - ty) / s }; };
  const centre = (id: string) => {
    const n = bd.querySelector(`[data-node="${id}"]`) as HTMLElement | null;
    if (!n || n.closest('[data-free]')) return null;
    const pin = (n.querySelector(':scope > .pin') as HTMLElement | null) ?? n;
    const r = pin.getBoundingClientRect();
    return toBoardXY(r.left + r.width / 2, r.top + r.height / 2);
  };

  // A box without a place gets the first free spot in its chapter's patch, and keeps it.
  const placeFree = () => {
    const taken = [...bd.querySelectorAll<HTMLElement>('[data-id]:not([data-free])')].map((b) => ({ x: b.offsetLeft, y: b.offsetTop, w: b.offsetWidth, h: b.offsetHeight }));
    for (const b of bd.querySelectorAll<HTMLElement>('[data-free]')) {
      const r = regionOf(b.dataset.free!) ?? o.regions()[0] ?? { x: 20, y: 20, w: W - 40, h: H - 40 };
      const w = b.offsetWidth, h = b.offsetHeight, gap = 14;
      let at: { x: number; y: number } | null = null;
      for (let y = r.y + 14; !at && y + Math.min(h, 120) <= r.y + r.h; y += 18) {
        for (let x = r.x + 14; x + w <= r.x + r.w; x += 18) {
          if (!taken.some((t) => x < t.x + t.w + gap && x + w + gap > t.x && y < t.y + t.h + gap && y + h + gap > t.y)) { at = { x, y }; break; }
        }
      }
      at ??= { x: r.x + r.w - w - 14 - (taken.length % 6) * 12, y: r.y + 14 + (taken.length % 6) * 22 };
      b.style.left = `${at.x}px`; b.style.top = `${at.y}px`; b.style.visibility = '';
      b.removeAttribute('data-free');
      taken.push({ x: at.x, y: at.y, w, h });
      o.layout.set(b.dataset.id!, at.x, at.y);
    }
  };

  const render = () => {
    statusEl.textContent = o.status();
    howEl.textContent = coarse()
      ? 'Drag a card to move it. Drag the cork to look around; pinch to zoom. Tap one piece of evidence, then another, to tie a red thread between them.'
      : 'Drag the cards about. Pull a red thread from a pin to another piece of evidence, or click one and then the other.';
    labels.innerHTML = o.regions().filter((r) => r.label).map((r) => `<div class="board-label" style="left:${r.x}px;top:${r.y - 34}px">${esc(r.label)}</div>`).join('');
    bd.querySelectorAll('.bcard, .bnote, .bq').forEach((n) => n.remove());
    bd.insertAdjacentHTML('beforeend', o.questions().map(questionHtml).join('') + o.cards().map(cardHtml).join(''));
    placeFree();
    // notes that answer no question stay on the board: near the two pins they came from, in the
    // nearest place where they cover no card (a note on top of a card would hide its pin)
    for (const n of o.notes()) {
      const saved = placeOf(n.id);
      bd.insertAdjacentHTML('beforeend', `<div class="bnote${n.hand ? ' hand' : ''}" data-note="${n.id}" data-id="${n.id}" style="left:${saved?.x ?? 0}px;top:${saved?.y ?? 0}px${saved ? '' : ';visibility:hidden'}">${n.hand ? '' : '<i class="pin pin-yellow"></i>'}<p>${esc(n.text)}</p></div>`);
      if (saved) continue;
      const el = bd.querySelector(`[data-note="${n.id}"]`) as HTMLElement;
      const a = centre(n.from[0]), b = centre(n.from[1]);
      const want = a && b ? { x: (a.x + b.x) / 2 - 95, y: (a.y + b.y) / 2 + 18 } : { x: 60, y: 60 };
      const w = el.offsetWidth, h = el.offsetHeight, gap = 10;
      const taken = [...bd.querySelectorAll<HTMLElement>('.bcard, .bq, .bnote')].filter((x) => x !== el)
        .map((x) => ({ x: x.offsetLeft, y: x.offsetTop, w: x.offsetWidth, h: x.offsetHeight }));
      const free = (x: number, y: number) => x >= 4 && y >= 4 && x + w <= W - 4 && y + h <= H - 4
        && !taken.some((t) => x < t.x + t.w + gap && x + w + gap > t.x && y < t.y + t.h + gap && y + h + gap > t.y);
      let at: { x: number; y: number } | null = null;
      // a word in pencil hangs on the yarn itself (the thread's own curve): where it covers least of
      // the cards, nearest the middle, and never a pin
      if (n.hand && a && b) {
        const q = curve(a, b);
        const pins = [...bd.querySelectorAll<HTMLElement>('.pin')].map((p) => { const r = p.getBoundingClientRect(); return toBoardXY(r.left + r.width / 2, r.top + r.height / 2); });
        let best = Infinity;
        for (let k = 0.1; k <= 0.9001; k += 0.04) {
          const x = (1 - k) * (1 - k) * q.a.x + 2 * (1 - k) * k * q.c.x + k * k * q.b.x, y = (1 - k) * (1 - k) * q.a.y + 2 * (1 - k) * k * q.c.y + k * k * q.b.y;
          const bx = Math.round(x - w / 2), by = Math.round(y - h * 0.3);
          if (bx < 4 || by < 4 || bx + w > W - 4 || by + h > H - 4) continue;
          if (pins.some((p) => p.x > bx - 12 && p.x < bx + w + 12 && p.y > by - 12 && p.y < by + h + 12)) continue;
          const cover = taken.reduce((sum, t) => sum + Math.max(0, Math.min(bx + w, t.x + t.w) - Math.max(bx, t.x)) * Math.max(0, Math.min(by + h, t.y + t.h) - Math.max(by, t.y)), 0);
          const score = cover + Math.abs(k - 0.5) * 40;
          if (score < best) { best = score; at = { x: bx, y: by }; }
        }
      }
      for (let r = 0; !at && r <= 900; r += 24) {
        for (let k = 0, m = Math.max(1, Math.round(r / 12)); k < m; k++) {
          const ang = (k / m) * Math.PI * 2, x = Math.round(want.x + Math.cos(ang) * r), y = Math.round(want.y + Math.sin(ang) * r);
          if (free(x, y)) { at = { x, y }; break; }
        }
      }
      at ??= { x: Math.round(clamp(want.x, 4, W - w - 4)), y: Math.round(clamp(want.y, 4, H - h - 4)) };
      el.style.left = `${at.x}px`; el.style.top = `${at.y}px`; el.style.visibility = '';
      o.layout.set(n.id, at.x, at.y);
    }
    if (selected) bd.querySelector(`[data-node="${selected}"]`)?.classList.add('sel');
    bd.querySelectorAll<HTMLButtonElement>('[data-more]').forEach((b) => b.addEventListener('click', (e) => {
      e.stopPropagation();
      const c = b.closest('.bcard') as HTMLElement;
      c.classList.toggle('open');
      if (c.classList.contains('open')) { open.add(c.dataset.card!); zs.set(c.dataset.id!, ++top); c.style.zIndex = String(top); } else open.delete(c.dataset.card!);
      threads();
    }));
    bd.querySelectorAll<HTMLButtonElement>('[data-hint]').forEach((b) => b.addEventListener('click', (e) => { e.stopPropagation(); say(o.hint(b.dataset.hint!)); }));
    bd.querySelectorAll<HTMLButtonElement>('[data-record]').forEach((b) => b.addEventListener('click', (e) => {
      e.stopPropagation();
      const r = o.record(b.dataset.record!);
      say(r);
      if (r.ok) render();
    }));
    threads();
  };

  // ---------- the yarn ----------
  const curve = (a: { x: number; y: number }, b: { x: number; y: number }, slack = 0.12, max = 60) => {
    const sag = Math.min(max, Math.hypot(b.x - a.x, b.y - a.y) * slack);
    return { a, c: { x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 + sag }, b };
  };
  const pathOf = (q: ReturnType<typeof curve>) => `M${q.a.x.toFixed(1)},${q.a.y.toFixed(1)} Q${q.c.x.toFixed(1)},${q.c.y.toFixed(1)} ${q.b.x.toFixed(1)},${q.b.y.toFixed(1)}`;
  // with the yarn picture: short stretched pieces of it along the curve, over a plain line
  const yarnOf = (q: ReturnType<typeof curve>, thin: boolean) => {
    if (!yarn) return '';
    const pt = (t: number) => ({ x: (1 - t) * (1 - t) * q.a.x + 2 * (1 - t) * t * q.c.x + t * t * q.b.x, y: (1 - t) * (1 - t) * q.a.y + 2 * (1 - t) * t * q.c.y + t * t * q.b.y });
    let out = '', p = pt(0);
    const h = thin ? 3 : 6;
    for (let i = 1; i <= 12; i++) {
      const n = pt(i / 12), len = Math.hypot(n.x - p.x, n.y - p.y) + 1;
      out += `<image class="yarn" href="${yarn}" x="0" y="${-h / 2}" width="${len.toFixed(1)}" height="${h}" preserveAspectRatio="none" transform="translate(${p.x.toFixed(1)},${p.y.toFixed(1)}) rotate(${(Math.atan2(n.y - p.y, n.x - p.x) * 180 / Math.PI).toFixed(1)})"/>`;
      p = n;
    }
    return out;
  };
  const threads = () => {
    let out = '';
    for (const t of o.threads()) {
      const pa = centre(t.a), pb = centre(t.b);
      if (!pa || !pb) continue;
      const q = curve(pa, pb);
      out += `<path class="thread${t.thin ? ' thin' : ''}${t.auto ? ' auto' : ''}" d="${pathOf(q)}"/>${yarnOf(q, !!t.thin)}<circle class="tack" cx="${pa.x.toFixed(1)}" cy="${pa.y.toFixed(1)}" r="4.5"/><circle class="tack" cx="${pb.x.toFixed(1)}" cy="${pb.y.toFixed(1)}" r="4.5"/>`;
    }
    tg.innerHTML = out;
  };
  // a thread that does not hold: drawn slack, then gone
  const slack = (a: string, b: string) => {
    const pa = centre(a), pb = centre(b);
    if (!pa || !pb) return;
    const p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    p.setAttribute('class', 'thread slack');
    p.setAttribute('d', pathOf(curve(pa, pb, 0.4, 140)));
    sg.appendChild(p);
    setTimeout(() => p.remove(), 1400);
  };

  const link = (a: string, b: string) => {
    selected = null;
    bd.querySelectorAll('.sel').forEach((n) => n.classList.remove('sel'));
    const r = o.connect(a, b);
    say(r);
    if (r.ok) render(); else slack(a, b);
    return r;
  };
  const tapNode = (n: string) => {
    if (selected && selected !== n) { link(selected, n); return; }
    selected = selected === n ? null : n;
    bd.querySelectorAll('.sel').forEach((x) => x.classList.remove('sel'));
    if (selected) { bd.querySelector(`[data-node="${selected}"]`)?.classList.add('sel'); say('Now the piece of evidence it goes with.'); }
  };

  // ---------- the view: pan and zoom ----------
  const apply = () => { bd.style.transform = `translate(${tx.toFixed(1)}px, ${ty.toFixed(1)}px) scale(${s.toFixed(4)})`; };
  const minScale = () => Math.min(0.5, Math.max(0.12, Math.min(view.clientWidth / W, view.clientHeight / H) * 0.95));
  const clampView = () => {
    const vw = view.clientWidth, vh = view.clientHeight, m = 60;
    tx = W * s + 2 * m <= vw ? (vw - W * s) / 2 : clamp(tx, vw - W * s - m, m);
    ty = H * s + 2 * m <= vh ? (vh - H * s) / 2 : clamp(ty, vh - H * s - m, m);
    apply();
  };
  const zoomAt = (ns: number, cx: number, cy: number) => {
    ns = clamp(ns, minScale(), 1.6);
    tx = cx - (cx - tx) * ns / s; ty = cy - (cy - ty) * ns / s; s = ns;
    clampView();
    if (drag?.kind !== 'pan') follow();
  };
  // a patch of the board in view: the whole of it if it fits at a size that can be read,
  // else from its top left corner (a phone opens the board at a readable size)
  const look = (r: { x: number; y: number; w: number; h: number }) => {
    const vw = view.clientWidth, vh = view.clientHeight;
    if (!vw || !vh) return;
    s = small() ? 0.8 : clamp(Math.min(vw / (r.w + 24), vh / (r.h + 24)), 0.45, 1.25);
    tx = r.w * s <= vw ? vw / 2 - (r.x + r.w / 2) * s : 12 - r.x * s;
    ty = r.h * s <= vh ? vh / 2 - (r.y + r.h / 2) * s : 12 - (r.y - 30) * s;
    clampView();
  };
  el.querySelectorAll<HTMLButtonElement>('[data-z]').forEach((b) => b.addEventListener('click', () => {
    const vw = view.clientWidth, vh = view.clientHeight;
    if (b.dataset.z === 'all') { s = minScale(); clampView(); } else zoomAt(s * (b.dataset.z === 'in' ? 1.25 : 0.8), vw / 2, vh / 2);
    threads();
  }));
  // the view only moves by its transform; a focused button must not scroll it out of step
  view.addEventListener('scroll', () => { view.scrollLeft = 0; view.scrollTop = 0; });
  // the whole text of an opened card scrolls by itself when it is longer than the card
  const scrolling = (t: EventTarget | null) => {
    const f = (t as HTMLElement | null)?.closest?.('.bcard.open .bcard-full') as HTMLElement | null;
    return !!f && f.scrollHeight > f.clientHeight + 2;
  };
  view.addEventListener('wheel', (e) => {
    if (!e.ctrlKey && !e.metaKey && scrolling(e.target)) return;
    e.preventDefault();
    const v = view.getBoundingClientRect();
    if (e.ctrlKey || e.metaKey) zoomAt(s * Math.exp(-e.deltaY * 0.0025), e.clientX - v.left, e.clientY - v.top);
    else { tx -= e.shiftKey ? e.deltaY : e.deltaX; ty -= e.shiftKey ? 0 : e.deltaY; clampView(); }
  }, { passive: false });

  // ---------- pointer: threads, taps, moving cards, panning, pinching ----------
  type Drag = { kind: 'thread'; from: string; sx: number; sy: number; x: number; y: number; moved: boolean }
    | { kind: 'card'; box: HTMLElement; key: string; node: string | null; sx: number; sy: number; x: number; y: number; gx: number; gy: number; moved: boolean }
    | { kind: 'pan'; sx: number; sy: number; x: number; y: number; tx0: number; ty0: number; moved: boolean };
  let drag: Drag | null = null;
  const ptrs = new Map<number, { x: number; y: number }>();
  let pinch: { d0: number; s0: number; mx: number; my: number } | null = null;
  let changed = false;
  // where the drag stands, again after the view has moved under it
  const follow = () => {
    if (!drag) return;
    if (drag.kind === 'card' && drag.moved) {
      const p = toBoardXY(drag.x, drag.y);
      const x = clamp(p.x - drag.gx, 0, W - 60), y = clamp(p.y - drag.gy, 0, H - 40);
      drag.box.style.left = `${x}px`; drag.box.style.top = `${y}px`;
      threads();
    } else if (drag.kind === 'thread' && drag.moved) {
      const a = centre(drag.from);
      if (a) live.setAttribute('d', pathOf(curve(a, toBoardXY(drag.x, drag.y))));
    }
  };
  // near the edge with a card or a thread, the board moves the other way
  let edgeRaf = 0;
  const edge = () => {
    edgeRaf = 0;
    if (!drag || drag.kind === 'pan' || !drag.moved) return;
    const v = view.getBoundingClientRect(), m = 36;
    const vx = drag.x < v.left + m ? 1 : drag.x > v.right - m ? -1 : 0;
    const vy = drag.y < v.top + m ? 1 : drag.y > v.bottom - m ? -1 : 0;
    if (!vx && !vy) return;
    tx += vx * 10; ty += vy * 10; clampView(); follow();
    edgeRaf = requestAnimationFrame(edge);
  };

  view.addEventListener('pointerdown', (e) => {
    const t = e.target as HTMLElement;
    if (t.closest('button')) return;
    if (e.pointerType === 'touch' && scrolling(t)) return;   // a finger reading down a long text
    e.preventDefault();
    ptrs.set(e.pointerId, { x: e.clientX, y: e.clientY });
    view.setPointerCapture(e.pointerId);
    if (ptrs.size === 2) {
      // two fingers: zoom about the point between them; a card in hand stays where it is
      if (drag?.kind === 'card' && drag.moved) { o.layout.set(drag.key, parseFloat(drag.box.style.left), parseFloat(drag.box.style.top)); changed = true; }
      drag = null; live.setAttribute('d', '');
      const [p, q] = [...ptrs.values()], v = view.getBoundingClientRect();
      pinch = { d0: Math.hypot(q.x - p.x, q.y - p.y) || 1, s0: s, mx: (p.x + q.x) / 2 - v.left, my: (p.y + q.y) / 2 - v.top };
      return;
    }
    if (ptrs.size > 2) return;
    const base = { sx: e.clientX, sy: e.clientY, x: e.clientX, y: e.clientY, moved: false };
    const pull = t.closest('[data-pull]') as HTMLElement | null;
    const b = t.closest('.bcard, .bq, .bnote') as HTMLElement | null;
    if (pull) drag = { kind: 'thread', from: pull.dataset.pull!, ...base };
    else if (b) {
      const p = toBoardXY(e.clientX, e.clientY);
      drag = { kind: 'card', box: b, key: b.dataset.id!, node: b.dataset.node ?? null, gx: p.x - b.offsetLeft, gy: p.y - b.offsetTop, ...base };
    } else drag = { kind: 'pan', tx0: tx, ty0: ty, ...base };
  });
  view.addEventListener('pointermove', (e) => {
    if (!ptrs.has(e.pointerId)) return;
    ptrs.set(e.pointerId, { x: e.clientX, y: e.clientY });
    if (pinch && ptrs.size >= 2) {
      const [p, q] = [...ptrs.values()], v = view.getBoundingClientRect();
      const mx = (p.x + q.x) / 2 - v.left, my = (p.y + q.y) / 2 - v.top;
      tx += mx - pinch.mx; ty += my - pinch.my; pinch.mx = mx; pinch.my = my;
      zoomAt(pinch.s0 * Math.hypot(q.x - p.x, q.y - p.y) / pinch.d0, mx, my);
      threads();
      return;
    }
    if (!drag) return;
    drag.x = e.clientX; drag.y = e.clientY;
    const far = Math.hypot(e.clientX - drag.sx, e.clientY - drag.sy);
    if (!drag.moved) {
      if (far < (drag.kind === 'thread' ? 8 : 6)) return;
      drag.moved = true;
      if (drag.kind === 'card') { drag.box.classList.add('lift'); zs.set(drag.key, ++top); drag.box.style.zIndex = String(top); }
    }
    if (drag.kind === 'pan') { tx = drag.tx0 + e.clientX - drag.sx; ty = drag.ty0 + e.clientY - drag.sy; clampView(); return; }
    follow();
    if (!edgeRaf) edgeRaf = requestAnimationFrame(edge);
  });
  const end = (e: PointerEvent) => {
    if (!ptrs.delete(e.pointerId)) return;
    if (pinch) { if (ptrs.size < 2) pinch = null; return; }
    const d = drag; drag = null;
    live.setAttribute('d', '');
    if (!d || e.type === 'pointercancel') return;
    if (d.kind === 'pan') {
      if (!d.moved && selected) { selected = null; bd.querySelectorAll('.sel').forEach((n) => n.classList.remove('sel')); say(''); }
      return;
    }
    if (d.kind === 'card') {
      d.box.classList.remove('lift');
      if (d.moved) { o.layout.set(d.key, parseFloat(d.box.style.left), parseFloat(d.box.style.top)); changed = true; return; }
      if (d.node) tapNode(d.node);
      else if (d.box.querySelector('.bline')) say('Each line that matters on this record has its own pin. Tap the line, then what it goes with.');
      return;
    }
    if (d.moved) {
      const under = document.elementFromPoint(e.clientX, e.clientY) as HTMLElement | null;
      const to = (under?.closest('[data-node]') as HTMLElement | null)?.dataset.node;
      if (to && to !== d.from) link(d.from, to);
      return;
    }
    tapNode(d.from);   // a tap on a line or a pin: choose one end, then the other
  };
  view.addEventListener('pointerup', end);
  view.addEventListener('pointercancel', end);

  const ro = new ResizeObserver(() => { clampView(); threads(); });
  ro.observe(view);
  ui.open(el, () => { ro.disconnect(); cancelAnimationFrame(edgeRaf); o.onClose?.(); });
  render();
  const focus = regionOf(o.focus) ?? o.regions()[0];
  if (focus) look(focus); else { s = minScale(); clampView(); }
  threads();
  if (small()) say(howEl.textContent ?? '');
  return {
    el, render, link, say,
    /** the view, for the tests: scale and offset */
    view: () => ({ s, tx, ty }),
    look: (id: string) => { const r = regionOf(id); if (r) { look(r); threads(); } },
    /** pins and boxes in view together, as large as they fit (story/sameThread.ts: the thread one
     *  did not tie, the first time the board is opened with it); false when one is not on the board */
    frame: (ids: string[]) => {
      const pts = ids.map((id) => {
        const c = centre(id); if (c) return [c];
        const b = bd.querySelector(`[data-id="${id}"]`) as HTMLElement | null;
        return b && !b.dataset.free ? [{ x: b.offsetLeft, y: b.offsetTop }, { x: b.offsetLeft + b.offsetWidth, y: b.offsetTop + b.offsetHeight }] : null;
      });
      if (pts.some((p) => !p)) return false;
      const all = pts.flat() as { x: number; y: number }[];
      const x0 = Math.min(...all.map((p) => p.x)), x1 = Math.max(...all.map((p) => p.x)), y0 = Math.min(...all.map((p) => p.y)), y1 = Math.max(...all.map((p) => p.y));
      const vw = view.clientWidth, vh = view.clientHeight;
      s = clamp(Math.min(vw / (x1 - x0 + 160), vh / (y1 - y0 + 200)), Math.max(minScale(), 0.32), 1.0);
      tx = vw / 2 - (x0 + x1) / 2 * s; ty = vh / 2 - (y0 + y1) / 2 * s;
      clampView(); threads();
      return true;
    },
    moved: () => changed,
    /** brings a pin to the middle of the view (the tests; a hint could use it) */
    show: (id: string) => {
      const c = centre(id);
      if (!c) return false;
      tx = view.clientWidth / 2 - c.x * s; ty = view.clientHeight / 2 - c.y * s;
      clampView(); threads();
      return true;
    },
  };
}
