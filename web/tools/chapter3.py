# Chapter three, "The Survey Station", played through in a headless browser: the truck at
# SARO, the drive south, STATION 01 with P06 to P09 in the Unity order rules (wrong answers
# and the Unity replies included), both field photographs from the real camera, Nora on
# the field telephone, Continue at the station, the way back, developing the field roll at
# the SARO wet bench, and the end card. Starts from S47.jump('chapter3').
# Usage: python3 tools/chapter3.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
IDB_GET = """((store, key) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).get(key);
    g.onsuccess = () => { ok(g.result ?? null); r.result.close(); }; g.onerror = () => ok(null); }; r.onerror = () => ok(null); }))"""
# Stand `d` metres from a station proxy, on walkable ground, facing it. Tries directions
# round the target, starting from the side the player arrives on.
STAND = """((key, d, dy) => { const s = S47.world.site, p = S47.player, r = p.radius + 0.02;
  const t = s.proxies[key].getWorldPosition(S47.camera.position.clone());
  const hut = s.hutBounds, inHut = (x, z) => x > hut.minX && x < hut.maxX && z > hut.minZ && z < hut.maxZ;
  const free = (x, z) => p.walkable(x, z) && !p.colliders.some((c) => x > c.minX - r && x < c.maxX + r && z > c.minZ - r && z < c.maxZ + r)
    && inHut(x, z) === inHut(t.x, t.z);   // the same side of the hut walls as the thing itself
  const a = s.anchors.arrive; const base = Math.atan2(a.x - t.x, a.z - t.z);
  for (let i = 0; i < 24; i++) {
    const ang = base + (i % 2 ? 1 : -1) * Math.ceil(i / 2) * Math.PI / 12;
    for (const k of [1, 0.8, 1.25, 0.6]) {
      const x = t.x + Math.sin(ang) * d * k, z = t.z + Math.cos(ang) * d * k;
      if (!free(x, z)) continue;
      p.place(x, z, Math.atan2(-(t.x - x), -(t.z - z)));
      p.pitch = Math.atan2((t.y + (dy || 0)) - p.eye, Math.hypot(t.x - x, t.z - z));
      S47.tick(0.05);
      return [x, z];
    }
  }
  return null; })"""
