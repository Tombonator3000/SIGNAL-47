import * as THREE from 'three';
import { M, box, cyl, plane, rod, mergeStatic, noMerge, floodlit } from './kit';
import { GlowPoints } from './glow';
import * as T from '../core/textures';
import { artTexture, artImage } from '../core/art';
import type { Collider } from './ControlRoom';
import type { Zone } from '../player/Player';
import { GROUND } from './Crossing';
import { ANTENNAS, APRON, FIELD_ZONES, GATE, GATE_WAY, FENCE_Z, HWY, HWY_WALK, LAMPS, onField, inRect, nearestAntenna, type Antenna } from './arrayLayout';

// The antenna field, walked (todo: the field walkable; FORSLAG.md, section 4). North of the
// chain-link fence at z -20 the array is a desert of gravel tracks: the walk gate north of the
// windows side, the spine along the cable trench, the rows east and west between the antennas'
// latitudes, a spur to each antenna, and an apron of gravel round each base, with the drive
// cabinet where the spur comes in. The layout is arrayLayout.ts. Part of SARO's area.
//
// Walking: every track and apron is a walk zone (arrayLayout.FIELD_ZONES), and so is the highway
// through the array (HWY_WALK: the truck can be left on it, World.canGetOut); the desert between
// them is not walked, the bases and the cabinets are colliders, and the gate's doorway zone is
// on while the gate is open (Doors.ts, 'gate'). The ground is SARO's (-0.6, Grounds.floorAt),
// graded flat over the whole array (Exterior.ts, ARRAY_FLAT).
//
// Cost: the gravel, the cabinets, their faces (one atlas for all 27 names), the pads and the
// gate posts are merged by material; the cabinets' pilot lamps are one GlowPoints; the gate leaf
// moves on its own. About ten draw calls with the field in view. No lamps of its own: between
// the sodium floods near SARO the field is dark, and the flashlight is what shows the way.

const AW = 1024, AH = 1024, TW = 128, TH = 224, COLS = 8;
/** The cabinet: a pad, the steel box and its door face (the atlas), a cap. Local y from the ground. */
const CAB = { w: 0.8, h: 1.4, d: 0.5, pad: 0.12 };
/** Where the inspection window is on a face (tile pixels), for the readout behind it. */
const WIN = { x0: 28, x1: 100, y0: 80, y1: 124 };
const SCREEN_W = 256, SCREEN_H = 160;

export interface FieldCabinet {
  a: Antenna;
  /** The invisible box the crosshair finds (and the closer look). */
  proxy: THREE.Object3D;
  /** The invisible box round the antenna's pedestal, for the closer look only. */
  mast: THREE.Object3D;
  /** The middle of its inspection window, and which way the door faces (world). */
  window: THREE.Vector3;
}

export class Field {
  group = new THREE.Group();
  zones: Zone[] = [];
  zone: Record<string, Zone> = {};
  colliders: Collider[] = [];
  cabinets: FieldCabinet[] = [];
  /** The gate: the box the crosshair finds, and its leaf's collider while it stands open. */
  gateProxy!: THREE.Object3D;
  gateLeaf: Collider = { minX: GATE.x0 - 0.08, maxX: GATE.x0 + 0.02, minZ: FENCE_Z - (GATE.x1 - GATE.x0) - 0.05, maxZ: FENCE_Z };
  private gateHinge = new THREE.Group();
  private glow = new GlowPoints();
  private screen: THREE.Mesh;
  private scr = T.screenCanvas(SCREEN_W, SCREEN_H);
  /** The cabinet whose readout is lit, and what it shows (main.ts asks, story/field.ts draws). */
  lit: { cab: FieldCabinet; lines: () => { head: string; rows: string[]; warn: string | null }; t: number } | null = null;
  private drawT = 0;

  constructor() {
    const st = new THREE.Group();
    this.group.add(st);
    this.walkable();
    this.gravel(st);
    this.cabinetsAt(st);
    this.gate(st);
    mergeStatic(st);
    this.group.add(this.glow.build());
    // the readout behind a cabinet's inspection glass: one screen, moved to the cabinet in use
    this.screen = new THREE.Mesh(new THREE.PlaneGeometry(0.43, 0.26), new THREE.MeshBasicMaterial({ map: this.scr.tex, toneMapped: false }));
    this.screen.name = 'fieldReadout';
    this.screen.visible = false;
    noMerge(this.screen);
    this.group.add(this.screen);
  }

