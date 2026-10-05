import type * as THREE from 'three';

// Frame timing overlay, switched on with ?debug in the address.
// Meant for measuring on real phones: average and 95th percentile frame time over
// the last four seconds, plus what the GPU was asked to draw in the last frame.
export class DebugHud {
  private el = document.createElement('div');
  private times: number[] = [];
  private acc = 0;

  constructor(private renderer: THREE.WebGLRenderer, private extra: () => string) {
    this.el.className = 'debughud';
    this.el.setAttribute('aria-hidden', 'true');
    document.body.appendChild(this.el);
  }

  dispose() { this.el.remove(); }

  frame(seconds: number) {
    this.times.push(seconds);
    if (this.times.length > 240) this.times.shift();
    this.acc += seconds;
    if (this.acc < 0.5) return;
    this.acc = 0;
    const sorted = [...this.times].sort((a, b) => a - b);
    const avg = this.times.reduce((a, b) => a + b, 0) / this.times.length;
    const p95 = sorted[Math.min(sorted.length - 1, Math.floor(sorted.length * 0.95))];
    const r = this.renderer.info.render;
    this.el.textContent =
      `${(1 / avg).toFixed(0)} fps   avg ${(avg * 1000).toFixed(1)} ms   p95 ${(p95 * 1000).toFixed(1)} ms\n` +
      `${r.calls} draw calls   ${(r.triangles / 1000).toFixed(1)}k tris   pixel ratio ${this.renderer.getPixelRatio().toFixed(2)}\n` +
      this.extra();
  }
}

export const debugOn = (() => { try { return new URLSearchParams(location.search).has('debug'); } catch { return false; } })();
