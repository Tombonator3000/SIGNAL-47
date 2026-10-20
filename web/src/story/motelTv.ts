import * as THREE from 'three';
import type { AudioSys } from '../core/Audio';
import type { Interaction } from '../core/Interaction';
import type { MotelFront } from '../world/MotelFront';
import type { DocSpec } from '../ui/UI';
import { clockText } from './time';

// HENDELSER.md, event 10: the television in the office of Sierra Motor Court, on B. The station
// signed off hours ago, so all night the portable set on the shelves shows the test card, with
// its thin tone. Once in a night, while the player is in the office or at its door and the truck
// stands on the lot: the test card goes and the bare carrier is on for 47 seconds, a grey field
// and the hum; then one frame of the motel's lot seen from straight above, with the truck where
// it stands, and the test card again. Nothing could have stood up there with a camera. The frame
// is the scene itself, drawn from a camera over the lot the way the time away draws its frames
// (story/abduction.ts), and shown through the set's picture tube.
//
// It goes on in the world whoever is there; seen (the player still in or at the office when the
// frame comes), it is written down and kept with the town's part of the case (main.ts, `town`:
// 'ev:tv' and a mark on the map at the motel, on B), and it never comes again. Unseen, the night
// may bring it back later. At least ten minutes of the night after the last event, as in town
// (story/townEvents.ts).
//
// It is one of the 47-second events, and those come at the night's peak (story/curve.ts). While
// the player is in the office with the truck on the lot and nothing else stands in its way, it is
// waiting (`waiting`): the curve hurries its build to the peak and keeps the peak for it. The set
// shows the hurry (`unrest`, the curve's level): the test card begins to slip, a roll now and
// then, the tone wavering and the hum of the carrier coming up under it, until the card goes.
//
// Round 15 supplies the blank test card. The lot must be the actual scene: a painted fallback
// cannot say where this night's truck stood. A witnessed frame is kept with the case and can
// be looked at again, from the television or the notebook's album.

const ART = import.meta.glob<string>('../assets/art/motel/tv_*.jpg', { eager: true, import: 'default' });
const art = (name: string) => ART[`../assets/art/motel/${name}.jpg`] ?? null;

export interface MotelTvIn {
  /** In play, no panel, not in a cinematic, at night, and the story allows it. */
  ok: boolean;
  /** In SARO's area (the motel is part of it). */
  saro: boolean;
  /** In the motel's office, or on the walk at its door. */
  here: boolean;
  /** The truck stands on the motel's lot (so it is in the frame). */
  truckOnLot: boolean;
  cam: THREE.Camera;
  /** The game's clock, seconds since midnight. */
  clock: number;
  /** The night's curve (story/curve.ts) lets the carrier come now (47 s: a peak, one extra in a
   *  chapter); left out, it may. */
  may?: boolean;
  /** 0..1: the curve is hurrying to its peak for this (NightCurve.unrest); the test card slips. */
  unrest?: number;
}
export interface MotelTvDeps {
  motel: MotelFront;
  audio: AudioSys;
  inter: Interaction;
  renderer: THREE.WebGLRenderer;
  scene: THREE.Scene;
  /** The parked truck's actual world transform, including the heading restored from a save. */
  truck: () => THREE.Object3D | null;
  file: (doc: DocSpec) => void;
  /** The saved frame, including after Continue (main.ts keeps it with the case). */
  document: () => DocSpec | undefined;
  open: (doc: DocSpec) => void;
  /** The town's part of the case: the events and the marks for the notebook's map. */
  seen: Set<string>;
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  save: () => void;
}

const GAP = 10 * 60;            // seconds of the night after the last event
const CARRIER = 47;             // the bare carrier, seconds
const FRAME = 2.6;              // how long the one frame stays on the screen
const DWELL = 6;                // seconds in or at the office before it may start
const W = 320, H = 240;         // the pictures (4:3); the screen's canvas is smaller
/** The camera over the lot, in the terms the court was built in (MotelFront.ts): over the middle
 *  of the lot and the walk, 30 m up, looking straight down, west at the top of the picture. */
const OVER = new THREE.Vector3(-36, 30, 15), FOV = 40;

type Mode = 'card' | 'carrier' | 'frame';

