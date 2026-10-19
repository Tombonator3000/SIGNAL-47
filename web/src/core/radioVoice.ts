import catalogSource from '../assets/audio/artbell/catalog.json?raw';
import callerCatalogSource from '../assets/audio/radiocast/catalog.json?raw';

export type CallerVoice = 'woman' | 'man' | 'old' | 'boy' | 'news';
export interface RadioClip { id: string; text: string; duration: number; file: string; voice?: CallerVoice }
interface TimePart { value: number; id: string; text: string }
interface Template { prefix: string; suffix: string; capitalize: boolean; before?: string; after?: string }
interface Catalog {
  clips: RadioClip[];
  templates?: Template[];
  timeBank?: { hours: TimePart[]; minutes: TimePart[] };
}

// Pages emits relative URLs; the single-file build inlines these as data URLs.
// Listing URLs does not download or decode the bank. Only the current talk loads.
const FILES = import.meta.glob<string>('../assets/audio/artbell/*.mp3', { eager: true, query: '?url', import: 'default' });
const CALLER_FILES = import.meta.glob<string>('../assets/audio/radiocast/*.mp3', { eager: true, query: '?url', import: 'default' });
const catalog = JSON.parse(catalogSource) as Catalog;
const callerCatalog = JSON.parse(callerCatalogSource) as Catalog;
const clean = (text: string) => text.replace(/\s+/g, ' ').trim();
const byId = new Map(catalog.clips.map((clip) => [clip.id, clip]));
const byText = new Map(catalog.clips.map((clip) => [clean(clip.text), clip]));
const callerByText = new Map(callerCatalog.clips.map((clip) => [`${clip.voice}\0${clean(clip.text)}`, clip]));
const MAX_BUFFERS = 24, MAX_BYTES = 24 * 1024 * 1024;

/** Bounded lazy banks; caller identities include their role as well as the exact text. */
export class RadioVoice {
  private buffers = new Map<string, AudioBuffer>();
  private bytes = 0;

  constructor(private kind: 'host' | 'caller' = 'host') {}

  get bufferCount() { return this.buffers.size; }

  /** Whole lines or an authored time template. Unknown words never silently disappear. */
  resolve(text: string, clock: number, voice?: CallerVoice): RadioClip[] | null {
    if (this.kind === 'caller') {
      const clip = voice && callerByText.get(`${voice}\0${clean(text)}`);
      return clip && this.valid([clip]) ? [clip] : null;
    }
    const whole = byText.get(clean(text));
    if (whole) return this.valid([whole]) ? [whole] : null;
    const day = ((clock % 86400) + 86400) % 86400;
    const h = Math.floor(day / 3600) % 12 || 12, m = Math.floor(day / 60) % 60;
    const hour = catalog.timeBank?.hours.find((part) => part.value === h);
    const minute = catalog.timeBank?.minutes.find((part) => part.value === m);
    if (!hour || !minute) return null;
    // Match sayTime exactly, including its special hyphen before "oh".
    const time = hour.text + (m > 0 && m < 10 ? '-' : ' ') + minute.text;
    for (const template of catalog.templates ?? []) {
      const spoken = template.capitalize ? time[0].toUpperCase() + time.slice(1) : time;
      if (clean(template.prefix + spoken + template.suffix) !== clean(text)) continue;
      const ids = [template.before, hour.id, minute.id, template.after].filter((id): id is string => !!id);
      const clips = ids.map((id) => byId.get(id));
      return clips.every((clip): clip is RadioClip => !!clip) && this.valid(clips) ? clips : null;
    }
    return null;
  }

  private valid(clips: RadioClip[]) {
    return clips.every((clip) => clip.duration > 0 && Number.isFinite(clip.duration) && !!this.url(clip));
  }

  private url(clip: RadioClip) {
    return this.kind === 'caller' ? CALLER_FILES[`../assets/audio/radiocast/${clip.file}`] : FILES[`../assets/audio/artbell/${clip.file}`];
  }

  /** Decode only the selected segment; cancellation never starts or resumes audio. */
  async load(clips: RadioClip[], ctx: AudioContext, signal: AbortSignal) {
    const result = new Map<string, AudioBuffer>();
    for (const clip of clips) {
      if (signal.aborted) throw new DOMException('Radio voice cancelled', 'AbortError');
      let buffer = result.get(clip.id) ?? this.buffers.get(clip.id);
      if (!buffer) {
        const url = this.url(clip);
        if (!url) throw new Error(`Missing radio recording: ${clip.id}`);
        let encoded: ArrayBuffer;
        if (url.startsWith('data:')) {
          const binary = atob(url.slice(url.indexOf(',') + 1));
          encoded = Uint8Array.from(binary, (character) => character.charCodeAt(0)).buffer;
        } else {
          const response = await fetch(url, { signal });
          if (!response.ok) throw new Error(`Radio recording HTTP ${response.status}`);
          encoded = await response.arrayBuffer();
        }
        if (signal.aborted) throw new DOMException('Radio voice cancelled', 'AbortError');
        buffer = await ctx.decodeAudioData(encoded);
        if (signal.aborted) throw new DOMException('Radio voice cancelled', 'AbortError');
        if (!(buffer.duration > 0) || !Number.isFinite(buffer.duration)) throw new Error('Empty radio recording');
        this.buffers.set(clip.id, buffer);
        this.bytes += buffer.length * buffer.numberOfChannels * 4;
        this.trim();
      } else if (this.buffers.has(clip.id)) {
        this.buffers.delete(clip.id); this.buffers.set(clip.id, buffer);
      }
      result.set(clip.id, buffer);
    }
    return result;
  }

  private trim() {
    // Active plans keep their own references when the LRU drops a buffer. Trim at
    // each insertion so a succession of aborted segment loads cannot grow the cache.
    while (this.buffers.size > MAX_BUFFERS || this.bytes > MAX_BYTES) {
      const id = this.buffers.keys().next().value;
      if (id === undefined) break;
      const buffer = this.buffers.get(id)!;
      this.bytes -= buffer.length * buffer.numberOfChannels * 4;
      this.buffers.delete(id);
    }
  }
}
