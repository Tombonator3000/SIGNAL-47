# Caliche (world/Town.ts, story/caliche.ts; HENDELSER.md section 9, step 1): the town is built
# with the diner and stands in the road area; the player walks its street with the real
# movement code from the diner's lot to the last house and across to the east side, and the
# ground underfoot has no step a foot would catch on; the buildings and posts hold the player
# and the truck; the night keeps its hours (the deputy out at 02:45 and back at 04:30, the
# Night Line off the air at four, the Sentinel's bundle at 04:45, the street lamps going off one
# by one from 05:05, the blinker over the crossing); the windows and signs answer, three of
# them go into the notebook once, also after a save and Continue; the truck can be left
# anywhere on the street; the place and the footsteps know the town; the draw calls stay in
# budget on High and Low. Screenshots of the street at night and at dawn.
# Usage: python3 tools/town.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000   # the road area's origin (World.ROAD_ORIGIN): the town is built in its terms
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
        async def tick(s, fps=None): await ev(f"S47.tick({s}{', ' + str(fps) if fps else ''})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def calls(): return await ev("S47.renderer.info.render.calls")
        async def aimed(): return await ev("(()=>{const it=S47.game.d.inter.update(S47.camera); return it ? it.id : null})()")
        async def label(i): return await ev(f"(() => {{ const it = S47.game.d.inter.get('{i}'); return it ? it.label() : 'MISSING'; }})()")
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use(); 0")
        notes = lambda: ev("S47.game.notes.slice()")
        async def walk(points):
            return await ev(f'''(()=>{{const pts={json.dumps(points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.15) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 9000) return [false, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        async def face(x, y, z):
            await ev(f"(()=>{{const p=S47.player; p.yaw=Math.atan2(-({x}-p.pos.x), -({z}-p.pos.z)); p.pitch=Math.atan2({y}-(p.floorY+p.eye), Math.hypot({x}-p.pos.x,{z}-p.pos.z)); S47.tick(0.05);}})()")
        async def proxy(k): return await ev(f"(()=>{{const v=S47.world.town.proxies['{k}'].getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]}})()")
        async def at(x, z, yaw=math.pi, pitch=0.0, clock=None):
            c = f"S47.game.clock = {clock}; " if clock is not None else ''
            await ev(f"{c}S47.world.enter('diner'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.tick(0.4)")
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0)")

        # ---------- built with the diner, in the road area ----------
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(120):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        st = await ev("S47.world.town.stats")
        check(st['lamps'] == 19 and st['quads'] >= 30 and st['glows'] >= 30, f"Caliche is built with the diner: its own lamps and the headlights' three ({st})")
        await at(X - 1.4, 2040.3, math.pi, 0, 3 * 3600)
        check(await ev("S47.world.area === 'diner' && S47.world.town.group.visible && S47.world.diner.group.visible && S47.world.road.group.visible"), 'in the diner\'s area the town, the diner and the road are all shown')
        await ev("S47.world.enter('saro'); S47.tick(0.1)")
        check(not await ev("S47.world.town.group.visible"), 'and the town is hidden at SARO')
        n_obs = await ev("(() => { const o = S47.world.road.obstacles; return S47.world.town.obstacles.filter((b) => o.includes(b)).length + '/' + S47.world.town.obstacles.length; })()")
        a, t = (int(v) for v in n_obs.split('/'))
        check(a == t and t > 40, f'its buildings and posts hold the truck: the road has every one of them ({n_obs})')

        # ---------- walking the street ----------
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.3)")
        route = [[X - 10, 2021], [X - 13, 2030], [X - 13, 2096], [X - 13.8, 2180], [X - 1.2, 2250], [X + 17.6, 2220], [X + 17.8, 2120], [X + 17.2, 2050], [X + 6, 2000], [X - 6, 2010]]
        r = await walk(route)
        check(r[0], f'from the diner\'s lot down the west side to the last house, across, up the east side to 1420 AM and back to the lot ({r[1]:.1f}, {r[2]:.1f})')
        steps = await ev('''(() => { const f = S47.world.town.floorAt; let worst = [0, 0, 0];
          const line = (x0, z0, x1, z1) => { const n = Math.ceil(Math.hypot(x1 - x0, z1 - z0) / 0.15); let prev = f(x0, z0);
            for (let i = 1; i <= n; i++) { const x = x0 + (x1 - x0) * i / n, z = z0 + (z1 - z0) * i / n, y = f(x, z); if (Math.abs(y - prev) > worst[0]) worst = [Math.abs(y - prev), x, z]; prev = y; } };
          const X = 8000;
          // where it can be walked (Town.zones): the crossing in front of the diner to the verge
          // across the highway, the street to the building fronts
          for (let x = -17.5; x <= 19.4; x += 0.75) line(X + x, x > -3.3 && x < 16.4 ? 1978.2 : 2015.2, X + x, 2255.8);   // down the street, every 75 cm across it
          for (let z = 1980; z <= 2255; z += 2.5) line(X + (z < 2015.2 ? -3.3 : -17.5), z, X + (z < 2015.2 ? 16.3 : 19.4), z);   // across it, every 2.5 m
          for (let x = -16; x <= -3.4; x += 0.5) line(X + x, 2012, X + x, 2022);               // off the diner's lot
          return worst; })()''')
        check(steps[0] <= 0.12, f'nowhere on the street a step over 12 cm between two points 15 cm apart (worst {steps[0]:.3f} m at {steps[1]:.1f}, {steps[2]:.1f})')
        lot = await ev(f"[S47.world.town.floorAt({X - 10}, 2000), S47.world.diner.group.position.y]")
        check(abs(lot[0] - lot[1]) < 1e-6, f'the diner\'s lot is as flat as it was ({lot[0]:.3f} against {lot[1]:.3f})')
        # the gas station office holds the player at its front
        await ev(f"S47.player.place({X - 14.5}, 2072, Math.PI / 2); S47.tick(0.1)")
        r = await walk([[X - 22, 2072]])
        px = await ev("S47.player.pos.x")
        check(not r[0] and px > X - 17.01, f'the gas station office is a wall, not a door left open ({px - X:.2f})')

        # ---------- the night's hours ----------
        async def hours(c):
            await ev(f"S47.game.clock = {c}; S47.tick(0.2)")
            return await ev('''(() => { const t = S47.world.town, f = t.flood, w = (k) => f.pos[t['slots'][k]].w;
              return { deputy: t.deputyIn, car: t['car'].visible, carCol: t['carCol'].minX < 1e8, sheriff: w('sheriffWin'), studio: w('studio'), paper: t.paperIn,
                lamps: t['lamps'].map((l) => f.pos[l.slot].w > 0), glows: t['lamps'].map((l) => t['glow'].points.geometry.getAttribute('aCol').getX(l.glow[0]) > 0) }; })()''')
        h = await hours(2 * 3600 + 30 * 60)
        check(h['deputy'] and h['car'] and h['carCol'] and h['sheriff'] > 0, f"02:30: the deputy at his desk, his car out front and in the way, the window lit ({h['sheriff']})")
        h = await hours(3 * 3600)
        check(not h['deputy'] and not h['car'] and not h['carCol'] and h['sheriff'] == 0, '03:00: out on patrol; the office dark, the car gone, nothing left in the way')
        check(all(h['lamps']) and all(h['glows']) and not h['paper'] and h['studio'] == 4, '03:00: every street lamp on, no paper yet, the Night Line on the air')
        h = await hours(4 * 3600 + 5 * 60)
        check(h['studio'] < 2 and not h['deputy'], f"04:05: the Night Line has signed off, the studio dim ({h['studio']}); the deputy still out")
        h = await hours(4 * 3600 + 35 * 60)
        check(h['deputy'] and h['car'] and not h['paper'], '04:35: the deputy is back at his desk, no paper yet')
        h = await hours(4 * 3600 + 50 * 60)
        check(h['paper'] and all(h['lamps']), '04:50: the Sentinel\'s bundle lies at the gas station door; the lamps still on')
        h = await hours(5 * 3600 + 12 * 60)
        on = sum(h['lamps'])
        check(0 < on < 7 and h['lamps'] == h['glows'], f'05:12: some lamps have gone off and some not ({on} of 7 on), the glow with each')
        h = await hours(5 * 3600 + 40 * 60)
        check(not any(h['lamps']) and not any(h['glows']), '05:40: every street lamp off')
        await ev("S47.game.clock = 3 * 3600; S47.tick(0.2)")
        seen = await ev('''(() => { const t = S47.world.town, f = t.flood, i = t['slots'].blink, out = new Set();
          for (let k = 0; k < 30; k++) { S47.tick(0.1); out.add(f.pos[i].w > 0); } return [...out]; })()''')
        check(len(seen) == 2, f'the light over the crossing blinks ({seen})')

        # ---------- windows and signs ----------
        ids = await ev("S47.game.d.inter.items.filter((i) => i.id.startsWith('town:')).map((i) => i.id)")
        check(len(ids) >= 22, f'every window, door and sign has an answer ({len(ids)})')
        await ev("S47.game.clock = 2 * 3600 + 30 * 60; S47.tick(0.2)")
        aims = {}
        for k, stand in [('gasDoor', [X - 14.9, 2067.4]), ('gasWindow', [X - 14.6, 2073]), ('payphone', [X - 13.6, 2064.6]), ('pumps', [X - 7.4, 2055]),
                         ('sheriffWindow', [X - 13.9, 2107.3]), ('patrolCar', [X - 9.4, 2104]), ('laundromat', [X - 13, 2129]), ('bench', [X - 13.2, 2133.4]),
                         ('cinema', [X - 13.2, 2160]), ('feed', [X - 9.8, 2201]), ('studio', [X + 17.6, 2037.2]), ('souvenir', [X + 18.9, 2062.0]),
                         ('letters', [X + 16.6, 2087.3]), ('churchDoor', [X + 19.2, 2095.5]), ('tv1', [X + 18.9, 2132.4]), ('kitchen', [X - 17.2, 2241])]:
            await ev(f"S47.player.place({stand[0]}, {stand[1]}, 0); S47.tick(0.05)")
            v = await proxy(k)
            await face(v[0], v[1], v[2])
            aims[k] = await aimed()
        bad = {k: v for k, v in aims.items() if v != 'town:' + k}
        check(not bad, f'each is aimed at from the street a player can stand on ({len(aims) - len(bad)} of {len(aims)}; {bad})')
        n0 = len(await notes())
        await use('town:sheriffWindow'); await tick(0.2)
        n1 = await notes()
        check(len(n1) == n0 + 1 and 'pins' in n1[-1], 'the sheriff\'s map goes into the notebook')
        await use('town:sheriffWindow'); await use('town:gasDoor'); await tick(0.2)
        n2 = await notes()
        check(len(n2) == n0 + 2 and 'NIGHT LINE' in n2[-1], 'once each: the map not again, the flyer on the gas station door added')
        check(await label('town:paper') is None, 'no newspapers to look at before they come')
        await ev("S47.game.clock = 4 * 3600 + 50 * 60; S47.tick(0.2)")
        check(await label('town:paper') == 'Newspapers', 'and at 04:50 there they are')
        await ev("S47.world.enter('saro'); S47.tick(0.1)")
        check(await label('town:gasDoor') is None, 'none of it answers from SARO')
        await ev("S47.world.enter('diner'); S47.tick(0.1)")

        # ---------- place, footsteps, map ----------
        await ev(f"S47.player.place({X - 12}, 2064, 0); S47.tick(0.1)")
        pl1 = await ev("S47.placeName()"); sf1 = await ev("S47.surface()")
        await ev(f"S47.player.place({X - 15.6}, 2150, 0); S47.tick(0.1)")
        sf2 = await ev("S47.surface()")
        await ev(f"S47.player.place({X - 10}, 2150, 0); S47.tick(0.1)")
        sf3 = await ev("S47.surface()")
        await ev("S47.world.placeAtDiner('inside'); S47.tick(0.1)")
        pl2 = await ev("S47.placeName()")
        check(pl1 == 'Caliche, the gas station' and pl2 == 'Mesa Diner' and (sf1, sf2, sf3) == ('concrete', 'concrete', 'dirt'),
              f'the corner of the screen and the footsteps know the town ({pl1}; {pl2}; {sf1}, {sf2}, {sf3})')
        check(await ev("S47.mapAt().spot") == 'diner', 'the notebook map puts the player at the diner and the town')

        # ---------- the truck left on the street ----------
        await ev("S47.world.placeTruck('diner'); S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.2)")
        await use('dinerTruck')
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev(f"(() => {{ const w = S47.world, L = w.legs[w.leg]; L.drive.place(new S47.three.Vector3({X + 2.2}, L.area.height({X + 2.2}, 2140), 2140), L.drive.heading); }})()")
        await tick(1.0)
        can = await ev("S47.world.canGetOut()")
        await ev("S47.world.getOutHere()")
        for _ in range(12): await tick(0.25)
        stt = await ev("[S47.world.area, S47.world.driving, S47.world.truckAt, S47.player.walkable(S47.player.pos.x, S47.player.pos.z), S47.player.pos.z]")
        tz = await ev("S47.world.truck.group.position.z")
        check(can and stt[0] == 'diner' and not stt[1] and stt[2] == 'diner' and stt[3] and abs(tz - 2140) < 3 and abs(stt[4] - tz) < 4,
              f'stopped on the street in town, the truck can be left there and the player gets out beside it ({stt}, truck at z {tz:.1f})')

        # ---------- save and Continue: the notebook keeps what it has from the town ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        for _ in range(60):
            if await ev("!!S47.world.town"): break
            await pg.wait_for_timeout(200)
        n3 = len(await notes())
        await ev("S47.world.enter('diner'); S47.tick(0.1)")
        await use('town:sheriffWindow'); await use('town:gasDoor'); await tick(0.2)
        check(await ev("S47.world.area") == 'diner' and len(await notes()) == n3, f'after Continue the map and the flyer are not written down again ({n3} notes)')

        # ---------- the look, and the draw calls on High and Low ----------
        views = [('t01_street_south', X - 1.3, 2012.4, math.pi, -0.02, 3 * 3600), ('t02_gas', X - 2.2, 2047.6, math.pi * 0.75, 0.02, 3 * 3600),
                 ('t03_east', X + 4.3, 2050.6, math.pi * 0.62, 0.05, 3 * 3600), ('t04_mid', X + 1.3, 2110.4, math.pi, 0, 3 * 3600),
                 ('t05_sheriff', X - 9.3, 2104.6, math.pi / 2, 0.02, 2 * 3600 + 30 * 60), ('t06_north', X + 2.3, 2250.4, 0, 0, 3 * 3600),
                 ('t07_dawn', X + 2.3, 2240.6, 0, 0, 5 * 3600 + 14 * 60)]
        most = {}
        for q in ['high', 'low']:
            await ev(f"S47.setQuality('{q}')")
            for name, x, z, yaw, pitch, c in views:
                await at(x, z, yaw, pitch, c)
                await pg.wait_for_timeout(200)
                n = await calls(); most[f'{q}:{name}'] = n
                if q == 'high': await shot(name)
        await ev("S47.setQuality('high')")
        check(max(most.values()) <= 300, f'draw calls in town within 300 on High and Low (most {max(most.values())}: {most})')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks, 'calls': most}, open(f'{OUT}/town.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