export class MotelTv {
  /** What the set shows (the developer menu, the tests). */
  mode: Mode = 'card';
  /** The picture of the lot from the last time (the tests). */
  frame: HTMLCanvasElement | null = null;
  private t = 0;
  private dwell = 0;
  private notBefore = 0;
  private paintT = 0;
  private card: HTMLCanvasElement;
  private shot: HTMLCanvasElement;
  private tvAt = new THREE.Vector3();
  private frameClock = 0;
  private canWitness = false;
  private snd: { tone: GainNode; hum: GainNode; humK: number; hiss: GainNode; whine: GainNode; stop: () => void } | null = null;
  private light = new THREE.Color();
  /** The camera over the lot, and the light from straight above that the picture has (shoot()). */
  private cam = new THREE.PerspectiveCamera(FOV, 4 / 3, 1, 120);
  private top = new THREE.PointLight(0xe4ebf2, 700, 70, 1.6);
  /** What is in the frame and still to be made ready to draw (warm()); null: not looked for yet. */
  private toWarm: THREE.Object3D[] | null = null;
  private part = new THREE.Object3D();

  constructor(private d: MotelTvDeps) {
    this.card = testCard();
    // round 15's blank card, if it is in: drawn over the one made here once it has loaded
    const url = art('tv_testcard_blank');
    if (url) { const img = new Image(); img.onload = () => this.card.getContext('2d')!.drawImage(img, 0, 0, W, H); img.src = url; }
    this.shot = document.createElement('canvas'); this.shot.width = W; this.shot.height = H;
    const px = d.motel.proxies.officeTv;
    if (px) d.inter.add({ id: 'court:officeTv', object: px, range: 3.6,
      label: () => px.parent?.visible ? this.mode === 'card' && d.document() ? 'Television / recorded frame' : 'Television' : null,
      use: () => {
        const doc = d.document();
        if (this.mode === 'card' && doc) { d.open(doc); return; }
        if (this.mode === 'card' && d.seen.has('ev:tv')) {
          d.toast('The test card again. The notebook has the observation, but the picture from that earlier case is unavailable.', 5); return;
        }
        d.toast(this.mode === 'card' ? 'A portable set on the shelf, left on. The station signed off hours ago: the test card, and a thin tone.'
          : this.mode === 'carrier' ? 'Nothing on the screen but grey, and the hum.' : 'The lot, from straight above. The truck exactly where you left it.', 4);
      },
      examine: () => this.mode === 'card' ? d.document() ? 'The test card again. The single picture of the lot is kept in the notebook.'
        : d.seen.has('ev:tv') ? 'The test card again. The notebook has the observation, but the earlier picture is unavailable.' : 'Colour bars, and a thin tone. Nobody has turned it off.'
        : this.mode === 'carrier' ? 'A grey field. The hum of a carrier with nothing on it.' : 'The motel lot, straight down. Your truck, at the same angle you parked it. No camera could stand there.' });
    this.paint(0);
  }

  /** Something on the set other than the test card. */
  get busy() { return this.mode !== 'card'; }
  /** It could come now but for the night's curve: the player in the office or at its door, the
   *  truck on the lot, not seen tonight, its hours, ten minutes after the last event (story/curve.ts
   *  hurries for it). Set by update(). */
  waiting = false;
  /** How far the test card has slipped (0..1), and a roll of the picture under way. */
  private unrest = 0;
  private roll: { y: number; v: number } | null = null;
  /** How many times the picture has rolled tonight (the tests). */
  slips = 0;
  /** And the player is near enough to hear it (the music keeps out of its way). */
  get heard() { return this.busy && this.dist < 30; }
  private dist = Infinity;

  private last() {
    let t = -1e9;
    for (const k of this.d.seen) if (k.startsWith('mark:')) t = Math.max(t, Number(k.slice(k.indexOf('@') + 1)));
    return t;
  }

