import './style.css';
import * as THREE from 'three';
import { UI } from './ui/UI';
import { AudioSys, type Surface } from './core/Audio';
import { ReadAloud } from './core/readAloud';
import { EarlDialogue } from './core/earlDialogue';
import { NPCDialogue } from './core/npcDialogue';
import { serlingBank } from './core/narrationBanks';
import { Input } from './core/Input';
import { Interaction } from './core/Interaction';
import { Player } from './player/Player';
import { Sky } from './world/Sky';
import { Backdrop } from './world/backdrop';
import { Exterior } from './world/Exterior';
import { ControlRoom } from './world/ControlRoom';
import { Prologue } from './story/Prologue';
import { ServiceYard, YARD } from './world/ServiceYard';
import { RecordsAnnex } from './world/Annex';
import { Vhs, type Picture } from './core/vhs';
import { FieldCamera } from './core/FieldCamera';
import { setQuality, type Quality } from './core/quality';
import { Ultra } from './core/ultra';
import { Flashlight } from './core/Flashlight';
import { loadFonts } from './core/fonts';
import { DebugHud, debugOn } from './core/debug';
import { DevMenu, devOn, type DevGroup } from './ui/DevMenu';
import { Score, CUES, TRACKS, type Cue } from './core/score';
import { Radio, FREQ, DIAL, dialX, SIGN_OFF, SIGN_ON, type StationId, BOY } from './core/radio';
import { TripRadio } from './story/tripRadio';
import { NightCalls } from './core/calls';
import { CB } from './core/cb';
import { Roadside } from './core/roadside';
import { areaMap, areaMapDescription, type MapView } from './ui/areaMap';
import type { KitItem, PersonCard } from './ui/UI';
import { portrait, portraitCount } from './core/portraits';
import { glowScale } from './world/glow';
import { SaveStore, type SaveKind, type SaveMeta } from './core/saves';
import { saveMenu, played } from './ui/SaveMenu';
import { type GameState, migrateOldSave } from './story/state';
import { clockText } from './story/time';
import { loadArt, artStatus, artImage } from './core/art';
import { initArtMaterials, floodSets, headCookie } from './world/kit';
import { windowLights, setWindowCookies } from './world/windowLight';
import { FLASH_DECAY } from './core/cookies';
import { World, STATION_ORIGIN as STATION_AT, ROAD_ORIGIN } from './world/World';
import { SARO_IN_ROAD } from './world/geo';
import { Doors } from './world/Doors';
import { bindTown, townPeople, townWitnessDocs } from './story/caliche';
import { bindField } from './story/field';
import { TownEvents, townMarks, ON_B } from './story/townEvents';
import { Glimpses, type GlimpsesIn } from './core/glimpses';
import { Cutouts, type CutoutsIn } from './core/cutouts';
import { NightEvents, BULLETIN, type NightIn } from './story/nightEvents';
import { Abduction, type AbductIn } from './story/abduction';
import { DishesFollow, nearArray } from './story/dishesFollow';
import { LineLights } from './story/lineLights';
import { Pasture, PASTURE_M } from './story/pasture';
import { MotelTv } from './story/motelTv';
import { Album, rollText } from './core/album';
import { albumSubjects } from './story/albumSubjects';
import { albumArt } from './ui/album';
import { surfaceY } from './drive/roadTerrain';
import { bindLooks } from './story/looks';
import { firstTime } from './core/hints';
import { BlenderHouse, BlenderTruck, blenderWanted } from './world/blenderModels';
import { startModels, licensedModels, modelsQuality, warmShown } from './world/licensedModels';
import type { AreaId } from './story/Chapter3';

// Settings and the one checkpoint live in localStorage. Every access is guarded,
// because storage can be blocked (private mode, sandboxed frames).
const store = {
  get<T>(k: string, d: T): T { try { const v = localStorage.getItem('s47.' + k); return v === null ? d : JSON.parse(v) as T; } catch { return d; } },
  set(k: string, v: unknown) { try { localStorage.setItem('s47.' + k, JSON.stringify(v)); } catch { /* ignore */ } },
};
const wait = (ms: number) => new Promise((r) => setTimeout(r, ms));

/** How much of the sky's haze the far land takes at night and at dawn in the night areas (main.ts
 *  loop; on the old road all of it at dawn): no lighter than the foot of the painted mesas behind it. */
const NIGHT_FOG = 0.36, DAWN_FOG = 0.42;

