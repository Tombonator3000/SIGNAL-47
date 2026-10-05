# Pictures of the truck, for looking at it: SARO 07 on its pad from six sides (night, the
# yard's lights), the cab from the driver's seat on the highway at night and on the old
# road at dawn, and the dash close up. No checks, only pictures (and console errors).
# Usage: python3 tools/truckshots.py OUTDIR [WxH]
import asyncio, sys, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        ev = pg.evaluate
        await pg.click('button[data-a=start]')
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1.5)")
        await ev("S47.game.d.ui.close(); document.querySelectorAll('.toast').forEach(t => t.remove())")
        # outside: a camera that stays where it is put (the player is moved far away)
        await ev("S47.player.place(-30, -30, 0)")
        tr = await ev("(() => { const g = S47.world.saroTruck.group; g.updateMatrixWorld(); return { x: g.position.x, y: g.position.y, z: g.position.z, h: g.rotation.y }; })()")
        async def view(name, dx, dy, dz, ly=0.9, fov=50):
            # dx, dz in the truck's own frame (x right, z back)
            c, s = math.cos(tr['h']), math.sin(tr['h'])
            wx, wz = tr['x'] + dx * c + dz * s, tr['z'] - dx * s + dz * c
            await ev(f"""(() => {{ const cam = S47.camera; S47.hold = true;
              cam.fov = {fov}; cam.updateProjectionMatrix(); cam.position.set({wx}, {tr['y'] + dy}, {wz});
              cam.lookAt({tr['x']}, {tr['y'] + ly}, {tr['z']}); S47.renderer.render(S47.scene, cam); }})()""")
            # the game's own frame draws with the VHS pass: put the camera, then draw one frame without moving the player
            await ev(f"""(() => {{ const cam = S47.camera; const p = cam.position.clone(), q = cam.quaternion.clone();
              const step = S47.player.update.bind(S47.player); S47.player.update = () => {{}};
              S47.tick(0.05); cam.position.copy(p); cam.quaternion.copy(q); S47.player.update = step; }})()""")
            await pg.screenshot(path=f'{OUT}/{name}.png')
        await view('t01_front34', -4.5, 1.4, -5.5)
        await view('t02_side', -7.0, 1.0, 0.0)
        await view('t03_rear34', 6.5, 2.2, 3.5)
        await view('t04_front', 0.0, 1.0, -7.0)
        await view('t05_low', -3.2, 0.35, -3.8, 0.7)
        await view('t06_high', 3.5, 4.5, 3.0)
        # inside: a free drive on the highway at night
        await ev("S47.world.freeDrive('road')")
        await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'road' && S47.world.driving")
        await ev("S47.hold = true; document.querySelectorAll('.toast').forEach(t => t.remove())")
        await ev("S47.world.testInput = { steer: 0, throttle: 0.6 }"); await ev("S47.tick(3)")
        await pg.screenshot(path=f'{OUT}/t10_cab_night.png')
        await ev("S47.world.testInput = { steer: 0.6, throttle: 0.3 }"); await ev("S47.tick(1)")
        await pg.screenshot(path=f'{OUT}/t11_cab_turn.png')
        await ev("S47.world.testInput = { steer: 0, throttle: -1 }"); await ev("S47.tick(3)")
        for name, yaw, pitch in [('t12_cab_left', 0.9, -0.15), ('t13_cab_right', -1.1, -0.25), ('t14_cab_down', 0.0, -0.75)]:
            await ev(f"(() => {{ const d = S47.world['drive']; d['yaw'] = {yaw}; d['pitch'] = {pitch}; d['idle'] = 0; S47.tick(0.05); }})()")
            await pg.screenshot(path=f'{OUT}/{name}.png')
        # the old road at dawn
        await ev("S47.game.clock = 5 * 3600 + 24 * 60")
        await ev("S47.world.freeDrive('oldroad')")
        await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'roswell' && S47.world.driving")
        await ev("S47.hold = true; document.querySelectorAll('.toast').forEach(t => t.remove())")
        await ev("S47.world.testInput = { steer: 0, throttle: 0.5 }"); await ev("S47.tick(2)")
        await pg.screenshot(path=f'{OUT}/t20_cab_dawn.png')
        await ev("(() => { const d = S47.world.oldDrive; d['yaw'] = 0.75; d['pitch'] = -0.2; d['idle'] = 0; S47.tick(0.05); })()")
        await pg.screenshot(path=f'{OUT}/t21_cab_dawn_left.png')
        await ev("(() => { const d = S47.world.oldDrive; d['yaw'] = -0.9; d['pitch'] = -0.35; d['idle'] = 0; S47.tick(0.05); })()")
        await pg.screenshot(path=f'{OUT}/t22_cab_dawn_right.png')
        # and the truck from the road at dawn
        await ev("S47.world.testInput = { steer: 0, throttle: -1 }"); await ev("S47.tick(3)")
        await ev("""(() => { const g = S47.world.oldTruck.group, cam = S47.camera; S47.world.oldControl = { camera: () => {} };
          const h = g.rotation.y, c = Math.cos(h), s = Math.sin(h);
          cam.position.set(g.position.x - 4.5 * c - 6 * s, g.position.y + 1.5, g.position.z + 4.5 * s - 6 * c); cam.lookAt(g.position.x, g.position.y + 0.9, g.position.z); S47.tick(0.05); })()""")
        await pg.screenshot(path=f'{OUT}/t23_dawn_outside.png')
        bad = [e for e in errs if 'GPU stall' not in e and 'Automatic fallback to software WebGL' not in e]
        print('\n'.join(bad) if bad else 'no console errors/warnings')
        await b.close()

asyncio.run(main())
