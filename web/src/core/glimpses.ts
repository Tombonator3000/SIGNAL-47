import * as THREE from 'three';
import { Silhouette } from './roadside';

// Them, in the background (HENDELSER.md, section 7; Tom, 16 October: always there, something
// that could happen at any time). One figure, about 1.2 m, black with a faint rim, stands still
// somewhere 40 to 300 m off, at the edge of the view. It is there only while it is more than 25
// degrees from the middle of the view: looked at, zoomed at, lit by the flashlight, or come
// closer than 40 m, and it was not there. No effect, no sound. It never moves and never comes
// nearer (the rules in HENDELSER.md, section 4).
//
// Where: behind the far antennas at SARO, out in the dark round Caliche and the diner, and on
// the highway's shoulder ahead of the truck at the far end of the headlights. When: now and then,
// rarely at first and more often towards morning, at most eight in a night. main.ts decides when
// it may run (not in the prologue, not in a cinematic, not once THE EVENT has the night).

export type GlimpseScene = 'saro' | 'road';
interface Spot { id: string; scene: GlimpseScene; x: number; z: number; rim: number }

const ROAD = new THREE.Vector3(8000, 0, 0);   // World.ROAD_ORIGIN
const SPOTS: Spot[] = [
  // SARO: behind antennas in the array's far rows
  { id: 'S-11', scene: 'saro', x: -12, z: -208, rim: 0x14100c },
  { id: 'S-14', scene: 'saro', x: 43, z: -238, rim: 0x14100c },
  { id: 'S-19', scene: 'saro', x: -42, z: -308, rim: 0x14100c },
  { id: 'S-22', scene: 'saro', x: 8, z: -368, rim: 0x14100c },
  // Caliche and the diner, in the road's terms (Town.ts), out past the last of the light
  { id: 'town-south', scene: 'road', x: ROAD.x + 3, z: 2296, rim: 0x2a1a08 },
  { id: 'town-mast', scene: 'road', x: ROAD.x + 47, z: 2052, rim: 0x2a0806 },
  { id: 'town-west', scene: 'road', x: ROAD.x - 62, z: 2086, rim: 0x1a1208 },
  { id: 'diner-west', scene: 'road', x: ROAD.x - 56, z: 1992, rim: 0x101a1a },
  { id: 'diner-north', scene: 'road', x: ROAD.x - 34, z: 1952, rim: 0x101a1a },
];
const MAX = 8;
const rnd = (a: number, b: number) => a + Math.random() * (b - a);

export interface GlimpsesIn {
  ok: boolean;
  scene: GlimpseScene | null;
  cam: THREE.PerspectiveCamera;
  clock: number;
  /** The ground's height under a point of this scene. */
  ground: (x: number, z: number) => number;
  /** Driving the highway: the truck's position and its heading (for the spot ahead). */
  truck: { pos: THREE.Vector3; dir: THREE.Vector3 } | null;
  flashlight: boolean;
  zoom: number;
}

export class Glimpses {
  /** Where it stands now, for the developer menu and the tests. */
  up: string | null = null;
  count = 0;
  private sil: Silhouette;
  private fig: THREE.Group;
  private wait = rnd(120, 300);
  private t = 0;

  constructor(scene: THREE.Scene) {
    this.sil = new Silhouette('glimpse', 0x14100c);
    this.fig = this.sil.group;
    this.fig.visible = false;
    scene.add(this.fig);
  }

  /** Where its head is while it stands somewhere (story/seen.ts), or null. */
  get where() { return this.up ? this.head.copy(this.fig.position).setY(this.fig.position.y + 0.6) : null; }
  private head = new THREE.Vector3();
  /** Degrees from the middle of the view to the figure, and whether it is in the view at all. */
  private sight(cam: THREE.PerspectiveCamera, at: THREE.Vector3) {
    const c = cam.getWorldPosition(new THREE.Vector3()), f = cam.getWorldDirection(new THREE.Vector3());
    const to = at.clone().setY(at.y + 0.6).sub(c), dist = to.length();
    const off = THREE.MathUtils.radToDeg(Math.acos(THREE.MathUtils.clamp(f.dot(to.normalize()), -1, 1)));
    const ndc = at.clone().setY(at.y + 0.6).project(cam);
    return { off, dist, inView: ndc.z < 1 && Math.abs(ndc.x) < 0.96 && Math.abs(ndc.y) < 0.96 };
  }

