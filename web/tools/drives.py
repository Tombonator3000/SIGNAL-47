# The drives are driven the whole way (Tom, 5 October: no cuts). From the truck on SARO's
# pad: down the ramp, the service road, south on the highway, into the road area, the
# survey track, into STATION 01's area and onto the gravel by the gate; then back to the
# pad. The truck drives itself (S47.world.autopilot). Just before and just after each change
# of area the picture is saved, and the change must look like any other step of the drive:
# the difference across it is compared with the difference between the two frames before.
# Usage: python3 tools/drives.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, sys, os, math
from playwright.async_api import async_playwright
from PIL import Image, ImageChops, ImageStat

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

def diff(a, b):
    A = Image.open(a).convert('L'); B = Image.open(b).convert('L')
    return sum(ImageStat.Stat(ImageChops.difference(A, B)).mean)

SOON = """((t) => { const w = S47.world, L = w.legs[w.leg], D = L.drive, h = D.heading, v = D.speed;
  return !!w.mods.legs.exitFrom(w.leg, D.pos.x - Math.sin(h) * v * t, D.pos.z - Math.cos(h) * v * t, h); })"""
STATE = """(() => { const w = S47.world, L = w.leg && w.legs[w.leg], D = L && L.drive;
  return { area: w.area, leg: w.leg, driving: w.driving, at: w.truckAt, x: D ? D.pos.x : null, z: D ? D.pos.z : null, mph: D ? D.mph : 0,
    kind: D ? D.surface.kind : null, clock: S47.game.clock }; })()"""

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        ctx = await b.new_context(viewport={'width': W, 'height': H})
        pg = await ctx.new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png'); return f'{OUT}/{name}.png'
        state = lambda: ev(STATE)

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1.0)")
        await ev("S47.setPicture('off')")
        check(await ev("S47.scene.getObjectByName('saroCorridor') !== undefined"), "SARO draws the road's own stretch south of the site")
        check(await ev("S47.scene.getObjectByName('horizon') !== undefined"), 'the shared skyline is in the scene')

        async def drive(what, to, start, budget=420):
            """Drive with the autopilot until the truck is parked in `to`; shots round each change."""
            await ev(start)
            for _ in range(60):
                await tick(0.25)
                if await ev("S47.world.driving"): break
            st = await state()
            check(st['driving'] and st['leg'] == st['area'], f"{what}: in the truck, the engine running ({st['leg']})")
            await tick(0.5); await shot(f'{what}_00_start')
            await ev("S47.world.autopilot = 12")
            changes, n, t0 = [], 0, st['clock']
            prev_leg = st['leg']
            for _ in range(int(budget / 0.5)):
                if not await ev("S47.world.driving"): break
                if await ev(f"{SOON}(0.6)"):
                    # step a frame at a time across the change, a picture of each
                    pics = []
                    for k in range(40):
                        await tick(1 / 30)
                        pics.append(await shot(f'{what}_c{len(changes) + 1}_{k:02d}'))
                        s2 = await state()
                        if s2['leg'] != prev_leg: break
                    s2 = await state()
                    if s2['leg'] != prev_leg and len(pics) >= 3:
                        d0, d1 = diff(pics[-3], pics[-2]), diff(pics[-2], pics[-1])
                        changes.append((prev_leg, s2['leg'], d0, d1))
                        check(d1 <= max(2.5 * d0, 3.0), f"{what}: {prev_leg} to {s2['leg']} at {s2['mph']:.0f} mph looks like any other frame (difference {d1:.2f}, the frame before {d0:.2f})")
                        prev_leg = s2['leg']
                    continue
                await tick(0.5)
                n += 1
                if n % 20 == 0:
                    s2 = await state(); await shot(f'{what}_{n:03d}_{s2["leg"]}')
            st = await state()
            mins = (st['clock'] - t0) / 60
            return st, changes, mins

        # ---------- out: SARO to STATION 01 ----------
        st, ch, mins = await drive('out', 'station01', "S47.world.driveOut()")
        check([c[:2] for c in ch] == [('saro', 'road'), ('road', 'station01')], f"out: through SARO, the road and the station's area in order {[c[:2] for c in ch]}")
        await tick(1.0)
        st = await state()
        check(st['area'] == 'station01' and not st['driving'] and st['at'] == 'station01', f'out: parked by the gate, out of the truck ({mins:.1f} game minutes)')
        check(await ev("S47.ch3.s.stage") == 'station', 'out: the chapter has arrived at STATION 01')
        check(await ev("S47.world.parked.group.visible && S47.world.parked.group.position.z - 8000 > 26 && S47.world.parked.group.position.z - 8000 < 40"), 'out: the truck stands on the pad')
        await shot('out_99_arrived')

        # ---------- back: STATION 01 to SARO ----------
        st, ch, mins = await drive('back', 'saro', "S47.world.driveBack()")
        check([c[:2] for c in ch] == [('station01', 'road'), ('road', 'saro')], f"back: through the road and SARO in order {[c[:2] for c in ch]}")
        await tick(1.0)
        st = await state()
        check(st['area'] == 'saro' and not st['driving'] and st['at'] == 'saro', f'back: parked on the pad, out of the truck ({mins:.1f} game minutes)')
        check(await ev("S47.world.saroTruck.group.visible && Math.abs(S47.world.saroTruck.group.position.x - 9.8) < 3.5 && S47.world.saroTruck.group.position.z < 13"), 'back: the truck stands on its pad again')
        await shot('back_99_arrived')

        bad = [e for e in errs if 'favicon' not in e]
        print('\n'.join(bad[:12]) if bad else 'no console errors/warnings')
        check(not bad, 'no console errors or warnings')
        await b.close()
    print(f'{sum(results)} of {len(results)} PASS')
    sys.exit(0 if all(results) else 1)

asyncio.run(main())
