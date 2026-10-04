// TEMPORARY STUB until the drive agent delivers. Not for commit.
import * as THREE from 'three';
import { floodSet } from '../world/kit';
export const roadFlood = floodSet(8, 'road', 0.08);
export class RoadArea {
  group = new THREE.Group();
  start: { pos: THREE.Vector3; heading: number };
  end: { pos: THREE.Vector3; heading: number };
  endZone: { minX: number; maxX: number; minZ: number; maxZ: number };
  obstacles = [];
  onCattleGuard?: (speed: number) => void;
  constructor(origin: THREE.Vector3) {
    this.group.position.copy(origin);
    this.group.add(new THREE.HemisphereLight(0x2c3b5e, 0x1b140e, 0.85));
    const g = new THREE.Mesh(new THREE.PlaneGeometry(40, 1200), new THREE.MeshStandardMaterial({ color: 0x555555 }));
    g.rotation.x = -Math.PI / 2; g.position.z = 600; this.group.add(g);
    this.start = { pos: origin.clone().add(new THREE.Vector3(0, 0, 0)), heading: Math.PI };
    this.end = { pos: origin.clone().add(new THREE.Vector3(0, 0, 1100)), heading: Math.PI };
    this.endZone = { minX: origin.x - 10, maxX: origin.x + 10, minZ: origin.z + 1090, maxZ: origin.z + 1110 };
  }
  surface(_x: number, _z: number) { return { kind: 'asphalt' as const, grip: 1, top: 30 }; }
  update(_dt: number, _t: number, _p: THREE.Vector3) {}
  dispose() {}
}
