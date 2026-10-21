# The truck's engine (drive/engineSound.ts; Tom, 18 October: a new and better engine sound): the
# recordings are decoded and play once the truck is driven; the key starts it with its own
# recording; the idle and the cruise loops cross over with the rpm and follow it in pitch; the
# level is close to the old engine made of oscillators (which stands in until the recordings
# are in, here by keeping the files from loading); getting out shuts it off.
# Usage: python3 tools/engine.py OUTDIR   (S47_URL for the Pages build; the single file has the
# recordings inside it)
import asyncio, sys, json, os
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

TAP = """(() => { const e = S47.world.engineSound; if (!e || e.__an) return !!e; const a = e.ctx.createAnalyser(); a.fftSize = 2048; e.out.connect(a); e.__an = a; return true; })()"""
RMS = """(() => { const a = S47.world.engineSound.__an, d = new Float32Array(a.fftSize); a.getFloatTimeDomainData(d); let s = 0; for (const v of d) s += v * v; return Math.sqrt(s / d.length); })()"""

async def session(b, block):
    pg = await (await b.new_context(viewport={'width': 1280, 'height': 720})).new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') and not (block and 'engine sound load failed' in m.text) else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    if block: await pg.route('**/engine_*', lambda r: r.fulfill(status=404, body='gone'))
    await pg.goto(URL); await pg.wait_for_function('window.S47')
    await pg.evaluate("localStorage.clear()"); await pg.reload(); await pg.wait_for_function('window.S47')
    await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_function('S47.started()', polling=200)
    ev = pg.evaluate
    await ev("S47.jump('chapter1')"); await pg.wait_for_timeout(500)
    await ev("S47.hold = true; S47.game.d.ui.close(); S47.tick(1.0)")
    await ev("S47.game.d.inter.get('truck').use()")
    for _ in range(240):
        await ev("S47.tick(0.25)")
        if await ev("S47.world.driving"): break
        await pg.wait_for_timeout(100)
    await ev(TAP)
    # real time for the start and the idle to come up (the audio clock runs in real time)
    for _ in range(16): await ev("S47.tick(0.25)"); await pg.wait_for_timeout(250)
    rec = await ev("S47.world.engineSound.recorded")
    idle = sum([await ev(RMS) for _ in range(5) if not await pg.wait_for_timeout(120)]) / 5
    # cruising at 2400 rpm with some throttle, held for a second of real time (no frames run meanwhile)
    rpm = [2400]
    await ev("S47.world.engineSound.set(2400, 0.5)"); await pg.wait_for_timeout(1200)
    cruise = sum([await ev(RMS) for _ in range(5) if not await pg.wait_for_timeout(120)]) / 5
    rates = await ev("(() => { const e = S47.world.engineSound['eng']; return e && e.rec ? [e.rec.idle.playbackRate.value, e.rec.drive.playbackRate.value, e.rec.gi.gain.value, e.rec.gd.gain.value] : null; })()")
    return pg, errs, rec, idle, cruise, sum(rpm) / len(rpm), rates

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        res = {}
        if os.environ.get('S47_URL'):
            pg, errs, rec, idle, cruise, rpm, rates = await session(b, True)
            check(not rec and idle > 0.002, f'without the files: the old engine stands in (level {idle:.4f})')
            res['synth'] = (idle, cruise); await pg.context.close()
        pg, errs, rec, idle, cruise, rpm, rates = await session(b, False)
        check(rec, 'the recordings are in and playing')
        check(idle > 0.002 and cruise > idle * 0.8, f'heard at idle ({idle:.4f}) and at {rpm:.0f} rpm ({cruise:.4f})')
        if rates: check(rates[2] < rates[3] * 0.2 and rates[3] > 1 and 1.2 < rates[1] < 1.45, f'cruising: the cruise loop has it, pitched to the rpm (rates {rates[0]:.2f} / {rates[1]:.2f}, gains {rates[2]:.2f} / {rates[3]:.2f})')
        if 'synth' in res:
            k = idle / res['synth'][0]
            check(0.4 < k < 2.5, f'about as loud as the old engine at idle (x{k:.2f}) and cruising (x{cruise / res["synth"][1]:.2f})')
        ev = pg.evaluate
        await pg.keyboard.press('e')
        for _ in range(24):
            await ev("S47.tick(0.25)")
            if not await ev("S47.world.driving"): break
        await pg.wait_for_timeout(3500)
        check(not await ev("S47.world.driving") and await ev("!S47.world.engineSound['eng']"), 'out of the truck: switched off')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/engine.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
