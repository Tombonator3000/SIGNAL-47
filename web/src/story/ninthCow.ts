import * as THREE from 'three';
import type { BoardCard } from '../ui/Board';
import { RuleBook, pair, type BookNote, type BookRule } from './boardRules';
import { facing } from './seen';
import { clockText } from './time';
import { TAG_RANCH, TAGS } from '../drive/herd';
import { HERD } from '../drive/roadShape';

// Where the ninth went (FORSLAG.md, 20 October, the order's item 6; Tom: "Spenning og skummelt slår
// faste regler"). HENDELSER.md, event 2: nine Herefords in the pasture south of the motel stand
// facing 026; the next time there are eight, and a ring of flattened grass with frost on it where
// the ninth stood. Nothing bloody. This is where she is: at STATION 01, in the same pasture's west
// end 614 m from where she stood, inside the station's chained fence, lying on her side on B, on
// the footpath from the transit out to the comparison vane, her head on 026 the way the herd stands
// (world/stationLayout.ts STATION_NINTH; world/ninthCow.ts builds her).
//
// The cattle mutilations of the late seventies, as they were reported: no blood anywhere, not on
// the ground and not in her; what was taken cut out clean, the edges dark and hard as if burnt (an
// oval of hide off the jaw to the bone, the tongue, a round piece of her side); no tracks to her or
// from her in the frost, and the ground under her not pressed down, as if she had been set there.
// Frost on her open eye. Cold through, colder than the ground, and over her a smell like the racks
// at SARO after an arc. Her ear tag is the herd's: the ranch's name and her number.
//
// A found thing, not an event: the night's curve does not hold it (story/curve.ts), and it has no
// 47. But it keeps the one-at-a-time rule (main.ts oneAtATime): she is not put down and not found
// while another of the night's things is going, and nothing else starts in the quiet round her.
//   - Put down: in chapter three, while the player is in the station's area, nothing else is going,
//     and her place cannot be seen (the player is far off, in the hut, or turned away from it); at
//     the same moment she goes from the pasture (Pasture.takeNinth: the herd far off too). She
//     stays for the rest of the night. Before chapter three she is not there.
//   - Found: on foot, nothing else going, no panel open, out of the hut, with nothing between (the
//     hut, the shed, the pier: Station01.occluders) and something to see her by (within 7 m and on
//     the screen within 50 degrees of its middle, or the flashlight on her within 24 m and 22
//     degrees), for half a second; or a closer look (the zoom) at her. Then the night goes quiet round her for a while (the crickets,
//     the music: `quiet`), a toast, a short note, the mark on the map at STATION 01 on B (which
//     writes the seen log: main.ts sawKey), her card on the board.
//   - Close to (within CLOSE of her, looking at her): what was done to her, once, a toast and a
//     note. From the far end of the flashlight she is a cow in the frost; the rest is up close.
//   - Until she is found, while the player is on foot at the station (not in the hut) and nothing
//     else of the night's is going: something out there in the dark ticks, now and then, the way
//     an engine ticks as it cools, from where she lies (main.ts `tick`, the click at a higher
//     rate, in the world). Heard near enough, once, it is said in words too (a phone is often
//     muted). It stops when she is found, and the night goes quiet.
//   - Looked at closely (the zoom): her head (the eye, the jaw, the tag), her side, the frost.
//   - Used ("Dead cow"): knelt by and touched: cold, the smell.
// Kept with the town's part of the case: 'nine:down', 'nine:tick', 'nine:found', 'nine:close', 'nine:tag', 'nine:touch',
// 'mark:ninth@<clock>', the board's threads as 'nl:<a|b>'; none of it comes again after a load.