  // ---------- where the player can go ----------
  private walkable() {
    for (const { id, r } of FIELD_ZONES) this.addZone(id, r.minX, r.maxX, r.minZ, r.maxZ);
    this.addZone('gate', GATE_WAY.minX, GATE_WAY.maxX, GATE_WAY.minZ, GATE_WAY.maxZ, false);
    // the highway north through the array, to get out of the truck left on it and walk to the masts
    // along the rows; it ends short of the fence's opening (on foot the gate is the way in) and
    // where the truck's way ends. Not part of the field's gravel (FIELD_ZONES)
    this.addZone('fHighway', HWY_WALK.minX, HWY_WALK.maxX, HWY_WALK.minZ, HWY_WALK.maxZ);
    for (const a of ANTENNAS) this.colliders.push({ ...a.base });
    // the lamp posts beside the tracks and the highway's (Exterior.ts), where a row goes by
    for (const [x, z] of [...[-32, -66, -100, -134, -168].map((z) => [-18.5, z]), ...LAMPS]) {
      this.colliders.push({ minX: x - 0.25, maxX: x + 0.25, minZ: z - 0.25, maxZ: z + 0.25 });
    }
  }
  private addZone(id: string, minX: number, maxX: number, minZ: number, maxZ: number, enabled = true) {
    const z: Zone = { id, minX, maxX, minZ, maxZ, enabled };
    this.zones.push(z); this.zone[id] = z;
  }

  /** On the field's gravel or the highway through it (for the place name). */
  inField(p: THREE.Vector3) { return onField(p.x, p.z, 0.3) || this.onHighway(p); }
  /** On the highway's asphalt north of the fence (the footsteps). */
  onHighway(p: THREE.Vector3) { return inRect(HWY_WALK, p.x, p.z) && p.x > HWY.x0 && p.x < HWY.x1; }
  /** What the place is called where the player stands in the field (saves, the corner). */
  placeAt(p: THREE.Vector3) {
    const { a, d } = nearestAntenna(p.x, p.z);
    return d < 14 ? `The array, ${a.id}` : 'The array';
  }

  // ---------- the gravel ----------
  // Each track and apron is a flat quad a little inside its walk zone, cut where a row crosses
  // the highway (the asphalt shows there). It lies just above the ground and is pushed forward
  // in depth (polygon offset), so it does not flicker against the desert a few hundred metres off.
  private gravel(st: THREE.Group) {
    const mat = floodlit(new THREE.MeshStandardMaterial({ map: gravelTex(), roughness: 1,
      polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4 }), 0.012);
    const y = GROUND + 0.01, m = 0.18;
    const quad = (x0: number, x1: number, z0: number, z1: number) => {
      if (x1 - x0 < 0.05 || z1 - z0 < 0.05) return;
      const cx = (x0 + x1) / 2, cz = (z0 + z1) / 2;
      const q = plane(st, x1 - x0, z1 - z0, mat, cx, y, cz, 0, -Math.PI / 2);
      // the picture by where it lies (2 m to a picture), so two quads that overlap where tracks
      // meet show the same gravel and their fight for the depth cannot be seen
      const uv = q.geometry.attributes.uv as THREE.BufferAttribute, pos = q.geometry.attributes.position as THREE.BufferAttribute;
      for (let i = 0; i < uv.count; i++) uv.setXY(i, (cx + pos.getX(i)) / 2, (cz - pos.getY(i)) / 2);
    };
    for (const { r } of FIELD_ZONES) {
      const x0 = r.minX + m, x1 = r.maxX - m, z0 = r.minZ + m, z1 = Math.min(r.maxZ - m, FENCE_Z - 0.1);
      if (x0 < HWY.x0 && x1 > HWY.x1) { quad(x0, HWY.x0, z0, z1); quad(HWY.x1, x1, z0, z1); }
      else if (x1 > HWY.x0 && x0 < HWY.x1) { if (x0 < HWY.x0) quad(x0, HWY.x0, z0, z1); if (x1 > HWY.x1) quad(HWY.x1, x1, z0, z1); }
      else quad(x0, x1, z0, z1);
    }
    // the gravel through the gate, to the windows side's ground
    quad(GATE.x0 + 0.1, GATE.x1 - 0.1, FENCE_Z - 0.5, FENCE_Z + 0.9);
  }

