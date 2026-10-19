# The Blender pilot's test (world/blenderModels.ts, tools/blender/): SARO's house as the game
# builds it (flag off) and as modelled and baked in Blender (?blender, S47.blender), from the same
# fixed viewpoints at the same moment, at night and at the first light (05:22), on High. Writes
# each pair and a side-by-side picture of it (compare/, off left, on right), prints the draw calls
# and triangles of every view both ways, the .glb's size and how long it takes to load, and checks:
# the flag is off by default and nothing is loaded then; on, the model is in and the boxes it
# stands for are hidden (and back when it is off again); it never costs a view a draw call more;
# SARO outside stays within grounds.py's 340 and the control room within its 300; the rays meet
# the same things either way (the model takes none); the doors still swing; Low and Ultra take it
# (Lambert with the mask, shadows); no console errors.
# Usage: python3 tools/blender_ab.py OUTDIR [WxH]   (S47_URL for another build)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 and 'x' in sys.argv[2] else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
GLB = os.path.abspath('src/assets/models/saro_house.glb')
checks = []
def check(ok, what): checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

def yaw_to(x, z, tx, tz): return math.atan2(-(tx - x), -(tz - z))
# name, x, z, yaw (or a point to look at), pitch
VIEWS = [
    ('a01_south', -2.0, 30.0, (4.0, 4.0), 0.06),          # the annex, the control room above it, the wing beyond
    ('a02_wing', 44.0, 16.0, (30.0, 2.0), 0.08),          # the wing's south face: its windows, the lettering
    ('a03_north', -14.0, -16.0, (4.0, -3.0), 0.1),        # the control room's long window, the parapet
    ('a04_corner', -10.0, -9.0, (-6.0, -4.0), 0.25),      # close: the north-west corner, coping, downpipe, sill
    ('a05_wing_back', 24.0, -16.0, (32.0, -4.0), 0.1),    # the wing's north face
    ('a06_east_door', 10.5, 1.0, (6.3, 1.9), 0.12),       # the service door from the landing, the hood
    ('a07_far', -30.0, -40.0, (10.0, 0.0), 0.03),         # from out by the array
    ('a08_wing_close', 34.6, 8.6, (33.4, 5.5), 0.12),     # close to the wing's windows
    ('g04', -2.0, 30.0, 0.15, 0.0),                       # tools/grounds.py's views of the house (its 340)
    ('g07', -8.0, -12.0, -math.pi / 2 + 0.5, 0.05),
    ('g11', 30.0, 16.0, 0.2, 0.05),
]
GROUNDS = {'g04', 'g07', 'g11'}
NIGHT, DAWN = 1 * 3600, 5 * 3600 + 22 * 60

