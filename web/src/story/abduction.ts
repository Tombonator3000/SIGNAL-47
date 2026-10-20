import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { DocSpec } from '../ui/UI';
import { clockText } from './time';
import { artImage, artLoaded, loadArtSome, ABDUCT_ART, type ArtId } from '../core/art';
import { Silhouette, type Pose } from '../core/roadside';
import type { LineId } from './lines';
import { Walker, PATH_DT, type Path, type Rec } from './recording';
import { facing } from './seen';

// The time away (HENDELSER.md, section 8; Tom, 16 October: yes to all four questions). On foot,
// alone, outdoors on one of the lines, at night: the pressure rises, and at the top of it they
// take a recording of you, the way you take a picture of an antenna. Nobody in the game says so.
//
//   the warning  always 47 s, timed as the 47 s after the line went dead at 02:13 (Prologue.
//                startCountdown: the low beep at 16, the printer at 31, K3 at 40, the event at 47;
//                FORSLAG.md, 20 October, item 2). The corner clock goes over to seconds, as the
//                station clock showed them that night, and counts the warning's own: HH:MM:00 to :47
//                (warnClock). At once the crickets stop. At :16 a small beep, and the player's own
//                footsteps are gone. At :31 the pressure comes in, and the flashlight starts to
//                stutter. At :40 it dies with the click K3 made. Whoever remembers the prologue
//                knows there are seven seconds left. Get off the line or in among people before the
//                light comes, and it passes: from the start of it there is time to reach shelter
//                some 140 m off at a run, from :40 some 20 m.
//   the light    white, from above, from nothing to be seen.
//   the taking   the picture freezes with a tracking fault, the sound is cut, a few single
//                frames (the light; the grass from above with your shadow in it; a hand with
//                fingers too long at the edge; yourself lying in the grass from above), black.
//   lost time    17 to 47 minutes, never past the next fixed point of the story.
//   over you     (FORSLAG.md, "Valg for Tom" 3; Tom, 20 October: "Spenning og skummelt slår
//                faste regler") on your back, the stars, and no sound at all, not even the
//                crickets. You blink and lift your head to look down along yourself (your chest,
//                your legs, your boots up against the frost), and at the top of the view, a few
//                metres off beyond your feet, one of them stands with its head bent down to you:
//                the outline from the glimpses, black against the sky or something lit (you lie
//                the way where it stands out), nearer than they ever come, and still. A second and
//                a half, or until your eyes go up to it, and it is not there; the crickets start
//                again all at once and the flashlight comes on. The first time it is a little
//                aside and some way off; the second time nearer, just past your boots, in the
//                frost round you; the third time there are no stars first, and there are two.
//   back         on your back in the grass, the stars; the view lifts. 100 to 400 m further
//                along, in a ring of flattened grass with frost on it; the flashlight beside
//                you, on. The truck's engine has died where it stood and wants the key.
//   traces       the lost time, one exposure too many on the field camera (yourself from
//                above), a line in your own hand you do not remember writing, a nosebleed that
//                stopped by itself. And afterwards, where you were taken, someone in a SARO
//                jacket stands quite still, the reflective band catching the light, for the rest
//                of the night: the recording (R5). Gone if you walk up to it. The last 47 seconds
//                are kept with it (where you went, which way you looked), and once, later, when
//                you are back within sight of the place, it walks them (story/recording.ts).
//
// At STATION 01, after N. Vega's call in chapter three, everyone goes (FORSLAG.md, 20 October,
// item 9 A; story/stationTaken.ts): a player not yet taken gets the warning on the way from the
// hut, and there is no getting away from it there (force).
//
// In the truck (FORSLAG.md, 20 October, item 10; story/truckTaken.ts): over B at SARO's turn-off,
// southbound, at the night's peak, the warning comes in the cab (beginCab). The same 47 seconds;
// the truck has the beats the flashlight and the footsteps have on foot (the engine, the tyres, the
// headlights), and it is the truck they have hold of: the engine catching, or the player out of it
// and away from it, lets the warning pass (AbductIn.cabFree), and nothing else does. Taken, the
// player wakes at the wheel 300 m further on, facing the wrong way (AbductDeps.cab puts the truck
// there), and the one over them is on the bonnet, bent to the glass, black against SARO's lamps.
//
// At least 40 minutes of the night between two, at most three in a night; the director sees to
// one before chapter five. The setting turns them off (on by default). main.ts decides when it
// may run (chapters one to four, at night, no panel, no cinematic), the night's curve when (its
// peak, story/curve.ts); story/lines.ts where the lines are.
//
// The single frames are ChatGPT's where they have come (ART_BRIEF.md, round 14, abduct/; asked for
// when the warning starts): the light with your legs in it, the truck standing open, three of them
// on the ridge. The hand is drawn here, and you from above are the scene itself, drawn from a
// camera over the place you lay (it shows the real place); the painted one from above is there
// for when that cannot be drawn, in the frames and on the roll.

/** The lines (story/lines.ts says where each crosses the ground). */
export type Line = LineId;
export interface AbductIn {
  ok: boolean;
  /** The setting (on by default). */
  enabled: boolean;
  /** On foot, outdoors, alone, on a line: which. Null when not. */
  line: Line | null;
  /** How the line runs here (a unit vector on the ground), to come back further along it. */
  along: THREE.Vector3;
  clock: number;
  /** The clock by which the story's next fixed point comes; the lost time stops short of it. */
  nextFixed: number;
  /** Chapter three or four: the one before chapter five is due. */
  late: boolean;
  calm: boolean;
  /** How much faster the pressure rises here (the antenna field before its dishes have turned,
   *  story/dishesFollow.ts); 1 when left out. */
  boost?: number;
  /** The night's curve (story/curve.ts) is at its peak: the warning may begin. Before that the
   *  pressure on the line still rises, and waits at the top. Left out, it may. */
  may?: boolean;
  /** The warning in the truck has let go of the player (story/truckTaken.ts: the engine caught, or
   *  they are out of the truck and away from it): it passes. */
  cabFree?: boolean;
}
/** The truck, as the time away in it needs it (main.ts, story/truckTaken.ts). */
export interface AbductCab {
  /** The player is at the wheel now (else out of the truck, on foot). */
  inCab(): boolean;
  /** The truck in the world: its group (its own metres, -z forward), or null. */
  group(): THREE.Object3D | null;
  /** The head's look in the cab (from straight ahead: yaw left, pitch up), set at once, and kept
   *  within a box (null lets it go). */
  look(yaw: number, pitch: number): void;
  lookNow(): { yaw: number; pitch: number } | null;
  lookBox(b: { yaw: [number, number]; pitch: [number, number] } | null): void;
  /** Where the truck will be put down (world, on the ground), once the light has come. */
  plan(): THREE.Vector3 | null;
  /** The waking: the truck put down further on (inCab: the player at the wheel), and where it
   *  stands now (world, on the ground). */
  wake(lost: number, inCab: boolean): THREE.Vector3 | null;
  /** The notes of it, after the time away's own (a: the clock it was taken, b: the clock now). */
  woke(a: string, b: string, inCab: boolean): void;
  /** What the waking says once the view has come up, at the wheel. */
  backText(): string;
  /** The light from above, into the cab through the glass (0..1). */
  light(k: number): void;
}
export interface AbductDeps {
  audio: AudioSys;
  player: Player;
  scene: THREE.Scene;
  overlay: HTMLElement;
  /** A frame of the scene seen by this camera, drawn at once (main.ts renders it). */
  shoot: (cam: THREE.PerspectiveCamera) => HTMLCanvasElement | null;
  /** The scene from this camera, small (w by h, RGBA, the bottom row first), as it is before the
   *  tape, with the open sky pure magenta: what would be behind them (main.ts). */
  probe?: (cam: THREE.PerspectiveCamera, w: number, h: number) => Uint8Array | null;
  ground: (x: number, z: number) => number;
  /** Ground a player can stand on (the area's zones, minus the boxes in the way). */
  standable: (x: number, z: number) => boolean;
  /** The truck, for the time away in it (story/truckTaken.ts; main.ts). */
  cab?: AbductCab;
  /** Off the asphalt, onto the verge, where the place has one (main.ts). */
  settle?: (p: THREE.Vector3) => void;
  flashlight: { have: boolean; on: boolean; flicker: (k: number) => void };
  /** The night's sounds to level (in about tau seconds), and back to all of them after secs
   *  (in about back seconds): Ambience.hush. */
  hush: (secs: number, level: number, tau?: number, back?: number) => void;
  glitch: (k: number) => void;
  skipClock: (sec: number) => void;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  /** The field camera: whether the player has it, and a picture on its roll. */
  camera: () => boolean;
  file: (doc: DocSpec) => void;
  keyStall: () => void;
  save: () => void;
  /** How much a light from near the eye (the flashlight, the headlights) falls on a point, 0 to 1:
   *  the reflective band on the one standing gives it back (story/recording.ts, Walker.shine). */
  shine?: (at: THREE.Vector3) => number;
}
/** What is kept with the case. dishes: the one night the array came down out of 026 and followed
 *  the player through a warning (story/dishesFollow.ts), when, which, and whether they are back. */
