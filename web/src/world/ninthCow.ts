import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { bodyGeometry, tagGeometry, NECK, RED, WHITE, NOSE } from '../drive/herd';
import { colored } from '../drive/roadProps';
import { rng } from '../core/textures';

// The ninth cow, at STATION 01 (story/ninthCow.ts has the when and the words; FORSLAG.md, 20
// October, the order's item 6). One of the herd from the pasture by the highway (drive/herd.ts,
// built from the same pieces, with the same yellow tag in her left ear), lying on her right side
// inside the fence, laid out along B from the comparison vane towards the transit, her head on
// 026, the way the herd stands. Stiff, the legs straight out. Frost on her upper side and on her
// open eye, and the ground round her white with it, unbroken.
//
// What was taken is cut out clean, the edges dark and hard as if burnt, and there is no blood
// anywhere: an oval of hide off the jaw down to the bone, the tongue (the mouth hangs open on
// nothing), and a round piece out of her side. Drawn on small flat pieces laid on the surfaces,
// from one picture (cuts()), so the edges stay sharp at night on a phone: a pale cut with a black
// rim reads in the flashlight from a few metres, and nothing in it is red.
//
// Draw calls: her hide and head 1 (vertex colours), the cuts and the eye 1, the frost on the
// ground 1. Built with the station (world/Station01.ts) and hidden until she is put down.

/** Station-local: where she lies (the middle of her, on the ground), and the way her head points. */
export interface NinthPlace { x: number; z: number; y: number; head: { x: number; z: number } }

/** The frost on the ground round her (metres). */
export const FROST_R = 3.3;

const FROST = [0.5, 0.56, 0.62];
/** Dead, and a night in the frost: the herd's colours, darker and duller (linear). */
const DULL: [number[], number[]][] = [[RED, [0.085, 0.022, 0.012]], [WHITE, [0.32, 0.3, 0.28]], [NOSE, [0.2, 0.13, 0.12]]];
const _v = new THREE.Vector3(), _n = new THREE.Vector3();

/** Her head as she lies: the herd's, with the mouth open and nothing in it. Head terms (headGeometry). */
function deadHead() {
  const B = (w: number, h: number, d: number, x: number, y: number, z: number, c: number[]) => colored(new THREE.BoxGeometry(w, h, d).translate(x, y, z), c);
  const jaw = colored(new THREE.BoxGeometry(0.2, 0.055, 0.15).translate(0, -0.0275, -0.075).rotateX(-0.32).translate(0, -0.155, -0.8), NOSE);
  return mergeGeometries([
    B(0.34, 0.44, 0.52, 0, -0.04, -0.2, RED), B(0.1, 0.05, 0.48, 0, 0.19, -0.2, WHITE),
    B(0.32, 0.36, 0.44, 0, 0.02, -0.6, WHITE),
    // the muzzle, open: the upper part, the lower jaw dropped, and dark inside
    B(0.25, 0.13, 0.17, 0, -0.035, -0.88, NOSE), jaw, B(0.17, 0.07, 0.15, 0, -0.12, -0.86, [0.02, 0.012, 0.01]),
    B(0.16, 0.05, 0.08, -0.24, 0.12, -0.46, RED), B(0.16, 0.05, 0.08, 0.24, 0.12, -0.46, RED),
    tagGeometry(),
  ])!;
}

