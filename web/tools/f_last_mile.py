# The last mile (story/Chapter6.ts, LAST_MILE; core/cb.ts, CB_ENGINE; FORSLAG.md, 20 October, punkt 6;
# Tom: "Spenning og skummelt slår faste regler"). Chapter six driven out the old road with the game's
# own drive (a lane-keeping input through S47.world.testInput), from before the seven-mile post:
#   - the CB opens by itself 47 s before the line at the speed the truck is going: at 55 mph and at two
#     other speeds (desktop 55 and 63 mph, the phone 38 mph) the line comes 47 s after it; past the
#     seven-mile post; the clock says 05:28:13 then and goes to seconds in amber, bound to the truck: it
#     reaches 05:29:00 on the line and never before, and waits when the truck stops
#   - on the channel only an engine (no clicks, no voice), the captions in text; the radios' level at 0
#     does not take it away (the game's own sounds); the cab radio dips under it; it shuts 12.8 s after,
#     at least 20 s (34 s) of nothing on the CB before 05:29; once a drive; the note; the seen log
#   - a truck put on past the point (or driven at it from too close): no CB, the clock as before
#   - E and Q before the line: nothing
#   - on the line: the engine dies as before; "Turn the key" from +1.5 s with "Get out" beside it; E
#     before that does nothing; the first turn: the solenoid four, a pause, seven (the field radio's
#     relay), and it never catches; the captions; breathing close after it; then one dead click, or
#     nothing; it never starts however often; Q: "The handle moves. The door does not." and still in
#     the cab; the second try the lock button; none of it after the FLASH
#   - the third one still stands 9 m on from where the truck stops, the prompt keeps clear of it; the
#     pull-out at 05:29:47 still goes out to where it stood; the clock back to minutes then
#   - offline renders of the sounds: the CB's engine (with the truck's own idle recording and without):
#     there, two coughs, dead after it, the squelch tail, band-limited against the truck's own engine,
#     no click anywhere in it; the key: eleven clicks, four, a gap, seven
#   - desktop 1280x720 and a phone 412x892 with touch (the Use button turns the key, the Get out button
#     tries the door); no console errors or warnings
# Usage (from web/): python3 tools/f_last_mile.py OUTDIR
import asyncio, sys, json, os, math, io
from playwright.async_api import async_playwright
from PIL import Image

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
T = 5 * 3600 + 29 * 60
MILE = 1609.34
s_at = lambda mi: (mi - 8.15) * MILE
hms = lambda c: f"{int(c // 3600):02d}:{int(c % 3600 // 60):02d}:{c % 60:05.2f}"
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    toasts: [], sol: [], restarts: 0, cbs: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push({ t, e: S47.ch6['et'], c: S47.game.clock, gt: S47.game['gt'] }); return o(t, s, c); };
      const cb = S47.cb, ce = cb.engine.bind(cb);
      cb.engine = (idle, dest) => { const n = ce(idle, dest); T.cbs.push({ rec: !!idle, sfx: dest === S47.audio.sfx, n, clock: S47.game.clock, gt: S47.game['gt'], real: performance.now() }); return n; };
    },
    hookEngine() {
      const e = S47.world.engineSound; if (!e || e.__h) return !!e; e.__h = 1;
      const s = e.solenoid.bind(e); e.solenoid = (secs, k) => { T.sol.push({ secs: [...secs], k: k ?? 1, e: S47.ch6['et'] }); return s(secs, k); };
      const r = e.restart.bind(e); e.restart = () => { T.restarts++; return r(); };
      return true;
    },
    /** What the player hears, measured: the RMS of everything (the master) from `a` to `b` ms of real time
     *  after `from` (performance.now()), sampled every 40 ms. */
    meter(from, a, b, cb) {
      const A = S47.audio, ctx = A.ctx;
      // (the master; or the CB alone, where it comes out, at the master's level)
      const an = ctx.createAnalyser(); an.fftSize = 2048; (cb ? S47.cb.out : A.master).connect(an);
      const k = cb ? A.master.gain.value : 1;
      const buf = new Float32Array(an.fftSize);
      return new Promise((ok) => {
        const out = [];
        const tick = () => {
          const t = performance.now() - from;
          if (t >= a) { an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; out.push(k * Math.sqrt(s / buf.length)); }
          if (t >= b) { try { (cb ? S47.cb.out : A.master)?.disconnect(an); } catch { /* gone */ } ok(out.length ? Math.sqrt(out.reduce((x, y) => x + y * y, 0) / out.length) : null); return; }
          setTimeout(tick, 40);
        };
        tick();
      });
    },
    said(re) { return T.toasts.filter((x) => new RegExp(re).test(x.t)); },
    st() {
      const c = S47.ch6, d = S47.world.oldDrive, r = S47.world.oldRoad, w = r.where(d.pos.x, d.pos.z), L = c.lastMile, cl = document.querySelector('.clockline');
      const pr = document.querySelector('.prompt'), use = document.querySelector('.touch .use'), alt = document.querySelector('.touch .alt');
      return { stage: c.stage, e: c['et'], mi: w.mi, s: w.s, speed: d.speed, engine: d.engine, clock: S47.game.clock, cb: L.cb, open: L.openAt, closed: L.closedAt,
        skipped: L.skipped, busy: S47.cb.busy, hud: cl.textContent, secs: cl.classList.contains('secs'), key: c.wantsKey, door: c.doorOffered, tries: { ...c.tries },
        prompt: pr.classList.contains('show') ? pr.textContent : null, use: use.textContent, useReady: use.classList.contains('ready'), alt: alt.hidden ? null : alt.textContent,
        canOut: document.documentElement.classList.contains('can-out'), driving: S47.world.driving, cabLevel: S47.audio['cab'] ? S47.audio['cab'].out.gain.value : null };
    },
    /** Drive on (ticks of 1/15 s, two steps of the game's 1/30 each) for at most maxT s or until the line:
     *  the game seconds from here at which the CB opened and shut and the line came, and the corner clock
     *  each second from the CB on. */
    drive(maxT) {
      const o = { openT: null, closeT: null, eventT: null, openMi: null, openSpeed: null, openClock: null, hud: [], cabLow: null, busyMid: null, t: 0 };
      let t = 0, nextHud = null;
      while (t < maxT) {
        S47.tick(1 / 15); t += 2 / 30;
        const c = S47.ch6, L = c.lastMile, d = S47.world.oldDrive;
        if (o.openT === null && L.openAt !== null) {
          const r = S47.world.oldRoad, w = r.where(d.pos.x, d.pos.z);
          o.openT = t; o.openMi = w.mi; o.openSpeed = d.speed; o.openClock = S47.game.clock; nextHud = t + 1;
        }
        if (o.openT !== null && o.busyMid === null && t > o.openT + 5) { o.busyMid = S47.cb.busy; o.cabLow = S47.audio['cab'] ? S47.audio['cab'].out.gain.value : null; }
        if (o.closeT === null && L.closedAt !== null) o.closeT = t;
        if (nextHud !== null && t >= nextHud) { const cl = document.querySelector('.clockline'); o.hud.push([+(t - o.openT).toFixed(2), cl.textContent, cl.classList.contains('secs'), S47.game.clock]); nextHud += 1; }
        if (c.stage === 'event') { o.eventT = t; break; }
      }
      o.t = t;
      return o;
    },
    third() {
      const c = S47.ch6, f = c.third, d = S47.world.oldDrive;
      if (!f) return null;
      const p = f.mesh.getWorldPosition(new V());
      return { shown: f.mesh.visible, at: [p.x, p.y, p.z], dist: Math.hypot(p.x - d.pos.x, p.z - d.pos.z), speed: d.speed };
    },
    /** Where the third one is on the screen: the box from its feet to its head (f_third_person.py). */
    box() {
      const f = S47.ch6.third, cam = S47.camera; cam.updateMatrixWorld(true);
      const p = f.mesh.getWorldPosition(new V());
      const P = (y, dx = 0) => { const right = new V(1, 0, 0).applyQuaternion(cam.quaternion); const q = p.clone().setY(p.y + y).addScaledVector(right, dx).project(cam); return [(q.x + 1) / 2 * innerWidth, (1 - q.y) / 2 * innerHeight]; };
      const top = P(1.85), feet = P(0), l = P(1.0, -0.45), r = P(1.0, 0.45);
      return { x0: Math.min(l[0], r[0]), x1: Math.max(l[0], r[0]), y0: top[1], y1: feet[1] };
    },
    /** The sounds rendered offline (24 kHz): 'cb' the CB's engine sequence (rec: with the truck's own idle
     *  recording), 'idle' the truck's own idle alone at the same pitch, 'key' the first turn's clicks. The
     *  RMS every 10 ms, the share of the energy under 150 Hz, and the sharp onsets (a click). */
    async render(kind, rec) {
      const m = await S47.lastMileSound(), sr = 24000, E = m.CB_ENGINE;
      const len = kind === 'key' ? 5.5 : kind === 'idle' ? 6 : E.end + 1;
      const ctx = new OfflineAudioContext(1, Math.ceil(sr * len), sr);
      const noise = ctx.createBuffer(1, sr * 3, sr), nd = noise.getChannelData(0); for (let i = 0; i < nd.length; i++) nd[i] = Math.random() * 2 - 1;
      const idle = S47.world.engineSound ? S47.world.engineSound.idleRecording : null;
      if (kind === 'cb') m.engineOnChannel(ctx, ctx.destination, noise, rec ? idle : null, 0);
      else if (kind === 'idle') { const s = ctx.createBufferSource(); s.buffer = idle; s.loop = true; s.playbackRate.value = 0.9; const g = ctx.createGain(); g.gain.value = 0.55 * 3.2; s.connect(g); g.connect(ctx.destination); s.start(0); }
      else m.solenoidClicks(ctx, ctx.destination, noise, m.fourSeven(0.15));
      const d = (await ctx.startRendering()).getChannelData(0);
      const env = [], step = sr / 100;
      for (let i = 0; i + step <= d.length; i += step) { let s = 0; for (let j = 0; j < step; j++) s += d[i + j] * d[i + j]; env.push(Math.sqrt(s / step)); }
      // under 150 Hz: a two-pole low-pass run over it
      const w = 2 * Math.PI * 150 / sr, a = Math.exp(-w); let y1 = 0, y2 = 0, lo = 0, all = 0;
      const from = Math.floor(sr * 2), to = Math.floor(sr * 5);
      for (let i = 0; i < d.length; i++) { y1 = (1 - a) * d[i] + a * y1; y2 = (1 - a) * y1 + a * y2; if (i >= from && i < to) { lo += y2 * y2; all += d[i] * d[i]; } }
      // sharp onsets (a click): 2 ms frames whose energy is 25 times the 100 ms before and above a floor (an
      // engine's own firing, fifteen times a second, is part of what it is like, not a click)
      const f2 = Math.floor(sr * 0.002), fe = [];
      for (let i = 0; i + f2 <= d.length; i += f2) { let s = 0; for (let j = 0; j < f2; j++) s += d[i + j] * d[i + j]; fe.push(s / f2); }
      const on = [];
      for (let i = 50; i < fe.length; i++) { let b = 0; for (let j = i - 50; j < i - 1; j++) b += fe[j]; b /= 49; if (fe[i] > 25 * b + 1e-7 && fe[i] > 2e-4 && (!on.length || i * 0.002 - on[on.length - 1] > 0.08)) on.push(+(i * 0.002).toFixed(3)); }
      return { env, low: all > 0 ? lo / all : null, onsets: on, peak: Math.max(...env) };
    },
  };
  return true;
})()"""

AUTO = """((v) => { const w = S47.world, road = w.oldRoad, drv = w.oldDrive;
  w.testInput = () => { const s = road.where(drv.pos.x, drv.pos.z).s; const tg = road.lane(s + 18);
    let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
    return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: v === null ? (drv.speed > 0.05 ? -1 : 0) : Math.max(-1, Math.min(1, (v - drv.speed) * 0.5)) }; }; })"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS); await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    await pg.evaluate("S47.hold = true; S47.curve.free = true; S47.tick(0.2)")
    return ctx, pg, errs

