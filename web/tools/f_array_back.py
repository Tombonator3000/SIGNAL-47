# Put the array back. It goes back (story/arrayBack.ts; FORSLAG.md, 20 October, item 12).
#   - before 02:15 there is nothing to resume; in chapter one, and in chapter two before Ward has
#     asked, the console says the array is held for the supervisor, and offers no button
#   - chapter two: after Ward's first call the supervisor line rings again in the control room;
#     let ring, it stops and rings again later (not for a player in the records room, but for one in
#     the corridor outside, with a caption that says where); answered, her two lines as captions (no recording),
#     a note, and the console offers ARRAY RESUME (a task in the chapter's list)
#   - the terminal: the button, HELP, the answer; the night (and the array) waits while it is open
#   - closed: the drives start up (the motor hum), all 27 to az 042 / el 48 exactly, ON SCHEDULE in
#     the log, the status screen counts; one thing at a time (oneAtATime, no save, a second command
#     says in progress, the time away does not begin); the curve lands on it (the seen log, FELT)
#   - 47 seconds: the lamps slide into one blink, the crickets stop; at 47 every light in the field
#     goes dark, then four red flashes and seven (11), flared and lighting the near dishes red; the
#     log has the pulse group exactly 47 s after ON SCHEDULE
#   - back: all 27 in perfect step, no motor, the lamps steady; home exactly at 026 / 32, every brake;
#     the log (MOTION NO COMMAND, DRIVE CURRENT 0.0 A, 27 DRIVES AZ 026 EL 32), the note, S-03's
#     motor bus has it in its own log, the seen log's entry ends 'back'
#   - every time: a second and a third, shorter notes, the console says when it last left the
#     schedule; a try in a rest does not start the curve's rest over
#   - Continue keeps the count and writes nothing twice; a case kept in the middle of a try comes
#     back with the array at 026, counted, and the log with the rest
#   - chapter five: Ward has the count ("You tried it three times. It went back three times."),
#     only as a choice when it was tried, once
#   - the warning of the time away begun in the middle of a try (the developer menu): the dishes it
#     takes are the warning's, from where they are, the rest turn back; nothing jumps; all at 026
#     after; counted
#   - a phone 412x892 with touch: the button (the line cannot be typed), the four and seven seen from
#     the windows; no console errors or warnings
# Usage: python3 tools/f_array_back.py OUTDIR   (S47_URL for another build)
import asyncio, sys, json, os, math
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

HELPERS = """(() => {
  window.A = {
    toasts: [],
    hook() {
      const ui = S47.game.d.ui; if (ui.__hooked) return; ui.__hooked = true;
      const o = ui.toast.bind(ui); ui.toast = (t, s, c) => { A.toasts.push(t); return o(t, s, c); };
    },
    ab() { return S47.arrayBack; },
    stat() {
      const ds = S47.ext.dishes, az = ds.map((d) => d.curAz), el = ds.map((d) => d.curEl);
      return { az0: Math.min(...az), az1: Math.max(...az), el0: Math.min(...el), el1: Math.max(...el),
        moving: ds.filter((d) => d.moving).length, back: ds.filter((d) => d.back).length, rogue: ds.filter((d) => d.rogue).length };
    },
    home() { return S47.ext.dishes.every((d) => d.curAz === 26 && d.curEl === 32 && !d.moving && !d.back && !d.rogue); },
    sched() { return S47.ext.dishes.every((d) => d.curAz === 42 && d.curEl === 48 && !d.moving); },
    log() { return S47.game.ops.run('SHOW LOG').split('\\n'); },
    secs(row) { const m = /(\\d\\d):(\\d\\d):(\\d\\d)/.exec(row); return m ? +m[1] * 3600 + +m[2] * 60 + +m[3] : null; },
    until(fn, most, step) { for (let t = 0; t < most; t += step) { if (fn()) return true; S47.tick(step); } return fn(); },
    // each lamp's place in its blink, and how far apart they are (radians, the widest from the first)
    phases() { const a = S47.ext.dishArray.beacons.points.geometry.getAttribute('aBlink').array, out = []; for (let i = 1; i < a.length; i += 2) out.push(a[i]); return out; },
    spread() { const p = A.phases(); return Math.max(...p.map((v) => Math.abs(Math.atan2(Math.sin(v - p[0]), Math.cos(v - p[0]))))); },
    // at the control room's windows looking out over the array; out at the gate in the array fence
    window(pitch = 0.16) { S47.player.place(-1.0, -2.4, 0); S47.player.pitch = pitch; S47.tick(0.05); },
    gate() { S47.player.place(-1.0, -17.2, 0.06); S47.player.pitch = 0.17; S47.tick(0.05); },
    term() { S47.game['openOps'](); },
    buttons() { return [...document.querySelectorAll('.term-cmds button')].map((b) => b.dataset.c); },
    out() { return document.querySelector('.termview .term-out')?.textContent ?? ''; },
    notes(s) { return S47.game.notes.filter((n) => n.includes(s)); },
    seen() { return S47.seen.filter((e) => e.what === 'array').map((e) => ({ end: e.end, looking: e.looking, line: e.line, clock: e.clock, place: e.place })); },
  };
  return true;
})()"""

