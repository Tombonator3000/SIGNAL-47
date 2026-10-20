# HENDELSER.md, event 7 (story/lineLights.ts): driving south between SARO and the motel at night,
# three lights low over the desert to the west keep pace with the truck, then turn off along B
# and go out; once in a night, in the notebook and on the map, kept after a save and Continue.
# The truck is stood in by a point moved south by hand (the drive itself is tools/drives.py).
# Also the map with marks on A and B and a time away (ui/areaMap.ts), saved as a picture.
# Usage: python3 tools/linelights.py OUTDIR [WxH]
import asyncio, sys, json, os, base64
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
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(240000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.evaluate("localStorage.clear()"); await pg.reload()
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0); S47.game.clock = 2 * 3600 + 30 * 60")
        drive = '''((z0, z1, secs) => { const L = S47.lineLights, v = new S47.camera.position.constructor(-25.8, 0, z0), out = [];
          for (let i = 0; i <= secs * 10; i++) { v.z = z0 + (z1 - z0) * i / (secs * 10);
            L.update(0.1, { ok: true, truck: v, clock: S47.game.clock + i / 10, ground: (x, z) => S47.world.grounds.floorAt(x, z) });
            const p = S47.scene.getObjectByName('lineLights'), a = p.geometry.getAttribute('position');
            out.push([L.state, p.visible, a.getX(0), a.getZ(0), v.z]); }
          return out; })'''
        n0 = len(await ev("S47.game.notes"))
        north = await ev(f"{drive}(240, 100, 8)")
        check(all(r[0] == 'none' for r in north), 'driving north: nothing')
        run = await ev(f"{drive}(100, 560, 26)")
        pace = [r for r in run if r[0] == 'pace']
        off = [r for r in run if r[0] == 'off']
        check(len(pace) > 50 and all(r[1] for r in pace), f'driving south past z 120 they come, and stay a while ({len(pace) / 10:.1f} s)')
        gap = [r[3] - r[4] for r in pace]
        check(max(gap) - min(gap) < 2 and all(r[2] < -200 for r in pace), f'abreast, keeping pace, some 200 m to the west (lead {min(gap):.1f} to {max(gap):.1f} m)')
        # B (story/lines.ts) runs from SARO's turn-off south-west to STATION 01, 210 to 270 m west of the highway here
        dB = lambda r: ev(f"S47.lines.distTo('B', ...S47.lines.toMap('saro', {r[2]}, {r[3]}))")
        d0, d1 = await dB(off[0]), await dB(off[-1])
        check(len(off) > 20 and off[-1][2] < off[0][2] - 100 and off[-1][3] > off[0][3] and d1 < d0 * 0.5,
              f'then off along B, south-west towards STATION 01, and gone (from {d0:.0f} m off B to {d1:.0f})')
        nn = (await ev("S47.game.notes"))[n0:]
        check(len(nn) == 1 and 'Three lights' in nn[0] and await ev("S47.townSeen.has('ev:lights')"), f'in the notebook ({nn[:1]})')
        again = await ev(f"{drive}(100, 400, 15)")
        check(all(r[0] == 'none' for r in again), 'once in a night')
        # the map, with a time away added by hand on B and the town's marks on A
        await ev("S47.abduction().data.spots.push({ scene: 'saro', take: [-24, 0, 300], back: [-30, 0, 520], clock: 2 * 3600 + 40 * 60 }); for (const k of ['phone@9000', 'lamps@11000', 'cutout@9500']) S47.townSeen.add('mark:' + k)")
        mk = await ev("S47.mapMarks().map((m) => [m.kind, m.line, m.at ? m.at.spot : null])")
        check(['away', 'B', 'highway'] in mk and ['lights', 'B', 'highway'] in mk and ['phone', 'A', None] in mk, f'the marks know their line and place ({mk})')
        m = await ev("S47.game.notebookExtras().map")
        open(f'{OUT}/l01_map.png', 'wb').write(base64.b64decode(m.split(',', 1)[1]))
        await ev("S47.abduction().data.spots.pop()")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600); await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        after = await ev(f"{drive}(100, 400, 15)")
        check(await ev("S47.lineLights.done") and all(r[0] == 'none' for r in after), 'after Continue they do not come again')
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/linelights.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
