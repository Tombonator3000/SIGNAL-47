import type { UI } from './UI';
import { PRINT, PHOTO_W, PHOTO_H } from '../core/FieldCamera';
import { portraitOf } from '../core/portraits';

// Panels for chapter one: wet bench, contact print, reference file, B-12 control,
// two exposures and the local report. Text inside the game is English (1986).

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
type Result = { ok: boolean; text: string };

function shell(ui: UI, title: string, body: string, onClose?: () => void) {
  const el = document.createElement('div');
  el.className = 'overlay';
  el.innerHTML = `<div class="labpanel" role="dialog" aria-label="${esc(title)}"><div class="lp-head"><h3>${esc(title)}</h3><button class="lp-close" data-a="close">Close</button></div>${body}</div>`;
  el.querySelector('[data-a=close]')!.addEventListener('click', () => ui.close());
  el.addEventListener('click', (e) => { if (e.target === el) ui.close(); });
  ui.open(el, onClose);
  return el;
}
function say(el: HTMLElement, sel: string, r: Result | string) {
  const box = el.querySelector(sel) as HTMLElement | null;
  if (!box) return; // a recorded finding replaces the page that had the reply line
  const text = typeof r === 'string' ? r : r.text;
  box.textContent = text;
  box.classList.toggle('bad', typeof r !== 'string' && !r.ok);
  box.classList.toggle('good', typeof r !== 'string' && r.ok);
}

// ---------- wet bench ----------
export function wetBench(ui: UI, o: { frame: string; step: 0 | 1 | 2 | 3; empty?: string; onAct: () => void }) {
  const steps = [
    ['1 / LOAD LIGHT-TIGHT TANK', 'The exposed film stays protected from room light.'],
    ['2 / TRANSFER PRINT TO FIXER', 'Lift the contact print into the adjacent tray.'],
    ['3 / COLLECT STABLE PRINT', 'The image can now be handled, filed and examined.'],
  ];
  const labels = ['LOAD SEALED FILM INTO TANK', '', 'TRANSFER CONTACT PRINT TO FIXER', 'COLLECT AND EXAMINE PRINT'];
  const active = o.step === 0 ? 0 : o.step === 1 ? 0 : o.step === 2 ? 1 : 2;
  const body = o.empty
    ? `<p class="lp-text">${esc(o.empty)}</p>`
    : `<p class="lp-sub">${esc(o.frame)}</p>
       <ol class="lp-steps">${steps.map(([h, c], i) => `<li class="${i < active ? 'done' : i === active ? 'now' : ''}"><b>${esc(h)}</b><span>${esc(c)}</span></li>`).join('')}</ol>
       ${o.step === 1 ? '<p class="lp-text">The film is in the tank. Wait for the timer.</p>' : `<button class="lp-btn primary" data-a="act">${esc(labels[o.step])}</button>`}`;
  const el = shell(ui, 'WET BENCH', body);
  el.querySelector('[data-a=act]')?.addEventListener('click', () => { ui.close(); o.onAct(); });
}