export interface AbductSave { count: number; last: number; spots: { scene: string; take: [number, number, number]; back: [number, number, number]; clock: number; pic?: string; doc?: DocSpec;
  /** Which time it was that one of them stood over you on waking (1 to 3); absent when none did
   *  (Missing time turned off on the way, the outlines not there, a case from before 20 October). */
  over?: number;
  /** The last 47 seconds before the light: x, z and the view's yaw every quarter second (story/
   *  recording.ts, PATH_DT); absent in a case from before 20 October. */
  path?: Path;
  /** When the recording walked them (the night's clock): once, never again. */
  walked?: number;
  /** STATION 01's, after N. Vega's call (story/stationTaken.ts). */
  station?: boolean;
  /** In the truck, over B at SARO's turn-off (story/truckTaken.ts): nobody stands for it, and
   *  nothing walks it (the recording is the truck itself, found further on). */
  cab?: boolean }[];
  dishes?: { clock: number; ids: string[]; back: boolean };
  /** STATION 01 after N. Vega's call (story/stationTaken.ts): 'taken' once the warning there has
   *  begun, 'passed' when Missing time was turned off on the way (then never again tonight). */
  station?: 'taken' | 'passed' }

type Stage = 'calm' | 'warning' | 'light' | 'taken' | 'black' | 'back';
const rnd = (a: number, b: number) => a + Math.random() * (b - a);
const UP = new THREE.Vector3(0, 1, 0);
const MAX = 3, GAP = 40 * 60, MIN_LOST = 17 * 60;
/** The warning's length, and its beats: the 47 seconds after the line went dead in the prologue
 *  (Prologue.startCountdown), the same seconds (story/dishesFollow.ts times the dishes by them). */
export const WARN = 47;
export const BEAT = { steps: 16, tone: 31, dark: 40 } as const;
// Over you on waking, the three times: whether the stars come first (then the blink, the head
// lifted behind it); how far off each one stands beyond your feet (metres), how far aside in the
// frame (of the half width) and how (the nearest with its head bent down to you); and up, where
// the nearest one's head is in the frame (of the half height above the middle): the view is turned
// down along you just so far, on any screen, so every player has it there without looking for it.
// Then how long it stays, at most.
const STARS_T = 1.3, BLINK_T = 0.24, OVER_T = 1.5;
const OVER: { stars: boolean; up: number; at: { dist: number; side: number; pose: Pose }[] }[] = [
  { stars: true, up: 0.66, at: [{ dist: 2.8, side: 0.2, pose: 'bent' }] },
  { stars: true, up: 0.6, at: [{ dist: 1.85, side: -0.05, pose: 'bent' }] },
  { stars: false, up: 0.64, at: [{ dist: 2.2, side: -0.3, pose: 'bent' }, { dist: 3.7, side: 0.38, pose: 'turned' }] },
];
/** The cold light of the sky along their edges out here. */
const OVER_RIM = 0x232c3a;
// Over you on waking in the truck (story/truckTaken.ts): at the wheel, slumped, the dash dark; the
// blink; the head comes up to look out through the glass, and one of them is on the bonnet with its
// head bent down to the windshield, black against SARO's lamps 300 m off (the truck faces north).
// Where each stands, in the truck's own metres (x right, z forward negative; the bonnet's top is at
// HOOD). Not nearer each time as on foot: nearer than about 2.3 m from the eye the roof over the
// windshield takes the head off it (seen from the seat the header is some 19 degrees up), and the
// head is what it is. So the first at the front of the bonnet, a little to the left; the second
// straight in front of the driver and a little closer, as near as the glass shows all of it; the
// third time two, the second one turned aside, with no dark first.
const HOOD = 1.12, SLUMP = -0.78;
const OVER_CAB: { stars: boolean; up: number; at: { x: number; z: number; pose: Pose }[] }[] = [
  { stars: true, up: 0.3, at: [{ x: -0.3, z: -2.45, pose: 'bent' }] },
  { stars: true, up: 0.27, at: [{ x: -0.42, z: -2.34, pose: 'bent' }] },
  { stars: false, up: 0.28, at: [{ x: -0.62, z: -2.4, pose: 'bent' }, { x: 0.46, z: -2.5, pose: 'turned' }] },
];
/** The driver's eye in the cab, the truck's own metres (drive/Truck.ts driverEye). */
const CAB_EYE = new THREE.Vector3(-0.42, 1.48, 0.2);
/** Which way you lie is chosen from this many (the view along you), for them to stand against
 *  the sky; the picture it is judged by is this many pixels across (main.ts, probe). */
const FACINGS = 8, PROBE_W = 112;
/** How well the open night sky stands behind them, as against a lit thing (0.6 at most). */
const SKY_AS = 0.32;

export class Abduction {
  stage: Stage = 'calm';
  /** The footsteps and the night's sounds are gone (main.ts mutes the steps). */
  silent = false;
  /** 0..1, how near the top (the developer menu, the tests). */
  pressure = 0;
  data: AbductSave = { count: 0, last: -1e9, spots: [] };
  /** The scene the player is in, as main.ts names it (rings and recordings belong to one). */
  sceneId = 'saro';
  private st = 0;
  private warnLen = WARN;
  /** Where in the warning it is up to (0: begun; 1: the beep, 2: the pressure, 3: the dark), and
   *  the clock it began at (the corner clock counts from its minute). */
  private beat = 0;
  private warnFrom = 0;
  private top = 1;
  private lost = 0;
  private from = 0;
  private take = new THREE.Vector3();
  private frames: HTMLCanvasElement[] = [];
  private canvas: HTMLCanvasElement;
  private tone: { stop: () => void; gain: GainNode } | null = null;
  /** The pressure under the warning, from its start to the taking (bought sound, AudioSys.pressure). */
  private bed: ReturnType<AudioSys['pressure']> = null;
  private rings: THREE.Mesh[] = [];
  /** The one standing where each time away was (n is which, 1 to 3): held while it walks (story/
   *  recording.ts), and hidden after that until its place has been out of the view. */
  private records: { g: THREE.Group; fig: Walker; scene: string; held: boolean; hide: boolean; none?: boolean }[] = [];
  /** The warning as it is lived (story/recording.ts): the player's way, a quarter second at a time. */
  private path: number[] = [];
  private pathT = 0;
  /** This warning cannot be got away from (story/stationTaken.ts): only Missing time turned off, or
   *  the night left, ends it. */
  forced = false;
  /** This warning is the truck's (story/truckTaken.ts): only the truck letting go ends it. */
  cab = false;
  /** In the truck at the wheel when the light came (the waking is at the wheel too). */
  private cabIn = false;
  private lying: THREE.Group;
  private hadLight = false;
  /** Over you on waking (the outlines are asked for when the warning starts), and where it is up
   *  to: the stars first, the blink, then there; and each one's head as seen from the eye, to know
   *  when the eyes have gone to it. */
  private overFigs: Silhouette[] = [new Silhouette('abduct:over', OVER_RIM, 'bent'), new Silhouette('abduct:over', OVER_RIM, 'turned')];
  private over: { v: number; t: number; phase: 'stars' | 'blink' | 'there'; pitch: number; yaw: number; dirs: THREE.Vector3[]; off: number[]; heads: THREE.Vector3[]; cab?: boolean } | null = null;
  /** Yourself as the eye has you, looking down along you on your back (only while one is there). */
  private self = selfFigure();
  private probeCam = new THREE.PerspectiveCamera();
  /** How each way you could have lain looked for them to stand in (the tests): its yaw, whether
   *  they could stand there, and how badly they would stand out (lower is better: see against). */
  facings: { yaw: number; reach: boolean; against: number }[] = [];
  /** Where the player will wake, once the light has come: it is chosen then (the dishes turn to
   *  it before anyone is there, story/dishesFollow.ts). */
  backAt: THREE.Vector3 | null = null;

