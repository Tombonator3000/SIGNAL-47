// Ported 1:1 from Unity: SignalProfile assets + SignalFeedback.Quality + SignalConsole stages.
export interface Profile {
  stage: string; action: string; card: string; cardIsTaped: boolean;
  f: number; fTol: number; gain: [number, number]; bw: [number, number]; az: [number, number]; note: string;
}

export const PROFILES: Profile[] = [
  { stage: 'CALIBRATION', action: 'LOG CALIBRATION', cardIsTaped: true,
    card: 'TAPED REF CARD  /  CAL 1419.900 MHz  /  AZ 042  /  GAIN 55 +-10  /  BW 48 +-14',
    f: 1419.9, fTol: 0.025, gain: [45, 65], bw: [34, 62], az: [35, 49], note: 'Receiver calibrated on the 1419.900 reference.' },
  { stage: 'INTERFERENCE', action: 'NOTCH INTERFERENCE', cardIsTaped: false,
    card: 'LOCAL SPIKE  /  centre carrier near 1420.110 MHz. Narrow BW below 22 kHz while keeping nominal gain.',
    f: 1420.11, fTol: 0.03, gain: [40, 72], bw: [4, 22], az: [0, 180], note: 'Local carrier at 1420.110 notched. Highway relay again.' },
  { stage: 'ANOMALY', action: 'ISOLATE PATTERN', cardIsTaped: false,
    card: 'UNLOGGED RESIDUAL  /  weak peak near 1420.4 MHz. Raise gain, narrow bandwidth, then search array azimuth by hand.',
    f: 1420.405, fTol: 0.008, gain: [78, 100], bw: [4, 14], az: [79, 87], note: 'Narrowband carrier isolated under the hydrogen line.' },
];

export const F_MIN = 1419.5, F_MAX = 1420.7;

export class Rx {
  frequency = 1419.62; gain = 27; bandwidth = 82; azimuth = 18;
  stage = 0;           // index into PROFILES; 3 = pattern locked
  solved = false;      // direction solve done
  status = 'CALIBRATION REQUIRED';
  powered = false;

  get profile() { return PROFILES[Math.min(this.stage, PROFILES.length - 1)]; }

  quality(p = this.profile) {
    const rd = (v: number, r: [number, number]) => Math.max(r[0] - v, 0, v - r[1]);
    const f = Math.max(0, Math.abs(this.frequency - p.f) - p.fTol) / Math.max(p.fTol * 4, 0.025);
    const g = rd(this.gain, p.gain) / 22, b = rd(this.bandwidth, p.bw) / 25, a = rd(this.azimuth, p.az) / 25;
    return Math.exp(-(f * f + g * g + b * b + a * a));
  }
  pass(p = this.profile) {
    return Math.abs(this.frequency - p.f) <= p.fTol && this.gain >= p.gain[0] && this.gain <= p.gain[1]
      && this.bandwidth >= p.bw[0] && this.bandwidth <= p.bw[1] && this.azimuth >= p.az[0] && this.azimuth <= p.az[1];
  }
  // 4 pulses, pause, 7 pulses. Returns 0..1 envelope.
  pulse(t: number) {
    if (this.stage < 3) return 1;
    const c = t % 5.2;
    const count = c < 1.6 ? 4 : 7;
    const local = c < 1.6 ? c : c - 2.1;
    if (local < 0) return 0.06;
    const slot = 0.22;
    return local < count * slot && (local % slot) < 0.11 ? 1 : 0.06;
  }
  advance() {
    if (this.stage === 0) { this.stage = 1; this.status = 'LOCAL CARRIER DRIFT'; this.frequency = 1419.98; this.gain = 55; this.bandwidth = 58; this.azimuth = 42; }
    else if (this.stage === 1) { this.stage = 2; this.status = 'SURVEY SWEEP ACTIVE  /  H-LINE  /  UNTIL 06:00'; }
    else if (this.stage === 2) { this.stage = 3; this.status = 'PATTERN LOCK  /  PULSE GROUP 4 / 7'; this.frequency = 1420.405; this.gain = 82; this.bandwidth = 12; this.azimuth = 83; }
  }
}

// Draw the spectrum trace. Used by the overlay console and by the physical CRT in the room.
export function drawSpectrum(g: CanvasRenderingContext2D, w: number, h: number, rx: Rx, t: number, opts: { residualVisible: boolean }) {
  g.fillStyle = '#021006'; g.fillRect(0, 0, w, h);
  g.strokeStyle = 'rgba(60,180,95,.22)'; g.lineWidth = 1;
  for (let x = 0; x <= w; x += w / 8) { g.beginPath(); g.moveTo(x, 0); g.lineTo(x, h); g.stroke(); }
  for (let y = h * 0.15; y < h; y += h * 0.17) { g.beginPath(); g.moveTo(0, y); g.lineTo(w, y); g.stroke(); }
  if (!rx.powered) return;
  const fx = (f: number) => (f - F_MIN) / (F_MAX - F_MIN) * w;
  const tuned = fx(rx.frequency);
  const half = Math.max(2, (rx.bandwidth / 1000) / (F_MAX - F_MIN) * w * 0.5);
  g.fillStyle = 'rgba(70,255,120,.08)'; g.fillRect(tuned - half, 0, half * 2, h);
  const p = rx.profile;
  const showPeak = rx.stage !== 2 || opts.residualVisible;
  const q = rx.quality();
  const amp = (12 + 110 * q * rx.gain / 100) * (h / 220) * rx.pulse(t);
  const peakX = fx(p.f);
  g.strokeStyle = '#8cffa4'; g.lineWidth = Math.max(1.5, w / 340); g.shadowColor = 'rgba(90,255,130,.8)'; g.shadowBlur = 6;
  g.beginPath();
  for (let x = 0; x <= w; x += 2) {
    const noise = (Math.sin(x * 0.17 + t * 4) * 2 + Math.sin(x * 0.71 + t * 7) * 3 + (Math.random() - 0.5) * 3) * (rx.bandwidth / 35 + 0.2) * (h / 220);
    let y = h * 0.84 - noise;
    if (showPeak) y -= amp * Math.exp(-Math.pow((x - peakX) / (7 * w / 512), 2));
    // the hydrogen line itself is always a soft broad hump
    y -= 14 * (h / 220) * Math.exp(-Math.pow((x - fx(1420.406)) / (40 * w / 512), 2));
    x === 0 ? g.moveTo(x, y) : g.lineTo(x, y);
  }
  g.stroke(); g.shadowBlur = 0;
  g.strokeStyle = 'rgba(255,200,90,.85)'; g.setLineDash([3, 3]);
  g.beginPath(); g.moveTo(tuned, 0); g.lineTo(tuned, h); g.stroke(); g.setLineDash([]);
}
