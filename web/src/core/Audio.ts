import * as THREE from 'three';
import { SignalVoice } from './signalVoice';
import { Ambience } from './ambience';
import { PHONE_TEXTS, PHONE_CALLS, NORA_CB_TEXTS, type PhoneCue } from './phoneScripts';
import { normalizeReadingText } from './readAloud';
import voiceCatalogSource from '../assets/audio/kinski/catalog.json?raw';
import ensembleCatalogSource from '../assets/audio/ensemble/catalog.json?raw';
import phoneRing from '../assets/audio/phone_ring.mp3';
import printer from '../assets/audio/printer.mp3';
import ceramic from '../assets/audio/ceramic.mp3';
import wind from '../assets/audio/wind.mp3';
import titleMusic from '../assets/audio/title_music.mp3';
// The night outside, doors and paper: CC0 recordings from Freesound, by way of Tom's own
// projects (morbidium, Loincloth-Legends). The steps are Kenney's Impact Sounds (CC0), five
// of each kind (Tom, 16 October: the old ones were tiresome). See THIRD_PARTY_NOTICES.md.
import crickets from '../assets/audio/crickets.mp3';
import owl from '../assets/audio/owl.mp3';
import dog0 from '../assets/audio/dog0.mp3';
import dog1 from '../assets/audio/dog1.mp3';
import gust from '../assets/audio/gust.mp3';
import doorCreak0 from '../assets/audio/door_creak0.mp3';
import doorCreak1 from '../assets/audio/door_creak1.mp3';
import doorMetal from '../assets/audio/door_metal.mp3';
import doorClose from '../assets/audio/door_close.mp3';
import paper0 from '../assets/audio/paper0.mp3';
import paper1 from '../assets/audio/paper1.mp3';
import click from '../assets/audio/click.mp3';
import sw from '../assets/audio/switch.mp3';
import thudSoft from '../assets/audio/thud_soft.mp3';
// the truck's start and shut-off (drive/engineSound.ts), also heard far off on a phone line
import engineStart from '../assets/audio/engine_start.mp3';
import engineOff from '../assets/audio/engine_off.mp3';

// the steps, by name: step_concrete0 becomes stepConcrete0
const STEP_FILES = import.meta.glob<string>('../assets/audio/step_*.mp3', { eager: true, import: 'default' });
const stepSources = Object.fromEntries(Object.entries(STEP_FILES).map(([path, url]) => {
  const m = /step_([a-z]+)(\d)\.mp3$/.exec(path)!;
  return [`step${m[1][0].toUpperCase()}${m[1].slice(1)}${m[2]}`, url];
}));

// Out in the dark (core/calls.ts): six short sounds, Freesound originals from the credits of
// They Are Here, CC0 and CC BY (THIRD_PARTY_NOTICES.md); alien_voice becomes alienVoice
const ALIEN_FILES = import.meta.glob<string>('../assets/audio/alien_*.mp3', { eager: true, import: 'default' });
const alienSources = Object.fromEntries(Object.entries(ALIEN_FILES).map(([path, url]) => {
  const m = /alien_([a-z]+)\.mp3$/.exec(path)!;
  return [`alien${m[1][0].toUpperCase()}${m[1].slice(1)}`, url];
}));

// Only the 24 telephone lines join the ordinary title-screen preload. Document
// narration keeps its separate bounded, lazy cache in ReadAloud.
const PHONE_FILES = import.meta.glob<string>('../assets/audio/kinski/*.mp3', { eager: true, query: '?url', import: 'default' });
const voiceCatalog = JSON.parse(voiceCatalogSource) as { clips: { id: string; text: string; file: string }[] };
const phoneTexts = new Set(PHONE_TEXTS.map(normalizeReadingText));
const phoneKeys = new Map<string, string>();
const phoneSources: Record<string, string> = {};
for (const clip of voiceCatalog.clips) {
  const text = normalizeReadingText(clip.text), url = PHONE_FILES[`../assets/audio/kinski/${clip.file}`];
  if (!phoneTexts.has(text) || !url) continue;
  const key = `phone:${clip.id}`;
  phoneKeys.set(text, key); phoneSources[key] = url;
}
const CHARACTER_FILES = import.meta.glob<string>('../assets/audio/ensemble/*.mp3', { eager: true, query: '?url', import: 'default' });
const ensembleCatalog = JSON.parse(ensembleCatalogSource) as { clips: { id: string; voice: string; text: string; file: string }[] };
const channelTexts = new Set([...PHONE_CALLS.nora, ...PHONE_CALLS.fieldNora].filter((cue) => !cue.aside)
  .map((cue) => normalizeReadingText(cue.text)).concat(NORA_CB_TEXTS.map(normalizeReadingText)));
const characterKeys = new Map<string, string>(), characterSources: Record<string, string> = {};
for (const clip of ensembleCatalog.clips) {
  const text = normalizeReadingText(clip.text);
  if (!(clip.voice === 'bacall' && channelTexts.has(text)
    || clip.voice === 'price' && text === 'Reference west. No. East. Hold the last reading.')) continue;
  const url = CHARACTER_FILES[`../assets/audio/ensemble/${clip.file}`];
  if (!url) continue;
  const key = `character:${clip.id}`;
  characterKeys.set(`${clip.voice}\0${text}`, key); characterSources[key] = url;
}

// Bought sounds (Ultimate SFX for Games, Function Loops), cut and levelled from the packs. Their
// licence does not allow the files in this public repo, so they live in the private repo
// SIGNAL-47-assets, cloned to src/assets/licensed (its sfx/MANIFEST.md says what each is for).
// A file that is there takes over its slot or adds to it (or(), lic()); without the folder the
// game sounds as it did. sfx/door_heavy.mp3 becomes lic:door_heavy.
const LICENSED = import.meta.glob<string>('../assets/licensed/sfx/*.{mp3,wav,ogg}', { eager: true, query: '?url', import: 'default' });
/** The bought sounds are in this build (the credits name them only then). */
export const BOUGHT_SFX = Object.keys(LICENSED).length > 0;
const licSources = Object.fromEntries(Object.entries(LICENSED).map(([path, url]) => [`lic:${/([^/]+)\.\w+$/.exec(path)![1]}`, url]));
/** A bought sound's level against the one it stands in for, at the same call: the files are cut
 *  to match, but a click that would clip at the old level, or a crackle, is raised here. */
const LIC_GAIN: Record<string, number> = { 'lic:rack_switch': 1.6, 'lic:small_switch': 2.2, 'lic:radio_crackle_long': 4.2, 'lic:radio_crackle_short': 3 };
/** The bought static against white noise, through the radios' band-pass: x5.8 to x6.4 for the same
 *  RMS, x5 to x5.5 for the same loudness (it is brighter inside the band). */
const STATIC_MAKEUP = 5.5;

const SOURCES: Record<string, string> = {
  phoneRing, printer, ceramic, wind, titleMusic, click, switch: sw, thudSoft,
  crickets, owl, dog0, dog1, gust, ...stepSources, ...alienSources,
  doorCreak0, doorCreak1, doorMetal, doorClose, paper0, paper1, engineStart, engineOff,
  ...phoneSources, ...characterSources, ...licSources,
};

