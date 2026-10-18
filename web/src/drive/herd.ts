import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { floodlit, cyl, type FloodSet } from '../world/kit';
import type { GlowPoints } from '../world/glow';
import { colored, circle, type Build } from './roadProps';
import { smooth, surfaceY } from './roadTerrain';
import { HERD } from './roadShape';

// The cattle in the pasture south of Sierra Motor Court, west of the highway (HENDELSER.md,
// event 2; the event itself is story/pasture.ts). A small herd of Herefords, red with white
// faces, inside the pasture's fences (roadTerrain.PASTURE), by the highway's fence just south of
// the survey track: the headlights of a truck coming down past the motel find them, they are in
// the beam as it turns onto the track, and ahead on the right coming back along it (north of the
// track the motel's rooms stood between them and the highway). Built by the road's builders, so the stretch
// SARO's area draws (drive/corridors.ts) has the very same herd as the road's (RoadArea.ts), lit
// by that area's floods; story/pasture.ts sets both to the same state.
//
// Grazing, every cow has its own heading and its head goes down and up. While the event is on,
// every one faces az 026 with its head up, and nothing moves at all, also in the headlights.
// Their eyes shine back green in the beam then (the only time they are seen to).
//
// Draw calls: the bodies 1, the heads 1 (both instanced), the ring of frost 1 when it is there;
// the eyes are points in the area's glow (no call of their own), the stock tank merges with the
// area's statics.

/** Road-local (x, z) of each cow, and the way it grazes (radians from north, clockwise). */
const COWS: [number, number, number][] = [
  [-27.0, 538.5, 2.3], [-33.5, 542.0, -0.7], [-26.5, 549.5, 1.1], [-40.5, 547.5, 2.9], [-30.5, 557.5, -2.2],
  [-46.0, 556.0, 0.6], [-38.0, 563.5, -1.6], [-52.5, 566.5, 1.9], [-34.0, 575.0, -2.8],
];
/** The one that is not there the second time (near the fence, so its ring is in the headlights). */
const MISSING = HERD.missing;
/** How near the truck can come to a cow (metres, from where it stands). */
const COW_R = 1.15;
/** Facing az 026: the direction on the ground (x east, z south) and the heading for rotation.y. */
const AZ = 26 * Math.PI / 180;
const FACING = new THREE.Vector3(Math.sin(AZ), 0, -Math.cos(AZ));

// colours (linear, as the other vertex colours of the road)
const RED = [0.3, 0.11, 0.055], WHITE = [0.74, 0.7, 0.62], HOOF = [0.07, 0.06, 0.05], NOSE = [0.46, 0.33, 0.31];
/** Where the head turns on the body (the neck's root), and where the eyes are on the head. */
const NECK = new THREE.Vector3(0, 1.12, -0.74), EYE = [new THREE.Vector3(-0.17, 0.11, -0.66), new THREE.Vector3(0.17, 0.11, -0.66)];

function bodyGeometry() {
  const B = (w: number, h: number, d: number, x: number, y: number, z: number, c: number[]) => colored(new THREE.BoxGeometry(w, h, d).translate(x, y, z), c);
  // the barrel along the cow, a little deeper at the chest; the white underline below it
  const barrel = colored(new THREE.CylinderGeometry(0.39, 0.44, 1.72, 7, 1).rotateX(Math.PI / 2).scale(1, 0.92, 1).translate(0, 1.04, 0.02), RED);
  const parts = [barrel, B(0.46, 0.12, 0.95, 0, 0.67, 0.05, WHITE), B(0.34, 0.24, 0.3, 0, 0.74, -0.72, WHITE)];
  for (const [x, z] of [[-0.2, -0.6], [0.2, -0.6], [-0.2, 0.62], [0.2, 0.62]]) parts.push(B(0.14, 0.7, 0.16, x, 0.39, z, RED), B(0.15, 0.08, 0.17, x, 0.04, z, HOOF));
  // the tail, hanging, with its white switch
  parts.push(colored(new THREE.BoxGeometry(0.05, 0.62, 0.05).translate(0, -0.31, 0).rotateX(-0.12).translate(0, 1.26, 0.87), RED),
    B(0.08, 0.16, 0.08, 0, 0.62, 0.95, WHITE));
  return mergeGeometries(parts)!;
}
function headGeometry() {
  const B = (w: number, h: number, d: number, x: number, y: number, z: number, c: number[]) => colored(new THREE.BoxGeometry(w, h, d).translate(x, y, z), c);
  // from the neck's root (the origin here) forward: the neck, the white crest on it, the white
  // face, the muzzle, the ears
  return mergeGeometries([
    B(0.34, 0.44, 0.52, 0, -0.04, -0.2, RED), B(0.1, 0.05, 0.48, 0, 0.19, -0.2, WHITE),
    B(0.32, 0.36, 0.44, 0, 0.02, -0.6, WHITE), B(0.25, 0.21, 0.17, 0, -0.08, -0.88, NOSE),
    B(0.16, 0.05, 0.08, -0.24, 0.12, -0.46, RED), B(0.16, 0.05, 0.08, 0.24, 0.12, -0.46, RED),
  ])!;
}

