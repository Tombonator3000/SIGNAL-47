import type { Interaction } from '../core/Interaction';
import type { Town } from '../world/Town';
import type { UI, PersonCard, DocSpec } from '../ui/UI';
import * as P from '../ui/Panels';
import { portrait, type PersonId } from '../core/portraits';
import { clockText } from './time';

// What Caliche's street says when it is looked at or used (world/Town.ts; HENDELSER.md, section
// 9). Step 1 of the town: the buildings, the windows and the signs, and three things the
// notebook keeps (the flyer on the gas station door, the sheriff's map, the morning paper).
// Step 2 (17 October): the people, one secret each (HENDELSER.md, section 9): the attendant behind
// the night window, the deputy at his desk, Earl at 1420 AM while the Night Line is on, the woman
// with the radio on the laundromat's bench, and the souvenir man smoking on his stoop. And event
// 5: two hours or more after the player first spoke to him, the attendant says the same sentence
// again, word for word, the cough in the same place.

export interface CalicheDeps {
  inter: Interaction;
  ui: UI;
  toast: (text: string, secs?: number) => void;
  /** The player is in the diner's area (the street is part of it). */
  here: () => boolean;
  /** The game's clock: seconds since midnight, and as the notebook writes it. */
  secs: () => number;
  clock: () => string;
  note: (text: string) => void;
  /** File a witness transcription in Papers; the host replaces an existing ID. */
  file: (doc: DocSpec) => void;
  /** What the notebook already has from the town (saved with the game, main.ts). */
  seen: Set<string>;
  save: () => void;
}

