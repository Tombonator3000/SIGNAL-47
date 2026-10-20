# The warning always lasts 47 seconds (story/abduction.ts, WARN and BEAT; FORSLAG.md, 20 October,
# item 2), and the night's curve hurries for the 47s that wait in a place (story/curve.ts, waiting).
#   - the warning: exactly 47 s; the corner clock goes over to seconds, the minute it began and the
#     warning's own 00 to 47 (the seconds in amber), held at :47 through the light; the night's clock
#     again on waking. At :16 the beep of 02:13 and the footsteps are gone (steps heard before, none
#     after); at :31 the tone and the bought pressure come in at once and the flashlight stutters;
#     at :40 it dies with K3's click, and cannot be put on again before the light at :47
#   - no save while it runs (the save menu is not a way out); saving again after
#   - fair: from the far end of the motel's shoulder, run at the beep and you reach the office; run
#     at the pressure and you do not; from the lot's edge seven seconds are enough
#   - the dishes in the field start a breath after :16, the far ones not before :31; home at 026/32
#   - the curve: the warning lands the peak and the rest waits for the 47 s to be over
#   - the motel TV: in the office during a rest nothing comes; in a build the curve hurries (the test
#     card slips), the carrier comes within some twenty seconds, indoors, and nothing else may start;
#     not on top of another thing; witnessed, it lands the peak; once
#   - the payphone's first call: the same, outdoors in Caliche (the bell tinks in its box)
#   - desktop 1280x720 and a phone 412x892 with touch; no console errors or warnings
# Usage (from web/): python3 tools/f_count47.py OUTDIR   (S47_URL for another build)
import asyncio, sys, json, os, math, base64
from playwright.async_api import async_playwright

OUT = sys.argv[1]
URL = os.environ.get('S47_URL') or 'file://' + os.path.abspath('dist-single/index.html')
ARGS = ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist', '--autoplay-policy=no-user-gesture-required']
X = 8000   # the road area's origin (World.ROAD_ORIGIN)
checks = []
def check(ok, what):
    checks.append(('PASS' if ok else 'FAIL', what)); print(('PASS ' if ok else 'FAIL ') + what, flush=True)

A, C, F = "S47.abduction()", "S47.curve", "S47.follow"
HELPERS = """(() => {
  const a = S47.audio, ab = () => S47.abduction();
  window.__snd = []; window.__steps = 0;
  const beep = a.beep.bind(a), play = a.play.bind(a), step = a.step.bind(a);
  a.beep = (...x) => { __snd.push(['beep', x[0], ab().warn ? ab().warn.t : -1]); return beep(...x); };
  a.play = (n, o) => { __snd.push(['play', n, ab().warn ? ab().warn.t : -1]); return play(n, o); };
  a.step = (...x) => { __steps++; return step(...x); };
  window.T = {
    // on to this second of the warning (the game's own 1/30 s frames), or until it is over
    to(t) { const w = ab(); let n = 0; while (w.warn && w.warn.t < t - 1e-6 && n++ < 3000) { const r = t - w.warn.t; S47.tick(r > 0.6 ? 0.5 : 1 / 30); } return w.warn ? w.warn.t : -1; },
    until(fn, secs) { for (let i = 0; i < secs * 2; i++) { if (fn()) return true; S47.tick(0.5); } return fn(); },
    clock: () => { const e = document.querySelector('.clockline'); return [e.textContent, e.classList.contains('secs'), (e.querySelector('.ss') || {}).textContent || null]; },
    // the warning, now, here (its own timer run out)
    warn() { const w = ab(); w.data.last = -1e9; w.pressure = 5; S47.tick(1 / 30); return w.stage; },
    look(tx, ty, tz) { const pl = S47.player, c = S47.camera.position; pl.yaw = Math.atan2(-(tx - c.x), -(tz - c.z)); pl.pitch = Math.atan2(ty - c.y, Math.hypot(tx - c.x, tz - c.z)); S47.tick(1 / 30); },
    // run (W and Shift) towards each point in turn, steering, until the warning is over or the
    // last point is reached; returns where the warning stood (its second, or -1) and how it ended
    run(pts, most) {
      const pl = S47.player, w = ab(); let i = 0, n = 0;
      S47.input.keys.add('KeyW'); S47.input.keys.add('ShiftLeft');
      while (n++ < most * 30) {
        const [x, z] = pts[i];
        if (Math.hypot(x - pl.pos.x, z - pl.pos.z) < 0.6) { if (++i >= pts.length) break; continue; }
        pl.yaw = Math.atan2(-(x - pl.pos.x), -(z - pl.pos.z));
        const t = w.warn ? w.warn.t : -1;
        S47.tick(1 / 30);
        if (w.stage !== 'warning') { S47.input.keys.clear(); return { t, stage: w.stage, at: [pl.pos.x, pl.pos.z] }; }
      }
      S47.input.keys.clear();
      return { t: w.warn ? w.warn.t : -1, stage: w.stage, at: [pl.pos.x, pl.pos.z] };
    },
    may: () => Object.fromEntries(['sign', 'call', 'cutout', 'post', 'event', 'long', 'glimpse', 'abduct', 'scare'].map((k) => [k, S47.curve.may(k)])),
    // the motel TV as a fresh night has it (the tests replay it)
    clearTv() { S47.motelTv.reset(); S47.townSeen.delete('ev:tv'); for (const k of [...S47.townSeen]) if (k.startsWith('mark:')) S47.townSeen.delete(k); S47.curve.long.clear(); },
  };
  return true;
})()"""

