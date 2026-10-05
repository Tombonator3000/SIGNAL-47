// The end of the night (KAPITLER.md, Roswell Road og THE EVENT): the frames of FLASH over
// the game, the black, SIGNAL / 47, the credits, and the whole 1947 photograph after them.
// The 3D game has stopped by the time the title comes up, so from there on everything runs
// on real time. Nothing here is saved.

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));

export const CREDITS: [string, string][] = [
  ['', 'A night in New Mexico, 14 April 1986'],
  ['Story and direction', 'Tom'],
  ['Code, chapters and sound', 'Claude (Anthropic)'],
  ['Mesa Diner, Sierra Motor Court, room 6, art and textures', 'Codex and ChatGPT (OpenAI)'],
  ['Music', '"Signal to Noise" by Scott Buckley, CC BY 4.0'],
  ['Sound effects, CC0', 'Freesound: viertelnachvier, transitking, geraldfiebig, DarkShroom, FreethinkerAnon, Gamba_Studio, rvandemark, qubodup, sqeeeek, Yoyodaman234, matth3wc04, BlondPanda, GiocoSound, Notarget, cabled_mess, CastIronCarousel, JarredGibb, Lunardrive, wjtaylor, yatoimtop, LilMati. Kenney.'],
  ['Fonts', 'VT323, Reenie Beanie and Oswald (SIL Open Font License 1.1). Special Elite (Apache License 2.0).'],
  ['Built with', 'three.js (MIT)'],
  ['', 'Everything in this story is invented. The comet was real.'],
];

export class Ending {
  private root = document.createElement('div');
  private frame = document.createElement('div');
  private cover = document.createElement('div');
  private timers: number[] = [];
  private gone = false;

  constructor() {
    this.root.className = 'ending';
    this.frame.className = 'ending-frame';
    this.cover.className = 'ending-cover';
    this.root.append(this.frame, this.cover);
    document.body.appendChild(this.root);
  }

  /** One frame of FLASH over the game (null: none). opacity below 1 for the slow fades. */
  flash(c: HTMLCanvasElement | null, opacity = 1) {
    if (!c) { this.frame.replaceChildren(); this.frame.style.opacity = '0'; return; }
    if (this.frame.firstChild !== c) { c.className = 'ending-img'; this.frame.replaceChildren(c); }
    this.frame.style.opacity = String(opacity);
  }
  /** Black over everything, faded in or out over ms. */
  black(on: boolean, ms = 600) {
    this.cover.style.transition = `opacity ${ms}ms ease`;
    this.cover.style.opacity = on ? '1' : '0';
  }

  private wait(ms: number) { return new Promise<void>((ok) => { this.timers.push(window.setTimeout(() => ok(), ms)); }); }
  private card(html: string, cls: string) {
    const el = document.createElement('div');
    el.className = 'ending-card ' + cls;
    el.innerHTML = html;
    this.root.appendChild(el);
    return el;
  }

  /** SIGNAL / 47 on the black. */
  async title() {
    const el = this.card('<h1>SIGNAL<span class="slash">/</span>47</h1>', 'ending-title');
    await this.wait(60); el.classList.add('on');
    await this.wait(5600); el.classList.remove('on');
    await this.wait(1400); el.remove();
  }

  /** The credits roll; a button skips them after a few seconds. */
  async credits() {
    const rows = CREDITS.map(([k, v]) => `<div class="cr">${k ? `<div class="cr-k">${esc(k)}</div>` : ''}<div class="cr-v">${esc(v)}</div></div>`).join('');
    const el = this.card(`<div class="roll"><h2>SIGNAL<span class="slash">/</span>47</h2>${rows}</div><button class="skip" hidden>Skip</button>`, 'ending-credits');
    const roll = el.querySelector('.roll') as HTMLElement, skip = el.querySelector('.skip') as HTMLButtonElement;
    const secs = 46;
    await this.wait(60);
    el.classList.add('on');
    roll.style.transition = `transform ${secs}s linear`;
    roll.style.transform = 'translateY(calc(-100% - 60vh))';
    let done: () => void = () => {};
    const over = new Promise<void>((ok) => { done = ok; });
    this.timers.push(window.setTimeout(() => { skip.hidden = false; }, 4000));
    skip.addEventListener('click', () => done());
    this.timers.push(window.setTimeout(() => done(), secs * 1000));
    await over;
    el.classList.remove('on');
    await this.wait(1200); el.remove();
  }

  /** After the credits: the whole photograph, no text. paper() plays the sound of paper. */
  async photo(src: HTMLCanvasElement | null, paper: () => void) {
    await this.wait(1500);
    paper();
    const el = this.card('', 'ending-photo');
    if (src) { src.className = 'ending-img'; el.appendChild(src); }
    await this.wait(60); el.classList.add('on');
    await this.wait(6000 + 8000); el.classList.remove('on');
    await this.wait(2200); el.remove();
    await this.wait(1800);
  }

  get disposed() { return this.gone; }
  dispose() {
    this.gone = true;
    for (const t of this.timers) clearTimeout(t);
    this.timers = [];
    this.root.remove();
  }
}