/** Her ear tag, as it is read. */
export const NINTH_TAG = `${TAG_RANCH} ${TAGS[HERD.missing]}`;
/** Under a free frame with her in it (core/album.ts, story/albumSubjects.ts). */
export const NINTH_PRINT = 'The ninth one from the pasture, inside the fence. Not a drop of blood.';
/** Seconds the night stays quiet round her once she is found (the crickets, the music). */
const QUIET = 26;
/** Finding her: the eyes on her this long. */
const LOOK = 0.5;
/** Near enough to see what was done to her (metres from her head or her side). */
const CLOSE = 3.4;
/** The ticking: heard from this far (metres), and said in words once heard this near. */
const TICK_R = 45, TICK_SAID = 28;
const LINK = 'nl:';
const PARTS = ['middle', 'head', 'body'] as const;

export interface NinthSite {
  ninthDown: boolean;
  setNinth(on: boolean): void;
  ninthAt(part: 'head' | 'body' | 'middle'): THREE.Vector3;
  /** What stands between the player and things at the station (the hut, the shed, the pier). */
  occluders: THREE.Object3D[];
}
export interface NinthDeps {
  seen: Set<string>;
  site: () => NinthSite | null;
  pasture: { takeNinth(): boolean };
  note: (text: string) => void;
  toast: (text: string, secs?: number) => void;
  /** The crickets and the wind drop out for secs (Ambience.hush). */
  hush: (secs: number) => void;
  /** One tick, from where she lies (main.ts: in the world, so it comes from her way). */
  tick?: (at: THREE.Vector3) => void;
  clock: () => number;
  save: () => void;
  refresh: () => void;
}
export interface NinthIn {
  /** In play, no panel open, not in a cinematic. */
  ok: boolean;
  /** Nothing else of the night's is going (main.ts oneAtATime, all but her own quiet), no call on the line. */
  free: boolean;
  chapter: string;
  area: string;
  driving: boolean;
  /** In the hut at the station (its walls between the player and her). */
  inHut: boolean;
  cam: THREE.Camera;
  /** The flashlight is on. */
  light: boolean;
}

// ---------- the board ----------
const NOTES: Record<string, BookNote> = {
  'n.nine.ring': { text: 'Her tag is the herd\'s. She is the one from the ring of frost in the pasture, six hundred metres east, and nothing walked her here.' },
  'n.nine.fb': { text: 'Jul 3, 1947: nine head. Jul 4: eight. He counted them too, and never wrote down where the ninth went.' },
};
const RULES: BookRule[] = [
  { a: 'nine.tag', b: 'nine.ring', note: 'n.nine.ring', text: NOTES['n.nine.ring'].text },
  // the herd's card from T. Vega's field book, when it is on the board (story/fieldBook.ts)
  { a: 'nine.tag', b: 'tn.cows', note: 'n.nine.ring', text: NOTES['n.nine.ring'].text },
  { a: 'nine.frost', b: 'fb.cows', note: 'n.nine.fb', text: NOTES['n.nine.fb'].text },
  { a: 'nine.tag', b: 'fb.cows', note: 'n.nine.fb', text: NOTES['n.nine.fb'].text },
];

export class NinthCow {
  readonly book: RuleBook;
  /** Seconds left of the quiet round her (main.ts holds the music on it). */
  quiet = 0;
  /** Tests and the developer menu: put her down at the next moment nobody is looking, in any
   *  chapter, without the rest of chapter three's terms. */
  force = false;
  /** Tests of the herd's own order (tools/cowstv.py, tools/f_fieldbook.py): she is not put down,
   *  so the herd at the station in chapter three keeps all nine until it has been seen standing. */
  off = false;
  private look = 0;
  private links: string[] = [];
  private rc = new THREE.Raycaster();
  /** At the last step the player was in the hut (its walls between: no closer look at her from there). */
  private inHut = false;
  /** The next tick, and how many have been heard within TICK_SAID since the night was opened. */
  private tickT = 2;
  private ticks = 0;
  /** At the last step she could have been found (nothing else going, on foot at the station). */
  private canFind = false;

