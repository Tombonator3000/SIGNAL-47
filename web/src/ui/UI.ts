import { albumPage, bindAlbum } from './album';
import type { AlbumView } from '../core/album';
import { KIT_ART, type KitArt } from './kitArt';
import { documentReadingParagraphs, type ReadAloud } from '../core/readAloud';
import type { EarlDialogue, EarlLine, EarlState } from '../core/earlDialogue';
import { characterLines, CHARACTER_LABELS, type NPCDialogue, type CharacterVoice } from '../core/npcDialogue';
import { BOUGHT_SFX } from '../core/Audio';

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
  /** Typed papers kept as separate sheets, each on its own paper (the exceptions binder on
   *  fanfold). `strip`: a run of continuous paper as long as the text; `term`: printer type. */
  sheets?: { page: string; paper: string; strip?: boolean; term?: boolean }[];
}

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
// A line that starts with "~ " is pencil on the paper (story/nightshift.ts).
const pencil = (html: string) => html.replace(/^~ (.*)$/gm, '<span class="pencil">$1</span>');

const CREDITS = `Music by Scott Buckley, CC BY 4.0, scottbuckley.com.au: "Signal to Noise", and excerpts of "Shadows and Dust", "Decoherence", "The Old Ones", "In Search of Solitude", "Neon" and "Hymn to the Dawn". Chris Zabriskie, CC BY 4.0, chriszabriskie.com: "Cylinder One", "Cylinder Two", "Cylinder Eight". Kevin MacLeod, CC BY 4.0, incompetech.com: "Floating Cities", "Ossuary 1 - A Beginning", "Ossuary 5 - Rest", "Long Note Four", "Lightless Dawn", "Phantom from Space", "Night on the Docks - Sax", and on the radio "Drankin Song", "Bama Country", "Cattails", "Anamalie". Shane Ivers, CC BY 4.0, silvermansound.com: "Signs To Nowhere". Sound effects: Freesound users inspire153, craigsmith, viertelnachvier, transitking, geraldfiebig, DarkShroom (CC0) and Kenney (CC0).${BOUGHT_SFX ? ' Ultimate SFX for Games, Function Loops.' : ''} Out in the dark: resourcemoon (CC BY 3.0), InSintesi (CC BY 4.0), superhalo, szegvari and JarredGibb (CC0). The national anthem on the radio: The United States Air Force Band (public domain). Fonts: VT323, Special Elite, Reenie Beanie, Oswald (OFL / Apache 2.0).`;

const QUALITY = { ultra: 'Ultra', high: 'High', low: 'Low' } as const;

type NbTab = 'tasks' | 'notes' | 'findings' | 'papers' | 'photos' | 'map' | 'kit' | 'people' | 'controls';
/** Something the player carries (the Kit tab). */
/** A thing the player has on them: its drawing (ui/kitArt.ts), and words written on the drawing. */
export interface KitItem { name: string; note: string; state?: string; art?: KitArt; tag?: string }
/** The notebook's map and kit (main.ts puts them together). */
/** Someone in the case: the name, who they are, a line about them, and their face (null while there is no picture). */
export interface PersonCard { name: string; role: string; note: string; portrait: string | null }
export interface NbExtras { map?: string; mapDescription?: string[]; /** a line in pencil under the map */ mapNote?: string; kit?: KitItem[]; people?: PersonCard[]; album?: AlbumView; /** a phone: its controls first */ touch?: boolean }
// Every control, for the notebook's Controls tab: the hints about them are said only once on a
// device (core/hints.ts), so they are all kept here. Keyboard and mouse, and a phone or tablet.
const CONTROLS: Record<'desk' | 'phone', { title: string; groups: { head: string; rows: [string, string][] }[] }> = {
  desk: { title: 'Keyboard and mouse', groups: [
    { head: 'On foot', rows: [
      ['W A S D, arrows', 'Walk. Shift: run.'],
      ['Mouse', 'Look. Click the picture first to take the mouse.'],
      ['E, Enter, click', 'Use what the crosshair is on. Put a paper back.'],
      ['Right mouse, held', 'A closer look at what the crosshair is on.'],
      ['V', 'Read the current or last description. Press again to stop.'],
      ['F', 'The flashlight, once you have it.'],
      ['C', 'Raise or lower the field camera.'],
      ['Space', 'The shutter, with the camera raised.'],
      ['Tab, N', 'The notebook. M: its map. I: the kit. H: these controls.'],
      ['Esc, P', 'Pause: settings, save and load.'],
    ] },
    { head: 'In the truck', rows: [
      ['W, S', 'Throttle and brake. S at a stop: reverse.'],
      ['A, D', 'Steer.'],
      ['Mouse', 'Look round the cab.'],
      ['R, or click the radio', 'The radio: on, the next station, off.'],
      ['E, stopped', 'Get out. When the engine has died: turn the key.'],
    ] },
  ] },
  phone: { title: 'Phone and tablet', groups: [
    { head: 'On foot', rows: [
      ['Left thumb', 'Walk.'],
      ['Right thumb', 'Look.'],
      ['Tap, or Use', 'Use a thing. Tap a paper to put it back.'],
      ['Zoom, held', 'A closer look.'],
      ['Light', 'The flashlight, once you have it.'],
      ['Camera', 'Raise or lower the field camera. Shutter: take the picture.'],
      ['Notes', 'The notebook.'],
      ['II', 'Pause: settings, save and load.'],
    ] },
    { head: 'In the truck', rows: [
      ['Left stick', 'Throttle, brake and steering.'],
      ['Radio, or tap it', 'The radio: on, the next station, off.'],
      ['Get out', 'Stopped: out of the truck. Turn the key: when the engine has died.'],
    ] },
  ] },
};
const controlsPage = (touch: boolean) => (touch ? ['phone', 'desk'] as const : ['desk', 'phone'] as const).map((k) => `
  <div class="nb-ctl"><h4>${CONTROLS[k].title}</h4>${CONTROLS[k].groups.map((g) => `<h5>${g.head}</h5>
    <dl>${g.rows.map(([key, what]) => `<dt>${esc(key)}</dt><dd>${esc(what)}</dd>`).join('')}</dl>`).join('')}</div>`).join('');
/** Initials for an empty frame: first and last name, without a title or "the". */
const initials = (name: string) => {
  const w = name.split(/\s+/).filter((x) => x && !/^(dr\.?|the)$/i.test(x));
  return ((w[0]?.[0] ?? '') + (w.length > 1 ? w[w.length - 1][0] : '')).toUpperCase();
};
// the picture setting: clean, the 1986 tape look, or a worn tape with tracking trouble
const PICTURE = { off: 'Clean', vhs: 'VHS', heavy: 'Worn VHS' } as const;