  // ---------- the drive cabinets ----------
  private cabinetsAt(st: THREE.Group) {
    const atlas = cabinetAtlas();
    const steel = floodlit(new THREE.MeshStandardMaterial({ map: artTexture('cabinet'), color: 0xa6b3af, roughness: 0.55, metalness: 0.45 }));
    const face = floodlit(new THREE.MeshStandardMaterial({ map: atlas, roughness: 0.6, metalness: 0.3 }));
    const hit = new THREE.MeshBasicMaterial({ visible: false });
    ANTENNAS.forEach((a, i) => {
      const g = new THREE.Group();
      g.position.set(a.cab.x, GROUND - 0.02, a.cab.z);
      g.rotation.y = a.cab.ry;
      const junction = a.id === 'S-03';   // its drive is the motor bus on the east walk (ServiceYard.ts)
      const k = junction ? 0.7 : 1, w = CAB.w * (junction ? 0.8 : 1), h = CAB.h * k;
      box(g, w + 0.2, CAB.pad, CAB.d + 0.2, M.concrete, 0, CAB.pad / 2, 0);
      box(g, w, h, CAB.d, steel, 0, CAB.pad + h / 2, 0);
      box(g, w + 0.06, 0.05, CAB.d + 0.08, steel, 0, CAB.pad + h + 0.025, 0.01);   // the rain cap
      const f = plane(g, w, h, face, 0, CAB.pad + h / 2, CAB.d / 2 + 0.003);
      const c = i % COLS, r = Math.floor(i / COLS), uv = f.geometry.attributes.uv as THREE.BufferAttribute;
      for (let j = 0; j < uv.count; j++) uv.setXY(j, (c * TW + uv.getX(j) * TW) / AW, 1 - (r * TH + (1 - uv.getY(j)) * TH) / AH);
      // the conduit out of the back, down into the trench to the pedestal
      rod(g, new THREE.Vector3(0.18, CAB.pad + 0.25, -CAB.d / 2), new THREE.Vector3(0.18, CAB.pad + 0.25, -CAB.d / 2 - 0.25), 0.035, M.pole, 6);
      rod(g, new THREE.Vector3(0.18, CAB.pad + 0.25, -CAB.d / 2 - 0.25), new THREE.Vector3(0.18, -0.05, -CAB.d / 2 - 0.25), 0.035, M.pole, 6);
      // and the cable trench on to the base, under concrete covers just proud of the gravel
      const run = (APRON - 1.4) - (3.5 * a.scale + 0.05) - 0.6, n = Math.max(1, Math.round(run / 0.6));
      for (let k = 0; k < n; k++) box(g, 0.5, 0.06, run / n - 0.04, M.concrete, 0.18, 0.04, -0.6 - (k + 0.5) * run / n);
      st.add(g);
      g.updateMatrixWorld(true);
      // a pilot lamp over the door: the only light out here, and only a speck
      const lamp = new THREE.Vector3(w / 2 - 0.1, CAB.pad + h - 0.08, CAB.d / 2 + 0.02).applyMatrix4(g.matrixWorld);
      this.glow.add(lamp.x, lamp.y, lamp.z, 0.14, junction ? 0xffa040 : 0x7dff9a);
      // where the readout goes: the middle of the inspection window, just in front of the glass
      const window = new THREE.Vector3(0, CAB.pad + h * (1 - ((WIN.y0 + WIN.y1) / 2) / TH), CAB.d / 2 + 0.006).applyMatrix4(g.matrixWorld);
      const proxy = new THREE.Mesh(new THREE.BoxGeometry(w + 0.1, h + 0.2, CAB.d + 0.15), hit);
      proxy.position.set(a.cab.x, GROUND + (h + 0.2) / 2, a.cab.z); proxy.rotation.y = a.cab.ry;
      proxy.name = `field:${a.id}`;
      noMerge(proxy); this.group.add(proxy);
      // the antenna's pedestal, for a closer look from the apron (the dishes themselves are instanced)
      const mast = new THREE.Mesh(new THREE.BoxGeometry(5 * a.scale, 11, 5 * a.scale), hit);
      mast.position.set(a.x, GROUND + 5.5, a.z); mast.name = `mast:${a.id}`;
      noMerge(mast); this.group.add(mast);
      const along = Math.abs(Math.sin(a.cab.ry)) > 0.5, hx = (along ? CAB.d + 0.2 : w + 0.2) / 2, hz = (along ? w + 0.2 : CAB.d + 0.2) / 2;
      this.colliders.push({ minX: a.cab.x - hx, maxX: a.cab.x + hx, minZ: a.cab.z - hz, maxZ: a.cab.z + hz });
      this.cabinets.push({ a, proxy, mast, window });
    });
  }

