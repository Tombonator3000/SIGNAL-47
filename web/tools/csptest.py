import asyncio
from playwright.async_api import async_playwright
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width':844,'height':390}); pg.set_default_timeout(90000)
        errs=[]
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error','warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: '+str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto('file:///tmp/csp_test.html')
        await pg.wait_for_selector('button[data-a=start]')
        print('fonts', await pg.evaluate("[...document.fonts].filter(f=>f.status==='loaded').map(f=>f.family)"))
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(1500)
        await pg.evaluate("S47.hold=true; S47.tick(1)")
        print('audio buffers', await pg.evaluate("Object.keys(S47.game.d.audio.buf||{}).length"))
        print('\n'.join(errs[:15]) or 'no errors')
        await b.close()
asyncio.run(main())
