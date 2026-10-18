#!/usr/bin/env node
'use strict';
// Tests for src/core/saves.ts in Node, with IndexedDB, localStorage and canvas imitated.
// Covers autosave rotation, manual slots, photographs stored once by content, cleanup of
// photographs no save points at, Codex's lost-photo case (a stand-in must never replace
// the original), moving the first web version's save into case 1, and the localStorage
// fallback when IndexedDB is missing, including a full album in all six slots and quota failures.
// Run from web/: node tools/saves.cjs   (exit code 1 on any failure)
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const web = path.resolve(__dirname, '..');
const ts = require(path.join(web, 'node_modules/typescript/lib/typescript.js'));
const source = fs.readFileSync(path.join(web, 'src/core/saves.ts'), 'utf8');
const compiled = ts.transpileModule(source, {
  compilerOptions: { target: ts.ScriptTarget.ES2022, module: ts.ModuleKind.CommonJS },
}).outputText;

const ORIGINAL = 'data:image/jpeg;base64,T1JJR0lOQUxfUEhPVE8=';
const SECOND = 'data:image/jpeg;base64,U0VDT05EX1BIT1RP';
const STAND_IN = 'data:image/jpeg;base64,UExBQ0VIT0xERVI=';
const LS_PHOTOS = 's47.photo.';
const ROLL = Number(fs.readFileSync(path.join(web, 'src/core/album.ts'), 'utf8').match(/export const ROLL = (\d+);/)[1]);
const localBytes = (local) => [...local].reduce((n, [k, v]) => n + 2 * (k.length + v.length), 0);
const photoEntries = (p) => [...p.local].filter(([k]) => k.startsWith(LS_PHOTOS));
const settle = async () => { for (let i = 0; i < 20; i++) await new Promise((r) => setImmediate(r)); };

// One browser profile: IndexedDB and localStorage survive between SaveStore instances.
function profile({ idb = true, quota = Infinity } = {}) {
  const stores = {};
  const local = new Map();
  let failReads = 0;
  let failLocalReads = 0, failWrite = null, failDelete = null;
  const writes = new Map();
  let now = 1_000_000;
  const db = {
    objectStoreNames: { contains: (n) => n in stores },
    createObjectStore(n) { stores[n] = new Map(); },
    transaction(name, mode) {
      const tx = {};
      const ops = [];
      let failed = false;
      const req = (fn) => { const r = {}; ops.push(() => fn(r)); return r; };
      const store = {
        get: (k) => req((r) => {
          if (mode === 'readonly' && name === 'photos' && failReads > 0) { failReads--; failed = true; return; }
          r.result = structuredClone(stores[name].get(k));
        }),
        put: (v, k) => req((r) => { stores[name].set(k, structuredClone(v)); r.result = k; }),
        delete: (k) => req(() => { stores[name].delete(k); }),
        getAll: () => req((r) => { r.result = [...stores[name].values()].map((v) => structuredClone(v)); }),
        getAllKeys: () => req((r) => { r.result = [...stores[name].keys()]; }),
      };
      tx.objectStore = () => store;
      queueMicrotask(() => queueMicrotask(() => {
        for (const op of ops) op();
        if (failed) tx.onerror?.({ type: 'error' }); else tx.oncomplete?.();
      }));
      return tx;
    },
  };
  const exports = {};
  const ctx = vm.createContext({
    exports, structuredClone, Promise, JSON, Math, Map, Set, Object, Array, String, Error,
    Date: { now: () => now },
    localStorage: {
      get length() { return local.size; },
      key: (i) => [...local.keys()][i] ?? null,
      getItem(k) {
        if (k.startsWith(LS_PHOTOS) && failLocalReads > 0) { failLocalReads--; throw new Error('Read blocked'); }
        return local.get(k) ?? null;
      },
      setItem(k, v) {
        if (k === failWrite) { failWrite = null; throw new Error('Write blocked'); }
        const next = new Map(local); next.set(k, String(v));
        if (localBytes(next) > quota) throw new Error('QuotaExceededError');
        local.set(k, String(v)); writes.set(k, (writes.get(k) ?? 0) + 1);
      },
      removeItem(k) {
        if (k === failDelete) { failDelete = null; throw new Error('Delete blocked'); }
        local.delete(k);
      },
    },
    indexedDB: idb ? { open() { const r = { result: db }; queueMicrotask(() => { r.onupgradeneeded?.(); r.onsuccess?.(); }); return r; } } : undefined,
    document: { createElement: () => ({ getContext: () => ({ fillRect() {}, fillText() {} }), toDataURL: () => STAND_IN }) },
    setTimeout(fn, ms) { const h = setTimeout(fn, ms); h.unref(); return h; },
    queueMicrotask,
  });
  vm.runInContext(compiled, ctx);
  return {
    stores, local, exports, writes,
    failNextReads(n) { failReads = n; },
    failNextLocalReads(n) { failLocalReads = n; },
    failNextWrite(k) { failWrite = k; },
    failNextDelete(k) { failDelete = k; },
    limit(bytes) { quota = bytes; },
    advance(ms) { now += ms; },
    async open(migrate) { const s = new exports.SaveStore(migrate); await s.ready; return s; },
  };
}

