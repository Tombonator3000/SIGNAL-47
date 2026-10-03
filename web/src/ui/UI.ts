export interface DocSpec {
  id: string;
  title: string;
  kind: 'hand' | 'printout' | 'typed';
  page: string;        // what is written on the object
  transcript: string;  // the readable transcription on the right
  stamp?: string;
}

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export class UI {
  root = document.getElementById('ui')!;
  modal: HTMLElement | null = null;
  private modalClose: (() => void) | null = null;
  private hud: HTMLElement;
  private promptEl: HTMLElement;
  private cross: HTMLElement;
  private toasts: HTMLElement;
  private clockEl: HTMLElement;
  private lockHint: HTMLElement;
  touch: HTMLElement;
  useBtn: HTMLButtonElement;
  fadeEl: HTMLElement;
  onModalChange?: (open: boolean) => void;
  onUse?: () => void;
  onNotes?: () => void;
  onPause?: () => void;

  constructor() {
    document.body.insertAdjacentHTML('beforeend', '<div class="grain"></div><div class="vignette"></div>');
    this.root.insertAdjacentHTML('beforeend', `
      <div class="hud passive" hidden><div class="crosshair"></div><div class="prompt"></div><div class="toasts"></div><div class="clockline"></div><div class="lockhint">Click to look around</div></div>
      <div class="touch" hidden>
        <div class="stick-base"><div class="stick-knob"></div></div>
        <button class="tbtn pause" aria-label="Pause">II</button>
        <button class="tbtn notes">Notes</button>
        <button class="tbtn use">Use</button>
      </div>
      <div class="fade passive"><div class="fade-text"></div></div>`);
    this.hud = this.root.querySelector('.hud')!;
    this.promptEl = this.root.querySelector('.prompt')!;
    this.cross = this.root.querySelector('.crosshair')!;
    this.toasts = this.root.querySelector('.toasts')!;
    this.clockEl = this.root.querySelector('.clockline')!;
    this.lockHint = this.root.querySelector('.lockhint')!;
    this.touch = this.root.querySelector('.touch')!;
    this.useBtn = this.root.querySelector('.touch .use')!;
    this.fadeEl = this.root.querySelector('.fade')!;
    this.useBtn.addEventListener('click', () => this.onUse?.());
    this.root.querySelector('.touch .notes')!.addEventListener('click', () => this.onNotes?.());
    this.root.querySelector('.touch .pause')!.addEventListener('click', () => this.onPause?.());
  }

  showHud(on: boolean, touch: boolean) {
    this.hud.hidden = !on;
    this.touch.hidden = !(on && touch);
  }

  prompt(label: string | null, touch: boolean) {
    this.cross.classList.toggle('active', !!label);
    if (touch) {
      this.useBtn.classList.toggle('ready', !!label);
      this.useBtn.textContent = label ? label : 'Use';
      this.promptEl.classList.remove('show');
    } else {
      this.promptEl.classList.toggle('show', !!label);
      if (label) this.promptEl.innerHTML = `<kbd>E</kbd>${esc(label)}`;
    }
  }

  clock(text: string) { if (this.clockEl.textContent !== text) this.clockEl.textContent = text; }
  hint(on: boolean) { this.lockHint.classList.toggle('show', on); }

  toast(text: string, secs = 3.2) {
    const el = document.createElement('div');
    el.className = 'toast'; el.textContent = text;
    this.toasts.appendChild(el);
    while (this.toasts.children.length > 3) this.toasts.firstElementChild!.remove();
    setTimeout(() => { el.classList.add('out'); setTimeout(() => el.remove(), 650); }, secs * 1000);
  }

  fade(on: boolean, text = '') {
    this.fadeEl.classList.toggle('on', on);
    (this.fadeEl.querySelector('.fade-text') as HTMLElement).textContent = text;
  }

  // ---------- modal handling ----------
  open(el: HTMLElement, onClose?: () => void) {
    this.close(true);
    this.root.appendChild(el);
    this.modal = el;
    this.modalClose = onClose ?? null;
    this.onModalChange?.(true);
  }
  close(silent = false) {
    if (!this.modal) return;
    this.modal.remove();
    this.modal = null;
    const cb = this.modalClose; this.modalClose = null;
    cb?.();
    if (!silent) this.onModalChange?.(false);
  }

  // ---------- screens ----------
  loading(msg = 'WARMING UP RECEIVERS') {
    const el = document.createElement('div');
    el.className = 'loading'; el.textContent = msg;
    this.root.appendChild(el);
    return el;
  }

  title(opts: { onStart: () => void; canContinue: boolean; onContinue: () => void; onSettings: () => void }) {
    const el = document.createElement('div');
    el.className = 'title-screen';
    el.innerHTML = `
      <h1>SIGNAL<span class="slash">/</span>47</h1>
      <div class="sub">New Mexico, 1986. Night shift at SARO.</div>
      <div class="menu">
        <button data-a="start">Start night shift</button>
        <button data-a="cont" ${opts.canContinue ? '' : 'disabled'}>Continue</button>
        <button data-a="set">Settings</button>
      </div>
      <p class="note rotate-hint">Turn your phone sideways for the wide view.</p>
      <p class="note">Prologue build for the three.js port. Headphones help. Desktop: WASD, mouse, E to use, Tab for notes. Phone: left thumb walks, right thumb looks, tap things to use them.</p>`;
    el.querySelector('[data-a=start]')!.addEventListener('click', opts.onStart);
    el.querySelector('[data-a=cont]')!.addEventListener('click', opts.onContinue);
    el.querySelector('[data-a=set]')!.addEventListener('click', opts.onSettings);
    this.root.appendChild(el);
    return el;
  }

  document(doc: DocSpec, onClose: () => void) {
    const el = document.createElement('div');
    el.className = 'docview';
    const page = doc.kind === 'printout'
      ? `<div class="page printout"><pre>${esc(doc.page).replace('SOURCE DISTANCE:  -39 LY', '<span class="hot">SOURCE DISTANCE:  -39 LY</span>')}</pre></div>`
      : `<div class="page">${doc.stamp ? `<div class="stamp">${esc(doc.stamp)}</div>` : ''}<div class="hand">${esc(doc.page)}</div></div>`;
    el.innerHTML = `
      <div class="page-wrap">${page}</div>
      <div class="text">
        <h3>${esc(doc.title)}</h3>
        ${doc.transcript.split('\n\n').map((p) => `<p>${esc(p)}</p>`).join('')}
        <div class="controls"><button data-a="back"><kbd>E</kbd>Put it back</button></div>
      </div>`;
    el.querySelector('[data-a=back]')!.addEventListener('click', () => this.close());
    el.addEventListener('click', (e) => { if (e.target === el) this.close(); });
    this.open(el, onClose);
  }

  notebook(tasks: { text: string; done: boolean }[], notes: string[], docs: DocSpec[], onDoc: (d: DocSpec) => void) {
    const el = document.createElement('div');
    el.className = 'overlay';
    el.innerHTML = `
      <div class="notebook">
        <button class="close">Close  [Tab]</button>
        <section>
          <h4>Tonight</h4>
          ${tasks.length ? `<ul>${tasks.map((t) => `<li class="${t.done ? 'done' : ''}"><span class="box">${t.done ? 'x' : '-'}</span>${esc(t.text)}</li>`).join('')}</ul>` : '<p class="empty">Nothing yet. Dale left the shift log on the desk.</p>'}
          <h4 style="margin-top:22px">Papers</h4>
          <div class="docs">${docs.length ? docs.map((d) => `<button data-d="${d.id}">${esc(d.title)}</button>`).join('') : '<p class="empty">None collected.</p>'}</div>
        </section>
        <section>
          <h4>Observations</h4>
          ${notes.length ? `<ul>${notes.map((n) => `<li>${esc(n)}</li>`).join('')}</ul>` : '<p class="empty">Nothing worth writing down. Yet.</p>'}
        </section>
      </div>`;
    el.querySelector('.close')!.addEventListener('click', () => this.close());
    el.addEventListener('click', (e) => { if (e.target === el) this.close(); });
    el.querySelectorAll<HTMLButtonElement>('[data-d]').forEach((b) => b.addEventListener('click', () => {
      const d = docs.find((x) => x.id === b.dataset.d); if (d) onDoc(d);
    }));
    this.open(el);
  }

  pause(o: { onResume: () => void; onTitle: () => void; volume: number; sens: number; onVolume: (v: number) => void; onSens: (v: number) => void; quality: 'high' | 'low'; onQuality: (q: 'high' | 'low') => void; title?: string; settingsOnly?: boolean }) {
    const el = document.createElement('div');
    el.className = 'overlay';
    el.innerHTML = `
      <div class="panel">
        <h3>${o.title ?? 'Paused'}</h3>
        <button class="opt" data-a="resume">${o.settingsOnly ? 'Back' : 'Resume'}</button>
        <div class="set"><label for="vol">Volume</label><input id="vol" type="range" min="0" max="1" step="0.01" value="${o.volume}"></div>
        <div class="set"><label for="sens">Look speed</label><input id="sens" type="range" min="0.3" max="2.5" step="0.05" value="${o.sens}"></div>
        <div class="set"><span class="lbl">Graphics</span><button class="opt qual" data-a="qual" aria-label="Graphics quality">${o.quality === 'high' ? 'High' : 'Low'}</button></div>
        ${o.settingsOnly ? '' : '<button class="opt" data-a="title">Quit to title</button>'}
        <p class="small">${o.settingsOnly ? 'Settings are kept on this device.' : 'Nothing in the control room moves on while the game is paused.'}</p>
      </div>`;
    el.querySelector('[data-a=resume]')!.addEventListener('click', () => this.close());
    el.querySelector('[data-a=title]')?.addEventListener('click', () => { this.close(true); o.onTitle(); });
    (el.querySelector('#vol') as HTMLInputElement).addEventListener('input', (e) => o.onVolume(+(e.target as HTMLInputElement).value));
    (el.querySelector('#sens') as HTMLInputElement).addEventListener('input', (e) => o.onSens(+(e.target as HTMLInputElement).value));
    let q = o.quality;
    const qb = el.querySelector('[data-a=qual]') as HTMLButtonElement;
    qb.addEventListener('click', () => { q = q === 'high' ? 'low' : 'high'; qb.textContent = q === 'high' ? 'High' : 'Low'; o.onQuality(q); });
    this.open(el, o.onResume);
  }

  endcard(onAgain: () => void, onTitle: () => void) {
    const el = document.createElement('div');
    el.className = 'endcard';
    el.innerHTML = `
      <h1>SIGNAL<span class="slash">/</span>47</h1>
      <div class="after">
        <p>Prologue: Night Shift.</p>
        <p>S-03 moved without a command. The service yard comes next.</p>
        <div class="row"><button data-a="again">Play again</button><button data-a="title">Title</button></div>
        <p class="credits">Music: "Signal to Noise" by Scott Buckley, CC BY 4.0, scottbuckley.com.au. Sound effects: Freesound users viertelnachvier, transitking, geraldfiebig, DarkShroom (CC0) and Kenney (CC0). Fonts: VT323, Special Elite, Reenie Beanie, Oswald (OFL / Apache 2.0).</p>
      </div>`;
    el.querySelector('[data-a=again]')!.addEventListener('click', onAgain);
    el.querySelector('[data-a=title]')!.addEventListener('click', onTitle);
    this.root.appendChild(el);
    return el;
  }
}
