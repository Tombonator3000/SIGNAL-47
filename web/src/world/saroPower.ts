import * as THREE from 'three';
import { M, type FloodSet } from './kit';
import type { GlowPoints } from './glow';
import type { ControlRoom } from './ControlRoom';
import type { ServiceYard } from './ServiceYard';
import type { RecordsAnnex } from './Annex';
import type { Exterior } from './Exterior';
import { materialFor } from '../core/quality';

// SARO's own power (story/saroDark.ts: 02:29, the station goes dark for 47 seconds). Every light
// that is SARO's goes with it: the control room's lamps, tubes and screens, the annex's tubes, the
// photo lab's safelights and lamps, the red glass of the DARKROOM lamp, the yard's and the array's
// sodium lamps, the wall packs, the drive cabinets' lamps and the red aircraft lamps on all 27
// antennas. What is not SARO's stays lit: the highway's street lamps along the road west of the site
// (the county's, on their own line: their orange spills over the grounds by the windows side, and a
// figure standing there is black against it), the motel's sign far down the road, the truck's own
// lights, and the lit places on the horizon.
//
// Two levels: `inner` for what comes straight back (tubes, safelights, incandescent lamps, screens,
// the red lamps) and `sodium` for the sodium lamps outside, which come back dim and take their time
// to warm up again, as sodium does after an outage. The state before the cut is taken whole and put
// back exactly (restore()), so nothing that was dimmed or switched before (the tube that died at the
// impact, the bank's screen with K3 open, the B-12 work lamp) is changed by it. A lamp the PC tier has
// handed to a real light (core/ultra.ts) is read through `strength`, and written as the game writes
// it, so the real light follows. Once the inside has struck again (inner back to 1, `settle`), it is
// left alone; and a lamp the game changes itself while the sodium lamps warm up (the B-12 work lamp
// with its shield) keeps what the game gave it: that becomes the level the lamp warms up to.

export interface PowerParts {
  room: ControlRoom;
  yard: ServiceYard;
  annex: RecordsAnnex;
  ext: Exterior;
  /** The antenna field's drive cabinets (World.field.glow). */
  fieldGlow: GlowPoints;
  /** The site's flood set (kit.flood): SARO's slots in it are found by where they stand. */
  site: FloodSet;
  /** A fake lamp's strength as the game wants it (core/ultra.ts, Ultra.strength). */
  strength: (set: FloodSet, i: number) => number;
}

interface GlowSnap { g: GlowPoints; col: Float32Array; pick: number[]; sodium: boolean[]; last: Float32Array }
interface Snap {
  ceiling: number[]; lamps: number[]; crt: number;
  tubes: THREE.Material[]; screens: boolean[]; door: THREE.Material | THREE.Material[];
  lab: number; annex: number;
  red: THREE.Color[]; bulb: THREE.Color;
  glows: GlowSnap[];
  slots: { i: number; w: number; last: number }[];
  /** The inside is back as it was and left alone from now on. */
  settled: boolean;
}

const OFF_GLASS = new THREE.MeshBasicMaterial({ color: 0x1a1a1a });
/** Warm, high sodium heads among the glows (the same test as world/airlight.ts). */
const isSodium = (r: number, g: number, b: number) => r > 0.85 && g > 0.3 && g < 0.62 && b < 0.26;
/** Where SARO is in its own scene: not the highway's lamp line (x -20), the motel down the road or
 *  the horizon's lights. */
const saroAt = (x: number, z: number) => z < 300 && Math.hypot(x, z) < 520 && Math.abs(x + 20.05) > 1.3;

export class SaroPower {
  inner = 1;
  sodium = 1;
  private snap: Snap | null = null;
  constructor(private p: PowerParts) {}

  /** The power is out, or coming back (anything but the state before the cut). */
  get out() { return this.snap !== null; }

