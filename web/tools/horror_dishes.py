# The dishes come down and follow you (story/dishesFollow.ts; FORSLAG.md, 20 October, Valg for
# Tom 2, the full version). Once in a night, when the warning of the time away comes while the
# player is out in the antenna field: a breath after the warning's beep at :16 (the warning always
# lasts 47 s, story/abduction.ts, BEAT) the nearest dish lowers out of 026 and turns until it
# points at the player, then the next two; further out later and slower,
# far out late or not at all. They track the player's eyes: walking, their motors run; standing,
# they stop. Got away (indoors): they stay on the place, then go back to az 026 / el 32 exactly,
# and the console log has every drive that moved with no command and the array back home. Taken:
# at the light they turn away to the grass where the player will wake, and on waking they point
# down at it, the view lifting towards the nearest; then home. Not off the field, not with Missing
# time off, not a second time, not after Continue; a save made while they were out comes back with
# them home and the line written. Desktop pictures and a phone's (touch), with the far dishes'
# light model the near ones there. No console errors.
# Usage: python3 tools/horror_dishes.py OUTDIR   (S47_URL for another build)
import asyncio, sys, json, os, math
from intro_flow import begin_opening
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

# in the page: how far a dish's boresight is off a point (degrees: az, el against the final limit)
HELPERS = """(() => {
  window.__eye = () => ({ x: S47.player.pos.x, y: S47.player.floorY + S47.player.eye, z: S47.player.pos.z });
  window.__off = (id, p) => {
    const d = S47.ext.dishes.find((x) => x.id === id), dx = p.x - d.x, dz = p.z - d.z, dy = p.y - 12.4 * d.scale, h = Math.hypot(dx, dz);
    const az = (Math.atan2(dx, -dz) * 180 / Math.PI + 360) % 360, el = Math.max(-15, Math.atan2(dy, h) * 180 / Math.PI);
    return [Math.abs(((az - d.curAz + 540) % 360) - 180), Math.abs(el - d.curEl), h];
  };
  window.__home = () => S47.ext.dishes.every((d) => d.curAz === 26 && d.curEl === 32 && !d.moving && !d.rogue);
  window.__out = () => S47.ext.dishes.filter((d) => Math.abs(d.curAz - 26) > 0.01 || Math.abs(d.curEl - 32) > 0.01).map((d) => d.id);
  // the motors as they come out of their voices, placed (after the distance): RMS every 60 ms over secs of real time
  window.__tap = () => {
    const ctx = S47.audio.ctx; if (!ctx) return false;
    const an = ctx.createAnalyser(); an.fftSize = 4096; window.__an = an;
    for (const v of S47.follow.sound.voices) v.pan.connect(an);
    return S47.follow.sound.voices.length;
  };
  window.__levels = async (secs) => {
    const an = window.__an, buf = new Float32Array(an.fftSize), out = [], t0 = performance.now();
    while (performance.now() - t0 < secs * 1000) {
      await new Promise((r) => setTimeout(r, 60));
      an.getFloatTimeDomainData(buf); let s = 0; for (const v of buf) s += v * v; out.push(Math.sqrt(s / buf.length));
    }
    return out;
  };
})()"""

async def page(b, w, h, touch, errs):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, has_touch=touch, is_mobile=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()"); await pg.reload()
    await pg.wait_for_selector('button[data-a=start]'); await pg.click('button[data-a=start]'); await begin_opening(pg); await pg.wait_for_timeout(500)
    # (the night's one curve, story/curve.ts, stands aside in this test: each thing on its own timer; tools/f_curve.py has them together)
    await pg.evaluate("S47.hold = true; S47.curve.free = true; S47.jump('chapter3'); S47.tick(1.0)")
    await pg.evaluate("S47.game.d.ui.close(); S47.tick(0.2)")
    await pg.evaluate(HELPERS)
    return pg

