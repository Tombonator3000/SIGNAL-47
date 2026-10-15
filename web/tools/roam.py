# The truck any time (Tom, 15 October): from chapter one on, the truck where it stands offers a
# drive of the player's own, and it is left on any lot it is stopped on. In the prologue it is
# not offered. From SARO's pad to the diner's lot, on to the gravel outside STATION 01's gate
# and back to the pad, with the autopilot steering (it is given each place in turn; the trips
# themselves have none). Before chapter three the walk gap at the station is chained, and
# before chapter five sends the truck out on it the old road has ROAD CLOSED across it.
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
            for _ in range(80):
                await tick(0.25)
                if await ev("S47.world.driving"): break
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

        phase0 = await ev("S47.game.phase")
        await drive('saro_diner', 'truck', 'diner')
        check(await ev("S47.game.phase") == phase0 and not await ev("S47.ch5.active"), f"saro_diner: the story is where it was ({phase0}); chapter five has not begun")
        check(await ev("S47.world['road']['closed'].group.visible"), 'saro_diner: ROAD CLOSED stands across the old road before chapter five')
        check(await label('dinerTruck') == 'Drive', f"saro_diner: the truck on the diner's lot offers a drive ({await label('dinerTruck')})")

        await drive('diner_station', 'dinerTruck', 'station01')
        check(await ev("S47.world.site.zone.walkThrough.enabled === false && S47.scene.getObjectByName('walkChain').visible"), 'diner_station: the walk gap at the station is chained before chapter three')
        check(await label('s1walkGate') == 'Walk gate', f"diner_station: the chained gap says so ({await label('s1walkGate')})")
        check(await label('s1truck') == 'Drive', f"diner_station: the truck by the gate offers a drive ({await label('s1truck')})")

        await drive('station_saro', 's1truck', 'saro')
        check(await ev("S47.world.saroTruck.group.visible"), "station_saro: SARO's truck is on its pad")
        check(await label('truck') == 'Drive', f"station_saro: and offers a drive again ({await label('truck')})")

        # ---------- chapter three opens the walk gap; the night's own drive comes first ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0)")
        check(await label('truck') == 'Drive to STATION 01', f"chapter three: the truck offers the chapter's drive ({await label('truck')})")
        await tick(0.2)
        check(await ev("S47.world.site.zone.walkThrough.enabled === true && !S47.scene.getObjectByName('walkChain').visible"), 'chapter three: the walk gap at the station is open')

        bad = [e for e in errs if 'favicon' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
