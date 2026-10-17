# Them, in the background (core/glimpses.ts; HENDELSER.md, section 7): the figure stands 40 to
# 300 m off at the edge of the view, behind the antennas at SARO and out in the dark round
# Caliche and the diner; it is gone once looked at, zoomed at, lit, come near, or when the story
# says no; left to itself it comes now and then, more often towards morning, never nearer than
# 40 m, never in the middle of the view, at most eight in a night. Screenshots with it standing.
# Usage: python3 tools/glimpses.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000
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
        up = lambda: ev("S47.glimpses.up")
        fig = lambda: ev("(() => { const f = S47.scene.getObjectByName('glimpse'); const c = S47.camera.getWorldPosition(f.position.clone()); return [f.visible, f.position.distanceTo(c)]; })()")
        async def look(yaw, pitch=0.0): await ev(f"S47.player.yaw = {yaw}; S47.player.pitch = {pitch}; S47.tick(0.05)")
        async def at_fig():
            await ev("(() => { const f = S47.scene.getObjectByName('glimpse').position, p = S47.player; p.yaw = Math.atan2(-(f.x - p.pos.x), -(f.z - p.pos.z)); p.pitch = Math.atan2(f.y + .6 - (p.floorY + p.eye), Math.hypot(f.x - p.pos.x, f.z - p.pos.z)); S47.tick(0.05); })()")
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0)")
        await ev("S47.game.clock = 2 * 3600 + 40 * 60")

        # ---------- SARO: behind an antenna in the far rows ----------
        # outside the control room on the walk, looking north and a little east of S-11
        await ev("S47.world.enter('saro'); S47.player.place(-2, -24, 0); S47.tick(0.3)")
        await look(-0.75)
        ok = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')")
        f = await fig()
        check(ok and await up() == 'S-11' and f[0] and f[1] > 40, f'off the middle of the view, behind S-11, {f[1]:.0f} m off')
        await pg.screenshot(path=f'{OUT}/g01_saro_edge.png')
        await ev("S47.tick(0.1)")
        check(await up() == 'S-11', 'while not looked at, it stays')
        await at_fig()
        check(await up() is None and not (await fig())[0], 'looked at: it was not there')
        await look(-0.75)
        ok = await ev("S47.glimpses.now(S47.glimpseIn(), 'S-11')")
        await ev("S47.game.cinematic = true; S47.tick(0.1)")
        check(await up() is None, 'and not in a cinematic either')
        await ev("S47.game.cinematic = false; S47.tick(0.1)")

        # ---------- the road scene: out past the diner's light ----------
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(120):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev(f"S47.world.enter('diner'); S47.player.place({X - 2}, 2000, Math.PI / 2); S47.tick(0.3)")
        await look(math.pi / 2 + 0.55)
        ok = await ev("S47.glimpses.now(S47.glimpseIn(), 'diner-west')")
        f = await fig()
        check(ok and f[0] and f[1] > 40, f'west of the diner, at the edge of the lot\'s light ({f[1]:.0f} m)')
        g = await ev("(() => { const f = S47.scene.getObjectByName('glimpse').position; return [f.y, S47.world.town.floorAt(f.x, f.z)]; })()")
        check(abs(g[0] - g[1]) < 0.5, f'standing on the ground ({g[0]:.2f} against {g[1]:.2f})')
        await pg.screenshot(path=f'{OUT}/g02_diner_edge.png')
        await ev("S47.input.zoomHeld = true"); await ev("S47.tick(0.4)")
        check(await up() is None, 'zoomed at: gone')
        await ev("S47.input.zoomHeld = false; S47.tick(0.4)")

        # ---------- left to itself, a night of it ----------
        res = await ev('''(() => { const G = S47.glimpses, seen = []; let near = 0, mid = 0; G.reset();
          for (let i = 0; i < 2400; i++) {
            S47.player.yaw = (i * 0.37) % (Math.PI * 2);
            S47.camera.updateMatrixWorld(true);
            const s = S47.glimpseIn(); s.clock = 3 * 3600 + i * 1.0;
            S47.player.update(0.001, 0, 0, false); S47.camera.updateMatrixWorld(true);
            G.update(1, s);
            if (G.up) { const f = S47.scene.getObjectByName('glimpse').position, c = S47.camera.getWorldPosition(f.clone());
              if (!seen.length || seen[seen.length - 1] !== G.up + i) seen.push(G.up + '@' + i);
              if (f.distanceTo(c) < 40) near++; }
          }
          return [G.count, near, seen.slice(0, 10)]; })()''')
        check(1 <= res[0] <= 8, f'left to itself over forty minutes of the night: some, not many ({res[0]}: {res[2][:6]})')
        check(res[1] == 0, 'never nearer than 40 m')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/glimpses.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