class Run:
    def __init__(self, pg, touch, tag): self.pg = pg; self.ev = pg.evaluate; self.touch = touch; self.tag = tag
    async def tick(self, s): await self.ev(f"S47.tick({s})")
    async def to(self, e):
        """THE EVENT to e seconds in (small steps)."""
        await self.ev(f"(() => {{ let left = {e} - S47.ch6['et']; while (left > 1e-4) {{ const s = Math.min(1 / 30, left); S47.tick(s); left -= s; }} }})()")
    async def shot(self, name):
        # (the captions fade in and out on the page's own clock: give them a moment in real time)
        await self.pg.wait_for_timeout(700)
        await self.pg.screenshot(path=f'{OUT}/{self.tag}_{name}.png')
    async def st(self): return await self.ev("T.st()")
    async def key(self):
        """E, or the Use button on a phone (False: the button is not there to be pressed)."""
        if self.touch:
            bb = await self.pg.locator('.touch .use').bounding_box()
            if not bb: return False
            await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        else: await self.pg.keyboard.press('KeyE')
        await self.tick(1 / 30)
        return True
    async def door(self):
        """Q, or the Get out button on a phone (False: the button is not there)."""
        if self.touch:
            bb = await self.pg.locator('.touch .alt').bounding_box()
            if not bb: return False
            await self.pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        else: await self.pg.keyboard.press('KeyQ')
        await self.tick(1 / 30)
        return True
    async def road(self, mi, speed, radio=None):
        """Chapter six from the diner, the truck put on the old road at a mile, driven at a speed."""
        ev = self.ev
        await ev("S47.game.d.ui.close(); S47.jump('chapter6'); S47.tick(0.5)")
        for _ in range(80):
            await self.tick(0.25); await self.pg.wait_for_timeout(100)
            if await ev("S47.world.area === 'roswell' && S47.world.driving && S47.ch6.stage === 'drive'"): break
        # (the truck's own recordings decoded: the CB plays its idle)
        for _ in range(60):
            if await ev("T.hookEngine() && !!S47.world.engineSound.idleRecording"): break
            await self.pg.wait_for_timeout(250)
        if radio is not None: await ev(f"S47.audio.setRadio({radio})")
        await ev(f"""(() => {{ const r = S47.world.oldRoad, d = S47.world.oldDrive, s = {s_at(mi)};
              const a = r.lane(s), b2 = r.lane(s + 4); d.place(a, Math.atan2(-(b2.x - a.x), -(b2.z - a.z))); }})()""")
        await self.tick(0.1)
        await ev(AUTO + f"({speed})")
        await ev("T.toasts.length = 0; T.cbs.length = 0; S47.game.d.ui.clearToasts()")