const card = (chapter = 'Chapter 1: The Second Exposure') => ({ chapter, place: 'Photo lab', clock: '02:40', playtime: 600, thumb: null });
const caseWith = (url) => ({ v: 2, checkpoint: 'chapter1', area: 'saro', pose: null, case: { s: { f1: { id: 'frame01', url } }, notes: [], clock: 9600 } });
const inlineRecord = (id, state, c = card()) => {
  const [caseId, kind, slot] = id.split(':');
  return { ...c, id, caseId: Number(caseId), kind, slot: Number(slot), savedAt: 1_000_000, photos: [], version: 2, state };
};

const results = [];
async function check(name, fn) {
  try { await fn(); results.push(['PASS', name]); } catch (e) { results.push(['FAIL', name, e.message]); }
}

(async () => {
  await check('autosaves take turns: an update within 90 s, a new generation after that, the oldest is replaced', async () => {
    const p = profile();
    const s = await p.open();
    const a = await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card());
    assert.equal(a.id, '1:auto:0');
    p.advance(30_000);
    assert.equal((await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card())).id, '1:auto:0', 'recent autosave is updated in place');
    s.newGeneration(1);
    p.advance(1000);
    assert.equal((await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card())).id, '1:auto:1', 'a milestone starts a new one');
    p.advance(100_000);
    assert.equal((await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card())).id, '1:auto:2');
    p.advance(100_000);
    assert.equal((await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card())).id, '1:auto:0', 'the oldest goes first');
    assert.equal(s.list(1).filter((m) => m.kind === 'auto').length, 3);
  });

  await check('manual saves keep their own slots, and cases are separate', async () => {
    const p = profile();
    const s = await p.open();
    await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card());
    await s.save(1, 'manual', 2, caseWith(ORIGINAL), card());
    await s.save(3, 'manual', 0, caseWith(SECOND), card());
    assert.equal(JSON.stringify(s.usedCases()), '[1,3]');
    assert.equal(s.freeCase(), 2);
    assert.ok(s.list(1).some((m) => m.id === '1:manual:2'));
    assert.equal(s.list(3).length, 1);
  });

  await check('a photograph is stored once, saves point at it, and loading brings it back', async () => {
    const p = profile();
    const s = await p.open();
    const m1 = await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card());
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    assert.equal(p.stores.photos.size, 1, 'one copy of the print');
    const rec = p.stores.saves.get(m1.id);
    assert.ok(rec.state.case.s.f1.url.startsWith('idb:p'), 'the save holds a reference, not the picture');
    const fresh = await p.open();
    const state = await fresh.load(m1.id);
    assert.equal(state.case.s.f1.url, ORIGINAL);
  });

  await check('a photograph no save points at is deleted, one still in use is kept', async () => {
    const p = profile();
    const s = await p.open();
    const a = await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    const b = await s.save(1, 'manual', 1, caseWith(SECOND), card());
    await s.save(2, 'manual', 0, caseWith(SECOND), card());
    assert.equal(p.stores.photos.size, 2);
    await s.remove(a.id); await settle();
    assert.equal(p.stores.photos.size, 1, 'the first print went with its only save');
    await s.remove(b.id); await settle();
    assert.equal(p.stores.photos.size, 1, 'the second print is still used by case 2');
  });

  await check('a photograph that cannot be read shows the stand-in, and the next save keeps the original', async () => {
    const p = profile();
    const s = await p.open();
    const m = await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card());
    const key = p.stores.saves.get(m.id).state.case.s.f1.url;
    p.failNextReads(2);
    const again = await p.open();
    const state = await again.load(m.id);
    assert.equal(state.case.s.f1.url, STAND_IN);
    await again.save(1, 'manual', 0, state, card());
    assert.equal(p.stores.saves.get('1:manual:0').state.case.s.f1.url, key, 'the save still points at the original');
    assert.equal(p.stores.photos.get(key.slice(4)), ORIGINAL, 'the original is still stored');
    assert.ok(![...p.stores.photos.values()].includes(STAND_IN), 'the stand-in was never stored');
    const fresh = await p.open();
    assert.equal((await fresh.load('1:manual:0')).case.s.f1.url, ORIGINAL);
  });

  await check('a new photograph under the same id replaces the one that could not be read', async () => {
    const p = profile();
    const s = await p.open();
    const m = await s.save(1, 'auto', 'rotate', caseWith(ORIGINAL), card());
    p.failNextReads(2);
    const again = await p.open();
    const state = await again.load(m.id);
    state.case.s.f1 = { id: 'frame01', url: SECOND };
    await again.save(1, 'manual', 0, state, card());
    const fresh = await p.open();
    assert.equal((await fresh.load('1:manual:0')).case.s.f1.url, SECOND);
  });

  await check('the first web version\'s case becomes the newest autosave of case 1', async () => {
    const p = profile();
    // what the old CaseStore left behind: the case in localStorage, the print in IndexedDB
    p.stores.photos = new Map([['frame01', ORIGINAL]]);
    p.stores.saves = new Map();
    p.local.set('s47.checkpoint', JSON.stringify('chapter1'));
    p.local.set('s47.case', JSON.stringify({ s: { f1: { id: 'frame01', url: 'idb:frame01' } }, notes: ['n'], clock: 9000 }));
    const migrate = (checkpoint, old) => ({ state: { v: 2, checkpoint, case: old, area: 'saro', pose: null }, card: card() });
    const s = await p.open(migrate);
    const list = s.list();
    assert.equal(list.length, 1);
    assert.equal(list[0].id, '1:auto:0');
    assert.equal(s.active, 1);
    assert.equal((await s.load('1:auto:0')).case.s.f1.url, ORIGINAL);
    const twice = await p.open(migrate);
    assert.equal(twice.list().length, 1, 'it is moved only once');
  });

  await check('without IndexedDB photographs are stored once by content and survive reopening', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    assert.equal(s.fallback, true);
    const m = await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    const state = caseWith(ORIGINAL);
    state.album = { shots: [{ id: 'a different photo id', url: ORIGINAL }], ghosts: 0 };
    await s.save(1, 'auto', 0, state, card());
    const key = p.exports.photoKey(ORIGINAL);
    assert.equal(photoEntries(p).length, 1);
    assert.equal(JSON.parse(p.local.get(LS_PHOTOS + key)), ORIGINAL);
    const rec = JSON.parse(p.local.get('s47.save.' + m.id));
    assert.equal(rec.state.case.s.f1.url, 'ls:' + key);
    assert.equal(JSON.stringify(rec.photos), JSON.stringify([key]));
    assert.equal(state.album.shots[0].url, ORIGINAL, 'the live game is not changed');
    const fresh = await p.open();
    assert.equal(fresh.list().length, 2);
    assert.equal((await fresh.load(m.id)).case.s.f1.url, ORIGINAL);
    assert.equal((await fresh.load('1:auto:0')).album.shots[0].url, ORIGINAL);
    await fresh.save(1, 'manual', 1, state, card());
    assert.equal(p.writes.get(LS_PHOTOS + key), 1, 'reopening does not write an existing print again');
  });

  await check('old localStorage saves with inline photographs still load and become references when saved again', async () => {
    const p = profile({ idb: false });
    const id = '1:manual:0', old = inlineRecord(id, caseWith(ORIGINAL));
    p.local.set('s47.save.' + id, JSON.stringify(old));
    p.local.set('s47.saves', JSON.stringify([id]));
    const s = await p.open();
    const state = await s.load(id);
    assert.equal(state.case.s.f1.url, ORIGINAL);
    await s.save(1, 'manual', 1, state, card());
    await s.remove('1:manual:1');
    assert.equal(photoEntries(p).length, 0, 'the old save still has its own inline original');
    assert.equal((await s.load(id)).case.s.f1.url, ORIGINAL);
    await s.save(1, 'manual', 0, state, card());
    assert.ok(JSON.parse(p.local.get('s47.save.' + id)).state.case.s.f1.url.startsWith('ls:p'));
    assert.equal((await (await p.open()).load(id)).case.s.f1.url, ORIGINAL);
  });

  await check('localStorage cleanup keeps shared prints across cases and removes them after the last save', async () => {
    const p = profile({ idb: false });
    p.local.set('another-game.photo', ORIGINAL);
    p.local.set('s47.settings', '{"quality":"low"}');
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    await s.save(1, 'manual', 1, caseWith(ORIGINAL), card());
    await s.save(2, 'manual', 0, caseWith(ORIGINAL), card());
    await s.save(1, 'manual', 0, caseWith(SECOND), card());
    assert.equal(photoEntries(p).length, 2, 'overwriting a slot keeps a print still used by other slots');
    const fresh = await p.open();
    await fresh.removeCase(1);
    assert.equal(photoEntries(p).length, 1, 'the other case still uses the original');
    assert.equal((await fresh.load('2:manual:0')).case.s.f1.url, ORIGINAL);
    await fresh.removeCase(2);
    assert.equal(photoEntries(p).length, 0);
    assert.equal(p.local.get('another-game.photo'), ORIGINAL);
    assert.equal(p.local.get('s47.settings'), '{"quality":"low"}');
  });

  await check('overwriting the last localStorage reference frees its print and queued saves keep theirs', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    await s.save(1, 'manual', 0, caseWith(SECOND), card());
    assert.equal(photoEntries(p).length, 1);
    assert.ok(!p.local.has(LS_PHOTOS + p.exports.photoKey(ORIGINAL)));
    await Promise.all([
      s.save(1, 'manual', 1, caseWith(ORIGINAL), card()),
      s.remove('1:manual:0'),
      s.save(1, 'manual', 2, caseWith(SECOND), card()),
    ]);
    const fresh = await p.open();
    assert.equal((await fresh.load('1:manual:1')).case.s.f1.url, ORIGINAL);
    assert.equal((await fresh.load('1:manual:2')).case.s.f1.url, SECOND);
  });

  await check('unreadable localStorage photographs keep their references instead of saving the stand-in', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    const fresh = await p.open();
    p.failNextLocalReads(2);
    const state = await fresh.load('1:manual:0');
    assert.equal(state.case.s.f1.url, STAND_IN);
    await fresh.save(1, 'manual', 1, state, card());
    const key = p.exports.photoKey(ORIGINAL);
    assert.equal(JSON.parse(p.local.get('s47.save.1:manual:1')).state.case.s.f1.url, 'ls:' + key);
    assert.equal(photoEntries(p).length, 1);
    assert.equal((await (await p.open()).load('1:manual:1')).case.s.f1.url, ORIGINAL);
    state.case.s.f1.url = SECOND;
    await fresh.save(1, 'manual', 1, state, card());
    assert.equal((await (await p.open()).load('1:manual:1')).case.s.f1.url, SECOND);
  });

  await check('a missing localStorage photograph can be restored later through the preserved reference', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    const key = LS_PHOTOS + p.exports.photoKey(ORIGINAL), original = p.local.get(key);
    p.local.delete(key);
    const fresh = await p.open();
    const state = await fresh.load('1:manual:0');
    assert.equal(state.case.s.f1.url, STAND_IN);
    await fresh.save(1, 'manual', 1, state, card());
    assert.equal(photoEntries(p).length, 0, 'the stand-in is not a photograph to keep');
    p.local.set(key, original);
    assert.equal((await (await p.open()).load('1:manual:1')).case.s.f1.url, ORIGINAL);
  });

  await check('quota exhaustion rolls back new photographs, keeps the old save and allows a retry', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    const before = new Map(p.local), bytes = localBytes(p.local);
    // The first new print fits, the second does not.
    p.limit(bytes + 2 * ((LS_PHOTOS + p.exports.photoKey(SECOND)).length + JSON.stringify(SECOND).length));
    const state = caseWith(SECOND);
    state.album = { shots: [{ id: 'free1', url: ORIGINAL + 'third' }], ghosts: 0 };
    await assert.rejects(s.save(1, 'manual', 0, state, card()), /Not enough room/);
    assert.deepEqual(p.local, before, 'no orphan photo is left after the failed write');
    assert.equal((await (await p.open()).load('1:manual:0')).case.s.f1.url, ORIGINAL);
    p.limit(Infinity);
    await s.save(1, 'manual', 0, state, card());
    assert.equal((await (await p.open()).load('1:manual:0')).album.shots[0].url, ORIGINAL + 'third');
    assert.equal(photoEntries(p).length, 2);
  });

  for (const key of ['s47.saves', 's47.save.1:manual:1', 's47.save.1:manual:0']) {
    await check(`a failed localStorage write to ${key} preserves existing saves and rolls back new prints`, async () => {
      const p = profile({ idb: false });
      const s = await p.open();
      await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
      const before = new Map(p.local);
      p.failNextWrite(key);
      await assert.rejects(s.save(1, 'manual', key.endsWith(':0') ? 0 : 1, caseWith(SECOND), card()), /Not enough room/);
      assert.deepEqual(p.local, before);
      assert.equal(s.list().length, 1);
      assert.equal((await (await p.open()).load('1:manual:0')).case.s.f1.url, ORIGINAL);
      await s.save(1, 'manual', 1, caseWith(SECOND), card());
      assert.equal((await (await p.open()).load('1:manual:1')).case.s.f1.url, SECOND);
    });
  }

  await check('a failed localStorage deletion keeps the save and its photographs', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    p.failNextDelete('s47.save.1:manual:0');
    await assert.rejects(s.remove('1:manual:0'), /could not be deleted/);
    assert.equal(s.list().length, 1);
    assert.equal(photoEntries(p).length, 1);
    assert.equal((await (await p.open()).load('1:manual:0')).case.s.f1.url, ORIGINAL);
    await s.remove('1:manual:0');
    assert.equal(photoEntries(p).length, 0);
  });

  await check('six localStorage saves with four case prints and a full 36-frame album fit a shared 5 MiB quota', async () => {
    assert.equal(ROLL, 36);
    // Synthetic data URLs sized like case prints (210 KiB) and the smaller fallback album (12 KiB).
    const print = (n, kb) => 'data:image/jpeg;base64,' + String(n).padStart(4, '0') + 'A'.repeat(kb * 1024 - 27);
    const state = caseWith(print(1, 210));
    state.checkpoint = 'chapter3';
    state.case.s.f2 = { id: 'frame02', url: print(2, 210) };
    state.case.ch3 = { f3: { id: 'frame03', url: print(3, 210) }, f4: { id: 'frame04', url: print(4, 210) } };
    state.album = { ghosts: 2, shots: Array.from({ length: ROLL }, (_, n) => ({ id: `free${n + 1}@10000`, url: print(n + 5, 12), clock: 10000 + n, place: 'Service yard', text: 'The antennas.' })) };
    const c = { ...card('Chapter 3: The Survey Station'), thumb: print(0, 8) };
    const slots = ['auto:0', 'auto:1', 'auto:2', 'manual:0', 'manual:1', 'manual:2'];
    const otherKey = 'another-game.save', other = 'X'.repeat(128 * 1024);
    const inline = new Map([[otherKey, other], ['s47.migrated', '2']]);
    for (const slot of slots) inline.set('s47.save.1:' + slot, JSON.stringify(inlineRecord('1:' + slot, state, c)));
    inline.set('s47.saves', JSON.stringify(slots.map((slot) => '1:' + slot)));
    const before = localBytes(inline), quota = 5 * 1024 * 1024;
    assert.ok(before > quota, 'the former inline layout exceeds the quota');
    const p = profile({ idb: false, quota });
    p.local.set(otherKey, other);
    const s = await p.open();
    for (const slot of slots) {
      const [kind, n] = slot.split(':');
      await s.save(1, kind, Number(n), state, c);
    }
    const after = localBytes(p.local);
    assert.equal(s.list(1).length, 6);
    assert.equal(photoEntries(p).length, 4 + ROLL, 'all 40 prints have exactly one copy');
    assert.ok(after < quota);
    const fresh = await p.open();
    for (const slot of slots) {
      const loaded = await fresh.load('1:' + slot);
      assert.equal(JSON.stringify(loaded), JSON.stringify(state), slot + ' restores every print and caption');
      const rec = JSON.parse(p.local.get('s47.save.1:' + slot));
      assert.equal(rec.photos.length, 4 + ROLL);
      assert.ok(rec.state.album.shots.every((shot) => shot.url.startsWith('ls:p')));
    }
    assert.equal(p.local.get(otherKey), other);
    console.log(`SIZE  six saves, 36 album + 4 case prints: inline ${before} bytes -> shared photos ${after} bytes (${(100 * (1 - after / before)).toFixed(1)}% less; UTF-16 keys + values, including another game's 256 KiB)`);
    await fresh.removeCase(1);
    assert.equal(photoEntries(p).length, 0);
    assert.equal(p.local.get(otherKey), other);
  });

  for (const r of results) console.log(r.join('  '));
  const failed = results.filter((r) => r[0] === 'FAIL').length;
  console.log(`${results.length - failed} of ${results.length} PASS`);
  if (failed) process.exitCode = 1;
})();
