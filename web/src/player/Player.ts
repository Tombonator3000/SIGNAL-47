import * as THREE from 'three';
import type { Collider } from '../world/ControlRoom';

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

  constructor(public camera: THREE.PerspectiveCamera, public colliders: Collider[], public bounds: { minX: number; maxX: number; minZ: number; maxZ: number }) {}

  place(x: number, z: number, yaw: number) { this.pos.set(x, 0, z); this.yaw = yaw; this.pitch = -0.06; }

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
    this.pos.x += vx; this.resolve('x');
    this.pos.z += vz; this.resolve('z');
    const moved = Math.hypot(this.pos.x - before.x, this.pos.z - before.z);
    if (moved > 0.0005) {
      this.bob += moved * 7.2;
      this.stepDist += moved;
      if (this.stepDist > 0.62) { this.stepDist = 0; this.onStep?.(); }
    } else this.bob *= 0.9;

    this.shake = Math.max(0, this.shake - dt * 1.4);
    const sh = this.shake * this.shake;
    this.camera.position.set(
      this.pos.x + (Math.random() - 0.5) * sh * 0.08,
      this.eye + Math.sin(this.bob) * 0.018 + (Math.random() - 0.5) * sh * 0.06,
      this.pos.z + (Math.random() - 0.5) * sh * 0.08,
    );
    this.camera.rotation.set(this.pitch + (Math.random() - 0.5) * sh * 0.02, this.yaw, Math.sin(this.bob * 0.5) * 0.004, 'YXZ');
  }

  private resolve(axis: 'x' | 'z') {
    const r = this.radius, p = this.pos, b = this.bounds;
    p.x = THREE.MathUtils.clamp(p.x, b.minX + r, b.maxX - r);
    p.z = THREE.MathUtils.clamp(p.z, b.minZ + r, b.maxZ - r);
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
