# The desert ground (world/flora.ts): the brush, grass, rocks and pebbles and the stains on the
# ground in every outdoor area. Shoots eight fixed views on High (SARO's grounds, the antenna
# field, the highway shoulder to the motel, the motel's lot, STATION 01, Caliche's street, the
# highway from the truck, the old road at dawn), prints the draw calls of each, and checks: the
# draw calls stay in budget (the control room about 300, SARO outside at most 340, every view at
# most SHARED more than before the round, for its five parts together), Low draws no more than
# High and has less ground cover, the cover is there in every area and moves in the wind, it
# stands on no walk the
# story uses (the tracks, the roads, the lots; tiny grass and pebbles only in a way's seam, within
# 0.6 m of its edge), out of the pools of light it is no brighter than the ground behind it and in
# them brighter (each view is shot again without the cover, gN_bare.png, and the two compared
# pixel by pixel), and the player still walks where it matters with the real movement code.
# --shots only shoots and prints (the pictures from before). The flashlight does not flicker here
# (the town's events would make it, at random), so two runs light a view alike.
# Usage: python3 tools/env_ground.py OUTDIR [WxH] [--shots]
import asyncio, sys, json, os, math
from PIL import Image
from playwright.async_api import async_playwright

ARGS = [a for a in sys.argv[1:] if not a.startswith('--')]
OUT = ARGS[0]
W, H = (int(v) for v in (ARGS[1] if len(ARGS) > 1 else '1280x800').split('x'))
SHOTS = '--shots' in sys.argv
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X = 8000   # the road area's origin (World.ROAD_ORIGIN)
# draw calls on High at each view before the ground cover (main at 048fcd1, 19 October, the graphics card)
BEFORE = {'saro': 316, 'field': 35, 'shoulder': 77, 'motel': 92, 'station': 66, 'town': 46, 'road': 151, 'oldroad': 53, 'window': 98}
# and the triangles (the road's old brush is gone: its views can draw fewer)
BEFORE_TRIS = {'saro': 132528, 'field': 98198, 'shoulder': 89712, 'motel': 150328, 'station': 121706, 'town': 310628, 'road': 341652, 'oldroad': 449648}
MORE_TRIS = 90000   # the most the ground cover may add to a view on High (a phone's share of it is Low's)
# What the round's five parts may add to a view on High together, over BEFORE (the same in
# env_ground, env_roads, env_buildings, env_interiors and env_atmosphere). Each part was held to 15
# in its own branch, but they meet in the same views: a part's share of a view is about six (the
# ground cover's five meshes; the life's moths, far lights and lying tumbleweeds; the roads' wear
# and lots; a lot's props, stains and pools; a room's things and dirt), five of them thirty. A view
# over it has a part costing twice its share, or a sixth thing in it: a real overshoot.
SHARED = 30
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