  constructor(private d: AbductDeps) {
    const css = document.createElement('style');
    css.textContent = '.abduct-light::after{content:"";position:fixed;inset:0;pointer-events:none;z-index:39;opacity:var(--abduct,0);background:radial-gradient(ellipse at 50% -10%,#fff 0%,#eef4f6 45%,rgba(220,232,236,.85) 100%)}'
      // while one of them is over you, nothing on the screen but the night (the words, the buttons
      // come back with the crickets)
      + 'html.abduct-held .hud,html.abduct-held .touch{visibility:hidden!important}';
    document.head.appendChild(css);
    this.canvas = document.createElement('canvas'); this.canvas.width = 640; this.canvas.height = 400;
    this.canvas.className = 'abduct-frame';
    Object.assign(this.canvas.style, { position: 'fixed', inset: '0', width: '100%', height: '100%', objectFit: 'cover', pointerEvents: 'none', zIndex: '40', display: 'none', background: '#000' });
    d.overlay.appendChild(this.canvas);
    this.lying = jacketFigure(true); this.lying.visible = false; d.scene.add(this.lying);
  }

  get busy() { return this.stage !== 'calm'; }
  /** How far into the warning, and how long it runs (seconds, 47 but for the tests); null outside it. */
  get warn(): { t: number; len: number } | null { return this.stage === 'warning' ? { t: this.st, len: this.warnLen } : null; }
  /** Holds the player still (the taking and the waking). */
  get holds() { return this.stage === 'light' || this.stage === 'taken' || this.stage === 'black' || this.stage === 'back'; }
  /** The corner clock while the warning runs (main.ts, UI.clockSeconds): the minute it began, and
   *  the warning's own seconds, 00 to 47, the way the station clock showed seconds at 02:13; held
   *  at 47 through the light and the taking, until the waking has the night's clock back. Null
   *  otherwise. */
  get warnClock(): { hm: string; ss: string } | null {
    const s = this.stage === 'warning' ? Math.min(this.warnLen, Math.floor(this.st)) : this.stage === 'light' || this.stage === 'taken' || this.stage === 'black' ? this.warnLen : -1;
    if (s < 0) return null;
    return { hm: clockText(this.warnFrom, false), ss: String(Math.round(s)).padStart(2, '0') };
  }
  /** Whether the warning or the taking is under way (main.ts: no save then; the save menu is not a
   *  way out of the 47 seconds). The waking saves the case itself. */
  get racing() { return this.stage === 'warning' || this.stage === 'light' || this.stage === 'taken' || this.stage === 'black'; }

  update(dt: number, s: AbductIn) {
    this.st += dt;
    switch (this.stage) {
      case 'calm': return this.calm(dt, s);
      case 'warning': return this.warning(dt, s);
      case 'light': return this.light(s);
      case 'taken': return this.taken(s);
      case 'black': if (this.st > 2) this.wake(s); return;
      case 'back': return this.over ? this.overStep(dt, s) : this.back(dt);
    }
  }

  // ---------- the pressure ----------
  private calm(dt: number, s: AbductIn) {
    this.recordsStep();
    const room = s.nextFixed - s.clock >= MIN_LOST + 60;
    const can = s.ok && s.enabled && !!s.line && this.data.count < MAX && s.clock - this.data.last >= GAP && room;
    if (!can) { this.pressure = Math.max(0, this.pressure - dt / 90); return; }
    // about four to eight minutes alone on a line, sooner towards morning, much sooner when the
    // one before chapter five is due and has not come
    const day = ((s.clock % 86400) + 86400) % 86400, k = THREE.MathUtils.clamp((day - 2.3 * 3600) / (2.5 * 3600), 0, 1);
    this.pressure += dt / 360 * (0.7 + 0.8 * k) * (s.late && this.data.count === 0 ? 2.5 : 1) * (s.boost ?? 1);
    // (alone on a line in the build, it waits at the top for the peak)
    if (s.may === false) { this.pressure = Math.min(this.pressure, this.top); return; }
    if (this.pressure >= this.top) this.begin(s);
  }
  /** Start the warning now (the developer menu, the tests). */
  begin(s: AbductIn) {
    this.stage = 'warning'; this.st = 0; this.warnLen = WARN; this.silent = false;
    this.beat = 0; this.warnFrom = s.clock;
    this.path = []; this.pathT = 0; this.sample();
    this.hadLight = this.d.flashlight.on;
    this.take.copy(this.d.player.pos);
    this.cab = false; this.cabIn = false;
    void loadArtSome(ABDUCT_ART);
    for (const f of this.overFigs) f.wake();
    this.d.hush(this.warnLen + 12, 0);
    this.bed?.stop(); this.bed = this.d.audio.pressure();
  }
  /** Start the warning that cannot be got away from (STATION 01 after N. Vega's call). */
  force(s: AbductIn) { this.begin(s); this.forced = true; }
  /** Start the warning in the truck (story/truckTaken.ts): the player at the wheel. */
  beginCab(s: AbductIn) {
    this.begin(s); this.cab = true; this.path = [];
    const g = this.d.cab?.group();
    if (g) this.take.set(g.position.x, 0, g.position.z);
  }
  /** Whether the time away could begin now as far as it goes itself, wherever the player is: the
   *  setting, at most three, 40 minutes since the last, and room for the lost time before the
   *  story's next fixed point (story/truckTaken.ts asks this for the truck). */
  mayBegin(s: AbductIn) {
    return this.stage === 'calm' && s.enabled && this.data.count < MAX && s.clock - this.data.last >= GAP && s.nextFixed - s.clock >= MIN_LOST + 60;
  }
  /** The player where they are now, on the way the recording will walk (a quarter second apart). */
  private sample() {
    const p = this.d.player;
    this.path.push(Math.round(p.pos.x * 100) / 100, Math.round(p.pos.z * 100) / 100, Math.round(p.yaw * 100) / 100);
  }
  private warning(_dt: number, s: AbductIn) {
    const t = this.st, w = this.warnLen, a = this.d.audio;
    // got away: off the line, inside, among people, before the light (not from the one that is forced:
    // only Missing time turned off, or the night left, ends that; nor from the truck's, which only
    // the truck letting go ends)
    if (this.cab) { if (!s.enabled || s.cabFree) { this.passOver(); return; } }
    else if (!s.ok || !s.enabled || (!s.line && !this.forced)) { this.passOver(); return; }
    if (!this.cab) while (this.pathT + PATH_DT <= Math.min(t, w) + 1e-6) { this.pathT += PATH_DT; this.sample(); }
    // :16, the beep of 02:13 (the same low tone, and a small one over it that a phone's speaker
    // plays), and the footsteps are gone
    if (t >= BEAT.steps && this.beat < 1) {
      this.beat = 1; this.silent = true;
      a.beep(52, 2.8, 0.09, 0, 'sine'); a.beep(1046, 0.05, 0.035, 0, 'sine');
    }
    // :31, the pressure: in at once, then rising to the light; the flashlight begins to stutter
    if (t >= BEAT.tone && this.beat < 2) { this.beat = 2; if (!this.tone) this.toneOn(); }
    const k = THREE.MathUtils.clamp((t - BEAT.tone) / (w - BEAT.tone), 0, 1);
    if (this.tone) this.tone.gain.gain.value = 0.08 + 0.22 * k;
    if (this.bed) this.bed.gain.gain.value = t < BEAT.tone ? 0.015 : Math.min(0.3 * Math.min(1, (t - BEAT.tone) / 0.8) + 0.2 * k, 0.5);
    if (t >= BEAT.tone && t < BEAT.dark) this.d.flashlight.flicker(0.2 + 0.8 * (t - BEAT.tone) / (BEAT.dark - BEAT.tone));
    // :40, it dies, with the click K3 made when it tripped seven seconds before the event; seven
    // seconds of the dark (it does not come on again before the light)
    if (t >= BEAT.dark && this.beat < 3) {
      this.beat = 3;
      a.play(a.or('rack_switch', 'switch'), { gain: 0.75, rate: 0.62 });
      a.play('thudSoft', { gain: 0.32, rate: 1.4, when: 0.03 });
      this.d.flashlight.flicker(0);
    }
    if (this.beat >= 3 && this.d.flashlight.on) this.d.flashlight.on = false;
    if (t >= w) {
      this.stage = 'light'; this.st = 0; this.from = s.clock;
      if (this.cab) {
        // (in the truck: where the truck is; out of it, where the player is, and they wake by the
        // truck where it is put down)
        this.cabIn = !!this.d.cab?.inCab();
        const g = this.d.cab?.group();
        if (this.cabIn && g) this.take.set(g.position.x, 0, g.position.z); else this.take.copy(this.d.player.pos);
        this.backAt = this.d.cab?.plan() ?? null;
      } else { this.take.copy(this.d.player.pos); this.backAt = this.further(this.take, s.along); }
    }
  }
  private passOver() {
    this.stage = 'calm'; this.st = 0; this.silent = false; this.pressure = 0.35; this.top = rnd(0.85, 1.15); this.beat = 0; this.forced = false; this.path = []; this.cab = false;
    this.toneOff(); this.d.flashlight.flicker(0); this.d.hush(0.1, 1);
    this.bed?.stop(1); this.bed = null;   // got away: it ebbs over a few seconds
    if (this.hadLight && this.d.flashlight.have) this.d.flashlight.on = true;
  }

