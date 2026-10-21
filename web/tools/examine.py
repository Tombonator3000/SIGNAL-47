# The closer look (right mouse, Zoom on a phone) on more of the night's things (story/looks.ts):
# the antennas, the signs round SARO and down the highway, the papers on the desks, the motel,
# the diner and the rest of Caliche's street. The hit areas only for a closer look never stand in
# the way of using what is behind them, nothing is read through the control room's wall, and the
# motel's office answers the same before and after chapter four binds it. The flashlight flickers
# near the events (a figure at the edge of the view, the silence on the shoulder) and stops after.
# The hints about the controls are said once on a device (core/hints.ts: the first drive, the
# flashlight, the field camera), also after a reload, and the notebook's Controls tab (H) lists
# every control, the phone's first on a phone.
# Usage: python3 tools/examine.py OUTDIR [WxH]
import asyncio, sys, os, json
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
W, H = (int(v) for v in (sys.argv[2] if len(sys.argv) > 2 else '1280x800').split('x'))
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
results = []
def check(ok, msg):
    results.append((bool(ok), msg)); print(('PASS ' if ok else 'FAIL ') + msg, flush=True)

HELPERS = """
window.T = {
  // stand (dx, dz) off a point, look straight at it with the zoom held, and say what a closer look says
  look(p, dx, dz, hold = true) {
    const V = S47.three.Vector3, pl = S47.player;
    pl.place(p.x + dx, p.z + dz, 0); S47.tick(0.05);
    const c = S47.camera.position;
    pl.place(p.x + dx, p.z + dz, Math.atan2(-(p.x - c.x), -(p.z - c.z)));
    pl.pitch = Math.atan2(p.y - c.y, Math.hypot(p.x - c.x, p.z - c.z));
    S47.input.zoomHeld = hold; S47.tick(0.8);
    const e = S47.game.d.inter.examine(S47.camera), u = S47.game.d.inter.update(S47.camera);
    const hud = document.querySelector('.hud .examine');
    return { id: e ? e.it.id : null, text: e ? e.text : null, use: u ? u.id : null, hud: hud && hud.classList.contains('show') ? hud.textContent : null };
  },
  at(id) { const it = S47.game.d.inter.get(id); return it ? it.object.getWorldPosition(new S47.three.Vector3()) : null; },
  toasts: [],
  hook() {
    const ui = S47.game.d.ui;
    if (ui.__hooked) return; ui.__hooked = true;
    const o = ui.toast.bind(ui);
    ui.toast = (t, s) => { T.toasts.push(t); return o(t, s); };
  },
};
"""

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
        ev = pg.evaluate
        async def tick(s, fps=30): await ev(f"S47.tick({s}, {fps})")
        async def shot(name): await pg.screenshot(path=f'{OUT}/{name}.png')
        async def start():
            await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)
            await ev(HELPERS); await ev("S47.hold = true; S47.tick(0.3); S47.setPicture('off'); T.hook()")
        async def look(target, dx, dz, hold=True):
            return await ev(f"T.look({target}, {dx}, {dz}, {'true' if hold else 'false'})")
        await start()
        await ev("S47.jump('chapter1'); S47.tick(1.0)")

        # ---------- the antennas ----------
        r = await look("(() => { const d = S47.ext.dishes.find((d) => d.id === 'S-03'); return { x: d.x, y: 9, z: d.z }; })()", 0, 23.5)
        await shot('s03')
        check(r['id'] == 'look:S-03' and r['text'].startswith('S-03.') and 'Az ' in r['text'] and r['hud'] == r['text'],
              f"S-03 from the end of the east walk, zoomed in: {r['text']}")
        check(r['use'] is None, f"nothing to use on the antenna ({r['use']})")
        r = await look("(() => { const d = S47.ext.dishes.find((d) => d.id === 'S-02'); return { x: d.x, y: 9, z: d.z }; })()", 10, 30)
        check(r['id'] == 'look:S-02' and 'S-02.' in r['text'], f"S-02 from the yard: {r['text']}")
        r = await look("(() => { const d = S47.ext.dishes.find((d) => d.id === 'S-02'); return { x: d.x, y: 9, z: d.z }; })()", 10, 30, False)
        check(r['hud'] is None, 'without the zoom nothing is said under the crosshair')
        # every dish at 026 after the event: the text says so
        r = await look("(() => { const d = S47.ext.dishes.find((d) => d.id === 'S-03'); return { x: d.x, y: 9, z: d.z }; })()", 0, 23.5)
        az = await ev("Math.round(S47.ext.dishes.find((d) => d.id === 'S-03').curAz)")
        check((az == 26) == ('points the same way' in r['text']), f"its azimuth in the text (az {az}): {r['text']}")

        # ---------- the signs ----------
        r = await look("{ x: 8.05, y: 0.9, z: -3.4 }", 1.6, 0)
        check(r['id'] == 'look:yardSign' and 'SERVICE YARD' in r['text'], f"the service yard sign: {r['text']}")
        r = await look("{ x: -14.5, y: 0.1, z: 20.8 }", -6, 0)
        check(r['id'] == 'look:siteSign' and r['text'].startswith('SARO.'), f"the site sign by the highway: {r['text']}")
        r = await look("{ x: -12.55, y: 0.95, z: 28.3 }", -5, 0)
        check(r['id'] == 'look:quietSign' and 'QUIET ZONE' in r['text'], f"the radio quiet zone: {r['text']}")
        sp = await ev("(() => { const o = S47.scene.getObjectByName('look:speedSign'); return { x: o.position.x, y: o.position.y, z: o.position.z }; })()")
        r = await look(json.dumps(sp), 3, -6)
        await shot('speed_sign')
        check(r['id'] == 'look:speedSign' and '55' in r['text'], f"the speed limit sign down the shoulder: {r['text']}")
        ground = await ev(f"S47.world.grounds.floorAt({sp['x'] + 3}, {sp['z'] - 6})")
        check(abs(sp['y'] - 1.975 - ground) < 0.6, f"its hit area stands on the sign's posts (face {sp['y']:.2f}, ground {ground:.2f})")

        # ---------- the papers on the desks, and not through the wall ----------
        await ev("S47.player.place(0, 0, 0); S47.tick(0.2)")
        lb = await ev("T.at('logbook')")
        r = await look(json.dumps(lb), 0.9, 0.6)
        await shot('logbook')
        check(r['id'] == 'logbook' and "Dale's shift log" in r['text'] and r['use'] == 'logbook', f"the shift log, close: {r['text']} (use: {r['use']})")
        r = await look(json.dumps(lb), 6.5, 0)
        check(r['text'] is None or r['id'] != 'logbook', f"not read from 6.5 m off, through the wall ({r['id']})")
        for pid, word in [('halley', 'April 11'), ('telex', 'telex'), ('excBinder', 'EXCEPTIONS'), ('crtLeft', 'S-27')]:
            pp = await ev(f"T.at('{pid}')")
            if not pp: check(False, f'{pid}: missing'); continue
            got = None
            for dx, dz in [(0.9, 0.6), (-0.9, 0.6), (0.9, -0.6), (-0.9, -0.6), (0, 1.1), (0, -1.1), (1.1, 0), (-1.1, 0)]:
                r = await look(json.dumps(pp), dx, dz)
                if r['id'] == pid: got = r; break
            check(got and word in got['text'], f"{pid}: {got['text'] if got else None}")

        # ---------- the motel, before chapter four ----------
        for pid, word in [('iceMachine', 'ICE'), ('car', 'sedan'), ('officeDoor', 'OFFICE'), ('pool', 'pool')]:
            pp = await ev(f"(() => {{ const v = S47.world.motel.proxies['{pid}'].getWorldPosition(new S47.three.Vector3()); return {{ x: v.x, y: v.y, z: v.z }}; }})()")
            r = await look(json.dumps(pp), 3.2, 0.4)
            check(r['text'] and word in r['text'] and r['use'] is None, f"motel {pid}: {r['text']} ({r['id']}, use {r['use']})")
            if pid == 'iceMachine': await shot('motel_ice')
        sign = await ev("(() => { const v = S47.scene.getObjectByName('Sierra Motor Court').getWorldPosition(new S47.three.Vector3()); return { x: v.x, y: v.y, z: v.z }; })()")
        r = await look(json.dumps(sign), 12, -14)
        check(r['text'] and 'Sierra in red script' in r['text'], f"the motel's sign from the shoulder: {r['text']}")

        # ---------- the flashlight near the events ----------
        # (the jump down the shoulder counts as a walk: the silence may have begun by itself; start from none)
        await ev("S47.flashlight.have = true; S47.flashlight.on = true; S47.player.place(-26, 300, Math.PI); S47.player.pitch = 0; S47.input.zoomHeld = false; S47.roadside.reset(); S47.tick(0.3)")
        f0 = await ev("S47.flashlight.flick")
        await ev("S47.roadside.silenceNow({ hush: () => {}, flicker: (k) => S47.flashlight.flicker(k, 'silence') }); S47.tick(0.3)")
        f1 = await ev("S47.flashlight.flick")
        await ev("S47.roadside.reset(); S47.tick(0.3)")
        f2 = await ev("S47.flashlight.flick")
        check(f0 == 0 and f1 >= 0.39 and f2 == 0, f"the flashlight flickers through the silence on the shoulder and stops after ({f0}, {f1}, {f2})")
        # (looking 30 degrees west of the figure, so it is not looked straight at and gone)
        await ev("S47.player.place(10, -18, 0.55); S47.player.pitch = 0; S47.tick(0.2)")
        up = await ev("['S-14', 'S-11'].some((id) => S47.glimpses.now(S47.glimpseIn(), id))")
        await tick(0.2)
        f3 = await ev("S47.flashlight.flick")
        await ev("S47.glimpses.reset(); S47.tick(0.2)")
        f4 = await ev("S47.flashlight.flick")
        check(up and f3 > 0.2 and f4 == 0, f"and while a figure stands at the edge of the view ({up}: {f3}, then {f4})")
        await ev("S47.flashlight.on = false; S47.flashlight.have = false")   # it comes off the truck's seat below

        # ---------- the hints, the first time on this device ----------
        cam = await ev("(() => { S47.player.place(0, 0, 0); S47.tick(0.1); T.toasts.length = 0; S47.game.d.inter.get('fieldCamera').use(); S47.tick(0.2); return T.toasts.slice(); })()")
        check(any('C to raise' in t for t in cam), f"the field camera, the first time: the keys ({cam})")
        hints = await ev("JSON.parse(localStorage.getItem('s47.hints') || '[]')")
        check('camera' in hints, f"kept on the device ({hints})")
        async def get_in(i):
            await ev(f"T.toasts.length = 0; S47.game.d.inter.get('{i}').use()")
            for _ in range(240):
                await tick(0.25)
                if await ev("S47.world.driving"): break
                await pg.wait_for_timeout(100)
            await tick(0.5)
            return await ev("T.toasts.slice()")
        async def get_out():
            await ev("S47.world.getOutHere()")
            for _ in range(24):
                await tick(0.25)
                if not await ev("S47.world.driving"): break
            await tick(0.5)
        t1 = await get_in('truck')
        check(any('W, S, A and D' in t for t in t1) and any('motel or the diner' in t for t in t1), f"the first drive: the keys and where to stop ({t1})")
        check(any('F switches it on' in t for t in t1), 'the flashlight: which key, the first time')
        await get_out()
        t2 = await get_in('truck')
        check(not any('W, S, A and D' in t or 'motel or the diner' in t for t in t2), f"the second drive says neither again ({t2})")
        await get_out()
        hints = await ev("JSON.parse(localStorage.getItem('s47.hints') || '[]')")
        check(all(h in hints for h in ['drive', 'roam', 'flashlight', 'camera']), f"all four kept ({hints})")

        # ---------- the notebook's Controls tab ----------
        await ev("S47.hold = false"); await pg.keyboard.press('h'); await pg.wait_for_timeout(500)
        ctl = await ev("""(() => { const p = document.querySelector('.notebook [data-p=controls]');
          return p ? { on: p.classList.contains('on'), heads: [...p.querySelectorAll('.nb-ctl > h4')].map((h) => h.textContent), text: p.textContent,
            sel: document.querySelector('.notebook [data-t=controls]').getAttribute('aria-selected') } : null; })()""")
        await shot('controls')
        check(ctl and ctl['on'] and ctl['sel'] == 'true' and ctl['heads'] == ['Keyboard and mouse', 'Phone and tablet'], f"H opens the notebook at Controls ({ctl and ctl['heads']})")
        check(ctl and all(w in ctl['text'] for w in ['W A S D', 'Right mouse, held', 'Throttle and brake', 'Left thumb', 'Zoom, held', 'Get out']), 'every control is on the page, desktop and phone')
        await pg.keyboard.press('Tab'); await pg.wait_for_timeout(300)
        check(not await ev("!!document.querySelector('.notebook')"), 'Tab closes it again')
        await ev("S47.input.touchMode = true; S47.game.openNotebook()"); await pg.wait_for_timeout(300)
        heads = await ev("[...document.querySelectorAll('.notebook [data-p=controls] .nb-ctl > h4')].map((h) => h.textContent)")
        await shot('controls_phone_first')
        check(heads and heads[0] == 'Phone and tablet', f"on a phone its own controls come first ({heads})")
        await ev("S47.game.d.ui.close(); S47.input.touchMode = false; S47.hold = true")

        # ---------- a new page on the same device: not said again ----------
        await pg.reload()
        await pg.wait_for_selector('button[data-a=start]')
        await start()
        await ev("S47.jump('chapter1'); S47.tick(1.0)")
        cam = await ev("(() => { T.toasts.length = 0; S47.game.d.inter.get('fieldCamera').use(); S47.tick(0.2); return T.toasts.slice(); })()")
        check(cam and not any('C to raise' in t for t in cam), f"after a reload the camera's keys are not said again ({cam})")
        t3 = await get_in('truck')
        check(any('passenger seat' in t for t in t3) and not any('F switches' in t for t in t3) and not any('W, S, A and D' in t for t in t3),
              f"the flashlight is taken, without the key, and the drive says nothing about the keys ({t3})")
        await get_out()

        # ---------- chapter four binds the office: the same closer look, and it can be used ----------
        await ev("S47.jump('chapter4'); S47.tick(1.0)")
        pp = await ev("(() => { const v = S47.world.motel.proxies.officeDoor.getWorldPosition(new S47.three.Vector3()); return { x: v.x, y: v.y, z: v.z }; })()")
        r = await look(json.dumps(pp), 1.6, 0.2)
        check(r['id'] == 'court:officeDoor' and r['text'] and 'OFFICE' in r['text'] and r['use'] == 'court:officeDoor', f"chapter four: the office door, looked at and used ({r['id']}, use {r['use']})")

        # ---------- the diner and Caliche ----------
        await ev("S47.world.goDiner('outside')")
        for _ in range(60):
            await pg.wait_for_timeout(250)
            if await ev("S47.world.area === 'diner' && !!S47.world.town"): break
        await tick(0.5)
        for pid, word, dx, dz in [('diner:sign', 'MESA DINER', 4, 6), ('town:gasSign', 'GAS', 6, 2), ('town:letters', 'HEAVENS', -5, 1), ('town:payphone', 'payphone', 1.5, 0)]:
            pp = await ev(f"T.at('{pid}')")
            got = None
            for ddx, ddz in [(dx, dz), (dx, -dz), (-dx, dz), (-dx, -dz)]:
                r = await look(json.dumps(pp), ddx, ddz)
                if r['id'] == pid: got = r; break
            check(got and word in got['text'], f"{pid}: {got['text'] if got else r}")
        await shot('town_letters')
        await ev("S47.input.zoomHeld = false; S47.tick(0.3)")

        n = await ev("Object.keys(S47.game.d.inter.looks).length")
        m = await ev("S47.game.d.inter.items.filter((i) => i.look).length")
        check(n >= 90 and m >= 40, f"{n} texts by id, {m} hit areas only for a closer look")
        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        await b.close()
    ok = sum(1 for r in results if r[0])
    json.dump([{'ok': r[0], 'msg': r[1]} for r in results], open(f'{OUT}/examine.json', 'w'), indent=1)
    print(f"{ok} of {len(results)} PASS")
    sys.exit(0 if ok == len(results) else 1)

asyncio.run(main())
