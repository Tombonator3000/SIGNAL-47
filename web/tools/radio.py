# The radio (core/radio.ts, Tom 16 October): the truck's radio works (R, the dial on the dash,
# three stations, the Night Line's captions, static swelling over it now and then, off again),
# the town's station signs off at 04:00 with the national anthem and the carrier, Roswell signs
# on at 05:00 with the anthem, the radio drops out while the starter turns, it plays on from the truck when the player gets out, the
# diner's radio plays the station that is on the air, and on the old road the cab radio is
# chapter six's. With S47_URL on the Pages build the anthem file itself is checked as it plays;
# the single-file build has no file beside it, so there the carrier stands in.
# Usage: python3 tools/radio.py OUTDIR [WxH]
import asyncio, sys, os
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
PAGES = URL.startswith('http')   # served like Pages, with the files beside the page
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

ST = "S47.radio.state()"
CAR = "(() => { const r = S47.carRadio; return { on: r.on, station: r.station, live: r.live, power: r.power }; })()"
DIAL = "(() => { const w = S47.world, t = w.truckHere(); return t ? t['radioShown'] : null; })()"

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs, bad404 = [], []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        pg.on('response', lambda r: bad404.append(r.url) if r.status >= 400 else None)
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def clock(): return await ev("S47.game.clock")
        async def heard(): return await ev("window.__toasts.join(' | ')")

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0)")
        await ev("window.__toasts = []; const u = S47.game.d.ui, o = u.toast.bind(u); u.toast = (t, s) => { window.__toasts.push(t); return o(t, s); };")
        st = await ev(ST)
        check(st['caliche']['seg'] != 'dead' and st['roswell']['seg'] == 'dead' and st['far']['seg'] != 'dead',
              f"at {await ev('S47.game.clock') / 3600:.2f} h: 1420 on the air, Roswell off until five, the far one there ({st['caliche']['seg']}, {st['roswell']['seg']}, {st['far']['seg']})")
        check(not (await ev(CAR))['on'] and not any(s['awake'] for s in st.values()), 'the radio is off, and no station makes a sound')

        # ---------- into the truck, and the radio on ----------
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        check('R: radio' in await heard() or 'Radio: the radio' in await heard(), 'the hint on getting in says R turns the radio on')
        await pg.keyboard.press('r'); await tick(0.5)
        c = await ev(CAR); st = await ev(ST)
        check(c['on'] and c['station'] == 'caliche' and c['live'] and st['caliche']['awake'] and st['caliche']['listeners'] >= 1,
              f"R: on, AM 1420, the station plays ({c}, listeners {st['caliche']['listeners']})")
        check('AM 1420' in await heard(), 'the dial is said: AM 1420')
        dial = await ev(DIAL)
        check(dial and dial.endswith(':true') and abs(int(dial.split(':')[0]) - 293) <= 1, f"the dash: the dial lit, the needle at 1420 ({dial})")
        # a little way out of the yard, then stopped in it (the rest is heard standing still)
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 13")
        await tick(3.0); await pg.screenshot(path=f'{OUT}/radio_on.png')
        await ev("S47.world.autopilot = null; S47.world.testInput = { steer: 0, throttle: -1 }")
        for _ in range(40):
            await tick(0.25)
            if await ev("Math.abs(S47.world.legs[S47.world.leg].drive.speed) < 0.3"): break
        await ev("S47.world.testInput = { steer: 0, throttle: 0 }")
        check(await ev("S47.world.driving && S47.world.area === 'saro'"), 'stopped in the yard, still in the cab')
        # looking at the radio in the cab: the prompt says what it does, and E (or a click) does it
        found = None
        for yi in range(0, 12):
            for pi in range(0, 9):
                y, p_ = -yi * 0.08, -0.1 - pi * 0.08
                await ev(f"(() => {{ const D = S47.world.legs[S47.world.leg].drive; D.yaw = {y}; D.pitch = {p_}; }})()"); await tick(0.05)
                if await ev("S47.radioAimed()"): found = (y, p_); break
            if found: break
        prompt = await ev("(() => { const e = document.querySelector('.hud .prompt'); return e && e.classList.contains('show') ? e.textContent : ''; })()")
        check(found and 'AM 1420: next station' in prompt, f"looking at the radio in the cab, it says what it does ({found}, '{prompt}')")
        await pg.screenshot(path=f'{OUT}/radio_aim.png')
        await pg.keyboard.press('e'); await tick(0.3)
        c = await ev(CAR)
        check(c['station'] == 'roswell' and await ev("S47.world.driving"), f"E on the radio turns it to the next station, and does not get out ({c['station']})")
        await ev("S47.carRadio.set(true, 'caliche'); (() => { const D = S47.world.legs[S47.world.leg].drive; D.yaw = 0; D.pitch = -0.08; })()"); await tick(0.6)
        # the Night Line: a host and callers, captioned while the radio can be heard
        got = False
        for _ in range(16):
            await tick(10, 20)
            if 'RADIO' in await heard(): got = True; break
        h = await heard()
        check(got, f"the Night Line is captioned ({[x for x in h.split(' | ') if 'RADIO' in x][:2]})")
        # now and then the static swells over the station (Tom: static at random, with the wah wah)
        mx, back = 0.0, False
        for _ in range(170):
            await tick(0.5, 20)
            k = await ev("S47.carRadio.interference")
            mx = max(mx, k)
            if mx > 0.2 and k == 0: back = True; break
        check(0.2 < mx <= 0.7 and back, f"now and then the static comes up over the station, and goes again (deepest {mx:.2f}; not too often or too deep since 16 October)")

        # ---------- along the dial and off ----------
        await pg.keyboard.press('r'); await tick(0.6)
        st = await ev(ST); c = await ev(CAR)
        check(c['station'] == 'roswell' and st['roswell']['strength'] == 0 and not st['caliche']['awake'],
              'R again: 1230, Roswell, off the air before five (static), and 1420 stops making sound')
        await pg.keyboard.press('r'); await tick(0.6)
        st = await ev(ST); c = await ev(CAR)
        check(c['station'] == 'far' and st['far']['awake'] and 0 < st['far']['strength'] < 1, f"R again: 1090, the far one, coming and going ({st['far']['strength']:.2f})")
        await pg.keyboard.press('r'); await tick(0.6)
        st = await ev(ST); c = await ev(CAR)
        check(not c['on'] and not any(s['awake'] for s in st.values()) and (await ev(DIAL)).endswith(':false'), 'R after the last: off, the dial dark, no station making sound')

        # ---------- the sign-off at four ----------
        await pg.keyboard.press('r'); await tick(0.3)
        await ev("S47.game.clock = 4 * 3600 - 4"); await tick(5)
        st = await ev(ST)
        check(st['caliche']['seg'] == 'talk', f"04:00: the Night Line signs off ({st['caliche']['seg']})")
        for _ in range(20):
            await tick(1)
            if (await ev(ST))['caliche']['seg'] == 'anthem': break
        check('signing off' in await heard(), 'its words are captioned: "signing off until six"')
        st = await ev(ST)
        check(st['caliche']['seg'] == 'anthem', f"then the national anthem ({st['caliche']['seg']})")
        if PAGES:
            await ev("S47.hold = true")
            await pg.wait_for_timeout(4000)
            st = await ev(ST)
            a = st['caliche']['anthem']
            check(a and not a['paused'] and a['time'] > 1.5 and not st['caliche']['anthemFailed'], f"the anthem file plays on the radio ({a})")
        else:
            check(st['caliche']['anthemFailed'], 'single-file build: no anthem file, the carrier stands in')
        await pg.screenshot(path=f'{OUT}/radio_anthem.png')
        await tick(80, 20)
        st = await ev(ST)
        check(st['caliche']['seg'] == 'carrier' and st['caliche']['anthem'] is None, f"after the anthem, the bare carrier ({st['caliche']['seg']})")
        await tick(80, 20)
        st = await ev(ST)
        check(st['caliche']['seg'] == 'dead' and st['caliche']['strength'] == 0, f"then nothing: 1420 is off the air ({st['caliche']['seg']})")

        # ---------- Roswell signs on at five ----------
        await pg.keyboard.press('r'); await tick(0.6)   # to 1230
        c = await ev(CAR)
        await ev("S47.game.clock = 5 * 3600 - 3"); await tick(4)
        st = await ev(ST)
        check(c['station'] == 'roswell' and st['roswell']['seg'] == 'carrier', f"05:00: Roswell's carrier comes up ({st['roswell']['seg']})")
        for _ in range(20):
            await tick(1)
            if (await ev(ST))['roswell']['seg'] == 'anthem': break
        st = await ev(ST)
        check(st['roswell']['seg'] == 'anthem' and 'beginning our broadcast day' in await heard(), 'it signs on, "beginning our broadcast day", and plays the anthem')
        if PAGES:
            await ev("S47.hold = true"); await pg.wait_for_timeout(3000)
            a = (await ev(ST))['roswell']['anthem']
            check(a and not a['paused'] and a['time'] > 1, f"the anthem file plays again ({a})")
        await tick(80, 20)
        st = await ev(ST)
        check(st['roswell']['seg'] in ('talk', 'music') and st['roswell']['strength'] > 0.5, f"then the morning programme ({st['roswell']['seg']})")

        # ---------- trouble: the radio drops out while the starter turns ----------
        await ev("S47.world.trouble.start('stall')")
        low = 1.0
        for _ in range(40):
            await tick(0.2)
            low = min(low, (await ev(CAR))['power'])
        await tick(4)
        check(low < 0.1 and (await ev(CAR))['power'] == 1, f"the engine dies and the starter turns: the radio drops out ({low:.2f}), and comes back")

        # ---------- out of the truck: it plays on from the dash ----------
        await ev("S47.world.testInput = null")
        out = False
        for _ in range(30):
            if await ev("S47.world.canGetOut()"): out = await ev("S47.world.getOutHere()"); break
            await ev("S47.world.testInput = { steer: 0, throttle: 0.6 }"); await tick(1.0); await ev("S47.world.testInput = { steer: 0, throttle: -1 }"); await tick(1.5); await ev("S47.world.testInput = null")
        await tick(0.5)
        c = await ev(CAR)
        check(out and not await ev("S47.world.driving") and c['on'] and c['live'], f"out of the truck at {await ev('S47.world.area')}: the radio plays on from the dash ({c})")
        check(await ev("document.querySelector('.tbtn.radio').hidden"), 'on foot the Radio button is gone')

        # ---------- the diner's radio ----------
        await ev("window.__go = S47.world.goDiner('inside')")
        for _ in range(60):
            await tick(0.25)
            if await ev("S47.world.area === 'diner'"): break
        await tick(1)
        d = await ev("(() => { const r = S47.dinerRadio; return { on: r.on, station: r.station, live: r.live }; })()")
        check(d['on'] and d['live'] and d['station'] == 'roswell', f"in the diner after five, the radio behind the counter plays Roswell ({d})")
        await ev("S47.game.clock = 3 * 3600 + 50 * 60"); await tick(1)
        d = await ev("(() => { const r = S47.dinerRadio; return { on: r.on, station: r.station, live: r.live }; })()")
        check(d['on'] and d['station'] == 'caliche', f"and before four the town's own station ({d})")

        # ---------- the old road: chapter six has the cab radio ----------
        await ev("S47.jump('chapter6')")
        for _ in range(120):
            await tick(0.25)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        await ev("window.__toasts = []")
        await pg.keyboard.press('r'); await tick(0.5)
        check(not (await ev(CAR))['live'] and 'stays on the Roswell station' in await heard(), 'on the old road the radio is chapter six\'s: R leaves it alone')

        # the radios have their own level in the pause menu (Tom's improvements, 16 October)
        await ev("S47.hold = false")
        await pg.keyboard.press('Escape'); await pg.wait_for_timeout(400)
        has = await ev("!!document.querySelector('#rad')")
        if has:
            await ev("(() => { const r = document.querySelector('#rad'); r.value = '0.25'; r.dispatchEvent(new Event('input')); })()")
        lvl = await ev("[S47.audio.radioLevel, S47.audio.radioBus ? S47.audio.radioBus.gain.value : null, localStorage.getItem('s47.radioVol') || Object.keys(localStorage).filter((k) => k.includes('radioVol')).map((k) => localStorage.getItem(k))[0]]")
        check(has and abs(lvl[0] - 0.25) < 1e-6 and lvl[1] is not None and abs(lvl[1] - 0.25) < 1e-6 and lvl[2] is not None, f'the pause menu has a radio level, kept on this device ({lvl})')
        await ev("(() => { const r = document.querySelector('#rad'); if (r) { r.value = '1'; r.dispatchEvent(new Event('input')); } })()")
        await pg.click('[data-a=resume]'); await pg.wait_for_timeout(200)
        await ev("S47.hold = true")

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        if PAGES: check(not [u for u in bad404 if 'favicon' not in u], f'no missing files ({bad404[:3]})')
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
