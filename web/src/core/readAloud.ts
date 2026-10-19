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

  constructor(private audio: AudioSys) {}

  get enabled() { return this.on; }
  set enabled(value: boolean) { this.on = value; if (!value) this.stop(); }
  get state() { return this.currentState; }
  get playing() { return this.currentState === 'playing'; }
  get loading() { return this.currentState === 'loading'; }

  available(texts: string[]) { return this.resolve(texts) !== null; }

  private resolve(texts: string[]): Clip[] | null {
    if (!texts.length) return null;
    const result: Clip[] = [];
    for (const text of texts) {
      const clip = clips.get(normalizeReadingText(text));
      // A changed or missing line must not silently vanish from a document.
      if (!clip || !FILES[`../assets/audio/kinski/${clip.file}`]) return null;
      result.push(clip);
    }
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
