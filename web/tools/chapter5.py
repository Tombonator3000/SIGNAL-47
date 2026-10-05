# Chapter five, "All Night", played through in a headless browser: the chapter starts at
# five outside room 6, the operations terminal chimes and shows the file opened at 05:29,
# the truck takes the player to Mesa Diner, and there the waitress (coffee, the lights over
# the mesa, the clipping), the driver (the old road, last October), the radio, the clipping
# itself (E15), Ward on the payphone, and the map puzzle at the booth with a real drag of
# the tracing (wrong answers, then P13). After P13 the driver confirms the bolt. Save and
# Continue inside the diner, out to the truck: the case is saved there, the cut takes the
# truck onto the old road (chapter six), and Continue from the title gives the diner back
# with the truck ready, so the end of the night can be played again. Starts from
# S47.jump('chapter5'). Usage: python3 tools/chapter5.py OUTDIR [WxH]
# Set S47_URL to test another build, for example the Pages build served over HTTP.
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
        use = lambda i: ev(f"S47.game.d.inter.get('{i}').use(); 0")
        s5 = lambda k: ev(f"S47.ch5.s.{k}")
        async def objective(): return await ev("document.querySelector('.objective').textContent")
        async def lines(): return await ev("[...document.querySelectorAll('.talk-line')].map(e => e.textContent).join(' | ')")
        async def choose(c): await pg.click(f'.talk [data-c={c}]'); await tick(0.1)
        async def choices(): return await ev("[...document.querySelectorAll('.talk [data-c]')].map(b => b.dataset.c)")
        async def notes(): return await ev("S47.game.notes")
        async def toasts(): return await ev("document.querySelector('.toasts').textContent")
        docs = lambda: ev("S47.game.docs.map(d => d.id)")
        async def walk(points):
            return await ev(f'''(()=>{{const pts={json.dumps(points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 2400) return [false, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        async def face(x, y, z):
            await ev(f"(()=>{{const p=S47.player; p.yaw=Math.atan2(-({x}-p.pos.x), -({z}-p.pos.z)); p.pitch=Math.atan2({y}-(p.floorY+p.eye), Math.hypot({x}-p.pos.x,{z}-p.pos.z)); S47.tick(0.05);}})()")
        async def face_proxy(k):
            v = await ev(f"(()=>{{const v=S47.world.diner.proxies['{k}'].getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]}})()")
            await face(v[0], v[1], v[2])
        async def anchor(k): return await ev(f"S47.world.diner.anchors['{k}']")
        async def close(): await ev("S47.game.d.ui.close(true)"); await tick(0.1)

        # ---------- five o'clock, outside room 6 ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter5'); S47.tick(1.0)")
        check(await ev("S47.game.phase") == 'ch5' and await s5('stage') == 'to-truck', 'chapter five starts: to the truck')
        check(await ev("S47.game.chapterTitle()") == 'Chapter 5: All Night', 'chapter title')
        check(await ev("S47.game.clock % 86400 >= 5 * 3600"), 'the clock is past five')
        check('TRUCK' in await objective(), 'objective names the truck')
        check(await ev("S47.ch4.s.stage") == 'complete' and 'p12' in await docs(), 'room 6 is behind: chapter four complete, P12 filed')

        # ---------- the terminal chimes; the 05:29 file ----------
        await place(0.5, 1.5, math.pi / 2, -0.1)
        await tick(0.5)
        check(await s5('bell') and 'chimes once' in await toasts(), 'coming past the control room, the operations terminal chimes')
        check('RUN860414_0529.DAT' in await ev("S47.game.ops.run('DIR SURVEY.RAW')"), 'a file opened at 05:29 stands in SURVEY.RAW')
        await use('opsTerminal'); await tick(0.2)
        await pg.fill('.termview input', 'TYPE RUN860414_0529.DAT'); await pg.press('.termview input', 'Enter')
        await tick(0.1)
        out = await ev("document.querySelector('.term-out').textContent")
        check('FILE INCOMPLETE' in out and 'OPERATOR  REYES' in out and await s5('file'), 'TYPE: 0 records, operator Reyes, file incomplete; the chapter notes it')
        check(any('opened at 05:29' in n for n in await notes()), 'the journal has the time it was read and the time it was opened')
        await shot('h01_terminal')
        await pg.press('.termview input', 'Escape'); await tick(0.1)

        # ---------- the truck to the diner ----------
        await place(10.6, 8.6, math.pi / 2, -0.1)
        check(await aimed() == 'truck' and await label('truck') == 'Drive to Mesa Diner', 'the truck on its pad offers the diner')
        clock0 = await ev("S47.game.clock")
        await use('truck')
        for _ in range(60):
            await tick(0.25)
            if await ev("S47.world.area === 'diner' && !S47.game.cinematic && S47.ch5.s.arrived"): break
        check(await ev("S47.world.area") == 'diner' and await s5('arrived') and await s5('stage') == 'diner', 'a cut with the name, and the player stands by the truck at Mesa Diner')
        dt = await ev("S47.game.clock") - clock0
        check(8 * 60 < dt < 12 * 60, f'about nine minutes on the clock ({dt / 60:.1f})')
        await tick(0.2)
        dd, obj = await docs(), await objective()
        check('e14' in dd and 'P13' in obj, f'the service map came along (E14); objective: where does C cross ({obj})')
        await shot('h02_diner_lot')

        # ---------- in, to the counter ----------
        a = await anchor('arrive'); o = await anchor('outside'); i = await anchor('inside'); st = await anchor('stool')
        ok = await walk([[o['x'], o['z']], [i['x'], i['z']], [st['x'], st['z']]])
        check(ok[0], f'walked in from the truck to the stool (stopped at {ok[1]:.1f}, {ok[2]:.1f})')
        await face_proxy('waitress')
        check(await aimed() == 'diner:waitress' and await label('diner:waitress') == 'Talk to the waitress', 'the waitress behind the counter')
        await use('diner:waitress'); await tick(0.2)
        check('Coffee?' in await lines() and set(await choices()) >= {'coffee', 'lights', 'clipping'}, 'she offers coffee')
        await choose('coffee')
        check(await s5('coffee') and await ev("S47.world.diner.objs.cup.visible"), 'coffee: a cup on the counter')
        await choose('lights')
        check(await s5('lights') and 'I was eleven' in await lines() and 'little green men' in await lines(), 'the lights over the mesa since she was eleven; not little green men')
        await choose('clipping')
        check(await s5('aunt') and 'trimmed it to fit the frame' in await lines(), 'her aunt cut the clipping out; the paper ran it bigger')
        await shot('h03_waitress')
        await close()

        await face_proxy('driver')
        check(await aimed() == 'diner:driver' and await label('diner:driver') == 'Talk to the driver', 'the driver on the next stool')
        await use('diner:driver'); await tick(0.2)
        check('bolt' not in await choices(), 'no question about the bolt yet')
        await choose('road'); await choose('october')
        check(await s5('october') and 'eight-mile post' in await lines() and 'battery' in await lines(), 'his rig died last October right past the eight-mile post; he figured battery')
        await close()

        await face_proxy('counter')
        check(await aimed() == 'diner:counter', 'the radio behind the counter')
        await use('diner:counter')
        heard = ''
        for _ in range(8):
            await tick(2.2); heard += await toasts()
        check(await s5('radio') and 'Nicklaus' in heard and 'Halley' in heard, 'the radio: the weather, Nicklaus at forty-six, Halley low in the south')

        # ---------- the clipping by the door ----------
        ok = await walk([[i['x'], i['z']], [1.0 - 8000, 1.35]])
        await face_proxy('clipping')
        check(ok[0] and await aimed() == 'diner:clipping', 'the clipping by the door')
        await use('diner:clipping'); await tick(0.3)
        img = await ev("(()=>{const i=document.querySelector('.docview img'); return i ? [i.naturalWidth, i.naturalHeight] : null})()")
        check(await s5('clipped') and 'e15' in await docs() and img and img[1] > 1000, f'E15: the clipping as it hangs, the 1947 photograph in it ({img})')
        await shot('h04_clipping')
        await close()

        # ---------- the payphone ----------
        ph = await anchor('phone')
        ok = await walk([[i['x'], i['z']], [-8001.3, 0.3], [-8001.3, 7.9], [ph['x'], ph['z']]])
        await face_proxy('payphone')
        am = await aimed()
        check(ok[0] and am == 'diner:payphone', f'the payphone in its niche ({ok}, {am})')
        await use('diner:payphone'); await tick(0.2)
        check('Hondo called. So did Site 11.' in await lines(), 'Ward: "Hondo called. So did Site 11."')
        await choose('what'); await choose('road')
        check(await s5('ward') and 'morning series' in await lines(), 'she wants you back before the morning series')
        await shot('h05_payphone')
        await close()

        # ---------- the booth: the maps (P13) ----------
        ok = await walk([[-8001.3, 7.9], [-8001.0, 3.0]])
        await face_proxy('booth')
        am = await aimed()
        check(ok[0] and am == 'diner:booth' and await label('diner:booth') == 'Spread the maps on the table', f'a booth: spread the maps on the table ({ok}, {am})')
        await use('diner:booth'); await tick(0.2)
        check(await ev("!!document.querySelector('.mapov canvas')"), 'the service map with the tracing on top')
        await pg.click('[data-c=bm6]'); await tick(0.1)
        say = await ev("document.querySelector('[data-say]').textContent")
        check('STATION 01 first' in say and await s5('wrong13') == 1 and not await s5('p13'), 'an answer before the tracing is on the station: lay it on STATION 01 first')
        await shot('h06_map_unaligned')
        # drag the tracing with the mouse: its transit from where it lies to STATION 01
        r = await ev("(()=>{const c=document.querySelector('.mapov canvas').getBoundingClientRect(); return [c.left, c.top, c.width, c.height]})()")
        tr = await s5('tracing'); stn = await ev("S47.ch5.s.tracing && [470, 360]")
        k = r[2] / 1200
        sx, sy = r[0] + (tr[0] + 30) * k, r[1] + tr[1] * k
        ex, ey = r[0] + (470 + 30 + 4) * k, r[1] + (360 + 3) * k
        await pg.mouse.move(sx, sy); await pg.mouse.down()
        for n in range(1, 13): await pg.mouse.move(sx + (ex - sx) * n / 12, sy + (ey - sy) * n / 12)
        await pg.mouse.up(); await tick(0.1)
        check(await s5('tracing') == [470, 360] and 'sits on STATION 01' in await ev("document.querySelector('[data-status]').textContent"), 'dragged close, the tracing settles on STATION 01, north to north')
        await shot('h07_map_aligned')
        await pg.click('[data-c=bm10]'); await tick(0.1)
        check('before that' in await ev("document.querySelector('[data-say]').textContent") and not await s5('p13'), 'past the ten-mile post: C meets the road before that')
        await pg.click('[data-c=bm8]'); await tick(0.2)
        check(await s5('p13') and await s5('stage') == 'road' and 'p13' in await docs(), 'P13 recorded: just past the eight-mile post, at a survey bolt')
        check('EIGHT-MILE POST' in await objective(), 'objective: the truck is outside')
        await close()

        # ---------- the driver confirms ----------
        ok = await walk([[i['x'], i['z']], [st['x'], st['z']]])
        await face_proxy('driver')
        await use('diner:driver'); await tick(0.2)
        check('bolt' in await choices(), 'now the driver can be asked about the bolt')
        await choose('bolt')
        check(await s5('confirmed') and 'right where she quit on me' in await lines(), 'the brass bolt is right where his rig died')
        await close()

        # ---------- Continue in the diner ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        check('Mesa Diner' in await ev("document.querySelector('.cont-info').textContent"), 'title screen offers to continue at Mesa Diner')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.world.area") == 'diner' and await s5('p13') and await s5('stage') == 'road' and await ev("S47.world.diner.objs.cup.visible"), 'Continue brings back the diner, P13 and the coffee on the counter')
        dd = await docs()
        check(all(x in dd for x in ('e14', 'e15', 'p13')), 'and the papers: the service map, the clipping, P13')
        await shot('h08_continue')

        # ---------- out to the truck: saved in the diner, then the old road ----------
        ok = await walk([[i['x'], i['z']], [o['x'], o['z']], [a['x'], a['z']]])
        tk = await ev("(()=>{const v=S47.world.dinerTruckProxy.getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]})()")
        await face(tk[0], tk[1], tk[2])
        check(ok[0] and await aimed() == 'dinerTruck' and await label('dinerTruck') == 'Drive the old road', 'out by the truck: drive the old road')
        await use('dinerTruck')
        await tick(0.2); await pg.wait_for_timeout(800)
        cont = await ev("(() => { const m = S47.saves.continueSave(); return m ? { chapter: m.chapter, place: m.place } : null; })()")
        check(cont is not None and cont['chapter'] == 'Chapter 5: All Night' and 'Mesa Diner' in cont['place'], f'the case is saved in the diner before the road: {cont}')
        for _ in range(30):
            await tick(0.25)
            if await ev("S47.world.area === 'roswell' && S47.world.driving"): break
        check(await ev("S47.game.phase") == 'ch6' and await ev("S47.ch6.stage") == 'drive' and await s5('stage') == 'complete', 'the cut: chapter six, on the old road')
        check(await ev("S47.game.clock") >= 5 * 3600 + 26 * 60 and 'MILE 8' in await objective(), 'the clock is past 05:26 and the objective is the line past mile 8')
        check(any('coffee stays on the counter' in n for n in await notes()), 'the coffee stays on the counter')
        check(await ev("S47.game.saveBlock()") == 'Not on the road.', 'no saving on the road')
        await shot('h09_old_road')
        # out to the title now, as after the ending, and Continue: the diner with the truck ready
        await ev("S47.hold = false")
        await pg.keyboard.press('Escape'); await pg.wait_for_timeout(400)
        await pg.click('[data-a=title]')
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.world.area") == 'diner' and await s5('stage') == 'road' and await ev("S47.game.phase") == 'ch5', 'Continue after the road: the diner again, the truck ready')
        await ev("S47.hold = true")

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-chapter5.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
