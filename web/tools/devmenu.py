# The developer menu (?dev): the DEV button on the title screen and in the game, a chapter
# started from it (and never saved), the truck taken out on the highway and on the old road
# from wherever the player stands and brought back, the run to the line on the old road,
# F2, and ?dev=0 taking the menu away again.
# Usage: python3 tools/devmenu.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, sys, os
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
T = 5 * 3600 + 29 * 60
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
        await pg.goto(URL + '?dev')
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def menu(label):
            if not await ev("!!document.querySelector('.devmenu')"): await pg.click('.devbtn')
            await pg.click(f'.devmenu button.dev:text-is("{label}")')
        async def labels(): return await ev("[...document.querySelectorAll('.devmenu button.dev')].map(b => b.textContent)")

        # ---------- the title screen ----------
        check(await ev("!!document.querySelector('.devbtn')"), 'with ?dev the title screen has the DEV button')
        await pg.click('.devbtn')
        ls = await labels()
        check('1 The Second Exposure' in ls and '6 At the line' in ls and 'Drive the highway' not in ls, f'the menu offers the chapters ({len(ls)} buttons), not the truck before a night runs')
        await shot('d01_title_menu')

        # ---------- a chapter, never saved ----------
        await pg.click('.devmenu button.dev:text-is("3 The Survey Station")')
        await pg.wait_for_function("S47.game.phase === 'ch3' && S47.ch3.s.stage === 'to-truck' && !document.querySelector('.fade.on')")
        await ev("S47.hold = true")
        await tick(3)
        n = await ev("S47.saves.list().length")
        active = await ev("localStorage.getItem('s47.activeCase')")
        check(n == 0 and active is None, f'chapter 3 started from the menu, and nothing was saved ({n} saves)')
        await shot('d02_ch3')

        # ---------- the truck from wherever the player is ----------
        before = await ev("({ x: S47.player.pos.x, z: S47.player.pos.z, area: S47.world.area })")
        await pg.keyboard.press('F2')
        check(await ev("!!document.querySelector('.devmenu')"), 'F2 opens the menu in the game')
        ls = await labels()
        check('Drive the highway' in ls and 'Drive the old road' in ls and '+5 minutes' in ls, 'in the game it offers the truck and the clock')
        await pg.click('.devmenu button.dev:text-is("Drive the highway")')
        await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'road' && S47.world.driving && S47.world.free === 'road'")
        await ev("S47.hold = true")
        await ev("S47.world.testInput = { steer: 0, throttle: 0.8 }")
        await tick(4)
        sp = await ev("S47.world['drive'].speed")
        check(sp > 4, f'free drive on the highway: the truck goes ({sp:.1f} m/s)')
        await ev("S47.world.testInput = null")
        await shot('d03_highway')
        await menu('Back on foot')
        await tick(0.5)
        after = await ev("({ x: S47.player.pos.x, z: S47.player.pos.z, area: S47.world.area, driving: S47.world.driving })")
        check(after['area'] == before['area'] and not after['driving'] and abs(after['x'] - before['x']) < 0.01 and abs(after['z'] - before['z']) < 0.01,
              f"back on foot where the player stood ({after['area']})")
        check(await ev("S47.ch3.s.stage") == 'to-truck', 'the chapter has not moved on')

        await menu('Drive the old road')
        await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'roswell' && S47.world.driving && S47.world.free === 'oldroad'")
        await ev("S47.hold = true")
        await tick(1)
        await shot('d04_oldroad_night')
        await menu('Back on foot')
        await tick(0.5)
        check(await ev("S47.world.area") == 'saro' and not await ev("S47.world.driving"), 'and back from the old road')
        c0 = await ev("S47.game.clock")
        await menu('+30 minutes')
        c1 = await ev("S47.game.clock")
        check(abs(c1 - c0 - 1800) < 1, 'the clock goes on half an hour')

        # ---------- straight to the line ----------
        await menu('6 At the line')
        await ev("S47.hold = false")
        await pg.wait_for_function("S47.game.phase === 'ch6' && S47.world.area === 'roswell' && S47.world.driving && Math.abs(S47.world.oldRoad.where(S47.world.oldDrive.pos.x, S47.world.oldDrive.pos.z).mi - 8.06) < 0.003")
        await ev("S47.hold = true")
        await ev("S47.world.testInput = { steer: 0, throttle: 0.5 }")
        for _ in range(40):
            await tick(1)
            if await ev("S47.ch6.stage") == 'event': break
        st = await ev("({ stage: S47.ch6.stage, clock: S47.game.clock, et: S47.ch6['et'] })")
        check(st['stage'] == 'event' and abs(st['clock'] - st['et'] - T) < 0.2, f"6 at the line: a few seconds to C, then THE EVENT at 05:29:00 ({st['stage']})")
        await ev("S47.world.testInput = null")
        check(await ev("S47.saves.list().length") == 0, 'still nothing saved')

        # ---------- ?dev=0 ----------
        await pg.goto(URL + '?dev=0')
        await pg.wait_for_selector('button[data-a=start]')
        check(not await ev("!!document.querySelector('.devbtn')"), '?dev=0 takes the menu away')
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        check(not await ev("!!document.querySelector('.devbtn')"), 'and it stays away')

        bad = [e for e in errs if 'GPU stall' not in e and 'Automatic fallback to software WebGL' not in e]
        print('\n'.join(bad) if bad else 'no console errors/warnings')
        await b.close()
    n = sum(1 for c in checks if c[0] == 'PASS')
    print(f'{n} of {len(checks)} PASS')
    sys.exit(0 if n == len(checks) and not bad else 1)

asyncio.run(main())
