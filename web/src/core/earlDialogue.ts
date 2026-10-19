import type { AudioSys } from './Audio';
import { RadioVoice, type RadioClip } from './radioVoice';

export type EarlState = 'idle' | 'loading' | 'playing' | 'error';
export interface EarlLine { who: string; text: string }

/** Earl at the studio door. Only his authored dialogue uses this voice. */
export class EarlDialogue {
  readonly bank = new RadioVoice('earl');
  private on = false;
  private currentState: EarlState = 'idle';
  private generation = 0;
  private pending: AbortController | null = null;
  private source: AudioBufferSourceNode | null = null;
  private finishSource: (() => void) | null = null;
  private notify: ((state: EarlState) => void) | undefined;
  private clips: { id: string; text: string; duration: number }[] = [];
  private text: string | null = null;

  constructor(private audio: AudioSys) {}

  get enabled() { return this.on; }
  set enabled(value: boolean) { this.on = value; if (!value) this.stop(); }
  get state() { return this.currentState; }
  get playing() { return this.state === 'playing'; }
  get loading() { return this.state === 'loading'; }
  get status() {
    return { state: this.state, enabled: this.enabled, text: this.text,
      clips: this.clips.map((clip) => ({ ...clip })), bufferCount: this.bank.bufferCount };
  }

  private resolve(lines: EarlLine[]): RadioClip[] | null {
    const speech = lines.filter((line) => line.who === 'EARL');
    if (!speech.length) return null;
    const clips: RadioClip[] = [];
    for (const line of speech) {
      const found = this.bank.resolve(line.text, 0);
      if (!found) return null;
      clips.push(...found);
    }
    return clips;
  }
  available(lines: EarlLine[]) { return this.resolve(lines) !== null; }
  private stateTo(state: EarlState) { this.currentState = state; this.notify?.(state); }

  stop() {
    ++this.generation;
    this.pending?.abort(); this.pending = null;
    if (this.source) {
      this.source.onended = null;
      try { this.source.stop(); } catch { /* already ended */ }
      this.source.disconnect(); this.source = null;
    }
    this.finishSource?.(); this.finishSource = null;
    this.text = null; this.clips = [];
    this.stateTo('idle'); this.notify = undefined;
  }

  async play(lines: EarlLine[], onState?: (state: EarlState) => void): Promise<void> {
    this.stop();
    this.notify = onState;
    if (!this.enabled) { this.stateTo('idle'); this.notify = undefined; return; }
    const clips = this.resolve(lines);
    if (!clips) { this.stateTo('error'); this.notify = undefined; return; }
    const generation = this.generation, pending = new AbortController(); this.pending = pending;
    const active = () => generation === this.generation && !pending.signal.aborted;
    this.stateTo('loading');
    try {
      // Keep resume/the initial silent blip inside the buzzer or topic gesture.
      const unlocked = this.audio.unlock();
      await unlocked;
      if (!active()) return;
      const ctx = this.audio.ctx;
      if (!ctx || ctx.state !== 'running') throw new Error('Earl audio could not start');
      // Prepare the whole exchange first: an unavailable later sentence must not
      // leave the player with a partial or word-mismatched spoken reply.
      const buffers = await this.bank.load(clips, ctx, pending.signal);
      if (!active()) return;
      if (clips.some((clip) => !buffers.has(clip.id))) throw new Error('Incomplete Earl dialogue');
      this.clips = clips.map((clip) => ({ id: clip.id, text: clip.text, duration: buffers.get(clip.id)!.duration }));
      for (let i = 0; i < clips.length; i++) {
        if (!active()) return;
        await new Promise<void>((resolve) => {
          const source = ctx.createBufferSource(); source.buffer = buffers.get(clips[i].id)!;
          // A person speaking nearby uses the ordinary volume, without the AM
          // receiver filter or the radio-volume slider.
          source.connect(this.audio.master);
          this.source = source; this.finishSource = resolve; this.text = clips[i].text;
          source.onended = () => {
            source.disconnect();
            if (this.source === source) { this.source = null; this.finishSource = null; }
            resolve();
          };
          source.start(ctx.currentTime + (i ? 0.22 : 0));
          this.stateTo('playing');
        });
      }
      if (active()) { this.pending = null; this.text = null; this.stateTo('idle'); this.notify = undefined; }
    } catch {
      if (!active()) return;
      this.pending?.abort(); this.pending = null;
      if (this.source) {
        this.source.onended = null;
        try { this.source.stop(); } catch { /* already ended */ }
        this.source.disconnect(); this.source = null;
      }
      this.finishSource?.(); this.finishSource = null; this.text = null;
      this.stateTo('error'); this.notify = undefined;
    }
  }
}
