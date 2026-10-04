export interface DocSpec {
  id: string;
  title: string;
  kind: 'hand' | 'printout' | 'typed' | 'photo' | 'map';
  page: string;        // what is written on the object
  transcript: string;  // the readable transcription on the right
  stamp?: string;
  image?: string;      // photo prints and maps: the image itself; typed papers: an attached drawing
  paper?: string;      // a picture of the paper itself under the writing (a 1947 field card, a letter)
  paperRatio?: number; // its width over height
}

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
// A line that starts with "~ " is pencil on the paper (story/nightshift.ts).
const pencil = (html: string) => html.replace(/^~ (.*)$/gm, '<span class="pencil">$1</span>');

const CREDITS = 'Music: "Signal to Noise" by Scott Buckley, CC BY 4.0, scottbuckley.com.au. Sound effects: Freesound users viertelnachvier, transitking, geraldfiebig, DarkShroom (CC0) and Kenney (CC0). Fonts: VT323, Special Elite, Reenie Beanie, Oswald (OFL / Apache 2.0).';

type NbTab = 'tasks' | 'notes' | 'findings' | 'papers' | 'photos';
// the picture setting: clean, the 1986 tape look, or a worn tape with tracking trouble
const PICTURE = { off: 'Clean', vhs: 'VHS', heavy: 'Worn VHS' } as const;

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
  onCamera?: () => void;
  private objEl: HTMLElement;
  private camBtn: HTMLButtonElement;
  private vf: HTMLElement;

  constructor() {
    document.body.insertAdjacentHTML('beforeend', '<div class="grain"></div><div class="vignette"></div>');
    this.root.insertAdjacentHTML('beforeend', `
      <div class="hud passive" hidden><div class="crosshair"></div><div class="prompt"></div><div class="toasts"></div><div class="clockline"></div><div class="objective"></div><div class="lockhint">Click to look around</div></div>
      <div class="viewfinder passive" hidden><div class="vf-frame"><i class="c tl"></i><i class="c tr"></i><i class="c bl"></i><i class="c br"></i><div class="vf-patch"></div>
        <div class="vf-top">SARO / FIELD CAMERA</div><div class="vf-no"></div><div class="vf-status"></div><div class="vf-hint"></div></div><div class="vf-flash"></div></div>
      <div class="touch" hidden>
        <div class="stick-base"><div class="stick-knob"></div></div>
        <button class="tbtn pause" aria-label="Pause">II</button>
        <button class="tbtn notes">Notes</button>
        <button class="tbtn camera" hidden>Camera</button>
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
    this.objEl = this.root.querySelector('.objective')!;
    this.camBtn = this.root.querySelector('.touch .camera')!;
    this.vf = this.root.querySelector('.viewfinder')!;
    this.camBtn.addEventListener('click', () => this.onCamera?.());
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
  objective(text: string | null) { const t = text ?? ''; if (this.objEl.textContent !== t) this.objEl.textContent = t; }
  cameraButton(on: boolean, raised = false) { this.camBtn.hidden = !on; this.camBtn.textContent = raised ? 'Lower' : 'Camera'; }

  // Field camera viewfinder. rect is the frame in CSS pixels (FieldCamera.frameRect).
  viewfinder(on: boolean, o?: { rect: { x: number; y: number; w: number; h: number }; frame: string; status: string; ok: boolean; touch: boolean }) {
    this.vf.hidden = !on;
    if (!on || !o) return;
    const f = this.vf.querySelector('.vf-frame') as HTMLElement;
    f.style.left = o.rect.x + 'px'; f.style.top = o.rect.y + 'px'; f.style.width = o.rect.w + 'px'; f.style.height = o.rect.h + 'px';
    (f.querySelector('.vf-no') as HTMLElement).textContent = o.frame;
    const st = f.querySelector('.vf-status') as HTMLElement;
    if (st.textContent !== o.status) st.textContent = o.status;
    st.classList.toggle('ok', o.ok);
    (f.querySelector('.vf-hint') as HTMLElement).textContent = o.touch ? 'CAMERA: LOWER   SHUTTER: EXPOSE' : 'C  LOWER     SPACE  SHUTTER';
  }
  shutterFlash() {
    const fl = this.vf.querySelector('.vf-flash') as HTMLElement;
    fl.classList.remove('on'); void fl.offsetWidth; fl.classList.add('on');
  }
  hint(on: boolean) { this.lockHint.classList.toggle('show', on); }

  toast(text: string, secs = 3.2) {
    // The same line twice in a row is noise.
    const live = [...this.toasts.children].filter((c) => !c.classList.contains('out')) as HTMLElement[];
    if (live.length && live[live.length - 1].textContent === text) return;
    const el = document.createElement('div');
    el.className = 'toast'; el.textContent = text;
    this.toasts.appendChild(el);
    live.push(el);
    // At most two lines on screen. Older ones fade out instead of vanishing mid-read.
    while (live.length > 2) this.dismiss(live.shift()!);
    setTimeout(() => this.dismiss(el), secs * 1000);
  }
  private dismiss(el: HTMLElement) {
    if (el.classList.contains('out')) return;
    el.classList.add('out');
    setTimeout(() => el.remove(), 650);
  }
  clearToasts() { this.toasts.replaceChildren(); }

  fade(on: boolean, text = '', small = false) {
    this.fadeEl.classList.toggle('on', on);
    const t = this.fadeEl.querySelector('.fade-text') as HTMLElement;
    t.textContent = text;
    t.classList.toggle('small', small);
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

  title(opts: { onStart: () => void; cont: string | null; onContinue: () => void; canLoad: boolean; onLoad: () => void; onSettings: () => void }) {
    const el = document.createElement('div');
    el.className = 'title-screen';
    el.innerHTML = `
      <h1>SIGNAL<span class="slash">/</span>47</h1>
      <div class="sub">New Mexico, 1986. Night shift at SARO.</div>
      <div class="menu">
        <button data-a="cont" ${opts.cont ? '' : 'disabled'}>Continue</button>
        ${opts.cont ? `<div class="cont-info">${esc(opts.cont)}</div>` : ''}
        <button data-a="start">New night</button>
        <button data-a="load" ${opts.canLoad ? '' : 'disabled'}>Load case</button>
        <button data-a="set">Settings</button>
      </div>
      <p class="note rotate-hint">Turn your phone sideways for the wide view.</p>
      <p class="note">The prologue and chapters one to four, from the control room to the survey station and room 6 at Sierra Motor Court. Headphones help. Desktop: WASD, mouse, E to use, Tab for notes, C for the camera. Phone: left thumb walks, right thumb looks, tap things to use them.</p>`;
    el.querySelector('[data-a=start]')!.addEventListener('click', opts.onStart);
    el.querySelector('[data-a=cont]')!.addEventListener('click', opts.onContinue);
    el.querySelector('[data-a=load]')!.addEventListener('click', opts.onLoad);
    el.querySelector('[data-a=set]')!.addEventListener('click', opts.onSettings);
    this.root.appendChild(el);
    (el.querySelector(opts.cont ? '[data-a=cont]' : '[data-a=start]') as HTMLButtonElement).focus({ preventScroll: true });
    return el;
  }

  /** Called when a paper is opened (the sound of the page; set by main). */
  onPaper?: () => void;
  document(doc: DocSpec, onClose: () => void) {
    this.onPaper?.();
    const el = document.createElement('div');
    el.className = 'docview';
    const page = doc.kind === 'printout'
      ? `<div class="page printout"><pre>${esc(doc.page).replace('SOURCE DISTANCE:  -39 LY', '<span class="hot">SOURCE DISTANCE:  -39 LY</span>')}</pre></div>`
      : (doc.kind === 'photo' || doc.kind === 'map') && doc.image
        ? `<div class="page photo${doc.kind === 'map' ? ' map' : ''}"><img alt="${esc(doc.title)}" src="${doc.image}"></div>`
        : doc.kind === 'typed'
          ? `<div class="page typed">${doc.stamp ? `<div class="stamp">${esc(doc.stamp)}</div>` : ''}<pre>${pencil(esc(doc.page))}</pre>${doc.image ? `<img class="attached" alt="" src="${doc.image}">` : ''}</div>`
          : `<div class="page${doc.paper ? ' on-paper' : ''}${(doc.paperRatio ?? 0) > 1 ? ' wide' : ''}"${doc.paper ? ` style="background-image:url('${doc.paper}');aspect-ratio:${doc.paperRatio ?? 0.77}"` : ''}>${doc.stamp ? `<div class="stamp">${esc(doc.stamp)}</div>` : ''}<div class="hand">${esc(doc.page)}</div></div>`;
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

  // The journal, in five tabs: tonight's tasks, the observations, the findings recorded
  // so far (with the evidence board once there is one), the papers and the photographs.
  // The last tab used is kept. After SPILLDESIGN.md ("Journal med fem faner").
  private nbTab: NbTab = 'tasks';
  notebook(tasks: { text: string; done: boolean }[], notes: string[], docs: DocSpec[], onDoc: (d: DocSpec) => void, heading = 'Tonight', onBoard?: () => void) {
    const el = document.createElement('div');
    el.className = 'overlay';
    const photos = docs.filter((d) => d.kind === 'photo' && d.image);
    const findings = docs.filter((d) => /^p\d\d$/.test(d.id) || d.id === 'finding');
    const papers = docs.filter((d) => !(d.kind === 'photo' && d.image) && !findings.includes(d));
    const kind = (d: DocSpec) => d.kind === 'hand' ? 'Handwritten' : d.kind === 'printout' ? 'Printout' : d.kind === 'typed' ? 'Typed' : d.kind === 'map' ? 'Map' : 'Print';
    const count = (n: number) => n ? ` <span class="n">${n}</span>` : '';
    const excerpt = (d: DocSpec) => {
      const one = (/^p\d\d$/.test(d.id) ? d.page ?? '' : d.transcript ?? d.page ?? '').replace(/^\S*\s*SUPPORTED \/ /, '').replace(/\s+/g, ' ');
      return one.length > 150 ? one.slice(0, 147).replace(/[\s.,;:]+\S*$/, '') + '...' : one;
    };
    const label = (d: DocSpec) => d.stamp && d.stamp.length <= 6 ? d.stamp : d.title.split(' / ')[0];
    el.innerHTML = `
      <div class="notebook">
        <div class="nb-tabs" role="tablist">
          <button role="tab" data-t="tasks">Tasks</button>
          <button role="tab" data-t="notes">Notes${count(notes.length)}</button>
          <button role="tab" data-t="findings">Findings${count(findings.length)}</button>
          <button role="tab" data-t="papers">Papers${count(papers.length)}</button>
          <button role="tab" data-t="photos">Photos${count(photos.length)}</button>
        </div>
        <button class="close">Close  [Tab]</button>
        <section class="nb-page" data-p="tasks">
          <h4>${esc(heading)}</h4>
          ${tasks.length ? `<ul>${tasks.map((t) => `<li class="${t.done ? 'done' : ''}"><span class="box">${t.done ? 'x' : '-'}</span>${esc(t.text)}</li>`).join('')}</ul>` : '<p class="empty">Nothing yet. Dale left the shift log on the desk.</p>'}
        </section>
        <section class="nb-page" data-p="notes">
          <h4>Observations</h4>
          ${notes.length ? `<ul class="nb-cols">${notes.map((n) => `<li>${esc(n)}</li>`).join('')}</ul>` : '<p class="empty">Nothing worth writing down. Yet.</p>'}
        </section>
        <section class="nb-page" data-p="findings">
          <h4>Findings</h4>
          ${onBoard ? '<button class="nb-board" data-a="board">Lay out the evidence board</button>' : ''}
          ${findings.length ? `<div class="findings">${findings.map((d) => `<button data-d="${d.id}"><b>${esc(label(d))}</b><span>${esc(d.title.split(' / ').slice(1).join(' / ') || d.title)}</span><em>${esc(excerpt(d))}</em></button>`).join('')}</div>` : '<p class="empty">Nothing recorded yet. A finding goes here once the records support it.</p>'}
        </section>
        <section class="nb-page nb-case" data-p="papers">
          <h4>Papers</h4>
          ${papers.length ? `<div class="cards">${papers.map((d) => `<button data-d="${d.id}" class="k-${d.kind}"><b>${esc(d.title)}</b><span>${kind(d)}${d.stamp ? ' / ' + esc(d.stamp) : ''}</span></button>`).join('')}</div>` : '<p class="empty">None collected.</p>'}
        </section>
        <section class="nb-page nb-case" data-p="photos">
          <h4>Photographs</h4>
          ${photos.length ? `<div class="thumbs">${photos.map((d) => `<button data-d="${d.id}"><img alt="" src="${d.image}"><span>${esc(d.title)}</span></button>`).join('')}</div>` : '<p class="empty">No exposures yet.</p>'}
        </section>
      </div>`;
    const show = (t: NbTab) => {
      this.nbTab = t;
      el.querySelectorAll<HTMLElement>('[data-p]').forEach((p) => p.classList.toggle('on', p.dataset.p === t));
      el.querySelectorAll<HTMLButtonElement>('[data-t]').forEach((b) => b.setAttribute('aria-selected', String(b.dataset.t === t)));
    };
    el.querySelectorAll<HTMLButtonElement>('[data-t]').forEach((b) => b.addEventListener('click', () => show(b.dataset.t as NbTab)));
    show(this.nbTab);
    el.querySelector('.close')!.addEventListener('click', () => this.close());
    el.addEventListener('click', (e) => { if (e.target === el) this.close(); });
    el.querySelector('[data-a=board]')?.addEventListener('click', () => { this.close(); onBoard?.(); });
    el.querySelectorAll<HTMLButtonElement>('[data-d]').forEach((b) => b.addEventListener('click', () => {
      const d = docs.find((x) => x.id === b.dataset.d); if (d) onDoc(d);
    }));
    this.open(el);
  }

  pause(o: {
    onResume: () => void; onTitle: () => void; volume: number; sens: number; onVolume: (v: number) => void; onSens: (v: number) => void;
    onSave?: () => void; onLoad?: () => void;
    quality: 'high' | 'low'; onQuality: (q: 'high' | 'low') => void; title?: string; settingsOnly?: boolean;
    picture: 'off' | 'vhs' | 'heavy'; onPicture: (p: 'off' | 'vhs' | 'heavy') => void;
    invertY: boolean; onInvertY: (v: boolean) => void; fov: number; onFov: (v: number) => void; largeText: boolean; onLargeText: (v: boolean) => void;
  }) {
    const el = document.createElement('div');
    el.className = 'overlay';
    el.innerHTML = `
      <div class="panel">
        <h3>${o.title ?? 'Paused'}</h3>
        <button class="opt" data-a="resume">${o.settingsOnly ? 'Back' : 'Resume'}</button>
        ${o.onSave ? '<button class="opt" data-a="save">Save case</button>' : ''}
        ${o.onLoad ? '<button class="opt" data-a="load">Load case</button>' : ''}
        <div class="set"><label for="vol">Volume</label><input id="vol" type="range" min="0" max="1" step="0.01" value="${o.volume}"></div>
        <div class="set"><label for="sens">Look speed</label><input id="sens" type="range" min="0.3" max="2.5" step="0.05" value="${o.sens}"></div>
        <div class="set"><label for="fov">Field of view</label><input id="fov" type="range" min="60" max="90" step="1" value="${o.fov}"></div>
        <div class="set"><span class="lbl">Invert look</span><button class="opt qual" data-a="inv" aria-label="Invert vertical look">${o.invertY ? 'On' : 'Off'}</button></div>
        <div class="set"><span class="lbl">Text</span><button class="opt qual" data-a="txt" aria-label="Text size">${o.largeText ? 'Large' : 'Normal'}</button></div>
        <div class="set"><span class="lbl">Graphics</span><button class="opt qual" data-a="qual" aria-label="Graphics quality">${o.quality === 'high' ? 'High' : 'Low'}</button></div>
        <div class="set"><span class="lbl">Picture</span><button class="opt qual" data-a="pic" aria-label="Picture: clean or video tape">${PICTURE[o.picture]}</button></div>
        ${o.settingsOnly ? '' : '<button class="opt" data-a="title">Quit to title</button>'}
        <p class="small">${o.settingsOnly ? 'Settings are kept on this device.' : 'Nothing in the control room moves on while the game is paused.'}</p>
      </div>`;
    el.querySelector('[data-a=resume]')!.addEventListener('click', () => this.close());
    el.querySelector('[data-a=title]')?.addEventListener('click', () => { this.close(true); o.onTitle(); });
    el.querySelector('[data-a=save]')?.addEventListener('click', () => o.onSave?.());
    el.querySelector('[data-a=load]')?.addEventListener('click', () => o.onLoad?.());
    (el.querySelector('#vol') as HTMLInputElement).addEventListener('input', (e) => o.onVolume(+(e.target as HTMLInputElement).value));
    (el.querySelector('#sens') as HTMLInputElement).addEventListener('input', (e) => o.onSens(+(e.target as HTMLInputElement).value));
    (el.querySelector('#fov') as HTMLInputElement).addEventListener('input', (e) => o.onFov(+(e.target as HTMLInputElement).value));
    let inv = o.invertY, big = o.largeText;
    el.querySelector('[data-a=inv]')!.addEventListener('click', (e) => { inv = !inv; (e.target as HTMLElement).textContent = inv ? 'On' : 'Off'; o.onInvertY(inv); });
    el.querySelector('[data-a=txt]')!.addEventListener('click', (e) => { big = !big; (e.target as HTMLElement).textContent = big ? 'Large' : 'Normal'; o.onLargeText(big); });
    let q = o.quality;
    const qb = el.querySelector('[data-a=qual]') as HTMLButtonElement;
    qb.addEventListener('click', () => { q = q === 'high' ? 'low' : 'high'; qb.textContent = q === 'high' ? 'High' : 'Low'; o.onQuality(q); });
    let pic = o.picture;
    const pb = el.querySelector('[data-a=pic]') as HTMLButtonElement;
    pb.addEventListener('click', () => { pic = pic === 'off' ? 'vhs' : pic === 'vhs' ? 'heavy' : 'off'; pb.textContent = PICTURE[pic]; o.onPicture(pic); });
    this.open(el, o.onResume);
  }

  // Between chapters: on the black, a line about what was just closed, then the next
  // chapter's title and the night's clock. The night does not stop for it.
  private cardEl: HTMLElement | null = null;
  chapterCard(o: { closed: string; recap: string; next: string; title: string; clock: string } | null) {
    if (this.cardEl) { const old = this.cardEl; old.classList.add('out'); setTimeout(() => old.remove(), 900); this.cardEl = null; }
    if (!o) return;
    const el = document.createElement('div');
    el.className = 'chapter-card';
    el.innerHTML = `
      <div class="cc-a"><div class="cc-closed">${esc(o.closed)}</div><p class="cc-recap">${esc(o.recap)}</p></div>
      <div class="cc-b"><div class="cc-next">${esc(o.next)}</div><h2>${esc(o.title)}</h2><div class="cc-clock">${esc(o.clock)}</div></div>`;
    document.body.appendChild(el);
    this.cardEl = el;
  }

  // Black card with the title, a few lines and buttons. Used where the built night ends.
  endcard(o: { lines: string[]; buttons: { label: string; on: () => void }[]; credits?: boolean }) {
    const el = document.createElement('div');
    el.className = o.credits || o.lines.length > 2 ? 'endcard long' : 'endcard';
    el.innerHTML = `
      <h1>SIGNAL<span class="slash">/</span>47</h1>
      <div class="after">
        ${o.lines.map((l) => `<p>${esc(l)}</p>`).join('')}
        <div class="row">${o.buttons.map((b, i) => `<button data-b="${i}">${esc(b.label)}</button>`).join('')}</div>
        ${o.credits ? `<p class="credits">${esc(CREDITS)}</p>` : ''}
      </div>`;
    el.querySelectorAll<HTMLButtonElement>('[data-b]').forEach((btn) => btn.addEventListener('click', () => o.buttons[+btn.dataset.b!].on()));
    this.root.appendChild(el);
    return el;
  }
}
