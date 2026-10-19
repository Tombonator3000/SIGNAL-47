import type { AudioSys } from './Audio';
import catalogSource from '../assets/audio/kinski/catalog.json?raw';

export type ReadingState = 'loading' | 'playing' | 'idle' | 'error';
type Clip = { id: string; text: string; duration: number; file: string };
type Catalog = { version: number; voice: string; clips: Clip[] };

// Imported URLs are relative in the Pages build and data URLs in the single HTML.
// Only the recording being read is fetched and decoded, never the whole library.
const FILES = import.meta.glob<string>('../assets/audio/kinski/*.mp3', { eager: true, query: '?url', import: 'default' });
const catalog = JSON.parse(catalogSource) as Catalog;
const clips = new Map(catalog.clips.map((clip) => [normalizeReadingText(clip.text), clip]));
const MAX_BUFFERS = 4, MAX_BYTES = 32 * 1024 * 1024;
const MAX_RESOLVED = 128;
function initialCaseAlternative(text: string) {
  const first = text[0] ?? '';
  return (first === first.toUpperCase() ? first.toLowerCase() : first.toUpperCase()) + text.slice(1);
}
const indexed = new Map<string, { text: string; clip: Clip }[]>();
for (const [text, clip] of clips) {
  if (!text || !FILES[`../assets/audio/kinski/${clip.file}`]) continue;
  // A clock-prefixed observation can also be stored without its clock. The
  // leading sentence punctuation in its recorded fragment carries no words.
  // Keep signs and hyphens: a recorded minus must never disappear this way.
  const bases = new Set([text, text.replace(/^[\s.,;:!?]+/, '')].filter(Boolean));
  // Album captions capitalize the first letter of their stored description.
  // Preserve every other letter: US/us and AM/am are not interchangeable.
  for (const key of new Set([...bases].flatMap((base) => {
    const alternative = initialCaseAlternative(base);
    return [base, ...(alternative !== base && !clips.has(alternative) ? [alternative] : [])];
  }))) {
    const list = indexed.get(key[0]) ?? [];
    list.push({ text: key, clip }); indexed.set(key[0], list);
  }
}
for (const list of indexed.values()) list.sort((a, b) => b.text.length - a.text.length);

