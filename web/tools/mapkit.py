# The notebook's map and kit (Tom, 16 October): M opens the pencil map with a cross where the
# player is, I opens what the player carries, and People has the people in the case with their
# faces once the portraits are in (core/portraits.ts). Checks the tabs, that the cross moves with the
# player (SARO, the yard, the motel, the highway, the diner, STATION 01), that STATION 01 and
# the old road come onto the map as the night goes on, and that the kit follows the flashlight
# and the camera.
# Usage: python3 tools/mapkit.py OUTDIR [WxH]
import asyncio, sys, os, base64
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

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
        TAB = "(() => { const t = document.querySelector('.notebook [aria-selected=true]'); return t ? t.dataset.t : null; })()"
        async def open_key(k):
            await pg.keyboard.press(k); await tick(0.2)
        async def close():
            await ev("document.querySelector('.notebook .close')?.click()"); await tick(0.2)
        async def where():
            return await ev("(() => { const v = S47.game.notebookExtras && S47.mapAt(); return v; })()")
        async def save_map(name):
            src = await ev("document.querySelector('.nb-map img')?.src || ''")
            if src.startswith('data:image/png;base64,'):
                open(f'{OUT}/{name}.png', 'wb').write(base64.b64decode(src.split(',', 1)[1]))
            return src

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.0)")
        # M: the map, I: the kit
        await open_key('m')
        check(await ev(TAB) == 'map' and await ev("!!document.querySelector('.nb-map img')"), 'M opens the notebook at the map')
        src = await save_map('map_saro')
        check(len(src) > 20000, f'the map is drawn ({len(src) // 1024} kB)')
        a = await where()
        check(a and a['spot'] in ('saro', 'yard'), f"the cross is at SARO ({a})")
        await close()
        await open_key('i')
        kit = await ev("[...document.querySelectorAll('.nb-kit li b')].map((b) => b.textContent)")
        check(await ev(TAB) == 'kit' and 'Notebook' in kit and 'Truck keys' in kit and 'Flashlight' not in kit, f'I opens the kit: no flashlight yet ({kit})')
        await close()
        # People: the people in the case so far, with an empty frame until there is a portrait
        await open_key('i')
        await ev("document.querySelector('.notebook [data-t=people]')?.click()"); await tick(0.1)
        ppl = await ev("[...document.querySelectorAll('.nb-people li b')].map((b) => b.textContent)")
        faces = await ev("[document.querySelectorAll('.nb-people li img').length, document.querySelectorAll('.nb-people li .noface').length, S47.portraitCount()]")
        check(await ev(TAB) == 'people' and ppl == ['Dale'] and faces[0] + faces[1] == len(ppl) and (faces[2] > 0 or faces[0] == 0),
              f'People: Dale in chapter one, a face or an empty frame each ({ppl}, faces {faces})')
        await close()
        # the motel down the highway
        await ev("(() => { const z = S47.world.motel.zones.find((z) => z.id === 'office'); S47.player.place((z.minX + z.maxX) / 2, (z.minZ + z.maxZ) / 2, 0); })()"); await tick(0.3)
        a = await where()
        check(a and a['spot'] == 'motel', f'in the motel office the cross is at the motel ({a})')
        # in the truck: the flashlight comes off the seat; on the highway the cross is on the highway
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev("S47.world['trip'].to = 'diner'; S47.world.autopilot = 13")
        for _ in range(120):
            await tick(1)
            if await ev("S47.world.area === 'road' && S47.world.truckHere().group.position.z > 600"): break
        a = await where()
        check(a and a['spot'] == 'highway' and 815 < a['m'] < 2300, f'on the highway the cross is on the highway, past the track and short of the diner ({a})')
        await open_key('m')
        await save_map('map_highway')
        await close()
        for _ in range(200):
            await tick(1)
            if not await ev("S47.world.driving"): break
        a = await where()
        check(await ev("S47.world.area") == 'diner' and a and a['spot'] == 'diner', f'at the diner the cross is at the diner ({a})')
        await open_key('i')
        kit = await ev("[...document.querySelectorAll('.nb-kit li')].map((l) => l.textContent)")
        check(any(k.startswith('Flashlight') for k in kit) and any('Mesa Diner' in k for k in kit), f'the kit: the flashlight now, and the truck on the diner lot ({kit})')
        await close()
        # later in the night: STATION 01 and the old road on the map
        check(not await ev("S47.game.at('ch3')"), 'STATION 01 is not on the map in chapter one')
        await ev("S47.jump('chapter5')")
        for _ in range(60):
            await tick(0.5)
            if await ev("S47.game.at('ch5')"): break
        await open_key('m')
        await save_map('map_ch5')
        check(await ev(TAB) == 'map', 'chapter five: the map, with STATION 01 and the old road on it')
        await close()
        await open_key('i')
        await ev("document.querySelector('.notebook [data-t=people]')?.click()"); await tick(0.1)
        ppl = await ev("[...document.querySelectorAll('.nb-people li b')].map((b) => b.textContent)")
        await pg.screenshot(path=f'{OUT}/people_ch5.png')
        check('Dr. Evelyn Ward' in ppl and 'N. Vega' in ppl, f'People in chapter five: Ward and N. Vega too ({ppl})')
        await close()
        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
