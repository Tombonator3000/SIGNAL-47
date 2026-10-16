// The music under the night. Tom, 16 October: music in the background all the time, scary
// but meditative, pieces that can go round almost like a loop, in the spirit of Mike Oldfield,
// Angelo Badalamenti and David Lynch (their own music is not free to use; these are open pieces
// in the same spirit). So a quiet bed that goes on: a piece chosen by where the player is, then
// a few seconds of nothing, then the next from the same place, never the same piece twice
// running if there is another. When the player goes somewhere else, the piece fades and the
// place's own begins. It gets out of the way of a scene that holds the player, a voice or a
// radio, and comes back after; never on the old road (its radio and its silence are the music
// there).
//
// The pieces (THIRD_PARTY_NOTICES.md, all credited in the credits): Chris Zabriskie's Cylinders
// (CC BY 4.0), minimal patterns that repeat and shift; Kevin MacLeod (CC BY 4.0), slow dark
// pads and a noir saxophone; Shane Ivers' Signs To Nowhere (CC BY 4.0), noir jazz with a
// vibraphone and a muted trumpet; and Scott Buckley (CC BY 4.0), as before. Levelled for the
// game and streamed from public/music/ when they are needed, so the game does not wait for them
// and a phone only fetches what it hears. The single-file build has no files beside it, so it
// plays none.

/** Where the player is, as far as the music goes. */
export type Cue = 'night' | 'eerie' | 'tension' | 'motel' | 'drive' | 'town' | 'dawn';
export interface Track { file: string; title: string; by: string; gain: number }
export const TRACKS: Record<string, Track> = {
  shadows: { file: 'shadows_and_dust.mp3', title: 'Shadows and Dust', by: 'Scott Buckley', gain: 0.5 },
  decoherence: { file: 'decoherence.mp3', title: 'Decoherence', by: 'Scott Buckley', gain: 0.45 },
  oldOnes: { file: 'the_old_ones.mp3', title: 'The Old Ones', by: 'Scott Buckley', gain: 0.45 },
  solitude: { file: 'in_search_of_solitude.mp3', title: 'In Search of Solitude', by: 'Scott Buckley', gain: 0.42 },
  neon: { file: 'neon.mp3', title: 'Neon', by: 'Scott Buckley', gain: 0.36 },
  hymn: { file: 'hymn_to_the_dawn.mp3', title: 'Hymn to the Dawn', by: 'Scott Buckley', gain: 0.45 },
  cylOne: { file: 'cylinder_one.mp3', title: 'Cylinder One', by: 'Chris Zabriskie', gain: 0.42 },
  cylTwo: { file: 'cylinder_two.mp3', title: 'Cylinder Two', by: 'Chris Zabriskie', gain: 0.42 },
  cylEight: { file: 'cylinder_eight.mp3', title: 'Cylinder Eight', by: 'Chris Zabriskie', gain: 0.4 },
  floating: { file: 'floating_cities.mp3', title: 'Floating Cities', by: 'Kevin MacLeod', gain: 0.45 },
  ossuary1: { file: 'ossuary_1.mp3', title: 'Ossuary 1 - A Beginning', by: 'Kevin MacLeod', gain: 0.45 },
  ossuary5: { file: 'ossuary_5.mp3', title: 'Ossuary 5 - Rest', by: 'Kevin MacLeod', gain: 0.45 },
  longNote: { file: 'long_note_four.mp3', title: 'Long Note Four', by: 'Kevin MacLeod', gain: 0.5 },
  lightless: { file: 'lightless_dawn.mp3', title: 'Lightless Dawn', by: 'Kevin MacLeod', gain: 0.45 },
  phantom: { file: 'phantom_from_space.mp3', title: 'Phantom from Space', by: 'Kevin MacLeod', gain: 0.4 },
  docks: { file: 'night_on_the_docks.mp3', title: 'Night on the Docks (Sax)', by: 'Kevin MacLeod', gain: 0.42 },
  nowhere: { file: 'signs_to_nowhere.mp3', title: 'Signs To Nowhere', by: 'Shane Ivers', gain: 0.38 },
};
/** What each place plays, in no order. */
export const CUES: Record<Cue, string[]> = {
  night: ['cylEight', 'floating', 'ossuary1', 'longNote', 'lightless', 'shadows', 'phantom', 'cylTwo'],
  eerie: ['ossuary5', 'cylOne', 'decoherence', 'longNote'],
  tension: ['oldOnes', 'longNote', 'lightless', 'phantom'],
  motel: ['solitude', 'ossuary5', 'cylOne'],
  drive: ['neon', 'cylTwo', 'floating', 'cylEight'],
  town: ['nowhere', 'docks'],
  dawn: ['hymn', 'cylEight', 'floating'],
};
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export const scoreOn = import.meta.env.MODE !== 'single';

