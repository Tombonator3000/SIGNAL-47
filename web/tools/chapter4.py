# Chapter four, "Room 6", played through in a headless browser: the fire exit at the end
# of the south corridor, the ramp and the walk across the highway (the player follows the
# ground height), the door of room 6, the conversation with N. Vega with wrong and right
# answers (P10, the three parts of P11 with her correction, Tomas' letter and his field
# card, P12 on the old field line, where C goes), the papers on the table, Continue inside
# room 6, leaving, and the end card. Starts from S47.jump('chapter4').
# Usage: python3 tools/chapter4.py OUTDIR [WxH]
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
        s4 = lambda k: ev(f"S47.ch4.s.{k}")
        async def objective(): return await ev("document.querySelector('.objective').textContent")
        async def lines(): return await ev("[...document.querySelectorAll('.talk-line')].map(e => e.textContent).join(' | ')")
        async def reply(): return await ev("(document.querySelector('.talk .lp-text') || {}).textContent || ''")
        async def choose(c): await pg.click(f'.talk [data-c={c}]'); await tick(0.1)
        async def choices(): return await ev("[...document.querySelectorAll('.talk [data-c]')].map(b => b.dataset.c)")
        docs = lambda: ev("S47.game.docs.map(d => d.id)")
        # walk the player along points with the real movement code (zones, colliders, ground height)
        async def walk(points):
            return await ev(f'''(()=>{{const pts={json.dumps(points)}; const p=S47.player;
              for (const [x,z] of pts) {{ let n=0;
                while (Math.hypot(p.pos.x-x, p.pos.z-z) > 0.12) {{
                  p.yaw = Math.atan2(-(x-p.pos.x), -(z-p.pos.z)); p.update(1/30, 0, -1, false);
                  if (++n > 2400) return [false, p.pos.x, p.pos.z]; }} }}
              return [true, p.pos.x, p.pos.z]; }})()''')
        async def face(x, y, z):
            await ev(f"(()=>{{const p=S47.player; p.yaw=Math.atan2(-({x}-p.pos.x), -({z}-p.pos.z)); p.pitch=Math.atan2({y}-(p.floorY+p.eye), Math.hypot({x}-p.pos.x,{z}-p.pos.z)); S47.tick(0.05);}})()")

        # ---------- SARO: the fire exit ----------
        await ev("S47.hold = true; S47.tick(0.3); S47.jump('chapter4'); S47.tick(1.0)")
        check(await ev("S47.game.phase") == 'ch4' and await s4('stage') == 'to-motel', 'chapter four starts: to the motel')
        check('FIRE EXIT' in await objective(), 'objective points to the fire exit')
        check(await ev("S47.game.chapterTitle()") == 'Chapter 4: Room 6', 'chapter title')
        await place(-4.6, 5.7, math.pi / 2, -0.05)
        check(await aimed() == 'exitDoor' and await label('exitDoor') == 'Open the fire exit', 'the fire exit offers to open')
        check(not await ev("S47.player.walkable(-6.15, 5.7)"), 'the doorway is closed while the door is shut')
        await use('exitDoor'); await tick(1.4)
        check(await s4('exitOpen') and await ev("S47.annex.exitOpen > 0.99 && S47.annex.zone.exitDoor.enabled"), 'the fire exit swings open and its doorway opens')
        check(await ev("S47.world.crossing.brick.visible"), 'a brick props it open')
        await shot('f01_fire_exit')

        # ---------- the step, the ramp, the road ----------
        ok = await walk([[-5.6, 5.7], [-6.9, 5.7]])
        check(ok[0] and abs(await ev("S47.player.floorY")) < 0.02, 'through the doorway onto the step, at floor level')
        await ev("S47.player.yaw = Math.PI / 2; S47.player.pitch = -0.12; S47.tick(0.1)")
        await shot('f02_step')
        ok = await walk([[-9.1, 5.7]])
        mid = await ev("S47.player.floorY")
        ok2 = await walk([[-11.2, 5.7]])
        low = await ev("S47.player.floorY")
        check(ok[0] and ok2[0] and -0.45 < mid < -0.15 and abs(low + 0.6) < 0.05, f'down the ramp to the ground (eye follows: {mid:.2f} m on the ramp, {low:.2f} m at the foot)')
        ok = await walk([[-24.0, 6.0]])
        check(ok[0], 'along the path to the highway')
        await ev("S47.player.yaw = Math.atan2(-(-41 + 24), -(55 - 6)); S47.player.pitch = -0.04; S47.tick(0.2)")
        await shot('f03_road')
        calls = await ev("S47.renderer.info.render.calls")
        print(f'draw calls on the road: {calls}', flush=True)
        check(calls < 300, 'draw calls on the road under 300')
        check('CROSS THE ROAD' in await objective(), 'objective: cross the road to room 6')
        ok = await walk([[-26.0, 6.0], [-29.0, 6.0]])
        check(ok[0] and abs(await ev("S47.player.floorY")) < 0.05, 'up the driveway ramp onto the motel lot, level with SARO')
        # the office first: Nora's note on the counter (world/MotelFront.ts)
        office = await ev("S47.world.court.anchors.officeInside")
        ok = await walk([[-37.5, 20.0], [-39.6, 24.1], [office['x'], office['z']]])
        check(ok[0] and await ev("S47.world.motel.interior.visible"), f'into the motel office (stopped at {ok[1]:.1f}, {ok[2]:.1f})')
        msg = await ev("(()=>{const v=S47.world.court.proxies.message.getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]})()")
        await face(msg[0], msg[1], msg[2])
        check(await aimed() == 'court:message', 'the note on the counter')
        await use('court:message'); await tick(0.2)
        check(any('Room 6. The door is open' in n for n in await ev("S47.game.notes")), 'her note leads to room 6')
        await shot('f03b_office')
        out = await ev("S47.world.court.anchors.room6Outside")
        ok = await walk([[-39.6, 24.1], [-37.5, 26.0], [-37.5, out['z'] - 0.6], [out['x'], out['z']]])
        check(ok[0], f'along the lot to room 6 (stopped at {ok[1]:.1f}, {ok[2]:.1f})')
        await ev(f"S47.player.yaw = {out['yaw']}; S47.player.pitch = 0.05; S47.tick(0.1)")
        check(await aimed() == 'room6Door' and await label('room6Door') == 'Room 6', 'the door of room 6')
        await shot('f04_room6_door')

        # ---------- room 6 ----------
        await use('room6Door')
        for _ in range(40):
            await tick(0.25); await pg.wait_for_timeout(150)   # the room loads in real time
            if await ev("S47.world.area === 'room6' && !S47.game.cinematic"): break
        await tick(0.1)   # the objective is redrawn on the next frame
        check(await ev("S47.world.area") == 'room6' and await s4('arrived') and await s4('stage') == 'room', 'into room 6')
        check(await ev("S47.world.room6.group.visible && !S47.ext.group.visible"), 'SARO is hidden while inside')
        check(await ev("S47.world.room6.tvOn") and await label('r6:tv') == 'Turn the television off', 'the television is on: snow and its light on the room')
        await use('r6:tv'); await tick(0.2)
        check(not await ev("S47.world.room6.tvOn") and 'quiet is worse' in await ev("document.querySelector('.toasts').textContent"), 'switched off, Nora would rather have the noise')
        await use('r6:tv'); await tick(0.2)
        check(await ev("S47.world.room6.tvOn"), 'and on again')
        check('WAITING AT THE TABLE' in await objective(), 'objective: she is waiting at the table')
        await tick(0.3); await pg.wait_for_timeout(700)   # the fade in is real time
        await shot('f05_room6')
        calls = await ev("S47.renderer.info.render.calls")
        print(f'draw calls in room 6: {calls}', flush=True)
        check(calls < 200, 'draw calls in room 6 under 200')
        talk = await ev("(()=>{const a=S47.world.room6.anchors.talk; return [a.x,a.z,a.yaw]})()")
        ok = await walk([[talk[0], talk[1]]])
        check(ok[0], 'walk in to the table')
        nora = await ev("(()=>{const v=S47.world.room6.proxies.nora.getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]})()")
        await face(nora[0], nora[1] + 0.25, nora[2])
        check(await aimed() == 'r6:nora', 'N. Vega can be spoken to')
        await use('r6:nora'); await tick(0.2)
        check('So you came across the road' in await lines(), 'she speaks first')

        # P10: the evidence on the table
        await pg.click('.talk [data-chip=e07]'); await pg.click('.talk [data-a=table]'); await tick(0.1)
        check('Both prints' in await reply() and not await s4('p10'), 'P10: one paper is not enough')
        await pg.click('.talk [data-chip=frame01]'); await pg.click('.talk [data-chip=frame02]'); await pg.click('.talk [data-chip=e06]')
        await pg.click('.talk [data-a=table]'); await tick(0.1)
        text = await lines()
        check('I know that second line' in text and 'I wrote it' in text and await s4('shown'), 'P10: the prints and the original on the table; she knows the amended copy')
        await pg.wait_for_timeout(2600)   # her lines come one after another
        await shot('f06_p10')
        await choose('none')
        check('Your lamp was shielded' in await reply() and not await s4('p10'), 'P10: a control that does not match the print is answered')
        await choose('lamp')
        check(await s4('p10') and 'p10' in await docs() and 'That is what I did in 1947' in await lines(), 'P10 recorded')

        # P11: the three parts, in another order than the menu
        await choose('tomas')
        check(await s4('topics.tomas') and 'e13' in await docs() and await ev("S47.world.room6.objs.letter.visible"), 'T. Vega: his letter on the table (E13)')
        check('thirty-nine years' in await lines(), 'she says how long he has been nineteen')
        await choose('record')
        check(await choices() == ['mild', 'critical'], 'the amended record: a mild or a hard question')
        await choose('critical')
        check(await s4('topics.record') and 'e12' in await docs() and await ev("S47.world.room6.objs.correction.visible") and 'Yes. I changed it' in await lines(), 'the amended record: her signed correction (E12)')
        check('why' not in await choices(), 'P11 waits for all three parts')
        # P12 in between: the phone on the nightstand
        await choose('line')
        text = await lines()
        check('Where did you leave the spare key' in text and 'Reference west. No. East.' in text, 'P12: a new question on the old field line, the 1947 fragment back')
        await choose('alive')
        check('in the mug' in await reply() and not await s4('p12'), 'P12: "he answered" is answered from his letter')
        await choose('retained')
        check(await s4('p12') and 'p12' in await docs(), 'P12 recorded')
        await choose('cable')
        check(await s4('topics.cable') and 'e11' in await docs() and await ev("S47.world.room6.objs.fieldCard.visible") and 'I cut it' in await lines(), 'the cable: she cut it; his last field card (E11)')
        await choose('why')
        await choose('fault')
        check('not a development fault' in await reply() and not await s4('p11'), 'P11: a wrong motive is answered')
        await choose('repeat')
        check(await s4('p11') and 'p11' in await docs(), 'P11 recorded')
        await pg.click('.talk [data-read=letter]'); await tick(0.1)
        check('spare key in the mug' in await ev("document.querySelector('.docview').textContent"), 'the letter can be read from the table')
        await ev("S47.game.d.ui.close()"); await tick(0.1)

        # papers on the real table
        await use('r6:nora'); await tick(0.1)
        await choose('end')
        text = await lines()
        check(await s4('told') and await s4('stage') == 'leave' and 'drive the road' in text and 'Do not stop on the line' in text, 'where C goes: across the highway to Roswell')
        await pg.wait_for_timeout(3200)
        await shot('f07_where_c_goes')
        await ev("S47.game.d.ui.close()"); await tick(0.2)
        card = await ev("(()=>{const v=S47.world.room6.proxies.fieldCard.getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]})()")
        await face(card[0], card[1], card[2])
        hit = await aimed()
        check(hit in ('r6:card', 'r6:letter', 'r6:correction'), f'the papers on the table can be picked up ({hit})')
        await shot('f08_table')
        check('LEAVE ROOM 6' in await objective(), 'objective: leave room 6')

        # ---------- Continue in room 6 ----------
        await ev("S47.saveNow('manual', 0)")
        await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        check('room 6' in await ev("document.querySelector('.cont-info').textContent"), 'title screen offers to continue in room 6')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        check(await ev("S47.world.area") == 'room6' and await s4('stage') == 'leave' and await s4('p11') and await s4('p12'), 'Continue brings back room 6 and the findings')
        check(await ev("S47.world.room6.objs.letter.visible && S47.world.room6.objs.correction.visible && S47.world.room6.objs.fieldCard.visible"), 'Continue puts her three papers back on the table')
        check(await ev("S47.ch4.s.exitOpen && S47.annex.exitOpen > 0.99"), 'Continue keeps the fire exit propped open')
        await shot('f09_continue')

        # ---------- out, and the end card ----------
        door = await ev("(()=>{const v=S47.world.room6.proxies.door.getWorldPosition(S47.camera.position.clone()); return [v.x,v.y,v.z]})()")
        await face(door[0], door[1], door[2])
        check(await aimed() == 'r6:door' and await label('r6:door') == 'Leave room 6', 'the door: leave room 6')
        await use('r6:door')
        for _ in range(16):
            await tick(0.25)
            if await ev("S47.world.area === 'saro'"): break
        check(await ev("S47.world.area") == 'saro' and await s4('stage') == 'complete', 'out on the motel front; chapter four complete')
        await ev("S47.hold = false")
        await pg.wait_for_function("() => { const a = document.querySelector('.endcard .after'); return !!a && getComputedStyle(a).opacity === '1'; }", polling=500)
        text = await ev("document.querySelector('.endcard').textContent")
        check('ROOM 6' in text and 'ROSWELL ROAD' in text, 'end card for chapter four')
        await shot('f10_ending')
        await ev("S47.hold = true")
        await pg.click('.endcard button:has-text("Return to the observatory")'); await pg.wait_for_timeout(400)
        await ev("S47.hold = true; S47.tick(0.3)")
        await ev("S47.game.openNotebook()")
        await pg.click('.notebook [data-t=findings]')
        ids = await ev("[...document.querySelectorAll('.notebook [data-d]')].map(b => b.dataset.d)")
        found = await ev("[...document.querySelectorAll('.notebook .nb-page.on .findings [data-d]')].map(b => b.dataset.d)")
        check(all(i in ids for i in ('e11', 'e12', 'e13')) and all(i in found for i in ('p10', 'p11', 'p12')), 'the journal has her papers, and P10 to P12 under Findings')
        await shot('f11_casefile')
        await ev("S47.game.d.ui.close(true)")

        print('\n'.join(errs[:30]) or 'no console errors/warnings')
        if errs: check(False, 'console clean')
        json.dump(checks, open(f'{OUT}/checks-chapter4.json', 'w'), indent=1)
        fails = [c for c in checks if c[0] == 'FAIL']
        print(f'{len(checks) - len(fails)} of {len(checks)} PASS')
        await b.close()
        sys.exit(1 if fails else 0)

asyncio.run(main())
