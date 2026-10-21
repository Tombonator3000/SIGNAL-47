# The buildings outside and what stands about them (world/weathering.ts and dressSaro, dressMotel,
# dressDiner, dressTown, dressStation): SARO's house and yard, Sierra Motor Court, Mesa Diner,
# Caliche's street and STATION 01's hut. Takes the same eight views every time on High (the
# before and after pictures), prints the draw calls there and on Low, and checks: every area is
# dressed; the draw calls stay in budget (SARO outside 340 as tools/grounds.py, the control room
# about 300, every view at most SHARED more than before the round for all five of its parts
# together); Low leaves the small things, the stains and the light pools out; nothing stands in
# front of the lettering on the wing; the new props hold the player; and the player still walks
# where it matters with the real movement code (round SARO's house and its wing, the motel's lot
# to the office and room 6, the diner's lot to its door, Caliche's street, STATION 01's gate to
# the hut and the pier).
# Usage: python3 tools/env_buildings.py OUTDIR [WxH] [--shots-only]
#   S47_URL=file:///.../index.html takes the pictures of another build (--shots-only: the BEFORE).
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 and 'x' in sys.argv[2] else '1280x800').split('x'))
SHOTS_ONLY = '--shots-only' in sys.argv
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
X, SZ = 8000, 8000          # the road area's origin (x), STATION 01's (z): World.ts
MX, MZ = -2.5, 650          # Sierra Motor Court, built in SARO's terms then moved (geo.ts, MOTEL_AT)
DX, DZ = 7979.95, 1999.95   # Mesa Diner (World.DINER_ORIGIN)
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

def yaw_to(x, z, tx, tz): return math.atan2(-(tx - x), -(tz - z))
# name, area, x, z, look-at x, z, pitch
VIEWS = [
    ('b01_saro_south', 'saro', -8.0, 22.0, 1.0, 10.0, 0.1),                 # the annex, the propane tank, the dumpster's cans, the wing beyond
    ('b02_saro_wing', 'saro', 36.0, 19.0, 27.0, 5.0, 0.06),                  # the staff's cars nose in to the east wing
    ('b03_saro_back', 'saro', 30.0, -14.0, 30.0, -4.0, 0.1),                 # behind the wing: the reel, the pallets, the tyres, the downspouts
    ('b04_saro_north', 'saro', -9.0, -14.0, 6.0, -4.0, 0.08),                # the windows side, its sills and the parapet's stains
    ('b05_motel', 'saro', MX - 28.5, MZ + 36.0, MX - 40.5, MZ + 50.0, 0.05),  # the rooms from the lot: the pickup, the chairs, the coolers
    ('b06_diner', 'diner', DX + 18.05, DZ + 1.05, DX - 1.95, DZ - 7.95, 0.05),  # the walk: bench, news box, drinks machine, the roof
    ('b07_caliche', 'diner', X - 5.0, 2094.0, X - 17.0, 2078.0, 0.05),       # the gas station's bay: tyres, drums, the machine by the office
    ('b08_station', 'station01', -6.0, SZ + 11.0, -11.0, SZ + 3.0, 0.04),    # the hut, its render, the shed
]
# the draw calls on High before this work (the same views, main at 048fcd1 on 19 October, before
# the round), and the most each may have now: SARO's outside is held by grounds.py's 340 too
BEFORE = {'b01_saro_south': 319, 'b02_saro_wing': 315, 'b03_saro_back': 154, 'b04_saro_north': 307, 'b05_motel': 57,
          'b06_diner': 83, 'b07_caliche': 106, 'b08_station': 59, 'b00_control_room': 211}
