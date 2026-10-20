import * as THREE from 'three';
import { box, cyl, plane, noMerge, mergeStatic } from './kit';
import { canvasTex } from '../core/textures';

// What the records room has for the archive's phone (story/archivePhone.ts; FORSLAG.md, 20 October,
// item 5 and Valg for Tom 1):
//
//   the drawer     a steel fire drawer let into the west wall at the back of the shelf bay where the
//                  STATION 01 box sits pulled half out (Annex.ts, x -5.62, z 8.6, the shelf at 1.135).
//                  Shut, its front is flush with the wall and the box stands in front of it, so it
//                  cannot be seen; out, it stands 0.3 m into the bay with a grey reel box in its tray.
//   the papers     the 1947 box's papers, spilled on the floor by the shelf when the box comes down.
//   the switchboard  SARO's key system unit on the east wall south of the calendar (a cabinet with its
//                  line lamps and the extension card), and under it on a bracket its call recorder,
//                  a small strip printer whose paper hangs out of its front as long as its lines.
//
// Built once into the annex's interior (drawn with the rooms), lit by the annex's own fake lamps
// through `mat` (Annex.mat). Nothing here moves by itself: the story sets the drawer, the lamps and
// the strip.

export interface Archive047 {
  group: THREE.Group;
  /** The drawer, moved along the room's +x as it comes out of the wall (setDrawer). */
  drawer: THREE.Group;
  /** The reel box in its tray (hidden once taken). */
  reel: THREE.Group;
  /** Hit areas (invisible): the drawer standing out, the switchboard, the call recorder. */
  hit: { drawer: THREE.Mesh; ksu: THREE.Mesh; recorder: THREE.Mesh };
  /** The 1947 box's papers on the floor (shown once it has come down). */
  papers: THREE.Group;
  /** Where the drawer's front is (room x): shut, and all the way out. */
  readonly shut: number;
  readonly out: number;
  /** 0 shut, 1 all the way out. */
  setDrawer(k: number): void;
  /** A line lamp on the switchboard: 0 dark, 1 lit. */
  lamp(line: 'trk1' | 'trk2', k: number): void;
  /** The strip out of the recorder, as long as these lines (the newest at the bottom). */
  setStrip(lines: string[]): void;
  /** Where the recorder's print head is, and the switchboard (sounds). */
  readonly recorderAt: THREE.Vector3;
  readonly drawerAt: THREE.Vector3;
}

const DRAWER = { x: -5.994, y: 1.27, z: 8.6, travel: 0.3 };
const KSU = { x: -1.05, y: 1.62, z: 10.22 };
const REC = { x: -1.12, y: 1.0, z: 10.22 };

