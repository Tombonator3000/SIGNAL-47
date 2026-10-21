# Trouble with the truck (drive/trouble.ts, Tom 16 October): on a drive with the autopilot,
# a misfire, a flicker, the dash going dark, the engine dying and starting again by itself, the
# engine dying and waiting for the player's key, and random trouble on and off. Checks the
# engine, the warning lamps, the light levels and the needles, and that all is given back.
# Usage: python3 tools/trouble.py OUTDIR [WxH]
import asyncio, sys, os
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

ST = """(() => { const w = S47.world, L = w.legs[w.leg], D = L.drive, T = L.truck;
  const lamps = Object.entries(T['lamps']).filter(([k, m]) => m.visible).map(([k]) => k);
  return { kind: w.trouble.kind, engine: D.engine, power: D.power, mph: D.mph, level: T['level'], dash: T['dashLevel'],
    lamps, fuel: T['gauge'].fuel, key: w.trouble.wantsKey, driving: w.driving,
    prompt: (() => { const e = document.querySelector('.hud .prompt'); return e && e.classList.contains('show') ? e.textContent : ''; })() }; })()"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def run(secs, every=0.2):
            seen, t = [], 0.0
            while t < secs - 1e-6:
                await tick(every); t += every
                seen.append(await ev(ST))
            return seen

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0)")
        check(await ev("S47.world.trouble.random === false"), 'random trouble is off until the story turns it on')
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 13")
        for _ in range(40):
            await tick(1)
            if (await ev(ST))['mph'] > 20: break
        st = await ev(ST)
        check(st['driving'] and st['mph'] > 20 and st['kind'] is None, f"on the road at {st['mph']:.0f} mph, nothing wrong")

        # ---------- a misfire ----------
        await ev("S47.world.trouble.start('sputter', { len: 2 })")
        seen = await run(3)
        check(min(s['power'] for s in seen) < 0.5, f"misfire: the pull drops out (lowest {min(s['power'] for s in seen):.2f})")
        st = seen[-1]
        check(st['kind'] is None and st['power'] == 1 and st['engine'], 'misfire: over, the engine pulls as before')

        # ---------- a flicker ----------
        await ev("S47.world.trouble.start('flicker', { len: 1.6 })")
        seen = await run(2.6, 0.1)
        lv = [s['level'] for s in seen if s['kind'] == 'flicker']
        check(lv and min(lv) < 0.3 and max(lv) > 0.7, f"flicker: the lights stutter ({min(lv or [1]):.2f} to {max(lv or [0]):.2f})")
        await tick(0.4); st = await ev(ST)
        check(st['level'] == 1 and st['dash'] is None and not st['lamps'], 'flicker: over, the lights are the truck\'s own again')

        # ---------- the dash goes dark ----------
        await ev("S47.world.trouble.start('dashOut', { len: 3 })")
        seen = await run(3)
        dark = [s for s in seen if s['kind'] == 'dashOut' and s['dash'] == 0]
        check(dark and min(s['fuel'] for s in seen) < 0.2, f"dash out: the instruments dark and the fuel needle falls ({min(s['fuel'] for s in seen):.2f})")
        await pg.screenshot(path=f'{OUT}/dash_out.png')
        await tick(3); st = await ev(ST)
        check(st['kind'] is None and st['fuel'] > 0.45, f"dash out: over, the needle back ({st['fuel']:.2f})")

        # ---------- the engine dies and the driver starts it again ----------
        await ev("S47.world.trouble.start('stall')")
        seen = await run(1.6)
        st = seen[-1]
        check(not st['engine'] and 'OIL' in st['lamps'] and 'VOLTS' in st['lamps'] and st['level'] < 0.8,
              f"stall: the engine dead, OIL and VOLTS lit, the lights on the battery ({st['level']:.2f}, {st['lamps']})")
        await pg.screenshot(path=f'{OUT}/stall_lamps.png')
        seen += await run(8)
        check(min(s['level'] for s in seen) < 0.45, 'stall: the lights dip as the starter turns it over')
        st = seen[-1]
        check(st['engine'] and st['kind'] is None and not st['lamps'] and st['level'] == 1, f"stall: it caught and runs, the lamps out ({st['kind']}, {st['lamps']})")

        # ---------- dead until the player turns the key ----------
        await ev("S47.world.trouble.start('stall', { key: true })")
        await run(4)
        st = await ev(ST)
        check(st['key'] and not st['engine'] and 'Turn the key' in st['prompt'], f"the key: dead and waiting, the prompt says so ({st['prompt']})")
        await run(3)
        check(not (await ev(ST))['engine'], 'the key: it stays dead while nobody turns it')
        tries = 0
        for _ in range(3):
            if not await ev("S47.world.trouble.wantsKey"): break
            await pg.keyboard.press('e'); tries += 1
            await run(2.6)
        st = await ev(ST)
        check(st['engine'] and st['kind'] is None, f"the key: E starts it ({tries} turns)")

        # ---------- held dead by the story, then let go ----------
        await ev("S47.world.trouble.start('stall', { hold: true })")
        await run(6)
        check(not (await ev(ST))['engine'], 'held: the engine stays dead for the story')
        await ev("S47.world.trouble.release()")
        await run(7)
        check((await ev(ST))['engine'], 'held: let go, the driver starts it')

        # ---------- random trouble ----------
        n0 = await ev("S47.world.trouble.count")
        await run(20, 0.5)
        check(await ev("S47.world.trouble.count") == n0, 'random off: nothing happens by itself')
        await ev("S47.world.trouble.random = true; S47.world.trouble.every = 4")
        await run(20, 0.5)
        n1 = await ev("S47.world.trouble.count")
        check(n1 > n0, f'random on: trouble comes by itself ({n1 - n0} in 20 s at a short interval)')
        await ev("S47.world.trouble.random = false")
        await run(10, 0.5)

        # ---------- trouble on the way from one area to the next: the copy left behind is itself ----------
        await ev("S47.world.trouble.start('stall', { hold: true })")
        await run(1.6)
        left = await ev("(() => { const w = S47.world; return Object.entries(w.legs).filter(([id]) => id !== w.leg).map(([id, L]) => ({ id, engine: L.drive.engine, level: L.truck['level'], lamps: Object.values(L.truck['lamps']).some((m) => m.visible) })); })()")
        check(all(l['engine'] and l['level'] == 1 and not l['lamps'] for l in left), f"handed over: the trucks left in other areas are untouched ({left})")
        await ev("S47.world.trouble.release()"); await run(8)
        # ---------- out of the truck ----------
        await ev("S47.world.stopDriving(); S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.5)")
        st = await ev("(() => ({ kind: S47.world.trouble.kind, lamps: [S47.world.saroTruck, S47.world['truck']].some((t) => t && Object.values(t['lamps']).some((m) => m.visible)) }))()")
        check(st['kind'] is None and not st['lamps'], 'out of the truck: all given back')
        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
