import * as THREE from 'three';
import { mergeStatic, type FloodSet } from '../world/kit';
import { GlowPoints } from '../world/glow';
import { roadMaterials, poles, fences, delineators, signs, cattleGuard, wireMesh, headlightGlints, ranchRoad, culverts, highwayWear, type Build, type Glint } from './roadProps';
import { wearMesh } from './roadSurface';
import { plants, ranchLights, saroLights, groundWithTufts } from './roadPlaces';
import { gravelTex } from './roadTextures';
import { groundGeometry, highwayGeometry, trackGeometry, vergeGeometry, surfaceY, trackNearest } from './roadTerrain';
import { SARO_PATCH, STATION_PATCH, inRect, fromRoad, stationBend } from '../world/geo';
import { Herd } from './herd';

// The stretches of road two areas both draw (world/geo.ts). Where the truck goes from one
// area into the next (legs.ts), everything near the road must be the same on both sides of
// the change, or it jumps in the headlights: so the stretch is built once more from the
// road's own builders, the same vertices and the same random plants, lit by the other
// area's floods, and placed into that area's terms.
//
//   SARO        the highway south from the end of the site's own (site z 36), past Sierra
//               Motor Court (SARO's own, MotelFront.ts) to past the change 60 m south of
//               it (site z 785), with its verges, the ground beside it, the telephone line,
//               the fences, the posts and signs, the mouth of the survey track with its
//               cattle guard and culvert, the ranch's road with its gate and mailbox, the wear
//               on the asphalt, the plants, the ranch's lights and the herd in the pasture
//               between the motel and the track (herd.ts)
//   STATION 01  the survey track's last 400 m to the gate, its verges, the ground round it,
//               the plants, the telephone line along the highway behind, the ranch's
//               lights and SARO's; the land is brought down to the station's flat ground
//               over the last 40 m (geo.ts)

/** Road-local z where the site's own highway ends and this stretch begins (site z 36). */
export const SARO_FROM = -264;
/** Arc length along the survey track where the station's stretch begins. */
export const STATION_FROM = 200;

function build(set: FloodSet, gravelArt: boolean): Build {
  return { root: new THREE.Group(), statics: new THREE.Group(), glow: new GlowPoints(), m: roadMaterials(set, gravelArt), obstacles: [], wires: [] };
}
const circles = (b: Build) => b.obstacles.filter((o): o is Extract<typeof o, { kind: 'circle' }> => o.kind === 'circle');

/** How much of the survey track SARO's stretch has (arc length from the highway). */
const SARO_TRACK = 100;
/** South of SARO. Returns the group in SARO's terms, the posts, poles, signs and big rocks the
 *  truck can hit (road-local circles), glints(), which lights the delineators' reflectors and
 *  the signs in the beam of headlights in these flood slots (every frame of a drive here),
 *  trackArt(), which lays the track's own gravel picture on it once that is loaded (with the
 *  first drive, World.ts), and the herd (the same as the road's, in SARO's floods). */
export function saroCorridor(set: FloodSet) {
  const b = build(set, false), R = SARO_PATCH;
  const keep = (x: number, z: number) => z > -220 && z < 600 && inRect(R, x, z);
  const ground = groundWithTufts([groundGeometry(R), ...vergeGeometry({ hwy: [SARO_FROM, 600], track: [0, SARO_TRACK] })], b.m.ground, 'saroCorridorGround', keep);
  const gravel = b.m.gravel;
  b.root.add(ground, new THREE.Mesh(highwayGeometry(SARO_FROM, 600), b.m.asphalt), new THREE.Mesh(trackGeometry(0, SARO_TRACK), gravel));
  poles(b, keep);
  fences(b, keep);
  signs(b, keep);
  cattleGuard(b);
  ranchRoad(b, keep);
  culverts(b, keep);
  b.root.add(wearMesh(highwayWear(keep), surfaceY, set));
  const glints = delineators(b, (_x, z) => z > -225 && z < 600);
  plants(b, keep);
  ranchLights(b);
  const herd = new Herd(b, set, 'saro');
  mergeStatic(b.statics);
  b.root.add(b.statics, wireMesh(b), b.glow.build());
  b.root.name = 'saroCorridor';
  fromRoad(b.root, 'saro');
  const o = b.root.position;
  const trackArt = () => {
    if (gravel.map) return;
    gravel.map = gravelTex(); gravel.color.set(0xf2ece0); gravel.needsUpdate = true;
  };
  const at = glints.map((g): Glint => ({ i: g.i, p: g.p.clone().add(o) }));
  return { group: b.root, obstacles: circles(b), glints: (slots: readonly number[]) => headlightGlints(set, slots, b.glow, at), trackArt, herd };
}

/** The last of the survey track at STATION 01, in the station's local terms (a child of its
 *  group), and the big rocks by it (road-local circles). The track's gravel picture must be
 *  loaded first (art.ts, 'gravelTrack'). */
export function stationCorridor(set: FloodSet) {
  const b = build(set, true), R = STATION_PATCH;
  const keep = (x: number, z: number) => inRect(R, x, z);
  const ground = groundWithTufts([groundGeometry(R), ...vergeGeometry({ hwy: null, track: [STATION_FROM, 588] })], b.m.ground, 'stationCorridorGround',
    (x, z) => { const t = trackNearest(x, z); return keep(x, z) && t.d < 12 && t.s >= STATION_FROM; });
  const trk = new THREE.Mesh(trackGeometry(STATION_FROM), b.m.gravel);
  bend(ground.geometry); bend(trk.geometry);
  b.root.add(ground, trk);
  plants(b, keep);
  // the telephone line along the highway stands tall on the skyline behind, east
  poles(b, (x, z) => x < 40 && z > 360 && z < 700);
  for (const c of b.root.children) if (c instanceof THREE.InstancedMesh) bendInstances(c);
  // the lights that are seen from the road: the ranch, and SARO's lamps and beacons
  ranchLights(b);
  saroLights(b);
  b.root.add(wireMesh(b), b.glow.build());
  b.root.name = 'stationCorridor';
  fromRoad(b.root, 'station01');
  return { group: b.root, obstacles: circles(b) };
}
// the station's copy of the land comes down to its flat ground by the gate
function bend(g: THREE.BufferGeometry) {
  const p = g.attributes.position as THREE.BufferAttribute;
  for (let i = 0; i < p.count; i++) p.setY(i, p.getY(i) - stationBend(p.getX(i), p.getZ(i)));
  p.needsUpdate = true;
  g.computeBoundingSphere();
}
function bendInstances(m: THREE.InstancedMesh) {
  const t = new THREE.Matrix4();
  for (let i = 0; i < m.count; i++) {
    m.getMatrixAt(i, t);
    t.elements[13] -= stationBend(t.elements[12], t.elements[14]);
    m.setMatrixAt(i, t);
  }
  m.instanceMatrix.needsUpdate = true;
  m.computeBoundingSphere();
}