  // ---------- the walk gate in the fence ----------
  // Two heavier posts in the chain-link (Exterior.ts leaves the gap), a leaf of the same mesh in a
  // pipe frame, hinged on the west post and swinging north into the field, a chain that hangs
  // loose, and the array's sign on the fence beside it.
  private gate(st: THREE.Group) {
    const w = GATE.x1 - GATE.x0, top = 1.8, bottom = GROUND + 0.05;
    for (const x of [GATE.x0, GATE.x1]) {
      cyl(st, 0.045, 0.045, 2.75, M.pole, x, GROUND + 1.37, FENCE_Z, 8);
      cyl(st, 0.055, 0.055, 0.04, M.pole, x, GROUND + 2.76, FENCE_Z, 8);
    }
    const leaf = this.gateHinge;
    leaf.position.set(GATE.x0, 0, FENCE_Z);
    const lw = w - 0.06, cx = 0.03 + lw / 2;
    const v = (x: number, y: number) => new THREE.Vector3(x, y, 0);
    for (const [a, b] of [[v(0.03, bottom), v(0.03 + lw, bottom)], [v(0.03, top), v(0.03 + lw, top)], [v(0.03, bottom), v(0.03, top)], [v(0.03 + lw, bottom), v(0.03 + lw, top)], [v(0.03, (bottom + top) / 2), v(0.03 + lw, (bottom + top) / 2)]]) {
      rod(leaf, a, b, 0.02, M.pole, 6);
    }
    const tex = T.chainlink();
    tex.repeat.set(60 * lw / 140, 3 * (top - bottom) / 2.4);
    const mesh = plane(leaf, lw, top - bottom, new THREE.MeshBasicMaterial({ map: tex, transparent: true, alphaTest: 0.4, side: THREE.DoubleSide, color: 0x6d6a62 }), cx, (top + bottom) / 2, 0);
    mesh.name = 'fieldGateLeaf';
    // the latch and its chain, hanging from the leaf's far side
    box(leaf, 0.06, 0.12, 0.05, M.steel, 0.03 + lw - 0.04, 0.95, 0.03);
    rod(leaf, v(0.03 + lw - 0.04, 0.92), new THREE.Vector3(0.03 + lw - 0.07, 0.55, 0.05), 0.012, M.steel, 4);
    mergeStatic(leaf);
    noMerge(leaf); this.group.add(leaf);
    // the sign on the fence west of the gate, facing the windows side
    const sign = plane(st, 0.9, 0.5, floodlit(new THREE.MeshStandardMaterial({ map: T.labelCard(['SARO ARRAY', 'AUTHORIZED PERSONNEL ONLY', 'KEEP TO THE TRACKS'], { w: 256, h: 144, size: 22 }), roughness: 0.8 })),
      GATE.x0 - 0.75, 0.75, FENCE_Z + 0.03);
    sign.name = 'arraySign';
    for (const dx of [-0.4, 0.4]) box(st, 0.03, 0.03, 0.02, M.steel, GATE.x0 - 0.75 + dx, 0.75, FENCE_Z + 0.04);
    const proxy = new THREE.Mesh(new THREE.BoxGeometry(w, 2.0, 0.5), new THREE.MeshBasicMaterial({ visible: false }));
    proxy.position.set((GATE.x0 + GATE.x1) / 2, GROUND + 1.0, FENCE_Z); proxy.name = 'fieldGate';
    noMerge(proxy); this.group.add(proxy);
    this.gateProxy = proxy;
  }
  /** 0 shut, 1 open (Doors.ts swings it): the leaf turns north on its hinge. */
  setGate(k: number) { this.gateHinge.rotation.y = k * Math.PI / 2; }

