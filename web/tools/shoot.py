# Headless walkthrough of dist-single/index.html. Takes screenshots and prints console errors.
# Usage: python3 tools/shoot.py [outdir] [WxH]
import asyncio, sys, json
from playwright.async_api import async_playwright
OUT = sys.argv[1] if len(sys.argv) > 1 else 'shots'
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '960x540').split('x'))
URL = 'file:///home/claude/signal47-web/dist-single/index.html'

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader',
                                          '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': W, 'height': H})
        pg.set_default_timeout(120000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]', timeout=60000)
        await pg.wait_for_timeout(1500)
        await pg.screenshot(path=f'{OUT}/00_title.png')
        await pg.click('button[data-a=start]')
        await pg.wait_for_timeout(2500)
        await pg.screenshot(path=f'{OUT}/01_spawn.png')

        async def view(name, x, z, yaw, pitch=0.0, wait=250):
            await pg.evaluate(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch};")
            await pg.wait_for_timeout(wait)
            await pg.screenshot(path=f'{OUT}/{name}.png')

        info = await pg.evaluate("({spawn:S47.room.spawn, bounds:S47.room.bounds})")
        print('room', json.dumps(info))
        await view('02_window', 0.05, -2.2, 0, 0.05)
        await pg.evaluate("S47.jump('residual')")
        await pg.wait_for_timeout(7500)
        await view('03_residual_crts', 0.05, -2.2, 0, -0.25)
        await pg.evaluate("S47.jump('locked')")
        await pg.wait_for_timeout(800)
        await pg.evaluate("S47.game.rxc.show(()=>{})")
        await pg.wait_for_timeout(800)
        await pg.screenshot(path=f'{OUT}/04_console_locked.png')
        await pg.evaluate("S47.game.d.ui.close(true)")
        await pg.evaluate("S47.jump('printed')")
        await pg.wait_for_timeout(600)
        await pg.screenshot(path=f'{OUT}/05_printer.png')
        await pg.evaluate("S47.jump('event')")
        await pg.wait_for_timeout(1800)
        await pg.screenshot(path=f'{OUT}/06_event.png')
        await pg.wait_for_timeout(5000)
        await pg.screenshot(path=f'{OUT}/07_turning.png')
        await pg.wait_for_timeout(9000)
        await pg.screenshot(path=f'{OUT}/08_end.png')
        r = await pg.evaluate("({phase:S47.game.phase, calls:(S47.renderer?S47.renderer.info.render.calls:null)})")
        print('state', json.dumps(r))
        print('\n'.join(errs[:40]) or 'no console errors')
        await b.close()
asyncio.run(main())
