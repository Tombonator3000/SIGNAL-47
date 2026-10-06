#!/usr/bin/env node
'use strict';

// Pass the actual DINERLIGHT_EXTRA expression as raw UTF-8 JavaScript on stdin.
// This is an API-double observation contract, not a browser or rendering test.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const expression = fs.readFileSync(0, 'utf8');

class Vector {
  constructor(...values) { this.values = values; }
  toArray() { return [...this.values]; }
  clone() { return new Vector(...this.values); }
  fromArray(values) { this.values = [...values]; return this; }
}

function fixture(driving) {
  const camera = {
    position: new Vector(7988.53, 5.789, 2006.15),
    quaternion: new Vector(0, 0, 0, 1),
    projectionMatrix: new Vector(...Array(16).fill(1)),
    matrixWorld: new Vector(...Array(16).fill(2)),
  };
  const player = { pos: new Vector(1.55, 0, 3.7) };
  const flood = {
    key: 'diner', n: 13, count: 1, scale: { value: 0.5 }, skip: new Set(),
    pos: [{ x: 8001, y: 3, z: 2000, w: -4.4 }],
    col: [new Vector(1, 0.3, 0.1)],
  };
  const body = {
    key: 'road_body', n: 2, count: 0, scale: { value: 1 }, skip: new Set(),
    pos: [], col: [],
  };
  const headlights = { set: body, slots: [] };
  const truck = {
    driving, group: { position: new Vector(7988, 0, 2006),
      quaternion: new Vector(0, 0, 0, 1), visible: true },
    body: { visible: true }, cab: { visible: driving }, shell: { visible: !driving },
  };
  const drive = { pos: new Vector(7988, 0, 2006), truck, area: { headlights } };
  const route = [new Vector(8000, 0, 1800), new Vector(8000, 0, 2200)];
  const world = {
    driving, leg: null, legs: {}, truckAt: 'diner', drive,
    area: driving ? 'road' : 'diner', busy: false, autopilot: null,
    testInput: null, oldControl: null,
    diner: { flood, dawn: 0.2, group: { position: new Vector(8000, 0, 2000) } },
    road: { headlights, group: { position: new Vector(8000, 0, 2000) } },
    oldRoad: { headlights, routes: { park: route } },
    mods: { legs: { roadRoute: () => route } },
  };
  const spotPosition = new Vector(8001, 3, 2000);
  const spotTarget = new Vector(8001, 0, 2000);
  const spot = {
    isSpotLight: true, uuid: 'fixture-spot', name: 'fixture', visible: true,
    parent: { visible: true, parent: null },
    getWorldPosition: vector => vector.fromArray(spotPosition.toArray()),
    target: { getWorldPosition: vector => vector.fromArray(spotTarget.toArray()) },
    color: new Vector(1, 0.3, 0.1), intensity: 4.4, distance: 30,
    castShadow: true, angle: 0.5, penumbra: 0.2, decay: 2,
    shadow: { mapSize: new Vector(256, 256), needsUpdate: false,
      autoUpdate: true, map: null },
  };
  const scene = {
    updates: 0,
    updateMatrixWorld() { this.updates++; },
    traverse(visitor) { visitor(spot); },
  };
  const S47 = {
    world, camera, player, scene, hold: true,
    game: { phase: 'ch5', cinematic: false, clock: 19200, ch6: {} },
    ch5: { s: { stage: driving ? 'to-truck' : 'diner', arrived: !driving } },
    ch6: { stage: 'drive' },
    vhs: { picture: 'vhs', ultra: true, on: true, supported: true, glitch: 0 },
    ultra: { on: true, mapped: 1 }, sky: { uniforms: {} },
    renderer: { getPixelRatio: () => 1, domElement: { width: 844, height: 390 },
      toneMapping: 0, toneMappingExposure: 1 },
  };
  const context = vm.createContext({ S47,
    localStorage: { getItem: () => '"ultra"' },
    document: { querySelector: () => ({ textContent: '05:20' }) },
  });
  const pose = () => ({
    driving: world.driving, camera: camera.position.toArray(),
    cameraQuaternion: camera.quaternion.toArray(), player: player.pos.toArray(),
    driver: drive.pos.toArray(), truck: truck.group.position.toArray(),
    truckQuaternion: truck.group.quaternion.toArray(),
    spot: spotPosition.toArray(), target: spotTarget.toArray(),
  });
  return { context, S47, pose };
}

const results = [];
function check(name, fn) {
  try { fn(); results.push({ name, status: 'PASS' }); }
  catch (error) { results.push({ name, status: 'FAIL', error: error.message }); }
}

for (const driving of [true, false]) {
  const role = driving ? 'driving' : 'on_foot_with_separated_camera';
  check(role, () => {
    const env = fixture(driving);
    const before = env.pose();
    const state = vm.runInContext(expression, env.context, { timeout: 1000 });
    // Crossing the VM boundary makes JSON the same interface as page.evaluate.
    const ledger = JSON.parse(JSON.stringify(state.body_light_ledger));
    assert.equal(ledger.driving, driving);
    assert.equal(typeof ledger.driving, 'boolean');
    assert.deepEqual(ledger.camera_xyz, before.camera);
    assert.deepEqual(ledger.player_xyz, before.player);
    assert.notDeepEqual(ledger.camera_xyz, ledger.player_xyz);
    assert.deepEqual(ledger.spots[0].position, before.spot);
    assert.deepEqual(ledger.spots[0].target, before.target);
    assert.deepEqual(env.pose(), before);
    assert.equal(env.S47.scene.updates, 1);
    // Observation arrays must be copies, never aliases of mutable input poses.
    state.body_light_ledger.camera_xyz[0] += 1000;
    state.body_light_ledger.player_xyz[0] += 1000;
    assert.deepEqual(env.pose(), before);
  });
}

const failed = results.filter(result => result.status === 'FAIL').length;
process.stdout.write(JSON.stringify({
  status: failed ? 'FAIL' : 'PASS', tests: results.length,
  passed: results.length - failed, failed, results,
  scope: 'actual DINERLIGHT_EXTRA executed against API doubles; no game or renderer',
}) + '\n');
process.exitCode = failed ? 1 : 0;
