# Chapter one, "The Second Exposure", played through end to end in a headless browser.
# Panels are used with real clicks: marking a detail on a print, choosing the reference,
# the hypothesis, the B-12 method and the conclusion. The loop is held and advanced with
# S47.tick() so the run works with a software renderer.
# Usage: python3 tools/chapter1.py OUTDIR [passive|active] [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
# S47_NO_IDB=1 hides IndexedDB, to test that the photographs fall back to localStorage.
import asyncio, sys, json, os
from playwright.async_api import async_playwright

OUT = sys.argv[1]
METHOD = sys.argv[2] if len(sys.argv) > 2 else 'passive'
W, H = (int(v) for v in (sys.argv[3] if len(sys.argv) > 3 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
NO_IDB = os.environ.get('S47_NO_IDB') == '1'
# Reads one record from the game's IndexedDB (database 's47', version 2).
IDB_GET = """((store, key) => new Promise((ok) => { const r = indexedDB.open('s47', 2);
  r.onsuccess = () => { const g = r.result.transaction(store).objectStore(store).get(key);
    g.onsuccess = () => { ok(g.result ?? null); r.result.close(); }; g.onerror = () => ok(null); }; r.onerror = () => ok(null); }))"""
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
        if NO_IDB: await pg.add_init_script("Object.defineProperty(window, 'indexedDB', { value: undefined });")
        await pg.goto(URL)
        # Let the first page finish fetching its sounds. On the Pages build they are separate
        # files, and a reload in the middle aborts the fetch, so the old page logs a warning.
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
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use()")
        label = lambda i: ev(f"S47.game.d.inter.get('{i}').label()")
        stage = lambda: ev("S47.ch1.s.stage")
        async def say(): return await ev("(document.querySelector('[data-say]')||{}).textContent || ''")
        async def aim_at(px, pz, tx, ty, tz, dy=0.0):
            yaw = await ev(f"Math.atan2(-({tx}-({px})), -({tz}-({pz})))")
            pitch = await ev(f"Math.atan2({ty}-1.62, Math.hypot({tx}-({px}),{tz}-({pz})))")
            await place(px, pz, yaw, pitch + dy)
        async def walk(points):
            # Steer the player at each waypoint with forward input only. False if it gets stuck.
            await place(points[0][0], points[0][1], 0.0)
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:])}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 1200) return false; }} }}
              return true; }})()''')
        async def click_uv(sel, u, v):
            # click a point of the photograph inside a print image (border offsets as in FieldCamera.PRINT)
            box = await pg.locator(sel).first.bounding_box()
            px = box['x'] + (28 + u * 960) / 1016 * box['width']; py = box['y'] + (28 + v * 600) / 720 * box['height']
            await pg.mouse.click(px, py)

        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter1'); S47.tick(1.2)")
        check(await ev("S47.game.phase") == 'ch1' and await stage() == 'collect-camera', 'chapter one starts at the camera shelf')
        check('COLLECT THE CAMERA' in await ev("document.querySelector('.objective').textContent"), 'objective line shown')
        await shot('c01_start')

        # camera and door, aimed for real
        await place(4.7, 0.75, -1.5708, -0.55)
        check(await aimed() == 'fieldCamera', 'camera on the shelf can be aimed at')
        await use('fieldCamera'); await tick(0.2)
        check(await ev("S47.fcam.have") and await stage() == 'motor-log', 'field camera collected')
        await place(4.9, 1.6, -1.5708, 0.0)
        check(await aimed() == 'doorEast' and await label('doorEast') == 'Open service door', 'east door is unlocked')
        await use('doorEast'); await tick(1.0)
        await place(9.5, 2.4, 0.0, 0.0); await tick(0.1)
        check(abs(await ev("S47.player.pos.x") - 9.5) < 0.01, 'player can stand in the yard')
        await shot('c02_yard')

        # walking for real (zones and colliders, no teleport): room, door, yard, walk to B-12
        route = [(5.0, 1.6), (7.2, 1.6), (9.5, 1.2), (9.5, -9.0), (10.0, -15.0)]
        check(await walk(route), 'walk from the room through the east door and up the walk to B-12')

        # S-03 motor bus
        await place(11.2, -11.0, -1.5708, -0.2)
        check(await aimed() == 'motorBus', 'motor bus cabinet can be aimed at')
        await use('motorBus'); await tick(0.2); await shot('c03_s03_log')
        await pg.keyboard.press('KeyE'); await tick(0.2)
        check(await ev("S47.ch1.s.log") and await stage() == 'first-exposure', 'S-03 log read and filed')

        # FRAME 01 from the apron: first a refused frame, then the real one
        await place(9.0, -4.0, 0.0, 0.0)
        await pg.keyboard.press('KeyC'); await tick(0.1)
        check(await ev("S47.fcam.raised"), 'viewfinder raised with C')
        check('APRON' in await ev("document.querySelector('.vf-status').textContent"), 'viewfinder refuses the wrong place')
        await pg.keyboard.press('Space'); await tick(0.1)
        check(await ev("!S47.ch1.s.f1"), 'refused frame uses no film')
        dish = await ev("(()=>{const d=S47.ext.dishes.find(d=>d.id==='S-03'); const c=S47.ext.dishArray.feedPosition(d).lerp(new S47.camera.position.constructor(d.x, 12.4*d.scale, d.z), 0.5); return [c.x,c.y,c.z]})()")
        await aim_at(10.4, -11.6, dish[0], dish[1], dish[2], -0.06); await tick(0.1)
        check(await ev("document.querySelector('.vf-status').textContent") == 'REFERENCE IN FRAME // READY', 'S-03 framed from the apron')
        await shot('c04_viewfinder')
        await pg.keyboard.press('Space'); await tick(0.8)
        check(await ev("!!S47.ch1.s.f1") and await stage() == 'develop-first', 'frame 01 exposed')
        t1 = await ev("S47.ch1.s.f1.targets")
        check('vane' in t1 and 'echo' in t1, 'frame 01 holds the vane and the film-only echo')
        check(await ev("!S47.yard.echo.visible"), 'echo is hidden again after the exposure')

        # photo lab: door, wet bench in three steps
        await place(11.6, -1.2, -1.5708, 0.0)
        check(await aimed() == 'labDoor', 'lab door can be aimed at')
        await use('labDoor'); await tick(1.0)
        check(await walk([(10.0, -11.6), (9.6, -1.2), (12.0, -1.2), (14.2, -1.2), (15.0, -3.6)]), 'walk from the apron into the photo lab')
        await place(15.0, -4.3, 0.0, -0.45)
        check(await aimed() == 'wetBench', 'wet bench can be aimed at')
        await shot('c05_lab')
        for step, sel in [('load', '[data-a=act]'), ('transfer', '[data-a=act]'), ('collect', '[data-a=act]')]:
            await use('wetBench'); await tick(0.1)
            await pg.click(sel); await tick(2.6 if step != 'collect' else 0.3)
        check(await ev("S47.ch1.s.dev1") == 3 and await stage() == 'interpret-first', 'frame 01 developed and collected')
        check(await ev("S47.yard.dryPrints[0].visible"), 'print hangs on the drying line')
        await shot('c06_print01')
        await click_uv('.loupe img', 0.92, 0.12)
        check('No comparable' in await say(), 'a click on the sky is not a reference')
        await click_uv('.loupe img', t1['vane'][0], t1['vane'][1])
        check('DETAIL MARKED' in await say() and await ev("!!S47.ch1.s.mark1"), 'B-12 stripe marked on print 01')
        await pg.click('[data-a=close]'); await tick(0.2)

        # reference file at the archive bench
        await place(17.4, -3.0, -1.5708, -0.4)
        check(await aimed() == 'archive', 'archive bench can be aimed at')
        await use('archive'); await tick(0.1); await shot('c07_reference')
        await pg.click('[data-r="R-07"]')
        check('THREE HORIZONTAL BARS' in await say(), 'R-07 rejected')
        await pg.click('[data-r="B-12"]')
        check('B-12 selected' in await say(), 'B-12 matched')
        await pg.click('[data-h=command]')
        check('ZERO commands' in await say(), 'motor command hypothesis rejected')
        hyp = 'light' if METHOD == 'passive' else 'drift'
        await pg.click(f'[data-h={hyp}]')
        check(await stage() == 'choose-control', f'{hyp} hypothesis accepted')
        await pg.click('[data-a=close]'); await tick(0.2)

        # B-12 control
        await place(9.3, -16.9, 1.5708, -0.3)
        check(await aimed() == 'b12Control', 'B-12 control can be aimed at')
        await use('b12Control'); await tick(0.1); await shot('c08_b12_panel')
        await pg.click(f'[data-m={METHOD}]'); await tick(0.5)
        check(await label('b12Control') is None, 'vane still moving')
        await tick(1.6)
        check(await label('b12Control') == 'Note the direct observation', 'vane settled')
        yaw = await ev("S47.yard.vane.rotation.y * 180 / Math.PI")
        check(abs(yaw - (-60 if METHOD == 'passive' else 45)) < 0.5, f'vane at {yaw:.0f} degrees')
        await use('b12Control'); await tick(0.2)
        await pg.keyboard.press('KeyE'); await tick(0.2)
        check(await stage() == 'second-exposure', 'direct observation noted')

        # FRAME 02 from the sight line
        vane = await ev("(()=>{const v=S47.yard.vane.getWorldPosition(new S47.camera.position.constructor()); return [v.x,v.y,v.z]})()")
        await aim_at(10.0, -15.0, vane[0], vane[1], vane[2]); await tick(0.1)
        await pg.keyboard.press('KeyC'); await tick(0.1)
        check(await ev("document.querySelector('.vf-status').textContent") == 'REFERENCE IN FRAME // READY', 'B-12 framed from the sight line')
        await shot('c09_viewfinder2')
        await pg.keyboard.press('Space'); await tick(0.8)
        check(await ev("!!S47.ch1.s.f2") and await stage() == 'develop-second', 'frame 02 exposed')
        t2 = await ev("S47.ch1.s.f2.targets")

        # develop and compare
        await place(15.0, -4.3, 0.0, -0.45)
        for _ in range(3):
            await use('wetBench'); await tick(0.1)
            await pg.click('[data-a=act]'); await tick(2.6)
        check(await stage() == 'compare-exposures' and await ev("S47.yard.dryPrints[1].visible"), 'frame 02 developed')
        await shot('c10_compare')
        await pg.click('[data-c=second]')
        check('Mark the reference detail' in await say(), 'no conclusion before marking frame 02')
        await click_uv('.pair-img img', t2['echo'][0], t2['echo'][1])
        check(await ev("!!S47.ch1.s.mark2"), 'reference marked on frame 02')
        await pg.click('[data-c=lamp]')
        check(await ev("S47.ch1.s.wrongConcl") == 1, 'lamp conclusion rejected')
        await pg.click('[data-c=second]')
        check('FINDING RECORDED' in await say() and await stage() == 'file-report', 'second reference concluded')
        await pg.click('[data-a=close]'); await tick(0.2)

        # file the report
        await use('records'); await tick(0.1); await shot('c11_report')
        await pg.click('[data-a=file]'); await tick(1.2)
        check(await stage() == 'complete', 'local case filed')
        # No end card: a chapter card closes chapter one (naming the chosen test) and chapter
        # two begins behind it.
        await tick(1.2)
        check(await ev("!!document.querySelector('.chapter-card')"), 'chapter card after chapter one')
        text = await ev("document.querySelector('.chapter-card').textContent")
        check('LOCAL INCIDENT S-03 / B-12' in text and 'THE AMENDED RECORD' in text and ('shielded lamp' in text if METHOD == 'passive' else 'encoder drift' in text), 'chapter card names the test and chapter two')
        check(await pg.locator('.endcard').count() == 0, 'no end card between the chapters')
        await pg.wait_for_function("() => { const b = document.querySelector('.chapter-card .cc-b'); return !!b && getComputedStyle(b).opacity === '1'; }", polling=500)
        await shot('c12_chapter_card')
        await tick(7)
        check(await ev("!document.querySelector('.chapter-card:not(.out)')") and await ev("S47.game.phase") == 'ch2' and await ev("S47.ch2.ringing"), 'chapter two begins behind the card: the supervisor line rings')

        notes = await ev("S47.game.notes")
        docs = await ev("S47.game.docs.map(d => d.id)")
        for want in ['frame01', 'frame02', 's03log', 'refsheet', 'observation', 'finding']:
            check(want in docs, f'paper filed: {want}')
        check(any(n.startswith('LOCAL CASE CLOSED') for n in notes), 'notebook closes the case')

        # The autosave at chapter two holds the whole case. Photographs are stored once,
        # apart from the saves, and the save points at them.
        await pg.wait_for_function("S47.saves.list(S47.caseId()).some(m => m.chapter.startsWith('Chapter 2'))", polling=300, timeout=30000)
        meta = await ev("S47.saves.list(S47.caseId())[0]")
        check(meta['chapter'] == 'Chapter 2: The Amended Record' and len(meta['photos']) == (0 if NO_IDB else 2) and (meta['thumb'] or '').startswith('data:image/jpeg'), f"autosave {meta['id']}: chapter two, {len(meta['photos'])} photographs, a picture")
        if NO_IDB:
            raw = await ev(f"localStorage.getItem('s47.save.{meta['id']}') || ''")
            check('data:image/jpeg' in raw, 'without IndexedDB the save keeps its photographs inline in localStorage')
            print(f'save in localStorage: {len(raw) / 1024:.0f} kB', flush=True)
        else:
            rec = await ev(IDB_GET + f"('saves', '{meta['id']}')")
            size = len(json.dumps(rec['state']))
            check(rec['state']['case']['s']['f1']['url'].startswith('idb:p') and rec['state']['case']['s']['f2']['url'].startswith('idb:p') and size < 40000, f'the save points at its photographs and stays small ({size / 1024:.0f} kB)')
            key = rec['state']['case']['s']['f1']['url'][4:]
            stored = await ev(IDB_GET + f"('photos', '{key}')")
            check((stored or '').startswith('data:image/jpeg') and len(stored) > 50000, 'frame 01 is stored once, under its content key')

        # Continue from the title restores the finished case with both prints
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await stage() == 'complete' and await ev("S47.yard.dryPrints[0].visible && S47.yard.dryPrints[1].visible"), 'Continue restores the case and both prints')
        check(await ev("S47.game.docs.length") >= 8, 'Continue restores the filed papers')
        check(await ev("S47.ch1.s.f1.url.startsWith('data:image/jpeg') && S47.ch1.s.f1.url.length > 50000 && S47.ch1.s.f2.url.length > 50000"), 'Continue brings back both photographs, not placeholders')

        if not NO_IDB:
            # A photograph that cannot be read must not be lost (Codex's P1). The next two reads
            # from the photo store abort: frame 01 fails twice (the store retries once), frame 02 loads.
            original = await ev("S47.ch1.s.f1.url")
            await pg.add_init_script("""(() => {
              let left = +sessionStorage.getItem('s47.failreads') || 0;
              if (!left) return;
              const get = IDBObjectStore.prototype.get;
              IDBObjectStore.prototype.get = function (key) {
                const req = get.call(this, key);
                if (left > 0 && this.name === 'photos') { left--; sessionStorage.setItem('s47.failreads', String(left)); this.transaction.abort(); }
                return req;
              };
            })();""")
            await ev("sessionStorage.setItem('s47.failreads', '2')")
            await pg.reload()
            await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
            await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
            await ev("S47.hold = true; S47.tick(0.5)")
            check(await ev("S47.ch1.s.f1.url.length < 50000 && S47.ch1.s.f2.url.length > 50000"), 'unreadable photograph shows the stand-in print, the other one loads')
            m = await ev("S47.saveNow('manual', 0)")
            rec = await ev(IDB_GET + f"('saves', '{m['id']}')")
            check(rec['state']['case']['s']['f1']['url'] == 'idb:' + key, 'a save after the read error still points at the original photograph')
            check(await ev(IDB_GET + f"('photos', '{key}')") == original, 'the original photograph is still stored')
            await ev("sessionStorage.removeItem('s47.failreads')")
            await pg.reload()
            await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
            await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
            await ev("S47.hold = true; S47.tick(0.5)")
            check(await ev("S47.ch1.s.f1.url") == original, 'next Continue (the manual save) brings the original photograph back')

        # the case file spread in the notebook: both prints as pictures, the papers as cards
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=case]')
        check(await ev("document.querySelectorAll('.notebook .thumbs img').length") == 2, 'case file shows both photographs')
        check(await ev("document.querySelectorAll('.notebook .cards button').length") >= 6, 'case file lists the papers')
        await shot('c13_casefile')
        await pg.click('.notebook .thumbs button')
        check(await ev("!!document.querySelector('.docview .page.photo img')"), 'a photograph opens from the case file')
        await ev("S47.game.d.ui.close(true)")

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-{METHOD}.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
