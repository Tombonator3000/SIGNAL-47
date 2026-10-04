import type { UI } from './UI';
import { PRINT, PHOTO_W, PHOTO_H } from '../core/FieldCamera';

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
  const box = el.querySelector(sel) as HTMLElement;
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
