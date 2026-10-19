import * as THREE from 'three';

// The night wind, one set of uniforms for every shader that moves with it (the brush and grass
// of world/flora.ts; anything else in the air may take the same). time runs on in seconds, the
// strength is how far a tip moves in metres at the most and breathes in slow gusts, dir is the
// way it blows on the ground (x, z), swinging a little about the south-west.
export const wind = {
  time: { value: 0 },
  strength: { value: 0.07 },
  dir: { value: new THREE.Vector2(0.8, 0.6) },
};

/** Moves the wind on to t seconds (flora.ts, before every picture). */
export function windAt(t: number) {
  wind.time.value = t;
  wind.strength.value = 0.07 * (0.75 + 0.2 * Math.sin(t * 0.11) + 0.15 * Math.sin(t * 0.37 + 1.3));
  const a = 0.64 + 0.18 * Math.sin(t * 0.023);
  wind.dir.value.set(Math.cos(a), Math.sin(a));
}
