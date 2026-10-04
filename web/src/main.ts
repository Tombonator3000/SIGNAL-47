import './style.css';
import * as THREE from 'three';
import { UI } from './ui/UI';
import { AudioSys, type Surface } from './core/Audio';
import { Input } from './core/Input';
import { Interaction } from './core/Interaction';
import { Player } from './player/Player';
import { Sky } from './world/Sky';
import { Exterior } from './world/Exterior';
import { ControlRoom } from './world/ControlRoom';
import { Prologue } from './story/Prologue';
import { ServiceYard, YARD } from './world/ServiceYard';
import { RecordsAnnex } from './world/Annex';
import { Vhs, type Picture } from './core/vhs';
import { FieldCamera } from './core/FieldCamera';
import { setQuality, type Quality } from './core/quality';
import { loadFonts } from './core/fonts';
import { DebugHud, debugOn } from './core/debug';
import { glowScale } from './world/glow';
import { SaveStore, type SaveKind, type SaveMeta } from './core/saves';
import { saveMenu, played } from './ui/SaveMenu';
import { type GameState, migrateOldSave } from './story/state';
import { clockText } from './story/time';
import { loadArt, artStatus } from './core/art';
import { initArtMaterials } from './world/kit';
import { World } from './world/World';
import { Doors } from './world/Doors';
import type { AreaId } from './story/Chapter3';

