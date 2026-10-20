import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Voice, ArchiveProgress } from '../core/radio';
import { clockText } from './time';

// More that can happen in the night, anywhere (Tom, 18 October: "we need more events and things
// that can happen in the game; more alive, more frightening"). The town has its own director
// (story/townEvents.ts); this one is for everywhere else. One thing at a time, a few minutes
// apart, at night only, and each in the notebook in the player's own words. Nothing chases and
// nothing answers (HENDELSER.md, the rules). The night's curve (story/curve.ts) says when: the
// small ones in the second half of its build, the 47-second ones (the bulletin, the silence) as a
// peak, one extra in a chapter, and none in the quiet after a peak.
//
//   steps     on foot outside: the player stops, and two or three steps go on behind them, on
//             the same ground, and stop. Up to three a night.
//   knocks    driving at night: three slow knocks on the cab roof, from above. Once.
//   radio     walking back to the parked truck: its radio is on, on 1090, though it was left
//             off. Once.
//   bulletin  driving with the radio on 1420: a news bulletin from July 1947 about a flying disc
//             on a ranch near Roswell, in the newsreader's voice, and then the Night Line again
//             as if nothing had been (HENDELSER.md, event 4). Once.
//   phone     outside near the SARO building: the control room's phone rings once, and stops.
//   door      outside at SARO, well away: the service door, shut, is found open. Once.
//   silence   on foot outside anywhere: the crickets, the wind and the footsteps stop for 47 s
//             (HENDELSER.md, event 3; core/roadside.ts does it, and on the walk to the motel too).

export type NightEventId = 'steps' | 'knocks' | 'radio' | 'bulletin' | 'phone' | 'door' | 'silence';
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export const BULLETIN: { text: string; who: Voice; by: string }[] = [
  { text: 'We interrupt this program. This is a local news bulletin.', who: 'news', by: 'a bulletin' },
  { text: 'Roswell, New Mexico. Tuesday, July eighth, nineteen forty-seven.', who: 'news', by: 'a bulletin' },
  { text: 'A flying disc is reported recovered on a ranch outside town. That is the wording on the notice.', who: 'news', by: 'a bulletin' },
  { text: 'There is no description of the object. The space for the name of the person reporting it is blank.', who: 'news', by: 'a bulletin' },
  { text: 'We have received the same line twice. We are reading it just as it came through.', who: 'news', by: 'a bulletin' },
  { text: 'Please stand by. We will return to our regular program.', who: 'news', by: 'a bulletin' },
];

export interface NightIn {
  /** At night, in play, nothing else holding the player (main.ts). */
  ok: boolean;
  clock: number;
  cam: THREE.Camera;
  /** On foot: where, whether outside, and whether walking just now. */
  foot: { pos: THREE.Vector3; outside: boolean; moving: boolean } | null;
  /** Driving at night (any road). */
  driving: boolean;
  /** The truck in this area while on foot, and whether its radio is on. */
  truck: { pos: THREE.Vector3; radioOn: boolean } | null;
  /** Driving with the truck's radio on 1420 and on the air. */
  radio1420: boolean;
  /** In SARO's area, outside, and how far from the building's phone and the service door. */
  saro: { phone: THREE.Vector3; door: THREE.Vector3; doorOpen: boolean } | null;
  /** The silence of core/roadside.ts can come now (not had in this case). */
  silenceFree: boolean;
  /** The night's curve (story/curve.ts) lets one start now: the events, and the 47-second ones
   *  (the bulletin, the silence: one extra in a chapter). Left out, they may. */
  may?: boolean;
  mayLong?: boolean;
  /** How much sooner the next comes than its own pace (the curve's build). */
  pace?: number;
}

export interface NightDeps {
  audio: AudioSys;
  note: (text: string) => void;
  seen: Set<string>;
  /** The kind of ground under the player, for the steps behind. */
  step: () => { name: string; gain: number };
  setRadio: (station: 'far') => void;
  bulletin: () => number;
  /** Still in the cab, tuned to this recording, and able to hear it. */
  bulletinHeard?: () => boolean;
  /** Actual station episode progress. Optional for legacy dt-only director fixtures. */
  bulletinProgress?: () => ArchiveProgress | null;
  openDoor: () => void;
  silence: () => void;
  place: () => string;
  /** The 47 s of silence are on (core/roadside.ts). */
  silent: () => boolean;
}

export class NightEvents {
  /** The last one, for the developer menu and the tests. */
  last: { id: NightEventId; clock: number } | null = null;
  private wait = rnd(90, 160);
  private walked = 0;
  private still = 0;
  private stepsLeft: number[] = [];
  private stepAt = new THREE.Vector3();
  private pending: { id: NightEventId; t: number; check?: () => boolean; heard?: boolean; archiveId?: number } | null = null;
  private radioNear = 1e9;