async def timing(R, speed, mi0, label, shots=False):
    """Out the old road at a speed: the CB 47 s before the line, shut at least 20 s before it."""
    ev = R.ev
    await R.road(mi0, speed, radio=0 if label == '55 mph' else None)
    # E and Q on the way: nothing (the old road has no getting out, and no key to turn)
    await R.tick(1.0)
    pressed = [await R.door(), await R.key()]; await R.tick(0.5)
    s = await R.st()
    check(s['tries'] == {'key': 0, 'door': 0, 'keyFree': 0, 'doorFree': 0} and not T_said(await ev("T.toasts"), 'handle') and s['driving'] and (pressed == [False, False] if R.touch else True),
          f'{label}: on the way out, {"no Use or Get out button" if R.touch else "E and Q do nothing"} (no key, no door)')
    # what the cab sounds like before it (the engine, the morning radio, the road), in real time
    bed = await ev("T.meter(performance.now(), 200, 2600)") if label == '55 mph' else None
    # up to the CB opening
    o = await ev("(() => { const o = { t: 0 }; while (S47.ch6.lastMile.openAt === null && S47.ch6.stage === 'drive' && o.t < 400) { S47.tick(1 / 15); o.t += 2 / 30; } return o; })()")
    s = await R.st()
    open_speed = s['speed']
    check(s['open'] is not None and s['mi'] >= 7.0 and s['stage'] == 'drive', f'{label}: the CB opens by itself past the seven-mile post (mile {s["mi"]:.3f}, {open_speed:.1f} m/s)')
    check(abs(s['clock'] - (T - 47)) < 0.05 and s['secs'] and s['hud'] == '05:28:13', f'{label}: the clock says {s["hud"]} then, in seconds (amber: {s["secs"]})')
    cbs = await ev("T.cbs")
    check(len(cbs) == 1 and cbs[0]['rec'] and cbs[0]['sfx'] and cbs[0]['n'] > 12, f'{label}: on the CB the truck\'s own idle, through the game\'s sounds{" with the radios at 0" if label == "55 mph" else ""} ({cbs})')
    await R.tick(1.2)
    if shots:
        await R.shot('01_cb_opens')
    if bed:
        # and with the engine on the channel idling (its sound goes on in real time, the game held)
        on = await ev(f"T.meter({cbs[0]['real']}, 1900, 5300, true)")
        db = 20 * math.log10(on / bed) if on and bed else None
        check(db is not None and 1 <= db <= 9, f'{label}: the engine on the CB is heard over the cab at 55 mph, a few dB and no more: {db:+.1f} dB against the cab before it ({on:.4f} against {bed:.4f} RMS)' if db is not None else 'measured')
    await R.pg.wait_for_timeout(1500)
    dip = (await R.st())['cabLevel']
    compass = T_said(await ev("T.toasts"), 'compass on the dash')
    g0 = cbs[0]['gt'] if cbs else 0
    check(all(c['gt'] < g0 - 6 for c in compass), f'{label}: the compass\'s caption does not come with the CB\'s ({[round(c["gt"] - g0, 1) for c in compass]} s from it)')
    rest = await ev("T.drive(120)")
    # (the time from the open: the first tick of drive() is 2/30 s after it, and 1.2 s were ticked above)
    open_to_line = rest['eventT'] + 1.2
    close_to_line = rest['eventT'] - rest['closeT'] if rest['closeT'] is not None else None
    check(rest['eventT'] is not None and abs(open_to_line - 47) < 0.6, f'{label}: the line comes {open_to_line:.2f} s after the CB opened (47)')
    await R.pg.wait_for_timeout(1500)
    back = (await R.st())['cabLevel']
    check(rest['busyMid'] is True and dip is not None and dip < 0.05 and back > 0.08, f'{label}: the CB is busy while it plays, the cab radio dips under it ({dip:.3f}) and comes back ({back:.3f}; on the line it is the carrier\'s)')
    check(close_to_line is not None and close_to_line >= 20, f'{label}: the CB shut {open_to_line - close_to_line:.1f} s after it opened, {close_to_line:.1f} s of nothing on it before 05:29 (at least 20)')
    compass = T_said(await ev("T.toasts"), 'compass on the dash')
    check(len(compass) <= 1 and all(c['gt'] < g0 - 6 or c['gt'] >= g0 + 12.7 for c in compass), f'{label}: the compass comes round to 026 (once a night), before the CB or after it has shut ({[round(c["gt"] - g0, 1) for c in compass]} s from it)')
    hud = rest['hud']
    secs_all = all(h[2] for h in hud)
    mono = all(hud[i][3] <= hud[i + 1][3] for i in range(len(hud) - 1))
    near = [h for h in hud if h[0] >= 19.5 and h[0] <= 20.5]
    check(secs_all and mono and hud and hud[-1][1] in ('05:28:59', '05:28:58') and all(h[3] < T for h in hud), f'{label}: the corner clock counts in seconds to the line and never shows 05:29 before it ({hud[0][1]} .. {hud[-1][1]})')
    if near: check(abs(near[0][3] - (T - 47 + near[0][0] + 1.2)) < 2.0, f'{label}: and keeps time with the truck at a steady speed ({near[0][1]} at {near[0][0] + 1.2:.1f} s after the CB)')
    return rest, open_to_line, close_to_line