async function boot() {
  const ui = new UI();
  const loading = ui.loading();
  // Textures draw text with these fonts, so they must be ready before the world is built.
  try {
    await Promise.all([Promise.race([loadFonts(), wait(3000)]), loadArt()]);
  } catch {
    loading.innerHTML = '<p class="err">The station artwork could not load. Check your connection and try again.</p><button>TRY AGAIN</button>';
    loading.querySelector('button')!.onclick = () => location.reload();
    return;
  }
  initArtMaterials();
  await wait(30);

  const touchGuess = matchMedia('(pointer: coarse)').matches;
  let renderer: THREE.WebGLRenderer;
  try {
    renderer = new THREE.WebGLRenderer({ antialias: !touchGuess, powerPreference: 'high-performance' });
  } catch {
    loading.innerHTML = '<p class="err">This device could not start WebGL, so the game cannot run here. Try a recent Chrome, Safari or Firefox.</p>';
    return;
  }
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.05;
  let maxPR = Math.min(window.devicePixelRatio || 1, touchGuess ? 1.6 : 2);
  let pr = maxPR;
  renderer.setPixelRatio(pr);
  document.getElementById('stage')!.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0x03050a);
  scene.fog = new THREE.FogExp2(0x0b0f19, 0.0021);
  const camera = new THREE.PerspectiveCamera(70, innerWidth / innerHeight, 0.03, 5000);
  camera.rotation.order = 'YXZ';

  const sky = new Sky();
  // the painted horizon all round, on the sky's ring (world/backdrop.ts, ART_BRIEF.md round 16)
  const backdrop = new Backdrop(sky, { night: artImage('horizonNight'), dawn: artImage('horizonDawn') });
  const ext = new Exterior();
  const room = new ControlRoom();
  const yard = new ServiceYard();
  const annex = new RecordsAnnex();
  scene.add(sky.group, ext.group, room.group, yard.group, annex.group);
  const fcam = new FieldCamera(renderer, scene, camera);

  // Ultra on a computer (an automated browser starts on High, so the budgets stay testable),
  // High on a phone; a phone never gets Ultra, even from an old setting
  const ultraOk = !touchGuess;
  let quality: Quality = store.get<Quality>('quality', ultraOk && !navigator.webdriver ? 'ultra' : 'high');
  if (quality === 'ultra' && !ultraOk) quality = 'high';
  const ultra = new Ultra(renderer, scene);
  // the flashlight from the truck's passenger seat (core/Flashlight.ts, Tom 15 October)
  const flashlight = new Flashlight(scene);
  let zoomK = 0;   // the closer look, 0 to 1 (right mouse, Zoom on a phone)
  ultra.addShadowLight(room.lights.lamps[0]);   // the lamp on the supervisor desk
  // the picture: a 1986 tape look over the whole night (core/vhs.ts), off by default on Low
  // (until the player picks one in Settings, the picture follows the graphics level)
  const vhs = new Vhs(renderer, quality !== 'low' ? 4 : 0);
  const applyPicture = (p: Picture, chosen = true) => {
    vhs.picture = p;
    if (chosen) store.set('picture', p);
    document.documentElement.classList.toggle('vhs-on', vhs.on);   // the shader has its own grain
  };
  applyPicture(store.get<Picture | null>('picture', null) ?? (quality !== 'low' ? 'vhs' : 'off'), false);
  renderer.info.autoReset = false;   // two passes per frame: count both (the draw-call budget)
  function applyQuality(q: Quality) {
    quality = q; store.set('quality', q);
    vhs.setSamples(q !== 'low' ? 4 : 0);
    if (store.get<Picture | null>('picture', null) === null) applyPicture(q !== 'low' ? 'vhs' : 'off', false);
    setQuality(scene, q);
    modelsQuality(q === 'low');   // (the bought models' pictures to the level's size, world/licensedModels.ts)
    ultra.set(q === 'ultra');
    vhs.ultra = q === 'ultra';
    maxPR = Math.min(window.devicePixelRatio || 1, q === 'low' ? 1 : (touchGuess ? 1.6 : 2));
    pr = Math.min(pr, maxPR);
    renderer.setPixelRatio(pr);
    resize();
  }

  // saved cases and their photographs (IndexedDB), read while the title screen is up
  const saves = new SaveStore<GameState>(migrateOldSave);
  let caseId = 0;                          // the case being played, 0 before a night starts
  let playtime = 0;                        // seconds of active play in this case
  let restoring: GameState | null = null;  // the save being opened; the chapters read their case from it
  const audio = new AudioSys();
  const readAloud = new ReadAloud(audio);
  readAloud.enabled = store.get<boolean>('kinskiReadAloud', false) === true;
  ui.readAloud = readAloud;
  const earlDialogue = new EarlDialogue(audio);
  earlDialogue.enabled = store.get<boolean>('artBellRadioHost', true) === true;
  ui.earlDialogue = earlDialogue;
  const narrator = new ReadAloud(audio, serlingBank);
  narrator.enabled = store.get<boolean>('serlingNarrator', true) === true;
  ui.narrator = narrator;
  const npcDialogue = new NPCDialogue(audio);
  npcDialogue.enabled = store.get<boolean>('characterVoices', true) === true;
  ui.npcDialogue = npcDialogue;
  audio.characterVoices = npcDialogue.enabled;
  audio.kinskiCalls = readAloud.enabled;
  const stopNarration = (except?: object) => {
    for (const player of [readAloud, narrator, earlDialogue, npcDialogue]) if (player !== except) player.stop();
  };
  ui.onReadingStart = (reader = readAloud) => { stopNarration(reader); audio.stopPhoneCalls(); cb.stop(); };
  ui.onEarlStart = () => { stopNarration(earlDialogue); audio.stopPhoneCalls(); cb.stop(); };
  ui.onCharacterStart = () => { stopNarration(npcDialogue); audio.stopPhoneCalls(); cb.stop(); };
  audio.onPhoneCall = () => { stopNarration(); cb.stop(); };
  audio.setVolume(store.get('vol', 0.8));
  audio.setMusic(store.get('music', 0.7));
  audio.setRadio(store.get('radioVol', 1));
  audio.preload(); // decodes while the title screen is up
  // the stations of the night, the truck's radio and the one behind the diner's counter (core/radio.ts)
  const radio = new Radio(audio);
  radio.setHostVoice(store.get<boolean>('artBellRadioHost', true) === true);
  radio.setCallerVoice(store.get<boolean>('radioCallerVoices', true) === true);
  const carRadio = radio.receiver({ vol: 0.38, ref: 1.0 });   // Tom, 18 October: 0.65 was too loud in the cab
  const dinerRadio = radio.receiver({ vol: 0.3, ref: 1.6 });
  // something out in the dark now and then, when the player walks about with the flashlight (core/calls.ts)
  const calls = new NightCalls(audio);
  // the CB under the truck's dash: someone calls it now and then (core/cb.ts)
  const cb = new CB(audio);
  // the walk down the highway to the motel: the line, a car far off, the silence, the roof (core/roadside.ts)
  const roadside = new Roadside(audio, scene);
  // them, in the background, at the edge of the view (core/glimpses.ts)
  const glimpses = new Glimpses(scene);
  // the souvenir shop's cardboard alien, turning up anywhere (core/cutouts.ts; Tom, 18 October)
  const cutouts = new Cutouts(scene, (first) => {
    if (first) game.note(`${clockText(game.clock, false)}. A cardboard alien, standing where nobody would put one. Nobody was there.`);
  });
  // the time away (story/abduction.ts), on by default; the setting turns it off
  let abductOn = store.get('abductions', true);
  // eslint-disable-next-line prefer-const
  let abduction: Abduction | null = null;
  const input = new Input(renderer.domElement, ui.touch);
  input.sensitivity = store.get('sens', 1);
  const inter = new Interaction();
  const colliders = [...room.colliders, ...yard.colliders, ...annex.colliders];
  const saroZones = [room.bounds, ...yard.zones, ...annex.zones];
  const player = new Player(camera, colliders, saroZones);
  // SARO, the road and STATION 01, and the drive between them (world/World.ts)
  const world = new World({
    scene, camera, player,
    saro: { groups: [ext.group, room.group, yard.group, annex.group], zones: saroZones, colliders, truck: YARD.truck, truckCol: yard.truckCol, truckProxy: yard.objs.truck },
    applyQuality: () => { setQuality(scene, quality); ultra.mark(scene); },
    audio: () => ({ ctx: audio.ctx, sfx: audio.sfx }),
    thud: (gain) => audio.play('thudSoft', { gain, rate: 0.8 }),
    keys: () => audio.play(audio.or('keys'), { gain: 0.6 }),
    fade: (on, text) => ui.fade(on, text ?? ''),
    hold: (on) => { game.cinematic = on; if (on) input.reset(); },
    after: (sec, fn) => game.after(sec, fn),
    toast: (text, secs) => ui.toast(text, secs),
    clock: () => clockText(game.clock, false),
    skipClock: (sec) => { game.clock += sec; },
    touch: () => input.touchMode,
    sky: () => ({ dawn: sky.uniforms.uDawn.value, fog: sky.fogColor, sunDir: sky.uniforms.uSunDir.value, sun: sky.uniforms.uSun.value }),
    time: () => game.clock,
  });
  // the Blender pilot: SARO's house modelled and baked in Blender in place of the boxes, with
  // ?blender in the address (world/blenderModels.ts, tools/blender/build.py)
  const blenderHouse = new BlenderHouse(ext.group, [room.outside, annex.outside, ...ext.house], (root) => { setQuality(root, quality); ultra.mark(root); },
    [ext.glow, annex.glow], () => world.grounds.dressing?.props ?? null);
  if (blenderWanted) void blenderHouse.set(true);
  // the control room's windows on the ground outside (world/windowLight.ts), laid on SARO's grounds
  room.lightOutside((x, z) => world.grounds.floorAt(x, z));
  // and the truck, every copy the world builds (step two: drive/Truck.ts setModel)
  const blenderTruck = new BlenderTruck((root) => { setQuality(root, quality); ultra.mark(root); });
  world.onTruck = (t) => blenderTruck.add(t);
  if (blenderWanted) void blenderTruck.set(true);
  // the bought models (world/licensedModels.ts, only with the private assets): the areas built so
  // far have their holders; their .glbs load now, a later area's as it is built. A phone and Low
  // get their pictures smaller; each area's pictures go up to the graphics card and its programs
  // are compiled before its models are shown (and again for an area's lights when it is entered,
  // World.onEnter)
  startModels({ prepare: (root) => { setQuality(root, quality); ultra.mark(root); }, touch: touchGuess, low: quality === 'low',
    upload: (t) => renderer.initTexture(t), compile: (root) => renderer.compileAsync(root, camera, scene) });
  world.onEnter = () => warmShown();
  // footsteps by what is underfoot: vinyl and tiles inside SARO, concrete in the yard, on the
  // highway and at the motel, dirt between them, boards in the hut, carpet at the motel
  const surfaceAt = (): Surface => {
    const p = player.pos;
    if (world.area === 'room6') return 'carpet';
    if (world.area === 'diner') return world.indoors(p) ? 'tile' : world.town?.inTown(p.x, p.z) ? world.town.surfaceAt(p.x, p.z) : world.roadStopAt(p)?.paved ? 'concrete' : 'dirt';
    if (world.area === 'station01') return world.indoors(p) ? 'wood' : 'dirt';
    if (world.area !== 'saro') return 'dirt';
    if (inside(motelOffice, p)) return 'carpet';
    if (space !== 'yard') return 'tile';
    if (world.grounds.onRoad(p) || world.crossing.atMotel(p)) return 'concrete';
    if (p.z > 64 && Math.abs(p.x + 24) < 6) return 'concrete';   // the highway south and its shoulders
    if (world.field.onHighway(p)) return 'concrete';               // and north through the array
    return player.floorY < -0.3 ? 'dirt' : 'concrete';
  };
  player.onStep = () => { if (!roadside.silent && !abduction?.silent) audio.step(surfaceAt()); };   // the silence takes the footsteps too
  ui.onPaper = () => audio.play(Math.random() < 0.5 ? 'paper0' : 'paper1', { gain: 0.35, rate: 0.95 + Math.random() * 0.1 });
  // SARO's doors open and shut at any time of the night (world/Doors.ts)
  const doors = new Doors({ inter, audio, colliders, player, toast: (text, secs) => ui.toast(text, secs), busy: () => game.cinematic || !!ui.modal });
  doors.add({ id: 'east', inter: 'doorEast', name: 'service door', proxy: room.objs.doorEast, set: (k) => room.setDoor(k),
    zones: [yard.zone.eastDoor], leaf: { minX: 6.06, maxX: 7.0, minZ: 1.08, maxZ: 1.18 } });
  doors.add({ id: 'south', inter: 'doorSouth', name: 'corridor door', proxy: room.objs.doorSouth, set: (k) => room.setSouthDoor(k),
    zones: [annex.zone.southDoor], leaf: { minX: -1.99, maxX: -1.87, minZ: 4.72, maxZ: 5.66 }, sound: 'light' });
  doors.add({ id: 'lab', inter: 'labDoor', name: 'lab door', proxy: yard.objs.labDoor, set: (k) => { yard.labDoorHinge.rotation.y = -k * Math.PI / 2; },
    zones: [yard.zone.labDoor], leaf: { minX: 12.8, maxX: 13.75, minZ: -0.78, maxZ: -0.68 } });
  doors.add({ id: 'exit', inter: 'exitDoor', name: 'the fire exit', proxy: annex.objs.exitDoor, set: (k) => annex.setExitDoor(k),
    zones: [annex.zone.exitDoor], leaf: annex.exitLeafCol });
  doors.add({ id: 'records', inter: 'recordsDoor', name: 'records room door', proxy: annex.objs.recordsDoor, set: (k) => annex.setRecordsDoor(k),
    zones: [annex.zone.recordsDoor], leaf: annex.recordsLeafCol, sound: 'light' });
  room.setDoorLock(true);
  // the antenna field through the gate in the array fence: its cabinets and pedestals (story/field.ts)
  bindField(world.field, { inter, doors, audio, toast: (text, secs) => ui.toast(text, secs), dishes: ext.dishes, here: () => world.area === 'saro' && !world.driving });
  const motelOffice = world.motel.zones.find((z) => z.id === 'office')!;
  const game = new Prologue({
    ui, audio, room, ext, player, inter, yard, fcam, colliders, annex,
    view: { restore: () => { renderer.setPixelRatio(pr); resize(); }, draw: () => draw() },
    // Every bit of progress the chapters record becomes an autosave (core/saves.ts).
    saveCase: () => requestAutosave(false),
    loadCase: () => restoring?.case ?? null,
    isTouch: () => input.touchMode,
    travel: {
      area: () => world.area, site: () => world.site, driving: () => world.driving,
      driveOut: () => world.driveOut(), driveBack: () => world.driveBack(),
      truckChoice: (place) => world.truckChoice(place),
    },
    motel: {
      area: () => world.area, room: () => world.room6, court: () => world.court,
      goIn: () => world.goRoom6(), goOut: () => world.leaveRoom6(), brick: () => world.crossing.brick,
      driveTo: () => world.driveToMotel(() => {}),
      driven: () => world.tripSecs,
      cb: (lines, onEnd) => cb.call(lines, null, (l, duration) => { if (mode === 'play' && !game.cinematic) ui.toast(`CB, ${l.by}: "${l.text}"`, duration ?? Math.min(8, 2.4 + l.text.length / 22), () => audio.ctx!.currentTime); }, onEnd) > 0,
    },
    allNight: {
      area: () => world.area, diner: () => world.diner, dinerTruck: () => world.dinerTruckProxy,
      driveToDiner: (onArrive) => world.driveToDiner(onArrive),
      driving: () => world.driving,
      truckChoice: (place) => world.truckChoice(place),
      truckAt: () => world.truckAt,
    },
    roswell: {
      world, camera,
      settings: () => ({ calmFlash, stillShots }),
      fog: (density) => { (scene.fog as THREE.FogExp2).density = density ?? (world.area === 'roswell' ? 0.00055 : 0.0021); },
      finale: () => { void finale(); },
    },
    doors,
    milestone: () => requestAutosave(true),
  });
  doors.onChange = (id, open) => { game.doorChanged(id, open); if (mode === 'play' && !restoringNow) requestAutosave(false); };
  // a new safe point (02:13, a new chapter) starts a new autosave generation
  game.onCheckpoint = () => requestAutosave(true);
  const ch1 = game.ch1;
  const ch2 = game.ch2;
  const ch3 = game.ch3;
  ch3.bindSaroTruck(yard.objs.truck);
  world.onStationLoaded = (site) => {
    ch3.bindSite(site);
    // the walk gap before chapter three: chained (Station01.setWalkGate, World.setStoryPlaces)
    if (site.proxies.walkGate) inter.add({ id: 's1walkGate', object: site.proxies.walkGate, range: 2.6,
      label: () => world.area === 'station01' && !game.at('ch3') ? 'Walk gate' : null,
      use: () => ui.toast('Chained, with a padlock. N. Vega holds the keys out here.', 3.4),
      examine: () => world.area === 'station01' && !game.at('ch3') ? 'A chain through the walk gap, and a padlock a good deal newer than the fence.' : null });
  };
  world.onArriveStation = () => ch3.arrivedStation();
  world.onArriveSaro = () => ch3.arrivedSaro();
  // the flashlight on the passenger seat comes along the first time the player gets in
  world.onGetIn = () => {
    if (flashlight.have) return;
    flashlight.have = true;
    // which button switches it, the first time on this device
    ui.toast('You take the flashlight off the passenger seat.' + (!firstTime('flashlight') ? '' : input.touchMode ? ' Light switches it on.' : ' F switches it on.'), 4.5);
  };
  // the truck any time (Tom 15 October): the night's own drive if a chapter has one from here,
  // otherwise one of the player's own; not while the shift's first hours are still running
  world.truckChoice = (place) => {
    if (!game.at('ch1')) return { label: 'Service truck', use: () => ui.toast('SARO 07, the service truck. Not now: someone has to sit the console tonight.', 3.4) };
    return ch3.truckTrip(place) ?? game.ch4.truckTrip(world.truckAt) ?? game.ch5.truckTrip(place) ?? { label: 'Drive', use: () => world.roam(place) };
  };
  const ch4 = game.ch4;
  world.onRoom6Loaded = (r) => ch4.bindRoom(r);
  world.onEnterRoom6 = () => ch4.enteredRoom();
  world.onLeaveRoom6 = () => ch4.leftRoom();
  const ch5 = game.ch5;
  world.onDinerLoaded = (d) => ch5.bindDiner(d);
  // Caliche, down the street from the diner (world/Town.ts): its windows and signs (story/caliche.ts)
  const townSeen = new Set<string>();
  const fileSideDoc = (doc: import('./ui/UI').DocSpec) => {
    const i = game.docs.findIndex((x) => x.id === doc.id);
    if (i < 0) game.docs.push(doc); else game.docs[i] = doc;
  };
  // what happens in the town once in a night (story/townEvents.ts), kept with the rest of it
  const townEvents = new TownEvents({ audio, toast: (text, secs) => ui.toast(text, secs), note: (text) => game.note(text), seen: townSeen,
    save: () => { if (mode === 'play') requestAutosave(false); } });
  world.onTownLoaded = (t) => {
    bindTown(t, { inter, ui, toast: (text, secs) => ui.toast(text, secs), here: () => world.area === 'diner',
      secs: () => game.clock, clock: () => clockText(game.clock, false), note: (text) => game.note(text), seen: townSeen, file: fileSideDoc,
      save: () => { if (mode === 'play') requestAutosave(false); } });
    townEvents.bind(t);
  };
  // HENDELSER.md, event 7: three lights keeping pace with the truck south of SARO (story/lineLights.ts)
  const lineLights = new LineLights(scene, (text) => game.note(text), (clock) => {
    townSeen.add('ev:lights'); townSeen.add(`mark:lights@${Math.round(clock)}`); requestAutosave(false);
  });
  // the time away (story/abduction.ts): a frame of the scene from a camera of its own, drawn at once
  const shotCanvas = document.createElement('canvas'); shotCanvas.width = 640; shotCanvas.height = 400;
  let probeRT: THREE.WebGLRenderTarget | null = null;
  const PROBE_SKY = new THREE.Color(1, 0, 1);
  abduction = new Abduction({
    audio, player, scene, overlay: document.body, flashlight,
    shoot: (cam) => {
      try {
        renderer.setRenderTarget(null); renderer.render(scene, cam);
        const src = renderer.domElement, g = shotCanvas.getContext('2d')!, k = Math.min(src.width / 640, src.height / 400);
        g.drawImage(src, (src.width - 640 * k) / 2, (src.height - 400 * k) / 2, 640 * k, 400 * k, 0, 0, 640, 400);
        return shotCanvas;
      } catch { return null; }
    },
    // what would be behind them on waking (story/abduction.ts): small, before the tape, with the
    // antennas and the lamps' discs as this camera has them, and the open sky in one flat colour
    // (PROBE_SKY) so it can be told from anything standing against it
    probe: (cam, w, h) => {
      try {
        if (!probeRT || probeRT.width !== w || probeRT.height !== h) { probeRT?.dispose(); probeRT = new THREE.WebGLRenderTarget(w, h); }
        const gs = glowScale.value, bg = scene.background, skyOn = sky.group.visible;
        glowScale.value = h / (2 * Math.tan(cam.fov * Math.PI / 360));
        sky.group.visible = false; scene.background = PROBE_SKY;
        ext.dishArray.cull(cam);
        try { renderer.setRenderTarget(probeRT); renderer.render(scene, cam); }
        finally { renderer.setRenderTarget(null); sky.group.visible = skyOn; scene.background = bg; glowScale.value = gs; }
        const px = new Uint8Array(w * h * 4);
        renderer.readRenderTargetPixels(probeRT, 0, 0, w, h, px);
        return px;
      } catch { return null; }
    },
    ground: (x, z) => player.floor ? player.floor(x, z) : 0,
    standable: (x, z) => player.walkable(x, z) && !player.colliders.some((c) => x > c.minX - .4 && x < c.maxX + .4 && z > c.minZ - .4 && z < c.maxZ + .4),
    // on the highway south of SARO: in the grass on the west verge, not on the road
    settle: (q) => { if (world.area === 'saro' && world.crossing.onHighway(q)) q.x = -30.6; },
    hush: (secs, level, tau, back) => audio.night?.hush(secs, level, tau, back), glitch: (k) => { vhs.glitch = k; },
    skipClock: (sec) => { game.clock += sec; }, note: (text) => game.note(text), toast: (text, secs) => ui.toast(text, secs),
    camera: () => fcam.have,
    file: (doc) => { const i = game.docs.findIndex((x) => x.id === doc.id); if (i < 0) game.docs.push(doc); else game.docs[i] = doc; },
    keyStall: () => { world.keyStall = true; }, save: () => { if (mode === 'play') requestAutosave(false); },
  });
  // the dishes come down out of 026 and follow the player through a warning in the antenna field,
  // once in the night (story/dishesFollow.ts; FORSLAG.md, 20 October, Valg for Tom 2), kept with
  // the time away
  const follow = new DishesFollow({ dishes: ext.dishes, array: ext.dishArray, audio,
    store: { get: () => abduction!.data.dishes, set: (v) => { abduction!.data.dishes = v; } },
    ground: (x, z) => world.grounds.floorAt(x, z),
    toast: (text, secs) => ui.toast(text, secs), note: (text) => game.note(text), log: (text, at) => game.consoleLog(text, at),
    save: () => { if (mode === 'play') requestAutosave(false); }, face: (yaw) => { player.yaw = yaw; } });
  const followEye = new THREE.Vector3(), followView = new THREE.Vector3();
  // HENDELSER.md, events 2 and 10, on B: the herd in the pasture south of the motel, facing 026
  // (story/pasture.ts), and the television in the motel's office (story/motelTv.ts); both kept
  // with the town's part of the case
  const pasture = new Pasture({ herds: () => world.herds, seen: townSeen, note: (text) => game.note(text), toast: (text, secs) => ui.toast(text, secs),
    save: () => { if (mode === 'play') requestAutosave(false); } });
  const motelTv = new MotelTv({ motel: world.motel, audio, inter, renderer, scene, seen: townSeen, note: (text) => game.note(text),
    truck: () => world.saroTruck?.group ?? null, file: fileSideDoc, document: () => game.docs.find((d) => d.id === 'motel-tv-frame'), open: (doc) => ui.document(doc, () => {}),
    toast: (text, secs) => ui.toast(text, secs), save: () => { if (mode === 'play') requestAutosave(false); } });
  // ---------- the free camera and the album (core/album.ts; todo.md, after They Are Here) ----------
  // Once the camera is had, the shutter takes a free frame anywhere a case frame is not to be had
  // (Chapter1.shutter); 36 in a night, kept with the case, in the notebook's album.
  const subjectsHere = () => albumSubjects({ world, ext, room, yard, annex, sky, scene, camera, cutouts,
    outdoors: () => space === 'yard', night: () => sky.uniforms.uDawn.value < 0.3, clock: () => game.clock });
  const album = new Album({
    fcam, scene,
    view: { restore: () => { renderer.setPixelRatio(pr); resize(); }, draw: () => draw() },
    subjects: subjectsHere,
    place: () => placeName(), clock: () => game.clock,
    extra: () => abduction!.data.spots.filter((s) => s.doc).length,
    before: () => ext.dishArray.cull(fcam.photoCam),
    // someone on the film only: out in the night where the glimpses are (SARO, the highway, Caliche)
    ghostGround: () => {
      const g = glimpseIn(true);
      if (!g.scene || space !== 'yard' || world.driving || !game.at('ch1') || game.phase === 'ch6' || sky.uniforms.uDawn.value >= 0.3) return null;
      return world.area === 'diner' && world.town ? world.town.floorAt : g.ground;   // Caliche's own ground by the diner's lot
    },
    small: () => saves.fallback,
    skip: () => [sky.group],
    done: (_shot, first, left) => {
      audio.shutter(); ui.shutterFlash();
      if (first) ui.toast(`A free frame, out of the instant back. The print goes in the album in the notebook. ${left} left.`, 4.4);
      else if (left === 5 || left === 0) ui.toast(left ? '5 free frames left on the roll.' : 'That was the last free frame on the roll.', 3);
      requestAutosave(false);
    },
  });
  ch1.free = { left: () => album.left, expose: () => { album.expose(); } };
  // the barricades across the old road, seen from the diner's lot (World.setStoryPlaces)
  // random trouble with the truck (when the story or the developer menu turns it on): at night
  // on the player's own drives and the story's, never in a cinematic
  world.troubleOk = () => mode === 'play' && !game.cinematic && sky.uniforms.uDawn.value < 0.3;
  world.onRoadLoaded = (road) => inter.add({ id: 'roadClosed', object: road.closedSign, label: () => null, use: () => {},
    examineRange: 40,
    examine: () => road.roadClosed ? 'ROAD CLOSED. Two barricades across the old road to Roswell, an amber lamp blinking on each.' : null });
  world.onDinerTruck = (px) => ch5.bindDinerTruck(px);
  world.initSaro().catch((e) => console.warn('the service truck could not be built', e));
  // a closer look at what had nothing to say: the antennas, the signs, the papers, the motel (story/looks.ts)
  bindLooks({ inter, dishes: ext.dishes, saro: ext.group, motel: world.motel,
    motelSign: ['Sierra Motor Court', 'Sierra Motor Court (back)'].map((n) => ext.group.getObjectByName(n)).filter((o): o is THREE.Object3D => !!o) });
  // the haze on the ground only out in the open (world/airlight.ts): the title's view, the cab, outdoors
  world.airOut = () => mode !== 'play' || world.driving || space === 'yard';
  // the life in the night (world/nightlife.ts): dust in the flashlight's beam; no far car and no
  // tumbleweed behind the title, in a cinematic, or while one of the night's strange things is on
  world.lifeBeam = () => flashlight.light;
  world.lifeQuiet = () => mode !== 'play' || game.cinematic || roadside.silent || !!glimpses.up || townEvents.now !== 'none';

  sky.onThunder = (delay, s) => setTimeout(() => { if (mode === 'play') audio.thunder(s); }, delay * 1000);

  type Mode = 'title' | 'play' | 'end';
  let mode: Mode = 'title';
  ui.canDescribe = () => mode === 'play' && !game.cinematic && !audio.phonePlaying && !cb.busy
    && !npcDialogue.playing && !npcDialogue.loading && !earlDialogue.playing && !earlDialogue.loading;
  let titleEl: HTMLElement | null = null;
  let endEl: HTMLElement | null = null;
  let pausedByMenu = false;
  let devRun = false;   // a run started from the developer menu: never saved
  let score: Score | null = null;   // the music under the night (core/score.ts)
  let dev: DevMenu | null = null;   // the developer menu, with ?dev in the address
  let devWait: { ok: () => boolean; fn: () => void } | null = null;
  let releasingLock = false;

  // ---------- modal / pointer lock plumbing ----------
  ui.onModalChange = (open) => {
    if (mode !== 'play') return;
    if (open && fcam.raised) { fcam.raise(false); ui.viewfinder(false); ui.cameraButton(input.touchMode && fcam.have, false); }
    if (open) {
      input.enabled = false; input.reset();
      ui.showHud(false, input.touchMode);
      if (input.locked) { releasingLock = true; input.releaseLock(); }
    } else {
      input.enabled = true;
      ui.showHud(true, input.touchMode);
      input.requestLock();
    }
  };
  input.onLockChange = (locked) => {
    if (locked) { releasingLock = false; return; }
    if (releasingLock) { releasingLock = false; return; }
    if (mode === 'play' && !ui.modal && !game.cinematic) openPause();
  };
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden || mode !== 'play') return;
    autosaveNow(); // a phone may close the tab while it is in the background
    if (!pausedByMenu) openPause();
  });

  // the settings shared by the pause menu and the title screen; all kept on this device
  let invertY = store.get('invertY', false);
  let baseFov = store.get('fov', 70);
  const setLargeText = (on: boolean) => document.documentElement.classList.toggle('text-large', on);
  setLargeText(store.get('largeText', false));
  // the end of the night: flashes or slow fades, a moving camera or still shots (KAPITLER.md)
  let calmFlash = store.get('calmFlash', false);
  let stillShots = store.get('stillShots', (() => { try { return matchMedia('(prefers-reduced-motion: reduce)').matches; } catch { return false; } })());
  const settings = () => ({
    characters: npcDialogue.enabled, onCharacters: (v: boolean) => {
      npcDialogue.enabled = v; audio.characterVoices = v; if (!v) { audio.stopPhoneCalls(); cb.stop(); }
      store.set('characterVoices', v);
    },
    serling: narrator.enabled, onSerling: (v: boolean) => { narrator.enabled = v; store.set('serlingNarrator', v); },
    kinski: readAloud.enabled, onKinski: (v: boolean) => { readAloud.enabled = v; audio.kinskiCalls = v; if (!v) audio.stopPhoneCalls(); store.set('kinskiReadAloud', v); },
    artBell: radio.hostVoice, onArtBell: (v: boolean) => { radio.setHostVoice(v); earlDialogue.enabled = v; store.set('artBellRadioHost', v); },
    radioCast: radio.callerVoice, onRadioCast: (v: boolean) => { radio.setCallerVoice(v); store.set('radioCallerVoices', v); },
    volume: audio.volume, sens: input.sensitivity,
    onVolume: (v: number) => { audio.setVolume(v); store.set('vol', v); },
    music: audio.musicLevel, onMusic: (v: number) => { audio.setMusic(v); store.set('music', v); },
    radio: audio.radioLevel, onRadio: (v: number) => { audio.setRadio(v); store.set('radioVol', v); },
    onSens: (v: number) => { input.sensitivity = v; store.set('sens', v); },
    quality, onQuality: (q: Quality) => applyQuality(q), ultraOk,
    picture: vhs.picture, onPicture: (p: Picture) => applyPicture(p),
    invertY, onInvertY: (v: boolean) => { invertY = v; store.set('invertY', v); },
    fov: baseFov, onFov: (v: number) => { baseFov = v; store.set('fov', v); resize(); },
    largeText: document.documentElement.classList.contains('text-large'),
    onLargeText: (v: boolean) => { setLargeText(v); store.set('largeText', v); },
    calmFlash, onCalmFlash: (v: boolean) => { calmFlash = v; store.set('calmFlash', v); },
    stillShots, onStillShots: (v: boolean) => { stillShots = v; store.set('stillShots', v); },
    abduct: abductOn, onAbduct: (v: boolean) => { abductOn = v; store.set('abductions', v); },
  });

  function openPause() {
    if (pausedByMenu) return;
    readAloud.stop();
    earlDialogue.stop();
    narrator.stop(); npcDialogue.stop();
    pausedByMenu = true;
    audio.suspend(true); score?.pause(true);
    const resume = () => { pausedByMenu = false; audio.suspend(false); score?.pause(false); };
    ui.pause({
      ...settings(),
      onResume: resume,
      onTitle: () => { resume(); autosaveNow(); toTitle(); },
      // the case menus sit on top of the pause menu and go back to it
      onSave: () => saveMenu(ui, {
        mode: 'save', metas: saves.list(), activeCase: caseId, block: saveBlock(), fallback: saves.fallback,
        onSave: (slot) => { draw(); return writeSave('manual', slot); },
        onBack: () => { pausedByMenu = false; openPause(); },
      }),
      onLoad: () => saveMenu(ui, {
        mode: 'load', metas: saves.list(), activeCase: caseId, fallback: saves.fallback,
        onLoad: (m) => { resume(); openSave(m); },
        onDelete: (m) => saves.remove(m.id),
        onBack: () => { pausedByMenu = false; openPause(); },
      }),
    });
  }

  function useCurrent() {
    // in the truck: Use (E) turns the key when the engine has died and waits for it, or gets
    // out where the truck stands when it is stopped (Tom, 16 October)
    if (mode === 'play' && !ui.modal && !game.cinematic && world.driving) {
      if (world.trouble.turnKey()) return;
      if (radioAimed()) { turnRadio(); return; }
      world.getOutHere();
      return;
    }
    if (mode !== 'play' || ui.modal || game.cinematic || world.driving) return;
    if (fcam.raised) { ch1.shutter(); return; } // the camera is up: Use is the shutter
    const it = inter.update(camera);
    if (it) it.use();
  }

  input.onKey = (code) => {
    if (code === 'F2' && dev) { dev.toggle(); return; }
    if (mode !== 'play') return;
    if (code === 'KeyE' || code === 'Enter') {
      if (ui.modal) { if (ui.modal.classList.contains('docview')) ui.close(); return; }
      useCurrent();
    } else if (code === 'KeyM' || code === 'KeyI' || code === 'KeyH') {
      // the map, the kit and the controls, straight to their tabs in the notebook
      if (ui.modal) { if (ui.modal.querySelector('.notebook')) ui.close(); return; }
      if (!game.cinematic) { ui.notebookAt(code === 'KeyM' ? 'map' : code === 'KeyI' ? 'kit' : 'controls'); game.openNotebook(); }
    } else if (code === 'Tab' || code === 'KeyN') {
      if (ui.modal) { if (ui.modal.querySelector('.notebook')) ui.close(); return; }
      if (!game.cinematic) game.openNotebook();
    } else if (code === 'KeyB') {
      // the case board (story/caseBoard.ts), straight from the game or from the notebook
      if (ui.modal) { if (ui.modal.classList.contains('board-overlay')) ui.close(); else if (ui.modal.querySelector('.notebook')) game.openBoard(); return; }
      if (!game.cinematic) game.openBoard();
    } else if (code === 'Escape' || code === 'KeyP') {
      if (ui.modal) { ui.close(); return; }
      if (fcam.raised) { ch1.toggleCamera(); return; }
      openPause();
    } else if (code === 'KeyF') {
      toggleLight();
    } else if (code === 'KeyR') {
      turnRadio();
    } else if (code === 'KeyV') {
      if (!game.cinematic) ui.readLastDescription();
    } else if (code === 'KeyC') {
      if (!ui.modal && !game.cinematic && !world.driving) ch1.toggleCamera();
    } else if (code === 'Space') {
      if (fcam.raised && !ui.modal) ch1.shutter();
    }
  };
  renderer.domElement.addEventListener('click', () => {
    if (mode !== 'play' || ui.modal || input.touchMode) return;
    if (!input.locked) input.requestLock(); else useCurrent();
  });
  input.onTap = (x, y) => {
    if (mode !== 'play' || ui.modal || game.cinematic) return;
    const ndc = new THREE.Vector2((x / innerWidth) * 2 - 1, -(y / innerHeight) * 2 + 1);
    if (world.driving) { if (radioAimed(ndc)) turnRadio(); return; }   // a tap on the radio in the cab
    if (fcam.raised) { ch1.shutter(); return; }
    const it = inter.pick(camera, ndc, 2.6);
    if (it) it.use();
  };
  ui.onUse = useCurrent;
  ui.onNotes = () => { if (mode === 'play' && !ui.modal && !game.cinematic) game.openNotebook(); };
  ui.onCamera = () => { if (mode === 'play' && !ui.modal && !game.cinematic && !world.driving) ch1.toggleCamera(); };
  const toggleLight = () => {
    if (mode !== 'play' || ui.modal || game.cinematic || world.driving) return;
    if (!flashlight.toggle()) ui.toast('No flashlight. There is one on the passenger seat of the truck.', 3);
  };
  ui.onLight = toggleLight;
  // the truck's radio: on, then the next station along the dial, and after the last, off
  const turnRadio = () => {
    if (mode !== 'play' || ui.modal || game.cinematic || !world.driving) return;
    if (!world.truckHere()) { ui.toast('The radio stays on the Roswell station.', 2.5); return; }
    const st = carRadio.next();
    audio.play('click', { gain: 0.5, rate: 0.8 });
    ui.toast(st ? `AM ${FREQ[st]}` : 'Radio off', 1.6);
  };
  ui.onRadio = turnRadio;
  // in the cab the radio is used by looking at it and clicking (or E), or by tapping it, as well
  // as with R (Tom, 16 October): a ray from the view's centre, or from the tap, to its faceplate
  const aimRay = new THREE.Raycaster(), centre = new THREE.Vector2();
  aimRay.far = 1.6;
  function radioAimed(ndc = centre) {
    const tr = world.driving ? world.truckHere() : null;
    if (!tr?.radioHit) return false;
    aimRay.setFromCamera(ndc, camera);
    return aimRay.intersectObject(tr.radioHit, false).length > 0;
  }
  const radioLabel = () => !carRadio.on ? 'Turn the radio on' : carRadio.station === DIAL[DIAL.length - 1] ? 'Turn the radio off' : `AM ${FREQ[carRadio.station]}: next station`;
  ui.onZoom = (held) => { input.zoomHeld = held; };
  ui.onPause = () => { if (mode === 'play' && !ui.modal) openPause(); };

  // ---------- flow ----------
  // Music on the title screen ("Signal to Noise", the end's music too). A browser lets sound
  // start only from a click or a key, so it comes in with the first one on the title screen;
  // back from a night, the sound is already allowed and it starts at once.
  function menuMusic() {
    if (mode !== 'title') return;
    audio.unlock().then(() => { if (mode === 'title') audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.6 }); }).catch(() => {});
  }
  // The first time, the menu waits behind "Tap to begin", so that first tap brings in the
  // music before anything is chosen (Tom, 18 October: music when the main menu comes up).
  // Not for the browser tests, which click their way straight in.
  const firstTouch = (e: Event) => {
    removeEventListener('pointerdown', firstTouch, true); removeEventListener('keydown', firstTouch, true);
    const el = titleEl;
    if (el?.classList.contains('gated')) {
      e.preventDefault(); e.stopPropagation();
      // the menu comes in after the tap is over, so the tap itself does not land on a button
      if (e.type === 'pointerdown') addEventListener('pointerup', () => setTimeout(() => ui.ungate(el), 0), { once: true, capture: true });
      else ui.ungate(el);
    }
    menuMusic();
  };
  function showTitle() {
    const gate = !audio.ctx && !navigator.webdriver;
    if (audio.ctx) menuMusic(); else { addEventListener('pointerdown', firstTouch, true); addEventListener('keydown', firstTouch, true); }
    const cont = saves.continueSave();
    titleEl = ui.title({
      gate, touch: matchMedia('(pointer: coarse)').matches,
      cont: cont ? `${cont.chapter} · ${cont.place} · ${cont.clock} · ${played(cont.playtime)} played` : null,
      onContinue: () => { const m = saves.continueSave(); if (m) openSave(m); },
      onStart: () => {
        const free = saves.freeCase();
        if (free) { startGame({ caseId: free, playtime: 0, state: null }); return; }
        // every case slot is taken: pick one to replace
        saveMenu(ui, { mode: 'new', metas: saves.list(), activeCase: saves.active, onNewCase: (c) => {
          const unlocking = audio.unlock(); // the click is the gesture that may start sound
          saves.removeCase(c).then(() => startGame({ caseId: c, playtime: 0, state: null, unlocking }));
        } });
      },
      canLoad: saves.list().length > 0,
      onLoad: () => saveMenu(ui, { mode: 'load', metas: saves.list(), activeCase: saves.active, fallback: saves.fallback,
        onLoad: (m) => openSave(m), onDelete: (m) => saves.remove(m.id),
        onBack: () => { if (mode === 'title') { titleEl?.remove(); showTitle(); } } }),
      onSettings: () => ui.pause({ ...settings(), title: 'Settings', settingsOnly: true, onResume: () => {}, onTitle: () => {} }),
      // the warning before the night: flashing images near the end, or slow fades instead
      flash: { calm: calmFlash, onPick: (calm) => { calmFlash = calm; store.set('calmFlash', calm); } },
    });
  }

  // Open a save: from the title, the pause menu or Continue. Sound is unlocked first,
  // inside the click, then the save is read.
  function openSave(m: SaveMeta) {
    const unlocking = audio.unlock();
    if (mode === 'play') leavePlay();
    saves.load(m.id).then((state) => {
      if (!state) { ui.toast('That save could not be read. Your other saves are unchanged.', 5); if (mode !== 'play') showTitleAgain(); return; }
      startGame({ caseId: m.caseId, playtime: m.playtime, state, unlocking });
    });
  }
  function showTitleAgain() { titleEl?.remove(); showTitle(); }

  let restoringNow = false;
  let starting = false;   // between Start or Continue and the first playable frame
  async function startGame(o: { caseId: number; playtime: number; state: GameState | null; jump?: string; unlocking?: Promise<void>; dev?: boolean }) {
    starting = true;
    devRun = !!o.dev;
    const unlocking = o.unlocking ?? audio.unlock(); // must start inside the tap/click
    if (input.touchMode) {
      try { const r = document.documentElement.requestFullscreen?.(); if (r) r.catch(() => {}); } catch { /* not allowed here */ }
    }
    titleEl?.remove(); titleEl = null;
    endEl?.remove(); endEl = null;
    ui.close(true);
    ui.fade(true, '');
    mode = 'play';
    input.enabled = true;
    input.requestLock();
    // Normally the sounds are decoded long before anyone taps Start. If not, say so.
    const slow = setTimeout(() => ui.fade(true, 'TUNING RECEIVERS', true), 350);
    await unlocking;
    audio.stop('music', 1.2);   // the title screen's music
    await saves.ready;
    // a save made at the station or in room 6 needs that area built first
    const savedArea = o.state?.area;
    const area = (savedArea === 'station01' || savedArea === 'room6' || savedArea === 'diner' ? savedArea : 'saro') as AreaId;
    try { await world.prepare(area); } catch (e) { console.error(e); }
    clearTimeout(slow);
    caseId = o.caseId; playtime = o.playtime; if (caseId) saves.setActive(caseId);
    audio.startRoomTone();
    audio.startNight();
    if (!score && audio.ctx) score = new Score(audio.ctx, audio.music);
    space = 'room';
    // the truck where the story left it: outside the station's gate, on the diner's lot, on the
    // motel's lot (a save made with it there) or on SARO's pad
    world.placeTruck(area === 'station01' ? 'station01' : area === 'diner' ? 'diner' : o.state?.truck === 'motel' ? 'motel' : 'saro', o.state?.truckPose);
    world.enter('saro');
    restoring = o.state; restoringNow = true;
    flashlight.have = !!o.state?.flashlight; flashlight.on = false;
    townSeen.clear(); for (const id of o.state?.town ?? []) townSeen.add(id);
    townEvents.reset(); glimpses.reset(); cutouts.reset(); nightEvents.reset(); abduction!.restore(o.state?.abduct); lineLights.reset(townSeen.has('ev:lights'));
    pasture.reset(); motelTv.reset();
    album.restore(o.state?.album);   // the free frames of the night that was saved, or none
    game.start(o.state?.checkpoint ?? o.jump);
    if (o.state?.doors) doors.restore(o.state.doors); // the doors as they were left
    abduction!.refile();
    follow.restore();   // after the log is back (game.start): a save made while the dishes were out
    for (const doc of townWitnessDocs(townSeen)) fileSideDoc(doc);
    motelTv.refile(o.state?.motelTv?.url);
    restoringNow = false; restoring = null;
    roadside.reset(); cb.stop(); audio.stopPhoneCalls(); radio.boyCalled = townSeen.has('ev:boy'); radio.reset(game.clock);   // the radios off; each station where it is at this hour
    world.enter(area === 'station01' && world.site ? 'station01' : area === 'room6' && world.room6 ? 'room6' : area === 'diner' && world.diner ? 'diner' : 'saro');
    if (world.area === 'station01') world.placeAtStation();
    if (world.area === 'room6') world.placeAtRoom6();
    if (world.area === 'diner') world.placeAtDiner('inside');
    // put the player back where they stood, if the restored world lets them stand there
    const pose = o.state?.pose;
    if (pose && o.state?.checkpoint !== 'residual' && player.walkable(pose.x, pose.z)) {
      player.place(pose.x, pose.z, pose.yaw); player.pitch = pose.pitch;
    }
    starting = false;
    ui.showHud(true, input.touchMode);
    setTimeout(() => ui.fade(false), 250);
  }

  // Stop the night that is running, without the title screen (before opening another save).
  function leavePlay() {
    ui.fade(true, '');
    fcam.raise(false); ui.viewfinder(false);
    ui.close(true);
    endEl?.remove(); endEl = null;
    score?.stop();
    if (audio.ctx) { audio.stop('music', 0.5); audio.stopMotors(); audio.setCarrier(0, 0, 0); audio.signalOff(); audio.tvHiss(null); }
    motelTv.silence();
    follow.reset();
    world.stopDriving();
    roadside.reset(); townEvents.reset(); nightEvents.reset(); cb.stop(); audio.stopPhoneCalls(); radio.reset(0);
    game.reset();
    world.enter('saro');
    pendingAuto = false;
  }

  // ---------- saving ----------
  let pendingAuto = false;
  function saveBlock(): string | null {
    if (devRun) return 'This is a developer run. Nothing is saved.';
    if (mode !== 'play' || !caseId) return 'There is no night running to save.';
    return game.saveBlock();
  }
  // ---------- the notebook's map and kit (Tom, 16 October) ----------
  // Where the player is on the pencil map (ui/areaMap.ts): the area, and on a road how far along.
  function mapAt(): MapView['at'] {
    const p = player.pos, tr = world.driving ? world.truckHere() : null, q = tr ? tr.group.position : p;
    switch (world.area) {
      case 'road': return { spot: 'highway', m: q.z - ROAD_ORIGIN.z - SARO_IN_ROAD.z };
      case 'diner': return world.roadStopAt(p) ? { spot: 'highway', m: p.z - ROAD_ORIGIN.z - SARO_IN_ROAD.z } : { spot: 'diner' };
      case 'room6': return { spot: 'motel' };
      case 'roswell': { const d = world.oldDrive; return { spot: 'oldroad', mi: world.oldRoad && d ? world.oldRoad.where(d.pos.x, d.pos.z).mi : 0 }; }
      case 'station01': {
        if (!world.driving) return { spot: 'station01' };
        const d = Math.hypot(q.x - STATION_AT.x, q.z - STATION_AT.z);
        return d < 60 ? { spot: 'station01' } : { spot: 'track', t: 1 - Math.min(1, d / 450) };
      }
      default:
        if (inside(motelOffice, q) || (world.crossing.outside(q) && world.crossing.atMotel(q))) return { spot: 'motel' };
        // on the highway at SARO, or down it (on foot or in the truck), metres south of SARO
        if (world.crossing.outside(q) || q.z > 64) return { spot: 'highway', m: Math.max(0, q.z) };
        if (world.grounds.onGround(p) || world.grounds.onRoad(p) || inside(yard.zone.truckPad, p)) return { spot: 'yard' };
        return { spot: 'saro' };
    }
  }
  function kitItems(): KitItem[] {
    const photos = game.docs.filter((d) => d.kind === 'photo' && d.image).length, papers = game.docs.length - photos;
    const lot = world.truckOnLot();   // (or left on the highway, among the masts, in the yard: where I left it)
    const truckAt = world.truckAt === 'station01' ? 'Outside the gate at STATION 01' : world.truckAt === 'diner' && lot ? 'On the lot at Mesa Diner'
      : world.truckAt === 'motel' ? 'On the lot at Sierra Motor Court' : world.truckAt === 'saro' && (lot || (!!world.saroTruck && inside(yard.zone.truckPad, world.saroTruck.group.position))) ? 'On the pad by the service yard' : 'Where I left it';
    const kit: KitItem[] = [{ name: 'Notebook', art: 'notebook', note: 'Tab opens it. Tasks, notes, findings, papers, photographs, this map.' }];
    if (game.at('ch1')) kit.push({ name: 'SARO jacket', art: 'jacket', state: 'On', note: 'From the hook by the east door. The reflective band across the back.' });
    const frames = game.docs.filter((d) => /^(frame|away)/.test(d.id)).length;
    if (fcam.have) kit.push({ name: 'Field camera', art: 'camera', state: `${frames} case frames, ${rollText(album.left)}`, note: 'The SARO field camera. C raises it, Space takes the picture: the case frames on sheet film, anything else out of the instant back, into the album.' });
    if (flashlight.have) kit.push({ name: 'Flashlight', art: 'flashlight', state: flashlight.on ? 'On' : 'Off', note: 'From the passenger seat of the truck. F switches it.' });
    if (game.at('ch1')) kit.push({ name: 'Truck keys', art: 'keys', tag: 'SARO 07', state: truckAt, note: 'SARO 07, the service truck. The keys stay in it.' });
    if (papers) kit.push({ name: 'Papers', art: 'papers', state: String(papers), note: 'Folded into the notebook. Papers and Findings have them.' });
    if (townSeen.has('nickel')) kit.push({ name: 'A nickel', art: 'nickel', state: '1947', note: 'From the coin return of the payphone in Caliche. Bright as the day it was struck.' });
    return kit;
  }
  // the people in the case, as the night meets them, with their faces once ChatGPT's portraits
  // are in (ART_BRIEF.md, round 13; core/portraits.ts)
  function peopleItems(): PersonCard[] {
    const p: PersonCard[] = [{ name: 'Dale', role: 'SARO, the shift before', note: 'Left the shift log on the desk and went home. "Don\'t break anything. D."', portrait: portrait('dale') }];
    if (game.at('ch2')) p.push({ name: 'Dr. Evelyn Ward', role: 'SARO, duty officer', note: 'On the line from the main site. Wants the run checked and logged by the book.', portrait: portrait('ward') });
    if (ch2.s.called || game.at('ch3')) p.push({ name: 'N. Vega', role: 'Sierra Motor Court, room 6', note: 'Key holder for STATION 01. Signed both versions of the 1947 record.', portrait: portrait('nora') });
    if (ch4.s.topics.tomas) p.push({ name: 'Tomás Vega', role: 'Survey crew, 1947', note: 'Her brother. Walked out to the closing point with the last reading.', portrait: portrait('tomas_1947') });
    if (ch5.s.arrived) p.push(
      { name: 'The waitress', role: 'Mesa Diner, all night', note: 'Has watched the lights over the mesa since she was eleven.', portrait: portrait('waitress') },
      { name: 'The driver', role: 'Mesa Diner, at the counter', note: 'His engine died on the old road last October.', portrait: portrait('driver') });
    p.push(...townPeople(townSeen));
    return p;
  }
  game.notebookExtras = () => ({
    map: areaMap({ at: mapAt(), station: game.at('ch3'), oldRoad: game.at('ch5'), marks: mapMarks() }),
    mapDescription: areaMapDescription({ at: mapAt(), station: game.at('ch3'), oldRoad: game.at('ch5'), marks: mapMarks() }),
    kit: kitItems(),
    people: peopleItems(),
    touch: input.touchMode,
    album: album.view(game.clock),
  });

  // where the lines run, for the time away (story/abduction.ts; HENDELSER.md, section 3)
  const FIXED: Record<string, () => number> = { ch1: () => game.eventClock + 40 * 60, ch2: () => 3 * 3600 + 40 * 60, ch3: () => 4 * 3600 + 35 * 60, ch4: () => 5 * 3600 };
  const abAlong = new THREE.Vector3();
  function townAlone(x: number, z: number) {
    const t = world.town!, lx = x - ROAD_ORIGIN.x, lz = z - ROAD_ORIGIN.z;
    if (lz < 2085) return false;   // the diner, the gas station's window
    if (t.womanIn && Math.hypot(lx + 14.4, lz - 2132.7) < 22) return false;
    if (t.smokerIn && Math.hypot(lx - 24.1, lz - 2159.4) < 22) return false;
    return true;
  }
  function abductIn(ok: boolean): AbductIn {
    const p = player.pos;
    let line: AbductIn['line'] = null;
    if (!world.driving) {
      if (world.area === 'saro' && (world.grounds.onGround(p) || world.grounds.onRoad(p) || world.crossing.outside(p)) && !inside(motelOffice, p)) {
        line = 'B';
        if (world.crossing.onHighway(p)) abAlong.set(0, 0, 1); else abAlong.set(-0.92, 0, -0.38);
      } else if (world.area === 'diner' && world.town?.inTown(p.x, p.z) && !world.indoors(p) && townAlone(p.x, p.z)) { line = 'A'; abAlong.set(0, 0, 1); }
      else if (world.area === 'station01' && !world.indoors(p)) { line = 'C'; abAlong.set(1, 0, 0); }
    }
    return { ok, enabled: abductOn, line, along: abAlong, clock: game.clock, nextFixed: FIXED[game.phase]?.() ?? -1e9,
      late: game.phase === 'ch3' || game.phase === 'ch4', calm: calmFlash,
      // out among the dishes before they have turned: a little sooner there (story/dishesFollow.ts)
      boost: line === 'B' && !abduction?.data.dishes && nearArray(p.x, p.z) ? 1.5 : 1 };
  }
  // the marks on the notebook's map: the town's events on A (the cutout and the deputy's car on the
  // highway north of it), the lights on B, and the times away where they happened
  function mapMarks(): NonNullable<MapView['marks']> {
    const hwy = (roadZ: number): MapView['at'] => ({ spot: 'highway', m: roadZ - SARO_IN_ROAD.z });
    const out: NonNullable<MapView['marks']> = townMarks(townSeen).map((m) => ({ ...m, line: ON_B.has(m.kind) ? 'B' as const : 'A' as const,
      at: m.kind === 'cutout' ? hwy(1450) : m.kind === 'car' ? hwy(1690) : m.kind === 'lights' ? { spot: 'highway', m: 330 }
        : m.kind === 'cows' ? { spot: 'highway', m: PASTURE_M } : m.kind === 'tv' ? { spot: 'office' } : undefined }));
    for (const sp of abduction!.data.spots) out.push({ kind: 'away', clock: sp.clock,
      line: sp.scene === 'saro' ? 'B' : sp.scene === 'road' ? 'A' : 'C',
      at: sp.scene === 'saro' ? (sp.take[2] > 64 ? { spot: 'highway', m: sp.take[2] } : { spot: 'yard' }) : sp.scene === 'station01' ? { spot: 'station01' } : undefined });
    return out.sort((a, b) => a.clock - b.clock);
  }
  function glimpseIn(ok: boolean): GlimpsesIn {
    const road = world.area === 'road' || world.area === 'diner', tr = world.driving ? world.truckHere() : null;
    return {
      ok, scene: world.area === 'saro' ? 'saro' : road ? 'road' : null, cam: camera, clock: game.clock,
      ground: road ? (x, z) => surfaceY(x - ROAD_ORIGIN.x, z - ROAD_ORIGIN.z) + ROAD_ORIGIN.y : (x, z) => world.grounds.floorAt(x, z),
      truck: tr && road ? { pos: tr.group.position, dir: camera.getWorldDirection(new THREE.Vector3()) } : null,
      flashlight: flashlight.on, zoom: zoomK,
    };
  }
  // ---------- more that can happen in the night, anywhere (story/nightEvents.ts) ----------
  let silencePlace: string | null = null;
  const lastFoot = new THREE.Vector3();
  const objAt = (o: THREE.Object3D) => o.getWorldPosition(new THREE.Vector3());
  const nightEvents = new NightEvents({
    audio, note: (text) => { game.note(text); requestAutosave(false); }, seen: townSeen,
    step: () => audio.stepPick(surfaceAt()),
    setRadio: (st) => carRadio.set(true, st),
    bulletin: () => radio.stations.caliche.cutIn(BULLETIN, game.clock),
    bulletinProgress: () => radio.stations.caliche.archiveStatus,
    bulletinHeard: () => world.driving && carRadio.live && carRadio.station === 'caliche' && carRadio.power > 0.5 && game.clock % 86400 < SIGN_OFF,
    openDoor: () => doors.set('east', true),
    silence: () => { silencePlace = placeName(); roadside.silenceNow({ hush: (secs, level) => audio.night?.hush(secs, level), flicker: (k) => flashlight.flicker(k, 'silence') }); },
    place: () => placeName(), silent: () => roadside.silent,
  });
  function nightIn(ok: boolean): NightIn {
    const tr = world.truckHere(), out = world.area === 'saro' ? space === 'yard' : !world.indoors(player.pos);
    const moving = player.pos.distanceTo(lastFoot) > 0.004; lastFoot.copy(player.pos);
    const foot = world.driving ? null : { pos: player.pos, outside: out, moving };
    return {
      ok, clock: game.clock, cam: camera, foot, driving: world.driving,
      truck: !world.driving && tr ? { pos: tr.group.position, radioOn: carRadio.on } : null,
      radio1420: world.driving && carRadio.live && carRadio.station === 'caliche' && radio.stations.caliche.free,
      saro: world.area === 'saro' && out && !world.driving ? { phone: objAt(room.objs.phone), door: objAt(room.objs.doorEast), doorOpen: doors.isOpen('east') } : null,
      silenceFree: !roadside.silent && !game.notes.some((n) => n.includes('No sound for 47 s')),
    };
  }
  function cutoutIn(ok: boolean): CutoutsIn {
    const g = glimpseIn(ok), tr = world.driving ? world.truckHere() : null;
    const ground = world.area === 'saro' || world.area === 'road' || world.area === 'diner';
    const r = 0.45, blocked = (x: number, z: number) => player.colliders.some((c) => x > c.minX - r && x < c.maxX + r && z > c.minZ - r && z < c.maxZ + r);
    return {
      ok, cam: camera, clock: game.clock,
      foot: world.driving ? null : { pos: player.pos, walkable: (x, z) => player.walkable(x, z), floor: (x, z) => (player.floor ? player.floor(x, z) : 0), blocked },
      drive: tr && ground ? { pos: tr.group.position, dir: camera.getWorldDirection(new THREE.Vector3()), ground: g.ground } : null,
    };
  }
  function placeName() {
    const p = player.pos;
    if (world.area === 'station01') return world.indoors(p) ? 'STATION 01, field hut' : 'STATION 01';
    if (world.area === 'road') return 'Highway south';
    if (world.area === 'room6') return world.indoors(p) ? 'Sierra Motor Court, room 6' : 'Sierra Motor Court';
    if (world.area === 'diner') return world.indoors(p) ? 'Mesa Diner' : world.town?.inTown(p.x, p.z) ? world.town.placeAt(p.x, p.z) : world.roadStopAt(p)?.name ?? 'Mesa Diner, the lot';
    if (world.area === 'roswell') return 'The old Roswell road';
    if (inside(motelOffice, p)) return 'Sierra Motor Court, office';
    if (world.grounds.onRoad(p)) return 'Service road';
    if (world.crossing.outside(p) && p.z > -5) return world.crossing.atMotel(p) ? 'Sierra Motor Court' : world.crossing.onHighway(p) ? 'The highway south' : 'West lot';
    if (world.field.inField(p)) return world.field.placeAt(p);
    if (world.grounds.onGround(p)) return 'SARO grounds';
    if (game.checkpointName() === 'residual') return 'Control room';
    if (inside(annex.zone.records, p) || inside(annex.zone.recordsDoor, p)) return 'Records room';
    if (inside(annex.zone.corridor, p) || inside(annex.zone.southDoor, p)) return 'South corridor';
    if (inside(room.bounds, p)) return 'Control room';
    if (inside(yard.zone.lab, p)) return 'Photo lab';
    if (inside(yard.zone.truckPad, p)) return 'Truck pad';
    return 'Service yard';
  }
  // A small picture of what the player sees, taken right after a frame is drawn
  // (the canvas does not keep its picture between frames).
  const thumbCanvas = document.createElement('canvas');
  thumbCanvas.width = 192; thumbCanvas.height = 108;
  function captureThumb(): string | null {
    try {
      const src = renderer.domElement, g = thumbCanvas.getContext('2d')!;
      const k = Math.max(192 / src.width, 108 / src.height);
      const w = 192 / k, h = 108 / k;
      g.drawImage(src, (src.width - w) / 2, (src.height - h) / 2, w, h, 0, 0, 192, 108);
      return thumbCanvas.toDataURL('image/jpeg', 0.72);
    } catch { return null; }
  }
  function writeSave(kind: SaveKind, slot: number | 'rotate'): Promise<SaveMeta> {
    const checkpoint = game.checkpointName();
    const prologue = checkpoint === 'residual';
    // the truck exactly where it was left; a save made on a drive finds it, and the player, where
    // the drive began (World.truckSave)
    const tk = world.truckSave();
    const tvFrame = game.docs.find((d) => d.id === 'motel-tv-frame')?.image;
    const state: GameState = {
      v: 2, checkpoint, case: game.snapshotCase(), area: world.driving && tk ? tk.area : world.area, doors: doors.states(), flashlight: flashlight.have,
      truck: tk?.at ?? world.truckAt, truckPose: tk?.pose ?? null, town: [...townSeen], abduct: abduction!.data,
      album: album.data,
      motelTv: tvFrame ? { id: 'motel-tv-frame', url: tvFrame } : undefined,
      // on your back after the time away, Continue stands you up looking ahead
      pose: prologue ? null : { x: player.pos.x, z: player.pos.z, yaw: player.yaw, pitch: abduction!.holds ? -0.06 : player.pitch },
    };
    // the picture without what stands over you on waking (story/abduction.ts)
    const over = abduction!.hideOver(true);
    if (over) draw();
    const thumb = captureThumb();
    if (over) { abduction!.hideOver(false); draw(); }
    return saves.save(caseId, kind, slot, state, {
      chapter: game.chapterTitle(), place: placeName(),
      clock: prologue ? '02:13' : clockText(game.clock, false), playtime, thumb,
    });
  }
  // Progress asks for an autosave; it is written after the next frame, once the game may be saved.
  function requestAutosave(milestone: boolean) {
    if (!caseId || mode !== 'play' || restoringNow) return;
    if (milestone) saves.newGeneration(caseId);
    pendingAuto = true;
  }
  function flushAutosave() {
    if (!pendingAuto || saveBlock()) return;
    pendingAuto = false;
    writeSave('auto', 'rotate').catch((e: Error) => ui.toast(`Autosave failed: ${e.message}`, 5));
  }
  // Right now, before leaving or when the tab goes to the background.
  function autosaveNow() {
    if (saveBlock()) return;
    draw();
    pendingAuto = true;
    flushAutosave();
  }
  // A page opened before a new version went out asks for pieces of the old one (the road, the
  // truck's cab) that are no longer on the server (Tom, 18 October: "The way could not be
  // loaded"). Save what can be saved and load the new version; Continue takes up the night.
  // Not twice in a minute: then it is the connection, and the usual message says so.
  addEventListener('vite:preloadError', () => {
    let last = 0;
    try { last = Number(sessionStorage.getItem('s47.reloaded')) || 0; } catch { /* no storage */ }
    if (Date.now() - last < 60000) return;
    try { sessionStorage.setItem('s47.reloaded', String(Date.now())); } catch { /* no storage */ }
    setTimeout(() => ui.toast('SIGNAL 47 has been updated. Saving the night and loading the new version.', 5), 50);
    const saved = saveBlock() ? Promise.resolve() : (draw(), writeSave('auto', 'rotate').then(() => {}, () => {}));
    Promise.all([saved, new Promise((r) => setTimeout(r, 2000))]).then(() => location.reload());
  });

  function toTitle() {
    mode = 'title';
    fcam.raise(false); ui.viewfinder(false);
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.close(true);
    ui.showHud(false, input.touchMode);
    endEl?.remove(); endEl = null;
    score?.stop();
    if (audio.ctx) { audio.stop('music', 1.5); audio.stopMotors(); audio.setCarrier(0, 0, 0); audio.signalOff(); audio.tvHiss(null); }
    motelTv.silence();
    follow.reset();
    world.stopDriving();
    roadside.reset(); cb.stop(); audio.stopPhoneCalls(); radio.reset(0);
    game.reset();
    world.enter('saro');
    pendingAuto = false;
    ui.fade(false);
    showTitle();
  }

  function showCard(o: Parameters<typeof ui.endcard>[0]) {
    mode = 'end';
    fcam.raise(false); ui.viewfinder(false);
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.showHud(false, input.touchMode);
    audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.9 });
    endEl = ui.endcard(o);
    setTimeout(() => ui.fade(false), 400);
  }
  function backToPlay(then: () => void) {
    audio.stop('music', 0.8);
    endEl?.remove(); endEl = null;
    ui.fade(true, '');
    mode = 'play';
    input.enabled = true;
    input.requestLock();
    then();
    ui.showHud(true, input.touchMode);
    setTimeout(() => ui.fade(false), 300);
  }

  // Between chapters the night goes on: a short card on the black with what was just
  // closed and the next chapter's title and clock, then straight back into the game.
  // The next chapter begins behind the black, so the clock and objective are already set.
  function chapterBreak(o: { closed: string; recap: string; next: string; title: string; begin: () => void }) {
    fcam.raise(false); ui.viewfinder(false);
    ui.close(true);
    game.cinematic = true;
    ui.fade(true, '');
    game.after(1.3, () => {
      o.begin();
      game.cinematic = true;
      ui.fade(true, '');
      let spoken = false;
      let titleWaiting = false, titleReady = false;
      void ui.chapterCard({ closed: o.closed, recap: o.recap, next: o.next, title: o.title, clock: clockText(game.clock, false) })
        .then(() => { spoken = true; });
      const finish = () => {
        if (mode !== 'play' || !game.cinematic) return;
        if ((ui.chapterStage === 'b' || ui.chapterStage === 'text-b' || ui.chapterStage === 'error-b') && !titleWaiting) {
          titleWaiting = true; game.after(3.2, () => { titleReady = true; });
        }
        if (!spoken || titleWaiting && !titleReady) { game.after(0.1, finish); return; }
        ui.chapterCard(null);
        ui.fade(false);
        game.cinematic = false;
        input.enabled = true;
        ui.showHud(true, input.touchMode);
      };
      game.after(6.4, finish);
    });
  }
  // End of the prologue: the night is not over.
  game.onFinish = () => chapterBreak({
    closed: 'NIGHT SHIFT // EVERY DISH AT AZ 026',
    recap: 'The array moved without a command. The phone played the room before it happened. The night is not over.',
    next: 'CHAPTER ONE', title: 'THE SECOND EXPOSURE',
    begin: () => game.beginChapter1(null),
  });
  // End of chapter one: Ward has the report, and the night goes on in the records room.
  ch1.onEnd = (method) => chapterBreak({
    closed: 'LOCAL INCIDENT S-03 / B-12 // FILED',
    recap: `${method === 'passive' ? 'The shielded lamp did not explain it.' : 'Simple encoder drift did not explain it.'} The eye saw one reference; the film kept two. One line on the receiver print is still unexplained: -39 LY.`,
    next: 'CHAPTER TWO', title: 'THE AMENDED RECORD',
    begin: () => game.beginChapter2(null),
  });
  // End of chapter two: the key holder sends you to the station first.
  ch2.onEnd = () => chapterBreak({
    closed: 'THE AMENDED RECORD // P04 AND P05 RECORDED',
    recap: 'The 1947 record was changed: the service copy leaves out C. B-12 still carries STATION 01, and the key holder wants you at the cut cable before you come to her.',
    next: 'CHAPTER THREE', title: 'THE SURVEY STATION',
    begin: () => game.beginChapter3(null),
  });
  // End of chapter three: the prints are filed, and N. Vega is waiting across the road.
  ch3.onEnd = () => chapterBreak({
    closed: 'THE SURVEY STATION // P06 TO P09 RECORDED',
    recap: 'Fixed point A was moved. The cable was cut on purpose. The receiver in the hut still carries T. Vega\'s correction. N. Vega is waiting in room 6, across the road.',
    next: 'CHAPTER FOUR', title: 'ROOM 6',
    begin: () => game.beginChapter4(null),
  });
  // End of chapter four: she has told you where C goes, and it is five in the morning.
  ch4.onEnd = () => chapterBreak({
    closed: 'ROOM 6 // P10 TO P12 RECORDED',
    recap: 'She cut the cable in 1947 with her brother standing on C, and he was not there afterwards. C crosses the old road to Roswell. The truck is on its pad.',
    next: 'CHAPTER FIVE', title: 'ALL NIGHT',
    begin: () => game.beginChapter5(null),
  });
  // End of chapter five, as far as the night is built.
  // End of chapter five: out of the diner and onto the old road (chapter six)
  ch5.onEnd = () => game.beginChapter6();
  // The end of the night: the 3D game stops on the black, then the title, the credits and the
  // whole photograph from 1947, and back to the title screen. Continue gives the diner again.
  async function finale() {
    const ch6 = game.ch6, end = ch6.ending;
    mode = 'end';
    fcam.raise(false); ui.viewfinder(false);
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.close(true); ui.clearToasts();
    ui.showHud(false, input.touchMode);
    world.stopDriving();
    audio.cabRadio(null);
    roadside.reset(); cb.stop(); audio.stopPhoneCalls(); radio.reset(0);
    audio.silence(false);
    score?.stop();
    if (!end) { toTitle(); return; }
    end.black(true, 0);
    audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.9 });
    await end.title();
    if (end.disposed) return;
    await end.credits();
    if (end.disposed) return;
    audio.stop('music', 2.5);
    await end.photo(ch6.photo(), () => audio.play('paper0', { gain: 0.9 }));
    if (end.disposed) return;
    toTitle();
  }

  // test hook, handy from the browser console: S47.jump('countdown')
  (window as any).S47 = {
    art: artStatus,
    jump: (p: string) => {
      audio.stopPhoneCalls();
      earlDialogue.stop();
      narrator.stop(); npcDialogue.stop(); ui.chapterCard(null);
      if (mode !== 'play') { startGame({ caseId: saves.freeCase() ?? 1, playtime: 0, state: null, jump: p }); return; }
      follow.reset();
      world.placeTruck('saro'); world.enter('saro'); game.start(p);
    },
    game, room, ext, camera, player, renderer, scene, flashlight, input, yard, fcam, ch1, annex, ch2, ch3, ch4, ch5, ch6: game.ch6, saves, world, doors, sky, backdrop, audio, earlDialogue, npcDialogue, narrator, radio, carRadio, dinerRadio, radioAimed: () => radioAimed(), calls, mapAt: () => mapAt(), placeName: () => placeName(), surface: () => surfaceAt(), musicWant: () => musicWant(), kitItems: () => kitItems(), peopleItems: () => peopleItems(), portraitCount, roadside, cb, townEvents, townSeen, glimpses, glimpseIn: () => glimpseIn(true), cutouts, cutoutIn: () => cutoutIn(true), nightEvents, nightIn: () => nightIn(true), lineLights, mapMarks: () => mapMarks(), abduction: () => abduction, abductIn: () => abductIn(true), setAbductions: (v: boolean) => { abductOn = v; },
    // write a save now (tests): the frame is drawn first so the save gets its picture
    saveNow: (kind: SaveKind = 'manual', slot: number | 'rotate' = 0) => { draw(); return writeSave(kind, slot); },
    playtime: () => playtime, runTime: () => t, caseId: () => caseId, score: () => score, musicTracks: () => Object.keys(TRACKS),
    // tests: the scene as the lights leave it, before the tape: how many colour values would not
    // fit the half-float targets the picture is drawn into (over 65504, or not a number at all),
    // and the brightest one (a 32-bit float target at a quarter of the size)
    hdrStats: () => {
      const s = renderer.getDrawingBufferSize(new THREE.Vector2());
      const rt = new THREE.WebGLRenderTarget(s.x >> 1, s.y >> 1, { type: THREE.FloatType });
      renderer.setRenderTarget(rt); renderer.render(scene, camera); renderer.setRenderTarget(null);
      const px = new Float32Array(rt.width * rt.height * 4);
      renderer.readRenderTargetPixels(rt, 0, 0, rt.width, rt.height, px);
      rt.dispose();
      let over = 0, max = 0, at = -1;
      for (let i = 0; i < px.length; i += 4) for (let k = 0; k < 3; k++) {
        const v = px[i + k];
        if (!(v <= 65504)) over++; else if (v > max) { max = v; at = i / 4; }
      }
      // where the brightest is, from the top left in screen fractions
      return { over, max, x: (at % rt.width) / rt.width, y: 1 - Math.floor(at / rt.width) / rt.height, w: rt.width, h: rt.height };
    },
    three: THREE,
    // true once a started or loaded night is running (tests wait for it after Continue)
    started: () => caseId > 0 && !starting && mode === 'play',
  };
  // the free camera and the album (tools/album.py): the album itself, what could be named in a
  // frame from here, and which of the round 15 album pictures the build has
  Object.assign((window as any).S47, {
    album, albumArt,
    albumSubjects: () => subjectsHere().map((s) => ({ name: s.name, n: s.at.length, odd: !!s.odd })),
  });

  Object.assign((window as any).S47, { pasture, motelTv });   // events 2 and 10 (tools/cowstv.py)
  Object.assign((window as any).S47, { follow });   // the dishes that follow (tools/horror_dishes.py)
  // tests: the bought models, and the code's props in their place (tools/licensed_models.py)
  Object.assign((window as any).S47, { models: licensedModels });

  // ---------- the developer menu (?dev in the address) ----------
  let devPaused = false;
  dev = devOn ? new DevMenu(ui, devGroups, {
    onOpen: () => { if (mode === 'play' && !pausedByMenu) { devPaused = true; pausedByMenu = true; audio.suspend(true); } },
    onClose: () => { if (devPaused) { devPaused = false; pausedByMenu = false; audio.suspend(false); } },
  }) : null;
  // Start a run from a chapter or beat: a new night that is never saved.
  function devStart(jump?: string, then?: () => void) {
    const unlocking = audio.unlock(); // inside the click
    devWait = null;
    if (mode !== 'title') leavePlay();
    void startGame({ caseId: 0, playtime: 0, state: null, jump, unlocking, dev: true }).then(() => then?.());
  }
  // then: once ok() holds, checked every frame (the new run may take a while to load)
  function waitFor(ok: () => boolean, fn: () => void) { devWait = { ok, fn }; }
  function devGroups(): DevGroup[] {
    const groups: DevGroup[] = [{
      title: 'Start at', note: 'A new run from there, with everything before it done.',
      items: [
        { label: 'Prologue 23:41', run: () => devStart() },
        { label: 'Prologue 02:13', run: () => devStart('residual') },
        { label: 'Prologue: locked on', run: () => devStart('locked') },
        { label: 'Prologue: countdown', run: () => devStart('countdown') },
        { label: '1 The Second Exposure', run: () => devStart('chapter1') },
        { label: '2 The Amended Record', run: () => devStart('chapter2') },
        { label: '3 The Survey Station', run: () => devStart('chapter3') },
        { label: '3 On the highway', run: () => devStart('chapter3', () => waitFor(() => !starting && ch3.s.stage === 'to-truck', () => world.driveOut(true))) },
        { label: '4 Room 6', run: () => devStart('chapter4') },
        { label: '5 All Night', run: () => devStart('chapter5') },
        { label: '6 Roswell Road', run: () => devStart('chapter6') },
        { label: '6 At the line', run: () => devStart('chapter6', () => waitFor(() => world.area === 'roswell' && world.driving && game.ch6.stage === 'drive', () => {
          const p = world.oldRoad!.poseAt(8.06);
          world.oldDrive!.place(p.pos, p.heading);
          game.clock = 5 * 3600 + 28 * 60 + 30;
        })) },
      ],
    }];
    if (mode === 'play') {
      groups.push({
        title: 'Truck', note: 'Out on a road from wherever you are. Nothing in the story happens there.',
        items: [
          { label: 'Drive the highway', run: () => world.freeDrive('road'), on: world.free === 'road' },
          { label: 'Drive the old road', run: () => world.freeDrive('oldroad'), on: world.free === 'oldroad' },
          ...(world.free ? [{ label: 'Back on foot', run: () => world.endFreeDrive() }] : []),
        ],
      });
      // trouble with the truck (drive/trouble.ts), for trying out what the story will use
      groups.push({
        title: 'Truck trouble', note: 'While driving on a trip. Random trouble comes now and then at night when it is on.',
        items: [
          { label: 'Engine misfires', run: () => world.trouble.start('sputter') },
          { label: 'Engine dies', run: () => world.trouble.start('stall') },
          { label: 'Engine dies, you turn the key', run: () => world.trouble.start('stall', { key: true }) },
          { label: 'Lights flicker', run: () => world.trouble.start('flicker') },
          { label: 'Dash goes dark', run: () => world.trouble.start('dashOut') },
          { label: world.trouble.random ? 'Random trouble: on' : 'Random trouble: off', run: () => { world.trouble.random = !world.trouble.random; }, on: world.trouble.random },
        ],
      });
      groups.push({ title: 'Clock', items: [
        { label: '+5 minutes', run: () => { game.clock += 300; } },
        { label: '+30 minutes', run: () => { game.clock += 1800; } },
        { label: 'Shooting star', run: () => sky.meteorNow() },
      ] });
      // the radio (core/radio.ts): 1420 signs off at four with the anthem, Roswell signs on at five
      groups.push({ title: 'Radio', note: 'The truck\'s radio plays while you drive (R, or Radio on a phone).', items: [
        { label: carRadio.on ? `Truck radio: AM ${FREQ[carRadio.station]}` : 'Truck radio: off', run: () => { if (!carRadio.on) carRadio.set(true, 'caliche'); else carRadio.next(); }, on: carRadio.on },
        { label: '1420 signs off (03:59:40)', run: () => { game.clock = SIGN_OFF - 20; } },
        { label: 'Roswell signs on (04:59:50)', run: () => { game.clock = SIGN_ON - 10; } },
      ] });
      groups.push({ title: 'Out in the dark', note: 'On foot outside at night with the flashlight on, something sounds now and then.', items: [
        { label: 'A sound out in the dark', run: () => calls.now() },
      ] });
      groups.push({ title: 'On B', note: 'The herd south of the motel turns to 026 once you are well away from it; the office television goes to the carrier while you are in the office with the truck on the lot.', items: [
        { label: `The herd: ${pasture.stage}`, run: () => pasture.now() },
        { label: `The office TV: ${motelTv.mode}`, run: () => motelTv.now() },
      ] });
      groups.push({ title: 'The highway', note: 'On foot on the shoulder between SARO and the motel at night: the line hums, a car far off, the silence once, someone on the roof. The CB in the truck.', items: [
        { label: 'Headlights far down the highway', run: () => roadside.carNow(player.pos) },
        { label: 'The silence (47 s)', run: () => roadside.silenceNow({ hush: (secs, level) => audio.night?.hush(secs, level), flicker: (k) => flashlight.flicker(k, 'silence') }) },
        ...(['steps', 'knocks', 'radio', 'bulletin', 'phone', 'door'] as const).map((id) => ({ label: `Night event: ${id}`, run: () => { if (!nightEvents.now(id, nightIn(true))) ui.toast(`Not here: ${id}`, 2); } })),
        { label: 'Someone on the motel roof', run: () => roadside.roofNow() },
        { label: 'A car on the county road (far east)', run: () => { if (!world.life.carNow()) ui.toast('No county road seen from here', 2); } },
        { label: 'A tumbleweed', run: () => { if (!world.life.tumbleNow(true)) ui.toast('No open ground for one here', 2); } },
        { label: 'A call on the CB', run: () => cb.call([{ text: 'SARO seven, this is Sierra. You hear me?', who: 'woman', by: 'N. VEGA' }], world.driving ? null : world.truckHere()?.group.position ?? null,
          (l) => ui.toast(`CB, ${l.by}: "${l.text}"`, 4)) },
      ] });
      // the time away (story/abduction.ts), and the dishes that follow in the antenna field (story/dishesFollow.ts)
      groups.push({ title: 'Missing time', note: 'Alone outdoors on a line at night. In the antenna field the warning brings the dishes down, once in a night.', items: [
        { label: 'Begin the warning here', run: () => { const s = abductIn(true); if (abduction!.busy || !s.line) ui.toast(abduction!.busy ? 'Already under way' : 'Not on a line here', 2); else abduction!.begin(s); } },
        { label: abduction!.data.dishes ? 'The dishes: done tonight (let them again)' : 'The dishes: not yet tonight', run: () => { follow.reset(); delete abduction!.data.dishes; } },
      ] });
    }
    if (mode === 'play' && score) {
      groups.push({ title: 'Music', note: 'Each piece from the start. The game plays them by itself all the time, chosen by place.', items: [
        ...Object.keys(TRACKS).map((k) => ({ label: TRACKS[k].title, run: () => score!.start((Object.keys(CUES) as Cue[]).find((c) => CUES[c].includes(k))!, k), on: score!.track === k && !!score!.playing })),
        ...(score.playing ? [{ label: 'Stop the music', run: () => score!.fade(1.5, 120) }] : []),
      ] });
    }
    groups.push({ title: 'Tools', items: [
      { label: debug ? 'Hide fps' : 'Show fps', run: () => { if (debug) { debug.dispose(); debug = null; } else debug = hud(); }, on: !!debug },
    ] });
    return groups;
  }

  // ---------- resize and adaptive resolution ----------
  function resize() {
    const w = innerWidth, h = innerHeight;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    // a phone held upright sees more of the room with a wider lens
    if (!fcam.raised) camera.fov = w < h ? baseFov + 12 : baseFov;
    camera.updateProjectionMatrix();
  }
  addEventListener('resize', resize);
  resize();
  let perfAcc = 0, perfN = 0, perfHold = 0;
  function adapt(dt: number) {
    perfAcc += dt; perfN++;
    if (perfAcc < 2) return;
    const avg = perfAcc / perfN; perfAcc = 0; perfN = 0;
    if (perfHold > 0) { perfHold--; return; }
    if (avg > 0.024 && pr > 0.7) { pr = Math.max(0.7, pr - 0.2); renderer.setPixelRatio(pr); resize(); perfHold = 1; }
    else if (avg < 0.0135 && pr < maxPR) { pr = Math.min(maxPR, pr + 0.1); renderer.setPixelRatio(pr); resize(); perfHold = 2; }
  }

  // ---------- title camera ----------
  const titleFrom = new THREE.Vector3(-22.5, 2.0, 70);
  const titleLook = new THREE.Vector3(2, 9, -60);
  function titleCam(t: number) {
    camera.position.set(titleFrom.x + Math.sin(t * 0.05) * 2.2, titleFrom.y + Math.sin(t * 0.08) * 0.15, titleFrom.z - Math.sin(t * 0.04) * 3);
    camera.lookAt(titleLook.x + Math.sin(t * 0.03) * 4, titleLook.y, titleLook.z);
  }

  // ---------- loop ----------
  // step() advances the simulation, draw() renders. Tests can hold the loop and
  // drive both by hand (S47.hold / S47.tick), which keeps headless runs fast.
  let last = performance.now(), t = 0, crtAcc = 0;
  // which ambience the listener is in (room tone, open yard, photo lab)
  let space: 'room' | 'yard' | 'lab' = 'room';
  const inside = (b: { minX: number; maxX: number; minZ: number; maxZ: number }, p: THREE.Vector3) => p.x >= b.minX && p.x <= b.maxX && p.z >= b.minZ && p.z <= b.maxZ;
  const indoors = [room.bounds, annex.zone.southDoor, annex.zone.corridor, annex.zone.recordsDoor, annex.zone.records];
  // Which fake lamps the PC tier may turn into real ones where the player stands
  // (core/ultra.ts), and whether the control room's own lamp shadows are worth drawing.
  const ultraLamps = (p: THREE.Vector3): [readonly string[], boolean] => {
    switch (world.area) {
      case 'saro':
        if (inside(room.bounds, p)) return [[], true];
        if ([annex.zone.corridor, annex.zone.records, annex.zone.recordsDoor, annex.zone.southDoor].some((z) => inside(z, p))) return [['annex'], false];
        if (inside(yard.zone.lab, p)) return [['lab'], false];
        if (inside(motelOffice, p)) return [['court'], false];
        return [['site', 'court'], false];
      case 'station01': return [world.indoors(p) ? ['hut'] : ['field'], false];
      case 'room6': return [['motel'], false];
      case 'diner': return [['diner'], false];
      case 'roswell': return [['oldroad'], false];
      default: return [['road'], false];
    }
  };
  const spaceOf = (p: THREE.Vector3) => indoors.some((b) => inside(b, p)) ? 'room' : inside(yard.zone.lab, p) ? 'lab' : 'yard';
  const hemiBase = room.lights.hemi.intensity;
  // What music fits where the player is (core/score.ts), and when it must keep out of the way.
  function musicWant(): Cue | null {
    if (game.phase === 'ch6') return null;                    // the old road has its radio and its silence
    if (world.driving) return 'drive';
    if (world.area === 'station01') return 'tension';
    if (world.area === 'room6') return 'motel';
    if (world.area === 'saro' && world.crossing.atMotel(player.pos)) return 'motel';
    const hour = (((game.clock % 86400) + 86400) % 86400) / 3600;
    if (hour >= 5 && hour < 12) return 'dawn';
    if (world.area === 'diner') return world.roadStopAt(player.pos) ? 'eerie' : 'town';   // the lot and the road by the diner at night (inside, its radio); out on the highway, the dark
    return space === 'yard' ? 'night' : 'eerie';
  }
  const HELD = ['ringing', 'call', 'countdown', 'event', 'turning', 'end'];
  // ---------- the radios (core/radio.ts) ----------
  // The truck's radio plays from its dash wherever the truck stands in this area (left on when
  // the player gets out); indoors it is a long way off. The diner's radio plays the town's
  // station while it is on the air, then Roswell once that signs on, near the diner.
  let radioHeard = false;
  const radioAt = new THREE.Vector3(), counterAt = new THREE.Vector3();
  let carNear = false;
  // the programme on the way: each trip's lines at marks along it, on the station the truck's
  // radio is tuned to (story/tripRadio.ts)
  const tripRadio = new TripRadio(radio, townSeen);
  (window as any).S47.tripRadio = () => tripRadio;   // the tests
  function radioStep(dt: number) {
    const tr = world.truckHere();
    if (tr) { tr.group.getWorldPosition(radioAt); radioAt.y += 1.05; }
    carRadio.pos = tr ? radioAt : null;
    carRadio.power = (world.driving ? world.trouble.radioLevel : 1) * (!world.driving && space === 'room' ? 0.3 : 1);
    carRadio.muffle = world.driving ? 0 : 1;   // out of the truck it is only just heard, through the closed cab
    tr?.setRadio(dialX(FREQ[carRadio.station]), carRadio.on);
    carNear = !!tr && (world.driving || player.pos.distanceTo(radioAt) < 7);
    const counter = world.diner?.proxies.counter;
    let dinerOn = false;
    if (counter && (world.area === 'diner' || world.area === 'road')) {
      counter.getWorldPosition(counterAt);
      const near = (world.driving ? camera.position : player.pos).distanceTo(counterAt) < 60;
      const st: StationId | null = radio.stations.caliche.onAir ? 'caliche' : radio.stations.roswell.onAir ? 'roswell' : null;
      if (near && st) { dinerOn = true; dinerRadio.set(true, st); }
    }
    if (!dinerOn && dinerRadio.on) dinerRadio.set(false);
    dinerRadio.pos = dinerOn ? counterAt : null;
    radio.update(dt, game.clock);
    // (after the stations: one that has just come to its sign-off is not free any more)
    tripRadio.update({ trip: world.tripNow, phase: game.phase, clock: game.clock, tuned: world.driving && carRadio.live && !game.cinematic ? carRadio.station : null });
    radioHeard = (carRadio.live && carNear) || (dinerRadio.live && !world.driving && player.pos.distanceTo(counterAt) < 12);
    ui.radioButton(world.driving && !!tr, carRadio.on);
  }
  // A station's line is captioned once while the truck or counter radio can be heard.
  radio.onLine = (id, text, by, duration) => {
    const car = carRadio.live && carRadio.station === id && carNear;
    const diner = dinerRadio.live && dinerRadio.station === id && !world.driving && player.pos.distanceTo(counterAt) < 12;
    if (mode !== 'play' || game.cinematic || !(car || diner)) return;
    if (radio.stations[id].strength(radio.clock, radio.time) < 0.35) return;
    ui.toast(by ? `RADIO, ${by}: "${text}"` : `RADIO: "${text}"`, duration === undefined ? Math.min(8, 2.2 + text.length / 22) : Math.max(2.4, duration + 0.2), () => audio.ctx!.currentTime);
    // event 9 (HENDELSER.md): heard, the boy's call goes into the notebook and onto the map
    if (by === BOY && !townSeen.has('ev:boy')) {
      townSeen.add('ev:boy'); townSeen.add(`mark:boy@${Math.round(game.clock)}`);
      game.note(`${clockText(game.clock, false)}. Night Line, 1420. A boy calling about a star with a tail, low in the south, and a light over the mesa. From his father's ranch. He said it was July.`);
      requestAutosave(false);
    }
  };
  // HENDELSER.md, events 2 and 10 (story/pasture.ts, story/motelTv.ts), and with three marks on B
  // the line drawn in on the map, as A is in town (story/townEvents.ts)
  const officeDoor = new THREE.Vector3(-41.1 + world.motel.at.x, 0, 24.1 + world.motel.at.z);
  let lineBCheck = 0;
  function eventsOnB(dt: number, ok: boolean) {
    const p = player.pos;
    // (on foot on the road's ground, by the herd too, the area is the diner's: the road's herd)
    pasture.update(dt, { ok, area: world.area === 'saro' ? 'saro' : world.area === 'road' || world.area === 'diner' ? 'road' : null, cam: camera, clock: game.clock,
      driving: world.driving });
    motelTv.update(dt, { ok, saro: world.area === 'saro', here: world.area === 'saro' && !world.driving && (inside(motelOffice, p) || Math.hypot(p.x - officeDoor.x, p.z - officeDoor.z) < 5),
      truckOnLot: world.truckAt === 'motel', cam: camera, clock: game.clock });
    if ((lineBCheck += dt) > 1 && !townSeen.has('lineB')) {
      lineBCheck = 0;
      if (mapMarks().filter((m) => m.line === 'B').length >= 3) {
        townSeen.add('lineB');
        game.note(`${clockText(game.clock, false)}. Three on one line. Drawn in on the map: across the highway just south of the motel, and on to the west-south-west, through STATION 01.`);
        requestAutosave(false);
      }
    }
  }
  function musicHeld() { return game.cinematic || HELD.includes(game.phase) || (audio.radioOn && space !== 'yard') || radioHeard || roadside.silent || motelTv.heard || abduction!.busy; }
  const dbg = { hold: false };
  let inCab = false;
  const townAt = new THREE.Vector3();
  let canOut = false;   // stopped in the truck with ground by a door: Use gets out
  function step(dt: number) {
    t += dt;
    if (mode === 'play') playtime += dt;
    const modalOpen = !!ui.modal;
    if (mode === 'title') { titleCam(t); annex.interior.visible = false; world.airStep(dt, t); }
    else {
      const active = mode === 'play' && !modalOpen && !game.cinematic && !abduction!.holds;
      const look = input.consumeLook();
      if (world.driving) {
        // the truck: forward and back are throttle and brake, sideways is steering
        const m = active ? input.move() : { x: 0, z: 0 };
        if (!modalOpen) world.update(dt, t, { steer: m.x, throttle: -m.z }, active ? { x: look.x, y: invertY ? -look.y : look.y } : { x: 0, y: 0 });
      } else {
        if (active) {
          const fine = 1 - 0.6 * zoomK;   // finer aim through the closer look
          player.look(look.x * fine, (invertY ? -look.y : look.y) * fine);
          const m = input.move();
          player.update(dt, m.x, m.z, m.run);
        } else {
          // held on waking: only the eyes, up, towards what is over you (story/abduction.ts)
          if (mode === 'play' && !modalOpen && abduction!.holds) abduction!.gaze(look.x, invertY ? -look.y : look.y);
          player.update(dt, 0, 0, false);
        }
        world.update(dt, t, null, { x: 0, y: 0 });
      }
      game.update(dt, t, modalOpen);
      const c5 = game.ch5.s.stage;
      world.setStoryPlaces({ stationOpen: game.at('ch3'), oldRoadOpen: game.at('ch6') || (game.ch5.active && (c5 === 'road' || c5 === 'complete')) });
      // the closer look: the lens narrows while right mouse (or Zoom) is held, on foot
      const canZoom = mode === 'play' && !modalOpen && !game.cinematic && !fcam.raised && !world.driving;
      const z0 = zoomK;
      zoomK += ((input.zoomHeld && canZoom ? 1 : 0) - zoomK) * Math.min(1, dt * 9);
      if (zoomK < 0.002) zoomK = 0;
      if (!fcam.raised && (zoomK > 0 || z0 > 0)) {
        camera.fov = (innerWidth < innerHeight ? baseFov + 12 : baseFov) * (1 - 0.55 * zoomK);
        camera.updateProjectionMatrix();
      }
      flashlight.update(dt, t, camera, mode !== 'play' || world.driving || game.cinematic, ultra.on);
      ui.lightButton(flashlight.have, flashlight.on);
      if (devWait?.ok()) { const f = devWait.fn; devWait = null; f(); }
      doors.update(dt);
      if (mode === 'play') { radioStep(dt); audio.nightLife(dt, player.pos, game.clock); score?.update(dt, musicWant(), musicHeld()); }
      if (mode === 'play') roadside.update(dt, {
        ok: !modalOpen && !game.cinematic && !world.driving && world.area === 'saro' && world.crossing.onHighway(player.pos) && game.at('ch1') && game.phase !== 'ch6'
          && sky.uniforms.uDawn.value < 0.3,
        pos: player.pos, cam: camera,
        silenceDone: game.notes.some((n) => n.includes('No sound for 47 s')),
        hush: (secs, level) => audio.night?.hush(secs, level), flicker: (k) => flashlight.flicker(k, 'silence'),
        silenceOver: () => { game.note(`${clockText(game.clock, false)}. ${silencePlace ?? 'The highway shoulder between SARO and the motel'}. No sound for 47 s. Not my footsteps either.`); silencePlace = null; requestAutosave(false); },
      });
      if (mode === 'play') calls.update(dt, {
        ok: !modalOpen && !game.cinematic && !world.driving && flashlight.have && flashlight.on && space === 'yard' && sky.uniforms.uDawn.value < 0.3 && !roadside.silent,
        cam: camera, flicker: (k) => flashlight.flicker(k), hush: (secs) => audio.night?.hush(secs),
      });
      if (mode === 'play') {
        const cp = camera.getWorldPosition(townAt);
        townEvents.update(dt, {
          // The return call follows Room 6 and remains available in the diner's dawn.
          // Each other town event keeps its own earlier clock window.
          ok: !modalOpen && !game.cinematic && game.at('ch1') && game.phase !== 'ch6'
            && (sky.uniforms.uDawn.value < 0.3 || game.phase === 'ch5') && !abduction!.busy && !nightEvents.busy,
          inTown: world.area === 'diner' && !world.indoors(cp) && !!world.town?.inTown(cp.x, cp.z),
          later: game.at('ch5'), cam: camera, clock: game.clock, t,
        });
      }
      // one thing at a time: not while the town has one of its own going (story/townEvents.ts)
      if (mode === 'play') nightEvents.update(dt, nightIn(!modalOpen && !game.cinematic && game.at('ch1') && game.phase !== 'ch6' && sky.uniforms.uDawn.value < 0.3 && !roadside.silent && !abduction!.busy && townEvents.now === 'none'));
      if (mode === 'play') cutouts.update(dt, cutoutIn(!modalOpen && !game.cinematic && game.at('ch1') && !abduction!.busy));
      if (mode === 'play') {
        // the flashlight flickers near the strange events (todo.md): through the silence on the
        // shoulder, while a figure stands at the edge of the view, while the town's lamps go out or
        // its payphone rings (the sounds in the dark and the time away flicker it themselves)
        flashlight.flicker(roadside.silent ? 0.4 : 0, 'silence');
        flashlight.flicker(glimpses.up ? 0.3 : 0, 'glimpse');
        flashlight.flicker(townEvents.now !== 'none' && world.area === 'diner' ? 0.35 : 0, 'town');
      }
      if (mode === 'play') glimpses.update(dt, glimpseIn(!modalOpen && !game.cinematic && game.at('ch1') && game.phase !== 'ch6' && sky.uniforms.uDawn.value < 0.3 && !roadside.silent && !abduction!.busy));
      if (mode === 'play') lineLights.update(dt, {
        ok: !modalOpen && !game.cinematic && game.at('ch1') && game.phase !== 'ch6' && sky.uniforms.uDawn.value < 0.3 && !abduction!.busy,
        truck: world.driving && world.area === 'saro' ? world.truckHere()?.group.position ?? null : null,
        clock: game.clock, ground: (x, z) => world.grounds.floorAt(x, z),
      });
      if (mode === 'play') eventsOnB(dt, !modalOpen && !game.cinematic && game.at('ch1') && game.phase !== 'ch6' && sky.uniforms.uDawn.value < 0.3 && !abduction!.busy);
      // the notebook or a paper open holds the warning where it is, as it holds the clock: it is not
      // a way out (abduction.ts lets the warning pass when ok goes false), and the dishes wait too
      if (mode === 'play' && !(modalOpen && abduction!.stage === 'warning')) {
        const ab = abduction!;
        ab.sceneId = world.area === 'saro' ? 'saro' : world.area === 'diner' || world.area === 'road' ? 'road' : world.area;
        ab.update(dt, abductIn(!modalOpen && !game.cinematic && ['ch1', 'ch2', 'ch3', 'ch4'].includes(game.phase) && sky.uniforms.uDawn.value < 0.3
          && !roadside.silent && townEvents.now === 'none' && !motelTv.busy));
        // the eyes from where the player stands now (the waking puts them down in the grass this step)
        follow.update(dt, { stage: ab.stage, warning: ab.warn, saro: world.area === 'saro', onFoot: !world.driving,
          eye: followEye.set(player.pos.x, player.floorY + player.eye, player.pos.z), view: camera.getWorldDirection(followView),
          clock: game.clock, backAt: ab.backAt, calm: calmFlash, woke: ab.data.last });
      } else if (mode === 'play') follow.wait();
      // in the cab: no crosshair, and the touch buttons for using things and the camera go away
      if (world.driving !== inCab) { inCab = world.driving; document.documentElement.classList.toggle('driving', inCab); }
      if (!world.driving && canOut) { canOut = false; document.documentElement.classList.remove('can-out'); }
      const sp = world.driving ? 'lab' : world.area === 'station01' || world.area === 'room6' || world.area === 'diner' ? (world.indoors(player.pos) ? 'room' : 'yard') : inside(motelOffice, player.pos) ? 'room' : spaceOf(player.pos);
      if (sp !== space) { space = sp; audio.setSpace(sp); }
      // the extension has no windows: draw its rooms only from inside, or through the open door
      annex.interior.visible = world.area === 'saro' && (indoors.slice(1).some((b) => inside(b, player.pos))
        || (inside(room.bounds, player.pos) && room.southDoorHinge.rotation.y > 0.01)
        || (annex.exitOpen > 0.01 && inside(world.crossing.zone.stoop, player.pos)) || inside(annex.zone.exitDoor, player.pos));
      crtAcc += dt;
      if (crtAcc > 1 / 12) { crtAcc = 0; game.drawCrts(t); }
      if (mode === 'play' && !modalOpen && !game.cinematic && !fcam.raised && !world.driving) {
        const it = inter.update(camera);
        ui.prompt(it ? it.label() : null, input.touchMode);
        ui.hint(!input.touchMode && !input.locked);
        // a closer look: what the thing under the crosshair is, once the zoom is most of the way in
        ui.examine(zoomK > 0.6 ? inter.examine(camera)?.text ?? null : null);
      } else {
        const live = mode === 'play' && !modalOpen && !game.cinematic;
        const key = live && world.driving && world.trouble.wantsKey, aimR = live && !key && radioAimed(), out = live && !key && !aimR && world.canGetOut();
        if ((out || key || aimR) !== canOut) { canOut = out || key || aimR; document.documentElement.classList.toggle('can-out', canOut); }
        ui.prompt(key ? 'Turn the key' : aimR ? radioLabel() : out ? 'Get out' : fcam.raised && input.touchMode ? 'Shutter' : null, input.touchMode);
        ui.hint(false);
        ui.examine(null);
      }
    }
    sky.group.rotation.y = world.skyYaw;
    sky.update(dt, t, camera.position, renderer.getPixelRatio());
    // in the truck the player stays where they got in (the control room, say): the lamps that
    // become real lights are the ones round the truck (drivelook dinerlight, Codex)
    const lampsAt = world.driving ? camera.position : player.pos;
    const [lampSets, roomLamps] = ultraLamps(lampsAt);
    ultra.update(dt, lampsAt, lampSets, roomLamps);
    world.shareLights();
    sky.setCometClock(game.clock);
    // the dawn follows the clock; the night areas are lit for the night, so they show less of it
    sky.setDawnLift(game.ch6.skyLift ?? (world.area === 'roswell' ? 1 : 0.7));
    sky.setClock(mode === 'title' ? 0 : game.clock, mode === 'title' ? null : game.ch6.sunElev);
    // out in the night areas the haze over the far land stays darker than the dawn sky behind the
    // mesas, which take the same haze (horizon.ts); a pale band in front of them otherwise (drivelook D02).
    // At night the far land is as dark as the foot of the painted mesas behind it (backdrop.ts): at
    // the sky's own haze it was a pale cold line all round under them (the art director, 19 October)
    const fogC = (scene.fog as THREE.FogExp2).color.copy(sky.fogColor), dawnK = THREE.MathUtils.smoothstep(sky.uniforms.uDawn.value, 0, 0.35);
    fogC.multiplyScalar(world.area !== 'roswell' ? THREE.MathUtils.lerp(NIGHT_FOG, DAWN_FOG, dawnK) : THREE.MathUtils.lerp(NIGHT_FOG, 1, dawnK));
    // the painted horizon: the area's air, the land where it ends, the lit places' glow where they are from here
    backdrop.update(fogC, (scene.fog as THREE.FogExp2).density, world.area === 'roswell', camera.position.y - world.groundUnder(camera.position),
      world.skyPlaces(), camera.position, world.skyYaw);
    ext.update(dt, t);
    room.updateOutside();
    yard.update(t);
    annex.update(t);
    room.lights.hemi.intensity = hemiBase + sky.uniforms.uFlash.value * 2.4;
    audio.listener(camera);
  }
  const buf = new THREE.Vector2();
  function draw() {
    camera.updateMatrixWorld();
    // glow points keep their size in metres: pixels per metre at one metre distance.
    // Set every frame, because the viewfinder and the adaptive resolution change both.
    renderer.info.reset();
    glowScale.value = vhs.lines(renderer.getDrawingBufferSize(buf).y) / (2 * Math.tan(camera.fov * Math.PI / 360));
    ext.dishArray.cull(camera);
    vhs.render(scene, camera, performance.now() / 1000);
  }
  const hud = () => new DebugHud(renderer, () => `${quality}   ${innerWidth}x${innerHeight}   ${mode}${mode === 'play' ? '  ' + game.phase : ''}${game.phase === 'ch1' ? '  ' + ch1.s.stage : ''}${game.phase === 'ch2' ? '  ' + ch2.s.stage : ''}${game.phase === 'ch3' ? '  ' + ch3.s.stage + '  ' + world.area : ''}${game.phase === 'ch4' ? '  ' + ch4.s.stage + '  ' + world.area : ''}${game.phase === 'ch5' ? '  ' + ch5.s.stage + '  ' + world.area : ''}${game.phase === 'ch6' ? '  ' + game.ch6.stage + '  ' + world.area : ''}${world.free ? '  free drive' : ''}${devRun ? '  dev' : ''}${score?.playing ? '  music ' + score.playing + ' ' + score.track : ''}`);
  let debug: DebugHud | null = debugOn ? hud() : null;
  function frame(now: number) {
    requestAnimationFrame(frame);
    const raw = (now - last) / 1000; last = now;
    const dt = Math.min(0.05, raw);
    if (pausedByMenu || dbg.hold) return;
    if (mode === 'end') return; // the end card covers the whole screen, so let the GPU rest
    step(dt);
    draw();
    flushAutosave(); // right after the frame, so the save's picture is this frame
    adapt(dt);
    debug?.frame(raw);
  }
  // Object.assign copies accessor values, not their descriptors. Keep hold live
  // so the automated scenarios really stop the realtime simulation loop.
  Object.defineProperty((window as any).S47, 'hold', {
    get: () => dbg.hold, set: (v: boolean) => { dbg.hold = v; }, enumerable: true,
  });
  // tests: the Blender models on and off (S47.blender = true, then wait for blenderHouse.shown
  // and blenderTruck.shown)
  Object.defineProperty((window as any).S47, 'blender', {
    get: () => blenderHouse.on, set: (v: boolean) => { void blenderHouse.set(!!v); void blenderTruck.set(!!v); }, enumerable: true,
  });
  Object.assign((window as any).S47, { blenderHouse, blenderTruck, floodSets });
  // tests: the truck as a save finds it, and the light in the air (world/airlight.ts)
  Object.assign((window as any).S47, { truckSave: () => world.truckSave(), air: () => (world as any).air });
  // tests: the light cookies on and off (tools/cookies.py): the flashlight's map, the headlights'
  // sealed beams in the floods, the windows' light on the ground
  Object.assign((window as any).S47, {
    cookies: (on: boolean) => { flashlight.cookie(on); headCookie.value = on ? 1 : 0; setWindowCookies(on); },
    cookieStats: () => ({
      flashlight: { cookie: flashlight.light.decay === FLASH_DECAY, map: !!flashlight.light.map },
      head: floodSets.filter((s) => s.head.value.x >= 0).map((s) => s.key),
      windows: Object.fromEntries(windowLights.map((w) => [w.name, w.windows])),
    }),
    windowLights,
  });
  Object.assign((window as any).S47, {
    // advance the game by `seconds` in fixed steps, then render one frame
    tick(seconds = 0, fps = 30) { const n = Math.max(1, Math.round(seconds * fps)); for (let i = 0; i < n; i++) step(1 / fps); draw(); flushAutosave(); },
    setQuality: (q: Quality) => applyQuality(ultraOk || q !== 'ultra' ? q : 'high'),
    ultra,
    vhs, setPicture: (p: Picture) => applyPicture(p),
  });

  applyQuality(quality);
  resize();
  titleCam(0);
  vhs.render(scene, camera, 0);
  loading.remove();
  showTitle();
  // the saves are read from IndexedDB while the title is up; then Continue knows what to offer
  saves.ready.then(() => { if (mode === 'title' && titleEl) showTitleAgain(); });
  requestAnimationFrame(frame);
}

boot();
