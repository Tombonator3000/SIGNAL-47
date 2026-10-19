import type { DocSpec } from './UI';
import type { AlbumView } from '../core/album';

// The notebook's album (the Photos tab, core/album.ts): black album pages with the case prints
// under photo corners (their own paper border, their numbers, opened as papers), and the free
// frames as instant prints with the caption in the player's hand on the wide bottom edge, in the
// order they were taken. A print taken in the last minute and a half is still coming up. A free
// print opens big on a click or a tap, with the previous and the next beside it (arrow keys, a
// swipe); Back, Escape, E or a tap outside puts it back in the album.
//
// The round 15 pictures (ART_BRIEF.md, album/) are used once their files are in the repository:
// the album paper, the photo corner, the instant print's frame and the sheet-film frame (for the
// exposures that were not the player's). Until then the same things are drawn with CSS.

const FILES = import.meta.glob<string>('../assets/art/album/**/*.{jpg,png,webp}', { eager: true, import: 'default' });
const art = (name: string) => FILES[`../assets/art/album/runtime/${name.replace(/\.\w+$/, '.webp')}`] ?? FILES[`../assets/art/album/${name}`] ?? null;
const ART = {
  paper: art('tex_album_page.jpg'), corner: art('photo_corner.png'),
  instant: art('frame_instant_blank.png'), sheet: art('frame_sheetfilm_blank.png'),
};
/** Which of the album's pictures the build has (the tests). */
export const albumArt = () => Object.fromEntries(Object.entries(ART).map(([k, v]) => [k, !!v]));

const esc = (s: string) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]!));
const TILT = [-1.4, 0.9, -0.5, 1.3, -1, 0.4, 1.1, -0.7];
// a print still coming up: pale, flat and cold at first
const developing = (k: number) => k >= 1 ? '' : `filter:brightness(${(1 + 0.9 * (1 - k)).toFixed(2)}) contrast(${(0.25 + 0.75 * k).toFixed(2)}) saturate(${k.toFixed(2)})`;
const corners = '<i class="al-c tl"></i><i class="al-c tr"></i><i class="al-c bl"></i><i class="al-c br"></i>';

/** The tab's page: the case prints and the free frames. `photos` are the photograph papers. */
export function albumPage(photos: DocSpec[], v: AlbumView | undefined) {
  const shots = v?.shots ?? [];
  const style = [ART.paper && `--al-paper:url('${ART.paper}')`, ART.corner && `--al-corner:url('${ART.corner}')`,
    ART.instant && `--al-instant:url('${ART.instant}')`, ART.sheet && `--al-sheet:url('${ART.sheet}')`].filter(Boolean).join(';');
  const cls = ['al-page', ART.paper && 'art-paper', ART.corner && 'art-corner', ART.instant && 'art-instant', ART.sheet && 'art-sheet'].filter(Boolean).join(' ');
  const prints = photos.map((d, i) => `<button data-d="${d.id}" class="${d.id.startsWith('away') ? 'al-sheet' : 'al-mount'}" style="--rot:${TILT[i % TILT.length]}deg"><span class="al-m">${corners}<img alt="" src="${d.image}"></span><span class="al-t">${esc(d.title)}</span></button>`).join('');
  const free = shots.map((s, i) => `<button class="al-print${s.odd ? ' odd' : ''}" data-f="${esc(s.id)}" style="--rot:${TILT[(i + 3) % TILT.length]}deg" aria-label="${esc(s.text || s.meta)}">
      <img alt="" src="${s.url}" style="${developing(s.dev)}"><span class="al-cap"><b>${esc(s.text)}</b><small>${esc(s.meta)}</small></span></button>`).join('');
  return `
    <div class="al-head"><h4>Album</h4>${v ? `<span class="al-roll">${v.left} of ${v.roll} free frames left</span>` : ''}</div>
    <div class="${cls}"${style ? ` style="${style}"` : ''}>
      ${photos.length ? `<h5>The case</h5><div class="thumbs">${prints}</div>` : ''}
      ${shots.length ? `<h5>Tonight</h5><div class="al-prints">${free}</div>` : ''}
      ${photos.length || shots.length ? '' : '<p class="empty">No exposures yet. Once you have the field camera, C raises it anywhere.</p>'}
    </div>`;
}

