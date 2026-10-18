# The truck's radio (Tom, 18 October): the songs stream from public/radio/, each time from a
# different place in the song; in the cab at 0.38; out of the truck still on, but faint and
# muffled through the closed cab. Needs the Pages build, served with byte ranges (as GitHub
# Pages does; python's http.server does not, and then a song cannot be started part way in).
# Usage: S47_URL=http://127.0.0.1:8047/SIGNAL-47/ python3 tools/radiomix.py
import asyncio, os, sys
from playwright.async_api import async_playwright
URL = os.environ.get('S47_URL', 'http://127.0.0.1:8047/SIGNAL-47/')
res = []
def check(ok, w): res.append(ok); print(('PASS ' if ok else 'FAIL ') + w, flush=True)
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': 1280, 'height': 720})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(m.text) if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append(str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL); await pg.wait_for_function('window.S47')
        await pg.evaluate("localStorage.clear()"); await pg.reload(); await pg.wait_for_function('window.S47')
        await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        await ev("S47.jump('chapter1')"); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(1.0); S47.game.clock = 1 * 3600 + 30 * 60")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(240):
            await ev("S47.tick(0.25)")
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        await ev("S47.carRadio.set(true, 'caliche')")
        st = lambda: ev("S47.radio.stations.caliche")
        # run the station to a music segment, in real time so the element plays
        seen = []
        for _ in range(400):
            await ev("S47.tick(0.5)"); await pg.wait_for_timeout(60)
            info = await ev("(() => { const s = S47.radio.stations.caliche, e = s.trackEl; return [s.seg.kind, s.seg.tune || null, e ? e.currentTime : null, e ? e.src.split('/').pop() : null, s.trackFailed, s['segNo']]; })()")
            if info[0] == 'music' and info[2] is not None and info[2] > 0:
                if not seen or seen[-1][5] != info[5]: seen.append(info)
                if len(seen) >= 2: break
        print(seen)
        check(len(seen) >= 1 and not seen[0][4], f'a song streams from public/radio ({seen[:1]})')
        if len(seen) >= 2: check(True, 'and the next music segment too')
        t1 = await ev("(() => { const e = S47.radio.stations.caliche.trackEl; return e ? e.currentTime : -1; })()")
        await pg.wait_for_timeout(1500)
        t2 = await ev("(() => { const e = S47.radio.stations.caliche.trackEl; return e ? e.currentTime : -1; })()")
        check(t2 > t1 >= 0, f'the song plays on ({t1:.1f} to {t2:.1f})')
        check(len(seen) >= 2 and seen[0][2] > 3 and seen[1][2] > 3, f'each time from somewhere inside the song, not the start ({[round(x[2], 1) for x in seen]})')
        gin = await ev("(() => { S47.tick(0.5); const n = S47.carRadio['n']; return [n.out.gain.value, n.cab.frequency.value]; })()")
        check(abs(gin[0] - 0.38) < 0.05 and gin[1] > 15000, f'in the cab: volume {gin[0]:.2f}, not muffled ({gin[1]:.0f} Hz)')
        await pg.keyboard.press('e')
        for _ in range(24):
            await ev("S47.tick(0.25)")
            if not await ev("S47.world.driving"): break
        await ev("S47.tick(1.0)"); await pg.wait_for_timeout(1500); await ev("S47.tick(0.1)")
        gout = await ev("(() => { const n = S47.carRadio['n']; return [n.out.gain.value, n.cab.frequency.value, S47.carRadio.live]; })()")
        check(gout[2] and gout[0] < 0.1 and gout[1] < 600, f'outside: still on, faint ({gout[0]:.3f}) and muffled ({gout[1]:.0f} Hz)')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors' + ('' if not noise else ': ' + '; '.join(noise[:3])))
        await b.close()
    print(f'{sum(res)} of {len(res)} PASS'); sys.exit(0 if all(res) else 1)
asyncio.run(main())