async def page(b, w, h, touch, errs):
    ctx = await b.new_context(viewport={'width': w, 'height': h}, is_mobile=touch, has_touch=touch)
    pg = await ctx.new_page(); pg.set_default_timeout(300000)
    pg.on('console', lambda m: errs.append(f'{m.type}: {m.text}') if m.type in ('error', 'warning') else None)
    pg.on('pageerror', lambda e: errs.append('PAGEERROR: ' + str(e)))
    await pg.add_init_script("HTMLElement.prototype.requestPointerLock = function(){ return Promise.resolve(); };")
    await pg.goto(URL)
    try: await pg.wait_for_function("window.S47 && Object.keys(S47.game.d.audio.buf || {}).length >= 25", timeout=120000)
    except Exception: pass
    await pg.evaluate("localStorage.clear()"); await pg.reload()
    await pg.wait_for_selector('button[data-a=start]')
    if touch: await pg.tap('button[data-a=start]')
    else: await pg.click('button[data-a=start]')
    await pg.wait_for_function("S47.started()", polling=200)
    await pg.evaluate(HELPERS)
    await pg.add_style_tag(content='.lockhint{display:none !important}')
    return ctx, pg

def hm(c):
    c = int(c) % 86400
    return f'{c // 3600:02d}:{c % 3600 // 60:02d}'

