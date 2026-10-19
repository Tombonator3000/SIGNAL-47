// Focused lifecycle checks without WebGL or a voice service. The browser journey
// in readaloud.py separately exercises the real MP3 decoder and UI.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const ts = require('typescript');

const catalog = { version: 1, voice: 'test', clips: Array.from({ length: 6 }, (_, i) => ({ id: `test${i}`, text: `Line ${i}.`, duration: 1, file: `test${i}.mp3` })) };
const source = fs.readFileSync('src/core/readAloud.ts', 'utf8');
const flush = () => new Promise((resolve) => setImmediate(resolve));
let passed = 0;
const check = (condition, message) => { assert.ok(condition, message); ++passed; };

function fixture({ embedded = false, bytes = 4096 } = {}) {
  const urls = Object.fromEntries(catalog.clips.map((c) => [`../assets/audio/kinski/${c.file}`, embedded ? 'data:audio/mpeg;base64,a2luc2tp' : `https://test.invalid/${c.file}`]));
  const transformed = source.replace(/const FILES = import\.meta\.glob[^\n]+;/, `const FILES = ${JSON.stringify(urls)};`);
  const output = ts.transpileModule(transformed, { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText;
  const f = { requests: 0, decodes: 0, unlocks: 0, starts: [], fetchHook: null, decodeHook: null };
  const exports = {};
  const buffer = () => ({ length: bytes / 4, numberOfChannels: 1, duration: 1 });
  const audio = { master: {}, ctx: {
    state: 'running',
    decodeAudioData: async (data) => { ++f.decodes; return f.decodeHook ? f.decodeHook(data) : buffer(); },
    createBufferSource() {
      return { onended: null, stopped: false, disconnected: false, connect() {},
        start() { f.starts.push(this); }, stop() { this.stopped = true; },
        disconnect() { this.disconnected = true; }, end() { this.onended?.(); } };
    },
  }, unlock() { ++f.unlocks; return Promise.resolve(); } };
  vm.runInNewContext(output, { exports, require: () => ({ default: JSON.stringify(catalog) }),
    AbortController, Float32Array, Uint8Array, atob, console: { warn() {} },
    fetch: async (url, options) => { ++f.requests; return f.fetchHook ? f.fetchHook(url, options) : { ok: true, arrayBuffer: async () => new ArrayBuffer(8) }; },
  });
  f.reader = new exports.ReadAloud(audio); f.reader.enabled = true;
  f.latest = () => f.starts.at(-1);
  f.complete = async (text) => { const play = f.reader.play([text]); await flush(); check(f.reader.playing, `playing ${text}`); f.latest().end(); await play; };
  return f;
}

(async () => {
  let f = fixture();
  check(f.reader.available(['  Line\n 0. ']), 'whitespace is normalized');
  check(!f.reader.available(['Line 0.', 'missing']), 'partial documents are rejected');
  check(!f.reader.available([]), 'empty documents are rejected');
  await f.reader.play(['Line 0.', 'missing']);
  check(f.reader.state === 'error' && f.requests === 0, 'missing text never starts partial audio');
  f.reader.enabled = false; await f.reader.play(['Line 0.']);
  check(f.unlocks === 0 && f.reader.state === 'idle', 'disabled narration stays silent');

  f = fixture(); let states = [];
  let play = f.reader.play(['Line 0.', 'Line 1.'], (state) => states.push(state));
  check(f.unlocks === 1, 'unlock starts inside the synchronous user gesture');
  await flush(); check(f.starts.length === 1 && f.reader.playing, 'one source starts at a time');
  f.latest().end(); await flush();
  check(f.starts.length === 2 && f.starts[0].disconnected, 'next line waits for the previous end');
  f.latest().end(); await play;
  check(f.reader.state === 'idle' && states.at(-1) === 'idle', 'finished queue returns idle');

  f = fixture(); let oldStates = [];
  const oldPlay = f.reader.play(['Line 0.'], (state) => oldStates.push(state)); await flush();
  const oldSource = f.latest(); play = f.reader.play(['Line 1.']); await flush(); await oldPlay;
  check(oldSource.stopped && oldSource.disconnected && !oldSource.onended, 'replacement stops and releases the old source');
  check(oldStates.at(-1) === 'idle' && f.reader.playing, 'old callback cannot overwrite replacement state');
  f.reader.enabled = false; await play;
  check(f.latest().stopped && f.reader.state === 'idle', 'disabling stops playback and resolves play');

  f = fixture(); let aborted = false;
  f.fetchHook = (_url, { signal }) => new Promise((_resolve, reject) => signal.addEventListener('abort', () => { aborted = true; reject(new Error('aborted')); }));
  play = f.reader.play(['Line 0.']); await flush(); f.reader.stop(); await play;
  check(aborted && f.starts.length === 0 && f.reader.state === 'idle', 'stop aborts a pending fetch');

  f = fixture(); let release;
  f.decodeHook = () => new Promise((resolve) => { release = resolve; });
  play = f.reader.play(['Line 0.']); await flush(); f.reader.stop();
  release({ length: 1024, numberOfChannels: 1, duration: 1 }); await play;
  check(f.starts.length === 0 && f.reader.state === 'idle', 'stale decode cannot play or change state');
  f.decodeHook = null; await f.complete('Line 0.');
  check(f.decodes === 2, 'cancelled decode is not cached');

  f = fixture();
  for (let i = 0; i < 5; i++) await f.complete(`Line ${i}.`);
  await f.complete('Line 4.'); check(f.decodes === 5, 'recent clips reuse the decoded buffer');
  await f.complete('Line 0.'); check(f.decodes === 6, 'cache retains at most four decoded clips');

  f = fixture({ bytes: 12 * 1024 * 1024 });
  for (let i = 0; i < 3; i++) await f.complete(`Line ${i}.`);
  await f.complete('Line 0.'); check(f.decodes === 4, 'cache also enforces the 32 MiB budget');

  f = fixture({ embedded: true }); await f.complete('Line 0.');
  check(f.requests === 0 && f.decodes === 1, 'single HTML decodes embedded bytes without connect-src or fetch');
  f = fixture(); f.fetchHook = async () => ({ ok: false, status: 404 });
  await f.reader.play(['Line 0.']); check(f.reader.state === 'error' && f.starts.length === 0, 'HTTP failure reports error');
  f = fixture(); f.decodeHook = async () => { throw new Error('bad MP3'); };
  await f.reader.play(['Line 0.']); check(f.reader.state === 'error' && f.starts.length === 0, 'decode failure reports error');
  console.log(`readaloud-core: ${passed}/${passed} PASS`);
})().catch((error) => { console.error(error); process.exitCode = 1; });
