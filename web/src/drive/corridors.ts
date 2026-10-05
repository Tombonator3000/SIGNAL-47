import * as THREE from 'three';
import { mergeGeometries } from 'three/examples/jsm/utils/BufferGeometryUtils.js';
import { mergeStatic, type FloodSet } from '../world/kit';
import { GlowPoints } from '../world/glow';
import { roadMaterials, poles, fences, delineators, wireMesh, type Build } from './roadProps';
import { plants, ranchLights, saroLights } from './roadPlaces';
import { groundGeometry, highwayGeometry, trackGeometry, vergeGeometry } from './roadTerrain';
import { SARO_PATCH, STATION_PATCH, inRect, fromRoad, stationBend } from '../world/geo';

// The stretches of road two areas both draw (world/geo.ts). Where the truck goes from one
// area into the next (legs.ts), everything near the road must be the same on both sides of
// the change, or it jumps in the headlights: so the stretch is built once more from the
// road's own builders, the same vertices and the same random plants, lit by the other
// area's floods, and placed into that area's terms.
//
//   SARO        the highway south from the end of the site's own (site z 36) to past the
//               change at site z 200, with its verges, the ground beside it, the telephone
//               line, the fences, the posts, the plants and the ranch's lights
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

/** South of SARO. Returns the group in SARO's terms, and the posts, poles and big rocks the
 *  truck can hit (road-local circles). */
export function saroCorridor(set: FloodSet) {
  const b = build(set, false), R = SARO_PATCH;
  const keep = (x: number, z: number) => z > -220 && z < 600 && inRect(R, x, z);
  const ground = new THREE.Mesh(mergeGeometries([groundGeometry(R), ...vergeGeometry({ hwy: [SARO_FROM, 600], track: null })])!, b.m.ground);
  ground.name = 'saroCorridorGround';
  b.root.add(ground, new THREE.Mesh(highwayGeometry(SARO_FROM, 600), b.m.asphalt));
  poles(b, keep);
  fences(b, keep);
  delineators(b, (_x, z) => z > -225 && z < 600);
  plants(b, keep);
  ranchLights(b);
  mergeStatic(b.statics);
  b.root.add(b.statics, wireMesh(b), b.glow.build());
  b.root.name = 'saroCorridor';
  fromRoad(b.root, 'saro');
  return { group: b.root, obstacles: circles(b) };
}

/** The last of the survey track at STATION 01, in the station's local terms (a child of its
 *  group), and the big rocks by it (road-local circles). The track's gravel picture must be
 *  loaded first (art.ts, 'gravelTrack'). */
export function stationCorridor(set: FloodSet) {
  const b = build(set, true), R = STATION_PATCH;
  const keep = (x: number, z: number) => inRect(R, x, z);
  const ground = new THREE.Mesh(mergeGeometries([groundGeometry(R), ...vergeGeometry({ hwy: null, track: [STATION_FROM, 588] })])!, b.m.ground);
  ground.name = 'stationCorridorGround';
  const trk = new THREE.Mesh(trackGeometry(STATION_FROM), b.m.gravel);
  bend(ground.geometry); bend(trk.geometry);
  b.root.add(ground, trk);
  plants(b, keep);
  // the telephone line along the highway stands tall on the skyline behind, east
  poles(b, (x, z) => x < 40 && z > 360 && z < 700);
  for (const c of b.root.children) if (c instanceof THREE.InstancedMesh) bendInstances(c);
  // the lights that are seen from the road: the ranch, and SARO's lamps and beacons
  ranchLights(b);
  b.root.add(wireMesh(b), saroLights(b), b.glow.build());
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
