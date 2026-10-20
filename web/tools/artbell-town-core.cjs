// Controlled-clock lifecycle tests. Browser QA separately proves real MP3 decode/UI.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const crypto = require('node:crypto');
const ts = require('typescript');
const base = require('./artbell-core.cjs');
let passed = 0;
const check = (ok, message) => { assert.ok(ok, message); ++passed; };
const near = (a, b, message) => check(Math.abs(a - b) < 1e-7, `${message}: ${a} / ${b}`);
const clean = text => text.replace(/\s+/g, ' ').trim();
const raw = JSON.parse(fs.readFileSync('src/assets/audio/artbell-town/catalog.json', 'utf8'));
// Manifest length is deliberately false; the controlled decoder is authoritative.
const catalog = { ...raw, clips: raw.clips.map((clip, i) => ({ ...clip, duration: 99, index: i })) };
const EA = text => ({ who: 'EARL', text });
const first = catalog.clips[0].text, second = catalog.clips[1].text;
const flush = () => new Promise(resolve => setImmediate(resolve));

function fixture({ remote = false, byteSize = 96000 } = {}) {
  const urls = Object.fromEntries(catalog.clips.map(c => [`../assets/audio/artbell-town/${c.file}`,
    remote ? `https://test.invalid/${c.file}` : `data:audio/mpeg;base64,${Buffer.from([c.index]).toString('base64')}`]));
  const f = { requests: [], starts: [], unlocks: 0, decodes: 0, decodeHook: null, fetchHook: null };
  const voice = base.bankModule({}, async (url, options) => {
    f.requests.push([url, options]);
    return f.fetchHook ? f.fetchHook(url, options) : { ok: true, arrayBuffer: async () => new Uint8Array([0]).buffer };
  }, { clips: [] }, {}, catalog, urls);
  const ctx = { currentTime: 100, state: 'running',
    async decodeAudioData(bytes) {
      ++f.decodes;
      return f.decodeHook ? f.decodeHook(bytes) : { duration: 1 + new Uint8Array(bytes)[0] / 10, length: byteSize / 4, numberOfChannels: 1 };
    },
    createBufferSource() {
      return { buffer: null, playbackRate: { value: 1 }, onended: null, stopped: false, disconnected: false,
        connect(node) { this.dest = node; }, disconnect() { this.disconnected = true; },
        start(at) { this.at = at; f.starts.push(this); }, stop() { this.stopped = true; },
        end() { this.onended?.(); } };
    },
  };
  const audio = { ctx, master: {}, radioBus: {}, unlock() { ++f.unlocks; return Promise.resolve(); } };
  const output = {};
  const playback = {};
  vm.runInNewContext(base.compile(fs.readFileSync('src/core/recordedDialogue.ts', 'utf8')),
    { exports: playback, AbortController });
  vm.runInNewContext(base.compile(fs.readFileSync('src/core/earlDialogue.ts', 'utf8')),
    { exports: output, require: name => name.includes('recordedDialogue') ? playback : voice, AbortController });
  f.reader = new output.EarlDialogue(audio); f.reader.enabled = true;
  f.ctx = ctx; f.audio = audio; f.latest = () => f.starts.at(-1);
  return f;
}

function townFixture() {
  const interactions = new Map(), seen = new Set(), captured = [], notes = [], papers = [];
  const ui = { close() { captured.at(-1).onClose?.(); } };
  const town = { proxies: new Proxy({}, { get: () => ({}) }), womanIn: true, smokerIn: true, deputyIn: true };
  const output = {};
  vm.runInNewContext(base.compile(fs.readFileSync('src/story/caliche.ts', 'utf8')), { exports: output,
    require: name => name.includes('Panels') ? { talk: (_ui, options) => { captured.push(options); } }
      : name.includes('portraits') ? { portrait: () => null } : { clockText: () => '02:00' } });
  output.bindTown(town, { inter: { add: item => interactions.set(item.id, item) }, ui, toast() {},
    here: () => true, secs: () => 7200, clock: () => '02:00', note: text => notes.push(text), file: doc => papers.push(doc), seen, save() {} });
  return { interactions, seen, captured, notes, papers, current: () => captured.at(-1) };
}

