# Taken in the truck (story/truckTaken.ts; FORSLAG.md, 20 October, item 10). Drives the truck south
# over B at SARO's turn-off, fast, with the game's own drive (a lane-keeping input through
# S47.world.testInput), and checks what the player gets:
#   - chapter one, not at the night's peak: nothing; at the peak northbound: nothing; with Missing
#     time turned off: nothing
#   - at the peak, southbound at 20 m/s: the warning in the cab. The corner clock in amber seconds,
#     the radio on a bare carrier (off as it is), the engine dead (OIL and VOLTS), the headlights
#     sunk, the truck brought to a stop with no brakes; "Turn the key" and "Get out" (Q); no saving,
#     one at a time; the seen log on B. :16 the beep and no sound of the truck at all (the starter
#     turns in silence and never catches); :31 down; :40 every light out, the warning lamps and the
#     radio with them, the key does nothing; :41 three knocks on the roof; :47 the light from above
#     into the cab. Then at the wheel 300 m further on, in the southbound lane with the nose north,
#     the clock 17 to 47 minutes on: one of them on the bonnet, bent to the glass, black against
#     SARO's lamps (it stands out: the frame with it against the same frame without it), the HUD
#     gone; the eyes going up to it and it is not there; the instruments come on, OIL and VOLTS, the
#     radio on 1090, frost on the glass and round the truck, the odometer 31.4 miles more than it
#     read at SARO (read off the dash), the notes, the extra exposure (the truck from above), kept
#     with the case (the spot in the truck, nobody standing for it), ne:knocks1 (the night's knocks
#     were owed: these were them), the curve lands on it; the key catches the first time and the
#     headlights come up. Saved where it was put down; Continue: the truck there, the odometer as it
#     was, and never again
#   - chapter two, four not had yet: the first pass over B is the ladder's (four, not the warning);
#     the next pass at the peak: the warning; E three times quickly: it catches on the third, it
#     passes (the note); another pass at a later peak: out (Q) and running north up the shoulder,
#     30 m from the truck: it passes (the note); a third time: no more (twice a night)
#   - out of the truck but standing by it at :47: taken all the same; on the verge beside the truck,
#     which is 300 m on, nose north, its radio on 1090
#   - Missing time turned off in the middle: it passes, the truck its own again
#   - chapter four, the usual way (the critic): the story's drive from the pad to the motel with
#     N. Vega's CB call due; the warning at the turn-off before the call, which waits; E as the engine
#     dies keeps the player at the wheel; the key; the CB after; the second warning in the truck the
#     key never catches ("The third turn. It does not catch this time."), out and away, the toast
#   - phone 412x892 with touch: the Use button turns the key and the Get out button is there with it;
#     taken; the one on the bonnet in a portrait view; the odometer; screenshots
#   - no console errors or warnings
# Usage (from web/): python3 tools/f_car_taken.py OUTDIR
import asyncio, sys, json, os, math, io
from playwright.async_api import async_playwright
from PIL import Image

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
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
      const w = S47.world, L = w.leg ? w.legs[w.leg] : null, tk = L ? L.truck : w.saroTruck, D = L ? L.drive : null, ab = S47.abduction(), TT = S47.truckTaken;
      const lamps = Object.entries(tk['lamps']).filter(([k, m]) => m.visible).map(([k]) => k);
      const pr = document.querySelector('.hud .prompt'), use = document.querySelector('.touch .use'), alt = document.querySelector('.touch .alt');
      const said = S47.input.touchMode ? (use && use.classList.contains('ready') ? use.textContent : '') : pr && pr.classList.contains('show') ? pr.textContent : '';
      const clk = document.querySelector('.clockline');
      return { phase: TT.phase, free: TT.free, ended: TT.ended, tries: TT.tries, dist: TT.dist, stage: ab.stage, cab: ab.cab, warn: ab.warn ? ab.warn.t : null, over: ab.overPhase,
        kind: w.trouble.kind, tstage: w.trouble.stage, turns: w.trouble.turns, dim: w.trouble.dim, level: tk['level'], dash: tk['dashLevel'], headOn: tk['headOn'],
        engine: D ? D.engine : null, held: D ? D.held : null, speed: D ? D.speed : 0, x: D ? D.pos.x : tk.group.position.x, z: D ? D.pos.z : tk.group.position.z, h: D ? D.heading : tk.group.rotation.y,
        lamps, key: w.trouble.wantsKey, driving: w.driving, hushed: !!(w['engine'] && w['engine'].hushed),
        carrying: S47.carRadio.carrying, radioOn: S47.carRadio.on, station: S47.carRadio.station, power: S47.carRadio.power,
        busy: S47.oneAtATime(), block: S47.saveBlock(), prompt: said, alt: alt && !alt.hidden ? alt.textContent : null,
        clock: S47.game.clock, clockText: clk ? clk.textContent : '', secs: clk ? clk.classList.contains('secs') : false,
        held2: document.documentElement.classList.contains('abduct-held'), odo: tk.odometer, frost: tk.frosted, phx: S47.player.pos.x, phz: S47.player.pos.z };
    },
    // the truck in the southbound lane of the highway through SARO's site, driven at v m/s by the
    // game's own drive (the throttle off once the warning has it: the engine is dead anyway)
    south(z, v) {
      const w = S47.world, D = T.L().drive; D.place(new V(-25.8, 0, z), Math.PI);
      w.testInput = () => { const tz = D.pos.z + 15; let err = Math.atan2(-(-25.8 - D.pos.x), -(tz - D.pos.z)) - D.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
        return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: S47.truckTaken.phase === 'warning' ? 0 : Math.max(-1, Math.min(1, (v - D.speed) * 0.5)) }; };
    },
    north(z, v) {
      const w = S47.world, D = T.L().drive; D.place(new V(-22.2, 0, z), 0);
      w.testInput = () => { const tz = D.pos.z - 15; let err = Math.atan2(-(-22.2 - D.pos.x), -(tz - D.pos.z)) - D.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
        return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: Math.max(-1, Math.min(1, (v - D.speed) * 0.5)) }; };
    },
    stop() { S47.world.testInput = null; },
    notes(s) { return S47.game.notes.filter((n) => n.includes(s)); },
    seen(k, w) { return S47.seen.filter((e) => e.kind === k && (!w || e.what === w)).map((e) => ({ kind: e.kind, what: e.what, line: e.line, looking: e.looking, place: e.place, end: e.end, at: e.at })); },
    // the one on the bonnet: where it stands in the truck's own metres, and its head on the screen
    overAt() {
      const tk = T.L() ? T.L().truck : null, c = S47.camera; c.updateMatrixWorld(true);
      const out = []; S47.scene.traverse((o) => { if (o.name === 'abduct:over' && o.visible && o.parent) {
        const l = tk ? tk.group.worldToLocal(o.position.clone()) : null; out.push({ x: l && l.x, y: l && l.y, z: l && l.z, card: !!o.getObjectByName('abduct:over:card') }); } });
      return out;
    },
    heads() { const c = S47.camera; c.updateMatrixWorld(true); const r = new V(1, 0, 0).applyQuaternion(c.quaternion);
      return S47.abduction().overHeads.map((h) => { const a = new V(...h).project(c), b = new V(...h).addScaledVector(r, 0.11).project(c);
        return { x: (a.x + 1) / 2 * innerWidth, y: (1 - a.y) / 2 * innerHeight, r: Math.abs(b.x - a.x) / 2 * innerWidth }; }); },
    // the odometer's window on the screen (the upper face of the speedometer, over its hub)
    odoBox() { const tk = T.L().truck; const p = tk.group.localToWorld(new V(-0.42, 1.183, -0.39)).project(S47.camera);
      return { x: (p.x + 1) / 2 * innerWidth, y: (1 - p.y) / 2 * innerHeight }; },
    rings() { const out = []; S47.scene.traverse((o) => { if (o.name === 'abduct:ring') { o.geometry.computeBoundingSphere(); out.push({ x: o.position.x, z: o.position.z, r: o.geometry.boundingSphere.radius }); } }); return out; },
  };
  return true;
})()"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');}catch{}")
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    await pg.evaluate("localStorage.clear()")
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("T.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        all_errs = []

        async def session(pg, tag):
            ev = pg.evaluate
            phone = tag == 'phone'
            async def tick(s, n=1):
                for _ in range(n): await ev(f"S47.tick({s}, 20)")
            async def shot(name):
                return await pg.screenshot(path=f'{OUT}/{tag}_{name}.png')
            st = lambda: ev("T.st()")
            async def quiet():
                await ev("S47.nightEvents['wait'] = 1e9; if (S47.archive) S47.archive.off = true; S47.game.d.ui.close(); S47.game.d.ui.clearToasts()")
            async def get_in():
                await ev("S47.game.d.inter.get('truck').use()")
                for _ in range(120):
                    await tick(0.25)
                    if await ev("S47.world.driving"): return True
                return False
            async def chapter(c, truckB_off=True):
                await ev(f"S47.hold = true; S47.jump('{c}'); S47.tick(1.0)"); await quiet()
                if truckB_off: await ev("S47.truckB.off = true")
                await ev("S47.setAbductions(true)")
                return await get_in()
            async def drive(fn, until, dt=0.05, max_s=40):
                seen = []
                for i in range(int(max_s / dt)):
                    await tick(dt); s = await st(); seen.append(s)
                    if until(s): break
                return seen
            async def south(z0=-90, v=20, until=None, max_s=14):
                await ev(f"T.south({z0}, {v})")
                return await drive(None, until or (lambda s: s['z'] > 90 or s['phase'] == 'warning'), max_s=max_s)
            async def north(z0=120, v=20, max_s=12):
                await ev(f"T.north({z0}, {v})")
                return await drive(None, lambda s: s['z'] < -70, max_s=max_s)
            async def peak():
                await ev("S47.curve.free = false; S47.curve.force('peak'); S47.tick(0.05)")
            async def turn():
                if phone:
                    use = pg.locator('.touch .use'); bb = await use.bounding_box()
                    await pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
                else: await pg.keyboard.press('KeyE')
            async def get_out():
                if phone:
                    alt = pg.locator('.touch .alt'); bb = await alt.bounding_box()
                    await pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
                else: await pg.keyboard.press('KeyQ')
            async def to_warn(t):
                for _ in range(400):
                    s = await st()
                    if s['warn'] is None or s['warn'] >= t: return s
                    await tick(min(0.25, max(0.034, t - s['warn'])))
                return await st()
            async def stands_out(name):
                """The frame with them, the same without them: round each head, how much darker it is with them."""
                heads = await ev("T.heads()")
                a = Image.open(io.BytesIO(await shot(name))).convert('L')
                await ev("S47.abduction().hideOver(true); S47.tick(1 / 30)")
                bb = Image.open(io.BytesIO(await pg.screenshot(path=f'{OUT}/{tag}_{name}_without.png'))).convert('L')
                await ev("S47.abduction().hideOver(false); S47.tick(1 / 30)")
                out = []
                for h in heads:
                    r = max(3, h['r']); box = (int(h['x'] - r), int(h['y'] - r), int(h['x'] + r) + 1, int(h['y'] + r) + 1)
                    pa, pb = a.crop(box).tobytes(), bb.crop(box).tobytes()
                    out.append({'dark': round(sum(y - x for x, y in zip(pa, pb)) / len(pa), 1), 'part': round(sum(1 for x, y in zip(pa, pb) if y - x >= 4) / len(pa), 2), 'at': [round(h['x']), round(h['y'])]})
                return out

            # ===================== chapter one: when it does not come, then taken =====================
            if not phone:
                check(await chapter('chapter1'), 'chapter one: in the truck at SARO')
                await ev("S47.curve.free = false; S47.fcam.have = true")
                ph = await ev("S47.curve.phase")
                ss = await south()
                check(ph != 'peak' and all(s['phase'] == 'idle' and s['stage'] == 'calm' and s['kind'] is None for s in ss) and min(s['dist'] for s in ss if s['dist'] is not None) < 6,
                      f'not at the night\'s peak ({ph}): south over B at 20 m/s, nothing')
                await peak()
                ss = await north()
                check(all(s['phase'] == 'idle' and s['stage'] == 'calm' for s in ss), 'at the peak, northbound over B: nothing (it is the way south it comes)')
                await ev("S47.setAbductions(false)"); await peak()
                ss = await south()
                check(all(s['phase'] == 'idle' and s['stage'] == 'calm' for s in ss), 'Missing time turned off, at the peak, south over B: nothing')
                await ev("S47.setAbductions(true)"); await peak()
                await ev("S47.carRadio.set(false); S47.game.d.ui.clearToasts()")
                n0 = await ev("S47.game.notes.length"); p0 = await ev("T.plays.length"); seen0 = await ev("S47.seen.length")
                odo_saro = await ev("S47.world.odoAtSaro")
                ss = await south()
                w0 = ss[-1]
                check(w0['phase'] == 'warning' and w0['stage'] == 'warning' and w0['cab'] and w0['dist'] < 6 and w0['speed'] > 15 and abs(w0['h'] - math.pi) < 0.2,
                      f'at the peak, south over B at {w0["speed"]:.0f} m/s, {w0["dist"]:.1f} m from it: the warning comes in the cab')
                await tick(0.05)
                s = await st()
                check(s['secs'] and len(s['clockText']) == 8, f'the corner clock goes over to seconds ({s["clockText"]})')
                check(s['carrying'] and not s['radioOn'], 'the radio, switched off, gives a bare carrier anyway')
                check(s['kind'] == 'stall' and 'A carrier comes out of it anyway. The engine dies.' in ' '.join(await ev("T.toasts.slice(-3)")), 'the engine dies (the toast)')
                ss = await drive(None, lambda s: abs(s['speed']) < 0.05, max_s=12)
                s = ss[-1]
                check(abs(s['speed']) < 0.05 and s['held'] > 2 and s['engine'] is False and (s['warn'] or 0) < 9, f'the truck slowed to a stop on its own, no brakes ({s["warn"]:.1f} s in)')
                check('OIL' in s['lamps'] and 'VOLTS' in s['lamps'] and 0.5 < s['level'] < 0.8, f'OIL and VOLTS; the headlights sunk onto the battery ({s["level"]:.2f})')
                check(s['busy'] and s['block'] and s['stage'] == 'warning', f'one at a time, and no saving ({s["block"]})')
                await tick(0.4); s = await st()
                check('Turn the key' in s['prompt'] and 'Get out' in s['prompt'], f'"Turn the key", and Q: "Get out" ({s["prompt"]})')
                sn = await ev("T.seen('abduction')")
                check(any(e['what'] == 'truck' and e['line'] == 'B' and 'in the truck' in e['place'] for e in sn) and any(e['what'] == 'warning' for e in sn), f'the seen log: the warning, and in the truck on B ({[e["what"] for e in sn]})')
                await ev("S47.game.d.ui.clearToasts()"); await tick(0.1)
                await shot('01_dead_on_B')
                s = await to_warn(16.3)
                check(s['hushed'] and s['warn'] >= 16, f':16, the beep: no sound of the truck at all ({s["warn"]:.1f} s)')
                await turn()
                ss = await drive(None, lambda s: s['tstage'] == 'pause', max_s=3)
                check(any(x['tstage'] == 'cranking' for x in ss) and ss[-1]['tstage'] == 'pause' and ss[-1]['engine'] is False and ss[-1]['turns'] == 1,
                      'after the beep, E: the starter turns it over, and it does not catch')
                check('The starter turns. There is no sound at all.' in await ev("T.toasts.slice(-3)"), '"The starter turns. There is no sound at all."')
                s = await to_warn(35.5)
                check(0.3 < s['dim'] < 0.9 and s['level'] < 0.6, f':31 on, the battery goes down: the lights at {s["level"]:.2f}')
                s = await to_warn(40.5)
                check(s['dim'] == 0 and s['level'] < 0.02 and (s['dash'] or 0) < 0.02 and not s['lamps'] and not s['carrying'] and s['power'] < 0.02,
                      f':40, every light in the truck out, the warning lamps and the radio with them ({s["lamps"]})')
                await turn(); await tick(0.1)
                check('The key turns. Nothing. Not even the starter.' in await ev("T.toasts.slice(-3)") and (await st())['tstage'] != 'cranking', 'then the key does nothing: not even the starter')
                k0 = await ev("T.plays.length")
                s = await to_warn(43.6)
                kn = [x for x in await ev(f"T.plays.slice({k0})") if x == 'thudSoft']
                check(len(kn) == 3, f':41, three knocks on the roof of the cab ({len(kn)})')
                await ev("S47.game.d.ui.clearToasts()"); await tick(0.05)
                await shot('02_dark_knocks')
                s = await to_warn(47)
                ss = await drive(None, lambda s: s['stage'] == 'light', max_s=2)
                await tick(1.2)
                above = await ev("T.L().truck['above']")
                check(ss[-1]['stage'] in ('light', 'taken') and above > 0.5, f':47, the light from above, into the cab ({above:.2f})')
                await shot('03_light')
                t_take = await ev("S47.abduction()['take'].z")
                clock0 = ss[-1]['clock']
                ss = await drive(None, lambda s: s['over'] == 'stars' or s['stage'] == 'back', dt=0.25, max_s=20)
                s = ss[-1]
                lost = (s['clock'] - clock0) / 60
                check(s['stage'] == 'back' and s['driving'], 'woken, still at the wheel')
                check(abs(s['x'] + 25.8) < 0.2 and abs(s['z'] - (t_take + 300)) < 1.5 and abs(math.atan2(math.sin(s['h']), math.cos(s['h']))) < 0.05,
                      f'300 m further on, in the southbound lane, the nose to the north (taken at z {t_take:.0f}, woken at x {s["x"]:.1f}, z {s["z"]:.0f}, heading {s["h"]:.2f})')
                check(16.5 <= lost <= 47.5, f'the time away: {lost:.0f} minutes')
                check(s['kind'] == 'stall' and s['engine'] is False and not s['headOn'] and s['dim'] == 0 and s['held2'], 'the engine off, every light off, nothing on the screen')
                ss = await drive(None, lambda s: s['over'] == 'there', dt=1 / 30, max_s=4)
                s = ss[-1]
                fig = await ev("T.overAt()")
                check(s['over'] == 'there' and len(fig) == 1 and fig[0]['card'] and -2.65 < fig[0]['z'] < -0.9 and abs(fig[0]['x']) < 0.85 and 0.9 < fig[0]['y'] < 1.35,
                      f'the head comes up: one of them on the bonnet, the painted outline ({fig})')
                so = await stands_out('04_bonnet')
                check(so and all(x['dark'] >= 5 and x['part'] >= 0.5 for x in so), f'it stands out, black against SARO\'s lamps and the sky ({so})')
                check(await ev("getComputedStyle(document.querySelector('.hud')).visibility") == 'hidden', 'no HUD while it is there')
                await ev("S47.input.lookDY = -0.3"); await tick(1 / 30)
                await tick(1 / 30)
                s = await st()
                check(s['over'] is None and await ev("!S47.scene.getObjectByName('abduct:over') || !S47.scene.getObjectByName('abduct:over').visible"), 'the eyes go up to it: it is not there')
                ss = await drive(None, lambda s: s['dim'] >= 1, dt=0.1, max_s=4)
                s = ss[-1]
                check(s['dim'] >= 1 and 'OIL' in s['lamps'] and 'VOLTS' in s['lamps'] and s['radioOn'] and s['station'] == 'far', 'then the instruments come on by themselves, OIL and VOLTS, and the radio is on, on 1090')
                check(s['frost'] > 0.5, f'frost on the windshield ({s["frost"]:.2f})')
                rg = [r for r in await ev("T.rings()") if math.hypot(r['x'] - s['x'], r['z'] - s['z']) < 1]
                check(rg and rg[0]['r'] > 5, f'a ring of frost round the truck on the asphalt ({rg})')
                ss = await drive(None, lambda s: s['phase'] == 'tail', dt=0.25, max_s=12)
                s = ss[-1]
                want = math.floor((84217 + odo_saro + 31.4) * 10 + 1e-6) / 10
                check(abs(s['odo'] - want) < 0.051, f'the odometer reads {s["odo"]:.1f}: 31.4 miles more than at SARO ({84217 + odo_saro:.1f})')
                tl = await ev("T.toasts.slice(-4)")
                check(any('At the wheel. The truck stands in the southbound lane, facing north.' in t for t in tl) and 'The odometer reads 31.4 miles more than it did at SARO.' in tl, f'the toasts ({tl[-2:]})')
                nn = (await ev("S47.game.notes.slice()"))[n0:]
                check(any(n.endswith('Nothing.') for n in nn) and any('A nosebleed' in n for n in nn) and any('Then three knocks on the roof of the cab, slow, from above, and the light.' in n and 'The turn-off to SARO, on B.' in n for n in nn)
                      and any('three hundred metres further on, in the southbound lane with the nose to the north' in n and 'on 1090' in n for n in nn) and any(n.endswith('The odometer reads 31.4 miles more than it did at SARO.') for n in nn),
                      f'the notes ({len(nn)})')
                img = await ev("(() => { const d = S47.game.docs.find((x) => x.id === 'away1'); return d ? d.image : null; })()")
                if img:
                    import base64
                    open(f'{OUT}/{tag}_04b_exposure.jpg', 'wb').write(base64.b64decode(img.split(',', 1)[1]))
                doc = await ev("(() => { const d = S47.game.docs.find((x) => x.id === 'away1'); return d ? d.transcript : null; })()")
                check(doc and 'the truck standing in the road with every light out' in doc and 'Someone at the wheel. Me.' in doc, 'one exposure too many on the roll: the truck from above')
                sp = await ev("S47.abduction().data.spots.slice(-1)[0]")
                check(sp['cab'] and 'path' not in sp and sp.get('over') == 1, f'kept with the times away: in the truck, no walk kept, the first one over ({ {k: sp[k] for k in sp if k != "doc"} })')
                check(await ev("[...S47.townSeen].some((k) => k.startsWith('tt:taken@')) && S47.townSeen.has('ne:knocks1')"), 'kept: taken (tt:taken@), and the night\'s knocks were these (ne:knocks1)')
                check(await ev("S47.abduction().recordsShown.length") == 0 and await ev("S47.abduction().recordings().length") == 0, 'nobody stands for it, nothing walks it (the truck is the recording)')
                sn = await ev(f"S47.seen.slice({seen0}).map((e) => e.kind + ':' + e.what + ':' + (e.end || ''))")
                check('abduction:truck:taken' in sn and 'abduction:warning:taken' in sn and any(x.startswith('over:first') for x in sn), f'the seen log ({sn})')
                check(await ev("S47.curve.phase") == 'rest' and await ev("S47.curve.landed && S47.curve.landed.what") == 'warning', 'the night\'s curve lands on it, and rests')
                await ev("S47.game.d.ui.clearToasts()"); await tick(0.1)
                await shot('05_odometer')
                box = await ev("T.odoBox()")
                await pg.screenshot(path=f'{OUT}/{tag}_05_odometer_crop.png', clip={'x': max(0, box['x'] - 110), 'y': max(0, box['y'] - 80), 'width': 220, 'height': 130})
                ss = await drive(None, lambda s: s['phase'] == 'idle', dt=0.25, max_s=12)
                await turn()
                ss = await drive(None, lambda s: s['kind'] is None and s['engine'], dt=0.1, max_s=5)
                await tick(0.3); s = await st()
                check(s['engine'] and s['headOn'] and s['turns'] == 1, 'the key: it catches the first time, and the headlights come up')
                await tick(0.5); await shot('06_going_on')
                # saved where it was put down, and Continue
                await ev("S47.world.testInput = { steer: 0, throttle: -1 }"); await tick(0.5); await ev("T.stop()")
                await ev("S47.saveNow('manual', 1)"); await pg.wait_for_timeout(600)
                sv = await ev("""(async () => { const m = S47.saves.list(S47.caseId()).find((x) => x.kind === 'manual' && x.slot === 1); const s = await S47.saves.load(m.id); return { truck: s.truck, pose: s.truckPose, odo: s.odo, cab: s.abduct.spots.slice(-1)[0].cab }; })()""")
                check(sv['truck'] == 'saro' and sv['pose'] and abs(sv['pose']['x'] + 25.8) < 1 and abs(sv['pose']['z'] - s['z']) < 4 and sv['odo'] and sv['odo']['x'] > 30 and sv['cab'],
                      f'a save made in it keeps the truck where it was put down, and the odometer ({sv["pose"] and [round(sv["pose"]["x"], 1), round(sv["pose"]["z"])]}, {sv["odo"]})')
                odo_now = s['odo']
                await pg.reload()
                await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
                await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
                await ev(HELPERS); await ev("T.hook(); S47.hold = true; S47.tick(0.5)"); await quiet()
                tp = await ev("[S47.world.saroTruck.group.position.x, S47.world.saroTruck.group.position.z, S47.world.truckAt]")
                check(abs(tp[0] + 25.8) < 1 and abs(tp[1] - s['z']) < 4 and tp[2] == 'saro', f'Continue: the truck where it was put down ({[round(v, 1) for v in tp[:2]]})')
                pp = await ev("[S47.player.pos.x, S47.player.pos.z]")
                check(math.hypot(pp[0] - tp[0], pp[1] - tp[1]) < 4, f'and the player beside it ({[round(v, 1) for v in pp]})')
                check(await get_in(), 'Continue: in the truck again')
                o2 = await ev("T.L().truck.odometer")
                check(abs(o2 - odo_now) < 0.15, f'the odometer as it was ({o2:.1f})')
                check(await ev("S47.truckTaken.taken") and len(await ev("T.notes('31.4 miles more')")) == 1, 'taken kept, its notes once')
                await peak()
                ss = await south()
                check(all(x['phase'] == 'idle' and x['stage'] == 'calm' for x in ss), 'south over B again at the peak: never again tonight')
                await ev("T.stop()")

            # ===================== the ladder first, then the key, then out and away =====================
            if not phone:
                ctx2, pg2, errs2 = await page(b, 1280, 720, False)
                all_errs.append(('desk2', errs2))
                await session2(pg2, tag)
                await ctx2.close()

        async def session2(pg, tag):
            ev = pg.evaluate
            async def tick(s, n=1):
                for _ in range(n): await ev(f"S47.tick({s}, 20)")
            st = lambda: ev("T.st()")
            async def drive(until, dt=0.05, max_s=40):
                seen = []
                for i in range(int(max_s / dt)):
                    await tick(dt); s = await st(); seen.append(s)
                    if until(s): break
                return seen
            async def peak():
                await ev("S47.curve.free = false; S47.curve.force('peak'); S47.tick(0.05)")
            await ev("S47.hold = true; S47.jump('chapter2'); S47.tick(1.0)")
            await ev("S47.nightEvents['wait'] = 1e9; if (S47.archive) S47.archive.off = true; S47.game.d.ui.close(); S47.setAbductions(true)")
            await ev("S47.game.d.inter.get('truck').use()")
            for _ in range(120):
                await tick(0.25)
                if await ev("S47.world.driving"): break
            check(not await ev("S47.truckB.four") and await ev("S47.truckB.due(S47.truckBIn())"), 'chapter two: four is due on the next pass over B')
            await peak()
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['z'] > 60, max_s=14)
            b4 = await ev("S47.truckB.phase")
            check(any(x['phase'] == 'idle' for x in ss) and all(x['stage'] == 'calm' for x in ss) and await ev("S47.truckTaken.ladderPasses") == 1 and (await ev("S47.truckB.four") or b4 == 'four' or b4 == 'tail'),
                  f'the first pass is the ladder\'s: four, and not the warning ({b4})')
            await drive(lambda s: not s['busy'], dt=0.25, max_s=20)
            await peak()
            n0 = await ev("S47.game.notes.length")
            await ev("S47.game.d.ui.clearToasts()")
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
            check(ss[-1]['phase'] == 'warning' and ss[-1]['tries'] == 1, 'the next pass at the peak: the warning in the cab')
            # the key, quickly, three times
            ss = await drive(lambda s: s['key'], max_s=4)
            for k in range(3):
                await pg.keyboard.press('KeyE')
                ss = await drive(lambda s: s['tstage'] in ('pause', 'catching') or s['kind'] is None, max_s=3)
            s = ss[-1]
            caught_at = s['warn']
            ss = await drive(lambda s: s['stage'] == 'calm', max_s=3)
            s = ss[-1]
            check(s['free'] == 'key' and s['stage'] == 'calm' and s['engine'] and s['turns'] == 3 and (caught_at or 0) < 16, f'E three times, quickly: it catches on the third turn ({caught_at and round(caught_at, 1)} s in) and the warning passes')
            tl = await ev("T.toasts.slice(-6)")
            check('It turns over and will not catch.' in tl and 'It turns over. Nearly.' in tl and 'It catches. The lights come up, and the radio.' in tl, f'the toasts ({tl[-3:]})')
            await tick(0.3); s = await st()
            check(s['held'] == 0 and s['dim'] == 1 and not s['hushed'] and not s['carrying'] and not s['secs'], 'the truck its own again: no hold, the lights, its sound, the radio, the night\'s clock')
            nn = [n for n in (await ev("S47.game.notes.slice()"))[n0:] if 'third turn of the key' in n]
            check(len(nn) == 1 and 'The turn-off to SARO, on B.' in nn[0], f'the note ({nn[:1]})')
            check(await ev("[...S47.townSeen].some((k) => k.startsWith('tt:key@'))"), 'kept (tt:key@)')
            sn = await ev("T.seen('abduction', 'truck')")
            check(sn and sn[-1]['end'] == 'key', f'the seen log: ended with the key ({sn[-1:]})')
            await drive(lambda s: not s['busy'], dt=0.25, max_s=20)
            # out and away, north up the shoulder
            await peak()
            n0 = await ev("S47.game.notes.length")
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
            check(ss[-1]['phase'] == 'warning' and ss[-1]['tries'] == 2, 'a later peak, south over B: the warning again (the second time tonight)')
            ss = await drive(lambda s: abs(s['speed']) < 0.05 and s['key'], max_s=12)
            await ev("S47.game.d.ui.clearToasts()"); await tick(0.2)
            await pg.screenshot(path=f'{OUT}/{tag}_07_get_out_prompt.png')
            await pg.keyboard.press('KeyQ')
            ss = await drive(lambda s: not s['driving'], max_s=3)
            await tick(0.6)
            check(not ss[-1]['driving'] and ss[-1]['stage'] == 'warning', 'Q: out of the truck, the warning still on')
            await ev("S47.player.yaw = 0; S47.input.keys.add('KeyW'); S47.input.keys.add('ShiftLeft')")
            ss = await drive(lambda s: s['stage'] == 'calm' or s['stage'] == 'light', max_s=30)
            await ev("S47.input.keys.clear()")
            s = ss[-1]
            check(s['stage'] == 'calm' and s['free'] == 'ran' and s['ended'] in ('ran', None), f'running north up the shoulder, {math.hypot(s["phx"] - s["x"], s["phz"] - s["z"]):.0f} m from the truck: it passes')
            await drive(lambda s: s['ended'] == 'ran', max_s=1)
            nn = [n for n in (await ev("S47.game.notes.slice()"))[n0:] if 'I got out and ran' in n]
            check(len(nn) == 1 and 'back up the shoulder towards SARO' in nn[0], f'the note ({nn[:1]})')
            check(await ev("[...S47.townSeen].some((k) => k.startsWith('tt:ran@'))") and await ev("S47.world.keyStall"), 'kept; the truck as dead as it was when got back into')
            await pg.screenshot(path=f'{OUT}/{tag}_08_ran.png')
            await drive(lambda s: not s['busy'], dt=0.25, max_s=20)
            await ev("S47.game.d.inter.get('truck').use()")
            for _ in range(120):
                await tick(0.25)
                if await ev("S47.world.driving"): break
            await ev("S47.world.trouble.reset(T.L().truck, T.L().drive)")
            await peak()
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['z'] > 80 or s['phase'] == 'warning', max_s=14)
            check(all(x['phase'] == 'idle' for x in ss) and ss[-1]['tries'] == 2, 'a third time at a peak: no more (twice a night)')
            # Missing time turned off in the middle: a fresh night
            await ev("for (const k of [...S47.townSeen]) if (k.startsWith('tt:')) S47.townSeen.delete(k)")
            await peak()
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
            await drive(lambda s: (s['warn'] or 0) > 8, max_s=10)
            await ev("S47.setAbductions(false)")
            ss = await drive(lambda s: s['stage'] == 'calm' and s['phase'] == 'idle', max_s=3)
            s = ss[-1]
            await tick(0.2); s = await st()
            check(s['stage'] == 'calm' and s['phase'] == 'idle' and s['held'] == 0 and s['dim'] == 1 and not s['hushed'] and not s['carrying'], 'Missing time turned off in the middle: it passes, the truck its own again')
            sn = await ev("T.seen('abduction', 'truck')")
            check(sn and sn[-1]['end'] == 'off', f'the seen log: off ({sn[-1:]})')
            await pg.keyboard.press('KeyE')
            ss = await drive(lambda s: s['engine'], max_s=4)
            check(ss[-1]['engine'], 'and the key catches it')
            # taken while out of the truck but by it
            await ev("S47.setAbductions(true); for (const k of [...S47.townSeen]) if (k.startsWith('tt:')) S47.townSeen.delete(k)")
            await ev("T.stop()")
            await peak()
            n0 = await ev("S47.game.notes.length")
            await ev(f"T.south(-90, 20)")
            ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
            ss = await drive(lambda s: abs(s['speed']) < 0.05 and s['key'], max_s=12)
            await pg.keyboard.press('KeyQ')
            await drive(lambda s: not s['driving'], max_s=3)
            tz = await ev("S47.world.saroTruck.group.position.z")
            ss = await drive(lambda s: s['stage'] == 'back' or s['stage'] == 'calm', dt=0.25, max_s=70)
            s = ss[-1]
            await tick(0.1); s = await st()
            tp = await ev("[S47.world.saroTruck.group.position.x, S47.world.saroTruck.group.position.z, S47.world.saroTruck.group.rotation.y]")
            check(s['stage'] == 'back' and not s['driving'] and abs(tp[1] - (tz + 300)) < 2 and abs(tp[0] + 25.8) < 0.5 and abs(tp[2]) < 0.05,
                  f'out of it but by it at :47: taken all the same, and the truck 300 m on, nose north ({[round(v, 1) for v in tp]})')
            check(math.hypot(s['phx'] - tp[0], s['phz'] - tp[1]) < 5 and s['radioOn'] and s['station'] == 'far', f'on the verge beside it, its radio on 1090 ({round(math.hypot(s["phx"] - tp[0], s["phz"] - tp[1]), 1)} m)')
            await drive(lambda s: s['over'] == 'there' or s['stage'] == 'calm', dt=1 / 30, max_s=5)
            await pg.screenshot(path=f'{OUT}/{tag}_09_verge.png')
            await drive(lambda s: s['stage'] == 'calm', dt=0.25, max_s=20)
            nn = (await ev("S47.game.notes.slice()"))[n0:]
            check(any('I got out, and did not get far enough.' in n for n in nn) and any('On my back on the verge. The truck beside me' in n for n in nn) and any('31.4 miles more' in n for n in nn), 'the notes')

        # ---------- the usual way out of SARO, chapter three: the story's drive to STATION 01 ----------
        ctx, pg, errs = await page(b, 1280, 720, False)
        all_errs.append(('desk3', errs))
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter3'); S47.tick(1.0)")
        await ev("S47.nightEvents['wait'] = 1e9; if (S47.archive) S47.archive.off = true; S47.game.d.ui.close(); S47.setAbductions(true); S47.truckB.off = true")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(120):
            await ev("S47.tick(0.25, 20)")
            if await ev("S47.world.driving"): break
        trip = await ev("S47.world.tripNow")
        await ev("S47.curve.free = false; S47.curve.force('peak'); S47.tick(0.05, 20); S47.world.autopilot = 11")
        first = None
        for _ in range(900):
            await ev("S47.tick(0.05, 20)")
            s = await ev("T.st()")
            if s['phase'] == 'warning': first = s; break
        await ev("S47.world.autopilot = null")
        check(trip and trip['to'] == 'station01' and first and -30 < first['x'] < -18 and 10 < first['z'] < 40,
              f'chapter three, the story\'s drive out of SARO to STATION 01 at a peak: it comes as the truck turns south out of the service road onto the highway ({first and [round(first["x"], 1), round(first["z"], 1), round(first["speed"], 1)]})')
        await ctx.close()

        # ---------- the usual way in chapter four (the critic, 21 October): the story's drive to the motel
        # with N. Vega's CB call due nine seconds out, four had in chapter three; E as the engine dies;
        # the key; the CB after it; the second time the key never catches, and out is the way ----------
        ctx, pg, errs = await page(b, 1280, 720, False)
        all_errs.append(('desk4', errs))
        ev = pg.evaluate
        async def tick4(s): await ev(f"S47.tick({s}, 20)")
        async def drive4(until, dt=0.05, max_s=40):
            seen = []
            for i in range(int(max_s / dt)):
                await tick4(dt); s = await ev("T.st()"); seen.append(s)
                if until(s): break
            return seen
        await ev("S47.hold = true; S47.jump('chapter4'); S47.tick(1.0)")
        await ev("S47.nightEvents['wait'] = 1e9; if (S47.archive) S47.archive.off = true; S47.game.d.ui.close(); S47.setAbductions(true); S47.townSeen.add('truckB:4'); S47.officePhone.reset()")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(120):
            await tick4(0.25)
            if await ev("S47.world.driving"): break
        trip = await ev("S47.world.tripNow")
        await ev("S47.curve.free = false; S47.curve.force('peak'); S47.tick(0.05, 20); S47.world.autopilot = 13")
        ss, cbFirst = [], False
        for _ in range(600):
            await tick4(0.05)
            s = await ev("T.st()"); s['cb'] = await ev("S47.cb.busy"); ss.append(s)
            if s['cb'] and s['phase'] == 'idle': cbFirst = True
            if s['phase'] == 'warning' or s['z'] > 120: break
        await ev("S47.world.autopilot = null")
        w = ss[-1]
        check(trip and trip['to'] == 'motel' and w['phase'] == 'warning' and not cbFirst and -30 < w['x'] < -18 and 10 < w['z'] < 40 and not await ev("S47.ch4.s.cb"),
              f'chapter four, the drive to the motel at a peak: it comes at the turn-off, N. Vega\'s CB call waiting for it ({[round(w["x"], 1), round(w["z"], 1)]}, CB first: {cbFirst})')
        # E as the engine dies, stopped but before the key is wanted (the plain Get out used to be Use's
        # then): still at the wheel; Q Get out on the screen
        ss = await drive4(lambda s: abs(s['speed']) < 0.5 or s['tstage'] != 'dying', dt=1 / 30, max_s=3)
        s0 = ss[-1]
        await pg.keyboard.press('KeyE'); await tick4(0.05)
        s = await ev("T.st()")
        check(s0['tstage'] == 'dying' and s['driving'] and 'QGet out' in s['prompt'] and 'Turn the key' not in s['prompt'],
              f'E as the engine dies, the truck stopped: still at the wheel (Use is the key here, not the door), Q Get out offered ({s0["tstage"]}, {s0["speed"]:.2f} m/s, "{s["prompt"]}")')
        ss = await drive4(lambda s: s['key'], max_s=4)
        for k in range(3):
            await pg.keyboard.press('KeyE')
            ss = await drive4(lambda s: s['tstage'] in ('pause', 'catching') or s['kind'] is None, max_s=3)
            if ss[-1]['tstage'] == 'pause': ss = await drive4(lambda s: s['key'], max_s=2)
        ss = await drive4(lambda s: s['stage'] == 'calm', max_s=3)
        check(ss[-1]['free'] == 'key', f'three turns, quickly: it catches ({ss[-1]["free"]})')
        ss = await drive4(lambda s: s['phase'] == 'idle', dt=0.25, max_s=14)
        await ev("S47.world.autopilot = 13")
        cbAt = None
        for _ in range(300):
            await tick4(0.1)
            if cbAt is None and await ev("S47.cb.busy"): cbAt = await ev("T.st()")
            if cbAt is not None and any('SARO seven' in t for t in await ev("T.toasts.slice(-4)")): break
        check(cbAt is not None and cbAt['phase'] == 'idle' and any('SARO seven' in t for t in await ev("T.toasts.slice(-4)")), f'then N. Vega on the CB, after it ({cbAt and round(cbAt["z"])})')
        # (to the end of her call: its note; the truck left standing where it is, not driven on to the lot)
        await ev("S47.world.autopilot = null; S47.world.testInput = { steer: 0, throttle: -1 }"); await tick4(1.5); await ev("T.stop()")
        # (the call runs on the audio clock, in real time)
        for _ in range(240):
            await tick4(0.1); await pg.wait_for_timeout(400)
            if await ev("S47.game.notes.some((n) => n.includes(\"on the truck's CB\"))") and not await ev("S47.cb.busy"): break
        await ev("S47.world.autopilot = null")
        # the second warning in the truck tonight: the key never catches; out and away
        await ev("S47.curve.force('peak'); S47.tick(0.05); S47.game.d.ui.clearToasts(); T.toasts.length = 0")
        await ev("T.south(-90, 20)")
        ss = await drive4(lambda s: s['phase'] == 'warning', max_s=14)
        check(ss[-1]['phase'] == 'warning' and ss[-1]['tries'] == 2 and await ev("S47.truckTaken.keyProof"), 'the second time tonight, after the key: the warning, and this time the key will not do')
        caught = False
        for _ in range(int(18 / 0.05)):
            await tick4(0.05); s = await ev("T.st()")
            if s['key'] and (s['warn'] or 0) < 15: await pg.keyboard.press('KeyE')
            if s['free'] == 'key' or s['tstage'] == 'catching': caught = True
            if (s['warn'] or 0) > 17 or s['stage'] != 'warning': break
        tl = await ev("T.toasts.slice()")
        check(not caught and s['turns'] >= 3 and 'It turns over. Nearly.' in tl and 'The third turn. It does not catch this time.' in tl, f'E every time it is wanted: {s["turns"]} turns, it never catches ({tl[-3:]})')
        await pg.keyboard.press('KeyQ')
        await drive4(lambda s: not s['driving'], max_s=3); await tick4(0.4)
        await ev("S47.player.yaw = 0; S47.input.keys.add('KeyW'); S47.input.keys.add('ShiftLeft')")
        ss = await drive4(lambda s: s['stage'] in ('calm', 'light'), max_s=30)
        await ev("S47.input.keys.clear()")
        await drive4(lambda s: s['ended'] == 'ran', max_s=1)
        check(ss[-1]['free'] == 'ran' and 'The crickets, all at once. Behind you the truck stands in the road with its lights out.' in await ev("T.toasts.slice(-3)"), 'Q and away up the shoulder: it passes, and says so')
        await ctx.close()

        # ---------- desktop ----------
        ctx, pg, errs = await page(b, 1280, 720, False)
        all_errs.append(('desk', errs))
        await session(pg, 'desk')
        await ctx.close()

        # ---------- phone ----------
        ctx, pg, errs = await page(b, 412, 892, True)
        all_errs.append(('phone', errs))
        ev = pg.evaluate
        tag = 'phone'
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s}, 20)")
        st = lambda: ev("T.st()")
        async def drive(until, dt=0.05, max_s=40):
            seen = []
            for i in range(int(max_s / dt)):
                await tick(dt); s = await st(); seen.append(s)
                if until(s): break
            return seen
        async def tap(sel):
            loc = pg.locator(sel); bb = await loc.bounding_box()
            await pg.touchscreen.tap(bb['x'] + bb['width'] / 2, bb['y'] + bb['height'] / 2)
        await ev("S47.hold = true; S47.jump('chapter4'); S47.tick(1.0)")
        await ev("S47.nightEvents['wait'] = 1e9; if (S47.archive) S47.archive.off = true; S47.game.d.ui.close(); S47.setAbductions(true); S47.truckB.off = true; S47.officePhone.reset(); S47.ch4.s.cb = true")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(120):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        clk = await ev("S47.game.clock")
        check(await ev("S47.world.driving") and await ev("S47.game.phase") == 'ch4', f'phone: chapter four, in the truck at SARO ({int(clk // 3600):02d}:{int(clk % 3600 // 60):02d})')
        await ev("S47.curve.free = false; S47.curve.force('peak'); S47.tick(0.05)")
        await ev("T.south(-90, 20)")
        ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
        check(ss[-1]['phase'] == 'warning', 'phone: south over B at the peak: the warning in the cab')
        ss = await drive(lambda s: abs(s['speed']) < 0.05 and s['key'], max_s=12)
        await ev("S47.game.d.ui.clearToasts()"); await tick(0.3)
        s = await st()
        ub, ab2 = await pg.locator('.touch .use').bounding_box(), await pg.locator('.touch .alt').bounding_box()
        apart = ub and ab2 and (ab2['y'] + ab2['height'] <= ub['y'] or ab2['x'] + ab2['width'] <= ub['x'])
        check(s['prompt'] == 'Turn the key' and s['alt'] == 'Get out' and await pg.locator('.touch .use').is_visible() and await pg.locator('.touch .alt').is_visible() and apart,
              f'phone: the Use button says "Turn the key", and the Get out button is there with it, apart ({s["prompt"]}, {s["alt"]})')
        await pg.screenshot(path=f'{OUT}/phone_10_warning.png')
        # the Get out button: out, and away up the shoulder on the stick
        await tap('.touch .alt')
        ss = await drive(lambda s: not s['driving'], max_s=3)
        await tick(0.6)
        check(not ss[-1]['driving'] and ss[-1]['stage'] == 'warning', 'phone: the Get out button: out of the truck, the warning still on')
        await ev("S47.player.yaw = 0; S47.input['stickY'] = -1")
        ss = await drive(lambda s: s['stage'] in ('calm', 'light'), max_s=30)
        await ev("S47.input['stickY'] = 0")
        check(ss[-1]['stage'] == 'calm' and ss[-1]['free'] == 'ran', f'phone: walking away up the shoulder, it passes ({ss[-1]["warn"]})')
        await drive(lambda s: not s['busy'], dt=0.25, max_s=20)
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(120):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev("S47.world.trouble.reset(T.L().truck, T.L().drive); S47.curve.force('peak'); S47.tick(0.05)")
        await ev("T.south(-90, 20)")
        ss = await drive(lambda s: s['phase'] == 'warning', max_s=14)
        check(ss[-1]['phase'] == 'warning' and ss[-1]['tries'] == 2, 'phone: the next peak, south over B: the warning again')
        ss = await drive(lambda s: abs(s['speed']) < 0.05 and s['key'], max_s=12)
        await tap('.touch .use')
        ss = await drive(lambda s: s['tstage'] == 'pause', max_s=3)
        check(ss[-1]['turns'] == 1 and not ss[-1]['engine'], 'phone: the Use button turns the key: it does not catch')
        # let it come
        ss = await drive(lambda s: s['stage'] in ('light', 'taken'), dt=0.25, max_s=50)
        ss = await drive(lambda s: s['over'] == 'there', dt=1 / 30, max_s=30)
        s = ss[-1]
        fig = await ev("T.overAt()")
        check(s['over'] == 'there' and fig and -2.65 < fig[0]['z'] < -0.9, f'phone: woken at the wheel, one of them on the bonnet ({fig})')
        heads = await ev("T.heads()")
        check(heads and 0 < heads[0]['x'] < 412 and 0 < heads[0]['y'] < 892 * 0.5, f'phone: its head in the upper part of the portrait view ({heads[0] if heads else None})')
        await pg.screenshot(path=f'{OUT}/phone_11_bonnet.png')
        check(await ev("getComputedStyle(document.querySelector('.touch')).visibility") == 'hidden', 'phone: no buttons while it is there')
        ss = await drive(lambda s: s['phase'] == 'tail', dt=0.25, max_s=14)
        await ev("S47.game.d.ui.clearToasts()"); await tick(0.1)
        s = await st()
        check(s['odo'] >= 84217 + 31.4 - 0.05 and s['radioOn'] and s['station'] == 'far', f'phone: the odometer {s["odo"]:.1f}, the radio on 1090')
        await pg.screenshot(path=f'{OUT}/phone_12_odometer.png')
        await ctx.close()

        bad = [(t, e) for t, es in all_errs for e in es if 'GPU stall' not in e and 'favicon' not in e]
        check(not bad, f'no console errors or warnings ({bad[:4]})')
        await b.close()
    fails = [c for c in checks if c[0] == 'FAIL']
    print(f'\n{len(checks) - len(fails)}/{len(checks)} PASS')
    with open(f'{OUT}/result.json', 'w') as f: json.dump(checks, f, indent=1)
    sys.exit(1 if fails else 0)

asyncio.run(main())