// ---------- contact print with a loupe ----------
// Zoom 1x to 4x, drag or sliders to move, click to mark. Marking never edits the photograph.
export function printView(ui: UI, o: { title: string; url: string; intro: string; mark?: [number, number]; onMark?: (u: number, v: number) => Result; hints?: string[]; onClose?: () => void; extra?: string }) {
  const el = shell(ui, o.title, `
    <div class="lp-print">
      <div class="loupe"><div class="loupe-inner"><img alt="Contact print" src="${o.url}" draggable="false"><div class="mark" hidden></div></div></div>
      <div class="lp-side">
        <p class="lp-text" data-say>${esc(o.intro)}</p>
        <div class="lp-zoom"><label>Loupe <input type="range" min="1" max="4" step="0.75" value="1" data-z></label>
          <label>X <input type="range" min="0" max="1" step="0.01" value="0.5" data-x></label>
          <label>Y <input type="range" min="0" max="1" step="0.01" value="0.5" data-y></label></div>
        ${o.hints?.length ? '<button class="lp-btn" data-a="hint">Hint</button>' : ''}
        ${o.extra ?? ''}
        <p class="lp-small">${o.onMark ? 'Click to mark a location. Marking never edits the source photograph.' : 'The original photograph is filed.'}</p>
      </div>
    </div>`, o.onClose);
  const inner = el.querySelector('.loupe-inner') as HTMLElement;
  const loupe = el.querySelector('.loupe') as HTMLElement;
  const markEl = el.querySelector('.mark') as HTMLElement;
  const zs = el.querySelector('[data-z]') as HTMLInputElement, xs = el.querySelector('[data-x]') as HTMLInputElement, ys = el.querySelector('[data-y]') as HTMLInputElement;
  let z = 1, cx = 0.5, cy = 0.5;
  const apply = () => {
    // keep the visible window inside the print
    const half = 0.5 / z;
    cx = Math.min(1 - half, Math.max(half, cx)); cy = Math.min(1 - half, Math.max(half, cy));
    inner.style.transform = `scale(${z}) translate(${(0.5 - cx) * 100}%, ${(0.5 - cy) * 100}%)`;
    xs.value = String(cx); ys.value = String(cy);
  };
  zs.addEventListener('input', () => { z = +zs.value; apply(); });
  xs.addEventListener('input', () => { cx = +xs.value; apply(); });
  ys.addEventListener('input', () => { cy = +ys.value; apply(); });
  const showMark = (u: number, v: number) => {
    markEl.hidden = false;
    markEl.style.left = `${((PRINT.x + u * PHOTO_W) / PRINT.w) * 100}%`;
    markEl.style.top = `${((PRINT.y + v * PHOTO_H) / PRINT.h) * 100}%`;
  };
  if (o.mark) showMark(o.mark[0], o.mark[1]);
  let down: { x: number; y: number; cx: number; cy: number } | null = null;
  loupe.addEventListener('pointerdown', (e) => { down = { x: e.clientX, y: e.clientY, cx, cy }; loupe.setPointerCapture(e.pointerId); });
  loupe.addEventListener('pointermove', (e) => {
    if (!down || z <= 1) return;
    const r = loupe.getBoundingClientRect();
    cx = down.cx - (e.clientX - down.x) / r.width / z; cy = down.cy - (e.clientY - down.y) / r.height / z;
    apply();
  });
  loupe.addEventListener('pointerup', (e) => {
    const d = down; down = null;
    if (!d || Math.hypot(e.clientX - d.x, e.clientY - d.y) > 8 || !o.onMark) return;
    const img = el.querySelector('img')!.getBoundingClientRect();
    const px = (e.clientX - img.left) / img.width, py = (e.clientY - img.top) / img.height;
    const u = (px * PRINT.w - PRINT.x) / PHOTO_W, v = (py * PRINT.h - PRINT.y) / PHOTO_H;
    if (u < 0 || u > 1 || v < 0 || v > 1) return;
    const r = o.onMark(u, v);
    say(el, '[data-say]', r);
    if (r.ok) showMark(u, v);
  });
  let hint = 0;
  el.querySelector('[data-a=hint]')?.addEventListener('click', () => {
    const h = o.hints!;
    say(el, '[data-say]', h[Math.min(hint, h.length - 1)]);
    hint++;
  });
  apply();
  return el;
}

// ---------- reference file ----------
export function referenceFile(ui: UI, o: { sheet: string[]; refDone: boolean; hyp: string | null; onRef: (c: 'B-12' | 'R-07') => Result; onHyp: (c: 'light' | 'drift' | 'command') => Result }) {
  const el = shell(ui, 'REFERENCE FILE', `
    <div class="lp-two">
      <div class="lp-paper typed">${o.sheet.map((p) => `<p>${esc(p)}</p>`).join('')}</div>
      <div class="lp-side">
        <p class="lp-q">Which reference is the marked detail?</p>
        <div class="lp-row"><button class="lp-btn" data-r="B-12">MATCH MARKED DETAIL: B-12</button><button class="lp-btn" data-r="R-07">MATCH MARKED DETAIL: R-07</button></div>
        <div class="lp-hyp" ${o.refDone ? '' : 'hidden'}>
          <p class="lp-q">What could put an extra stripe on the film?</p>
          <div class="lp-col">
            <button class="lp-btn" data-h="light">HYPOTHESIS: STRAY LIGHT</button>
            <button class="lp-btn" data-h="drift">HYPOTHESIS: ENCODER DRIFT</button>
            <button class="lp-btn" data-h="command">HYPOTHESIS: MOTOR COMMAND</button>
          </div>
        </div>
        <p class="lp-text" data-say>${o.hyp ? 'A working hypothesis is on file. The field test is at B-12.' : o.refDone ? 'B-12 selected. Choose a cause you can test.' : ''}</p>
      </div>
    </div>`);
  el.querySelectorAll<HTMLButtonElement>('[data-r]').forEach((b) => b.addEventListener('click', () => {
    const r = o.onRef(b.dataset.r as 'B-12' | 'R-07');
    say(el, '[data-say]', r);
    if (r.ok) (el.querySelector('.lp-hyp') as HTMLElement).hidden = false;
  }));
  el.querySelectorAll<HTMLButtonElement>('[data-h]').forEach((b) => b.addEventListener('click', () => {
    say(el, '[data-say]', o.onHyp(b.dataset.h as 'light' | 'drift' | 'command'));
  }));
  return el;
}

