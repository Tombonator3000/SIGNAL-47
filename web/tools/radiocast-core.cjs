// Real resolver/Station/NightEvents logic with controlled audio clocks.
// Browser tests separately prove actual MP3 decoding and audibility.
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const fs = require('node:fs');
const vm = require('node:vm');
const THREE = require('three');
const base = require('./artbell-core.cjs');
let count = 0;
const check = (ok, message) => { assert.ok(ok, message); ++count; };
const near = (a, b, message) => check(Math.abs(a - b) < 1e-7, `${message}: ${a} / ${b}`);
const clean = text => text.replace(/\s+/g, ' ').trim();
const hash = (voice, text) => crypto.createHash('sha256').update(voice + '\0' + clean(text)).digest('hex').slice(0, 16);
const raw = JSON.parse(fs.readFileSync('src/assets/audio/radiocast/catalog.json', 'utf8'));
const cast = { ...raw, clips: raw.clips.map((c, index) => ({ ...c, duration: 99, index: 128 + index })) };
const callerFiles = Object.fromEntries(cast.clips.map(c => [`../assets/audio/radiocast/${c.file}`, `data:audio/mpeg;base64,${Buffer.from([c.index]).toString('base64')}`]));
const bank = base.bankModule(base.files, () => { throw new Error('Inline clips never fetch'); }, cast, callerFiles);
const runtime = base.radioModule(bank);
const load = (path, modules) => {
  const exports = {};
  vm.runInNewContext(base.compile(fs.readFileSync(path, 'utf8')), { exports, Math, console, require: modules });
  return exports;
};
const time = load('src/story/time.ts', () => undefined);
const { BULLETIN, NightEvents } = load('src/story/nightEvents.ts', name => name === 'three' ? THREE : time);
const line = voice => ({ who: voice, text: cast.clips.find(c => c.voice === voice).text, by: 'a caller' });
const H = text => ({ who: 'host', text });
const tick = (x, seconds, gameDt = seconds) => {
  if (x.ctx.state === 'running') x.ctx.currentTime += seconds;
  x.radio.update(gameDt, 7200);
};
const ordinary = async x => {
  x.radio.boyCalled = true;
  x.station.start([x.station.cfg.loop[0]], 7200);
  x.station.prev = 7200; x.station.calm = 15;
  await base.flush();
};
function director(x) {
  const seen = new Set(), notes = [], control = { heard: true };
  const input = { ok: true, clock: 7200, cam: new THREE.PerspectiveCamera(), foot: null,
    driving: true, truck: null, radio1420: true, saro: null, silenceFree: false };
  const event = new NightEvents({ audio: x.audio, seen, note: t => notes.push(t), step: () => ({ name: 'step', gain: 1 }),
    setRadio() {}, bulletin: () => x.station.cutIn(BULLETIN, 7200),
    bulletinHeard: () => control.heard, bulletinProgress: () => x.station.archiveStatus,
    openDoor() {}, silence() {}, place: () => 'Highway', silent: () => false });
  return { event, seen, notes, input, control, update: dt => event.update(dt, input) };
}

