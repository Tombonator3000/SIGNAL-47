import type { UI } from './UI';
import { CASES, MANUAL, type SaveMeta } from '../core/saves';

// The case menus: load a save, save to one of the case's manual slots, or pick a case slot
// for a new night. Saves are cards with a small picture, the chapter, the place, the
// night's clock, play time and the real date. Overwriting and deleting ask first.

export interface SaveMenuOpts {
  mode: 'load' | 'save' | 'new';
  metas: SaveMeta[];
  activeCase: number | null;
  block?: string | null;            // save mode: why saving is not possible right now
  fallback?: boolean;               // saves live in localStorage (no IndexedDB)
  onLoad?: (m: SaveMeta) => void;
  onSave?: (slot: number) => Promise<SaveMeta>;
  onDelete?: (m: SaveMeta) => Promise<void>;
  onNewCase?: (caseId: number) => void;
  onBack?: () => void;
}

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
const when = (ms: number) => new Intl.DateTimeFormat('en-US', { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', hour12: false }).format(ms);
export function played(sec: number) {
  const m = Math.floor(sec / 60);
  if (m < 1) return 'under a minute';
  if (m < 60) return `${m} min`;
  return `${Math.floor(m / 60)} h ${m % 60} min`;
}
const kindName = (m: SaveMeta) => m.kind === 'auto' ? `AUTOSAVE ${m.slot + 1}` : `SAVE ${m.slot + 1}`;

function card(m: SaveMeta, buttons: string) {
  return `<div class="save-card" data-id="${m.id}">
    ${m.thumb ? `<img class="save-thumb" alt="" src="${m.thumb}">` : '<div class="save-thumb blank"></div>'}
    <div class="save-text">
      <div class="save-kind">${kindName(m)}</div>
      <div class="save-chapter">${esc(m.chapter)}</div>
      <div class="save-where">${esc(m.place)} · ${esc(m.clock)}</div>
      <div class="save-when">Saved ${when(m.savedAt)} · ${played(m.playtime)} played</div>
    </div>
    <div class="save-btns">${buttons}</div>
  </div>`;
}

export function saveMenu(ui: UI, o: SaveMenuOpts) {
  const el = document.createElement('div');
  el.className = 'overlay savemenu-wrap';
  let metas = [...o.metas];
  const caseMetas = (c: number) => metas.filter((m) => m.caseId === c).sort((a, b) => b.savedAt - a.savedAt);

  const render = (status = '') => {
    let body = '';
    if (o.mode === 'save') {
      const c = o.activeCase ?? 1;
      body += `<p class="save-note">Case ${c}. Autosaves are made at each new chapter, area and finding. Manual saves stay until you overwrite them.</p>`;
      if (o.block) body += `<p class="save-block">${esc(o.block)}</p>`;
      for (let s = 0; s < MANUAL; s++) {
        const m = metas.find((x) => x.caseId === c && x.kind === 'manual' && x.slot === s);
        const btn = o.block ? '' : `<button data-save="${s}">${m ? 'Overwrite' : 'Save here'}</button>`;
        body += m ? card(m, btn) : `<div class="save-card empty"><div class="save-thumb blank"></div><div class="save-text"><div class="save-kind">SAVE ${s + 1}</div><div class="save-where">Empty</div></div><div class="save-btns">${btn}</div></div>`;
      }
    } else {
      for (let c = 1; c <= CASES; c++) {
        const list = caseMetas(c);
        const last = list[0];
        const head = last ? `Last saved ${when(last.savedAt)} · ${played(Math.max(...list.map((m) => m.playtime)))} played` : 'Empty';
        body += `<section class="save-case"><h4>CASE ${c}${o.activeCase === c ? ' <span class="cur">current</span>' : ''}</h4><p class="save-head">${head}</p>`;
        if (o.mode === 'new') {
          body += `<div class="save-btns row"><button data-new="${c}">${list.length ? 'Replace with a new night' : 'Start a new night here'}</button></div>`;
          if (last) body += card(last, '');
        } else {
          for (const m of list) body += card(m, `<button data-load="${m.id}">Load</button><button class="quiet" data-del="${m.id}">Delete</button>`);
        }
        body += '</section>';
      }
    }
    const title = o.mode === 'save' ? 'Save case' : o.mode === 'new' ? 'New case' : 'Load case';
    el.innerHTML = `<div class="savemenu">
      <div class="save-top"><h3>${title}</h3><button class="close" data-a="back">Back</button></div>
      ${o.fallback ? '<p class="save-note warn">This browser blocks the photo store, so saves are kept in a smaller space and may not all fit.</p>' : ''}
      <div class="save-list">${body}</div>
      <p class="save-status" role="status">${esc(status)}</p>
    </div>`;
    wire();
  };

  // A question in place of the buttons, so nothing is overwritten or deleted by one stray tap.
  const confirmIn = (host: Element, text: string, yes: string, act: () => void) => {
    host.innerHTML = `<span class="ask">${esc(text)}</span><button data-yes>${esc(yes)}</button><button class="quiet" data-no>Cancel</button>`;
    host.querySelector('[data-yes]')!.addEventListener('click', act);
    host.querySelector('[data-no]')!.addEventListener('click', () => render());
  };

  const wire = () => {
    el.querySelector('[data-a=back]')!.addEventListener('click', () => { ui.close(); });
    el.querySelectorAll<HTMLButtonElement>('[data-load]').forEach((b) => b.addEventListener('click', () => {
      const m = metas.find((x) => x.id === b.dataset.load); if (m) { ui.close(true); o.onLoad?.(m); }
    }));
    el.querySelectorAll<HTMLButtonElement>('[data-del]').forEach((b) => b.addEventListener('click', () => {
      const m = metas.find((x) => x.id === b.dataset.del); if (!m) return;
      confirmIn(b.parentElement!, `Delete ${kindName(m).toLowerCase()}?`, 'Delete', async () => {
        await o.onDelete?.(m); metas = metas.filter((x) => x.id !== m.id); render('Deleted.');
      });
    }));
    el.querySelectorAll<HTMLButtonElement>('[data-save]').forEach((b) => b.addEventListener('click', () => {
      const slot = +b.dataset.save!;
      const c = o.activeCase ?? 1;
      const existing = metas.find((x) => x.caseId === c && x.kind === 'manual' && x.slot === slot);
      const go = async () => {
        render('Saving...');
        try {
          const m = await o.onSave!(slot);
          metas = [...metas.filter((x) => x.id !== m.id), m];
          render(`Saved to save ${slot + 1}.`);
        } catch (e) { render((e as Error).message || 'The save failed. Your earlier saves are unchanged.'); }
      };
      if (existing) confirmIn(b.parentElement!, `Overwrite save ${slot + 1}?`, 'Overwrite', go); else go();
    }));
    el.querySelectorAll<HTMLButtonElement>('[data-new]').forEach((b) => b.addEventListener('click', () => {
      const c = +b.dataset.new!;
      const used = caseMetas(c).length > 0;
      const go = () => { ui.close(true); o.onNewCase?.(c); };
      if (used) confirmIn(b.parentElement!, `Case ${c} and all its saves will be deleted.`, 'Start new night', go); else go();
    }));
  };

  render();
  el.addEventListener('click', (e) => { if (e.target === el) ui.close(); });
  ui.open(el, o.onBack);
  return el;
}
