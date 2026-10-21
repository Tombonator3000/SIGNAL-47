# Loads the single-file build under a strict content-security policy, close to what a
# published claude.ai artifact gets (no network, fonts only from fonts.gstatic.com), and
# checks that all fonts and sounds still load. Usage, after npm run build:single:
#   python3 tools/csptest.py
import asyncio, os, tempfile
from pathlib import Path
from intro_flow import begin_opening
from playwright.async_api import async_playwright
from readaloud import wait_for_js

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
        # Legacy timing/voice baseline; new defaults are exercised in ensemble.py/serling.py.
        await pg.add_init_script("try{localStorage.setItem('s47.serlingNarrator','false');localStorage.setItem('s47.characterVoices','false');}catch{}")
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto('file://' + path)
        await pg.wait_for_selector('button[data-a=start]')
        fonts = await pg.evaluate("[...document.fonts].filter(f => f.status === 'loaded').map(f => f.family)")
        await pg.click('button[data-a=start]'); await begin_opening(pg)
        await wait_for_js(pg, 'S47.started() && S47.audio.ready')
        await pg.wait_for_timeout(1500)
        await pg.evaluate("S47.hold = true; S47.tick(1)")
        sound_keys = await pg.evaluate("Object.keys(S47.game.d.audio.buf || {})")
        sounds = len(sound_keys)
        ordinary = len([key for key in sound_keys if not key.startswith(('phone:', 'lic:', 'character:'))])
        telephone = len([key for key in sound_keys if key.startswith('phone:')])
        licensed_names = {key[4:] for key in sound_keys if key.startswith('lic:')}
        licensed = len(licensed_names)
        # The optional private bank grew in PR138 (roof_steps). Verify the exact
        # present file inventory, preserving the no-private-assets build case.
        expected_licensed = {p.stem for p in Path('src/assets/licensed/sfx').glob('*')
                             if p.is_file() and p.suffix in ('.mp3', '.wav', '.ogg')}
        characters = len([key for key in sound_keys if key.startswith('character:')])
        art = await pg.evaluate("S47.art()")
        print('fonts loaded:', len(fonts), fonts)
        print('sounds decoded:', sounds, {'ordinary': ordinary, 'telephone': telephone, 'character': characters, 'licensed': licensed})
        print('art images loaded:', len(art['loaded']), '/', art['expected'])
        print('\n'.join(errs[:15]) or 'no console errors or warnings')
        # the 28 start images, plus later images already asked for: the Halley poster loads
        # right after the start (round 10), so it must come through the policy too
        start = [i for i in art['loaded'] if i not in art['later']]
        early = sorted(i for i in art['loaded'] if i in art['later'])
        print('later images already loaded:', early)
        passed = len(fonts) == 4 and ordinary == 47 and telephone == 24 and characters == 11 and licensed_names in (set(), expected_licensed) and len(start) == art['expected'] == 30 and 'halleyPoster' in early and not errs
        print('PASS' if passed else 'FAIL')
        await b.close()
        if not passed:
            raise SystemExit(1)

asyncio.run(main())
