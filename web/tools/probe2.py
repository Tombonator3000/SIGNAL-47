import asyncio, time, os
from playwright.async_api import async_playwright
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width':640,'height':360}); pg.set_default_timeout(60000)
        await pg.goto('file://' + os.path.abspath('dist-single/index.html'))
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate("S47.setQuality('low')")
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        await pg.evaluate("S47.hold=true")
        t=time.time(); await pg.screenshot(path='/tmp/a.png', animations='disabled'); print('shot1', round(time.time()-t,2), flush=True)
        await pg.evaluate("S47.game.d.inter.get('rack').use()")
        t=time.time(); await pg.evaluate("S47.tick(2.5)"); print('tick', round(time.time()-t,2), flush=True)
        t=time.time(); await pg.screenshot(path='/tmp/b.png', animations='disabled'); print('shot2', round(time.time()-t,2), flush=True)
        await pg.evaluate("S47.player.place(0.05,-1.7,0); S47.player.pitch=-0.32; S47.tick(0.2)")
        t=time.time(); await pg.screenshot(path='/tmp/c.png', animations='disabled'); print('shot3', round(time.time()-t,2), flush=True)
        await b.close()
asyncio.run(main())