  /** The power goes: everything as it is now is kept, and all of it dark. */
  cut() {
    if (!this.snap) this.snap = this.take();
    this.set(0, 0);
  }
  /** Levels while out or coming back (0..1 each). */
  set(inner: number, sodium: number) {
    const s = this.snap; if (!s) return;
    this.sodium = sodium;
    if (!s.settled) this.inside(s, inner);
    const { site, strength } = this.p;
    // the lamps' own glass (the yard's lamp posts and the desk lamps' bulbs: M.lampBulb is SARO's only)
    M.lampBulb.color.copy(s.bulb).multiplyScalar(0.012 + 0.988 * sodium);
    for (const gs of s.glows) {
      const a = gs.g.points.geometry.getAttribute('aCol') as THREE.BufferAttribute | undefined;
      if (!a) continue;
      gs.pick.forEach((i, k) => {
        if (!gs.sodium[k] && s.settled) return;
        // (changed by the game since: that is what it is to be)
        const c = a.array as Float32Array, j = i * 3;
        if (Math.abs(c[j] - gs.last[j]) + Math.abs(c[j + 1] - gs.last[j + 1]) + Math.abs(c[j + 2] - gs.last[j + 2]) > 1e-5) { gs.col[j] = c[j]; gs.col[j + 1] = c[j + 1]; gs.col[j + 2] = c[j + 2]; }
        const f = gs.sodium[k] ? sodium : inner;
        a.setXYZ(i, gs.col[j] * f, gs.col[j + 1] * f, gs.col[j + 2] * f);
        gs.last[j] = c[j]; gs.last[j + 1] = c[j + 1]; gs.last[j + 2] = c[j + 2];
      });
      a.needsUpdate = true;
    }
    for (const sl of s.slots) {
      const now = strength(site, sl.i);
      if (Math.abs(now - sl.last) > 1e-6) sl.w = now;
      sl.last = site.pos[sl.i].w = sl.w * sodium;
    }
  }
  /** The inside back as it was, and left alone from now on (the tubes, the safelights, the screens,
   *  the red lamps: they come straight back; the sodium lamps go on warming up). */
  settle() {
    const s = this.snap; if (!s || s.settled) return;
    this.inside(s, 1);
    s.settled = true;
    this.set(1, this.sodium);
  }
  private inside(s: Snap, inner: number) {
    this.inner = inner;
    const { room, yard, annex } = this.p;
    const on = inner > 0.5;
    room.lights.ceiling.forEach((l, i) => { l.intensity = s.ceiling[i] * inner; });
    room.lights.lamps.forEach((l, i) => { l.intensity = s.lamps[i] * inner; });
    room.lights.crt.intensity = s.crt * inner;
    room.tubes.forEach((tb, i) => { tb.material = on ? s.tubes[i] : materialFor(M.emissiveTubeOff); });
    [room.crtLeft, room.crtCenter, room.crtRight, room.crtOps].forEach((c, i) => c.setPowered(on && s.screens[i]));
    room.doorLight.material = on ? s.door : OFF_GLASS;
    yard.labFlood.scale.value = s.lab * inner;
    annex.flood.scale.value = s.annex * inner;
    annex.setPower(inner);
    yard.redGlass.forEach((m, i) => m.color.copy(s.red[i]).multiplyScalar(0.008 + 0.992 * inner));
  }
  /** The power is back and everything as it was before the cut (or as the game has set it since). */
  restore() {
    const s = this.snap; if (!s) return;
    this.settle();
    this.set(1, 1);
    this.snap = null;
    this.inner = this.sodium = 1;
  }

  private take(): Snap {
    const { room, yard, annex, ext, fieldGlow, site, strength } = this.p;
    const glow = (g: GlowPoints, keep: (x: number, y: number, z: number) => boolean): GlowSnap | null => {
      const a = g.points.geometry.getAttribute('aCol') as THREE.BufferAttribute | undefined;
      const pos = g.points.geometry.getAttribute('position') as THREE.BufferAttribute | undefined;
      if (!a || !pos) return null;
      const col = Float32Array.from(a.array as ArrayLike<number>), pick: number[] = [], sodium: boolean[] = [];
      for (let i = 0; i < a.count; i++) {
        if (!keep(pos.getX(i), pos.getY(i), pos.getZ(i))) continue;
        pick.push(i); sodium.push(isSodium(col[i * 3], col[i * 3 + 1], col[i * 3 + 2]));
      }
      return { g, col, pick, sodium, last: Float32Array.from(col) };
    };
    const all = () => true;
    const glows = [
      glow(yard.glow, all), glow(annex.glow, all), glow(fieldGlow, all), glow(ext.dishArray.beaconGlow, all),
      // the site's own lamps among the exterior's: not the highway's, the motel's sign or the horizon's
      glow(ext.glow, (x, _y, z) => saroAt(x, z)),
    ].filter((g): g is GlowSnap => !!g);
    const slots: { i: number; w: number; last: number }[] = [];
    for (let i = 0; i < site.count; i++) {
      if (site.skip.has(i)) continue;
      const p = site.pos[i];
      if (p.y < -100 || !saroAt(p.x, p.z)) continue;
      const w = strength(site, i);
      slots.push({ i, w, last: w });
    }
    return {
      ceiling: room.lights.ceiling.map((l) => l.intensity), lamps: room.lights.lamps.map((l) => l.intensity), crt: room.lights.crt.intensity,
      tubes: room.tubes.map((t) => t.material as THREE.Material),
      screens: [room.crtLeft, room.crtCenter, room.crtRight, room.crtOps].map((c) => c.powered),
      door: room.doorLight.material,
      lab: yard.labFlood.scale.value, annex: annex.flood.scale.value,
      red: yard.redGlass.map((m) => m.color.clone()), bulb: M.lampBulb.color.clone(),
      glows, slots, settled: false,
    };
  }

  /** Back to the state before the cut at once (a new night, the title, a save opened). */
  reset() { this.restore(); }
}
