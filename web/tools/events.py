# The town's events (story/townEvents.ts; HENDELSER.md, section 6, events 1, 6 and 8, and the
# marks on the map, section 10): the payphone rings once the player has been near it a while, the
# control room printer is on the line for 47 seconds, and a 1947 nickel is in the coin return and
# in the kit; it does not ring again. The street lamps go out one by one down the street and come
# back on, at least ten minutes of the night later. The cardboard alien stands on the highway's
# shoulder north of town, is written down once it has been seen in front of the player, and is
# gone once passed. Three marks: line A is in the notebook. After room 6 the payphone rings once
# more (an engine, nothing, clicks in four and seven). After a save and Continue none of it comes
# again. The map page with the marks is saved as a picture.
# Usage: python3 tools/events.py OUTDIR [WxH]
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000   # the road area's origin (World.ROAD_ORIGIN)
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
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def label(i): return await ev(f"(() => {{ const it = S47.game.d.inter.get('{i}'); return it ? it.label() : 'MISSING'; }})()")
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use(); 0")
        notes = lambda: ev("S47.game.notes.slice()")
        now = lambda: ev("S47.townEvents.now")
        async def clock(c): await ev(f"S47.game.clock = {c}")
        async def face(x, y, z):
            await ev(f"(()=>{{const p=S47.player; p.yaw=Math.atan2(-({x}-p.pos.x), -({z}-p.pos.z)); p.pitch=Math.atan2({y}-(p.floorY+p.eye), Math.hypot({x}-p.pos.x,{z}-p.pos.z)); S47.tick(0.05);}})()")
        # (the night's one curve, story/curve.ts, stands aside in this test: each thing on its own timer; tools/f_curve.py has them together)
        await ev("S47.hold = true; S47.curve.free = true; S47.jump('chapter1'); S47.tick(1.0)")
        # the night's own events (story/nightEvents.ts) come at random after a minute and a half or
        # more: none in this test, or one run in three the 47 s of silence held the town's back
        # (the cutout not written down, and the rest after it)
        await ev("S47.nightEvents['wait'] = 1e9")
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(120):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)

        # ---------- 1: the payphone ----------
        await clock(2 * 3600 + 50 * 60)
        await ev(f"S47.world.enter('diner'); S47.player.place({X - 12}, 2064, Math.PI / 2); S47.tick(0.2)")
        check(await ev("S47.world.area") == 'diner' and await now() == 'none' and await label('town:payphone') == 'Payphone', 'in town at 02:50 nothing yet')
        await tick(1, 12)
        check(await now() == 'none', 'not on arriving: the player has to be near a while first')
        await tick(1, 12)
        check(await now() == 'ringing' and await label('town:payphone') == 'Answer the payphone', 'after twenty seconds near it, the payphone rings')
        n0 = len(await notes())
        await use('town:payphone'); await tick(0.2)
        check(await now() == 'line', 'answered: the line is open')
        await tick(1, 30)
        check(await now() == 'line', 'thirty seconds on, still the printer on the line')
        await tick(1, 19)
        nn = await notes()
        check(await now() == 'none' and len(nn) == n0 + 1 and 'control room printer' in nn[-1] and '1947 nickel' in nn[-1], f'after 47 s the line goes dead, and the notebook has it ({nn[-1][:70] if nn else ""})')
        kit = [k['name'] for k in await ev("S47.kitItems()")]
        check('A nickel' in kit, f'the nickel is in the kit ({kit})')
        await tick(1, 40)
        check(await now() == 'none' and await label('town:payphone') == 'Payphone', 'it does not ring again')

        # ---------- 8: the lamps (ten minutes of the night after the phone at the least) ----------
        await clock(2 * 3600 + 58 * 60); await tick(1, 45)
        check(await now() == 'none', 'not before five past three, and not within ten minutes of the phone')
        # chapter 1 holds the clock before 02:55, so the night past three is driven by hand here
        await ev(f"S47.player.place({X - 2}, 2140, 0); S47.tick(0.1)")
        upd = "S47.townEvents.update(1, { ok: true, inTown: true, later: {l}, cam: S47.camera, clock: {c}, t: S47.runTime() })"
        for _ in range(42): await ev(upd.replace('{l}', 'false').replace('{c}', str(3 * 3600 + 20 * 60)))
        lamps = lambda: ev("(() => { const t = S47.world.town; return t['lamps'].map((L) => t.flood.pos[L.slot].w > 0); })()")
        check(await now() == 'lamps', 'after forty seconds out on the street at 03:20, the lamps start')
        l1 = await lamps(); await tick(1, 4); l2 = await lamps()
        check(sum(l2) < sum(l1) or sum(l1) < 7, f'one by one they go out ({sum(l1)} on, then {sum(l2)})')
        south_first = await ev("(() => { const t = S47.world.town, L = t['lamps']; const s = L.reduce((a, b) => a.z > b.z ? a : b), n = L.reduce((a, b) => a.z < b.z ? a : b); return [t.flood.pos[s.slot].w > 0, t.flood.pos[n.slot].w > 0]; })()")
        check(south_first == [False, True], f'south first: the southernmost out, the northernmost still on ({south_first})')
        n1 = len(await notes())
        await tick(1, 33)
        nn = await notes()
        check(await now() == 'none' and all(await lamps()) and len(nn) == n1 + 1 and 'street lights out one by one' in nn[-1], 'back on in the same order, and written down')

        # ---------- 6: the cutout on the highway north of town ----------
        check(await ev("S47.world.town.cutoutOut"), 'the cardboard alien stands on the shoulder')
        c = await ev("(() => { const v = S47.world.town.cutoutAt; return [v.x, v.y, v.z]; })()")
        await ev(f"S47.world.enter('road'); S47.player.place({X + c[0] + 1.5}, {c[2] - 60}, Math.PI); S47.tick(0.2)")
        await face(X + c[0], c[1] + 0.6, c[2]); await tick(0.2)
        n2 = len(await notes())
        check(n2 == len(await notes()) and not await ev("S47.townSeen.has('ev:cutout')"), 'sixty metres off it is not yet seen')
        await ev(f"S47.player.place({X + c[0] + 1.5}, {c[2] - 30}, Math.PI); S47.tick(0.1)")
        await face(X + c[0], c[1] + 0.6, c[2]); await tick(0.2)
        await pg.screenshot(path=f'{OUT}/e01_cutout.png')
        nn = await notes()
        check(await ev("S47.townSeen.has('ev:cutout')") and any('cardboard alien' in n for n in nn[n2:]), 'thirty metres off, in front: written down, "Kids, probably."')
        await tick(1, 2)
        nn = await notes()
        check(any('Three on one line' in n for n in nn), 'three marks on A: the line is drawn in')
        await ev(f"S47.player.place({X - 1.8}, {c[2] + 200}, Math.PI); S47.tick(0.3)")
        check(not await ev("S47.world.town.cutoutOut"), 'passed and far behind, it is gone from the road')
        await ev("S47.world.enter('diner'); S47.tick(0.1)")
        await ev(f"S47.player.place({X + 18.9}, 2062.0, 0); S47.tick(0.1)")
        await use('town:souvenir'); await tick(0.1)
        check('dew on it' in await ev("[...document.querySelectorAll('.toast')].map((e) => e.textContent).join(' ')"), 'and it is back in the shop window, with dew on it')

        # ---------- 12: the deputy's car on the shoulder, while he is out (driven by hand) ----------
        car = await ev("(() => { const v = S47.world.town.carShoulder; return [v.x, v.y, v.z]; })()")
        hand = lambda c: ev(f"S47.townEvents.update(0.5, {{ ok: true, inTown: false, later: false, cam: S47.camera, clock: {c}, t: S47.runTime() }})")
        await ev(f"S47.world.enter('road'); S47.player.place({X - 1.8}, {car[2] - 400}, Math.PI); S47.tick(0.1)")
        await ev("S47.world.town.setCarOut(false)")   # the lamps' night past three set it out already
        await hand(3 * 3600 + 5 * 60)
        check(not await ev("S47.world.town.carOut"), 'not before 03:12')
        await hand(3 * 3600 + 14 * 60)
        check(await ev("S47.world.town.carOut && S47.world.town.group.getObjectByName('town:patrolCar').position.z") == car[2], 'at 03:14, out of sight down the road, the deputy\'s car is on the shoulder')
        n5 = len(await notes())
        await ev(f"S47.player.place({X - 1.8}, {car[2] - 18}, Math.PI); S47.tick(0.1)")
        await hand(3 * 3600 + 15 * 60)
        await face(X + car[0], car[1] + 1, car[2]); await pg.screenshot(path=f'{OUT}/e03_car.png')
        nn = await notes()
        check(len(nn) == n5 + 1 and 'nobody in it' in nn[-1], 'come up to it: engine running, door open, nobody, in the notebook')
        await ev(f"S47.player.place({X - 1.8}, {car[2] + 180}, Math.PI); S47.tick(0.1)")
        await hand(3 * 3600 + 16 * 60)
        check(not await ev("S47.world.town.carOut"), 'driven on past it: gone')
        await hand(3 * 3600 + 20 * 60); await ev(f"S47.player.place({X - 1.8}, {car[2] - 400}, Math.PI); S47.tick(0.1)"); await hand(3 * 3600 + 21 * 60)
        check(not await ev("S47.world.town.carOut"), 'and it does not come back')
        await ev("S47.world.enter('diner'); S47.tick(0.1)")

        # ---------- the second call, after room 6 (driven by hand: later) ----------
        await clock(3 * 3600 + 50 * 60); await ev(f"S47.player.place({X - 12}, 2064, Math.PI / 2); S47.tick(0.1)")
        upd = "S47.townEvents.update(1, { ok: true, inTown: true, later: {l}, cam: S47.camera, clock: S47.game.clock, t: S47.runTime() })"
        for _ in range(25): await ev(upd.replace('{l}', 'false'))
        check(await now() == 'none', 'before room 6 it rings only the once')
        for _ in range(25): await ev(upd.replace('{l}', 'true'))
        check(await now() == 'ringing', 'after room 6 it rings again')
        n3 = len(await notes())
        await use('town:payphone')
        for _ in range(48): await ev(upd.replace('{l}', 'true'))
        nn = await notes()
        check(await now() == 'none' and len(nn) == n3 + 1 and 'four and seven' in nn[-1], 'an engine dying on the line, then clicks, four and seven')

        # ---------- the map ----------
        m = await ev("S47.game.notebookExtras().map")
        open(f'{OUT}/e02_map.png', 'wb').write(base64.b64decode(m.split(',', 1)[1]))
        marks = await ev("[...S47.townSeen].filter((k) => k.startsWith('mark:')).length")
        check(marks == 5, f'five marks for the map (the phone twice, the lamps, the cutout, the car: {marks})')

        # ---------- save and Continue: none of it again ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)")
        for _ in range(60):
            if await ev("!!S47.world.town"): break
            await pg.wait_for_timeout(200)
        await clock(3 * 3600 + 10 * 60)
        await ev(f"S47.world.enter('diner'); S47.player.place({X - 12}, 2064, Math.PI / 2); S47.tick(0.2)")
        n4 = len(await notes())
        await tick(1, 50)
        check(await now() == 'none' and len(await notes()) == n4, 'after Continue the phone does not ring and the lamps stay on')
        check(not await ev("S47.world.town.cutoutOut") and 'A nickel' in [k['name'] for k in await ev("S47.kitItems()")], 'and the cutout is not on the road again; the nickel is still in the kit')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/events.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
