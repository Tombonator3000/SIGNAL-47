import { sayTime, type Line, type Radio, type Slotted, type Station, type StationId } from '../core/radio';
import type { LegId } from '../drive/legs';
import type { Phase } from './Prologue';
import { MOTEL_AT } from '../world/geo';

// The radio on the way (Tom's plan, todo.md: a radio programme in the truck while driving; mood,
// news, info and hints). Every trip has its own lines at marks along the way, in metres driven:
// said on the station the truck's radio is tuned to, in that station's voices (the Night Line's
// host and callers on 1420 until four, Roswell's morning man after five), captioned like the
// rest of the station (main.ts, radio.onLine). The station's own programme goes on in between
// (core/radio.ts, Station.slot): the lines come after the talk that is on, or over the end of a
// record, faded down, with the next record after them; never over the boy's call, the 1947
// bulletin, the sign-off or the sign-on. Each is heard once a night (kept with the case, `tr:` in the town's set); a mark
// with more than one gives the next one the next time the same way is driven. One taken by the
// station but not begun when the truck is left goes back, for another drive.
//
// The hints are quiet: a caller, a story of the host's, an advertisement. Nothing tells the
// player what to do. Between four and five nothing talks (1420 has signed off, Roswell is not
// on yet), so the drive to the motel in chapter four, and most drives back from STATION 01 in
// chapter three, have only the far station's music.
//
// A first draft, for Tom to put right.

/** A trip as the programme sees it (World.tripNow). */
export interface TripNow { n: number; from: LegId; to: LegId | null; bound: LegId | null; metres: number }
export interface TripRadioIn {
  trip: TripNow | null;
  phase: Phase;
  clock: number;
  /** The station the truck's radio is on and heard on in the cab, or null (off, or not driving). */
  tuned: StationId | null;
}

/** A mark on the way: on trips from>to (either may be *), from `at` metres driven and for `reach`
 *  metres after, in these chapters, on this station. Each of `say` is heard once a night, in
 *  turn. */
interface Mark { trip: string; at: number; reach?: number; ch: Phase[]; on: StationId; say: { id: string; lines: Line[] }[] }
const REACH = 450;

const cap = (s: string) => s[0].toUpperCase() + s.slice(1);
const H = (text: Line['text']): Line => ({ text, who: 'host' });
const R = (text: Line['text']): Line => ({ text, who: 'roswell' });
const W = (text: string): Line => ({ text, who: 'woman', by: 'a caller' });
const M = (text: string): Line => ({ text, who: 'man', by: 'a caller' });
const O = (text: string): Line => ({ text, who: 'old', by: 'a caller' });
const EARLY: Phase[] = ['ch1', 'ch2'], NIGHT: Phase[] = ['ch1', 'ch2', 'ch3'], CH3: Phase[] = ['ch3'], MORNING: Phase[] = ['ch3', 'ch4', 'ch5'];