export interface PhoneCallTimeline {
  duration: number;
  cues: { at: number; duration: number; text: string; by?: string; aside?: boolean }[];
}

// What the player walks on, and how a step on it sounds: which recordings, how loud, the
// pitch, and a low-pass to take the edge off. Quieter than before and never the same
// recording twice running; the desert's dirt and gravel are Kenney's snow steps a little
// deeper, which crunch like gravel.
export type Surface = 'tile' | 'concrete' | 'dirt' | 'wood' | 'carpet';
const five = (kind: string) => [0, 1, 2, 3, 4].map((i) => `step${kind}${i}`);
const STEPS: Record<Surface, { names: string[]; gain: number; rate: number; lp?: number }> = {
  tile: { names: five('Concrete'), gain: 0.15, rate: 1.08, lp: 6000 },
  concrete: { names: five('Concrete'), gain: 0.18, rate: 0.96, lp: 5000 },
  dirt: { names: five('Gravel'), gain: 0.16, rate: 0.88, lp: 3800 },
  wood: { names: five('Wood'), gain: 0.17, rate: 1.0, lp: 5000 },
  carpet: { names: five('Carpet'), gain: 0.15, rate: 0.95, lp: 2200 },
};

function decodeDataUrl(url: string): ArrayBuffer {
  const b64 = url.slice(url.indexOf(',') + 1);
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out.buffer;
}

export class AudioSys {
  ctx: AudioContext | null = null;
  master!: GainNode; sfx!: GainNode; music!: GainNode; amb!: GainNode;
  /** The radios: the truck's, the diner's, the one on the filing cabinets (its own level). */
  radioBus!: GainNode;
  buf: Record<string, AudioBuffer> = {};
  private noise!: AudioBuffer; private brown!: AudioBuffer;
  volume = 0.8;
  ready = false;
  private loops = new Map<string, { src: AudioBufferSourceNode; gain: GainNode }>();
  carrier: { osc: OscillatorNode; gain: GainNode; stat: GainNode } | null = null;
  signal: SignalVoice | null = null;   // the anomaly as it comes in (core/signalVoice.ts)
  roomTone: GainNode | null = null;
  motor: { gain: GainNode; osc: OscillatorNode } | null = null;

  private loading: Promise<void> | null = null;
  kinskiCalls = false;
  characterVoices = false;
  characterBuffer(voice: 'bacall' | 'price', text: string) {
    if (!this.characterVoices) return undefined;
    const key = characterKeys.get(`${voice}\0${normalizeReadingText(text)}`);
    return key ? this.buf[key] : undefined;
  }
  onPhoneCall?: () => void;
  get phonePlaying() { return this.phoneSession !== null; }
  private phoneSession: { sources: AudioBufferSourceNode[]; nodes: AudioNode[] } | null = null;

  // Decode every sound while the title screen is up, so Start does not wait on it.
  // An OfflineAudioContext needs no user gesture and logs no autoplay warning. Its
  // buffers play fine in the real context that unlock() creates later.
  preload() {
    if (this.loading) return this.loading;
    const O = window.OfflineAudioContext || (window as any).webkitOfflineAudioContext;
    let dec: BaseAudioContext;
    try { dec = new O(1, 1, 48000); } catch { return null; }
    this.loading = this.decodeAll(dec);
    return this.loading;
  }

  private async decodeAll(ctx: BaseAudioContext) {
    await Promise.all(Object.entries(SOURCES).map(async ([k, url]) => {
      if (this.buf[k]) return;
      try {
        const data = url.startsWith('data:') ? decodeDataUrl(url) : await (await fetch(url)).arrayBuffer();
        this.buf[k] = await ctx.decodeAudioData(data);
      } catch (err) { console.warn('audio load failed', k, err); }
    }));
  }

  async unlock() {
    if (this.ctx) { if (this.ctx.state !== 'running') await this.ctx.resume(); return; }
    const C = window.AudioContext || (window as any).webkitAudioContext;
    // larger buffers than the default ('interactive', about 10 ms): with a heavy picture on the
    // same machine the small ones run dry and the sound drops out (Tom, 16 October). Nothing
    // here needs the lowest latency; the footsteps are a few tens of ms later at most.
    let ctx: AudioContext;
    try { ctx = new C({ latencyHint: 'playback' }); } catch { ctx = new C(); }
    this.ctx = ctx;
    this.master = ctx.createGain(); this.master.gain.value = this.volume; this.master.connect(ctx.destination);
    this.sfx = ctx.createGain(); this.sfx.connect(this.master);
    this.music = ctx.createGain(); this.music.gain.value = this.musicLevel; this.music.connect(this.master);
    this.amb = ctx.createGain(); this.amb.gain.value = 1; this.amb.connect(this.master);
    this.radioBus = ctx.createGain(); this.radioBus.gain.value = this.radioLevel; this.radioBus.connect(this.master);
    this.noise = this.makeNoise(false); this.brown = this.makeNoise(true);
    // A silent blip satisfies iOS's "sound must start inside the gesture" rule.
    const b = ctx.createBufferSource(); b.buffer = ctx.createBuffer(1, 1, 22050); b.connect(ctx.destination); b.start();
    if (this.loading) await this.loading;
    await this.decodeAll(ctx); // anything the preload missed
    this.ready = true;
  }

  setVolume(v: number) { this.volume = v; if (this.master) this.master.gain.value = v; }
  /** The music's own level (the pause menu), 0..1. */
  musicLevel = 0.7;
  setMusic(v: number) { this.musicLevel = v; if (this.music) this.music.gain.value = v; }
  /** The radios' own level (the pause menu), 0..1. */
  radioLevel = 1;
  setRadio(v: number) { this.radioLevel = v; if (this.radioBus) this.radioBus.gain.value = v; }

