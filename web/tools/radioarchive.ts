// Event 4's interruption, using the real station and bulletin without a browser or sound device.
// Bundle with esbuild for Node and --define:import.meta.env.MODE='"single"', then run the output.
import { Radio, ARCHIVE_SECS, SIGN_OFF, secsOf } from '../src/core/radio';
import { BULLETIN, NightEvents, type NightIn } from '../src/story/nightEvents';
import * as THREE from 'three';

let failed = 0, checks = 0;
const check = (ok: boolean, what: string) => {
  checks++; if (!ok) failed++;
  console.log(`${ok ? 'PASS' : 'FAIL'} ${what}`);
};
const near = (a: number, b: number) => Math.abs(a - b) < 1e-8;
const CLOCK = 2 * 3600 + 40 * 60;
const oldRandom = Math.random;
Math.random = () => 0;
function night(clock = CLOCK, boyCalled = true) {
  const r = new Radio({ ctx: null } as any);
  r.boyCalled = boyCalled;
  r.reset(clock);
  return r;
}
function snapshot(r: Radio) {
  const s = r.stations.caliche as any;
  return { seg: s.seg, t: s.t, said: s.said, sc: s.sc, segNo: s.segNo, loopAt: s.loopAt };
}
function director(len = ARCHIVE_SECS) {
  const seen = new Set<string>(), notes: string[] = [], plays: unknown[][] = [];
  const control = { heard: true, starts: 0 };
  const input: NightIn = { ok: true, clock: CLOCK, cam: new THREE.PerspectiveCamera(), foot: null,
    driving: true, truck: null, radio1420: true, saro: null, silenceFree: false };
  const event = new NightEvents({
    audio: { ctx: null, play: (...args: unknown[]) => { plays.push(args); return null; } } as any,
    seen, note: (text) => notes.push(text), step: () => ({ name: 'step', gain: 1 }),
    setRadio: () => {}, bulletin: () => { control.starts++; return len; }, bulletinHeard: () => control.heard,
    openDoor: () => {}, silence: () => {}, place: () => 'Highway', silent: () => false,
  });
  const tick = (dt: number) => { input.clock += dt; event.update(dt, input); };
  return { event, input, seen, notes, plays, control, tick };
}
try {
  const r = night(), s = r.stations.caliche;
  const heard: string[] = [];
  r.onLine = (_station, text) => heard.push(text);
  r.update(1.4, CLOCK);
  const before = snapshot(r), oldLineCount = heard.length;
  const natural = BULLETIN.reduce((n, line) => n + secsOf(line.text) + .7, .4);
  check(!near(natural, ARCHIVE_SECS), `regression baseline: text alone lasts ${natural.toFixed(2)} s`);
  check(s.cutIn(BULLETIN, CLOCK) === 47, 'the archive reports exactly 47 seconds');
  const archive = s.seg;
  check(s.cutIn(BULLETIN, CLOCK) === 0 && s.seg === archive, 'an interruption cannot replace one already playing');
  for (let i = 0; i < 187; i++) r.update(.25, CLOCK + (i + 1) * .25);
  r.update(.24, CLOCK + 46.99);
  check(s.seg === archive && near(s.t, 46.99), 'still the recorded bulletin at 46.99 seconds');
  check(heard.slice(oldLineCount).join('|') === BULLETIN.map((line) => line.text).join('|'), 'all recorded lines are captioned once, in their fixed order');
  r.update(.01, CLOCK + 47);
  const resumed = snapshot(r);
  check(s.seg === before.seg && near(s.t, before.t), 'at 47 seconds the interrupted sentence resumes at its exact point');
  check(resumed.said === before.said && resumed.sc === before.sc, 'time-check wording and caption position are preserved');
  check(resumed.segNo === before.segNo && resumed.loopAt === before.loopAt, 'the original track offset and programme order are preserved');
  const count = heard.length;
  r.update(.1, CLOCK + 47.1);
  check(heard.length === count, 'resuming inside a sentence does not repeat its caption');

  // A long frame may carry a small remainder back into the held programme.
  const over = night(), os = over.stations.caliche;
  over.update(1, CLOCK);
  const ot = os.t, original = os.seg;
  os.cutIn(BULLETIN, CLOCK); over.update(47.25, CLOCK + 47.25);
  check(os.seg === original && near(os.t, ot + .25), 'only time beyond the 47 seconds advances the held programme');

  // The same snapshot preserves the position within a streamed record.
  Math.random = () => .1;
  const music = night(), ms = music.stations.caliche;
  music.update(5, CLOCK);
  const mb = snapshot(music);
  check(ms.seg.kind === 'music', 'record-resume test starts within a record');
  ms.cutIn(BULLETIN, CLOCK); music.update(47, CLOCK + 47);
  const ma = snapshot(music);
  check(ms.seg === mb.seg && near(ms.t, mb.t) && ma.segNo === mb.segNo, 'a record resumes from the same source offset');
  Math.random = () => 0;

  // A resumed programme needs its usual ten seconds of breathing room. It can pass from a
  // sentence into a record while this runs: eligibility depends on elapsed ordinary airtime.
  const breath = night(), br = breath.stations.caliche;
  const tripLines = [{ text: 'The county road is still closed at the junction.', who: 'host' as const }];
  br.cutIn(BULLETIN, CLOCK); breath.update(47, CLOCK + 47);
  check(!br.free && !br.slot(tripLines, CLOCK + 47), 'resuming does not immediately permit a trip insertion, even after a single 47-second tick');
  breath.update(1, CLOCK + 48);
  check(!br.free && !br.slot(tripLines, CLOCK + 48), 'one second of resumed programme is not enough breathing room');
  breath.update(1, CLOCK + 49);
  check(!br.free && !br.slot(tripLines, CLOCK + 49), 'two seconds of resumed programme cannot be replaced by trip lines');
  breath.update(7.75, CLOCK + 56.75);
  check(!br.free && !br.slot(tripLines, CLOCK + 56.75), 'the station still protects its own programme at 9.75 resumed seconds');
  breath.update(.25, CLOCK + 57);
  check(br.free, 'the station becomes free after ten seconds of ordinary programme');
  // A record that began during the calm period also keeps its existing five-second minimum.
  let resumedSlot = br.slot(tripLines, CLOCK + 57);
  for (let i = 0; !resumedSlot && i < 24; i++) {
    breath.update(.25, CLOCK + 57 + (i + 1) * .25);
    resumedSlot = br.slot(tripLines, CLOCK + 57 + (i + 1) * .25);
  }
  check(!!resumedSlot, 'trip lines can follow once both the calm period and any new record\'s minimum have elapsed');
  check(!os.free, 'a frame crossing the archive boundary counts only its quarter-second remainder toward the calm window');

  const boy = night(CLOCK, false), bs = boy.stations.caliche, bq = bs.queue.slice();
  check(bs.cutIn(BULLETIN, CLOCK) === 0 && bq.every((seg, i) => bs.queue[i] === seg), 'the boy and the host\'s response cannot be interrupted');
  const trip = night(), ts = trip.stations.caliche;
  const slotted = ts.slot([{ text: 'County crews are out on the north road.', who: 'host' }], CLOCK);
  check(!!slotted && ts.cutIn(BULLETIN, CLOCK) === 0 && ts.where(slotted) === 'waiting', 'a queued trip programme is kept intact');
  const late = night(SIGN_OFF - 120), ls = late.stations.caliche, lateSeg = ls.seg;
  check(ls.cutIn(BULLETIN, SIGN_OFF - 120) === 0 && ls.seg === lateSeg, 'no archive starts in the final three minutes before sign-off');

  const crossed = night(), cs = crossed.stations.caliche;
  cs.cutIn(BULLETIN, CLOCK);
  crossed.update(1, SIGN_OFF + 1);
  check(cs.seg.kind === 'talk' && !cs.seg.duration && (cs as any).interrupted === null, 'crossing sign-off discards the held programme and starts the fixed sequence');
  crossed.update(47, SIGN_OFF + 48);
  check(cs.seg.kind === 'anthem', 'an old archive cannot restore the Night Line over the anthem');
  const reset = night(), rs = reset.stations.caliche;
  rs.cutIn(BULLETIN, CLOCK); reset.reset(2 * 3600 + 10 * 60);
  check((rs as any).interrupted === null && !('duration' in rs.seg), 'loading or resetting clears the interruption');
  const off = night(SIGN_OFF + 600);
  check(off.stations.caliche.cutIn(BULLETIN, SIGN_OFF + 600) === 0, 'off-air stations reject the archive');

  // The director must not invent evidence if the station declines the recording.
  const declined = director(0);
  check(!declined.event.now('bulletin', declined.input), 'a declined bulletin returns false from the director');
  declined.tick(.5);
  check(!declined.event.busy && declined.event.last === null, 'a declined bulletin leaves no pending event or announced start');
  check(declined.control.starts === 1 && declined.seen.size === 0 && declined.notes.length === 0,
    'a declined bulletin does not consume its once-only key or write a finding');

  const full = director();
  check(full.event.now('bulletin', full.input) && full.event.busy, 'an accepted bulletin occupies the director');
  full.tick(46.75);
  check(full.notes.length === 0 && full.seen.size === 0 && full.event.busy, 'hearing less than 47 seconds is not yet full evidence');
  full.tick(.25);
  check(full.notes.length === 1 && full.notes[0].includes('1947') && full.seen.has('ne:bulletin1') && !full.event.busy,
    'the full finding is filed exactly at 47 seconds, without an extra half-second');
  full.control.heard = false; full.input.radio1420 = false; full.tick(.5);
  check(full.notes.length === 1 && full.seen.size === 1, 'switching off after 47 seconds preserves the completed finding');
  full.control.heard = true; full.input.radio1420 = true;
  check(!full.event.now('bulletin', full.input) && full.control.starts === 1, 'a completed bulletin cannot be consumed twice');

  for (const reason of ['tuned away', 'story blocked'] as const) {
    const partial = director();
    partial.event.now('bulletin', partial.input); partial.tick(46.9);
    if (reason === 'tuned away') { partial.control.heard = false; partial.input.radio1420 = false; }
    else partial.input.ok = false;
    partial.tick(.11);
    check(!partial.event.busy && partial.notes.length === 0 && partial.seen.size === 0,
      `${reason} at 46.9 seconds: no full finding or consumed event`);
    partial.control.heard = true; partial.input.radio1420 = true; partial.input.ok = true;
    check(partial.event.now('bulletin', partial.input), `${reason}: the unconsumed bulletin can be tried again`);
    partial.tick(47);
    check(partial.notes.length === 1 && partial.seen.has('ne:bulletin1') && partial.control.starts === 2,
      `${reason}: hearing the retry through files exactly one finding`);
  }

  const overlap = director();
  overlap.event.now('bulletin', overlap.input);
  check(!overlap.event.now('knocks', overlap.input) && overlap.plays.length === 0 && overlap.event.last?.id === 'bulletin',
    'a pending bulletin cannot be replaced or overlapped by another forced event');
  overlap.tick(47);
  check(overlap.notes.length === 1 && overlap.seen.has('ne:bulletin1') && !overlap.seen.has('ne:knocks1'),
    'rejecting an overlap leaves the original completion intact');
} finally { Math.random = oldRandom; }
console.log(`${checks - failed}/${checks} PASS`);
process.exit(failed ? 1 : 0);