  constructor(private d: NightDeps) {}
  /** Something of this director's is going on (the town's director waits for it). */
  get busy() { return !!this.pending || this.stepsLeft.length > 0 || this.d.silent(); }
  /** Something is playing out now (the steps, the knocks, the phone, the bulletin, the silence);
   *  not the radio left on or the door left open, which wait to be come back to (story/curve.ts:
   *  one at a time). */
  get running() { return (!!this.pending && this.pending.id !== 'radio' && this.pending.id !== 'door') || this.stepsLeft.length > 0 || this.d.silent(); }

  private count(id: string) { let n = 0; for (const k of this.d.seen) if (k.startsWith(`ne:${id}`)) n++; return n; }
  private done(id: NightEventId, clock: number) {
    this.d.seen.add(`ne:${id}${this.count(id) + 1}`);
    this.last = { id, clock };
    this.wait = rnd(130, 260);
  }
  private can(id: NightEventId, s: NightIn): boolean {
    const f = s.foot;
    switch (id) {
      case 'steps': return !!f && f.outside && this.count('steps') < 3 && this.still > 0.25 && this.still < 1 && this.walked > 8;
      case 'knocks': return s.driving && this.count('knocks') < 1;
      case 'radio': return !!f && !!s.truck && !s.truck.radioOn && this.count('radio') < 1 && this.radioNear > 14 && this.radioNear < 40;
      case 'bulletin': return s.radio1420 && this.count('bulletin') < 1;
      case 'phone': return !!f && f.outside && !!s.saro && f.pos.distanceTo(s.saro.phone) < 30 && this.count('phone') < 2;
      case 'door': return !!f && f.outside && !!s.saro && !s.saro.doorOpen && f.pos.distanceTo(s.saro.door) > 25 && !this.inView(s.cam, s.saro.door) && this.count('door') < 1;
      case 'silence': return !!f && f.outside && s.silenceFree && this.count('silence') < 1;
    }
  }
  private inView(cam: THREE.Camera, at: THREE.Vector3) {
    const n = at.clone().project(cam);
    return n.z < 1 && Math.abs(n.x) < 1.05 && Math.abs(n.y) < 1.05;
  }

  update(dt: number, s: NightIn) {
    const f = s.foot;
    if (f && f.moving) { this.walked += dt; this.still = 0; } else { this.still += dt; if (this.still > 1.5) this.walked = 0; }
    if (f && s.truck) this.radioNear = f.pos.distanceTo(s.truck.pos);
    this.stepsBehind(dt, s);
    // something started that ends with a note: when it has been seen or heard through
    if (this.pending) {
      const p = this.pending;
      if (p.id === 'bulletin' && this.d.bulletinProgress) {
        const progress = this.d.bulletinProgress();
        if (!progress || progress.id !== p.archiveId || progress.state === 'cancelled') {
          this.pending = null; this.wait = rnd(130, 260); return;
        }
        // A menu pause suspends both clocks; it does not make an otherwise heard recording
        // incomplete. Story/receiver interruptions still invalidate the whole finding.
        if ((!s.ok && this.d.audio.ctx?.state !== 'suspended') || this.d.bulletinHeard?.() === false) p.heard = false;
        p.t = Math.max(0, progress.duration - progress.elapsed);
        if (progress.state === 'complete' && p.t <= 0) {
          this.pending = null;
          if (p.heard !== false) this.finish(p.id, s);
          else this.wait = rnd(130, 260);
        }
        return;
      }
      if (p.id === 'bulletin' && (!s.ok || this.d.bulletinHeard?.() === false)) p.heard = false;
      p.t -= dt;
      if (p.t <= 0 && (!p.check || p.check())) {
        this.pending = null;
        if (p.heard !== false) this.finish(p.id, s);
        else this.wait = rnd(130, 260);
      }
      return;
    }
    if (!s.ok || (s.may === false && s.mayLong === false)) return;
    this.wait -= dt * (s.pace ?? 1);
    if (this.wait > 0) return;
    const all: NightEventId[] = ['steps', 'knocks', 'radio', 'bulletin', 'phone', 'door', 'silence'];
    // (the night's curve: the 47-second ones are a peak, the rest come in its build)
    const can = all.filter((id) => this.can(id, s) && ((id === 'bulletin' || id === 'silence') ? s.mayLong !== false : s.may !== false));
    if (!can.length) return;
    this.start(can[Math.floor(Math.random() * can.length)], s);
  }

  /** One now if it can be (the developer menu, the tests). */
  now(id: NightEventId, s: NightIn) {
    if (this.busy) return false;
    if (!this.can(id, { ...s }) && id !== 'steps') return false;
    return this.start(id, s);
  }

