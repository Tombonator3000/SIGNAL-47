# The Blender pilot's test (world/blenderModels.ts, tools/blender/): SARO's house and SARO 07 as
# the game builds them (flag off) and as modelled and baked in Blender (?blender, S47.blender),
# from the same fixed viewpoints at the same moment, on High. Writes each pair and a side-by-side
# picture of it (compare/, off left, on right), prints the draw calls and triangles of every view
# both ways, the .glbs' sizes and how long they take to load, and checks:
#   the house (step one), at night and at the first light (05:22): the flag is off by default and
#   nothing is loaded then; on, the model is in and the boxes it stands for are hidden (and back
#   when it is off again); it never costs a view a draw call more; SARO outside stays within
#   grounds.py's 340 and the control room within its 300 with both models in; the rays meet the
#   same things either way (the model takes none); the doors still swing; Low and Ultra take it.
#   the truck (step two): every copy of it (SARO's pad and the motel's lot, STATION 01, the road
#   and the diner's lot, the old road) gets the model in its own area's floods, the mask on the
#   right UV set, the modelled wheel in the instances; the lenses, decals, mirror glass, beacon
#   and glow stay; parked on the pad at night (four sides), on the motel's lot, at the station,
#   on the diner's lot, in its own headlights on the highway, from the driver's seat while
#   driving (night) and on the old road at dawn (the cab and outside); from behind (the first
#   pass's rear camera stood in a wall); the wheels turn and steer,
#   the steering wheel, the head and brake lights, the needles, the cab while driving and the
#   shell hidden; the rays the same; never a draw call more; Low and Ultra.
#   the final pass: the house's lamps baked and scaled by their floods, the wall lamps' glows small,
#   the blinds in the reveals, the lit panes' rooms, the skirt; the truck's shade on the ground, its
#   chrome and glass with the sky; and the phone's size, 844 by 390: the wing and the pad at night.
# No console errors.
# Usage: python3 tools/blender_ab.py OUTDIR [WxH]   (S47_URL for another build)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 and 'x' in sys.argv[2] else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
GLB = os.path.abspath('src/assets/models/saro_house.glb')
TRUCK_GLB = os.path.abspath('src/assets/models/saro_truck.glb')
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
# the truck's views: name, where (pad, motel, station01, diner), the camera in the truck's own
# frame (x right, z back, y up) and the height looked at
TRUCK_PARKED = [
    ('t01_pad_front34', 'saro', -4.5, 1.4, -5.5, 0.9),
    ('t02_pad_side', 'saro', -6.5, 1.0, 0.3, 0.9),
    ('t03_pad_rear34', 'saro', -3.2, 1.7, 5.4, 0.9),       # (from the walk behind the pad: the first pass's camera stood in a wall)
    ('t03b_pad_rear', 'saro', 0.0, 1.5, 6.2, 0.9),
    ('t04_pad_wheel', 'saro', -3.2, 0.4, -3.8, 0.6),
    ('t05_motel', 'motel', -4.0, 1.5, -5.0, 0.9),
    ('t06_station', 'station01', -4.5, 1.5, -5.5, 0.9),
    ('t07_diner', 'diner', 4.5, 1.6, -5.5, 0.9),
]

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
        d = await ev("""(() => { const B = S47.blenderTruck, t = S47.world.saroTruck;
          return [B.on, !!B['model'], [...B.trucks].length, t.modelled, t['replaced'].every((o) => o.visible), t['modelMeshes'] === null]; })()""")
        check(d[:2] == [False, False] and d[2] >= 1 and d[3:] == [False, True, True], f'off by default: the truck as the code builds it, its model not loaded ({d})')

        # ---------- loading ----------
        load = await ev("""(async () => { const t = performance.now(); void S47.blenderHouse.set(true);
          while (!S47.blenderHouse.shown && !S47.blenderHouse.failed && performance.now() - t < 20000) await new Promise((r) => setTimeout(r, 10));
          const ms = performance.now() - t, f = performance.now(); S47.tick(0.01); return { ms, frame: performance.now() - f, failed: S47.blenderHouse.failed }; })()""")
        m = await ev("""(() => { const h = S47.blenderHouse, out = { meshes: [], hidden: h.replaced.every((o) => !o.visible), group: h.group.visible };
          h.group.traverse((o) => { if (o.isMesh) out.meshes.push({ name: o.name, mat: o.material.name, tris: (o.geometry.index ? o.geometry.index.count : o.geometry.attributes.position.count) / 3,
            ao: !!o.material.aoMap && o.material.aoMap.channel === 1 && !!o.geometry.attributes.uv1, map: o.material.map && o.material.map.name,
            emissive: !!o.material.emissive && o.material.emissiveIntensity * o.material.emissive.getHex() > 0 }); });
          return out; })()""")
        size = os.path.getsize(GLB)
        print(f"loaded in {load['ms']:.0f} ms, first frame after it {load['frame']:.0f} ms; .glb {size / 1024:.0f} KB; meshes {json.dumps(m['meshes'])}")
        names = sorted(x['mat'] for x in m['meshes'])
        check(not load['failed'] and m['group'] and m['hidden'] and names == ['glass_dark', 'glass_lit', 'house', 'skirt', 'wing'],
              f"?blender: the model is in (house, wing, the panes and the skirt) and the boxes it stands for are hidden ({load['ms']:.0f} ms){' ' + load['failed'] if load['failed'] else ''}")
        lit = await ev("""(() => { const h = S47.blenderHouse, out = { glows: h.glowsAt.length, blinds: h.blinds ? h.blinds.i.length : 0, mats: {} };
          h.group.traverse((o) => { if (o.isMesh) out.mats[o.material.name] = { key: o.material.customProgramCacheKey ? o.material.customProgramCacheKey() : '', type: o.material.type,
            blend: o.material.blending, lamp: !!(o.material.userData && o.material.userData.lamps), emissiveMap: !!o.material.emissiveMap }; });
          return out; })()""")
        check(all('lamps' in lit['mats'][k]['key'] and 'floodsite' in lit['mats'][k]['key'] for k in ('house', 'wing')),
              'the walls take the lamps\' baked light (scaled by their floods) and keep the fake ones at a part of their strength')
        check(lit['glows'] == 6 and lit['blinds'] > 0, f"the wall lamps' glows made small ({lit['glows']}) and the lit windows' blinds moved into the reveals ({lit['blinds']} corners)")
        check(lit['mats']['glass_lit']['emissiveMap'] and 'flood' not in lit['mats']['glass_lit']['key'] and lit['mats']['skirt']['blend'] == 4,
              'the lit panes show their rooms (not lit by the floods), the skirt multiplies the ground')
        walls = [x for x in m['meshes'] if x['mat'] in ('house', 'wing')]
        check(all(x['ao'] and x['map'] == 'art/concrete' for x in walls) and any(x['emissive'] for x in m['meshes'] if x['mat'] == 'glass_lit'),
              'the walls carry the baked mask as aoMap on the second UV set and the concrete picture on the first; the lit panes glow')
        check(size < 2.5 * 1024 * 1024, f'the house .glb is {size / 1024:.0f} KB (both models together are to stay well under 5 MB)')
        # the truck's model, fetched and parsed the first time here (SARO's copy takes it), then off again
        tload = await ev("""(async () => { const t = performance.now(); await S47.blenderTruck.set(true);
          const ms = performance.now() - t, f = performance.now(); S47.tick(0.01);
          const r = { ms, frame: performance.now() - f, failed: S47.blenderTruck.failed, modelled: S47.world.saroTruck.modelled };
          await S47.blenderTruck.set(false); return r; })()""")
        check(not tload['failed'] and tload['modelled'], f"the truck's model loads ({tload['ms']:.0f} ms, the first frame after it {tload['frame']:.0f} ms){' ' + tload['failed'] if tload['failed'] else ''}")

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
                await ev("S47.blenderHouse.set(true); S47.tick(1 / 30)")
                await pg.screenshot(path=f'{OUT}/{v[0]}_{tod}_on.png'); on = await stats()
                await ev("S47.blenderTruck.set(true); S47.tick(1 / 30)"); both = await stats()
                await ev("S47.blender = false")
                calls[f'{v[0]}_{tod}'] = {'off': off, 'on': on, 'both': both}
        room = {}
        for flag in ('false', 'true'):
            await ev(f"S47.blender = {flag}; S47.game.clock = {NIGHT}; S47.world.enter('saro'); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, S47.room.spawn.yaw); S47.player.pitch = 0.02; S47.tick(0.4)")
            room[flag] = await stats()
        await pg.screenshot(path=f'{OUT}/a00_room_night_on.png')
        print('draw calls and triangles, off -> the house -> both models:')
        for k, c in calls.items(): print(f"  {k:24s} {c['off'][0]:4d} -> {c['on'][0]:4d} -> {c['both'][0]:4d}   {c['off'][1]:7d} -> {c['on'][1]:7d} -> {c['both'][1]:7d}")
        print(f"  {'a00_room (inside, both)':24s} {room['false'][0]:4d} -> {room['true'][0]:4d}   {room['false'][1]:7d} -> {room['true'][1]:7d}")
        more = [f"{k} {c['off'][0]} -> {c['on'][0]}" for k, c in calls.items() if c['on'][0] > c['off'][0]]
        check(not more, 'the model never costs a view a draw call more' + (': ' + ', '.join(more) if more else
              f" (from {min(c['off'][0] - c['on'][0] for c in calls.values())} to {max(c['off'][0] - c['on'][0] for c in calls.values())} fewer)"))
        more = [f"{k} {c['on'][0]} -> {c['both'][0]}" for k, c in calls.items() if c['both'][0] > c['on'][0]]
        check(not more, 'the truck model never costs one of these views a draw call more' + (': ' + ', '.join(more) if more else ''))
        g = max(c['both'][0] for k, c in calls.items() if k.split('_')[0] in GROUNDS)
        check(g <= 340, f'SARO outside within 340 on High with both models (grounds.py views, most {g})')
        check(room['true'][0] <= 300 and room['false'][0] <= 300, f"the control room within its 300 either way, both models ({room['false'][0]} -> {room['true'][0]})")

        # ---------- the doors, Low, Ultra ----------
        await ev("S47.blender = true; S47.doors.set('east', true, true); S47.doors.set('exit', true, true); S47.tick(0.1)")
        door = await ev("[S47.room.doorHinge.rotation.y, S47.annex.exitOpen, S47.doors.isOpen('east')]")
        check(abs(door[0] - math.pi / 2) < 0.01 and door[1] == 1 and door[2], 'the east door and the fire exit still swing open with the model in')
        await at(VIEWS[5]); await pg.screenshot(path=f'{OUT}/a06_east_door_night_on_open.png')
        await ev("S47.doors.set('east', false, true); S47.doors.set('exit', false, true)")
        await ev("S47.setQuality('low'); S47.tick(0.1)")
        await at(VIEWS[1]); low = await stats()
        await pg.screenshot(path=f'{OUT}/a02_wing_night_on_low.png')
        lowm = await ev("""(() => { const out = []; S47.blenderHouse.group.traverse((o) => { if (o.isMesh) out.push([o.material.type, !!o.material.aoMap, o.material.name]); }); return out; })()""")
        check(all(t == 'MeshLambertMaterial' for t, _, n in lowm if n != 'skirt') and sum(1 for _, a, n in lowm if a) == 2 and low[0] <= calls['a02_wing_night']['on'][0],
              f"Low: Lambert with the mask on the walls, the skirt as it was ({low[0]} draw calls)")
        await ev("S47.setQuality('ultra'); S47.tick(0.3)")
        for v in (VIEWS[1], VIEWS[3]):
            await at(v); await ev("S47.tick(0.2)")
            await pg.screenshot(path=f'{OUT}/{v[0]}_night_on_ultra.png')
            await ev("S47.blender = false; S47.tick(0.05)")
            await pg.screenshot(path=f'{OUT}/{v[0]}_night_off_ultra.png')
            await ev("S47.blender = true")
        shadow = await ev("""(() => { let n = 0, c = 0; S47.blenderHouse.group.traverse((o) => { if (o.isMesh && o.material.name !== 'skirt') { n++; if (o.castShadow && o.receiveShadow) c++; } }); return [c, n]; })()""")
        check(shadow[0] == shadow[1] == 4, f'Ultra: the model casts and takes the real lamps\' shadows ({shadow[0]} of {shadow[1]}; the skirt is a shade itself)')
        await ev("S47.setQuality('high')")

        # ---------- the truck ----------
        await ev(f"S47.blender = false; S47.game.clock = {NIGHT}; S47.tick(0.1)")
        # every copy of it: SARO's (the pad, the motel's lot), STATION 01's, the road's (the diner's lot), the old road's
        await ev("window.__p = Promise.all([S47.world.prepare('station01'), S47.world.prepare('diner'), S47.world['ensureOldRoad']()])")
        for _ in range(600):
            if await ev("!!S47.world.site && !!S47.world.diner && !!S47.world.oldTruck && !!S47.world['parked'] && !!S47.world['truck']"): break
            await pg.wait_for_timeout(100)
        TR = "[S47.world.saroTruck, S47.world['parked'], S47.world['truck'], S47.world.oldTruck]"
        d = await ev(f"(() => {{ const ts = {TR}; return [ts.every((t) => S47.blenderTruck.trucks.has(t)), S47.blenderTruck.trucks.size, ts.some((t) => t.modelled)]; }})()")
        check(d == [True, 4, False], f'the four copies of the truck are handed to the pilot as they are built, none modelled while it is off ({d})')
        await ev("S47.blenderTruck.set(true)")
        tm = await ev(f"""(() => {{ const out = [];
          for (const [i, t] of {TR}.entries()) {{
            const meshes = t['modelMeshes'] ?? [], set = t['floods'];
            const ch = (m, g) => !!m.aoMap && !!g.attributes[m.aoMap.channel ? 'uv' + m.aoMap.channel : 'uv'];
            const keep = new Set([t.m.head, t.m.park, t.m.tail, t.m.rev, t.m.mirror, t.m.decal, t.m.beacon]);
            const kept = [...t.body.children, ...t.shell.children].filter((c) => keep.has(c.material));
            const lit = meshes.filter((x) => x.name !== 'blender:shadow'), shade = meshes.find((x) => x.name === 'blender:shadow');
            out.push({{ i, modelled: t.modelled, n: meshes.length, shown: meshes.every((x) => x.visible), hidden: t['replaced'].every((o) => !o.visible),
              floods: lit.every((x) => x.material.customProgramCacheKey().includes('flood' + set.key)),
              shade: !!shade && shade.parent === t.group && shade.material.blending === 4 && !!shade.material.map,
              chrome: lit.filter((x) => x.name.endsWith('_chrome')).every((x) => x.material.customProgramCacheKey().includes('skychrome')),
              glass: lit.filter((x) => x.name.endsWith('_glass')).every((x) => x.material.customProgramCacheKey().includes('skyglass') && x.material.opacity < 0.7),
              ao: meshes.filter((x) => !x.material.transparent).every((x) => ch(x.material, x.geometry)),
              paintMap: meshes.filter((x) => x.name.endsWith('_paint')).every((x) => x.material.map === t.m.paint.map),
              wheel: t.wheels.geometry !== t['codeWheel'] && ch(t.wheels.material, t.wheels.geometry) && t.wheels.material.vertexColors,
              kept: kept.length, keptShown: kept.every((c) => c.visible), glow: t['glow'].points.visible,
              tris: lit.reduce((a, x) => a + x.geometry.index.count / 3, 0) + 4 * t.wheels.geometry.index.count / 3 }});
          }}
          return out; }})()""")
        tsize = os.path.getsize(TRUCK_GLB)
        print(f"truck .glb {tsize / 1024:.0f} KB; {json.dumps(tm)}")
        check(all(x['modelled'] and x['n'] == 8 and x['shown'] and x['hidden'] for x in tm),
              'every copy of the truck shows the model (body, cab, glass, its shade on the ground) and hides the panels it stands for')
        check(all(x['shade'] and x['chrome'] and x['glass'] for x in tm), 'each copy lays its baked shade on the ground with it; its chrome and glass take the sky')
        check(all(x['floods'] for x in tm), "each copy's model is lit by its own area's floods (site, field, road, old road)")
        check(all(x['ao'] and x['paintMap'] and x['wheel'] for x in tm), 'the mask is the aoMap on the UV set it was baked on, the paint keeps its picture, the instances turn the modelled wheel')
        check(all(x['kept'] == 8 and x['keptShown'] and x['glow'] for x in tm), 'the lenses, decals, plates, mirror glass, beacon and glow stay the code\'s, and shown')
        check(tsize + os.path.getsize(GLB) < 5 * 1024 * 1024, f'the truck .glb is {tsize / 1024:.0f} KB, both models {(tsize + os.path.getsize(GLB)) / 1024:.0f} KB (under 5 MB)')

        # the rays meet the same things on the pad
        await ev("S47.blenderTruck.set(false); S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1)")
        trays = """(() => { const T = S47.three, rc = new T.Raycaster(), g = S47.world.saroTruck.group, out = [];
          S47.scene.updateMatrixWorld(true);
          const h = g.rotation.y, c = Math.cos(h), s = Math.sin(h), at = (x, y, z) => new T.Vector3(g.position.x + x * c + z * s, g.position.y + y, g.position.z - x * s + z * c);
          for (const [x, y, z] of [[-5, 1.0, 0], [5, 0.9, -1.5], [0, 0.8, -7], [0, 1.1, 7], [-5, 0.8, 1.0], [-4, 1.6, -0.3]]) {
            const o = at(x, y, z), to = at(0, y, z === 0 || Math.abs(x) > 1 ? z : 0); rc.set(o, to.sub(o).normalize()); rc.far = 12;
            const hit = rc.intersectObject(g, true)[0]; out.push(hit ? (hit.object.name || hit.object.type) + '@' + hit.distance.toFixed(3) : '-'); }
          return out; })()"""
        r_off = await ev(trays)
        await ev("S47.blenderTruck.set(true)"); r_on = await ev(trays)
        check(r_off == r_on and r_on.count('-') < len(r_on), f'the rays meet the same things with the truck model in: {r_on}')

        # parked: on the pad at night, on the motel's lot, at the station, on the diner's lot
        def frame_js(place, dx, dy, dz, ly):
            who = {'saro': 'S47.world.saroTruck', 'motel': 'S47.world.saroTruck', 'station01': "S47.world['parked']", 'diner': "S47.world['truck']"}[place]
            return f"""(() => {{ const g = {who}.group, cam = S47.camera; g.updateMatrixWorld();
              const h = g.rotation.y, c = Math.cos(h), s = Math.sin(h);
              const x = g.position.x + {dx} * c + {dz} * s, z = g.position.z - {dx} * s + {dz} * c;
              S47.player.place(x, z, 0);
              const step = S47.player.update.bind(S47.player); S47.player.update = () => {{}};
              window.__fov ??= cam.fov; cam.fov = 50; cam.updateProjectionMatrix();
              cam.position.set(x, g.position.y + {dy}, z); cam.lookAt(g.position.x, g.position.y + {ly}, g.position.z);
              const p = cam.position.clone(), q = cam.quaternion.clone(); S47.tick(0.05); cam.position.copy(p); cam.quaternion.copy(q); S47.player.update = step; }})()"""
        async def truck_view(name, place, dx, dy, dz, ly, tod='night', store=None):
            area = {'saro': 'saro', 'motel': 'saro', 'station01': 'station01', 'diner': 'diner'}[place]
            await ev(f"S47.world.placeTruck('{place}'); S47.world.enter('{area}'); S47.tick(0.2)")
            frame = frame_js(place, dx, dy, dz, ly)
            out = {}
            for flag in ('false', 'true'):
                await ev(f"S47.blenderTruck.set({flag})"); await ev(frame)
                await pg.screenshot(path=f"{OUT}/{name}_{tod}_{'on' if flag == 'true' else 'off'}.png")
                out['on' if flag == 'true' else 'off'] = await stats()
            await ev("S47.camera.fov = window.__fov; S47.camera.updateProjectionMatrix()")
            (tcalls if store is None else store)[f'{name}_{tod}'] = out
        tcalls = {}
        for v in TRUCK_PARKED: await truck_view(*v)
        await ev(f"S47.game.clock = {DAWN}; S47.tick(0.1)")
        await truck_view('t01_pad_front34', 'saro', -4.5, 1.4, -5.5, 0.9, 'dawn')
        await ev(f"S47.game.clock = {NIGHT}; S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1)")

        # driving on the highway at night: the cab, the wheels, the lights, the needles
        await ev("S47.blenderTruck.set(true); S47.world.freeDrive('road')"); await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'road' && S47.world.driving")
        await ev("S47.hold = true; document.querySelectorAll('.toast').forEach((t) => t.remove()); S47.world.testInput = { steer: 0, throttle: 0.6 }; S47.tick(3)")
        for flag in ('false', 'true'):
            await ev(f"S47.blenderTruck.set({flag}); S47.tick(1 / 60)")
            await pg.screenshot(path=f"{OUT}/t08_cab_night_{'on' if flag == 'true' else 'off'}.png")
            tcalls.setdefault('t08_cab_night', {})['on' if flag == 'true' else 'off'] = await stats()
        live = await ev("""(() => { const t = S47.world['truck'], T = S47.three, m = new T.Matrix4(), q = new T.Quaternion(), p = new T.Vector3(), s = new T.Vector3(), e = new T.Euler();
          const wheels = () => [0, 1, 2, 3].map((i) => { t.wheels.getMatrixAt(i, m); m.decompose(p, q, s); e.setFromQuaternion(q, 'YXZ'); return [e.y, e.x]; });
          const needle = () => { t['needles'].getMatrixAt(0, m); return m.elements.slice(0, 2); };
          S47.world.testInput = { steer: 0.8, throttle: 0.5 }; S47.tick(0.6);
          const a = wheels(), n0 = needle(); S47.tick(0.25); const b = wheels(), n1 = needle();
          const r = { spin: [0, 1, 2, 3].map((i) => Math.abs(b[i][1] - a[i][1]) > 1e-3), steer: Math.abs(Math.abs(b[0][0] - b[2][0])) > 0.05 && Math.abs(Math.abs(b[1][0] - b[3][0])) > 0.05,
            wheelSpin: t['wheelSpin'].rotation.z, beam: t.beam, head: t.m.head.color.r, needles: n0[0] !== n1[0] || n0[1] !== n1[1] || t['gauge'].speed > 0.05,
            cab: t.cab.visible, shell: t.shell.visible, body: t['modelMeshes'].filter((x) => x.parent === t.body).every((x) => x.visible && x.parent.visible),
            radio: t.radioHit.parent === t.cab };
          S47.world.testInput = { steer: 0, throttle: -1 }; S47.tick(0.4); r.brake = t.m.tail.color.r;
          return r; })()""")
        print('driving with the model:', json.dumps(live))
        check(all(live['spin']) and live['steer'] and abs(live['wheelSpin']) > 0.5, f"the modelled wheels turn and the front ones steer, the steering wheel with them ({live['wheelSpin']:.2f})")
        check(live['beam'] > 0.5 and live['head'] > 1 and live['brake'] > 0.5, f"the headlights and the brake lights light ({live['head']:.2f}, {live['brake']:.2f})")
        check(live['needles'] and live['cab'] and not live['shell'] and live['body'] and live['radio'],
              'while driving: the needles move, the cab is in and its radio, the shell (and the model\'s) out of the way, the modelled hood seen')
        # the truck in its own headlights, stopped, from in front
        await ev("S47.world.testInput = { steer: 0, throttle: 0 }; S47.tick(2.5)")
        hl = """(() => { const g = S47.world['truck'].group, cam = S47.camera; S47.world['truck'].setDriving(false);
          const h = g.rotation.y, c = Math.cos(h), s = Math.sin(h), step = S47.world['drivingStep']; S47.world['drivingStep'] = () => {};
          cam.position.set(g.position.x - 3.2 * c - 6.5 * s, g.position.y + 1.25, g.position.z + 3.2 * s - 6.5 * c); cam.lookAt(g.position.x, g.position.y + 0.8, g.position.z);
          S47.tick(0.02); S47.world['drivingStep'] = step; })()"""
        for flag in ('false', 'true'):
            await ev(f"S47.blenderTruck.set({flag})"); await ev(hl)
            await pg.screenshot(path=f"{OUT}/t09_headlights_night_{'on' if flag == 'true' else 'off'}.png")
            tcalls.setdefault('t09_headlights_night', {})['on' if flag == 'true' else 'off'] = await stats()
        await ev("S47.world.endFreeDrive(); S47.tick(0.1)")
        # the old road at dawn: the cab, and the truck from the road
        await ev(f"S47.game.clock = {DAWN + 2 * 60}; S47.world.freeDrive('oldroad')"); await ev("S47.hold = false")
        await pg.wait_for_function("S47.world.area === 'roswell' && S47.world.driving")
        await ev("S47.hold = true; document.querySelectorAll('.toast').forEach((t) => t.remove()); S47.world.testInput = { steer: 0, throttle: 0.5 }; S47.tick(2)")
        for flag in ('false', 'true'):
            await ev(f"S47.blenderTruck.set({flag}); S47.tick(1 / 60)")
            await pg.screenshot(path=f"{OUT}/t10_cab_dawn_{'on' if flag == 'true' else 'off'}.png")
            tcalls.setdefault('t10_cab_dawn', {})['on' if flag == 'true' else 'off'] = await stats()
        await ev("S47.world.testInput = { steer: 0, throttle: 0 }; S47.tick(2.5)")
        od = """(() => { const g = S47.world.oldTruck.group, cam = S47.camera; S47.world.oldControl = { camera: () => {} };
          const h = g.rotation.y, c = Math.cos(h), s = Math.sin(h);
          cam.position.set(g.position.x - 4.5 * c - 6 * s, g.position.y + 1.5, g.position.z + 4.5 * s - 6 * c); cam.lookAt(g.position.x, g.position.y + 0.9, g.position.z); S47.tick(0.02); })()"""
        for flag in ('false', 'true'):
            await ev(f"S47.blenderTruck.set({flag})"); await ev(od)
            await pg.screenshot(path=f"{OUT}/t11_oldroad_dawn_{'on' if flag == 'true' else 'off'}.png")
            tcalls.setdefault('t11_oldroad_dawn', {})['on' if flag == 'true' else 'off'] = await stats()
        await ev("S47.world.oldControl = null; S47.world.endFreeDrive(); S47.tick(0.1)")
        await ev(f"S47.game.clock = {NIGHT}; S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.1)")
        print('the truck: draw calls and triangles, off -> on:')
        for k, c in tcalls.items(): print(f"  {k:28s} {c['off'][0]:4d} -> {c['on'][0]:4d}   {c['off'][1]:7d} -> {c['on'][1]:7d}")
        more = [f"{k} {c['off'][0]} -> {c['on'][0]}" for k, c in tcalls.items() if c['on'][0] > c['off'][0]]
        check(not more, 'the truck model never costs a view a draw call more' + (': ' + ', '.join(more) if more else
              f" (from {min(c['off'][0] - c['on'][0] for c in tcalls.values())} to {max(c['off'][0] - c['on'][0] for c in tcalls.values())} fewer)"))
        # with both models in: the truck's views at SARO within the 340
        await ev("S47.blenderHouse.set(true)")
        both = {}
        for v in [x for x in TRUCK_PARKED if x[1] in ('saro', 'motel')]:
            await truck_view(*v, tod='both', store=both)
        most = max(c['on'][0] for c in both.values())
        print('  with the house in too: ' + ', '.join(f"{k} {c['on'][0]}" for k, c in both.items()))
        check(most <= 340, f'SARO outside within 340 on High with both models, the truck in view (most {most})')
        await ev("S47.blenderHouse.set(false)")
        # Low: Lambert with the mask; Ultra: shadows
        await ev("S47.setQuality('low'); S47.tick(0.1)")
        await truck_view('t01_pad_front34', 'saro', -4.5, 1.4, -5.5, 0.9, 'low')
        lowt = await ev("""(() => { const t = S47.world.saroTruck, ms = [...t['modelMeshes'].filter((x) => x.name !== 'blender:shadow').map((x) => x.material), t.wheels.material];
          return [ms.every((m) => m.type === 'MeshLambertMaterial'), ms.filter((m) => m.aoMap).length]; })()""")
        check(lowt[0] and lowt[1] == 7 and tcalls['t01_pad_front34_low']['on'][0] <= tcalls['t01_pad_front34_low']['off'][0],
              f"Low: Lambert with the mask on the truck too ({lowt}, {tcalls['t01_pad_front34_low']['off'][0]} -> {tcalls['t01_pad_front34_low']['on'][0]} draw calls)")
        await ev("S47.setQuality('ultra'); S47.tick(0.3)")
        await truck_view('t01_pad_front34', 'saro', -4.5, 1.4, -5.5, 0.9, 'ultra')
        # Ultra draws its shadow maps again a few times a second (core/ultra.ts SHADOW_HZ), so one
        # frame against another says little: a dozen frames each way, those without the shadow
        # maps and those with them
        ultra, fj = {}, frame_js('saro', -4.5, 1.4, -5.5, 0.9).replace('S47.tick(0.05)', 'S47.tick(1 / 30)')
        for flag in ('false', 'true'):
            await ev(f"S47.blenderTruck.set({flag})")
            ultra[flag] = []
            for _ in range(12):
                await ev(fj); ultra[flag].append(await stats())
        await ev("S47.camera.fov = window.__fov; S47.camera.updateProjectionMatrix()")
        uo, un = sorted(c for c, _ in ultra['false']), sorted(c for c, _ in ultra['true'])
        tcalls['t01_pad_front34_ultra'] = {'off': [uo[0], 0], 'on': [un[0], 0], 'off_shadows': [uo[-1], 0], 'on_shadows': [un[-1], 0]}
        print(f"  Ultra, the pad, a dozen frames: without the shadow maps {uo[0]} -> {un[0]}, with them {uo[-1]} -> {un[-1]}")
        check(un[0] <= uo[0] and un[-1] <= uo[-1], f'Ultra: no draw call more either, with or without the shadow maps drawn ({uo[0]} -> {un[0]}, {uo[-1]} -> {un[-1]})')
        tsh = await ev("""(() => { const t = S47.world.saroTruck, ms = t['modelMeshes'].filter((x) => !x.material.transparent);
          return [ms.filter((x) => x.castShadow && x.receiveShadow).length, ms.length, t.wheels.castShadow]; })()""")
        check(tsh[0] == tsh[1] == 6 and tsh[2], f'Ultra: the truck model casts and takes the real lamps\' shadows ({tsh})')
        await ev("S47.setQuality('high'); S47.blender = false; S47.tick(0.1)")

        # ---------- the phone: 844 by 390, the size Tom judges it on ----------
        # the wing's two faces and the truck on its pad at night, both models off and on
        await pg.set_viewport_size({'width': 844, 'height': 390}); await pg.wait_for_timeout(300)
        await ev(f"S47.game.clock = {NIGHT}; S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.2)")
        phone = {}
        for v in (VIEWS[1], VIEWS[4], VIEWS[0]):
            for flag in ('false', 'true'):
                await ev(f"S47.blender = {flag}"); await at(v)
                await pg.screenshot(path=f"{OUT}/phone_{v[0]}_night_{'on' if flag == 'true' else 'off'}.png")
                phone.setdefault(v[0], {})['on' if flag == 'true' else 'off'] = await stats()
        for name, dx, dy, dz, ly in (('t01_pad_front34', -4.5, 1.4, -5.5, 0.9), ('t03_pad_rear34', -3.2, 1.7, 5.4, 0.9)):
            for flag in ('false', 'true'):
                await ev(f"S47.blender = {flag}"); await ev(frame_js('saro', dx, dy, dz, ly))
                await pg.screenshot(path=f"{OUT}/phone_{name}_night_{'on' if flag == 'true' else 'off'}.png")
                phone.setdefault(name, {})['on' if flag == 'true' else 'off'] = await stats()
            await ev("S47.camera.fov = window.__fov; S47.camera.updateProjectionMatrix()")
        await ev("S47.blender = false"); await pg.set_viewport_size({'width': W, 'height': H}); await pg.wait_for_timeout(300)
        print('the phone (844x390), draw calls off -> on: ' + ', '.join(f"{k} {c['off'][0]} -> {c['on'][0]}" for k, c in phone.items()))
        more = [f"{k} {c['off'][0]} -> {c['on'][0]}" for k, c in phone.items() if c['on'][0] > c['off'][0]]
        check(not more, 'on the phone too, the models never cost a view a draw call more' + (': ' + ', '.join(more) if more else ''))
        json.dump({'phone': phone, 'calls': calls, 'room': room, 'truck_calls': tcalls, 'truck_both': both, 'load_ms': load['ms'], 'first_frame_ms': load['frame'], 'glb_bytes': size,
                   'truck_load_ms': tload['ms'], 'truck_first_frame_ms': tload['frame'], 'truck_glb_bytes': tsize, 'truck': tm},
                  open(f'{OUT}/numbers.json', 'w'), indent=1)

        # ---------- ?blender in the address ----------
        pg2 = await (await b.new_context(viewport={'width': 640, 'height': 400})).new_page(); pg2.set_default_timeout(300000)
        pg2.on('console', lambda m: errs.append(f'(?blender) {m.type}: {m.text}') if m.type in ('error', 'warning') else None)
        pg2.on('pageerror', lambda e: errs.append('(?blender) PAGEERROR: ' + str(e)))
        await pg2.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
        await pg2.goto(URL + ('&' if '?' in URL else '?') + 'blender')
        await pg2.wait_for_selector('button[data-a=start]'); await pg2.click('button[data-a=start]'); await pg2.wait_for_timeout(400)
        await pg2.evaluate("S47.hold = true; S47.jump('chapter1'); S47.tick(0.5)")
        url_on = await pg2.evaluate("""(async () => { const t = performance.now();
          while ((!S47.blenderHouse.shown || !S47.blenderTruck.shown || !S47.world.saroTruck?.modelled) && !S47.blenderHouse.failed && !S47.blenderTruck.failed
            && performance.now() - t < 20000) await new Promise((r) => setTimeout(r, 20));
          return [S47.blender, S47.blenderHouse.shown, S47.blenderHouse.replaced.every((o) => !o.visible), S47.blenderTruck.shown, !!S47.world.saroTruck?.modelled]; })()""")
        check(url_on == [True, True, True, True, True], f'?blender in the address shows both models from the start ({url_on})')
        await b.close()

    # ---------- the side-by-side pictures ----------
    try:
        from PIL import Image, ImageDraw
        pairs = [(v, tod) for v in [x[0] for x in VIEWS] for tod in ('night', 'dawn')]
        pairs += [(v[0], 'night') for v in TRUCK_PARKED] + [('t01_pad_front34', 'dawn'), ('t01_pad_front34', 'low'), ('t01_pad_front34', 'ultra')]
        pairs += [('t08_cab', 'night'), ('t09_headlights', 'night'), ('t10_cab', 'dawn'), ('t11_oldroad', 'dawn')]
        pairs += [(f'phone_{v}', 'night') for v in ('a02_wing', 'a05_wing_back', 'a01_south', 't01_pad_front34', 't03_pad_rear34')]
        for v, tod in pairs:
            if True:
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
