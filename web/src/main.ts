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
import { Prologue } from './story/Prologue';
import { setQuality, type Quality } from './core/quality';
import { loadFonts } from './core/fonts';

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
  scene.add(sky.group, ext.group, room.group);

  let quality: Quality = store.get<Quality>('quality', 'high');
  function applyQuality(q: Quality) {
    quality = q; store.set('quality', q);
    setQuality(scene, q);
    maxPR = Math.min(window.devicePixelRatio || 1, q === 'low' ? 1 : (touchGuess ? 1.6 : 2));
    pr = Math.min(pr, maxPR);
    renderer.setPixelRatio(pr);
    resize();
  }

  const audio = new AudioSys();
  audio.setVolume(store.get('vol', 0.8));
  const input = new Input(renderer.domElement, ui.touch);
  input.sensitivity = store.get('sens', 1);
  const inter = new Interaction();
  const player = new Player(camera, room.colliders, room.bounds);
  player.onStep = () => audio.play('step' + Math.floor(Math.random() * 3), { gain: 0.22, rate: 0.94 + Math.random() * 0.12 });
  const game = new Prologue({ ui, audio, room, ext, player, inter });
  game.onCheckpoint = (n) => store.set('checkpoint', n);

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

  function openPause() {
    if (pausedByMenu) return;
    pausedByMenu = true;
    audio.suspend(true);
    ui.pause({
      volume: audio.volume, sens: input.sensitivity,
      onVolume: (v) => { audio.setVolume(v); store.set('vol', v); },
      onSens: (v) => { input.sensitivity = v; store.set('sens', v); },
      quality, onQuality: (q) => applyQuality(q),
      onResume: () => { pausedByMenu = false; audio.suspend(false); },
      onTitle: () => { pausedByMenu = false; audio.suspend(false); toTitle(); },
    });
  }

  function useCurrent() {
    if (mode !== 'play' || ui.modal || game.cinematic) return;
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
      openPause();
    }
  };
  renderer.domElement.addEventListener('click', () => {
    if (mode !== 'play' || ui.modal || input.touchMode) return;
    if (!input.locked) input.requestLock(); else useCurrent();
  });
  input.onTap = (x, y) => {
    if (mode !== 'play' || ui.modal || game.cinematic) return;
    const ndc = new THREE.Vector2((x / innerWidth) * 2 - 1, -(y / innerHeight) * 2 + 1);
    const it = inter.pick(camera, ndc, 2.6);
    if (it) it.use();
  };
  ui.onUse = useCurrent;
  ui.onNotes = () => { if (mode === 'play' && !ui.modal && !game.cinematic) game.openNotebook(); };
  ui.onPause = () => { if (mode === 'play' && !ui.modal) openPause(); };

  // ---------- flow ----------
  function showTitle() {
    titleEl = ui.title({
      canContinue: !!store.get<string | null>('checkpoint', null),
      onStart: () => startGame(),
      onContinue: () => startGame(store.get<string | null>('checkpoint', null) ?? undefined),
      onSettings: () => ui.pause({
        title: 'Settings', settingsOnly: true, volume: audio.volume, sens: input.sensitivity,
        onVolume: (v) => { audio.setVolume(v); store.set('vol', v); },
        onSens: (v) => { input.sensitivity = v; store.set('sens', v); },
        quality, onQuality: (q) => applyQuality(q),
        onResume: () => {}, onTitle: () => {},
      }),
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
    await unlocking;
    audio.startRoomTone();
    audio.loop('wind', 'wind', { dest: audio.amb, gain: 0.14 });
    game.start(checkpoint);
    ui.showHud(true, input.touchMode);
    setTimeout(() => ui.fade(false), 250);
  }

  function toTitle() {
    mode = 'title';
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

  game.onFinish = () => {
    mode = 'end';
    input.enabled = false; input.reset();
    if (input.locked) { releasingLock = true; input.releaseLock(); }
    ui.showHud(false, input.touchMode);
    audio.loop('music', 'titleMusic', { dest: audio.music, gain: 0.9 });
    endEl = ui.endcard(() => { audio.stop('music', 0.8); endEl?.remove(); endEl = null; startGame(); }, () => toTitle());
    setTimeout(() => ui.fade(false), 400);
  };

  // test hook, handy from the browser console: S47.jump('countdown')
  (window as any).S47 = {
    jump: (p: string) => { if (mode !== 'play') startGame(p); else game.start(p); },
    game, room, ext, camera, player, renderer, scene,
  };

  // ---------- resize and adaptive resolution ----------
  function resize() {
    const w = innerWidth, h = innerHeight;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.fov = w < h ? 82 : 70;
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
  const hemiBase = room.lights.hemi.intensity;
  const dbg = { hold: false };
  function step(dt: number) {
    t += dt;
    const modalOpen = !!ui.modal;
    if (mode === 'title') titleCam(t);
    else {
      if (mode === 'play' && !modalOpen && !game.cinematic) {
        const look = input.consumeLook();
        player.look(look.x, look.y);
        const m = input.move();
        player.update(dt, m.x, m.z, m.run);
      } else {
        input.consumeLook();
        player.update(dt, 0, 0, false);
      }
      game.update(dt, t, modalOpen);
      crtAcc += dt;
      if (crtAcc > 1 / 12) { crtAcc = 0; game.drawCrts(t); }
      if (mode === 'play' && !modalOpen && !game.cinematic) {
        const it = inter.update(camera);
        ui.prompt(it ? it.label() : null, input.touchMode);
        ui.hint(!input.touchMode && !input.locked);
      } else { ui.prompt(null, input.touchMode); ui.hint(false); }
    }
    sky.update(dt, t, camera.position);
    ext.update(dt, t);
    room.lights.hemi.intensity = hemiBase + sky.uniforms.uFlash.value * 2.4;
    audio.listener(camera);
  }
  function draw() { renderer.render(scene, camera); }
  function frame(now: number) {
    requestAnimationFrame(frame);
    const dt = Math.min(0.05, (now - last) / 1000); last = now;
    if (pausedByMenu || dbg.hold) return;
    step(dt);
    draw();
    adapt(dt);
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
