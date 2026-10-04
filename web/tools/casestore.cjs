#!/usr/bin/env node
'use strict';
// Regression test for src/core/caseStore.ts: a photograph that cannot be read from
// IndexedDB must never be replaced by the "PRINT NOT AVAILABLE" stand-in.
// Built on Codex's reproduction in evidence/claude-handoff-2026-10-04/reproduce.cjs, but it
// tests the file in the working tree and expects the fixed behaviour.
// Run from web/: node tools/casestore.cjs   (exit code 1 on any failure)
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const web = path.resolve(__dirname, '..');
const ts = require(path.join(web, 'node_modules/typescript/lib/typescript.js'));
const source = fs.readFileSync(path.join(web, 'src/core/caseStore.ts'), 'utf8');
const compiled = ts.transpileModule(source, {
  compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS },
}).outputText;

const ORIGINAL = 'data:image/jpeg;base64,T1JJR0lOQUxfUEhPVE8=';
const STAND_IN = 'data:image/jpeg;base64,UExBQ0VIT0xERVI=';
const RETAKE = 'data:image/jpeg;base64,UkVUQUtF';
const settle = async () => { for (let i = 0; i < 5; i++) await new Promise((r) => setImmediate(r)); };

// One browser profile: IndexedDB and localStorage survive between CaseStore instances.
function profile() {
  const photos = new Map([['f1', ORIGINAL]]);
  const local = new Map([['s47.case', JSON.stringify({ s: { f1: { id: 'f1', url: 'idb:f1' } }, notes: [] })]]);
  const ops = [];
  let failReads = 0;
  const db = {
    transaction(store, mode) {
      const tx = {
        objectStore() {
          return {
            get(id) {
              const req = {};
              queueMicrotask(() => {
                if (failReads > 0) { failReads--; ops.push(['get-failed', id]); tx.onerror?.({ type: 'error' }); return; }
                req.result = photos.get(id); ops.push(['get', id]); tx.oncomplete?.();
              });
              return req;
            },
            put(value, id) {
              assert.equal(mode, 'readwrite');
              const req = {};
              queueMicrotask(() => { photos.set(id, value); req.result = id; ops.push(['put', id, value]); tx.oncomplete?.(); });
              return req;
            },
          };
        },
      };
      return tx;
    },
  };
  const exports = {};
  const ctx = vm.createContext({
    exports,
    localStorage: { getItem: (k) => local.get(k) ?? null, setItem: (k, v) => local.set(k, v), removeItem: (k) => local.delete(k) },
    indexedDB: { open() { const req = { result: db }; queueMicrotask(() => req.onsuccess?.()); return req; } },
    document: {
      createElement: () => ({ getContext: () => ({ fillRect() {}, fillText() {} }), toDataURL: () => STAND_IN }),
    },
    setTimeout(fn, ms) { const h = setTimeout(fn, ms); h.unref(); return h; },
  });
  vm.runInContext(compiled, ctx);
  return {
    photos, local, ops,
    failNextReads(n) { failReads = n; },
    async open() { const s = new exports.CaseStore(); await s.ready; return s; },
    savedRef: () => JSON.parse(local.get('s47.case')).s.f1.url,
  };
}

const results = [];
async function check(name, fn) {
  try { await fn(); results.push(['PASS', name]); } catch (e) { results.push(['FAIL', name, e.message]); }
}

(async () => {
  await check('a normal load and save keeps the original and writes nothing', async () => {
    const p = profile();
    const s = await p.open();
    const c = s.load();
    assert.equal(c.s.f1.url, ORIGINAL);
    c.notes.push('later save'); s.save(c); await settle();
    assert.equal(p.photos.get('f1'), ORIGINAL);
    assert.equal(p.savedRef(), 'idb:f1');
    assert.equal(p.ops.filter((o) => o[0] === 'put').length, 0);
  });

  await check('one failed read is retried and the original still loads', async () => {
    const p = profile();
    p.failNextReads(1);
    const s = await p.open();
    assert.equal(s.load().s.f1.url, ORIGINAL);
  });

  await check('a photo that cannot be read shows the stand-in, and the next save keeps the original', async () => {
    const p = profile();
    p.failNextReads(2);
    const s = await p.open();
    const c = s.load();
    assert.equal(c.s.f1.url, STAND_IN, 'the stand-in is shown while the photo cannot be read');
    c.notes.push('later save'); s.save(c); await settle();
    assert.equal(p.photos.get('f1'), ORIGINAL, 'IndexedDB still holds the original');
    assert.equal(p.savedRef(), 'idb:f1', 'localStorage still points at the original');
    assert.equal(p.ops.filter((o) => o[0] === 'put').length, 0, 'nothing written over the photo');
    const again = await p.open();
    assert.equal(again.load().s.f1.url, ORIGINAL, 'a fresh load reads the original again');
  });

  await check('a new photograph under the same id still replaces the old one', async () => {
    const p = profile();
    p.failNextReads(2);
    const s = await p.open();
    const c = s.load();
    c.s.f1 = { id: 'f1', url: RETAKE };
    s.save(c); await settle();
    assert.equal(p.photos.get('f1'), RETAKE);
    assert.equal(p.savedRef(), 'idb:f1');
    const again = await p.open();
    assert.equal(again.load().s.f1.url, RETAKE);
  });

  for (const r of results) console.log(r.join('  '));
  const failed = results.filter((r) => r[0] === 'FAIL').length;
  console.log(`${results.length - failed} of ${results.length} PASS`);
  if (failed) process.exitCode = 1;
})();
