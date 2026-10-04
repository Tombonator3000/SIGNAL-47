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

// ---------- chapter two: the archive table ----------
// One panel at the records room work table, after the Unity dossier (WorldCase22): the
// four sources as tabs, then P04 (what changed) and P05 (where the reference leads).
// A source that is still on the shelf says where it is instead.
export type ArchivePage = 'original' | 'amended' | 'lineage' | 'index' | 'compare' | 'route';
export type SourceKey = 'original' | 'amended' | 'lineage' | 'index';
export interface ArchiveSource { have: boolean; heading: string; text: string; image?: string; where: string }
type Marker = 'triangle-bar' | 'triangle' | 'three-bars';

export function archive(ui: UI, o: {
  page: ArchivePage;
  sources: Record<SourceKey, ArchiveSource>;
  status: () => string;
  p04: () => boolean; p05: () => boolean;
  supported04: string; supported05: string;
  onCompare: (c: 'author-guilt' | 'omitted-c' | 'development-only') => Result;
  onRoute: (destination: string, survey: string, marker: Marker) => Result;
  hints04: string[]; hints05: string[];
  icons: Record<Marker, string>;
}) {
  const tabs: [ArchivePage, string][] = [['original', 'E07 / ORIGINAL'], ['amended', 'E06 / AMENDED'], ['lineage', 'B-12 / LINEAGE'], ['index', 'E08 / INDEX'], ['compare', 'P04 / COMPARE'], ['route', 'P05 / DESTINATION']];
  const el = shell(ui, 'SARO ARCHIVE / THE AMENDED RECORD', `
    <p class="lp-sub" data-status></p>
    <div class="lp-tabs" role="tablist">${tabs.map(([k, l]) => `<button class="lp-tab" role="tab" data-p="${k}">${l}</button>`).join('')}</div>
    <div class="lp-page" data-page></div>`);
  const pageEl = el.querySelector('[data-page]') as HTMLElement;
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  let hint04 = 0, hint05 = 0;
  const sel: { destination?: string; survey?: string; marker?: Marker } = {};

  const paper = (text: string) => `<div class="lp-paper typed">${text.split('\n\n').map((p) => `<p>${esc(p).replace(/\n/g, '<br>')}</p>`).join('')}</div>`;
  const render = (page: ArchivePage) => {
    statusEl.textContent = o.status();
    el.querySelectorAll<HTMLButtonElement>('[data-p]').forEach((b) => b.setAttribute('aria-selected', String(b.dataset.p === page)));
    if (page === 'compare') return compare();
    if (page === 'route') return route();
    const src = o.sources[page];
    pageEl.innerHTML = src.have
      ? `<div class="lp-two"><div><p class="lp-q">${esc(src.heading)}</p>${paper(src.text)}</div>${src.image ? `<figure class="lp-figure"><img alt="" src="${src.image}"></figure>` : ''}</div>
         <p class="lp-small">Read in any order. Use COMPARE and DESTINATION to record findings.</p>`
      : `<p class="lp-text">Not on the table yet. ${esc(src.where)}</p>`;
  };

  const compare = () => {
    const s = o.sources;
    if (o.p04()) { pageEl.innerHTML = `${paper(o.supported04)}<p class="lp-small">P04 recorded. The sources stay available above.</p>`; return; }
    const both = s.original.have && s.amended.have;
    pageEl.innerHTML = `
      <div class="lp-two">
        <div><p class="lp-q">ORIGINAL / E07</p>${paper('Three references: A, the fixed survey point; B, the optical comparison vane; C, the closing sight line.\n\nThe mark remained in the plate with the lamp circuit opened.')}</div>
        <div><p class="lp-q">AMENDED / E06</p>${paper('The incomplete closing sight line has been omitted.\n\nThe additional mark is attributed to a fault in plate development. No repeat observation is required.')}</div>
      </div>
      <p class="lp-q">${both ? 'Which finding is supported by the source records?' : 'Read E07 and E06 before recording a finding. The source tabs stay available above.'}</p>
      <div class="lp-col">
        <button class="lp-btn" data-c="author-guilt">The signature proves that the author caused the anomaly.</button>
        <button class="lp-btn" data-c="omitted-c">The amended copy removes C and replaces the retained-mark observation with a development explanation.</button>
        <button class="lp-btn" data-c="development-only">The development explanation accounts for both versions without an omitted reference.</button>
      </div>
      <div class="lp-row"><button class="lp-btn" data-a="hint">Hint</button></div>
      <p class="lp-text" data-say></p>`;
    pageEl.querySelectorAll<HTMLButtonElement>('[data-c]').forEach((b) => b.addEventListener('click', () => {
      const r = o.onCompare(b.dataset.c as 'author-guilt' | 'omitted-c' | 'development-only');
      if (r.ok) { render('compare'); return; }
      say(el, '[data-say]', r);
    }));
    pageEl.querySelector('[data-a=hint]')!.addEventListener('click', () => { say(el, '[data-say]', o.hints04[Math.min(hint04, o.hints04.length - 1)]); hint04++; });
  };

  const route = () => {
    if (o.p05()) { pageEl.innerHTML = `${paper(o.supported05)}<p class="lp-small">P05 recorded. The sources stay available above.</p>`; return; }
    const s = o.sources;
    const lead = !o.p04() ? 'First record the comparison in P04. You can inspect all four source cards now.'
      : !s.lineage.have || !s.index.have ? 'Read the B-12 lineage card and E08 index before preparing the destination.'
        : 'Match both source identifiers. Select a destination, survey ID and fixed-point mark.';
    const choice = (group: string, key: string, label: string, icon?: string) =>
      `<button class="lp-choice" data-g="${group}" data-k="${key}" aria-pressed="false">${icon ? `<img alt="" src="${icon}">` : ''}<span>${esc(label)}</span></button>`;
    pageEl.innerHTML = `
      <p class="lp-q">${esc(lead)}</p>
      <div class="lp-cols3">
        <div><p class="lp-small">FIELD DESTINATION</p>${choice('destination', 'old-survey-station', 'OLD SURVEY STATION')}${choice('destination', 'saro-apron', 'SARO ARRAY APRON')}${choice('destination', 'unlisted', 'UNLISTED FIELD SITE')}</div>
        <div><p class="lp-small">SURVEY IDENTIFIER</p>${choice('survey', 'STATION 01', 'STATION 01')}${choice('survey', 'S-03', 'S-03')}${choice('survey', '-39 LY AS A YEAR CODE', '-39 LY AS A YEAR CODE')}</div>
        <div><p class="lp-small">FIXED-POINT MARK</p>${choice('marker', 'triangle-bar', 'OUTLINED TRIANGLE + BAR', o.icons['triangle-bar'])}${choice('marker', 'triangle', 'TRIANGLE / NO BAR', o.icons.triangle)}${choice('marker', 'three-bars', 'THREE HORIZONTAL BARS', o.icons['three-bars'])}</div>
      </div>
      <p class="lp-small">SOURCE CHECK / The maintenance card must connect today's B-12 to the same survey and fixed-point reference in the sleeve. A place name alone is insufficient.</p>
      <div class="lp-row"><button class="lp-btn primary" data-a="record">RECORD SUPPORTED FIELD DESTINATION</button><button class="lp-btn" data-a="hint">Hint</button></div>
      <p class="lp-text" data-say></p>`;
    const rec = pageEl.querySelector('[data-a=record]') as HTMLButtonElement;
    const sync = () => { rec.disabled = !(sel.destination && sel.survey && sel.marker); };
    pageEl.querySelectorAll<HTMLButtonElement>('[data-g]').forEach((b) => {
      const g = b.dataset.g as 'destination' | 'survey' | 'marker';
      if (sel[g] === b.dataset.k) b.setAttribute('aria-pressed', 'true');
      b.addEventListener('click', () => {
        (sel as Record<string, string>)[g] = b.dataset.k!;
        pageEl.querySelectorAll<HTMLButtonElement>(`[data-g=${g}]`).forEach((x) => x.setAttribute('aria-pressed', String(x === b)));
        sync();
      });
    });
    rec.addEventListener('click', () => {
      const r = o.onRoute(sel.destination!, sel.survey!, sel.marker!);
      if (r.ok) { render('route'); return; }
      say(el, '[data-say]', r);
    });
    pageEl.querySelector('[data-a=hint]')!.addEventListener('click', () => { say(el, '[data-say]', o.hints05[Math.min(hint05, o.hints05.length - 1)]); hint05++; });
    sync();
  };

  el.querySelectorAll<HTMLButtonElement>('[data-p]').forEach((b) => b.addEventListener('click', () => render(b.dataset.p as ArchivePage)));
  render(o.page);
  return el;
}