  constructor(private d: NinthDeps) {
    this.book = new RuleBook({
      key: 'ch3',
      owns: (n) => n === 'nine' || n.startsWith('nine.'),
      notes: NOTES, rules: RULES, questions: [],
      links: () => this.links,
      wrong: () => {},
      save: () => { this.keepLinks(); this.d.save(); },
      cards: () => this.cards(),
      empty: 'Not that one. What on her says where she came from?',
    });
  }

  get down() { return this.d.seen.has('nine:down'); }
  get found() { return this.d.seen.has('nine:found'); }
  get close() { return this.d.seen.has('nine:close'); }
  /** For the tests and the developer menu. */
  get state() { return this.found ? 'found' : this.down ? 'down' : 'not yet'; }
  /** Her card on the board, once found. */
  books() { return this.found ? [this.book] : []; }

  // ---------- each frame ----------
  update(dt: number, s: NinthIn) {
    this.quiet = Math.max(0, this.quiet - dt);
    const site = this.d.site();
    if (!site) return;
    // a case opened with her there, or the station built since
    if (this.down !== site.ninthDown) site.setNinth(this.down);
    this.inHut = s.inHut;
    if (!this.down) { this.canFind = false; this.putDown(site, s); return; }
    // (in the hut its walls are between, the east window too far round: she is not seen from there)
    this.canFind = s.area === 'station01' && !s.driving && s.ok && s.free && !s.inHut;
    if (!this.canFind) { this.look = 0; return; }
    const mid = site.ninthAt('middle'), cam = s.cam.getWorldPosition(new THREE.Vector3()), d = cam.distanceTo(mid);
    if (this.found) {
      // close to: what was done to her (her own quiet does not hold this back: it is hers)
      if (!this.close && Math.min(cam.distanceTo(site.ninthAt('head')), cam.distanceTo(site.ninthAt('body'))) < CLOSE
        && PARTS.some((p) => facing(s.cam, site.ninthAt(p), 8, 40)) && this.inSight(site, cam)) this.closeTo();
      this.look = 0; return;
    }
    // before she is found: the ticking, from her
    if (d < TICK_R) this.ticking(dt, site, d);
    // found: the eyes on her with nothing between, and something to see her by (near, on the
    // screen and not far off the middle of it; or in the flashlight)
    const see = (d < 7 ? facing(s.cam, mid, 7, 50) : s.light && d < 24 && facing(s.cam, mid, 24, 22)) && this.inSight(site, cam);
    this.look = see ? this.look + dt : 0;
    if (this.look >= LOOK) this.find();
  }
  private ticking(dt: number, site: NinthSite, d: number) {
    this.tickT -= dt;
    if (this.tickT > 0) return;
    // (now and then; sometimes two close together, as cooling metal does)
    this.tickT = Math.random() < 0.25 ? 0.15 + Math.random() * 0.1 : 1.3 + Math.random() * 2.6;
    this.d.tick?.(site.ninthAt('body'));
    if (d >= TICK_SAID) return;
    this.ticks++;
    if (this.ticks === 3 && !this.d.seen.has('nine:tick')) {
      this.d.seen.add('nine:tick');
      this.d.toast('Somewhere in the dark inside the fence, a small, sharp tick. Then another, the way an engine ticks as it cools.', 5);
    }
  }
  /** Some part of her can be seen from `from`: nothing of the station's (the hut, the shed, the pier) in the way. */
  private inSight(site: NinthSite, from: THREE.Vector3) {
    if (!site.occluders.length) return true;
    for (const part of PARTS) {
      const to = site.ninthAt(part).sub(from), len = to.length();
      if (len < 0.4) return true;
      this.rc.set(from, to.divideScalar(len)); this.rc.near = 0.05; this.rc.far = len - 0.3;
      if (this.rc.intersectObjects(site.occluders, false).length === 0) return true;
    }
    return false;
  }

