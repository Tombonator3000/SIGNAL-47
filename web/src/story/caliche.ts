import type { Interaction } from '../core/Interaction';
import type { Town } from '../world/Town';

// What Caliche's street says when it is looked at or used (world/Town.ts; HENDELSER.md, section
// 9). Step 1 of the town: the buildings, the windows and the signs, and three things the
// notebook keeps (the flyer on the gas station door, the sheriff's map, the morning paper). The
// people with their lines and the night's events come in step 2.

export interface CalicheDeps {
  inter: Interaction;
  toast: (text: string, secs?: number) => void;
  /** The player is in the diner's area (the street is part of it). */
  here: () => boolean;
  /** The game's clock: seconds since midnight, and as the notebook writes it. */
  secs: () => number;
  clock: () => string;
  note: (text: string) => void;
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
  add('gasWindow', () => 'Window', say('Oil cans in rows, a calendar still on April, the register. The attendant reads behind the counter with his feet up.', 4.2),
    () => 'Behind the counter a man in a pressed shirt turns a page. He has not looked at the clock once.');
  add('pumps', () => 'Pumps', say('Regular, 89.9. It has come down all spring. The nozzles are padlocked for the night.', 3.8),
    () => 'Two pumps on a concrete island. The counters stand at zero.');
  add('payphone', () => 'Payphone', say('A dial tone, and under it something like wind. Nobody to call at this hour.', 3.6),
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
  add('sheriffDoor', () => 'Door', () => d.toast(town.deputyIn ? 'COUNTY SHERIFF, SUBSTATION. A light under the door, and a radio turned low.' : 'Locked. A card in the frame, in ballpoint: BACK SOON.', 3.6), null);
  add('patrolCar', () => town.deputyIn ? 'Patrol car' : null, say('The deputy\'s car. A clipboard on the passenger seat, a thermos, a county map folded small.', 3.8),
    () => town.deputyIn ? 'The hood is still warm. Pins in a cigar box on the dash.' : null);

  // ---------- the laundromat, the Plaza, the feed store ----------
  add('laundromat', () => 'Laundromat', say('Six washers, one tube light, nobody. One machine is running.', 3.6),
    () => 'The one machine turns, stops, turns. There is no basket on the bench and no one waiting.');
  add('bench', () => 'Bench', say('A bench under the window. The paint is worn through in one place, where somebody sits often.', 3.6), null);
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
  add('amDoor', () => 'Door', say('1420 AM. A buzzer and a card: STUDIO IN USE, PLEASE WAIT. Nobody comes.', 3.6), null);
  add('souvenir', () => 'Shop window', say('A cardboard alien, life size if they come in that size. A rubber mask. Bags of crumpled foil, a dollar each: GENUINE DEBRIS.', 4.6),
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
}
