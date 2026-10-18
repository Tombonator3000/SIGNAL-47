# The time away (story/abduction.ts; HENDELSER.md, section 8): out on the highway's shoulder
# south of SARO, on line B, alone at night, the pressure rises; the warning comes (the footsteps
# go, the flashlight dies, the tone); going indoors before the light lets it pass over. Taken:
# the light, the frames, black; the clock jumps 17 to 47 minutes and stops short of the next
# fixed point; the player wakes on their back further along, on ground that can be stood on, in
# a ring of frost, the flashlight on; the notebook has the lost time, the line in their own hand
# and the nosebleed; the field camera has one exposure too many; the truck wants the key; the
# recording stands where they were taken, and is gone when walked up to. Not again within forty
# minutes, at most three, not with the setting off; kept after a save and Continue.
# Usage: python3 tools/abduct.py OUTDIR [WxH]
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        A = "S47.abduction()"
        stage = lambda: ev(f"{A}.stage")
        notes = lambda: ev("S47.game.notes.slice()")
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def until(st, most=120, step=0.5):
            for _ in range(int(most / step)):
                if await stage() == st: return True
                await ev(f"S47.tick({step})")
            return await stage() == st
        await ev("S47.hold = true; S47.jump('chapter2'); S47.tick(1.0)")
        await ev("S47.game.d.ui.close(); S47.game.clock = 2 * 3600 + 58 * 60; S47.tick(0.2)")
        await ev("S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        ab = await ev("(() => { const s = S47.abductIn(); return [s.line, s.nextFixed, s.enabled]; })()")
        check(ab[0] == 'B' and ab[1] == 3 * 3600 + 40 * 60 and ab[2], f'on the shoulder south of SARO in chapter two: on line B, the next fixed point 03:40 ({ab})')
        check(await stage() == 'calm' and await ev(f"{A}.pressure") < 0.05, 'nothing at once')
        await tick(1, 60)
        pr = await ev(f"{A}.pressure")
        check(0 < pr < 1 and await stage() == 'calm', f'a minute alone on the line: the pressure rises ({pr:.2f})')

        # ---------- the warning, and getting away from it ----------
        await ev(f"{A}.pressure = 5; S47.tick(0.5)")
        check(await stage() == 'warning', 'at the top of it, the warning')
        await ev("S47.flashlight.have = true; S47.flashlight.on = true")
        wl = await ev(f"{A}['warnLen']")
        await tick(0.5, int(wl * 0.5 / 0.5) + 2)
        check(await ev(f"{A}.silent") and await stage() == 'warning', f'half way through ({wl:.0f} s): the footsteps have gone')
        await ev("S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.5)")
        check(await stage() == 'calm' and not await ev(f"{A}.silent") and await ev("S47.flashlight.on"), 'indoors before the light: it passes over, the flashlight comes back')

        # ---------- taken ----------
        await ev("S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        c0 = await ev("S47.game.clock")
        n0 = len(await notes())
        d0 = await ev("S47.game.docs.length")
        await ev(f"{A}.pressure = 5; S47.tick(0.5)")
        take = await ev("[S47.player.pos.x, S47.player.pos.z]")
        check(await until('light', 60), 'the warning runs its length: the light')
        await tick(0.5, 4)
        await pg.screenshot(path=f'{OUT}/a01_light.png')
        check(await until('taken', 6), 'and they take you')
        await tick(0.5, 2)
        await pg.screenshot(path=f'{OUT}/a02_frame.png')
        nfr = await ev(f"{A}['frames'].length")
        check(nfr >= 3, f'a few single frames ({nfr})')
        check(await until('back', 20, 0.25), 'black, and back')
        c1 = await ev("S47.game.clock")
        lost = (c1 - c0) / 60
        check(17 <= lost <= 47 + 1 and c1 < 3 * 3600 + 40 * 60, f'the clock has jumped {lost:.1f} minutes, short of 03:40')
        pos = await ev("[S47.player.pos.x, S47.player.pos.z, S47.player.walkable(S47.player.pos.x, S47.player.pos.z), S47.player.pitch]")
        moved = math.hypot(pos[0] - take[0], pos[1] - take[1])
        check(moved >= 29 and pos[2], f'{moved:.0f} m further on, on ground that can be stood on')
        check(pos[3] > 0.8, f'on your back, looking up at the stars (pitch {pos[3]:.2f})')
        await pg.screenshot(path=f'{OUT}/a03_back.png')
        check(await until('calm', 12), 'the view lifts')
        nn = (await notes())[n0:]
        check(any('Nothing.' in n for n in nn) and any('own hand' in n for n in nn) and any('nosebleed' in n.lower() for n in nn), f'the notebook: the lost time, the line in your own hand, the nosebleed ({len(nn)} notes)')
        docs = await ev("S47.game.docs.filter((d) => d.id.startsWith('away')).map((d) => [d.id, !!d.image, d.title])")
        check(len(docs) == 1 and docs[0][1], f'one exposure too many on the field camera ({docs})')
        if docs:
            img = await ev("S47.game.docs.find((d) => d.id === 'away1').image")
            open(f'{OUT}/a04_from_above.jpg', 'wb').write(base64.b64decode(img.split(',', 1)[1]))
        check(await ev("S47.flashlight.on") and await ev("S47.world.keyStall"), 'the flashlight is on; the truck will want the key')
        ring = await ev("(() => { const r = S47.scene.getObjectByName('abduct:ring'); return r ? Math.hypot(r.position.x - S47.player.pos.x, r.position.z - S47.player.pos.z) : -1; })()")
        check(0 <= ring < 1, f'in a ring of frost ({ring:.2f} m from its middle)')
        rec = lambda: ev("(() => { const r = S47.scene.getObjectByName('abduct:record'); return r ? [r.visible, Math.hypot(r.position.x - S47.player.pos.x, r.position.z - S47.player.pos.z)] : null; })()")
        r = await rec()
        check(r is not None and (r[0] == (r[1] > 45)), f'the recording stands where you were taken ({r})')
        await ev(f"S47.player.place({take[0]} + 10, {take[1]}, 0); S47.tick(0.2)")
        r = await rec()
        check(r is not None and not r[0], 'walk up to it, and it was not there')

        # ---------- not again so soon; the setting ----------
        await ev("S47.player.place(-24, 200, Math.PI); S47.tick(0.2)")
        await ev(f"{A}.pressure = 5"); await tick(1, 3)
        check(await stage() == 'calm', 'not again within forty minutes of the night')
        await ev(f"{A}.data.last = -1e9")
        await ev("S47.game.clock = 2 * 3600 + 58 * 60")
        await ev("S47.setAbductions(false)"); await ev(f"{A}.pressure = 5"); await tick(1, 2)
        check(await stage() == 'calm', 'with Missing time off in the settings: nothing')
        await ev("S47.setAbductions(true)")
        cnt = await ev(f"{A}.data.count")
        await ev(f"{A}.data.count = 3"); await ev(f"{A}.pressure = 5"); await tick(1, 2)
        check(await stage() == 'calm', 'and never more than three in a night')
        await ev(f"{A}.data.count = {cnt}; {A}.pressure = 0")

        # ---------- save and Continue ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        kept = await ev(f"[{A}.data.count, !!S47.scene.getObjectByName('abduct:ring'), !!S47.scene.getObjectByName('abduct:record'), S47.game.docs.some((d) => d.id === 'away1')]")
        check(kept[0] >= 1 and kept[1] and kept[2] and kept[3], f'after Continue: the time away is kept, the ring, the recording and the exposure ({kept})')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/abduct.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