// Settings and the one checkpoint live in localStorage. Every access is guarded,
// because storage can be blocked (private mode, sandboxed frames).
const store = {
  get<T>(k: string, d: T): T { try { const v = localStorage.getItem('s47.' + k); return v === null ? d : JSON.parse(v) as T; } catch { return d; } },
  set(k: string, v: unknown) { try { localStorage.setItem('s47.' + k, JSON.stringify(v)); } catch { /* ignore */ } },
};
const wait = (ms: number) => new Promise((r) => setTimeout(r, ms));

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
  const ext = new Exterior();
  const room = new ControlRoom();
  const yard = new ServiceYard();
  const annex = new RecordsAnnex();
  scene.add(sky.group, ext.group, room.group, yard.group, annex.group);
  const fcam = new FieldCamera(renderer, scene, camera);

  let quality: Quality = store.get<Quality>('quality', 'high');
  // the picture: a 1986 tape look over the whole night (core/vhs.ts), off by default on Low
  // (until the player picks one in Settings, the picture follows the graphics level)
  const vhs = new Vhs(renderer, quality === 'high' ? 4 : 0);
  const applyPicture = (p: Picture, chosen = true) => {
    vhs.picture = p;
    if (chosen) store.set('picture', p);
    document.documentElement.classList.toggle('vhs-on', vhs.on);   // the shader has its own grain
  };
  applyPicture(store.get<Picture | null>('picture', null) ?? (quality === 'high' ? 'vhs' : 'off'), false);
  renderer.info.autoReset = false;   // two passes per frame: count both (the draw-call budget)
  function applyQuality(q: Quality) {
    quality = q; store.set('quality', q);
    vhs.setSamples(q === 'high' ? 4 : 0);
    if (store.get<Picture | null>('picture', null) === null) applyPicture(q === 'high' ? 'vhs' : 'off', false);
    setQuality(scene, q);
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
  audio.setVolume(store.get('vol', 0.8));
  audio.preload(); // decodes while the title screen is up
  const input = new Input(renderer.domElement, ui.touch);
  input.sensitivity = store.get('sens', 1);
  const inter = new Interaction();
  const colliders = [...room.colliders, ...yard.colliders, ...annex.colliders];
  const saroZones = [room.bounds, ...yard.zones, ...annex.zones];
  const player = new Player(camera, colliders, saroZones);
  // SARO, the road and STATION 01, and the drive between them (world/World.ts)
  const world = new World({
    scene, camera, player,
    saro: { groups: [ext.group, room.group, yard.group, annex.group], zones: saroZones, colliders, truck: YARD.truck },
    applyQuality: () => setQuality(scene, quality),
    audio: () => ({ ctx: audio.ctx, sfx: audio.sfx }),
    thud: (gain) => audio.play('thudSoft', { gain, rate: 0.8 }),
    fade: (on, text) => ui.fade(on, text ?? ''),
    hold: (on) => { game.cinematic = on; if (on) input.reset(); },
    after: (sec, fn) => game.after(sec, fn),
    toast: (text, secs) => ui.toast(text, secs),
    clock: () => clockText(game.clock, false),
    skipClock: (sec) => { game.clock += sec; },
    touch: () => input.touchMode,
  });
  // footsteps by what is underfoot: vinyl and tiles inside SARO, concrete in the yard and
  // on the motel's walk, dirt between them, boards in the hut, carpet at the motel
  const surfaceAt = (): Surface => {
    const p = player.pos;
    if (world.area === 'room6') return 'carpet';
    if (world.area === 'diner') return world.indoors(p) ? 'tile' : 'dirt';
    if (world.area === 'station01') return world.indoors(p) ? 'wood' : 'dirt';
    if (world.area !== 'saro') return 'dirt';
    if (inside(motelOffice, p)) return 'carpet';
    if (space !== 'yard') return 'tile';
    return player.floorY < -0.3 ? 'dirt' : 'concrete';
  };
  player.onStep = () => audio.step(surfaceAt());
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
    },
    motel: {
      area: () => world.area, room: () => world.room6, court: () => world.court,
      goIn: () => world.goRoom6(), goOut: () => world.leaveRoom6(), brick: () => world.crossing.brick,
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
  world.onStationLoaded = (site) => ch3.bindSite(site);
  world.onArriveStation = () => ch3.arrivedStation();
  world.onArriveSaro = () => ch3.arrivedSaro();
  const ch4 = game.ch4;
  world.onRoom6Loaded = (r) => ch4.bindRoom(r);
  world.onEnterRoom6 = () => ch4.enteredRoom();
  world.onLeaveRoom6 = () => ch4.leftRoom();
  world.initSaro().catch((e) => console.warn('the service truck could not be built', e));

  sky.onThunder = (delay, s) => setTimeout(() => { if (mode === 'play') audio.thunder(s); }, delay * 1000);

  type Mode = 'title' | 'play' | 'end';
  let mode: Mode = 'title';
  let titleEl: HTMLElement | null = null;
  let endEl: HTMLElement | null = null;
  let pausedByMenu = false;
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
  const settings = () => ({
    volume: audio.volume, sens: input.sensitivity,
    onVolume: (v: number) => { audio.setVolume(v); store.set('vol', v); },
    onSens: (v: number) => { input.sensitivity = v; store.set('sens', v); },
    quality, onQuality: (q: Quality) => applyQuality(q),
    picture: vhs.picture, onPicture: (p: Picture) => applyPicture(p),
    invertY, onInvertY: (v: boolean) => { invertY = v; store.set('invertY', v); },
    fov: baseFov, onFov: (v: number) => { baseFov = v; store.set('fov', v); resize(); },
    largeText: document.documentElement.classList.contains('text-large'),
    onLargeText: (v: boolean) => { setLargeText(v); store.set('largeText', v); },
  });

  function openPause() {
    if (pausedByMenu) return;
    pausedByMenu = true;
    audio.suspend(true);
    const resume = () => { pausedByMenu = false; audio.suspend(false); };
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
    if (mode !== 'play' || ui.modal || game.cinematic || world.driving) return;
    if (fcam.raised) { ch1.shutter(); return; } // the camera is up: Use is the shutter
    const it = inter.update(camera);
    if (it) it.use();
  }

  input.onKey = (code) => {
    if (mode !== 'play') return;
    if (code === 'KeyE' || code === 'Enter') {
      if (ui.modal) { if (ui.modal.classList.contains('docview')) ui.close(); return; }
      useCurrent();
    } else if (code === 'Tab' || code === 'KeyN') {
      if (ui.modal) { if (ui.modal.querySelector('.notebook')) ui.close(); return; }
      if (!game.cinematic) game.openNotebook();
    } else if (code === 'Escape' || code === 'KeyP') {
      if (ui.modal) { ui.close(); return; }
      if (fcam.raised) { ch1.toggleCamera(); return; }
      openPause();
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
    if (mode !== 'play' || ui.modal || game.cinematic || world.driving) return;
    if (fcam.raised) { ch1.shutter(); return; }
    const ndc = new THREE.Vector2((x / innerWidth) * 2 - 1, -(y / innerHeight) * 2 + 1);
    const it = inter.pick(camera, ndc, 2.6);
    if (it) it.use();
  };
  ui.onUse = useCurrent;
  ui.onNotes = () => { if (mode === 'play' && !ui.modal && !game.cinematic) game.openNotebook(); };
  ui.onCamera = () => { if (mode === 'play' && !ui.modal && !game.cinematic && !world.driving) ch1.toggleCamera(); };
  ui.onPause = () => { if (mode === 'play' && !ui.modal) openPause(); };

  // ---------- flow ----------
  function showTitle() {
    const cont = saves.continueSave();
    titleEl = ui.title({
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
  async function startGame(o: { caseId: number; playtime: number; state: GameState | null; jump?: string; unlocking?: Promise<void> }) {
    starting = true;
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
    await saves.ready;
    // a save made at the station or in room 6 needs that area built first
    const savedArea = o.state?.area;
    const area = (savedArea === 'station01' || savedArea === 'room6' ? savedArea : 'saro') as AreaId;
    try { await world.prepare(area); } catch (e) { console.error(e); }
    clearTimeout(slow);
    caseId = o.caseId; playtime = o.playtime; saves.setActive(caseId);
    audio.startRoomTone();
    audio.loop('wind', 'wind', { dest: audio.amb, gain: 0.14 });
    audio.loop('crickets', 'crickets', { dest: audio.amb, gain: 0.02 });
    space = 'room';
    world.stopDriving();
    world.enter('saro');
    restoring = o.state; restoringNow = true;
    game.start(o.state?.checkpoint ?? o.jump);
    if (o.state?.doors) doors.restore(o.state.doors); // the doors as they were left
    restoringNow = false; restoring = null;
    world.enter(area === 'station01' && world.site ? 'station01' : area === 'room6' && world.room6 ? 'room6' : 'saro');
    if (world.area === 'station01') world.placeAtStation();
    if (world.area === 'room6') world.placeAtRoom6();
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
    if (audio.ctx) { audio.stop('music', 0.5); audio.stopMotors(); audio.setCarrier(0, 0, 0); audio.signalOff(); audio.tvHiss(null); }
    world.stopDriving();
    game.reset();
    world.enter('saro');
    pendingAuto = false;
  }

  // ---------- saving ----------
  let pendingAuto = false;
  function saveBlock(): string | null {
    if (mode !== 'play' || !caseId) return 'There is no night running to save.';
    return game.saveBlock();
  }
  function placeName() {
    const p = player.pos;
    if (world.area === 'station01') return world.indoors(p) ? 'STATION 01, field hut' : 'STATION 01';
    if (world.area === 'road') return 'Highway south';
    if (world.area === 'room6') return world.indoors(p) ? 'Sierra Motor Court, room 6' : 'Sierra Motor Court';
    if (world.area === 'diner') return world.indoors(p) ? 'Mesa Diner' : 'Mesa Diner, the lot';
    if (inside(motelOffice, p)) return 'Sierra Motor Court, office';
    if (world.crossing.outside(p)) return world.crossing.atMotel(p) ? 'Sierra Motor Court' : 'West lot';
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
    const state: GameState = {
      v: 2, checkpoint, case: game.snapshotCase(), area: world.area, doors: doors.states(),
      pose: prologue ? null : { x: player.pos.x, z: player.pos.z, yaw: player.yaw, pitch: player.pitch },
    };
    return saves.save(caseId, kind, slot, state, {
      chapter: game.chapterTitle(), place: placeName(),
      clock: prologue ? '02:13' : clockText(game.clock, false), playtime, thumb: captureThumb(),
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

  function toTitle() {
    mode = 'title';
    fcam.raise(false); ui.viewfinder(false);
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.close(true);
    ui.showHud(false, input.touchMode);
    endEl?.remove(); endEl = null;
    if (audio.ctx) { audio.stop('music', 1.5); audio.stopMotors(); audio.setCarrier(0, 0, 0); audio.signalOff(); audio.tvHiss(null); }
    world.stopDriving();
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
      ui.chapterCard({ closed: o.closed, recap: o.recap, next: o.next, title: o.title, clock: clockText(game.clock, false) });
      game.after(6.4, () => {
        ui.chapterCard(null);
        ui.fade(false);
        game.cinematic = false;
        input.enabled = true;
        ui.showHud(true, input.touchMode);
      });
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
  // End of chapter four, as far as the night is built.
  ch4.onEnd = () => showCard({
    lines: ch4.endingLines(),
    buttons: [
      { label: 'Return to the observatory', on: () => backToPlay(() => {}) },
      { label: 'Title', on: () => toTitle() },
    ],
    credits: true,
  });

  // test hook, handy from the browser console: S47.jump('countdown')
  (window as any).S47 = {
    art: artStatus,
    jump: (p: string) => {
      if (mode !== 'play') { startGame({ caseId: saves.freeCase() ?? 1, playtime: 0, state: null, jump: p }); return; }
      world.stopDriving(); world.enter('saro'); game.start(p);
    },
    game, room, ext, camera, player, renderer, scene, yard, fcam, ch1, annex, ch2, ch3, ch4, saves, world, doors, sky,
    // write a save now (tests): the frame is drawn first so the save gets its picture
    saveNow: (kind: SaveKind = 'manual', slot: number | 'rotate' = 0) => { draw(); return writeSave(kind, slot); },
    playtime: () => playtime, caseId: () => caseId,
    // true once a started or loaded night is running (tests wait for it after Continue)
    started: () => caseId > 0 && !starting && mode === 'play',
  };

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
  const spaceOf = (p: THREE.Vector3) => indoors.some((b) => inside(b, p)) ? 'room' : inside(yard.zone.lab, p) ? 'lab' : 'yard';
  const hemiBase = room.lights.hemi.intensity;
  const dbg = { hold: false };
  let inCab = false;
  function step(dt: number) {
    t += dt;
    if (mode === 'play') playtime += dt;
    const modalOpen = !!ui.modal;
    if (mode === 'title') { titleCam(t); annex.interior.visible = false; }
    else {
      const active = mode === 'play' && !modalOpen && !game.cinematic;
      const look = input.consumeLook();
      if (world.driving) {
        // the truck: forward and back are throttle and brake, sideways is steering
        const m = active ? input.move() : { x: 0, z: 0 };
        if (!modalOpen) world.update(dt, t, { steer: m.x, throttle: -m.z }, active ? { x: look.x, y: invertY ? -look.y : look.y } : { x: 0, y: 0 });
      } else {
        if (active) {
          player.look(look.x, invertY ? -look.y : look.y);
          const m = input.move();
          player.update(dt, m.x, m.z, m.run);
        } else player.update(dt, 0, 0, false);
        world.update(dt, t, null, { x: 0, y: 0 });
      }
      game.update(dt, t, modalOpen);
      doors.update(dt);
      if (mode === 'play') audio.nightLife(dt, player.pos);
      // in the cab: no crosshair, and the touch buttons for using things and the camera go away
      if (world.driving !== inCab) { inCab = world.driving; document.documentElement.classList.toggle('driving', inCab); }
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
      } else {
        ui.prompt(fcam.raised && input.touchMode ? 'Shutter' : null, input.touchMode);
        ui.hint(false);
      }
    }
    sky.update(dt, t, camera.position);
    sky.setCometClock(game.clock);
    ext.update(dt, t);
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
  const debug = debugOn ? new DebugHud(renderer, () => `${quality}   ${innerWidth}x${innerHeight}   ${mode}${mode === 'play' ? '  ' + game.phase : ''}${game.phase === 'ch1' ? '  ' + ch1.s.stage : ''}${game.phase === 'ch2' ? '  ' + ch2.s.stage : ''}${game.phase === 'ch3' ? '  ' + ch3.s.stage + '  ' + world.area : ''}${game.phase === 'ch4' ? '  ' + ch4.s.stage + '  ' + world.area : ''}`) : null;
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
  Object.assign((window as any).S47, {
    // advance the game by `seconds` in fixed steps, then render one frame
    tick(seconds = 0, fps = 30) { const n = Math.max(1, Math.round(seconds * fps)); for (let i = 0; i < n; i++) step(1 / fps); draw(); flushAutosave(); },
    setQuality: (q: 'high' | 'low') => applyQuality(q),
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
