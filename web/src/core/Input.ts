// Unified input for desktop (WASD + mouse) and phones (left stick, right-side drag, tap to use).
export class Input {
  keys = new Set<string>();
  lookDX = 0; lookDY = 0;
  stickX = 0; stickY = 0;
  touchMode = false;
  sensitivity = 1;
  locked = false;
  onKey?: (code: string) => void;
  onTap?: (x: number, y: number) => void;
  onLockChange?: (locked: boolean) => void;
  enabled = false;

  private stickId: number | null = null;
  private lookId: number | null = null;
  private stickOrigin = { x: 0, y: 0 };
  private lookLast = { x: 0, y: 0 };
  private lookStart = { x: 0, y: 0, t: 0 };
  private base: HTMLElement; private knob: HTMLElement;

  constructor(private canvas: HTMLCanvasElement, touchLayer: HTMLElement) {
    this.touchMode = matchMedia('(pointer: coarse)').matches || 'ontouchstart' in window;
    this.base = touchLayer.querySelector('.stick-base') as HTMLElement;
    this.knob = touchLayer.querySelector('.stick-knob') as HTMLElement;

    window.addEventListener('keydown', (e) => {
      if (['Tab', 'Space', 'ArrowUp', 'ArrowDown'].includes(e.code)) e.preventDefault();
      if (e.repeat) return;
      this.keys.add(e.code);
      this.onKey?.(e.code);
    });
    window.addEventListener('keyup', (e) => this.keys.delete(e.code));
    window.addEventListener('blur', () => this.keys.clear());

    document.addEventListener('mousemove', (e) => {
      if (!this.locked || !this.enabled) return;
      this.lookDX += e.movementX * 0.0022 * this.sensitivity;
      this.lookDY += e.movementY * 0.0022 * this.sensitivity;
    });
    document.addEventListener('pointerlockchange', () => {
      this.locked = document.pointerLockElement === this.canvas;
      this.onLockChange?.(this.locked);
    });

    canvas.addEventListener('touchstart', (e) => this.touchStart(e), { passive: false });
    canvas.addEventListener('touchmove', (e) => this.touchMove(e), { passive: false });
    canvas.addEventListener('touchend', (e) => this.touchEnd(e), { passive: false });
    canvas.addEventListener('touchcancel', (e) => this.touchEnd(e), { passive: false });
  }

  requestLock() {
    if (this.touchMode) return;
    const p = this.canvas.requestPointerLock?.() as any;
    if (p && p.catch) p.catch(() => {});
  }
  releaseLock() { if (document.pointerLockElement) document.exitPointerLock(); }

  private touchStart(e: TouchEvent) {
    e.preventDefault();
    this.touchMode = true;
    if (!this.enabled) return;
    for (const t of Array.from(e.changedTouches)) {
      if (t.clientX < window.innerWidth * 0.42 && this.stickId === null) {
        this.stickId = t.identifier;
        this.stickOrigin = { x: t.clientX, y: t.clientY };
        this.base.style.display = 'block';
        this.base.style.left = t.clientX + 'px'; this.base.style.top = t.clientY + 'px';
        this.knob.style.transform = 'translate(0,0)';
      } else if (this.lookId === null) {
        this.lookId = t.identifier;
        this.lookLast = { x: t.clientX, y: t.clientY };
        this.lookStart = { x: t.clientX, y: t.clientY, t: performance.now() };
      }
    }
  }
  private touchMove(e: TouchEvent) {
    e.preventDefault();
    if (!this.enabled) return;
    for (const t of Array.from(e.changedTouches)) {
      if (t.identifier === this.stickId) {
        let dx = t.clientX - this.stickOrigin.x, dy = t.clientY - this.stickOrigin.y;
        const len = Math.hypot(dx, dy), max = 50;
        if (len > max) { dx = dx / len * max; dy = dy / len * max; }
        this.stickX = dx / max; this.stickY = dy / max;
        this.knob.style.transform = `translate(${dx}px,${dy}px)`;
      } else if (t.identifier === this.lookId) {
        this.lookDX += (t.clientX - this.lookLast.x) * 0.0055 * this.sensitivity;
        this.lookDY += (t.clientY - this.lookLast.y) * 0.0055 * this.sensitivity;
        this.lookLast = { x: t.clientX, y: t.clientY };
      }
    }
  }
  private touchEnd(e: TouchEvent) {
    e.preventDefault();
    for (const t of Array.from(e.changedTouches)) {
      if (t.identifier === this.stickId) {
        this.stickId = null; this.stickX = this.stickY = 0; this.base.style.display = 'none';
      } else if (t.identifier === this.lookId) {
        this.lookId = null;
        const moved = Math.hypot(t.clientX - this.lookStart.x, t.clientY - this.lookStart.y);
        if (moved < 12 && performance.now() - this.lookStart.t < 300) this.onTap?.(t.clientX, t.clientY);
      }
    }
  }

  move() {
    let x = 0, z = 0;
    if (this.keys.has('KeyW') || this.keys.has('ArrowUp')) z -= 1;
    if (this.keys.has('KeyS') || this.keys.has('ArrowDown')) z += 1;
    if (this.keys.has('KeyA') || this.keys.has('ArrowLeft')) x -= 1;
    if (this.keys.has('KeyD') || this.keys.has('ArrowRight')) x += 1;
    x += this.stickX; z += this.stickY;
    const l = Math.hypot(x, z);
    if (l > 1) { x /= l; z /= l; }
    return { x, z, run: this.keys.has('ShiftLeft') || this.keys.has('ShiftRight') };
  }
  consumeLook() { const d = { x: this.lookDX, y: this.lookDY }; this.lookDX = this.lookDY = 0; return d; }
  reset() { this.keys.clear(); this.stickX = this.stickY = 0; this.lookDX = this.lookDY = 0; this.stickId = this.lookId = null; this.base.style.display = 'none'; }
}
