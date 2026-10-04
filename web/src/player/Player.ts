import * as THREE from 'three';
import type { Collider } from '../world/ControlRoom';

// A walkable rectangle. The player may stand anywhere inside the union of the enabled
// zones, minus the colliders. Doorways are small zones that bridge two rooms and are
// switched on when the door opens.
export type Zone = Collider & { id?: string; enabled?: boolean };

export class Player {
  pos = new THREE.Vector3();
  yaw = 0; pitch = 0;
  eye = 1.62;
  radius = 0.27;
  speed = 1.9;
  bob = 0;
  stepDist = 0;
  shake = 0;
  onStep?: () => void;
  /** Ground height under the player, where it is not the floor at y = 0 (the walk down
   *  from SARO's fire exit and over the road). The eye follows it smoothly. */
  floor: ((x: number, z: number) => number) | null = null;
  floorY = 0;

  constructor(public camera: THREE.PerspectiveCamera, public colliders: Collider[], public zones: Zone[]) {}

  place(x: number, z: number, yaw: number) { this.pos.set(x, 0, z); this.yaw = yaw; this.pitch = -0.06; this.floorY = this.floor ? this.floor(x, z) : 0; }

  look(dx: number, dy: number) {
    this.yaw -= dx;
    this.pitch = THREE.MathUtils.clamp(this.pitch - dy, -1.35, 1.25);
  }

  update(dt: number, mx: number, mz: number, run: boolean) {
    const sp = this.speed * (run ? 1.6 : 1);
    const s = Math.sin(this.yaw), c = Math.cos(this.yaw);
    // forward is -Z in camera space
    const vx = (mx * c + mz * s) * sp * dt;
    const vz = (-mx * s + mz * c) * sp * dt;
    const before = this.pos.clone();
    this.pos.x += vx; this.resolve('x', before.x);
    this.pos.z += vz; this.resolve('z', before.z);
    const moved = Math.hypot(this.pos.x - before.x, this.pos.z - before.z);
    if (moved > 0.0005) {
      this.bob += moved * 7.2;
      this.stepDist += moved;
      if (this.stepDist > 0.62) { this.stepDist = 0; this.onStep?.(); }
    } else this.bob *= 0.9;

    const fy = this.floor ? this.floor(this.pos.x, this.pos.z) : 0;
    this.floorY += (fy - this.floorY) * Math.min(1, dt * 14);
    this.shake = Math.max(0, this.shake - dt * 1.4);
    const sh = this.shake * this.shake;
    this.camera.position.set(
      this.pos.x + (Math.random() - 0.5) * sh * 0.08,
      this.floorY + this.eye + Math.sin(this.bob) * 0.018 + (Math.random() - 0.5) * sh * 0.06,
      this.pos.z + (Math.random() - 0.5) * sh * 0.08,
    );
    this.camera.rotation.set(this.pitch + (Math.random() - 0.5) * sh * 0.02, this.yaw, Math.sin(this.bob * 0.5) * 0.004, 'YXZ');
  }

  private inside(b: Zone, x: number, z: number) {
    const r = this.radius, e = 1e-6;
    return x >= b.minX + r - e && x <= b.maxX - r + e && z >= b.minZ + r - e && z <= b.maxZ - r + e;
  }
  walkable(x: number, z: number) {
    for (const b of this.zones) if (b.enabled !== false && this.inside(b, x, z)) return true;
    return false;
  }

  private resolve(axis: 'x' | 'z', old: number) {
    const r = this.radius, p = this.pos;
    if (!this.walkable(p.x, p.z)) {
      // Slide along the edge of whichever zone we were standing in.
      const want = axis === 'x' ? p.x : p.z;
      let best = old;
      for (const b of this.zones) {
        if (b.enabled === false) continue;
        const ox = axis === 'x' ? old : p.x, oz = axis === 'z' ? old : p.z;
        if (!this.inside(b, ox, oz)) continue;
        const v = axis === 'x' ? THREE.MathUtils.clamp(want, b.minX + r, b.maxX - r) : THREE.MathUtils.clamp(want, b.minZ + r, b.maxZ - r);
        if (Math.abs(v - want) < Math.abs(best - want)) best = v;
      }
      if (axis === 'x') p.x = best; else p.z = best;
    }
    for (const c of this.colliders) {
      if (p.x + r <= c.minX || p.x - r >= c.maxX || p.z + r <= c.minZ || p.z - r >= c.maxZ) continue;
      if (axis === 'x') {
        const left = p.x + r - c.minX, right = c.maxX - (p.x - r);
        p.x += left < right ? -left : right;
      } else {
        const front = p.z + r - c.minZ, back = c.maxZ - (p.z - r);
        p.z += front < back ? -front : back;
      }
    }
  }
}