  update(dt: number, s: MotelTvIn) {
    const day = ((s.clock % 86400) + 86400) % 86400;
    this.d.motel.tv.group.getWorldPosition(this.tvAt);
    const cam = s.cam.getWorldPosition(new THREE.Vector3()), dist = s.saro ? cam.distanceTo(this.tvAt) : Infinity;
    this.dist = dist;
    this.dwell = s.ok && s.here ? this.dwell + dt : 0;
    const could = this.mode === 'card' && s.ok && s.saro && s.here && !this.d.seen.has('ev:tv') && s.truckOnLot && s.clock >= this.notBefore && s.clock - this.last() >= GAP
      && day >= 2 * 3600 + 30 * 60 && day < 5 * 3600 + 5 * 60;
    this.waiting = could;
    this.unrest = could ? THREE.MathUtils.clamp(s.unrest ?? 0, 0, 1) : 0;
    switch (this.mode) {
      case 'card':
        if (could && s.may !== false && this.dwell > DWELL) {
          this.carrier();
          this.d.toast('On the television the test card is gone: a blank grey screen, and the hum of a carrier with nothing on it.', 5.5);
        }
        break;
      case 'carrier':
        if (!s.saro) { this.missed(s.clock); break; }
        // A paper, conversation or cinematic must not consume the only chance to see it.
        if (!s.ok) break;
        this.warm();
        if ((this.t += dt) >= CARRIER) this.showFrame(s, dist);
        break;
      case 'frame':
        if (!s.saro) { this.missed(s.clock); break; }
        if (!s.ok) break;
        this.witness(s, dist);
        if ((this.t += dt) >= FRAME) {
          if (!this.d.seen.has('ev:tv')) this.notBefore = s.clock + GAP;
          this.mode = 'card'; this.t = 0; this.click();
        }
        break;
    }
    // the picture, while someone could see it
    if (dist < 40 && (this.paintT += dt) >= 1 / 15) { this.paintT = 0; this.paint(performance.now() / 1000); }
    this.sound(dist);
    // its light on the office: the bars a cool white, the carrier a dim grey
    const k = this.mode === 'card' ? 1.3 : this.mode === 'carrier' ? 0.75 : 1.6;
    this.light.setRGB(this.mode === 'carrier' ? 0.62 : 0.66, this.mode === 'carrier' ? 0.66 : 0.72, this.mode === 'carrier' ? 0.7 : 0.92);
    this.d.motel.setTvLight(s.saro ? k * (0.94 + 0.06 * Math.random()) : 0, this.light);
  }

  private carrier() { this.mode = 'carrier'; this.t = 0; this.toWarm = null; this.canWitness = false; this.roll = null; this.click(); }

  private missed(clock: number) {
    this.mode = 'card'; this.t = 0; this.dwell = 0; this.notBefore = clock + GAP;
    this.toWarm = null; this.canWitness = false;
  }

  private showFrame(s: MotelTvIn, dist: number) {
    this.mode = 'frame'; this.t = 0;
    this.click();
    // the lot from straight above, as it is now; the scene can only be drawn in SARO's area
    const truck = this.d.truck();
    this.canWitness = s.saro && s.truckOnLot && !!truck && shown(truck);
    this.frame = this.canWitness ? this.shoot() : null;
    this.frameClock = s.clock;
    if (!this.frame) { this.missed(s.clock); return; }
    this.witness(s, dist);
  }

  private witness(s: MotelTvIn, dist: number) {
    if (this.d.seen.has('ev:tv') || !this.frame || !this.canWitness || !s.ok || !s.saro || !(s.here || dist < 9) || !this.inView(s.cam)) return;
    const t = clockText(this.frameClock, false);
    let image: string;
    try { image = this.frame.toDataURL('image/jpeg', 0.88); }
    catch { return; } // do not spend an event whose actual frame cannot be kept
    this.d.file(frameDocument(image, t));
    this.d.seen.add('ev:tv'); this.d.seen.add(`mark:tv@${Math.round(this.frameClock)}`);
    this.d.toast('For a moment the screen shows the motel\'s lot from straight above: the office roof, the sign, the truck where you left it. Nothing up there could hold a camera. Then the test card again.', 7);
    this.d.note(`${t}. Motel office TV. One frame of the lot from above. My truck in it.`);
    this.d.save();
  }

