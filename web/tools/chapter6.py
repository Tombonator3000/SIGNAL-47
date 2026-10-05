# Chapter six, "Roswell Road" and THE EVENT, played through in a headless browser: the
# warning about flashing images on the title screen, the cut onto the old road at 05:26:20,
# the drive past the six-mile post (the bolt there), the invisible end behind the start,
# the clock that will not show 05:29 before the line, the line itself just past the
# eight-mile post, the 47 seconds (carrier, the engine, the dash blinking four and seven,
# the headlights in colours, the radio with the night in pieces, FLASH, silence), the
# pull-out (out of the cab, up, the sun on the mesa at 05:30, the stars), the title, the
# credits and the whole 1947 photograph, then the title screen. A second run takes the end
# again with slow fades and still shots. Starts from S47.jump('chapter6').
# Usage: python3 tools/chapter6.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
T = 5 * 3600 + 29 * 60
MILE = 1609.34
s_at = lambda mi: (mi - 8.15) * MILE
# the right lane, at about 24 m/s; brake to hold when speed is None
AUTO = """((v) => { const w = S47.world, road = w.oldRoad, drv = w.oldDrive;
  w.testInput = () => { const s = road.where(drv.pos.x, drv.pos.z).s; const tg = road.lane(s + 18);
    let err = Math.atan2(-(tg.x - drv.pos.x), -(tg.z - drv.pos.z)) - drv.heading; err = Math.atan2(Math.sin(err), Math.cos(err));
    const want = v === null ? 0 : v;
    return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: v === null ? (drv.speed > 0.05 ? -1 : 0) : Math.max(-1, Math.min(1, (want - drv.speed) * 0.5)) }; }; })"""
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def where(): return await ev("(() => { const r = S47.world.oldRoad, d = S47.world.oldDrive, w = r.where(d.pos.x, d.pos.z); return { s: w.s, mi: w.mi, o: w.o, speed: d.speed, clock: S47.game.clock, stage: S47.ch6.stage, pastC: r.pastC(d.pos.x, d.pos.z) }; })()")
        async def place(mi, back=False):
            await ev(f"""(() => {{ const r = S47.world.oldRoad, d = S47.world.oldDrive, s = {s_at(mi)};
              const a = r.lane(s), b2 = r.lane(s + 4); const h = Math.atan2(-(b2.x - a.x), -(b2.z - a.z)) + ({'Math.PI' if back else '0'});
              d.place(a, h); }})()""")
            await tick(0.1)
        async def toasts(): return await ev("document.querySelector('.toasts').textContent")
        priv = lambda o, k: f"S47.world.oldTruck['{k}']" if o == 'truck' else f"S47.ch6['{k}']"

        # ---------- the warning on the title screen ----------
        note = await ev("(document.querySelector('.flash-note') || {}).textContent || ''")
        check('flashing images' in note and await ev("document.querySelector('[data-a=fl-keep]').getAttribute('aria-pressed')") == 'true', 'the title screen warns about flashing images; flashes is the default')
        await pg.click('[data-a=fl-calm]')
        check(await ev("localStorage.getItem('s47.calmFlash')") == 'true' and await ev("document.querySelector('[data-a=fl-calm]').getAttribute('aria-pressed')") == 'true', 'slow fades can be picked there and is kept')
        await pg.click('[data-a=fl-keep]')
        await shot('k00_title_warning')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter6'); S47.tick(0.5)")

        # ---------- the cut onto the old road ----------
        for _ in range(40):
            await tick(0.25)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        st = await where()
        check(await ev("S47.game.phase") == 'ch6' and st['stage'] == 'drive' and abs(st['mi'] - 5.9) < 0.01 and abs(st['o'] - 1.6) < 0.05, f"the old road: the right lane at {st['mi']:.2f} miles")
        check(abs(st['clock'] - (5 * 3600 + 26 * 60 + 20)) < 3, f"the clock: {st['clock'] / 3600:.4f} h, about 05:26:20")
        check('MILE 8' in await ev("document.querySelector('.objective').textContent") and await ev("S47.game.saveBlock()") == 'Not on the road.', 'objective: the line past mile 8; no saving on the road')
        await tick(0.1)   # the truck is put on the road after a promise; the sky follows on the next frame
        check(await ev("S47.sky.uniforms.uDawn.value") > 0.8 and await ev("S47.camera.far") >= 10000, 'dawn in the sky, and the far plane opened for the old road')
        art = await ev("S47.art().loaded")
        check(all(x in art for x in ('milepostBlank', 'witnessPost', 'surveyDisk', 'oldAsphalt')), 'the road came with its pictures from round 11 (mile post, witness post, brass disk, asphalt)')
        await ev("S47.camera.rotation.set(0,0,0)"); await tick(0.2)
        calls = {'start': await ev("S47.renderer.info.render.calls")}
        await shot('k01_start')
        # past the six-mile post and its bolt
        await ev(AUTO + "(24)")
        for _ in range(60):
            await tick(0.5)
            st = await where()
            if st['mi'] > 6.02: break
        check(st['mi'] > 6.02 and abs(st['o'] - 1.6) < 0.6, f"driven past the six-mile post in the lane ({st['mi']:.3f} mi, {st['o']:.2f} m)")
        calls['mile6'] = await ev("S47.renderer.info.render.calls")
        await shot('k02_past_mile6')
        # the invisible end behind the start
        await place(5.78, back=True)
        await ev("S47.world.testInput = { steer: 0, throttle: 0.7 }")
        for _ in range(48): await tick(0.25)
        st = await where()
        check(st['s'] > s_at(5.9) - 252 and 'Ward can wait' in await toasts(), f"turned back, the truck stops at the end behind the start ({st['s'] - s_at(5.9):.0f} m) and Ward can wait")

        # ---------- towards the line: the clock holds at 05:28 ----------
        await place(8.02)
        await ev(f"S47.game.clock = {T - 25}")
        await ev(AUTO + "(14)")
        for _ in range(80):
            await tick(0.25)
            if (await where())['mi'] > 8.085: break
        await ev(AUTO + "(null)")
        await shot('k03_before_the_line')
        for _ in range(12): await tick(4)
        st = await where()
        check(st['stage'] == 'drive' and st['clock'] < T and st['pastC'] < 0, f"stopped short of the line for 48 s: the clock still says {int(st['clock'] // 3600):02d}:{int(st['clock'] % 3600 // 60):02d}:{st['clock'] % 60:05.2f}")
        check(await ev("document.querySelector('.clockline').textContent") == '05:28', 'and the clock on the screen says 05:28')
        check('line is just ahead' in await toasts(), 'a nudge: the line is just ahead')

        # ---------- THE EVENT ----------
        await ev(AUTO + "(16)")
        for _ in range(900):
            await tick(1 / 30)
            if (await where())['stage'] == 'event': break
        st = await where()
        check(st['stage'] == 'event' and abs(st['clock'] - T) < 0.1 and abs(st['mi'] - 8.15) < 0.006, f"on the line just past the eight-mile post ({st['mi']:.4f} mi): 05:29:00")
        await tick(1.0)
        check(await ev("S47.world.oldDrive.engine") is False and await ev(priv('truck', 'dashLevel')) == 0, '05:29:00: the engine dies and the instruments go dark')
        await shot('k04_on_the_line')
        await tick(3.2)                                   # + 4.2
        blinks = []
        for _ in range(42):
            blinks.append(await ev(priv('truck', 'dashLevel'))); await tick(1 / 6)
        # count the blinks in the two groups (a long gap between them)
        on_idx = [i for i, v in enumerate(blinks) if v == 1]
        groups, g = [], 1
        for a, c in zip(on_idx, on_idx[1:]):
            if c - a > 1:
                if c - a > 5: groups.append(g); g = 1
                else: g += 1
        groups.append(g)
        n_on = len([1 for i, v in enumerate(blinks) if v == 1 and (i == 0 or blinks[i - 1] != 1)])
        check(n_on == 11 and groups[:2] == [4, 7], f'+4 s: the dash blinks four times, a pause, seven times ({groups})')
        st = await where()
        check(abs(st['speed']) < 0.05 and abs(st['o'] - 1.6) < 0.8, f'the truck has rolled out and stands in its lane ({st["speed"]:.2f} m/s)')
        await tick(11.4 - (await ev("S47.game.clock") - T))   # + 11.4
        hc = await ev(f"(c => [c.r, c.g, c.b])({priv('truck', 'headColor')})")
        check(hc[2] > hc[0] and hc[2] > hc[1], f'+10 s: the headlights wake deep blue {[round(v, 2) for v in hc]}')
        await shot('k05_blue')
        await tick(4.0)                                   # + 15.4, green (about 2.2 s per colour)
        hc = await ev(f"(c => [c.r, c.g, c.b])({priv('truck', 'headColor')})")
        check(hc[1] > hc[0] and hc[1] > hc[2], f'then amber, then green {[round(v, 2) for v in hc]}')
        await tick(8.8)                                   # + 24.2
        check('Reference west. No. East.' in await toasts(), '+24 s: on the radio, Tomás')
        await tick(3.0)                                   # + 27.2
        check('Do not stop on the line.' in await toasts(), '+27 s: Nora')
        hc = await ev(f"(c => [c.r, c.g, c.b])({priv('truck', 'headColor')})")
        check(hc[0] > hc[1] and hc[0] > hc[2], f'and the headlights red {[round(v, 2) for v in hc]}')
        await shot('k06_red_radio')
        await tick(32.03 - (await ev("S47.game.clock") - T))
        fr = await ev("(() => { const f = document.querySelector('.ending-frame'); return { o: +getComputedStyle(f).opacity, img: !!f.querySelector('canvas') }; })()")
        check(await ev(f"{priv('ch6', 'done')}.has('printer') && {priv('ch6', 'done')}.has('phone') && {priv('ch6', 'done')}.has('cup')"), 'the radio played the printer, the phone and the cup')
        check(fr['o'] == 1 and fr['img'], '+32 s: FLASH, a frame over the screen')
        art = await ev("S47.art().loaded")
        check(all(x in art for x in ('flashEye', 'flashHand', 'flashFace', 'flashMan')), 'the frames of FLASH from round 11 were loaded for it')
        await shot('k07_flash')
        seen = set()
        for _ in range(40):
            await tick(1 / 30)
            seen.add(await ev("(() => { const c = document.querySelector('.ending-frame canvas'); return c && getComputedStyle(document.querySelector('.ending-frame')).opacity === '1' ? [...document.querySelectorAll('.ending-frame canvas')].length + ':' + c.width + ':' + (c.dataset.k || (c.dataset.k = Math.random().toString(36).slice(2))) : 'black'; })()"))
        check(len(seen) >= 7, f'single frames one after another, with black between ({len(seen) - 1} frames seen)')
        await tick(35.2 - (await ev("S47.game.clock") - T))
        check(await ev(f"{priv('ch6', 'done')}.has('silence')") and await ev("getComputedStyle(document.querySelector('.ending-frame')).opacity") == '0', '+34 s: no more frames, total silence')
        await shot('k08_silence')

        # ---------- the pull-out ----------
        await tick(47.3 - (await ev("S47.game.clock") - T))
        check(await ev("S47.ch6.stage") == 'pullout', '05:29:47: the camera leaves')
        eye = await ev("(() => { const t = S47.world.oldTruck; t.group.updateMatrixWorld(); return t.driverEye.clone().applyMatrix4(t.group.matrixWorld).toArray(); })()")
        await tick(5.0)
        cam = await ev("S47.camera.position.toArray()")
        check(math.dist(cam, eye) > 5 and await ev("S47.world.oldTruck.shell.visible"), 'out through the windshield: the truck from outside, alone on the road')
        await shot('k09_truck_alone')
        await tick(6.5)
        h = await ev("S47.camera.position.y - S47.world.oldRoad.height(S47.camera.position.x, S47.camera.position.z)")
        check(h > 100, f'higher: {h:.0f} m over the desert')
        await shot('k10_high')
        await tick(5.5)
        c = await ev("S47.game.clock")
        check(c >= 5 * 3600 + 30 * 60 and await ev("S47.sky.uniforms.uSun.value") == 1 and await ev("S47.ch6.sunElev") is not None, f'05:30: the sun on the edge of the mesa (clock {c - 5 * 3600 - 30 * 60:+.1f} s from 05:30)')
        await shot('k11_sun')
        await tick(15.5)
        check(await ev("S47.ch6.skyLift") < 0.35, 'up to the stars: the dawn falls away')
        await shot('k12_stars')
        calls['pullout'] = await ev("S47.renderer.info.render.calls")

        # ---------- the title, the credits, the photograph, the title screen ----------
        await tick(3.0)
        await pg.wait_for_selector('.ending-title.on', timeout=20000)
        check('SIGNAL' in await ev("document.querySelector('.ending-title').textContent"), 'SIGNAL / 47')
        await shot('k13_title')
        await pg.wait_for_selector('.ending-credits.on', timeout=20000)
        text = await ev("document.querySelector('.ending-credits').textContent")
        check(all(x in text for x in ('Tom', 'Claude', 'Codex', 'Scott Buckley', 'three.js', 'The comet was real')), 'the credits: who made it, the music, the sounds, the fonts, three.js')
        await pg.wait_for_timeout(1500)
        await shot('k14_credits')
        await pg.wait_for_selector('.ending-credits .skip:not([hidden])', timeout=10000)
        await pg.click('.ending-credits .skip')
        await pg.wait_for_selector('.ending-photo.on', timeout=20000)
        check(await ev("!!document.querySelector('.ending-photo canvas')"), 'after the credits: the whole photograph from 1947, no text')
        await pg.wait_for_timeout(6500)
        await shot('k15_photo')
        await pg.wait_for_selector('button[data-a=start]', timeout=40000)
        check(await ev("!document.querySelector('.ending')") and await ev("S47.world.area") == 'saro', 'and back to the title screen')

        # ---------- again, with slow fades and still shots ----------
        await ev("localStorage.setItem('s47.calmFlash', 'true'); localStorage.setItem('s47.stillShots', 'true')")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        check(await ev("document.querySelector('[data-a=fl-calm]').getAttribute('aria-pressed')") == 'true', 'the title screen shows slow fades picked')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter6'); S47.tick(0.5)")
        for _ in range(40):
            await tick(0.25)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        await place(8.12)
        await ev(AUTO + "(10)")
        for _ in range(600):
            await tick(1 / 15)
            if (await where())['stage'] == 'event': break
        await tick(32.5)
        ops = []
        for _ in range(16):
            await tick(0.5)
            ops.append(float(await ev("getComputedStyle(document.querySelector('.ending-frame')).opacity")))
        check(max(ops) <= 0.36 and max(ops) > 0.1 and len([o for o in ops if o > 0.05]) >= 8, f'slow fades: dim frames over several seconds, no flashes (max {max(ops):.2f})')
        await tick(47.2 - (await ev("S47.game.clock") - T))
        await tick(2.0)
        a = await ev("S47.camera.position.toArray()")
        await tick(3.0)
        b2 = await ev("S47.camera.position.toArray()")
        await tick(6.0)
        c2 = await ev("S47.camera.position.toArray()")
        check(math.dist(a, b2) < 0.01 and math.dist(b2, c2) > 50, 'still shots: the camera holds each shot, then cuts')
        await shot('k16_still')

        print('draw calls:', calls)
        check(max(calls.values()) <= 90, f'draw calls at most {max(calls.values())} on the old road')
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-chapter6.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
