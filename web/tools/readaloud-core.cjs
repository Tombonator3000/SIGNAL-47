// Focused lifecycle checks without WebGL or a voice service. The browser journey
// in readaloud.py separately exercises the real MP3 decoder and UI.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const ts = require('typescript');
const crypto = require('node:crypto');

const catalog = { version: 1, voice: 'test', clips: Array.from({ length: 6 }, (_, i) => ({ id: `test${i}`, text: `Line ${i}.`, duration: 1, file: `test${i}.mp3` })) };
const source = fs.readFileSync('src/core/readAloud.ts', 'utf8');
const flush = () => new Promise((resolve) => setImmediate(resolve));
let passed = 0;
const check = (condition, message) => { assert.ok(condition, message); ++passed; };

function fixture({ embedded = false, bytes = 4096, extras = [], inputCatalog, timers } = {}) {
  const localCatalog = inputCatalog ?? { ...catalog, clips: [...catalog.clips, ...extras.map((text, i) => ({ id: `extra${i}`, text, duration: 1, file: `extra${i}.mp3` }))] };
  const urls = Object.fromEntries(localCatalog.clips.map((c) => [`../assets/audio/kinski/${c.file}`, embedded ? 'data:audio/mpeg;base64,a2luc2tp' : `https://test.invalid/${c.file}`]));
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
  vm.runInNewContext(output, { exports, require: () => ({ default: JSON.stringify(localCatalog) }),
    AbortController, Float32Array, Uint8Array, atob, setTimeout: timers?.set ?? setTimeout, clearTimeout: timers?.clear ?? clearTimeout, console: { warn() {} },
    fetch: async (url, options) => { ++f.requests; return f.fetchHook ? f.fetchHook(url, options) : { ok: true, arrayBuffer: async () => new ArrayBuffer(8) }; },
  });
  f.reader = new exports.ReadAloud(audio); f.reader.enabled = true;
  f.audio = audio;
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

  const bank = ['zero', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight', 'nine', 'ten', 'eleven', 'twelve', 'thirteen', 'fourteen', 'fifteen', 'sixteen', 'seventeen', 'eighteen', 'nineteen', 'twenty', 'thirty', 'forty', 'fifty', 'sixty', 'seventy', 'eighty', 'ninety', 'hundred', 'thousand', 'million', 'minus', 'plus', 'point', 'percent'];
  const alphabet = Array.from({ length: 26 }, (_, i) => String.fromCharCode(65 + i));
  f = fixture({ extras: [...bank, ...alphabet, 'frame', 'At', 'Az', 'LY', 'The gate moved.', 'Red apple', 'Red', 'apple tree.', 'Line', 'a free photograph.', 'us'] });
  const segments = (text) => Array.from(f.reader.segments([text]) ?? []);
  check(JSON.stringify(segments('Line 0.')) === JSON.stringify(['Line 0.']), 'whole recording wins over word and numeric fragments');
  check(JSON.stringify(segments('Line 0. Line 1.')) === JSON.stringify(['Line 0.', 'Line 1.']), 'complete recorded sentences can form one description');
  check(JSON.stringify(segments('Red apple tree.')) === JSON.stringify(['Red', 'apple tree.']), 'resolver finds the shorter complete route when longest prefix fails');
  check(!f.reader.available(['Red apple treehouse.']) && !f.reader.available(['Chapter1']), 'unknown words and attached chapter identifiers cannot be split into convenient fragments');
  check(JSON.stringify(segments('At 03:05:47. The gate moved.')) === JSON.stringify(['At', 'three', 'zero', 'five', 'forty', 'seven', 'The gate moved.']), 'clock components compose without dropping the remaining description');
  check(JSON.stringify(segments('-39 LY')) === JSON.stringify(['minus', 'thirty', 'nine', 'LY']), 'negative distance retains its spoken minus');
  check(JSON.stringify(segments('Az 026')) === JSON.stringify(['Az', 'zero', 'two', 'six']), 'leading zeroes in an angle are preserved');
  check(JSON.stringify(segments('1420.405')) === JSON.stringify(['one', 'thousand', 'four', 'hundred', 'twenty', 'point', 'four', 'zero', 'five']), 'frequency decimal digits retain order and zeroes');
  check(JSON.stringify(segments('+25%')) === JSON.stringify(['plus', 'twenty', 'five', 'percent']), 'explicit sign and percent are narrated');
  check(!f.reader.available(['At 3. Unknown continuation.']), 'numeric support cannot hide missing narrative words');
  check(!f.reader.available(['3-5']), 'an unrecorded numeric range is not silently changed into two unrelated numbers');
  check(JSON.stringify(segments('S-03')) === JSON.stringify(['S', 'zero', 'three']) && JSON.stringify(segments('E09A')) === JSON.stringify(['E', 'zero', 'nine', 'A']), 'explicit reference identifiers retain every letter and leading zero');
  check(JSON.stringify(segments('frame01')) === JSON.stringify(['frame', 'zero', 'one']), 'frame identifiers use the recorded frame prefix');
  check(JSON.stringify(segments('A free photograph.')) === JSON.stringify(['a free photograph.']), 'album initial capitalization reuses the same recording');
  check(!f.reader.available(['US']) && !f.reader.available(['Chapter1']), 'capitalization fallback and letter bank do not reinterpret acronyms or arbitrary words');
  play = f.reader.play(['At 3.']); await flush(); const firstFragment = f.latest();
  firstFragment.end(); await flush(); check(f.reader.playing && f.latest() !== firstFragment, 'composed description plays its numeric fragment after the text');
  f.latest().end(); await play; check(f.reader.state === 'idle', 'composed description resolves after the entire reading');
  f = fixture({ extras: ['thirty', 'nine', 'LY'] });
  check(!f.reader.available(['-39 LY']), 'missing minus recording rejects the entire signed number');
  f = fixture({ extras: ['don', 't'] });
  check(!f.reader.available(["don't"]) && !f.reader.available(['don’t']), 'apostrophes inside a word are not fragment boundaries');
  f = fixture({ extras: ['. Outside line. No voice.', '-39 LY'] });
  check(f.reader.available(['Outside line. No voice.']), 'a clock-free observation reuses the same words behind leading sentence punctuation');
  check(!f.reader.available(['side line. No voice.']) && !f.reader.available(['39 LY']), 'punctuation aliases preserve whole words and a recorded minus');
  // A chapter card waits for play(), so a loader that NEVER settles must be
  // bounded independently of a recording's natural playback duration.
  function deadlineClock() {
    const pending = new Map(); let id = 0;
    return { set(fn, ms) { const key = ++id; pending.set(key, { fn, ms }); return key; },
      clear(key) { pending.delete(key); },
      fire() { for (const [key, timer] of [...pending]) { pending.delete(key); timer.fn(); } }, pending };
  }
  for (const stage of ['fetch', 'decode']) {
    let timers = deadlineClock(); f = fixture({ timers }); let signal;
    if (stage === 'fetch') f.fetchHook = (_url, options) => { signal = options.signal; return new Promise(() => {}); };
    else f.decodeHook = () => new Promise(() => {});
    play = f.reader.play(['Line 0.']); await flush();
    check(timers.pending.size === 1 && [...timers.pending.values()][0].ms === 15000, `${stage}: only buffer preparation receives the 15-second deadline`);
    timers.fire(); await play;
    check(f.reader.state === 'error' && !f.starts.length && !f.reader.pending, `${stage}: a never-settling loader times out and resolves play without partial audio`);
    if (stage === 'fetch') check(signal.aborted, 'timed-out Pages fetch receives an abort signal');
    timers = deadlineClock(); f = fixture({ timers });
    if (stage === 'fetch') f.fetchHook = () => new Promise(() => {}); else f.decodeHook = () => new Promise(() => {});
    play = f.reader.play(['Line 0.']); await flush(); f.reader.stop(); await play;
    check(f.reader.state === 'idle' && !timers.pending.size && !f.starts.length, `${stage}: close resolves play immediately even when the loader never returns`);
  }
  const timers = deadlineClock(); f = fixture({ timers });
  play = f.reader.play(['Line 0.']); await flush();
  check(!timers.pending.size && f.reader.playing, 'a playing recording has no load deadline that could truncate a long sentence');
  timers.fire(); check(f.reader.playing && !f.latest().stopped, 'firing every load timer cannot stop an already playing buffer');
  f.latest().end(); await play;

  for (const cancel of [false, true]) {
    const timers = deadlineClock(); f = fixture({ timers });
    f.audio.unlock = () => new Promise(() => {});
    play = f.reader.play(['Line 0.']); await flush();
    if (cancel) f.reader.stop(); else timers.fire();
    await play;
    check(f.reader.state === (cancel ? 'idle' : 'error') && !f.starts.length && !timers.pending.size,
      `a never-settling unlock is ${cancel ? 'immediately cancellable' : 'bounded by the loading deadline'}`);
  }
  {
    const timers = deadlineClock(); f = fixture({ timers }); let late;
    f.decodeHook = () => new Promise(resolve => { late = resolve; });
    play = f.reader.play(['Line 0.']); await flush(); timers.fire(); await play;
    late({ duration: 1, length: 24000, numberOfChannels: 1 }); await flush();
    check(f.reader.state === 'error' && !f.reader.buffers.size && !f.starts.length,
      'a decode completing after its timeout cannot cache, revive narration or hide its error');
  }

  console.log(`readaloud-core: ${passed}/${passed} PASS`);

  if (process.argv.includes('--corpus')) {
    // Text-only coverage: every corpus entry receives a synthetic URL. This
    // checks composition while the offline TTS batch is still producing files;
    // it is deliberately not evidence of a real recording or audio playback.
    const corpus = JSON.parse(fs.readFileSync('production/kinski/corpus.json', 'utf8'));
    const reader = fixture({ inputCatalog: { version: 1, voice: 'text-only fixture', clips: corpus.clips.map((clip) => ({ ...clip, duration: 1, file: `${clip.id}.mp3` })) } }).reader;
    const samples = [
      ['examine-turning', 'S-03. It is turning, the drive motors whining. Az 142, el 32.'],
      ['examine-aligned', 'S-03. Az 026, el 32. Every dish in the array points the same way.'],
      ['examine-scenery', 'S-01. A white dish on a concrete pedestal, the red lamp blinking on the feed legs. Az 142, el 48.'],
      ['examine-negative', 'S-27. Az 026, el -10. Every dish in the array points the same way.'],
      ['note-clock', '03:14. A cardboard alien, standing where nobody would put one. Nobody was there.'],
      ['note-without-clock', 'Outside line. No voice. Room tone, a printer, a heavy thud, a gasp, something ceramic breaking. Then nothing.'],
      ['note-line', '04:15. Three on one line. Drawn in on the map: across the highway just south of the motel, and on to the west-south-west, through STATION 01.'],
      ['photo-clock', 'Exposed from the S-03 apron at 02:47:11. The central antenna, and below it the B-12 reference vane in the lower foreground.'],
      ['photo-control-clock', 'Exposed from the B-12 sight line at 03:12:47, under the shielded, passive control.'],
      ['kit-name', 'Field camera'],
      ['kit-count', '6 case frames, 32 of 36 free frames left'],
      ['kit-note', 'The SARO field camera. C raises it, Space takes the picture: the case frames on sheet film, anything else out of the instant back, into the album.'],
      ['kit-truck', 'Outside the gate at STATION 01'],
      ['person-name', 'Dr. Evelyn Ward'],
      ['person-role', 'SARO, duty officer'],
      ['person-note', 'On the line from the main site. Wants the run checked and logged by the book.'],
      ['album-one', 'The phone.'],
      ['album-two', 'The phone and my coffee.'],
      ['album-ids', 'B-12 and the S-03 motor cabinet.'],
      ['album-meta', 'Control room, 03:14'],
      ['album-meta-field', 'STATION 01, field hut, 04:05'],
      ['negative-distance', 'SOURCE DISTANCE: -39 LY'],
      ['identifier-e09a', 'E09A'],
      ['identifier-p13', 'P13'],
      ['identifier-frame', 'frame01'],
    ];
    const cases = samples.map(([id, text]) => ({ id, text, expected: true, segments: reader.segments([text]) }));
    cases.push({ id: 'unknown-must-fail', text: 'An invented unrecorded platypus description.', expected: false, segments: reader.segments(['An invented unrecorded platypus description.']) });
    const failures = cases.filter((item) => (item.segments !== null) !== item.expected);
    const report = { evidence: 'text-only resolver coverage with synthetic asset URLs; no real audio playback', corpusClips: corpus.clips.length,
      corpusHash: crypto.createHash('sha256').update(JSON.stringify(corpus.clips.map(({ id, text }) => ({ id, text })))).digest('hex'),
      passed: cases.length - failures.length, total: cases.length, unknown: failures.map(({ id, text }) => ({ id, text })), cases };
    fs.writeFileSync('production/kinski/resolver-coverage.json', JSON.stringify(report, null, 2) + '\n');
    console.log(`readaloud text-only corpus: ${report.passed}/${report.total} ${failures.length ? 'FAIL' : 'PASS'}`);
    if (failures.length) { console.error(JSON.stringify(report.unknown, null, 2)); process.exitCode = 1; }
  }
})().catch((error) => { console.error(error); process.exitCode = 1; });
