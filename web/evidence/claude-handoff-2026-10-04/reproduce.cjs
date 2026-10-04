#!/usr/bin/env node
'use strict';

// Isolated regression proof. No repository files are written or checked out.
// Run: node reproduce.cjs [repository-directory] [git-ref]
const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const { createHash } = require('node:crypto');
const path = require('node:path');
const vm = require('node:vm');

const repo = path.resolve(process.argv[2] || path.join(__dirname, '../../..'));
const ref = process.argv[3] || 'e5706fb';
const ts = require(path.join(repo, 'web/node_modules/typescript/lib/typescript.js'));
const git = (...args) => execFileSync('git', args, { cwd: repo, encoding: 'utf8' }).trimEnd();
const commit = git('rev-parse', ref);
const sourcePath = 'web/src/core/caseStore.ts';
const source = execFileSync('git', ['show', `${commit}:${sourcePath}`], { cwd: repo, encoding: 'utf8' });
const checkCandidate = process.argv.includes('--check-candidate');
let checkedSource = source;
if (checkCandidate) {
  // Apply the suggested change only to the in-memory string, never the repo.
  const replacements = [
    [
      '  ready: Promise<void>;',
      '  private unresolved = new Map<string, { url: string; ref: string }>();\n  ready: Promise<void>;',
    ],
    [
      '        if (url) { p.url = url; this.stored.set(id, url); } else p.url = missingPrint();',
      '        if (url) { p.url = url; this.stored.set(id, url); } else {\n          const ref = p.url;\n          p.url = missingPrint();\n          this.unresolved.set(p.id, { url: p.url, ref });\n        }',
    ],
    [
      "      if (!p.url.startsWith('data:')) continue;",
      "      if (!p.url.startsWith('data:')) continue;\n      const unresolved = this.unresolved.get(p.id);\n      if (unresolved && p.url === unresolved.url) { p.url = unresolved.ref; continue; }",
    ],
  ];
  for (const [before, after] of replacements) {
    assert.equal(checkedSource.split(before).length, 2, 'Candidate patch must match exactly once');
    checkedSource = checkedSource.replace(before, after);
  }
}
const compiled = ts.transpileModule(checkedSource, {
  compilerOptions: { target: ts.ScriptTarget.ES2020, module: ts.ModuleKind.CommonJS },
}).outputText;
const original = 'data:image/jpeg;base64,T1JJR0lOQUxfUEhPVE8=';
const placeholder = 'data:image/jpeg;base64,UExBQ0VIT0xERVI=';
const settle = () => new Promise(resolve => setImmediate(resolve));