def yaw_to(x, z, tx, tz): return math.atan2(-(tx - x), -(tz - z))

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        errs = []
        pg = await page(b, 1280, 720, False, errs)
        ev = pg.evaluate
        A, F = "S47.abduction()", "S47.follow"
        stage = lambda: ev(f"{A}.stage")
        mode = lambda: ev(f"{F}.mode")
        async def tick(s, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def until(fn, most, step=0.5):
            for _ in range(int(most / step)):
                if await ev(fn): return True
                await ev(f"S47.tick({step})")
            return await ev(fn)
        async def warning(length):
            await ev(f"{A}.data.last = -1e9; {A}.pressure = 5; S47.tick(0.05)")
            if await stage() == 'warning': await ev(f"{A}['warnLen'] = {length}")
            return await stage() == 'warning'
        async def escape():
            await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        c0 = await ev("S47.game.clock")
        check(await ev("__home()") and await mode() == 'idle', f'chapter three, {c0 // 3600:.0f}:{c0 % 3600 // 60:02.0f}: every dish at az 026, el 32, nothing following')
        await ev("S47.flashlight.have = true; S47.flashlight.on = true; S47.input.locked = true")

        # ---------- not where it should not ----------
        await ev("S47.world.enter('saro'); S47.player.place(-24, 200, Math.PI); S47.tick(0.3)")
        s = await ev("(() => { const s = S47.abductIn(); return [s.line, s.boost]; })()")
        check(s[0] == 'B' and s[1] == 1, f'on the shoulder south of SARO: on line B, no hurry ({s})')
        check(await warning(30), 'a warning there')
        await tick(1, 22)
        check(await stage() == 'warning' and await mode() == 'idle' and await ev("__home()") and not await ev(f"{A}.data.dishes"), 'far from the array, 22 s into it: no dish moves, nothing kept')
        await escape()
        check(await stage() == 'calm', 'indoors: it passes')
        X, Z = -1, -62   # the spine, between S-02, S-03 and S-04
        await ev(f"S47.player.place({X}, {Z}, {yaw_to(X, Z, -16, -52)}); S47.player.pitch = 0.3; S47.tick(0.3)")
        s = await ev("(() => { const s = S47.abductIn(); return [s.line, s.boost]; })()")
        check(s[0] == 'B' and s[1] == 1.5, f'out in the antenna field: on line B, and the pressure comes sooner there ({s})')
        await ev("S47.setAbductions(false)")
        await ev(f"{A}.data.last = -1e9; {A}.pressure = 5"); await tick(1, 3)
        check(await stage() == 'calm' and await mode() == 'idle' and await ev("__home()"), 'with Missing time off in the settings: no warning, no dish moves')
        await ev("S47.setAbductions(true)")

        # ---------- the moment ----------
        await pg.wait_for_timeout(3500)   # the chapter's own lines gone from the screen (real time)
        check(await warning(47), 'in the field: the warning')
        first = await ev("S47.follow.constructor.firstAt(47)")
        await tick(first - 0.4)
        check(await mode() == 'idle' and await ev("__home()"), f'the first {first - 0.4:.1f} s: only the silence')
        await tick(0.9)
        st = await ev(f"{F}.state")
        on = [r['id'] for r in st if r['on']]
        near = [r['id'] for r in st if r['tier'] == 0]
        check(await mode() == 'follow' and on == ['S-02'] and await ev("(() => { const d = S47.ext.dishes.find((x) => x.id === 'S-02'); return d.rogue && d.moving; })()"),
              f'then the nearest, S-02, starts to move by itself ({on}; near {near})')
        check(sorted(near) == ['S-02', 'S-03', 'S-04'], f'the three nearest are the unmistakable ones ({near})')
        saved = await ev(f"{A}.data.dishes")
        check(saved is not None and saved['back'] is False, f'kept with the case from the start ({saved})')
        toasts = await ev("[...document.querySelectorAll('.toast')].map((e) => e.textContent)")
        check(any('drive motor starts up' in t for t in toasts), f'"{[t for t in toasts if "drive" in t][:1]}"')
        await tick(0.5, 4)
        st = {r['id']: r for r in await ev(f"{F}.state")}
        check(st['S-02']['el'] < 30 and st['S-02']['az'] != 26, f"S-02 comes down out of 026 (az {st['S-02']['az']:.1f}, el {st['S-02']['el']:.1f})")
        await tick(0.5, 20)   # 12 s after it began, :30 (S-04 has the furthest to come round: 169 degrees)
        offs = {i: await ev(f"__off('{i}', __eye())") for i in ['S-02', 'S-03', 'S-04']}
        check(all(o[0] < 4 and o[1] < 4 for o in offs.values()), 'thirteen seconds in, all three point at the player\'s eyes (or as low as they go): ' + ', '.join(f'{k} {v[0]:.1f}/{v[1]:.1f} deg at {v[2]:.0f} m' for k, v in offs.items()))
        lims = await ev("S47.ext.dishes.filter((d) => d.rogue).map((d) => d.curEl)")
        check(min(lims) >= -15.6, f'none below the final limit, el -15 (lowest {min(lims):.1f})')
        toasts = await ev("[...document.querySelectorAll('.toast')].map((e) => e.textContent)")
        check(any('It is pointing at me' in t for t in toasts), f'"{[t for t in toasts if "pointing" in t][:1]}"')
        await pg.screenshot(path=f'{OUT}/d01_s02_on_me.png')
        st = await ev(f"{F}.state")
        far = await ev("__off('S-27', __eye())")
        check(await ev("(() => { const d = S47.ext.dishes.find((x) => x.id === 'S-27'); return d.curAz === 26 && d.curEl === 32 && !d.rogue; })()"), f'S-27, {far[2]:.0f} m off at the far end: still at 026')
        t0 = {r['id']: r['onAt'] for r in st if r['on']}
        mids = [r['id'] for r in st if r['tier'] == 1 and r['on']]
        check(mids and min(t0[m] for m in mids) > max(t0[n] for n in ['S-02', 'S-03', 'S-04']), f'further out they come later ({len(mids)} of the middle distance started, the first {min(t0[m] for m in mids) if mids else -1:.1f} s in)')
        calls0 = await ev("(() => { S47.tick(0); return S47.renderer.info.render.calls; })()")

        # walk: they follow; stand: they stop
        await ev("S47.player.yaw = 0; S47.input.keys.add('KeyW')"); await tick(0.5, 6)
        await ev("S47.input.keys.delete('KeyW')")
        moved = await ev(f"[S47.player.pos.x, S47.player.pos.z]")
        runs = await ev(f"{F}.state.filter((r) => r.tier === 0 && r.run).map((r) => r.id)")
        voiced = await ev(f"{F}.voices")
        check(abs(moved[1] - Z) > 4 and runs and voiced, f'walking up the spine ({abs(moved[1] - Z):.1f} m): their motors run ({runs}; voices {[v["id"] for v in voiced]})')
        await tick(0.5, 8)
        offs = {i: await ev(f"__off('{i}', __eye())") for i in ['S-02', 'S-03', 'S-04']}
        check(all(o[0] < 4 and o[1] < 4 for o in offs.values()), 'and turned after the player: ' + ', '.join(f'{k} {v[0]:.1f}/{v[1]:.1f}' for k, v in offs.items()))
        runs = await ev(f"{F}.state.filter((r) => r.tier === 0 && r.run && !r.push).map((r) => r.id)")
        check(not runs, f'standing still, the near ones have stopped ({runs})')
        toasts = await ev("[...document.querySelectorAll('.toast')].map((e) => e.textContent)")
        check(any('When I move, they move.' in t for t in toasts), f'"When I move, they move." ({toasts[-1:]})')
        calls1 = await ev("(() => { S47.tick(0); return S47.renderer.info.render.calls; })()")
        check(calls1 <= calls0 + 2, f'no more draw calls with the dishes turned ({calls0} and {calls1})')
        # the sound as heard: standing, then walking (real time; the warning held open meanwhile)
        await ev(f"{A}['warnLen'] = 90")
        nv = await ev("__tap()")
        await ev("S47.hold = false"); await pg.wait_for_timeout(1200)
        quiet = sorted(await ev("__levels(2.4)"))
        await ev("S47.player.yaw = Math.PI; S47.input.keys.add('KeyW')"); await pg.wait_for_timeout(500)
        loud = await ev("__levels(1.6)")
        await ev("S47.input.keys.delete('KeyW'); S47.hold = true")
        db = lambda x: 20 * math.log10(max(x, 1e-9))
        q, l = quiet[len(quiet) // 3] if quiet else 0, sum(loud) / max(1, len(loud))
        check(nv == 3 and db(l) > -45 and db(l) - db(q) > 6, f'heard through their voices: walking, the motors ({db(l):.1f} dB); standing still, quiet ({db(q):.1f} dB, most of the time)')
        await ev(f"{A}['warnLen'] = {await ev(f'{A}.warn.t') + 6}")   # the rest of it
        # many eyes: look up the field
        await ev(f"S47.player.yaw = 0; S47.player.pitch = 0.1; S47.tick(4)")
        await pg.screenshot(path=f'{OUT}/d02_the_field.png')
        nout = len(await ev("__out()"))
        check(8 <= nout <= 22, f'by the end of the warning {nout} of 27 have come out of 026')

        # ---------- got away: they stay, then home ----------
        await escape()
        check(await stage() == 'calm' and await mode() == 'hold', 'indoors: the warning passes; the dishes stay on the place')
        await tick(2)
        check(len(await ev("__out()")) == nout, 'two seconds on, still there')
        early = await ev("S47.game.notes.filter((x) => x.includes('came down out of 026') || x.includes('back on 026'))")
        check(len(early) == 1 and 'went back' not in early[0] and 'back on 026' not in early[0], 'the notebook, while they are still out: what was seen, and not yet that they went back')
        await ev("S47.player.place(-1.0, -2.4, 0); S47.player.pitch = 0.16; S47.tick(4)")   # at the control room's windows, looking out at the array
        await pg.screenshot(path=f'{OUT}/d03_from_the_window.png')
        check(await until(f"{F}.mode === 'idle'", 200, 2), 'slowly, every one goes back')
        check(await ev("__home()"), 'all 27 exactly at az 026, el 32 again, none moving: later chapters see 026')
        sv = await ev(f"{A}.data.dishes")
        check(sv and sv['back'] and 'S-02' in sv['ids'], f'kept: when, which, and back ({sv and len(sv["ids"])} drives)')
        log = await ev("S47.game.ops.run('SHOW LOG')")
        rows = log.split('\n')
        check(any('DRIVE S-02  MOTION  NO COMMAND' in r for r in rows) and any('DRIVE S-03  MOTION  NO COMMAND' in r for r in rows) and any('AZ 026 EL 32  NO COMMAND' in r for r in rows),
              'SHOW LOG: each near drive that moved with no command, and the array back at 026')
        check(any('EL FINAL LIMIT' in r for r in rows) and any(r.startswith('14-APR') and 'ARRAY  S-' in r and 'MOTION' in r for r in rows), 'and the final limit, and the ones further out')
        order = [r[:16] for r in rows if r.startswith('1')]
        check(order == sorted(order, key=lambda x: (x[:2] != '13', x)), 'in the night\'s order')
        await ev("S47.game['openOps']()"); await pg.wait_for_timeout(300)
        await pg.click('.term-cmds button[data-c="SHOW LOG"]'); await pg.wait_for_timeout(300)
        await pg.screenshot(path=f'{OUT}/d04_show_log.png')
        await ev("S47.game.d.ui.close(); S47.tick(0.2)")
        notes = await ev("S47.game.notes.slice()")
        n = [x for x in notes if 'came down out of 026' in x]
        back = [x for x in notes if 'back on 026' in x]
        check(len(n) == 1 and 'When I moved, they followed.' in n[0] and 'stayed on the place' in n[0] and len(back) == 1 and notes.index(back[0]) > notes.index(n[0]),
              f'the notebook: "{n[0][:150] if n else ""}...", and once they were home: "{back[0] if back else ""}"')

        # ---------- once ----------
        await ev(f"S47.player.place({X}, {Z}, 0); S47.tick(0.3)")
        check(await warning(30), 'another warning in the field')
        await tick(1, 15)
        check(await mode() == 'idle' and await ev("__home()"), 'not again: no dish moves')
        await escape()

        # ---------- kept after Continue ----------
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)"); await ev(HELPERS)
        kept = await ev(f"[{A}.data.dishes, S47.game.ops.run('SHOW LOG').includes('DRIVE S-02  MOTION  NO COMMAND'), __home()]")
        check(kept[0] and kept[0]['back'] and kept[1] and kept[2], f'after Continue: kept, the log with it, the array at 026')
        await ev(f"S47.world.enter('saro'); S47.player.place({X}, {Z}, 0); S47.tick(0.3)")
        check(await warning(30), 'a warning in the field after Continue')
        await tick(1, 15)
        check(await mode() == 'idle' and await ev("__home()"), 'and still not again')
        await escape()
        # a save made while they were out
        await ev(f"{A}.data.dishes = {{ clock: S47.game.clock - 30, ids: ['S-02', 'S-03'], back: false }}")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)"); await ev(HELPERS)
        kept = await ev(f"[{A}.data.dishes, S47.game.ops.run('SHOW LOG').split('\\n').filter((r) => r.includes('ARRAY  2 DRIVES  AZ 026 EL 32')).length, __home()]")
        check(kept[0]['back'] and kept[1] == 1 and kept[2], f'a save made while they were out: home after Continue, and the line that says so ({kept})')
        # saved as it began, before any drive was in the log (begin saves at once)
        await ev(f"{A}.data.dishes = {{ clock: S47.game.clock - 20, ids: [], back: false }}")
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(600)
        await pg.reload()
        await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
        await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
        await ev("S47.hold = true; S47.curve.free = true; S47.tick(0.5)"); await ev(HELPERS)
        rows = (await ev("S47.game.ops.run('SHOW LOG')")).split('\n')
        mv = [i for i, r in enumerate(rows) if r.endswith('ARRAY  MOTION  NO COMMAND')]
        hm = [i for i, r in enumerate(rows) if r.endswith('ARRAY  AZ 026 EL 32  NO COMMAND')]
        check(len(mv) == 1 and len(hm) == 1 and mv[0] < hm[0] and await ev("__home()"), f'a save made as it began: after Continue the log has them move and come home ({rows[mv[0]] if mv else ""} / {rows[hm[0]] if hm else ""})')

        # ---------- taken: they turn to where you will wake ----------
        await ev(f"delete {A}.data.dishes; {A}.data.count = 0; S47.game.clock = 3 * 3600 + 40 * 60")
        await ev(f"S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true; S47.input.locked = true; S47.player.place({X}, {Z}, {yaw_to(X, Z, -16, -52)}); S47.player.pitch = 0.3; S47.tick(0.3)")
        check(await warning(47), 'a night where they have not: the warning in the field, standing still')
        # the notebook open in the middle of it holds it, as it holds the clock: not a way out (at :20,
        # the dishes on the player since a breath after :16)
        await tick(0.5, 40)
        w0, c1 = await ev(f"{A}.warn.t"), await ev("S47.game.clock")
        await ev("S47.game.openNotebook(); S47.tick(0.5); S47.tick(0.5); S47.tick(0.5)")
        held = await ev(f"[!!S47.game.d.ui.modal, {A}.stage, {A}.warn && {A}.warn.t, {F}.mode, S47.game.clock, {F}.voices.length]")
        await ev("S47.game.d.ui.close(); S47.tick(0.05)")
        check(held[0] and held[1] == 'warning' and abs(held[2] - w0) < 0.01 and held[3] == 'follow' and abs(held[4] - c1) < 0.01 and held[5] == 0 and await stage() == 'warning',
              f'the notebook opened in the middle of it: the warning and the dishes wait with the clock, their motors quiet, and go on when it is closed ({held[1:4]}, {held[5]} voices)')
        check(await until(f"{A}.stage === 'light'", 30, 0.25), 'the warning runs its length, 47 s: the light')
        await tick(0.3)
        back = await ev(f"{A}.backAt && [{A}.backAt.x, {A}.backAt.z]")
        aims = await ev(f"{F}.state.filter((r) => r.on).map((r) => Math.hypot(r.aim[0] - {A}.backAt.x, r.aim[2] - {A}.backAt.z))")
        check(back and await mode() == 'away' and aims and max(aims) < 0.5, f'as the light comes they all turn away, to the grass where the player will wake ({back and [round(v) for v in back]}; {len(aims)} drives)')
        check(await until(f"{A}.stage === 'back'", 40, 0.25), 'taken, and back')
        await tick(0.1)
        pos = await ev("[S47.player.pos.x, S47.player.pos.z]")
        offs = await ev(f"{F}.state.filter((r) => r.on && r.tier === 0).map((r) => __off(r.id, __eye()))")
        check(await mode() == 'watch' and back and math.hypot(pos[0] - back[0], pos[1] - back[1]) < 0.5 and offs and all(o[0] < 1 and o[1] < 1 for o in offs),
              f'waking there: they are pointing down at the player lying in the grass ({[(round(o[0], 1), round(o[1], 1), round(o[2])) for o in offs]})')
        face = await ev(f"(() => {{ const r = {F}.state.filter((r) => r.on).map((r) => [r.id, __off(r.id, __eye())[2]]).sort((a, b) => a[1] - b[1])[0]; const d = S47.ext.dishes.find((x) => x.id === r[0]); const y = Math.atan2(-(d.x - S47.player.pos.x), -(d.z - S47.player.pos.z)); return [r[0], Math.abs(Math.atan2(Math.sin(y - S47.player.yaw), Math.cos(y - S47.player.yaw)))]; }})()")
        # with one of them over the player on waking (story/abduction.ts, the outlines there and Missing
        # time on), the abduction turns the player to stand it against something lighter, and the
        # dishes wait on the place until it is gone; without it the view lifts towards the nearest
        over = await ev(f"{A}.overPhase")
        if over:
            check(await mode() == 'watch', f'one of them over the player on waking ({over}): the way the player lies is the abduction\'s, and the dishes wait on the place')
            if await until(f"{A}.overPhase === 'there'", 3, 0.1): await pg.screenshot(path=f'{OUT}/d05a_over_you_and_the_dishes.png')
            gone = await until(f"{A}.overPhase === null", 6, 0.1)
            offs = await ev(f"{F}.state.filter((r) => r.on && r.tier === 0).map((r) => __off(r.id, __eye()))")
            check(gone and await mode() == 'watch' and offs and all(o[0] < 1 and o[1] < 1 for o in offs),
                  f'once it is not there, they are still pointing down at the player ({[(round(o[0], 1), round(o[1], 1)) for o in offs]})')
        else:
            check(face[1] < 0.3, f'the view lifts towards the nearest of them, {face[0]} ({face[1]:.2f} rad off)')
        await pg.wait_for_timeout(4200)   # the warning's lines gone (real time)
        await tick(3.2)
        await pg.screenshot(path=f'{OUT}/d05_waking.png')
        notes = await ev("S47.game.notes.slice()")
        check(any('When I woke' in x and 'pointing down at the grass where I lay' in x for x in notes), 'the notebook: they were pointing down at the grass where I lay')
        check(await until(f"{F}.mode === 'home'", 14, 0.5), 'a few seconds, and they let go')
        check(await until(f"{F}.mode === 'idle'", 200, 2) and await ev("__home()"), 'and go back to 026, every one exactly')
        log = await ev("S47.game.ops.run('SHOW LOG')")
        check(log.count('DRIVES  AZ 026 EL 32') >= 2, 'the log: the array back home after the time away')
        await pg.close()

        # ---------- a phone: far out in the field, where the near ones are the light model ----------
        pg = await page(b, 412, 892, True, errs)
        ev = pg.evaluate
        PX, PZ = -10, -315   # row six, between S-19 and S-22
        await ev(f"S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true; S47.player.place({PX}, {PZ}, {yaw_to(PX, PZ, -40, -300)}); S47.player.pitch = 0.32; S47.tick(0.5)")
        check(await ev("S47.player.walkable(S47.player.pos.x, S47.player.pos.z)") and await ev("S47.abductIn().line") == 'B', 'phone: on the gravel far out in the field, on B')
        await pg.wait_for_timeout(3500)
        check(await warning(47), 'phone: the warning')
        await tick(0.5, 62)   # :31, the near ones started a breath after :16
        st = await ev(f"{F}.state")
        near = sorted(r['id'] for r in st if r['tier'] == 0)
        lod = await ev(f"S47.ext.dishes.filter((d) => {json.dumps(near)}.includes(d.id)).map((d) => d.lod)")
        o19 = await ev("__off('S-19', __eye())")
        check(near and all(l == 1 for l in lod) and o19[0] < 4 and o19[1] < 4, f'phone: the near ones here are far dishes, the light model (lod {lod}): {near}; S-19 on the player ({o19[0]:.1f}/{o19[1]:.1f})')
        await pg.screenshot(path=f'{OUT}/p01_phone_s19.png')
        await tick(0.5, 20)
        offs = {i: await ev(f"__off('{i}', __eye())") for i in near}
        check(all(o[0] < 4 and o[1] < 4 for o in offs.values()), 'phone: and S-22, which had the long way round: ' + ', '.join(f'{k} {v[0]:.1f}/{v[1]:.1f}' for k, v in offs.items()))
        await ev(f"S47.player.yaw = {yaw_to(PX, PZ, 10, -360)}; S47.player.pitch = 0.22; S47.tick(0.5)")
        await pg.screenshot(path=f'{OUT}/p02_phone_s22.png')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        check(await stage() == 'calm' and await mode() == 'hold', 'phone: indoors, it passes')
        await until(f"{F}.mode === 'idle'", 200, 2)
        # and by the gate, where the floods light them (the night let go again for the picture)
        await ev(f"delete {A}.data.dishes; S47.player.place({X}, {Z}, {yaw_to(X, Z, -16, -52)}); S47.player.pitch = 0.34; S47.flashlight.on = true; S47.tick(0.3)")
        await pg.wait_for_timeout(3000)
        ok = await warning(47)
        if not ok: print('  (why not:', await ev(f"(() => {{ const s = S47.abductIn(true), a = {A}; return [a.stage, s.ok, s.line, s.enabled, s.clock, s.nextFixed, a.data.count, a.data.last, S47.game.phase, !!S47.game.d.ui.modal, S47.game.cinematic, S47.roofSteps.busy, S47.follow.mode]; }})()"), ')', flush=True)
        check(ok, 'phone: a warning by the gate')
        await tick(0.5, 54)   # :27
        check(await ev("__off('S-02', __eye())[0] < 4"), 'phone: S-02 on the player')
        await pg.screenshot(path=f'{OUT}/p03_phone_s02.png')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/horror_dishes.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