  // ---------- the light, the taking ----------
  private light(s: AbductIn) {
    const k = Math.min(1, this.st / 3);
    this.d.overlay.style.setProperty('--abduct', String(k));
    this.d.overlay.classList.add('abduct-light');
    // (at the wheel it comes into the cab from above, through the glass, before it is everything)
    if (this.cab && this.cabIn) this.d.cab?.light(Math.min(1, this.st / 1.6));
    if (this.st >= 3) {
      if (this.cab && this.cabIn) this.d.cab?.light(0);
      this.stage = 'taken'; this.st = 0;
      this.toneOff(); this.bed?.stop(); this.bed = null;
      this.d.glitch(1);
      this.frames = this.makeFrames();
      this.d.audio.suspend(true);
      const room = s.nextFixed - s.clock - 60;
      this.lost = Math.round(Math.min(rnd(17, 47) * 60, room));
    }
  }
  private taken(s: AbductIn) {
    const per = s.calm ? 1.4 : 0.95, i = Math.floor(this.st / per), k = this.st - i * per;
    this.d.overlay.classList.remove('abduct-light');
    this.d.glitch(Math.max(0, 1 - this.st * 2));
    if (i >= this.frames.length) { this.show(null); this.stage = 'black'; this.st = 0; return; }
    if (s.calm) this.show(this.frames[i], 0.35 * Math.min(1, k / 0.4, (per - k) / 0.4));
    else this.show(k < per - 0.12 ? this.frames[i] : null, 1);
  }
  private show(c: HTMLCanvasElement | null, alpha = 1) {
    const cv = this.canvas, g = cv.getContext('2d')!;
    g.fillStyle = '#000'; g.fillRect(0, 0, cv.width, cv.height);
    if (c) { g.globalAlpha = alpha; g.drawImage(c, 0, 0, cv.width, cv.height); g.globalAlpha = 1; }
    cv.style.display = 'block';
  }

  // ---------- back ----------
  private wake(s: AbductIn) {
    const p = this.d.player, d = this.d;
    d.skipClock(this.lost);
    const t1 = this.from + this.lost;
    // the truck's (story/truckTaken.ts): the truck put down further on, the player at its wheel, or
    // (out of it at the light) on their back on the verge beside it
    const cab = this.cab, inCab = cab && this.cabIn;
    const truckAt = cab ? d.cab?.wake(this.lost, inCab) ?? null : null;
    if (cab && !inCab && truckAt) {
      const g = d.cab?.group(), side = new THREE.Vector3(-1, 0, 0).applyAxisAngle(UP, g?.rotation.y ?? 0);
      this.backAt = truckAt.clone().addScaledVector(side, 3.4);
      if (!d.standable(this.backAt.x, this.backAt.z)) this.backAt = truckAt.clone().addScaledVector(side, -3.4);
    }
    let back: THREE.Vector3;
    if (inCab && truckAt) back = truckAt.clone();
    else {
      // further along the line, on ground that can be stood on
      back = this.backAt ?? this.further(this.take, s.along);
      p.place(back.x, back.z, p.yaw + rnd(-2, 2));
      p.pitch = 1.3; p.eye = 0.35;
      p.update(0, 0, 0, false);   // the view is there on the first frame after the black, not the one before
    }
    this.backAt = null;
    back.y = d.ground(back.x, back.z); this.take.y = d.ground(this.take.x, this.take.z);
    // (round the truck it is a wider ring, the truck in the middle of it, out over the asphalt)
    if (inCab && truckAt) this.ring(back, 2.05); else this.ring(back);
    if (cab && !inCab && truckAt) this.ring(truckAt.clone().setY(d.ground(truckAt.x, truckAt.z)), 2.05);
    d.flashlight.flicker(0);
    d.audio.suspend(false);
    this.canvas.style.display = 'none';
    this.silent = false;
    this.data.count++; this.data.last = t1;
    const spot: AbductSave['spots'][number] = { scene: this.sceneId, take: [this.take.x, this.take.y, this.take.z], back: [back.x, back.y, back.z], clock: this.from };
    if (this.path.length >= 6 && !cab) spot.path = this.path;
    if (this.forced) spot.station = true;
    if (cab) spot.cab = true;
    this.path = []; this.forced = false; this.cab = false;
    this.data.spots.push(spot);
    this.record(this.take, this.sceneId, cab);
    if (!inCab) d.keyStall();
    // the traces
    const a = clockText(this.from, false), b = clockText(t1, false);
    d.note(`${a} to ${b}. Nothing.`);
    d.note(`${b}. A line in the notebook in my own hand. I do not remember writing it: ${Math.random() < 0.5 ? '026' : clockText(s.nextFixed, false)}.`);
    d.note(`${b}. A nosebleed. It had stopped by itself.`);
    if (cab) d.cab?.woke(a, b, inCab);
    if (d.camera()) {
      const pic = (inCab ? this.fromAbove(back, false, true) : this.fromAbove(back, true)) ?? painted(inCab ? 'abductTruck' : 'abductAbove', 0.5, 21, 0.9);
      if (pic) {
        spot.doc = { id: `away${this.data.count}`, kind: 'photo', image: pic.toDataURL('image/jpeg', 0.8), page: '', title: `PHOTO / Not exposed by me (${a})`,
          transcript: inCab ? `One more exposure on the roll than there should be, with the counter at ${b}. Taken from above, from higher than anyone could stand: the truck standing in the road with every light out, white in a light from straight over it. Someone at the wheel. Me.`
            : `One more exposure on the roll than there should be, with the counter at ${b}. Taken from above, from higher than anyone could stand: someone lying on their back in the grass in a SARO jacket, the reflective band across the chest. Me.` };
        d.file(spot.doc);
      }
    }
    // over you: the first, the second, the third time (not with Missing time turned off on the way,
    // nor without the outlines: never the figure in code this near). After the pictures from
    // above, so it is in none of them. Until it is not there, no sound at all and no light; the case
    // is kept from then (a save has a picture of the frame).
    const v = Math.min(OVER.length, this.data.count) - 1;
    this.awakeCab = inCab;
    if (s.enabled && this.overFigs[0].painted) { spot.over = v + 1; if (inCab) this.overStartCab(v); else this.overStart(v); }
    else {
      if (d.flashlight.have && !inCab) d.flashlight.on = true;
      if (inCab) d.cab?.look(0, -0.6);
      d.hush(6, 0.2); d.save();
    }
    this.stage = 'back'; this.st = 0;
  }
  /** The waking is at the wheel (the time away in the truck), until it is over. */
  awakeCab = false;