async def page(b, w, h, touch):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(240000)
    errs = []
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    init = "try{localStorage.setItem('s47.serlingNarrator','false');}catch{}"
    await pg.add_init_script(init)
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()")
    await pg.add_init_script(init)
    await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.evaluate("A.hook()")
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg, errs

async def cont(pg):
    await pg.reload()
    await pg.wait_for_selector('button[data-a=cont]:not([disabled])')
    await pg.click('button[data-a=cont]'); await pg.wait_for_function('S47.started()', polling=200)
    await pg.evaluate(HELPERS); await pg.evaluate("A.hook()")
    await pg.evaluate("S47.hold = true; S47.tick(0.5)")

async def main():
    os.makedirs(OUT, exist_ok=True)
    async with async_playwright() as p:
        b = await p.chromium.launch(args=['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required'])
        ctx, pg, errs = await page(b, 1280, 720, False)
        ev = pg.evaluate
        shot = lambda n: pg.screenshot(path=f'{OUT}/{n}.png')

        # ---------- not before ----------
        r = await ev("S47.hold = true; S47.game.ops.run('ARRAY RESUME')")
        check('NOTHING TO RESUME' in r and await ev("A.ab().stage") == 'idle', f'the prologue, before 02:15: "{r.splitlines()[-1]}"')
        await ev("S47.jump('chapter1'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        r = await ev("S47.game.ops.run('ARRAY RESUME')")
        check('ARRAY-E-HELD' in r and 'supervisor' in r and not await ev("S47.game.ops.buttons().includes('ARRAY RESUME')") and await ev("A.home()"),
              f'chapter one: held for the supervisor, no button, every dish at 026 ({r.splitlines()[0]})')
        check('ARRAY RESUME' in await ev("S47.game.ops.run('HELP')"), 'HELP lists ARRAY RESUME')

        # ---------- chapter two: Ward calls back ----------
        await ev("S47.jump('chapter2'); S47.tick(1.5); S47.game.d.ui.close(); S47.tick(0.2)")
        check(await ev("S47.ch2.ringing && !S47.ch2.ringBack"), 'chapter two: the supervisor line rings (her first call)')
        await ev("S47.game.d.inter.get('phone').use()")
        await ev("A.until(() => S47.ch2.s.answered, 14, 0.25)")
        check(await ev("S47.ch2.s.stage") == 'records' and not await ev("S47.ch2.s.arrayAsk"), 'her first call over, as it was')
        r = await ev("S47.game.ops.run('ARRAY RESUME')")
        check('ARRAY-E-HELD' in r, 'before she calls back: still held')
        rang = await ev("A.until(() => S47.ch2.ringing && S47.ch2.ringBack, 9, 0.25)")
        check(rang and any("supervisor's line again" in t for t in await ev("A.toasts")), 'a few seconds later, with the player in the control room, the line rings again')
        await ev("S47.tick(25)")
        check(not await ev("S47.ch2.ringing") and any('The ringing stops.' in t for t in await ev("A.toasts")), 'let ring, it stops')
        # (gone on to the records room: not there, where the steps on the roof want their quiet; in the
        # corridor outside the control room, yes, and the caption says where it rings)
        # (the steps on the roof kept out of it: they may come in that quiet, and then nothing else would)
        await ev("S47.roofSteps['phase'] = 'done'; S47.player.place(-3.6, 8.6, Math.PI); S47.tick(0.1)")
        check(not await ev("A.until(() => S47.ch2.ringing, 46, 0.5)"), 'in the records room she does not call back')
        await ev("A.toasts.length = 0; S47.player.place(-2.4, 5.6, 0); S47.tick(0.1)")
        check(await ev("A.until(() => S47.ch2.ringing && S47.ch2.ringBack, 2, 0.25)") and any('In the control room, the supervisor' in t for t in await ev("A.toasts")),
              'and rings again later, heard from the corridor outside the control room')
        await ev("S47.player.place(3.35, 1.45, 2.75); S47.tick(0.1)")
        await ev("A.toasts.length = 0; S47.game.d.inter.get('phone').use()")
        await ev("S47.tick(1.0)")
        cap1 = [t for t in await ev("A.toasts") if t.startswith('WARD:')]
        await ev("S47.tick(3.0)")
        cap2 = [t for t in await ev("A.toasts") if t.startswith('WARD:')]
        await shot('a00_ward_calls_back')
        await ev("A.until(() => S47.ch2.s.arrayAsk, 8, 0.25)")
        check(cap1[:1] == ['WARD: "Reyes. One more thing."'] and 'WARD: "Put the array back on the schedule and tell me it stays there."' in cap2,
              f'answered: her two lines, captioned on the line ({cap2})')
        check(await ev("S47.ch2.s.arrayAsk") and len(await ev("A.notes('Ward called back')")) == 1 and await ev("S47.game.ops.buttons().includes('ARRAY RESUME')"),
              'asked: kept, a note, and the console offers ARRAY RESUME')
        tasks = await ev("S47.ch2.tasks().map((t) => t.text + (t.done ? ' [x]' : ''))")
        check(any(t.startswith('Put the array back on the schedule') and '[x]' not in t for t in tasks), 'a task in the chapter\'s list, not done yet')

        # ---------- the terminal ----------
        await ev("A.window(); A.term()"); await pg.wait_for_timeout(300)
        await pg.click('.term-cmds button[data-c="HELP"]'); await pg.wait_for_timeout(150)
        check('ARRAY RESUME' in await ev("A.buttons()") and 'ARRAY TO SCHEDULE' in await ev("A.out()"), 'the terminal: a button, and HELP')
        c0 = await ev("S47.game.clock")
        await pg.click('.term-cmds button[data-c="ARRAY RESUME"]'); await pg.wait_for_timeout(250)
        out = await ev("A.out()")
        check('POINTING  AZ 042  EL 48' in out and 'S-01 TO S-27  ENABLED' in out and await ev("A.ab().stage") == 'armed', 'ARRAY RESUME: the schedule, the pointing, the drives enabled')
        await shot('a01_terminal')
        await ev("S47.tick(2.0)")
        check(await ev("A.ab().stage") == 'armed' and await ev("A.home()") and abs(await ev("S47.game.clock") - c0) < 0.01, 'while the terminal is open the night waits, and so does the array')
        check(await ev("S47.oneAtATime()") and await ev("S47.saveBlock()") is not None, 'one thing at a time from the command: nothing else starts, no save')
        landed = await ev("S47.curve.landed && S47.curve.landed.what")
        sn = await ev("A.seen()")
        check(landed == 'array' and await ev("S47.curve.phase") == 'rest' and len(sn) == 1, f'the seen log has it, and the night\'s curve lands on it ({landed}, {sn})')
        await ev("S47.game.d.ui.close(); S47.tick(0.05); A.window()")

        # ---------- the slew ----------
        check(await ev("A.until(() => A.ab().stage === 'slew' && S47.ext.dishes.some((d) => d.moving), 3, 0.1)"), 'closed: the drives start up')
        await ev("S47.tick(1.0)")
        m = await ev("[!!S47.audio.motor, A.stat()]")
        check(m[0] and m[1]['moving'] >= 15 and 26 < m[1]['az1'] < 42, f'the console\'s motor hum of 02:15, and they turn ({m[1]["moving"]} moving, az up to {m[1]["az1"]:.1f})')
        await ev("S47.tick(4.0)")
        await shot('a02_slewing_from_window')
        ok = await ev("A.until(() => A.ab().stage === 'hold', 25, 0.1)")
        t_on = await ev("S47.game.clock")
        check(ok and await ev("A.sched()") and not await ev("!!S47.audio.motor"), 'all 27 on the schedule, az 042 / el 48 exactly, the hum stopped')
        log = await ev("A.log()")
        check(any('ARRAY RESUME  RUN860413_2341  OPERATOR REYES' in r for r in log) and any('ON SCHEDULE  AZ 042 EL 48' in r for r in log), 'the console log: the command, and on the schedule')
        check('ON SCHEDULE  00:00:00' in (await ev("A.ab().status()"))['line'], 'the status screen counts from 00:00:00')
        r = await ev("S47.game.ops.run('ARRAY RESUME')")
        check('INPROG' in r, f'a second command: "{r}"')
        cab = await ev("(() => { const d = S47.ext.dishes.find((x) => x.id === 'S-02'); return [d.curAz, d.curEl, d.rogue, d.back]; })()")
        check(cab == [42, 48, False, False], f'S-02 on the schedule, on the console\'s track ({cab})')
        s0 = await ev("A.spread()")
        await ev("S47.tick(4.0)")
        await shot('a03_on_schedule_window')
        await ev("S47.tick(26.0)")
        await ev("S47.player.place(-2.15, -2.95, 0); S47.player.pitch = -0.62; S47.tick(0.05)")
        await shot('a04_status_screen_count')
        st = await ev("A.ab().status().line")
        await ev("S47.tick(6.0)")
        s1 = await ev("A.spread()")
        check(s0 > 0.8 and s1 < 0.05, f'the red lamps slide into one blink across the field (spread {s0:.2f} to {s1:.3f} rad) ({st})')
        await ev("A.gate()")
        await ev("S47.tick(2.0)")
        check(await ev("A.ab().hushed") and await ev("S47.space()") == 'yard', 'out at the fence; the crickets have stopped')
        check(await ev("S47.abduction().stage") == 'calm', 'the time away does not begin')

        # ---------- 47: four, seven ----------
        ok = await ev("A.until(() => A.ab().stage === 'signal', 12, 1 / 30)")
        t_sig = await ev("S47.game.clock")
        check(ok and abs((t_sig - t_on) - 47) < 0.2, f'47 seconds on the schedule ({t_sig - t_on:.2f} s)')
        await ev("S47.tick(0.3)")
        check(await ev("S47.ext.dishArray.beaconsHeld") == 0 and await ev("S47.ext.fieldNow") == 'out', 'every light out in the field at once: the red lamps, the posts, their floods')
        await shot('a05_lamps_out')
        await ev("A.until(() => A.ab().lit, 2, 1 / 30); S47.tick(0.1)")
        check(await ev("S47.ext.dishArray.beaconsHeld") > 1.5 and await ev("S47.ext.dishArray.flaring") > 1 and await ev("S47.ext.fieldNow") == 'red' and await ev("A.ab().flashes") == 1,
              'the first flash, all together: the red lamps flare round their dishes and light the near ones red')
        await shot('a06_flash_one_of_four')
        await ev("A.until(() => A.ab().flashes === 5 && A.ab().lit, 6, 1 / 30); S47.tick(0.1)")
        await shot('a07_flash_one_of_seven')
        ok = await ev("A.until(() => A.ab().stage === 'back', 8, 1 / 30)")
        check(ok and await ev("A.ab().flashes") == 11 and await ev("A.ab().got.lamps"), f'four, a gap, seven: {await ev("A.ab().flashes")} flashes, seen')
        log = await ev("A.log()")
        pg47 = [r for r in log if 'PULSE GROUP 4 / 7' in r]
        on = [r for r in log if 'ON SCHEDULE  AZ 042 EL 48' in r]
        check(len(pg47) == 1 and await ev(f"A.secs({json.dumps(pg47[0])}) - A.secs({json.dumps(on[-1])})") == 47, f'the log: the pulse group, 47 s after ON SCHEDULE ({pg47[0][:30]})')

        # ---------- back, without a sound ----------
        await ev("S47.tick(4.0)")
        s = await ev("A.stat()")
        check(s['back'] == 27 and s['az1'] - s['az0'] < 1e-6 and s['el1'] - s['el0'] < 1e-6 and 26 < s['az0'] < 42 and not await ev("!!S47.audio.motor"),
              f'all 27 turning back in perfect step, no motor (az {s["az0"]:.2f}, el {s["el0"]:.2f})')
        check(await ev("S47.ext.dishArray.beaconsHeld") == 1 and await ev("S47.ext.dishArray.flaring") == 0 and await ev("S47.ext.fieldNow") == 'on', 'the field\'s lamps back; the red ones steady, not blinking')
        await shot('a08_turning_back')
        t = await ev("A.toasts")
        check(any('All twenty-seven at once, and not a sound.' in x for x in t), '"They are turning back. All twenty-seven at once, and not a sound."')
        # what the field says of them now: the drive cabinet, the pedestal, the dish itself
        await ev("A.toasts.length = 0; S47.game.d.inter.get('field:S-02').use()")
        cab = [x for x in await ev("A.toasts") if x.startswith('S-02.')]
        mast = await ev("S47.game.d.inter.get('mast:S-02').examine()")
        look = await ev("S47.game.d.inter.get('look:S-02').examine()")
        check(cab and 'The motor draws nothing' in cab[0] and 'by itself' in mast and 'no sound from it at all' in look,
              f'the drive cabinet, the pedestal and the dish say so ("{cab[0] if cab else ""}" / "{mast}" / "{look}")')
        ok = await ev("A.until(() => A.ab().stage === 'settle', 20, 0.1)")
        check(ok and await ev("A.home()") and await ev("S47.ext.dishArray.beaconsHeld") is None, 'home: every dish exactly at az 026, el 32, the lamps blinking again')
        await ev("A.until(() => A.ab().stage === 'idle', 4, 0.1)")
        log = await ev("A.log()")
        check(any(r.endswith('ARRAY  S-01 TO S-27  MOTION  NO COMMAND') for r in log) and any(r.endswith('ARRAY  DRIVE CURRENT 0.0 A') for r in log)
              and any(r.endswith('ARRAY  27 DRIVES  AZ 026 EL 32  NO COMMAND') for r in log), 'the log: motion with no command, no drive current, all 27 home at 026')
        order = [r[:16] for r in log if r.startswith('1')]
        check(order == sorted(order, key=lambda x: (x[:2] != '13', x)), 'in the night\'s order')
        n1 = await ev("A.notes('stayed there 47 seconds')")
        check(len(n1) == 1 and 'four times, then seven' in n1[0] and 'No drive current' in n1[0], f'the note: "{n1[0][:160] if n1 else ""}..."')
        s03 = await ev("(() => { const d = S47.game.docs.find((x) => x.id === 's03log'); return d ? [d.page, d.transcript] : null; })()")
        check(s03 and 'CMD RESUME   AZ 042 EL 48' in s03[0] and 'NO COMMAND   AZ 026 EL 32' in s03[0] and 'one command' in s03[1], 'S-03\'s motor bus has it in its own log (the notebook\'s copy)')
        sn = await ev("A.seen()")
        check(len(sn) == 1 and sn[0]['end'] == 'back' and sn[0]['looking'] is True, f'the seen log: back, looked at ({sn})')
        check(not await ev("S47.oneAtATime()") and await ev("S47.saveBlock()") is None and await ev("A.ab().count") == 1 and not await ev("A.ab().hushed"), 'over: counted, the night free again, the crickets back')
        tasks = await ev("S47.ch2.tasks().map((t) => t.text + (t.done ? ' [x]' : ''))")
        check(any(t.startswith('Put the array back') and '[x]' in t for t in tasks), 'the task done')
        await ev("A.window(); A.term()"); await pg.wait_for_timeout(300)
        await pg.click('.term-cmds button[data-c="SHOW LOG"]'); await pg.wait_for_timeout(250)
        await shot('a09_show_log')
        await ev("S47.game.d.ui.close(); S47.tick(0.1)")

        # ---------- every time ----------
        await ev("S47.tick(3)")
        cv = await ev("[S47.curve.phase, S47.curve.t, S47.curve.len, S47.curve.cycle]")
        r = await ev("S47.game.ops.run('ARRAY RESUME')")
        check(r.startswith('%ARRAY-W-LEFTSCHED, left schedule') and 'POINTING' in r, f'again: the console says when it last left the schedule ({r.splitlines()[0]})')
        await ev("S47.tick(0.5)")
        cv2 = await ev("[S47.curve.phase, S47.curve.t, S47.curve.len, S47.curve.cycle]")
        check(cv2[0] == 'rest' and cv2[1] >= cv[1] and cv2[2] == cv[2], f'a try in the curve\'s rest does not start the rest over ({cv} / {cv2})')
        check(await ev("A.until(() => A.ab().stage === 'idle', 120, 0.5)") and await ev("A.home()") and await ev("A.ab().count") == 2, 'the second time: on the schedule, and back')
        check(len(await ev("A.notes('Twice now.')")) == 1, 'the note: "Twice now."')
        await ev("S47.game.ops.run('ARRAY RESUME')")
        check(await ev("A.until(() => A.ab().stage === 'idle', 120, 0.5)") and await ev("A.ab().count") == 3 and len(await ev("A.notes('The third time.')")) == 1, 'and a third: "The third time."')
        s03 = await ev("S47.game.docs.find((x) => x.id === 's03log').page")
        check(s03.count('CMD RESUME') == 3 and s03.count('NO COMMAND   AZ 026') == 3, 'S-03\'s log: three commands, three times back with none')

        # ---------- busy: refused, in the console's words ----------
        r = await ev("(() => { S47.audio.phoneSession = { sources: [], nodes: [] }; const r = S47.game.ops.run('ARRAY RESUME'); S47.audio.phoneSession = null; return r; })()")
        check('LINKBUSY' in r and await ev("A.ab().stage") == 'idle', f'with something else going (a call on the line): "{r.splitlines()[0]}"')
        r = await ev("(() => { S47.follow.mode = 'hold'; const r = S47.game.ops.run('ARRAY RESUME'); S47.follow.mode = 'idle'; return r; })()")
        check('DRVMOTION' in r and await ev("A.ab().stage") == 'idle', f'while the dishes move by themselves: "{r.splitlines()[0]}"')

        # ---------- Continue ----------
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(700)
        await cont(pg)
        k = await ev("[A.ab().count, A.home(), S47.ch2.s.arrayAsk, S47.game.ops.buttons().includes('ARRAY RESUME'), A.log().filter((r) => r.includes('PULSE GROUP')).length, ['Ward called back', 'stayed there 47 seconds', 'Twice now.', 'The third time.'].map((x) => A.notes(x).length)]")
        check(k == [3, True, True, True, 3, [1, 1, 1, 1]], f'after Continue: three tries, the array at 026, the button, the log, nothing written twice ({k})')
        # a case kept in the middle of a try (S47.saveNow writes when the menu would not)
        await ev("A.window(); S47.game.ops.run('ARRAY RESUME'); A.until(() => A.ab().stage === 'hold', 25, 0.25); S47.tick(5)")
        check(await ev("A.sched()") and await ev("S47.saveBlock()") is not None, 'on the schedule again; the menu would not save now')
        await ev("S47.saveNow('manual', 0)"); await pg.wait_for_timeout(700)
        await cont(pg)
        log = await ev("A.log()")
        k = await ev("[A.ab().count, A.home(), !!(S47.game.ns.array && S47.game.ns.array.run), A.ab().stage]")
        check(k == [4, True, False, 'idle'] and sum('PULSE GROUP' in r for r in log) == 4 and sum(r.endswith('27 DRIVES  AZ 026 EL 32  NO COMMAND') for r in log) == 4,
              f'kept in the middle: after Continue the array is at 026, the try counted, the log has the rest once ({k})')
        await ctx.close()

        # ---------- chapter five: Ward has the count ----------
        ctx, pg, errs2 = await page(b, 1280, 720, False)
        errs += errs2
        ev = pg.evaluate
        shot = lambda n: pg.screenshot(path=f'{OUT}/{n}.png')
        async def ward(runs):
            await ev(f"""S47.game.d.ui.close(); S47.game.ns.array = {{ runs: {json.dumps([{'cmd': 10000 + i * 200, 'on': 10014 + i * 200, 'lost': 10061 + i * 200, 'turn': 10070 + i * 200, 'home': 10083 + i * 200} for i in range(runs)])} }};
              S47.ch5.s.arrived = true; S47.ch5.s.ward = false; S47.ch5.s.arrayTold = false; S47.ch5['openTalk']('ward'); S47.tick(0.1)""")
            await pg.wait_for_timeout(300)
            return await ev("[...document.querySelectorAll('button.lp-btn[data-c]')].map((b) => b.dataset.c)")
        await ev("S47.hold = true; S47.jump('chapter5'); S47.tick(1.5)")
        cs = await ward(0)
        check('array' not in cs and 'road' in cs, f'never tried: Ward says nothing about it ({cs})')
        cs = await ward(3)
        check('array' in cs, 'tried three times: a choice, to tell her')
        await pg.click('button.lp-btn[data-c="array"]'); await pg.wait_for_timeout(3200)
        lines = await ev("[...document.querySelectorAll('.talk-line')].map((p) => p.textContent)")
        check(any('You tried it three times. It went back three times.' in l for l in lines) and any('Leave it where it is.' in l for l in lines), f'"You tried it three times. It went back three times." ({lines[-3:]})')
        await shot('c01_ward_count')
        cs = await ev("[...document.querySelectorAll('button.lp-btn[data-c]')].map((b) => b.dataset.c)")
        check('array' not in cs and len(await ev("A.notes('You tried it three times')")) == 1, 'once, and a note')
        cs = await ward(1)
        await pg.click('button.lp-btn[data-c="array"]'); await pg.wait_for_timeout(400)
        lines = await ev("[...document.querySelectorAll('.talk-line')].map((p) => p.textContent)")
        check(any('You tried it once. It went back once.' in l for l in lines), 'tried once: "You tried it once. It went back once."')
        await ev("S47.game.d.ui.close(); S47.tick(0.1)")

        # ---------- the warning in the middle of a try (the developer menu) ----------
        await ev("S47.jump('chapter3'); S47.tick(1.0); S47.game.d.ui.close(); S47.curve.free = true; S47.game.ns.array = undefined; S47.tick(0.2)")
        await ev("S47.flashlight.have = true; S47.flashlight.on = true")
        r = await ev("S47.game.ops.run('ARRAY RESUME')")
        check('POINTING' in r, 'chapter three: offered without her asking (the menu\'s chapter three)')
        await ev("A.window(); A.until(() => A.ab().stage === 'hold', 25, 0.25); S47.tick(8)")
        X, Z = -1, -62
        await ev(f"S47.player.place({X}, {Z}, Math.atan2(15, 10)); S47.player.pitch = 0.3; S47.tick(0.1)")
        await ev("S47.abduction().data.last = -1e9; S47.abduction().pressure = 5; S47.tick(1.0)")
        check(await ev("S47.abduction().stage") == 'calm', 'out on B among the dishes with the pressure up: the time away waits while the array is on the schedule')
        await ev("S47.abduction().begin(S47.abductIn())")
        jumps = await ev("""(() => {
          const ds = S47.ext.dishes, last = ds.map((d) => [d.curAz, d.curEl]); let worst = 0, followed = false;
          for (let i = 0; i < 30 * 30; i++) {
            S47.tick(1 / 30);
            ds.forEach((d, k) => { worst = Math.max(worst, Math.abs(((d.curAz - last[k][0] + 540) % 360) - 180), Math.abs(d.curEl - last[k][1])); last[k] = [d.curAz, d.curEl]; });
            if (S47.follow.mode === 'follow') followed = true;
          }
          return [worst, followed, S47.arrayBack.stage, S47.follow.mode];
        })()""")
        check(jumps[1] and jumps[0] < 1.5, f'the warning begun in the middle of it: the dishes follow from where they are, nothing jumps (largest step {jumps[0]:.2f} deg; {jumps[2:]})')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        ok = await ev("A.until(() => S47.follow.mode === 'idle' && A.ab().stage === 'idle', 240, 1)")
        check(ok and await ev("A.home()") and await ev("A.ab().count") == 1 and len(await ev("A.notes('when the warning came')")) == 1, 'after it: every dish at 026 / 32 exactly, the try counted, a note')
        await ctx.close()

        # ---------- a phone ----------
        ctx, pg, errs3 = await page(b, 412, 892, True)
        errs += errs3
        ev = pg.evaluate
        shot = lambda n: pg.screenshot(path=f'{OUT}/{n}.png')
        await ev("S47.hold = true; S47.jump('chapter3'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("A.window(0.12); A.term()"); await pg.wait_for_timeout(400)
        check(await ev("document.querySelector('.termview input').readOnly") and 'ARRAY RESUME' in await ev("A.buttons()"), 'phone: the line is read-only, ARRAY RESUME is a button')
        await pg.tap('.term-cmds button[data-c="ARRAY RESUME"]'); await pg.wait_for_timeout(300)
        check('POINTING  AZ 042  EL 48' in await ev("A.out()") and await ev("A.ab().stage") == 'armed', 'phone: tapped, the console answers')
        await shot('p01_phone_terminal')
        await pg.tap('.term-foot button[data-a=close]'); await pg.wait_for_timeout(300)
        await ev("S47.tick(0.1); A.window(0.12)")
        ok = await ev("A.until(() => A.ab().stage === 'signal' && A.ab().lit, 80, 0.25)")
        check(ok, 'phone: on the schedule, and at 47 the lamps')
        await shot('p02_phone_flash_from_window')
        await ev("A.until(() => A.ab().stage === 'back', 10, 1 / 30); S47.tick(5)")
        await shot('p03_phone_turning_back')
        check(await ev("A.until(() => A.ab().stage === 'idle', 30, 0.25)") and await ev("A.home()") and await ev("A.ab().count") == 1 and await ev("A.ab().got.lamps"),
              'phone: home at 026, counted, the four and seven seen from the windows')
        await ctx.close()

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:4])))
        json.dump({'checks': checks}, open(f'{OUT}/f_array_back.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