// The places are SARO (the motel is on its stretch of the highway), STATION 01 and the diner
// (Caliche is just beyond it). On the highway south of SARO the truck passes the motel at about
// 650 m, the survey track's mouth at 850 m and the diner at 2.3 km; the gate is 1.45 km out.
// The first mark of a trip is 150 m out, clear of the hints said on getting in.
const PROGRAMME: Mark[] = [
  // ---------- 1420, the Night Line: leaving SARO, any way ----------
  { trip: 'saro>*', at: 150, ch: EARLY, on: 'caliche', say: [
    { id: 'out1', lines: [H((c) => `Night Line, fourteen-twenty. ${cap(sayTime(c))}, and the coffee in here is older than some of you.`),
      H('Bank sign says thirty-six now. Warming up by Wednesday, they tell me.')] },
    { id: 'out2', lines: [H((c) => `${cap(sayTime(c))} on the Night Line. I can see the highway out my window, and it's empty both ways.`)] },
    { id: 'out3', lines: [W('Earl? I can\'t sleep. The wind stopped all at once tonight, like somebody shut a door.'),
      H('I noticed that. Rattled the windows all evening, and then nothing.')] },
  ] },
  { trip: 'saro>*', at: 420, reach: 300, ch: EARLY, on: 'caliche', say: [
    { id: 'roof', lines: [W('I drove past the motor court a while ago. There\'s somebody sitting up on the roof. Small. Real still.'),
      H('Up on a roof at this hour, they can have it.')] },
  ] },
  // ---------- out to STATION 01 before chapter three: chained ----------
  { trip: 'saro>station01', at: 900, reach: 350, ch: EARLY, on: 'caliche', say: [
    { id: 'chain', lines: [O('Earl, you know the old survey station, out the gravel west of the highway?'),
      H('The government place. Nobody\'s been out there in years.'),
      O('Somebody put a new chain on it. Shiny. Who buys a new chain for a place nobody goes?')] },
  ] },
  { trip: 'saro>station01', at: 1200, reach: 300, ch: EARLY, on: 'caliche', say: [
    { id: 'lamps', lines: [H('My dad said the survey crews out there worked nights, back in forty-seven. You could see their lamps from town.'),
      H('Then one summer they packed up and went, and left the lamps.')] },
  ] },
  // ---------- chapter three: the drive out to STATION 01 ----------
  { trip: 'saro>station01', at: 150, ch: CH3, on: 'caliche', say: [
    { id: 'c3out', lines: [H((c) => `${cap(sayTime(c))}. Not long now till four, so if you've been meaning to call, call.`)] },
  ] },
  { trip: 'saro>station01', at: 400, reach: 400, ch: CH3, on: 'caliche', say: [
    { id: 'fence', lines: [M('Earl, somebody\'s been cutting my fence on the Hondo road. Both ends, clean, like with bolt cutters.'),
      H('Not the wind, then.'), M('Wind don\'t carry bolt cutters.')] },
  ] },
  { trip: 'saro>station01', at: 850, reach: 350, ch: CH3, on: 'caliche', say: [
    { id: 'gate', lines: [W('Earl, it\'s Dolores again. There\'s a light on out at the old survey station.'),
      H('County chained that gate years ago, Dolores.'), W('The big gate. Not the little one beside it.')] },
  ] },
  { trip: 'saro>station01', at: 1150, reach: 300, ch: CH3, on: 'caliche', say: [
    { id: 'marks', lines: [H('My granddad ran chain for the survey crews. Never trust one mark, he said. Sight two, and if one has walked, the other tells you.'),
      H('And if you think it\'s your own lamp making the shadows, put your hat over the lamp and look again.')] },
  ] },
  // ---------- chapter three: back to SARO with the film (before four, or after five) ----------
  { trip: 'station01>saro', at: 150, reach: 550, ch: CH3, on: 'caliche', say: [
    { id: 'film', lines: [H('My sister ran the photo counter at the drugstore twenty years. Film doesn\'t care what time it is, she used to say.'),
      H('But it doesn\'t keep, either. Not in a warm truck.')] },
  ] },
  { trip: 'station01>saro', at: 900, reach: 600, ch: CH3, on: 'caliche', say: [
    { id: 'late', lines: [H('That\'s about all from the Night Line. When the anthem comes on, that\'s me going home.')] },
  ] },
  { trip: 'station01>saro', at: 150, reach: 850, ch: MORNING, on: 'roswell', say: [
    { id: 'mfilm', lines: [R('The camera shop on Main opens at nine. Film left in a warm car is film you won\'t like.')] },
  ] },
  // ---------- back from the station before chapter three ----------
  { trip: 'station01>*', at: 150, ch: EARLY, on: 'caliche', say: [
    { id: 'track', lines: [M('Earl, I drove out the survey track tonight. Just to see.'), H('And?'),
      M('Chained, like you said. And it took me the longest time to find the highway again. It\'s one road, Earl.')] },
  ] },
  // ---------- down the highway to the diner and Caliche, at night ----------
  { trip: 'saro>diner', at: 1000, reach: 500, ch: NIGHT, on: 'caliche', say: [
    { id: 'oldroad', lines: [O('Earl, is the old Roswell road open past the diner yet?'),
      H('Not past the junction. Barricade\'s still up. You want Roswell, go around like everybody else.')] },
    { id: 'probe', lines: [H('That European probe flew right up to the comet last month and sent back a picture of a potato. Seventy-six years we waited, for a potato.')] },
  ] },
  { trip: 'saro>diner', at: 1600, reach: 450, ch: NIGHT, on: 'caliche', say: [
    { id: 'coffee', lines: [W('Earl, I\'m down at the Mesa. She says tell you the coffee\'s fresh and the pie isn\'t.'),
      H('Tell her I heard. And tell her I\'m still not paying for last week\'s.')] },
    { id: 'tax', lines: [H('Tuesday is tax day, folks. If you haven\'t mailed it yet, I don\'t want to hear about it on this program.')] },
  ] },
  { trip: 'saro>diner', at: 2200, reach: 700, ch: NIGHT, on: 'caliche', say: [
    { id: 'door', lines: [H('If you\'re in town and you see the light on over my door, I\'m in here. Lean on the buzzer during a long record.')] },
  ] },
  // ---------- leaving the diner or Caliche, at night ----------
  { trip: 'diner>*', at: 150, reach: 450, ch: NIGHT, on: 'caliche', say: [
    { id: 'watch', lines: [W('Earl, there\'s a light sitting over the mesa east of town. It isn\'t a star. Stars don\'t sit that low.'),
      H('Is it doing anything?'), W('It\'s watching the road.'), H('Well. Wave.')] },
    { id: 'pumps', lines: [H((c) => `${cap(sayTime(c))} in Caliche. Pumps are locked till six, so if you're low, you're low.`)] },
  ] },
  { trip: 'diner>saro', at: 1600, reach: 600, ch: NIGHT, on: 'caliche', say: [
    { id: 'dishes', lines: [M('Earl, those big dishes up at the observatory. Did they all turn at once tonight?'),
      H('They do that. They\'re listening for something.'), M('A little after two. All of them together, like sunflowers.')] },
    { id: 'cattle', lines: [H('Cattle report, for those of you who keep cattle and can\'t sleep. Feeders off again at the Roswell sale. Everybody still blames the dairy buyout.')] },
  ] },
  // ---------- 1230, Roswell, after five: chapter five's drive to the diner ----------
  { trip: 'saro>diner', at: 250, reach: 700, ch: MORNING, on: 'roswell', say: [
    { id: 'm1', lines: [R((c) => `${cap(sayTime(c))} on a Monday morning, April the fourteenth. Here's the farm and ranch report.`),
      R('Feeder steers a dollar lower at the sale barn. The cattlemen are still sore about the dairy buyout, and who can blame them.')] },
  ] },
  { trip: 'saro>diner', at: 1000, reach: 500, ch: MORNING, on: 'roswell', say: [
    { id: 'm2', lines: [R('Tuesday is tax day. The post office in Roswell keeps the window open till midnight tomorrow, for the brave.')] },
  ] },
  { trip: 'saro>diner', at: 1600, reach: 500, ch: MORNING, on: 'roswell', say: [
    { id: 'm3', lines: [R('Mesa Diner at the old road junction has been open all night, and the coffee is on. Tell her the radio sent you.')] },
  ] },
  { trip: 'saro>diner', at: 2150, reach: 700, ch: MORNING, on: 'roswell', say: [
    { id: 'm4', lines: [R('Sunrise at five thirty. If you\'re headed east this morning, the sun will be right in your eyes. Take it slow out there.')] },
  ] },
  // ---------- 1230, other ways in the morning ----------
  { trip: 'diner>saro', at: 1500, reach: 700, ch: MORNING, on: 'roswell', say: [
    { id: 'm5', lines: [R('Clear and cold this morning, and the wind should stay down till noon. Then it\'s spring in New Mexico again, and hold on to your hat.')] },
  ] },
  { trip: 'diner>*', at: 150, reach: 550, ch: MORNING, on: 'roswell', say: [
    { id: 'm6', lines: [R('The comet is past its best now. Next time around is twenty sixty-one. I\'ll let my grandkids have that one.')] },
  ] },
];
/** How many lines the programme has (the tests, the report). */
export const programmeSize = () => PROGRAMME.reduce((a, m) => a + m.say.reduce((b, s) => b + s.lines.length, 0), 0);

