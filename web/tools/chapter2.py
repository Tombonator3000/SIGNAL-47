# Chapter two, "The Amended Record", played through in a headless browser: Ward's call,
# the corridor door, walking into the records room, the three collections in an order
# the player might pick, wrong and right answers for P04 and P05 with the Unity replies,
# the call to the key holder, the end card, and Continue both halfway and at the end.
# Starts from S47.jump('chapter2'): a finished chapter one without photographs.
# Usage: python3 tools/chapter2.py OUTDIR [WxH]
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
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
        s2 = lambda k: ev(f"S47.ch2.s.{k}")
        async def aim_at(px, pz, tx, ty, tz):
            yaw = math.atan2(-(tx - px), -(tz - pz))
            pitch = math.atan2(ty - 1.62, math.hypot(tx - px, tz - pz))
            await place(px, pz, yaw, pitch)
        async def walk(points):
            await place(points[0][0], points[0][1], 0.0)
            return await ev(f'''(()=>{{const pts={json.dumps(points[1:])}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 1200) return false; }} }}
              return true; }})()''')
        async def toasts(): return await ev("document.querySelector('.toasts').textContent")
        async def put_back():
            await pg.click('.docview [data-a=back]'); await tick(0.2)
        async def say(): return await ev("(document.querySelector('.labpanel [data-say]')||{}).textContent || ''")
        docs = lambda: ev("S47.game.docs.map(d => d.id)")

        # ---------- Ward ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter2'); S47.tick(1.5)")
        check(await ev("S47.game.phase") == 'ch2' and await s2('stage') == 'ward-call', 'chapter two starts with the supervisor line')
        check(await ev("S47.ch2.ringing") and 'SUPERVISOR LINE' in await ev("document.querySelector('.objective').textContent"), 'phone rings and the objective says so')
        await aim_at(3.4, 1.55, 3.02, 0.84, 2.36)
        check(await aimed() == 'phone' and await label('phone') == 'Answer the phone', 'the ringing phone can be aimed at')
        await shot('d01_desk')
        await use('phone'); await tick(1.0)
        check('WARD:' in await toasts() and 'morning series' in await toasts(), 'Ward\'s first line is captioned')
        await tick(5.0)
        check('reference record' in await toasts(), 'Ward\'s second line is captioned')
        await tick(5.0)
        check(await s2('stage') == 'records' and await s2('answered'), 'call over, the records are next')
        check(any('morning series' in n for n in await ev("S47.game.notes")), 'notebook records that the morning series is held')

        # ---------- corridor door and the walk ----------
        await aim_at(-2.4, 3.3, -2.4, 1.1, 4.65)
        check(await aimed() == 'doorSouth' and await label('doorSouth') == 'Open corridor door', 'corridor door is unlocked')
        await use('doorSouth'); await tick(1.0)
        check(abs(await ev("S47.room.southDoorHinge.rotation.y") - math.pi / 2) < 0.01, 'corridor door swings open')
        await place(-2.4, 3.2, math.pi, 0.0); await tick(0.1)
        await shot('d02_corridor_door')
        check(await walk([(-2.4, 3.0), (-2.4, 5.7), (-3.9, 5.7), (-3.9, 7.6), (-4.9, 7.6)]), 'walk through the corridor door, along the corridor and into the records room')
        await tick(0.1)  # the chapter notices where the player is on its next frame
        check(await s2('entered'), 'records room entered')
        await place(-2.0, 7.4, math.atan2(-(-5.0 + 2.0), -(9.6 - 7.4)), -0.1)
        await shot('d03_records')

        # ---------- the table before anything is on it ----------
        await aim_at(-3.4, 7.75, -3.4, 0.9, 8.6)
        check(await aimed() == 'workTable' and await label('workTable') == 'Work table', 'work table can be aimed at')
        await use('workTable'); await tick(0.1)
        check('Nothing on the table yet' in await toasts(), 'empty table says where the records are')

        # ---------- lineage card first ----------
        await aim_at(-2.05, 7.95, -1.3, 0.8, 7.95)
        check(await aimed() == 'lineage', 'card index can be aimed at')
        await use('lineage'); await tick(0.1)
        check(await ev("!!document.querySelector('.docview')") and 'STATION 01' in await ev("document.querySelector('.docview').textContent"), 'lineage card opens and names STATION 01')
        await put_back()
        check(await s2('read.lineage') and 'b12lineage' in await docs(), 'lineage card filed')

        # P04 cannot be recorded yet
        await aim_at(-3.4, 7.75, -3.4, 0.9, 8.6)
        check(await label('workTable') == 'Lay out the records', 'table offers the records once one is collected')
        await use('workTable'); await tick(0.1)
        check(await ev("!!document.querySelector('.labpanel')") and 'SARO ARCHIVE' in await ev("document.querySelector('.labpanel h3').textContent"), 'archive panel opens')
        await pg.click('.labpanel [data-p=original]')
        check('Not on the table yet' in await ev("document.querySelector('.lp-page').textContent"), 'an uncollected source says where it is')
        await pg.click('.labpanel [data-p=compare]')
        await pg.click('.labpanel [data-c=omitted-c]')
        check('Read both the original protocol' in await say() and not await s2('p04'), 'P04 refused before both versions are read')
        await pg.click('.labpanel .lp-close'); await tick(0.1)

        # ---------- the 1947 box ----------
        await aim_at(-5.05, 8.6, -5.55, 1.28, 8.6)
        check(await aimed() == 'fieldBox' and await label('fieldBox') == 'STATION 01 field records, 1947', 'STATION 01 box can be aimed at')
        await use('fieldBox'); await tick(0.1)
        check(await ev("!!document.querySelector('.docview .page.typed img.attached')"), 'original record opens with its arrangement drawing')
        await put_back()
        check(await s2('read.original') and await s2('read.index') and 'e07' in await docs() and 'e08' in await docs(), 'original record and archive sleeve filed')
        check('archive sleeve' in await toasts(), 'the sleeve is pointed out')

        # ---------- the service copy ----------
        await aim_at(-2.3, 9.62, -2.32, 1.0, 10.2)
        check(await aimed() == 'binder', 'service copies binder can be aimed at')
        await use('binder'); await tick(0.1)
        check('AMENDED COPY' in await ev("document.querySelector('.docview').textContent"), 'amended copy opens')
        await put_back()
        check(await s2('read.amended') and 'e06' in await docs(), 'amended copy filed')

        # ---------- P04 ----------
        await aim_at(-3.4, 7.75, -3.4, 0.9, 8.6)
        await use('workTable'); await tick(0.1)
        await pg.click('.labpanel [data-p=original]')
        check('closing sight line' in await ev("document.querySelector('.lp-page').textContent") and await ev("!!document.querySelector('.lp-page .lp-figure img')"), 'E07 tab shows the text and the drawing')
        await pg.click('.labpanel [data-p=compare]')
        await pg.click('.labpanel [data-c=author-guilt]')
        check('shared signature' in await say(), 'P04: blaming the author is answered with the Unity reply')
        await pg.click('.labpanel [data-c=development-only]')
        check('Compare the arrangement' in await say(), 'P04: the development story is answered with the Unity reply')
        await shot('d04_p04_feedback')
        await pg.click('.labpanel [data-c=omitted-c]')
        check(await s2('p04') and await s2('wrong04') == 2 and 'SUPPORTED' in await ev("document.querySelector('.lp-page').textContent"), 'P04 recorded: C was omitted')
        check('p04' in await docs(), 'P04 finding filed')

        # ---------- P05 ----------
        await pg.click('.labpanel [data-p=route]')
        async def choose(g, k): await pg.click(f'.labpanel [data-g={g}][data-k="{k}"]')
        await choose('destination', 'old-survey-station'); await choose('survey', '-39 LY AS A YEAR CODE'); await choose('marker', 'triangle-bar')
        await pg.click('.labpanel [data-a=record]')
        check('not a calendar code' in await say(), 'P05: -39 LY as a year code is refused')
        await choose('survey', 'STATION 01'); await choose('marker', 'triangle')
        await pg.click('.labpanel [data-a=record]')
        check('only an elevation symbol' in await say(), 'P05: a triangle without the bar is refused')
        await choose('marker', 'triangle-bar'); await choose('destination', 'saro-apron')
        await pg.click('.labpanel [data-a=record]')
        check('identify OLD SURVEY STATION' in await say(), 'P05: the wrong destination is refused')
        await shot('d05_p05_feedback')
        await choose('destination', 'old-survey-station')
        await pg.click('.labpanel [data-a=record]')
        check(await s2('p05') and await s2('stage') == 'call-nora' and await s2('wrong05') == 3, 'P05 recorded: OLD SURVEY STATION')
        d = await docs()
        check('p05' in d and 'access' in d and 'casemap' in d, 'P05 finding, field access sheet and case map filed')
        await pg.click('.labpanel .lp-close'); await tick(0.1)
        await pg.wait_for_timeout(300)
        saved = await ev("JSON.parse(localStorage.getItem('s47.case') || 'null')")
        check(saved and saved.get('ch2', {}).get('p05') and await ev("localStorage.getItem('s47.checkpoint')") == '"chapter2"', 'chapter two progress saved with checkpoint chapter2')

        # ---------- Continue halfway (before the call) ----------
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.game.phase") == 'ch2' and await s2('stage') == 'call-nora' and await s2('p04') and await s2('read.original'), 'Continue restores chapter two halfway')
        check(abs(await ev("S47.room.southDoorHinge.rotation.y") - math.pi / 2) < 0.01, 'Continue keeps the corridor door open')
        check('access' in await docs() and 'e06' in await docs(), 'Continue restores the archive papers')

        # ---------- the key holder ----------
        await aim_at(-1.85, 9.25, -1.12, 1.42, 9.25)
        check(await aimed() == 'recPhone' and await label('recPhone') == 'Call the key holder', 'wall phone offers the call')
        await use('recPhone')
        seen = ''
        for _ in range(30):
            await tick(1.0)
            seen += await toasts()
            if await s2('called'): break
        check('N. VEGA' in seen and 'cable' in seen and 'originals' in seen, 'N. Vega\'s lines are captioned')
        check(await s2('called') and await s2('stage') == 'complete', 'call made, chapter two complete')
        check(any('Called the key holder' in n for n in await ev("S47.game.notes")), 'notebook records the call')
        await tick(4.0)
        await pg.wait_for_function("() => { const a = document.querySelector('.endcard .after'); return !!a && getComputedStyle(a).opacity === '1'; }", polling=500)
        text = await ev("document.querySelector('.endcard').textContent")
        check('THE AMENDED RECORD' in text and 'OLD SURVEY STATION' in text and 'N. Vega' in text, 'end card for chapter two')
        await shot('d06_ending')

        # ---------- Continue at the end ----------
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_timeout(500)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.game.phase") == 'ch2' and await s2('stage') == 'complete' and await s2('called'), 'Continue restores the finished chapter two')
        check('FIELD ACCESS PREPARED' in await ev("document.querySelector('.objective').textContent"), 'objective says the next area is not built yet')
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=case]')
        n = await ev("document.querySelectorAll('.notebook .cards button').length")
        check(n >= 12, f'case file holds both chapters\' papers ({n})')
        await shot('d07_casefile')
        await ev("S47.game.d.ui.close(true)")

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-chapter2.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
