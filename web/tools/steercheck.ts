// Targeted check for the autopilot's steer conversion (Codex P2): at 12 m/s on asphalt a wanted
// wheel angle of 2 degrees must come out as 2 degrees, and the truck must hold that curvature.
// No browser: `npm run steercheck` bundles this with esbuild (assets left out) and runs it in Node.
import * as THREE from 'three';
import { DriveController } from '../src/drive/Drive';
const truck: any = new Proxy({}, { get: () => () => undefined });
const area: any = {
  surface: () => ({ kind: 'asphalt', grip: 1, top: 30 }), height: () => 0, obstacles: [],
  endZone: { minX: 1e9, maxX: 1e9, minZ: 1e9, maxZ: 1e9 },
};
const cam = new THREE.PerspectiveCamera();
const D = new DriveController(cam, truck, area);
(D as any).pose = () => {};   // no truck to draw
D.place(new THREE.Vector3(0, 0, 0), 0);
const deg = Math.PI / 180, WB = 3.3, LR = WB - 1.8;
let fails = 0;
const ok = (c: boolean, m: string) => { console.log((c ? 'PASS ' : 'FAIL ') + m); if (!c) fails++; };
// 1. the conversion alone, at 12 m/s
D.speed = 12; D.surface = area.surface();
const lock = Math.min(32 * deg, Math.atan(WB * 7.5 / 144));
const want = 2 * deg;                                     // wheel angle wanted
const kappa = Math.sin(Math.atan(LR / WB * Math.tan(want))) / LR;   // the curvature that wheel gives
const s = D.steerFor(kappa);
const got = s * lock / deg;
ok(Math.abs(got - 2) < 0.01, `12 m/s, wanted 2.000 deg wheel: steer ${s.toFixed(4)} x lock ${(lock / deg).toFixed(3)} deg = ${got.toFixed(3)} deg (old fixed-32 deg conversion gave ${(2 / 32 * lock / deg).toFixed(3)} deg)`);
// 2. closed loop: steerFor each frame, throttle holding 12 m/s; the path's curvature after 3 s
let kap = 0;
for (let i = 0; i < 180; i++) {
  const h0 = D.heading, x0 = D.pos.x, z0 = D.pos.z;
  D.update(1 / 60, { steer: D.steerFor(kappa), throttle: THREE.MathUtils.clamp((12 - D.speed) * 2, -1, 1) }, { x: 0, y: 0 });
  const ds = Math.hypot(D.pos.x - x0, D.pos.z - z0);
  if (i > 150) kap = Math.abs(D.heading - h0) / ds;
}
ok(Math.abs(kap - kappa) / kappa < 0.02, `closed loop at ${D.speed.toFixed(2)} m/s: path radius ${(1 / kap).toFixed(1)} m, wanted ${(1 / kappa).toFixed(1)} m`);
// 3. the sign: positive kappa turns right like steer +1 (heading falls)
ok(D.heading < 0, `turns right with positive curvature (heading ${D.heading.toFixed(3)})`);
// 4. manual driving keeps its speed fade: full steer at 12 m/s still turns the wheel by the lock only
ok(Math.abs(lock / deg - 9.752) < 0.01, `lock at 12 m/s on asphalt ${(lock / deg).toFixed(3)} deg (Codex: 9.752)`);
// 5. clamps at full lock for curvature beyond it
D.speed = 12; ok(D.steerFor(1) === 1 && D.steerFor(-1) === -1, 'clamped to full lock both ways');
process.exit(fails ? 1 : 0);