export function bindTown(town: Town, d: CalicheDeps) {
  const day = () => ((d.secs() % 86400) + 86400) % 86400;
  const once = (id: string, text: string) => {
    if (d.seen.has(id)) return;
    d.seen.add(id);
    d.note(`${d.clock()}. ${text}`);
    d.save();
  };
  const add = (id: string, label: () => string | null, use: () => void, examine: (() => string | null) | null, range = 2.6) => {
    const object = town.proxies[id];
    if (!object) return;
    d.inter.add({ id: 'town:' + id, object, range, label: () => d.here() ? label() : null, use,
      examine: examine ? () => d.here() ? examine() : null : undefined, examineRange: Math.max(8, range + 2) });
  };
  const say = (text: string, secs = 3.6) => () => d.toast(text, secs);

  // ---------- the gas station ----------
  const flyer = () => once('flyer', 'Caliche, the gas station door: a flyer, SEEN LIGHTS? CALL THE NIGHT LINE, 1420 AM, midnight to four.');
  add('gasDoor', () => 'Gas station door', () => {
    d.toast('Locked, with a night window and a bell. The attendant looks up, sees you are not a car, and goes back to his magazine. Taped inside the glass: SEEN LIGHTS? CALL THE NIGHT LINE.', 5.4);
    flyer();
  }, () => { flyer(); return 'SEEN LIGHTS? CALL THE NIGHT LINE. 1420 AM, MIDNIGHT TO FOUR. Typed, copied, and copied again.'; });
  add('gasWindow', () => 'Night window', () => talk('attendant'),
    () => 'Oil cans in rows, a calendar still on April, the register. Behind the counter a man in a pressed shirt turns a page. He has not looked at the clock once.');
  add('pumps', () => 'Pumps', say('Regular, 89.9. It has come down all spring. The nozzles are padlocked for the night.', 3.8),
    () => 'Two pumps on a concrete island. The counters stand at zero.');
  add('payphone', () => town.ringing ? 'Answer the payphone' : 'Payphone', () => {
    if (town.ringing) { town.onAnswer?.(); return; }
    d.toast(d.seen.has('nickel') ? 'A dial tone. The coin return is empty now.' : 'A dial tone, and under it something like wind. Nobody to call at this hour.', 3.6);
  },
    () => 'A payphone on a post, a phone book on a chain with half its pages gone.');
  add('gasSign', () => 'Sign', say('GAS. DIESEL. No brand, just the words.', 2.8), null, 3);
  add('paper', () => town.paperIn ? 'Newspapers' : null, () => {
    if (!town.paperIn) return;
    d.toast('The Pecos Valley Sentinel, Monday, April 14, 1986, still in its twine. NICKLAUS, 46, WINS MASTERS. Dropped from a car that did not stop.', 5);
    once('paper', 'Caliche: the Pecos Valley Sentinel dropped at the gas station before five. Monday, April 14, 1986.');
  }, () => town.paperIn ? 'NICKLAUS, 46, WINS MASTERS. Under the fold, smaller: County to grade the old Roswell road this summer.' : null);

  // ---------- the sheriff's substation ----------
  const map = () => {
    once('pins', "Caliche, the sheriff's substation: a county map on the wall, stuck with pins. Most are scattered. Two rows run straight, one east along the old road, one west into the empty part of the map.");
  };
  add('sheriffWindow', () => 'Window', () => {
    d.toast(town.deputyIn
      ? 'A county map on the wall, stuck with pins: red, blue, yellow. Most are scattered. Two rows are not. The deputy at the desk looks up at you, and back at the map.'
      : 'The office is dark. Through the bars you can still make out the map and its pins, and two rows of them running straight.', 5.2);
    map();
  }, () => { map(); return 'One row of pins runs out east along the old road. The other runs west, off the highway, into country with nothing drawn on it.'; });
  add('sheriffDoor', () => town.deputyIn ? 'Knock' : 'Door', () => town.deputyIn ? talk('deputy') : d.toast('Locked. A card in the frame, in ballpoint: BACK SOON.', 3.6),
    () => town.deputyIn ? 'COUNTY SHERIFF, SUBSTATION. A light under the door, and a radio turned low.' : null);
  add('patrolCar', () => town.deputyIn || town.carOut ? 'Patrol car' : null, () => d.toast(town.carOut
    ? 'The engine is running. The driver\'s door hangs open with the keys in it, his hat on the seat, the radio on the dash turned all the way down. Nobody.'
    : 'The deputy\'s car. A clipboard on the passenger seat, a thermos, a county map folded small.', town.carOut ? 5.6 : 3.8),
    () => town.carOut ? 'No footprints going off the shoulder that you can see. The exhaust hangs in the cold, straight up.'
      : town.deputyIn ? 'The hood is still warm. Pins in a cigar box on the dash.' : null);

  // ---------- the laundromat, the Plaza, the feed store ----------
  add('laundromat', () => 'Laundromat', say('Six washers, one tube light, nobody. One machine is running.', 3.6),
    () => 'The one machine turns, stops, turns. There is no basket on the bench and no one waiting.');
  add('bench', () => town.womanIn ? null : 'Bench', say('A bench under the window. The paint is worn through in one place, where somebody sits often.', 3.6), null);
  add('cinema', () => 'The Plaza', say('Closed. The doors are chained. The posters behind the glass have faded to the same blue.', 3.8),
    () => 'A box office with its blind down, and a card: CLOSED.');
  add('marquee', () => null, () => {}, () => 'CLOSED. THANK YOU CALICHE. FOR 31 YEARS. A letter has fallen out of the last line.', 4);
  add('feed', () => 'Feed store', say('FEED & SEED. Sacks on a pallet on the dock, a bulb over the door with moths round it.', 3.6), null, 3);

  // ---------- 1420 AM, the shop, the church, the houses ----------
  const onAir = () => day() < 4 * 3600 || day() > 20 * 3600;
  add('studio', () => 'Studio window', () => d.toast(onAir()
    ? 'The Night Line, through the glass: the host at the microphone with his headphones on, the red ON AIR light. He does not look up.'
    : 'The studio is empty and the red light is off. Through the wall, the carrier: a hum with nothing in it.', 4.6),
  () => onAir() ? 'Reels turning on the deck behind him. A coffee can of pencils, a phone with its line light blinking.' : 'His chair pushed back. The phone\'s line light still blinking.');
  add('amDoor', () => onAir() ? 'Buzzer' : 'Door', () => onAir() ? talk('host')
    : d.toast('1420 AM. A buzzer and a card: STUDIO IN USE, PLEASE WAIT. Nobody comes. Through the door, the carrier.', 4.2), null);
  add('souvenir', () => 'Shop window', () => d.toast(d.seen.has('ev:cutout') && !town.cutoutOut
    ? 'The cardboard alien is back in the window. There is dew on it, and grit from the road along its feet.'
    : 'A cardboard alien, life size if they come in that size. A rubber mask. Bags of crumpled foil, a dollar each: GENUINE DEBRIS.', 4.6),
    () => 'The foil has been crumpled by hand. There is a price sticker on the alien too.');
  add('souvenirSign', () => 'Sign', say('SAUCER CRASH 1947. TOURS. ASK INSIDE. Painted by hand, and painted over at least once.', 3.8), null, 3.4);
  add('souvenirDoor', () => 'Door', say('CLOSED. The hours are on a card faded white.', 3), null);
  add('letters', () => 'Letter board', say('THE HEAVENS DECLARE THE GLORY OF GOD. PS 19:1. A bulb over it, burning all night.', 3.8), null, 3);
  add('churchDoor', () => 'Church door', say('Locked. Sunday 10 AM, it says. All welcome.', 3), null);
  const tv1On = () => day() < 3 * 3600 + 40 * 60;
  add('tv1', () => 'Window', () => d.toast(tv1On() ? 'A television on in a dark front room, and someone asleep in the chair in front of it.' : 'The television is off now. The chair is empty.', 3.8),
    () => tv1On() ? 'The late movie, black and white, with the sound down.' : null, 6.5);
  add('tv2', () => 'Window', say('A television on with nobody watching it. Snow, then the test pattern, then snow.', 3.8), null, 6.5);
  add('kitchen', () => 'Window', say('A kitchen light behind a curtain, and a shadow that does not move.', 3.4), null, 4);

  // ---------- the people ----------
  add('woman', () => town.womanIn ? 'Woman on the bench' : null, () => { if (town.womanIn) talk('radio_woman'); },
    () => town.womanIn ? 'A woman in a winter coat with a transistor radio in her lap. No laundry with her.' : null);
  add('smoker', () => town.smokerIn ? 'Man on the stoop' : null, () => { if (town.smokerIn) talk('souvenir_man'); },
    () => town.smokerIn ? 'A man in a hat on his front step, smoking, watching the street.' : null, 5.6);

  let who: Who | null = null;
  let lines: Line[] = [];
  const asked = (w: Who, t: string) => d.seen.has(`ask:${w}:${t}`);
  // Follow-ups use only observations already filed in this case. The same check guards
  // selection, so an old panel cannot supply a topic before its witness has returned.
  const available = (w: Who, t: Topic) => !asked(w, t.id)
    && (!t.after || asked(w, t.after))
    && (!t.need || (Array.isArray(t.need) ? t.need : [t.need]).every((k) => d.seen.has(k)))
    && (t.notBefore === undefined || day() >= t.notBefore);
  function talk(w: Who) {
    const p = PEOPLE[w], first = !d.seen.has('met:' + w);
    who = w;
    d.seen.add('met:' + w);
    lines = p.open({ first, day: day(), town, seen: d.seen });
    if (w === 'attendant') attendant(first);
    if (w === 'deputy' && day() >= 4 * 3600 + 30 * 60) once('deputyBack', DEPUTY_BACK);
    d.save();
    P.talk(d.ui, {
      title: p.title,
      earl: w === 'host',
      view: () => {
        const v: P.TalkView = { status: p.status, lines, choices: [], reply: null };
        for (const t of p.topics) if (available(w, t)) v.choices.push({ id: t.id, label: t.label });
        v.choices.push({ id: 'bye', label: p.bye });
        return v;
      },
      onChip: () => {}, onTable: () => {}, onRead: () => {},
      onChoice: (id) => {
        if (id === 'bye') { d.ui.close(); return; }
        const t = p.topics.find((x) => x.id === id);
        if (!t || who !== w || !available(w, t)) return;
        d.seen.add(`ask:${w}:${id}`);
        lines = t.lines;
        if (t.note) once(`note:${w}:${id}`, t.note);
        const paper = t.paper && townWitnessDocs(d.seen).find((doc) => doc.id === t.paper!.id);
        if (paper) { d.file(paper); d.toast('Witness note filed in Papers.', 3); }
        d.save();
      },
      onClose: () => { who = null; },
    });
  }
  // event 5: the attendant. The first time is kept (with the second's clock, in the save); two
  // hours or more after it, he opens the window as if for the first time and says it again.
  function attendant(first: boolean) {
    const at = [...d.seen].find((k) => k.startsWith('attendant@'));
    if (first || !at) { if (!at) d.seen.add('attendant@' + Math.round(d.secs())); return; }
    const t0 = Number(at.slice(10));
    if (d.secs() - t0 < 2 * 3600) return;
    lines = [aside('He slides the window open a hand\'s width, and looks at you as if you had just pulled in.'), ...COLD];
    d.seen.add(`mark:repeat@${Math.round(d.secs())}`);
    once('repeat', `Station attendant, ${clockText(t0, false)} and ${clockText(d.secs(), false)}. The same sentence, the same cough.`);
  }
}

