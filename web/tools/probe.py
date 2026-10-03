import asyncio, json, time
from playwright.async_api import async_playwright
async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader','--enable-unsafe-swiftshader','--ignore-gpu-blocklist','--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width':640,'height':360}); pg.set_default_timeout(100000)
        await pg.goto('file:///home/claude/signal47-web/dist-single/index.html')
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate("S47.setQuality('low')")
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        await pg.evaluate("S47.hold=true")
        T = lambda js: pg.evaluate(f"(()=>{{const t=performance.now(); {js}; return Math.round(performance.now()-t)}})()")
        print('tick0', await T("S47.tick(0)"), flush=True)
        print('render', await T("S47.renderer.render(S47.scene,S47.camera); S47.renderer.getContext().finish()"), flush=True)
        print('crts unpowered x5', await T("for(let i=0;i<5;i++)S47.game.drawCrts(1)"), flush=True)
        await pg.evaluate("S47.game.d.inter.get('rack').use()")
        print('tick2.5', await T("S47.tick(2.5)"), flush=True)
        print('crts powered x5', await T("for(let i=0;i<5;i++)S47.game.drawCrts(1)"), flush=True)
        print('render', await T("S47.renderer.render(S47.scene,S47.camera); S47.renderer.getContext().finish()"), flush=True)
        print('render2', await T("S47.renderer.render(S47.scene,S47.camera); S47.renderer.getContext().finish()"), flush=True)
        print(await pg.evaluate("JSON.stringify(S47.renderer.info.render)"))
        await b.close()
asyncio.run(main())