export class Score {
  /** The place's music playing now (or fading out), for the developer HUD and the tests. */
  playing: Cue | null = null;
  /** The piece playing now (a key of TRACKS). */
  track: string | null = null;
  private el: HTMLAudioElement | null = null;
  private gain: GainNode | null = null;
  private rest = rnd(6, 14);           // seconds of quiet before the next piece starts
  private t = 0;
  private last = new Map<string, number>();   // when each piece last started
  private fading = false;

  constructor(private ctx: AudioContext, private dest: AudioNode) {}

  /** Every frame while a night runs. want: the place's music (or none);
   *  held: a scene holds the player, or a voice or the radio must be heard. */
  update(dt: number, want: Cue | null, held: boolean) {
    this.t += dt;
    if (this.playing) {
      if (held) this.fade(2.5, 6);
      else if (want !== this.playing && !this.fading) this.fade(5, rnd(3, 8));   // somewhere else now
      return;
    }
    if (held || !want) { this.rest = Math.max(this.rest, 4); return; }
    this.rest -= dt;
    if (this.rest > 0) return;
    this.start(want);
  }

  /** The next piece for a place: not the one just played, and the one heard longest ago. */
  pick(cue: Cue) {
    const list = CUES[cue];
    const fresh = list.filter((k) => k !== this.track);
    const pool = fresh.length ? fresh : list;
    const ages = pool.map((k) => this.t - (this.last.get(k) ?? -1e9));
    // the longest unheard is likeliest; a little chance so the order is not always the same
    let best = 0;
    for (let i = 1; i < pool.length; i++) if (ages[i] + rnd(0, 120) > ages[best] + rnd(0, 120)) best = i;
    return pool[best];
  }

  /** A place's music, or one piece by name (the developer menu). */
  start(cue: Cue, track = this.pick(cue)) {
    if (!scoreOn) return;
    this.stop();
    const c = TRACKS[track];
    const el = new Audio();
    el.preload = 'auto';
    el.src = new URL(`music/${c.file}`, document.baseURI).href;
    const g = this.ctx.createGain(); g.gain.value = 0;
    try { this.ctx.createMediaElementSource(el).connect(g); } catch { return; }
    g.connect(this.dest);
    const now = this.ctx.currentTime;
    g.gain.setValueAtTime(0, now); g.gain.linearRampToValueAtTime(c.gain, now + 6);
    el.addEventListener('ended', () => { if (this.el === el) this.done(rnd(4, 15)); });
    el.addEventListener('error', () => { if (this.el === el) this.done(120); });   // not there: try later
    el.play().catch(() => { if (this.el === el) this.done(30); });
    this.el = el; this.gain = g; this.playing = cue; this.track = track; this.fading = false;
    this.last.set(track, this.t);
  }

  /** Fade the piece out over `secs`, then `quiet` seconds before the next. */
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

  /** The pause menu: the piece stops where it is and goes on after. */
  pause(on: boolean) { if (this.el) { if (on) this.el.pause(); else void this.el.play().catch(() => {}); } }

  private done(quiet: number) {
    this.stop();
    this.rest = quiet;
  }
}
