// TEMPORARY STUB until the drive agent delivers. Not for commit.
import * as THREE from 'three';
import type { FloodSet } from '../world/kit';
export class Truck {
  group = new THREE.Group();
  cab = new THREE.Group();
  driverEye = new THREE.Vector3(-0.4, 1.45, -0.2);
  constructor(_o: { flood?: FloodSet } = {}) {
    const m = new THREE.Mesh(new THREE.BoxGeometry(2.0, 1.8, 5.3), new THREE.MeshStandardMaterial({ color: 0xdddddd }));
    m.position.y = 0.9; this.group.add(m);
  }
  setDash(_o: { mph: number; rpm: number; clock: string; fuel: number }) {}
  setSteer(_v: number) {}
  setHeadlights(_on: boolean) {}
  setLightLevel(_l: number) {}
  setBrake(_on: boolean) {}
  headlightFloods(_set: FloodSet, _slots: number[]) {}
  setDriving(_on: boolean) {}
  update(_dt: number, _speed: number, _wheel = 0) {}
}