  // ---------- over you, in the truck ----------
  /** At the wheel, slumped over it and dark; then the head comes up and one of them is on the bonnet. */
  private overStartCab(v: number) {
    const c = this.d.cab!;
    OVER_CAB[v].at.forEach((a, i) => this.overFigs[i].pose(a.pose));
    c.look(0, SLUMP);
    this.over = { v, t: 0, phase: 'stars', pitch: SLUMP, yaw: 0, dirs: [], off: [], heads: [], cab: true };
    this.overLooked = false;
    this.d.hush(60, 0, 0.05);
    document.documentElement.classList.add('abduct-held');
    if (!OVER_CAB[v].stars) this.overRevealCab();
  }
  private overRevealCab() {
    const o = this.over!, c = this.d.cab!, g = c.group(), spec = OVER_CAB[o.v], cam = this.d.player.camera;
    if (!g) { this.overGone(); return; }
    g.updateMatrixWorld(true);
    // the head raised to look out through the glass, a little towards the nearest of them, just so
    // far that its head is at `up` of the half height over the middle of the view, on any screen
    const ty = Math.tan(THREE.MathUtils.degToRad(cam.fov) / 2);
    const a0 = spec.at[0], hy = HOOD + this.overFigs[0].headY;
    const dx = a0.x - CAB_EYE.x, dz = a0.z - CAB_EYE.z, dist = Math.hypot(dx, dz);
    const yaw = 0.55 * Math.atan2(-dx, -dz), pitch = Math.atan2(hy - CAB_EYE.y, dist) - Math.atan(spec.up * ty);
    c.look(yaw, pitch);
    c.lookBox({ yaw: [yaw - 0.4, yaw + 0.4], pitch: [pitch - 0.3, pitch + 0.5] });
    o.pitch = pitch; o.yaw = yaw; o.phase = 'there'; o.t = 0;
    const eye = g.localToWorld(CAB_EYE.clone());
    const q = g.getWorldQuaternion(new THREE.Quaternion()).multiply(new THREE.Quaternion().setFromEuler(new THREE.Euler(pitch, yaw, 0, 'YXZ')));
    const look = new THREE.Vector3(0, 0, -1).applyQuaternion(q);
    spec.at.forEach((a, i) => {
      const f = this.overFigs[i], feet = g.localToWorld(new THREE.Vector3(a.x, HOOD, a.z));
      f.group.position.copy(feet);
      f.group.visible = true; this.d.scene.add(f.group);
      f.group.traverse((x) => { x.renderOrder = 2; });
      const at = feet.clone().setY(feet.y + f.headY), head = at.clone().sub(eye).normalize();
      o.heads.push(at); o.dirs.push(head); o.off.push(look.angleTo(head));
    });
  }

