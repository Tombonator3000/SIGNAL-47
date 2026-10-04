// TEMPORARY STUB until the drive agent delivers. Not for commit.
import * as THREE from 'three';
import type { Truck } from './Truck';
import type { RoadArea } from './RoadArea';
export class DriveController {
  pos = new THREE.Vector3(); heading = 0; speed = 0; mph = 0; rpm = 800;
  onArrive?: () => void; onBump?: (s: number) => void;
  constructor(private camera: THREE.PerspectiveCamera, private truck: Truck, private road: RoadArea) {}
  place(p: THREE.Vector3, h: number) { this.pos.copy(p); this.heading = h; }
  update(dt: number, i: { steer: number; throttle: number }, _look: { x: number; y: number }) {
    this.speed = Math.max(0, this.speed + i.throttle * 3.5 * dt);
    this.heading -= i.steer * dt * 0.5;
    this.pos.x -= Math.sin(this.heading) * this.speed * dt; this.pos.z -= Math.cos(this.heading) * this.speed * dt;
    this.mph = this.speed * 2.237;
    this.truck.group.position.copy(this.pos); this.truck.group.rotation.y = this.heading;
    this.camera.position.set(this.pos.x, 1.45, this.pos.z); this.camera.rotation.set(0, this.heading, 0, 'YXZ');
    const z = this.road.endZone;
    if (this.pos.x > z.minX && this.pos.x < z.maxX && this.pos.z > z.minZ && this.pos.z < z.maxZ && this.speed < 6) this.onArrive?.();
  }
}