/** The people of Caliche the player has spoken to, for the notebook's People tab (main.ts). */
export function townPeople(seen: Set<string>): PersonCard[] {
  return (Object.keys(PEOPLE) as Who[]).filter((w) => seen.has('met:' + w)).map((w) => ({ ...PEOPLE[w].card, portrait: portrait(w) }));
}

/** Static notebook copies restored from the existing town keys after loading a case. */
export function townWitnessDocs(seen: Set<string>): DocSpec[] {
  return (Object.keys(PEOPLE) as Who[]).flatMap((w) => PEOPLE[w].topics
    .filter((t) => t.paper && seen.has(`ask:${w}:${t.id}`))
    .map((t) => ({ ...t.paper! })));
}

// ---------- the lines ----------
type Line = P.TalkLine;
type Who = Extract<PersonId, 'attendant' | 'deputy' | 'host' | 'radio_woman' | 'souvenir_man'>;
interface Topic { id: string; label: string; lines: Line[]; note?: string; paper?: DocSpec; after?: string; need?: string | string[]; notBefore?: number }
interface Person {
  title: string; status: string; bye: string;
  card: Omit<PersonCard, 'portrait'>;
  open: (o: { first: boolean; day: number; town: Town; seen: Set<string> }) => Line[];
  topics: Topic[];
}
const aside = (text: string): Line => ({ who: '', text });
const you = (text: string): Line => ({ who: 'YOU', text });
const A = (text: string): Line => ({ who: 'THE ATTENDANT', text });
const DP = (text: string): Line => ({ who: 'THE DEPUTY', text });
const EA = (text: string): Line => ({ who: 'EARL', text });
const RW = (text: string): Line => ({ who: 'THE WOMAN', text });
const SM = (text: string): Line => ({ who: 'THE SOUVENIR MAN', text });
const witnessPaper = (id: string, title: string, page: string): DocSpec => ({
  id: 'witness-' + id, title, kind: 'hand', stamp: 'REYES',
  page: 'REYES / NOTEBOOK COPY\n\n' + page,
  transcript: 'Copied into my notebook after the conversation. My observation and the witness\'s words.\n\n' + page,
});

