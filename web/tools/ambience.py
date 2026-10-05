# The desert outside (core/ambience.ts): it runs, it varies, and it follows the night.
# Crickets around the listener at midnight and none left at dawn, the wind's level and
# colour wandering, the outdoor level turned down indoors and in the cab, and every one of
# the far-off sounds played once without an error. Audio is not listened to here: how it
# sounds on real speakers is UNVERIFIED.
# Usage: python3 tools/ambience.py
import asyncio, sys, os
from playwright.async_api import async_playwright

URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': 960, 'height': 600}); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL); await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]')
        ev = pg.evaluate
        await pg.wait_for_function("S47.started()")
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1)")
        check(await ev("!!S47.audio.night"), 'the night outside starts with the game')
        # outside in the service yard
        await ev("S47.player.place(12, 6, 0); S47.tick(0.5)")
        sp = await ev("S47.audio['space']")
        check(sp == 'yard', f'out in the yard the outdoor sounds are up ({sp})')
        await asyncio.sleep(3)   # the level eases in on the audio clock
        out_yard = await ev("S47.audio.night['out'].gain.value")
        async def run(secs, clock):
            # real time does not pass in a headless tick; drive the ambience on its own clock
            return await ev(f"""(() => {{ const n = S47.audio.night, at = S47.player.pos; const seen = [];
              for (let i = 0; i < {secs} * 10; i++) {{ n.update(0.1, at, {clock}); if (i % 50 === 0) seen.push(n['crickets'].filter(c => !c.dying).length); }}
              return {{ crickets: n['crickets'].filter(c => !c.dying).length, seen, wind: n['wind'].level.value, colour: n['wind'].colour.value }}; }})()""")
        mid = await run(90, 0 * 3600 + 10 * 60)
        check(mid['crickets'] >= 1, f"after midnight there are crickets round the listener ({mid['crickets']}, over time {mid['seen']})")
        w1 = mid['wind']; c1 = mid['colour']
        late = await run(240, 3 * 3600 + 50 * 60)
        check(late['wind'] != w1 or late['colour'] != c1, f"the wind wanders (level {w1:.3f} -> {late['wind']:.3f}, colour {c1:.0f} -> {late['colour']:.0f} Hz)")
        dawn = await run(120, 5 * 3600 + 26 * 60)
        check(dawn['crickets'] == 0, f"by 05:26 the crickets have gone quiet ({dawn['crickets']})")
        # every far-off sound once
        for k in ['gust', 'poorwill', 'coyotes', 'train', 'truck', 'meadowlark', 'lark']:
            await ev(f"(() => {{ const n = S47.audio.night; n['{k}'](S47.player.pos); }})()")
        await ev("(() => { const n = S47.audio.night; n['far']('owl', S47.player.pos, 0.2); n['far']('dog0', S47.player.pos, 0.2); })()")
        check(True, 'gust, poorwill, coyotes, train, truck, meadowlark, lark, owl and dog all played')
        # indoors and in the cab
        await ev("S47.audio.setSpace('room')")
        await asyncio.sleep(3)
        room = await ev("({ g: S47.audio.night['out'].gain.value, f: S47.audio.night['lp'].frequency.value })")
        await ev("S47.audio.setSpace('lab')")
        await asyncio.sleep(3)
        cab = await ev("S47.audio.night['out'].gain.value")
        await ev("S47.audio.setSpace('yard')")
        check(room['g'] < out_yard * 0.5 and room['f'] < 1500 and cab < room['g'], f"indoors muffled ({room['g']:.2f}, {room['f']:.0f} Hz), in the cab lower still ({cab:.2f}), outside {out_yard:.2f}")
        bad = [e for e in errs if 'GPU stall' not in e and 'Automatic fallback to software WebGL' not in e]
        print('\n'.join(bad) if bad else 'no console errors/warnings')
        await b.close()
    n = sum(1 for c in checks if c[0] == 'PASS')
    print(f'{n} of {len(checks)} PASS')
    sys.exit(0 if n == len(checks) and not bad else 1)

asyncio.run(main())