// ---------- the picture for the cuts and the eye ----------
// Three cells side by side, 128 x 128 each: the round cut in her side, the oval off the jaw (bone,
// and the teeth at its lower edge), the eye with frost on it. Cut out at the alpha (alphaTest).
type Cell = 0 | 1 | 2;
let cutsTex: THREE.CanvasTexture | null = null;
function cuts() {
  if (cutsTex) return cutsTex;
  const S = 128, c = document.createElement('canvas'); c.width = S * 3; c.height = S;
  const g = c.getContext('2d')!, r = rng(909);
  g.clearRect(0, 0, c.width, c.height);
  const oval = (cx: number, rx: number, ry: number) => { g.beginPath(); g.ellipse(cx, S / 2, rx, ry, 0, 0, Math.PI * 2); };
  // the cut: a thin hard black rim, as if burnt in, and inside it the hide's own cut edge, a pale
  // line all the way round, even, the way nothing tears; inside that, what was under the hide, dry
  const cut = (cx: number, rx: number, ry: number) => {
    oval(cx, rx, ry); g.fillStyle = '#080504'; g.fill();
    for (let i = 0; i < 50; i++) {
      const a = r() * Math.PI * 2;
      g.fillStyle = `rgba(30,16,10,${(0.5 + r() * 0.4).toFixed(2)})`;
      g.beginPath(); g.arc(cx + Math.cos(a) * (rx - 2), S / 2 + Math.sin(a) * (ry - 2), 1 + r() * 1.6, 0, Math.PI * 2); g.fill();
    }
    oval(cx, rx - 6, ry - 6); g.fillStyle = '#cdbca2'; g.fill();
    oval(cx, rx - 10, ry - 10);
  };
  // 0: the round piece out of her side: dark and dry inside, nothing wet, nothing red
  {
    const cx = S / 2;
    cut(cx, 62, 62);
    const gr = g.createRadialGradient(cx, S / 2, 4, cx, S / 2, 52);
    gr.addColorStop(0, '#1c0f0c'); gr.addColorStop(0.75, '#2e1a15'); gr.addColorStop(1, '#170c0a');
    g.fillStyle = gr; g.fill();
    // the grain of what is under the hide, cut straight across
    g.strokeStyle = 'rgba(70,46,38,0.55)'; g.lineWidth = 1.4;
    for (let i = 0; i < 8; i++) { const y = S / 2 - 36 + i * 10 + r() * 3; g.beginPath(); g.moveTo(cx - 40, y); g.bezierCurveTo(cx - 10, y + 4, cx + 10, y - 4, cx + 40, y + 1); g.stroke(); }
  }
  // 1: the oval off the jaw: down to the bone, the jawbone along its lower part and the teeth in it
  {
    const cx = S + S / 2;
    cut(cx, 62, 42);
    g.fillStyle = '#1e110d'; g.fill();
    g.save(); oval(cx, 52, 32); g.clip();
    // the jawbone, pale, along the bottom, and the cheek teeth set in it, the gaps between them dark
    g.fillStyle = '#d6ccb2'; g.beginPath(); g.ellipse(cx, S / 2 + 30, 60, 22, 0, 0, Math.PI * 2); g.fill();
    // (worn and uneven, set a little crooked, as teeth are: not a comb)
    let x = cx - 40;
    for (let i = 0; i < 6 && x < cx + 34; i++) {
      const w = 9 + r() * 5, h = 13 + r() * 9, y = S / 2 - 6 + r() * 4;
      g.fillStyle = '#120a07'; g.fillRect(x - 2, y - 2, w + 4, h + 4);
      g.fillStyle = r() < 0.5 ? '#e8e0c8' : '#d8cdaf'; g.beginPath(); g.moveTo(x, y + h); g.lineTo(x + 1, y + 2); g.lineTo(x + w * 0.5, y); g.lineTo(x + w - 1, y + 3); g.lineTo(x + w, y + h); g.fill();
      x += w + 2 + r() * 2;
    }
    g.restore();
  }
  // 2: the eye, open, a frost glaze over it and frost in the lashes round it
  {
    const cx = 2 * S + S / 2;
    g.fillStyle = 'rgba(214,226,236,0.95)'; oval(cx, 60, 44); g.fill();
    for (let i = 0; i < 90; i++) {
      const a = r() * Math.PI * 2, d = 0.82 + r() * 0.2;
      g.strokeStyle = `rgba(240,246,255,${(0.6 + r() * 0.4).toFixed(2)})`; g.lineWidth = 1 + r();
      g.beginPath(); g.moveTo(cx + Math.cos(a) * 46 * d, S / 2 + Math.sin(a) * 34 * d); g.lineTo(cx + Math.cos(a) * 62 * d, S / 2 + Math.sin(a) * 46 * d); g.stroke();
    }
    g.fillStyle = '#060506'; oval(cx, 42, 32); g.fill();
    // the glaze: milky, thicker at the bottom, and one hard point of light in it
    const gl = g.createLinearGradient(0, S / 2 - 30, 0, S / 2 + 30);
    gl.addColorStop(0, 'rgba(150,165,180,0.4)'); gl.addColorStop(1, 'rgba(200,214,228,0.85)');
    g.fillStyle = gl; oval(cx, 38, 29); g.fill();
    g.fillStyle = 'rgba(255,255,255,0.95)'; g.beginPath(); g.arc(cx - 12, S / 2 - 10, 5, 0, Math.PI * 2); g.fill();
  }
  cutsTex = new THREE.CanvasTexture(c); cutsTex.colorSpace = THREE.SRGBColorSpace; cutsTex.anisotropy = 4;
  return cutsTex;
}
/** A flat piece of the picture's cell, w by h metres, laid on a surface at p (normal n, the
 *  picture's up along `up`), a hair off it. */