  // ---------- the readout ----------
  /** Light the readout behind a cabinet's glass (story/field.ts gives what it says). */
  showReadout(cab: FieldCabinet, lines: () => { head: string; rows: string[]; warn: string | null }) {
    this.lit = { cab, lines, t: 0 };
    this.screen.position.copy(cab.window);
    this.screen.rotation.set(0, cab.a.cab.ry, 0);
    this.screen.visible = true;
    this.drawT = 0;
    this.draw(0);
  }
  hideReadout() { this.lit = null; this.screen.visible = false; }
  private draw(t: number) {
    if (!this.lit) return;
    const g = this.scr.ctx, w = SCREEN_W, h = SCREEN_H, o = this.lit.lines();
    g.fillStyle = '#0b0d08'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#ffb052'; g.font = '24px VT323, monospace'; g.fillText(o.head, 12, 28, w - 24);
    g.font = '21px VT323, monospace';
    o.rows.forEach((l, i) => g.fillText(l, 12, 60 + i * 24, w - 24));
    if (o.warn && Math.floor(t * 2) % 2 === 0) { g.fillStyle = '#ffd27a'; g.fillText(o.warn, 12, 60 + o.rows.length * 24 + 6, w - 24); }
    this.scr.tex.needsUpdate = true;
  }

  /** Every frame in SARO's area: the pilot lamps, and the readout while the player stays by it. */
  update(dt: number, t: number, eye: THREE.Vector3) {
    this.glow.update(t);
    const L = this.lit;
    if (!L) return;
    L.t += dt;
    const at = L.cab.proxy.position;
    if (L.t > 60 || Math.hypot(eye.x - at.x, eye.z - at.z) > 12) { this.hideReadout(); return; }
    this.drawT -= dt;
    if (this.drawT <= 0) { this.drawT = 0.25; this.draw(t); }
  }
}

// ---------- the gravel ----------
// Crushed rock, greyer and paler than the red desert either side, so a track shows in the
// flashlight and under the floods: a mottled ground with a few hundred stones on it, 2 m across.
function gravelTex() {
  return T.canvasTex(256, 256, (g, w, h) => {
    const r = T.rng(1986);
    g.fillStyle = '#8f8778'; g.fillRect(0, 0, w, h);
    for (let i = 0; i < 1400; i++) { const v = 110 + r() * 70; g.fillStyle = `rgba(${v + 8},${v + 2},${v - 8},${0.25 + r() * 0.3})`; g.fillRect(r() * w, r() * h, 2 + r() * 5, 2 + r() * 5); }
    // the stones, drawn three times over the edges so the picture repeats without a seam
    for (let i = 0; i < 520; i++) {
      const x = r() * w, y = r() * h, rx = 1.2 + r() * 3.4, ry = rx * (0.6 + r() * 0.4), a = r() * Math.PI, v = 95 + r() * 120;
      const col = `rgb(${v + 10},${v + 4},${v - 10})`, dark = `rgba(30,26,20,.45)`;
      for (const [ox, oy] of [[0, 0], [w, 0], [-w, 0], [0, h], [0, -h]]) {
        g.fillStyle = dark; g.beginPath(); g.ellipse(x + ox + 0.8, y + oy + 1, rx, ry, a, 0, 7); g.fill();
        g.fillStyle = col; g.beginPath(); g.ellipse(x + ox, y + oy, rx, ry, a, 0, 7); g.fill();
      }
    }
  }, [1, 1]);
}