async function scenario(failFirstReadonly) {
  const photos = new Map([['f1', original]]);
  const local = new Map([['s47.case', JSON.stringify({ s: { f1: { id: 'f1', url: 'idb:f1' } }, notes: [] })]]);
  const operations = [];
  const drawnText = [];
  let failuresRemaining = failFirstReadonly ? 1 : 0;

  const db = {
    transaction(storeName, mode) {
      assert.equal(storeName, 'photos');
      const tx = {
        objectStore(name) {
          assert.equal(name, 'photos');
          return {
            get(id) {
              assert.equal(mode, 'readonly');
              const req = {};
              queueMicrotask(() => {
                if (failuresRemaining) {
                  failuresRemaining--;
                  operations.push({ operation: 'get', id, result: 'simulated transaction error', originalStillPresent: photos.get(id) === original });
                  tx.onerror?.({ type: 'error' });
                } else {
                  req.result = photos.get(id);
                  operations.push({ operation: 'get', id, result: req.result });
                  tx.oncomplete?.();
                }
              });
              return req;
            },
            put(value, id) {
              assert.equal(mode, 'readwrite');
              const req = {};
              queueMicrotask(() => {
                photos.set(id, value);
                req.result = id;
                operations.push({ operation: 'put', id, value });
                tx.oncomplete?.();
              });
              return req;
            },
          };
        },
      };
      return tx;
    },
  };

  const exports = {};
  const context = vm.createContext({
    exports,
    localStorage: {
      getItem: key => local.get(key) ?? null,
      setItem: (key, value) => local.set(key, value),
      removeItem: key => local.delete(key),
    },
    indexedDB: {
      open(name, version) {
        assert.equal(name, 's47');
        assert.equal(version, 1);
        const req = { result: db };
        queueMicrotask(() => req.onsuccess?.());
        return req;
      },
    },
    document: {
      createElement(tag) {
        assert.equal(tag, 'canvas');
        return {
          getContext(kind) {
            assert.equal(kind, '2d');
            return { fillRect() {}, fillText(text) { drawnText.push(text); } };
          },
          toDataURL(type, quality) {
            assert.equal(type, 'image/jpeg');
            assert.equal(quality, 0.8);
            return placeholder;
          },
        };
      },
    },
    // Retain production timeout behavior, but do not keep Node alive for the
    // losing Promise.race timer after all assertions finish.
    setTimeout(fn, ms) { const handle = setTimeout(fn, ms); handle.unref(); return handle; },
  });
  vm.runInContext(compiled, context, { filename: `${commit}:${sourcePath}` });

  const before = photos.get('f1');
  assert.equal(before, original, 'Fixture must contain the actual original before load');
  const store = new exports.CaseStore();
  await store.ready;
  const afterLoad = photos.get('f1');
  assert.equal(afterLoad, original, 'A read failure alone must not mutate IndexedDB');
  const loadedCase = store.load();
  const loadedUrl = loadedCase.s.f1.url;
  assert.equal(loadedUrl, failFirstReadonly ? placeholder : original);
  assert.equal(failuresRemaining, 0);
  if (failFirstReadonly) assert.deepEqual(drawnText, ['PRINT NOT AVAILABLE']);

  // A normal later gameplay save, with one unrelated note added.
  loadedCase.notes.push('Next gameplay save');
  store.save(loadedCase);
  for (let i = 0; i < 3; i++) await settle();
  const afterSave = photos.get('f1');
  const expectedAfterSave = failFirstReadonly && !checkCandidate ? placeholder : original;
  assert.equal(afterSave, expectedAfterSave);
  const savedReference = JSON.parse(local.get('s47.case')).s.f1.url;
  assert.equal(savedReference, 'idb:f1');

  // Prove the replacement survives a fresh CaseStore reading successfully.
  const reloaded = new exports.CaseStore();
  await reloaded.ready;
  const afterReload = reloaded.load().s.f1.url;
  assert.equal(afterReload, expectedAfterSave);
  const puts = operations.filter(event => event.operation === 'put');
  assert.equal(puts.length, failFirstReadonly && !checkCandidate ? 1 : 0);
  if (failFirstReadonly && !checkCandidate) assert.equal(puts[0].value, placeholder);

  return {
    scenario: failFirstReadonly ? 'one_readonly_transaction_error' : 'control_successful_read',
    beforeRead: before,
    indexedDbAfterLoad: afterLoad,
    loadedPhotoUrl: loadedUrl,
    indexedDbAfterNextSave: afterSave,
    localStoragePhotoReference: savedReference,
    freshReloadPhotoUrl: afterReload,
    originalOverwritten: afterSave !== original,
    operations,
  };
}

(async () => {
  const results = [await scenario(false), await scenario(true)];
  assert.equal(results[0].originalOverwritten, false);
  assert.equal(results[1].originalOverwritten, !checkCandidate);
  console.log(JSON.stringify({
    outcome: checkCandidate ? 'FIX_CANDIDATE_PASSED' : 'BUG_REPRODUCED',
    commit,
    sourcePath,
    sourceSha256: createHash('sha256').update(source).digest('hex'),
    checkedSourceSha256: createHash('sha256').update(checkedSource).digest('hex'),
    nodeVersion: process.version,
    typescriptVersion: ts.version,
    scope: checkCandidate
      ? 'Suggested patch applied only to an in-memory source string. Mocked browser storage and canvas. No repository changes or browser runtime test.'
      : 'Unmodified git-show source transpiled in memory. Mocked browser storage and canvas. No browser runtime test.',
    results,
  }, null, 2));
})().catch(error => { console.error(error); process.exitCode = 1; });
