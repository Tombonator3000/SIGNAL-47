// The music under the night (Tom, 5 October: more music). Sparse on purpose: a cue now and
// then, chosen by where the player is, faded in under the sounds of the place and out again,
// then several minutes of nothing. Never under a scene that holds the player, never on the
// old road (its radio and its silence are the music there), never twice in a quarter of an hour.
//
// The cues are excerpts of Scott Buckley's music (CC BY 4.0, scottbuckley.com.au), cut and
// levelled for the game (THIRD_PARTY_NOTICES.md). They are files in public/music/, streamed
// when they are needed, so the game does not wait for them and a phone only fetches what
// it hears. The single-file build has no files beside it, so it plays no cues.

export type Cue = 'night' | 'eerie' | 'tension' | 'motel' | 'drive' | 'dawn';
export const CUES: Record<Cue, { file: string; title: string; gain: number }> = {
  night: { file: 'shadows_and_dust.mp3', title: 'Shadows and Dust', gain: 0.5 },
  eerie: { file: 'decoherence.mp3', title: 'Decoherence', gain: 0.45 },
  tension: { file: 'the_old_ones.mp3', title: 'The Old Ones', gain: 0.45 },
  motel: { file: 'in_search_of_solitude.mp3', title: 'In Search of Solitude', gain: 0.42 },
  drive: { file: 'neon.mp3', title: 'Neon', gain: 0.36 },
  dawn: { file: 'hymn_to_the_dawn.mp3', title: 'Hymn to the Dawn', gain: 0.45 },
};
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const REPEAT = 15 * 60;   // a cue waits this long (in played seconds) before it is heard again

export const scoreOn = import.meta.env.MODE !== 'single';

export class Score {
  /** The cue playing now (or fading out), for the developer HUD and the tests. */
  playing: Cue | null = null;
  private el: HTMLAudioElement | null = null;
  private gain: GainNode | null = null;
  private rest = rnd(35, 70);          // seconds of quiet before the next cue may start
  private t = 0;
  private last = new Map<Cue, number>();
  private fading = false;

  constructor(private ctx: AudioContext, private dest: AudioNode) {}

  /** Every frame while a night runs. want: the cue that fits where the player is (or none);
   *  held: a scene holds the player, or a voice or the radio must be heard. */
  update(dt: number, want: Cue | null, held: boolean) {
    this.t += dt;
    if (this.playing) {
      if (held) this.fade(2.5, 25);
      else if (want !== this.playing && !this.fading) this.fade(5, rnd(10, 25));   // somewhere else now
      return;
    }
    if (held || !want) { this.rest = Math.max(this.rest, 8); return; }
    this.rest -= dt;
    if (this.rest > 0) return;
    const at = this.last.get(want);
    if (at !== undefined && this.t - at < REPEAT) { this.rest = 30; return; }
    this.start(want);
  }

  start(cue: Cue) {
    if (!scoreOn) return;
    this.stop();
    const c = CUES[cue];
    const el = new Audio();
    el.preload = 'auto';
    el.src = new URL(`music/${c.file}`, document.baseURI).href;
    const g = this.ctx.createGain(); g.gain.value = 0;
    try { this.ctx.createMediaElementSource(el).connect(g); } catch { return; }
    g.connect(this.dest);
    const now = this.ctx.currentTime;
    g.gain.setValueAtTime(0, now); g.gain.linearRampToValueAtTime(c.gain, now + 6);
    el.addEventListener('ended', () => { if (this.el === el) this.done(rnd(150, 330)); });
    el.addEventListener('error', () => { if (this.el === el) this.done(600); });   // not there: try much later
    el.play().catch(() => { if (this.el === el) this.done(120); });
    this.el = el; this.gain = g; this.playing = cue; this.fading = false;
    this.last.set(cue, this.t);
  }

  /** Fade the cue out over `secs`, then `quiet` seconds before another may start. */
  fade(secs: number, quiet: number) {
    if (!this.el || !this.gain || this.fading) return;
    this.fading = true;
    const el = this.el, g = this.gain, now = this.ctx.currentTime;
    g.gain.cancelScheduledValues(now); g.gain.setValueAtTime(g.gain.value, now); g.gain.linearRampToValueAtTime(0, now + secs);
    setTimeout(() => { if (this.el === el) this.done(quiet); }, secs * 1000 + 100);
  }

  /** At once: leaving the night, the title, the end. */
  stop() {
    if (this.el) { this.el.pause(); this.el.removeAttribute('src'); this.el.load(); }
    this.gain?.disconnect();
    this.el = null; this.gain = null; this.playing = null; this.fading = false;
  }

  /** The pause menu: the cue stops where it is and goes on after. */
  pause(on: boolean) { if (this.el) { if (on) this.el.pause(); else void this.el.play().catch(() => {}); } }

  private done(quiet: number) {
    this.stop();
    this.rest = quiet;
  }
}