async function main() {
  check(cast.clips.length === 28 && BULLETIN.length === 6, 'the current manuscript has 22 caller and six archive clips');
  check(cast.clips.every(c => c.id === hash(c.voice, c.text)), 'every caller ID includes its role and exact normalized text');
  const resolver = new bank.RadioVoice('caller');
  check(resolver.bufferCount === 0, 'constructing the caller bank decodes nothing');
  for (const voice of ['woman', 'man', 'old', 'boy', 'news']) {
    const selected = line(voice);
    check(resolver.resolve('  ' + selected.text.replace(/ /g, '\n ') + '  ', 7200, voice)?.[0].voice === voice,
      `${voice} resolves exact whitespace-normalized text only within its role`);
    check(resolver.resolve(selected.text, 7200, voice === 'man' ? 'woman' : 'man') === null,
      `${voice} cannot borrow another role's recording for matching text`);
  }
  check(resolver.resolve('Unknown radio words', 7200, 'news') === null, 'unknown caller words reject the whole clip');
  const cacheClock = new base.Clock();
  await resolver.load(cast.clips, cacheClock, new AbortController().signal);
  check(resolver.bufferCount <= 24 && resolver.bytes <= 24 * 1024 * 1024, 'walking all 28 caller clips keeps both cache limits');
  const host = new bank.RadioVoice();
  await host.load(base.catalog.clips, cacheClock, new AbortController().signal);
  check(host.bufferCount <= 24 && host.bytes <= 24 * 1024 * 1024, 'the independent existing host bank retains its original limits');

  let x = base.setup([H(base.hostText), line('woman'), line('man'), line('old'), line('boy')], bank, true);
  await base.flush();
  check(x.station.hostSources.length === 5 && x.station.spoken.every(l => l.buffers?.length === 1), 'mixed host/callers share one atomic prepared plan');
  check(x.station.hostSources.every(s => s.playbackRate.value === 1 && base.reaches(s, x.radioBus) && base.reaches(s, x.master)),
    'all roles use natural speed and the same AM receiver/radio/master route');
  check(x.station.callerStatus.clips.length === 4 && x.station.hostStatus.clips.length === 1, 'host and caller diagnostics retain separate exact clip identities');
  x.radio.setHostVoice(false); await base.flush();
  check(x.radio.callerVoice && !x.radio.hostVoice && !x.station.spoken[0].buffers && x.station.spoken.slice(1).every(l => l.buffers), 'host Off preserves caller recordings');
  x.radio.setHostVoice(true); x.radio.setCallerVoice(false); await base.flush();
  check(x.radio.hostVoice && !x.radio.callerVoice && x.station.spoken[0].buffers && x.station.spoken.slice(1).every(l => !l.buffers), 'caller Off preserves host recordings');
  x.radio.setCallerVoice(true); await base.flush();
  x.radio.stations.roswell.start([{ kind: 'talk', lines: [line('woman'), H(base.hostText)] }], 18000);
  check(x.radio.stations.roswell.spoken.every(l => !l.clips), 'even exact caller and host text stays procedural on Roswell');

  x = base.setup([line('woman')], bank, true); await base.flush(); tick(x, .6);
  const warmed = x.station.hostSources.slice(), warmedDecodes = x.ctx.decodes, joinedCaptions = [];
  x.radio.onLine = (...args) => joinedCaptions.push(args);
  x.receiver.set(false); x.receiver.update(0);
  x.receiver.set(true); x.receiver.update(0);
  check(warmed.every(s => s.stopped && s.disconnected) && x.station.hostSources.length === 1 && x.ctx.decodes === warmedDecodes,
    'warm caller retuning releases old sources and reuses the exact decoded buffer once');
  near(x.station.hostSources[0].offset, .4, 'warm caller retuning joins its existing sample offset');
  check(joinedCaptions.length === 1 && joinedCaptions[0][1] === line('woman').text, 'warm caller retuning restores one exact caption');
  const secondReceiver = x.radio.receiver({ vol: .2 }); secondReceiver.pos = new THREE.Vector3(); secondReceiver.set(true); secondReceiver.update(0);
  check(x.station.listeners === 2 && x.station.hostSources.length === 1 && joinedCaptions.length === 1, 'two receivers never duplicate a caller source or caption');

  // One failed bank must never leave a partly recorded mixed segment playing.
  x = base.setup([H(base.hostText)], bank, true); await base.flush();
  x.radio.callerBank.load = async () => { throw new Error('Controlled real-loader rejection'); };
  x.station.start([{ kind: 'talk', lines: [H(base.secondText), line('woman')] }], 7200); await base.flush();
  check(!x.station.hostPending && !x.station.hostSources.length && x.station.spoken.every(l => !l.clips && !l.buffers), 'a failed caller load falls back the whole mixed plan before any partial source');

  // Cold archive: elapsed game/wall time is not heard audio time.
  x = base.setup([H(base.hostText)], bank, true); await ordinary(x); tick(x, .6);
  const held = { seg: x.station.seg, t: x.station.t, spoken: x.station.spoken, no: x.station.segNo };
  let release;
  x.ctx.waitDecode = new Promise(resolve => { release = resolve; });
  const d = director(x);
  check(d.event.now('bulletin', d.input), 'the real director accepts the six-line archive on a free station');
  const episode = x.station.archiveStatus.id;
  check(x.station.archiveStatus.state === 'loading' && x.station.hostPending && !x.station.hostSources.length, 'a cold archive is silent until all six recordings are prepared');
  tick(x, 80); d.update(80);
  check(x.station.t === 0 && x.station.archiveStatus.elapsed === 0 && d.event.busy && !d.notes.length && !d.seen.size, '80 cold-loading seconds neither shorten the archive nor file an unheard finding');
  release(); x.ctx.waitDecode = null; await base.flush();
  const sources = x.station.hostSources.slice(), spoken = x.station.spoken;
  check(sources.length === 6 && spoken.every(l => l.buffers?.length === 1), 'the complete archive starts only after all six real resolver/load results');
  check(sources.every(s => s.playbackRate.value === 1) && spoken.every((l, i) => Math.abs(l.secs - sources[i].buffer.duration) < 1e-7), 'archive speech keeps every decoded buffer at its full natural duration');
  near(sources[0].startAt - x.ctx.currentTime, .2, 'archive keeps its initial lead');
  near(sources.at(-1).startAt + sources.at(-1).buffer.duration - x.ctx.currentTime, 46.8, 'rest time is distributed as pauses with a final 0.2 second tail');
  near(x.station.len, 47, 'prepared archive length remains exactly 47 seconds');
  const captions = []; x.radio.onLine = (...args) => captions.push(args);
  tick(x, .21); d.update(.21);
  near(captions[0][3], sources[0].buffer.duration, 'archive caption duration follows the full decoded first sentence');
  x.ctx.state = 'suspended'; d.input.ok = false;
  tick(x, 200, 200); d.update(200);
  near(x.station.archiveStatus.elapsed, .21, 'a suspended context freezes archive progress despite large game ticks');
  check(!d.notes.length && !d.seen.size && d.event.busy, 'a long real pause cannot award the complete finding');
  x.ctx.state = 'running'; d.input.ok = true;
  tick(x, 46.78); d.update(46.78);
  check(x.station.archiveStatus.state === 'playing' && !d.notes.length && d.event.busy, '46.99 heard seconds are still incomplete');
  tick(x, .01); d.update(.01);
  check(x.station.archiveStatus.id === episode && x.station.archiveStatus.state === 'complete' && d.notes.length === 1 && d.seen.has('ne:bulletin1'), 'exactly 47 station seconds complete the same episode and one finding');
  check(x.station.seg === held.seg && x.station.spoken === held.spoken && x.station.segNo === held.no, 'return preserves the original programme plan and decoded buffers');
  near(x.station.t, held.t, 'return restores the precise interrupted position without decode-time drift');
  near(x.station.hostSources[0].offset, held.t - .2, 'the interrupted recorded sentence resumes at its prior sample offset');
  tick(x, .1); d.update(.1);
  check(d.notes.length === 1 && d.seen.size === 1, 'completion cannot award the archive twice');

  // The browser found a 61 ms late render frame skipping unheard held speech.
  x = base.setup([H(base.hostText)], bank, true); await ordinary(x); tick(x, .32);
  const lateHeld = { t: x.station.t, spoken: x.station.spoken, buffer: x.station.hostSources[0].buffer };
  x.station.cutIn(BULLETIN, 7200); await base.flush(); tick(x, 47.06133333333333);
  check(x.station.archiveStatus.state === 'complete' && x.station.archiveStatus.elapsed === 47,
    'a late restart frame still completes exactly 47 archive seconds');
  check(x.station.spoken === lateHeld.spoken && x.station.hostSources[0].buffer === lateHeld.buffer,
    'a late restart retains the held decoded plan and exact buffer');
  near(x.station.t, lateHeld.t, 'late render time cannot consume unheard held speech');
  near(x.station.hostSources[0].offset, lateHeld.t - .2, 'the first resumed source preserves the exact held sample after a late frame');
  near(x.station.calm, 0, 'late archive time cannot count as ordinary airtime');
  tick(x, .1);
  near(x.station.t, lateHeld.t + .1, 'the resumed audio clock advances only from its actual restart');

  x = base.setup([H(base.hostText)], bank, true); await ordinary(x);
  const switched = director(x); switched.event.now('bulletin', switched.input); await base.flush();
  tick(x, .6); switched.update(.6); x.radio.setCallerVoice(false); switched.update(0);
  check(x.station.archiveStatus.state === 'cancelled' && !switched.event.busy && !switched.notes.length && !switched.seen.size,
    'switching archive voice layouts after speech begins cannot award a partially skipped full-listen finding');
  tick(x, 46.4);
  check(x.station.seg === x.station.cfg.loop[0], 'a cancelled mixed listen still returns the held programme at the original 47-second boundary');

  for (const action of ['Off', 'tune', 'reset']) {
    x = base.setup([H(base.hostText)], bank, true); await ordinary(x);
    let releaseLoad;
    x.ctx.waitDecode = new Promise(resolve => { releaseLoad = resolve; });
    x.station.cutIn(BULLETIN, 7200);
    const pending = x.station.hostPending;
    if (action === 'Off') x.radio.setCallerVoice(false);
    else if (action === 'tune') { x.receiver.set(true, 'roswell'); x.receiver.update(0); }
    else x.radio.reset(7200);
    releaseLoad(); x.ctx.waitDecode = null; await base.flush();
    check(pending.signal.aborted && !x.station.hostPending && !x.station.hostSources.length, `${action} during a cold news decode cannot publish stale or future sources`);
  }

  // A cached archive also cancels all its future sources when the last receiver leaves.
  for (const action of ['tune', 'reset', 'seek']) {
    x = base.setup([H(base.hostText)], bank, true); await ordinary(x);
    const e = director(x); check(e.event.now('bulletin', e.input), `${action}: archive starts`); await base.flush();
    tick(x, .6); e.update(.6); const active = [...x.station.hostSources];
    if (action === 'tune') { x.receiver.set(true, 'roswell'); x.receiver.update(0); }
    else if (action === 'reset') x.radio.reset(7200);
    else x.station.start([x.station.cfg.loop[0]], 7200);
    await base.flush(); e.update(100);
    check(active.every(s => s.stopped && s.disconnected) && x.station.archiveStatus.state === 'cancelled', `${action}: current and future archive sources are cancelled`);
    check(!e.notes.length && !e.seen.size && !e.event.busy, `${action}: cancelling or seeking cannot consume the full finding`);
  }

  for (const option of ['host', 'caller']) {
    x = base.setup([H(base.hostText)], bank, true); await ordinary(x); tick(x, .6);
    const before = x.station.t;
    x.station.cutIn(BULLETIN, 7200); await base.flush();
    if (option === 'host') x.radio.setHostVoice(false); else x.radio.setCallerVoice(false);
    await base.flush(); tick(x, 47); await base.flush();
    check(x.station.archiveStatus.state === 'complete' && x.station.seg === x.station.cfg.loop[0], `${option} toggle during the archive still restores the held programme`);
    if (option === 'host') check(x.station.spoken.every(l => !l.buffers), 'held host plan respects host Off on return');
    else check(x.radio.hostVoice && !x.radio.callerVoice && x.station.spoken[0].buffers, 'held host recording survives caller Off on return');
    check(x.station.t > 0 && x.station.t < before + 3, `${option} toggle preserves a corresponding held word position`);
  }

  x = base.setup([H(base.hostText)], bank, true);
  const mixed = { kind: 'talk', lines: [line('man'), H(base.hostText)] };
  x.station.cfg.loop[0] = mixed; await ordinary(x); tick(x, .6);
  const oldCallerSecs = x.station.spoken[0].secs, oldCallerPosition = x.station.t;
  x.station.cutIn(BULLETIN, 7200); await base.flush();
  check(x.station.interrupted.hostVoice && x.station.interrupted.callerVoice, 'the held snapshot records both independently enabled voice options');
  x.radio.setCallerVoice(false); tick(x, 47); await base.flush();
  check(x.station.seg === mixed && !x.station.spoken[0].buffers && x.station.spoken[1].buffers,
    'caller Off during the archive reapplies only the affected held caller while preserving the host');
  near(x.station.t, .2 + (oldCallerPosition - .2) / oldCallerSecs * runtime.secsOf(line('man').text),
    'a held caller changes voice at the corresponding relative word position');

  x = base.setup([H(base.hostText)], bank, true); await ordinary(x);
  const music = x.station.cfg.loop.find(s => s.kind === 'music');
  x.station.start([music], 7200); x.station.calm = 15; tick(x, 7);
  const musicPosition = x.station.t, musicNo = x.station.segNo, loopDuration = x.station.n.music.buffer.duration;
  x.station.cutIn(BULLETIN, 7200); await base.flush(); tick(x, 47.125);
  check(x.station.seg === music && x.station.segNo === musicNo, 'the recorded news archive preserves the interrupted record selection');
  near(x.station.t, musicPosition, 'recorded archive return preserves the exact record programme position');
  near(x.station.n.music.offset, musicPosition % loopDuration, 'the actual fallback music source resumes its previous sample offset');

  // Actual decoded length, not manifest length, decides whether six recordings fit.
  x = base.setup([H(base.hostText)], bank, true); await ordinary(x);
  x.ctx.decodeAudioData = async () => ({ duration: 10, length: 240000, numberOfChannels: 1 });
  x.station.cutIn(BULLETIN, 7200); await base.flush();
  check(x.station.spoken.every(l => !l.buffers && !l.clips) && !x.station.hostSources.length && x.station.len === 47,
    'an overlong decoded archive retains the complete 47-second procedural script without speeding up or partly playing');
  console.log(`radiocast-core: ${count}/${count} PASS`);
}
main().catch(e => { console.error(e); process.exitCode = 1; });