  /** In chapter three at the station, while nobody can see her place: she is put down there, and goes from the pasture. */
  private putDown(site: NinthSite, s: NinthIn) {
    if (this.off) return;
    if (!(this.force || s.chapter === 'ch3') || s.area !== 'station01' || !s.free || !s.ok) return;
    if (!this.unseen(site, s)) return;
    if (!this.d.pasture.takeNinth()) return;
    this.force = false;
    this.d.seen.add('nine:down');
    site.setNinth(true);
    this.d.save();
  }
  /** Her place cannot be seen from where the player is: far off, in the hut, or turned away from it. */
  private unseen(site: NinthSite, s: NinthIn) {
    if (s.inHut) return true;
    const mid = site.ninthAt('middle'), cam = s.cam.getWorldPosition(new THREE.Vector3()), d = cam.distanceTo(mid);
    if (d > 110) return true;
    const f = s.cam.getWorldDirection(new THREE.Vector3()), to = mid.clone().sub(cam);
    // (outside the widest view there is, a phone on its side, and never right beside the player)
    return d > 6 && f.setY(0).angleTo(to.setY(0)) > THREE.MathUtils.degToRad(78);
  }

  private find() {
    if (this.found) return;
    const seen = this.d.seen, t = clockText(this.d.clock(), false);
    seen.add('nine:found');
    // the map's mark at STATION 01, on B (it also writes the seen log: main.ts sawKey)
    seen.add(`mark:ninth@${Math.round(this.d.clock())}`);
    this.look = 0;
    this.quiet = QUIET;
    this.d.hush(QUIET);
    const ticked = this.ticks >= 2 || seen.has('nine:tick');
    this.d.toast(`Inside the fence, on the path out to the B vane: a cow, on her side in a patch of frost. She does not move.${ticked ? ' The ticking has stopped.' : ''}`, 5.5);
    const ring = seen.has('ev:cows') ? ' She is the one missing from the ring of frost in the pasture.' : seen.has('ev:cows1') ? ' There were nine in the pasture by the highway tonight.' : '';
    this.d.note(`${t}. STATION 01, inside the fence: a Hereford cow, dead on her side on B, between the transit and the comparison vane, her head to 026, in a ring of frost.${ticked ? ' Something out here was ticking like an engine cooling. It stopped when I found her.' : ''} The vehicle gate is chained.${ring}`);
    this.d.refresh();
    this.d.save();
  }
  /** Close to her, once: no blood, and how clean it is. */
  private closeTo() {
    const seen = this.d.seen;
    seen.add('nine:close');
    // (the night stays quiet while it is taken in, if her quiet has nearly gone)
    if (this.quiet < 10) { this.quiet = 10; this.d.hush(10); }
    this.d.toast('No blood. Not on her, not in the frost under her. The jaw bare to the bone, the tongue gone, a round piece of her side cut out clean. The edges are dark and hard.', 7);
    this.d.note(`${clockText(this.d.clock(), false)}. Close to: no blood anywhere, on her or under her. Cut out clean: an oval of the jaw to the bone, the tongue, a round piece of her side, the edges dark and hard. No tracks in the frost round her.`);
    this.d.save();
  }

