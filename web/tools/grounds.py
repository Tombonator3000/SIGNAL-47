# SARO's grounds (world/Grounds.ts) and the sky (world/Sky.ts). Walks round the house with
# the real movement code: out of the east door, round the truck on its pad, down the ramp,
# along the service road and the footpath to the fire exit, up the west side, along the
# windows side and up the steps to the east walk. Then round the east wing to the fence.
# Checks the height under the player on the way, that the slab edges are walls from both
# sides (only the ramps and the steps lead up and down), that the things standing about
# block the way, and that the sky's painted stars are gone and the points are there.
# Usage: python3 tools/grounds.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright
OUT = sys.argv[1]; W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

async def main():
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'])
        pg = await b.new_page(viewport={'width': W, 'height': H}); pg.set_default_timeout(300000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(300)
        ev = pg.evaluate
        await ev("S47.hold = true")
        await ev("S47.jump('chapter1')"); await ev("S47.tick(1.5)")
        await ev("S47.doors.set('east', true, true)"); await ev("S47.tick(0.2)")

        async def place(x, z, yaw=0.0, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.05)")
        # walk from point to point with Player.update; false if a point cannot be reached
        async def walk(points, start=True):
            if start: await place(points[0][0], points[0][1])
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:] if start else points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 1500) return [false, x, z, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        # push towards a point for a while and say where the player ended up
        async def push(x0, z0, x1, z1, steps=240):
            await place(x0, z0)
            return await ev(f'''(()=>{{const p=S47.player;
              for (let n=0; n<{steps}; n++) {{ p.yaw = Math.atan2(-({x1}-p.pos.x), -({z1}-p.pos.z)); p.update(1/30, 0, -1, false); }}
              return [p.pos.x, p.pos.z, p.floorY]; }})()''')
        async def floor_at(x, z): return await ev(f"S47.world.grounds.floorAt({x},{z})")
        async def shot(name, x, z, yaw, pitch=-0.05):
            await place(x, z, yaw, pitch); await ev("S47.tick(0.5)")
            await pg.screenshot(path=f'{OUT}/{name}.png')
            return await ev("S47.renderer.info.render.calls")

        # ---------- the sky ----------
        sky = await ev("""(()=>{ const s = S47.sky; let pts = null, dome = null;
          s.group.traverse(o => { if (o.isPoints) pts = o; if (o.isMesh && o.material.uniforms && o.material.uniforms.uPanorama) dome = o; });
          return { n: pts && pts.geometry.attributes.position.count, ms: s.filterMs, tex: dome && dome.material.uniforms.uPanorama.value.name,
                   big: pts && Array.from(pts.geometry.attributes.aSize.array).filter(v => v > 1).length }; })()""")
        check(sky['tex'] == 'art/sky (stars removed)', 'the sky shows the painted Milky Way with its painted stars taken out')
        check(sky['n'] == 14000 and 50 <= sky['big'] <= 400, f"{sky['n']} sharp stars, {sky['big']} of them larger than one pixel")
        print(f"taking the stars out took {sky['ms']:.0f} ms here (software renderer, UNVERIFIED on a phone)")

        # ---------- heights ----------
        heights = [((0, 0), 0, 'the control room'), ((9.5, -10), 0, 'the east walk'), ((9.8, 8), 0, 'the truck pad'),
                   ((9.8, 14.5), -0.3, 'half way down the truck ramp'), ((9.8, 18), -0.6, 'the foot of the ramp'),
                   ((-4, 24.5), -0.6, 'the service road'), ((-9, -10), -0.6, 'the west side'), ((3, -12), -0.6, 'the windows side'),
                   ((7.55, -12), -0.27, 'the middle of the steps'), ((-9.1, 5.7), -0.3, 'half way up the fire exit ramp'), ((-35, 30), 0, "the motel's lot"),
                   ((-27.0, 6), -0.28, "the motel's driveway"), ((30, 20), -0.6, 'south of the east wing')]
        bad = []
        for (x, z), want, what in heights:
            y = await floor_at(x, z)
            if abs(y - want) > 0.03: bad.append(f'{what} {y:.2f} (want {want})')
        check(not bad, 'the ground is at the right height everywhere: ' + ('; '.join(bad) if bad else f'{len(heights)} places'))

        # ---------- round the house ----------
        loop = [(5.0, 1.6), (7.2, 1.6), (10.2, 2.0), (10.6, 4.2),       # out of the east door, through the gap to the pad
                (7.35, 4.6), (7.35, 11.3), (11.8, 11.6), (11.8, 4.6), (10.6, 4.4), (11.8, 11.8),   # round the truck
                (9.8, 12.6), (9.8, 17.6),                                 # down the ramp
                (9.8, 20.5), (8.6, 23.3), (4.0, 24.5), (-6.0, 24.5),      # the service road, round the bend
                (-1.25, 21.0), (-1.25, 14.2), (-6.1, 13.5),               # between the propane tank and the dumpster (-8.1, 12.4), (-9.6, 8.5), (-11.0, 5.7),   # the footpath to the fire exit
                (-11.4, 4.2), (-9.0, 0.0), (-9.0, -12.0), (5.0, -12.0),   # round the ramp foot, up the west side, along the windows
                (6.5, -12.0), (8.6, -12.0), (9.5, -12.0),                 # up the steps
                (9.5, -1.0), (9.5, 1.5)]                                  # back along the walk to the landing
        r = await walk(loop)
        check(r[0], 'round the house on foot: east door, round the truck, ramp, road, path, fire exit, west side, windows side, steps, walk' + ('' if r[0] else f' (stuck going to {r[1]},{r[2]} at {r[3]:.2f},{r[4]:.2f})'))
        check(abs(await ev("S47.player.floorY")) < 0.05, 'and back on the landing at floor level')
        east = [(9.8, 17.6), (16.5, 18.0), (16.5, 3.0), (19.7, 1.5), (19.7, -10.0), (30.0, -12.0), (42.5, -12.0), (42.5, 20.0), (25.0, 20.0), (9.8, 18.0)]
        r = await walk(east)
        check(r[0], 'round the photo lab and the east wing, between them and along the fence' + ('' if r[0] else f' (stuck going to {r[1]},{r[2]} at {r[3]:.2f},{r[4]:.2f})'))

        # ---------- edges are walls ----------
        walls = [((5.0, -8.0), (5.0, -3.0), lambda x, z: z < -6.1, 'from the windows side into the control room plinth'),
                 ((7.3, 6.0), (4.0, 6.0), lambda x, z: x > 6.8, 'off the west edge of the truck pad'),
                 ((4.0, 12.0), (8.5, 12.0), lambda x, z: x < 6.35, 'from the ground up onto the pad'),
                 ((5.5, 15.0), (9.5, 15.0), lambda x, z: x < 6.35, 'up onto the side of the ramp'),
                 ((-2.0, 13.0), (-2.0, 8.0), lambda x, z: z > 11.35, 'from the ground into the annex'),
                 ((-9.0, 3.5), (-9.0, 7.5), lambda x, z: z < 4.5, 'under the fire exit ramp rail'),
                 ((6.0, -16.0), (9.5, -16.0), lambda x, z: x < 6.5, 'up the side of the east walk where there are no steps'),
                 ((9.5, -16.0), (5.0, -16.0), lambda x, z: x > 8.2, 'off the east walk where there are no steps'),
                 ((10.0, 30.0), (10.0, 40.0), lambda x, z: z < 33.8, 'through the ranch fence'),
                 ((4.3, 5.9 + 0.5), (4.3, 12.0), lambda x, z: z < 6.8, 'through the generator'),
                 ((-10.0, 20.8), (-18.0, 20.8), lambda x, z: x > -14.0, 'through the site sign')]
        bad = []
        for (a, b2, ok, what) in walls:
            x, z, _ = await push(a[0], a[1], b2[0], b2[1])
            if not ok(x, z): bad.append(f'{what} (ended at {x:.2f},{z:.2f})')
        check(not bad, 'the edges and the things standing about stop the player: ' + ('; '.join(bad) if bad else f'{len(walls)} tries'))

        # ---------- names and footsteps ----------
        names = {}
        for key, (x, z) in {'road': (-4, 24.5), 'ground': (3, -12), 'pad': (9.8, 8)}.items():
            await place(x, z); names[key] = [await ev("S47.placeName()"), await ev("S47.surface()")]
        check(names == {'road': ['Service road', 'concrete'], 'ground': ['SARO grounds', 'dirt'], 'pad': ['Truck pad', 'concrete']},
              f'place names for saves and footsteps: {names}')

        # ---------- views ----------
        calls = {}
        calls['ramp'] = await shot('g01_ramp_down_to_road', 9.8, 13.0, math.pi, -0.08)
        calls['road'] = await shot('g02_road_to_highway', 6.0, 24.5, math.pi / 2, -0.04)
        calls['sign'] = await shot('g03_site_sign', -19.0, 23.0, -math.pi / 2 + 0.3, -0.02)
        calls['south'] = await shot('g04_house_from_the_south', -2.0, 30.0, 0.15, 0.0)
        calls['path'] = await shot('g05_path_to_fire_exit', -2.0, 14.2, math.pi / 2 - 0.2, -0.1)
        calls['west'] = await shot('g06_west_side', -9.0, 10.0, 0.0, 0.0)
        calls['windows'] = await shot('g07_windows_side', -8.0, -12.0, -math.pi / 2 + 0.5, 0.05)
        calls['steps'] = await shot('g08_steps_up_to_walk', 4.0, -12.0, -math.pi / 2, -0.05)
        calls['truck'] = await shot('g09_round_the_truck', 7.35, 11.0, 0.0, -0.1)
        calls['pocket'] = await shot('g10_generator', 5.5, 14.0, 0.2, -0.1)
        calls['wing'] = await shot('g11_east_wing', 30.0, 16.0, 0.2, 0.05)
        calls['sky'] = await shot('g12_sky_north', 3.0, -15.0, 0.1, 0.6)
        print('draw calls:', calls)
        check(max(calls.values()) <= 340, f'draw calls at most {max(calls.values())} on High (budget about 300 plus the new grounds)')
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