  /** The screen must actually be in view, and seen from its front. Being in the office with
   *  your back to the set is not an observation; turning during the held frame still counts. */
  private inView(cam: THREE.Camera) {
    cam.updateMatrixWorld(true);
    const screen = this.d.motel.tv.group.localToWorld(new THREE.Vector3(-0.045, 0.18, 0.1815));
    const eye = cam.getWorldPosition(new THREE.Vector3()), towards = screen.clone().sub(eye);
    const front = new THREE.Vector3(0, 0, 1).transformDirection(this.d.motel.tv.group.matrixWorld);
    if (towards.dot(front) >= 0) return false;
    const ndc = screen.clone().project(cam);
    if (!(ndc.z >= -1 && ndc.z <= 1 && Math.abs(ndc.x) < 0.9 && Math.abs(ndc.y) < 0.9)) return false;
    // Projection alone also sees through the office walls. Ignore hit proxies and clear
    // glass, but require a clear sight line to the face of the picture tube.
    const ray = new THREE.Raycaster(eye, towards.clone().normalize(), 0.02, Math.max(0.02, towards.length() - 0.08));
    return !ray.intersectObjects(this.d.scene.children, true).some((hit) => {
      const mesh = hit.object as THREE.Mesh;
      if (!mesh.isMesh || !shown(mesh)) return false;
      const materials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
      const mat = materials[hit.face?.materialIndex ?? 0];
      return mat?.visible && (!mat.transparent || mat.opacity > 0.5);
    });
  }

  /** The camera over the lot as the canvas is now, and the light over it put in the scene (taken
   *  out again by the caller). Returns the height of the picture on the canvas. */
  private aim() {
    const src = this.d.renderer.domElement, motel = this.d.motel, cam = this.cam;
    // the picture is the middle 4:3 of the canvas, and that must take in the same ground on any screen
    const ch = Math.min(src.height, src.width * 3 / 4);
    cam.aspect = src.width / src.height;
    cam.fov = 2 * Math.atan(Math.tan(FOV * Math.PI / 360) * src.height / ch) * 180 / Math.PI;
    const over = OVER.clone(), truck = this.d.truck();
    if (truck) {
      const bounds = new THREE.Box3().setFromObject(truck), a = motel.at;
      // Keep the established close view for the normal parking place. A freely parked or
      // restored truck can stand by any room, so extend the same shot to include it as well.
      const half = OVER.y * Math.tan(FOV * Math.PI / 360);
      if (bounds.min.x < a.x + OVER.x - half + 2 || bounds.max.x > a.x + OVER.x + half - 2
        || bounds.min.z < a.z + OVER.z - half * 4 / 3 + 2 || bounds.max.z > a.z + OVER.z + half * 4 / 3 - 2) {
        const x0 = Math.min(-46, bounds.min.x - a.x - 2), x1 = Math.max(-27, bounds.max.x - a.x + 2);
        const z0 = Math.min(1.5, bounds.min.z - a.z - 2), z1 = Math.max(30, bounds.max.z - a.z + 2);
        over.x = (x0 + x1) / 2; over.z = (z0 + z1) / 2;
        over.y = Math.max(OVER.y, (x1 - x0) / (2 * Math.tan(FOV * Math.PI / 360)), (z1 - z0) / (2 * Math.tan(FOV * Math.PI / 360) * 4 / 3)) + 3;
      }
    }
    const at = motel.group.localToWorld(over), below = at.clone().setY(motel.floorY);
    cam.position.copy(at); cam.up.set(-1, 0, 0); cam.lookAt(below);
    cam.updateProjectionMatrix(); cam.updateMatrixWorld(true);
    // a light from straight above for this one picture, as the frames of the time away have
    // theirs (the lot by its two lamps alone is a black square seen from up there)
    this.top.position.copy(at).setY(motel.floorY + 22);
    this.d.scene.add(this.top); this.top.updateMatrixWorld(true);
    return ch;
  }

