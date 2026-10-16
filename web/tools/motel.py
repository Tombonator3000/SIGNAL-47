# Sierra Motor Court 650 m down the highway (Tom, 15 and 16 October: further from the stations).
# Chapter four: the objective sends the player down the highway; on foot from SARO's west side
# along the shoulder to the motel's lot with the real movement code, the ground under the feet
# the same as the ground drawn (a ray down every 50 m); the place name, the map's cross, the
# footsteps and the music at the motel; the truck offers the drive there, parks on the lot by
# the office and is still there for chapter five; from the lot on south to the diner, out of
# SARO's area onto the road's 60 m past the motel. On the way (core/roadside.ts): the silence
# (no crickets, no wind, no footsteps for 47 s, then a note), headlights far off that come,
# stop and go out, someone on the motel's roof only while not looked at; in the truck, Nora on
# the CB (core/cb.ts). The notebook map. Pictures of the walk and the drive.
# Usage: python3 tools/motel.py OUTDIR [WxH]
import asyncio, sys, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append(bool(ok)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

STATE = """(() => { const w = S47.world, L = w.leg && w.legs[w.leg], D = L && L.drive;
  return { area: w.area, leg: w.leg, driving: w.driving, at: w.truckAt, x: D ? D.pos.x : 0, z: D ? D.pos.z : 0, mph: D ? D.mph : 0 }; })()"""
# the ground drawn under a point: a ray straight down through the scene's meshes
GROUND = """((x, z) => { const r = new S47.three.Raycaster(new S47.three.Vector3(x, 30, z), new S47.three.Vector3(0, -1, 0), 0, 60);
  const hits = r.intersectObjects(S47.scene.children, true).filter((h) => h.object.visible && h.object.isMesh && !h.object.isInstancedMesh
    && !(h.object.material && h.object.material.visible === false) && !String(h.object.name).startsWith('motel:'));
  const vis = (o) => { for (; o; o = o.parent) if (!o.visible) return false; return true; };
  const h = hits.find((h) => vis(h.object)); return h ? h.point.y : null; })"""

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
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def objective(): return await ev("document.querySelector('.objective').textContent")
        async def walk(points):
            return await ev(f'''(()=>{{const pts={points}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.15) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 6000) return [false, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter4')")
        for _ in range(40):
            await tick(0.25)
            if await ev("S47.game.phase === 'ch4' && S47.world.area === 'saro'"): break
        A = await ev("S47.world.motel.at")
        print(f'the motel stands at {A}', flush=True)
        o = await objective()
        check('DOWN THE HIGHWAY' in o and 'TRUCK' in o, f'chapter four: the objective sends the player down the highway ({o})')
        check(await ev("S47.game.notes.some((n) => n.includes('650 metres down the highway'))"), 'the note says where the motel is now')

        # ---------- on foot: from SARO's west side down the shoulder ----------
        await ev("S47.player.place(-24, 20, Math.PI); S47.player.pitch = -0.03; S47.tick(0.2)")
        await shot('m01_from_saro_south')
        ok = await walk([[-32.5, 40]])
        check(ok[0], f'onto the west shoulder at SARO ({ok})')
        bad, worst, n = [], 0.0, 0
        for z in range(90, 640, 50):
            ok = await walk([[-32.5, z]])
            if not ok[0]: bad.append(('stuck', z, ok)); break
            fy = await ev("S47.player.floorY")
            gy = await ev(f"{GROUND}(-32.5, {z})")
            n += 1
            if gy is None or abs(fy - gy) > 0.12: bad.append((z, round(fy, 3), gy))
            elif abs(fy - gy) > worst: worst = abs(fy - gy)
            if z == 340:
                await ev("S47.player.yaw = Math.PI; S47.player.pitch = -0.02; S47.tick(0.2)")
                await shot('m02_shoulder_halfway')
        check(not bad, f'down the shoulder {n} spots: the feet on the ground drawn (worst {worst:.3f} m) {bad[:4]}')
        await ev("S47.player.yaw = Math.PI - 0.25; S47.player.pitch = -0.04; S47.tick(0.2)")
        await shot('m03_motel_ahead')
        lot = [A['x'] - 33, A['z'] + 8]
        ok = await walk([[-32.5, A['z'] - 4], lot])
        fy = await ev("S47.player.floorY")
        check(ok[0] and abs(fy - A['y']) < 0.02, f"onto the motel's lot ({ok}, floor {fy:.2f}, lot {A['y']:.2f})")
        place, surf = await ev("S47.placeName()"), await ev("S47.surface()")
        check(place == 'Sierra Motor Court' and surf == 'concrete', f'the place is the motel and the lot sounds hard underfoot ({place}, {surf})')
        where = await ev("S47.mapAt()")
        check(where and where['spot'] == 'motel', f'the map has the cross at the motel ({where})')
        check(await ev("S47.musicWant()") == 'motel', 'the motel\'s own music out on its lot')
        await ev("S47.player.yaw = Math.PI / 2 + 0.3; S47.player.pitch = 0.02; S47.tick(0.3)")
        await shot('m04_on_the_lot')
        office = await ev("S47.world.court.anchors.officeInside")
        ok = await walk([[A['x'] - 37.5, A['z'] + 20], [A['x'] - 39.6, A['z'] + 24.1], [office['x'], office['z']]])
        check(ok[0] and await ev("S47.world.motel.interior.visible"), f'into the office ({ok})')
        out = await ev("S47.world.court.anchors.room6Outside")
        ok = await walk([[A['x'] - 39.6, A['z'] + 24.1], [A['x'] - 37.5, A['z'] + 26], [A['x'] - 37.5, out['z'] - 0.6], [out['x'], out['z']]])
        await ev(f"S47.player.yaw = {out['yaw']}; S47.player.pitch = 0.05; S47.tick(0.1)")
        aimed = await ev("(()=>{const it=S47.game.d.inter.update(S47.camera); return it ? it.id : null})()")
        check(ok[0] and aimed == 'room6Door', f'along the walk to the door of room 6 ({ok}, {aimed})')
        await ev("S47.player.yaw = -Math.PI / 2 - 0.2; S47.player.pitch = 0.04; S47.tick(0.2)")
        await shot('m05_from_room6_to_road')
        await ev("document.querySelector('.notebook .close')?.click()")
        await pg.keyboard.press('m'); await tick(0.3)
        src = await ev("document.querySelector('.nb-map img')?.src || ''")
        if src.startswith('data:image/png;base64,'):
            import base64
            open(f'{OUT}/m05b_map.png', 'wb').write(base64.b64decode(src.split(',', 1)[1]))
        await ev("document.querySelector('.notebook .close')?.click()"); await tick(0.2)

        # ---------- on the way (core/roadside.ts): the silence, the car, the roof ----------
        await ev("S47.player.place(-32.5, 120, Math.PI); S47.tick(0.3)")
        await ev("window.__steps = 0; const st = S47.audio.step.bind(S47.audio); S47.audio.step = (k) => { window.__steps++; return st(k); }")
        started = None
        for z in range(130, 400, 10):
            await walk([[-32.5, z]]); await tick(0.1)
            if await ev("S47.roadside.silent"): started = z; break
        check(started is not None and started > 250, f'down the shoulder the night goes silent (at site z {started})')
        await ev("window.__steps = 0")
        await walk([[-32.5, (started or 300) + 8]])
        check(await ev("window.__steps") == 0, 'in the silence the footsteps make no sound')
        await tick(48)
        check(not await ev("S47.roadside.silent") and await ev("S47.game.notes.some((n) => n.includes('No sound for 47 s'))"), 'after 47 s the sound comes back, and it is in the notebook')
        await walk([[-32.5, (started or 300) + 16]])
        check(await ev("window.__steps") > 0, 'and the footsteps are heard again')
        await ev("S47.roadside.carNow(S47.player.pos)"); await tick(3)
        c0 = await ev("S47.roadside.carState")
        await ev("S47.player.yaw = Math.PI; S47.player.pitch = 0; S47.tick(0.1)")
        await shot('m05c_headlights_far_off')
        await tick(10)
        c1 = await ev("S47.roadside.carState")
        check(c0 and c1 and c0['state'] == 'come' and c1['z'] < c0['z'] - 80 and c1['z'] > 1500, f'headlights far down the highway coming north ({c0}, {c1})')
        await tick(50)
        check(await ev("S47.roadside.carState") is None, 'they stop a long way off and go out; nothing arrives')
        await walk([[-32.5, A['z'] - 100]])
        await ev("S47.player.yaw = 0; S47.player.pitch = 0; S47.tick(0.1)")     # looking back north
        await ev("S47.roadside.roofNow()"); await tick(0.2)
        check(await ev("S47.roadside.roofUp"), 'someone on the motel roof while the player looks the other way')
        await ev("S47.player.yaw = Math.PI - 0.62; S47.tick(0.1)")              # half round: the roof at the edge of the view
        up = await ev("S47.roadside.roofUp")
        await shot('m05d_roof_corner_of_the_eye')
        await ev(f"(()=>{{const p=S47.player; p.yaw=Math.atan2(-({A['x']}-46.5-p.pos.x), -({A['z']}+31-p.pos.z)); S47.tick(0.1);}})()")
        check(not await ev("S47.roadside.roofUp"), f'looked at, it was not there (seen at the edge of the view first: {up})')

        # ---------- the truck: SARO's pad to the motel's lot ----------
        await ev("S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        await ev("S47.world.placeTruck('saro'); S47.world.enter('saro'); S47.tick(0.2)")
        lab = await ev("S47.world.truckChoice('saro').label")
        check(lab == 'Drive to Sierra Motor Court', f'chapter four: the truck offers the drive to the motel ({lab})')
        await ev("S47.world.truckChoice('saro').use()")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev("S47.world.autopilot = 13")
        seen, hint, cbline = [], False, ''
        for _ in range(200):
            await tick(0.5, 20)
            st = await ev(STATE)
            if st['leg'] and (not seen or seen[-1] != st['leg']): seen.append(st['leg'])
            toasts = await ev("document.querySelector('.toasts')?.textContent || ''")
            if not hint and 'lit sign on the right' in toasts: hint = True
            if not cbline and 'CB, N. VEGA' in toasts: cbline = toasts
            if st['driving'] and abs(st['z'] - (A['z'] - 60)) < 8 and not os.path.exists(f'{OUT}/m06_drive_in.png'):
                await shot('m06_drive_in')
            if not st['driving']: break
        st = await ev(STATE)
        tp = await ev("(() => { const g = S47.world.saroTruck.group.position; return [g.x, g.y, g.z]; })()")
        check(seen == ['saro'] and st['at'] == 'motel' and st['area'] == 'saro', f'the truck is left on the motel\'s lot, in SARO\'s area all the way ({seen}, {st})')
        check(A['x'] - 43.5 < tp[0] < A['x'] - 30 and A['z'] + 1 < tp[2] < A['z'] + 22 and abs(tp[1] - A['y']) < 0.05, f'by the office, on the lot ({tp})')
        check(hint, 'the hint says where to leave it')
        check('SARO seven' in cbline and await ev("S47.ch4.s.cb"), f'on the way Nora calls the truck on the CB ({cbline[:90]})')
        await tick(12)
        check(await ev("S47.game.notes.some((n) => n.includes(\"on the truck's CB\"))"), 'and the call is in the notebook')
        pp = await ev("[S47.player.pos.x, S47.player.pos.z, S47.player.floorY]")
        check(abs(pp[2] - A['y']) < 0.05 and math.hypot(pp[0] - tp[0], pp[1] - tp[2]) < 4, f'out by the door, on the lot ({pp})')
        await ev("S47.player.yaw = Math.atan2(-(%f - S47.player.pos.x), -(%f - S47.player.pos.z)); S47.player.pitch = -0.1; S47.tick(0.2)" % (tp[0], tp[2]))
        await shot('m07_truck_on_the_lot')
        kit = await ev("S47.kitItems().map((k) => k.name + ': ' + (k.state || ''))")
        check(any('Sierra Motor Court' in k for k in kit), f'the kit knows where the truck is ({kit})')
        lab = await ev("S47.world.truckChoice('saro').label")
        check(lab == 'Drive', f'with the truck already at the motel it offers a drive of the player\'s own ({lab})')

        # ---------- from the lot south: out of SARO's area onto the road's past the motel ----------
        await ev("S47.world.driveToDiner(() => {})")
        for _ in range(80):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        await ev("S47.world.autopilot = 14")
        seen, cross = [], None
        for _ in range(400):
            await tick(0.5, 20)
            st = await ev(STATE)
            if st['leg'] and (not seen or seen[-1] != st['leg']):
                if seen and st['leg'] == 'road': cross = st
                seen.append(st['leg'])
            if cross and st['leg'] == 'road' and st['z'] - cross['z'] > 15 and not os.path.exists(f'{OUT}/m08_past_the_change.png'):
                await shot('m08_past_the_change')
            if not st['driving']: break
        st = await ev(STATE)
        check(seen == ['saro', 'road'] and st['at'] == 'diner', f'on to the diner: SARO, then the road ({seen}, {st})')
        check(cross is not None and 470 < cross['z'] < 500, f'the change to the road\'s area is 60 m past the motel (road z {cross and round(cross["z"], 1)})')

        # ---------- coming north on the road: the motel ahead from the road's area ----------
        await ev("S47.world.freeDrive('road')")
        for _ in range(60):
            await tick(0.25)
            if await ev("S47.world.driving && S47.world.free === 'road'"): break
        await ev("(() => { const w = S47.world, L = w.legs.road; L.drive.place(new S47.three.Vector3(8001.8, L.area.height(8001.8, 560), 560), 0); w['leg'] = 'road'; w.enter('road'); })()")
        await tick(0.3)
        await shot('m09_north_from_the_road')
        await ev("S47.world.endFreeDrive()"); await tick(0.3)

        bad = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not bad, 'no console errors or warnings' + ('' if not bad else ': ' + '; '.join(bad[:4])))
        print(f'{sum(results)} of {len(results)} PASS')
        await b.close()
        sys.exit(0 if all(results) else 1)

asyncio.run(main())