function piece(cell: Cell, w: number, h: number, p: THREE.Vector3, n: THREE.Vector3, up: THREE.Vector3) {
  const g = new THREE.PlaneGeometry(w, h);
  const uv = g.attributes.uv as THREE.BufferAttribute;
  for (let i = 0; i < uv.count; i++) uv.setX(i, (cell + uv.getX(i)) / 3);
  const z = n.clone().normalize(), x = new THREE.Vector3().crossVectors(up, z).normalize(), y = new THREE.Vector3().crossVectors(z, x);
  g.applyMatrix4(new THREE.Matrix4().makeBasis(x, y, z).setPosition(p.clone().addScaledVector(z, 0.006)));
  return g;
}

// ---------- the frost on the ground ----------
let frostTex: THREE.CanvasTexture | null = null;
/** White where the frost is: thick under and round her, patchy out to the edge, unbroken (no
 *  track through it anywhere). The disc's own UVs: a square. */
function frostPicture() {
  if (frostTex) return frostTex;
  const N = 256, c = document.createElement('canvas'); c.width = c.height = N;
  const g = c.getContext('2d')!, im = g.createImageData(N, N), r = rng(2547);
  const cells = Array.from({ length: 3 }, (_, o) => { const n = 6 << o; return { n, v: Array.from({ length: n * n }, r) }; });
  const noise = (x: number, y: number) => {
    let t = 0, w = 0;
    cells.forEach(({ n, v }, o) => {
      const fx = x * (n - 1), fy = y * (n - 1), ix = Math.floor(fx), iy = Math.floor(fy), ax = fx - ix, ay = fy - iy;
      const at = (i: number, j: number) => v[Math.min(n - 1, j) * n + Math.min(n - 1, i)];
      const k = 1 / (1 + o);
      t += k * ((at(ix, iy) * (1 - ax) + at(ix + 1, iy) * ax) * (1 - ay) + (at(ix, iy + 1) * (1 - ax) + at(ix + 1, iy + 1) * ax) * ay); w += k;
    });
    return t / w;
  };
  for (let y = 0; y < N; y++) for (let x = 0; x < N; x++) {
    const u = (x + 0.5) / N * 2 - 1, v = (y + 0.5) / N * 2 - 1, d = Math.hypot(u, v);
    // whole to two thirds out, then in patches, and soft at the edge
    const a0 = Math.atan2(v, u), wob = 0.12 * Math.sin(a0 * 3 + 1.3) + 0.08 * Math.sin(a0 * 7 + 0.4);
    const edge = 1 - THREE.MathUtils.smoothstep(d, 0.45 + wob + 0.2 * noise(u * 0.5 + 0.5, v * 0.5 + 0.5), 0.98);
    const patch = THREE.MathUtils.clamp((noise((x + 0.5) / N, (y + 0.5) / N) - 0.36) * 2.6 + (1 - d) * 1.1, 0, 1);
    const fine = noise(((x * 3.7) % N + 0.5) / N, ((y * 3.7) % N + 0.5) / N);
    const a = edge * Math.min(1, patch * 1.35) * (0.62 + 0.38 * fine);
    const i = (y * N + x) * 4; im.data[i] = im.data[i + 1] = im.data[i + 2] = 255; im.data[i + 3] = Math.round(255 * Math.min(1, a));
  }
  g.putImageData(im, 0, 0);
  // and the crystals: small hard points where the frost is thick
  for (let k = 0; k < 900; k++) {
    const u = r() * 2 - 1, v = r() * 2 - 1, d = Math.hypot(u, v);
    if (d > 0.8 || r() > 1 - d * 0.9) continue;
    g.fillStyle = `rgba(255,255,255,${(0.7 + r() * 0.3).toFixed(2)})`;
    g.fillRect((u + 1) / 2 * N, (v + 1) / 2 * N, 1 + (r() < 0.2 ? 1 : 0), 1);
  }
  frostTex = new THREE.CanvasTexture(c); frostTex.colorSpace = THREE.SRGBColorSpace;
  return frostTex;
}