  // ---------- over you ----------
  private overStart(v: number) {
    const p = this.d.player;
    OVER[v].at.forEach((a, i) => this.overFigs[i].pose(a.pose));
    // which way you lie: still black, so the looking round is not seen
    p.yaw = this.facing(v);
    p.update(0, 0, 0, false);
    this.over = { v, t: 0, phase: 'stars', pitch: p.pitch, yaw: p.yaw, dirs: [], off: [], heads: [] };
    this.overLooked = false;
    this.d.hush(60, 0, 0.05);
    document.documentElement.classList.add('abduct-held');
    if (!OVER[v].stars) this.overReveal();
  }
  /** The head lifted to look down along yourself, and them where they stand. */
  private overReveal() {
    const o = this.over!, p = this.d.player, cam = p.camera;
    const L = this.layout(o.v, p.yaw);
    p.pitch = L.pitch;
    p.update(0, 0, 0, false);
    cam.updateMatrixWorld(true);
    o.pitch = p.pitch; o.yaw = p.yaw; o.phase = 'there'; o.t = 0;
    const eye = cam.position.clone(), look = new THREE.Vector3(0, 0, -1).applyEuler(new THREE.Euler(p.pitch, p.yaw, 0, 'YXZ'));
    L.at.forEach((a, i) => {
      const f = this.overFigs[i];
      f.group.position.copy(a.q);
      f.group.visible = true; this.d.scene.add(f.group);
      f.group.traverse((x) => { x.renderOrder = 2; });   // over the frost it stands in, not under it
      const at = new THREE.Vector3(a.q.x, a.head, a.q.z), head = at.clone().sub(eye).normalize();
      o.heads.push(at); o.dirs.push(head); o.off.push(look.angleTo(head));
    });
    // and you, along the bottom of the view: the jacket, the jeans, the boots up, dark
    this.self.position.set(p.pos.x, p.floorY, p.pos.z); this.self.rotation.y = p.yaw;
    this.self.visible = true; this.d.scene.add(this.self);
  }
  /** Where they stand this time if you lie facing yaw (on the ground there, the head's height), and
   *  how far the view is turned down for the nearest one's head to be at its height in the frame. */
  private layout(v: number, yaw: number) {
    const p = this.d.player, cam = p.camera, spec = OVER[v];
    const ty = Math.tan(THREE.MathUtils.degToRad(cam.fov) / 2), tx = ty * cam.aspect;
    const fwd = new THREE.Vector3(-Math.sin(yaw), 0, -Math.cos(yaw)), right = new THREE.Vector3(Math.cos(yaw), 0, -Math.sin(yaw));
    const eyeY = p.floorY + p.eye;
    const at = spec.at.map((a, i) => {
      const b = Math.atan(a.side * tx);
      const q = p.pos.clone().addScaledVector(fwd, a.dist * Math.cos(b)).addScaledVector(right, a.dist * Math.sin(b));
      q.y = this.d.ground(q.x, q.z);
      return { q, head: q.y + this.overFigs[i].headY };
    });
    return { at, right, pitch: Math.atan2(at[0].head - eyeY, spec.at[0].dist) - Math.atan(spec.up * ty) };
  }
  /** Which way you lie. Of a few ways round, one where they have ground to stand on with nothing
   *  between them and you, and stand out black against something lighter (HENDELSER.md, section 7:
   *  always a silhouette against something lighter): the sky, a lit dish, a lit wall, evenly behind
   *  the head and shoulders; not lost against the dark, not under a lamp's glare. The one you fell
   *  is kept when it is as good as any. */
  private facing(v: number) {
    const p = this.d.player;
    this.facings = [];
    let best = p.yaw, bestScore = Infinity;
    for (let i = 0; i < FACINGS; i++) {
      const yaw = p.yaw + (i * 2 * Math.PI) / FACINGS, L = this.layout(v, yaw);
      const reach = L.at.every((a) => this.reach(a.q)), against = this.against(L, yaw);
      this.facings.push({ yaw, reach, against });
      const score = (reach ? 0 : 100) + against - (i === 0 ? 0.01 : 0);
      if (score < bestScore) { bestScore = score; best = yaw; }
    }
    return best;
  }
  /** Ground one of them could stand on, and nothing in the way between it and you. */
  private reach(q: THREE.Vector3) {
    const p = this.d.player.pos;
    for (const k of [0.5, 0.75, 1]) if (!this.d.standable(p.x + (q.x - p.x) * k, p.z + (q.z - p.z) * k)) return false;
    return true;
  }
  /** How badly they would stand out, seen from where you lie facing yaw (lower is better): in a small
   *  picture of the scene without them, behind each head and shoulders, how much is open sky (the
   *  stars go out round the outline), and where it is not, how dark and how uneven what stands
   *  there is (they are black: lost against the dark side of a dish, a pole, an edge, clear against
   *  a lit dish or a lit wall); and how much glare there is round about (a lamp's disc behind or
   *  beside the head washes the outline out on the tape, and is a halo). */
  private against(L: ReturnType<Abduction['layout']>, yaw: number) {
    const probe = this.d.probe;
    if (!probe) return 0;
    const p = this.d.player, cam = p.camera, pc = this.probeCam;
    pc.fov = cam.fov; pc.aspect = cam.aspect; pc.near = cam.near; pc.far = cam.far; pc.updateProjectionMatrix();
    pc.position.set(p.pos.x, p.floorY + p.eye, p.pos.z); pc.rotation.set(L.pitch, yaw, 0, 'YXZ'); pc.updateMatrixWorld(true);
    const w = cam.aspect >= 1 ? PROBE_W : Math.round(PROBE_W * 0.6), h = Math.max(24, Math.round(w / cam.aspect));
    const px = probe(pc, w, h);
    if (!px) return 0;
    const v = new THREE.Vector3();
    // the pixels of a box standing at q: half metres either side, from y0 to y1 above the ground
    const box = (q: THREE.Vector3, half: number, y0: number, y1: number) => {
      v.copy(q).addScaledVector(L.right, -half).setY(q.y + y1).project(pc);
      const ax = v.x, ay = v.y;
      v.copy(q).addScaledVector(L.right, half).setY(q.y + y0).project(pc);
      const cx = (n: number) => THREE.MathUtils.clamp(Math.round((n + 1) / 2 * w), 0, w), cy = (n: number) => THREE.MathUtils.clamp(Math.round((n + 1) / 2 * h), 0, h);
      return [cx(ax), cx(v.x), cy(v.y), cy(ay)];
    };
    const sky = (i: number) => px[i] > 246 && px[i + 1] < 10 && px[i + 2] > 246;
    const lum = (i: number) => Math.pow((0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2]) / 255, 1 / 2.2);
    let score = 0;
    for (const a of L.at) {
      const hy = a.head - a.q.y;
      const [x0, x1, y0, y1] = box(a.q, 0.34, hy - 0.36, hy + 0.3);
      let n = 0, open = 0, sum = 0, sq = 0;
      for (let y = y0; y < y1; y++) for (let x = x0; x < x1; x++) {
        const i = (y * w + x) * 4; n++;
        if (sky(i)) open++; else { const l = lum(i); sum += l; sq += l * l; }
      }
      const [X0, X1, Y0, Y1] = box(a.q, 1.5, 0.25, hy + 1.4);
      let N = 0, hot = 0;
      for (let y = Y0; y < Y1; y++) for (let x = X0; x < X1; x++) { const i = (y * w + x) * 4; N++; if (!sky(i) && lum(i) > 0.7) hot++; }
      const k = n - open, mean = k ? sum / k : 0, sd = k ? Math.sqrt(Math.max(0, sq / k - mean * mean)) : 0;
      const skyF = n ? open / n : 0, glare = N ? hot / N : 0;
      score += -(skyF * SKY_AS + (1 - skyF) * (Math.min(mean, 0.6) - 1.5 * sd)) + 5 * glare;
    }
    return score;
  }
  /** Still on your back: the stars, the blink, then it is there for a second and a half. It goes
   *  when that is up, or when the eyes go to it; and not with an effect: it was not there. */
  private overStep(dt: number, s: AbductIn) {
    const o = this.over!, p = this.d.player;
    if (!s.enabled) { this.overGone(); return; }   // Missing time turned off in the pause menu: not there
    o.t += dt;
    if (o.phase === 'stars') {
      if (o.t >= STARS_T) { o.phase = 'blink'; o.t = 0; this.show(null); }
      return;
    }
    if (o.phase === 'blink') {
      if (o.t >= BLINK_T) { if (o.cab) this.overRevealCab(); else this.overReveal(); this.canvas.style.display = 'none'; }
      return;
    }
    // (in the cab the eyes are the camera's, turned by the drive's look and the truck under it)
    const look = o.cab ? p.camera.getWorldDirection(new THREE.Vector3()) : new THREE.Vector3(0, 0, -1).applyEuler(new THREE.Euler(p.pitch, p.yaw, 0, 'YXZ'));
    const looked = o.dirs.some((d, i) => look.angleTo(d) < o.off[i] - 0.05);
    if (looked) this.overLooked = true;
    if (o.t >= OVER_T || looked) this.overGone();
  }
  private overGone() {
    const cab = !!this.over?.cab;
    this.overDrop();
    // the crickets, all at once, and the rest of the night after them; the light, on (in the cab:
    // the instruments, story/truckTaken.ts)
    this.d.hush(5, 0.6, 0.06, 1.8);
    if (this.d.flashlight.have && !cab) this.d.flashlight.on = true;
    this.d.save();
    this.st = 0;
  }
  private overDrop() {
    for (const f of this.overFigs) { f.group.visible = false; f.group.removeFromParent(); }
    this.self.visible = false; this.self.removeFromParent();
    document.documentElement.classList.remove('abduct-held');
    if (this.over?.phase === 'blink') this.canvas.style.display = 'none';
    if (this.over?.cab) this.d.cab?.lookBox(null);
    this.over = null;
  }
  /** The look while held at the wheel (main.ts gives it to the drive, which keeps it in the box):
   *  only while one of them is there, and stiffly. */
  cabGaze(dx: number, dy: number) { return this.over?.cab && this.over.phase === 'there' ? { x: dx * 0.45, y: dy * 0.45 } : { x: 0, y: 0 }; }
  /** The look while you are held (main.ts): only while one of them is there, and stiffly; towards
   *  it is enough. */
  gaze(dx: number, dy: number) {
    const o = this.over, p = this.d.player;
    if (!o || o.phase !== 'there') return;
    p.yaw = THREE.MathUtils.clamp(p.yaw - dx * 0.45, o.yaw - 0.4, o.yaw + 0.4);
    p.pitch = THREE.MathUtils.clamp(p.pitch - dy * 0.45, o.pitch - 0.3, o.pitch + 0.5);
  }
  /** The last time one of them was over you, it went because the eyes went up to it (story/seen.ts). */
  overLooked = false;
  /** Where waking is up to with one of them over you, or null (the tests). */
  get overPhase() { return this.over?.phase ?? null; }
  /** The middle of each one's head while they are there (the tests). */
  get overHeads() { return this.over?.phase === 'there' ? this.over.heads.map((h) => h.toArray()) : []; }
  /** A save's picture is taken without them (main.ts writeSave); true when they were there. */
  hideOver(hide: boolean) {
    if (this.over?.phase !== 'there') return false;
    for (const f of this.overFigs) if (f.group.parent) f.group.visible = !hide;
    this.self.visible = !hide;
    return true;
  }
  private back(dt: number) {
    if (this.awakeCab) { this.backCab(dt); return; }
    const p = this.d.player, k = Math.min(1, this.st / 6);
    p.eye = 0.35 + (1.62 - 0.35) * THREE.MathUtils.smoothstep(k, 0.3, 1);
    p.pitch += (-0.06 - p.pitch) * Math.min(1, dt * (k > 0.4 ? 1.4 : 0.2));
    if (this.st > 2.5 && this.st - dt <= 2.5) this.d.toast('On your back in the grass. Stars. The flashlight is lying beside you, on. The grass round you is flat, and white with frost.', 6);
    if (k >= 1) { p.eye = 1.62; this.stage = 'calm'; this.st = 0; this.pressure = 0; this.top = rnd(0.85, 1.15); }
  }
  /** At the wheel: the head comes down to the dash, where the odometer is, and the night goes on. */
  private backCab(dt: number) {
    const c = this.d.cab, now = c?.lookNow();
    // (down to the instruments, where the odometer is under the speedometer)
    if (now) c!.look(now.yaw + (0 - now.yaw) * Math.min(1, dt * 1.2), now.pitch + (-0.6 - now.pitch) * Math.min(1, dt * 1.2));
    if (this.st > 1.6 && this.st - dt <= 1.6 && c) this.d.toast(c.backText(), 6.5);
    if (this.st >= 4) { this.awakeCab = false; this.stage = 'calm'; this.st = 0; this.pressure = 0; this.top = rnd(0.85, 1.15); }
  }
  private further(from: THREE.Vector3, along: THREE.Vector3) {
    const dir = along.clone().setY(0).normalize();
    for (const dist of [rnd(220, 400), rnd(100, 220), 100, 60, 30]) for (const sgn of Math.random() < 0.5 ? [1, -1] : [-1, 1]) {
      const q = from.clone().addScaledVector(dir, dist * sgn);
      this.d.settle?.(q);
      if (this.d.standable(q.x, q.z)) return q;
    }
    return from.clone();
  }

  // ---------- what is left ----------
  private ring(at: THREE.Vector3, scale = 1) {
    // (patchy, soft at both edges: seen from the grass, lying in it, as well as standing)
    const geo = new THREE.RingGeometry(1.3 * scale, 2.7 * scale, 40, 1); geo.rotateX(-Math.PI / 2);
    const m = new THREE.Mesh(geo, new THREE.MeshBasicMaterial({ color: 0xc8d4dc, map: frost(), transparent: true, opacity: 0.32, depthWrite: false }));
    m.position.set(at.x, at.y + 0.03, at.z); m.name = 'abduct:ring';
    this.d.scene.add(m); this.rings.push(m);
  }
  private record(at: THREE.Vector3, scene: string, none = false) {
    const fig = new Walker('abduct:record'), g = fig.group;
    g.position.copy(at); g.rotation.y = rnd(0, Math.PI * 2);
    g.visible = false; this.d.scene.add(g); this.records.push({ g, fig, scene, held: false, hide: false, none });
    g.userData.home = { x: at.x, y: at.y, z: at.z, yaw: g.rotation.y };
  }
  /** The recordings standing where the player was taken that are shown from here (story/seen.ts):
   *  which time away each is from (1 to 3), and where it stands (world terms, at the ground). Not
   *  the one walking (story/recording.ts writes that down itself). */
  get recordsShown() { return this.records.flatMap((r, i) => r.g.visible && !r.held ? [{ n: i + 1, at: r.g.position }] : []); }
  /** The recordings stand where they were taken; walk up to one and it was not there. */
  private recordsStep() {
    const c = this.d.player.pos;
    for (const r of this.records) {
      if (r.held || r.none) continue;
      const far = r.scene === this.sceneId && Math.hypot(r.g.position.x - c.x, r.g.position.z - c.z) > 45;
      // (back where it stands after it walked: only once that place has been out of the view)
      if (r.hide && (!far || !facing(this.d.player.camera, this.recHead.copy(r.g.position).setY(r.g.position.y + 1.2), 1000, 50))) r.hide = false;
      r.g.visible = far && !r.hide;
      if (r.g.visible) r.fig.shine(this.d.shine?.(this.recHead.copy(r.g.position).setY(r.g.position.y + 1.2)) ?? 0, this.d.player.camera);
    }
  }
  private recHead = new THREE.Vector3();
  /** The times away as the recording's walk sees them (story/recording.ts). */
  recordings(): Rec[] {
    return this.records.flatMap((r, i) => {
      const sp = this.data.spots[i];
      return sp && !sp.cab ? [{ n: i + 1, scene: r.scene, take: new THREE.Vector3(...sp.take), path: sp.path, walked: sp.walked, clock: sp.clock, fig: r.fig,
        markWalked: (c: number | undefined) => { if (c === undefined) delete sp.walked; else sp.walked = c; } }] : [];
    });
  }
  /** The walk has the one standing for time away n (held), or gives it back: to where it stands,
   *  hidden until that place has been out of the view. */
  holdRecord(n: number, held: boolean) {
    const r = this.records[n - 1];
    if (!r) return;
    r.held = held;
    if (!held) {
      const h = r.g.userData.home as { x: number; y: number; z: number; yaw: number };
      r.g.position.set(h.x, h.y, h.z); r.g.rotation.set(0, h.yaw, 0); r.fig.stand();
      r.hide = true; r.g.visible = false;
    }
  }
  /** Yourself lying in the grass, from above (the frame and the extra exposure). */
  private fromAbove(at: THREE.Vector3, me: boolean, truck = false) {
    const y = this.d.ground(at.x, at.z);
    if (me) { this.lying.position.set(at.x, y + 0.12, at.z); this.lying.rotation.y = rnd(0, Math.PI * 2); this.lying.visible = true; }
    const cam = new THREE.PerspectiveCamera(40, 1.6, 0.5, 400);
    // (the truck: higher, the whole of it and the ring round it in the frame)
    cam.position.set(at.x + 0.3, y + (truck ? 13 : me ? 7.5 : 5.5), at.z + 0.2); cam.lookAt(at.x, y, at.z);
    cam.updateMatrixWorld(true);
    // their light, from straight above: the only time the ground here is seen like this
    const L = new THREE.PointLight(0xeef4ff, truck ? 320 : 140, truck ? 40 : 30, 1.6); L.position.set(at.x, y + (truck ? 14 : 9), at.z);
    this.d.scene.add(L); L.updateMatrixWorld(true);
    const c = this.d.shoot(cam);
    L.removeFromParent(); L.dispose();
    this.lying.visible = false;
    return c ? grain(c, 7, me ? 0.9 : 1.3) : null;
  }
  private makeFrames() {
    // the light: painted, with your legs in it and the shadow going from under them; else the light
    // drawn here and the grass from above with nobody in it
    const light = painted('abductLight', 0.25, 3);
    const out: HTMLCanvasElement[] = [light ?? frameLight(3)];
    // (at the wheel: the truck, not where the player's body was left when they got in)
    const truck = this.cab && this.cabIn ? this.d.cab?.group() : null;
    if (!light) { const grass = this.fromAbove(truck?.position ?? this.d.player.pos, false); if (grass) out.push(grass); }
    out.push(frameHand(11, artLoaded('abductLight') ? artImage('abductLight') : null));
    for (const [id, y, seed] of [['abductTruck', 0.5, 13], ['abductFigures', 0.45, 17]] as const) { const f = painted(id, y, seed); if (f) out.push(f); }
    // (at the wheel: the truck from above, with the player in it)
    const me = (truck ? this.fromAbove(truck.position, false, true) : this.fromAbove(this.d.player.pos, true)) ?? painted('abductAbove', 0.5, 19, 0.9);
    if (me) out.push(me);
    return out;
  }

  // ---------- the tone ----------
  private toneOn() {
    const ctx = this.d.audio.ctx;
    if (!ctx) return;
    const g = ctx.createGain(); g.gain.value = 0;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 160;
    const o = [37, 52.5, 74].map((f) => { const x = ctx.createOscillator(); x.frequency.value = f; x.connect(lp); x.start(); return x; });
    lp.connect(g); g.connect(this.d.audio.sfx);
    // without the bought pressure (a build without the licensed folder, as Pages has it) all of the
    // tone is below what a phone's speaker plays, and :31 would pass unheard there: a little of its
    // upper partials over it, a hair off so they beat slowly (the ear puts the 74 Hz back under them)
    if (!this.bed) {
      const up = ctx.createGain(); up.gain.value = 0.16; up.connect(g);
      for (const f of [222.3, 296.5, 369.4]) { const x = ctx.createOscillator(); x.frequency.value = f; x.connect(up); x.start(); o.push(x); }
    }
    this.tone = { gain: g, stop: () => { for (const x of o) { try { x.stop(); } catch { /* stopped */ } } g.disconnect(); } };
  }
  private toneOff() { this.tone?.stop(); this.tone = null; }

  // ---------- the case ----------
  restore(v: AbductSave | null | undefined) {
    this.clear();
    this.data = v ? { count: v.count, last: v.last, spots: [...v.spots], ...(v.dishes ? { dishes: { ...v.dishes, ids: [...v.dishes.ids] } } : {}), ...(v.station ? { station: v.station } : {}) } : { count: 0, last: -1e9, spots: [] };
    for (const sp of this.data.spots) {
      this.ring(new THREE.Vector3(...sp.back), sp.cab && Math.hypot(sp.back[0] - sp.take[0], sp.back[2] - sp.take[2]) > 100 ? 2.05 : 1);
      this.record(new THREE.Vector3(...sp.take), sp.scene, !!sp.cab);
    }
  }
  /** Out to the title in the middle of it (main.ts, toTitle): nothing of the warning or the taking is
   *  left running behind the title (the tone, the pressure, the light, the frames, the night hushed
   *  for the rest of the 47 seconds, the flashlight's stutter). The case's data stays as it is; the
   *  night opened next restores its own. */
  halt() {
    if (this.stage === 'calm') return;
    this.d.flashlight.flicker(0); this.d.hush(0.1, 1); this.d.glitch(0);
    this.clear();
  }
  /** The exposures that were not the player's, back among the papers (after the case is opened). */
  refile() { for (const sp of this.data.spots) if (sp.doc) this.d.file(sp.doc); }
  private clear() {
    this.toneOff(); this.bed?.stop(); this.bed = null; this.silent = false; this.stage = 'calm'; this.st = 0; this.beat = 0; this.pressure = 0; this.top = rnd(0.85, 1.15); this.backAt = null;
    this.forced = false; this.path = []; this.cab = false; this.cabIn = false; this.awakeCab = false;
    this.canvas.style.display = 'none'; this.d.overlay.classList.remove('abduct-light');
    this.d.cab?.light(0);
    this.overDrop();
    for (const m of this.rings) { m.removeFromParent(); m.geometry.dispose(); (m.material as THREE.Material).dispose(); }
    for (const r of this.records) r.g.removeFromParent();
    this.rings = []; this.records = [];
  }
}