  /** The frame is drawn with one light more than the night has, and straight onto the canvas,
   *  and three.js puts both in every shader program: everything in it wants programs of its own.
   *  Made when the frame comes, they would hold the game still for a second or more on a phone
   *  (core/Flashlight.ts has the same about lights). So they are made while the carrier is on, a
   *  few things a frame: what the camera over the lot sees, as three.js would draw it (shown, in
   *  its layers, in view). The frame then finds them made. */
  private warm() {
    const r = this.d.renderer, scene = this.d.scene;
    const target = r.getRenderTarget();
    if (this.toWarm && !this.toWarm.length) return;
    try {
      this.aim();
      const cam = this.cam;
      if (!this.toWarm) {
        const fr = new THREE.Frustum().setFromProjectionMatrix(new THREE.Matrix4().multiplyMatrices(cam.projectionMatrix, cam.matrixWorldInverse));
        const list: THREE.Object3D[] = [];
        scene.traverseVisible((o) => {
          const m = o as THREE.Mesh, sprite = (o as THREE.Sprite).isSprite;
          if (!(m.isMesh || (o as THREE.Points).isPoints || (o as THREE.Line).isLine || sprite) || !m.material || !o.layers.test(cam.layers)) return;
          if (!o.frustumCulled || (sprite ? fr.intersectsSprite(o as THREE.Sprite) : fr.intersectsObject(o))) list.push(o);
        });
        this.toWarm = list;
        return;
      }
      // three.js makes ready what `traverse` gives it, lit by the lights of the scene it is to be
      // drawn in (with the one over the lot)
      const batch = this.toWarm.splice(0, 16);
      this.part.traverse = (f) => { for (const o of batch) f(o); };
      this.part.traverseVisible = () => {};
      r.setRenderTarget(null); r.compile(this.part, cam, scene);
    } catch { this.toWarm = []; /* then they are made with the frame, as before */ }
    finally { this.top.removeFromParent(); r.setRenderTarget(target); }
  }

  /** The lot from a camera over it, drawn at once into a picture the size of the set's (main.ts
   *  draws the next frame over the canvas as usual). */
  private shoot(): HTMLCanvasElement | null {
    const r = this.d.renderer, target = r.getRenderTarget(), viewport = r.getViewport(new THREE.Vector4());
    const scissor = r.getScissor(new THREE.Vector4()), clipped = r.getScissorTest();
    try {
      const src = r.domElement, size = r.getSize(new THREE.Vector2());
      const ch = this.aim();
      r.setRenderTarget(null); r.setViewport(0, 0, size.x, size.y); r.setScissorTest(false);
      r.render(this.d.scene, this.cam);
      const g = this.shot.getContext('2d')!, cw = ch * 4 / 3;
      // a picture off the air: less colour, harder, and a little brighter than the night is
      g.filter = 'saturate(0.6) contrast(1.3) brightness(1.15)';
      g.drawImage(src, (src.width - cw) / 2, (src.height - ch) / 2, cw, ch, 0, 0, W, H);
      g.filter = 'none';
      return this.shot;
    } catch { return null; }
    finally {
      this.top.removeFromParent(); r.setRenderTarget(target);
      r.setViewport(viewport); r.setScissor(scissor); r.setScissorTest(clipped);
    }
  }