/** The truck's headlight beam as these materials see it: the lamps (xyz, w how much they are on)
 *  and the way they point, world terms. */
type Beam = { pos: { value: THREE.Vector4 }; dir: { value: THREE.Vector3 } };
/** A floodlit material that is also lit by the headlight beam itself, not only by its pools on
 *  the road: the herd stands too far off the highway for those (kit.floodlit; the signs' own
 *  shine is roadProps.retro). flat: lit as if facing the lamps (the ring of frost on the ground). */
function beamLit<T extends THREE.MeshStandardMaterial>(m: T, set: FloodSet, beam: Beam, flat = false): T {
  floodlit(m, 0.012, set);
  const flood = m.onBeforeCompile;
  m.onBeforeCompile = (sh, r) => {
    flood.call(m, sh, r);
    sh.uniforms.uBeamPos = beam.pos; sh.uniforms.uBeamDir = beam.dir;
    sh.fragmentShader = sh.fragmentShader
      .replace('uniform float uFloodScale;', 'uniform float uFloodScale; uniform vec4 uBeamPos; uniform vec3 uBeamDir;')
      .replace('totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;', `totalEmissiveRadiance += fAcc * diffuseColor.rgb * uFloodScale;
        vec3 bL = uBeamPos.xyz - vFWorld; float bD = length(bL); bL /= max(bD, 1e-3);
        float bCone = smoothstep(0.68, 0.96, dot(-bL, uBeamDir));
        float bNdl = ${flat ? '1.0' : 'max(dot(normalize(vFNormal), bL), 0.0) * 0.8 + 0.2'};
        totalEmissiveRadiance += diffuseColor.rgb * uBeamPos.w * bCone * bNdl * 1.4 / (1.0 + bD * bD / 2500.0);`);
  };
  m.customProgramCacheKey = () => 'flood' + set.key + 0.012 + 'beam' + (flat ? 'f' : '');
  return m;
}
/** The same light on the CPU, at a world point (whether a cow is lit, story/pasture.ts). */
function beamAt(beam: Beam, p: THREE.Vector3) {
  const s = beam.pos.value, d = beam.dir.value;
  const lx = s.x - p.x, ly = s.y - p.y, lz = s.z - p.z, dist = Math.hypot(lx, ly, lz) || 1e-3;
  const cone = smooth(0.68, 0.96, -(lx * d.x + ly * d.y + lz * d.z) / dist);
  return s.w * cone * 1.4 / (1 + dist * dist / 2500);
}

const _m = new THREE.Matrix4(), _h = new THREE.Matrix4(), _q = new THREE.Quaternion(), _e = new THREE.Euler(), _v = new THREE.Vector3(), _one = new THREE.Vector3(1, 1, 1);
const _zero = new THREE.Matrix4().makeScale(0, 0, 0);

export class Herd {
  /** In road-local terms, a child of the area's root (the road's group, or SARO's stretch). */
  readonly group = new THREE.Group();
  /** Still, all facing 026 (the event is on); otherwise grazing. */
  still = false;
  /** Which cow is not there (-1: all are), and whether the ring of frost lies where it stood. */
  missing = -1;
  private bodies: THREE.InstancedMesh;
  private heads: THREE.InstancedMesh;
  private ring: THREE.Mesh;
  private beam: Beam = { pos: { value: new THREE.Vector4(0, -999, 0, 0) }, dir: { value: new THREE.Vector3(0, 0, -1) } };
  private glow: GlowPoints;
  private eyes: number[] = [];
  /** The eyes of the still pose, road-local (two to a cow). */
  private eyeAt: THREE.Vector3[] = [];
  private eyesLit = false;
  /** The missing one's place in the areas' lists of what stops the truck (obstacle()). */
  private gone: { kind: 'circle'; x: number; z: number; r: number }[] = [];
  private ys: number[];
  private dirty = true;
  private lastT = 0;

