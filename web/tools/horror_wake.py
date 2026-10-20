# Something stands over you when you wake (story/abduction.ts; FORSLAG.md, "Valg for Tom" 3):
# after the time away, on your back, the stars and no sound at all; the blink; then, at the top of
# the view, a few metres off beyond your feet, one of them with its head bent to you, the painted
# outline from the glimpses (not the figure in code). A second and a half, or until the eyes go up
# to it, and it is not there; the crickets start again, the flashlight comes on, the words and the
# buttons come back. The first time some way off and aside, the second nearer and in the middle,
# the third with no stars first and two of them. Once each time, kept with the case, never again
# after Continue; nothing with Missing time off, before or on the way; not in the save's picture,
# and the save stands you up. Desktop 1280x720 and a phone 412x892 with touch.
# Usage (from web/): python3 tools/horror_wake.py OUTDIR
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

A = "S47.abduction()"
# where each one of them is: its distance on the ground, and its head in the frame (x, y from -1 to 1)
WHERE = """(() => { const T = S47.three, c = S47.camera; c.updateMatrixWorld(true);
  const out = []; S47.scene.traverse((o) => { if (o.name === 'abduct:over' && o.visible && o.parent) {
    const h = new T.Vector3(o.position.x, o.position.y + 1.1, o.position.z).project(c);
    out.push({ d: Math.hypot(o.position.x - S47.player.pos.x, o.position.z - S47.player.pos.z), x: h.x, y: h.y, card: !!o.getObjectByName('abduct:over:card') }); } });
  return out; })()"""
HELD = "document.documentElement.classList.contains('abduct-held')"

async def new_page(b, w, h, touch, errs):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, has_touch=touch, is_mobile=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()"); await pg.reload()
    await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await pg.wait_for_timeout(500)
    await pg.evaluate("S47.hold = true; S47.jump('chapter2'); S47.tick(1.0)")
    await pg.evaluate("S47.game.d.ui.close(); S47.flashlight.have = true; S47.tick(0.2)")
    return pg

