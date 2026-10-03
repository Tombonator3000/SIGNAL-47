import { Rx, PROFILES, F_MIN, F_MAX, drawSpectrum } from '../story/Signal';
import type { UI } from './UI';

export class RxConsole {
  el: HTMLElement | null = null;
  private canvas!: HTMLCanvasElement;
  private g!: CanvasRenderingContext2D;
  private inputs: Record<string, HTMLInputElement> = {};
  private vals: Record<string, HTMLElement> = {};
  residualVisible = false;
  onAction?: () => void;  // stage passed
  onClick?: () => void;
  onAzimuth?: (az: number) => void;

  constructor(private ui: UI, private rx: Rx) {}

  get open() { return !!this.el && this.ui.modal === this.el; }

  show(onClose: () => void) {
    const rx = this.rx;
    const el = document.createElement('div');
    el.className = 'overlay';
    el.innerHTML = `
      <div class="crt" role="dialog" aria-label="Receiver console">
        <div class="lcol">
        <div class="row"><h2>SARO / RX CONTROL 03</h2><span class="carrier">CARRIER 0%</span></div>
        <div class="status"></div>
        <canvas width="900" height="300"></canvas>
        <div class="axis"><span>1419.5</span><span>1419.8</span><span>1420.1</span><span>1420.4</span><span>1420.7 MHz</span></div>
        <div class="refcard"></div>
        </div><div class="rcol">
        ${this.row('frequency', 'FREQUENCY MHz', F_MIN, F_MAX, 0.001, true)}
        ${this.row('gain', 'GAIN', 0, 100, 1)}
        ${this.row('bandwidth', 'BANDWIDTH kHz', 4, 100, 1)}
        ${this.row('azimuth', 'ARRAY AZ', 0, 180, 1)}
        <div class="actions">
          <button class="kbtn primary" data-a="act"></button>
          <span class="readout"></span>
          <button class="kbtn" data-a="exit">EXIT</button>
        </div>
        </div>
      </div>`;
    this.canvas = el.querySelector('canvas')!;
    this.g = this.canvas.getContext('2d')!;
    for (const k of ['frequency', 'gain', 'bandwidth', 'azimuth']) {
      const inp = el.querySelector(`input[data-k=${k}]`) as HTMLInputElement;
      this.inputs[k] = inp;
      this.vals[k] = el.querySelector(`[data-v=${k}]`) as HTMLElement;
      inp.addEventListener('input', () => { (rx as any)[k] = +inp.value; if (k === 'azimuth') this.onAzimuth?.(rx.azimuth); this.sync(false); });
    }
    el.querySelectorAll<HTMLButtonElement>('[data-step]').forEach((b) => b.addEventListener('click', () => {
      rx.frequency = Math.min(F_MAX, Math.max(F_MIN, +(rx.frequency + +b.dataset.step!).toFixed(3)));
      this.onClick?.(); this.sync(true);
    }));
    el.querySelector('[data-a=act]')!.addEventListener('click', () => { this.onClick?.(); this.act(); });
    el.querySelector('[data-a=exit]')!.addEventListener('click', () => { this.onClick?.(); this.ui.close(); });
    el.addEventListener('click', (e) => { if (e.target === el) this.ui.close(); });
    this.el = el;
    this.ui.open(el, () => { this.el = null; onClose(); });
    this.sync(true);
  }

  private row(k: string, label: string, min: number, max: number, step: number, fine = false) {
    return `<div class="ctl"><label>${label}: <span data-v="${k}"></span></label>
      <input type="range" data-k="${k}" min="${min}" max="${max}" step="${step}" aria-label="${label}">
      ${fine ? '<span class="fine"><button class="kbtn" data-step="-0.010">--</button><button class="kbtn" data-step="-0.001">-</button><button class="kbtn" data-step="0.001">+</button><button class="kbtn" data-step="0.010">++</button></span>' : '<span></span>'}</div>`;
  }

  sync(setInputs: boolean) {
    if (!this.el) return;
    const rx = this.rx;
    const fmt: Record<string, (v: number) => string> = { frequency: (v) => v.toFixed(3), gain: (v) => v.toFixed(0), bandwidth: (v) => v.toFixed(0), azimuth: (v) => v.toFixed(0).padStart(3, '0') };
    for (const k of Object.keys(this.inputs)) {
      const v = (rx as any)[k];
      if (setInputs) this.inputs[k].value = String(v);
      this.vals[k].textContent = fmt[k](v);
      this.inputs[k].disabled = rx.stage >= 3 || this.surveyOnly;
    }
    const st = this.el.querySelector('.status') as HTMLElement;
    st.textContent = rx.status;
    st.classList.toggle('warn', rx.status.startsWith('NO LOCK') || rx.status.startsWith('UNLOGGED'));
    const card = this.el.querySelector('.refcard') as HTMLElement;
    const p = rx.profile;
    if (rx.stage >= 3) { card.className = 'refcard screen'; card.textContent = 'Pattern repeats every 5.2 s: four pulses, a gap, seven pulses. Run a direction solve on the tracking terminal.'; }
    else if (this.surveyOnly) { card.className = 'refcard screen'; card.textContent = 'Survey runs unattended. Nothing scheduled until 06:00.'; }
    else { card.className = p.cardIsTaped ? 'refcard' : 'refcard screen'; card.textContent = p.card; }
    const act = this.el.querySelector('[data-a=act]') as HTMLButtonElement;
    act.textContent = rx.stage >= 3 ? 'LOCKED' : this.surveyOnly ? 'SWEEP RUNNING' : p.action;
    act.disabled = rx.stage >= 3 || this.surveyOnly;
    const ro = this.el.querySelector('.readout') as HTMLElement;
    ro.textContent = rx.stage >= 3 ? 'SOURCE: UNKNOWN   1420.405 MHz\nS/N 4.71   DIRECTION: NOT SOLVED' : '';
  }

  get surveyOnly() { return this.rx.stage === 2 && !this.residualVisible; }

  private act() {
    const rx = this.rx;
    if (!rx.pass()) { rx.status = 'NO LOCK  /  ADJUST PARAMETERS'; this.sync(false); return; }
    this.onAction?.();
    this.sync(true);
  }

  update(t: number) {
    if (!this.el) return;
    drawSpectrum(this.g, this.canvas.width, this.canvas.height, this.rx, t, { residualVisible: this.residualVisible });
    const q = this.rx.stage >= 3 ? 1 : this.surveyOnly ? 0 : this.rx.quality();
    (this.el.querySelector('.carrier') as HTMLElement).textContent = `CARRIER ${Math.round(q * 100)}%`;
  }
}

export { PROFILES };