export class UI {
  /** Optional prerecorded synthetic narration, supplied by main after audio setup. */
  readAloud: ReadAloud | null = null;
  earlDialogue: EarlDialogue | null = null;
  narrator: ReadAloud | null = null;
  npcDialogue: NPCDialogue | null = null;
  get descriptionReader() { return this.narrator?.enabled ? this.narrator : this.readAloud?.enabled ? this.readAloud : null; }
  stopSpeech() { this.readAloud?.stop(); this.narrator?.stop(); this.earlDialogue?.stop(); this.npcDialogue?.stop(); }
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
  private altBtn: HTMLButtonElement;
  fadeEl: HTMLElement;
  onModalChange?: (open: boolean) => void;
  onUse?: () => void;
  /** The second thing the prompt offers (Q, or its button on a phone). */
  onAlt?: () => void;
  onNotes?: () => void;
  onPause?: () => void;
  onCamera?: () => void;
  /** The flashlight's button on a phone (Light), and the zoom held down (Zoom). */
  onLight?: () => void;
  onRadio?: () => void;
  onZoom?: (held: boolean) => void;
  private lightBtn!: HTMLButtonElement;
  private radioBtn!: HTMLButtonElement;
  private examineEl!: HTMLElement;
  private objEl: HTMLElement;
  private camBtn: HTMLButtonElement;
  private vf: HTMLElement;
  private descriptionObservers: MutationObserver[] = [];
  private descriptionButton!: HTMLButtonElement;
  private lastDescription = '';
  private examineText = '';
  private examineTimer: ReturnType<typeof setTimeout> | null = null;
  private descriptionActive = false;
  private examineActive = false;
  /** Automatic closer-look narration yields to a handset call or a cinematic. */
  canDescribe?: () => boolean;