  // ---------- the picture tube ----------
  private paint(t: number) {
    const c = this.d.motel.tv.canvas, g = c.getContext('2d')!, w = c.width, h = c.height;
    if (this.mode === 'carrier') {
      // a flat grey raster, the hum bar rolling slowly up it
      g.fillStyle = '#3a3d40'; g.fillRect(0, 0, w, h);
      const y = h - ((t * 11) % (h + 60));
      const bar = g.createLinearGradient(0, y - 30, 0, y + 30); bar.addColorStop(0, 'rgba(0,0,0,0)'); bar.addColorStop(0.5, 'rgba(0,0,0,.28)'); bar.addColorStop(1, 'rgba(0,0,0,0)');
      g.fillStyle = bar; g.fillRect(0, y - 30, w, 60);
    } else {
      // a little overscan, and the picture jitters by a line now and then
      const src = this.mode === 'frame' && this.frame ? this.frame : this.card, j = Math.random() < 0.08 ? 1 : 0;
      const u = this.mode === 'card' ? this.unrest : 0;
      // the curve hurrying for it (story/curve.ts): the card slips, the vertical hold losing it
      // now and then, the picture rolling down through the frame once with the black bar between
      if (u > 0.08 && !this.roll && Math.random() < 0.012 + 0.09 * u * u) { this.roll = { y: 0, v: h * (0.5 + 1.6 * u) }; this.slips++; }
      if (this.roll) {
        const r = this.roll, y = Math.round(r.y), bar = Math.max(3, Math.round(h * 0.07));
        g.fillStyle = '#000'; g.fillRect(0, 0, w, h);
        g.drawImage(src, 6, 5, W - 12, H - 10, 0, y, w, h);
        g.drawImage(src, 6, 5, W - 12, H - 10, 0, y - h - bar, w, h);
        g.fillStyle = '#050505'; g.fillRect(0, y - bar, w, bar);
        if ((r.y += r.v / 15) >= h + bar) this.roll = null;
      } else g.drawImage(src, 6, 5, W - 12, H - 10, 0, j, w, h);
      // and the lines tearing sideways for a moment
      if (u > 0.2 && Math.random() < 0.25 * u) {
        const y0 = Math.floor(Math.random() * h), n = 2 + Math.floor(Math.random() * h * 0.12 * u);
        g.drawImage(c, 0, y0, w, n, Math.round((Math.random() - 0.3) * w * 0.08 * u), y0, w, n);
      }
    }
    // the tube: scan lines, a little snow, the dark corners, the brightness breathing
    g.fillStyle = 'rgba(0,0,0,.24)';
    for (let y = 0; y < h; y += 2) g.fillRect(0, y, w, 1);
    for (let i = 0; i < (this.mode === 'carrier' ? 260 : 90); i++) { g.fillStyle = `rgba(255,255,255,${Math.random() * 0.12})`; g.fillRect(Math.random() * w, Math.random() * h, 1, 1); }
    const v = g.createRadialGradient(w / 2, h / 2, h * 0.32, w / 2, h / 2, w * 0.66); v.addColorStop(0, 'rgba(0,0,0,0)'); v.addColorStop(1, 'rgba(0,0,0,.6)');
    g.fillStyle = v; g.fillRect(0, 0, w, h);
    g.fillStyle = `rgba(0,0,0,${0.04 + 0.04 * Math.random()})`; g.fillRect(0, 0, w, h);
    this.d.motel.tv.tex.needsUpdate = true;
  }

  // ---------- the set's little speaker ----------
  private sound(dist: number) {
    const a = this.d.audio, ctx = a.ctx;
    if (!ctx) return;
    if (dist > 30) { if (this.snd) this.silence(); return; }
    if (!this.snd) {
      const spk = a.spatial(this.tvAt, 0.9), noise = a.noiseBuffer();
      if (!spk || !noise) return;
      const gain = (v = 0) => { const g = ctx.createGain(); g.gain.value = v; g.connect(spk); return g; };
      // the tone with the test card, through the small speaker
      const tone = gain(), tOsc = ctx.createOscillator(); tOsc.frequency.value = 1000;
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1300; bp.Q.value = 0.5;
      tOsc.connect(bp); bp.connect(tone);
      // the carrier's hum: the mains buzz in a set with nothing coming in. With the bought sounds
      // it is a recorded transformer hum (an appliance's 50 Hz, played 1.2 times as fast: 60 Hz,
      // with its uneven harmonics), as loud through the low-pass as the sawtooth (x1.8, measured)
      const hum = gain(), hb = a.lic('tv_hum');
      let hOsc: AudioScheduledSourceNode;
      if (hb) { const s = ctx.createBufferSource(); s.buffer = hb; s.loop = true; hOsc = s; }
      else { const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.value = 60; hOsc = o; }
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 380; hOsc.connect(lp); lp.connect(hum);
      const hiss = gain(), n = ctx.createBufferSource(); n.buffer = noise; n.loop = true;
      const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 900; n.connect(hp); hp.connect(hiss);
      // and the line whistle of the tube, as long as it is on
      const whine = gain(), wOsc = ctx.createOscillator(); wOsc.frequency.value = 15734; wOsc.connect(whine);
      for (const o of [tOsc, hOsc, n, wOsc]) o.start();
      this.snd = { tone, hum, humK: hb ? 1.8 : 1, hiss, whine, stop: () => { for (const o of [tOsc, hOsc, n, wOsc]) { try { o.stop(); } catch { /* stopped */ } } spk.disconnect(); } };
    }
    const m = this.mode, now = ctx.currentTime, set = (g: GainNode, v: number) => g.gain.setTargetAtTime(v, now, 0.04);
    // slipping: the tone wavers and drops out for a moment, the carrier's hum comes up under it
    const u = m === 'card' ? this.unrest : 0, waver = u > 0 ? 1 - u * (0.35 + 0.65 * Math.random() * Math.random()) : 1;
    set(this.snd.tone, m === 'card' ? 0.03 * (this.roll ? 0.3 : waver) : 0);
    set(this.snd.hum, m === 'carrier' ? 0.11 * this.snd.humK : 0.05 * u * u * this.snd.humK);
    set(this.snd.hiss, m === 'carrier' ? 0.014 : m === 'card' ? 0.006 : 0);
    set(this.snd.whine, m === 'frame' ? 0 : 0.0016);
  }
  private click() { this.d.audio.play('click', { at: this.tvAt, gain: 0.22, rate: 1.3, lp: 2400 }); }
  /** The set's sound off (leaving the night). */
  silence() {
    const s = this.snd; if (!s) return;
    this.snd = null;
    const ctx = this.d.audio.ctx;
    if (ctx) for (const g of [s.tone, s.hum, s.hiss, s.whine]) g.gain.setTargetAtTime(0, ctx.currentTime, 0.03);
    setTimeout(() => s.stop(), 300);
  }