async def main():
    os.makedirs(f'{OUT}/compare', exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        pg = await (await b.new_context(viewport={'width': W, 'height': H})).new_page(); pg.set_default_timeout(300000)
        errs = []
        pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
        await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg.goto(URL)
        await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(400)
        ev = pg.evaluate
        await ev("S47.hold = true; S47.jump('chapter1'); S47.tick(1.0); S47.setQuality('high'); S47.setAbductions(false)")
        await pg.add_style_tag(content='#ui { visibility: hidden !important; }')

        # ---------- off by default ----------
        d = await ev("""(() => { const h = S47.blenderHouse; return { on: S47.blender, group: h.group.visible, model: !!h.model,
          replaced: h.replaced.map((o) => [o.name, o.visible]) }; })()""")
        check(d['on'] is False and not d['group'] and not d['model'] and all(v for _, v in d['replaced']) and len(d['replaced']) == 4,
              f"off by default: nothing loaded, the four code-built meshes shown ({', '.join(n for n, _ in d['replaced'])})")

        # ---------- loading ----------
        load = await ev("""(async () => { const t = performance.now(); S47.blender = true;
          while (!S47.blenderHouse.shown && !S47.blenderHouse.failed && performance.now() - t < 20000) await new Promise((r) => setTimeout(r, 10));
          const ms = performance.now() - t, f = performance.now(); S47.tick(0.01); return { ms, frame: performance.now() - f, failed: S47.blenderHouse.failed }; })()""")
        m = await ev("""(() => { const h = S47.blenderHouse, out = { meshes: [], hidden: h.replaced.every((o) => !o.visible), group: h.group.visible };
          h.group.traverse((o) => { if (o.isMesh) out.meshes.push({ name: o.name, mat: o.material.name, tris: (o.geometry.index ? o.geometry.index.count : o.geometry.attributes.position.count) / 3,
            ao: !!o.material.aoMap && o.material.aoMap.channel === 1 && !!o.geometry.attributes.uv1, map: o.material.map && o.material.map.name,
            emissive: o.material.emissiveIntensity * o.material.emissive.getHex() > 0 }); });
          return out; })()""")
        size = os.path.getsize(GLB)
        print(f"loaded in {load['ms']:.0f} ms, first frame after it {load['frame']:.0f} ms; .glb {size / 1024:.0f} KB; meshes {json.dumps(m['meshes'])}")
        names = sorted(x['mat'] for x in m['meshes'])
        check(not load['failed'] and m['group'] and m['hidden'] and names == ['glass_dark', 'glass_lit', 'house', 'wing'],
              f"?blender: the model is in (house, wing and the panes) and the boxes it stands for are hidden ({load['ms']:.0f} ms){' ' + load['failed'] if load['failed'] else ''}")
        walls = [x for x in m['meshes'] if x['mat'] in ('house', 'wing')]
        check(all(x['ao'] and x['map'] == 'art/concrete' for x in walls) and any(x['emissive'] for x in m['meshes'] if x['mat'] == 'glass_lit'),
              'the walls carry the baked mask as aoMap on the second UV set and the concrete picture on the first; the lit panes glow')
        check(size < 2.5 * 1024 * 1024, f'the house .glb is {size / 1024:.0f} KB (both models together are to stay well under 5 MB)')

        # ---------- the rays meet the same things ----------
        rays = """(() => { const T = S47.three, rc = new T.Raycaster(), out = [];
          const shots = [[10, 1.5, 1.6, -1, 0, 0], [-12, 1.5, 0, 1, 0, 0], [0, 1.5, -12, 0, 0, 1], [30, 1.5, 14, 0, 0, -1], [30, 1.5, -14, 0, 0, 1], [-2, 1.6, 20, 0, 0, -1]];
          S47.scene.updateMatrixWorld(true);
          for (const [x, y, z, dx, dy, dz] of shots) { rc.set(new T.Vector3(x, y, z), new T.Vector3(dx, dy, dz)); rc.far = 30;
            const h = rc.intersectObjects(S47.scene.children, true)[0]; out.push(h ? (h.object.name || h.object.type) + '@' + h.distance.toFixed(2) : '-'); }
          return out; })()"""
        hits_on = await ev(rays)
        await ev("S47.blender = false; S47.tick(0.01)")
        back = await ev("S47.blenderHouse.replaced.every((o) => o.visible) && !S47.blenderHouse.group.visible")
        hits_off = await ev(rays)
        check(back, 'off again: the boxes are back and the model hidden')
        check(hits_on == hits_off, f'the rays meet the same things with the model in: {hits_on}')

        # ---------- the views, off and on at the same moment ----------
        async def at(v):
            name, x, z, look, pitch = v
            yaw = yaw_to(x, z, *look) if isinstance(look, tuple) else look
            await ev(f"S47.world.enter('saro'); S47.player.place({x}, {z}, {yaw}); S47.player.pitch = {pitch}; S47.tick(0.4)")
        async def stats(): return await ev("[S47.renderer.info.render.calls, S47.renderer.info.render.triangles]")
        calls = {}
        for tod, clock in (('night', NIGHT), ('dawn', DAWN)):
            await ev(f"S47.game.clock = {clock}; S47.tick(0.2)")
            for v in VIEWS:
                await at(v)
                await pg.screenshot(path=f'{OUT}/{v[0]}_{tod}_off.png'); off = await stats()
                await ev("S47.blender = true; S47.tick(1 / 30)")
                await pg.screenshot(path=f'{OUT}/{v[0]}_{tod}_on.png'); on = await stats()
                await ev("S47.blender = false")
                calls[f'{v[0]}_{tod}'] = {'off': off, 'on': on}
        room = {}
        for flag in ('false', 'true'):
            await ev(f"S47.blender = {flag}; S47.game.clock = {NIGHT}; S47.player.place(S47.room.spawn.x, S47.room.spawn.z, S47.room.spawn.yaw); S47.player.pitch = 0.02; S47.tick(0.4)")
            room[flag] = await stats()
        await pg.screenshot(path=f'{OUT}/a00_room_night_on.png')
        print('draw calls and triangles, off -> on:')
        for k, c in calls.items(): print(f"  {k:24s} {c['off'][0]:4d} -> {c['on'][0]:4d}   {c['off'][1]:7d} -> {c['on'][1]:7d}")
        print(f"  {'a00_room (inside)':24s} {room['false'][0]:4d} -> {room['true'][0]:4d}   {room['false'][1]:7d} -> {room['true'][1]:7d}")
        json.dump({'calls': calls, 'room': room, 'load_ms': load['ms'], 'first_frame_ms': load['frame'], 'glb_bytes': size}, open(f'{OUT}/numbers.json', 'w'), indent=1)
        more = [f"{k} {c['off'][0]} -> {c['on'][0]}" for k, c in calls.items() if c['on'][0] > c['off'][0]]
        check(not more, 'the model never costs a view a draw call more' + (': ' + ', '.join(more) if more else
              f" (from {min(c['off'][0] - c['on'][0] for c in calls.values())} to {max(c['off'][0] - c['on'][0] for c in calls.values())} fewer)"))
        g = max(c['on'][0] for k, c in calls.items() if k.split('_')[0] in GROUNDS)
        check(g <= 340, f'SARO outside within 340 on High with the model (grounds.py views, most {g})')
        check(room['true'][0] <= 300 and room['false'][0] <= 300, f"the control room within its 300 either way ({room['false'][0]} -> {room['true'][0]})")

        # ---------- the doors, Low, Ultra ----------
        await ev("S47.blender = true; S47.doors.set('east', true, true); S47.doors.set('exit', true, true); S47.tick(0.1)")
        door = await ev("[S47.room.doorHinge.rotation.y, S47.annex.exitOpen, S47.doors.isOpen('east')]")
        check(abs(door[0] - math.pi / 2) < 0.01 and door[1] == 1 and door[2], 'the east door and the fire exit still swing open with the model in')
        await at(VIEWS[5]); await pg.screenshot(path=f'{OUT}/a06_east_door_night_on_open.png')
        await ev("S47.doors.set('east', false, true); S47.doors.set('exit', false, true)")
        await ev("S47.setQuality('low'); S47.tick(0.1)")
        await at(VIEWS[1]); low = await stats()
        await pg.screenshot(path=f'{OUT}/a02_wing_night_on_low.png')
        lowm = await ev("""(() => { const out = []; S47.blenderHouse.group.traverse((o) => { if (o.isMesh) out.push([o.material.type, !!o.material.aoMap]); }); return out; })()""")
        check(all(t == 'MeshLambertMaterial' for t, _ in lowm) and sum(1 for _, a in lowm if a) == 2 and low[0] <= calls['a02_wing_night']['on'][0],
              f"Low: Lambert with the mask on the walls ({low[0]} draw calls)")
        await ev("S47.setQuality('ultra'); S47.tick(0.3)")
        for v in (VIEWS[1], VIEWS[3]):
            await at(v); await ev("S47.tick(0.2)")
            await pg.screenshot(path=f'{OUT}/{v[0]}_night_on_ultra.png')
            await ev("S47.blender = false; S47.tick(0.05)")
            await pg.screenshot(path=f'{OUT}/{v[0]}_night_off_ultra.png')
            await ev("S47.blender = true")
        shadow = await ev("""(() => { let n = 0, c = 0; S47.blenderHouse.group.traverse((o) => { if (o.isMesh) { n++; if (o.castShadow && o.receiveShadow) c++; } }); return [c, n]; })()""")
        check(shadow[0] == shadow[1] == 4, f'Ultra: the model casts and takes the real lamps\' shadows ({shadow[0]} of {shadow[1]})')
        await ev("S47.setQuality('high')")

        # ---------- ?blender in the address ----------
        pg2 = await (await b.new_context(viewport={'width': 640, 'height': 400})).new_page(); pg2.set_default_timeout(300000)
        pg2.on('console', lambda m: errs.append(f'(?blender) {m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg2.on('pageerror', lambda e: errs.append('(?blender) PAGEERROR: ' + str(e)))
        await pg2.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg2.goto(URL + ('&' if '?' in URL else '?') + 'blender')
        await pg2.wait_for_selector('button[data-a=start]'); await pg2.click('button[data-a=start]'); await pg2.wait_for_timeout(400)
        await pg2.evaluate("S47.hold = true; S47.jump('chapter1'); S47.tick(0.5)")
        url_on = await pg2.evaluate("""(async () => { const t = performance.now();
          while (!S47.blenderHouse.shown && !S47.blenderHouse.failed && performance.now() - t < 20000) await new Promise((r) => setTimeout(r, 20));
          return [S47.blender, S47.blenderHouse.shown, S47.blenderHouse.replaced.every((o) => !o.visible)]; })()""")
        check(url_on == [True, True, True], f'?blender in the address shows the model from the start ({url_on})')
        await b.close()

    # ---------- the side-by-side pictures ----------
    try:
        from PIL import Image, ImageDraw
        for v in [x[0] for x in VIEWS]:
            for tod in ('night', 'dawn'):
                a, c = Image.open(f'{OUT}/{v}_{tod}_off.png').convert('RGB'), Image.open(f'{OUT}/{v}_{tod}_on.png').convert('RGB')
                w, h = a.size
                s = Image.new('RGB', (w * 2 + 8, h), (12, 12, 12)); s.paste(a, (0, 0)); s.paste(c, (w + 8, 0))
                dr = ImageDraw.Draw(s)
                for x0, t in ((10, f'{v} {tod}: as built in code'), (w + 18, f'{v} {tod}: ?blender')): dr.text((x0, 10), t, fill=(255, 230, 160))
                s.save(f'{OUT}/compare/{v}_{tod}.jpg', quality=88)
    except ImportError:
        print('(no PIL: no side-by-side pictures)')
    print('\n'.join(errs[:30]) or 'no console errors/warnings')
    if errs: check(False, 'console clean')
    json.dump(checks, open(f'{OUT}/checks.json', 'w'), indent=1)
    fails = [c for c in checks if c[0] == 'FAIL']
    print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
    if fails: raise SystemExit(1)

asyncio.run(main())