  constructor() {
    document.body.insertAdjacentHTML('beforeend', '<div class="grain"></div><div class="vignette"></div>');
    this.root.insertAdjacentHTML('beforeend', `
      <div class="hud passive" hidden><div class="crosshair"></div><div class="prompt"></div><div class="examine"></div><button class="description-read" hidden data-a="read-description">Read last description [V]</button><div class="toasts"></div><div class="clockline"></div><div class="objective"></div><div class="lockhint">Click to look around</div></div>
      <div class="viewfinder passive" hidden><div class="vf-frame"><i class="c tl"></i><i class="c tr"></i><i class="c bl"></i><i class="c br"></i><div class="vf-patch"></div>
        <div class="vf-top">SARO / FIELD CAMERA</div><div class="vf-no"></div><div class="vf-free"></div><div class="vf-status"></div><div class="vf-hint"></div></div><div class="vf-flash"></div></div>
      <div class="touch" hidden>
        <div class="stick-base"><div class="stick-knob"></div></div>
        <button class="tbtn pause" aria-label="Pause">II</button>
        <button class="tbtn notes">Notes</button>
        <button class="tbtn camera" hidden>Camera</button>
        <button class="tbtn light" hidden>Light</button>
        <button class="tbtn radio" hidden>Radio</button>
        <button class="tbtn zoom">Zoom</button>
        <button class="tbtn use">Use</button>
        <button class="tbtn alt" hidden></button>
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
    this.lightBtn = this.root.querySelector('.touch .light')!;
    this.lightBtn.addEventListener('click', () => this.onLight?.());
    this.radioBtn = this.root.querySelector('.touch .radio')!;
    this.radioBtn.addEventListener('click', () => this.onRadio?.());
    this.examineEl = this.root.querySelector('.hud .examine')!;
    this.descriptionButton = this.root.querySelector('.description-read')!;
    this.descriptionButton.addEventListener('click', (e) => { e.stopPropagation(); this.readLastDescription(); });
    // Zoom is held, like the right mouse button
    const zoom = this.root.querySelector('.touch .zoom') as HTMLButtonElement;
    const hold = (on: boolean) => (e: Event) => { e.preventDefault(); zoom.classList.toggle('held', on); this.onZoom?.(on); };
    zoom.addEventListener('pointerdown', hold(true));
    for (const ev of ['pointerup', 'pointercancel', 'pointerleave']) zoom.addEventListener(ev, hold(false));
    this.useBtn.addEventListener('click', () => this.onUse?.());
    this.altBtn = this.root.querySelector('.touch .alt')!;
    this.altBtn.addEventListener('click', () => this.onAlt?.());
    this.root.querySelector('.touch .notes')!.addEventListener('click', () => this.onNotes?.());
    this.root.querySelector('.touch .pause')!.addEventListener('click', () => this.onPause?.());
  }

  showHud(on: boolean, touch: boolean) {
    this.hud.hidden = !on;
    this.touch.hidden = !(on && touch);
  }

  /** What Use does here (E, the Use button), or null; and a second thing that can be done at the
   *  same time (Q, its own button on a phone: story/truckTaken.ts, Get out while the key is wanted). */
  prompt(label: string | null, touch: boolean, alt: string | null = null) {
    this.cross.classList.toggle('active', !!label);
    const a = alt;
    if (this.altBtn.hidden !== (!a || !touch)) this.altBtn.hidden = !a || !touch;
    if (a && touch && this.altBtn.textContent !== a) this.altBtn.textContent = a;
    if (touch) {
      this.useBtn.classList.toggle('ready', !!label);
      this.useBtn.textContent = label ? label : 'Use';
      this.promptEl.classList.remove('show');
    } else {
      this.promptEl.classList.toggle('show', !!(label || a));
      // (the second thing on its own while Use offers nothing: Q Get out while the starter turns)
      if (label || a) this.promptEl.innerHTML = `${label ? `<kbd>E</kbd>${esc(label)}` : ''}${a ? `<span class="alt"${label ? '' : ' style="margin-left:0"'}><kbd>Q</kbd>${esc(a)}</span>` : ''}`;
    }
  }

  /** The corner clock: the night's (hh:mm), set every frame by the story. */
  clock(text: string) { this.clockBase = text; this.paintClock(); }
  /** The corner clock with seconds, over the night's (the warning before the time away, story/
   *  abduction.ts: the minute it began and the warning's own seconds, as the station clock showed
   *  seconds at 02:13), or null for the night's clock again. */
  clockSeconds(v: { hm: string; ss: string } | null) { this.clockHeld = v; this.paintClock(); }
  private clockBase = '';
  private clockHeld: { hm: string; ss: string } | null = null;
  private clockKey = '';
  private paintClock() {
    const h = this.clockHeld, key = h ? `${h.hm}:${h.ss}` : this.clockBase;
    if (key === this.clockKey) return;
    this.clockKey = key;
    if (h) this.clockEl.innerHTML = `${esc(h.hm)}<span class="ss">:${esc(h.ss)}</span>`;
    else this.clockEl.textContent = key;
    this.clockEl.classList.toggle('secs', !!h);
  }
  objective(text: string | null) { const t = text ?? ''; if (this.objEl.textContent !== t) this.objEl.textContent = t; }
  /** The flashlight's button on a phone: shown once there is a flashlight, lit while it is on. */
  lightButton(have: boolean, on: boolean) { this.lightBtn.hidden = !have; this.lightBtn.classList.toggle('on', on); }
  /** The truck's radio (in the cab only): on, next station, off. */
  radioButton(show: boolean, on: boolean) { if (this.radioBtn.hidden === show) this.radioBtn.hidden = !show; this.radioBtn.classList.toggle('on', on); }
  /** What a closer look says (the zoom), under the crosshair; null clears it. */
  examine(text: string | null) {
    const t = text ?? '';
    if (this.descriptionActive && this.canDescribe?.() === false) this.descriptionReader?.stop();
    if (this.examineEl.textContent !== t) this.examineEl.textContent = t;
    this.examineEl.classList.toggle('show', !!text);
    this.descriptionButton.hidden = !this.descriptionReader?.enabled || !(t || this.lastDescription) || !!this.modal || this.canDescribe?.() === false;
    if (this.examineText === t) return;
    this.examineText = t;
    if (this.examineTimer) clearTimeout(this.examineTimer);
    this.examineTimer = null;
    if (this.examineActive) this.descriptionReader?.stop();
    this.examineActive = false;
    this.updateDescriptionButton();
    if (t && this.descriptionReader?.enabled) this.examineTimer = setTimeout(() => {
      this.examineTimer = null;
      const reader = this.descriptionReader;
      if (!reader?.enabled || this.modal || !this.canDescribe?.() || reader.playing || reader.loading || !reader.available([t])) return;
      this.playDescription(t, true);
    }, 650);
  }
  private updateDescriptionButton() {
    this.descriptionButton.hidden = !this.descriptionReader?.enabled || !(this.examineText || this.lastDescription) || !!this.modal || this.canDescribe?.() === false;
    this.descriptionButton.textContent = this.descriptionActive ? 'Stop reading [V]' : this.examineText ? 'Read description [V]' : 'Read last description [V]';
    this.descriptionButton.setAttribute('aria-pressed', String(this.descriptionActive));
    this.descriptionButton.disabled = !this.descriptionActive && !this.descriptionReader?.available([this.examineText || this.lastDescription]);
    this.descriptionButton.title = this.descriptionButton.disabled ? 'No recording for this text.' : this.descriptionReader?.label ?? '';
  }
  private playDescription(text: string, automatic = false) {
    const reader = this.descriptionReader;
    if (!reader?.enabled || !text) return;
    this.onReadingStart?.(reader);
    void reader.play([text], (state) => {
      this.descriptionActive = state === 'playing' || state === 'loading';
      this.examineActive = automatic && this.descriptionActive;
      this.updateDescriptionButton();
    });
  }
  /** A gesture-accessible repeat, also available after a short toast has disappeared. */
  readLastDescription() {
    if (!this.descriptionReader?.enabled || this.modal || this.canDescribe?.() === false) return;
    if (this.descriptionActive) this.descriptionReader.stop();
    else this.playDescription(this.examineText || this.lastDescription);
  }
  cameraButton(on: boolean, raised = false) { this.camBtn.hidden = !on; this.camBtn.textContent = raised ? 'Lower' : 'Camera'; }

  // Field camera viewfinder. rect is the frame in CSS pixels (FieldCamera.frameRect). `free`:
  // the album's free frames (core/album.ts); while one is ready the case frame's line dims.
  viewfinder(on: boolean, o?: { rect: { x: number; y: number; w: number; h: number }; frame: string; status: string; ok: boolean; touch: boolean; free?: { text: string; ok: boolean } }) {
    this.vf.hidden = !on;
    if (!on || !o) return;
    const f = this.vf.querySelector('.vf-frame') as HTMLElement;
    f.style.left = o.rect.x + 'px'; f.style.top = o.rect.y + 'px'; f.style.width = o.rect.w + 'px'; f.style.height = o.rect.h + 'px';
    (f.querySelector('.vf-no') as HTMLElement).textContent = o.frame;
    const st = f.querySelector('.vf-status') as HTMLElement;
    if (st.textContent !== o.status) st.textContent = o.status;
    st.classList.toggle('ok', o.ok);
    st.classList.toggle('far', !!o.free?.ok);
    const fr = f.querySelector('.vf-free') as HTMLElement, ft = o.free?.text ?? '';
    if (fr.textContent !== ft) fr.textContent = ft;
    fr.classList.toggle('ok', !!o.free?.ok);
    (f.querySelector('.vf-hint') as HTMLElement).textContent = o.touch ? 'CAMERA: LOWER   SHUTTER: EXPOSE' : 'C  LOWER     SPACE  SHUTTER';
  }
  shutterFlash() {
    const fl = this.vf.querySelector('.vf-flash') as HTMLElement;
    fl.classList.remove('on'); void fl.offsetWidth; fl.classList.add('on');
  }
  hint(on: boolean) { this.lockHint.classList.toggle('show', on); }

  toast(text: string, secs = 3.2, clock?: () => number) {
    // Captions already have their own radio/telephone delivery. Do not turn them
    // into a second narrator or replace the last environmental description.
    if (!/^(?:RADIO(?:,|:)|CB,|WARD:|DR\. WARD:|NORA:|N\. VEGA:|THE LINE:|YOU:|TOM[ÁA]S:)/i.test(text)) {
      this.lastDescription = text;
      this.updateDescriptionButton();
    }
    // The same line twice in a row is noise.
    const live = [...this.toasts.children].filter((c) => !c.classList.contains('out')) as HTMLElement[];
    if (live.length && live[live.length - 1].textContent === text) return;
    const el = document.createElement('div');
    el.className = 'toast'; el.textContent = text;
    this.toasts.appendChild(el);
    live.push(el);
    // At most two lines on screen. Older ones fade out instead of vanishing mid-read.
    while (live.length > 2) this.dismiss(live.shift()!);
    if (clock) {
      // Recorded radio captions share the suspended AudioContext clock, so a
      // long pause cannot consume the text while its recording is still paused.
      const end = clock() + secs;
      const expire = () => {
        if (!el.isConnected || el.classList.contains('out')) return;
        const remaining = end - clock();
        if (remaining <= 0) this.dismiss(el);
        else setTimeout(expire, Math.max(16, Math.min(200, remaining * 1000)));
      };
      setTimeout(expire, Math.min(200, secs * 1000));
    } else setTimeout(() => this.dismiss(el), secs * 1000);
  }
  private dismiss(el: HTMLElement) {
    if (el.classList.contains('out')) return;
    el.classList.add('out');
    setTimeout(() => el.remove(), 650);
  }
  clearToasts() { this.toasts.replaceChildren(); this.lastDescription = ''; this.examine(null); this.updateDescriptionButton(); }

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
    const panel = el.querySelector<HTMLElement>('.labpanel');
    if (panel) this.descriptionReadings(panel, '.lp-text, .lp-small, .lp-q, .lp-paper p, .lp-steps span, .lp-card span, figcaption' + (panel.querySelector('.mapov') ? ', .lp-sub[data-status]' : ''));
    const receiver = el.querySelector<HTMLElement>('.crt .lcol');
    if (receiver) this.descriptionReadings(receiver, '.refcard', 'receiver-description');
    const board = el.querySelector<HTMLElement>('.board-wrap');
    if (board) this.descriptionReadings(board, '.board-say, .bcard-where, .bcard:not(.open) .bcard-sum, .bcard.open .bcard-full p, .bline > span, .bq-text, .bq-done, .bq-else, .bq-slots li.on, .bnote > p', 'board-description');
  }
  close(silent = false) {
    for (const observer of this.descriptionObservers) observer.disconnect();
    this.descriptionObservers = [];
    if (this.examineTimer) clearTimeout(this.examineTimer);
    this.examineTimer = null;
    this.stopSpeech(); this.chapterCard(null);
    if (!this.modal) return;
    this.modal.remove();
    this.modal = null;
    const cb = this.modalClose; this.modalClose = null;
    cb?.();
    if (!silent) this.onModalChange?.(false);
    this.updateDescriptionButton();
  }

  // ---------- screens ----------
  loading(msg = 'WARMING UP RECEIVERS') {
    const el = document.createElement('div');
    el.className = 'loading'; el.textContent = msg;
    this.root.appendChild(el);
    return el;
  }

  title(opts: { onStart: () => void; cont: string | null; onContinue: () => void; canLoad: boolean; onLoad: () => void; onSettings: () => void;
    flash: { calm: boolean; onPick: (calm: boolean) => void }; gate?: boolean; touch?: boolean }) {
    const el = document.createElement('div');
    el.className = 'title-screen' + (opts.gate ? ' gated' : '');
    el.innerHTML = `
      <h1>SIGNAL<span class="slash">/</span>47</h1>
      <div class="sub">New Mexico, 1986. Night shift at SARO.</div>
      <div class="gate">${opts.touch ? 'Tap to begin' : 'Click or press any key'}</div>
      <div class="menu">
        <button data-a="cont" ${opts.cont ? '' : 'disabled'}>Continue</button>
        ${opts.cont ? `<div class="cont-info">${esc(opts.cont)}</div>` : ''}
        <button data-a="start">New night</button>
        <button data-a="load" ${opts.canLoad ? '' : 'disabled'}>Load case</button>
        <button data-a="set">Settings</button>
      </div>
      <p class="note flash-note">Near the end of the night there is a short sequence of flashing images.
        <span class="pick"><button data-a="fl-keep" aria-pressed="${!opts.flash.calm}">Flashes</button><button data-a="fl-calm" aria-pressed="${opts.flash.calm}">Slow fades</button></span></p>
      <p class="note rotate-hint">Turn your phone sideways for the wide view.</p>
      <p class="note">The whole night, from the control room at 23:41 to the old Roswell road at dawn. Headphones help. Desktop: WASD, mouse, E to use, Tab for notes, B for the case board, C for the camera, F for a flashlight once you have one, right mouse for a closer look, H for all of them. Phone: left thumb walks, right thumb looks, tap things to use them. The notebook has every control on its Controls page.</p>`;
    const pick = (calm: boolean) => {
      el.querySelector('[data-a=fl-keep]')!.setAttribute('aria-pressed', String(!calm));
      el.querySelector('[data-a=fl-calm]')!.setAttribute('aria-pressed', String(calm));
      opts.flash.onPick(calm);
    };
    el.querySelector('[data-a=fl-keep]')!.addEventListener('click', () => pick(false));
    el.querySelector('[data-a=fl-calm]')!.addEventListener('click', () => pick(true));
    el.querySelector('[data-a=start]')!.addEventListener('click', opts.onStart);
    el.querySelector('[data-a=cont]')!.addEventListener('click', opts.onContinue);
    el.querySelector('[data-a=load]')!.addEventListener('click', opts.onLoad);
    el.querySelector('[data-a=set]')!.addEventListener('click', opts.onSettings);
    this.root.appendChild(el);
    if (!opts.gate) (el.querySelector(opts.cont ? '[data-a=cont]' : '[data-a=start]') as HTMLButtonElement).focus({ preventScroll: true });
    return el;
  }
  /** The first tap or key on the title screen: the menu comes in (with the music, main.ts). */
  ungate(el: HTMLElement) {
    if (!el.classList.contains('gated')) return;
    el.classList.remove('gated');
    (el.querySelector<HTMLButtonElement>('[data-a=cont]:not([disabled])') ?? el.querySelector<HTMLButtonElement>('[data-a=start]'))?.focus({ preventScroll: true });
  }

  /** Called when a paper is opened (the sound of the page; set by main). */
  onPaper?: () => void;
  onReadingStart?: (reader?: ReadAloud) => void;
  /** A page of the notebook has been turned to (main.ts: the map, the first time the lines meet on it). */
  onNotebookTab?: (tab: string) => void;
  onCharacterStart?: () => void;
  onEarlStart?: () => void;
  characterReadingControls(parent: HTMLElement, voice: CharacterVoice, lines: EarlLine[]) {
    const reader = this.npcDialogue;
    if (!reader?.enabled) return null;
    const speech = characterLines(voice, lines);
    if (!speech.length) return null;
    const parodyLabel = voice === 'bacall' && speech.some((line) => line.who === 'THE LINE')
      ? 'Synthetic Lauren Bacall / Vincent Price parodies' : `Synthetic ${CHARACTER_LABELS[voice]} parody`;
    const input = { voice, lines };
    const box = document.createElement('div'); box.className = 'read-aloud';
    box.dataset.readingKind = 'character-dialogue'; box.dataset.voice = voice;
    box.dataset.readingTexts = JSON.stringify(speech.map((line) => line.text));
    box.innerHTML = '<button class="lp-btn" data-a="character">Replay voice</button><span data-reading-status role="status" aria-live="polite"></span>';
    const button = box.querySelector('button')!, status = box.querySelector<HTMLElement>('[data-reading-status]')!;
    button.disabled = !reader.available(input); let active = false;
    const update = (state: EarlState) => {
      active = state === 'loading' || state === 'playing';
      button.textContent = active ? 'Stop voice' : 'Replay voice';
      button.setAttribute('aria-pressed', String(active));
      status.textContent = state === 'loading' ? 'Loading voice…' : state === 'error' ? 'Recording could not be played. Try again.'
        : button.disabled ? 'No recording for this reply.' : parodyLabel;
    };
    const play = () => { this.onCharacterStart?.(); void reader.play(input, update); };
    button.addEventListener('click', () => { if (active) reader.stop(); else play(); });
    update('idle'); parent.appendChild(box);
    // a reply without a recording stays text, and says so (not a failed playback), as Earl's does
    return button.disabled ? null : play;
  }
  /** Only Chapter4 calls this for its authored N. VEGA quote replies/toasts. */
  speakNora(text: string) {
    if (!this.npcDialogue?.enabled) return;
    const match = /^N\. VEGA: "([\s\S]*)"$/.exec(text);
    if (!match) return;
    this.onCharacterStart?.();
    void this.npcDialogue.play({ voice: 'bacall', lines: [{ who: 'N. VEGA', text: match[1] }] });
  }
  /** Controls belong only to the live conversation with Earl, never papers or People. */
  earlReadingControls(parent: HTMLElement, lines: EarlLine[]) {
    const reader = this.earlDialogue;
    if (!reader?.enabled) return null;
    const box = document.createElement('div');
    box.className = 'read-aloud'; box.dataset.readingKind = 'earl-dialogue';
    box.dataset.readingTexts = JSON.stringify(lines.filter((line) => line.who === 'EARL').map((line) => line.text));
    box.innerHTML = '<button class="lp-btn" data-a="earl">Replay Earl</button><span data-reading-status role="status" aria-live="polite"></span>';
    const button = box.querySelector('button')!, status = box.querySelector<HTMLElement>('[data-reading-status]')!;
    button.disabled = !reader.available(lines);
    let active = false;
    const update = (state: EarlState) => {
      active = state === 'loading' || state === 'playing';
      button.textContent = active ? 'Stop Earl' : 'Replay Earl';
      button.setAttribute('aria-pressed', String(active));
      status.textContent = state === 'loading' ? 'Loading Earl…' : state === 'error' ? 'Recording could not be played. Try again.'
        : button.disabled ? 'No recording for this reply.' : 'Synthetic Art Bell parody';
    };
    const play = () => {
      this.onEarlStart?.();
      void reader.play(lines, update);
    };
    button.addEventListener('click', () => { if (active) reader.stop(); else play(); });
    update('idle'); parent.appendChild(box);
    // a reply without a recording stays text, and says so (not a failed playback)
    return button.disabled ? null : play;
  }
  /** Explicit playback only: opening a paper or refreshing a conversation stays quiet. */
  readingControls(parent: HTMLElement, texts: string[], label = 'Read aloud', kind = 'conversation') {
    const reader = kind === 'document' || kind === 'conversation' ? this.readAloud : this.descriptionReader;
    if (!reader?.enabled) return;
    const box = document.createElement('div');
    box.className = 'read-aloud';
    box.dataset.readingKind = kind;
    box.dataset.readingTexts = JSON.stringify(texts);
    box.innerHTML = '<button class="lp-btn" data-a="narrate">Read aloud</button><span data-reading-status role="status" aria-live="polite"></span>';
    const button = box.querySelector('button')!;
    button.textContent = label;
    const status = box.querySelector<HTMLElement>('[data-reading-status]')!;
    const available = reader.available(texts);
    button.disabled = !available;
    status.textContent = available ? reader.label : 'No recording for this text.';
    let active = false;
    const update = (state: 'loading' | 'playing' | 'idle' | 'error') => {
      active = state === 'loading' || state === 'playing';
      button.textContent = active ? 'Stop reading' : label;
      button.setAttribute('aria-pressed', String(active));
      status.textContent = state === 'loading' ? 'Loading recording…' : state === 'playing' ? reader.label : state === 'error' ? 'Recording could not be played. Try again.' : reader.label;
    };
    button.setAttribute('aria-pressed', 'false');
    button.addEventListener('click', () => {
      if (active) reader.stop();
      else { this.onReadingStart?.(reader); void reader.play(texts, update); }
    });
    parent.insertBefore(box, parent.querySelector('.controls'));
  }
  /** Read the descriptions currently visible in a changing puzzle or notebook.
   *  Observe text changes, not frames: hints, answers and tab changes replace the
   *  old queue immediately, without restarting a recording on every redraw. */
  descriptionReadings(parent: HTMLElement, selector: string, kind = 'panel-description', extra?: () => string[]) {
    const reader = this.descriptionReader;
    if (!reader?.enabled) return;
    let key = '', box: HTMLElement | null = null;
    const refresh = () => {
      const texts = [...parent.querySelectorAll<HTMLElement>(selector)]
        .filter((node) => !node.closest('.read-aloud, [data-character-quote], [hidden], .nb-page:not(.on)') && getComputedStyle(node).display !== 'none')
        .map((node) => {
          if (node.dataset.readingText) return node.dataset.readingText;
          const copy = node.cloneNode(true) as HTMLElement;
          copy.querySelectorAll('.box, .noface').forEach((mark) => mark.remove());
          copy.querySelectorAll('br').forEach((br) => br.replaceWith('\n'));
          return copy.textContent?.trim() ?? '';
        }).flatMap(documentReadingParagraphs).filter(Boolean);
      texts.push(...(extra?.() ?? []));
      const next = JSON.stringify(texts);
      if (next === key && (!texts.length || box?.parentElement === parent)) return;
      key = next;
      if (box?.querySelector('[aria-pressed=true]')) reader.stop();
      box?.remove(); box = null;
      if (!texts.length) return;
      this.readingControls(parent, texts, 'Read description', kind);
      box = parent.querySelector<HTMLElement>(`:scope > .read-aloud[data-reading-kind="${kind}"]`);
    };
    refresh();
    const observer = new MutationObserver((records) => {
      if (records.every((record) => (record.target instanceof Element ? record.target : record.target.parentElement)?.closest('.read-aloud, svg, [data-labels]'))) return;
      refresh();
    });
    observer.observe(parent, { childList: true, subtree: true, characterData: true, attributes: true, attributeFilter: ['class', 'hidden'] });
    this.descriptionObservers.push(observer);
    return () => {
      observer.disconnect();
      this.descriptionObservers = this.descriptionObservers.filter((item) => item !== observer);
      if (box?.querySelector('[aria-pressed=true]')) reader.stop();
    };
  }
  document(doc: DocSpec, onClose: () => void) {
    this.onPaper?.();
    const el = document.createElement('div');
    el.className = 'docview';
    const page = doc.kind === 'printout'
      ? `<div class="page printout"><pre>${esc(doc.page).replace('SOURCE DISTANCE:  -39 LY', '<span class="hot">SOURCE DISTANCE:  -39 LY</span>')}</pre></div>`
      : (doc.kind === 'photo' || doc.kind === 'map') && doc.image
        ? `<div class="page photo${doc.kind === 'map' ? ' map' : ''}"><img alt="${esc(doc.title)}" src="${doc.image}"></div>`
        : doc.kind === 'typed' && doc.sheets
          ? `<div class="sheets">${doc.sheets.map((sh, i) => `<div class="page sheet${sh.strip ? ' strip' : ''}${sh.term ? ' term' : ''}" style="background-image:url('${sh.paper}')">${i === 0 && doc.stamp ? `<div class="stamp">${esc(doc.stamp)}</div>` : ''}<pre>${pencil(esc(sh.page))}</pre></div>`).join('')}</div>`
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
    const side = el.querySelector<HTMLElement>('.text')!;
    const paragraphs = (text: string) => text.split(/\n\s*\n/).filter((p) => p.trim());
    if (doc.page.trim()) this.readingControls(side, documentReadingParagraphs(doc.page), 'Read document', 'document');
    this.readingControls(side, paragraphs(doc.transcript), 'Read description', 'description');
  }

  // The journal, in five tabs: tonight's tasks, the observations, the findings recorded
  // so far, the papers and the photographs. The last tab used is kept. After SPILLDESIGN.md
  // ("Journal med fem faner"). First in the row is Board: the case board with everything in
  // the journal on it (story/caseBoard.ts), where the findings are made.
  private nbTab: NbTab = 'tasks';
  /** The tab the notebook opens at next (M for the map, I for the kit, H for the controls). */
  notebookAt(t: NbTab) { this.nbTab = t; }
  notebook(tasks: { text: string; done: boolean }[], notes: string[], docs: DocSpec[], onDoc: (d: DocSpec) => void, heading = 'Tonight', onBoard?: () => void, extras: NbExtras = {}) {
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
          ${onBoard ? '<button class="nb-boardtab" data-a="board" aria-label="Open the case board">Board</button>' : ''}
          <button role="tab" data-t="tasks">Tasks</button>
          <button role="tab" data-t="notes">Notes${count(notes.length)}</button>
          <button role="tab" data-t="findings">Findings${count(findings.length)}</button>
          <button role="tab" data-t="papers">Papers${count(papers.length)}</button>
          <button role="tab" data-t="photos">Album${count(photos.length + (extras.album?.shots.length ?? 0))}</button>
          ${extras.map ? '<button role="tab" data-t="map">Map</button>' : ''}
          ${extras.kit ? '<button role="tab" data-t="kit">Kit</button>' : ''}
          ${extras.people ? `<button role="tab" data-t="people">People${count(extras.people.length)}</button>` : ''}
          <button role="tab" data-t="controls">Controls</button>
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
          ${onBoard ? '<button class="nb-board" data-a="board">Lay it all out on the case board</button>' : ''}
          ${findings.length ? `<div class="findings">${findings.map((d) => `<button data-d="${d.id}"><b>${esc(label(d))}</b><span>${esc(d.title.split(' / ').slice(1).join(' / ') || d.title)}</span><em>${esc(excerpt(d))}</em></button>`).join('')}</div>` : '<p class="empty">Nothing recorded yet. A finding goes here once the records support it.</p>'}
        </section>
        <section class="nb-page nb-case" data-p="papers">
          <h4>Papers</h4>
          ${papers.length ? `<div class="cards">${papers.map((d) => `<button data-d="${d.id}" class="k-${d.kind}"><b>${esc(d.title)}</b><span>${kind(d)}${d.stamp ? ' / ' + esc(d.stamp) : ''}</span></button>`).join('')}</div>` : '<p class="empty">None collected.</p>'}
        </section>
        <section class="nb-page nb-case nb-album" data-p="photos">${albumPage(photos, extras.album)}</section>
        ${extras.map ? `<section class="nb-page nb-map" data-p="map"><img alt="A pencil sketch of the area round SARO" src="${extras.map}">${extras.mapNote ? `<p class="nb-mapnote">${esc(extras.mapNote)}</p>` : ''}</section>` : ''}
        ${extras.kit ? `<section class="nb-page nb-kit" data-p="kit">
          <h4>What I have on me</h4>
          <ul class="kit">${extras.kit.map((k) => `<li${k.art ? ` class="has-art"` : ''}>${k.art ? `<span class="kit-art kit-${k.art}"><img alt="" src="${KIT_ART[k.art]}">${k.tag ? `<em>${esc(k.tag)}</em>` : ''}</span>` : ''}<b>${esc(k.name)}</b>${k.state ? `<i>${esc(k.state)}</i>` : ''}<span>${esc(k.note)}</span></li>`).join('')}</ul>
        </section>` : ''}
        ${extras.people ? `<section class="nb-page nb-people" data-p="people">
          <h4>People in the case</h4>
          <ul class="people">${extras.people.map((p) => `<li>${p.portrait ? `<img alt="" src="${p.portrait}">` : `<span class="noface">${esc(initials(p.name))}</span>`}<b>${esc(p.name)}</b><i>${esc(p.role)}</i><span>${esc(p.note)}</span></li>`).join('')}</ul>
        </section>` : ''}
        <section class="nb-page nb-case nb-controls" data-p="controls">${controlsPage(!!extras.touch)}</section>
      </div>`;
    const show = (t: NbTab) => {
      this.stopSpeech();
      this.nbTab = t;
      el.querySelectorAll<HTMLElement>('[data-p]').forEach((p) => p.classList.toggle('on', p.dataset.p === t));
      el.querySelectorAll<HTMLButtonElement>('[data-t]').forEach((b) => b.setAttribute('aria-selected', String(b.dataset.t === t)));
      this.onNotebookTab?.(t);
    };
    el.querySelectorAll<HTMLButtonElement>('[data-t]').forEach((b) => b.addEventListener('click', () => show(b.dataset.t as NbTab)));
    show(el.querySelector(`[data-p="${this.nbTab}"]`) ? this.nbTab : 'tasks');
    el.querySelector('.close')!.addEventListener('click', () => this.close());
    el.addEventListener('click', (e) => { if (e.target === el) this.close(); });
    el.querySelectorAll('[data-a=board]').forEach((b) => b.addEventListener('click', () => { this.close(); onBoard?.(); }));
    el.querySelectorAll<HTMLButtonElement>('[data-d]').forEach((b) => b.addEventListener('click', () => {
      const d = docs.find((x) => x.id === b.dataset.d); if (d) onDoc(d);
    }));
    bindAlbum(el, extras.album, {
      show: (view) => this.descriptionReadings(view, '.al-cap b, .al-cap small', 'photo-description'),
      change: () => this.stopSpeech(),
    });
    this.open(el);
    this.descriptionReadings(el.querySelector('.notebook')!, '.nb-page.on[data-p=tasks] li, .nb-page.on[data-p=notes] li, .nb-page.on .empty, .nb-page.on[data-p=kit] li > b, .nb-page.on[data-p=kit] li > i, .nb-page.on[data-p=kit] li > span:not(.kit-art), .nb-page.on[data-p=people] li > b, .nb-page.on[data-p=people] li > i, .nb-page.on[data-p=people] li > span:not(.noface)', 'notebook-description',
      () => this.nbTab === 'map' ? extras.mapDescription ?? [] : []);
  }

  pause(o: {
    onResume: () => void; onTitle: () => void; volume: number; sens: number; onVolume: (v: number) => void; onSens: (v: number) => void;
    music: number; onMusic: (v: number) => void; radio: number; onRadio: (v: number) => void;
    characters: boolean; onCharacters: (v: boolean) => void;
    serling: boolean; onSerling: (v: boolean) => void;
    kinski: boolean; onKinski: (v: boolean) => void;
    artBell: boolean; onArtBell: (v: boolean) => void;
    radioCast: boolean; onRadioCast: (v: boolean) => void;
    onSave?: () => void; onLoad?: () => void;
    quality: 'ultra' | 'high' | 'low'; onQuality: (q: 'ultra' | 'high' | 'low') => void; ultraOk?: boolean; title?: string; settingsOnly?: boolean;
    picture: 'off' | 'vhs' | 'heavy'; onPicture: (p: 'off' | 'vhs' | 'heavy') => void;
    invertY: boolean; onInvertY: (v: boolean) => void; fov: number; onFov: (v: number) => void; largeText: boolean; onLargeText: (v: boolean) => void;
    calmFlash: boolean; onCalmFlash: (v: boolean) => void; stillShots: boolean; onStillShots: (v: boolean) => void;
    abduct?: boolean; onAbduct?: (v: boolean) => void;
  }) {
    const el = document.createElement('div');
    el.className = 'overlay';
    el.innerHTML = `
      <div class="panel settings-panel">
        <h3>${o.title ?? 'Paused'}</h3>
        <button class="opt" data-a="resume">${o.settingsOnly ? 'Back' : 'Resume'}</button>
        ${o.onSave ? '<button class="opt" data-a="save">Save case</button>' : ''}
        ${o.onLoad ? '<button class="opt" data-a="load">Load case</button>' : ''}
        <div class="set"><label for="vol">Volume</label><input id="vol" type="range" min="0" max="1" step="0.01" value="${o.volume}"></div>
        <div class="set"><label for="mus">Music</label><input id="mus" type="range" min="0" max="1" step="0.01" value="${o.music}"></div>
        <div class="set"><label for="rad">Radio</label><input id="rad" type="range" min="0" max="1" step="0.01" value="${o.radio}"></div>
        <div class="set"><span class="lbl">Art Bell radio host</span><button class="opt qual" data-a="artbell" aria-label="Art Bell radio host" aria-pressed="${o.artBell}">${o.artBell ? 'On' : 'Off'}</button></div>
        <p class="small reading-note">Synthetic Art Bell parody for Earl on Night Line, AM 1420, and when you speak with him in Caliche.</p>
        <div class="set"><span class="lbl">Radio callers &amp; bulletin</span><button class="opt qual" data-a="radiocast" aria-label="Radio callers and bulletin" aria-pressed="${o.radioCast}">${o.radioCast ? 'On' : 'Off'}</button></div>
        <p class="small reading-note">Synthetic callers and the 1947 news bulletin on AM 1420.</p>
        <div class="set"><span class="lbl">Character voices</span><button class="opt qual" data-a="characters" aria-label="Character voices" aria-pressed="${o.characters}">${o.characters ? 'On' : 'Off'}</button></div>
        <p class="small reading-note">Synthetic parody voices for the attendant, deputy, souvenir man, Nora and the recorded field line.</p>
        <div class="set"><span class="lbl">Rod Serling narrator</span><button class="opt qual" data-a="serling" aria-label="Rod Serling narrator" aria-pressed="${o.serling}">${o.serling ? 'On' : 'Off'}</button></div>
        <p class="small reading-note">Synthetic Rod Serling parody for descriptions and chapter transitions. Hold Zoom for a spoken closer look; V repeats a description.</p>
        <div class="set"><span class="lbl">Kinski read-aloud</span><button class="opt qual" data-a="kinski" aria-label="Kinski read-aloud" aria-pressed="${o.kinski}">${o.kinski ? 'On' : 'Off'}</button></div>
        <p class="small reading-note">Synthetic Kinski parody for papers, optional conversation reading and other phone voices. Also reads descriptions when the Rod Serling narrator is Off.</p>
        <div class="set"><label for="sens">Look speed</label><input id="sens" type="range" min="0.3" max="2.5" step="0.05" value="${o.sens}"></div>
        <div class="set"><label for="fov">Field of view</label><input id="fov" type="range" min="60" max="90" step="1" value="${o.fov}"></div>
        <div class="set"><span class="lbl">Invert look</span><button class="opt qual" data-a="inv" aria-label="Invert vertical look">${o.invertY ? 'On' : 'Off'}</button></div>
        <div class="set"><span class="lbl">Text</span><button class="opt qual" data-a="txt" aria-label="Text size">${o.largeText ? 'Large' : 'Normal'}</button></div>
        <div class="set"><span class="lbl">Graphics</span><button class="opt qual" data-a="qual" aria-label="Graphics quality">${QUALITY[o.quality]}</button></div>
        <div class="set"><span class="lbl">Picture</span><button class="opt qual" data-a="pic" aria-label="Picture: clean or video tape">${PICTURE[o.picture]}</button></div>
        <div class="set"><span class="lbl">Flashing images</span><button class="opt qual" data-a="flash" aria-label="Flashing images near the end: flashes or slow fades">${o.calmFlash ? 'Slow fades' : 'Flashes'}</button></div>
        ${o.onAbduct ? `<div class="set"><span class="lbl">Missing time</span><button class="opt qual" data-a="abduct" aria-label="Missing time out in the dark at night: on or off">${o.abduct ? 'On' : 'Off'}</button></div>` : ''}
        <div class="set"><span class="lbl">Ending camera</span><button class="opt qual" data-a="still" aria-label="Camera at the end: moving or still shots">${o.stillShots ? 'Still shots' : 'Moving'}</button></div>
        ${o.settingsOnly ? '' : '<button class="opt" data-a="title">Quit to title</button>'}
        <p class="small">${o.settingsOnly ? 'Settings are kept on this device.' : 'Nothing in the control room moves on while the game is paused.'}</p>
      </div>`;
    el.querySelector('[data-a=resume]')!.addEventListener('click', () => this.close());
    el.querySelector('[data-a=title]')?.addEventListener('click', () => { this.close(true); o.onTitle(); });
    el.querySelector('[data-a=save]')?.addEventListener('click', () => o.onSave?.());
    el.querySelector('[data-a=load]')?.addEventListener('click', () => o.onLoad?.());
    (el.querySelector('#vol') as HTMLInputElement).addEventListener('input', (e) => o.onVolume(+(e.target as HTMLInputElement).value));
    (el.querySelector('#mus') as HTMLInputElement).addEventListener('input', (e) => o.onMusic(+(e.target as HTMLInputElement).value));
    (el.querySelector('#rad') as HTMLInputElement).addEventListener('input', (e) => o.onRadio(+(e.target as HTMLInputElement).value));
    let artBell = o.artBell;
    el.querySelector<HTMLButtonElement>('[data-a=artbell]')!.addEventListener('click', (e) => {
      artBell = !artBell;
      const button = e.currentTarget as HTMLButtonElement;
      button.textContent = artBell ? 'On' : 'Off'; button.setAttribute('aria-pressed', String(artBell));
      o.onArtBell(artBell);
    });
    let radioCast = o.radioCast;
    el.querySelector<HTMLButtonElement>('[data-a=radiocast]')!.addEventListener('click', (e) => {
      radioCast = !radioCast;
      const button = e.currentTarget as HTMLButtonElement;
      button.textContent = radioCast ? 'On' : 'Off'; button.setAttribute('aria-pressed', String(radioCast));
      o.onRadioCast(radioCast);
    });
    for (const [key, initial, apply] of [['characters', o.characters, o.onCharacters], ['serling', o.serling, o.onSerling]] as const) {
      let on = initial;
      el.querySelector<HTMLButtonElement>(`[data-a=${key}]`)!.addEventListener('click', (event) => {
        on = !on; const button = event.currentTarget as HTMLButtonElement;
        button.textContent = on ? 'On' : 'Off'; button.setAttribute('aria-pressed', String(on)); apply(on);
      });
    }
    let kinski = o.kinski;
    el.querySelector<HTMLButtonElement>('[data-a=kinski]')!.addEventListener('click', (e) => {
      kinski = !kinski;
      const button = e.currentTarget as HTMLButtonElement;
      button.textContent = kinski ? 'On' : 'Off'; button.setAttribute('aria-pressed', String(kinski));
      o.onKinski(kinski);
    });
    (el.querySelector('#sens') as HTMLInputElement).addEventListener('input', (e) => o.onSens(+(e.target as HTMLInputElement).value));
    (el.querySelector('#fov') as HTMLInputElement).addEventListener('input', (e) => o.onFov(+(e.target as HTMLInputElement).value));
    let inv = o.invertY, big = o.largeText, calm = o.calmFlash, still = o.stillShots, ab = !!o.abduct;
    el.querySelector('[data-a=abduct]')?.addEventListener('click', (e) => { ab = !ab; (e.target as HTMLElement).textContent = ab ? 'On' : 'Off'; o.onAbduct?.(ab); });
    el.querySelector('[data-a=flash]')!.addEventListener('click', (e) => { calm = !calm; (e.target as HTMLElement).textContent = calm ? 'Slow fades' : 'Flashes'; o.onCalmFlash(calm); });
    el.querySelector('[data-a=still]')!.addEventListener('click', (e) => { still = !still; (e.target as HTMLElement).textContent = still ? 'Still shots' : 'Moving'; o.onStillShots(still); });
    el.querySelector('[data-a=inv]')!.addEventListener('click', (e) => { inv = !inv; (e.target as HTMLElement).textContent = inv ? 'On' : 'Off'; o.onInvertY(inv); });
    el.querySelector('[data-a=txt]')!.addEventListener('click', (e) => { big = !big; (e.target as HTMLElement).textContent = big ? 'Large' : 'Normal'; o.onLargeText(big); });
    let q = o.quality;
    const qb = el.querySelector('[data-a=qual]') as HTMLButtonElement;
    // Ultra (shadows, occlusion, bloom) is offered on a computer; phones keep High and Low
    const order: ('ultra' | 'high' | 'low')[] = o.ultraOk ? ['ultra', 'high', 'low'] : ['high', 'low'];
    qb.addEventListener('click', () => { q = order[(order.indexOf(q) + 1) % order.length]; qb.textContent = QUALITY[q]; o.onQuality(q); });
    let pic = o.picture;
    const pb = el.querySelector('[data-a=pic]') as HTMLButtonElement;
    pb.addEventListener('click', () => { pic = pic === 'off' ? 'vhs' : pic === 'vhs' ? 'heavy' : 'off'; pb.textContent = PICTURE[pic]; o.onPicture(pic); });
    this.open(el, o.onResume);
  }

  // Between chapters: on the black, a line about what was just closed, then the next
  // chapter's title and the night's clock. The night does not stop for it.
  private cardEl: HTMLElement | null = null;
  private cardGeneration = 0;
  chapterStage: 'idle' | 'a' | 'b' | 'text-b' | 'error-b' | 'cancelled' = 'idle';
  async chapterCard(o: { closed: string; recap: string; next: string; title: string; clock: string } | null) {
    const generation = ++this.cardGeneration;
    this.chapterStage = 'idle';
    if (this.cardEl) { const old = this.cardEl; old.classList.add('out'); setTimeout(() => old.remove(), 900); this.cardEl = null; }
    this.narrator?.stop();
    if (!o) return;
    this.lastDescription = o.recap;
    this.updateDescriptionButton();
    const el = document.createElement('div');
    el.className = 'chapter-card';
    el.innerHTML = `
      <div class="cc-a"><div class="cc-closed">${esc(o.closed)}</div><p class="cc-recap">${esc(o.recap)}</p></div>
      <div class="cc-b"><div class="cc-next">${esc(o.next)}</div><h2>${esc(o.title)}</h2><div class="cc-clock">${esc(o.clock)}</div></div>`;
    document.body.appendChild(el);
    this.cardEl = el;
    const reader = this.narrator;
    if (!reader?.enabled) return;
    el.classList.add('voiced'); el.dataset.voicePhase = 'a'; this.chapterStage = 'a';
    const current = () => generation === this.cardGeneration && this.cardEl === el;
    this.onReadingStart?.(reader);
    const narration = reader.play([o.closed, o.recap, o.next, o.title, o.clock], undefined, (index) => {
      if (index === 2 && current() && reader.enabled) { el.dataset.voicePhase = 'b'; this.chapterStage = 'b'; }
    });
    const run = reader.runId;
    await narration;
    if (!current()) return;
    if (!reader.enabled) {
      el.dataset.voicePhase = 'b'; this.chapterStage = 'text-b'; return;
    }
    if (reader.runId !== run) { this.chapterStage = 'cancelled'; return; }
    if (reader.state === 'error') {
      // A failed load still gives the next chapter readable screen time. No
      // replacement voice or later phase of speech starts after the failure.
      el.dataset.voicePhase = 'b'; this.chapterStage = 'error-b';
    }
  }

  /** The card cut away at once, and the black with it (main.ts: CHAPTER TWO ends on the phone's
   *  ring, FORSLAG.md, 20 October, small change 6), instead of the slow fade. */
  chapterCut() {
    const el = this.cardEl;
    if (el) el.classList.add('cut');
    void this.chapterCard(null);
    this.fadeEl.classList.add('quick');
    this.fade(false);
    setTimeout(() => this.fadeEl.classList.remove('quick'), 700);
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
    el.querySelectorAll<HTMLButtonElement>('[data-b]').forEach((btn) => btn.addEventListener('click', () => { this.stopSpeech(); o.buttons[+btn.dataset.b!].on(); }));
    this.root.appendChild(el);
    this.readingControls(el.querySelector('.after')!, o.lines, 'Read description', 'ending-description');
    return el;
  }
}