// ---------- B-12 local control ----------
export function b12Control(ui: UI, o: { method: 'passive' | 'active' | null; onChoose: (m: 'passive' | 'active') => void; review?: string }) {
  const body = o.method
    ? `<p class="lp-sub">B-12 / LOCAL OPTICAL REFERENCE / MANUAL CONTROL ONLY</p><div class="lp-paper typed"><p>${esc(o.review ?? '')}</p></div>`
    : `<p class="lp-sub">B-12 / LOCAL OPTICAL REFERENCE / MANUAL CONTROL ONLY</p>
       <div class="lp-cards">
         <button class="lp-card" data-m="passive"><b>PASSIVE / SHIELD THE LAMP</b><span>Fold the vane into its shield stop. The warm lamp is blocked and the reference motor remains isolated. A reflection should depend on its source.</span></button>
         <button class="lp-card" data-m="active"><b>ACTIVE / DRIVE TO 042</b><span>Move the local reference to a known heading. Compare the visible vane with its encoder. The work lamp remains on.</span></button>
       </div>`;
  const el = shell(ui, 'B-12 CONTROL', body);
  el.querySelectorAll<HTMLButtonElement>('[data-m]').forEach((b) => b.addEventListener('click', () => { ui.close(); o.onChoose(b.dataset.m as 'passive' | 'active'); }));
}

// ---------- two exposures ----------
export function twoExposures(ui: UI, o: { url1: string; url2: string; mark2?: [number, number]; concluded: boolean; onMark: (u: number, v: number) => Result; onConclude: (c: 'lamp' | 'commands' | 'second') => Result; hints: string[] }) {
  const el = shell(ui, 'TWO EXPOSURES', `
    <div class="lp-pair">
      <figure><img alt="Frame 01" src="${o.url1}" draggable="false"><figcaption>FRAME 01 / S-03 APRON</figcaption></figure>
      <figure class="clickable"><div class="pair-img"><img alt="Frame 02" src="${o.url2}" draggable="false"><div class="mark" hidden></div></div><figcaption>FRAME 02 / B-12 CONTROL. Mark the same reference.</figcaption></figure>
    </div>
    <p class="lp-text" data-say>${o.concluded ? 'Finding recorded. File both prints at the records desk.' : 'Mark the reference detail in FRAME 02 before recording a conclusion.'}</p>
    <div class="lp-row" data-concl>
      <button class="lp-btn" data-c="lamp">THE LAMP CREATED THE MARK</button>
      <button class="lp-btn" data-c="commands">TWO ANTENNA COMMANDS</button>
      <button class="lp-btn" data-c="second">FILM RETAINS A SECOND REFERENCE</button>
      <button class="lp-btn" data-a="hint">Hint</button>
    </div>`);
  const markEl = el.querySelector('.pair-img .mark') as HTMLElement;
  const showMark = (u: number, v: number) => {
    markEl.hidden = false;
    markEl.style.left = `${((PRINT.x + u * PHOTO_W) / PRINT.w) * 100}%`;
    markEl.style.top = `${((PRINT.y + v * PHOTO_H) / PRINT.h) * 100}%`;
  };
  if (o.mark2) showMark(o.mark2[0], o.mark2[1]);
  const img = el.querySelector('.pair-img img') as HTMLImageElement;
  img.addEventListener('click', (e) => {
    const r = img.getBoundingClientRect();
    const u = ((e.clientX - r.left) / r.width * PRINT.w - PRINT.x) / PHOTO_W, v = ((e.clientY - r.top) / r.height * PRINT.h - PRINT.y) / PHOTO_H;
    if (u < 0 || u > 1 || v < 0 || v > 1) return;
    const res = o.onMark(u, v);
    say(el, '[data-say]', res);
    if (res.ok) showMark(u, v);
  });
  el.querySelectorAll<HTMLButtonElement>('[data-c]').forEach((b) => b.addEventListener('click', () => say(el, '[data-say]', o.onConclude(b.dataset.c as 'lamp' | 'commands' | 'second'))));
  let hint = 0;
  el.querySelector('[data-a=hint]')!.addEventListener('click', () => { say(el, '[data-say]', o.hints[Math.min(hint, o.hints.length - 1)]); hint++; });
}

