# The truck any time (Tom, 15 October): from chapter one on, the truck where it stands offers a
# drive of the player's own, and it is left on any lot it is stopped on. In the prologue it is
# not offered. From SARO's pad to the diner's lot, on to the gravel outside STATION 01's gate
# and back to the pad, with the autopilot steering (it is given each place in turn; the trips
# themselves have none). Before chapter three the walk gap at the station is chained, and
# before chapter five sends the truck out on it the old road has ROAD CLOSED across it. The
# flashlight comes off the passenger seat on getting in (F), and the zoom (right mouse) reads
# the barricades from the diner's lot.
# Usage: python3 tools/roam.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, sys, os
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

STATE = """(() => { const w = S47.world, L = w.leg && w.legs[w.leg], D = L && L.drive;
  return { area: w.area, leg: w.leg, driving: w.driving, at: w.truckAt, mph: D ? D.mph : 0, clock: S47.game.clock }; })()"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        ctx = await b.new_context(viewport={'width': W, 'height': H})
        pg = await ctx.new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def label(i): return await ev(f"(() => {{ const it = S47.game.d.inter.get('{i}'); return it ? it.label() : 'MISSING'; }})()")
        async def use(i): await ev(f"S47.game.d.inter.get('{i}').use()")
        state = lambda: ev(STATE)

        await ev("S47.hold = true; S47.tick(0.3)")
        await ev("S47.setPicture('off')")
        # ---------- the prologue: not yet ----------
        await ev("S47.jump('residual'); S47.tick(1.0)")
        check(await label('truck') == 'Service truck', f"prologue: the truck is not offered for a drive ({await label('truck')})")
        await use('truck'); await tick(0.5)
        check(not await ev("S47.world.driving"), 'prologue: using it does not start a drive')

        # ---------- chapter one: any time ----------
        await ev("S47.jump('chapter1'); S47.tick(1.0)")
        check(await label('truck') == 'Drive', f"chapter one: the truck on its pad offers a drive ({await label('truck')})")

        async def drive(what, use_id, place, budget=600):
            """Get in with use_id, steer with the autopilot to `place`, and wait until parked."""
            await use(use_id)
            # the trip loads what it needs first (the station's area the first time): real time too
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): break
                await pg.wait_for_timeout(100)
            st = await state()
            check(st['driving'] and st['at'] is None, f"{what}: in the truck, the engine running ({st['leg']})")
            # standing still where it was: not parked again at once
            await tick(2.0)
            check(await ev("S47.world.driving"), f'{what}: standing still on the lot it left, the truck is not left there again')
            await ev(f"S47.world['trip'].to = '{place}'; S47.world.autopilot = 14")
            n = 0
            for _ in range(int(budget / 0.5)):
                if not await ev("S47.world.driving"): break
                await tick(0.5); n += 1
                if n % 40 == 0: await shot(f'{what}_{n:03d}')
            await tick(1.0)
            st = await state()
            check(st['area'] == place and not st['driving'] and st['at'] == place, f"{what}: left on the lot at {place}, out of the truck ({st['area']}, {st['at']})")
            await shot(f'{what}_parked')
            return st

        # ---------- getting out (Tom, 16 October: no way out once in) ----------
        async def get_in(i):
            await use(i)
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): break
                await pg.wait_for_timeout(100)
            await tick(0.5)
        async def press_out():
            await pg.keyboard.press('e')
            for _ in range(24):
                await tick(0.25)
                if not await ev("S47.world.driving"): break
            await tick(0.5)
        prompt = lambda: ev("(() => { const e = document.querySelector('.hud .prompt'); return e ? e.textContent : ''; })()")
        check(not await ev("S47.flashlight.have"), 'chapter one: no flashlight before the truck')
        await get_in('truck')
        check(await ev("S47.flashlight.have"), 'get out: the flashlight came off the passenger seat on getting in')
        pr = await prompt()
        check(await ev("S47.world.canGetOut()") and 'Get out' in pr, f"get out: in the truck on its pad, Get out is offered ({pr})")
        await press_out()
        st = await state()
        check(not st['driving'] and st['at'] == 'saro' and st['area'] == 'saro', f"get out: E gets out on the pad ({st['area']}, {st['at']})")
        pad = await ev("(() => { const g = S47.world.saroTruck.group.position; return [g.x, g.z]; })()")
        # drive off the pad a little way, stop, and get out where it stands
        await get_in('truck')
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 5")
        await tick(5.0)
        await ev("S47.world.autopilot = null")
        for _ in range(60):
            await tick(0.25)
            if abs(await ev("(() => { const w = S47.world, L = w.legs[w.leg]; return L ? L.drive.speed : 0; })()")) < 0.3: break
        await tick(0.5)
        can = await ev("S47.world.canGetOut()")
        await shot('getout_yard')
        await press_out()
        st = await state()
        at = await ev("(() => { const g = S47.world.saroTruck.group.position; return [g.x, g.z]; })()")
        moved = ((at[0] - pad[0]) ** 2 + (at[1] - pad[1]) ** 2) ** 0.5
        check(can and not st['driving'] and st['at'] == 'saro' and moved > 6, f"get out: stopped off the pad, E gets out and the truck stays there ({moved:.1f} m from the pad)")
        near = await ev("(() => { const p = S47.player.pos, g = S47.world.saroTruck.group.position; return Math.hypot(p.x - g.x, p.z - g.z); })()")
        check(near < 3.6 and await label('truck') == 'Drive', f"get out: the player stands by the door ({near:.2f} m) and the truck offers a drive again ({await label('truck')})")
        await shot('getout_out')

        phase0 = await ev("S47.game.phase")
        await drive('saro_diner', 'truck', 'diner')
        await pg.keyboard.press('f'); await tick(0.3)
        on = await ev("S47.flashlight.on && S47.flashlight.light.intensity > 1")
        await pg.keyboard.press('f'); await tick(0.3)
        check(on and not await ev("S47.flashlight.on"), 'saro_diner: F switches the flashlight on and off')
        # a closer look from the lot (right mouse): the barricades across the old road
        bp = await ev("(() => { const v = S47.world['road'].closedSign.getWorldPosition(S47.camera.position.clone()); return [v.x, v.z]; })()")
        await ev(f"S47.player.place({bp[0] - 9}, {bp[1]}, -Math.PI / 2); S47.player.pitch = 0; S47.flashlight.on = true; S47.input.zoomHeld = true")
        await tick(0.8)
        ex = await ev("(() => { const e = S47.game.d.inter.examine(S47.camera); return e ? e.it.id + ': ' + e.text : null; })()")
        hud = await ev("(() => { const e = document.querySelector('.hud .examine'); return e && e.classList.contains('show') ? e.textContent : null; })()")
        await shot('diner_barricades')
        check(ex and ex.startswith('roadClosed: ROAD CLOSED') and hud and hud.startswith('ROAD CLOSED'), f"saro_diner: zoomed in from the lot, the barricades read ROAD CLOSED ({ex})")
        check(await ev("S47.camera.fov < 40"), f"saro_diner: the zoom narrows the view ({await ev('S47.camera.fov')})")
        await ev("S47.input.zoomHeld = false; S47.flashlight.on = false"); await tick(0.8)
        check(await ev("S47.camera.fov > 60"), 'saro_diner: and lets go again')
        check(await ev("S47.game.phase") == phase0 and not await ev("S47.ch5.active"), f"saro_diner: the story is where it was ({phase0}); chapter five has not begun")
        check(await ev("S47.world['road']['closed'].group.visible"), 'saro_diner: ROAD CLOSED stands across the old road before chapter five')
        check(await label('dinerTruck') == 'Drive', f"saro_diner: the truck on the diner's lot offers a drive ({await label('dinerTruck')})")

        await drive('diner_station', 'dinerTruck', 'station01')
        check(await ev("S47.world.site.zone.walkThrough.enabled === false && S47.scene.getObjectByName('walkChain').visible"), 'diner_station: the walk gap at the station is chained before chapter three')
        check(await label('s1walkGate') == 'Walk gate', f"diner_station: the chained gap says so ({await label('s1walkGate')})")
        check(await label('s1truck') == 'Drive', f"diner_station: the truck by the gate offers a drive ({await label('s1truck')})")
        check((await ev("S47.game.d.inter.get('s1walkGate').examine()") or '').startswith('A chain'), 'diner_station: a closer look at the gap: chain and padlock')

        await drive('station_saro', 's1truck', 'saro')
        check(await ev("S47.world.saroTruck.group.visible"), "station_saro: SARO's truck is on its pad")
        check(await label('truck') == 'Drive', f"station_saro: and offers a drive again ({await label('truck')})")

        # ---------- chapter three opens the walk gap; the night's own drive comes first ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0)")
        check(await label('truck') == 'Drive to STATION 01', f"chapter three: the truck offers the chapter's drive ({await label('truck')})")
        await tick(0.2)
        check(await ev("S47.world.site.zone.walkThrough.enabled === true && !S47.scene.getObjectByName('walkChain').visible"), 'chapter three: the walk gap at the station is open')
        check(await label('s1walkGate') is None and await ev("!S47.world.site.proxies.walkGate.layers.test(S47.camera.layers)"), 'chapter three: and its hit box is out of the way')
        check(await ev("S47.world['road'].roadClosed"), 'chapter three: the old road is still closed')

        # ---------- chapter five sends the truck out on the old road: the barricades go ----------
        await ev("S47.jump('chapter5'); S47.tick(1.0)")
        check(await label('truck') == 'Drive to Mesa Diner', f"chapter five: the truck offers the diner ({await label('truck')})")
        check(await ev("S47.world['road'].roadClosed"), 'chapter five: closed while the map is still to be read')
        await ev("S47.ch5.s.stage = 'road'"); await tick(0.3)
        check(await ev("!S47.world['road'].roadClosed && !S47.world['road'].closedSign.visible && S47.game.d.inter.get('roadClosed').examine() === null"), 'chapter five, C found: the barricades are gone from the old road')
        check(await label('truck') == 'Drive to Mesa Diner', f"chapter five, C found: from SARO the truck still goes by the diner ({await label('truck')})")

        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