def glow(name):
    """The cover against the ground behind it: the pixels of the ground (under the horizon, over the
    captions) that the cover changes (by more than the picture's grain), by how lit the bare ground
    is there. Out of the pools of light (bare ground 40 or darker): the mean with the cover over the
    mean bare. In them (45 or brighter): the lit straw, the brightest tenth of the pixels the cover
    makes brighter, over the bare ground's median there (its dark feet, the stones and the tufts
    between the eye and the pool are the rest). Returns ((dark ratio, pixels), (lit ratio, pixels))."""
    a = Image.open(f'{OUT}/{name}.png').convert('L'); b = Image.open(f'{OUT}/{name}_bare.png').convert('L')
    w, h = a.size; pa, pb = a.load(), b.load()
    dark, lit = [0, 0, 0], []
    for y in range(int(h * 0.46), int(h * 0.8)):
        for x in range(0, w, 2):
            f, g = pa[x, y], pb[x, y]
            if abs(f - g) < max(6, 0.12 * g): continue
            if g <= 40: dark[0] += f; dark[1] += g; dark[2] += 1
            elif g >= 45 and f > g: lit.append((f, g))
    d = (dark[0] / dark[1] if dark[1] else 0, dark[2])
    if len(lit) < 50: return d, (0, len(lit))
    fs = sorted(f for f, g in lit); gs = sorted(g for f, g in lit)
    return d, (fs[int(len(fs) * 0.9)] / gs[len(gs) // 2], len(lit))

# each view: the area, how to get there, and the camera (on foot: x, z, yaw, pitch; or a drive)
VIEWS = [
    ('saro', 'g1_saro_south', "S47.world.enter('saro'); S47.player.place(18, 31, 0.55); S47.player.pitch = -0.1"),
    ('field', 'g2_antenna_field', "S47.world.enter('saro'); S47.player.place(-1, -68, 0.42); S47.player.pitch = -0.08"),
    ('shoulder', 'g3_shoulder_to_motel', "S47.world.enter('saro'); S47.player.place(-16.2, 300, Math.PI + 0.12); S47.player.pitch = -0.12"),
    ('motel', 'g4_motel_lot', "S47.world.enter('saro'); S47.player.place(-31, 723, 0.85); S47.player.pitch = -0.08"),
    ('station', 'g5_station01', "S47.world.enter('station01'); (() => { const a = S47.world.site.anchors.arrive; S47.player.place(a.x, a.z, a.yaw + 0.5); })(); S47.player.pitch = -0.1"),
    ('town', 'g6_caliche_street', f"S47.world.enter('diner'); S47.player.place({X - 8.5}, 2112, Math.PI - 0.12); S47.player.pitch = -0.06"),
]

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
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def info(): return await ev("({ calls: S47.renderer.info.render.calls, tris: S47.renderer.info.render.triangles })")
        has = await ev("!!(S47.world && S47.world.flora)")
        await ev("S47.hold = true; S47.jump('chapter3'); S47.tick(1.0)")
        await ev("S47.game.clock = 3 * 3600 + 5 * 60")   # a little past three, the dead of the night
        await ev("S47.flashlight.have = true; S47.flashlight.on = true; S47.flashlight.flicker = () => {}; S47.flashlight.flick = 0")
        for area in ('station01', 'road', 'diner'):
            await ev(f"window.__p = S47.world.prepare('{area}')")
            for _ in range(200):
                if await ev(f"(() => {{ const w = S47.world; return '{area}' === 'station01' ? !!w.site : '{area}' === 'road' ? !!w.legs.road : !!w.town; }})()"): break
                await pg.wait_for_timeout(100)
        await ev("S47.setQuality('high')"); await tick(0.2)

        calls, tris, low, cover, glows = {}, {}, {}, {}, {}
        # the cover in the scene now: instances drawn per kind, over every part of the ground shown
        COVER = "S47.world.flora.drawn()"
        async def bare(key, name):
            # the same picture without the cover, to weigh it against the ground behind it
            await ev("S47.world.flora.patches.forEach((f) => { f.group.visible = false; })"); await tick(0.02)
            await shot(name + '_bare')
            await ev("S47.world.flora.patches.forEach((f) => { f.group.visible = true; })"); await tick(0.02)
            glows[key] = glow(name)
            print(f"  {name}: out of the light the cover is {glows[key][0][0]:.2f} times the ground ({glows[key][0][1]} px), in it {glows[key][1][0]:.2f} ({glows[key][1][1]} px)", flush=True)
        async def view(key, name, js, q='high'):
            await ev(js); await tick(0.6)
            if q == 'high': await shot(name)
            i = await info()
            c = await ev(COVER) if has else None
            if q == 'high' and has: await bare(key, name)
            return i, c

        for key, name, js in VIEWS:
            i, c = await view(key, name, js)
            calls[key] = i['calls']; tris[key] = i['tris']; cover[key] = c
            print(f"{name}: {i['calls']} draw calls, {i['tris']} triangles; cover {c}", flush=True)

        # the highway from the truck (south of the motel, the headlights on the verge) and the old
        # road at dawn, mile 3: the camera in the cab of a truck put on the road
        await ev("S47.world.freeDrive('road')")
        for _ in range(120):
            await tick(0.25); await pg.wait_for_timeout(50)
            if await ev("S47.world.driving && S47.world.free === 'road'"): break
        await ev(f"(() => {{ const w = S47.world, L = w.legs.road; L.drive.place(new S47.three.Vector3({X} - 1.8, L.area.height({X} - 1.8, 1180), 1180), Math.PI); w['leg'] = 'road'; w.enter('road'); }})()")
        await tick(0.5); await shot('g7_highway_from_the_truck')
        i = await info(); calls['road'] = i['calls']; tris['road'] = i['tris']; cover['road'] = await ev(COVER) if has else None
        if has: await bare('road', 'g7_highway_from_the_truck')
        print(f"g7_highway_from_the_truck: {i['calls']} draw calls, {i['tris']} triangles; cover {cover['road']}", flush=True)
        await ev("S47.world.endFreeDrive()"); await tick(0.3)

        await ev("S47.jump('chapter6'); S47.tick(0.5)")
        for _ in range(240):
            await tick(0.25); await pg.wait_for_timeout(50)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        await ev("""(() => { const r = S47.world.oldRoad, d = S47.world.oldDrive, s = (3.0 - 8.15) * 1609.34;
          const a = r.lane(s), b = r.lane(s + 4); d.place(a, Math.atan2(-(b.x - a.x), -(b.z - a.z))); S47.world.testInput = { steer: 0, throttle: -1 }; })()""")
        await tick(0.6); await shot('g8_old_road_at_dawn')
        i = await info(); calls['oldroad'] = i['calls']; tris['oldroad'] = i['tris']; cover['oldroad'] = await ev(COVER) if has else None
        if has: await ev("S47.world.testInput = { steer: 0, throttle: 0 }"); await bare('oldroad', 'g8_old_road_at_dawn')
        print(f"g8_old_road_at_dawn: {i['calls']} draw calls, {i['tris']} triangles; cover {cover['oldroad']}", flush=True)
        await ev("S47.world.testInput = null; S47.world.stopDriving(); S47.jump('chapter3'); S47.tick(0.5)")

        # the control room, looking out of the windows at the array
        await ev("S47.world.enter('saro'); S47.player.place(0.05, -2.2, 0); S47.player.pitch = 0.06"); await tick(0.4)
        calls['window'] = (await info())['calls']
        print('draw calls on High:', calls, flush=True)
        if SHOTS:
            print('\n'.join(errs[:20]) or 'no console errors/warnings')
            await b.close(); return

        # ---------- the budgets ----------
        check(calls['window'] <= 300, f"the control room, looking out at the array: {calls['window']} draw calls (about 300 or fewer)")
        check(calls['saro'] <= 340 and calls['field'] <= 340 and calls['shoulder'] <= 340 and calls['motel'] <= 340,
              f"SARO outside at most 340 (the grounds {calls['saro']}, the field {calls['field']}, the shoulder {calls['shoulder']}, the motel {calls['motel']})")
        more = {k: calls[k] - BEFORE[k] for k in BEFORE}
        check(all(v <= SHARED for v in more.values()), f'at most {SHARED} more draw calls in every view than before the round, its five parts together: {more}')
        moret = {k: tris[k] - BEFORE_TRIS[k] for k in BEFORE_TRIS}
        check(all(v <= MORE_TRIS for v in moret.values()), f'and at most {MORE_TRIS} more triangles: {moret}')

        # ---------- in every area, in the wind ----------
        named = {k: (sum(v.values()) if v else 0) for k, v in cover.items()}
        check(all(n > 400 for n in named.values()), f'ground cover in every view: {named}')
        kinds = set(k for v in cover.values() if v for k, n in v.items() if n > 0)
        check({'grass', 'sage', 'creosote', 'stalks', 'succulent', 'stones', 'stain'} <= kinds, f'grass, sagebrush, creosote, dead stalks, yucca and cholla, rocks and pebbles, stains: {sorted(kinds)}')
        w0 = await ev("S47.world.flora.wind.time.value"); await tick(1.0); await pg.wait_for_timeout(300); await tick(0.1); w1 = await ev("S47.world.flora.wind.time.value")
        check(w1 > w0 and await ev("S47.world.flora.wind.strength.value") > 0, f'the wind moves on from picture to picture ({w0:.2f} to {w1:.2f}), one shared uniform')

        # ---------- dark out of the light, bright in it ----------
        # (the views on foot at night: out of the pools no brighter than the ground behind it, in the
        # flashlight's pool and under the lamps clearly brighter; dawn and the cab are left out)
        night = {k: glows[k] for k in ('saro', 'field', 'shoulder', 'motel', 'station', 'town') if k in glows}
        check(all(v[0][0] <= 1.1 for v in night.values() if v[0][1] > 200), 'out of the pools of light the cover is no more than 1.1 times the ground behind it: ' + str({k: round(v[0][0], 2) for k, v in night.items()}))
        check(all(v[1][0] >= 1.5 for v in night.values() if v[1][1] >= 300), 'and lit in them at least 1.5 times (where the light falls on enough of it): ' + str({k: (round(v[1][0], 2), v[1][1]) for k, v in night.items()}))

        # ---------- not on the ways the story uses ----------
        bad, seams = [], {}
        for key, name, js in VIEWS:
            await ev(js); await tick(0.2)
            bad += await ev("S47.world.flora.onWays()")
            seams[key] = await ev("S47.world.flora.seams()")
        check(not bad, 'nothing stands on the tracks, the roads, the paths, the lots or the floors round any view' + ('' if not bad else f': {len(bad)}, {bad[:6]}'))
        check(all(seams[k]['n'] > 20 for k in ('field', 'motel', 'town')), f"tiny grass and pebbles in the seams at the edges of the ways (and how many of them inside a way's edge): {seams}")

        # ---------- Low ----------
        await ev("S47.setQuality('low')")
        for key, name, js in VIEWS:
            i, c = await view(key, name, js, 'low')
            low[key] = (i['calls'], sum(c.values()) if c else 0, i['tris'])
        check(all(low[k][0] <= calls[k] for k in low), f"Low draws no more than High: {({k: (low[k][0], calls[k]) for k in low})}")
        check(all(low[k][1] < named[k] * 0.45 for k in low), f"and less than half the ground cover: {({k: (low[k][1], named[k]) for k in low})}")
        check(all(low[k][2] - BEFORE_TRIS[k] <= MORE_TRIS / 3 for k in low), f"and its triangles at most {MORE_TRIS // 3} more than before: {({k: low[k][2] - BEFORE_TRIS[k] for k in low})}")
        await ev("S47.setQuality('high')"); await tick(0.2)

        # ---------- walking where it matters ----------
        async def walk(area, points):
            await ev(f"S47.world.enter('{area}'); S47.player.place({points[0][0]}, {points[0][1]}, 0); S47.tick(0.05)")
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:])}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.15) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 9000) return [false, x, z, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        await ev("S47.doors.set('gate', true, true); S47.doors.set('exit', true, true)"); await tick(0.2)
        walks = [
            ('saro', 'round the house: the ramp, the service road, the footpath, the west side, the windows side',
             [(9.8, 13.0), (9.8, 17.6), (9.8, 20.5), (8.6, 23.3), (4.0, 24.5), (-6.0, 24.5), (-1.25, 21.0), (-1.25, 14.2), (-6.1, 13.5), (-8.1, 12.4), (-9.6, 8.5), (-11.0, 5.7), (-11.4, 4.2), (-9.0, 0.0), (-9.0, -12.0), (5.0, -12.0)]),
            ('saro', 'through the array gate up the spine to S-11 and along a row to S-01',
             [(-1.0, -17.0), (-1.0, -22.0), (-1.0, -195.0), (-10.0, -195.0), (-1.0, -195.0), (-1.0, -74.0), (-40.0, -74.0), (-48.0, -72.0)]),
            ('saro', 'from the fire exit over the highway and down its shoulder to the motel\'s lot',
             [(-12.0, 5.7), (-21.0, 12.0), (-21.0, 60.0), (-17.5, 400.0), (-18.0, 690.0), (-31.0, 700.0)]),
            ('diner', "Caliche's street from the diner's lot to the last house and back up the east side",
             [(X - 10, 2021), (X - 13, 2030), (X - 13, 2096), (X - 13.8, 2180), (X - 1.2, 2250), (X + 17.6, 2220), (X + 17.8, 2120)]),
        ]
        for area, what, pts in walks:
            r = await walk(area, pts)
            check(r[0], what + ('' if r[0] else f' (stuck going to {r[1]},{r[2]} at {r[3]:.2f},{r[4]:.2f})'))
        # STATION 01: from the truck through the walk gap in the fence to the hut's door
        r = await ev("""(() => { S47.world.enter('station01'); S47.world.setStoryPlaces({ stationOpen: true, oldRoadOpen: false });
          const s = S47.world.site, a = s.anchors.arrive, p = S47.player, g = s.zone.walkThrough, o = S47.world.site.group.position, gx = (g.minX + g.maxX) / 2;
          p.place(a.x, a.z, a.yaw);
          for (const [x, z] of [[gx, g.maxZ - 0.3], [gx, g.minZ + 0.3], ...[[-22.3, 24.6], [-19.8, 19.5], [-15.8, 12.8], [-11.8, 7.8]].map(([u, v]) => [o.x + u, o.z + v])]) { let n = 0;
            while (Math.hypot(p.pos.x - x, p.pos.z - z) > 0.3) { p.yaw = Math.atan2(-(x - p.pos.x), -(z - p.pos.z)); p.update(1 / 30, 0, -1, false); if (++n > 4000) return [false, p.pos.x - o.x, p.pos.z - o.z]; } }
          return [true, p.pos.x - o.x, p.pos.z - o.z]; })()""")
        check(r[0], f'STATION 01: from the truck through the walk gap to the hut\'s door (station {r[1]:.1f}, {r[2]:.1f})')

        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'console clean' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        json.dump({'checks': checks, 'calls': calls, 'tris': tris, 'low': low, 'cover': cover, 'glows': glows, 'seams': seams}, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
