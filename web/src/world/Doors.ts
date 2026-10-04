import * as THREE from 'three';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import type { Interaction } from '../core/Interaction';
import type { AudioSys } from '../core/Audio';

// The doors of SARO, open and shut at any time of the night (Tom, 4 October: the player
// moves freely, whatever the chapter). A door swings, and while it is open its doorway
// zone joins the rooms on either side and its open leaf blocks the floor it stands on.
// The chapters do not own doors any more; they may listen (onChange) and ask (isOpen).
// The state goes into every save.

export type DoorId = 'east' | 'south' | 'lab' | 'exit';
export interface DoorSpec {
  id: DoorId;
  inter: string;                   // interactable id (kept from before, the tests use them)
  name: string;                    // 'service door': labels say "Open service door"
  proxy: THREE.Object3D;
  set: (k: number) => void;        // 0 shut, 1 open: moves the leaf
  zones: Zone[];                   // enabled while open
  leaf?: Collider;                 // the open leaf, while open
  range?: number;
  sound?: 'heavy' | 'light';
}

interface Door { spec: DoorSpec; open: boolean; k: number; from: number; t0: number }
const SWING = 0.9;

export class Doors {
  onChange?: (id: DoorId, open: boolean) => void;
  private doors = new Map<DoorId, Door>();
  private t = 0;

  constructor(private d: { inter: Interaction; audio: AudioSys; colliders: Collider[]; player: { pos: THREE.Vector3; radius: number }; toast: (text: string, secs?: number) => void; busy: () => boolean }) {}

  add(spec: DoorSpec) {
    const door: Door = { spec, open: false, k: 0, from: 0, t0: -1 };
    this.doors.set(spec.id, door);
    this.apply(door, 0);
    this.d.inter.add({ id: spec.inter, object: spec.proxy, range: spec.range ?? 2.4,
      label: () => `${door.open ? 'Close' : 'Open'} ${spec.name}`,
      use: () => this.toggle(spec.id) });
  }

  isOpen(id: DoorId) { return !!this.doors.get(id)?.open; }
  states(): Record<string, boolean> {
    const out: Record<string, boolean> = {};
    for (const [id, d] of this.doors) out[id] = d.open;
    return out;
  }

  /** Open or shut a door. `instant` puts it there at once (a restored save, a new night). */
  set(id: DoorId, open: boolean, instant = false) {
    const door = this.doors.get(id);
    if (!door) return;
    const changed = door.open !== open;
    door.open = open;
    for (const z of door.spec.zones) z.enabled = open;
    if (door.spec.leaf) {
      const cols = this.d.colliders, i = cols.indexOf(door.spec.leaf);
      if (open && i < 0) cols.push(door.spec.leaf);
      if (!open && i >= 0) cols.splice(i, 1);
    }
    if (instant) { door.t0 = -1; this.apply(door, open ? 1 : 0); }
    else if (changed) { door.from = door.k; door.t0 = this.t; }
    if (changed) this.onChange?.(id, open);
  }
  /** Back to the start of the night: every door shut. */
  closeAll() { for (const id of this.doors.keys()) this.set(id, false, true); }
  restore(states: Record<string, boolean>) {
    for (const [id, open] of Object.entries(states)) this.set(id as DoorId, !!open, true);
  }

  toggle(id: DoorId) {
    const door = this.doors.get(id);
    if (!door || this.d.busy()) return;
    if (door.open && this.inDoorway(door)) { this.d.toast('Step out of the doorway first.', 2.2); return; }
    this.set(id, !door.open);
    const at = door.spec.proxy.getWorldPosition(new THREE.Vector3());
    const heavy = door.spec.sound !== 'light';
    this.d.audio.play('switch', { gain: heavy ? 0.55 : 0.45, rate: door.open ? 1 : 0.85, at });
    this.d.audio.play('thudSoft', { gain: door.open ? 0.3 : 0.45, when: door.open ? 0.6 : 0.75, at });
  }

  // Standing in the doorway, or where the leaf swings: the door stays open.
  private inDoorway(door: Door) {
    const p = this.d.player.pos, r = this.d.player.radius;
    const near = (c: Collider) => p.x > c.minX - r && p.x < c.maxX + r && p.z > c.minZ - r && p.z < c.maxZ + r;
    return door.spec.zones.some((z) => near(z)) || (!!door.spec.leaf && near(door.spec.leaf));
  }

  private apply(door: Door, k: number) { door.k = k; door.spec.set(k); }

  update(dt: number) {
    this.t += dt;
    for (const door of this.doors.values()) {
      if (door.t0 < 0) continue;
      const u = Math.min(1, (this.t - door.t0) / SWING);
      const e = 1 - Math.pow(1 - u, 3);
      this.apply(door, door.from + ((door.open ? 1 : 0) - door.from) * e);
      if (u >= 1) door.t0 = -1;
    }
  }
}