const SMALL = ['zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten', 'eleven', 'twelve', 'thirteen', 'fourteen', 'fifteen', 'sixteen', 'seventeen', 'eighteen', 'nineteen'];
const TENS = ['', '', 'twenty', 'thirty', 'forty', 'fifty', 'sixty', 'seventy', 'eighty', 'ninety'];
const wordCharacter = (s: string) => /[\p{L}\p{N}_]/u.test(s);
const insideWord = (text: string, at: number) => at > 0 && at < text.length && (
  (wordCharacter(text[at - 1]) && wordCharacter(text[at])) ||
  (/[’']/.test(text[at - 1]) && at > 1 && wordCharacter(text[at - 2]) && wordCharacter(text[at])) ||
  (/[’']/.test(text[at]) && at + 1 < text.length && wordCharacter(text[at - 1]) && wordCharacter(text[at + 1]))
);
// Punctuation has no spoken content. Signs and hyphens are deliberately absent:
// a missing minus in -39 LY must never turn into an apparently complete reading.
const silentCharacter = (s: string) => /[\s.,;:!?"'“”‘’()[\]{}\/\\|…•]/u.test(s);

function cardinal(value: number): string[] {
  if (value < 20) return [SMALL[value]];
  if (value < 100) return [TENS[Math.floor(value / 10)], ...(value % 10 ? cardinal(value % 10) : [])];
  for (const [scale, name] of [[1000000, 'million'], [1000, 'thousand'], [100, 'hundred']] as const)
    if (value >= scale) return [...cardinal(Math.floor(value / scale)), name, ...(value % scale ? cardinal(value % scale) : [])];
  return [];
}

function integerWords(value: string): string[] {
  // Leading zeroes in angles and reference numbers are meaningful, as are the
  // individual digits of unusually large identifiers. Do not round either.
  if ((value.length > 1 && value[0] === '0') || value.length > 9) return [...value].map((digit) => SMALL[Number(digit)]);
  return cardinal(Number(value));
}

function numberWords(token: string): string[] {
  if (token.includes(':')) return token.split(':').flatMap((part, i) => {
    const value = Number(part);
    // A clock reads 03:05 as "three zero five"; seconds use the same rule.
    return i && value < 10 ? ['zero', ...cardinal(value)] : cardinal(value);
  });
  const sign = token[0] === '-' ? ['minus'] : token[0] === '+' ? ['plus'] : [];
  const unsigned = token.replace(/^[+-]/, '').replace(/%$/, '').replace(/,/g, '');
  const [whole, fraction] = unsigned.split('.');
  return [...sign, ...integerWords(whole), ...(fraction === undefined ? [] : ['point', ...[...fraction].map((digit) => SMALL[Number(digit)])]), ...(token.endsWith('%') ? ['percent'] : [])];
}

function numericAt(text: string, at: number): { token: string; words: string[] } | null {
  if (at && wordCharacter(text[at - 1])) return null;
  const rest = text.slice(at);
  const clock = /^\d{1,2}:\d{2}(?::\d{2})?(?![\p{L}\p{N}_])/u.exec(rest);
  const number = clock ?? /^[+-]?(?:\d{1,3}(?:,\d{3})+|\d+)(?:\.\d+)?%?(?![\p{L}\p{N}_])/u.exec(rest);
  if (!number) return null;
  return { token: number[0], words: numberWords(number[0]) };
}

function identifierAt(text: string, at: number): { token: string; words: string[] } | null {
  if (at && wordCharacter(text[at - 1])) return null;
  const rest = text.slice(at);
  const code = /^([A-Z]{1,3}|[a-z])-?(\d{1,3})([A-Za-z])?(?![\p{L}\p{N}_])/u.exec(rest);
  if (code) return { token: code[0], words: [...code[1].toUpperCase(), ...integerWords(code[2]), ...(code[3] ? [code[3].toUpperCase()] : [])] };
  const frame = /^(?:frame|FRAME)(\d{1,3})(?![\p{L}\p{N}_])/u.exec(rest);
  return frame ? { token: frame[0], words: ['frame', ...integerWords(frame[1])] } : null;
}

export function normalizeReadingText(text: string) { return text.replace(/\s+/g, ' ').trim(); }

/** UI.document draws these source markers as pencil and paper separators. */
export function documentReadingParagraphs(text: string) {
  return text.replace(/^~[ \t]+/gm, '').replace(/^[ \t]*(?:[-=][ \t]*){3,}$/gm, '')
    .split(/\n\s*\n/).map(normalizeReadingText).filter(Boolean);
}

/** Optional prerecorded narration. No running voice service is needed by the player. */
export class ReadAloud {
  private on = false;
  private currentState: ReadingState = 'idle';
  private notify: ((state: ReadingState) => void) | undefined;
  private generation = 0;
  private pending: AbortController | null = null;
  private source: AudioBufferSourceNode | null = null;
  private finishSource: (() => void) | null = null;
  private buffers = new Map<string, AudioBuffer>();
  private bufferBytes = 0;
  private resolved = new Map<string, Clip[] | null>();

  constructor(private audio: AudioSys) {}

  get enabled() { return this.on; }
  set enabled(value: boolean) { this.on = value; if (!value) this.stop(); }
  get state() { return this.currentState; }
  get playing() { return this.currentState === 'playing'; }
  get loading() { return this.currentState === 'loading'; }

  available(texts: string[]) { return this.resolve(texts) !== null; }
  /** The actual recorded fragments, useful when checking a composed description. */
  segments(texts: string[]): string[] | null { return this.resolve(texts)?.map((clip) => clip.text) ?? null; }

  private resolve(texts: string[]): Clip[] | null {
    if (!texts.length) return null;
    const result: Clip[] = [];
    for (const text of texts) {
      const sequence = this.resolveText(normalizeReadingText(text));
      // A changed or missing word must not silently vanish from a description.
      if (!sequence) return null;
      result.push(...sequence);
    }
    return result;
  }

  private resolveText(text: string): Clip[] | null {
    if (!text) return null;
    if (this.resolved.has(text)) return this.resolved.get(text)!;
    const whole = clips.get(text) ?? clips.get(initialCaseAlternative(text));
    let result: Clip[] | null = null;
    if (whole && FILES[`../assets/audio/kinski/${whole.file}`]) result = [whole];
    else {
      // Work backwards so a long matching prefix cannot hide a shorter valid
      // route. Links avoid recursion and keep long notebooks cheap to resolve.
      const paths: ({ clips: Clip[]; next: number } | undefined)[] = new Array(text.length + 1);
      paths[text.length] = { clips: [], next: text.length };
      for (let at = text.length - 1; at >= 0; --at) {
        for (const item of indexed.get(text[at]) ?? []) {
          const end = at + item.text.length;
          if (!paths[end] || !text.startsWith(item.text, at)) continue;
          if (wordCharacter(item.text[0]) && insideWord(text, at)) continue;
          if (wordCharacter(item.text[item.text.length - 1]) && insideWord(text, end)) continue;
          paths[at] = { clips: [item.clip], next: end }; break;
        }
        if (paths[at]) continue;
        const token = /[+\-\d]/.test(text[at]) ? numericAt(text, at)
          : /[A-Za-z]/.test(text[at]) ? identifierAt(text, at) : null;
        if (token) {
          if (paths[at + token.token.length]) {
            const spoken = token.words.map((word) => clips.get(word) ?? clips.get(initialCaseAlternative(word)));
            if (spoken.every((clip) => clip && FILES[`../assets/audio/kinski/${clip.file}`]))
              paths[at] = { clips: spoken as Clip[], next: at + token.token.length };
          }
        }
        if (!paths[at] && silentCharacter(text[at]) && paths[at + 1]) paths[at] = { clips: [], next: at + 1 };
      }
      if (paths[0]) {
        result = [];
        for (let at = 0; at < text.length;) { const path = paths[at]!; result.push(...path.clips); at = path.next; }
        if (!result.length) result = null;
      }
    }
    if (this.resolved.size >= MAX_RESOLVED) this.resolved.delete(this.resolved.keys().next().value!);
    this.resolved.set(text, result);
    return result;
  }

  private stateTo(state: ReadingState) { this.currentState = state; this.notify?.(state); }

  stop() {
    ++this.generation;
    this.pending?.abort(); this.pending = null;
    if (this.source) {
      this.source.onended = null;
      try { this.source.stop(); } catch { /* already ended */ }
      this.source.disconnect(); this.source = null;
    }
    // Resolve the old play() even when its onended handler has been removed.
    this.finishSource?.(); this.finishSource = null;
    this.stateTo('idle'); this.notify = undefined;
  }

  async play(texts: string[], onState?: (state: ReadingState) => void): Promise<void> {
    this.stop();
    this.notify = onState;
    if (!this.enabled) { this.stateTo('idle'); this.notify = undefined; return; }
    const queue = this.resolve(texts);
    if (!queue) { this.stateTo('error'); this.notify = undefined; return; }
    const generation = this.generation;
    const pending = new AbortController(); this.pending = pending;
    const active = () => generation === this.generation && !pending.signal.aborted;
    this.stateTo('loading');
    try {
      // Call unlock synchronously from the Read button's gesture, before an await:
      // iOS must receive resume()/the initial silent blip inside that gesture.
      const unlocked = this.audio.unlock();
      await unlocked;
      if (!active()) return;
      const ctx = this.audio.ctx;
      if (!ctx || ctx.state !== 'running') throw new Error('Audio could not start');
      for (const clip of queue) {
        if (!active()) return;
        this.stateTo('loading');
        const buffer = await this.load(clip, ctx, pending.signal, active);
        if (!active() || !buffer) return;
        await new Promise<void>((resolve) => {
          const source = ctx.createBufferSource(); source.buffer = buffer;
          source.connect(this.audio.master);
          this.source = source; this.finishSource = resolve;
          source.onended = () => {
            source.disconnect();
            if (this.source === source) { this.source = null; this.finishSource = null; }
            resolve();
          };
          source.start(); this.stateTo('playing');
        });
      }
      if (active()) { this.pending = null; this.stateTo('idle'); this.notify = undefined; }
    } catch (error) {
      if (!active()) return;
      this.pending = null;
      if (this.source) {
        this.source.onended = null;
        try { this.source.stop(); } catch { /* already ended */ }
        this.source.disconnect(); this.source = null;
      }
      this.finishSource?.(); this.finishSource = null;
      this.stateTo('error'); this.notify = undefined;
      console.warn('Read-aloud could not play', error);
    }
  }

  private async load(clip: Clip, ctx: AudioContext, signal: AbortSignal, active: () => boolean): Promise<AudioBuffer | null> {
    const cached = this.buffers.get(clip.id);
    if (cached) {
      this.buffers.delete(clip.id); this.buffers.set(clip.id, cached);
      return cached;
    }
    const url = FILES[`../assets/audio/kinski/${clip.file}`];
    let encoded: ArrayBuffer;
    if (url.startsWith('data:')) {
      // The self-contained build can run from file: and under the existing CSP
      // without connect-src. Decode its embedded bytes without a fetch(data:).
      const binary = atob(url.slice(url.indexOf(',') + 1));
      const bytes = new Uint8Array(binary.length);
      for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
      encoded = bytes.buffer;
    } else {
      const response = await fetch(url, { signal });
      if (!response.ok) throw new Error(`Recording unavailable (${response.status})`);
      encoded = await response.arrayBuffer();
    }
    if (!active()) return null;
    const buffer = await ctx.decodeAudioData(encoded);
    // decodeAudioData cannot be aborted. A closed/replaced panel invalidates its
    // result before it can enter the cache, start a sound or update the old UI.
    if (!active()) return null;
    const bytes = buffer.length * buffer.numberOfChannels * Float32Array.BYTES_PER_ELEMENT;
    if (bytes <= MAX_BYTES) {
      while (this.buffers.size && (this.buffers.size >= MAX_BUFFERS || this.bufferBytes + bytes > MAX_BYTES)) {
        const oldest = this.buffers.keys().next().value!;
        const old = this.buffers.get(oldest)!;
        this.bufferBytes -= old.length * old.numberOfChannels * Float32Array.BYTES_PER_ELEMENT;
        this.buffers.delete(oldest);
      }
      this.buffers.set(clip.id, buffer); this.bufferBytes += bytes;
    }
    return buffer;
  }
}