/** The free prints open big (inside the notebook, so putting one back leaves the album open). */
type AlbumReading = { show: (view: HTMLElement) => void | (() => void); change: () => void };
export function bindAlbum(root: HTMLElement, v: AlbumView | undefined, reading?: AlbumReading) {
  const shots = v?.shots ?? [];
  root.querySelectorAll<HTMLButtonElement>('[data-f]').forEach((b) => b.addEventListener('click', (e) => {
    e.stopPropagation();
    const i = shots.findIndex((s) => s.id === b.dataset.f);
    if (i >= 0) big(root, shots, i, reading);
  }));
}

function big(root: HTMLElement, shots: AlbumView['shots'], at: number, reading?: AlbumReading) {
  const el = document.createElement('div');
  el.className = 'al-view';
  el.setAttribute('role', 'dialog');
  const page = root.querySelector('.al-page') as HTMLElement | null;
  if (page?.getAttribute('style')) el.setAttribute('style', page.getAttribute('style')!);
  if (page?.classList.contains('art-instant')) el.classList.add('art-instant');
  let i = at;
  let readingBound = false;
  let disposeReading: (() => void) | void;
  const show = () => {
    reading?.change();
    const s = shots[i];
    el.innerHTML = `
      <figure class="al-print al-big${s.odd ? ' odd' : ''}"><img alt="" src="${s.url}" style="${developing(s.dev)}"><figcaption class="al-cap"><b>${esc(s.text)}</b><small>${esc(s.meta)}</small></figcaption></figure>
      <button class="al-nav prev" data-v="prev" aria-label="Previous photograph"${i > 0 ? '' : ' disabled'}>&lsaquo;</button>
      <button class="al-nav next" data-v="next" aria-label="Next photograph"${i < shots.length - 1 ? '' : ' disabled'}>&rsaquo;</button>
      <button class="al-back" data-v="back"><kbd>E</kbd>Back to the album</button>`;
    if (!readingBound) { disposeReading = reading?.show(el); readingBound = true; }
  };
  const go = (k: number) => { const j = i + k; if (j >= 0 && j < shots.length) { i = j; show(); } };
  const close = () => { reading?.change(); disposeReading?.(); el.remove(); removeEventListener('keydown', key, true); };
  // before the game's own keys (core/Input.ts): these only page through the album and put the print back
  const key = (e: KeyboardEvent) => {
    if (!el.isConnected) { removeEventListener('keydown', key, true); return; }
    const reading = e.target instanceof Element ? e.target.closest<HTMLButtonElement>('.read-aloud button') : null;
    if (reading && (e.code === 'Enter' || e.code === 'Space')) {
      e.preventDefault(); e.stopPropagation();
      if (!e.repeat) reading.click();
      return;
    }
    if (e.code === 'ArrowLeft' || e.code === 'KeyA') go(-1);
    else if (e.code === 'ArrowRight' || e.code === 'KeyD') go(1);
    else if (e.code === 'Escape' || e.code === 'KeyE' || e.code === 'Enter' || e.code === 'Space') close();
    else return;
    e.preventDefault(); e.stopPropagation();
  };
  // a swipe across the print turns to the next or the previous one (and is not also a tap)
  let x0: number | null = null, swiped = false;
  el.addEventListener('pointerdown', (e) => { x0 = e.clientX; swiped = false; });
  el.addEventListener('pointerup', (e) => {
    if (x0 === null) return;
    const dx = e.clientX - x0; x0 = null;
    if (Math.abs(dx) > 60) { swiped = true; go(dx < 0 ? 1 : -1); }
  });
  el.addEventListener('click', (e) => {
    e.stopPropagation();
    if (swiped) { swiped = false; return; }
    const t = (e.target as HTMLElement).closest('[data-v]') as HTMLElement | null;
    if ((e.target as HTMLElement).closest('.read-aloud')) return;
    if (t?.dataset.v === 'prev') go(-1); else if (t?.dataset.v === 'next') go(1);
    else if (t?.dataset.v === 'back' || !(e.target as HTMLElement).closest('.al-print')) close();
  });
  show();
  addEventListener('keydown', key, true);
  root.appendChild(el);
}
