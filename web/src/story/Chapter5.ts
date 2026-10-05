import * as THREE from 'three';
import type { DocSpec, UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { Player } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { Chapter3, AreaId } from './Chapter3';
import * as P from '../ui/Panels';
import { mapOverlay } from '../ui/MapOverlay';
import { SERVICE_MAP, serviceMap } from './drawings';
import { clockText } from './time';

// Chapter five, "All Night" (KAPITLER.md). 05:00 to 05:20. Out of room 6 and back across
// the road to the truck; on the way past the control room the operations terminal chimes:
// a new file, opened at 05:29, half an hour from now. Then the diner where the old road to
// Roswell leaves the highway: coffee, a waitress who has watched the lights over the mesa
// since she was eleven, a driver whose engine died on the old road last October, the 1947
// clipping, the radio, and a payphone to Ward. At a booth the service map and E09A on
// tracing paper show where C crosses the old road (P13). The driver confirms it without
// knowing he does. The chapter ends at the truck; the Roswell road is not built yet.

export type Stage5 = 'to-truck' | 'diner' | 'road' | 'complete';

export interface Ch5State {
  v: 1;
  stage: Stage5;
  bell: boolean; file: boolean;       // the terminal chimed; the 05:29 file was read
  arrived: boolean;
  coffee: boolean; lights: boolean; clipped: boolean; aunt: boolean; radio: boolean;
  road: boolean; october: boolean; confirmed: boolean;
  ward: boolean;
  tracing: [number, number] | null;   // where the tracing lies on the map
  p13: boolean; wrong13: number;
}

/** The diner, as chapter five needs it (world/Diner.ts). */
export interface DinerLike {
  proxies: Record<string, THREE.Object3D>;
  objs: Record<string, THREE.Object3D>;
  setDawn(k: number): void;
  setSignLit(on: boolean): void;
}

export interface Chapter5Host {
  clock: number;
  gt: number;
  notes: string[];
  docs: DocSpec[];
  cinematic: boolean;
  note(s: string): void;
  after(s: number, fn: () => void, tag?: string): void;
}

export interface Chapter5Deps {
  ui: UI; audio: AudioSys; player: Player; inter: Interaction;
  ch3: Chapter3;
  area(): AreaId;
  diner(): DinerLike | null;
  dinerTruck(): THREE.Object3D | null;
  terminal(): THREE.Vector3;          // the operations terminal on the west desk
  driveToDiner(onArrive: () => void): void;
  save: () => void;
  milestone: () => void;
}

const fresh = (): Ch5State => ({
  v: 1, stage: 'to-truck', bell: false, file: false, arrived: false,
  coffee: false, lights: false, clipped: false, aunt: false, radio: false,
  road: false, october: false, confirmed: false, ward: false,
  tracing: null, p13: false, wrong13: 0,
});

const DAWN = 5 * 3600;   // 05:00, the chapter's start; civil dawn is 05:05, sunrise 05:30
const SUPPORTED = 'P13 SUPPORTED / With E09A traced and laid on STATION 01, north to north, C runs due east and meets the old Roswell road just past the eight-mile post, at a survey bolt in the shoulder. It fits N. Vega\'s account: east from the station, across the old highway, a bolt where it crosses.';

type Who = 'waitress' | 'driver' | 'ward';
type Line = P.TalkLine;
const aside = (text: string): Line => ({ who: '', text });
const you = (text: string): Line => ({ who: 'YOU', text });
const W = (text: string): Line => ({ who: 'THE WAITRESS', text });
const D = (text: string): Line => ({ who: 'THE DRIVER', text });
const E = (text: string): Line => ({ who: 'E. WARD', text });

export class Chapter5 {
  active = false;
  started = false;
  s: Ch5State = fresh();
  onEnd?: () => void;
  private lines: Line[] = [];
  private who: Who | null = null;
  private panel: { refresh: () => void } | null = null;
  private dinerBound = false;
  private clippingUrl: string | null = null;
  private mapUrl: string | null = null;
  private radioAt: THREE.Vector3 | null = null;

  constructor(private g: Chapter5Host, private d: Chapter5Deps) {}

  // ---------- lifecycle ----------
  begin(saved: Ch5State | null) {
    this.reset();
    this.active = true; this.started = true;
    this.s = saved ? { ...fresh(), ...saved } : fresh();
    // the truck on its pad takes chapter five's road (it belongs to chapter three's code)
    this.d.ch3.truckOverride = {
      label: () => this.active && this.s.stage === 'to-truck' ? 'Drive to Mesa Diner' : null,
      use: () => this.useSaroTruck(),
    };
    this.applyWorld();
    if (!saved) {
      this.g.note(`${this.time()}. Out of room 6. The truck is on its pad at SARO. C crosses the old road to Roswell; the road leaves the highway at the diner south of here, the one that is open all night.`);
      this.d.ui.toast('The truck is on its pad at SARO.', 3.4);
    }
    this.d.save();
  }

  reset() {
    this.active = false; this.started = false;
    this.s = fresh(); this.lines = []; this.who = null; this.panel = null;
    this.d.ch3.truckOverride = null;
    this.d.audio.amRadio(null);
    this.applyWorld();
  }

  saveBlock(): string | null { return null; }

  private save() { this.d.save(); }
  private time() { return clockText(this.g.clock, false); }
  private inDiner() { return this.d.area() === 'diner'; }
  private file(doc: DocSpec) {
    const i = this.g.docs.findIndex((x) => x.id === doc.id);
    if (i >= 0) { this.g.docs[i] = doc; return false; }
    this.g.docs.push(doc);
    return true;
  }

  // The diner as the state has it (after begin, and when the diner has loaded).
  private applyWorld() {
    const diner = this.d.diner();
    if (diner) {
      const cup = diner.objs.cup;
      // the cup is on the counter once she has poured, and stays there when you go
      if (cup) cup.visible = !this.active || this.s.coffee;
      diner.setSignLit(true);
    }
    if (!this.active) return;
    if (this.s.arrived) this.file(this.docMap());
    if (this.s.clipped) this.file(this.docClipping());
    if (this.s.p13) this.file(this.docFinding());
  }

  // ---------- objective and journal ----------
  objective(): string {
    const s = this.s;
    switch (s.stage) {
      case 'to-truck': return 'ALL NIGHT // THE TRUCK IS ON ITS PAD AT SARO';
      case 'diner':
        if (!this.inDiner()) return 'ALL NIGHT // MESA DINER';
        return 'MESA DINER // WHERE DOES C CROSS THE OLD ROAD? SPREAD THE MAPS ON A TABLE (P13)';
      case 'road': return 'MESA DINER // C CROSSES JUST PAST THE EIGHT-MILE POST. THE TRUCK IS OUTSIDE';
      case 'complete': return 'ROSWELL ROAD // NEXT PART NOT YET PLAYABLE';
    }
  }

  tasks() {
    const s = this.s;
    const t = [{ text: 'Take the truck to the diner', done: s.arrived }];
    if (s.bell || s.file) t.push({ text: 'The new file on the operations terminal (optional)', done: s.file });
    if (s.arrived) t.push(
      { text: 'Coffee (optional)', done: s.coffee },
      { text: 'Ask the waitress about the lights over the mesa', done: s.lights },
      { text: 'The clipping on the wall', done: s.clipped },
      { text: 'Find where C crosses the old Roswell road (P13)', done: s.p13 },
      { text: 'Ask the driver about the old road', done: s.october },
      { text: 'Call Ward from the payphone (optional)', done: s.ward },
      { text: 'Drive the old road', done: s.stage === 'complete' },
    );
    return t;
  }

  // ---------- SARO: the terminal and the truck ----------
  /** The operations terminal ran a command (story/Prologue.ts openOps). */
  terminalCommand(cmd: string) {
    if (!this.active || this.s.file || !/TYPE RUN860414_0529/.test(cmd)) return;
    this.s.file = true;
    this.g.note(`${this.time()}. A new file in SURVEY.RAW on the operations terminal: RUN860414_0529.DAT. 0 blocks, operator Reyes, opened at 05:29. It is ${this.time()} now. "File incomplete. Record not closed."`);
    this.save();
  }

  private useSaroTruck() {
    const { ui } = this.d;
    if (this.s.stage !== 'to-truck') { ui.toast('SARO 07. It is parked at the diner tonight.', 3); return; }
    this.d.driveToDiner(() => this.arrivedDiner());
  }

  /** Called when the drive cut ends at the diner (and when a save there is restored). */
  arrivedDiner() {
    const s = this.s;
    if (!s.arrived) {
      s.arrived = true;
      if (s.stage === 'to-truck') s.stage = 'diner';
      this.file(this.docMap());
      this.g.note(`${this.time()}. Mesa Diner, where the old road to Roswell leaves the highway. Open all night. Grey light in the east. The service map from the truck's door pocket came along (E14).`);
      this.d.ui.toast('Mesa Diner. The service map from the door pocket is in your jacket.', 4);
      this.d.milestone();
    }
    this.applyWorld();
    this.save();
  }

  // ---------- the diner's things ----------
  // Every hit area in the diner has a label whenever the player is there. Before or after
  // the chapter they are just a diner; while it runs they carry it.
  bindDiner(diner: DinerLike) {
    if (!this.dinerBound) {
      this.dinerBound = true;
      const { inter, ui } = this.d;
      const px = diner.proxies;
      const here = () => this.inDiner();
      const add = (id: string, label: () => string | null, use: () => void, range = 2.4) => {
        if (px[id]) inter.add({ id: 'diner:' + id, object: px[id], range, label: () => here() ? label() : null, use });
      };
      const on = () => this.active && (this.s.stage === 'diner' || this.s.stage === 'road');
      add('waitress', () => on() ? 'Talk to the waitress' : 'The waitress', () => on() ? this.openTalk('waitress') : ui.toast('She is wiping the same stretch of counter.', 2.8), 2.8);
      add('driver', () => on() ? 'Talk to the driver' : 'A driver at the counter', () => on() ? this.openTalk('driver') : ui.toast('He is eating eggs and not looking up.', 2.8), 2.6);
      add('coffee', () => on() && !this.s.coffee ? 'Coffee' : 'The coffee pot', () => this.useCoffee());
      add('counter', () => 'The radio behind the counter', () => this.useRadio());
      add('clipping', () => 'The clipping by the door', () => this.readClipping());
      add('payphone', () => on() && !this.s.ward ? 'Payphone' : 'Payphone', () => this.usePayphone());
      add('booth', () => on() ? (this.s.p13 ? 'The maps on the table' : 'Spread the maps on the table') : 'A booth by the window', () => on() ? this.openMap() : ui.toast('A booth by the window. The vinyl is cold.', 2.6));
      add('menu', () => 'Menu board', () => ui.toast('Coffee .35. Two eggs any style 1.95. Green chile stew 2.95. Pie .95, under a glass dome on the counter.', 4.2));
      add('jukebox', () => 'Jukebox', () => ui.toast('A card in the glass, in ballpoint: OUT OF ORDER. It has been there a while.', 3.2));
      add('window', () => 'Window', () => ui.toast(this.active && this.s.arrived ? 'Grey behind the mesa to the east. The truck on the lot, and the old road going off past it.' : 'The lot, the highway, the dark.', 3.6));
      add('door', () => 'Door', () => ui.toast('The door sticks in the cold and lets it in.', 2.6));
      add('sign', () => 'Sign', () => ui.toast('MESA DINER / OPEN ALL NIGHT. One letter of the neon buzzes.', 3));
      add('rig', () => 'Rig', () => ui.toast('A tractor and a refrigerated trailer, the unit running. The driver\'s.', 3), 4.5);
    }
    this.applyWorld();
  }
  /** The truck on the diner's lot (world/World.ts gives it a hit area). */
  bindDinerTruck(object: THREE.Object3D) {
    const { inter, ui } = this.d;
    inter.add({ id: 'dinerTruck', object, range: 3.2,
      label: () => !this.inDiner() ? null : this.active && this.s.stage === 'road' ? 'Drive the old road' : 'SARO 07',
      use: () => {
        if (!this.active) { ui.toast('SARO 07, the service truck.', 2.6); return; }
        if (this.s.stage === 'road') { this.finish(); return; }
        ui.toast(this.s.stage === 'complete' ? 'Out on the old road.' : 'Not yet. Where C crosses the road is on the maps. A booth table would do.', 3.4);
      } });
  }

  private useCoffee() {
    const { ui, audio } = this.d;
    if (!this.active || !this.s.arrived) { ui.toast('Fresh, or near enough.', 2.4); return; }
    if (this.s.coffee) { ui.toast('Hot, and strong enough to stand a spoon in.', 2.6); return; }
    this.pour();
    ui.toast('She pours without asking how you take it. "Thirty-five cents. Pay when you go."', 4);
    audio.play('thudSoft', { gain: 0.12, rate: 2.2 });
  }
  private pour() {
    this.s.coffee = true;
    const cup = this.d.diner()?.objs.cup; if (cup) cup.visible = true;
    this.g.note(`${this.time()}. Coffee at the counter. Thirty-five cents.`);
    this.save();
  }

  // The radio is on low behind the counter (core/Audio.ts amRadio); turned up, the early
  // news is the real news of the morning of 14 April 1986.
  private radioBusy = false;
  private useRadio() {
    const { ui } = this.d;
    if (this.radioBusy) return;
    this.radioBusy = true;
    const parts = [
      'RADIO: "...clearing after last night\'s dry storms west of the Magdalenas. Cold this morning, high around seventy."',
      'RADIO: "Jack Nicklaus won the Masters yesterday. Forty-six years old, thirty on the back nine. Nobody saw it coming."',
      'RADIO: "And if you are up at this hour, Halley\'s comet is low in the south around midnight this week. Get away from town lights."',
      `RADIO: "It is ${this.time().replace(/^0/, '')}."`,
    ];
    parts.forEach((p, i) => this.g.after(i * 4.4, () => ui.toast(p, 4.2)));
    this.g.after(parts.length * 4.4, () => { this.radioBusy = false; });
    if (this.active && !this.s.radio) {
      this.s.radio = true;
      this.g.note(`${this.time()}. The radio behind the counter: the weather after last night's dry storms, Nicklaus won the Masters at forty-six, Halley's comet low in the south around midnight this week.`);
      this.save();
    }
  }

  private readClipping() {
    const doc = this.docClipping();
    if (this.active && !this.s.clipped) {
      this.s.clipped = true;
      this.file(doc);
      this.g.note(`${this.time()}. The clipping by the diner door (E15): the Pecos Valley Sentinel, July 1947. "LIGHT HELD OVER MESA FOR AN HOUR." The mesa, a steady light high over it, two ranchers at a fence. Low to the right of the mesa, a smudge with a short tail.`);
      this.save();
    }
    this.d.ui.document(doc, () => {});
  }

  private usePayphone() {
    const { ui } = this.d;
    if (!this.active || !this.s.arrived) { ui.toast('A payphone in a niche. A dime for a local call.', 2.8); return; }
    if (this.s.ward) { ui.toast('She said what she had to say. Six o\'clock.', 2.8); return; }
    this.openTalk('ward');
  }

  // ---------- conversations ----------
  private openTalk(who: Who) {
    const s = this.s;
    this.who = who;
    if (who === 'waitress') this.lines = [W(s.coffee ? 'More? No? Suit yourself.' : 'Morning. Or whatever this is.'), ...(s.coffee ? [] : [W('Coffee?')])];
    if (who === 'driver') this.lines = [D('Morning.'), aside('He does not look up from his eggs.')];
    if (who === 'ward') {
      this.lines = [
        aside('A dime in the slot, and her number from the card in your wallet.'),
        E('Ward.'),
        you('It is Reyes. I am at the diner on the old road.'),
        E('I read the telex. All of it.'),
        E('Hondo called. So did Site 11.'),
      ];
      this.d.audio.play('click', { gain: 0.4 });
    }
    this.panel = P.talk(this.d.ui, {
      title: who === 'ward' ? 'PAYPHONE / E. WARD' : who === 'waitress' ? 'MESA DINER / THE WAITRESS' : 'MESA DINER / THE DRIVER',
      view: () => this.view(),
      onChip: () => {}, onTable: () => {},
      onChoice: (id) => this.choose(id),
      onRead: () => {},
      onClose: () => { this.panel = null; this.who = null; },
    });
  }

  private view(): P.TalkView {
    const s = this.s;
    const v: P.TalkView = { status: this.objective(), lines: this.lines, choices: [], reply: null };
    if (this.who === 'waitress') {
      if (!s.coffee) v.choices.push({ id: 'coffee', label: 'Coffee, please' });
      if (!s.lights) v.choices.push({ id: 'lights', label: 'Ask about the lights over the mesa' });
      if (!s.aunt) v.choices.push({ id: 'clipping', label: 'Ask about the clipping by the door' });
      v.choices.push({ id: 'bye', label: 'Thank her' });
    }
    if (this.who === 'driver') {
      if (!s.road) v.choices.push({ id: 'road', label: 'Ask about the old road' });
      if (!s.october) v.choices.push({ id: 'october', label: 'Ask if anything ever happens out there' });
      if (s.p13 && !s.confirmed) v.choices.push({ id: 'bolt', label: 'Ask about the survey bolt past the eight-mile post' });
      v.choices.push({ id: 'bye', label: 'Let him eat' });
    }
    if (this.who === 'ward' && !s.ward) {
      v.choices.push({ id: 'what', label: 'Ask what they said' });
      v.choices.push({ id: 'road', label: 'Tell her you are checking the old road first' });
    }
    if (this.who === 'ward' && s.ward) v.choices.push({ id: 'bye', label: 'Hang up' });
    return v;
  }

  private choose(id: string) {
    const s = this.s;
    if (id === 'bye') { this.d.ui.close(); return; }
    if (this.who === 'waitress') {
      if (id === 'coffee') { this.lines = [you('Coffee, please.'), aside('She pours without asking how you take it.'), W('Thirty-five cents. Pay when you go.')]; this.pour(); }
      if (id === 'lights') {
        s.lights = true;
        this.lines = [
          you('The lights over the mesa. You have seen them?'),
          W('You are from the observatory. You people never ask about anything else.'),
          W('First time I was eleven. Over the mesa, steady, no sound. An hour, maybe.'),
          W('My daddy said it was the Army. The Army said it was weather.'),
          W('Every few years since. Last October was the last time.'),
          W('And before you ask: no, I do not think it is little green men. I think it is the mesa.'),
        ];
        this.g.note(`${this.time()}. The waitress has seen the light over the mesa since she was eleven: steady, no sound, an hour. Every few years. Last October was the last time. "I do not think it is little green men. I think it is the mesa."`);
        this.d.milestone();
      }
      if (id === 'clipping') {
        s.aunt = true;
        this.lines = [
          you('The clipping by the door.'),
          W('My aunt cut that out of the Sentinel. The Kesslers had the ranch on the old road then.'),
          W('The paper ran it bigger. She trimmed it to fit the frame.'),
        ];
        this.g.note('The waitress: her aunt cut the clipping out of the Sentinel. The paper ran the picture bigger; it was trimmed to fit the frame.');
      }
    }
    if (this.who === 'driver') {
      if (id === 'road') {
        s.road = true;
        this.lines = [
          you('You take the old road to Roswell?'),
          D('Nobody takes it since they put the new one through. I take it. Saves me eleven miles.'),
          D('Nothing out there but fence and the mesa.'),
        ];
      }
      if (id === 'october') {
        s.october = true;
        this.lines = [
          you('Anything ever happen out there?'),
          D('Last October she quit on me out there. Right past the eight-mile post.'),
          D('Lights, radio, engine, all at once. Coasted to a stop. Then it all came back by itself.'),
          D('I figured battery. Put a new one in the next day. Never did it again.'),
        ];
        this.g.note(`${this.time()}. The driver: last October his rig died on the old road, right past the eight-mile post. Lights, radio and engine at once, then all of it came back. He put in a new battery. It has not happened since.`);
        this.d.milestone();
      }
      if (id === 'bolt') {
        s.confirmed = true;
        this.lines = [
          you('There is a survey bolt in the shoulder just past the eight-mile post.'),
          D('The brass thing? Sure.'),
          D('That is right where she quit on me. I remember, because I sat there looking at it.'),
        ];
        this.g.note(`${this.time()}. The driver knows the bolt past the eight-mile post. That is where his rig died last October.`);
        this.d.milestone();
      }
    }
    if (this.who === 'ward') {
      if (id === 'what') this.lines = [...this.lines.slice(-2), you('What did they say?'), E('Enough. I am not doing this on a pay phone.')];
      if (id === 'road') {
        s.ward = true;
        this.lines = [
          you('I am checking the old road first.'),
          E('Then check it, and come back. I want you on the console before the morning series.'),
          E('Six o\'clock, Reyes.'),
          aside('She hangs up.'),
        ];
        this.g.note(`${this.time()}. Ward, from the payphone: "Hondo called. So did Site 11." She wants me back on the console before the morning series at six.`);
        this.d.audio.play('click', { gain: 0.4 });
        this.d.milestone();
      }
    }
    this.save();
  }

  // ---------- P13: the maps on the booth table ----------
  private openMap() {
    const s = this.s;
    if (!s.tracing) s.tracing = [300, 690];
    mapOverlay(this.d.ui, {
      at: s.tracing,
      solved: s.p13 ? SUPPORTED : null,
      question: 'P13 / Where does C cross the old Roswell road?',
      choices: [
        { id: 'bm8', label: 'At the survey bolt just past the eight-mile post' },
        { id: 'bm6', label: 'At the bolt by the six-mile post' },
        { id: 'bm10', label: 'At the bolt past the ten-mile post' },
        { id: 'none', label: 'Nowhere. C runs clear of the road' },
      ],
      onMove: (x, y) => { s.tracing = [x, y]; this.save(); },
      onChoose: (id) => this.answer(id),
    });
  }
  private aligned() {
    const t = this.s.tracing;
    return !!t && Math.hypot(t[0] - SERVICE_MAP.station[0], t[1] - SERVICE_MAP.station[1]) < 0.5;
  }
  answer(id: string): { ok: boolean; text: string } {
    const s = this.s;
    if (id !== 'bm8') {
      s.wrong13++;
      this.save();
      if (!this.aligned()) return { ok: false, text: 'Lay the tracing on STATION 01 first, transit on the station, north to north. Then follow C.' };
      return { ok: false, text: id === 'none' ? 'Follow C east from the station. It does meet the road.' : id === 'bm6' ? 'That bolt is south of the line. C meets the road further on.' : 'C meets the road before that.' };
    }
    s.p13 = true;
    if (s.stage === 'diner') s.stage = 'road';
    this.file(this.docFinding());
    this.g.note(`${this.time()}. P13: with E09A on STATION 01, C runs due east and crosses the old Roswell road just past the eight-mile post, at a survey bolt in the shoulder. The truck is outside.`);
    this.d.audio.play('click', { gain: 0.5 });
    this.d.milestone();
    this.save();
    return { ok: true, text: SUPPORTED };
  }

  // ---------- out ----------
  private finish() {
    const s = this.s;
    if (s.stage === 'complete') return;
    s.stage = 'complete';
    this.g.note(`${this.time()}. Out to the truck. ${s.coffee ? 'The coffee stays on the counter, half drunk.' : 'The waitress is wiping the counter.'} The old road goes off south-east in the grey.`);
    this.d.milestone();
    this.g.after(1.0, () => this.onEnd?.());
  }

  endingLines() {
    return [
      'ALL NIGHT',
      'MESA DINER // P13 RECORDED',
      'The waitress has watched the light over the mesa since she was eleven. The driver\'s rig died on the old road last October, right past the eight-mile post. The clipping by the door is the same picture as in 1947, with more of it.',
      'C crosses the old road to Roswell just past the eight-mile post, at a survey bolt in the shoulder. The coffee stays on the counter.',
      'NEXT: ROSWELL ROAD. This part of the night is still being built.',
    ];
  }

  // ---------- documents ----------
  private docMap(): DocSpec {
    if (!this.mapUrl) this.mapUrl = serviceMap();
    return { id: 'e14', title: 'E14 / SARO 07 service map', kind: 'map', stamp: 'E14', page: '', image: this.mapUrl,
      transcript: 'From the door pocket of the service truck. The old Roswell road leaves the highway at the diner, and its mileposts count from there. Survey bolts in the shoulder are marked BM.\n\nC is not on it. E09A gives C as a direction from the transit, not a place.' };
  }
  private docClipping(): DocSpec {
    if (!this.clippingUrl) {
      // the clipping as it hangs on the wall (world/Diner.ts draws it on a canvas)
      let c: HTMLCanvasElement | null = null;
      this.d.diner()?.objs.clipping?.traverse((o) => {
        const m = (o as THREE.Mesh).material as THREE.MeshStandardMaterial | undefined;
        const img = m?.map?.image as HTMLCanvasElement | undefined;
        if (img instanceof HTMLCanvasElement && img.height >= 1000) c = img;
      });
      if (c) this.clippingUrl = (c as HTMLCanvasElement).toDataURL('image/jpeg', 0.85);
    }
    return { id: 'e15', title: 'E15 / The Sentinel clipping', kind: 'photo', page: '', image: this.clippingUrl ?? undefined, stamp: 'E15',
      transcript: 'The Pecos Valley Sentinel, July 1947, cut out and framed by the diner door. LIGHT HELD OVER MESA FOR AN HOUR. Ranchers on the old survey road watched a steady glow; the Army field office cites weather equipment.\n\nThe photograph: a flat mesa, a steady light high over it, two ranchers at a fence. Low to the right of the mesa, a smudge with a short tail. The right-hand edge of the picture is trimmed close.' };
  }
  private docFinding(): DocSpec {
    return { id: 'p13', title: 'P13 / Where C crosses', kind: 'typed', stamp: 'P13', page: SUPPORTED,
      transcript: 'The service map from the truck with E09A traced over it. C crosses the old road just past the eight-mile post.' };
  }

  // ---------- per frame ----------
  update(_dt: number, _t: number) {
    if (!this.active) return;
    const s = this.s;
    // the terminal chimes once, the first time the player comes past the control room
    if (s.stage === 'to-truck' && !s.bell && this.d.area() === 'saro' && this.d.player.pos.distanceTo(this.d.terminal()) < 22) {
      s.bell = true;
      this.d.audio.beep(880, 0.12, 0.08); this.d.audio.beep(880, 0.12, 0.08, 0.22);
      this.d.ui.toast('Inside, the operations terminal chimes once.', 3);
      this.save();
    }
    // the grey comes up in the east while the player is in the diner
    const diner = this.d.diner();
    if (diner && this.inDiner()) {
      diner.setDawn(THREE.MathUtils.clamp((this.g.clock % 86400 - DAWN) / (25 * 60), 0.15, 0.85));
      const counter = diner.proxies.counter;
      if (counter && !this.radioAt) { this.radioAt = counter.getWorldPosition(new THREE.Vector3()); this.d.audio.amRadio(this.radioAt); }
    } else if (this.radioAt) { this.radioAt = null; this.d.audio.amRadio(null); }
    this.d.ui.objective(this.objective());
  }
}