def T_said(toasts, word): return [x for x in toasts if word in x['t']]

async def at_the_line(R):
    """On the line: the engine, the key, the door, the third one, the pull-out."""
    ev, pg = R.ev, R.pg
    s = await R.st()
    check(s['stage'] == 'event' and s['hud'] == '05:29:00' and s['secs'], f'on the line: 05:29:00 on the corner clock ({s["hud"]})')
    await R.to(0.5)
    s = await R.st()
    check(not s['engine'] and not s['key'] and s['prompt'] is None, 'the engine dies on the line, as before; no key yet while it dies')
    await R.key()
    s = await R.st()
    check(s['tries']['key'] == 0 and await ev("T.sol.length") == 0, 'E while the engine is dying: nothing (THE EVENT has E, nothing else answers)')
    await R.to(1.7)
    s = await R.st()
    if R.touch: ok = s['key'] and s['use'] == 'Turn the key' and s['useReady'] and s['alt'] == 'Get out' and s['canOut']
    else: ok = s['key'] and s['prompt'] and 'Turn the key' in s['prompt'] and 'Get out' in s['prompt']
    check(ok, f'+1.7 s: "Turn the key", as all night, with Get out beside it ({s["prompt"] if not R.touch else (s["use"], s["alt"], s["canOut"])})')
    await R.shot('02_turn_the_key')
    # the first turn: four, a pause, seven
    await R.key()
    s = await R.st()
    sol = await ev("T.sol")
    secs = sol[0]['secs'] if sol else []
    gaps = [round(b - a, 2) for a, b in zip(secs, secs[1:])]
    check(s['tries']['key'] == 1 and len(secs) == 11 and gaps[:3] == [0.32] * 3 and gaps[3] == 1.22 and gaps[4:] == [0.32] * 6,
          f'the key: the solenoid four, a pause, seven, the field radio\'s relay ({gaps})')
    check(not s['key'] and (s['prompt'] is None or 'Turn the key' not in s['prompt']), 'no "Turn the key" while it clicks')
    e0 = s['e']
    await R.to(e0 + 1.3)
    four = T_said(await ev("T.toasts"), 'four times')
    await R.to(e0 + 4.5)
    seven = T_said(await ev("T.toasts"), 'Then seven')
    await R.to(e0 + 5.0)
    breath = T_said(await ev("T.toasts"), 'Breathing')
    check(four and seven and breath and four[0]['e'] < seven[0]['e'] < breath[0]['e'], 'captions with the clicks: four, then seven, nothing turns over; then breathing close, the player\'s own')
    await R.shot('03_four_and_seven')
    await R.to(e0 + 5.4)
    s = await R.st()
    check(s['key'] and not s['engine'] and await ev("T.restarts") == 0, '"Turn the key" again after it: the engine still dead, it never caught')
    # later turns: one dead click, or nothing
    await R.key(); s1 = await R.st(); n1 = await ev("T.sol.length")
    await R.to(s1['e'] + 0.9)
    await R.key(); s2 = await R.st(); n2 = await ev("T.sol.length")
    await R.to(s2['e'] + 0.9)
    await R.key(); n3 = await ev("T.sol.length")
    sol = await ev("T.sol")
    check(n1 == 2 and len(sol[1]['secs']) == 1 and sol[1]['k'] < 1 and n2 == 2 and n3 == 3 and T_said(await ev("T.toasts"), 'One click') and T_said(await ev("T.toasts"), 'Not even the click'),
          'then one dead click, then nothing, then a click: never the starter')
    for _ in range(4):
        await R.to((await R.st())['e'] + 0.9); await R.key()
    s = await R.st()
    check(s['tries']['key'] == 8 and not s['engine'] and await ev("T.restarts") == 0 and await ev("S47.world.oldDrive.engine") is False, f'eight turns of the key and it never starts ({s["tries"]["key"]})')
    # the door
    cam0 = await ev("S47.camera.position.toArray()")
    await R.door()
    s = await R.st()
    d1 = T_said(await ev("T.toasts"), 'The handle moves')
    check(s['tries']['door'] == 1 and d1 and d1[0]['t'] == 'The handle moves. The door does not.' and s['driving'] and s['stage'] == 'event',
          f'{"Get out" if R.touch else "Q"}: "The handle moves. The door does not." Still in the cab')
    await R.shot('04_the_door')
    await R.door()
    check((await R.st())['tries']['door'] == 1, 'a second try at once: nothing yet')
    await R.to((await R.st())['e'] + 0.7); await R.door()
    d2 = T_said(await ev("T.toasts"), 'lock button')
    cam1 = await ev("S47.camera.position.toArray()")
    check(d2 and (await R.st())['tries']['door'] == 2 and math.dist(cam0, cam1) < 1.5, 'again: "The lock button is up." and the camera still in the cab')
    # the third one: the truck stands 9 m short of it, as planned
    await R.to(9.8)
    th = await ev("T.third()")
    calls = await ev("S47.renderer.info.render.calls")
    check(calls <= 90, f'draw calls on the line with the key and the door: {calls} (at most 90)')
    check(th and th['shown'] and abs(th['speed']) < 0.05 and abs(th['dist'] - 9) < 0.7, f'the truck stands 9 m short of the third one, as before ({th["dist"]:.2f} m)' if th else 'the third one is there')
    at0 = th['at'] if th else None
    await R.to(11.4)
    await ev("S47.game.d.ui.clearToasts()"); await pg.wait_for_timeout(250)
    s = await R.st()
    if not s['key']: await R.to((await R.st())['e'] + 1.0)
    box = await ev("T.box()")
    pb = await ev("(() => { const p = document.querySelector('.prompt'); if (!p.classList.contains('show')) return null; const r = p.getBoundingClientRect(); return [r.left, r.top, r.right, r.bottom]; })()")
    if not R.touch:
        clear = pb is not None and (pb[1] > box['y1'] + 4 or pb[3] < box['y0'] - 4 or pb[2] < box['x0'] or pb[0] > box['x1'])
        check(clear, f'the prompt keeps clear of the third one in the blue headlights (prompt {[round(v) for v in pb] if pb else None}, it {[round(box[k]) for k in ("x0", "y0", "x1", "y1")]})')
    await R.shot('05_blue_key')
    # after the FLASH: no key, no door
    await R.to(31.6)
    s = await R.st()
    check(not s['key'] and not s['door'] and s['prompt'] is None and s['alt'] is None, '+31.6 s: no key and no door any more (the FLASH)')
    k0 = s['tries']['key']; await R.key(); await R.door()
    check((await R.st())['tries']['key'] == k0, 'and E does nothing then')
    await R.to(46.0)
    s = await R.st()
    check(s['hud'] == '05:29:46' and s['secs'], f'the clock counts THE EVENT in seconds ({s["hud"]})')
    # the pull-out, out to where it stood
    await R.to(47.3); await R.tick(6.5)
    cam = await ev("S47.camera.position.toArray()")
    s = await R.st()
    check(s['stage'] == 'pullout' and at0 and math.hypot(cam[0] - at0[0], cam[2] - at0[2]) < 0.8 and not s['secs'] and s['hud'] in ('05:29', '05:30'),
          f'05:29:47: the pull-out goes out to where it stood ({math.hypot(cam[0] - at0[0], cam[2] - at0[2]):.2f} m), the clock in minutes again ({s["hud"]})')
    await R.shot('06_pullout')
    seen = await ev("S47.seen.filter((e) => e.what === 'the last mile')")
    check(len(seen) == 1 and seen[0]['kind'] == 'event' and seen[0]['line'] == 'C' and seen[0]['looking'] is None and seen[0]['clock'] == T - 47,
          f'the seen log: the last mile once, on C, heard ({seen})')
    notes = await ev("S47.game.notes.filter((n) => /CB opened by itself/.test(n))")
    check(len(notes) == 1 and 'This one kept running' in notes[0], f'the note ({notes[0][:60] if notes else None})')

