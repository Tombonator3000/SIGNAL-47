// Real AudioSys scheduling logic with a controllable audio clock, without WebGL.
// Browser checks separately exercise decoded MP3 playback and chapter progression.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const ts = require('typescript');
let passed = 0;
const check = (ok, text) => { assert.ok(ok, text); ++passed; };
const near = (actual, expected, text) => check(Math.abs(actual - expected) < 1e-8, text);
const compile = (text) => ts.transpileModule(text, { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText;
const scripts = {};
vm.runInNewContext(compile(fs.readFileSync('src/core/phoneScripts.ts', 'utf8')), { exports: scripts });
const { PHONE_CALLS, PHONE_TEXTS } = scripts;
const catalog = { clips: [...PHONE_TEXTS, 'Unrelated narration.', 'Another unrelated document.'].map((text, i) => ({ id: `test${i}`, text, duration: 999, file: `test${i}.mp3` })) };
const urls = Object.fromEntries(catalog.clips.map((clip, i) => [`../assets/audio/kinski/${clip.file}`, `data:audio/mpeg;base64,${Buffer.from([i]).toString('base64')}`]));
let source = fs.readFileSync('src/core/Audio.ts', 'utf8');
source = source.replace(/const (STEP_FILES|ALIEN_FILES|PHONE_FILES|CHARACTER_FILES|LICENSED) = import\.meta\.glob[^\n]+;/g, (_match, name) => `const ${name} = ${name === 'PHONE_FILES' ? JSON.stringify(urls) : '{}'};`);
assert.ok(!source.includes('import.meta.glob'), 'asset imports are isolated');
const normalize = (text) => text.replace(/\s+/g, ' ').trim();
class Decoder {
  async decodeAudioData(encoded) {
    const duration = 1 + new Uint8Array(encoded)[0] / 10;
    return { duration, length: duration * 24000, numberOfChannels: 1 };
  }
}
const exportsForAudio = {};
vm.runInNewContext(compile(source), { exports: exportsForAudio, atob, console,
  window: { OfflineAudioContext: Decoder }, require(name) {
    if (name === './phoneScripts') return scripts;
    if (name === './readAloud') return { normalizeReadingText: normalize };
    if (name.endsWith('catalog.json?raw')) return { default: JSON.stringify(name.includes('/ensemble/') ? { clips: [] } : catalog) };
    if (name.endsWith('.mp3')) return { default: 'data:audio/mpeg;base64,AA==' };
    return {};
  },
});

class Clock {
  currentTime = 100; state = 'running'; nodes = [];
  node(kind) {
    const node = { kind, gain: { value: 0 }, frequency: { value: 0 }, playbackRate: { value: 1 },
      disconnected: false, stopped: false, onended: null, startAt: null, stopAt: null,
      connect(destination) { this.destination = destination; },
      disconnect() { this.disconnected = true; },
      start(at) { this.startAt = at; }, stop(at) { this.stopAt = at; this.stopped = true; },
    };
    this.nodes.push(node); return node;
  }
  createGain() { return this.node('gain'); }
  createBiquadFilter() { return this.node('filter'); }
  createBufferSource() { return this.node('source'); }
}

(async () => {
  check(PHONE_TEXTS.length === 24 && new Set(PHONE_TEXTS).size === 24, 'telephone preload bank has 24 unique lines');
  const speech = Object.values(PHONE_CALLS).flat().filter((cue) => !cue.aside);
  check(speech.length === 9 && speech.every((cue) => PHONE_TEXTS.includes(cue.text)), 'all nine new authored call lines are in the bank');
  const audio = new exportsForAudio.AudioSys();
  await audio.preload();
  check(Object.keys(audio.buf).filter((key) => key.startsWith('phone:')).length === 24, 'preload decodes only the 24 telephone clips');
  check(!audio.buf['phone:test24'] && !audio.buf['phone:test25'], 'unrelated narration stays outside the preload');
  const clock = new Clock(); audio.ctx = clock; audio.sfx = clock.createGain(); audio.noise = { duration: 3 };
  check(audio.phoneCall(PHONE_CALLS.ward) === null && clock.nodes.length === 1, 'off by default and no audio nodes created');
  audio.kinskiCalls = true;
  let timeline = audio.phoneCall(PHONE_CALLS.ward);
  near(timeline.cues[0].at, 0.3, 'default lead');
  near(timeline.cues[0].duration, 1, 'actual decoded duration, not manifest 999');
  near(timeline.cues[1].at, 1.75, 'gap after actual first duration');
  near(timeline.duration, 3.05, 'whole call includes lead, gap and trailing click');
  check(timeline.cues[0].by === 'WARD', 'actor stays a separate caption field');
  let session = audio.phoneSession;
  const sources = [...session.sources];
  near(sources[0].startAt, 100.3, 'speech uses the audio clock');
  near(sources[1].startAt, 101.75, 'second source starts at the matching caption cue');
  check(sources.length === 5, 'two voices, hiss and two clicks share the session');
  check(clock.nodes.some((n) => n.type === 'highpass' && n.frequency.value === 320) && clock.nodes.some((n) => n.type === 'lowpass' && n.frequency.value === 3200), 'telephone filter is 320 to 3200 Hz');
  audio.stopPhoneCalls(); audio.stopPhoneCalls();
  check(sources.every((n) => n.stopped && n.disconnected && n.onended === null), 'idempotent hang-up stops current and future sources');
  check(session.nodes.every((n) => n.disconnected), 'hang-up disconnects the complete filter graph');
  audio.phoneCall(PHONE_CALLS.ward); const old = audio.phoneSession;
  timeline = audio.phoneCall(PHONE_CALLS.nora);
  check(old.sources.every((n) => n.stopped && n.disconnected), 'a new call replaces all old sound');
  near(timeline.cues[1].duration, 3.4, 'authored player aside is a fixed silent pause');
  check(timeline.cues[1].aside === true && !timeline.cues[1].by, 'aside caption metadata is retained');
  check(audio.phoneSession.sources.length === 6, 'aside never becomes a voice source');
  const count = clock.nodes.length; delete audio.buf['phone:test1'];
  check(audio.phoneCall(PHONE_CALLS.ward) === null && clock.nodes.length === count, 'missing later clip rejects the whole call before audio allocation');
  check(audio.phoneSession === null, 'failed replacement also releases the previous call');
  audio.buf['phone:test1'] = { duration: 1.1 };
  clock.state = 'suspended'; timeline = audio.phoneCall(PHONE_CALLS.ward, { lead: 0, gap: 0 });
  near(timeline.cues[0].at, 0, 'custom zero lead is respected');
  near(timeline.cues[1].at, 1, 'custom zero gap is respected');
  check(clock.state === 'suspended', 'scheduling never resumes a paused context');
  session = audio.phoneSession;
  const hiss = session.sources.find((n) => n.onended);
  near(hiss.stopAt, 102.3, 'hiss ends on the same pausable clock');
  hiss.onended();
  check(audio.phoneSession === null && session.sources.every((n) => n.disconnected), 'natural end releases all session nodes without a wall timer');
  audio.kinskiCalls = false;
  check(audio.phoneCall(PHONE_CALLS.fieldNora) === null, 'turning the setting off returns the legacy fallback');
  console.log(`phonecalls-core: ${passed}/${passed} PASS`);
})().catch((error) => { console.error(error); process.exitCode = 1; });
