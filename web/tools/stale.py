# The title screen's gate ("Tap to begin", the music before the menu) and a page left open across a
# new version: the drive's pieces are gone from the server (404), the night is saved and the page
# loads the new version, Continue is there, and not twice in a minute. Needs the Pages build
# (the single file has no pieces to lose): S47_URL=http://127.0.0.1:8047/SIGNAL-47/
# Usage: python3 tools/stale.py OUTDIR
import asyncio, os, sys
from playwright.async_api import async_playwright
URL = os.environ.get('S47_URL', 'http://127.0.0.1:8047/SIGNAL-47/')
OUT = sys.argv[1]; os.makedirs(OUT, exist_ok=True)
res = []
def check(ok, w): res.append(ok); print(('PASS ' if ok else 'FAIL ') + w, flush=True)
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        # 1: the gate, as a phone, as if not a test browser
        ctx = await b.new_context(viewport={'width': 892, 'height': 412}, is_mobile=True, has_touch=True)
        pg = await ctx.new_page(); pg.set_default_timeout(120000)
        await pg.add_init_script("Object.defineProperty(Navigator.prototype, 'webdriver', { get: () => false });")
        await pg.goto(URL); await pg.wait_for_selector('.title-screen')
        await pg.wait_for_timeout(500)
        g = await pg.evaluate("[document.querySelector('.title-screen').classList.contains('gated'), getComputedStyle(document.querySelector('.title-screen .menu')).display, document.querySelector('.title-screen .gate').textContent]")
        check(g[0] and g[1] == 'none' and g[2] == 'Tap to begin', f'first load: gated, menu hidden, says {g[2]}')
        await pg.screenshot(path=f'{OUT}/gate1.png')
        # tap where the New night button would be
        box = await pg.evaluate("(() => { const r = document.querySelector('[data-a=start]'); const t = document.querySelector('.title-screen'); t.classList.remove('gated'); const q = r.getBoundingClientRect(); t.classList.add('gated'); return [q.x + q.width/2, q.y + q.height/2]; })()")
        await pg.touchscreen.tap(box[0], box[1])
        await pg.wait_for_timeout(1500)
        st = await pg.evaluate("[document.querySelector('.title-screen') && document.querySelector('.title-screen').classList.contains('gated'), S47.started(), S47.game.d.audio.ctx && S47.game.d.audio.ctx.state, document.activeElement && document.activeElement.dataset.a]")
        check(st[0] is False and not st[1], f'tap: menu in, no night started by the same tap ({st})')
        check(st[2] == 'running', f'the music context is running ({st[2]})')
        await pg.screenshot(path=f'{OUT}/gate2.png')
        await ctx.close()
        # 2: a stale page: the drive's pieces are gone from the server
        ctx = await b.new_context(viewport={'width': 1280, 'height': 720})
        pg = await ctx.new_page(); pg.set_default_timeout(240000)
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL); await pg.wait_for_function('window.S47')
        await pg.evaluate("localStorage.clear(); sessionStorage.clear()"); await pg.reload(); await pg.wait_for_function('window.S47')
        check(not await pg.evaluate("document.querySelector('.title-screen').classList.contains('gated')"), 'test browser: no gate')
        await pg.route('**/assets/{Station01,RoadArea,Drive,legs,corridors}-*.js', lambda r: r.fulfill(status=404, body='gone'))
        await pg.click('button[data-a=start]'); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        await ev("S47.jump('chapter3')"); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(1.0)")
        await ev("S47.saveNow('auto', 'rotate')"); await pg.wait_for_timeout(500)
        nav = asyncio.get_event_loop().create_future()
        pg.on('framenavigated', lambda f: nav.done() or nav.set_result(1) if f == pg.main_frame else None)
        await ev("S47.game.d.inter.get('truck').use()")
        toast = ''
        for _ in range(20):
            await pg.wait_for_timeout(150)
            if nav.done(): break
            try: toast = await ev("Array.from(document.querySelectorAll('.toast')).map(e => e.textContent).join(' | ')") or toast
            except Exception: break
        check('updated' in toast, f'toast: {toast}')
        try: await asyncio.wait_for(nav, 10); ok = True
        except Exception: ok = False
        check(ok, 'the page loads itself again')
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])', timeout=60000)
        info = await ev("document.querySelector('.cont-info') && document.querySelector('.cont-info').textContent")
        check(bool(info), f'Continue is there: {info}')
        # a second failure within a minute: no reload loop
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(1.0)")
        n0 = await ev("performance.timeOrigin")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(30): await ev("S47.tick(0.25)"); await pg.wait_for_timeout(100)
        await pg.wait_for_timeout(2500)
        check(await ev("performance.timeOrigin") == n0, 'twice in a minute: no second reload')
        await b.close()
    print(f"{sum(res)} of {len(res)} PASS"); sys.exit(0 if all(res) else 1)
asyncio.run(main())
