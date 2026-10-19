import * as THREE from 'three';
import { FLASH_ANGLE, FLASH_BEAM, FLASH_GAIN, FLASH_DECAY, FLASH_PLAIN } from './cookies';

// The flashlight (Tom 15 October: a flashlight, no battery). It lies on the truck's passenger
// seat (drive/Truck.ts draws it there) and is taken the first time the player gets in; F
// switches it, and on a phone the Light button. It is a real spot light from the player's
// hand: the only light that goes everywhere, so it is not one of an area's fake floods. It
// stays in the scene with intensity 0 when it is off, since adding or taking away a light
// rebuilds every material's shader. On the PC tier it casts a shadow, so it does not shine
// through walls (main.ts passes the tier). Near the strange events it may flicker (flicker()).
// Its beam is a cheap flashlight's, not a perfect disc (Tom, 19 October, light cookies): the
// shaders draw it in the light (core/cookies.ts) with a hot core a little off the middle, the
// filament's dark spot, faint rings, a ragged rim and a dim corona. It fills the old disc's cone
// (so the shadow's camera on the PC tier sees no more than before): the beam is the inner three
// quarters of it and the corona the rest. The same real light on Low, High and Ultra, so every
// tier shows the same beam; the shaders know it by its linear decay (FLASH_DECAY).
export class Flashlight {
  /** Taken off the truck's seat (saved with the case). */
  have = false;
  on = false;
  readonly light: THREE.SpotLight;
  private level = 0;
  private flick = 0;
  private dark = 0;
  private readonly _f = new THREE.Vector3();
  private readonly _r = new THREE.Vector3();

  constructor(scene: THREE.Scene) {
    // a warm incandescent bulb behind a reflector: a bright middle and a soft edge. It fades
    // with the distance itself, not faster: with the physical fall-off a rail or a wall a hand's
    // breadth away came out hundreds of times brighter than the yard and blinded the picture
    // on the PC tier (Tom, 16 October); this is about as bright at 4 m and far milder up close.
    this.light = new THREE.SpotLight(0xffe6c0, 0, 30, FLASH_ANGLE, 0.12, FLASH_DECAY);
    this.light.name = 'flashlight';
    this.cookie(true);
    this.light.target.name = 'flashlight target';
    const sh = this.light.shadow;
    sh.mapSize.set(512, 512); sh.bias = -0.0006; sh.normalBias = 0.02; sh.camera.near = 0.1; sh.autoUpdate = false;
    scene.add(this.light, this.light.target);
  }

  /** The cookie in the beam, or the plain disc it had before (tools/cookies.py compares the two):
   *  the decay a hair off linear, which the shaders do not take for the flashlight. */
  cookie(on: boolean) {
    this.cookieOn = on;
    const L = this.light;
    L.decay = on ? FLASH_DECAY : FLASH_PLAIN;
    L.angle = FLASH_ANGLE; L.penumbra = on ? 0.12 : 0.55;
    // for the dust in the beam (world/nightlife.ts): the beam's own cone (outer, inner), not the
    // corona's, and its intensity at full strength
    L.userData.cone = on ? [Math.atan(FLASH_BEAM * Math.tan(FLASH_ANGLE)) * 1.12, Math.atan(FLASH_BEAM * Math.tan(FLASH_ANGLE)) * 0.6] : [0.4, 0.18];
    L.userData.full = on ? 40 * FLASH_GAIN : 40;
  }
  private cookieOn = true;

  /** F, or the Light button. False when there is none yet. */
  toggle() {
    if (!this.have) return false;
    this.on = !this.on;
    return true;
  }

  /** How much it flickers, 0 to 1, near the strange events. Each event says so for itself
   *  (from: the sounds out in the dark and the silence, a figure at the edge of the view, the
   *  town's events, the time away), and the strongest of them counts. */
  flicker(k: number, from = 'event') {
    this.flicks.set(from, THREE.MathUtils.clamp(k, 0, 1));
    let m = 0;
    for (const v of this.flicks.values()) m = Math.max(m, v);
    this.flick = m;
  }
  private readonly flicks = new Map<string, number>();

  /** Every frame: held a little right of and below the eyes, pointing where they look. off: in
   *  the truck, in a cinematic, on the title screen (it goes out, and comes back on after). */
  update(dt: number, t: number, cam: THREE.PerspectiveCamera, off: boolean, shadows = false) {
    const want = this.have && this.on && !off ? 1 : 0;
    this.level += (want - this.level) * Math.min(1, dt * 14);
    // a weak contact now and then when something is near: a dip, sometimes out for a moment
    let k = 1;
    if (this.flick > 0) {
      this.dark -= dt;
      if (this.dark <= 0 && Math.random() < this.flick * dt * 1.2) this.dark = 0.05 + Math.random() * 0.25 * this.flick;
      k = this.dark > 0 ? 0.08 : 1 - this.flick * 0.25 * (0.5 + 0.5 * Math.sin(t * 37) * Math.sin(t * 11.3));
    }
    const L = this.light;
    L.intensity = this.level < 0.01 ? 0 : 40 * (this.cookieOn ? FLASH_GAIN : 1) * this.level * k;
    cam.getWorldDirection(this._f);
    this._r.set(1, 0, 0).applyQuaternion(cam.quaternion);
    L.position.copy(cam.position).addScaledVector(this._r, 0.22); L.position.y -= 0.18;
    L.target.position.copy(cam.position).addScaledVector(this._f, 10);
    L.target.updateMatrixWorld();
    // the PC tier: a shadow, drawn each frame while it is on (it moves with the hand)
    L.castShadow = shadows;
    L.shadow.autoUpdate = shadows && L.intensity > 0;
  }
}
