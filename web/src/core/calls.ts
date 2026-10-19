import * as THREE from 'three';
import type { AudioSys } from './Audio';

// Out in the dark (Tom, 16 October: alien sounds now and then when the player walks about with
// the flashlight). The light shows where you are. On foot, outside, at night, with the
// flashlight on, something sounds out in the dark now and then: first the crickets and the wind
// go quiet, the flashlight stutters, and then a call, far off (35 to 110 m) and never in front
// where the beam goes, but behind or off to one side. Nothing comes closer and nothing answers
// (R5, HENDELSER.md). The more the light is used, the sooner the next one comes.
//
// The sounds are Freesound originals from the credits of They Are Here, CC0 and CC BY
// (THIRD_PARTY_NOTICES.md): a voice with clicks in it (resourcemoon), something on a radio
// (superhalo), calls of animals that are not from here and a singing (InSintesi), a rush of air
// (szegvari), and now and then a long dark horn across the desert (JarredGibb). With the bought
// sounds (Audio.ts) there is one more: a single note that swells out of nowhere and is gone.

const CALLS = ['alienVoice', 'alienRadio', 'alienCalls', 'alienSong', 'alienTemple'];
const RARE = 'alienHorn';
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export interface CallsWhen {
  /** On foot, outside, at night, the flashlight on, no scene holding the player. */
  ok: boolean;
  cam: THREE.Camera;
  /** The flashlight's stutter, 0..1 (Flashlight.flicker). */
  flicker: (k: number) => void;
  /** The night goes quiet for a few seconds (Ambience.hush). */
  hush: (secs: number) => void;
}

export class NightCalls {
  /** Seconds of walking with the light on before the next. */
  next = rnd(45, 100);
  /** How many so far (tests, the developer menu). */
  count = 0;
  /** The last one: which, where, how far and how far round from where the player looked (tests). */
  last: { name: string; at: THREE.Vector3; dist: number; off: number } | null = null;
  private lead = 0;              // seconds from the hush to the call
  private stutter = 0;           // the flashlight's stutter, fading after the call
  private before: string | null = null;

  constructor(private audio: AudioSys) {}

  /** One now (the developer menu, tests): the hush, then the call in a moment. */
  now() { this.next = 0; }

  update(dt: number, w: CallsWhen) {
    // the stutter fades out after a call
    if (this.stutter > 0) { this.stutter = Math.max(0, this.stutter - dt * 0.35); w.flicker(this.stutter); }
    if (!w.ok) { if (this.lead > 0) { this.lead = 0; w.flicker(0); this.stutter = 0; } return; }
    if (this.lead > 0) {
      this.lead -= dt;
      w.flicker(Math.min(1, 0.4 + (1.8 - this.lead) * 0.4));
      if (this.lead <= 0) this.call(w.cam);
      return;
    }
    this.next -= dt;
    if (this.next > 0) return;
    // first the quiet: the crickets and the wind drop away, the light starts to stutter
    this.lead = 1.8;
    w.hush(10);
    this.next = rnd(90, 220);
  }

  private call(cam: THREE.Camera) {
    const tone = this.audio.or('call_tone'), calls = tone ? [...CALLS, tone] : CALLS;
    let name = Math.random() < 0.12 ? RARE : calls[Math.floor(Math.random() * calls.length)];
    if (name === this.before) name = calls[(calls.indexOf(name) + 1) % calls.length];
    this.before = name;
    // behind the player or off to one side, out of the beam, and far
    const f = new THREE.Vector3(); cam.getWorldDirection(f);
    const yaw = Math.atan2(f.x, f.z);
    const off = (Math.random() < 0.5 ? -1 : 1) * rnd(1.6, Math.PI);   // 90 to 180 degrees round
    const dist = name === RARE ? rnd(90, 140) : rnd(35, 110);
    const at = cam.position.clone().add(new THREE.Vector3(Math.sin(yaw + off) * dist, rnd(-0.5, 4), Math.cos(yaw + off) * dist));
    const pan = this.audio.spatial(at, 28);
    if (!pan) return;
    this.audio.play(name, { dest: pan, gain: name === RARE ? 0.55 : 0.75, rate: rnd(0.86, 1.04), lp: THREE.MathUtils.clamp(4200 - dist * 22, 1100, 3600) });
    this.stutter = 1;
    this.count++;
    this.last = { name, at, dist, off: Math.abs(off) };
  }
}
