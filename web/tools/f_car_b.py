# Four, and not yet seven: the truck on B (story/truckOnB.ts; FORSLAG.md, 20 October, item 8).
# Drives the truck over B fast, down and up the highway past SARO's turn-off, with the game's own
# drive (a lane-keeping input through S47.world.testInput), and checks what the player gets:
#   - chapter one: over B, nothing; the compass on the dash reads the heading (180, then 000)
#   - chapter two, over B with something else going (the office television on its carrier): missed,
#     nothing; the next time over: four. The radio on Night Line drops to a bare carrier (the 760 Hz
#     on the radio bus) for about a second, the engine misses (the pull drops), every light in the
#     truck goes out exactly four times, evenly (the headlights, the dash: sampled), a relay ticks in
#     the dash with each, the compass on 026 while the truck runs at 180; the toasts, one note, the
#     key, a mark on B on the notebook's map (at the turn-off), the seen log (event truck4, line B);
#     the curve takes it as the build's event, not its peak; no saving while it runs; one at a time
#     while it runs and its tail, then free
#   - over B again before room 6: nothing; the compass still swings to 026 over B and back after
#   - Continue: four kept, its note once; over B: nothing
#   - after room 6 (chapter five), radio off: the engine dies over B, OIL and VOLTS, the dial lit on a
#     carrier, "Turn the key", no getting out, it waits; E: it turns over and does not catch; E again:
#     it catches; then every light out exactly seven times, the relay with each; "Seven.", the note
#     ("Four, the last time."), the key, the curve lands on it and rests, the seen log; over B again:
#     nothing; the second mark on the map, its time under the first
#   - the way most come out of room 6, the truck on the motel's lot where a driver leaves it ("Drive to
#     Mesa Diner"): the drive never goes over B again, so the engine dies as the truck pulls out onto
#     the highway (B's ground); the key twice, seven; the note, the mark on B in front of the motel,
#     the seen log; a radio line is not captioned while the radio gives only the carrier
#   - Flashing images: slow fades: the four's blinks dip softly (never out), still four of them
#   - the old road (chapter six): the compass reads the road at mile 6, between at 6.9, on 026 from
#     7.6 on while the road runs north-east; the toast once
#   - phone 412x892 with touch: four in chapter two, the stall in chapter five with the Use button
#     turning the key twice, seven; screenshots
#   - no console errors or warnings
# Usage: python3 tools/f_car_b.py OUTDIR
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
MILE = 1609.34
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  const V = S47.three.Vector3;
  window.T = {
    toasts: [], plays: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { T.toasts.push(t); return o(t, s, c); };
      const a = S47.audio, p = a.play.bind(a); a.play = (name, opt) => { T.plays.push(name); return p(name, opt); };
    },
    L() { const w = S47.world; return w.legs[w.leg]; },
    st() {
      const w = S47.world, L = w.legs[w.leg], tk = L.truck, D = L.drive, B = S47.truckB;
      const lamps = Object.entries(tk['lamps']).filter(([k, m]) => m.visible).map(([k]) => k);
      // what the prompt says: under the crosshair on a desktop (after its key), on the Use button on a phone
      const pr = document.querySelector('.hud .prompt'), use = document.querySelector('.touch .use'), kb = pr && pr.querySelector('kbd');
      const said = S47.input.touchMode ? (use && use.classList.contains('ready') ? use.textContent : '') : pr && pr.classList.contains('show') ? pr.textContent.slice(kb ? kb.textContent.length : 0) : '';
      return { phase: B.phase, kind: w.trouble.kind, stage: w.trouble.stage, level: tk['level'], dash: tk['dashLevel'], engine: D.engine, power: D.power,
        speed: D.speed, x: D.pos.x, z: D.pos.z, lamps, key: w.trouble.wantsKey, out: w.canGetOut(), compass: tk.compassReads, bearing: tk.bearing,
        carrying: S47.carRadio.carrying, radioOn: S47.carRadio.on, dist: B.dist, k: B.pull ? B.pull.k : 0, blinked: B.blinked, busy: S47.oneAtATime(),
        block: S47.saveBlock(), canOut: document.documentElement.classList.contains('can-out'),
        prompt: said };
    },
    // the truck in a lane of the highway through SARO's site (x -25.8 southbound, -22.2 northbound),
    // driven at v m/s along it by the game's own drive
    lane(x, z, h, v) {
      const w = S47.world, D = T.L().drive; D.place(new V(x, 0, z), h);
      w.testInput = () => { const tz = D.pos.z - Math.cos(h) * 15; let err = Math.atan2(-(x - D.pos.x), -(tz - D.pos.z)) - D.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
        return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: Math.max(-1, Math.min(1, (v - D.speed) * 0.5)) }; };
    },
    // brake to a stop, then nothing on the pedals (not into reverse)
    hold() { const D = T.L().drive; S47.world.testInput = () => ({ steer: 0, throttle: D.speed > 0.2 ? -1 : 0 }); },
    // the radio bus, its spectrum: how far the 760 Hz carrier stands above the speech band (dB)
    listen() {
      const a = S47.audio, ctx = a.ctx; if (T.an) return true;
      const an = ctx.createAnalyser(); an.fftSize = 4096; an.smoothingTimeConstant = 0; a.radioBus.connect(an); T.an = an; T.fd = new Float32Array(an.frequencyBinCount);
      return true;
    },
    tone() {
      const an = T.an, ctx = S47.audio.ctx; an.getFloatFrequencyData(T.fd);
      const bin = (f) => Math.round(f / (ctx.sampleRate / an.fftSize)), c = bin(760);
      let peak = -Infinity; for (let i = c - 2; i <= c + 2; i++) peak = Math.max(peak, T.fd[i]);
      const band = []; for (let i = bin(300); i <= bin(3000); i++) if (Math.abs(i - c) > 6) band.push(T.fd[i]);
      band.sort((p, q) => p - q);
      return { peak, above: peak - band[Math.floor(band.length / 2)] };
    },
    notes(s) { return S47.game.notes.filter((n) => n.includes(s)); },
    seen(w) { return S47.seen.filter((e) => e.what === w).map((e) => ({ kind: e.kind, line: e.line, looking: e.looking, place: e.place, at: e.at })); },
    // where the compass ball is on the screen (for a close look at it)
    compassBox() {
      const tk = T.L().truck, p = tk.compassAt(new V()).project(S47.camera), W = innerWidth, H = innerHeight;
      return { x: (p.x + 1) / 2 * W, y: (1 - p.y) / 2 * H };
    },
  };
  return true;
})()"""

async def page(b, w, h, touch, calm=False):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');}catch{}"
    await pg.add_init_script(init)
    if calm: await pg.add_init_script("try{localStorage.setItem('s47.calmFlash','true');}catch{}")
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    await pg.evaluate("localStorage.clear()")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]'); await begin_opening(pg, touch=True)
    else: await pg.click('button[data-a=start]'); await begin_opening(pg)
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

def dark_runs(samples, dt):
    """The runs of every light out in a list of light levels sampled every dt: [(start, length)]."""
    runs, start = [], None
    for i, v in enumerate(samples):
        if v < 0.1 and start is None: start = i
        if v >= 0.1 and start is not None: runs.append((start * dt, (i - start) * dt)); start = None
    if start is not None: runs.append((start * dt, (len(samples) - start) * dt))
    return runs

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])

        async def session(pg, errs, phone):
            ev = pg.evaluate
            tag = 'phone' if phone else 'desk'
            async def tick(s, n=1):
                for _ in range(n): await ev(f"S47.tick({s}, 20)")
            async def shot(name, crop=False):
                await pg.screenshot(path=f'{OUT}/{tag}_{name}.png')
                if crop:
                    c = await ev("T.compassBox()")
                    await pg.screenshot(path=f'{OUT}/{tag}_{name}_compass.png', clip={'x': max(0, c['x'] - 70), 'y': max(0, c['y'] - 45), 'width': 140, 'height': 90})
            st = lambda: ev("T.st()")
            async def quiet():
                await ev("S47.nightEvents['wait'] = 1e9; S47.setAbductions(false); if (S47.archive) S47.archive.off = true")
            async def get_in():
                await ev("S47.game.d.inter.get('truck').use()")
                for _ in range(120):
                    await tick(0.25)
                    if await ev("S47.world.driving"): return True
                return False
            async def out():
                await ev("S47.world.testInput = null; S47.world.stopDriving(); S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.3)")
            async def drive(x, z0, h, v, until, dt=0.05, max_s=40, on_step=None):
                """Drive the lane until until(state) (a python predicate); every state, sampled every dt."""
                await ev(f"T.lane({x}, {z0}, {h}, {v})")
                seen = []
                for i in range(int(max_s / dt)):
                    await tick(dt)
                    s = await st(); seen.append(s)
                    if on_step: await on_step(i, s)
                    if until(s): break
                return seen
            south = lambda z0=-90, v=20, **k: drive(-25.8, z0, 'Math.PI', v, **k)
            north = lambda z0=130, v=20, **k: drive(-22.2, z0, 0, v, **k)
            await ev("S47.hold = true")

            if not phone:
                # ---------- chapter one: nothing on B ----------
                await ev("S47.jump('chapter1'); S47.tick(1.0)"); await quiet()
                check(await get_in(), 'chapter one: in the truck at SARO')
                ss = await south(until=lambda s: s['z'] > 80)
                check(all(s['phase'] == 'idle' and s['kind'] is None for s in ss) and await ev("S47.truckB.crossings") == 0,
                      'chapter one: down the highway over B at 20 m/s, nothing happens (no rung, no trouble)')
                near = [s for s in ss if s['dist'] is not None and s['dist'] < 6]
                check(near and all(abs(s['compass'] - 180) <= 3 for s in ss[-40:]) and all(s['k'] == 0 for s in ss),
                      f'chapter one: the compass reads the heading, 180, also over B ({near[0]["compass"] if near else None})')
                ss = await north(until=lambda s: s['z'] < -40)
                check(min(s['compass'] if s['compass'] < 180 else 360 - s['compass'] for s in ss[-30:]) <= 3, 'the compass reads 000 heading north')
                await out()

            # ---------- chapter two: missed while something else goes, then four ----------
            await ev("S47.jump('chapter2'); S47.tick(1.0)"); await quiet()
            check(await get_in(), f'{tag}: chapter two, in the truck at SARO')
            if not phone:
                await ev("S47.motelTv.now(); S47.tick(0.1)")
                check(await ev("S47.motelTv.busy && S47.oneAtATime()"), 'the office television on its carrier: one of the night\'s things is going')
                ss = await south(until=lambda s: s['z'] > 80)
                check(all(s['phase'] == 'idle' and s['kind'] is None for s in ss) and await ev("S47.truckB.missed") == 1 and not await ev("S47.truckB.four"),
                      'over B while it goes: missed, nothing on top of it (one at a time)')
                await ev("S47.motelTv.reset(); S47.tick(0.2)")
                await ev("S47.carRadio.set(true, 'caliche'); T.listen()"); await tick(0.5)
                await ev("S47.game.d.ui.clearToasts()")
            n0 = await ev("S47.game.notes.length")
            p0 = await ev("T.plays.length")
            took0 = await ev("S47.curve.took.event"); seen0 = await ev("S47.seen.length")
            shots = {}
            tones = []
            async def step4(i, s):
                # (a few frames in: the radio takes the carrier on the frame after it is asked for)
                if s['phase'] == 'four' and s['carrying']: shots['nc'] = shots.get('nc', 0) + 1
                if shots.get('nc') == 3 and 'carrier' not in shots:
                    shots['carrier'] = 1
                    if not phone:
                        await pg.wait_for_timeout(250); tones.append(await ev("T.tone()"))
                    await shot('01_carrier', crop=True)
                if s['phase'] == 'four' and s['kind'] == 'blink' and s['level'] < 0.1 and s['blinked'] == 2 and 'dark' not in shots:
                    shots['dark'] = 1; await shot('02_dark', crop=True)
                if s['phase'] == 'four' and s['kind'] == 'blink' and s['level'] >= 0.99 and s['blinked'] == 3 and 'lit' not in shots:
                    shots['lit'] = 1; await shot('03_between', crop=True)
            # (down from north of the site, over B at the turn-off at 20 m/s)
            ss = await south(until=lambda s: s['phase'] == 'tail', on_step=step4)
            four = [s for s in ss if s['phase'] == 'four']
            check(four and four[0]['dist'] < 6 and four[0]['speed'] > 15, f'{tag}: over B at {four[0]["speed"]:.0f} m/s, {four[0]["dist"]:.1f} m from it: four begins' if four else f'{tag}: four begins')
            carry = [s for s in four if s['carrying']]
            check(carry and 0.9 <= len(carry) * 0.05 <= 1.3 and four[0]['carrying'], f'{tag}: the radio goes to a bare carrier at once, for about a second ({len(carry) * 0.05:.2f} s)')
            if not phone:
                t0 = tones[0] if tones else None
                after = await ev("T.tone()")
                check(t0 and t0['above'] > 25 and t0['above'] > after['above'] + 10, f'measured on the radio bus: the 760 Hz carrier stands {t0 and t0["above"]:.0f} dB over the band, Night Line back after ({after["above"]:.0f} dB)')
            sp = [s for s in four if s['kind'] == 'sputter']
            check(sp and min(s['power'] for s in sp) < 0.5 and all(s['engine'] for s in four), f'{tag}: the engine misses (the pull drops to {min(s["power"] for s in sp) if sp else 1:.2f}) and runs on')
            bl = [s['level'] for s in four if s['kind'] == 'blink']
            runs = dark_runs(bl, 0.05)
            gaps = [round(runs[i + 1][0] - runs[i][0], 2) for i in range(len(runs) - 1)]
            check(len(runs) == 4 and all(0.2 <= r[1] <= 0.3 for r in runs) and all(0.45 <= g <= 0.55 for g in gaps),
                  f'{tag}: every light out exactly four times, evenly: {len(runs)} dark runs of {[r[1] for r in runs]} s, {gaps} s apart')
            darks = [s for s in four if s['kind'] == 'blink' and s['level'] < 0.1]
            check(darks and all(s['dash'] is not None and s['dash'] < 0.1 for s in darks), f'{tag}: the dash out with the headlights in each')
            ticks = (await ev("T.plays.slice(%d)" % p0))
            nt = sum(1 for t in ticks if t in ('switch', 'lic:rack_switch'))
            check(nt == 4, f'{tag}: a relay ticks in the dash with each blink ({nt})')
            check(all(abs(s['compass'] - 26) <= 4 for s in four if s['kind'] == 'blink') and all(abs(s['bearing'] - 180) < 2 for s in four),
                  f'{tag}: the compass on the dash reads 026 through the count while the truck runs at 180 ({sorted(set(s["compass"] for s in four if s["kind"] == "blink"))})')
            check(all(s['busy'] and s['block'] for s in four), f'{tag}: one at a time while it runs, and no saving ({four[0]["block"]})')
            tl = await ev("T.toasts.slice(-6)")
            want0 = 'The radio drops to a bare carrier. The engine misses.' if not phone else 'The radio is off. A carrier comes out of it anyway. The engine misses.'
            check(want0 in tl and 'Four. Every light in the truck, four times. The compass on the dash reads 026.' in tl, f'{tag}: the toasts: the carrier and the miss, then "Four." ({tl[-2:]})')
            await tick(0.6); await shot('04_four', crop=True)
            nn = (await ev("S47.game.notes.slice()"))[n0:]
            on = [n for n in nn if 'went out four times' in n]
            check(len(on) == 1 and 'The turn-off to SARO, on B.' in on[0] and 'swung round to 026' in on[0] and ('bare carrier' in on[0] if not phone else 'was off and gave a carrier anyway' in on[0]),
                  f'{tag}: one note: {on[:1]}')
            check(await ev("S47.truckB.four && S47.townSeen.has('truckB:4') && [...S47.townSeen].some((k) => k.startsWith('mark:truck4@'))"), f'{tag}: kept with the town\'s part of the case, with a mark')
            sn = await ev("T.seen('truck4')")
            check(len(sn) == 1 and sn[0]['kind'] == 'event' and sn[0]['line'] == 'B' and 'in the truck' in sn[0]['place'], f'{tag}: the seen log: the event, on B, in the truck ({sn})')
            took = await ev("S47.curve.took.event"); landed = await ev("S47.curve.landed && S47.curve.landed.what")
            # (anything else the night gave meanwhile, the boy on Night Line say, counts as well)
            others = await ev(f"S47.seen.slice({seen0}).filter((e) => e.what !== 'truck4').map((e) => e.kind + ':' + e.what)")
            check(took >= took0 + 1 and took <= took0 + 1 + len(others) and landed != 'truck4', f'{tag}: the curve takes it as the build\'s event, not its peak (events {took0} to {took}, landed {landed}, others {others})')
            # its tail: nothing starts for a few seconds after, then free
            check(await ev("S47.oneAtATime()") and not await ev("S47.curve.may('event')"), f'{tag}: just after it, still one at a time')
            ss = await south(z0=120, v=12, until=lambda s: s['phase'] == 'idle', max_s=12)
            check(ss and ss[-1]['phase'] == 'idle' and not ss[-1]['busy'] and 7 <= len(ss) * 0.05 <= 9.5, f'{tag}: free again {len(ss) * 0.05:.1f} s later')

            if phone:
                await out()
            else:
                # ---------- again over B before room 6: nothing, only the compass ----------
                n1 = await ev("S47.game.notes.length")
                ss = await north(until=lambda s: s['z'] < -60)
                over = [s for s in ss if s['dist'] is not None and s['dist'] < 4]
                check(all(s['phase'] == 'idle' and s['kind'] is None for s in ss) and await ev("S47.truckB.crossings") >= 2 and await ev("S47.game.notes.length") == n1,
                      'over B again in chapter two: nothing (four is had)')
                cmp = [s['compass'] for s in ss if s['dist'] is not None and s['dist'] < 9]
                check(cmp and any(abs(c - 26) <= 12 for c in cmp) and min(abs(((s['compass'] + 180) % 360) - 180) for s in ss[-20:]) <= 3,
                      f'the compass still swings towards 026 over B ({min(cmp, key=lambda c: abs(c - 26)) if cmp else None}) and back to 000 after')
                await shot('05_compass_north', crop=True)
                # ---------- Continue ----------
                await out(); await tick(1.0)
                await ev("S47.saveNow('manual', 0)")
                await pg.reload()
                await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
                await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
                await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.3)"); await quiet()
                check(await ev("S47.truckB.four && !S47.truckB.seven") and len(await ev("T.notes('went out four times')")) == 1, 'Continue: four kept, its note once')
                check(await get_in(), 'Continue: in the truck again')
                ss = await south(until=lambda s: s['z'] > 80)
                check(all(s['phase'] == 'idle' and s['kind'] is None for s in ss) and len(await ev("T.notes('went out four times')")) == 1, 'Continue: over B, nothing given twice')
                await out()

            # ---------- after room 6: the engine dies, the key twice, seven ----------
            await ev("S47.jump('chapter5'); S47.tick(1.0)"); await quiet()
            check(await ev("S47.truckBIn().after6") and await ev("S47.game.phase") == 'ch5', f'{tag}: chapter five, room 6 is over')
            check(await get_in(), f'{tag}: chapter five, in the truck at SARO ("Drive to Mesa Diner")')
            await ev("S47.carRadio.set(false); S47.game.d.ui.clearToasts()"); await tick(0.3)
            n0 = await ev("S47.game.notes.length"); p0 = await ev("T.plays.length")
            if phone:
                ss = await south(until=lambda s: s['phase'] == 'stall' and s['key'], max_s=20)
            else:
                # the way every drive out of SARO goes: down the ramp, west along the service road and
                # round onto the highway at the turn-off, by the game's own autopilot
                await ev("S47.world.autopilot = 11")
                ss = []
                for _ in range(900):
                    await tick(0.05); s0 = await st(); ss.append(s0)
                    if s0['phase'] == 'stall' and s0['key']: break
                await ev("S47.world.autopilot = null")
                fs = next((s for s in ss if s['phase'] == 'stall'), None)
                where = f'x {fs["x"]:.1f}, z {fs["z"]:.1f}, {fs["speed"]:.1f} m/s' if fs else 'never'
                check(fs and -27 < fs['x'] < -12 and 18 < fs['z'] < 34 and fs['speed'] > 2,
                      f'out of SARO the usual way: it comes as the truck turns out of the service road onto the highway ({where})')
            st0 = ss[-1]
            first = next((s for s in ss if s['phase'] == 'stall'), None)
            check(first and first['dist'] < 6, f'{tag}: over B: the stall begins ({first and first["dist"]:.1f} m from B)')
            check(not st0['engine'] and 'OIL' in st0['lamps'] and 'VOLTS' in st0['lamps'] and st0['level'] < 0.8, f'{tag}: the engine dead, OIL and VOLTS lit, the lights on the battery ({st0["lamps"]}, {st0["level"]:.2f})')
            check(st0['carrying'] and not st0['radioOn'], f'{tag}: the radio, switched off, on a bare carrier')
            await ev("S47.game.d.ui.clearToasts()"); await tick(0.3)
            st0 = await st()
            check(st0['key'] and st0['prompt'] == 'Turn the key' and not st0['out'] and st0['canOut'], f'{tag}: "Turn the key"; no getting out until it runs ({st0["prompt"]})')
            await shot('06_dead_key', crop=True)
            await ev("T.hold()"); await tick(0.25, 16)
            st0 = await st()
            check(st0['key'] and not st0['engine'] and abs(st0['speed']) < 0.3, f'{tag}: it waits, dead, the truck stopped on the line ({st0["speed"]:.2f} m/s)')
            async def turn():
                if phone:
                    use = pg.locator('.touch .use')
                    vis = await use.is_visible()
                    bb = await use.bounding_box()
                    await pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
                    return vis
                await pg.keyboard.press('KeyE'); return True
            vis = await turn()
            tries = []
            for _ in range(60):
                await tick(0.05); s = await st(); tries.append(s)
                if s['stage'] == 'pause' and s['key']: break
            check(vis and any(s['stage'] == 'cranking' for s in tries) and tries[-1]['stage'] == 'pause' and not tries[-1]['engine'] and await ev("S47.world.trouble.turns") == 1,
                  f'{tag}: {"the Use button" if phone else "E"} turns the key: the starter turns it over, and it does not catch')
            check('It turns over and will not catch.' in await ev("T.toasts.slice(-3)"), f'{tag}: "It turns over and will not catch."')
            await tick(0.3); await shot('07_wont_catch', crop=True)
            await turn()
            seq = []
            async def step7(i, s):
                if s['phase'] == 'seven' and s['level'] < 0.1 and s['blinked'] == 4 and 'dark7' not in shots:
                    shots['dark7'] = 1; await shot('08_seven_dark', crop=True)
            for i in range(300):
                await tick(0.05); s = await st(); seq.append(s); await step7(i, s)
                if s['phase'] == 'tail': break
            caught = next((i for i, s in enumerate(seq) if s['engine']), None)
            check(caught is not None and await ev("S47.world.trouble.turns") == 2, f'{tag}: the second turn: it catches')
            sev = [s for s in seq if s['phase'] == 'seven']
            runs = dark_runs([s['level'] for s in sev if s['kind'] == 'blink'], 0.05)
            gaps = [round(runs[i + 1][0] - runs[i][0], 2) for i in range(len(runs) - 1)]
            start7 = next((i for i, s in enumerate(seq) if s['phase'] == 'seven'), None)
            check(len(runs) == 7 and all(0.2 <= r[1] <= 0.3 for r in runs) and all(0.45 <= g <= 0.55 for g in gaps), f'{tag}: then every light out exactly seven times, evenly ({len(runs)}, {gaps})')
            gap7 = (start7 - caught) * 0.05 if start7 is not None and caught is not None else None
            check(gap7 is not None and 0.8 <= gap7 <= 2.4, f'{tag}: a breath between the catch and the seven ({gap7} s)')
            check(all(s['carrying'] for s in sev if s['kind'] == 'blink') and not seq[-1]['carrying'], f'{tag}: the carrier holds through the seven, and lets go after')
            check(all(not s['canOut'] for s in sev) and seq[-1]['canOut'] == (await st())['out'], f'{tag}: no getting out in the middle of the seven (the truck stands, the engine runs); after it, as ever')
            nt = sum(1 for t in (await ev("T.plays.slice(%d)" % p0)) if t in ('switch', 'lic:rack_switch'))
            check(nt == 7, f'{tag}: the relay with each of the seven ({nt})')
            check(all(abs(s['compass'] - 26) <= 4 for s in sev), f'{tag}: the compass on 026 through it')
            check('Seven.' in await ev("T.toasts.slice(-2)"), f'{tag}: "Seven."')
            await tick(0.5); await shot('09_seven', crop=True)
            nn = [n for n in (await ev("S47.game.notes.slice()"))[n0:] if 'seven times' in n]
            check(len(nn) == 1 and 'The engine died under me, OIL and VOLTS lit' in nn[0] and 'second turn of the key' in nn[0] and nn[0].endswith('Four, the last time.') and ', again.' in nn[0],
                  f'{tag}: one note: {nn[:1]}')
            check(await ev("S47.truckB.seven && S47.townSeen.has('truckB:7')"), f'{tag}: kept')
            check(await ev("S47.curve.landed && S47.curve.landed.what") == 'truck7' and await ev("S47.curve.phase") == 'rest', f'{tag}: the night\'s curve lands on it, and rests')
            sn = await ev("T.seen('truck7')")
            check(len(sn) == 1 and sn[0]['line'] == 'B', f'{tag}: the seen log ({sn})')
            if not phone:
                ss = await north(until=lambda s: s['z'] < -60)
                check(all(s['phase'] in ('idle', 'tail') and s['kind'] is None for s in ss), 'after seven, over B again: nothing more')
                # Continue after seven: kept, nothing given twice
                await out(); await tick(1.0)
                await ev("S47.saveNow('manual', 0)")
                await pg.reload()
                await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
                await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
                await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.3)"); await quiet()
                check(await ev("S47.truckB.four && S47.truckB.seven") and len(await ev("T.notes('went out seven times')")) == 1, 'Continue: seven kept, its note once')
                check(await get_in(), 'Continue: in the truck again')
                ss = await south(until=lambda s: s['z'] > 80)
                check(all(s['phase'] == 'idle' and s['kind'] is None for s in ss) and len(await ev("T.notes('went out seven times')")) == 1, 'Continue: over B, nothing given twice')
                mk = [m for m in await ev("S47.mapMarks()") if m['kind'] in ('truck4', 'truck7')]
                check(len(mk) == 2 and all(m['line'] == 'B' and m['at']['spot'] == 'highway' and abs(m['at']['m'] - 23.4) < 1 for m in mk), f'the map: two marks on B at the turn-off ({mk})')
                await out()
                await ev("S47.game.openNotebook()"); await tick(0.2)
                await pg.click('.notebook [data-t=map]'); await pg.wait_for_timeout(400)
                await shot('10_map')
                await ev("S47.game.d.ui.close(true)"); await tick(0.2)

                # ---------- the way most come out of room 6: the truck on the motel's lot ----------
                # (a driver leaves it there in chapter four; the drive south to the diner never goes over B)
                await ev("for (const k of [...S47.townSeen]) if (k.startsWith('truckB:7') || k.startsWith('mark:truck7')) S47.townSeen.delete(k)")
                await ev("S47.jump('chapter5'); S47.tick(1.0)"); await quiet()
                await ev("S47.world.placeTruck('motel'); S47.tick(0.3)")
                check(await ev("S47.truckB.four && !S47.truckB.seven && S47.world.truckAt === 'motel' && S47.truckBIn().after6"), 'out of room 6, the truck on the motel\'s lot, where a driver leaves it')
                check(await ev("S47.game.d.inter.get('truck').label()") == 'Drive to Mesa Diner' and await get_in(), 'in the truck on the motel\'s lot: "Drive to Mesa Diner"')
                # (the station on the air at this hour: Night Line signs off at four, Roswell on at five)
                sta = await ev("S47.radio.stations.roswell.onAir ? 'roswell' : 'caliche'")
                await ev(f"S47.carRadio.set(true, '{sta}'); S47.game.d.ui.clearToasts()"); await tick(0.3)
                n0 = await ev("S47.game.notes.length"); p0 = await ev("T.plays.length")
                await ev("S47.world.autopilot = 13")
                ss = []
                for _ in range(600):
                    await tick(0.05); s0 = await st(); ss.append(s0)
                    if s0['phase'] == 'stall' and s0['key']: break
                await ev("S47.world.autopilot = null")
                fs = next((s for s in ss if s['phase'] == 'stall'), None)
                trip = await ev("S47.world.tripNow")
                check(fs is not None and trip and trip['from'] == 'motel' and trip['metres'] < 60 and fs['speed'] > 3,
                      f'pulling out of the motel\'s lot onto the highway, the engine dies ({trip and round(trip["metres"])} m out, {fs and fs["speed"]:.1f} m/s, {fs and fs["dist"]:.0f} m from B)')
                st0 = await st()
                check(not st0['engine'] and 'OIL' in st0['lamps'] and 'VOLTS' in st0['lamps'] and st0['carrying'] and st0['key'], 'there: dead, OIL and VOLTS, the carrier, "Turn the key"')
                # a station line while only the carrier plays: not captioned
                strong = await ev(f"S47.radio.stations['{sta}'].strength(S47.radio.clock, S47.radio.time)")
                await ev(f"S47.radio.onLine('{sta}', 'A line under the carrier.', null, 2)")
                check(strong >= 0.35 and 'RADIO: "A line under the carrier."' not in await ev("T.toasts.slice(-4)"), f'a station line under the bare carrier is not captioned ({sta}, {strong:.2f})')
                await ev("T.hold()"); await tick(0.25, 8)
                await pg.keyboard.press('KeyE')
                for _ in range(60):
                    await tick(0.05); s = await st()
                    if s['stage'] == 'pause' and s['key']: break
                check(not s['engine'] and await ev("S47.world.trouble.turns") == 1, 'E: it turns over and does not catch')
                await pg.keyboard.press('KeyE')
                seq = []
                for _ in range(300):
                    await tick(0.05); s = await st(); seq.append(s)
                    if s['phase'] == 'seven' and s['level'] < 0.1 and s['blinked'] == 3 and 'motel7' not in shots:
                        shots['motel7'] = 1; await shot('12_motel_seven_dark', crop=True)
                    if s['phase'] == 'tail': break
                runs = dark_runs([s['level'] for s in seq if s['phase'] == 'seven' and s['kind'] == 'blink'], 0.05)
                check(len(runs) == 7 and await ev("S47.world.trouble.turns") == 2, f'E again: it catches, then every light out seven times ({len(runs)})')
                nn = [n for n in (await ev("S47.game.notes.slice()"))[n0:] if 'seven times' in n]
                check(len(nn) == 1 and 'Out of Sierra Motor Court onto the highway.' in nn[0] and nn[0].endswith('Four, the last time.'), f'the note: {nn[:1]}')
                sn = await ev("T.seen('truck7m')")
                check(len(sn) == 1 and sn[0]['kind'] == 'event' and sn[0]['line'] == 'B', f'the seen log: the event, on B ({sn})')
                mk = [m for m in await ev("S47.mapMarks()") if m['kind'] == 'truck7m']
                check(len(mk) == 1 and mk[0]['line'] == 'B' and mk[0]['at']['spot'] == 'highway' and abs(mk[0]['at']['m'] - 661) < 3, f'the map: a mark on B in front of the motel ({mk})')
                await tick(0.5); await shot('13_motel_seven', crop=True)
                # (and once the carrier has let go, a station line is captioned again, as ever)
                await ev(f"S47.radio.onLine('{sta}', 'A line after the carrier.', null, 2)")
                check(not (await st())['carrying'] and 'RADIO: "A line after the carrier."' in await ev("T.toasts.slice(-4)"), 'after it, a station line is captioned again')
                await ev("S47.world.testInput = null"); await out()
                await ev("S47.game.openNotebook()"); await tick(0.2)
                await pg.click('.notebook [data-t=map]'); await pg.wait_for_timeout(400)
                await shot('14_map_motel')
                await ev("S47.game.d.ui.close(true)"); await tick(0.2)

                # ---------- the old road: the compass slides to 026 from mile 6 ----------
                await ev("S47.jump('chapter6'); S47.tick(0.5)")
                for _ in range(80):
                    await tick(0.25)
                    if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
                async def at_mile(mi):
                    await ev(f"""(() => {{ const r = S47.world.oldRoad, d = S47.world.oldDrive, s = {(mi - 8.15) * MILE};
                      const a = r.lane(s), b2 = r.lane(s + 4); d.place(a, Math.atan2(-(b2.x - a.x), -(b2.z - a.z))); S47.world.testInput = {{ steer: 0, throttle: 0 }}; }})()""")
                    await tick(0.05, 80)
                    return await ev("({ c: S47.world.oldTruck.compassReads, b: S47.world.oldTruck.bearing, mi: S47.world.oldRoad.where(S47.world.oldDrive.pos.x, S47.world.oldDrive.pos.z).mi, k: S47.truckB.pull ? S47.truckB.pull.k : 0 })")
                m6 = await at_mile(6.0)
                check(abs(m6['c'] - m6['b']) <= 3 and 55 < m6['b'] < 70, f'the old road, mile {m6["mi"]:.2f}: the compass reads the road, {m6["c"]:03d} ({m6["b"]:.0f})')
                m69 = await at_mile(6.9)
                check(26 + 4 < m69['c'] < m69['b'] - 2, f'mile {m69["mi"]:.2f}: it has slid off the road ({m69["b"]:.0f}) towards 026: {m69["c"]:03d}')
                m78 = await at_mile(7.8)
                check(abs(m78['c'] - 26) <= 2 and m78['b'] > 33, f'mile {m78["mi"]:.2f}: on 026 ({m78["c"]:03d}) while the road runs {m78["b"]:.0f}')
                check(len([t for t in await ev("T.toasts") if t == 'The compass on the dash has come round to 026. The road has not.']) == 1, 'the toast, once: "The compass on the dash has come round to 026. The road has not."')
                await ev("S47.game.d.ui.clearToasts()"); await tick(0.3)
                await shot('11_oldroad_026', crop=True)
                await ev("S47.world.testInput = null")
            await pg.wait_for_timeout(300)
            bad = [e for e in errs if 'GPU stall' not in e]
            print('\n'.join(bad[:30]) or f'{tag}: no console errors/warnings')
            check(not bad, f'{tag}: console clean')

        async def calm_session(pg, errs):
            ev = pg.evaluate
            async def tick(s, n=1):
                for _ in range(n): await ev(f"S47.tick({s}, 20)")
            await ev("S47.hold = true; S47.jump('chapter2'); S47.tick(1.0)")
            await ev("S47.nightEvents['wait'] = 1e9; S47.setAbductions(false); if (S47.archive) S47.archive.off = true")
            await ev("S47.game.d.inter.get('truck').use()")
            for _ in range(120):
                await tick(0.25)
                if await ev("S47.world.driving"): break
            p0 = await ev("T.plays.length")
            await ev("T.lane(-25.8, -90, Math.PI, 20)")
            ss = []
            for _ in range(800):
                await tick(0.05); s = await ev("T.st()"); ss.append(s)
                if s['phase'] == 'tail': break
            lv = [s['level'] for s in ss if s['phase'] == 'four' and s['kind'] == 'blink']
            dips = sum(1 for i in range(1, len(lv) - 1) if lv[i] < 0.75 and lv[i] <= lv[i - 1] and lv[i] < lv[i + 1])
            nt = sum(1 for t in (await ev("T.plays.slice(%d)" % p0)) if t in ('switch', 'lic:rack_switch'))
            check(lv and min(lv) > 0.35 and not dark_runs(lv, 0.05) and dips == 4 and nt == 4 and await ev("S47.truckB.four"),
                  f'slow fades: the four\'s blinks dip softly, never out (lowest {min(lv) if lv else None:.2f}), four of them ({dips} dips, {nt} ticks)')
            await ev("S47.world.testInput = null")
            bad = [e for e in errs if 'GPU stall' not in e]
            check(not bad, f'slow fades: console clean {bad[:3]}')

        # ======================= desktop =======================
        ctx, pg, errs = await page(b, 1280, 720, False)
        await session(pg, errs, False)
        await ctx.close()
        # ======================= Flashing images: slow fades =======================
        ctx, pg, errs = await page(b, 1280, 720, False, calm=True)
        await calm_session(pg, errs)
        await ctx.close()
        # ======================= phone, touch =======================
        ctx, pg, errs = await page(b, 412, 892, True)
        check(await pg.evaluate("S47.input.touchMode"), 'phone: touch controls')
        await session(pg, errs, True)
        await ctx.close()

        json.dump(checks, open(f'{OUT}/checks-car-b.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