// ---------- the atlas: each cabinet's door, its antenna's name stencilled on it ----------
// One picture for all 27 doors (TW x TH each, in the order of ANTENNAS): the steel of the cabinet
// art, the name in stencil paint with its bridges and overspray, the inspection window, the
// danger plate. The words are drawn in code (AGENTS.md: text that must be right).
function cabinetAtlas() {
  return T.canvasTex(AW, AH, (g) => {
    g.fillStyle = '#3a3f3e'; g.fillRect(0, 0, AW, AH);
    const im = artImage('cabinet'), rnd = T.rng(47);
    ANTENNAS.forEach((a, i) => {
      const x0 = (i % COLS) * TW, y0 = Math.floor(i / COLS) * TH, junction = a.id === 'S-03';
      g.save(); g.translate(x0, y0);
      g.beginPath(); g.rect(0, 0, TW, TH); g.clip();
      g.drawImage(im, 0, 0, TW, TH);
      // weather: grime low on the door, rust from the hinges
      const grime = g.createLinearGradient(0, TH * 0.55, 0, TH);
      grime.addColorStop(0, 'rgba(60,48,30,0)'); grime.addColorStop(1, 'rgba(60,48,30,.45)');
      g.fillStyle = grime; g.fillRect(0, 0, TW, TH);
      for (let k = 0; k < 14; k++) { g.fillStyle = `rgba(110,60,25,${0.15 + rnd() * 0.3})`; g.fillRect(rnd() < 0.5 ? rnd() * 8 : TW - rnd() * 8, rnd() * TH, 1 + rnd() * 3, 2 + rnd() * 12); }
      g.strokeStyle = 'rgba(20,24,26,.8)'; g.lineWidth = 2; g.strokeRect(5, 5, TW - 10, TH - 10);
      stencil(g, a.id, TW / 2, 44, junction ? 40 : 46, TW - 16, rnd);
      g.fillStyle = '#e2dccb'; g.font = '500 13px Oswald'; g.textAlign = 'center';
      g.fillText(junction ? 'CABLE JUNCTION' : 'AZ / EL DRIVE', TW / 2, 68, TW - 20);
      // the inspection window (the readout sits behind it)
      g.fillStyle = '#141a1d'; g.fillRect(WIN.x0, WIN.y0, WIN.x1 - WIN.x0, WIN.y1 - WIN.y0);
      g.strokeStyle = 'rgba(200,196,180,.35)'; g.lineWidth = 2; g.strokeRect(WIN.x0 - 3, WIN.y0 - 3, WIN.x1 - WIN.x0 + 6, WIN.y1 - WIN.y0 + 6);
      g.fillStyle = 'rgba(255,255,255,.06)'; g.fillRect(WIN.x0 + 4, WIN.y0 + 3, 18, WIN.y1 - WIN.y0 - 6);
      g.fillStyle = '#d8d1bd'; g.font = '11px Oswald';
      g.fillText(junction ? 'TO MOTOR BUS, EAST WALK' : 'SARO ARRAY', TW / 2, WIN.y1 + 18, TW - 16);
      // the handle and the danger plate
      g.fillStyle = '#23282a'; g.fillRect(TW - 22, 138, 8, 30);
      g.fillStyle = '#d1a91f'; g.fillRect(30, 182, 68, 26);
      g.fillStyle = '#16120a'; g.font = '600 13px Oswald'; g.fillText('DANGER', 64, 194);
      g.font = '10px Oswald'; g.fillText('480 VOLTS', 64, 205);
      g.restore();
    });
  });
}

// A name in stencil paint: a soft overspray, the letters, the bridges a stencil leaves, and wear.
function stencil(g: CanvasRenderingContext2D, text: string, x: number, y: number, size: number, maxW: number, rnd: () => number) {
  const c = document.createElement('canvas'); c.width = TW; c.height = 80;
  const t = c.getContext('2d')!;
  t.font = `600 ${size}px Oswald`; t.textAlign = 'center'; t.textBaseline = 'middle';
  t.fillStyle = '#e6dfc8';
  t.shadowColor = 'rgba(230,223,200,.35)'; t.shadowBlur = 3;
  t.fillText(text, TW / 2, 40, maxW);
  t.shadowBlur = 0;
  t.globalCompositeOperation = 'destination-out';
  // the bridges: a thin band across every letter, a little above the middle
  t.fillRect(0, 40 - size * 0.09, TW, Math.max(2, size * 0.06));
  for (let i = 0; i < 160; i++) { t.fillStyle = `rgba(0,0,0,${rnd() * 0.7})`; t.fillRect(rnd() * TW, rnd() * 80, 1 + rnd() * 3, 1 + rnd() * 2); }
  g.drawImage(c, x - TW / 2, y - 40);
}