export function buildArchive047(mat: (p: THREE.MeshStandardMaterialParameters) => THREE.MeshStandardMaterial): Archive047 {
  const group = new THREE.Group();
  group.name = 'archive047';
  const steel = mat({ color: 0x6f777a, roughness: 0.45, metalness: 0.55 });
  const steelDark = mat({ color: 0x4a5154, roughness: 0.55, metalness: 0.5 });
  const beige = mat({ color: 0xcfc3a3, roughness: 0.55 });
  const dark = mat({ color: 0x23262a, roughness: 0.6 });
  const hidden = new THREE.MeshBasicMaterial({ visible: false });

  // ---------- the drawer in the wall ----------
  const drawer = new THREE.Group();
  drawer.position.set(DRAWER.x, DRAWER.y, DRAWER.z);
  // (its local x: 0 at the face of its front plate, the tray behind it to -0.3; a low front, the way a
  // file drawer for reels is made, so what stands in it shows over the front when it is out)
  const lit = mat({ color: 0x8b9497, roughness: 0.42, metalness: 0.5 });
  box(drawer, 0.014, 0.11, 0.27, lit, -0.007, -0.035, 0);                   // front plate (y 1.18 to 1.29)
  box(drawer, 0.02, 0.016, 0.12, steelDark, 0.012, -0.012, 0);             // the pull
  box(drawer, 0.004, 0.034, 0.07, mat({ color: 0xd9d1bb, roughness: 0.9 }), 0.001, -0.06, 0.09);   // an empty card frame
  box(drawer, 0.286, 0.008, 0.25, steelDark, -0.157, -0.088, 0);           // the tray: bottom, sides, back
  box(drawer, 0.286, 0.11, 0.006, steel, -0.157, -0.035, 0.122);
  box(drawer, 0.286, 0.11, 0.006, steel, -0.157, -0.035, -0.122);
  box(drawer, 0.006, 0.15, 0.25, steelDark, -0.297, -0.016, 0);
  // the reel box in the tray, standing on its edge and leaning back, its label to the room: grey
  // board, the number typed, the pencil under it
  const reel = new THREE.Group();
  reel.position.set(-0.16, -0.084, 0);
  reel.rotation.z = 0.3;
  box(reel, 0.032, 0.19, 0.19, mat({ color: 0x8d9290, roughness: 0.85 }), 0, 0.095, 0);
  const label = canvasTex(256, 256, (g, w, h) => {
    g.fillStyle = '#8d9290'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#e9e3d0'; g.fillRect(34, 22, 188, 128);
    g.fillStyle = '#2b2b30'; g.font = '58px "Special Elite", monospace'; g.textAlign = 'center';
    g.fillText('047', 128, 92);
    g.font = '22px "Special Elite", monospace'; g.fillText('10-AUG-79', 128, 130);
    g.fillStyle = '#34343e'; g.font = '30px "Reenie Beanie", cursive';
    g.save(); g.translate(128, 196); g.rotate(-0.05); g.fillText('NOT FOR CATALOGUE', 0, 0); g.restore();
  });
  plane(reel, 0.186, 0.186, mat({ map: label, roughness: 0.9 }), 0.0165, 0.095, 0, Math.PI / 2);
  drawer.add(reel);
  noMerge(reel);
  // the hit area: what of the drawer stands out of the wall (moved with it)
  const hitDrawer = new THREE.Mesh(new THREE.BoxGeometry(0.34, 0.24, 0.32), hidden);
  hitDrawer.position.set(-0.15, 0, 0);
  hitDrawer.name = 'arcDrawer';
  drawer.add(noMerge(hitDrawer));
  mergeStatic(drawer);
  noMerge(drawer);
  group.add(drawer);

  // ---------- the 1947 box's papers on the floor ----------
  const papers = new THREE.Group();
  const sheet = mat({ color: 0xe6dfcb, roughness: 0.9, side: THREE.DoubleSide });
  const typed = mat({ color: 0xded6bf, roughness: 0.9, side: THREE.DoubleSide });
  // (fanned out from where the box came down, towards the room; one half under it)
  for (const [x, z, ry, y, m] of [[-5.12, 8.4, 0.42, 0.004, sheet], [-4.92, 8.8, -0.74, 0.005, typed], [-4.78, 8.52, 1.18, 0.006, sheet],
    [-5.0, 9.02, 2.3, 0.007, typed], [-4.62, 8.94, -1.9, 0.008, sheet], [-4.84, 8.2, 0.9, 0.009, typed]] as const) {
    plane(papers, 0.21, 0.29, m, x, y, z, ry, -Math.PI / 2);
  }
  mergeStatic(papers);
  papers.visible = false;
  noMerge(papers);
  group.add(papers);

  // ---------- the switchboard on the east wall ----------
  const ksu = new THREE.Group();
  ksu.position.set(KSU.x, KSU.y, KSU.z);
  box(ksu, 0.1, 0.46, 0.34, beige, 0, 0, 0);
  const face = canvasTex(256, 352, (g, w, h) => {
    g.fillStyle = '#cdc2a2'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#5a5246'; g.fillRect(14, 14, w - 28, 3);
    g.font = '600 30px Oswald, sans-serif'; g.fillStyle = '#3a342c'; g.fillText('KSU-12', 20, 56);
    g.font = '16px Oswald, sans-serif'; g.fillText('KEY SERVICE UNIT', 20, 78);
    // the line lamps' labels (the lamps themselves are their own little boxes)
    g.font = '15px Oswald, sans-serif';
    ['TRK 1', 'TRK 2', 'INT', 'HOLD'].forEach((t, i) => g.fillText(t, 22 + i * 56, 128));
    // the extension card, typed
    g.fillStyle = '#efe9d6'; g.fillRect(20, 168, w - 40, 150);
    g.fillStyle = '#2b2b30'; g.font = '19px "Special Elite", monospace';
    ['EXT 201  CONTROL ROOM', 'EXT 214  RECORDS', 'EXT 220  OPERATIONS', 'EXT 230  PHOTO LAB', 'TRK 1    OUTSIDE LINE', 'TRK 2    SUPERVISOR']
      .forEach((t, i) => g.fillText(t, 30, 194 + i * 22));
  });
  plane(ksu, 0.34, 0.46, mat({ map: face, roughness: 0.8 }), -0.051, 0, 0, -Math.PI / 2);
  const lamps: Record<'trk1' | 'trk2', THREE.MeshBasicMaterial> = {
    trk1: new THREE.MeshBasicMaterial({ color: 0x2a1a10, toneMapped: false }),
    trk2: new THREE.MeshBasicMaterial({ color: 0x2a1a10, toneMapped: false }),
  };
  // (the face's left, as seen from the room, is the room's -z side)
  const lampZ = (i: number) => -0.17 + (22 + i * 56 + 18) / 256 * 0.34;
  box(ksu, 0.014, 0.026, 0.036, lamps.trk1, -0.055, 0.112, lampZ(0));
  box(ksu, 0.014, 0.026, 0.036, lamps.trk2, -0.055, 0.112, lampZ(1));
  for (const i of [2, 3]) box(ksu, 0.014, 0.026, 0.036, dark, -0.055, 0.112, lampZ(i));
  // its cable up the wall into the ceiling
  box(ksu, 0.03, 0.9, 0.03, dark, 0.02, 0.68, 0.12);
  mergeStatic(ksu);
  noMerge(ksu);
  group.add(ksu);
  const hitKsu = new THREE.Mesh(new THREE.BoxGeometry(0.16, 0.5, 0.38), hidden);
  hitKsu.position.set(KSU.x - 0.03, KSU.y, KSU.z); hitKsu.name = 'arcKsu';
  group.add(hitKsu);

  // ---------- the call recorder on its bracket ----------
  const rec = new THREE.Group();
  rec.position.set(REC.x, REC.y, REC.z);
  box(rec, 0.24, 0.016, 0.4, steelDark, 0.0, -0.008, 0);                    // the bracket shelf
  box(rec, 0.2, 0.09, 0.24, beige, 0.01, 0.045, 0);                          // the recorder
  box(rec, 0.006, 0.012, 0.16, dark, -0.091, 0.07, 0);                       // its paper slot
  box(rec, 0.006, 0.018, 0.05, dark, -0.091, 0.03, 0.08);                    // a little window on the head
  cyl(rec, 0.035, 0.035, 0.16, mat({ color: 0xece6d4, roughness: 0.9 }), 0.03, 0.115, 0, 14).rotation.x = Math.PI / 2;   // the paper roll
  mergeStatic(rec);
  noMerge(rec);
  group.add(rec);
  const hitRec = new THREE.Mesh(new THREE.BoxGeometry(0.28, 0.3, 0.32), hidden);
  hitRec.position.set(REC.x - 0.02, REC.y + 0.0, REC.z); hitRec.name = 'arcRecorder';
  group.add(hitRec);
  // the strip: out of the slot, over the bracket's edge and down; drawn with its lines as print
  const sc = document.createElement('canvas'); sc.width = 64; sc.height = 512;
  const stripTex = new THREE.CanvasTexture(sc); stripTex.colorSpace = THREE.SRGBColorSpace;
  const stripGeo = new THREE.PlaneGeometry(0.056, 1); stripGeo.translate(0, -0.5, 0);
  const strip = new THREE.Mesh(stripGeo, mat({ map: stripTex, roughness: 0.9, side: THREE.DoubleSide }));
  strip.position.set(REC.x - 0.098, REC.y + 0.07, REC.z);
  strip.rotation.set(0, -Math.PI / 2, 0, 'YXZ');
  strip.scale.y = 0.05;
  noMerge(strip);
  group.add(strip);
  const ROW = 0.024, TOP = 0.05;
  const setStrip = (lines: string[]) => {
    const g = sc.getContext('2d')!, n = lines.length, len = TOP + n * ROW + 0.02;
    // the canvas holds the whole strip at 512 px for up to 1 m: as much of it as is out
    const px = Math.min(512, Math.round(len * 512));
    g.clearRect(0, 0, 64, 512);
    g.fillStyle = '#ebe5d3'; g.fillRect(0, 0, 64, px);
    g.fillStyle = 'rgba(40,40,48,0.78)';
    lines.forEach((l, i) => {
      // the print, too small to read from a step away: a row of marks as long as the line
      const y = Math.round((TOP + i * ROW) * 512) + 2, w = Math.min(56, 4 + l.replace(/\s+/g, ' ').length * 1.6);
      for (let x = 4; x < w; x += 3) if (l[(x * 7) % Math.max(1, l.length)] !== ' ') g.fillRect(x, y, 2, 3);
    });
    stripTex.needsUpdate = true;
    // (the plane's v runs from the top of the canvas at the slot down to its bottom)
    stripTex.repeat.set(1, len); stripTex.offset.set(0, 1 - len);
    strip.scale.y = len;
    strip.rotation.x = -0.06 - Math.min(0.12, len * 0.6);   // the longer, the more it curls off the bracket
  };
  setStrip([]);

  return {
    group, drawer, reel, papers,
    hit: { drawer: hitDrawer, ksu: hitKsu, recorder: hitRec },
    shut: DRAWER.x, out: DRAWER.x + DRAWER.travel,
    setDrawer(k: number) { drawer.position.x = DRAWER.x + DRAWER.travel * THREE.MathUtils.clamp(k, 0, 1); },
    lamp(line, k) {
      const m = lamps[line];
      m.color.setRGB(0.14 + 0.86 * k, 0.08 + 0.5 * k, 0.05 + 0.13 * k);
    },
    setStrip,
    recorderAt: new THREE.Vector3(REC.x - 0.06, REC.y + 0.06, REC.z),
    drawerAt: new THREE.Vector3(DRAWER.x + 0.1, DRAWER.y, DRAWER.z),
  };
}