  private makeNoise(brown: boolean) {
    const ctx = this.ctx!;
    const len = ctx.sampleRate * 3;
    const b = ctx.createBuffer(1, len, ctx.sampleRate);
    const d = b.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) {
      const w = Math.random() * 2 - 1;
      if (brown) { last = (last + 0.02 * w) / 1.02; d[i] = last * 3.5; } else d[i] = w;
    }
    return b;
  }

  listener(cam: THREE.Camera) {
    const ctx = this.ctx; if (!ctx) return;
    const l = ctx.listener as any;
    const p = cam.position;
    const f = new THREE.Vector3(0, 0, -1).applyQuaternion(cam.quaternion);
    const u = new THREE.Vector3(0, 1, 0).applyQuaternion(cam.quaternion);
    if (l.positionX) {
      const t = ctx.currentTime;
      l.positionX.setTargetAtTime(p.x, t, 0.02); l.positionY.setTargetAtTime(p.y, t, 0.02); l.positionZ.setTargetAtTime(p.z, t, 0.02);
      l.forwardX.setTargetAtTime(f.x, t, 0.02); l.forwardY.setTargetAtTime(f.y, t, 0.02); l.forwardZ.setTargetAtTime(f.z, t, 0.02);
      l.upX.setTargetAtTime(u.x, t, 0.02); l.upY.setTargetAtTime(u.y, t, 0.02); l.upZ.setTargetAtTime(u.z, t, 0.02);
    } else { l.setPosition(p.x, p.y, p.z); l.setOrientation(f.x, f.y, f.z, u.x, u.y, u.z); }
  }

  private panner(pos: THREE.Vector3, ref = 1.2, dest: AudioNode = this.sfx) {
    const p = this.ctx!.createPanner();
    p.panningModel = 'equalpower'; p.distanceModel = 'inverse'; p.refDistance = ref; p.rolloffFactor = 1.2; p.maxDistance = 60;
    if ((p as any).positionX) { p.positionX.value = pos.x; p.positionY.value = pos.y; p.positionZ.value = pos.z; } else (p as any).setPosition(pos.x, pos.y, pos.z);
    p.connect(dest);
    return p;
  }

  /** from, dur: only that part of the recording (seconds into it, how long). */
  play(name: string, o: { gain?: number; rate?: number; at?: THREE.Vector3; when?: number; dest?: AudioNode; loop?: boolean; lp?: number; from?: number; dur?: number } = {}) {
    const ctx = this.ctx; const b = this.buf[name];
    if (!ctx || !b) return null;
    const src = ctx.createBufferSource(); src.buffer = b; src.playbackRate.value = o.rate ?? 1; src.loop = !!o.loop;
    const g = ctx.createGain(); g.gain.value = (o.gain ?? 1) * (LIC_GAIN[name] ?? 1);
    if (o.lp) { const f = ctx.createBiquadFilter(); f.type = 'lowpass'; f.frequency.value = o.lp; src.connect(f); f.connect(g); } else src.connect(g);
    g.connect(o.dest ?? (o.at ? this.panner(o.at) : this.sfx));
    src.start(ctx.currentTime + (o.when ?? 0), o.from ?? 0, o.dur);
    return { src, gain: g };
  }
  /** The bought sound `name` (sfx/<name>.*) to play() in place of `old`, if its file is there;
   *  otherwise `old` (an empty name plays nothing: a sound only the bought files have). */
  or(name: string, old = '') { return this.buf[`lic:${name}`] ? `lic:${name}` : old; }
  /** A bought sound's buffer, for the slots that build their own nodes; undefined without it. */
  lic(name: string): AudioBuffer | undefined { return this.buf[`lic:${name}`]; }

  loop(key: string, name: string, o: { gain?: number; at?: THREE.Vector3; dest?: AudioNode; rate?: number } = {}) {
    this.stop(key);
    const h = this.play(name, { ...o, loop: true });
    if (h) this.loops.set(key, h);
    return h;
  }
  stop(key: string, fade = 0.05) {
    const h = this.loops.get(key); if (!h || !this.ctx) return;
    const t = this.ctx.currentTime;
    h.gain.gain.setTargetAtTime(0, t, fade);
    h.src.stop(t + fade * 6);
    this.loops.delete(key);
  }

  // ---------- Synth ----------
  private noiseSrc(brown = false, loop = true) {
    const s = this.ctx!.createBufferSource(); s.buffer = brown ? this.brown : this.noise; s.loop = loop; return s;
  }
  /** The static of an AM set at night, looping and already started: the bought hiss with its
   *  random crackle when it is there (frying, the old foley stand-in for static; from a random
   *  point, so two radios are not in step), otherwise white noise. `out` has the level the white
   *  noise had through the radios' band-pass. Null until unlock(). */
  staticSrc(): { src: AudioBufferSourceNode; out: AudioNode } | null {
    const ctx = this.ctx; if (!ctx || !this.noise) return null;
    const b = this.lic('radio_static'), src = ctx.createBufferSource(); src.buffer = b ?? this.noise; src.loop = true;
    if (!b) { src.start(); return { src, out: src }; }
    const g = ctx.createGain(); g.gain.value = STATIC_MAKEUP; src.connect(g);
    src.start(0, Math.random() * b.duration);
    return { src, out: g };
  }
  /** The deep pressure of the time away (story/abduction.ts) and of THE EVENT (story/Chapter6.ts),
   *  the same in both so it is known again: a bought low bed, looping, not placed, at gain 0 for
   *  the caller to raise. Null without the file (then the synth tone is all there is). stop(fade):
   *  0 cuts it at once (before the sound is suspended, so it is not there on waking). */
  pressure(): { gain: GainNode; stop: (fade?: number) => void } | null {
    const ctx = this.ctx, b = this.lic('pressure'); if (!ctx || !b) return null;
    const src = ctx.createBufferSource(); src.buffer = b; src.loop = true;
    const gain = ctx.createGain(); gain.gain.value = 0; src.connect(gain); gain.connect(this.sfx); src.start();
    return { gain, stop: (fade = 0) => {
      const t = ctx.currentTime;
      if (fade <= 0) { try { src.stop(); } catch { /* stopped */ } gain.disconnect(); return; }
      gain.gain.cancelScheduledValues(t); gain.gain.setTargetAtTime(0, t, fade); src.stop(t + fade * 6);
    } };
  }

  startRoomTone() {
    const ctx = this.ctx; if (!ctx || this.roomTone) return;
    const g = ctx.createGain(); g.gain.value = 0.0; g.connect(this.amb);
    const n = this.noiseSrc(true); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 420;
    const ng = ctx.createGain(); ng.gain.value = 0.22; n.connect(lp); lp.connect(ng); ng.connect(g); n.start();
    const hum = ctx.createOscillator(); hum.frequency.value = 60; const hg = ctx.createGain(); hg.gain.value = 0.035; hum.connect(hg); hg.connect(g); hum.start();
    const hum2 = ctx.createOscillator(); hum2.frequency.value = 120; const hg2 = ctx.createGain(); hg2.gain.value = 0.02; hum2.connect(hg2); hg2.connect(g); hum2.start();
    g.gain.setTargetAtTime(0.5, ctx.currentTime, 1.5);
    this.roomTone = g;
  }

  // Receiver tone: a sine whose loudness follows lock quality, plus band noise.
  startCarrier() {
    const ctx = this.ctx; if (!ctx || this.carrier) return;
    const osc = ctx.createOscillator(); osc.type = 'sine'; osc.frequency.value = 760;
    const gain = ctx.createGain(); gain.gain.value = 0;
    osc.connect(gain); gain.connect(this.sfx); osc.start();
    const n = this.noiseSrc(false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1800; bp.Q.value = 0.6;
    const stat = ctx.createGain(); stat.gain.value = 0; n.connect(bp); bp.connect(stat); stat.connect(this.sfx); n.start();
    this.carrier = { osc, gain, stat };
  }
  /** The anomaly near 1420.405 as the receiver hears it: clarity 0 (static) to 1 (locked). */
  signalFrame(clarity: number, level: number, t: number) {
    if (!this.ctx || !this.noise) return;
    if (!this.signal) {
      if (clarity <= 0 || level <= 0) return;
      this.signal = new SignalVoice(this.ctx, this.sfx, this.noise);
    }
    this.signal.frame(clarity, level, t);
  }
  signalOff() { this.signal?.stop(); this.signal = null; }
  /** White noise, for synths elsewhere (core/decoder.ts). Null until unlock(). */
  noiseBuffer(): AudioBuffer | null { return this.ctx ? this.noise : null; }
  /** A point in the world that sounds can be sent to (core/walkie.ts). */
  spatial(at: THREE.Vector3, ref = 1.0, dest?: AudioNode): AudioNode | null { return this.ctx ? this.panner(at, ref, dest) : null; }

  // ---------- the radio on the filing cabinets ----------
  // AM at night: a station a long way off that fades in and out of the static (skywave).
  // The tune is made here, a slow three-chord waltz on a tinny little speaker; no
  // recording, so no licence. amRadioJam() drops it into the hiss while the line is dead.
  private am: { out: GainNode; music: GainNode; hiss: GainNode; stop: () => void } | null = null;
  /** The radio on the filing cabinets is playing (the music waits for it). */
  get radioOn() { return !!this.am; }
  amRadio(at: THREE.Vector3 | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (!at) {
      const r = this.am; if (!r) return;
      this.am = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.08);
      setTimeout(() => r.stop(), 900);
      return;
    }
    if (this.am) return;
    const sr = ctx.sampleRate, secs = 19.2, buf = ctx.createBuffer(1, Math.floor(sr * secs), sr), d = buf.getChannelData(0);
    // G, C, D, G in 3/4 at 100 bpm: bass on one, chord on two and three, a little melody over it
    const hz = (n: number) => 440 * Math.pow(2, (n - 69) / 12);
    const bars: [number, number[], number[]][] = [[43, [59, 62, 67], [71, 74, 71]], [48, [60, 64, 67], [72, 71, 69]], [50, [62, 66, 69], [69, 66, 62]], [43, [59, 62, 67], [67, 0, 0]]];
    const beat = 0.6;
    const tone = (f: number, t0: number, dur: number, amp: number, harm: number) => {
      const i0 = Math.floor(t0 * sr), n = Math.floor(dur * sr);
      for (let i = 0; i < n && i0 + i < d.length; i++) {
        const t = i / sr, env = Math.min(1, t / 0.01) * Math.exp(-t * 3.2);
        const ph = 2 * Math.PI * f * t;
        d[i0 + i] += amp * env * (Math.sin(ph) + harm * Math.sin(2 * ph) + harm * 0.5 * Math.sin(3 * ph));
      }
    };
    for (let rep = 0; rep < 2; rep++) bars.forEach(([bass, chord, mel], b) => {
      const t0 = (rep * 4 + b) * 3 * beat;
      tone(hz(bass), t0, beat * 1.2, 0.22, 0.4);
      for (const k of [1, 2]) for (const n of chord) tone(hz(n), t0 + k * beat, beat * 0.8, 0.06, 0.6);
      mel.forEach((n, k) => { if (n) tone(hz(n + (rep ? 0 : 0)), t0 + k * beat, beat * 1.4, 0.1, 0.3); });
    });
    const out = ctx.createGain(); out.gain.value = 0;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1300; bp.Q.value = 0.7;
    out.connect(bp); bp.connect(this.panner(at, 0.8, this.radioBus));
    const src = ctx.createBufferSource(); src.buffer = buf; src.loop = true;
    const music = ctx.createGain(); music.gain.value = 0.8;
    const fade = ctx.createGain(); fade.gain.value = 0.6;
    const lfo = ctx.createOscillator(); lfo.frequency.value = 0.07;
    const lg = ctx.createGain(); lg.gain.value = 0.4; lfo.connect(lg); lg.connect(fade.gain);
    src.connect(music); music.connect(fade); fade.connect(out);
    const n = this.staticSrc()!; const hiss = ctx.createGain(); hiss.gain.value = 0.18; n.out.connect(hiss); hiss.connect(out);
    for (const x of [src, lfo]) x.start();
    out.gain.setTargetAtTime(0.11, ctx.currentTime, 0.3);
    this.am = { out, music, hiss, stop: () => { for (const x of [src, lfo, n.src]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }
  amRadioJam(on: boolean) {
    const ctx = this.ctx, r = this.am; if (!ctx || !r) return;
    const t = ctx.currentTime;
    r.music.gain.setTargetAtTime(on ? 0 : 0.8, t, on ? 0.4 : 1.2);
    r.hiss.gain.setTargetAtTime(on ? 0.5 : 0.18, t, 0.4);
  }

  setCarrier(level: number, pitchOffset: number, staticLevel: number) {
    if (!this.carrier || !this.ctx) return;
    const t = this.ctx.currentTime;
    this.carrier.gain.gain.setTargetAtTime(level, t, 0.015);
    this.carrier.osc.frequency.setTargetAtTime(760 * (1 + pitchOffset), t, 0.05);
    this.carrier.stat.gain.setTargetAtTime(staticLevel, t, 0.08);
  }

  beep(freq = 1200, dur = 0.08, gain = 0.12, when = 0, type: OscillatorType = 'square') {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const o = ctx.createOscillator(); o.type = type; o.frequency.value = freq;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(gain, t + 0.005); g.gain.setValueAtTime(gain, t + dur); g.gain.linearRampToValueAtTime(0, t + dur + 0.02);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3000;
    o.connect(lp); lp.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + dur + 0.05);
  }

  boom(strength = 1, dest?: AudioNode, when = 0) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const out = dest ?? this.sfx;
    const o = ctx.createOscillator(); o.type = 'sine';
    o.frequency.setValueAtTime(70, t); o.frequency.exponentialRampToValueAtTime(24, t + 1.6);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.9 * strength, t + 0.02); g.gain.exponentialRampToValueAtTime(0.001, t + 2.6);
    o.connect(g); g.connect(out); o.start(t); o.stop(t + 2.8);
    const n = this.noiseSrc(true, false); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.setValueAtTime(900, t); lp.frequency.exponentialRampToValueAtTime(80, t + 2.2);
    const ng = ctx.createGain(); ng.gain.setValueAtTime(0, t); ng.gain.linearRampToValueAtTime(1.4 * strength, t + 0.015); ng.gain.exponentialRampToValueAtTime(0.001, t + 2.8);
    n.connect(lp); lp.connect(ng); ng.connect(out); n.start(t); n.stop(t + 3);
  }

  // Thunder far away over the mesa: no crack, only rolls that come in slowly, darker and
  // later the further off it is, after the far-thunder layers in Tom's Loincloth-Legends
  // (thunderSyn with `far`), with a low sine swell under them.
  thunder(strength = 0.6) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const roll = (at: number, dur: number, f0: number, f1: number, v: number) => {
      const n = this.noiseSrc(true, false); const lp = ctx.createBiquadFilter(); lp.type = 'lowpass';
      lp.frequency.setValueAtTime(f0, t + at); lp.frequency.exponentialRampToValueAtTime(f1, t + at + dur);
      const g = ctx.createGain(); g.gain.setValueAtTime(0, t + at); g.gain.linearRampToValueAtTime(v, t + at + dur * 0.28); g.gain.exponentialRampToValueAtTime(0.001, t + at + dur);
      n.connect(lp); lp.connect(g); g.connect(this.amb); n.start(t + at); n.stop(t + at + dur + 0.1);
    };
    const s = strength;
    roll(0, 3.4, 360, 55, 0.5 * s);
    roll(0.8 + Math.random() * 0.7, 2.6, 280, 45, 0.38 * s);
    if (Math.random() < 0.7) roll(2.1 + Math.random() * 1.2, 3.2, 220, 38, 0.3 * s);
    const o = ctx.createOscillator(); o.frequency.setValueAtTime(44, t + 0.3); o.frequency.exponentialRampToValueAtTime(30, t + 2.8);
    const og = ctx.createGain(); og.gain.setValueAtTime(0, t + 0.3); og.gain.linearRampToValueAtTime(0.28 * s, t + 1.0); og.gain.exponentialRampToValueAtTime(0.001, t + 3.0);
    o.connect(og); og.connect(this.amb); o.start(t + 0.3); o.stop(t + 3.1);
  }

  gasp(dest?: AudioNode, when = 0) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime + when;
    const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 1.8;
    bp.frequency.setValueAtTime(900, t); bp.frequency.linearRampToValueAtTime(1600, t + 0.35);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.5, t + 0.06); g.gain.exponentialRampToValueAtTime(0.001, t + 0.45);
    n.connect(bp); bp.connect(g); g.connect(dest ?? this.sfx); n.start(t); n.stop(t + 0.5);
  }

  pour(at?: THREE.Vector3) {
    const ctx = this.ctx; if (!ctx) return;
    // a recorded pour, the fill rising, when the bought one is there
    if (this.lic('pour')) { this.play('lic:pour', { gain: 0.33, at }); return; }
    const t = ctx.currentTime;
    const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.Q.value = 3;
    bp.frequency.setValueAtTime(500, t); bp.frequency.linearRampToValueAtTime(1300, t + 1.6);
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.25, t + 0.1); g.gain.setValueAtTime(0.25, t + 1.4); g.gain.linearRampToValueAtTime(0, t + 1.7);
    n.connect(bp); bp.connect(g); g.connect(at ? this.panner(at, 1) : this.sfx); n.start(t); n.stop(t + 1.8);
  }

  // A television with no station: hiss from the set's little speaker, where it stands.
  private tv: { src: AudioBufferSourceNode; gain: GainNode } | null = null;
  tvHiss(at: THREE.Vector3 | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (this.tv) { const old = this.tv; this.tv = null; old.gain.gain.setTargetAtTime(0, ctx.currentTime, 0.05); old.src.stop(ctx.currentTime + 0.3); }
    if (!at) return;
    const n = this.noiseSrc(false, true);
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 2600; bp.Q.value = 0.5;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 450;
    const g = ctx.createGain(); g.gain.value = 0;
    g.gain.setTargetAtTime(0.09, ctx.currentTime, 0.15);
    n.connect(hp); hp.connect(bp); bp.connect(g); g.connect(this.panner(at, 0.9));
    n.start();
    this.tv = { src: n, gain: g };
  }

  // The handset sound: band-limited like a 1980s phone line.
  phoneLine() {
    const ctx = this.ctx!;
    const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 320;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3200;
    const g = ctx.createGain(); g.gain.value = 1.0;
    hp.connect(lp); lp.connect(g); g.connect(this.sfx);
    return { input: hp, gain: g, nodes: [hp, lp, g] };
  }

  futureCall(): number {
    const ctx = this.ctx; if (!ctx) return 4.3;
    const line = this.phoneLine();
    const t = ctx.currentTime;
    const n = this.noiseSrc(false); const sg = ctx.createGain(); sg.gain.value = 0.05; n.connect(sg); sg.connect(line.input); n.start(t); n.stop(t + 4.3);
    const h = this.noiseSrc(true); const hg = ctx.createGain(); hg.gain.value = 0.5; h.connect(hg); hg.connect(line.input); h.start(t); h.stop(t + 4.3);
    this.play('printer', { gain: 0.15, when: 0.4, dest: line.input });
    this.boom(0.8, line.input, 2.0);
    this.gasp(line.input, 2.25);
    this.play('ceramic', { gain: 0.9, when: 2.55, dest: line.input });
    this.play('click', { gain: 0.8, when: 4.3 });
    return 4.3;
  }

  startMotors() {
    const ctx = this.ctx; if (!ctx || this.motor) return;
    const osc = ctx.createOscillator(); osc.type = 'sawtooth'; osc.frequency.value = 46;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 240;
    const gain = ctx.createGain(); gain.gain.value = 0;
    osc.connect(lp); lp.connect(gain); gain.connect(this.amb); osc.start();
    const t = ctx.currentTime;
    gain.gain.setTargetAtTime(0.16, t, 0.8);
    osc.frequency.setTargetAtTime(58, t, 2);
    this.motor = { gain, osc };
  }
  stopMotors() {
    if (!this.motor || !this.ctx) return;
    const t = this.ctx.currentTime;
    this.motor.gain.gain.setTargetAtTime(0, t, 0.6);
    this.motor.osc.frequency.setTargetAtTime(30, t, 0.8);
    this.motor.osc.stop(t + 4);
    this.motor = null;
  }

  // An ordinary phone call on the handset: line hiss for `secs`, a click at each end.
  callLine(secs: number) {
    const ctx = this.ctx; if (!ctx) return;
    const line = this.phoneLine();
    const t = ctx.currentTime;
    const n = this.noiseSrc(false); const g = ctx.createGain(); g.gain.value = 0.035;
    n.connect(g); g.connect(line.input); n.start(t); n.stop(t + secs);
    this.play('click', { gain: 0.6 }); this.play('click', { gain: 0.5, when: secs });
  }

  /** One prerecorded handset session. Captions and story events use the returned
   *  game-time timeline; every sound itself uses the pausable AudioContext clock. */
  phoneCall(lines: PhoneCue[], options: { lead?: number; gap?: number } = {}): PhoneCallTimeline | null {
    this.stopPhoneCalls();
    const ctx = this.ctx;
    if ((!this.kinskiCalls && !this.characterVoices) || !ctx || ctx.state === 'closed' || !this.noise || !lines.length || !lines.some((l) => !l.aside)) return null;
    const buffers = lines.map((line) => {
      const actor = line.by === 'N. VEGA' ? this.characterBuffer('bacall', line.text)
        : line.by === 'THE LINE' ? this.characterBuffer('price', line.text) : undefined;
      if (actor) return actor;
      if (!this.kinskiCalls) return undefined;
      const key = phoneKeys.get(normalizeReadingText(line.text));
      return key ? this.buf[key] : undefined;
    });
    // Validate the entire call before making any sound. Missing recordings fall
    // back to the existing caption/line flow, never a half-spoken conversation.
    if (lines.some((line, i) => !line.aside && (!buffers[i] || !Number.isFinite(buffers[i]!.duration) || buffers[i]!.duration <= 0))) return null;
    const seconds = (value: number | undefined, fallback: number) => value !== undefined && Number.isFinite(value) ? Math.max(0, value) : fallback;
    const lead = seconds(options.lead, 0.3), gap = seconds(options.gap, 0.45);
    let at = lead;
    const cues = lines.map((line, i) => {
      const duration = line.aside ? seconds(line.pause, 1.2) : buffers[i]!.duration;
      const cue = { at, duration, text: line.text, by: line.by, aside: line.aside };
      at += duration + (i < lines.length - 1 ? gap : 0);
      return cue;
    });
    const duration = at + 0.2, started = ctx.currentTime;
    this.onPhoneCall?.();
    const session = { sources: [] as AudioBufferSourceNode[], nodes: [] as AudioNode[] };
    this.phoneSession = session;
    try {
      const line = this.phoneLine();
      // phoneLine's input is the high-pass, followed by low-pass and output gain.
      const speech = ctx.createGain(); speech.gain.value = 0.95; speech.connect(line.input);
      session.nodes.push(speech, ...line.nodes);
      lines.forEach((cue, i) => {
        if (cue.aside) return;
        const source = ctx.createBufferSource(); source.buffer = buffers[i]!;
        source.connect(speech); session.sources.push(source); source.start(started + cues[i].at);
      });
      const hiss = this.noiseSrc(false), level = ctx.createGain(); level.gain.value = 0.025;
      hiss.connect(level); level.connect(line.input);
      session.sources.push(hiss); session.nodes.push(level);
      hiss.onended = () => { if (this.phoneSession === session) this.stopPhoneCalls(); };
      hiss.start(started); hiss.stop(started + duration);
      for (const [when, gain] of [[0, 0.6], [at, 0.5]] as const) {
        const click = this.play('click', { gain, when, dest: line.input });
        if (click) { session.sources.push(click.src); session.nodes.push(click.gain); }
      }
      return { duration, cues };
    } catch (error) {
      this.stopPhoneCalls();
      console.warn('Telephone recording could not play', error);
      return null;
    }
  }

  /** A hang-up, replacement, new night, save load or title: future sources stop too. */
  stopPhoneCalls() {
    const session = this.phoneSession;
    if (!session) return;
    this.phoneSession = null;
    for (const source of session.sources) {
      source.onended = null;
      try { source.stop(); } catch { /* already stopped or never started */ }
      source.disconnect();
    }
    for (const node of session.nodes) node.disconnect();
  }

  // Touch-tone dialling. Returns how long it takes.
  dial(number: string) {
    const ctx = this.ctx; if (!ctx) return 1.5;
    const rows = [697, 770, 852, 941], cols = [1209, 1336, 1477], keys = '123456789*0#';
    let when = 0.25;
    for (const ch of number) {
      const k = keys.indexOf(ch);
      if (k < 0) { when += 0.2; continue; }
      const t = ctx.currentTime + when;
      for (const f of [rows[Math.floor(k / 3)], cols[k % 3]]) {
        const o = ctx.createOscillator(); o.frequency.value = f;
        const g = ctx.createGain();
        g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.05, t + 0.01); g.gain.setValueAtTime(0.05, t + 0.09); g.gain.linearRampToValueAtTime(0, t + 0.1);
        o.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + 0.12);
      }
      when += 0.16;
    }
    return when;
  }

  // North American ringback in the earpiece: 440 and 480 Hz together, on for 1.6 s.
  ringback(times: number, start = 0) {
    const ctx = this.ctx; if (!ctx) return start + times * 3.2;
    for (let i = 0; i < times; i++) {
      const t = ctx.currentTime + start + i * 3.2;
      for (const f of [440, 480]) {
        const o = ctx.createOscillator(); o.frequency.value = f;
        const g = ctx.createGain();
        g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.04, t + 0.03); g.gain.setValueAtTime(0.04, t + 1.6); g.gain.linearRampToValueAtTime(0, t + 1.65);
        o.connect(g); g.connect(this.sfx); o.start(t); o.stop(t + 1.7);
      }
    }
    return start + times * 3.2;
  }

  // Positional electrical hum (a transformer cabinet, a lamp ballast): mains and two harmonics.
  private hums = new Map<string, GainNode>();
  hum(key: string, at: THREE.Vector3, gain = 0.05, base = 60) {
    const ctx = this.ctx; if (!ctx || this.hums.has(key)) return;
    const out = ctx.createGain(); out.gain.value = 0;
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 900;
    out.connect(lp); lp.connect(this.panner(at, 1.0));
    for (const [mul, g, type] of [[1, 1, 'sine'], [2, 0.5, 'sine'], [3, 0.12, 'square']] as const) {
      const o = ctx.createOscillator(); o.type = type; o.frequency.value = base * mul;
      const og = ctx.createGain(); og.gain.value = g; o.connect(og); og.connect(out); o.start();
    }
    out.gain.setTargetAtTime(gain, ctx.currentTime, 1.0);
    this.hums.set(key, out);
  }
  stopHums() {
    if (!this.ctx) return;
    for (const g of this.hums.values()) g.gain.setTargetAtTime(0, this.ctx.currentTime, 0.3);
    this.hums.clear();
  }

  // The valve receiver in the STATION 01 hut: a weak carrier that fades in and out, with
  // static, from a small speaker. `voice` lays a voice-shaped band of noise under it for
  // a few seconds (the fragment is captioned; there is no recorded actor).
  private radioNodes: { out: GainNode; voice: GainNode; stop: () => void } | null = null;
  radio(at: THREE.Vector3 | null, level = 0.06) {
    const ctx = this.ctx; if (!ctx) return;
    if (!at) {
      const r = this.radioNodes; if (!r) return;
      this.radioNodes = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.15);
      setTimeout(() => r.stop(), 1200);
      return;
    }
    if (this.radioNodes) return;
    const out = ctx.createGain(); out.gain.value = 0;
    const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1100; bp.Q.value = 0.6;
    out.connect(bp); bp.connect(this.panner(at, 1.0));
    const osc = ctx.createOscillator(); osc.frequency.value = 760;
    const og = ctx.createGain(); og.gain.value = 0.45; osc.connect(og); og.connect(out);
    const lfo = ctx.createOscillator(); lfo.frequency.value = 0.21;
    const lg = ctx.createGain(); lg.gain.value = 0.3; lfo.connect(lg); lg.connect(og.gain);
    const n = this.staticSrc()!; const ng = ctx.createGain(); ng.gain.value = 0.3; n.out.connect(ng); ng.connect(out);
    // the voice: noise through a speech band, chopped at a syllable rate
    const vn = this.noiseSrc(true); const vb = ctx.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = 650; vb.Q.value = 1.8;
    const voice = ctx.createGain(); voice.gain.value = 0;
    const syl = ctx.createOscillator(); syl.type = 'square'; syl.frequency.value = 4.3;
    const sg = ctx.createGain(); sg.gain.value = 0.5; syl.connect(sg);
    const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
    vn.connect(vb); vb.connect(chop); chop.connect(voice); voice.connect(out);
    for (const x of [osc, lfo, vn, syl]) x.start();
    out.gain.setTargetAtTime(level, ctx.currentTime, 0.5);
    this.radioNodes = { out, voice, stop: () => { for (const x of [osc, lfo, n.src, vn, syl]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
  }
  radioVoice(secs: number, when = 0) {
    const ctx = this.ctx, r = this.radioNodes; if (!ctx || !r) return;
    const t = ctx.currentTime + when;
    r.voice.gain.setTargetAtTime(2.2, t, 0.08);
    r.voice.gain.setTargetAtTime(0, t + secs, 0.12);
  }

  // The radio in the truck's dash on the old road (chapter six): not placed in the world,
  // a small speaker in front of the driver. 'talk' is the morning programme from Roswell,
  // a mumbling horn too low to follow (the captions say what it says);
  // 'carrier' is the bare carrier when the night takes the station; null fades it out.
  // radioIn() is where the fragments of the night are played through the same speaker.
  private cab: { out: GainNode; spk: AudioNode; talk: GainNode; car: GainNode; hiss: GainNode; swell: GainNode; stop: () => void } | null = null;
  cabRadio(mode: 'talk' | 'carrier' | 'hiss' | null) {
    const ctx = this.ctx; if (!ctx) return;
    if (!mode) {
      const r = this.cab; if (!r) return;
      this.cab = null;
      r.out.gain.setTargetAtTime(0, ctx.currentTime, 0.05);
      setTimeout(() => r.stop(), 700);
      return;
    }
    if (!this.cab) {
      const out = ctx.createGain(); out.gain.value = 0;
      const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1250; bp.Q.value = 0.8;
      const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 3400;
      out.connect(bp); bp.connect(lp); lp.connect(this.sfx);
      // talk: the morning man in Roswell as a muted horn that mumbles like the grown-ups in
      // Peanuts (Tom, 16 October; core/radio.ts has the same voice): the mute opens and closes on
      // every syllable ("wah"), the pitch wanders, and there are pauses between phrases
      const horn = ctx.createOscillator(); horn.type = 'sawtooth'; horn.frequency.value = 124;
      const p1 = ctx.createOscillator(); p1.frequency.value = 0.73;
      const p1g = ctx.createGain(); p1g.gain.value = 16; p1.connect(p1g); p1g.connect(horn.frequency);
      const p2 = ctx.createOscillator(); p2.frequency.value = 1.9;
      const p2g = ctx.createGain(); p2g.gain.value = 7; p2.connect(p2g); p2g.connect(horn.frequency);
      const mute = ctx.createBiquadFilter(); mute.type = 'lowpass'; mute.frequency.value = 800; mute.Q.value = 6.5;
      const syl = ctx.createOscillator(); syl.type = 'triangle'; syl.frequency.value = 4.3;
      const sw = ctx.createGain(); sw.gain.value = 520; syl.connect(sw); sw.connect(mute.frequency);
      const sg = ctx.createGain(); sg.gain.value = 0.5; syl.connect(sg);
      const phr = ctx.createOscillator(); phr.type = 'square'; phr.frequency.value = 0.31;
      const pg = ctx.createGain(); pg.gain.value = 0.5; phr.connect(pg);
      const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
      const pause = ctx.createGain(); pause.gain.value = 0.5; pg.connect(pause.gain);
      const talk = ctx.createGain(); talk.gain.value = 0;
      horn.connect(mute); mute.connect(chop); chop.connect(pause); pause.connect(talk); talk.connect(out);
      const car = ctx.createGain(); car.gain.value = 0;
      const osc = ctx.createOscillator(); osc.frequency.value = 760; osc.connect(car); car.connect(out);
      const n = this.staticSrc()!, hiss = ctx.createGain(); hiss.gain.value = 0.25; n.out.connect(hiss); hiss.connect(out);
      // now and then the static swells over the talk and the talk sinks under it (Tom, 16 October):
      // three slow waves that only now and then add up past a threshold, so it comes irregularly
      const fz = ctx.createGain(); fz.gain.value = 0;
      const waves = [0.071, 0.113, 0.29].map((f, i) => { const o = ctx.createOscillator(); o.frequency.value = f; const g = ctx.createGain(); g.gain.value = [0.45, 0.35, 0.25][i]; o.connect(g); g.connect(fz); return o; });
      const over = ctx.createWaveShaper();
      const curve = new Float32Array(512); for (let i = 0; i < 512; i++) { const x = (i / 511) * 2.1 - 1.05; curve[i] = Math.max(0, (x - 0.55) / 0.5) ** 1.4; }
      over.curve = curve;
      fz.connect(over);
      const swell = ctx.createGain(); swell.gain.value = 0; over.connect(swell); swell.connect(hiss.gain);
      const duckAmt = ctx.createGain(); duckAmt.gain.value = -0.8; over.connect(duckAmt);
      const duck = ctx.createGain(); duck.gain.value = 1; duckAmt.connect(duck.gain);
      talk.disconnect(); talk.connect(duck); duck.connect(out);
      for (const x of [horn, p1, p2, syl, phr, osc, ...waves]) x.start();
      this.cab = { out, spk: bp, talk, car, hiss, swell, stop: () => { for (const x of [horn, p1, p2, syl, phr, osc, n.src, ...waves]) { try { x.stop(); } catch { /* stopped */ } } out.disconnect(); } };
    }
    const r = this.cab, t = ctx.currentTime;
    r.out.gain.setTargetAtTime(0.09, t, 0.2);
    r.talk.gain.setTargetAtTime(mode === 'talk' ? 0.55 : 0, t, mode === 'talk' ? 0.4 : 0.01);
    r.car.gain.setTargetAtTime(mode === 'carrier' ? 0.22 : 0, t, 0.01);
    r.hiss.gain.setTargetAtTime(mode === 'talk' ? 0.22 : mode === 'carrier' ? 0.08 : 0.35, t, 0.02);
    r.swell.gain.setTargetAtTime(mode === 'talk' ? 0.5 : 0, t, 0.05);   // the swells only under the morning talk, not in THE EVENT
  }
  /** The cab speaker's input (null when the radio is off): sounds played here come out of it. */
  radioIn(): AudioNode | null { return this.cab?.spk ?? null; }
  /** A voice on the cab radio, as noise in a speech band (no recorded actor; it is captioned).
   *  formant: lower for a man (about 520 Hz), higher for a woman (about 820 Hz). */
  cabVoice(secs: number, formant: number) {
    const ctx = this.ctx, r = this.cab; if (!ctx || !r) return;
    const t = ctx.currentTime;
    const vn = this.noiseSrc(true, true), vb = ctx.createBiquadFilter(); vb.type = 'bandpass'; vb.frequency.value = formant; vb.Q.value = 2.2;
    const vb2 = ctx.createBiquadFilter(); vb2.type = 'peaking'; vb2.frequency.value = formant * 2.6; vb2.gain.value = 8;
    const syl = ctx.createOscillator(); syl.type = 'triangle'; syl.frequency.value = 3.9 + (formant > 700 ? 0.8 : 0);
    const sg = ctx.createGain(); sg.gain.value = 0.55; syl.connect(sg);
    const chop = ctx.createGain(); chop.gain.value = 0.5; sg.connect(chop.gain);
    const g = ctx.createGain(); g.gain.value = 0;
    vn.connect(vb); vb.connect(vb2); vb2.connect(chop); chop.connect(g); g.connect(r.spk);
    g.gain.setTargetAtTime(3.2, t, 0.05); g.gain.setTargetAtTime(0, t + secs, 0.08);
    vn.start(t); syl.start(t); vn.stop(t + secs + 0.6); syl.stop(t + secs + 0.6);
  }
  /** A cup set down on a saucer: two short bright rings. dest: where it plays (the cab radio). */
  cupDown(dest?: AudioNode | null) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime, out = dest ?? this.sfx;
    for (const [dt, f, a] of [[0, 2380, 0.22], [0.012, 3710, 0.12], [0.09, 2390, 0.07], [0.1, 5120, 0.05]] as const) {
      const o = ctx.createOscillator(); o.frequency.value = f;
      const g = ctx.createGain(); g.gain.value = 0;
      g.gain.setValueAtTime(0, t + dt); g.gain.linearRampToValueAtTime(a, t + dt + 0.002); g.gain.exponentialRampToValueAtTime(0.0005, t + dt + 0.35);
      o.connect(g); g.connect(out); o.start(t + dt); o.stop(t + dt + 0.4);
    }
    const n = this.noiseSrc(false, false), ng = ctx.createGain(), hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 2500;
    ng.gain.setValueAtTime(0.18, t); ng.gain.exponentialRampToValueAtTime(0.0005, t + 0.04);
    n.connect(hp); hp.connect(ng); ng.connect(out); n.start(t); n.stop(t + 0.06);
  }
  /** Total silence (THE EVENT): every sound and the ambience off; music is left alone. */
  silence(on: boolean) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    this.sfx.gain.setTargetAtTime(on ? 0 : 1, t, on ? 0.04 : 0.3);
    this.amb.gain.setTargetAtTime(on ? 0 : 1, t, on ? 0.08 : 0.5);
  }

  // Where the listener is. Out in the yard the wind and the crickets take over from the
  // room tone; the photo lab is small and closed, with its own ventilation hum.
  private space: 'room' | 'yard' | 'lab' = 'room';
  setSpace(space: 'room' | 'yard' | 'lab') {
    this.space = space;
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    this.roomTone?.gain.setTargetAtTime(space === 'room' ? 0.5 : space === 'lab' ? 0.3 : 0.1, t, 0.7);
    this.night?.setSpace(space);
  }

  // The desert outside (core/ambience.ts): wind, crickets and far-off things, never quite
  // the same, through one outdoor level that setSpace() turns down indoors and in the cab.
  night: Ambience | null = null;
  startNight() {
    if (!this.ctx || this.night) return;
    this.night = new Ambience(this.ctx, this.amb, this.buf, this.noise, this.brown);
    this.night.setSpace(this.space);
  }

  /** A step's recording and level for a surface, without playing it (story/nightEvents.ts). */
  stepPick(surface: Surface) { const s = STEPS[surface]; return { name: s.names[Math.floor(Math.random() * s.names.length)], gain: s.gain }; }
  /** One step on a surface (Player.onStep). */
  private lastStep = '';
  step(surface: Surface) {
    const s = STEPS[surface];
    let name = s.names[Math.floor(Math.random() * s.names.length)];
    if (name === this.lastStep) name = s.names[(s.names.indexOf(name) + 1 + Math.floor(Math.random() * (s.names.length - 1))) % s.names.length];
    this.lastStep = name;
    this.play(name, { gain: s.gain * (0.8 + Math.random() * 0.3), rate: s.rate * (0.95 + Math.random() * 0.1), lp: s.lp });
  }

  /** Every frame while a night runs: the desert outside moves on (clock: the night's clock). */
  nightLife(dt: number, at: THREE.Vector3, clock = 0) { this.night?.update(dt, at, clock); }
  /** A recording far off in a random direction (an owl, a dog): through the desert outside. */
  far(name: string, at: THREE.Vector3, gain: number, rate = 1) { this.night?.far(name, at, gain, rate); }

  duckAll(seconds: number) {
    if (!this.ctx) return;
    const t = this.ctx.currentTime;
    this.sfx.gain.setTargetAtTime(0, t, 0.3); this.amb.gain.setTargetAtTime(0, t, 0.6);
    setTimeout(() => { if (!this.ctx) return; const t2 = this.ctx.currentTime; this.sfx.gain.setTargetAtTime(1, t2, 0.2); this.amb.gain.setTargetAtTime(1, t2, 0.5); }, seconds * 1000);
  }

  // Rangefinder shutter: two dry clicks, then the film advance lever.
  shutter() {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const click = (when: number, f: number, g: number) => {
      const n = this.noiseSrc(false, false); const bp = ctx.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = f; bp.Q.value = 2.5;
      const gn = ctx.createGain(); gn.gain.setValueAtTime(0, t + when); gn.gain.linearRampToValueAtTime(g, t + when + 0.002); gn.gain.exponentialRampToValueAtTime(0.001, t + when + 0.04);
      n.connect(bp); bp.connect(gn); gn.connect(this.sfx); n.start(t + when); n.stop(t + when + 0.06);
    };
    click(0, 3400, 0.8); click(0.05, 2200, 0.55);
    const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.setValueAtTime(150, t + 0.16); o.frequency.linearRampToValueAtTime(95, t + 0.42);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 1100;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t + 0.16); g.gain.linearRampToValueAtTime(0.07, t + 0.19); g.gain.linearRampToValueAtTime(0, t + 0.44);
    o.connect(lp); lp.connect(g); g.connect(this.sfx); o.start(t + 0.16); o.stop(t + 0.46);
  }

  // A small geared motor running for `secs` (the B-12 reference drive).
  servo(secs: number, at?: THREE.Vector3) {
    const ctx = this.ctx; if (!ctx) return;
    const t = ctx.currentTime;
    const o = ctx.createOscillator(); o.type = 'sawtooth'; o.frequency.setValueAtTime(210, t); o.frequency.linearRampToValueAtTime(260, t + secs * 0.5); o.frequency.linearRampToValueAtTime(190, t + secs);
    const lp = ctx.createBiquadFilter(); lp.type = 'lowpass'; lp.frequency.value = 700;
    const g = ctx.createGain(); g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.18, t + 0.08); g.gain.setValueAtTime(0.18, t + secs - 0.1); g.gain.linearRampToValueAtTime(0, t + secs);
    o.connect(lp); lp.connect(g); g.connect(at ? this.panner(at, 1.5) : this.sfx); o.start(t); o.stop(t + secs + 0.05);
    this.play('switch', { gain: 0.5, when: secs, at });
  }

  // Clicks in the 4 / 7 grouping of the signal.
  signature(gain = 0.5) {
    if (!this.ctx) return;
    const times: number[] = [];
    for (let i = 0; i < 4; i++) times.push(i * 0.22);
    for (let i = 0; i < 7; i++) times.push(1.4 + i * 0.22);
    times.forEach((w) => this.play('click', { gain, when: w, rate: 0.9 }));
  }

  suspend(on: boolean) { if (!this.ctx) return; if (on) this.ctx.suspend(); else this.ctx.resume(); }
}