async def main():
    os.makedirs(OUT, exist_ok=True)
    errs = []
    async with async_playwright() as p:
        b = await p.chromium.launch(args=ARGS)
        # ======================= desktop =======================
        ctx, pg = await page(b, 1280, 720, False, errs)
        ev = pg.evaluate
        async def tick(s=0.1, n=1):
            for _ in range(n): await ev(f"S47.tick({s})")
        async def shot(name, corner=False):
            await pg.screenshot(path=f'{OUT}/{name}.png')
            if corner:
                vw = await ev("innerWidth")
                await pg.screenshot(path=f'{OUT}/{name}_clock.png', clip={'x': vw - 240, 'y': 0, 'width': 240, 'height': 60})
        snd = lambda: ev("__snd.slice()")
        async def calm_down():
            await ev("T.until(() => S47.abduction().stage === 'calm', 90)")
            await ev("T.until(() => S47.follow.mode === 'idle', 300)")

        # (the night's one curve stands aside for the warning itself: the time away on its own timer;
        # the curve is tested with it further down)
        await ev("S47.hold = true; S47.curve.free = true; S47.jump('chapter2'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9")
        await ev("S47.game.clock = 2 * 3600 + 58 * 60; S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true; S47.input.locked = true")
        await ev("S47.player.place(-24, 200, Math.PI); S47.player.pitch = 0.05; S47.tick(0.3)")
        await pg.wait_for_timeout(3500)   # the chapter's own lines gone from the screen (real time)
        c0 = await ev("S47.game.clock")
        ck = await ev("T.clock()")
        check(ck[0] == hm(c0) and not ck[1], f'before: the corner clock is the night\'s, hours and minutes ({ck[0]})')

        # ---------- the warning: 47 s, the clock in seconds ----------
        check(await ev("T.warn()") == 'warning', 'on B on the shoulder south of SARO, at the top of the pressure: the warning')
        w = await ev(f"{A}.warn")
        c1 = await ev(f"{A}['warnFrom']")
        ck = await ev("T.clock()")
        check(w and w['len'] == 47, f'it lasts 47 s ({w})')
        check(ck[0] == hm(c1) + ':00' and ck[1] and ck[2] == ':00', f'at once the corner clock goes over to seconds, the warning\'s own: {ck[0]}')
        check(await ev("S47.saveBlock()") == 'Not right now. Let the moment finish first.', 'no save while it runs (the save menu is not a way out of the 47 seconds)')
        await ev("T.to(3.02)"); await shot('d01_warning_03', True)
        # walking before :16: the footsteps are heard
        await ev("T.to(9.5); __steps = 0; S47.input.keys.add('KeyW')"); await ev("T.to(15.5)"); await ev("S47.input.keys.clear()")
        s0 = await ev("__steps")
        ck = await ev("T.clock()")
        b0 = [x for x in await snd() if x[0] == 'beep' and x[2] >= 0]
        check(s0 >= 4 and not await ev(f"{A}.silent") and not b0 and ck[0] == hm(c1) + ':15', f'up to :15 the footsteps are heard ({s0} steps walking), no beep yet ({ck[0]})')
        await ev("T.to(16.01)")
        b1 = [x for x in await snd() if x[0] == 'beep' and x[2] >= 0]
        ck = await ev("T.clock()")
        check(len(b1) == 2 and {x[1] for x in b1} == {52, 1046} and all(16 <= x[2] < 16.1 for x in b1) and await ev(f"{A}.silent") and ck[0] == hm(c1) + ':16',
              f'at :16 the beep of 02:13 (52 Hz, and a small one a phone plays: {[(x[1], round(x[2], 2)) for x in b1]}), the clock at {ck[0]}')
        await shot('d02_warning_16', True)
        await ev("__steps = 0; S47.input.keys.add('KeyW')"); await ev("T.to(22)"); await ev("S47.input.keys.clear()")
        check(await ev("__steps") == 0, 'and after it, walking, not one footstep')
        await ev("T.to(30.9)")
        st = await ev(f"(() => {{ const a = {A}; return {{ tone: !!a['tone'], bed: a['bed'] ? a['bed'].gain.gain.value : null, fl: S47.flashlight['flicks'].get('event') || 0, on: S47.flashlight.on }}; }})()")
        lic = st['bed'] is not None
        check(not st['tone'] and st['fl'] == 0 and st['on'] and (not lic or st['bed'] < 0.03), f'up to :31 no tone, the pressure barely there, the flashlight steady ({st})')
        await ev("T.to(32.2)")
        st2 = await ev(f"(() => {{ const a = {A}; return {{ tone: a['tone'] ? a['tone'].gain.gain.value : null, bed: a['bed'] ? a['bed'].gain.gain.value : null, fl: S47.flashlight['flicks'].get('event') || 0, on: S47.flashlight.on }}; }})()")
        check(st2['tone'] is not None and st2['fl'] > 0.2 and st2['on'] and (not lic or st2['bed'] > 0.25),
              f'at :31 the pressure comes in at once ({"the bought bed " + str(round(st["bed"], 3)) + " to " + str(round(st2["bed"], 3)) if lic else "no bought bed: the synth tone"}, the tone {st2["tone"]:.2f}) and the flashlight stutters ({st2["fl"]:.2f})')
        await ev("T.to(39.95)")
        on39 = await ev("S47.flashlight.on")
        await ev("T.to(40.02)")
        k3 = [x for x in await snd() if x[0] == 'play' and x[1] in ('lic:rack_switch', 'switch') and x[2] >= 39.9]
        ck = await ev("T.clock()")
        check(on39 and not await ev("S47.flashlight.on") and len(k3) == 1 and 40 <= k3[0][2] < 40.1 and ck[2] == ':40', f'at :40 the flashlight dies, with the click K3 made ({k3}, {ck[0]})')
        await ev("T.to(42); S47.flashlight.toggle(); S47.tick(1 / 30)"); await ev("T.to(43.02)")
        check(not await ev("S47.flashlight.on"), 'switched on again at :42: it will not stay on; seven seconds of the dark')
        await shot('d03_dark_43', True)
        await ev("T.to(46.9)")
        ck = await ev("T.clock()")
        check(await ev(f"{A}.stage") == 'warning' and ck[2] == ':46', f'at :46 still the warning ({ck[0]})')
        await tick(1 / 30, 4)
        ck = await ev("T.clock()")
        check(await ev(f"{A}.stage") == 'light' and ck[0] == hm(c1) + ':47', f'at :47 the light, and the clock stops at {ck[0]}')
        await ev("T.until(() => S47.abduction().stage === 'taken', 6)")
        ck = await ev("T.clock()")
        check(ck[0] == hm(c1) + ':47', f'through the taking it stays at :47 ({ck[0]})')
        await ev("T.until(() => S47.abduction().stage === 'back', 30)")
        await tick(1 / 30)
        c2 = await ev("S47.game.clock")
        ck = await ev("T.clock()")
        check(ck[0] == hm(c2) and not ck[1] and c2 - c1 > 16 * 60, f'waking: the night\'s clock again, {ck[0]}, {round((c2 - c1) / 60)} minutes on')
        check(await ev("S47.saveBlock()") is None, 'and the night can be saved again')
        await ev("T.until(() => S47.abduction().stage === 'calm', 40)")
        await pg.wait_for_timeout(1500)
        await shot('d04_woke', True)

        # ---------- fair: run at the beep and you make it; at the pressure you do not ----------
        at = await ev("[S47.world.motel.at.x, S47.world.motel.at.z]")
        OFFICE = -45.6 + at[0], 24.1 + at[1]                   # well inside the office (MotelFront, zone 'office')
        FAR, NEAR = (-20.0, at[1] + 70.0), (-26.0, at[1] + 24.1)  # the shoulder south of the motel; the lot's edge by the office
        BEND = (-20.0, at[1] + 24.1)
        dist_far = math.hypot(FAR[0] - BEND[0], FAR[1] - BEND[1]) + abs(BEND[0] - (-41.2 + at[0]))
        async def race(start, at_t, pts):
            await ev("S47.input.keys.clear(); S47.game.clock = 2 * 3600 + 58 * 60; S47.flashlight.on = true")
            await ev(f"S47.player.place({start[0]}, {start[1]}, 0); S47.player.pitch = 0; S47.tick(0.2)")
            ok = await ev("T.warn()") == 'warning'
            await ev(f"T.to({at_t})")
            r = await ev(f"T.run({json.dumps(pts)}, 60)")
            return ok, r
        ok, r1 = await race(FAR, 16, [BEND, OFFICE])
        ck = await ev("T.clock()")
        check(ok and r1['stage'] == 'calm' and 16 < r1['t'] < 47 and not ck[1] and await ev("S47.flashlight.on"),
              f'from the shoulder {dist_far:.0f} m from the office door, running at the beep: in by :{r1["t"]:.1f}, and it passes (the clock {ck[0]}, the flashlight on again)')
        ok, r2 = await race(NEAR, 40, [OFFICE])
        check(ok and r2['stage'] == 'calm' and 40 < r2['t'] < 47, f'from the lot\'s edge, 18 m off, running at :40 when the light dies: in by :{r2["t"]:.1f}')
        ok, r3 = await race(FAR, 31, [BEND, OFFICE])
        left = math.hypot(r3['at'][0] - OFFICE[0], r3['at'][1] - OFFICE[1])
        check(ok and r3['stage'] == 'light' and left > 3, f'from the shoulder again, running only when the pressure comes at :31: the light at :47, {left:.0f} m short of the door')
        await calm_down()
        check(await ev(f"{A}.data.count") == 2, f'taken twice so far ({await ev(f"{A}.data.count")})')

        # ---------- the dishes on the beats ----------
        await ev(f"{A}.data.count = 0; delete {A}.data.dishes; S47.game.clock = 2 * 3600 + 58 * 60")
        await ev("S47.player.place(-1, -62, Math.atan2(-(-16 - -1), -(-52 - -62))); S47.player.pitch = 0.3; S47.flashlight.on = true; S47.tick(0.3)")
        check(await ev("T.warn()") == 'warning', 'out in the antenna field: the warning')
        await ev("T.to(17.3)")
        m17 = await ev(f"{F}.mode")
        await ev("T.to(17.7)")
        on = [r['id'] for r in await ev(f"{F}.state") if r['on']]
        check(m17 == 'idle' and on == ['S-02'], f'the first dish starts a breath after the beep, the footsteps gone: at :17.5 ({m17} at :17.3, then {on})')
        await ev("T.to(28)")
        await shot('d05_dishes_28', True)
        await ev("T.to(30.9)")
        far0 = [r for r in await ev(f"{F}.state") if r['tier'] == 2 and r['on']]
        await ev("T.to(38)")
        far1 = [r for r in await ev(f"{F}.state") if r['tier'] == 2 and r['on']]
        check(not far0 and all(r['onAt'] >= 31 - 17.5 - 0.05 for r in far1), f'the far ones not before the pressure at :31 ({len(far1)} of them out by :38)')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        check(await ev(f"{A}.stage") == 'calm' and await ev(f"{F}.mode") == 'hold', 'indoors: it passes, the dishes stay on the place')
        await ev("T.until(() => S47.follow.mode === 'idle', 300)")
        home = await ev("S47.ext.dishes.every((d) => d.curAz === 26 && d.curEl === 32 && !d.moving && !d.rogue)")
        check(home, 'and go home: every one at az 026, el 32 exactly')

        # ======================= the curve =======================
        await ev("S47.curve.free = false")
        # ---------- the warning lands the peak; the rest waits for the 47 s ----------
        await ev(f"S47.game.clock = 2 * 3600 + 58 * 60; {A}.data.count = 0; S47.player.place(-24, 200, Math.PI); S47.tick(0.2)")
        await ev(f"{C}.force('peak'); {C}['signT'] = 1e9")
        check(await ev("T.warn()") == 'warning', 'at the peak: the warning')
        await ev("T.to(20)")
        cv = await ev(f"({{ phase: {C}.phase, landed: {C}.landed && {C}.landed.what }})")
        t20, busy, mays = await ev(f"{C}.t"), await ev("S47.oneAtATime()"), await ev("T.may()")
        check(cv['phase'] == 'rest' and cv['landed'] == 'warning' and t20 < 0.01 and busy and not any(mays.values()),
              f'the warning is the peak: the rest begins and waits for it (20 s in, {t20:.2f} s of rest), and nothing else may start ({cv})')
        await ev("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        await tick(1, 3)
        check(await ev(f"{A}.stage") == 'calm' and await ev(f"{C}.t") > 1, 'got away: now the rest counts')

        # ---------- the motel TV ----------
        await ev("S47.jump('chapter4'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9")
        await ev("S47.game.clock = 4 * 3600 + 10 * 60; S47.world.placeTruck('motel'); S47.world.enter('saro'); S47.tick(0.2); T.clearTv()")
        motel = await ev("(() => { const m = S47.world.motel, v = m.tv.group.getWorldPosition(new S47.three.Vector3()), a = m.anchors.officeInside; return { tv: [v.x, v.y, v.z], a: [a.x, a.z] }; })()")
        tv, a = motel['tv'], motel['a']
        into = f"S47.player.place({a[0]}, {a[1]}, 0); S47.tick(0.05); T.look({tv[0]}, {tv[1]}, {tv[2]})"
        await ev(f"{C}.force('rest')"); await ev(into)
        r0 = await ev(f"{C}.t")
        await tick(1, 30)
        st = await ev(f"({{ mode: S47.motelTv.mode, waiting: S47.motelTv.waiting, pulled: {C}.pulled, phase: {C}.phase, t: {C}.t, long: {C}.may('long') }})")
        check(st['mode'] == 'card' and st['waiting'] and st['pulled'] and st['phase'] == 'rest' and 28 < st['t'] - r0 < 32 and not st['long'],
              f'in the office with the truck on the lot, in a rest: it waits, but nothing comes, and the rest is not cut short ({st})')
        # not on top of another thing: at the peak, while steps go on behind
        await ev(f"{C}.force('peak')")
        stp = await ev("S47.nightEvents.now('steps', S47.nightIn())")
        await tick(1 / 30, 3)
        m1, busy = await ev("S47.motelTv.mode"), await ev("S47.oneAtATime()")
        await ev("T.until(() => !S47.oneAtATime(), 10)")
        await tick(0.5, 2)
        m2 = await ev("S47.motelTv.mode")
        check(stp and busy and m1 == 'card' and m2 == 'carrier', f'at the peak with steps going on behind: the test card while they last ({m1}), then the carrier ({m2})')
        await ev("T.clearTv()"); await tick(0.5, 2)
        # the build: the curve hurries for it, indoors too
        # (the flashlight off for the picture: its beam on the screen washes the picture out)
        await ev(f"{C}.force('build', 0.1); {C}['signT'] = 1e9; S47.flashlight.on = false"); await ev(into)
        await ev(f"(() => {{ const p = S47.player; p.place(p.pos.x + ({tv[0]} - p.pos.x) * 0.35, p.pos.z + ({tv[2]} - p.pos.z) * 0.35, p.yaw); S47.tick(0.05); T.look({tv[0]}, {tv[1]}, {tv[2]}); }})()")
        L = await ev(f"{C}.len")
        rolls, unrest, lv, mays, slip = 0, 0, [], [], False
        for i in range(60):
            await tick(1 / 3, 1)
            s = await ev(f"({{ mode: S47.motelTv.mode, roll: !!S47.motelTv['roll'], u: {C}.unrest, level: {C}.level, phase: {C}.phase, may: T.may() }})")
            if s['roll']:
                rolls += 1
                if not slip:
                    slip = True; await shot('d06_tv_slip')
                    url = await ev("S47.world.motel.tv.canvas.toDataURL('image/png')")
                    open(f'{OUT}/d06_tv_slip_screen.png', 'wb').write(base64.b64decode(url.split(',', 1)[1]))
            unrest = max(unrest, s['u']); lv.append(s['level'])
            if s['mode'] == 'card': mays.append(s['may'])
            if s['mode'] != 'card': break
        secs = len(lv) / 3
        normal = (0.6 - 0.1) * L / 0.35
        check(s['mode'] == 'carrier' and secs < 24 and s['level'] >= 0.55, f'in a build, in the office: the curve hurries (level 0.10 to {s["level"]:.2f}), and the carrier comes after {secs:.0f} s (at the indoor pace about {normal:.0f} s)')
        check(rolls > 0 and unrest > 0.4, f'meanwhile the test card slips: the picture rolls ({rolls} of {len(lv)} looks), the curve\'s unrest up to {unrest:.2f}')
        check(all(not m['glimpse'] and not m['event'] and not m['call'] and not m['scare'] and not m['abduct'] and not m['cutout'] for m in mays),
              'and while it hurries for the TV nothing else may take the peak (no glimpse, event, call, scare, warning or cutout)')
        await shot('d07_tv_carrier')
        n0 = len(await ev("S47.game.notes"))
        await ev("T.until(() => S47.motelTv.mode !== 'carrier', 50)")
        await tick(0.5, 2)
        nn = (await ev("S47.game.notes"))[n0:]
        cv = await ev(f"({{ phase: {C}.phase, landed: {C}.landed && {C}.landed.what, long: [...{C}.long] }})")
        check(await ev("S47.townSeen.has('ev:tv')") and any('Motel office TV' in n for n in nn) and cv['phase'] == 'rest' and cv['landed'] == 'tv' and 'ch4' in cv['long'],
              f'47 s of the carrier and the frame, seen: written down, and that is the peak ({cv})')
        await tick(1, 30)
        check(await ev("S47.motelTv.mode") == 'card' and not await ev("S47.motelTv.waiting") and not await ev(f"{C}.pulled"), 'once: the test card, nothing waiting any more')

        # ---------- the payphone's first call ----------
        await ev("S47.jump('chapter2'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await ev("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9")
        await ev("window.__p = S47.world.prepare('diner')")
        for _ in range(150):
            if await ev("!!S47.world.town && !!S47.world.diner"): break
            await pg.wait_for_timeout(200)
        await ev("S47.game.clock = 3 * 3600; for (const k of [...S47.townSeen]) if (k.startsWith('mark:') || k.startsWith('ev:phone')) S47.townSeen.delete(k); S47.curve.long.clear()")
        await ev(f"S47.world.enter('diner'); S47.player.place({X - 12}, 2064, Math.PI / 2); S47.tick(0.2)")
        await ev(f"{C}.force('rest')")
        await tick(1, 30)
        st = await ev(f"({{ now: S47.townEvents.now, waiting: S47.townEvents.waiting, pulled: {C}.pulled, phase: {C}.phase, tinks: S47.townEvents.tinks }})")
        check(st['now'] == 'none' and st['waiting'] and st['pulled'] and st['phase'] == 'rest' and st['tinks'] == 0, f'by the payphone at 03:00 in a rest: it waits, it does not ring, no tink ({st})')
        await ev(f"{C}.force('build', 0.05); {C}['signT'] = 1e9")
        L = await ev(f"{C}.len")
        n = 0
        for n in range(1, 120):
            await tick(1 / 3, 1)
            if await ev("S47.townEvents.now") != 'none': break
        secs, tinks = n / 3, await ev("S47.townEvents.tinks")
        check(await ev("S47.townEvents.now") == 'ringing' and secs < 26, f'in a build, by the payphone: it rings after {secs:.0f} s (at its own pace outdoors about {(0.6 - 0.05) * L:.0f} s)')
        check(tinks >= 1, f'meanwhile its bell tinks in the box, by itself ({tinks})')
        await ev("S47.game.d.inter.get('town:payphone').use(); S47.tick(0.2)")
        await ev("T.until(() => S47.townEvents.now === 'none', 60)")
        cv = await ev(f"({{ phase: {C}.phase, landed: {C}.landed && {C}.landed.what, long: [...{C}.long] }})")
        check(await ev("S47.townSeen.has('ev:phone1')") and cv['phase'] == 'rest' and cv['landed'] == 'phone' and 'ch2' in cv['long'], f'answered: 47 s of the printer, the nickel, and that is the peak ({cv})')
        await tick(1, 30)
        check(await ev("S47.townEvents.now") == 'none' and not await ev("S47.townEvents.waiting"), 'once: it does not ring again')
        await ctx.close()

        # ======================= phone =======================
        ctx2, pp = await page(b, 412, 892, True, errs)
        e2 = pp.evaluate
        async def shot2(name, corner=False):
            await pp.screenshot(path=f'{OUT}/{name}.png')
            if corner: await pp.screenshot(path=f'{OUT}/{name}_clock.png', clip={'x': 412 - 200, 'y': 0, 'width': 200, 'height': 110})
        await e2("S47.hold = true; S47.curve.free = true; S47.jump('chapter2'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await e2("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9")
        await e2("S47.game.clock = 2 * 3600 + 58 * 60; S47.world.enter('saro'); S47.flashlight.have = true; S47.flashlight.on = true")
        await e2("S47.player.place(-24, 200, Math.PI); S47.player.pitch = 0.05; S47.tick(0.3)")
        await pp.wait_for_timeout(3500)
        check(await e2("T.warn()") == 'warning', 'phone: the warning')
        await e2("T.to(5.02)")
        box = await e2("(() => { const e = document.querySelector('.clockline'), r = e.getBoundingClientRect(), n = document.querySelector('.touch .notes').getBoundingClientRect(); return { text: e.textContent, l: r.left, r: r.right, b: r.bottom, nt: n.top, w: innerWidth, fs: parseFloat(getComputedStyle(e).fontSize) }; })()")
        check(box['text'].endswith(':05') and box['l'] > box['w'] / 2 and box['r'] <= box['w'] and box['b'] <= box['nt'] and box['fs'] >= 20,
              f'phone: the clock in seconds fits its corner, clear of the Notes button, {box["fs"]:.0f} px ({box})')
        await shot2('p01_warning_05', True)
        await e2("T.to(43.02)")
        await shot2('p02_dark_43', True)
        check(not await e2("S47.flashlight.on") and (await e2("T.clock()"))[2] == ':43', 'phone: dark at :43, the seconds counting')
        await e2("S47.input.keys.clear(); S47.player.place(S47.room.spawn.x, S47.room.spawn.z, 0); S47.tick(0.3)")
        check(await e2(f"{A}.stage") == 'calm' and not (await e2("T.clock()"))[1], 'phone: indoors before the light, it passes; the clock is the night\'s again')
        # the slipping card on a phone
        await e2("S47.curve.free = false; S47.jump('chapter4'); S47.tick(1.0); S47.game.d.ui.close(); S47.tick(0.2)")
        await e2("S47.nightEvents['wait'] = 1e9; S47.glimpses['wait'] = 1e9; S47.calls.next = 1e9; S47.cutouts['wait'] = 1e9")
        await e2("S47.game.clock = 4 * 3600 + 10 * 60; S47.world.placeTruck('motel'); S47.world.enter('saro'); S47.tick(0.2); T.clearTv()")
        motel = await e2("(() => { const m = S47.world.motel, v = m.tv.group.getWorldPosition(new S47.three.Vector3()), a = m.anchors.officeInside; return { tv: [v.x, v.y, v.z], a: [a.x, a.z] }; })()")
        tv, a = motel['tv'], motel['a']
        await e2(f"{C}.force('build', 0.1); {C}['signT'] = 1e9; S47.flashlight.on = false; S47.player.place({a[0]}, {a[1]}, 0); S47.tick(0.05); T.look({tv[0]}, {tv[1]}, {tv[2]})")
        got = False
        for _ in range(90):
            await e2("S47.tick(1 / 3)")
            if await e2("!!S47.motelTv['roll']") and not got: got = True; await shot2('p03_tv_slip')
            if await e2("S47.motelTv.mode") != 'card': break
        sl = await e2("S47.motelTv.slips")
        check(sl > 0 and await e2("S47.motelTv.mode") == 'carrier', f'phone: in the office the test card slips as the curve hurries ({sl} rolls{", one in the picture" if got else ""}), then the carrier')

        noise = [e for e in errs if 'favicon' not in e and 'GPU stall' not in e]
        check(not noise, 'no console errors or warnings' + ('' if not noise else ': ' + '; '.join(noise[:6])))
        json.dump({'checks': checks}, open(f'{OUT}/f_count47.json', 'w'), indent=1)
        print(f"{sum(1 for c in checks if c[0] == 'PASS')} of {len(checks)} PASS")
        await b.close()
        sys.exit(0 if all(c[0] == 'PASS' for c in checks) else 1)

asyncio.run(main())
