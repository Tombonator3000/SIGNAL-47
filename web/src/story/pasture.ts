import * as THREE from 'three';
import type { Herd } from '../drive/herd';
import { HERD } from '../drive/roadShape';
import { clockText } from './time';

// HENDELSER.md, event 2: the cows, on B, in the pasture beside the highway south of the motel
// (drive/herd.ts has them). Until it comes they graze as cattle do, each its own way, heads
// going down and up. Then, once in a night, the whole herd stands facing az 026, quite still,
// also when the truck comes by with its lights right on them; nine head. The next time the
// player comes by there are eight, and where the ninth stood there is a ring of flattened grass
// with frost on it. Nothing bloody. Once that has been seen, and the player has gone, the eight
// graze again; the ring stays for the rest of the night.
//
// Nothing changes while it can be seen: the herd turns, loses the one and goes back to grazing
// only while the player is far off (out of the areas it stands in, or more than FAR away).
// It turns the first time once the player has been by it once, or from 03:45 in any case, and
// at least ten minutes of the night after the last event (as in town, story/townEvents.ts).
// Kept with the town's part of the case (main.ts, `town`): 'cows:on', 'ev:cows1', 'cows:short',
// 'ev:cows', and a mark on the map at the pasture, on B; none of it comes again after a load.

export interface PastureIn {
  /** In play, no panel, not in a cinematic, at night, and the story allows it. */
  ok: boolean;
  /** Which herd's area the player is in ('saro', 'road'), or null. */
  area: 'saro' | 'road' | null;
  cam: THREE.Camera;
  /** The game's clock, seconds since midnight. */
  clock: number;
  /** Driving the truck (its headlights light the herd), or on foot with the flashlight on. */
  driving: boolean;
  flashlight: boolean;
}
export interface PastureDeps {
  /** The herds as the areas are built (World.herds). */
  herds: () => readonly Herd[];
  seen: Set<string>;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  save: () => void;
}

type Stage = 'graze' | 'still' | 'short' | 'after';
const GAP = 10 * 60;
/** Metres: by it (a pass), and far enough off that a change cannot be seen. */
const NEAR = 150, FAR = 260;
/** Where the pasture is on the notebook's map: metres down the highway from SARO (road z + 300). */
export const PASTURE_M = HERD.z + 300;

export class Pasture {
  stage: Stage = 'graze';
  /** Times the player has come by while the herd grazed. */
  passes = 0;
  private near = false;
  private force = false;
  private _v = new THREE.Vector3();

  constructor(private d: PastureDeps) {}

  private last() {
    let t = -1e9;
    for (const k of this.d.seen) if (k.startsWith('mark:')) t = Math.max(t, Number(k.slice(k.indexOf('@') + 1)));
    return t;
  }
  /** After a load the herd stands as that case left it (nobody is looking while it loads). */
  reset() {
    const s = this.d.seen;
    this.stage = s.has('ev:cows') ? 'after' : s.has('cows:short') ? 'short' : s.has('cows:on') ? 'still' : 'graze';
    this.passes = 0; this.near = false; this.force = false;
    this.apply();
  }
  private apply() {
    const st = this.stage, gone = st === 'short' || st === 'after';
    for (const h of this.d.herds()) h.set(st === 'still' || st === 'short', gone ? HERD.missing : -1, gone);
  }

  update(_dt: number, s: PastureIn) {
    // a herd built since (SARO's stretch, the road) takes the state at once
    this.apply();
    const h = s.area ? this.d.herds().find((x) => x.area === s.area) ?? null : null;
    const cam = s.cam.getWorldPosition(this._v.clone());
    const dist = h ? cam.distanceTo(h.middle(this._v)) : Infinity;
    const far = dist > FAR;
    const day = ((s.clock % 86400) + 86400) % 86400;
    const seen = this.d.seen;

    // ---------- the changes, out of sight ----------
    if (dist < NEAR) this.near = true;
    else if (far && this.near) { this.near = false; if (this.stage === 'graze') this.passes++; }
    if (far) {
      if (this.stage === 'graze' && (this.force || (s.ok && (this.passes > 0 || day >= 3 * 3600 + 45 * 60) && day >= 2 * 3600 + 40 * 60 && day < 5 * 3600
        && s.clock - this.last() >= GAP))) {
        this.stage = 'still'; this.force = false; seen.add('cows:on'); this.d.save();
      } else if (this.stage === 'still' && seen.has('ev:cows1')) {
        this.stage = 'short'; seen.add('cows:short'); this.d.save();
      } else if (this.stage === 'short' && seen.has('ev:cows')) this.stage = 'after';
      this.apply();
    }

    // ---------- what the player sees of it ----------
    if (!h || !s.ok || dist > 120 || (this.stage !== 'still' && this.stage !== 'short')) return;
    const lit = this.litCows(h, s, cam);
    const t = clockText(s.clock, false);
    if (this.stage === 'still' && !seen.has('ev:cows1') && lit >= (s.driving ? 3 : 2)) {
      seen.add('ev:cows1'); seen.add(`mark:cows@${Math.round(s.clock)}`);
      this.d.note(`${t}. The pasture south of the motel: the whole herd standing facing the same way, north-north-east, on 026. Nine head. Not one of them moved, with the ${s.driving ? 'headlights' : 'flashlight'} right on them.`);
      this.d.save();
    } else if (this.stage === 'short' && !seen.has('ev:cows') && lit >= (s.driving ? 3 : 2)) {
      seen.add('ev:cows');
      this.d.toast('The herd in the pasture, still facing the same way. Eight of them. There were nine. Where the ninth stood, the grass is flattened in a ring, white with frost.', 6.5);
      this.d.note(`${t}. Pasture by the highway: the herd facing 026. One missing. A ring of frost in the grass.`);
      this.d.save();
    }
  }
  /** How many cows are in front of the player and lit: by the headlights, or by the flashlight
   *  close enough. */
  private litCows(h: Herd, s: PastureIn, cam: THREE.Vector3) {
    const fwd = s.cam.getWorldDirection(new THREE.Vector3()), p = new THREE.Vector3();
    let n = 0;
    for (let i = 0; i < h.count; i++) {
      if (!h.cowAt(i, p)) continue;
      const d = p.distanceTo(cam), toward = p.sub(cam).normalize().dot(fwd);
      if (s.driving ? d < 85 && toward > 0.8 && h.litAt(i) > 0.2 : s.flashlight && d < 34 && toward > 0.9) n++;
    }
    return n;
  }
  /** Turn them now, whatever the hour (the developer menu): as soon as the player is far off. */
  now() { if (this.stage === 'graze') this.force = true; }
}