  update(dt: number, s: GlimpsesIn) {
    this.t += dt;
    if (this.up) {
      const sg = this.sight(s.cam, this.fig.position);
      if (this.t > 40 || !s.ok || sg.off < 25 || sg.dist < 40 || s.zoom > 0.25 || (s.flashlight && sg.off < 40 && sg.dist < 70)) this.hide();
      return;
    }
    if (!s.ok || !s.scene || this.count >= MAX) return;
    this.sil.wake();
    // rarely at first, more often towards morning: about every four minutes at three, two at five
    const day = ((s.clock % 86400) + 86400) % 86400, k = THREE.MathUtils.clamp((day - 2.5 * 3600) / (2.5 * 3600), 0, 1);
    this.wait -= dt * (0.6 + 1.4 * k);
    if (this.wait > 0) return;
    this.wait = rnd(20, 40);   // nowhere to stand just now: try again soon
    const at = this.pick(s);
    if (!at) return;
    this.fig.position.copy(at.p);
    const c = s.cam.getWorldPosition(new THREE.Vector3());
    this.fig.rotation.y = Math.atan2(c.x - at.p.x, c.z - at.p.z);   // facing the player
    this.stand(at, c);
    this.fig.visible = true; this.up = at.id; this.t = 0; this.count++;
    this.wait = rnd(150, 330);
  }

  /** A place 40 to 300 m off, in the view but well off its middle (the tests can name one). */
  private pick(s: GlimpsesIn, only?: string) {
    const out: { id: string; p: THREE.Vector3; rim: number }[] = [];
    for (const sp of SPOTS) if (sp.scene === s.scene && (!only || only === sp.id)) out.push({ id: sp.id, p: new THREE.Vector3(sp.x, s.ground(sp.x, sp.z), sp.z), rim: sp.rim });
    // on the highway, at the far end of the headlights on the shoulder
    if (s.truck && s.scene === 'road' && (!only || only === 'shoulder')) {
      const d = s.truck.dir.clone().setY(0).normalize(), side = new THREE.Vector3(-d.z, 0, d.x);
      const p = s.truck.pos.clone().addScaledVector(d, rnd(110, 160)).addScaledVector(side, (Math.random() < 0.5 ? -1 : 1) * 8.5);
      p.y = s.ground(p.x, p.z);
      out.push({ id: 'shoulder', p, rim: 0x1c1610 });
    }
    const ok = out.filter((o) => { const sg = this.sight(s.cam, o.p); return sg.dist > 40 && sg.dist < 300 && sg.inView && (only || sg.off > 30); });
    return ok.length ? ok[Math.floor(Math.random() * ok.length)] : null;
  }

  /** Put it at a named place now if it can stand there (the developer menu, the tests). */
  now(s: GlimpsesIn, id: string) {
    this.sil.wake();
    const at = this.pick(s, id);
    if (!at) return false;
    this.fig.position.copy(at.p);
    const c = s.cam.getWorldPosition(new THREE.Vector3());
    this.fig.rotation.y = Math.atan2(c.x - at.p.x, c.z - at.p.z);
    this.stand(at, c);
    this.fig.visible = true; this.up = at.id; this.t = 0; this.count++;
    return true;
  }
  /** How it stands there: far off it is soft at the edges, as in heat haze; nearer, upright or
   *  turned half aside, listening. The edge of light is the light of the place behind it. */
  private stand(at: { p: THREE.Vector3; rim: number }, cam: THREE.Vector3) {
    this.sil.pose(at.p.distanceTo(cam) > 150 ? 'far' : Math.random() < 0.65 ? 'stand' : 'turned');
    this.sil.rim(at.rim);
  }
  /** Whether it is one of round 14's outlines now (the tests). */
  get look() { return { painted: this.sil.painted }; }
  private hide() { this.fig.visible = false; this.up = null; }
  /** A new night, the title, a save opened. */
  reset() { this.hide(); this.count = 0; this.wait = rnd(120, 300); }
}
