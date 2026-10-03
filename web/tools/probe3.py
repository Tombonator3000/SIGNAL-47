import asyncio, time, os
from playwright.async_api import async_playwright
async def tryshot(pg, name):
    t=time.time()
    try:
        await pg.screenshot(path=f'/tmp/{name}.png', timeout=20000); print(name,'ok',round(time.time()-t,1), flush=True)
    except Exception as e: print(name,'TIMEOUT', flush=True)
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width':640,'height':360}); pg.set_default_timeout(60000)
        await pg.goto('file://' + os.path.abspath('dist-single/index.html'))
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate("S47.setQuality('low')")
        await tryshot(pg,'title')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(1500)
        await tryshot(pg,'play_nohold')
        await pg.evaluate("S47.hold=true; S47.tick(0.1)")
        await tryshot(pg,'hold_tick')
        await pg.evaluate("S47.hold=false"); await pg.wait_for_timeout(500)
        await tryshot(pg,'unhold')
        print(await pg.evaluate("({hidden:document.hidden, lock:!!document.pointerLockElement, fs:!!document.fullscreenElement})"))
        await b.close()
asyncio.run(main())
