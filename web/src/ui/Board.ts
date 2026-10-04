import type { UI } from './UI';

// The evidence board: the records lie on a cork board as cards, each with the lines that
// matter on it. The player pulls a red thread from a line on one card to a line on
// another that says something about it. A thread has one of three outcomes:
//   holds       it stays, red; what the two lines show together goes into the slot on the
//               question it belongs to by itself (or, if it answers nothing, it stays on
//               the board as a note, or as a thin thread for plain background)
//   contradicts it falls slack and the reply says why (the Unity replies); counts as wrong
//   empty       "those two say nothing about each other"; falls slack, counts for nothing
// When every slot on a question is filled, RECORD lights up, and one press records the
// finding. Mouse and touch: drag from a line to a line, or tap one and then the other.
// Cards can be moved by their heads. All the rules live in the chapter (`connect`,
// `record`); this file only draws. After SPILLDESIGN.md, "Bevisbordet".
//
// The board has a fixed size of its own (W x H) and is scaled to the window. On a small
// screen (under 700 px high or wide) it becomes one column instead: the questions on top,
// the cards in a list, tap one line and then the other, and the links shown as tags.

export interface BoardLine { id: string; text: string; icon?: string }
export interface BoardCard {
  id: string; stamp: string; title: string;
  have: boolean; where: string;            // not collected yet: where it is
  image?: string; full?: string;           // a picture on the card, the whole text behind TEXT
  lines: BoardLine[];
  x: number; y: number; w?: number;
}
export interface BoardQuestion { id: string; stamp: string; text: string; slots: number; x: number; y: number; w?: number }
export interface BoardNote { id: string; text: string; from: [string, string] }
export interface BoardState {
  notes: BoardNote[];                      // notes that answer no question, on the board
  threads: { a: string; b: string; thin?: boolean }[];
  slots: (q: string) => string[];          // what fills the question's slots, in order
  done: (q: string) => string | null;      // the supported finding once recorded
}
export interface BoardSpec {
  key: string;                             // remembers moved cards while the page lives
  title: string;
  status: () => string;
  cards: BoardCard[];
  questions: BoardQuestion[];
  state: () => BoardState;
  connect: (a: string, b: string) => { ok: boolean; text: string };
  record: (q: string) => { ok: boolean; text: string };
  hint: (q: string) => string;
  noteAt?: (id: string) => { x: number; y: number } | undefined;   // where a note goes
}

const W = 1180, H = 760;
const moved = new Map<string, { x: number; y: number }>();   // key/id -> board position
const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
const small = () => innerHeight < 700 || innerWidth < 700;

