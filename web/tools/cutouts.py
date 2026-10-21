# The cardboard alien that turns up anywhere (core/cutouts.ts; Tom, 18 October): put somewhere
# the player can walk and out of the view; seen when turned to; gone once looked away from; and
# later somewhere else. Driving: on the shoulder ahead. Not in the prologue; a note the first time.
# Usage: python3 tools/cutouts.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from intro_flow import begin_opening
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
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_function('S47.started()', polling=200)
        ev = pg.evaluate
        C = "S47.cutouts"
        up = lambda: ev(f"{C}.up")
        # (the night's one curve, story/curve.ts, stands aside here: this is the cutout on its own timer; tools/f_curve.py has them together)
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(1.0)")
        await ev(f"{C}['wait'] = 0; S47.tick(0.5)")
        check(await up() is None, 'the prologue: not yet')

        await ev("S47.jump('chapter1')"); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.game.d.ui.close(); S47.tick(1.0)")
        n0 = await ev("S47.game.notes.length")
        async def placed():
            for _ in range(40):
                await ev(f"{C}['wait'] = 0; S47.tick(0.2)")
                if await up(): return True
            return False
        check(await placed() and await up() == 'foot', 'chapter one, on foot: it is put somewhere')
        st = await ev(f"(() => {{ const c = {C}.position, p = S47.player.pos; return [c.x, c.y, c.z, Math.hypot(c.x - p.x, c.z - p.z), S47.player.walkable(c.x, c.z)]; }})()")
        vis = await ev(f"(() => {{ const v = {C}['view'](S47.camera, {C}.position); return v.inView; }})()")
        check(6 <= st[3] <= 41 and st[4] and not vis, f'{st[3]:.0f} m off, where the player can walk, out of the view ({st[:3]})')
        await ev("S47.tick(0.5)")
        check(await up() == 'foot', 'not looked at: it stays')
        # turn to it (and walk closer if it is far)
        await ev(f"(() => {{ const c = {C}.position, p = S47.player.pos, d = Math.hypot(c.x - p.x, c.z - p.z); if (d > 20) {{ const k = (d - 12) / d; S47.player.place(p.x + (c.x - p.x) * k, p.z + (c.z - p.z) * k, 0); }} }})()")
        await ev(f"(() => {{ const c = {C}.position, p = S47.player.pos; S47.player.yaw = Math.atan2(-(c.x - p.x), -(c.z - p.z)); S47.player.pitch = -0.1; }})()")
        await ev("S47.tick(0.3)")
        await ev("S47.tick(0.3)"); await ev("S47.tick(0.3)")
        await pg.screenshot(path=f'{OUT}/c01_seen.png')
        check(await ev(f"{C}.seen") == 1, f'turned to it: seen ({await ev(f"{C}.seen")})')
        check(await ev(f"{C}['mat'].map.name") == 'art/cutout', "the painted cardboard of round 14 (town/cutout_alien.png), with a little light of its own")
        nn = (await ev("S47.game.notes.slice()"))[n0:]
        check(any('cardboard alien' in n for n in nn), f'the notebook: a cardboard alien ({nn[-1:] if nn else nn})')
        first = await ev(f"[{C}.position.x, {C}.position.z]")
        # looking at it: it stays
        for _ in range(10): await ev("S47.tick(0.5)")
        check(await up() == 'foot', 'it does not go while it is looked at')
        await ev("S47.player.yaw += Math.PI; S47.tick(0.2)")
        for _ in range(8): await ev("S47.tick(0.5)")
        check(await up() is None, 'looked away from: gone')
        check(await placed(), 'and later: back')
        second = await ev(f"[{C}.position.x, {C}.position.z]")
        check(math.hypot(second[0] - first[0], second[1] - first[1]) > 0.5, f'somewhere else ({first} then {second})')
        await ev(f"(() => {{ const c = {C}.position, p = S47.player.pos; S47.player.yaw = Math.atan2(-(c.x - p.x), -(c.z - p.z)); }})()"); await ev("S47.tick(0.3)")
        await pg.screenshot(path=f'{OUT}/c02_again.png')

        # driving: on the shoulder ahead
        await ev(f"{C}.reset()")
        await ev("S47.game.d.inter.get('truck').use()")
        for _ in range(240):
            await ev("S47.tick(0.25)")
            if await ev("S47.world.driving"): break
            await pg.wait_for_timeout(100)
        check(await placed() and await up() == 'shoulder', 'driving: on the shoulder')
        d = await ev(f"(() => {{ const c = {C}.position, t = S47.world.truckHere().group.position; return Math.hypot(c.x - t.x, c.z - t.z); }})()")
        check(130 < d < 230, f'a good way ahead ({d:.0f} m)')
        await ev(f"{C}.reset(); {C}['wait'] = 0; S47.game.cinematic = true; S47.tick(0.5); S47.game.cinematic = false")

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/cutouts.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