# An autopilot for the truck: aims at the next waypoint of road.route, slows down for bends,
# on gravel and before the gate. Positive steer turns right; heading h faces (-sin h, -cos h).
# An autopilot for the truck (the path follower from the drive preview): pure pursuit along
# road.route with a look-ahead that grows with speed, slow for the turn-off and the gate.
# Positive steer turns right; heading h faces (-sin h, -cos h).
AUTOPILOT = """(() => { const w = S47.world, road = w.road, drive = w.drive, R = road.route, o = road.group.position; let wp = 0;
  w.testInput = () => {
    const p = drive.pos;
    for (let i = wp; i < Math.min(R.length, wp + 20); i++) if (R[i].distanceToSquared(p) < R[wp].distanceToSquared(p)) wp = i;
    const ahead = 4.5 + Math.abs(drive.speed) * 0.5;
    let tg = R[R.length - 1];
    for (let i = wp + 1; i < R.length; i++) if (R[i].distanceTo(p) > ahead) { tg = R[i]; break; }
    let err = Math.atan2(-(tg.x - p.x), -(tg.z - p.z)) - drive.heading;
    err = Math.atan2(Math.sin(err), Math.cos(err));
    const lx = p.x - o.x, lz = p.z - o.z, dEnd = p.distanceTo(road.end.pos);
    const vT = lx > -7 ? (lz < 400 ? 25 : 5) : lx > -32 ? 4 : dEnd > 70 ? 13 : Math.max(0, (dEnd - 4) / 4);
    return { steer: Math.max(-1, Math.min(1, -err * 2.2)), throttle: Math.max(-1, Math.min(1, (vT - drive.speed) * 0.6)) };
  }; })()"""
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

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
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 11", timeout=120000)
        except Exception: pass
        await pg.evaluate("localStorage.clear()")
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
        ev = pg.evaluate
        async def tick(s): await ev(f"S47.tick({s})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def place(x, z, yaw, pitch=0.0): await ev(f"S47.player.place({x},{z},{yaw}); S47.player.pitch={pitch}; S47.tick(0.05)")
        async def aimed(): return await ev("(()=>{const it=S47.game.d.inter.update(S47.camera); return it ? it.id : null})()")
        async def label(i): return await ev(f"S47.game.d.inter.get('{i}').label()")
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use()")
        s3 = lambda k: ev(f"S47.ch3.s.{k}")
        async def toasts(): return await ev("document.querySelector('.toasts').textContent")
        async def say(): return await ev("(document.querySelector('.labpanel [data-say]')||{}).textContent || ''")
        async def objective(): return await ev("document.querySelector('.objective').textContent")
        async def stand(key, d, dy=0.0): return await ev(f"{STAND}('{key}', {d}, {dy})")
        async def walk(points):
            await place(points[0][0], points[0][1], 0.0)
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:])}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 1200) return false; }} }}
              return true; }})()''')
        async def close_panel():
            await pg.click('.labpanel .lp-close'); await tick(0.1)
        async def shutter():
            await ev("S47.ch1.toggleCamera()"); await tick(0.3)
            status = await ev("(document.querySelector('.viewfinder .vf-status')||{}).textContent || ''")
            await ev("S47.ch1.shutter()"); await tick(1.0)
            return status
        docs = lambda: ev("S47.game.docs.map(d => d.id)")

        # ---------- SARO: the truck ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter3'); S47.tick(1.0)")
        check(await ev("S47.game.phase") == 'ch3' and await s3('stage') == 'to-truck', 'chapter three starts: to the truck')
        check('TAKE THE SARO TRUCK' in await objective(), 'objective names the truck')
        check(await ev("!!S47.world.saroTruck && S47.world.saroTruck.group.visible"), 'the service truck stands on its pad')
        await place(9.6, 1.6, math.pi, -0.05)
        await shot('e01_landing_to_pad')
        check(await walk([(5.0, 1.6), (7.2, 1.6), (10.2, 2.0), (10.2, 4.2), (10.3, 8.6)]), 'walk out the east door, through the gap in the railing and down to the truck')
        await place(10.3, 8.6, math.pi / 2, -0.1)
        check(await aimed() == 'truck' and await label('truck') == 'Drive to STATION 01', 'the truck offers the drive')
        await shot('e02_truck')

        # ---------- the drive ----------
        await use('truck')
        for _ in range(40):
            await tick(0.25)
            if await ev("S47.world.driving"): break
        check(await ev("S47.world.driving && S47.world.area === 'road'"), 'the drive begins on the highway')
        check(await ev("!S47.world.saroTruck.group.visible && !S47.yard.group.visible"), 'SARO is hidden while away')
        await tick(1.2)
        await shot('e03_cab')
        check(await ev("S47.game.saveBlock()") is not None, 'no saving while driving')
        # The whole way with an autopilot along road.route: steer at the next waypoint, slow
        # for bends, the gravel and the gate. If it gets stuck, the truck is put down before
        # the gate instead, so the rest of the chapter is still tested.
        await ev(AUTOPILOT)
        shots = {'e04_highway': False, 'e04b_track': False, 'e04c_gate': False}
        best, still, t_drive = 1e9, 0, 0.0
        for _ in range(400):
            await tick(0.5); t_drive += 0.5
            if await ev("S47.world.area === 'station01'"): break
            st = await ev("(() => { const d = S47.world.drive, r = S47.world.road, e = r.route[r.route.length - 1]; return { mph: d.mph, kind: d.surface.kind, left: Math.hypot(e.x - d.pos.x, e.z - d.pos.z), calls: S47.renderer.info.render.calls }; })()")
            if not shots['e04_highway'] and st['mph'] > 25:
                shots['e04_highway'] = True; await shot('e04_highway'); print(f"highway at {st['mph']:.0f} mph, {st['calls']} draw calls", flush=True)
            if not shots['e04b_track'] and st['kind'] == 'gravel' and st['left'] < 520:
                shots['e04b_track'] = True; await shot('e04b_track'); print(f"survey track, {st['calls']} draw calls", flush=True)
            if not shots['e04c_gate'] and st['left'] < 45:
                shots['e04c_gate'] = True; await shot('e04c_gate'); print(f"at the gate, {st['calls']} draw calls", flush=True)
            if st['left'] < best - 1: best, still = st['left'], 0
            else: still += 0.5
            if still > 12: break
        drove = await ev("S47.world.area === 'station01'")
        check(drove, f'the autopilot drives the whole way to the gate ({t_drive:.0f} s of game time, {best:.0f} m left at worst)')
        check(all(shots.values()), 'highway, survey track and gate were all passed')
        if not drove:
            # roll into the end zone at the gate
            await ev("""(() => { const r = S47.world.road, z = r.endZone, e = r.end;
              const cx = (z.minX + z.maxX) / 2, cz = (z.minZ + z.maxZ) / 2;
              const back = 14, h = e.heading;
              S47.world.drive.place(S47.camera.position.clone().set(cx + Math.sin(h) * back, 0, cz + Math.cos(h) * back), h); })()""")
            await ev("S47.world.testInput = { steer: 0, throttle: 0.4 }")
            for _ in range(60):
                await tick(0.25)
                if await ev("S47.world.area === 'station01'"): break
                if await ev("S47.world.drive.mph > 9"): await ev("S47.world.testInput = { steer: 0, throttle: -0.3 }")
        await ev("S47.world.testInput = null")
        await tick(1.5)
        check(await ev("S47.world.area === 'station01' && !S47.world.driving"), 'arrived at STATION 01 and out of the truck')
        check(await s3('stage') == 'station' and await s3('arrived'), 'chapter three notes the arrival')
        art = await ev("S47.art()")
        check(all(i in art['loaded'] for i in art['later']) and len(art['later']) == 4
              and any(t['name'] == 'art/stucco' for t in art['textures']), 'the station brought its four surface images')
        await tick(0.5)
        await shot('e05_arrival')
        calls = await ev("S47.renderer.info.render.calls")
        print(f'draw calls at the arrival point: {calls}', flush=True)
        await pg.wait_for_function("S47.saves.list(S47.caseId()).some(m => m.place.startsWith('STATION 01'))", polling=300, timeout=20000)
        check(True, 'autosave at the station')

        # ---------- P06 before the photograph ----------
        check(await stand('transit', 1.6) is not None, 'the transit can be reached')
        check(await aimed() == 's1transit', 'the transit can be aimed at')
        await use('s1transit'); await tick(0.1)
        check(await s3('readE09a') and 'e09a' in await docs(), 'E09A read and filed')
        check('STATION 01 / FIELD RECORD' in await ev("document.querySelector('.labpanel h3').textContent"), 'the field record opens at the transit')
        await shot('e06_field_record')
        await pg.click('.labpanel [data-c=a]')
        check('FRAME 03 FIRST' in await say() and not await s3('p06'), 'P06 refused before FRAME 03')
        await close_panel()

        # ---------- FRAME 03 ----------
        spot = await ev("""(() => { const s = S47.world.site; const t = s.anchors.markerTarget; const p = S47.player;
          for (const sp of (s.anchors.photoSpots && s.anchors.photoSpots.marker ? [s.anchors.photoSpots.marker] : [])) return [sp.x, sp.z];
          return null; })()""")
        if spot:
            await ev(f"""(() => {{ const s = S47.world.site, t = s.anchors.markerTarget, p = S47.player;
              p.place({spot[0]}, {spot[1]}, Math.atan2(-(t.x - {spot[0]}), -(t.z - {spot[1]})));
              p.pitch = Math.atan2(t.y - p.eye, Math.hypot(t.x - {spot[0]}, t.z - {spot[1]})); S47.tick(0.05); }})()""")
        else:
            await ev("""(() => { const s = S47.world.site, t = s.anchors.markerTarget, p = S47.player, a = s.anchors.arrive;
              const base = Math.atan2(a.x - t.x, a.z - t.z);
              for (let i = 0; i < 24; i++) { const ang = base + (i % 2 ? 1 : -1) * Math.ceil(i / 2) * Math.PI / 12;
                const x = t.x + Math.sin(ang) * 7, z = t.z + Math.cos(ang) * 7;
                if (!p.walkable(x, z)) continue;
                p.place(x, z, Math.atan2(-(t.x - x), -(t.z - z))); p.pitch = Math.atan2(t.y - p.eye, 7); S47.tick(0.05); return; } })()""")
        status = await shutter()
        check('READY' in status and await ev("!!S47.ch3.s.f3"), f'FRAME 03 exposed: the empty footing and the moved post ({status.strip()})')
        await shot('e07_after_frame03')
        check(await ev("Object.keys(S47.ch3.s.f3.targets).length") >= 1, 'FRAME 03 records where the footing and post are on the film')

        # ---------- P06 ----------
        await stand('transit', 1.6); await use('s1transit'); await tick(0.1)
        await pg.click('.labpanel [data-c=b]')
        check('do not identify the moved fixed point' in await say() and await s3('wrong06') == 1, 'P06: B is answered with the Unity reply')
        await pg.click('.labpanel [data-c=a]')
        check(await s3('p06') and 'P06 SUPPORTED' in await ev("document.querySelector('.lp-page').textContent"), 'P06 recorded: A moved')

        # ---------- P08 before P07 ----------
        await pg.click('.labpanel [data-p=cable]')
        await pg.click('.labpanel [data-c=deliberate-cut]')
        check('REQUIRES BOTH' in await say(), 'P08 refused before the null test')
        await close_panel()

        # ---------- P07 ----------
        check(await stand('lamp', 1.4) is not None, 'the field lamp can be reached')
        check(await aimed() == 's1lamp', 'the field lamp can be aimed at')
        await use('s1lamp'); await tick(0.1)
        await pg.click('.labpanel [data-a=observe]')
        check('COVER THE LOCAL LAMP' in await say(), 'the null test needs the lamp covered')
        await pg.click('.labpanel [data-a=cover]'); await tick(0.2)
        check(await s3('lampCovered'), 'lamp covered')
        await pg.click('.labpanel [data-a=observe]')
        check(await s3('p07') and 'NULL TEST RECORDED' in await ev("document.querySelector('.lp-page').textContent"), 'P07 recorded')
        await close_panel()
        await shot('e08_lamp_covered')
        # take the hood off again, so the player can see the cable
        await use('s1lamp'); await tick(0.1)
        await pg.click('.labpanel [data-a=cover]'); await tick(0.2)
        check(not await s3('lampCovered') and await s3('p07'), 'lamp uncovered again, P07 stays recorded')
        await close_panel()

        # ---------- the cable and FRAME 04 ----------
        check(await stand('cable', 1.3, -0.3) is not None, 'the cable break can be reached')
        check(await aimed() == 's1cable', 'the cable break can be aimed at')
        await use('s1cable'); await tick(0.1)
        check(await s3('cableInspected'), 'cut faces inspected')
        await close_panel()
        await ev("""(() => { const s = S47.world.site, t = s.anchors.cableTarget, p = S47.player;
          p.pitch = Math.atan2(t.y - p.eye, Math.hypot(t.x - p.pos.x, t.z - p.pos.z));
          p.yaw = Math.atan2(-(t.x - p.pos.x), -(t.z - p.pos.z)); S47.tick(0.05); })()""")
        status = await shutter()
        check('READY' in status and await ev("!!S47.ch3.s.f4"), f'FRAME 04 exposed: the cut, close up ({status.strip()})')
        await shot('e09_cable')
        await use('s1cable'); await tick(0.1)
        await pg.click('.labpanel [data-c=author-guilt]')
        check('do not show who cut it' in await say(), 'P08: motive is answered')
        await pg.click('.labpanel [data-c=weathering]')
        check('opposed and clean' in await say() and await s3('wrong08') == 2, 'P08: weathering is answered with the Unity reply')
        await pg.click('.labpanel [data-c=deliberate-cut]')
        check(await s3('p08') and 'P08 SUPPORTED' in await ev("document.querySelector('.lp-page').textContent"), 'P08 recorded: a deliberate cut')
        await close_panel()

        # ---------- the hut: ledger and receiver ----------
        check(await stand('timingLog', 1.2) is not None, 'the ledger in the hut can be reached')
        await shot('e10_hut')
        check(await aimed() == 's1timing', 'the ledger can be aimed at')
        await use('s1timing'); await tick(0.1)
        check(await s3('readE10') and 'e10' in await docs(), 'E10 read and filed')
        await pg.click('.labpanel [data-c="02:17:47"]')
        check('LISTEN TO THE HUT RECEIVER' in await say(), 'P09 waits for the receiver')
        await close_panel()
        await stand('receiver', 1.2)
        check(await aimed() == 's1receiver', 'the receiver can be aimed at')
        await use('s1receiver')
        seen = ''
        for _ in range(12):
            await tick(1.0); seen += await toasts()
            if await s3('heard'): break
        check('Reference west. No. East. Hold the last reading.' in seen and await s3('heard'), 'the receiver gives the fragment, captioned')
        await stand('timingLog', 1.2); await use('s1timing'); await tick(0.1)
        await pg.click('.labpanel [data-c="02:17:00"]')
        check('47-second relay impact' in await say() and await s3('wrong09') == 1, 'P09: carrier time is answered with the Unity reply')
        await pg.click('.labpanel [data-c="02:17:47"]')
        check(await s3('p09') and await s3('stage') == 'call', 'P09 recorded: 02:17:47')
        await shot('e11_p09')
        await close_panel()

        # ---------- Nora ----------
        for _ in range(12):
            await tick(0.5)
            if await ev("S47.ch3.ringing"): break
        check(await ev("S47.ch3.ringing") and 'FIELD TELEPHONE' in await objective(), 'the field telephone rings')
        await stand('fieldPhone', 1.1)
        check(await aimed() == 's1phone' and await label('s1phone') == 'Answer the field telephone', 'the field telephone can be answered')
        await use('s1phone')
        seen = ''
        for _ in range(24):
            await tick(1.0); seen += await toasts()
            if await s3('called'): break
        check('N. VEGA' in seen and 'motel office' in seen and 'Room 6' in seen, 'Nora\'s lines are captioned')
        check(await s3('stage') == 'return' and 'DRIVE BACK' in await objective(), 'the field record is done: drive back')
        n = len([d for d in await docs() if d in ('p06', 'p07', 'p08', 'p09')])
        check(n == 4, 'P06 to P09 filed')

        # ---------- Continue at the station ----------
        await pg.wait_for_function("S47.saves.list(S47.caseId())[0].place.startsWith('STATION 01')", polling=300, timeout=20000)
        await pg.wait_for_timeout(400)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        check('STATION 01' in await ev("document.querySelector('.cont-info').textContent"), 'title screen offers to continue at STATION 01')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.world.area") == 'station01' and await s3('stage') == 'return' and await s3('p07') and await ev("S47.world.site.lampCovered === S47.ch3.s.lampCovered"), 'Continue brings back the station, the findings and the lamp as it was left')
        check(await ev("S47.ch3.s.f3.url.length > 50000 && S47.ch3.s.f4.url.length > 50000"), 'Continue brings back both field photographs')
        await shot('e12_continue_station')

        # ---------- back to SARO ----------
        check(await stand('truckSpot', 2.0) is not None, 'the truck at the station can be reached')
        check(await aimed() == 's1truck' and await label('s1truck') == 'Drive back to SARO', 'the truck offers the way back')
        await use('s1truck')
        for _ in range(20):
            await tick(0.5)
            if await ev("S47.world.area === 'saro' && !S47.game.cinematic"): break
        check(await ev("S47.world.area") == 'saro' and await s3('stage') == 'develop', 'back at SARO with the field roll')
        check('DEVELOP FRAMES 03 AND 04' in await objective(), 'objective: the wet bench')
        await shot('e13_back')

        # ---------- the wet bench ----------
        await ev("S47.player.place(15.1, -4.6, 0); S47.player.pitch = -0.5; S47.tick(0.1)")
        await use('wetBench'); await tick(0.1)
        check('FRAMES 03 + 04' in await ev("document.querySelector('.labpanel').textContent"), 'the wet bench takes the field roll')
        for step in range(3):
            await pg.click('.labpanel [data-a=act]'); await tick(2.6)
            if step < 2: await use('wetBench'); await tick(0.1)
        check(await s3('dev') == 3 and 'frame03' in await docs() and 'frame04' in await docs(), 'both field prints developed and filed')
        check(await ev("S47.yard.dryPrints[2].visible && S47.yard.dryPrints[3].visible"), 'the field prints hang on the drying line')
        await shot('e14_field_prints')
        await pg.click('.labpanel .lp-close'); await tick(1.2)
        await ev("S47.hold = false")
        await pg.wait_for_function("() => { const a = document.querySelector('.endcard .after'); return !!a && getComputedStyle(a).opacity === '1'; }", polling=500)
        text = await ev("document.querySelector('.endcard').textContent")
        check('THE SURVEY STATION' in text and 'ROOM 6' in text, 'end card for chapter three')
        await shot('e15_ending')
        await ev("S47.hold = true")

        # ---------- the case file ----------
        await pg.click('.endcard button:has-text("Return to the observatory")'); await pg.wait_for_timeout(400)
        await ev("S47.hold = true; S47.tick(0.3)")
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=case]')
        # S47.jump('chapter3') starts after a chapter one without photographs: frames 03 and 04
        ids = await ev("[...document.querySelectorAll('.notebook .thumbs button')].map(b => b.dataset.d)")
        check(ids == ['frame03', 'frame04'], f'case file shows both field photographs ({ids})')
        bright = await ev("""Promise.all(['frame03', 'frame04'].map((id) => new Promise((ok) => {
          const img = new Image(); img.onload = () => { const c = document.createElement('canvas'); c.width = 96; c.height = 60;
            const g = c.getContext('2d'); g.drawImage(img, 28, 28, 960, 600, 0, 0, 96, 60);
            const d = g.getImageData(0, 0, 96, 60).data; let s = 0; for (let i = 0; i < d.length; i += 4) s += d[i] + d[i + 1] + d[i + 2];
            ok(s / (d.length / 4) / 3); }; img.src = S47.ch3.s[id === 'frame03' ? 'f3' : 'f4'].url; })))""")
        print(f'mean brightness of the field photographs: {bright[0]:.0f} and {bright[1]:.0f} of 255', flush=True)
        check(all(b > 35 for b in bright), 'the flash lights both field photographs')
        await shot('e16_casefile')
        await ev("S47.game.d.ui.close(true)")

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-chapter3.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