// ---------- local report ----------
export function localReport(ui: UI, o: { text: string; filed: boolean; onFile: () => void }) {
  const el = shell(ui, 'LOCAL REPORT', `
    <div class="lp-paper typed report">${o.text.split('\n\n').map((p) => `<p>${esc(p)}</p>`).join('')}</div>
    ${o.filed ? '<p class="lp-text">Filed.</p>' : '<button class="lp-btn primary" data-a="file">FILE PRINTS AND CLOSE THE LOCAL CASE</button>'}`);
  el.querySelector('[data-a=file]')?.addEventListener('click', () => { ui.close(); o.onFile(); });
}

// ---------- chapter two ----------
// The archive table lays out the case board (story/caseBoard.ts); the four sources keep their keys.
export type SourceKey = 'original' | 'amended' | 'lineage' | 'index';

// ---------- chapter three: the null test at the STATION 01 field lamp ----------
// P07 stays in the world (SPILLDESIGN.md): cover the lamp's tin hood, observe, and the result
// is pinned on the case board by itself. P06, P08 and P09 are made on the board, where the
// tabbed field record with three sentences per page used to be (Tom: "knotete").
export function nullTest(ui: UI, o: {
  status: () => string;
  state: () => { covered: boolean; observed: boolean; p07: boolean };
  supported: string;
  onLamp: (cover: boolean) => void;
  onObserve: () => Result;
  hints: string[];
}) {
  const el = shell(ui, 'STATION 01 / FIELD LAMP', '<p class="lp-sub" data-status></p><div class="lp-page" data-page></div>');
  const pageEl = el.querySelector('[data-page]') as HTMLElement;
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  const paper = (text: string) => `<div class="lp-paper typed">${text.split('\n\n').map((p) => `<p>${esc(p).replace(/\n/g, '<br>')}</p>`).join('')}</div>`;
  let hint = 0;
  const render = () => {
    const s = o.state();
    statusEl.textContent = o.status();
    pageEl.innerHTML = `
      ${paper('Screen the practical lamp and observe the ordinary local null condition. This isolates the source. It does not move the physical reference, and it does not finish the cable finding.')}
      <p class="lp-q">LAMP STATUS / ${s.covered ? 'COVERED' : 'EXPOSED'}</p>
      <div class="lp-row"><button class="lp-btn" data-a="cover">${s.covered ? 'UNCOVER LAMP' : 'COVER LAMP'}</button><button class="lp-btn primary" data-a="observe">${s.observed ? 'REVIEW NULL TEST' : 'OBSERVE NULL TEST'}</button><button class="lp-btn" data-a="hint">Hint</button></div>
      ${s.p07 ? paper(o.supported) + '<p class="lp-small">P07 is pinned on the case board.</p>' : ''}
      <p class="lp-text" data-say></p>`;
    pageEl.querySelector('[data-a=cover]')!.addEventListener('click', () => { o.onLamp(!o.state().covered); render(); });
    pageEl.querySelector('[data-a=observe]')!.addEventListener('click', () => { const r = o.onObserve(); if (r.ok) render(); say(el, '[data-say]', r); });
    pageEl.querySelector('[data-a=hint]')!.addEventListener('click', () => { say(el, '[data-say]', o.hints[Math.min(hint, o.hints.length - 1)]); hint++; });
  };
  render();
  return { el, refresh: render };
}

// Two developed field prints side by side, for a look before they go into the case file.
export function fieldPrints(ui: UI, o: { prints: { url: string; caption: string }[]; text: string; onClose?: () => void }) {
  shell(ui, 'STATION 01 / FIELD PRINTS', `
    <div class="lp-pair">${o.prints.map((p) => `<figure><img alt="" src="${p.url}"><figcaption>${esc(p.caption)}</figcaption></figure>`).join('')}</div>
    <p class="lp-text">${esc(o.text)}</p>`, o.onClose);
}