async def taken(pg, count, until=('stars', 'there')):
    """Out on B south of SARO, alone, at night; the warning cut short; on to the waking (the
    count before it picks which time it is). Returns the first stage of the waking seen."""
    ev = pg.evaluate
    for _ in range(3):
        await ev("S47.game.d.ui.close(); S47.roadside.reset(); S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        await ev(f"{A}.data.count = {count}; {A}.data.last = -1e9; S47.game.clock = 2 * 3600 + 58 * 60")
        await ev(f"{A}.pressure = 5; S47.tick(0.1)")
        if await ev(f"{A}.stage") != 'warning': continue
        await ev(f"{A}['warnLen'] = 0.5; {A}['st'] = 0.5")
        for _ in range(160):
            st, ph = await ev(f"[{A}.stage, {A}.overPhase]")
            if st == 'back' or st == 'calm': return st, ph
            if until == 'black' and st == 'black': return st, ph
            await ev("S47.tick(0.1)")
    return None, None

async def to_there(pg):
    # a frame at a time (S47.tick rounds to whole frames of 1/30 s), so the moment it comes is known
    for _ in range(120):
        if await pg.evaluate(f"{A}.overPhase") == 'there': return True
        await pg.evaluate("S47.tick(1 / 30)")
    return False

async def main():
    os.makedirs(OUT, exist_ok=True)
    errs = []
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        pg = await new_page(b, 1280, 720, False, errs)
        ev = pg.evaluate

        # ---------- Missing time off: none of it ----------
        await ev("S47.setAbductions(false); S47.roadside.reset(); S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        await ev(f"{A}.data.last = -1e9; {A}.pressure = 5"); await ev("S47.tick(1)")
        await ev("S47.tick(3)")
        check(await ev(f"{A}.stage") == 'calm' and await ev(f"{A}.overPhase") is None and not await ev(WHERE) and not await ev(HELD),
              'with Missing time off in the settings: no time away, and nobody over you')
        await ev("S47.setAbductions(true)")

        # ---------- the first time ----------
        st, ph = await taken(pg, 0)
        check(st == 'back' and ph == 'stars', f'taken, and back: on your back, the stars first ({st}, {ph})')
        pose = await ev("[S47.player.pitch, S47.player.eye, S47.flashlight.on]")
        check(pose[0] > 1.0 and pose[1] < 0.5 and not await ev(WHERE), f'looking up from the grass, and nobody there yet (pitch {pose[0]:.2f}, eye {pose[1]:.2f})')
        check(await ev(HELD) and not pose[2], 'no words or buttons on the screen, and no light: the flashlight is dark')
        await pg.screenshot(path=f'{OUT}/w1_stars.png')
        check(await to_there(pg), 'the blink, and then')
        t0 = await ev("S47.runTime()")
        await ev("S47.tick(0.05)")
        w1 = await ev(WHERE)
        check(len(w1) == 1 and w1[0]['card'] and 0.4 < w1[0]['y'] < 0.95 and abs(w1[0]['x']) < 0.8,
              f'one of them, the painted outline, its head at the top of the view ({w1})')
        check(bool(w1) and 2 <= w1[0]['d'] <= 3.6, f'standing a few metres off beyond your feet ({w1[0]["d"]:.2f} m)' if w1 else 'standing a few metres off')
        pitch = await ev("S47.player.pitch")
        check(-0.6 < pitch < 0.2 and await ev("S47.player.eye") < 0.5, f'the head lifted to look down along yourself, still lying (pitch {pitch:.2f})')
        quiet = await ev("(() => { const n = S47.audio.night; return n && n.ctx.state === 'running' ? n.quiet.gain.value : null; })()")
        check(quiet is None or quiet < 0.15, f'no sound from the night while it is there (crickets and wind at {quiet})')
        await pg.screenshot(path=f'{OUT}/w1_there.png')
        # it does not move while it is there
        p0 = await ev("(() => { const g = S47.scene.getObjectByName('abduct:over'); return [g.position.x, g.position.y, g.position.z]; })()")
        await ev("S47.tick(0.6)")
        p1 = await ev("(() => { const g = S47.scene.getObjectByName('abduct:over'); return g && g.visible ? [g.position.x, g.position.y, g.position.z] : null; })()")
        check(p1 is not None and max(abs(a - b) for a, b in zip(p0, p1)) < 1e-6, 'still, the whole time')
        for _ in range(90):
            if await ev(f"{A}.overPhase") != 'there': break
            await ev("S47.tick(1 / 30)")
        t_there = await ev("S47.runTime()") - t0
        check(1.4 <= t_there <= 1.6, f'and then not there, after about a second and a half ({t_there:.2f} s)')
        after = await ev(f"[{HELD}, S47.flashlight.on, {A}.stage, {A}.data.spots.at(-1).over]")
        check(not after[0] and after[1] and after[2] == 'back', f'the words and the buttons back, the flashlight on, and the view lifts ({after})')
        check(not await ev(WHERE) and not await ev("!!S47.scene.getObjectByName('abduct:over')"), 'it is not in the scene at all')
        check(after[3] == 1, f'kept with the case: the first time ({after[3]})')
        await pg.screenshot(path=f'{OUT}/w1_gone.png')
        # once: never again on the way up, nor after
        again = False
        for _ in range(40):
            await ev("S47.tick(0.25)")
            if await ev(WHERE) or await ev(f"{A}.overPhase") is not None: again = True
            if await ev(f"{A}.stage") == 'calm': break
        await ev("S47.tick(3)")
        again = again or bool(await ev(WHERE))
        check(not again and await ev(f"{A}.stage") == 'calm', 'once: not again on the way up, nor after it')
        # the autosave from then: the case has it, the pose stands you up
        auto = await ev("""(async () => { const m = S47.saves.list(S47.caseId()).filter((x) => x.kind === 'auto').sort((a, b) => b.savedAt - a.savedAt)[0];
          if (!m) return null; const s = await S47.saves.load(m.id); return { over: s.abduct.spots.map((x) => x.over ?? null), pitch: s.pose && s.pose.pitch, thumb: !!m.thumb }; })()""")
        check(auto is not None and auto['over'][-1] == 1 and auto['thumb'], f'the autosave keeps it ({auto})')

        # ---------- the second time: nearer; the eyes go up to it ----------
        st, ph = await taken(pg, 1)
        check(st == 'back' and ph == 'stars' and await to_there(pg), f'the second time: the stars, the blink, then ({st}, {ph})')
        await ev("S47.tick(0.05)")
        w2 = await ev(WHERE)
        check(len(w2) == 1 and bool(w1) and w2[0]['d'] < w1[0]['d'] - 0.5 and abs(w2[0]['x']) < abs(w1[0]['x']),
              f'nearer than the first time, and nearer the middle ({w2[0]["d"]:.2f} m, x {w2[0]["x"]:.2f})' if w2 else 'nearer')
        await pg.screenshot(path=f'{OUT}/w2_there.png')
        # a save while it is there: the picture without it, standing you up; Continue from it later
        await ev("S47.saveNow('manual', 1)"); await pg.wait_for_timeout(400)
        mid = await ev("""(async () => { const m = S47.saves.list(S47.caseId()).find((x) => x.kind === 'manual' && x.slot === 1);
          const s = await S47.saves.load(m.id); return { pitch: s.pose.pitch, thumb: m.thumb }; })()""")
        check(abs(mid['pitch'] + 0.06) < 1e-6, f'a save made while it is there stands you up on Continue (pitch {mid["pitch"]})')
        if mid['thumb']: open(f'{OUT}/w2_save_thumb.jpg', 'wb').write(base64.b64decode(mid['thumb'].split(',', 1)[1]))
        check(await ev(f"{A}.overPhase") == 'there' and len(await ev(WHERE)) == 1, 'and it is still there after the save')
        await ev(f"{A}.gaze(0, -0.12); S47.tick(0.05)")
        gone = await ev(f"{A}.overPhase")
        check(gone is None and not await ev(WHERE), 'the eyes go up to it, and it is not there (long before its second and a half)')
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")

        # ---------- the third time: no stars first, two of them ----------
        st, ph = await taken(pg, 2)
        check(st == 'back' and ph == 'there', f'the third time: no stars first ({st}, {ph})')
        await ev("S47.tick(0.05)")
        w3 = await ev(WHERE)
        check(len(w3) == 2 and all(w['card'] for w in w3) and max(w['y'] for w in w3) < 0.95, f'two of them ({w3})')
        await pg.screenshot(path=f'{OUT}/w3_there.png')
        await ev("S47.tick(1.6)")
        check(not await ev(WHERE) and await ev(f"{A}.data.spots.at(-1).over") == 3, 'gone, and kept: the third time')
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")
        await ev(f"{A}.pressure = 5; {A}.data.last = -1e9"); await ev("S47.tick(2)")
        check(await ev(f"{A}.stage") == 'calm' and await ev(f"{A}.data.count") == 3, 'and never a fourth')

        # ---------- Missing time turned off on the way: back as before, nobody over you ----------
        st, ph = await taken(pg, 1, until='black')
        await ev("S47.setAbductions(false)")
        for _ in range(40):
            if await ev(f"{A}.stage") == 'back': break
            await ev("S47.tick(0.1)")
        await ev("S47.tick(0.1)")
        off = await ev(f"[{A}.stage, {A}.overPhase, {A}.data.spots.at(-1).over ?? null, S47.flashlight.on, {HELD}]")
        check(off[0] == 'back' and off[1] is None and off[2] is None and off[3] and not off[4] and not await ev(WHERE),
              f'Missing time turned off on the way: you wake as before, the light on, nobody there ({off})')
        await ev("S47.setAbductions(true)")
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")

        # ---------- Missing time turned off while it is there (from the pause menu): gone at once ----------
        st, ph = await taken(pg, 1)
        ok = st == 'back' and await to_there(pg)
        await ev("S47.setAbductions(false); S47.tick(1 / 30)")
        off2 = await ev(f"[{A}.overPhase, {HELD}, S47.flashlight.on, getComputedStyle(document.querySelector('.abduct-frame')).display]")
        check(ok and off2[0] is None and not off2[1] and off2[2] and off2[3] == 'none' and not await ev(WHERE),
              f'Missing time turned off while it is there: not there any more ({off2})')
        await ev("S47.setAbductions(true)")
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")

        # ---------- Continue: kept, and not again ----------
        overs = await ev(f"{A}.data.spots.map((s) => s.over ?? null)")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.tick(0.5)")
        kept = await ev(f"{A}.data.spots.map((s) => s.over ?? null)")
        seen = False
        for _ in range(12):
            await ev("S47.tick(0.5)")
            if await ev(WHERE) or await ev(f"{A}.overPhase") is not None or await ev(HELD): seen = True
        check(kept == overs and not seen and await ev(f"{A}.stage") == 'calm', f'after Continue: kept ({kept}), and nobody over you again')
        # the save made while the second one was there
        mid_id = await ev("S47.saves.list(S47.caseId()).find((x) => x.kind === 'manual' && x.slot === 1).id")
        st2 = await ev(f"""(async () => {{ const s = await S47.saves.load({json.dumps(mid_id)}); return [s.abduct.count, s.abduct.spots.map((x) => x.over ?? null), s.pose.pitch]; }})()""")
        check(st2[0] == 2 and st2[1][-1] == 2 and st2[2] == -0.06, f'the save from while it was there holds that time, already over ({st2})')
        await pg.screenshot(path=f'{OUT}/w4_after_continue.png')
        await pg.close()

        # ---------- a phone, upright, touch ----------
        pg = await new_page(b, 412, 892, True, errs)
        ev = pg.evaluate
        st, ph = await taken(pg, 0)
        ok = st == 'back' and await to_there(pg)
        await ev("S47.tick(0.05)")
        f1 = await ev(WHERE)
        vis = await ev("[getComputedStyle(document.querySelector('.touch')).visibility, getComputedStyle(document.querySelector('.hud')).visibility]")
        check(ok and len(f1) == 1 and 0.4 < f1[0]['y'] < 0.95 and abs(f1[0]['x']) < 0.8 and vis == ['hidden', 'hidden'],
              f'phone: the first time, its head at the top of the view, no buttons over it ({f1}, {vis})')
        await pg.screenshot(path=f'{OUT}/phone_w1_there.png')
        await ev("S47.tick(1.6)")
        vis = await ev("getComputedStyle(document.querySelector('.touch')).visibility")
        check(not await ev(WHERE) and vis == 'visible', f'phone: gone, the buttons back ({vis})')
        await pg.screenshot(path=f'{OUT}/phone_w1_gone.png')
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")
        st, ph = await taken(pg, 1)
        ok = st == 'back' and await to_there(pg)
        await ev("S47.tick(0.05)")
        f2 = await ev(WHERE)
        check(ok and len(f2) == 1 and 0.3 < f2[0]['y'] < 0.95, f'phone: the second time ({f2})')
        await pg.screenshot(path=f'{OUT}/phone_w2_there.png')
        await ev("S47.tick(1.6)")
        for _ in range(60):
            if await ev(f"{A}.stage") == 'calm': break
            await ev("S47.tick(0.25)")
        st, ph = await taken(pg, 2)
        await ev("S47.tick(0.05)")
        f3 = await ev(WHERE)
        check(st == 'back' and ph == 'there' and len(f3) == 2, f'phone: the third time, two of them ({f3})')
        await pg.screenshot(path=f'{OUT}/phone_w3_there.png')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/horror_wake.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
