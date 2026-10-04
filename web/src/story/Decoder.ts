import * as THREE from 'three';
import type { UI } from '../ui/UI';
import type { AudioSys } from '../core/Audio';
import type { ControlRoom } from '../world/ControlRoom';
import type { Interaction } from '../core/Interaction';
import { DecoderPlayer, type DecoderParams } from '../core/decoder';
import * as P from '../ui/Panels';

// The signal processor next to RX bank 3: it holds the tape of the anomaly once the
// receiver has locked on it (from 02:14 in the prologue, and all night after that) and
// plays it back raw or harmonized (core/decoder.ts). Its display and reels follow the
// playback. It belongs to the control room, not to a chapter.

export class DecoderDesk {
  private player: DecoderPlayer | null = null;
  private params: DecoderParams = { mode: 'harmonized', pitch: 0, stretch: 2, space: 0.6 };
  private scrT = 0;
  private spin = 0;

  constructor(private d: { ui: UI; audio: AudioSys; room: ControlRoom; inter: Interaction; locked: () => boolean; recorded: () => string; busy: () => boolean; power: () => boolean }) {
    const { inter, room } = d;
    inter.add({ id: 'decoder', object: room.objs.decoder, range: 2.4,
      label: () => this.playing ? 'Signal processor (playing)' : 'Signal processor',
      use: () => this.open() });
    this.drawScreen();
  }

  get playing() { return !!this.player?.playing; }

  private open() {
    const { ui, audio } = this.d;
    if (this.d.busy()) return;
    if (!this.d.power()) { ui.toast('No power to the processor. It runs off RX bank 3.', 3); return; }
    if (!this.d.locked()) {
      ui.toast('The deck records whatever the receiver locks on. Nothing has been locked tonight.', 3.6);
      return;
    }
    if (!audio.ctx) { ui.toast('The processor needs sound. Tap the screen first.', 3); return; }
    P.decoderPanel(ui, {
      view: () => ({ tape: `TAPE B / 1420.405 MHz / PULSE GROUP 4 / 7 / RECORDED ${this.d.recorded()}`, ...this.params, playing: this.playing, current: () => this.player?.current ?? -1 }),
      onMode: (m) => { this.params.mode = m; this.player?.set({ mode: m }); },
      onParam: (k, v) => { this.params[k] = v; this.player?.set({ [k]: v }); },
      onPlay: (play) => { if (play) this.play(); else this.stop(); },
    });
  }

  private play() {
    const { audio, room } = this.d;
    const noise = audio.noiseBuffer();
    if (!audio.ctx || !noise) return;
    if (!this.player) {
      const at = room.objs.decoder.getWorldPosition(new THREE.Vector3()).add(new THREE.Vector3(-0.3, 1.1, 0));
      this.player = new DecoderPlayer(audio.ctx, audio.sfx, at, noise);
    }
    audio.play('click', { gain: 0.5 });
    this.player.start({ ...this.params });
  }
  stop() {
    if (!this.player?.playing) return;
    this.d.audio.play('click', { gain: 0.4 });
    this.player.stop();
    this.drawScreen();
  }

  // ---------- per frame ----------
  update(dt: number) {
    this.player?.update();
    if (this.playing) {
      this.spin += dt * (this.params.mode === 'raw' ? 2.4 : 1.6) / this.params.stretch;
      this.d.room.reels.forEach((r, i) => { r.rotation.x = this.spin * (i ? 0.8 : 1.15); });
    }
    this.scrT -= dt;
    if (this.scrT <= 0 && (this.playing || this.scrT < -1)) { this.scrT = 1 / 12; this.drawScreen(); }
  }

  // The display on the processor: mode, settings, and the pulses as they pass.
  private drawScreen() {
    const s = this.d.room.dspScreen;
    if (!s) return;
    const g = s.ctx, w = s.canvas.width, h = s.canvas.height;
    g.fillStyle = '#031008'; g.fillRect(0, 0, w, h);
    g.fillStyle = '#8cffa4'; g.font = '20px VT323'; g.textBaseline = 'top';
    if (!this.d.locked()) { g.fillStyle = 'rgba(140,255,164,.35)'; g.fillText('NO TAPE', 10, 8); s.tex.needsUpdate = true; return; }
    const p = this.params;
    g.fillText(`${p.mode === 'raw' ? 'RAW ' : 'HARM'} ${p.pitch >= 0 ? '+' : ''}${p.pitch}ST X${p.stretch.toFixed(1)}`, 8, 6);
    const cur = this.playing ? this.player!.current : -1;
    for (let i = 0; i < 11; i++) {
      g.fillStyle = i === cur ? '#b9ffc7' : 'rgba(110,220,150,.3)';
      g.fillRect(10 + (i < 4 ? i : i + 1) * 20, 40, 14, 22);
    }
    s.tex.needsUpdate = true;
  }
}