  private start(id: NightEventId, s: NightIn): boolean {
    const a = this.d.audio, cam = s.cam;
    switch (id) {
      case 'steps': {
        // behind, where the player came from: on the same ground, slower than theirs
        const back = new THREE.Vector3(0, 0, 1).applyQuaternion(cam.quaternion).setY(0).normalize();
        this.stepAt.copy(cam.position).addScaledVector(back, rnd(3.5, 5.5)).setY(cam.position.y - 1.5);
        const n = Math.random() < 0.5 ? 2 : 3;
        this.stepsLeft = Array.from({ length: n }, (_, i) => 0.35 + i * rnd(0.55, 0.7));
        this.pending = { id, t: this.stepsLeft[n - 1] + 1.5 };
        break;
      }
      case 'knocks': {
        [0, 0.95, 1.9].forEach((w) => a.play('thudSoft', { gain: 1.1, rate: 0.62, lp: 900, when: w }));
        this.pending = { id, t: 3.5 };
        break;
      }
      case 'radio':
        this.d.setRadio('far');
        // written down once the player is by the truck and has heard it
        this.pending = { id, t: 1, check: () => this.radioNear < 7 };
        break;
      case 'bulletin': {
        const len = this.d.bulletin();
        if (len <= 0) return false;
        this.pending = { id, t: len, heard: true, archiveId: this.d.bulletinProgress?.()?.id };
        break;
      }
      case 'phone':
        if (s.saro) {
          const h = a.play('phoneRing', { gain: 1.0, at: s.saro.phone });
          if (h && a.ctx) { const t = a.ctx.currentTime; h.gain.gain.setValueAtTime(1, t + 1.7); h.gain.gain.linearRampToValueAtTime(0, t + 1.85); h.src.stop(t + 1.9); }
        }
        this.pending = { id, t: 2.5 };
        break;
      case 'door':
        this.d.openDoor();
        this.doorAt.copy(s.saro!.door);
        this.pending = { id, t: 1, check: () => this.doorSeen };
        break;
      case 'silence':
        this.d.silence();
        this.pending = { id, t: 0 };
        break;
    }
    this.last = { id, clock: s.clock };
    return true;
  }
  private doorAt = new THREE.Vector3();
  /** Where the steps behind the player were, the last time (story/seen.ts). */
  get stepsAt() { return this.stepAt; }
  private doorSeen = false;

  private stepsBehind(dt: number, s: NightIn) {
    // the door: noticed once the player is back by it and sees it open
    if (this.pending?.id === 'door' && s.foot && s.saro) this.doorSeen = s.saro.doorOpen && s.foot.pos.distanceTo(this.doorAt) < 10 && this.inView(s.cam, this.doorAt);
    if (!this.stepsLeft.length) return;
    // walking again: whatever it was stops with the player
    if (s.foot?.moving) { this.stepsLeft = []; return; }
    for (let i = 0; i < this.stepsLeft.length; i++) this.stepsLeft[i] -= dt;
    while (this.stepsLeft.length && this.stepsLeft[0] <= 0) {
      this.stepsLeft.shift();
      const st = this.d.step();
      this.d.audio.play(st.name, { gain: st.gain * 1.2, rate: 0.86 + Math.random() * 0.06, at: this.stepAt, lp: 3200 });
    }
  }

  private finish(id: NightEventId, s: NightIn) {
    const t = clockText(s.clock, false), place = this.d.place();
    const first = this.count(id) === 0;
    this.done(id, s.clock);
    switch (id) {
      case 'steps':
        this.d.note(first ? `${t}. ${place}. I stopped, and the footsteps behind me went on two or three more and stopped. Nobody there.`
          : `${t}. ${place}. The steps behind me again when I stopped.`);
        break;
      case 'knocks': this.d.note(`${t}. Driving. Three knocks on the roof of the cab, slow, from above. Nothing on the road behind.`); break;
      case 'radio': this.d.note(`${t}. The truck's radio was on when I came back to it, on 1090. I had left it off.`); break;
      case 'bulletin': this.d.note(`${t}. AM 1420, a news bulletin: a flying disc found on a ranch near Roswell, the eighth of July. 1947. Then the Night Line again, as if nothing.`); break;
      case 'phone': this.d.note(`${t}. ${place}. The phone in the control room rang once, and stopped. Nobody else is in the building.`); break;
      case 'door': this.d.note(`${t}. The service door stood open. I had shut it.`); break;
      case 'silence': break;   // core/roadside.ts writes it down when the 47 s are over
    }
  }

  /** A new night, the title, a save opened. */
  reset() { this.wait = rnd(90, 160); this.pending = null; this.stepsLeft = []; this.walked = 0; this.still = 0; this.doorSeen = false; this.radioNear = 1e9; }
}