  /** area: the area whose builders made it (World.ts keeps one per area). */
  constructor(b: Build, set: FloodSet, readonly area: 'saro' | 'road') {
    this.group.name = 'herd';
    const mat = beamLit(new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.95, flatShading: true }), set, this.beam);
    this.bodies = new THREE.InstancedMesh(bodyGeometry(), mat, COWS.length);
    this.heads = new THREE.InstancedMesh(headGeometry(), mat, COWS.length);
    this.bodies.name = 'herd:bodies'; this.heads.name = 'herd:heads';
    this.ys = COWS.map(([x, z]) => surfaceY(x, z));
    // the eyes of the still pose, dark until the beam is on them
    this.glow = b.glow;
    this.still = true;
    COWS.forEach((_, i) => {
      this.pose(i, 0);
      for (const e of EYE) { const p = e.clone().applyMatrix4(_h); this.eyeAt.push(p); this.eyes.push(b.glow.add(p.x, p.y, p.z, 0.3, 0x000000)); }
    });
    this.still = false;
    this.place(0);
    this.bodies.computeBoundingSphere(); this.heads.computeBoundingSphere();
    // the ring of flattened grass with frost on it where the missing one stood, draped on the ground
    const rg = new THREE.RingGeometry(0.95, 2.05, 36, 2); rg.rotateX(-Math.PI / 2);
    const [mx, mz] = COWS[MISSING], my = this.ys[MISSING];
    const p = rg.attributes.position as THREE.BufferAttribute;
    for (let i = 0; i < p.count; i++) p.setY(i, surfaceY(mx + p.getX(i), mz + p.getZ(i)) - my + 0.05);
    rg.computeVertexNormals();
    this.ring = new THREE.Mesh(rg, beamLit(new THREE.MeshStandardMaterial({ color: 0xe6eef2, emissive: 0x0d1114, roughness: 0.6, transparent: true, opacity: 0.62,
      depthWrite: false, polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -2 }), set, this.beam, true));
    this.ring.position.set(mx, my, mz); this.ring.name = 'herd:frost'; this.ring.visible = false;
    this.group.add(this.bodies, this.heads, this.ring);
    b.root.add(this.group);
    // off the survey track the truck can be driven into the pasture: the cows stop it, as the
    // posts and the rocks do; the one that will not be there only while it is (obstacle())
    COWS.forEach(([x, z], i) => { if (i !== MISSING) circle(b, x, z, COW_R); });
    // a galvanised stock tank among them, the float box on its rim; it stops the truck too
    const tx = -45.0, tz = 537.5, ty = surfaceY(tx, tz);
    circle(b, tx, tz, 1.3);
    cyl(b.statics, 1.25, 1.25, 0.62, b.m.steel, tx, ty + 0.29, tz, 14);
    cyl(b.statics, 1.18, 1.18, 0.02, b.m.dark, tx, ty + 0.55, tz, 14);
    cyl(b.statics, 0.16, 0.16, 0.3, b.m.steel, tx + 1.05, ty + 0.66, tz - 0.4, 6);
  }

  /** Cow i's body (in _m) and head (in _h), road-local: grazing at time t, or still facing 026. */
  private pose(i: number, t: number) {
    const [x, z, graze] = COWS[i];
    let yaw: number, pitch: number, turn: number;
    if (this.still) { yaw = -AZ; pitch = 0.06; turn = 0; }
    else {
      // each grazes on its own clock: head down, a little to and fro, now and then up for a while
      const c = (t / (13 + i * 1.9) + i * 0.37) % 1, up = smooth(0, 0.07, c) * (1 - smooth(0.3, 0.37, c));
      yaw = -graze + 0.09 * Math.sin(t * 0.045 + i * 1.7);
      pitch = -1.22 * (1 - up) + 0.05 * up + 0.05 * Math.sin(t * 1.3 + i) * (1 - up);
      turn = 0.28 * Math.sin(t * 0.29 + i * 2.1) * (1 - up) + 0.35 * Math.sin(t * 0.21 + i) * up;
    }
    _m.compose(_v.set(x, this.ys[i], z), _q.setFromEuler(_e.set(0, yaw, 0)), _one);
    _h.compose(NECK, _q.setFromEuler(_e.set(pitch, turn, 0, 'YXZ')), _one).premultiply(_m);
  }
  private place(t: number) {
    for (let i = 0; i < COWS.length; i++) {
      if (i === this.missing) { this.bodies.setMatrixAt(i, _zero); this.heads.setMatrixAt(i, _zero); continue; }
      this.pose(i, t);
      this.bodies.setMatrixAt(i, _m); this.heads.setMatrixAt(i, _h);
    }
    this.bodies.instanceMatrix.needsUpdate = true; this.heads.instanceMatrix.needsUpdate = true;
  }

  /** The state, from story/pasture.ts (the same for every area's herd). */
  set(still: boolean, missing: number, ring: boolean) {
    if (still !== this.still || missing !== this.missing) { this.still = still; this.missing = missing; this.place(this.lastT); }
    this.ring.visible = ring;
    for (const o of this.gone) o.r = missing === MISSING ? 0 : COW_R;
  }
  /** The one that goes, in an area's own terms (origin: where the road's frame is in it), for
   *  its list of what stops the truck: in the way while it stands there, not after (set()). Only
   *  the road's area needs it; SARO's stretch keeps the truck between the highway's fences. */
  obstacle(origin: { x: number; z: number }) {
    const [x, z] = COWS[MISSING], o = { kind: 'circle' as const, x: x + origin.x, z: z + origin.z, r: this.missing === MISSING ? 0 : COW_R };
    this.gone.push(o);
    return o;
  }

  /** Every frame while its area is shown. headlights: the truck's lamps in this area's floods
   *  (their first and last pools give the beam, as roadProps.headlightGlints), null when it is
   *  not driven here or its lights are off. */
  update(t: number, headlights: { set: FloodSet; slots: readonly number[] } | null) {
    // grazing they move; still, nothing does
    this.lastT = t;
    if (!this.still || this.dirty) { this.place(t); this.dirty = false; }
    const b = this.beam.pos.value, d = this.beam.dir.value;
    if (headlights) {
      const { set, slots } = headlights, f0 = set.pos[slots[0]], f2 = set.pos[slots[2]];
      const on = f0.w > 0 ? set.col[slots[0]].r : 0;
      d.set(f2.x - f0.x, 0, f2.z - f0.z);
      if (d.lengthSq() > 1) d.normalize(); else d.set(0, 0, -1);
      b.set(f0.x - d.x * 4, f0.y, f0.z - d.z * 4, on);
    } else b.w = 0;
    // their eyes in the beam, only while they stand still facing it
    const want = this.still && b.w > 0;
    if (!want && !this.eyesLit) return;
    this.eyesLit = want;
    const col = this.glow.points.geometry.getAttribute('aCol') as THREE.BufferAttribute;
    this.group.updateWorldMatrix(true, false);
    COWS.forEach((_, i) => {
      for (let e = 0; e < 2; e++) {
        let k = 0;
        if (want && i !== this.missing) {
          const p = _v.copy(this.eyeAt[i * 2 + e]).applyMatrix4(this.group.matrixWorld);
          // they shine back when the lamps are in front of the face
          const fx = b.x - p.x, fz = b.z - p.z, fd = Math.hypot(fx, fz) || 1;
          k = beamAt(this.beam, p) * smooth(0.55, 0.9, (fx * FACING.x + fz * FACING.z) / fd) * 1.6;
        }
        col.setXYZ(this.eyes[i * 2 + e], k * 0.55, k * 0.9, k * 0.7);
      }
    });
    col.needsUpdate = true;
  }

  /** Where cow i stands (world, at its shoulder), or null when it is the missing one. */
  cowAt(i: number, out = new THREE.Vector3()) {
    if (i === this.missing) return null;
    const [x, z] = COWS[i];
    this.group.updateWorldMatrix(true, false);
    return this.group.localToWorld(out.set(x, this.ys[i] + 1.1, z));
  }
  /** How much of the beam is on cow i now (0 when the truck's lights are not on it). */
  litAt(i: number) { const p = this.cowAt(i, _v); return p ? beamAt(this.beam, p) : 0; }
  get count() { return COWS.length; }
  /** The middle of the herd, world. */
  middle(out = new THREE.Vector3()) { this.group.updateWorldMatrix(true, false); return this.group.localToWorld(out.set(HERD.x, surfaceY(HERD.x, HERD.z) + 1, HERD.z)); }
}
