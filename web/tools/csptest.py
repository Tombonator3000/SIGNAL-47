# Loads the single-file build under a strict content-security policy, close to what a
# published claude.ai artifact gets (no network, fonts only from fonts.gstatic.com), and
# checks that all fonts and sounds still load. Usage, after npm run build:single:
#   python3 tools/csptest.py
import asyncio, os, tempfile
from playwright.async_api import async_playwright

CSP = ("default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; "
       "img-src data: blob:; media-src data: blob:; font-src https://fonts.gstatic.com")

async def main():
    src = open('dist-single/index.html', encoding='utf-8').read()
    page_html = src.replace('<head>', f'<head>\n<meta http-equiv="Content-Security-Policy" content="{CSP}">', 1)
    path = os.path.join(tempfile.mkdtemp(), 'csp_test.html')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(page_html)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 844, 'height': 390}); pg.set_default_timeout(90000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto('file://' + path)
        await pg.wait_for_selector('button[data-a=start]')
        fonts = await pg.evaluate("[...document.fonts].filter(f => f.status === 'loaded').map(f => f.family)")
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(1500)
        await pg.evaluate("S47.hold = true; S47.tick(1)")
        sounds = await pg.evaluate("Object.keys(S47.game.d.audio.buf || {}).length")
        print('fonts loaded:', len(fonts), fonts)
        print('sounds decoded:', sounds)
        print('\n'.join(errs[:15]) or 'no console errors or warnings')
        print('PASS' if len(fonts) == 4 and sounds == 11 and not errs else 'FAIL')
        await b.close()

asyncio.run(main())