// ---------- the figures and the frames ----------
/** The frost on the flattened grass: thick in the middle of the ring, thinning out at both edges in
 *  patches, as frost lies (white where the alpha is; the ring's own UVs are flat, so a square). */
let frostTex: THREE.CanvasTexture | null = null;
function frost() {
  if (frostTex) return frostTex;
  const N = 256, c = document.createElement('canvas'); c.width = c.height = N;
  const g = c.getContext('2d')!, im = g.createImageData(N, N);
  let sd = 47;
  const r = () => { sd = (sd * 1664525 + 1013904223) >>> 0; return sd / 4294967296; };
  // a coarse noise of a few octaves, round the ring
  const cells = Array.from({ length: 3 }, (_, o) => { const n = 8 << o; return { n, v: Array.from({ length: n * n }, r) }; });
  const noise = (x: number, y: number) => {
    let t = 0, w = 0;
    cells.forEach(({ n, v }, o) => {
      const fx = x * (n - 1), fy = y * (n - 1), ix = Math.floor(fx), iy = Math.floor(fy), ax = fx - ix, ay = fy - iy;
      const at = (i: number, j: number) => v[Math.min(n - 1, j) * n + Math.min(n - 1, i)];
      const k = 1 / (1 + o);
      t += k * ((at(ix, iy) * (1 - ax) + at(ix + 1, iy) * ax) * (1 - ay) + (at(ix, iy + 1) * (1 - ax) + at(ix + 1, iy + 1) * ax) * ay); w += k;
    });
    return t / w;
  };
  for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
    const u = (x + 0.5) / N * 2 - 1, v = (y + 0.5) / N * 2 - 1, d = Math.hypot(u, v) * 2.7;   // metres from the middle
    const band = THREE.MathUtils.smoothstep(d, 1.25, 1.75) * (1 - THREE.MathUtils.smoothstep(d, 2.0, 2.72));
    // patches, and in them the grain of the frost on each blade
    const a = band * THREE.MathUtils.clamp((noise((x + 0.5) / N, (y + 0.5) / N) - 0.32) * 2.4, 0, 1) * (0.55 + 0.45 * r());
    const i = (y * N + x) * 4; im.data[i] = im.data[i + 1] = im.data[i + 2] = 255; im.data[i + 3] = Math.round(255 * a);
  }
  g.putImageData(im, 0, 0);
  frostTex = new THREE.CanvasTexture(c); frostTex.colorSpace = THREE.SRGBColorSpace;
  return frostTex;
}
/** You, as the eye has you on your back with the head lifted, looking down along yourself: the
 *  jacket over the chest, the jeans, the boots standing up at the end; built facing -z from under
 *  the eye, dark (no light on you: the flashlight is out), only ever an outline against the frost. */
