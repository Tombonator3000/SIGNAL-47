#!/usr/bin/env node
'use strict';
// Tests for src/core/saves.ts in Node, with IndexedDB, localStorage and canvas imitated.
// Covers autosave rotation, manual slots, photographs stored once by content, cleanup of
// photographs no save points at, Codex's lost-photo case (a stand-in must never replace
// the original), moving the first web version's save into case 1, and the localStorage
// fallback when IndexedDB is missing.
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
const settle = async () => { for (let i = 0; i < 20; i++) await new Promise((r) => setImmediate(r)); };

// One browser profile: IndexedDB and localStorage survive between SaveStore instances.
function profile({ idb = true } = {}) {
  const stores = {};
  const local = new Map();
  let failReads = 0;
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
    localStorage: { getItem: (k) => local.get(k) ?? null, setItem: (k, v) => local.set(k, String(v)), removeItem: (k) => local.delete(k) },
    indexedDB: idb ? { open() { const r = { result: db }; queueMicrotask(() => { r.onupgradeneeded?.(); r.onsuccess?.(); }); return r; } } : undefined,
    document: { createElement: () => ({ getContext: () => ({ fillRect() {}, fillText() {} }), toDataURL: () => STAND_IN }) },
    setTimeout(fn, ms) { const h = setTimeout(fn, ms); h.unref(); return h; },
    queueMicrotask,
  });
  vm.runInContext(compiled, ctx);
  return {
    stores, local, exports,
    failNextReads(n) { failReads = n; },
    advance(ms) { now += ms; },
    async open(migrate) { const s = new exports.SaveStore(migrate); await s.ready; return s; },
  };
}

const card = (chapter = 'Chapter 1: The Second Exposure') => ({ chapter, place: 'Photo lab', clock: '02:40', playtime: 600, thumb: null });
const caseWith = (url) => ({ v: 2, checkpoint: 'chapter1', area: 'saro', pose: null, case: { s: { f1: { id: 'frame01', url } }, notes: [], clock: 9600 } });

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

  await check('without IndexedDB the saves go to localStorage with the photograph inline', async () => {
    const p = profile({ idb: false });
    const s = await p.open();
    assert.equal(s.fallback, true);
    const m = await s.save(1, 'manual', 0, caseWith(ORIGINAL), card());
    assert.ok(p.local.has('s47.save.' + m.id));
    const fresh = await p.open();
    assert.equal(fresh.list().length, 1);
    assert.equal((await fresh.load(m.id)).case.s.f1.url, ORIGINAL);
  });

  for (const r of results) console.log(r.join('  '));
  const failed = results.filter((r) => r[0] === 'FAIL').length;
  console.log(`${results.length - failed} of ${results.length} PASS`);
  if (failed) process.exitCode = 1;
})();
