# Full prologue through the real interactables and console DOM.
# The render loop is held and the game is advanced with S47.tick(), so it runs
# in a software renderer too. Usage: python3 tools/walkthrough.py OUTDIR WxH quality
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, math, sys, json, time, os
from playwright.async_api import async_playwright
OUT = sys.argv[1]; W, H = (int(v) for v in sys.argv[2].split('x')); Q = sys.argv[3] if len(sys.argv) > 3 else 'high'
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        # Headless Chromium stalls screenshots while the pointer is locked, so tests skip the lock.
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.evaluate(f"S47.setQuality('{Q}')")
        await pg.wait_for_timeout(2500)
        await pg.screenshot(path=f'{OUT}/00_title.png')
        await pg.click('button[data-a=start]')            # real click, proves the fade layer no longer blocks
        await pg.wait_for_timeout(300)
        await pg.evaluate("S47.hold = true")
        ev = lambda js: pg.evaluate(js)
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def look(x, z, yaw, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch};")
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use()")
        await tick(2.0); await shot('01_spawn')
        check(await ev("S47.game.phase") == 'intro', 'game starts in intro phase')
        await use('logbook'); await tick(0.2); await shot('02_shiftlog')
        await ev("S47.game.d.ui.close()"); await tick(1)
        check(await ev("S47.game.logRead") is True, 'shift log read and filed')
        await use('coffeePot'); await tick(2)
        check(await ev("S47.game.coffee") == 'poured', 'coffee poured')
        await use('rack'); await tick(2.5)
        check(await ev("S47.game.powered") is True, 'RX bank 3 powered')
        await look(0.05, -1.7, 0, -0.32); await tick(0.2); await shot('03_crts_on')
        await use('crtCenter'); await tick(0.3); await shot('04_console_cal')
        async def setrx(f, g, bw, az):
            await ev(f"""(()=>{{const set=(k,v)=>{{const i=document.querySelector('input[data-k='+k+']'); i.value=v; i.dispatchEvent(new Event('input'))}};
              set('frequency',{f}); set('gain',{g}); set('bandwidth',{bw}); set('azimuth',{az});}})()""")
        # wrong values first: must not pass
        await setrx(1419.7, 30, 80, 20); await pg.click('[data-a=act]')
        check(await ev("S47.game.rx.stage") == 0, 'wrong calibration is rejected')
        await setrx(1419.900, 55, 48, 42); await pg.click('[data-a=act]')
        check(await ev("S47.game.rx.stage") == 1, 'calibration 1419.900 accepted')
        await setrx(1420.110, 55, 15, 42); await pg.click('[data-a=act]')
        check(await ev("S47.game.rx.stage") == 2, 'interference 1420.110 notched')
        await tick(0.2); await shot('05_console_survey')
        await pg.click('[data-a=exit]'); await tick(12)
        await tick(4)
        check(await ev("S47.game.phase") == 'residual', 'time skip lands at 02:13 residual')
        await tick(7)
        check(await ev("S47.game.rxc.residualVisible") is True, 'residual appears on the spectrum')
        await use('crtCenter'); await tick(0.2)
        # the signal comes through as the receiver closes on 1420.405 (core/signalVoice.ts)
        await setrx(1420.30, 90, 10, 83); await tick(0.5)
        off = await ev("S47.game.d.audio.signal ? S47.game.d.audio.signal.level : 0")
        await setrx(1420.405, 90, 10, 83); await tick(0.5); await shot('06_console_residual')
        on = await ev("S47.game.d.audio.signal ? S47.game.d.audio.signal.level : 0")
        check(on > 0.8 and on - off > 0.4, f'the signal gets clearer as the receiver closes on 1420.405 ({off:.2f} off the peak, {on:.2f} on it)')
        await pg.click('[data-a=act]')
        check(await ev("S47.game.rx.stage") == 3 and await ev("S47.game.phase") == 'locked', 'pattern 4/7 locked at 1420.405')
        await tick(1.5); await shot('07_console_locked')
        await pg.click('[data-a=exit]'); await tick(0.5)
        # the signal processor has the tape now: play it harmonized, then stop
        await use('decoder'); await tick(0.2)
        check(await ev("!!document.querySelector('.dsp-scope')"), 'the signal processor opens with the tape')
        await pg.click('.labpanel [data-a=play]'); await tick(0.6)
        check(await ev("S47.game.decoder.playing"), 'the tape plays harmonized from the rack')
        await pg.wait_for_timeout(1500); await tick(0.5); await shot('07b_signal_processor')
        await pg.click('.labpanel [data-a=play]'); await tick(0.2)
        check(not await ev("S47.game.decoder.playing"), 'and stops')
        await ev("S47.game.d.ui.close()"); await tick(0.2)
        # the doors are free, even in the prologue: open the service door and shut it again
        await look(4.5, 1.6, -math.pi / 2, 0.0); await tick(0.1)
        await use('doorEast'); await tick(1.2)
        check(await ev("S47.doors.isOpen('east') && S47.yard.zone.eastDoor.enabled"), 'the service door opens in the prologue')
        await use('doorEast'); await tick(1.2)
        check(await ev("!S47.doors.isOpen('east') && !S47.yard.zone.eastDoor.enabled"), 'and shuts again')
        await use('crtRight'); await tick(5)
        check(await ev("S47.game.phase") == 'printing', 'direction solve starts printer')
        await tick(6)
        check(await ev("S47.game.phase") == 'printed', 'printout ready')
        await use('printer'); await tick(0.2); await shot('08_printout')
        await ev("S47.game.d.ui.close()"); await tick(3.5)
        check(await ev("S47.game.ringing") is True, 'phone rings after printout')
        await use('phone'); await tick(6)
        check(await ev("S47.game.phase") == 'countdown', 'call ends, 47 s countdown runs')
        await look(2.4, 1.6, 3.14159, -0.35); await tick(0.2); await shot('09_mug_before')
        since = await ev("S47.game.gt - S47.game.lineDeadT")
        await tick(46.6 - since)
        check(await ev("S47.game.phase") == 'countdown', 'nothing happens before 47 s (t=%.1f)' % await ev("S47.game.gt - S47.game.lineDeadT"))
        await tick(0.6)
        check(await ev("S47.game.phase") == 'event', 'impact at 47 s (t=%.1f)' % await ev("S47.game.gt - S47.game.lineDeadT"))
        await tick(1.8); await shot('10_mug_broken')
        check(await ev("S47.room.mug.visible") is False, 'mug shattered')
        await tick(1.4)
        check(await ev("S47.game.phase") == 'turning', 'dishes start turning')
        await tick(1.5)
        await look(0.05, -2.2, 0, 0.08); await tick(0.1); await shot('11_dishes_turning')
        for i in range(30):
            await tick(1)
            if await ev("S47.game.phase") in ('end',): break
        check(await ev("S47.game.phase") == 'end', 'all dishes arrived')
        az = await ev("S47.ext.dishes.map(d=>Math.round(d.curAz))")
        check(all(a == 26 for a in az), f'all {len(az)} dishes at az 026')
        # No end card any more: the night goes on. A chapter card on the black closes the
        # prologue and names chapter one, then the yard door is open behind it.
        for i in range(20):
            await tick(0.5)
            if await ev("!!document.querySelector('.chapter-card')"): break
        check(await ev("!!document.querySelector('.chapter-card')"), 'chapter card after the prologue')
        text = await ev("document.querySelector('.chapter-card').textContent")
        check('NIGHT SHIFT' in text and 'CHAPTER ONE' in text and 'THE SECOND EXPOSURE' in text, 'chapter card closes the prologue and names chapter one')
        check(await pg.locator('.endcard').count() == 0, 'no end card between the prologue and chapter one')
        await pg.wait_for_function("() => { const b = document.querySelector('.chapter-card .cc-b'); return !!b && getComputedStyle(b).opacity === '1'; }", polling=500)
        await shot('12_chapter_card')
        await tick(7)
        check(await ev("!document.querySelector('.chapter-card:not(.out)')") and await ev("S47.game.phase") == 'ch1' and not await ev("S47.game.cinematic"), 'chapter one begins behind the card')
        # the new chapter is an autosave of this case, with a picture and the chapter's name
        await pg.wait_for_function("S47.saves.list(S47.caseId()).some(m => m.chapter.startsWith('Chapter 1'))", polling=300, timeout=20000)
        m = await ev("S47.saves.list(S47.caseId())[0]")
        check(m['kind'] == 'auto' and m['chapter'] == 'Chapter 1: The Second Exposure' and (m['thumb'] or '').startswith('data:image/jpeg'), f"autosave at chapter one: {m['id']}, {m['place']}, {m['clock']}")
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
