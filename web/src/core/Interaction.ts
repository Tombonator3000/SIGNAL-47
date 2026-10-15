import * as THREE from 'three';

export interface Interactable {
  id: string;
  object: THREE.Object3D;
  label: () => string | null; // null hides the prompt (object is inert right now)
  use: () => void;
  range?: number;
  /** What a closer look says (the zoom: right mouse, or Zoom on a phone), English, in the game. */
  examine?: () => string | null;
  /** How far off it can be looked at closely (8 m by default). */
  examineRange?: number;
}

export class Interaction {
  items: Interactable[] = [];
  private ray = new THREE.Raycaster();
  private meshes: THREE.Object3D[] = [];
  current: Interactable | null = null;

  add(it: Interactable) {
    it.object.traverse((o) => { o.userData.interactId = it.id; });
    this.items.push(it);
    this.meshes.push(it.object);
    return it;
  }
  get(id: string) { return this.items.find((i) => i.id === id); }

  pick(camera: THREE.Camera, ndc = new THREE.Vector2(0, 0), maxDist = 2.4): Interactable | null {
    this.ray.setFromCamera(ndc, camera);
    this.ray.far = 6;
    const hits = this.ray.intersectObjects(this.meshes, true);
    for (const h of hits) {
      const id = h.object.userData.interactId;
      const it = this.items.find((i) => i.id === id);
      if (!it) continue;
      if (h.distance > (it.range ?? maxDist)) return null;
      return it.label() ? it : null;
    }
    return null;
  }

  /** A closer look (the zoom): the first thing on the line of sight that has something to say
   *  about it, within its examineRange, whether or not there is anything to use. */
  examine(camera: THREE.Camera): { it: Interactable; text: string } | null {
    this.ray.setFromCamera(new THREE.Vector2(0, 0), camera);
    this.ray.far = 60;
    for (const h of this.ray.intersectObjects(this.meshes, true)) {
      const it = this.items.find((i) => i.id === h.object.userData.interactId);
      if (!it?.examine || h.distance > (it.examineRange ?? 8)) continue;
      const text = it.examine();
      if (text) return { it, text };
    }
    return null;
  }

  update(camera: THREE.Camera) {
    camera.updateMatrixWorld(); // the player moved the camera this step; aim with where it is now
    this.current = this.pick(camera);
    return this.current;
  }
}