export interface NinthCowBuilt {
  group: THREE.Group;
  /** Hit areas (invisible): her head, her body, and the frost round her (a closer look only). */
  hit: { head: THREE.Mesh; body: THREE.Mesh; frost: THREE.Mesh };
  /** Where to look at her (station-local, world terms of the station's group): her head, her side. */
  at: { head: THREE.Vector3; body: THREE.Vector3; middle: THREE.Vector3 };
  /** Her footprint on the ground (station-local), for a collider. */
  box: { minX: number; maxX: number; minZ: number; maxZ: number };
  /** Hers to dispose with the station (the pictures are shared, made once: not these). */
  materials: THREE.Material[];
}

/** Builds her lying at `at`; `lit` makes a material lit by the station's floods (Station01.lit). */
export function buildNinthCow(at: NinthPlace, lit: (o: THREE.MeshStandardMaterialParameters) => THREE.MeshStandardMaterial): NinthCowBuilt {
  // ---------- in her own terms first, standing, facing -z ----------
  const head = deadHead();
  // the head on its neck as she fell: turned a little to her right (that is towards the ground once
  // she lies on her right side), stretched out
  const headM = new THREE.Matrix4().compose(NECK.clone().add(new THREE.Vector3(0.04, -0.06, -0.02)), new THREE.Quaternion().setFromEuler(new THREE.Euler(-0.08, -0.42, 0, 'YXZ')), new THREE.Vector3(1, 1, 1));
  head.applyMatrix4(headM);
  const body = bodyGeometry();
  const hide = mergeGeometries([body, head])!;
  // the cuts and the eye on her left side (the side that will be up), in the same terms
  const bodyMesh = new THREE.Mesh(body), rc = new THREE.Raycaster();
  /** Where a ray from her left, at (y, z), meets her side, and the surface's normal there. */
  const onSide = (y: number, z: number) => {
    rc.set(new THREE.Vector3(-2, y, z), new THREE.Vector3(1, 0, 0));
    const h = rc.intersectObject(bodyMesh)[0];
    return h ? { p: h.point.clone(), n: h.face!.normal.clone() } : { p: new THREE.Vector3(-0.42, y, z), n: new THREE.Vector3(-1, 0, 0) };
  };
  const side = onSide(1.1, 0.16);
  // the jaw and the eye: on the white face's left side (x -0.16 of the head), as the head is turned;
  // the pictures' up is the top of her head (the teeth at the jaw's lower edge)
  const nHead = new THREE.Vector3(-1, 0, 0).transformDirection(headM), upHead = new THREE.Vector3(0, 1, 0).transformDirection(headM);
  const onHead = (x: number, y: number, z: number) => new THREE.Vector3(x, y, z).applyMatrix4(headM);
  const cutGeo = mergeGeometries([
    piece(0, 0.34, 0.34, side.p, side.n, new THREE.Vector3(0, 0, -1)),
    piece(1, 0.27, 0.18, onHead(-0.161, -0.06, -0.69), nHead, upHead),
    piece(2, 0.13, 0.095, onHead(-0.161, 0.105, -0.655), nHead, upHead),
  ])!;

  // ---------- lying on her right side ----------
  // her left side up, her back to +x, the legs out to -x; rolled a little past the side, so the face
  // of her flank with the cut in it lies level (the barrel has seven faces) and the legs stand out
  // and a little up, stiff; then down onto the ground
  const tilt = Math.atan2(-side.n.y, -side.n.x);
  const roll = new THREE.Matrix4().makeRotationZ(-Math.PI / 2 - tilt);
  hide.applyMatrix4(roll); cutGeo.applyMatrix4(roll);
  hide.computeBoundingBox();
  const bb = hide.boundingBox!, shift = new THREE.Matrix4().makeTranslation(-(bb.min.x + bb.max.x) / 2 + 0.12, -bb.min.y - 0.03, 0);
  hide.applyMatrix4(shift); cutGeo.applyMatrix4(shift);
  // dulled, and frost on what faces up, in patches (a vertex at a time: each face of the boxes has its own)
  const pos = hide.attributes.position as THREE.BufferAttribute, nor = hide.attributes.normal as THREE.BufferAttribute, col = hide.attributes.color as THREE.BufferAttribute;
  for (let i = 0; i < pos.count; i++) {
    for (const [from, to] of DULL) if (Math.abs(col.getX(i) - from[0]) + Math.abs(col.getY(i) - from[1]) + Math.abs(col.getZ(i) - from[2]) < 0.01) { col.setXYZ(i, to[0], to[1], to[2]); break; }
    _n.fromBufferAttribute(nor, i); _v.fromBufferAttribute(pos, i);
    const up = THREE.MathUtils.smoothstep(_n.y, 0.25, 0.9), grain = 0.55 + 0.45 * Math.abs(Math.sin(_v.x * 17.3 + _v.z * 11.1) * Math.cos(_v.z * 7.7));
    // (not on the red, which a grey bloom turns pink in the flashlight: she stays dark red-brown; on the white more)
    const k = col.getX(i) < 0.2 ? 0 : up * grain * 0.4;
    if (k <= 0) continue;
    col.setXYZ(i, col.getX(i) + (FROST[0] - col.getX(i)) * k, col.getY(i) + (FROST[1] - col.getY(i)) * k, col.getZ(i) + (FROST[2] - col.getZ(i)) * k);
  }

  // ---------- into the station, her head on `at.head` ----------
  const group = new THREE.Group(); group.name = 'ninth';
  group.position.set(at.x, at.y, at.z);
  group.rotation.y = Math.atan2(-at.head.x, -at.head.z);   // her -z along at.head
  const hideMat = lit({ vertexColors: true, roughness: 0.92, flatShading: true });
  const cutMat = lit({ map: cuts(), roughness: 0.75, alphaTest: 0.5, side: THREE.DoubleSide });
  const hideMesh = new THREE.Mesh(hide, hideMat); hideMesh.name = 'ninth:hide';
  const cutMesh = new THREE.Mesh(cutGeo, cutMat); cutMesh.name = 'ninth:cuts';
  // the frost round her, on the ground: lit, white in the flashlight and on film, and with a little
  // light of its own, so it reads pale in the dark as the ring in the pasture does (drive/herd.ts)
  const fg = new THREE.CircleGeometry(FROST_R, 48); fg.rotateX(-Math.PI / 2);
  const frostMat = lit({ color: 0xe6eef4, emissive: 0x2c3a4a, map: frostPicture(), roughness: 0.55, transparent: true, opacity: 0.68, depthWrite: false,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -2 });
  const frost = new THREE.Mesh(fg, frostMat); frost.name = 'ninth:frost'; frost.position.y = 0.025; frost.renderOrder = 1;
  group.add(frost, hideMesh, cutMesh);

  // ---------- where she can be looked at, and what stops the player ----------
  hide.computeBoundingBox();
  const hb = hide.boundingBox!;
  const hitMat = new THREE.MeshBasicMaterial({ visible: false });
  // her head: round the head's end of her (the -z end), her body: the rest
  const headZ = hb.min.z, headHit = new THREE.Mesh(new THREE.BoxGeometry(0.9, 0.7, 0.85), hitMat);
  _v.set(0, 0, 0).applyMatrix4(headM).applyMatrix4(roll).applyMatrix4(shift);
  headHit.position.set(_v.x - 0.05, 0.3, headZ + 0.42); headHit.name = 'ninth:head';
  const bodyHit = new THREE.Mesh(new THREE.BoxGeometry(hb.max.x - hb.min.x, 0.95, 1.95), hitMat);
  bodyHit.position.set((hb.min.x + hb.max.x) / 2, 0.47, 0.05); bodyHit.name = 'ninth:body';
  const frostHit = new THREE.Mesh(new THREE.BoxGeometry(FROST_R * 1.9, 0.12, FROST_R * 1.9), hitMat);
  frostHit.position.y = 0.04; frostHit.name = 'ninth:frostHit';
  group.add(headHit, bodyHit, frostHit);
  group.updateMatrix();
  // the footprint, round her turned box (local corners into the station)
  const corners = [[hb.min.x, hb.min.z], [hb.max.x, hb.min.z], [hb.min.x, hb.max.z], [hb.max.x, hb.max.z]].map(([x, z]) => new THREE.Vector3(x, 0, z).applyMatrix4(group.matrix));
  const box = { minX: Math.min(...corners.map((c) => c.x)) + 0.25, maxX: Math.max(...corners.map((c) => c.x)) - 0.25, minZ: Math.min(...corners.map((c) => c.z)) + 0.25, maxZ: Math.max(...corners.map((c) => c.z)) - 0.25 };
  return {
    group, hit: { head: headHit, body: bodyHit, frost: frostHit },
    at: { head: headHit.position.clone().applyMatrix4(group.matrix), body: bodyHit.position.clone().applyMatrix4(group.matrix), middle: new THREE.Vector3(at.x, at.y + 0.45, at.z) },
    box, materials: [hideMat, cutMat, frostMat, hitMat],
  };
}