// ---------- chapter four: a conversation in room 6 ----------
// N. Vega at the table. The chapter keeps the state and says what is on screen: the last
// exchange, the evidence that can be put on the table, a question, and the choices. The
// panel only draws it and passes clicks back.
export interface TalkLine { who: string; text: string }
export interface TalkChip { id: string; label: string; image?: string; have: boolean; on: boolean }
export interface TalkView {
  status: string;
  lines: TalkLine[];
  table?: { prompt: string; chips: TalkChip[]; submit: string };
  question?: string;
  choices: { id: string; label: string; disabled?: boolean }[];
  reply?: Result | null;
  given?: { id: string; label: string }[];
}
export function talk(ui: UI, o: {
  title: string;
  view: () => TalkView;
  onChip: (id: string) => void;
  onTable: () => void;
  onChoice: (id: string) => void;
  onRead: (id: string) => void;
  onClose?: () => void;
}) {
  const el = shell(ui, o.title, '<p class="lp-sub" data-status></p><div class="talk" data-body></div>', o.onClose);
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  const body = el.querySelector('[data-body]') as HTMLElement;
  let shown = '';
  const render = () => {
    if (!el.isConnected) return;
    const v = o.view();
    statusEl.textContent = v.status;
    // new lines come one after another; a redraw of the same lines does not repeat that
    const key = JSON.stringify(v.lines);
    const fresh = key !== shown;
    shown = key;
    // the face of whoever spoke last (core/portraits.ts), once there is a picture of them
    const last = [...v.lines].reverse().find((l) => l.who && l.who !== 'YOU');
    const face = last ? portraitOf(last.who) : null;
    const line = (l: TalkLine, i: number) => `<p class="talk-line${l.who ? '' : ' aside'}${fresh ? ' fresh' : ''}" style="animation-delay:${fresh ? i * 0.55 : 0}s">${l.who ? `<span class="who">${esc(l.who)}</span>` : ''}${esc(l.text)}</p>`;
    const chip = (c: TalkChip) => `<button class="lp-choice talk-chip" data-chip="${c.id}" aria-pressed="${c.on}" ${c.have ? '' : 'disabled'}>${c.image ? `<img alt="" src="${c.image}">` : ''}<span>${esc(c.label)}${c.have ? '' : ' / not in the case'}</span></button>`;
    body.innerHTML = `
      ${face ? `<img class="talk-portrait" alt="${esc(last!.who)}" src="${face}">` : ''}
      <div class="talk-lines">${v.lines.map(line).join('')}</div>
      ${v.table ? `<p class="lp-q">${esc(v.table.prompt)}</p><div class="talk-table">${v.table.chips.map(chip).join('')}</div>
        <div class="lp-row"><button class="lp-btn primary" data-a="table">${esc(v.table.submit)}</button></div>` : ''}
      ${v.question ? `<p class="lp-q">${esc(v.question)}</p>` : ''}
      <div class="lp-col">${v.choices.map((c) => `<button class="lp-btn" data-c="${c.id}" ${c.disabled ? 'disabled' : ''}>${esc(c.label)}</button>`).join('')}</div>
      ${v.reply ? `<p class="lp-text ${v.reply.ok ? 'good' : 'bad'}">${esc(v.reply.text)}</p>` : ''}
      ${v.given?.length ? `<p class="lp-small">On the table</p><div class="lp-row">${v.given.map((g) => `<button class="lp-btn" data-read="${g.id}">${esc(g.label)}</button>`).join('')}</div>` : ''}`;
    body.querySelectorAll<HTMLButtonElement>('[data-chip]').forEach((b) => b.addEventListener('click', () => { o.onChip(b.dataset.chip!); render(); }));
    body.querySelector('[data-a=table]')?.addEventListener('click', () => { o.onTable(); render(); });
    body.querySelectorAll<HTMLButtonElement>('[data-c]').forEach((b) => b.addEventListener('click', () => { o.onChoice(b.dataset.c!); render(); }));
    body.querySelectorAll<HTMLButtonElement>('[data-read]').forEach((b) => b.addEventListener('click', () => o.onRead(b.dataset.read!)));
  };
  render();
  return { el, refresh: render };
}

