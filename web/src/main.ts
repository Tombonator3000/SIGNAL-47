import './style.css';
import * as THREE from 'three';
import { UI } from './ui/UI';
import { AudioSys } from './core/Audio';
import { Input } from './core/Input';
import { Interaction } from './core/Interaction';
import { Player } from './player/Player';
import { Sky } from './world/Sky';
import { Exterior } from './world/Exterior';
import { ControlRoom } from './world/ControlRoom';
import { Prologue, type SavedCase } from './story/Prologue';
import { ServiceYard } from './world/ServiceYard';
import { RecordsAnnex } from './world/Annex';
import { FieldCamera } from './core/FieldCamera';
import { setQuality, type Quality } from './core/quality';
import { loadFonts } from './core/fonts';
import { DebugHud, debugOn } from './core/debug';
import { glowScale } from './world/glow';
import { CaseStore } from './core/caseStore';

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
  await Promise.race([loadFonts(), wait(3000)]);
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
  function applyQuality(q: Quality) {
    quality = q; store.set('quality', q);
    setQuality(scene, q);
    maxPR = Math.min(window.devicePixelRatio || 1, q === 'low' ? 1 : (touchGuess ? 1.6 : 2));
    pr = Math.min(pr, maxPR);
    renderer.setPixelRatio(pr);
    resize();
  }

  // the saved case and its photographs (IndexedDB), read while the title screen is up
  const cases = new CaseStore<SavedCase>();
  const audio = new AudioSys();
  audio.setVolume(store.get('vol', 0.8));
  audio.preload(); // decodes while the title screen is up
  const input = new Input(renderer.domElement, ui.touch);
  input.sensitivity = store.get('sens', 1);
  const inter = new Interaction();
  const colliders = [...room.colliders, ...yard.colliders, ...annex.colliders];
  const player = new Player(camera, colliders, [room.bounds, ...yard.zones, ...annex.zones]);
  // footsteps: a little deeper and with some grit out on the concrete
  player.onStep = () => {
    const out = space === 'yard';
    audio.play('step' + Math.floor(Math.random() * 3), { gain: out ? 0.26 : 0.22, rate: (out ? 0.82 : 0.94) + Math.random() * 0.12 });
    if (out) audio.grit(0.045);
  };
  const game = new Prologue({
    ui, audio, room, ext, player, inter, yard, fcam, colliders, annex,
    view: { restore: () => { renderer.setPixelRatio(pr); resize(); }, draw: () => draw() },
    // The case lives in its own key; photographs go to IndexedDB (core/caseStore.ts).
    saveCase: (c) => cases.save(c),
    loadCase: () => cases.load(),
    isTouch: () => input.touchMode,
  });
  game.onCheckpoint = (n) => store.set('checkpoint', n);
  const ch1 = game.ch1;
  const ch2 = game.ch2;

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
    if (document.hidden && mode === 'play' && !pausedByMenu) openPause();
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
    invertY, onInvertY: (v: boolean) => { invertY = v; store.set('invertY', v); },
    fov: baseFov, onFov: (v: number) => { baseFov = v; store.set('fov', v); resize(); },
    largeText: document.documentElement.classList.contains('text-large'),
    onLargeText: (v: boolean) => { setLargeText(v); store.set('largeText', v); },
  });

  function openPause() {
    if (pausedByMenu) return;
    pausedByMenu = true;
    audio.suspend(true);
    ui.pause({
      ...settings(),
      onResume: () => { pausedByMenu = false; audio.suspend(false); },
      onTitle: () => { pausedByMenu = false; audio.suspend(false); toTitle(); },
    });
  }

  function useCurrent() {
    if (mode !== 'play' || ui.modal || game.cinematic) return;
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
      if (!ui.modal && !game.cinematic) ch1.toggleCamera();
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
    if (fcam.raised) { ch1.shutter(); return; }
    const ndc = new THREE.Vector2((x / innerWidth) * 2 - 1, -(y / innerHeight) * 2 + 1);
    const it = inter.pick(camera, ndc, 2.6);
    if (it) it.use();
  };
  ui.onUse = useCurrent;
  ui.onNotes = () => { if (mode === 'play' && !ui.modal && !game.cinematic) game.openNotebook(); };
  ui.onCamera = () => { if (mode === 'play' && !ui.modal && !game.cinematic) ch1.toggleCamera(); };
  ui.onPause = () => { if (mode === 'play' && !ui.modal) openPause(); };

  // ---------- flow ----------
  function showTitle() {
    titleEl = ui.title({
      canContinue: !!store.get<string | null>('checkpoint', null),
      onStart: () => startGame(),
      onContinue: () => startGame(store.get<string | null>('checkpoint', null) ?? undefined),
      onSettings: () => ui.pause({ ...settings(), title: 'Settings', settingsOnly: true, onResume: () => {}, onTitle: () => {} }),
    });
  }

  async function startGame(checkpoint?: string) {
    const unlocking = audio.unlock(); // must start inside the tap/click
    if (input.touchMode) {
      try { const r = document.documentElement.requestFullscreen?.(); if (r) r.catch(() => {}); } catch { /* not allowed here */ }
    }
    titleEl?.remove(); titleEl = null;
    endEl?.remove(); endEl = null;
    ui.fade(true, '');
    mode = 'play';
    input.enabled = true;
    input.requestLock();
    // Normally the sounds are decoded long before anyone taps Start. If not, say so.
    const slow = setTimeout(() => ui.fade(true, 'TUNING RECEIVERS', true), 350);
    await unlocking;
    await cases.ready;
    clearTimeout(slow);
    audio.startRoomTone();
    audio.loop('wind', 'wind', { dest: audio.amb, gain: 0.14 });
    space = 'room';
    game.start(checkpoint);
    ui.showHud(true, input.touchMode);
    setTimeout(() => ui.fade(false), 250);
  }

  function toTitle() {
    mode = 'title';
    fcam.raise(false); ui.viewfinder(false);
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.close(true);
    ui.showHud(false, input.touchMode);
    endEl?.remove(); endEl = null;
    if (audio.ctx) { audio.stop('music', 1.5); audio.stopMotors(); audio.setCarrier(0, 0, 0); }
    game.reset();
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

  // End of the prologue: the night is not over.
  game.onFinish = () => showCard({
    lines: ['Prologue: Night Shift.', 'THE NIGHT IS NOT OVER // CHECK THE LOCAL CONTROLLER'],
    buttons: [
      { label: 'Continue: service yard', on: () => backToPlay(() => game.beginChapter1(null)) },
      { label: 'Title', on: () => toTitle() },
    ],
  });
  // End of chapter one: the night goes on in the records room.
  ch1.onEnd = (method) => showCard({
    lines: ch1.endingLines(method),
    buttons: [
      { label: 'Continue: the reference record', on: () => backToPlay(() => game.beginChapter2(null)) },
      { label: 'Title', on: () => toTitle() },
    ],
  });
  // End of chapter two, as far as the night is built.
  ch2.onEnd = () => showCard({
    lines: ch2.endingLines(),
    buttons: [
      { label: 'Return to the observatory', on: () => backToPlay(() => {}) },
      { label: 'Title', on: () => toTitle() },
    ],
    credits: true,
  });

  // test hook, handy from the browser console: S47.jump('countdown')
  (window as any).S47 = {
    jump: (p: string) => { if (mode !== 'play') startGame(p); else game.start(p); },
    game, room, ext, camera, player, renderer, scene, yard, fcam, ch1, annex, ch2,
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
  function step(dt: number) {
    t += dt;
    const modalOpen = !!ui.modal;
    if (mode === 'title') { titleCam(t); annex.interior.visible = false; }
    else {
      if (mode === 'play' && !modalOpen && !game.cinematic) {
        const look = input.consumeLook();
        player.look(look.x, invertY ? -look.y : look.y);
        const m = input.move();
        player.update(dt, m.x, m.z, m.run);
      } else {
        input.consumeLook();
        player.update(dt, 0, 0, false);
      }
      game.update(dt, t, modalOpen);
      const sp = spaceOf(player.pos);
      if (sp !== space) { space = sp; audio.setSpace(sp); }
      // the extension has no windows: draw its rooms only from inside, or through the open door
      annex.interior.visible = indoors.slice(1).some((b) => inside(b, player.pos))
        || (inside(room.bounds, player.pos) && room.southDoorHinge.rotation.y > 0.01);
      crtAcc += dt;
      if (crtAcc > 1 / 12) { crtAcc = 0; game.drawCrts(t); }
      if (mode === 'play' && !modalOpen && !game.cinematic && !fcam.raised) {
        const it = inter.update(camera);
        ui.prompt(it ? it.label() : null, input.touchMode);
        ui.hint(!input.touchMode && !input.locked);
      } else {
        ui.prompt(fcam.raised && input.touchMode ? 'Shutter' : null, input.touchMode);
        ui.hint(false);
      }
    }
    sky.update(dt, t, camera.position);
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
    glowScale.value = renderer.getDrawingBufferSize(buf).y / (2 * Math.tan(camera.fov * Math.PI / 360));
    ext.dishArray.cull(camera);
    renderer.render(scene, camera);
  }
  const debug = debugOn ? new DebugHud(renderer, () => `${quality}   ${innerWidth}x${innerHeight}   ${mode}${mode === 'play' ? '  ' + game.phase : ''}${game.phase === 'ch1' ? '  ' + ch1.s.stage : ''}${game.phase === 'ch2' ? '  ' + ch2.s.stage : ''}`) : null;
  function frame(now: number) {
    requestAnimationFrame(frame);
    const raw = (now - last) / 1000; last = now;
    const dt = Math.min(0.05, raw);
    if (pausedByMenu || dbg.hold) return;
    if (mode === 'end') return; // the end card covers the whole screen, so let the GPU rest
    step(dt);
    draw();
    adapt(dt);
    debug?.frame(raw);
  }
  Object.assign((window as any).S47, {
    set hold(v: boolean) { dbg.hold = v; },
    get hold() { return dbg.hold; },
    // advance the game by `seconds` in fixed steps, then render one frame
    tick(seconds = 0, fps = 30) { const n = Math.max(1, Math.round(seconds * fps)); for (let i = 0; i < n; i++) step(1 / fps); draw(); },
    setQuality: (q: 'high' | 'low') => applyQuality(q),
  });

  applyQuality(quality);
  resize();
  titleCam(0);
  renderer.render(scene, camera);
  loading.remove();
  showTitle();
  requestAnimationFrame(frame);
}

boot();