// ---------- chapter three: the STATION 01 field record ----------
// One panel for P06 to P09 at the station, after the Unity field record (Station26):
// the transit record and which fixed point moved, the local lamp null test, the cable
// break, and the 1947 timing log against tonight's receiver. Opened by the transit, the
// lamp, the cable and the ledger; the tabs reach the other pages.
export type FieldPage = 'marker' | 'lamp' | 'cable' | 'timing';
export function fieldRecord(ui: UI, o: {
  page: FieldPage;
  status: () => string;
  transit: { have: boolean; text: string; image: string; where: string };
  timing: () => { have: boolean; text: string; where: string };
  state: () => { covered: boolean; observed: boolean; inspected: boolean; frame04: boolean; p06: boolean; p07: boolean; p08: boolean; p09: boolean };
  supported: Record<FieldPage, string>;
  onMarker: (c: 'a' | 'b' | 'lamp') => Result;
  onLamp: (cover: boolean) => void;
  onObserve: () => Result;
  onInspect: () => Result;
  onCable: (c: 'deliberate-cut' | 'weathering' | 'author-guilt') => Result;
  onTiming: (c: '02:17:47' | '02:17:00' | '47-minutes') => Result;
  hints: Record<FieldPage, string[]>;
}) {
  const tabs: [FieldPage, string][] = [['marker', 'P06 / TRANSIT'], ['lamp', 'P07 / NULL TEST'], ['cable', 'P08 / CABLE'], ['timing', 'P09 / TIMING']];
  const el = shell(ui, 'STATION 01 / FIELD RECORD', `
    <p class="lp-sub" data-status></p>
    <div class="lp-tabs" role="tablist">${tabs.map(([k, l]) => `<button class="lp-tab" role="tab" data-p="${k}">${l}</button>`).join('')}</div>
    <div class="lp-page" data-page></div>`);
  const pageEl = el.querySelector('[data-page]') as HTMLElement;
  const statusEl = el.querySelector('[data-status]') as HTMLElement;
  const hint: Record<FieldPage, number> = { marker: 0, lamp: 0, cable: 0, timing: 0 };
  let page = o.page;

  const paper = (text: string) => `<div class="lp-paper typed">${text.split('\n\n').map((p) => `<p>${esc(p).replace(/\n/g, '<br>')}</p>`).join('')}</div>`;
  const choices = (list: [string, string][]) => `<div class="lp-col">${list.map(([k, l]) => `<button class="lp-btn" data-c="${k}">${esc(l)}</button>`).join('')}</div>`;
  const tail = () => `<div class="lp-row"><button class="lp-btn" data-a="hint">Hint</button></div><p class="lp-text" data-say></p>`;
  const wire = (p: FieldPage, on: (c: string) => Result) => {
    pageEl.querySelectorAll<HTMLButtonElement>('[data-c]').forEach((b) => b.addEventListener('click', () => {
      const r = on(b.dataset.c!);
      if (r.ok) { render(p); say(el, '[data-say]', r); return; }
      say(el, '[data-say]', r);
    }));
    pageEl.querySelector('[data-a=hint]')?.addEventListener('click', () => { say(el, '[data-say]', o.hints[p][Math.min(hint[p], o.hints[p].length - 1)]); hint[p]++; });
  };

  const render = (p: FieldPage) => {
    page = p;
    const s = o.state();
    statusEl.textContent = o.status();
    el.querySelectorAll<HTMLButtonElement>('[data-p]').forEach((b) => b.setAttribute('aria-selected', String(b.dataset.p === p)));
    if (p === 'marker') {
      const t = o.transit;
      pageEl.innerHTML = `
        <div class="lp-two">
          <div><p class="lp-q">E09A / TRANSIT RECORD</p>${t.have ? paper(t.text) : `<p class="lp-text">Not read yet. ${esc(t.where)}</p>`}</div>
          <figure class="lp-figure"><img alt="The 1947 arrangement: A, B and C" src="${t.image}"><figcaption>A  FIXED POINT<br>B  COMPARISON VANE<br>C  CLOSING SIGHT LINE</figcaption></figure>
        </div>
        ${s.p06 ? paper(o.supported.marker) : `<p class="lp-q">Which fixed position changed?</p>
          ${choices([['a', 'A / MOVED'], ['b', 'B / MOVED'], ['lamp', 'UNCERTAIN / LOCAL LAMP']])}${tail()}`}`;
      if (!s.p06) wire(p, (c) => o.onMarker(c as 'a' | 'b' | 'lamp'));
    } else if (p === 'lamp') {
      pageEl.innerHTML = `
        ${paper('Screen the practical lamp and observe the ordinary local null condition. This isolates the source. It does not move the physical reference, and it does not finish the cable finding.')}
        <p class="lp-q">LAMP STATUS / ${s.covered ? 'COVERED' : 'EXPOSED'}</p>
        <div class="lp-row"><button class="lp-btn" data-a="cover">${s.covered ? 'UNCOVER LAMP' : 'COVER LAMP'}</button><button class="lp-btn primary" data-a="observe">${s.observed ? 'REVIEW NULL TEST' : 'OBSERVE NULL TEST'}</button><button class="lp-btn" data-a="hint">Hint</button></div>
        ${s.p07 ? paper(o.supported.lamp) : ''}
        <p class="lp-text" data-say></p>`;
      pageEl.querySelector('[data-a=cover]')!.addEventListener('click', () => { o.onLamp(!o.state().covered); render(p); });
      pageEl.querySelector('[data-a=observe]')!.addEventListener('click', () => { const r = o.onObserve(); if (r.ok) render(p); say(el, '[data-say]', r); });
      pageEl.querySelector('[data-a=hint]')!.addEventListener('click', () => { say(el, '[data-say]', o.hints.lamp[Math.min(hint.lamp, o.hints.lamp.length - 1)]); hint.lamp++; });
    } else if (p === 'cable') {
      pageEl.innerHTML = `
        ${paper('Follow the cable from the local relay. Inspect the opposed cut faces, then expose and inspect the genuine near frame. The retained reference and a deliberate cut are separate findings.')}
        <div class="lp-row"><button class="lp-btn" data-a="inspect">${s.inspected ? 'CABLE INSPECTED' : 'INSPECT CUT FACES'}</button></div>
        <p class="lp-q">${s.frame04 ? 'FRAME 04 / EXPOSED, SEALED / PROCESS AT THE SARO WET BENCH' : 'FRAME 04 / NOT EXPOSED YET / A NEAR FRAME OF THE CUT'}</p>
        ${s.p08 ? paper(o.supported.cable) : `<p class="lp-q">What does the break show?</p>
          ${choices([['deliberate-cut', 'The cable was cut on purpose: two clean, opposed faces.'], ['weathering', 'The cable failed with age and weather.'], ['author-guilt', 'The cut shows who stopped the 1947 run, and why.']])}${tail()}`}`;
      pageEl.querySelector('[data-a=inspect]')!.addEventListener('click', () => { const r = o.onInspect(); if (r.ok) render(p); say(el, '[data-say]', r); });
      if (!s.p08) wire(p, (c) => o.onCable(c as 'deliberate-cut' | 'weathering' | 'author-guilt'));
    } else {
      const t = o.timing();
      pageEl.innerHTML = `
        <p class="lp-q">E10 / TIMING RECORD</p>
        ${t.have ? paper(t.text) : `<p class="lp-text">Not read yet. ${esc(t.where)}</p>`}
        ${s.p09 ? paper(o.supported.timing) : `<p class="lp-q">Match the receiver fragment to the 1947 log. Which reading does T. Vega's correction lead up to?</p>
          ${choices([['02:17:47', '02:17:47 / Relay impact and reference motion, 47 seconds after the carrier ceased.'], ['02:17:00', '02:17:00 / The moment the carrier ceased.'], ['47-minutes', '47 minutes later / A separate event the log does not record.']])}${tail()}`}`;
      if (!s.p09) wire(p, (c) => o.onTiming(c as '02:17:47' | '02:17:00' | '47-minutes'));
    }
  };

  el.querySelectorAll<HTMLButtonElement>('[data-p]').forEach((b) => b.addEventListener('click', () => render(b.dataset.p as FieldPage)));
  render(o.page);
  return { el, refresh: () => render(page) };
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
    const line = (l: TalkLine, i: number) => `<p class="talk-line${l.who ? '' : ' aside'}${fresh ? ' fresh' : ''}" style="animation-delay:${fresh ? i * 0.55 : 0}s">${l.who ? `<span class="who">${esc(l.who)}</span>` : ''}${esc(l.text)}</p>`;
    const chip = (c: TalkChip) => `<button class="lp-choice talk-chip" data-chip="${c.id}" aria-pressed="${c.on}" ${c.have ? '' : 'disabled'}>${c.image ? `<img alt="" src="${c.image}">` : ''}<span>${esc(c.label)}${c.have ? '' : ' / not in the case'}</span></button>`;
    body.innerHTML = `
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