AREA = {'saro': 'grounds', 'motel': 'motel', 'diner': 'diner', 'town': 'town', 'station01': 'site'}
# What the round's five parts may add to a view on High together, over BEFORE (the same in
# env_ground, env_roads, env_buildings, env_interiors and env_atmosphere). Each part was held to 15
# in its own branch, but they meet in the same views: a part's share of a view is about six (the
# ground cover's five meshes; the life's moths, far lights and lying tumbleweeds; the roads' wear
# and lots; a lot's props, stains and pools; a room's things and dirt), five of them thirty. A view
# over it has a part costing twice its share, or a sixth thing in it: a real overshoot.
SHARED = 30

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
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0)")
        await ev("S47.setQuality('high')")
        # the station and the diner (with Caliche) load on demand
        await ev("window.__p = Promise.all([S47.world.prepare('station01'), S47.world.prepare('diner')])")
        for _ in range(300):
            if await ev("!!S47.world.site && !!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev("S47.game.clock = 2 * 3600 + 30 * 60; S47.tick(0.1)")   # half past two: the deputy in, the Night Line on
        # the views without the HUD and its toasts
        await pg.add_style_tag(content='#ui { visibility: hidden !important; }')

        async def at(area, x, z, tx, tz, pitch):
            await ev(f"S47.world.enter('{area}'); S47.player.place({x}, {z}, {yaw_to(x, z, tx, tz)}); S47.player.pitch = {pitch}; S47.tick(0.4)")
        async def calls(): return await ev("S47.renderer.info.render.calls")
        async def room_calls():   # inside the control room where the night begins, the window and the array ahead
            await ev("S47.world.enter('saro'); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, S47.room.spawn.yaw); S47.player.pitch = 0.02; S47.tick(0.4)")
            return await calls()

        hi, lo = {}, {}
        for name, area, x, z, tx, tz, pitch in VIEWS:
            await at(area, x, z, tx, tz, pitch)
            await pg.screenshot(path=f'{OUT}/{name}.png')
            hi[name] = await calls()
        hi['b00_control_room'] = await room_calls()
        await ev("S47.setQuality('low')")
        for name, area, x, z, tx, tz, pitch in VIEWS:
            await at(area, x, z, tx, tz, pitch)
            lo[name] = await calls()
        lo['b00_control_room'] = await room_calls()
        await at(*VIEWS[1][1:])
        await pg.screenshot(path=f'{OUT}/low_{VIEWS[1][0]}.png')
        # (the props are one mesh, the big ones first: Low draws it only as far as they go)
        low_hidden = await ev("""(() => { const w = S47.world, ds = [w.grounds, w.motel, w.diner, w.town, w.site].map((a) => a && a.dressing);
          if (ds.some((d) => !d || !d.props)) return null;
          return ds.every((d) => { const p = d.props, all = p.geometry.attributes.position.count;
            return p.visible && p.userData.lowRange > 0 && p.userData.lowRange < all && p.geometry.drawRange.count === p.userData.lowRange
              && d.group.children.filter((c) => c.userData.highOnly && c.isMesh).every((m) => !m.visible); }); })()""")
        # Ultra on a computer: the props cast and take the real lamps' shadows (core/ultra.ts)
        await ev("S47.setQuality('ultra')")
        for v in (VIEWS[1], VIEWS[7]):
            await at(*v[1:]); await ev("S47.tick(0.2)")
            await pg.screenshot(path=f'{OUT}/ultra_{v[0]}.png')
        await ev("S47.setQuality('high')")
        high_shown = await ev("""(() => { const w = S47.world, ds = [w.grounds, w.motel, w.diner, w.town, w.site].map((a) => a && a.dressing);
          return !ds.some((d) => !d || !d.props) && ds.every((d) => d.props.geometry.drawRange.count >= d.props.geometry.attributes.position.count
            && d.group.children.filter((c) => c.isMesh).every((m) => m.visible)); })()""")
        print('draw calls on High:', json.dumps(hi))
        print('draw calls on Low: ', json.dumps(lo))
        json.dump({'high': hi, 'low': lo}, open(f'{OUT}/calls.json', 'w'), indent=1)
        if SHOTS_ONLY:
            print('\n'.join(errs[:30]) or 'no console errors/warnings')
            await b.close(); return

        # ---------- dressed ----------
        st = await ev("(() => { const w = S47.world; return { saro: w.grounds.dressing.stats, motel: w.motel.dressing.stats, diner: w.diner.dressing.stats, town: w.town.dressing.stats, station: w.site.dressing.stats }; })()")
        print('dressing:', json.dumps(st))
        check(all(s['stains'] >= 60 and s['props'] >= 4 for s in st.values()), 'every area is dressed: stains on its walls and props about it (' + ', '.join(f"{k} {v['stains']}/{v['props']}" for k, v in st.items()) + ')')
        check(st['town']['pools'] >= 6 and st['diner']['pools'] >= 3 and st['motel']['pools'] >= 3, f"porch lights and the drinks machines throw their light (pools: town {st['town']['pools']}, diner {st['diner']['pools']}, motel {st['motel']['pools']})")

        # ---------- the wing's lettering (Exterior.ts: 8 m at x 26.4) stays clear of the pipes ----------
        hits = await ev("""(() => { const T = S47.three, g = S47.world.grounds.dressing.group, rc = new T.Raycaster(), out = [];
          g.updateMatrixWorld(true);
          for (let x = 22.6; x <= 29.4; x += 0.1) for (const y of [1.1, 1.6, 2.1, 2.6, 3.1]) {
            rc.set(new T.Vector3(x, y, 14), new T.Vector3(0, 0, -1)); rc.far = 8.46;
            const h = rc.intersectObjects(g.children, true).filter((q) => q.object.material.name.endsWith('/props'));
            if (h.length) out.push([+x.toFixed(1), y]);
          }
          return out; })()""")
        check(not hits, 'nothing of the dressing stands in front of the SARO lettering on the wing' + (f' (at {hits[:6]})' if hits else ''))

        # ---------- the budget ----------
        over = [f'{k} {hi[k]} (before {BEFORE[k]})' for k in hi if hi[k] > BEFORE[k] + SHARED]
        check(not over, f'no view costs more than {SHARED} draw calls over main before the round (the five parts together): ' + (', '.join(over) if over else ', '.join(f'{k} +{hi[k] - BEFORE[k]}' for k in hi)))
        saro = max(hi[v[0]] for v in VIEWS if v[1] == 'saro' and not v[0].startswith('b05'))
        check(saro <= 340, f'SARO outside at most 340 draw calls on High ({saro}, tools/grounds.py)')
        check(hi['b00_control_room'] <= 300, f"the control room where the night begins, under its 300 ({hi['b00_control_room']})")
        check(all(lo[k] < hi[k] for k in hi if not k.startswith('b00')) and lo['b00_control_room'] <= hi['b00_control_room'], 'Low draws less than High in every view: ' + ', '.join(f'{k} {lo[k]}/{hi[k]}' for k in hi))
        check(low_hidden is True and high_shown is True, f'Low leaves out the small props, the stains and the light pools in all five areas, and High has them back ({low_hidden}, {high_shown})')

        # ---------- walking ----------
        async def place(x, z, yaw=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch=0; S47.tick(0.05)")
        async def walk(points, start=True):
            if start: await place(points[0][0], points[0][1])
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:] if start else points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.14) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 6000) return [false, x, z, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        async def push(x0, z0, x1, z1, steps=240):
            await place(x0, z0)
            return await ev(f'''(()=>{{const p=S47.player;
              for (let n=0; n<{steps}; n++) {{ p.yaw = Math.atan2(-({x1}-p.pos.x), -({z1}-p.pos.z)); p.update(1/30, 0, -1, false); }}
              return [p.pos.x, p.pos.z]; }})()''')
        def said(r): return '' if r[0] else f' (stuck going to {r[1]:.1f},{r[2]:.1f} at {r[3]:.2f},{r[4]:.2f})'

        await ev("S47.world.enter('saro'); S47.doors.set('east', true, true); S47.tick(0.2)")
        # round the house (tools/grounds.py's loop), and round the lab and the wing past the new props
        loop = [(5.0, 1.6), (7.2, 1.6), (10.2, 2.0), (10.6, 4.2), (7.35, 4.6), (7.35, 11.3), (11.8, 11.6), (11.8, 4.6), (10.6, 4.4), (11.8, 11.8),
                (9.8, 12.6), (9.8, 17.6), (9.8, 20.5), (8.6, 23.3), (4.0, 24.5), (-6.0, 24.5), (-1.25, 21.0), (-1.25, 14.2), (-6.1, 13.5),
                (-8.1, 12.4), (-9.6, 8.5), (-11.0, 5.7), (-11.4, 4.2), (-9.0, 0.0), (-9.0, -12.0), (5.0, -12.0), (6.5, -12.0), (8.6, -12.0), (9.5, -12.0),
                (9.5, -1.0), (9.5, 1.5)]
        r = await walk(loop)
        check(r[0], 'SARO: round the house on foot as before (east door, truck, ramp, road, path, fire exit, windows side, steps)' + said(r))
        east = [(9.8, 17.6), (16.5, 18.0), (16.5, 3.0), (19.7, 1.5), (19.7, -10.0), (30.0, -12.0), (42.5, -12.0), (42.5, 20.0), (25.0, 20.0), (9.8, 18.0)]
        r = await walk(east)
        check(r[0], 'SARO: round the photo lab and the east wing, past the reel and the cars' + said(r))
        r = await walk([(24.0, 14.5), (24.0, 6.5), (33.5, 6.5), (33.5, 14.5)])
        check(r[0], 'SARO: in front of the cars, between their noses and the wing' + said(r))
        x, z = await push(27.0, 15.5, 27.0, 8.0)
        check(z > 12.3, f'SARO: the sedan holds the player (ended at {x:.2f},{z:.2f})')
        x, z = await push(36.6, -10.0, 35.5, -6.0)
        check(z < -7.6, f'SARO: and so does the cable reel behind the wing ({x:.2f},{z:.2f})')
        # the motel: the lot to the office and along the front of the lot to room 6 (tools/motel.py)
        office = await ev("S47.world.court.anchors.officeInside"); out = await ev("S47.world.court.anchors.room6Outside")
        r = await walk([(MX - 33, MZ + 8), (MX - 37.5, MZ + 20), (MX - 39.6, MZ + 24.1), (office['x'], office['z'])])
        check(r[0], 'the motel: from the lot into the office' + said(r))
        r = await walk([(MX - 39.6, MZ + 24.1), (MX - 37.5, MZ + 26), (MX - 37.5, out['z'] - 0.6), (out['x'], out['z'])], start=False)
        check(r[0], 'the motel: along the lot past the pickup to the door of room 6' + said(r))
        r = await walk([(MX - 39.5, MZ + 29.9), (MX - 39.5, MZ + 69.5)])   # (clear of the planters at -40.2)
        check(r[0], 'the motel: the length of the covered walk past the drinks machine and the chairs' + said(r))
        x, z = await push(MX - 30.0, MZ + 43.5, MX - 36.0, MZ + 43.5)
        check(x > MX - 31.2, f'the motel: the pickup holds the player ({x - MX:.2f},{z - MZ:.2f})')
        # the diner: the lot to the door and inside; the walk its length past the bench and the machine
        await ev("S47.world.enter('diner'); S47.world.placeAtDiner('arrive'); S47.tick(0.3)")
        r = await walk([(DX + 3.1, DZ + 0.0), (DX + 0.75, DZ + 0.0)], start=False)
        check(r[0] and await ev("S47.world.indoors(S47.player.pos)"), 'the diner: from the truck across the lot, through the door and inside' + said(r))
        r = await walk([(DX + 3.2, DZ - 0.3), (DX + 2.45, DZ - 0.5), (DX + 2.45, DZ - 8.9), (DX + 2.45, DZ + 3.6), (DX + 3.25, DZ + 4.0), (DX + 3.25, DZ + 9.1), (DX + 6.0, DZ + 11.0)])
        check(r[0], 'the diner: the length of the walk past the barrel, the news box, the bench and the drinks machine' + said(r))
        # Caliche: the street (tools/town.py's route)
        route = [(X - 10, 2021), (X - 13, 2030), (X - 13, 2096), (X - 13.8, 2180), (X - 1.2, 2250), (X + 17.6, 2220), (X + 17.8, 2120), (X + 17.2, 2050), (X + 6, 2000), (X - 6, 2010)]
        await place(*route[0])
        r = await walk(route)
        check(r[0], "Caliche: down the west side, across, up the east side and back to the diner's lot" + said(r))
        r = await walk([(X - 13.0, 2060.0), (X - 15.5, 2071.0), (X - 15.6, 2090.0), (X - 13.6, 2124.0), (X - 13.6, 2141.0), (X - 11.5, 2190.0), (X - 11.0, 2218.0)])
        check(r[0], 'Caliche: close along the fronts past the gas station, the laundromat and the feed store' + said(r))
        # STATION 01 (its walk gap is chained in chapter one: from inside the gate), to the hut and in,
        # round the hut and the shed past the new props, then the worn path out to the pier
        await ev("S47.world.enter('station01'); S47.tick(0.2)")
        r = await walk([(-22.3, SZ + 23.5), (-15.8, SZ + 12.8), (-11.8, SZ + 7.8), (-9.95, SZ + 5.7), (-9.95, SZ + 4.2), (-11.0, SZ + 3.2)])
        check(r[0] and await ev("S47.world.indoors(S47.player.pos)"), 'STATION 01: from inside the gate along the path to the hut and in at its door' + said(r))
        r = await walk([(-9.95, SZ + 6.2), (-14.2, SZ + 6.5), (-14.6, SZ + 3.6), (-19.0, SZ + 2.0), (-19.0, SZ - 3.2), (-14.2, SZ - 2.6), (-11.0, SZ + 0.75),
                        (-7.5, SZ + 0.75), (-6.8, SZ + 4.2), (-3.2, SZ + 2.2), (0.4, SZ - 0.6)])
        check(r[0], 'STATION 01: round the hut and the shed, past the drum, the crates and the pallet, out to the pier' + said(r))

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
