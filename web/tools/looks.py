# Quick visual check: title, window view, console. Usage: python3 tools/looks.py OUTDIR WxH quality
import asyncio, sys, os
from playwright.async_api import async_playwright
OUT=sys.argv[1]; W,H=(int(v) for v in sys.argv[2].split('x')); Q=sys.argv[3]
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width':W,'height':H}); pg.set_default_timeout(120000)
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto('file://'+os.path.abspath('dist-single/index.html'))
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate(f"S47.setQuality('{Q}')"); await pg.wait_for_timeout(2500)
        tag=f'{Q}_{W}x{H}'
        await pg.screenshot(path=f'{OUT}/title_{tag}.png')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        await pg.evaluate("S47.hold=true; S47.tick(1.5)")
        await pg.evaluate("S47.player.place(0.05,-2.2,0); S47.player.pitch=0.06; S47.tick(0.1)")
        await pg.screenshot(path=f'{OUT}/window_{tag}.png')
        await pg.evaluate("S47.player.place(-3.5,-2.6,0.6); S47.player.pitch=0.04; S47.tick(0.1)")
        await pg.screenshot(path=f'{OUT}/motel_{tag}.png')
        await pg.evaluate("S47.game.d.inter.get('rack').use(); S47.tick(2.5); S47.game.d.inter.get('crtCenter').use(); S47.tick(0.3)")
        await pg.screenshot(path=f'{OUT}/console_{tag}.png')
        await b.close()
asyncio.run(main())
