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
  /** Only there for a closer look (an antenna, a sign): never offered for use, and never in the
   *  way of something behind it that is (pick() looks past it). */
  look?: boolean;
}

/** Drawn: the object and everything above it visible (a hit area itself has an invisible material). */
const shown = (o: THREE.Object3D | null) => { for (; o; o = o.parent) if (!o.visible) return false; return true; };

export class Interaction {
  items: Interactable[] = [];
  private ray = new THREE.Raycaster();
  private meshes: THREE.Object3D[] = [];
  current: Interactable | null = null;
  /** What a closer look says about things that carry no examine() of their own, by id (story/looks.ts):
   *  the papers on the desks, the motel, the diner and the rest of the night's things. Without a
   *  range of its own it reaches a metre past where the thing can be used, so the papers inside
   *  are not read through the control room's wall. */
  looks: Record<string, { say: () => string | null; range?: number }> = {};

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
      if (!it || it.look) continue;
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
      if (!it || !shown(h.object)) continue;   // nothing to look at in a hidden group (another area, a room not drawn)
      const look = it.examine ? null : this.looks[it.id], say = it.examine ?? look?.say;
      if (!say || h.distance > (look ? look.range ?? (it.range ?? 2.4) + 1 : it.examineRange ?? 8)) continue;
      const text = say();
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
