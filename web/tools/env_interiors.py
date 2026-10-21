# The interiors (world/clutter.ts and the rooms that use it): the control room, the south
# corridor, the records room, the photo lab, the motel office, room 6, the diner and the hut at
# STATION 01. Shoots each from a fixed place on High (and Low), prints the draw calls, and checks
# that each room stays within its budget (at most SHARED more than before the round for its five
# parts together, the control room about 300), that Low draws less of the clutter than High, and that
# the player still walks where the chapters need them to with the real movement code.
# Usage: python3 tools/env_interiors.py OUTDIR [WxH] [before]
#   before: shots and draw calls only (the state before the clutter was added)
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright
OUT = sys.argv[1]; W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 and 'x' in sys.argv[2] else '1280x800').split('x'))
BEFORE = 'before' in sys.argv[2:]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
os.makedirs(OUT, exist_ok=True)
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# area, name, eye (x, z, area-local), target (x, y, z, area-local)
VIEWS = [
    ('saro', 'i01_control_desk', (0.2, 0.7), (3.6, 0.95, 3.4)),
    ('saro', 'i02_control_console', (1.5, -1.9), (-1.6, 0.8, -4.0)),
    ('saro', 'i03_corridor', (1.25, 5.75), (-5.5, 1.15, 5.7)),
    ('saro', 'i04_records', (-2.0, 7.6), (-5.4, 0.9, 9.8)),
    ('saro', 'i05_photolab', (14.5, -1.4), (15.6, 1.0, -5.4)),
    ('court', 'i06_motel_office', (-41.9, 23.0), (-45.0, 1.15, 26.2)),
    ('room6', 'i07_room6', (-1.37, 2.62), (0.12, 1.02, -0.5)),
    ('diner', 'i08_diner', (0.6, -0.6), (-4.6, 1.15, -4.0)),
    ('station01', 'i09_hut', (-10.15, 4.05), (-12.4, 0.95, 1.8)),
]
# draw calls on High measured at these views before the clutter (env_interiors.py ... before,
# main at 048fcd1, 19 October 2026, on the graphics card)
BASE = {'i01_control_desk': 190, 'i02_control_console': 99, 'i03_corridor': 157, 'i04_records': 98, 'i05_photolab': 97,
        'i06_motel_office': 72, 'i07_room6': 56, 'i08_diner': 62, 'i09_hut': 55}
