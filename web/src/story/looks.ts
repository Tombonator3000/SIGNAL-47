import * as THREE from 'three';
import type { Interaction } from '../core/Interaction';
import type { Dish } from '../world/Dish';
import { GROUND } from '../world/Crossing';
import { SARO_IN_ROAD } from '../world/geo';
import { surfaceY } from '../drive/roadTerrain';

// What a closer look says (the zoom: right mouse, or Zoom on a phone; core/Interaction.ts) about
// the night's things that had nothing to say (todo.md: the antennas, the signs, the papers on the
// desks, the motel). Two kinds:
//   - things nobody uses: the antennas, the signs at SARO and down the highway, the motel's sign,
//     its ice machine, the car and the pool. Each gets an invisible hit area of its own, only for
//     a closer look (look: true), so it never stands in the way of using what is behind it.
//   - things the chapters already bind for use (the papers, the phones, the diner, Caliche's
//     street, room 6): a text by their id in Interaction.looks, read when they have no examine()
//     of their own. The chapters bind them when they are built; the texts wait here.
// Short and plain, the way the shift log is written. No new facts the story leans on.

export interface LooksDeps {
  inter: Interaction;
  /** SARO's antennas (Exterior.dishes): where they point, and whether they are turning. */
  dishes: Dish[];
  /** SARO's own outdoor group (Exterior.group): the hit areas stand in it and hide with it. */
  saro: THREE.Object3D;
  /** The motel's front (MotelFront): its hit areas, by id. */
  motel: { proxies: Record<string, THREE.Object3D> };
  /** The motel's road sign, both faces (Exterior.motel). */
  motelSign: THREE.Object3D[];
}

const pad = (n: number) => String(((Math.round(n) % 360) + 360) % 360).padStart(3, '0');
const hit = new THREE.MeshBasicMaterial({ visible: false });

/** A few things said about an antenna, chosen by its number. */
const DISH = [
  'A white dish on a concrete pedestal, the red lamp blinking on the feed legs.',
  'Rust runs down the pedestal from every bolt. Two of the reflector panels are newer than the rest.',
  'The drive motors tick as it tracks. There is frost on the lower rim.',
  'A ladder up the pedestal, chained at the bottom. Its red lamp blinks out of step with the next one.',
  'Bird droppings down one side of the feed housing. The number is stencilled on the pedestal twice.',
];