  /** A new night or a case opened: the test card, and whether it has been seen in it. */
  reset() {
    this.mode = 'card'; this.t = 0; this.dwell = 0; this.notBefore = 0; this.frame = null; this.toWarm = null; this.canWitness = false;
    this.waiting = false; this.unrest = 0; this.roll = null;
    this.silence();
    this.paint(0);
  }
  /** Restore the actual saved image after the chapter has restored its ordinary papers. Old
   *  cases can have the finding without an image: keep the finding, never substitute artwork. */
  refile(url?: string) {
    if (!url || !this.d.seen.has('ev:tv')) return;
    const mark = [...this.d.seen].find((k) => k.startsWith('mark:tv@'));
    const clock = Number(mark?.slice('mark:tv@'.length));
    this.d.file(frameDocument(url, Number.isFinite(clock) ? clockText(clock, false) : 'time unrecorded'));
  }
  /** The carrier now (the developer menu): the frame comes 47 s later as it would. */
  now() { if (this.mode === 'card') this.carrier(); }
}

function shown(o: THREE.Object3D) { for (let p: THREE.Object3D | null = o; p; p = p.parent) if (!p.visible) return false; return true; }

function frameDocument(image: string, t: string): DocSpec {
  return { id: 'motel-tv-frame', kind: 'photo', title: `TV FRAME / Sierra Motor Court (${t})`, page: '', image,
    transcript: `Motel office television, ${t}. After 47 seconds of bare carrier, one fixed frame of the lot from directly above. The office roof, the sign, and my truck exactly where I parked it, at the same angle. No camera could stand there. Then the test card again.` };
}

function blank() { const c = document.createElement('canvas'); c.width = W; c.height = H; return c; }
/** The test card: colour bars, as stations put them up at sign-off; no station name. */
function testCard() {
  const c = blank(), g = c.getContext('2d')!, bw = W / 7;
  const top = ['#c0c0c0', '#c0c000', '#00c0c0', '#00c000', '#c000c0', '#c00000', '#0000c0'];
  const mid = ['#0000c0', '#131313', '#c000c0', '#131313', '#00c0c0', '#131313', '#c0c0c0'];
  top.forEach((col, i) => { g.fillStyle = col; g.fillRect(Math.floor(i * bw), 0, Math.ceil(bw), H * 0.67); });
  mid.forEach((col, i) => { g.fillStyle = col; g.fillRect(Math.floor(i * bw), H * 0.67, Math.ceil(bw), H * 0.08); });
  // the bottom row: -I, white, +Q, black, the three narrow bars to set the black by, black
  const b = H * 0.75, q = bw * 5 / 4;
  [['#00214c', 0], ['#ffffff', 1], ['#32006a', 2], ['#131313', 3]].forEach(([col, i]) => { g.fillStyle = col as string; g.fillRect(Math.floor((i as number) * q), b, Math.ceil(q), H - b); });
  ['#090909', '#131313', '#1d1d1d'].forEach((col, i) => { g.fillStyle = col; g.fillRect(Math.floor(5 * bw + i * bw / 3), b, Math.ceil(bw / 3), H - b); });
  g.fillStyle = '#131313'; g.fillRect(Math.floor(6 * bw), b, Math.ceil(bw), H - b);
  return c;
}