# What the round's five parts may add to a view on High together, over BASE (the same in
# env_ground, env_roads, env_buildings, env_interiors and env_atmosphere). Each part was held to 15
# in its own branch, but they meet in the same views: a part's share of a view is about six (the
# ground cover's five meshes; the life's moths, far lights and lying tumbleweeds; the roads' wear
# and lots; a lot's props, stains and pools; a room's things and dirt), five of them thirty. A view
# over it has a part costing twice its share, or a sixth thing in it: a real overshoot. (Through a
# window or a door a room sees the yard, the road and the land too.)
SHARED = 30
# artcheck's control room views: about 300 at most
ROOM_VIEWS = [('desk', 2.6, 3.5, 0, -0.50), ('room', -4.7, 3.5, -0.65, -0.16), ('window', 0.05, -2.2, 0, 0.06)]

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
        await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(300)
        ev = pg.evaluate
        await ev("S47.hold = true"); await ev("S47.jump('chapter1')"); await ev("S47.tick(1.5)")
        await ev("S47.game.d.ui.close && S47.game.d.ui.close(true)")
        await ev("S47.doors.set('south', true, true); S47.doors.set('records', true, true); S47.doors.set('lab', true, true); S47.tick(0.5)")
        # the areas that load on demand (their modules and pictures come in real time)
        for area in ['room6', 'diner', 'station01']:
            await ev(f"window.__ready_{area} = false; S47.world.prepare('{area}').then(() => window.__ready_{area} = true)")
            for _ in range(200):
                if await ev(f"window.__ready_{area}"): break
                await pg.wait_for_timeout(100)
            check(await ev(f"window.__ready_{area}"), f'{area} loaded')

        # an area's origin (its local metres are added to it)
        async def origin(kind):
            if kind == 'saro': return [0, 0, 0]
            if kind == 'court': a = await ev("S47.world.court.at"); return [a['x'], a['y'], a['z']]
            g = {'room6': 'room6', 'diner': 'diner', 'station01': 'site'}[kind]
            return await ev(f"(()=>{{const p=S47.world.{g}.group.position; return [p.x,p.y,p.z]}})()")
        async def enter(kind):
            area = 'saro' if kind in ('saro', 'court') else kind
            if await ev("S47.world.area") != area: await ev(f"S47.world.enter('{area}'); S47.tick(0.1)")
        async def look(kind, eye, target):
            o = await origin(kind)
            x, z = eye[0] + o[0], eye[1] + o[2]
            tx, ty, tz = target[0] + o[0], target[1] + o[1], target[2] + o[2]
            await enter(kind)
            await ev(f"""(()=>{{const p=S47.player; p.place({x},{z},0);
              p.yaw=Math.atan2(-({tx}-p.pos.x), -({tz}-p.pos.z));
              p.pitch=Math.atan2({ty}-(p.floorY+p.eye), Math.hypot({tx}-p.pos.x,{tz}-p.pos.z)); S47.tick(0.3); S47.tick(0.1);}})()""")
            return await ev("({calls:S47.renderer.info.render.calls, tris:S47.renderer.info.render.triangles})")
        # walk from point to point with Player.update (area-local points); false if one cannot be reached
        async def walk(kind, points):
            o = await origin(kind); await enter(kind)
            pts = [[x + o[0], z + o[2]] for x, z in points]
            await ev(f"S47.player.place({pts[0][0]},{pts[0][1]},0); S47.tick(0.05)")
            r = await ev(f'''(()=>{{const pts={json.dumps(pts[1:])}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 2000) return [false, x, z, p.pos.x, p.pos.z]; }} }}
              return [true]; }})()''')
            return r if r[0] else [False, r[1] - o[0], r[2] - o[2], r[3] - o[0], r[4] - o[2]]
        # the clutter meshes (world/clutter.ts) in an area and how much of them is drawn
        async def clutter():
            return await ev("""(()=>{ const out = []; S47.scene.traverse(o => { if (o.isMesh && o.name.startsWith('clutter:')) {
                const g = o.geometry, n = (g.index ? g.index.count : g.attributes.position.count);
                out.push({ name: o.name, all: n, drawn: o.visible ? Math.min(n, g.drawRange.count) : 0, mat: o.material.type }); } }); return out; })()""")

        # ---------- the views, on High and on Low ----------
        await ev("document.getElementById('ui').style.visibility = 'hidden'")   # the pictures without the HUD
        res = {}
        for q in ['high', 'low']:
            await ev(f"S47.setQuality('{q}'); S47.tick(0.1)")
            for kind, name, eye, target in VIEWS:
                r = await look(kind, eye, target)
                await pg.screenshot(path=f'{OUT}/{name}{"" if q == "high" else "-low"}.png')
                res[(name, q)] = r
            for name, x, z, yaw, pitch in ROOM_VIEWS:
                await enter('saro')
                await ev(f'S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.3)')
                res[(name, q)] = await ev("({calls:S47.renderer.info.render.calls, tris:S47.renderer.info.render.triangles})")
            res[('clutter', q)] = await clutter()
        print('draw calls (High / Low):')
        for kind, name, *_ in VIEWS + [(None, n) for n, *_ in ROOM_VIEWS]:
            h, l = res[(name, 'high')], res[(name, 'low')]
            print(f"  {name:22s} {h['calls']:4d} / {l['calls']:4d}   triangles {h['tris']:7d} / {l['tris']:7d}", flush=True)
        json.dump({f'{k[0]}-{k[1]}': v for k, v in res.items()}, open(f'{OUT}/views.json', 'w'), indent=1)

        room = max(res[(n, 'high')]['calls'] for n, *_ in ROOM_VIEWS + [('i01_control_desk',), ('i02_control_console',)])
        check(room <= 300, f'the control room at most about 300 draw calls on High ({room})')
        if not BEFORE:
            over = [f"{n} {res[(n, 'high')]['calls']} (was {BASE[n]})" for _, n, *_ in VIEWS if n in BASE and res[(n, 'high')]['calls'] > BASE[n] + SHARED]
            check(not over, f'each room at most {SHARED} draw calls on High over main before the round, the five parts together: ' + ('; '.join(over) if over else ', '.join(f"{n} +{res[(n, 'high')]['calls'] - BASE[n]}" for _, n, *_ in VIEWS if n in BASE)))
            low = [n for _, n, *_ in VIEWS if res[(n, 'low')]['calls'] > res[(n, 'high')]['calls']]
            check(not low, 'Low never draws more than High' + (': ' + ', '.join(low) if low else ''))
            cl_h, cl_l = res[('clutter', 'high')], res[('clutter', 'low')]
            names = sorted({c['name'] for c in cl_h})
            want = ['clutter:control', 'clutter:annex', 'clutter:lab', 'clutter:office', 'clutter:room6', 'clutter:diner', 'clutter:hut']
            check(all(any(n.startswith(w) for n in names) for w in want), f'every room has its clutter ({len(names)} meshes: {", ".join(names)})')
            thin = sum(c['drawn'] for c in cl_l) < 0.8 * sum(c['drawn'] for c in cl_h) and all(c['drawn'] == c['all'] for c in cl_h)
            check(thin, f"Low draws less of the clutter: {sum(c['drawn'] for c in cl_l)} of {sum(c['all'] for c in cl_h)} vertices (High all of them)")
            tris = [n for _, n, *_ in VIEWS if res[(n, 'low')]['tris'] > res[(n, 'high')]['tris']]
            check(not tris, 'and fewer triangles on Low in every view' + (': ' + ', '.join(tris) if tris else ''))
        await ev("S47.setQuality('high'); document.getElementById('ui').style.visibility = ''; S47.tick(0.1)")

        # ---------- where the chapters walk ----------
        await ev("S47.doors.set('east', true, true); S47.tick(0.5)")
        walks = [
            ('saro', 'control room: the desk, the console, the rack, the printer, the map, the west desk, the coffee corner',
             [(1.55, 3.7), (0.6, 1.2), (0.0, -2.35), (-2.15, -2.35), (2.15, -2.35), (4.85, -2.4), (4.85, -0.4), (4.9, 0.7), (0.5, 0.0), (-4.9, -3.0), (-4.9, -0.6), (-4.3, 1.5), (-4.25, 3.4), (-2.4, 3.9), (1.2, 3.9), (3.4, 3.9), (3.65, 3.3), (3.95, 2.5), (3.4, 1.6), (5.4, 1.6)]),
            ('saro', 'out of the east door to the walk', [(4.5, 1.6), (7.2, 1.6), (9.5, 1.0)]),
            ('saro', 'corridor and records room: the doorway, the field box, the work table, the binder, the cabinet, the phone',
             [(-2.4, 3.9), (-2.4, 6.0), (0.5, 6.0), (1.4, 5.4), (0.5, 6.0), (-3.9, 6.0), (-3.9, 7.3), (-5.15, 8.6), (-4.0, 7.65), (-2.4, 7.6), (-2.3, 9.6), (-1.65, 9.25), (-2.3, 9.0), (-2.2, 7.4), (-3.9, 7.3), (-3.9, 6.0), (-5.4, 5.7)]),
            ('saro', 'photo lab: in at the door, the wet bench, the enlarger, the archive bench, the records desk',
             [(11.8, -1.2), (13.4, -1.2), (14.5, -1.4), (14.7, -4.6), (16.8, -4.6), (17.75, -3.0), (17.0, -0.35), (14.0, -1.5), (13.4, -1.2), (11.8, -1.2)]),
            ('court', 'motel office: in at the door, the counter, the phone, the key board, the shelves',
             [(-39.6, 24.1), (-42.6, 24.1), (-43.2, 23.0), (-45.05, 22.9), (-43.2, 23.0), (-42.9, 26.3), (-42.9, 24.1), (-39.6, 24.1)]),
            ('room6', 'room 6: from the walk to the table, Nora, the phone, the television, the bathroom door, round the east side of the table',
             [(-1.4, 4.3), (-1.4, 2.2), (-1.4, 1.28), (-0.42, 1.28), (-0.5, -0.2), (-0.27, -1.75), (-1.15, -0.55), (-0.97, -2.6), (-0.5, -0.2), (1.2, -0.05), (1.62, 0.3), (1.75, 1.45), (1.62, 0.3), (1.2, -0.05), (-0.5, -0.2), (-1.4, 1.0), (-1.4, 4.3)]),
            ('diner', 'diner: the door, the stool, the aisle to the payphone, the jukebox and the menu',
             [(3.1, 0.0), (0.75, 0.0), (-2.8, 0.0), (-1.2, 0.0), (-1.2, 7.9), (0.4, 8.05), (-1.2, 7.9), (-1.2, -6.9), (-2.8, -2.4), (-3.72, -2.4), (-1.2, 0.0), (0.75, 0.0), (3.1, 0.0)]),
            ('station01', 'the hut: in at the door, the receiver, the timing log, the field phone, out again',
             [(-9.95, 6.0), (-9.95, 4.0), (-11.0, 3.2), (-11.85, 2.55), (-11.05, 2.55), (-12.45, 2.55), (-11.5, 2.6), (-11.0, 3.4), (-9.95, 4.0), (-9.95, 6.0)]),
        ]
        await ev("S47.world.diner.objs.door.rotation.y = Math.PI / 2; S47.tick(0.1)")
        for kind, what, pts in walks:
            r = await walk(kind, pts)
            check(r[0], what + ('' if r[0] else f' (stuck going to {r[1]:.2f},{r[2]:.2f} at {r[3]:.2f},{r[4]:.2f})'))

        flood = [e for e in errs if 'flood set full' in e]
        check(not flood, 'no flood set is over full' + (': ' + '; '.join(flood[:3]) if flood else ''))
        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        if fails: raise SystemExit(1)
asyncio.run(main())