/** The places as the programme names them: the motel is on SARO's stretch, and a trip begun out on
 *  the road (the developer menu's start) is SARO's too. */
const place = (p: LegId | null) => !p ? null : p === 'motel' || p === 'road' ? 'saro' : p === 'roswell' ? null : p;
/** A trip from the motel's lot is a trip from SARO that starts 650 m down the highway. */
const START: Partial<Record<LegId, number>> = { motel: MOTEL_AT.z };

export class TripRadio {
  /** The last one on the air: which, at how many metres it was taken, at what clock (the tests). */
  last: { id: string; metres: number; clock: number; trip: string } | null = null;
  /** One taken by the station and waiting there to begin. */
  private taken: { st: Station; x: Slotted; on: NonNullable<TripRadio['last']> } | null = null;
  /** The marks, for the tests: where, on which station, their ids, and of each the first line
   *  that is the same every time (a time check is not) and who says it. */
  readonly plan = PROGRAMME.map((m) => {
    const fixed = m.say.map((x) => x.lines.find((l) => typeof l.text === 'string'));
    return { trip: m.trip, at: m.at, reach: m.reach ?? REACH, ch: m.ch, on: m.on, ids: m.say.map((x) => x.id),
      first: fixed.map((l) => (l?.text as string | undefined) ?? ''), by: fixed.map((l) => l?.by ?? null) };
  });
  readonly lines = programmeSize();
  /** Each trip's marks that are done with (said, or driven past). */
  private trip: { n: number; done: Set<Mark> } | null = null;