(async () => {
  const ast = ts.createSourceFile('caliche.ts', fs.readFileSync('src/story/caliche.ts', 'utf8'), ts.ScriptTarget.Latest, true);
  const authored = [];
  function visit(node) {
    if (ts.isCallExpression(node) && node.expression.getText(ast) === 'EA' && ts.isStringLiteralLike(node.arguments[0])) authored.push(clean(node.arguments[0].text));
    ts.forEachChild(node, visit);
  }
  visit(ast);
  check(new Set(authored).size === 16 && catalog.clips.length === 16, 'all 16 exact static Earl lines are represented once');
  check(catalog.clips.every(c => c.id === crypto.createHash('sha256').update(clean(c.text)).digest('hex').slice(0, 16)
    && authored.includes(clean(c.text))), 'catalog IDs and text match the authored Earl corpus');

  let f = fixture();
  check(f.reader.bank.bufferCount === 0 && f.decodes === 0, 'constructing the Earl bank does not decode anything');
  check(f.reader.available([EA('  ' + first.replace(/ /g, '\n ') + '  ')]), 'Earl lookup normalizes whitespace');
  check(!f.reader.available([{ who: 'YOU', text: first }, { who: '', text: first }, { who: 'REYES', text: first }]), 'matching words from Reyes or stage directions cannot borrow Earl recordings');
  check(!f.reader.available([EA(first), EA('Unknown words.')]), 'an exchange with missing Earl words rejects the whole recording');
  check(f.reader.bank.resolve(first.toUpperCase(), 0) === null && f.reader.bank.resolve(base.hostText, 0) === null,
    'Earl requires exact text and cannot borrow radio time templates or host lines');
  f.reader.enabled = false; await f.reader.play([EA(first)]);
  check(f.unlocks === 0 && f.starts.length === 0 && f.reader.state === 'idle', 'Art Bell Off leaves Earl silent');

  f = fixture(); const states = [];
  let play = f.reader.play([{ who: '', text: 'He looks up.' }, EA(first), { who: 'YOU', text: 'A question.' }, EA(second)], s => states.push(s));
  check(f.unlocks === 1 && f.reader.loading, 'unlock begins synchronously inside the caller gesture');
  await flush();
  check(f.decodes === 2 && f.starts.length === 1 && f.reader.playing, 'the current two-line reply loads atomically and starts only one source');
  check(f.latest().dest === f.audio.master && f.latest().dest !== f.audio.radioBus && f.latest().playbackRate.value === 1,
    'nearby speech uses ordinary volume and natural speed without the radio bus');
  check(f.reader.status.text === first && f.reader.status.clips[0].duration === 1,
    'diagnostics report exact spoken text and decoded duration rather than manifest 99');
  f.ctx.state = 'suspended'; await flush();
  check(f.starts.length === 1 && !f.latest().stopped && f.reader.playing, 'a paused context does not consume or advance the queue on wall time');
  f.ctx.state = 'running'; f.ctx.currentTime += 1; f.latest().end(); await flush();
  check(f.starts.length === 2 && f.starts[0].disconnected && f.reader.status.text === second, 'the next Earl sentence waits for the actual previous source end');
  near(f.latest().at, f.ctx.currentTime + .22, 'the small interline pause is scheduled on the audio clock');
  f.latest().end(); await play;
  check(f.reader.state === 'idle' && states.at(-1) === 'idle' && f.reader.status.text === null, 'natural completion returns to replayable idle');
  play = f.reader.play([EA(first)]); await flush();
  check(f.decodes === 2, 'replay reuses the cached exact decoded buffer');
  f.reader.stop(); await play;
  check(f.latest().stopped && f.latest().disconnected && !f.latest().onended, 'Stop releases the active source and its completion callback');

  f = fixture(); const oldStates = [];
  const old = f.reader.play([EA(first), EA(second)], s => oldStates.push(s)); await flush(); const stopped = f.latest();
  play = f.reader.play([EA(second)]); await flush(); await old;
  check(stopped.stopped && f.reader.status.text === second && f.starts.length === 2, 'a new reply cancels old speech and every not-yet-started line');
  const lastOld = oldStates.length; await flush();
  check(oldStates.length === lastOld && f.reader.playing, 'the replaced queue cannot emit stale UI states');
  f.reader.enabled = false; await play;
  check(f.reader.state === 'idle' && f.latest().stopped, 'turning the shared Art Bell choice Off cancels active Earl speech');

  f = fixture({ remote: true }); let aborted = false;
  f.fetchHook = (_url, { signal }) => new Promise((_resolve, reject) => signal.addEventListener('abort', () => { aborted = true; reject(new Error('cancelled')); }));
  play = f.reader.play([EA(first)]); await flush(); f.reader.stop(); await play;
  check(aborted && f.starts.length === 0 && f.reader.state === 'idle', 'close during Pages loading aborts fetch without ghost playback');
  f = fixture(); let release;
  f.decodeHook = () => new Promise(resolve => { release = resolve; });
  play = f.reader.play([EA(first)]); await flush(); f.reader.stop();
  release({ duration: 1, length: 24000, numberOfChannels: 1 }); await play;
  check(f.starts.length === 0 && f.reader.bank.bufferCount === 0 && f.reader.state === 'idle', 'a late non-abortable decode cannot cache, play or change the closed panel');
  f = fixture();
  f.decodeHook = async () => { if (f.decodes === 2) throw new Error('second decode fails'); return { duration: 1, length: 24000, numberOfChannels: 1 }; };
  await f.reader.play([EA(first), EA(second)]);
  check(f.starts.length === 0 && f.reader.state === 'error', 'a failed later sentence never leaves a partial spoken reply');
  f = fixture({ remote: true }); f.fetchHook = async () => ({ ok: false, status: 404 });
  await f.reader.play([EA(first)]);
  check(f.reader.state === 'error' && !f.starts.length, 'an HTTP failure preserves text and exposes a retryable error');
  f = fixture(); play = f.reader.play([EA(first)]); await flush();
  check(!f.requests.length && f.decodes === 1, 'embedded single-file bytes decode without fetch or connect-src');
  f.reader.stop(); await play;
  f = fixture({ byteSize: 8 * 1024 * 1024 });
  await f.reader.bank.load(catalog.clips, f.ctx, new AbortController().signal);
  check(f.reader.bank.bufferCount <= 24 && f.reader.bank.bytes <= 24 * 1024 * 1024, 'the isolated Earl bank retains both existing LRU bounds');

  const t = townFixture(); t.interactions.get('town:amDoor').use();
  check(t.current().earl && t.current().view().lines.some(l => l.who === 'EARL'), 'the real studio-door interaction opts into Earl dialogue');
  const firstOpening = t.current().view().lines;
  check(firstOpening.some(l => !l.who) && firstOpening.filter(l => l.who === 'EARL').length === 1, 'the opening retains its silent stage direction and one spoken Earl line');
  t.current().onChoice('boy');
  check(!t.seen.has('ask:host:boy'), 'unseen boy follow-up cannot bypass its original gates');
  t.current().onChoice('calls');
  check(t.current().view().lines.filter(l => l.who === 'EARL').length === 4 && t.seen.has('ask:host:calls'), 'the authored calls topic retains its four Earl replies and persistence');
  t.current().onChoice('tape'); t.seen.add('ev:boy'); t.current().onChoice('boy');
  check(t.seen.has('ask:host:boy') && t.papers.length === 1 && t.papers[0].id === 'witness-boy', 'the witnessed-boy follow-up still files its existing paper and gate');
  check(t.current().view().lines.filter(l => l.who === 'EARL').length === 4 && t.current().view().lines.some(l => l.who === 'YOU'), 'the boy exchange retains Reyes and stage directions separately from its four Earl lines');
  t.current().onChoice('bye'); t.interactions.get('town:amDoor').use();
  check(t.current().view().lines.length === 1 && t.current().view().lines[0].text === second, 'returning to Earl uses the authored repeat opening');
  for (const id of ['gasWindow', 'sheriffDoor', 'woman', 'smoker']) {
    t.interactions.get('town:' + id).use();
    check(!t.current().earl, `${id} never opts another person into Earl playback`);
  }
  console.log(`artbell-town-core: ${passed}/${passed} PASS`);
})().catch(error => { console.error(error); process.exitCode = 1; });