// ---------- the signal processor on the control room rack ----------
export interface DecoderView {
  tape: string;
  mode: 'raw' | 'harmonized'; pitch: number; stretch: number; space: number;
  playing: boolean;
  current: () => number;   // which pulse sounds (0..10), -1 between
}
export function decoderPanel(ui: UI, o: {
  view: () => DecoderView;
  onMode: (m: 'raw' | 'harmonized') => void;
  onParam: (k: 'pitch' | 'stretch' | 'space', v: number) => void;
  onPlay: (play: boolean) => void;
  onClose?: () => void;
}) {
  const v0 = o.view();
  const el = shell(ui, 'SIGNAL PROCESSOR / DSP-4', `
    <p class="lp-sub" data-tape></p>
    <canvas class="dsp-scope" width="640" height="120" aria-hidden="true"></canvas>
    <div class="lp-row" role="group" aria-label="Mode">
      <button class="lp-choice dsp-mode" data-m="raw">RAW TAPE</button>
      <button class="lp-choice dsp-mode" data-m="harmonized">HARMONIZED</button>
    </div>
    <div class="dsp-knobs">
      <label>PITCH <span data-o="pitch"></span><input type="range" min="-12" max="12" step="1" data-k="pitch" value="${v0.pitch}"></label>
      <label>TIME <span data-o="stretch"></span><input type="range" min="1" max="6" step="0.5" data-k="stretch" value="${v0.stretch}"></label>
      <label>SPACE <span data-o="space"></span><input type="range" min="0" max="1" step="0.05" data-k="space" value="${v0.space}"></label>
    </div>
    <div class="lp-row"><button class="lp-btn primary" data-a="play"></button></div>
    <p class="lp-small">The deck keeps what the receiver locked on. Slowed down and harmonized, the pulses stop sounding like a machine. The processor keeps playing from the rack while you walk around.</p>`, () => { cancelAnimationFrame(raf); o.onClose?.(); });
  const scope = el.querySelector('.dsp-scope') as HTMLCanvasElement;
  const g = scope.getContext('2d')!;
  const sync = () => {
    const v = o.view();
    (el.querySelector('[data-tape]') as HTMLElement).textContent = v.tape;
    el.querySelectorAll<HTMLButtonElement>('[data-m]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.m === v.mode)));
    (el.querySelector('[data-o=pitch]') as HTMLElement).textContent = `${v.pitch > 0 ? '+' : ''}${v.pitch} st`;
    (el.querySelector('[data-o=stretch]') as HTMLElement).textContent = `x${v.stretch.toFixed(1)}`;
    (el.querySelector('[data-o=space]') as HTMLElement).textContent = `${Math.round(v.space * 100)}%`;
    (el.querySelector('[data-a=play]') as HTMLElement).textContent = v.playing ? 'STOP' : 'PLAY TAPE';
  };
  el.querySelectorAll<HTMLButtonElement>('[data-m]').forEach((b) => b.addEventListener('click', () => { o.onMode(b.dataset.m as 'raw' | 'harmonized'); sync(); }));
  el.querySelectorAll<HTMLInputElement>('[data-k]').forEach((i) => i.addEventListener('input', () => { o.onParam(i.dataset.k as 'pitch' | 'stretch' | 'space', Number(i.value)); sync(); }));
  el.querySelector('[data-a=play]')!.addEventListener('click', () => { o.onPlay(!o.view().playing); sync(); });
  // the eleven pulses of one group cycle, the sounding one lit
  let raf = 0;
  const draw = () => {
    const v = o.view(), w = scope.width, h = scope.height, cur = v.playing ? v.current() : -1;
    g.fillStyle = '#04110a'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 11; i++) {
      const x = 30 + (i < 4 ? i : i + 1.5) * 47, hh = (i < 4 ? 0.55 : 0.75) * h * (v.mode === 'raw' ? 0.8 : 0.5 + 0.05 * ((i < 4 ? [0, 3, 7, 10][i] : [12, 10, 7, 5, 3, 2, 0][i - 4])));
      g.fillStyle = i === cur ? '#b9ffc7' : 'rgba(110,220,150,.35)';
      g.fillRect(x, h - 10 - hh, 30, hh);
    }
    g.fillStyle = 'rgba(140,255,164,.8)'; g.font = '18px VT323';
    g.fillText(`${v.mode === 'raw' ? 'RAW' : 'HARM'}  4 / 7  ${v.playing ? 'PLAY' : 'STOP'}`, 12, 22);
    raf = requestAnimationFrame(draw);
  };
  sync(); draw();
  return { el, refresh: sync };
}

// shared with panels in other files (ui/MapOverlay.ts)
export { shell as panelShell, say as panelSay };