  constructor(private radio: Radio, private seen: Set<string>) {}

  update(s: TripRadioIn) {
    const tr = s.trip, tk = this.taken, at = tk ? tk.st.where(tk.x) : null;
    if (tk && at === 'on') { this.last = tk.on; this.taken = null; }
    // the truck left before it came on, or something else came first: back for another drive
    else if (tk && (!at || !tr || this.trip?.n !== tr.n)) {
      tk.st.unslot(tk.x); this.seen.delete(`tr:${tk.on.id}`);
      this.taken = null;
    }
    if (!tr) { this.trip = null; return; }
    if (this.trip?.n !== tr.n) this.trip = { n: tr.n, done: new Set() };
    if (!s.tuned || this.taken) return;
    const from = place(tr.from), to = place(tr.bound), metres = tr.metres + (START[tr.from] ?? 0);
    const st = this.radio.stations[s.tuned], done = this.trip.done;
    // the marks in order along the way: the first one that is due waits until the station takes it
    for (const m of PROGRAMME) {
      if (done.has(m) || m.on !== s.tuned || metres < m.at || !m.ch.includes(s.phase)) continue;
      const [a, b] = m.trip.split('>');
      if ((a !== '*' && a !== from) || (b !== '*' && b !== to)) continue;
      if (metres > m.at + (m.reach ?? REACH)) { done.add(m); continue; }
      const say = m.say.find((x) => !this.seen.has(`tr:${x.id}`));
      if (!say) { done.add(m); continue; }
      const x = st.slot(say.lines, s.clock);
      if (x) {
        done.add(m); this.seen.add(`tr:${say.id}`);
        this.taken = { st, x, on: { id: say.id, metres, clock: s.clock, trip: `${from}>${to ?? '?'}` } };
      }
      return;
    }
  }
}