  // ---------- a closer look, and kneeling by her ----------
  /** What the zoom says about a part of her (Interaction.examine), from where the player is. */
  examine(part: 'head' | 'body' | 'frost', cam: THREE.Camera): string | null {
    const site = this.d.site();
    if (!site?.ninthDown) return null;
    // (not through the hut's walls or the shed: the zoom looks past her hit area to what is there)
    const from = cam.getWorldPosition(new THREE.Vector3());
    if (this.inHut || !this.inSight(site, from)) return null;
    // a closer look finds her too, but not while another of the night's things is going
    if (!this.found) { if (!this.canFind) return null; this.find(); }
    const d = cam.getWorldPosition(new THREE.Vector3()).distanceTo(site.ninthAt(part === 'head' ? 'head' : 'middle'));
    if (d > 7) return part === 'frost' ? 'White frost on the ground, in a ring round something lying there.' : 'A cow, on her side in the frost. She does not move. Nothing near her moves.';
    if (part === 'head') {
      if (!this.d.seen.has('nine:tag')) this.tagNote();
      return `Frost on her open eye. An oval of hide is gone from the jaw, down to the bone, and the mouth hangs open on nothing: the tongue is gone. A yellow tag in her ear: ${NINTH_TAG}.`;
    }
    if (part === 'body') return 'A round piece cut out of her side, clean as a chassis punch. The edges are dark and hard, as if burnt. Not a drop of blood, on her or under her.';
    return 'The frost round her is unbroken. No tracks in it, hers or anyone\'s. The grass under her is not pressed down.';
  }
  private tagNote() {
    const seen = this.d.seen;
    seen.add('nine:tag');
    const ring = seen.has('ev:cows') ? ' She is the ninth.' : '';
    this.d.note(`${clockText(this.d.clock(), false)}. Her ear tag: ${NINTH_TAG}, yellow. Alderete runs the herd in the pasture by the highway, six hundred metres east of here.${ring}`);
    this.d.save();
  }
  label() { return this.d.site()?.ninthDown && this.found ? 'Dead cow' : null; }
  use() {
    const seen = this.d.seen;
    if (seen.has('nine:touch')) { this.d.toast('Still cold. The frost on her has not changed.', 3); return; }
    seen.add('nine:touch');
    this.d.toast('You put your hand on her flank. Cold, colder than the ground, and hard. The frost does not melt under your hand. Over her a smell like the racks at SARO after an arc: hot metal.', 6.5);
    this.d.note(`${clockText(this.d.clock(), false)}. She is cold through, colder than the ground, and the frost on her does not melt under my hand. A smell over her like the racks after an arc. Hot metal, out here.`);
    this.d.save();
  }

  // ---------- the board ----------
  private when() { for (const k of this.d.seen) if (k.startsWith('mark:ninth@')) return clockText(Number(k.slice(11)), false); return null; }
  private cards(): BoardCard[] {
    if (!this.found) return [];
    const t = this.when(), ring = this.d.seen.has('ev:cows');
    const text = `Inside the fence at STATION 01${t ? `, ${t}` : ''}: a cow from the pasture, dead on her side in the frost. No blood.`;
    return [{
      id: 'nine', stamp: 'STATION 01', title: 'THE NINTH, ON B', kind: 'thing', have: true, x: 790, y: 1020, w: 360, text,
      lines: [
        { id: 'nine.tag', text: `Her ear tag: ${NINTH_TAG}, yellow. The herd by the highway.` },
        { id: 'nine.cut', text: 'Cut out clean: the jaw to the bone, the tongue, a piece of her side. No blood.' },
        { id: 'nine.frost', text: 'On B, her head to 026, in a ring of frost. No tracks to her.' },
      ],
    }, {
      id: 'nine.ring', stamp: 'PASTURE', title: 'THE RING OF FROST', kind: 'thing', have: ring, x: 1000, y: 790, w: 170,
      where: ring ? undefined : 'The pasture by the highway, where she stood.',
      text: 'The herd facing 026. Eight. Where the ninth stood, flattened grass in a ring, white with frost.',
    }];
  }
  private keepLinks() {
    for (const k of [...this.d.seen]) if (k.startsWith(LINK)) this.d.seen.delete(k);
    for (const k of this.links) this.d.seen.add(LINK + k);
  }

  // ---------- a night opened ----------
  /** A new night or a case opened: as the case has her; her threads. */
  reset() {
    this.quiet = 0; this.look = 0; this.force = false; this.tickT = 2; this.ticks = 0;
    this.links = [...this.d.seen].filter((k) => k.startsWith(LINK)).map((k) => k.slice(LINK.length)).filter((k) => RULES.some((r) => pair(r.a, r.b) === k));
    this.d.site()?.setNinth(this.down);
  }
  /** The developer menu, the tests: put her down at the next moment nobody is looking. */
  now() { if (!this.down) this.force = true; }
}