function selfFigure() {
  const g = new THREE.Group(); g.name = 'abduct:self';
  const coat = new THREE.MeshBasicMaterial({ color: 0x0d0f12 });
  const jeans = new THREE.MeshBasicMaterial({ color: 0x0c0f14 }), boot = new THREE.MeshBasicMaterial({ color: 0x090807 });
  const put = (geo: THREE.BufferGeometry, m: THREE.Material, x: number, y: number, z: number, rx = 0, sx = 1) => {
    const b = new THREE.Mesh(geo, m); b.position.set(x, y, z); b.rotation.x = rx; b.scale.x = sx; g.add(b);
  };
  // the chest and the belly, lying along the view, low
  put(new THREE.CapsuleGeometry(0.1, 0.4, 4, 10), coat, 0, 0.06, -0.34, Math.PI / 2, 1.7);
  for (const sd of [-1, 1]) {
    // the legs a little apart, from the hips to the ankles, and the boots up, toes a little out
    put(new THREE.CapsuleGeometry(0.068, 0.82, 4, 8), jeans, sd * 0.115, 0.075, -1.06, Math.PI / 2);
    const b = new THREE.Mesh(new THREE.CapsuleGeometry(0.052, 0.12, 4, 8), boot);
    b.position.set(sd * 0.125, 0.14, -1.55); b.rotation.set(-0.22, 0, sd * 0.26); b.scale.z = 1.4; g.add(b);
  }
  g.traverse((x) => { x.frustumCulled = false; });
  g.visible = false;
  return g;
}
/** Someone in a SARO jacket, the reflective band across it: standing, or lying on their back. */
function jacketFigure(lying: boolean) {
  const g = new THREE.Group(), body = new THREE.Group();
  const coat = new THREE.MeshBasicMaterial({ color: 0x2a3036 }), dark = new THREE.MeshBasicMaterial({ color: 0x15171a });
  const band = new THREE.MeshBasicMaterial({ color: 0xd8dccc }), skin = new THREE.MeshBasicMaterial({ color: 0x6e5a4c });
  const box = (w: number, h: number, d: number, m: THREE.Material, x: number, y: number, z: number) => { const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), m); b.position.set(x, y, z); body.add(b); };
  box(.2, .82, .2, dark, -.11, .41, 0); box(.2, .82, .2, dark, .11, .41, 0);
  box(.5, .62, .28, coat, 0, 1.14, 0); box(.52, .07, .3, band, 0, 1.2, 0);
  box(.12, .6, .14, coat, -.32, 1.12, 0); box(.12, .6, .14, coat, .32, 1.12, 0);
  const head = new THREE.Mesh(new THREE.SphereGeometry(.13, 10, 8), skin); head.position.y = 1.6; body.add(head);
  if (lying) { body.rotation.x = -Math.PI / 2; body.position.y = .14; }
  g.add(body);
  g.traverse((x) => { x.frustumCulled = false; });
  return g;
}
function grain(src: HTMLCanvasElement, seed: number, dark = 1, grey = 0.7, contrast = 1.3) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  g.filter = `grayscale(${grey}) contrast(${contrast}) brightness(${dark})`; g.drawImage(src, 0, 0, 640, 400); g.filter = 'none';
  let s = seed;
  const r = () => { s = (s * 1664525 + 1013904223) >>> 0; return s / 4294967296; };
  for (let i = 0; i < 9000; i++) { g.fillStyle = `rgba(255,255,255,${r() * 0.08})`; g.fillRect(r() * 640, r() * 400, 1, 1); }
  const v = g.createRadialGradient(320, 200, 90, 320, 200, 400); v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,.9)');
  g.fillStyle = v; g.fillRect(0, 0, 640, 400);
  for (let y = 0; y < 400; y += 3) { g.fillStyle = 'rgba(0,0,0,.18)'; g.fillRect(0, y, 640, 1); }
  return c;
}
/** One of the painted frames (4:3), cut to the frames' 16:10 (y: where the cut sits, 0 the top)
 *  and given the same grain and lines as the drawn ones, a little less grey; null if it did not come. */
function painted(id: ArtId, y: number, seed: number, dark = 1) {
  if (!artLoaded(id)) return null;
  const im = artImage(id), c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const sh = im.width / 1.6;
  c.getContext('2d')!.drawImage(im, 0, (im.height - sh) * y, im.width, sh, 0, 0, 640, 400);
  return grain(c, seed, dark, 0.45, 1.15);
}
function frameLight(seed: number) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  const l = g.createRadialGradient(320, 40, 10, 320, 160, 420); l.addColorStop(0, '#ffffff'); l.addColorStop(0.35, '#d8e0e4'); l.addColorStop(1, '#101214');
  g.fillStyle = l; g.fillRect(0, 0, 640, 400);
  return grain(c, seed, 1);
}
/** A hand with fingers too long at the right-hand edge, against the light: over the grass of the
 *  painted light frame, close (round 14), or over the light drawn here. */
function frameHand(seed: number, under: HTMLImageElement | null) {
  const c = document.createElement('canvas'); c.width = 640; c.height = 400;
  const g = c.getContext('2d')!;
  if (under) {
    // the lit grass beside the boots, nearer: the lower left of the picture
    g.drawImage(under, 0, under.height * 0.56, under.width * 0.44, under.width * 0.44 / 1.6, 0, 0, 640, 400);
    g.fillStyle = 'rgba(235,242,246,.18)'; g.fillRect(0, 0, 640, 400);
  } else {
    const l = g.createRadialGradient(260, 120, 20, 300, 200, 420); l.addColorStop(0, '#e8eef0'); l.addColorStop(1, '#202426');
    g.fillStyle = l; g.fillRect(0, 0, 640, 400);
  }
  // out of focus, too near the lens: the palm in from the edge, four long jointed fingers and a thumb
  g.fillStyle = '#060607'; g.filter = 'blur(2.5px)';
  g.beginPath(); g.ellipse(668, 252, 88, 64, -0.35, 0, 7); g.fill();
  const finger = (x: number, y: number, a: number, len: number, w: number, bend: number) => {
    // three joints, each a little narrower and bent a little more
    let px = x, py = y, ang = a, wd = w;
    g.beginPath();
    const left: [number, number][] = [], right: [number, number][] = [];
    for (const k of [0.42, 0.33, 0.25]) {
      const nx = px + Math.cos(ang) * len * k, ny = py + Math.sin(ang) * len * k, o = ang + Math.PI / 2;
      left.push([px + Math.cos(o) * wd, py + Math.sin(o) * wd]); right.push([px - Math.cos(o) * wd, py - Math.sin(o) * wd]);
      px = nx; py = ny; ang += bend; wd *= 0.78;
    }
    g.moveTo(...left[0]); for (const q of left.slice(1)) g.lineTo(...q);
    g.lineTo(px + Math.cos(ang) * wd * 1.2, py + Math.sin(ang) * wd * 1.2);
    for (const q of right.reverse()) g.lineTo(...q);
    g.closePath(); g.fill();
  };
  for (const [a, len, b] of [[-2.25, 250, -0.1], [-2.55, 300, -0.08], [-2.85, 310, -0.05], [-3.1, 270, 0.02]] as const) finger(612, 238, a, len, 10, b);
  finger(640, 300, -2.6, 150, 11, 0.18);
  g.filter = 'none';
  return grain(c, seed, 1, under ? 0.45 : 0.7, under ? 1.15 : 1.3);
}
