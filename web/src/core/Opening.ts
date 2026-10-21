import type { ReadAloud, ReadingState } from './readAloud';

/** Original opening narration. Buttons and the heading are never in the speech queue. */
export const OPENING_PARAGRAPHS = [
  'New Mexico, 1986. Twenty-seven antennas listen to a sky that has never answered. On the night shift, silence is a measurement. It can be filed, signed, and forgotten.',
  'Tonight, the instruments will record something the sky has not sent. A telephone will ring in an empty room. A photograph will remember what no one saw.',
  'Your name is Reyes. Your shift ends at dawn. Between now and then, forty-seven seconds will refuse to belong to any clock.',
  'The record calls it interference.',
  'The record is incomplete.',
] as const;

export type OpeningState = 'idle' | 'loading' | 'playing' | 'ready' | 'error';
export interface OpeningChoice { caseId: number; replace: boolean }
export interface OpeningView {
  close(): void;
  state(value: OpeningState): void;
}
interface OpeningHooks {
  show(actions: { begin(): void; back(): void; closed(): void }): OpeningView;
  quiet(): void;
  begin(choice: OpeningChoice): void;
  back(): void;
  dismissed?(): void;
}

/** A title-screen choice, not a running night. Begin always remains a real gesture. */
export class Opening {
  private generation = 0;
  private choice: OpeningChoice | null = null;
  private view: OpeningView | null = null;
  private currentState: OpeningState = 'idle';
  constructor(private reader: ReadAloud, private hooks: OpeningHooks) {}

  get active() { return this.choice !== null; }
  get state() { return this.currentState; }
  get status() { return { active: this.active, state: this.state, choice: this.choice ? { ...this.choice } : null }; }

  open(choice: OpeningChoice) {
    if (this.active) return false;
    const generation = ++this.generation;
    this.choice = { ...choice };
    this.currentState = 'ready';
    this.hooks.quiet();
    this.view = this.hooks.show({ begin: () => this.begin(), back: () => this.cancel(), closed: () => this.dismiss() });
    const active = () => this.active && this.generation === generation;
    if (this.reader.enabled) {
      const update = (state: ReadingState) => {
        if (!active()) return;
        this.currentState = state === 'idle' ? 'ready' : state;
        this.view?.state(this.currentState);
      };
      void this.reader.play([...OPENING_PARAGRAPHS], update).then(() => {
        if (active()) update(this.reader.state);
      });
    } else this.view.state('ready');
    return true;
  }

  /** No await here: startGame must request fullscreen/pointer lock in this click. */
  begin() {
    if (!this.choice) return false;
    const choice = this.choice;
    this.finish(true);
    this.hooks.begin(choice);
    return true;
  }
  cancel() {
    if (!this.active) return false;
    this.finish(true); this.hooks.back(); return true;
  }
  /** Another modal, a direct load or DEV abandons the card without replaying it. */
  dismiss() {
    const active = this.active;
    this.finish(true);
    if (active) this.hooks.dismissed?.();
  }
  private finish(close: boolean) {
    if (!this.active && !this.view) return;
    ++this.generation;
    this.choice = null;
    this.currentState = 'idle';
    const view = this.view; this.view = null;
    this.reader.stop();
    if (close) view?.close();
  }
}
