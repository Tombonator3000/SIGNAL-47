import type { UI } from './UI';

// The developer menu: start any chapter, take the truck out any time, move the clock.
// Switched on with ?dev in the address and kept on this device; ?dev=0 switches it off.
// A small DEV button sits at the top of the screen (F2 on a keyboard). Runs started from
// here are never saved, so they cannot write over a case.

export const devOn = (() => {
  try {
    const q = new URLSearchParams(location.search);
    if (q.has('dev')) {
      if (q.get('dev') === '0') localStorage.removeItem('s47.dev');
      else localStorage.setItem('s47.dev', '1');
    }
    return localStorage.getItem('s47.dev') === '1';
  } catch { return false; }
})();

export type DevAction = { label: string; run: () => void; on?: boolean };
export type DevGroup = { title: string; note?: string; items: DevAction[] };

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export class DevMenu {
  button = document.createElement('button');

  /** groups: what the menu offers now (asked each time it opens). */
  constructor(private ui: UI, private groups: () => DevGroup[], private hooks: { onOpen: () => void; onClose: () => void }) {
    this.button.className = 'devbtn';
    this.button.textContent = 'DEV';
    this.button.setAttribute('aria-label', 'Developer menu');
    this.button.addEventListener('click', (e) => { e.stopPropagation(); this.toggle(); });
    document.body.appendChild(this.button);
  }

  get isOpen() { return !!this.ui.modal?.classList.contains('devmenu'); }
  toggle() { if (this.isOpen) this.ui.close(); else this.open(); }

  open() {
    const el = document.createElement('div');
    el.className = 'overlay devmenu';
    const groups = this.groups();
    el.innerHTML = `<div class="panel">
      <h3>Developer</h3>
      ${groups.map((g, gi) => `<section><h4>${esc(g.title)}</h4>${g.note ? `<p class="small">${esc(g.note)}</p>` : ''}
        <div class="devgrid">${g.items.map((it, i) => `<button class="opt dev${it.on ? ' on' : ''}" data-g="${gi}" data-i="${i}">${esc(it.label)}</button>`).join('')}</div></section>`).join('')}
      <button class="opt" data-a="close">Close</button>
      <p class="small">Runs started here are not saved. ?dev=0 in the address takes the menu away.</p>
    </div>`;
    el.querySelectorAll<HTMLButtonElement>('button.dev').forEach((b) => b.addEventListener('click', () => {
      const it = groups[+b.dataset.g!].items[+b.dataset.i!];
      this.ui.close();
      it.run();
    }));
    el.querySelector('[data-a=close]')!.addEventListener('click', () => this.ui.close());
    el.addEventListener('click', (e) => { if (e.target === el) this.ui.close(); });
    this.hooks.onOpen();
    this.ui.open(el, this.hooks.onClose);
  }
}