export function bindLooks(d: LooksDeps) {
  const { inter } = d;
  const looks = (id: string, say: () => string | null, range?: number) => { inter.looks[id] = { say, range }; };
  const say = (text: string) => () => text;
  // an invisible box only for a closer look (in SARO's outdoor group, world metres)
  const box = (id: string, w: number, h: number, dp: number, x: number, y: number, z: number, ry: number, text: () => string | null, range = 14) => {
    const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, dp), hit);
    m.position.set(x, y, z); m.rotation.y = ry; m.name = 'look:' + id;
    d.saro.add(m);
    inter.add({ id: 'look:' + id, object: m, label: () => null, use: () => {}, look: true, examine: text, examineRange: range });
  };

  // ---------- the antennas: the pedestal, and round the dish whichever way it points ----------
  const ped = new THREE.CylinderGeometry(2.1, 2.1, 7.8, 8), ball = new THREE.IcosahedronGeometry(9.2, 1);
  d.dishes.forEach((dish, i) => {
    const g = new THREE.Group(), s = dish.scale;
    g.name = 'look:' + dish.id;
    g.position.set(dish.x, 0, dish.z); g.scale.setScalar(s);
    const a = new THREE.Mesh(ped, hit); a.position.y = 3.9;
    const b = new THREE.Mesh(ball, hit); b.position.y = 12.4;   // the elevation axle, where the dish turns
    g.add(a, b);
    d.saro.add(g);
    const line = dish.id === 'S-03' ? 'S-03, at the end of the east walk, its motor cabinet on the apron.' : DISH[i % DISH.length];
    inter.add({ id: 'look:' + dish.id, object: g, label: () => null, use: () => {}, look: true, examineRange: 60,
      examine: () => {
        const az = pad(dish.curAz), el = Math.round(dish.curEl);
        if (dish.moving) return `${dish.id}. It is turning, the drive motors whining. Az ${az}, el ${el}.`;
        // come down out of 026 by itself (story/dishesFollow.ts), standing a moment where it turned to
        if (dish.rogue) return `${dish.id}. It has come down out of 026 and stopped. Az ${az}, el ${el}. Nobody sent a command.`;
        // after 02:13 every dish in the array stands at 026, and nobody sent a command
        if (az === '026') return `${dish.id}. Az 026, el ${el}. Every dish in the array points the same way.`;
        return `${dish.id}. ${line} Az ${az}, el ${el}.`;
      } });
  });

  // ---------- signs round SARO (ServiceYard.ts, Grounds.ts, Exterior.ts) ----------
  box('yardSign', 0.08, 0.56, 1.06, 8.05, 0.9, -3.4, 0, say('SERVICE YARD. AUTHORIZED PERSONNEL ONLY. The enamel has chipped away round the screws.'));
  box('routeSign', 0.08, 0.4, 0.66, 10.98, 0.85, -2.6, 0, say('EAST WALK. B-12: NORTH OF S-03. Lettered by hand, under varnish gone yellow.'));
  box('b12Sign', 0.08, 0.36, 0.56, 8.05, 0.85, -15.6, 0, say('B-12, OPTICAL REFERENCE. A typed card behind plexiglass gone cloudy.'));
  box('labSign', 0.08, 0.42, 0.84, 12.58, 2.45, -1.2, 0, say('PHOTOGRAPHIC LAB. PROCESS / EXAMINE / FILE. Under it, in marker: SHUT THE DOOR WHEN THE RED LIGHT IS ON.'));
  box('siteSign', 0.6, 1.3, 3.5, -14.5, GROUND + 0.65, 20.8, 0,
    say('SARO. SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY. AUTHORIZED VEHICLES ONLY. NO VISITORS AFTER DARK. Sun-bleached on the side facing the highway.'), 20);
  box('quietSign', 0.1, 0.9, 1.36, -12.55, GROUND + 1.55, 28.3, 0,
    say('RADIO QUIET ZONE. NO TRANSMITTERS BEYOND THIS SIGN. The CB under the truck\'s dash is meant to stay off past here.'), 20);
  // the stop sign where the service road meets the highway (Grounds.ts: ROAD_Z 24.5, less half the road and 0.9)
  box('stopSign', 0.1, 0.8, 0.8, -18.95, GROUND + 2.0, 24.5 - 2.5 - 0.9, 0, say('STOP. The highway is empty both ways, as far as the dark goes.'), 20);
  box('wingLetters', 8, 2.6, 0.12, 26.4, 2.0, 5.55, 0, say('SARO, and under it SOUTHWEST ASTRONOMICAL RESEARCH OBSERVATORY. One bolt in the last word has rusted through.'), 30);
  // ---------- down the highway to the motel: where drive/roadProps.ts signs() puts them (road terms) ----------
  const road = (rx: number, rz: number, ry: number, w: number, h: number, bottom: number, id: string, text: string) => {
    const y = surfaceY(rx, rz) + bottom + h / 2;
    box(id, w + 0.1, h + 0.1, 0.12, rx - SARO_IN_ROAD.x, y, rz - SARO_IN_ROAD.z, ry, say(text), 24);
  };
  road(-7.6, 60, Math.PI, 0.76, 0.95, 1.5, 'speedSign', 'SPEED LIMIT 55. Pocked with birdshot along the bottom edge.');
  road(-7.4, 210, Math.PI, 0.3, 0.6, 1.0, 'mileSign', 'MILE 112. A green plate on a steel post, the white numbers gone dull.');
  road(-9.2, 315, Math.PI, 2.6, 0.98, 1.8, 'roswellSign', 'ROSWELL 64. Sixty-four miles of nothing, and then Roswell.');
  road(-12.5, 511.5, 2.6, 1.8, 1.13, 1.0, 'stationSign', 'STATION 01. U.S. SURVEY. NO THROUGH ROAD. Old enamel on wooden posts, and the survey track going off behind it.');

  // ---------- Sierra Motor Court (MotelFront.ts; chapter four binds the office and the doors) ----------
  const motel = (id: string, text: string, range = 10) => {
    const o = d.motel.proxies[id];
    looks('court:' + id, say(text));
    // its own hit area until chapter four binds the proxy for use (then the text above answers)
    if (o && o.userData.interactId === undefined) inter.add({ id: 'look:motel.' + id, object: o, label: () => null, use: () => {}, look: true, examine: say(text), examineRange: range });
  };
  motel('officeDoor', 'OFFICE, in gold letters on the glass, half of them flaked away. A bell on the counter inside.');
  motel('otherDoors', 'A brass number on the door, the curtains drawn. Dark behind them.');
  motel('iceMachine', 'ICE. It rattles, and drops a load into the bin for nobody.');
  motel('car', 'A dusty sedan nosed in by the office, dew on the glass. It has not been driven tonight.');
  motel('pool', 'The pool behind its fence, closed for the season. Leaves turn slowly on the water.', 16);
  // (inside the office: from close by, not through its walls)
  motel('register', 'A ledger in green cloth, open at April. The same name, all the way down the page.', 3.5);
  motel('keyBoard', 'Brass tags worn smooth by thumbs. One hook is empty.', 3.5);
  motel('officePhone', 'An oak wall telephone with a crank, older than anything else in the office.', 3.5);
  motel('envelope', 'T. VEGA, C/O SIERRA MOTOR COURT. A three-cent stamp, and a wax seal nobody has broken.', 3.5);
  motel('message', 'Pencil on the back of a bill, pressed hard enough to dent the paper.', 3.5);
  looks('room6Door', say('A brass 6 on the door, and a light behind the curtains.'));
  if (d.motel.proxies.room6Door && d.motel.proxies.room6Door.userData.interactId === undefined) {
    inter.add({ id: 'look:motel.room6Door', object: d.motel.proxies.room6Door, label: () => null, use: () => {}, look: true, examine: say('A brass 6 on the door, and a light behind the curtains.'), examineRange: 10 });
  }
  for (const face of d.motelSign) inter.add({ id: 'look:' + face.name, object: face, label: () => null, use: () => {}, look: true, examineRange: 40,
    examine: say('Sierra in red script, MOTOR COURT in cyan, a yellow star on top. CLEAN ROOMS. CABLE TV. VACANCY. The neon hums.') });

  // ---------- room 6 (Chapter4.ts) ----------
  looks('r6:bed', say('A chenille spread pulled tight. The pillow has not been touched.'));
  looks('r6:lamp', say('A ceramic lamp with a paper shade gone brown on the side nearest the bulb.'));
  looks('r6:tv', say('A portable set on the dresser, rabbit ears bent to find Roswell.'));
  looks('r6:window', say('The Sierra sign through a gap in the curtains, red and cyan, and the highway.'));
  looks('r6:phone', say('A beige phone. The cord runs to a wall jack marked OFFICE.'));
  looks('r6:bathroom', say('Shut. A tap drips behind it, steady as a clock.'));
  looks('r6:walkie', say('A field radio in olive drab, its whip aerial folded down. The battery door is taped shut.'));
  looks('r6:photo', say('A snapshot in a dime-store frame, turned a little towards the window.'));
  looks('r6:door', say('The chain is off. The door to the walk, and the cold behind it.'));
  looks('r6:table', say('A table by the window: the papers, a cup gone cold, her reading glasses on their cord.'));

  // ---------- the control room: the papers on the desks, and the rest (Prologue.ts, Decoder.ts) ----------
  looks('logbook', say("Dale's shift log, open at tonight. His writing leans harder to the right as the page goes on."));
  looks('workOrder', say('The night work order, typed, initialled D. A coffee ring on one corner.'));
  looks('excBinder', say('A grey ring binder, SOLVER EXCEPTIONS on the spine, fat with fan-fold.'));
  looks('serviceRecord', say("RX bank 3's service record, in three different hands. The last is Dale's."));
  looks('telex', say('The telex roll, curling off the machine and onto the floor.'));
  looks('rfiCard', say("An index card taped to the console. Dale's block capitals."));
  looks('halley', say("The comet over a desert, in a cheap frame. Somebody has pencilled in the night it came closest: April 11."));
  looks('opsTerminal', say('The operations terminal. Green letters, and a cursor waiting.'));
  looks('map', say('A road map of New Mexico, the folds worn white. The pencil ring round Roswell has been gone over twice.'));
  looks('clock', say('An electric wall clock with a sweep hand. It hums, and the hand never stops.'));
  looks('printer', say('Dot matrix. The fan-fold feeds up out of a box under the table.'));
  looks('phone', say('A beige desk phone, two lines. The supervisor line has a light of its own.'));
  looks('rack', say('RX bank 3. The rack is warm to the touch and smells of dust burning off.'));
  looks('crtCenter', say('The receiver console, its screen in a hood of black card against the glare.'));
  looks('crtLeft', say('The status terminal: one line for every dish, S-01 to S-27.'));
  looks('crtRight', say('The tracking terminal. The keys in the middle are worn blank.'));
  looks('coffeePot', say('A drip coffee maker on a hot plate, the glass stained brown to the line.'));
  looks('mug', say('SARO, in brown on white. Chipped at the handle.'));
  looks('radio', say('A transistor radio on the shelf, its aerial bent where somebody caught it.'));
  looks('fieldCamera', say('The SARO field camera in its dented case, a sheet film holder beside it.'));
  looks('decoder', say('The signal processor: a tape deck and two spools behind smoked plastic.'));
  // ---------- SARO's doors (Doors.ts) ----------
  looks('doorEast', say('The service door. A push bar, and a card: KEEP CLOSED. DUST.'));
  looks('doorSouth', say('The corridor door, a square of wired glass at head height.'));
  looks('labDoor', say('The lab door. A red bulb over it for when the safelight is on.'));
  looks('exitDoor', say('FIRE EXIT, stencilled in red on grey steel.'));
  looks('recordsDoor', say('RECORDS, on a card in a brass holder.'));
  // ---------- the yard and the records room (Chapter1.ts, Chapter2.ts) ----------
  looks('motorBus', say("S-03's motor bus cabinet. A sticker on the door: DO NOT RESET WITHOUT A LOG ENTRY."));
  looks('wetBench', say('Three trays, tongs, brown bottles in a row, and the smell of fixer.'));
  looks('archive', say('The archive bench: a light box, a loupe on a string, a print tray.'));
  looks('b12Control', say("B-12's local control, a grey box on a post with its key on a chain."));
  looks('vane', say('A plate on a post with one ivory stripe, turned to face the apron.'));
  looks('records', say('The night report on a clipboard: observation, method, conclusion.'));
  looks('r07', say('Three black bars on white, the paint crazed by the sun.'));
  looks('fieldBox', say('A records box, STATION 01 1947 in grease pencil. The lid is soft with handling.'));
  looks('binder', say('A binder of service copies, the spine cracked white.'));
  looks('lineage', say('A long drawer of index cards with a brass pull: MAINTENANCE.'));
  looks('workTable', say('A big table under a strip light, scored all over with a craft knife.'));
  looks('recPhone', say('A wall phone, beige, the cord stretched out long.'));
  looks('vending', say('Cola, orange soda, peanut butter crackers. The light inside flickers.'));
  looks('officeDoor', say('A frosted glass door, OPERATIONS in black. Dark behind it.'));
  looks('restroomDoor', say('MEN and WOMEN. The fan inside runs all night.'));

  // ---------- STATION 01 (Chapter3.ts) ----------
  looks('s1transit', say('A brass transit on a concrete pier, its cover off. The bubble still settles.'));
  looks('s1footing', say('Grit in the bolt holes. Nothing has stood on it for a long time.'));
  looks('s1markerA', say('A triangle outlined on a plate, and a bar under it.'));
  looks('s1markerB', say('A vane plate on a post, still set square in its footing.'));
  looks('s1stakes', say('Wooden stakes going east into the dark, grey with age.'), 8);
  looks('s1diagram', say('A survey diagram in a frame, the glass fly-spotted.'));
  looks('s1lamp', say('A field lamp on a stand, the glass sooted on one side.'));
  looks('s1cable', say('A field cable in the dirt, the insulation cracked by the sun.'));
  looks('s1hutDoor', say('Boards gone silver, and a door that has not been locked in years.'));
  looks('s1receiver', say('A valve receiver on the bench, older than anything at SARO.'));
  looks('s1timing', say('A timing log on a clip: pencil, columns of seconds.'));
  looks('s1phone', say('A field telephone with a crank. Its line leaves through the wall.'));
  looks('s1gate', say('The vehicle gate, chained and padlocked.'), 10);

  // ---------- Mesa Diner (Chapter5.ts) ----------
  looks('diner:waitress', say('A pencil behind her ear. She looks at the window each time a truck might pass.'));
  looks('diner:driver', say('A driver in a quilted vest, eating slowly. A feed store\'s name on his cap.'));
  looks('diner:coffee', say('The pot on the warmer, a ring of old coffee baked round the rim.'));
  looks('diner:counter', say('A radio on the shelf behind the counter, between the pie dome and the register.'));
  looks('diner:clipping', say('Yellow newsprint behind a strip of tape, the date cut off.'));
  looks('diner:payphone', say('A payphone in a niche. Numbers written all over the wall round it.'));
  looks('diner:booth', say('Red vinyl, split and taped. The window beside it is cold.'));
  looks('diner:menu', say('White letters pushed into a felt board. The P in PIE is upside down.'));
  looks('diner:jukebox', say('OUT OF ORDER, in ballpoint behind the glass. A record still on the arm.'));
  looks('diner:window', say('The lot, the truck, the highway, and past it the dark.'));
  looks('diner:door', say('MESA DINER, OPEN ALL NIGHT, on the glass. The closer hisses.'));
  looks('diner:sign', say('MESA DINER. One letter of the neon buzzes, goes out, and buzzes.'), 14);
  looks('diner:rig', say('A refrigerated trailer, the unit running. Frost on the doors at the back.'), 12);

  // ---------- Caliche, what had nothing to say yet (caliche.ts) ----------
  looks('town:bench', say('A bench under the window, the paint worn through where somebody sits.'), 6);
  looks('town:gasSign', say('GAS. DIESEL. The tubes in one side of the sign are dark.'), 12);
  looks('town:feed', say('FEED & SEED. Moths round the bulb over the door, and sacks on the dock.'), 10);
  looks('town:amDoor', say('1420 AM. STUDIO IN USE, PLEASE WAIT. The buzzer button is worn white.'), 6);
  looks('town:souvenirSign', say('SAUCER CRASH 1947. TOURS. ASK INSIDE. Under the paint, an older word: BAIT.'), 10);
  looks('town:souvenirDoor', say('CLOSED. The hours on the card have faded white.'), 6);
  looks('town:letters', say('THE HEAVENS DECLARE THE GLORY OF GOD. PS 19:1. One letter hangs crooked.'), 10);
  looks('town:churchDoor', say('SUNDAY 10 AM. ALL WELCOME. The doors are new; the step is worn hollow.'), 8);
  looks('town:tv2', say('Snow, then the test pattern, then snow. Nobody in the room.'), 9);
  looks('town:kitchen', say('A kitchen light behind a curtain, and a shadow that does not move.'), 7);
}
