// Real bank, Station and Receiver logic on a controllable audio clock, without WebGL.
// Browser checks separately decode the generated MP3s through the real AM/radio graph.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const ts = require('typescript');
const THREE = require('three');
let passed = 0;
const check = (ok, text) => { assert.ok(ok, text); ++passed; };
const near = (actual, expected, text) => check(Math.abs(actual - expected) < 1e-7, `${text}: ${actual} vs ${expected}`);
const compile = (source) => ts.transpileModule(source, { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText;
const corpus = JSON.parse(fs.readFileSync('production/artbell/corpus.json', 'utf8'));
// Deliberately wrong manifest durations prove that playback and captions use decoded audio.
const catalog = { ...corpus, clips: corpus.clips.map((clip, i) => ({ ...clip, duration: 99, file: `${clip.id}.mp3`, index: i })) };
const files = Object.fromEntries(catalog.clips.map((clip) => [`../assets/audio/artbell/${clip.file}`, `data:audio/mpeg;base64,${Buffer.from([clip.index]).toString('base64')}`]));
function bankModule(urls = files, fetcher = () => { throw new Error('data URLs must never fetch under strict CSP'); }, callerCatalog = { clips: [] }, callerFiles = {}, earlCatalog = { clips: [] }, earlFiles = {}) {
  const source = fs.readFileSync('src/core/radioVoice.ts', 'utf8')
    .replace(/const CALLER_FILES = import\.meta\.glob[^\n]+;/, `const CALLER_FILES = ${JSON.stringify(callerFiles)};`)
    .replace(/const EARL_FILES = import\.meta\.glob[^\n]+;/, `const EARL_FILES = ${JSON.stringify(earlFiles)};`)
    .replace(/const FILES = import\.meta\.glob[^\n]+;/, `const FILES = ${JSON.stringify(urls)};`);
  const result = {};
  vm.runInNewContext(compile(source), { exports: result, atob, Uint8Array, DOMException, fetch: fetcher,
    require: (name) => ({ default: JSON.stringify(name.includes('/artbell-town/') ? earlCatalog : name.includes('/radiocast/') ? callerCatalog : catalog) }) });
  return result;
}
const bank = bankModule(), runtime = {};
const radioSource = fs.readFileSync('src/core/radio.ts', 'utf8').replace("import.meta.env.MODE !== 'single'", 'false');
vm.runInNewContext(compile(radioSource), { exports: runtime, AbortController, console, require: (name) => name === 'three' ? THREE : bank });
const { Radio, sayTime, secsOf, SIGN_OFF } = runtime;
function radioModule(voiceModule) {
  const result = {};
  vm.runInNewContext(compile(radioSource), { exports: result, AbortController, console,
    require: (name) => name === 'three' ? THREE : voiceModule });
  return result;
}
const H = (text) => ({ text, who: 'host' });
const hostText = corpus.clips.find((clip) => clip.sources.some((source) => source.kind === 'static-host')).text;
const secondText = "Dolores, I believe you. Now turn the porch light off and go to bed.";
const actualDuration = (text) => 1 + catalog.clips.find((clip) => clip.text === text).index / 100;
const flush = async () => { for (let i = 0; i < 150; i++) await Promise.resolve(); };

class Clock {
  currentTime = 100; state = 'running'; nodes = []; decodes = 0; waitDecode = null;
  parameter() { return { value: 0, events: [], setValueAtTime(v, t) { this.events.push([v, t]); this.value = v; }, linearRampToValueAtTime(v, t) { this.events.push([v, t]); }, cancelScheduledValues() {}, setTargetAtTime(v) { this.value = v; } }; }
  node(kind) {
    const node = { kind, connections: [], disconnected: false, stopped: false, onended: null,
      gain: this.parameter(), frequency: this.parameter(), Q: this.parameter(), playbackRate: { ...this.parameter(), value: 1 },
      positionX: this.parameter(), positionY: this.parameter(), positionZ: this.parameter(),
      connect(destination) { this.connections.push(destination); },
      disconnect(destination) { if (destination) this.connections = this.connections.filter((x) => x !== destination); else { this.disconnected = true; this.connections = []; } },
      start(at = 0, offset = 0) { this.startAt = at; this.offset = offset; },
      stop() { this.stopped = true; },
    };
    this.nodes.push(node); return node;
  }
  createGain() { return this.node('gain'); }
  createOscillator() { return this.node('oscillator'); }
  createBiquadFilter() { return this.node('filter'); }
  createWaveShaper() { return this.node('shaper'); }
  createBufferSource() { return this.node('source'); }
  createBuffer(channels, length, rate) { const data = new Float32Array(length); return { duration: length / rate, length, numberOfChannels: channels, getChannelData: () => data }; }
  async decodeAudioData(encoded) {
    ++this.decodes;
    if (this.waitDecode) await this.waitDecode;
    const duration = 1 + new Uint8Array(encoded)[0] / 100;
    return { duration, length: Math.round(duration * 24000), numberOfChannels: 1 };
  }
}
function setup(lines = [H(hostText), { text: 'A caller remains the muted horn.', who: 'woman', by: 'a caller' }, H(secondText)], voiceModule = bank, callers = false) {
  const ctx = new Clock(), master = ctx.createGain(), radioBus = ctx.createGain(); radioBus.connect(master);
  const audio = {
    ctx, radioBus, buf: {}, noiseBuffer: () => ({ duration: 3 }),
    staticSrc() {
      const src = ctx.createBufferSource(); src.buffer = this.noiseBuffer(); src.loop = true; src.start();
      return { src, out: src };
    },
    or(name, old = '') { return this.buf[`lic:${name}`] ? `lic:${name}` : old; },
    play(name, options = {}) {
      const buffer = this.buf[name]; if (!buffer) return null;
      const src = ctx.createBufferSource(), gain = ctx.createGain();
      src.buffer = buffer; src.playbackRate.value = options.rate ?? 1; src.loop = !!options.loop;
      gain.gain.value = options.gain ?? 1; src.connect(gain); gain.connect(options.dest ?? radioBus);
      src.start(ctx.currentTime + (options.when ?? 0), options.from ?? 0);
      return { src, gain };
    },
    spatial(_pos, _ref, bus) { const n = ctx.node('panner'); n.connect(bus); return n; },
  };
  const { Radio: RuntimeRadio } = voiceModule === bank ? runtime : radioModule(voiceModule);
  const radio = new RuntimeRadio(audio), station = radio.stations.caliche;
  radio.setCallerVoice(callers);
  radio.setHostVoice(true); station.start([{ kind: 'talk', lines }], 7200); station.prev = 7200;
  const receiver = radio.receiver({ vol: 0.7 }); receiver.pos = new THREE.Vector3(); receiver.set(true);
  receiver.update(0);
  return { ctx, radio, station, receiver, master, radioBus, audio };
}
function reaches(source, target, seen = new Set()) {
  if (source === target) return true;
  if (seen.has(source)) return false; seen.add(source);
  return source.connections.some((next) => reaches(next, target, seen));
}

async function main() {
  const resolver = new bank.RadioVoice();
  check(resolver.bufferCount === 0, 'constructing the bank decodes nothing');
  let timeCoverage = true;
  for (let m = 0; m < 1440; m++) for (const template of corpus.templates) {
    let time = sayTime(m * 60);
    if (template.capitalize) time = time[0].toUpperCase() + time.slice(1);
    const text = template.prefix + time + template.suffix;
    const clips = resolver.resolve(text, m * 60);
    if (!clips || clips.length !== 2 + Number(!!template.before) + Number(!!template.after)) timeCoverage = false;
  }
  check(timeCoverage, 'all 8640 rendered time-template variants resolve without changing a word');
  check(resolver.resolve('It is the wrong text.', 7200) === null, 'unknown words reject the entire recording');
  check(resolver.resolve("It's two-oh-three in Caliche.", 7380).length === 4, 'hour, oh-minute and both fragments compose one time check');
  check(resolver.resolve("It's two-oh-three in Caliche.", 7440) === null, 'a mismatched game clock cannot select a false time check');
  const c = new Clock(), selected = resolver.resolve(hostText, 7200);
  await resolver.load([...selected, ...selected], c, new AbortController().signal);
  check(c.decodes === 1 && resolver.bufferCount === 1, 'only the requested unique clip is decoded');
  await resolver.load(selected, c, new AbortController().signal);
  check(c.decodes === 1, 'a repeated line reuses the bounded buffer cache');
  await resolver.load(catalog.clips, c, new AbortController().signal);
  check(resolver.bufferCount <= 24, 'walking through the bank cannot retain more than 24 buffers');

  let x = setup();
  check(x.station.hostStatus.loading && x.ctx.decodes === 1, 'a live receiver lazily prepares the current talk');
  const said = []; x.radio.onLine = (...args) => said.push(args);
  x.station.update(40, 7200);
  near(x.station.t, 0, 'a slow first decode holds station position');
  check(said.length === 0, 'caption waits for the same prepared recording');
  await flush();
  const sources = [...x.station.hostSources];
  check(sources.length === 2 && x.ctx.decodes === 2, 'only two host lines load; caller stays procedural');
  near(sources[0].startAt, 100.2, 'first host starts at the station lead on the sound clock');
  const first = actualDuration(hostText), caller = secsOf('A caller remains the muted horn.');
  near(sources[1].startAt, 100.2 + first + 0.7 + caller + 0.7, 'next host is scheduled after decoded first length and unchanged caller length');
  check(sources.every((source) => reaches(source, x.radioBus) && reaches(source, x.master)), 'recordings use the AM receiver graph and radio/master buses');
  check(x.ctx.nodes.some((n) => n.type === 'bandpass' && n.frequency.value === 1250) && x.ctx.nodes.some((n) => n.type === 'lowpass' && n.frequency.value === 3400), 'existing narrow AM filters remain in the playback path');
  x.ctx.currentTime += 0.21; x.station.update(80, 7200);
  near(x.station.t, 0.21, 'fast game ticks cannot truncate a real recording');
  check(said.length === 1 && said[0][1] === hostText, 'one matching caption starts with the first real host');
  near(said[0][3], first, 'caption duration comes from decoded audio, not manifest 99');
  x.ctx.state = 'suspended'; x.station.update(100, 7200);
  near(x.station.t, 0.21, 'a suspended audio clock also holds the shared programme');
  x.ctx.state = 'running'; x.ctx.currentTime += 0.25; x.station.update(0.01, 7200);
  near(x.station.t, 0.46, 'resume continues the same source and station position');
  let repeatedSync = 0;
  const sync = x.station.sync.bind(x.station), stableDecodes = x.ctx.decodes, stableNodes = x.ctx.nodes.length;
  x.station.sync = (...args) => { ++repeatedSync; return sync(...args); };
  for (let i = 0; i < 600; i++) { x.ctx.currentTime += 0.001; x.station.update(0.001, 7200); }
  check(repeatedSync === 0 && x.ctx.decodes === stableDecodes && x.ctx.nodes.length === stableNodes &&
    x.station.hostSources.length === sources.length && x.station.hostSources.every((source, i) => source === sources[i] && !source.stopped),
    '600 updates within one recorded line cause zero resyncs, restarts, new nodes or decodes');
  x.station.sync = sync;
  const other = x.radio.receiver({ vol: 0.5 }); other.pos = new THREE.Vector3(); other.set(true); other.update(0);
  check(x.station.listeners === 2 && x.station.hostSources.length === 2 && x.ctx.decodes === 2, 'car and diner share one decode and one playback per line');
  x.receiver.set(false); x.receiver.update(0);
  check(x.station.listeners === 1 && sources.every((source) => !source.stopped), 'turning one receiver off preserves the other receiver programme');
  other.set(false); other.update(0);
  check(x.station.listeners === 0 && sources.every((source) => source.stopped && source.disconnected), 'last receiver leaving stops all current and future recorded sources');
  const captionsBeforeRetune = said.length, decodesBeforeRetune = x.ctx.decodes;
  x.receiver.set(true); x.receiver.update(0);
  near(x.station.hostSources[0].offset, x.station.t - 0.2, 'tuning back joins the existing line at its current offset');
  check(said.length === captionsBeforeRetune + 1 && said.at(-1)[1] === hostText && said.at(-1)[3] === first &&
    x.ctx.decodes === decodesBeforeRetune && !x.station.hostStatus.loading,
    'warm retuning re-announces the exact current recorded caption once without loading or replaying its beginning');
  x.station.update(0, 7200); other.set(true); other.update(0);
  check(said.length === captionsBeforeRetune + 1 && x.station.hostSources.length === sources.length,
    'the next update and a second receiver neither duplicate the joined caption nor restart its sources');
  other.set(false); other.update(0);
  const replacement = [...x.station.hostSources]; x.radio.setHostVoice(false);
  check(!x.radio.hostVoice && !x.station.hostStatus.loading && replacement.every((source) => source.stopped), 'option Off cancels recordings immediately and returns to the horn');

  x = setup([H(hostText)]); const resetPending = x.station.hostPending;
  x.radio.reset(7200); await flush();
  check(resetPending.signal.aborted && x.station.hostSources.length === 0 && !x.station.hostStatus.loading, 'new night/load invalidates a pending decode without ghost sound');
  x = setup([H(hostText)]); const offPending = x.station.hostPending;
  x.radio.setHostVoice(false); await flush();
  check(offPending.signal.aborted && !x.station.hostStatus.playing && x.station.hostSources.length === 0, 'option Off during loading cannot start an old source later');
  x = setup([H(hostText)]); const tunePending = x.station.hostPending;
  x.receiver.set(true, 'roswell'); x.receiver.update(0); await flush();
  check(tunePending.signal.aborted && x.station.hostSources.length === 0, 'tuning away during loading cancels the unheard station');

  x = setup([H(hostText)]); await flush();
  // Event 4 only interrupts the station's own free programme. Keep real recorded sources
  // playing here; a custom talk fixture is deliberately not eligible for the event anymore.
  x.station.start([x.station.cfg.loop[0]], 7200); await flush();
  const before = [...x.station.hostSources];
  const interrupted = x.station.cutIn([{ text: 'An unrelated interruption.', who: 'boy', by: 'a boy' }], 7200);
  check(interrupted === 47 && before.length > 0 && before.every((source) => source.stopped) && x.station.hostSources.length === 0, 'an accepted programme interruption cancels all old host audio');
  check(x.station.spoken.every((line) => line.clips === null), 'the boy and other callers never get the host voice');
  x.radio.stations.roswell.start([{ kind: 'talk', lines: [H(hostText)] }], 18000);
  check(x.radio.stations.roswell.spoken[0].clips === null, 'even host-tagged text on 1230 cannot use Art Bell');
  x.station.reset(SIGN_OFF); await flush();
  check(x.station.spoken.length === 2 && x.station.spoken.every((line) => line.buffers?.length === 1), 'both actual sign-off host lines use the same recorded timeline');

  x = setup([H(hostText), H(secondText)]); await flush();
  const uninterrupted = [...x.station.hostSources], captions = [];
  x.radio.onLine = (...args) => captions.push(args);
  x.ctx.currentTime += 0.21; x.station.update(0, 7200);
  x.ctx.currentTime += first + 0.7; x.station.update(0, 7200);
  check(captions.length === 2 && captions[1][1] === secondText, 'the next caption follows the full first recording and gap');
  near(captions[1][3], actualDuration(secondText), 'the second caption also has its actual decoded duration');
  x.ctx.currentTime = uninterrupted[1].startAt + uninterrupted[1].buffer.duration - 0.001;
  x.station.update(500, 7200);
  check(uninterrupted.every((source) => !source.stopped) && x.station.seg.lines[1].text === secondText, 'the whole second buffer can finish before the segment is replaced');

  x = setup([H(hostText)]); await flush();
  x.radio.boyCalled = false;
  x.station.start([{ kind: 'carrier', len: 0 }], 2 * 3600 + 27 * 60); await flush();
  check(x.radio.boyCalled && x.station.spoken.length === 5, 'the actual once-a-night boy programme is selected');
  check(x.station.spoken.filter((line) => line.buffers).length === 2 && x.station.spoken.filter((line) => line.line.who === 'boy').every((line) => !line.clips), 'both boy-call host replies are recorded while the three boy lines retain their own voice');

  x = setup(); await flush();
  x.station.start([x.station.cfg.loop[0]], 7200); await flush();
  const template = corpus.templates.find((item) => item.prefix.startsWith('Night Line,'));
  const trip = [H((clock) => { const time = sayTime(clock); return template.prefix + (template.capitalize ? time[0].toUpperCase() + time.slice(1) : time) + template.suffix; }), { text: 'A trip caller keeps her own voice.', who: 'woman', by: 'a caller' }];
  const waiting = x.station.slot(trip, 7200), decoded = x.ctx.decodes;
  check(waiting && x.station.where(waiting) === 'waiting' && x.station.unslot(waiting), 'a queued trip can be returned before it begins');
  check(x.ctx.decodes === decoded, 'a cancelled waiting slot never downloads or decodes its host');
  const accepted = x.station.slot(trip, 7200), oldProgramme = [...x.station.hostSources];
  x.ctx.currentTime += x.station.len + 0.1; x.station.update(0, 7200); await flush();
  check(x.station.where(accepted) === 'on' && x.station.spoken[0].buffers?.length === 4, 'accepted trip host uses the same central recorded time-template path');
  check(oldProgramme.every((source) => source.stopped) && x.station.spoken[1].clips === null, 'trip transition releases old host sources and preserves the trip caller');

  x = setup([H(hostText)]); await flush(); x.receiver.set(false); x.receiver.update(0);
  x.radio.hostBank.load = async () => { throw new Error('Simulated HTTP/decode failure'); };
  x.station.start([{ kind: 'talk', lines: [H(secondText)] }], 7200); x.receiver.set(true); x.receiver.update(0); await flush();
  check(!x.station.hostStatus.loading && x.station.spoken[0].clips === null, 'failed loading releases the hold and falls back to a complete procedural line');
  near(x.station.spoken[0].secs, secsOf(secondText), 'fallback duration also matches the original procedural caption');

  let aborted = false;
  const external = { [`../assets/audio/artbell/${selected[0].file}`]: 'https://example.invalid/voice.mp3' };
  const remote = bankModule(external, (_url, options) => new Promise((_resolve, reject) => options.signal.addEventListener('abort', () => { aborted = true; reject(new DOMException('cancelled', 'AbortError')); })));
  const controller = new AbortController(), request = new remote.RadioVoice().load(selected, new Clock(), controller.signal).catch((error) => error.name);
  controller.abort(); check(await request === 'AbortError' && aborted, 'Pages fetch receives the real cancellation signal');
  console.log(`artbell-core: ${passed}/${passed} PASS`);
}
module.exports = { bankModule, radioModule, setup, Clock, compile, corpus, catalog, files, runtime, bank, flush, reaches, hostText, secondText };
if (require.main === module) main().catch((error) => { console.error(error); process.exitCode = 1; });
