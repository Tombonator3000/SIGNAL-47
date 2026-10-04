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
        await pg.goto(URL)
        # Let the first page finish fetching its sounds. On the Pages build they are separate
        # files, and a reload in the middle aborts the fetch, so the old page logs a warning.
        try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
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
        async def say(): return await ev("(document.querySelector('.board-wrap [data-say]')||{}).textContent || ''")
        # the evidence board: tap one end, then the other (as on a touch screen)
        async def thread(a, b):
            await pg.click(f'[data-node="{a}"]'); await pg.click(f'[data-node="{b}"]')
            return await say()
        # or pull the thread with the mouse, from one pin to the other
        async def pull(a, b):
            pa = await pg.locator(f'[data-node="{a}"]').bounding_box(); pb = await pg.locator(f'[data-node="{b}"]').bounding_box()
            await pg.mouse.move(pa['x'] + pa['width'] / 2, pa['y'] + pa['height'] / 2); await pg.mouse.down()
            for i in range(1, 9): await pg.mouse.move(pa['x'] + pa['width'] / 2 + (pb['x'] - pa['x'] + (pb['width'] - pa['width']) / 2) * i / 8, pa['y'] + pa['height'] / 2 + (pb['y'] - pa['y'] + (pb['height'] - pa['height']) / 2) * i / 8)
            await pg.mouse.up()
            return await say()
        has = lambda sel: ev(f"!!document.querySelector('{sel}')")
        async def close_board(): await pg.click('.board-wrap .lp-close'); await tick(0.1)
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
        check(await has('.board-wrap') and 'SARO ARCHIVE' in await ev("document.querySelector('.board-wrap h3').textContent"), 'archive board opens')
        check('Not on the table yet' in await ev("document.querySelector('.bcard[data-card=e07]').textContent") and not await has('[data-node="e07.abc"]'), 'an uncollected record says where it is')
        r = await thread('b12.id', 'P04')
        check('Read both the original protocol' in r and not await s2('p04'), 'P04 refused before both versions are read')
        await close_board()

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

        # ---------- P04 on the evidence board ----------
        await aim_at(-3.4, 7.75, -3.4, 0.9, 8.6)
        await use('workTable'); await tick(0.1)
        check(await has('.bcard[data-card=e07] .bcard-img') and await has('[data-node="e07.abc"]') and await has('[data-node="e06.omit"]'), 'E07 and E06 lie on the board with their drawings and lines')
        await pg.click('.bcard[data-card=e07] [data-more]')
        check('Preserve the original plates' in await ev("document.querySelector('.bcard[data-card=e07] .bcard-full').textContent")
              and await ev("getComputedStyle(document.querySelector('.bcard[data-card=e07] .bcard-full')).display") == 'block', 'TEXT shows the whole original')
        await pg.click('.bcard[data-card=e07] [data-more]')
        r = await pull('e07.sig', 'e06.sig')
        check('who changed the record' in r and await has('.bnote[data-node="n.sig"]'), 'a thread pulled with the mouse from signature to signature leaves a note')
        r = await thread('n.sig', 'P04')
        check('shared signature' in r and await s2('wrong04') == 1, 'P04: the signature note is answered with the Unity reply')
        r = await thread('e06.fault', 'P04')
        check('Compare the arrangement' in r and await s2('wrong04') == 2, 'P04: the development explanation alone is answered with the Unity reply')
        r = await thread('e07.head', 'e06.repeat')
        check('do not say anything' in r and await ev("S47.ch2.s.links.length") == 1, 'a thread between unrelated lines falls slack')
        r = await thread('e07.abc', 'e06.omit')
        check('leaves it out' in r and await has('.bnote[data-node="n.c"]'), 'the three references and the omitted sight line: C is gone')
        r = await thread('e07.mark', 'e06.fault')
        check('development fault' in r and await has('.bnote[data-node="n.mark"]'), 'the retained mark and the development fault')
        r = await thread('n.c', 'P04')
        check('Pinned' in r and not await s2('p04'), 'one note pinned on P04, one slot left')
        await shot('d04_p04_feedback')
        r = await thread('n.mark', 'P04')
        check(await s2('p04') and await s2('wrong04') == 2 and 'SUPPORTED' in await ev("document.querySelector('.bq[data-node=P04] .bq-done').textContent"), 'P04 recorded: C was omitted')
        check('p04' in await docs(), 'P04 finding filed')

        # ---------- P05 ----------
        # a player who read E07 on the shelf before laying out the table has only that picture
        # drawn; the board must still draw the mark icons
        await close_board()
        await ev("S47.ch2.images = { original: S47.ch2.images.original }")
        await use('workTable'); await tick(0.1)
        check(await has('[data-node="b12.mark"] img') and await has('[data-node="e08.elev"] img'), 'the mark icons are drawn also when E07 was read before the table')
        r = await thread('e08.dest', 'P05')
        check('A place name alone' in r and await s2('wrong05') == 0, 'P05: the place name alone is not enough')
        r = await thread('rx.dist', 'e08.id')
        check('not a calendar code' in r and await s2('wrong05') == 1, 'P05: -39 LY as a year is refused')
        r = await thread('b12.mark', 'e08.elev')
        check('only an elevation symbol' in r and await s2('wrong05') == 2, 'P05: a triangle without the bar is refused')
        r = await pull('b12.id', 'e08.id')
        check('Same survey' in r, 'STATION 01 on the card and on the sleeve')
        r = await thread('b12.mark', 'e08.mark')
        check('Same fixed-point mark' in r, 'the two triangles with the bar')
        r = await thread('n.mark', 'P05')
        check('P04 is recorded' in r or 'other question' in r, 'a P04 note does not go on P05')
        for n in ('n.id', 'n.mark2'): r = await thread(n, 'P05')
        check('Now the place' in r and not await s2('p05'), 'both matches pinned on P05')
        await shot('d05_p05_feedback')
        r = await thread('e08.dest', 'P05')
        check(await s2('p05') and await s2('stage') == 'call-nora' and await s2('wrong05') == 2, 'P05 recorded: OLD SURVEY STATION')
        d = await docs()
        check('p05' in d and 'access' in d and 'casemap' in d, 'P05 finding, field access sheet and case map filed')
        await close_board()
        # a save from before the board has P04 and P05 but no threads: they are laid out again
        await ev("S47.ch2.s.links = undefined")
        await use('workTable'); await tick(0.1)
        check(await ev("S47.ch2.s.links.length") == 9 and await has('.bq.done[data-node=P05]') and await has('.bnote[data-node="n.mark2"]'), 'an older save gets its threads and notes back on the board')
        await close_board()
        await tick(0.1)
        await pg.wait_for_function("S47.saves.list(S47.caseId()).length > 0", polling=300, timeout=20000)
        await pg.wait_for_timeout(300)
        meta = await ev("S47.saves.list(S47.caseId())[0]")
        rec = await ev(IDB_GET + f"('saves', '{meta['id']}')")
        check(rec and rec['state']['checkpoint'] == 'chapter2' and rec['state']['case']['ch2']['p05'] and meta['place'] == 'Records room', f"chapter two progress autosaved ({meta['id']}, {meta['place']}, {meta['clock']})")

        # ---------- Continue halfway (before the call) ----------
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
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
        # no end card: a chapter card, then chapter three begins in the records room
        for i in range(24):
            await tick(0.5)
            if await ev("!!document.querySelector('.chapter-card')"): break
        check(await ev("!!document.querySelector('.chapter-card')"), 'chapter card after chapter two')
        text = await ev("document.querySelector('.chapter-card').textContent")
        check('THE AMENDED RECORD' in text and 'CHAPTER THREE' in text and 'THE SURVEY STATION' in text, 'chapter card closes chapter two and names chapter three')
        check(await pg.locator('.endcard').count() == 0, 'no end card between the chapters')
        await pg.wait_for_function("() => { const b = document.querySelector('.chapter-card .cc-b'); return !!b && getComputedStyle(b).opacity === '1'; }", polling=500)
        await shot('d06_chapter_card')
        await tick(7)
        check(await ev("S47.game.phase") == 'ch3' and 'TAKE THE SARO TRUCK' in await ev("document.querySelector('.objective').textContent"), 'chapter three begins: take the truck to STATION 01')
        await pg.wait_for_function("S47.saves.list(S47.caseId()).some(m => m.chapter.startsWith('Chapter 3'))", polling=300, timeout=20000)

        # ---------- Continue at the end ----------
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        check('Chapter 3' in await ev("document.querySelector('.cont-info').textContent"), 'title screen offers to continue chapter three')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.game.phase") == 'ch3' and await s2('stage') == 'complete' and await s2('called'), 'Continue restores chapter three with chapter two finished')
        check('TAKE THE SARO TRUCK' in await ev("document.querySelector('.objective').textContent"), 'objective: the service truck')
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