export function board(ui: UI, o: BoardSpec) {
  const el = document.createElement('div');
  el.className = 'overlay board-overlay';
  el.innerHTML = `
    <div class="board-wrap" role="dialog" aria-label="${esc(o.title)}">
      <div class="board-head">
        <h3>${esc(o.title)}</h3>
        <p class="board-status" data-status></p>
        <button class="lp-close" data-a="close">Close</button>
      </div>
      <p class="board-how"></p>
      <div class="board-scroll"><div class="board-size"><div class="board" data-board>
        <svg class="board-threads" viewBox="0 0 ${W} ${H}" width="${W}" height="${H}" aria-hidden="true"><g data-threads></g><path data-live class="thread live" d=""/></svg>
      </div></div></div>
      <p class="board-say" data-say aria-live="polite"></p>
    </div>`;
  el.querySelector('[data-a=close]')!.addEventListener('click', () => ui.close());
  const scroller = el.querySelector('.board-scroll') as HTMLElement;
  const sizer = el.querySelector('.board-size') as HTMLElement;
  const bd = el.querySelector('[data-board]') as HTMLElement;
  const tg = el.querySelector('[data-threads]') as SVGGElement;
  const live = el.querySelector('[data-live]') as SVGPathElement;
  const sayEl = el.querySelector('[data-say]') as HTMLElement;
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  const howEl = el.querySelector('.board-how') as HTMLElement;
  let scale = 1;
  let col = small();
  let selected: string | null = null;

  const pos = (id: string, x: number, y: number) => moved.get(o.key + '/' + id) ?? { x, y };
  const say = (r: { ok: boolean; text: string } | string) => {
    const text = typeof r === 'string' ? r : r.text;
    sayEl.textContent = text;
    sayEl.classList.toggle('bad', typeof r !== 'string' && !r.ok);
    sayEl.classList.toggle('good', typeof r !== 'string' && r.ok);
  };
  const lineText = (id: string) => { for (const c of o.cards) for (const l of c.lines) if (l.id === id) return l.text; return id; };
  const stampOf = (id: string) => o.cards.find((c) => c.lines.some((l) => l.id === id))?.stamp ?? '';

  // ---------- drawing ----------
  const cardHtml = (c: BoardCard, st: BoardState) => {
    const p = pos(c.id, c.x, c.y);
    const place = col ? '' : `style="left:${p.x}px;top:${p.y}px;width:${c.w ?? 270}px"`;
    if (!c.have) return `<div class="bcard away" data-card="${c.id}" ${place}>
      <div class="bcard-head" data-drag="${c.id}"><b>${esc(c.stamp)}</b><span>${esc(c.title)}</span></div>
      <p class="bcard-where">Not on the table yet. ${esc(c.where)}</p></div>`;
    // on one column there are no lines to see, so each linked line names its partner
    const tags = (id: string) => !col ? '' : st.threads.filter((t) => t.a === id || t.b === id)
      .map((t) => { const other = t.a === id ? t.b : t.a; return `<em class="btag${t.thin ? ' thin' : ''}">${esc(stampOf(other))}</em>`; }).join('');
    return `<div class="bcard" data-card="${c.id}" ${place}>
      <div class="bcard-head" data-drag="${c.id}"><b>${esc(c.stamp)}</b><span>${esc(c.title)}</span>${c.full ? '<button class="bcard-more" data-more>TEXT</button>' : ''}</div>
      ${c.image ? `<img class="bcard-img" alt="" src="${c.image}">` : ''}
      <div class="bcard-full">${(c.full ?? '').split('\n\n').map((t) => `<p>${esc(t).replace(/\n/g, '<br>')}</p>`).join('')}</div>
      ${c.lines.map((l) => `<div class="bline" data-node="${l.id}">${l.icon ? `<img alt="" src="${l.icon}">` : ''}<span>${esc(l.text)}${tags(l.id)}</span><i class="pin"></i></div>`).join('')}
    </div>`;
  };
  const questionHtml = (q: BoardQuestion, st: BoardState) => {
    const p = pos(q.id, q.x, q.y), done = st.done(q.id), filled = st.slots(q.id);
    const place = col ? '' : `style="left:${p.x}px;top:${p.y}px;width:${q.w ?? 250}px"`;
    const slots = Array.from({ length: q.slots }, (_, i) => `<li class="${filled[i] ? 'on' : ''}">${filled[i] ? esc(filled[i]) : '&nbsp;'}</li>`).join('');
    const ready = !done && filled.length >= q.slots;
    return `<div class="bq${done ? ' done' : ''}${ready ? ' ready' : ''}" data-q="${q.id}" ${place}>
      <div class="bq-head" data-drag="${q.id}"><b>${esc(q.stamp)}</b>${done ? '<span>RECORDED</span>' : `<span class="bq-count">${filled.length} / ${q.slots}</span>`}</div>
      <p class="bq-text">${esc(q.text)}</p>
      ${done ? `<p class="bq-done">${esc(done)}</p>` : `<ol class="bq-slots">${slots}</ol>
        <div class="bq-row">${ready ? `<button class="lp-btn primary bq-record" data-record="${q.id}">RECORD</button>` : ''}<button class="lp-btn bq-hint" data-hint="${q.id}">Hint</button></div>`}
    </div>`;
  };
  const render = () => {
    const st = o.state();
    col = small();
    el.classList.toggle('board-col', col);
    statusEl.textContent = o.status();
    howEl.textContent = col
      ? 'Tap a line, then the line on another record that says something about it.'
      : 'Pull a thread from a line on one record to a line on another that says something about it. What they show together goes to the question.';
    bd.querySelectorAll('.bcard, .bnote, .bq').forEach((n) => n.remove());
    const qs = o.questions.map((q) => questionHtml(q, st)).join('');
    const cards = o.cards.map((c) => cardHtml(c, st)).join('');
    bd.insertAdjacentHTML('beforeend', col ? `<div class="bq-strip">${qs}</div>${cards}` : qs + cards);
    // notes that answer no question stay on the board, between the lines they came from
    let html = '';
    for (const n of st.notes) {
      if (col) { html += `<div class="bnote"><p>${esc(n.text)}</p></div>`; continue; }
      const a = centre(n.from[0]), b = centre(n.from[1]);
      const def = o.noteAt?.(n.id) ?? (a && b ? { x: (a.x + b.x) / 2 - 85, y: (a.y + b.y) / 2 + 18 } : { x: 40, y: 40 });
      const p = pos(n.id, Math.round(Math.min(Math.max(def.x, 4), W - 190)), Math.round(Math.min(Math.max(def.y, 4), H - 90)));
      html += `<div class="bnote" data-note="${n.id}" style="left:${p.x}px;top:${p.y}px"><div class="bnote-head" data-drag="${n.id}"><i class="pin"></i></div><p>${esc(n.text)}</p></div>`;
    }
    bd.insertAdjacentHTML('beforeend', html);
    if (selected) bd.querySelector(`[data-node="${selected}"]`)?.classList.add('sel');
    bd.querySelectorAll<HTMLButtonElement>('[data-more]').forEach((b) => b.addEventListener('click', (e) => {
      e.stopPropagation();
      b.closest('.bcard')!.classList.toggle('open');
      fit(); threads();
    }));
    bd.querySelectorAll<HTMLButtonElement>('[data-hint]').forEach((b) => b.addEventListener('click', (e) => { e.stopPropagation(); say(o.hint(b.dataset.hint!)); }));
    bd.querySelectorAll<HTMLButtonElement>('[data-record]').forEach((b) => b.addEventListener('click', (e) => {
      e.stopPropagation();
      const r = o.record(b.dataset.record!);
      say(r);
      if (r.ok) render();
    }));
    fit();
    threads();
  };

  // the centre of a line's pin in board coordinates
  const centre = (id: string) => {
    const n = bd.querySelector(`[data-node="${id}"]`) as HTMLElement | null;
    if (!n) return null;
    const pin = (n.querySelector(':scope > .pin') as HTMLElement | null) ?? n;
    const r = pin.getBoundingClientRect(), b = bd.getBoundingClientRect();
    return { x: (r.left + r.width / 2 - b.left) / scale, y: (r.top + r.height / 2 - b.top) / scale };
  };
  const yarn = (a: { x: number; y: number }, b: { x: number; y: number }) => {
    const sag = Math.min(60, Math.hypot(b.x - a.x, b.y - a.y) * 0.12);
    return `M${a.x.toFixed(1)},${a.y.toFixed(1)} Q${((a.x + b.x) / 2).toFixed(1)},${((a.y + b.y) / 2 + sag).toFixed(1)} ${b.x.toFixed(1)},${b.y.toFixed(1)}`;
  };
  const threads = () => {
    if (col) { tg.innerHTML = ''; return; }
    let s = '';
    for (const t of o.state().threads) {
      const pa = centre(t.a), pb = centre(t.b);
      if (pa && pb) s += `<path class="thread${t.thin ? ' thin' : ''}" d="${yarn(pa, pb)}"/><circle class="tack" cx="${pa.x}" cy="${pa.y}" r="4.5"/><circle class="tack" cx="${pb.x}" cy="${pb.y}" r="4.5"/>`;
    }
    tg.innerHTML = s;
  };

  // a thread that does not hold: drawn slack, then gone
  const slack = (a: string, b: string) => {
    if (col) return;
    const pa = centre(a), pb = centre(b);
    if (!pa || !pb) return;
    const p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    p.setAttribute('class', 'thread slack');
    const sag = Math.min(140, Math.hypot(pb.x - pa.x, pb.y - pa.y) * 0.4);
    p.setAttribute('d', `M${pa.x},${pa.y} Q${(pa.x + pb.x) / 2},${(pa.y + pb.y) / 2 + sag} ${pb.x},${pb.y}`);
    tg.appendChild(p);
    setTimeout(() => p.remove(), 1400);
  };

  const link = (a: string, b: string) => {
    selected = null;
    bd.querySelectorAll('.sel').forEach((n) => n.classList.remove('sel'));
    const r = o.connect(a, b);
    say(r);
    if (r.ok) render(); else slack(a, b);
  };

  // ---------- fitting the board to the window ----------
  const fit = () => {
    if (col) {
      bd.style.transform = ''; sizer.style.width = ''; sizer.style.height = ''; scale = 1;
      return;
    }
    const w = scroller.clientWidth, h = scroller.clientHeight;
    if (!w || !h) return;
    scale = Math.max(0.68, Math.min(1.25, w / W, h / H));
    bd.style.transform = `scale(${scale})`;
    sizer.style.width = `${W * scale}px`; sizer.style.height = `${H * scale}px`;
  };
  const ro = new ResizeObserver(() => { if (small() !== col) render(); else { fit(); threads(); } });
  ro.observe(scroller);

  // ---------- pointer: threads, taps and moving cards ----------
  type Drag = { kind: 'card'; id: string; el: HTMLElement; sx: number; sy: number; x0: number; y0: number }
    | { kind: 'thread'; from: string; sx: number; sy: number; moved: boolean };
  let drag: Drag | null = null;
  const toBoard = (e: PointerEvent) => { const b = bd.getBoundingClientRect(); return { x: (e.clientX - b.left) / scale, y: (e.clientY - b.top) / scale }; };

  bd.addEventListener('pointerdown', (e) => {
    const t = e.target as HTMLElement;
    if (t.closest('button')) return;
    const handle = col ? null : t.closest('[data-drag]') as HTMLElement | null;
    if (handle) {
      const id = handle.dataset.drag!;
      const box = (handle.closest('[data-card], .bnote, .bq') as HTMLElement);
      drag = { kind: 'card', id, el: box, sx: e.clientX, sy: e.clientY, x0: parseFloat(box.style.left), y0: parseFloat(box.style.top) };
      bd.setPointerCapture(e.pointerId);
      e.preventDefault();
      return;
    }
    const node = t.closest('[data-node]') as HTMLElement | null;
    if (!node) return;
    drag = { kind: 'thread', from: node.dataset.node!, sx: e.clientX, sy: e.clientY, moved: false };
    if (!col) { bd.setPointerCapture(e.pointerId); e.preventDefault(); }
  });
  bd.addEventListener('pointermove', (e) => {
    if (!drag) return;
    if (drag.kind === 'card') {
      const x = Math.min(Math.max(drag.x0 + (e.clientX - drag.sx) / scale, 0), W - 60);
      const y = Math.min(Math.max(drag.y0 + (e.clientY - drag.sy) / scale, 0), H - 40);
      drag.el.style.left = `${x}px`; drag.el.style.top = `${y}px`;
      moved.set(o.key + '/' + drag.id, { x, y });
      threads();
      return;
    }
    if (col) return;   // one column: a finger that moves is scrolling, not pulling a thread
    if (!drag.moved && Math.hypot(e.clientX - drag.sx, e.clientY - drag.sy) < 8) return;
    drag.moved = true;
    const a = centre(drag.from);
    if (a) live.setAttribute('d', yarn(a, toBoard(e)));
  });
  const end = (e: PointerEvent) => {
    if (!drag) return;
    const d = drag; drag = null;
    live.setAttribute('d', '');
    if (d.kind === 'card') return;
    if (col && Math.hypot(e.clientX - d.sx, e.clientY - d.sy) > 10) return;   // that was a scroll
    const under = document.elementFromPoint(e.clientX, e.clientY) as HTMLElement | null;
    const to = (under?.closest('[data-node]') as HTMLElement | null)?.dataset.node;
    if (d.kind === 'thread' && d.moved) { if (to && to !== d.from) link(d.from, to); return; }
    // a tap: choose one end, then the other
    if (selected && selected !== d.from) { link(selected, d.from); return; }
    selected = selected === d.from ? null : d.from;
    bd.querySelectorAll('.sel').forEach((n) => n.classList.remove('sel'));
    if (selected) { bd.querySelector(`[data-node="${selected}"]`)?.classList.add('sel'); say('Now the line on another record that it connects to.'); }
  };
  bd.addEventListener('pointerup', end);
  bd.addEventListener('pointercancel', () => { drag = null; live.setAttribute('d', ''); });

  ui.open(el, () => ro.disconnect());
  render();
  return { el, render, link };
}