async def desktop(b):
    ctx, pg, errs = await page(b, 1280, 720, False)
    R = Run(pg, False, 'd')
    ev = R.ev
    # ---------- the sounds, offline ----------
    await R.road(6.9, 0)
    cb = await ev("T.render('cb', true)")
    env = cb['env']
    at = lambda t: env[int(t * 100)]
    win = lambda a, b2: max(env[int(a * 100):int(b2 * 100)])
    steady = sum(env[200:500]) / 300
    dips = [min(env[int(c * 100):int(c * 100) + 20]) for c in (5.6, 6.9)]
    dead = win(9.45, 12.25)
    tail = win(12.28, 12.5)
    after = win(12.75, 13.6)
    check(steady > 0.02 and all(d < 0.45 * steady for d in dips), f'the CB, offline: an engine there from 1.4 s ({steady:.3f} RMS, peak {cb["peak"]:.3f}), two coughs ({[round(d / steady, 2) for d in dips]} of it)')
    check(dead < 0.08 * steady and tail > 3 * dead and after < 0.01 * tail, f'it dies at 8 s, the channel open on nothing ({dead / steady:.3f} of it), the squelch tail at 12.3 s ({tail:.3f}), then shut ({after:.5f})')
    check(all(o < 0.08 or abs(o - 12.3) < 0.08 for o in cb['onsets']), f'no click anywhere in it: the only sharp onsets are the squelch opening and its tail ({cb["onsets"]})')
    idle = await ev("T.render('idle', true)")
    check(cb['low'] is not None and idle['low'] is not None and cb['low'] < 0.5 * idle['low'], f'band-limited, as through the CB\'s speaker: {cb["low"] * 100:.0f}% of it under 150 Hz against {idle["low"] * 100:.0f}% of the truck\'s own idle')
    syn = await ev("T.render('cb', false)")
    se = syn['env']; sst = sum(se[200:500]) / 300
    check(sst > 0.01 and max(se[945:1225]) < 0.08 * sst and max(se[1280:1360]) < 1e-3, f'and without the recording (the field radio\'s made engine, until the truck\'s own is in): there, dead at 8 s, shut after the tail ({sst:.3f} RMS)')
    key = await ev("T.render('key')")
    on = key['onsets']
    groups, g = [], 1
    for a, c in zip(on, on[1:]):
        if c - a > 0.6: groups.append(g); g = 1
        else: g += 1
    groups.append(g)
    check(len(on) == 11 and groups == [4, 7], f'the key, offline: eleven clicks, four, a gap, seven ({groups}, at {on})')
    # ---------- 55 mph, the radios at 0 ----------
    await timing(R, 24.6, 6.9, '55 mph', shots=True)
    st = await R.st()
    check(await ev("T.cbs.length") == 1 and st['closed'] is not None, 'once a drive: the CB opened once')
    await at_the_line(R)
    # ---------- 63 mph ----------
    await ev("T.sol.length = 0; T.restarts = 0")
    await timing(R, 28.2, 6.6, '63 mph')
    # ---------- stopped in the 47: the clock waits ----------
    await R.road(6.9, 24.6)
    await ev("(() => { let t = 0; while (S47.ch6.lastMile.openAt === null && t < 400) { S47.tick(1 / 15); t += 2 / 30; } })()")
    await R.tick(20)
    await ev(AUTO + "(null)")
    await R.tick(4)
    a = await R.st(); await R.tick(8); b2 = await R.st()
    check(a['secs'] and abs(a['speed']) < 0.05 and a['hud'] == b2['hud'] and a['stage'] == 'drive', f'stopped 20 s after the CB: the seconds wait with the truck ({a["hud"]}, {b2["hud"]} 8 s later)')
    await R.shot('07_stopped_waits')
    await ev(AUTO + "(20)")
    for _ in range(80):
        await R.tick(1)
        if (await R.st())['stage'] == 'event': break
    s = await R.st()
    check(s['stage'] == 'event' and s['hud'] == '05:29:00' and await ev("T.cbs.length") == 1, 'and on again: 05:29:00 on the line, the CB not again')
    # ---------- put on past the point: nothing ----------
    await R.road(7.9, 24.6)
    o = await ev("T.drive(60)")
    s = await R.st()
    check(o['openT'] is None and await ev("T.cbs.length") == 0 and s['stage'] == 'event' and s['skipped'] is False and s['open'] is None and not await ev("T.toasts.some((x) => /CB opens/.test(x.t))"),
          'a truck put on the road past the point (mile 7.9): no CB')
    hud_before = [h for h in o['hud']]
    check(not hud_before, 'and the corner clock as before, in minutes')
    await ctx.close()
    return errs

async def phone(b):
    ctx, pg, errs = await page(b, 412, 892, True)
    R = Run(pg, True, 'p')
    await timing(R, 17.0, 6.9, '38 mph (phone)', shots=True)
    await at_the_line(R)
    await ctx.close()
    return errs

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        allerr = []
        allerr += await desktop(b)
        allerr += await phone(b)
        print('\n'.join(allerr[:30]) or 'no console errors/warnings')
        check(not allerr, 'console clean (desktop, phone)')
        json.dump(checks, open(f'{OUT}/checks-last-mile.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