// said the same way every time, the cough in the same place
const COLD: Line[] = [A('Cold one tonight. Radio says it\'ll'), aside('He coughs into his fist.'), A('warm up by Wednesday.')];
const DEPUTY_BACK = 'The deputy, back at his desk after half past four. He says he drove the loop and came straight back. He went out at a quarter to three.';

const PEOPLE: Record<Who, Person> = {
  attendant: {
    title: 'GAS STATION / THE ATTENDANT', status: 'The night window, a hand\'s width open.', bye: 'Let him read',
    card: { name: 'The attendant', role: 'Caliche, the gas station', note: 'Reads behind the night window. "Cold one tonight."' },
    open: ({ first }) => first
      ? [aside('You knock on the night window. He takes his time, and slides it open a hand\'s width.'), A('Pumps are locked till six. You want something from in here, I can pass it through.'), ...COLD]
      : [aside('He slides the window open again.'), A('Still out, huh.')],
    topics: [
      { id: 'roads', label: 'Ask about the roads', lines: [
        you('Which way is quickest back up to the observatory?'),
        A('Highway north. There is no other way that goes anywhere.'),
        A('The old road\'s closed at the crossing. County says they\'ll grade it this summer. County says that every summer.'),
        A('Your people come down for gas on Fridays. Nice folks. Don\'t say much.'),
      ] },
      { id: 'flyer', label: 'Ask about the flyer on the door', lines: [
        you('SEEN LIGHTS? CALL THE NIGHT LINE.'),
        A('Earl brings those. The radio fella, across the street.'),
        A('I let him tape them up. He buys his gas here.'),
        A('People call him. I couldn\'t tell you why they\'d want to.'),
      ] },
      { id: 'old', label: 'Ask if he has ever been out on the old road at night', lines: [
        you('You ever go out the old road at night?'),
        A('Towed a rig in off it once. Last October.'),
        A('Left here a quarter to two. Got back twenty-five past four.'),
        A('It\'s a forty minute job.'),
        aside('He says it like a sum he has done a lot of times and cannot get to come out.'),
        A('Anyway. Cold one tonight.'),
      ], note: 'The attendant towed a rig in off the old road one night last October. He left at a quarter to two and was back at twenty-five past four, for a forty minute job.' },
      { id: 'earlier', label: 'Tell him you were here before', need: 'repeat', lines: [
        you('I was here before. You told me that.'),
        A('Wasn\'t me, then. I\'ve been here since ten.'),
        aside('He looks up at the clock over the register for a long time, as if it might say something else.'),
      ] },
      { id: 'phone', label: 'Show him the nickel from the payphone', after: 'old', need: ['ev:phone1', 'nickel'], lines: [
        you('The payphone gave me this. After the printer stopped.'),
        aside('You set the bright nickel on the sill. He turns it over without picking it up.'),
        A('That phone takes dimes.'),
        you('Nineteen forty-seven.'),
        A('I can read it.'),
        aside('He pushes it back with the edge of his magazine.'),
        A('Keep it. I write down what goes in the till. I want it to add up.'),
        aside('I copy his words into the notebook, beside the account of the call.'),
      ], note: 'The attendant examined the nickel from the payphone. "That phone takes dimes." He pushed it back and would not put it in the till.',
      paper: witnessPaper('phone', 'Witness / The attendant and the payphone',
        'MAIN STREET PAYPHONE\n\nOn the line: the control room printer. Afterwards, a bright nickel dated 1947 in the coin return.\n\nTHE ATTENDANT, shown the coin:\n"That phone takes dimes."\n"Keep it. I write down what goes in the till. I want it to add up."\n\nHe pushed the coin back across the sill. I kept it.') },
    ],
  },
  deputy: {
    title: 'SHERIFF SUBSTATION / THE DEPUTY', status: 'The door open on the chain, then off it.', bye: 'Let him work',
    card: { name: 'The deputy', role: 'Caliche, sheriff\'s substation', note: '"I don\'t believe in little men. I believe in maps."' },
    open: ({ first, day }) => day >= 4 * 3600 + 30 * 60
      ? [aside('He opens before you have finished knocking.'), DP('Quiet night. Drove the loop out to the county line and back.'),
        aside('He looks at his watch, and then at the clock on the wall, and does not say anything about either.')]
      : first
        ? [aside('You knock. A chair scrapes. He opens the door on the chain, looks at your jacket, and takes the chain off.'), DP('Help you?'),
          you('Reyes. From the observatory up the road.'), DP('SARO. Sure. You folks lose something?')]
        : [DP('Back again.')],
    topics: [
      { id: 'map', label: 'Ask about the map with the pins', lines: [
        you('The map on your wall.'),
        DP('Calls. Every call I get that I can\'t write up as anything.'),
        DP('Red is engines dying. Blue is cattle. Yellow is lights. Green is people who lost track of the time.'),
        DP('I don\'t believe in little men. I believe in maps.'),
        DP('Most of them are all over. But you put enough pins in anything, and some of them start to line up.'),
      ], note: 'The deputy\'s pins: red for engines that died, blue for cattle, yellow for lights, green for people who lost time. "I don\'t believe in little men. I believe in maps."' },
      { id: 'east', label: 'Ask about the row going east', after: 'map', lines: [
        you('The row running east.'),
        DP('The old road. Red, mostly. Engines. Out past where the county stops grading.'),
        DP('Every one of those is a car somebody had to tow back.'),
        DP('You want to know where, drive it with your eyes open. I\'m not putting it in writing.'),
      ] },
      { id: 'green', label: 'Ask about the green pins', after: 'map', lines: [
        you('The green ones.'),
        DP('Three of them are right here in town.'),
        DP('The fella at the gas station. October nineteenth, last year. Went out to tow a rig and lost two hours he can\'t account for.'),
        DP('A man whose wife sits out at the laundromat nights. March of \'83.'),
        DP('And a kid who called the radio station in September of \'81. Family moved away after.'),
        DP('Don\'t go asking them about it. They don\'t know they\'ve got a pin.'),
      ], note: 'Green pins in Caliche, people who lost time: the gas station attendant, October 19, 1985; the husband of the woman at the laundromat, March 1983; a boy who called the radio, September 1981.' },
      { id: 'car', label: 'Tell him his patrol car was empty', after: 'green', need: 'ev:car', notBefore: 4 * 3600 + 30 * 60, lines: [
        you('Your car was on the shoulder north of town. Engine running, door open. Nobody in it.'),
        DP('I drove the loop. I came straight back.'),
        you('Your hat was on the seat.'),
        aside('He looks at the hat hanging beside the door.'),
        DP('Write down where you saw it.'),
        aside('He opens his logbook, then lays a loose sheet over the page.'),
        DP('Here. In your own words.'),
        aside('I write it on his sheet and keep a copy in the notebook.'),
      ], note: 'The deputy, back at his desk: I described his empty patrol car and the hat on its seat. He said he drove the loop and came straight back, then asked for my account on a separate sheet.',
      paper: witnessPaper('car', 'Witness / The empty patrol car',
        'HIGHWAY NORTH OF CALICHE\n\nThe deputy\'s car on the shoulder. Engine running, driver\'s door open, nobody in it. His hat was on the seat.\n\nTHE DEPUTY, back at the substation:\n"I drove the loop. I came straight back."\n\nWhen I mentioned the hat, he looked at the one hanging beside the door. He asked for my account on a loose sheet. This is my notebook copy.') },
    ],
  },
  host: {
    title: '1420 AM / EARL', status: 'A record on. Three minutes and forty seconds.', bye: 'Let him go back in',
    card: { name: 'Earl', role: '1420 AM, the Night Line', note: 'Keeps every call on reel but one.' },
    open: ({ first }) => first
      ? [aside('You lean on the buzzer. After a while the door opens on a man with his headphones pushed down round his neck.'), EA('I\'ve got a record on. Three minutes and forty seconds, so talk fast.')]
      : [EA('You again. Song and a half this time.')],
    topics: [
      { id: 'calls', label: 'Ask what people call in about', lines: [
        you('What do people call in about?'),
        EA('Lights, mostly. Cattle, some nights. Engines.'),
        EA('And missing time. You read that book? Everybody\'s read that book now.'),
        EA('Half of them, I think they read it first and then it happened to them.'),
        EA('The other half never read anything in their lives.'),
      ] },
      { id: 'visitors', label: 'Ask what he thinks they are', lines: [
        you('What do you think they are?'),
        EA('On the air I call them our visitors. It keeps it friendly.'),
        EA('Off the air I don\'t call them anything.'),
      ] },
      { id: 'tape', label: 'Ask if he keeps the calls', after: 'calls', lines: [
        you('Do you keep the calls?'),
        EA('Every one, on reel. Twelve years of them.'),
        EA('Except one. September of \'81, a boy called in. I\'ve never played it back.'),
        EA('I wasn\'t on the air that night. The station was off.'),
        aside('He looks back at the studio, where the reels are turning.'),
        EA('Record\'s ending.'),
      ], note: 'Earl at 1420 AM keeps every call on reel. One he has never played: September 1981, a boy, on a night the station was off the air.' },
      { id: 'boy', label: 'Ask about the boy you heard tonight', after: 'tape', need: 'ev:boy', lines: [
        you('I heard the boy on your show tonight. The star with the little tail.'),
        EA('You heard him too.'),
        aside('He pulls a programme sheet from his pocket. A line in pencil reads BOY / COMET. The space for a caller\'s number is blank.'),
        EA('I put the receiver down and tried the line again. Dial tone. Just a phone.'),
        you('Like the call you kept from eighty-one?'),
        EA('I haven\'t played that one. I told you.'),
        aside('He folds the sheet along the same crease.'),
        EA('I wrote this one down.'),
        aside('I copy the entry and his words into the notebook.'),
      ], note: 'Earl showed me tonight\'s programme sheet: BOY / COMET in pencil, no caller number. He checked the line afterwards and got an ordinary dial tone. He still has not played the 1981 reel.',
      paper: witnessPaper('boy', 'Witness / Earl\'s programme sheet',
        '1420 AM / NIGHT LINE\n\nI heard a boy call about a star with a little tail, low in the south.\n\nEARL\'S PROGRAMME SHEET, copied by hand:\nBOY / COMET\nCaller number: blank.\n\nEARL:\n"I put the receiver down and tried the line again. Dial tone. Just a phone."\n\nHe still has not played the reel from September 1981. I have not heard that reel.') },
    ],
  },
  radio_woman: {
    title: 'LAUNDROMAT / THE WOMAN', status: 'Hiss from the radio in her lap.', bye: 'Leave her to it',
    card: { name: 'The woman on the bench', role: 'Caliche, outside the laundromat', note: 'A radio tuned between two stations. "It\'s louder tonight."' },
    open: ({ first }) => first
      ? [aside('A transistor radio in her lap, tuned between two stations. Hiss, and under it something that is not quite a voice.'), RW('It\'s louder tonight.'), aside('She does not look at you when she says it.')]
      : [RW('Still louder.')],
    topics: [
      { id: 'radio', label: 'Ask what she is listening to', lines: [
        you('What are you listening to?'),
        RW('Nothing. That\'s the point. In between.'),
        RW('My husband said you could hear it in between, if you were quiet enough.'),
        RW('Most nights it\'s just this. Tonight it\'s louder.'),
      ] },
      { id: 'husband', label: 'Ask about her husband', after: 'radio', lines: [
        you('Your husband.'),
        RW('He went out to look at the cattle one night. March, three years ago.'),
        RW('Forty-seven minutes. I had the clock right there on the stove.'),
        RW('He came back in and sat down and took off his boots, and he never said one word about it. Not then, not since.'),
        RW('He sleeps fine. I don\'t.'),
      ], note: 'The woman on the laundromat bench: her husband went out to the cattle one night in March 1983 and came back 47 minutes later. He has never said a word about it.' },
      { id: 'why', label: 'Ask why she sits out here', lines: [
        you('Why sit out here?'),
        RW('Washer\'s broke at home.'),
        aside('There is no basket on the bench beside her.'),
      ] },
      { id: 'cattle', label: 'Tell her about the missing cow', after: 'husband', need: 'ev:cows', lines: [
        you('The pasture south of the motel. There were nine. Now there are eight, and a ring of frost in the grass.'),
        RW('You counted them?'),
        you('Both times.'),
        aside('She turns the radio down. The hiss is still there, very faint.'),
        RW('Write both numbers. Before you start thinking you counted wrong.'),
        you('Did your husband find one missing?'),
        RW('He never said.'),
        aside('She turns the radio back up.'),
        aside('I copy both counts and her words into the notebook.'),
      ], note: 'I told the woman about the pasture: nine cattle, then eight, and the frost ring. She asked whether I counted both times. Her husband never told her what he saw that night.',
      paper: witnessPaper('cows', 'Witness / Two counts at the pasture',
        'PASTURE SOUTH OF THE MOTEL\n\nFirst count: nine cattle.\nSecond count: eight. A ring of flattened grass, white with frost, where the ninth stood.\n\nTHE WOMAN AT THE LAUNDROMAT:\n"You counted them?"\n"Write both numbers. Before you start thinking you counted wrong."\n\nI asked if her husband found one missing the night he went out to the cattle in March 1983.\n"He never said."') },
    ],
  },
  souvenir_man: {
    title: 'SAUCER CRASH 1947 / THE SOUVENIR MAN', status: 'Ash tapped off the step.', bye: 'Leave him his smoke',
    card: { name: 'The souvenir man', role: 'Caliche, Saucer Crash 1947', note: 'Knows the whole story the way the book tells it. Has never seen a thing himself.' },
    open: ({ first }) => first
      ? [SM('Shop\'s closed, friend. Open at ten. Tours on Saturdays.'), aside('He taps the ash off the end of his cigarette onto the step.')]
      : [SM('Still closed.')],
    topics: [
      { id: 'crash', label: 'Ask about the crash', lines: [
        you('The crash. 1947.'),
        SM('Summer of \'47. A rancher out north finds a pasture full of junk. Foil you can\'t crease. Little beams with writing on them.'),
        SM('The Army says flying disc. Same day, they say weather balloon.'),
        SM('Then nothing for thirty years, till the book.'),
        SM('The boys from \'47. That\'s what I call them. The boys.'),
      ] },
      { id: 'seen', label: 'Ask if he has ever seen anything', after: 'crash', lines: [
        you('Have you ever seen anything yourself?'),
        SM('Me?'),
        aside('He takes a long time over the cigarette.'),
        SM('Forty years on this street. Everybody I sell to has a story. Lights. Cattle. The hum.'),
        SM('I sit out here every night it\'s clear. Never seen a thing.'),
        SM('You\'d think they\'d come by the shop once. Out of courtesy.'),
      ], note: 'The souvenir man sits out every clear night and has never seen anything. "You\'d think they\'d come by the shop once."' },
      { id: 'alien', label: 'Ask about the cardboard alien', lines: [
        you('The cardboard alien in the window.'),
        SM('Had it made in Albuquerque. Kids take it about once a month. It always turns up.'),
        SM('Last time it was out on the highway north of town. Standing up, facing the traffic.'),
        SM('Somebody\'s idea of a joke.'),
      ] },
    ],
  },
};
