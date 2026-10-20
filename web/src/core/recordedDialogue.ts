import type { AudioSys } from './Audio';
import type { RadioVoice, RadioClip } from './radioVoice';

export type DialogueState = 'idle' | 'loading' | 'playing' | 'error';
export interface DialogueLine { who: string; text: string }

/** One foreground queue shared by authored character-dialogue wrappers. */
export class RecordedDialogue<Input> {
  private on = false;
  private currentState: DialogueState = 'idle';
  private generation = 0;
  private pending: AbortController | null = null;
  private source: AudioBufferSourceNode | null = null;
  private finishSource: (() => void) | null = null;
  private notify: ((state: DialogueState) => void) | undefined;
  private clips: { id: string; text: string; duration: number; voice?: RadioClip['voice'] }[] = [];
  private text: string | null = null;

  private extraNodes: AudioNode[] = [];
  constructor(private audio: AudioSys, readonly bank: RadioVoice,
    private resolveInput: (input: Input, bank: RadioVoice) => RadioClip[] | null,
    private telephone: (clip: RadioClip) => boolean = () => false) {}

  get enabled() { return this.on; }
  set enabled(value: boolean) { this.on = value; if (!value) this.stop(); }
  get state() { return this.currentState; }
  get playing() { return this.state === 'playing'; }
  get loading() { return this.state === 'loading'; }
  get status() {
    return { state: this.state, enabled: this.enabled, text: this.text,
      clips: this.clips.map((clip) => ({ ...clip })), bufferCount: this.bank.bufferCount };
  }

  available(input: Input) { return this.resolveInput(input, this.bank) !== null; }
  private stateTo(state: DialogueState) { this.currentState = state; this.notify?.(state); }

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
    this.disposeNodes();
    this.stateTo('idle'); this.notify = undefined;
  }

  private disposeNodes() { for (const node of this.extraNodes) node.disconnect(); this.extraNodes = []; }

  async play(input: Input, onState?: (state: DialogueState) => void): Promise<void> {
    this.stop();
    this.notify = onState;
    if (!this.enabled) { this.stateTo('idle'); this.notify = undefined; return; }
    const clips = this.resolveInput(input, this.bank);
    if (!clips) { this.stateTo('error'); this.notify = undefined; return; }
    const generation = this.generation, pending = new AbortController(); this.pending = pending;
    const active = () => generation === this.generation && !pending.signal.aborted;
    this.stateTo('loading');
    try {
      // Keep resume/the initial silent blip inside the interaction gesture.
      const unlocked = this.audio.unlock();
      await unlocked;
      if (!active()) return;
      const ctx = this.audio.ctx;
      if (!ctx || ctx.state !== 'running') throw new Error('Dialogue audio could not start');
      // Prepare the whole exchange first: an unavailable later sentence must not
      // leave the player with a partial or word-mismatched spoken reply.
      const buffers = await this.bank.load(clips, ctx, pending.signal);
      if (!active()) return;
      if (clips.some((clip) => !buffers.has(clip.id))) throw new Error('Incomplete dialogue');
      this.clips = clips.map((clip) => ({ id: clip.id, text: clip.text, duration: buffers.get(clip.id)!.duration, voice: clip.voice }));
      for (let i = 0; i < clips.length; i++) {
        if (!active()) return;
        await new Promise<void>((resolve) => {
          const source = ctx.createBufferSource(); source.buffer = buffers.get(clips[i].id)!;
          // A person speaking nearby uses the ordinary volume, without the AM
          // receiver filter or the radio-volume slider.
          if (this.telephone(clips[i])) {
            const line = this.audio.phoneLine(); source.connect(line.input); this.extraNodes.push(...line.nodes);
          } else source.connect(this.audio.master);
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
      if (active()) { this.pending = null; this.text = null; this.disposeNodes(); this.stateTo('idle'); this.notify = undefined; }
    } catch {
      if (!active()) return;
      this.pending?.abort(); this.pending = null;
      if (this.source) {
        this.source.onended = null;
        try { this.source.stop(); } catch { /* already ended */ }
        this.source.disconnect(); this.source = null;
      }
      this.finishSource?.(); this.finishSource = null; this.text = null; this.disposeNodes();
      this.stateTo('error'); this.notify = undefined;
    }
  }
}
